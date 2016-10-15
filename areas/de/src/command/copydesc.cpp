//
//  File: copydesc.cpp   originally part of durisEdit
//
//  Usage: functions for copying room/mob descriptions
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


#include "../types.h"
#include "../fh.h"

#include "../room/room.h"
#include "../mob/mob.h"

extern bool g_madeChanges;

//
// copyRoomDesc : copies a room desc from one room to another
//
//   tocopy : vnum of room to copy from
//   copyto : vnum of room to copy to
//

void copyRoomDesc(const uint tocopy, const uint copyto)
{
  const room *copyFrom;
  room *copyTo;
  char strn[128];


  copyFrom = findRoom(tocopy);
  copyTo = findRoom(copyto);

  if (copyFrom == copyTo)
  {
    _outtext("\n"
"Copying to and from the same room?\n\n");

    return;
  }

  if (!copyFrom)
  {
    _outtext("\nThe room vnum specified in the third argument does not exist.\n\n");
    return;
  }

  if (!copyTo)
  {
    _outtext("\nThe room vnum specified in the fourth argument does not exist.\n\n");
    return;
  }

  deleteStringNodes(copyTo->roomDescHead);
  copyTo->roomDescHead = copyStringNodes(copyFrom->roomDescHead);

  sprintf(strn, "\nRoom desc of room #%u copied to room #%u.\n\n",
          tocopy, copyto);

  _outtext(strn);

  g_madeChanges = true;
}


//
// copyMobDesc : copies a mob desc from one mob to another
//
//   tocopy : the mob to copy from
//   copyto : the mob to copy to
//

void copyMobDesc(const uint tocopy, const uint copyto)
{
  const mobType *copyFrom;
  mobType *copyTo;
  char strn[128];


  copyFrom = findMob(tocopy);
  copyTo = findMob(copyto);

  if (copyFrom == copyTo)
  {
    _outtext("\n"
"Copying to and from the same mob?\n\n");

    return;
  }

  if (!copyFrom)
  {
    _outtext("\nThe mob type vnum specified in the third argument does not exist.\n\n");
    return;
  }

  if (!copyTo)
  {
    _outtext("\nThe mob type vnum specified in the fourth argument does not exist.\n\n");
    return;
  }

  deleteStringNodes(copyTo->mobDescHead);
  copyTo->mobDescHead = copyStringNodes(copyFrom->mobDescHead);

  sprintf(strn, "\nMob desc of mob type #%u copied to mob type #%u.\n\n",
          tocopy, copyto);

  _outtext(strn);

  g_madeChanges = true;
}
