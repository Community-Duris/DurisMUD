//
//  File: delmobt.cpp    originally part of durisEdit
//
//  Usage: functions for deleting mob types
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


#include "../fh.h"
#include "../types.h"

extern mobType **g_mobLookup;
extern uint g_numbMobTypes, g_lowestMobNumber, g_highestMobNumber, g_numbLookupEntries;
extern bool g_madeChanges;

//
// deleteMobType : deletes all the associated baggage with a mobType rec
//
//      *srcMob : record to delete
//  decNumbMobs : if TRUE, decrements g_numbMobTypes variable
//

void deleteMobType(mobType *srcMob, const bool decNumbMobs)
{
 // make sure src exists

  if (!srcMob) 
    return;

 // delete mob keywords

  deleteStringNodes(srcMob->keywordListHead);

 // delete mob desc

  deleteStringNodes(srcMob->mobDescHead);

 // delete quest

  deleteQuest(srcMob->questPtr, decNumbMobs);

 // delete shop

  deleteShop(srcMob->shopPtr, decNumbMobs);

 // if requested, remove from lookup table

  if (decNumbMobs)
  {
    const uint numb = srcMob->mobNumber;

    g_numbMobTypes--;
    if (!g_numbMobTypes)
    {
      g_lowestMobNumber = g_numbLookupEntries;
      g_highestMobNumber = 0;
    }

    g_mobLookup[numb] = NULL;
    createPrompt();

    deleteEntityinLoaded(ENTITY_MOB, numb);

    resetLowHighMob();

    g_madeChanges = true;
  }

 // finally, delete the record itself

  delete srcMob;
}


//
// deleteMobTypeAssocLists : deletes all the associated baggage with a
//                           mob type, but not the record itself
//
//     *srcMob : record to alter
//

void deleteMobTypeAssocLists(mobType *srcMob)
{

 // make sure src exists

  if (!srcMob) 
    return;

 // delete mob keywords

  deleteStringNodes(srcMob->keywordListHead);

 // delete mob desc

  deleteStringNodes(srcMob->mobDescHead);

 // delete mob quest

  deleteQuest(srcMob->questPtr, false);

 // delete mob shop

  deleteShop(srcMob->shopPtr, false);
}
