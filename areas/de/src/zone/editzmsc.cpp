//
//  File: editzmsc.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing misc. zone info
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fh.h"
#include "types.h"
#include "keys.h"

#include "graphcon.h"

#include "misc/menu.h"
#include "zone.h"

extern menu g_zoneMiscMenu;


//
// displayEditZoneMiscMenu : Displays the edit zone miscellany menu
//
//    *zone : zone being edited
//

void displayEditZoneMiscMenu(const zone *zone)
{
  displayMenuHeader(ENTITY_ZONE, zone->zoneName, zone->zoneNumber, "miscellany", false);

  displayMenu(&g_zoneMiscMenu, zone);
}


//
// interpEditZoneMiscMenu : Interprets user input for edit zone misc menu -
//                          returns TRUE if the user hit 'Q'
//
//      ch : user input
//   *zone : zone to edit
//

bool interpEditZoneMiscMenu(const usint ch, zone *zone)
{
  if (interpretMenu(&g_zoneMiscMenu, zone, ch))
  {
    displayEditZoneMiscMenu(zone);

    return false;
  }
  else

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) return true;

  return false;
}


//
// editZoneMisc : Edit zone misc - "main" function
//
//   *zone : zone to edit
//

void editZoneMisc(zone *zone)
{
  usint ch;

  uint oldDiff = zone->zoneDiff, oldLowLife = zone->lifeLow, oldHighLife = zone->lifeHigh, 
       oldResetMode = zone->resetMode;


  displayEditZoneMiscMenu(zone);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      zone->zoneDiff = oldDiff;
      zone->lifeLow = oldLowLife;
      zone->lifeHigh = oldHighLife;
      zone->resetMode = oldResetMode;

      _outtext("\n\n");

      return;
    }

    if (interpEditZoneMiscMenu(ch, zone)) 
      return;
  }
}
