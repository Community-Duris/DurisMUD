//
//  File: delmobhu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for deleting mobHeres
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
#include <string.h>
#include <stdlib.h>

#include "fh.h"
#include "types.h"

#include "room/room.h"

extern bool g_madeChanges;
extern room *g_currentRoom;


//
// deleteMobHereUser : Deletes a mobHere from current room based on info specified by user in args
//
//   args : user-entered string
//

void deleteMobHereUser(const char *args)
{
  char outStrn[512];
  mobHere *mobHere = g_currentRoom->mobHead;
  uint numb;


  if (!mobHere)
  {
    _outtext("\nThere are no mobs in this room.\n\n");
    return;
  }

  if (!strnumer(args))
  {
    _outtext("\nError in input - specify vnum of mob in this room.\n\n");
    return;
  }

  numb = strtoul(args, NULL, 10);

  while (mobHere && (mobHere->mobNumb != numb))
    mobHere = mobHere->Next;

  if (!mobHere)
  {
    sprintf(outStrn, "\nMob #%u not found in this room.\n\n", numb);
    _outtext(outStrn);

    return;
  }

  sprintf(outStrn, "\nMob #%u '%s&n' deleted from this room.\n\n", 
          numb, getMobShortName(mobHere->mobPtr));
  displayColorString(outStrn);

  deleteMobHereinList(&g_currentRoom->mobHead, mobHere, true);

  deleteMasterKeywordList(g_currentRoom->masterListHead);
  g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

  deleteEditableList(g_currentRoom->editableListHead);
  g_currentRoom->editableListHead = createEditableList(g_currentRoom);


  g_madeChanges = true;
}
