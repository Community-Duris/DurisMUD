//
//  File: readqst.cpp    originally part of durisEdit
//
//  Usage: functions for reading mob quest info from the .qst file
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

#include "../fh.h"
#include "../types.h"

#include "../readfile.h"

#include "readqst.h"



extern bool g_readFromSubdirs;


//
// readQuestFileMessage : read a quest message for a mob's quest - returns
//                        pointer to message
//
//   *questFile : file that contains quest info
//    questNumb : mob number quest belongs to - used for error messages
//

questMessage *readQuestFileMessage(FILE *questFile, const uint questNumb)
{
  questMessage *questMsg;
  char strn[512];


  questMsg = new(std::nothrow) questMessage;
  if (!questMsg)
  {
    displayAllocError("questMessage", "readQuestFileMessage");

    exit(1);
  }

  memset(questMsg, 0, sizeof(questMessage));

 // Now, read the mob keywords

  if (!readAreaFileLine(questFile, strn, MAX_QSTMSGKEY_LEN + 2, ENTITY_QUEST, questNumb, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "keyword", 0, NULL, true, true))
    exit(1);

  questMsg->keywordListHead = createKeywordList(strn);
  questMsg->questMessageHead = readStringNodes(questFile, TILDE_LINE, false);

  return questMsg;
}


//
// readQuestFileQuestNumbArgsError
//

void readQuestFileQuestNumbArgsError(const uint questNumb, const char *strn, const char ch)
{
  char outstrn[512];


  sprintf(outstrn,
"Error: %c-line for quest info in mob #%u's quest contains an\n"
"              invalid number of fields (%u).  String read was '",
          ch, questNumb, numbArgs(strn));

  _outtext(outstrn);
  _outtext(strn);
  _outtext("'.  Aborting.\n");

  exit(1);
}


//
// readQuestFileQuestSecondCharLengthError
//

void readQuestFileQuestSecondCharLengthError(const uint questNumb, const char *strn, const char ch)
{
  char outstrn[512];


  sprintf(outstrn,
"Error: Second character after %c-line for quest for mob #%u not valid -\n"
"       (entire string read was '",
          ch, questNumb);

  _outtext(outstrn);
  _outtext(strn);
  _outtext("').  Aborting.\n");

  exit(1);
}



//
// readQuestFileQuestSecondCharLengthError
//

void readQuestFileQuestSecondCharInvalidError(const uint questNumb, const char *strn, const char ch)
{
  char outstrn[512];


  sprintf(outstrn,
"Error: Illegal character specified after %c-line in .qst file (quest #%u).\n"
"       Line read was '",
          ch, questNumb);

  _outtext(outstrn);
  _outtext(strn);
  _outtext("'.  Aborting.\n");

  exit(1);
}


//
// readQuestFileQuest : read quest info for a mob quest - returns pointer to
//                      info
//
//      *questFile : file reading from
//  *endofMobQuest : set to TRUE if end of entire mob quest has been hit
//        *hitWhat : set if another message or quest info block is hit
//       questNumb : mob number - used for error messages
//

questQuest *readQuestFileQuest(FILE *questFile, bool *endofMobQuest, char *hitWhat, const uint questNumb)
{
  questQuest *questQst;
  questItem *lastRecv = NULL, *lastGive = NULL, *qstItem;
  char strn[512], secondChar[512], dummy[512], outstrn[512];
  bool done = false;
  uint numb;


  *endofMobQuest = false;
  *hitWhat = HIT_NOTHING;

  questQst = new(std::nothrow) questQuest;
  if (!questQst)
  {
    displayAllocError("questQuest", "readQuestFileQuest");

    exit(1);
  }

  memset(questQst, 0, sizeof(questQuest));

  questQst->questReplyHead = readStringNodes(questFile, TILDE_LINE, false);

 // now, read R, G, and D lines

  while (!done)
  {
    if (!readAreaFileLine(questFile, strn, 512, ENTITY_QUEST, questNumb, ENTITY_TYPE_UNUSED, 
                          ENTITY_NUMB_UNUSED, "quest info", 0, NULL, false, true))
      exit(1);

    if (strlen(getArg(strn, 1, dummy, 511)) > 2)
    {
      sprintf(outstrn,
"Error: Quest info type for mob #%u not valid (too long) -\n"
"       (entire string read was '",
              questNumb);

      _outtext(outstrn);
      _outtext(strn);
      _outtext("').  Aborting.\n");

      exit(1);
    }

    strn[0] = toupper(strn[0]);

    switch (strn[0])
    {
     // things given to player

      case 'R' : if (numbArgs(strn) != 3)
                 {
                   readQuestFileQuestNumbArgsError(questNumb, strn, 'R');
                 }

                 sscanf(strn, "%s%s%u", dummy, secondChar, &numb);

                 qstItem = new(std::nothrow) questItem;

                 if (!qstItem)
                 {
                   displayAllocError("questItem", "readQuestFileQuest");

                   exit(1);
                 }

                 memset(qstItem, 0, sizeof(questItem));

                 qstItem->itemVal = numb;

                 if (lastRecv) 
                   lastRecv->Next = qstItem;

                 lastRecv = qstItem;

                 if (!questQst->questPlayRecvHead)
                   questQst->questPlayRecvHead = qstItem;

                 if (strlen(secondChar) != 1)
                 {
                   readQuestFileQuestSecondCharLengthError(questNumb, strn, 'R');
                 }

                 switch (toupper(secondChar[0]))
                 {
                   case 'C' : qstItem->itemType = QUEST_RITEM_COINS; break;
                    // coins

                   case 'I' : qstItem->itemType = QUEST_RITEM_OBJ; break;
                    // item

                   case 'S' : qstItem->itemType = QUEST_RITEM_SKILL; break;
                    // skill

                   case 'E' : qstItem->itemType = QUEST_RITEM_EXP; break;
                    // experience

                   default  : readQuestFileQuestSecondCharInvalidError(questNumb, strn, 'R');
                 }

                 break;

     // things player gives to mob
      case 'G' : if (numbArgs(strn) != 3)
                 {
                   readQuestFileQuestNumbArgsError(questNumb, strn, 'G');
                 }

                 sscanf(strn, "%s%s%u", dummy, secondChar, &numb);

                 qstItem = new(std::nothrow) questItem;

                 if (!qstItem)
                 {
                   displayAllocError("questItem", "readQuestFileQuest");

                   exit(1);
                 }

                 memset(qstItem, 0, sizeof(questItem));

                 qstItem->itemVal = numb;
                 if (lastGive) lastGive->Next = qstItem;
                 lastGive = qstItem;

                 if (!questQst->questPlayGiveHead)
                   questQst->questPlayGiveHead = qstItem;

                 if (strlen(secondChar) != 1)
                 {
                   readQuestFileQuestSecondCharLengthError(questNumb, strn, 'G');
                 }

                 switch (toupper(secondChar[0]))
                 {
                   case 'I' : qstItem->itemType = QUEST_GITEM_OBJ; break;
                    // item

                   case 'C' : qstItem->itemType = QUEST_GITEM_COINS; break;
                    // coins

                   case 'T' : qstItem->itemType = QUEST_GITEM_OBJTYPE; break;
                    // certain type of object

                   default  : readQuestFileQuestSecondCharInvalidError(questNumb, strn, 'G');
                 }

                 break;

     // message mob spouts when running off (if a quest has two D entries
     // [for whatever reason], new one replaces old one)
      case 'D' : if (questQst->disappearHead)
                   deleteStringNodes(questQst->disappearHead);

                 questQst->disappearHead =
                   readStringNodes(questFile, TILDE_LINE, false);

                 break;

     // hit an M-line
      case 'M' : done = true;

                 if (strn[1] == 'A')
                   *hitWhat = HIT_MESSAGEECHO;
                 else
                   *hitWhat = HIT_MESSAGE;

                 return questQst;

     // hit another Q-line
      case 'Q' : done = true;

                 if (strn[1] == 'A')
                   *hitWhat = HIT_QUESTECHO;
                 else
                   *hitWhat = HIT_QUEST;

                 return questQst;

     // end of mob quest info
      case 'S' : done = *endofMobQuest = true;
                 return questQst;

     // unrecognized character
      default : sprintf(outstrn,
               "Error: Illegal character specified in quest info line for quest #%u -\n"
               "       entire string read was '",
                        questNumb);

                _outtext(outstrn);
                _outtext(strn);
                _outtext("'.  Aborting.\n");

                exit(1);
    }
  }

  return questQst;
}


//
// readQuestFromFile : returns NULL if hit EOF in the expected spotte, else returns the address of the new
//                     questte
//
//     questFile : file
//     questNumb : if non-NULL, set with # of quest
//

quest *readQuestFromFile(FILE *questFile, uint *questNumb)
{
  char strn[1024];


 // first, read the quest's mob number - may hit EOF here

  bool hitEOF;

  if (!readAreaFileLineAllowEOF(questFile, strn, 512, ENTITY_QUEST, ENTITY_NUMB_UNUSED, 
                                ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, "vnum", 0, NULL, false, true, 
                                &hitEOF))
    exit(1);

  if (hitEOF)
    return NULL;

  if (strn[0] != '#')
  {
    char outstrn[512];

    sprintf(outstrn,
"Error - '#' expected at start of quest's mob vnum ref, instead found '%c'.\n\n"
"Entire string read was '",
            strn[0]);
    
    _outtext(outstrn);
    _outtext(strn);
    _outtext("'.\n");

    exit(1);
  }

 // remove the durn '#' at the start of the string

  deleteChar(strn, 0);

  if (!strnumber(strn))
  {
    _outtext("Error: Non-numerics found in mob number ref in .qst file (string read was\n'#");
    _outtext(strn);
    _outtext("').  Aborting.\n");

    exit(1);
  }

  const uint mobNumb = strtoul(strn, NULL, 10);

  if (questNumb)
    *questNumb = mobNumb;

 // allocate and check that we succeeded..  if you fail on a quest rec,
 // you've got some problems, since they're like 12 bytes

  quest *questPtr = new(std::nothrow) quest;
  if (!questPtr)
  {
    displayAllocError("quest", "readQuest");

    exit(1);
  }

  memset(questPtr, 0, sizeof(quest));

 // this while loop reads command by command, one command per iteration

  bool done = false, endofMobQuest;
  questMessage *questMsg, *questMsgPrev = NULL;
  questQuest *questQst, *questQstPrev = NULL;
  char hitWhat = HIT_NOTHING;

  while (!done)
  {
   // Read a command

    if (hitWhat == HIT_MESSAGE)  // work a little magic
    {
      strn[0] = 'M';
      strn[1] = 0;
    }
    else
    if (hitWhat == HIT_QUEST)
    {
      strn[0] = 'Q';
      strn[1] = 0;
    }
    else
    if (hitWhat == HIT_MESSAGEECHO)
    {
      strn[0] = 'M';
      strn[1] = 'A';
    }
    else
    if (hitWhat == HIT_QUESTECHO)
    {
      strn[0] = 'Q';
      strn[1] = 'A';
    }
    else
    {
      if (!readAreaFileLine(questFile, strn, 512, ENTITY_QUEST, mobNumb, ENTITY_TYPE_UNUSED, 
                            ENTITY_NUMB_UNUSED, "quest info", 0, NULL, false, true))
        exit(1);

      if ((strlen(strn) != 1) &&
          ((strlen(strn) != 2) && (strn[1] != 'A') &&
           ((strn[0] == 'M') || (strn[0] == 'Q'))))
      {
        char outstrn[512];

        sprintf(outstrn, "Error: Line for mob #%u's quest invalid - string read was\n'",
                mobNumb);

        _outtext(outstrn);
        _outtext(strn);
        _outtext("'.  Aborting.\n");

        exit(1);
      }
    }

    switch (strn[0])
    {
      case 'M' : questMsg = readQuestFileMessage(questFile, mobNumb);

                 questMsg->messageToAll = (strn[1] == 'A');

                 if (questMsgPrev) 
                   questMsgPrev->Next = questMsg;

                 questMsgPrev = questMsg;

                 if (!questPtr->messageHead)
                   questPtr->messageHead = questMsg;

                 break;

      case 'Q' : questQst = readQuestFileQuest(questFile, &endofMobQuest, &hitWhat, mobNumb);

                 questQst->messagesToAll = (strn[1] == 'A');

                 if (questQstPrev) 
                   questQstPrev->Next = questQst;

                 questQstPrev = questQst;

                 if (!questPtr->questHead)
                   questPtr->questHead = questQst;

                 if (endofMobQuest) 
                   done = true;  // hit S

                 break;

      case 'S' : done = true;
                 break;  // done reading commands for this mob's quest

      default : _outtext("Unrecognized command in .qst file: string read was '");
                _outtext(strn);
                _outtext("'.\n(quest/mob #");
                
                sprintf(strn, "%u).  Aborting.\n", mobNumb);

                _outtext(strn);

                exit(1);
    }
  }

  return questPtr;
}


//
// readQuestFile : Reads all the quest info, sticking it where it belongs -
//                 returns TRUE if the file was read successfully, FALSE
//                 otherwise
//
//   *filename : if non-NULL, specifies filename, else value in MAINZONENAME
//               is used
//

bool readQuestFile(const char *filename)
{
  FILE *questFile;
  quest *questPtr;

  char questFilename[512] = "";

  uint mobNumb;


 // assemble the filename of the quest file

  if (g_readFromSubdirs) 
    strcpy(questFilename, "qst/");

  if (filename) 
    strcat(questFilename, filename);
  else 
    strcat(questFilename, getMainZoneNameStrn());

  strcat(questFilename, ".qst");

 // open the quest file for reading

  if ((questFile = fopen(questFilename, "rt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(questFilename);
    _outtext(", skipping\n");

    return false;
  }

  _outtext("Reading ");
  _outtext(questFilename);
  _outtext("...\n");

  while (true)
  {
    questPtr = readQuestFromFile(questFile, &mobNumb);

   // NULL quest means EOF

    if (!questPtr)
      break;

    mobType *mob = findMob(mobNumb);
    if (!mob)
    {
      char outstrn[512];

      sprintf(outstrn,
  "Error: .qst file specifies a mob vnum that doesn't exist (#%u).\n"
  "       Aborting.\n\n",
              mobNumb);

      _outtext(outstrn);

      exit(1);
    }

   // check for dupe quest

    if (mob->questPtr)
    {
      char outstrn[512];

      sprintf(outstrn,
  "Error: .qst file specifies a quest for a mob (#%u) that already has\n"
  "       one.  Aborting.\n",
              mobNumb);

      _outtext(outstrn);

      exit(1);
    }

    mob->questPtr = questPtr;
  }

  fclose(questFile);

  return true;
}
