//
//  File: drop.cpp        originally part of durisEdit
//
//  Usage: functions related to dropping objects/mobs
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

#include "../de.h"
#include "../fh.h"
#include "../obj/objhere.h"
#include "../mob/mobhere.h"

extern editableListNode *g_inventory;
extern room *g_currentRoom;

//
// dropEntityObj : drop an object - returns false if user aborted
//

bool dropEntityObj(objectHere *objH, objectHere **objHdest, const bool deleteOriginal, size_t &numbLines)
{
  const char *copyOf;

 // if not deleting the original, make a copy

  if (!deleteOriginal)
  {
    copyOf = "Copy of ";

    objH = copyObjHere(objH);
  }
  else
  {
    copyOf = "";
  }

  objH->Next = NULL;

 // add object to room's object list

  addObjHeretoList(objHdest, objH, true);

 // rebuild room's master/editable lists

  deleteMasterKeywordList(g_currentRoom->masterListHead);
  g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

  deleteEditableList(g_currentRoom->editableListHead);
  g_currentRoom->editableListHead = createEditableList(g_currentRoom);

 // if dropping for good, remove from inventory

  if (deleteOriginal)
  {
    editableListNode *inv = g_inventory;

    while (inv && (inv->entityPtr != objH))
      inv = inv->Next;

    deleteEditableinList(&g_inventory, inv);
  }

 // visual feedback

  char outstrn[MAX_OBJSNAME_LEN + 64];

  sprintf(outstrn, "%s'%s&n' dropped.\n", copyOf, getObjShortName(objH->objectPtr));

  return !checkPause(outstrn, numbLines);
}


//
// dropEntityMob : drop a mob - returns false if user aborted
//

bool dropEntityMob(mobHere *mobH, mobHere **mobHdest, const bool deleteOriginal, size_t& numbLines)
{
  const char *copyOf;


 // can't drop shop mob in a room that already has a shop

  if (mobH->mobPtr && mobH->mobPtr->shopPtr && getShopinCurrentRoom())
  {
    _outtext("You cannot drop a shop mob in a room that already has one.\n");

    return true;
  }

 // if not deleting the original, make a copy

  if (!deleteOriginal)
  {
   // can't drop copies of shops
  
    if (mobH->mobPtr && mobH->mobPtr->shopPtr)
    {
      _outtext("You cannot drop copies of mobs with shops.\n");

      return true;
    }

    mobH = copyMobHere(mobH);

    copyOf = "Copy of ";
  }
  else
  {
    copyOf = "";
  }

  mobH->Next = NULL;

 // add mob to room's mob list

  addMobHeretoList(mobHdest, mobH, true);

 // recreate room's master/editable lists

  deleteMasterKeywordList(g_currentRoom->masterListHead);
  g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

  deleteEditableList(g_currentRoom->editableListHead);
  g_currentRoom->editableListHead = createEditableList(g_currentRoom);

 // if dropping for good, remove from inventory

  if (deleteOriginal)
  {
    editableListNode *inv = g_inventory;

    while (inv && (inv->entityPtr != mobH))
      inv = inv->Next;

    deleteEditableinList(&g_inventory, inv);
  }

 // visual feedback

  char outstrn[MAX_MOBSNAME_LEN + 64];

  sprintf(outstrn, "%s'%s&n' dropped.\n", copyOf, getMobShortName(mobH->mobPtr));

  return !checkPause(outstrn, numbLines);
}


//
// dropEntityEdesc : drop an extra desc into the current room - returns false if user aborted
//

bool dropEntityEdesc(extraDesc *desc, const bool deleteOriginal, size_t& numbLines)
{
  const char *copyOf;


 // if not deleting the original, make a copy

  if (!deleteOriginal)
  {
    desc = copyOneExtraDesc(desc);

    copyOf = "Copy of extra";
  }
  else
  {
    copyOf = "Extra";
  }

  desc->Next = NULL;

 // add mob to room's mob list

  addExtraDesctoList(&(g_currentRoom->extraDescHead), desc);

 // recreate room's master/editable lists

  deleteMasterKeywordList(g_currentRoom->masterListHead);
  g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

  deleteEditableList(g_currentRoom->editableListHead);
  g_currentRoom->editableListHead = createEditableList(g_currentRoom);

 // if dropping for good, remove from inventory

  if (deleteOriginal)
  {
    editableListNode *inv = g_inventory;

    while (inv && (inv->entityPtr != desc))
      inv = inv->Next;

    deleteEditableinList(&g_inventory, inv);
  }

 // visual feedback

  char outstrn[1536];
  char keystrn[1024];

  sprintf(outstrn, "%s desc '%s&n' dropped.\n", 
          copyOf, getReadableKeywordStrn(desc->keywordListHead, keystrn, 1023));

  return !checkPause(outstrn, numbLines);
}


//
// dropEntityCmd : Copies an entity from user's inventory to g_currentRoom
//
//           *args : string entered by user
//  deleteOriginal : if TRUE, deletes original from inventory (drop vs dropc)
//

void dropEntityCmd(const char *args, const bool deleteOriginal)
{
  editableListNode *matchingNode, *invNode;
  uint vnum;
  size_t numbLines = 0;


 // dropping all

  if (strcmpnocase(args, "ALL"))
  {
    if (!g_inventory)
    {
      _outtext("You are not carrying anything.\n");
      return;
    }

    invNode = g_inventory;

    while (invNode)
    {
      switch (invNode->entityType)
      {
        case ENTITY_OBJECT :
          if (!dropEntityObj((objectHere *)(invNode->entityPtr), &(g_currentRoom->objectHead), deleteOriginal,
                             numbLines))
            return;

          break;

        case ENTITY_MOB :
          if (!dropEntityMob((mobHere *)(invNode->entityPtr), &(g_currentRoom->mobHead), deleteOriginal,
                             numbLines))
            return;

          break;

        case ENTITY_R_EDESC :
        case ENTITY_O_EDESC :
          if (!dropEntityEdesc((extraDesc *)(invNode->entityPtr), deleteOriginal, numbLines))
            return;

          break;
      }

      invNode = invNode->Next;
    }

    return;
  }

 // dropping by vnum

  if (strnumber(args))
  {
    vnum = strtoul(args, NULL, 10);

    invNode = g_inventory;

    while (invNode)
    {
      switch (invNode->entityType)
      {
        case ENTITY_OBJECT :
          if (((objectHere *)(invNode->entityPtr))->objectNumb == vnum)
          {
            dropEntityObj((objectHere *)(invNode->entityPtr), &(g_currentRoom->objectHead), deleteOriginal,
                          numbLines);

            return;
          }

          break;

        case ENTITY_MOB    :
          if (((mobHere *)(invNode->entityPtr))->mobNumb == vnum)
          {
            dropEntityMob((mobHere *)(invNode->entityPtr), &(g_currentRoom->mobHead), deleteOriginal,
                          numbLines);

            return;
          }

          break;
      }

      invNode = invNode->Next;
    }
  }

 // no vnum match, look for keyword match

  char whatMatched;

  checkEditableList(args, g_inventory, &whatMatched, &matchingNode, 1);

 // drop it

  switch (whatMatched)
  {
    case NO_MATCH : 
      _outtext("No match found for that vnum/keyword.\n");  
      return;

    case ENTITY_OBJECT :
      dropEntityObj((objectHere *)(matchingNode->entityPtr), &(g_currentRoom->objectHead), deleteOriginal,
                    numbLines);
      return;

    case ENTITY_MOB :
      dropEntityMob((mobHere *)(matchingNode->entityPtr), &(g_currentRoom->mobHead), deleteOriginal,
                    numbLines);
      return;

    case ENTITY_R_EDESC :
    case ENTITY_O_EDESC :
      dropEntityEdesc((extraDesc *)(matchingNode->entityPtr), deleteOriginal, numbLines);

      return;
  }
}
