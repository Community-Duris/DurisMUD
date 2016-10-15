//
//  File: crtmobhu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for creating mobHeres
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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../fh.h"
#include "../types.h"

extern room *g_currentRoom;

//
// createMobHereUser : User interface for creating a mobHere in g_currentRoom
//

void createMobHereUser(void)
{
  char strn[11] = "";


  _outtext("\n");

  editStrnValSearchableList(strn, 10, "mob vnum to create", displayMobTypeList);

 // user aborted

  if (!strlen(strn))
  {
    _outtext("\n\n");
    return;
  }

  createMobHereStrn(strn);
}


//
// createMobHereStrn : Creates a mob type with id number specified in
//                     strn
//
//   *args : user-entered string
//

void createMobHereStrn(const char *args)
{
  uint numb;
  mobType *mob;
  char strn[1024];


  if (!strlen(args))
  {
    createMobHereUser();
    return;
  }

  if (!strnumber(args))
  {
    _outtext("\nError in input (not numeric).\n\n");
    return;
  }

  numb = strtoul(args, NULL, 10);
  mob = findMob(numb);

  if (!mob && getVnumCheckVal())
  {
    _outtext("\nCouldn't find that mob.\n\n");
    return;
  }
  else
  {
    if (!createMobHereinRoom(mob, numb)) 
      return;

    sprintf(strn, "\n'%s&n' (#%u) created in current room.\n\n", getMobShortName(mob), numb);
    displayColorString(strn);
  }
}
