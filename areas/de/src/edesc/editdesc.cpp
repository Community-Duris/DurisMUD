//
//  File: editdesc.cpp   originally part of durisEdit
//
//  Usage: functions related to users editing descs and extra descs
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
#include <stdlib.h>

#include "types.h"
#include "fh.h"
#include "misc/menu.h"
#include "graphcon.h"
#include "edesc.h"

extern bool g_madeChanges;
extern menu g_edescMenu;


//
// editDesc : edit description, also known as a list of stringNodes
//

void editDesc(stringNode **descHead)
{
  stringNode *stringCopy;


  stringCopy = copyStringNodes(*descHead);

  *descHead = editStringNodes(*descHead, true);

  if (!compareStringNodes(*descHead, stringCopy))
    g_madeChanges = true;

  deleteStringNodes(stringCopy);
}


//
// displayEditExtraDescMenu : Displays the edit extra desc menu
//
//   *extraDescNode : extra desc being edited
//

void displayEditExtraDescMenu(const extraDesc *extraDescNode)
{
  char keywords[1536], outStrn[1024];

 // turn the keyword list into something that's readable by mere mortals

  getReadableKeywordStrn(extraDescNode->keywordListHead, keywords, 1023);

 // display the menu

  sprintf(outStrn, "extra desc &+L\"&n%s&+L\"", keywords);

  displaySimpleMenuHeader(outStrn);

  displayMenu(&g_edescMenu, extraDescNode);
}


//
// interpEditRoomExtraDescMenu : Interpret command user typed - returns TRUE
//                               if the user hits 'Q'
//
//               ch : command typed by user
//   *extraDescNode : node being edited
//

bool interpEditExtraDescMenu(const usint ch, extraDesc *extraDescNode)
{
  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

 // menu

  if (interpretMenu(&g_edescMenu, extraDescNode, ch))
  {
    displayEditExtraDescMenu(extraDescNode);

    return false;
  }

  return false;
}


//
// editExtraDesc : Edit an extra desc..  More accurately, the "main" menu
//                 function for the menu to do just that.  Returns TRUE if
//                 changes have been made.
//
//     *extraDescNode : extra desc being edited
//

bool editExtraDesc(extraDesc *extraDescNode)
{
  usint ch;
  extraDesc *newExtraNode;


  newExtraNode = copyOneExtraDesc(extraDescNode);
  if (!newExtraNode)
  {
    displayAllocError("extraDescNode", "editExtraDesc");

    return false;
  }

 // display the menu

  displayEditExtraDescMenu(newExtraNode);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      deleteOneExtraDesc(newExtraNode);

      _outtext("\n\n");

      return false;
    }


   // if interpEditExtraDescMenu is TRUE, user has quit (hit 'Q')

    if (interpEditExtraDescMenu(ch, newExtraNode))
    {
     // see if changes have been made in the extra desc

      if (!compareOneExtraDesc(extraDescNode, newExtraNode))
      {
        deleteStringNodes(extraDescNode->keywordListHead);
        deleteStringNodes(extraDescNode->extraDescStrnHead);

        memcpy(extraDescNode, newExtraNode, sizeof(extraDesc));

        delete newExtraNode;

        g_madeChanges = true;

        _outtext("\n\n");

        return true;
      }
      else
      {
        deleteOneExtraDesc(newExtraNode);

        _outtext("\n\n");

        return false;
      }
    }
  }
}


//
// deleteExtraDescUser :
//

void deleteExtraDescUser(extraDesc **extraDescHead, uint *numbExtraDescs)
{
  extraDesc *descNode;
  usint ch, descNumb = 0;
  const struct rccoord oldxy = _gettextposition();


  clrline(oldxy.row);
  _settextposition(oldxy.row, 1);

  displayColorString(
"&+CEnter letter of extra desc to delete (Q to abort): &n");

  do
  {
    ch = toupper(getkey());

    if (ch == 'Q')
      return;
  } while (!((ch >= 'A') && (ch <= ('A' + ((*numbExtraDescs) - 1)))));

  ch -= 'A';

  descNode = *extraDescHead;

  while (descNode)
  {
    if (descNumb == ch)
    {
      if (descNode->Last)
      {
        descNode->Last->Next = descNode->Next;
      }

      if (descNode->Next)
      {
        descNode->Next->Last = descNode->Last;
      }

      if (descNode == *extraDescHead)
      {
        *extraDescHead = descNode->Next;
      }

      deleteOneExtraDesc(descNode);

      return;
    }

    descNumb++;
    descNode = descNode->Next;
  }
}
