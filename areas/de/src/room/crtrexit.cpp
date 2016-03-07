//
//  File: crtrexit.cpp   originally part of durisEdit
//
//  Usage: user-interface functions used to create exits in rooms
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

#include "types.h"
#include "fh.h"

#include "exit.h"
#include "room.h"

extern room *g_currentRoom;
extern bool g_madeChanges;
extern char *g_exitnames[];


//
// commonDisplayExit
//

char *commonDisplayExit(const char *chstr, const char *exitName, const uint flag, const bool negateFlag,
                        const bool comma, char *strn)
{
  if ((!negateFlag && !flag) || (negateFlag && flag))
    sprintf(strn, "&+c[&+C%s&+c]%s", chstr, exitName);
  else
    sprintf(strn, "&+L[%s]%s&+c", chstr, exitName);

  if (comma)
    strcat(strn, ", ");
  else
    strcat(strn, " ");

  return strn;
}


//
// buildCommonDisplayExitsStrn
//

char *buildCommonDisplayExitsStrn(char *strn, const uint exitTaken, const bool negateFlag)
{
  char workstrn[256];

  strcat(strn, commonDisplayExit("N", "orth", exitTaken & EXIT_NORTH_FLAG, negateFlag, true, workstrn));
  strcat(strn, commonDisplayExit("1", "Northwest", exitTaken & EXIT_NORTHWEST_FLAG, negateFlag, true, workstrn));
  strcat(strn, commonDisplayExit("2", "Northeast", exitTaken & EXIT_NORTHEAST_FLAG, negateFlag, true, workstrn));
  strcat(strn, commonDisplayExit("S", "outh", exitTaken & EXIT_SOUTH_FLAG, negateFlag, true, workstrn));
  strcat(strn, commonDisplayExit("3", "Southwest", exitTaken & EXIT_SOUTHWEST_FLAG, negateFlag, true, workstrn));
  strcat(strn, commonDisplayExit("4", "Southeast", exitTaken & EXIT_SOUTHEAST_FLAG, negateFlag, true, workstrn));
  strcat(strn, commonDisplayExit("W", "est", exitTaken & EXIT_WEST_FLAG, negateFlag, true, workstrn));
  strcat(strn, commonDisplayExit("E", "ast", exitTaken & EXIT_EAST_FLAG, negateFlag, true, workstrn));
  strcat(strn, commonDisplayExit("U", "p", exitTaken & EXIT_UP_FLAG, negateFlag, true, workstrn));
  strcat(strn, "or ");
  strcat(strn, commonDisplayExit("D", "own", exitTaken & EXIT_DOWN_FLAG, negateFlag, false, workstrn));

  return strn;
}


//
// createRoomExitPrompt : user interface for creating an exit in some
//                        user-selected direction - cannot create an exit
//                        where one already exists.  function returns
//                        the 'flag' value of exit created if successful,
//                        NO_EXIT otherwise
//
//        room : room to create exit in
//   exitTaken : bitvector created by getExitAvailFlags() that specifies
//               which exits exist
//

int createRoomExitPrompt(room *room, const uint exitTaken, const bool incLoaded)
{
  char strn[256];
  usint ch;


  strcpy(strn, "\n&+cCreate ");

  buildCommonDisplayExitsStrn(strn, exitTaken, false);

  strcat(strn, "&+c(&+CQ to quit&+c)? &n");

  displayColorString(strn);

  do
  {
    ch = toupper(getkey());
  } while (((ch != 'N') || ((exitTaken & EXIT_NORTH_FLAG))) &&
           ((ch != '1') || ((exitTaken & EXIT_NORTHWEST_FLAG))) &&
           ((ch != '2') || ((exitTaken & EXIT_NORTHEAST_FLAG))) &&
           ((ch != 'S') || ((exitTaken & EXIT_SOUTH_FLAG))) &&
           ((ch != '3') || ((exitTaken & EXIT_SOUTHWEST_FLAG))) &&
           ((ch != '4') || ((exitTaken & EXIT_SOUTHEAST_FLAG))) &&
           ((ch != 'W') || ((exitTaken & EXIT_WEST_FLAG))) &&
           ((ch != 'E') || ((exitTaken & EXIT_EAST_FLAG))) &&
           ((ch != 'U') || ((exitTaken & EXIT_UP_FLAG))) &&
           ((ch != 'D') || ((exitTaken & EXIT_DOWN_FLAG))) && (ch != 'Q'));

  switch (ch)
  {
    case 'N' : ch = NORTH;  break;
    case '1' : ch = NORTHWEST;  break;
    case '2' : ch = NORTHEAST;  break;
    case 'S' : ch = SOUTH;  break;
    case '3' : ch = SOUTHWEST;  break;
    case '4' : ch = SOUTHEAST;  break;
    case 'W' : ch = WEST;  break;
    case 'E' : ch = EAST;  break;
    case 'U' : ch = UP;  break;
    case 'D' : ch = DOWN;  break;

    default :
    case 'Q' : _outtext("quit\n\n");  return NO_EXIT;
  }

  _outtext(g_exitnames[ch]);

  createExit(&room->exits[ch], incLoaded);

  sprintf(strn, "\n%s exit created.\n\n", g_exitnames[ch]);
  strn[1] = toupper(strn[1]);

  _outtext(strn);

  return ch;
}


//
// createRoomExit : User interface to create an exit in the current room.
//                  Function only allows user to create an exit if the exit 
//                  "slot" is empty and returns NO_EXIT on failure for whatever 
//                  reason, or the exit direction if exit creation was successful
//
//  exitDir : direction to create exit in, or USER_CHOICE to prompt the user
//            (defined in room/exit.h)
//

int createRoomExit(const int exitdir)
{
  uint exitTaken;
  char strn[256];


  if (((exitdir != USER_CHOICE) && ((exitdir < 0) || (exitdir >= NUMB_EXITS))))
  {
    return NO_EXIT;
  }

 // if not user choice, try to create the exit in the direction specified

  if (exitdir != USER_CHOICE)
  {
    if (g_currentRoom->exits[exitdir])
    {
     // proper grammar!

      if ((exitdir == DOWN) || (exitdir == UP))
      {
        sprintf(strn, "\nExit already exists %s.\n\n",
                g_exitnames[exitdir]);
      }
      else
      {
        sprintf(strn, "\nExit already exists to the %s.\n\n",
                g_exitnames[exitdir]);
      }

      _outtext(strn);

      return NO_EXIT;
    }
    else
    {
      char strn2[64];

      createExit(&g_currentRoom->exits[exitdir], true);

      strcpy(strn2, g_exitnames[exitdir]);
      strn2[0] = toupper(strn2[0]);

      sprintf(strn, "\n%s exit created.\n\n", strn2);
      _outtext(strn);

      g_madeChanges = true;

      return exitdir;
    }
  }

  exitTaken = getExitTakenFlags(g_currentRoom);


 // all exits taken up

  if (exitTaken == EXIT_ALL_EXITS_FLAG)
  {
    _outtext("\nNo exit slots available - move or delete some exits.\n\n");

    return NO_EXIT;
  }

  return createRoomExitPrompt(g_currentRoom, exitTaken, true);
}


//
// preCreateExit : parses the args string and passes it to createRoomExit()
//                 if it specifies a valid direction - exit created in current
//                 room
//
//    args : user input
//

void preCreateExit(const char *args)
{
  char arg1[256], val;

  getArg(args, 1, arg1, 255);

  if (!strlen(arg1)) 
  {
    createRoomExit(USER_CHOICE);
  }
  else
  {
    val = getDirfromKeyword(arg1);
    if (val == NO_EXIT)
    {
      _outtext("\nCreate which exit?\n\n");
      return;
    }

    createRoomExit(val);
  }
}
