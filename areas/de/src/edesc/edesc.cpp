//
//  File: edesc.cpp      originally part of durisEdit
//
//  Usage: functions for manipulating extra desc records
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

#include "fh.h"
#include "types.h"

#include "edesc.h"



//
// getNumbExtraDescs : Returns the number of extra descs in a list
//
//   *extraDescNode : head of list
//

uint getNumbExtraDescs(const extraDesc *extraDescNode)
{
  uint numb = 0;


  while (extraDescNode)
  {
    numb++;

    extraDescNode = extraDescNode->Next;
  }

  return numb;
}


//
// copyExtraDescs : Copies a list of extra descs, returning the address of the
//                  head of the new list
//
//   *srcExtraDesc : head node of list to copy
//

extraDesc *copyExtraDescs(const extraDesc *srcExtraDesc)
{
  extraDesc *newExtraDesc, *prevExtraDesc = NULL, *headExtraDesc = NULL;


  while (srcExtraDesc)
  {
    newExtraDesc = new(std::nothrow) extraDesc;
    if (!newExtraDesc)
    {
      displayAllocError("extraDesc", "copyExtraDescs");

      return headExtraDesc;
    }

    memset(newExtraDesc, 0, sizeof(extraDesc));

    newExtraDesc->Last = prevExtraDesc;

    if (!headExtraDesc) 
      headExtraDesc = newExtraDesc;

    if (prevExtraDesc) 
      prevExtraDesc->Next = newExtraDesc;

    prevExtraDesc = newExtraDesc;

    newExtraDesc->keywordListHead = copyStringNodes(srcExtraDesc->keywordListHead);
    newExtraDesc->extraDescStrnHead = copyStringNodes(srcExtraDesc->extraDescStrnHead);


    srcExtraDesc = srcExtraDesc->Next;
  }

  return headExtraDesc;
}


//
// deleteExtraDescs : Deletes a list of extraDescs
//
//   *srcExtraDesc : head of list to delete
//

void deleteExtraDescs(extraDesc *srcExtraDesc)
{
  extraDesc *nextExtraDesc;


  while (srcExtraDesc)
  {
    nextExtraDesc = srcExtraDesc->Next;

    deleteStringNodes(srcExtraDesc->keywordListHead);
    deleteStringNodes(srcExtraDesc->extraDescStrnHead);

    delete srcExtraDesc;

    srcExtraDesc = nextExtraDesc;
  }
}


//
// copyOneExtraDesc : Copies just the specified extraDesc, returning the
//                    address of the copy
//
//   *srcExtraDesc : source to copy from
//

extraDesc *copyOneExtraDesc(const extraDesc *srcExtraDesc)
{
  extraDesc *newExtraDesc;


  if (!srcExtraDesc) 
    return NULL;

  newExtraDesc = new(std::nothrow) extraDesc;
  if (!newExtraDesc)
  {
    displayAllocError("extraDesc", "copyOneExtraDesc");

    return NULL;
  }

  memcpy(newExtraDesc, srcExtraDesc, sizeof(extraDesc));

  newExtraDesc->keywordListHead = copyStringNodes(srcExtraDesc->keywordListHead);
  newExtraDesc->extraDescStrnHead = copyStringNodes(srcExtraDesc->extraDescStrnHead);

  return newExtraDesc;
}


//
// addExtraDesctoList : yep
//

void addExtraDesctoList(extraDesc **descHead, extraDesc *desc)
{
  desc->Next = NULL;

  if (!(*descHead))  // no list exists
  {
    *descHead = desc;
    desc->Last = NULL;
  }
  else
  {
    extraDesc *extraDescNode = *descHead;

    while (extraDescNode->Next)
      extraDescNode = extraDescNode->Next;

    desc->Last = extraDescNode;
    extraDescNode->Next = desc;
  }
}


//
// removeExtraDescfromList : remove extra desc from list, don't delete anything
//
//                           assumes extra desc is in list
//

void removeExtraDescfromList(extraDesc **descHead, extraDesc *desc)
{
  extraDesc *prevDesc;


  if (desc == (*descHead))
  {
    *descHead = (*descHead)->Next;
  }
  else
  {
    prevDesc = *descHead;

    while (prevDesc->Next != desc)
      prevDesc = prevDesc->Next;

    prevDesc->Next = desc->Next;
  }

  desc->Next = NULL;
}


//
// deleteOneExtraDesc : Deletes just the specified extraDesc
//
//   *extraDescNode : extraDesc to delete
//

void deleteOneExtraDesc(extraDesc *extraDescNode)
{
  if (!extraDescNode) 
    return;

  deleteStringNodes(extraDescNode->keywordListHead);
  deleteStringNodes(extraDescNode->extraDescStrnHead);

  delete extraDescNode;
}


//
// compareExtraDescs : Compares two lists of extra descs - returns TRUE if
//                     they match
//
//  *list1 : the first list
//  *list2 : #2
//

bool compareExtraDescs(const extraDesc *list1, const extraDesc *list2)
{
  if (list1 == list2) 
    return true;

  if (!list1 || !list2) 
    return false;

  while (list1 && list2)
  {
    if (!compareStringNodesIgnoreCase(list1->keywordListHead, list2->keywordListHead)) 
      return false;

    if (!compareStringNodes(list1->extraDescStrnHead, list2->extraDescStrnHead)) 
      return false;

    list1 = list1->Next;
    list2 = list2->Next;
  }

  if ((!list1 && list2) || (list1 && !list2)) 
    return false;

  return true;
}


//
// compareOneExtraDesc : Compares two extra desc nodes - returns TRUE if
//                       they match
//
//  *node1 : the first node
//  *node2 : #2
//

bool compareOneExtraDesc(const extraDesc *node1, const extraDesc *node2)
{
  if (node1 == node2) 
    return true;

  if (!node1 || !node2) 
    return false;

  if (!compareStringNodesIgnoreCase(node1->keywordListHead, node2->keywordListHead)) 
    return false;

  if (!compareStringNodes(node1->extraDescStrnHead, node2->extraDescStrnHead)) 
    return false;


  return true;
}


//
// getEdescinList : given keyword, return first matching edesc
//

extraDesc *getEdescinList(extraDesc *descHead, const char *keyword)
{
  extraDesc *descNode = descHead;

  if (!descHead) 
    return NULL;

  while (descNode)
  {
    if (scanKeyword(keyword, descNode->keywordListHead)) 
      return descNode;

    descNode = descNode->Next;
  }

  return NULL;
}


//
// getEdescinList : const version of above
//

const extraDesc *getEdescinList(const extraDesc *descHead, const char *keyword)
{
  const extraDesc *descNode = descHead;

  if (!descHead) 
    return NULL;

  while (descNode)
  {
    if (scanKeyword(keyword, descNode->keywordListHead)) 
      return descNode;

    descNode = descNode->Next;
  }

  return NULL;
}


//
// displayExtraDescNodes : display keywords of all extra descs in list - used by stat room and stat obj
//

void displayExtraDescNodes(const extraDesc *extraDescNode)
{
  char extraStrn[1024];

  while (extraDescNode)
  {
    getReadableKeywordStrn(extraDescNode->keywordListHead, extraStrn, 1023);

    _outtext(extraStrn);

    extraDescNode = extraDescNode->Next;

    if (extraDescNode)
      _outtext(" / ");
  }
}
