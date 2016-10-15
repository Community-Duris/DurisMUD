//
//  File: editexit.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing exits
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

#include "../types.h"
#include "../fh.h"

#include "../graphcon.h"

#include "../misc/menu.h"
#include "../dispflag.h"
#include "editexit.h"
#include "room.h"
#include "exit.h"

extern bool g_madeChanges;
extern const char *g_exitnames[];
extern menu g_exitMenu, g_exitStateTopMenu, g_exitStateBottomMenu;
extern room *g_currentRoom;


//
// getExitStateMenuStrn : redundant code used by 'edit exit state' menu
//

void getExitStateMenuStrn(char *strn, const char ch, const char *descStrn, const bool active)
{
  const char *preLetter, *postLetter, *postLetter2, *postLetter3;

  if (active || getEditUneditableFlagsVal())
  {
    preLetter = "&+Y";
    postLetter = "&+L";
    postLetter2 = "&+c";
    postLetter3 = "&+w";
  }
  else
  {
    preLetter = "&+L";
    postLetter = "";
    postLetter2 = "";
    postLetter3 = "";
  }

  sprintf(strn, "   %s%c%s. %s(.zon) %s%s\n",
          preLetter, ch, postLetter, postLetter2, postLetter3, descStrn);
}


//
// displayEditExitStateMenu : Displays the menu that allows you to edit exit
//                            state
//
//      *room : room that contains the exit
//  *exitNode : exit node being edited
//  *exitName : "name" of exit - direction it leads
//

void displayEditExitStateMenu(const room *room, const roomExit *exitNode, const char *exitName)
{
  char strn[256];
  struct rccoord coords;
  int i;


  sprintf(strn, "exit state for %s exit in", exitName);

  displayMenuHeaderBasic(strn, room->roomName);

  displayMenuNoFooter(&g_exitStateTopMenu, exitNode);

 // only allow user to make door closed and locked if there is an actual door (key for locked)

  getExitStateMenuStrn(strn, 'G', "Set door state to \"open\"", (exitNode->worldDoorType & 3) != 0);
  displayColorString(strn);

  getExitStateMenuStrn(strn, 'H', "Set door state to \"closed\"", (exitNode->worldDoorType & 3) != 0);
  displayColorString(strn);

  getExitStateMenuStrn(strn, 'I', "Set door state to \"closed and locked\"", 
                       ((exitNode->worldDoorType & 3) >= 2));
  displayColorString(strn);

  displayMenu(&g_exitStateBottomMenu, exitNode);

  coords = _gettextposition();

 // check world door type

  for (i = 3; i >= 0; i--)
  {
    if ((exitNode->worldDoorType & i) == i)
    {
      _settextposition(WORLD_FLAG_STARTY + (sint)i, WORLD_FLAGX);

      displayColorString(FLAG_ACTIVE_STR);

      break;
    }
  }

 // check zone door state

  for (i = 2; i >= 1; i--)
  {
    if ((exitNode->zoneDoorState & i) == i)
    {
      _settextposition(ZONE_FLAG_STARTY + (sint)i, ZONE_FLAGX);

      displayColorString(FLAG_ACTIVE_STR);

      break;
    }
  }

 // only show 'door open' state if there is a door type or if user can edit uneditable flags

  if (((exitNode->zoneDoorState & 3) == 0) && ((exitNode->worldDoorType & 3) || getEditUneditableFlagsVal()))
  {
    _settextposition(ZONE_FLAG_STARTY, ZONE_FLAGX);

    displayColorString(FLAG_ACTIVE_STR);
  }

 // next, the secret/blocked bits for the world door type

  if (exitNode->worldDoorType & 4)  // secret state
  {
    _settextposition(WORLD_FLAG_STARTY + 5, WORLD_FLAGX);

    displayColorString(FLAG_ACTIVE_STR);
  }

  if (exitNode->worldDoorType & 8)  // blocked state
  {
    _settextposition(WORLD_FLAG_STARTY + 6, WORLD_FLAGX);

    displayColorString(FLAG_ACTIVE_STR);
  }

 // next, the secret/blocked bits for the zone door state

  if (exitNode->zoneDoorState & 4)  // secret state
  {
    _settextposition(ZONE_FLAG_STARTY + 4, ZONE_FLAGX);

    displayColorString(FLAG_ACTIVE_STR);
  }

  if (exitNode->zoneDoorState & 8)  // blocked state
  {
    _settextposition(ZONE_FLAG_STARTY + 5, ZONE_FLAGX);

    displayColorString(FLAG_ACTIVE_STR);
  }

  _settextposition(coords.row, coords.col);
}


//
// interpEditExitStateMenu : Interprets user input - returns TRUE if user hit
//                           'Q', FALSE otherwise
//
//         ch : user input
//  *exitNode : exit node being edited
//

bool interpEditExitStateMenu(const usint ch, const room *room, roomExit *exitNode, const char *exitName)
{
  struct rccoord coords = _gettextposition();

  int oldWorldDoorBits = (exitNode->worldDoorType & 12),
      oldWorldDoorType = (exitNode->worldDoorType & 3),
      oldZoneDoorBits = (exitNode->zoneDoorState & 12),
      oldZoneDoorState = (exitNode->zoneDoorState & 3);


  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

 // door type

 // set exit type to no door

  if (ch == 'A')
  {
    exitNode->worldDoorType = 0 | oldWorldDoorBits;

   // clear any close/lock state

    if (oldZoneDoorState != 0)
      exitNode->zoneDoorState = oldZoneDoorBits;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // set exit type to door (no key)

  if (ch == 'B')
  {
    exitNode->worldDoorType = 1 | oldWorldDoorBits;

   // only the closed state is valid

    if (oldZoneDoorState == 2)
      exitNode->zoneDoorState = 1 | oldZoneDoorBits;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // set exit type to door (key)

  if (ch == 'C')
  {
    exitNode->worldDoorType = 2 | oldWorldDoorBits;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // set exit type to unpickable door (key)

  if (ch == 'D')
  {
    exitNode->worldDoorType = 3 | oldWorldDoorBits;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // toggle secret bit

  if (ch == 'E')
  {
    exitNode->worldDoorType ^= 4;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // toggle blocked bit

  if (ch == 'F')
  {
    exitNode->worldDoorType ^= 8;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // zone exit state

 // set exit state to open

  if (ch == 'G')
  {
    exitNode->zoneDoorState = 0 | oldZoneDoorBits;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // set exit state to closed, but only if there actually is a door

  if ((ch == 'H') && (oldWorldDoorType || getEditUneditableFlagsVal()))
  {
    exitNode->zoneDoorState = 1 | oldZoneDoorBits;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // set exit state to closed and locked, but only if the door requires a key

  if ((ch == 'I') && ((oldWorldDoorType > 1) || getEditUneditableFlagsVal()))
  {
    exitNode->zoneDoorState = 2 | oldZoneDoorBits;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // toggle secret bit

  if (ch == 'J')
  {
    exitNode->zoneDoorState ^= 4;

    displayEditExitStateMenu(room, exitNode, exitName);
  }
  else

 // toggle blocked bit

  if (ch == 'K')
  {
    exitNode->zoneDoorState ^= 8;

    displayEditExitStateMenu(room, exitNode, exitName);
  }


  return false;
}


//
// editExitState : "Main" function for the "edit exit state" menu.
//
//      *room : room that contains exit being edited
//  *exitNode : exit being edited
//  *exitName : exit "name" - direction exit leads
//

void editExitState(const room *room, roomExit *exitNode, const char *exitName)
{
  usint ch;
  int oldWorldDoorType = exitNode->worldDoorType,
      oldZoneDoorState = exitNode->zoneDoorState;


  displayEditExitStateMenu(room, exitNode, exitName);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      exitNode->worldDoorType = oldWorldDoorType;
      exitNode->zoneDoorState = oldZoneDoorState;

      _outtext("\n\n");

      return;
    }

    if (interpEditExitStateMenu(ch, room, exitNode, exitName))
    {
      return;
    }
  }
}


//
// displayEditExitMenu : Displays the edit exit menu
//
//       *room : pointer to room that contains the exit
//   *exitName : direction exit is heading in a nice ASCII string
//

void displayEditExitMenu(const room *room, const char *exitName, const roomExit *exitNode)
{
  char exitStrn[64];


 // display menu

  sprintf(exitStrn, "%s exit", exitName);

  displayMenuHeader(ENTITY_ROOM, room->roomName, room->roomNumber, exitStrn, false);

  displayMenu(&g_exitMenu, exitNode);
}


//
// interpEditExitMenu : Interprets user input - returns TRUE if user hit 'Q',
//                      FALSE otherwise
//
//         ch : user input
//      *room : room that contains the exit
//  *exitNode : exit node being edited
//  *exitName : "name" of exit
//

bool interpEditExitMenu(const usint ch, const room *room, roomExit *exitNode, const char *exitName)
{
  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

 // edit exit door state

  if (ch == 'C')
  {
    editExitState(room, exitNode, exitName);

    displayEditExitMenu(room, exitName, exitNode);
  }
  else

 // edit exit key object number

  if (ch == 'D')
  {
    while (true)
    {
      int val = exitNode->keyNumb;

      editIntValSearchableList(&val, true, "key vnum", displayObjectTypeList);

      if (!getVnumCheckVal() || objExists(val) || (val == -1) || (val == -2) || (val == 0))
      {
        exitNode->keyNumb = val;

        displayEditExitMenu(room, exitName, exitNode);
        break;
      }
    }
  }
  else

 // edit exit destination room

  if (ch == 'E')
  {
    while (true)
    {
      int val = exitNode->destRoom;

      editIntValSearchableList(&val, true, "room vnum", displayRoomList);

      if (!getVnumCheckVal() || roomExists(val) || (val == -1))
      {
        exitNode->destRoom = val;

        displayEditExitMenu(room, exitName, exitNode);
        break;
      }
    }
  }
  else
 
 // the rest

  if (interpretMenu(&g_exitMenu, exitNode, ch))
  {
    displayEditExitMenu(room, exitName, exitNode);

    return false;
  }

  return false;
}


//
// editExit : The "main" function for the edit exit menu
//
//       *room : room that contains exit being edited
//  **exitNode : pointer to a pointer to the exit being edited
//   *exitName : exit "name" - one of the cardinal directions
//

void editExit(room *room, roomExit **exitNode, const char *exitName, const bool updateMadeChanges)
{
  uchar ch;
  roomExit *newExit;


  if (!*exitNode)
    return;

  newExit = copyRoomExit(*exitNode, false);

  if (!newExit)
  {
    displayAllocError("exitNode", "editExit");

    return;
  }

  displayEditExitMenu(room, exitName, *exitNode);


  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      deleteRoomExit(newExit, false);

      _outtext("\n\n");

      return;
    }

    if (interpEditExitMenu(ch, room, newExit, exitName))
    {
      if (!compareRoomExits(*exitNode, newExit))
      {
        deleteRoomExit(*exitNode, false);
        *exitNode = newExit;

        if (updateMadeChanges) 
          g_madeChanges = true;
      }
      else 
      {
        deleteRoomExit(newExit, false);
      }

      _outtext("\n\n");

      return;
    }
  }
}


//
// preEditExit
//

void preEditExit(const char *args)
{
  int val;

  if (!strlen(args)) 
  {
    editRoomExits(g_currentRoom);
  }
  else
  {
    val = getDirfromKeyword(args);
    if (val == NO_EXIT)
    {
      _outtext("\nThat is not a valid direction.\n\n");
      return;
    }

    if (!g_currentRoom->exits[val])
    {
      _outtext("\nNo exit exists in that direction.\n\n");
    }
    else
    {
      editExit(g_currentRoom, &g_currentRoom->exits[val], g_exitnames[val], true);
    }
  }
}
