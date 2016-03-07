//
//  File: dispmob.cpp    originally part of durisEdit
//
//  Usage: functions for displaying mob information
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

#include "types.h"
#include "fh.h"
#include "mob.h"

extern "C" flagDef action_bits[], affected1_bits[], affected2_bits[], affected3_bits[], affected4_bits[], 
                   aggro_bits[], aggro2_bits[], aggro3_bits[];
extern "C" const struct race_names race_names_table[];                      
extern "C" const struct class_names class_names_table[];
extern room *g_currentRoom;


//
// getClassString : return string of all of mob's classes
//

char *getClassString(const mobType *mob, char *strn, const size_t intMaxLen)
{
  size_t len = 0;
  uint i;

  const uint class_code = mob->mobClass;

  strn[0] = '\0';

  for (i = 0; i < CLASS_COUNT; i++) 
  {
    if (class_code & (1 << i))
    {
      strncpy(strn + len, class_names_table[i + 1].ansi, intMaxLen - len);
      
      strn[intMaxLen] = '\0';

      len += strlen(class_names_table[i + 1].ansi);

      if (len < intMaxLen)
      {
        strcat(strn, " ");
        len++;
      }

      if (len > intMaxLen)
        return strn;
    }
  }

 // remove trailing space, or set to 'none' if no classes set

  if (len > 0)
    strn[len - 1] = '\0';
  else
    strcpy(strn, "none");

  return strn;
}


//
// displayMobTypeList : Displays the list of mob types loaded into
//                      DE, starting at mobHead
//

void displayMobTypeList(const char *strn)
{
  const mobType *mob;
  size_t lines = 1;
  uint numb, low = getLowestMobNumber(), high = getHighestMobNumber();
  char outStrn[512];
  bool foundMob = false, vnum, listAll = false;


  if (noMobTypesExist())
  {
    _outtext("\nThere are currently no mob types.\n\n");

    return;
  }

  if (!strn || (strn[0] == '\0'))
  {
    listAll = true;
  }
  else
  {
    vnum = strnumer(strn);
    numb = strtoul(strn, NULL, 10);
  }

  _outtext("\n\n");

  for (uint mobNumb = low; mobNumb <= high; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mob = findMob(mobNumb);

      if (listAll ||
          (vnum && (numb == mobNumb)) ||
          scanKeyword(strn, mob->keywordListHead))
      {
        sprintf(outStrn, "%s&n (#%u)\n", mob->mobShortName, mobNumb);

        foundMob = true;

        if (checkPause(outStrn, lines))
          return;
      }
    }
  }

  if (!foundMob) 
    _outtext("No matching mob types found.\n");

  _outtext("\n");
}


//
// displayMobHereList
//

void displayMobHereList(void)
{
  const room *room;
  const mobHere *mob;
  char strn[MAX_MOBSNAME_LEN + MAX_ROOMNAME_LEN + 256];
  size_t lines = 1;
  bool foundOne = false;
  uint numb = getLowestRoomNumber();
  const uint high = getHighestRoomNumber();


  _outtext("\n\n");

  for (; numb <= high; numb++)
  {
    if (!roomExists(numb))
      continue;

    room = findRoom(numb);

    mob = room->mobHead;

    while (mob)
    {
      sprintf(strn, "%s&n (#%u) in %s&n (#%u)\n",
              getMobShortName(mob->mobPtr), mob->mobNumb,
              room->roomName, room->roomNumber);

      foundOne = true;

      if (checkPause(strn, lines))
        return;

      mob = mob->Next;
    }
  }

  if (!foundOne)
    _outtext("There are no mobs loaded in this zone.\n");

  _outtext("\n");
}


//
// displayMobSpeciesList
//

void displayMobSpeciesList(const char *searchStrn)
{
  displayList(1, LAST_RACE, searchStrn, getMobSpeciesNumb, getMobSpeciesCode, NULL, "specie");
}


//
// displayMobInfo
//

void displayMobInfo(const char *args)
{
  char strn[4096], strn2[2048];
  char fstrn[2048], fstrn2[2048], fstrn3[2048], fstrn4[2048], fstrn5[2048], fstrn6[2048], fstrn7[2048], fstrn8[2048];

  const mobType *mob;


 // input-checking galore

  if (!strlen(args))
  {
    _outtext("\nSpecify a mob vnum/keyword to stat.\n\n");

    return;
  }

  mob = getMatchingMob(args);
  if (!mob)
  {
    if (strnumer(args))
      _outtext("\nNo mob type exists with that vnum.\n\n");
    else
      _outtext("\nNo mob type exists with that keyword.\n\n");

    return;
  }

  sprintf(strn,
"\n"
"&+YVnum:&n %u  &+YShort name:&n %s&n\n"
"&+YLong name:&n %s&n\n"
"&+YKeywords:&n %s\n"
"\n"
"&+YMob action flags:&n %u (%s)\n"
"&+YMob aff1 flags  :&n %u (%s)\n"
"&+YMob aff2 flags  :&n %u (%s)\n"
"&+YMob aff3 flags  :&n %u (%s)\n"
"&+YMob aff4 flags  :&n %u (%s)\n"
"&+YMob aggro flags :&n %u (%s)\n"
"&+YMob aggro2 flags:&n %u (%s)\n"
"&+YMob aggro3 flags:&n %u (%s)\n"
"\n"
"&+CPress a key to continue...",

mob->mobNumber, mob->mobShortName,
mob->mobLongName,
getReadableKeywordStrn(mob->keywordListHead, strn2, 2047),

mob->actionBits, getFlagStrn(mob->actionBits, action_bits, fstrn, 1023), 
mob->affect1Bits, getFlagStrn(mob->affect1Bits, affected1_bits, fstrn2, 1023), 
mob->affect2Bits, getFlagStrn(mob->affect2Bits, affected2_bits, fstrn3, 1023), 
mob->affect3Bits, getFlagStrn(mob->affect3Bits, affected3_bits, fstrn4, 1023),
mob->affect4Bits, getFlagStrn(mob->affect4Bits, affected4_bits, fstrn5, 1023),
mob->aggroBits, getFlagStrn(mob->aggroBits, aggro_bits, fstrn6, 1023),
mob->aggro2Bits, getFlagStrn(mob->aggro2Bits, aggro2_bits, fstrn7, 1023),
mob->aggro3Bits, getFlagStrn(mob->aggro3Bits, aggro3_bits, fstrn8, 1023));

  displayColorString(strn);

  getkey();

  sprintf(strn,
"\n\n"
"&+YClass(es):&n %u (%s)\n"
"&+YSpec:    :&n %u (%s)\n"
"&+YSpecies  :&n %s (%s)\n"
"&+YHometown :&n %d (%s)\n"
"&+YAlignment:&n %d\n"
"&+YSize     :&n %d (%s)\n"
"\n"
"&+YLevel/AC/THAC0 :&n %d/%d/%d\n"
"&+YDamage/HP/Exp  :&n %s/%s/%u\n"
"\n"
"&+YCopper/Silver/Gold/Platinum:&n %u/%u/%u/%u\n"
"&+YPos/Default Pos/Sex:&n %d (%s)/%d (%s)/%d (%s)\n"
"\n",

mob->mobClass, getClassString(mob, fstrn, 1023),
mob->mobSpec, getMobSpecStrn(mob->mobSpec),
mob->mobSpecies, getMobSpeciesStrn(mob->mobSpecies),
mob->mobHometown, getMobHometownStrn(mob->mobHometown),
mob->alignment,
mob->size, getMobSizeStrn(mob->size),

mob->level, mob->ac, mob->thac0, 
mob->mobDamage, mob->hitPoints, mob->exp,

mob->copper, mob->silver, mob->gold, mob->platinum,

mob->position, getMobPosStrn(mob->position),
mob->defaultPos, getMobPosStrn(mob->defaultPos),
mob->sex, getMobSexStrn(mob->sex));

  displayColorString(strn);
}
