//
//  File: quest.cpp      originally part of durisEdit
//
//  Usage: functions used to manipulate mob quest records
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

#include <string.h>
#include <stdlib.h>

#include "../fh.h"
#include "../types.h"

#include "quest.h"



extern room *g_currentRoom;
extern mobType **g_mobLookup;
extern quest *g_defaultQuest;
extern uint g_numbLookupEntries;
extern bool g_madeChanges;

//
// questExists : returns true if mob exists and has a quest
//

bool questExists(const uint numb)
{
  mobType *mob = findMob(numb);

  if (!mob) 
    return false;

  return (mob->questPtr != NULL);
}


//
// getLowestQuestMobNumber : making sure that a mob does indeed have a
//                           quest somewhere in the zone should be done
//                           before calling this function
//

uint getLowestQuestMobNumber(void)
{
  uint highest = getHighestMobNumber();

  for (uint i = getLowestMobNumber(); i <= highest; i++)
  {
    if (mobExists(i) && findMob(i)->questPtr) 
      return i;
  }

  return g_numbLookupEntries;
}


//
// getHighestQuestMobNumber : making sure that a mob does indeed have a
//                            quest somewhere in the zone should be done
//                            before calling this function
//

uint getHighestQuestMobNumber(void)
{
  uint lowest = getLowestMobNumber();

  for (uint i = getHighestMobNumber(); i >= lowest; i--)
  {
    if (mobExists(i) && findMob(i)->questPtr) 
      return i;
  }

  return 0;
}


//
// getPrevQuestMob : find quest mob right before mobNumb, numerically
//

mobType *getPrevQuestMob(const uint mobNumb)
{
  uint i = mobNumb - 1;

  if (mobNumb <= getLowestQuestMobNumber()) 
    return NULL;

  while (!g_mobLookup[i] || !g_mobLookup[i]->questPtr) 
    i--;

  return g_mobLookup[i];
}


//
// getNextQuestMob : find quest mob right after mobNumb, numerically
//

mobType *getNextQuestMob(const uint mobNumb)
{
  uint i = mobNumb + 1;

  if (mobNumb >= getHighestQuestMobNumber()) 
    return NULL;

  while (!g_mobLookup[i] || !g_mobLookup[i]->questPtr) 
    i++;

  return g_mobLookup[i];
}


//
// compareQuestMessage : returns true if they match, false if not
//

bool compareQuestMessage(const questMessage *msg1, const questMessage *msg2)
{
  if (msg1 == msg2) 
    return true;

  if (!msg1 || !msg2) 
    return false;

  if (!compareStringNodesIgnoreCase(msg1->keywordListHead, msg2->keywordListHead))
    return false;
  if (!compareStringNodes(msg1->questMessageHead, msg2->questMessageHead))
    return false;
  if (msg1->messageToAll != msg2->messageToAll)
    return false;

  return true;
}


//
// copyQuestMessage : copies msg, returning pointer to new
//

questMessage *copyQuestMessage(const questMessage *msg)
{
  questMessage *newMsg;

  if (!msg) 
    return NULL;

  newMsg = new(std::nothrow) questMessage;
  if (!newMsg)
  {
    displayAllocError("questMessage", "copyQuestMessage");

    return NULL;
  }

  memcpy(newMsg, msg, sizeof(questMessage));

  newMsg->keywordListHead = copyStringNodes(msg->keywordListHead);
  newMsg->questMessageHead = copyStringNodes(msg->questMessageHead);

  return newMsg;
}


//
// createQuestMessage : creates new quest message
//

questMessage *createQuestMessage(void)
{
  questMessage *msg = new(std::nothrow) questMessage;

  if (!msg)
  {
    displayAllocError("questMessage", "createQuestMessage");

    return NULL;
  }

  memset(msg, 0, sizeof(questMessage));

  msg->keywordListHead = createKeywordList("default~");

  return msg;
}


//
// deleteQuestMessage : deletes quest message
//

void deleteQuestMessage(questMessage *msg)
{
  if (!msg) 
    return;

  deleteStringNodes(msg->keywordListHead);
  deleteStringNodes(msg->questMessageHead);

  delete msg;
}


//
// deleteQuestMessageList : deletes list of quest messages
//

void deleteQuestMessageList(questMessage *msg)
{
  while (msg)
  {
    questMessage *next = msg->Next;

    deleteQuestMessage(msg);

    msg = next;
  }
}


//
// addQuestMessagetoList : adds existing message to list
//

void addQuestMessagetoList(questMessage **msgListHead, questMessage *msgToAdd)
{
  if (!msgToAdd) 
    return;

  if (!*msgListHead) 
  {
    *msgListHead = msgToAdd;
  }
  else
  {
    questMessage *msg = *msgListHead;

    while (msg->Next)
      msg = msg->Next;

    msg->Next = msgToAdd;
  }
}


//
// deleteQuestMessageinList : deletes quest message in list - assumes msg is in list
//

void deleteQuestMessageinList(questMessage **msgHead, questMessage *msg)
{
  if (!(*msgHead) || !msg) 
    return;

  if (msg == (*msgHead))
  {
    *msgHead = (*msgHead)->Next;

    deleteQuestMessage(msg);
  }
  else
  {
    questMessage *prevMsg = *msgHead;

    while (prevMsg->Next != msg)
      prevMsg = prevMsg->Next;

    prevMsg->Next = msg->Next;

    deleteQuestMessage(msg);
  }
}


//
// createQuestItem : creates quest item
//

questItem *createQuestItem(void)
{
  questItem *item;

  item = new(std::nothrow) questItem;
  if (!item)
  {
    displayAllocError("questItem", "createQuestItem");

    return NULL;
  }

  memset(item, 0, sizeof(questItem));

  return item;
}


//
// addQuestItemtoList : adds existing quest item to list
//

void addQuestItemtoList(questItem **itemListHead, questItem *itemToAdd)
{
  if (!itemToAdd) 
    return;

  if (!*itemListHead) 
  {
    *itemListHead = itemToAdd;
  }
  else
  {
    questItem *item = *itemListHead;

    while (item->Next)
      item = item->Next;

    item->Next = itemToAdd;
  }
}


//
// deleteQuestItem : deletes quest item
//

void deleteQuestItem(questItem *item)
{
  if (!item) 
    return;

  delete item;  // nobody ever said it had to be hard
}


//
// deleteQuestIteminList : delete quest item in list - assumes item is in list
//

void deleteQuestIteminList(questItem **itemHead, questItem *item)
{
  if (!(*itemHead) || !item) 
    return;

  if (item == (*itemHead))
  {
    *itemHead = (*itemHead)->Next;

    deleteQuestItem(item);
  }
  else
  {
    questItem *prevItem = *itemHead;

    while (prevItem->Next != item)
      prevItem = prevItem->Next;

    prevItem->Next = item->Next;

    deleteQuestItem(item);
  }
}


//
// deleteQuestItemList : delete entire quest item list
//

void deleteQuestItemList(questItem *item)
{
  while (item)
  {
    questItem *old = item->Next;

    deleteQuestItem(item);

    item = old;
  }
}


//
// createQuestQuest : create
//

questQuest *createQuestQuest(void)
{
  questQuest *qst = new(std::nothrow) questQuest;

  if (!qst)
  {
    displayAllocError("questQuest", "createQuestQuest");

    return NULL;
  }

  memset(qst, 0, sizeof(questQuest));

  return qst;
}


//
// deleteQuestQuest : delete
//

void deleteQuestQuest(questQuest *qst)
{
  if (!qst) 
    return;

  deleteStringNodes(qst->questReplyHead);
  deleteQuestItemList(qst->questPlayRecvHead);
  deleteQuestItemList(qst->questPlayGiveHead);
  deleteStringNodes(qst->disappearHead);

  delete qst;
}


//
// deleteQuestQuestList : delete list
//

void deleteQuestQuestList(questQuest *qst)
{
  while (qst)
  {
    questQuest *next = qst->Next;

    deleteQuestQuest(qst);

    qst = next;
  }
}


//
// addQuestQuesttoList : add questquest to list
//

void addQuestQuesttoList(questQuest **qstListHead, questQuest *qstToAdd)
{
  if (!qstToAdd) 
    return;

  if (!*qstListHead) 
  {
    *qstListHead = qstToAdd;
  }
  else
  {
    questQuest *qst = *qstListHead;

    while (qst->Next)
      qst = qst->Next;

    qst->Next = qstToAdd;
  }
}


//
// deleteQuestQuestinList : delete questquest from list - assumes it exists
//

void deleteQuestQuestinList(questQuest **qstHead, questQuest *qst)
{
  if (!(*qstHead) || !qst) 
    return;

  if (qst == (*qstHead))
  {
    *qstHead = (*qstHead)->Next;

    deleteQuestQuest(qst);
  }
  else
  {
    questQuest *prevQst = *qstHead;

    while (prevQst->Next != qst)
      prevQst = prevQst->Next;

    prevQst->Next = qst->Next;

    deleteQuestQuest(qst);
  }
}


//
// copyQuestMessageList : copies a list of quest messages, returns head of new list
//

questMessage *copyQuestMessageList(const questMessage *headQstMsg)
{
  questMessage *newHead = NULL, *oldMsg = NULL;


  while (headQstMsg)
  {
    questMessage *msg = new(std::nothrow) questMessage;
    if (!msg)
    {
      displayAllocError("questMessage", "copyQuestMessageList");

      return newHead;
    }

    memset(msg, 0, sizeof(questMessage));

    if (!newHead) 
      newHead = msg;

    if (oldMsg) 
      oldMsg->Next = msg;

    oldMsg = msg;

    msg->keywordListHead = copyStringNodes(headQstMsg->keywordListHead);
    msg->questMessageHead = copyStringNodes(headQstMsg->questMessageHead);
    msg->messageToAll = headQstMsg->messageToAll;


    headQstMsg = headQstMsg->Next;
  }

  return newHead;
}


//
// copyQuestItemList : copies list of questItems, returning head to new list
//

questItem *copyQuestItemList(const questItem *headQstItem)
{
  questItem *newHead = NULL, *oldItem = NULL;


  while (headQstItem)
  {
    questItem *item = new(std::nothrow) questItem;
    if (!item)
    {
      displayAllocError("questItem", "copyQuestItemList");

      return newHead;
    }

   // all too easy

    memcpy(item, headQstItem, sizeof(questItem));

    if (!newHead) 
      newHead = item;

    if (oldItem) 
      oldItem->Next = item;

    oldItem = item;

    headQstItem = headQstItem->Next;
  }

  return newHead;
}


//
// compareQuestItemList : returns true if lists match, false otherwise
//

bool compareQuestItemList(const questItem *item1, const questItem *item2)
{
  if (item1 == item2) 
    return true;

  if (!item1 || !item2) 
    return false;

  while (item1 && item2)
  {
    if (item1->itemType != item2->itemType) 
      return false;

    if (item1->itemVal != item2->itemVal) 
      return false;

    item1 = item1->Next;
    item2 = item2->Next;
  }

  if ((!item1 && item2) || (item1 && !item2)) 
    return false;

  return true;
}


//
// compareQuestQuest : returns true if they match
//

bool compareQuestQuest(const questQuest *qst1, const questQuest *qst2)
{
  if (qst1 == qst2) 
    return true;

  if (!qst1 || !qst2) 
    return false;

  if (!compareStringNodes(qst1->questReplyHead, qst2->questReplyHead))
    return false;

  if (!compareStringNodes(qst1->disappearHead, qst2->disappearHead))
    return false;

  if (!compareQuestItemList(qst1->questPlayRecvHead, qst2->questPlayRecvHead))
    return false;

  if (!compareQuestItemList(qst1->questPlayGiveHead, qst2->questPlayGiveHead))
    return false;

  if (qst1->messagesToAll != qst2->messagesToAll)
    return false;

  return true;
}


//
// copyQuestQuest : make a copy of a quest quest
//

questQuest *copyQuestQuest(const questQuest *qstQst)
{
  questQuest *newQst;


  if (!qstQst) 
    return NULL;

  newQst = new(std::nothrow) questQuest;
  if (!newQst)
  {
    displayAllocError("questQuest", "copyQuestQuest");

    return NULL;
  }

  memcpy(newQst, qstQst, sizeof(questQuest));

  newQst->questReplyHead = copyStringNodes(qstQst->questReplyHead);
  newQst->questPlayRecvHead = copyQuestItemList(qstQst->questPlayRecvHead);
  newQst->questPlayGiveHead = copyQuestItemList(qstQst->questPlayGiveHead);
  newQst->disappearHead = copyStringNodes(qstQst->disappearHead);


  return newQst;
}


//
// copyQuestQuestList : make a copy of a list of quest quests
//

questQuest *copyQuestQuestList(const questQuest *headQstQst)
{
  questQuest *newHead = NULL, *oldQst = NULL;


  while (headQstQst)
  {
    questQuest *qst = new(std::nothrow) questQuest;
    if (!qst)
    {
      displayAllocError("questQuest", "copyQuestQuestList");

      return newHead;
    }

    memset(qst, 0, sizeof(questQuest));

    if (!newHead) 
      newHead = qst;

    if (oldQst) 
      oldQst->Next = qst;

    oldQst = qst;

    qst->questReplyHead = copyStringNodes(headQstQst->questReplyHead);
    qst->questPlayRecvHead = copyQuestItemList(headQstQst->questPlayRecvHead);
    qst->questPlayGiveHead = copyQuestItemList(headQstQst->questPlayGiveHead);
    qst->disappearHead = copyStringNodes(headQstQst->disappearHead);
    qst->messagesToAll = headQstQst->messagesToAll;


    headQstQst = headQstQst->Next;
  }

  return newHead;
}


//
// copyQuest : copies an entire quest
//

quest *copyQuest(const quest *srcQst)
{
  if (!srcQst) 
    return NULL;

  quest *newQst = new(std::nothrow) quest;
  if (!newQst)
  {
    displayAllocError("quest", "copyQuest");

    return NULL;
  }

  memcpy(newQst, srcQst, sizeof(quest));

  newQst->messageHead = copyQuestMessageList(srcQst->messageHead);
  newQst->questHead = copyQuestQuestList(srcQst->questHead);

  return newQst;
}


//
// createQuest : create a quest
//

quest *createQuest(void)
{
  quest *qst;
  

  if (g_defaultQuest)
  {
    qst = copyQuest(g_defaultQuest);
  }
  else
  {
    qst = new(std::nothrow) quest;

    if (qst)
      memset(qst, 0, sizeof(quest));
  }

  if (!qst)
  {
    displayAllocError("quest", "createQuest");

    return NULL;
  }

  return qst;
}


//
// deleteQuest : delete a quest
//

void deleteQuest(quest *qst, const bool madeChanges)
{
  if (!qst) 
    return;

  deleteQuestMessageList(qst->messageHead);
  deleteQuestQuestList(qst->questHead);

  delete qst;

  if (madeChanges)
    g_madeChanges = true;
}


//
// deleteQuestAssocLists : delete quest members but not quest itself
//

void deleteQuestAssocLists(quest *qst)
{
  if (!qst) 
    return;

  deleteQuestMessageList(qst->messageHead);
  deleteQuestQuestList(qst->questHead);
}


//
// compareQuestMessageLists : compare quest messages
//

bool compareQuestMessageLists(const questMessage *msg1, const questMessage *msg2)
{
  if (msg1 == msg2) 
    return true;

  if (!msg1 || !msg2) 
    return false;

  while (msg1 && msg2)
  {
    if (!compareQuestMessage(msg1, msg2))
      return false;

    msg1 = msg1->Next;
    msg2 = msg2->Next;
  }

  if ((!msg1 && msg2) || (msg1 && !msg2)) 
    return false;

  return true;
}


//
// compareQuestQuestLists : compares list of quest quests
//

bool compareQuestQuestLists(const questQuest *qst1, const questQuest *qst2)
{
  if (qst1 == qst2) 
    return true;

  if (!qst1 || !qst2) 
    return false;

  while (qst1 && qst2)
  {
    if (!compareQuestQuest(qst1, qst2))
      return false;

    qst1 = qst1->Next;
    qst2 = qst2->Next;
  }

  if ((!qst1 && qst2) || (qst1 && !qst2)) 
    return false;

  return true;
}


//
// compareQuestInfo : compare entire quest
//

bool compareQuestInfo(const quest *qst1, const quest *qst2)
{
  if (qst1 == qst2) 
    return true;

  if (!qst1 || !qst2) 
    return false;

  if (!compareQuestMessageLists(qst1->messageHead, qst2->messageHead))
    return false;

  if (!compareQuestQuestLists(qst1->questHead, qst2->questHead))
    return false;


  return true;
}


//
// getQuestRecvString : returns a readable string of what all player gets
//                      from mob's quest - used in menu
//

char *getQuestRecvString(const questItem *item, char *strn, const size_t intMaxLen)
{
  char strn2[8192];


  if (!item)
  {
    strcpy(strn, "(nothing)");

    return strn;
  }

  strn[0] = '\0';

  while (item)
  {
    getQuestItemStrn(item, QUEST_RECVITEM, strn2);

    item = item->Next;

   // comma + space + possible two periods, terminating null

    if ((strlen(strn) + strlen(strn2) + 5) < intMaxLen)
    {
      strcat(strn, strn2);

      if (item)
        strcat(strn, ", ");
    }
    else
    {
      strcat(strn, "..");
      break;
    }
  }

  return strn;
}


//
// getNumbMessageNodes : returns number of nodes in list
//

uint getNumbMessageNodes(const questMessage *messageHead)
{
  uint numb = 0;

  while (messageHead)
  {
    numb++;
    messageHead = messageHead->Next;
  }

  return numb;
}


//
// getNumbQuestNodes : returns number of nodes in list
//

uint getNumbQuestNodes(const questQuest *questHead)
{
  uint numb = 0;

  while (questHead)
  {
    numb++;
    questHead = questHead->Next;
  }

  return numb;
}


//
// getNumbItemNodes : returns number of nodes in list
//

uint getNumbItemNodes(const questItem *itemHead)
{
  uint numb = 0;

  while (itemHead)
  {
    numb++;
    itemHead = itemHead->Next;
  }

  return numb;
}


//
// getMessageNodeNumb : number 0 is first node
//

questMessage *getMessageNodeNumb(const uint numb, questMessage *msgHead)
{
  uint i = 0;

  while (msgHead)
  {
    if (i == numb) 
      return msgHead;

    msgHead = msgHead->Next;
    i++;
  }

  return NULL;
}


//
// getQuestNodeNumb : number 0 is first node
//

questQuest *getQuestNodeNumb(const uint numb, questQuest *qstHead)
{
  uint i = 0;

  while (qstHead)
  {
    if (i == numb) 
      return qstHead;

    qstHead = qstHead->Next;
    i++;
  }

  return NULL;
}


//
// getItemNodeNumb : number 0 is first node
//

questItem *getItemNodeNumb(const uint numb, questItem *itemHead)
{
  uint i = 0;

  while (itemHead)
  {
    if (i == numb) 
      return itemHead;

    itemHead = itemHead->Next;
    i++;
  }

  return NULL;
}


//
// getQuestItemStrn : called from edit menu - strn should be at least MAX_OBJSNAME_LEN + 64 chars
//

char *getQuestItemStrn(const questItem *item, const char itemList, char *strn)
{
  strn[0] = '\0';

  if (!item) 
    return strn;

  if (itemList == QUEST_GIVEITEM)  // player gives mob
  {
    switch (item->itemType)
    {
      case QUEST_GITEM_OBJ     :
        sprintf(strn, "%s&n (#%u)", getObjShortName(findObj(item->itemVal)), item->itemVal);

        break;

      case QUEST_GITEM_COINS   :
        getMoneyStrn(item->itemVal, strn);
        break;

      case QUEST_GITEM_OBJTYPE :
        sprintf(strn, "obj type #%u (%s)", 
                item->itemVal, getObjTypeStrn(item->itemVal));
        break;

      default                  :
        sprintf(strn, "err #%u", item->itemType);
        break;
    }
  }
  else
  if (itemList == QUEST_RECVITEM)  // player receives from mob
  {
    switch (item->itemType)
    {
      case QUEST_RITEM_OBJ     :
        sprintf(strn, "%s&n (#%u)", getObjShortName(findObj(item->itemVal)), item->itemVal);

        break;

      case QUEST_RITEM_COINS   :
        getMoneyStrn(item->itemVal, strn);
        break;

      case QUEST_RITEM_SKILL   :
        sprintf(strn, "skill '%s' (#%u)", getSkillTypeStrn(item->itemVal), item->itemVal);
        break;

      case QUEST_RITEM_EXP     :
        sprintf(strn, "%u exp", item->itemVal);
        break;

      default                  :
        sprintf(strn, "err #%u", item->itemType);
        break;
    }
  }

  return strn;
}


//
// getNumbQuestMobs : returns number of quest mobs in zone
//

uint getNumbQuestMobs(void)
{
  uint numbQuests = 0, numb = getLowestMobNumber(), high = getHighestMobNumber();

  while (numb <= high)
  {
    const mobType *mob = findMob(numb);

    if (mob && mob->questPtr) 
      numbQuests++;

    numb++;
  }

  return numbQuests;
}


//
// checkForMobWithQuest : returns true if some mob type in the zone has a quest
//

bool checkForMobWithQuest(void)
{
  const uint highMobNumb = getHighestMobNumber();


  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      const mobType *mob = findMob(mobNumb);

      if (mob->questPtr) 
        return true;
    }
  }

  return false;
}


//
// displayQuestList : Displays the list of mob types with quests loaded into DE
//

void displayQuestList(const char *args)
{
  const uint high = getHighestQuestMobNumber();
  size_t lines = 1;
  bool foundMob = false, listAll = false;


  if (noMobTypesExist())
  {
    _outtext("\nThere are currently no mob types.\n\n");
    return;
  }

  if (!checkForMobWithQuest())
  {
    _outtext("\nNone of the current mob types have a quest.\n\n");
    return;
  }

  if (!args || (args[0] == '\0'))
  {
    listAll = true;
  }

  _outtext("\n\n");

  for (uint mobNumb = getLowestQuestMobNumber(); mobNumb <= high; mobNumb++)
  {
    if (questExists(mobNumb))
    {
      const mobType *mob = findMob(mobNumb);

      if (listAll || scanKeyword(args, mob->keywordListHead))
      {
        char outstrn[MAX_MOBSNAME_LEN + 64];

        sprintf(outstrn, "%s&n (#%u)\n", mob->mobShortName, mobNumb);

        foundMob = true;

        if (checkPause(outstrn, lines))
          return;
      }
    }
  }

  if (!foundMob) 
    _outtext("No matching mob types found.\n");

  _outtext("\n");
}


//
// tellRedund : redundant code in tell command
//

void tellRedund(const bool toAll, const char *whatMessage, const char *whatContain, const stringNode *desc)
{
  char outstrn[512];

  if (desc)
  {
    _outtext("\n");

    if (toAll)
    {
      sprintf(outstrn, "&+W(%smessage is displayed to all in room.)&n\n", whatMessage);
      outstrn[4] = toupper(outstrn[4]);

      displayColorString(outstrn);
    }

    displayStringNodes(desc);
    _outtext("\n");

    return;
  }
  else
  {
    sprintf(outstrn, "\nThat %s has no %smessage.\n\n", whatContain, whatMessage);
    _outtext(outstrn);

    return;
  }
}


//
// tell : display quest info in a duris-like fashion
//

void tell(const char *args)
{
  char arg[512];

 // first, find mob, check for quest, and check for proper matchup - also
 // allow q1, q2 etc to display responses to finishing 'first' quest, etc

  if (!strlen(args))
  {
    _outtext("\n"
"Specify a mob vnum/keyword to tell as the first arg and stuff to tell as the\n"
"second arg.\n\n");

    return;
  }

  getArg(args, 1, arg, 511);

  const mobType *mob = getMatchingMob(arg);
  if (!mob)
  {
    if (strnumber(arg))
    {
      _outtext("\nNo mob type exists with that vnum.\n\n");
      return;
    }
    else
    {
      _outtext("\nNo mob type exists with that keyword.\n\n");
      return;
    }
  }

  if (!mob->questPtr)
  {
    _outtext("\nThat mob has no quest information.\n\n");
    return;
  }

  const uint numbq = getNumbQuestNodes(mob->questPtr->questHead);
  const uint numbm = getNumbMessageNodes(mob->questPtr->messageHead);

  getArg(args, 2, arg, 511);

  if (!arg[0])
  {
    _outtext("\nTell the mob what?\n\n");
    return;
  }


 // check quest messages first, so as to avoid endless checking with
 // D and Q records

  const questMessage *qstm = mob->questPtr->messageHead;

  while (qstm)
  {
    if (scanKeyword(arg, qstm->keywordListHead))
    {
      tellRedund(qstm->messageToAll, "", "reply", qstm->questMessageHead);
      return;
    }

    qstm = qstm->Next;
  }

  arg[0] = toupper(arg[0]);

  if ((arg[0] == 'D') || (arg[0] == 'Q'))
  {
    const questQuest *qstq = getQuestTellCmd(mob, numbq, arg);

    if (!qstq)
      return;

    if (arg[0] == 'D')
      tellRedund(qstq->messagesToAll, "disappearance ", "quest", qstq->disappearHead);
    else
      tellRedund(qstq->messagesToAll, "reply ", "quest", qstq->questReplyHead);

    return;
  }

  _outtext("\n"
"No match found - use Q# and D# to show quest # finish and disappearance\n"
"messages, and message keywords to show message replies.\n\n");
}


//
// getQuestTellCmd : redundant code in tell()
//

questQuest *getQuestTellCmd(const mobType *mob, const uint numbq, const char *strn)
{
  uint val;


  if (!strn[1])
  {
    char outstrn[256];
    const char ch = toupper(strn[0]);

    sprintf(outstrn, "\nSpecify a number after the %c, i.e. %c1.\n\n",
            ch, ch);

    _outtext(outstrn);
    return NULL;
  }
  else
  {
   // skip initial letter

    strn++;

    if (strnumber(strn) && atoi(strn))
    {
      val = strtoul(strn, NULL, 10);
      val--;

      if (val >= numbq)
      {
        _outtext("\nThe mob doesn't have that many quests.\n\n");
        return NULL;
      }
    }
    else
    {
      _outtext("\nSpecify a valid integer greater than zero.\n\n");
      return NULL;
    }
  }

  return getQuestNodeNumb(val, mob->questPtr->questHead);
}
