//
//  File: mobu.cpp       originally part of durisEdit
//
//  Usage: function(s) invoked by user to alter mobs
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

#include "types.h"
#include "fh.h"

#include "mob.h"

extern uint g_numbMobTypes, g_numbLookupEntries;


//
// renumberMobsUser
//

void renumberMobsUser(const char *args)
{
  char strn[256];
  uint usernumb, tempstart;

  if (strlen(args) == 0)
  {
    if (noMobTypesExist())
    {
      _outtext("\nThere are no mobs to renumber.\n\n");
    }
    else 
    {
      renumberMobs(false, 0);

      sprintf(strn, "\nMobs have been renumbered (starting at %u).\n\n",
              getLowestMobNumber());
      _outtext(strn);
    }

    return;
  }

  if (!strnumer(args))
  {
    _outtext(
"\nThe 'renumbermob' command's first argument must be a positive number.\n\n");
    return;
  }

  usernumb = strtoul(args, NULL, 10);

  if (usernumb == 0)
  {
    _outtext(
"\nYou cannot renumber starting from 0.\n\n");
    return;
  }

  if (noMobTypesExist())
  {
    _outtext("\nThere are no mobs to renumber.\n\n");
    return;
  }
  else
  {
    tempstart = getHighestMobNumber() + g_numbMobTypes;

    if (((usernumb + g_numbMobTypes) >= g_numbLookupEntries) || 
        ((tempstart + g_numbMobTypes) >= g_numbLookupEntries))
    {
      uint newLimit;

      if (usernumb > tempstart)
        newLimit = usernumb;
      else
        newLimit = tempstart;

      if (!changeMaxVnumAutoEcho(newLimit + g_numbMobTypes + 1000))
        return;
    }

    renumberMobs(true, tempstart);

    renumberMobs(true, usernumb);
  }

  sprintf(strn,
"\nMobs renumbered starting at %u.\n\n", usernumb);

  _outtext(strn);
}
