//
//  File: editqstq.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing actual 'quest' bits of
//         a mob's quest info
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

#include "fh.h"
#include "types.h"
#include "misc/menu.h"
#include "keys.h"

#include "graphcon.h"

#include "editqst.h"
#include "editqstq.h"
#include "quest.h"



extern bool g_madeChanges;
extern menu g_questQuestMenu, g_questDeleteCreateMenu;


//
// displayEditQuestQuestMenu : Displays the edit quest quest menu
//
//   *qst : quest quest being edited
//

void displayEditQuestQuestMenu(const questQuest *qst, const mobType *mob)
{
  char strn[1024], itemstrn[512], ch;
  const questItem *recv = qst->questPlayRecvHead, *give = qst->questPlayGiveHead;
  uint numbShown = 0;


  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "specific quest info", false);

  displayMenuNoFooter(&g_questQuestMenu, qst);

  if (!recv && !give)
  {
    displayColorString("&+w  This quest contains no give/receive information.\n");
  }

 // run through stuff player gives to mob

  while (give)
  {
    if ((numbShown + FIRST_QSTQST_CH) > LAST_QSTQST_CH)
      ch = FIRST_POSTALPHA_CH +
           (char)(numbShown - (LAST_QSTQST_CH - FIRST_QSTQST_CH)) - 1;
    else 
      ch = (char)(numbShown + FIRST_QSTQST_CH);

    sprintf(strn, "   &+Y%c&+L. &+wMob wants %s&n\n",
            ch, getQuestItemStrn(give, QUEST_GIVEITEM, itemstrn));

    displayColorString(strn);

    numbShown++;

    if ((numbShown % MAX_QSTQST_ENTRIES) == 0)
    {
      displayColorString("\n&+CPress a key to continue..");
      getkey();
      _outtext("\n\n");
    }

    give = give->Next;
  }

 // run through stuff mob gives to player

  while (recv)
  {
    if ((numbShown + FIRST_QSTQST_CH) > LAST_QSTQST_CH)
      ch = FIRST_POSTALPHA_CH +
           (char)(numbShown - (LAST_QSTQST_CH - FIRST_QSTQST_CH)) - 1;
    else 
      ch = (char)(numbShown + FIRST_QSTQST_CH);

    sprintf(strn, "   &+Y%c&+L. &+wMob gives %s&n\n",
            ch, getQuestItemStrn(recv, QUEST_RECVITEM, itemstrn));

    displayColorString(strn);

    numbShown++;

    if ((numbShown % MAX_QSTQST_ENTRIES) == 0)
    {
      displayColorString("\n&+CPress a key to continue..");
      getkey();
      _outtext("\n\n");
    }

    recv = recv->Next;
  }

  displayMenu(&g_questDeleteCreateMenu, qst);
}


//
// interpEditQuestQuestMenu : Interpret command user typed - returns TRUE
//                            if the user hits 'Q'
//
//               ch : command typed by user
//   *extraDescNode : node being edited
//

bool interpEditQuestQuestMenu(const usint origch, questQuest *qst, const mobType *mob)
{
  struct rccoord coords;
  uint numbRecv = getNumbItemNodes(qst->questPlayRecvHead),
        numbGive = getNumbItemNodes(qst->questPlayGiveHead);
  questItem *item;


 // delete stuff

  if (origch == 'Y')
  {
    usint ch;

    coords = _gettextposition();

    _settextposition(coords.row, 1);
    clrline(coords.row);

    displayColorString(
"&+cEnter letter of entry to delete (&+CB to delete disappearance info&+c): ");

    do
    {
      ch = toupper(getkey());

      if ((ch == 'X') || (ch == K_Enter) || (ch == K_Escape))
      {
        displayEditQuestQuestMenu(qst, mob);
        return false;
      }

      if ((ch == 'B') && (qst->disappearHead)) break;

      if ((ch >= FIRST_POSTALPHA_CH) && (ch < 'A'))
         ch += (LAST_QSTQST_CH - FIRST_POSTALPHA_CH) + 1;  // it's magic
    } while (!((char)(numbRecv + numbGive) > (ch - FIRST_QSTQST_CH)));

    if ((ch == 'B') && qst->disappearHead)
    {
      deleteStringNodes(qst->disappearHead);
      qst->disappearHead = NULL;

      displayEditQuestQuestMenu(qst, mob);

      return false;
    }
    else

    if ((char)(numbGive + numbRecv) > (ch - FIRST_QSTQST_CH))
    {
      if (((ch - FIRST_QSTQST_CH) <= (char)(numbGive - 1)) && numbGive)  // give
      {
        deleteQuestIteminList(&(qst->questPlayGiveHead),
           getItemNodeNumb(ch - FIRST_QSTQST_CH, qst->questPlayGiveHead));
      }
      else  // receive
      {
        deleteQuestIteminList(&(qst->questPlayRecvHead),
           getItemNodeNumb(ch - FIRST_QSTQST_CH - numbGive,
                           qst->questPlayRecvHead));
      }
    }

    displayEditQuestQuestMenu(qst, mob);
  }
  else

 // create something

  if (origch == 'Z')
  {
    usint ch;

    coords = _gettextposition();

    _settextposition(coords.row, 1);
    clrline(coords.row);

    displayColorString("&+c"
"Create new item given to &+Cm&+cob, given to &+CP&+cC, or e&+CX&+cit (&+Cm/p/X&+c)? ");

    do
    {
      ch = toupper(getkey());
    }
    while ((ch != 'M') && (ch != 'P') && (ch != 'G') && (ch != 'R') &&
           (ch != 'X') && (ch != K_Enter));

    switch (ch)
    {
      case 'G' :  // support old key
      case 'M' :
        item = createQuestItem();
        addQuestItemtoList(&(qst->questPlayGiveHead), item);

        break;

      case 'R' :  // support old key
      case 'P' :
        item = createQuestItem();
        addQuestItemtoList(&(qst->questPlayRecvHead), item);

        break;

      case 'X' :
      case K_Enter :
        break;
    }

    displayEditQuestQuestMenu(qst, mob);
  }
  else

  if (interpretMenu(&g_questQuestMenu, qst, origch))
  {
    displayEditQuestQuestMenu(qst, mob);

    return false;
  }
  else

 // get input, and love it

  if (((origch >= FIRST_QSTQST_CH) || (origch >= FIRST_POSTALPHA_CH)) &&
      (numbRecv || numbGive))
  {
    usint ch = origch;

    if ((ch >= FIRST_POSTALPHA_CH) && (ch < 'A'))
      ch += (LAST_QSTQST_CH - FIRST_POSTALPHA_CH) + 1;  // it's magic

    if ((char)(numbRecv + numbGive) > (ch - FIRST_QSTQST_CH))
    {
      // edit one or the other

      if (((ch - FIRST_QSTQST_CH) <= (char)(numbGive - 1)) && numbGive)
                                         // edit items given to mob
      {
        editQuestItem(getItemNodeNumb(ch - FIRST_QSTQST_CH,
                       qst->questPlayGiveHead), mob,
                       QUEST_GIVEITEM);

        displayEditQuestQuestMenu(qst, mob);
      }
      else
      {
        editQuestItem(getItemNodeNumb(ch - FIRST_QSTQST_CH - numbGive,
                       qst->questPlayRecvHead), mob,
                       QUEST_RECVITEM);

        displayEditQuestQuestMenu(qst, mob);
      }
    }
  }
  else

  if ((checkMenuKey(origch, false) == MENUKEY_SAVE) && (origch != 'Q')) 
    return true;

  return false;
}


//
// editQuestQuest : Edit a 'quest quest'..  More accurately, the "main" menu
//                  function for the menu to do just that.  Returns TRUE if
//                  changes have been made.
//
//     *qst : quest quest being edited
//

bool editQuestQuest(questQuest *qst, const mobType *mob)
{
  usint ch;
  questQuest *newQst;


  newQst = copyQuestQuest(qst);
  if (!newQst)
  {
    displayAllocError("questQuest", "editQuestQuest");

    return false;
  }

 // display the menu

  displayEditQuestQuestMenu(newQst, mob);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      deleteQuestQuest(newQst);

      _outtext("\n\n");

      return false;
    }


   // if interpEditQuestQuestMenu is TRUE, user has quit (hit 'Q')

    if (interpEditQuestQuestMenu(ch, newQst, mob))
    {

     // see if changes have been made in the message

      if (!compareQuestQuest(qst, newQst))
      {
        deleteStringNodes(qst->questReplyHead);
        deleteQuestItemList(qst->questPlayRecvHead);
        deleteQuestItemList(qst->questPlayGiveHead);
        deleteStringNodes(qst->disappearHead);

        memcpy(qst, newQst, sizeof(questQuest));

        delete newQst;

        g_madeChanges = true;

        _outtext("\n\n");

        return true;
      }
      else
      {
        deleteQuestQuest(newQst);

        _outtext("\n\n");

        return false;
      }
    }
  }
}
