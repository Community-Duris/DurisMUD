//
//  File: exit.cpp       originally part of durisEdit
//
//  Usage: functions related to room exits
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

#include "../fh.h"
#include "../types.h"

#include "exit.h"

extern uint g_numbExits;
extern bool g_madeChanges;
extern room *g_currentRoom;


const char *g_exitnames[NUMB_EXITS] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest",
  "southwest",
  "northeast",
  "southeast"
};

const char *g_exitnames_short[NUMB_EXITS] =
{
  "N",
  "E",
  "S",
  "W",
  "U",
  "D",
  "NW",
  "SW",
  "NE",
  "SE"
};

const char *g_exitkeys[NUMB_EXITS] =
{
  "N",
  "E",
  "S",
  "W",
  "U",
  "D",
  "1",
  "3",
  "2",
  "4"
};

uint g_exitflags[NUMB_EXITS] =
{
  EXIT_NORTH_FLAG,
  EXIT_EAST_FLAG,
  EXIT_SOUTH_FLAG,
  EXIT_WEST_FLAG,
  EXIT_UP_FLAG,
  EXIT_DOWN_FLAG,
  EXIT_NORTHWEST_FLAG,
  EXIT_SOUTHWEST_FLAG,
  EXIT_NORTHEAST_FLAG,
  EXIT_SOUTHEAST_FLAG
};

char g_revdirs[NUMB_EXITS] =
{
  2,
  3,
  0,
  1,
  5,
  4,
  9,
  8,
  7,
  6
};

//
// getDirfromKeyword : based on data in strn, return corresponding exit
//                     direction or NO_EXIT if no matches found
//
//    strn : exit direction name (hopefully)
//

char getDirfromKeyword(const char *strn)
{
  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    if (strcmpnocase(strn, g_exitnames[i]) || strcmpnocase(strn, g_exitnames_short[i]))
      return i;
  }

  return NO_EXIT;
}


//
// exitNeedsKey : if the exit passed by pointer has a keyNumb
//                equal to keyNumb passed, return TRUE
//
//       ex : exit pointer to check
//  keyNumb : value to check for
//

bool exitNeedsKey(const roomExit *ex, const int keyNumb)
{
  return (ex && (ex->keyNumb == keyNumb));
}


//
// checkRoomExitKey : Checks the exit key object number for validity (if it
//                    isn't found in the object list, it's considered
//                    invalid).
//
//    exitNode : pointer to exit node being checked
//        room : pointer to room that contains exit
//    exitName : "name" of exit - direction it leads
//

void checkRoomExitKey(const roomExit *exitNode, const room *room, const char *exitName)
{
  char outstrn[512];


  if (exitNode && (exitNode->keyNumb))
  {
    if (!objExists(exitNode->keyNumb) && (exitNode->keyNumb != -1) &&
        (exitNode->keyNumb != -2) && getVnumCheckVal())
    {
      sprintf(outstrn, "\n\n"
"Warning: Room #%u's %s exit requires a key that doesn't exist in this\n"
"         .obj file (#%d).\n\n"
"Hit any key to continue ...",
              room->roomNumber, exitName, exitNode->keyNumb);

      _outtext(outstrn);

      getkey();
    }
  }
}


//
// compareRoomExits : returns TRUE if they match, FALSE if they don't
//
//  *exit1 : first
//  *exit2 : second
//

bool compareRoomExits(const roomExit *exit1, const roomExit *exit2)
{
  if (exit1 == exit2) 
    return true;
  if (!exit1 || !exit2) 
    return false;

  if (exit1->worldDoorType != exit2->worldDoorType) 
    return false;
  if (exit1->zoneDoorState != exit2->zoneDoorState) 
    return false;

  if (exit1->keyNumb != exit2->keyNumb) 
    return false;
  if (exit1->destRoom != exit2->destRoom) 
    return false;

  if (!compareStringNodes(exit1->exitDescHead, exit2->exitDescHead)) 
    return false;

  return compareStringNodesIgnoreCase(exit1->keywordListHead, exit2->keywordListHead);
}


//
// deleteRoomExit : deletes the specified exit
//
//   *srcExit : exit to delete
//

void deleteRoomExit(roomExit *srcExit, const bool decNumbExits)
{
  if (!srcExit) 
    return;


  deleteStringNodes(srcExit->exitDescHead);
  deleteStringNodes(srcExit->keywordListHead);

  delete srcExit;

  if (decNumbExits)
  {
    g_numbExits--;

    createPrompt();

    g_madeChanges = true;
  }
}


//
// copyRoomExit : copies a room exit into a new exit record, returning the
//                address to that new exit
//
//   srcExit : exit to copy from
//

roomExit *copyRoomExit(const roomExit *srcExit, const bool incNumbLoaded)
{
  roomExit *newExit;


  if (!srcExit) 
    return NULL;

 // alloc mem for new exit rec

  newExit = new(std::nothrow) roomExit;
  if (!newExit)
  {
    displayAllocError("roomExit", "copyRoomExit");

    return NULL;
  }

 // copy all the easy stuff

  memcpy(newExit, srcExit, sizeof(roomExit));

 // copy all the linked lists

  newExit->exitDescHead = copyStringNodes(srcExit->exitDescHead);
  newExit->keywordListHead = copyStringNodes(srcExit->keywordListHead);

  if (incNumbLoaded)
  {
    g_numbExits++;

    createPrompt();
  }

 // return the address of the new rec

  return newExit;
}


//
// getExitTakenFlags : Sets an unsigned int's bits based on which exits
//                     are "taken" - more accurately, a corresponding bit
//                     is set if a room's particular exit pointer is non-NULL.
//                     The value in the unsigned int is returned.
//
//    roomPtr : room to check
//

uint getExitTakenFlags(const room *roomPtr)
{
  uint exitFlag = 0;

  if (!roomPtr) 
    return 0;

  for (uint i = 0; i < NUMB_EXITS; i++)
    if (roomPtr->exits[i]) 
      exitFlag |= g_exitflags[i];

  return exitFlag;
}


//
// getRoomExitsShortStrn : returns a readable string that shows which
//                         exits exist in the specified room.  string is
//                         copied into strn
//
//     room : room to base string on
//     strn : string to copy stuff into
//

char *getRoomExitsShortStrn(const room *room, char *strn)
{
  uint exitFlag;


  strn[0] = '\0';

  if (room) 
    exitFlag = getExitTakenFlags(room);
  else 
    return strn;

  if (exitFlag & EXIT_NORTH_FLAG) strcat(strn, "N ");
  if (exitFlag & EXIT_NORTHWEST_FLAG) strcat(strn, "NW ");
  if (exitFlag & EXIT_NORTHEAST_FLAG) strcat(strn, "NE ");
  if (exitFlag & EXIT_SOUTH_FLAG) strcat(strn, "S ");
  if (exitFlag & EXIT_SOUTHWEST_FLAG) strcat(strn, "SW ");
  if (exitFlag & EXIT_SOUTHEAST_FLAG) strcat(strn, "SE ");
  if (exitFlag & EXIT_WEST_FLAG) strcat(strn, "W ");
  if (exitFlag & EXIT_EAST_FLAG) strcat(strn, "E ");
  if (exitFlag & EXIT_UP_FLAG) strcat(strn, "U ");
  if (exitFlag & EXIT_DOWN_FLAG) strcat(strn, "D ");

  if (strlen(strn)) 
    strn[strlen(strn) - 1] = '\0';

  return strn;
}


//
// swapExits : Swaps two exits of current room
//
//             This function returns TRUE if the parser recognized the
//             keywords, FALSE otherwise
//
//   *strn : user input taken from prompt
//

bool swapExits(const char *args)
{
  char strn[256], arg1[256], arg2[256], val;
  roomExit *exit1, *exit2;
  uint exitType1, exitType2;


  getArg(args, 1, arg1, 255);
  getArg(args, 2, arg2, 255);

  if ((arg1[0] == '\0') || (arg2[0] == '\0'))
  {
    _outtext(
"\nInvalid arguments specified - the format is 'swap <exit1 dir> <exit2 dir>'.\n\n");
    return false;
  }

  val = getDirfromKeyword(arg1);
  if (val == NO_EXIT)
  {
    _outtext(
"\nInvalid arguments specified - the format is 'swap <exit1 dir> <exit2 dir>'.\n\n");
    return false;
  }

  exit1 = g_currentRoom->exits[val];
  exitType1 = val;

  val = getDirfromKeyword(arg2);
  if (val == NO_EXIT)
  {
    _outtext(
"\nInvalid arguments specified - the format is 'swap <exit1 dir> <exit2 dir>'.\n\n");
    return false;
  }

  exit2 = g_currentRoom->exits[val];
  exitType2 = val;

  if (exitType1 == exitType2)
  {
    _outtext("\nYou can't swap an exit with itself.\n\n");
    return false;
  }

  g_currentRoom->exits[exitType1] = exit2;
  g_currentRoom->exits[exitType2] = exit1;

  sprintf(strn, "\nSwapped %s exit with %s exit.\n\n",
          g_exitnames[exitType1], g_exitnames[exitType2]);
  _outtext(strn);

  if (!compareRoomExits(exit1, exit2))
    g_madeChanges = true;

  return true;
}


//
// swapExitsNorthSouth : Swaps all north/south exits, "reversing" map
//

void swapExitsNorthSouth(void)
{
  const uint highRoomNumb = getHighestRoomNumber();

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      roomExit *tmpExit = roomPtr->exits[SOUTH];
      roomPtr->exits[SOUTH] = roomPtr->exits[NORTH];
      roomPtr->exits[NORTH] = tmpExit;
    }
  }

  g_madeChanges = true;

  displayColorString("\n&+cAll north/south exits in all rooms swapped.\n\n");
}


//
// swapExitsWestEast : Swaps all west/east exits, "reversing" map
//

void swapExitsWestEast(void)
{
  const uint highRoomNumb = getHighestRoomNumber();

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      roomExit *tmpExit = roomPtr->exits[EAST];
      roomPtr->exits[EAST] = roomPtr->exits[WEST];
      roomPtr->exits[WEST] = tmpExit;
    }
  }

  g_madeChanges = true;

  displayColorString("\n&+cAll west/east exits in all rooms swapped.\n\n");
}


//
// swapExitsUpDown : Swaps all up/down exits, "reversing" map
//

void swapExitsUpDown(void)
{
  const uint highRoomNumb = getHighestRoomNumber();

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      roomExit *tmpExit = roomPtr->exits[DOWN];
      roomPtr->exits[DOWN] = roomPtr->exits[UP];
      roomPtr->exits[UP] = tmpExit;
    }
  }

  g_madeChanges = true;

  displayColorString("\n&+cAll up/down exits in all rooms swapped.\n\n");
}


//
// getWorldDoorType : Returns the door type of a specific exit record
//
//   exitRec : exit record to return info on
//

int getWorldDoorType(const roomExit *exitRec)
{
  if (exitRec) 
    return exitRec->worldDoorType;
  else 
    return 0;
}


//
// getZoneDoorState : Returns the door state of a specific exit record
//
//   exitRec : exit record to return info on
//

int getZoneDoorState(const roomExit *exitRec)
{
  if (exitRec) 
    return exitRec->zoneDoorState;
  else 
    return 0;
}


//
// resetExits : Searches through the global list of rooms, looking for exits
//              that lead to destRoom and setting to newDest.
//
//     destRoom : original destination
//      newDest : new destination
//

void resetExits(const int destRoom, const int newDest)
{
  const uint highRoomNumb = getHighestRoomNumber();


  if (destRoom == newDest) 
    return;

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomExit **exitPtr = findRoom(roomNumb)->exits;

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (*exitPtr && ((*exitPtr)->destRoom == destRoom))
          (*exitPtr)->destRoom = newDest;

        exitPtr++;
      }
    }
  }
}


//
// clearExits : Searches through a list of rooms, deleting all exits
//              that lead to destRoom
//
//       destRoom : destination room
//   decNumbExits : if TRUE, decrements internal number of exits

void clearExits(const int destRoom, const bool decNumbExits)
{
  uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (roomPtr->exits[i] && (roomPtr->exits[i]->destRoom == destRoom))
        {
          deleteRoomExit(roomPtr->exits[i], decNumbExits);
          roomPtr->exits[i] = NULL;
        }
      }
    }
  }
}


//
// findCorrespondingExit : Finds an exit that is in srcRoom and leads to
//                         destRoom - tries to, anyway.  exitDir is set
//                         to the direction of the matching exit
//

roomExit *findCorrespondingExit(const int srcRoom, const int destRoom, char *exitDir)
{
  room *roomPtr;


  roomPtr = findRoom(srcRoom);
  if (!roomPtr) 
    return NULL;

  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    if (roomPtr->exits[i] && (roomPtr->exits[i]->destRoom == destRoom))
    {
      *exitDir = i;
      return roomPtr->exits[i];
    }
  }

  return NULL;
}


//
// getExitStrn : Returns a descriptive string based on the exit direction
//               value
//

const char *getExitStrn(const int exitDir)
{
  if ((exitDir >= 0) && (exitDir < NUMB_EXITS)) 
    return g_exitnames[exitDir];
  else 
    return "unrecognized";
}


//
// getExitNode : Based on a room number and an exit direction, returns the
//               corresponding exit node
//

roomExit *getExitNode(const uint roomNumb, const char exitDir)
{
  return getExitNode(findRoom(roomNumb), exitDir);
}


//
// getExitNode : Based on a room address and an exit direction, returns the
//               corresponding exit node
//

roomExit *getExitNode(const room *roomPtr, const char exitDir)
{
  if (!roomPtr || (exitDir < 0) || (exitDir >= NUMB_EXITS)) 
    return NULL;

  return roomPtr->exits[exitDir];
}


//
// checkExit : checks an exit to make sure it has a corresponding exit on
//             the opposite side with valid type and state set based on the
//             type and state for this exit - prompts user to fix (and fixes)
//             if any errors are found
//
//    exitNode : node to check
//    exitName : name of exit (north, south, etc)
//    roomNumb : number of room exitNode is in
//

void checkExit(roomExit *exitNode, const char *exitName, const int roomNumb)
{
  int zoneType, worldType, zoneBits, worldBits;
  char strn[512];
  usint ch;


  if (!exitNode) 
    return;

 // make sure that exit doesn't have a zone state of 1 or 2 and no door
 // state of 1 2 or 3

  zoneType = exitNode->zoneDoorState & 3;
  zoneBits = exitNode->zoneDoorState & 12;

  worldType = exitNode->worldDoorType & 3;
  worldBits = exitNode->worldDoorType & 12;

  if (zoneType && !worldType)
  {
    sprintf(strn, "\n\n"
"Warning: %s exit in room #%u has a door state of %u set in the .zon, but\n"
"         a door type of 'no door' in the .wld.\n\n"
"Set type/flags in .zon equal to %u (Z), or set door type in .wld to %u (W)? ",

   exitName, roomNumb, exitNode->zoneDoorState,
   worldType | zoneBits, zoneType | worldBits);

    _outtext(strn);

    do
    {
      ch = toupper(getkey());
    } while ((ch != 'Z') && (ch != 'W'));

    sprintf(strn, "%c\n\n", (char)ch);
    _outtext(strn);

    if (ch == 'Z') 
      exitNode->zoneDoorState = worldType | zoneBits;
    else 
      exitNode->worldDoorType = zoneType | worldBits;

    g_madeChanges = true;
  }
}


//
// checkAllExits : checks all exits in the zone, calling checkExit() for each
//

void checkAllExits(void)
{
  uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      const room *roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        char strn[64];

        strncpy(strn, g_exitnames[i], 63);
        strn[63] = '\0';

        strn[0] = toupper(strn[0]);

        checkExit(roomPtr->exits[i], strn, roomPtr->roomNumber);
      }
    }
  }
}


//
// getDescExitStrnforZonefile : Creates a descriptive string about an exit -
//                              used for comments in zones
//
//       room : room exit is in
//    exitRec : specific exit
//    endStrn : string is put in here
//   exitName : "name" of exit - north, south, etc
//

char *getDescExitStrnforZonefile(const room *roomPtr, const roomExit *exitRec, char *endStrn, 
                                 const char *exitName)
{
  char fromRoom[MAX_ROOMNAME_LEN + 1], toRoom[MAX_ROOMNAME_LEN + 1], exitInfo[256];
  int doorState;
  room *tmpRoom;


  strcpy(fromRoom, roomPtr->roomName);
  remColorCodes(fromRoom);

  tmpRoom = findRoom(exitRec->destRoom);
  if (tmpRoom)
  {
    strcpy(toRoom, tmpRoom->roomName);
    remColorCodes(toRoom);
  }
  else
  {
    strcpy(toRoom, "[destination room not found]");
  }

 // now, fill up exitInfo string

  doorState = getZoneDoorState(exitRec);

//  if (doorState)
//  {
    strcpy(exitInfo, " - ");

    if ((doorState & 3) == 3)
    {
      strcat(exitInfo, "closed/locked/unpickable/");
    }
    else
    if (doorState & 1)
    {
      strcat(exitInfo, "closed/");
    }
    else
    if (doorState & 2)
    {
      strcat(exitInfo, "closed/locked/");
    }
    else strcat(exitInfo, "open/");

    if (doorState & 4)
    {
      strcat(exitInfo, "secret/");
    }

    if (doorState & 8)
    {
      strcat(exitInfo, "blocked/");
    }

   // cut out the final trailing slash

    exitInfo[strlen(exitInfo) - 1] = '\0';
//  }

 // compose string to be suitable for zone file

  sprintf(endStrn, "*     %s exit from \"%s\" to\n"
                   "*     \"%s\"%s\n*\n",
                         exitName, fromRoom, toRoom, exitInfo);

  return endStrn;
}


//
// getNumbExits : returns number of exits in room passed
//
//    room : room to check
//

char getNumbExits(const room *roomPtr)
{
  char numb = 0;

  if (!roomPtr) 
    return 0;

  for (uint i = 0; i < NUMB_EXITS; i++) 
    if (roomPtr->exits[i]) 
      numb++;

  return numb;
}


//
// isExitOut : returns TRUE if exit is considered to lead to somewhere
//             (exit destination room does not have to exist, a destination
//             of less than 0 is considered to be nowhere)
//
//     ex : exit to check
//

bool isExitOut(const roomExit *ex)
{
  return (ex && (ex->destRoom >= 0));
}


//
// roomExitOutsideZone : returns TRUE if exit is considered to lead outside
//                       the zone
//
//   ex : exit to check
//

bool roomExitOutsideZone(const roomExit *ex)
{
  return (ex && (((ex->destRoom < 0) && getNegDestOutofZoneVal()) ||
                 ((ex->destRoom >= 0) && !roomExists(ex->destRoom))));
}
