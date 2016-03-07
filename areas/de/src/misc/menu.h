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


// MENU.H

#ifndef _MENU_H_

#include <stdlib.h>

#define MENUKEY_OTHER     0     // unrecognized
#define MENUKEY_ABORT     1     // abort
#define MENUKEY_SAVE      2     // save
#define MENUKEY_NEXT      3     // go to next
#define MENUKEY_PREV      4     // go to previous
#define MENUKEY_JUMP      5     // show prompt, jump to vnum

#define MENUKEY_SAVE_CH        K_Enter
#define MENUKEY_SAVE2_CH       'Q'
#define MENUKEY_FLAG_SAVE2_CH  K_Ctrl_Q

#define MENUKEY_ABORT_CH       K_Escape
#define MENUKEY_ABORT2_CH      'X'
#define MENUKEY_FLAG_ABORT2_CH K_Ctrl_X

#define MENUKEY_NEXT_CH        '>'
#define MENUKEY_NEXT2_CH       '.'

#define MENUKEY_PREV_CH        '<'
#define MENUKEY_PREV2_CH       ','

#define MENUKEY_JUMP_CH        '/'

#define MENU_JUMP_ERROR   1
#define MENU_JUMP_VALID   2


#define MENU_COMMON  "  &+YCR&+L. &+wExit, save changes\n" \
                     " &+YEsc&+L. &+wExit, discard changes\n"

typedef enum
{
  mctDirect = 200,
  mctDirectNoKey = 201,

  mctChar = 1,
  mctByte = 2,
  mctUByte = 3,
  mctShort = 4,
  mctUShort = 5,
  mctInt = 6,
  mctIntNoZero = 7,    // when editing, 0 not allowed
  mctUInt = 8,
  mctUIntNoZero = 9,
  mctString = 10,
  mctFloat = 11,
  mctDouble = 12,
  mctBoolYesNo = 13,

  mctPointerYesNo = 30,
  mctPointerYesNoSansExists = 31,

  mctVnum = 40,
  mctKeywords = 41,
  mctPercentage = 42,
  mctDescription = 43,
  mctDescriptionNoStartBlankLines = 44,
  mctQuestDisappearanceDescription = 45,  // adds 'disappears: yes/no'
  mctMoney = 46,
  mctDieString = 47,  // XdY+Z

  mctRoomFlag = 60,
  mctExits = 61,
  mctRoomEdescs = 62,

  mctObjExtra = 80,
  mctObjExtra2 = 81,
  mctObjWear = 82,
  mctObjAnti = 83,
  mctObjAnti2 = 84,
  mctObjAffect1 = 85,
  mctObjAffect2 = 86,
  mctObjAffect3 = 87,
  mctObjAffect4 = 88,
  mctObjTrapFlag = 89,
  mctObjEdescs = 90,
  mctObjectLimit = 91, 
  mctObjectLimitOverride = 92,
  mctObjIDKeywords = 93,
  mctObjIDShort = 94,
  mctObjIDLong = 95,

  mctSpecies = 100,
  mctMobClass = 101,
  mctMobAction = 102,
  mctMobAggro = 103,
  mctMobAggro2 = 104,
  mctMobAggro3 = 105,
  mctMobAffect1 = 106,
  mctMobAffect2 = 107,
  mctMobAffect3 = 108,
  mctMobAffect4 = 109,
  mctMobLimit = 110,
  mctMobLimitOverride = 111,

  mctNumbShopSold = 120,
  mctNumbShopBought = 121, 
  mctShopMessage = 122,  // one %s
  mctShopMessageProduced = 123,  // two %s unless produced list is empty, then one or two %s
  mctShopMessageTraded = 124,  // two %s unless traded list is empty, then one or two %s
  mctBuyMultiplier = 125,
  mctSellMultiplier = 126,
  mctShopFirstOpen = 127,
  mctShopFirstClose = 128,
  mctShopSecondOpen = 129,
  mctShopSecondClose = 130,

  mctZoneFlag = 140,
  mctLowLifespan = 141,
  mctHighLifespan = 142,

  mctVarYesNo = 160,
  mctVarUInt = 161,
  mctVarString = 162,

  mctItemValue = 300,

} menuChoiceDataType;

typedef enum 
{ 
  mclNone = 0, 

  mclRoomSector = 10, 
  mclRoomManaFlag = 11, 
  mclCurrentDirection = 12, 

  mclObjectType = 20,
  mclApplyWhere = 21, 
  mclObjectSize = 22, 
  mclMaterial = 23, 
  mclCraftsmanship = 24, 
  mclTrapFlags = 25,
  mclTrapDamage = 26, 
  mclObjectValue1 = 27, 
  mclObjectValue2 = 28, 
  mclObjectValue3 = 29, 
  mclObjectValue4 = 30, 
  mclObjectValue5 = 31, 
  mclObjectValue6 = 32, 
  mclObjectValue7 = 33, 
  mclObjectValue8 = 34, 

  mclHometown = 40, 
  mclMobSize = 41, 
  mclPosition = 42, 
  mclSex = 43,
  mclMobSpec = 44,
    
  mclResetMode = 50, 

  mclQuestItemGivenToMobType = 60,
  mclQuestItemGivenToPCType = 61,
  mclQuestItemGivenToMobValue = 62,
  mclQuestItemGivenToPCValue = 63,

  mclShopkeeperRace = 70
} menuChoiceListType;

typedef enum
{
  mcdDefault = 0,
  mcdDoNotDisplay = 1,
  mcdNoListInline = 2,    // don't show list value in inline menu view
  mcdPipeEverySecond = 3  // '1/3|4/5'
} menuChoiceDisplayType;

// individual choice

typedef struct _menuChoice
{
  const char *choiceGroupName;  // shown in inline menus
  const char *choiceName;       // shown in right-side menus if non-NULL
  const char *choiceDescName;   // shown in right-side menus if non-NULL - has priority over choiceName

  size_t offset;  // offset of value into entityPtr type - also used to store vnum for obj/mob limit
  menuChoiceDataType dataType;
  menuChoiceListType listType;
  menuChoiceDisplayType displayType;

  uint intMaxLen;  // maximum length of strings when editing, not including NULL - also used to store
                   // original vnum for objects (for checking for loaded containers when changing type)

  const struct _menuChoice* choiceMoreArr;  // for displaying more info on the same line in inline menus

  void *extraPtr;  // used to store mobType ptr when editing shop race and quest items
} menuChoice;

// choice group

typedef struct _menuChoiceGroup
{
  const menuChoice* choiceArr;

  const char chKey;  // chKey = 0 means blank line
  const char chKey2;
} menuChoiceGroup;

// menu type - values inline is that shown in top room, obj, mob, etc., menus, values right is that shown
//             in room misc, obj misc, etc.

typedef enum 
{ 
  mtDisplayValuesInline = 0, 
  mtDisplayValuesRight = 1 
} menuDisplayType;

// menu

typedef struct _menu
{
  const menuChoiceGroup* choiceGroupArr;
  const menuDisplayType displayType;
} menu;

// blank line

#define MENU_BLANK_LINE  { (const menuChoice*)8675309, 0, 0 }  // menuChoice addr must be non-zero

#define _MENU_H_
#endif
