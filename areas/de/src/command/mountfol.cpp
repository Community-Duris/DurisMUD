//
//  File: mountfol.cpp   originally part of durisEdit
//
//  Usage: functions for making mobs follow/ride other mobs
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
#include <stdlib.h>
#include <ctype.h>

#include "../types.h"
#include "../fh.h"
#include "../room/room.h"
#include "../mob/mobhere.h"
#include "../misc/master.h"

extern bool g_madeChanges;
extern room *g_currentRoom;

//
// mountMob
//

void mountMob(const char *args)
{
  char arg1[256], arg2[256], strn[1024], whatMatched;
  editableListNode *matchingNode;
  mobHere *mounting = NULL, *mount = NULL, *mob;
  uint vnum;


  getArg(args, 1, arg1, 255);
  getArg(args, 2, arg2, 255);

  if ((arg1[0] == '\0') || (arg2[0] == '\0'))
  {
    _outtext(
"\nThe 'mount' command requires two arguments - the mounting mob and the mount.\n\n");
    return;
  }

 // check arg1 (mounting) to see if it's a vnum or a keywordish string

  if (!strnumber(arg1))
  {
    checkEditableList(arg1, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

    switch (whatMatched)
    {
      case NO_MATCH : 
        _outtext("\nNo match found for keyword in first arg (mounting mob).\n\n");
        return;

      case ENTITY_MOB :
        mounting = ((mobHere *)(matchingNode->entityPtr));

        break;

      default :
        char outstrnerr[256];

        sprintf(outstrnerr, "\n%ss can't mount anything.\n\n",
                getEntityTypeStrn(whatMatched));

        outstrnerr[1] = toupper(outstrnerr[1]);

        _outtext(outstrnerr);

        return;
    }
  }
  else
  {
    vnum = strtoul(arg1, NULL, 10);

    mob = g_currentRoom->mobHead;

    while (mob)
    {
      if (mob->mobNumb == vnum) 
        mounting = mob;

      mob = mob->Next;
    }

    if (!mounting)
    {
      _outtext("\nNo mobs found that match vnum in first arg.\n\n");
      return;
    }
  }


 // check arg2 (mount) to see if it's a vnum or a keywordish string

  if (!strnumber(arg2))
  {
    checkEditableList(arg2, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

    switch (whatMatched)
    {
      case NO_MATCH : 
        _outtext("\nNo match found for keyword in second arg (mob being mounted).\n\n");
        return;

      case ENTITY_MOB :
        mount = ((mobHere *)(matchingNode->entityPtr));

        break;

      default :
        char outstrnerr[256];

        sprintf(outstrnerr, "\n%ss can't be mounted.\n\n",
                getEntityTypeStrn(whatMatched));

        outstrnerr[1] = toupper(outstrnerr[1]);

        _outtext(outstrnerr);

        return;
    }
  }
  else
  {
    vnum = strtoul(arg2, NULL, 10);

    mob = g_currentRoom->mobHead;

    while (mob)
    {
      if (mob->mobNumb == vnum) 
        mount = mob;

      mob = mob->Next;
    }

    if (!mount)
    {
      _outtext("\nNo mobs found that match vnum in second arg.\n\n");
      return;
    }
  }

  if (mount == mounting)
  {
    _outtext("\nA mob can't ride itself..\n\n");
    return;
  }

  if (mount->riding == mounting)
  {
    _outtext("\nWe can't have mobs riding each other now, can we?\n\n");
    return;
  }

  if (mounting->riding == mount)
  {
    _outtext("\nThat mount is already ridden by that mob.\n\n");
    return;
  }

  if (mounting->riding)
  {
    _outtext("\nThe mounting mob already is riding another.\n\n");
    return;
  }

  if (mount->riddenBy)
  {
    _outtext("\nThat mount is already being ridden by another mob.\n\n");
    return;
  }

  if (mounting->following == mount)
  {
    _outtext("\nA rider cannot be following his mount.\n\n");
    return;
  }

  if (mount->following == mounting)
  {
    _outtext("\nA mount cannot be following his rider.\n\n");
    return;
  }

  mount->riddenBy = mounting;
  mounting->riding = mount;

  sprintf(strn, "\n'%s&n' now rides '%s&n'.\n\n", 
          getMobShortName(mounting->mobPtr), getMobShortName(mount->mobPtr));

  displayColorString(strn);

  g_madeChanges = true;
}


//
// unmountMob
//

void unmountMob(const char *args)
{
  char arg1[256], strn[1024], whatMatched;
  editableListNode *matchingNode;
  mobHere *mounting = NULL, *mob;
  uint vnum;


  getArg(args, 1, arg1, 255);

  if (arg1[0] == '\0')
  {
    _outtext(
"\nThe 'unmount' command requires one argument - the mob that is mounting a mob.\n\n");
    return;
  }

 // check arg1 (mounting) to see if it's a vnum or a keywordish string

  if (!strnumber(arg1))
  {
    checkEditableList(arg1, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

    switch (whatMatched)
    {
      case NO_MATCH : 
        _outtext("\nNo match found for keyword in first arg (mob that is mounting).\n\n");
        return;

      case ENTITY_MOB :
        mounting = ((mobHere *)(matchingNode->entityPtr));

        break;

      default :
        char outstrnerr[256];

        sprintf(outstrnerr, "\n%ss can't mount anything.\n\n",
                getEntityTypeStrn(whatMatched));

        outstrnerr[1] = toupper(outstrnerr[1]);

        _outtext(outstrnerr);

        return;
    }
  }
  else
  {
    vnum = strtoul(arg1, NULL, 10);

    mob = g_currentRoom->mobHead;

    while (mob)
    {
      if (mob->mobNumb == vnum)
      {
        mounting = mob;
        break;
      }

      mob = mob->Next;
    }

    if (!mounting)
    {
      _outtext("\nNo mobs found that match vnum in first arg (mounting mob).\n\n");
      return;
    }
  }

  if (!mounting->riding)
  {
    _outtext("\nThat mob isn't riding anything.\n\n");
    return;
  }

 // display first, since mounting->riding gets reset

  sprintf(strn, "\n'%s&n' no longer rides '%s&n'.\n\n", 
          getMobShortName(mounting->mobPtr), getMobShortName(mounting->riding->mobPtr));
  displayColorString(strn);

  mounting->riding->riddenBy = NULL;
  mounting->riding = NULL;

  g_madeChanges = true;
}


//
// followMob
//

void followMob(const char *args)
{
  char arg1[256], arg2[256], strn[1024], whatMatched;
  editableListNode *matchingNode;
  mobHere *following = NULL, *leader = NULL, *mob;
  uint vnum;


  getArg(args, 1, arg1, 255);
  getArg(args, 2, arg2, 255);

  if ((arg1[0] == '\0') || (arg2[0] == '\0'))
  {
    _outtext(
"\nThe 'follow' command requires two arguments - the following mob and the leader.\n\n");
    return;
  }

 // check arg1 (following) to see if it's a vnum or a keywordish string

  if (!strnumber(arg1))
  {
    checkEditableList(arg1, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

    switch (whatMatched)
    {
      case NO_MATCH : 
        _outtext("\nNo match found for keyword in first arg (mob that is following).\n\n");
        return;

      case ENTITY_MOB :
        following = ((mobHere *)(matchingNode->entityPtr));

        break;

      default :
        char outstrnerr[256];

        sprintf(outstrnerr, "\n%ss can't follow anything.\n\n",
                getEntityTypeStrn(whatMatched));

        outstrnerr[1] = toupper(outstrnerr[1]);

        _outtext(outstrnerr);

        return;
    }
  }
  else
  {
    vnum = strtoul(arg1, NULL, 10);

    mob = g_currentRoom->mobHead;

    while (mob)
    {
      if (mob->mobNumb == vnum) 
        following = mob;

      mob = mob->Next;
    }

    if (!following)
    {
      _outtext("\nNo mobs found that match vnum in first arg (mob that is following).\n\n");
      return;
    }
  }


 // check arg2 (leader) to see if it's a vnum or a keywordish string

  if (!strnumber(arg2))
  {
    checkEditableList(arg2, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

    switch (whatMatched)
    {
      case NO_MATCH : 
        _outtext("\nNo match found for keyword in second arg (mob that is leading).\n\n");
        return;

      case ENTITY_MOB :
        leader = ((mobHere *)(matchingNode->entityPtr));

        break;

      default :
        char outstrnerr[256];

        sprintf(outstrnerr, "\n%ss can't be followed.\n\n",
                getEntityTypeStrn(whatMatched));

        outstrnerr[1] = toupper(outstrnerr[1]);

        _outtext(outstrnerr);

        return;
    }
  }
  else
  {
    vnum = strtoul(arg2, NULL, 10);

    mob = g_currentRoom->mobHead;

    while (mob)
    {
      if (mob->mobNumb == vnum) 
        leader = mob;

      mob = mob->Next;
    }

    if (!leader)
    {
      _outtext("\nNo mobs found that match vnum in second arg (mob that is leading).\n\n");
      return;
    }
  }

  if (leader == following)
  {
    _outtext("\nA mob can't follow itself..\n\n");
    return;
  }

  if (leader->following == following)
  {
    _outtext("\nWe can't have mobs following each other now, can we?\n\n");
    return;
  }

  if (following->following == leader)
  {
    _outtext("\nThat mob is already following that leader.\n\n");
    return;
  }

  if (following->following)
  {
    _outtext("\nThat mob is already following another.\n\n");
    return;
  }

  if (following->riding)
  {
    _outtext("\nA mob riding another mob cannot follow anyone.\n\n");
    return;
  }

  if (following->riddenBy)
  {
    _outtext("\nA mob that is being ridden cannot follow anyone.\n\n");
    return;
  }

  if (leader == following->riddenBy)
  {
    _outtext("\nA ridden mob cannot follow the mob that is riding it.\n\n");
    return;
  }

  following->following = leader;

  sprintf(strn, "\n'%s&n' now follows '%s&n'.\n\n", 
          getMobShortName(following->mobPtr), getMobShortName(leader->mobPtr));
  displayColorString(strn);

  g_madeChanges = true;
}


//
// unfollowMob
//

void unfollowMob(const char *args)
{
  char arg1[256], strn[1024], whatMatched;
  editableListNode *matchingNode;
  mobHere *following = NULL, *mob;
  uint vnum;


  getArg(args, 1, arg1, 255);

  if (arg1[0] == '\0')
  {
    _outtext(
"\nThe 'unfollow' command requires one argument - the mob that is following a mob.\n\n");
    return;
  }

 // check arg1 (following) to see if it's a vnum or a keywordish string

  if (!strnumber(arg1))
  {
    checkEditableList(arg1, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

    switch (whatMatched)
    {
      case NO_MATCH : _outtext("\nNo match found for keyword in first arg (mob following another).\n\n");
                      return;

      case ENTITY_MOB :
        following = ((mobHere *)(matchingNode->entityPtr));

        break;

      default :
        char outstrnerr[256];

        sprintf(outstrnerr, "\n%ss can't follow anything.\n\n",
                getEntityTypeStrn(whatMatched));

        outstrnerr[1] = toupper(outstrnerr[1]);

        _outtext(outstrnerr);

        return;
    }
  }
  else
  {
    vnum = strtoul(arg1, NULL, 10);

    mob = g_currentRoom->mobHead;

    while (mob)
    {
      if (mob->mobNumb == vnum) 
        following = mob;

      mob = mob->Next;
    }

    if (!following)
    {
      _outtext("\nNo mobs found that match vnum in first arg (mob that is following).\n\n");
      return;
    }
  }

  if (!following->following)
  {
    _outtext("\nThat mob isn't following anything.\n\n");
    return;
  }

  sprintf(strn, "\n'%s&n' no longer follows '%s&n'.\n\n", 
          getMobShortName(following->mobPtr), getMobShortName(following->following->mobPtr));
  displayColorString(strn);

  following->following = NULL;

  g_madeChanges = true;
}
