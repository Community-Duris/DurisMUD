//
//  File: command.cpp    originally part of durisEdit
//
//  Usage: function for matching user input to a command
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



#include <stdlib.h>
#include <string.h>

#include "../types.h"
#include "../fh.h"
#include "command.h"




//
// checkCommands : runs through an array of command structs pointed to
//                 by cmd, checking for a lefthand match
//
//                 returns true if execCommand func signalled 'quit'
//
//         strn : user input
//          cmd : start of array of commands
// notfoundStrn : if non-NULL, text displayed if no match is found
//  execCommand : if non-NULL, function executed to process command (based on number)
//     cmdFound : if non-NULL, set to true if command was found, false otherwise
//  matchingCmd : if non-NULL, set to command number matched - not changed if no command matched
//

bool checkCommands(const char *strn, const command *cmd, const char *notfoundStrn,
                   bool (*execCommand)(const usint commands, const char *args),
                   bool *cmdFound, usint *matchingCmd)
{
  const char *argptr;
  size_t cmdend = 0;
  const command *cmdorig = cmd;


 // skip past leading spaces

  while (*strn == ' ')
    strn++;

  if (!*strn)
  {
    if (notfoundStrn)
      displayColorString(notfoundStrn);

    if (cmdFound)
      *cmdFound = false;

    return false;
  }

 // figure out where the command string ends

  argptr = strn;

  while (*argptr && (*argptr != ' '))
  {
    cmdend++;
    argptr++;
  }

 // move past spaces to any args

  while (*argptr == ' ')
    argptr++;

 // left-side match

  while (cmd->commandStrn)
  {
    if (strlefticount(cmd->commandStrn, strn, cmdend))
    {
      if (cmdFound)
        *cmdFound = true;

      if (matchingCmd)
        *matchingCmd = cmd->command;

      if (execCommand)
        return execCommand(cmd->command, argptr);
      else
        return false;
    }

    cmd++;
  }

  if (notfoundStrn)
    displayColorString(notfoundStrn);

  if (cmdFound)
    *cmdFound = false;

  return false;
}
