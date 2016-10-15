//
//  File: editrchk.cpp   originally part of durisEdit
//
//  Usage: functions for handling 'room check options' menu
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
#include <ctype.h>

#include "../fh.h"
#include "../types.h"
#include "../boolean.h"

#include "../misc/menu.h"
#include "../vardef.h"

extern variable *g_varHead;
extern bool g_madeChanges;
extern menu g_roomCheckMenu;


//
// displayEditRoomCheckMenu : Displays edit config menu
//

void displayEditRoomCheckMenu(void)
{
  displaySimpleMenuHeader("room/exit checking options");

  displayMenu(&g_roomCheckMenu, NULL);
}


//
// setAllRoomCheckOptions
//

void setAllRoomCheckOptions(const bool blnYesNo)
{
  setVarBoolVal(&g_varHead, VAR_CHECKLONEROOM_NAME, blnYesNo, false);
  setVarBoolVal(&g_varHead, VAR_CHECKMISSINGKEYS_NAME, blnYesNo, false);
  setVarBoolVal(&g_varHead, VAR_CHECKROOM_NAME, blnYesNo, false);
  setVarBoolVal(&g_varHead, VAR_CHECKEXIT_NAME, blnYesNo, false);
  setVarBoolVal(&g_varHead, VAR_CHECKEXITDESC_NAME, blnYesNo, false);
}


//
// interpEditRoomCheckMenu : Interprets user input for edit check menu - returns
//                           TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//

bool interpEditRoomCheckMenu(const usint ch)
{
  if (interpretMenu(&g_roomCheckMenu, NULL, ch))
  {
    displayEditRoomCheckMenu();
    return false;
  }
  else

 // turn all check options off

  if (ch == 'Y')
  {
    setAllRoomCheckOptions(false);

    displayEditRoomCheckMenu();
  }
  else

 // turn all check options on

  if (ch == 'Z')
  {
    setAllRoomCheckOptions(true);

    displayEditRoomCheckMenu();
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editRoomCheck : "main" function for edit room config menu
//
//  *config : config being edited
//

void editRoomCheck(void)
{
  usint ch;
  variable *oldVar = copyVarList(g_varHead), *vartemp;


  displayEditRoomCheckMenu();

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
     // only fragment heap if necessary

      if (!compareVarLists(g_varHead, oldVar))
      {
        vartemp = g_varHead;
        g_varHead = oldVar;

        deleteVarList(vartemp);
      } 
      else 
      {
        deleteVarList(oldVar);
      }

      _outtext("\n\n");

      return;
    }

   // if interpEditRoomCheckMenu is TRUE, user has quit

    if (interpEditRoomCheckMenu(ch))
    {
      if (!compareVarLists(g_varHead, oldVar))
      {
        g_madeChanges = true;
      }

      deleteVarList(oldVar);

      _outtext("\n\n");
      return;
    }
  }
}
