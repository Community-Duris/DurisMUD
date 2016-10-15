//
//  File: delobjh.cpp    originally part of durisEdit
//
//  Usage: functions for deleting objectHere structures
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


#include "../types.h"
#include "../fh.h"

#include "objhere.h"
#include "../zone/zone.h"

extern uint g_numbObjs;
extern bool g_madeChanges;


//
// deleteObjHereList : Deletes an entire objectHere list
//
//    srcObjHere : head of list
//     decLoaded : if TRUE, decrements stuff
//

void deleteObjHereList(objectHere *srcObjHere, const bool decLoaded)
{
  objectHere *nextObjHere;


  while (srcObjHere)
  {
    nextObjHere = srcObjHere->Next;

    if (decLoaded)
      removeObjLoad(srcObjHere);

    deleteObjHereList(srcObjHere->objInside, false);  // removeObjLoad() handles contents

    delete srcObjHere;

    srcObjHere = nextObjHere;
  }
}


//
// deleteObjHere : Deletes one objectHere node
//
//    objHere : delete
//

void deleteObjHere(objectHere *objHere, const bool decLoaded)
{
  if (!objHere) 
    return;

 // must be done before innards are deleted

  if (decLoaded)
    removeObjLoad(objHere);

  deleteObjHereList(objHere->objInside, false);
                                          // takes care of all objectHere
                                          // nodes in objInside

  delete objHere;
}


//
// deleteObjHereinList : Deletes a specific objectHere in a list that starts
//                       at *objHead, updating whatever needs to be updated for
//                       the list to be valid.
//
//                       assumes objHere is in list
//

void deleteObjHereinList(objectHere **objHead, objectHere *objHere, const bool decLoaded)
{
  objectHere *prevObj;


  if (objHere == (*objHead))
  {
    *objHead = (*objHead)->Next;

    deleteObjHere(objHere, decLoaded);
  }
  else
  {
    prevObj = *objHead;

    while (prevObj->Next != objHere)
      prevObj = prevObj->Next;

   // couldn't find an object that points to object being deleted - error

    prevObj->Next = objHere->Next;

    deleteObjHere(objHere, decLoaded);
  }
}


//
// removeObjHerefromList : exactly like deleteObjHereinList, except it
//                         only removes the references to the record, not
//                         the record itself
//
//                         assumes objHere is in list
//

void removeObjHerefromList(objectHere **objHead, objectHere *objHere, const bool decLoaded)
{
  objectHere *prevObj;


  if (objHere == (*objHead))
  {
    *objHead = (*objHead)->Next;
  }
  else
  {
    prevObj = *objHead;

    while (prevObj->Next != objHere)
      prevObj = prevObj->Next;

    prevObj->Next = objHere->Next;
  }

  objHere->Next = NULL;

  if (decLoaded)
    removeObjLoad(objHere);
}


//
// deleteAllObjHereofTypeInList :
//        Runs through a list pointed to by objHereHead, deleting all
//        objectHeres that match objType.
//

void deleteAllObjHereofTypeInList(objectHere **objHereHead, const objectType *objType,
                                  const bool decLoaded)
{
  objectHere *objHere = *objHereHead, *prevObj, *nextObj;


  while (objHere)
  {
    nextObj = objHere->Next;

    if (objHere->objInside)
    {
      deleteAllObjHereofTypeInList(&(objHere->objInside), objType, decLoaded);
    }

    if (objHere->objectPtr == objType)
    {
      if (objHere == *objHereHead) 
      {
        *objHereHead = objHere->Next;
      }
      else
      {
        prevObj = *objHereHead;
        while (prevObj->Next != objHere)
          prevObj = prevObj->Next;

        prevObj->Next = objHere->Next;
      }

      deleteObjHere(objHere, decLoaded);
    }

    objHere = nextObj;
  }
}


//
// deleteAllObjHeresofType : Deletes all objectHere nodes that match objType
//

void deleteAllObjHeresofType(const objectType *objType, const bool decLoaded)
{
  room *roomPtr;
  mobHere *mob;
  const uint highRoomNumb = getHighestRoomNumber();


 // delete objects laying about and that mobs are carrying and equipping
 // from every room

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      deleteAllObjHereofTypeInList(&(roomPtr->objectHead), objType, decLoaded);

      mob = roomPtr->mobHead;

      while (mob)
      {
        deleteAllObjHereofTypeInList(&(mob->inventoryHead), objType, decLoaded);

        for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
          deleteAllObjHereofTypeInList(&(mob->equipment[i]), objType, decLoaded);

        mob = mob->Next;
      }
    }
  }
}
