//
//  File: flags.cpp      originally part of durisEdit
//
//  Usage: functions for handling commands related to flags
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

#include "fh.h"
#include "types.h"
#include "flags.h"

extern "C" flagDef room_bits[], extra_bits[], extra2_bits[], wear_bits[],
                   affected1_bits[], affected2_bits[], affected3_bits[], affected4_bits[],
                   action_bits[], aggro_bits[], aggro2_bits[], aggro3_bits[];
extern flagDef g_npc_class_bits[], g_race_names[];
extern bool g_madeChanges;
extern uint g_numbObjTypes, g_numbMobTypes;


//
// whichFlag : compares input versus the short names in the flag array pointed
//             to in flagArr, returning the index of the matching flag or -1
//             if no match
//

int whichFlag(const char *input, const flagDef *flagArr)
{
  for (uint i = 0; flagArr[i].flagShort; i++)
  {
    if (strcmpnocase(input, flagArr[i].flagShort)) 
      return i;
  }

  return -1;
}


//
// getFlagStrn : dumps short names into strn based on the flags that are set
//               in the bitvect flagVal and the names in flagArr
//

char *getFlagStrn(const uint flagVal, const flagDef *flagArr, char *strn, const size_t intMaxLen)
{
  size_t len = 0;


  strn[0] = '\0';

  for (uint i = 0; flagArr[i].flagShort; i++)
  {
    if (flagVal & (1 << i))
    {
      strncpy(strn + len, flagArr[i].flagShort, intMaxLen - len);
      strn[intMaxLen] = '\0';

      len += strlen(flagArr[i].flagShort);

      if (len < intMaxLen)
      {
        strcat(strn, " ");
        len++;
      }

      if (len > intMaxLen)
        return strn;
    }
  }

  remTrailingSpaces(strn);

  return strn;
}


//
// getFlagNameFromList : given flag list, return readable string with value of intVal
//

const char *getFlagNameFromList(const flagDef *flagList, const int intVal)
{
  const flagDef *flagPtr = flagList;

  while (flagPtr->flagShort)
  {
    if (flagPtr->defVal == intVal)
      return flagPtr->flagLong;

    flagPtr++;
  }

  return "unrecognized";
}


//
// fixFlags : "fixes" flags in a bitvector by checking whether each bit 
//            is editable and not set to its default value.
//            if both conditions are true, the bit is toggled
//
//     flagVal : pointer to bitvector being changed
//     flagArr : array of flag info for flagVal
//

uint fixFlags(uint *flagVal, const flagDef *flagArr)
{
  uint changed = 0;

  for (uint i = 0; flagArr[i].flagShort; i++)
  {
    uint val = (*flagVal & (1 << i));
    if (val) 
      val = 1;  // makes things easier to check below

    if ((!flagArr[i].editable) && (flagArr[i].defVal != val))
    {
      *flagVal ^= (1 << i);
      changed++;
    }
  }

  return changed;
}


//
// fixAllFlags : "fixes" flags - that is, sets all uneditable flags that aren't
//               set to their default value to their default value
//

void fixAllFlags(void)
{
  char strn[256];
  uint numbFixed = 0, bitVects = 0;
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestMobNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      numbFixed += fixFlags(&(roomPtr->roomFlags), room_bits);
      bitVects++;
    }
  }

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      objectType *obj = findObj(objNumb);

      numbFixed += fixFlags(&(obj->extraBits), extra_bits);
      bitVects++;
      numbFixed += fixFlags(&(obj->extra2Bits), extra2_bits);
      bitVects++;
      numbFixed += fixFlags(&(obj->antiBits), g_npc_class_bits);
      bitVects++;
      numbFixed += fixFlags(&(obj->anti2Bits), g_race_names);
      bitVects++;
      numbFixed += fixFlags(&(obj->wearBits), wear_bits);
      bitVects++;
      numbFixed += fixFlags(&(obj->affect1Bits), affected1_bits);
      bitVects++;
      numbFixed += fixFlags(&(obj->affect2Bits), affected2_bits);
      bitVects++;
      numbFixed += fixFlags(&(obj->affect3Bits), affected3_bits);
      bitVects++;
      numbFixed += fixFlags(&(obj->affect4Bits), affected4_bits);
      bitVects++;
    }
  }

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      numbFixed += fixFlags(&(mob->actionBits), action_bits);
      bitVects++;
      numbFixed += fixFlags(&(mob->affect1Bits), affected1_bits);
      bitVects++;
      numbFixed += fixFlags(&(mob->affect2Bits), affected2_bits);
      bitVects++;
      numbFixed += fixFlags(&(mob->affect3Bits), affected3_bits);
      bitVects++;
      numbFixed += fixFlags(&(mob->affect4Bits), affected4_bits);
      bitVects++;
      numbFixed += fixFlags(&(mob->aggroBits), aggro_bits);
      bitVects++;
      numbFixed += fixFlags(&(mob->aggro2Bits), aggro2_bits);
      bitVects++;
      numbFixed += fixFlags(&(mob->aggro3Bits), aggro3_bits);
      bitVects++;
      numbFixed += fixFlags(&(mob->mobClass), g_npc_class_bits);
      bitVects++;
    }
  }

  sprintf(strn, "\nReset %u uneditable non-default flag%s in %u flag list%s.\n\n",
          numbFixed, plural(numbFixed), bitVects, plural(bitVects));

  _outtext(strn);

  if (numbFixed) 
    g_madeChanges = true;
}


//
// checkForFlagExistence : checks if flag name exists for rooms, objs, and/or mobs
//

bool checkForFlagExistence(const bool checkRooms, const bool checkObjs, const bool checkMobs,
                           const char *flagName)
{
  uint matched;


  if (checkRooms)
  {
    if (getFlagLocRoom(flagName, &matched, NULL) != -1)
    {
      return true;
    }
  }

  if (checkObjs)
  {
    if (getFlagLocObj(flagName, &matched, NULL) != -1)
    {
      return true;
    }
  }

  if (checkMobs)
  {
    if (getFlagLocMob(flagName, &matched, NULL) != -1)
    {
      return true;
    }
  }

  return false;
}


//
// getFlagLocRoom : given flag name, returns location in flag vector for rooms and sets whichSet and 
//                  flagListPtr for the appropriate flag list
//

int getFlagLocRoom(const char *flagName, uint *whichSet, const flagDef **flagListPtr)
{
  int flagNumb;

  flagNumb = whichFlag(flagName, room_bits);
  if (flagNumb != -1)
  {
    *whichSet = ROOM_FL;
    
    if (flagListPtr)
      *flagListPtr = room_bits;

    return flagNumb;
  }

  *whichSet = 0;
  if (flagListPtr)
    *flagListPtr = NULL;

  return -1;
}


//
// getFlagLocObj : given flag name, returns location in flag vector for objs and sets whichSet and 
//                 flagListPtr for the appropriate flag list
//

int getFlagLocObj(const char *flagName, uint *whichSet, const flagDef **flagListPtr)
{
  int flagNumb;


  flagNumb = whichFlag(flagName, extra_bits);
  if (flagNumb != -1) 
  {
    *whichSet = EXTR_FL;

    if (flagListPtr)
      *flagListPtr = extra_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, extra2_bits);
  if (flagNumb != -1) 
  {
    *whichSet = EXT2_FL;

    if (flagListPtr)
      *flagListPtr = extra2_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, wear_bits);
  if (flagNumb != -1) 
  {
    *whichSet = WEAR_FL;

    if (flagListPtr)
      *flagListPtr = wear_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, g_npc_class_bits);
  if (flagNumb != -1) 
  {
    *whichSet = ANTI_FL;

    if (flagListPtr)
      *flagListPtr = g_npc_class_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, g_race_names);
  if (flagNumb != -1) 
  {
    *whichSet = ANT2_FL;

    if (flagListPtr)
      *flagListPtr = g_race_names;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, affected1_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AFF1_FL;

    if (flagListPtr)
      *flagListPtr = affected1_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, affected2_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AFF2_FL;

    if (flagListPtr)
      *flagListPtr = affected2_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, affected3_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AFF3_FL;

    if (flagListPtr)
      *flagListPtr = affected3_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, affected4_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AFF4_FL;

    if (flagListPtr)
      *flagListPtr = affected4_bits;

    return flagNumb;
  }

  *whichSet = 0;
  if (flagListPtr)
    *flagListPtr = NULL;

  return -1;
}


//
// getFlagLocMob : given flag name, returns location in flag vector for mobs and sets whichSet and 
//                 flagListPtr for the appropriate flag list
//

int getFlagLocMob(const char *flagName, uint *whichSet, const flagDef **flagListPtr)
{
  int flagNumb;


  flagNumb = whichFlag(flagName, action_bits);
  if (flagNumb != -1) 
  {
    *whichSet = MACT_FL;

    if (flagListPtr)
      *flagListPtr = action_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, affected1_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AFF1_FL;

    if (flagListPtr)
      *flagListPtr = affected1_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, affected2_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AFF2_FL;

    if (flagListPtr)
      *flagListPtr = affected2_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, affected3_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AFF3_FL;

    if (flagListPtr)
      *flagListPtr = affected3_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, affected4_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AFF4_FL;

    if (flagListPtr)
      *flagListPtr = affected4_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, aggro_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AGGR_FL;

    if (flagListPtr)
      *flagListPtr = aggro_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, aggro2_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AGGR2_FL;

    if (flagListPtr)
      *flagListPtr = aggro2_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, aggro3_bits);
  if (flagNumb != -1) 
  {
    *whichSet = AGGR3_FL;

    if (flagListPtr)
      *flagListPtr = aggro3_bits;

    return flagNumb;
  }

  flagNumb = whichFlag(flagName, g_npc_class_bits);
  if (flagNumb != -1) 
  {
    *whichSet = CLASS_FL;

    if (flagListPtr)
      *flagListPtr = g_npc_class_bits;

    return flagNumb;
  }

  *whichSet = 0;
  if (flagListPtr)
    *flagListPtr = NULL;

  return -1;
}


//
// getRoomFlagPtr
//

uint *getRoomFlagPtr(room *roomPtr, const uint flagVect)
{
  switch (flagVect)
  {
    case ROOM_FL : return &(roomPtr->roomFlags);

    default : return NULL;
  }
}


//
// getObjFlagPtr
//

uint *getObjFlagPtr(objectType *obj, const uint flagVect)
{
  switch (flagVect)
  {
    case EXTR_FL : return &(obj->extraBits);
    case EXT2_FL : return &(obj->extra2Bits);
    case WEAR_FL : return &(obj->wearBits);
    case ANTI_FL : return &(obj->antiBits);
    case ANT2_FL : return &(obj->anti2Bits);
    case AFF1_FL : return &(obj->affect1Bits);
    case AFF2_FL : return &(obj->affect2Bits);
    case AFF3_FL : return &(obj->affect3Bits);
    case AFF4_FL : return &(obj->affect4Bits);

    default : return NULL;
  }
}


//
// getMobFlagPtr
//

uint *getMobFlagPtr(mobType *mob, const uint flagVect)
{
  switch (flagVect)
  {
    case MACT_FL  : return &(mob->actionBits);
    case AFF1_FL  : return &(mob->affect1Bits);
    case AFF2_FL  : return &(mob->affect2Bits);
    case AFF3_FL  : return &(mob->affect3Bits);
    case AFF4_FL  : return &(mob->affect4Bits);
    case AGGR_FL  : return &(mob->aggroBits);
    case AGGR2_FL : return &(mob->aggro2Bits);
    case AGGR3_FL : return &(mob->aggro3Bits);
    case CLASS_FL : return &(mob->mobClass);

    default : return NULL;
  }
}

        
//
// which : much like the which command for Duris..  except this one doesn't
//         have to take a room, obj, or mob arg (and it checks all the
//         flag fields)
//

void which(const char *args)
{
  char outStrn[512], flagName[256];
  const char *whatStrn;

  bool foundSomething = false, foundRoom = false, foundObj = false,
       displayedEqual = false, notFlag = false;

  bool checkRoom = false, checkObj = false, checkMob = false;

  size_t lines = 1;
  uint matched;

  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestMobNumber();


  if (strlen(args) == 0)
  {
    _outtext("\n"
"Specify a flagname as the first argument, or <room|obj|mob> then a flagname.\n\n");

    return;
  }

  getArg(args, 1, flagName, 255);

  if (strlefti("ROOM", flagName)) 
  {
    checkRoom = true;
    whatStrn = "room";
  }
  else if (strlefti("OBJECT", flagName)) 
  {
    checkObj = true;
    whatStrn = "object";
  }
  else if (strlefti("MOB", flagName) || strlefti("CHAR", flagName)) 
  {
    checkMob = true;
    whatStrn = "mob";
  }
  else 
  {
    checkRoom = checkObj = checkMob = true;
    whatStrn = "rooms, objects, or mob";
  }

 // if not checking all, require flag name in second arg

  if (!checkRoom || !checkObj || !checkMob)
  {
    getArg(args, 2, flagName, 255);

    if (!flagName[0])
    {
      _outtext("\n"
"Specify a flag name to look for as the second argument when specifically\n"
"checking rooms, objects, or mobs.\n\n");

      return;
    }
  }
  else 
  {
    strncpy(flagName, args, 255);
    flagName[255] = '\0';
  }

  if (flagName[0] == '!')
  {
    notFlag = true;
    deleteChar(flagName, 0);
  }

 // check for flag existence

  if (!checkForFlagExistence(checkRoom, checkObj, checkMob, flagName))
  {
    sprintf(outStrn, "\nThat flag does not exist for %ss.\n\n", whatStrn);
    _outtext(outStrn);

    return;
  }

  _outtext("\n\n");

 // check for rooms with this flagname set

  if (checkRoom)
  {
    const int flagNumb = getFlagLocRoom(flagName, &matched, NULL);

   // might not get a match if checking all

    if (matched)
    {
      const uint flagVal = 1 << flagNumb;

      for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
      {
        if (roomExists(roomNumb))
        {
          room *roomPtr = findRoom(roomNumb);
          const uint *flagPtr = getRoomFlagPtr(roomPtr, matched);

          if (notFlag ? (!(*flagPtr & flagVal)) :
                        (*flagPtr & flagVal))
          {
            sprintf(outStrn, "%s&n (#%u)\n", roomPtr->roomName, roomPtr->roomNumber);

            foundSomething = foundRoom = true;

            if (checkPause(outStrn, lines))
              return;
          }
        }
      }
    }
  }

 // check for objects with this flagname set

  if (checkObj)
  {
    const int flagNumb = getFlagLocObj(flagName, &matched, NULL);

   // might not get a match if checking all

    if (matched)
    {
      const uint flagVal = 1 << flagNumb;

      for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
      {
        if (objExists(objNumb))
        {
          objectType *obj = findObj(objNumb);
          const uint *flagPtr = getObjFlagPtr(obj, matched);

          if (notFlag ? (!(*flagPtr & flagVal)) : 
                        (*flagPtr & flagVal))
          {
            if (foundRoom && !displayedEqual)
            {
              _outtext("===\n");
              lines++;
              displayedEqual = true;
            }

            sprintf(outStrn, "%s&n (#%u)\n", obj->objShortName, objNumb);

            foundSomething = foundObj = true;

            if (checkPause(outStrn, lines))
              return;
          }
        }
      }
    }
  }

  displayedEqual = false;

 // check for mobs with this flagname set

  if (checkMob)
  {
    const int flagNumb = getFlagLocMob(flagName, &matched, NULL);

   // might not get a match if checking all

    if (matched)
    {
      const uint flagVal = 1 << flagNumb;

      for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
      {
        if (mobExists(mobNumb))
        {
          mobType *mob = findMob(mobNumb);
          const uint *flagPtr = getMobFlagPtr(mob, matched);

          if (notFlag ? (!(*flagPtr & flagVal)) : 
                        (*flagPtr & flagVal))
          {
            if (foundObj && !displayedEqual)
            {
              _outtext("===\n");
              lines++;
              displayedEqual = true;
            }

            sprintf(outStrn, "%s&n (#%u)\n", mob->mobShortName, mobNumb);

            foundSomething = true;

            if (checkPause(outStrn, lines))
              return;
          }
        }
      }
    }
  }

  if (!foundSomething)
  {
    sprintf(outStrn, "No matching %ss found.\n", whatStrn);
    _outtext(outStrn);
  }

  _outtext("\n");
}


//
// massSet : mass-set a particular bit in a bitvector on or off for all
//           rooms/object types/mob types
//

#define MASSSET_ARG_ERROR "\n" \
"The \"massset\" command takes three arguments, in the format 'massset\n" \
"<room|object|mob> <flagname> <on/1|off/0>'.\n\n"

void massSet(const char *args)
{
  char flagName[256], strn[256];
  const char *whatStrn;
  bool setOn;
  bool setRoom = false, setObj = false, setMob = false;
  uint matched;

  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestMobNumber();


  if (strlen(args) == 0)
  {
    _outtext(MASSSET_ARG_ERROR);

    return;
  }

  getArg(args, 1, flagName, 255);

  if (strlefti("ROOM", flagName)) 
  {
    setRoom = true;

    whatStrn = "room";
  }
  else if (strlefti("OBJECT", flagName)) 
  {
    if (noObjTypesExist())
    {
      _outtext("\nThere are no object types to set.\n\n");
      return;
    }

    setObj = true;

    whatStrn = "object";
  }
  else if (strlefti("MOB", flagName) || strlefti("CHAR", flagName)) 
  {
    if (noMobTypesExist())
    {
      _outtext("\nThere are no mob types to set.\n\n");
      return;
    }

    setMob = true;

    whatStrn = "mob";
  }
  else
  {
    _outtext(MASSSET_ARG_ERROR);

    return;
  }

  getArg(args, 2, flagName, 255);
  if (!flagName[0])
  {
    _outtext(MASSSET_ARG_ERROR);

    return;
  }

  getArg(args, 3, strn, 255);

  if (strcmpnocase(strn, "ON") || !strcmp(strn, "1")) 
  {
    setOn = true;
  }
  else if (strcmpnocase(strn, "OFF") || !strcmp(strn, "0")) 
  {
    setOn = false;
  }
  else
  {
    _outtext(MASSSET_ARG_ERROR);

    return;
  }

 // check for flag existence

  if (!checkForFlagExistence(setRoom, setObj, setMob, flagName))
  {
    sprintf(strn, "\nThat flag does not exist for %ss.\n\n", whatStrn);
    _outtext(strn);

    return;
  }

  int flagNumb;
  const flagDef *flagList;

 // check for rooms with this flagname set

  if (setRoom)
  {
    flagNumb = getFlagLocRoom(flagName, &matched, &flagList);
  }
  else if (setObj)
  {
    flagNumb = getFlagLocObj(flagName, &matched, &flagList);
  }
  else if (setMob)
  {
    flagNumb = getFlagLocMob(flagName, &matched, &flagList);
  }

  if (!flagList[flagNumb].editable && !getEditUneditableFlagsVal())
  {
    _outtext("\nThat flag value is not editable.\n\n");
    return;
  }

  const uint flagVal = 1 << flagNumb;

  if (setRoom)
  {
    for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
    {
      if (roomExists(roomNumb))
      {
        room* roomPtr = findRoom(roomNumb);
        uint* flagPtr = getRoomFlagPtr(roomPtr, matched);

        if (setOn) 
          *flagPtr |= flagVal;
        else 
          *flagPtr &= ~flagVal;
      }
    }
  }
  else
  if (setObj)
  {
    for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
    {
      if (objExists(objNumb))
      {
        objectType* obj = findObj(objNumb);
        uint* flagPtr = getObjFlagPtr(obj, matched);

        if (setOn) 
          *flagPtr |= flagVal;
        else 
          *flagPtr &= ~flagVal;
      }
    }
  }
  else
  if (setMob)
  {
    for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
    {
      if (mobExists(mobNumb))
      {
        mobType* mob = findMob(mobNumb);
        uint* flagPtr = getMobFlagPtr(mob, matched);

        if (setOn) 
          *flagPtr |= flagVal;
        else 
          *flagPtr &= ~flagVal;
      }
    }
  }

  sprintf(strn, "\n'%s' flag set %s for all %ss.\n\n",
          flagName, getOnOffStrn(setOn), whatStrn);
  _outtext(strn);

  g_madeChanges = true;
}
