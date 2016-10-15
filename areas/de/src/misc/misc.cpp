//
//  File: misc.cpp       originally part of durisEdit
//
//  Usage: miscellaneous functions that fit nowhere else
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

#include <ctype.h>
#ifdef __UNIX__
#  include <unistd.h>
#endif

#include "../types.h"
#include "../fh.h"
#include "../de.h"
#include "../room/room.h"
#include "../obj/object.h"
#include "../mob/mob.h"
#include "../command/alias.h"
#include "../keys.h"
#include "../vardef.h"
#include "../misc/loaded.h"
#include "../misc/misc.h"
#include "../graphcon.h"


extern bool g_madeChanges;
extern room *g_currentRoom;
extern variable *g_varHead;
extern char g_promptString[256];
extern uint g_numbRooms, g_numbExits, g_numbObjTypes, g_numbObjs, g_numbMobTypes, g_numbMobs,
            g_lowestRoomNumber, g_highestRoomNumber, g_lowestObjNumber, g_highestObjNumber,
            g_lowestMobNumber, g_highestMobNumber;
extern bool g_roomOnly, g_objOnly, g_mobOnly, g_readFromSubdirs, g_zoneLoadedWithQuest, g_zoneLoadedWithShop;
extern bool g_inAtCmd;
extern uint g_numbLookupEntries;
extern zone g_zoneRec;
extern room **g_roomLookup;
extern objectType **g_objLookup;
extern mobType **g_mobLookup;
extern command g_lookupCommands[];

extern void updateQuestItems();

//
// getScreenHeight : returns number of lines screen has, as defined by user
//

uint getScreenHeight(void)
{
  return getScreenHeightVal();
}


//
// getScreenWidth : returns number of columns screen has, as defined by user
//

uint getScreenWidth(void)
{
  return getScreenWidthVal();
}


//
// numbLinesStringNeeds : number of lines string needs, considers Duris color codes and linefeeds
//

size_t numbLinesStringNeeds(const char *strn)
{
  const size_t truelen = truestrlen(strn);
  const size_t extra = truelen % getScreenWidth();
  size_t lines = truelen / getScreenWidth();


 // add line if there is part of a line left or if the length is the exact screen width

  if (!lastCharLF(strn) && (extra || (!extra && (lines > 0))))
    lines++;

 // count a linefeed as an extra line (truestrlen already removes them as part of truelen)

  while (*strn != '\0')
  {
    if (*strn == '\n')
      lines++;

    strn++;
  }

  return lines;
}


//
// confirmChanges : Checks g_madeChanges and checks to see if the user wants to
//                  save zone or not - returns TRUE if everything was successful,
//                  FALSE otherwise
//

bool confirmChanges(void)
{
  if (g_madeChanges || (getStartRoomActiveVal() && (getRoomStartVal() != g_currentRoom->roomNumber)))
  {
    if (displayYesNoPrompt("\n\nChanges were made - save them", promptYes, false) == promptYes)
    {
      if (!writeFiles()) 
        return false;
    }
  }

  return true;
}


//
// writeFiles : writes all zone info to its respective file -
//              returns FALSE if an error was hit, TRUE otherwise
//

bool writeFiles(void)
{
  char strn[512];

  updateQuestItems();

  if (roomHasAllFollowers() >= 0)
  {
    sprintf(strn, "\n"
"Error: Room #%u has no non-followers.  At least one mob must be a\n"
"       non-follower.  Save aborted.\n\n", roomHasAllFollowers());

    _outtext(strn);

    return false;
  }

  if (!checkShopsOnSave()) 
    return false;

  if (getCheckSaveVal())
  {
    if (checkAll())
    {
      if (getNoSaveonCheckErrVal())
      {
        _outtext("\nErrors found while checking entities - aborting.\n\n");
        return false;
      }
      else 
      {
        _outtext("\nErrors found while checking entities - continuing...\n\n");
      }
    }
  }
  else 
  {
    _outtext("\n\n");
  }

  writeZoneFile(NULL);
  writeWorldFile(NULL);
  writeObjectFile(NULL);
  writeMobFile(NULL);


  if (checkForMobWithQuest()) 
  {
    writeQuestFile(NULL);
  }

 // delete .qst if necessary

  else
  {
    if (g_readFromSubdirs) 
      strcpy(strn, "qst/");
    else 
      strn[0] = '\0';

    strcat(strn, getMainZoneNameStrn());
    strcat(strn, ".qst");

   // purge the file

    deleteFile(strn);
  }


 // check all shops to make sure mob loaded as shop has all the items it needs
 // in its inventory

  if (checkForMobWithShop())
  {
    writeShopFile(NULL);
  }

 // delete .shp if necessary

  else
  {
    if (g_readFromSubdirs) 
      strcpy(strn, "shp/");
    else 
      strn[0] = '\0';

    strcat(strn, getMainZoneNameStrn());
    strcat(strn, ".shp");

    deleteFile(strn);
  }

  _outtext("\n");

  writeDefaultFiles(getMainZoneNameStrn());
  writeInventoryFile();

  _outtext("\n");

  strcpy(strn, getMainZoneNameStrn());
  strcat(strn, ".set");
  writeSettingsFile(strn, true);  // write zonename.set

  _outtext("\n");

 // prevent extraneous saves

  if (getStartRoomActiveVal())
  {
    sprintf(strn, "%u", g_currentRoom->roomNumber);
    addVar(&g_varHead, VAR_STARTROOM_NAME, strn);
  }

  g_madeChanges = false;

  return true;
}


//
// createPrompt : creates the prompt
//

void createPrompt(void)
{
  if (strcmpnocase(getMainPromptStrn(), "default"))
  {
    sprintf(g_promptString, "&=lg< %ur %ue %uot %uol %umt %uml >\n<> &n",
            g_numbRooms, g_numbExits, g_numbObjTypes, g_numbObjs,
            g_numbMobTypes, g_numbMobs);
  }
  else
  {
    sprintf(g_promptString, "%s\n&=lg<> &n", getMainPromptStrn());
  }
}


//
// displayRecordSizeInfo
//

void displayRecordSizeInfo(void)
{
  char strn[2048];


  sprintf(strn,
"\n"
"\n"
"room       : %u\n"
"objectType : %u\n"
"mobType    : %u\n"
"zone       : %u\n"
"quest      : %u\n"
"shop       : %u\n"
"\n"
"roomExit   : %u\n"
"extraDesc  : %u\n"
"objHere    : %u\n"
"mobHere    : %u\n"
"\n"
"masterNode : %u\n"
"editNode   : %u\n"
"stringNode : %u\n"
"entityLoad : %u\n"
"\n",

sizeof(room),
sizeof(objectType),
sizeof(mobType),
sizeof(zone),
sizeof(quest),
sizeof(shop),

sizeof(roomExit),
sizeof(extraDesc),
sizeof(objectHere),
sizeof(mobHere),

sizeof(masterKeywordListNode),
sizeof(editableListNode),
sizeof(stringNode),
sizeof(entityLoaded));

  _outtext(strn);

  sprintf(strn,
"command    : %u\n"
"alias      : %u\n"
"variable   : %u\n"
"\n",

sizeof(command),
sizeof(alias),
sizeof(variable));

  _outtext(strn);
}


//
// getOnOffStrn : return 'on' or 'off' based on bool
//

const char *getOnOffStrn(const bool value)
{
  if (value) 
    return "on";
  else 
    return "off";
}


//
// getYesNoStrn : return 'yes' or 'no' based on bool
//

const char *getYesNoStrn(const bool value)
{
  if (value) 
    return "yes";
  else 
    return "no";
}


//
// getYesNoStrn : pointer version
//

const char *getYesNoStrn(const void *ptr)
{
  if (ptr) 
    return "yes";
  else 
    return "no";
}


//
// plural : for use in pluralizing nouns based on amount
//

const char *plural(const int numb)
{
  if (numb == 1) 
    return "";
  else 
    return "s";
}


//
// getVowelN
//

const char *getVowelN(const char ch)
{
  switch (toupper(ch))
  {
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U': return "n";

    default : return "";
  }
}


//
// hasHelpArg : returns true if user entered 'help' arg on command-line - '-h' '-?' '--help' -
//              args can also start with /, so /h, /?, /-help
//

bool hasHelpArg(const int argc, const char *argv[])
{
  for (int i = 0; i < argc; i++)
  {
    const char *strn = argv[i];

    if ((*strn == '-') || (*strn == '/')) 
      strn++;
    else 
      continue;

    if ((!strcmp(strn, "?")) || (strcmpnocase(strn, "H")) || (strcmpnocase(strn, "-HELP")))
    {
      return true;
    }
  }

  return false;
}


//
// checkPreArgs : some args must be checked before reading .set
//

void checkPreArgs(const int argc, const char *argv[])
{
  for (int i = 0; i < argc; i++)
  {
    const char *strn = argv[i];

    if ((*strn == '-') || (*strn == '/')) 
      strn++;
    else 
      continue;

    if ((strcmpnocase(strn, "READDURISDIR")) ||
        (strcmpnocase(strn, "RDD")) ||
        (strcmpnocase(strn, "D")))
    {
      g_readFromSubdirs = true;
    }
  }
}


//
// checkArgs : check all args, after .set reading
//

void checkArgs(const int argc, const char *argv[])
{
  for (int i = 0; i < argc; i++)
  {
    const char *strn = argv[i];

    if ((strn[0] == '-') || (strn[0] == '/')) 
      strn++;
    else 
      continue;

    if ((strcmpnocase(strn, "NOVNUMCHECK")) ||
        (strcmpnocase(strn, "NVC")) ||
        (strcmpnocase(strn, "NOVC")))
    {
      setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, false, false);
    }
    else
    if ((strcmpnocase(strn, "VNUMCHECK")) ||
        (strcmpnocase(strn, "VC")))
    {
      setVarBoolVal(&g_varHead, VAR_VNUMCHECK_NAME, true, false);
    }
    else
    if ((strcmpnocase(strn, "NOEQCLASSCHECK")) ||
        (strcmpnocase(strn, "NECC")) ||
        (strcmpnocase(strn, "NOECC")))
    {
      setVarBoolVal(&g_varHead, VAR_CHECKMOBCLASSEQ_NAME, false, false);
    }
    else
    if ((strcmpnocase(strn, "EQCLASSCHECK")) ||
        (strcmpnocase(strn, "ECC")))
    {
      setVarBoolVal(&g_varHead, VAR_CHECKMOBCLASSEQ_NAME, true, false);
    }
    else
    if ((strcmpnocase(strn, "OBJAFFECT")) ||
        (strcmpnocase(strn, "OA")))
    {
      setVarBoolVal(&g_varHead, VAR_OBJAFFECT_NAME, true, false);
    }
    else
    if ((strcmpnocase(strn, "EDITALLFLAGS")) ||
        (strcmpnocase(strn, "EAF")))
    {
      setVarBoolVal(&g_varHead, VAR_EDITUNEDITABLEFLAGS_NAME, true, false);
    }
    else
    if ((strcmpnocase(strn, "NOLIMITCHECK")) ||
        (strcmpnocase(strn, "NLC")) ||
        (strcmpnocase(strn, "NOLC")))
    {
      setVarBoolVal(&g_varHead, VAR_CHECKLIMITS_NAME, false, false);
    }
    else
    if ((strcmpnocase(strn, "LIMITCHECK")) ||
        (strcmpnocase(strn, "LC")))
    {
      setVarBoolVal(&g_varHead, VAR_CHECKLIMITS_NAME, true, false);
    }
    else
    if ((strcmpnocase(strn, "NOZONEFLAGCHECK")) ||
        (strcmpnocase(strn, "NZFC")) ||
        (strcmpnocase(strn, "NOZFC")))
    {
      setVarBoolVal(&g_varHead, VAR_CHECKZONEFLAGS_NAME, false, false);
    }
    else
    if ((strcmpnocase(strn, "ZONEFLAGCHECK")) ||
        (strcmpnocase(strn, "ZFC")))
    {
      setVarBoolVal(&g_varHead, VAR_CHECKZONEFLAGS_NAME, true, false);
    }
    else
    if ((strcmpnocase(strn, "IGNOREZONES")) ||
        (strcmpnocase(strn, "IZS")))
    {
      setVarBoolVal(&g_varHead, VAR_IGNOREZONES_NAME, true, false);
    }
    else
    if (strcmpnocase(strn, "R"))
    {
      g_roomOnly = true;
    }
    else
    if (strcmpnocase(strn, "O"))
    {
      g_objOnly = true;
    }
    else
    if (strcmpnocase(strn, "M"))
    {
      g_mobOnly = true;
    }
    else
    if (strlefti(strn, "LL="))
    {
      while (*strn)
      {
        strn++;

        if (*strn == '=')
        {
          strn++;
          break;
        }
      }

      if (strnumber(strn) && strtoul(strn, NULL, 10))
        g_numbLookupEntries = strtoul(strn, NULL, 10);
    }
  }
}


//
// renumberCurrent : renumber everything such that lowest room/obj/mob numb stay the same -
//                   removes gaps
//

void renumberCurrent(void)
{
  char strn[256];


  renumberRooms(false, 0);

  sprintf(strn, "\nRooms have been renumbered (starting at %u).\n",
          getLowestRoomNumber());
  _outtext(strn);

  if (!noObjTypesExist())
  {
    renumberObjs(false, 0);

    sprintf(strn, "Objects have been renumbered (starting at %u).\n",
            getLowestObjNumber());
    _outtext(strn);
  }

  if (!noMobTypesExist())
  {
    renumberMobs(false, 0);

    sprintf(strn, "Mobs have been renumbered (starting at %u).\n",
            getLowestMobNumber());
    _outtext(strn);
  }

  _outtext("\n");
}


//
// renumberAll : renumbers everything starting from new vnum
//

void renumberAll(const char *args)
{
  char strn[256];   // used for output

  uint usernumb,    // number specified in args
       tempstart;   // list is temporarily renumbered to this to prevent overlap


  if (strlen(args) == 0)
  {
    renumberCurrent();
    return;
  }

  if (!strnumber(args))
  {
    _outtext("\nThe 'renumber' command's first argument must be a positive number.\n\n");
    return;
  }

  usernumb = strtoul(args, NULL, 10);

  if (usernumb == 0)
  {
    _outtext("\nYou cannot renumber starting from 0.\n\n");
    return;
  }

 // set temp range start (renumbered twice to prevent overlap)

  if (usernumb > getHighestRoomNumber())
    tempstart = usernumb;
  else
    tempstart = getHighestRoomNumber();

  tempstart += g_numbRooms;

  if ((tempstart + g_numbRooms) >= g_numbLookupEntries)
  {
    if (!changeMaxVnumAutoEcho(tempstart + g_numbRooms + 1000))
      return;
  }

  renumberRooms(true, tempstart);

  renumberRooms(true, usernumb);

  if (!noObjTypesExist())
  {
    if (usernumb > getHighestObjNumber())
      tempstart = usernumb;
    else
      tempstart = getHighestObjNumber();

    tempstart += g_numbObjTypes;

    if ((tempstart + g_numbObjTypes) >= g_numbLookupEntries)
    {
      if (!changeMaxVnumAutoEcho(tempstart + g_numbObjTypes + 1000))
        return;
    }

    renumberObjs(true, tempstart);

    renumberObjs(true, usernumb);
  }

  if (!noMobTypesExist())
  {
    if (usernumb > getHighestMobNumber())
      tempstart = usernumb;
    else
      tempstart = getHighestMobNumber();

    tempstart += g_numbMobTypes;

    if ((tempstart + g_numbMobTypes) >= g_numbLookupEntries)
    {
      if (!changeMaxVnumAutoEcho(tempstart + g_numbMobTypes + 1000))
        return;
    }

    renumberMobs(true, tempstart);

    renumberMobs(true, usernumb);
  }

  sprintf(strn,
"\nAll rooms, objects, and mobs renumbered starting at %u.\n\n", usernumb);

  _outtext(strn);
}


//
// lookup : given vnum or keyword, list all rooms, obj types, and mob types that match
//
//          can also specify 'room', 'object', or 'mob' as second arg, in which case only
//          that type is checked
//

void lookup(const char *args)
{
  room *room;
  objectType *obj;
  mobType *mob;

  char outStrn[512];
  bool foundSomething = false, foundRoom = false, foundObj = false, displayedEqual = false, vnum;

  size_t lines = 1;
  uint num;

  uint itemNumb, highNumb;


#define LOOKUP_BAD_ARG_MSG  "\n" \
"Specify a keyword or vnum, or one of <room|object|mob> as the first argument.\n\n"

  if (strlen(args) == 0)
  {
    _outtext(LOOKUP_BAD_ARG_MSG);

    return;
  }

  if (numbArgs(args) >= 2)
  {
    checkCommands(args, g_lookupCommands, LOOKUP_BAD_ARG_MSG, lookupExecCommand, NULL, NULL);

    return;
  }

 // only one arg specified, so check everything - rooms first

  vnum = strnumber(args);
  if (vnum) 
    num = strtoul(args, NULL, 10);

  _outtext("\n\n");

  itemNumb = getLowestRoomNumber();
  highNumb = getHighestRoomNumber();

  for (; itemNumb <= highNumb; itemNumb++)
  {
    char tempStrn[MAX_ROOMNAME_LEN + 1];

    if (!roomExists(itemNumb))
      continue;

    room = findRoom(itemNumb);

    if (!vnum)
    {
      strcpy(tempStrn, room->roomName);
      remColorCodes(tempStrn);
    }

    if ((!vnum && strstrnocase(tempStrn, args)) ||
         (vnum && (room->roomNumber == num)))
    {
      sprintf(outStrn, "&n%s&n (#%u)\n", room->roomName, room->roomNumber);

      foundSomething = foundRoom = true;

      if (checkPause(outStrn, lines))
        return;
    }
  }

 // now objects

  itemNumb = getLowestObjNumber();
  highNumb = getHighestObjNumber();

  for (; itemNumb <= highNumb; itemNumb++)
  {
    if (!objExists(itemNumb))
      continue;

    obj = findObj(itemNumb);

    if ((!vnum && scanKeyword(args, obj->keywordListHead)) ||
         (vnum && (itemNumb == num)))
    {
      if (foundRoom && !displayedEqual)
      {
        _outtext("===\n");
        lines++;
        displayedEqual = true;
      }

      sprintf(outStrn, "%s&n (#%u)\n", obj->objShortName, itemNumb);

      foundSomething = foundObj = true;

      if (checkPause(outStrn, lines))
        return;
    }
  }

  displayedEqual = false;

 // finally, mobs

  itemNumb = getLowestMobNumber();
  highNumb = getHighestMobNumber();

  for (; itemNumb <= highNumb; itemNumb++)
  {
    if (!mobExists(itemNumb))
      continue;

    mob = findMob(itemNumb);

    if ((!vnum && scanKeyword(args, mob->keywordListHead)) ||
         (vnum && (itemNumb == num)))
    {
      if (!displayedEqual && (foundObj || foundRoom))
      {
        _outtext("===\n");
        lines++;
        displayedEqual = true;
      }

      sprintf(outStrn, "%s&n (#%u)\n", mob->mobShortName, itemNumb);

      foundSomething = true;

      if (checkPause(outStrn, lines))
        return;
    }
  }

  if (!foundSomething) 
    _outtext("No matching rooms/objects/mobs found.\n");

  _outtext("\n");
}


//
// where : given keyword or vnum, list all obj/mobheres that exist with keyword/vnum
//

void where(const char *keyStrn)
{
  const room *roomPtr;
  const objectType *obj;
  const mobHere *mob;
  const questItem *qitem;
  const questQuest *qquest;
  char strn[1024];
  bool vnum = strnumber(keyStrn), match2 = false, foundMatch = false;
  uint num, j;
  size_t lines = 1;
  const uint highRoomNumb = getHighestRoomNumber();


  if (vnum)
    num = strtoul(keyStrn, NULL, 10);

  _outtext("\n\n");

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      sprintf(strn, " in %s&n (#%u)",
              roomPtr->roomName, roomPtr->roomNumber);

      if (!vnum)
      {
        if (displayEntireObjectHereList(roomPtr->objectHead, strn, lines, keyStrn,
                                        roomPtr->roomNumber, &foundMatch))
         return;
      }
      else
      {
        if (displayEntireObjectHereList(roomPtr->objectHead, strn, lines, num,
                                        roomPtr->roomNumber, &foundMatch))
         return;
      }

      mob = roomPtr->mobHead;
      while (mob)
      {
        if (vnum || mob->mobPtr)
        {
          if
  ((!vnum && scanKeyword(keyStrn, mob->mobPtr->keywordListHead)) ||
    (vnum && (mob->mobNumb == num)))
          {
            sprintf(strn, "%s&n (#%u) in %s&n (#%u)\n",
                    getMobShortName(mob->mobPtr), mob->mobNumb,
                    roomPtr->roomName, roomPtr->roomNumber);

            if (checkPause(strn, lines))
              return;

            foundMatch = true;
          }

         // check quests for items given to PCs

          if (mob->mobPtr && mob->mobPtr->questPtr)
          {
            qquest = mob->mobPtr->questPtr->questHead;

            while (qquest)
            {
              qitem = qquest->questPlayRecvHead;
              while (qitem)
              {
                if (vnum)
                {
                  if (qitem->itemVal == num)
                  {
                    sprintf(strn,
  "%s&n (#%d) given by %s&n (#%u)'s quest in %s&n (#%u)\n",
                            getObjShortName(findObj(qitem->itemVal)), num, 
                            getMobShortName(mob->mobPtr), mob->mobNumb,
                            roomPtr->roomName, roomPtr->roomNumber);

                    match2 = true;
                  }
                }
                else
                {
                  obj = findObj(qitem->itemVal);

                  if (obj && scanKeyword(keyStrn, obj->keywordListHead))
                  {
                    sprintf(strn,
  "%s&n (#%d) given by %s&n (#%u)'s quest in %s&n (#%u)\n",
                            obj->objShortName, qitem->itemVal,
                            getMobShortName(mob->mobPtr), mob->mobNumb,
                            roomPtr->roomName, roomPtr->roomNumber);

                    match2 = true;
                  }
                }

                if (match2)
                {
                  if (checkPause(strn, lines))
                    return;

                  foundMatch = true;
                  match2 = false;
                }

                qitem = qitem->Next;
              }

              qquest = qquest->Next;
            }
          }

         // check for shops that sell item

          if (mob->mobPtr && mob->mobPtr->shopPtr)
          {
            for (j = 0; j < MAX_NUMBSHOPITEMS; j++)
            {
              if (vnum)
              {
                if (mob->mobPtr->shopPtr->producedItemList[j] == num)
                {
                  sprintf(strn,
  "%s&n (#%d) sold by %s&n (#%u)'s shop in %s&n (#%u)\n",
                          getObjShortName(findObj(num)), num, 
                          getMobShortName(mob->mobPtr), mob->mobNumb,
                          roomPtr->roomName, roomPtr->roomNumber);

                  match2 = true;
                }
              }
              else
              {
                obj = findObj(mob->mobPtr->shopPtr->producedItemList[j]);

                if (obj && scanKeyword(keyStrn, obj->keywordListHead))
                {
                  sprintf(strn,
  "%s&n (#%u) sold by %s&n (#%u)'s shop in %s&n (#%u)\n",
                          obj->objShortName, obj->objNumber,
                          getMobShortName(mob->mobPtr), mob->mobNumb,
                          roomPtr->roomName, roomPtr->roomNumber);

                  match2 = true;
                }
              }

              if (match2)
              {
                if (checkPause(strn, lines))
                  return;

                foundMatch = true;
                match2 = false;
              }
            }
          }
        }

        sprintf(strn, " equipped by %s&n (#%u)  [room #%u]",
                getMobShortName(mob->mobPtr), mob->mobNumb, roomPtr->roomNumber);

        if (!vnum)
        {
          for (j = WEAR_LOW; j <= WEAR_TRUEHIGH; j++)
            if (displayEntireObjectHereList(mob->equipment[j], strn, lines, keyStrn,
                                            roomPtr->roomNumber, &foundMatch))
             return;
        }
        else
        {
          for (j = WEAR_LOW; j <= WEAR_TRUEHIGH; j++)
            if (displayEntireObjectHereList(mob->equipment[j], strn, lines, num,
                                            roomPtr->roomNumber, &foundMatch))
             return;
        }

        sprintf(strn, " carried by %s&n (#%u)  [room #%u]",
                getMobShortName(mob->mobPtr), mob->mobNumb, roomPtr->roomNumber);

        if (!vnum)
        {
          if (displayEntireObjectHereList(mob->inventoryHead, strn, lines, keyStrn,
                                          roomPtr->roomNumber, &foundMatch))
           return;
        }
        else
        {
          if (displayEntireObjectHereList(mob->inventoryHead, strn, lines, num,
                                          roomPtr->roomNumber, &foundMatch))
           return;
        }

        mob = mob->Next;
      }
    }
  }

  if (!foundMatch) 
    _outtext("No matching mobs or objects found.\n");

  _outtext("\n");
}


//
// dumpTextFile : display contents of file with filename to screen, interpreting color codes if
//                showColor is true
//

void dumpTextFile(const char *filename, const bool showColor)
{
  FILE *file;
  char strn[2048];
  size_t lines = 1;


  if ((file = fopen(filename, "rt")) == NULL)
  {
    sprintf(strn, "'%s' not found.\n", filename);

    _outtext(strn);

    return;
  }

  while (true)
  {
    if (fgets(strn, 2048, file) == NULL)
      break;

    if (showColor) 
      displayColorString(strn);
    else 
      _outtext(strn);

    _settextcolor(7);
    _setbkcolor(0);

    if (showColor) 
    {
      lines += numbLinesStringNeeds(strn);
    }

   // truestrlen checks internal DE showColor var, so do it 'manually'

    else
    {
      lines += (strlen(strn) / getScreenWidth());

      if (strlen(strn) % getScreenWidth())
      {
       // 'remainder'

        lines++;
      }
      else
      {
       // if string is exact size of screen width, it will generate an extra newline

        if ((strlen(strn) / getScreenWidth()) > 0)
          lines++;
      }
    }

    if (checkPause(NULL, lines))
      break;
  }

  fclose(file);
}


//
// displayVersionInfo : display DE version, my email address and website URL, and date compiled to screen
//

void displayVersionInfo(void)
{
  displayColorString(
    "\n&+YdurisEdit v" DE_VERSION "&n, written by Michael Glosenger (halitoxin@hotmail.com) -\n"
    "  available at http://www.jollython.com/, compiled " __DATE__ "\n\n");
}


//
// verifyZoneFlags : check that zone number is non-zero and all 'room zone' numbers are valid
//

void verifyZoneFlags(void)
{
  room *roomPtr;
  char bigstrn[256];
  bool checkedZF = false;
  const uint highRoomNumb = getHighestRoomNumber();


 // check that zone number isn't 0 (0 is bad)

  if (g_zoneRec.zoneNumber == 0)
  {
    roomPtr = findRoom(getLowestRoomNumber());

    if (roomPtr->zoneNumber)
      g_zoneRec.zoneNumber = roomPtr->zoneNumber;
    else 
      g_zoneRec.zoneNumber = 1;

    editUIntVal(&g_zoneRec.zoneNumber, false, 
                 "&+CZone's zone number is equal to 0 - specify a valid zone number: ");

    g_madeChanges = true;
  }

  roomPtr = findRoom(getLowestRoomNumber());

  if (roomPtr->zoneNumber == 0) 
    roomPtr->zoneNumber = g_zoneRec.zoneNumber;

 // if "check zone flags" var is set, check all room zone flags vs. zone
 // numb flag

  if (getCheckZoneFlagsVal())
  {
    for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
    {
      if (roomExists(roomNumb))
      {
        roomPtr = findRoom(roomNumb);

        if (roomPtr->zoneNumber != g_zoneRec.zoneNumber)
        {
          if (!checkedZF)
          {
            sprintf(bigstrn, "\n"
      "Warning - zone flag of room #%u doesn't match zone flag of zone (room is\n"
      "          %u, zone is %u).  Setting every room's zone flag equal to %u...\n\n",
            roomPtr->roomNumber, roomPtr->zoneNumber, g_zoneRec.zoneNumber,
            g_zoneRec.zoneNumber);

            _outtext(bigstrn);

            g_madeChanges = true;
            checkedZF = true;

            getkey();
          }

          roomPtr->zoneNumber = g_zoneRec.zoneNumber;
        }
      }
    }
  }
}


//
// isValidBoolYesNo : to be a valid bool yes/no strn must be Y, N, 1, or 0
//

bool isValidBoolYesNo(const char *strn)
{
  return (strcmpnocase(strn, "Y") || strcmpnocase(strn, "N") || 
          strcmpnocase(strn, "1") || strcmpnocase(strn, "0"));
}


//
// getBoolYesNo : if strn == "Y"/"1", TRUE, if "N"/"0", FALSE, if anything else, FALSE
//

bool getBoolYesNo(const char *strn)
{
  if (strcmpnocase(strn, "Y") || !strcmp(strn, "1")) 
    return true;

  if (strcmpnocase(strn, "N") || !strcmp(strn, "0"))
    return false;

  return false;
}


//
// getYesNoBool : if TRUE, return "Y", if FALSE, return "N"
//

const char *getYesNoBool(const bool boolean)
{
  if (boolean) 
    return "Y";
  else 
    return "N";
}


//
// toggleGuildVar : toggle 'check guild stuff' var
//

void toggleGuildVar(void)
{
  if (!varExists(g_varHead, VAR_CHECKGUILDSTUFF_NAME))
  {
    setVarArgs(&g_varHead, VAR_CHECKGUILDSTUFF_NAME " TRUE", true, true, true);
  }
  else
    toggleVar(&g_varHead, VAR_CHECKGUILDSTUFF_NAME);
}


//
// at : execute command in arg 2+ in location specified by arg 1 (room vnum or keyword of objh/mobh/room name)
//

void at(const char *args)
{
  char atwhere[512];
  room *room, *oldRoom;


 // prevent stack overflow badness

  if (g_inAtCmd)
  {
    _outtext("\nYou try, but fail.\n\n");
    return;
  }

  if (!strlen(args))
  {
    _outtext("\nAt where do what?  Eh?\n\n");
    return;
  }

  getArg(args, 1, atwhere, 511);

  room = getRoomKeyword(atwhere, true);
  if (!room)
  {
    _outtext("\nCouldn't find a room that matched first argument.\n\n");
    return;
  }

 // get rid of arg1

  while (*args && (*args != ' '))
    args++;

  if (*args == '\0')
  {
    _outtext("\nSpecify a command to execute at the specified location.\n\n");
    return;
  }

  g_inAtCmd = true;

 // get rid of any spaces

  while (*args == ' ')
    args++;

  oldRoom = g_currentRoom;
  g_currentRoom = room;

  interpCommands(args, false);

  g_currentRoom = oldRoom;

  g_inAtCmd = false;
}


//
// displayLookupList : display room/obj type/mob type lookup table
//

void displayLookupList(const char whichLookup, uint start)
{
  uint j, low, high;
  size_t lines = 1;
  char strn[256];


  if ((whichLookup < ROOM_LOOKUP) || (whichLookup > MOB_LOOKUP)) 
    return;

  switch (whichLookup)
  {
    case ROOM_LOOKUP : low = getLowestRoomNumber();  high = getHighestRoomNumber();
                       break;
    case OBJ_LOOKUP  : if (!g_numbObjTypes)
                       {
                         _outtext("\nThere are currently no object types.\n\n");
                         return;
                       }
                       low = getLowestObjNumber();  high = getHighestObjNumber();
                       break;
    case MOB_LOOKUP  : if (!g_numbMobTypes)
                       {
                         _outtext("\nThere are currently no mob types.\n\n");
                         return;
                       }
                       low = getLowestMobNumber();  high = getHighestMobNumber();
                       break;
    default : return;
  }

  if (start < low) 
    start = low;

  if (start > high)
  {
    sprintf(strn, "\n\nStarting vnum specified too high - reset to %u.", low);
    _outtext(strn);

    start = low;
    lines = 3;
  }

  _outtext("\n\n");

  for (j = start; j <= high; j++)
  {
    switch (whichLookup)
    {
      case ROOM_LOOKUP : if (g_roomLookup[j])
                           sprintf(strn, "  room #%u - 0x%p (#%u)\n",
                                   j, g_roomLookup[j], g_roomLookup[j]->roomNumber);
                         else 
                           strn[0] = '\0';

                         break;

      case OBJ_LOOKUP  : if (g_objLookup[j])
                           sprintf(strn, "  object #%u - 0x%p (#%u)\n",
                                   j, g_objLookup[j], g_objLookup[j]->objNumber);
                         else 
                           strn[0] = '\0';

                         break;

      case MOB_LOOKUP  : if (g_mobLookup[j])
                           sprintf(strn, "  mob #%u - 0x%p (#%u)\n",
                                   j, g_mobLookup[j], g_mobLookup[j]->mobNumber);
                         else 
                           strn[0] = '\0';

                         break;
    }

    if (strn[0])
    {
      if (checkPause(strn, lines))
        return;
    }
  }

  _outtext("\n");
}


//
// preDisplayLookupList : called from listExecCommand(), process user input before displayLookupList()
//

#define LOOKUP_CMD_FORMAT_STR  "\n" \
"The format of the 'lookup' command is 'lookup <room|object|mob>\n" \
"[start vnum (optional)]'.\n\n"

void preDisplayLookupList(const char *args)
{
  char arg1[512], arg2[512];
  uint numb;

  getArg(args, 1, arg1, 511);
  getArg(args, 2, arg2, 511);

  if (!strlen(arg1))
  {
    _outtext(LOOKUP_CMD_FORMAT_STR);
    return;
  }

  if (strlen(arg2))
  {
    if (!strnumber(arg2))
    {
      _outtext(LOOKUP_CMD_FORMAT_STR);
      return;
    }

    numb = strtoul(arg2, NULL, 10);
  }
  else 
  {
    numb = 0;
  }

  if (strlefti("ROOM", arg1)) 
  {
    displayLookupList(ROOM_LOOKUP, numb);
  }
  else if (strlefti("OBJECT", arg1)) 
  {
    displayLookupList(OBJ_LOOKUP, numb);
  }
  else if (strlefti("MOB", arg1) || strlefti("CHAR", arg1)) 
  {
    displayLookupList(MOB_LOOKUP, numb);
  }
  else 
  {
    _outtext(LOOKUP_CMD_FORMAT_STR);
  }
}


//
// displayVnumInfo : display low/high vnums for everything
//

void displayVnumInfo(void)
{
  char strn[2048];

  sprintf(strn,
"\n"
"Vnum range    : 0-%u\n"
"\n"
"Room low vnum : %u\n"
"Room high vnum: %u\n"
"\n"
"Obj low vnum  : %u\n"
"Obj high vnum : %u\n"
"\n"
"Mob low vnum  : %u\n"
"Mob high vnum : %u\n"
"\n", g_numbLookupEntries - 1,
      g_lowestRoomNumber, g_highestRoomNumber,
      g_lowestObjNumber, g_highestObjNumber,
      g_lowestMobNumber, g_highestMobNumber);

  _outtext(strn);
}


//
// getMoneyStrn : given money amount in copper, create human-friendly plat/silver/etc amount in strn
//

char *getMoneyStrn(uint coins, char *strn)
{
  char catstrn[256];


  if (!coins) 
  {
    strcpy(strn, "0&+yc&n");
  }
  else
  {
    strn[0] = '\0';

    if (coins / 1000)
    {
      sprintf(strn, "%u&+Wp&n, ", coins / 1000);
    }

    coins %= 1000;

    if (coins / 100)
    {
      sprintf(catstrn, "%u&+Yg&n, ", coins / 100);
      strcat(strn, catstrn);
    }

    coins %= 100;

    if (coins / 10)
    {
      sprintf(catstrn, "%us, ", coins / 10);
      strcat(strn, catstrn);
    }

    coins %= 10;

    if (coins)
    {
      sprintf(catstrn, "%u&+yc&n", coins);
      strcat(strn, catstrn);
    }

   // get rid of comma and space if they exist

    if (strn[strlen(strn) - 1] == ' ') 
      strn[strlen(strn) - 2] = '\0';
  }

  return strn;
}


//
// deleteUnusedObjMobTypes : delete objects and mobs not referenced/loaded somewhere
//

void deleteUnusedObjMobTypes(void)
{
  usint objsDel = 0, mobsDel = 0;
  char strn[256];
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestObjNumber();


  if (displayYesNoPrompt("\n&+cDelete all unused object and mob types", promptNo, false) == promptNo)
    return;

 // delete unused mobs first to get rid of any shop/quest object references

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      if (!getNumbEntities(ENTITY_MOB, mobNumb, false))
      {
        mobType *mob = findMob(mobNumb);

        deleteMobType(mob, true);

        mobsDel++;
      }
    }
  }

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      if (!isObjTypeUsed(objNumb))
      {
        objectType *obj = findObj(objNumb);

        deleteObjectType(obj, true);

        objsDel++;
      }
    }
  }

  sprintf(strn, "%u object type%s and %u mob type%s deleted.\n\n",
          objsDel, plural(objsDel), mobsDel, plural(mobsDel));

  _outtext(strn);
}


//
// resetEntityPointersByNumb : run through all rooms and reset object/mob type pointers on obj/mobHeres -
//                             called when an object/mob type is created or deleted or a vnum is changed
//

void resetEntityPointersByNumb(const bool resetObjs, const bool resetMobs)
{
  bool changed = false;
  const uint highRoomNumb = getHighestRoomNumber();


  if (!resetObjs && !resetMobs) 
    return;

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      if (resetObjs && roomPtr->objectHead)
      {
        resetObjHEntityPointersByNumb(roomPtr->objectHead);

        changed = true;
      }

      if (resetMobs && roomPtr->mobHead)
      {
        mobHere *mob = roomPtr->mobHead;

        while (mob)
        {
          mob->mobPtr = findMob(mob->mobNumb);

          mob = mob->Next;
        }

        changed = true;
      }

      if (changed)
      {
        deleteMasterKeywordList(roomPtr->masterListHead);
        roomPtr->masterListHead = createMasterKeywordList(roomPtr);

        deleteEditableList(roomPtr->editableListHead);
        roomPtr->editableListHead = createEditableList(roomPtr);

        changed = false;
      }
    }
  }
}


//
// resetObjCond : set all object types with conditions less than 100 to 100
//

void resetObjCond(void)
{
  uint numb, high = getHighestObjNumber();

  for (numb = getLowestObjNumber(); numb <= high; numb++)
  {
    if (objExists(numb))
    {
      objectType *obj = findObj(numb);

      if (obj->condition < 100) 
      {
        obj->condition = 100;

        g_madeChanges = true;
      }
    }
  }

  _outtext("\nAll objects with conditions less than 100 (if any) set to 100.\n\n");
}


//
// fixGuildStuff : probably useless anymore, was used back when players made their own guild zones
//

void fixGuildStuff(void)
{
  uint numb, high, numbNoshow = 0, numbAggr = 0, numbHunt = 0, numbNonNeut = 0;
  char strn[256];


  if (!getCheckGuildStuffVal())
  {
    if (displayYesNoPrompt("\n&+c"
"The 'check guild stuff' variable is not set - do you wish to reset object\n&+c"
"and mob flags based on required guild settings anyway", promptNo, false) == promptNo)
    {
      return;
    }
  }

 // reset all object noshow bits

  high = getHighestObjNumber();

  for (numb = getLowestObjNumber(); numb <= high; numb++)
  {
    if (objExists(numb))
    {
      objectType *obj = findObj(numb);

      if (obj->extraBits & ITEM_NOSHOW)
      {
        obj->extraBits &= ~ITEM_NOSHOW;
        numbNoshow++;
      }
    }
  }

 // now reset aggro flags and the hunter flag

  high = getHighestMobNumber();

  for (numb = getLowestMobNumber(); numb <= high; numb++)
  {
    if (mobExists(numb))
    {
      mobType *mob = findMob(numb);

      if (mob->aggroBits || mob->aggro2Bits || mob->aggro3Bits)
      {
        mob->aggroBits = mob->aggro2Bits = mob->aggro3Bits = 0;
        numbAggr++;
      }

      if (mob->actionBits & ACT_HUNTER)
      {
        mob->actionBits &= ~ACT_HUNTER;
        numbHunt++;
      }

      if (mob->alignment != 0)
      {
        mob->alignment = 0;
        numbNonNeut++;
      }
    }
  }

  sprintf(strn, "\n"
"Reset %u object%s with the 'noshow' flag set, %u mob%s with 'aggro' flags\n"
"set, %u mob%s with the 'hunter' flag set, and %u non-neutral mob%s.\n\n",
          numbNoshow, plural(numbNoshow), numbAggr, plural(numbAggr),
          numbHunt, plural(numbHunt), numbNonNeut, plural(numbNonNeut));

  displayColorString(strn);

  if (numbNoshow || numbAggr || numbHunt || numbNonNeut) 
    g_madeChanges = true;
}


//
// editRoomsOnly : called via command-line switch, load only .wld and exist only in 'edit room' menu
//

void editRoomsOnly(void)
{
  room *roomPtr;


  g_roomLookup = new(std::nothrow) room*[g_numbLookupEntries];
  if (!g_roomLookup)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error allocating memory for room lookup tables - the current size of\n"
"%u entries will consume %u bytes - try reducing\n"
"the size with the -ll=<loaded> command-line switch, or buy more RAM.\n",
            g_numbLookupEntries, (g_numbLookupEntries * sizeof(room*)));

    _outtext(outstrn);

    exit(1);
  }

  memset(g_roomLookup, 0, g_numbLookupEntries * sizeof(room*));

  readWorldFile(NULL);

// init stuff (Win32 is already initialized)

#ifdef __UNIX__
  initUnixCursesScreen();
#endif

  clrscr(7, 0);

  roomPtr = findRoom(getLowestRoomNumber());

  if (!roomPtr)
  {
    _outtext(
"There are currently no rooms to edit.  Room-edit mode requires that at least\n"
"one room exists.\n\n");

    exit(1);
  }

  do
  {
    roomPtr = editRoom(roomPtr, true);
  } while (roomPtr);

  if (g_madeChanges)
  {
    if (displayYesNoPrompt("\n\nChanges were made - save them", promptYes, false) == promptYes)
    {
      writeWorldFile(NULL);
    }
  }
}


//
// editObjsOnly : called via command-line switch, load only .obj and exist only in 'edit object' menu
//

void editObjsOnly(void)
{
  objectType *obj;


  g_objLookup = new(std::nothrow) objectType*[g_numbLookupEntries];
  if (!g_objLookup)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error allocating memory for object lookup tables - the current size of\n"
"%u entries will consume %u bytes - try reducing\n"
"the size with the -ll=<loaded> command-line switch, or buy more RAM.\n",
            g_numbLookupEntries, (g_numbLookupEntries * sizeof(objectType*)));

    _outtext(outstrn);

    exit(1);
  }

  memset(g_objLookup, 0, g_numbLookupEntries * sizeof(objectType*));

  readObjectFile(NULL);

// init stuff (Win32 is already initialized)

#ifdef __UNIX__
  initUnixCursesScreen();
#endif

  clrscr(7, 0);

  if (noObjTypesExist())
  {
    _outtext(
"There are currently no object types to edit.  Object-edit mode requires that\n"
"at least one object type exists.\n\n");

    exit(1);
  }

  obj = findObj(getLowestObjNumber());

  editObjType(obj, true);

  if (g_madeChanges)
  {
    if (displayYesNoPrompt("\n\nChanges were made - save them", promptYes, false) == promptYes)
    {
      writeObjectFile(NULL);
    }
  }
}


//
// editMobsOnly : called via command-line switch, load only .mob and exist only in 'edit mob' menu
//

void editMobsOnly(void)
{
  mobType *mob;


  g_mobLookup = new(std::nothrow) mobType*[g_numbLookupEntries];
  if (!g_mobLookup)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error allocating memory for mob lookup tables - the current size of\n"
"%u entries will consume %u bytes - try reducing\n"
"the size with the -ll=<loaded> command-line switch, or buy more RAM.\n",
            g_numbLookupEntries, (g_numbLookupEntries * sizeof(mobType*)));

    _outtext(outstrn);

    exit(1);
  }

  memset(g_mobLookup, 0, g_numbLookupEntries * sizeof(mobType*));

  readMobFile(NULL);

// init stuff (Win32 is already initialized)

#ifdef __UNIX__
  initUnixCursesScreen();
#endif

  clrscr(7, 0);

  if (noMobTypesExist())
  {
    _outtext(
"There are currently no mob types to edit.  Mob-edit mode requires that at\n"
"least one mob type exists.\n\n");

    exit(1);
  }

  mob = findMob(getLowestMobNumber());

  editMobType(mob, true);

  if (g_madeChanges)
  {
    if (displayYesNoPrompt("\n\nChanges were made - save them", promptYes, false) == promptYes)
    {
      writeMobFile(NULL);
    }
  }
}


//
// validANSIletter : returns true if ch is valid DURIS color code letter
//

bool validANSIletter(const char ch)
{
  switch (toupper(ch))
  {
    case 'B' :
    case 'C' :
    case 'L' :
    case 'R' :
    case 'G' :
    case 'M' :
    case 'Y' :
    case 'W' : return true;

    default : return false;
  }
}


//
// durisANSIcode : checks text at specified pos in string for "duris ANSIness"
//                 returns number of characters that need to be skipped to
//                 get past the code
//
//     strn : string being checked
//      pos : position in string at which to check
//

uchar durisANSIcode(const char *strn, const size_t pos)
{
  if (pos >= strlen(strn)) 
    return 0;

  if ((strn[pos] != '&') || !strn[pos + 1]) 
    return 0;

 // &n

  if (toupper(strn[pos + 1]) == 'N') 
    return 2;

 // &+(whatever)

  if ((strn[pos + 1] == '+') && validANSIletter(strn[pos + 2])) 
    return 3;

 // &-(whatever)

  if ((strn[pos + 1] == '-') && validANSIletter(strn[pos + 2])) 
    return 3;

 // &=(bg)(fg)

  if ((strn[pos + 1] == '=') &&
      (strn[pos + 2] && validANSIletter(strn[pos + 2])) &&
      (validANSIletter(strn[pos + 3])))
    return 4;

  return 0;
}


//
// changeMaxVnum : resizes room/obj/mob lookup tables to new size, assumes that new top vnum is high enough 
//                 to fit old vnums
//
//                 returns false if there is some error
//

bool changeMaxVnum(const uint newTopVnum)
{
  const uint numbEntries = newTopVnum + 1;
  room **newRoomArr = new(std::nothrow) room*[numbEntries];
  objectType **newObjArr = new(std::nothrow) objectType*[numbEntries];
  mobType **newMobArr = new(std::nothrow) mobType*[numbEntries];


  if (!newRoomArr || !newObjArr || !newMobArr)
  {
    if (newRoomArr)
      delete[] newRoomArr;

    if (newObjArr)
      delete[] newObjArr;

    if (newMobArr)
      delete[] newMobArr;

    return false;
  }

 // copy old entries into new table

  uint numbCopy;

  if (numbEntries < g_numbLookupEntries)
  {
    numbCopy = numbEntries;
  }
  else
  {
   // clear upper range in new tables

    const uint intNewRangeAmt = numbEntries - g_numbLookupEntries;

    memset(newRoomArr + g_numbLookupEntries, 0, sizeof(room *) * intNewRangeAmt);
    memset(newObjArr + g_numbLookupEntries, 0, sizeof(objectType *) * intNewRangeAmt);
    memset(newMobArr + g_numbLookupEntries, 0, sizeof(mobType *) * intNewRangeAmt);

    numbCopy = g_numbLookupEntries;
  }

  memcpy(newRoomArr, g_roomLookup, sizeof(room *) * numbCopy);
  memcpy(newObjArr, g_objLookup, sizeof(objectType *) * numbCopy);
  memcpy(newMobArr, g_mobLookup, sizeof(mobType *) * numbCopy);

  g_numbLookupEntries = numbEntries;

 // if room/obj/mob high numb equals 0, then there are no rooms/objs/mobs so reset low to new limit + 1

  if (g_highestRoomNumber == 0)
    g_lowestRoomNumber = newTopVnum + 1;

  if (g_highestObjNumber == 0)
    g_lowestObjNumber = newTopVnum + 1;

  if (g_highestMobNumber == 0)
    g_lowestMobNumber = newTopVnum + 1;

 // free old tables, reset global vars to new tables

  delete[] g_roomLookup;
  delete[] g_objLookup;
  delete[] g_mobLookup;

  g_roomLookup = newRoomArr;
  g_objLookup = newObjArr;
  g_mobLookup = newMobArr;

  return true;
}


//
// changeMaxVnumUser : user front end for changing maximum vnum
//

void changeMaxVnumUser(const char *args)
{
  const uint topVnum = strtoul(args, NULL, 10);


  if (!strnumber(args))
  {
    _outtext("\nSpecify the new max vnum for rooms, objects, and mobs as the first argument.\n\n");
    return;
  }

  if ((topVnum < getHighestRoomNumber()) || (topVnum < getHighestObjNumber()) || 
      (topVnum < getHighestMobNumber()))
  {
    _outtext("\nThat vnum limit would be too low for the current rooms, objects, and mobs.\n\n");
    return;
  }

  if (!changeMaxVnum(topVnum))
  {
    _outtext("\nError changing max vnum - you probably don't have enough free memory for the\n"
             "new limit.\n\n");
    return;
  }

  char strn[256];

  sprintf(strn, "\nMaximum vnum for rooms, objects, and mobs reset to %u.\n\n", topVnum);
  _outtext(strn);
}


//
// changeMaxVnumAutoEcho : called when max vnum is automatically changed when creating room, loading zone, etc,
//                         outputs to screen, returns false on failure
//

bool changeMaxVnumAutoEcho(const uint newTopVnum)
{
  char strn[256];

  sprintf(strn,
"\nThe current vnum limit is too low.  Changing limit to %u...\n",
          newTopVnum);

  _outtext(strn);

  if (!changeMaxVnum(newTopVnum))
  {
    _outtext("Error changing limit - not enough memory - aborting.\n");

    return false;
  }

  return true;
}


//
// subZeroFloor : don't subtract if it would cause a negative result, instead return zero
//

size_t subZeroFloor(const size_t first, const size_t second)
{
  if (second > first)
    return 0;
  else
    return first - second;
}


//
// deleteFile : attempt to delete a file
//

void deleteFile(const char *filename)
{
  if (unlink(filename))
    remove(filename);
}
