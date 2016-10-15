//
//  File: editshpt.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing shop 'bought' list
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
#include <string.h>
#include <stdlib.h>

#include "../types.h"
#include "../fh.h"
#include "../misc/menu.h"
#include "../keys.h"

#include "../graphcon.h"

#include "shop.h"

#include "editshp.h"

extern flagDef g_objTypeList[];
extern menu g_shopItemTypeMenu;


//
// displayEditShopBoughtMenu : Displays edit shop bought menu
//
//   *shp : shop being edited
//

void displayEditShopBoughtMenu(const shop *shp, const mobType *mob)
{
  char strn[1024], ch;
  uint numbShown = 0, numbItems = getNumbShopBought(shp->tradedItemList), i;


  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "shop traded list", false);

  if (numbItems == 0)
  {
    displayColorString("&+w  This shop does not buy any item types.\n");
  }

  for (i = 0; i < numbItems; i++)
  {
   // if these babies get over 127 we're doomed anyway

    if ((numbShown + 'A') > LAST_SHOP_CH)
      ch = (char)(FIRST_SHOP_POSTALPHA_CH + (numbShown - (LAST_SHOP_CH - 'A')) - 1);
    else 
      ch = (char)(numbShown + 'A');

    sprintf(strn, "   &+Y%c&+L. &+w%s (#%u)\n",
            ch, getObjTypeStrn(shp->tradedItemList[i]), shp->tradedItemList[i]);

    displayColorString(strn);

    numbShown++;

    if ((numbShown % MAX_SHOP_ENTRIES) == 0)
    {
      displayColorString("\n&+CPress a key to continue..");
      getkey();
      _outtext("\n\n");
    }
  }

  displayMenu(&g_shopItemTypeMenu, shp);
}


//
// interpEditShopBoughtMenu : Interprets user input for edit room menu - returns
//                            TRUE if the user hits 'Q', FALSE otherwise
//
//    ch : user input
//  *shp : shop to edit
//

bool interpEditShopBoughtMenu(const usint origch, shop *shp, mobType *mob)
{
  struct rccoord coords;
  uint numbItems = getNumbShopBought(shp->tradedItemList), i;
  int oldVal;


 // delete an entry (or all of them)

  if ((origch == 'Y') && numbItems)
  {
    usint ch;

    coords = _gettextposition();

    _settextposition(coords.row, 1);
    clrline(coords.row);

    displayColorString(
"&+CEnter letter of item type to delete (Y to delete all item types): ");

    do
    {
      ch = toupper(getkey());

      if (ch == 'Y')
      {
        shp->tradedItemList[0] = 0;

        displayEditShopBoughtMenu(shp, mob);
        return false;
      }

      if ((ch == 'X') || (ch == K_Escape) || (ch == K_Enter))
      {
        displayEditShopBoughtMenu(shp, mob);
        return false;
      }

      if ((ch >= FIRST_SHOP_POSTALPHA_CH) && (ch < 'A'))
         ch += (LAST_SHOP_CH - FIRST_SHOP_POSTALPHA_CH) + 1;  // it's magic
    } while (!(numbItems > (uint)(ch - 'A')));

    if (numbItems > (uint)(ch - 'A'))
    {
      for (i = ch - 'A'; shp->tradedItemList[i] && (i < MAX_NUMBSHOPITEMTYPES - 1); i++)
      {
        shp->tradedItemList[i] = shp->tradedItemList[i + 1];
      }

      shp->tradedItemList[i] = 0;
    }

    displayEditShopBoughtMenu(shp, mob);
    return false;
  }
  else

 // add an entry

  if ((origch == 'Z') && (numbItems != MAX_NUMBSHOPITEMTYPES))
  {
    if (editFlags(g_objTypeList, &(shp->tradedItemList[numbItems]), ENTITY_SHOP, 
                  getMobShortName(mob), mob->mobNumber, "object type bought", NULL, 0, false))
    {
     // check for duplicate types

      for (i = 0; i < numbItems; i++)
      {
        if (shp->tradedItemList[i] == shp->tradedItemList[numbItems])
        {
          coords = _gettextposition();

          _settextposition(coords.row, 1);
          clrline(coords.row);

          displayColorString("&+CError: this shop already buys that type of item.  Press a key..");
          getkey();

          shp->tradedItemList[numbItems] = 0;

          displayEditShopBoughtMenu(shp, mob);
          return false;
        }
      }

      shp->tradedItemList[numbItems + 1] = 0;
    }

    displayEditShopBoughtMenu(shp, mob);
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
      oldVal = shp->tradedItemList[(uint)(ch - 'A')];

      editFlags(g_objTypeList, &(shp->tradedItemList[(uint)(ch - 'A')]), ENTITY_SHOP, 
                getMobShortName(mob), mob->mobNumber, "object type bought", NULL, 0, false);

     // check for duplicates

      for (i = 0; i < numbItems; i++)
      {
        if (i == (ch - 'A')) continue;
       
        if (shp->tradedItemList[i] == shp->tradedItemList[(uint)(ch - 'A')])
        {
          coords = _gettextposition();

          _settextposition(coords.row, 1);
          clrline(coords.row);

          displayColorString("&+CError: this shop already buys that type of item.  Press a key..");
          getkey();

          shp->tradedItemList[(uint)(ch - 'A')] = oldVal;

          displayEditShopBoughtMenu(shp, mob);
          return false;
        }
      }

      displayEditShopBoughtMenu(shp, mob);
    }

    return false;
  }
  else

 // quit - including nice little hack to prevent 'Q' from registering

  if ((checkMenuKey(origch, false) == MENUKEY_SAVE) && (origch != 'Q')) 
    return true;

  return false;
}


//
// editShopBought : edit a shop's sold list
//

void editShopBought(shop *shp, mobType *mob)
{
  usint ch;
  uint oldList[MAX_NUMBSHOPITEMTYPES];


  memcpy(oldList, shp->tradedItemList, sizeof(uint) * MAX_NUMBSHOPITEMTYPES);

  displayEditShopBoughtMenu(shp, mob);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      memcpy(shp->tradedItemList, oldList, sizeof(uint) * MAX_NUMBSHOPITEMTYPES);

      _outtext("\n\n");

      return;
    }

    if (interpEditShopBoughtMenu(ch, shp, mob)) 
      return;
  }
}
