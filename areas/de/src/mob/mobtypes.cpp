//
//  File: mobtypes.cpp   originally part of durisEdit
//
//  Usage: various functions for checking mob stats or readable strings
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

#include "mob.h"
#include "boolean.h"
#include "fh.h"

extern flagDef g_mobHometownList[], g_mobPositionList[], g_mobSexList[], g_mobSizeList[], g_mobSpecList[];
extern "C" const struct race_names race_names_table[];
extern "C" const char *specdata[][MAX_SPEC];

//
// getMobSpeciesStrn : return colored name for particular species ID
//

const char *getMobSpeciesStrn(const char *id)
{
  for (uint i = 1; i <= LAST_RACE; i++)
    if (strcmpnocase(id, race_names_table[i].code)) 
      return race_names_table[i].ansi;

  return "unrecog. species";
}


//
// getMobSpeciesNoColorStrn : return non-colored name for species ID
//

const char *getMobSpeciesNoColorStrn(const char *id)
{
  for (uint i = 1; i <= LAST_RACE; i++)
    if (strcmpnocase(id, race_names_table[i].code)) 
      return race_names_table[i].normal;

  return "unrecog. species";
}


//
// getMobSpeciesNumb : return colored name for species ID by number
//

const char *getMobSpeciesNumb(const int numb)
{
  if ((numb < 1) || (numb > LAST_RACE))
    return "unrecog. species";

  return race_names_table[numb].ansi;
}


//
// getMobSpeciesCode : given species number, return species ID
//

const char *getMobSpeciesCode(const int numb)
{
  if ((numb < 1) || (numb > LAST_RACE))
    return "unrecog. species";

  return race_names_table[numb].code;
}


//
// getMobPosStrn
//

const char *getMobPosStrn(const uint position)
{
  return getFlagNameFromList(g_mobPositionList, position);
}


//
// getMobSexStrn
//

const char *getMobSexStrn(const int sex)
{
  return getFlagNameFromList(g_mobSexList, sex);
}


//
// getMobHometownStrn
//

const char *getMobHometownStrn(const int hometown)
{
  return getFlagNameFromList(g_mobHometownList, hometown);
}


//
// getMobSpecStrn
//

const char *getMobSpecStrn(const int spec)
{
  /*
  const char *buff = "None";
  if (!spec)
    return buff;
  return specdata[classNumb(cls)][spec];
  */
  return getFlagNameFromList(g_mobSpecList, spec);
}


//
// getMobSizeStrn
//

const char *getMobSizeStrn(const int mobSize)
{
  return getFlagNameFromList(g_mobSizeList, mobSize);

}


//
// isAggro
//

bool isAggro(const mobType *mob)
{
  return (mob->aggroBits || mob->aggro2Bits || mob->aggro3Bits);
}


//
// castingClass
//

bool castingClass(const uint cl)
{
  return (cl & CLASS_RANGER) || (cl & CLASS_PSIONICIST) || (cl & CLASS_PALADIN)
    || (cl & CLASS_ANTIPALADIN) || (cl & CLASS_CLERIC) || (cl & CLASS_DRUID)
    || (cl & CLASS_SHAMAN) || (cl & CLASS_SORCERER) || (cl & CLASS_NECROMANCER)
    || (cl & CLASS_CONJURER) || (cl & CLASS_WARLOCK) || (cl & CLASS_ETHERMANCER)
    || (cl & CLASS_ILLUSIONIST) || (cl & CLASS_THEURGIST) || (cl & CLASS_AVENGER)
	  || (cl & CLASS_BLIGHTER);
}

//
// Count how many classes are set on the mob
//

uint countClass(const uint cl)
{
  uint i;
  uint count = 0;
  for (i = 0; i < 32; i++)
  {
    if (cl & (1 << i))
      count++;
  }
  return count;
}

uint classNumb(const uint cl)
{
  uint i;

  for (i = 0; i < 32; i++)
  {
    if (cl & (1 << i))
      break;
  }
  return i+1;
}

