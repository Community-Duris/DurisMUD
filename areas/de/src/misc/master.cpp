//
//  File: master.cpp     originally part of durisEdit
//
//  Usage: functions for manipulating masterKeywordListNode records and
//         lists
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



#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "../fh.h"
#include "master.h"
#include "../edesc/edesc.h"
#include "../obj/objhere.h"
#include "../mob/mobhere.h"
#include "../room/room.h"


extern room *g_currentRoom;


const char *entityTypes[ENTITY_HIGHEST + 1] = 
{
  "object extra desc",
  "object",
  "room extra desc",
  "mob",
  "exit",
  "room",
  "zone",
  "zone file line",
  "quest",
  "shop",
  "inventory item"
};


//
// getEntityTypeStrn
//

const char *getEntityTypeStrn(const uchar entityType)
{
  if (entityType > ENTITY_HIGHEST)
    return "unrecognized";

  return entityTypes[entityType];
}


//
// createMasterNode : create master list node
//

masterKeywordListNode *createMasterListNode(const stringNode *keywords, const void *entityPtr, 
                                            const stringNode *desc, const char entityType)
{
  masterKeywordListNode *listNode = new(std::nothrow) masterKeywordListNode;

  if (!listNode)
  {
    displayAllocError("masterKeywordListNode", "createMasterNode");

    return NULL;
  }

  memset(listNode, 0, sizeof(masterKeywordListNode));

  listNode->keywordListHead = keywords;
  listNode->entityPtr = entityPtr;
  listNode->entityDesc = desc;
  listNode->entityType = entityType;

  return listNode;
}


//
// addMasterNodeToList : returns false if there's an error, true if success
//

bool addMasterNodeToList(masterKeywordListNode **head, masterKeywordListNode *nodeAdd)
{
  if (!head || !nodeAdd) 
    return false;

  if (!*head) 
  {
    *head = nodeAdd;
  }
  else
  {
    masterKeywordListNode *masterNodePrev = *head;

    while (masterNodePrev->Next)
      masterNodePrev = masterNodePrev->Next;

    masterNodePrev->Next = nodeAdd;
  }

  return true;
}


//
// createMasterKeywordList : Creates a "master keyword list" for the specified
//                           room node, returning a pointer to the head of the
//                           list.
//
//   *room : room to create master keyword list for
//

masterKeywordListNode *createMasterKeywordList(room *roomPtr)
{
  masterKeywordListNode *listHead = NULL, *listNode;
  extraDesc *extraDescNode;
  objectHere *objHere;
  mobHere *mobHNode;


 // first, grab all of the keyword lists for extra descs in the room

  extraDescNode = roomPtr->extraDescHead;

  while (extraDescNode)
  {
    listNode = createMasterListNode(extraDescNode->keywordListHead, extraDescNode, 
                                    extraDescNode->extraDescStrnHead, ENTITY_R_EDESC);

    if (!listNode)
    {
      displayAllocError("masterKeywordListNode", "createMasterKeywordList");

      return listHead;
    }

    addMasterNodeToList(&listHead, listNode);

    extraDescNode = extraDescNode->Next;
  }

 // now, grab all of the keyword lists for the objects in the room

 // first, create node for each edesc, then one for object itself (in case object has keywords that don't
 // match any of its edescs)

  objHere = roomPtr->objectHead;

  while (objHere)
  {
    if (!objHere->objectPtr)
    {
      objHere = objHere->Next;
      continue;
    }

    extraDescNode = objHere->objectPtr->extraDescHead;

   // create a master node for each edesc

    while (extraDescNode)
    {
      listNode = createMasterListNode(extraDescNode->keywordListHead, objHere, extraDescNode->extraDescStrnHead,
                                      ENTITY_OBJECT);

      if (!listNode)
      {
        displayAllocError("masterKeywordListNode", "createMasterKeywordList");

        return listHead;
      }

      addMasterNodeToList(&listHead, listNode);

      extraDescNode = extraDescNode->Next;
    }

    objHere = objHere->Next;
  }

 // put objects with no extra descs first

  objHere = roomPtr->objectHead;

  while (objHere)
  {
    if (!objHere->objectPtr || objHere->objectPtr->extraDescHead)
    {
      objHere = objHere->Next;
      continue;
    }

   // now, object-specific node

    listNode = createMasterListNode(objHere->objectPtr->keywordListHead, objHere, NULL, ENTITY_OBJECT);

    if (!listNode)
    {
      displayAllocError("masterKeywordListNode", "createMasterKeywordList");

      return listHead;
    }

    addMasterNodeToList(&listHead, listNode);

    objHere = objHere->Next;
  }

 // now do objects with extra descs

  objHere = roomPtr->objectHead;

  while (objHere)
  {
    if (!objHere->objectPtr || !objHere->objectPtr->extraDescHead)
    {
      objHere = objHere->Next;
      continue;
    }

   // now, object-specific node

    listNode = createMasterListNode(objHere->objectPtr->keywordListHead, objHere, NULL, ENTITY_OBJECT);

    if (!listNode)
    {
      displayAllocError("masterKeywordListNode", "createMasterKeywordList");

      return listHead;
    }

    addMasterNodeToList(&listHead, listNode);

    objHere = objHere->Next;
  }

 // finally, grab all the mob keywords

  mobHNode = roomPtr->mobHead;

  while (mobHNode)
  {
    if (!mobHNode->mobPtr)
    {
      mobHNode = mobHNode->Next;
      continue;
    }

    listNode = createMasterListNode(mobHNode->mobPtr->keywordListHead, mobHNode, mobHNode->mobPtr->mobDescHead,
                                    ENTITY_MOB);

    if (!listNode)
    {
      displayAllocError("masterKeywordListNode", "createMasterKeywordList");

      return listHead;
    }

    addMasterNodeToList(&listHead, listNode);

    mobHNode = mobHNode->Next;
  }


  return listHead;
}


//
// checkCurrentMasterKeywordList : 
//                          Scans through the master keyword list in
//                          g_currentRoom for the keyword specified by strn.
//                          whatMatched is set to the entity type
//                          that matched and a pointer to the entity
//                          description is returned.  returns a pointer to
//                          the description of entity matched (if any)
//
//          *strn : string as entered by user
//   *whatMatched : set based on what type of entity matched
// **matchingNode : upon a match, the matching node's address is put in this
//

const stringNode *checkCurrentMasterKeywordList(const char *strn, char *whatMatched, 
                                                masterKeywordListNode **matchingNode)
{
  char workstrn[1024];
  char *workptr = workstrn;
  const char *keyptr = workstrn;
  uint entityToGrab = 1, entityGrabbing = 1;
  masterKeywordListNode *masterNode = g_currentRoom->masterListHead;


  *whatMatched = NO_MATCH;
  *matchingNode = NULL;

  if (!strn[0]) 
    return NULL;

  strncpy(workstrn, strn, 1023);
  workstrn[1023] = '\0';

 // find any period and grab the #

  while (*workptr)
  {
    if (*workptr == '.')
    {
      *workptr = '\0';
      entityToGrab = strtoul(workstrn, NULL, 10);

      keyptr = workptr + 1;

      break;
    }

    workptr++;
  }

 // run through master nodes

  while (masterNode)
  {
    if (scanKeyword(keyptr, masterNode->keywordListHead))
    {
      if (entityToGrab == entityGrabbing)
      {
        *whatMatched = masterNode->entityType;

        *matchingNode = masterNode;

        return masterNode->entityDesc;
      }
      else 
      {
        entityGrabbing++;
      }
    }

    masterNode = masterNode->Next;
  }

  return NULL;
}


//
// deleteMasterKeywordNode
//

void deleteMasterKeywordNode(masterKeywordListNode *node)
{
  delete node;
}


//
// deleteMasterKeywordList : Deletes a master keyword list
//
//   *node : head of list to delete
//

void deleteMasterKeywordList(masterKeywordListNode *node)
{
  masterKeywordListNode *nextNode;


  while (node)
  {
    nextNode = node->Next;

    deleteMasterKeywordNode(node);

    node = nextNode;
  }
}
