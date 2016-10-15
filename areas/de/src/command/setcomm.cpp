//
//  File: setcomm.cpp    originally part of durisEdit
//
//  Usage: functions for handling reading of .set files
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

#include "../fh.h"
#include "../types.h"

#include "setcomm.h"
#include "../vardef.h"
#include "alias.h"

extern alias *g_aliasHead;
extern variable *g_varHead;

//
// setExecCommandFile
//

bool setExecCommandFile(const usint command, const char *args)
{
 // don't process SET MAINZONENAME command, cuz it screws up paths while
 // loading

  if ((command == SETCMD_SET) && strlefti(args, VAR_MAINZONENAME_NAME))
  {
    return false;
  }

  switch (command)
  {
    case SETCMD_SET     : setVarArgs(&g_varHead, args, false, false, false); break;
    case SETCMD_ALIAS   : addAliasArgs(args, false, false, &g_aliasHead, false); break;
    case SETCMD_LIMIT   : setLimitArgsStartup(args); break;
    case SETCMD_SETTEMP : setTemplateArgs(args, false, false); break;
  }

  return false;
}
