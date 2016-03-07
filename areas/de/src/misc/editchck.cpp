//
//  File: editchck.cpp   originally part of durisEdit
//
//  Usage: functions for handling main 'checkconfig' menu
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

#include "types.h"
#include "fh.h"
#include "misc/menu.h"

#include "graphcon.h"
#include "command/var.h"
#include "vardef.h"

extern variable *g_varHead;
extern bool g_madeChanges;
extern menu g_checkMenu;


//
// displayEditCheckMenu : Displays edit config menu
//

void displayEditCheckMenu(void)
{
  displaySimpleMenuHeader("check options");

  displayMenu(&g_checkMenu, NULL);
}


//
// setAllCheckOptions : handy
//

void setAllCheckOptions(const bool blnOnOff)
{
  setVarBoolVal(&g_varHead, VAR_CHECKSAVE_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_NOSAVEONCHECKERR_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SAVECHECKLOG_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_PAUSECHECKSCREENFUL_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKLONEROOM_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKMISSINGKEYS_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKLOADED_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKROOM_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKEXIT_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKEXITDESC_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKOBJ_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKOBJDESC_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKOBJVAL_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKGUILDSTUFF_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKMOB_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKMOBDESC_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKEDESC_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKOBJMATERIAL_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKFLAGS_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKZONE_NAME, blnOnOff, false);
}


//
// interpEditCheckMenu : Interprets user input for edit check menu - returns
//                       TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//

bool interpEditCheckMenu(const usint ch)
{

 // room/exit check options

  if (ch == 'A')
  {
    editRoomCheck();

    displayEditCheckMenu();
  }
  else

 // obj check options

  if (ch == 'B')
  {
    editObjCheck();

    displayEditCheckMenu();
  }
  else

 // mob check options

  if (ch == 'C')
  {
    editMobCheck();

    displayEditCheckMenu();
  }
  else

 // misc check options

  if (ch == 'D')
  {
    editMiscCheck();

    displayEditCheckMenu();
  }
  else

  if (interpretMenu(&g_checkMenu, NULL, ch))
  {
    displayEditCheckMenu();

    return false;
  }
  else

 // turn all check options off

  if (ch == 'Y')
  {
    setAllCheckOptions(false);

    displayEditCheckMenu();
  }
  else

 // turn all check options on

  if (ch == 'Z')
  {
    setAllCheckOptions(true);

    displayEditCheckMenu();
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editCheck : "main" function for edit config menu
//
//  *config : config being edited
//

void editCheck(void)
{
  usint ch;
  variable *oldVar = copyVarList(g_varHead), *vartemp;


  displayEditCheckMenu();

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
      } else deleteVarList(oldVar);

      _outtext("\n\n");

      return;
    }

   // if interpEditCheckMenu is TRUE, user has quit

    if (interpEditCheckMenu(ch))
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
