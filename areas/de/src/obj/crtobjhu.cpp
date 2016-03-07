//
//  File: crtobjhu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for creating objectHeres
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

#include "object.h"
#include "objhere.h"

extern room *g_currentRoom;

//
// createObjectHereUser : User interface for creating an objectHere in
//                        g_currentRoom
//

void createObjectHereUser(void)
{
  char strn[11] = "";


  _outtext("\n");

  editStrnValSearchableList(strn, 10, "object vnum to create", displayObjectTypeList);

 // user aborted

  if (!strlen(strn))
  {
    _outtext("\n\n");
    return;
  }

  createObjectHereStrn(strn);
}


//
// createObjectHereStrn : Creates an objectHere with id number specified in
//                        args.
//
//   *args : user-entered string
//

void createObjectHereStrn(const char *args)
{
  objectType *obj;
  char strn[1024];
  uint numb;


  if (!strlen(args))
  {
    createObjectHereUser();
    return;
  }

  if (!strnumer(args))
  {
    _outtext("\nError in input (not numeric).\n\n");
    return;
  }

  numb = strtoul(args, NULL, 10);
  obj = findObj(numb);

  if (!obj && getVnumCheckVal())
  {
    sprintf(strn,
"\nObject of type #%u does not exist in this zone.\n\n", numb);
    _outtext(strn);
  }
  else
  {
    createObjectHereinRoom(obj, numb);

    sprintf(strn, "\n'%s&n' (#%u) created in current room.\n\n", getObjShortName(obj), numb);
    displayColorString(strn);
  }
}
