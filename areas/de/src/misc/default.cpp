//
//  File: default.cpp    originally part of durisEdit
//
//  Usage: functions related to default rooms, exits, etc
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

#include <ctype.h>

#include "../fh.h"
#include "../types.h"
#include "../system.h"
#include "../keys.h"
#include "../room/room.h"
#include "../obj/object.h"
#include "../mob/mob.h"



extern room *g_defaultRoom;
extern objectType *g_defaultObject;
extern mobType *g_defaultMob;
extern roomExit *g_defaultExit;
extern extraDesc *g_defaultExtraDesc;
extern quest *g_defaultQuest;
extern shop *g_defaultShop;
extern bool g_madeChanges;

//
// readDefaultsFromFiles
//

void readDefaultsFromFiles(const char *filePrefix)
{
  FILE *defaultFile;
  char strn[1024] = "", fileName[512], *retVal;
     // dummy strn for reading obj


  _outtext("\n");

  sprintf(fileName, "%s%s", filePrefix, DEF_WLD_EXT);

 // read default room first

  defaultFile = fopen(fileName, "rt");

  if (defaultFile)
  {
    _outtext("Reading room default in ");
    _outtext(fileName);
    _outtext("...\n");

    g_defaultRoom = readRoomFromFile(defaultFile, false, false);

    g_defaultRoom->roomNumber = 0;
    g_defaultRoom->defaultRoom = true;

    fclose(defaultFile);
  }

 // read default object

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_OBJ_EXT);

  defaultFile = fopen(fileName, "rt");

  if (defaultFile)
  {
    _outtext("Reading object default in ");
    _outtext(fileName);
    _outtext("...\n");

    g_defaultObject = readObjectFromFile(defaultFile, strn, false, false);

    g_defaultObject->objNumber = 0;
    g_defaultObject->defaultObj = true;

    fclose(defaultFile);
  }

 // read default mob

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_MOB_EXT);

  defaultFile = fopen(fileName, "rt");

  if (defaultFile)
  {
    _outtext("Reading mob default in ");
    _outtext(fileName);
    _outtext("...\n");

    g_defaultMob = readMobFromFile(defaultFile, false, false);

    g_defaultMob->mobNumber = 0;
    g_defaultMob->defaultMob = true;

    fclose(defaultFile);
  }

 // read default exit

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_EXIT_EXT);

  defaultFile = fopen(fileName, "rt");

  if (defaultFile)
  {
    _outtext("Reading exit default in ");
    _outtext(fileName);
    _outtext("...\n");

    retVal = fgets(strn, 256, defaultFile);
    nolf(strn);
    g_defaultExit = readRoomExitFromFile(defaultFile, NULL, strn, false);

    fclose(defaultFile);
  }

 // read default extra desc

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_EDESC_EXT);

  defaultFile = fopen(fileName, "rt");

  if (defaultFile)
  {
    _outtext("Reading extra desc default in ");
    _outtext(fileName);
    _outtext("...\n");

    g_defaultExtraDesc = readExtraDescFromFile(defaultFile, ENTITY_R_EDESC, 0, ENTITY_R_EDESC, NULL);

    fclose(defaultFile);
  }

 // read default quest

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_QUEST_EXT);

  defaultFile = fopen(fileName, "rt");

  if (defaultFile)
  {
    _outtext("Reading quest default in ");
    _outtext(fileName);
    _outtext("...\n");

    g_defaultQuest = readQuestFromFile(defaultFile, NULL);

    fclose(defaultFile);
  }

 // read default shop

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_SHOP_EXT);

  defaultFile = fopen(fileName, "rt");

  if (defaultFile)
  {
    _outtext("Reading shop default in ");
    _outtext(fileName);
    _outtext("...\n");

    g_defaultShop = readShopFromFile(defaultFile, true);

    fclose(defaultFile);
  }
}


//
// writeDefaultFiles
//

void writeDefaultFiles(const char *filePrefix)
{
  FILE *defaultFile;
  char fileName[512], strn[1024];


 // default room

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_WLD_EXT);

  if (g_defaultRoom)
  {
    sprintf(strn, "Writing room default to %s...\n", fileName);
    _outtext(strn);

    defaultFile = fopen(fileName, "wt");

    writeRoomtoFile(defaultFile, g_defaultRoom);

    fclose(defaultFile);
  }
  else  // delete old default, if any
  {
    deleteFile(fileName);
  }


 // default object

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_OBJ_EXT);

  if (g_defaultObject)
  {
    sprintf(strn, "Writing object default to %s...\n", fileName);
    _outtext(strn);

    defaultFile = fopen(fileName, "wt");

    writeObjecttoFile(defaultFile, g_defaultObject);

    fclose(defaultFile);
  }
  else  // delete old default, if any
  {
    deleteFile(fileName);
  }


 // default mob

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_MOB_EXT);

  if (g_defaultMob)
  {
    sprintf(strn, "Writing mob default to %s...\n", fileName);
    _outtext(strn);

    defaultFile = fopen(fileName, "wt");

    writeMobtoFile(defaultFile, g_defaultMob);

    fclose(defaultFile);
  }
  else  // delete old default, if any
  {
    deleteFile(fileName);
  }


 // default exit

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_EXIT_EXT);

  if (g_defaultExit)
  {
    sprintf(strn, "Writing exit default to %s...\n", fileName);
    _outtext(strn);

    defaultFile = fopen(fileName, "wt");

    writeWorldExittoFile(defaultFile, g_defaultExit, "D0\n");

    fclose(defaultFile);
  }
  else  // delete old default, if any
  {
    deleteFile(fileName);
  }


 // default extra desc

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_EDESC_EXT);

  if (g_defaultExtraDesc)
  {
    sprintf(strn, "Writing extra desc default to %s...\n", fileName);
    _outtext(strn);

    defaultFile = fopen(fileName, "wt");

    writeExtraDesctoFile(defaultFile, g_defaultExtraDesc);

    fclose(defaultFile);
  }
  else  // delete old default, if any
  {
    deleteFile(fileName);
  }


 // default quest

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_QUEST_EXT);

  if (g_defaultQuest)
  {
    sprintf(strn, "Writing quest default to %s...\n", fileName);
    _outtext(strn);

    defaultFile = fopen(fileName, "wt");

    writeQuesttoFile(defaultFile, g_defaultQuest, 0);

    fclose(defaultFile);
  }
  else  // delete old default, if any
  {
    deleteFile(fileName);
  }


 // default shop

  strcpy(fileName, filePrefix);
  strcat(fileName, DEF_SHOP_EXT);

  if (g_defaultShop)
  {
    sprintf(strn, "Writing shop default to %s...\n", fileName);
    _outtext(strn);

    defaultFile = fopen(fileName, "wt");

    writeShoptoFile(defaultFile, g_defaultShop, 0, true);

    fclose(defaultFile);
  }
  else  // delete old default, if any
  {
    deleteFile(fileName);
  }
}


//
// editDefaultRoom
//

void editDefaultRoom(void)
{
  if (!g_defaultRoom) 
  {
    if (displayYesNoPrompt("\n&+cNo default room exists - create one", promptYes, false) == promptYes)
    {
      g_defaultRoom = createRoom(false, 0, false);
      if (!g_defaultRoom)
        return;

      g_defaultRoom->defaultRoom = true;

      g_madeChanges = true;
    }
    else
    {
      return;
    }
  }

  editRoom(g_defaultRoom, false);
}


//
// editDefaultObject
//

void editDefaultObject(void)
{
  if (!g_defaultObject) 
  {
    if (displayYesNoPrompt("\n&+cNo default object exists - create one", promptYes, false) == promptYes)
    {
      g_defaultObject = createObjectType(false, 0, false);
      if (!g_defaultObject)
        return;

      g_defaultObject->defaultObj = true;

      g_madeChanges = true;
    }
    else
    {
      return;
    }
  }

  editObjType(g_defaultObject, false);
}


//
// editDefaultMob
//

void editDefaultMob(void)
{
  if (!g_defaultMob) 
  {
    if (displayYesNoPrompt("\n&+cNo default mob exists - create one", promptYes, false) == promptYes)
    {
      g_defaultMob = createMobType(false, 0, false);
      if (!g_defaultMob)
        return;

      g_defaultMob->defaultMob = true;

      g_madeChanges = true;
    }
    else
    {
      return;
    }
  }

  editMobType(g_defaultMob, false);
}


//
// editDefaultExit
//

void editDefaultExit(void)
{
  if (!g_defaultExit) 
  {
    if (displayYesNoPrompt("\n&+cNo default exit exists - create one", promptYes, false) == promptYes)
    {
      createExit(&g_defaultExit, false);
      if (!g_defaultExit)
        return;

      g_madeChanges = true;
    }
    else
    {
      return;
    }
  }

  room *roomPtr = createRoom(false, 0, false);

  if (!roomPtr) 
    return;

  strcpy(roomPtr->roomName, "(default exit)");

  editExit(roomPtr, &g_defaultExit, "default", true);

  deleteRoomInfo(roomPtr, false, false);
}


//
// editDefaultExtraDesc
//

void editDefaultExtraDesc(void)
{
  if (!g_defaultExtraDesc) 
  {
    if (displayYesNoPrompt("\n&+cNo default extra desc exists - create one", promptYes, false) == promptYes)
    {
      g_defaultExtraDesc = createExtraDesc(NULL, NULL, false);
      if (!g_defaultExtraDesc)
        return;

      g_madeChanges = true;
    }
    else
    {
      return;
    }
  }

  editExtraDesc(g_defaultExtraDesc);
}


//
// editDefaultQuest
//

void editDefaultQuest(void)
{
  if (!g_defaultQuest) 
  {
    if (displayYesNoPrompt("\n&+cNo default quest exists - create one", promptYes, false) == promptYes)
    {
      g_defaultQuest = createQuest();
      if (!g_defaultQuest)
        return;

      g_madeChanges = true;
    }
    else
    {
      return;
    }
  }

  mobType *mob = createMobType(false, 0, false);

  if (!mob)
    return;

  strcpy(mob->mobShortName, "(default quest)");

  mob->questPtr = g_defaultQuest;

  editQuest(mob, false);

  mob->questPtr = NULL;
  deleteMobType(mob, false);
}


//
// editDefaultShop
//

void editDefaultShop(void)
{
  if (!g_defaultShop) 
  {
    if (displayYesNoPrompt("\n&+cNo default shop exists - create one", promptYes, false) == promptYes)
    {
      g_defaultShop = createShop();
      if (!g_defaultShop)
        return;

      g_madeChanges = true;
    }
    else
    {
      return;
    }
  }

  mobType *mob = createMobType(false, 0, false);

  if (!mob)
    return;

  strcpy(mob->mobShortName, "(default shop)");

  mob->shopPtr = g_defaultShop;

  editShop(mob, false);

  mob->shopPtr = NULL;
  deleteMobType(mob, false);
}


//
// deleteDefaultRoom
//

void deleteDefaultRoom(void)
{
  if (g_defaultRoom)
  {
    deleteRoomInfo(g_defaultRoom, false, false);

    g_defaultRoom = NULL;

    g_madeChanges = true;

    displayColorString("\nDefault room deleted.\n\n");
  }
  else
  {
    displayColorString("\nNo default room info to delete.\n\n");
  }
}


//
// deleteDefaultObject
//

void deleteDefaultObject(void)
{
  if (g_defaultObject)
  {
    deleteObjectType(g_defaultObject, false);

    g_defaultObject = NULL;

    g_madeChanges = true;

    displayColorString("\nDefault object deleted.\n\n");
  }
  else
  {
    displayColorString("\nNo default object info to delete.\n\n");
  }
}


//
// deleteDefaultMob
//

void deleteDefaultMob(void)
{
  if (g_defaultMob)
  {
    deleteMobType(g_defaultMob, false);

    g_defaultMob = NULL;

    g_madeChanges = true;

    displayColorString("\nDefault mob deleted.\n\n");
  }
  else
  {
    displayColorString("\nNo default mob info to delete.\n\n");
  }
}


//
// deleteDefaultExit
//

void deleteDefaultExit(void)
{
  if (g_defaultExit)
  {
    deleteRoomExit(g_defaultExit, false);

    g_defaultExit = NULL;

    g_madeChanges = true;

    displayColorString("\nDefault exit deleted.\n\n");
  }
  else
  {
    displayColorString("\nNo default exit info to delete.\n\n");
  }
}


//
// deleteDefaultExtraDesc
//

void deleteDefaultExtraDesc(void)
{
  if (g_defaultExtraDesc)
  {
    deleteOneExtraDesc(g_defaultExtraDesc);

    g_defaultExtraDesc = NULL;

    g_madeChanges = true;

    displayColorString("\nDefault extra desc deleted.\n\n");
  }
  else
  {
    displayColorString("\nNo default extra desc info to delete.\n\n");
  }
}


//
// deleteDefaultQuest
//

void deleteDefaultQuest(void)
{
  if (g_defaultQuest)
  {
    deleteQuest(g_defaultQuest, true);

    g_defaultQuest = NULL;

    displayColorString("\nDefault quest deleted.\n\n");
  }
  else
  {
    displayColorString("\nNo default quest info to delete.\n\n");
  }
}


//
// deleteDefaultShop
//

void deleteDefaultShop(void)
{
  if (g_defaultShop)
  {
    deleteShop(g_defaultShop, true);

    g_defaultShop = NULL;

    displayColorString("\nDefault shop deleted.\n\n");
  }
  else
  {
    displayColorString("\nNo default shop info to delete.\n\n");
  }
}
