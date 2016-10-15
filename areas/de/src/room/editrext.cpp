//
//  File: editrext.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing room exits
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



#include <ctype.h>

#include "../fh.h"
#include "../types.h"

#include "../graphcon.h"

#include "room.h"
#include "exit.h"

#include "../misc/menu.h"



extern bool g_madeChanges;
extern const char *g_exitnames[];
extern const char *g_exitkeys[];
extern uint g_exitflags[];
extern menu g_roomExitMenu;


//
// displayEditRoomExitsRedundant
//

inline void displayEditRoomExitsRedundant(const room *room, const usint exitDir, uint *exitFlagsVal)
{
  if (room->exits[exitDir])
  {
    char strn[256];

    *exitFlagsVal |= g_exitflags[exitDir];

    sprintf(strn, "   &+Y%s&+L. &+wEdit %s exit\n", g_exitkeys[exitDir], g_exitnames[exitDir]);

    displayColorString(strn);
  }
}


//
// displayEditRoomExitsMenu : displays the edit exits menu for rooms
//
//       *room : pointer to room being edited
//  *exitFlags : "exit flags" - specify which exits are editable
//

void displayEditRoomExitsMenu(const room *room, uint *exitFlagsVal)
{
 // display title

  displayMenuHeader(ENTITY_ROOM, room->roomName, room->roomNumber, "exits", false);

  *exitFlagsVal = 0;

  displayEditRoomExitsRedundant(room, NORTH, exitFlagsVal);
  displayEditRoomExitsRedundant(room, NORTHWEST, exitFlagsVal);
  displayEditRoomExitsRedundant(room, NORTHEAST, exitFlagsVal);
  displayEditRoomExitsRedundant(room, SOUTH, exitFlagsVal);
  displayEditRoomExitsRedundant(room, SOUTHWEST, exitFlagsVal);
  displayEditRoomExitsRedundant(room, SOUTHEAST, exitFlagsVal);
  displayEditRoomExitsRedundant(room, WEST, exitFlagsVal);
  displayEditRoomExitsRedundant(room, EAST, exitFlagsVal);
  displayEditRoomExitsRedundant(room, UP, exitFlagsVal);
  displayEditRoomExitsRedundant(room, DOWN, exitFlagsVal);

  if (*exitFlagsVal == 0)
    displayColorString("  &nThis room has no exits.\n");

 // display the rest of the stuff

  displayMenu(&g_roomExitMenu, room);
}


//
// interpEditRoomExitsMenu : Interprets user input - returns TRUE if user
//                           hit 'Q', FALSE otherwise
//
//          ch : user input
//       *room : room record associated with extra descs being edited
//  *exitFlags : "exit flags" - specifies which exits are editable
//

bool interpEditRoomExitsMenu(const usint ch, room *room, uint *exitFlagsVal)
{
  struct rccoord coords;
  const bool blnAlterNumbExits = !room->defaultRoom;


  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  if ((ch == 'Y') && *exitFlagsVal)
  {
    coords = _gettextposition();

    _settextposition(coords.row - 1, 1);

    deleteExitPromptPrompt(room, *exitFlagsVal, blnAlterNumbExits);

    displayEditRoomExitsMenu(room, exitFlagsVal);
  }
  else

  if ((ch == 'Z') && (*exitFlagsVal != EXIT_ALL_EXITS_FLAG))
  {
    coords = _gettextposition();

    _settextposition(coords.row - 1, 1);

    createRoomExitPrompt(room, *exitFlagsVal, blnAlterNumbExits);

    displayEditRoomExitsMenu(room, exitFlagsVal);
  }
  else

 // check all exit keys

  {
    uint i;

    for (i = 0; i < NUMB_EXITS; i++)
    {
      if ((ch == g_exitkeys[i][0]) && ((*exitFlagsVal) & g_exitflags[i]))
      {
        editExit(room, &(room->exits[i]), g_exitnames[i], false);

        displayEditRoomExitsMenu(room, exitFlagsVal);

        break;
      }
    }
  }


  return false;
}


//
// editRoomExits : The "main" function for the edit room exits menu -
//                 lists all the exits, etc.
//
//   *room : room being edited
//

void editRoomExits(room *roomPtr)
{
  usint ch;
  uint exitFlagsVal;

  room *newRoom;

  const bool origMadeChanges = g_madeChanges;
  const bool blnUpdateNumbExits = !roomPtr->defaultRoom;


  newRoom = new(std::nothrow) room;

  if (!newRoom)
  {
    displayAllocError("room", "editRoomExits");

    return;
  }

  memcpy(newRoom, roomPtr, sizeof(room));

  copyAllExits(roomPtr, newRoom, blnUpdateNumbExits);

  displayEditRoomExitsMenu(newRoom, &exitFlagsVal);


  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      deleteAllExits(newRoom, blnUpdateNumbExits);

      delete newRoom;

      _outtext("\n\n");

      g_madeChanges = origMadeChanges;

      return;
    }

    if (interpEditRoomExitsMenu(ch, newRoom, &exitFlagsVal))
    {
      if (!compareRoomInfo(roomPtr, newRoom))
      {
        deleteAllExits(roomPtr, blnUpdateNumbExits);

        memcpy(roomPtr, newRoom, sizeof(room));

        delete newRoom;

        g_madeChanges = true;
      }
      else 
      {
        deleteAllExits(newRoom, blnUpdateNumbExits);

        delete newRoom;

        g_madeChanges = origMadeChanges;
      }

      _outtext("\n\n");

      return;
    }
  }
}
