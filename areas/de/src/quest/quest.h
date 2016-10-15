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


#ifndef _QUEST_H_

#include "../misc/strnnode.h"

#define MAX_QSTMSGKEY_LEN   (usint)256

typedef struct _questMessage   // user types keyword, mob spits out stuff
{                              // (M-format)
  stringNode *keywordListHead;
  stringNode *questMessageHead;

  bool messageToAll;

  struct _questMessage *Next;
} questMessage;

// constants for item types players give to mobs (G-lines)

#define QUEST_GITEM_LOWEST   0
#define QUEST_GITEM_OBJ      0  // specific obj vnum
#define QUEST_GITEM_COINS    1  // coins
#define QUEST_GITEM_OBJTYPE  2  // certain object type
#define QUEST_GITEM_HIGHEST  2

#define NUMB_QUEST_GITEMS    3

// constants for items/stuff player gets from quest (R-lines)

#define QUEST_RITEM_LOWEST   0
#define QUEST_RITEM_OBJ      0  // specific obj vnum
#define QUEST_RITEM_COINS    1  // coins
#define QUEST_RITEM_SKILL    2  // gives skill/spell
#define QUEST_RITEM_EXP      3  // gives experience
#define QUEST_RITEM_HIGHEST  3

#define NUMB_QUEST_RITEMS    4

#define QUEST_GIVEITEM       0  // used to reduce code redundancy
#define QUEST_RECVITEM       1

typedef struct _questItem
{
  uint itemType;  // item type
  uint itemVal;   // item vnum/value

  struct _questItem *Next;
} questItem;

typedef struct _questQuest   // user gives mob obj, mob does something -
{                            // a "true" quest (Q-format)
  stringNode *questReplyHead;   // reply to user

  questItem *questPlayRecvHead; // items player gets for quest (R-format)
  questItem *questPlayGiveHead; // items player gives for quest (G-format)

  stringNode *disappearHead;    // text when disappearing, if any (D-format)

  bool messagesToAll;     // if true, everyone in room sees messages

  struct _questQuest *Next;
} questQuest;


#define MAX_QUESTRECVSTRING_LEN  51  // quest receive strings are shortened to
                                     // this

typedef struct _quest
{
  questMessage *messageHead;  // (M-format)
  questQuest *questHead;      // (Q-format)
} quest;

#define _QUEST_H_
#endif
