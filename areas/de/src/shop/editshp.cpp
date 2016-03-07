//
//  File: editshp.cpp    originally part of durisEdit
//
//  Usage: user-interface functions for edit shops
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
#include <ctype.h>


#include "fh.h"
#include "types.h"
#include "keys.h"
#include "misc/menu.h"

#include "graphcon.h"

#include "editshp.h"



extern bool g_madeChanges;
extern uint g_numbLookupEntries;
extern flagDef g_shopShopkeeperRaceList[];
extern menu g_shopMenu;


//
// displayEditShopMenu : Displays edit shop menu
//
//   *shp : shop being edited
//

void displayEditShopMenu(const shop *shp, const mobType *mob)
{
  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "shop", false);

  displayMenu(&g_shopMenu, shp);
}


//
// interpEditShopMenu : Interprets user input for edit shop menu - returns
//                      TRUE if the user hits 'Q', SHOP_DELETED if deleted, 
//                      FALSE otherwise
//
//    ch : user input
//  *shp : shop to edit
//

char interpEditShopMenu(const usint ch, shop *shp, mobType *mob)
{
 // edit list of sold items

  if (ch == 'A')
  {
    editShopSold(shp, mob);

    displayEditShopMenu(shp, mob);
  }
  else

 // edit list of item types bought

  if (ch == 'D')
  {
    editShopBought(shp, mob);

    displayEditShopMenu(shp, mob);
  }
  else

 // edit shop messages

  if (ch == 'E')
  {
    editShopMessages(shp, mob);

    displayEditShopMenu(shp, mob);
  }
  else

 // edit shop opening/closing times

  if (ch == 'F')
  {
    editShopTimes(shp, mob);

    displayEditShopMenu(shp, mob);
  }
  else

 // edit shop roam/no magic/killable flags

  if (ch == 'G')
  {
    editShopBooleans(shp, mob);

    displayEditShopMenu(shp, mob);
  }
  else

 // edit shop racist info

  if (ch == 'H')
  {
    editShopRacist(shp, mob);

    displayEditShopMenu(shp, mob);
  }
  else

 // some options

  if (interpretMenu(&g_shopMenu, shp, ch))
  {
    displayEditShopMenu(shp, mob);

    return false;
  }
  else

 // delete entire shop

  if (ch == 'Y')
  {
    if (displayYesNoPrompt("&+cDelete this mob's shop", promptNo, true) == promptYes)
    {
      deleteShop(mob->shopPtr, false);
      mob->shopPtr = NULL;

      return SHOP_DELETED;
    }

    displayEditShopMenu(shp, mob);
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// realEditShop : edit a shop
//

mobType *realEditShop(shop *shp, mobType *mob, const bool allowJump)
{
  usint ch;
  shop *newShop;
  char done = false;  // might be SHOP_DELETED
  const uint mobNumb = mob->mobNumber;


  if (!shp) 
    return NULL;

 // copy shp into newShop and display the menu

  newShop = copyShop(shp);
  if (!newShop)
  {
    displayAllocError("shop", "realEditShop");

    return NULL;
  }

  displayEditShopMenu(newShop, mob);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      deleteShop(newShop, false);

      _outtext("\n\n");

      return NULL;
    }

    if (allowJump)
    {
      if ((checkMenuKey(ch, false) == MENUKEY_NEXT) || (checkMenuKey(ch, false) == MENUKEY_PREV) ||
          (checkMenuKey(ch, false) == MENUKEY_JUMP))
      {
        done = true;
      }
    }

   // if interpEditShopMenu is TRUE, user has quit

    if (!done) 
      done = interpEditShopMenu(ch, newShop, mob);

    if (done == SHOP_DELETED)
    {
      return NULL;
    }

    if (done)
    {
      if (!compareShopInfo(shp, newShop))   // changes have been made
      {
        memcpy(shp, newShop, sizeof(shop));

        delete newShop;

        g_madeChanges = true;
      }
      else 
      {
        deleteShop(newShop, false);
      }

      if (allowJump)
      {
        if (checkMenuKey(ch, false) == MENUKEY_JUMP)
        {
          uint numb = mob->mobNumber;

          switch (jumpShop(&numb))
          {
            case MENU_JUMP_ERROR : return mob;
            case MENU_JUMP_VALID : return findMob(numb);

            default : return NULL;
          }
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_NEXT)
        {
          if (mobNumb != getHighestShopMobNumber())
            return getNextShopMob(mobNumb);
          else
            return mob;
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_PREV)
        {
          if (mobNumb != getLowestShopMobNumber())
            return getPrevShopMob(mobNumb);
          else
            return mob;
        }
      }

      _outtext("\n\n");

      return NULL;
    }
  }
}


//
// editShop
//

void editShop(mobType *mob, const bool allowJump)
{
  do
  {
    mob = realEditShop(mob->shopPtr, mob, allowJump);
  } while (mob);
}


//
// jumpShop
//

char jumpShop(uint *numb)
{
  char promptstrn[128];
  const struct rccoord coords = _gettextposition();

  clrline(coords.row, 7, 0);
  _settextposition(coords.row, 1);

  sprintf(promptstrn, "shop vnum to jump to [%u-%u]", 
          getLowestShopMobNumber(), getHighestShopMobNumber());

  editUIntValSearchableList(numb, false, promptstrn, displayShopList);

  if ((*numb >= g_numbLookupEntries) || !shopExists(*numb))
    return MENU_JUMP_ERROR;
  else
    return MENU_JUMP_VALID;
}
