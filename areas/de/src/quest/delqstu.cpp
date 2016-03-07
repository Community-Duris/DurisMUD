//
//  File: delqstu.cpp    originally part of durisEdit
//
//  Usage: user-interface functions for deleting quest info from mobs
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
#include <string.h>

#include "types.h"
#include "fh.h"

#include "quest.h"

extern bool g_madeChanges;

//
// deleteQuestUser : Deletes a quest based on info specified by user in args.
//
//   *args : user-entered string
//

void deleteQuestUser(const char *args)
{
  mobType *mob;
  uint numb;
  char strn[512];


  if (!checkForMobWithQuest())
  {
    _outtext("\nNone of the current mob types have a quest.\n\n");

    return;
  }

  if (!strlen(args))
  {
    deleteQuestUserPrompt();

    return;
  }

  if (!strnumer(args))
  {
    _outtext(
"\nError in input - specify vnum of mob from which to delete quest.\n\n");

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

  if (!mob->questPtr)
  {
    sprintf(strn, "\nMob type #%u '%s&n' has no quest info.\n\n", 
            numb, getMobShortName(mob));
    displayColorString(strn);

    return;
  }

  deleteQuest(mob->questPtr, true);
  mob->questPtr = NULL;

  sprintf(strn, "\nQuest info for mob type #%u '%s&n' deleted.\n\n", 
          numb, getMobShortName(mob));
  displayColorString(strn);
}


//
// deleteQuestUserPrompt
//

void deleteQuestUserPrompt(void)
{
  char strn[11] = "";


  _outtext("\n");

  editStrnValSearchableList(strn, 10, "quest vnum to delete", displayQuestList);

  if (strlen(strn))
    deleteQuestUser(strn);
  else 
    _outtext("\n\n");
}
