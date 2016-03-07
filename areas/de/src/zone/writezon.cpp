//
//  File: writezon.cpp   originally part of durisEdit
//
//  Usage: functions for writing zone info to the .zon file
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

#include "fh.h"
#include "types.h"
#include "de.h"

#include "writezon.h"



extern zone g_zoneRec;
extern bool g_readFromSubdirs;
extern uint g_numbObjs, g_numbMobs;
extern char *g_exitnames[];


//
// addExitWrittentoList : Adds an exitWritten node onto the end of a linked
//                        list of exitWrittens
//
//    **exitHead : pointer to pointer to head of list
//    *exittoAdd : pointer to node to add
//

void addExitWrittentoList(exitWritten **exitHead, exitWritten *exittoAdd)
{
  exitWritten *exitNode;


  if (!exittoAdd) 
    return;

  if (!*exitHead) 
  {
    *exitHead = exittoAdd;
  }
  else
  {
    exitNode = *exitHead;

    while (exitNode->Next)
      exitNode = exitNode->Next;

    exitNode->Next = exittoAdd;
  }
}


//
// deleteExitWrittenList : Deletes a linked list of exitWrittens
//
//    *exitHead : pointer to head of list
//

void deleteExitWrittenList(exitWritten *exitHead)
{
  exitWritten *nextExit;


  while (exitHead)
  {
    nextExit = exitHead->Next;

    delete exitHead;

    exitHead = nextExit;
  }
}


//
// checkExitWrittenforExit : Checks a list of exitWrittens for an exit
//                           that matches the room number and exit direction
//                           specified - returns the address of the first
//                           matching node
//
//    *exitHead : head of list to check
//
//     roomNumb : src room numb to match
//     exitNumb : exit dir to match
//

exitWritten *checkExitWrittenListforExit(exitWritten *exitHead, const int roomNumb, const char exitNumb)
{
  exitWritten *exitNode = exitHead;

  while (exitNode)
  {
    if ((exitNode->roomNumber == roomNumb) && (exitNode->exitNumber == exitNumb)) 
      return exitNode;

    exitNode = exitNode->Next;
  }

  return NULL;
}


//
// checkForPairedExit : Tries to find the "paired exit" for the exit specified
//
//    *exitNode : pointer to exit node to find "pair" of
//  srcRoomNumb : room number exit is in
//     *exitDir : direction exit leads in (set by findCorrespondingExit)
//

roomExit *checkForPairedExit(const roomExit *exitNode, const int srcRoomNumb, char *exitDir)
{
 // look for an exit that is the "opposite" of this one

  return findCorrespondingExit(exitNode->destRoom, srcRoomNumb, exitDir);
}


//
// writeAllExitsPairedRedundant :
//      Redundant stuff used for each exit in writeAllExitsPaired - returns
//      TRUE if a door state has been changed, not currently used, may be
//      useful someday
//
//   *zoneFile : file to write exit pair commands to
//       *room : room that source exit is in
//
// **exitWHead : pointer to pointer to start of exitWritten list
//   *exitNode : source exit being written
//
//   *exitName : "name" of source exit - north, south, etc
//    exitChar : key that should be hit to set door state for this exit
//               (north = N, etc.)
//
//     origDir : direction of *exitNode
//

bool writeAllExitsPairedRedundant(FILE *zoneFile, const room *room, exitWritten **exitWHead,
                                  roomExit *exitNode, const char *exitName, const char exitChar,
                                  const char origDir)
{
  roomExit *pairedExit;
  exitWritten *exitWrit;
  char exitDir, upExitName[64];
  char strn[2048];
  bool changedDoorState = false;
  int zoneType, worldType, zoneBits, worldBits;
  int pzoneType, pworldType, pzoneBits, pworldBits;
  usint ch;


  if (!room || !exitNode) 
    return false;

  strcpy(upExitName, exitName);
  upExitName[0] = toupper(upExitName[0]);

  zoneType = exitNode->zoneDoorState & 3;
  zoneBits = exitNode->zoneDoorState & 12;

  worldType = exitNode->worldDoorType & 3;
  worldBits = exitNode->worldDoorType & 12;

 // if exit has a door state, do stuff to it, but only if it hasn't already
 // been written

  if (((zoneType || worldType) || ((zoneBits & 8) || (worldBits & 8))) &&
       !checkExitWrittenListforExit(*exitWHead, room->roomNumber, origDir))
  {
   // this side of door has a type of some kind, check that there is an opposite side

    pairedExit = checkForPairedExit(exitNode, room->roomNumber, &exitDir);

    if (!pairedExit)
    {
      sprintf(strn, "\n\n"
"Warning: %s exit in room #%u has a door type/state set, but has no\n"
"         corresponding exit on the opposite side.  Thus, this exit's door\n"
"         state and type must be set to 0.\n\n"
"Press a key to continue..\n\n", 
              upExitName, room->roomNumber);

      displayAnyKeyPrompt(strn);

      exitNode->zoneDoorState = 0 | zoneBits;

      if (exitNode->zoneDoorState & 8) 
        exitNode->zoneDoorState ^= 8;

      exitNode->worldDoorType = 0 | worldBits;

      if (exitNode->worldDoorType & 8) 
        exitNode->worldDoorType ^= 8;

      return true;
    }

    zoneType = exitNode->zoneDoorState & 3;
    zoneBits = exitNode->zoneDoorState & 12;

    worldType = exitNode->worldDoorType & 3;
    worldBits = exitNode->worldDoorType & 12;

    pzoneType = pairedExit->zoneDoorState & 3;
    pzoneBits = pairedExit->zoneDoorState & 12;

    pworldType = pairedExit->worldDoorType & 3;
    pworldBits = pairedExit->worldDoorType & 12;

   // exit has an opposite side, check that opposite side is set to a valid type

    if (!((zoneBits & 8) || (pzoneBits & 8)))
    {
      if ((pzoneType && !zoneType) || (!pzoneType && zoneType))
      {
        sprintf(strn, "\n"
  "Warning: %s exit in room #%u has a non-zero door state set in the\n"
  "         .zon (%u) but the exit on the opposite side has a door state of\n"
  "         %u (.zon).\n\n"
  "Either the opposite exit must be set to the same door state as the %s\n"
  "exit, or the %s exit must be set to that of the opposite exit.\n\n"
  "Set opposite equal to %s (O), or set %s equal to opposite (%c)? ",

  upExitName, room->roomNumber, zoneType, pzoneType,
  exitName, exitName, exitName, exitName, exitChar);

        _outtext(strn);

        do
        {
          ch = toupper(getkey());
        } while ((ch != 'O') && (ch != exitChar));

        sprintf(strn, "%c\n\n", (char)ch);
        _outtext(strn);

        if (ch == 'O') 
          pairedExit->zoneDoorState = zoneType | pzoneBits;
        else 
          exitNode->zoneDoorState = pzoneType | zoneBits;

        changedDoorState = true;
      }

      if ((pworldType && !worldType) || (!pworldType && worldType))
      {
        sprintf(strn, "\n"
  "Warning: %s exit in room #%u has a non-zero door type set in the\n"
  "         .wld (%u) but the exit on the opposite side has a door type of\n"
  "         %u (.wld).\n\n"
  "Either the opposite exit must be set to the same door type as the %s\n"
  "exit, or the %s exit must be set to that of the opposite exit.\n\n"
  "Set opposite equal to %s (O), or set %s equal to opposite (%c)? ",

  upExitName, room->roomNumber, worldType, pworldType,
  exitName, exitName, exitName, exitName, exitChar);

        _outtext(strn);

        do
        {
          ch = toupper(getkey());
        } while ((ch != 'O') && (ch != exitChar));

        sprintf(strn, "%c\n\n", (char)ch);
        _outtext(strn);

        if (ch == 'O') 
          pairedExit->worldDoorType = worldType | pworldBits;
        else 
          exitNode->worldDoorType = pworldType | worldBits;

        changedDoorState = true;
      }
    }

   // write paired exits

    // first, write "original" exit

    fprintf(zoneFile, "D 0 %u %u %d 100 0 0 0\n",
            room->roomNumber, origDir, getZoneDoorState(exitNode));

    exitWrit = new(std::nothrow) exitWritten;
    if (!exitWrit)
    {
     // on failed alloc, duplicate exit state commands will possibly be written, but at least you'll get your 
     // zone

      displayAllocError("exitWritten", "writeAllExitsPairedRedundant");
    }
    else
    {
      exitWrit->roomNumber = room->roomNumber;
      exitWrit->exitNumber = origDir;
      exitWrit->Next = NULL;

      addExitWrittentoList(exitWHead, exitWrit);
    }

    getDescExitStrnforZonefile(room, exitNode, strn, exitName);
    fputs(strn, zoneFile);

   // now, "paired" exit

    fprintf(zoneFile, "D 0 %u %u %d 100 0 0 0\n",
            exitNode->destRoom, exitDir, getZoneDoorState(pairedExit));

    exitWrit = new(std::nothrow) exitWritten;
    if (!exitWrit)
    {
     // on failed alloc, duplicate exit state commands will possibly be written, but at least you'll get your 
     // zone

      displayAllocError("exitWritten", "writeAllExitsPairedRedundant");
    }
    else
    {
      exitWrit->roomNumber = exitNode->destRoom;
      exitWrit->exitNumber = exitDir;
      exitWrit->Next = NULL;

      addExitWrittentoList(exitWHead, exitWrit);
    }

   // descriptive string of what leads where

    getDescExitStrnforZonefile(findRoom(exitNode->destRoom), getExitNode(exitNode->destRoom, exitDir), strn,
                               getExitStrn(exitDir));

    fputs(strn, zoneFile);

    return changedDoorState;
  }

  return false;
}


//
// writeAllExitsPaired : Runs through the room list, writing all exits
//                       with door states and pairing them appropriately.
//
//   *zoneFile : file to write set door state commands to
//

void writeAllExitsPaired(FILE *zoneFile)
{
  exitWritten *exitWHead = NULL;
  room *room;
  int i;
  uint numb, high;


  for( numb = getLowestRoomNumber(), high = getHighestRoomNumber(); numb <= high; numb++)
  {
    if( !(room = findRoom(numb)) )
    {
      continue;
    }

    for( i = 0; i < NUMB_EXITS; i++ )
    {
      writeAllExitsPairedRedundant(zoneFile, room, &exitWHead, room->exits[i],
        g_exitnames[i], toupper(g_exitnames[i][0]), i);
    }
  }

  deleteExitWrittenList(exitWHead);
}


//
// writeAllObjInsideSpecific : called by writeAllObjInside, takes arg as to whether to write all conts or
//                             non-conts
//

void writeAllObjInsideSpecific(FILE *zoneFile, const objectHere *insidePtr,
                               const uint containerNumb, const uint extraSpaces,
                               const bool writeConts)
{
  char preComment[256], fullLine[MAX_OBJSNAME_LEN + 512], objName[MAX_OBJSNAME_LEN + 1];
  size_t i;


 // write objects inside original object

  while (insidePtr)
  {
    const bool hasContents = (insidePtr->objInside != NULL);

    if (writeConts == hasContents)
    {
      sprintf(preComment, "P 1 %u %u %u %u 0 0 0",
              insidePtr->objectNumb,
              getNumbEntities(ENTITY_OBJECT, insidePtr->objectNumb, true),
              containerNumb, insidePtr->randomChance);

      for (i = strlen(preComment); i < COMMENT_POS; i++)
      {
        preComment[i] = ' ';
      }

      preComment[i] = '\0';

      strcpy(objName, getObjShortName(insidePtr->objectPtr));
      remColorCodes(objName);

      sprintf(fullLine, "%s*   ", preComment);
      for (i = 0; i < extraSpaces; i++) 
        strcat(fullLine, " ");

      strcat(fullLine, objName);
      strcat(fullLine, "\n");

      fputs(fullLine, zoneFile);

     // write obj inside lists for items inside this object

      if (insidePtr->objInside)
      {
        uint actualExtraSpaces;

        if (extraSpaces + 2 <= MAX_ZONEFILE_EXTRA_SPACES)
          actualExtraSpaces = extraSpaces + 2;
        else
          actualExtraSpaces = extraSpaces;

        writeAllObjInside(zoneFile, insidePtr->objInside, insidePtr->objectNumb, actualExtraSpaces);
      }
    }

    insidePtr = insidePtr->Next;
  }
}


//
// writeAllObjInside
//

void writeAllObjInside(FILE *zoneFile, const objectHere *insidePtr,
                       const uint containerNumb, const uint extraSpaces)
{
  if (!insidePtr) 
    return;

 // first write objects with nothing inside them, then write objects with something inside them,
 // to allow multiple loads of the same containing object type to match the intent of the zone
 // creator as closely as possible

  writeAllObjInsideSpecific(zoneFile, insidePtr, containerNumb, extraSpaces, false);
  writeAllObjInsideSpecific(zoneFile, insidePtr, containerNumb, extraSpaces, true);
}


//
// writeLoadObjectCmd : Writes the "load object" command for the specified
//                      objHere node
//
//       *room : room that contains the object
//    *objHere : objHere node that "is" the object
//   *zoneFile : file to write to
//

void writeLoadObjectCmd(const room *room, const objectHere *objHere, FILE *zoneFile)
{
  size_t i;
  char objName[MAX_OBJSNAME_LEN + 1], preComment[MAX_OBJSNAME_LEN + 256], comment[MAX_OBJSNAME_LEN + 128];


  strcpy(objName, getObjShortName(objHere->objectPtr));
  remColorCodes(objName);

  sprintf(preComment, "O 0 %u %u %u %u 0 0 0",
          objHere->objectNumb,
          getNumbEntities(ENTITY_OBJECT, objHere->objectNumb, true),
          room->roomNumber, objHere->randomChance);

  for (i = strlen(preComment); i < COMMENT_POS; i++)
  {
    preComment[i] = ' ';
  }

  preComment[i] = '\0';

  sprintf(comment, "* %s\n", objName);

  strcat(preComment, comment);

  fputs(preComment, zoneFile);
}


//
// writeLoadMobCommand : Writes a "load mob" command to the zone file
//
//      *room : room that mob is in
//  *mobHNode : mob to load
//  *zoneFile : file to write info to
//

void writeLoadMobCommand(const room *room, const mobHere *mobHNode, FILE *zoneFile)
{
  size_t i;
  char mobName[MAX_MOBSNAME_LEN + 1], preComment[MAX_MOBSNAME_LEN + 256], comment[MAX_MOBSNAME_LEN + 128];


  strcpy(mobName, getMobShortName(mobHNode->mobPtr));
  remColorCodes(mobName);

  sprintf(preComment, "M 0 %u %u %u %u 0 0 0",
          mobHNode->mobNumb,
          getNumbEntities(ENTITY_MOB, mobHNode->mobNumb, true),
          room->roomNumber, mobHNode->randomChance);

  for (i = strlen(preComment); i < COMMENT_POS; i++)
  {
    preComment[i] = ' ';
  }

  preComment[i] = '\0';

  sprintf(comment, "* %s\n", mobName);

  strcat(preComment, comment);

  fputs(preComment, zoneFile);
}


//
// writeEquipMobCommand : Write a "equip mob with obj" command to the zone file
//
//   *mobEquip : object to equip mob with
//   *zoneFile : file to write command to
//   eqpos : pos equipment is in, may get altered
//

void writeEquipMobCommand(const objectHere *mobEquip, FILE *zoneFile, const uint origeqpos)
{
  size_t i;
  uint eqpos;
  char equipName[MAX_OBJSNAME_LEN + 1], preComment[MAX_OBJSNAME_LEN + 256], comment[MAX_OBJSNAME_LEN + 128];


  if (!mobEquip) 
    return;

  strcpy(equipName, getObjShortName(mobEquip->objectPtr));
  remColorCodes(equipName);

 // alter eq pos as necessary

  eqpos = getZoneCommandEquipPos(origeqpos);

  sprintf(preComment, "E 1 %u %u %u %u 0 0 0",
          mobEquip->objectNumb,
          getNumbEntities(ENTITY_OBJECT, mobEquip->objectNumb, true), eqpos,
          mobEquip->randomChance);

  for (i = strlen(preComment); i < COMMENT_POS; i++)
  {
    preComment[i] = ' ';
  }

  preComment[i] = '\0';

  sprintf(comment, "*   %s\n", equipName);

  strcat(preComment, comment);

  fputs(preComment, zoneFile);
}


//
// writeGiveMobCommand : Write a "give obj to mob" command to the zone file
//
//     *mobObj : object to give
//   *zoneFile : file to write command to
//

void writeGiveMobCommand(const objectHere *mobObj, FILE *zoneFile)
{
  size_t i;
  char objName[MAX_OBJSNAME_LEN + 1], preComment[MAX_OBJSNAME_LEN + 256], comment[MAX_OBJSNAME_LEN + 128];


  strcpy(objName, getObjShortName(mobObj->objectPtr));
  remColorCodes(objName);

  sprintf(preComment, "G 1 %u %u 0 %u 0 0 0",
          mobObj->objectNumb,
          getNumbEntities(ENTITY_OBJECT, mobObj->objectNumb, true),
          mobObj->randomChance);

  for (i = strlen(preComment); i < COMMENT_POS; i++)
  {
    preComment[i] = ' ';
  }

  preComment[i] = '\0';

  sprintf(comment, "*    %s\n", objName);

  strcat(preComment, comment);

  fputs(preComment, zoneFile);
}


//
// writeRideMobCommand : Writes a "ride mob" command to the zone file along
//                       with all eq loads
//
//  *mobHNode : mob riding something
//  *zoneFile : file to write info to
//

void writeRideMobCommand(const room *room, const mobHere *mobHNode, FILE *zoneFile)
{
  size_t i;
  char mobName[MAX_MOBSNAME_LEN + 1], preComment[MAX_MOBSNAME_LEN + 256], comment[MAX_MOBSNAME_LEN + 128];
  const mobHere *mount;
  const objectHere *mobEquip, *mobObj;


  if (!mobHNode || !mobHNode->riding) 
    return;

  mount = mobHNode->riding;

  strcpy(mobName, getMobShortName(mount->mobPtr));
  remColorCodes(mobName);

  sprintf(preComment, "R 1 %u %u %u %u 0 0 0",
          mount->mobNumb,
          getNumbEntities(ENTITY_MOB, mount->mobNumb, true),
          room->roomNumber, mount->randomChance);

  for (i = strlen(preComment); i < COMMENT_POS; i++)
  {
    preComment[i] = ' ';
  }

  preComment[i] = '\0';

  sprintf(comment, "* %s (being ridden)\n", mobName);

  strcat(preComment, comment);

  fputs(preComment, zoneFile);

  for (uint eq = WEAR_LOW; eq <= WEAR_TRUEHIGH; eq++)
  {
    mobEquip = mount->equipment[eq];

    if (mobEquip)
    {
      writeEquipMobCommand(mobEquip, zoneFile, eq);
      writeAllObjInside(zoneFile, mobEquip->objInside, mobEquip->objectNumb, 0);
    }
  }

  mobObj = mount->inventoryHead;
  while (mobObj)
  {
    writeGiveMobCommand(mobObj, zoneFile);

    writeAllObjInside(zoneFile, mobObj->objInside, mobObj->objectNumb, 0);

    mobObj = mobObj->Next;
  }
}


//
// writeMobFollowerCommands : writes "F" lines for all mobs following a
//                            certain leader
//
//  *mobLeader : leader
//   *zoneFile : zone file
//

void writeMobFollowerCommands(const mobHere *mobLeader, FILE *zoneFile)
{
  char mobName[MAX_MOBSNAME_LEN + 1], preComment[MAX_MOBSNAME_LEN + 256], comment[MAX_MOBSNAME_LEN + 128];
  const room *room;
  const mobHere *follower;
  const objectHere *mobEquip, *mobObj;
  uint roomNumb;
  const uint high = getHighestRoomNumber();
  size_t i;


  if (!mobLeader) 
    return;

  for (roomNumb = getLowestRoomNumber(); roomNumb <= high; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room = findRoom(roomNumb);

      follower = room->mobHead;
      while (follower)
      {
        if (follower->following == mobLeader)
        {
          strcpy(mobName, getMobShortName(follower->mobPtr));
          remColorCodes(mobName);

          sprintf(preComment, "F 1 %u %u %u %u 0 0 0",
                  follower->mobNumb,
                  getNumbEntities(ENTITY_MOB, follower->mobNumb, true),
                  room->roomNumber, follower->randomChance);

          for (i = strlen(preComment); i < COMMENT_POS; i++)
          {
            preComment[i] = ' ';
          }

          preComment[i] = '\0';

          sprintf(comment, "* %s (following)\n", mobName);

          strcat(preComment, comment);

          fputs(preComment, zoneFile);

          for (uint eq = WEAR_LOW; eq <= WEAR_TRUEHIGH; eq++)
          {
            mobEquip = follower->equipment[eq];

            if (mobEquip)
            {
              writeEquipMobCommand(mobEquip, zoneFile, eq);
              writeAllObjInside(zoneFile, mobEquip->objInside, mobEquip->objectNumb, 0);
            }
          }

          mobObj = follower->inventoryHead;
          while (mobObj)
          {
            writeGiveMobCommand(mobObj, zoneFile);

            writeAllObjInside(zoneFile, mobObj->objInside, mobObj->objectNumb, 0);

            mobObj = mobObj->Next;
          }
        }

        follower = follower->Next;
      }
    }
  }
}


//
// writeZoneFile : Write the zone file - controls what is loaded where
//

void writeZoneFile(const char *filename)
{
  FILE *zoneFile;
  char zoneFilename[512] = "";
  char strn[512], i;

  const room *roomPtr;

  const objectHere *objHere, *mobEquip, *mobObj;
  const mobHere *mobHNode, *mobLeader = NULL;

  const uint highRoomNumb = getHighestRoomNumber();


 // assemble the filename of the zone file

  if (g_readFromSubdirs) 
    strcpy(zoneFilename, "zon/");

  if (filename) 
    strcat(zoneFilename, filename);
  else 
    strcat(zoneFilename, getMainZoneNameStrn());

  strcat(zoneFilename, ".zon");


 // open the zone file for writing

  if ((zoneFile = fopen(zoneFilename, "wt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(zoneFilename);
    _outtext(" for writing - aborting\n");

    return;
  }

  sprintf(strn, "Writing %s - %u object%s, %u mob%s\n",
          zoneFilename, g_numbObjs, plural(g_numbObjs),
                        g_numbMobs, plural(g_numbMobs));

  _outtext(strn);

 // write the zone number and name

  fprintf(zoneFile, "#%u\n", g_zoneRec.zoneNumber);
  fprintf(zoneFile, "%s~\n", g_zoneRec.zoneName);

 // now write the highest room numb, reset mode, zone bits, lifespan info,
 // and zone difficulty

  fprintf(zoneFile, "%u %u %u %u %u %u\n",
          getHighestRoomNumber(), g_zoneRec.resetMode,
          g_zoneRec.miscBits.longIntFlags, g_zoneRec.lifeLow, g_zoneRec.lifeHigh,
          g_zoneRec.zoneDiff);

  if( g_zoneRec.miscBits.zoneMiscBits.map )
  {
    fprintf(zoneFile, "%u %u\n", g_zoneRec.mapWidth, g_zoneRec.mapHeight);
  } 

 // write comment

  fputs(
"*\n"
"* Zone edited with durisEdit v" DE_VERSION "\n"
"*\n", zoneFile);

 // write door states

//  fputs("*\n*\n* door states\n*\n", zoneFile);
  fputs("*\n", zoneFile);

  checkAllExits();  // check exit type vs flags

  writeAllExitsPaired(zoneFile);

 // object loading

  fputs("*\n", zoneFile);
//  fputs("*\n*\n* object loading\n*\n", zoneFile);

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      objHere = roomPtr->objectHead;

      while (objHere)
      {
        writeLoadObjectCmd(roomPtr, objHere, zoneFile);

        writeAllObjInside(zoneFile, objHere->objInside, objHere->objectNumb, 0);

        objHere = objHere->Next;
      }
    }
  }

 // mob loading

  fputs("*\n", zoneFile);

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

     // make sure to write non-following mob first

      mobHNode = roomPtr->mobHead;

      while (mobHNode)
      {
       // prevent dupe load commands

        if (!mobHNode->following)
        {
          if (!mobHNode->riddenBy)
          {
            writeLoadMobCommand(roomPtr, mobHNode, zoneFile);

            for (i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
            {
              mobEquip = mobHNode->equipment[i];

              if (mobEquip)
              {
                writeEquipMobCommand(mobEquip, zoneFile, i);

                writeAllObjInside(zoneFile, mobEquip->objInside, mobEquip->objectNumb, 0);
              }
            }

            mobObj = mobHNode->inventoryHead;
            while (mobObj)
            {
              writeGiveMobCommand(mobObj, zoneFile);

              writeAllObjInside(zoneFile, mobObj->objInside, mobObj->objectNumb, 0);

              mobObj = mobObj->Next;
            }
          }

          if (mobHNode->riding)
          {
            writeRideMobCommand(roomPtr, mobHNode, zoneFile);  // also writes eq loads
          }

          writeMobFollowerCommands(mobHNode, zoneFile);  // also writes eq loads

          mobLeader = mobHNode;
          break;
        }

        mobHNode = mobHNode->Next;
      }

     // write rest of mobs

      mobHNode = roomPtr->mobHead;

      while (mobHNode)
      {
       // prevent dupe load commands

        if (mobHNode != mobLeader)
        {
          if (!mobHNode->following && !mobHNode->riddenBy)
          {
            writeLoadMobCommand(roomPtr, mobHNode, zoneFile);

            for (i = WEAR_LOW; i <= WEAR_TRUEHIGH; i++)
            {
              mobEquip = mobHNode->equipment[i];

              if (mobEquip)
              {
                writeEquipMobCommand(mobEquip, zoneFile, i);

                writeAllObjInside(zoneFile, mobEquip->objInside, mobEquip->objectNumb, 0);
              }
            }

            mobObj = mobHNode->inventoryHead;

            while (mobObj)
            {
              writeGiveMobCommand(mobObj, zoneFile);

              writeAllObjInside(zoneFile, mobObj->objInside, mobObj->objectNumb, 0);

              mobObj = mobObj->Next;
            }
          }

          if (mobHNode->riding)
          {
            writeRideMobCommand(roomPtr, mobHNode, zoneFile);  // also writes eq loads
          }

          writeMobFollowerCommands(mobHNode, zoneFile);  // also writes eq loads
        }

        mobHNode = mobHNode->Next;
      }
    }
  }

 // write the end token and close the file

  fputs("*\nS\n", zoneFile);

  fclose(zoneFile);
}
