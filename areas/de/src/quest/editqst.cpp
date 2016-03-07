//
//  File: editqst.cpp    originally part of durisEdit
//
//  Usage: user-interface functions for editing quest info
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

#include <ctype.h>

#include "fh.h"
#include "types.h"
#include "misc/menu.h"
#include "keys.h"

#include "graphcon.h"

#include "editqst.h"


extern bool g_madeChanges;
extern uint g_numbLookupEntries;
extern menu g_questDeleteCreateMenu;

extern void updateQuestItems();

//
// displayEditQuestMenu : Displays edit quest menu
//
//   *room : room being edited
//

void displayEditQuestMenu(const quest *qst, const mobType *mob)
{
  char strn[1536], keystrn[1024], ch;
  const questMessage *msg = qst->messageHead;
  const questQuest *qstqst = qst->questHead;
  uint numbShown = 0;


  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "quest info", false);

  if (!msg && !qstqst)
  {
    displayColorString("&+w  This quest contains no information.\n");
  }

  while (msg)
  {
    if ((numbShown + 'A') > LAST_QUEST_CH)
      ch = FIRST_POSTALPHA_CH + (char)(numbShown - (LAST_QUEST_CH - 'A')) - 1;
    else 
      ch = (char)numbShown + 'A';

    sprintf(strn, "   &+Y%c&+L. &+wResponse to \"%s\"\n",
            ch, getReadableKeywordStrn(msg->keywordListHead, keystrn, 1023));

    displayColorString(strn);

    numbShown++;

    if ((numbShown % MAX_ENTRIES) == 0)
    {
      displayColorString("\n&+CPress a key to continue..");
      getkey();
      _outtext("\n\n");
    }

    msg = msg->Next;
  }

  while (qstqst)
  {
    if ((numbShown + 'A') > LAST_QUEST_CH)
      ch = FIRST_POSTALPHA_CH + (char)(numbShown - (LAST_QUEST_CH - 'A')) - 1;
    else 
      ch = (char)numbShown + 'A';

    sprintf(strn, "   &+Y%c&+L. &+wQuest for \"%s\"\n",
            ch, getQuestRecvString(qstqst->questPlayRecvHead, keystrn, 1024));

    displayColorString(strn);

    numbShown++;

    if ((numbShown % MAX_ENTRIES) == 0)
    {
      displayColorString("\n&+CPress a key to continue..");
      getkey();
      _outtext("\n\n");
    }

    qstqst = qstqst->Next;
  }

  displayMenu(&g_questDeleteCreateMenu, qst);
}


//
// interpEditQuestMenu : Interprets user input for edit quest menu - returns
//                       TRUE if the user hits 'Q', QUEST_DELETED if user deleted,
//                       false otherwise
//
//     ch : user input
//   *qst : quest to edit
//

char interpEditQuestMenu(const usint origch, quest *qst, mobType *mob)
{
  struct rccoord coords;
  uint numbMsgs = getNumbMessageNodes(qst->messageHead),
        numbQsts = getNumbQuestNodes(qst->questHead);

  questMessage *msg;
  questQuest *qstQst;


 // delete stuff

  if (origch == 'Y')
  {
    usint ch;

    coords = _gettextposition();

    _settextposition(coords.row, 1);
    clrline(coords.row);

    displayColorString(
"&+cEnter letter of entry to delete (&+CY to delete entire quest&+c): &n");

    do
    {
      ch = toupper(getkey());

      if (ch == 'Y')
      {
        deleteQuest(qst, false);
        mob->questPtr = NULL;
        _outtext("entire quest\n\n");
        return QUEST_DELETED;
      }

      if ((ch == 'X') || (ch == K_Escape) || (ch == K_Enter))
      {
        displayEditQuestMenu(qst, mob);
        return false;
      }

      if ((ch >= FIRST_POSTALPHA_CH) && (ch < 'A'))
         ch += (LAST_QUEST_CH - FIRST_POSTALPHA_CH) + 1;  // it's magic
    } while (!((usint)(numbMsgs + numbQsts) > (ch - 'A')));

    if ((usint)(numbMsgs + numbQsts) > (ch - 'A'))
    {
      if (((ch - 'A') <= (usint)(numbMsgs - 1)) && numbMsgs) // messages
      {
        deleteQuestMessageinList(&(qst->messageHead),
           getMessageNodeNumb(ch - 'A', qst->messageHead));
      }
      else  // quests
      {
        deleteQuestQuestinList(&(qst->questHead),
           getQuestNodeNumb(ch - 'A' - numbMsgs, qst->questHead));
      }
    }

    displayEditQuestMenu(qst, mob);
  }
  else

 // create something

  if (origch == 'Z')
  {
    usint ch;

    coords = _gettextposition();

    _settextposition(coords.row, 1);
    clrline(coords.row);

    displayColorString(
"&+cCreate new &+Cm&+cessage, new &+Cq&+cuest entry, or e&+CX&+cit (&+Cm/q/X&+c)? ");

    do
    {
      ch = toupper(getkey());
    }
    while ((ch != 'M') && (ch != 'Q') && (ch != 'X') && (ch != K_Enter) &&
           (ch != K_Escape));

    switch (ch)
    {
      case 'M' :
        msg = createQuestMessage();
        addQuestMessagetoList(&(qst->messageHead), msg);

        break;

      case 'Q' :
        qstQst = createQuestQuest();
        addQuestQuesttoList(&(qst->questHead), qstQst);

        break;

      case 'X' :
      case K_Escape:
      case K_Enter :
        break;
    }

    displayEditQuestMenu(qst, mob);
  }
  else

 // get input

  if (((origch >= 'A') || (origch >= FIRST_POSTALPHA_CH)) && (numbMsgs || numbQsts))
  {
    usint ch = origch;

    if ((ch >= FIRST_POSTALPHA_CH) && (ch < 'A'))
      ch += (LAST_QUEST_CH - FIRST_POSTALPHA_CH) + 1;  // it's magic

    if ((usint)(numbMsgs + numbQsts) > (ch - 'A'))
    {
      // edit one or the other

      if (((ch - 'A') <= (usint)(numbMsgs - 1)) && numbMsgs)  // edit messages
      {
        editQuestMessage(getMessageNodeNumb(ch - 'A', qst->messageHead));
        displayEditQuestMenu(qst, mob);
      }
      else  // edit quests
      {
        editQuestQuest(getQuestNodeNumb(ch - 'A' - numbMsgs, qst->questHead), mob);
        displayEditQuestMenu(qst, mob);
      }
    }
  }
  else

 // quit

  if (checkMenuKey(origch, false) == MENUKEY_SAVE)
    return true;

  return false;
}


//
// realEditQuest : edit a quest
//

mobType *realEditQuest(quest *qst, mobType *mob, const bool allowJump)
{
  usint ch;
  quest *newQuest;
  char done = false;  // can't be bool, interp() returns special code
  const uint mobNumb = mob->mobNumber;


  if( !qst )
  {
    return NULL;
  }

  // copy qst into newQuest and display the menu
  newQuest = copyQuest(qst);
  if( !newQuest )
  {
    displayAllocError("quest", "realEditQuest");

    return NULL;
  }

  displayEditQuestMenu(newQuest, mob);

  while( TRUE )
  {
    ch = toupper(getkey());

    if( checkMenuKey(ch, false) == MENUKEY_ABORT )
    {
      deleteQuest(newQuest, false);

      _outtext("\n\n");
      updateQuestItems();
      return NULL;
    }

    if( allowJump )
    {
      if( (checkMenuKey(ch, false) == MENUKEY_NEXT) || (checkMenuKey(ch, false) == MENUKEY_PREV) ||
          (checkMenuKey(ch, false) == MENUKEY_JUMP) )
      {
        done = TRUE;
      }
    }

   // if interpEditQuestMenu is TRUE, user has quit
    if( !done )
    {
      done = interpEditQuestMenu(ch, newQuest, mob);
    }

    if( done == QUEST_DELETED )
    {
      updateQuestItems();
      return NULL;
    }

    if( done )
    {
      if (!compareQuestInfo(qst, newQuest))   // changes have been made
      {
        deleteQuestAssocLists(qst);

        memcpy(qst, newQuest, sizeof(quest));

        delete newQuest;

        g_madeChanges = TRUE;
        updateQuestItems();
      }
      else
      {
        deleteQuest(newQuest, false);
      }

      if( allowJump )
      {
        if( checkMenuKey(ch, false) == MENUKEY_JUMP )
        {
          uint numb = mob->mobNumber;

          switch (jumpQuest(&numb))
          {
            case MENU_JUMP_ERROR : return mob;
            case MENU_JUMP_VALID : return findMob(numb);

            default : return NULL;
          }
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_NEXT)
        {
          if (mobNumb != getHighestQuestMobNumber())
            return getNextQuestMob(mobNumb);
          else
            return mob;
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_PREV)
        {
          if (mobNumb != getLowestQuestMobNumber())
            return getPrevQuestMob(mobNumb);
          else
            return mob;
        }
      }

      _outtext("\n\n");

      return NULL;
    }
  }
}


//
// editQuest
//

void editQuest(mobType *mob, const bool allowJump)
{
  do
  {
    mob = realEditQuest(mob->questPtr, mob, allowJump);
  } while (mob);
}


//
// jumpQuest
//

char jumpQuest(uint *numb)
{
  char promptstrn[128];
  const struct rccoord coords = _gettextposition();

  clrline(coords.row, 7, 0);
  _settextposition(coords.row, 1);

  sprintf(promptstrn, "quest vnum to jump to [%u-%u]", 
          getLowestQuestMobNumber(), getHighestQuestMobNumber());

  editUIntValSearchableList(numb, false, promptstrn, displayQuestList);

  if ((*numb >= g_numbLookupEntries) || !questExists(*numb))
    return MENU_JUMP_ERROR;
  else
    return MENU_JUMP_VALID;
}
