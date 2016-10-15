//
//  File: equip.cpp      originally part of durisEdit
//
//  Usage: functions for (un)equipping mobs with objects
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
#include "../misc/editable.h"

extern bool g_madeChanges;
extern room *g_currentRoom;

//
// equipEquiponMob : returns false if user quit out of pause
//
//   sticks obj on appropriate spot on mob, if possible - doesn't alter loaded count
//

bool equipEquiponMob(mobHere *mob, objectHere *obj, bool *equipped, size_t &numbLines)
{
  char tempStrn[MAX_OBJSNAME_LEN + 128];
  uint eqslot;


  *equipped = false;

  if (!obj->objectPtr) 
    return true;

  eqslot = getMobHereEquipSlot(mob, obj->objectPtr, -1);

 // if no error, add the equipment

  if (eqslot <= WEAR_TRUEHIGH)
  {
    *equipped = true;
    mob->equipment[eqslot] = obj;
  }

  return !checkPause(getVerboseCanEquipStrn(eqslot, obj->objectPtr->objShortName, tempStrn), numbLines);
}


//
// equipMobSpecific - called from equipMob() if user specifies 2 args - sticks appropriate eq on mob,
//                    checking carried list or, if obj vnum, loading on the fly
//

void equipMobSpecific(mobHere *mobH, const char *args)
{
  char objStrn[256];

  objectHere *objHere;
  bool isVnum;
  uint vnum;


 // object will be in arg 2

  getArg(args, 2, objStrn, 255);

  if (!mobH->inventoryHead && !strnumber(objStrn))
  {
   _outtext("\nThat mob is not carrying anything.\n\n");

   return;
  }

 // search mob's inventory for object that matches second keyword/vnum

  isVnum = strnumber(objStrn);
  if (isVnum)
  {
    vnum = strtoul(objStrn, NULL, 10);

   // not able to equip an object that doesn't exist, at least for now

    if (!objExists(vnum))
    {
      _outtext("\nIt is not possible to equip mob with objects not in this .obj file.\n\n");

      return;
    }
  }

  objHere = mobH->inventoryHead;
  while (objHere)
  {
    if ((isVnum && (objHere->objectNumb == vnum)) || 
        (objHere->objectPtr && scanKeyword(objStrn, objHere->objectPtr->keywordListHead)))
      break;

    objHere = objHere->Next;
  }

 // couldn't find match - if vnum, check if mob can equip and load up on the fly if so

  _outtext("\n");

  if (!objHere)
  {
    if (isVnum)
    {
      const uint eqslot = getMobHereEquipSlot(mobH, findObj(vnum), -1);

      if (eqslot > WEAR_TRUEHIGH)
      {
        char tempStrn[MAX_OBJSNAME_LEN + 128];

        displayColorString(getVerboseCanEquipStrn(eqslot, findObj(vnum)->objShortName, tempStrn));
      }
      else
      {
        objHere = createObjHere(vnum, true);
        if (!objHere)
        {
          displayAllocError("objectHere", "equipMobSpecific");
        }
        else
        {
          size_t numbLines = 0;
          bool equipped;  // not used

          equipEquiponMob(mobH, objHere, &equipped, numbLines);
        }
      }
    }
    else
    {
      _outtext("\nNone of the objects carried by the mob match keyword specified.\n\n");
    }
  }

 // got a match in mob's inventory, try to equip

  else
  {
    bool equipped;
    size_t numbLines = 0;

    equipEquiponMob(mobH, objHere, &equipped, numbLines);

    if (equipped)
    {
      removeObjHerefromList(&(mobH->inventoryHead), objHere, false);

      g_madeChanges = true;
    }
  }

  _outtext("\n");
}


//
// equipMob : takes items from a mob's carried list and equips them, or if vnum loads on the fly
//

void equipMob(const char *strn)
{
  char whatMatched;
  editableListNode *matchingNode;
  objectHere *objHere, *objNext;
  char mobName[256];
  mobHere *mobH;


 // get target mob

  getArg(strn, 1, mobName, 255);

  checkEditableList(mobName, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

  switch (whatMatched)
  {
    case NO_MATCH :
      _outtext("\nNo mob found that matches first keyword.\n\n");

      return;

    case ENTITY_MOB :
      mobH = (mobHere *)(matchingNode->entityPtr);

      break;

    default :
      char outstrnerr[256];
      const char *entityStrn = getEntityTypeStrn(whatMatched);

      sprintf(outstrnerr, "\nYou cannot equip anything on a%s %s.\n\n",
              getVowelN(entityStrn[0]), entityStrn);

      _outtext(outstrnerr);

      return;
  }

 // if more than one arg, try to equip specific object, otherwise equip everything in mob's inventory

  if (numbArgs(strn) > 1)
  {
    equipMobSpecific(mobH, strn);
  }
  else
  {
   // make sure mob has stuff in its inventory - must be only one arg (mob name)

    if (!(mobH->inventoryHead))
    {
      _outtext("\nThat mob is not carrying anything.\n\n");

      return;
    }

    size_t numbLines = 0;

    objHere = mobH->inventoryHead;

    _outtext("\n");

   // run through inventory and equip what can be equipped

    while (objHere)
    {
      bool equipped;

      objNext = objHere->Next;

      const bool quitPause = !equipEquiponMob(mobH, objHere, &equipped, numbLines);

      if (equipped)
      {
        removeObjHerefromList(&(mobH->inventoryHead), objHere, false);

        g_madeChanges = true;
      }

      if (quitPause)
        return;

      objHere = objNext;
    }

    char outstrn[MAX_MOBSNAME_LEN + 64];

    sprintf(outstrn, "\nAll equippable objects carried by '%s&n' equipped.\n\n",
            getMobShortName(mobH->mobPtr));

    checkPause(outstrn, numbLines);
  }
}
