//
//  File: clone.cpp      originally part of durisEdit
//
//  Usage: functions related to cloning stuff
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
#include <ctype.h>

#include "../types.h"
#include "../fh.h"
#include "../keys.h"

extern bool g_madeChanges;
extern room **g_roomLookup;
extern objectType **g_objLookup;
extern mobType **g_mobLookup;
extern command g_cloneCommands[];
extern uint g_highestRoomNumber, g_highestObjNumber, g_highestMobNumber, g_numbLookupEntries;

//
// cloneEntity
//

void cloneEntity(const char *args)
{
  checkCommands(args, g_cloneCommands,
                "\nSpecify one of <room|obj|mob> as the first argument.\n\n",
                cloneExecCommand, NULL, NULL);
}


//
// cloneRoom : clone a specific room a specific number of times
//
//      vnum : vnum of room to clone
//   toclone : number of times to clone room
//

void cloneRoom(const uint vnum, const uint toclone)
{
  const uint startclone = getFirstFreeRoomNumber();
  const room *roomPtr;
  bool copyExits;


  if (!roomExists(vnum))
  {
    _outtext("\nRoom vnum specified not found.\n\n");
    return;
  }

  if (toclone == 0)
  {
    _outtext("\nSpecify an amount to clone greater than 0.\n\n");
    return;
  }

  roomPtr = findRoom(vnum);

  if (!roomPtr)
  {
    _outtext("\nRoom vnum specified in second arg not found.\n\n");
    return;
  }

  if (displayYesNoPrompt("\n&+cCopy room exits along with rooms", promptNo, false) == promptNo)
    copyExits = false;
  else
    copyExits = true;

  for (uint cloning = 0; cloning < toclone; cloning++)
  {
    if (getFirstFreeRoomNumber() >= g_numbLookupEntries)
    {
      if (!changeMaxVnumAutoEcho(g_numbLookupEntries + toclone + 1000))
      {
        if (cloning) 
          g_madeChanges = true;

        return;
      }
    }

    room *newRoom = copyRoomInfo(roomPtr, true);

    if (!copyExits)
    {
      for (uint exitNumb = 0; exitNumb < NUMB_EXITS; exitNumb++)
      {
        if (newRoom->exits[exitNumb])
        {
          deleteRoomExit(newRoom->exits[exitNumb], true);
          newRoom->exits[exitNumb] = NULL;
        }
      }
    }

    newRoom->roomNumber = getFirstFreeRoomNumber();

    if (newRoom->roomNumber > g_highestRoomNumber)
      g_highestRoomNumber = newRoom->roomNumber;

    g_roomLookup[newRoom->roomNumber] = newRoom;
  }

  g_madeChanges = true;

  char outstrn[256];

  if (toclone != 1)
  {
    sprintf(outstrn, "Room #%u cloned %u times (starting at %u).\n\n",
            vnum, toclone, startclone);
  }
  else 
  {
    sprintf(outstrn, "Room #%u cloned once into %u.\n\n",
            vnum, startclone);
  }

  _outtext(outstrn);
}


//
// cloneObjectType : clone a specific object a specific number of times
//
//      vnum : vnum of object to clone
//   toclone : number of times to clone object
//

void cloneObjectType(const uint vnum, const uint toclone)
{
  const uint startclone = getFirstFreeObjNumber();
  const objectType *obj;


  if (!objExists(vnum))
  {
    _outtext("\nObject vnum specified not found.\n\n");
    return;
  }

  if (toclone == 0)
  {
    _outtext("\nSpecify an amount to clone greater than 0.\n\n");
    return;
  }

  obj = findObj(vnum);

  if (!obj)
  {
    _outtext("\nObject vnum specified in second arg not found.\n\n");
    return;
  }

  for (uint cloning = 0; cloning < toclone; cloning++)
  {
    if (getFirstFreeObjNumber() >= g_numbLookupEntries)
    {
      if (!changeMaxVnumAutoEcho(g_numbLookupEntries + toclone + 1000))
      {
        if (cloning) 
          g_madeChanges = true;

        return;
      }
    }

    objectType *objClone = copyObjectType(obj, true);

    objClone->objNumber = getFirstFreeObjNumber();

    if (objClone->objNumber > g_highestObjNumber)
      g_highestObjNumber = objClone->objNumber;

    g_objLookup[objClone->objNumber] = objClone;
  }

  g_madeChanges = true;

  char outstrn[MAX_OBJSNAME_LEN + 128];

  if (toclone != 1)
  {
    sprintf(outstrn, "\nObject type #%u '%s&n' cloned %u times (starting at %u).\n\n",
            vnum, getObjShortName(obj), toclone, startclone);
  }
  else 
  {
    sprintf(outstrn, "\nObject type #%u '%s&n' cloned once into %u.\n\n",
            vnum, getObjShortName(obj), startclone);
  }

  displayColorString(outstrn);
}


//
// cloneMobType : clone a specific mob a specific number of times
//
//      vnum : vnum of mob to clone
//   toclone : number of times to clone mob
//

void cloneMobType(const uint vnum, const uint toclone)
{
  const uint startclone = getFirstFreeMobNumber();
  const mobType *mob;


  if (!mobExists(vnum))
  {
    _outtext("\nMob vnum specified not found.\n\n");
    return;
  }

  if (toclone == 0)
  {
    _outtext("\nSpecify an amount to clone greater than 0.\n\n");
    return;
  }

  mob = findMob(vnum);

  if (!mob)
  {
    _outtext("\nMob vnum specified in second arg not found.\n\n");
    return;
  }

  for (uint cloning = 0; cloning < toclone; cloning++)
  {
    if (getFirstFreeMobNumber() >= g_numbLookupEntries)
    {
      if (!changeMaxVnumAutoEcho(g_numbLookupEntries + toclone + 1000))
      {
        if (cloning) 
          g_madeChanges = true;

        return;
      }
    }

    mobType *mobClone = copyMobType(mob, true);

    mobClone->mobNumber = getFirstFreeMobNumber();

    if (mobClone->mobNumber > g_highestMobNumber)
      g_highestMobNumber = mobClone->mobNumber;

    g_mobLookup[mobClone->mobNumber] = mobClone;
  }

  g_madeChanges = true;

  char outstrn[MAX_MOBSNAME_LEN + 128];

  if (toclone != 1)
  {
    sprintf(outstrn, "\nMob type #%u '%s&n' cloned %u times (starting at %u).\n\n",
            vnum, getMobShortName(mob), toclone, startclone);
  }
  else 
  {
    sprintf(outstrn, "\nMob type #%u '%s&n' cloned once into %u.\n\n",
            vnum, getMobShortName(mob), startclone);
  }

  displayColorString(outstrn);
}
