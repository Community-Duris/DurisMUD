//
//  File: readzon.cpp    originally part of durisEdit
//
//  Usage: functions for reading zone info from the .zon file
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

#include "types.h"
#include "fh.h"

#include "zone.h"
#include "readzon.h"
#include "misc/loaded.h"
#include "keys.h"
#include "vardef.h"
#include "defines.h"

#include "readfile.h"



extern bool g_madeChanges;
extern variable *g_varHead;
extern zone g_zoneRec;
extern uint g_numbObjs, g_numbMobs;
extern bool g_readFromSubdirs;


//
// addLastObjLoaded : Adds a lastObjHereLoaded node to a list of em.
//
//    **lastHead : pointer to pointer to head of list
//
//       objNumb : obj type vnum being added
//       *objPtr : pointer to objectHere
//

void addLastObjLoaded(lastObjHereLoaded **lastHead, const uint objNumb, objectHere *objPtr)
{
  lastObjHereLoaded *lastNode = *lastHead, *newNode, *prevNode;


  while (lastNode)
  {
    if ((lastNode->objNumb) == objNumb)
    {
      lastNode->objHerePtr = objPtr;

      return;
    }

    prevNode = lastNode;
    lastNode = lastNode->Next;
  }

  newNode = new(std::nothrow) lastObjHereLoaded;
  if (!newNode)
  {
    displayAllocError("lastObjHereLoaded", "addLastObjLoaded");

    exit(1);
  }

  newNode->objHerePtr = objPtr;
  newNode->objNumb = objNumb;
  newNode->Next = NULL;

  if (!*lastHead) 
    *lastHead = newNode;
  else 
    prevNode->Next = newNode;
}


//
// getLastObjLoaded : Runs through a list of lastObjHereLoadeds, returning the
//                    address of the first match's objHere node
//
//     *lastHead : head of list
//       objNumb : object number being looked for
//

objectHere *getLastObjLoaded(lastObjHereLoaded *lastHead, const uint objNumb)
{
  while (lastHead)
  {
    if (lastHead->objNumb == objNumb) 
      return lastHead->objHerePtr;

    lastHead = lastHead->Next;
  }

  return NULL;
}


//
// deleteLastObjLoadedList : Runs through a list of lastObjHereLoadeds,
//                           deleting the bastard.
//
//     *lastHead : head of list
//

void deleteLastObjLoadedList(lastObjHereLoaded *lastHead)
{
  while (lastHead)
  {
    lastObjHereLoaded *nextNode = lastHead->Next;

    delete lastHead;

    lastHead = nextNode;
  }
}


//
// addLimitSpec : Adds a limitSpecified node to a list of em.
//
//    **specHead : pointer to pointer to head of list
//
//       entType : type of entity (misc/master.h)
//       entNumb : vnum of entity being added
//         limit : limit specified
//

void addLimitSpec(limitSpecified **specHead, const char entType, const uint entNumb, const uint limit)
{
  limitSpecified *specNode = *specHead, *newNode, *prevNode;


  while (specNode)
  {
    if ((specNode->entityType == entType) && (specNode->vnum == entNumb) && (specNode->limit < limit))
    {
      specNode->limit = limit;
      return;
    }

    prevNode = specNode;
    specNode = specNode->Next;
  }

  newNode = new(std::nothrow) limitSpecified;
  if (!newNode)
  {
    displayAllocError("limitSpecified", "addLimitSpec");

    exit(1);
  }

  newNode->entityType = entType;
  newNode->vnum = entNumb;
  newNode->limit = limit;
  newNode->Next = NULL;

  if (!*specHead) 
    *specHead = newNode;
  else 
    prevNode->Next = newNode;
}


//
// deleteLimitSpecList : Runs through a list of limitSpecifieds,
//                       deleting the bastard.
//
//     *limitHead : head of list
//

void deleteLimitSpecList(limitSpecified *limitHead)
{
  while (limitHead)
  {
    limitSpecified *nextNode = limitHead->Next;

    delete limitHead;

    limitHead = nextNode;
  }
}


//
// setOverrideFromLimSpec : Set override amount for all objects and mobs loaded with
//                          a limit higher than the actual amount in the zone
//

void setOverrideFromLimSpec(limitSpecified *specHead)
{
  const entityLoaded *numbLoadedNode;

  while (specHead)
  {
    numbLoadedNode = getNumbEntitiesNode(specHead->entityType, specHead->vnum);

    if ((specHead->limit > numbLoadedNode->numberLoaded) &&
        (numbLoadedNode->overrideLoaded < specHead->limit))
    {
      setEntityOverride(specHead->entityType, specHead->vnum, specHead->limit);
    }

    specHead = specHead->Next;
  }
}


//
// removeComments : Removes anything after and including an asterisk (for use
//                  with zone commands)
//
//   *strn : String to alter
//

void removeComments(char *strn)
{
  char *strptr = strn;

  while (*strptr)
  {
    if (*strptr == '*')
    {
      *strptr = '\0';
      break;
    }

    strptr++;
  }

 // next, remove all the spaces preceeding the asterisk

  remTrailingSpaces(strn);
}


//
// loadObj : Load (put, place, whatever) an object into a room
//
//   *strn : String as read from zone file
//

void loadObj(const char *strn, lastObjHereLoaded **lastObjLoadedHead, limitSpecified **limitSpecHead, 
             const uint linesRead)
{
  uint objNumb, maxExist, roomNumb, unused, ifFlag;
  int randVal;
  room *roomPtr;
  char tempstrn[512];


 // check command string

  if (numbArgs(strn) == 5)  // old format
  {
    sscanf(strn, "%s%u%u%u%u",
           tempstrn, &ifFlag, &objNumb, &maxExist, &roomNumb);

    randVal = 100;

    g_madeChanges = true;
  }
  else if (numbArgs(strn) == 9)  // new format
  {
    sscanf(strn, "%s%u%u" "%u%u%d" "%u%u%u",
           tempstrn, &ifFlag, &objNumb,
           &maxExist, &roomNumb, &randVal,
           &unused, &unused, &unused);
  }
  else  // error
  {
    char outstrn[512];

    sprintf(outstrn, "Error in obj load (line #%u) - wrong number of args (%u)\n", 
            linesRead, numbArgs(strn));

    _outtext(outstrn);

    exit(1);
  }

 // check random value

  if ((randVal > 100) || (randVal <= 0))
  {
    char outstrn[512];

    sprintf(outstrn, "Error in obj load command for object #%u (line #%u) -\n"
                     "random value is not 1-100%% (value read was %d) - setting to 100%%\n\n", 
            objNumb, linesRead, randVal);

    displayAnyKeyPrompt(outstrn);

    randVal = 100;
    g_madeChanges = true;
  }

 // check limit

  if ((getNumbEntities(ENTITY_OBJECT, objNumb, false) >= maxExist) && getCheckLimitsVal())
  {
    char outstrn[512];

    sprintf(outstrn, "Already %u or more instances of object #%u loaded, skipping (line #%u).\n", 
            maxExist, objNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

  roomPtr = findRoom(roomNumb);
  if (!roomPtr)
  {
    char outstrn[512];

    sprintf(outstrn,
"Warning: Load object ('O') command for object #%u failed because the\n"
"         target room #%u does not exist.\n\n", objNumb, roomNumb);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // allocate and init objhere node

  objectHere *objHereNode = new(std::nothrow) objectHere;
  if (!objHereNode)
  {
    displayAllocError("objectHere", "loadObj");

    exit(1);
  }

  memset(objHereNode, 0, sizeof(objectHere));

  objHereNode->randomChance = randVal;

 // set appropriate vars..

  objHereNode->objectNumb = objNumb;

 // add object to room

  addObjHeretoList(&(roomPtr->objectHead), objHereNode, false);

 // set object's object type pointer

  objHereNode->objectPtr = findObj(objNumb);

  if (!(objHereNode->objectPtr) && getVnumCheckVal())
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Object #%u specified in load object ('O') command not found.\n\n",
            objNumb);

    _outtext(outstrn);

    if (displayYesNoPrompt("Turn vnum checking off and continue", promptYes, false) == promptNo)
      exit(1);

    setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, false, true);
  }

 // add the object to the entityLoaded list

  addEntity(ENTITY_OBJECT, objNumb);

 // add limit specified to limit spec list

  addLimitSpec(limitSpecHead, ENTITY_OBJECT, objNumb, maxExist);

 // add the object to the lastObjLoaded list

  addLastObjLoaded(lastObjLoadedHead, objNumb, objHereNode);

  g_numbObjs++;
}


//
// loadMob : Load (place, put) a mob into a room
//
//      *strn : string as read from zone file
//  **lastMob : last mob loaded - used for equip and give commands
//

void loadMob(const char *strn, mobHere **lastMob, mobHere **lastMobEquippable, limitSpecified **limitSpecHead, 
             const uint linesRead)
{
  uint mobNumb, maxExist, roomNumb, unused, ifflag;
  int randVal;
  room *roomPtr;
  char tempstrn[512];


  if (numbArgs(strn) == 5)  // old format
  {
    sscanf(strn, "%s%u%u%u%u",
           tempstrn, &ifflag, &mobNumb, &maxExist, &roomNumb);

    randVal = 100;

    g_madeChanges = true;
  }
  else if (numbArgs(strn) == 9)  // new format
  {
    sscanf(strn, "%s%u%u" "%u%u%d" "%u%u%u",
           tempstrn, &ifflag, &mobNumb,
           &maxExist, &roomNumb, &randVal,
           &unused, &unused, &unused);
  }
  else
  {
    char outstrn[512];

    sprintf(outstrn, "Error in mob load (line #%u) - wrong number of args (%u)\n",
            linesRead, numbArgs(strn));

    _outtext(outstrn);

    exit(1);
  }

 // check random value

  if ((randVal > 100) || (randVal <= 0))
  {
    char outstrn[512];

    sprintf(outstrn, "Error in mob load command for mob #%u (line #%u) - random\n"
                     "value is not 1-100%% (value read was %d) - setting to 100%%\n",
            mobNumb, linesRead, randVal);

    displayAnyKeyPrompt(outstrn);

    randVal = 100;
    g_madeChanges = true;
  }

  if ((getNumbEntities(ENTITY_MOB, mobNumb, false) >= maxExist) && getCheckLimitsVal())
  {
    char outstrn[512];

    sprintf(outstrn, "Already %u or more instances of mob #%u loaded, skipping (line #%u).\n",
            maxExist, mobNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

  roomPtr = findRoom(roomNumb);

  if (!roomPtr)
  {
    char outstrn[512];

    sprintf(outstrn, 
"Warning: Load mob ('M') command for mob #%u failed because the target\n"
"         room #%u does not exist.\n\n",
            mobNumb, roomNumb);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

  mobHere *mobHereNode = new(std::nothrow) mobHere;
  if (!mobHereNode)
  {
    displayAllocError("mobHere", "loadMob");

    exit(1);
  }

  memset(mobHereNode, 0, sizeof(mobHere));

  mobHereNode->randomChance = randVal;

  mobHereNode->mobNumb = mobNumb;

  addMobHeretoList(&(roomPtr->mobHead), mobHereNode, false);

  mobHereNode->mobPtr = findMob(mobNumb);
  if (!(mobHereNode->mobPtr) && getVnumCheckVal())
  {
    char outstrn[512];

    sprintf(outstrn, 
"Error: Mob #%u requested by load mob ('M') command in zone file\n"
"       not found.\n\n",
            mobNumb);

    _outtext(outstrn);

    if (displayYesNoPrompt("Turn vnum checking off and continue", promptYes, false) == promptNo)
      exit(1);

    setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, false, true);
  }

 // add the mob to the entityLoaded list

  addEntity(ENTITY_MOB, mobNumb);

 // add limit specified to limit spec list

  addLimitSpec(limitSpecHead, ENTITY_MOB, mobNumb, maxExist);

  g_numbMobs++;

  *lastMob = *lastMobEquippable = mobHereNode;
}


//
// setDoorState : Set the state of a door in a room
//
//   *strn : String as read from zone file
//

void setDoorState(const char *strn, const uint linesRead)
{
  room *room;
  roomExit *exitNode;
  int dstate, exitnumb, ifflag;
  char dummy[512];
  uint roomnumb, unused;
  usint ch;


 // get info from string

  if (numbArgs(strn) == 5)  // old format
  {
    sscanf(strn, "%s%d%u%d%d",
           dummy, &ifflag, &roomnumb, &exitnumb, &dstate);
  }
  else if (numbArgs(strn) == 9)  // new format
  {
    sscanf(strn, "%s%d%u" "%d%d%u" "%u%u%u",
           dummy, &ifflag, &roomnumb,
           &exitnumb, &dstate, &unused,
           &unused, &unused, &unused);
  }
  else  // error
  {
    char outstrn[512];

    sprintf(outstrn, "Error in door state command (line #%u) - wrong number of args (%u)\n",
            linesRead, numbArgs(strn));

    _outtext(outstrn);

    exit(1);
  }

 // ensure the corresponding room exists

  room = findRoom(roomnumb);

  if (!room)
  {
    char outstrn[512];

    sprintf(outstrn,
"Warning: Set door state ('D') command for exit #%d invalid because the\n"
"         target room #%u does not exist (line %u).\n\n",
            exitnumb, roomnumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;
    return;
  }

 // find which exit this belongs to

  if ((exitnumb < 0) || (exitnumb >= NUMB_EXITS))
  {
    char outstrn[512];

    sprintf(outstrn,
"Warning: Exit number (%d) specified in set door state 'D' command on\n"
"         line %u is invalid.  Skipping.\n",
            exitnumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }
  else 
  {
    exitNode = room->exits[exitnumb];
  }

 // check for exit existence

  if (!exitNode)
  {
    char outstrn[512];

    sprintf(outstrn,
"Warning: Set door state ('D') command issued for a non-existent exit in\n"
"         room #%u.  (Exit number specified was #%d, line %u.)\n"
"         Ignoring.\n",
            roomnumb, exitnumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;
    return;
  }

 // check to see if door state is a valid value

  if ((dstate < 0) || (dstate > 15) || ((dstate & 3) == 3))
  {
    char outstrn[512];

    sprintf(outstrn, 
"Error: Set door state ('D') command for exit #%d in room #%u\n"
"       invalid because the door state specified is an impossible value (%d).\n"
"       (Less than 0, greater than 15, or trying to set state to 3) -\n"
"       line %u.  Aborting.\n\n",
            exitnumb, roomnumb, dstate, linesRead);

    _outtext(outstrn);

    exit(1);
  }

 // door state set by zone

  const int dstateMain = dstate & 3;   // closed/locked (unpickable is meaningless for door state)
  const int dstateMisc = dstate & 12;  // blocked/secret

 // door type of actual exit

  const int wtype = exitNode->worldDoorType & 3;
  const int wbits = exitNode->worldDoorType & 12;

 // ensure a zone door state is being set on an actual door

  if (dstateMain && !wtype)
  {
    char outstrn[512];

    sprintf(outstrn, 
"Warning: 'set door state' command specified in .zon sets door state to closed or\n"
"         locked, but the door type specified for the exit in the .wld is 'no\n"
"         door'.  (Room #%u, exit #%d, type in .wld %d,\n"
"         door state in .zon %d - line %u.)\n\n",
            roomnumb, exitnumb, wtype, dstateMain, linesRead);

    _outtext(outstrn);

    _outtext(
  "Set door type in .wld file equal to that in .zon file (W), or\n"
  "set door type in .zon file equal to that in .wld file (Z) (W/z)? ");

    do
    {
      ch = toupper(getkey());
    } while ((ch != 'W') && (ch != 'Z') && (ch != K_Enter));

    if (ch != 'Z')
    {
      _outtext("W\n");

      exitNode->worldDoorType = dstateMain | wbits;

      g_madeChanges = true;
    }
    else
    {
      _outtext("Z\n");

      exitNode->zoneDoorState = wtype | dstateMisc;

      g_madeChanges = true;
    }
  }
  else exitNode->zoneDoorState = dstate;
}


//
// putObjObj : Put an object inside another object (hopefully a container)
//
//   *strn : String as read from zone file
//

void putObjObj(const char *strn, lastObjHereLoaded **lastObjLoaded, limitSpecified **limitSpecHead, 
               bool *pauseonCont, const uint linesRead)
{
  objectHere *objContainerNode;
  objectType *objContainee, *objContainer;
               // objContainer is object that objContainee is being put into

  char dummy[512];
  uint ifflag, objConteeNumb, objContNumb, maxExist, unused;
  int randVal;


 // get info from string

  if (numbArgs(strn) == 5)  // old format
  {
    sscanf(strn, "%s%u%u%u%u",
           dummy, &ifflag, &objConteeNumb, &maxExist, &objContNumb);

    randVal = 100;

    g_madeChanges = true;
  }
  else if (numbArgs(strn) == 9)  // new format
  {
    sscanf(strn, "%s%u%u" "%u%u%d" "%u%u%u",
           dummy, &ifflag, &objConteeNumb,
           &maxExist, &objContNumb, &randVal,
           &unused, &unused, &unused);
  }
  else  // error
  {
    char outstrn[512];

    sprintf(outstrn, "Error in obj load (line #%u) - wrong number of args (%u)\n",
            linesRead, numbArgs(strn));

    _outtext(outstrn);

    exit(1);
  }

 // check random value

  if ((randVal > 100) || (randVal <= 0))
  {
    char outstrn[512];

    sprintf(outstrn, 
"Error in obj load command for object #%u (line #%u) -\n"
"random value is not 1-100%% (value read was %d) - setting to 100%%\n\n",
            objConteeNumb, linesRead, randVal);

    displayAnyKeyPrompt(outstrn);

    randVal = 100;
    g_madeChanges = true;
  }

  if ((getNumbEntities(ENTITY_OBJECT, objConteeNumb, false) >= maxExist) && getCheckLimitsVal())
  {
    char outstrn[512];

    sprintf(outstrn, "Already %u or more instances of object #%u loaded, skipping (line #%u).\n",
            maxExist, objConteeNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // find the "containee" object and make sure it exists

  objContainee = findObj(objConteeNumb);

  if (!objContainee && getVnumCheckVal())
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Put obj in obj ('P') command for object #%u invalid\n"
"       because this object type does not exist.\n\n",
            objConteeNumb);

    _outtext(outstrn);

    if (displayYesNoPrompt("Turn vnum checking off and continue", promptYes, false) == promptNo)
      exit(1);

    setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, false, true);
  }

  objContainerNode = getLastObjLoaded(*lastObjLoaded, objContNumb);

  if (!objContainerNode)
  {
    char outstrn[512];

    sprintf(outstrn, 
"Error: Put obj in obj ('P') command for object #%u invalid because\n"
"       the container into which to place the object (#%u) has not been\n"
"       previously loaded (line #%u).\n"
"\n"
"Skipping load, press a key to continue.\n\n",
            objConteeNumb, objContNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

  objContainer = objContainerNode->objectPtr;

  if (!objContainer && *pauseonCont)
  {
    sprintf(dummy, "\n"
"Error: Put obj in obj ('P') command for object #%u invalid because\n"
"       the container's object type (#%u) is not in this zone -\n"
"       stick item (and all items in a similar situation) in the\n"
"       container anyway (line %u)", 
            objConteeNumb, objContNumb, linesRead);

    if (displayYesNoPrompt(dummy, promptYes, false) == promptNo)
      exit(1);

    *pauseonCont = false;
  }
  else
  if (objContainer && (objContainer->objType != ITEM_CONTAINER))
  {
    sprintf(dummy, "\n"
"Error: Put obj in obj ('P') command for object #%u is invalid because\n"
"       this object is not a container (line %u).\n"
"\n"
"Ignore put command and continue",
            objContNumb, linesRead);

    if (displayYesNoPrompt(dummy, promptYes, false) == promptNo)
      exit(1);

    g_madeChanges = true;

    return;
  }

 // allocate memory for objHereNode

  objectHere *objHereNode = new(std::nothrow) objectHere;
  if (!objHereNode)
  {
    displayAllocError("objectHere", "putObjObj");

    exit(1);
  }

  memset(objHereNode, 0, sizeof(objectHere));

 // set stuff for objHereNode - instance of "containee" object

  objHereNode->objectNumb = objConteeNumb;
  objHereNode->objectPtr = objContainee;
  objHereNode->randomChance = randVal;

  addLastObjLoaded(lastObjLoaded, objConteeNumb, objHereNode);

 // pointers in objHereNode already set to null by memset() command above..

 // add the new object to the inside list

  addObjHeretoList(&(objContainerNode->objInside), objHereNode, false);

 // finally, add the "containee" object to the entityLoaded list

  addEntity(ENTITY_OBJECT, objConteeNumb);

 // add limit specified to limit spec list

  addLimitSpec(limitSpecHead, ENTITY_OBJECT, objConteeNumb, maxExist);

  g_numbObjs++;
}


//
// equipMobObj : Equip an object on a mob
//
//         *strn : string as read from zone file
//  *lastMobRead : mob to equip object with
//

void equipMobObj(const char *strn, mobHere *lastMobRead, lastObjHereLoaded **lastObjLoaded,
                 limitSpecified **limitSpecHead, const uint linesRead)
{
  objectType *obj;
  char dummy[512], strn2[512];
  bool incarry = false;
  uint objNumb, maxExist, equipWhere, unused, eqslot, ifflag;
  int randVal;

 // get info from string

  if (numbArgs(strn) == 5)  // old format
  {
    sscanf(strn, "%s%u%u%u%u",
           dummy, &ifflag, &objNumb, &maxExist, &equipWhere);

    randVal = 100;

    g_madeChanges = true;
  }
  else if (numbArgs(strn) == 9)  // new format
  {
    sscanf(strn, "%s%u%u" "%u%u%d" "%u%u%u",
           dummy, &ifflag, &objNumb,
           &maxExist, &equipWhere, &randVal,
           &unused, &unused, &unused);
  }
  else  // error
  {
    char outstrn[512];

    sprintf(outstrn, "Error in mob equip (line #%u) - wrong number of args (%u)\n",
            linesRead, numbArgs(strn));

    _outtext(outstrn);

    exit(1);
  }

 // check random value

  if ((randVal > 100) || (randVal <= 0))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error in obj load command for object #%u (line #%u) -\n"
"random value is not 1-100%% (value read was %d) - setting to 100%%\n\n",
            objNumb, linesRead, randVal);

    displayAnyKeyPrompt(outstrn);

    randVal = 100;

    g_madeChanges = true;
  }

 // check equipWhere

  if (equipWhere > CUR_MAX_WEAR)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Equip mob with obj ('E') command for object #%u\n"
"       (line #%u) failed because equip position (#%u) is\n"
"       invalid.\n\n"
"       Aborting.\n",
            objNumb, linesRead, equipWhere);

    _outtext(outstrn);

    exit(1);
  }

 // check load limit

  if ((getNumbEntities(ENTITY_OBJECT, objNumb, false) >= maxExist) && getCheckLimitsVal())
  {
    char outstrn[512];

    sprintf(outstrn, "Already %u or more instances of object #%u loaded, skipping (line #%u).\n\n",
            maxExist, objNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // find the mob and make sure it exists

  if (!lastMobRead || !lastMobRead->mobPtr)
  {
    char outstrn[512];

    sprintf(outstrn, 
"Error: Equip mob with obj ('E') command invalid because no mob has been loaded\n"
"       previous to the command (object #%u, line #%u).\n"
"\n"
"Skipping load, press a key.\n\n",
            objNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // find the object to be equipped and make sure it exists

  obj = findObj(objNumb);

  if (!obj && getVnumCheckVal())
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Equip mob with obj ('E') command for object #%u invalid because\n"
"       this object type does not exist (line #%u).\n\n",
            objNumb, linesRead);

    _outtext(outstrn);

    if (displayYesNoPrompt("Turn vnum checking off and continue", promptYes, false) == promptNo)
      exit(1);

    setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, false, true);
  }

  eqslot = getMobHereEquipSlot(lastMobRead, obj, equipWhere);
  if (eqslot > WEAR_TRUEHIGH)
  {
    char outstrn[512];

    sprintf(outstrn, "\n"
"Error: Equip mob with obj ('E') command failed - object #%u is being\n"
"       equipped on mob #%u (line #%u).\n"
"\n"
"       The error was '%s'.\n"
"\n"
"Stick object in mob's carried list and continue",
            objNumb, lastMobRead->mobNumb, linesRead, getCanEquipErrStrn(eqslot, strn2));

    if (displayYesNoPrompt(outstrn, promptYes, false) == promptNo)
      exit(1);

    incarry = g_madeChanges = true;
  }

 // allocate memory for objHereNode

  objectHere *objHereNode = new(std::nothrow) objectHere;
  if (!objHereNode)
  {
    displayAllocError("objectHere", "equipMobObj");

    exit(1);
  }

  memset(objHereNode, 0, sizeof(objectHere));

 // set stuff

  objHereNode->objectNumb = objNumb;
  objHereNode->objectPtr = obj;
  objHereNode->randomChance = randVal;

 // pointers in objHereNode already set to null by memset() command above..

 // add the new node to the equipHead list of the mob

  if (!incarry) 
    lastMobRead->equipment[eqslot] = objHereNode;
  else 
    addObjHeretoList(&(lastMobRead->inventoryHead), objHereNode, false);

 // add the object being equipped to the entityLoaded list and
 // lastObjLoaded list

  addEntity(ENTITY_OBJECT, objNumb);
  addLastObjLoaded(lastObjLoaded, objNumb, objHereNode);

 // add limit specified to limit spec list

  addLimitSpec(limitSpecHead, ENTITY_OBJECT, objNumb, maxExist);

  g_numbObjs++;
}


//
// giveMobObj : Give an object to a mob
//
//         *strn : string as read from zone file
//  *lastMobRead : mob to give object to
//

void giveMobObj(const char *strn, mobHere *lastMobRead, lastObjHereLoaded **lastObjLoaded,
                limitSpecified **limitSpecHead, const uint linesRead)
{
  objectType *obj;
  char dummy[512];
  uint objNumb, maxExist, unused, ifflag;
  int randVal;

 // get info from string

  if (numbArgs(strn) == 4)  // old format
  {
    sscanf(strn, "%s%u%u%u", dummy, &ifflag, &objNumb, &maxExist);

    randVal = 100;
    g_madeChanges = true;
  }
  else if (numbArgs(strn) == 9)  // new format
  {
    sscanf(strn, "%s%u%u" "%u%u%d" "%u%u%u",
           dummy, &ifflag, &objNumb,
           &maxExist, &unused, &randVal,
           &unused, &unused, &unused);
  }
  else  // error
  {
    char outstrn[512];

    sprintf(outstrn, "Error in obj load (line #%u) - wrong number of args (%u)\n",
            linesRead, numbArgs(strn));

    _outtext(outstrn);

    exit(1);
  }

 // check random value

  if ((randVal > 100) || (randVal <= 0))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error in obj load command for object #%u (line #%u) -\n"
"random value is not 1-100%% (value read was %d) - setting to 100%%\n",
            objNumb, linesRead, randVal);

    _outtext(outstrn);

    randVal = 100;
    g_madeChanges = true;
  }

 // check number loaded

  if ((getNumbEntities(ENTITY_OBJECT, objNumb, false) >= maxExist) && getCheckLimitsVal())
  {
    char outstrn[512];

    sprintf(outstrn, "Already %u or more instances of object #%u loaded, skipping (line #%u).\n",
            maxExist, objNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // find the object to be equipped and make sure it exists

  obj = findObj(objNumb);

  if (!obj && getVnumCheckVal())
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Give obj to mob ('G') command for object #%u invalid because\n"
"       this object type does not exist (line #%u).\n\n",
            objNumb, linesRead);

    _outtext(outstrn);

    if (displayYesNoPrompt("Turn vnum checking off and continue", promptYes, false) == promptNo)
      exit(1);

    setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, false, true);
  }

 // find the mob and make sure it exists

  if (!lastMobRead)
  {
    char outstrn[512];

    sprintf(outstrn, 
"Error: Give obj to mob ('G') command invalid because no mob has been loaded\n"
"       previous to the command (line #%u).\n"
"\n"
"Skipping load, press a key to continue.\n",
            linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // allocate memory for objHereNode

  objectHere *objHereNode = new(std::nothrow) objectHere;
  if (!objHereNode)
  {
    displayAllocError("objectHere", "giveMobObj");

    exit(1);
  }

  memset(objHereNode, 0, sizeof(objectHere));

 // set stuff

  objHereNode->objectNumb = objNumb;
  objHereNode->objectPtr = obj;
  objHereNode->randomChance = randVal;

 // pointers in objHereNode already set to null by memset() command above..

 // add the new node to the inventory list of the mob

  addObjHeretoList(&(lastMobRead->inventoryHead), objHereNode, false);

 // finally, add the object being given to the entityLoaded list

  addEntity(ENTITY_OBJECT, objNumb);
  addLastObjLoaded(lastObjLoaded, objNumb, objHereNode);

 // add limit specified to limit spec list

  addLimitSpec(limitSpecHead, ENTITY_OBJECT, objNumb, maxExist);

  g_numbObjs++;
}


//
// mountMob : One mob mounts another
//
//          *strn : string as read from zone file
//  **lastMobRead : mob to mount
//

void mountMob(const char *strn, mobHere **lastMobRead, limitSpecified **limitSpecHead, 
              const uint linesRead)
{
  char dummy[512];
  uint mobNumb, maxExist, roomNumb, unused, ifflag;
  mobType *mob;
  room *roomPtr;
  int randVal;

 // get info from string

  if (numbArgs(strn) == 5)
  {
    sscanf(strn, "%s%u%u%u%u", dummy, &ifflag, &mobNumb, &maxExist, &roomNumb);

    randVal = 100;
    g_madeChanges = true;
  }
  else if (numbArgs(strn) == 9)
  {
    sscanf(strn, "%s%u%u" "%u%u%d" "%u%u%u",
           dummy, &ifflag, &mobNumb,
           &maxExist, &roomNumb, &randVal,
           &unused, &unused, &unused);
  }
  else
  {
    char outstrn[512];

    sprintf(outstrn, "Error in mob load (line #%u) - wrong number of args (%u)\n",
            linesRead, numbArgs(strn));

    _outtext(outstrn);

    exit(1);
  }

 // check random value

  if ((randVal > 100) || (randVal <= 0))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error in mount command for mob #%u (line #%u) -\n"
"random value is not 1-100%% (value read was %d) - setting to 100%%\n\n",
            mobNumb, linesRead, randVal);

    _outtext(outstrn);

    randVal = 100;
    g_madeChanges = true;
  }

 // check limit

  if ((getNumbEntities(ENTITY_MOB, mobNumb, false) >= maxExist) && getCheckLimitsVal())
  {
    char outstrn[512];

    sprintf(outstrn, "Already %u or more instances of mob #%u loaded, skipping (line #%u).\n",
            maxExist, mobNumb, linesRead);

    displayAnyKeyPrompt(outstrn);
            
    g_madeChanges = true;

    return;
  }

 // find the mob to be the mount and make sure it exists

  mob = findMob(mobNumb);

  if (!mob && getVnumCheckVal())
  {
    char outstrn[512];

    sprintf(outstrn, 
"Error: Mount mob ('R') command for mob #%u invalid because this\n"
"       mob (mount) type does not exist (line #%u).\n\n",
            mobNumb, linesRead);

    _outtext(outstrn);

    if (displayYesNoPrompt("Turn vnum checking off and continue", promptYes, false) == promptNo)
      exit(1);

    setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, false, true);
  }

 // make sure a mob has been loaded previous

  if (!(*lastMobRead))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Mount mob ('R') command invalid because no mob has been loaded\n"
"       previous to the command (line #%u).\n"
"\n"
"Skipping load, press a key.\n\n",
            linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // make sure mob isn't already riding something

  if ((*lastMobRead)->riding)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Mount mob ('R') command invalid because mob #%u\n"
"       is already riding a mount (mob #%u) - line #%u.  Aborting.\n",
            (*lastMobRead)->mobNumb, (*lastMobRead)->riding->mobNumb, linesRead);

    _outtext(outstrn);

    exit(1);
  }

  roomPtr = findRoom(roomNumb);
  if (!roomPtr)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Mount mob ('R') command invalid because invalid room vnum specified\n"
"       (#%u, line #%u).\n"
"\n"
"Skipping load, press a key.\n\n",
            roomNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // allocate memory for mobHereNode - mount

  mobHere *mobHereNode = new(std::nothrow) mobHere;
  if (!mobHereNode)
  {
    displayAllocError("mobHere", "mountMob");

    exit(1);
  }

  memset(mobHereNode, 0, sizeof(mobHere));

 // set stuff

  mobHereNode->mobNumb = mobNumb;
  mobHereNode->mobPtr = mob;
  mobHereNode->randomChance = randVal;

  addMobHeretoList(&(roomPtr->mobHead), mobHereNode, false);

 // pointers in mobHereNode already set to null by memset() command above..

  (*lastMobRead)->riding = mobHereNode;
  mobHereNode->riddenBy = *lastMobRead;

 // finally, add the mount being ridden to the entityLoaded list

  addEntity(ENTITY_MOB, mobNumb);
  g_numbMobs++;

 // add limit specified to limit spec list

  addLimitSpec(limitSpecHead, ENTITY_MOB, mobNumb, maxExist);

  *lastMobRead = mobHereNode;  // reset last mob read to mount
}


//
// followMob : One mob follows another
//
//          *strn : string as read from zone file
//   *lastMobRead : mob to follow
//  **lastMobEquippable : this gets reset
//

void followMob(const char *strn, mobHere *lastMobRead, mobHere **lastMobEquippable,
               limitSpecified **limitSpecHead, const uint linesRead)
{
  char dummy[512];
  uint mobNumb, maxExist, roomNumb, unused, ifflag;
  mobType *mob;
  room *room;
  int randVal;


 // make sure a mob has been loaded previous

  if (!lastMobRead)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Follow mob ('F') command invalid because no mob has been loaded\n"
"       previous to the command (line #%u).\n"
"\n"
"Skipping load, press a key.\n\n",
            linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // get info from string

  if (numbArgs(strn) == 5)
  {
    sscanf(strn, "%s%u%u%u%u", dummy, &ifflag, &mobNumb, &maxExist, &roomNumb);

    randVal = 100;
    g_madeChanges = true;
  }
  else if (numbArgs(strn) == 9)
  {
    sscanf(strn, "%s%u%u" "%u%u%d" "%u%u%u",
           dummy, &ifflag, &mobNumb,
           &maxExist, &roomNumb, &randVal,
           &unused, &unused, &unused);
  }
  else
  {
    char outstrn[512];

    sprintf(outstrn, "Error in mob load (line #%u) - wrong number of args (%u)\n",
            linesRead, numbArgs(strn));
    _outtext(outstrn);

    exit(1);
  }

 // check random value

  if ((randVal > 100) || (randVal <= 0))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error in follow command for mob #%u (line #%u) -\n"
"random value is not 1-100%% (value read was %d) - setting to 100%%\n",
            mobNumb, linesRead, randVal);

    displayAnyKeyPrompt(outstrn);

    randVal = 100;
    g_madeChanges = true;
  }

 // check limit

  if ((getNumbEntities(ENTITY_MOB, mobNumb, false) >= maxExist) && getCheckLimitsVal())
  {
    char outstrn[512];

    sprintf(outstrn, "Already %u or more instances of mob #%u loaded, skipping (line #%u).\n",
            maxExist, mobNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // find the mob to be the mount and make sure it exists

  mob = findMob(mobNumb);

  if (!mob && getVnumCheckVal())
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Follow mob ('F') command for mob #%u invalid because this\n"
"       mob type does not exist (line #%u).\n\n",
            mobNumb, linesRead);

    _outtext(outstrn);

    if (displayYesNoPrompt("Turn vnum checking off and continue", promptYes, false) == promptNo)
      exit(1);

    setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, false, true);
  }

  room = findRoom(roomNumb);
  if (!room)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Follow mob ('F') command invalid because invalid room vnum specified\n"
"       (#%u, line #%u).\n"
"\n"
"Skipping load, press a key.\n\n",
            roomNumb, linesRead);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    return;
  }

 // allocate memory for mobHereNode - follower

  mobHere *mobHereNode = new(std::nothrow) mobHere;
  if (!mobHereNode)
  {
    displayAllocError("mobHere", "followMob");

    exit(1);
  }

  memset(mobHereNode, 0, sizeof(mobHere));

 // set stuff

  mobHereNode->mobNumb = mobNumb;
  mobHereNode->mobPtr = mob;
  mobHereNode->following = lastMobRead;
  mobHereNode->randomChance = randVal;

  addMobHeretoList(&(room->mobHead), mobHereNode, false);

 // finally, add the follower to the entityLoaded list

  addEntity(ENTITY_MOB, mobNumb);
  g_numbMobs++;

 // add limit specified to limit spec list

  addLimitSpec(limitSpecHead, ENTITY_MOB, mobNumb, maxExist);

  *lastMobEquippable = mobHereNode;  // reset last mob read to mount
}


//
// readZoneFile : Reads all the zone info and commands from the user-specified
//                zone file - if filename is non-NULL, it is used as the name
//                of the file to be read, else value in MAINZONENAME is used -
//                returns FALSE if couldn't read file
//

bool readZoneFile(const char *filename)
{
  FILE *zoneFile;
  mobHere *lastMobRead = NULL, *lastMobEquippable = NULL;

 // lastMobRead is used for F commands, lastMobEquippable is used
 // for E, G commands.  lastMobRead reset only by M.  lastMobEquippable
 // reset by M F and R

  lastObjHereLoaded *lastObjLoaded = NULL;

  limitSpecified *limitSpecHead = NULL;

  char zoneFilename[512] = "", strn[512];
  bool contPause = true;

  uint lineReading = 1, unused, temp, toproom;

 // note - every zone command now has 8 args - the 5th arg is always the
 //        %age to load, 6-8 are unused as of now (8 args include if-flag)

 // weather info is gone - first #zone numb, CR/LF, name, ~, CR/LF,
 //                        then top room, reset mode, flags, lifespan min,
 //                        lifespan max, CR/LF, and rest of data


 // assemble the filename of the zone file

  if (g_readFromSubdirs) 
    strcpy(zoneFilename, "zon/");

  if (filename) 
    strcat(zoneFilename, filename);
  else 
    strcat(zoneFilename, getMainZoneNameStrn());

  strcat(zoneFilename, ".zon");

 // open the zone file for reading

  if ((zoneFile = fopen(zoneFilename, "rt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(zoneFilename);
    _outtext(", skipping\n");

    return false;
  }

  _outtext("Reading ");
  _outtext(zoneFilename);
  _outtext("...\n");

 // first, read the zone number

  if (!readAreaFileLine(zoneFile, strn, 512, ENTITY_ZONE_LINE, lineReading, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "numb", 0, NULL, false, true))
    exit(1);

  if (strn[0] != '#')
  {
    char outstrn[1024];

    sprintf(outstrn,
"Error: '#' expected at start of zone number, instead found '%c'.\n\n"
"Entire string was '%s'.\n",
            strn[0], strn);

    _outtext(outstrn);

    exit(1);
  }

 // remove the '#' at the start of the string

  deleteChar(strn, 0);

 // set the zone number

  g_zoneRec.zoneNumber = strtoul(strn, NULL, 10);

  lineReading++;

 // read the zone name

  if (!readAreaFileLine(zoneFile, strn, MAX_ZONENAME_LEN, ENTITY_ZONE_LINE, lineReading, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "name", 0, NULL, true, true))
    exit(1);

  strcpy(g_zoneRec.zoneName, strn);

  lineReading++;

 // check if the zone file is new format or the old format - new format
 // has 6 args on this line, old format has 4

 // weather info is gone - first #zone numb, CR/LF, name, ~, CR/LF,
 //                        then top room, reset mode, flags, lifespan min,
 //                        lifespan max, CR/LF, and rest of data

  const size_t intMiscArgs[] = { 4, 6, 0 };

  if (!readAreaFileLine(zoneFile, strn, 512, ENTITY_ZONE_LINE, lineReading, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "misc info", 0, intMiscArgs, false, true))
    exit(1);

  if (numbArgs(strn) == 4)  // old format
  {
    displayAnyKeyPromptNoClr(
"Zone file is in old format.  durisEdit will convert the zone file into the new\n"
"format when you save the zone.\n\n"
"Press a key to continue..\n");

   // interpret the string

    sscanf(strn, "%u%u%u%u",
           &toproom,
           &temp,
           &(g_zoneRec.resetMode),
           &(g_zoneRec.miscBits.longIntFlags));

   // read the rest of the zone info

    if (fscanf(zoneFile, "%u%u%u\n"   "%u%u%u%u\n" "%u%u%u%u\n"
                         "%u%u%u%u\n" "%u%u%u%u\n" "%u%u%u%u\n",

              // yeeha..  read all the now-useless stuff

               &unused, &unused, &unused,
               &unused, &unused, &unused, &unused,
               &unused, &unused, &unused, &unused,
               &unused, &unused, &unused, &unused,
               &unused, &unused, &unused, &unused,
               &unused, &unused, &unused, &unused) == EOF)
    {
      _outtext("\n"
  "Error: Hit EOF while reading info beyond zone name (reading old format zone\n"
  "file).  Aborting.\n");

      exit(1);
    }

   // set new lifeLow and lifeHigh vals based on old lifeSpan var

    if (temp > 5) 
      g_zoneRec.lifeLow = temp - 5;
    else 
      g_zoneRec.lifeLow = 1;

    g_zoneRec.lifeHigh = temp + 5;

    g_zoneRec.zoneDiff = 1;

    g_madeChanges = true;

    lineReading += 7;
  }
  else if (numbArgs(strn) == 6)  // new format
  {
    sscanf(strn, "%u%u%u%u%u%u\n",
           &toproom,
           &(g_zoneRec.resetMode),
           &(g_zoneRec.miscBits.longIntFlags),
           &(g_zoneRec.lifeLow),
           &(g_zoneRec.lifeHigh),
           &(g_zoneRec.zoneDiff));

   // arg 6 used to be unused, so let's reset the difficulty, SHALL WE??

    if ((g_zoneRec.zoneDiff <= 0) || (g_zoneRec.zoneDiff > 10))
      g_zoneRec.zoneDiff = 1;

    lineReading++;
  }

 // check for common errors in zone data

  if (!roomExists(toproom))
  {
    char outstrn[512];

    sprintf(outstrn,
"\nWarning: Top room (#%u) for zone not found.  It will be reset on save.\n",
            toproom);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;
  }
  else
  if (toproom != getHighestRoomNumber())
  {
    char outstrn[512];

    sprintf(outstrn, "\n"
"Warning: Top room (#%u) for zone not equal to actual top room number\n"
"         (#%u).  It will be reset on save.\n",
             toproom, getHighestRoomNumber());

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;
  }

  if (g_zoneRec.lifeLow >= g_zoneRec.lifeHigh)
  {
    char outstrn[512];

    sprintf(outstrn, "\n"
"Warning: Low lifespan value for zone less than or equal to high lifespan value\n"
"         (%u vs %u.)\n\n"
"Setting low lifespan value to %u and high lifespan value to %u.\n",
            g_zoneRec.lifeLow, g_zoneRec.lifeHigh, g_zoneRec.lifeHigh, g_zoneRec.lifeHigh + 5);

    displayAnyKeyPrompt(outstrn);

    g_madeChanges = true;

    g_zoneRec.lifeLow = g_zoneRec.lifeHigh;
    g_zoneRec.lifeHigh += 5;
  }

  if( g_zoneRec.miscBits.zoneMiscBits.map )
  {
     if (!readAreaFileLine(zoneFile, strn, 512, ENTITY_ZONE_LINE, lineReading, ENTITY_TYPE_UNUSED, 
               ENTITY_NUMB_UNUSED, "map size", 0, NULL, false, true))
               exit(0);

      sscanf(strn, "%u %u",
        &(g_zoneRec.mapWidth),
        &(g_zoneRec.mapHeight));
  }

 // read command by command, one command per iteration

  while (true)
  {
   // Read a command

    bool hitEOF;

    if (!readAreaFileLineAllowEOF(zoneFile, strn, 512, ENTITY_ZONE_LINE, lineReading, ENTITY_TYPE_UNUSED, 
                                  ENTITY_NUMB_UNUSED, "command", 0, NULL, false, true, &hitEOF))
      exit(1);

    if (hitEOF)
    {
     // if not ignoring the 'S' that is supposed to be at the end of
     // each zone file, hitting EOF with no 'S' is an error - otherwise,
     // assume we ignored the appropriate 'S' and close file as we would
     // normally

      if (!getIgnoreZoneSVal())
      {
        _outtext("\n"
"Error: Hit end of file on zone file while reading commands.  Aborting.\n");

        exit(1);
      }
      else
      {
        fclose(zoneFile);  
        
        deleteLastObjLoadedList(lastObjLoaded);

        setOverrideFromLimSpec(limitSpecHead);
        deleteLimitSpecList(limitSpecHead);

        return true;
      }
    }

    removeComments(strn);
    remLeadingSpaces(strn);

    switch (toupper(strn[0]))
    {
      case '*' : break;  // ignore rest of line (comment)
      case '\0': break;  // blank line - ignore it

     // load a mob

      case 'M' : loadMob(strn, &lastMobRead, &lastMobEquippable, &limitSpecHead, lineReading);
                 break;

     // load an object

      case 'O' : loadObj(strn, &lastObjLoaded, &limitSpecHead, lineReading);
                 break;

     // give a mob an object

      case 'G' : giveMobObj(strn, lastMobEquippable, &lastObjLoaded, &limitSpecHead, lineReading);
                 break;

     // equip mob with object

      case 'E' : equipMobObj(strn, lastMobEquippable, &lastObjLoaded, &limitSpecHead, lineReading);
                 break;

     // put object in object

      case 'P' : putObjObj(strn, &lastObjLoaded, &limitSpecHead, &contPause, lineReading);
                 break;

     // set door state

      case 'D' : setDoorState(strn, lineReading);
                 break;

     // mount mob on another

      case 'R' : mountMob(strn, &lastMobEquippable, &limitSpecHead, lineReading);
                 break;

     // have mob follow another

      case 'F' : followMob(strn, lastMobRead, &lastMobEquippable, &limitSpecHead, lineReading);
                 break;

     // end of zone file

      case 'S' : if (!getIgnoreZoneSVal())
                 {
                   fclose(zoneFile);  
                   
                   deleteLastObjLoadedList(lastObjLoaded);

                   setOverrideFromLimSpec(limitSpecHead);
                   deleteLimitSpecList(limitSpecHead);

                   return true;
                 }

                 break;

      default  : _outtext("\n"
"Error: Unrecognized command in zone file - string read was\n"
"       '");
        
                 _outtext(strn);
                 
                 sprintf(zoneFilename, "' (line #%u).  Aborting.\n",
                         lineReading);

                 _outtext(zoneFilename);

                 exit(1);
    }

    lineReading++;
  }
}
