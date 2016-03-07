//
//  File: editoms2.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing misc object info
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fh.h"
#include "types.h"
#include "misc/menu.h"

#include "graphcon.h"

extern menu g_objMisc2Menu;


//
// displayEditObjMisc2Menu : Displays the edit obj miscellany menu
//
//    *obj : obj being edited
//

void displayEditObjMisc2Menu(const objectType *obj)
{
  displayMenuHeader(ENTITY_OBJECT, getObjShortName(obj), obj->objNumber, "miscellany", false);

  displayMenu(&g_objMisc2Menu, obj);
}


//
// interpEditObjMisc2Menu : Interprets user input for edit obj flags menu -
//                          returns TRUE if the user hit 'Q'
//
//     ch : user input
//   *obj : obj to edit
//

bool interpEditObjMisc2Menu(const usint ch, objectType *obj)
{
 // hit menu

  if (interpretMenu(&g_objMisc2Menu, obj, ch))
  {
    displayEditObjMisc2Menu(obj);

    return false;
  }
  else

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editObjMisc2 : Edit obj misc - "main" function
//
//   *obj : obj to edit
//

void editObjMisc2(objectType *obj)
{
  usint ch;
  uint oldMat = obj->material, oldSize = obj->size,
        oldCraft = obj->craftsmanship, oldDam = obj->damResistBonus,
        oldWorth = obj->worth, oldCond = obj->condition, oldVol = obj->space;
  int oldWeight = obj->weight;

 // back up the old stuff

  displayEditObjMisc2Menu(obj);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      obj->material = oldMat;
      obj->size = oldSize;
      obj->craftsmanship = oldCraft;
      obj->damResistBonus = oldDam;
      obj->weight = oldWeight;
      obj->worth = oldWorth;
      obj->condition = oldCond;
      obj->space = oldVol;

      _outtext("\n\n");

      return;
    }

    if (interpEditObjMisc2Menu(ch, obj)) 
      return;
  }
}
