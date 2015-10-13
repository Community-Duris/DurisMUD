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


// header for durisEdit, the Duris area editor

#ifndef _DE_H_

#include "types.h"
#include "misc/strnnode.h"
#include "boolean.h"


#define MAX_PROMPTINPUT_LEN  256    // max length of prompt input string

#define MAX_EXPANDEDPROMPTINPUT_LEN   1024  // max length of ultimate user input after alias/variable expansion

#define MAX_MAINNAME_LEN     MAX_VARVAL_LEN
                                    // max length of name of zone files

#define MAIN_PROMPT_COMM_NOT_FOUND   \
"\n&nInvalid command.  (Type \"?\" for short help, \"HELP\" for long help, or \"COMMANDS\" for a command list.)\n\n"
                                    // string shown when invalid command
                                    // entered at main prompt

#define NUMB_FLAG_TEMPLATES  7      // number of flag templates

#define DE_VERSION           "3.00" // version string

#ifndef __UNIX__
#  define UNUSED_FIELD_CH '°'  // looks like half-shaded square with standard IBM PC character set
#else
#  define UNUSED_FIELD_CH ' '
#endif

#define DEFAULT_LOOKUP_ENTRIES  100000

#ifdef _WIN32
#  define DE_WIN32_CONSOLEAPP_NAME "durisEdit - the Duris area editor"
#endif

#define _DE_H_
#endif
