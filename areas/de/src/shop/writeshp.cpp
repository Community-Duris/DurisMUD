//
//  File: writeshp.cpp   originally part of durisEdit
//
//  Usage: functions for writing shop info to the .shp file
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
#include <string.h>

#include "../types.h"
#include "../fh.h"

#include "shop.h"



extern bool g_readFromSubdirs;

//
// writeShoptoFile : shopRec may be altered if improperly set (roaming)
//
//                   if defaultShop is true, doesn't check if shop is loaded in zone
//

void writeShoptoFile(FILE *shopFile, shop *shopRec, const uint mobNumb, const bool defaultShop)
{
  uint i;
  char strn[512];


 // write shop numb (same as mob numb) and N line

  fprintf(shopFile, "#%u~\nN\n", mobNumb);

 // write list of items produced by shop

  for (i = 0; (i < MAX_NUMBSHOPITEMS) && shopRec->producedItemList[i]; i++)
  {
    fprintf(shopFile, "%u\n", shopRec->producedItemList[i]);
  }

 // terminate list

  fputs("0\n", shopFile);

 // write buy/sell multipliers

  fprintf(shopFile, "%.2f\n%.2f\n", shopRec->buyMult, shopRec->sellMult);

 // write list of item types bought by shop

  for (i = 0; (i < MAX_NUMBSHOPITEMTYPES) && shopRec->tradedItemList[i]; i++)
  {
    fprintf(shopFile, "%u\n", shopRec->tradedItemList[i]);
  }

 // terminate list

  fputs("0\n", shopFile);

 // write shop messages and two zeroes

  fprintf(shopFile, "%s~\n" "%s~\n" "%s~\n" "%s~\n" "%s~\n" "%s~\n" "%s~\n"
                    "0\n" "0\n",
          shopRec->notSellingItem,  shopRec->playerNoItem,
          shopRec->shopNoTradeItem, shopRec->shopNoMoney,
          shopRec->playerNoMoney,   shopRec->sellMessage, shopRec->buyMessage);

 // write shop's mob numb and yet another zero

  fprintf(shopFile, "%u\n" "0\n", mobNumb);

 // write shop's shop number (room number) - if roaming or default, must be 0

  if (shopRec->roaming || defaultShop) 
  {
    fputs("0\n", shopFile);
  }
  else
  {
    if (!findMobHere(mobNumb, &i, false))  // shouldn't happen..
    {
      sprintf(strn, "\n"
"Warning: Mob #%u has a shop, but is not loaded.  Writing shop room number\n"
"         as 0 and setting to roaming, but this may cause problems..\n\n", 
              mobNumb);

      displayAnyKeyPrompt(strn);

      shopRec->roaming = true;

      fputs("0\n", shopFile);
    }
    else 
    {
      fprintf(shopFile, "%u\n", i);
    }
  }

 // write shop's closing/opening times

  fprintf(shopFile, "%u\n" "%u\n" "%u\n" "%u\n",
          shopRec->firstOpen,  shopRec->firstClose,
          shopRec->secondOpen, shopRec->secondClose);

 // write shop roam, no magic, killable flags

  fprintf(shopFile, "%s\n" "%s\n" "%s\n",
          getYesNoBool(shopRec->roaming),
          getYesNoBool(shopRec->noMagic),
          getYesNoBool(shopRec->killable));

 // write shop opening and closing messages

  fprintf(shopFile, "%s~\n" "%s~\n",
          shopRec->openMessage, shopRec->closeMessage);

 // write shop hometown and social action flags

  fprintf(shopFile, "%d\n" "%d\n",
          shopRec->hometownFlag, shopRec->socialActionTypes);

 // write shop racist value, race, and racist message, terminate with X

  fprintf(shopFile, "%s\n" "%d\n" "%s~\n" "X\n",
          getYesNoBool(shopRec->racist),
          shopRec->shopkeeperRace,
          shopRec->racistMessage);
}


//
// writeShopFile : Write the shop file
//

void writeShopFile(const char *filename)
{
  FILE *shopFile;
  char shopFilename[512] = "";
  char strn[512];

  uint numb = getLowestMobNumber(), i;
  const uint highest = getHighestMobNumber();


 // assemble the filename of the shop file

  if (g_readFromSubdirs) 
    strcpy(shopFilename, "shp/");

  if (filename) 
    strcat(shopFilename, filename);
  else 
    strcat(shopFilename, getMainZoneNameStrn());

  strcat(shopFilename, ".shp");

 // open the world file for writing

  if ((shopFile = fopen(shopFilename, "wt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(shopFilename);
    _outtext(" for writing - aborting\n");

    return;
  }

  i = getNumbShopMobs();

  sprintf(strn, "Writing %s - %u shop%s\n", shopFilename, i, plural(i));

  _outtext(strn);

  while (numb <= highest)
  {
    const mobType *mob = findMob(numb);

    if (mob && mob->shopPtr)
      writeShoptoFile(shopFile, mob->shopPtr, numb, false);

    numb++;
  }

  fclose(shopFile);
}
