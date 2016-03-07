//
//  File: objhere.cpp    originally part of durisEdit
//
//  Usage: functions related to objectHere structures in general
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




#include "fh.h"
#include "types.h"

#include "objhere.h"
#include "room/room.h"
#include "object.h"



extern uint g_numbObjs;
extern editableListNode *g_inventory;
extern bool g_madeChanges;
extern room *g_currentRoom;


//
// addObjLoad : given objHere, inc loaded amounts, including any contents
//

void addObjLoad(const objectHere *objHere)
{
  addEntity(ENTITY_OBJECT, objHere->objectNumb);

  loadAllinObjHereList(objHere->objInside);

  g_numbObjs++;

  createPrompt();

  g_madeChanges = true;
}


//
// removeObjLoad : reduce internal count, set g_madeChanges, etc - removes contents
//

void removeObjLoad(const objectHere *objHere)
{
  if (objHere->objInside)
  {
    const objectHere *objIn = objHere->objInside;

    while (objIn)
    {
      removeObjLoad(objIn);

      objIn = objIn->Next;
    }
  }

  decEntity(ENTITY_OBJECT, objHere->objectNumb);

  g_numbObjs--;

  createPrompt();

  g_madeChanges = true;
}


//
// loadAllinObjHereList : Adds all the entities in an objHere list to the
//                        entityLoaded list
//

void loadAllinObjHereList(const objectHere *objHead)
{
  while (objHead)
  {
    addObjLoad(objHead);

    objHead = objHead->Next;
  }
}


//
// checkAllObjInsideForVnum : if objHere or its contents have vnum, returns objHere
//

objectHere *checkAllObjInsideForVnum(objectHere *objHere, const uint vnum)
{
  while (objHere)
  {
    if (objHere->objectNumb == vnum) 
      return objHere;

    objectHere *objH = checkAllObjInsideForVnum(objHere->objInside, vnum);
    if (objH)
      return objH;

    objHere = objHere->Next;
  }

  return NULL;
}


//
// findObjHere : Returns the address of the objectHere node that has the
//               requested objNumber (if any) - scans through every objHere
//               in the room list
//
//            objNumber : object number to look for in all objectHere nodes
//                        everywhere
//            *roomNumb : pointer to uint to set to roomNumb - if NULL, not set
// checkOnlyCurrentRoom : if TRUE, checks only current room, otherwise all rooms
//     checkOnlyVisible : if TRUE, doesn't check objs in containers or on mobs
//

objectHere *findObjHere(const uint objNumber, uint *roomNumb, const bool checkOnlyCurrentRoom, 
                        const bool checkOnlyVisible)
{
  room *roomPtr;
  
  
  if (checkOnlyCurrentRoom)
    roomPtr = g_currentRoom;
  else
    roomPtr = findRoom(getLowestRoomNumber());


  while (roomPtr)
  {
    objectHere *obj = roomPtr->objectHead;

    while (obj)
    {
      if (obj->objectNumb == objNumber)
      {
        if (roomNumb)
          *roomNumb = roomPtr->roomNumber;

        return obj;
      }

      if (obj->objInside && !checkOnlyVisible)
      {
        objectHere *objH = checkAllObjInsideForVnum(obj->objInside, objNumber);

        if (objH)
        {
          if (roomNumb)
            *roomNumb = roomPtr->roomNumber;

          return objH;
        }
      }

      obj = obj->Next;
    }

    if (!checkOnlyVisible)
    {
      mobHere *mob = roomPtr->mobHead;

      while (mob)
      {
        if (mob->inventoryHead)
        {
          obj = mob->inventoryHead;

          while (obj)
          {
            if (obj->objectNumb == objNumber)
            {
              if (roomNumb)
                *roomNumb = roomPtr->roomNumber;

              return obj;
            }

            objectHere *objH = checkAllObjInsideForVnum(obj->objInside, objNumber);

            if (objH)
            {
              if (roomNumb)
                *roomNumb = roomPtr->roomNumber;

              return objH;
            }

            obj = obj->Next;
          }
        }

        for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
        {
          obj = mob->equipment[i];

          if (obj)
          {
            if (obj->objectNumb == objNumber)
            {
              if (roomNumb)
                *roomNumb = roomPtr->roomNumber;

              return obj;
            }

            objectHere *objH = checkAllObjInsideForVnum(obj->objInside, objNumber);

            if (objH)
            {
              if (roomNumb)
                *roomNumb = roomPtr->roomNumber;

              return objH;
            }
          }
        }

        mob = mob->Next;
      }
    }

    if (!checkOnlyCurrentRoom) 
      roomPtr = getNextRoom(roomPtr);
    else 
      return NULL;
  }

  return NULL;
}


//
// copyObjHere : Copies just one objectHere - doesn't inc loaded
//
//    *srcObjHere : copy it
//

objectHere *copyObjHere(const objectHere *srcObjHere)
{
  objectHere *newObjHere;


  if (!srcObjHere) 
    return NULL;

  newObjHere = new(std::nothrow) objectHere;
  if (!newObjHere)
  {
    displayAllocError("objectHere", "copyObjHere");

    return NULL;
  }

 // first, the easy stuff

  memcpy(newObjHere, srcObjHere, sizeof(objectHere));

  newObjHere->Next = NULL;

 // now, the not quite as easy but still relatively simple stuff

  newObjHere->objInside = copyObjHereList(srcObjHere->objInside, false);


  return newObjHere;
}


//
// copyObjHereList : Copies an objectHere list, returning the address of the
//                   new list
//
//    *srcObjHere : head of list
//      incLoaded : if true, incs loaded amounts
//

objectHere *copyObjHereList(const objectHere *srcObjHere, const bool incLoaded)
{
  objectHere *newObjHere, *prevObjHere = NULL, *headObjHere = NULL;


  if (!srcObjHere) 
    return NULL;

  while (srcObjHere)
  {
    newObjHere = new(std::nothrow) objectHere;
    if (!newObjHere)
    {
      displayAllocError("objectHere", "copyObjHereList");

      return headObjHere;
    }

    newObjHere->Next = NULL;

    if (!headObjHere) 
      headObjHere = newObjHere;

    if (prevObjHere) 
      prevObjHere->Next = newObjHere;

    prevObjHere = newObjHere;


   // first, the easy stuff

    memcpy(newObjHere, srcObjHere, sizeof(objectHere));

   // load before objInside is copied so that dupe loads don't occur

    if (incLoaded)
    {
      newObjHere->objInside = NULL;

      addObjLoad(newObjHere);
    }

   // now, the not quite as easy but still relatively simple stuff

    newObjHere->objInside = copyObjHereList(srcObjHere->objInside, incLoaded);

    srcObjHere = srcObjHere->Next;
  }

  return headObjHere;
}


//
// checkEntireObjHereListForType :
//      checks entire objectHere list for an objHere of type *objType - if one exists, returns true
//

bool checkEntireObjHereListForType(const objectHere *objHereHead, const objectType *objType)
{
  while (objHereHead)
  {
    if (objHereHead->objectPtr == objType) 
      return true;

    if (checkEntireObjHereListForType(objHereHead->objInside, objType))
      return true;

    objHereHead = objHereHead->Next;
  }

  return false;
}


//
// checkForObjHeresofType : Checks for any objectHere nodes anywhere in the zone and inventory that have objType
//

bool checkForObjHeresofType(const objectType *objType)
{
  const editableListNode *inv = g_inventory;


  if (getNumbEntities(ENTITY_OBJECT, objType->objNumber, false) > 0)
    return true;

 // check inventory

  while (inv)
  {
    if ((inv->entityType == ENTITY_OBJECT) && (((objectHere *)(inv->entityPtr))->objectPtr == objType))
      return true;

    inv = inv->Next;
  }

  return false;
}


//
// scanObjHereListForLoadedContainer :
//       Scans through a list of objectHeres and their objInside pointers
//       for containers of a certain vnum with items in them
//

bool scanObjHereListForLoadedContainer(const objectHere *objHere, const uint containNumb)
{
  while (objHere)
  {
    if ((objHere->objectNumb == containNumb) && objHere->objInside) 
      return true;

    if (scanObjHereListForLoadedContainer(objHere->objInside, containNumb))
      return true;

    objHere = objHere->Next;
  }

  return false;
}


//
// checkForObjHeresWithLoadedContainer :
//       Checks all objHeres in zone for containers of a certain vnum with items in them
//
//   containNumb : vnum of container to check for
//

bool checkForObjHeresWithLoadedContainer(const uint containNumb)
{
  const uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      const room *roomPtr = findRoom(roomNumb);

      const objectHere *objHere = roomPtr->objectHead;

      while (objHere)
      {
        if (scanObjHereListForLoadedContainer(objHere, containNumb))
          return true;

        objHere = objHere->Next;
      }

      const mobHere *mobH = roomPtr->mobHead;

      while (mobH)
      {
        if (scanObjHereListForLoadedContainer(mobH->inventoryHead, containNumb))
          return true;

        for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
          if (scanObjHereListForLoadedContainer(mobH->equipment[i], containNumb))
            return true;

        mobH = mobH->Next;
      }
    }
  }

  return false;
}


//
// resetObjHereList : resets list of objHeres having oldNumb to newNumb - doesn't adjust loaded list
//

void resetObjHereList(const uint oldNumb, const uint newNumb, objectHere *objHead)
{
  while (objHead)
  {
    if (objHead->objectNumb == oldNumb) 
      objHead->objectNumb = newNumb;

    resetObjHereList(oldNumb, newNumb, objHead->objInside);

    objHead = objHead->Next;
  }
}


//
// resetAllObjHere : resets all objHeres in zone having oldNumb to newNumb - doesn't adjust loaded list
//
//   oldNumb : see above
//   newNumb : ditto
//

void resetAllObjHere(const uint oldNumb, const uint newNumb)
{
  const uint highRoomNumb = getHighestRoomNumber();


  if (oldNumb == newNumb) 
    return;

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      resetObjHereList(oldNumb, newNumb, roomPtr->objectHead);

      mobHere *mob = roomPtr->mobHead;

      while (mob)
      {
        resetObjHereList(oldNumb, newNumb, mob->inventoryHead);

        for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
          resetObjHereList(oldNumb, newNumb, mob->equipment[i]);

        mob = mob->Next;
      }
    }
  }
}


//
// addObjHeretoList : adds already created objHere node to end of list, optionally incs loaded
//

void addObjHeretoList(objectHere **objListHead, objectHere *objToAdd, const bool incLoaded)
{
  objectHere *obj;


  if (!*objListHead) 
  {
    *objListHead = objToAdd;
  }
  else
  {
    obj = *objListHead;

    while (obj->Next)
      obj = obj->Next;

    obj->Next = objToAdd;
  }

  objToAdd->Next = NULL;

  if (incLoaded)
    addObjLoad(objToAdd);
}


//
// objinInv : checks mob's inventory (not equipment) for a particular object type vnum
//

const objectHere *objinInv(const mobHere *mob, const objectType *obj)
{
  if (!obj) 
    return NULL;

  return objinInv(mob, obj->objNumber);
}


//
// objinInv : check's mob's carried list for a particular object type (by vnum)
//

const objectHere *objinInv(const mobHere *mob, const uint objNumb)
{
  const objectHere *objH;

  if (!mob) 
    return NULL;

  objH = mob->inventoryHead;
  while (objH)
  {
    if (objH->objectNumb == objNumb) 
      return objH;

    objH = objH->Next;
  }

  return NULL;
}


//
// isObjTypeUsed : returns true if object type is loaded in zone or used by a shop or quest
//

bool isObjTypeUsed(const uint numb)
{
  const uint highMobNumb = getHighestMobNumber();


  if (getNumbEntities(ENTITY_OBJECT, numb, false)) 
    return true;

 // check quests and shops

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      if (mob->questPtr)
      {
        questQuest *qstq = mob->questPtr->questHead;

        while (qstq)
        {
          questItem *qsti = qstq->questPlayRecvHead;

          while (qsti)
          {
            if ((qsti->itemType == QUEST_RITEM_OBJ) && (qsti->itemVal == numb))
              return true;

            qsti = qsti->Next;
          }

          qstq = qstq->Next;
        }
      }

      if (mob->shopPtr)  // this code is semi-pointless, since it sets mobs' inv
      {                  // appropriately anyway when saving improperly set shops
        for (uint i = 0; (i < MAX_NUMBSHOPITEMS) && mob->shopPtr->producedItemList[i]; i++)
        {
          if (mob->shopPtr->producedItemList[i] == numb) 
            return true;
        }
      }
    }
  }

  return false;
}


//
// resetObjHEntityPointersByNumb : resets object type pointers of all objectHeres in list based on
//                                 objheres' objectNumbs
//

void resetObjHEntityPointersByNumb(objectHere *objH)
{
  while (objH)
  {
    resetObjHEntityPointersByNumb(objH->objInside);

    objH->objectPtr = findObj(objH->objectNumb);

    objH = objH->Next;
  }
}


//
// createObjectHereinRoom : Create a new objectHere and add it to current room's objectHere list
//
//    *obj : object type of objectHere
//    numb : vnum of objectHere being created
//

void createObjectHereinRoom(objectType *obj, const uint numb)
{
  objectHere *newObjHere;


 // create a new objectHere

  newObjHere = new(std::nothrow) objectHere;
  if (!newObjHere)
  {
    displayAllocError("objectHere", "createObjectHereinRoom");

    return;
  }

  memset(newObjHere, 0, sizeof(objectHere));

  newObjHere->objectNumb = numb;
  newObjHere->objectPtr = obj;

  newObjHere->randomChance = 100;

  addObjHeretoList(&g_currentRoom->objectHead, newObjHere, true);

  deleteMasterKeywordList(g_currentRoom->masterListHead);
  g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

  deleteEditableList(g_currentRoom->editableListHead);
  g_currentRoom->editableListHead = createEditableList(g_currentRoom);
}


//
// createObjHere : Create a new objectHere
//
//           numb : vnum of objectHere
//  incLoadedList : if true, increments stuff appropriately
//

objectHere *createObjHere(const uint numb, const bool incLoadedList)
{
  objectHere *newObjHere;


 // create a new objectHere

  newObjHere = new(std::nothrow) objectHere;
  if (!newObjHere)
  {
    displayAllocError("objectHere", "createObjHere");

    return NULL;
  }

  memset(newObjHere, 0, sizeof(objectHere));

  newObjHere->objectNumb = numb;
  newObjHere->objectPtr = findObj(numb);

  newObjHere->randomChance = 100;

  if (incLoadedList)
    addObjLoad(newObjHere);

  return newObjHere;
}
