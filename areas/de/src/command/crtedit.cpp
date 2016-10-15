//
//  File: crtedit.cpp    originally part of durisEdit
//
//  Usage: interpreter functions for handling 'createedit' syntax
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

#include "../types.h"
#include "../fh.h"

extern command g_createEditCommands[];
extern room *g_currentRoom;
extern const char *g_exitnames[];

//
// createEdit
//

void createEdit(const char *args)
{
  checkCommands(args, g_createEditCommands, "\n"
"Specify one of <room|obj|mob|exit> as the first argument.\n\n",
                createEditExecCommand, NULL, NULL);
}


//
// createEditExit
//

void createEditExit(const char *args)
{
  int returnVal, val;


  if (!strlen(args))
  {
    returnVal = createRoomExit(USER_CHOICE);
  }
  else
  {
    val = getDirfromKeyword(args);
    if (val == NO_EXIT)
    {
      _outtext("\nCreate and edit which exit?\n\n");
      return;
    }

    returnVal = createRoomExit(val);
  }

  if (returnVal == NO_EXIT)
    return;

  editExit(g_currentRoom, &g_currentRoom->exits[returnVal], g_exitnames[returnVal], true);
}
