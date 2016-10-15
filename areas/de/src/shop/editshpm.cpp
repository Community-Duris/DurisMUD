//
//  File: editshpm.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing shop messages
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
#include <ctype.h>

#include "../fh.h"
#include "../types.h"
#include "../misc/menu.h"

#include "../graphcon.h"

#include "shop.h"

extern menu g_shopMessagesMenu;


//
// displayEditShopMessagesMenu : Displays edit shop messages menu
//

void displayEditShopMessagesMenu(const mobType *mob)
{
  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "shop messages", false);

  displayMenu(&g_shopMessagesMenu, mob->shopPtr);
}


//
// interpEditShopMessagesMenu :
//                      Interprets user input for edit shop msgs menu - returns
//                      TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//   *shp : shop to edit
//

bool interpEditShopMessagesMenu(const usint ch, shop *shp, mobType *mob)
{
  if (interpretMenu(&g_shopMessagesMenu, shp, ch))
  {
    displayEditShopMessagesMenu(mob);
    return false;
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editShopMessages : edit shop messages
//

void editShopMessages(shop *shp, mobType *mob)
{
  char noSellOld[MAX_SHOPSTRING_LEN + 1], playerNoOld[MAX_SHOPSTRING_LEN + 1],
       shopNoTradeOld[MAX_SHOPSTRING_LEN + 1], shopNoMoneyOld[MAX_SHOPSTRING_LEN + 1],
       playerNoMoneyOld[MAX_SHOPSTRING_LEN + 1], sellMsgOld[MAX_SHOPSTRING_LEN + 1],
       buyMsgOld[MAX_SHOPSTRING_LEN + 1], openMsgOld[MAX_SHOPSTRING_LEN + 1],
       closeMsgOld[MAX_SHOPSTRING_LEN + 1], raceMsgOld[MAX_SHOPSTRING_LEN + 1];


 // copy all the messages

  strcpy(noSellOld, shp->notSellingItem);
  strcpy(playerNoOld, shp->playerNoItem);
  strcpy(shopNoTradeOld, shp->shopNoTradeItem);
  strcpy(shopNoMoneyOld, shp->shopNoMoney);
  strcpy(playerNoMoneyOld, shp->playerNoMoney);
  strcpy(sellMsgOld, shp->sellMessage);
  strcpy(buyMsgOld, shp->buyMessage);
  strcpy(openMsgOld, shp->openMessage);
  strcpy(closeMsgOld, shp->closeMessage);
  strcpy(raceMsgOld, shp->racistMessage);

 // display the menu

  displayEditShopMessagesMenu(mob);

  while (true)
  {
    const usint ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      strcpy(shp->notSellingItem, noSellOld);
      strcpy(shp->playerNoItem, playerNoOld);
      strcpy(shp->shopNoTradeItem, shopNoTradeOld);
      strcpy(shp->shopNoMoney, shopNoMoneyOld);
      strcpy(shp->playerNoMoney, playerNoMoneyOld);
      strcpy(shp->sellMessage, sellMsgOld);
      strcpy(shp->buyMessage, buyMsgOld);
      strcpy(shp->openMessage, openMsgOld);
      strcpy(shp->closeMessage, closeMsgOld);
      strcpy(shp->racistMessage, raceMsgOld);

      return;
    }

   // if interpEditShopMessagesMenu is TRUE, user has exited menu

    if (interpEditShopMessagesMenu(ch, shp, mob)) 
      return;
  }
}
