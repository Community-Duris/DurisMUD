//
//  File: editoexd.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing object extra descs
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

#include "fh.h"
#include "types.h"
#include "keys.h"

#include "object.h"
#include "edesc/edesc.h"

#include "misc/menu.h"
#include "editoexd.h"



extern menu g_objEdescMenu;


//
// displayEditObjExtraDescMenu : displays the edit extra desc menu for objs
//
//             *obj : pointer to obj being edited
//   *extraDescHead : head of extraDesc list being edited
//  *numbExtraDescs : pointer to data that is set to the number of extra descs
//                    the obj contains
//

void displayEditObjExtraDescMenu(const objectType *obj, const extraDesc *extraDescHead, uint *numbExtraDescs)
{
  char strn[2048], strn2[1024], ch = 'A';
  uint numbShown = 0;
  const extraDesc *extraDescNode = extraDescHead;


 // display title

  displayMenuHeader(ENTITY_OBJECT, getObjShortName(obj), obj->objNumber, "extra descs", false);

 // if obj has no extra descs, say so

  if (!extraDescHead)
    displayColorString(" &+wThis object has no extra descriptions.\n\n");

 // set numbExtraDescNodes and *numbExtraDescs

  *numbExtraDescs = getNumbExtraDescs(extraDescHead);

 // run through the list of extraDescNodes, stopping when we hit the end of
 // the list or when the max supported displayable is hit

  while (extraDescNode)
  {
    if ((numbShown + FIRST_EDITOEXD_CH) > LAST_EDITOEXD_CH)
      ch = FIRST_POSTALPHA_CH +
           (char)(numbShown - (LAST_EDITOEXD_CH - FIRST_EDITOEXD_CH)) - 1;
    else 
      ch = (char)numbShown + FIRST_EDITOEXD_CH;

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

    if ((numbShown % MAX_EDITABLE_OBJ_EXTRADESCS) == 0)
    {
      displayColorString("\n&+CPress a key to continue..");
      getkey();
      _outtext("\n\n");
    }

    extraDescNode = extraDescNode->Next;
  }

  displayMenu(&g_objEdescMenu, extraDescHead);
}


//
// interpEditObjExtraDescMenu : Interprets user input - returns TRUE if user
//                              hit 'Q', FALSE otherwise
//
//               ch : user input
//             *obj : obj record associated with extra descs being edited
//  **extraDescNode : pointer to pointer to extra desc being edited
//  *numbExtraDescs : number of extra descs
//

bool interpEditObjExtraDescMenu(const usint ch, objectType *obj, extraDesc **extraDescHead,
                                uint *numbExtraDescs, bool *addedKeyword)
{
  uint i = 0;
  extraDesc *descNode;
  char strn[1024];


 // allow 'Q' to edit edesc..

  if ((ch != 'Q') && (checkMenuKey(ch, false) == MENUKEY_SAVE)) 
    return true;

 // user picked Z to create a new desc

  if (ch == 'Z')
  {
   // first edesc gets the object's keywords

    if (!*extraDescHead)
    {
      createExtraDesc(extraDescHead, createKeywordString(obj->keywordListHead, strn, 1023), true);
    }
    else 
    {
      createExtraDesc(extraDescHead, NULL, true);
    }

    displayEditObjExtraDescMenu(obj, *extraDescHead, numbExtraDescs);

    return false;
  }
  else

 // delete an edesc

  if ((ch == 'Y') && *extraDescHead)
  {
    deleteExtraDescUser(extraDescHead, numbExtraDescs);

    displayEditObjExtraDescMenu(obj, *extraDescHead, numbExtraDescs);

    return false;
  }
  else

 // special identification edescs - keywords

  if (ch == 'U')
  {
    descNode = getEdescinList(*extraDescHead, "_ID_NAME_");

   // shucks, no match

    if (!descNode)
    {
      descNode = createExtraDesc(extraDescHead, "_id_name_~", false);

     // add '_id_' keyword to object if it doesn't have it

      if (!scanKeyword("_ID_", obj->keywordListHead))
      {
        addKeywordtoList(&obj->keywordListHead, "_id_");

        *addedKeyword = true;
      }
    }

    descNode->extraDescStrnHead =
        editStringNodes(descNode->extraDescStrnHead, true);

    displayEditObjExtraDescMenu(obj, *extraDescHead, numbExtraDescs);

    return false;
  }
  else

 // special identification edescs - short name

  if (ch == 'V')
  {
    descNode = getEdescinList(*extraDescHead, "_ID_SHORT_");

   // shucks, no match

    if (!descNode)
    {
      descNode = createExtraDesc(extraDescHead, "_id_short_~", false);

     // add '_id_' keyword to object if it doesn't have it

      if (!scanKeyword("_ID_", obj->keywordListHead))
      {
        addKeywordtoList(&obj->keywordListHead, "_id_");

        *addedKeyword = true;
      }
    }

    descNode->extraDescStrnHead =
        editStringNodes(descNode->extraDescStrnHead, true);

    displayEditObjExtraDescMenu(obj, *extraDescHead, numbExtraDescs);

    return false;
  }
  else

 // special identification edescs - long name

  if (ch == 'W')
  {
    descNode = getEdescinList(*extraDescHead, "_ID_DESC_");

   // shucks, no match

    if (!descNode)
    {
      descNode = createExtraDesc(extraDescHead, "_id_desc_~", false);

     // add '_id_' keyword to object if it doesn't have it

      if (!scanKeyword("_ID_", obj->keywordListHead))
      {
        addKeywordtoList(&obj->keywordListHead, "_id_");

        *addedKeyword = true;
      }
    }

    descNode->extraDescStrnHead = editStringNodes(descNode->extraDescStrnHead, true);

    displayEditObjExtraDescMenu(obj, *extraDescHead, numbExtraDescs);

    return false;
  }
  else

  if (((ch >= FIRST_EDITOEXD_CH) || (ch >= FIRST_POSTALPHA_CH)) && *numbExtraDescs)
  {
    usint pos = ch;

    if ((pos >= FIRST_POSTALPHA_CH) && (pos < 'A'))
      pos += (LAST_EDITOEXD_CH - FIRST_POSTALPHA_CH) + 1;  // it's magic

    pos -= FIRST_EDITOEXD_CH;

    descNode = *extraDescHead;
    while (descNode)
    {
     // if a match is found, edit the extra desc

      if (i == pos)
      {
        editExtraDesc(descNode);

        displayEditObjExtraDescMenu(obj, *extraDescHead, numbExtraDescs);

        return false;
      }

      i++;
      descNode = descNode->Next;
    }

   // not an error due to new method - keypress out of bounds, though

    if (!descNode)
      return false;
  }

  return false;
}


//
// editObjExtraDesc : The "main" function for the edit obj extra desc menu -
//                    lists all the extra descs, etc. - returns TRUE if
//                    user aborted, FALSE otherwise
//
//   *obj : obj being edited
//

bool editObjExtraDesc(objectType *obj, bool *addedIdentKeyword)
{
  uchar ch;
  uint numbExtraDescs;
  extraDesc *newExtraDescHead = copyExtraDescs(obj->extraDescHead);


 // display the menu

  displayEditObjExtraDescMenu(obj, newExtraDescHead, &numbExtraDescs);


  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      deleteExtraDescs(newExtraDescHead);

      _outtext("\n\n");

      return true;
    }

    if (interpEditObjExtraDescMenu(ch, obj, &newExtraDescHead, &numbExtraDescs, addedIdentKeyword))
    {
      if (!compareExtraDescs(obj->extraDescHead, newExtraDescHead))
      {
        deleteExtraDescs(obj->extraDescHead);
        obj->extraDescHead = newExtraDescHead;
      }
      else 
      {
        deleteExtraDescs(newExtraDescHead);
      }

      return false;
    }
  }
}
