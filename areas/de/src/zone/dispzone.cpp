//
//  File: dispzone.cpp   originally part of durisEdit
//
//  Usage: functions for displaying zone info
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

#include "fh.h"
#include "types.h"

#include "dispzone.h"

#include "misc/loaded.h"

extern zone g_zoneRec;
extern uint g_numbRooms, g_numbExits, g_numbObjTypes, g_numbObjs, g_numbMobTypes, g_numbMobs;
extern flagDef g_zoneMiscFlagDef[];


//
// addZoneExittoList
//

void addZoneExittoList(zoneExit **head, const roomExit *ex, const char dir, const room *room)
{
  zoneExit *node, *prev;

  node = new(std::nothrow) zoneExit;
  if (!node)
  {
    displayAllocError("zoneExit", "addZoneExittoList");

    return;
  }

  memset(node, 0, sizeof(zoneExit));

  if (!*head) 
  {
    *head = node;
  }
  else
  {
    prev = *head;

    while (prev->Next) 
      prev = prev->Next;

    prev->Next = node;
  }

  node->roomIn = room;
  node->outRoom = ex->destRoom;
  node->exitDir = dir;
}


//
// displayZoneExits - display exits that lead to rooms not in zone
//

void displayZoneExits(const char *args)
{
  const uint lowNumb = getLowestRoomNumber(), topNumb = getHighestRoomNumber();
  size_t lines = 1;
  zoneExit *zoneExitHead = NULL, *node;
  const room *room;
  char strn[1024], j;
  bool ignoreWithinRange = false;


  if (strcmpnocase(args, "I"))
    ignoreWithinRange = true;

 // create zone exit list..

  for (uint currNumb = lowNumb; currNumb <= topNumb; currNumb++)
  {
    room = findRoom(currNumb);
    if (!room) 
      continue;

    for (j = 0; j < NUMB_EXITS; j++)
    {
      if (roomExitOutsideZone(room->exits[j]) &&
          (!ignoreWithinRange ||
           ((room->exits[j]->destRoom < (int)lowNumb) ||
            (room->exits[j]->destRoom > (int)topNumb))))
        addZoneExittoList(&zoneExitHead, room->exits[j], j, room);
    }
  }


 // run through list, displaying as we go..

  node = zoneExitHead;
  if (!node)
  {
    _outtext("\nNo out-of-zone exit destinations found.\n\n");
    return;
  }

  _outtext("\n\n");

  while (node)
  {
    sprintf(strn, "  &+croom #&+g%u &+L(&n%s&+L) &+R%s&n to &+Croom &+G#%d&n\n",
            node->roomIn->roomNumber, node->roomIn->roomName,
            getExitStrn(node->exitDir), node->outRoom);

    if (checkPause(strn, lines))
      break;

    node = node->Next;
  }

 // delete the list..

  while (zoneExitHead)
  {
    node = zoneExitHead->Next;

    delete zoneExitHead;

    zoneExitHead = node;
  }

  _outtext("\n");
}


//
// displayZoneInfo
//

void displayZoneInfo(void)
{
  char strn[4096], fstrn[1024];
  usint ch;


  sprintf(strn,
    "\n"
    "&+YZone number:&n %u  &+YZone name:&n %s&n\n"
    "\n"
    "&+YTop room number:&n %u        &+YZone lifespan  :&n %u-%u\n"
    "&+YZone reset mode:&n %u (%s)\n"
    "&+YDifficulty: &n%u\n"
    "&+YFlags:&n %u (%s)\n"
    "\n"
    "&+YNumber of rooms/exits     :&n %u/%u\n"
    "&+YNumber of obj types/loaded:&n %u/%u\n"
    "&+YNumber of mob types/loaded:&n %u/%u\n"
    "\n",

    g_zoneRec.zoneNumber, g_zoneRec.zoneName,
    getHighestRoomNumber(), g_zoneRec.lifeLow, g_zoneRec.lifeHigh,
    g_zoneRec.resetMode, getZoneResetStrn(g_zoneRec.resetMode),
    g_zoneRec.zoneDiff,
    g_zoneRec.miscBits.longIntFlags, getFlagStrn(g_zoneRec.miscBits.longIntFlags, g_zoneMiscFlagDef, fstrn, 1023),
    g_numbRooms, g_numbExits,
    g_numbObjTypes, g_numbObjs,
    g_numbMobTypes, g_numbMobs );


  displayColorString(strn);

  displayColorString("\n&+C"
"Press a key to display the out-of-zone exits (I for 'ignore mode'), Q to quit&n");

  ch = toupper(getkey());

  if (ch == 'Q') 
  {
    _outtext("\n\n");

    return;
  }

  if (ch == 'I')
    displayZoneExits("I");
  else
    displayZoneExits("");
}
