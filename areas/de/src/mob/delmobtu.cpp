//
//  File: delmobtu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for deleting mob types
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
#include <string.h>
#include <stdlib.h>

#include "../types.h"
#include "../fh.h"
#include "../keys.h"

#include "mob.h"

extern bool g_madeChanges;


//
// checkDeleteMobsofType : returns TRUE if user doesn't abort - prompts user
//                         if they really want to delete mob type because mobHeres
//                         of this type are loaded
//

bool checkDeleteMobsofType(const mobType *mobType)
{
  if (checkForMobHeresofType(mobType))
  {
    if (displayYesNoPrompt("\n&+cMobs exist of this type and will have to be deleted - continue", promptNo, 
                           false) == promptNo)
    {
      return false;
    }
    else
    {
      return true;
    }
  }
  else 
  {
    return true;
  }
}


//
// deleteMobTypeUser : Deletes a mobType based on info specified by user
//                     in args.
//
//   args : user-entered string
//

void deleteMobTypeUser(const char *args)
{
  char outStrn[512];
  mobType *mob;
  uint numb;


  if (!strlen(args))
  {
    deleteMobTypeUserPrompt();
    return;
  }

  if (!strnumber(args))
  {
    _outtext("\nError in input - specify vnum of mob type.\n\n");
    return;
  }

  numb = strtoul(args, NULL, 10);
  mob = findMob(numb);

  if (!mob)
  {
    sprintf(outStrn, "\nMob type #%u not found.\n\n", numb);
    _outtext(outStrn);

    return;
  }

  if (!checkDeleteMobsofType(mob)) 
    return;

  deleteAllMobHeresofType(mob, true);
  deleteMobsinInv(mob);

  sprintf(outStrn, "\nMob type #%u '%s&n' deleted.\n\n", numb, getMobShortName(mob));
  displayColorString(outStrn);

  deleteMobType(mob, true);
}


//
// deleteMobTypeUserPrompt
//

void deleteMobTypeUserPrompt(void)
{
  char strn[11] = "";


  _outtext("\n");

  editStrnValSearchableList(strn, 10, "mob vnum to delete", displayMobTypeList);

  if (strlen(strn)) 
    deleteMobTypeUser(strn);
  else 
    _outtext("\n\n");
}
