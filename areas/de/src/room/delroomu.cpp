//
//  File: delroomu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions to delete rooms
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


#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "types.h"
#include "fh.h"
#include "keys.h"

#include "room.h"

extern room *g_currentRoom;
extern uint g_numbRooms, g_lowestRoomNumber, g_highestRoomNumber;


//
// deleteRoomUser : interprets arguments passed by user to command -
//                  if no input, assumes current room, otherwise interprets
//                  args as vnum of room to delete
//
//     args : user input
//

void deleteRoomUser(const char *args)
{
  char outStrn[MAX_ROOMNAME_LEN + 128];
  room *roomPtr;
  uint numb;
  promptType userChoice;


  if (g_numbRooms == 1)
  {
    _outtext(
"\nYou cannot delete any rooms - there is only one in the zone.\n\n");

    return;
  }

 // delete g_currentRoom

  if (strlen(args) == 0)
  {
    userChoice = displayYesNoPrompt("\n&+cDelete current room", promptNo, false);

    numb = g_currentRoom->roomNumber;
    roomPtr = g_currentRoom;
  }
  else
  {
    if (!strnumer(args))
    {
      _outtext("\nError in input - specify a room number as the argument.\n\n");

      return;
    }

    numb = strtoul(args, NULL, 10);
    roomPtr = findRoom(numb);

    if (!roomPtr)
    {
      sprintf(outStrn, "\nRoom #%u does not exist.\n\n", numb);

      _outtext(outStrn);

      return;
    }

    sprintf(outStrn, "\n&+cDelete room #%u, &+L\"&n%s&+L\"&+c",
            numb, roomPtr->roomName);

    userChoice = displayYesNoPrompt(outStrn, promptNo, false);
  }

  if (userChoice == promptYes)
  {
    if (roomPtr == g_currentRoom)
    {
      if (roomPtr->roomNumber == getLowestRoomNumber())
        g_currentRoom = getNextRoom(roomPtr);
      else
        g_currentRoom = findRoom(getLowestRoomNumber());
    }

    const uint oldRoomNumb = roomPtr->roomNumber;

    deleteRoomInfo(roomPtr, true, true);

    displayColorString(
"&+cExits leading to deleted room: &+Cs&+cet dest to -1, &+Cd&+celete,"
" or &+Cn&+ceither (&+Cs/D/n&+c)? &n");

    usint ch;

    do
    {
      ch = toupper(getkey());
    } while ((ch != 'S') && (ch != 'D') && (ch != 'N') && (ch != K_Enter));

    if (ch == 'S')
    {
      _outtext("set to -1\n\n");

      resetExits(numb, -1);
    }
    else
    if (ch == 'N')
    {
      _outtext("neither\n\n");
    }
    else  // d or enter
    {
      _outtext("delete\n\n");

      clearExits(numb, true);
    }
  }
}
