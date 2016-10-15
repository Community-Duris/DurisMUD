//
//  File: zone.cpp       originally part of durisEdit
//
//  Usage: functions used for various zone-related stuff
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

#include "../fh.h"
#include "../types.h"
#include "../de.h"

#include "zone.h"

extern zone g_zoneRec;
extern bool g_madeChanges;


//
// setZoneNumb
//

void setZoneNumb(const uint zoneNumb, const bool dispText)
{
  const uint high = getHighestRoomNumber();


  if (zoneNumb == 0)
  {
    if (dispText)
      _outtext("\nZone number unchanged.\n\n");

    return;
  }

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= high; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      if (roomPtr->zoneNumber != zoneNumb)
      {
        roomPtr->zoneNumber = zoneNumb;
        g_madeChanges = true;
      }
    }
  }

  if (g_zoneRec.zoneNumber != zoneNumb)
  {
    g_zoneRec.zoneNumber = zoneNumb;
    g_madeChanges = true;
  }

  if (dispText)
  {
    char strn[128];

    sprintf(strn, "\nZone and all rooms set to zone number %u.\n\n",
            zoneNumb);

    _outtext(strn);
  }

  g_madeChanges = true;
}


//
// setZoneNumbStrn : Sets zone number of all room recs and of zone rec.
//
//   *strn : user input
//

void setZoneNumbStrn(const char *args)
{
  if (!strlen(args))
  {
    setZoneNumbPrompt();
    return;
  }

  if (!strnumber(args))
  {
    _outtext("\nThe 'set zone number' command requires a numeric argument.\n\n");
    return;
  }

  const uint numb = strtoul(args, NULL, 10);

  setZoneNumb(numb, true);
}


//
// setZoneNumbPrompt : Prompts a user to type in a new zone number
//

void setZoneNumbPrompt(void)
{
  uint numb = g_zoneRec.zoneNumber;


  _outtext("\n");

  editUIntVal(&numb, false, "&+CEnter new zone number:&n ");

  setZoneNumb(numb, true);
}
