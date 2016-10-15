//
//  File: dispobj.cpp    originally part of durisEdit
//
//  Usage: functions for displaying information on objects
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

#include "object.h"
#include "traps.h"

extern "C" flagDef room_bits[], extra_bits[], extra2_bits[], wear_bits[],
                          affected1_bits[], affected2_bits[], affected3_bits[], 
                          affected4_bits[], affected5_bits[],
                          action_bits[], aggro_bits[], aggro2_bits[], aggro3_bits[];
extern flagDef g_npc_class_bits[];
extern flagDef g_race_names[];
extern room *g_currentRoom;


//
// displayObjectTypeList : one arg version, always scans titles
//

void displayObjectTypeList(const char *strn)
{
  displayObjectTypeList(strn, true);
}


//
// displayObjectTypeList : Displays the list of object types loaded into
//                         DE, starting at objectHead
//

void displayObjectTypeList(const char *strn, const bool scanTitle)
{
  const objectType *obj;
  size_t lines = 1;
  uint numb, high = getHighestObjNumber();
  char outStrn[512];
  bool foundObj = false, vnum, listAll = false;


  if (noObjTypesExist())
  {
    _outtext("\nThere are currently no object types.\n\n");

    return;
  }

  if (!strn || (strn[0] == '\0'))
  {
    listAll = true;
  }
  else
  {
    vnum = strnumber(strn);
    numb = strtoul(strn, NULL, 10);
  }

  _outtext("\n\n");

  for (uint objNumb = getLowestObjNumber(); objNumb <= high; objNumb++)
  {
    if (objExists(objNumb))
    {
      char noColorTitle[MAX_OBJSNAME_LEN + 1];

      obj = findObj(objNumb);

      if (!listAll)
      {
        strcpy(noColorTitle, obj->objShortName);
        remColorCodes(noColorTitle);
      }

      if (listAll ||
          (vnum && (numb == objNumb)) ||
          scanKeyword(strn, obj->keywordListHead) ||
          (scanTitle && strstrnocase(noColorTitle, strn)))
      {
        sprintf(outStrn, "%s&n (#%u)\n", obj->objShortName, objNumb);

        foundObj = true;

        if (checkPause(outStrn, lines))
          return;
      }
    }
  }

  if (!foundObj) 
    _outtext("No matching object types found.\n");

  _outtext("\n");
}


//
// displayObjectHereList
//

void displayObjectHereList(void)
{
  const room *room;
  const mobHere *mob;
  char strn[1024];
  bool foundOne = false;
  uint numb = getLowestRoomNumber();
  const uint high = getHighestRoomNumber();
  size_t lines = 1;
  uint j;


  _outtext("\n\n");

  for (; numb <= high; numb++)
  {
    if (!roomExists(numb))
      continue;

    room = findRoom(numb);

    sprintf(strn, " in %s&n (#%u)",
            room->roomName, room->roomNumber);

    if (displayEntireObjectHereList(room->objectHead, strn, lines, &foundOne)) 
      return;

    mob = room->mobHead;
    while (mob)
    {
      sprintf(strn, " equipped by %s&n (#%u) [room #%u]",
              getMobShortName(mob->mobPtr), mob->mobNumb, room->roomNumber);

     // not really lists, but it works

      for (j = WEAR_LOW; j <= WEAR_TRUEHIGH; j++)
      {
        if (displayEntireObjectHereList(mob->equipment[j], strn, lines, &foundOne)) 
          return;
      }

      sprintf(strn, " carried by %s&n (#%u) [room #%u]",
              getMobShortName(mob->mobPtr), mob->mobNumb, room->roomNumber);

      if (displayEntireObjectHereList(mob->inventoryHead, strn, lines, &foundOne)) 
        return;

      mob = mob->Next;
    }
  }

  if (!foundOne)
    _outtext("There are no objects loaded in this zone.\n");

  _outtext("\n");
}


//
// displayEntireObjectHereList - this one shows a list of stuff in an objectHere
//                               list, and all the stuff inside the objHeres -
//                               returns TRUE if user hits Q
//

bool displayEntireObjectHereList(const objectHere *objHead, const char *postStrn, size_t &lines,
                                 bool *foundMatch)
{
  const objectHere *objHere = objHead;
  char strn[2048];


  while (objHere)
  {
    sprintf(strn, "%s&n (#%u)%s\n",
            getObjShortName(objHere->objectPtr), objHere->objectNumb, postStrn);

    *foundMatch = true;

    if (checkPause(strn, lines))
      return true;

    if (objHere->objInside)
    {
      sprintf(strn, " inside %s&n (#%u)",
              getObjShortName(objHere->objectPtr), objHere->objectNumb);

      if (displayEntireObjectHereList(objHere->objInside, strn, lines, foundMatch)) 
        return true;
    }

    objHere = objHere->Next;
  }

  return false;
}


//
// displayEntireObjectHereList - this one shows a list of stuff in an objectHere
//                               list, and all the stuff inside the objHeres -
//                               takes keyStrn
//

bool displayEntireObjectHereList(const objectHere *objHead, const char *postStrn, size_t &lines, 
                                 const char *keyStrn, const uint roomNumber, bool *foundMatch)
{
  const objectHere *objHere = objHead;
  char strn[1024];


  while (objHere)
  {
    if (objHere->objectPtr && scanKeyword(keyStrn, objHere->objectPtr->keywordListHead))
    {
      *foundMatch = true;

      sprintf(strn, "%s&n (#%u)%s\n",
              getObjShortName(objHere->objectPtr), objHere->objectNumb, postStrn);

      if (checkPause(strn, lines))
        return true;
    }

    if (objHere->objInside)
    {
      sprintf(strn, " inside %s&n (#%u)  [room #%u]",
              getObjShortName(objHere->objectPtr), objHere->objectNumb, roomNumber);

      if (displayEntireObjectHereList(objHere->objInside, strn, lines, keyStrn, roomNumber, foundMatch))
        return true;
    }

    objHere = objHere->Next;
  }

  return false;
}


//
// displayEntireObjectHereList - this one shows a list of stuff in an objectHere
//                               list, and all the stuff inside the objHeres - returns
//                               true if user hits Q - takes obj vnum
//

bool displayEntireObjectHereList(const objectHere *objHead, const char *postStrn, size_t &lines,
                                 const uint objNumber, const uint roomNumber, bool *foundMatch)
{
  const objectHere *objHere = objHead;
  char strn[1024];


  while (objHere)
  {
    if (objHere->objectNumb == objNumber)
    {
      *foundMatch = true;

      sprintf(strn, "%s&n (#%u)%s\n",
              getObjShortName(objHere->objectPtr), objHere->objectNumb, postStrn);

      if (checkPause(strn, lines))
        return true;
    }

    if (objHere->objInside)
    {
      sprintf(strn, " inside %s&n (#%u)  [room #%u]",
              getObjShortName(objHere->objectPtr), objHere->objectNumb, roomNumber);

      if (displayEntireObjectHereList(objHere->objInside, strn, lines,
                                      objNumber, roomNumber, foundMatch))
        return true;
    }

    objHere = objHere->Next;
  }

  return false;
}


//
// displayObjApplyTypeList : Displays list of object apply types
//

void displayObjApplyTypeList(const char *searchStrn)
{
  displayList(APPLY_NONE, APPLY_LAST, searchStrn, getObjApplyStrn, NULL, NULL, "apply type");
}


//
// displayObjectInfo : Displays various info on the object record passed to it
//
//   *args : would-be vnum/keyword of object
//

void displayObjectInfo(const char *args)
{
  char strn[4096],
       fstrn[2048], fstrn2[2048], fstrn3[2048], fstrn4[2048],
        fstrn5[2048], fstrn6[2048], fstrn7[2048], fstrn8[2048],
        fstrn9[2048];

  const objectType *obj;


 // input-checking

  if (!strlen(args))
  {
    _outtext("\nSpecify an object vnum/keyword to stat.\n\n");

    return;
  }

  obj = getMatchingObj(args);
  if (!obj)
  {
    if (strnumber(args))
      _outtext("\nNo object type exists with that vnum.\n\n");
    else
      _outtext("\nNo object type exists with that keyword.\n\n");

    return;
  }

  sprintf(strn,
"\n"
"&+YVnum:&n %u  &+YShort name:&n %s&n\n"
"&+YLong name:&n %s&n\n"
"&+YKeywords:&n %s\n"
"&+YExtra descs:&n ",
obj->objNumber, obj->objShortName,
obj->objLongName,
getReadableKeywordStrn(obj->keywordListHead, fstrn, 2047));

  displayColorString(strn);

  displayExtraDescNodes(obj->extraDescHead);

  sprintf(strn, 
"\n"
"\n"
"&+YObject extra flags :&n %u (%s)\n"
"&+YObject extra2 flags:&n %u (%s)\n"
"&+YObject wear flags  :&n %u (%s)\n"
"&+YObject anti flags  :&n %u (%s)\n"
"&+YObject anti2 flags :&n %u (%s)\n"
"&+YObject aff1 flags  :&n %u (%s)\n"
"&+YObject aff2 flags  :&n %u (%s)\n"
"&+YObject aff3 flags  :&n %u (%s)\n"
"&+YObject aff4 flags  :&n %u (%s)\n"
"\n"
"&+CPress a key to continue...",

obj->extraBits,
  getFlagStrn(obj->extraBits, extra_bits, fstrn, 1023),
obj->extra2Bits,
  getFlagStrn(obj->extra2Bits, extra2_bits, fstrn2, 1023),
obj->wearBits,
  getFlagStrn(obj->wearBits, wear_bits, fstrn3, 1023),
obj->antiBits,
  getFlagStrn(obj->antiBits, g_npc_class_bits, fstrn4, 1023),
obj->anti2Bits,
  getFlagStrn(obj->anti2Bits, g_race_names, fstrn5, 1023),
obj->affect1Bits,
  getFlagStrn(obj->affect1Bits, affected1_bits, fstrn6, 1023),
obj->affect2Bits,
  getFlagStrn(obj->affect2Bits, affected2_bits, fstrn7, 1023),
obj->affect3Bits,
  getFlagStrn(obj->affect3Bits, affected3_bits, fstrn8, 1023),
obj->affect4Bits,
  getFlagStrn(obj->affect4Bits, affected4_bits, fstrn9, 1023));

  displayColorString(strn);

  getkey();

  sprintf(strn,
"\n\n"
"&+YObject type:&n %u (%s)\n"
"&+YValue #1   :&n %d (%s)\n"
"&+YValue #2   :&n %d (%s)\n"
"&+YValue #3   :&n %d (%s)\n"
"&+YValue #4   :&n %d (%s)\n"
"&+YValue #5   :&n %d (%s)\n"
"&+YValue #6   :&n %d (%s)\n"
"&+YValue #7   :&n %d (%s)\n"
"&+YValue #8   :&n %d (%s)\n"
"\n"
"&+YObj apply #1:&n %u (%s) by %d\n"
"&+YObj apply #2:&n %u (%s) by %d\n"
"\n"
"&+YWeight:&n %d    &+YValue:&n %u    &+YCondition:&n %u\n"
"\n",
obj->objType, getObjTypeStrn(obj->objType),
obj->objValues[0], getObjValueStrn(obj->objType, 0, obj->objValues[0], fstrn, true),
obj->objValues[1], getObjValueStrn(obj->objType, 1, obj->objValues[1], fstrn2, true),
obj->objValues[2], getObjValueStrn(obj->objType, 2, obj->objValues[2], fstrn3, true),
obj->objValues[3], getObjValueStrn(obj->objType, 3, obj->objValues[3], fstrn4, true),
obj->objValues[4], getObjValueStrn(obj->objType, 4, obj->objValues[4], fstrn5, true),
obj->objValues[5], getObjValueStrn(obj->objType, 5, obj->objValues[5], fstrn6, true),
obj->objValues[6], getObjValueStrn(obj->objType, 6, obj->objValues[6], fstrn7, true),
obj->objValues[7], getObjValueStrn(obj->objType, 7, obj->objValues[7], fstrn8, true),

obj->objApply[0].applyWhere, getObjApplyStrn(obj->objApply[0].applyWhere),
  obj->objApply[0].applyModifier,
obj->objApply[1].applyWhere, getObjApplyStrn(obj->objApply[1].applyWhere),
  obj->objApply[1].applyModifier,

obj->weight, obj->worth, obj->condition);

  displayColorString(strn);
}
