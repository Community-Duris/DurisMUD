//
//  File: editotrp.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing object trap info
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
#include "misc/menu.h"

#include "graphcon.h"

#include "object.h"

extern menu g_objTrapMenu;

//
// displayEditObjTrapInfoMenu : Displays the edit obj trap menu
//
//    *obj : obj being edited
//

void displayEditObjTrapInfoMenu(const objectType *obj)
{
  displayMenuHeader(ENTITY_OBJECT, getObjShortName(obj), obj->objNumber, "trap info", false);

  displayMenu(&g_objTrapMenu, obj);
}


//
// interpEditObjTrapInfoMenu : Interprets user input for edit obj trap menu -
//                             returns TRUE if the user hit 'Q'
//
//     ch : user input
//   *obj : obj to edit
//

bool interpEditObjTrapInfoMenu(const usint ch, objectType *obj)
{
  if (interpretMenu(&g_objTrapMenu, obj, ch))
  {
    displayEditObjTrapInfoMenu(obj);

    return false;
  }
  else

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) return true;

  return false;
}


//
// editObjTrapInfo : Edit obj misc - "main" function
//
//   *obj : obj to edit
//

void editObjTrapInfo(objectType *obj)
{
  usint ch;
  uint oldBits = obj->trapBits, oldLevel = obj->trapLevel;
  int oldDam = obj->trapDam, oldCharge = obj->trapCharge;


  displayEditObjTrapInfoMenu(obj);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      obj->trapBits = oldBits;
      obj->trapDam = oldDam;
      obj->trapCharge = oldCharge;
      obj->trapLevel = oldLevel;

      _outtext("\n\n");

      return;
    }

    if (interpEditObjTrapInfoMenu(ch, obj)) 
      return;
  }
}
