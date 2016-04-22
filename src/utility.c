/*
 * ***************************************************************************
 * *  File: utility.c                                          Part of Duris *
 * *  Usage: misc. small functions/macros                                      *
 * *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 * *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 * ***************************************************************************
 */

#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
using namespace std;
#ifdef _HPUX_SOURCE
#include <varargs.h>
#endif

#include "utility.h"

#include "comm.h"
#include "db.h"
#include "events.h"
#include "graph.h"
#include "interp.h"
#include "justice.h"
#include "new_combat.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "assocs.h"
#include "grapple.h"
#include "map.h"
#include "specializations.h"
#include "defines.h"
#include "sql.h"
#include "mm.h"
#include "epic.h"

/*
 * extern variables
 */
extern Skill skills[];
extern int RUNNING_PORT;
extern int allowed_secondary_classes[][5];
extern int class_table[LAST_RACE + 1][CLASS_COUNT + 1];
extern int num_appearances;
extern int num_shapes;
extern int num_modifiers;
extern P_char character_list;
extern struct str_app_type str_app[];
extern P_desc descriptor_list;
extern P_room world;
extern P_index obj_index;
extern const struct racial_data_type racial_data[];
extern const struct stat_data stat_factor[];
extern const int movement_loss[];
extern int top_of_zone_table;
extern const int top_of_world;
extern struct command_info cmd_info[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern int hometown_arena[][3];
extern const struct race_names race_names_table[];
extern const char *appearance_descs[];
extern const char *shape_descs[];
extern const char *modifier_descs[];
extern const struct class_names class_names_table[];
extern const char *god_list[];
extern const char *specdata[][MAX_SPEC];
uint     debugcount = 0;
extern P_index mob_index;
extern const int rev_dir[];
extern void event_spellcast(P_char, P_char, P_obj, void *);
int ship_obj_proc(P_obj obj, P_char ch, int cmd, char *arg);
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern const char *sector_types[];

char     GS_buf1[MAX_STRING_LENGTH];

int      is_ice(P_char ch, int room);
int      CheckFor_remember(P_char ch, P_char victim);

/* vis_mode:
 * 1 - God sight: sees everything
 * 2 - Normal in light, ultra in dark: sees most things
 * 3 - Infra in the dark: quite limited
 * 4 - Wraithsight: sees all beings, no objects
 * 5 - Too dark: sees nothing.
 * 6 - Too bright: sees nothing.
 */
int get_vis_mode(P_char ch, int room)
{
  P_char tch;
  bool   flame, globe;

  if( IS_TRUSTED(ch) )
  {
    return 1;
  }
  if( IS_AFFECTED(ch, AFF_WRAITHFORM) )
  {
    return 4;
  }
  // Twilight rooms: everyone can see.
  if( IS_TWILIGHT_ROOM(room) )
  {
    return 2;
  }

  flame = globe = FALSE;
  for (tch = world[room].people; tch; tch = tch->next_in_room)
  {
    if( IS_AFFECTED4(tch, AFF4_MAGE_FLAME) )
    {
      flame = TRUE;
    }
    if( IS_AFFECTED4(tch, AFF4_GLOBE_OF_DARKNESS) )
    {
      globe = TRUE;
    }
  }

  // Normal dayvision: Not dayblind and in lit room.
  if( !IS_DAYBLIND(ch) && (CAN_DAYPEOPLE_SEE(room) || flame) )
  {
    return 2;
  }
  // Normal nightvision: Ultravision and dark/twilight room.
  if( IS_AFFECTED2(ch, AFF2_ULTRAVISION) && (CAN_NIGHTPEOPLE_SEE(room) || globe) )
  {
    return 2;
  }
  // Normal inside rooms are twilight, unless magic lit/darked.
  if( (IS_SET(world[room].room_flags, LOCKER) || IS_INSIDE(room)) && !IS_MAGIC_LIGHT(room) && !IS_MAGIC_DARK(room) )
  {
    return 2;
  }
  // Underdark rooms are visible to Ultravision naturally.
  if( IS_UNDERWORLD(room) && IS_AFFECTED2(ch, AFF2_ULTRAVISION) )
  {
    return 2;
  }

  // If they have infra, they use it here; we failed at regular vision.
  if( IS_AFFECTED(ch, AFF_INFRAVISION) )
  {
    return 3;
  }

  // Ok, they can't see for some reason:
  // If in dark (no infra/ultra)
  if( CAN_NIGHTPEOPLE_SEE(room) )
  {
    return 5;
  }
  // Not dark, then they're dayblind via sun or magic light.
  else
  {
    return 6;
  }
}

int god_check(char *name)
{
  int      i;

  for (i = 0; *god_list[i] != '\0'; i++)
  {
    if (isname(god_list[i], name))
    {
      return TRUE;
    }
  }
  return FALSE;
}

int can_exec_cmd(P_char ch, int cmd)
{
  if( IS_PC(ch) )
  {
    for( int i = 0; i < ch->only.pc->numb_gcmd; i++ )
    {
      if( (ch->only.pc->gcmd_arr[i] == cmd) && IS_TRUSTED(ch) )
      {
        return TRUE;
      }
    }
  }

  // NPCs do NOT execute God commands.
  if( cmd_info[cmd].minimum_level > MAXLVLMORTAL && IS_NPC(ch) )
  {
    return FALSE;
  }

  if( cmd_info[cmd].minimum_level <= GET_LEVEL(ch) )
  {
    return TRUE;
  }

  return FALSE;
}

bool is_ansi_char(char collor_char)
{
   switch(collor_char)
   {
      case 'W':
      case 'w':
      case 'L':
      case 'l':
      case 'R':
      case 'r':
      case 'B':
      case 'b':
      case 'G':
      case 'g':
      case 'Y':
      case 'y':
      case 'C':
      case 'c':
      case 'M':
      case 'm':
         return TRUE;
      default:
         break;
   }
   
   return FALSE;
}

/* w - white   l - black   r - red   b - blue      */
/* g - green   y - yellow  c - cyan   m - magenta  */
/* N - color terminator */
int is_valid_ansi(char *mesg, bool can_set_blinking)
{
  int      i;

  if (mesg == NULL)
  {
    return FALSE;
  }

  i = 0;
  while (mesg[i] != '\0')
  {
     if( mesg[i] == '&' )
     {
        if( mesg[i+1] == '\0' )
           return FALSE;
        
        if( mesg[i+1] != 'N' && mesg[i+1] != 'n' )
        {
           if( mesg[i+1] == '+' || mesg[i+1] == '-' )
           {
              if( mesg[i+2] == '\0' )
                 return FALSE;
              if( !is_ansi_char(mesg[i+2]) )
                 return FALSE;
              
              if(!can_set_blinking && (mesg[i+1] == '-') && isupper(mesg[i+2]))
                 return FALSE;
           }
           else
           if( mesg[i+1] == '=' )
           {
              if( mesg[i+2] == '\0' || mesg[i+3] == '\0')
                 return FALSE;
              
              if( !is_ansi_char(mesg[i+2]) || !is_ansi_char(mesg[i+3]) )
                 return FALSE;

              if(!can_set_blinking && isupper(mesg[i+2]))
                 return FALSE;
           }
           else
              return FALSE;
        }
     }
     i++;
  }

  return TRUE;
}

bool is_valid_ansi_with_msg(P_char ch, char *ansi_text, bool can_set_blinking)
{
   if( !is_valid_ansi(ansi_text, can_set_blinking) )
   {
      if( IS_TRUSTED(ch) )
      {
         send_to_char("Invalid ANSI characters in name.\n", ch);
      }
      else
      {
         act("&+L~~~~&+W(o)&+L~~~~&N", FALSE, ch, 0, 0, TO_CHAR);
         act("&+LThe clouds part above and the unblinking &+WEye of Zion &+Llooks down at you skeptically.&N",
             FALSE, ch, 0, 0, TO_CHAR);
         act("&+LA booming voice calls down, &+w'Check the helpfiles on how to use ANSI properly, puny mortal!'&N",
             FALSE, ch, 0, 0, TO_CHAR);
      }

      return FALSE;
   }

   return TRUE;
}

__attribute__ ((deprecated))
     char    *stripansi(const char *mesg)
{
  char     tmp_buf[MAX_STRING_LENGTH];
  int      i, j, length;

  if (mesg == NULL)
  {
    return NULL;
  }
  length = strlen(mesg);

  for (i = 0, j = 0; j < length; )
  {
    while (mesg[j] == '&' && j < length-1)
    {
      switch (mesg[j+1])
      {
      case '-':
      case '+':
        j += 3;
        break;
      case '=':
        j += 4;
        break;
      case 'n':
      case 'N':
      case 'L':
      case 'l':
        j += 2;
        break;
      default:
        j++;
        break;
      }
    }
    tmp_buf[i] = mesg[j];
    j++;
    i++;
  }
  tmp_buf[i] = '\0';
  if (tmp_buf[0] == '\0')
  {
    return NULL;
  }
  return str_dup(tmp_buf);
}

int stripansi_2(const char *mesg, char *destination)
{
  int      i,j, length;

  if ((mesg == NULL) || (destination == NULL))
  {
    return FALSE;
  }
  length = strlen(mesg);

  for (i = 0, j = 0; j < length; )
  {
    while (mesg[j] == '&' && j < length-1)
    {
      switch (mesg[j+1])
      {
      case '-':
      case '+':
        j += 3;
        break;
      case '=':
        j += 4;
        break;
      case 'n':
      case 'N':
      case 'L':
      case 'l':
        j += 2;
        break;
      default:
        j++;
        break;
      }
    }
    destination[i] = mesg[j];
    j++;
    i++;
  }
  destination[i] = '\0';
  if (destination[0] == '\0')
  {
    return FALSE;
  }
  return i;
}

char    *striplinefeed(char *mesg)
{
  char     tmp_buf[MAX_STRING_LENGTH];
  int      i;

  if (mesg == NULL)
  {
    return NULL;
  }
  i = 0;
  while (*mesg != '\0')
  {
    while (*mesg == '\r' || *mesg == '\n')
    {
      mesg++;
    }
    tmp_buf[i] = *mesg;
    mesg++;
    i++;
  }
  tmp_buf[i] = '\0';
  if (tmp_buf[0] == '\0')
  {
    return NULL;
  }
  return str_dup(tmp_buf);
}

/* yes, we're not using C++, so use macros..  bleh */
/* macros are pass-by-name, and that screws everything up */
#if 0
int MIN(int a, int b)
{
  if (a < b)
    return a;
  else
    return b;
}

int MAX(int a, int b)
{
  if (a > b)
    return a;
  else
    return b;
}
#endif
int BOUNDED(int a, int b, int c)
{
  return (MIN(MAX(a, b), c));
}

float BOUNDEDF(float a, float b, float c)
{
  if (b < a)
    b = a;
  else if (b > c)
    b = c;

  return b;
}


/* too bad we're not using C++...

template <class T>
T BOUNDED(T a, T b, T c)
{
  return (MIN (MAX (a, b), c));
}
*/

/*
 * creates a random number in interval [from;to]
 */

int number(int from, int to)
{
  if (to == from)
    return from;
    
  if (to < from)
  {
    return (( /*erandom () */ (int) (genrand_real2() * (from - to + 1))) +
            to);
  }
  return (( /*erandom () */ (int) (genrand_real2() * (to - from + 1))) +
          from);
}

/*
 * simulates dice roll
 */

int dice(int num, int size)
{
  int      r;
  int      sum = 0;

  if (size < 1)
  {
    size = 1;
  }
  if (num < 1)
  {
    num = 1;
  }
  for (r = 1; r <= num; r++)
    sum += number(1, size);

  return (sum);
}

/*
 * Create a duplicate of a string
 */

char *str_dup( const char *source )
{
  char    *nnew;

  if( source == NULL )
  {
    return NULL;
  }

  CREATE(nnew, char, strlen(source) + 1, MEM_TAG_STRING);
  return strcpy(nnew, source);
}

/*
 * free a string created with str_dup()
 */
void str_free(char *source)
{
  if (source == NULL)
    return;

  FREE(source);
  source = NULL;
}

/*
 * returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2
 */
/*
 * scan 'till found different or end of both
 */
#if 0
int str_cmp(const char *arg1, const char *arg2)
{
  int      chk, i;

  /*
   * NULL ptr checks added, SAM 7-94
   */
  if ((arg1 == NULL) && (arg2 == NULL))
    return (0);
  else if (arg1 == NULL)
    return (-1);
  else if (arg2 == NULL)
    return (1);

  for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
      if (chk < 0)
        return (-1);
      else
        return (1);
  return (0);
}
#endif
/*
 * returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2
 */
/*
 * scan 'till found different, end of both, or n reached
 */
/*
 * case insensitive, I'm leaving it, but it's named wrong -SE
 */

int strn_cmp(const char *arg1, const char *arg2, uint n)
{
  int      chk, i;

  for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
      if (chk < 0)
        return (-1);
      else
        return (1);

  return (0);
}

int str_n_cmp(const char *argv1, const char *argv2)
{
  char     name[MAX_INPUT_LENGTH];

  strncpy(name, argv1, strlen(argv2));
  return str_cmp(name, argv2);
}

/* returns TRUE if char is in global list, FALSE if not */

const int char_in_list(const P_char ch)
{
  register P_char tmp;

  if (!ch)
    return FALSE;

  for (tmp = character_list; tmp; tmp = tmp->next)
  {
    if (tmp == ch)
      return TRUE;
  }

  return FALSE;
}

/* returns TRUE if char is in given real room, FALSE if not */

const int is_char_in_room(P_char ch, const int room)
{
  register P_char tmp;

  if (!ch || room >= top_of_world || room == 0)
    return FALSE;

  for (tmp = world[room].people; tmp; tmp = tmp->next_in_room)
  {
    if (tmp == ch)
      return TRUE;
  }

  return FALSE;
}

/*
 * yes, that's right, delete a specified char from a string.  amazing, eh?
 */

char    *deleteChar(char *strn, const unsigned long strnPos)
{
  ulong    i, len = strlen(strn);


  if (strnPos >= len)
    return strn;

  for (i = strnPos; i < len; i++)
  {
    strn[i] = strn[i + 1];
  }

  return strn;
}

/*
 * if substr is on left of str, return TRUE
 */

char strleft(const char *strn, const char *substrn)
{
  register ulong i, len = strlen(strn), sublen = strlen(substrn);


  if (sublen > len)
    return FALSE;

  for (i = 0; i < sublen; i++)
  {
    if (strn[i] != substrn[i])
      return FALSE;
  }

  return TRUE;
}


/*
 * convert a command to an exit numb
 */

int cmd_to_exitnumb(int cmd)
{
  if ((cmd >= CMD_NORTH) && (cmd <= CMD_DOWN))
    return (cmd - 1);

  if ((cmd >= CMD_NORTHWEST) && (cmd <= CMD_SOUTHEAST))
    return ((cmd - CMD_NORTHWEST) + CMD_DOWN);

  if ((cmd >= CMD_NW) && (cmd <= CMD_SE))
    return ((cmd - CMD_NW) + CMD_DOWN);

  return -1;
}

/*
 * vice versa
 */

int exitnumb_to_cmd(int exitnumb)
{
  if ((exitnumb >= NORTH) && (exitnumb <= DOWN))
    return exitnumb + 1;

  if ((exitnumb >= NORTHWEST) && (exitnumb <= SOUTHEAST))
    return (exitnumb + CMD_NORTHWEST) - CMD_DOWN;

  return -1;
}

/*
 * writes a string to the log
 */

void logit(const char *filename, const char *format, ...)
{
  FILE    *log_f;
  char     lbuf[MAX_STRING_LENGTH], tbuf[MAX_STRING_LENGTH];
  time_t   ct;

  //long ct;
  va_list  args;

  va_start(args, format);
  ct = time(0);

  bzero(lbuf, MAX_STRING_LENGTH);
  bzero(tbuf, MAX_STRING_LENGTH);

  if( str_cmp(filename, LOG_EVENT) )
  {
    strcpy(tbuf, asctime(localtime(&ct)));
    tbuf[strlen(tbuf) - 1] = 0;
    strcat(tbuf, "::");
  }
  else
    *tbuf = '\0';

  if( str_cmp(filename, LOG_DEBUG) )
    debugcount++;

  vsprintf(lbuf, format, args);

  strcat(tbuf, lbuf);
  strcat(tbuf, "\n");

  log_f = fopen(filename, "a");
  if (!log_f)
  {
    if (str_cmp(filename, LOG_FILE))
      logit(LOG_FILE, "failure opening logfile %s", filename);
    return;
  }
  if (!(debugcount % 500))
  {
    rewind(log_f);
  }
  fputs(tbuf, log_f);
  fclose(log_f);
  if( !str_cmp(filename, LOG_EXIT) )
  {
    perror(tbuf);
  }
  va_end(args);
}

void ereglog(int level, const char *format, ...)
{
  va_list  args;
  char     lbuf[MAX_STRING_LENGTH];
  P_desc   d;

  level = MIN(60, level);
  strcpy(lbuf, "$&+M*** EMAIL REG:&N ");
  va_start(args, format);
  vsprintf(lbuf + strlen(lbuf), format, args);
  strcat(lbuf, "\r\n");
  lbuf[sizeof(lbuf)] = 0;
  for (d = descriptor_list; d; d = d->next)
  {
    if (d->connected == CON_PLYNG &&
        IS_TRUSTED(d->character) &&
        GET_LEVEL(d->character) >= level &&
        IS_SET(d->character->specials.act, PLR_SNOTIFY))
    {
      send_to_char(lbuf, d->character);
    }
  }
  va_end(args);
}

void wizlog(int level, const char *format, ...)
{
  va_list  args;
  char     lbuf[MAX_STRING_LENGTH];
  P_desc   d;

  strcpy(lbuf, "&+C*** WIZLOG:&n ");
  va_start(args, format);
  vsprintf(lbuf + strlen(lbuf), format, args);
  strcat(lbuf, "\r\n");
  lbuf[sizeof(lbuf)] = 0;

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->connected == CON_PLYNG &&
        IS_TRUSTED(d->character) &&
        GET_LEVEL(d->character) >= level &&
        IS_SET(d->character->specials.act, PLR_WIZLOG))
    {
      send_to_char(lbuf, d->character);
    }
  }
  va_end(args);
}

void debug(const char *format, ...)
{
  P_desc   i;
  va_list  args;
  char     lbuf[MAX_STRING_LENGTH];

  strcpy(lbuf, "&+C*** DEBUG:&n ");
  va_start(args, format);
  vsprintf(lbuf + strlen(lbuf), format, args);
  strcat(lbuf, "\r\n");
  lbuf[sizeof(lbuf)] = 0;
  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character &&
        IS_TRUSTED(i->character) &&
        IS_SET(i->character->specials.act, PLR_DEBUG))
      send_to_char(lbuf, i->character);
  va_end(args);
}

void loginlog(int level, const char *format, ...)
{
  va_list  args;
  char     lbuf[MAX_STRING_LENGTH];
  P_desc   d;

  strcpy(lbuf, "&+c*** LOGMSG:&n ");
  va_start(args, format);
  vsprintf(lbuf + strlen(lbuf), format, args);
  strcat(lbuf, "\r\n");
  lbuf[sizeof(lbuf)] = 0;

  for (d = descriptor_list; d; d = d->next)
  {
    if ((d->connected == CON_PLYNG) &&
        IS_TRUSTED(d->character) &&
        (GET_LEVEL(d->character) >= level) &&
        IS_SET(d->character->specials.act, PLR_PLRLOG))
    {
      send_to_char(lbuf, d->character);
    }
  }
  va_end(args);
}

void statuslog(int level, const char *format, ...)
{
  va_list  args;
  char     lbuf[MAX_STRING_LENGTH];
  char     sbuf[MAX_STRING_LENGTH];
  P_desc   d;

  strcpy(lbuf, "&+c*** STATUS:&n ");
  va_start(args, format);
  vsprintf(lbuf + strlen(lbuf), format, args);
  strcat(lbuf, "\r\n");
  lbuf[sizeof(lbuf)] = 0;

  for( d = descriptor_list; d; d = d->next )
  {
    if( d->connected == CON_PLYNG && IS_TRUSTED(d->character)
      && GET_LEVEL(d->character) >= level && IS_SET(d->character->specials.act, PLR_STATUS) )
    {
      send_to_char(lbuf, d->character);
    }
  }
  va_end(args);

  // Remove the newline/carriage return and send it to the log file.
  lbuf[strlen(lbuf)-2] = '\0';
  logit(LOG_STATUS, stripansi(lbuf));
}

void epiclog(int level, const char *format, ...)
{
  va_list  args;
  char     lbuf[MAX_STRING_LENGTH];
  char     sbuf[MAX_STRING_LENGTH];
  P_desc   d;

  strcpy(lbuf, "&+c*** EPIC:&n ");
  va_start(args, format);
  vsprintf(lbuf + strlen(lbuf), format, args);
  strcat(lbuf, "\r\n");
  lbuf[sizeof(lbuf)] = 0;

  for( d = descriptor_list; d; d = d->next )
  {
    if( d->connected == CON_PLYNG && IS_TRUSTED(d->character)
      && GET_LEVEL(d->character) >= level && IS_SET(d->character->specials.act3, PLR3_EPICWATCH) )
    {
      send_to_char(lbuf, d->character);
    }
  }
  va_end(args);

  // Remove the newline/carriage return and send it to the log file.
  lbuf[strlen(lbuf)-2] = '\0';
  logit(LOG_EPIC, stripansi(lbuf));
}

void banlog(int level, const char *format, ...)
{
  va_list  args;
  char     lbuf[MAX_STRING_LENGTH];
  P_desc   d;

  strcpy(lbuf, "&+y*&+Y*&N&+y*&+Y B&N&+yA&+YN&N&+y:&n ");
  va_start(args, format);
  vsprintf(lbuf + strlen(lbuf), format, args);
  strcat(lbuf, "\r\n");
  lbuf[sizeof(lbuf)] = 0;

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->connected == CON_PLYNG &&
        IS_TRUSTED(d->character) &&
        GET_LEVEL(d->character) >= level &&
        IS_SET(d->character->specials.act, PLR_BAN))
    {
      send_to_char(lbuf, d->character);
    }
  }
  va_end(args);
}

void sprintbit(ulong vektor, const char *names[], char *result)
{
  long     nr;

  *result = '\0';

  for (nr = 0; vektor; vektor >>= 1)
  {
    if (IS_SET(1, vektor))
      if (*names[nr] != '!')
      {
        if (*names[nr] != '\n')
        {
          strcat(result, names[nr]);
          strcat(result, " ");
        }
        else
        {
          strcat(result, "UNDEFINED ");
        }
      }
    if (*names[nr] != '\n')
      nr++;
/*
    if (nr != 0)
      vektor &= 0x7fffffff;
*/
  }

  if (!*result)
    strcat(result, "NOBITS");
}

void sprintbitde(ulong vektor, const flagDef names[], char *result)
{
  long     nr;

  *result = '\0';

  for (nr = 0; vektor; vektor >>= 1)
  {
    if (IS_SET(1, vektor))
      if (names[nr].flagShort != NULL)
      {
        strcat(result, names[nr].flagShort);
        strcat(result, " ");
      }
      else
      {
        strcat(result, "UNDEFINED ");
      }
    if (names[nr].flagShort != NULL)
      nr++;
  }

  if (!*result)
    strcat(result, "NOBITS");
}

void sprint64bit(ulong * vektor, const char *names[], char *result)
{
  ulong    nr, v;

  if (!vektor)
    return;

  sprintbit(*vektor, names, result);

  for (nr = 0; nr < 32; nr++)
    if (*names[nr] == '\n')
      return;

  if (!str_cmp(result, "NOBITS"))
    *result = '\0';

  v = vektor[1];
  for (nr = 32; v; v >>= 1)
  {
    if (IS_SET(1, v))
      if (*names[nr] != '!')
      {
        if (*names[nr] != '\n')
        {
          strcat(result, names[nr]);
          strcat(result, " ");
        }
        else
        {
          strcat(result, "UNDEFINED ");
        }
      }
    if (*names[nr] != '\n')
      nr++;

/*    if (nr > 32)
      v &= 0x7fffffff;*/
  }
  if (!*result)
    strcat(result, "NOBITS");
}

void sprinttype(int type, const char *names[], char *result)
{
  int      nr;

  for (nr = 0; (*names[nr] != '\n'); nr++) ;
  if (type < nr)
    strcpy(result, names[type]);
  else
    strcpy(result, "UNDEFINED");
}

struct time_info_data real_time_countdown(time_t t2, time_t t1, int max_sec)
{
  long secs;
  struct time_info_data now;

  secs = (long) max_sec - (t2 - t1);

  now.second = MAX(0, secs % 60);
  secs -= now.second;
  now.minute = MAX(0, (secs / 60) % 60);
  secs -= 60 * now.minute;
  now.hour = MAX(0, (secs / SECS_PER_REAL_HOUR) % 24);
  secs -= SECS_PER_REAL_HOUR * now.hour;
  now.day = MAX(0, (secs / SECS_PER_REAL_DAY));
  secs -= SECS_PER_REAL_DAY * now.day;
  now.month = -1;
  now.year = -1;
  return now;
}

/*
 * Calculate the REAL time passed over the last t2-t1 centuries (secs)
 */

struct time_info_data real_time_passed(time_t t2, time_t t1)
{
  long     secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.second = secs % 60;       /*
                                 * 0 - 59 seconds
                                 */
  secs -= now.second;

  now.minute = (secs / 60) % 60;        /*
                                         * 0 - 59 minutes
                                         */
  secs -= 60 * now.minute;

  now.hour = (secs / SECS_PER_REAL_HOUR) % 24;  /*
                                                 * 0 - 23 hours
                                                 */
  secs -= SECS_PER_REAL_HOUR * now.hour;

  now.day = (secs / SECS_PER_REAL_DAY); /*
                                         * 0+ days
                                         */
  secs -= SECS_PER_REAL_DAY * now.day;

  now.month = -1;
  now.year = -1;

  return now;
}

/*
 * Calculate the MUD time passed over the last t2-t1 centuries (secs)
 */

struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
  long     secs;
  static struct time_info_data now;

  secs = (long) (t2 - t1);

  now.second = secs % SECS_PER_MUD_HOUR;
  secs -= now.second;

  now.hour = (secs / SECS_PER_MUD_HOUR) % 24;   /*
                                                 * 0..23 hours
                                                 */
  secs -= SECS_PER_MUD_HOUR * now.hour;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;     /*
                                                 * 0..34 days
                                                 */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17; /*
                                                 * 0..16 months
                                                 */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);        /*
                                                 * 0..XX? years
                                                 */

  return now;
}

struct time_info_data age(P_char ch)
{
  struct time_info_data player_age;

//  player_age = mud_time_passed(time (0), time(0));
  player_age = mud_time_passed(time(0), ch->player.time.birth);
  /* natural
   * aging
   */
  player_age.year += 5;
/*  player_age.year += ch->player.time.perm_aging;        *
                                                         * permanent
                                                         * 'unnatural' aging
                                                         */
  player_age.year += ch->player.time.age_mod;   /*
                                                 * temporary 'unnatural' aging
                                                 */
  player_age.year = MAX(0, player_age.year);    /*
                                                 * since I don't want to deal
                                                 * with 'infants'
                                                 */
  player_age.year += racial_data[(int) GET_RACE(ch)].base_age;

  return player_age;
}

int exist_in_equipment(P_char ch, int bitflag)
{
  register int i;

  for (i = 0; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
      if (IS_SET(ch->equipment[i]->extra_flags, bitflag))
        return TRUE;
  }
  return FALSE;
}

/*
 * Functions added by AC
 */

#ifndef USE_MACRO

/* Determine visibility of a person */

/*
 * This function is called about a billion times a minute (ok, so I exaggerated a little), don't even
 * THINK about doing anything to it that would make it slower, even a LITTLE bit.  JAB
 */
bool ac_can_see(P_char sub, P_char obj, bool check_z)
{
  bool   globe, flame, dayblind;
  int    sroom, oroom, race;
  P_char tmp_char;

  if( !sub )
  {
    logit(LOG_EXIT, "No P_char sub found during ac_can_see() call");
    raise(SIGSEGV);
  }

  // No idea what happened, but let's hack this until we figure it out.
  if( sub->in_room == NOWHERE )
    return FALSE;

  /* minor detail, sleeping chars can't see squat! */
  if( !IS_AWAKE(sub) )
    return FALSE;

  // Determine visibility by "vis" command
  if( WIZ_INVIS(sub, obj) )
    return FALSE;

  // This shouldn't make too much of a difference since not many lvl 57+ mobs/chars.
  // Had to add these checks for toggle fog?  Also, NPCs 57+ see all?
  // Most common fail: NPC, second: mortal, third: fog set.
  if( IS_TRUSTED(sub) )
  {
      return TRUE;
  }

  /* Flyers */
/* Just commented this out.  Should be at least as fast if not faster than if( 0 ||...
  if( check_z && (abs(sub->specials.z_cord - obj->specials.z_cord) > 2)
    && !IS_AFFECTED4(sub, AFF4_HAWKVISION) )
  {
    if (sub->specials.z_cord != obj->specials.z_cord)
      return FALSE;
  }
*/

  /* Object is invisible and subject does not have detect invis */
  // Only check for CTF tag if CTF is active.
  if( (IS_AFFECTED(obj, AFF_INVISIBLE) || IS_AFFECTED2(obj, AFF2_MINOR_INVIS)
    || IS_AFFECTED3(obj, AFF3_ECTOPLASMIC_FORM))
#if defined(CTF_MUD) && (CTF_MUD == 1)
 && !affected_by_spell(obj, TAG_CTF)
#endif
     )
  {
    /* if the fellow is non-detectable you ain't gonna see jack */
    if(IS_AFFECTED3(obj, AFF3_NON_DETECTION))
    {
      return FALSE;
    }

    if( !IS_AFFECTED(sub, AFF_DETECT_INVISIBLE)
      && !(IS_NPC(obj) && (obj->following == sub) && IS_AFFECTED4(sub, AFF4_SENSE_FOLLOWER)) )
    {
      return FALSE;
    }

/* NPCs can't be non-detected?  why? */
/*
    if ((!IS_AFFECTED(sub, AFF_DETECT_INVISIBLE)) ||
        (!IS_NPC(sub) &&
          IS_AFFECTED (sub, AFF_DETECT_INVISIBLE)
          && IS_AFFECTED3(obj, AFF3_NON_DETECTION)))
          return FALSE;
*/
  }                             /* end sub invis */

  /* Subject is blinded */
  if( IS_BLIND(sub) )
  {
    return FALSE;
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
  if( IS_AFFECTED(obj, AFF_HIDE) && !affected_by_spell(obj, TAG_CTF) )
#else
  if( IS_AFFECTED(obj, AFF_HIDE) )
#endif
  {
    return FALSE;
  }

  // As wraithform is kind of kludge, semi-godsight.
  if( IS_AFFECTED(sub, AFF_WRAITHFORM) || IS_AFFECTED4(sub, AFF4_VAMPIRE_FORM) )
  {
      return TRUE;
  }

  // As wraithform is kind of kludge, it is shown nowhere.
  if( IS_AFFECTED(obj, AFF_WRAITHFORM) )
  {
    return FALSE;
  }

  // NPCs don't suffer day/night blindness.
  if( IS_NPC(sub) )
  {
    return TRUE;
  }

  // First, we need to see if the subject can see anything or is dayblind / nightblind in the room their in.
  dayblind = IS_DAYBLIND(sub);
  sroom = sub->in_room;
  if( !IS_MAP_ROOM(sroom) )
  {
    // Check for dayblind: Light room with dayblind subject.
    if( dayblind && !CAN_NIGHTPEOPLE_SEE(sroom) )
    {
      globe = FALSE;
      // Anyone with globe of dark overrides room to twilight.
      for( tmp_char = world[sroom].people; tmp_char; tmp_char = tmp_char->next_in_room )
      {
        if( IS_AFFECTED4(tmp_char, AFF4_GLOBE_OF_DARKNESS) )
        {
          globe = TRUE;
          break;
        }
      }
      // If no globe of darkness in room, they are dayblind.
      if( !globe )
      {
        // Here we check sub's infra vs obj's race (some races are invis to infravision).
        if( IS_AFFECTED(sub, AFF_INFRAVISION) )
        {
          // We use GET_RACE2 becaue of shape shifters.
          race = GET_RACE2(obj);
          if( INFRA_INVIS_RACE(race) )
          {
            return FALSE;
          }
        }
        else
        {
          return FALSE;
        }
      }
    }
    // Else check for nightblind: Dark room with no ultra subject.
    else if( !IS_AFFECTED2(sub, AFF2_ULTRAVISION) && !CAN_DAYPEOPLE_SEE(sroom) )
    {
      flame = FALSE;
      // Anyone with mage flame overrides room to twilight.
      for( tmp_char = world[sroom].people; tmp_char; tmp_char = tmp_char->next_in_room )
      {
        if( IS_AFFECTED4(tmp_char, AFF4_MAGE_FLAME) )
        {
          flame = TRUE;
          break;
        }
      }
      // If no mage flame in room, they are night blind.
      if( !flame )
      {
        // Here we check sub's infra vs obj's race (some races are invis to infravision).
        if( IS_AFFECTED(sub, AFF_INFRAVISION) )
        {
          // We use GET_RACE2 becaue of shape shifters.
          race = GET_RACE2(obj);
          if( INFRA_INVIS_RACE(race) )
          {
            return FALSE;
          }
        }
        else
        {
          return FALSE;
        }
      }
    }
  }

  // Now, we need to look at the room obj is in too see if sub can see anything in it.
  oroom = obj->in_room;
  if( !IS_MAP_ROOM(oroom) && oroom >= 0 )
  {
    // Check for dayblind: Light room with dayblind subject.
    if( dayblind && !CAN_NIGHTPEOPLE_SEE(oroom) )
    {
      globe = FALSE;
      // Anyone with globe of dark overrides room to twilight.
      for( tmp_char = world[oroom].people; tmp_char; tmp_char = tmp_char->next_in_room )
      {
        if( IS_AFFECTED4(tmp_char, AFF4_GLOBE_OF_DARKNESS) )
        {
          globe = TRUE;
          break;
        }
      }
      // If no globe of darkness in room, then sub can not see in obj's room.
      if( !globe )
      {
        if( IS_AFFECTED(sub, AFF_INFRAVISION) )
        {
          // We use GET_RACE2 becaue of shape shifters.
          race = GET_RACE2(obj);
          if( INFRA_INVIS_RACE(race) )
          {
            return FALSE;
          }
        }
        else
        {
          return FALSE;
        }
      }
    }
    // Else check for nightblind: Dark room with no infra/ultra subject.
    else if( !IS_AFFECTED(sub, AFF_INFRAVISION) && !IS_AFFECTED2(sub, AFF2_ULTRAVISION) && !CAN_DAYPEOPLE_SEE(oroom) )
    {
      flame = FALSE;
      // Anyone with mage flame overrides room to twilight.
      for( tmp_char = world[sroom].people; tmp_char; tmp_char = tmp_char->next_in_room )
      {
        if( IS_AFFECTED4(tmp_char, AFF4_MAGE_FLAME) )
        {
          flame = TRUE;
          break;
        }
      }
      // If no mage flame in room, then sub can not see in obj's room.
      if( !flame )
      {
        if( IS_AFFECTED(sub, AFF_INFRAVISION) )
        {
          // We use GET_RACE2 becaue of shape shifters.
          race = GET_RACE2(obj);
          if( INFRA_INVIS_RACE(race) )
          {
            return FALSE;
          }
        }
        else
        {
          return FALSE;
        }
      }
    }
  }

  return TRUE;
}

/*
 * ** Determine visibility of an object
 */

bool ac_can_see_obj(P_char sub, P_obj obj, int zrange )
{
  int      rroom, zcord;
  P_char   tmp_char;
  int      vis_mode;

  /* wraiths can't see any objects */
  if( IS_AFFECTED(sub, AFF_WRAITHFORM) )
    return FALSE;

  /* sub is flying, obj isn't */
  if( OBJ_ROOM(obj) && sub->specials.z_cord != obj->z_cord )
  {
    zcord = sub->specials.z_cord - obj->z_cord;
    if( zcord < 0 )
    {
      zcord *= -1;
    }
    // Ships show above/below, other objects only show if within zrange.
    if( obj_index[obj->R_num].func.obj != ship_obj_proc && zcord > zrange )
    {
      return FALSE;
    }
  }

/*
  if (OBJ_NOWHERE(obj)) {
    debug("OBJ_NOWHERE\r\n");
    return FALSE;
  }
*/
  /* Immortal can see anything */
  // level 58 and higher (no avatars).
  if( IS_TRUSTED(sub) && GET_LEVEL(sub) >= IMMORTAL )
  {
    return TRUE;
  }

  /* minor detail, sleeping chars can't see squat! */
  if( !IS_AWAKE(sub) )
    return FALSE;

  if( IS_NOSHOW(obj) )
    return FALSE;

  /* Check to see if object is invis */
  if( IS_SET(obj->extra_flags, ITEM_INVISIBLE) && !IS_AFFECTED(sub, AFF_DETECT_INVISIBLE) )
    return FALSE;

  /* Check if subject is blind */
  if( IS_BLIND(sub) )
    return FALSE;

  if( IS_SET((obj)->extra_flags, ITEM_SECRET) )
    return FALSE;

  if( IS_SET((obj)->extra_flags, ITEM_BURIED) )
    return FALSE;

  /* Room is magically dark
     Done Later - Granor */
  /*if (IS_SET (world[obj->loc.room].room_flags, MAGIC_DARK) && IS_PC(sub)
     && !IS_AFFECTED2(sub, AFF2_ULTRAVISION) &&
     !IS_TWILIGHT_ROOM(obj->loc.room))
     return 0;
   */

  while( OBJ_INSIDE(obj) )
    obj = obj->loc.inside;

  if( OBJ_ROOM(obj) )
  {
    rroom = obj->loc.room;
  }
  else if( OBJ_WORN(obj) )
  {
    rroom = obj->loc.wearing->in_room;
    // You know the eq your wearing regardless.
    if( OBJ_WORN_BY(obj, sub) )
    {
      return TRUE;
    }
  }
  else if( OBJ_CARRIED(obj) )
  {
    rroom = obj->loc.carrying->in_room;
    // You know what you have in inventory regardless.
    if( OBJ_CARRIED_BY(obj, sub) )
    {
      return TRUE;
    }
  }
  // Anti-crash hack?
  else if( OBJ_NOWHERE(obj) )
  {
    rroom = sub->in_room;
  }
  else
  {
    return IS_TRUSTED(sub) ? TRUE : FALSE;
  }

  if( IS_NPC(sub) )
    return TRUE;

  vis_mode = get_vis_mode(sub, rroom);
  // vis 1 == god, 2 == normal, 3 == infra, 4 == wraith, 5 == too dark, 6 == too bright.

  // Ships are big enough to be visible regardless of light.
  if( vis_mode != 4 && (obj_index[obj->R_num].func.obj == ship_obj_proc) )
  {
    return TRUE;
  }

  // Allowing infravision to see items in room.  They can already see inventory/equipped.
  return (vis_mode == 1 || vis_mode == 2 || vis_mode == 3);
}

#endif


bool can_char_multi_to_class(P_char ch,  int m_class)
{
  int i = 0;

  while (allowed_secondary_classes[flag2idx(ch->player.m_class)][i] != -1)
  {
    if (flag2idx(allowed_secondary_classes[flag2idx(ch->player.m_class)][i]) == m_class)
    {
      if ((class_table[(int)GET_RACE(ch)] [m_class ]) != 5)
      {
        return TRUE;
      }
    }
    i++;
  }

  return FALSE;
}


int coin_type(char *s)
{
  if( !*s )
  {
    return -1;
  }
  if( is_abbrev(s, "copper") )
    return 0;
  if( is_abbrev(s, "silver") )
    return 1;
  if( is_abbrev(s, "gold") )
    return 2;
  if( is_abbrev(s, "platinum") )
    return 3;
  return -1;
}

int ScaleAreaDamage(P_char ch, int orig_dam)
{
  P_char   tch;
  int      count = 0;

  if (IS_PC(ch))
    return (int) (orig_dam * 0.15);
  return orig_dam;              // disabled for now

  if (!ch || IS_NPC(ch))
    return orig_dam;
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (IS_PC(tch) && should_area_hit(ch, tch))
      count++;
  }
  if (count < 5)
    return orig_dam * 14 / 15;
  if (count < 9)
    return orig_dam * 13 / 15;
  if (count < 13)
    return orig_dam * 12 / 15;
  if (count < 17)
    return orig_dam * 11 / 15;
  return orig_dam * 10 / 15;
}

// Takes an amount and converts it to a string containing the corresponding coinage.
// If padfront > 0, then we add padfront spaces to the string minus the number of digits
//   of the first non-zero coin type. (So, padfront 5 on 5 plat -> "    5 plat").
char *coin_stringv( int amount, int padfront )
{
  int p, g, s, c;
  static char buf[300], *spot;

  if( padfront < 0 )
    padfront = 0;

  if( !amount )
  {
    // Works a little backwards, but add the '\0', then spaces from it down to and including buf[0].
    buf[padfront] = '\0';
    while( padfront > 0 )
    {
      buf[--padfront] = ' ';
    }
    return buf;
  }

  buf[0] = '\0';
  spot = buf;
  if( amount < 0 )
  {
    amount *= -1;
    *(spot++) = '-';
    if( padfront )
      padfront--;
  }

  p = amount / 1000;
  amount %= 1000;
  g = amount / 100;
  amount %= 100;
  s = amount / 10;
  c = amount % 10;

  if( padfront )
  {
    amount = (p > 0) ? p : ((g > 0) ? g : ((s > 0) ? s : c));
    while( amount > 0 && padfront > 0 )
    {
      amount /= 10;
      padfront--;
    }
    while( padfront-- > 0 )
    {
      *(spot++) = ' ';
    }
  }

  if( p )
  {
    amount = sprintf(spot, "%d &+Wplatinum&N", p);
    spot += amount;
  }
  if( g )
  {
    if( p && !s && !c )
    {
      sprintf(spot, ", and ");
      spot += 6;
    }
    else if( p )
    {
      sprintf(spot, ", ");
      spot += 2;
    }
    amount = sprintf(spot, "%d &+Ygold&N", g);
    spot += amount;
  }
  if( s )
  {
    if( (p || g) && !c )
    {
      sprintf(spot, ", and ");
      spot += 6;
    }
    else if( p || g )
    {
      sprintf(spot, ", ");
      spot += 2;
    }
    amount = sprintf(spot, "%d silver", s);
    spot += amount;
  }
  if( c )
  {
    if( p || g || s )
    {
      sprintf(spot, ", and ");
      spot += 6;
    }
    sprintf(spot, "%d &+ycopper&N", c);
  }
  return buf;
}

/*
 * adds flat amount (in copper), to ch, using smallest number of coins
 */

void ADD_MONEY(P_char ch, int amount)
{
  int      t = 0;
  int      t2 = 0;

  if (amount < 0)
  {
    logit(LOG_EXIT, "ADD_MONEY: negative amount");
    raise(SIGSEGV);
  }

  if (amount == 0)
    return;

  /* plat is a bit rarer, and thus is returned as change less often */
  if (amount > 999)
  {
    t = amount / 1000;
    t2 = number(0, t / 2);
    GET_PLATINUM(ch) += t;
    amount -= t * 1000;
  }
  if (amount > 99)
  {
    t = amount / 100;
    t2 = number(0, t);
    GET_GOLD(ch) += t;
    amount -= t * 100;
  }
  if (amount > 9)
  {
    t = amount / 10;
    GET_SILVER(ch) += t;
    amount -= t * 10;
  }
  if (amount)
    GET_COPPER(ch) += amount;
}

/* TOWARDS BANK MONEY
 * mode 0 = make change, return amount of change made
 * mode 1 = subtract extra, return extra amount subtracted
 * mode 2 = subtract as close as possible without going over, return amount short
 * modes 1 & 2 not supported yet
 */
int SUB_BALANCE(P_char ch, int amount, int mode)
{
  int      t = 0;

  if (amount <= 0)
    return -1;
  if (amount > GET_BALANCE(ch))
    return -1;
  if (mode != 0)
    return -1;

  if (amount > GET_BALANCE_COPPER(ch))
  {
    amount -= GET_BALANCE_COPPER(ch);
    GET_BALANCE_COPPER(ch) = 0;
  }
  else
  {
    GET_BALANCE_COPPER(ch) -= amount;
    return 0;
  }

  if (amount > 0)
  {
    t = GET_BALANCE_SILVER(ch) * 10;
    if (amount >= t)
    {
      amount -= t;
      GET_BALANCE_SILVER(ch) = 0;
    }
    else
    {
      t = (int) (amount / 10) + 1;
      GET_BALANCE_SILVER(ch) -= t;
      amount -= t * 10;
    }
  }
  if (amount > 0)
  {
    t = GET_BALANCE_GOLD(ch) * 100;
    if (amount >= t)
    {
      amount -= t;
      GET_BALANCE_GOLD(ch) = 0;
    }
    else
    {
      t = (int) (amount / 100) + 1;
      GET_BALANCE_GOLD(ch) -= t;
      amount -= t * 100;
    }
  }
  if (amount > 0)
  {
    t = GET_BALANCE_PLATINUM(ch) * 1000;
    if (amount >= t)
    {
      amount -= t;
      GET_BALANCE_PLATINUM(ch) = 0;
    }
    else
    {
      t = (int) (amount / 1000) + 1;
      GET_BALANCE_PLATINUM(ch) -= t;
      amount -= t * 1000;
    }
  }
  if (amount < 0)
    ADD_MONEY(ch, -(amount));

  return 0;
}

int SUB_MONEY(P_char ch, int amount, int mode)
{
  int      t = 0;

  if (amount <= 0)
    return -1;
  if (amount > GET_MONEY(ch))
    return -1;
  if (mode != 0)
    return -1;

  if (amount > GET_COPPER(ch))
  {
    amount -= GET_COPPER(ch);
    GET_COPPER(ch) = 0;
  }
  else
  {
    GET_COPPER(ch) -= amount;
    return 0;
  }

  if (amount > 0)
  {
    t = GET_SILVER(ch) * 10;
    if (amount >= t)
    {
      amount -= t;
      GET_SILVER(ch) = 0;
    }
    else
    {
      t = (int) (amount / 10) + 1;
      GET_SILVER(ch) -= t;
      amount -= t * 10;
    }
  }
  if (amount > 0)
  {
    t = GET_GOLD(ch) * 100;
    if (amount >= t)
    {
      amount -= t;
      GET_GOLD(ch) = 0;
    }
    else
    {
      t = (int) (amount / 100) + 1;
      GET_GOLD(ch) -= t;
      amount -= t * 100;
    }
  }
  if (amount > 0)
  {
    t = GET_PLATINUM(ch) * 1000;
    if (amount >= t)
    {
      amount -= t;
      GET_PLATINUM(ch) = 0;
    }
    else
    {
      t = (int) (amount / 1000) + 1;
      GET_PLATINUM(ch) -= t;
      amount -= t * 1000;
    }
  }
  if (amount < 0)
    ADD_MONEY(ch, -(amount));

  return 0;
}

void strToLower(char *s1)
{
  int      i, len;

  len = strlen(s1);
  for (i = 0; i < len; i++)
  {
    if (s1[i] >= 'A' && s1[i] <= 'Z')
      s1[i] += 'a' - 'A';
  }
}                               /*
                                 * strToLower
                                 */

/*
 * I'm such a genius!  Talk about your redundant code!  Does basic sanity
 * checks on a char_data struct, logs it and returns FALSE if there is a
 * problem, else returns TRUE.  I really hate to think about how many places
 * this can be (or should be) used. -JAB
 */

bool SanityCheck(P_char ch, const char *calling)
{
  // Rules here: If no ch, "Null -1", If ch has no only data "<name> -1"
  if( !IS_ALIVE(ch) )
  {
    if( !ch )
      logit( LOG_DEBUG, "%s: ch does not exist!", calling );
    else
      logit( LOG_DEBUG, "%s: ch is not alive: %s %d.", calling, GET_NAME(ch),
        (ch->only.npc != NULL) ? (IS_NPC(ch) ? ch->only.npc->idnum : ch->only.pc->pid) : -1 );
    return FALSE;
  }

  if (ch->in_room && ch->in_room == NOWHERE)
  {
    if (ch->specials.was_in_room == NOWHERE &&
       (IS_NPC(ch) && GET_VNUM(ch) != IMAGE_REFLECTION_VNUM))
    {
      logit(LOG_EXIT, "%s in NOWHERE in call to SanityCheck from %s().",
            GET_NAME(ch), calling);
      raise(SIGSEGV);
    }
    else
    {
      debug( "SanityCheck called from %s() for %s at NOWHERE! Original room: %d",
             calling, GET_NAME(ch), ch->specials.was_in_room);
      logit(LOG_DEBUG,
            "SanityCheck called from %s() for %s at NOWHERE! Original room: %d",
            calling, GET_NAME(ch), ch->specials.was_in_room);
      if (GET_STAT(ch) != STAT_DEAD)
      {
        char_from_room(ch);
        char_to_room(ch, real_room(ch->specials.was_in_room), -2);
      }
    }
    return FALSE;
  }
  return TRUE;
}

/*
 * another stroke of genius (damn I'm modest).  Goes SanityCheck one better,
 * this check for validity of both ch and vic, and also checks to see that
 * ch is fighting, and that ch and vic are in the same room.  Returns FALSE
 * if any check fails, TRUE if everything is fine.
 */

bool FightingCheck(P_char ch, P_char vic, const char *calling)
{
  if (!SanityCheck(ch, calling))
    return FALSE;

  if (!SanityCheck(vic, calling))
    return FALSE;

  if (!IS_FIGHTING(ch))
  {
    logit(LOG_DEBUG, "%s not fighting %s in call to %s().",
          GET_NAME(ch), GET_NAME(vic), calling);
    return FALSE;
  }
  if (ch->in_room != vic->in_room)
  {
    stop_fighting(ch);
    stop_fighting(vic);
/*    logit (LOG_DEBUG,
           "%s [%d] and %s [%d] not in same room while fighting in %s().",
           GET_NAME (ch), world[ch->in_room].number,
           GET_NAME (vic), world[vic->in_room].number, calling);
*/
    return FALSE;
  }
  return TRUE;
}

/*
   This function sweeps the char list and returns the number of
   chars that are 1. of the class(param) 2. >= to min_level(param)
   3. in same room and 4. are consented to leader
*/





int get_multicast_chars(P_char leader, int m_class, int min_level)
{
  P_char   i;
  int      count = 1;           // 1 for the leader

//  for(i = character_list; i; i=i->next) {
  LOOP_THRU_PEOPLE(i, leader)
  {
    if (i != leader &&
        !is_linked_to(leader, i, LNK_CONSENT) &&
        GET_CLASS(i, m_class) &&
        GET_LEVEL(i) >= min_level && i->in_room == leader->in_room)
      count++;
  }
  return count;
}

/*
 * calcs number of moves needed to move from current room to room in direction
 * 'dir', returns -1 on error, else 0+ for moves needed.  -JAB
 */

int move_cost(P_char ch, int dir)
{
  P_char   mount;
  int      moves, a, b;

  if ((dir < 0) || (dir > (NUM_EXITS - 1)))
    return -1;

  a = movement_loss[(int) world[ch->in_room].sector_type];
  b = movement_loss[(int) world[world[ch->in_room].dir_option[dir]->to_room].sector_type];

  moves = a + b;
  moves = (load_modifier(ch) * moves) / 200;

  if(dir == DOWN)
    moves = (moves / 3) * 2;    /* slightly less going down */
  if(dir == UP)
    moves = (moves * 3) / 2;    /* slightly more going up */

  if(ch->specials.z_cord > 0 ||
    (IS_NPC(ch) && IS_SET(ch->specials.act, ACT_MOUNT)))
        moves = MAX(4, moves / 3);  /* Fly removes bunches of the cost */

  if(IS_AFFECTED(ch, AFF_FLY) ||      /* Fly/lev up/down costs very little */
    (IS_AFFECTED(ch, AFF_LEVITATE) && ((dir == UP) || (dir == DOWN))))
  {
    if(world[ch->in_room].sector_type == SECT_MOUNTAIN ||
        world[ch->in_room].sector_type == SECT_OCEAN)
      ;
    else if(world[ch->in_room].sector_type == SECT_INSIDE ||
       world[ch->in_room].sector_type == SECT_CITY ||
       world[ch->in_room].sector_type == SECT_ROAD)
          moves = 1;
    else
      moves = 2;
  }
  
  if((world[world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_OCEAN) &&
     is_ice(ch, ch->in_room))
        moves = moves >> 2;

  if(IS_TLEGLOCK(ch))
    moves += 10;

  return MAX(1, moves);
}

/*
 * little routine to count chars in room that are attacking 'ch' -JAB
 */

int NumAttackers(P_char ch)
{
  P_char   tch;
  int      total = 0;

  if (!SanityCheck(ch, "NumAttackers"))
    return -1;

  if (ALONE(ch))
    return 0;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    if (tch->specials.fighting && (tch->specials.fighting == ch))
      total++;

  return total;
}

bool aggressive_to_race(P_char ch, P_char target)
{
    if (IS_AGGROFLAG(ch, AGGR_HUMAN) && (GET_RACE1(target) == RACE_HUMAN))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_BARBARIAN) &&
        (GET_RACE1(target) == RACE_BARBARIAN))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_DROW_ELF) && (GET_RACE1(target) == RACE_DROW))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_GREY_ELF) && (GET_RACE1(target) == RACE_GREY))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_MOUNT_DWARF) && (GET_RACE1(target) == RACE_MOUNTAIN))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_DUERGAR) && (GET_RACE1(target) == RACE_DUERGAR))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_HALFLING) && (GET_RACE1(target) == RACE_HALFLING))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_GNOME) && (GET_RACE1(target) == RACE_GNOME))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_OGRE) && (GET_RACE1(target) == RACE_OGRE))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_TROLL) && (GET_RACE1(target) == RACE_TROLL))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_HALF_ELF) && (GET_RACE1(target) == RACE_HALFELF))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_ILLITHID) && (GET_RACE1(target) == RACE_ILLITHID))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_ORC) && (GET_RACE1(target) == RACE_ORC))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_THRIKREEN) && (GET_RACE1(target) == RACE_THRIKREEN))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_CENTAUR) && (GET_RACE1(target) == RACE_CENTAUR))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_GITHYANKI) && (GET_RACE1(target) == RACE_GITHYANKI))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_MINOTAUR) && (GET_RACE1(target) == RACE_MINOTAUR))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_GOBLIN) && (GET_RACE1(target) == RACE_GOBLIN))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_SGIANT) && (GET_RACE1(target) == RACE_SGIANT))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_WIGHT) && (GET_RACE1(target) == RACE_WIGHT))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_PHANTOM) && (GET_RACE1(target) == RACE_PHANTOM))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_PLICH) && (GET_RACE1(target) == RACE_PLICH))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_PVAMPIRE) && (GET_RACE1(target) == RACE_PVAMPIRE))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_PDKNIGHT) && (GET_RACE1(target) == RACE_PDKNIGHT))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_SHADE) && (GET_RACE1(target) == RACE_SHADE))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_REVENANT) && (GET_RACE1(target) == RACE_REVENANT))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_GITHZERAI) && (GET_RACE1(target) == RACE_GITHZERAI))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_OROG) && (GET_RACE1(target) == RACE_OROG))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_DRIDER) && (GET_RACE1(target) == RACE_DRIDER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_KOBOLD) && (GET_RACE1(target) == RACE_KOBOLD))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_KUOTOA) && (GET_RACE1(target) == RACE_KUOTOA))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_WOODELF) && (GET_RACE1(target) == RACE_WOODELF))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_FIRBOLG) && (GET_RACE1(target) == RACE_FIRBOLG))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_AGATHINON) && (GET_RACE1(target) == RACE_AGATHINON))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_ELADRIN) && (GET_RACE1(target) == RACE_ELADRIN))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_PILLITHID) && (GET_RACE1(target) == RACE_PILLITHID))
      return TRUE;

    return FALSE;
}


bool aggressive_to_class(P_char ch, P_char target)
{
    if (IS_AGGRO2FLAG(ch, AGGR2_WARRIOR) && GET_CLASS1(target, CLASS_WARRIOR))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_RANGER) && GET_CLASS1(target, CLASS_RANGER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_PSIONICIST) && GET_CLASS1(target, CLASS_PSIONICIST))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_PALADIN) && GET_CLASS1(target, CLASS_PALADIN))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_ANTIPALADIN) && GET_CLASS1(target, CLASS_ANTIPALADIN))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_CLERIC) && GET_CLASS1(target, CLASS_CLERIC))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_MONK) && GET_CLASS1(target, CLASS_MONK))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_DRUID) && GET_CLASS1(target, CLASS_DRUID))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_SHAMAN) && GET_CLASS1(target, CLASS_SHAMAN))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_SORCERER) && GET_CLASS1(target, CLASS_SORCERER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_NECROMANCER) && GET_CLASS1(target, CLASS_NECROMANCER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_CONJURER) && GET_CLASS1(target, CLASS_CONJURER))
      return TRUE;

    // Summoners are almost identical to conjs. retrofitting..
    if (IS_AGGRO2FLAG(ch, AGGR2_CONJURER) && GET_CLASS1(target, CLASS_SUMMONER))
      return TRUE;

    if (IS_AGGRO3FLAG(ch, AGGR3_SUMMONER) && GET_CLASS1(target, CLASS_SUMMONER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_ROGUE) && GET_CLASS1(target, CLASS_ROGUE))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_ASSASSIN) && GET_SPEC(target, CLASS_ROGUE, SPEC_ASSASSIN))
      return TRUE;
    
    if (IS_AGGRO2FLAG(ch, AGGR2_MERCENARY) && GET_CLASS1(target, CLASS_MERCENARY))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_BARD) && GET_CLASS1(target, CLASS_BARD))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_THIEF) && GET_CLASS1(target, CLASS_THIEF))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_WARLOCK) && GET_CLASS1(target, CLASS_WARLOCK))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_MINDFLAYER) && GET_CLASS1(target, CLASS_MINDFLAYER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_THEURGIST) && GET_CLASS1(target, CLASS_THEURGIST))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_ALCHEMIST) && GET_CLASS1(target, CLASS_ALCHEMIST))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_BERSERKER) && GET_CLASS1(target, CLASS_BERSERKER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_REAVER) && GET_CLASS1(target, CLASS_REAVER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_ILLUSIONIST) && GET_CLASS1(target, CLASS_ILLUSIONIST))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_ETHERMANCER) && GET_CLASS1(target, CLASS_ETHERMANCER))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_DREADLORD) && GET_CLASS1(target, CLASS_DREADLORD))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR3_AVENGER) && GET_CLASS1(target, CLASS_AVENGER))
      return TRUE;

    return FALSE;
}

bool aggressive_to(P_char ch, P_char target)
{
  int      tmp_race = 0;
  register int chance;

  if (!ch || !target || ch == target)
    return FALSE;

  // 99.9% calls will leave here
  if (IS_NPC(ch) && IS_NPC(target) &&
      (!target->following || IS_NPC(target->following)) && !IS_MORPH(target))
    return FALSE;

  if (IS_TRUSTED(target) && IS_SET(target->specials.act, PLR_AGGIMMUNE))
    return FALSE;

  if (TRUSTED_NPC(ch))
    return FALSE;

  if (IS_IMMOBILE(ch))
    return FALSE;

  if ((world[ch->in_room].room_flags & SINGLE_FILE) &&
      !AdjacentInRoom(ch, target))
    return FALSE;

  /* now needs a master to inhibit aggression */
  if (GET_MASTER(ch) && ch->in_room == GET_MASTER(ch)->in_room)
  {
    /* master is in room, lets check to see if he's fighting */
    if (GET_MASTER(ch)->specials.fighting == target)
      return TRUE;              /* master is fighting the target, so return true */
    return FALSE;
  }

  /* now different checks for pcs/npcs */
  if (IS_NPC(ch))
  {
    if (GET_MASTER(target) && GET_MASTER(target) == GET_MASTER(ch))
      return FALSE;

    if (GET_MASTER(target) == ch)
      return FALSE;

    /* rangers and druids might not get attacked by aggro animals */

    if (IS_AGGRESSIVE(ch) && IS_ANIMAL(ch) &&
        (GET_CLASS(target, CLASS_DRUID) || GET_CLASS(target, CLASS_RANGER)))
    {
      chance = GET_LEVEL(target) - GET_LEVEL(ch);
      if (chance > 0)
        chance = BOUNDED(0, chance * 3, 100);
      else
        chance = chance + 5 + number(1, 10);
      if (number(1, 101) < chance)
        return FALSE;
    }

    if ((GET_LEVEL(ch) <= (GET_LEVEL(target) - 10)) && \
        has_innate(target, INNATE_UNDEAD_FEALTY) && \
	IS_UNDEADRACE(ch) && \
	!affected_by_spell(ch, TAG_CTF) && \
        !CheckFor_remember(ch, target))
      return FALSE;  // Liches are revered/feared by lower level undead - Jexni 8/18/08

    /* check if mob is nocturnal/diurnal/perpetually angry */
    // During twilight times (4am to 8am and 4pm to 7pm) we want them to be aggro 'cause they can see.
    //   This is because we are the land of Bloodlust! Rawr!
    if (IS_AGGROFLAG(ch, AGGR_NIGHT_ONLY) && !IS_NIGHT)
      return FALSE;
    if (IS_AGGROFLAG(ch, AGGR_DAY_ONLY) && !IS_DAY)
      return FALSE;

    if (has_innate(target, INNATE_ASTRAL_NATIVE) && 
        GET_LEVEL(target) >= GET_LEVEL(ch) &&
        world[target->in_room].number >= ASTRAL_VNUM_BEGIN &&
        world[target->in_room].number <= ASTRAL_VNUM_END)
      return FALSE;

    /* aggro to all?  how nice! */
    if (IS_AGGROFLAG(ch, AGGR_ALL))
      return TRUE;

    /* check alignment */

    if ((IS_EVIL(target) && IS_AGGROFLAG(ch, AGGR_EVIL_ALIGN)) ||
        (IS_GOOD(target) && IS_AGGROFLAG(ch, AGGR_GOOD_ALIGN)) ||
        (IS_NEUTRAL(target) && IS_AGGROFLAG(ch, AGGR_NEUTRAL_ALIGN)))
      return TRUE;

    if (IS_AGGROFLAG(ch, AGGR_GOOD_RACE) &&
        (GET_RACE(target) != GET_RACE(ch)) &&
        (!IS_DISGUISE_NPC(target) || IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION)) &&  // undetected NPC disguise makes immune
        (((!IS_DISGUISE_PC(target) || IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION)) ?
          target->player.racewar : target->disguise.racewar) == RACEWAR_GOOD
         ))
    {
      return TRUE;
    }
    /*
     if (IS_AGGROFLAG(ch, AGGR_GOOD_RACE) &&
         (GET_RACE(target) != GET_RACE(ch)) && OLD_RACE_PUNDEAD(ch))
           {
             return (TRUE);
           }
           */
    
    if (IS_AGGROFLAG(ch, AGGR_EVIL_RACE) &&
        (GET_RACE(target) != GET_RACE(ch)) &&
        (!IS_DISGUISE_NPC(target) || IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION)) &&  // undetected NPC disguise makes immune
        (((!IS_DISGUISE_PC(target) || IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION)) ?
          target->player.racewar : target->disguise.racewar) == RACEWAR_EVIL
         ))
    {
      return (TRUE);
    }
   
    /*
    if (IS_AGGROFLAG(ch, AGGR_EVIL_RACE) &&
    (GET_RACE(target) != GET_RACE(ch)) && OLD_RACE_PUNDEAD(ch))
    {
      return (TRUE);
    }*/
   
    
    if (IS_AGGROFLAG(ch, AGGR_UNDEAD_RACE) &&
        (GET_RACE(target) != GET_RACE(ch)) &&
        (!IS_DISGUISE_NPC(target) || IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION)) &&  // undetected NPC disguise makes immune
        ((!IS_DISGUISE_PC(target) || IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION)) ?
         target->player.racewar : target->disguise.racewar) == RACEWAR_UNDEAD)
    {
      return TRUE;
    }

    /* check follower flags */

    /* if following, must be following PC due to check above */

    if (target->following)
    {
      if (IS_AGGROFLAG(ch, AGGR_FOLLOWERS))
        return TRUE;

      if (IS_AGGROFLAG(ch, AGGR_ELEMENTALS) && IS_ELEMENTAL(target))
        return TRUE;

      if (IS_AGGROFLAG(ch, AGGR_UNDEAD_FOL) &&
          IS_UNDEAD(target) && IS_NPC(target))
        return TRUE;

      if (IS_AGGROFLAG(ch, AGGR_DRACOLICH) && IS_DRACOLICH(target))
        return TRUE;
    }

    /* try to kill invaders on sight  */

    if (IS_AGGROFLAG(ch, AGGR_OUTCASTS) && IS_PC(target) &&
        ((IS_INVADER(target)) ||
         ((PC_JUSTICE_FLAGS(target)) == JUSTICE_IS_OUTCAST)))
      return TRUE;

    /* now, race-specific flags */
    if (aggressive_to_race(ch, target))
        return TRUE;


    /* now, check class */
    if (aggressive_to_class(ch, target))
        return TRUE;


    /* final flags?  sex! */

    if (IS_AGGRO2FLAG(ch, AGGR2_MALE) && (GET_SEX(target) == SEX_MALE))
      return TRUE;

    if (IS_AGGRO2FLAG(ch, AGGR2_FEMALE) && (GET_SEX(target) == SEX_FEMALE))
      return TRUE;

    /* if target in memory, then it acts as if it's agg, even if it's not */

    if (CheckFor_remember(ch, target))
      return TRUE;

    /* nothing to get angry about */

    return FALSE;
  }
  else
  {
    /*  if ( (GET_RACE(ch) == RACE_CENTAUR) && (GET_RACE(target) == RACE_ORC))
       return TRUE; *//* centaurs always agg orcs */

    if (affected_by_spell(ch, SKILL_BERSERK) &&
        !GET_CLASS(ch, CLASS_BERSERKER) && (GET_RACE(ch) != RACE_MOUNTAIN) &&
        (GET_RACE(ch) != RACE_DUERGAR))
      return TRUE;
    if (IS_PC(target))
      return FALSE;
    if (GET_WIMPY(ch) > GET_HIT(ch))    /* not agg and wimpy both  */
      return FALSE;

    if (!IS_SET(ch->specials.act, PLR_VICIOUS))
    {
      if (IS_IMMOBILE(ch))
        return FALSE;
    }
    if ((ch->only.pc->aggressive != -1) &&
        (ch->only.pc->aggressive < GET_HIT(ch)) && is_aggr_to(target, ch))
      return TRUE;
  }

  return FALSE;
}

/*
 * ugly code all over, so I'm making it a function.  Warning:  this function is called
 * extremely often, even a slight increase in size will strongly affect cpu load. JAB
 */
bool is_aggr_to(P_char ch, P_char target)
{
  int      tmp_race = 0;
  register int chance;

  if( ch == target || !IS_ALIVE(ch) || !IS_ALIVE(target) )
  {
    return FALSE;
  }
  // 99.9% calls will leave here
  if(IS_NPC(ch) &&
     IS_NPC(target) &&
     (!target->following || IS_NPC(target->following)) &&
     !IS_MORPH(target))
  {
    return FALSE;
  }
  
  if(IS_FIGHTING(ch) ||
    IS_DESTROYING(ch) ||
    !IS_AWAKE(ch) ||
    IS_IMMOBILE(ch) ||
    CHAR_IN_SAFE_ZONE(ch) ||
    !CAN_SEE(ch, target) ||
    IS_AFFECTED(target, AFF_WRAITHFORM))
  {
    return FALSE;
  }
  return aggressive_to(ch, target);
}

bool aggressive_to_basic(P_char ch, P_char target)  // For exp checks
{
    if (IS_AGGROFLAG(ch, AGGR_ALL))
      return TRUE;

    if ((IS_EVIL(target) && IS_AGGROFLAG(ch, AGGR_EVIL_ALIGN)) ||
        (IS_GOOD(target) && IS_AGGROFLAG(ch, AGGR_GOOD_ALIGN)) ||
        (IS_NEUTRAL(target) && IS_AGGROFLAG(ch, AGGR_NEUTRAL_ALIGN)))
    {
      return TRUE;
    }

    if (GET_RACE(target) != GET_RACE(ch) &&
        ((IS_AGGROFLAG(ch, AGGR_GOOD_RACE) && target->player.racewar == RACEWAR_GOOD) ||
         (IS_AGGROFLAG(ch, AGGR_EVIL_RACE) && target->player.racewar == RACEWAR_EVIL) ||
         (IS_AGGROFLAG(ch, AGGR_UNDEAD_RACE) && target->player.racewar == RACEWAR_UNDEAD)))
    {
      return TRUE;
    }

    if (aggressive_to_race(ch, target))
        return TRUE;

    if (aggressive_to_class(ch, target))
        return TRUE;

    return FALSE;
}


/*
 * saving throws based on ch's stats, rather than magical saves.  Only DEX/AGI/INT
 * are imped so far, but all stats can be done with this function.  'mod' is
 * only any additional modifiers, this routine checks for all 'normal' mods
 * to a stat-based save.  For simplicity, it uses the APPLY_* macros for 'stat'
 * JAB
 */
bool StatSave(P_char ch, int stat, int mod)
{
  int save_num;

  if( !SanityCheck(ch, "StatSave") )
  {
    return FALSE;
  }

  /*
   * change this when adding other stats
   */
  if( (stat != APPLY_AGI) && (stat != APPLY_INT) &&
    (stat != APPLY_POW) && (stat != APPLY_DEX) &&
    (stat != APPLY_CON) && (stat != APPLY_WIS) &&
    (stat != APPLY_STR) )
  {
    return FALSE;
  }

  switch (stat)
  {
    case APPLY_AGI:
      save_num = STAT_INDEX(GET_C_AGI(ch)) + mod;

      if( !GET_CLASS(ch, CLASS_MONK) )
      {
        if( IS_AFFECTED(ch, AFF_HASTE) )
          save_num += 2;

        if( IS_AFFECTED2(ch, AFF2_SLOW) )
          save_num -= 2;
      }

      // Those heavy loaded folks are less than nimble eh?
      if( load_modifier(ch) > 299 )
      {
        save_num -= 3;
      }
      else if (load_modifier(ch) > 199)
      {
        save_num -= 2;
      }
      else if (load_modifier(ch) > 99)
      {
        save_num -= 1;
      }

      // And let us penalize for being off balance eh?
      if( IS_AFFECTED2(ch, AFF2_STUNNED) )
      {
        save_num -= 3;
      }
      save_num += GET_POS(ch) - 3;

      // There are a few bonuses
      if( IS_AFFECTED(ch, AFF_FLY) || IS_AFFECTED(ch, AFF_LEVITATE) )
      {
        save_num += 1;
      }
      break;

  case APPLY_DEX:
    save_num = STAT_INDEX(GET_C_DEX(ch)) + mod;

    if (IS_THIEF(ch))
      save_num += 3;

    if (IS_AFFECTED(ch, AFF_HASTE))
      save_num += 2;

    if (IS_AFFECTED2(ch, AFF2_SLOW))
      save_num -= 2;

    /*
     * those heavy loaded folks are less than nimble eh?
     */
    if (load_modifier(ch) > 299)
      save_num -= 3;
    else if (load_modifier(ch) > 199)
      save_num -= 2;
    else if (load_modifier(ch) > 99)
      save_num -= 1;

    /*
     * and let us penalize for being off balance eh?
     */
    if (IS_AFFECTED2(ch, AFF2_STUNNED))
      save_num -= 3;

    save_num += GET_POS(ch) - 3;

    break;
  case APPLY_INT:
    save_num = STAT_INDEX(GET_C_INT(ch)) + mod;
    if( has_innate(ch, INNATE_QUICK_THINKING) && number(1,100) <= get_property("saves.quickthinking.percentage", 15) )
      return TRUE;
    break;
  case APPLY_POW:
    save_num = STAT_INDEX(GET_C_POW(ch)) + mod;
    if( has_innate(ch, INNATE_QUICK_THINKING) && number(1,100) <= get_property("saves.quickthinking.percentage", 15) )
      return TRUE;
    break;
  case APPLY_CON:
    save_num = STAT_INDEX(GET_C_CON(ch)) + mod;
    break;
  case APPLY_WIS:
    save_num = STAT_INDEX(GET_C_WIS(ch)) + mod;
    break;
  case APPLY_STR:
    save_num = STAT_INDEX(GET_C_STR(ch)) + mod;
    break;
  default:
    return FALSE;
    break;
  }

  // 1/21 chance to fail, always.. why not 1/20 chance?  Guess we're a little nicer than D&D.
  return (number(1, 21) < BOUNDED(1, save_num, 20));
}

/*
 * capitalize the first alpha character in string, had to replace the old
 * macro because so many strings have ansi at the start of them now.  JAB
 */
void CAP(char *str)
{
  int      pos = 0;

  if (!str || !*str)
    return;

  // This is a while loop in case we have &n&N&+R&-Lthe ugly colored coat.&n
  while(str[pos] == '&')
  {
    if( str[pos + 1] == '=' && is_ansi_char(str[pos + 2]) && is_ansi_char(str[pos+3]) )
      pos += 4;
    else if( str[pos + 1] == 'n' || str[pos + 1] == 'N' )
      pos += 2;
    else if( str[pos + 1] == '-' || str[pos + 1] == '+' && is_ansi_char(str[pos+2]) )
      pos += 3;
    // It's an actual & at the start of the string.. go figure.
    else
      break;
  }
  str[pos] = UPPER(str[pos]);
}

// Makes the first non-ansi char in the string lower case.
void DECAP(char *str)
{
  int      pos = 0;

  if (!str || !*str)
    return;

  // This is a while loop in case we have &n&N&+R&-Lthe ugly colored coat.&n
  while(str[pos] == '&')
  {
    if( str[pos + 1] == '=' && is_ansi_char(str[pos + 2]) && is_ansi_char(str[pos+3]) )
      pos += 4;
    else if( str[pos + 1] == 'n' || str[pos + 1] == 'N' )
      pos += 2;
    else if( str[pos + 1] == '-' || str[pos + 1] == '+' && is_ansi_char(str[pos+2]) )
      pos += 3;
    // It's an actual & at the start of the string.. go figure.
    else
      break;
  }
  str[pos] = LOWER(str[pos]);
}

char *PERS(P_char ch, P_char vict, int short_d)
{
  return PERS(ch, vict, short_d, false);
}

// This is kinda backwards: how does vict see ch?
char *PERS(P_char ch, P_char vict, int short_d, bool noansi)
{
  // If vict can't see ch.
  if( !CAN_SEE_Z_CORD(vict, ch) )
  {
    strcpy(GS_buf1, "someone");
    return GS_buf1;
  }

  // We can assume here that vict can see ch in some way.

  // Handle infravision during dayblind / nightblind.
  // If they have infra, then it's a red shape..
  if( IS_AFFECTED(vict, AFF_INFRAVISION) && !IS_TRUSTED(vict) )
  {
    if( IS_DAYBLIND(vict) && !CAN_NIGHTPEOPLE_SEE(ch->in_room) )
    {
      bool globe = FALSE;
      if( ch->in_room >= 0 )
      {
        for( P_char rch = world[ch->in_room].people; rch; rch = rch->next_in_room )
        {
          if( IS_AFFECTED4(rch, AFF4_GLOBE_OF_DARKNESS) )
          {
            globe = TRUE;
            break;
          }
        }
      }
      if( !globe )
      {
        strcpy(GS_buf1, noansi ? "a red shape" : "&+ra red shape&n");
        return GS_buf1;
      }
    }
    else if( !IS_AFFECTED2(vict, AFF2_ULTRAVISION) && !CAN_DAYPEOPLE_SEE(ch->in_room) )
    {
      bool flame = FALSE;
      if( ch->in_room >= 0 )
      {
        for( P_char rch = world[ch->in_room].people; rch; rch = rch->next_in_room )
        {
          if( IS_AFFECTED4(rch, AFF4_MAGE_FLAME) )
          {
            flame = TRUE;
            break;
          }
        }
      }
      if( !flame )
      {
        strcpy(GS_buf1, noansi ? "a red shape" : "&+ra red shape&n");
        return GS_buf1;
      }
    }
  }

  // If it's across racewar sides.
#if 0
  if( (racewar(vict, ch) /* && !IS_ILLITHID(vict)) */  || !is_introd(ch, vict) )
#else
  if( racewar(vict, ch) /* && !IS_ILLITHID(vict) */ )
#endif
  {
    if( IS_DISGUISE_PC(ch) )
    {
      sprintf( GS_buf1, noansi ? "%s" : "%s %s", noansi ? race_names_table[GET_DISGUISE_RACE(ch)].normal
        : ANA(*(race_names_table[GET_DISGUISE_RACE(ch)].normal)), race_names_table[GET_DISGUISE_RACE(ch)].ansi );
    }
    else if( IS_DISGUISE_NPC(ch) )
    {
      sprintf(GS_buf1, "%s", (noansi ? strip_ansi(ch->disguise.name).c_str() : (ch->disguise.name)) );
    }
    else
    {
      sprintf( GS_buf1, noansi ? "%s" : "%s %s", noansi ? race_names_table[GET_RACE(ch)].normal
        : ANA(*(race_names_table[GET_RACE(ch)].normal)), race_names_table[GET_RACE(ch)].ansi );
    }
    return GS_buf1;
  }

  // You can always see yourself (unless blind/invis & no di/etc).
  if( IS_PC(vict) && vict == ch )
  {
    return vict->player.name;
  }

  if( IS_NPC(ch) )
  {
    return ch->player.short_descr;
  }
  if( IS_DISGUISE_PC(ch) )
  {
    return GET_DISGUISE_NAME(ch);
  }
  if( IS_DISGUISE_NPC(ch) )
  {
    sprintf(GS_buf1, "%s", (noansi ? strip_ansi(ch->disguise.name).c_str() : (ch->disguise.name)) );
    return GS_buf1;
  }
  if( is_introd(ch, vict) )
  {
    return ch->player.name;
  }
  if( short_d )
  {
    return ch->player.short_descr;
  }
  sprintf( GS_buf1, noansi ? "%s" : "%s %s", noansi ? race_names_table[GET_RACE(ch)].normal
    : ANA(*(race_names_table[GET_RACE(ch)].normal)), race_names_table[GET_RACE(ch)].ansi );
  return GS_buf1;
}

/*
 * adds spaces to the end of a string containing ANSI sequences so that the
 * printable string length == previous actual string length.  use only for
 * formatted %#s type outputs.  JAB
 */

void ansi_comp(char *str)
{
  int      np_len = 0;

  while (*str)
  {
    if (*str == '&')
    {
      if (LOWER(*(str + 1)) == 'n')
      {
        str += 2;
        np_len += 4;
        continue;
      }
      if ((*(str + 1) == '-') || (*(str + 1) == '+'))
      {
        if (isupper(*(str + 1)))
          np_len += 2;
        str += 3;
        np_len += 4;
        continue;
      }
      if ((*(str + 1) == '='))
      {
        if (isupper(*(str + 1)))
          np_len += 2;
        if (isupper(*(str + 2)))
          np_len += 2;
        str += 4;
        np_len += 7;
        continue;
      }
    }
    str++;
  }

  /*
   * this can overwrite memory, as there is no easy way to check for assigned
   * string length, without passing it in, if it becomes a problem we can do
   * that. JAB
   */

  while (np_len)
  {
    *str = ' ';
    np_len--;
    str++;
  }

  *str = '\0';
}

// str = string to pad, length = max number of total chars of return string
// trim_to_length -> trim strings that are longer than length chars.
// Note: if length is negative, we pad the beginning instead of the end.
string pad_ansi(const char *str, int length, bool trim_to_length)
{
  register char lookat;
  bool bPadEnd = TRUE;
  string ret_str("");
  int to_pad;

  if( length < 0 )
  {
    bPadEnd = FALSE;
    // Make length non-negative.
    length *= -1;
  }
  to_pad = length - strip_ansi(str).size();

  // If we're not padding the end, pad the beginning.
  if( !bPadEnd )
  {
    while( to_pad > 0 )
    {
      ret_str += " ";
      to_pad--;
    }
  }

  ret_str += str;
  // If we need to trim the end,
  if( trim_to_length && to_pad < 0 )
  {
    int count = 0, retLength = 0;

    while( count < length )
    {
      // If we're at the beginning of a possible ansi code,
      if( str[retLength] == '&' )
      {
        lookat = str[retLength+1];
        // &n or &N ansi code found: skip past it without incrementing count.
        if( lookat == 'n' || lookat == 'N' )
        {
          retLength += 2;
        }
        // If the next char is part of a possible ansi code, increment count and retLength by two.
        else if( lookat == '+' || lookat == '-' || lookat == '=' )
        {
          // We're looking at the lowercase version of the next (3rd) char.
          //   We use switch because there's more than 6 good codes => faster.
          switch( LOWER(str[retLength+2]) )
          {
            // Valid color codes:
            case 'b':
            case 'c':
            case 'g':
            case 'l':
            case 'm':
            case 'r':
            case 'w':
            case 'y':
              // For &=<x><y> where <x> is a valid color code, we check <y> for a valid color code.
              if( lookat == '=' )
              {
                switch( LOWER(str[retLength+3]) )
                {
                  case 'b':
                  case 'c':
                  case 'g':
                  case 'l':
                  case 'm':
                  case 'r':
                  case 'w':
                  case 'y':
                    // If <y> is a valid color code, we skip &=<x><y> -> 4 chars.
                    retLength += 4;
                    break;
                  // &=<x><y> where <x> is a color code, but <y> is not.  We count &=<x> -> 3 chars.
                  default:
                    count += 3;
                    retLength += 3;
                    break;
                }
              }
              // For &+<x> or &-<x> where x is a color code, we skip these 3 characters.
              else
              {
                retLength +=3;
              }
              break;
            // &<+|-|=><x> where <x> is not a color code.  We count the &+ or &- or &= as regular chars.
            default:
              count += 2;
              retLength += 2;
              break;
          }
        }
        // The '&' is a counted char. (We don't know about the next char).
        else
        {
          count++;
          retLength++;
        }
      }
      // Otherwise, go to the next char.
      else
      {
        count++;
        retLength++;
      }
    } // End trimming

    // This occurs when we end with a bad ansi code (ie pad_ansi( "Hi&=Rx", 4, TRUE) will jump 4 chars at the &).
    if( count > length )
    {
      // We just cut off the last chars as they aren't valid ansi codes.
      retLength -= (count - length);
    }
    return ret_str.substr( 0, retLength );
  }
  // Otherwise, we check to see if it needs padding at the end.
  else
  {
    // This allows for padding at the end instead of the beginning by sending a negative length.
    // If we're padding the end, pad the end.
    if( bPadEnd )
    {
      while( to_pad > 0 )
      {
        ret_str += " ";
        to_pad--;
      }
    }
  }
  return ret_str;
}

string strip_ansi(const char *str)
{
  int      i = 0;
  string colorless;
  
  while (*str)
  {
    if (*str == '&')
    {
      if (LOWER(*(str + 1)) == 'n')
      {
        str += 2;
        continue;
      }
      if ((*(str + 1) == '-') || (*(str + 1) == '+'))
      {
        if (isupper(*(str + 1))) ;
        str += 3;
        continue;
      }
      if ((*(str + 1) == '='))
      {
        if (isupper(*(str + 1))) ;
        if (isupper(*(str + 2))) ;
        str += 4;
        continue;
      }
    }
    colorless.push_back(*str);
    i++;
    str++;
  }

  return colorless;
}



/*
 * all stats are 1-100, all adjusted stats are 0-511 (ubyte), by nature of the thing,
 * they are all treated identically, however, I shudder at the thought of 10 tables
 * with 511 entries each.  So, run the GET_C_XXX value through this function to yield
 * a number from 0 to 51.  Index values from 0-16 follow the bell-curve, but above
 * 100, the divisions are linear.  The lookup tables reflect this.
 * You can of course, use this return value directly, or the stat directly.
 * This WAS a macro, but the expansions were hideous, and actually exhausted virtual
 * memory during compile.  So it's a function, though a simple one.  JAB
 */

/* Someone changed this from a range 0 .. 51 to 0 .. 24.
 *   So, I'm changing it back below, but saving the old stuff just in case.
int STAT_INDEX(int v)
{
  if (v < 1)
    return 0;
  else if (v < 10)
    return 1;
  else if (v < 16)
    return 2;
  else if (v < 22)
    return 3;
  else if (v < 28)
    return 4;
  else if (v < 34)
    return 5;
  else if (v < 40)
    return 6;
  else if (v < 46)
    return 7;
  else if (v < 51)
    return 8;
  else if (v < 56)
    return 9;
  else if (v < 62)
    return 10;
  else if (v < 68)
    return 11;
  else if (v < 74)
    return 12;
  else if (v < 80)
    return 13;
  else if (v < 86)
    return 14;
  else if (v < 92)
    return 15;
  else if (v < 101)
    return 16;
  else if (v < 106)
    return 17;
  else if (v < 116)
    return 18;
  else if (v < 130)
    return 19;
  else if (v < 148)
    return 20;
  else if (v < 170)
    return 21;
  else if (v < 196)
    return 22;
  else if (v < 226)
    return 23;
  else
    return 24;

return 24;
}
*/

// This function converts an attribute (range 0 .. 500 and some) to a proper
//   index of the attribute type tables (range 0 .. 51).
// Turn a GET_C_*** into a range of 0 .. 51 for use in the ***_app_type tables.
// Example: str_app[STAT_INDEX(GET_C_STR(ch))].todam
int STAT_INDEX( int v )
{
  if( v < 1 )
    return 0;
  else if( v < 10 )
    return 1;
  else if( v < 16 )
    return 2;
  else if( v < 22 )
    return 3;
  else if( v < 28 )
    return 4;
  else if( v < 34 )
    return 5;
  else if( v < 40 )
    return 6;
  else if( v < 46 )
    return 7;
  else if( v < 51 )
    return 8;
  else if( v < 56 )
    return 9;
  else if( v < 62 )
    return 10;
  else if( v < 68 )
    return 11;
  else if( v < 74 )
    return 12;
  else if( v < 80 )
    return 13;
  else if( v < 86 )
    return 14;
  else if( v < 92 )
    return 15;
  else if( v < 101 )
    return 16;
  else if( v < 113 )
    return 17;
  else if( v < 125 )
    return 18;
  else if( v < 137 )
    return 19;
  else if( v < 149 )
    return 20;
  else if( v < 161 )
    return 21;
  else if( v < 173 )
    return 22;
  else if( v < 185 )
    return 23;
  else if( v < 197 )
    return 24;
  else if( v < 209 )
    return 25;
  else if( v < 221 )
    return 26;
  else if( v < 233 )
    return 27;
  else if( v < 245 )
    return 28;
  else if( v < 257 )
    return 29;
  else if( v < 269 )
    return 30;
  else if( v < 281 )
    return 31;
  else if( v < 293 )
    return 32;
  else if( v < 305 )
    return 33;
  else if( v < 317 )
    return 34;
  else if( v < 329 )
    return 35;
  else if( v < 341 )
    return 36;
  else if( v < 353 )
    return 37;
  else if( v < 365 )
    return 38;
  else if( v < 377 )
    return 39;
  else if( v < 389 )
    return 40;
  else if( v < 401 )
    return 41;
  else if( v < 413 )
    return 42;
  else if( v < 425 )
    return 43;
  else if( v < 437 )
    return 44;
  else if( v < 449 )
    return 45;
  else if( v < 461 )
    return 46;
  else if( v < 473 )
    return 47;
  else if( v < 485 )
    return 48;
  else if( v < 497 )
    return 49;
  else if( v < 509 )
    return 50;

  return 51;
}

// This relates to stat_names3 where average = human at 100
int STAT_INDEX2(int v)
{
  if (v < 54)
    return 0;
  else if (v < 66)
    return 1;
  else if (v < 76)
    return 2;
  else if (v < 86)
    return 3;
  else if (v < 96)
    return 4;
  else if (v < 106)
    return 5;
  else if (v < 116)
    return 6;
  else if (v < 130)
    return 7;
  else if (v < 148)
    return 8;
  else if (v < 170)
    return 9;
  else if (v < 196)
    return 10;
  else if (v < 226)
    return 11;
  else
    return 12;

  return 12;
}

// This relates to stat_names3 where average = human pulse at 14
int STAT_INDEX_DAMAGE_PULSE(float v)
{
  if (v > 18)
    return 0;
  else if (v > 17.00)
    return 1;
  else if (v > 16.00)
    return 2;
  else if (v > 15.00)
    return 3;
  else if (v > 14.00)
    return 4;
  else if (v > 13.00)
    return 5;
  else if (v > 12.00)
    return 6;
  else if (v > 11.00)
    return 7;
  else if (v > 10.00)
    return 8;
  else if (v > 9.00)
    return 9;
  else if (v > 8.00)
    return 10;
  else if (v > 7.00)
    return 11;
  else
    return 12;

  return 12;
}

// This relates to stat_names3 where average = human pulse (1.00)
int STAT_INDEX_SPELL_PULSE(float v)
{
 if (v > 1.450)
   return 0;
 if (v > 1.350)
   return 1;
 if (v > 1.250)
   return 2;
 if (v > 1.150)
   return 3;
 if (v > 1.050)
   return 4;
 if (v > 0.950)
   return 5;
 if (v > 0.900)
   return 6;
 if (v > 0.850)
   return 7;
 if (v > 0.80)
   return 8;
 if (v > 0.750)
   return 9;
 if (v > 0.700)
   return 10;
 if (v > 0.650)
   return 11;
 else
   return 12;

 return 12;
}

bool are_together(P_char ch1, P_char ch2)
{                               /*
                                 * SKB - 20 May 1995
                                 */
  if (!ch1 || !ch2)
  {
    return (FALSE);
  }
  
  if((ch1 == ch2->following) ||
    (ch2 == ch1->following))
  {
    return (TRUE);
  }
  else if(ch1->following &&
          (ch1->following == ch2->following))
  {
    return (TRUE);
  }
  
  return (FALSE);
}

bool has_help(P_char ch)
{
  P_char   tmp = NULL;

  if (!ch)
    return (FALSE);

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if (tmp == ch)
      continue;
    if (are_together(ch, tmp))
      return (TRUE);
  }
  return (FALSE);
}

P_char char_in_room(int room)
{
  int      num = 0;
  P_char   pl;

  /*
   * first count the number of mortals in the room
   */
  for (pl = world[room].people; pl; pl = pl->next_in_room)
    if (!IS_TRUSTED(pl) && IS_PC(pl))
      num++;
  if (!num)
    return (0);

  num = number(1, num);         /*
                                 * else pick one char at random
                                 */
  for (pl = world[room].people; pl; pl = pl->next_in_room)
  {
    if (!IS_TRUSTED(pl) && IS_PC(pl))
      num--;
    if (!num)
      return (pl);
  }
  return (0);
}

/* if spell is cast on ch, will it hurt the bastard?  (checks globe, etc)
   spl better be SPELL_* value or yer doomed */

bool spell_can_affect_char(P_char ch, int spl)
{
  int      i = GetLowestSpellCircle_p(spl);
  
  if(spl == SPELL_MOLTEN_SPRAY &&
     IS_UNDEADRACE(ch))
        return true;
        
  if(spl == SPELL_BALLISTIC_ATTACK)
    return true;

  return !((IS_AFFECTED(ch, AFF_MINOR_GLOBE) && (i < 4)) ||
           (IS_AFFECTED3(ch, AFF3_SPIRIT_WARD) && (i < 5)) ||
           (IS_AFFECTED3(ch, AFF3_GR_SPIRIT_WARD) && (i < 6)) ||
           (IS_AFFECTED2(ch, AFF2_GLOBE) && (i < 7) &&
            (spl != SPELL_NEG_ENERGY_BARRIER)));
}

/* is viewee at war with viewer? */

bool racewar(P_char viewer, P_char viewee)
{
  if( !IS_ALIVE(viewer) || !IS_ALIVE(viewee) )
  {
    return FALSE;
  }

  if( IS_MORPH(viewer) )
  {
    viewer = MORPH_ORIG(viewer);
    if( !IS_ALIVE(viewer) )
    {
      return FALSE;
    }
  }

  if( IS_TRUSTED(viewer) || IS_TRUSTED(viewee) || (viewer == viewee) || IS_NPC(viewer) )
  {
    return FALSE;
  }

  if( IS_NPC(viewee) && GET_VNUM(viewee) != IMAGE_REFLECTION_VNUM )
  {
    return FALSE;
  }

  /* illithids see everyone's true name */
//  if (IS_ILLITHID(viewer)) return FALSE;

  if( IS_DISGUISE(viewee) )
  {
    if (IS_DISGUISE_NPC(viewee))
    {
      return FALSE;
    }
    if (GET_RACEWAR(viewer) == viewee->disguise.racewar)
    {
       //if (IS_PC(viewer) && IS_PC(viewee))
//         wizlog(57,"&+Wooooooooooooooooooooooooops. viewer: %d, viewee: %d. viewee disguise: %d",GET_RACEWAR(viewer), GET_RACEWAR(viewee), viewee->disguise.racewar);
      return FALSE;
    }
    return TRUE;
  }

  if (RACE_EVIL(viewer) && !RACE_EVIL(viewee))
    return TRUE;

  if (RACE_PUNDEAD(viewer) && !RACE_PUNDEAD(viewee))
    return TRUE;

  if (RACE_GOOD(viewer) && !RACE_GOOD(viewee))
    return TRUE;

  if ((IS_HARPY(viewer) && GET_RACEWAR(viewer) == RACEWAR_NEUTRAL) &&
      !(IS_HARPY(viewee) && GET_RACEWAR(viewee) == RACEWAR_NEUTRAL))
    return TRUE;

#if 0
  if (RACE_PUNDEAD(viewer) && RACE_PUNDEAD(viewee))
    return FALSE;

  if (RACE_PUNDEAD(viewee) && RACE_PUNDEAD(viewer))
    return FALSE;

  if (RACE_EVIL(viewer) && RACE_PUNDEAD(viewee) && !RACE_PUNDEAD(viewer))
    return TRUE;

  if (RACE_PUNDEAD(viewer) && RACE_EVIL(viewee))
    return TRUE;

  if ((RACE_PUNDEAD(viewer) && RACE_GOOD(viewee)) ||
      (RACE_PUNDEAD(viewee) && RACE_GOOD(viewer)))
  {
    return TRUE;
  }

  /* no one sees an illithid's true name except illithids themselves,
     handled above */

/*  if (IS_ILLITHID(viewer) != IS_ILLITHID(viewee))
    return TRUE;*/

#endif
/*  if (IS_ILLITHID(viewee)) return TRUE; */

  /* charmed followers act as their masters */
  if (IS_NPC(viewer) && GET_MASTER(viewer) &&
      viewer->in_room == GET_MASTER(viewer)->in_room)
  {
//    if (RACE_EVIL(viewer->following) != RACE_EVIL(viewee))
    if (opposite_racewar(viewer->following, viewee))
      return TRUE;
  }
/*
  if (!(IS_PC(viewer) && IS_PC(viewee)) &&
      (GET_VNUM(viewee) != 250)) return FALSE;
*/
  return FALSE;
}

int IS_MORPH(P_char ch)
{
  if (!ch || !IS_NPC(ch))
    return FALSE;

#if 0
  if (ch->only.npc->memory && !IS_SET(ch->specials.act, ACT_MEMORY))
    return TRUE;
#endif
  if (ch->only.npc && ch->only.npc->orig_char)
    return TRUE;

  return FALSE;
}

  /* fathoms what square on the map a zone occupies */
  /* returns virtual nmber */
int maproom_of_zone(int zone_num)
{
  int      i, i2;
  struct zone_data *zone = 0;

  if (zone_num < 0 || zone_num > top_of_zone_table)
    return NOWHERE;

  zone = &zone_table[zone_num];

  for (i = zone->real_bottom; (i != NOWHERE) && (i <= zone->real_top); i++)
    for (i2 = 0; i2 < NUM_EXITS; i2++)
      if (world[i].dir_option[i2])
        if (world[world[i].dir_option[i2]->to_room].zone != world[i].zone)
          if (world[i].dir_option[i2]->to_room != NOWHERE &&
              IS_MAP_ROOM(world[i].dir_option[i2]->to_room))
            return (world[world[i].dir_option[i2]->to_room].number);
  return NOWHERE;
}


/* real rooms only! */
int distance_from_shore(int room)
{
  int      dir, distance;

  for (dir = 0; dir < NUM_EXITS; dir++)
  {
    for (distance = 0; distance <= 10; distance++)
    {
      if (!IS_WATER_ROOM(room))
        return distance;
      if (VIRTUAL_EXIT(room, dir) &&
          (VIRTUAL_EXIT(room, dir)->to_room != NOWHERE))
        room = world[room].dir_option[dir]->to_room;
    }
  }
  return 10;                    /* over 10 is irrelevant */
}


/* check an argument, return an exit number (or -1 if none found)..  nice n
   reusable */

int dir_from_keyword(char *keyword)
{
  int      dir;
  const char *keywords[] = {
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "northwest",
    "southwest",
    "northeast",
    "southeast",
    "nw",
    "sw",
    "ne",
    "se",
    "\n"
  };

  dir = search_block(keyword, keywords, FALSE);

  /* adjust shortened diagonals.. */

  if ((dir >= 10) && (dir <= 13))
    dir -= 4;
  if (dir >= NUM_EXITS)
    return -1;

  return dir;
}

/* self-explanatory :) 0 would be naked, 14 overloaded */
int weight_notches_above_naked(P_char ch)
{
  P_char rider;
  int percent = CAN_CARRY_W(ch);

  if (percent <= 0)
    percent = 1;

  percent = (int) ((IS_CARRYING_W(ch, rider) * 100) / percent);

  if (percent <= 0)
    return 0;
  else if (percent <= 1)
    return 1;
  else if (percent <= 3)
    return 2;
  else if (percent <= 6)
    return 3;
  else if (percent <= 10)
    return 4;
  else if (percent <= 15)
    return 5;
  else if (percent <= 25)
    return 6;
  else if (percent <= 40)
    return 7;
  else if (percent <= 55)
    return 8;
  else if (percent <= 70)
    return 9;
  else if (percent <= 80)
    return 10;
  else if (percent <= 89)
    return 11;
  else if (percent <= 94)
    return 12;
  else if (percent < 100)
    return 13;

  return 14;
}


/*
 * char_in_snoopby_list
 */

char char_in_snoopby_list(snoop_by_data * head, P_char ch)
{
  snoop_by_data *node = head;

  while (node)
  {
    if (node->snoop_by == ch)
      return TRUE;

    node = node->next;
  }

  return FALSE;
}


/*
 * rem_char_from_snoopby_list
 */

void rem_char_from_snoopby_list(snoop_by_data ** head, P_char ch)
{
  snoop_by_data *node = *head, *old = NULL;

  if ((*head)->snoop_by == ch)
  {
    old = *head;
    *head = (*head)->next;

    FREE(old);
    return;
  }

  while (node)
  {
    if (node->snoop_by == ch)
    {
      if (!old)
      {
        send_to_char("rem_char_from_snoopby_list: error #1\r\n", ch);
        return;
      }

      old->next = node->next;

      FREE(node);
      return;
    }

    old = node;
    node = node->next;
  }

  send_to_char("rem_char_from_snoopby_list: error #2\r\n", ch);
}

int race_portal_check(P_char ch, P_char vict)
{
  /* okay to hit them if:
     1) either side is illithid
     2) they are above X level AND
     3) are in same zone
   */
  return FALSE;
/*

  if (IS_ILLITHID(ch) || IS_ILLITHID(vict))
     return TRUE;   can always hit illithids
*/
  if (GET_LEVEL(vict) < (30 + number(1, 10)))
    return FALSE;               /* can't hit newbies */
  if (world[ch->in_room].zone != world[vict->in_room].zone)
    return FALSE;
  return TRUE;
}

int room_has_valid_exit(const int rnum)
{
  int      i;

  if ((rnum < 0) || (rnum > top_of_world))
  {
    logit(LOG_DEBUG, "room_has_valid_exit(): rnum out of range (%d)", rnum);
    return FALSE;
  }

  for (i = 0; i < NUM_EXITS; i++)
  {
    if (world[rnum].dir_option[i] &&
        !(world[rnum].dir_option[i]->exit_info & EX_CLOSED) &&
        !(world[rnum].dir_option[i]->exit_info & EX_SECRET) &&
        !(world[rnum].dir_option[i]->exit_info & EX_BLOCKED) &&
        !(world[rnum].dir_option[i]->exit_info & EX_WALLED) &&
        (world[rnum].dir_option[i]->to_room >= 0))
      return TRUE;
  }

  return FALSE;
}

void do_introduce(P_char ch, char *arg, int level)
{

#ifdef INTRO
  P_char   vict;
  P_desc   d;
  int      x;
  char     buf[40];

  if (IS_TRUSTED(ch) && (*arg == '\0'))
  {
    for (x = 0; x < MAX_INTRO; x++)
      if (ch->only.pc->introd_list[x])
      {
        sprintf(buf, "%ld\r\n", ch->only.pc->introd_list[x]);
        send_to_char(buf, ch);
      }
    return;
  }


  /* check room frist */

  vict = get_char_room_vis(ch, arg);

/*  vict = get_char_vis(ch,arg); */
  if (!vict)
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (!d->character || d->connected || !d->character->player.name)
        continue;
      if (!isname(d->character->player.name, arg))
        continue;
      if (!CAN_SEE_Z_CORD(ch, d->character))
        continue;

      vict = d->character;
      break;
    }
  }

  if (!vict || racewar(ch, vict))
  {
    send_to_char("Introduce yourself to whom?\r\n", ch);
    return;
  }

  if (vict == ch)
  {
    send_to_char
      ("Wow, after you get to know yourself you're really not such a bad guy.\r\n",
       ch);
    return;
  }

  if (IS_NPC(ch) || IS_NPC(vict))
    return;

  if (ch->in_room != vict->in_room)
  {
    send_to_char("That person doesn't seem to be here.\r\n", ch);
    return;
  }

  if (is_introd(ch, vict))
  {
    send_to_char("They already are familiar with you.\r\n", ch);
    return;
  }

  if (!CAN_SEE(vict, ch))
  {
    send_to_char
      ("How can you introduce yourself when they can't see you?\r\n", ch);
    return;
  }

  /* add ch to victs list */
  add_intro(ch, vict);
  send_to_char("They now know your name.\r\n", ch);
/*  sprintf(buf,"%s has introduced himself to you!\n\r",GET_NAME(ch));
  send_to_char(buf, vict); */
  act("$n has introduced $mself to you!", FALSE, ch, 0, vict, TO_VICT);
  return;
#else
  send_to_char("Pardon?\r\n", ch);
#endif
}
void do_testdesc(P_char ch, char *arg, int level)
{
  P_desc   d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->character)
      generate_desc(d->character);
  }
}

/* read in and alloc the array sizes for shape, appearance, and modifier arrays */
void boot_desc_data()
{
  FILE    *f;
  int      count;
  char     buf[20];

  /* appearance first */
  if (!(f = fopen("lib/descs/appearance", "r")))
    return;
  count = 0;
  do
  {
    fgets(buf, 100, f);
    buf[strlen(buf) - 1] = '\0';
    appearance_descs[count] = str_dup(buf);
    count++;
  }
  while (buf[0] != '$');

  num_appearances = count - 1;
  fclose(f);
  count = 0;
  buf[0] = ' ';
  if (!(f = fopen("lib/descs/shapes", "r")))
    return;
  while (buf[0] != '$')
  {
    fgets(buf, 100, f);
    buf[strlen(buf) - 1] = '\0';
    shape_descs[count] = str_dup(buf);
    count++;
  }
  num_shapes = count - 1;
  fclose(f);
  count = 0;
  buf[0] = ' ';
  if (!(f = fopen("lib/descs/modifiers", "r")))
    return;
  while (buf[0] != '$')
  {
    fgets(buf, 100, f);
    buf[strlen(buf) - 1] = '\0';
    modifier_descs[count] = str_dup(buf);
    count++;
  }
  num_modifiers = count - 1;
  fclose(f);

}

char    *generate_shape(P_char ch)
{
  return str_dup(shape_descs[number(0, num_shapes - 1)]);
}

char    *generate_appear(P_char ch)
{
  return str_dup(appearance_descs[number(0, num_appearances - 1)]);
}

char    *generate_modif(P_char ch)
{
  char    *buf;
  int      flag = 0;

  while (!flag)
  {
    buf = str_dup(modifier_descs[number(0, num_modifiers - 1)]);
    flag = 1;
    /* no bearded elves */
    if (((GET_RACE(ch) == RACE_GREY) || (GET_RACE(ch) == RACE_HALFELF) ||
         (GET_RACE(ch) == RACE_DROW) || (GET_RACE(ch) == RACE_THRIKREEN)) &&
        strstr(buf, "beard"))
      flag = 0;
    /* or female beards */
    if ((GET_SEX(ch) == SEX_FEMALE) && strstr(buf, "beard"))
      flag = 0;
  }
  return str_dup(buf);
}

void generate_desc(P_char ch)
{
  char     buf[80];
  char     buf2[40];
  const char *prep;

  if ((GET_RACE(ch) == RACE_ORC) || (GET_RACE(ch) == RACE_OGRE) ||
      (GET_RACE(ch) == RACE_OROG) || (GET_RACE(ch) == RACE_ILLITHID) ||
      (GET_RACE(ch) == RACE_AGATHINON) || (GET_RACE(ch) == RACE_PILLITHID) ||
      (GET_RACE(ch) == RACE_ELADRIN))
  {
    prep = "An";
  }
  else
    prep = "A";


  switch (number(0, 9))
  {
  case 0:                      /* just shape */
    sprintf(buf2, "%s", generate_shape(ch));
    sprintf(buf, "%s %s %s", VOWEL(buf2[0]) ? "An" : "A", buf2,
            race_names_table[GET_RACE(ch)].ansi);
    break;
  case 1:                      /* just modif */
    sprintf(buf, "%s %s with %s", prep, race_names_table[GET_RACE(ch)].ansi,
            generate_modif(ch));
    break;
  case 2:                      /* just appearance */
    sprintf(buf2, "%s", generate_appear(ch));
    sprintf(buf, "%s %s %s", VOWEL(buf2[0]) ? "An" : "A", buf2,
            race_names_table[GET_RACE(ch)].ansi);
    break;
  case 3:
  case 4:                      /* s+m */
    sprintf(buf2, "%s", generate_shape(ch));
    sprintf(buf, "%s %s %s with %s",
            VOWEL(buf2[0]) ? "An" : "A", buf2,
            race_names_table[GET_RACE(ch)].ansi, generate_modif(ch));
    break;
  case 5:
  case 6:                      /*s+a */
    sprintf(buf2, "%s", generate_shape(ch));
    sprintf(buf, "%s %s, %s %s",
            VOWEL(buf2[0]) ? "An" : "A", buf2, generate_appear(ch),
            race_names_table[GET_RACE(ch)].ansi);
    break;
  case 7:
  case 8:                      /*m+a */
    sprintf(buf2, "%s", generate_appear(ch));
    sprintf(buf, "%s %s %s with %s",
            VOWEL(buf2[0]) ? "An" : "A", buf2,
            race_names_table[GET_RACE(ch)].ansi, generate_modif(ch));
    break;
  case 9:                      /*m+a+s */
    sprintf(buf2, "%s", generate_shape(ch));
    sprintf(buf, "%s %s, %s %s with %s",
            VOWEL(buf2[0]) ? "An" : "A", buf2, generate_appear(ch),
            race_names_table[GET_RACE(ch)].ansi, generate_modif(ch));
    break;
  }                             /* case */
  ch->player.short_descr = str_dup(buf);
}

void remove_plushit_bits(P_char mob)
{
  mob->specials.affected_by3 &=
    ~(AFF3_PLUSONE | AFF3_PLUSTWO | AFF3_PLUSTHREE | AFF3_PLUSFOUR |
      AFF3_PLUSFIVE);
}

/* does victim know of ch's name? */

int is_introd(P_char viewee, P_char viewer)
{
  /* binary search of introd array */
  int      hi, lo, mid, nr;

  if (viewer == viewee)
    return TRUE;
  if (IS_MORPH(viewee))
    viewee = MORPH_ORIG(viewee);        /* get back to orig char for morphs */

/*  if (IS_ILLITHID(viewer)) return TRUE; */

  /* if we have a pet, pet is intro'd based on master..  for targeting, etc */

  if (IS_NPC(viewer) && viewer->following && IS_PC(viewer->following))
    return is_introd(viewee, viewer->following);

  if (IS_NPC(viewee) || (IS_NPC(viewer) && (GET_VNUM(viewer) != 250)))
    return TRUE;
  if (GET_A_NUM(viewee) && (GET_A_NUM(viewee) == GET_A_NUM(viewer)) &&
      IS_MEMBER(GET_A_BITS(viewer)) && IS_MEMBER(GET_A_BITS(viewee)))
    return TRUE;
  if (IS_TRUSTED(viewer) || IS_TRUSTED(viewee))
    return TRUE;
#ifdef INTRO
  hi = MAX_INTRO - 1;
  lo = 0;
  mid = (hi + lo) >> 1;
  nr = GET_RNUM(viewee);

  while ((viewer->only.pc->introd_list[mid] != nr) && (hi >= lo))
  {
    if (viewer->only.pc->introd_list[mid] < nr)
    {
      lo = mid + 1;
    }
    else
    {
      hi = mid - 1;
    }

    mid = (hi + lo) >> 1;
  }

  if (viewer->only.pc->introd_list[mid] == nr)
  {
    viewer->only.pc->introd_times[mid] = time(0);
    return TRUE;
  }
#else


  if (((RACE_EVIL(viewer) && RACE_EVIL(viewee)) ||
       (RACE_GOOD(viewer) && RACE_GOOD(viewee)) ||
       (RACE_PUNDEAD(viewer) && RACE_PUNDEAD(viewee)) ||
       (IS_NHARPY(viewer) && IS_NHARPY(viewee)) ||
       (IS_ILLITHID(viewer))))
    return TRUE;
//  if (!racewar(viewer, viewee)) return TRUE;
#endif
  return FALSE;
}

/* add vict to ch's list */
void add_intro(P_char introer, P_char introee)
{

  int      oldest;
  int      x;
  int      found;

  if (introee->only.pc->introd_list[0] != 0)
  {                             /* no free slot */
/*    oldest = ch->only.pc->introd_times[0];*/
    oldest = 0;
    for (x = 0; x < MAX_INTRO; x++)
      if (introee->only.pc->introd_times[x] <
          introee->only.pc->introd_times[oldest])
        oldest = x;
    /* got oldest, put 0, bubble 0 to top */
    for (x = oldest; x > 0; x--)
    {
      introee->only.pc->introd_times[x] =
        introee->only.pc->introd_times[x - 1];
      introee->only.pc->introd_list[x] = introee->only.pc->introd_list[x - 1];
    }
    introee->only.pc->introd_times[0] = 0;
    introee->only.pc->introd_list[0] = 0;
  }
  /* Okay, now we got 0 at top of array */
  /* step down, bumping everything up till we don't go down anymore */
  x = 1;
  found = 0;
  while ((x < MAX_INTRO) && (found == FALSE))
  {
    if (GET_PID(introer) < introee->only.pc->introd_list[x + 1])
    {
      found = TRUE;
      introee->only.pc->introd_list[x - 1] = introee->only.pc->introd_list[x];
      introee->only.pc->introd_times[x - 1] =
        introee->only.pc->introd_times[x];
      introee->only.pc->introd_list[x] = GET_PID(introer);
      introee->only.pc->introd_times[x] = time(0);
    }
    else
    {
      introee->only.pc->introd_list[x - 1] = introee->only.pc->introd_list[x];
      introee->only.pc->introd_times[x - 1] =
        introee->only.pc->introd_times[x];
    }
    x++;
  }
  if (!found)
  {                             /* tack to end */
    x--;
    introee->only.pc->introd_list[x] = GET_PID(introer);
    introee->only.pc->introd_times[x] = time(0);
  }
}

void purge_old_intros(P_char ch)
{
  int      swaps;
  int      temptime, tempnum;
  int      x, currtime;

  currtime = time(0);

  for (x = 0; x < MAX_INTRO; x++)
  {
    /* go through, 0 out old ones */
    if ((currtime - ch->only.pc->introd_times[x]) > 604800)
    {
      ch->only.pc->introd_list[x] = 0;
      ch->only.pc->introd_times[x] = 0;
    }
  }

  /* now bubblesort */
  swaps = 1;
  while (swaps)
  {
    swaps = 0;
    for (x = 0; x < MAX_INTRO - 1; x++)
    {
      if (ch->only.pc->introd_list[x] > ch->only.pc->introd_list[x + 1])
      {
        /* swap */
        swaps = 1;
        temptime = ch->only.pc->introd_times[x];
        tempnum = ch->only.pc->introd_list[x];
        ch->only.pc->introd_times[x] = ch->only.pc->introd_times[x + 1];
        ch->only.pc->introd_list[x] = ch->only.pc->introd_list[x + 1];
        ch->only.pc->introd_times[x + 1] = temptime;
        ch->only.pc->introd_list[x + 1] = tempnum;
      }
    }
  }
}

void setCharPhysTypeInfo(P_char ch)
{
  int      size;

#ifndef NEW_COMBAT
  return;
#else

#   if 0
  GET_PHYS_TYPE(ch) = getPhysTypebyRace(GET_RACE(ch));
#   endif
#   if 1
  GET_PHYS_TYPE(ch) = PHYS_TYPE_HUMANOID;       /* temporary .. */
#   endif
  size = getNumbBodyLocsbyPhysType(GET_PHYS_TYPE(ch));

  CREATE(ch->points.location_hit, sh_int, size);

  if (!ch->points.location_hit)
  {
    logit(LOG_EXIT, "setCharPhysInfo(): couldn't alloc phys info");
    raise(SIGSEGV);
  }

  bzero(ch->points.location_hit, sizeof(sh_int) * size);
#endif
}


/* broadcasts message to whatever arena rm is part of */

void broadcast_to_arena(const char *msg, P_char ch, P_char vict, int rm)
{
  int      i, j, pos;
  char     strn[MAX_STRING_LENGTH];
  P_char   c;


  if (!IS_SET(world[rm].room_flags, ARENA))
    return;                     /* ? */

  rm = world[rm].number;

  for (i = 0; i <= LAST_HOME; i++)
  {
    if (rm == hometown_arena[i][0])
      break;

    if ((rm >= hometown_arena[i][1]) && (rm <= hometown_arena[i][2]))
      break;
  }

  if (i > LAST_HOME)
    return;                     /* ? */

  j = hometown_arena[i][0];

  while (TRUE)
  {
    rm = real_room(j);

    for (c = world[rm].people; c; c = c->next_in_room)
    {
      if ((c == ch) || (c == vict))
        continue;

      /* msg must have first %s be name of ch, second be name of vict */

      sprintf(strn, msg, PERS(ch, c, 1), vict ? PERS(vict, c, 1) : "(null)");
      send_to_char(strn, c);
    }

    if (j == hometown_arena[i][0])
      j = hometown_arena[i][1];
    else
    {
      j++;
      if (j > hometown_arena[i][2])
        break;
    }
  }
}


/*
 * get_class_string
 */

char *get_class_string(P_char ch, char *strn)
{
  int   i = 0;
  char *t;
  bool found;

  strn[0] = 0;
  found = FALSE;
  // Display all classes (multiple -> ch is a NPC).
  for( i = 0; i <= CLASS_COUNT; i++ )
  {
    if( ch->player.m_class & (1 << i) )
    {
      sprintf(strn + strlen(strn), "%s%s", found ? " " : "", class_names_table[i + 1].ansi);
      found = TRUE;
    }
  }
  if( IS_SPECIALIZED(ch) )
  {
    sprintf(strn + strlen(strn), "&n / %s", GET_SPEC_NAME(ch->player.m_class, ch->player.spec-1));
  }

  if( IS_MULTICLASS_PC(ch) )
  {
    for( i = 0; i <= CLASS_COUNT; i++ )
    {
      {
        if( ch->player.secondary_class & (1 << i) )
        {
          sprintf(strn + strlen(strn), " %s", class_names_table[i + 1].ansi);
        }
      }
    }
  }

  return strn;
}

int flag2idx(int flag)
{
  int      i = 0;

  while (flag > 0)
  {
    i++;
    flag >>= 1;
  }

  return i;
}

/*
 *
 */

/* Turned this into a macro: returns same results. - Lohrr
int GET_LEVEL(P_char ch)
{
  if(!ch)
  return -1;

  return ch->player.level;
}
*/

/*
 * GET_CLASS
 */

int GET_CLASS(P_char ch, uint m_class)
{
  return ((ch->player.m_class & m_class) || (ch->player.secondary_class & m_class));
}

int GET_PRIME_CLASS(P_char ch, uint m_class)
{
  return ((ch->player.m_class & m_class));
}

int GET_SECONDARY_CLASS(P_char ch, uint m_class)
{
  return ((ch->player.secondary_class & m_class));
}

int GET_CLASS1(P_char ch, uint m_class)
{
  if (!IS_DISGUISE(ch))
    return GET_CLASS(ch, m_class);

  if (IS_DISGUISE_PC(ch))
  {
    return GET_DISGUISE_CLASS(ch) == m_class;
  }
  else if (IS_DISGUISE_NPC(ch))
  {
    return (ch->disguise.m_class & (1 << (m_class - 1)));
  }
  else
    return FALSE;

}


/*
#define GET_ALT_SIZE(ch) (BOUNDED(SIZE_MINIMUM, ((ch)->player.size + \
                                 ((IS_AFFECTED3((ch), AFF3_ENLARGE)) ? 1 : \
                                                           ((IS_AFFECTED3((ch), AFF3_REDUCE)) ? -1 : 0))) + \
                               ( ( IS_AFFECTED5( (ch) , AFF5_TITAN_FORM ) ) ? 3 : 0), \
                                                        SIZE_MAXIMUM))

*/
int GET_ALT_SIZE(P_char ch)
{
  int size = GET_SIZE(ch);

  if( affected_by_spell(ch,TAG_RACE_CHANGE) )
    size = race_size(GET_RACE(ch));
  
  
  if(IS_AFFECTED3(ch, AFF3_ENLARGE))
    size++;
  
  if(IS_AFFECTED3(ch, AFF3_REDUCE))
    size--;

  if( IS_AFFECTED5( ch , AFF5_TITAN_FORM ) )
    size = (size+2);
    
  return BOUNDED(SIZE_MINIMUM, size, SIZE_MAXIMUM);
}

/*
 * GET_CHAR_SKILL_P : get skill by player skill index # - works for NPCs and PCs
 *
 *          effective skill level can be dropped down to 0 by this func and is
 *          limited to max of 100
 */


ClassSkillInfo SKILL_DATA_ALL(P_char ch, int skill)
{
  ClassSkillInfo dummy;
  int required_level, new_cap;
  int pri_rlevel, sec_rlevel, pri_cap, sec_cap;
  float pri_mod = get_property("skill.cap.multi.mod.primarySkill", 100.0);
  float sec_mod = get_property("skill.cap.multi.mod.secondarySkill", 95.0);

  if (IS_MULTICLASS_PC(ch))
  {
    new_cap = 0;
    required_level = 0;

    pri_rlevel = SKILL_DATA(ch, skill).rlevel[0];
    sec_rlevel = SKILL_DATA2(ch, skill).rlevel[0];
    pri_cap = SKILL_DATA(ch, skill).maxlearn[0];
    sec_cap = SKILL_DATA2(ch, skill).maxlearn[0];

    if( pri_rlevel && !sec_rlevel )
    {
      new_cap = (int) ( pri_cap * pri_mod) / 100;
      required_level = pri_rlevel;
    }
    else if( sec_rlevel && !pri_rlevel )
    {
      new_cap = (int) (sec_cap * sec_mod) / 100;
      required_level = sec_rlevel + 5;
    }
    else if( pri_rlevel && sec_rlevel )
    {
      new_cap = (MAX( (int) (pri_cap * pri_mod), (int) (sec_cap * sec_mod) ))/100;
      required_level = MIN(pri_rlevel, (sec_rlevel+5) );
    }

    dummy.maxlearn[0] = new_cap;
    dummy.rlevel[0] = required_level;
  }
  else if( IS_MULTICLASS_NPC(ch) )
  {
    /* because multiclass NPCs store their multiple classes in m_class, it's not possible
    to simply use SKILL_DATA(), as this just returns the skill data for the class with the
    highest order flag bit. This routine should go through and find the skill with the highest
    value for any of the set classes - Torgal 4/2009 */

    int highest = 0;
    struct ClassSkillInfo *classEntry = skills[skill].m_class;
    struct ClassSkillInfo *highestClassEntry = classEntry;

    for (int i = 0; i < CLASS_COUNT; i++, classEntry++)
    {
      if (ch->player.m_class & (1 << i))
      {
        if (classEntry->maxlearn[0] > highest)
        {
          highestClassEntry = classEntry;
          highest = classEntry->maxlearn[0];
        }
      }
    }

    dummy = *highestClassEntry;
  }
  else
  {
    dummy = SKILL_DATA(ch, skill);
  }

  return dummy;
}


int GET_CHAR_SKILL_P(P_char ch, int skl)
{
  struct affected_type *af;
  int      mod = 0, skllvl, highest, i, bit, lvl, cls, spec;
  struct ClassSkillInfo *classEntry;

  if(!ch)
  {
    return 0;
  }

  //Centaurs get innate 2h slashing at lvl 31.
  if( (skl == SKILL_2H_SLASHING) && (has_innate(ch, TWO_HANDED_SWORD_MASTERY)) )
  {
    return 100;
  }

  if(skl <= 0)
  {
    return 0;
  }

  if( IS_PC(ch) )
  {
    skllvl = ch->only.pc->skills[skl].learned;
  }
  else
  {
    // PC pets don't get epic skills.
    if( IS_PC_PET(ch) && IS_EPIC_SKILL(skl) )
    {
      return 0;
    }
    bit = 1;
    highest = 0;
    classEntry = skills[skl].m_class;
    lvl = GET_LEVEL(ch);
    cls = ch->player.m_class;
    spec = ch->player.spec;

    for (i = 0; i < CLASS_COUNT; i++, bit <<= 1, classEntry++)
    {
      if (cls & bit)
      {
        if (GET_LVL_FOR_SKILL(ch, skl) && lvl >= GET_LVL_FOR_SKILL(ch, skl))
        {
          if (classEntry->maxlearn[0] > highest)
            highest = classEntry->maxlearn[0];
          if (classEntry->maxlearn[spec] > highest)
            highest = classEntry->maxlearn[spec];
        }

        // multiclass-ness already calculated, so only need to check one class for non-multiclassers

        if (!IS_AFFECTED4(ch, AFF4_MULTI_CLASS))
          break;
      }
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && IS_PC_PET(ch))
      skllvl = BOUNDED(0, lvl + (lvl >> 1), highest);
    else
      skllvl = BOUNDED(0, lvl << 1, highest);
  }

  /*  af = ch->affected;

     while (af)
     {
     if ((af->location == APPLY_SKILL_ADD) && (af->loc2 == skl) && skllvl)
     mod += af->modifier;
     else
     if ((af->location == APPLY_SKILL_GRANT) && (af->loc2 == skl))
     mod += af->modifier;

     af = af->next;
       } */

#ifdef STANCES_ALLOWED
  if(skllvl > 0 && IS_PC(ch) && (ch->only.pc->frags > 0))
   mod = (int) (ch->only.pc->frags / 500);

  if(skllvl > 0)
  { 
    if( IS_SET(skills[skl].category , SKILL_CATEGORY_DEFENSIVE)){ 
       if(IS_AFFECTED5(ch, AFF5_STANCE_OFFENSIVE))
         mod = mod - 15;
       if(IS_AFFECTED5(ch, AFF5_STANCE_DEFENSIVE))
        mod = mod  + 10;
    } 

   
    if( IS_SET(skills[skl].category , SKILL_CATEGORY_OFFENSIVE))  {
      if(IS_AFFECTED5(ch, AFF5_STANCE_OFFENSIVE))
         mod = mod + 10;
      if(IS_AFFECTED5(ch, AFF5_STANCE_DEFENSIVE)) 
         mod = mod - 10;
   }
  }
 
   return MAX(0, (skllvl + mod) );
#else
   return skllvl;
#endif
}

int GET_LVL_FOR_SKILL(P_char ch, int skill)
{
  int nonspeclvl = SKILL_DATA_ALL(ch, skill).rlevel[0];
  int speclvl = SKILL_DATA_ALL(ch, skill).rlevel[ch->player.spec];

  if(!ch)
  {
    return 0;
  }  
  if( nonspeclvl && speclvl )
  {
    return MIN(nonspeclvl, speclvl);
  }
  else if( nonspeclvl )
  {
    return nonspeclvl;
  }
  else if( speclvl )
  {
    return speclvl;
  }
  else
  {
    return 0;
  }
}

/* this function returns a random non-god character in room
   flag can be DISALLOW_SELF, DISALLOW_GROUPED, DISALLOW_BACKRANK or
   any combination of these to respectively
   prevent picking ch, PCs or mobs grouped with him or
   PCs in the back rank
   function may return NULL if it does not find anybody
   matching the criteria
*/
P_char get_random_char_in_room(int room, P_char ch, int flag)
{
  P_char   tch;
  int      count;
  int      chosen;

  count = 0;

  for (tch = world[room].people, count = 0; tch; tch = tch->next_in_room)
  {
    if (IS_TRUSTED(tch) ||
        ((flag & DISALLOW_BACKRANK) && !on_front_line(tch)) ||
        ((flag & DISALLOW_SELF) && tch == ch) ||
        ((flag & DISALLOW_UNGROUPED) && !grouped(ch, tch)) ||
        ((flag & DISALLOW_GROUPED) && grouped(ch, tch)))
      continue;
    count++;
  }

  if (count == 0)
    return NULL;

  chosen = number(0, count - 1);

  for (tch = world[room].people, count = 0; tch; tch = tch->next_in_room)
  {
    if (IS_TRUSTED(tch) ||
        ((flag & DISALLOW_BACKRANK) && !on_front_line(tch)) ||
        ((flag & DISALLOW_SELF) && tch == ch) ||
        ((flag & DISALLOW_UNGROUPED) && !grouped(ch, tch)) ||
        ((flag & DISALLOW_GROUPED) && grouped(ch, tch)))
      continue;
    if (count == chosen)
      break;
    count++;
  }

  return tch;
}

/*
 * generic check for affect of area spells, returns: 1 - victim should be
 * hit by ch's area spell 0 - victim should not be hit by ch's area spell
 */
bool should_area_hit(P_char ch, P_char victim)
{
  P_char   c_leader = NULL, v_leader = NULL;
  struct affected_type *af;

  if( !ch || !victim )
    return FALSE;

  if( victim->specials.z_cord != ch->specials.z_cord )
    return FALSE;

  if( ch == victim )
    return FALSE;

  if( CHAR_IN_SAFE_ZONE(victim) )
    return FALSE;

  if( GET_STAT(victim) == STAT_DEAD )
    return FALSE;

  if( (ch->specials.fighting == victim) || (victim->specials.fighting == ch) )
    return TRUE;

  if( IS_AFFECTED(victim, AFF_WRAITHFORM) )
    return FALSE;

  if( IS_NPC(GET_PLYR(ch)) && IS_NPC(GET_PLYR(victim))
    && !(GET_PLYR(victim)->following && IS_PC(GET_PLYR(victim)->following))
    && !(GET_PLYR(ch)->following && IS_PC(GET_PLYR(ch)->following)) )
    return FALSE;

  if (IS_TRUSTED(victim) && (victim->specials.fighting != ch))
    return FALSE;

  if( (world[ch->in_room].room_flags & SINGLE_FILE) && !AdjacentInRoom(ch, victim) )
    return FALSE;

  if( af = get_spell_from_char(victim, TAG_IMMUNE_AREA) )
  {
    affect_remove(victim, af);
    return FALSE;
  }

  v_leader = victim;
  while (v_leader->following)
    v_leader = v_leader->following;

  c_leader = ch;
  while (c_leader->following)
    c_leader = c_leader->following;

  if ((ch->specials.fighting == v_leader) ||
      (victim->specials.fighting == c_leader))
    return TRUE;

  if (grouped(ch, victim))
    return FALSE;
  /*
  if (grouped(ch, victim) && on_front_line(ch))
    return FALSE;
  if (grouped(ch, victim) && !on_front_line(ch) && !on_front_line(victim))
    return FALSE;

  if (grouped(ch, victim))
    if (on_front_line(ch))
      return !number(0, 5);
    else
      return on_front_line(victim) || !number(0, 5);
      */

  return TRUE;
}

/*
 * as name implies, this function casts the given spell as if
 * it was area spell - used by staves and some item procs
 * currently it never casts on ch and harmful only on
 * ungrouped people, can add some flag parameter if you need it
*/
void cast_as_area(P_char ch, int spl, int level, char *arg)
{
  P_char   tch, nch;
  P_obj    tobj, nobj;
  void     (*spell_func) (int, P_char, char *, int, P_char, P_obj);

  if(!ch)
  {
    return;
  }
  
  spell_func = skills[spl].spell_pointer;

  if (spell_func == NULL)
    return;

  if (IS_AGG_SPELL(spl))
  {
    appear(ch);
    if (IS_SET(skills[spl].targets, TAR_AREA) ||
        IS_SET(skills[spl].targets, TAR_IGNORE))
      ((*spell_func) (level, ch, arg, SPELL_TYPE_SPELL, 0, 0));
    else if (IS_SET(skills[spl].targets, TAR_CHAR_ROOM))
    {
      for (tch = world[ch->in_room].people; tch; tch = nch)
      {
        nch = tch->next_in_room;
        if (ch != tch && (!ch->group || (ch->group != tch->group)))
        {
          justice_witness(ch, tch, CRIME_ATT_MURDER);
          ((*spell_func) (level, ch, arg, SPELL_TYPE_SPELL, tch, 0));
        }
      }
    }
  }
  else if (IS_SET(skills[spl].targets, TAR_AREA) ||
           IS_SET(skills[spl].targets, TAR_IGNORE))
  {
    ((*spell_func) (level, ch, 0, SPELL_TYPE_SPELL, 0, 0));
  }
  else
  {
    for (tch = world[ch->in_room].people; tch; tch = nch)
    {
      nch = tch->next_in_room;
      if (WIZ_INVIS(ch, tch))
      {
        continue;
      }
      if (IS_SET(skills[spl].targets, TAR_CHAR_ROOM))
      {
        ((*spell_func) (level, ch, arg, SPELL_TYPE_SPELL, tch, 0));
      }
      else if (IS_SET(skills[spl].targets, TAR_SELF_ONLY))
      {
        ((*spell_func) (level, tch, arg, SPELL_TYPE_SPELL, tch, 0));
      }
    }
  }

  // if (IS_SET(skills[spl].targets, TAR_OBJ_ROOM))
  // {
    // for (tobj = world[ch->in_room].contents; tobj; tobj = nobj)
    // {
      // nobj = tobj->next_content;
      // ((*spell_func) (level, ch, arg, SPELL_TYPE_SPELL, 0, tobj));
    // }
  // }
}

/*
 * Used by damage area spells
 */
int cast_as_damage_area(P_char ch, void (*spell_func) (int, P_char, char *, int, P_char, P_obj), int level,
  P_char victim, float min_chance, float chance_step, bool (*select_func) (P_char, P_char))
{
  P_char   tch, *vict_array;

  if( !IS_ALIVE(ch) )
  {
    return 0;
  }

  int ch_room = ch->in_room;

  ////////////
  // new algo

  if(IS_SET(world[ch_room].room_flags, SAFE_ZONE))
  {
    return 0;
  }

  int count = 0;
  for(tch = world[ch_room].people; tch; tch = tch->next_in_room)
  {
    if( IS_ALIVE(tch) )
      count++;
  }

  if( count <= 0 )
  {
    return 0;
  }

  CREATE(vict_array, P_char, count + 1, MEM_TAG_ARRAY);

  count = 0;
  int pc_count = 0;
  for(tch = world[ch_room].people; tch; tch = tch->next_in_room)
  {
    if( IS_ALIVE(tch) && select_func(ch, tch) )
    {
      vict_array[count++] = tch;
      if( IS_PC(tch) )
        pc_count++;
    }
  }

  if( pc_count > 0 )
  {
    float median = (float)pc_count / 2.0 + 5.0 / (float)pc_count;
    float range = 1.5;
    int pc_hit = number((int)((median - range / 2.0) * 1000.0), (int)((median + range / 2.0) * 1000.0)) / 1000;
    pc_hit = MAX((int)(pc_count * min_chance / 100), pc_hit);
    pc_hit = MIN(pc_hit, pc_count);
    int pc_skip = pc_count - pc_hit;
    if( pc_skip > 0 )
    {
      for( int i = number(0, count - 1); pc_skip; i = (i + 1) % count )
      {
        if( !vict_array[i] )
          continue;
        if( !IS_PC(vict_array[i]) )
          continue;
        if( vict_array[i] == victim )
          continue;
        if( !number(0, 1) )
          continue;
        vict_array[i] = 0;
        pc_skip--;
      }
    }
  }

  int hit = 0;
  for (int i = 0; i < count; i++)
  {
    P_char tch = vict_array[i];
    if( !tch )
      continue;
    if( !is_char_in_room(tch, ch_room) )
      continue;
    if( !is_char_in_room(ch, ch_room) )
      break;
    if( has_innate(tch, INNATE_EVASION) && GET_SPEC(tch, CLASS_MONK, SPEC_WAYOFSNAKE) )
    {
      if ((GET_LEVEL(tch) - ((int) get_property("innate.evasion.removechance", 15.000))) > number(1,100))
      {
        send_to_char("You twist out of the way avoiding the harmful magic!\n", tch);
        act ("$n twists out of the way avoiding the harmful magic!", FALSE, tch, 0, ch, TO_ROOM);
        continue;
      }
    }
    spell_func(level, ch, (char *) &hit, 0, tch, NULL);
    hit++;
  }

  /* old algo

  if(!(victim = get_random_char_in_room(ch_room, ch, DISALLOW_SELF)))
  {
    return 0;
  }

  int vict_room = victim->in_room;
  
  if(IS_SET(world[ch_room].room_flags, SAFE_ZONE) ||
     IS_SET(world[vict_room].room_flags, SAFE_ZONE))
  {
    return 0;
  }

  for(i = 0, tch = world[vict_room].people; tch; tch = tch->next_in_room)
  {
    if(IS_ALIVE(tch))
    {
      i++;
    }
  }

  CREATE(vict_array, P_char, i + 1, MEM_TAG_ARRAY);

  vict_array[i] = NULL;

  int i;
  for (i = 0, tch = victim; tch && (i == 0 || tch != victim);
       tch =
       (tch->next_in_room ? tch->next_in_room : world[vict_room].people))
  {
    vict_array[i++] = tch;
  }

  int hit = 0;
  for (int chance = 100, i = 0; tch = vict_array[i]; i++)
  {
    if (!is_char_in_room(tch, vict_room))
      continue;
    if (!is_char_in_room(ch, ch_room))
      break;
    if ((tch == ch ? ch == victim : select_func(ch, tch))
        && (chance > number(0, 99) || IS_NPC(ch)))    
    {
      if (has_innate(tch, INNATE_EVASION))
      {
  
        if ((GET_LEVEL(tch) - ((int) get_property("innate.evasion.removechance", 15.000))) > number(1,100))
        {
          send_to_char("You twist out of the way avoiding the harmful magic!\n", tch);
          act ("$n twists out of the way avoiding the harmful magic!", FALSE, tch, 0, ch, TO_ROOM);
          break;
        }
      }    

      spell_func(level, ch, (char *) &hit, 0, tch, NULL);
      chance = MAX((int) min_chance, (int) (chance - chance_step));
      hit++;
    }
  }*/

  FREE(vict_array);
  return hit;
}


int cast_as_damage_area(P_char ch, void (*spell_func) (int, P_char, char *, int, P_char, P_obj),
  int level, P_char victim, float min_chance, float chance_step)
{
  return cast_as_damage_area(ch, spell_func, level, victim, min_chance, chance_step, should_area_hit);
}

void hummer(P_obj obj)
{
  if (!obj || number(0, 9))
    return;
  if ((OBJ_WORN(obj) || OBJ_CARRIED(obj)))
  {
    act("&+LA faint hum can be heard from&N $p&n&+L carried by $n&n.",
        FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
    act("&+LA faint hum can be heard from&N $p&n&+L you are carrying.",
        FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
  }
  else if (OBJ_ROOM(obj))
  {
    act("&+LA strange humming sound comes from&N $p&+L.",
        0, world[obj->loc.room].people, obj, 0, TO_ROOM);
    act("&+LA strange humming sound comes from&N $p&+L.",
        0, world[obj->loc.room].people, obj, 0, TO_ROOM);

  }
  return;
}

bool grouped(P_char ch, P_char ch2)
{
  return ch->group && ch->group == ch2->group;
}

int get_takedown_size(P_char ch)
{
  P_char   mount;

  if (mount = get_linked_char(ch, LNK_RIDING))
  {
    if (GET_ALT_SIZE(mount) >= GET_ALT_SIZE(ch))
      return MIN(SIZE_MAXIMUM, GET_ALT_SIZE(ch) + 1);
  }

  return GET_ALT_SIZE(ch);
}

bool char_falling(P_char ch)
{
  P_char   mount = get_linked_char(ch, LNK_RIDING);

  return (((world[(ch)->in_room].sector_type == SECT_NO_GROUND) ||
           (world[(ch)->in_room].sector_type == SECT_UNDRWLD_NOGROUND) ||
           (ch->specials.z_cord > 0)) && !IS_TRUSTED(ch) &&
          !IS_AFFECTED(ch, AFF_LEVITATE) && !IS_AFFECTED(ch, AFF_FLY) &&
          !(mount && (IS_AFFECTED(mount, AFF_LEVITATE) ||
                      IS_AFFECTED(mount, AFF_FLY))));
}

/*
 */

#define MAX_BUFFER     65536    /* size of the parse buffer          */
#define HTML_SIZE         21    /* length of <font color=\"??????\"> */

/* local procudures */
char    *ansi_to_html(const char *str);
void     append_html_color(char *buf, char *colorcode);

int ansi_strlen(const char *string)
{
  int      length = 0;
  int      i;
  int      state = 0;           /* 0 - clear, 1 - & last, 2 - &+ last */

  if (string == NULL)
    return 0;

  for (i = 0; string[i]; i++)
  {
    if (string[i] == '&' && state == 0)
      state = 1;
    else if ((string[i] == '+' || string[i] == '-') && state == 1)
      state = 2;
    else if (string[i] >= 'A' && string[i] <= 'z' && state == 2)
      state = 0;
    else if ((string[i] == 'n' || string[i] == 'N') && state == 1)
      state = 0;
    else
    {
      state = 0;
      length += state + 1;
    }
  }

  return length;
}

bool is_hot_in_room(int room)
{
  struct room_affect *raf;

  for (raf = world[room].affected; raf; raf = raf->next) {
    if (raf->type == SPELL_INCENDIARY_CLOUD ||
        raf->type == SPELL_FIRESTORM ||
        raf->type == SPELL_SCATHING_WIND)
      return true;
  }

  if( world[room].sector_type == SECT_FIREPLANE || world[room].sector_type == SECT_LAVA )
    return TRUE;

  return FALSE;
}

// This function determines whether room real number r is currently twilight.
// Checks for all cases where a room is twilight.  If it finds one, returns TRUE.
//   If none found, it defaults to return FALSE.  The one exception is the room flag DARK.
bool IS_TWILIGHT_ROOM(int r)
{
  int sect = world[r].sector_type;
  int flags = world[r].room_flags;

  // Twilight rooms are twilight unless magically darkened or lit.
  //   Twilight flag also overrides dark flag.
  if( IS_SET(flags, TWILIGHT) && !IS_SET(flags, MAGIC_DARK | MAGIC_LIGHT) )
  {
    return TRUE;
  }

  // If it's lit and darked, it's twilight.
  if( IS_SET(flags, MAGIC_LIGHT) && IS_SET(flags, MAGIC_DARK | DARK) )
  {
    return TRUE;
  }

  // This should be the only return FALSE until the end default return.
  //   If a room isn't flagged MAGIC_LIGHT, but is flagged DARK or MAGIC_DARK, it's not twilight.
  //   Also, if a room isn't flagged DARK or MAGIC_DARK, but is flagged MAGIC_LIGHT, it's not twilight.
  if( IS_SET(flags, DARK | MAGIC_DARK) || IS_SET(flags, MAGIC_LIGHT) )
  {
    return FALSE;
  }

  // All UD map rooms are twilight unless magically darkened/lit.
  // Underwater rooms and indoors, by default, are twilight, for now.
  if( IS_UD_MAP(r) || IS_SET(flags, UNDERWATER | INDOORS) )
  {
    return TRUE;
  }

  // Twilight by sector type.
  switch( sect )
  {
    // Sectors that are twilight if not magically lit/dark or regular room dark.
    // These are in order of creation (add to bottom of list).
    // Note the exceptions SWAMP & SNOWY_FOREST to get the extra check.
    //   These are only twilight when the sun is shining.
    case SECT_FOREST:
    case SECT_SWAMP:
    case SECT_SNOWY_FOREST:
      if( IS_NIGHT )
      {
        break;
      }
    case SECT_UNDERWATER:
    case SECT_UNDERWATER_GR:
    case SECT_FIREPLANE:
    case SECT_UNDRWLD_WILD:
    case SECT_UNDRWLD_CITY:
    case SECT_UNDRWLD_INSIDE:
    case SECT_UNDRWLD_WATER:
    case SECT_UNDRWLD_NOSWIM:
    case SECT_UNDRWLD_NOGROUND:
    case SECT_AIR_PLANE:
    case SECT_WATER_PLANE:
    case SECT_EARTH_PLANE:
    case SECT_ETHEREAL:
    case SECT_ASTRAL:
    case SECT_UNDRWLD_MOUNTAIN:
    case SECT_UNDRWLD_SLIME:
    case SECT_UNDRWLD_LOWCEIL:
    case SECT_UNDRWLD_LIQMITH:
    case SECT_UNDRWLD_MUSHROOM:
    case SECT_CASTLE:
    case SECT_NEG_PLANE:
    case SECT_PLANE_OF_AVERNUS:
    case SECT_CITY:
      return TRUE;
      break;
    // These sectors are also in order.  These are the ones exposed to the sunlight.
    // These are only twilight during sunrise/sunset (dawn/dusk).
    case SECT_INSIDE:
    case SECT_FIELD:
    case SECT_HILLS:
    case SECT_MOUNTAIN:
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
    case SECT_NO_GROUND:
    case SECT_OCEAN:
    case SECT_DESERT:
    case SECT_ARCTIC:
    case SECT_CASTLE_WALL:
    case SECT_CASTLE_GATE:
    case SECT_ROAD:
    case SECT_LAVA:
      // If time of day makes it twilight
      if( IS_TWILIGHT )
      {
        return TRUE;
      }
      break;
    default:
      debug( "IS_TWILIGHT_ROOM: Sector '%s' (%d) not found in switch.", sector_types[sect],sect );
      break;
  }

  return FALSE;
}

// This function should probably be retired for a macro to speed things up,
//   although I'm not sure if a bunch of tests on sector type would be faster via switch.
// Returns TRUE iff the room, real num r, is an outdoors room not under cover (ie forest is under cover).
// Note: This is intended for use with the IS_SUNLIT macro, so if the sun can't shine there,
//   it's not outdoors, ok?
bool IS_OUTDOORS(int r)
{
  int sect = world[r].sector_type;
  int flags = world[r].room_flags;

  // Rooms that are Lockers/Indoors/etc are not outside rooms.
  // Note: The "HOUSE" bit was removed.  I dunno if it'll come back or not,
  //   but should be in this list if it does.
  if( IS_SET(flags, LOCKER | INDOORS | UNDERWATER | TUNNEL | PRIVATE | NO_PRECIP
    | SINGLE_FILE | JAIL | GUILD_ROOM ) )
  {
    return FALSE;
  }

  // Outside by sector type.
  switch( sect )
  {
    // Sectors that are outside.
    // These are in order of creation (add to bottom of list).
    // These sectors are in order.  These are the ones that can be exposed to sunlight.
    case SECT_CITY:
    case SECT_FIELD:
    case SECT_HILLS:
    case SECT_MOUNTAIN:
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
    case SECT_NO_GROUND:
    case SECT_OCEAN:
    case SECT_DESERT:
    case SECT_ARCTIC:
    case SECT_CASTLE_WALL:
    case SECT_CASTLE_GATE:
    case SECT_ROAD:
    case SECT_LAVA:
      return TRUE;
      break;
    default:
      break;
  }

  return FALSE;
}

P_char find_player_by_pid(int pid)
{
  for (P_desc desc = descriptor_list; desc; desc = desc->next)
  {
    if (STATE(desc) == CON_PLYNG && desc->character && 
        IS_PC(desc->character) && GET_PID(desc->character) == pid )
      return desc->character;
  }
  
  return NULL;  
}

P_char find_player_by_name(const char *name)
{
  for (P_desc desc = descriptor_list; desc; desc = desc->next)
  {
    if (STATE(desc) == CON_PLYNG && desc->character && 
        IS_PC(desc->character) && !strcmp(GET_NAME(desc->character), name) )
      return desc->character;
  }
  
  return NULL;  
}

// Doesn't have to be logged in
P_char get_player_from_name(char *name)
{
  P_char player;

  player = (struct char_data *) mm_get(dead_mob_pool);
  player->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

  if (restoreCharOnly(player, skip_spaces(name)) < 0 || !player)
  {
    if (player)
      free_char(player);
    return NULL;
  }
  return player;
}

// Doesn't have to be logged in
int get_player_pid_from_name(char *name)
{
  P_char player;
  int pid = 0;

  player = (struct char_data *) mm_get(dead_mob_pool);
  player->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

  if (restoreCharOnly(player, skip_spaces(name)) < 0 || !player)
  {
    if (player)
      free_char(player);
    return 0;
  }
  if (player)
  {
    pid = GET_PID(player);
    free_char(player);
  }
  return pid;
}

char *get_player_name_from_pid(int pid)
{
  static char name[MAX_STRING_LENGTH];

  if (!pid)
    return NULL;

  if (!qry("SELECT name FROM players_core WHERE pid = '%d' AND active=1", pid))
  {
    debug("get_player_name_from_pid(): cant read from db");
    return NULL;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return NULL;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  sprintf(name, "%s", row[0]);
 
  mysql_free_result(res);

  return name;
}

sh_int *char_stat(P_char ch, int stat)
{
  if( !ch )
    return NULL;
  
  switch( stat )
  {
    case APPLY_STR:
      return &ch->base_stats.Str;
      break;
      
    case APPLY_DEX:
      return &ch->base_stats.Dex;
      break;
      
    case APPLY_INT:
      return &ch->base_stats.Int;
      break;
      
    case APPLY_WIS:
      return &ch->base_stats.Wis;
      break;
      
    case APPLY_AGI:
      return &ch->base_stats.Agi;
      break;
      
    case APPLY_CON:
      return &ch->base_stats.Con;
      break;
      
    case APPLY_POW:
      return &ch->base_stats.Pow;
      break;
      
    case APPLY_CHA:
      return &ch->base_stats.Cha;
      break;
      
    case APPLY_LUCK:
      return &ch->base_stats.Luk;
      break;
  }

  return NULL;
}  

/* always put a -1 at the end of array! */
bool in_array(int val, int arr[])
{
  for( int i = 0; arr[i] != -1; i++ )
  {
    if( val == arr[i] )
      return TRUE;
  }
  return FALSE;
}

/* always put a -1 at the end of array! */
int array_size(int arr[])
{
  int size = 0;

  for( int i = 0; arr[i] != -1; i++ )
  {
    size++;
  }
  return size;
}

// convert an ip address to a base10 number
long unsigned int ip2ul(const char *ip)
{
  long unsigned int result = 0;
  int var1, var2, var3, var4;
  
  if( sscanf(ip, "%d.%d.%d.%d", &var1, &var2, &var3, &var4) )
  {
    result = (var1 << 24) + (var2 << 16) + (var3 << 8) + var4;
  }
  
  return result;
}

// make binary representation string
int decimal2binary(unsigned decimal, char* str)
{
   unsigned remainder = 0;
   unsigned number = decimal;

   unsigned index = flag2idx(decimal);
   if(index) str[index] = '\0'; else str[1] = '\0';
   while(index > 1)
   {
      remainder = number%2;
      str[index-1] = (char)('0'+remainder);

      number = number >> 1;
      index--;
   }

   str[0] = (char)('0'+number);

   return index+1;
}

bool is_natural_creature(P_char ch)
{
  if (!ch)
    return FALSE;

  switch (GET_RACE(ch))
  {
    case RACE_REPTILE:
    case RACE_HERBIVORE:
    case RACE_CARNIVORE:
    case RACE_SNAKE:
    case RACE_INSECT:
    case RACE_ARACHNID:
    case RACE_AQUATIC_ANIMAL:
    case RACE_FLYING_ANIMAL:
    case RACE_QUADRUPED:
    case RACE_ANIMAL:
    case RACE_PLANT:
    case RACE_PARASITE:
    case RACE_PWORM:
    case RACE_SLIME:
    case RACE_PRIMATE:
     return TRUE;
  }
  return FALSE;
}

bool is_casting_aggr_spell(P_char ch)
{
    struct spellcast_datatype *data;
    P_nevent e1;

    if( !IS_ALIVE(ch) || !IS_CASTING(ch) )
    {
      return FALSE;
    }

    LOOP_EVENTS_CH( e1, ch->nevents )
    {
      if ( e1->func == event_spellcast )
      {
        data = (struct spellcast_datatype*)e1->data;
        if (IS_AGG_SPELL(data->spell))
        {
          return TRUE;
        }
        else
          return FALSE;
      }
    }
    return FALSE;
}

int is_prime_plane(int room)
{
  if ((world[room].sector_type != SECT_AIR_PLANE) &&
      (world[room].sector_type != SECT_WATER_PLANE) &&
      (world[room].sector_type != SECT_EARTH_PLANE) &&
      (world[room].sector_type != SECT_ETHEREAL) &&
      (world[room].sector_type != SECT_ASTRAL) &&
      (world[room].sector_type != SECT_NEG_PLANE) &&
      (world[room].sector_type != SECT_PLANE_OF_AVERNUS) &&
      (world[room].sector_type != SECT_FIREPLANE))
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

bool match_pattern(const char *pat, const char *str) 
{
  while (*str) {
    switch (*pat) {
      case '?':
        if (*str == '.') return FALSE;
        break;
      case '*':
        do { ++pat; } while (*pat == '*'); /* enddo */
        if (!*pat) return TRUE;
        while (*str) if (match_pattern(pat, str++)) return TRUE;
        return FALSE;
      default:
        if (*str != *pat) return FALSE;
        break;
    } /* endswitch */
    ++pat, ++str;
  } /* endwhile */
  while (*pat == '*') ++pat;
  return !*pat;
}

void connect_rooms(int v1, int v2, int to_dir, int from_dir)
{
  /* create exits from room1<->room2 */

  int      r1, r2, rdir;

  r1 = real_room0(v1);
  r2 = real_room0(v2);

  if (!r1 || !r2 || to_dir >= NUM_EXITS || from_dir >= NUM_EXITS )
  {
    logit(LOG_DEBUG, "Error: connect_rooms(%d, %d, %d, %d)", v1, v2, to_dir, from_dir);
    return;
  }

  if (to_dir >= 0 && !world[r1].dir_option[to_dir])
  {
    CREATE(world[r1].dir_option[to_dir], room_direction_data, 1, MEM_TAG_DIRDATA);
    world[r1].dir_option[to_dir]->to_room = r2;
    world[r1].dir_option[to_dir]->exit_info = 0;        
  }

  if (from_dir >= 0 && !world[r2].dir_option[from_dir])
  {
    CREATE(world[r2].dir_option[from_dir], room_direction_data, 1, MEM_TAG_DIRDATA);
    world[r2].dir_option[from_dir]->to_room = r1;
    world[r2].dir_option[from_dir]->exit_info = 0;    
  }
}

void connect_rooms(int v1, int v2, int dir)
{                               
  connect_rooms(v1, v2, dir, rev_dir[dir]);
}

void disconnect_exit(int v1, int dir)
{
  int r1 = real_room0(v1);

  if( !r1 || dir < 0 || dir >= NUM_EXITS || !VIRTUAL_EXIT(r1, dir) )
  return;

  FREE( VIRTUAL_EXIT(r1, dir) );
  VIRTUAL_EXIT(r1, dir) = NULL;  
}

void disconnect_rooms(int v1, int v2)
{
  int r1 = real_room0(v1);
  int r2 = real_room0(v2);

  if( !r1 || !r2 )
    return;

  int d1 = -1, d2 = -1;

  for( int i = 0; i < NUM_EXITS; i++ )
  {
    if( VIRTUAL_EXIT(r1, i) && VIRTUAL_EXIT(r1, i)->to_room == r2 )
      d1 = i;

    if( VIRTUAL_EXIT(r2, i) && VIRTUAL_EXIT(r2, i)->to_room == r1 )
      d2 = i;  
  }

  if( d1 >= 0 && d1 < NUM_EXITS )
  {
    FREE( VIRTUAL_EXIT(r1, d1) );
    VIRTUAL_EXIT(r1, d1) = NULL;
  }

  if( d2 >= 0 && d2 < NUM_EXITS )
  {
    FREE( VIRTUAL_EXIT(r2, d2) );
    VIRTUAL_EXIT(r2, d2) = NULL;    
  }
}

P_char get_char_online(char *name)
{
  P_char   i;

  for (i = character_list; i; i = i->next)
    if (isname(name, GET_NAME(i)))
      return i;
  return 0;
}

// Similar to get_char_online, except uses pid, and we don't care about returning the whole P_char.
// If you want to change this to get_char_from_pid_online... just change "return TRUE"s to "break"s and
//   change the "return FALSE" to "return temp_ch" and initialize temp_ch to NULL.
// The main reason I didn't return the P_char is mainly just to make the name of the function easier.
bool is_pid_online( int pid, bool includeLD )
{
  P_char temp_ch;

  if( includeLD )
  {
    for( temp_ch = character_list; temp_ch; temp_ch = temp_ch->next)
    {
      if( IS_PC( temp_ch ) && GET_PID(temp_ch) == pid )
      {
        return TRUE;
      }
    }
  }
  else
  {
    for( P_desc temp_d = descriptor_list; temp_d; temp_d = temp_d->next )
    {
      // Not checking people at the menu.
      // If you wanted to check them, could add && connected != CON_SLCT (at menu) or such.
      if( temp_d->connected != CON_PLYNG )
      {
        continue;
      }

      // Just making sure.  (If they're CON_PLYNG, they pretty much have to have a char, don't they?)
      if( (temp_ch = (temp_d->original != NULL) ? temp_d->original : temp_d->character) == NULL )
      {
        continue;
      }

      if( IS_PC( temp_ch ) && GET_PID(temp_ch) == pid )
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/* returns the CMD_ corresponding to the given direction */
int cmd_from_dir(int dir)
{
  switch(dir)
  {
    case NORTH:
      return CMD_NORTH;
    case EAST:
      return CMD_EAST;
    case SOUTH:
      return CMD_SOUTH;
    case WEST:
      return CMD_WEST;
    case UP:
      return CMD_UP;
    case DOWN:
      return CMD_DOWN;
    case NORTHWEST:
      return CMD_NORTHWEST;
    case SOUTHWEST:
      return CMD_SOUTHWEST;
    case NORTHEAST:
      return CMD_NORTHEAST;
    case SOUTHEAST:
      return CMD_SOUTHEAST;
  }
  return CMD_NONE;
}

int direction_tag(P_char ch)
{
  for (struct affected_type *afp = ch->affected; afp; afp = afp->next)
  {
    if (afp->type == TAG_DIRECTION)
    {
      return afp->modifier;
    }
  }
  return -1;  
}

const char *condition_str(P_char ch)
{
  int percent;
  
  if (GET_MAX_HIT(ch) > 0 && GET_HIT(ch) > 0)
    percent = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
  else
    percent = -1;
  
  if (percent >= 100)
    return "&+gexcellent";
  else if (percent >= 90)
    return "&+Yfew scratches";
  else if (percent >= 75)
    return "&+Ysmall wounds";
  else if (percent >= 50)
    return "&+Mfew wounds";
  else if (percent >= 30)
    return "&+mnasty wounds";
  else if (percent >= 15)
    return "&+Rpretty hurt";
  else if (percent >= 0)
    return "&+rawful";
  else
    return "&+rbleeding, close to death";
}

// Returns TRUE iff substr is found within str
// Not case sensitive!
bool sub_string( const char *str, const char *substr)
{
  int strlength = strlen(str);
  int sublength = strlen(substr);
  int i = 0;
  int j;

  // While substr will fit into strlength - i
  while( strlength >= sublength + i )
  {
    j = 0;
    // While the letters match..
    while( LOWER(str[i+j]) == LOWER(substr[j]) )
    {
      j++;
      // If we've found all the letters..
      if( j == sublength )
        return TRUE;
    }
    i++;
  }
  return FALSE;
}

// Looks through list of strings strset to see if name contains one of them.
// Not case sensitive!
// Returns TRUE iff name contains one of the strings in strset.
bool sub_string_set( const char *name, const char **strset )
{
  int i = 0;

  while( strset[i][0] != '\n' )
  {
    if( sub_string( name, strset[i] ) )
      return TRUE;
    i++;
  }

  return FALSE;
}

char *CRYPT2( char *passwd, char *name )
{
  char buf[40];

  // If it's not already encrypted.
  if( *name != '$' )
  {
    sprintf( buf, "$1$" );
    strncpy( buf+3, name, 8 );
    strcat( buf, "$" );
  }
  else
  {
    sprintf( buf, "%s", name );
  }

  return crypt( passwd, buf );
}

// Returns TRUE iff ch has a stone/monolith for a zone touch.
//   These chars do not get poofed out of a zone.
bool has_touch_stone( P_char ch )
{
  int i;
  P_obj obj;

  // First check inventory.
  for( obj = ch->carrying; obj; obj = obj->next_content )
  {
    if( obj_index[obj->R_num].func.obj == epic_stone )
    {
      return TRUE;
    }
  }
  // Then check to see if it's eq'd.. just to make sure.
  for( i = 0; i < MAX_WEAR; i++ )
  {
    if( (obj = ch->equipment[i]) != NULL )
    {
      if( obj_index[obj->R_num].func.obj == epic_stone )
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

// This function is designed to handle the random load code for the 2015-6 wipe.
// Quest items have 100% load and should be checked before calling this function.
// Approaches 50% chance as ival approaches infinity.  ival 1 -> 100% chance, 2 -> ~98.11%...
// For each failure of an item to load, we add an additional 5% chance.
//   This guarentees that at least 1 out of 10 items load.
bool item_load_check( P_obj item, int ival, int zone_percent )
{
  static int fails = 0;
  register int percent = (100 * ival + 5000) / (2 * ival + 49);

  percent += 5 * fails;

  percent = MIN( percent, zone_percent );

  if( percent > number(0, 99) )
  {
    fails = 0;
    return TRUE;
  }

  fails++;
  return FALSE;
}

// Takes a char's name and looks through the descriptors for it.
P_desc get_descriptor_from_name( char *name )
{
  for( P_desc d = descriptor_list; d; d = d->next)
  {
    if( !d->character || !d->character->player.name )
    {
      continue;
    }
    if( !strcmp(name, d->character->player.name) )
    {
      return d;
    }
  }
  return NULL;
}
