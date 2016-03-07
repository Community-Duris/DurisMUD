//
//  File: check.cpp      originally part of durisEdit
//
//  Usage: functions galore for checking validity of everything
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
#include <string.h>

#include "fh.h"
#include "types.h"

#include "obj/objsize.h"
#include "obj/traps.h"
#include "obj/armor.h"
#include "spells.h"
#include "obj/weapons.h"
#include "obj/missiles.h"
#include "obj/liquids.h"
#include "obj/shields.h"
#include "obj/material.h"
#include "obj/objcraft.h"
#include "defines.h"

#include "misc/mudcomm.h"
#include "check.h"

extern zone g_zoneRec;
extern "C" flagDef room_bits[], extra_bits[], extra2_bits[], wear_bits[],
                   affected1_bits[], affected2_bits[], affected3_bits[], affected4_bits[],
                   action_bits[], aggro_bits[], aggro2_bits[], aggro3_bits[];
extern flagDef g_npc_class_bits[], g_race_names[];
extern char *g_exitnames[];

//
// outCheckError : writes error to file, pauses if numbLines is too high, returns true if user quits
//
//    adds 'Error: ' to start of error string unless first char is a \n
//

bool outCheckError(const char *error, FILE *file, size_t& numbLines)
{
  char outerror[2048];
  const char *outptr;

  if (file) 
    fputs(error, file);

  if (error[0] != '\n')
  {
    strcpy(outerror, "Error: ");
    strncpy(outerror + strlen(outerror), error, 2000);

    outerror[2007] = '\0';

    outptr = outerror;
  }
  else
  {
    outptr = error;
  }

  if (getPauseCheckScreenfulVal()) 
  {
    if (checkPause(outptr, numbLines))
      return true;
  }
  else
  {
    _outtext(outptr);
  }

  return false;
}


//
// checkFlags
//

uint checkFlags(FILE *file, const uint flagVal, const flagDef *flagArr, const char *flagName, 
                const char *entityName, const uint entityNumb, size_t& numbLines, bool *userQuit)
{
  uint errors = 0, i;
  char strn[256];


  for (i = 0; flagArr[i].flagShort; i++)
  {
    if (!flagArr[i].editable && (((flagVal & (1 << i)) ? 1 : 0) != flagArr[i].defVal))
    {
      errors++;

      sprintf(strn,
"%s #%u - %s flag's %s flag is set to %u instead of %u\n",
              entityName, entityNumb, flagName, flagArr[i].flagShort,
              (flagVal & (1 << i)) ? 1 : 0, flagArr[i].defVal);

      if (outCheckError(strn, file, numbLines))
      {
        *userQuit = true;
        return errors;
      }
    }
  }

  return errors;
}


//
// checkAllFlags
//

uint checkAllFlags(FILE *file, size_t& numbLines, bool* userQuit)
{
  const room *roomPtr;
  const objectType *obj;
  const mobType *mob;
  uint errors = 0;
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestMobNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      errors += checkFlags(file, roomPtr->roomFlags, room_bits,
                           "room", "room", roomPtr->roomNumber, numbLines, userQuit);

      if (*userQuit)
        return errors;
    }
  }

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      errors += checkFlags(file, obj->extraBits, extra_bits,
                           "extra", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, obj->extra2Bits, extra2_bits,
                           "extra2", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, obj->wearBits, wear_bits,
                           "wear", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, obj->affect1Bits, affected1_bits,
                           "aff1", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, obj->affect2Bits, affected2_bits,
                           "aff2", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, obj->affect3Bits, affected3_bits,
                           "aff3", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, obj->affect4Bits, affected4_bits,
                           "aff4", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, obj->antiBits, g_npc_class_bits,
                           "anti", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, obj->anti2Bits, g_race_names,
                           "anti2", "object", objNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;
    }
  }

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mob = findMob(mobNumb);

      errors += checkFlags(file, mob->mobClass, g_npc_class_bits,
                           "class", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, mob->actionBits, action_bits,
                           "action", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, mob->affect1Bits, affected1_bits,
                           "aff1", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, mob->affect2Bits, affected2_bits,
                           "aff2", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, mob->affect3Bits, affected3_bits,
                           "aff3", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, mob->affect4Bits, affected4_bits,
                           "aff4", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, mob->aggroBits, aggro_bits,
                           "aggro", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;

      errors += checkFlags(file, mob->aggro2Bits, aggro2_bits,
                           "aggro2", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;
      
      errors += checkFlags(file, mob->aggro3Bits, aggro3_bits,
                           "aggro3", "mob", mobNumb, numbLines, userQuit);

      if (*userQuit)
        return errors;
    }
  }

  return errors;
}


//
// checkZone
//

uint checkZone(FILE *file, size_t& numbLines, bool* userQuit)
{
  const char *strn;
  uint errors = 0;

  if ((g_zoneRec.resetMode < ZONE_RESET_LOWEST) || (g_zoneRec.resetMode > ZONE_RESET_HIGHEST))
  {
    strn = "zone reset mode is invalid\n";
    errors++;

    if (outCheckError(strn, file, numbLines))
    {
      *userQuit = true;
      return errors;
    }
  }

  if ((g_zoneRec.zoneDiff < 1) || (g_zoneRec.zoneDiff > 10))
  {
    strn = "zone difficulty is out of range (1-10)\n";
    errors++;

    if (outCheckError(strn, file, numbLines))
    {
      *userQuit = true;
      return errors;
    }
  }

  if (!strlen(g_zoneRec.zoneName))
  {
    strn = "zone has no name\n";
    errors++;

    if (outCheckError(strn, file, numbLines))
    {
      *userQuit = true;
      return errors;
    }
  }

  return errors;
}


//
// checkRooms
//

uint checkRooms(FILE *file, size_t& numbLines, bool *userQuit)
{
  const room *roomPtr;
  char strn[256];
  uint errors = 0;
  const uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

     // check room name

      if (!roomPtr->roomName[0])
      {
        sprintf(strn, "room #%u has no name\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for rooms with no descriptions

      if (!roomPtr->roomDescHead)
      {
        sprintf(strn, "room #%u has no description\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check fall chance

      if ((roomPtr->fallChance < 0) || (roomPtr->fallChance > 100))
      {
        sprintf(strn, "room #%u's fall chance is out of range (0-100)\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check current chance/direction

      if ((roomPtr->current < 0) || (roomPtr->current > 100))
      {
        sprintf(strn, "room #%u's current chance is out of range (0-100)\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

      if ((roomPtr->currentDir < EXIT_LOWEST) || (roomPtr->currentDir > EXIT_HIGHEST))
      {
        sprintf(strn, "room #%u's current direction is out of range\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check mana flag type

      if (/*(roomPtr->manaFlag < MANA_LOWEST) ||*/ (roomPtr->manaFlag > MANA_HIGHEST))
      {
        sprintf(strn, "room #%u's mana type is out of range\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check sector type

      if (/*(roomPtr->sectorType < SECT_LOWEST) ||*/ (roomPtr->sectorType > SECT_HIGHEST))
      {
        sprintf(strn, "room #%u's sector type is out of range\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for single-file rooms with incorrect number of exits

      if ((roomPtr->roomFlags & SINGLE_FILE) && (getNumbExits(roomPtr) != 2))
      {
        sprintf(strn, "room #%u has SINGLE_FILE flag set, but has %u exit%s instead of 2\n",
                roomNumb, getNumbExits(roomPtr), plural(getNumbExits(roomPtr)));
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for underwater rooms improperly set one way or the other

      if (((roomPtr->sectorType == SECT_UNDERWATER) || (roomPtr->sectorType == SECT_UNDERWATER_GR) ||
           (roomPtr->sectorType == SECT_PLANE_OF_WATER)) &&
          !(roomPtr->roomFlags & UNDERWATER))
      {
        sprintf(strn,
  "room #%u has sector type of UNDERWATER, UNDERWATER_GROUND, or PLANE_OF_WATER, but doesn't have UNDERWATER "
  "flag set\n",
                roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

      if ((!((roomPtr->sectorType == SECT_UNDERWATER) || (roomPtr->sectorType == SECT_UNDERWATER_GR) ||
           (roomPtr->sectorType == SECT_PLANE_OF_WATER))) &&
           (roomPtr->roomFlags & UNDERWATER))
      {
        sprintf(strn,
  "room #%u has UNDERWATER flag set, but a sector type other than UNDERWATER, UNDERWATER_GROUND, or "
  "PLANE_OF_WATER\n",
                roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for no-ground/fall rooms with no valid down exit

      if (((roomPtr->sectorType == SECT_NO_GROUND) || (roomPtr->sectorType == SECT_UNDRWLD_NOGROUND)) &&
           (!roomPtr->exits[DOWN] || (roomPtr->exits[DOWN]->destRoom < 0)))
      {
        sprintf(strn,
  "room #%u is NO_GROUND or UNDRWLD_NOGROUND but has no valid down exit\n",
                roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

      if (roomPtr->fallChance && (!roomPtr->exits[DOWN] || (roomPtr->exits[DOWN]->destRoom < 0)))
      {
        sprintf(strn,
  "room #%u has a fall percentage but no valid down exit\n",
                roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for rooms with more than one light flag set

      if (((roomPtr->roomFlags & DARK) &&
          (roomPtr->roomFlags & (TWILIGHT | MAGIC_LIGHT | MAGIC_DARK))) ||
          ((roomPtr->roomFlags & TWILIGHT) &&
          (roomPtr->roomFlags & (MAGIC_LIGHT | MAGIC_DARK))) ||
          ((roomPtr->roomFlags & MAGIC_LIGHT) && (roomPtr->roomFlags & MAGIC_DARK)))
      {
        sprintf(strn, "room #%u has some combination of nonsensical light flags set\n",
                roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for rooms with a nonsensical combination of heal flags set

      if ((roomPtr->roomFlags & HEAL) && (roomPtr->roomFlags & NO_HEAL))
      {
        sprintf(strn, "room #%u is both HEAL and NOHEAL - makes no sense\n",
                roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
    }
  }

  return errors;
}


//
// checkLoneRooms
//

uint checkLoneRooms(FILE *file, size_t& numbLines, bool* userQuit)
{
  const room *roomPtr;
  char strn[256];
  uint errors = 0;
  const uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      if (noExitsLeadtoRoom(roomNumb))
      {
        sprintf(strn, "no exits lead to room #%u\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

      if (noExitOut(roomPtr))
      {
        sprintf(strn, "no exits lead out of room #%u\n", roomNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
    }
  }

  return errors;
}


//
// checkExitKeyLoaded - true if error
//

bool checkExitKeyLoaded(const roomExit *exitNode)
{
  if (!exitNode) 
    return false;

 // exit with key vnum specified, but no key loaded

  if ((exitNode->keyNumb > 0) && !isObjTypeUsed(exitNode->keyNumb))
    return true;
  else
    return false;
}


//
// checkMissingKeys : returns number of errors
//

uint checkMissingKeys(FILE *file, size_t& numbLines, bool* userQuit)
{
  uint errors = 0;
  const room *roomPtr;
  const objectType *obj, *objCont;
  char strn[256];
  bool found;
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (checkExitKeyLoaded(roomPtr->exits[i]))
        {
          sprintf(strn, "%s exit of room #%u requires key #%u, but key isn't loaded\n",
                  g_exitnames[i], roomNumb, roomPtr->exits[i]->keyNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }
    }
  }

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      if ((obj->objType == ITEM_CONTAINER) && (obj->objValues[2] > 0) && !isObjTypeUsed(obj->objValues[2]))
      {
        sprintf(strn, "object #%u requires key #%u, but key isn't loaded\n",
                objNumb, obj->objValues[2]);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
    }
  }

 // check for extraneous keys

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      found = false;

      if (obj->objType == ITEM_KEY)
      {
        for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
        {
          if (roomExists(roomNumb))
          {
            roomPtr = findRoom(roomNumb);

            for (uint i = 0; i < NUMB_EXITS; i++)
            {
              if (roomPtr->exits[i] && (roomPtr->exits[i]->keyNumb == objNumb))
              {
                found = true;
                break;
              }
            }

            if (found) 
              break;
          }
        }

        if (!found)
        {
          for (uint objNumbCont = getLowestObjNumber(); objNumbCont <= highObjNumb; objNumbCont++)
          {
            if (objExists(objNumbCont))
            {
              objCont = findObj(objNumbCont);

              if ((objCont->objType == ITEM_CONTAINER) && (objCont->objValues[2] == objNumb))
              {
                found = true;

                break;
              }
            }
          }
        }

        if (!found)
        {
          sprintf(strn, "key #%u never used in this zone\n", objNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }
    }
  }

  return errors;
}


//
// checkLoaded : returns number of errors
//

uint checkLoaded(FILE *file, size_t& numbLines, bool* userQuit)
{
  const objectType *obj;
  const mobType *mob;
  char strn[256];
  uint errors = 0;
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestMobNumber();


  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      if (!isObjTypeUsed(objNumb))
      {
        sprintf(strn, "object #%u is never loaded/used\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
    }
  }

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mob = findMob(mobNumb);

      if (!getNumbEntities(ENTITY_MOB, mobNumb, false))
      {
        sprintf(strn, "mob #%u is never loaded\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
    }
  }

  return errors;
}


//
// checkExit - true if error
//

bool checkExit(const roomExit *exitNode)
{
  if (!exitNode) 
    return false;

 // exit with door type set and no keywords

  if ((exitNode->worldDoorType & 3) && (!exitNode->keywordListHead))
    return true;
  else
    return false;
}


//
// checkExits - check for door state/type w/ no keywords - returns number
//              of errors found
//

uint checkExits(FILE *file, size_t& numbLines, bool* userQuit)
{
  const room *roomPtr;
  char strn[256];
  uint errors = 0;
  const uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (checkExit(roomPtr->exits[i]))
        {
          sprintf(strn, "%s exit of room #%u has door type but no keywords\n",
                  g_exitnames[i], roomNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }
    }
  }

  return errors;
}


//
// checkExitDesc - true if no exit desc
//

bool checkExitDesc(const roomExit *exitNode)
{
  if (!exitNode) 
    return false;

 // exit with no exit desc

  return (exitNode->exitDescHead == NULL);
}


//
// checkExitDescs - check for exit w/ no desc - returns number of errors
//

uint checkExitDescs(FILE *file, size_t& numbLines, bool* userQuit)
{
  const room *roomPtr;
  char strn[256];
  uint errors = 0;
  const uint highRoomNumb = getHighestRoomNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (checkExitDesc(roomPtr->exits[i]))
        {
          sprintf(strn, "%s exit of room #%u has no exit desc\n",
                  g_exitnames[i], roomNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }
    }
  }

  return errors;
}


//
// checkObjects
//

uint checkObjects(FILE *file, size_t& numbLines, bool* userQuit)
{
  uint errors = 0, high, i;
  const objectType *obj;
  const objectHere *objHere = NULL;
  const extraDesc *edesc;
  const room *room;
  uint objFlags;
  uint objExtra;
  char strn[256];
  uint highObjNumb = getHighestObjNumber();

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

     // check for keyword existence

      if (!obj->keywordListHead)
      {
        sprintf(strn, "object #%u has no keywords\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
      else
      if (scanKeyword("_ignore_", obj->keywordListHead))
      {
        sprintf(strn, "object #%u uses keyword _IGNORE_ (use ignore flag in extra)\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for short and long names

      if (!obj->objShortName[0])
      {
        sprintf(strn, "object #%u has no short name\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

      if (!obj->objLongName[0])
      {
        sprintf(strn, "object #%u has no long name\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for no _id_ keyword with ident extra descs

      if ((getEdescinList(obj->extraDescHead, "_ID_NAME_") || getEdescinList(obj->extraDescHead, "_ID_SHORT_") ||
           getEdescinList(obj->extraDescHead, "_ID_DESC_")) && !scanKeyword("_ID_", obj->keywordListHead))
      {
        sprintf(strn, "object #%u has 'ident' extra descs but no _ID_ keyword\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for _id_ keyword without ident extra descs

      if (scanKeyword("_ID_", obj->keywordListHead) && !getEdescinList(obj->extraDescHead, "_ID_NAME_") &&
          !getEdescinList(obj->extraDescHead, "_ID_SHORT_") && !getEdescinList(obj->extraDescHead, "_ID_DESC_"))
      {
        sprintf(strn, "object #%u has _ID_ keyword but no 'ident' extra descs\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for ident extra descs with wrong amount of text in em

      if (edesc = getEdescinList(obj->extraDescHead, "_ID_NAME_"))
      {
        if ((i = getNumbStringNodes(edesc->extraDescStrnHead)) != 1)
        {
          sprintf(strn,
  "object #%u's 'ident' extra desc '_ID_NAME_' has the wrong number of lines of info - %u instead of one\n",
                  objNumb, i);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }

      if (edesc = getEdescinList(obj->extraDescHead, "_ID_SHORT_"))
      {
        if ((i = getNumbStringNodes(edesc->extraDescStrnHead)) != 1)
        {
          sprintf(strn,
  "object #%u's 'ident' extra desc '_ID_SHORT_' has the wrong number of lines of info - %u instead of one\n",
                  objNumb, i);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }

      if (edesc = getEdescinList(obj->extraDescHead, "_ID_DESC_"))
      {
        if ((i = getNumbStringNodes(edesc->extraDescStrnHead)) != 1)
        {
          sprintf(strn,
  "object #%u's 'ident' extra desc '_ID_DESC_' has the wrong number of lines of info - %u instead of one\n",
                  objNumb, i);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }

     // check object type for validity

      if ((obj->objType < ITEM_LOWEST) || (obj->objType > ITEM_LAST))
      {
        sprintf(strn,
  "object #%u's type is out of range\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check object size

      if (/*(obj->size < OBJSIZE_LOWEST) || */(obj->size > OBJSIZE_HIGHEST))
      {
        sprintf(strn, "object #%u's size is out of range\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check object craftsmanship

      if (/*(obj->craftsmanship < OBJCRAFT_LOWEST) ||*/
          (obj->craftsmanship > OBJCRAFT_HIGHEST))
      {
        sprintf(strn, "object #%u's craftsmanship is out of range\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check object condition percentage

      if ((obj->condition == 0) || (obj->condition > 100))
      {
        sprintf(strn, "object #%u's condition is not between 1-100%%\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // all takeable objects should be worth something

      if (!obj->worth && (obj->wearBits & ITEM_TAKE))
      {
        sprintf(strn, "object #%u is takeable with a worth of 0\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check object applies

      for (i = 0; i < NUMB_OBJ_APPLIES; i++)
      {
        if (!obj->objApply[i].applyWhere && obj->objApply[i].applyModifier)
        {
          sprintf(strn, "object #%u's apply #%u has 'where' of NOWHERE but non-zero modifier\n",
                  objNumb, i + 1);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }

        if ((obj->objApply[i].applyWhere == APPLY_CURSE) && (obj->objApply[i].applyModifier <= 0))
        {
          sprintf(strn, "object #%u's apply #%u's APPLY_CURSE modifier must be greater than 0\n",
                  objNumb, i + 1);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }

        if (obj->objApply[i].applyWhere && !obj->objApply[i].applyModifier &&
            (obj->objApply[i].applyWhere != APPLY_FIRE_PROT))
        {
          sprintf(strn, "object #%u's apply #%u has valid 'where' location, but modifier is zero\n",
                  objNumb, i + 1);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }

        if (obj->objApply[i].applyWhere > APPLY_LAST)
        {
          sprintf(strn, "object #%u's apply #%u has invalid 'where' location (too high)\n",
                  objNumb, i + 1);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }

     // check trap stuff if object has one

      if (obj->trapBits)
      {
        if ((obj->trapDam < TRAP_DAM_LOWEST) || (obj->trapDam > TRAP_DAM_HIGHEST))
        {
          sprintf(strn, "object #%u has a trap, but trap damage value is out of range\n",
                  objNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }

        if ((obj->trapLevel == 0) || (obj->trapLevel > 100))
        {
          sprintf(strn, "object #%u has a trap, but trap level is out of range (1-100)\n",
                  objNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }

        if (obj->trapCharge == 0)
        {
          sprintf(strn, "object #%u has a trap, but no charges\n",
                  objNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }

     // check for objects that are wearable but not takeable

      objFlags = obj->wearBits;
      objExtra = obj->extraBits;

      if ((objFlags & ~(ITEM_TAKE | ITEM_THROW)) && (~objFlags & ITEM_TAKE))
      {
        sprintf(strn, "object #%u is wearable, but not takeable\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // if none of the "usable" flags in the first/second anti bitvector are
     // set and this is armor, worn, weapon, or some other usable item, 
     // the object is not usable by anyone

      const bool usableItem = 
        ((obj->objType == ITEM_SCROLL) || (obj->objType == ITEM_WAND) || (obj->objType == ITEM_STAFF) || 
         (obj->objType == ITEM_WEAPON) || (obj->objType == ITEM_FIREWEAPON) || (obj->objType == ITEM_ARMOR) || 
         (obj->objType == ITEM_WORN) || (obj->objType == ITEM_POTION) || (obj->objType == ITEM_TELEPORT) || 
         (obj->objType == ITEM_PICK) || (obj->objType == ITEM_INSTRUMENT) || (obj->objType == ITEM_TOTEM) || 
         (obj->objType == ITEM_SHIELD) || (obj->objType == ITEM_SCABBARD) || (obj->objType == ITEM_QUIVER) ||
         (obj->objType == ITEM_SPELLBOOK) || (obj->objType == ITEM_MISSILE) || (obj->objType == ITEM_FOOD));

     // not all 32 anti-class bits are valid, so check if all valid flags are set

      bool allClassFlagsSet = true;

      for (int i = 0; g_npc_class_bits[i].flagShort; i++)
      {
        if (!(obj->antiBits & (1 << i)))
        {
          allClassFlagsSet = false;
          break;
        }
      }

      const bool antiClassMeansAllowed = ((objExtra & ITEM_ALLOWED_CLASSES) != 0);
      const bool antiRaceMeansAllowed = ((objExtra & ITEM_ALLOWED_RACES) != 0);

      if (usableItem && 
          (((antiClassMeansAllowed && (obj->antiBits == 0)) ||
            (antiRaceMeansAllowed && (obj->anti2Bits == 0))) ||
            (!antiClassMeansAllowed && allClassFlagsSet) || 
            (!antiRaceMeansAllowed && (obj->anti2Bits == 0xffffffff))))
      {
        sprintf(strn,
  "object #%u is usable but has no 'use' flags set in anti/anti2\n",
                  objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check if guild zone is proper - mostly useless anymore

      if (getCheckGuildStuffVal())
      {
        if (obj->extraBits & ITEM_NOSHOW)
        {
          sprintf(strn, "object #%u in guild is NOSHOW\n",
                  objNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }

        if (obj->objType == ITEM_TELEPORT)
        {
          if ((obj->objValues[1] != MUDCOMM_ENTER) && (obj->objValues[1] != MUDCOMM_TOUCH))
          {
            sprintf(strn,
  "object #%u is a guild teleporter with command other than 'enter' or 'touch'\n",
                    objNumb);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }
          }
        }
      }
    }
  }

 // now, check for rooms that are water rooms and yet have non-floating
 // objects in them

 // let's check for objects in no_ground rooms while we're at it

 // let's check for transient objects on the ground too

  high = getHighestRoomNumber();

  for (i = getLowestRoomNumber(); i <= high; i++)
  {
    if (roomExists(i))
    {
      room = findRoom(i);

      objHere = room->objectHead;
      while (objHere)
      {
        if (objHere->objectPtr && (objHere->objectPtr->extraBits & ITEM_TRANSIENT))
        {
          sprintf(strn,
"object #%u is transient and loads on ground in room #%u - will decay on load\n",
                  objHere->objectNumb, i);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }

        objHere = objHere->Next;
      }

      if ((room->sectorType == SECT_NO_GROUND) || (room->sectorType == SECT_UNDRWLD_NOGROUND))
      {
        objHere = room->objectHead;
        while (objHere)
        {
          if (!objHere->objectPtr ||
              (objHere->objectPtr && !(objHere->objectPtr->extraBits & ITEM_LEVITATES)))
          {
            sprintf(strn,
"object #%u is non-levitating and loads in a room with no ground (#%u)\n",
                    objHere->objectNumb, i);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }
          }

          objHere = objHere->Next;
        }
      }


      if ((room->sectorType == SECT_WATER_SWIM) || (room->sectorType == SECT_WATER_NOSWIM) ||
          (room->sectorType == SECT_UNDERWATER) || (room->sectorType == SECT_OCEAN) ||
          (room->sectorType == SECT_UNDRWLD_WATER) || (room->sectorType == SECT_UNDRWLD_NOSWIM) ||
          (room->sectorType == SECT_PLANE_OF_WATER))
      {
        objHere = room->objectHead;
        while (objHere)
        {
          if (objHere->objectPtr && !(objHere->objectPtr->extraBits & ITEM_FLOAT))
          {
            sprintf(strn,
"object #%u is non-floating but loads in a water room (#%u)\n",
                objHere->objectNumb, i);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }
          }

          objHere = objHere->Next;
        }
      }
    }
  }

  return errors;
}


//
// checkObjectValueRange : generic function to check value based on various
//                         parameters - returns number of errors
//

uint checkObjectValueRange(const objectType *obj, const uchar valNumb, const int lowRange, 
                           const int highRange, const int exclude, FILE *file, size_t& numbLines,
                           bool *userQuit)
{
  uint errors = 0;
  int val;
  char strn[256], valstrn2[256];
  const char *valstrn;


  val = obj->objValues[valNumb];

  if ((val == exclude) && (exclude != NO_EXCLUSION)) 
    return 0;

  valstrn = getObjValueStrn(obj->objType, valNumb, val, valstrn2, false);

 // if lowRange == highRange, this value numb can't == val stored in
 // low/highRange

  if ((lowRange == highRange) && (lowRange != NO_EXCLUSION) && (lowRange != NO_LIMIT))
  {
    if (val == lowRange)
    {
      sprintf(strn,
"object #%u's val #%u (%s) is invalid (value is %d)\n",
                obj->objNumber, valNumb + 1, valstrn, lowRange);
      errors++;

      if (outCheckError(strn, file, numbLines))
      {
        *userQuit = true;
        return errors;
      }
    }

    return errors;
  }

 // another special case

  if (highRange == MUST_BE_LOWEST)
  {
    if (val != lowRange)
    {
      sprintf(strn,
"object #%u's val #%u (%s) is not equal to %d (required)\n",
                obj->objNumber, valNumb + 1, valstrn, lowRange);
      errors++;

      if (outCheckError(strn, file, numbLines))
      {
        *userQuit = true;
        return errors;
      }
    }

    return errors;
  }

  if ((lowRange != NO_LIMIT) && (val < lowRange))
  {
    sprintf(strn,
"object #%u's val #%u (%s) is too low (below %d)\n",
            obj->objNumber, valNumb + 1, valstrn, lowRange);
    errors++;

    if (outCheckError(strn, file, numbLines))
    {
      *userQuit = true;
      return errors;
    }
  }

  if ((highRange != NO_LIMIT) && (val > highRange))
  {
    sprintf(strn,
"object #%u's val #%u (%s) is too high (above %d)\n",
            obj->objNumber, valNumb + 1, valstrn, highRange);
    errors++;

    if (outCheckError(strn, file, numbLines))
    {
      *userQuit = true;
      return errors;
    }
  }

  return errors;
}


//
// checkObjValValidLevel
//

uint checkObjValValidLevel(const objectType *obj, const char valNumb, FILE *file, size_t& numbLines, 
                           bool *userQuit)
{
  return checkObjectValueRange(obj, valNumb, 1, 60, NO_EXCLUSION, file, numbLines, userQuit);
}


//
// checkObjValValidSpellType
//

uint checkObjValValidSpellType(const objectType *obj, const char valNumb, FILE *file, size_t& numbLines,
                               bool *userQuit)
{
  return checkObjectValueRange(obj, valNumb, 1, LAST_SPELL - 2, -1, file, numbLines, userQuit);
}


//
// checkObjectValues : returns number of errors
//

uint checkObjectValues(FILE *file, size_t& numbLines, bool *userQuit)
{
  uint errors = 0;
  const objectType *obj;
  char strn[256];
  const uint highObjNumb = getHighestObjNumber();

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      const uint otype = obj->objType;

      switch (otype)
      {
        case ITEM_LIGHT      :
         // lights should have at least an hour of light (or -1 for eternal)

          errors += checkObjectValueRange(obj, 2, 1, NO_LIMIT, -1, file, numbLines, userQuit);
          if (*userQuit)
            return errors;
          break;

        case ITEM_SCROLL     :
        case ITEM_WAND       :
        case ITEM_STAFF      :
        case ITEM_POTION     :

         // check spell level range (common to all 4 types)

          errors += checkObjValValidLevel(obj, 0, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // further break em down..

          switch (otype)
          {
           // check spell type range for scrolls and potions

            case ITEM_SCROLL :
            case ITEM_POTION :
              errors += checkObjValValidSpellType(obj, 1, file, numbLines, userQuit);
              if (*userQuit)
                return errors;

              errors += checkObjValValidSpellType(obj, 2, file, numbLines, userQuit);
              if (*userQuit)
                return errors;

              errors += checkObjValValidSpellType(obj, 3, file, numbLines, userQuit);
              if (*userQuit)
                return errors;

              break;

           // check max charges (has to be at least 1), current charges
           // (has to be at least 0), and spell type for wands and staves

            case ITEM_WAND :
            case ITEM_STAFF :
              errors += checkObjectValueRange(obj, 1, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
              if (*userQuit)
                return errors;

              errors += checkObjectValueRange(obj, 2, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
              if (*userQuit)
                return errors;

              errors += checkObjValValidSpellType(obj, 3, file, numbLines, userQuit);
              if (*userQuit)
                return errors;

              break;
          }

          break;

        case ITEM_WEAPON     :
         // weapon type should be within range

          errors += checkObjectValueRange(obj, 0, WEAPON_LOWEST, WEAPON_HIGHEST, NO_EXCLUSION, file, 
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

         // check damage dice and damage type

          errors += checkObjectValueRange(obj, 1, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 2, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

/*          errors += checkObjectValueRange(obj, 3, WEAPON_DAM_LOWEST, WEAPON_DAM_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

         // a few values in between are considered invalid..

          errors += checkObjectValueRange(obj, 3, 4, 4, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 5, 5, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 8, 8, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 9, 9, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 10, 10, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;*/

          break;

        case ITEM_FIREWEAPON :
         // range must be at least 1 (or maybe not..  shrug, oh well)

          errors += checkObjectValueRange(obj, 0, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // rate of fire should be 0 or higher (maybe 1 or higher..)

          errors += checkObjectValueRange(obj, 1, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // check for valid missile type range

          errors += checkObjectValueRange(obj, 3, MISSILE_LOWEST, MISSILE_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_MISSILE    :
         // check damage dice and missile type

          errors += checkObjectValueRange(obj, 1, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 2, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, MISSILE_LOWEST, MISSILE_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_ARMOR      :

          errors += checkObjectValueRange(obj, 0, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // armor thickness should be within bounds

          errors += checkObjectValueRange(obj, 3, ARMOR_THICKNESS_LOWEST, ARMOR_THICKNESS_HIGHEST, 
                                          NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // misc flags should be at least 0

          errors += checkObjectValueRange(obj, 4, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_CONTAINER  :
         // weight container can hold should be at least 1

          errors += checkObjectValueRange(obj, 0, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // container flags should be at least 0

          errors += checkObjectValueRange(obj, 1, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // check for closed container that isn't closeable

          if ((obj->objValues[1] & 4) && (!(obj->objValues[1] & 1)))
          {
            sprintf(strn,
    "object #%u is closed container, but not closeable (val #2)\n",
                    objNumb);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }
          }

          break;

        case ITEM_DRINKCON   :
         // should be at least 0 current and 1 max drink units and a
         // valid liquid type, poison value should be at least 0 (there is
         // a high value for poisons, too..  dunno what it is)

          errors += checkObjectValueRange(obj, 0, 1, NO_LIMIT, -1, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 1, 0, NO_LIMIT, -1, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 2, LIQ_LOWEST, LIQ_HIGHEST, NO_EXCLUSION, file, numbLines, 
                                          userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_KEY        :
         // percentage of breaking should be between 0-100%

          errors += checkObjectValueRange(obj, 1, 0, 100, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_FOOD       :
         // should fill for at least 1 hour, poison value should be at least 0
         // (there is a high value on poisons as well..  shrug)

          errors += checkObjectValueRange(obj, 0, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_MONEY      :
         // money item should have at least 1 of something and no negative
         // values

          errors += checkObjectValueRange(obj, 0, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 1, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 2, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          if (!obj->objValues[0] && !obj->objValues[1] && !obj->objValues[2] && !obj->objValues[3])
          {
            sprintf(strn,
    "object #%u is money object with no actual money\n",
                    objNumb);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }
          }

          break;

        case ITEM_TELEPORT   :
         // target room should be at least 0, command should be valid, number
         // of charges should be >0 or -1

          errors += checkObjectValueRange(obj, 0, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 1, MUDCOMM_LOWEST, MUDCOMM_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 2, 1, NO_LIMIT, -1, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_SWITCH     :
         // command should be valid, room # should be at least 0, direction
         // of exit should be valid, val 3 should be 0 or 1

          errors += checkObjectValueRange(obj, 0, MUDCOMM_LOWEST, MUDCOMM_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 1, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 2, EXIT_LOWEST, EXIT_HIGHEST, NO_EXCLUSION, file, numbLines, 
                                          userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 0, 1, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_QUIVER     :
         // quivers should have a capacity of at least one, no closed and
         // not closeable container flag, missile type should be valid,
         // current amount of missiles should be at least 0

          errors += checkObjectValueRange(obj, 0, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // quiver flags should be at least 0

          errors += checkObjectValueRange(obj, 1, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          if ((obj->objValues[1] & 4) && (!(obj->objValues[1] & 1)))
          {
            sprintf(strn,
    "object #%u is closed quiver, but not closeable (val #2)\n",
                    objNumb);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }
          }

          errors += checkObjectValueRange(obj, 2, MISSILE_LOWEST, MISSILE_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 3, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_PICK       :
         // percentages should be between 0-100%

          errors += checkObjectValueRange(obj, 0, 0, 100, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 1, 0, 100, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_INSTRUMENT :
         // instrument type should be valid, level should be valid, break
         // chance should be at least 0 and no more than 1000, min level to
         // use must be valid

         // flute is lowest, horn is highest, this may be as good as it gets

          errors += checkObjectValueRange(obj, 0, INSTRUMENT_FLUTE, INSTRUMENT_HORN, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjValValidLevel(obj, 1, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 2, 0, 1000, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjValValidLevel(obj, 3, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_SPELLBOOK  :
         // number of pages must be at least 1

          errors += checkObjectValueRange(obj, 2, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_TOTEM      :
         // spheres value must be at least 1

          errors += checkObjectValueRange(obj, 0, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;

        case ITEM_SHIELD     :
         // shield type, shape, size should be within bounds

          errors += checkObjectValueRange(obj, 0, SHIELDTYPE_LOWEST, SHIELDTYPE_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 1, SHIELDSHAPE_LOWEST, SHIELDSHAPE_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          errors += checkObjectValueRange(obj, 2, SHIELDSIZE_LOWEST, SHIELDSIZE_HIGHEST, NO_EXCLUSION, file,
                                          numbLines, userQuit);
          if (*userQuit)
            return errors;

          // val3 is ac, so should be at least 1
          errors += checkObjectValueRange(obj, 3, 1, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // shield thickness should be within bounds

          errors += checkObjectValueRange(obj, 4, ARMOR_THICKNESS_LOWEST, ARMOR_THICKNESS_HIGHEST, 
                                          NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

         // shield misc flags should be at least 0

          errors += checkObjectValueRange(obj, 5, 0, NO_LIMIT, NO_EXCLUSION, file, numbLines, userQuit);
          if (*userQuit)
            return errors;

          break;
      }
    }
  }

  return errors;
}


//
// checkObjMaterial : returns number of errors
//

uint checkObjMaterial(FILE *file, size_t& numbLines, bool* userQuit)
{
  uint errors = 0;
  const objectType *obj;
  char strn[256];
  const uint highObjNumb = getHighestObjNumber();

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      if ((obj->material > MAT_HIGHEST) || (obj->material < MAT_LOWEST))
      {
        sprintf(strn, "object #%u has invalid material\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
    }
  }

  return errors;
}


//
// checkMobs : returns number of errors
//

uint checkMobs(FILE *file, size_t& numbLines, bool* userQuit)
{
  uint errors = 0, i;
  const mobType *mob;
  const questMessage *qstm;
  const questQuest *qstq;
  char strn[2560], strn2[2048];
  const uint highMobNumb = getHighestMobNumber();


  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mob = findMob(mobNumb);

      if (!mob->mobShortName[0])
      {
        sprintf(strn, "mob #%u has no short name\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

      if (!mob->keywordListHead)
      {
        sprintf(strn, "mob #%u has no keywords\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check for '_IGNORE_' keyword (should no longer be used)

      else
      if (scanKeyword("_IGNORE_", mob->keywordListHead))
      {
        sprintf(strn, "mob #%u uses _IGNORE_ keyword (use ignore flag in act flags)\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check mob level

      if (!(mob->actionBits & ACT_IGNORE) && ((mob->level < 1) || (mob->level > 60)))
      {
        sprintf(strn, "mob #%u's level is out of range (1-60)\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check mob alignment

      if ((mob->alignment < -1000) || (mob->alignment > 1000))
      {
        sprintf(strn, "mob #%u's alignment is out of range\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check mob species

      if (!strcmp(getMobSpeciesStrn(mob->mobSpecies), "unrecog. species"))
      {
        sprintf(strn, "mob #%u's species is unrecognized\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check mob hometown

      if ((mob->mobHometown < HOME_LOWEST) || (mob->mobHometown > HOME_HIGHEST))
      {
        sprintf(strn, "mob #%u's hometown is out of range\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check mob size

      if ((mob->size != MOB_SIZE_DEFAULT) &&
          ((mob->size < MOB_SIZE_LOWEST) || (mob->size > MOB_SIZE_HIGHEST)))
      {
        sprintf(strn, "mob #%u's size is out of range\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check mob position and default pos

      if ((mob->position < POSITION_LOWEST_LEGAL) || (mob->position > POSITION_HIGHEST))
      {
        sprintf(strn, "mob #%u's position is out of range\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

      if ((mob->defaultPos < POSITION_LOWEST_LEGAL) || (mob->defaultPos > POSITION_HIGHEST))
      {
        sprintf(strn, "mob #%u's default pos is out of range\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check mob's sex

      if ((mob->sex < SEX_LOWEST) || (mob->sex > SEX_HIGHEST))
      {
        sprintf(strn, "mob #%u's sex is out of range\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check if mob is set aggro, but has a non-standing pos

      if (((mob->defaultPos < POSITION_STANDING) || (mob->position < POSITION_STANDING)) && isAggro(mob))
      {
        sprintf(strn, "mob #%u is aggro, but has a position/default position less than standing (non-standing "
                      "mobs don't attack anyone)\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check class as well..  a non-standing caster is gonna be pretty ill-prepared

      if (((mob->defaultPos < POSITION_STANDING) || (mob->position < POSITION_STANDING)) &&
           castingClass(mob->mobClass))
      {
        sprintf(strn, "mob #%u is a caster, but has a position/default position less than standing "
                      "(non-standing mobs can't spell up)\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }

     // check spec's, no specs if multiclass or class none
      if (!countClass(mob->mobClass))
      {
	sprintf(strn, "mob #%u is class None with a specialization set.\r\n", mobNumb);
	errors++;

	if (outCheckError(strn, file, numbLines))
	{
	  *userQuit = true;
	  return errors;
	}
      }
      else if (countClass(mob->mobClass) > 1)
      {
	sprintf(strn, "mob #%u has more than one class with specialization set.\r\n", mobNumb);
	errors++;
	
	if (outCheckError(strn, file, numbLines))
	{
	  *userQuit = true;
	  return errors;
	}
      }

     // check mob quest info

      if (mob->questPtr)
      {
        if (!mob->questPtr->messageHead && !mob->questPtr->questHead)
        {
          sprintf(strn,
    "mob #%u has a quest, but no messages or actual quests\n",
                  mobNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
        else
        {
          qstm = mob->questPtr->messageHead;
          while (qstm)
          {
            if (!qstm->keywordListHead)
            {
              sprintf(strn, "mob #%u has a quest message with no keywords\n",
                      mobNumb);
              errors++;

              if (outCheckError(strn, file, numbLines))
              {
                *userQuit = true;
                return errors;
              }
            }

            if (!qstm->questMessageHead)
            {
              sprintf(strn, "mob #%u has a quest message (%s) with no actual message text\n",
                      mobNumb,
                      qstm->keywordListHead ?
                        getReadableKeywordStrn(qstm->keywordListHead, strn2, 2047) :
                        "no keywords");
              errors++;

              if (outCheckError(strn, file, numbLines))
              {
                *userQuit = true;
                return errors;
              }
            }

            qstm = qstm->Next;
          }

          qstq = mob->questPtr->questHead;
          while (qstq)
          {
            if (!qstq->questReplyHead)
            {
              sprintf(strn, "mob #%u has quest info with no response when finishing\n",
                      mobNumb);
              errors++;

              if (outCheckError(strn, file, numbLines))
              {
                *userQuit = true;
                return errors;
              }
            }

           // all quests require the mob to get something from the players..
           // but the players won't always get something from the mob

            if (!qstq->questPlayGiveHead)
            {
              sprintf(strn,
  "mob #%u has quest info that requires nothing from players (quest is impossible to complete)\n",
                      mobNumb);
              errors++;

              if (outCheckError(strn, file, numbLines))
              {
                *userQuit = true;
                return errors;
              }
            }

            qstq = qstq->Next;
          }
        }
      }

     // check mob shop info

      if (mob->shopPtr)
      {
        if (!mob->shopPtr->producedItemList[0] && !mob->shopPtr->tradedItemList[0])
        {
          sprintf(strn, "mob #%u has a shop, but doesn't buy or sell anything\n",
                  mobNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
        else
        {
          for (i = 0; (i < MAX_NUMBSHOPITEMTYPES) && mob->shopPtr->tradedItemList[i]; i++)
          {
            if ((mob->shopPtr->tradedItemList[i] < ITEM_LOWEST) || 
                (mob->shopPtr->tradedItemList[i] > ITEM_LAST))
            {
              sprintf(strn, "mob #%u has a shop with one or more invalid item types in bought list\n",
                      mobNumb);
              errors++;

              if (outCheckError(strn, file, numbLines))
              {
                *userQuit = true;
                return errors;
              }

              break;  // no need to spam ceaselessly
            }
          }
        }

       // check opening/closing times

        if ((mob->shopPtr->firstClose <= mob->shopPtr->firstOpen) || 
            (mob->shopPtr->secondClose <= mob->shopPtr->secondOpen))
        {
          sprintf(strn, "mob #%u's shop has invalid opening/closing time - closing must be after opening\n",
                  mobNumb);

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }

     // check guild stuff

      if (getCheckGuildStuffVal())
      {
        if (isAggro(mob) || (mob->actionBits & ACT_HUNTER))
        {
          sprintf(strn, "mob #%u is a guild mob with an aggro/hunter flag set\n",
                  mobNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }

        if (mob->alignment != 0)
        {
          sprintf(strn, "mob #%u is a guild mob with non-neutral alignment\n",
                  mobNumb);
          errors++;

          if (outCheckError(strn, file, numbLines))
          {
            *userQuit = true;
            return errors;
          }
        }
      }
    }
  }


  return errors;
}


//
// checkObjectDescs : returns number of errors
//

uint checkObjectDescs(FILE *file, size_t& numbLines, bool *userQuit)
{
  uint errors = 0;
  const objectType *obj;
  char strn[256];
  const uint highObjNumb = getHighestObjNumber();


  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      if (!obj->extraDescHead)
      {
        sprintf(strn, "object #%u has no extra descs\n",
                objNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
    }
  }

  return errors;
}


//
// checkMobDescs : returns number of errors
//

uint checkMobDescs(FILE *file, size_t& numbLines, bool* userQuit)
{
  uint errors = 0;
  const mobType *mob;
  char strn[256];
  const uint highMobNumb = getHighestMobNumber();


  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mob = findMob(mobNumb);

      if (!mob->mobDescHead)
      {
        sprintf(strn, "mob #%u has no description\n",
                mobNumb);
        errors++;

        if (outCheckError(strn, file, numbLines))
        {
          *userQuit = true;
          return errors;
        }
      }
    }
  }

  return errors;
}


//
// checkEdesc : returns code based on error
//

char checkEdesc(const extraDesc *descNode)
{
  if (!descNode) 
    return EDESC_NO_ERROR;

  if (!descNode->keywordListHead) 
    return EDESC_NO_KEYWORDS;

  if (!descNode->extraDescStrnHead) 
    return EDESC_NO_DESC;

  return EDESC_NO_ERROR;
}


//
// checkAllEdescs : returns number of errors
//

uint checkAllEdescs(FILE *file, size_t& numbLines, bool *userQuit)
{
  uint errors = 0;
  const room *roomPtr;
  const objectType *obj;
  const extraDesc *descNode;
  char strn[256];
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();


  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      descNode = roomPtr->extraDescHead;
      while (descNode)
      {
        switch (checkEdesc(descNode))
        {
          case EDESC_NO_KEYWORDS :
            sprintf(strn, "room #%u has an extra desc with no keywords\n", 
                    roomNumb);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }

            break;

          case EDESC_NO_DESC :
            sprintf(strn, "room #%u has an extra desc with no description\n",
                    roomNumb);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }

            break;
        }

        descNode = descNode->Next;
      }
    }
  }

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      descNode = obj->extraDescHead;
      while (descNode)
      {
        switch (checkEdesc(descNode))
        {
          case EDESC_NO_KEYWORDS :
            sprintf(strn, "object #%u has an extra desc with no keywords\n", 
                    objNumb);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }

            break;

          case EDESC_NO_DESC :
            sprintf(strn, "object #%u has an extra desc with no description\n",
                    objNumb);
            errors++;

            if (outCheckError(strn, file, numbLines))
            {
              *userQuit = true;
              return errors;
            }

            break;
        }

        descNode = descNode->Next;
      }
    }
  }

  return errors;
}


//
// checkAll : returns true if any errors found
//

bool checkAll(void)
{
  uint errors = 0;
  size_t numbLines = 1;
  bool userQuit = false;
  char strn[256];
  FILE *outFile = NULL;


  if (getSaveCheckLogVal()) 
    outFile = fopen("check.log", "wt");

  _outtext("\n");

  if (getCheckExitDescVal()) 
    errors += checkExitDescs(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckObjDescVal()) 
    errors += checkObjectDescs(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckMobDescVal()) 
    errors += checkMobDescs(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckEdescVal()) 
    errors += checkAllEdescs(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckFlagsVal()) 
    errors += checkAllFlags(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckLoneRoomVal()) 
    errors += checkLoneRooms(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckRoomVal()) 
    errors += checkRooms(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckMissingKeysVal()) 
    errors += checkMissingKeys(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckLoadedVal()) 
    errors += checkLoaded(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckExitVal()) 
    errors += checkExits(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckObjVal()) 
    errors += checkObjects(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckObjValuesVal()) 
    errors += checkObjectValues(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckObjMaterialVal()) 
    errors += checkObjMaterial(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckMobVal()) 
    errors += checkMobs(outFile, numbLines, &userQuit);

  if (!userQuit && getCheckZoneVal()) 
    errors += checkZone(outFile, numbLines, &userQuit);

  if (userQuit)
  {
    outCheckError("\nUser aborted.\n\n", outFile, numbLines);

    if (outFile) 
      fclose(outFile);

    if (errors)
      return true;
    else
      return false;
  }
  else
  if (errors)
  {
    if (errors == 1) 
      strcpy(strn, "\nOne error found while checking entities.\n\n");
    else 
      sprintf(strn, "\n%u errors found while checking entities.\n\n", errors);

    outCheckError(strn, outFile, numbLines);

    if (outFile) 
      fclose(outFile);

    return true;
  }
  
  _outtext("No errors found in entities.\n\n");

  if (outFile) 
    fclose(outFile);

  return false;
}
