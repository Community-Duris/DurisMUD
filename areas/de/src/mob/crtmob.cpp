//
//  File: crtmob.cpp     originally part of durisEdit
//
//  Usage: non-user functions for creating mobs
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

#include "fh.h"
#include "types.h"

#include "mob.h"
#include "mobhere.h"



extern uint g_numbMobTypes, g_lowestMobNumber, g_highestMobNumber, g_numbLookupEntries;
extern bool g_madeChanges;
extern mobType *g_defaultMob, **g_mobLookup;

//
// createMobType : Function to create a new mob type - returns pointer to
//                 the new mob type
//
//    incLoaded : whether to inc loaded
//      mobNumb : vnum of new mob
//  getFreeNumb : if true, ignores mobNumb and assigns first free numb
//

mobType *createMobType(const bool incLoaded, const uint mobNumb, const bool getFreeNumb)
{
  mobType *newMob;
  uint numb;


 // create a new mob type

  newMob = new(std::nothrow) mobType;
  if (!newMob)
  {
    displayAllocError("mobType", "createMobType");

    return NULL;
  }

  if (g_defaultMob)
  {
    newMob = copyMobType(g_defaultMob, false);

    newMob->defaultMob = false;
  }
  else
  {
    memset(newMob, 0, sizeof(mobType));

   // default mob names

    strcpy(newMob->mobShortName, "an unnamed mob");
    strcpy(newMob->mobLongName, "An unnamed mob stands here, looking lonely and forelorn.");

    newMob->keywordListHead = createKeywordList("mob unnamed~");

   // level, position, sex, and species

    newMob->level = 1;
    newMob->position = newMob->defaultPos = POSITION_STANDING;
    newMob->sex = SEX_NEUTER;
    strcpy(newMob->mobSpecies, "PH");  // human
    newMob->size = MOB_SIZE_DEFAULT;

    newMob->actionBits |= ACT_ISNPC;  // all mobs are NPCs

   // default hit points and damage strings

    strcpy(newMob->hitPoints, "1d1+1");
    strcpy(newMob->mobDamage, "1d1+1");
  }

  if (!getFreeNumb)
  {
    newMob->mobNumber = numb = mobNumb;

    if (mobExists(numb))
    {
      deleteMobType(newMob, false);
      return NULL;
    }
  }
  else
  {
    if (noMobTypesExist()) 
      newMob->mobNumber = numb = getLowestRoomNumber();
    else 
      newMob->mobNumber = numb = getFirstFreeMobNumber();
  }

  if (incLoaded)
  {
   // if not enough lookup entries, increase the tables

    if (numb >= g_numbLookupEntries)
    {
      if (!changeMaxVnumAutoEcho(numb + 1000))
      {
        deleteMobType(newMob, false);
        return NULL;
      }
    }

    g_numbMobTypes++;

    g_mobLookup[numb] = newMob;

    if (numb > g_highestMobNumber) g_highestMobNumber = numb;
    if (numb < g_lowestMobNumber)  g_lowestMobNumber = numb;

    createPrompt();
  }

  g_madeChanges = true;

  return newMob;
}
