//
//  File: look.cpp       originally part of durisEdit
//
//  Usage: functions related to looking at rooms, maps, obj contents..
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
#include <stdlib.h>
#include <ctype.h>

#include "../types.h"
#include "../fh.h"

#include "../obj/objhere.h"
#include "../misc/master.h"
#include "../misc/editable.h"

extern room *g_currentRoom;
extern zone g_zoneRec;
extern const char *g_exitnames[];


//
// getObjShortNameDisplayStrn : you best ensure that strn is big enough - it should be at least 2048
//

char *getObjShortNameDisplayStrn(char *strn, const objectHere *objHere, const uint intIndent, 
                                 const bool blnKeywords)
{
  char *strptr = strn;

  for (uint i = 0; i < intIndent; i++)
  {
    *strptr = ' ';
    strptr++;
  }

  sprintf(strptr, "%s&n", getObjShortName(objHere->objectPtr));

  if (getShowObjVnumVal() || !objHere->objectPtr)
    sprintf(strn + strlen(strn), " (#%u)", objHere->objectNumb);

  if (blnKeywords)
  {
    char keystrn[1024];

    if (objHere->objectPtr)
      getReadableKeywordStrn(objHere->objectPtr->keywordListHead, keystrn, 1023);
    else
      strcpy(keystrn, "keywords unknown");

    sprintf(strn + strlen(strn), " (%s)", keystrn);
  }

  sprintf(strn + strlen(strn), " &+c(%u%%)&n\n", objHere->randomChance);

  return strn;
}


//
// displayIndividualObj : display one obj in a list - inventory or obj contents
//                        returns true if user quit on pause
//

bool displayIndividualObj(const objectHere *objHere, const uint intIndent, size_t &numbLines)
{
  char strn[2048];

  getObjShortNameDisplayStrn(strn, objHere, intIndent, false);

  return checkPause(strn, numbLines);
}


//
// displayContainerContents : returns true if user quit on pause
//

bool displayContainerContents(const objectHere *objInside, const uint intIndent, size_t &numbLines)
{
  while (objInside)
  {
    if (displayIndividualObj(objInside, intIndent, numbLines))
      return true;

    if (intIndent + 2 <= 50)
    {
      if (getShowObjContentsVal() && displayContainerContents(objInside->objInside, intIndent + 2, numbLines))
        return true;
    }
    else
    {
      if (getShowObjContentsVal() && displayContainerContents(objInside->objInside, 50, numbLines))
        return true;
    }

    objInside = objInside->Next;
  }

  return false;
}


//
// lookObj : look at an object
//

void lookObj(const objectHere *objHere, size_t &numbLines)
{
 // if object has extra descs, display the first one as 'the' desc - works for me

  _outtext("\n");

  if (!objHere->objectPtr || !objHere->objectPtr->extraDescHead || 
      !objHere->objectPtr->extraDescHead->extraDescStrnHead)
  {
    if (checkPause("That object has no matching description.\n\n", numbLines))
      return;
  }
  else
  {
    displayStringNodes(objHere->objectPtr->extraDescHead->extraDescStrnHead);
  }

 // display anything inside the object

  if (objHere->objInside)
  {
    if (checkPause("\n&+YObject contains:&n\n", numbLines))
      return;

    if (displayContainerContents(objHere->objInside, 2, numbLines))
      return;

    checkPause("\n", numbLines);
  }
}


//
// lookMob : look at a mob
//

void lookMob(const mobHere *mob, size_t &numbLines)
{
 // display mob desc, if any

  _outtext("\n");

  if (!mob->mobPtr || !mob->mobPtr->mobDescHead)
  {
    if (checkPause("That mob has no description.\n", numbLines))
      return;
  }
  else
  {
    displayStringNodes(mob->mobPtr->mobDescHead);
  }

 // display species, align, class, and whatever else

  if (displayMobMisc(mob, numbLines))
    return;

  if (checkPause("\n", numbLines))
    return;

 // display mob equipment, if there is any

  const bool mobEq = mobEquippingSomething(mob);

  if (mobEq)
  {
    if (displayMobEquip(mob, 0, numbLines))
      return;

    if (checkPause("\n", numbLines))
      return;
  }

 // display objects mob is carrying, if any

  if (mob->inventoryHead)
  {
    if (!mobEq && checkPause("\n", numbLines))
      return;

    if (checkPause("&+YCarrying:&n\n", numbLines))
      return;

    const objectHere *mobObj = mob->inventoryHead;

    while (mobObj)
    {
      displayIndividualObj(mobObj, 2, numbLines);

      if (displayContainerContents(mobObj->objInside, 4, numbLines))
        return;

      mobObj = mobObj->Next;
    }

    _outtext("\n");
  }
}


//
// displayMobEqPos
//

bool displayMobEqPos(const mobHere *mob, const char pos, const char *posStrn, const uint intIndent,
                     size_t& numbLines)
{
  objectHere *obj;
  char strn[512];

  if (!mob || (pos < WEAR_LOW) || (pos > WEAR_TRUEHIGH)) 
    return false;

  obj = mob->equipment[pos];
  if (obj)
  {
    char fullPosStrn[256] = "", *strptr = fullPosStrn;

    for (uint i = 0; (i < intIndent) && (i < 50); i++)
    {
      *strptr = ' ';
      strptr++;
    }

    *strptr = '\0';

    sprintf(fullPosStrn + strlen(fullPosStrn), "<%s>", posStrn);

    sprintf(strn, "%-22s", fullPosStrn);
    
    getObjShortNameDisplayStrn(strn + strlen(strn), obj, 0, false);

    if (checkPause(strn, numbLines))
      return true;

   // now, display any contents in equipment

    if (getShowObjContentsVal() && displayContainerContents(obj->objInside, intIndent + 2, numbLines))
      return true;
  }

  return false;
}

// eq list for use in displayMobEquip() - slots are displayed from first to last

typedef struct _mobeqDisp
{
  uchar eqPos;
  const char *posName;
} mobeqDisp;

const mobeqDisp mobeqDispList[] =
{
  { WEAR_IOUN,      "worn as ioun stone" },
  { GUILD_INSIGNIA, "worn as a badge" },
  { WEAR_HORN,      "worn on horns" },
  { WEAR_WHOLEHEAD, "worn on whole head" },
  { WEAR_HEAD,      "worn on head" },
  { WEAR_EYES,      "worn on eyes" },
  { WEAR_EARRING_L, "worn in ear" },
  { WEAR_EARRING_R, "worn in ear" },
  { WEAR_NOSE,      "worn in nose" },
  { WEAR_FACE,      "worn on face" },
  { WEAR_QUIVER,    "worn as quiver" },
  { WEAR_NECK_1,    "worn around neck" },
  { WEAR_NECK_2,    "worn around neck" },
  { WEAR_BACK,      "worn on back" },
  { WEAR_WHOLEBODY, "worn on whole body" },
  { WEAR_BODY,      "worn on body" },
  { WEAR_ABOUT,     "worn about body" },
  { WEAR_WAIST,     "worn about waist" },
  { WEAR_ATTACH_BELT_1, "attached to belt" },
  { WEAR_ATTACH_BELT_2, "attached to belt" },
  { WEAR_ATTACH_BELT_3, "attached to belt" },
  { WEAR_ARMS,      "worn on arms" },
  { WEAR_ARMS_2,    "worn on lower arms" },
  { WEAR_SHIELD,    "held as shield" },
  { WEAR_WRIST_L,   "worn around wrist" },
  { WEAR_WRIST_R,   "worn around wrist" },
  { WEAR_WRIST_LL,  "worn on lower wrist" },
  { WEAR_WRIST_LR,  "worn on lower wrist" },
  { WEAR_HANDS,     "worn on hands" },
  { WEAR_HANDS_2,   "worn on lower hands" },
  { WEAR_FINGER_L,  "worn on finger" },
  { WEAR_FINGER_R,  "worn on finger" },
  { WIELD_TWOHANDS, "wielding two-handed" },
  { WIELD_TWOHANDS2,"wielding two-handed" },
  { WIELD,          "primary weapon" },
  { WIELD2,         "secondary weapon" },
  { WIELD3,         "third weapon" },
  { WIELD4,         "fourth weapon" },
  { WEAR_LIGHT,     "as light-not imped" },
  { HOLD,           "held" },
  { HOLD2,          "held" },
  { WEAR_LEGS,      "worn on legs" },
  { WEAR_LEGS_REAR, "worn on rear legs" },
  { WEAR_FEET,      "worn on feet" },
  { WEAR_FEET_REAR, "worn on rear feet" },
  { WEAR_TAIL,      "worn on tail" },
  { 0, 0 }
};

//
// displayMobEquip : Displays a mob's equipment list
//
//   *mob : Pointer to mob
//

bool displayMobEquip(const mobHere *mob, const uint intIndent, size_t& numbLines)
{
  const mobeqDisp *dispPtr = mobeqDispList;

  while (dispPtr->posName)
  {
    if (displayMobEqPos(mob, dispPtr->eqPos, dispPtr->posName, intIndent, numbLines))
      return true;

    dispPtr++;
  }

  return false;
}


//
// displayMobMisc : Displays "miscellaneous" info on a mob - level, align, and
//                  class
//
//    *mob : mob to display info on
//

bool displayMobMisc(const mobHere *mob, size_t &numbLines)
{
  char outstrn[3072], strn[2048];
  const mobType *mobT;


  mobT = mob->mobPtr;
  updateSpecList(mobT);

  if (!mobT)
  {
    return checkPause(
"This mob is not in this .mob file; thus, no information is available.\n", numbLines);
  }

  sprintf(outstrn,
"&+W%s&n is &+clevel %d&n, &+malign %d&n, of class(es) '%s&n'(%s), and a%s &+g%s&n.\n",
          mobT->mobShortName, mobT->level,
          mobT->alignment, getClassString(mobT, strn, 2047),
	  getMobSpecStrn(mobT->mobSpec),
          getVowelN(getMobSpeciesNoColorStrn(mobT->mobSpecies)[0]),
          getMobSpeciesStrn(mobT->mobSpecies));

  if (checkPause(outstrn, numbLines))
    return true;

  if (mobT->questPtr)
  {
    if (checkPause("&+YThis mob has quest information.&n\n", numbLines))
      return true;
  }

  if (mobT->shopPtr)
  {
    if (checkPause("&+YThis mob has a shop.&n\n", numbLines))
      return true;
  }

  return false;
}


//
// lookEntity : Look at something
//
//   *args : string as entered by user
//

void lookEntity(const char *args)
{
  char whatMatched;
  masterKeywordListNode *matchingNode;

  size_t numbLines = 0;


 // remove "at " if it exists (i.e. "look at me")

  if (strlefti(args, "AT "))
  {
    args += 3;
  }

 // check if user is looking by vnum

  if (strnumber(args))
  {
    const uint vnum = strtoul(args, NULL, 10);

    const objectHere *objHere = findObjHere(vnum, NULL, true, true);

    if (objHere)
    {
      lookObj(objHere, numbLines);

      return;
    }

   // no object match, check for mobs

    const mobHere *mob = findMobHere(vnum, NULL, true);

    if (mob)
    {
      lookMob(mob, numbLines);

      return;
    }
  }

 // user either entered keyword or no vnum match found

  checkCurrentMasterKeywordList(args, &whatMatched, &matchingNode);

 // for objects or mobs, set a few variables

  if (whatMatched == ENTITY_OBJECT)
  {
    lookObj((objectHere *)(matchingNode->entityPtr), numbLines);
  }
  else
  if (whatMatched == ENTITY_MOB)
  {
    lookMob((mobHere *)(matchingNode->entityPtr), numbLines);
  }

 // room/obj edesc

  else 
  if ((whatMatched == ENTITY_R_EDESC) || (whatMatched == ENTITY_O_EDESC))
  {
    const stringNode *desc = matchingNode->entityDesc;

    if (desc)
    {
      if (checkPause("\n", numbLines))
        return;

      if (displayStringNodesPause(desc, numbLines))
        return;

      if (checkPause("\n", numbLines))
        return;
    }
    else
    {
      _outtext("\nThat extra desc has no actual description.\n\n");
    }
  }

 // no match found

  else 
  {
    _outtext("\nLook at what?\n\n");
  }
}


//
// lookExit : Looks at a room exit
//
//  *exitNode : exit to "look" at
//

void lookExit(const roomExit *exitNode)
{
  int zoneState, worldType;
  char strn[1024];
  objectType *keyObj;
  stringNode *strnNode;
  size_t numbLines = 0;


  if (exitNode)
  {
    zoneState = getZoneDoorState(exitNode);
    worldType = getWorldDoorType(exitNode);

   // display description, if any

    if (exitNode->exitDescHead)
    {
      checkPause("\n", numbLines);
      if (displayStringNodesPause(exitNode->exitDescHead, numbLines))
        return;

      if (!worldType && checkPause("\n", numbLines))
          return;
    }
    else
    {
      if (checkPause("\nThat exit has no description.\n", numbLines))
        return;

      if (!worldType && checkPause("\n", numbLines))
          return;
    }

   // display door type, if any

    if (worldType)
    {
      strcpy(strn, "[");

      if ((worldType & 3) == 3)
      {
        strcat(strn, "&+Rclosable&+L/&+Rlockable&+L/&+Runpickable&+L/");
      }
      else
      if (worldType & 1)
      {
        strcat(strn, "&+cclosable&+L/");
      }
      else
      if (worldType & 2)
      {
        strcat(strn, "&+Cclosable&+L/&+Clockable&+L/");
      }

      if (worldType & 4)
      {
        strcat(strn, "&+Lsecret/");
      }

      if (worldType & 8)
      {
        strcat(strn, "&+rblocked&+L/");
      }

     // remove trailing slash, add pipe seperator

      strn[strlen(strn) - 1] = '\0';
      strcat(strn, "&n | ");

     // display zone state

      if (zoneState & 1)
      {
        strcat(strn, "&+cclosed&+L/");
      }
      else
      if (zoneState & 2)
      {
        strcat(strn, "&+Cclosed&+L/&+Clocked&+L/");
      }

     // only show 'open' if exit has a door and isn't blocked

      else 
      if ((worldType & 3) && (~zoneState & 8))
      {
        strcat(strn, "&+Wopen&+L/");
      }

      if (zoneState & 4)
      {
        strcat(strn, "&+Lsecret&+L/");
      }

      if (zoneState & 8)
      {
        strcat(strn, "&+rblocked&+L/");
      }

     // remove trailing slash, add closing bracket

      strn[strlen(strn) - 1] = '\0';

      strcat(strn, "&n]\n\n");

     // lockable door (type 2 or 3), check for key

      if (worldType & 2)
      {
       // key = -1, no key works

        if (exitNode->keyNumb == -1)
        {
          strcat(strn, "No key can unlock this door (keyNumb = -1).\n\n");
        }
        else

       // key = -2, magic door (say a word to open it)

        if (exitNode->keyNumb == -2)
        {
          strnNode = exitNode->keywordListHead;

          if (!strnNode)
          {
            strcat(strn,
            "ERROR: Door key number is -2 (magic word door), but there are no exit keywords.\n\n");
          }
          else
          {
           // magic word to open door is last keyword

            while (strnNode->Next)
              strnNode = strnNode->Next;

            sprintf(strn + strlen(strn),
            "Door is magical - the magic word \"%s\" will open it.\n\n",
                    strnNode->string);
          }
        }
        else

       // normal key vnum, find key and display name

        {
          keyObj = findObj(exitNode->keyNumb);

          if (!keyObj && getVnumCheckVal())
          {
            sprintf(strn + strlen(strn), "Key object #%d not found.\n\n", exitNode->keyNumb);
          }
          else
          {
            sprintf(strn + strlen(strn), "Door requires '%s&n' (#%d) to unlock it.\n\n",
                    getObjShortName(keyObj), exitNode->keyNumb);
          }
        }
      }

      if (checkPause(strn, numbLines))
        return;
    }
  }
  else
  {
    if (checkPause("\nThere is no exit in that direction.\n\n", numbLines))
      return;
  }
}


//
// lookInObj : Displays the contents of an object, if it has any
//
//   *strn : string as entered by user - contains keyword
// strnPos : position in strn where keyword starts
//

void lookInObj(const char *args)
{
  char strn[MAX_OBJSNAME_LEN + 128];

  objectHere *objHere, *objInside;

  masterKeywordListNode *matchingNode;
  char whatMatched;

  const stringNode *strnNode;

  size_t numbLines = 0;


 // find item that matches keyword

  strnNode = checkCurrentMasterKeywordList(args, &whatMatched, &matchingNode);

 // no match

  if (whatMatched == NO_MATCH)
  {
    _outtext("\nLook in what?\n\n");

    return;
  }

 // match, but not an object

  if (whatMatched != ENTITY_OBJECT)
  {
    _outtext("\n"
"You can't look inside that (the match for the keyword isn't an object).\n\n");

    return;
  }

 // check for objects inside of the g_currentRoom that match the keyword

  objHere = (objectHere *)(matchingNode->entityPtr);

 // if object has a defined type, check container-ness, otherwise assume it's a container

  if (objHere->objectPtr && (objHere->objectPtr->objType != ITEM_CONTAINER))
  {
    _outtext("\n");
    displayColorString(objHere->objectPtr->objShortName);
    displayColorString("&n is not a container.\n\n");

    return;
  }

 // display contents

  _outtext("\n");

  if (objHere->objectPtr)
  {
    strcpy(strn, objHere->objectPtr->objShortName);
  }
  else
  {
    sprintf(strn, "(object not in this .obj file) (#%u)",
            objHere->objectNumb);
  }

  objInside = objHere->objInside;

 // container has contents

  if (objInside)
  {
    strcat(strn, "&n contains:\n");

    if (checkPause(strn, numbLines) ||
        displayContainerContents(objInside, 0, numbLines))
      return;

    _outtext("\n");
  }
  else

 // container is empty

  {
    strcat(strn, "&n is empty.\n\n");

    checkPause(strn, numbLines);
  }
}


//
// displayCurrentRoom : Display the current room title, flags, description, and exits
//

void displayCurrentRoom(void)
{
  const objectHere *objPtr;
  const mobHere *mobPtr;
  char strn[2048], strn2[2048];
  bool foundExit = false;
  bool map;
  size_t numbLines = 0;


  map = g_zoneRec.miscBits.zoneMiscBits.map;

  if (map)
    displayMap();
  else
    _outtext("\n");

 // display room title with color

  if (getShowRoomVnumVal())
    sprintf(strn, "%s&n (#%u)\n", g_currentRoom->roomName, g_currentRoom->roomNumber);
  else
    sprintf(strn, "%s&n\n", g_currentRoom->roomName);

  if (checkPause(strn, numbLines))
    return;


 // display room flags and sector info

  if (getShowRoomExtraVal()) 
    if (displayRoomFlagsandSector(numbLines))
      return;


 // display room description

  if (!(g_currentRoom->roomDescHead))
  {
    if (!map) 
    {
      if (checkPause("&n[[[ This room has no description ]]]\n", numbLines))
        return;
    }
  }
  else 
  {
    if (displayStringNodesPause(g_currentRoom->roomDescHead, numbLines))
      return;
  }


 // now, display exits

  strcpy(strn, "&+gObvious exits: ");

  for (uint i = 0; i < NUMB_EXITS; i++)
  {
    if (g_currentRoom->exits[i])
    {
      strncpy(strn2, g_exitnames[i], 2047);
      strn2[2047] = '\0';

      strn2[0] = toupper(strn2[0]);

      sprintf(strn + strlen(strn), "&+c-%s ", strn2);

      if (getShowExitFlagsVal()) 
      {
        strcat(strn, getDoorStateStrn(g_currentRoom->exits[i], strn2));
      }

      if (getShowExitDestVal()) 
      {
        strcat(strn, getExitDestStrn(g_currentRoom->exits[i], strn2));
      }

      foundExit = true;
    }
  }

  if (!foundExit)
    strcat(strn, "&+cNone!");

  strcat(strn, "&n\n");

  if (checkPause(strn, numbLines))
    return;

 // list objects in room

  if (g_currentRoom->objectHead)
  {
    objPtr = g_currentRoom->objectHead;

    while (objPtr)
    {
      if (objPtr->objectPtr)
      {
        const objectType *objType = objPtr->objectPtr;

        strn2[0] = '\0';

        if (getShowObjFlagsVal())
        {
          const uint extraFlags = objType->extraBits;
          const uint extra2Flags = objType->extra2Bits;

          if (extraFlags & ITEM_GLOW)       strcat(strn2, "&+MG");
          if (extraFlags & ITEM_HUM)        strcat(strn2, "&+rH");
          if (extraFlags & ITEM_NOSHOW)     strcat(strn2, "&+cN");
          if (extraFlags & ITEM_BURIED)     strcat(strn2, "&+LB");
          if (extraFlags & ITEM_INVISIBLE)  strcat(strn2, "&+LI");
          if (extraFlags & ITEM_TRANSIENT)  strcat(strn2, "&+RT");
          if (extra2Flags & ITEM2_MAGIC)    strcat(strn2, "&+bM");
          if (extraFlags & ITEM_SECRET)     strcat(strn2, "&+gS");
          if (extraFlags & ITEM_LIT)        strcat(strn2, "&+WL");
          if (extraFlags & ITEM_FLOAT)      strcat(strn2, "&+yF");

          if (strlen(strn2)) 
            strcat(strn2, "&n]");
        }

        sprintf(strn, "%s%s&n", strn2, objType->objLongName);

        if (getShowObjVnumVal())
        {
          sprintf(strn2, " (#%u)", objPtr->objectNumb);
          strcat(strn, strn2);
        }
      }
      else
      {
        sprintf(strn,
"An object of a type not in this .obj file is here. (#%u)",
                objPtr->objectNumb);
      }

      sprintf(strn + strlen(strn), " &+c(%u%%)&n\n", objPtr->randomChance);

      if (checkPause(strn, numbLines))
        return;

      objPtr = objPtr->Next;
    }
  }

 // check for mobs in room

  if (g_currentRoom->mobHead)
  {
    mobPtr = g_currentRoom->mobHead;

    while (mobPtr)
    {
      if (mobPtr->mobPtr)
      {
        const mobType *mobT = mobPtr->mobPtr;

        strn[0] = '\0';

        if (getShowMobFlagsVal())
        {
          const uint affect1Flags = mobT->affect1Bits;
          const uint actionFlags = mobT->actionBits;
          const uint aggroFlags = mobT->aggroBits;

          if (mobT->questPtr) strcat(strn, "&+GQ");
          if (mobT->shopPtr)  strcat(strn, "&+MSh");

          if (affect1Flags & AFF_INVISIBLE)    strcat(strn, "&+LI");
          if (affect1Flags & AFF_HIDE)         strcat(strn, "&+rH");
          if (actionFlags & ACT_MEMORY)        strcat(strn, "&+CM");
          if (actionFlags & ACT_SENTINEL)      strcat(strn, "&+WS");
          if (actionFlags & ACT_STAY_ZONE)     strcat(strn, "&+YZ");
          if (actionFlags & ACT_PROTECTOR)     strcat(strn, "&+mP");
          if (actionFlags & ACT_HUNTER)        strcat(strn, "&+RHu");
          if (actionFlags & ACT_SCAVENGER)     strcat(strn, "&+ySc");
          if (actionFlags & ACT_TEACHER)       strcat(strn, "&+YT");
          if (actionFlags & ACT_WIMPY)         strcat(strn, "&+gW");
          if (actionFlags & ACT_GUILD_GOLEM)   strcat(strn, "&+WG");
          if (aggroFlags & AGGR_ALL)           strcat(strn, "&+RA");
          if (aggroFlags & AGGR_EVIL_ALIGN)    strcat(strn, "&+RaE");
          if (aggroFlags & AGGR_GOOD_ALIGN)    strcat(strn, "&+RaG");
          if (aggroFlags & AGGR_NEUTRAL_ALIGN) strcat(strn, "&+RaN");
          if (aggroFlags & AGGR_EVIL_RACE)     strcat(strn, "&+RaEr");
          if (aggroFlags & AGGR_GOOD_RACE)     strcat(strn, "&+RaGr");
          if (aggroFlags & AGGR_OUTCASTS)      strcat(strn, "&+RaO");

          if (strlen(strn)) strcat(strn, "&n]");
        }

        strcat(strn, mobT->mobLongName);

        if (getShowMobPosVal())
        {
          strcat(strn, "&n [&+c");

          switch (mobT->defaultPos)
          {
            case POSITION_SLEEPING  : strcat(strn, "Sl");  break;
            case POSITION_RESTING   : strcat(strn, "R");  break;
            case POSITION_SITTING   : strcat(strn, "Si");  break;
            case POSITION_STANDING  : strcat(strn, "St");  break;
            case POSITION_LEVITATED : strcat(strn, "L");  break;
            case POSITION_FLYING    : strcat(strn, "F");  break;
            case POSITION_SWIMMING  : strcat(strn, "Sw");  break;
            default : strcat(strn, "&+RXX");  break;
          }

          strcat(strn, "&n]");
        }

        strcpy(strn2, strn);
        if (getShowMobVnumVal())
        {
          sprintf(strn, " (#%u)", mobPtr->mobNumb);
          strcat(strn2, strn);
        }
      }
      else
      {
        sprintf(strn2,
"A mob of a type not in this .mob file is here. (#%u)",
                mobPtr->mobNumb);
      }

      sprintf(strn, " &+c(%u%%)&n", mobPtr->randomChance);
      strcat(strn2, strn);

      if (mobPtr->riding && getShowMobRideFollowVal())
      {
        sprintf(strn, " (riding %s&n (#%u))", 
                getMobShortName(mobPtr->riding->mobPtr), mobPtr->riding->mobNumb);

        strcat(strn2, strn);
      }

      if (mobPtr->riddenBy && getShowMobRideFollowVal())
      {
        sprintf(strn, " (ridden by %s&n (#%u))", 
                getMobShortName(mobPtr->riddenBy->mobPtr), mobPtr->riddenBy->mobNumb);

        strcat(strn2, strn);
      }

      if (getShowMobRideFollowVal())
      {
/*   // ehh this kinda sucks, maybe readd it as a toggleable option
        i = getNumbFollowers(mobPtr);
        if (i)
        {
          sprintf(strn, " (followed by %u mob%s)",
                  i, plural(i));
          strcat(strn2, strn);
        }
*/

        if (mobPtr->following)
        {
          sprintf(strn, " (following %s&n (#%u))", 
                  getMobShortName(mobPtr->following->mobPtr), mobPtr->following->mobNumb);

          strcat(strn2, strn);
        }
      }

      strcat(strn2, "\n");
      if (checkPause(strn2, numbLines))
        return;

      mobPtr = mobPtr->Next;
    }
  }

  checkPause("\n", numbLines);
}


//
// look : Checks args to look command for certain stuff
//
//  *args : arguments to 'look' command
//

void look(const char *args)
{
  char exitval;


  if (!strlen(args))
  {
    displayCurrentRoom();

    return;
  }

  exitval = getDirfromKeyword(args);
  if (exitval != NO_EXIT)
  {
    lookExit(g_currentRoom->exits[exitval]);
    return;
  }

  if (strlefti(args, "IN "))
  {
   // get rid of 'in '

    args += 3;

    lookInObj(args);
  }
  else
  {
    lookEntity(args);
  }
}
