//
//  File: delobjt.cpp    originally part of durisEdit
//
//  Usage: functions for deleting object types
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


#include "types.h"
#include "fh.h"

#include "object.h"

extern objectType **g_objLookup;
extern uint g_numbObjTypes;
extern bool g_madeChanges;

//
// deleteObjectType : deletes all the associated baggage with an objectType rec
//
//       srcObject : record to delete
//    updateGlobal : if TRUE, decrements g_numbObjTypes variable
//

void deleteObjectType(objectType *srcObject, const bool updateGlobal)
{
 // make sure src exists

  if (!srcObject) 
    return;

 // delete object keywords

  deleteStringNodes(srcObject->keywordListHead);

 // extra desc linked list

  deleteExtraDescs(srcObject->extraDescHead);

 // update "globally" - a bit ambiguous, but life's a bitch

  if (updateGlobal)
  {
    const uint numb = srcObject->objNumber;

    if (isObjTypeUsed(numb)) 
      updateAllObjMandElists();

    g_objLookup[numb] = NULL;

    g_numbObjTypes--;
    createPrompt();

    resetLowHighObj();

    deleteEntityinLoaded(ENTITY_OBJECT, numb);

    g_madeChanges = true;
  }

 // finally, delete the object itself

  delete srcObject;
}


//
// deleteObjectTypeAssocLists : deletes all the associated baggage with an
//                              object type, but not the record itself
//
//     *srcObject : record to alter
//

void deleteObjectTypeAssocLists(objectType *srcObject)
{
 // make sure src exists

  if (!srcObject) 
    return;

 // delete object keywords

  deleteStringNodes(srcObject->keywordListHead);

 // extra desc linked list

  deleteExtraDescs(srcObject->extraDescHead);
}
