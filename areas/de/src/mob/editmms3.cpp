//
//  File: editmms3.cpp   originally part of durisEdit
//
//  Usage: functions for user-editing of misc mob info (thac0, money, hp, dam)
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
#include "misc/menu.h"
#include "graphcon.h"
#include "mob.h"

extern menu g_mobMisc3Menu;


//
// displayEditMobMisc3Menu : Displays the edit mob miscellany menu
//
//    *mob : mob being edited
//

void displayEditMobMisc3Menu(const mobType *mob)
{
  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, "miscellany", false);

  displayMenu(&g_mobMisc3Menu, mob);
}


//
// interpEditMobMisc3Menu : Interprets user input for edit mob flags menu -
//                          returns TRUE if the user hit 'Q'
//
//     ch : user input
//   *mob : mob to edit
//

bool interpEditMobMisc3Menu(const usint ch, mobType *mob)
{
  if (interpretMenu(&g_mobMisc3Menu, mob, ch))
  {
    displayEditMobMisc3Menu(mob);

    return false;
  }
  else

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// editMobMisc3 : Edit mob misc #3 - "main" function
//
//   *mob : mob to edit
//

void editMobMisc3(mobType *mob)
{
  const uint oldC = mob->copper, oldS = mob->silver, oldG = mob->gold, oldP = mob->platinum;

  const int oldTHAC0 = mob->thac0;
  const int oldAC = mob->ac;

  char oldHP[MAX_MOBHP_LEN + 1];
  char oldDmg[MAX_MOBDAM_LEN + 1];



  strcpy(oldHP, mob->hitPoints);
  strcpy(oldDmg, mob->mobDamage);

  displayEditMobMisc3Menu(mob);

  while (true)
  {
    const usint ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
      mob->copper = oldC;
      mob->silver = oldS;
      mob->gold = oldG;
      mob->platinum = oldP;

      mob->thac0 = oldTHAC0;
      mob->ac = oldAC;

      strcpy(mob->hitPoints, oldHP);
      strcpy(mob->mobDamage, oldDmg);

      _outtext("\n\n");

      return;
    }

    if (interpEditMobMisc3Menu(ch, mob)) 
      return;
  }
}
