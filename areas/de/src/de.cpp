//
//  File: de.cpp         originally part of durisEdit
//
//  Usage: global variable declarations and main() function
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

#include "de.h"          // miscellaneous global-type static settings
#include "system.h"      // filenames, screen size
#include "vardef.h"      // default values for various DE variables

#include "fh.h"          // contains function predefinitions




// global variables

bool g_madeChanges = false;      // set to TRUE if user makes changes to zone
bool g_readFromSubdirs = false;  // whether to read from wld/ mob/ etc. subdirs

bool g_roomOnly = false,   // set to TRUE if user wants to edit only rooms/
     g_objOnly = false,    // objs/mobs
     g_mobOnly = false;

bool g_oldObjConv = false; // has to be done..  if an object file containing
                           // objects from pre-material/space/size days, this
                           // is set (quite outdated nowadays, but one never
                           // knows what old Duris [or even diku?] areas one 
                           // may try to edit!)

bool g_inAtCmd = false;    // prevent stack overflow naughtiness with doing 
                           // 'at x at x at x ...'

uint g_commandsEntered = 0;   // for "save every X commands"

// pointers to linked lists and other stuff that is vital to durisEdit

room *g_currentRoom = NULL;       // room in which user is
zone g_zoneRec;                   // .zon

// default room/exit/object/mob/edesc/quest/shop

room *g_defaultRoom = NULL;
roomExit *g_defaultExit = NULL;
objectType *g_defaultObject = NULL;
mobType *g_defaultMob = NULL;
extraDesc *g_defaultExtraDesc = NULL;
quest *g_defaultQuest = NULL;
shop *g_defaultShop = NULL;

// lookup tables

room       **g_roomLookup = NULL;
objectType **g_objLookup = NULL;
mobType    **g_mobLookup = NULL;

uint g_numbLookupEntries = DEFAULT_LOOKUP_ENTRIES;

uint g_lowestRoomNumber = g_numbLookupEntries;
uint g_highestRoomNumber = 0;

uint g_lowestObjNumber = g_numbLookupEntries;
uint g_highestObjNumber = 0;

uint g_lowestMobNumber = g_numbLookupEntries;
uint g_highestMobNumber = 0;


// counters that keep track of number of whatever existing

uint g_numbRooms = 0, g_numbObjTypes = 0, g_numbMobTypes = 0,
     g_numbExits = 0, g_numbObjs = 0,     g_numbMobs = 0;


// miscellaneous strings and list heads

char g_promptString[1024];  // input prompt - actual prompt text is stored here, prompt
                            // "template" is in user variable

entityLoaded *g_numbLoadedHead = NULL;  // used to store how many of whatever
                                        // have been loaded

editableListNode *g_inventory = NULL;  // stores inventory

alias *g_aliasHead = NULL;   // head of alias list

variable *g_varHead = NULL;  // head of variable list

stringNode *g_commandHistory = NULL;  // pointer to head of command history list


// templates - defined in template.cpp

extern uint g_roomFlagTemplates[], g_objExtraFlagTemplates[], g_objWearFlagTemplates[],
            g_objExtra2FlagTemplates[], g_objAntiFlagTemplates[], g_objAnti2FlagTemplates[],
            g_objAff1FlagTemplates[], g_objAff2FlagTemplates[],
            g_objAff3FlagTemplates[], g_objAff4FlagTemplates[],
            g_mobActionFlagTemplates[],
            g_mobAff1FlagTemplates[], g_mobAff2FlagTemplates[],
            g_mobAff3FlagTemplates[], g_mobAff4FlagTemplates[],
            g_mobAggroFlagTemplates[], g_mobAggro2FlagTemplates[],
	    g_mobAggro3FlagTemplates[];


// all uppercase apply names looks bad to me, and lowercasing the table in-place causes the 'no execute'
// bit to explode, so this is used to create a duplicate of whatever apply_types table is in common.c,
// only all lowercase

char *g_apply_types_low[APPLY_LAST + 1];

extern "C" char *apply_types[];

// main - entry point

#ifndef _WIN32
int main(const int argc, const char *argv[])
#else
int __cdecl main(const int argc, const char *argv[])
#endif
{
  char strn[512];
  bool getZoneNameFromArgs = true;  // might be false for Win32
  bool newZoneWin32 = false;  // if user wants to create a new zone, don't open any old files


 // if Win32, perform console initialization - init Unix later so that command-line help shows up decently

#ifdef _WIN32
  if (checkWindowsVer()) 
    return 1;

  if (setupWindowsConsole()) 
    return 1;
#endif

 // init flag template arrays

  memset(g_roomFlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);

  memset(g_objExtraFlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_objWearFlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_objExtra2FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_objAntiFlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_objAnti2FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_objAff1FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_objAff2FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_objAff3FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_objAff4FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);

  memset(g_mobActionFlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_mobAff1FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_mobAff2FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_mobAff3FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_mobAff4FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_mobAggroFlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_mobAggro2FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);
  memset(g_mobAggro3FlagTemplates, 0, sizeof(uint) * NUMB_FLAG_TEMPLATES);

 // check for command-line arguments or lack thereof

  if (argc < 2)
  {
   // no filename, but under Win32 pop up prompt and 'open file' dialog

#ifdef _WIN32
    char filename[MAX_VARVAL_LEN + 1];  // OpenFile must have a buffer of at least 256

    if (!handleNoArgs(filename, &newZoneWin32))
    {
      _outtext("\n\nExiting..\n\n");

      return 1;
    }
    else
    {
      addVar(&g_varHead, VAR_MAINZONENAME_NAME, filename);
    }

    getZoneNameFromArgs = false;
#else
   // unix

    puts(
"Specify the \"main\" name of the zone as the first command-line argument -\n"
"i.e. 'de forest'.  The .wld, .mob, etc. files will have this prefix.\n\n"
"For more information on valid command-line parameters, type 'de -h'.\n");

    return 1;
#endif
  }

  if (hasHelpArg(argc, argv))
  {
    displayArgHelp();
    return 0;
  }

#ifdef __UNIX__
  initUnixCursesScreen();
#endif

 // fix apply types so they're all lowercase

  for (uint i = 0; i <= APPLY_LAST; i++)
  {
    g_apply_types_low[i] = new(std::nothrow) char[strlen(apply_types[i]) + 1];

    if (!g_apply_types_low[i])
    {
      displayAllocError("char[]", "main");

      return 1;
    }

    strcpy(g_apply_types_low[i], apply_types[i]);

    lowstrn(g_apply_types_low[i]);
  }

  if (getZoneNameFromArgs)
  {
    if (strlen(argv[1]) >= MAX_MAINNAME_LEN)
    {
      char outstrn[256];

      sprintf(outstrn, 
"That \"main\" zone name is too long (greater than %u characters).\n",
              (uint)(MAX_MAINNAME_LEN - 1));

      _outtext(outstrn);

      return 1;
    }

  // set mainName var

    addVar(&g_varHead, VAR_MAINZONENAME_NAME, argv[1]);
  }

 // under Win32, set up 'press a key to continue..' exit func - do this separately so that help screen
 // doesn't display prompt as well as everything else before this point, aye

#ifdef _WIN32
  setWin32ExitFunc();
#endif

 // read the data files

  readSettingsFile(MAIN_SETFILE_NAME);

  strcpy(strn, getMainZoneNameStrn());
  strcat(strn, ".set");

  checkPreArgs(argc, argv);   // certain args need to be checked for first

  if (!newZoneWin32)
  {
    readSettingsFile(strn);     // read zone-specific settings file
  }

 // check command-line args (after reading set file..)

  checkArgs(argc, argv);

 // if user wants to go into edit-only mode, oblige willingly

  if (g_roomOnly)
  {
    editRoomsOnly();
    return 0;
  }
  else
  if (g_objOnly)
  {
    editObjsOnly();
    return 0;
  }
  else
  if (g_mobOnly)
  {
    editMobsOnly();
    return 0;
  }

 // init reference tables

  g_roomLookup = new(std::nothrow)       room*[g_numbLookupEntries];
  g_objLookup  = new(std::nothrow) objectType*[g_numbLookupEntries];
  g_mobLookup  = new(std::nothrow)    mobType*[g_numbLookupEntries];

  if (!g_roomLookup || !g_objLookup || !g_mobLookup)
  {
    char outstrn[512];

    sprintf(outstrn, "\n" 
"Error allocating memory for room, object, and/or mob lookup tables - the\n"
"current size of %u entries will consume %u bytes - try reducing\n"
"the size with the -ll=<max vnum> command-line switch, or buy more RAM.\n",
            g_numbLookupEntries,
            (g_numbLookupEntries * (sizeof(room*) + sizeof(objectType*) + sizeof(mobType*))));

    _outtext(outstrn);

    return 1;
  }

  memset(g_roomLookup, 0, g_numbLookupEntries * sizeof(room*));
  memset(g_objLookup, 0, g_numbLookupEntries * sizeof(objectType*));
  memset(g_mobLookup, 0, g_numbLookupEntries * sizeof(mobType*));

 // read the main data files

  if (!newZoneWin32)
  {
    readWorldFile(NULL);
    readObjectFile(NULL);
    readMobFile(NULL);
    readZoneFile(NULL);
    readQuestFile(NULL);
    readShopFile(NULL);
    readInventoryFile();

    checkShopsOnLoad();
  }

  readDefaultsFromFiles(MAIN_DEFAULT_NAME); // first, read "default defaults"...

  if (!newZoneWin32)
  {
    readDefaultsFromFiles(getMainZoneNameStrn()); // then, zone name...
  }

 // quest/shop vars used to determine if we need to delete .qst/.shp file
 // when user saves - otherwise we'd have remnants if some existed before
 // and user deleted all quest/shops

#ifdef _WIN32
  clrscr(7, 0);  // Windows init done earlier
#endif
  startInit();

 // set g_currentRoom

 // check startRoom var - if set and room is valid, stick user there

  if (getStartRoomActiveVal() && varExists(g_varHead, VAR_STARTROOM_NAME) && roomExists(getRoomStartVal()))
  {
    g_currentRoom = findRoom(getRoomStartVal());
  }
  else
  {
    g_currentRoom = findRoom(getLowestRoomNumber());
  }

 // display the initial room and prompt

  displayCurrentRoom();
  displayPrompt();

 // prompt user until interpCommands() wants to quit

  while (!interpCommands(NULL, true)) {}

  return 0;
}
