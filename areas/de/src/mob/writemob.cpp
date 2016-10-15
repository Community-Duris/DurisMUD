//
//  File: writemob.cpp   originally part of durisEdit
//
//  Usage: functions for writing mob info to the .mob file
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

#include "mob.h"



extern uint g_numbMobTypes;
extern bool g_readFromSubdirs;

//
// writeMobtoFile
//

void writeMobtoFile(FILE *mobFile, const mobType *mob)
{
  char strn[512];


 // first, write the mob number

  fprintf(mobFile, "#%u\n", mob->mobNumber);

 // next, the mob keyword list

  createKeywordString(mob->keywordListHead, strn, 511);
  lowstrn(strn);
  strcat(strn, "\n");

  fputs(strn, mobFile);

 // next, the short name of the mob

  fprintf(mobFile, "%s~\n", mob->mobShortName);

 // next, the long name of the mob

  fprintf(mobFile, "%s\n", mob->mobLongName);

 // add another line with nothing but a tilde cuz it must be there

  fputs("~\n", mobFile);

 // write the mob desc

  writeStringNodes(mobFile, mob->mobDescHead);

 // terminate the description

  fputs("~\n", mobFile);

 // next, write the action, agg, agg2, aff1, aff2, aff3, aff4 flags,
 // alignment and "S"

  fprintf(mobFile, "%u %u %u %u %u %u %u %u %d S\n",
                                            mob->actionBits,
                                            mob->aggroBits,
                                            mob->aggro2Bits,
                                            mob->aggro3Bits,
                                            mob->affect1Bits,
                                            mob->affect2Bits,
                                            mob->affect3Bits,
                                            mob->affect4Bits,
                                            mob->alignment);

 // next, the species, hometown, mob class, and size

  char speciesUp[MAX_SPECIES_LEN + 1];

  strcpy(speciesUp, mob->mobSpecies);
  upstrn(speciesUp);

  fprintf(mobFile, "%s %d %u %u %d\n", mob->mobSpecies, mob->mobHometown,
                                    mob->mobClass, mob->mobSpec, mob->size);

 // next, the level, thac0, ac, hit points and damage

  fprintf(mobFile, "%d %d %d %s %s\n", mob->level, mob->thac0, mob->ac,
                                       mob->hitPoints, mob->mobDamage);

 // next, the gold and exp

  fprintf(mobFile, "%u.%u.%u.%u %u\n", mob->copper, mob->silver, mob->gold,
                                       mob->platinum, mob->exp);

 // finally, the pos, default pos, and sex

  fprintf(mobFile, "%d %d %d\n", mob->position, mob->defaultPos, mob->sex);
}


//
// writeMobFile : Write the mob file - contains all the mobs
//

void writeMobFile(const char *filename)
{
  FILE *mobFile;
  char mobFilename[512] = "";
  char strn[512];

  uint numb = getLowestMobNumber();
  const uint highest = getHighestMobNumber();


 // assemble the filename of the mob file

  if (g_readFromSubdirs) 
    strcpy(mobFilename, "mob/");

  if (filename) 
    strcat(mobFilename, filename);
  else 
    strcat(mobFilename, getMainZoneNameStrn());

  strcat(mobFilename, ".mob");


 // open the mob file for writing

  if ((mobFile = fopen(mobFilename, "wt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(mobFilename);
    _outtext(" for writing - aborting\n");

    return;
  }

  sprintf(strn, "Writing %s - %u mob type%s\n",
          mobFilename, g_numbMobTypes, plural(g_numbMobTypes));

  _outtext(strn);

  while (numb <= highest)
  {
    const mobType *mob = findMob(numb);

    if (mob)
      writeMobtoFile(mobFile, mob);

    numb++;
  }

  fclose(mobFile);
}
