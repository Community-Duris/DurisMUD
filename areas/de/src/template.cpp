//
//  File: template.cpp   originally part of durisEdit
//
//  Usage: template variable declarations and setting function
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
#include <ctype.h>

#include "graphcon.h"

#include "types.h"
#include "boolean.h"
#include "fh.h"
#include "de.h"
#include "keys.h"

extern bool g_madeChanges;

// template variables

uint g_roomFlagTemplates[NUMB_FLAG_TEMPLATES];

uint g_objExtraFlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_objExtra2FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_objWearFlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_objAntiFlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_objAnti2FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_objAff1FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_objAff2FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_objAff3FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_objAff4FlagTemplates[NUMB_FLAG_TEMPLATES];

uint g_mobActionFlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_mobAff1FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_mobAff2FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_mobAff3FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_mobAff4FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_mobAggroFlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_mobAggro2FlagTemplates[NUMB_FLAG_TEMPLATES];
uint g_mobAggro3FlagTemplates[NUMB_FLAG_TEMPLATES];

// different keys for templates depending on OS

#ifndef __UNIX__
usint g_templateKeys[NUMB_FLAG_TEMPLATES]    =
                            { K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7 };
usint g_setTemplateKeys[NUMB_FLAG_TEMPLATES] =
                            { K_Shift_F1, K_Shift_F2, K_Shift_F3, K_Shift_F4,
                              K_Shift_F5, K_Shift_F6, K_Shift_F7 };
#else

// any-terminal compatible keys

usint g_templateKeys[NUMB_FLAG_TEMPLATES]    =
      { '~', '@', '#', '$', '%', '^', '&' };
usint g_setTemplateKeys[NUMB_FLAG_TEMPLATES] =
      { '*', '(', ')', '_', '+', '{', '}' };
#endif


//
// displayTemplateRedundant : redundant code used by setTemplateArgs - foundTemplate only set if template
//                            val displayed, returns false if user aborted
//
//

bool displayTemplateRedundant(const uint templateNumb, const char *flagname, const uint templateVal,
                              size_t& numblines, bool *foundTemplate)
{
  if (templateVal)
  {
    char strn[1024];

    sprintf(strn, "template #%u - %s = %u\n", 
            templateNumb, flagname, templateVal);

    *foundTemplate = true;

    if (checkPause(strn, numblines))
      return false;
  }

  return true;
}


//
// setTemplateArgs : Sets a flag template based on user args
//

void setTemplateArgs(const char *args, const bool updateChanges, const bool displayStuff)
{
  uint i, tempNumb, value;
  size_t numblines = 1;
  char flagname[512];
  bool foundTemplate = false;


  if (!strlen(args) && displayStuff) // display template settings
  {
    _outtext("\n\n");

    for (i = 0; i < NUMB_FLAG_TEMPLATES; i++)
    {
      if (!displayTemplateRedundant(i, "roomflag", g_roomFlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objextra", g_objExtraFlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objextra2", g_objExtra2FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objanti", g_objAntiFlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objanti2", g_objAnti2FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objwear", g_objWearFlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objaff1", g_objAff1FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objaff2", g_objAff2FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objaff3", g_objAff3FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "objaff4", g_objAff4FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "mobact", g_mobActionFlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "mobaff1", g_mobAff1FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "mobaff2", g_mobAff2FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "mobaff3", g_mobAff3FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "mobaff4", g_mobAff4FlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "mobaggro", g_mobAggroFlagTemplates[i], numblines, &foundTemplate))
        return;

      if (!displayTemplateRedundant(i, "mobaggro2", g_mobAggro2FlagTemplates[i], numblines, &foundTemplate))
        return;
      
      if (!displayTemplateRedundant(i, "mobaggro3", g_mobAggro3FlagTemplates[i], numblines, &foundTemplate))
        return;
    }

    if (!foundTemplate) 
      _outtext("No templates are defined.\n");

    _outtext("\n");

    return;
  }

  if (numbArgs(args) != 3)
  {
    _outtext("\n"
"The format of the 'settemplate' command is 'settemp <flagname> <temp #>\n"
"<value>'.\n\n");

    return;
  }

  getArg(args, 1, flagname, 511);

  char arg[512];

  tempNumb = strtoul(getArg(args, 2, arg, 511), NULL, 10);
  value = strtoul(getArg(args, 3, arg, 511), NULL, 10);

  if (tempNumb >= NUMB_FLAG_TEMPLATES)
  {
    _outtext("\n"
"Specify one of the seven flag templates available, from 0-6.\n\n");

    return;
  }

 // set appropriate template

  if (strcmpnocase(flagname, "ROOMFLAG"))
    g_roomFlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJEXTRA"))
    g_objExtraFlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJEXTRA2"))
    g_objExtra2FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJWEAR"))
    g_objWearFlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJAFF1"))
    g_objAff1FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJAFF2"))
    g_objAff2FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJAFF3"))
    g_objAff3FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJAFF4"))
    g_objAff4FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJANTI"))
    g_objAntiFlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "OBJANTI2"))
    g_objAnti2FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "MOBACT"))
    g_mobActionFlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "MOBAFF1"))
    g_mobAff1FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "MOBAFF2"))
    g_mobAff2FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "MOBAFF3"))
    g_mobAff3FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "MOBAFF4"))
    g_mobAff4FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "MOBAGGRO"))
    g_mobAggroFlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "MOBAGGRO2"))
    g_mobAggro2FlagTemplates[tempNumb] = value;
  else
  if (strcmpnocase(flagname, "MOBAGGRO3"))
    g_mobAggro3FlagTemplates[tempNumb] = value;
  else
  {
    _outtext("\n"
"Invalid flag name specified - valid flagnames are roomflag, objextra,\n"
"objextra2, objwear, objaff1, objaff2, objaff3, objaff4, objanti,\n"
"objanti2, mobact, mobaff1, mobaff2, mobaff3, mobaff4, mobaggro,\n"
"mobaggro2, and mobaggro3.\n\n");

    return;
  }

  if (displayStuff)
  {
    char strn[256];

    sprintf(strn, "\nTemplate #%u's %s value set to %u.\n\n",
            tempNumb, flagname, value);

    _outtext(strn);
  }

  if (updateChanges) 
    g_madeChanges = true;
}
