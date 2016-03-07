//
//  File: editshps.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing shop 'sold' list
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

#include "types.h"
#include "fh.h"

#include "graphcon.h"

#include "editshp.h"
#include "misc/menu.h"
#include "keys.h"

extern menu g_shopItemMenu;


//
// displayEditShopSoldMenu : Displays edit shop sold menu
//
//   *shp : shop being edited
//   *mob : mob with shop
//

void displayEditShopSoldMenu(const shop *shp, const mobType *mob)
{
  char strn[1024], ch;
  uint numbShown = 0, numbItems = getNumbShopSold(shp->producedItemList), i;


  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "shop sold list", false);

  if (numbItems == 0)
  {
    displayColorString("&+w  This shop is not selling anything.\n");
  }

  for (i = 0; i < numbItems; i++)
  {
   // if either one goes over 127 we're screwed anyway

    if ((numbShown + 'A') > LAST_SHOP_CH)
      ch = (char)(FIRST_SHOP_POSTALPHA_CH + (numbShown - (LAST_SHOP_CH - 'A')) - 1);
    else 
      ch = (char)(numbShown + 'A');

    sprintf(strn, "   &+Y%c&+L. &+w%s&n (#%u)\n",
            ch, getObjShortName(findObj(shp->producedItemList[i])), shp->producedItemList[i]);

    displayColorString(strn);

    numbShown++;

    if ((numbShown % MAX_SHOP_ENTRIES) == 0)
    {
      displayColorString("\n&+CPress a key to continue..");
      getkey();
      _outtext("\n\n");
    }
  }

  displayMenu(&g_shopItemMenu, shp);
}


//
// interpEditShopSoldMenu : Interprets user input for edit shop sold menu - returns
//                          TRUE if the user hits 'Q', FALSE otherwise
//
//    ch : user input
//  *shp : shop to edit
//

bool interpEditShopSoldMenu(const usint origch, shop *shp, mobType *mob)
{
  const uint numbItems = getNumbShopSold(shp->producedItemList);
  uint i;


 // delete an entry (or all of them)

  if ((origch == 'Y') && numbItems)
  {
    usint ch;

    const struct rccoord coords = _gettextposition();

    _settextposition(coords.row, 1);
    clrline(coords.row);

    displayColorString(
"&+CEnter letter of item to delete (Y to delete all items): ");

    do
    {
      ch = toupper(getkey());

      if (ch == 'Y')
      {
        shp->producedItemList[0] = 0;

        displayEditShopSoldMenu(shp, mob);
        return false;
      }

      if ((ch == 'X') || (ch == K_Escape) || (ch == K_Enter))
      {
        displayEditShopSoldMenu(shp, mob);
        return false;
      }

      if ((ch >= FIRST_SHOP_POSTALPHA_CH) && (ch < 'A'))
         ch += (LAST_SHOP_CH - FIRST_SHOP_POSTALPHA_CH) + 1;  // it's magic
    } while (!(numbItems > (uint)(ch - 'A')));

    if (numbItems > (uint)(ch - 'A'))
    {
      for (i = ch - 'A'; shp->producedItemList[i]; i++)
      {
        shp->producedItemList[i] = shp->producedItemList[i + 1];
      }
    }

    displayEditShopSoldMenu(shp, mob);
    return false;
  }
  else

 // add an entry

  if ((origch == 'Z') && (numbItems != MAX_NUMBSHOPITEMS))
  {
    while (true)
    {
      uint vnum = 0;

      editUIntValSearchableList(&vnum, true, "vnum", displayObjectTypeList);

      if (!getVnumCheckVal() || objExists(vnum))
      {
        shp->producedItemList[numbItems] = vnum;
        if (numbItems < (MAX_NUMBSHOPITEMS - 1))
          shp->producedItemList[numbItems + 1] = 0;

        break;
      }
    }

    displayEditShopSoldMenu(shp, mob);
    return false;
  }
  else

 // get input, and love it

  if (((origch >= 'A') || (origch >= FIRST_SHOP_POSTALPHA_CH)) && numbItems)
  {
    usint ch = origch;

    if ((ch >= FIRST_SHOP_POSTALPHA_CH) && (ch < 'A'))
      ch += (LAST_SHOP_CH - FIRST_SHOP_POSTALPHA_CH) + 1;  // it's magic

    if (numbItems > (uint)(ch - 'A'))
    {
      while (true)
      {
        uint vnum = shp->producedItemList[ch - 'A'];

        editUIntValSearchableList(&vnum, true, "vnum", displayObjectTypeList);

        if (!getVnumCheckVal() || objExists(vnum))
        {
          shp->producedItemList[ch - 'A'] = vnum;

          break;
        }
      }
    }

    displayEditShopSoldMenu(shp, mob);

    return false;
  }
  else

 // quit - including nice little hack to prevent 'Q' from registering

  if ((checkMenuKey(origch, false) == MENUKEY_SAVE) && (origch != 'Q')) 
    return true;

  return false;
}


//
// editShopSold : edit a shop's sold list
//

void editShopSold(shop *shp, mobType *mob)
{
  usint ch;
  uint oldList[MAX_NUMBSHOPITEMS];


  memcpy(oldList, shp->producedItemList, sizeof(uint) * MAX_NUMBSHOPITEMS);

  displayEditShopSoldMenu(shp, mob);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      memcpy(shp->producedItemList, oldList,
             sizeof(uint) * MAX_NUMBSHOPITEMS);

      _outtext("\n\n");

      return;
    }

    if (interpEditShopSoldMenu(ch, shp, mob)) 
      return;
  }
}
