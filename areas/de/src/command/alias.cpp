//
//  File: alias.cpp      originally part of durisEdit
//
//  Usage: functions for implementing and interpreting alias system
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

#include "../fh.h"
#include "../types.h"

#include "alias.h"



extern bool g_madeChanges;
extern variable *g_varHead;


//
// aliasExists : returns true if alias exists, false if not, case insensitive
//

bool aliasExists(const alias *aliasHead, const char *aliasName)
{
  while (aliasHead)
  {
    if (strcmpnocase(aliasHead->aliasStrn, aliasName))
      return true;
    
    aliasHead = aliasHead->Next;
  }

  return false;
}


//
// getAlias - return alias that matches if any
//

alias *getAlias(alias *aliasHead, const char *aliasName)
{
  while (aliasHead)
  {
    if (strcmpnocase(aliasHead->aliasStrn, aliasName))
      return aliasHead;
    
    aliasHead = aliasHead->Next;
  }

  return NULL;
}


//
// unaliasCmd : remove an alias from the list pointed to by aliasHead
//              based on argument list in args
//
//        args : arguments to command
//   aliasHead : pointer to head of list that hopefully contains alias
//

void unaliasCmd(const char *args, alias **aliasHead)
{
  alias *node = *aliasHead, *old = NULL;

  if (numbArgs(args) != 1)
  {
    _outtext("\nThe 'unalias' command takes one argument - the alias to delete.\n\n");
    return;
  }

  if (!node)
  {
    _outtext("\nNo aliases are defined.\n\n");
    return;
  }

  while (node)
  {
    if (strcmpnocase(node->aliasStrn, args))
    {
      if (node == *aliasHead) 
        *aliasHead = (*aliasHead)->Next;

      if (old) 
        old->Next = node->Next;

      displayColorString("\nDeleted alias '&+c");
      _outtext(node->aliasStrn);
      displayColorString("&n'\n\n");

      delete node;

      g_madeChanges = true;

      return;
    }

    old = node;
    node = node->Next;
  }

  _outtext("\nNo matching alias found.\n\n");
}


//
// addAlias : returns value based on what happened when attempting to
//            add the alias named in aliasStrn to the list pointed to by
//            *aliasHead
//
//    aliasHead : pointer to pointer to head of linked list of aliases
//    aliasStrn : name of alias being added
//         strn : contents of alias being added
//

char addAlias(alias **aliasHead, const char *aliasStrn, const char *strn)
{
  alias *node = *aliasHead, *adding, *old = NULL;


  if (strcmpnocase(aliasStrn, strn))
  {
    return ALIAS_ITSELF;
  }

 // check for dupes

  while (node)
  {
    if (strcmpnocase(node->aliasStrn, aliasStrn))
    {
      if (!strlen(strn))  // display to what alias is set
      {
        displayColorString("\n'&+c");
        _outtext(node->aliasStrn);
        displayColorString("&n' aliases '&+Y");
        _outtext(node->commandStrn);
        displayColorString("&n'\n\n");

        return ALIAS_DISPLAY;
      }
      else
      {
        strncpy(node->commandStrn, strn, MAX_ALIAS_COMMAND_LEN - 1);
        node->commandStrn[MAX_ALIAS_COMMAND_LEN - 1] = '\0';

        return ALIAS_REPLACED;
      }
    }

    old = node;
    node = node->Next;
  }

  if (!strlen(strn))  // no match for alias name, ignore it
    return ALIAS_BLANK;

  adding = new(std::nothrow) alias;
  if (!adding)
  {
    displayAllocError("alias", "addAlias");
    return ALIAS_ERROR;
  }

  memset(adding, 0, sizeof(alias));

 // check string length - if it's too long, truncate it

  strncpy(adding->aliasStrn, aliasStrn, MAX_ALIAS_LEN - 1);
  strncpy(adding->commandStrn, strn, MAX_ALIAS_COMMAND_LEN - 1);

 // if the string is over the length, strncpy won't add a trailing null

  adding->aliasStrn[MAX_ALIAS_LEN - 1] = '\0';
  adding->commandStrn[MAX_ALIAS_COMMAND_LEN - 1] = '\0';

  node = *aliasHead;

  if (!*aliasHead) 
  {
    *aliasHead = adding;
  }
  else
  {
    while (node->Next)
      node = node->Next;

    node->Next = adding;
  }

  return ALIAS_ADDED;
}


//
// addAliasArgs : adds an alias using addAlias() based on user input
//                contained in args
//
//           args : user input
//         addLFs : if TRUE, adds linefeeds around output
//  updateChanges : if TRUE, updates g_madeChanges var if a change is made
//      aliasHead : pointer to pointer to head of alias list
//        display : if false, doesn't echo to screen
//

void addAliasArgs(const char *origargs, const bool addLFs, const bool updateChanges, alias **aliasHead,
                  const bool display)
{
  char args[1024], result;
  char *aliasptr, *aliasargptr;
  const char *outptr = NULL;  // output.


  strncpy(args, origargs, 1023);
  args[1023] = '\0';
  
  if (!strlen(args))
  {
    if (addLFs) 
      _outtext("\n");

    _outtext("The format of the alias command is 'alias <alias name> <aliased string>'.\n");
    return;
  }

 // get first arg (name of alias)

  aliasptr = args;

 // skip past spaces

  while (*aliasptr && (*aliasptr == ' '))
    aliasptr++;

 // skip past alias name, set \0

  aliasargptr = aliasptr;

  while (*aliasargptr && (*aliasargptr != ' '))
    aliasargptr++;

  if (*aliasargptr)
  {
    *aliasargptr = '\0';
    aliasargptr++;
  }

 // find start of args

  while (*aliasargptr && (*aliasargptr == ' '))
    aliasargptr++;

 // process command

  result = addAlias(aliasHead, aliasptr, aliasargptr);

 // may not have been able to add entire args, so get pointer to possible new alias

  const alias *aliasAdded = getAlias(*aliasHead, aliasptr);

  switch (result)
  {
    case ALIAS_ADDED    : if (display)
                          {
                            if (addLFs) 
                              _outtext("\n");

                            displayColorString("'&+c");
                            _outtext(aliasAdded->aliasStrn);
                            displayColorString("&n' now aliases '&+Y");
                            _outtext(aliasAdded->commandStrn);
                            displayColorString("&n'\n");

                            if (addLFs) 
                              _outtext("\n");
                          }

                          if (updateChanges) 
                            g_madeChanges = true;

                          break;

    case ALIAS_REPLACED : if (display)
                          {
                            if (addLFs) 
                              _outtext("\n");

                            displayColorString("Replaced alias '&+c");
                            _outtext(aliasAdded->aliasStrn);
                            displayColorString("&n' with '&+Y");
                            _outtext(aliasAdded->commandStrn);
                            displayColorString("&n'\n");

                            if (addLFs) 
                              _outtext("\n");
                          }

                          if (updateChanges) 
                            g_madeChanges = true;

                          break;

    case ALIAS_ITSELF   : outptr = "An alias can't alias itself.\n";
                          break;

    case ALIAS_BLANK    : outptr = "The 'alias' command requires at least two arguments.\n"; 
                          break;

    case ALIAS_DISPLAY  :  // perhaps this function should handle this, but
                           // it's easier to let addAlias do it.  so there
    case ALIAS_ERROR    : break;
  }

  if (display && outptr && (result != ALIAS_DISPLAY))
  {
    if (addLFs) 
      _outtext("\n");

    displayColorString(outptr);

    if (addLFs) 
      _outtext("\n");
  }
}


//
// aliasCmd : the first function called when user types "alias ..." -
//            displays list of aliases if there are no args, calls
//            addAliasArgs otherwise
//
//      args : user input
//    addLFs : if TRUE, adds linefeeds around output
// aliasHead : pointer to pointer to head of list
//

void aliasCmd(const char *args, const bool addLFs, alias **aliasHead)
{
  const alias *node = *aliasHead;
  size_t numbLines = 1;


  if (!strlen(args))
  {
    if (!node) 
      _outtext("\nThere are currently no aliases defined.\n");
    else 
      _outtext("\n\n");

    while (node)
    {
     // below is done because i want to show any color codes in the alias
     // without interpreting them - numbLines may be slightly off if there
     // are any codes but it's good enough

      char outstrn[MAX_ALIAS_LEN + MAX_ALIAS_COMMAND_LEN + 128];

      sprintf(outstrn, "  &+c%s&n aliases '&+Y%s&n'\n",
              node->aliasStrn, node->commandStrn);

      displayColorString("  &+c");
      _outtext(node->aliasStrn);
      displayColorString("&n aliases '&+Y");
      _outtext(node->commandStrn);
      displayColorString("&n'\n");

      numbLines += numbLinesStringNeeds(outstrn);

      if (checkPause(NULL, numbLines))
        return;
      
      node = node->Next;
    }

    _outtext("\n");

    return;
  }
  else 
  {
    addAliasArgs(args, addLFs, true, aliasHead, true);
  }
}


//
// expandAliasArgs : takes an alias string and expands it, replacing argument
//                   tokens with whatever should be there
//
//                   returns amount added to endStrn
//
//     endStrn : expanded string is put here
// commandStrn : alias command string
//        args : args to alias
//   intMaxLen : max length in endStrn, not including ending null
//

size_t expandAliasArgs(char *endStrn, const char *commandStrn, const char *args, const size_t intMaxLen)
{
  char *origendStrn = endStrn;
  size_t lenadded = 0;


  while (*commandStrn != '\0')
  {
    if ((*commandStrn == '%') && *(commandStrn + 1))
    {
      const char ch = *(commandStrn + 1) - '0';

      commandStrn += 2;

      if ((ch < 0) || (ch > MAX_ALIAS_ARGS))
      {
        *endStrn = '%';
        endStrn++;
        lenadded++;

        if (lenadded <= intMaxLen)
        {
          *endStrn = ch + '0';
          endStrn++;
          lenadded++;
        }
      }
      else

     // if %0, stick the entire arg list in there

      if (ch == 0) 
      {
        strncpy(endStrn, args, intMaxLen - lenadded);
        origendStrn[intMaxLen] = '\0';

        const size_t arglen = strlen(args);

        lenadded += arglen;
        endStrn += arglen;
      }

     // ch is 1-MAX_ALIAS_ARGS

      else 
      {
        size_t argadded = 0;

        getArg(args, ch, endStrn, intMaxLen - lenadded, &argadded);

        lenadded += argadded;
        endStrn += argadded;
      }
    }
    else
    {
      *endStrn = *commandStrn;

      endStrn++;
      commandStrn++;

      lenadded++;
    }

    if (lenadded > intMaxLen)
    {
      origendStrn[intMaxLen] = '\0';
      return intMaxLen;
    }
  }

  *endStrn = '\0';

  return lenadded;
}
