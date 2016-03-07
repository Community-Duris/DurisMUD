//
//  File: roomu.cpp      originally part of durisEdit
//
//  Usage: various user-interface functions relating to commands
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

#include "fh.h"
#include "types.h"
#include "de.h"

#include "room.h"
#include "roomu.h"

#include "keys.h"

extern room *g_currentRoom;
extern uint g_numbExits, g_numbRooms, g_numbLookupEntries;
extern bool g_madeChanges;
extern char *g_exitnames[];
extern char g_revdirs[];

//
// gotoRoomStrn : Goes to a room based on user input in *args
//
//   *args : user input
//

void gotoRoomStrn(const char *origargs)
{
  char outstrn[768], args[512];
  const bool numeric = strnumer(args);


  if (!strlen(origargs))
  {
    gotoRoomPrompt();
    return;
  }

  strncpy(args, origargs, 511);
  args[511] = '\0';

 // try to find a match

  room *roomPtr = getRoomKeyword(args, true);

 // uh-oh, no match

  if (!roomPtr)
  {
    if (numeric)
    {
      sprintf(outstrn, "\nRoom #%s not found.\n\n", args);
    }
    else
    {
      sprintf(outstrn, "\n"
"No objs or mobs contain that keyword (%s), and no room titles\n"
"contain that string.\n\n",
              args);
    }

    _outtext(outstrn);

    return;
  }
  else
  {
    g_currentRoom = roomPtr;

    displayCurrentRoom();
  }
}


//
// gotoRoomPrompt : Prompts a user to type in a room number to jump to.
//

void gotoRoomPrompt(void)
{
  uint vnum;
  char strn[11];
  room *roomPtr;


  _outtext("\n");

  sprintf(strn, "%u", g_currentRoom->roomNumber);

  editStrnValSearchableList(strn, 10, "room vnum to go to", displayRoomList);

  vnum = strtoul(strn, NULL, 10);

  roomPtr = findRoom(vnum);

  if (!roomPtr)
  {
    if (strlen(strn))
    {
      char outstrn[256];

      sprintf(outstrn, "\nRoom #%u not found.\n\n", vnum);

      _outtext(outstrn);
    }
    else
    {
      _outtext("\n\n");
    }
  }
  else
  {
    g_currentRoom = roomPtr;

    _outtext("\n");

    displayCurrentRoom();
  }
}


//
// createGrid : given x, y, and z sizes, creates a grid of rooms connected by
//              north, south, west, and east exits - returns TRUE if
//              successful, FALSE otherwise
//
//         sizex : number of columns in grid
//         sizey : number of rows in grid
//         sizez : number of levels in grid
//  exitCreation : exit creation mode (diagonal, standard)
//

bool createGrid(const uint sizex, const uint sizey, const uint sizez, const char exitCreation)
{
  uint arrPos;
  uint x, y, z;
  uint *roomArr, *roomArrPtr;
  const uint startExit = g_numbExits, startRoom = g_numbRooms;
  room *roomRec;
  char strn[256];


  const uint gridsize = sizex * sizey * sizez;

  if (getNumbFreeRooms() < gridsize)
  {
    if (!changeMaxVnumAutoEcho(g_numbLookupEntries + gridsize + 999))
      return false;
  }

  roomArr = new(std::nothrow) uint[gridsize];
  if (!roomArr)
  {
    displayAllocError("uint[]", "createGrid");

    return false;
  }

 // create rooms for grid

 // start at end of room array so that upper-left-most corner is lowest vnum

  roomArrPtr = roomArr + gridsize - 1;

  while (roomArrPtr >= roomArr)
  {
    roomRec = createRoom(true, 0, true);

   // on failed room alloc, clean up everything and return false

    if (!roomRec)
    {
      roomArrPtr++;

      _outtext("\nOut of memory - aborting grid creation.\n\n");

      while (roomArrPtr < roomArr + gridsize)
      {
        deleteRoomInfo(findRoom(*roomArrPtr), true, true);

        roomArrPtr++;
      }

      delete[] roomArr;

      return false;
    }

    *roomArrPtr = roomRec->roomNumber;
    roomArrPtr--;
  }

 // link rooms

  arrPos = 0;

  for (z = 0; z < sizez; z++)
  {
    for (y = 0; y < sizey; y++)
    {
      for (x = 0; x < sizex; x++)
      {
        const uint roomNumb = roomArr[arrPos];

        if (z != 0) createExit(roomNumb, roomArr[arrPos - (sizex * sizey)], DOWN);
        if (z != (sizez - 1))
                    createExit(roomNumb, roomArr[arrPos + (sizex * sizey)], UP);

        if ((exitCreation == GRID_STANDARD) || (exitCreation == GRID_BOTH))
        {
          if (y != 0) createExit(roomNumb, roomArr[arrPos - sizex], SOUTH);
          if (y != (sizey - 1))
                      createExit(roomNumb, roomArr[arrPos + sizex], NORTH);
          if (x != 0) createExit(roomNumb, roomArr[arrPos - 1], EAST);
          if (x != (sizex - 1))
                      createExit(roomNumb, roomArr[arrPos + 1], WEST);
        }

        if ((exitCreation == GRID_DIAGONAL) || (exitCreation == GRID_BOTH))
        {
          if ((x != (sizex - 1)) && (y != (sizey - 1)))
            createExit(roomNumb, roomArr[arrPos + sizex + 1], NORTHWEST);
          if ((x != 0) && (y != (sizey - 1)))
            createExit(roomNumb, roomArr[(arrPos + sizex) - 1], NORTHEAST);
          if ((x != (sizex - 1)) && (y != 0))
            createExit(roomNumb, roomArr[(arrPos - sizex) + 1], SOUTHWEST);
          if ((x != 0) && (y != 0))
            createExit(roomNumb, roomArr[arrPos - sizex - 1], SOUTHEAST);
        }

        arrPos++;
      }
    }
  }

  sprintf(strn, "\n"
"%ux%ux%u grid created, starting at room #%u (upper northwest corner)\n"
"and ending at room #%u (lower southeast corner) (%u exits, %u rooms\n"
"created).\n\n",
          sizex, sizey, sizez, roomArr[gridsize - 1], roomArr[0],
          g_numbExits - startExit, g_numbRooms - startRoom);

  _outtext(strn);

  g_madeChanges = true;

  delete[] roomArr;

  return true;
}


//
// createGridInterp : interprets user input, and if valid, passes it to
//                    createGrid()
//
//    args : user arguments to command
//

void createGridInterp(const char *args)
{
  uint x, y, z;
  char arg1[256], arg2[256], arg3[256];
  usint ch;


  if (!strlen(args))
  {
    _outtext("\n"
"Specify an x, y, and z size (optional) for the grid as the first, second, and\n"
"and third arguments.\n\n");

    return;
  }

  getArg(args, 1, arg1, 255);
  getArg(args, 2, arg2, 255);
  getArg(args, 3, arg3, 255);

  if (arg3[0] == '\0')  // z-size - assume 1
  {
    strcpy(arg3, "1");
  }

  if (arg2[0] == '\0')  // no second arg - DOH!
  {
    _outtext("\nSpecify a y-size as the second argument.\n\n");

    return;
  }

  if (!strnumer(arg1) || !strnumer(arg2) || !strnumer(arg3))
  {
    _outtext("\n"
"The first, second, and third arguments (third argument optional) should specify\n"
"the x, y, and z sizes of the grid.\n\n");

    return;
  }

  x = strtoul(arg1, NULL, 10);
  y = strtoul(arg2, NULL, 10);
  z = strtoul(arg3, NULL, 10);

  if ((x == 0) || (y == 0) || (z == 0))
  {
    _outtext("\n"
"The x-size, y-size, or z-size of grid is 0.\n\n");

    return;
  }

  displayColorString("\n"
"&+cLink the rooms with &+Cs&+ctandard compass directions, &+Cd&+ciagonal directions, or\n"
"both (&+Cs/d/B&+c)? &n");

  do
  {
    ch = toupper(getkey());
  } while ((ch != K_Enter) && (ch != 'S') && (ch != 'D') && (ch != 'B'));

  if ((ch == K_Enter) || (ch == 'B'))
  {
    _outtext("both\n");
    createGrid(x, y, z, GRID_BOTH);
  }
  else if (ch == 'D')
  {
    _outtext("diagonal\n");
    createGrid(x, y, z, GRID_DIAGONAL);
  }
  else
  {
    _outtext("standard\n");
    createGrid(x, y, z, GRID_STANDARD);
  }
}


//
// linkRooms : assumes from room and to room exist and dir is valid - creates
//             exits between the two rooms specified, leading to each other -
//             returns TRUE if successful, FALSE is failed (can only fail if
//             user aborts)
//
//     fromnumb : source room
//       tonumb : destination room
//    direction : direction of exit from source room
//

bool linkRooms(const int fromnumb, const int tonumb, const char direction)
{
  room *fromr = findRoom(fromnumb), *tor = findRoom(tonumb);
  char rev = g_revdirs[direction], strn[512];
  bool addedboth = true;


  _outtext("\n");

  if (fromr->exits[direction])
  {
    if (displayYesNoPrompt("&+cSource room already has an exit in that direction - continue", promptNo, 
                           false) == promptNo)
    {
      return false;
    }

    addedboth = false;
  }

  if (tor->exits[rev])
  {
    if (displayYesNoPrompt("&+cDestination room already has an exit in the opposite direction -\n"
                           "continue", promptNo, false) == promptNo)
    {
      return false;
    }

    addedboth = false;
  }

  if (!fromr->exits[direction])
  {
    createExit(fromnumb, tonumb, direction);
  }
  else
  {
    fromr->exits[direction]->destRoom = tonumb;
  }

  if (!tor->exits[rev])
  {
    createExit(tonumb, fromnumb, rev);
  }
  else
  {
    tor->exits[rev]->destRoom = fromnumb;
  }

  sprintf(strn, "Exit%s created between rooms %u and %u (%s-%s).\n\n",
          addedboth ? "s" : "", fromnumb, tonumb,
          g_exitnames[direction], g_exitnames[rev]);
  _outtext(strn);

  return true;
}


//
// linkRoomsInterp : interprets user input, massages it, checks it, and passes
//                   it to linkRooms if it's valid
//
//      args : user arguments to command
//

void linkRoomsInterp(const char *args)
{
  char arg1[512], arg2[512], arg3[512], dir;
  bool isnum1, isnum2 = true;
  uint room1, room2;


 // common values

  getArg(args, 1, arg1, 511);
  getArg(args, 2, arg2, 511);

  isnum1 = strnumer(arg1);

 // specific to number of args

  if (numbArgs(args) == 3)
  {
    getArg(args, 3, arg3, 511);

    room1 = strtoul(arg1, NULL, 10);
    room2 = strtoul(arg2, NULL, 10);
    dir = getDirfromKeyword(arg3);

    isnum2 = strnumer(arg2);
  }
  else 
  if (numbArgs(args) == 2)
  {
    room1 = g_currentRoom->roomNumber;
    room2 = strtoul(arg1, NULL, 10);
    dir = getDirfromKeyword(arg2);
  }
  else  // in order to prevent redundancy, just set some variable to invalid
  {
    isnum1 = false;
  }

  if (!isnum1 || !roomExists(room1) || !isnum2 || !roomExists(room2) || (dir == NO_EXIT))
  {
    _outtext("\n"
"The 'link' command can take two or three arguments: either 'link <fromroom>\n"
"<toroom> <direction>' or 'link <toroom> <direction>' (current room is assumed\n"
"as source room in latter case).\n\n");

    return;
  }

  linkRooms(room1, room2, dir);
}


//
// renumberRoomsUser
//

void renumberRoomsUser(const char *args)
{
  char strn[256];
  uint usernumb, tempstart;


  if (strlen(args) == 0)
  {
    renumberRooms(false, 0);

    sprintf(strn, "\nRooms have been renumbered (starting at %u).\n\n",
            getLowestRoomNumber());
    _outtext(strn);

    return;
  }

  if (!strnumer(args))
  {
    _outtext(
"\nThe 'renumberroom' command's first argument must be a positive number.\n\n");
    return;
  }

  usernumb = strtoul(args, NULL, 10);

  if (usernumb == 0)
  {
    _outtext(
"\nYou cannot renumber starting from 0.\n\n");
    return;
  }

  tempstart = getHighestRoomNumber() + g_numbRooms;

  if (((usernumb + g_numbRooms) >= g_numbLookupEntries) || ((tempstart + g_numbRooms) >= g_numbLookupEntries))
  {
    uint newLimit;

    if (usernumb > tempstart)
      newLimit = usernumb;
    else
      newLimit = tempstart;

    if (!changeMaxVnumAutoEcho(newLimit + g_numbRooms + 1000))
      return;
  }

  renumberRooms(true, tempstart);

  renumberRooms(true, usernumb);

  sprintf(strn,
"\nRooms renumbered starting at %u.\n\n", usernumb);

  _outtext(strn);
}
