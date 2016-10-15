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


// OBJECT.H - useful stuff for objects

#ifndef _OBJECT_H_

#include "edesc/edesc.h"

#include "../types.h"
#include "misc/strnnode.h"
#include "defines.h"  // numb_obj_vals

// object types

#define NUMB_ITEM_TYPES (ITEM_HIGHEST - ITEM_LOWEST) + 1
#define ITEM_DEFAULT    ITEM_TRASH

// numb flags

#define NUMB_OBJ_ARMOR_MISC_FLAGS    1
#define NUMB_OBJ_CONT_FLAGS          5
#define NUMB_OBJ_TOTEM_SPHERE_FLAGS  6
#define NUMB_OBJ_SHIELD_MISC_FLAGS   1

// record for "object apply" stuff

typedef struct _objApplyRec
{
  uint applyWhere;
  uint applyWhere2;

  int applyModifier;
} objApplyRec;


// record etc for objects themselves

#define MAX_OBJLNAME_LEN  (usint)256
#define MAX_OBJSNAME_LEN  (usint)256
#define MAX_OBJKEY_LEN    (usint)256 // used for input field in edit obj menu

#define NUMB_OBJ_APPLIES   (usint)4

typedef struct _objectType
{
  uint objNumber;  // object number

  char objShortName[MAX_OBJSNAME_LEN + 1];  // short name of object
  char objLongName[MAX_OBJLNAME_LEN + 1];   // long name of object

  stringNode *keywordListHead;  // keyword list head

  extraDesc *extraDescHead;     // list of extra descs

  uint objType;
  uint material;
  uint size;
  uint space;

  uint extraBits;
  uint extra2Bits;
  uint wearBits;
  uint antiBits;
  uint anti2Bits;

  uint affect1Bits;
  uint affect2Bits;
  uint affect3Bits;
  uint affect4Bits;

  int objValues[NUMB_OBJ_VALS];

  int weight;
  uint worth;
  uint condition;
  uint craftsmanship;
  uint damResistBonus;

  objApplyRec objApply[NUMB_OBJ_APPLIES];

  uint trapBits;
  int trapDam;
  int trapCharge;
  uint trapLevel;

  bool defaultObj;
} objectType;

int itemvalue( objectType *obj );

#define OBJ_DEFAULT_LONG_DESC "An unnamed object lies here, looking lonely and forelorn."
#define OBJ_DEFAULT_SHORT_DESC "an unnamed object"

#define _OBJECT_H_
#endif
