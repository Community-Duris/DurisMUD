//
//  File: misccomm.cpp   originally part of durisEdit
//
//  Usage: command dispatchers for all non-main command lists
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
#include <stdlib.h>

#include "../types.h"
#include "../fh.h"
#include "../keys.h"

#include "../room/room.h"

#include "misccomm.h"

extern room *g_currentRoom, *g_defaultRoom;
extern roomExit *g_defaultExit;
extern objectType *g_defaultObject;
extern mobType *g_defaultMob;
extern extraDesc *g_defaultExtraDesc;
extern quest *g_defaultQuest;
extern shop *g_defaultShop;
extern zone g_zoneRec;
extern command g_copyDescCommands[], g_copyCommands[], g_copyDefaultCommands[], g_mainCommands[];
extern bool g_madeChanges;
extern const char *g_exitnames[];
extern alias *g_aliasHead;
extern variable *g_varHead;

//
// lookupExecCommand
//

bool lookupExecCommand(const usint command, const char *args)
{
  switch (command)
  {
    case LOOKUPCMD_ROOM   : displayRoomList(args); break;
    case LOOKUPCMD_OBJECT : displayObjectTypeList(args, false); break;
    case LOOKUPCMD_MOB    : displayMobTypeList(args); break;
  }

  return false;
}


//
// createExecCommand
//

bool createExecCommand(const usint command, const char *args)
{
  switch (command)
  {

   // allow these funcs to accept vnum args

    case CREATECMD_ROOM    : createRoomPrompt(args); break;
    case CREATECMD_OBJECT  : createObjectTypeUser(args); break;
    case CREATECMD_OBJHERE : createObjectHereStrn(args); break;
    case CREATECMD_MOB     : createMobTypeUser(args); break;
    case CREATECMD_MOBHERE : createMobHereStrn(args); break;
    case CREATECMD_EXIT    : preCreateExit(args); break;
    case CREATECMD_ALIAS   : aliasCmd(args, true, &g_aliasHead); break;
    case CREATECMD_VARIABLE : varCmd(args, true, &g_varHead); break;
  }

  return false;
}


//
// loadExecCommand
//

bool loadExecCommand(const usint command, const char *args)
{
  switch (command)
  {

   // allow these funcs to accept vnum args

    case LOADCMD_OBJECT : createObjectHereStrn(args); break;
    case LOADCMD_MOB    : createMobHereStrn(args); break;
  }

  return false;
}


//
// editExecCommand
//

bool editExecCommand(const usint command, const char *args)
{
  switch (command)
  {
    case EDITCMD_ROOM     : preEditRoom(args); break;
    case EDITCMD_OBJECT   : editObjectTypeStrn(args); break;
    case EDITCMD_MOB      : editMobTypeStrn(args); break;
    case EDITCMD_QUEST    : editQuestStrn(args); break;
    case EDITCMD_SHOP     : editShopStrn(args); break;
    case EDITCMD_EXIT     : preEditExit(args); break;
    case EDITCMD_SECTOR   : editSector(); break;
    case EDITCMD_ZONE     : editZone(&g_zoneRec); break;
    case EDITCMD_DEFROOM  : editDefaultRoom(); break;
    case EDITCMD_DEFOBJ   : editDefaultObject(); break;
    case EDITCMD_DEFMOB   : editDefaultMob(); break;
    case EDITCMD_DEFEXIT  : editDefaultExit(); break;
    case EDITCMD_DEFEDESC : editDefaultExtraDesc(); break;
    case EDITCMD_DEFQUEST : editDefaultQuest(); break;
    case EDITCMD_DEFSHOP  : editDefaultShop(); break;
    case EDITCMD_DESC     : editDesc(&g_currentRoom->roomDescHead);
                            displayCurrentRoom(); break;
  }

  return false;
}


//
// listExecCommand
//

bool listExecCommand(const usint command, const char *args)
{
  switch (command)
  {
    case LISTCMD_ROOM    : displayRoomList(args); break;
    case LISTCMD_EXIT    : displayExitList(args); break;
    case LISTCMD_OBJECT  : displayObjectTypeList(args, false); break;
    case LISTCMD_MOB     : displayMobTypeList(args); break;
    case LISTCMD_QUEST   : displayQuestList(args); break;
    case LISTCMD_SHOP    : displayShopList(args); break;
    case LISTCMD_OBJHERE : displayObjectHereList(); break;
    case LISTCMD_MOBHERE : displayMobHereList(); break;
    case LISTCMD_LOADED  : displayLoadedList(args); break;
    case LISTCMD_LOOKUP  : preDisplayLookupList(args); break;
    case LISTCMD_ALIAS   : aliasCmd("", true, &g_aliasHead); break;
    case LISTCMD_VARIABLE : varCmd("", true, &g_varHead); break;
    case LISTCMD_BOOLEAN : toggleVar(&g_varHead, ""); break;
    case LISTCMD_TEMPLATE : setTemplateArgs("", false, true); break;
    case LISTCMD_COMMANDS : displayCommands(g_mainCommands); break;
  }

  return false;
}


//
// createEditExecCommand
//

bool createEditExecCommand(const usint command, const char *args)
{
  objectType *obj;
  mobType *mob;
  char strn[256];


  switch (command)
  {
    case CRTEDITCMD_ROOM   : if (strlen(args))
                             {
                               if (createRoomPrompt(args)) 
                                 preEditRoom(args);
                             }
                             else
                             {
                               sprintf(strn, "%u", getFirstFreeRoomNumber());

                               if (createRoomPrompt(strn)) 
                                 preEditRoom(strn);
                             }

                             break;

    case CRTEDITCMD_OBJECT : obj = createObjectTypeUser(args);
                             if (obj)
                               editObjType(obj, true);
                             break;

    case CRTEDITCMD_MOB    : mob = createMobTypeUser(args);
                             if (mob)
                               editMobType(mob, true);
                             break;

    case CRTEDITCMD_EXIT   : createEditExit(args); 
                             break;
  }

  return false;
}


//
// deleteExecCommand
//

bool deleteExecCommand(const usint command, const char *args)
{
  switch (command)
  {
    case DELETECMD_ROOM      : deleteRoomUser(args); break;
    case DELETECMD_EXIT      : preDeleteExitPrompt(args); break;
    case DELETECMD_OBJECT    : deleteObjectTypeUser(args); break;
    case DELETECMD_MOB       : deleteMobTypeUser(args); break;
    case DELETECMD_QUEST     : deleteQuestUser(args); break;
    case DELETECMD_SHOP      : deleteShopUser(args); break;
    case DELETECMD_UNUSED    : deleteUnusedObjMobTypes(); break;

    case DELETECMD_OBJHERE   : deleteObjectHereUser(args); break;
    case DELETECMD_MOBHERE   : deleteMobHereUser(args); break;

    case DELETECMD_ALIAS     : unaliasCmd(args, &g_aliasHead); break;
    case DELETECMD_VARIABLE  : unvarCmd(args, &g_varHead); break;

    case DELETECMD_DEFROOM   : deleteDefaultRoom(); break;
    case DELETECMD_DEFOBJECT : deleteDefaultObject(); break;
    case DELETECMD_DEFMOB    : deleteDefaultMob(); break;
    case DELETECMD_DEFEXIT   : deleteDefaultExit(); break;
    case DELETECMD_DEFEDESC  : deleteDefaultExtraDesc(); break;
    case DELETECMD_DEFQUEST  : deleteDefaultQuest(); break;
    case DELETECMD_DEFSHOP   : deleteDefaultShop(); break;
  }

  return false;
}


//
// purgeExecCommand
//

bool purgeExecCommand(const usint command, const char *args)
{
  switch (command)
  {
    case PURGECMD_ALL    : purgeAll(); break;
    case PURGECMD_ALLMOB : purgeAllMob(); break;
    case PURGECMD_ALLOBJ : purgeAllObj(); break;
    case PURGECMD_INV    : purgeInv(); break;
  }

  return false;
}


//
// statExecCommand
//

bool statExecCommand(const usint command, const char *args)
{
  switch (command)
  {
    case STATCMD_ROOM   : displayRoomInfo(args); break;
    case STATCMD_OBJECT : displayObjectInfo(args); break;
    case STATCMD_MOB    : displayMobInfo(args); break;
    case STATCMD_ZONE   : displayZoneInfo(); break;
  }

  return false;
}


//
// setLimitExecCommandStartup
//

bool setLimitExecCommandStartup(const usint command, const char *args)
{
  switch (command)
  {
    case SETLIMCMD_OBJECT : setLimitOverrideObj(args, false, false); break;
    case SETLIMCMD_MOB    : setLimitOverrideMob(args, false, false); break;
  }

  return false;
}


//
// setLimitExecCommand
//

bool setLimitExecCommand(const usint command, const char *args)
{
  switch (command)
  {
    case SETLIMCMD_OBJECT : setLimitOverrideObj(args, true, true); break;
    case SETLIMCMD_MOB    : setLimitOverrideMob(args, true, true); break;
  }

  return false;
}


//
// cloneExecCommand
//

bool cloneExecCommand(const usint command, const char *args)
{
  uint vnum;
  int toclone = -1;
  char arg1[256], arg2[256];


  if (!strlen(args))
  {
    _outtext("\n"
"Specify a vnum and optional clone amount as the second and third arguments.\n\n");
    return false;
  }

  getArg(args, 1, arg1, 255);
  getArg(args, 2, arg2, 255);

  if (arg2[0] == '\0')  // no second arg
  {
    if (!strnumber(arg1))
    {
      _outtext("\nThe second argument should specify the vnum to clone.\n\n");
      return false;
    }

    toclone = 1;
  }
  else
  {
    if (!strnumber(arg1) || !strnumber(arg2))
    {
      _outtext("\n"
"The second and third arguments should specify the vnum and amount to clone.\n\n");
      return false;
    }
  }

  vnum = strtoul(arg1, NULL, 10);

  if (toclone == -1) 
    toclone = atoi(arg2);

  switch (command)
  {
    case CLONECMD_ROOM   : cloneRoom(vnum, toclone); break;
    case CLONECMD_OBJECT : cloneObjectType(vnum, toclone); break;
    case CLONECMD_MOB    : cloneMobType(vnum, toclone); break;
  }

  return false;
}


//
// copyDescExecCommand
//

bool copyDescExecCommand(const usint command, const char *args)
{
  uint tocopy, copyto;
  char arg1[1024], arg2[1024];


  if (!strlen(args))
  {
    _outtext("\n"
"Specify a vnum to be copied and a vnum to copy to (optional for rooms) as the\n"
"third and fourth arguments.\n\n");

    return false;
  }

  getArg(args, 1, arg1, 1023);
  getArg(args, 2, arg2, 1023);

  if (arg2[0] == '\0')  // no second arg
  {
    if (command != COPYDESCCMD_ROOM)
    {
      _outtext("\nWhen copying mob descs, a target must be specified.\n\n");
      return false;
    }

    sprintf(arg2, "%u", g_currentRoom->roomNumber);
  }

  if (!strnumber(arg1) || !strnumber(arg2))
  {
    _outtext("\n"
"The third and fourth arguments should specify the vnum of the desc to be copied\n"
"and the vnum of the entity to copy the desc to, respectively.\n\n");

    return false;
  }

  tocopy = strtoul(arg1, NULL, 10);
  copyto = strtoul(arg2, NULL, 10);

  switch (command)
  {
    case COPYDESCCMD_ROOM   : copyRoomDesc(tocopy, copyto); break;
    case COPYDESCCMD_MOB    : copyMobDesc(tocopy, copyto); break;
  }

  return false;
}


#define COPYDEF_NOT_ENOUGH_ARGS_MSG   "\n" \
"Specify <room|object|mob|exit|edesc|quest|shop> as the second arg and a vnum to\n" \
"be copied (optional for rooms, exit direction for exits, keywords for edescs)\n" \
"to the default as the third argument.\n\n"

//
// copyDefaultExecCommand
//

bool copyDefaultExecCommand(const usint command, const char *args)
{
  char outstrn[512];
  promptType copyExitChoice;
  uint tocopy;
  int val;
  room *room;
  objectType *obj;
  mobType *mob;


  if (!strlen(args) && (command == COPYDEFCMD_ROOM))
  {
    tocopy = g_currentRoom->roomNumber;
  }
  else
  if (!strlen(args) || (!strnumber(args) && (command != COPYDEFCMD_EXIT) && (command != COPYDEFCMD_EDESC)))
  {
    _outtext(COPYDEF_NOT_ENOUGH_ARGS_MSG);

    return false;
  }
  else
  {
    tocopy = strtoul(args, NULL, 10);
  }

  switch (command)
  {
    case COPYDEFCMD_ROOM   :
      room = findRoom(tocopy);

      if (!room)
      {
        _outtext("\n"
"Room with vnum specified to copy not found.\n\n");
        break;
      }

      copyExitChoice = 
        displayYesNoPrompt("\n&+cCopy any existing exits into the default room", promptNo, false);

      if (g_defaultRoom)
      {
        deleteRoomInfo(g_defaultRoom, false, false);
      }

      g_defaultRoom = copyRoomInfo(room, false);

     // if user doesn't want to copy exits, delete them

      if (copyExitChoice == promptNo)
        deleteAllExits(g_defaultRoom, false);

      deleteObjHereList(g_defaultRoom->objectHead, false);
      g_defaultRoom->objectHead = NULL;

      deleteMobHereList(g_defaultRoom->mobHead, false);
      g_defaultRoom->mobHead = NULL;

      deleteMasterKeywordList(g_defaultRoom->masterListHead);
      g_defaultRoom->masterListHead = NULL;

      deleteEditableList(g_defaultRoom->editableListHead);
      g_defaultRoom->editableListHead = NULL;

      g_defaultRoom->defaultRoom = true;
      g_defaultRoom->roomNumber = 0;

      sprintf(outstrn, "\nSettings for room #%u copied into default room.\n\n", tocopy);
      _outtext(outstrn);

      g_madeChanges = true;

      break;

    case COPYDEFCMD_EXIT :
      val = getDirfromKeyword(args);
      if (val == NO_EXIT)
      {
        _outtext("\nCopy which exit?\n\n");
        return false;
      }

      if (g_currentRoom->exits[val])
      {
        char strn[256];

        deleteRoomExit(g_defaultExit, false);
        g_defaultExit = copyRoomExit(g_currentRoom->exits[val], false);

        strncpy(strn, g_exitnames[val], 255);
        strn[255] = '\0';

        strn[0] = toupper(strn[0]);

        sprintf(outstrn, "\n%s exit of current room copied into default exit.\n\n",
                strn);
        _outtext(outstrn);

        g_madeChanges = true;

        break;
      }
      else
      {
        _outtext("\nNo exit in that direction to copy.\n\n");
        break;
      }

    case COPYDEFCMD_EDESC :
      char whatMatched;
      editableListNode *matchingNode;

      checkEditableList(args, g_currentRoom->editableListHead, &whatMatched, &matchingNode, 1);

      switch (whatMatched)
      {
        case NO_MATCH :
          _outtext("\nNo match on third argument (extra desc keyword).\n\n");
          return false;

        case ENTITY_O_EDESC :
        case ENTITY_R_EDESC :
          if (g_defaultExtraDesc)
            deleteOneExtraDesc(g_defaultExtraDesc);

          g_defaultExtraDesc = copyOneExtraDesc((extraDesc *)(matchingNode->entityPtr));

          char outstrn[1536], keystrn[1024];

          getReadableKeywordStrn(g_defaultExtraDesc->keywordListHead, keystrn, 1023);

          sprintf(outstrn, "\nExtra desc '%s' copied into default.\n\n", keystrn);

          _outtext(outstrn);

          return false;

        default :
          const char *match = getEntityTypeStrn(whatMatched);

          sprintf(outstrn, "\nThat target is not an extra desc - it is a%s %s.\n\n",
                  getVowelN(match[0]), match);

          _outtext(outstrn);

          return false;
      }

      break;

    case COPYDEFCMD_OBJECT :
      obj = findObj(tocopy);

      if (!obj)
      {
        _outtext("\n"
"Object type with vnum specified to copy not found.\n\n");
        break;
      }

      if (g_defaultObject)
        deleteObjectType(g_defaultObject, false);

      g_defaultObject = copyObjectType(obj, false);

      g_defaultObject->defaultObj = true;
      g_defaultObject->objNumber = 0;

      sprintf(outstrn, "\n"
"Settings for object #%u copied into default object.\n\n",
              tocopy);
      _outtext(outstrn);

      g_madeChanges = true;

      break;

    case COPYDEFCMD_MOB    :
      mob = findMob(tocopy);

      if (!mob)
      {
        _outtext("\n"
"Mob type with vnum specified to copy not found.\n\n");
        break;
      }

      if (g_defaultMob)
        deleteMobType(g_defaultMob, false);

      g_defaultMob = copyMobType(mob, false);

      g_defaultMob->defaultMob = true;
      g_defaultMob->mobNumber = 0;

      sprintf(outstrn, "\n"
"Settings for mob #%u copied into default mob.\n\n",
              tocopy);
      _outtext(outstrn);

      g_madeChanges = true;

      break;

    case COPYDEFCMD_QUEST  :
      mob = findMob(tocopy);

      if (!mob)
      {
        _outtext("\n"
"Mob type with vnum specified to copy not found.\n\n");
        break;
      }

      if (!mob->questPtr)
      {
        _outtext("\n"
"The specified mob type does not have a quest.\n\n");
        break;
      }

      if (g_defaultQuest)
        deleteQuest(g_defaultQuest, false);

      g_defaultQuest = copyQuest(mob->questPtr);

      sprintf(outstrn, "\n"
"Settings for quest #%u copied into default quest.\n\n",
              tocopy);
      _outtext(outstrn);

      g_madeChanges = true;

      break;

    case COPYDEFCMD_SHOP   :
      mob = findMob(tocopy);

      if (!mob)
      {
        _outtext("\n"
"Mob type with vnum specified to copy not found.\n\n");
        break;
      }

      if (!mob->shopPtr)
      {
        _outtext("\n"
"The specified mob type does not have a shop.\n\n");
        break;
      }

      if (g_defaultShop)
        deleteShop(g_defaultShop, false);

      g_defaultShop = copyShop(mob->shopPtr);

      sprintf(outstrn, "\n"
"Settings for shop #%u copied into default shop.\n\n",
              tocopy);
      _outtext(outstrn);

      g_madeChanges = true;

      break;
  }

  return false;
}


//
// copyExecCommand
//

bool copyExecCommand(const usint command, const char *args)
{
  switch (command)
  {
   // allow these funcs to accept vnum args

    case COPYCMD_DEFAULT :
       checkCommands(args, g_copyDefaultCommands, COPYDEF_NOT_ENOUGH_ARGS_MSG,
                     copyDefaultExecCommand, NULL, NULL); 
       break;

    case COPYCMD_DESC    :
       checkCommands(args, g_copyDescCommands, "\n"
"Specify one of <room|mob> as the second argument.\n\n",
                     copyDescExecCommand, NULL, NULL); 
       break;
  }

  return false;
}


//
// copyCommand
//

void copyCommand(const char *args)
{
  checkCommands(args, g_copyCommands, "\nSpecify one of DEFAULT or DESC as the first argument.\n\n",
                copyExecCommand, NULL, NULL);
}
