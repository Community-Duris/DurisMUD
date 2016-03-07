//
//  File: crtexit.cpp    originally part of durisEdit
//
//  Usage: functions used to create exit records
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

#include "types.h"
#include "fh.h"

#include "exit.h"
#include "misc/strnnode.h"



extern roomExit *g_defaultExit;
extern uint g_numbExits;
extern bool g_madeChanges;

//
// createExit : Function to create a new exit - returns true if successful
//
//   **exitNode : pointer to pointer to exit to create
//

bool createExit(roomExit **exitNode, const bool incNumbExits)
{
  roomExit *newExit;


 // create a new exit

  newExit = new(std::nothrow) roomExit;
  if (!newExit)
  {
    displayAllocError("roomExit", "createExit");

    return false;
  }

  if (g_defaultExit)
  {
    newExit = copyRoomExit(g_defaultExit, false);
  }
  else
  {
    memset(newExit, 0, sizeof(roomExit));
    newExit->keywordListHead = createKeywordList("~");

    newExit->destRoom = -1;
  }


  if (incNumbExits)
  {
    g_numbExits++;

    createPrompt();
  }

  *exitNode = newExit;

  g_madeChanges = true;

  return true;
}


//
// createExit : returns true if successful
//

bool createExit(const uint roomNumb, const int destNumb, const char exitDir)
{
  roomExit *exitRec;
  room *srcRoom = findRoom(roomNumb);


  createExit(&exitRec, true);
  if (!exitRec) 
    return false;

  exitRec->destRoom = destNumb;

  srcRoom->exits[exitDir] = exitRec;

  return true;
}
