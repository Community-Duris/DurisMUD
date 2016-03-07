//
//  File: editrexd.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing room extra descs
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

#include "types.h"
#include "fh.h"

#include "editrexd.h"
#include "room.h"
#include "exit.h"

#include "misc/menu.h"
#include "keys.h"



extern menu g_roomEdescMenu;


//
// displayEditRoomExtraDescMenu : displays the edit extra desc menu for rooms
//
//            *room : pointer to room being edited
//   *extraDescNode : head of extraDesc list being edited
//  *numbExtraDescs : pointer to data that is set to the number of extra descs
//                    the room contains
//

void displayEditRoomExtraDescMenu(const room *room, const extraDesc *extraDescHead, uint *numbExtraDescs)
{
  char strn[1536], strn2[1024], ch;
  const extraDesc *extraDescNode = extraDescHead;
  uint numbShown = 0;


 // display the menu header

  displayMenuHeader(ENTITY_ROOM, room->roomName, room->roomNumber, "extra descs", false);

 // if room has no extra descs, say so

  if (!extraDescHead)
    displayColorString(" &+wThis room has no extra descriptions.\n\n");

 // set *numbExtraDescs

  *numbExtraDescs = getNumbExtraDescs(extraDescHead);

 // run through the list of extraDescNodes, stopping when we hit the end of
 // the list or when the max supported displayable is hit

  while (extraDescNode)
  {
    if ((numbShown + FIRST_EDITREXD_CH) > LAST_EDITREXD_CH)
      ch = FIRST_POSTALPHA_CH +
           (char)(numbShown - (LAST_EDITREXD_CH - FIRST_EDITREXD_CH)) - 1;
    else 
      ch = (char)(numbShown + FIRST_EDITREXD_CH);

    getReadableKeywordStrn(extraDescNode->keywordListHead, strn2, 1023);

    sprintf(strn, "   &+Y%c&+L. &+w%s\n", ch, strn2);
    displayColorString(strn);

    if ((ch == '@') && extraDescNode->Next)
    {
      displayColorString("\n&+M"
".. too many to list (edit extras manually) ..\n");

      break;
    }

    numbShown++;

    if ((numbShown % MAX_EDITABLE_ROOM_EXTRADESCS) == 0)
    {
      displayColorString("\n&+CPress a key to continue..");
      getkey();
      _outtext("\n\n");
    }

    extraDescNode = extraDescNode->Next;
  }

 // display the rest of the stuff

  displayMenu(&g_roomEdescMenu, extraDescHead);
}


//
// interpEditRoomExtraDescMenu : Interprets user input - returns TRUE if user
//                               hit 'Q', FALSE otherwise
//
//               ch : user input
//            *room : room record associated with extra descs being edited
//  **extraDescNode : pointer to pointer to extra desc being edited
//  *numbExtraDescs : number of extra descs
//

bool interpEditRoomExtraDescMenu(const usint ch, room *room, extraDesc **extraDescHead, uint *numbExtraDescs)
{
 // allow 'Q' to edit edesc..

  if ((ch != 'Q') && (checkMenuKey(ch, false) == MENUKEY_SAVE)) 
    return true;

 // create a new desc

  if (ch == 'Z')
  {
    createExtraDesc(extraDescHead, NULL, true);

    displayEditRoomExtraDescMenu(room, *extraDescHead, numbExtraDescs);

    return false;
  }
  else

 // delete a desc

  if ((ch == 'Y') && *extraDescHead)
  {
    deleteExtraDescUser(extraDescHead, numbExtraDescs);

    displayEditRoomExtraDescMenu(room, *extraDescHead, numbExtraDescs);

    return false;
  }
  else

  if (((ch >= FIRST_EDITREXD_CH) || (ch >= FIRST_POSTALPHA_CH)) && *numbExtraDescs)
  {
    extraDesc *descNode = *extraDescHead;
    usint pos = ch;
    uint i = 0;


    if ((ch >= FIRST_POSTALPHA_CH) && (ch < 'A'))
      pos += (LAST_EDITREXD_CH - FIRST_POSTALPHA_CH) + 1;  // it's magic

    pos -= FIRST_EDITREXD_CH;

    while (descNode)
    {
     // if a match is found, edit the extra desc

      if (i == pos)
      {
        editExtraDesc(descNode);

        displayEditRoomExtraDescMenu(room, *extraDescHead, numbExtraDescs);

        return false;
      }

      i++;
      descNode = descNode->Next;
    }

   // a NULL node means the user hit an out-of-bounds key

    if (!descNode)
      return false;
  }

  return false;
}


//
// editRoomExtraDesc : The "main" function for the edit room extra desc menu -
//                     lists all the extra descs, etc.
//
//   *room : room being edited
//

void editRoomExtraDesc(room *room)
{
  usint ch;
  uint numbExtraDescs;
  extraDesc *newExtraDescHead = copyExtraDescs(room->extraDescHead);


 // display the menu

  displayEditRoomExtraDescMenu(room, newExtraDescHead, &numbExtraDescs);


  while (true)
  {
    ch = toupper(getkey());

   // allow 'X' to edit edesc..

    if ((ch != 'X') && (checkMenuKey(ch, false) == MENUKEY_ABORT))
    {
      deleteExtraDescs(newExtraDescHead);

      _outtext("\n\n");

      return;
    }

    if (interpEditRoomExtraDescMenu(ch, room, &newExtraDescHead, &numbExtraDescs))
    {
      if (!compareExtraDescs(room->extraDescHead, newExtraDescHead))
      {
        deleteExtraDescs(room->extraDescHead);
        room->extraDescHead = newExtraDescHead;
      }
      else 
      {
        deleteExtraDescs(newExtraDescHead);
      }

      return;
    }
  }
}
