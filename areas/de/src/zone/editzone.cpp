//
//  File: editzone.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing zone info
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

#include "../fh.h"
#include "../types.h"

#include "../graphcon.h"

#include "../misc/menu.h"

#include "zone.h"

extern bool g_madeChanges;
extern menu g_zoneMenu;

//
// displayEditZoneMenu : Displays edit zone menu
//
//   *zone : zone being edited
//

void displayEditZoneMenu(const zone *zonePtr)
{
  displayMenuHeader(ENTITY_ZONE, zonePtr->zoneName, zonePtr->zoneNumber, NULL, false);

  displayMenu(&g_zoneMenu, zonePtr);
}


//
// interpEditZoneMenu : Interprets user input for edit zone menu - returns
//                      TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//  *zone : zone to edit
//

bool interpEditZoneMenu(const usint ch, zone *zone)
{
 // edit zone top room numb, lifespan, reset mode

  if (ch == 'C')
  {
    editZoneMisc(zone);

    displayEditZoneMenu(zone);
  }
  else

 // menu

  if (interpretMenu(&g_zoneMenu, zone, ch))
  {
    displayEditZoneMenu(zone);

    return false;
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) return true;

  return false;
}


//
// editZone : "main" function for edit zone menu
//
//  *zone : zone being edited
//

void editZone(zone *zonePtr)
{
  usint ch;
  zone newZone;


  if (!zonePtr) 
    return;

 // copy zone into newZone and display the menu

  memcpy(&newZone, zonePtr, sizeof(zone));

  displayEditZoneMenu(&newZone);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      _outtext("\n\n");

      return;
    }

   // if interpEditZoneMenu is TRUE, user has selected to save changes

    if (interpEditZoneMenu(ch, &newZone))
    {
     // technically, zone name could have changed without any actual noticeable change to the user,
     // but -- I don't care!  feel free to fix it if you do

      if (memcmp(zonePtr, &newZone, sizeof(zone)))   // changes have been made
      {
        memcpy(zonePtr, &newZone, sizeof(zone));

        setZoneNumb(zonePtr->zoneNumber, false);

        g_madeChanges = true;
      }

      _outtext("\n\n");

      return;
    }
  }
}
