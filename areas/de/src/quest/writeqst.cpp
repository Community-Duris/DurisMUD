//
//  File: writeqst.cpp   originally part of durisEdit
//
//  Usage: functions used for writing quest info to the .qst file
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


#include "types.h"
#include "fh.h"

#include "quest.h"



extern bool g_readFromSubdirs;

void updateQuestItems();
void addQuestItems(quest *questPtr);

//
// writeQuesttoFile : Writes a quest to a file
//

void writeQuesttoFile(FILE *questFile, const quest *qst, const uint mobNumb)
{
  const questMessage *msgNode;
  const questQuest *qstNode;
  const questItem *qstItem;

  char strn[512];


  fprintf(questFile, "#%u\n", mobNumb);

 // next, run through M and Q formats

  msgNode = qst->messageHead;
  qstNode = qst->questHead;

  while (msgNode)
  {
    if (msgNode->messageToAll)
      fputs("MA\n", questFile);
    else
      fputs("M\n", questFile);

    fprintf(questFile, "%s\n", createKeywordString(msgNode->keywordListHead, strn, 511));
    writeStringNodes(questFile, msgNode->questMessageHead);
    fputs("~\n", questFile);

    msgNode = msgNode->Next;
  }

  while (qstNode)
  {
    if (qstNode->messagesToAll)
      fputs("QA\n", questFile);
    else
      fputs("Q\n", questFile);

    writeStringNodes(questFile, qstNode->questReplyHead);
    fputs("~\n", questFile);

    qstItem = qstNode->questPlayRecvHead;
    while (qstItem)
    {
      switch (qstItem->itemType)
      {
        case QUEST_RITEM_OBJ : fprintf(questFile, "R I %u\n", qstItem->itemVal); break;
        case QUEST_RITEM_COINS : fprintf(questFile, "R C %u\n", qstItem->itemVal); break;
        case QUEST_RITEM_SKILL : fprintf(questFile, "R S %u\n", qstItem->itemVal); break;
        case QUEST_RITEM_EXP : fprintf(questFile, "R E %u\n", qstItem->itemVal); break;
      }

      qstItem = qstItem->Next;
    }

    qstItem = qstNode->questPlayGiveHead;
    while (qstItem)
    {
      switch (qstItem->itemType)
      {
        case QUEST_GITEM_OBJ : fprintf(questFile, "G I %u\n", qstItem->itemVal); break;
        case QUEST_GITEM_COINS : fprintf(questFile, "G C %u\n", qstItem->itemVal); break;
        case QUEST_GITEM_OBJTYPE : fprintf(questFile, "G T %u\n", qstItem->itemVal); break;
      }

      qstItem = qstItem->Next;
    }

    if (qstNode->disappearHead)
    {
      fputs("D\n", questFile);
      writeStringNodes(questFile, qstNode->disappearHead);
      fputs("~\n", questFile);
    }

    qstNode = qstNode->Next;
  }

 // terminate the quest info

  fputs("S\n", questFile);
}


//
// writeQuestFile : Write the quest file
//

void writeQuestFile(const char *filename)
{
  FILE *questFile;
  char questFilename[512] = "";
  char strn[512];

  uint numb = getLowestMobNumber(), i;
  const uint highest = getHighestMobNumber();


 // assemble the filename of the quest file

  if (g_readFromSubdirs) 
    strcpy(questFilename, "qst/");

  if (filename) 
    strcat(questFilename, filename);
  else 
    strcat(questFilename, getMainZoneNameStrn());

  strcat(questFilename, ".qst");

 // open the world file for writing

  if ((questFile = fopen(questFilename, "wt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(questFilename);
    _outtext(" for writing - aborting\n");

    return;
  }

  i = getNumbQuestMobs();

  sprintf(strn, "Writing %s - %u quest%s\n", questFilename, i, plural(i));

  _outtext(strn);

  updateQuestItems();

  while (numb <= highest)
  {
    const mobType *mob = findMob(numb);

    if (mob && mob->questPtr)
      writeQuesttoFile(questFile, mob->questPtr, numb);

    numb++;
  }

  fclose(questFile);
}

// Sets the quest-item flag appropriately (ITEM2_QUESTITEM).
//   Note: This is not to be handled by zone writers.
void updateQuestItems()
{
  uint objVnum = getLowestObjNumber();
  const uint objVnumHigh = getHighestObjNumber();
  objectType *qObjType;
  uint mobVnum = getLowestMobNumber();
  const uint mobVnumHigh = getHighestMobNumber();

  // Remove all quest item flags.
  while( objVnum <= objVnumHigh )
  {
    if( (qObjType = findObj(objVnum)) != NULL )
    {
      // extra2Bits and not quest item flag.
      qObjType->extra2Bits &= ~ITEM2_QUESTITEM;
    }
    objVnum++;
  }

  // Add the proper quest item flags.
  while( mobVnum <= mobVnumHigh )
  {
    const mobType *mob = findMob(mobVnum);

    if( mob && mob->questPtr )
    {
      addQuestItems(mob->questPtr);
    }

    mobVnum++;
  }

}

// Assigns the ITEM2_QUESTITEM flag to quest items in the questPtr list.
void addQuestItems(quest *questPtr)
{
  questQuest *qList = (questPtr == NULL) ? NULL : questPtr->questHead;
  questItem  *qItem;
  objectType *qObjType;

  // Walk through the quests.
  while( qList )
  {
    // Walk though each thing given for quest.
    qItem = qList->questPlayGiveHead;
    while( qItem != NULL )
    {
      // If it's an object..
      if( qItem->itemType == QUEST_GITEM_OBJ && (qObjType = findObj(qItem->itemVal)) != NULL )
      {
        qObjType->extra2Bits = qObjType->extra2Bits | ITEM2_QUESTITEM;
      }
      qItem = qItem->Next;
    }
    qList = qList->Next;
  }
}
