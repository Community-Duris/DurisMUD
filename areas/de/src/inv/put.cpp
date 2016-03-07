//
//  File: put.cpp        originally part of durisEdit
//
//  Usage: functions for giving/putting objects
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

#include "types.h"
#include "fh.h"

extern editableListNode *g_inventory;
extern room *g_currentRoom;


//
// putEntityObj : put an object into an object list, remove from inventory if deleting original -
//                returns false if user aborted pause
//
//      if given is false, displays 'put into' else 'given to'
//

bool putEntityObj(objectHere *objH, objectHere **objHdest, const bool deleteOriginal, const bool given, 
                  const char *inStrn, size_t& numbLines)
{
  const char *copyStr;

 // if putting a copy, make a copy

  if (!deleteOriginal)
  {
    copyStr = "Copy of ";
    objH = copyObjHere(objH);
  }
  else
  {
    copyStr = "";
  }

  addObjHeretoList(objHdest, objH, true);

 // if putting for good, remove from inventory

  if (deleteOriginal)
  {
    editableListNode *inv = g_inventory;

    while (inv && (inv->entityPtr != objH))
      inv = inv->Next;

    deleteEditableinList(&g_inventory, inv);
  }

 // symbols on-screen

  char outstrn[MAX_OBJSNAME_LEN + MAX_OBJSNAME_LEN + MAX_MOBSNAME_LEN + 64];
  const char *givenStrn;

  if (given)
  {
    givenStrn = "given ";
  }
  else
  {
    givenStrn = "put in";
  }

  sprintf(outstrn, "%s'%s&n' %sto '%s&n'.\n", 
          copyStr, getObjShortName(objH->objectPtr), givenStrn, inStrn);

  return !checkPause(outstrn, numbLines);
}


//
// putEntityEdesc : add an extra desc to an object type
//

bool putEntityEdesc(extraDesc *desc, objectType *objType, const bool deleteOriginal, size_t& numbLines)
{
  const char *copyStr;

 // if putting a copy, make a copy

  if (!deleteOriginal)
  {
    copyStr = "Copy of extra";
    desc = copyOneExtraDesc(desc);
  }
  else
  {
    copyStr = "Extra";
  }

  addExtraDesctoList(&(objType->extraDescHead), desc);

 // if putting for good, remove from inventory

  if (deleteOriginal)
  {
    editableListNode *inv = g_inventory;

    while (inv && (inv->entityPtr != desc))
      inv = inv->Next;

    deleteEditableinList(&g_inventory, inv);
  }

 // update the master and editable lists of all rooms with objectheres

  updateAllObjMandElists();

  char outstrn[1536];
  char keystrn[1024];

  sprintf(outstrn, "%s desc '%s&n' added to '%s&n'.\n", 
          copyStr, getReadableKeywordStrn(desc->keywordListHead, keystrn, 1023), getObjShortName(objType));

  return !checkPause(outstrn, numbLines);
}


//
// putEntityCmd : Put/gives an entity into/to another
//
//           *args : string entered by user
//  deleteOriginal : if TRUE, original is deleted
//

void putEntityCmd(const char *args, const bool deleteOriginal)
{
  char argone[256], argtwo[256];
  editableListNode *containeeNode, *containerNode, *invNode, *nextInvNode;
  objectHere **putObjTargetListHead;  // pointer to head of object list in which to put putObj
  objectHere *objCont = NULL;
  mobHere *mobCont = NULL;
  const char *inStrn;
  char whatMatched;
  size_t numbLines = 0;
  bool given;  // if true, giving to mob, if false, putting in obj


  getArg(args, 1, argone, 255);
  getArg(args, 2, argtwo, 255);

  if ((argone[0] == '\0') || (argtwo[0] == '\0'))
  {
    _outtext(
"Not enough args - format is 'put <obj name/vnum> <target name/vnum>'.\n");

    return;
  }

 // find target container

  if (strnumer(argtwo))
  {
    const uint vnum = strtoul(argtwo, NULL, 10);

    objCont = findObjHere(vnum, NULL, true, true);

    if (!objCont)
    {
    // if no valid objHere found, look for mob

      mobCont = findMobHere(vnum, NULL, true);
    }
  }

 // if no vnum match, search by keyword

  if (!objCont && !mobCont)
  {
    checkEditableList(argtwo, g_currentRoom->editableListHead, &whatMatched, &containerNode, 1);

    if (whatMatched == ENTITY_OBJECT)
    {
      objCont = ((objectHere *)(containerNode->entityPtr));
      given = false;
    }
    else 
    if (whatMatched == ENTITY_MOB)
    {
      mobCont = ((mobHere *)(containerNode->entityPtr));
      given = true;
    }

   // 'container not found' case handled below

    else if (whatMatched != NO_MATCH)
    {
      char outstrn[256];

      sprintf(outstrn, "You can't put anything in %ss.\n", getEntityTypeStrn(whatMatched));
      _outtext(outstrn);

      return;
    }
  }

 // get appropriate objHere list and name of container

  if (objCont)
  {
    if (objCont->objectPtr && (objCont->objectPtr->objType == ITEM_CONTAINER))
    {
      putObjTargetListHead = &(objCont->objInside);
      inStrn = getObjShortName(objCont->objectPtr);
      given = false;
    }
    else
    {
      _outtext("That object is not a valid container.\n");
      return;
    }
  }
  else if (mobCont)
  {
    putObjTargetListHead = &(mobCont->inventoryHead);
    inStrn = getMobShortName(mobCont->mobPtr);
    given = true;
  }
  else
  {
    _outtext("Nothing in the room matches the second argument.\n");
    return;
  }

 // check if user is putting 'all'

  if (strcmpnocase(argone, "ALL"))
  {
    if (!g_inventory)
    {
      _outtext("You are not carrying anything.\n");
      return;
    }

   // have a target, run through inventory

    bool putOne = false;

    invNode = g_inventory;

    while (invNode)
    {
      nextInvNode = invNode->Next;

     // only objects can be put in anything, so only put objects in anything

      if (invNode->entityType == ENTITY_OBJECT)
      {
        if (!putEntityObj((objectHere *)(invNode->entityPtr), putObjTargetListHead, deleteOriginal, given,
                          inStrn, numbLines))
          return;

        putOne = true;
      }
      else 
      if (((invNode->entityType == ENTITY_R_EDESC) || (invNode->entityType == ENTITY_O_EDESC)) && objCont &&
          objCont->objectPtr)
      {
        if (!putEntityEdesc((extraDesc *)(invNode->entityPtr), objCont->objectPtr, deleteOriginal, numbLines))
          return;

        putOne = true;
      }

      invNode = nextInvNode;
    }

    if (!putOne)
      _outtext("No suitable objects found to place inside container.\n");

    return;
  }

 // check inventory for obj matching vnum in arg one

  if (strnumer(argone))
  {
    const uint vnum = strtoul(argone, NULL, 10);

    invNode = g_inventory;

   // can only ever put an object inside of something, so check only for objects

    while (invNode)
    {
      switch (invNode->entityType)
      {
        case ENTITY_OBJECT :
          if (((objectHere *)(invNode->entityPtr))->objectNumb == vnum)
          {
            putEntityObj((objectHere *)(invNode->entityPtr), putObjTargetListHead, deleteOriginal, given,
                         inStrn, numbLines);

            return;
          }

          break;
      }

      invNode = invNode->Next;
    }

   // not found, instead load object on the fly

    objectHere *newObjHere = createObjHere(vnum, false);  // putEntityObj() increments loaded count

    if (!newObjHere)
    {
      displayAllocError("objectHere", "putEntityCmd");
    }
    else
    {
     // deleteOriginal == false, so don't create copy, don't remove from inventory

      putEntityObj(newObjHere, putObjTargetListHead, false, given, inStrn, numbLines);
    }

    return;
  }

 // no vnum, check by keyword

  checkEditableList(argone, g_inventory, &whatMatched, &containeeNode, 1);

 // check type of thing to put in (containee) - only object is valid

  switch (whatMatched)
  {
    case NO_MATCH :
      _outtext("Nothing in your inventory matches the first argument.\n");
      return;

   // putting object in something

    case ENTITY_OBJECT :
      putEntityObj((objectHere *)(containeeNode->entityPtr), putObjTargetListHead, deleteOriginal, given,
                   inStrn, numbLines);
      return;

    case ENTITY_R_EDESC :
    case ENTITY_O_EDESC :
      if (!objCont || !objCont->objectPtr)
      {
        _outtext("You can only add extra descs to objects with types.\n");
        return;
      }

      putEntityEdesc((extraDesc *)(containeeNode->entityPtr), objCont->objectPtr, deleteOriginal, numbLines);
      return;

   // trying to put a non-object inside something

    default :
      char outstrn[256];
      const char *entityStrn = getEntityTypeStrn(whatMatched);
      
      sprintf(outstrn, "A%s %s cannot be placed in anything.\n", getVowelN(entityStrn[0]), entityStrn);
      _outtext(outstrn);

      return;
  }
}
