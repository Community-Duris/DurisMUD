//
//  File: editqstu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing quests by commandline
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
#include <ctype.h>

#include "types.h"
#include "fh.h"
#include "keys.h"
#include "misc/menu.h"

#include "quest.h"

extern bool g_madeChanges;


//
// editQuestStrn : Edits a quest based on user input in *strn.
//
//   *strn : user input
//

void editQuestStrn(const char *strn)
{
  uint numb;
  mobType *mob, *mobCopy;
  char outstrn[512];


 // get the number section

  if (!strlen(strn))
  {
    editQuestPrompt();

    return;
  }

  if (!strnumer(strn))
  {
    _outtext("\nSpecify the vnum of the mob you want to edit.\n\n");

    return;
  }

  numb = strtoul(strn, NULL, 10);
  mob = findMob(numb);

  if (!mob)
  {
    sprintf(outstrn, "\nMob type #%u not found.\n\n", numb);
    _outtext(outstrn);

    return;
  }
  else
  {
    if (!mob->questPtr)
    {
      if (displayYesNoPrompt("\n&+cThis mob has no quest info - create some", promptNo, false) == promptNo)
        return;

      mobCopy = copyMobType(mob, false);

      mob->questPtr = createQuest();

      if (!mob->questPtr || !mobCopy)  // error
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

    editQuest(mob, true);

    if (!compareMobType(mob, mobCopy))
      g_madeChanges = true;

    deleteMobType(mobCopy, false);
  }
}


//
// editQuestPrompt : Prompts a user to type in a quest to edit.
//

void editQuestPrompt(void)
{
  char strn[11] = "";


  _outtext("\n");

  editStrnValSearchableList(strn, 10, "quest vnum to edit", displayQuestList);

  if (strlen(strn))
    editQuestStrn(strn);
  else 
    _outtext("\n\n");
}
