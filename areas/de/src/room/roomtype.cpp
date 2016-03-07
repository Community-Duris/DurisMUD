//
//  File: roomtype.cpp   originally part of durisEdit
//
//  Usage: functions that return strings based on various enumerated
//         bits of roominess
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

#include "fh.h"
#include "types.h"

#include "room.h"

extern flagDef g_roomSectList[];


//
// getRoomSectorStrn : strn should be at least 256 bytes
//

char *getRoomSectorStrn(const uint sectorType, const bool addSpaces, const bool addBrackets, char *strn)
{
  bool foundName = false;
  flagDef* roomSectPtr = g_roomSectList;


 // see if sector type is in g_roomSectList table (contains sector names)

  while (roomSectPtr->flagShort)
  {
    if (sectorType == roomSectPtr->defVal)
    {
      foundName = true;
      break;
    }

    roomSectPtr++;
  }

 // build sector string

  strn[0] = '\0';

  if (addBrackets)
    strcat(strn, "[");

  if (foundName)
  {
    strncat(strn, roomSectPtr->flagLong, 127);
    strn[128] = '\0';
  }
  else
  {
    sprintf(strn + strlen(strn), "&+RUnrecognized - %u", sectorType);
  }

  strcat(strn, "&n");

  if (addBrackets)
    strcat(strn, "]");

  if (addSpaces)
    strcat(strn, "  ");

  return strn;
}
