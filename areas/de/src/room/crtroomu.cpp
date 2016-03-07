//
//  File: crtroomu.cpp   originally part of durisEdit
//
//  Usage: user-interface end functions for creating rooms from prompt
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
#include <stdlib.h>
#include <ctype.h>

#include "types.h"
#include "fh.h"
#include "keys.h"
#include "room.h"

extern room *g_currentRoom;
extern uint g_numbLookupEntries;
extern bool g_madeChanges;
extern char *g_exitnames[];
extern char g_revdirs[];

//
// createRoomPrompt : This function is poorly named, so sue me.
//
//                    At any rate, it creates a new room record, tacks it onto
//                    the end of the list pointed to by roomHead, creates an
//                    exit leading to the new room from g_currentRoom, and
//                    optionally creates an exit loading from the new room to
//                    g_currentRoom.
//
//                    returns FALSE if didn't create, TRUE if did
//
//   args : user input
//

bool createRoomPrompt(const char *args)
{
  usint ch, oldch;
  uint exitTaken;
  char strn[512];
  roomExit **exitPtr;
  room *roomNew;
  uint numb = 0;
  bool getFreeNumb;


  if (strlen(args))
  {
    if (!strnumer(args))
    {
      _outtext("\nError in vnum argument - non-numerics in input.\n\n");
      return false;
    }

    numb = strtoul(args, NULL, 10);

    if (roomExists(numb))
    {
      sprintf(strn, "\n"
"Cannot create a room with the vnum %u - a room with this vnum already exists.\n\n",
              numb);
      _outtext(strn);

      return false;
    }

    getFreeNumb = false;
  }
  else
  {
    getFreeNumb = true;
  }

  exitTaken = getExitTakenFlags(g_currentRoom);

 // based on value of exitTaken, do stuff

  strcpy(strn, "\n&+cWhich direction should the new room be from this one:\n");

  buildCommonDisplayExitsStrn(strn, exitTaken, false);

  strcat(strn, "&+c(&+CQ to quit, X for none&+c)? &n");

  displayColorString(strn);

  do
  {
    ch = toupper(getkey());
  } while (((ch != 'N') || ((exitTaken & EXIT_NORTH_FLAG))) &&
           ((ch != '1') || ((exitTaken & EXIT_NORTHWEST_FLAG))) &&
           ((ch != '2') || ((exitTaken & EXIT_NORTHEAST_FLAG))) &&
           ((ch != 'S') || ((exitTaken & EXIT_SOUTH_FLAG))) &&
           ((ch != '3') || ((exitTaken & EXIT_SOUTHWEST_FLAG))) &&
           ((ch != '4') || ((exitTaken & EXIT_SOUTHEAST_FLAG))) &&
           ((ch != 'W') || ((exitTaken & EXIT_WEST_FLAG))) &&
           ((ch != 'E') || ((exitTaken & EXIT_EAST_FLAG))) &&
           ((ch != 'U') || ((exitTaken & EXIT_UP_FLAG))) &&
           ((ch != 'D') || ((exitTaken & EXIT_DOWN_FLAG))) &&
            (ch != 'Q') && (ch != 'X'));

  switch (ch)
  {
    case 'N' : ch = NORTH;  break;
    case '1' : ch = NORTHWEST;  break;
    case '2' : ch = NORTHEAST;  break;
    case 'S' : ch = SOUTH;  break;
    case '3' : ch = SOUTHWEST;  break;
    case '4' : ch = SOUTHEAST;  break;
    case 'W' : ch = WEST;  break;
    case 'E' : ch = EAST;  break;
    case 'U' : ch = UP;  break;
    case 'D' : ch = DOWN;  break;
    default :
    case 'Q' : _outtext("quit\n\n");  return false;
    case 'X' : break;
  }

  if (ch != 'X')
  {
    exitPtr = &(g_currentRoom->exits[ch]);
    _outtext(g_exitnames[ch]);
  }
  else 
  {
    exitPtr = NULL;
    _outtext("no exit");
  }

 // create the room

  roomNew = createRoom(true, numb, getFreeNumb);
  if (!roomNew)
    exit(1);

 // create the exit and set stuff - must be done here

  if (exitPtr)
  {
    createExit(exitPtr, true);
    (*exitPtr)->destRoom = roomNew->roomNumber;
  }

 // now do opposite exit - don't allow them to hit Enter for opposite
 // exit if no exit entered in first direction

  strcpy(strn,
  "\n\n"
  "&+cWhich direction should lead to this room from the new room:\n");

  buildCommonDisplayExitsStrn(strn, 0, false);

  displayColorString(strn);

  if (ch != 'X')
  {
    displayColorString("(&+CEnter=opposite/X=none/Q=quit&+c)? &n");
  }
  else
  {
    displayColorString("(&+LEnter=opposite&+C/X=none/Q=quit&+c)? &n");
  }

  oldch = ch;  // save the previous exit choice for opposite exit choice (Enter)

  while (true)
  {
    ch = toupper(getkey());

    if ((ch == 'N') || (ch == 'S') || (ch == 'W') || (ch == 'E') ||
        (ch == 'U') || (ch == 'D') || (ch == '1') || (ch == '2') ||
        (ch == '3') || (ch == '4') || ((oldch != 'X') && (ch == K_Enter)) ||
        (ch == 'X') || (ch == 'Q'))
      break;
  }

 // work some magic for Enter (opposite)

  if (ch == K_Enter)
  {
    ch = g_revdirs[oldch];
  }
  else
  {
    switch (ch)
    {
      case 'N' : ch = NORTH;  break;
      case '1' : ch = NORTHWEST;  break;
      case '2' : ch = NORTHEAST;  break;
      case 'S' : ch = SOUTH;  break;
      case '3' : ch = SOUTHWEST;  break;
      case '4' : ch = SOUTHEAST;  break;
      case 'W' : ch = WEST;  break;
      case 'E' : ch = EAST;  break;
      case 'U' : ch = UP;  break;
      case 'D' : ch = DOWN;  break;
      case 'X' : break;
      default :
      case 'Q' : deleteRoomInfo(roomNew, true, true);

                 if (exitPtr && *exitPtr) 
                 {
                   deleteRoomExit(*exitPtr, true);
                   *exitPtr = NULL;
                 }

                 _outtext("quit\n\n");
                 return false;
    }
  }

  if (ch != 'X')
  {
    exitPtr = &(roomNew->exits[ch]);

   // there may be an exit if the new room was created from a default room with exits

    if (*exitPtr)
      deleteRoomExit(*exitPtr, true);

    createExit(exitPtr, true);

    (*exitPtr)->destRoom = g_currentRoom->roomNumber;

    _outtext(g_exitnames[ch]);
  }
  else
  {
    _outtext("no exit");
  }

  g_madeChanges = true;

  createPrompt();  // update the prompt

  sprintf(strn, "\n\nRoom #%u '%s&n' created.\n\n", roomNew->roomNumber, roomNew->roomName);
  displayColorString(strn);

  return true;
}
