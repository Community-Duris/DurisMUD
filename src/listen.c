#include <stdio.h>
#include <string.h>
#include <math.h>

#include <string>
#include <vector>
#include <list>
using namespace std;

#include "structs.h"
#include "spells.h"
#include "comm.h"
#include "interp.h"
#include "prototypes.h"
#include "utils.h"
#include "db.h"
#include "sql.h"
#include "epic.h"
#include "events.h"
#include "listen.h"


extern P_room world;
extern P_desc descriptor_list;

// Main call for the listen skill broadcasting
// cmd option: 0 = say, 1 = gsay, 2 = improved listen(emote)
void listen_broadcast(P_char ch, const char *buf, int cmd)
{
  P_desc i;
  P_char tch;

  if (IS_TRUSTED(ch) || IS_ILLITHID(ch) || IS_PILLITHID(ch) || IS_NPC(ch))
  {
    return;
  }

  for (i = descriptor_list; i; i = i->next)
  {
    if (!i->character || i->connected)
    {
      continue;
    }
    
    tch = i->character;
    
    // Lets weed out anyone who dosn't have listen skill, otherwise we lag!
    if (!GET_CHAR_SKILL(tch, SKILL_LISTEN))
    {
      continue;
    }

    if (tch && IS_PC(tch))
    {
      switch (cmd)
      {
        case LISTEN_SAY: 
          listen_say(ch, tch, buf); 
          break;
      
        case LISTEN_GSAY:
          listen_gsay(ch, tch, buf);
          break;
        
        case LISTEN_EMOTE:
          listen_emote(ch, tch, buf);
          break;
          
        default: 
          wizlog(57, "function listen_broadcast was called with an invalid subcommand.");
          break;
      }
    }
  }
}

void listen_say(P_char ch, P_char tch, const char *buf)
{
  int iskl, range, nrange, howclose;
  char strn[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
  
  if (IS_SET(world[tch->in_room].room_flags, ROOM_SILENT))
    return;

  iskl = GET_CHAR_SKILL(tch, SKILL_IMPROVED_LISTEN);
  nrange = (int) get_property("skill.listen.range.norm", 1);

  if (iskl)
  {
    range = (nrange + (int)get_property("skill.listen.range.norm.extend", 1));
  }
  else
  {
    range = nrange;
  }

  howclose = how_close(ch->in_room, tch->in_room, range);

  if (howclose < 0)
  {
    return;
  }

  if ((howclose > 1) && (BOUNDED(20, iskl, 95) < number(0, 100)))
  {
    return;
  }

  if ((GET_CHAR_SKILL(tch, SKILL_LISTEN) && (ch != tch) &&
      (world[ch->in_room].number != world[tch->in_room].number)))
  {
    if (MAX(40, GET_CHAR_SKILL(tch, SKILL_LISTEN)) >
        (number(25, 45)))
    {
      muddle_listened_string(ch, tch, buf, strn, howclose);

      sprintf(buf1, "You faintly hear %s say the words '%s'.\r\n",
              (GET_CHAR_SKILL(tch, SKILL_LISTEN) > number(15, 35)) ?
              PERS(ch, tch, 1) : "someone:", strn);

      send_to_char(buf1, tch);

      notch_skill(tch, SKILL_LISTEN, 17);
    }
    else
    {
      notch_skill(tch, SKILL_LISTEN, 1);
    }
  }
}

void listen_gsay(P_char ch, P_char tch, const char *buf)
{
  int iskl, range, nrange, howclose;
  char strn[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
  
  if (IS_SET(world[tch->in_room].room_flags, ROOM_SILENT))
    return;
  
  iskl = GET_CHAR_SKILL(tch, SKILL_IMPROVED_LISTEN);
  nrange = (int) get_property("skill.listen.range.norm", 1);

  if (iskl)
  {
    range = (nrange + (int)get_property("skill.listen.range.norm.extend", 1));
  }
  else
  {
    range = nrange;
  }

  howclose = how_close(ch->in_room, tch->in_room, range);

  if (howclose < 0)
  {
    return;
  }

  if ((howclose > 1) && (BOUNDED(20, iskl, 95) < number(0, 100)))
  {
    return;
  }

  if ((GET_CHAR_SKILL(tch, SKILL_LISTEN) && ch->group != tch->group))
  {
    if (MAX(40, GET_CHAR_SKILL(tch, SKILL_LISTEN)) >
        (number(25, 45)))
    {
      muddle_listened_string(ch, tch, buf, strn, howclose);

      sprintf(buf1, "You faintly hear %s groupsay the words '%s'.\r\n",
              (GET_CHAR_SKILL(tch, SKILL_LISTEN) > number(20, 40)) ?
              PERS(ch, tch, 1) : "someone:", strn);

      send_to_char(buf1, tch);

      notch_skill(tch, SKILL_LISTEN, 5);
    }
    else
    {
      notch_skill(tch, SKILL_LISTEN, 1);
    }
  }
}

void listen_emote(P_char ch, P_char tch, const char *buf)
{
  int iskl, irange, howclose;
  char buf1[MAX_STRING_LENGTH];

  if (IS_TRUSTED(tch))
    return;

  strcpy(buf1, "You notice in the distance, ");
  strcat(buf1, buf);
  
  if (ch == tch || world[ch->in_room].number == world[tch->in_room].number)
  {
    return;
  }

  iskl = GET_CHAR_SKILL(tch, SKILL_IMPROVED_LISTEN);
  irange = (int)get_property("skill.listen.range.improved", 2);

  howclose = how_close(ch->in_room, tch->in_room, irange);

  if (howclose < 0)
  {
    return;
  }

  if ((howclose > 1) && (BOUNDED(20, iskl, 95)) < number(0, 100))
  {
    return;
  }

  if (iskl)
  {
    if ((MAX(40, iskl) > (number(15, 55))) && IS_PC(tch))
    {
      if ((GET_SIZE(ch) < GET_SIZE(tch)) && (number(70, 90) > MAX(75, iskl)))
      {
        send_to_char("You notice someone in the distance scurrying about.\r\n", tch);
      }
      else
      {
        act(buf1, TRUE, ch, 0, tch, TO_VICT);
      }
    }
  }
}

/*
 * muddle_listened_string - CRYPTs and drops some words
 *
 *  ch - char talking
 *  vict - char listening
 *  arg - stuff said
 *  extr_depth - if one room away, 0, if further, >0
 */
 // Taken from actcomm.c
void muddle_listened_string(P_char ch, P_char vict, const char *arg,
                            char *muddled, int extr_depth)
{
  const char missed_word[] = "(mumble)";
  char     buf[MAX_STRING_LENGTH];
  int      i, j, k, len;


  i = j = 0;

  // okay drop some words

  while (TRUE)
  {
    // if at space, go to next non-space (word)

    if (arg[i] == ' ')
    {
      while (arg[i] == ' ')
      {
        muddled[j++] = ' ';
        i++;
      }

      if (!arg[i])
      {
        muddled[j] = '\0';
        break;
      }
    }
    if (MAX(40, GET_CHAR_SKILL(vict, SKILL_LISTEN)) <
        (number(25, 75) + (extr_depth * 10)))
    {
      // insert an appropriate number of asterisks

      for (k = 0; k < strlen(missed_word); k++)
        muddled[j++] = '*';

      // advance to next space

      while ((arg[i] != ' ') && (arg[i] != '\0'))
        i++;

      if (arg[i] == '\0')
      {
        muddled[j] = '\0';
        break;
      }
    }
    else                        // they passed, give em the whole word
    {
      while ((arg[i] != ' ') && (arg[i] != '\0'))
        muddled[j++] = arg[i++];

      if (!arg[i])
      {
        muddled[j] = '\0';
        break;
      }
    }
  }

  // CRYPT it up

  strcpy(buf, muddled);
  strcpy(muddled, language_CRYPT(ch, vict, buf));

  // change ******** stuff to missed_word

  len = strlen(muddled);
  k = 0;

  for (i = 0; i < len; i++)
  {
    if (muddled[i] == '*')
    {
      // make sure we have the right number of em..

      for (j = i; j < i + strlen(missed_word); j++)
      {
        if (muddled[j] != '*')
        {
          while (muddled[i] == '*')
            i++;
          k = 1;
        }
      }

      if (!k)
      {
        for (j = i; j < i + strlen(missed_word); j++)
        {
          muddled[j] = missed_word[k++];
        }
      }

      k = 0;
    }
  }
}

