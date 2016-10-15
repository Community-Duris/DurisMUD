//
//  File: purge.cpp      originally part of durisEdit
//
//  Usage: functions for handling the 'purge' command
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
#include <ctype.h>
#include <stdlib.h>

#include "../fh.h"
#include "../types.h"
#include "../keys.h"

#include "../room/room.h"
#include "../obj/objhere.h"
#include "../mob/mobhere.h"
#include "../zone/zone.h"
#include "../misc/editable.h"

extern room *g_currentRoom;
extern bool g_madeChanges;
extern editableListNode *g_inventory;
extern command g_purgeCommands[];

//
// purge
//

#define PURGE_VALID_ARG_STR  "\n" \
"No objects/mobs matching keyword/vnum specified found - the format of the\n" \
"'purge' command is 'purge [keyword|vnum|all|allobj|allmob|inv]'.\n\n"

void purge(const char *args)
{
 // user entered "purge", with no arguments

  if (!strlen(args))
  {
    if (!g_currentRoom->objectHead && !g_currentRoom->mobHead)
    {
      _outtext("\nThere is nothing in this room to purge.\n\n");
      return;
    }

    if (displayYesNoPrompt("\n&+cAre you sure you want to purge all mobs and objects in this room", 
                           promptNo, false) == promptNo)
    {
      return;
    }

    deleteObjHereList(g_currentRoom->objectHead, true);
    g_currentRoom->objectHead = NULL;

    deleteMobHereList(g_currentRoom->mobHead, true);
    g_currentRoom->mobHead = NULL;

    deleteMasterKeywordList(g_currentRoom->masterListHead);
    g_currentRoom->masterListHead = NULL;

    deleteEditableList(g_currentRoom->editableListHead);
    g_currentRoom->editableListHead = NULL;

    g_madeChanges = true;

    _outtext("All objects and mobs in current room deleted.\n\n");
    return;
  }

 // now, check args against keywords of mobs/objs in room

  objectHere *obj = NULL;
  mobHere *mob = NULL;

  if (!strnumber(args))
  {
    editableListNode *matchingNode;
    char whatMatched;

    checkEditableList(args, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

    switch (whatMatched)
    {
     // if no keyword match, fall through to special keyword checking

      case NO_MATCH : 
        break;

      case ENTITY_OBJECT :
        obj = (objectHere *)(matchingNode->entityPtr);
        break;

      case ENTITY_MOB :
        mob = (mobHere *)(matchingNode->entityPtr);
        break;

      default :
        char outstrn[256];

        sprintf(outstrn, "\nYou cannot purge %ss.\n\n", getEntityTypeStrn(whatMatched));
        _outtext(outstrn);

        return;
    }
  }
  else

 // arg is numeric, check by vnum

  {
    const uint vnum = strtoul(args, NULL, 10);

    obj = g_currentRoom->objectHead;

    while (obj)
    {
      if (obj->objectNumb == vnum)
        break;

      obj = obj->Next;
    }

    if (!obj)
    {
      mob = g_currentRoom->mobHead;

      while (mob)
      {
        if (mob->mobNumb == vnum)
          break;

        mob = mob->Next;
      }
    }
  }

 // found an obj

  if (obj)
  {
    char outstrn[MAX_OBJSNAME_LEN + 128];

    sprintf(outstrn, "\nObject '%s&n' (#%u) purged from current room.\n\n",
           getObjShortName(obj->objectPtr), obj->objectNumb);

    displayColorString(outstrn);

    deleteObjHereinList(&(g_currentRoom->objectHead), obj, true);

    deleteMasterKeywordList(g_currentRoom->masterListHead);
    g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

    deleteEditableList(g_currentRoom->editableListHead);
    g_currentRoom->editableListHead = createEditableList(g_currentRoom);

    g_madeChanges = true;

    return;
  }

 // found a mob

  if (mob)
  {
    if (mob->riding || mob->riddenBy || (getNumbFollowers(mob) >= 1))
    {
     _outtext("\n"
"You cannot purge mobs that are riding, being followed, or ridden.\n\n");

     return;
    }

    char outstrn[MAX_MOBSNAME_LEN + 128];

    sprintf(outstrn, "\nMob '%s&n' (#%u) purged from current room.\n\n",
           getMobShortName(mob->mobPtr), mob->mobNumb);

    displayColorString(outstrn);

    deleteMobHereinList(&(g_currentRoom->mobHead), mob, true);

    deleteMasterKeywordList(g_currentRoom->masterListHead);
    g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

    deleteEditableList(g_currentRoom->editableListHead);
    g_currentRoom->editableListHead = createEditableList(g_currentRoom);

    g_madeChanges = true;

    return;
  }

  checkCommands(args, g_purgeCommands, PURGE_VALID_ARG_STR, purgeExecCommand, NULL, NULL);
}


//
// purgeAll : if user typed ALL, delete all mobs and objs loaded in zone
//

void purgeAll(void)
{
  const uint highRoomNumb = getHighestRoomNumber();


  if (displayYesNoPrompt("\n&+cDelete all objects and mobs loaded in zone", promptNo, false) == promptNo)
  {
    return;
  }

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      if (roomPtr->objectHead || roomPtr->mobHead)
        g_madeChanges = true;

      deleteObjHereList(roomPtr->objectHead, true);
      roomPtr->objectHead = NULL;

      deleteMobHereList(roomPtr->mobHead, true);
      roomPtr->mobHead = NULL;
    }
  }

  updateAllObjMandElists();
  updateAllMobMandElists();

  createPrompt();

  displayColorString("&+cAll objects and mobs loaded in zone deleted.&n\n\n");
}


//
// purgeAllMob : delete all mobs
//

void purgeAllMob(void)
{
  const uint highRoomNumb = getHighestRoomNumber();


  if (displayYesNoPrompt("\n&+cDelete all mobs loaded in zone", promptNo, false) == promptNo)
  {
    return;
  }

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      if (roomPtr->mobHead)
        g_madeChanges = true;

      deleteMobHereList(roomPtr->mobHead, true);
      roomPtr->mobHead = NULL;
    }
  }

  updateAllMobMandElists();  // update rooms with mobs
  createPrompt();

  displayColorString("&+cAll mobs loaded in zone deleted.&n\n\n");
}


//
// purgeAllObj : delete all objects
//

void purgeAllObj(void)
{
  const uint highRoomNumb = getHighestRoomNumber();


  if (displayYesNoPrompt("\n&+cDelete all objects loaded in zone", promptNo, false) == promptNo)
  {
    return;
  }

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      if (roomPtr->objectHead)
        g_madeChanges = true;

      deleteObjHereList(roomPtr->objectHead, true);
      roomPtr->objectHead = NULL;

      mobHere *mob = roomPtr->mobHead;
      while (mob)
      {
        for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
        {
          if (mob->equipment[i])
          {
            deleteObjHere(mob->equipment[i], true);
            mob->equipment[i] = NULL;

            g_madeChanges = true;
          }
        }

        if (mob->inventoryHead)
          g_madeChanges = true;

        deleteObjHereList(mob->inventoryHead, true);
        mob->inventoryHead = NULL;

        mob = mob->Next;
      }
    }
  }

  updateAllObjMandElists();  // update rooms with objects
  createPrompt();

  displayColorString("&+cAll objects loaded in zone deleted.&n\n\n");
}


//
// purgeInv : if user typed INV, delete everything in inventory
//

void purgeInv(void)
{
  if (!g_inventory)
  {
    _outtext("\nYou have no inventory to purge.\n\n");
    return;
  }

  deleteEditableList(g_inventory);
  g_inventory = NULL;

  _outtext("\nYour inventory has been deleted.\n\n");

  g_madeChanges = true;
}
