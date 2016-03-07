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


// ROOM.H - various stuff related to rooms


#ifndef _ROOM_H_

#include "exit.h"

#include "obj/objhere.h"
#include "mob/mobhere.h"

#include "misc/master.h"
#include "misc/editable.h"

// mana flag types

#define MANA_LOWEST     0
#define APPLY_MANA_ALL  0
#define MANA_GOOD       1
#define MANA_NEUTRAL    2
#define MANA_EVIL       3
#define MANA_HIGHEST    3
#define NUMB_MANA_TYPES 4

// sector types

#define SECT_LOWEST              0
#define SECT_INSIDE              0 // Uses as if walking indoors
#define SECT_CITY                1 // Uses as if walking in a city
#define SECT_FIELD               2 // Uses as if walking in a field
#define SECT_FOREST              3 // Uses as if walking in a forest
#define SECT_HILLS               4 // Uses as if walking in hills
#define SECT_MOUNTAIN            5 // Uses as if climbing in mountains
#define SECT_WATER_SWIM          6 // Uses as if swimming
#define SECT_WATER_NOSWIM        7 // Impossible to swim water - requires a boat
#define SECT_NO_GROUND           8 // (must be flying/levitating)
#define SECT_UNDERWATER          9 // Need water breathing here
#define SECT_UNDERWATER_GR      10 // Underwater, on the bottom.  Can bash
#define SECT_PLANE_OF_FIRE      11 // Used in the Elemental Plane of Fire
#define SECT_OCEAN              12 // Use if in the ocean or vast seas only!
#define SECT_UNDRWLD_WILD       13 // [underworld flags: intended to be used for
#define SECT_UNDRWLD_CITY       14 //  underground settings; drow, duergars, mind
#define SECT_UNDRWLD_INSIDE     15 //  flayers, etc -- details to be fleshed
#define SECT_UNDRWLD_WATER      16 //  out later]
#define SECT_UNDRWLD_NOSWIM     17
#define SECT_UNDRWLD_NOGROUND   18
#define SECT_PLANE_OF_AIR       19 // Used in the Elemental Plane of Air
#define SECT_PLANE_OF_WATER     20 // Used in the Elemental Plane of Water
#define SECT_PLANE_OF_EARTH     21 // Used in the Elemental Plane of Earth
#define SECT_ETHEREAL           22
#define SECT_ASTRAL             23
#define SECT_DESERT             24
#define SECT_ARCTIC             25
#define SECT_SWAMP              26
#define SECT_UNDRWLD_MOUNTAIN   27
#define SECT_UNDRWLD_SLIME      28
#define SECT_UNDRWLD_LOWCEIL    29
#define SECT_UNDRWLD_LIQMITH    30
#define SECT_UNDRWLD_MUSHROOM   31
#define SECT_CASTLE_WALL        32
#define SECT_CASTLE_GATE        33
#define SECT_CASTLE             34
#define SECT_NEG_PLANE          35
#define SECT_PLANE_OF_AVERNUS   36
#define SECT_ROAD               37
#define SECT_SNOWY_FOREST       38
#define SECT_HIGHEST            38
#define NUMB_SECT_TYPES         (SECT_HIGHEST+1)

// room info

#define MAX_ROOMNAME_LEN  (usint)256

typedef struct _room
{
  uint roomNumber;  // room number - woah

  char roomName[MAX_ROOMNAME_LEN + 1];  // name of room
  stringNode *roomDescHead;

  uint zoneNumber;
  uint roomFlags;
  uint sectorType;
  uint resourceInfo;

  uint fallChance; // fall percentage

  uint current;    // current
  uint currentDir; // current direction

  uint manaFlag;   // mana stuff
  int manaApply;

  roomExit *exits[NUMB_EXITS];

  extraDesc *extraDescHead;

  objectHere *objectHead;
  mobHere *mobHead;

  masterKeywordListNode *masterListHead;
  editableListNode *editableListHead;

  bool defaultRoom;
} room;


#define _ROOM_H_
#endif
