//
//  File: crtroom.cpp    originally part of durisEdit
//
//  Usage: functions for creating a room structure
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

#include "../types.h"
#include "../fh.h"



extern room *g_defaultRoom;
extern zone g_zoneRec;
extern uint g_numbLookupEntries, g_numbRooms, g_highestRoomNumber, g_lowestRoomNumber;
extern room **g_roomLookup;

//
// createRoom : Function to create a new room - returns pointer to the new
//              room
//

room *createRoom(const bool incLoaded, const uint roomNumb, const bool getFreeNumb)
{
  room *newRoom;
  uint numb;


 // create a new room

  newRoom = new(std::nothrow) room;
  if (!newRoom)
  {
    displayAllocError("room", "createRoom");

    return NULL;
  }

  if (g_defaultRoom)
  {
    newRoom = copyRoomInfo(g_defaultRoom, true);

    newRoom->defaultRoom = false;
  }
  else
  {
    memset(newRoom, 0, sizeof(room));

    strcpy(newRoom->roomName, "Unnamed");
  }

  newRoom->zoneNumber = g_zoneRec.zoneNumber;

  if (!getFreeNumb)
    newRoom->roomNumber = numb = roomNumb;
  else
    newRoom->roomNumber = numb = getFirstFreeRoomNumber();

  if (incLoaded)
  {
   // check if we have a dupe

    if (roomExists(numb))
    {
      deleteRoomInfo(newRoom, false, false);
      return NULL;
    }

   // if not enough lookup entries, increase the tables

    if (numb >= g_numbLookupEntries)
    {
      if (!changeMaxVnumAutoEcho(numb + 1000))
      {
        deleteRoomInfo(newRoom, false, false);
        return NULL;
      }
    }

   // copyRoomInfo call above already incs numb rooms when using a default room

    if (!g_defaultRoom)
      g_numbRooms++;

    g_roomLookup[numb] = newRoom;

    if (numb > getHighestRoomNumber()) 
      g_highestRoomNumber = numb;

    if (numb < getLowestRoomNumber())  
      g_lowestRoomNumber = numb;

    createPrompt();
  }

  return newRoom;
}
