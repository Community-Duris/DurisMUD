//
//  File: take.cpp        originally part of durisEdit
//
//  Usage: functions related to taking objects/mobs from mobs/objs/rooms
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
#include <ctype.h>

#include "de.h"
#include "fh.h"
#include "obj/objhere.h"
#include "mob/mobhere.h"
#include "defines.h"

extern editableListNode *g_inventory;
extern room *g_currentRoom;
extern bool g_madeChanges;


//
// takeEntityFromEntity : Picks up an entity based on the keywords in strings,
//                        checking *room's keyword list/vnums.  If deleteOriginal is
//                        true, the original is deleted.
//
//         objStrn : keyword/vnum ob object to get from container
//  deleteOriginal : if TRUE, deletes original object (getc vs get)
//   containerStrn : keyword/vnum of container
//

void takeEntityFromEntity(const char *objStrn, const bool deleteOriginal, const char *containerStrn)
{
  char contMatched = NO_MATCH;  // object or mob

  editableListNode *contNode;  // container info

  uint objnumb, contnumb;

  objectHere *objHere = NULL, **contObjList = NULL;
  objectType *contObjType;
  mobHere *mob = NULL;

  const char *contStrn;

  size_t numbLines = 0;


 // find container object

 // first try by vnum

  if (strnumer(containerStrn))
  {
   // user entered vnum for container

    contnumb = strtoul(containerStrn, NULL, 10);

    objHere = findObjHere(contnumb, NULL, true, true);

    if (!objHere)
    {
     // couldn't find obj container by vnum, search for mob by vnum

      mob = findMobHere(contnumb, NULL, true);
      if (mob)
      {
        contObjList = &(mob->inventoryHead);

        contStrn = getMobShortName(mob->mobPtr);
      }
    }
    else
    {
     // found container obj that matches by vnum

      contObjList = &(objHere->objInside);

      contStrn = getObjShortName(objHere->objectPtr);
    }
  }

 // no match, either user entered non-vnum or vnum didn't match, check by keyword

  if (!contObjList)
  {
    checkEditableList(containerStrn, g_currentRoom->editableListHead, &contMatched, &contNode, 1);

   // attempt to get object based on what matched for container (if anything)

    switch (contMatched)
    {
      case NO_MATCH : 
        _outtext("No match on second keyword (container/mob).\n");  
        return;

      case ENTITY_OBJECT :
        objHere = ((objectHere *)(contNode->entityPtr));

        contObjList = &(objHere->objInside);

        contStrn = getObjShortName(objHere->objectPtr);

        break;

      case ENTITY_MOB :
        mob = ((mobHere *)(contNode->entityPtr));

        contObjList = &(mob->inventoryHead);

        contStrn = getMobShortName(mob->mobPtr);

        break;

      default :
        char outstrn[256];

        sprintf(outstrn, "You can't get anything from %ss.\n", getEntityTypeStrn(contMatched));
        _outtext(outstrn);

        return;
    }
  }

  if (objHere)
  {
    contObjType = objHere->objectPtr;

   // if container is an object and the object has a type, ensure it's a container or that it has
   // extra descs

    if (contObjType && (contObjType->objType != ITEM_CONTAINER) && !contObjType->extraDescHead)
    {
      _outtext("That object is not a container and has no extra descs.\n");

      return;
    }
  }
  else
  {
    contObjType = NULL;
  }

 // check if container is empty/desc-less

  if (!*contObjList && (!contObjType || !contObjType->extraDescHead))
  {
    _outtext("Nothing is in that container/mob.\n");
    return;
  }

 // get thing to grab from container

 // check if grabbing all

  if (strcmpnocase(objStrn, "ALL"))
  {
    takeAllObj(contObjList, deleteOriginal, contStrn, numbLines);

    return;
  }

 // seek out the would-be target and procure it

 // first try by vnum

  objHere = NULL;

  if (strnumer(objStrn))
  {
   // user entered vnum for object

    objnumb = strtoul(objStrn, NULL, 10);

    objHere = *contObjList;

    while (objHere && (objHere->objectNumb != objnumb))
      objHere = objHere->Next;
  }

 // no match, either user entered non-vnum or vnum didn't match, check for object by keyword

  if (!objHere)
  {
    objHere = *contObjList;

    while (objHere)
    {
      if (objHere->objectPtr && scanKeyword(objStrn, objHere->objectPtr->keywordListHead))
        break;

      objHere = objHere->Next;
    }
  }

 // no match, now check for extra desc

  extraDesc *desc = NULL;

  if (!objHere && contObjType)
  {
    extraDesc *descPtr = contObjType->extraDescHead;

    while (descPtr)
    {
      if (scanKeyword(objStrn, contObjType->keywordListHead))
      {
        desc = descPtr;
        break;
      }

      descPtr = descPtr->Next;
    }
  }

  if (!desc && !objHere)
  {
    _outtext("Nothing in that container/mob matches that vnum/keyword.\n");
    return;
  }

 // match!

  if (objHere)
    takeEntityObj(objHere, contObjList, deleteOriginal, contStrn, numbLines);
  else
    takeEntityObjEdesc(contObjType, desc, deleteOriginal);
}


//
// takeEntityRoomEdesc : take a room extra desc - returns false on error
//

bool takeEntityRoomEdesc(extraDesc *desc, const bool deleteOriginal)
{
 // create new inventory entry

  editableListNode *matchingNode = 
    createEditableListNode(desc->keywordListHead, desc, ENTITY_R_EDESC);

  if (!matchingNode)
  {
    displayAllocError("editableListNode", "takeEntityRoomEdesc");

    return false;
  }

 // stick node in inventory

  addEditabletoList(&g_inventory, matchingNode);

 // if deleteOriginal is true, remove from current room and rebuild list, else make new copy of edesc
 // for inventory node

  const char *tookWhat;

  if (deleteOriginal)
  {
    removeExtraDescfromList(&(g_currentRoom->extraDescHead), desc);

   // not necessary if taking an object from a container, but this is easiest

    deleteMasterKeywordList(g_currentRoom->masterListHead);
    g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

    deleteEditableList(g_currentRoom->editableListHead);
    g_currentRoom->editableListHead = createEditableList(g_currentRoom);

    tookWhat = "Room";
  }
  else
  {
    matchingNode->entityPtr = copyOneExtraDesc(desc);

    tookWhat = "Copy of room";
  }

 // visual feedback

  char outstrn[1536];  // this shall be long enough
  char keystrn[1024];

  sprintf(outstrn, "%s extra desc '%s' taken.\n",
          tookWhat, getReadableKeywordStrn(desc->keywordListHead, keystrn, 1023));

  displayColorString(outstrn);

  g_madeChanges = true;

  return true;
}


//
// takeEntityObjEdesc : take an object extra desc - returns false on error
//

bool takeEntityObjEdesc(objectType *obj, extraDesc *desc, const bool deleteOriginal)
{
  const char *tookWhat;

 // create new inventory entry

  editableListNode *matchingNode = 
    createEditableListNode(desc->keywordListHead, desc, ENTITY_O_EDESC);

  if (!matchingNode)
  {
    displayAllocError("editableListNode", "takeEntityObjEdesc");

    return false;
  }

 // stick node in inventory

  addEditabletoList(&g_inventory, matchingNode);

 // if obj is NULL, figure out which object it came from - must be in current room

  if (!obj)
  {
    objectHere *objH = g_currentRoom->objectHead;

    while (objH)
    {
      extraDesc *descSearch = NULL;

      if (objH->objectPtr)
      {
        obj = objH->objectPtr;

        descSearch = obj->extraDescHead;

        while (descSearch && (descSearch != desc))
          descSearch = descSearch->Next;
      }

      if (descSearch)
        break;

      objH = objH->Next;
    }
  }

 // if deleteOriginal is true, remove from object type and rebuild lists, else make new copy of edesc
 // for inventory node

  if (deleteOriginal)
  {
    removeExtraDescfromList(&(obj->extraDescHead), desc);

   // rebuild master and editable lists of every room with objects

    updateAllObjMandElists();

    tookWhat = "Extra";
  }
  else
  {
    matchingNode->entityPtr = copyOneExtraDesc(desc);

    tookWhat = "Copy of extra";
  }

 // visual feedback

  char outstrn[1536];  // this shall be long enough
  char keystrn[1024];

  sprintf(outstrn, "%s desc '%s' taken from object type '%s&n'.\n",
          tookWhat, getReadableKeywordStrn(desc->keywordListHead, keystrn, 1023), getObjShortName(obj));
  displayColorString(outstrn);

  g_madeChanges = true;

  return true;
}


//
// takeEntityObj : take an objhere - returns false on error/pause abortion
//

bool takeEntityObj(objectHere *objH, objectHere **objHsrc, const bool deleteOriginal, const char *fromStrn,
                   size_t& numbLines)
{
  const char *copyOf;


 // create new inventory entry

  editableListNode *matchingNode = 
    createEditableListNode(objH->objectPtr ? objH->objectPtr->keywordListHead : NULL,
                           objH, ENTITY_OBJECT);

  if (!matchingNode)
  {
    displayAllocError("editableListNode", "takeEntityObj");

    return false;
  }

 // stick node in inventory

  addEditabletoList(&g_inventory, matchingNode);

 // if deleteOriginal is true, remove from room and rebuild list, else make new copy of objHere
 // for inventory node

  if (deleteOriginal)
  {
    removeObjHerefromList(objHsrc, objH, true);

   // not necessary if taking an object from a container, but this is easiest

    deleteMasterKeywordList(g_currentRoom->masterListHead);
    g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

    deleteEditableList(g_currentRoom->editableListHead);
    g_currentRoom->editableListHead = createEditableList(g_currentRoom);

    copyOf = "";
  }
  else
  {
    matchingNode->entityPtr = copyObjHere(objH);

    copyOf = "Copy of ";
  }

  g_madeChanges = true;

 // visual feedback

  char outstrn[MAX_OBJSNAME_LEN + MAX_OBJSNAME_LEN + MAX_MOBSNAME_LEN + 64];  // this shall be long enough

  if (fromStrn)
    sprintf(outstrn, "%s'%s&n' taken from '%s&n'.\n",
            copyOf, getObjShortName(objH->objectPtr), fromStrn);
  else
    sprintf(outstrn, "%s'%s&n' taken.\n",
            copyOf, getObjShortName(objH->objectPtr));

  return !checkPause(outstrn, numbLines);
}


//
// takeEntityMob : take a mobhere - returns false on fatal error (memory alloc)/pause abortion
//

bool takeEntityMob(mobHere *mobH, const bool deleteOriginal, size_t& numbLines)
{
  const char *copyOf;


 // picking up riders, mounts, leaders, followers, or copies of shops is not allowed due to
 // impossibility

  if (mobH->riding || mobH->riddenBy || mobH->following || (getNumbFollowers(mobH) >= 1))
  {
    _outtext(
"You cannot pick up mobs that are following another mob or being ridden,\n"
"riding, or followed.\n");

    return true;
  }

  if (mobH->mobPtr && mobH->mobPtr->shopPtr && !deleteOriginal)
  {
    _outtext(
"You cannot pick up copies of mobs with shops.\n");

    return true;
  }

 // if matching node came from room list, create a copy

  editableListNode *matchingNode = 
    createEditableListNode(mobH->mobPtr ? mobH->mobPtr->keywordListHead : NULL,
                           mobH, ENTITY_MOB);

  if (!matchingNode)
  {
    displayAllocError("editableListNode", "takeEntityMob");

    return false;
  }

 // stick node in inventory

  addEditabletoList(&g_inventory, matchingNode);

 // if deleteOriginal is true, remove from room and rebuild list, else make new copy of mobHere
 // for inventory node

  if (deleteOriginal)
  {
    removeMobHerefromList(&(g_currentRoom->mobHead), mobH, true);

    deleteMasterKeywordList(g_currentRoom->masterListHead);
    g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

    deleteEditableList(g_currentRoom->editableListHead);
    g_currentRoom->editableListHead = createEditableList(g_currentRoom);

    copyOf = "";
  }
  else
  {
    matchingNode->entityPtr = copyMobHere(mobH);

    copyOf = "Copy of ";
  }

  g_madeChanges = true;

 // display message

  char outstrn[MAX_MOBSNAME_LEN + 64];

  sprintf(outstrn, "%s'%s&n' taken.\n", copyOf, getMobShortName(mobH->mobPtr));
  return !checkPause(outstrn, numbLines);
}


//
// takeEntityCmd : Picks up the entity in g_currentRoom based on the keyword 
//                 in *strn, checking checking room's keyword list.  If 
//                 deleteOriginal is true, the original is deleted (woah).
//
//       *origargs : string entered by user
//  deleteOriginal : if TRUE, deletes original object (getc vs get)
//

void takeEntityCmd(const char *args, const bool deleteOriginal)
{
  char strn1[256], strn2[256];
  editableListNode *matchingNode;
  objectHere *objHere;
  mobHere *mob;
  uint vnum;
  size_t numbLines = 0;


 // if more than one arg, taking obj from mob/obj

  if (numbArgs(args) >= 2)
  {
    takeEntityFromEntity(getArg(args, 1, strn1, 255), deleteOriginal, getArg(args, 2, strn2, 255));
    return;
  }

 // take all mobs and objects in room

  if (strcmpnocase(args, "ALL"))
  {
    const bool blnHadObjects = (g_currentRoom->objectHead != NULL);  // to properly echo 'nothing to take' message
    bool userAborted = false;

    if (g_currentRoom->objectHead)
      userAborted = !takeAllObj(&(g_currentRoom->objectHead), deleteOriginal, NULL, numbLines);

    if (!userAborted && (g_currentRoom->mobHead || !blnHadObjects))
      takeAllMob(&(g_currentRoom->mobHead), deleteOriginal, numbLines);

    deleteMasterKeywordList(g_currentRoom->masterListHead);
    g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

    deleteEditableList(g_currentRoom->editableListHead);
    g_currentRoom->editableListHead = createEditableList(g_currentRoom);

    return;
  }

 // check if user entered vnum

  if (strnumer(args))
  {
    vnum = strtoul(args, NULL, 10);

    objHere = findObjHere(vnum, NULL, true, true);

   // couldn't find object that matches vnum, search for mob

    if (!objHere)
    {
      mob = findMobHere(vnum, NULL, true);
      if (mob)
      {
        takeEntityMob(mob, deleteOriginal, numbLines);

        return;
      }
    }
    else
    {
     // found objHere match by vnum

      takeEntityObj(objHere, &(g_currentRoom->objectHead), deleteOriginal, NULL, numbLines);

      return;
    }
  }

 // user entered keyword, or vnum didn't match

  char whatMatched;

  checkEditableList(args, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

  switch (whatMatched)
  {
    case NO_MATCH : 
      _outtext("No match found for that keyword.\n");  
      return;

    case ENTITY_R_EDESC :
      takeEntityRoomEdesc((extraDesc *)(matchingNode->entityPtr), deleteOriginal);

      return;

    case ENTITY_O_EDESC :
      takeEntityObjEdesc(NULL, (extraDesc *)(matchingNode->entityPtr), deleteOriginal);

      return;

    case ENTITY_OBJECT :
      takeEntityObj((objectHere *)(matchingNode->entityPtr), &(g_currentRoom->objectHead), deleteOriginal, NULL,
                    numbLines);

      return;

    case ENTITY_MOB :
      takeEntityMob((mobHere *)(matchingNode->entityPtr), deleteOriginal, numbLines);

      return;

    default :
      char strn[256];

      sprintf(strn, "%ss cannot be taken.\n", getEntityTypeStrn(whatMatched));
      strn[0] = toupper(strn[0]);
      _outtext(strn);

      return;
  }
}


//
// takeAllObj : returns false if user aborts in pause
//

bool takeAllObj(objectHere **objHereHead, const bool deleteOriginal, const char *fromStrn, size_t &numbLines)
{
  objectHere *anotherObjHere = *objHereHead, *objNext;  // necessary to keep old link when removing


  if (!anotherObjHere)
  {
    checkPause("There are no objects to take.\n", numbLines);
    return true;
  }

  while (anotherObjHere)
  {
    objNext = anotherObjHere->Next;

    if (!takeEntityObj(anotherObjHere, objHereHead, deleteOriginal, fromStrn, numbLines))
      return false;

    anotherObjHere = objNext;
  }

  return true;
}


//
// takeAllMob : returns false if user aborts in pause
//

bool takeAllMob(mobHere **mobHereHead, const bool deleteOriginal, size_t &numbLines)
{
  mobHere *anotherMobHere = *mobHereHead, *mobNext;  // necessary to keep old link when removing


  if (!anotherMobHere)
  {
    checkPause("There is nothing to take.\n", numbLines);
    return true;
  }

  while (anotherMobHere)
  {
    mobNext = anotherMobHere->Next;

    if (!takeEntityMob(anotherMobHere, deleteOriginal, numbLines))
      return false;

    anotherMobHere = mobNext;
  }

  return true;
}
