//
//  File: delobjtu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for deleting object types
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

#include "types.h"
#include "fh.h"
#include "keys.h"

#include "object.h"

extern bool g_madeChanges;


//
// checkDeleteObjectsofType : returns TRUE if user doesn't abort - this
//                            function prompts user if any of these object
//                            types exist
//

bool checkDeleteObjectsofType(const objectType *objType)
{
  if (checkForObjHeresofType(objType))
  {
    if (displayYesNoPrompt("\n&+cObjects exist of this type and will have to be deleted - continue", promptNo,
                           false) == promptYes)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  return true;
}


//
// deleteObjectTypeUser : Deletes an objectType based on info specified by user
//                        in strn.
//
//   *strn : user-entered string
//     pos : pos in string where id number starts
//

void deleteObjectTypeUser(const char *args)
{
  char outStrn[512];
  objectType *obj;
  uint numb;


  if (!strlen(args))
  {
    deleteObjectTypeUserPrompt();
    return;
  }

  if (!strnumer(args))
  {
    _outtext("\nError in input - specify vnum of object type.\n\n");
    return;
  }

  numb = strtoul(args, NULL, 10);

  obj = findObj(numb);
  if (!obj)
  {
    sprintf(outStrn, "\nObject type #%u not found.\n\n", numb);
    _outtext(outStrn);

    return;
  }

  if (!checkDeleteObjectsofType(obj)) 
    return;

  deleteAllObjHeresofType(obj, true);
  deleteObjsinInv(obj);

  sprintf(outStrn, "\nObject type #%u '%s&n' deleted.\n\n", 
          numb, getObjShortName(obj));
  displayColorString(outStrn);

  deleteObjectType(obj, true);
}


//
// deleteObjectTypeUserPrompt
//

void deleteObjectTypeUserPrompt(void)
{
  char strn[11] = "";


  _outtext("\n");

  editStrnValSearchableList(strn, 10, "object vnum to delete", displayObjectTypeList);

  if (strlen(strn)) 
    deleteObjectTypeUser(strn);
  else 
    _outtext("\n\n");
}
