//
//  File: editmchk.cpp   originally part of durisEdit
//
//  Usage: functions handling 'mob check options' menu
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
#include "vardef.h"
#include "misc/menu.h"

extern variable *g_varHead;
extern bool g_madeChanges;
extern menu g_mobCheckMenu;


//
// displayEditMobCheckMenu : Displays edit config menu
//

void displayEditMobCheckMenu(void)
{
  displaySimpleMenuHeader("mob check options");

  displayMenu(&g_mobCheckMenu, NULL);
}


//
// setAllMobCheckOptions
//

void setAllMobCheckOptions(const bool blnOnOff)
{
  setVarBoolVal(&g_varHead, VAR_CHECKLOADED_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKMOB_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKMOBDESC_NAME, blnOnOff, false);
  setVarBoolVal(&g_varHead, VAR_CHECKGUILDSTUFF_NAME, blnOnOff, false);
}


//
// interpEditMobCheckMenu : Interprets user input for edit check menu - returns
//                       TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//

bool interpEditMobCheckMenu(const usint ch)
{
  if (interpretMenu(&g_mobCheckMenu, NULL, ch))
  {
    displayEditMobCheckMenu();
    return false;
  }
  else

 // turn all check options off

  if (ch == 'Y')
  {
    setAllMobCheckOptions(false);

    displayEditMobCheckMenu();
  }
  else

 // turn all check options on

  if (ch == 'Z')
  {
    setAllMobCheckOptions(true);

    displayEditMobCheckMenu();
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editMobCheck : "main" function for edit config menu
//
//  *config : config being edited
//

void editMobCheck(void)
{
  usint ch;
  variable *oldVar = copyVarList(g_varHead), *vartemp;


  displayEditMobCheckMenu();

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

   // if interpEditMobCheckMenu is TRUE, user has quit

    if (interpEditMobCheckMenu(ch))
    {
      if (!compareVarLists(g_varHead, oldVar))
        g_madeChanges = true;

      deleteVarList(oldVar);

      _outtext("\n\n");
      return;
    }
  }
}
