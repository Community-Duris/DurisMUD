/* ************************************************************************
*   File: boards.c                                      Part of CircleMUD *
*  Usage: handling of multiple bulletin boards                            *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#if 0
/*
TO ADD A NEW BOARD, simply follow our easy 3-step program:
1 - Create a new board object in the object files
2 - Increase the NUM_OF_BOARDS constant near array
3 - Add a new line to the board_info array below.  The fields, in order, are:
	Board's virtual number
	Min level one must be to look at this board or read messages on it.
	Min level one must be to post a message to the board.
	Min level one must be to remove other people's messages from this
		board (but you can always remove your own message).
	Filename of this board, in quotes.
	Last field must always be 0.
*/
#endif

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include "comm.h"
#include "db.h"
#include "interp.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "boards.h"

extern P_room world;
extern P_desc descriptor_list;
extern P_index obj_index;

int      Board_display_msg(int board_type, struct char_data *ch, char *arg);
int      Board_show_board(int board_type, struct char_data *ch, char *arg);
int      Board_remove_msg(int board_type, struct char_data *ch, char *arg);
void     Board_save_board(int board_type);
void     Board_load_board(int board_type);
void     Board_reset_board(int board_num);
void     Board_write_message(int board_type, struct char_data *ch, char *arg);


#define NUM_OF_BOARDS		44

/* vnum, read lvl, write lvl, remove lvl, filename, 0 */
struct board_info_type board_info[NUM_OF_BOARDS] = {
  {89, AVATAR, AVATAR, AVATAR, "lib/boards/holy", 0},
  {90, 0, 25, AVATAR, "lib/boards/mortal", 0},
  {48101, 25, 25, 30, "lib/boards/guild1", 0},
  {48102, 25, 25, 30, "lib/boards/guild2", 0},
  {48103, 25, 25, 30, "lib/boards/guild3", 0},
  {48104, 25, 25, 30, "lib/boards/guild4", 0},
  {48105, 25, 25, 30, "lib/boards/guild5", 0},
  {48106, 25, 25, 30, "lib/boards/guild6", 0},
  {48107, 25, 25, 30, "lib/boards/guild7", 0},
  {48108, 25, 25, 30, "lib/boards/guild8", 0},
  {48109, 25, 25, 30, "lib/boards/guild9", 0},
  {48110, 25, 25, 30, "lib/boards/guild10", 0},
  {48111, 25, 25, 30, "lib/boards/guild11", 0},
  {48112, 25, 25, 30, "lib/boards/guild12", 0},
  {48113, 25, 25, 30, "lib/boards/guild13", 0},
  {48114, 25, 25, 30, "lib/boards/guild14", 0},
  {48115, 25, 25, 30, "lib/boards/guild15", 0},
  {48116, 25, 25, 30, "lib/boards/guild16", 0},
  {48117, 25, 25, 30, "lib/boards/guild17", 0},
  {48118, 25, 25, 30, "lib/boards/guild18", 0},
  {48119, 25, 25, 30, "lib/boards/guild19", 0},
  {48120, 25, 25, 30, "lib/boards/guild20", 0},
  {48121, 25, 25, 30, "lib/boards/guild21", 0},
  {48122, 25, 25, 30, "lib/boards/guild22", 0},
  {48123, 25, 25, 30, "lib/boards/guild23", 0},
  {48124, 25, 25, 30, "lib/boards/guild24", 0},
  {48125, 25, 25, 30, "lib/boards/guild25", 0},
  {48126, 25, 25, 30, "lib/boards/guild26", 0},
  {48127, 25, 25, 30, "lib/boards/guild27", 0},
  {48128, 25, 25, 30, "lib/boards/guild28", 0},
  {48129, 25, 25, 30, "lib/boards/guild29", 0},
  {11130, 50, 50, AVATAR, "lib/boards/council", 0},
  {92, 20, 20, AVATAR, "lib/boards/flame", 0},
  {75, AVATAR, AVATAR, AVATAR, "lib/boards/areas", 0},
  {84, AVATAR, AVATAR, AVATAR, "lib/boards/bug", 0},
  {78, AVATAR, AVATAR, AVATAR, "lib/boards/namesA-F", 0},
  {79, AVATAR, AVATAR, AVATAR, "lib/boards/namesG-L", 0},
  {80, AVATAR, AVATAR, AVATAR, "lib/boards/namesM-R", 0},
  {81, AVATAR, AVATAR, AVATAR, "lib/boards/namesS-Z", 0},
  {85, AVATAR, AVATAR, AVATAR, "lib/boards/troubleplayer", 0},
  {88, AVATAR, AVATAR, AVATAR, "lib/boards/reimbursement", 0},
  {91, AVATAR, AVATAR, AVATAR, "lib/boards/meeting", 0},
  {29, AVATAR, AVATAR, AVATAR, "lib/boards/code", 0},
  {1204, AVATAR, AVATAR, AVATAR, "lib/boards/gellzboard", 0}
};


char    *msg_storage[INDEX_SIZE];
int      msg_storage_taken[INDEX_SIZE];
int      num_of_msgs[NUM_OF_BOARDS];
struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];


int find_slot(void)
{
  int      i;

  for (i = 0; i < INDEX_SIZE; i++)
    if (!msg_storage_taken[i])
    {
      msg_storage_taken[i] = 1;
      return i;
    }
  return -1;
}


/* search the room ch is standing in to find which board he's looking at */
int find_board(struct char_data *ch)
{
  P_obj    obj;
  int      i;

  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
    for (i = 0; i < NUM_OF_BOARDS; i++)
      if (obj_index[BOARD_RNUM(i)].virtual_number ==
          obj_index[obj->R_num].virtual_number)
        return i;
  return -1;
}


void initialize_boards(void)
{
  int      i, j, fatal_error = 0;
  char     buf[256];

  for (i = 0; i < INDEX_SIZE; i++)
  {
    msg_storage[i] = 0;
    msg_storage_taken[i] = 0;
  }

  for (i = 0; i < NUM_OF_BOARDS; i++)
  {
    if ((BOARD_RNUM(i) = real_object(BOARD_VNUM(i))) == -1)
    {
      sprintf(buf, " Fatal board error: board vnum %d does not exist!",
              BOARD_VNUM(i));
      logit(LOG_BOARD, buf);
      continue;
    }
    num_of_msgs[i] = 0;
    for (j = 0; j < MAX_BOARD_MESSAGES; j++)
    {
      memset((char *) &(msg_index[i][j]), 0, sizeof(struct board_msginfo));
      msg_index[i][j].slot_num = -1;
    }
    Board_load_board(i);
    if (!obj_index[real_object0(BOARD_VNUM(i))].func.obj)
      obj_index[real_object0(BOARD_VNUM(i))].func.obj = board;
  }

  if (fatal_error)
    exit(1);
}

int board(P_obj obj, P_char ch, int cmd, char *argument)
{
  int      board_type;
  static bool loaded = FALSE;

  /* check for periodic event calls  */
  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !loaded )
  {
    initialize_boards();
    loaded = TRUE;
  }

  if( !ch || !ch->desc )
  {
    return FALSE;
  }

  if( cmd != CMD_WRITE && cmd != CMD_LOOK && cmd != CMD_EXAMINE
    && cmd != CMD_READ && cmd != CMD_REMOVE )
  {
    return FALSE;
  }

  if( (board_type = find_board(ch)) == -1 )
  {
    logit(LOG_BOARD, "  degenerate board!  (what the hell...)");
    return FALSE;
  }
#if 0
  if( !(obj_index[obj->R_num].virtual_number > 11100) && !IS_TRUSTED(ch)
    && (cmd != CMD_LOOK) && (cmd != CMD_EXAMINE) && (cmd != CMD_REMOVE) )
  {
    send_to_char("The crustified board system has been replaced by our web-based message system hosted at\r\n"
       "http://www.duris.org/.  Enjoy.\r\n", ch);
    return TRUE;
  }
#endif
  if( cmd == CMD_WRITE )
  {
    Board_write_message(board_type, ch, argument);
    return 1;
  }
  else if( cmd == CMD_LOOK || cmd == CMD_EXAMINE )
    return (Board_show_board(board_type, ch, argument));
  else if( cmd == CMD_READ )
    return (Board_display_msg(board_type, ch, argument));
  else if( cmd == CMD_REMOVE )
    return (Board_remove_msg(board_type, ch, argument));
  else
    return FALSE;
}


void Board_write_message(int board_type, struct char_data *ch, char *arg)
{
  char    *tmstr;
  int      len;
  time_t   ct;
  char     buf[MAX_INPUT_LENGTH];

  if (GET_LEVEL(ch) < WRITE_LVL(board_type))
  {
    send_to_char("You are not holy enough to write on this board.\r\n", ch);
    return;
  }
  if (num_of_msgs[board_type] >= MAX_BOARD_MESSAGES)
  {
    send_to_char("The board is full.\r\n", ch);
    return;
  }
  if ((NEW_MSG_INDEX(board_type).slot_num = find_slot()) == -1)
  {
    send_to_char("The board is malfunctioning - sorry.\r\n", ch);
    logit(LOG_BOARD, " Board: failed to find empty slot on write.");
    return;
  }
  /* skip blanks */
  arg = skip_spaces(arg);

  /* JE 27 Oct 95 - Truncate headline at 70 chars if it's longer than that */
  arg[71] = '\0';

  if (!*arg)
  {
    send_to_char("We must have a headline!\r\n", ch);
    return;
  }

  ct = time(0);
  tmstr = (char *) asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 9) = '\0';  /* kill seconds and year */

  sprintf(buf, "[%s (%s)] %s", tmstr, GET_NAME(ch), arg);
  len = strlen(buf) + 1;

  CREATE(NEW_MSG_INDEX(board_type).heading, char, len, MEM_TAG_STRING);
  if ( NEW_MSG_INDEX(board_type).heading == NULL )
  {
    send_to_char("The board is malfunctioning - sorry.\r\n", ch);
    return;
  }
  strcpy(NEW_MSG_INDEX(board_type).heading, buf);
  NEW_MSG_INDEX(board_type).heading[len - 1] = '\0';
  NEW_MSG_INDEX(board_type).level = GET_LEVEL(ch);

  act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char("Write your message.  (/s saves /h for help)\r\n\r\n", ch);

  if (!IS_NPC(ch))
    SET_BIT(ch->specials.act, PLR_WRITE);

  ch->desc->str = NULL;
  ch->desc->str = &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]);
  ch->desc->max_str = MAX_MESSAGE_LENGTH;
  num_of_msgs[board_type]++;
}


int Board_show_board(int board_type, struct char_data *ch, char *arg)
{
  int      i;
  char     tmp[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];

  if (!ch->desc)
    return 0;

  one_argument(arg, tmp);

  if (!*tmp || !isname(tmp, "board bulletin"))
    return 0;

  if (GET_LEVEL(ch) < READ_LVL(board_type))
  {
    send_to_char("You try but fail to understand the holy words.\r\n", ch);
    return 1;
  }
  act("$n studies the board.", TRUE, ch, 0, 0, TO_ROOM);
  Board_save_board(board_type);

  strcpy(buf,
         "This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE <header>.\r\n"
         "You will need to look at the board to save your message.\r\n");
  if (!num_of_msgs[board_type])
    strcat(buf, "The board is empty.\r\n");
  else
  {
    sprintf(buf + strlen(buf), "There are %d messages on the board.\r\n",
            num_of_msgs[board_type]);
    /*   for (i = 0; i < num_of_msgs[board_type]; i++) {  */
    for (i = num_of_msgs[board_type] - 1; i >= 0; i--)
    {
      if (MSG_HEADING(board_type, i))
        sprintf(buf + strlen(buf), "%-2d : %s\r\n", i + 1,
                MSG_HEADING(board_type, i));
      else
      {
        logit(LOG_BOARD, " The board is fubar'd.");
        send_to_char("Sorry, the board isn't working.\r\n", ch);
        return 1;
      }
    }
  }
  page_string(ch->desc, buf, 1);

  return 1;
}


int Board_display_msg(int board_type, struct char_data *ch, char *arg)
{
  char     which[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];
  int      msg, ind;

  one_argument(arg, which);
  if (!*which)
    return 0;
  if (isname(which, "board bulletin"))  /* so "read board" works */
    return (Board_show_board(board_type, ch, arg));
  if (!isdigit(*which) || (!(msg = atoi(which))))
    return 0;

  if (GET_LEVEL(ch) < READ_LVL(board_type))
  {
    send_to_char("You try but fail to understand the holy words.\r\n", ch);
    return 1;
  }
  if (!num_of_msgs[board_type])
  {
    send_to_char("The board is empty!\r\n", ch);
    return (1);
  }
  if (msg < 1 || msg > num_of_msgs[board_type])
  {
    send_to_char("That message exists only in your imagination.\r\n", ch);
    return (1);
  }
  ind = msg - 1;
  if (MSG_SLOTNUM(board_type, ind) < 0 ||
      MSG_SLOTNUM(board_type, ind) >= INDEX_SIZE)
  {
    send_to_char("Sorry, the board is not working.\r\n", ch);
    logit(LOG_BOARD, " Board is screwed up.");
    return 1;
  }
  if (!(MSG_HEADING(board_type, ind)) || !msg)
  {
    send_to_char("That message appears to be screwed up.\r\n", ch);
    return 1;
  }
  if (!(msg_storage[MSG_SLOTNUM(board_type, ind)]))
  {
    send_to_char("That message seems to be empty.\r\n", ch);
    return 1;
  }
  sprintf(buffer, "Message %d : %s\r\n\r\n%s\r\n", msg,
          MSG_HEADING(board_type, ind),
          msg_storage[MSG_SLOTNUM(board_type, ind)]);

  page_string(ch->desc, buffer, 1);

  return 1;
}


int Board_remove_msg(int board_type, struct char_data *ch, char *arg)
{
  int      ind, msg, slot_num;
  char     which[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct descriptor_data *d;

  one_argument(arg, which);

  if (!*which || !isdigit(*which))
    return 0;
  if (!(msg = atoi(which)))
    return (0);

  if (!num_of_msgs[board_type])
  {
    send_to_char("The board is empty!\r\n", ch);
    return 1;
  }
  if (msg < 1 || msg > num_of_msgs[board_type])
  {
    send_to_char("That message exists only in your imagination.\r\n", ch);
    return 1;
  }
  ind = msg - 1;
  if (!MSG_HEADING(board_type, ind))
  {
    send_to_char("That message appears to be screwed up.\r\n", ch);
    return 1;
  }
  sprintf(buf, "(%s)", GET_NAME(ch));
  if (GET_LEVEL(ch) < REMOVE_LVL(board_type) &&
      !(strstr(MSG_HEADING(board_type, ind), buf)))
  {
    send_to_char
      ("You are not holy enough to remove other people's messages.\r\n", ch);
    return 1;
  }
  if (GET_LEVEL(ch) < MSG_LEVEL(board_type, ind))
  {
    send_to_char("You can't remove a message holier than yourself.\r\n", ch);
    return 1;
  }
  slot_num = MSG_SLOTNUM(board_type, ind);
  if (slot_num < 0 || slot_num >= INDEX_SIZE)
  {
    logit(LOG_BOARD, " The board is seriously screwed up.");
    send_to_char("That message is majorly screwed up.\r\n", ch);
    return 1;
  }
  for (d = descriptor_list; d; d = d->next)
    if (!d->connected && d->str == &(msg_storage[slot_num]))
    {
      send_to_char
        ("At least wait until the author is finished before removing it!\r\n",
         ch);
      return 1;
    }
  if (msg_storage[slot_num])
    FREE(msg_storage[slot_num]);
  msg_storage[slot_num] = 0;
  msg_storage_taken[slot_num] = 0;
  if (MSG_HEADING(board_type, ind))
    FREE(MSG_HEADING(board_type, ind));

  for (; ind < num_of_msgs[board_type] - 1; ind++)
  {
    MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
    MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
    MSG_LEVEL(board_type, ind) = MSG_LEVEL(board_type, ind + 1);
  }
  num_of_msgs[board_type]--;
  send_to_char("Message removed.\r\n", ch);
  sprintf(buf, "$n just removed message %d.", msg);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  Board_save_board(board_type);

  return 1;
}


void Board_save_board(int board_type)
{
  FILE    *fl;
  int      i;
  char    *tmp1 = 0, *tmp2 = 0;

  if (!num_of_msgs[board_type])
  {
    fprintf(stderr, "number of messages is 0! NOT saving!\r\n");
    unlink(FILENAME(board_type));
    return;
  }
  if (!(fl = fopen(FILENAME(board_type), "wb")))
  {
    fprintf(stderr, "ERROR! Could not open board file!");
    return;
  }
  fwrite(&(num_of_msgs[board_type]), sizeof(int), 1, fl);

  for (i = 0; i < num_of_msgs[board_type]; i++)
  {
    if ((tmp1 = MSG_HEADING(board_type, i)))
      msg_index[board_type][i].heading_len = strlen(tmp1) + 1;
    else
      msg_index[board_type][i].heading_len = 0;

    if (MSG_SLOTNUM(board_type, i) < 0 ||
        MSG_SLOTNUM(board_type, i) >= INDEX_SIZE ||
        (!(tmp2 = msg_storage[MSG_SLOTNUM(board_type, i)])))
      msg_index[board_type][i].message_len = 0;
    else
      msg_index[board_type][i].message_len = strlen(tmp2) + 1;

    fwrite(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
    if (tmp1)
      fwrite(tmp1, sizeof(char),
             (unsigned) msg_index[board_type][i].heading_len, fl);
    if (tmp2)
      fwrite(tmp2, sizeof(char),
             (unsigned) msg_index[board_type][i].message_len, fl);
  }

  fclose(fl);
}


void Board_load_board(int board_type)
{
  FILE    *fl;
  int      i, len1 = 0, len2 = 0;
  char    *tmp1 = NULL, *tmp2 = NULL;

  if (!(fl = fopen(FILENAME(board_type), "rb")))
  {
    return;
  }
  fread(&(num_of_msgs[board_type]), sizeof(int), 1, fl);
  if (num_of_msgs[board_type] < 1 ||
      num_of_msgs[board_type] > MAX_BOARD_MESSAGES)
  {
    logit(LOG_BOARD, " Board file corrupt.  Resetting.");
    Board_reset_board(board_type);
    return;
  }
  for (i = 0; i < num_of_msgs[board_type]; i++)
  {
    fread(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
    if (!(len1 = msg_index[board_type][i].heading_len))
    {
      logit(LOG_BOARD, " Board file corrupt!  Resetting.");
      Board_reset_board(board_type);
      return;
    }
    CREATE(tmp1, char, len1, MEM_TAG_STRING);
    if (!tmp1)
    {
      logit(LOG_BOARD, " Error - malloc failed for board header");
      exit(1);
    }
    fread(tmp1, sizeof(char), (unsigned) len1, fl);
    MSG_HEADING(board_type, i) = tmp1;

    if ((len2 = msg_index[board_type][i].message_len))
    {
      if ((MSG_SLOTNUM(board_type, i) = find_slot()) == -1)
      {
        logit(LOG_BOARD, " Out of slots booting board!  Resetting...");
        Board_reset_board(board_type);
        return;
      }
      CREATE(tmp2, char, len2, MEM_TAG_STRING);
      if (!tmp2)
      {
        logit(LOG_BOARD, " malloc failed for board text");
        exit(1);
      }
      fread(tmp2, sizeof(char), (unsigned) len2, fl);
      msg_storage[MSG_SLOTNUM(board_type, i)] = tmp2;
    }
  }

  fclose(fl);
}


void Board_reset_board(int board_type)
{
  int      i;

  for (i = 0; i < MAX_BOARD_MESSAGES; i++)
  {
    if (MSG_HEADING(board_type, i))
      FREE(MSG_HEADING(board_type, i));
    if (msg_storage[MSG_SLOTNUM(board_type, i)])
      FREE(msg_storage[MSG_SLOTNUM(board_type, i)]);
    msg_storage_taken[MSG_SLOTNUM(board_type, i)] = 0;
    memset((char *) &(msg_index[board_type][i]), 0,
           sizeof(struct board_msginfo));
    msg_index[board_type][i].slot_num = -1;
  }
  num_of_msgs[board_type] = 0;
  unlink(FILENAME(board_type));
}
