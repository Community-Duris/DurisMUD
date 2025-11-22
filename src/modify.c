/*
   ***************************************************************************
   *  File: modify.c                                           Part of Duris *
   *  Usage: Run-time modification (by users) of game variables                *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "comm.h"
#include "db.h"
#include "interp.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "mm.h"
#include "events.h"
#include "spells.h"
#include "sql.h"
#include "ships.h"

/*
   external variables
 */

extern P_desc descriptor_list;
extern P_char character_list;
extern P_room world;
extern int slow_death;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;

/* action modes for parse_action */
#define PARSE_FORMAT            0
#define PARSE_REPLACE           1
#define PARSE_HELP              2
#define PARSE_DELETE            3
#define PARSE_INSERT            4
#define PARSE_LIST_NORM         5
#define PARSE_LIST_NUM          6
#define PARSE_EDIT              7

#define FORMAT_INDENT          (1 << 0)

#define TP_MOB      0
#define TP_OBJ      1
#define TP_ERROR    2

int      help_array[27][2];
int      info_array[27][2];

const char *string_fields[] = {
  "name",                  // 1
  "short",                 // 2
  "long",                  // 3
  "description",           // 4
  "title",                 // 5
  "password",              // 6
  "delete-description",    // 7
  "\n"
};

/* maximum length for text field x+1 */
const int length[] = {
  80,
  80,
  256,
  240,
  80
};

bool rename_character(P_char ch, char *old_name, char *new_name);

  /************************************************************************
   *  modification of malloc'ed strings                                   *
   ************************************************************************/

/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
char    *replace(char *g_string, char *replace_from, char *replace_to)
{
  char    *p, *p1, *return_str;
  int      i_diff;

  i_diff = strlen(replace_from) - strlen(replace_to);   //the margin between the replace_from and replace_to;
  CREATE(return_str, char, strlen(g_string) + 1, MEM_TAG_STRING);
  //return_str = (char *) malloc(strlen(g_string) + 1);   //Changed line

  if (return_str == NULL)
    return g_string;
  return_str[0] = 0;

  p = g_string;

  for (;;)
  {
    p1 = p;                     // old position
    p = strstr(p, replace_from);        // next position
    if (p == NULL)
    {
      strcat(return_str, p1);
      break;
    }
    while (p > p1)
    {
      snprintf(return_str, MAX_STRING_LENGTH, "%s%c", return_str, *p1);
      p1++;
    }
    if (i_diff > 0)
    {
      RECREATE(return_str, char, strlen(g_string) + i_diff + 1);
      if (return_str == NULL)
        return g_string;
    }
    strcat(return_str, replace_to);
    p += strlen(replace_from);  // new point position
  }
  return return_str;
}




int replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size)
{
  char    *replace_buffer = NULL;
  char    *flow = NULL, *jetsam = NULL, temp;
  unsigned int len;
  int      i;

  if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
    return -1;

  CREATE(replace_buffer, char, (unsigned)(max_size), MEM_TAG_STRING);
  i = 0;
  jetsam = *string;
  flow = *string;
  *replace_buffer = '\0';
  if (rep_all)
  {
    while ((flow = (char *) strstr(flow, pattern)) != NULL)
    {
      i++;
      temp = *flow;
      *flow = '\0';
      if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) >
          max_size)
      {
        i = -1;
        break;
      }
      strcat(replace_buffer, jetsam);
      strcat(replace_buffer, replacement);
      *flow = temp;
      flow += strlen(pattern);
      jetsam = flow;
    }
    strcat(replace_buffer, jetsam);
  }
  else
  {
    if ((flow = (char *) strstr(*string, pattern)) != NULL)
    {
      i++;
      flow += strlen(pattern);
      len = ((char *) flow - (char *) *string) - strlen(pattern);

      strncpy(replace_buffer, *string, len);
      strcat(replace_buffer, replacement);
      strcat(replace_buffer, flow);
    }
  }
  if (i == 0)
    return 0;
  if (i > 0)
  {
    RECREATE(*string, char, strlen(replace_buffer) + 3);

    strcpy(*string, replace_buffer);
  }
  FREE(replace_buffer);
  return i;
}

/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */

void format_text(char **ptr_string, int mode, struct descriptor_data *d,
                 int maxlen)
{
  int      total_chars, cap_next = TRUE, cap_next_next = FALSE;
  char    *flow = NULL, *start = NULL, temp;

  /* warning: do not edit messages with max_str's of over this value */
  char     formated[MAX_STRING_LENGTH];

  flow = *ptr_string;
  if (!flow)
    return;

  if (IS_SET(mode, FORMAT_INDENT))
  {
    strcpy(formated, "   ");
    total_chars = 3;
  }
  else
  {
    *formated = '\0';
    total_chars = 0;
  }

  while (*flow != '\0')
  {
    while ((*flow == '\n') ||
           (*flow == '\r') ||
           (*flow == '\f') ||
           (*flow == '\t') || (*flow == '\v') || (*flow == ' '))
      flow++;
    if (*flow != '\0')
    {
      start = flow++;
      while ((*flow != '\0') &&
             (*flow != '\n') &&
             (*flow != '\r') &&
             (*flow != '\f') &&
             (*flow != '\t') && (*flow != '\v') && (*flow != ' ') &&
/*           (*flow != '.') && */
             (*flow != '?') && (*flow != '!'))
        flow++;

      if (cap_next_next)
      {
        cap_next_next = FALSE;
        cap_next = TRUE;
      }
/* this is so that if we stopped on a sentance, we move off the sentance delim. */
      while ( /* (*flow == '.') || */ (*flow == '!') || (*flow == '?'))
      {
        cap_next_next = TRUE;
        flow++;
      }

      temp = *flow;
      *flow = '\0';

      if ((total_chars + strlen(start) + 1) > 69)
      {
        strcat(formated, "\r\n");
        total_chars = 0;
      }
      if (!cap_next)
      {
        if (total_chars > 0)
        {
          strcat(formated, " ");
          total_chars++;
        }
      }
      else
      {
        cap_next = FALSE;
        *start = UPPER(*start);
      }

      total_chars += strlen(start);
      strcat(formated, start);

      *flow = temp;
    }
    if (cap_next_next)
    {
      if ((total_chars + 3) > 79)
      {
        strcat(formated, "\r\n");
        total_chars = 0;
      }
      else
      {
        strcat(formated, "  ");
        total_chars += 2;
      }
    }
  }
  strcat(formated, "\r\n");

  if (strlen(formated) > maxlen)
    formated[maxlen] = '\0';
  RECREATE(*ptr_string, char, MIN(maxlen, (signed) strlen(formated) + 3));

  strcpy(*ptr_string, formated);
}

/*  handle some editor commands */

void parse_action(int command, char *string, struct descriptor_data *d)
{
  int      indent = 0, rep_all = 0, flags = 0, total_len, replaced;
  register int j = 0;
  int      i, line_low, line_high;
  char    *s = NULL, *t = NULL, temp, buf[MAX_STRING_LENGTH],
    buf2[MAX_STRING_LENGTH];

  buf[0] = '\0';
  buf2[0] = '\0';

  switch (command)
  {
  case PARSE_HELP:
    snprintf(buf, MAX_STRING_LENGTH,
            "Editor command formats: /<letter>\r\n\r\n"
            "/a         -  aborts editor\r\n"
            "/c         -  clears buffer\r\n"
            "/d#        -  deletes a line #\r\n"
            "/e# <text> -  changes the line at # with <text>\r\n"
            "/f         -  formats text\r\n"
            "/fi        -  indented formatting of text\r\n"
            "/h         -  list text editor commands\r\n"
            "/i# <text> -  inserts <text> before line #\r\n"
            "/l         -  lists buffer\r\n"
            "/n         -  lists buffer with line numbers\r\n"
            "/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\r\n"
            "/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\r\n"
            "              usage: /r[a] 'pattern' 'replacement'\r\n"
            "/s         -  saves text\r\n");
    SEND_TO_Q(buf, d);
    break;
  case PARSE_FORMAT:
    while (isalpha(string[j]) && j < 2)
    {
      switch (string[j])
      {
      case 'i':
        if (!indent)
        {
          indent = 1;
          flags += FORMAT_INDENT;
        }
        break;
      default:
        break;
      }
      j++;
    }
    format_text(d->str, flags, d, d->max_str);
    snprintf(buf, MAX_STRING_LENGTH, "Text formarted with%s indent.\r\n", (indent ? "" : "out"));
    SEND_TO_Q(buf, d);
    break;
  case PARSE_REPLACE:
    while (isalpha(string[j]) && j < 2)
    {
      switch (string[j])
      {
      case 'a':
        if (!indent)
        {
          rep_all = 1;
        }
        break;
      default:
        break;
      }
      j++;
    }
    s = strtok(string, "'");
    if (s == NULL)
    {
      SEND_TO_Q("Invalid format.\r\n", d);
      return;
    }
    s = strtok(NULL, "'");
    if (s == NULL)
    {
      SEND_TO_Q("Target string must be enclosed in single quotes.\r\n", d);
      return;
    }
    t = strtok(NULL, "'");
    if (t == NULL)
    {
      SEND_TO_Q("No replacement string.\r\n", d);
      return;
    }
    t = strtok(NULL, "'");
    if (t == NULL)
    {
      SEND_TO_Q("Replacement string must be enclosed in single quotes.\r\n",
                d);
      return;
    }
    total_len = ((strlen(t) - strlen(s)) + strlen(*d->str));
    if (total_len <= d->max_str)
    {
      if ((replaced = replace_str(d->str, s, t, rep_all, d->max_str)) > 0)
      {
        snprintf(buf, MAX_STRING_LENGTH, "Replaced %d occurance%sof '%s' with '%s'.\r\n",
                replaced, ((replaced != 1) ? "s " : " "), s, t);
        SEND_TO_Q(buf, d);
      }
      else if (replaced == 0)
      {
        snprintf(buf, MAX_STRING_LENGTH, "String '%s' not found.\r\n", s);
        SEND_TO_Q(buf, d);
      }
      else
      {
        SEND_TO_Q
          ("ERROR: Replacement string causes buffer overflow, aborted replace.\r\n",
           d);
      }
    }
    else
      SEND_TO_Q("Not enough space left in buffer.\r\n", d);
    break;
  case PARSE_DELETE:
    switch (sscanf(string, " %d - %d ", &line_low, &line_high))
    {
    case 0:
      SEND_TO_Q("You must specify a line number or range to delete.\r\n", d);
      return;
    case 1:
      line_high = line_low;
      break;
    case 2:
      if (line_high < line_low)
      {
        SEND_TO_Q("That range is invalid.\r\n", d);
        return;
      }
      break;
    }

    i = 1;
    total_len = 1;
    if ((s = *d->str) == NULL)
    {
      SEND_TO_Q("Buffer is empty.\r\n", d);
      return;
    }
    if (line_low > 0)
    {
      while (s && (i < line_low))
        if ((s = strchr(s, '\n')) != NULL)
        {
          i++;
          s++;
        }
      if ((i < line_low) || (s == NULL))
      {
        SEND_TO_Q("Line(s) out of range; not deleting.\r\n", d);
        return;
      }
      t = s;
      while (s && (i < line_high))
        if ((s = strchr(s, '\n')) != NULL)
        {
          i++;
          total_len++;
          s++;
        }
      if ((s) && ((s = strchr(s, '\n')) != NULL))
      {
        s++;
        while (*s != '\0')
          *(t++) = *(s++);
      }
      else
        total_len--;
      *t = '\0';
      RECREATE(*d->str, char, strlen(*d->str) + 3);

      snprintf(buf, MAX_STRING_LENGTH, "%d line%sdeleted.\r\n", total_len,
              ((total_len != 1) ? "s " : " "));
      SEND_TO_Q(buf, d);
    }
    else
    {
      SEND_TO_Q("Invalid line numbers to delete must be higher than 0.\r\n",
                d);
      return;
    }
    break;
  case PARSE_LIST_NORM:
    /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they
     * are prolly ok fer what i want to do here. */
    *buf = '\0';
    if (*string != '\0')
      switch (sscanf(string, " %d - %d ", &line_low, &line_high))
      {
      case 0:
        line_low = 1;
        line_high = 999999;
        break;
      case 1:
        line_high = line_low;
        break;
      }
    else
    {
      line_low = 1;
      line_high = 999999;
    }

    if (line_low < 1)
    {
      SEND_TO_Q("Line numbers must be greater than 0.\r\n", d);
      return;
    }
    if (line_high < line_low)
    {
      SEND_TO_Q("That range is invalid.\r\n", d);
      return;
    }
    *buf = '\0';
    if ((line_high < 999999) || (line_low > 1))
    {
      snprintf(buf, MAX_STRING_LENGTH, "Current buffer range [%d - %d]:\r\n", line_low,
              line_high);
    }
    i = 1;
    total_len = 0;
    s = *d->str;
    while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL)
      {
        i++;
        s++;
      }
    if ((i < line_low) || (s == NULL))
    {
      SEND_TO_Q("Line(s) out of range; no buffer listing.\r\n", d);
      return;
    }
    t = s;
    while (s && (i <= line_high))
      if ((s = strchr(s, '\n')) != NULL)
      {
        i++;
        total_len++;
        s++;
      }
    if (s)
    {
      temp = *s;
      *s = '\0';
      strcat(buf, t);
      *s = temp;
    }
    else
      strcat(buf, t);
    page_string(d, buf, TRUE);
    break;
  case PARSE_LIST_NUM:
    /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they
     * are prolly ok fer what i want to do here. */
    *buf = '\0';
    if (*string != '\0')
      switch (sscanf(string, " %d - %d ", &line_low, &line_high))
      {
      case 0:
        line_low = 1;
        line_high = 999999;
        break;
      case 1:
        line_high = line_low;
        break;
      }
    else
    {
      line_low = 1;
      line_high = 999999;
    }

    if (line_low < 1)
    {
      SEND_TO_Q("Line numbers must be greater than 0.\r\n", d);
      return;
    }
    if (line_high < line_low)
    {
      SEND_TO_Q("That range is invalid.\r\n", d);
      return;
    }
    *buf = '\0';
    i = 1;
    total_len = 0;
    s = *d->str;
    while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL)
      {
        i++;
        s++;
      }
    if ((i < line_low) || (s == NULL))
    {
      SEND_TO_Q("Line(s) out of range; no buffer listing.\r\n", d);
      return;
    }
    t = s;
    while (s && (i <= line_high))
      if ((s = strchr(s, '\n')) != NULL)
      {
        i++;
        total_len++;
        s++;
        temp = *s;
        *s = '\0';
        snprintf(buf, MAX_STRING_LENGTH, "&+c%s&n&+B%d:&n ", buf, (i - 1));
        strcat(buf, t);
        *s = temp;
        t = s;
      }
    if (s && t)
    {
      temp = *s;
      *s = '\0';
      strcat(buf, t);
      *s = temp;
    }
    else if (t)
      strcat(buf, t);
    page_string(d, buf, TRUE);
    break;

  case PARSE_INSERT:
    half_chop(string, buf, buf2);
    if (*buf == '\0')
    {
      SEND_TO_Q
        ("You must specify a line number before which to insert text.\r\n",
         d);
      return;
    }
    line_low = atoi(buf);
    strcat(buf2, "\r\n");

    i = 1;
    *buf = '\0';
    if ((s = *d->str) == NULL)
    {
      SEND_TO_Q("Buffer is empty, nowhere to insert.\r\n", d);
      return;
    }
    if (line_low > 0)
    {
      while (s && (i < line_low))
        if ((s = strchr(s, '\n')) != NULL)
        {
          i++;
          s++;
        }
      if ((i < line_low) || (s == NULL))
      {
        SEND_TO_Q("Line number out of range; insert aborted.\r\n", d);
        return;
      }
      temp = *s;
      *s = '\0';
      if ((strlen(*d->str) + strlen(buf2) + strlen(s + 1) + 3) > d->max_str)
      {
        *s = temp;
        SEND_TO_Q
          ("Insert text pushes buffer over maximum size, insert aborted.\r\n",
           d);
        return;
      }
      if (*d->str && (**d->str != '\0'))
        strcat(buf, *d->str);
      *s = temp;
      strcat(buf, buf2);
      if (s && (*s != '\0'))
        strcat(buf, s);
      RECREATE(*d->str, char, strlen(buf) + 3);

      strcpy(*d->str, buf);
      SEND_TO_Q("Line inserted.\r\n", d);
    }
    else
    {
      SEND_TO_Q("Line number must be higher than 0.\r\n", d);
      return;
    }
    break;

  case PARSE_EDIT:
    half_chop(string, buf, buf2);
    if (*buf == '\0')
    {
      SEND_TO_Q("You must specify a line number at which to change text.\r\n",
                d);
      return;
    }
    line_low = atoi(buf);
    strcat(buf2, "\r\n");

    i = 1;
    *buf = '\0';
    if ((s = *d->str) == NULL)
    {
      SEND_TO_Q("Buffer is empty, nothing to change.\r\n", d);
      return;
    }
    if (line_low > 0)
    {
      /* loop through the text counting /n chars till we get to the line */
      while (s && (i < line_low))
        if ((s = strchr(s, '\n')) != NULL)
        {
          i++;
          s++;
        }
      /* make sure that there was a THAT line in the text */
      if ((i < line_low) || (s == NULL))
      {
        SEND_TO_Q("Line number out of range; change aborted.\r\n", d);
        return;
      }
      /* if s is the same as *d->str that means im at the beginning of the
       * message text and i dont need to put that into the changed buffer */
      if (s != *d->str)
      {
        /* first things first .. we get this part into buf. */
        temp = *s;
        *s = '\0';
        /* put the first 'good' half of the text into storage */
        strcat(buf, *d->str);
        *s = temp;
      }
      /* put the new 'good' line into place. */
      strcat(buf, buf2);
      if ((s = strchr(s, '\n')) != NULL)
      {
        /* this means that we are at the END of the line we want outta there. */
        /* BUT we want s to point to the beginning of the line AFTER
         * the line we want edited */
        s++;
        /* now put the last 'good' half of buffer into storage */
        strcat(buf, s);
      }
      /* check for buffer overflow */
      if (strlen(buf) > d->max_str)
      {
        SEND_TO_Q
          ("Change causes new length to exceed buffer maximum size, aborted.\r\n",
           d);
        return;
      }
      /* change the size of the REAL buffer to fit the new text */
      RECREATE(*d->str, char, strlen(buf) + 3);
      strcpy(*d->str, buf);
      SEND_TO_Q("Line changed.\r\n", d);
    }
    else
    {
      SEND_TO_Q("Line number must be higher than 0.\r\n", d);
      return;
    }
    break;
  default:
    SEND_TO_Q("Invalid option.\r\n", d);
    return;
  }
}

/* strips \r's from line */
char    *stripcr(char *dest, const char *src)
{
  int      i, len;
  char    *temp = NULL;

  if (!dest || !src)
    return NULL;
  temp = &dest[0];
  len = strlen(src);
  for (i = 0; *src && (i < len); i++, src++)
    if (*src != '\r')
      *(temp++) = *src;
  *temp = '\0';
  return dest;
}

/* Add user input to the 'current' string (as defined by d->str) */
void string_add(struct descriptor_data *d, char *str)
{
  int      terminator = 0, num = 0;
  register int i = 2, j = 0;
  char     actions[MAX_INPUT_LENGTH], *ch_ptr, buf1[MAX_STRING_LENGTH];
  FILE    *fl;

  actions[0] = '\0';
  buf1[0] = '\0';

  /*
     Check for ansi characters, mortals not allowed to use them to put color in
     says, shouts, gossips, titles, etc. SAM 6-94
   */
  if (GET_LEVEL(d->character) < MINLVLIMMORTAL)
  {
    for (ch_ptr = str; *ch_ptr; ch_ptr++)
    {
      if (*ch_ptr == '&')
        switch (*(ch_ptr + 1))
        {
        case '+':
        case '-':
        case '=':
        case 'n':
        case 'N':
        case 'L':
          SEND_TO_Q("No ansi chars allowed as input.  Discarding line.\r\n",
                    d);
          return;
          break;
        }
    }
  }
  /* determine if this is the terminal string, and truncate if so */
  /* changed to accept '/<letter>' style editing commands - instead */
  /* of solitary '@' to end - (modification of improved_edit patch) */
  /*   M. Scott 10/15/96 */

  if ((num = (*str == '/')) && (strlen(str) < MAX_INPUT_LENGTH))
  {
    while (str[i] != '\0')
    {
      actions[j] = str[i];
      i++;
      j++;
    }
    actions[j] = '\0';
    *str = '\0';
    switch (str[1])
    {
    case 'a':
      terminator = 2;           /* working on an abort message */
      break;
    case 'c':
      if (*d->str)
      {
        FREE(*d->str);
        (*d->str) = NULL;
        SEND_TO_Q("Current buffer cleared.\r\n", d);
      }
      else
        SEND_TO_Q("Current buffer empty.\r\n", d);
      break;
    case 'd':
      parse_action(PARSE_DELETE, actions, d);
      break;
    case 'e':
      parse_action(PARSE_EDIT, actions, d);
      break;
    case 'f':
      if (*(d->str))
        parse_action(PARSE_FORMAT, actions, d);
      else
        SEND_TO_Q("Current buffer empty.\r\n", d);
      break;
    case 'i':
      if (*(d->str))
        parse_action(PARSE_INSERT, actions, d);
      else
        SEND_TO_Q("Current buffer empty.\r\n", d);
      break;
    case 'h':
      parse_action(PARSE_HELP, actions, d);
      break;
    case 'l':
      if (*d->str)
        parse_action(PARSE_LIST_NORM, actions, d);
      else
        SEND_TO_Q("Current buffer empty.\r\n", d);
      break;
    case 'n':
      if (*d->str)
        parse_action(PARSE_LIST_NUM, actions, d);
      else
        SEND_TO_Q("Current buffer empty.\r\n", d);
      break;
/*    case 'r':  // i'm afraid this is broken somehow..  we'll fix it someday, honest
      parse_action(PARSE_REPLACE, actions, d);
      break;*/
    case 's':
      if (*(d->str))
      {
        terminator = 1;
        *str = '\0';
      }
      else
        terminator = 2;
      break;
    default:
      SEND_TO_Q("Invalid option.\r\n", d);
      break;
    }
  }
  if (!(*d->str))
  {
    if (strlen(str) > d->max_str)
    {
      send_to_char("String too long - Truncated.\r\n", d->character);
      *(str + d->max_str) = '\0';
    }
    CREATE(*d->str, char, strlen(str) + 3, MEM_TAG_STRING);

    strcpy(*d->str, str);
  }
  else
  {
    if (strlen(str) + strlen(*d->str) > d->max_str)
    {
      send_to_char
        ("String too long, limit reached on message.  Last line ignored.\r\n",
         d->character);
      terminator = 1;
    }
    else
    {
      RECREATE(*d->str, char, strlen(*d->str) + strlen(str) + 3);
      if (!(*d->str))
      {
        perror("string_add");
        logit(LOG_EXIT, "string_add");
        exit(1);
      }
      strcat(*d->str, str);
    }
  }

  if (terminator)
  {
    /* here we check for the abort option and reset the pointers */
    if ((terminator == 2) && (STATE(d) == CON_GET_EXTRA_DESC))
    {
      FREE(*d->str);
      if (d->backstr)
      {
        *d->str = d->backstr;
      }
      else
        *d->str = NULL;
      d->backstr = NULL;
      d->str = NULL;

      SEND_TO_Q("Description aborted.\r\n", d);
      SEND_TO_Q(MENU, d);
      d->connected = CON_MAIN_MENU;
    }
    else if ((d->str) && (*d->str) && (**d->str == '\0'))
    {
      FREE(*d->str);
      *d->str = NULL;

      if (!d->connected)
      {
        send_to_char("Message aborted.\r\n", d->character);
      }
      else
      {
        SEND_TO_Q("Description cleared.\r\n", d);
        SEND_TO_Q(MENU, d);
        d->connected = CON_MAIN_MENU;
      }
    }
    else if (!d->connected && (IS_SET(d->character->specials.act, PLR_MAIL)))
    {
      if ((terminator == 1) && *d->str)
      {
        store_mail(d->name, d->character->player.name, *d->str);
        SEND_TO_Q("Message sent!\r\n", d);
      }
      else
        SEND_TO_Q("Mail aborted.\r\n", d);
      d->name = 0;
      FREE(*d->str);
      *d->str = NULL;
      d->str = NULL;
    }
    else if (STATE(d) == CON_GET_EXTRA_DESC)
    {
      if (terminator != 1)
        SEND_TO_Q("Description aborted.\r\n", d);
      SEND_TO_Q(MENU, d);
      d->connected = CON_MAIN_MENU;
    }
    else if (!d->connected && d->character && !IS_NPC(d->character))
    {
      if (terminator == 1)
      {
        if (strlen(*d->str) == 0)
        {
          FREE(*d->str);
          *d->str = NULL;
        }
        d->str = 0;
      }
      else
      {
        FREE(*d->str);
        *d->str = NULL;
        if (d->backstr)
        {
          *d->str = d->backstr;
        }
        else
          *d->str = NULL;
        d->backstr = NULL;
        SEND_TO_Q("Message aborted.\r\n", d);
      }
    }
    if (d->character && !IS_NPC(d->character))
      REMOVE_BIT(d->character->specials.act, PLR_MAIL | PLR_WRITE);
    if (d->backstr)
      FREE(d->backstr);
    d->backstr = NULL;
    d->str = NULL;
  }
  else if (!num)
    strcat(*d->str, "\r\n");
}

/* interpret an argument for do_string */
void quad_arg(char *arg, int *type, char *name, int *field, char *string)
{
  char     buf[MAX_STRING_LENGTH];

  buf[0] = '\0';

  /* determine type */
  arg = one_argument(arg, buf);
  if (is_abbrev(buf, "char"))
    *type = TP_MOB;
  else if (is_abbrev(buf, "obj"))
    *type = TP_OBJ;
  else
  {
    *type = TP_ERROR;
    return;
  }

  /*
     find name
   */
  arg = one_argument(arg, name);

  /*
     field name and number
   */
  arg = one_argument(arg, buf);
  if (!(*field = old_search_block(buf, 0, strlen(buf), string_fields, 0)))
    return;

  /*
     string
   */
  for (; isspace(*arg); arg++) ;
  for (; (*string = *arg); arg++, string++) ;

  return;
}

/* modification of malloc'ed strings in chars/objects */
void do_string(P_char ch, char *arg, int cmd)
{
  char     name[MAX_STRING_LENGTH], string[MAX_STRING_LENGTH];
  int      field, type;
  P_char   mob;
  P_obj    obj;
  struct extra_descr_data *ed, *tmp;

  name[0] = '\0';
  string[0] = '\0';

  if (IS_NPC(ch))
    return;

  quad_arg(arg, &type, name, &field, string);

  if (cmd && *arg && IS_TRUSTED(ch) && 
      field != 6 ) /* don't log when changing passwords */
  {
    wizlog(GET_LEVEL(ch), "%s: string %s", GET_NAME(ch), arg);
    logit(LOG_WIZ, "%s: string %s", GET_NAME(ch), arg);
    sql_log(ch, WIZLOG, "string %s", arg);
  }

  if (type == TP_ERROR)
  {
    send_to_char("Syntax:\r\nstring ('obj'|'char')<name><field>[<string>].",
                 ch);
    return;
  }
  if (!field)
  {
    send_to_char("No field by that name. Try 'help string'.\r\n", ch);
    return;
  }
  if (type == TP_MOB)
  {
    /*
       locate the beast
     */
    if (!(mob = get_char_vis(ch, name)))
    {
      send_to_char("I don't know anyone by that name...\r\n", ch);
      return;
    }
    switch (field)
    {
    case 1:
      if (IS_PC(mob) && (GET_LEVEL(ch) < OVERLORD))
      {
        send_to_char("You can't change that field for players.", ch);
        return;
      }
      else if (!ch->desc)
        return;
      if (IS_NPC(mob))
      {
        if ((mob->only.npc->str_mask & STRUNG_KEYS) && mob->player.name)
          FREE(mob->player.name);
        mob->player.name = NULL;
        mob->only.npc->str_mask |= STRUNG_KEYS;
      }
      ch->desc->str = &mob->player.name;

      if (IS_PC(mob))
        send_to_char("WARNING: You have changed the name of a player.\r\n",
                     ch);
      break;

    case 2:
      if (IS_PC(mob) && (GET_LEVEL(ch) < OVERLORD))
      {
        send_to_char("You can't change that field for players.", ch);
        return;
      }
      else if (!ch->desc)
        return;
      if ((mob->only.npc->str_mask & STRUNG_DESC2) && mob->player.short_descr)
        FREE(mob->player.short_descr);
      mob->player.short_descr = NULL;
      if (IS_NPC(mob))
        mob->only.npc->str_mask |= STRUNG_DESC2;
      ch->desc->str = &mob->player.short_descr;
      if (IS_PC(mob))
        send_to_char
          ("WARNING: You have changed the short description of a player.\r\n",
           ch);
      break;

    case 3:
      if (IS_PC(mob))
      {
        send_to_char("That field is for monsters only.\r\n", ch);
        return;
      }
      else if (!ch->desc)
        return;
      if ((mob->only.npc->str_mask & STRUNG_DESC1) && mob->player.long_descr)
        FREE(mob->player.long_descr);
      mob->player.long_descr = NULL;
      mob->only.npc->str_mask |= STRUNG_DESC1;
      ch->desc->str = &mob->player.long_descr;
      break;

    case 4:
      if (!ch->desc)
        return;
      if (IS_NPC(mob))
      {
        if ((mob->only.npc->str_mask & STRUNG_DESC3) &&
            mob->player.description)
          mob->player.description = NULL;
        mob->only.npc->str_mask |= STRUNG_DESC3;
      }
      ch->desc->str = &mob->player.description;
      break;

    case 5:
      if (IS_NPC(mob))
      {
        send_to_char("Monsters have no titles.\r\n", ch);
        return;
      }
      else if (!ch->desc)
        return;
      ch->desc->str = &mob->player.title;
      break;
    case 6:
      if (GET_LEVEL(ch) < FORGER)
      {
        send_to_char("Ye haven't the power for such a task.\r\n", ch);
        return;
      }
      if (IS_NPC(mob))
      {
        send_to_char("Monsters have no passwords.\r\n", ch);
        return;
      }
      else if (!ch->desc)
        return;
      strcpy( mob->only.pc->pwd, CRYPT2(string, mob->player.name) );
      ch->desc->str = NULL;
      send_to_char("Password set.\r\n", ch);
      return;
      break;
    default:
      send_to_char("That field is undefined for monsters.\r\n", ch);
      return;
      break;
    }
  }
  else
  {                             /* type == TP_OBJ */
    /* locate the object */

    if ((obj = get_obj_in_list_vis(ch, name, ch->carrying)) == NULL)
    {
      if ((obj =
           get_obj_in_list_vis(ch, name,
                               world[ch->in_room].contents)) == NULL)
      {
        send_to_char("No object by that name here.\r\n", ch);
        return;
      }
    }
    switch (field)
    {
    case 1:
      if ((obj->str_mask & STRUNG_KEYS) && obj->name)
        FREE(obj->name);
      obj->name = NULL;
      obj->str_mask |= STRUNG_KEYS;
      ch->desc->str = &obj->name;
      /* quest item hack */
      if (isname("quest", obj->name))
        set_obj_affected(obj, 302400, TAG_OBJ_DECAY, 0);
      break;

    case 2:
      if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
        FREE(obj->short_description);
      obj->short_description = NULL;
      obj->str_mask |= STRUNG_DESC2;
      ch->desc->str = &obj->short_description;
      break;

    case 3:
      if ((obj->str_mask & STRUNG_DESC1) && obj->description)
        FREE(obj->description);
      obj->description = NULL;
      obj->str_mask |= STRUNG_DESC1;
      ch->desc->str = &obj->description;
      break;

    case 4:
      if (!*string)
      {
        send_to_char("You have to supply a keyword.\r\n", ch);
        return;
      }
      /*
         try to locate extra description
       */
      for (ed = obj->ex_description;; ed = ed->next)
        if (!ed)
        {                       /*
                                   the field was not found. create a new one.
                                 */
          CREATE(ed, struct extra_descr_data, 1, MEM_TAG_EXDESCD);

          ed->next = obj->ex_description;
          obj->ex_description = ed;
          CREATE(ed->keyword, char, strlen(string) + 1, MEM_TAG_STRING);

          strcpy(ed->keyword, string);
          ed->description = 0;
          ch->desc->str = &ed->description;
          send_to_char("New field.\r\n", ch);
          obj->str_mask |= STRUNG_EDESC;
          break;
        }
        else if (!str_cmp(ed->keyword, string))
        {                       /*
                                   the field exists
                                 */
          FREE(ed->description);
          ed->description = 0;
          ch->desc->str = &ed->description;
          send_to_char("Modifying description.\r\n", ch);
          obj->str_mask |= STRUNG_EDESC;
          break;
        }
      ch->desc->max_str = MAX_STRING_LENGTH;
      return;                   /*
                                   the stndrd (see below) procedure does not
                                   apply here
                                 */
      break;
    case 7:
      if (!*string)
      {
        send_to_char("You must supply a field name.\r\n", ch);
        return;
      }
      /*
         try to locate field
       */
      for (ed = obj->ex_description;; ed = ed->next)
        if (!ed)
        {
          send_to_char("No field with that keyword.\r\n", ch);
          return;
        }
        else if (!str_cmp(ed->keyword, string))
        {
          FREE(ed->keyword);
          if (ed->description)
            FREE(ed->description);
          /*
             delete the entry in the desr list
           */
          if (ed == obj->ex_description)
            obj->ex_description = ed->next;
          else
          {
            for (tmp = obj->ex_description; tmp->next != ed;
                 tmp = tmp->next) ;
            tmp->next = ed->next;
          }
          FREE(ed);
          ed = NULL;
          send_to_char("Field deleted.\r\n", ch);
          return;
        }
      break;

    default:
      send_to_char("That field is undefined for objects.\r\n", ch);
      return;
      break;
    }
  }

  if (*ch->desc->str)
  {
    FREE(*ch->desc->str);
  }
  if (*string)
  {                             /* there was a string in the argument array */
    if (strlen(string) > length[field - 1])
    {
      send_to_char("String too long - truncated.\r\n", ch);
      *(string + length[field - 1]) = '\0';
    }
    CREATE(*ch->desc->str, char, strlen(string) + 1, MEM_TAG_STRING);

    strcpy(*ch->desc->str, string);
    ch->desc->str = 0;
    send_to_char("Ok.\r\n", ch);
  }
  else
  {                             /* there was no string. enter string mode */
    send_to_char("Enter string (/s saves /h for help)\r\n\r\n", ch);
    *ch->desc->str = 0;
    ch->desc->max_str = length[field - 1];
  }
}

// Like string, but used for changing bogus names. Strings name, kills old pfile.
void do_rename(P_char ch, char *arg, int cmd)
{
  char rest[MAX_INPUT_LENGTH];
  char type[MAX_STRING_LENGTH];
  char who_to_rename[MAX_STRING_LENGTH], new_name[MAX_STRING_LENGTH];
  bool has_wrong_arg = FALSE;

  arg = one_argument(arg, type);
  if( !type || !*type || !arg || !*arg || !(is_abbrev(type, "char") || is_abbrev(type, "ship")) )
  {
    has_wrong_arg = TRUE;
  }

  if( !has_wrong_arg )
  {
    // get name of char whos name/ship will be renamed
    arg = one_argument(arg, who_to_rename);
    if( !who_to_rename || !*who_to_rename || !*arg || !arg )
    {
      has_wrong_arg = TRUE;
    }
  }

  if( !has_wrong_arg )
  {
    if( is_abbrev(type, "char") )
    {
      // get new name, drop anything after the new name since names only accept one word.
      half_chop(arg, new_name, rest);
      if( !new_name || !*new_name )
      {
        has_wrong_arg = TRUE;
      }
      else
      {
        strcpy(new_name, strip_ansi(new_name).c_str());

        if( rename_character(ch, who_to_rename, new_name) == TRUE )
        {
           send_to_char("Name changed, old one deleted. Good job!\r\n", ch);
        }
        else
        {
          send_to_char("Character name change failed!\r\n", ch);
        }
      }
    }
    else if( is_abbrev(type, "ship") )
    {
      // renaming the ship.
      if( rename_ship(ch, who_to_rename, skip_spaces(arg)) == TRUE )
      {
         send_to_char("Ship name changed. Good job!\r\n", ch);
      }
      else
      {
         send_to_char("Ship name change failed!\r\n", ch);
      }
    }
    else
    {
      send_to_char("You should never see this.\n", ch);
      has_wrong_arg = TRUE;
    }
  }

  if( has_wrong_arg )
  {
    send_to_char("Syntax:\nrename <char | ship> <char/owner name> <new name>.\n", ch);
  }
}

int mob_do_rename_hook(P_char npc, P_char ch, int cmd, char *arg)
{
  char old_name[MAX_STRING_LENGTH];
  char new_name[MAX_STRING_LENGTH];
  char askWho[MAX_STRING_LENGTH];
  char askFor[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC || cmd != CMD_ASK || !IS_ALIVE(npc) || !IS_ALIVE(ch) || !arg )
  {
    return FALSE;
  }

  arg = one_argument(arg, askWho);
  // If they're asking someone else..
  if( !strstr(npc->player.name, askWho) )
  {
    return FALSE;
  }
  arg = one_argument(arg, askFor);
  if( !str_cmp(askFor, "rename") )
  {
    arg = one_argument(arg, new_name);
    if( !*new_name || !new_name )
    {
      snprintf(buffer, MAX_STRING_LENGTH, "Syntax: ask %s rename <newname>\r\n", npc->player.short_descr);
      send_to_char(buffer, ch);
      return TRUE;
    }
    if( !CAN_SPEAK(npc) )
    {
      return TRUE;
    }
    if( !CAN_SEE(npc, ch) )
    {
      mobsay(npc, "How may I be of help if I cannot see you?");
      return TRUE;
    }

    /* count money */
    int renamePrice = get_property("mobspecs.rename.price", 5000000);
    if( GET_MONEY(ch) < renamePrice )
    {
      snprintf(buffer, MAX_STRING_LENGTH, "You need %s&n more!", coin_stringv(renamePrice - GET_MONEY(ch)));
      mobsay(npc, buffer);
      return TRUE;
    }

    // Bad names handled in rename_character function.
    strcpy(old_name, GET_NAME(ch)); 
    if( !rename_character(ch, old_name, new_name) )
    {
      return TRUE;
    }
    rename_ship_owner( old_name, new_name );
    SUB_MONEY(ch, renamePrice, 0);

    snprintf(buffer, MAX_STRING_LENGTH, "&+WCongratulations! From now on you will be known as %s!\r\n", GET_NAME(ch));
    send_to_char(buffer, ch);

    wizlog(AVATAR, "%s renamed %sself to %s\r\n", old_name, GET_SEX(ch) == SEX_MALE ? "him" : "her", new_name);
    logit(LOG_PLAYER, "%s renamed %sself to %s\r\n", old_name, GET_SEX(ch) == SEX_MALE ? "him" : "her", new_name);
    sql_log(ch, PLAYERLOG, "Renamed self from %s to %s\r\n", old_name, new_name);
    return TRUE;
  }

  return FALSE;
}

bool rename_spellbook( char *old_name, char *new_name )
{
  char  old_book[MAX_STRING_LENGTH];
  char  new_book[MAX_STRING_LENGTH];
  char  command[MAX_STRING_LENGTH];
  FILE *file;

  snprintf(old_book, MAX_STRING_LENGTH, "%s/%c/%s.spellbook", SAVE_DIR, LOWER(*old_name), old_name );
  snprintf(new_book, MAX_STRING_LENGTH, "%s/%c/%s.spellbook", SAVE_DIR, LOWER(*new_name), new_name );

  // If old_name has a spellbook...
  if ((file = fopen( old_book, "r")))
  {
    fclose( file );
    // Move it to new_name.
    snprintf(command, MAX_STRING_LENGTH, "mv -f %s %s", old_book, new_book );
    if( system( command ) == 0 )
    {
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }

  return TRUE;
}

bool rename_craftlist( char *old_name, char *new_name )
{
  char  old_book[MAX_STRING_LENGTH];
  char  new_book[MAX_STRING_LENGTH];
  char  command[MAX_STRING_LENGTH];
  FILE *file;

  snprintf(old_book, MAX_STRING_LENGTH, "%s/Tradeskills/%c/%s.crafting", SAVE_DIR, LOWER(*old_name), old_name );
  snprintf(new_book, MAX_STRING_LENGTH, "%s/Tradeskills/%c/%s.crafting", SAVE_DIR, LOWER(*new_name), new_name );

  // If old_name has a crafting book...
  if ((file = fopen( old_book, "r")))
  {
    fclose( file );
    // Move it to new_name.
    snprintf(command, MAX_STRING_LENGTH, "mv -f %s %s", old_book, new_book );
    if( system( command ) == 0 )
    {
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }

  return TRUE;
}



/* ------------------------------------------------------------------------------ */
/* pure char rename function, to be called from rename hooks or command functions */
/* ------------------------------------------------------------------------------ */
bool rename_character(P_char ch, char *old_name, char *new_name)
{
  char     buf[256], *buff;
  struct   acct_chars *c = NULL;
  P_char   doofus;

  // Validate new name (sets new_name to all lowercase)
  if( _parse_name(new_name, new_name) )
  {
    send_to_char("Illegal name, please try again.\r\n", ch);
    return FALSE;
  }

  // Check for char in game...
  if( !(doofus = get_char_vis(ch, old_name)) )
  {
    send_to_char("I don't know anyone by that name...\r\n", ch);
    return FALSE;
  }
  // No renaming NPCs.
  if( IS_NPC(doofus) )
  {
    send_to_char("You can't rename an NPC.\r\n", ch);
    return FALSE;
  }

  // No renaming those who are above/equal your level (yourself is an exception).
  if( GET_LEVEL(doofus) >= GET_LEVEL(ch) && ch != doofus )
  {
    send_to_char("We call that cheating bud.\r\n", ch);
    wizlog(AVATAR, "%s attempted to rename an equal or superior (\"%s\" to \"%s\")!", GET_NAME(ch), old_name, new_name);
    return FALSE;
  }

/* Simplified this in above.
  P_char finger_foo = NULL;
  finger_foo = (struct char_data *) mm_get(dead_mob_pool);
  finger_foo->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

  // Check existance of oldname
  if( restoreCharOnly(finger_foo, skip_spaces(old_name)) >= 0 && finger_foo )
  {
    if( GET_LEVEL(finger_foo) > GET_LEVEL(ch) )
    {
      send_to_char("We call that cheating bud.\r\n", ch);
      wizlog(AVATAR, "%s attempted to rename (%s) to another gods name (%s) !", GET_NAME(ch), old_name, new_name);
      free_char(finger_foo);
      return FALSE;
    }
    free_char(finger_foo);
  }
*/

  // New name must not be in use.
  // Simplifying this!
//  if( restoreCharOnly(finger_foo, skip_spaces(new_name)) < 0 || !finger_foo )
  if( !pfile_exists(SAVE_DIR, new_name) )
  {
    /* be sure new name isn't in declined list */
    if( pfile_exists(BADNAME_DIR, new_name) )
    {
      /* if GOD changing someones name */
      if( IS_TRUSTED(ch) )
      {
        // Remove new_name from BADNAME_DIR
        snprintf(buf, 256, "%s/%c/%s", BADNAME_DIR, *new_name, new_name );
        unlink(buf);
        send_to_char("Name was in the declined list, but has been removed. :)\r\n", ch);
      }
      else
      {
        send_to_char("That name has been declined before, and would be now too!\r\n", ch);
        return FALSE;
      }
    }

    /* If failed rename spellbook (list of conjurable pets), then don't rename */
    if( !rename_spellbook( old_name, new_name ) )
    {
      send_to_char("Failed to move spellbook?!?\r\n", ch);
      return FALSE;
    }

    /* If failed rename craft/forge list, then don't rename */
    if( !rename_craftlist( old_name, new_name ) )
    {
      send_to_char("Failed to move craft list?!?\r\n", ch);
      return FALSE;
    }

    /* if failed rename locker - is in use or something wierd, then dont rename */
    if( !rename_locker(ch, old_name, new_name) )
    {
      return FALSE;
    }

    /* if failed rename ship owner - then dont rename */
    if( get_ship_from_char(doofus) && !rename_ship_owner(old_name, new_name) )
    {
      return FALSE;
    }

    /* if GOD changing someones name, put old one to deny list */
    if( IS_TRUSTED(ch) )
    {
       deny_name(GET_NAME(doofus));
    }

    moveToBackup(GET_NAME(doofus));

    /* put new name and save char file */
    CAP(new_name);
    GET_NAME(doofus) = str_dup(new_name);
    // Need to update the core stuff here.
    sql_save_player_core(doofus);
    writeCharacter(doofus, 1, doofus->in_room);

#ifdef USE_ACCOUNT
    c = find_char_in_list(doofus->desc->account->acct_character_list, new_name);
    if( c )
    {
      FREE(c->charname);
      c->charname = str_dup(new_name);
      write_account(doofus->desc->account);
    }
#endif
  }
  else
  {
    send_to_char("Can't use that name, as it belongs to another.\r\n", ch);
    return FALSE;
  }

  return TRUE;
}

/* One_Word is like one_argument, except that words in quotes '' are */
/* regarded as ONE word                                              */
char *one_word(char *argument, char *first_arg)
{
  int      found, begin, look_at;

  found = begin = 0;

  do
  {
    for (; isspace(*(argument + begin)); begin++) ;

    if (*(argument + begin) == '\"')
    {                           /* is it a quote */
      begin++;

      for (look_at = 0; (*(argument + begin + look_at) >= ' ') &&
           (*(argument + begin + look_at) != '\"'); look_at++)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
      if (*(argument + begin + look_at) == '\"')
        begin++;
    }
    else
    {
      for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    }
    *(first_arg + look_at) = '\0';
    begin += look_at;
  }
  while (fill_word(first_arg));

  return (argument + begin);
}

/*
Commented out by Weebler
void clear_help_index(struct help_index_element **list_head,
                      const int help_size)
{
  int      i;

  if (!*list_head)
    return;

  for (i = 0; i < help_size; i++)
  {
    FREE((*list_head)[i].keyword);
  }

  FREE(*list_head);

  *list_head = NULL;
}
*/

/*struct help_index_element *build_help_index(FILE * fl, int *num)
{
  // Commented out by Weebler
  int      nr = -1, issorted, i, j;
  struct help_index_element *list = 0, mem;
  char     buf[MAX_STRING_LENGTH], tmp[MAX_STRING_LENGTH], *scan;
  char    *pbuf;
  long     pos;

  for (;;)
  {
    pos = ftell(fl);
    fgets(buf, MAX_STRING_LENGTH, fl);
    *(buf + strlen(buf) - 1) = '\0';
    scan = buf;
    for (;;)
    {
      /* extract the keywords * /
      scan = one_word(scan, tmp);
      stripansi_2(tmp, pbuf);
      if (!*tmp)
        break;
      if (!list)
      {
        CREATE(list, struct help_index_element, 1);

        nr = 0;
      }
      else
        RECREATE(list, struct help_index_element, ++nr + 1);

      list[nr].pos = pos;
//      CREATE(list[nr].keyword, char, strlen(tmp) + 1);

//      strcpy(list[nr].keyword, tmp);
      list[nr].keyword = pbuf;
      if (!list[nr].keyword)
      {
        list[nr].keyword = str_dup("\0");
      }
    }
    /* skip the text * /
    do
      fgets(buf, MAX_STRING_LENGTH, fl);
    while (*buf != '#');
    if (*(buf + 1) == '~')
      break;
  }
  /* we might as well sort the stuff * /
  do
  {
    issorted = 1;
    for (i = 0; i < nr; i++)
      if (str_cmp(list[i].keyword, list[i + 1].keyword) > 0)
      {
        mem = list[i];
        list[i] = list[i + 1];
        list[i + 1] = mem;
        issorted = 0;
      }
  }
  while (!issorted);

  /*
     ok, new step, once list is sorted, scan down it, and make note of the list
     location of the first element that starts with each letter.  Save this in
     a global array.  Then do_help will reference that array, and use the
     indicated element for the start of a linear search.  Doing it this way so
     that partial matches will always match the first element they fit, not the
     one it happens to land on (as previous binary method did, all too often).
     JAB
   */

  /* init the lookup array * /
  for (i = 0; i < 27; i++)
  {
    help_array[i][0] = -1;
    help_array[i][1] = nr + 1;
  }

  /*
     ok, help_array works like this: 0 - non-alpha entries 1 - [Aa] . . 26 -
     [Zz] Help is case_insensitive, and things are lowercased as SOP, so
     basically all non-lapha entries will sort out lower in the list[] than the
     alpha entries (shouldn't be many non-alpha anyway).
     help_array[0] will either be 0 or -1, always, depending on whether or not
     there are any non-alpha entries, so set that up outside the loop, if no
     entires start with a given letter, then it's entry will be -1, and do_help
     will automatically report 'no help avail'.
   * /

  if (LOWER(list[0].keyword[0]) < 'a')
    help_array[0][0] = 0;

  i = 0;
  j = 0;
  while (LOWER(list[i].keyword[0]) < 'a')
    i++;

  for (; i <= nr; i++)
  {
    if (LOWER(list[i].keyword[0]) == ('a' + j - 1))
      continue;
    if (LOWER(list[i].keyword[0]) > 'z')
      break;                    /* not gonna deal with non-alpha above 'z', meaningless * /

    help_array[(int) j][1] = (int) i;   /* end of previous entry * /

    j = (LOWER(list[i].keyword[0]) - 'a' + 1);

    help_array[(int) j][0] = (int) i;   /* start of new entry * /
  }

  *num = nr;
  return (list);
}*/

/*********************************************************************
* New Pagination Code
* Michael Buselli submitted the following code for an enhanced pager
* for DurisMUD.  All functions below are his.  --Fafhrd
*
*********************************************************************/

#define PAGE_WIDTH      80

/* Traverse down the string until the begining of the next page has been
 * reached.  Return NULL if this is the last page of the string.
 */
char    *next_page(char *str, struct descriptor_data *d)
{
  int      col = 1, line = 1, spec_code = FALSE;

  /* Arih : this will fix the crash if paging was on when rent the char. */
  /* Safety check: if character is invalid, return NULL to disable paging */
  if (!d || !d->character)
    return NULL;

  for (;; str++)
  {
    /* If end of string, return NULL. */
    if (*str == '\0')
      return NULL;

    /* If we're at the start of the next page, return this fact. */
    /* The size - 4, is to keep infobar clean, for those who use */
    else if (line >
             MAX(8, (GET_PLYR(d->character)->only.pc->screen_length - 4)))
      return str;

    else if (*str == '&' && *(str + 1))
    {
      if ((*(str + 1) == 'n') || (*(str + 1) == 'N'))
      {
        str += 1;
        continue;
      }
      if (*(str + 2) && ((*(str + 1) == '+') ||
                         (*(str + 1) == '=') || (*(str + 1) == '-')))
      {
        str += 2;
        continue;
      }
    }
    /* Check for the begining of an ANSI color code block. */
    else if (*str == '\x1B' && !spec_code)
      spec_code = TRUE;

    /* Check for the end of an ANSI color code block. */
    else if (*str == 'm' && spec_code)
      spec_code = FALSE;

    /* Check for everything else. */
    else if (!spec_code)
    {
      /* Newline puts us on the next line. */
      if (*str == '\n')
      {
        col = 1;
        line++;
      }
      else if (col++ > PAGE_WIDTH)
      {
        col = 1;
        line++;
      }
    }
  }
}

// this is a fake now, your command output will be paged automatically
// now without you having to care about it - tharkun
void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
  if (d)
    send_to_char(str, d->character);
}

void page_string_real(struct descriptor_data *d, char *str)
{
  char    *s;
  int      i, pages;

  if (!IS_SET(d->character->specials.act, PLR_PAGING_ON) ||
      d->connected == CON_MAIN_MENU)
  {
    /* added CON_MAIN_MENU to disable paging when doing 'who' from menu. -JAB */
    SEND_TO_Q(str, d);
    return;
  }

  for (s = str, pages = 1; (s = next_page(s, d)); pages++)
    ;

  d->showstr_count = pages;
  CREATE(d->showstr_vector, char*, pages, MEM_TAG_ARRAY);
  d->showstr_head = str_dup(str);

  if (pages)
    s = d->showstr_vector[0] = d->showstr_head;

  for (i = 1; i < pages && s; i++)
    s = d->showstr_vector[i] = next_page(s, d);

  d->showstr_page = 0;
  show_string(d, "");
}

void free_paging_data(struct descriptor_data *d)
{
  if (d->showstr_vector)
  {
    FREE(d->showstr_vector);
    d->showstr_vector = 0;
  }
  if (d->showstr_head)
  {
    FREE(d->showstr_head);
    d->showstr_head = 0;
  }
  d->showstr_count = 0;
}

/* The call that displays the next page. */
void show_string(struct descriptor_data *d, const char *input)
{
  char     buffer[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int      diff;

  buffer[0] = '\0';
  buf[0] = '\0';

  one_argument(input, buf);

  /* Q is for quit. :) */
  if (LOWER(*buf) == 'q')
  {
    free_paging_data(d);
    return;
  }
  /* R is for refresh, so back up one page internally so we can display
   * it again.
   */
  else if (LOWER(*buf) == 'r')
    d->showstr_page = MAX(0, d->showstr_page - 1);

  /* B is for back, so back up two pages internally so we can display the
   * correct page here.
   */
  else if (LOWER(*buf) == 'b')
    d->showstr_page = MAX(0, d->showstr_page - 2);

  /* Feature to 'goto' a page.  Just type the number of the page and you
   * are there!
   */
  else if (isdigit(*buf))
    d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1));

  else if (*buf)
  {
    free_paging_data(d);
    return;
  }
  /* If we're displaying the last page, just send it to the character, and
   * then free up the space we used.
   */
  if (d->showstr_page + 1 >= d->showstr_count)
  {
    send_to_char(d->showstr_vector[d->showstr_page], d->character);
    free_paging_data(d);
  }
  /* Or if we have more to show.... */
  else
  {
    strncpy(buffer, d->showstr_vector[d->showstr_page],
            (unsigned)
            (diff = ((intptr_t) d->showstr_vector[d->showstr_page + 1])
             - ((intptr_t) d->showstr_vector[d->showstr_page])));
    buffer[diff] = '\0';
    send_to_char(buffer, d->character);
    send_to_char("&N", d->character);
    d->showstr_page++;
  }
}

//--------------------------------------------------------------------
// for debug purpose, to check if correctly loaded/unloded char lists
//--------------------------------------------------------------------
void for_debug_print_char_list(P_char ch)
{
   P_char chLocker = NULL;
   P_desc chDesc = NULL;
   char buffer[MAX_STRING_LENGTH];
   int nCnt;

   nCnt = 1;
   send_to_char("------------------\r\n", ch);
   send_to_char("- char list      -\r\n", ch);
   send_to_char("------------------\r\n", ch);

   for (chLocker = character_list; chLocker; chLocker = chLocker->next)
   {
           snprintf(buffer, MAX_STRING_LENGTH, "%d) %s\r\n", nCnt, GET_NAME(chLocker));
           send_to_char(buffer, ch);
           nCnt++;
   }
   
   send_to_char("------------------\r\n", ch);
   
   nCnt = 1;
   send_to_char("------------------\r\n", ch);
   send_to_char("- desc list      -\r\n", ch);
   send_to_char("------------------\r\n", ch);

   for (chDesc = descriptor_list; chDesc; chDesc = chDesc->next)
   {
           snprintf(buffer, MAX_STRING_LENGTH, "%d) %s\r\n", nCnt, GET_NAME(chDesc->character));
           send_to_char(buffer, ch);
           nCnt++;
   }
   
   send_to_char("------------------\r\n", ch);
   
   return;
}
