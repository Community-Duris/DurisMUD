//
//  File: editmobu.cpp   originally part of durisEdit
//
//  Usage: calls editMobType() functions based on user input from
//         command-line
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

#include "fh.h"
#include "types.h"

#include "mob.h"

//
// editMobTypeStrn : Edits a mob type based on user input in strn
//
//   *strn : user input
//

void editMobTypeStrn(const char *args)
{
  mobType *mob;
  char strn[256];
  uint numb;


  if (!strlen(args))
  {
    editMobTypePrompt();

    return;
  }

  if (!strnumer(args))
  {
    _outtext("\nSpecify the vnum of the mob you want to edit.\n\n");

    return;
  }

  numb = strtoul(args, NULL, 10);
  mob = findMob(numb);

  if (!mob)
  {
    sprintf(strn, "\nMob type #%u not found.\n\n", numb);
    _outtext(strn);
  }
  else 
  {
    editMobType(mob, true);
  }
}


//
// editMobTypePrompt : Prompts a user to type in a mob type id to edit.
//

void editMobTypePrompt(void)
{
  char strn[11] = "";


  _outtext("\n");

  editStrnValSearchableList(strn, 10, "mob vnum to edit", displayMobTypeList);

  if (strlen(strn) == 0)
    _outtext("\n\n");
  else
    editMobTypeStrn(strn);
}
