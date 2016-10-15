//
//  File: writeset.cpp   originally part of durisEdit
//
//  Usage: functions for writing stuff to .set file
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

#include "../fh.h"
#include "../types.h"
#include "../de.h"

#include "readset.h"

#include "../command/alias.h"
#include "../vardef.h"



extern room *g_currentRoom;
extern variable *g_varHead;
extern entityLoaded *g_numbLoadedHead;
extern bool g_readFromSubdirs;
extern uint g_roomFlagTemplates[], 
            g_objExtraFlagTemplates[], g_objExtra2FlagTemplates[], g_objWearFlagTemplates[], 
             g_objAff1FlagTemplates[], 
            g_objAff2FlagTemplates[], g_objAff3FlagTemplates[], g_objAff4FlagTemplates[], 
             g_objAntiFlagTemplates[], 
            g_objAnti2FlagTemplates[],
            g_mobActionFlagTemplates[], g_mobAff1FlagTemplates[], g_mobAff2FlagTemplates[], 
             g_mobAff3FlagTemplates[], 
            g_mobAff4FlagTemplates[], g_mobAggroFlagTemplates[], g_mobAggro2FlagTemplates[],
	    g_mobAggro3FlagTemplates[];
extern alias *g_aliasHead;

//
// writeSettingsFile
//

void writeSettingsFile(const char *filename, const bool writeLimits)
{
  FILE *setFile;
  char strn[256], setFilename[512] = "";
  const char *limstrn;
  const alias *aliasNode = g_aliasHead;
  const variable *varNode = g_varHead;
  const entityLoaded *loadNode = g_numbLoadedHead;


  if (g_readFromSubdirs) 
    strcpy(setFilename, "set/");

  strcat(setFilename, filename);

  if ((setFile = fopen(setFilename, "wt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(setFilename);
    _outtext(" for writing - aborting\n");

    return;
  }
  else
  {
    _outtext("Writing ");
    _outtext(setFilename);
    _outtext("...\n");
  }

 //
 // format of setfile is as follows -
 //
 // set <var> <value>   or
 // alias <alias> <aliased string>   or
 // limit <obj|mob> <vnum> <limit>   or
 // template <name> <number> <value>
 //

 // write aliases

  while (aliasNode)
  {
    fprintf(setFile, "alias %s %s\n", 
            aliasNode->aliasStrn, aliasNode->commandStrn);

    aliasNode = aliasNode->Next;
  }

 // set 'start room' variable to vnum of g_currentRoom

  if (getStartRoomActiveVal())
  {
    sprintf(strn, "%u", g_currentRoom->roomNumber);

    addVar(&g_varHead, VAR_STARTROOM_NAME, strn);
  }

 // write variables

  while (varNode)
  {
    fprintf(setFile, "set %s %s\n", 
            varNode->varName, varNode->varValue);

    varNode = varNode->Next;
  }

 // write object/mob load limits

  if (writeLimits)
  {
    while (loadNode)
    {
      loadNode = getFirstEntityOverride(loadNode);

      if (!loadNode) 
        break;

      switch (loadNode->entityType)
      {
        case ENTITY_OBJECT : limstrn = "object"; break;
        case ENTITY_MOB    : limstrn = "mob"; break;
      }

      fprintf(setFile, "limit %s %u %u\n",
              limstrn, loadNode->entityNumb, loadNode->overrideLoaded);

      loadNode = loadNode->Next;
    }
  }

 // write template values

  for (uint i = 0; i < NUMB_FLAG_TEMPLATES; i++)
  {
    writeTemplateRedundant(setFile, "roomflag", i, g_roomFlagTemplates[i]);

    writeTemplateRedundant(setFile, "objextra", i, g_objExtraFlagTemplates[i]);
    writeTemplateRedundant(setFile, "objextra2", i, g_objExtra2FlagTemplates[i]);
    writeTemplateRedundant(setFile, "objwear", i, g_objWearFlagTemplates[i]);
    writeTemplateRedundant(setFile, "objaff1", i, g_objAff1FlagTemplates[i]);
    writeTemplateRedundant(setFile, "objaff2", i, g_objAff2FlagTemplates[i]);
    writeTemplateRedundant(setFile, "objaff3", i, g_objAff3FlagTemplates[i]);
    writeTemplateRedundant(setFile, "objaff4", i, g_objAff4FlagTemplates[i]);
    writeTemplateRedundant(setFile, "objanti", i, g_objAntiFlagTemplates[i]);
    writeTemplateRedundant(setFile, "objanti2", i, g_objAnti2FlagTemplates[i]);

    writeTemplateRedundant(setFile, "mobact", i, g_mobActionFlagTemplates[i]);
    writeTemplateRedundant(setFile, "mobaff1", i, g_mobAff1FlagTemplates[i]);
    writeTemplateRedundant(setFile, "mobaff2", i, g_mobAff2FlagTemplates[i]);
    writeTemplateRedundant(setFile, "mobaff3", i, g_mobAff3FlagTemplates[i]);
    writeTemplateRedundant(setFile, "mobaff4", i, g_mobAff4FlagTemplates[i]);
    writeTemplateRedundant(setFile, "mobaggro", i, g_mobAggroFlagTemplates[i]);
    writeTemplateRedundant(setFile, "mobaggro2", i, g_mobAggro2FlagTemplates[i]);
    writeTemplateRedundant(setFile, "mobaggro3", i, g_mobAggro3FlagTemplates[i]);
  }

  fclose(setFile);
}


//
// writeTemplateRedundant : redundancy
//

void writeTemplateRedundant(FILE *setFile, const char *tempName, const uint tempNumb, const uint tempVal)
{
  if (tempVal)
    fprintf(setFile, "settemp %s %u %u\n", tempName, tempNumb, tempVal);
}
