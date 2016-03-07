//
//  File: config.cpp     originally part of durisEdit
//
//  Usage: functions for handling 'config' menu, containing misc options
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

#include "fh.h"
#include "types.h"
#include "vardef.h"
#include "system.h"
#include "menu.h"
#include "graphcon.h"

extern bool g_madeChanges;
extern variable *g_varHead;
extern menu g_configMenu;


//
// displayEditConfigMenu : Displays edit config menu
//

void displayEditConfigMenu(void)
{
  displaySimpleMenuHeader("miscellaneous config options");

  displayMenu(&g_configMenu, NULL);
}


//
// interpEditConfigMenu : Interprets user input for edit config menu - returns
//                        TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//

bool interpEditConfigMenu(const usint ch)
{
 // edit 'when to autosave' variable

  if (ch == 'I')
  {
    uint val = getSaveHowOftenVal();
    char tempStrn[32];

    editUIntVal(&val, true, "&+CNew number of commands between autosaving: ");

    sprintf(tempStrn, "%u", val);

    addVar(&g_varHead, VAR_SAVEHOWOFTEN_NAME, tempStrn);

    displayEditConfigMenu();
  }
  else

 // edit screen height

  if (ch == 'J')
  {
    uint val = getScreenHeight();

    editUIntVal(&val, false, "&+CNew screen height: ");

    if (val >= MINIMUM_SCREEN_HEIGHT)
    {
      char tempStrn[32];

      sprintf(tempStrn, "%u", val);

      addVar(&g_varHead, VAR_SCREENHEIGHT_NAME, tempStrn);
    }

    displayEditConfigMenu();
  }
  else

 // edit screen width

  if (ch == 'K')
  {
    uint val = getScreenWidth();

    editUIntVal(&val, false, "&+CNew screen width: ");

    if (val >= MINIMUM_SCREEN_WIDTH)
    {
      char tempStrn[32];

      sprintf(tempStrn, "%u", val);

      addVar(&g_varHead, VAR_SCREENWIDTH_NAME, tempStrn);
    }

    displayEditConfigMenu();
  }
  else

 // edit name of external editor

  if (ch == 'L')
  {
    char tempStrn[MAX_VARVAL_LEN];

    strcpy(tempStrn, getEditorName());

    editStrnVal(tempStrn, MAX_VARVAL_LEN - 1, "&+CNew external editor (full path, if necessary): ");

    addVar(&g_varHead, VAR_TEXTEDIT_NAME, tempStrn);

    displayEditConfigMenu();
  }
  else

 // edit name of menu edit prompt

  if (ch == 'M')
  {
    char tempStrn[MAX_VARVAL_LEN];

    strcpy(tempStrn, getMenuPromptName());

    editStrnVal(tempStrn, MAX_VARVAL_LEN - 1, "&+CNew prompt for menus: ");

    if ((strlen(tempStrn) < (MAX_VARVAL_LEN - 1 - 2)) &&
        !strright(tempStrn, "&n") && !strright(tempStrn, "&N"))
      strcat(tempStrn, "&n");

    addVar(&g_varHead, VAR_MENUPROMPT_NAME, tempStrn);

    displayEditConfigMenu();
  }
  else

 // edit name of main prompt

  if (ch == 'N')
  {
    char tempStrn[MAX_VARVAL_LEN];

    strcpy(tempStrn, getMainPromptStrn());

    editStrnVal(tempStrn, MAX_VARVAL_LEN - 1, "&+CNew main prompt: ");

    if ((strlen(tempStrn) < (MAX_VARVAL_LEN - 1 - 2)) &&
        !strrighti(tempStrn, "&n") && !strcmpnocase(tempStrn, "default")) 
      strcat(tempStrn, "&n");

    addVar(&g_varHead, VAR_MAINPROMPT_NAME, tempStrn);
    createPrompt();

    displayEditConfigMenu();
  }
  else

 // everything else

  if (interpretMenu(&g_configMenu, NULL, ch))
  {
    displayEditConfigMenu();
    return false;
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editConfig : "main" function for edit config menu
//
//  *config : config being edited
//

void editConfig(void)
{
  usint ch;
  variable *oldVar = copyVarList(g_varHead), *vartemp;


  displayEditConfigMenu();

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

   // if interpEditConfigMenu is TRUE, user has quit

    if (interpEditConfigMenu(ch))
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
