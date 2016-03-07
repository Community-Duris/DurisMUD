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


#ifndef _SHOP_H_

#include "types.h"

// shopkeeper race

#define SHOP_RACE_LOWEST         1
#define SHOP_RACE_HUMAN          1
#define SHOP_RACE_BARB           2
#define SHOP_RACE_DROW           3
#define SHOP_RACE_GREY           4
#define SHOP_RACE_DWARF          5
#define SHOP_RACE_DUERGAR        6
#define SHOP_RACE_HALFLING       7
#define SHOP_RACE_GNOME          8
#define SHOP_RACE_OGRE           9
#define SHOP_RACE_TROLL         10
#define SHOP_RACE_HALFELF       11
#define SHOP_RACE_HIGHEST       11

#define MAX_SHOPSTRING_LEN     (usint)512
#define MAX_NUMBSHOPITEMTYPES   (usint)50
#define MAX_NUMBSHOPITEMS       (usint)50

typedef struct _shop
{
  uint producedItemList[MAX_NUMBSHOPITEMS];  // 0-terminated
  double buyMult;
  double sellMult;

  uint tradedItemList[MAX_NUMBSHOPITEMTYPES];  // 0-terminated

  char notSellingItem[MAX_SHOPSTRING_LEN + 32];  // strings are + 32 because readShopFile() reads strings
  char playerNoItem[MAX_SHOPSTRING_LEN + 32];    // directly into them, including LF and tilde, and this is
  char shopNoTradeItem[MAX_SHOPSTRING_LEN + 32]; // easier than changing readShopFile()
  char shopNoMoney[MAX_SHOPSTRING_LEN + 32];
  char playerNoMoney[MAX_SHOPSTRING_LEN + 32];
  char sellMessage[MAX_SHOPSTRING_LEN + 32];
  char buyMessage[MAX_SHOPSTRING_LEN + 32];

  uint roomNumb;  // room number mob's shop is in

  uint firstOpen;
  uint firstClose;
  uint secondOpen;
  uint secondClose;

  bool roaming;
  bool noMagic;
  bool killable;

  char openMessage[MAX_SHOPSTRING_LEN + 32];
  char closeMessage[MAX_SHOPSTRING_LEN + 32];

  int hometownFlag;  // currently unused
  int socialActionTypes;  // unused

  bool racist;
  int shopkeeperRace;

  char racistMessage[MAX_SHOPSTRING_LEN + 32];
} shop;

#define _SHOP_H_
#endif
