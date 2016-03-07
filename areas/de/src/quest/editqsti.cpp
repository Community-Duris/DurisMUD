//
//  File: editqsti.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing 'items' in mob quests
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
#include <ctype.h>

#include "types.h"
#include "fh.h"
#include "misc/menu.h"

#include "graphcon.h"

#include "quest.h"

extern menu questItemGivenToMobMenu, g_questItemGivenToPCMenu;
extern menuChoice questItemGivenToMobItemTypeChoiceArr[], questItemGivenToPCItemTypeChoiceArr[],
                  questItemGivenToMobItemValueChoiceArr[], questItemGivenToPCItemValueChoiceArr[];


//
// displayEditQuestItemMenu : Displays the edit quest item menu
//
//    *item : item being edited
//

void displayEditQuestItemMenu(const questItem *item, const mobType *mob, const char giveRecv)
{
  const char *extraStrn;
  char strn[256];


  if (giveRecv == QUEST_GIVEITEM)
    extraStrn = "";
  else
    extraStrn = "PC by ";

  sprintf(strn, "quest item given to %smob #&+c%u&+w,",
          extraStrn, mob->mobNumber);

  displayMenuHeaderBasic(strn, getMobShortName(mob));

  if (giveRecv == QUEST_GIVEITEM)
    displayMenu(&questItemGivenToMobMenu, item);
  else
    displayMenu(&g_questItemGivenToPCMenu, item);
}


//
// interpEditQuestItemMenu : Interprets user input for edit quest item menu -
//                           returns TRUE if the user hit 'Q'
//
//     ch : user input
//   *item : item to edit
//

bool interpEditQuestItemMenu(const usint ch, questItem *item, const mobType *mob, const char giveRecv)
{
  const menu* menuPtr;

  if (giveRecv == QUEST_GIVEITEM)
    menuPtr = &questItemGivenToMobMenu;
  else
    menuPtr = &g_questItemGivenToPCMenu;

  if (interpretMenu(menuPtr, item, ch))
  {
    displayEditQuestItemMenu(item, mob, giveRecv);
    return false;
  }
  else

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
  {
    return true;
  }

  return false;
}


//
// editQuestItem : Edit item misc - "main" function
//
//   *item : item to edit
//

void editQuestItem(questItem *item, const mobType *mob, const char giveRecv)
{
  usint ch;
  questItem oldItem;


  questItemGivenToMobItemTypeChoiceArr[0].extraPtr = (void *)mob;
  questItemGivenToPCItemTypeChoiceArr[0].extraPtr = (void *)mob;
  questItemGivenToMobItemValueChoiceArr[0].extraPtr = (void *)mob;
  questItemGivenToPCItemValueChoiceArr[0].extraPtr = (void *)mob;

 // back up the old stuff

  memcpy(&oldItem, item, sizeof(questItem));


  displayEditQuestItemMenu(item, mob, giveRecv);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      memcpy(item, &oldItem, sizeof(questItem));

      _outtext("\n\n");

      return;
    }

    if (interpEditQuestItemMenu(ch, item, mob, giveRecv)) 
      return;
  }
}
