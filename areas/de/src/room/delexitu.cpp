//
//  File: delexitu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for deleting exits
//

/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <string.h>
#include <ctype.h>

#include "../types.h"
#include "../fh.h"

#include "room.h"
#include "exit.h"

extern bool g_madeChanges;
extern const char *g_exitnames[];
extern room *g_currentRoom;


//
// deleteExitPromptPrompt : deletes an exit from a specified room, based
//                          on user input and exit info in exitAvail bitvect
//
//       room : room to delete exit from
//  exitAvail : specifies which exits exist in room
//

void deleteExitPromptPrompt(room *room, const uint exitTaken, const bool decLoaded)
{
  char strn[256], exitDel;
  usint ch;


  strcpy(strn, "\n&+cDelete ");

  buildCommonDisplayExitsStrn(strn, exitTaken, true);

  strcat(strn, "&+c(&+CQ to quit&+c)? &n");

  displayColorString(strn);

  do
  {
    ch = toupper(getkey());
  } while (((ch != 'N') || (!(exitTaken & EXIT_NORTH_FLAG))) &&
           ((ch != '1') || (!(exitTaken & EXIT_NORTHWEST_FLAG))) &&
           ((ch != '2') || (!(exitTaken & EXIT_NORTHEAST_FLAG))) &&
           ((ch != 'S') || (!(exitTaken & EXIT_SOUTH_FLAG))) &&
           ((ch != '3') || (!(exitTaken & EXIT_SOUTHWEST_FLAG))) &&
           ((ch != '4') || (!(exitTaken & EXIT_SOUTHEAST_FLAG))) &&
           ((ch != 'W') || (!(exitTaken & EXIT_WEST_FLAG))) &&
           ((ch != 'E') || (!(exitTaken & EXIT_EAST_FLAG))) &&
           ((ch != 'U') || (!(exitTaken & EXIT_UP_FLAG))) &&
           ((ch != 'D') || (!(exitTaken & EXIT_DOWN_FLAG))) && (ch != 'Q'));

  switch (ch)
  {
    case 'N' : exitDel = NORTH;  break;
    case '1' : exitDel = NORTHWEST;  break;
    case '2' : exitDel = NORTHEAST;  break;
    case 'S' : exitDel = SOUTH;  break;
    case '3' : exitDel = SOUTHWEST;  break;
    case '4' : exitDel = SOUTHEAST;  break;
    case 'W' : exitDel = WEST;  break;
    case 'E' : exitDel = EAST;  break;
    case 'U' : exitDel = UP;  break;
    case 'D' : exitDel = DOWN;  break;
    case 'Q' : break;

    default : return;
  }

  if (ch != 'Q') 
  {
    deleteRoomExit(room->exits[exitDel], decLoaded);
    room->exits[exitDel] = NULL;

    _outtext(g_exitnames[exitDel]);

    sprintf(strn, "\n%s exit deleted.\n\n", g_exitnames[exitDel]);
    strn[1] = toupper(strn[1]);

    _outtext(strn);
  }
  else
  {
    _outtext("quit\n\n");
  }
}


//
// deleteExitPrompt : User interface to delete an exit from current room - this
//                    function deletes exit specified if dir isn't USER_CHOICE
//                    and calls deleteExitPromptPrompt() otherwise
//
//        dir : direction to delete or USER_CHOICE
//

void deleteExitPrompt(const char dir)
{
  uint exitTaken;
  char strn[256], outstrn[256];


  if (dir != USER_CHOICE)
  {
    if (g_currentRoom->exits[dir])
    {
      deleteRoomExit(g_currentRoom->exits[dir], true);
      g_currentRoom->exits[dir] = NULL;

      strcpy(strn, g_exitnames[dir]);
      strn[0] = toupper(strn[0]);

      sprintf(outstrn, "\n%s exit deleted.\n\n", strn);
      _outtext(outstrn);
    }
    else 
    {
      _outtext("\nNo exit exists in that direction.\n\n");
    }

    return;
  }

  exitTaken = getExitTakenFlags(g_currentRoom);

 // no exits exist

  if (exitTaken == 0)
  {
    _outtext("\nThis room has no exits.\n\n");

    return;
  }

  deleteExitPromptPrompt(g_currentRoom, exitTaken, true);
}


//
// preDeleteExitPrompt : parses user input for deleteExitPrompt() - if
//                       not a valid exit direction, returns
//
//      args : user input
//

void preDeleteExitPrompt(const char *args)
{
  char val;


  if (!strlen(args)) 
  {
    deleteExitPrompt(USER_CHOICE);
  }
  else
  {
    val = getDirfromKeyword(args);
    if (val == NO_EXIT)
    {
      _outtext("\nDelete which exit?\n\n");
      return;
    }

    deleteExitPrompt(val);
  }
}
