//
//  File: loaded.cpp     originally part of durisEdit
//
//  Usage: functions used to manipulate 'numb loaded' list
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


#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "fh.h"
#include "types.h"
#include "loaded.h"
#include "obj/objhere.h"
#include "mob/mobhere.h"
#include "misc/master.h"
#include "command/command.h"



extern entityLoaded *g_numbLoadedHead;
extern objectType *objHead;
extern command g_limitCommands[];
extern uint g_numbObjs;
extern bool g_madeChanges;


//
// addEntity : Add a specific entity to the list - that is, increments the
//             number loaded for a specific entity.
//
//   entityType : type of entity that is being added
//   entityNumb : vnum of entity that is being added
//

void addEntity(const char entityType, const uint entityNumb)
{
  entityLoaded *numbLoadedNode = g_numbLoadedHead, *oldNode = NULL;


 // first, check to see if node referencing this entity type and numb already
 // exists

  while (numbLoadedNode)
  {
    if (((numbLoadedNode->entityType) == entityType) && ((numbLoadedNode->entityNumb) == entityNumb))
    {
      numbLoadedNode->numberLoaded++;

      if ((numbLoadedNode->numberLoaded > numbLoadedNode->overrideLoaded) && numbLoadedNode->overrideLoaded)
      {
        numbLoadedNode->overrideLoaded = numbLoadedNode->numberLoaded;
      }

      return;
    }

    oldNode = numbLoadedNode;
    numbLoadedNode = numbLoadedNode->Next;
  }

 // no matching node found, create a new one

  numbLoadedNode = new(std::nothrow) entityLoaded;

  if (!numbLoadedNode)
  {
    displayAllocError("entityLoaded", "addEntity");

   // if this happens the zone is pretty well boned, so just die

    exit(1);
  }

  if (!g_numbLoadedHead) 
    g_numbLoadedHead = numbLoadedNode;

  memset(numbLoadedNode, 0, sizeof(entityLoaded));

  numbLoadedNode->entityType = entityType;
  numbLoadedNode->entityNumb = entityNumb;
  numbLoadedNode->numberLoaded = 1;

 // add to end of list

  if (oldNode) 
    oldNode->Next = numbLoadedNode;
}


//
// setEntityOverride
//

void setEntityOverride(const char entityType, const uint entityNumb, const uint override)
{
  entityLoaded *numbLoadedNode = g_numbLoadedHead, *oldNode = NULL;


 // first, check to see if node referencing this entity type and numb already
 // exists

  while (numbLoadedNode)
  {
    if (((numbLoadedNode->entityType) == entityType) && ((numbLoadedNode->entityNumb) == entityNumb))
    {
      numbLoadedNode->overrideLoaded = override;

      return;
    }

    oldNode = numbLoadedNode;
    numbLoadedNode = numbLoadedNode->Next;
  }

 // no matching node found, create a new one

  numbLoadedNode = new(std::nothrow) entityLoaded;

  if (!numbLoadedNode)
  {
    displayAllocError("entityLoaded", "setEntityOverride");

   // if this happens the zone will be written all sorts of wrong, so just die

    exit(1);
  }

  if (!g_numbLoadedHead) 
    g_numbLoadedHead = numbLoadedNode;

  memset(numbLoadedNode, 0, sizeof(entityLoaded));

  numbLoadedNode->entityType = entityType;
  numbLoadedNode->entityNumb = entityNumb;
  numbLoadedNode->overrideLoaded = override;

 // add it to the list

  if (oldNode) 
    oldNode->Next = numbLoadedNode;
}


//
// getFirstEntityOverride
//

const entityLoaded *getFirstEntityOverride(const entityLoaded *startNode)
{
  while (startNode)
  {
    if (startNode->overrideLoaded > 0) 
      return startNode;

    startNode = startNode->Next;
  }

  return NULL;
}


//
// deleteEntityinLoaded : Deletes a specific entity from the list
//
//   entityType : type of entity that is being added
//   entityNumb : number of entity that is being added
//

void deleteEntityinLoaded(const uint entityType, const uint entityNumb)
{
  entityLoaded *numbLoadedNode = g_numbLoadedHead, *oldNode = NULL;


  while (numbLoadedNode)
  {
    if ((numbLoadedNode->entityType == entityType) && (numbLoadedNode->entityNumb == entityNumb))
      break;

    oldNode = numbLoadedNode;
    numbLoadedNode = numbLoadedNode->Next;
  }

  if (!numbLoadedNode) 
    return;  // couldn't find a match

  if (numbLoadedNode == g_numbLoadedHead) 
    g_numbLoadedHead = g_numbLoadedHead->Next;
  else 
    oldNode->Next = numbLoadedNode->Next;

  delete numbLoadedNode;
}


//
// decEntity : Decrements number loaded for a specific entity
//
//   entityType : type of entity that is being decremented
//   entityNumb : number of entity that is being decremented
//

void decEntity(const uint entityType, const uint entityNumb)
{
  entityLoaded *numbLoadedNode = g_numbLoadedHead;


  while (numbLoadedNode)
  {
    if (((numbLoadedNode->entityType) == entityType) && ((numbLoadedNode->entityNumb) == entityNumb))
    {
      numbLoadedNode->numberLoaded--;
      if (numbLoadedNode->numberLoaded == 0)
      {
        deleteEntityinLoaded(entityType, entityNumb);
      }

      return;
    }

    numbLoadedNode = numbLoadedNode->Next;
  }

 // oops, no match (bug)

  char strn[256];

  sprintf(strn, "\nWarning: decEntity() - attempted to decrement non-existent entry (type %u, #%u)\n\n",
          entityType, entityNumb);

  _outtext(strn);
}


//
// getNumbEntities : Gets the number of a specific entities loaded
//
//       entityType : type of entity
//       entityNumb : id numb of entity
//  includeOverride : if true, returns override value if greater than zero
//

uint getNumbEntities(const uint entityType, const uint entityNumb, const bool includeOverride)
{
  entityLoaded *numbLoadedNode = g_numbLoadedHead;


  while (numbLoadedNode)
  {
    if (((numbLoadedNode->entityType) == entityType) && ((numbLoadedNode->entityNumb) == entityNumb))
    {
      if (includeOverride && (numbLoadedNode->overrideLoaded > 0))
        return numbLoadedNode->overrideLoaded;
      else 
        return numbLoadedNode->numberLoaded;
    }

    numbLoadedNode = numbLoadedNode->Next;
  }

  return 0;
}


//
// getEntityOverride : Returns the value of overrideLoaded for a particular entity
//
//       entityType : type of entity
//       entityNumb : id numb of entity
//

uint getEntityOverride(const uint entityType, const uint entityNumb)
{
  entityLoaded *numbLoadedNode = g_numbLoadedHead;


  while (numbLoadedNode)
  {
    if (((numbLoadedNode->entityType) == entityType) && ((numbLoadedNode->entityNumb) == entityNumb))
    {
      return numbLoadedNode->overrideLoaded;
    }

    numbLoadedNode = numbLoadedNode->Next;
  }

  return 0;
}


//
// getNumbEntitiesNode : Gets the node of a specific entity loaded
//
//   entityType : type of entity
//   entityNumb : id numb of entity
//

const entityLoaded *getNumbEntitiesNode(const uint entityType, const uint entityNumb)
{
  entityLoaded *numbLoadedNode = g_numbLoadedHead;


  while (numbLoadedNode)
  {
    if (((numbLoadedNode->entityType) == entityType) && ((numbLoadedNode->entityNumb) == entityNumb))
    {
      return numbLoadedNode;
    }

    numbLoadedNode = numbLoadedNode->Next;
  }

  return NULL;
}


//
// resetNumbLoaded : Looks for an entityLoaded record in the numbLoadedNode
//                   list that matches the type and oldNumb, and changes the
//                   entityNumb to newNumb
//

void resetNumbLoaded(const uint entityType, const uint oldNumb, const uint newNumb)
{
  entityLoaded *numbLoadedNode = g_numbLoadedHead;

  while (numbLoadedNode)
  {
    if ((numbLoadedNode->entityType == entityType) && (numbLoadedNode->entityNumb == oldNumb))
    {
      numbLoadedNode->entityNumb = newNumb;
      return;
    }

    numbLoadedNode = numbLoadedNode->Next;
  }
}


//
// displayLoadedList
//

void displayLoadedList(const char *args)
{
  entityLoaded *node = g_numbLoadedHead;
  size_t lines = 1;
  uint numb;
  char strn[256];
  bool vnum, gotMatch = false;  // for matching by vnum


  if (!node)
  {
    _outtext("\nThere are currently no entries in the numbLoaded list.\n\n");

    return;
  }

  vnum = strnumer(args);

  if (vnum) 
    numb = strtoul(args, NULL, 10);

  _outtext("\n\n");

  while (node)
  {
    if (!vnum || (node->entityNumb == numb))
    {
      if (node->overrideLoaded)
      {
        sprintf(strn, "type: %d (%s)   vnum: %d   numb loaded: %u   override: %u\n",
                node->entityType, getEntityTypeStrn(node->entityType), node->entityNumb, node->numberLoaded,
                node->overrideLoaded);
      }
      else
      {
        sprintf(strn, "type: %d (%s)   vnum: %d   numb loaded: %u\n",
                node->entityType, getEntityTypeStrn(node->entityType), node->entityNumb, node->numberLoaded);
      }

      if (checkPause(strn, lines))
        return;

      gotMatch = true;
    }

    node = node->Next;
  }

  if (vnum && !gotMatch)
    _outtext("No loaded node found for that vnum.\n");

  _outtext("\n");
}


//
// setLimitArgs : sets override limit
//

void setLimitArgs(const char *args)
{
  checkCommands(args, g_limitCommands,
                "\nSpecify one of <object|mob> as the second argument.\n\n",
                setLimitExecCommand, NULL, NULL);
}


//
// setLimitArgsStartup : sets override limit
//

void setLimitArgsStartup(const char *args)
{
  checkCommands(args, g_limitCommands,
                "\nSpecify one of <object|mob> as the second argument.\n\n",
                setLimitExecCommandStartup, NULL, NULL);
}


//
// setLimitOverrideObj : args passed should be <vnum> <limit>
//

void setLimitOverrideObj(const char *args, const bool updateChanges, const bool display)
{
  char arg1[256], arg2[256];
  uint vnum, limit;


  getArg(args, 1, arg1, 255);
  getArg(args, 2, arg2, 255);

  if (!strnumer(arg1) || !strnumer(arg2))
  {
    _outtext("\nThe second and third args should be the vnum and the limit.\n\n");
    return;
  }

  vnum = strtoul(arg1, NULL, 10);
  limit = strtoul(arg2, NULL, 10);

  setEntityOverride(ENTITY_OBJECT, vnum, limit);

  if (display)
  {
    sprintf(arg1, "\n"
"Objects of type #%u (if any) will now be written to the zone file with\n"
"a limit of %u.\n\n",
            vnum, limit);

    displayColorString(arg1);
  }

  if (updateChanges) 
    g_madeChanges = true;
}


//
// setLimitOverrideMob : args passed should be <vnum> <limit>
//

void setLimitOverrideMob(const char *args, const bool updateChanges, const bool display)
{
  char arg1[256], arg2[256];
  uint vnum, limit;


  getArg(args, 1, arg1, 255);
  getArg(args, 2, arg2, 255);

  if (!strnumer(arg1) || !strnumer(arg2))
  {
    _outtext("\nThe second and third args should be the vnum and the limit.\n\n");
    return;
  }

  vnum = strtoul(arg1, NULL, 10);
  limit = strtoul(arg2, NULL, 10);

  setEntityOverride(ENTITY_MOB, vnum, limit);

  if (display)
  {
    sprintf(arg1, "\n"
"Mobs of type #%u (if any) will now be written to the zone file with a\n"
"limit of %u.\n\n",
            vnum, limit);

    displayColorString(arg1);
  }

  if (updateChanges) 
    g_madeChanges = true;
}
