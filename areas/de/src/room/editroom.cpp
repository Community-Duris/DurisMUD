//
//  File: editroom.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing rooms
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
#include <ctype.h>
#include <string.h>

#include "../fh.h"
#include "../types.h"

#include "../graphcon.h"

#include "room.h"
#include "../misc/menu.h"
#include "../keys.h"

extern room **g_roomLookup, *g_currentRoom;
extern bool g_madeChanges;
extern uint g_lowestRoomNumber, g_highestRoomNumber, g_numbRooms, g_numbLookupEntries;
extern flagDef g_roomManaList[], g_roomSectList[];
extern uint g_numbExits;
extern menu g_roomMenu;


//
// displayEditRoomMenu : Displays edit room menu
//
//   *roomPtr : room being edited
//

void displayEditRoomMenu(const room *roomPtr)
{
  displayMenuHeader(ENTITY_ROOM, roomPtr->roomName, roomPtr->roomNumber, NULL, false);

  displayMenu(&g_roomMenu, roomPtr);
}


//
// interpEditRoomMenu : Interprets user input for edit room menu - returns
//                      TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//  *room : room to edit
//

bool interpEditRoomMenu(const usint ch, room *roomPtr, const uint origVnum)
{
 // edit room misc. stuff

  if (ch == 'G')
  {
    editRoomMisc(roomPtr);

    displayEditRoomMenu(roomPtr);
  }
  else

 // hit menu for most

  if (interpretMenu(&g_roomMenu, roomPtr, ch))
  {
    displayEditRoomMenu(roomPtr);

    return false;
  }
  else

 // change vnum (don't allow them to try to change vnum if this is default room)

  if ((ch == 'V') && !roomPtr->defaultRoom)
  {
    const struct rccoord coords = _gettextposition();
    uint vnum;
    char strn[256];


    sprintf(strn, "&+CNew vnum (highest allowed %u): ",
            g_numbLookupEntries - 1);

    while (true)
    {
      vnum = roomPtr->roomNumber;

      editUIntVal(&vnum, false, strn);

     // check user input

      if ((vnum >= g_numbLookupEntries) || (roomExists(vnum) && (origVnum != vnum)))
      {
        if (vnum != roomPtr->roomNumber)
        {
          clrline(coords.row, 7, 0);
          _settextposition(coords.row, 1);

          displayColorString(
  "&+CError: vnum already exists or is too high - press a key");

          getkey();
        }
      }
      else
      {
        break;
      }
    }

    roomPtr->roomNumber = vnum;

    displayEditRoomMenu(roomPtr);
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editRoom : "main" function for edit room menu
//
//  *room : room being edited
//

//
// for rooms, a copy is made and then edited and if changes are kept, the copy is copied into the original
// objects and mobs edit the original and if changes are not kept, the copy is copied back into the original
//
// rooms are done differently because they use the method I originally implemented.  objects and mobs were
// changed because that method makes it easier to check if all eq is still valid on mobs.  rooms don't have
// this issue so I see no reason to bother to change them.  enjoy
//

room *editRoom(room *roomPtr, const bool allowJump)
{
  room *newRoom;
  bool done = false;
  const bool origMadeChanges = g_madeChanges;
  const bool blnUpdateNumbExits = !roomPtr->defaultRoom;


  if (!roomPtr) 
    return NULL;

 // newRoom is working room - duplicate exits, extra descs, and description, leave everything else

  newRoom = new(std::nothrow) room;
  if (!newRoom)
  {
    displayAllocError("room", "editRoom");

    return NULL;
  }

  memcpy(newRoom, roomPtr, sizeof(room));

  copyAllExits(roomPtr, newRoom, blnUpdateNumbExits);
  newRoom->extraDescHead = copyExtraDescs(roomPtr->extraDescHead);
  newRoom->roomDescHead = copyStringNodes(roomPtr->roomDescHead);

  displayEditRoomMenu(newRoom);

  while (true)
  {
    const usint ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      deleteAllExits(newRoom, blnUpdateNumbExits);
      deleteExtraDescs(newRoom->extraDescHead);
      deleteStringNodes(newRoom->roomDescHead);

      delete newRoom;

      g_madeChanges = origMadeChanges;  // deleteAllExits call alters g_madeChanges

      _outtext("\n\n");

      return NULL;
    }

    if (allowJump)
    {
      if ((checkMenuKey(ch, false) == MENUKEY_NEXT) || (checkMenuKey(ch, false) == MENUKEY_PREV) ||
          (checkMenuKey(ch, false) == MENUKEY_JUMP))
      {
        done = true;
      }
    }

   // if interpEditRoomMenu is TRUE, user has quit

    if (!done) 
      done = interpEditRoomMenu(ch, newRoom, roomPtr->roomNumber);

    if (done)
    {
      if (!compareRoomInfo(roomPtr, newRoom))   // changes have been made
      {
       // if vnum has changed, update everything

        if (roomPtr->roomNumber != newRoom->roomNumber)
        {
          const uint oldNumb = roomPtr->roomNumber;
          const uint newNumb = newRoom->roomNumber;

          if (g_numbRooms == 1) 
          {
            g_lowestRoomNumber = g_highestRoomNumber = newNumb;
          }
          else
          {
            if (oldNumb == g_lowestRoomNumber)
              g_lowestRoomNumber = getNextRoom(oldNumb)->roomNumber;

            if (newNumb < g_lowestRoomNumber) 
              g_lowestRoomNumber = newNumb;

            if (oldNumb == g_highestRoomNumber)
              g_highestRoomNumber = getPrevRoom(oldNumb)->roomNumber;

            if (newNumb > g_highestRoomNumber) 
              g_highestRoomNumber = newNumb;
          }

          resetExits(oldNumb, newNumb);

          checkAndFixRefstoRoom(oldNumb, newNumb);

          g_roomLookup[newNumb] = roomPtr;
          g_roomLookup[oldNumb] = NULL;
        }

       // copy new info over old

        deleteAllExits(roomPtr, blnUpdateNumbExits);
        deleteExtraDescs(roomPtr->extraDescHead);
        deleteStringNodes(roomPtr->roomDescHead);

        memcpy(roomPtr, newRoom, sizeof(room));

        delete newRoom;

       // may have changed edescs

        deleteMasterKeywordList(roomPtr->masterListHead);
        roomPtr->masterListHead = createMasterKeywordList(roomPtr);

        deleteEditableList(roomPtr->editableListHead);
        roomPtr->editableListHead = createEditableList(roomPtr);

        g_madeChanges = true;
      }

     // exiting this room, but no alterations made

      else 
      {
        deleteAllExits(newRoom, blnUpdateNumbExits);
        deleteExtraDescs(newRoom->extraDescHead);
        deleteStringNodes(newRoom->roomDescHead);

        delete newRoom;

        g_madeChanges = origMadeChanges;  // deleteAllExits call resets g_madeChanges
      }

      if (allowJump)
      {
        if (checkMenuKey(ch, false) == MENUKEY_JUMP)
        {
          uint numb = roomPtr->roomNumber;

          switch (jumpRoom(&numb))
          {
            case MENU_JUMP_ERROR : return roomPtr;
            case MENU_JUMP_VALID : return findRoom(numb);

            default : return NULL;
          }
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_NEXT)
        {
          if (roomPtr->roomNumber != getHighestRoomNumber())
            return getNextRoom(roomPtr);
          else
            return roomPtr;
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_PREV)
        {
          if (roomPtr->roomNumber != getLowestRoomNumber())
            return getPrevRoom(roomPtr);
          else
            return roomPtr;
        }
      }

      _outtext("\n\n");

      return NULL;
    }
  }
}


//
// preEditRoom
//

void preEditRoom(const char *args)
{
  uint numb;
  char strn[256];
  room *room;

  if (!strlen(args)) 
  {
    room = g_currentRoom;
  }
  else
  {
    numb = strtoul(args, NULL, 10);
    room = findRoom(numb);

    if (!room)
    {
      sprintf(strn, "\nRoom #%u does not exist.\n\n", numb);
      _outtext(strn);

      return;
    }
  }

  do
  {
    room = editRoom(room, true);
  } while (room);
}


//
// jumpRoom
//

char jumpRoom(uint *numb)
{
  char promptstrn[128];
  const struct rccoord coords = _gettextposition();

  clrline(coords.row, 7, 0);
  _settextposition(coords.row, 1);

  sprintf(promptstrn, "room vnum to jump to [%u-%u]", 
          getLowestRoomNumber(), getHighestRoomNumber());

  editUIntValSearchableList(numb, false, promptstrn, displayRoomList);

  if ((*numb >= g_numbLookupEntries) || !roomExists(*numb))
    return MENU_JUMP_ERROR;
  else
    return MENU_JUMP_VALID;
}
