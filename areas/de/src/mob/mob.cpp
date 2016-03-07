//
//  File: mob.cpp        originally part of durisEdit
//
//  Usage: multitudinous functions related to handling mob type structures
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



#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "fh.h"
#include "types.h"

#include "mob.h"
#include "mobhere.h"



extern uint g_numbLookupEntries, g_numbMobTypes, g_lowestMobNumber, g_highestMobNumber;
extern mobType **g_mobLookup;
extern bool g_madeChanges;
extern editableListNode *g_inventory;
extern room *g_currentRoom;


//
// noMobTypesExist
//

bool noMobTypesExist(void)
{
  return (getLowestMobNumber() > getHighestMobNumber());
}


//
// getMobShortName
//

const char *getMobShortName(const mobType *mob)
{
  if (!mob)
    return "(mob type not in this .mob file)";
  else
    return mob->mobShortName;
}


//
// findMob : Returns the address of the mob node that has the requested
//           mobNumber (if any)
//
//   mobNumber : mob number to return
//

mobType *findMob(const uint mobNumber)
{
  if (mobNumber >= g_numbLookupEntries) 
    return NULL;

  return g_mobLookup[mobNumber];
}


//
// mobExists : Returns true if a mob with number exists
//
//   mobNumber : mob number to seek
//

bool mobExists(const uint mobNumber)
{
  if (mobNumber >= g_numbLookupEntries) 
    return false;

  return (g_mobLookup[mobNumber] != NULL);
}


//
// copyMobType : copies all the info from a mob type record into a new
//               record and returns the address of the new record
//
//   srcMob : source mob type record
//

mobType *copyMobType(const mobType *srcMob, const bool incLoaded)
{
  mobType *newMob;


 // make sure src exists

  if (!srcMob) 
    return NULL;

 // alloc mem for new rec

  newMob = new(std::nothrow) mobType;
  if (!newMob)
  {
    displayAllocError("mobType", "copyMobType");

    return NULL;
  }

 // first, copy the simple stuff

  memcpy(newMob, srcMob, sizeof(mobType));

 // next, the not-so-simple stuff

  newMob->keywordListHead = copyStringNodes(srcMob->keywordListHead);
  newMob->mobDescHead = copyStringNodes(srcMob->mobDescHead);
  newMob->questPtr = copyQuest(srcMob->questPtr);
  newMob->shopPtr = copyShop(srcMob->shopPtr);

  if (incLoaded)
  {
    g_numbMobTypes++;
    createPrompt();
  }

 // return the address of the new mob

  return newMob;
}


//
// compareMobInfo : Compares almost all - returns FALSE if they don't
//                  match, TRUE if they do
//
//                  doesn't compare defaultMob var
//

bool compareMobType(const mobType *mob1, const mobType *mob2)
{
  if (mob1 == mob2) 
    return true;

  if (!mob1 || !mob2) 
    return false;

 // check all mob attributes

  if (strcmp(mob1->mobShortName, mob2->mobShortName)) 
    return false;

  if (strcmp(mob1->mobLongName, mob2->mobLongName)) 
    return false;

  if (mob1->mobNumber != mob2->mobNumber) 
    return false;

  if (mob1->actionBits != mob2->actionBits)
    return false;

  if (mob1->aggroBits != mob2->aggroBits)
    return false;

  if (mob1->aggro2Bits != mob2->aggro2Bits)
    return false;

  if (mob1->aggro3Bits != mob2->aggro3Bits)
    return false;

  if (mob1->affect1Bits != mob2->affect1Bits)
    return false;

  if (mob1->affect2Bits != mob2->affect2Bits)
    return false;

  if (mob1->affect3Bits != mob2->affect3Bits)
    return false;

  if (mob1->affect4Bits != mob2->affect4Bits)
    return false;

  if (mob1->alignment != mob2->alignment) 
    return false;

  if (strcmp(mob1->mobSpecies, mob2->mobSpecies)) 
    return false;

  if (mob1->mobHometown != mob2->mobHometown) 
    return false;

  if (mob1->mobClass != mob2->mobClass)
    return false;

  if (mob1->mobSpec != mob2->mobSpec)
    return false;

  if (mob1->level != mob2->level) return false;
  if (mob1->thac0 != mob2->thac0) return false;
  if (mob1->ac != mob2->ac) return false;

  if (strcmp(mob1->hitPoints, mob2->hitPoints)) return false;
  if (strcmp(mob1->mobDamage, mob2->mobDamage)) return false;

  if (mob1->copper != mob2->copper) return false;
  if (mob1->silver != mob2->silver) return false;
  if (mob1->gold != mob2->gold) return false;
  if (mob1->platinum != mob2->platinum) return false;
  if (mob1->exp != mob2->exp) return false;

  if (mob1->position != mob2->position) return false;
  if (mob1->defaultPos != mob2->defaultPos) return false;
  if (mob1->sex != mob2->sex) return false;
  if (mob1->size != mob2->size) return false;

 // quest and shop

  if (!compareQuestInfo(mob1->questPtr, mob2->questPtr)) return false;
  if (!compareShopInfo(mob1->shopPtr, mob2->shopPtr)) return false;

 // description and keywords

  if (!compareStringNodesIgnoreCase(mob1->keywordListHead, mob2->keywordListHead))
    return false;

  if (!compareStringNodes(mob1->mobDescHead, mob2->mobDescHead))
    return false;


  return true;
}


//
// getHighestMobNumber : Return the highest mob number currently in the zone
//

uint getHighestMobNumber(void)
{
  return g_highestMobNumber;
}


//
// getLowestMobNumber : Return the lowest mob number currently in the zone
//

uint getLowestMobNumber(void)
{
  return g_lowestMobNumber;
}


//
// getFirstFreeMobNumber : Gets the first "free" mob number currently in the
//                         mobHead list, returning it
//

uint getFirstFreeMobNumber(void)
{
  for (uint numb = g_lowestMobNumber + 1; numb <= g_highestMobNumber - 1; numb++)
  {
    if (!g_mobLookup[numb]) 
      return numb;
  }

  return g_highestMobNumber + 1;
}


//
// getMatchingMob : returns mob type that matches strn (keyword or vnum), checking current room first and then
//                  mob type list
//

mobType *getMatchingMob(const char *strn)
{
  uint vnum;
  bool isVnum;
  const uint highMobNumb = getHighestMobNumber();


  if (strnumer(strn))
  {
    isVnum = true;
    vnum = atoi(strn);
  }
  else 
  {
    isVnum = false;
  }

  mobHere *mobHere = g_currentRoom->mobHead;

  while (mobHere)
  {
    if (mobHere->mobPtr)
    {
      if (!isVnum && scanKeyword(strn, mobHere->mobPtr->keywordListHead))
      {
        return mobHere->mobPtr;
      }

      if (isVnum && (mobHere->mobNumb == vnum))
      {
        return mobHere->mobPtr;
      }
    }

    mobHere = mobHere->Next;
  }

  if (isVnum)
    return findMob(vnum);

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      if (scanKeyword(strn, mob->keywordListHead)) 
        return mob;
    }
  }

  return NULL;
}


//
// renumberMobs : Renumbers the mobs so that there are no "gaps" - starts
//                at the first mob and simply renumbers from there
//

void renumberMobs(const bool renumberHead, const uint newHeadNumb)
{
  uint mobNumb = getLowestMobNumber(), lastNumb, oldNumb;
  mobType *mobPtr = findMob(mobNumb);


 // basic technique - keep all old mob pointers in g_mobLookup table until clearing them at the
 // very end so that resetMobHere()/etc calls work; similarly, do not reset low/high mob
 // number until the very end

  if (renumberHead)
  {
    oldNumb = mobNumb;
    mobPtr->mobNumber = lastNumb = newHeadNumb;

    resetMobHere(mobNumb, newHeadNumb);
    resetNumbLoaded(ENTITY_MOB, mobNumb, newHeadNumb);

    g_mobLookup[newHeadNumb] = mobPtr;

    g_madeChanges = true;
  }
  else
  {
    mobNumb = oldNumb = lastNumb = getLowestMobNumber();
  }

 // skip past first mob

  mobPtr = getNextMob(oldNumb);

 // remove gaps

  while (mobPtr)
  {
    oldNumb = mobNumb = mobPtr->mobNumber;

    if (mobNumb != (lastNumb + 1))
    {
      mobPtr->mobNumber = mobNumb = lastNumb + 1;

      resetMobHere(oldNumb, mobNumb);
      resetNumbLoaded(ENTITY_MOB, oldNumb, mobNumb);

      g_madeChanges = true;

      g_mobLookup[mobNumb] = mobPtr;
      if (!renumberHead)
        g_mobLookup[oldNumb] = NULL;
    }

    lastNumb = mobPtr->mobNumber;

    mobPtr = getNextMob(oldNumb);
  }

  resetEntityPointersByNumb(false, true);

 // clear old range

  if (renumberHead)
  {
   // entire range was moved, assumes no overlap

    memset(g_mobLookup + g_lowestMobNumber, 0, ((g_highestMobNumber - g_lowestMobNumber) + 1) * sizeof(mobType*));
  }
  else
  {
   // if the head vnum wasn't changed, then the most that happened was that the high vnum came down

    memset(g_mobLookup + lastNumb + 1, 0, (g_highestMobNumber - lastNumb) * sizeof(mobType*));
  }

 // set new low/high

  if (renumberHead)
    g_lowestMobNumber = newHeadNumb;

  g_highestMobNumber = lastNumb;
}


//
// getPrevMob : given mob numb, return first valid mob type before it
//

mobType *getPrevMob(const uint mobNumb)
{
  uint i = mobNumb - 1;

  if (mobNumb <= g_lowestMobNumber) 
    return NULL;

  while (!g_mobLookup[i]) 
    i--;

  return g_mobLookup[i];
}


//
// getNextMob : find mob right after mobNumb, numerically
//

mobType *getNextMob(const uint mobNumb)
{
  uint i = mobNumb + 1;

  if (mobNumb >= getHighestMobNumber()) 
    return NULL;

  while (!g_mobLookup[i]) 
    i++;

  return g_mobLookup[i];
}


//
// displayDieStrnError : called by checkDieStrnValidityReal on invalid die string
//

void displayDieStrnError(const uint mobNumb, const char *fieldName)
{
  char outstrn[512];

  _outtext("\nError in ");
  _outtext(fieldName);

  sprintf(outstrn, " string for mob #%u.\nString read is '",
          mobNumb);
  
  _outtext(outstrn);
  _outtext("'.  Aborting.\n");
}


//
// checkDieStrnValidityReal : redundant code for checking die string validity - if blnExit is true, echoes
//                            appropriate message to screen and exit(1)s, otherwise returns false if die
//                            string is invalid
//

bool checkDieStrnValidityReal(const char *strn, const uint mobNumb, const char *fieldName, const bool blnExit)
{
  bool foundDigit = false;


  while ((*strn != '\0') && (toupper(*strn) != 'D'))
  {
    if (!isdigit(*strn))
    {
      if (blnExit)
      {
        displayDieStrnError(mobNumb, fieldName);

        exit(1);
      }
      else
      {
        return false;
      }
    }
    else 
    {
      foundDigit = true;
    }

    strn++;
  }

  if ((*strn == '\0') || !foundDigit) // error
  {
    if (blnExit)
    {
      displayDieStrnError(mobNumb, fieldName);

      exit(1);
    }
    else
    {
      return false;
    }
  }

  foundDigit = false;
  strn++;

  while ((*strn != '\0') && (*strn != '+'))
  {
    if (!isdigit(*strn)) // error
    {
      if (blnExit)
      {
        displayDieStrnError(mobNumb, fieldName);

        exit(1);
      }
      else
      {
        return false;
      }
    }
    else 
    {
      foundDigit = true;
    }

    strn++;
  }

  if ((*strn == '\0') || !foundDigit) // error
  {
    if (blnExit)
    {
      displayDieStrnError(mobNumb, fieldName);

      exit(1);
    }
    else
    {
      return false;
    }
  }

  strn++;

  if (!isdigit(*strn))
  {
    if (blnExit)
    {
      displayDieStrnError(mobNumb, fieldName);

      exit(1);
    }
    else
    {
      return false;
    }
  }

  while (*strn != '\0')
  {
    if (!isdigit(*strn)) // error
    {
      if (blnExit)
      {
        displayDieStrnError(mobNumb, fieldName);

        exit(1);
      }
      else
      {
        return false;
      }
    }

    strn++;
  }

  return true;
}


//
// checkDieStrnValidityShort - return false if string is not a valid die string (XdY+Z)
//

bool checkDieStrnValidityShort(const char *strn)
{
  return checkDieStrnValidityReal(strn, 0, NULL, false);
}


//
// checkDieStrnValidity - checks an XdY+Z format string - returns true if successful, exit(1)s if not
//

bool checkDieStrnValidity(const char *strn, const uint mobNumb, const char *fieldName)
{
  return checkDieStrnValidityReal(strn, mobNumb, fieldName, true);
}


//
// deleteMobsinInv : delete mobHeres of type mobPtr in inventory
//

void deleteMobsinInv(const mobType *mobPtr)
{
  editableListNode *edit = g_inventory, *next;


  if (!mobPtr) 
    return;

  while (edit)
  {
    next = edit->Next;

    if ((edit->entityType == ENTITY_MOB) && (((mobHere *)(edit->entityPtr))->mobPtr == mobPtr))
      deleteEditableinList(&g_inventory, edit);

    edit = next;
  }
}


//
// updateInvKeywordsMob : recreate keyword lists of mobHeres that match mobPtr in inventory
//

void updateInvKeywordsMob(const mobType *mobPtr)
{
  editableListNode *edit = g_inventory;


  if (!mobPtr) 
    return;

  while (edit)
  {
    if ((edit->entityType == ENTITY_MOB) && (((mobHere *)(edit->entityPtr))->mobPtr == mobPtr))
    {
      edit->keywordListHead = mobPtr->keywordListHead;
    }

    edit = edit->Next;
  }
}


//
// resetLowHighMob : reset global low/high mob vnum vars
//

void resetLowHighMob(void)
{
  uint i, high = 0, low = g_numbLookupEntries;

  for (i = 0; i < g_numbLookupEntries; i++)
  {
    if (g_mobLookup[i])
    {
      if (i > high) 
        high = i;

      if (i < low) 
        low = i;
    }
  }

  g_lowestMobNumber = low;
  g_highestMobNumber = high;
}
