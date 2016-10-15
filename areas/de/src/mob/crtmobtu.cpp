//
//  File: crtmobtu.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for creating mob types
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

#include "../types.h"
#include "../fh.h"


//
// createMobTypeUser : User interface for creating a mob type - returns
//                     pointer to mob if created, NULL if not
//

mobType *createMobTypeUser(const char *args)
{
  char strn[512];
  mobType *newMob;
  uint numb = 0;  // initialized to keep debugger happy
  bool getFreeNumb;


  if (strlen(args))
  {
    if (!strnumber(args))
    {
      _outtext("\nError in vnum argument - non-numerics in input.\n\n");
      return NULL;
    }

    numb = strtoul(args, NULL, 10);

    if (mobExists(numb))
    {
      sprintf(strn,
"\nCannot create a mob type with the vnum %u - this vnum is already taken.\n\n",
              numb);
      _outtext(strn);

      return NULL;
    }

    getFreeNumb = false;
  }
  else
  {
    getFreeNumb = true;
  }

  newMob = createMobType(true, numb, getFreeNumb);

  if (!newMob)
    return NULL;

  resetEntityPointersByNumb(false, true);

  sprintf(strn, "\nMob type #%u '%s&n' created.\n\n", newMob->mobNumber, getMobShortName(newMob));
  displayColorString(strn);

  return newMob;
}
