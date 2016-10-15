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


// ZONE.H - useful stuff for zones

#ifndef _ZONE_H_

#include "../types.h"

#define NUMB_ZONEMISC_FLAGS   5

#define ZONE_SILENT       BIT_1  // entire zone is silent (possibly works)
#define ZONE_SAFE         BIT_2  // entire zone is safe (probably works)
#define ZONE_HOMETOWN     BIT_3  // zone is a hometown
#define ZONE_NEWRESET     BIT_4
#define ZONE_MAP          BIT_5

// zone misc flags

typedef struct _zoneMiscBitFlags
{
  unsigned silent      : 1;
  unsigned safe        : 1;
  unsigned hometown    : 1;
  unsigned newReset    : 1;
  unsigned map         : 1;

  unsigned slack       : 27;
} zoneMiscBitFlags;

typedef union _zoneMiscFlagRec
{
  uint longIntFlags;

  zoneMiscBitFlags zoneMiscBits;
} zoneMiscFlagRec;


// zone reset values

#define ZONE_RESET_LOWEST   0
#define ZONE_NO_RESET       0
#define ZONE_RESET_EMPTY    1
#define ZONE_RESET_ALWAYS   2
#define ZONE_RESET_HIGHEST  2

#define NUMB_ZONE_RESET_MODES  (ZONE_RESET_HIGHEST - ZONE_RESET_LOWEST) + 1

// mob equipment areas

//#define WEAR_NOTWORN          -1
#define WEAR_LOW               0
#define WEAR_LIGHT             0  // not imped (obsolete)
#define WEAR_FINGER_R          1
#define WEAR_FINGER_L          2
#define WEAR_NECK_1            3
#define WEAR_NECK_2            4
#define WEAR_BODY              5
#define WEAR_HEAD              6
#define WEAR_LEGS              7
#define WEAR_FEET              8
#define WEAR_HANDS             9
#define WEAR_ARMS              10
#define WEAR_SHIELD            11
#define WEAR_ABOUT             12
#define WEAR_WAIST             13
#define WEAR_WRIST_R           14
#define WEAR_WRIST_L           15
#define WIELD_PRIMARY          16
#define WIELD_SECOND           17
#define HOLD                   18
#define WEAR_EYES              19
#define WEAR_FACE              20
#define WEAR_EARRING_R         21
#define WEAR_EARRING_L         22
#define WEAR_QUIVER            23
#define WEAR_BADGE             24
#define WIELD_THIRD            25
#define WIELD_FOURTH           26
#define WEAR_BACK              27
#define WEAR_ATTACH_BELT_1     28
#define WEAR_ATTACH_BELT_2     29
#define WEAR_ATTACH_BELT_3     30
#define WEAR_ARMS_2            31
#define WEAR_HANDS_2           32
#define WEAR_WRIST_LR          33
#define WEAR_WRIST_LL          34
#define WEAR_HORSE_BODY        35
#define WEAR_LEGS_REAR         36  /* never used */
#define WEAR_TAIL              37
#define WEAR_FEET_REAR         38  /* never actually used in mud */
#define WEAR_NOSE              39
#define WEAR_HORN              40
#define WEAR_IOUN              41
#define WEAR_SIDER_BODY        42
#define WEAR_HIGH              42

#define WEAR_WHOLEBODY  WEAR_HIGH + 1  // returned by checkMobEquipSlot() -
#define WEAR_WHOLEHEAD  WEAR_HIGH + 2  // used as indices in eq array just like
#define WIELD_TWOHANDS  WEAR_HIGH + 3  // the rest
#define WIELD_TWOHANDS2 WEAR_HIGH + 4 /* for thrikreen */
#define HOLD2           WEAR_HIGH + 5
#define WEAR_TRUEHIGH   HOLD2


// record etc for zones themselves

#define MAX_ZONENAME_LEN    (usint)128

typedef struct _zone
{
  uint zoneNumber;  // zone number

  char zoneName[MAX_ZONENAME_LEN + 1];  // name of zone

  uint lifeLow;     // replaces lifeSpan - number is randomly chosen between
  uint lifeHigh;    // these two limits at bootup (per reset?)

  uint resetMode;
  zoneMiscFlagRec miscBits;

  uint mapWidth;
  uint mapHeight;

  uint zoneDiff;  // difficulty
} zone;

#define _ZONE_H_
#endif
