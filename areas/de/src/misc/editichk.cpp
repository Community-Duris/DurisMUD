//
//  File: editichk.cpp   originally part of durisEdit
//
//  Usage: functions for handling 'misc check options' menu
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
extern menu g_miscCheckMenu;


//
// displayEditMiscCheckMenu : Displays edit config menu
//

void displayEditMiscCheckMenu(void)
{
  displaySimpleMenuHeader("miscellaneous check options");

  displayMenu(&g_miscCheckMenu, NULL);
}


//
// setAllMiscCheckOptions
//

void setAllMiscCheckOptions(const bool blnOnOff)
{
  setVarBoolVal(&g_varHead, VAR_CHECKEDESC_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKFLAGS_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKZONE_NAME, blnOnOff, false);
}


//
// interpEditMiscCheckMenu : Interprets user input for edit check menu - returns
//                       TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//

bool interpEditMiscCheckMenu(const usint ch)
{
  if (interpretMenu(&g_miscCheckMenu, NULL, ch))
  {
    displayEditMiscCheckMenu();
    return false;
  }
  else

 // turn all check options off

  if (ch == 'Y')
  {
    setAllMiscCheckOptions(false);

    displayEditMiscCheckMenu();
  }
  else

 // turn all check options on

  if (ch == 'Z')
  {
    setAllMiscCheckOptions(true);

    displayEditMiscCheckMenu();
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editMiscCheck : "main" function for edit config menu
//
//  *config : config being edited
//

void editMiscCheck(void)
{
  usint ch;
  variable *oldVar = copyVarList(g_varHead), *vartemp;


  displayEditMiscCheckMenu();

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

   // if interpEditMiscCheckMenu is TRUE, user has quit

    if (interpEditMiscCheckMenu(ch))
    {
      if (!compareVarLists(g_varHead, oldVar))
        g_madeChanges = true;

      deleteVarList(oldVar);

      _outtext("\n\n");
      return;
    }
  }
}
