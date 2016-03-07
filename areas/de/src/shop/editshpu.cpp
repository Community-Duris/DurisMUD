//
//  File: editshpu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing shops from commandline
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
#include "keys.h"

#include "shop.h"

extern bool g_madeChanges;

//
// editShopStrn : Edits a quest based on user input in *strn.
//
//   *strn : user input
//

void editShopStrn(const char *args)
{
  uint numb;
  mobType *mob, *mobCopy;
  char strn[256];


 // get the number section

  if (!strlen(args))
  {
    editShopPrompt();

    return;
  }

  if (!strnumer(args))
  {
    _outtext("\nSpecify the vnum of the mob you want to edit.\n\n");

    return;
  }

  numb = strtoul(args, NULL, 10);
  mob = findMob(numb);

  if (!mob)
  {
    sprintf(strn, "\nMob type #%u not found.\n\n", numb);
    _outtext(strn);

    return;
  }
  else
  {
    if (!mob->shopPtr)
    {
      if (getNumbEntities(ENTITY_MOB, mob->mobNumber, false) > 1)
      {
        _outtext(
"\nThis mob has no shop, but more than one is loaded and there can be only one.\n\n");

        return;
      }

      if (addingShopMakesTwoShopsInRoom(mob->mobNumber))
      {
        _outtext(
"\nThis mob has no shop, but adding one would cause two or more shops in a room.\n\n");

        return;
      }

      if (displayYesNoPrompt("\n&+cThis mob has no shop - create one", promptNo, false) == promptNo)
        return;

      mobCopy = copyMobType(mob, false);

      mob->shopPtr = createShop();

      if (!mob->shopPtr || !mobCopy)   // error
      {
        if (mobCopy)
          deleteMobType(mobCopy, false);

        return;
      }
    }
    else
    {
      mobCopy = copyMobType(mob, false);
    }

    editShop(mob, true);

    if (!compareMobType(mob, mobCopy))
      g_madeChanges = true;

    deleteMobType(mobCopy, false);
  }
}


//
// editShopPrompt : Prompts a user to type in a quest to edit.
//

void editShopPrompt(void)
{
  char strn[11] = "";


  _outtext("\n");

  editStrnValSearchableList(strn, 10, "shop vnum to edit", displayShopList);

  if (strlen(strn))
    editShopStrn(strn);
  else 
    _outtext("\n\n");
}
