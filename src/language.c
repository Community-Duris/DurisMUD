/*
 * ***************************************************************************
 *  File: language.c                                         Part of Duris *
 *  Usage: handle 'foreign' languages
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * 
 * *************************************************************************** 
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "comm.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"


/*
 * external variables 
 */

extern const char *language_names[];
extern const struct stat_data stat_factor[];
extern int language_base[][TONGUE_GOD];

char    *makedrunk(char *, P_char);
void     keep_char(int);
void     trans_char(int);

/*
 * Muhahaa, main code 
 */

int npc_get_pseudo_spoken_language(P_char ch)
{
  if (IS_HUMANOID(ch))
    /* wow, a genuine PC race. let's ronk. :) */
    return language_base[GET_RACE(ch) - 1][0];
  else if (ch->following)
    if (IS_PC(ch->following))
      return language_base[GET_RACE(ch->following) - 1][0];
    else
      return npc_get_pseudo_spoken_language(ch->following);
  else if (EVILRACE(ch))
    return TONGUE_ORC;
  else
    return TONGUE_COMMON;
}

int npc_get_pseudo_language_skill(P_char ch, int lang)
{
  int      s;

  if (IS_PC(ch))
    return 0;

  if (IS_HUMANOID(ch))
  {
    s = language_base[GET_RACE(ch) - 1][lang];
    if (s > 100)
      s = s % 100 + (STAT_INDEX(GET_C_INT(ch)) * s / 100);
    if (s > 100)
      s = 100;
    return s;
  }
  else
    return 90;
}

void language_show(P_char ch)
{
  int      a, lang;
  char     buf[512];

  if (IS_NPC(ch))
    return;

  act("Proficiencies in languages:", TRUE, ch, 0, 0, TO_CHAR);
  for (a = 1; a < TONGUE_LASTHEARD; a++)
    if ((lang = GET_LANGUAGE(ch, a)) >= 20)
    {
      if (lang < 40)
        sprintf(buf, "You have a basic understanding of %s.",
                language_names[a - 1]);
      else if (lang < 60)
        sprintf(buf, "You comprehend %s%s.",
                language_names[a - 1], lang < 55 ? " somewhat" : "");
      else if (lang < 90)
        sprintf(buf, "You are quite fluent in %s.", language_names[a - 1]);
      else
        sprintf(buf, "You are a master of %s.", language_names[a - 1]);
      act(buf, TRUE, ch, 0, 0, TO_CHAR);
    }
  if (GET_LANGUAGE(ch, 0) >= 1 && GET_LANGUAGE(ch, 0) <= TONGUE_GOD)
    sprintf(buf, "Currently speaking: %s",
            language_names[GET_LANGUAGE(ch, 0) - 1]);
  else
    sprintf(buf,
            "Currently speaking impossible language. Please contact someone about this.");
  act(buf, TRUE, ch, 0, 0, TO_CHAR);
}

void do_speak(P_char ch, char *argument, int cmd)
{
  char     buf[512];
  int      a;

  if (IS_NPC(ch))
    return;

  if (!argument || !*argument)
  {
    language_show(ch);
    return;
  }
  one_argument(argument, buf);
  a = search_block(buf, language_names, FALSE);
  if ((a < 0) || (a >= TONGUE_GOD))
  {
    send_to_char("Speak what language from now on?\r\n", ch);
    return;
  }
  if (GET_LANGUAGE(ch, a + 1) >= 20)
  {
    GET_LANGUAGE(ch, 0) = a + 1;
    sprintf(buf, "Okay, now speaking %s.\r\n", language_names[a]);
    send_to_char(buf, ch);
  }
  else
    send_to_char("You don't know that language well enough to speak it!\r\n",
                 ch);
}

void init_defaultlanguages(P_char ch)
{
  int      a, b;
  byte     c;

  if (IS_NPC(ch))
    return;
  if ((GET_RACE(ch) < 1 || GET_RACE(ch) > LAST_RACE) && !IS_TRUSTED(ch))
  {
    send_to_char
      ("Your race seems to be .. weird. Please contact a coder about it.\r\n",
       ch);
    return;
  }
  if (!GET_LANGUAGE(ch, 0) ||
      (!IS_TRUSTED(ch) && GET_LANGUAGE(ch, 0) >= TONGUE_GOD))
    GET_LANGUAGE(ch, 0) = language_base[GET_RACE(ch) - 1][0];
  if (!IS_TRUSTED(ch))
    for (a = 1; a < TONGUE_GOD; a++)
      if (GET_LANGUAGE(ch, a) > 0 && !language_base[GET_RACE(ch) - 1][a])
        GET_LANGUAGE(ch, a) = 0;
  for (a = 1; a < TONGUE_GOD; a++)
    if (IS_TRUSTED(ch))
      GET_LANGUAGE(ch, a) = 100;
    else
    {
      b = language_base[GET_RACE(ch) - 1][a];
      c = 0;                    /* GET_LANGUAGE (ch, a); */
      if (b > 100)
        b = b % 100 + (STAT_INDEX(GET_C_INT(ch)) * b / 100);
      if (b > 100)
        b = 100;
      if (c < b)
        c = (int) b;
      if (c > 100)
        c = 100;
      GET_LANGUAGE(ch, a) = c;
    }
  if (IS_TRUSTED(ch))
    GET_LANGUAGE(ch, TONGUE_GOD) = 100;
  else
    GET_LANGUAGE(ch, TONGUE_GOD) = 0;

  if (EVIL_RACE(ch))
    GET_LANGUAGE(ch, TONGUE_ORC) = MAX(GET_LANGUAGE(ch, TONGUE_ORC),
                                       number(30, 50) + GET_LEVEL(ch));
  else
    GET_LANGUAGE(ch, TONGUE_COMMON) = MAX(GET_LANGUAGE(ch, TONGUE_COMMON),
                                          number(30, 50) + GET_LEVEL(ch));

}

char    *language_known(P_char ch, P_char vict)
{
  static char kala[256];

#ifdef LANGUAGE_CRYPT
  int      a;

  kala[0] = '\0';
  if (IS_TRUSTED(ch) ||
      (IS_PC(ch) &&
       ((GET_LANGUAGE(ch, 0) < 1) || (GET_LANGUAGE(ch, 0) > TONGUE_GOD))))
    return kala;
  if (IS_NPC(GET_PLYR(ch)))
    a = npc_get_pseudo_spoken_language(ch);
  else
    a = GET_LANGUAGE(ch, 0);
  if ((a > 0) && (a <= TONGUE_GOD))
    if (affected_by_spell(ch, SPELL_COMPREHEND_LANGUAGES) ||
        ((IS_PC(vict) && (GET_LANGUAGE(vict, a) >= (20 + number(1, 20))))))
      sprintf(kala, "in %s ", language_names[a - 1]);
#else
  strcpy(kala, "");
#endif
  return kala;
}

void language_gain(P_char ch, P_char vict, int tongue)
{
#ifdef LANGUAGE_CRYPT

  if (IS_NPC(vict))
    return;

  if (!number(0, 19) && (number(50, 150) < (GET_C_INT(vict) - 15)) &&
      GET_LANGUAGE(vict, tongue) && (GET_LANGUAGE(vict, tongue) < 98) &&
      (GET_LANGUAGE(ch, tongue) > 75))
    GET_LANGUAGE(vict, tongue)++;

  if (((tongue == TONGUE_ORC) && EVIL_RACE(vict)) ||
      ((tongue == TONGUE_COMMON) && !EVIL_RACE(vict)))
    GET_LANGUAGE(vict, tongue) =
      MAX(GET_LEVEL(vict) + 20, GET_LANGUAGE(vict, tongue));

#endif
}

struct translation_table
{
  const char *OrigString;
  const char *NewString;
};

/* fill this table with language transforms. */
struct translation_table language_table[] = {
  {"mine", "myne"},
  {"Mine", "Myne"},
  {"that", "thaet"},
  {"That", "Thaet"},
  {"this", "thys"},
  {"This", "Thys"},
  {"the", "thea"},
  {"The", "Thea"},
  {"you", "you"},
  {"You", "You"},
  {"me", "me"},
  {"Me", "Me"},
  {"a", "e"},
  {"A", "E"},
  {"b", "c"},
  {"B", "C"},
  {"c", "d"},
  {"C", "D"},
  {"d", "f"},
  {"D", "F"},
  {"e", "i"},
  {"E", "I"},
  {"f", "g"},
  {"F", "G"},
  {"g", "h"},
  {"G", "H"},
  {"h", "j"},
  {"H", "J"},
  {"i", "o"},
  {"I", "O"},
  {"j", "k"},
  {"J", "K"},
  {"k", "l"},
  {"K", "L"},
  {"l", "m"},
  {"L", "M"},
  {"m", "n"},
  {"M", "N"},
  {"n", "p"},
  {"N", "P"},
  {"o", "u"},
  {"O", "U"},
  {"p", "q"},
  {"P", "Q"},
  {"q", "r"},
  {"Q", "R"},
  {"r", "s"},
  {"R", "S"},
  {"s", "t"},
  {"S", "T"},
  {"t", "v"},
  {"T", "V"},
  {"u", "y"},
  {"U", "Y"},
  {"v", "w"},
  {"V", "W"},
  {"w", "x"},
  {"W", "X"},
  {"x", "z"},
  {"X", "Z"},
  {"y", "a"},
  {"Y", "A"},
  {"z", "b"},
  {"Z", "B"},
  {"", ""}                      /*table must end with empty string */
};

static char *ntstr;
static char *str;

void trans_char(int num)
{
  int      i;

  for (i = 0; (i < num) && (*str != '\0'); i++)
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

void keep_char(int num)
{
  int      i;

  for (i = 0; (i < num) && (*str != '\0'); i++)
  {
    *ntstr = *str;
    str++;
    ntstr++;
  }
}

/* transformation routine */
/* ch is the one who speaks.. victim the one who reads .. */
char    *language_CRYPT(P_char ch, P_char victim, char *message)
{
  char     translation[MAX_INPUT_LENGTH];
  static char string[MAX_INPUT_LENGTH];
  int      len, learned, i, ch_skill, vict_skill;
  ulong    ttl;

  strncpy(string, message, MAX_INPUT_LENGTH);
  strncpy(string, makedrunk(string, ch), MAX_INPUT_LENGTH);

  ntstr = translation;
  str = string;

  if (string[0] == 0)
  {
    strcpy(string, "");
    return string;
  }
  if ((ch == NULL) || (victim == NULL))
  {
    strcpy(string, "");
    return string;
  }

  /* temp for stability check */
//  if (IS_NPC(ch) || (GET_RACEWAR(ch) == GET_RACEWAR(victim)) || IS_TRUSTED(ch))
//    return string;
  if (IS_TRUSTED(ch) || IS_TRUSTED(victim) || IS_NPC(ch) || IS_NPC(victim) ||
      affected_by_spell(victim, SPELL_COMPREHEND_LANGUAGES))
    return string;
  if (((IS_TRUSTED(ch) && GET_LANGUAGE(ch, 0) != TONGUE_GOD)) ||
      IS_TRUSTED(victim))
    return string;
  if (IS_PC(ch) &&
      (GET_LANGUAGE(ch, 0) < 1 || GET_LANGUAGE(ch, 0) > TONGUE_GOD))
    return string;

  /* below this line is an autotranslator for sameside, disabling it -Raxxel */
  if (IS_RACEWAR_EVIL(ch) && IS_RACEWAR_EVIL(victim))
    return string;
  if (IS_RACEWAR_GOOD(ch) && IS_RACEWAR_GOOD(victim))
    return string;
  if (IS_RACEWAR_UNDEAD(ch) && IS_RACEWAR_UNDEAD(victim))
    return string;

  if (IS_HARPY(ch) && IS_HARPY(victim))
    return string;


  if (IS_PC(ch))
    ch_skill = GET_LANGUAGE(ch, GET_LANGUAGE(ch, 0));
  else
    ch_skill = 100;

  ch_skill = 0;
  /* listening is easier than speaking */
  if (IS_PC(victim))
    vict_skill = GET_LANGUAGE(victim, GET_LANGUAGE(ch, 0)) + 10;
  else                          /* npc's listen at 100% */
    vict_skill = 100;

/* works across racewars again */
/*  if ((IS_RACEWAR_EVIL(ch) && IS_RACEWAR_EVIL(victim)) ||
     (IS_RACEWAR_GOOD(ch) && IS_RACEWAR_GOOD(victim))  ||
     (IS_RACEWAR_UNDEAD(ch) && IS_RACEWAR_UNDEAD(victim))) */

  if (affected_by_spell(victim, SPELL_COMPREHEND_LANGUAGES))
  {
    vict_skill = MAX(vict_skill, 90);
    ch_skill = MAX(ch_skill, 90);
  }

  learned = (vict_skill + ch_skill) / 2;
  len = strlen(string);

  /* gain skill code */
  /* do a checksum and if the checksum is DIFFERENT from the last this
     vict heard, they have a chance to gain...  also require a minimum
     length of the phrase! */

  if (IS_PC(victim) && (len > 20))
  {
    ttl = 0;
    for (i = 0; i < len; i++)
      ttl += string[i];
    if (((ubyte) ttl) != GET_LANGUAGE(victim, TONGUE_LASTHEARD))
    {
      GET_LANGUAGE(victim, TONGUE_LASTHEARD) = ((ubyte) ttl);
      language_gain(ch, victim, GET_LANGUAGE(ch, 0));
    }
  }

  if (learned >= 90)
    return string;              /* no need for translation .. */
  if (learned == 0)
  {
    trans_char(len);
    strcpy(string, translation);
    return string;
  }

  while (*str != '\0')
  {

/* don't change all letters .. */
/* why not change all the letter? */
//    if (learned < 30) {
    if (learned < 0)
    {
      keep_char(1);
      trans_char(BOUNDED(1, 100 / learned - 1, 3));
    }
    else
    {
      keep_char(BOUNDED(2, (learned / (101 - learned) + 1), 5));
      trans_char(1);
    }
    ntstr[0] = '\0';
  }

  *ntstr = '\0';
  strcpy(string, translation);
  return string;
}

struct drunk_struct
{
  int      min_drunk_level;
  int      number_of_rep;
  const char *replacement[11];
};


char    *makedrunk(char *string, P_char ch)
{
  char     buf[MAX_STRING_LENGTH], temp;
  int      pos = 0, randomnum;
  struct drunk_struct drunk[] = {
    {3, 10,
     {"a", "a", "a", "A", "aa", "ah", "Ah", "ao", "aw", "oa", "ahhhh"}},
    {8, 5,
     {"b", "b", "b", "B", "B", "vb"}},
    {3, 5,
     {"c", "c", "C", "cj", "sj", "zj"}},
    {5, 2,
     {"d", "d", "D"}},
    {3, 3,
     {"e", "e", "eh", "E"}},
    {4, 5,
     {"f", "f", "ff", "fff", "fFf", "F"}},
    {8, 2,
     {"g", "g", "G"}},
    {9, 6,
     {"h", "h", "hh", "hhh", "Hhh", "HhH", "H"}},
    {7, 6,
     {"i", "i", "Iii", "ii", "iI", "Ii", "I"}},
    {9, 5,
     {"j", "j", "jj", "Jj", "jJ", "J"}},
    {7, 2,
     {"k", "k", "K"}},
    {3, 2,
     {"l", "l", "L"}},
    {5, 8,
     {"m", "m", "mm", "mmm", "mmmm", "mmmmm", "MmM", "mM", "M"}},
    {6, 6,
     {"n", "n", "nn", "Nn", "nnn", "nNn", "N"}},
    {3, 6,
     {"o", "o", "ooo", "ao", "aOoo", "Ooo", "ooOo"}},
    {3, 2,
     {"p", "p", "P"}},
    {5, 5,
     {"q", "q", "Q", "ku", "ququ", "kukeleku"}},
    {4, 2,
     {"r", "r", "R"}},
    {2, 5,
     {"s", "ss", "zzZzssZ", "ZSssS", "sSzzsss", "sSss"}},
    {5, 2,
     {"t", "t", "T"}},
    {3, 6,
     {"u", "u", "uh", "Uh", "Uhuhhuh", "uhU", "uhhu"}},
    {4, 2,
     {"v", "v", "V"}},
    {4, 2,
     {"w", "w", "W"}},
    {5, 6,
     {"x", "x", "X", "ks", "iks", "kz", "xz"}},
    {3, 2,
     {"y", "y", "Y"}},
    {2, 9,
     {"z", "z", "ZzzZz", "Zzz", "Zsszzsz", "szz", "sZZz", "ZSz", "zZ", "Z"}}
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
          temp = '0' + number(0, 9);
          buf[pos++] = temp;
        }
        else
          buf[pos++] = *string;
      }
    }
    while (*string++);

    buf[pos] = '\0';            /* Mark end of the string... */
    strcpy(string, buf);
    return (string);
  }
  return (string);
}
