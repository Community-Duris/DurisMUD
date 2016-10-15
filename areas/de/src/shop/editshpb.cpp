//
//  File: editshpb.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing shop boolean values
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
#include <ctype.h>

#include "../types.h"
#include "../fh.h"
#include "../misc/menu.h"

#include "../graphcon.h"

#include "shop.h"

extern menu g_shopBooleansMenu;


//
// displayEditShopBooleansMenu : Displays edit shop booleans menu
//

void displayEditShopBooleansMenu(const shop *shp, const mobType *mob)
{
  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "shop toggles", false);

  displayMenu(&g_shopBooleansMenu, shp);
}


//
// interpEditShopBooleansMenu :
//                      Interprets user input for edit shop booleans menu - returns
//                      TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//   *shp : shop to edit
//

bool interpEditShopBooleansMenu(const usint ch, shop *shp, mobType *mob)
{
  if (interpretMenu(&g_shopBooleansMenu, shp, ch))
  {
    displayEditShopBooleansMenu(shp, mob);

    return false;
  }

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editShopBooleans : edit shop booleans
//

void editShopBooleans(shop *shp, mobType *mob)
{
  usint ch;
  bool oldRoam = shp->roaming, oldMagic = shp->noMagic,
       oldKill = shp->killable;


 // display the menu

  displayEditShopBooleansMenu(shp, mob);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      shp->roaming = oldRoam;
      shp->noMagic = oldMagic;
      shp->killable = oldKill;

      return;
    }

   // if interpEditShopBooleansMenu is TRUE, user has exited menu

    if (interpEditShopBooleansMenu(ch, shp, mob)) 
      return;
  }
}
