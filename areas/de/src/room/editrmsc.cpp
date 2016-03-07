//
//  File: editrmsc.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing miscellaneous room info
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

#include "fh.h"
#include "types.h"

#include "graphcon.h"

#include "room.h"
#include "misc/menu.h"

extern menu g_roomMiscMenu;


//
// displayEditRoomMiscMenu : Displays the edit room miscellany menu
//
//    *room : room being edited
//

void displayEditRoomMiscMenu(const room *roomPtr)
{
  displayMenuHeader(ENTITY_ROOM, roomPtr->roomName, roomPtr->roomNumber, "miscellany", false);

  displayMenu(&g_roomMiscMenu, roomPtr);
}


//
// interpEditRoomMiscMenu : Interprets user input for edit room flags menu -
//                          returns TRUE if the user hit 'Q'
//
//     ch : user input
//   *room : room to edit
//

bool interpEditRoomMiscMenu(const usint ch, room *room)
{
 // hit menu for most

  if (interpretMenu(&g_roomMiscMenu, room, ch))
  {
    displayEditRoomMiscMenu(room);

    return false;
  }
  else

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) return true;

  return false;
}


//
// editRoomMisc : Edit room misc - "main" function
//
//   *room : room to edit
//

void editRoomMisc(room *room)
{
  usint ch;
  uint oldFallChance = room->fallChance;
  int oldManaApply = room->manaApply;
  uint oldManaFlag = room->manaFlag;
  uint oldCurrent = room->current;
  uint oldCurrDir = room->currentDir;


  displayEditRoomMiscMenu(room);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      room->fallChance = oldFallChance;
      room->manaFlag = oldManaFlag;
      room->manaApply = oldManaApply;
      room->current = oldCurrent;
      room->currentDir = oldCurrDir;

      _outtext("\n\n");

      return;
    }

    if (interpEditRoomMiscMenu(ch, room)) 
      return;
  }
}
