//
//  File: disproom.cpp   originally part of durisEdit
//
//  Usage: functions that display information about rooms
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

#include "../types.h"
#include "../fh.h"

#include "room.h"



extern room *g_currentRoom;
extern "C" flagDef room_bits[];
extern flagDef g_roomManaList[];
extern const char *g_exitnames[];


//
// displayRoomList : Displays the list of rooms, matching strn as appropriate
//

void displayRoomList(const char *strn)
{
  const room *room;
  size_t lines = 1;
  uint cur = getLowestRoomNumber();
  const uint high = getHighestRoomNumber();
  uint numb;
  char outStrn[MAX_ROOMNAME_LEN + 128];
  bool foundRoom = false, vnum, listAll = false;


  _outtext("\n\n");

  if (!strn || (strn[0] == '\0'))
  {
    listAll = true;
  }
  else
  {
    vnum = strnumber(strn);
    numb = strtoul(strn, NULL, 10);
  }

  while (cur <= high)
  {
    if (roomExists(cur))
    {
      char tempStrn[MAX_ROOMNAME_LEN + 1];

      room = findRoom(cur);

      if (!listAll)
      {
        strcpy(tempStrn, room->roomName);
        remColorCodes(tempStrn);
      }

      if (listAll || (vnum && (numb == room->roomNumber)) || strstrnocase(tempStrn, strn))
      {
        sprintf(outStrn, "%s&n (#%u)\n", room->roomName, room->roomNumber);

        foundRoom = true;

        if (checkPause(outStrn, lines))
          return;
      }
    }

    cur++;
  }

  if (!foundRoom) 
    _outtext("No matching rooms found.\n");

  _outtext("\n");
}


//
// displayRoomFlagsandSector : Displays the room flags and sector info for the current room
//

bool displayRoomFlagsandSector(size_t &numbLines)
{
  const uint roomBits = g_currentRoom->roomFlags;
  char outstrn[4096];


 // first, get the room sector info

  getRoomSectorStrn(g_currentRoom->sectorType, true, true, outstrn);


 // get room flags

  char flagStrn[1024];

 // color codes for each flag

  const char* roomFlagColor[32] =
  {
    "&+L",
    "&+R",
    "&+r",
    "&+c",
    "&+L",
    "&+b",
    "&+g",
    "&+g",
    "&+y",  // tunnel
    "&+L",
    "&+r",
    "&+W",
    "&+w",
    "&+g",
    "&+c",
    "&+g",
    "&+R",  // private zone
    "&+W",
    "&+L",
    "&+y",
    "&+y",
    "&+L",
    "&+W",  // magical light
    "&+g",
    "&+C",
    "&+c",  // twilight
    "&+g",
    "&+g",
    "&+C",
    "&+g",
    "&+L",
    "&+R"
  };

  for (uint i = 0; i < 32; i++) 
  {
    if (roomBits & (1 << i)) 
    {
      strncpy(flagStrn, room_bits[i].flagShort, 1023);
      flagStrn[1023] = '\0';

     // uppercase first char, replace underscores with spaces

      lowstrn(flagStrn);
      flagStrn[0] = toupper(flagStrn[0]);

      char *flagPtr = flagStrn;

      while (*flagPtr != '\0')
      {
        if (*flagPtr == '_')
          *flagPtr = ' ';

        flagPtr++;
      }

      sprintf(outstrn + strlen(outstrn), "%s(%s)  ", roomFlagColor[i], flagStrn);
    }
  }

  strcat(outstrn, "&n\n");

  return checkPause(outstrn, numbLines);
}


//
// displayExitList : Displays every exit in every room
//

void displayExitList(const char *strn)
{
  const room *room;
  const roomExit *ex;
  uint j, numb, cur = getLowestRoomNumber();
  const uint high = getHighestRoomNumber();
  size_t lines = 1;
  char outStrn[512];
  bool foundExit = false, vnum;


  if (strlen(strn) == 0)
  {
    vnum = false;
  }
  else
  {
    if (strnumber(strn))
    {
      numb = strtoul(strn, NULL, 10);
      vnum = true;
    }
    else
    {
      _outtext("\nSpecify a numeric argument (or no argument).\n\n");
      return;
    }
  }

  _outtext("\n\n");

  while (cur <= high)
  {
    if (roomExists(cur))
    {
      room = findRoom(cur);

      for (j = 0; j < NUMB_EXITS; j++)
      {
        if (!room->exits[j]) 
          continue;

        ex = room->exits[j];

        if (!vnum || (vnum && ((numb == ex->destRoom) || (numb == room->roomNumber))))
        {
          sprintf(outStrn, "  &+croom #&+g%u &+L(&n%s&+L) &+R%s&n to &+Croom &+G#%d&n\n",
                  room->roomNumber, room->roomName, getExitStrn(j), ex->destRoom);

          foundExit = true;

          if (checkPause(outStrn, lines))
            return;
        }
      }
    }

    cur++;
  }

  if (!foundExit) 
    _outtext("No matching exits found.\n");

  _outtext("\n");
}


//
// getDoorStateStrn : Displays the "door state" for an exit - each applicable
//                    flag has a letter, you see
//
//   *exitNode : pointer to exit to display info on
//       *strn : string into which to do things
//

char *getDoorStateStrn(const roomExit *exitNode, char *strn)
{
  int doorState, doorType;


  doorType = exitNode->worldDoorType;
  doorState = exitNode->zoneDoorState;

  strn[0] = '\0';

  if (doorState || doorType)
  {
    strcat(strn, "&n[");

   // world flags

    if ((doorType & 3) == 3) 
      strcat(strn, "&+RU");
    else 
    if (doorType & 1) 
      strcat(strn, "&+cC");
    else 
    if (doorType & 2) 
      strcat(strn, "&+CL");

    if (doorType & 4) 
      strcat(strn, "&+LS");
    if (doorType & 8) 
      strcat(strn, "&+rB");

    strcat(strn, "&n|");

   // zone flags

    if (doorState & 1) 
      strcat(strn, "&+cC");
    else 
    if (doorState & 2) 
      strcat(strn, "&+CL");

  // only show 'door open' state if exit has a door and is not blocked

    else 
    if ((~doorState & 8) && (doorType & 3)) 
      strcat(strn, "&+WO");

    if (doorState & 4) 
      strcat(strn, "&+LS");
    if (doorState & 8) 
      strcat(strn, "&+rB");

    strcat(strn, "&n] ");
  }

  return strn;
}


//
// getExitDestStrn : Displays the exit dest for exit
//
//   *exitNode : pointer to exit to display info on
//       *strn : string into which to get
//

char *getExitDestStrn(const roomExit *exitNode, char *strn)
{
  sprintf(strn, "&n(&+Y%d&n) ", exitNode->destRoom);

  return strn;
}


//
// displayRoomInfo : information displayed by stat command
//

void displayRoomInfo(const char *args)
{
  char strn[4096], strn2[4096];
  char fstrn[2048];
  room *room;


  if (!strlen(args)) 
  {
    room = g_currentRoom;
  }
  else
  {
    if (!strnumber(args))
    {
      _outtext("\nError in input - specify a valid room vnum.\n\n");

      return;
    }

    room = findRoom(strtoul(args, NULL, 10));

    if (!room)
    {
      _outtext("\nNo room exists with that vnum.\n\n");

      return;
    }
  }

  sprintf(strn,
"\n"
"&+YVnum        :&n %u\n"
"&+YName        :&n %s&n\n"
"&+YExtra descs :&n ",
room->roomNumber, room->roomName);

  displayColorString(strn);

  displayExtraDescNodes(room->extraDescHead);

  sprintf(strn,
"\n"
"\n"
"&+YSector type :&n %u (%s)\n"
"&+YRoom flags  :&n %u (%s)\n"
"&+YZone number :&n %u\n"
"\n"
"&+YFall %%age    :&n %u\n"
"&+YCurrent %%age :&n %u\n"
"&+YCurrent dir  :&n %u (%s)\n"
"\n"
"&+YMana type  :&n %u (%s)\n"
"&+YMana apply :&n %d\n"
"\n",

room->sectorType, getRoomSectorStrn(room->sectorType, false, false, strn2),
room->roomFlags,
  getFlagStrn(room->roomFlags, room_bits, fstrn, 1023),
room->zoneNumber,

room->fallChance,
room->current,
room->currentDir,
 ((room->currentDir >= EXIT_LOWEST) && (room->currentDir <= EXIT_HIGHEST)) ?
  g_exitnames[room->currentDir] : "invalid dir",

room->manaFlag, getFlagNameFromList(g_roomManaList, room->manaFlag),
room->manaApply);

  displayColorString(strn);
}


//
// displayRoomExitInfo : similar to "exits" command on Duris
//

void displayRoomExitInfo(void)
{
  char strn[512], direction[128], roomName[MAX_ROOMNAME_LEN + 1];
  bool foundExit = false;
  size_t numbLines = 0;


  if (checkPause("\n&+gExits:&n\n", numbLines))
    return;

  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    if (g_currentRoom->exits[i])
    {
      const roomExit *exitNode = g_currentRoom->exits[i];
      const room *roomDest = findRoom(exitNode->destRoom);

      if (roomDest)
      {
        strcpy(roomName, roomDest->roomName);
      }
      else
      {
        if (exitNode->destRoom != -1) 
          strcpy(roomName, "Room info not in this .wld file");
        else 
          strcpy(roomName, "Exit leads nowhere");
      }

      sprintf(direction, "%-10s", g_exitnames[i]);
      direction[0] = toupper(direction[0]);

      sprintf(strn, "&+c%s&n - %s&+c (#%d)&n\n", direction, roomName, exitNode->destRoom);

      if (checkPause(strn, numbLines))
        return;

      foundExit = true;
    }
  }

  if (!foundExit)
    displayColorString("&+RNone!&n\n");

  _outtext("\n");
}
