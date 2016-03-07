//
//  File: editqstm.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing mob quest messages
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

#include <stdio.h>
#include <string.h>

#include "types.h"
#include "fh.h"
#include "misc/menu.h"

#include "graphcon.h"

#include "quest.h"



extern bool g_madeChanges;
extern menu g_questMessageMenu;


//
// displayEditQuestMessageMenu : Displays the edit extra desc menu
//
//   *extraDescNode : extra desc being edited
//

void displayEditQuestMessageMenu(const questMessage *msg)
{
  char strn[1536], keywords[1024];


 // display the menu header

 // turn the keyword list into something that's readable by mere mortals

  getReadableKeywordStrn(msg->keywordListHead, keywords, 1023);

  sprintf(strn, "quest message &+L\"&n%s&+L\"", keywords);

  displaySimpleMenuHeader(strn);

  displayMenu(&g_questMessageMenu, msg);
}


//
// interpEditQuestMessageMenu : Interpret command user typed - returns TRUE
//                              if the user hits 'Q'
//
//               ch : command typed by user
//   *extraDescNode : node being edited
//

bool interpEditQuestMessageMenu(const usint ch, questMessage *msg)
{
  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  if (interpretMenu(&g_questMessageMenu, msg, ch))
  {
    displayEditQuestMessageMenu(msg);
    return false;
  }

  return false;
}


//
// editQuestMessage : Edit a quest msg..  More accurately, the "main" menu
//                    function for the menu to do just that.  Returns TRUE if
//                    changes have been made.
//
//     *msg : quest message being edited
//

bool editQuestMessage(questMessage *msg)
{
  char ch;
  questMessage *newMsg;


  newMsg = copyQuestMessage(msg);
  if (!newMsg)
  {
    displayAllocError("questMessage", "editQuestMessage");

    return false;
  }

 // display the menu

  displayEditQuestMessageMenu(newMsg);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      deleteQuestMessage(newMsg);

      _outtext("\n\n");

      return false;
    }


   // if interpEditQuestMessageMenu is TRUE, user has quit (hit 'Q')

    if (interpEditQuestMessageMenu(ch, newMsg))
    {

     // see if changes have been made in the message

      if (!compareQuestMessage(msg, newMsg))
      {
        deleteStringNodes(msg->keywordListHead);
        deleteStringNodes(msg->questMessageHead);

        memcpy(msg, newMsg, sizeof(questMessage));

        delete newMsg;

        g_madeChanges = true;

        _outtext("\n\n");

        return true;
      }
      else
      {
        deleteQuestMessage(newMsg);

        _outtext("\n\n");

        return false;
      }
    }
  }
}
