//
//  File: editdisp.cpp   originally part of durisEdit
//
//  Usage: functions for handling 'displayconfig' menu
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

#include "fh.h"
#include "types.h"
#include "vardef.h"
#include "misc/menu.h"

extern variable *g_varHead;
extern bool g_madeChanges;
extern menu g_dispMenu;


//
// displayEditDisplayMenu : Displays edit config menu
//

void displayEditDisplayMenu(void)
{
  displaySimpleMenuHeader("display options");

  displayMenu(&g_dispMenu, NULL);
}


//
// setAllDisplayOptions
//

void setAllDisplayOptions(const bool blnOnOff)
{
  setVarBoolVal(&g_varHead, VAR_SHOWROOMEXTRA_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWROOMVNUM_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWEXITFLAGS_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWEXITDEST_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWCONTENTS_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWOBJFLAGS_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWOBJVNUM_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWMOBFLAGS_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWMOBPOS_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWMOBVNUM_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_SHOWMOBRIDEFOLLOW_NAME, blnOnOff, false);
}


//
// interpEditDisplayMenu : Interprets user input for edit config menu - returns
//                         TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//

bool interpEditDisplayMenu(const usint ch)
{
  if (interpretMenu(&g_dispMenu, NULL, ch))
  {
    displayEditDisplayMenu();
    return false;
  }
  else

 // turn all display options off

  if (ch == 'Y')
  {
    setAllDisplayOptions(false);

    displayEditDisplayMenu();
  }
  else

 // turn all display options on

  if (ch == 'Z')
  {
    setAllDisplayOptions(true);

    displayEditDisplayMenu();
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editDisplay : "main" function for edit config menu
//
//  *config : config being edited
//

void editDisplay(void)
{
  usint ch;
  variable *oldVar = copyVarList(g_varHead), *vartemp;


  displayEditDisplayMenu();

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

   // if interpEditDisplayMenu is TRUE, user has quit

    if (interpEditDisplayMenu(ch))
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
