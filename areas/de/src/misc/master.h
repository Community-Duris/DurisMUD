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


// MASTER.H - header file for MASTER.CPP - stuff related to the master keyword
//            list

#ifndef _MASTER_H_

#include "misc/strnnode.h"

#define NO_MATCH        69   // used when no match is found

#define ENTITY_LOWEST    0
#define ENTITY_O_EDESC   0   // object extra desc
#define ENTITY_OBJECT    1   // object
#define ENTITY_R_EDESC   2   // room extra desc - previously ENTITY_ROOM
#define ENTITY_MOB       3   // mob
#define ENTITY_EXIT      4   // exit - used in inventory (not implemented)
#define ENTITY_ROOM      5   // room
#define ENTITY_ZONE      6   // zone
#define ENTITY_ZONE_LINE 7   // zone line number - for error messages out of readAreaFileLine()
#define ENTITY_QUEST     8   // mob quest
#define ENTITY_SHOP      9   // mob shop
#define ENTITY_INV_LINE 10   // .inv file
#define ENTITY_HIGHEST  10  // NOTE : if you increase this you must add an entry to entityTypes[]

typedef struct _masterKeywordListNode
{
  const stringNode *keywordListHead;  // keywords for this entity

  const void *entityPtr;  // pointer to the entity
  char entityType;  // type of entity

  const stringNode *entityDesc;  // description

  struct _masterKeywordListNode *Next;  // pointer to next node
} masterKeywordListNode;

#define _MASTER_H_
#endif
