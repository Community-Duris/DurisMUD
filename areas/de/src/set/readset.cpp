//
//  File: readset.cpp    originally part of durisEdit
//
//  Usage: functions for reading .set file
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
#include <string.h>


#include "../fh.h"
#include "../types.h"

#include "readset.h"



extern bool g_readFromSubdirs;
extern command g_setCommands[];

//
// readSettingsFile : reads the .set file specified by filename (subdir
//                    is stuck at start of filename, no extension is added,
//                    though) line-by-line until eof
//
//   filename : filename to read (with extension)
//

void readSettingsFile(const char *filename)
{
  FILE *setFile;
  char strn[MAX_SETLINE_LEN], strn2[256], setFilename[512] = "";
  uint line = 1;


  if (g_readFromSubdirs) 
    strcpy(setFilename, "set/");

  strcat(setFilename, filename);

  if ((setFile = fopen(setFilename, "rt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(setFilename);
    _outtext(", skipping\n");

    return;
  }
  else
  {
    _outtext("Reading ");
    _outtext(setFilename);
    _outtext("...\n");
  }

 //
 // format of setfile is as follows -
 //
 // set <var> <value>   or
 // alias <alias> <aliased string>   or
 // limit <obj|mob> <vnum> <limit>   or
 // random <obj|mob> <vnum> <room numb> <random val> (not imped) or
 // settemplate <i forget>
 //

  while (true)
  {
    if (!fgets(strn, MAX_SETLINE_LEN, setFile)) 
      break;

    nolf(strn);

    sprintf(strn2,
"Error in line %u of .set file - specify one of <set|alias|limit|settemplate>\n",
            line);

    checkCommands(strn, g_setCommands, strn2, setExecCommandFile, NULL, NULL);

    line++;
  }

  fclose(setFile);
}
