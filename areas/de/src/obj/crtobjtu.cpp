//
//  File: crtobjtu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for creating object types
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
#include <string.h>
#include <stdlib.h>

#include "../types.h"
#include "../fh.h"

#include "object.h"


//
// createObjectTypeUser : user interface stuff for creating an object type -
//                        returns pointer to new object, NULL if error
//
//   args : user input
//

objectType *createObjectTypeUser(const char *args)
{
  char strn[512];
  objectType *newObj;
  uint numb = 0;  // initialized to keep debugger happy
  bool getFreeNumb;
 

  if (strlen(args))
  {
    if (!strnumber(args))
    {
      _outtext("\nError in vnum argument - non-numerics in input.\n\n");
      return NULL;
    }

    numb = strtoul(args, NULL, 10);
    if (objExists(numb))
    {
      sprintf(strn,
"\nCannot create an object type with the vnum %d - this vnum is already taken.\n\n",
              numb);
      _outtext(strn);

      return NULL;
    }

    getFreeNumb = false;
  }
  else
  {
    getFreeNumb = true;
  }

  newObj = createObjectType(true, numb, getFreeNumb);
  if (!newObj)
    return NULL;

  resetEntityPointersByNumb(true, false);

  sprintf(strn, "\nObject type #%u '%s&n' created.\n\n", newObj->objNumber, getObjShortName(newObj));
  displayColorString(strn);

  return newObj;
}
