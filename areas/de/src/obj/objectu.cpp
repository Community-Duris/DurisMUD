//
//  File: objectu.cpp    originally part of durisEdit
//
//  Usage: miscellaneous functions invoked by the user, object-related
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

extern uint g_numbObjTypes, g_numbLookupEntries;

//
// renumberObjectsUser
//

void renumberObjectsUser(const char *args)
{
  char strn[256];
  uint usernumb, tempstart;

  if (strlen(args) == 0)
  {
    if (noObjTypesExist())
    {
      _outtext("\nThere are no objects to renumber.\n\n");
    }
    else 
    {
      renumberObjs(false, 0);

      sprintf(strn, "\nObjects have been renumbered (starting at %u).\n\n",
              getLowestObjNumber());
      _outtext(strn);
    }

    return;
  }

  if (!strnumber(args))
  {
    _outtext(
"\nThe 'renumberobject' command's first argument must be a positive number.\n\n");
    return;
  }

  usernumb = strtoul(args, NULL, 10);

  if (usernumb == 0)
  {
    _outtext(
"\nYou cannot renumber starting from 0.\n\n");
    return;
  }

  if (noObjTypesExist())
  {
    _outtext("\nThere are no objects to renumber.\n\n");
    return;
  }
  else
  {
    tempstart = getHighestObjNumber() + g_numbObjTypes;

    if (((usernumb + g_numbObjTypes) >= g_numbLookupEntries) || ((tempstart + g_numbObjTypes) >= g_numbLookupEntries))
    {
      uint newLimit;

      if (usernumb > tempstart)
        newLimit = usernumb;
      else
        newLimit = tempstart;

      if (!changeMaxVnumAutoEcho(newLimit + g_numbObjTypes + 1000))
        return;
    }

    renumberObjs(true, tempstart);

    renumberObjs(true, usernumb);
  }

  sprintf(strn,
"\nObjects renumbered starting at %u.\n\n", usernumb);

  _outtext(strn);
}
