//
//  File: readwld.cpp    originally part of durisEdit
//
//  Usage: functions for reading room info from the .wld file
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



#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fh.h"
#include "types.h"

#include "room.h"
#include "exit.h"
#include "edesc/edesc.h"
#include "vardef.h"
#include "readfile.h"



extern uint g_numbExits, g_numbLookupEntries, g_numbRooms, g_lowestRoomNumber, g_highestRoomNumber;
extern room **g_roomLookup;
extern bool g_madeChanges, g_readFromSubdirs;
extern variable *g_varHead;


//
// readRoomExitFromFile : Reads a room exit, basically
//
//  *worldFile : pointer to world file
//    *roomPtr : pointer to room that exit belongs to - if NULL, default exit
//   *exitStrn : string as read from worldFile that contains exit info
//

roomExit *readRoomExitFromFile(FILE *worldFile, room *roomPtr, const char *exitStrn, const bool incNumbExits)
{
  roomExit *roomExitRec;
  char currentExit, strn[512];
  uint roomNumb;


  if (roomPtr)
    roomNumb = roomPtr->roomNumber;
  else
    roomNumb = 0;

 // make sure that the string is at least the right length

  if ((strlen(exitStrn) != 2) || (toupper(exitStrn[0]) != 'D'))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: No or incorrect exit type specified in exit data for room number\n"
"       %u.\n\n"
"       Aborting.\n",
            roomNumb);

    _outtext(outstrn);

    exit(1);
  }

 // allocate memory for the roomExitRec

  roomExitRec = new(std::nothrow) roomExit;
  if (!roomExitRec)
  {
    displayAllocError("roomExit", "readRoomExit");

    exit(1);
  }

 // clear it

  memset(roomExitRec, 0, sizeof(roomExit));

 // set currentExit

  currentExit = exitStrn[1];
  if ((currentExit < EXIT_FIRST_CH) || (currentExit > EXIT_LAST_CH))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Invalid exit direction for room #%u specified ('%c')\n\n"
"       Aborting.\n",
            roomNumb, currentExit);

    _outtext(outstrn);

    exit(1);
  }

 // read the exit description

  roomExitRec->exitDescHead = readStringNodes(worldFile, TILDE_LINE, true);

 // read keyword string for exit

  if (!readAreaFileLine(worldFile, strn, MAX_EXITKEY_LEN + 2, ENTITY_EXIT, currentExit - '0', 
                        ENTITY_ROOM, roomNumb, "keyword", 0, NULL, true, true))
    exit(1);

 // create a keyword list

  roomExitRec->keywordListHead = createKeywordList(strn);

 // read the rest of the exit info

  if (!readAreaFileLine(worldFile, strn, 512, ENTITY_EXIT, currentExit - '0', 
                        ENTITY_ROOM, roomNumb, "misc info", 3, NULL, false, true))
    exit(1);

  sscanf(strn, "%d%d%d",
         &(roomExitRec->worldDoorType), &(roomExitRec->keyNumb),
         &(roomExitRec->destRoom));

 // check door type

  const int dtype = roomExitRec->worldDoorType;

  if ((dtype < 0) || (dtype > 15))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: The door type specified for exit #%c in room #%u is an\n"
"       impossible value (%d).\n\n"
"       Aborting.\n",
            currentExit, roomNumb, dtype);

    _outtext(outstrn);

    exit(1);
  }

 // done reading the exit, now put it in the room

  if (roomPtr)
  {
    currentExit -= '0';
    roomPtr->exits[currentExit] = roomExitRec;
  }

  if (incNumbExits) 
    g_numbExits++;

  return roomExitRec;
}


//
// readRoomFromFile : Returns pointer to new roomOldNode
//

room *readRoomFromFile(FILE *worldFile, const bool checkDupes, const bool incNumbRooms)
{
  char strn[1024];
  room *roomPtr;


 // Read the room number

  bool hitEOF;

  if (!readAreaFileLineAllowEOF(worldFile, strn, 512, ENTITY_ROOM, ENTITY_NUMB_UNUSED, 
                                ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, "vnum", 0, NULL, false, true, 
                                &hitEOF))
    exit(1);

  if (hitEOF)
    return NULL;  // reached end of world file

 // allocate memory for room

  roomPtr = new(std::nothrow) room;
  if (!roomPtr)
  {
    displayAllocError("room", "readRoomFromFile");

    exit(1);
  }

 // set everything in room record to 0/NULL

  memset(roomPtr, 0, sizeof(room));

 // get room vnum

  if (strn[0] != '#')
  {
    _outtext("Room line that should have '#' and vnum doesn't - string read was\n'");
    _outtext(strn);
    _outtext("'\n\nAborting.\n");

    exit(1);
  }

  deleteChar(strn, 0);

  const uint roomNumb = strtoul(strn, NULL, 10);

 // if incNumbRooms is TRUE, assume room is being read into main room list
 // and not a default room or whatever else

  if (!strnumer(strn) && incNumbRooms)
  {
    _outtext("Error: room has vnum of '");
    _outtext(strn);
    _outtext("' - only positive, numeric vnums are allowed.  Aborting.\n");

    exit(1);
  }

  if (checkDupes)
  {
    if (roomNumb == 0)
    {
      _outtext("Error - room in .wld file has an invalid vnum of 0.  Aborting.\n");

      exit(1);
    }

    if (roomExists(roomNumb))
    {
      char outstrn[512];

      sprintf(outstrn,
"Error: Room #%u has more than one entry in the .wld file.\n"
"       Aborting.\n",
              roomNumb);

      _outtext(outstrn);

      exit(1);
    }
  }

  if (roomNumb >= g_numbLookupEntries)
  {
    if (!changeMaxVnumAutoEcho(roomNumb + 1000))
      exit(1);
  }

  roomPtr->roomNumber = roomNumb;


 // Now, read the room name - this should be one line, but yet has a tilde
 // at the end anyway.  Go figure.

  if (!readAreaFileLine(worldFile, strn, MAX_ROOMNAME_LEN, ENTITY_ROOM, roomNumb, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "name", 0, NULL, true, false))
    exit(1);

  strcpy(roomPtr->roomName, strn);

 // read the room description

  roomPtr->roomDescHead = readStringNodes(worldFile, TILDE_LINE, true);

 // read miscellaneous room info

  const size_t intMiscArgs[] = { 3, 4, 0 };

  if (!readAreaFileLine(worldFile, strn, 512, ENTITY_ROOM, roomNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                        "misc info", 0, intMiscArgs, false, true))
    exit(1);

  if (numbArgs(strn) == 3)
  {
    sscanf(strn, "%u%u%u",
           &(roomPtr->zoneNumber), &(roomPtr->roomFlags),
           &(roomPtr->sectorType));
  }
  else
  {
    sscanf(strn, "%u%u%u%u",
           &(roomPtr->zoneNumber), &(roomPtr->roomFlags),
           &(roomPtr->sectorType), &(roomPtr->resourceInfo));

   // assume that 4-arg line means this is a map zone

    setVarBoolVal(&g_varHead, VAR_ISMAPZONE_NAME, true, true); 
  }

 // Now, check for either a D, E, or S - D is exit, E is extra description, S
 // signifies end of room data - keep checking for Ds and Es until we hit an
 // S of death

  while (true)
  {
    if (!readAreaFileLine(worldFile, strn, 512, ENTITY_ROOM, roomNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                          "D, E, S, F, M, or C", 0, NULL, false, true))
      exit(1);

    if (strcmpnocase(strn, "S"))  // end of room info
    {
      break;
    }
    else
    if (strcmpnocase(strn, "E"))  // extra description
    {
      readExtraDescFromFile(worldFile, ENTITY_ROOM, roomPtr->roomNumber, ENTITY_R_EDESC, 
                            &(roomPtr->extraDescHead));
    }
    else
    if (toupper(strn[0]) == 'D')      // exit
    {
      readRoomExitFromFile(worldFile, roomPtr, strn, incNumbRooms);
    }
    else
    if (strcmpnocase(strn, "F"))  // fall percentage
    {
      if (!readAreaFileLine(worldFile, strn, 512, ENTITY_ROOM, roomNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                            "fall info", 1, NULL, false, true))
        exit(1);

      sscanf(strn, "%u", &(roomPtr->fallChance));
    }
    else
    if (strcmpnocase(strn, "M"))  // mana apply stuff
    {
      if (!readAreaFileLine(worldFile, strn, 512, ENTITY_ROOM, roomNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                            "mana info", 2, NULL, false, true))
        exit(1);

      sscanf(strn, "%u%d", &(roomPtr->manaFlag), &(roomPtr->manaApply));
    }
    else
    if (strcmpnocase(strn, "R"))  // min level max level stuff - lose it forever
    {
      if (!readAreaFileLine(worldFile, strn, 512, ENTITY_ROOM, roomNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                            "level info", 2, NULL, false, true))
        exit(1);
    }
    else
    if (strcmpnocase(strn, "C"))  // current
    {
      if (!readAreaFileLine(worldFile, strn, 512, ENTITY_ROOM, roomNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                            "current info", 2, NULL, false, true))
        exit(1);

      sscanf(strn, "%u%u", &(roomPtr->current), &(roomPtr->currentDir));
    }
    else
    {
      char outstrn[512];

      _outtext("Unrecognized extra data - '");
      _outtext(strn);
      
      sprintf(outstrn, "' found in\n"
                       "data for room #%u.  Aborting.\n",
              roomPtr->roomNumber);

      _outtext(outstrn);

      exit(1);
    }
  }

  if (incNumbRooms)  // assume we're adding to global list
  {
    g_numbRooms++;

    g_roomLookup[roomNumb] = roomPtr;

    if (roomNumb > g_highestRoomNumber) 
      g_highestRoomNumber = roomNumb;

    if (roomNumb < g_lowestRoomNumber)  
      g_lowestRoomNumber = roomNumb;
  }

  return roomPtr;
}


//
// readWorldFile : Reads rooms from the world file
//

bool readWorldFile(const char *filename)
{
  FILE *worldFile;
  char worldFilename[512] = "";

  room *roomPtr;

  uint lastRoom = 0;


 // assemble the filename of the world file

  if (g_readFromSubdirs) 
    strcpy(worldFilename, "wld/");

  if (filename) 
    strcat(worldFilename, filename);
  else 
    strcat(worldFilename, getMainZoneNameStrn());

  strcat(worldFilename, ".wld");

 // open the world file for reading

  if ((worldFile = fopen(worldFilename, "rt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(worldFilename);
    _outtext(", skipping\n");

    return false;
  }

  _outtext("Reading ");
  _outtext(worldFilename);
  _outtext("...\n");

 // this while loop reads room by room, one room per iteration

  while (true)
  {
    roomPtr = readRoomFromFile(worldFile, true, true);
    if (!roomPtr) 
      break;  // eof

    if (roomPtr->roomNumber < lastRoom)
    {
      char outstrn[512];

      sprintf(outstrn,
"Warning: Room numbers out of order - #%u and #%u\n",
              lastRoom, roomPtr->roomNumber);

      displayAnyKeyPrompt(outstrn);

      g_madeChanges = true;
    }
    else 
    {
      lastRoom = roomPtr->roomNumber;
    }
  }

  fclose(worldFile);

  return true;
}
