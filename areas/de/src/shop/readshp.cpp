//
//  File: readshp.cpp    originally part of durisEdit
//
//  Usage: functions for reading shop info from the .shp file
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



#include <stdlib.h>
#include <string.h>

#include "../types.h"
#include "../fh.h"

#include "../readfile.h"

#include "shop.h"



extern bool g_madeChanges, g_readFromSubdirs;


//
// ensureShopValueIsNumeric : if string is not numeric, display error and exit(1)
//

void ensureShopValueIsNumeric(const char *input, const char *inputWhat, const uint shopNumb)
{
  if (!strnumber(input))
  {
    char outstrn[512];

    _outtext(
"Error: Line that should be shop's ");
    
    _outtext(inputWhat);
    _outtext(" isn't numeric.  String\n"
"       read was\n'");

    _outtext(input);
    
    sprintf(outstrn, "'  (shop #%u).  Aborting.\n",
            shopNumb);

    _outtext(outstrn);

    exit(1);
  }
}


//
// ensureShopValueIsYorN : if string is not Y or N, display error and exit(1)
//

void ensureShopValueIsYorN(const char *input, const char *inputWhat, const uint shopNumb)
{
  if (!isValidBoolYesNo(input))
  {
    _outtext(
"Error: ");

    _outtext(input);

    char outstrn[512];

    sprintf(outstrn, " for shop #%u is not 'Y' or 'N' - string read was\n"
"       '",
            shopNumb);
    
    _outtext(outstrn);
    _outtext(input);
    _outtext("'.  Aborting.\n");

    exit(1);
  }
}

//
// readShopMessage : multitudinous redundant checks - returns false if there is an error
//

bool readShopMessage(FILE *shopFile, const uint shopNumb, char *strn, const char *strMsgName, 
                     const uint intNumbPct, const bool allowOnePctS)
{
  if (!readAreaFileLine(shopFile, strn, MAX_SHOPSTRING_LEN + 2, ENTITY_SHOP, shopNumb, 
                        ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        strMsgName, 0, NULL, true, true))
    return false;

 // check string for %s's

  if ((numbPercentS(strn) != intNumbPct) && (!allowOnePctS || (numbPercentS(strn) != 1)))
  {
    const char *oneStrn;

    if (allowOnePctS)
      oneStrn = " or 1";
    else
      oneStrn = "";

    char outstrn[512];

    sprintf(outstrn,
"Error: '%s' string for shop #%u doesn't have the\n"
"       correct number of '%%s's (%u%s).  string read was '",
            strMsgName, shopNumb, intNumbPct, oneStrn);

    _outtext(outstrn);
    _outtext(strn);

    _outtext("'.\n\n"
"       Aborting.\n");

    return false;
  }

  return true;
}


//
// readShopFromFile : read a mob's shop info from a file - returns pointer
//                    to shop info
//
//    *shopFile : file to read info from
//  defaultShop : if true, doesn't check if mob exists
//

shop *readShopFromFile(FILE *shopFile, const bool defaultShop)
{
  char strn[MAX_SHOPSTRING_LEN + 5];
  shop *shopNode;


 // Read the shop number - ends in a tilde

  bool hitEOF;

  if (!readAreaFileLineAllowEOF(shopFile, strn, 512, ENTITY_SHOP, ENTITY_NUMB_UNUSED, 
                                ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, "vnum", 0, NULL, true, true, 
                                &hitEOF))
    exit(1);

  if (hitEOF)
    return NULL;

 // allocate memory for shop

  shopNode = new(std::nothrow) shop;
  if (!shopNode)
  {
    displayAllocError("shop", "readShopFromFile");

    exit(1);
  }

 // set everything in shop record to 0/NULL

  memset(shopNode, 0, sizeof(shop));

 // do stuff to string read above

  if (strn[0] != '#')
  {
    _outtext("String that should be vnum of shop doesn't start with a '#'.  String read was\n'");
    _outtext(strn);
    _outtext("'.  Aborting.\n");

    exit(1);
  }

  deleteChar(strn, 0);

  if (!strnumber(strn))
  {
    _outtext(
"Error in string that should be shop vnum - after removing first and last\n"
"characters ('#' at start and '~' at end), string is '");
    _outtext(strn);
    _outtext("'.\n\nAborting.\n");

    exit(1);
  }

  const uint shopNumb = strtoul(strn, NULL, 10);

  if (!defaultShop)
  {
    mobType *mob = findMob(shopNumb);

    if (!mob)
    {
      char outstrn[512];

      sprintf(outstrn,
"Error: Couldn't find mob #%u specified in .shp file.  Aborting.\n",
              shopNumb);

      _outtext(outstrn);

      exit(1);
    }

    if (mob->shopPtr)
    {
      char outstrn[512];

      sprintf(outstrn,
"Error: Mob #%u specified in .shp file already has a shop.  Aborting.\n",
              shopNumb);

      _outtext(outstrn);

      exit(1);
    }

    mob->shopPtr = shopNode;
  }

 // Next string should be 'N' for new format

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "N line", 0, NULL, false, true))
    exit(1);

  if (!strcmpnocase(strn, "N"))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: String after shop number (for shop #%u) that should be 'N'\n"
"       is '",
            shopNumb);
    
    _outtext(outstrn);
    _outtext(strn);
    _outtext("'.  Aborting.\n");

    exit(1);
  }

 // read item list

  uint numbEntries = 0;
  while (true)
  {
    if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                          "produced item list", 1, NULL, false, true))
      exit(1);

    ensureShopValueIsNumeric(strn, "entry in produced item list", shopNumb);

    if (atoi(strn) == 0)
      break;  // end of list (array already set to 0 above)

    if (numbEntries < MAX_NUMBSHOPITEMS) 
      shopNode->producedItemList[numbEntries] = strtoul(strn, NULL, 10);

    numbEntries++;
  }

  if (numbEntries > MAX_NUMBSHOPITEMS)
  {
    char outstrn[512];

    sprintf(outstrn,
"Warning: Produced object list for shop #%u contains more than %u\n"
"         objects (%u objects read total) - list truncated.\n",
            shopNumb, (uint)MAX_NUMBSHOPITEMS, numbEntries);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;
  }

 // read buy multiplier (0.0 - 1.0)

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "buy multiplier", 1, NULL, false, true))
    exit(1);

  shopNode->buyMult = atof(strn);
  if ((shopNode->buyMult > 1.0) || (shopNode->buyMult <= 0.0))
  {
    char outstrn[1024];

    sprintf(outstrn,
"Error: Buy multiplier for shop #%u is an invalid value\n"
"       (less than or equal to 0.0 or greater than 1.0).  value read\n"
"       was %f.  Aborting.\n",
            shopNumb, shopNode->buyMult);

    _outtext(outstrn);

    exit(1);
  }

 // read sell multiplier (>1.0)

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "sell multiplier", 1, NULL, false, true))
    exit(1);

  shopNode->sellMult = atof(strn);
  if (shopNode->sellMult < 1.0)
  {
    char outstrn[1024];

    sprintf(outstrn,
"Error: Sell multiplier for shop #%u is an invalid value\n"
"       (less than 1.0).  value read was %f.\n"
"       Aborting.\n",
            shopNumb, shopNode->sellMult);

    _outtext(outstrn);

    exit(1);
  }

 // read item types bought list

  numbEntries = 0;
  while (true)
  {
    if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                          "item type list", 1, NULL, false, true))
      exit(1);

    ensureShopValueIsNumeric(strn, "entry in item type list", shopNumb);

    if (atoi(strn) == 0)
      break;  // end of list (array already set to 0 above)

    if (numbEntries < MAX_NUMBSHOPITEMTYPES) 
      shopNode->tradedItemList[numbEntries] = strtoul(strn, NULL, 10);

    numbEntries++;
  }

  if (numbEntries > MAX_NUMBSHOPITEMTYPES)
  {
    char outstrn[512];

    sprintf(outstrn,
"Warning: Object type list for shop #%u has more than %u\n"
"         objects (%u items read total) - list truncated.\n",
            shopNumb, (uint)MAX_NUMBSHOPITEMTYPES, numbEntries);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;
  }

 // read "not selling item" message

  if (!readShopMessage(shopFile, shopNumb, shopNode->notSellingItem, "not selling item", 1, false))
    exit(1);

 // read "player does not have item trying to sell" message

  if (!readShopMessage(shopFile, shopNumb, shopNode->playerNoItem, "player doesn't have item", 1, false))
    exit(1);

 // read "shop doesn't trade in item trying to sell" message

  if (!readShopMessage(shopFile, shopNumb, shopNode->shopNoTradeItem, "shop doesn't trade in item type", 1, 
                       false))
    exit(1);

 // read "shop doesn't have the money" message

  if (!readShopMessage(shopFile, shopNumb, shopNode->shopNoMoney, "shop has no money", 1, false))
    exit(1);

 // read "player doesn't have the money" message

  if (!readShopMessage(shopFile, shopNumb, shopNode->playerNoMoney, "player has no money", 1, false))
    exit(1);

 // read "sell message" message (shop sells to player)

  if (!readShopMessage(shopFile, shopNumb, shopNode->sellMessage, "shop sells to player", 2, 
                       (shopNode->producedItemList[0] == 0)))
    exit(1);

 // read "buy message" message (shop buys from player)

  if (!readShopMessage(shopFile, shopNumb, shopNode->buyMessage, "shop buys from player", 2, 
                       (shopNode->tradedItemList[0] == 0)))
    exit(1);

 // next two lines should simply be zeroes

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "0 line", 0, NULL, false, true))
    exit(1);

  if (strcmp(strn, "0"))
  {
    char outstrn[512];

    _outtext(
"Error: First line after first block of shop messages should be '0', but instead\n"
"       is '");
    _outtext(strn);
    
    sprintf(outstrn, "'  (shop #%u).  Aborting.\n",
            shopNumb);

    _outtext(outstrn);

    exit(1);
  }

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "0 line", 0, NULL, false, true))
    exit(1);

  if (strcmp(strn, "0"))
  {
    char outstrn[512];

    _outtext(
"Error: Second line after first block of shop messages should be '0', but\n"
"       instead is '");
    _outtext(strn);
    
    sprintf(outstrn, "'  (shop #%u).  Aborting.\n",
            shopNumb);

    _outtext(outstrn);

    exit(1);
  }

 // read shopkeeper's mob numb - same as shop numb, hopefully

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "mob #", 0, NULL, false, true))
    exit(1);

  if (strtoul(strn, NULL, 10) != shopNumb)
  {
    char outstrn[512];

    _outtext(
"Error: Shop's mob number should be same as the shop number, but string read\n"
"       was '");
    _outtext(strn);

    sprintf(outstrn, "'.  (Shop number is %u.)\n"
"       Aborting.\n",
            shopNumb);

    _outtext(outstrn);

    exit(1);
  }

 // yet another 0

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "0 line", 0, NULL, false, true))
    exit(1);

  if (strcmp(strn, "0"))
  {
    char outstrn[512];

    _outtext(
"Error: Line after shop's mob number should be '0', but instead is\n'");
    _outtext(strn);
    
    sprintf(outstrn, "'  (shop #%u).  Aborting.\n",
            shopNumb);

    _outtext(outstrn);

    exit(1);
  }

 // shopkeeper's room numb

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "room #", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsNumeric(strn, "room numb", shopNumb);

  shopNode->roomNumb = strtoul(strn, NULL, 10);

  if (shopNode->roomNumb && !roomExists(shopNode->roomNumb))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Shop's room numb line for shop #%u specifies a room that\n"
"       doesn't exist in this .wld (room #%u).  Aborting.\n",
            shopNumb, shopNode->roomNumb);

    _outtext(outstrn);

    exit(1);
  }

 // read first opening and closing times

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "opening time", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsNumeric(strn, "opening time", shopNumb);

  shopNode->firstOpen = strtoul(strn, NULL, 10);

 // read closing time

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "closing time", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsNumeric(strn, "closing time", shopNumb);

  shopNode->firstClose = strtoul(strn, NULL, 10);

 // read second opening and closing times

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "second opening time", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsNumeric(strn, "second opening time", shopNumb);

  shopNode->secondOpen = strtoul(strn, NULL, 10);

 // read second closing time

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "second closing time", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsNumeric(strn, "second closing time", shopNumb);

  shopNode->secondClose = strtoul(strn, NULL, 10);

 // read shop roaming value - Y/N

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "roam value", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsYorN(strn, "Roaming value", shopNumb);

  shopNode->roaming = getBoolYesNo(strn);

  if (!defaultShop)
  {
    if (shopNode->roaming)
    {
      if (shopNode->roomNumb)
      {
        char outstrn[512];

        sprintf(outstrn,
"Error: Shop #%u is a roaming shop, but has a room number specified\n"
"       (#%u).  Room number should be 0.  Aborting.\n",
                shopNumb, shopNode->roomNumb);

        _outtext(outstrn);

        exit(1);
      }
    }
    else  // not roaming
    {
      if (!shopNode->roomNumb)
      {
        char outstrn[512];

        sprintf(outstrn,
"Error: Shop #%u is not a roaming shop, but has a room number of 0.\n"
"       Aborting.\n",
                shopNumb);

        _outtext(outstrn);

        exit(1);
      }
    }
  }

 // read shop no magic value - Y/N

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "no-magic value", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsYorN(strn, "No-magic value", shopNumb);

  shopNode->noMagic = getBoolYesNo(strn);

 // read shop killable value - Y/N

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "killable value", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsYorN(strn, "Killable value", shopNumb);

  shopNode->killable = getBoolYesNo(strn);

 // read opening message

  if (!readAreaFileLine(shopFile, shopNode->openMessage, MAX_SHOPSTRING_LEN + 2, ENTITY_SHOP, shopNumb, 
                        ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "opening message", 0, NULL, true, true))
    exit(1);

 // read closing message

  if (!readAreaFileLine(shopFile, shopNode->closeMessage, MAX_SHOPSTRING_LEN + 2, ENTITY_SHOP, shopNumb, 
                        ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "closing message", 0, NULL, true, true))
    exit(1);

 // read hometown flag

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "hometown flag", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsNumeric(strn, "hometown flag", shopNumb);

  shopNode->hometownFlag = strtoul(strn, NULL, 10);

 // read social action types - not sure this was ever imp'd on Duris

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "social action flag", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsNumeric(strn, "social action flag", shopNumb);

  shopNode->socialActionTypes = atoi(strn);

 // read shop racism value - Y/N

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "racism value", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsYorN(strn, "Racist value", shopNumb);

  shopNode->racist = getBoolYesNo(strn);

 // read shopkeeper's race

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "race flag", 0, NULL, false, true))
    exit(1);

  ensureShopValueIsNumeric(strn, "race flag", shopNumb);

  shopNode->shopkeeperRace = strtoul(strn, NULL, 10);

 // read racism message

  if (!readAreaFileLine(shopFile, shopNode->racistMessage, MAX_SHOPSTRING_LEN + 2, ENTITY_SHOP, shopNumb, 
                        ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "racist message", 0, NULL, true, true))
    exit(1);

 // finally, last line should be an 'X'

  if (!readAreaFileLine(shopFile, strn, 512, ENTITY_SHOP, shopNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, 
                        "X line", 0, NULL, false, true))
    exit(1);

  if (!strcmpnocase(strn, "X"))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Last line for shop #%u should be an 'X', but is\n"
"       '",
            shopNumb);
    
    _outtext(outstrn);
    _outtext(strn);
    _outtext("' instead.  Aborting.\n");

    exit(1);
  }

  return shopNode;
}


//
// readShopFile : Reads shops from the shop file - returns TRUE if file
//                was found, FALSE otherwise
//
//   *filename : pointer to filename - if NULL, checks the MAINZONENAME var
//

bool readShopFile(const char *filename)
{
  FILE *shopFile;
  char shopFilename[512] = "";


 // assemble the filename of the shop file

  if (g_readFromSubdirs) 
    strcpy(shopFilename, "shp/");

  if (filename) 
    strcat(shopFilename, filename);
  else 
    strcat(shopFilename, getMainZoneNameStrn());

  strcat(shopFilename, ".shp");

 // open the shop file for reading

  if ((shopFile = fopen(shopFilename, "rt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(shopFilename);
    _outtext(", skipping\n");

    return false;
  }

  _outtext("Reading ");
  _outtext(shopFilename);
  _outtext("...\n");

 // this while loop reads shop by shop, one shop per iteration

  while (true)
  {
    if (!readShopFromFile(shopFile, false)) 
      break;  // eof
  }

  fclose(shopFile);

  return true;
}
