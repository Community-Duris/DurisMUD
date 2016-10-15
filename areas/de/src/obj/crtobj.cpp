//
//  File: crtobj.cpp     originally part of durisEdit
//
//  Usage: functions related to creating object types and objHeres
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

#include "../types.h"
#include "../fh.h"

#include "object.h"
#include "objhere.h"
#include "objsize.h"
#include "material.h"
#include "objcraft.h"



extern objectType *g_defaultObject, **g_objLookup;
extern uint g_numbObjs, g_numbLookupEntries, g_numbObjTypes, g_lowestObjNumber, g_highestObjNumber;
extern bool g_madeChanges;

//
// createObjectType : Function to create a new object type - returns pointer to
//                    the new object type
//
//    incLoaded : if TRUE, assumes object is being added to global list
//      objNumb : number of obj type being created
//  getFreeNumb : if true, ignores objNumb and assigns first free numb
//

objectType *createObjectType(const bool incLoaded, const uint objNumb, const bool getFreeNumb)
{
  objectType *newObj;
  uint numb;


 // create a new object type

  newObj = new(std::nothrow) objectType;
  if (!newObj)
  {
    displayAllocError("objectType", "createObjectType");

    return NULL;
  }

  if (g_defaultObject)
  {
    newObj = copyObjectType(g_defaultObject, false);

    newObj->defaultObj = false;
  }
  else
  {
    memset(newObj, 0, sizeof(objectType));

    strcpy(newObj->objShortName, OBJ_DEFAULT_SHORT_DESC);
    strcpy(newObj->objLongName, OBJ_DEFAULT_LONG_DESC);

    newObj->keywordListHead = createKeywordList("object unnamed~");

    newObj->objType = ITEM_TRASH;
    newObj->size = OBJSIZE_MEDIUM;
    newObj->material = MAT_IRON;
    newObj->condition = 100;
    newObj->craftsmanship = OBJCRAFT_AVERAGE;
  }

  if (!getFreeNumb)
  {
    newObj->objNumber = numb = objNumb;

    if (objExists(numb))  // a dupe (shouldn't hit this)
    {
      deleteObjectType(newObj, false);
      return NULL;
    }
  }
  else
  {
    if (noObjTypesExist()) 
      newObj->objNumber = numb = getLowestRoomNumber();
    else 
      newObj->objNumber = numb = getFirstFreeObjNumber();
  }

  if (incLoaded)
  {
   // if not enough lookup entries, increase the tables

    if (numb >= g_numbLookupEntries)
    {
      if (!changeMaxVnumAutoEcho(numb + 1000))
      {
        deleteObjectType(newObj, false);
        return NULL;
      }
    }

   // set everything appropriately

    g_numbObjTypes++;

    g_objLookup[numb] = newObj;

    if (numb > g_highestObjNumber) 
      g_highestObjNumber = numb;

    if (numb < g_lowestObjNumber)  
      g_lowestObjNumber = numb;

    createPrompt();

    g_madeChanges = true;
  }

  return newObj;
}
