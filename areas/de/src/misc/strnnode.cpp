//
//  File: strnnode.cpp   originally part of durisEdit
//
//  Usage: functions for manipulating stringNode structures/lists
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

#include <stdlib.h>
#include <errno.h>
#ifdef __UNIX__
#  include <unistd.h>
#else
#  include <direct.h>
#endif

#include "../fh.h"
#include "../system.h"
#include "strnnode.h"    // header for string node functions




//
// getNumbStringNodes : returns number of nodes in list
//

uint getNumbStringNodes(const stringNode *strnHead)
{
  uint numb = 0;

  while (strnHead)
  {
    numb++;
    strnHead = strnHead->Next;
  }

  return numb;
}


//
// fixStringNodes : Removes trailing spaces and deletes blank nodes at end
//                  of list
//

void fixStringNodes(stringNode **strnHead, const bool remStartLines)
{
  stringNode *strnNode = *strnHead, *lastNode = NULL;


 // get to end of list and delete trailing spaces in one fell swoop

  while (strnNode)
  {
    remTrailingSpaces(strnNode->string);  // if spaces are removed, end memory won't be immediately recovered, but I don't mind
    strnNode->len = strlen(strnNode->string);

    lastNode = strnNode;
    strnNode = strnNode->Next;
  }

  strnNode = lastNode;

 // check for blank lines, from the end to the start

 // first, get rid of blank lines at end of desc

  while (strnNode && *strnHead)
  {
    if (!strnNode->len)
    {
      if (strnNode->Last) 
        strnNode->Last->Next = NULL;

      lastNode = strnNode->Last;

      if (strnNode == *strnHead) 
        *strnHead = NULL;

      delete strnNode;

      strnNode = lastNode;
    }
    else break;
  }

 // then, get rid of blank lines at start of desc

  if (remStartLines)
  {
    while (*strnHead)
    {
      if (!((*strnHead)->len))
      {
        strnNode = *strnHead;

        *strnHead = (*strnHead)->Next;
        (*strnHead)->Last = NULL;

        delete strnNode;
      }
      else break;
    }
  }
}


//
// readStringNodes : Reads strings from inFile, creating a list of stringNodes
//                   until an end-point determined by EOSN is hit.  The head
//                   of the list is returned to the calling function.
//
//  *inFile : file to read strings from
//     EOSN : "end of string nodes" - determines when to stop - 
//            values defined in strnnode.h
//

stringNode *readStringNodes(FILE *inFile, const char EOSN, const bool remStartLines)
{
  stringNode *root = NULL, *node, *oldnode = NULL;
  char strn[MAX_STRNNODE_LEN + 16];  // safety!
  bool nullFG = false;


  while (true)
  {
    if (fgets(strn, MAX_STRNNODE_LEN, inFile) == NULL)
      nullFG = true;

    if (nullFG && (EOSN != ENDOFFILE))
    {
      displayAnyKeyPromptNoClr("\n\n"
"Warning: Hit EOF while reading file in readStringNodes().\n"
"         Bad things may begin happening.  You have been warned.\n\n"
"         Press a key...\n");

      fixStringNodes(&root, remStartLines);

      return root;
    }

    nolf(strn);

    if ((EOSN == TILDE_LINE) && !strcmp(strn, "~"))
    {
      fixStringNodes(&root, remStartLines);

      return root;
    }

    if (nullFG == false)
    {
      node = new(std::nothrow) stringNode;
      if (!node)
      {
        displayAllocError("stringNode", "readStringNodes");

       // not exiting will leave zone in a bad way

        exit(1);
      }

      node->setString(strn);

      if (!node->string)
      {
        displayAllocError("stringNode", "readStringNodes");

        exit(1);
      }

      if (!root) 
        root = node;

      node->Last = oldnode;

      if (oldnode) 
        oldnode->Next = node;

      oldnode = node;
    }
    else 

   // hit end of file and 'end of string nodes' var is ENDOFFILE

    if (EOSN == ENDOFFILE)
    {
      fixStringNodes(&root, remStartLines);

      return root;
    }
  }
}


//
// writeStringNodes : Writes strings to outFile from a list of stringNodes.
//
//    *outFile : file to write strings to
//   *strnNode : head of string node list to write
//

void writeStringNodes(FILE *outFile, const stringNode *strnNode)
{
  while (strnNode)
  {
    fprintf(outFile, "%s\n", strnNode->string);

    strnNode = strnNode->Next;
  }
}


//
// deleteStringNodes : Deletes a list of string nodes
//
//   *strnNode : head of list to delete
//

void deleteStringNodes(stringNode *strnNode)
{
  while (strnNode)
  {
    stringNode *nextNode = strnNode->Next;

    delete strnNode;

    strnNode = nextNode;
  }
}


//
// deleteMatchingStringNodes : remove all string nodes that match a particular string ignoring case
//

void deleteMatchingStringNodes(stringNode **strnHead, const char *strn)
{
  stringNode *strnNode = *strnHead;

  while (strnNode)
  {
    stringNode *next = strnNode->Next;

    if (strcmpnocase(strnNode->string, strn))
    {
      if (!strnNode->Last) // head of list
      {
        *strnHead = strnNode->Next;
        (*strnHead)->Last = NULL;
      }
      else
      {
        strnNode->Last->Next = strnNode->Next;

        if (strnNode->Next)
          strnNode->Next->Last = strnNode->Last;
      }

      delete strnNode;
    }

    strnNode = next;
  }
}


//
// copyStringNodes : Reads strings from the list pointed to by strnNode,
//                   "copying" them into a new list.  The pointer to the new
//                   list is returned.
//
//  *strnNode : head string node to copy
//

stringNode *copyStringNodes(const stringNode *strnNode)
{
  stringNode *oldNode = NULL, *headNode = NULL;


  while (strnNode)
  {
    stringNode *node = new(std::nothrow) stringNode;
    if (!node)
    {
      displayAllocError("stringNode", "copyStringNodes");

     // user will likely lose something here, but I'd rather return than die

      return headNode;
    }

    if (!headNode) 
      headNode = node;

    node->setString(strnNode->string);

    if (!node->string)
    {
      displayAllocError("stringNode", "copyStringNodes");

      delete node;

      return headNode;
    }

    node->Last = oldNode;

    if (oldNode) 
      oldNode->Next = node;

    oldNode = node;

    strnNode = strnNode->Next;
  }

  return headNode;
}


//
// compareStringNodes : Compares two lists of string nodes - returns TRUE if
//                      they match exactly
//
//   *list1 : first list to compare against
//   *list2 : second list to compare against
//

bool compareStringNodes(const stringNode *list1, const stringNode *list2)
{
  if (list1 == list2) 
    return true;

  if (!list1 || !list2) 
    return false;

  while (list1 && list2)
  {
    if (strcmp(list1->string, list2->string)) 
      return false;

    list1 = list1->Next;
    list2 = list2->Next;
  }

  if ((!list1 && list2) || (list1 && !list2)) 
    return false;

  return true;
}


//
// compareStringNodesIgnoreCase : Compares two lists of string nodes - returns TRUE if
//                                they match ignoring case
//
//   *list1 : first list to compare against
//   *list2 : second list to compare against
//

bool compareStringNodesIgnoreCase(const stringNode *list1, const stringNode *list2)
{
  if (list1 == list2) 
    return true;

  if (!list1 || !list2) 
    return false;

  while (list1 && list2)
  {
    if (!strcmpnocase(list1->string, list2->string)) 
      return false;

    list1 = list1->Next;
    list2 = list2->Next;
  }

  if ((!list1 && list2) || (list1 && !list2)) 
    return false;

  return true;
}


//
// editStringNodes : Saves a list of string nodes to a temporary file, calls a
//                   user-defined editor to edit em, then rereads the file.
//                   The function returns a pointer to the new list.
//
//   *strnNode : head of list to edit
//

stringNode *editStringNodes(stringNode *strnNode, const bool remStartLines)
{
  char path[1024], fullpath[2048], ultrapath[4096], *retVal;
  FILE *tmpFile;


  retVal = getcwd(path, 1024);

 // open the tmp file

 // root directories have a trailing slash/backslash, others don't..

  if ((path[strlen(path) - 1] == '\\') || (path[strlen(path) - 1] == '/'))
  {
    sprintf(fullpath, "%s%s", path, TMPFILE_NAME);
  }
  else
  {
#ifdef __UNIX__
    sprintf(fullpath, "%s/%s", path, TMPFILE_NAME);
#else
    sprintf(fullpath, "%s\\%s", path, TMPFILE_NAME);
#endif
  }

  if ((tmpFile = fopen(fullpath, "wt")) == NULL)
  {
    sprintf(ultrapath, "\nError: Couldn't open '%s' for writing.  Aborting.\n\n", fullpath);

    _outtext(ultrapath);

    return strnNode;
  }

 // write the current desc and close the file

  writeStringNodes(tmpFile, strnNode);

  fclose(tmpFile);

 // execute the editor

  sprintf(ultrapath, "%s \"%s\"", getEditorName(), fullpath);

 // system() returns -1 on error

  if (system(ultrapath))
  {
    clrscr(7, 0);

    _outtext("Couldn't load command shell to execute editor:\n\n");

    switch (errno)
    {
      case 0       : break;  // no error
      case E2BIG   : _outtext("Arg list too big (E2BIG)\n\n"); break;
      case EACCES  : _outtext("Access violation (EACCES)\n\n"); break;
      case EMFILE  : _outtext("No file handles available (EMFILE)\n\n"); break;
      case ENOENT  : _outtext("No such file or directory (ENOENT)\n\n"); break;
      case ENOMEM  : _outtext("Not enough memory to exec (ENOMEM)\n\n"); break;

      default : sprintf(path, "err #%d\n\n", errno);
                _outtext(path);
    }

    return strnNode;
  }

  clrscr(7, 0);

 // read the new description

  if ((tmpFile = fopen(fullpath, "rt")) == NULL)
  {
    sprintf(ultrapath, "\nError: Couldn't open '%s' for reading.  Aborting.\n\n", fullpath);

    _outtext(ultrapath);

    getkey();

    return strnNode;
  }

  stringNode *strnNodeNew = readStringNodes(tmpFile, ENDOFFILE, remStartLines);

 // close the file and delete it

  fclose(tmpFile);

  deleteFile(fullpath);

 // remove trailing spaces and blank lines

  fixStringNodes(&strnNodeNew, remStartLines);

 // compare the new and the old

 // the two lists match - delete the new and return the pointer to the old list

  if (compareStringNodes(strnNode, strnNodeNew))
  {
    deleteStringNodes(strnNodeNew);

    return strnNode;
  }
  else  // two lists don't match - delete the old and return the new
  {
    deleteStringNodes(strnNode);

    return strnNodeNew;
  }
}
