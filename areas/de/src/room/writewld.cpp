//
//  File: writewld.cpp   originally part of durisEdit
//
//  Usage: functions for writing room info to the .wld info
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

#include "fh.h"
#include "types.h"

#include "exit.h"
#include "room.h"



extern bool g_readFromSubdirs;
extern uint g_numbRooms, g_numbExits;
extern zone g_zoneRec;

//
// writeWorldExit : Writes a room exit to worldFile
//
//   *exitNode : pointer to exit record
//   *exitStrn : string that identifies direction exit leads
//  *worldFile : file to write to
//

void writeWorldExittoFile(FILE *worldFile, const roomExit *exitNode, const char *exitStrn)
{
  char strn[MAX_EXITKEY_LEN + 64];


 // first, the exit identifier

  fputs(exitStrn, worldFile);

 // next, write the exit description

  writeStringNodes(worldFile, exitNode->exitDescHead);

 // terminate it

  fputs("~\n", worldFile);

 // write the keyword list

  createKeywordString(exitNode->keywordListHead, strn, MAX_EXITKEY_LEN + 63);
  lowstrn(strn);

  strcat(strn, "\n");

  fputs(strn, worldFile);

 // finally, write the door flag and other info

  fprintf(worldFile, "%d %d %d\n",
          getWorldDoorType(exitNode), exitNode->keyNumb, exitNode->destRoom);
}


//
// writeRoomtoFile : Writes a room to a file
//

void writeRoomtoFile(FILE *worldFile, const room *room)
{
  char strn[512];
  extraDesc *descNode;
  int i;


 // first, the room number

  fprintf(worldFile, "#%u\n", room->roomNumber);

 // next, the name of the room

  fprintf(worldFile, "%s~\n", room->roomName);

 // next, the room description

  writeStringNodes(worldFile, room->roomDescHead);

 // terminate the description

  fputs("~\n", worldFile);

 // next, write the zone number, room flags, and sector type info

  if (getIsMapZoneVal() || g_zoneRec.miscBits.zoneMiscBits.map)
    fprintf(worldFile, "%u %u %u %u\n", room->zoneNumber,
                                        room->roomFlags,
                                        room->sectorType,
                                        room->resourceInfo);
  else
    fprintf(worldFile, "%u %u %u\n", room->zoneNumber,
                                     room->roomFlags,
                                     room->sectorType);

 // next, the exit info

  for (i = 0; i < NUMB_EXITS; i++)
  {
    if (room->exits[i])
    {
      sprintf(strn, "D%u\n", i);
      writeWorldExittoFile(worldFile, room->exits[i], strn);
    }
  }

 // finally, write all the extra descs

  descNode = room->extraDescHead;

  while (descNode)
  {
    fputs("E\n", worldFile);

    writeExtraDesctoFile(worldFile, descNode);

    descNode = descNode->Next;
  }

 // more finally, write fall percentage, etc.

  if (room->fallChance)
  {
    fprintf(worldFile, "F\n%u\n", room->fallChance);
  }

  if (room->manaFlag || room->manaApply)
  {
    fprintf(worldFile, "M\n%u %d\n", room->manaFlag, room->manaApply);
  }

  if (room->current)
  {
    fprintf(worldFile, "C\n%u %u\n", room->current, room->currentDir);
  }

 // terminate the room info

  fputs("S\n", worldFile);
}


//
// writeWorldFile : Write the world file - contains all the rooms
//

void writeWorldFile(char *filename)
{
  FILE *worldFile;
  char worldFilename[512] = "";
  char strn[512];

  uint numb = getLowestRoomNumber();
  const uint highest = getHighestRoomNumber();


 // assemble the filename of the world file

  if (g_readFromSubdirs) 
    strcpy(worldFilename, "wld/");

  if (filename) 
    strcat(worldFilename, filename);
  else 
    strcat(worldFilename, getMainZoneNameStrn());

  strcat(worldFilename, ".wld");

 // open the world file for writing

  if ((worldFile = fopen(worldFilename, "wt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(worldFilename);
    _outtext(" for writing - aborting\n");

    return;
  }

  sprintf(strn, "Writing %s - %u room%s, %u exit%s\n",
          worldFilename, g_numbRooms, plural(g_numbRooms),
                         g_numbExits, plural(g_numbExits));

  _outtext(strn);

  while (numb <= highest)
  {
    const room *roomPtr = findRoom(numb);

    if (roomPtr) 
      writeRoomtoFile(worldFile, roomPtr);

    numb++;
  }

  fclose(worldFile);
}
