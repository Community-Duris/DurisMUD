//
//  File: menudef.cpp   originally part of durisEdit
//
//  Usage: defines menus used in DE
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

#include <stddef.h>

#include "fh.h"

#include "vardef.h"


//
// room menu
//

const menuChoice roomNameChoiceArr[] =
{ { "name", 0, 0, offsetof(room, roomName), mctString, mclNone, mcdDoNotDisplay, MAX_ROOMNAME_LEN, 0 }, 
  { 0 } };

const menuChoice roomDescChoiceArr[] =
{ { "description", 0, 0, offsetof(room, roomDescHead), mctDescriptionNoStartBlankLines, mclNone, mcdDefault, 
    0, 0 }, { 0 } };

const menuChoice roomFlagsChoiceArr[] =
{ { "flags", 0, 0, offsetof(room, roomFlags), mctRoomFlag, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice roomSectorChoiceArr[] =
{ { "sector", 0, 0, offsetof(room, sectorType), mctUInt, mclRoomSector, mcdDefault, 0, 0 }, { 0 } };

const menuChoice roomExtraDescChoiceArr[] =
{ { "extra descriptions", 0, 0, offsetof(room, extraDescHead), mctRoomEdescs, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice roomExitsChoiceArr[] =
{ { "exits", 0, 0, 0, mctExits, mclNone, mcdDefault, 0, 0 }, { 0 } };

// extra data for mana (apply amount)

const menuChoice roomMiscChoiceManaExtraArr[] = 
{ { "mana value", 0, 0, offsetof(room, manaApply), mctInt, mclNone, mcdDefault, 0, 0 }, { 0 } };

// extra data for current (strength)

const menuChoice roomMiscChoiceCurrentExtraArr[] =
{ { "current strength", 0, 0, offsetof(room, current), mctUInt, mclNone, mcdDefault, 0, 0 }, { 0 } };

menuChoice roomMiscChoiceArr[] =
{ { "fall chance", 0, 0, offsetof(room, fallChance), mctPercentage, mclNone, mcdDefault, 0, 0 }, 
  { "mana", "mana type", 0, offsetof(room, manaFlag), mctUInt, mclRoomManaFlag, mcdDefault, 0, 
    roomMiscChoiceManaExtraArr }, 
  { "current", "current direction", 0, offsetof(room, currentDir), mctUInt, mclCurrentDirection, mcdDefault, 
    0, roomMiscChoiceCurrentExtraArr }, { 0 } };

const menuChoice roomVnumArr[] =
{ { "vnum", 0, 0, 0, mctVnum, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup roomGroupArr[] =
{ { roomNameChoiceArr, 'A', 0 },
  { roomDescChoiceArr, 'B', 0 },
  { roomFlagsChoiceArr, 'C', 0 },
  { roomSectorChoiceArr, 'D', 0 },
  { roomExtraDescChoiceArr, 'E', 0 },
  { roomExitsChoiceArr, 'F', 0 },
  { roomMiscChoiceArr, 'G', 0 },
  MENU_BLANK_LINE,
  { roomVnumArr, 'V', 0 },
  { 0, 0, 0 } };

menu g_roomMenu =
{ roomGroupArr, mtDisplayValuesInline };


//
// room misc menu
//

const menuChoiceGroup roomMiscGroupArr[] =
{ { &roomMiscChoiceArr[0], 'A', 0 },
  MENU_BLANK_LINE,
  { &roomMiscChoiceArr[1], 'B', 0 },
  MENU_BLANK_LINE,
  { &roomMiscChoiceArr[2], 'D', 0 },
  { 0, 0, 0 } };

menu g_roomMiscMenu =
{ roomMiscGroupArr, mtDisplayValuesRight };


//
// object menu
//

// extra data for short/long name (long name)

const menuChoice objNameChoiceLongNameExtraArr[] =
{ { "long name", 0, 0, offsetof(objectType, objLongName), mctString, mclNone, mcdDoNotDisplay, 
    MAX_OBJLNAME_LEN, 0 }, { 0 } };

const menuChoice objNameChoiceArr[] =
{ { "short/long name", "short name", 0, offsetof(objectType, objShortName), mctString, mclNone, 
    mcdDoNotDisplay, MAX_OBJSNAME_LEN, objNameChoiceLongNameExtraArr }, { 0 } };

const menuChoice objKeywordsChoiceArr[] =
{ { "keywords", 0, 0, offsetof(objectType, keywordListHead), mctKeywords, mclNone, mcdDefault, MAX_OBJKEY_LEN, 
    0 }, { 0 } };

// extra data for extra/extra2 flags (extra2 flags)

const menuChoice objExtra12FlagsChoiceExtra2ExtraArr[] =
{ { "extra2 flags", 0, 0, offsetof(objectType, extra2Bits), mctObjExtra2, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice objExtra12FlagsChoiceArr[] =
{ { "extra/extra2 flags", "extra flags", 0, offsetof(objectType, extraBits), mctObjExtra, mclNone, mcdDefault, 
    0, objExtra12FlagsChoiceExtra2ExtraArr }, { 0 } };

const menuChoice objWearFlagsChoiceArr[] =
{ { "wear flags", 0, 0, offsetof(objectType, wearBits), mctObjWear, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice objAntiFlagsChoiceArr[] =
{ { "anti flags", 0, 0, offsetof(objectType, antiBits), mctObjAnti, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice objAnti2FlagsChoiceArr[] =
{ { "anti2 flags", 0, 0, offsetof(objectType, anti2Bits), mctObjAnti2, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

// extra data for obj misc (objValues 1-7)

const menuChoice objMiscChoiceValuesExtraArr[] =
{ { "value #2", 0, 0, offsetof(objectType, objValues[1]), mctInt, mclObjectValue2, mcdNoListInline, 0, 0 },
  { "value #3", 0, 0, offsetof(objectType, objValues[2]), mctInt, mclObjectValue3, mcdNoListInline, 0, 0 },
  { "value #4", 0, 0, offsetof(objectType, objValues[3]), mctInt, mclObjectValue4, mcdNoListInline, 0, 0 },
  { "value #5", 0, 0, offsetof(objectType, objValues[4]), mctInt, mclObjectValue5, mcdNoListInline, 0, 0 },
  { "value #6", 0, 0, offsetof(objectType, objValues[5]), mctInt, mclObjectValue6, mcdNoListInline, 0, 0 },
  { "value #7", 0, 0, offsetof(objectType, objValues[6]), mctInt, mclObjectValue7, mcdNoListInline, 0, 0 },
  { "value #8", 0, 0, offsetof(objectType, objValues[7]), mctInt, mclObjectValue8, mcdNoListInline, 0, 0 }, 
  { 0 } };

// extra data for obj misc (applies)

const menuChoice objMiscChoiceAppliesExtraArr[] =
{ { "apply #1 - value", 0, 0, offsetof(objectType, objApply[0].applyModifier), mctInt, mclNone, 
    mcdPipeEverySecond, 0, 0 },
  { "apply #2 - what", 0, 0, offsetof(objectType, objApply[1].applyWhere), mctUInt, mclApplyWhere, 
    mcdPipeEverySecond, 0, 0 },
  { "apply #2 - value", 0, 0, offsetof(objectType, objApply[1].applyModifier), mctInt, mclNone, 
    mcdPipeEverySecond, 0, 0 }, { 0 } };

menuChoice objMiscChoiceArr[] =
{ { "type", 0, 0, offsetof(objectType, objType), mctUInt, mclObjectType, mcdDefault, 0, 0 },
  { "values", "value #1", 0, offsetof(objectType, objValues[0]), mctInt, mclObjectValue1, mcdNoListInline, 0, 
    objMiscChoiceValuesExtraArr },
  { "applies", "apply #1 - what", 0, offsetof(objectType, objApply[0].applyWhere), mctUInt, mclApplyWhere, 
    mcdPipeEverySecond, 0, objMiscChoiceAppliesExtraArr }, { 0 } };

const menuChoice objMisc2ChoiceArr[] =
{ { "size", 0, 0, offsetof(objectType, size), mctUInt, mclObjectSize, mcdDefault, 0, 0 },
  { "material", 0, 0, offsetof(objectType, material), mctUInt, mclMaterial, mcdDefault, 0, 0 },
  { "craftsmanship", 0, 0, offsetof(objectType, craftsmanship), mctUInt, mclCraftsmanship, mcdDefault, 0, 0 },
  { "dmg resist", "dmg resist (obsolete)", 0, offsetof(objectType, damResistBonus), mctUInt, mclNone, 
    mcdDefault, 0, 0 },
  { "weight", 0, 0, offsetof(objectType, weight), mctInt, mclNone, mcdDefault, 0, 0 },
  { "volume", "volume (obsolete)", 0, offsetof(objectType, space), mctUInt, mclNone, mcdDefault, 0, 0 },
  { "worth", 0, 0, offsetof(objectType, worth), mctMoney, mclNone, mcdDefault, 0, 0 },
  { "condition", 0, 0, offsetof(objectType, condition), mctPercentage, mclNone, mcdDefault, 0, 0 },
  { 0 } };

const menuChoice objExtraDescChoiceArr[] =
{ { "extra descriptions", 0, 0, offsetof(objectType, extraDescHead), mctObjEdescs, mclNone, mcdDefault, 0, 
    0 }, { 0 } };

// extra data for obj trap info

const menuChoice objTrapChoiceExtraArr[] =
{ { "damage type/effect", 0, 0, offsetof(objectType, trapDam), mctInt, mclTrapDamage, mcdDefault, 0, 0 },
  { "number of charges", 0, 0, offsetof(objectType, trapCharge), mctInt, mclNone, mcdDefault, 0, 0 },
  { "level", 0, 0, offsetof(objectType, trapLevel), mctUInt, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objTrapChoiceArr[] =
{ { "trap info", "flags (0 = no trap)", 0, offsetof(objectType, trapBits), mctObjTrapFlag, mclTrapFlags, 
    mcdDefault, 0, objTrapChoiceExtraArr }, { 0 } };

// extra data for aff1/aff2 flags (aff2 flags)

const menuChoice objAff12FlagsChoiceAff2ExtraArr[] =
{ { "affect2 flags", 0, 0, offsetof(objectType, affect2Bits), mctObjAffect2, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice objAff12FlagsChoiceArr[] =
{ { "affect1/affect2 flags", "affect1 flags", 0, offsetof(objectType, affect1Bits), mctObjAffect1, mclNone, 
    mcdDefault, 0, objAff12FlagsChoiceAff2ExtraArr }, { 0 } };

// extra data for aff3/aff4 flags (aff4 flags)

const menuChoice objAff34FlagsChoiceAff4ExtraArr[] =
{ { "affect4 flags", 0, 0, offsetof(objectType, affect4Bits), mctObjAffect4, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice objAff34FlagsChoiceArr[] =
{ { "affect3/affect4 flags", "affect3 flags", 0, offsetof(objectType, affect3Bits), mctObjAffect3, mclNone, 
    mcdDefault, 0, objAff34FlagsChoiceAff4ExtraArr }, { 0 } };

// obj limit - offset used to store vnum for limit

menuChoice objLimitChoiceOverrideExtraArr[] =
{ { (char *)8675309, 0, 0, 0, mctObjectLimitOverride, mclNone, mcdDefault, 0, 0 },  // group name must be non-NULL
  { 0 } };

menuChoice objLimitChoiceArr[] =
{ { "limit override for all objs of type", 0, 0, 0, mctObjectLimit, mclNone, mcdDefault, 0, 
    objLimitChoiceOverrideExtraArr }, { 0 } };

const menuChoice objVnumArr[] =
{ { "vnum", 0, 0, 0, mctVnum, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup objNoAffGroupArr[] =
{ { objNameChoiceArr, 'A', 'B' },
  { objKeywordsChoiceArr, 'C', 0 },
  { objExtra12FlagsChoiceArr, 'D', 'E' },
  { objWearFlagsChoiceArr, 'F', 0 },
  { objAntiFlagsChoiceArr, 'G', 0 },
  { objAnti2FlagsChoiceArr, 'H', 0 },
  { objMiscChoiceArr, 'I', 0 },
  { objMisc2ChoiceArr, 'J', 0 },
  { objExtraDescChoiceArr, 'K', 0 },
  { objTrapChoiceArr, 'M', 0 },
  MENU_BLANK_LINE,
  { objLimitChoiceArr, 'L', 0 },
  { objVnumArr, 'V', 0 },
  { 0, 0, 0 } };

const menuChoiceGroup objGroupArr[] =
{ { objNameChoiceArr, 'A', 'B' },
  { objKeywordsChoiceArr, 'C', 0 },
  { objExtra12FlagsChoiceArr, 'D', 'E' },
  { objWearFlagsChoiceArr, 'F', 0 },
  { objAntiFlagsChoiceArr, 'G', 0 },
  { objAnti2FlagsChoiceArr, 'H', 0 },
  { objMiscChoiceArr, 'I', 0 },
  { objMisc2ChoiceArr, 'J', 0 },
  { objExtraDescChoiceArr, 'K', 0 },
  { objTrapChoiceArr, 'M', 0 },
  { objAff12FlagsChoiceArr, 'N', 'O' },
  { objAff34FlagsChoiceArr, 'P', 'R' },
  MENU_BLANK_LINE,
  { objLimitChoiceArr, 'L', 0 },
  { objVnumArr, 'V', 0 },
  { 0, 0, 0 } };

menu g_objNoAffMenu =
{ objNoAffGroupArr, mtDisplayValuesInline };

menu g_objMenu =
{ objGroupArr, mtDisplayValuesInline };


//
// obj misc menu
//

const menuChoiceGroup objMiscGroupArr[] =
{ { &objMiscChoiceArr[0], 'A', 0 },
  MENU_BLANK_LINE,
  { &objMiscChoiceArr[1], 'B', 0 },
  MENU_BLANK_LINE,
  { &objMiscChoiceArr[2], 'J', 0 },
  { 0, 0, 0 } };

menu g_objMiscMenu =
{ objMiscGroupArr, mtDisplayValuesRight };


//
// obj misc 2 menu
//

const menuChoiceGroup objMisc2GroupArr[] =
{ { &objMisc2ChoiceArr[0], 'A', 0 },
  { &objMisc2ChoiceArr[1], 'B', 0 },
  { &objMisc2ChoiceArr[2], 'C', 0 },
  { &objMisc2ChoiceArr[3], 'D', 0 },
  MENU_BLANK_LINE,
  { &objMisc2ChoiceArr[4], 'E', 0 },
  { &objMisc2ChoiceArr[5], 'F', 0 },
  MENU_BLANK_LINE,
  { &objMisc2ChoiceArr[6], 'G', 0 },
  { &objMisc2ChoiceArr[7], 'H', 0 },
  { 0, 0, 0 } };

menu g_objMisc2Menu =
{ objMisc2GroupArr, mtDisplayValuesRight };


//
// obj trap menu
//

const menuChoiceGroup objTrapGroupArr[] =
{ { &objTrapChoiceArr[0], 'A', 0 },
  { 0, 0, 0 } };

menu g_objTrapMenu =
{ objTrapGroupArr, mtDisplayValuesRight };


//
// mob menu
//

const menuChoice mobShortNameChoiceArr[] =
{ { "short name", 0, 0, offsetof(mobType, mobShortName), mctString, mclNone, mcdDoNotDisplay, 
    MAX_MOBSNAME_LEN, 0 }, { 0 } };

const menuChoice mobLongNameChoiceArr[] =
{ { "long name", 0, 0, offsetof(mobType, mobLongName), mctString, mclNone, mcdDoNotDisplay, MAX_MOBLNAME_LEN, 
    0 }, { 0 } };

const menuChoice mobKeywordsChoiceArr[] =
{ { "keywords", 0, 0, offsetof(mobType, keywordListHead), mctKeywords, mclNone, mcdDefault, MAX_MOBKEY_LEN, 
    0 }, { 0 } };

const menuChoice mobDescChoiceArr[] =
{ { "description", 0, 0, offsetof(mobType, mobDescHead), mctDescriptionNoStartBlankLines, mclNone, mcdDefault, 
    0, 0 }, { 0 } };

const menuChoice mobActFlagsChoiceArr[] =
{ { "action flags", 0, 0, offsetof(mobType, actionBits), mctMobAction, mclNone, mcdDefault, 0, 0 }, { 0 } };

// extra data for agg/agg2 flags (agg2 flags)

const menuChoice mobAggro12FlagsChoiceAggro2ExtraArr[] =
{ { "aggro2 flags", 0, 0, offsetof(mobType, aggro2Bits), mctMobAggro2, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice mobAggro12FlagsChoiceArr[] =
{ { "aggro1/2 flags", "aggro1 flags", 0, offsetof(mobType, aggroBits), mctMobAggro, mclNone, mcdDefault, 0, 
    mobAggro12FlagsChoiceAggro2ExtraArr }, { 0 } };

// extra data for agg3 flags

const menuChoice mobAggro3FlagsChoiceArr[] =
{ { "aggro3 flags", 0, 0, offsetof(mobType, aggro3Bits), mctMobAggro3, mclNone, mcdDefault, 0, 
    0 }, { 0 } };

// extra data for aff1/aff2 flags (aff2 flags)

const menuChoice mobAff12FlagsChoiceAff2ExtraArr[] =
{ { "affect2 flags", 0, 0, offsetof(mobType, affect2Bits), mctMobAffect2, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice mobAff12FlagsChoiceArr[] =
{ { "affect1/2 flags", "affect1 flags", 0, offsetof(mobType, affect1Bits), mctMobAffect1, mclNone, mcdDefault, 
    0, mobAff12FlagsChoiceAff2ExtraArr }, { 0 } };

// extra data for aff3/aff4 flags (aff4 flags)

const menuChoice mobAff34FlagsChoiceAff4ExtraArr[] =
{ { "affect4 flags", 0, 0, offsetof(mobType, affect4Bits), mctMobAffect4, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice mobAff34FlagsChoiceArr[] =
{ { "affect3/4 flags", "affect3 flags", 0, offsetof(mobType, affect3Bits), mctMobAffect3, mclNone, mcdDefault, 
    0, mobAff34FlagsChoiceAff4ExtraArr }, { 0 } };

const menuChoice mobMiscChoiceArr[] =
{ { "species", 0, 0, offsetof(mobType, mobSpecies), mctSpecies, mclNone, mcdDefault, MAX_SPECIES_LEN, 0 }, 
  { "class(es)", 0, 0, offsetof(mobType, mobClass), mctMobClass, mclNone, mcdDefault, 0, 0 },
  { "specialization", 0, 0, offsetof(mobType, mobSpec), mctInt, mclMobSpec, mcdDefault, 0, 0 },
  { "hometown", 0, 0, offsetof(mobType, mobHometown), mctInt, mclHometown, mcdDefault, 0, 0 },
  { "alignment", 0, 0, offsetof(mobType, alignment), mctInt, mclNone, mcdDefault, 0, 0 },
  { "size", 0, 0, offsetof(mobType, size), mctInt, mclMobSize, mcdDefault, 0, 0 },
  { 0 } };

const menuChoice mobMisc2ChoiceArr[] =
{ { "level", 0, 0, offsetof(mobType, level), mctInt, mclNone, mcdDefault, 0, 0 }, 
  { "position", 0, 0, offsetof(mobType, position), mctInt, mclPosition, mcdDefault, 0, 0 },
  { "default pos", 0, 0, offsetof(mobType, defaultPos), mctInt, mclPosition, mcdDefault, 0, 0 },
  { "sex", 0, 0, offsetof(mobType, sex), mctInt, mclSex, mcdDefault, 0, 0 },
  { 0 } };

// extra data for mob misc 3 (silver, gold, platinum)

const menuChoice mobMisc3FlagsChoiceMoneyExtraArr[] =
{ { "silver", 0, 0, offsetof(mobType, silver), mctUInt, mclNone, mcdDefault, 0, 0 }, 
  { "gold", 0, 0, offsetof(mobType, gold), mctUInt, mclNone, mcdDefault, 0, 0 }, 
  { "platinum", 0, 0, offsetof(mobType, platinum), mctUInt, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice mobMisc3ChoiceArr[] =
{ { "money", "copper", 0, offsetof(mobType, copper), mctUInt, mclNone, mcdDefault, 0, 
    mobMisc3FlagsChoiceMoneyExtraArr }, 
  { "exp", 0, 0, offsetof(mobType, exp), mctUInt, mclNone, mcdDefault, 0, 0 },
  { "THAC0", 0, 0, offsetof(mobType, thac0), mctInt, mclNone, mcdDefault, 0, 0 },
  { "AC", 0, 0, offsetof(mobType, ac), mctInt, mclNone, mcdDefault, 0, 0 },
  { "hit points", 0, 0, offsetof(mobType, hitPoints), mctDieString, mclNone, mcdDefault, MAX_MOBHP_LEN, 0 },
  { "damage", 0, 0, offsetof(mobType, mobDamage), mctDieString, mclNone, mcdDefault, MAX_MOBDAM_LEN, 0 }, 
  { 0 } };

// extra data for mob quest/shop (shop)

const menuChoice mobQuestShopChoiceShopExtraArr[] =
{ { "shop", 0, 0, offsetof(mobType, shopPtr), mctPointerYesNoSansExists, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice mobQuestShopChoiceArr[] =
{ { "quest/shop", 0, 0, offsetof(mobType, questPtr), mctPointerYesNo, mclNone, mcdDefault, 0, 
    mobQuestShopChoiceShopExtraArr }, { 0 } };

// mob limit - offset used to store vnum for limit

menuChoice mobLimitChoiceExtraOverrideLimitArr[] =
{ { (char *)8675309, 0, 0, 0, mctMobLimitOverride, mclNone, mcdDefault, 0, 0 }, { 0 } };

menuChoice mobLimitChoiceArr[] =
{ { "limit override for all mobs of type", 0, 0, 0, mctMobLimit, mclNone, mcdDefault, 0, 
    mobLimitChoiceExtraOverrideLimitArr }, { 0 } };

const menuChoice mobVnumArr[] =
{ { "vnum", 0, 0, 0, mctVnum, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup mobGroupArr[] =
{ { mobShortNameChoiceArr, 'A', 0 },
  { mobLongNameChoiceArr, 'B', 0 },
  { mobKeywordsChoiceArr, 'C', 0 },
  { mobDescChoiceArr, 'D', 0 },
  { mobActFlagsChoiceArr, 'E', 0 },
  { mobAggro12FlagsChoiceArr, 'F', 'G' },
  { mobAggro3FlagsChoiceArr, 'P', 0 },
  { mobAff12FlagsChoiceArr, 'H', 'I' },
  { mobAff34FlagsChoiceArr, 'J', 'K' },
  { mobMiscChoiceArr, 'M', 0 },
  { mobMisc2ChoiceArr, 'N', 0 },
  { mobMisc3ChoiceArr, 'O', 0 },
  MENU_BLANK_LINE,
  { mobQuestShopChoiceArr, 'Q', 'S' },
  { mobLimitChoiceArr, 'L', 0 },
  { mobVnumArr, 'V', 0 },
  { 0, 0, 0 } };

menu g_mobMenu =
{ mobGroupArr, mtDisplayValuesInline };


//
// mob misc menu
//

const menuChoiceGroup mobMiscGroupArr[] =
{ { &mobMiscChoiceArr[0], 'A', 0 },
  { &mobMiscChoiceArr[1], 'B', 0 },
  { &mobMiscChoiceArr[2], 'C', 0 },
  { &mobMiscChoiceArr[3], 'D', 0 },
  { &mobMiscChoiceArr[4], 'E', 0 },
  { &mobMiscChoiceArr[5], 'F', 0 },
  { 0, 0, 0 } };

menu g_mobMiscMenu =
{ mobMiscGroupArr, mtDisplayValuesRight };


//
// mob misc 2 menu
//

const menuChoiceGroup mobMisc2GroupArr[] =
{ { &mobMisc2ChoiceArr[0], 'A', 0 },
  { &mobMisc2ChoiceArr[1], 'B', 0 },
  { &mobMisc2ChoiceArr[2], 'C', 0 },
  MENU_BLANK_LINE,
  { &mobMisc2ChoiceArr[3], 'D', 0 },
  { 0, 0, 0 } };

menu g_mobMisc2Menu =
{ mobMisc2GroupArr, mtDisplayValuesRight };


//
// mob misc 3 menu
//

menuChoice mobMisc3FirstNoteLine[] =
{ { "&+WNOTE:&n All of these values are automatically set when the mob loads into", 0, 0, 0, 
    mctDirectNoKey, mclNone, mcdDoNotDisplay, 0 } };
menuChoice mobMisc3SecondNoteLine[] =
{ { "      the mud.  Thus, they should generally be left at their defaults.", 0, 0, 0, 
    mctDirectNoKey, mclNone, mcdDoNotDisplay, 0 } };

const menuChoiceGroup mobMisc3GroupArr[] =
{ { mobMisc3FirstNoteLine, '.', 0 },
  { mobMisc3SecondNoteLine, '.', 0 },
  MENU_BLANK_LINE,
  { &mobMisc3ChoiceArr[0], 'A', 0 },
  MENU_BLANK_LINE,
  { &mobMisc3ChoiceArr[1], 'E', 0 },
  { &mobMisc3ChoiceArr[2], 'F', 0 },
  { &mobMisc3ChoiceArr[3], 'G', 0 },
  MENU_BLANK_LINE,
  { &mobMisc3ChoiceArr[4], 'H', 0 },
  { &mobMisc3ChoiceArr[5], 'I', 0 },
  { 0, 0, 0 } };

menu g_mobMisc3Menu =
{ mobMisc3GroupArr, mtDisplayValuesRight };


//
// quest message
//

menuChoice questMessageTriggersArr[] =
{ { "trigger keywords", 0, 0, offsetof(questMessage, keywordListHead), mctKeywords, mclNone, mcdDoNotDisplay, 
    MAX_QSTMSGKEY_LEN, 0 }, { 0 } };

menuChoice questMessageReplyArr[] =
{ { "reply", 0, 0, offsetof(questMessage, questMessageHead), mctDescription, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

menuChoice questMessageEchoArr[] =
{ { "echo", 0, "Echo message to all in room?", offsetof(questMessage, messageToAll), mctBoolYesNo, mclNone, 
    mcdDefault, 0, 0 }, { 0 } };

const menuChoiceGroup questMessageGroupArr[] =
{
  { questMessageTriggersArr, 'A', 0 },
  { questMessageReplyArr, 'B', 0 },
  { questMessageEchoArr, 'C', 0 },
  { 0, 0, 0 }
};

menu g_questMessageMenu =
{ questMessageGroupArr, mtDisplayValuesRight };


//
// quest quest
//

menuChoice questQuestFinishReplyArr[] =
{ { "finish reply", 0, 0, offsetof(questQuest, questReplyHead), mctDescription, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

menuChoice questQuestDisappearanceMessageArr[] =
{ { "disappearance message", 0, 0, offsetof(questQuest, disappearHead), mctQuestDisappearanceDescription, 
    mclNone, mcdDefault, 0, 0 }, { 0 } };

menuChoice questQuestEchoArr[] =
{ { "echo", 0, "Echo messages to all in room?", offsetof(questQuest, messagesToAll), mctBoolYesNo, mclNone, 
    mcdDefault, 0, 0 }, { 0 } };

const menuChoiceGroup questQuestGroupArr[] =
{
  { questQuestFinishReplyArr, 'A', 0 },
  { questQuestDisappearanceMessageArr, 'B', 0 },
  { questQuestEchoArr, 'C', 0 },
  MENU_BLANK_LINE,
  { 0, 0, 0 }
};

menu g_questQuestMenu =
{ questQuestGroupArr, mtDisplayValuesRight };


//
// quest item
//

menuChoice questItemGivenToMobItemTypeChoiceArr[] =
{ { "item type", 0, 0, offsetof(questItem, itemType), mctUInt, mclQuestItemGivenToMobType, mcdDefault, 0, 0 },
  { 0 } };

menuChoice questItemGivenToMobItemValueChoiceArr[] =
{ { "item value", 0, 0, offsetof(questItem, itemVal), mctInt, mclQuestItemGivenToMobValue, mcdDefault, 0, 0 },
  { 0 } };

const menuChoiceGroup questItemGivenToMobGroupArr[] =
{
  { questItemGivenToMobItemTypeChoiceArr, 'A', 0 },
  { questItemGivenToMobItemValueChoiceArr, 'B', 0 },
  { 0, 0, 0 }
};

menu questItemGivenToMobMenu =
{ questItemGivenToMobGroupArr, mtDisplayValuesRight };

menuChoice questItemGivenToPCItemTypeChoiceArr[] =
{ { "item type", 0, 0, offsetof(questItem, itemType), mctUInt, mclQuestItemGivenToPCType, mcdDefault, 0, 0 },
  { 0 } };

menuChoice questItemGivenToPCItemValueChoiceArr[] =
{ { "item value", 0, 0, offsetof(questItem, itemVal), mctInt, mclQuestItemGivenToPCValue, mcdDefault, 0, 0 },
  { 0 } };

const menuChoiceGroup questItemGivenToPCGroupArr[] =
{
  { questItemGivenToPCItemTypeChoiceArr, 'A', 0 },
  { questItemGivenToPCItemValueChoiceArr, 'B', 0 },
  { 0, 0, 0 }
};

menu g_questItemGivenToPCMenu =
{ questItemGivenToPCGroupArr, mtDisplayValuesRight };


//
// quest end choices
//

const menuChoice questDeleteChoiceArr[] =
{ { "Delete an entry", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice questCreateChoiceArr[] =
{ { "Create an entry", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoiceGroup questDeleteCreateGroupArr[] =
{
  MENU_BLANK_LINE,
  { questDeleteChoiceArr, 'Y', 0 },
  { questCreateChoiceArr, 'Z', 0 },
  { 0, 0, 0 }
};

menu g_questDeleteCreateMenu =
{ questDeleteCreateGroupArr, mtDisplayValuesRight };


//
// shop
//

const menuChoice shopListSellsChoiceArr[] =
{ { "list of items sold", 0, 0, offsetof(shop, producedItemList), mctNumbShopSold, mclNone, mcdDefault, 0, 
    0 }, { 0 } };

// extra data for buy/sell multiplier (sell multiplier)

const menuChoice shopBuySellMultChoiceSellExtraArr[] =
{ { "sell multiplier", 0, 0, offsetof(shop, sellMult), mctSellMultiplier, mclNone, mcdDefault, 0, 0 }, 
    { 0 } };

const menuChoice shopBuySellMultChoiceArr[] =
{ { "buy/sell multipliers", "buy multiplier", 0, offsetof(shop, buyMult), mctBuyMultiplier, mclNone, 
    mcdDefault, 0, shopBuySellMultChoiceSellExtraArr }, { 0 } };

const menuChoice shopListBuysChoiceArr[] =
{ { "list of item types bought", 0, 0, offsetof(shop, tradedItemList), mctNumbShopBought, mclNone, 
    mcdDefault, 0, 0 }, { 0 } };

const menuChoice shopMessagesChoiceArr[] =
{ { "messages", 0, 0, 0, mctString, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

// extra data for opening/closing times (close #1, open/close #2)

const menuChoice shopOpeningClosingChoiceTimeExtraArr[] =
{ { "first closing time", 0, 0, offsetof(shop, firstClose), mctShopFirstClose, mclNone, mcdPipeEverySecond, 0, 0 }, 
  { "second opening time", 0, 0, offsetof(shop, secondOpen), mctShopSecondOpen, mclNone, mcdPipeEverySecond, 0, 0 }, 
  { "second closing time", 0, 0, offsetof(shop, secondClose), mctShopSecondClose, mclNone, mcdPipeEverySecond, 0, 0 }, 
  { 0 } };

const menuChoice shopOpeningClosingChoiceArr[] =
{ { "opening/closing times", "first opening time", 0, offsetof(shop, firstOpen), mctShopFirstOpen, mclNone, 
    mcdPipeEverySecond, 0, shopOpeningClosingChoiceTimeExtraArr }, { 0 } };

const menuChoice shopYesNoChoiceExtraArr[] =
{ { "no magic", 0, "Is shop's room NO_MAGIC?", offsetof(shop, noMagic), mctBoolYesNo, mclNone, mcdDefault, 0, 
    0 },
  { "killable", 0, "Allow shopkeeper to be killed?", offsetof(shop, killable), mctBoolYesNo, mclNone, 
    mcdDefault, 0, 0 }, { 0 } };

const menuChoice shopYesNoChoiceArr[] =
{ { "roam/no magic/killable values", "roam", "Does shop roam?", offsetof(shop, roaming), mctBoolYesNo, 
    mclNone, mcdDefault, 0, shopYesNoChoiceExtraArr }, { 0 } };

menuChoice shopRacistChoiceExtraRaceArr[] =
{ { "shopkeeper race", 0, 0, offsetof(shop, shopkeeperRace), mctInt, mclShopkeeperRace, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice shopRacistChoiceArr[] =
{ { "racist info", "racist", "Is shopkeeper racist?", offsetof(shop, racist), mctBoolYesNo, mclNone, 
    mcdDefault, 0, shopRacistChoiceExtraRaceArr }, { 0 } };

const menuChoice shopDeleteChoiceArr[] =
{ { "Delete shop", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup shopGroupArr[] =
{ { shopListSellsChoiceArr, 'A', 0 },
  { shopBuySellMultChoiceArr, 'B', 'C' },
  { shopListBuysChoiceArr, 'D', 0 },
  { shopMessagesChoiceArr, 'E', 0 },
  { shopOpeningClosingChoiceArr, 'F', 0 },
  { shopYesNoChoiceArr, 'G', 0 },
  { shopRacistChoiceArr, 'H', 0 },
  MENU_BLANK_LINE,
  { shopDeleteChoiceArr, 'Y', 0 },
  { 0, 0, 0 } };

menu g_shopMenu =
{ shopGroupArr, mtDisplayValuesInline };


//
// shop messages
//

const menuChoice shopMessagesNotSellingItemChoiceArr[] =
{ { "'shop not selling that item' message", 0, 0, offsetof(shop, notSellingItem), mctShopMessage, mclNone, 
    mcdDoNotDisplay, MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesPlayerDoesntHaveItemChoiceArr[] =
{ { "'player doesn't have item' message", 0, 0, offsetof(shop, playerNoItem), mctShopMessage, 
    mclNone, mcdDoNotDisplay, MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesShopDoesntBuyItemChoiceArr[] =
{ { "'shop doesn't buy that type of item' message", 0, 0, offsetof(shop, shopNoTradeItem), mctShopMessage, 
    mclNone, mcdDoNotDisplay, MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesShopNoMoneyChoiceArr[] =
{ { "'shop lacks money' message", 0, 0, offsetof(shop, shopNoMoney), mctShopMessage, mclNone, 
    mcdDoNotDisplay, MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesPlayerNoMoneyChoiceArr[] =
{ { "'player lacks money' message", 0, 0, offsetof(shop, playerNoMoney), mctShopMessage, 
    mclNone, mcdDoNotDisplay, MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesSellChoiceArr[] =
{ { "'selling item to player' message", 0, 0, offsetof(shop, sellMessage), mctShopMessageProduced, mclNone, 
    mcdDoNotDisplay, MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesBuyChoiceArr[] =
{ { "'buying item from player' message", 0, 0, offsetof(shop, buyMessage), mctShopMessageTraded, mclNone, 
    mcdDoNotDisplay, MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesOpenChoiceArr[] =
{ { "opening message", 0, 0, offsetof(shop, openMessage), mctString, mclNone, mcdDoNotDisplay, 
    MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesCloseChoiceArr[] =
{ { "closing message", 0, 0, offsetof(shop, closeMessage), mctString, mclNone, mcdDoNotDisplay,
  MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoice shopMessagesRacistChoiceArr[] =
{ { "racist shopkeeper message", 0, 0, offsetof(shop, racistMessage), mctString, mclNone, mcdDoNotDisplay, 
    MAX_SHOPSTRING_LEN, 0 }, { 0 } };

const menuChoiceGroup shopMessagesGroupArr[] =
{ { shopMessagesNotSellingItemChoiceArr, 'A', 0 },
  { shopMessagesPlayerDoesntHaveItemChoiceArr, 'B', 0 },
  { shopMessagesShopDoesntBuyItemChoiceArr, 'C', 0 },
  { shopMessagesShopNoMoneyChoiceArr, 'D', 0 },
  { shopMessagesPlayerNoMoneyChoiceArr, 'E', 0 },
  { shopMessagesSellChoiceArr, 'F', 0 },
  { shopMessagesBuyChoiceArr, 'G', 0 },
  MENU_BLANK_LINE,
  { shopMessagesOpenChoiceArr, 'H', 0 },
  { shopMessagesCloseChoiceArr, 'I', 0 },
  MENU_BLANK_LINE,
  { shopMessagesRacistChoiceArr, 'J', 0 },
  { 0, 0, 0 } };

menu g_shopMessagesMenu =
{ shopMessagesGroupArr, mtDisplayValuesRight };


//
// shop times menu
//

const menuChoiceGroup shopTimesGroupArr[] =
{ { shopOpeningClosingChoiceArr, 'A', 0 },
  { 0, 0, 0 } };

menu g_shopTimesMenu =
{ shopTimesGroupArr, mtDisplayValuesRight };


//
// shop booleans menu
//

const menuChoiceGroup shopBooleansGroupArr[] = 
{ { shopYesNoChoiceArr, 'A', 0 },
  { 0, 0, 0 } };

menu g_shopBooleansMenu =
{ shopBooleansGroupArr, mtDisplayValuesRight };


//
// shop racist menu
//

const menuChoiceGroup shopRacistGroupArr[] =
{ { shopRacistChoiceArr, 'A', 0 },
  { 0, 0, 0 } };

menu g_shopRacistMenu =
{ shopRacistGroupArr, mtDisplayValuesRight };


//
// shop items menu
//

const menuChoice shopItemDeleteChoiceArr[] =
{ { "Delete an item", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice shopItemCreateChoiceArr[] =
{ { "Create an item", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoiceGroup shopItemGroupArr[] =
{ MENU_BLANK_LINE,
  { shopItemDeleteChoiceArr, 'Y', 0 },
  { shopItemCreateChoiceArr, 'Z', 0 },
  { 0, 0, 0 } };

menu g_shopItemMenu =
{ shopItemGroupArr, mtDisplayValuesInline };


//
// shop type menu
//

const menuChoice shopItemTypeDeleteChoiceArr[] =
{ { "Delete an item type", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice shopItemTypeCreateChoiceArr[] =
{ { "Create an item type", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoiceGroup shopItemTypeGroupArr[] =
{ MENU_BLANK_LINE,
  { shopItemTypeDeleteChoiceArr, 'Y', 0 },
  { shopItemTypeCreateChoiceArr, 'Z', 0 },
  { 0, 0, 0 } };

menu g_shopItemTypeMenu =
{ shopItemTypeGroupArr, mtDisplayValuesInline };


//
// zone
//

const menuChoice zoneNameChoiceArr[] =
{ { "name", 0, 0, offsetof(zone, zoneName), mctString, mclNone, mcdDoNotDisplay, MAX_ZONENAME_LEN }, 
  { 0 } };

const menuChoice zoneNumberChoiceArr[] =
{ { "number", 0, 0, offsetof(zone, zoneNumber), mctUInt, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

// extra data for misc (high lifespan)

const menuChoice zoneMiscChoiceLifespanExtraArr[] =
{ { "high lifespan (minutes)", 0, 0, offsetof(zone, lifeHigh), mctHighLifespan, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice zoneMiscChoiceArr[] =
{ { "lifespan", "low lifespan (minutes)", 0, offsetof(zone, lifeLow), mctLowLifespan, mclNone, mcdDefault, 0, 
    zoneMiscChoiceLifespanExtraArr },
  { "reset mode", 0, 0, offsetof(zone, resetMode), mctUInt, mclResetMode, mcdDefault, 0, 0 }, 
  { "difficulty (1-10)", 0, 0, offsetof(zone, zoneDiff), mctUInt, mclNone, mcdDefault, 0, 0 },
  { 0 } };

const menuChoice zoneFlagChoiceArr[] =
{ { "flags", 0, 0, offsetof(zone, miscBits), mctZoneFlag, mclNone, mcdDefault, 0, 0 },
  { 0 } };

const menuChoiceGroup zoneGroupArr[] =
{ { zoneNameChoiceArr, 'A', 0 },
  { zoneNumberChoiceArr, 'B', 0 },
  { zoneMiscChoiceArr, 'C', 0 },
  { zoneFlagChoiceArr, 'D', 0 },
  { 0, 0, 0 } };

menu g_zoneMenu =
{ zoneGroupArr, mtDisplayValuesInline };


//
// zone misc
//

const menuChoiceGroup zoneMiscGroupArr[] =
{ { &zoneMiscChoiceArr[0], 'A', 0 },
  { &zoneMiscChoiceArr[1], 'C', 0 },
  { &zoneMiscChoiceArr[2], 'D', 0 },
  { 0, 0, 0 } };

menu g_zoneMiscMenu =
{ zoneMiscGroupArr, mtDisplayValuesRight };


//
// exit menu
//

const menuChoice exitKeywordsChoiceArr[] =
{ { "keywords", 0, 0, offsetof(roomExit, keywordListHead), mctKeywords, mclNone, mcdDefault, MAX_EXITKEY_LEN, 
    0 }, 
  { 0 } };

const menuChoice exitDescChoiceArr[] =
{ { "description", 0, 0, offsetof(roomExit, exitDescHead), mctDescriptionNoStartBlankLines, mclNone, 
    mcdDefault, 0, 0 }, { 0 } };

// extra data for type/state flags (state flag)

const menuChoice exitTypeStateChoiceExtraStateArr[] =
{ { "door state", 0, 0, offsetof(roomExit, zoneDoorState), mctInt, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice exitTypeStateChoiceArr[] =
{ { "door type/state", "door type", 0, offsetof(roomExit, worldDoorType), mctInt, mclNone, mcdDefault, 0, 
    exitTypeStateChoiceExtraStateArr }, { 0 } };

const menuChoice exitKeyObjectNumberChoiceArr[] =
{ { "key object number", 0, 0, offsetof(roomExit, keyNumb), mctInt, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoice exitDestRoomChoiceArr[] =
{ { "destination room", 0, 0, offsetof(roomExit, destRoom), mctInt, mclNone, mcdDefault, 0, 0 }, 
  { 0 } };

const menuChoiceGroup exitGroupArr[] =
{ { exitKeywordsChoiceArr, 'A', 0 },
  { exitDescChoiceArr, 'B', 0 },
  { exitTypeStateChoiceArr, 'C', 0 },
  { exitKeyObjectNumberChoiceArr, 'D', 0 },
  { exitDestRoomChoiceArr, 'E', 0 },
  { 0, 0, 0 } };

menu g_exitMenu =
{ exitGroupArr, mtDisplayValuesInline };


//
// exit state menu(s) - top menu
//

const menuChoice exitStateTopTypeNoDoorChoiceArr[] =
{ { "&+c(.wld) &+wSet door type to \"no door\"", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice exitStateTopTypeNoKeyChoiceArr[] =
{ { "&+c(.wld) &+wSet door type to \"door (no key)\"", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, 
  { 0 } };

const menuChoice exitStateTopTypeReqKeyChoiceArr[] =
{ { "&+c(.wld) &+wSet door type to \"door (req key)\"", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, 
  { 0 } };

const menuChoice exitStateTopTypeUnpickableChoiceArr[] =
{ { "&+c(.wld) &+wSet door type to \"unpickable door (req key)\"", 0, 0, 0, mctDirect, mclNone, 
    mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice exitStateTopWldSecretChoiceArr[] =
{ { "&+c(.wld) &+wToggle secret door flag", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice exitStateTopWldBlockedChoiceArr[] =
{ { "&+c(.wld) &+wToggle blocked door flag", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoiceGroup exitStateTopGroupArr[] =
{ { exitStateTopTypeNoDoorChoiceArr, 'A', 0 },
  { exitStateTopTypeNoKeyChoiceArr, 'B', 0 },
  { exitStateTopTypeReqKeyChoiceArr, 'C', 0 },
  { exitStateTopTypeUnpickableChoiceArr, 'D', 0 },
  MENU_BLANK_LINE,
  { exitStateTopWldSecretChoiceArr, 'E', 0 },
  { exitStateTopWldBlockedChoiceArr, 'F', 0 },
  MENU_BLANK_LINE,
  { 0, 0, 0 } };

menu g_exitStateTopMenu =
{ exitStateTopGroupArr, mtDisplayValuesInline };


//
// exit state bottom
//

const menuChoice exitStateBottomZonSecretChoiceArr[] =
{ { "&+c(.zon) &+wToggle secret door flag", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice exitStateBottomZonBlockedChoiceArr[] =
{ { "&+c(.zon) &+wToggle blocked door flag", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoiceGroup exitStateBottomGroupArr[] =
{ MENU_BLANK_LINE,
  { exitStateBottomZonSecretChoiceArr, 'J', 0 },
  { exitStateBottomZonBlockedChoiceArr, 'K', 0 },
  { 0, 0, 0 } };

menu g_exitStateBottomMenu =
{ exitStateBottomGroupArr, mtDisplayValuesInline };


//
// room exit menu
//

const menuChoice roomExitDeleteChoiceArr[] =
{ { "Delete exit", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice roomExitCreateChoiceArr[] =
{ { "Create exit", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoiceGroup roomExitGroupArr[] =
{ MENU_BLANK_LINE,
  { roomExitDeleteChoiceArr, 'Y', 0 },
  { roomExitCreateChoiceArr, 'Z', 0 },
  { 0, 0, 0 } };

menu g_roomExitMenu =
{ roomExitGroupArr, mtDisplayValuesInline };


//
// extra desc menu
//

const menuChoice edescKeywordsChoiceArr[] =
{ { "keywords", 0, 0, offsetof(extraDesc, keywordListHead), mctKeywords, mclNone, mcdDoNotDisplay, 
    MAX_EDESCKEY_LEN, 0 }, { 0 } };

const menuChoice edescDescChoiceArr[] =
{ { "description", 0, 0, offsetof(extraDesc, extraDescStrnHead), mctDescription, mclNone, mcdDefault, 0, 0 },
  { 0 } };

const menuChoiceGroup edescGroupArr[] =
{ { edescKeywordsChoiceArr, 'A', 0 },
  { edescDescChoiceArr, 'B', 0 },
  { 0, 0, 0 } };

menu g_edescMenu =
{ edescGroupArr, mtDisplayValuesInline };


//
// object extra desc menu
//

const menuChoice objEdescKeywordsChoiceArr[] =
{ { "ID keywords", 0, 0, 0, mctObjIDKeywords, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objEdescShortNameChoiceArr[] =
{ { "ID short name", 0, 0, 0, mctObjIDShort, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objEdescLongNameChoiceArr[] =
{ { "ID long name", 0, 0, 0, mctObjIDLong, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice edescDeleteChoiceArr[] =
{ { "Delete an extra desc", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoice edescCreateChoiceArr[] =
{ { "Create an extra desc", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup objEdescGroupArr[] =
{ MENU_BLANK_LINE,
  { objEdescKeywordsChoiceArr, 'U', 0 },
  { objEdescShortNameChoiceArr, 'V', 0 },
  { objEdescLongNameChoiceArr, 'W', 0 },
  MENU_BLANK_LINE,
  { edescDeleteChoiceArr, 'Y', 0 },
  { edescCreateChoiceArr, 'Z', 0 },
  { 0, 0, 0 } };

menu g_objEdescMenu =
{ objEdescGroupArr, mtDisplayValuesInline };


//
// room extra desc menu
//

const menuChoiceGroup roomEdescGroupArr[] =
{ MENU_BLANK_LINE,
  { edescDeleteChoiceArr, 'Y', 0 },
  { edescCreateChoiceArr, 'Z', 0 },
  { 0, 0, 0 } };

menu g_roomEdescMenu =
{ roomEdescGroupArr, mtDisplayValuesInline };


//
// main check menu
//

const menuChoice checkRoomExitChoiceArr[] =
{ { "room/exit checking options", 0, 0, 0, mctString, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice checkObjectChoiceArr[] =
{ { "object checking options", 0, 0, 0, mctString, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice checkMobChoiceArr[] =
{ { "mob checking options", 0, 0, 0, mctString, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice checkMiscChoiceArr[] =
{ { "miscellaneous checking options", 0, 0, 0, mctString, mclNone, mcdDoNotDisplay, 0 }, { 0 } };

const menuChoice checkAutoCheckChoiceArr[] =
{ { VAR_CHECKSAVE_NAME, 0, "Automatically run check every save?", (size_t)&getCheckSaveVal, mctVarYesNo, 
    mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice checkAbortSavingChoiceArr[] =
{ { VAR_NOSAVEONCHECKERR_NAME, 0, "Abort saving if any errors found?", (size_t)&getNoSaveonCheckErrVal, 
    mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice checkWriteToCheckLogChoiceArr[] =
{ { VAR_SAVECHECKLOG_NAME, 0, "Write errors to CHECK.LOG as well as screen?", (size_t)&getSaveCheckLogVal, 
    mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice checkPauseEveryScreenfulCheckErrorsChoiceArr[] =
{ { VAR_PAUSECHECKSCREENFUL_NAME, 0, "Pause every screenful of check errors?", 
    (size_t)&getPauseCheckScreenfulVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice checkTurnAllOnOffChoiceArr[] =
{ { "Turn &+Wall&+w check options off/on", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup checkGroupArr[] =
{ { checkRoomExitChoiceArr, 'A', 0 },
  { checkObjectChoiceArr, 'B', 0 },
  { checkMobChoiceArr, 'C', 0 },
  { checkMiscChoiceArr, 'D', 0 },
  MENU_BLANK_LINE,
  { checkAutoCheckChoiceArr, 'E', 0 },
  { checkAbortSavingChoiceArr, 'F', 0 },
  { checkWriteToCheckLogChoiceArr, 'G', 0 },
  { checkPauseEveryScreenfulCheckErrorsChoiceArr, 'H', 0 },
  MENU_BLANK_LINE,
  { checkTurnAllOnOffChoiceArr, 'Y', 'Z' },
  { 0, 0, 0 } };

menu g_checkMenu =
{ checkGroupArr, mtDisplayValuesRight };


//
// misc check menu
//

const menuChoice miscCheckExtraDescNoStringsChoiceArr[] =
{ { VAR_CHECKEDESC_NAME, 0, "Check for extra descs with no keywords/desc?", (size_t)&getCheckEdescVal, 
    mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice miscCheckMissetFlagsChoiceArr[] =
{ { VAR_CHECKFLAGS_NAME, 0, "Check all room/object/mob flags for misset flags?", 
    (size_t)&getCheckFlagsVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice miscCheckZoneChoiceArr[] =
{ { VAR_CHECKZONE_NAME, 0, "Check zone settings?", (size_t)&getCheckZoneVal, 
    mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice miscCheckTurnAllOnOffChoiceArr[] =
{ { "Turn all miscellaneous check options off/on", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 }, 
  { 0 } };

const menuChoiceGroup miscCheckGroupArr[] =
{ { miscCheckExtraDescNoStringsChoiceArr, 'A', 0 },
  { miscCheckMissetFlagsChoiceArr, 'B', 0 },
  { miscCheckZoneChoiceArr, 'C', 0 },
  MENU_BLANK_LINE,
  { miscCheckTurnAllOnOffChoiceArr, 'Y', 'Z' },
  { 0, 0, 0 } };

menu g_miscCheckMenu =
{ miscCheckGroupArr, mtDisplayValuesRight };


//
// mob check menu
//

const menuChoice mobCheckTypesNotLoadedChoiceArr[] =
{ { VAR_CHECKLOADED_NAME, 0, "Check for object/mob types that aren't loaded?", (size_t)&getCheckLoadedVal, 
    mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice mobCheckMobsChoiceArr[] =
{ { VAR_CHECKMOB_NAME, 0, "Check for mobs with illegal values?", 
    (size_t)&getCheckMobVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice mobCheckMobDescsChoiceArr[] =
{ { VAR_CHECKMOBDESC_NAME, 0, "Check for mobs with no descriptions?", (size_t)&getCheckMobDescVal, 
    mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice mobCheckMobGuildChoiceArr[] =
{ { VAR_CHECKGUILDSTUFF_NAME, 0, "Check objects/mobs for guild-restricted stuff?", 
    (size_t)&getCheckGuildStuffVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice mobCheckTurnAllOnOffChoiceArr[] =
{ { "Turn all mob check options off/on", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup mobCheckGroupArr[] =
{ { mobCheckTypesNotLoadedChoiceArr, 'A', 0 },
  { mobCheckMobsChoiceArr, 'B', 0 },
  { mobCheckMobDescsChoiceArr, 'C', 0 },
  { mobCheckMobGuildChoiceArr, 'D', 0 },
  MENU_BLANK_LINE,
  { mobCheckTurnAllOnOffChoiceArr, 'Y', 'Z' },
  { 0, 0, 0 } };

menu g_mobCheckMenu =
{ mobCheckGroupArr, mtDisplayValuesRight };


//
// object check menu
//

const menuChoice objCheckTypesNotLoadedChoiceArr[] =
{ { VAR_CHECKLOADED_NAME, 0, "Check for object/mob types that aren't loaded?", (size_t)&getCheckLoadedVal, 
    mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objCheckObjsChoiceArr[] =
{ { VAR_CHECKOBJ_NAME, 0, "Check for objects with illegal values?", 
    (size_t)&getCheckObjVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objCheckObjValuesChoiceArr[] =
{ { VAR_CHECKOBJVAL_NAME, 0, "Check 'object value' values based on object type?", 
    (size_t)&getCheckObjValuesVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objCheckObjEdescChoiceArr[] =
{ { VAR_CHECKOBJDESC_NAME, 0, "Check for objects with no extra descriptions?", 
    (size_t)&getCheckObjDescVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objCheckKeysChoiceArr[] =
{ { VAR_CHECKMISSINGKEYS_NAME, 0, "Check for missing/extraneous keys?", 
    (size_t)&getCheckMissingKeysVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objCheckObjMaterialChoiceArr[] =
{ { VAR_CHECKOBJMATERIAL_NAME, 0, "Check for objects with an invalid material?", 
    (size_t)&getCheckObjMaterialVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objCheckObjGuildChoiceArr[] =
{ { VAR_CHECKGUILDSTUFF_NAME, 0, "Check objects/mobs for guild-restricted stuff?", 
    (size_t)&getCheckGuildStuffVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice objCheckTurnAllOnOffChoiceArr[] =
{ { "Turn all object check options off/on", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup objCheckGroupArr[] =
{ { objCheckTypesNotLoadedChoiceArr, 'A', 0 },
  { objCheckObjsChoiceArr, 'B', 0 },
  { objCheckObjValuesChoiceArr, 'C', 0 },
  { objCheckObjEdescChoiceArr, 'D', 0 },
  { objCheckKeysChoiceArr, 'E', 0 },
  { objCheckObjMaterialChoiceArr, 'F', 0 },
  { objCheckObjGuildChoiceArr, 'G', 0 },
  MENU_BLANK_LINE,
  { objCheckTurnAllOnOffChoiceArr, 'Y', 'Z' },
  { 0, 0, 0 } };

menu g_objCheckMenu =
{ objCheckGroupArr, mtDisplayValuesRight };


//
// room check menu
//

const menuChoice roomCheckStrandedChoiceArr[] =
{ { VAR_CHECKLONEROOM_NAME, 0, "Check for 'stranded' rooms/rooms with no exits in/out?", 
    (size_t)&getCheckLoneRoomVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice roomCheckKeysChoiceArr[] =
{ { VAR_CHECKMISSINGKEYS_NAME, 0, "Check for missing/extraneous keys?", 
    (size_t)&getCheckMissingKeysVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice roomCheckRoomsChoiceArr[] =
{ { VAR_CHECKROOM_NAME, 0, "Check for rooms with illegal values/no descs?", 
    (size_t)&getCheckRoomVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice roomCheckExitsChoiceArr[] =
{ { VAR_CHECKEXIT_NAME, 0, "Check for exits with illegal values?", 
    (size_t)&getCheckExitVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice roomCheckExitDescsChoiceArr[] =
{ { VAR_CHECKEXITDESC_NAME, 0, "Check for exits with no descriptions?", 
    (size_t)&getCheckExitDescVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice roomCheckTurnAllOnOffChoiceArr[] =
{ { "Turn all room check options off/on", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 }, { 0 } };

const menuChoiceGroup roomCheckGroupArr[] =
{ { roomCheckStrandedChoiceArr, 'A', 0 },
  { roomCheckKeysChoiceArr, 'B', 0 },
  { roomCheckRoomsChoiceArr, 'C', 0 },
  { roomCheckExitsChoiceArr, 'D', 0 },
  { roomCheckExitDescsChoiceArr, 'E', 0 },
  MENU_BLANK_LINE,
  { roomCheckTurnAllOnOffChoiceArr, 'Y', 'Z' },
  { 0, 0, 0 } };

menu g_roomCheckMenu =
{ roomCheckGroupArr, mtDisplayValuesRight };


//
// configuration menu
//

const menuChoice configVnumExistChoiceArr[] =
{ { VAR_VNUMCHECK_NAME, 0, "Check all vnums and vnum input to make sure they exist?", 
    (size_t)&getVnumCheckVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configZoneFlagsChoiceArr[] =
{ { VAR_CHECKZONEFLAGS_NAME, 0, "Upon loading, check all .wld zone flags vs. .zon numb?", 
    (size_t)&getCheckZoneFlagsVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configMenuExtraInfoChoiceArr[] =
{ { VAR_SHOWMENUINFO_NAME, 0, "Show extra info about entity being edited on menus?", 
    (size_t)&getShowMenuInfoVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configWalkCreateChoiceArr[] =
{ { VAR_WALKCREATE_NAME, 0, "Enable 'create room as you walk' creation?", 
    (size_t)&getWalkCreateVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configSaveVnumChoiceArr[] =
{ { VAR_SROOMACTIVE_NAME, 0, "Save vnum of current room for reentrance when reloading?", 
    (size_t)&getStartRoomActiveVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configNegOneOutOfZoneChoiceArr[] =
{ { VAR_NEGDESTOUTOFZONE_NAME, 0, "Consider exits with destinations of -1 out of zone?", 
    (size_t)&getNegDestOutofZoneVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configShopPricesAdjustedChoiceArr[] =
{ { VAR_SHOWPRICESADJUSTED_NAME, 0, "Show shop prices adjusted for mob's sell percentage?", 
    (size_t)&getShowPricesAdjustedVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configAutoSaveChoiceArr[] =
{ { VAR_SAVEEVERYXCOMMANDS_NAME, 0, "Automatically save zone based on 'save when' below?", 
    (size_t)&getSaveEveryXCommandsVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configAutoSaveWhenChoiceArr[] =
{ { VAR_SAVEHOWOFTEN_NAME, 0, "If autosave is on, save every X commands ...", 
    (size_t)&getSaveHowOftenVal, mctVarUInt, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configScreenHeightChoiceArr[] =
{ { VAR_SCREENHEIGHT_NAME, 0, "Screen height", 
    (size_t)&getScreenHeight, mctVarUInt, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configScreenWidthChoiceArr[] =
{ { VAR_SCREENWIDTH_NAME, 0, "Screen width", 
    (size_t)&getScreenWidth, mctVarUInt, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configDescEditorChoiceArr[] =
{ { VAR_TEXTEDIT_NAME, 0, "External desc editor", 
    (size_t)&getEditorName, mctVarString, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configMenuPromptChoiceArr[] =
{ { VAR_MENUPROMPT_NAME, 0, "Menu prompt", 
    (size_t)&getMenuPromptName, mctVarString, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice configMainPromptChoiceArr[] =
{ { VAR_MAINPROMPT_NAME, 0, "Main prompt", 
    (size_t)&getMainPromptStrn, mctVarString, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoiceGroup configGroupArr[] =
{ { configVnumExistChoiceArr, 'A', 0 },
  { configZoneFlagsChoiceArr, 'B', 0 },
  { configMenuExtraInfoChoiceArr, 'C', 0 },
  MENU_BLANK_LINE,
  { configWalkCreateChoiceArr, 'D', 0 },
  { configSaveVnumChoiceArr, 'E', 0 },
  { configNegOneOutOfZoneChoiceArr, 'F', 0 },
  { configShopPricesAdjustedChoiceArr, 'G', 0 },
  { configAutoSaveChoiceArr, 'H', 0 },
  { configAutoSaveWhenChoiceArr, 'I', 0 },
  MENU_BLANK_LINE,
  { configScreenHeightChoiceArr, 'J', 0 },
  { configScreenWidthChoiceArr, 'K', 0 },
  MENU_BLANK_LINE,
  { configDescEditorChoiceArr, 'L', 0 },
  { configMenuPromptChoiceArr, 'M', 0 },
  { configMainPromptChoiceArr, 'N', 0 },
  { 0, 0, 0 } };

menu g_configMenu =
{ configGroupArr, mtDisplayValuesRight };


//
// display menu
//

const menuChoice displayInterpretColorChoiceArr[] =
{ { VAR_INTERPCOLOR_NAME, 0, "Interpret Duris color codes?", 
    (size_t)&getInterpColorVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayDisplayColorChoiceArr[] =
{ { VAR_SHOWCOLOR_NAME, 0, "Display Duris color codes?", 
    (size_t)&getShowColorVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayRoomExtraChoiceArr[] =
{ { VAR_SHOWROOMEXTRA_NAME, 0, "Show room 'extra info' (sector type and flags)?", 
    (size_t)&getShowRoomExtraVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayRoomVnumChoiceArr[] =
{ { VAR_SHOWROOMVNUM_NAME, 0, "Show room vnum after room name?", 
    (size_t)&getShowRoomVnumVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayExitFlagsChoiceArr[] =
{ { VAR_SHOWEXITFLAGS_NAME, 0, "Show exit flags after exit name?", 
    (size_t)&getShowExitFlagsVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayExitVnumChoiceArr[] =
{ { VAR_SHOWEXITDEST_NAME, 0, "Show exit room vnum dest after exit name?", 
    (size_t)&getShowExitDestVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayObjContentsChoiceArr[] =
{ { VAR_SHOWCONTENTS_NAME, 0, "Show container contents in inventory, on mobs, etc?", 
    (size_t)&getShowObjContentsVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayObjFlagsChoiceArr[] =
{ { VAR_SHOWOBJFLAGS_NAME, 0, "Show object flags info before name?", 
    (size_t)&getShowObjFlagsVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayObjVnumChoiceArr[] =
{ { VAR_SHOWOBJVNUM_NAME, 0, "Show object vnum after object name?", 
    (size_t)&getShowObjVnumVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayMobFlagsChoiceArr[] =
{ { VAR_SHOWMOBFLAGS_NAME, 0, "Show mob flags info before name?", 
    (size_t)&getShowMobFlagsVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayMobPosChoiceArr[] =
{ { VAR_SHOWMOBPOS_NAME, 0, "Show mob default pos after name?", 
    (size_t)&getShowMobPosVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayMobVnumChoiceArr[] =
{ { VAR_SHOWMOBVNUM_NAME, 0, "Show mob vnum after mob name?", 
    (size_t)&getShowMobVnumVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayMobFollowRideChoiceArr[] =
{ { VAR_SHOWMOBRIDEFOLLOW_NAME, 0, "Show mobs following/riding/ridden by others?", 
    (size_t)&getShowMobRideFollowVal, mctVarYesNo, mclNone, mcdDefault, 0, 0 }, { 0 } };

const menuChoice displayTurnAllOnOffChoiceArr[] =
{ { "Turn all vnum/extra info display options off/on", 0, 0, 0, mctDirect, mclNone, mcdDoNotDisplay, 0, 0 },
  { 0 } };

const menuChoiceGroup displayGroupArr[] =
{ { displayInterpretColorChoiceArr, 'A', 0 },
  { displayDisplayColorChoiceArr, 'B', 0 },
  MENU_BLANK_LINE,
  { displayRoomExtraChoiceArr, 'C', 0 },
  { displayRoomVnumChoiceArr, 'D', 0 },
  { displayExitFlagsChoiceArr, 'E', 0 },
  { displayExitVnumChoiceArr, 'F', 0 },
  MENU_BLANK_LINE,
  { displayObjContentsChoiceArr, 'G', 0 },
  { displayObjFlagsChoiceArr, 'H', 0 },
  { displayObjVnumChoiceArr, 'I', 0 },
  MENU_BLANK_LINE,
  { displayMobFlagsChoiceArr, 'J', 0 },
  { displayMobPosChoiceArr, 'K', 0 },
  { displayMobVnumChoiceArr, 'L', 0 },
  { displayMobFollowRideChoiceArr, 'M', 0 },
  MENU_BLANK_LINE,
  { displayTurnAllOnOffChoiceArr, 'Y', 'Z' },
  { 0, 0, 0 } };

menu g_dispMenu =
{ displayGroupArr, mtDisplayValuesRight };
