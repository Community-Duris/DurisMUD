//
//  File: init.cpp       originally part of durisEdit
//
//  Usage: various functions used when initializing at startup
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

#include "fh.h"
#include "types.h"
#include "keys.h"
#include "vardef.h"
#include "defines.h"

#include "graphcon.h"



flagDef g_npc_class_bits[CLASS_COUNT + 1];
flagDef g_race_names[RACE_PLAYER_MAX + 1];

extern room **g_roomLookup;
extern zone g_zoneRec;
extern uint g_numbLookupEntries, g_highestRoomNumber, g_lowestRoomNumber, g_numbRooms;
extern bool g_madeChanges;
extern variable *g_varHead;
extern const char *g_exitnames[];

extern "C" struct class_names class_names_table[];
extern "C" struct race_names race_names_table[];
extern void initialize_skills();

//
// startInit : Initializes after reading the data files
//

void startInit(void)
{
  room *roomPtr;
  char strn[64] = "";
  const uint highRoomNumb = getHighestRoomNumber();


  initialize_skills();

  for (uint i = 0; i < CLASS_COUNT; i++) 
  {
    g_npc_class_bits[i].flagShort = class_names_table[i + 1].normal;
    g_npc_class_bits[i].flagLong = class_names_table[i + 1].ansi;
    g_npc_class_bits[i].editable = 1;
    g_npc_class_bits[i].defVal = 0;
  }

  g_npc_class_bits[CLASS_COUNT].flagShort = 0;

  for (uint i = 0; i < RACE_PLAYER_MAX; i++) 
  {
    g_race_names[i].flagShort = race_names_table[i + 1].no_spaces;
    g_race_names[i].flagLong = race_names_table[i + 1].ansi;
    g_race_names[i].editable = 1;
    g_race_names[i].defVal = 0;
  }

  g_race_names[RACE_PLAYER_MAX].flagShort = 0;
  
 // make sure that key vnums specified for exits are valid (only checks
 // if the "check vnums to make sure they're valid" var is true)

  if (getVnumCheckVal())
  {
    for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
    {
      if (roomExists(roomNumb))
      {
        roomPtr = findRoom(roomNumb);

        for (uint i = 0; i < NUMB_EXITS; i++)
        {
          checkRoomExitKey(roomPtr->exits[i], roomPtr, g_exitnames[i]);
        }
      }
    }
  }

 // clear the screen to fg 7, bg 0

  clrscr(7, 0);
  _settextcolor(7);
  _setbkcolor(0);

  roomPtr = findRoom(getLowestRoomNumber());

 // if no low room can be found, it's a new zone, so create an initial room

  if (!roomPtr)
  {
   // clear zone rec first so that room's zone # gets set to 0

    memset(&g_zoneRec, 0, sizeof(zone));

   // set zone defaults

    g_zoneRec.lifeLow = 40;
    g_zoneRec.lifeHigh = 50;
    g_zoneRec.resetMode = ZONE_RESET_ALWAYS;
    g_zoneRec.zoneDiff = 1;

    uint vnum = 1;

    while (true)
    {
      editUIntVal(&vnum, false, "&+CStarting new zone - specify a starting vnum for rooms, objs, and mobs: ");

      if (vnum < g_numbLookupEntries)
        break;
    }

    roomPtr = createRoom(true, vnum, false);

    if (!roomPtr)
      exit(1);

    _outtext("\n");

   // get name for zone straight off

    strcpy(g_zoneRec.zoneName, "The Horrible Tracts of the Unnamed");

    editStrnVal(g_zoneRec.zoneName, MAX_ZONENAME_LEN, "&+CEnter a name for the zone: ");

    g_madeChanges = true;
  }

 // create a master keyword list and editable list for each room

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      roomPtr->masterListHead = createMasterKeywordList(roomPtr);
      roomPtr->editableListHead = createEditableList(roomPtr);
    }
  }

  _outtext("\n");

 // check that all rooms have the same zone value as the zone itself
 // (mud apparently doesn't actually use the room value, mind you)

  verifyZoneFlags();

 // if any room vnum is >= 100000, we have a map zone, so set variable
 // appropriately

  if (highRoomNumb >= 100000)
  {
    setVarBoolVal(&g_varHead, VAR_ISMAPZONE_NAME, true, true);
  }

 // create the initial g_promptString

  createPrompt();

 // clear the screen

  clrscr(7, 0);
}
