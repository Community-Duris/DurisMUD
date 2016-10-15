//
//  File: object.cpp     originally part of durisEdit
//
//  Usage: multitudes of functions for use with objects
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
#include <ctype.h>

#include "../types.h"
#include "../fh.h"
#include "../spells.h"

#include "object.h"
#include "objhere.h"



extern objectType **g_objLookup;
extern uint g_lowestObjNumber, g_highestObjNumber, g_numbLookupEntries, g_numbObjTypes;
extern editableListNode *g_inventory;
extern bool g_madeChanges;
extern const char *g_exitnames[];
extern room *g_currentRoom;

extern Skill skills[MAX_AFFECT_TYPES];

//
// noObjTypesExist
//

bool noObjTypesExist(void)
{
  return (getLowestObjNumber() > getHighestObjNumber());
}


//
// getObjShortName
//

const char *getObjShortName(const objectType *obj)
{
  if (!obj)
    return "(object type not in this .obj file)";
  else
    return obj->objShortName;
}


//
// findObj : Returns the address of the object node that has the requested
//           objNumber (if any)
//
//   objNumber : object number to return
//

objectType *findObj(const uint objNumber)
{
  if (objNumber >= g_numbLookupEntries) 
    return NULL;

  return g_objLookup[objNumber];
}


//
// objExists : Returns true if object with objNumber exists
//
//   objNumber : object number to seek
//

bool objExists(const uint objNumber)
{
  if (objNumber >= g_numbLookupEntries) 
    return false;

  return (g_objLookup[objNumber] != NULL);
}


//
// copyObjectType : copies all the info from an object type record into a new
//                  record and returns the address of the new record
//
//        *srcObj : source object type record
// incNumbObjects : if TRUE, increments the global g_numbObjTypes var
//

objectType *copyObjectType(const objectType *srcObj, const bool incNumbObjects)
{
  objectType *newObj;


 // make sure src exists

  if (!srcObj) 
    return NULL;

 // alloc mem for new rec

  newObj = new(std::nothrow) objectType;
  if (!newObj)
  {
    displayAllocError("objectType", "copyObjectType");

    return NULL;
  }

 // first, copy the simple stuff

  memcpy(newObj, srcObj, sizeof(objectType));

 // next, the not-so-simple stuff

  newObj->keywordListHead = copyStringNodes(srcObj->keywordListHead);

 // extra desc linked list

  newObj->extraDescHead = copyExtraDescs(srcObj->extraDescHead);

  if (incNumbObjects)
  {
    g_numbObjTypes++;
    
    createPrompt();
  }

 // return the address of the new object

  return newObj;
}


//
// compareObjectApply : FALSE no match TRUE match
//
//   *app1 : first record to compare
//   *app2 : second record
//

bool compareObjectApply(const objApplyRec *app1, const objApplyRec *app2)
{
  if (!memcmp(app1, app2, sizeof(objApplyRec))) 
    return true;
  else 
    return false;
}


//
// compareObjectType : compares almost all - returns FALSE if they don't
//                     match, TRUE if they do
//
//                     doesn't compare defaultObj var
//

bool compareObjectType(const objectType *obj1, const objectType *obj2)
{
  if (obj1 == obj2) 
    return true;

  if (!obj1 || !obj2) 
    return false;

 // check all object attributes

  if (strcmp(obj1->objShortName, obj2->objShortName)) 
    return false;
  if (strcmp(obj1->objLongName, obj2->objLongName)) 
    return false;

  if (obj1->objNumber != obj2->objNumber) 
    return false;

  if (obj1->extraBits != obj2->extraBits)
    return false;
  if (obj1->wearBits != obj2->wearBits)
    return false;
  if (obj1->extra2Bits != obj2->extra2Bits)
    return false;
  if (obj1->antiBits != obj2->antiBits)
    return false;
  if (obj1->anti2Bits != obj2->anti2Bits)
    return false;

  if (obj1->affect1Bits != obj2->affect1Bits)
    return false;
  if (obj1->affect2Bits != obj2->affect2Bits)
    return false;
  if (obj1->affect3Bits != obj2->affect3Bits)
    return false;
  if (obj1->affect4Bits != obj2->affect4Bits)
    return false;

  if (obj1->objType != obj2->objType) 
    return false;
  if (obj1->material != obj2->material) 
    return false;
  if (obj1->size != obj2->size) 
    return false;
  if (obj1->space != obj2->space) 
    return false;
  if (obj1->craftsmanship != obj2->craftsmanship) 
    return false;
  if (obj1->damResistBonus != obj2->damResistBonus) 
    return false;
  if (obj1->weight != obj2->weight) 
    return false;
  if (obj1->worth != obj2->worth) 
    return false;
  if (obj1->condition != obj2->condition) 
    return false;

  for (uint i = 0; i < NUMB_OBJ_APPLIES; i++)
  {
    if (!compareObjectApply(&(obj1->objApply[i]), &(obj2->objApply[i])))
      return false; 
  }

  if (memcmp(obj1->objValues, obj2->objValues, sizeof(int) * NUMB_OBJ_VALS))
    return false;

  if (obj1->trapBits != obj2->trapBits) 
    return false;
  if (obj1->trapDam != obj2->trapDam) 
    return false;
  if (obj1->trapCharge != obj2->trapCharge) 
    return false;
  if (obj1->trapLevel != obj2->trapLevel) 
    return false;

 // description and extra descs

  if (!compareStringNodesIgnoreCase(obj1->keywordListHead, obj2->keywordListHead))
    return false;

  if (!compareExtraDescs(obj1->extraDescHead, obj2->extraDescHead))
    return false;


  return true;
}


//
// getHighestObjNumber : Gets the highest object number
//

uint getHighestObjNumber(void)
{
  return g_highestObjNumber;
}


//
// getLowestObjNumber : Gets the lowest object number
//

uint getLowestObjNumber(void)
{
  return g_lowestObjNumber;
}


//
// getFirstFreeObjNumber : Starts at lowest obj number + 1, loops up to highest - 1,
//                         returning the first number with no object type
//

uint getFirstFreeObjNumber(void)
{
  for (uint i = g_lowestObjNumber + 1; i <= g_highestObjNumber - 1; i++)
  {
    if (!g_objLookup[i]) 
      return i;
  }

  return g_highestObjNumber + 1;
}


//
// checkAndFixRefstoObj : reset any object value refs (i.e. container key), room exit
//                        refs, quest and shop obj vnum refs from oldNumb to newNumb
//

void checkAndFixRefstoObj(const uint oldNumb, const uint newNumb)
{
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestMobNumber();


 // scan through object types and room exits

 // fix object field refs

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      objectType *obj = findObj(objNumb);

      for (uint i = 0; i < NUMB_OBJ_VALS; i++)
      {
        if ((fieldRefsObjNumb(obj->objType, i) && (obj->objValues[i] == oldNumb)))
        {
          obj->objValues[i] = newNumb;
        }
      }
    }
  }

 // fix keynumb refs

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (roomPtr->exits[i] && roomPtr->exits[i]->keyNumb && (roomPtr->exits[i]->keyNumb == oldNumb))
          roomPtr->exits[i]->keyNumb = newNumb;
      }
    }
  }

 // fix quest refs

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      if (mob->questPtr)
      {
        questQuest *qst = mob->questPtr->questHead;

        while (qst)
        {
          questItem *item = qst->questPlayRecvHead;
          while (item)
          {
            if ((item->itemType == QUEST_RITEM_OBJ) && (item->itemVal == oldNumb))
            {
              item->itemVal = newNumb;
            }

            item = item->Next;
          }

          item = qst->questPlayGiveHead;
          while (item)
          {
            if ((item->itemType == QUEST_GITEM_OBJ) && (item->itemVal == oldNumb))
            {
              item->itemVal = newNumb;
            }

            item = item->Next;
          }

          qst = qst->Next;
        }
      }
    }
  }

 // fix shop refs

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      if (mob->shopPtr)
      {
        for (uint i = 0; (i < MAX_NUMBSHOPITEMS) && mob->shopPtr->producedItemList[i]; i++)
        {
          if (mob->shopPtr->producedItemList[i] == oldNumb)
          {
            mob->shopPtr->producedItemList[i] = newNumb;
          }
        }
      }
    }
  }
}


//
// renumberObjs : Renumbers the objs so that there are no "gaps" - starts
//                at the first obj and simply renumbers from there
//

void renumberObjs(const bool renumberHead, const uint newHeadNumb)
{
  uint objNumb = getLowestObjNumber(), lastNumb, oldNumb;
  objectType *objPtr = findObj(objNumb);


 // basic technique - keep all old obj pointers in g_objLookup table until clearing them at the
 // very end so that checkAndFixRefstoObj()/etc calls work; similarly, do not reset low/high obj
 // number until the very end

 if (renumberHead)
  {
    oldNumb = objNumb;
    objPtr->objNumber = lastNumb = newHeadNumb;

    resetAllObjHere(objNumb, newHeadNumb);
    resetNumbLoaded(ENTITY_OBJECT, objNumb, newHeadNumb);

    checkAndFixRefstoObj(objNumb, newHeadNumb);

    g_objLookup[newHeadNumb] = objPtr;

    g_madeChanges = true;
  }
  else
  {
    objNumb = oldNumb = lastNumb = getLowestObjNumber();
  }

 // skip past first object

  objPtr = getNextObj(oldNumb);

 // remove gaps

  while (objPtr)
  {
    oldNumb = objNumb = objPtr->objNumber;

    if (objNumb != (lastNumb + 1))
    {
      objPtr->objNumber = objNumb = lastNumb + 1;

      resetAllObjHere(oldNumb, objNumb);
      resetNumbLoaded(ENTITY_OBJECT, oldNumb, objNumb);

      checkAndFixRefstoObj(oldNumb, objNumb);

      g_madeChanges = true;

      g_objLookup[objNumb] = objPtr;
      if (!renumberHead)
        g_objLookup[oldNumb] = NULL;
    }

    lastNumb = objPtr->objNumber;

    objPtr = getNextObj(oldNumb);
  }

  resetEntityPointersByNumb(true, false);

 // clear old range

  if (renumberHead)
  {
   // entire range was moved, assumes no overlap

    memset(g_objLookup + g_lowestObjNumber, 0, ((g_highestObjNumber - g_lowestObjNumber) + 1) * sizeof(objectType*));
  }
  else
  {
   // if the head vnum wasn't changed, then the most that happened was that the high vnum came down

    memset(g_objLookup + lastNumb + 1, 0, (g_highestObjNumber - lastNumb) * sizeof(objectType*));
  }

 // set new low/high

  if (renumberHead)
    g_lowestObjNumber = newHeadNumb;

  g_highestObjNumber = lastNumb;
}


//
// getPrevObj : find object right before objNumb, numerically
//

objectType *getPrevObj(const uint objNumb)
{
  uint i = objNumb - 1;

  if (objNumb <= getLowestObjNumber()) 
    return NULL;

  while (!g_objLookup[i]) 
    i--;

  return g_objLookup[i];
}


//
// getNextObj : find object right after objNumb, numerically
//

objectType *getNextObj(const uint objNumb)
{
  uint i = objNumb + 1;

  if (objNumb >= getHighestObjNumber()) 
    return NULL;

  while (!g_objLookup[i]) 
    i++;

  return g_objLookup[i];
}


//
// deleteObjsinInv : delete any carried items that have objPtr
//

void deleteObjsinInv(const objectType *objPtr)
{
  editableListNode *edit = g_inventory;
  

  if (!objPtr) 
    return;

  while (edit)
  {
    editableListNode *next = edit->Next;

    if ((edit->entityType == ENTITY_OBJECT) && (((objectHere *)(edit->entityPtr))->objectPtr == objPtr))
      deleteEditableinList(&g_inventory, edit);

    edit = next;
  }
}


//
// updateInvKeywordsObj : update keywords of any items in inventory that match objPtr
//

void updateInvKeywordsObj(const objectType *objPtr)
{
  editableListNode *edit = g_inventory;


  if (!objPtr) 
    return;

  while (edit)
  {
    if ((edit->entityType == ENTITY_OBJECT) && (((objectHere *)(edit->entityPtr))->objectPtr == objPtr))
    {
      edit->keywordListHead = objPtr->keywordListHead;
    }

    edit = edit->Next;
  }
}


//
// resetLowHighObj : reset low and high obj type numb to actual
//

void resetLowHighObj(void)
{
  uint high = 0, low = g_numbLookupEntries;

  for (uint i = 0; i < g_numbLookupEntries; i++)
  {
    if (g_objLookup[i])
    {
      if (i > high) 
        high = i;

      if (i < low) 
        low = i;
    }
  }

  g_lowestObjNumber = low;
  g_highestObjNumber = high;
}


//
// getMatchingObj : snags the first matching object type based on character
//                  string - if numeric, looks for vnum, if non-numeric, scans
//                  keyword list.  checks current room obj list first, then
//                  entire object type list
//

objectType *getMatchingObj(const char *strn)
{
  uint vnum;
  bool isVnum;
  const uint highObjNumb = getHighestObjNumber();


  if (strnumber(strn))
  {
    isVnum = true;
    vnum = strtoul(strn, NULL, 10);
  }
  else 
  {
    isVnum = false;
  }

  objectHere *objHere = g_currentRoom->objectHead;

  while (objHere)
  {
    if (objHere->objectPtr)
    {
      if (!isVnum)
      {
        if (scanKeyword(strn, objHere->objectPtr->keywordListHead))
          return objHere->objectPtr;
      }
      else
      {
        if (objHere->objectNumb == vnum)
          return objHere->objectPtr;
      }
    }

    objHere = objHere->Next;
  }
  
  if (isVnum)
    return findObj(vnum);

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      objectType *obj = findObj(objNumb);

      if (scanKeyword(strn, obj->keywordListHead)) 
        return obj;
    }
  }

  return NULL;
}


//
// showKeyUsed : list all objects of type ITEM_KEY or with the keyword 'key' if no arg, or show where object
//               is used as key if arg
//

void showKeyUsed(const char *args)
{
  objectType *obj = NULL;
  room *roomPtr;
  uint vnum = 0;
  size_t lines = 0;
  char outStrn[512];
  bool foundKey = false;
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();


  if (strlen(args) == 0)
  {
    if (noObjTypesExist())
    {
      _outtext("\nThere are no object types.\n\n");

      return;
    }

    _outtext("\n\n");

    lines += 2;

    for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
    {
      if (objExists(objNumb))
      {
        obj = findObj(objNumb);

        if ((obj->objType == ITEM_KEY) || scanKeyword("KEY", obj->keywordListHead))
        {
          foundKey = true;

          sprintf(outStrn, "%s&n (#%u)\n", obj->objShortName, objNumb);

          if (checkPause(outStrn, lines))
            return;
        }
      }
    }

    if (!foundKey) 
      checkPause("There are no items of type key or with the keyword 'key'.\n", lines);

    checkPause("\n", lines);

    return;
  }

 // allow looking for objects that don't exist in this zone

  if (strnumber(args))
  {
    vnum = strtoul(args, NULL, 10);

    obj = findObj(vnum);
  }
  else
  {
    for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
    {
      if (objExists(objNumb))
      {
        obj = findObj(objNumb);

        if (scanKeyword(args, obj->keywordListHead))
        {
          vnum = objNumb;
          break;
        }
      }
    }

    if (!obj)
    {
      _outtext("\nNo object could be found that matches that keyword.\n\n");
      return;
    }
  }

  if (obj && (obj->objType != ITEM_KEY))
  {
    sprintf(outStrn, "\n&+WNOTE:&n '%s&n' is not of item type KEY.\n\n", obj->objShortName);
    if (checkPause(outStrn, lines))
      return;
  }

 // find the thingies that need this key and display them

  sprintf(outStrn, "\n\n'%s&n' (#%u) is needed for -\n\n",
          getObjShortName(obj), vnum);
  if (checkPause(outStrn, lines))
    return;

 // first, room exits

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (exitNeedsKey(roomPtr->exits[i], vnum))
        {
          sprintf(outStrn, "  &+C%s&n exit of &+groom #&+c%u&n, '%s&n'\n",
                  g_exitnames[i], roomPtr->roomNumber, roomPtr->roomName);

          if (checkPause(outStrn, lines)) 
            return;

          foundKey = true;
        }
      }
    }
  }

 // then, objects (containers)

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      if ((obj->objType == ITEM_CONTAINER) && (obj->objValues[2] == vnum))
      {
        sprintf(outStrn, "  &+gobject #&+c%u&n, '%s&n' (&+ycontainer&n)\n",
                objNumb, obj->objShortName);

        if (checkPause(outStrn, lines)) 
          return;

        foundKey = true;
      }
    }
  }

  if (!foundKey) 
    _outtext("That key is not used on any doors or containers.\n");

  _outtext("\n");
}

#ifdef GODMODE
#define IS_SET(flag, bit)  ((flag) & (bit))
int get_mincircle( int spell );

int itemvalue( objectType *obj )
{
  long workingvalue = 0;
  double multiplier = 1;
  double mod;

  if( !obj )
  {
    return 0;
  }

  if(IS_SET(obj->wearBits, ITEM_WEAR_EYES))
    multiplier *= 1.3;

  if(IS_SET(obj->wearBits, ITEM_WEAR_EARRING))
    multiplier *= 1.2;

  if (IS_SET(obj->wearBits, ITEM_WEAR_FACE))
    multiplier *= 1.3;

  if (IS_SET(obj->wearBits, ITEM_WEAR_QUIVER))
    multiplier *= 1.1;

  if (IS_SET(obj->wearBits, ITEM_WEAR_FINGER))
    multiplier *= 1.2;

  if (IS_SET(obj->wearBits, ITEM_GUILD_INSIGNIA))
    multiplier *= 1.5;

  if (IS_SET(obj->wearBits, ITEM_WEAR_NECK))
    multiplier *= 1.2;

  if (IS_SET(obj->wearBits, ITEM_WEAR_WAIST))
    multiplier *= 1.1;

  if (IS_SET(obj->wearBits, ITEM_WEAR_WRIST))
    multiplier *= 1.1;

  // Aff's add to the base value.
  if (IS_SET(obj->affect1Bits, AFF_STONE_SKIN))
	  workingvalue += 125;

  if (IS_SET(obj->affect1Bits, AFF_BIOFEEDBACK))
	  workingvalue += 110;

  if (IS_SET(obj->affect1Bits, AFF_FARSEE))
	  workingvalue += 45;

  if (IS_SET(obj->affect1Bits, AFF_DETECT_INVISIBLE))
	  workingvalue += 90;

  if (IS_SET(obj->affect1Bits, AFF_HASTE))
	  workingvalue += 75;

  if (IS_SET(obj->affect1Bits, AFF_INVISIBLE))
	  workingvalue += 35;

  if (IS_SET(obj->affect1Bits, AFF_SENSE_LIFE))
	  workingvalue += 45;

  if (IS_SET(obj->affect1Bits, AFF_MINOR_GLOBE))
	  workingvalue += 28;

  if (IS_SET(obj->affect1Bits, AFF_UD_VISION))
	  workingvalue += 40;

  if (IS_SET(obj->affect1Bits, AFF_WATERBREATH))
	  workingvalue += 45;

  if (IS_SET(obj->affect1Bits, AFF_PROTECT_EVIL))
	  workingvalue += 35;

  if (IS_SET(obj->affect1Bits, AFF_SLOW_POISON))
	  workingvalue += 20;

  if (IS_SET(obj->affect1Bits, AFF_SNEAK))
	  workingvalue += 125;

  if (IS_SET(obj->affect1Bits, AFF_BARKSKIN))
	  workingvalue += 25;

  if (IS_SET(obj->affect1Bits, AFF_INFRAVISION))
	  workingvalue += 7;

  if (IS_SET(obj->affect1Bits, AFF_LEVITATE))
	  workingvalue += 13;

  if (IS_SET(obj->affect1Bits, AFF_HIDE))
	  workingvalue += 85;

  if (IS_SET(obj->affect1Bits, AFF_FLY))
	  workingvalue += 75;

  if (IS_SET(obj->affect1Bits, AFF_AWARE))
	  workingvalue += 75;

  if (IS_SET(obj->affect1Bits, AFF_PROT_FIRE))
	  workingvalue += 35;

  if (IS_SET(obj->affect2Bits, AFF2_FIRESHIELD))
	  workingvalue += 45;

  if (IS_SET(obj->affect2Bits, AFF2_ULTRAVISION))
	  workingvalue += 75;

  if (IS_SET(obj->affect2Bits, AFF2_DETECT_EVIL))
	  workingvalue += 10;

  if (IS_SET(obj->affect2Bits, AFF2_DETECT_GOOD))
	  workingvalue += 10;

  if (IS_SET(obj->affect2Bits, AFF2_DETECT_MAGIC))
	  workingvalue += 15;

  if (IS_SET(obj->affect2Bits, AFF2_PROT_COLD))
    workingvalue += 35;

  if (IS_SET(obj->affect2Bits, AFF2_PROT_LIGHTNING))
    workingvalue += 35;

  if (IS_SET(obj->affect2Bits, AFF2_GLOBE))
	  workingvalue += 75;

  if (IS_SET(obj->affect2Bits, AFF2_PROT_GAS))
    workingvalue += 35;

  if (IS_SET(obj->affect2Bits, AFF2_PROT_ACID))
    workingvalue += 35;

  if (IS_SET(obj->affect2Bits, AFF2_SOULSHIELD))
    workingvalue += 45;

  if (IS_SET(obj->affect2Bits, AFF2_MINOR_INVIS))
	  workingvalue += 15;

  if (IS_SET(obj->affect2Bits, AFF2_VAMPIRIC_TOUCH))
	  workingvalue += 60;

  if (IS_SET(obj->affect2Bits, AFF2_EARTH_AURA))
	  workingvalue += 110;

  if (IS_SET(obj->affect2Bits, AFF2_WATER_AURA))
	  workingvalue += 115;

  if (IS_SET(obj->affect2Bits, AFF2_FIRE_AURA))
	  workingvalue += 120;

  if (IS_SET(obj->affect2Bits, AFF2_AIR_AURA))
	  workingvalue += 130;

  if (IS_SET(obj->affect2Bits, AFF2_PASSDOOR))
	  workingvalue += 80;

  if (IS_SET(obj->affect2Bits, AFF2_FLURRY))
	  workingvalue += 150;

  if (IS_SET(obj->affect3Bits, AFF3_PROT_ANIMAL))
	  workingvalue += 25;

  if (IS_SET(obj->affect3Bits, AFF3_SPIRIT_WARD))
	  workingvalue += 40;

  if (IS_SET(obj->affect3Bits, AFF3_GR_SPIRIT_WARD))
	  workingvalue += 65;

  if (IS_SET(obj->affect3Bits, AFF3_ENLARGE))
	  workingvalue += 120;

  if (IS_SET(obj->affect3Bits, AFF3_REDUCE))
	  workingvalue += 120;

  if (IS_SET(obj->affect3Bits, AFF3_INERTIAL_BARRIER))
	  workingvalue += 125;

  if (IS_SET(obj->affect3Bits, AFF3_COLDSHIELD))
	  workingvalue += 45;

  if (IS_SET(obj->affect3Bits, AFF3_TOWER_IRON_WILL))
	  workingvalue += 45;

  if (IS_SET(obj->affect3Bits, AFF3_BLUR))
	  workingvalue += 65;

  if (IS_SET(obj->affect3Bits, AFF3_PASS_WITHOUT_TRACE))
	  workingvalue += 45;

  if (IS_SET(obj->affect4Bits, AFF4_VAMPIRE_FORM))
	  workingvalue += 90;

  if (IS_SET(obj->affect4Bits, AFF4_HOLY_SACRIFICE))
	  workingvalue += 105;

  if (IS_SET(obj->affect4Bits, AFF4_BATTLE_ECSTASY))
	  workingvalue += 105;

  if (IS_SET(obj->affect4Bits, AFF4_DAZZLER))
	  workingvalue += 45;

  if (IS_SET(obj->affect4Bits, AFF4_PHANTASMAL_FORM))
	  workingvalue += 105;

  if (IS_SET(obj->affect4Bits, AFF4_NOFEAR))
	  workingvalue += 40;

  if (IS_SET(obj->affect4Bits, AFF4_REGENERATION))
	  workingvalue += 60;

  if (IS_SET(obj->affect4Bits, AFF4_GLOBE_OF_DARKNESS))
	  workingvalue += 15;

  if (IS_SET(obj->affect4Bits, AFF4_HAWKVISION))
	  workingvalue += 20;

  if (IS_SET(obj->affect4Bits, AFF4_SANCTUARY))
	  workingvalue += 105;

  if (IS_SET(obj->affect4Bits, AFF4_HELLFIRE))
	  workingvalue += 110;

  if (IS_SET(obj->affect4Bits, AFF4_SENSE_HOLINESS))
	  workingvalue += 15;

  if (IS_SET(obj->affect4Bits, AFF4_PROT_LIVING))
	  workingvalue += 65;

  if (IS_SET(obj->affect4Bits, AFF4_DETECT_ILLUSION))
	  workingvalue += 40;

  if (IS_SET(obj->affect4Bits, AFF4_ICE_AURA))
	  workingvalue += 90;

  if (IS_SET(obj->affect4Bits, AFF4_NEG_SHIELD))
	  workingvalue += 45;

  if (IS_SET(obj->affect4Bits, AFF4_WILDMAGIC))
	  workingvalue += 240;

  // Has a old school proc (Up to three spells).
  // Can un-comment the debug stuff if you want to modify this.
  if(IS_SET(obj->wearBits, ITEM_WIELD) && (obj->objValues[5] > 0))
  {
    int spells[3];
    int spellcirclesum, numspells;

    // val5 : 3 spells + all or one.
    spells[0] = obj->objValues[5] % 1000;
    spells[1] = obj->objValues[5] % 1000000 / 1000;
    spells[2] = obj->objValues[5] % 1000000000 / 1000000;
//    debug( "Spells0: %d, Spells1: %d, Spells2: %d.", spells[0], spells[1], spells[2] );

    // val6 = level * val7 = chance -> 1/30 chance = 1, 1/60 chance = .5, 1/15 chance = 2, etc.
    mod = ((obj->objValues[6] > 19) ? obj->objValues[6] / 10.0 : 1) * ( 30.0 / obj->objValues[7] );
//    debug( "mod: %f, objval6/10: %f, 30/objval7: %f", mod, (obj->objValues[6] > 19) ? obj->objValues[6] / 10.0 : 1, (30.0 / obj->objValues[7]) );

    spellcirclesum = get_mincircle( spells[0] );
    spellcirclesum += get_mincircle( spells[1] );
    spellcirclesum += get_mincircle( spells[2] );
//    debug( "spellcirclesum: %d, circle0: %d, circle1: %d, circle2: %d.", spellcirclesum, get_mincircle(spells[0]), get_mincircle(spells[1]), get_mincircle(spells[2]) );

    // 1 lvl 10 spell  2nd circle 1/60 chance = 1*1* 2*.5 =  1
    // 1 lvl 60 spell  1st circle 1/30 chance = 1*6* 1*1  =  6
    // 1 lvl 40 spell  3rd circle 1/30 chance = 1*4* 3*1  = 12
    // 1 lvl 60 spell 12th circle 1/30 chance = 1*6*12*1  = 72 etc.
    // val5 / 1000000000 -> 1, otherwise casts all.
    if( obj->objValues[5] / 1000000000 )
    {
      // Add up number of spells
      numspells = ((spells[0]) ? 1 : 0)+((spells[1]) ? 1 : 0)+((spells[2]) ? 1 : 0);
      // If there are none?!, set to 1 anyway.
      numspells = numspells ? numspells : 1;
      // Compute average circle.
//      debug( "mod * spellcirclesum / numspells: %d.",(int) (mod * (spellcirclesum / numspells)) );
      workingvalue += (int) (mod * (spellcirclesum / numspells));
    }
    else
    {
//      debug( "spellcirclesum * mod: %d.", (int) (spellcirclesum * mod) );
      workingvalue += mod * spellcirclesum;
    }
  }

  //------- A0/A1/A2 -------------  
  int i = 0;
  while(i < 3)
  {
    mod = obj->objApply[i].applyModifier;
    //dam/hitroll are normal values
    if( (obj->objApply[i].applyWhere == APPLY_DAMROLL)
	    || (obj->objApply[i].applyWhere == APPLY_HITROLL) )
    {
      // 1:1, 2:2, 3:6, 4:12, 5:20, 6:30, 7:42, 8:56, 9:72, 10:90, 11: 110..
      workingvalue += (mod <= 2) ? mod : mod * (mod - 1);
      multiplier *= 1.25;
    }

    //regular stats can be high numbers - half them
    if( (obj->objApply[i].applyWhere == APPLY_STR)
	    || (obj->objApply[i].applyWhere == APPLY_DEX)
	    || (obj->objApply[i].applyWhere == APPLY_INT)
	    || (obj->objApply[i].applyWhere == APPLY_WIS)
	    || (obj->objApply[i].applyWhere == APPLY_CON)
	    || (obj->objApply[i].applyWhere == APPLY_AGI)
	    || (obj->objApply[i].applyWhere == APPLY_POW)
	    || (obj->objApply[i].applyWhere == APPLY_LUCK) )
    {
      // 1:1, 2:2, 3:4, 4:9, 5:16, 6:25, 7:36, 8:49, 9:64, 10:81, 11:100
      workingvalue += (mod <= 2) ? mod : (mod - 1) * (mod - 1);
    }

    //hit, move, mana, are generally large #'s
    if( (obj->objApply[i].applyWhere == APPLY_HIT)
	    || (obj->objApply[i].applyWhere == APPLY_MOVE)
	    || (obj->objApply[i].applyWhere == APPLY_MANA) )
    {
      // Right now, 25 : 25, 35 : 55, 50 : 100
      workingvalue += (mod <= 25) ? mod : 3 * mod - 50;
    }
    //hit, move, mana, regen are generally large #'s, but we don't want above 9.
    if( (obj->objApply[i].applyWhere == APPLY_HIT_REG)
	    || (obj->objApply[i].applyWhere == APPLY_MOVE_REG)
	    || (obj->objApply[i].applyWhere == APPLY_MANA_REG) )
    {
      // 1:1, 2:2, 3:3, 4:5, 5:8, 6:12, 7:16, 8:21, 9:27, 10:33
      // 11:40, 12:48, 13:56, 14:65, 15:75, 16:85, 17:96, 18:108
      workingvalue += (mod < 4) ? mod : mod * mod / 3;
    }

    // Racial attributes #'s - Do we still have these?
    if( (obj->objApply[i].applyWhere == APPLY_AGI_RACE)
	    || (obj->objApply[i].applyWhere == APPLY_STR_RACE)
	    || (obj->objApply[i].applyWhere == APPLY_CON_RACE)
	    || (obj->objApply[i].applyWhere == APPLY_INT_RACE)
	    || (obj->objApply[i].applyWhere == APPLY_WIS_RACE)
	    || (obj->objApply[i].applyWhere == APPLY_CHA_RACE)
	    || (obj->objApply[i].applyWhere == APPLY_DEX_RACE) )
    {
      if( mod < 1 || mod > LAST_RACE )
      {
//        debug( "itemvalue: obj '%s' %d has APPLY_..._RACE %d and bad modifier %d.",
//          obj->objShortName, GET_OBJ_VNUM(obj), obj->objApply[i].applyWhere, mod );
        workingvalue += 100;
      }
      else
      {
        switch( obj->objApply[i].applyWhere )
        {
          // We're looking for the stat vs 100. 75->0pts, 100->50pts, 150->150pts, 200->250pts
          // Since racial stats vary depending on values in duris_properties, which gets changed
          //   all the time, we just set the value as if the stat was 100. (Weak, but works).
          case APPLY_AGI_RACE:
            workingvalue += 2 * 100 - 150;
            break;
          case APPLY_STR_RACE:
            workingvalue += 2 * 100 - 150;
            break;
          case APPLY_CON_RACE:
            workingvalue += 2 * 100 - 150;
            break;
          case APPLY_INT_RACE:
            workingvalue += 2 * 100 - 150;
            break;
          case APPLY_WIS_RACE:
            workingvalue += 2 * 100 - 150;
            break;
          case APPLY_CHA_RACE:
            workingvalue += 2 * 100 - 150;
            break;
          case APPLY_DEX_RACE:
            workingvalue += 2 * 100 - 150;
            break;
          // Should never be the case but..
          default:
//            debug( "itemvalue: obj '%s' %d has 'bad' APPLY_..._RACE %d, modifier %d.",
//              obj->objShortName, GET_OBJ_VNUM(obj), obj->objApply[i].applyWhere, mod );
            workingvalue += 100;
            break;
        }
      }
    }

    //obj procs - No idea how to do this atm?
/* Procs aren't assigned in DE, so it sucks.
    if(obj_index[obj->R_num].func.obj && i < 1)
    {
      workingvalue += get_ival_from_proc(obj_index[obj->R_num].func.obj);
    }
*/
    //AC negative is good
    if( (obj->objApply[i].applyWhere == APPLY_AC) )
    {
      workingvalue -= mod / 10;
    }

    //saving throw values (good) are negative
    if( (obj->objApply[i].applyWhere == APPLY_SAVING_PARA)
	    || (obj->objApply[i].applyWhere == APPLY_SAVING_ROD)
	    || (obj->objApply[i].applyWhere == APPLY_SAVING_FEAR)
	    || (obj->objApply[i].applyWhere == APPLY_SAVING_BREATH)
	    || (obj->objApply[i].applyWhere == APPLY_SAVING_SPELL) )
    {
      // -1:2, -2:8, -3:18, -4:32, -5:50, -6:72, -7:98, -8:128
      workingvalue += mod * mod * ((mod <= 0) ? 2 : -2);
    }

    //pulse is quite valuable and negative is good
    if( (obj->objApply[i].applyWhere == APPLY_COMBAT_PULSE)
      || (obj->objApply[i].applyWhere == APPLY_SPELL_PULSE) )
    {
      multiplier *= 2;
      workingvalue += mod * -75;
    }

    //max_stats double points
    if( (obj->objApply[i].applyWhere == APPLY_STR_MAX)
      || (obj->objApply[i].applyWhere == APPLY_DEX_MAX)
      || (obj->objApply[i].applyWhere == APPLY_INT_MAX)
      || (obj->objApply[i].applyWhere == APPLY_WIS_MAX)
      || (obj->objApply[i].applyWhere == APPLY_CON_MAX)
      || (obj->objApply[i].applyWhere == APPLY_CHA_MAX)
      || (obj->objApply[i].applyWhere == APPLY_AGI_MAX)
      || (obj->objApply[i].applyWhere == APPLY_POW_MAX)
      || (obj->objApply[i].applyWhere == APPLY_LUCK_MAX) )
    {
      // 1:2, 2:6, 3:14, 4:25, 5:39, 6:56, 7:77, 8:100
      workingvalue += (mod < 2) ? 2 * mod : 1.5625 * mod * mod;
    }
    i++;
  }

  if(obj->objType == ITEM_WEAPON)
  {
    // Add avg damage.
    workingvalue += (obj->objValues[1] * obj->objValues[2])/2;
  }

  if(workingvalue < 1)
  {
    workingvalue = 1;
  }

  workingvalue *= multiplier;
  //debug("&+YItem value is: &n%d", workingvalue); 
  return workingvalue;
}

int get_mincircle( int spell )
{
  int i, j;
  int lowest = 13;

  // If not a spell, return 0.
  if( spell < 1 || spell > LAST_SPELL )
  {
    return 0;
  }

  // Skip CLASS_NONE..
  for( i = 1; i <= CLASS_COUNT; i++ )
  {
    if( (skills[spell].minLevel[i] > 0) && (skills[spell].minLevel[i] <= lowest))
    {
        lowest = skills[spell].minLevel[i];
    }
  }

  return lowest;
}

// New object proc?  Add it here with a value associated with it.
/*
 * This can not work for now .. We'd have to port in a proc link from specs.assign.c
 *   and proc stubs (?) for each proc assigned or have each proc set to a unique #
 *   and have obj_proc_type changed to int, and fix in specs.assign.c.
 * The file we'd have to port in is assign_objects in specs.assign.c.  Maybe have it
 *   converted to assigning to a {vnum, proc} array and have get_ival_from_proc first
 *   search the array for the proc and send vnum to the below function.
int get_ival_from_proc( obj_proc_type proc )
{
  // Procs stoneskin on 1 min timer.
  if( proc == artifact_stone )
  {
    return 125;
  }
  // Gives bio.
  if( proc == artifact_biofeedback )
  {
    return 110;
  }
  // Hides char.
  if( proc == artifact_hide )
  {
    return 85;
  }
  // Turn invis on say invis proc.
  if( proc == artifact_invisible )
  {
    return 80;
  }
  // Poof if in sulight proc.
  if( proc == generic_drow_eq )
  {
    return 5;
  }
  // Str/Enhance Str proc.
  if( proc == jet_black_maul )
  {
    return 50;
  }
  // (un)holy word + dispel good|evil area.
  if( proc == faith )
  {
    return 60;
  }
  // Area damage of 12 + target damage of 50.
  if( proc == mistweave )
  {
    return 60;
  }
  // Procs pseudo-dodge 1/50 chance.
  if( proc == leather_vest )
  {
    return 50;
  }
  // Area levitate/fly.
  if( proc == deva_cloak )
  {
    return 75;
  }
  // Procs ice storm once per minute.
  if( proc == icicle_cloak )
  {
    return 50;
  }
  // 1/15 chance to proc big fist on an ogre.
  if( proc == ogrebane )
  {
    return 60;
  }
  // 1/20 chance to proc fist on giant !dragon.
  if( proc == giantbane )
  {
    return 65;
  }
  // 1/25 chance to proc wither/lightning on either dwarf.
  if( proc == dwarfslayer )
  {
    return 65;
  }
  // 1/30 chance of uber feeblemind.
  if( proc == mindbreaker )
  {
    return 70;
  }
  // Level potions? hah.
  if( proc == treasure_chest )
  {
    return 200;
  }
  // Monolith: feeds artis.
  if( proc == artifact_monolith )
  {
    return 200;
  }
  // Gives miners sight.
  if( proc == miners_helmet )
  {
    return 125;
  }
  // 1/30 chance for destroy undead or dispel evil or cause crit.
  if( proc == glades_dagger )
  {
    return 85;
  }
  // 1/30 chance to proc extra attacks.
  if( proc == lucky_weapon )
  {
    return 55;
  }
  // 1/10 chance for 400-500 dam to dragons.
  if( proc == mace_dragondeath )
  {
    return 135;
  }
  // 1/30 chance to proc random spellup.
  if( proc == khildarak_warhammer )
  {
    return 65;
  }
  // 1/30 chance to proc berserk.
  if( proc == ogre_warlords_sword )
  {
    return 45;
  }
  // 1/30 chance to proc random fire spell.
  if( proc == flaming_axe_of_azer )
  {
    return 70;
  }
  // 1/14 chance for small fireball.
  if( proc == mrinlor_whip )
  {
    return 45;
  }
  // ' sphinx -> mordenkainens_lucubration evey 5 min.
  if( proc == sphinx_prefect_crown )
  {
    return 125;
  }
  // 1min stone timer and hummer
  if( proc == platemail_of_defense )
  {
    return 130;
  }
  // Effects based on # items worn
  if( proc == master_set )
  {
    return 35;
  }
  // Feeds the user.. not really useful.
  if( proc == ioun_sustenance )
  {
    return 45;
  }
  // 1/4 chance to deflect nuke to someone else.
  if( proc == deflect_ioun )
  {
    return 225;
  }
  // Raises a level on 1st touch.  Destroys all eq in room on second touch.
  if( proc == ioun_testicle )
  {
    return 250;
  }
  // Relocate ioun usable each minute
  if( proc == ioun_warp )
  {
    return 225;
  }
  // Monk artifact tendrils proc (riposte + stone).
  if( proc == tendrils )
  {
    return 205;
  }
  // 1/4 chance to deflect spell.
  if( proc == elvenkind_cloak )
  {
    return 225;
  }
  // 1/10 chance to proc earthquake if fighting.
  if( proc == earthquake_gauntlet )
  {
    return 65;
  }
  // 1/25 chance to proc blind if fighting (limited targets).
  if( proc == blind_boots )
  {
    return 50;
  }
  // Invigorates every 200 sec.
  if( proc == thanksgiving_wings )
  {
    return 85;
  }
  // Pass without trace proc every 800 sec.
  if( proc == pathfinder )
  {
    return 85;
  }
  // 1/16 chance on hit to proc disintegrate.
  if( proc == orb_of_destruction )
  {
    return 115;
  }
  // armor/bless invoke proc every 5 min.
  if( proc == sanguine )
  {
    return 55;
  }
  // 1/30 chance during battle to proc 75 damage.
  if( proc == neg_orb )
  {
    return 85;
  }
  // Lots of say procs and no timer and battle proc.
  if( proc == totem_of_mastery )
  {
    return 350;
  }
  // Procs heal on rub invoke every 150 sec = 2 1/2 min.
  if( proc == ring_of_regeneration )
  {
    return 110;
  }
  // Summons water mental invoke every 3 min.
  if( proc == glowing_necklace )
  {
    return 120;
  }
  // Summons shadow pet every 12 min. agg if under.
  if( proc == staff_shadow_summoning )
  {
    return 115;
  }
  // 1/16 chance during battle to proc stornogs_lowered_magical_res.
  if( proc == rod_of_magic )
  {
    return 95;
  }
  // Skill whirlwind on invoke every 5 min.
  if( proc == proc_whirlwinds )
  {
    return 150;
  }
  // 1/3 chance procs random bard off songs. Bard prot songs every 30 sec.
  if( proc == lyrical_instrument_of_time )
  {
    return 220;
  }
  // 1/25 chance to proc flee.
  if( proc == sinister_tactics_staff )
  {
    return 85;
  }
  // 1/25 chance for low lvl feeb.
  if( proc == shard_frozen_styx_water )
  {
    return 65;
  }
  // Ambran/DeathRider proc.
  if( proc == holy_weapon )
  {
    return 350;
  }
  // Mox totem proc (heal / 30 sec & 1/3 chance to debuff).
  if( proc == mox_totem )
  {
    return 150;
  }
  // Cures blind every 30 min on invoke.
  if( proc == flayed_mind_mask )
  {
    return 25;
  }
  // Enlarge/Reduce on say 3 min timer/Hide 1 min timer.
  if( proc == stalker_cloak )
  {
    return 235;
  }
  // Airy water on 1 min timer.
  if( proc == finslayer_air )
  {
    return 105;
  }
  // Procs hide every 15 min.
  if( proc == aboleth_pendant )
  {
    return 95;
  }
  // 1/30 chance on got-hit to proc lightning/call lightning.
  if( proc == lightning_armor )
  {
    return 115;
  }
  // Procs major para for 5 mins on each got-hit.
  if( proc == imprison_armor )
  {
    return 550;
  }
  // 1/2 chance to proc magic missile + ice missile.
  if( proc == fun_dagger )
  {
    return 50;
  }
  // 1/2 chance to bigbys hand.
  if( proc == rax_red_dagger )
  {
    return 200;
  }
  // Procs damage when try to pick up.
  if( proc == cutting_dagger )
  {
    return 100;
  }
  // Procs mass heal on say, and 1/15 chance for lightning bolt in battle.
  if( proc == circlet_of_light )
  {
    return 220;
  }
  // 1/30 lvl 25 greater spirit anguish heh.
  if( proc == ljs_sword )
  {
    return 75;
  }
  // 1/30 chance to proc lvl 1 magic missile.
  if( proc == wuss_sword )
  {
    return 5;
  }
  // 1/30 chance to proc lvl 30 magma burst.
  if( proc == head_guard_sword )
  {
    return 65;
  }
  // Procs mass heal every 2 min.
  if( proc == priest_rudder )
  {
    return 125;
  }
  // Makes ingreds every minute.
  if( proc == alch_bag )
  {
    return 75;
  }
  // 1/30 chance to proc mental/shadow monsters
  if( proc == alch_rod )
  {
    return 105;
  }
  // 1/10 chance to prot vs undead/living depending on victim.
  if( proc == ljs_armor )
  {
    return 75;
  }
  // 1/2 periodic proc of destroy undead & various detect buffs every minute.
  if( proc == dragon_skull_helm )
  {
    return 225;
  }
  // 1/30 chance to proc drain moves.
  if( proc == nightcrawler_dagger )
  {
    return 50;
  }
  // 1/30 chance to proc virtue then procs damage spell.
  if( proc == righteous_blade )
  {
    return 85;
  }
  // 1/20 chance to proc a nuke.
  if( proc == xmas_cap )
  {
    return 125;
  }
  // 1/50 chance to remove skin spell.
  if( proc == khaziddea_blade )
  {
    return 75;
  }
  // Makes ch a Revenant.
  if( proc == revenant_helm )
  {
    return 225;
  }
  // Stornogs spheres every 30 min & stone every 30 sec.
  if( proc == dragonlord_plate )
  {
    return 255;
  }
  // 1/25 chance for sunray/heal & stone every 30 sec.
  if( proc == sunblade )
  {
    return 275;
  }
  // Procs damage/vamp.
  if( proc == bloodfeast )
  {
    return 175;
  }
  // Must be lvl < 36, 1/20 chance of haste/stone/burning hands on dragon.
  if( proc == dragonslayer )
  {
    return 55;
  }
  // 1/15 chance to proc stun/nightmare/shatter on men.
  if( proc == mankiller )
  {
    return 115;
  }
  // 1/30 chance to blur, 1/20 chance to riposte (multiple hits possible).
  if( proc == madman_mangler )
  {
    return 220;
  }
  // 1/10 chance to proc 10-20 damage back on attacker.
  if( proc == madman_shield )
  {
    return 115;
  }
  // Reflections on a 5 min timer.
  if( proc == mentality_mace )
  {
    return 115;
  }
  // 1/10 chance of damage+vamp proc, coldshield/fireshield, globe.
  if( proc == vapor )
  {
    return 275;
  }
  // 2min 40sec timer for random buff.
  if( proc == serpent_of_miracles )
  {
    return 115;
  }
  // 1/30 chance for 300 damage proc.
  if( proc == transparent_blade )
  {
    return 175;
  }
  // Invoke heal 3 min timer, blocks/bashes.
  if( proc == Einjar )
  {
    return 200;
  }
  // 1/10 chance to 70% chance knockdown or fail.
  if( proc == tripboots )
  {
    return 125;
  }
  // 1/6 chance to blind target (70%) or ch (30%)/
  if( proc == blindbadge )
  {
    return 85;
  }
  // Makes you drop yer weapon?!?
  if( proc == fumblegaunts )
  {
    return 5;
  }
  // 1/30 chance to inflict pain/ego blast.
  if( proc == confusionsword )
  {
    return 65;
  }
  // Random stat buffs.
  if( proc == guild_badge )
  {
    return 125;
  }
  // Random object proc (6 same zone == stone etc).
  if( proc == set_proc )
  {
    return 55;
  }
  // All in room cure crit proc every periodic (kills vamp).
  if( proc == thrusted_eq_proc )
  {
    return 105;
  }
  // Random skill or energy drain.
  if( proc == encrusted_eq_proc )
  {
    return 125;
  }
  // Forge recipe.
  if( proc == parchment_forge )
  {
    return 25;
  }
  // Old proc.. Holy relic of the Gods.
  if( proc == relic_proc )
  {
    return 200;
  }
  // 1/15 chance to summon mentals on hit, 1/4 chance to revenge nuke (Alchemist arti).
  if( proc == cold_hammer )
  {
    return 175;
  }
  // 1/30 chance to inflict pain/feeb or zerk.
  if( proc == brainripper )
  {
    return 125;
  }
  // 1/30 chance to double lightningbolt.
  if( proc == hammer_titans )
  {
    return 75;
  }
  // 1/30 chance for cyclone then another area nuke.
  if( proc == stormbringer )
  {
    return 175;
  }
  // Necroplasm arti proc..
  if( proc == living_necroplasm )
  {
    return 500;
  }
  // Vit mask.
  if( proc == vigor_mask )
  {
    return 125;
  }
  // Armor/bless proc invoke every 15sec.
  if( proc == church_door )
  {
    return 35;
  }
  // Double cure crit invoke every 15min.
  if( proc == splinter )
  {
    return 75;
  }
  // 1/25 chance to damage moves.
  if( proc == demo_scimitar )
  {
    return 65;
  }
  // 1/10 chance to proc flee on ch->fighting.
  if( proc == dranum_mask )
  {
    return 105;
  }
  // Procs invoke area slow every 24 min.
  if( proc == golem_chunk )
  {
    return 155;
  }
  // Mayhem/Symmetry.. big arti proc.
  if( proc == good_evil_sword )
  {
    return 500;
  }
  // Stones every 30 sec, invoke invis/hide every 2 min.
  if( proc == transp_tow_misty_gloves )
  {
    return 240;
  }
  // Big god-only arti
  if( proc == gfstone )
  {
    return 750;
  }
  // 1/10 chance to proc disarm on got-hit.
  if( proc == disarm_pick_gloves )
  {
    return 165;
  }
  // Yeah.. reload/fire pistol until dead.
  if( proc == roulette_pistol )
  {
    return 200;
  }
  // Invoke proc of mirage every 3 min.
  if( proc == orb_of_deception )
  {
    return 180;
  }
  // Zombies game proc.  God only.
  if( proc == zombies_game )
  {
    return 1000;
  }
  // Sword of sharpness proc: cuts limbs.
  if( proc == illesarus )
  {
    return 185;
  }
  // 30% chance on got-hit dispel good/disease/dispel magic
  if( proc == vecna_pestilence )
  {
    return 95;
  }
  // 6% chance to proc level 51 bigbys on hit.
  if( proc == vecna_minifist )
  {
    return 130;
  }
  // 5% chance to level 51 dispel magic proc on hit.
  if( proc == vecna_dispel )
  {
    return 125;
  }
  // 6% chance to level 51 iceball proc on hit.
  if( proc == vecna_boneaxe )
  {
    return 130;
  }
  // 10% chance for off proc, say procs.
  if( proc == vecna_staffoaken )
  {
    return 225;
  }
  // Krindors device.. big arti.
  if( proc == vecna_krindor_main )
  {
    return 255;
  }
  // Various damage procs.
  if( proc == vecna_death_mask )
  {
    return 135;
  }
  // Various procs vs undead.
  if( proc == mob_vecna_procs )
  {
    return 150;
  }
  // 1/4 chance to bleed proc on periodic.
  if( proc == lifereaver )
  {
    return 105;
  }
  // Kills stats on item if given to another.
  if( proc == flame_blade )
  {
    return 10;
  }
  // Area vit and tsunami wave proc.
  if( proc == SeaKingdom_Tsunami )
  {
    return 225;
  }
  // 1/30 chance to proc lightningbolt.
  if( proc == shimmering_longsword )
  {
    return 65;
  }
  // 3% chance to do mega damage (female/Drow/Cleric only).
  if( proc == rod_of_zarbon )
  {
    return 225;
  }
  // Procs coldshield/fireshield on invoke when lit.
  if( proc == frost_elb_dagger )
  {
    return 85;
  }
  // Procs feeblemind on backstab.
  if( proc == dagger_submission )
  {
    return 185;
  }
  // 1/4 chance to proc wither on hit, 60 sec stone proc.
  if( proc == trans_tower_shadow_globe )
  {
    return 195;
  }
  // Burns the target upon get/take/etc.
  if( proc == burn_touch_obj )
  {
    return 105;
  }
  // Important zone proc.
  if( proc == drowcrusher )
  {
    return 150;
  }
  // 5% chance to proc silence.
  if( proc == squelcher )
  {
    return 115;
  }
  // Absorbs dragonbreath.
  if( proc == dragonarmor )
  {
    return 225;
  }
  // Turns char into a Kearonor Beast (Demon).
  if( proc == kearonor_hide )
  {
    return 185;
  }
  // Allows clairovoyance to anyone while playing organ.
  if( proc == hewards_mystical_organ )
  {
    return 275;
  }
  // Clairovoyance to specific room on timer.
  if( proc == amethyst_orb )
  {
    return 105;
  }
  // Random procs on use wand (limited uses).
  if( proc == wand_of_wonder )
  {
    return 125;
  }
  // 1/31 chance to holy word.
  if( proc == blade_of_paladins )
  {
    return 125;
  }
  // 1/31 chance to proc level 35 power word blind.  invoke invis once per mud day.
  if( proc == fade_drusus )
  {
    return 95;
  }
  // Procs chain lightning and eats corpses and stuff.
  if( proc == lightning_sword )
  {
    return 225;
  }
  // Changes stats based on target.
  if( proc == elfdawn_sword )
  {
    return 105;
  }
  // Invoke fly/proc flamestrike 1/31 chance.
  if( proc == flame_of_north_sword )
  {
    return 130;
  }
  // 1/31 chance to feeb, and updates stats based on target.
  if( proc == magebane_falchion )
  {
    return 175;
  }
  // Heals wielder on hit.
  if( proc == woundhealer_scimitar )
  {
    return 125;
  }
  // 1/30 chance to proc harm on hit, invoke full heal once per mud day.
  if( proc == martelo_mstar )
  {
    return 210;
  }
  // 1/400 chance to proc behead (instadeath).
  if( proc == githpc_special_weap )
  {
    return 550;
  }
  // Allows accept/decline/ptell imm commands to user.
  if( proc == trustee_artifact )
  {
    return 10000;
  }
  // Invoke proc that sacrifices/terminates/CDs target.
  if( proc == orcus_wand )
  {
    return 10000;
  }
  // Command murder -> Instakill target.
  if( proc == varon )
  {
    return 10000;
  }
  // Proc for traps.
  if( proc == huntsman_ward )
  {
    return 125;
  }
  // Switch proc: unblocks passageways (invaluable in zones).
  if( proc == item_switch )
  {
    return 500;
  }
  // 1/20 chance to proc lightning bolt on hit.
  if( proc == hammer )
  {
    return 45;
  }
  // The uber warhammer arti proc.
  if( proc == barb )
  {
    return 550;
  }
  // 1/10 chance to proc harm.
  if( proc == gesen )
  {
    return 85;
  }
  // Labelas god proc.
  if( proc == labelas )
  {
    return 10000;
  }
  // Various dragon procs.
  if( proc == dragonkind )
  {
    return 235;
  }
  // Invoke area res once per hour, 1/17 chance to proc group heal periodic.
  if( proc == resurrect_totem )
  {
    return 325;
  }
  // Slowly disappears on cast(s).
  if( proc == crystal_spike )
  {
    return 75;
  }
  // Opens a doorway (invaluable in a zone).
  if( proc == automaton_lever )
  {
    return 200;
  }
  // Object turns into a corpse heh.
  if( proc == forest_corpse )
  {
    return 100;
  }
  // Procs uber blind/poison.
  if( proc == torment )
  {
    return 125;
  }
  // Avernus proc.  Uber arti.
  if( proc == avernus )
  {
    return 250;
  }
  // 1/35 chance to proc behead/big hurt.
  if( proc == githyanki )
  {
    return 550;
  }
  // 1/30 chance to proc firestorm/immo/ice storm/cone of cold.
  if( proc == kvasir_dagger )
  {
    return 105;
  }
  // Mega arti Doombringer proc.
  if( proc == doombringer )
  {
    return 350;
  }
  // 1/30 chance for incendiary, burning hands & fireball.
  if( proc == flamberge )
  {
    return 220;
  }
  // Charms mentals.  Limited usage.
  if( proc == ring_elemental_control )
  {
    return 175;
  }
  // 1/25 chance to proc flamestrike & full harm, procs healing 1/4 chance on got hit.
  if( proc == holy_mace )
  {
    return 185;
  }
  // Uber arti staff of blue flames proc.
  if( proc == staff_of_blue_flames )
  {
    return 320;
  }
  // 1/16 chance to proc psychic crush.  Various psi spellups.
  if( proc == staff_of_power )
  {
    return 320;
  }
  // Summons a pegaus pet.
  if( proc == reliance_pegasus )
  {
    return 180;
  }
  // 1/16 chance for triple lightning bolt proc, invoke area bash.
  if( proc == lightning )
  {
    return 225;
  }
  // Procs stop battles.
  if( proc == dagger_of_wind )
  {
    return 125;
  }
  // 1/90 chance to banish to water plane.
  if( proc == orb_of_the_sea )
  {
    return 280;
  }
  // 1/25 chance to proc earthquake & earthen rain.  Invoke stone, proc stone on got hit.
  if( proc == zion_mace_of_earth )
  {
    return 280;
  }
  // 1/25 chance for damage + vamp on melee hit proc.
  if( proc == unholy_avenger_bloodlust )
  {
    return 240;
  }
  // 1/16 to proc major para spell.
  if( proc == tiamat_stinger )
  {
    return 280;
  }
  // 1/16 chance to proc 3 fireballs and a flamestrike.
  if( proc == dispator )
  {
    return 280;
  }
  // 1/30 chance to proc uber sleep and 1/40 to proc darkness.
  if( proc == nightbringer )
  {
    return 175;
  }
  // 1/30 chance to proc attempt to charm undead.
  if( proc == undead_trident )
  {
    return 185;
  }
  // 1/30 chance to proc disarm.
  if( proc == iron_flindbar )
  {
    return 135;
  }
  // 1/20 chance to proc parry
  if( proc == generic_parry_proc )
  {
    return 135;
  }
  // 1/20 chance to proc riposte
  if( proc == generic_riposte_proc )
  {
    return 155;
  }
  // Various Reaver spellups.
  if( proc == flame_of_north )
  {
    return 215;
  }
  // Summons a mob and breaks.
  if( proc == menden_figurine )
  {
    return 105;
  }
  // 1/33 chance to do 10d24 spell damage proc & 1/33 chance to do 2 extra hits.
  if( proc == sevenoaks_longsword )
  {
    return 225;
  }
  // 1/30 chance for lightning bolt proc.
  if( proc == mace_of_sea )
  {
    return 115;
  }
  // 1/30 chance for poison & minor para.
  if( proc == serpent_blade )
  {
    return 145;
  }
  // 1/30 chance to proc uber wither.
  if( proc == lich_spine )
  {
    return 185;
  }
  // Max 1/9 chance to proc random necro debuff.
  if( proc == doom_blade_Proc )
  {
    return 125;
  }
  // 1/50 chance for random fire spell.
  if( proc == dagger_ra )
  {
    return 195;
  }
  // 1/35 chance to proc psychic crush or bash, and 10 min invoke ether warp.
  if( proc == illithid_axe )
  {
    return 320;
  }
  // 1/33 chance for random damage proc.  Lots of random buffs/heals.
  if( proc == deathseeker_mace )
  {
    return 235;
  }
  // 1/33 chance to nuke, and nice buffs on 10 min timer.
  if( proc == demon_slayer )
  {
    return 235;
  }
  // 10 min timer for skin spells & 1/33 chance for random nuke.
  if( proc == snowogre_warhammer )
  {
    return 235;
  }
  // 1/33 chance to proc heal/bash/energy drain & lame teleport proc.
  if( proc == volo_longsword )
  {
    return 185;
  }
  // 1/25 chance random nuke, blur 10 min timer, shadow shield & vanish on 10 min timer.
  if( proc == blur_shortsword )
  {
    return 235;
  }
  // 7 min 30 sec timer for double group heal/vit/soulshield,accel healing & endurance(all area).  1/25 chance to proc dispel align.
  if( proc == buckler_saints )
  {
    return 235;
  }
  // Invoke vamp trance.
  if( proc == helmet_vampires )
  {
    return 300;
  }
  // Invoke call lightning 10 min timer, 1/30 chance to proc double lightning, regen & endurance on 10 min timer.
  if( proc == storm_legplates )
  {
    return 205;
  }
  // 1/10 chance to hit/vamp, 3 min stone timer, 10 min invoke nonexistance timer.
  if( proc == gauntlets_legend )
  {
    return 235;
  }
  // 1/30 chance to proc backstab on melee hit.
  if( proc == gladius_backstabber )
  {
    return 205;
  }
  // Transforms on rub wand.
  if( proc == elemental_wand )
  {
    return 105;
  }
  // Adds 2 maxstats to earring random every 10 min.
  if( proc == earring_powers )
  {
    return 125;
  }
  // Transforms into another object.
  if( proc == lorekeeper_scroll )
  {
    return 135;
  }
  // 1/20 chance to proc curse or malison.
  if( proc == damnation_staff )
  {
    return 115;
  }
  // 1/20 chance to proc curse or malison.
  if( proc == nuke_damnation )
  {
    return 115;
  }
  // Summons an ice mental, 10 min timer.
  if( proc == collar_frost )
  {
    return 185;
  }
  // Summons a fire mental, 10 min timer.
  if( proc == collar_flames )
  {
    return 185;
  }
  // Creates a gift when opened.
  if( proc == lancer_gift )
  {
    return 150;
  }
  // 1/20 chance to absorb spell and blind ch.
  if( proc == zion_shield_absorb_proc )
  {
    return 195;
  }
  // 1/5 chance to block a hit.
  if( proc == generic_shield_block_proc )
  {
    return 185;
  }
  // Turns land to lava, fire auras, and 1/25 procs fire spells.
  if( proc == zion_fnf )
  {
    return 245;
  }
  // 1/25 chance to proc nuke, 1 min timer on stone + buffs.
  if( proc == zion_light_dark )
  {
    return 235;
  }
  // Procs nukes on undead.
  if( proc == barovia_undead_necklace )
  {
    return 185;
  }
  // Procs shadow shield every minute as needed.
  if( proc == artifact_shadow_shield )
  {
    return 135;
  }
  // Opens swordcase in zone.
  if( proc == ravenloft_bell )
  {
    return 200;
  }
  // Curses and blurs wielder.
  if( proc == shimmer_shortsword )
  {
    return 175;
  }
  // Switch proc: invaluable in zones.
  if( proc == toe_chamber_switch )
  {
    return 500;
  }
  // 1/50 chance to proc hellfire
  if( proc == hellfire_axe )
  {
    return 175;
  }
  // 1/30 chance to proc trip.
  if( proc == illithid_whip )
  {
    return 210;
  }
  // Auto-wear leggings. lame.
  if( proc == skull_leggings )
  {
    return 25;
  }
  // 1/20 chance to proc damage to undead.
  if( proc == deliverer_hammer )
  {
    return 155;
  }
  // Invoke armor every 60 sec.
  if( proc == blue_sword_armor )
  {
    return 45;
  }
  // 1/30 chance to proc level 30 dispel magic.
  if( proc == sword_named_magik )
  {
    return 65;
  }
  // 1/20 chance to proc 400 damage & vamp, invoke ilienzes flame sword every 4 minues.
  if( proc == bel_sword )
  {
    return 210;
  }
  // 1/25 chance to proc destroy undead & cure serious.
  if( proc == zarthos_vampire_slayer )
  {
    return 115;
  }
  // 1/30 chance to proc bash/stun/blind.
  if( proc == critical_attack_proc )
  {
    return 155;
  }
  // Just displays cool periodic messages.
  if( proc == mask_of_wildmagic )
  {
    return 100;
  }
  // Invoke auto-mem a spell on 5 min 50 sec timer.
  if( proc == flow_amulet )
  {
    return 200;
  }
  // Invoke proc blink every 15-20 sec, Invoke proc group deflect every 8 min 20 sec.
  if( proc == zion_netheril )
  {
    return 275;
  }
  // 1/20 chance to proc blind, disease, curse or dispel magic on hit.  Procs heal every 20 sec as necessary.
  if( proc == eth2_godsfury )
  {
    return 185;
  }
  // 1/5 chance to proc summon helper on periodic.
  if( proc == eth2_aramus_crown )
  {
    return 175;
  }
  // Procs word of recall on 30 min timer.
  if( proc == lucrot_mindstone )
  {
    return 200;
  }
  return 100;
}
*/

void huntForOPEquipment(const char *args)
{
  int lowerIValLimit, currIVal;
  int lowest, highest;
  char buf[3000];

  if( (lowerIValLimit = atoi(args)) <= 0 )
  {
    _outtext( "\n\rHunting for OP equipment requires a positive integer as input.\n\r" );
    return;
  }
  sprintf( buf, "\n\r&+WSearching for items with itemvalues greater than or equal to %d.&n\n\r\n\r", lowerIValLimit );
  displayColorString(buf);

  lowest = getLowestObjNumber();
  highest = getHighestObjNumber();
  for( uint i = lowest; i <= highest; i++ )
  {
    if( (g_objLookup[i] != NULL) && ((currIVal = itemvalue(g_objLookup[i])) >= lowerIValLimit) )
    {
      sprintf( buf, "&+WItem #&n%6d&+W '&n%s&+W' is OP with itemvalue &n%d&+W.&n\n\r", i, g_objLookup[i]->objShortName, currIVal );
      displayColorString(buf);
    }
  }
}
#endif
