//
//  File: keywords.cpp   originally part of durisEdit
//
//  Usage: functions used to handle keyword lists
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


//
// createKeywordList : Taking keywordStrn as input, breaks the individual
//                     words in the string (separated by spaces) into
//                     "keywords."  Each keyword goes into its own stringNode,
//                     and a linked list is created.  The head of this linked
//                     list is returned to the calling function.
//
//                     If list ends in a tilde, the tilde is removed.
//
//   *keywordStrn : Pointer to the keyword string as read from the data file
//

stringNode *createKeywordList(const char *keywordStrn)
{
  size_t i, strnpos = 0, len = strlen(keywordStrn);
  stringNode *homeSNode, *SNode, *oldSNode = NULL;
  char strn[MAX_STRNNODE_LEN] = "";


  if ((keywordStrn[0] == '~') || (keywordStrn[0] == '\0'))
    return NULL;

  homeSNode = SNode = new(std::nothrow) stringNode;
  if (!homeSNode)
  {
    displayAllocError("stringNode", "createKeywordList");

    return NULL;
  }

  for (i = 0; i <= len; i++)
  {
    if ((keywordStrn[i] == ' ') || (keywordStrn[i] == '~') || (keywordStrn[i] == '\0'))
    {
      if (strnpos > 0)
      {
       // null-terminate the string

        strn[strnpos] = '\0';
        strnpos = 0;

        SNode->setString(strn);
        if (!SNode->string)
        {
          delete SNode;

          displayAllocError("stringNode", "createKeywordList");

          if (SNode == homeSNode)
            homeSNode = NULL;

          return homeSNode;
        }

        if (oldSNode) 
          oldSNode->Next = SNode;

        oldSNode = SNode;

       // reset SNode to NULL so that a new one is only created if it is needed

        SNode = NULL;
      }
    }
    else
    {
     // if this is a new keyword, create a node

      if (!SNode)
      {
        SNode = new(std::nothrow) stringNode;

        if (!SNode)
        {
          displayAllocError("stringNode", "createKeywordList");

          return homeSNode;  // might have gotten part-way
        }
      }

      strn[strnpos] = keywordStrn[i];

      strnpos++;
    }
  }

  if (strn[0] == '\0')
  {
    delete homeSNode;

    return NULL;
  }
  else 
  {
    return homeSNode;
  }
}


//
// addKeywordtoList : returns FALSE if there's an error, TRUE if success
//

bool addKeywordtoList(stringNode **keywordHead, const char *keyword)
{
  stringNode *strnNode, *strnNodePrev;


  strnNode = new(std::nothrow) stringNode;

  if (!strnNode) 
    return false;

  strnNode->setString(keyword);
  if (!strnNode->string)
  {
    displayAllocError("stringNode", "addKeywordtoList");

    delete strnNode;

    return false;
  }

  if (!*keywordHead) 
  {
    *keywordHead = strnNode;
  }
  else
  {
    strnNodePrev = *keywordHead;

    while (strnNodePrev->Next)
      strnNodePrev = strnNodePrev->Next;

    strnNodePrev->Next = strnNode;
    strnNode->Last = strnNodePrev;
  }

  return true;
}


//
// getReadableKeywordStrn :
//                  Creates a user-readable string of keywords from the list
//                  pointed to by keywordHead, with each keyword separated by
//                  a comma.
//
//  *keywordHead : pointer to the head of the stringNode keyword list
//      *endStrn : pointer to string that contains user-readable string
//     intMaxLen : max length of endStrn
//

char *getReadableKeywordStrn(const stringNode *keywordHead, char *endStrn, const size_t intMaxLen)
{
  size_t len = 0;


  endStrn[0] = '\0';

  while (keywordHead)
  {
    strncpy(endStrn + len, keywordHead->string, intMaxLen - len);
    endStrn[intMaxLen] = '\0';

    len += strlen(keywordHead->string);

    if (len > intMaxLen)
    {
      endStrn[intMaxLen] = '\0';

      return endStrn;
    }

    keywordHead = keywordHead->Next;

    if (keywordHead) 
    {
      strncpy(endStrn + len, ", ", intMaxLen - len);
      len += 2;

      if (len > intMaxLen)
      {
        endStrn[intMaxLen] = '\0';

        return endStrn;
      }
    }
  }

  return endStrn;
}


//
// scanKeyword : Searches through the list pointed to by keywordListHead for
//               a substring constructed using userinput and keywordpos.  If
//               a match is found, TRUE is returned, else FALSE is returned.
//
//          *userinput : Points to the string where the substring for which to search
//                       resides
//    *keywordListHead : Pointer to the head of a list of stringNodes
//

bool scanKeyword(const char *userinput, const stringNode *keywordListHead)
{
  char *keyword;


  if (!keywordListHead) 
    return false;

 // skip leading spaces

  while (*userinput == ' ')
    userinput++;

  keyword = new(std::nothrow) char[strlen(userinput) + 1];
  if (!keyword)
  {
    displayAllocError("char[]", "scanKeyword");

    return false;
  }

  strcpy(keyword, userinput);

  remTrailingSpaces(keyword);

  while (keywordListHead)
  {
    if (strcmpnocase(keyword, keywordListHead->string))
    {
      delete[] keyword;

      return true;
    }

    keywordListHead = keywordListHead->Next;
  }

  delete[] keyword;

  return false;
}


//
// createKeywordString : Creates a standard Duris tilde-terminated keyword
//                       string from a list of stringNodes, altering strn
//                       and returning the address of the new string.
//
//   *strnNode : head of linked list of stringNodes
//    *keyStrn : string into which to put new string
//   intMaxLen : max length of strnNode
//

char *createKeywordString(const stringNode *strnNode, char *keyStrn, const size_t intMaxLen)
{
  size_t len = 0;


  keyStrn[0] = '\0';

  while (strnNode)
  {
    strncpy(keyStrn + len, strnNode->string, intMaxLen - len);
    keyStrn[intMaxLen] = '\0';

    len += strlen(strnNode->string);

    if (len > intMaxLen)
    {
      keyStrn[intMaxLen - 1] = '~';
      keyStrn[intMaxLen] = '\0';

      break;
    }

    strnNode = strnNode->Next;

    if (strnNode && (len < intMaxLen)) 
    {
      strcat(keyStrn, " ");
      len++;
    }
  }

  if (len < intMaxLen)
  {
    strcat(keyStrn, "~");
  }
  else
  {
    keyStrn[intMaxLen - 1] = '~';
    keyStrn[intMaxLen] = '\0';
  }

  lowstrn(keyStrn);


  return keyStrn;
}
