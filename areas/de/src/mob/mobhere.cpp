//
//  File: mobhere.cpp    originally part of durisEdit
//
//  Usage: tons o functions for manipulating mobHere structures
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



#include <stdlib.h>
#include <string.h>

#include "fh.h"
#include "types.h"

#include "mobhere.h"



extern editableListNode *g_inventory;
extern uint g_numbMobs;
extern bool g_madeChanges;
extern room *g_currentRoom;
extern "C" const struct race_names race_names_table[];                      


//
// addMobLoad : given mobHere, increment loaded count for mob itself and everything mob is carrying and
//              equipping
//

void addMobLoad(const mobHere *mob)
{
  objectHere *inv;


  addEntity(ENTITY_MOB, mob->mobNumb);

 // add equipment and inventory

  for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
    if (mob->equipment[i])
      addObjLoad(mob->equipment[i]);

  inv = mob->inventoryHead;
  while (inv)
  {
    addObjLoad(inv);

    inv = inv->Next;
  }

  g_numbMobs++;
  createPrompt();

  g_madeChanges = true;
}


//
// removeMobLoad : reduce internal count, set g_madeChanges, etc
//

void removeMobLoad(const mobHere *mob)
{
  const objectHere *inv;


  decEntity(ENTITY_MOB, mob->mobNumb);

 // remove equipment and inventory

  for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
    if (mob->equipment[i])
      removeObjLoad(mob->equipment[i]);

  inv = mob->inventoryHead;
  while (inv)
  {
    removeObjLoad(inv);

    inv = inv->Next;
  }

  g_numbMobs--;
  createPrompt();

  g_madeChanges = true;
}


//
// loadAllinMobHereList : increment loaded counts for entire mobHere list
//

void loadAllinMobHereList(const mobHere *mobHead)
{
  while (mobHead)
  {
    addMobLoad(mobHead);

    mobHead = mobHead->Next;
  }
}


//
// findMobHere : Returns the address of the mobHere node that has the
//               requested mobNumber (if any)
//
//            mobNumber : mob number to look for in all mobHere nodes everywhere
//             roomNumb : points to variable to update with room number - if NULL, not updated
// checkOnlyCurrentRoom : if false, checks all rooms
//

mobHere *findMobHere(const uint mobNumber, uint *roomNumb, const bool checkOnlyCurrentRoom)
{
  room *roomPtr;


  if (checkOnlyCurrentRoom)
    roomPtr = g_currentRoom;
  else
    roomPtr = findRoom(getLowestRoomNumber());

  while (roomPtr)
  {
    mobHere *mobHereNode = roomPtr->mobHead;

    while (mobHereNode)
    {
      if (mobHereNode->mobNumb == mobNumber)
      {
        if (roomNumb) 
          *roomNumb = roomPtr->roomNumber;

        return mobHereNode;
      }

      mobHereNode = mobHereNode->Next;
    }

    if (checkOnlyCurrentRoom) 
      break;

    roomPtr = getNextRoom(roomPtr);
  }

  return NULL;
}


//
// copyMobHere : Copies just one mobHere, returning the address of the copy - doesn't increment any loaded counts
//
//    *srcMobHere : copiee
//

mobHere *copyMobHere(const mobHere *srcMobHere)
{
  mobHere *newMobHere;


  if (!srcMobHere) 
    return NULL;

  newMobHere = new(std::nothrow) mobHere;
  if (!newMobHere)
  {
    displayAllocError("mobHere", "copyMobHere");

    return NULL;
  }

  newMobHere->Next = NULL;


 // first, the easy stuff

  memcpy(newMobHere, srcMobHere, sizeof(mobHere));

 // now, the not quite as easy but still relatively simple stuff

  newMobHere->inventoryHead = copyObjHereList(srcMobHere->inventoryHead, false);

  for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
  {
    const objectHere *obj = srcMobHere->equipment[i];

    if (obj) 
      newMobHere->equipment[i] = copyObjHere(obj);
  }

  return newMobHere;
}


//
// copyMobHereList : Copies a mobHere list, returning the address to the head
//                   of the copy
//
//    *srcMobHere : src
//

mobHere *copyMobHereList(const mobHere *srcMobHere, const bool incLoaded)
{
  mobHere *prevMobHere = NULL, *headMobHere = NULL;


  if (!srcMobHere) 
    return NULL;

  while (srcMobHere)
  {
    mobHere *newMobHere = copyMobHere(srcMobHere);

    if (!newMobHere)
    {
      displayAllocError("mobHere", "copyMobHereList");

      return headMobHere;
    }

    newMobHere->Next = NULL;

    if (!headMobHere) 
      headMobHere = newMobHere;

    if (prevMobHere) 
      prevMobHere->Next = newMobHere;

    prevMobHere = newMobHere;

   // increment obj/mob loads

    if (incLoaded)
      addMobLoad(newMobHere);

    srcMobHere = srcMobHere->Next;
  }

  return headMobHere;
}


//
// deleteMobHereList : Deletes a mobHere list
//
//    *srcMobHere : src
//

void deleteMobHereList(mobHere *srcMobHere, const bool decLoaded)
{
  while (srcMobHere)
  {
    mobHere *nextMobHere = srcMobHere->Next;

    deleteMobHere(srcMobHere, decLoaded);

    srcMobHere = nextMobHere;
  }
}


//
// deleteMobHere : Deletes one mobHere node
//
//    mobHere : mob
//

void deleteMobHere(mobHere *mobHere, const bool decLoaded)
{
  if (!mobHere) 
    return;

 // must be done before deleting equipment/inventory

  if (decLoaded)
    removeMobLoad(mobHere);

 // delete it - decrementing loads of inventory/equip is handled by removeMobLoad(), so
 // false is passed for decLoaded regardless

  deleteObjHereList(mobHere->inventoryHead, false);

  for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
  {
    if (mobHere->equipment[i])
      deleteObjHere(mobHere->equipment[i], false);
  }

  delete mobHere;
}


//
// removeMobHerefromList : exactly like deleteMobHereinList, except it
//                         only removes the references to the record, not
//                         the record itself
//
//                         assumes mob exists in list
//

void removeMobHerefromList(mobHere **mobHead, mobHere *mob, const bool decLoaded)
{
  mobHere *prevMob;


  if (mob == (*mobHead))
  {
    *mobHead = (*mobHead)->Next;
  }
  else
  {
    prevMob = *mobHead;

    while (prevMob->Next != mob)
      prevMob = prevMob->Next;

    prevMob->Next = mob->Next;
  }

  mob->Next = NULL;

  if (decLoaded)
    removeMobLoad(mob);
}


//
// checkForMobHeresofType : returns true if any mobheres in the zone or inventory are
//                          mobType
//

bool checkForMobHeresofType(const mobType *mobType)
{
  const editableListNode *inv = g_inventory;


  if (getNumbEntities(ENTITY_MOB, mobType->mobNumber, false) > 0)
    return true;

 // check inventory

  while (inv)
  {
    if ((inv->entityType == ENTITY_MOB) && (((mobHere *)(inv->entityPtr))->mobPtr == mobType))
      return true;

    inv = inv->Next;
  }

  return false;
}


//
// addMobHeretoList : adds existing mobHere record to list
//

void addMobHeretoList(mobHere **mobListHead, mobHere *mobToAdd, const bool incLoaded)
{
  mobHere *mob;

  if (!*mobListHead) 
  {
    *mobListHead = mobToAdd;
  }
  else
  {
    mob = *mobListHead;

    while (mob->Next)
      mob = mob->Next;

    mob->Next = mobToAdd;
  }

  mobToAdd->Next = NULL;

  if (incLoaded)
    addMobLoad(mobToAdd);
}


//
// deleteAllMobHereofTypeInList :
//        Runs through a list pointed to by mobHereHead, deleting all
//        mobHeres that match mobType.
//

void deleteAllMobHereofTypeInList(mobHere **mobHereHead, const mobType *mobType, const bool decLoaded)
{
  mobHere *mobHere = *mobHereHead, *prevMob, *nextMob;


  while (mobHere)
  {
    nextMob = mobHere->Next;

    if (mobHere->mobPtr == mobType)
    {
      if (mobHere == *mobHereHead) 
      {
        *mobHereHead = mobHere->Next;
      }
      else
      {
        prevMob = *mobHereHead;

        while (prevMob->Next != mobHere)
          prevMob = prevMob->Next;

        prevMob->Next = mobHere->Next;
      }

      deleteMobHere(mobHere, decLoaded);
    }

    mobHere = nextMob;
  }
}


//
// deleteAllMobHeresofType : Deletes all mobHere nodes that match mobType
//

void deleteAllMobHeresofType(const mobType *mobType, const bool decLoaded)
{
  const uint highRoomNumb = getHighestRoomNumber();


 // delete mobs laying about and items that mobs are carrying and equipping
 // from every room

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
      deleteAllMobHereofTypeInList(&(findRoom(roomNumb)->mobHead), mobType, decLoaded);
  }
}


//
// deleteMobHereinList : Deletes a specific mobHere in a list that starts
//                       at *mobHead, updating whatever needs to be updated for
//                       the list to be valid.
//
//                       assumes mob exists in list
//

void deleteMobHereinList(mobHere **mobHead, mobHere *mob, const bool decLoaded)
{
  mobHere *prevMob;


  if (mob == (*mobHead))
  {
    *mobHead = (*mobHead)->Next;
  }
  else
  {
    prevMob = *mobHead;

    while (prevMob->Next != mob)
      prevMob = prevMob->Next;

    prevMob->Next = mob->Next;
  }

  deleteMobHere(mob, decLoaded);
}


//
// resetMobHere : Resets mobHeres with a mobNumb of oldNumb to newNumb - does not
//                reset mobPtr members
//
//   oldNumb : see above
//   newNumb : ditto
//

void resetMobHere(const uint oldNumb, const uint newNumb)
{
  room *roomPtr;
  mobHere *mob;
  const uint highRoomNumb = getHighestRoomNumber();


  if (oldNumb == newNumb) 
    return;

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      mob = roomPtr->mobHead;

      while (mob)
      {
        if (mob->mobNumb == oldNumb) 
          mob->mobNumb = newNumb;

        mob = mob->Next;
      }
    }
  }
}


//
// eqSlotFull : if given equipment slot is taken on mob, returns true
//

bool eqSlotFull(const mobHere *mob, const uint slot)
{
  if (!mob || (slot < WEAR_LOW) || (slot > WEAR_TRUEHIGH))
    return true;  // doomed

  return (mob->equipment[slot] != NULL);
}


//
// eqRestrict : returns various codes based on whether mob can wear eq and if not, why not
//

uint eqRestrict(const mobType *mob, const uint objExtra, uint objAnti, uint objAnti2)
{
  if (!mob) 
    return EQ_WEARABLE;  // allow mobHeres with no type to wear anything

 // if allowed races/classes is set, anti flag becomes allowed flag, and inversion makes it all O.K.

  if (objExtra & ITEM_ALLOWED_RACES)
    objAnti2 ^= 0xffffffff;
  
  if (objExtra & ITEM_ALLOWED_CLASSES)
    objAnti ^= 0xffffffff;

  if (getCheckMobClassEqVal() && (objAnti & mob->mobClass))
    return EQ_RESCLASS;

  if (getCheckMobRaceEqVal())
  {
   // get position of race in race_names table

    uint i;

    for (i = 1; race_names_table[i].normal; i++)
      if (strcmpnocase(mob->mobSpecies, race_names_table[i].code))
        break;

    if (i <= 32)
    {
      i--;

      if (objAnti2 & (1 << i))
        return EQ_RESRACE;
    }
  }

  return EQ_WEARABLE;
}


//
// handsFree : returns number of hands mob has free
//

char handsFree(const mobHere *mob)
{
  char hands;

  if (!mob) 
    return 0;

 // thri-kreen/four-armed have 4 hands, everyone else has 2

  if (mob->mobPtr && 
      (strcmpnocase(mob->mobPtr->mobSpecies, race_names_table[RACE_THRIKREEN].code) ||
       (mob->mobPtr->affect3Bits & AFF3_FOUR_ARMS))) 
    hands = 4;
  else 
    hands = 2;

  if (mob->equipment[HOLD]) hands--;
  if (mob->equipment[HOLD2]) hands--;
  if (mob->equipment[WIELD_PRIMARY]) hands--;
  if (mob->equipment[WIELD_SECOND]) hands--;
  if (mob->equipment[WIELD_THIRD]) hands--;
  if (mob->equipment[WIELD_FOURTH]) hands--;
  if (mob->equipment[WEAR_SHIELD]) hands--;
  if (mob->equipment[WIELD_TWOHANDS]) hands -= 2;
  if (mob->equipment[WIELD_TWOHANDS2]) hands -= 2;

  if (hands < 0) 
    hands = 0;  // who knows

  return hands;
}


//
// getMobHereEquipSlot : if mobHere can wear obj, return slot.  if not,
//                       return error
//
//                       if where is not a valid slot and obj is non-NULL,
//                       find the first slot that the item fits into and
//                       return the pos
//

uint getMobHereEquipSlot(const mobHere *mob, const objectType *obj, const int where)
{
  uint objWear;
  uint objExtra;
  bool isThrikreen = false;
  bool isMinotaur = false;
  bool isCentaur = false;
  bool noEq = false;
  bool hasFourArms = false;
  bool wearFirst;  // if 'where' val out of range, place eq in first valid slot on mob
  uint eqRes;


  if (!mob) 
    return EQ_ERROR;

 // no flags available, so simply check for objects already equipped in
 // this slot

  wearFirst = ((where < WEAR_LOW) || (where > WEAR_TRUEHIGH));

  if (!obj)
  {
    if (wearFirst) 
      return EQ_ERROR;

    if (mob->equipment[where]) 
      return EQ_SLOT_FILLED;

    return where;
  }

 // set species helpfulness

  if (mob->mobPtr) 
  {
    char mobSpecies[MAX_SPECIES_LEN + 1];

    strcpy(mobSpecies, mob->mobPtr->mobSpecies);

    if (strcmpnocase(mobSpecies, race_names_table[RACE_THRIKREEN].code))
    {
      isThrikreen = true;
      hasFourArms = true;
    }
    else if (strcmpnocase(mobSpecies, race_names_table[RACE_MINOTAUR].code))
    {
      isMinotaur = true;
    }
    else if (strcmpnocase(mobSpecies, race_names_table[RACE_CENTAUR].code))
    {
      isCentaur = true;
    }

    if (mob->mobPtr->affect3Bits & AFF3_FOUR_ARMS)
      hasFourArms = true;
  }


  objWear = obj->wearBits;
  objExtra = obj->extraBits;

 // check if object is wearable - all bits except take and throw are wear bits

  if ((objWear & ~(ITEM_TAKE | ITEM_THROW)) == 0)
  {
    return EQ_NO_WEARBITS;
  }

  eqRes = eqRestrict(mob->mobPtr, objExtra, obj->antiBits, obj->anti2Bits);
  if (eqRes != EQ_WEARABLE) 
    return eqRes;

 // if where var is valid, check if the location specified matches the wear
 // bits available on the object

  if (!wearFirst)
  {
    eqRes = eqRaceRestrict(mob->mobPtr, obj, where);
    if (eqRes != EQ_WEARABLE)
      return eqRes;

   // in this first chunk, just check if the object has the proper
   // wear bits set for the particular slot requested (if not, set noEq)

    switch (where)
    {
      case WEAR_FINGER_L :
      case WEAR_FINGER_R : if (~objWear & ITEM_WEAR_FINGER) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_NECK_1   :
      case WEAR_NECK_2   : if (~objWear & ITEM_WEAR_NECK) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_BODY     : if ((~objWear & ITEM_WEAR_BODY) && (~objExtra & ITEM_WHOLE_BODY))
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_HEAD     : if ((~objWear & ITEM_WEAR_HEAD) && (~objExtra & ITEM_WHOLE_HEAD))
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_LEGS     : if (~objWear & ITEM_WEAR_LEGS) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_FEET     : if (~objWear & ITEM_WEAR_FEET) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_HANDS_2   :
      case WEAR_HANDS    : if (~objWear & ITEM_WEAR_HANDS) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_ARMS_2    :
      case WEAR_ARMS     : if (~objWear & ITEM_WEAR_ARMS) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_SHIELD   : if (~objWear & ITEM_WEAR_SHIELD) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_ABOUT    : if (~objWear & ITEM_WEAR_ABOUT) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_WAIST    : if (~objWear & ITEM_WEAR_WAIST) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_WRIST_LL :
      case WEAR_WRIST_LR :
      case WEAR_WRIST_R  :
      case WEAR_WRIST_L  : if (~objWear & ITEM_WEAR_WRIST) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WIELD_FOURTH  :
      case WIELD_THIRD   :
      case WIELD_SECOND  :
      case WIELD_PRIMARY : if ((~objWear & ITEM_WIELD) && (~objExtra & ITEM_TWOHANDS))
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case HOLD2         :
      case HOLD          : if (~objWear & ITEM_HOLD) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_EYES     : if (~objWear & ITEM_WEAR_EYES) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_FACE     : if (~objWear & ITEM_WEAR_FACE) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_EARRING_R:
      case WEAR_EARRING_L: if (~objWear & ITEM_WEAR_EARRING) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_QUIVER   : if (~objWear & ITEM_WEAR_QUIVER) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_BADGE    : if (~objWear & ITEM_GUILD_INSIGNIA) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_WHOLEBODY: if (~objExtra & ITEM_WHOLE_BODY) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_WHOLEHEAD: if (~objExtra & ITEM_WHOLE_HEAD) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WIELD_TWOHANDS2:
      case WIELD_TWOHANDS: if (~objExtra & ITEM_TWOHANDS) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_BACK     : if (~objWear & ITEM_WEAR_BACK) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_ATTACH_BELT_3 :
      case WEAR_ATTACH_BELT_2 :
      case WEAR_ATTACH_BELT_1 :
                           if (~objWear & ITEM_ATTACH_BELT) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_HORSE_BODY :
                           if (~objWear & ITEM_HORSE_BODY) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_TAIL     : if (~objWear & ITEM_WEAR_TAIL) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_NOSE     : if (~objWear & ITEM_WEAR_NOSE) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_HORN     : if (~objWear & ITEM_WEAR_HORN) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
      case WEAR_IOUN     : if (~objWear & ITEM_WEAR_IOUN) 
                             noEq = true;
                           else 
                             noEq = false;
                           break;
    }

    if (noEq) 
      return EQ_WEAR_NOTSET;
  }

  const char numbHandsFree = handsFree(mob);

 // if 'where' is a valid position, then the check above has already verified that the object has valid
 // wear bits, so now just check against the location

 // need to check for whole body/head flags on non-whole body/head objs too -
 // simplest way is to set equip pos to WEAR_WHOLEBODY and HEAD internally
 // and write them out as WEAR_BODY and HEAD externally

  if ((objExtra & ITEM_WHOLE_BODY) && !eqSlotFull(mob, WEAR_BODY) && !eqSlotFull(mob, WEAR_LEGS) &&
      !eqSlotFull(mob, WEAR_ARMS) && !eqSlotFull(mob, WEAR_WHOLEBODY) && !isThrikreen && !isMinotaur && 
      !isCentaur && (wearFirst || (where == WEAR_BODY) || (where == WEAR_WHOLEBODY)))
  {
    return WEAR_WHOLEBODY;
  }
  else
  if ((objExtra & ITEM_WHOLE_HEAD) && !eqSlotFull(mob, WEAR_HEAD) && !eqSlotFull(mob, WEAR_EYES) &&
      !eqSlotFull(mob, WEAR_FACE) && !eqSlotFull(mob, WEAR_WHOLEHEAD) && !isMinotaur &&
      (wearFirst || (where == WEAR_HEAD) || (where == WEAR_WHOLEHEAD)))
  {
    return WEAR_WHOLEHEAD;
  }
  else
  if ((objExtra & ITEM_TWOHANDS) && (numbHandsFree >= 2) &&
      (wearFirst || (where == WIELD_TWOHANDS) || (where == WIELD_TWOHANDS2) || (where == WIELD) || 
       (where == WIELD2)))
  {
    if (!eqSlotFull(mob, WIELD_TWOHANDS)) 
      return WIELD_TWOHANDS;
    else
      return WIELD_TWOHANDS2;
  }
  else
  if ((objWear & ITEM_WEAR_FINGER) && !eqSlotFull(mob, WEAR_FINGER_R) && !isThrikreen &&
      (wearFirst || (where == WEAR_FINGER_R)))
  {
    return WEAR_FINGER_R;
  }
  else
  if ((objWear & ITEM_WEAR_FINGER) && !eqSlotFull(mob, WEAR_FINGER_L) && !isThrikreen &&
      (wearFirst || (where == WEAR_FINGER_L)))
  {
    return WEAR_FINGER_L;
  }
  else
  if ((objWear & ITEM_WEAR_WRIST) && !eqSlotFull(mob, WEAR_WRIST_R) && (wearFirst || (where == WEAR_WRIST_R)))
  {
    return WEAR_WRIST_R;
  }
  else
  if ((objWear & ITEM_WEAR_WRIST) && !eqSlotFull(mob, WEAR_WRIST_L) && (wearFirst || (where == WEAR_WRIST_L)))
  {
    return WEAR_WRIST_L;
  }
  else

 // checked 'normal' wrist slots, now if mob has four arms check lower slots

  if ((objWear & ITEM_WEAR_WRIST) && hasFourArms && !eqSlotFull(mob, WEAR_WRIST_LR) && 
      (wearFirst || (where == WEAR_WRIST_LR)))
  {
    return WEAR_WRIST_LR;
  }
  else
  if ((objWear & ITEM_WEAR_WRIST) && hasFourArms && !eqSlotFull(mob, WEAR_WRIST_LL) && 
      (wearFirst || (where == WEAR_WRIST_LL)))
  {
    return WEAR_WRIST_LL;
  }
  else
  if ((objWear & ITEM_WEAR_NECK) && !eqSlotFull(mob, WEAR_NECK_1) && (wearFirst || (where == WEAR_NECK_1)))
  {
    return WEAR_NECK_1;
  }
  else
  if ((objWear & ITEM_WEAR_NECK) && !eqSlotFull(mob, WEAR_NECK_2) && (wearFirst || (where == WEAR_NECK_2)))
  {
    return WEAR_NECK_2;
  }
  else
  if ((objWear & ITEM_WEAR_EARRING) && !eqSlotFull(mob, WEAR_EARRING_R) && !isThrikreen && 
      (wearFirst || (where == WEAR_EARRING_R)))
  {
    return WEAR_EARRING_R;
  }
  else
  if ((objWear & ITEM_WEAR_EARRING) && !eqSlotFull(mob, WEAR_EARRING_L) && !isThrikreen &&
      (wearFirst || (where == WEAR_EARRING_L)))
  {
    return WEAR_EARRING_L;
  }
  else
  if ((objWear & ITEM_WEAR_BODY) && !eqSlotFull(mob, WEAR_BODY) && !eqSlotFull(mob, WEAR_WHOLEBODY) && 
      !isThrikreen && (!((objExtra & ITEM_WHOLE_BODY) && (isMinotaur || isCentaur))) &&
      (wearFirst || (where == WEAR_BODY)))
  {
    return WEAR_BODY;
  }
  else
  if ((objWear & ITEM_WEAR_HEAD) && !eqSlotFull(mob, WEAR_HEAD) && !eqSlotFull(mob, WEAR_WHOLEHEAD) && 
      !isMinotaur && (wearFirst || (where == WEAR_HEAD)))
  {
    return WEAR_HEAD;
  }
  else
  if ((objWear & ITEM_WEAR_LEGS) && !eqSlotFull(mob, WEAR_LEGS) && !eqSlotFull(mob, WEAR_WHOLEBODY) && 
      !isMinotaur && !isCentaur && (wearFirst || (where == WEAR_LEGS)))
  {
    return WEAR_LEGS;
  }
  else
  if ((objWear & ITEM_WEAR_FEET) && !eqSlotFull(mob, WEAR_FEET) && !isMinotaur && !isThrikreen && 
      (!isCentaur || scanKeyword("horseshoe", obj->keywordListHead)) && (wearFirst || (where == WEAR_FEET)))
  {
    return WEAR_FEET;
  }
  else
  if ((objWear & ITEM_WEAR_HANDS) && !eqSlotFull(mob, WEAR_HANDS) && (wearFirst || (where == WEAR_HANDS)))
  {
    return WEAR_HANDS;
  }
  else
  if ((objWear & ITEM_WEAR_ARMS) && !eqSlotFull(mob, WEAR_ARMS) && !eqSlotFull(mob, WEAR_WHOLEBODY) &&
      (wearFirst || (where == WEAR_ARMS)))
  {
    return WEAR_ARMS;
  }
  else
  if ((objWear & ITEM_WEAR_ARMS) && hasFourArms && !eqSlotFull(mob, WEAR_ARMS_2) &&
      (wearFirst || (where == WEAR_ARMS_2)))
  {
    return WEAR_ARMS_2;
  }
  else
  if ((objWear & ITEM_WEAR_SHIELD) && !eqSlotFull(mob, WEAR_SHIELD) && numbHandsFree &&
      (wearFirst || (where == WEAR_SHIELD)))
  {
    return WEAR_SHIELD;
  }
  else
  if ((objWear & ITEM_WEAR_BACK) && !eqSlotFull(mob, WEAR_BACK) && (wearFirst || (where == WEAR_BACK)))
  {
    return WEAR_BACK;
  }
  else
  if ((objWear & ITEM_WEAR_ABOUT) && !eqSlotFull(mob, WEAR_ABOUT) && (wearFirst || (where == WEAR_ABOUT)))
  {
    return WEAR_ABOUT;
  }
  else
  if ((objWear & ITEM_WEAR_WAIST) && !eqSlotFull(mob, WEAR_WAIST) && !eqSlotFull(mob, WEAR_WHOLEBODY) &&
      (wearFirst || (where == WEAR_WAIST)))
  {
    return WEAR_WAIST;
  }
  else
  if ((objWear & ITEM_WIELD) && !eqSlotFull(mob, WIELD_PRIMARY) && numbHandsFree &&
      (wearFirst || (where == WIELD_PRIMARY)))
  {
    return WIELD_PRIMARY;
  }
  else
  if ((objWear & ITEM_WIELD) && !eqSlotFull(mob, WIELD_SECOND) && numbHandsFree &&
      (wearFirst || (where == WIELD_SECOND)))
  {
    return WIELD_SECOND;
  }
  else
  if ((objWear & ITEM_WIELD) && !eqSlotFull(mob, WIELD_THIRD) && numbHandsFree &&
      (wearFirst || (where == WIELD_THIRD)))
  {
    return WIELD_THIRD;
  }
  else
  if ((objWear & ITEM_WIELD) && !eqSlotFull(mob, WIELD_FOURTH) && numbHandsFree &&
      (wearFirst || (where == WIELD_FOURTH)))
  {
    return WIELD_FOURTH;
  }
  else
  if ((objWear & ITEM_WEAR_EYES) && !eqSlotFull(mob, WEAR_EYES) && !eqSlotFull(mob, WEAR_WHOLEHEAD) &&
      (wearFirst || (where == WEAR_EYES)))
  {
    return WEAR_EYES;
  }
  else
  if ((objWear & ITEM_WEAR_FACE) && !eqSlotFull(mob, WEAR_FACE) && !eqSlotFull(mob, WEAR_WHOLEHEAD) && 
      (wearFirst || (where == WEAR_FACE)))
  {
    return WEAR_FACE;
  }
  else
  if ((objWear & ITEM_GUILD_INSIGNIA) && !eqSlotFull(mob, WEAR_BADGE) && (wearFirst || (where == WEAR_BADGE)))
  {
    return WEAR_BADGE;
  }
  else
  if ((objWear & ITEM_WEAR_QUIVER) && !eqSlotFull(mob, WEAR_QUIVER) && (wearFirst || (where == WEAR_QUIVER)))
  {
    return WEAR_QUIVER;
  }
  else
  if ((objWear & ITEM_HORSE_BODY) && !eqSlotFull(mob, WEAR_HORSE_BODY) && isCentaur &&
      (wearFirst || (where == WEAR_HORSE_BODY)))
  {
    return WEAR_HORSE_BODY;
  }
  else
  if ((objWear & ITEM_WEAR_TAIL) && !eqSlotFull(mob, WEAR_TAIL) && (isCentaur || isMinotaur) &&
      (wearFirst || (where == WEAR_TAIL)))
  {
    return WEAR_TAIL;
  }
  else
  if ((objWear & ITEM_WEAR_NOSE) && !eqSlotFull(mob, WEAR_NOSE) && isMinotaur &&
      (wearFirst || (where == WEAR_NOSE)))
  {
    return WEAR_NOSE;
  }
  else
  if ((objWear & ITEM_WEAR_HORN) && !eqSlotFull(mob, WEAR_HORN) && isMinotaur &&
      (wearFirst || (where == WEAR_HORN)))
  {
    return WEAR_HORN;
  }
  else
  if ((objWear & ITEM_WEAR_IOUN) && !eqSlotFull(mob, WEAR_IOUN) && (wearFirst || (where == WEAR_IOUN)))
  {
    return WEAR_IOUN;
  }
  else
  if ((objWear & ITEM_ATTACH_BELT) && eqSlotFull(mob, WEAR_WAIST) && 
      (!eqSlotFull(mob, WEAR_ATTACH_BELT_1) || !eqSlotFull(mob, WEAR_ATTACH_BELT_2) || 
       !eqSlotFull(mob, WEAR_ATTACH_BELT_3)) &&
      (wearFirst || (where == WEAR_ATTACH_BELT_1) || (where == WEAR_ATTACH_BELT_2) || 
       (where == WEAR_ATTACH_BELT_3)))
  {
    if (!eqSlotFull(mob, WEAR_ATTACH_BELT_1)) 
      return WEAR_ATTACH_BELT_1;
    else if (!eqSlotFull(mob, WEAR_ATTACH_BELT_2)) 
      return WEAR_ATTACH_BELT_2;
    else 
      return WEAR_ATTACH_BELT_3;
  }
  else
  if ((objWear & ITEM_HOLD) && numbHandsFree && (wearFirst || (where == HOLD) || (where == HOLD2)))
  {
    if (!eqSlotFull(mob, HOLD)) 
      return HOLD;
    else if (!eqSlotFull(mob, HOLD2)) 
      return HOLD2;
  }

  return EQ_SLOT_FILLED;
}


//
// eqRaceRestrict : returns error code if mob can't use obj due to race, otherwise returns 0
//

uint eqRaceRestrict(const mobType *mob, const objectType *obj, const uint where)
{
  bool isThrikreen = false;
  bool isCentaur = false;
  bool isMinotaur = false;
  bool hasFourArms = false;


  if (strcmpnocase(mob->mobSpecies, race_names_table[RACE_THRIKREEN].code))
  {
    isThrikreen = true;
    hasFourArms = true;
  }
  else if (strcmpnocase(mob->mobSpecies, race_names_table[RACE_MINOTAUR].code))
  {
    isMinotaur = true;
  }
  else if (strcmpnocase(mob->mobSpecies, race_names_table[RACE_CENTAUR].code))
  {
    isCentaur = true;
  }

  if (mob->affect3Bits & AFF3_FOUR_ARMS)
    hasFourArms = true;

  switch (where)
  {
    case WIELD_THIRD     :
    case WIELD_FOURTH    :
    case WEAR_ARMS_2     :
    case WEAR_HANDS_2    :
    case WEAR_WRIST_LR   :
    case WEAR_WRIST_LL   : if (!hasFourArms)
                             return EQ_WRONGRACE;
                           break;

    case WEAR_FINGER_L   :
    case WEAR_FINGER_R   :
    case WEAR_EARRING_L  :
    case WEAR_EARRING_R  : if (isThrikreen)
                             return EQ_RACECANTUSE;
                           break;

    case WEAR_BODY       : if (isThrikreen)
                             return EQ_RACECANTUSE;

                           if ((obj->extraBits & ITEM_WHOLE_BODY) &&
                               (isCentaur || isMinotaur))
                             return EQ_RACECANTUSE;
                           break;

    case WEAR_FEET       : if (isThrikreen || isMinotaur ||
                               (isCentaur && !scanKeyword("horseshoe", obj->keywordListHead)))
                            return EQ_RACECANTUSE;
                           break;

    case WEAR_WHOLEHEAD  :
    case WEAR_HEAD       : if (isMinotaur)
                             return EQ_RACECANTUSE;
                           break;

    case WEAR_WHOLEBODY  : if (isThrikreen || isMinotaur || isCentaur)
                             return EQ_RACECANTUSE;
                           break;

    case WEAR_HORSE_BODY : if (!isCentaur)
                             return EQ_WRONGRACE;
                           break;

    case WEAR_TAIL       : if (!isCentaur && !isMinotaur)
                             return EQ_WRONGRACE;
                           break;

    case WEAR_HORN       :
    case WEAR_NOSE       : if (!isMinotaur)
                             return EQ_WRONGRACE;
                           break;

    case WEAR_LEGS       : if (isCentaur)
                             return EQ_RACECANTUSE;
                           break;
  }

  if ((where == WEAR_FEET_REAR) || (where == WEAR_LEGS_REAR))
    return EQ_WRONGRACE;  // not implemented, apparently

  return EQ_WEARABLE;  // mob can use
}


//
// canMobTypeEquip : checks align/class and position, for race
//

uint canMobTypeEquip(const mobType *mob, const objectType *obj, const uint where)
{
  uint objWear;
  uint objExtra;
  uint eqRes;


  if (!mob || !obj) 
    return EQ_ERROR;

  objWear = obj->wearBits;
  objExtra = obj->extraBits;

  if ((objWear & ~(ITEM_TAKE | ITEM_THROW)) == 0)
  {
    return EQ_NO_WEARBITS;
  }

  eqRes = eqRaceRestrict(mob, obj, where);

  if (eqRes != EQ_WEARABLE)
    return eqRes;

  return eqRestrict(mob, objExtra, obj->antiBits, obj->anti2Bits);
}


//
// getCanEquipErrStrn : returns error strings for equipping errors, but not
//                      valid equip codes
//

const char *getCanEquipErrStrn(const uint err, char *strn)
{
  switch (err)
  {
    case EQ_SLOT_FILLED : return "mob has no free slot for that object";
    case EQ_RESCLASS    : return "mob cannot use equipment due to class";
    case EQ_RESRACE     : return "mob cannot use equipment due to race (anti2 flags)";
    case EQ_NO_WEARBITS : return "object has no valid wear flags";
    case EQ_ERROR       : return "error checking mob or object";
    case EQ_WEAR_NOTSET : return "correct wear flag not set for position";
    case EQ_WRONGRACE   : return "wrong race for position";
    case EQ_RACECANTUSE : return "race can't use equipment in that slot";
    case EQ_NOBELTATTCH : return "trying to attach something to belt, but no belt";

    default : sprintf(strn, "err #%u", err);
              return strn;
  }
}


//
// getVerboseCanEquipErrorStrn - redundancies for getVerboseCanEquipStrn
//

const char *getVerboseCanEquipErrorStrn(const uint err, char *strn)
{
  char errstrn[256];

  sprintf(strn, "Cannot equip that obj on this mob - %s.\n", getCanEquipErrStrn(err, errstrn));

  return strn;
}


//
// getVerboseCanEquipEquippedStrn
//

const char *getVerboseCanEquipEquippedStrn(const uint where, const char *objName, char *strn)
{
  const char *slotNames[WEAR_TRUEHIGH + 1] =
  {
    "light (not imped)",
    "right finger slot",
    "left finger slot",
    "first neck slot",
    "second neck slot",
    "body slot",
    "head slot",
    "legs slot",
    "feet slot",
    "hands slot",
    "arms slot",
    "shield slot",
    "about body slot",
    "waist slot",
    "right wrist slot",
    "left wrist slot",
    "primary weapon slot",
    "secondary weapon slot",
    "hold slot",
    "eyes slot",
    "face slot",
    "right earring slot",
    "left earring slot",
    "quiver slot",
    "badge slot",
    "tertiary weapon slot",
    "quaternary weapon slot",
    "back slot",
    "first belt attach slot",
    "second belt attach slot",
    "third belt attach slot",
    "lower arms slot",
    "lower hands slot",
    "lower right wrist slot",
    "lower left wrist slot",
    "horse body",
    "rear legs slot",
    "tail slot",
    "rear feet slot",
    "nose slot",
    "horn slot",
    "ioun slot",
    "entire body",
    "entire head",
    "two-handed slot",
    "lower two-handed slot",
    "lower hold slot"
  };

  sprintf(strn, "'%s&n' equipped on mob's %s.\n", objName, slotNames[where]);

  return strn;
}


//
// getVerboseCanEquipStrn
//

const char *getVerboseCanEquipStrn(const uint val, const char *objName, char *strn)
{
  switch (val)
  {
    case EQ_SLOT_FILLED : 
    case EQ_RESCLASS    : 
    case EQ_RESRACE     : 
    case EQ_NO_WEARBITS : 
    case EQ_WEAR_NOTSET :
    case EQ_ERROR       :
    case EQ_WRONGRACE   :
    case EQ_RACECANTUSE :
    case EQ_NOBELTATTCH :
      return getVerboseCanEquipErrorStrn(val, strn);

    case WEAR_FINGER_R  :
    case WEAR_FINGER_L  :
    case WEAR_NECK_1    :
    case WEAR_NECK_2    :
    case WEAR_BODY      :
    case WEAR_WHOLEBODY :
    case WEAR_HEAD      :
    case WEAR_WHOLEHEAD :
    case WEAR_LEGS      :
    case WEAR_FEET      :
    case WEAR_HANDS     :
    case WEAR_ARMS      :
    case WEAR_SHIELD    :
    case WEAR_ABOUT     :
    case WEAR_WAIST     :
    case WEAR_WRIST_R   :
    case WEAR_WRIST_L   :
    case WIELD_PRIMARY  :
    case WIELD_SECOND   :
    case WIELD_TWOHANDS :
    case WIELD_TWOHANDS2:
    case HOLD           :
    case HOLD2          :
    case WEAR_EYES      :
    case WEAR_FACE      :
    case WEAR_EARRING_R :
    case WEAR_EARRING_L :
    case WEAR_BADGE     :
    case WEAR_QUIVER    :
    case WIELD_THIRD    :
    case WIELD_FOURTH   :
    case WEAR_BACK      :
    case WEAR_ATTACH_BELT_1:
    case WEAR_ATTACH_BELT_2:
    case WEAR_ATTACH_BELT_3:
    case WEAR_ARMS_2    :
    case WEAR_HANDS_2   :
    case WEAR_WRIST_LR  :
    case WEAR_WRIST_LL  :
    case WEAR_HORSE_BODY:
    case WEAR_LEGS_REAR :
    case WEAR_TAIL      :
    case WEAR_FEET_REAR :
    case WEAR_NOSE      :
    case WEAR_HORN      :
    case WEAR_IOUN      :
      return getVerboseCanEquipEquippedStrn(val, objName, strn);

    default : 
      sprintf(strn, "err #%u\n", val);
      return strn;
  }
}


//
// mobsCanEquipEquipped : checks if all mobs can equip what they are currently equipping, based on race, sex, 
//                        alignment - if fixEq is true, moves any invalid eq to inventory and always 
//                        returns true, otherwise returns false if invalid eq found
//

bool mobsCanEquipEquipped(const bool fixEq)
{
  uint numb = getLowestRoomNumber();
  const uint high = getHighestRoomNumber();


 // run through all valid rooms, looking for loaded mobs

  while (numb <= high)
  {
    room *room = findRoom(numb);
    if (room)
    {
     // run through mobs, mob by mob

      mobHere *mob = room->mobHead;
      while (mob)
      {
       // run through objects, obj by obj

        for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
        {
          uint val;

          if (mob->equipment[i] && mob->equipment[i]->objectPtr)
          {
            objectHere *tmpObj = mob->equipment[i];
            mob->equipment[i] = NULL;

            val = canMobTypeEquip(mob->mobPtr, tmpObj->objectPtr, i);

            mob->equipment[i] = tmpObj;
          }
          else continue;

          if (val > WEAR_TRUEHIGH)
          {
            if (!fixEq) 
            {
              return false;
            }
            else
            {
              addObjHeretoList(&(mob->inventoryHead), mob->equipment[i], false);
              mob->equipment[i] = NULL;
            }
          }
        }

        mob = mob->Next;
      }
    }

    numb++;
  }

  return true;
}


//
// mobEqinCorrectSlot: checks if all mobs' equipment is in the correct slot - if fixEq is true,
//                     moves any invalid eq to inventory and always returns true, otherwise returns
//                     false if invalid eq found
//

bool mobsEqinCorrectSlot(const bool fixEq)
{
  uint numb = getLowestRoomNumber();
  const uint high = getHighestRoomNumber();


  while (numb <= high)
  {
    room *room = findRoom(numb);
    if (room)
    {
      mobHere *mob = room->mobHead;
      while (mob)
      {
        for (uint i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
        {
          uint val;

          if (mob->equipment[i])
          {
            objectHere *tmpObj = mob->equipment[i];
            mob->equipment[i] = NULL;

            val = getMobHereEquipSlot(mob, tmpObj->objectPtr, i);

            mob->equipment[i] = tmpObj;
          }
          else continue;

          if (val > WEAR_TRUEHIGH)
          {
            if (!fixEq) 
            {
              return false;
            }
            else
            {
              addObjHeretoList(&(mob->inventoryHead), mob->equipment[i], false);
              mob->equipment[i] = NULL;
            }
          }
        }

        mob = mob->Next;
      }
    }

    numb++;
  }

  return true;
}


//
// mobEquippingSomething : returns true if mob is equipping something
//

bool mobEquippingSomething(const mobHere *mob)
{
  if (!mob) 
    return false;

  for (int i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
    if (mob->equipment[i]) 
      return true;

  return false;
}


//
// getNumbFollowers : returns number of mobs following specific mob
//

uint getNumbFollowers(const mobHere *mob)
{
  uint numb = 0;
  const uint high = getHighestRoomNumber();


  if (!mob) 
    return 0;

  for (uint i = getLowestRoomNumber(); i <= high; i++)
  {
    if (roomExists(i))
    {
      mobHere *mobN = findRoom(i)->mobHead;

      while (mobN)
      {
        if (mobN->following == mob) 
          numb++;

        mobN = mobN->Next;
      }
    }
  }

  return numb;
}


//
// createMobHereinRoom : Create a new mobHere in the current room - 
//                       returns false if not created
//
//    *mob : mob type of mobHere
//    numb : vnum of mob being created, because mob can be NULL
//

bool createMobHereinRoom(mobType *mob, const uint numb)
{
  mobHere *newMobHere;


 // if mob type is a shop, check for properness

  if (mob && mob->shopPtr)
  {
   // don't load more than one of the same type of shop

    if (getNumbEntities(ENTITY_MOB, mob->mobNumber, false))
    {
      _outtext("\n"
"Error: Already one of this mob loaded - since it has a shop, no more than\n"
"       one can be loaded.  Aborting.\n\n");

      return false;
    }

   // don't put more than one shop in a room

    if (getShopinCurrentRoom())
    {
      _outtext("\n"
"Error: This room already has a shop - there can only be one shop per room.\n"
"       Aborting.\n\n");

      return false;
    }
  }

 // create a new mobHere

  newMobHere = new(std::nothrow) mobHere;
  if (!newMobHere)
  {
    displayAllocError("mobHere", "createMobHereinRoom");

    return false;
  }

  memset(newMobHere, 0, sizeof(mobHere));

  newMobHere->mobNumb = numb;
  newMobHere->mobPtr = mob;

  newMobHere->randomChance = 100;

 // tack new node onto end of list

  addMobHeretoList(&g_currentRoom->mobHead, newMobHere, true);

 // rebuild master and editable list of current room

  deleteMasterKeywordList(g_currentRoom->masterListHead);
  g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

  deleteEditableList(g_currentRoom->editableListHead);
  g_currentRoom->editableListHead = createEditableList(g_currentRoom);

  return true;
}


//
// getZoneCommandEquipPos : convert positions used in DE to positions used in .ZON file, i.e.
//                          WEAR_WHOLEBODY to WEAR_BODY - most are unaffected
//

uint getZoneCommandEquipPos(const uint eqpos)
{
  uint newpos = eqpos;

  if (eqpos == WEAR_WHOLEBODY) 
    newpos = WEAR_BODY;
  else 
  if (eqpos == WEAR_WHOLEHEAD) 
    newpos = WEAR_HEAD;
  else
  if (eqpos == WIELD_TWOHANDS) 
    newpos = WIELD_PRIMARY;
  else 
  if (eqpos == WIELD_TWOHANDS2) 
    newpos = WIELD_SECOND;
  else 
  if (eqpos == HOLD2) 
    newpos = HOLD;

  return newpos;
}
