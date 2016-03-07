//
//  File: shop.cpp       originally part of durisEdit
//
//  Usage: various functions for manipulating shop records
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


#include <ctype.h>

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "fh.h"
#include "keys.h"

#include "shop.h"
#include "mob/mob.h"



extern shop *g_defaultShop;
extern uint g_numbLookupEntries;
extern room *g_currentRoom;
extern mobType **g_mobLookup;
extern bool g_madeChanges;

//
// shopExists : returns true if mob exists and has a shop
//

bool shopExists(const uint numb)
{
  mobType *mob = findMob(numb);

  if (!mob) 
    return false;

  return (mob->shopPtr != NULL);
}


//
// getLowestShopMobNumber : making sure that a mob does indeed have a
//                          shop somewhere in the zone should be done
//                          before calling this function
//

uint getLowestShopMobNumber(void)
{
  uint highest = getHighestMobNumber();

  for (uint i = getLowestMobNumber(); i <= highest; i++)
  {
    if (mobExists(i) && findMob(i)->shopPtr) 
      return i;
  }

  return g_numbLookupEntries;
}


//
// getHighestShopMobNumber : making sure that a mob does indeed have a
//                           shop somewhere in the zone should be done
//                           before calling this function
//

uint getHighestShopMobNumber(void)
{
  uint lowest = getLowestMobNumber();

  for (uint i = getHighestMobNumber(); i >= lowest; i--)
  {
    if (mobExists(i) && findMob(i)->shopPtr) 
      return i;
  }

  return 0;
}


//
// getPrevShopMob : find shop mob right before mobNumb, numerically
//

mobType *getPrevShopMob(const uint mobNumb)
{
  uint i = mobNumb - 1;

  if (mobNumb <= getLowestShopMobNumber()) 
    return NULL;

  while (!g_mobLookup[i] || !g_mobLookup[i]->shopPtr) 
    i--;

  return g_mobLookup[i];
}


//
// getNextShopMob : find shop mob right after mobNumb, numerically
//

mobType *getNextShopMob(const uint mobNumb)
{
  uint i = mobNumb + 1;

  if (mobNumb >= getHighestShopMobNumber()) 
    return NULL;

  while (!g_mobLookup[i] || !g_mobLookup[i]->shopPtr) 
    i++;

  return g_mobLookup[i];
}


//
// createShop : creates a shop
//

shop *createShop(void)
{
  shop *shp;
  
  if (g_defaultShop)
  {
    shp = copyShop(g_defaultShop);

    if (!shp)
    {
      displayAllocError("shop", "createShop");

      return NULL;
    }
  }
  else
  {
    shp = new(std::nothrow) shop;

    if (!shp)
    {
      displayAllocError("shop", "createShop");

      return NULL;
    }

    memset(shp, 0, sizeof(shop));

    shp->buyMult = shp->sellMult = 1.0;

    strcpy(shp->notSellingItem, "I'm not selling that, %s!");
    strcpy(shp->playerNoItem, "You don't seem to have that item, %s.");
    strcpy(shp->shopNoTradeItem, "I won't buy that, %s.");
    strcpy(shp->shopNoMoney, "Sorry there %s, but I don't have the money.");
    strcpy(shp->playerNoMoney, "You don't have the money, %s!");
    strcpy(shp->sellMessage, "Thank you, %s, that will be %s.");
    strcpy(shp->buyMessage, "%s, I will give you %s for that.");

    shp->firstOpen = shp->secondOpen = 0;
    shp->firstClose = shp->secondClose = 28;

    shp->noMagic = true;

    strcpy(shp->openMessage, "I am now open!");
    strcpy(shp->closeMessage, "My shop is now closed.");

    shp->shopkeeperRace = SHOP_RACE_HUMAN;

    strcpy(shp->racistMessage, "I do not sell to your people!");
  }

  return shp;
}


//
// deleteShop : deletes a shop
//

void deleteShop(shop *shp, const bool zoneChange)
{
  if (!shp) 
    return;

  delete shp;

  if (zoneChange)
    g_madeChanges = true;
}


//
// copyShop : copies a shop
//

shop *copyShop(const shop *srcShp)
{
  shop *newShp;


  if (!srcShp) 
    return NULL;

  newShp = new(std::nothrow) shop;
  if (!newShp)
  {
    displayAllocError("shop", "copyShop");

    return NULL;
  }

  memcpy(newShp, srcShp, sizeof(shop));

  return newShp;
}


//
// compareShopInfo : returns true if shops match
//

bool compareShopInfo(const shop *shp1, const shop *shp2)
{
  if (shp1 == shp2) 
    return true;

  if (!shp1 || !shp2) 
    return false;

 // technically, strings could have changed without any actual noticeable change to the user,
 // but -- I don't care!  feel free to fix it if you do

  if (memcmp(shp1, shp2, sizeof(shop))) 
    return false;

  return true;
}


//
// getNumbShopMobs : returns number of mob types with shops in the zone
//

uint getNumbShopMobs(void)
{
  uint numbShops = 0;
  const uint highMobNumb = getHighestMobNumber();


  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      const mobType *mob = findMob(mobNumb);

      if (mob->shopPtr) 
        numbShops++;
    }
  }

  return numbShops;
}


//
// checkForMobWithShop : returns true if a mob type has a shop
//

bool checkForMobWithShop(void)
{
  const uint highMobNumb = getHighestMobNumber();


  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      if (mob->shopPtr) 
        return true;
    }
  }

  return false;
}


//
// addingShopMakesTwoShopsInRoom : returns true if adding a shop to mobNumb would make for 2+ shops in some
//                                 room somewhere in the zone
//

bool addingShopMakesTwoShopsInRoom(const uint mobNumb)
{
  const uint highRoom = getHighestRoomNumber();
  uint roomNumb = getLowestRoomNumber();

  while (roomNumb <= highRoom)
  {
    if (roomExists(roomNumb))
    {
      const room *roomPtr = findRoom(roomNumb);

      if (getShopinRoom(roomPtr))
      {
        const mobHere *mob = roomPtr->mobHead;

        while (mob)
        {
          if (mob->mobNumb == mobNumb)
            return true;

          mob = mob->Next;
        }
      }
    }

    roomNumb++;
  }

  return false;
}


//
// displayShopList : Displays the list of mob types with shops
//

void displayShopList(const char *args)
{
  size_t lines = 1;
  const uint high = getHighestShopMobNumber();
  bool foundMob = false, listAll = false;


  if (noMobTypesExist())
  {
    _outtext("\nThere are currently no mob types.\n\n");
    return;
  }

  if (!checkForMobWithShop())
  {
    _outtext("\nNone of the current mob types have a shop.\n\n");
    return;
  }

  if (!args || (args[0] == '\0'))
  {
    listAll = true;
  }

  _outtext("\n\n");

  for (uint mobNumb = getLowestShopMobNumber(); mobNumb <= high; mobNumb++)
  {
    if (shopExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      if (listAll || scanKeyword(args, mob->keywordListHead))
      {
        char outstrn[MAX_MOBSNAME_LEN + 64];

        sprintf(outstrn, "%s&n (#%u)\n", mob->mobShortName, mobNumb);

        if (checkPause(outstrn, lines))
          return;

        foundMob = true;
      }
    }
  }

  if (!foundMob) 
    _outtext("No matching mob types found.\n");

  _outtext("\n");
}


//
// getNumbShopSold : returns number of items a shop is selling, max of MAX_NUMBSHOPITEMS
//

uint getNumbShopSold(const uint *soldArr)
{
  uint numb = 0;

  while ((numb < MAX_NUMBSHOPITEMS) && soldArr[numb])
    numb++;

  return numb;
}


//
// getNumbShopBought : returns number of item types a shop is selling, max of MAX_NUMBSHOPITEMTYPES
//

uint getNumbShopBought(const uint *soldArr)
{
  uint numb = 0;

  while ((numb < MAX_NUMBSHOPITEMTYPES) && soldArr[numb])
    numb++;

  return numb;
}


//
// getShopinRoom : returns pointer to any shop in the room
//

const shop *getShopinRoom(const room *roomPtr)
{
  const mobHere *mob;


  mob = roomPtr->mobHead;

 // should only be one shop mob in each room

  while (mob)
  {
    if (mob->mobPtr && mob->mobPtr->shopPtr)
      return mob->mobPtr->shopPtr;

    mob = mob->Next;
  }

  return NULL;
}


//
// getShopinCurrentRoom : returns shop in current room
//

const shop *getShopinCurrentRoom(void)
{
  return getShopinRoom(g_currentRoom);
}


//
// getShopOwner : returns mob type that owns shop
//

const mobType *getShopOwner(const shop *shp)
{
  const uint highMobNumb = getHighestMobNumber();


  if (!shp) 
    return NULL;

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      const mobType *mob = findMob(mobNumb);

      if (mob->shopPtr == shp) 
        return mob;
    }
  }

  return NULL;
}


//
// listShopSold : displays a list of things sold by the shop much like Duris; 
//                returns TRUE if shop found, FALSE otherwise
//

bool listShopSold(void)
{
  size_t numbLines = 0;
  char strn[MAX_MOBSNAME_LEN + MAX_OBJSNAME_LEN + 64], pricestrn[512];


 // check for shop in the same room

  const shop *shp = getShopinCurrentRoom();
  if (!shp) 
    return false;

  if (shp->producedItemList[0] == 0)
  {
    _outtext("\nThe shop in this room isn't selling anything.\n\n");

    return true;
  }

  const mobType *mob = getShopOwner(shp);
  if (!mob)
    return false;

 // should only be one of this mob loaded

  const mobHere *mobH = findMobHere(mob->mobNumber, NULL, true);
  if (!mobH)
    return false;

  sprintf(strn, "\n%s&n (#%u) is selling -\n\n", getMobShortName(mob), mob->mobNumber);
  if (checkPause(strn, numbLines))
    return true;

 // run through shop list

  for (uint i = 0; (i < MAX_NUMBSHOPITEMS) && shp->producedItemList[i]; i++)
  {
    const objectType *obj = findObj(shp->producedItemList[i]);

    if (obj)
    {
      if (getShowPricesAdjustedVal())
        getMoneyStrn((int)((float)obj->worth * shp->sellMult), pricestrn);
      else
        getMoneyStrn(obj->worth, pricestrn);
    }
    else
    {
      strcpy(pricestrn, "(price unknown)");
    }

    sprintf(strn, "  %s&n (#%u) - %s",
            getObjShortName(obj), shp->producedItemList[i], pricestrn);

   // let user know if obj is in shop list but not inventory

    if (!objinInv(mobH, shp->producedItemList[i]))
      strcat(strn, " &+c(not in mob's inventory)&n");

    strcat(strn, "\n");

    if (checkPause(strn, numbLines))
      return true;
  }

  _outtext("\n");

  return true;
}


//
// getMobHereShop : returns mobHere that owns shop
//

// used by checkAllShops() below to add item to mob's inv if necessary, so non-const

mobHere *getMobHereShop(const shop *shp)
{
  const uint highRoomNumb = getHighestRoomNumber();


  if (!shp) 
    return NULL;

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      mobHere *mob = roomPtr->mobHead;
      while (mob)
      {
        if (mob->mobPtr && (mob->mobPtr->shopPtr == shp)) 
          return mob;

        mob = mob->Next;
      }
    }
  }

  return NULL;
}


//
// checkShopsOnSave : returns false if unable to write shop file for some reason or another
//

bool checkShopsOnSave(void)
{
  char strn[1024];
  const uint highMobNumb = getHighestMobNumber();


  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      const mobType *mob = findMob(mobNumb);

      if (mob->shopPtr)
      {
        mobHere *mobH = getMobHereShop(mob->shopPtr);
        if (!mobH)
        {
          sprintf(strn, "\n"
  "Error: Mob #%u has a shop defined, but no instance of this mob is loaded.\n"
  "       Shop mobs must be loaded.  Save aborted.\n\n", mob->mobNumber);

          _outtext(strn);

          return false;
        }

        for (uint i = 0; (i < MAX_NUMBSHOPITEMS) && mob->shopPtr->producedItemList[i]; i++)
        {
          if (!objinInv(mobH, mob->shopPtr->producedItemList[i]))
          {
            sprintf(strn, "\n"
  "Warning: Mob #%u's shop sells object #%u, but it isn't in the mob's\n"
  "         carried list.  Add it",
                    mobNumb, mob->shopPtr->producedItemList[i]);

            if (displayYesNoPrompt(strn, promptYes, false) == promptYes)
            {
              objectHere *objH = createObjHere(mob->shopPtr->producedItemList[i], true);
              if (!objH)
              {
                displayAllocError("objectHere", "checkAllShops");

                return false;
              }

              addObjHeretoList(&(mobH->inventoryHead), objH, false);

             // set override so the object reloads properly

              setEntityOverride(ENTITY_OBJECT, mob->shopPtr->producedItemList[i], 999);
            }
          }
        }
      }
    }
  }

  return true;
}


//
// checkForDupeShopLoaded : returns true if a shop mob is loaded more than once
//

void checkForDupeShopLoaded(void)
{
  const uint highMobNumb = getHighestMobNumber();


  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      const mobType *mob = findMob(mobNumb);

      if (mob->shopPtr && (getNumbEntities(ENTITY_MOB, mobNumb, false) > 1))
      {
        char strn[1024];

        sprintf(strn, "\n"
  "Error: Mob #%u has a shop and is loaded more than once.  You can't\n"
  "       load a particular shop mob more than once.  Aborting.\n", mobNumb);

        _outtext(strn);

        exit(1);
      }
    }
  }
}


//
// checkForMultipleShopsInOneRoom : exits DE if a room has more than one shop mob
//

void checkForMultipleShopsInOneRoom(void)
{
  const uint high = getHighestRoomNumber();
  uint curr = getLowestRoomNumber();

  while (curr <= high)
  {
    if (roomExists(curr))
    {
      const room *roomPtr = findRoom(curr);
      const mobHere *mobPtr = roomPtr->mobHead;
      bool blnGotShop = false;

      while (mobPtr)
      {
        if (mobPtr->mobPtr && mobPtr->mobPtr->shopPtr)
        {
          if (blnGotShop)
          {
            char strn[1024];

            sprintf(strn, "\n"
"Error: Room #%u has two or more shop mobs loaded.  A room cannot\n"
"       contain more than one shop mob.  The vnum of one of the shop\n"
"       mobs is %u.  Aborting.\n", curr, mobPtr->mobNumb);

            _outtext(strn);

            exit(1);
          }
          else
          {
            blnGotShop = true;
          }
        }

        mobPtr = mobPtr->Next;
      }
    }

    curr++;
  }
}


//
// checkShopsOnLoad : check if any shops are loaded twice in the zone and if any room has more than one
//                    shop
//

void checkShopsOnLoad(void)
{
  checkForDupeShopLoaded();
  checkForMultipleShopsInOneRoom();
}
