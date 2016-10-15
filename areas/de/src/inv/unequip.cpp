//
//  File: unequip.cpp    originally part of durisEdit
//
//  Usage: functions for removing objects from a mob's equipment list
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
#include <ctype.h>

#include "../fh.h"
#include "../types.h"
#include "../de.h"
#include "../misc/editable.h"
#include "../obj/objhere.h"
#include "../mob/mobhere.h"

extern bool g_madeChanges;
extern room *g_currentRoom;

//
// unequipMobSpecific
//

void unequipMobSpecific(mobHere *mobH, const char *objStrn)
{
  objectHere *objHere = NULL;
  int where = -1;
  bool isVnum;
  uint vnum;


  isVnum = strnumber(objStrn);
  if (isVnum)
    vnum = strtoul(objStrn, NULL, 10);

// try to find equipment that matches second vnum/keyword

  for (uint pos = WEAR_LOW; pos <= WEAR_TRUEHIGH; pos++)
  {
    objHere = mobH->equipment[pos];

    if (objHere)
    {
      if (!isVnum)
      {
        if (objHere->objectPtr && scanKeyword(objStrn, objHere->objectPtr->keywordListHead))
        {
          where = pos;
          break;
        }
      }
      else
      {
        if (objHere->objectNumb == vnum)
        {
          where = pos;
          break;
        }
      }
    }
  }

  if (where == -1)
  {
    _outtext("\nNone of the objects equipped by the mob match vnum/keyword specified.\n\n");

    return;
  }

  mobH->equipment[where] = NULL;
  addObjHeretoList(&(mobH->inventoryHead), objHere, false);

  char outstrn[MAX_OBJSNAME_LEN + MAX_MOBSNAME_LEN + 64];

  sprintf(outstrn, "\n'%s&n' unequipped on '%s&n'.\n\n",
          getObjShortName(objHere->objectPtr), getMobShortName(mobH->mobPtr));
  displayColorString(outstrn);

  g_madeChanges = true;
}


//
// unequipMob : takes items from a mob's equipment list and unequips them
//

void unequipMob(const char *strn)
{
  char mobName[MAX_PROMPTINPUT_LEN], whatMatched;
  editableListNode *mobPtr;
  mobHere *mobH;
  objectHere *objHere;
  size_t numbLines = 0;


 // get mob name

  getArg(strn, 1, mobName, MAX_PROMPTINPUT_LEN - 1);

  checkEditableList(mobName, g_currentRoom->editableListHead, &whatMatched, &mobPtr, 1);

  switch (whatMatched)
  {
    case NO_MATCH : 
      _outtext("\nNothing found that matches that keyword.\n\n");  
      return;

    case ENTITY_MOB :
      mobH = (mobHere *)(mobPtr->entityPtr);

    // make sure mob has some equipment

      if (!mobEquippingSomething(mobH))
      {
        _outtext("\nThat mob is not equipping anything.\n\n");
        return;
      }

      break;

    default :
      char outstrnerr[256];

      sprintf(outstrnerr, "\n%ss cannot equip anything.\n\n",
              getEntityTypeStrn(whatMatched));

      outstrnerr[1] = toupper(outstrnerr[1]);

      _outtext(outstrnerr);

      return;
  }

 // if there are two arguments or more, unequip specific piece

  if (numbArgs(strn) >= 2)
  {
    char arg2[MAX_PROMPTINPUT_LEN];

    unequipMobSpecific(mobH, getArg(strn, 2, arg2, MAX_PROMPTINPUT_LEN - 1));

    return;
  }

 // one arg, unequip everything

  _outtext("\n");

  for (uint pos = WEAR_LOW; pos <= WEAR_TRUEHIGH; pos++)
  {
    objHere = mobH->equipment[pos];

    if (!objHere) 
      continue;

    addObjHeretoList(&(mobH->inventoryHead), objHere, false);

    mobH->equipment[pos] = NULL;

    g_madeChanges = true;

    char outstrn[MAX_OBJSNAME_LEN + MAX_MOBSNAME_LEN + 64];

    sprintf(outstrn, "'%s&n' unequipped on '%s&n'.\n",
            getObjShortName(objHere->objectPtr), 
            getMobShortName(mobH->mobPtr));

    if (checkPause(outstrn, numbLines))
      return;
  }

  _outtext("\n");
}
