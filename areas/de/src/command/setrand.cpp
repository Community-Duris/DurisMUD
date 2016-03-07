//
//  File: setrand.cpp    originally part of durisEdit
//
//  Usage: functions for handling 'setrand' command
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

#include "fh.h"

#include "misc/master.h"
#include "mob/mobhere.h"
#include "obj/objhere.h"

extern bool g_madeChanges;
extern room *g_currentRoom;


//
// setEntityRandomVal
//

void setEntityRandomVal(const char *args)
{
  char whatMatched, arg1[512], arg2[512], outstrn[MAX_OBJSNAME_LEN + MAX_MOBSNAME_LEN + 64];
  masterKeywordListNode *matchingNode;
  objectHere *obj = NULL;
  mobHere *mob = NULL;
  int chance;


  getArg(args, 1, arg1, 511);
  getArg(args, 2, arg2, 511);

  chance = atoi(arg2);

  if (!strnumer(arg2) || (chance <= 0) || (chance > 100))
  {
    _outtext("\nInput for second argument is invalid (should be 1-100).\n\n");
    return;
  }

 // if first arg specifies a vnum, search through room's obj and mobhere list

  if (strnumer(arg1))
  {
    uint vnum;

    vnum = strtoul(arg1, NULL, 10);

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

  if (!obj && !mob)
  {
    checkCurrentMasterKeywordList(arg1, &whatMatched, &matchingNode);

    switch (whatMatched)
    {
      case NO_MATCH :
        _outtext("\nNothing in this room matches that vnum/keyword.\n\n");
        return;

      case ENTITY_OBJECT :
        obj = ((objectHere *)(matchingNode->entityPtr));

        break;

      case ENTITY_MOB :
        mob = ((mobHere *)(matchingNode->entityPtr));

        break;

      default :
        _outtext("\nInvalid target.\n\n");
        return;
    }
  }

  if (obj)
  {
    if (obj->randomChance != chance)
      g_madeChanges = true;

    obj->randomChance = chance;

    sprintf(outstrn, "\nRandom chance for '%s&n' to load set to %u%%.\n\n",
            getObjShortName(obj->objectPtr), chance);
  }
  else
  {
    if (mob->randomChance != chance)
      g_madeChanges = true;

    mob->randomChance = chance;

    sprintf(outstrn, "\nRandom chance for '%s&n' to load set to %u%%.\n\n",
           getMobShortName(mob->mobPtr), chance);
  }

  displayColorString(outstrn);
}
