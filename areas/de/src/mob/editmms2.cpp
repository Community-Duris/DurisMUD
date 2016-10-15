//
//  File: editmms2.cpp   originally part of durisEdit
//
//  Usage: functions for editing misc mob info (level, pos, sex)
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

#include "../fh.h"
#include "../types.h"
#include "../misc/menu.h"
#include "../keys.h"
#include "../graphcon.h"
#include "../mob/mob.h"

extern menu g_mobMisc2Menu;


//
// displayEditMobMisc2Menu : Displays the edit mob miscellany menu
//
//    *mob : mob being edited
//

void displayEditMobMisc2Menu(const mobType *mob)
{
  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "miscellany", false);

  displayMenu(&g_mobMisc2Menu, mob);
}


//
// interpEditMobMisc2Menu : Interprets user input for edit mob flags menu -
//                          returns TRUE if the user hit 'Q'
//
//     ch : user input
//   *mob : mob to edit
//

bool interpEditMobMisc2Menu(const usint ch, mobType *mob)
{
  if (interpretMenu(&g_mobMisc2Menu, mob, ch))
  {
    displayEditMobMisc2Menu(mob);

    return false;
  }
  else

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editMobMisc2 : Edit mob misc #2 - "main" function
//
//   *mob : mob to edit
//

void editMobMisc2(mobType *mob)
{
  usint ch;
  int oldPos = mob->position, oldDefPos = mob->defaultPos,
      oldSex = mob->sex, oldLevel = mob->level;



  displayEditMobMisc2Menu(mob);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      mob->level = oldLevel;
      mob->position = oldPos;
      mob->defaultPos = oldDefPos;
      mob->sex = oldSex;

      _outtext("\n\n");

      return;
    }

    if (interpEditMobMisc2Menu(ch, mob)) 
      return;
  }
}
