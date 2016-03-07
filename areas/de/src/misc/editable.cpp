//
//  File: editable.cpp   originally part of durisEdit
//
//  Usage: functions for manipulating editableListNode records
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
#include <string.h>
#include <ctype.h>

#include "types.h"
#include "fh.h"

#include "editable.h"
#include "master.h"




//
// createEditableListNode
//

editableListNode *createEditableListNode(const stringNode *keywordList, void *entityPtr, const char entityType)
{
  editableListNode *node;


  node = new(std::nothrow) editableListNode;
  if (!node)
  {
    displayAllocError("editableListNode", "createEditableListNode");

    return NULL;
  }

  memset(node, 0, sizeof(editableListNode));

  node->keywordListHead = keywordList;
  node->entityPtr = entityPtr;
  node->entityType = entityType;

  return node;
}


//
// createEditableListNode : keywordList is copied if non-NULL
//

editableListNode *createEditableListNode(const editableListNode *srcNode)
{
  editableListNode *node;


  node = new(std::nothrow) editableListNode;
  if (!node)
  {
    displayAllocError("editableListNode", "createEditableListNode");

    return NULL;
  }

  memcpy(node, srcNode, sizeof(editableListNode));

  return node;
}


//
// addEditabletoList
//

void addEditabletoList(editableListNode **edListHead, editableListNode *edToAdd)
{
  editableListNode *ed;


  if (!*edListHead) 
  {
    *edListHead = edToAdd;
  }
  else
  {
    ed = *edListHead;

    while (ed->Next)
      ed = ed->Next;

    ed->Next = edToAdd;
  }

  edToAdd->Next = NULL;
}


//
// createEditableList : Creates an "editable entities list" for the specified
//                      room, returning a pointer to the head of the editable
//                      list.
//
//   *roomPtr : room for which to create editable list
//

editableListNode *createEditableList(room *roomPtr)
{
  editableListNode *listHead = NULL;
  extraDesc *extraDescNode;
  objectHere *objHere;
  mobHere *mobHNode;


 // first, grab all of the keyword lists for extra descs in the room

  extraDescNode = roomPtr->extraDescHead;

  while (extraDescNode)
  {
    editableListNode *listNode = 
      createEditableListNode(extraDescNode->keywordListHead, extraDescNode, ENTITY_R_EDESC);

    if (!listNode)
    {
      displayAllocError("editableListNode", "createEditableList");

      return listHead;
    }

    addEditabletoList(&listHead, listNode);

    extraDescNode = extraDescNode->Next;
  }

 // now, grab all of the keyword lists for the objects in the room

  objHere = roomPtr->objectHead;

  while (objHere)
  {
    if (objHere->objectPtr)
    {
      editableListNode *listNode = 
        createEditableListNode(objHere->objectPtr->keywordListHead, objHere, ENTITY_OBJECT);

      if (!listNode)
      {
        displayAllocError("editableListNode", "createEditableList");

        return listHead;
      }

      addEditabletoList(&listHead, listNode);
    }

    objHere = objHere->Next;
  }

 // grab extra descs after all object keywords

  objHere = roomPtr->objectHead;

  while (objHere)
  {
    if (objHere->objectPtr)
    {
     // copy extra desc node stuff

      extraDescNode = objHere->objectPtr->extraDescHead;

      while (extraDescNode)
      {
        editableListNode *listNode = 
          createEditableListNode(extraDescNode->keywordListHead, extraDescNode, ENTITY_O_EDESC);

        if (!listNode)
        {
          displayAllocError("editableListNode", "createEditableList");

          return listHead;
        }

        addEditabletoList(&listHead, listNode);

        extraDescNode = extraDescNode->Next;
      }
    }

    objHere = objHere->Next;
  }

 // finally, grab all the mob keywords

  mobHNode = roomPtr->mobHead;

  while (mobHNode)
  {
    if (mobHNode->mobPtr)
    {
      editableListNode *listNode = 
        createEditableListNode(mobHNode->mobPtr->keywordListHead, mobHNode, ENTITY_MOB);

      if (!listNode)
      {
        displayAllocError("editableListNode", "createEditableList");

        return listHead;
      }

      addEditabletoList(&listHead, listNode);
    }

    mobHNode = mobHNode->Next;
  }


  return listHead;
}


//
// checkEditableList : Scans through the editable list passed to the function for the keyword 
//                     specified by strn and strnPos.  whatMatched is set to the entity type
//                     that matched and *matchingNode is set to the address of the matching 
//                     node.
//
//                     Returns current position - if skipped through 3 due to dot notation, 
//                     returns 4
//
//          *strn : string as entered by user
//      *editNode : head of editableListNode list
//   *whatMatched : set based on what type of entity matched
// **matchingNode : upon a match, the matching node's address is put in this
//  currentEntity : the number of entities already scanned through - entityToGrab 
//                  takes this into account
//

char checkEditableList(const char *strn, editableListNode *editNode, char *whatMatched, 
                       editableListNode **matchingNode, char currentEntity)
{
  char workstrn[1024];
  char *workptr = workstrn;
  const char *keyptr = workstrn;
  uint entityToGrab = 1;


  *whatMatched = NO_MATCH;
  *matchingNode = NULL;

  if (!strn[0] || !editNode)
    return 1;

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

 // scan through the edit nodes

  while (editNode)
  {
   // check if user-specified word matches keyword list for this node

    if (scanKeyword(keyptr, editNode->keywordListHead))
    {
     // entityToGrab will be > 1 for dot notation

      if (entityToGrab == currentEntity)
      {
        *whatMatched = editNode->entityType;
        *matchingNode = editNode;

        return currentEntity;
      }

     // we don't want this one, increment currentEntity

      else 
      {
        currentEntity++;
      }
    }

    editNode = editNode->Next;
  }

  return currentEntity;
}


//
// copyEditableList : Copies an editable list, returning the address of the head of the new list
//
//    *srcNode : list to copy from
//

editableListNode *copyEditableList(const editableListNode *srcNode)
{
  editableListNode *headNode = NULL;


  if (!srcNode) 
    return NULL;

  while (srcNode)
  {
    editableListNode *newNode = createEditableListNode(srcNode);
    if (!newNode)
    {
      displayAllocError("editableListNode", "copyEditableList");

      return headNode;
    }

    addEditabletoList(&headNode, newNode);

    srcNode = srcNode->Next;
  }


  return headNode;
}


//
// deleteEditable : deletes an editable node
//

void deleteEditable(editableListNode *edit)
{
  delete edit;
}


//
// deleteEditableList : Deletes an editable list
//
//   *node : head of list to delete
//

void deleteEditableList(editableListNode *node)
{
  while (node)
  {
    editableListNode *nextNode = node->Next;

    deleteEditable(node);

    node = nextNode;
  }
}


//
// deleteEditableinList : Deletes a specific editable in a list that starts
//                        at *editHead, updating whatever needs to be updated
//                        for the list to be valid.
//

void deleteEditableinList(editableListNode **editHead, editableListNode *edit)
{
  if (edit == (*editHead))
  {
    *editHead = (*editHead)->Next;

    deleteEditable(edit);
  }
  else
  {
    editableListNode *prevEdit = *editHead;

    while (prevEdit->Next != edit)
      prevEdit = prevEdit->Next;

    prevEdit->Next = edit->Next;

    deleteEditable(edit);
  }
}
