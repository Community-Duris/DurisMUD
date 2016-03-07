//
//  File: room.cpp       originally part of durisEdit
//
//  Usage: a plethora of functions relating to rooms
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
#include <string.h>

#include <ctype.h>

#include "types.h"
#include "fh.h"
#include "room.h"        // header for room stuff



extern room **g_roomLookup, *g_currentRoom;
extern uint g_numbLookupEntries, g_numbRooms, g_lowestRoomNumber, g_highestRoomNumber;
extern bool g_madeChanges;
extern char g_revdirs[];
extern char *g_exitnames[];

//
// findRoom : Returns the address of the room node that has the requested
//            roomNumber (if any)
//
//   roomNumber : room number to return
//

room *findRoom(const uint roomNumber)
{
  if (roomNumber >= g_numbLookupEntries) 
    return NULL;

  return g_roomLookup[roomNumber];
}


//
// roomExists : Returns true iff room exists with requested roomNumber
//
//   roomNumber : room number to check
//

bool roomExists(const uint roomNumber)
{
  if (roomNumber >= g_numbLookupEntries) 
    return false;

  return (g_roomLookup[roomNumber] != NULL);
}


//
// goDirection : "Goes" in a direction by changing g_currentRoom to the room
//               pointed to by a specific exit record.
//
//  direction : direction user is going
//

void goDirection(const uint direction)
{
  const roomExit *exitRec = g_currentRoom->exits[direction];


 // exit exists

  if (exitRec)
  {
    if (exitRec->destRoom == -1)
    {
      _outtext("\nYou can't go that direction (exit dest. is -1 [nowhere]).\n\n");

      return;
    }

    if (roomExists(exitRec->destRoom))
    {
      g_currentRoom = findRoom(exitRec->destRoom);

      displayCurrentRoom();
    }
    else
    {
      _outtext("\nYou can't go that direction (room pointed to by exit doesn't exist).\n\n");
    }
  }
  else  // no exit in this direction
  {
    if (getWalkCreateVal())
    {
      room *roomPtr = createRoom(true, 0, true);
      if (!roomPtr)
        return;

      roomExit *fromExit, *toExit;

      createExit(&fromExit, true);
      fromExit->destRoom = roomPtr->roomNumber;

      createExit(&toExit, true);
      toExit->destRoom = g_currentRoom->roomNumber;

      g_currentRoom->exits[direction] = fromExit;

     // if new room was created from a default room, there may be an exit, so delete it

      if (roomPtr->exits[g_revdirs[direction]])
        deleteRoomExit(roomPtr->exits[g_revdirs[direction]], true);

      roomPtr->exits[g_revdirs[direction]] = toExit;

     // create master and editable lists

      roomPtr->masterListHead = createMasterKeywordList(roomPtr);
      roomPtr->editableListHead = createEditableList(roomPtr);

      displayColorString("\n"
"&+cNew room created ('create room as you walk' mode).&n\n");

      goDirection(direction);
    }
    else
    {
      _outtext("\nYou can't go that direction (no exit).\n\n");
    }
  }
}


//
// copyAllExits : copy all exits in srcRoom into destRoom
//

void copyAllExits(const room *srcRoom, room *destRoom, const bool blnIncNumbExits)
{
  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    destRoom->exits[i] = copyRoomExit(srcRoom->exits[i], blnIncNumbExits);
  }
}


//
// deleteAllExits : delete all exits in a room
//

void deleteAllExits(room *roomPtr, const bool blnDecNumbExits)
{
  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    deleteRoomExit(roomPtr->exits[i], blnDecNumbExits);
    roomPtr->exits[i] = NULL;
  }
}


//
// copyRoomInfo : copies all the info from a room record into a new record and
//                returns the address of the new record
//
//     *srcRoom : source room record
//    incLoaded : if TRUE, increments internal counter
//

room *copyRoomInfo(const room *srcRoom, const bool incLoaded)
{
  room *newRoom;

 // make sure src exists

  if (!srcRoom) 
    return NULL;

 // alloc mem for new rec

  newRoom = new(std::nothrow) room;
  if (!newRoom)
  {
    displayAllocError("room", "copyRoomInfo");

    return NULL;
  }

 // first, copy the simple stuff

  memcpy(newRoom, srcRoom, sizeof(room));

 // next, the not-so-simple stuff

  newRoom->roomDescHead = copyStringNodes(srcRoom->roomDescHead);

 // room exits..

  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    newRoom->exits[i] = copyRoomExit(srcRoom->exits[i], incLoaded);
  }

 // extra desc linked list

  newRoom->extraDescHead = copyExtraDescs(srcRoom->extraDescHead);

 // object list

  newRoom->objectHead = copyObjHereList(srcRoom->objectHead, incLoaded);

 // mob list

  newRoom->mobHead = copyMobHereList(srcRoom->mobHead, incLoaded);

 // master list

  newRoom->masterListHead = createMasterKeywordList(newRoom);

 // editable list

  newRoom->editableListHead = createEditableList(newRoom);


  if (incLoaded)
  {
    g_numbRooms++;
    createPrompt();
  }

 // return the address of the new room

  return newRoom;
}


//
// compareRoomInfo : compares almost all - returns FALSE if they don't
//                   match, TRUE if they do
//
//                   doesn't compare obj/mobHere lists, master/editable lists,
//                   or defaultRoom var
//

bool compareRoomInfo(const room *room1, const room *room2)
{
  if (room1 == room2) 
    return true;

  if (!room1 || !room2) 
    return false;

 // check all room attributes

  if (strcmp(room1->roomName, room2->roomName)) 
    return false;
  if (room1->roomNumber != room2->roomNumber) 
    return false;
  if (room1->zoneNumber != room2->zoneNumber) 
    return false;
  if (room1->roomFlags  != room2->roomFlags) 
    return false;
  if (room1->sectorType != room2->sectorType) 
    return false;
  if (room1->fallChance != room2->fallChance) 
    return false;
  if (room1->manaFlag   != room2->manaFlag) 
    return false;
  if (room1->manaApply  != room2->manaApply) 
    return false;
  if (room1->current    != room2->current) 
    return false;
  if (room1->currentDir != room2->currentDir) 
    return false;

 // description and extra descs

  if (!compareStringNodes(room1->roomDescHead, room2->roomDescHead))
    return false;

  if (!compareExtraDescs(room1->extraDescHead, room2->extraDescHead))
    return false;

 // compare exits

  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    if (!compareRoomExits(room1->exits[i], room2->exits[i])) 
      return false;
  }

  return true;
}


//
// deleteRoomInfo : deletes all the associated baggage with a room rec
//
//     *srcRoom : record to delete
//

void deleteRoomInfo(room *srcRoom, const bool decNumbRooms, const bool decLoaded)
{
 // make sure src exists

  if (!srcRoom) 
    return;

 // delete room desc

  deleteStringNodes(srcRoom->roomDescHead);

 // room exits..

  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    deleteRoomExit(srcRoom->exits[i], decLoaded);
  }

 // extra desc linked list

  deleteExtraDescs(srcRoom->extraDescHead);

 // object list

  deleteObjHereList(srcRoom->objectHead, decLoaded);

 // mob list

  deleteMobHereList(srcRoom->mobHead, decLoaded);

 // master list

  deleteMasterKeywordList(srcRoom->masterListHead);

 // editable list

  deleteEditableList(srcRoom->editableListHead);

 // if requested, remove from room list

  if (decNumbRooms)
  {
    g_numbRooms--;
    g_roomLookup[srcRoom->roomNumber] = NULL;

   // assume no caller will ever try to delete the only remaining room

    if (g_numbRooms == 1) 
    {
      if (srcRoom->roomNumber == g_lowestRoomNumber)
        g_lowestRoomNumber = g_highestRoomNumber;
      else
        g_highestRoomNumber = g_lowestRoomNumber;
    }
    else
    {
      if (srcRoom->roomNumber == getLowestRoomNumber()) 
        g_lowestRoomNumber = getNextRoom(srcRoom->roomNumber)->roomNumber;

      if (srcRoom->roomNumber == getHighestRoomNumber()) 
        g_highestRoomNumber = getPrevRoom(srcRoom->roomNumber)->roomNumber;
    }

    g_madeChanges = true;
  }

 // finally, delete the record itself

  delete srcRoom;

  createPrompt();
}


//
// getHighestRoomNumber : Returns the highest room number currently in the
//                        room list
//

uint getHighestRoomNumber(void)
{
  return g_highestRoomNumber;
}


//
// getLowestRoomNumber : Returns the lowest room number currently in the
//                       room list
//

uint getLowestRoomNumber(void)
{
  return g_lowestRoomNumber;
}


//
// getFirstFreeRoomNumber : Returns the first free room number, starting at
//                          the current lowest number + 1
//

uint getFirstFreeRoomNumber(void)
{
  for (uint i = g_lowestRoomNumber + 1; i <= g_highestRoomNumber - 1; i++)
  {
    if (!g_roomLookup[i]) 
      return i;
  }

  return g_highestRoomNumber + 1;
}


//
// checkAndFixRefstoRoom : scans through the object type list, looking for
//                         the few object types that specify a room vnum
//                         in one of their values and changing it to
//                         new number if necessary
//
//      oldNumb : old vnum
//      newNumb : new vnum
//

void checkAndFixRefstoRoom(const uint oldNumb, const uint newNumb)
{
  const uint highObjNumb = getHighestObjNumber();


 // scan through object types

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      objectType *obj = findObj(objNumb);

      for (uint i = 0; i < NUMB_OBJ_VALS; i++)
      {
        if ((fieldRefsRoomNumb(obj->objType, i) && (obj->objValues[i] == oldNumb)))
        {
          obj->objValues[i] = newNumb;

          g_madeChanges = true;
        }
      }
    }
  }
}


//
// renumberRooms : Renumbers the rooms so that there are no "gaps" - starts
//                 at the first room and simply renumbers from there
//
//   renumberHead : if true, renumbers entire range of rooms starting at newHeadNumb -
//                  if false, keeps old start vnum and removes any gaps
//    newHeadNumb : ignored if renumberHead is false
//

// note that if renumberHead is true, this code assumes that the old and new range do not overlap.
// if they do overlap, you're screwed

void renumberRooms(const bool renumberHead, const uint newHeadNumb)
{
  uint roomNumb = getLowestRoomNumber(), lastNumb, oldNumb;
  room *roomPtr = findRoom(roomNumb);


 // basic technique - keep all old room pointers in g_roomLookup table until clearing them at the
 // very end so that resetExits() call works; similarly, do not reset low/high room number until
 // the very end

  if (renumberHead)
  {
    oldNumb = roomNumb;
    roomPtr->roomNumber = lastNumb = newHeadNumb;

    resetExits(roomNumb, newHeadNumb);
    checkAndFixRefstoRoom(roomNumb, newHeadNumb);

    g_roomLookup[newHeadNumb] = roomPtr;

    g_madeChanges = true;
  }
  else
  {
    roomNumb = oldNumb = lastNumb = getLowestRoomNumber();
  }

 // get next room

  roomPtr = getNextRoom(oldNumb);

 // number all consecutive rooms as 1 + previous vnum

  while (roomPtr)
  {
    oldNumb = roomNumb = roomPtr->roomNumber;

    if (roomNumb != (lastNumb + 1))
    {
      roomPtr->roomNumber = roomNumb = lastNumb + 1;

      resetExits(oldNumb, roomNumb);
      checkAndFixRefstoRoom(oldNumb, roomNumb);

      g_madeChanges = true;

      g_roomLookup[roomNumb] = roomPtr;
      if (!renumberHead)
        g_roomLookup[oldNumb] = NULL;
    }

    lastNumb = roomPtr->roomNumber;

    roomPtr = getNextRoom(oldNumb);
  }

 // clear old range

  if (renumberHead)
  {
   // entire range was moved, assumes no overlap

    memset(g_roomLookup + g_lowestRoomNumber, 0, ((g_highestRoomNumber - g_lowestRoomNumber) + 1) * sizeof(room*));
  }
  else
  {
   // if the head vnum wasn't changed, then the most that happened was that the high vnum came down

    memset(g_roomLookup + lastNumb + 1, 0, (g_highestRoomNumber - lastNumb) * sizeof(room*));
  }

 // set new low/high

  if (renumberHead)
    g_lowestRoomNumber = newHeadNumb;

  g_highestRoomNumber = lastNumb;
}


//
// updateAllObjMandElists : updates masterKeyword and editable lists of all rooms with objHeres
//

void updateAllObjMandElists(void)
{
  const uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      if (roomPtr->objectHead)
      {
        deleteMasterKeywordList(roomPtr->masterListHead);
        roomPtr->masterListHead = createMasterKeywordList(roomPtr);

        deleteEditableList(roomPtr->editableListHead);
        roomPtr->editableListHead = createEditableList(roomPtr);
      }
    }
  }
}


//
// updateAllMobMandElists : updates masterKeyword and editable lists of all rooms with mobHeres
//

void updateAllMobMandElists(void)
{
  const uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      if (roomPtr->mobHead)
      {
        deleteMasterKeywordList(roomPtr->masterListHead);
        roomPtr->masterListHead = createMasterKeywordList(roomPtr);

        deleteEditableList(roomPtr->editableListHead);
        roomPtr->editableListHead = createEditableList(roomPtr);
      }
    }
  }
}


//
// noExitsLeadtoRoom : returns true if no exits in the zone lead to room vnum
//

bool noExitsLeadtoRoom(const uint destRoomNumb)
{
  const uint highRoomNumb = getHighestRoomNumber();

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      const room *roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (roomPtr->exits[i] && (roomPtr->exits[i]->destRoom == destRoomNumb)) 
          return false;
      }
    }
  }

  return true;
}


//
// noExitOut : returns true if no exits in room lead out (dest room >= 0)
//

bool noExitOut(const room *roomPtr)
{
  if (!roomPtr) 
    return false;

  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    if (isExitOut(roomPtr->exits[i])) 
      return false;
  }

  return true;
}


//
// roomHasAllFollowers : checks all rooms to see if any are filled with
//                       mobs that are all following somebody - no way
//                       to write this out to the zone file, you see -
//                       returns -1 if no room found, room number otherwise
//

int roomHasAllFollowers(void)
{
  const uint highRoomNumb = getHighestRoomNumber();

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      const room *roomPtr = findRoom(roomNumb);

      if (roomPtr->mobHead)
      {
        bool allFollow = true;

        const mobHere *mob = roomPtr->mobHead;
        while (mob)
        {
          if (!mob->following)
          {
            allFollow = false;
            break;
          }

          mob = mob->Next;
        }

        if (allFollow) 
          return roomPtr->roomNumber;
      }
    }
  }

  return -1;
}


//
// getRoomKeyword : returns a pointer to the first room that contains
//                  an obj or mob with the specified keyword.  if
//                  checkRooms is set, checks room titles as well.
//

room *getRoomKeyword(const char *key, const bool checkRooms)
{
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestMobNumber();


  if (strnumer(key))
  {
    return findRoom(strtoul(key, NULL, 10));
  }

  room *roomPtr = findRoom(getLowestRoomNumber());

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      const objectType *obj = findObj(objNumb);

      if (scanKeyword(key, obj->keywordListHead))
      {
        uint numb;

        if (findObjHere(objNumb, &numb, false, false)) 
          return findRoom(numb);
      }
    }
  }

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      const mobType *mob = findMob(mobNumb);

      if (scanKeyword(key, mob->keywordListHead))
      {
        uint numb;

        if (findMobHere(mobNumb, &numb, false)) 
          return findRoom(numb);
      }
    }
  }

  if (checkRooms)
  {
    for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
    {
      if (roomExists(roomNumb))
      {
        char tempStrn[MAX_ROOMNAME_LEN + 1];

        roomPtr = findRoom(roomNumb);

        strcpy(tempStrn, roomPtr->roomName);
        remColorCodes(tempStrn);

        if (strstrnocase(tempStrn, key)) 
          return roomPtr;
      }
    }
  }

  return NULL;
}


//
// getNumbFreeRooms : returns number of unused room vnums between low and high
//

uint getNumbFreeRooms(void)
{
  room * const* roomLookupPtr = g_roomLookup;
  room * const* roomLookupEnd = g_roomLookup + g_numbLookupEntries;
  uint numb = 0;

  while (roomLookupPtr != roomLookupEnd)
  {
    if (*roomLookupPtr == NULL)
      numb++;

    roomLookupPtr++;
  }

  return numb;
}


//
// getPrevRoom : find room right before roomNumb, numerically
//

room *getPrevRoom(const uint roomNumb)
{
  room * const* roomLookupPtr = g_roomLookup + roomNumb - 1;

  if (roomNumb <= getLowestRoomNumber()) 
    return NULL;

  while (*roomLookupPtr == NULL)
    roomLookupPtr--;

  return *roomLookupPtr;
}


//
// getPrevRoom : room pointer version
//

room *getPrevRoom(const room *roomPtr)
{
  if (roomPtr == NULL)
    return NULL;

  return getPrevRoom(roomPtr->roomNumber);
}


//
// getNextRoom : find room right after roomNumb, numerically
//

room *getNextRoom(const uint roomNumb)
{
  room * const* roomLookupPtr = g_roomLookup + roomNumb + 1;

  if (roomNumb >= getHighestRoomNumber()) 
    return NULL;

  while (*roomLookupPtr == NULL)
    roomLookupPtr++;

  return *roomLookupPtr;
}


//
// getNextRoom : room pointer version
//

room *getNextRoom(const room *roomPtr)
{
  if (roomPtr == NULL)
    return NULL;

  return getNextRoom(roomPtr->roomNumber);
}


//
// checkMapTruenessRedund : redundant checking for checkMapTrueness
//

bool checkMapTruenessRedund(const room *roomPtr, const uint exitNumb, const int dest, size_t &lines,
                            bool *gotOne)
{
  char strn[1024];

  if (roomPtr->exits[exitNumb] && (roomPtr->exits[exitNumb]->destRoom != dest))
  {
    sprintf(strn, "  &+croom #%%&+g%u&n's %s exit leads to &+R%d&n &+ginstead of &+R%d&n\n",
            roomPtr->roomNumber, g_exitnames[exitNumb], roomPtr->exits[exitNumb]->destRoom, dest);

    *gotOne = true;

    if (checkPause(strn, lines))
      return true;
  }

  return false;
}


//
// checkMapTrueness : checks map 'trueness' - a truly perfect map will
//                    have rooms that all have north exits leading to room
//                    numb - 100, east exits to room numb + 1, and the
//                    opposite for south/west
//

void checkMapTrueness(void)
{
  size_t lines = 2;
  const uint highRoomNumb = getHighestRoomNumber();
  bool gotOne = false;


  _outtext("\n\n");

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      const room *roomPtr = findRoom(roomNumb);
      const uint numb = roomPtr->roomNumber;

      if (checkMapTruenessRedund(roomPtr, NORTH, numb - 100, lines, &gotOne) ||
          checkMapTruenessRedund(roomPtr, SOUTH, numb + 100, lines, &gotOne) ||
          checkMapTruenessRedund(roomPtr, WEST,  numb - 1, lines, &gotOne) ||
          checkMapTruenessRedund(roomPtr, EAST,  numb + 1, lines, &gotOne))
      {
        _outtext("\n");
        return;
      }
    }
  }

  if (!gotOne)
    _outtext("All exits match expected 100x100 zone map format.\n");

  _outtext("\n");
}
