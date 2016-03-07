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


// MOB.H - useful stuff for mobs

#ifndef _MOB_H_

#include "quest/quest.h"
#include "shop/shop.h"

// mob positions

#define POSITION_LOWEST       0
#define POSITION_LOWEST_LEGAL 4  // lowest "legal" value for a loaded mob
#define POSITION_DEAD         0
#define POSITION_MORTALLYW    1
#define POSITION_INCAP        2
#define POSITION_STUNNED      3
#define POSITION_SLEEPING     4
#define POSITION_RESTING      5
#define POSITION_SITTING      6
#define POSITION_FIGHTING     7
#define POSITION_STANDING     8
#define POSITION_PRONE        9
#define POSITION_LEVITATED   10
#define POSITION_FLYING      11
#define POSITION_SWIMMING    12   // new flag
#define POSITION_HIGHEST     12

#define NUMB_MOB_POSITIONS   13


// mob sex

#define SEX_LOWEST    0
#define SEX_NEUTER    0
#define SEX_MALE      1
#define SEX_FEMALE    2
#define SEX_HIGHEST   2

#define NUMB_MOB_SEXES 3

// mob size info

#define MOB_SIZE_LOWEST     0
#define MOB_SIZE_NONE       0
#define MOB_SIZE_TINY       1
#define MOB_SIZE_SMALL      2
#define MOB_SIZE_MEDIUM     3
#define MOB_SIZE_LARGE      4
#define MOB_SIZE_HUGE       5
#define MOB_SIZE_GIANT      6
#define MOB_SIZE_GARGANTUAN 7
#define MOB_SIZE_HIGHEST    7
#define MOB_SIZE_DEFAULT  (-1)

#define NUMB_MOB_SIZES      9 // including 'default'

// hometown stuff

#define HOME_LOWEST              0
#define HOME_NONE                0
#define HOME_WINTERHAVEN         1      /* all goodies */
#define HOME_IXARKON             2      /* illithid */
#define HOME_ARACHDRATHOS        3      /* drow elf */
#define HOME_SYLVANDAWN          4      /* grey-elf, half-elf */
#define HOME_KIMORDRIL           5      /* mountain dwarf */
#define HOME_KHILDARAK           6      /* duergar - trip's town */
#define HOME_WOODSEER            7      /* halfling */
#define HOME_ASHRUMITE           8      /* gnome */
#define HOME_FAANG               9      /* ogre */
#define HOME_GHORE              10      /* troll */
#define HOME_UGTA               11      /* barbarians */
#define HOME_BLOODSTONE         12      /* evil humans */
#define HOME_SHADY              13      /* orcs */
#define HOME_NAXVARAN           14
#define HOME_MARIGOT            15
#define HOME_CHARIN             16
#define HOME_CITYRUINS          17
#define HOME_KHHIYTIK           18      /* thrikreen */
#define HOME_GITHFORT           19      /* githyanki */
#define HOME_GOBLIN             20      /* goblin */
#define HOME_HARPY              21
#define HOME_NEWBIE             22
#define HOME_HIGHEST            22      /* number of last hometown */

#define NUMB_MOB_HOMETOWNS      (HOME_HIGHEST - HOME_LOWEST) + 1

// record etc for mobs themselves

#define MAX_MOBSNAME_LEN   (usint)256
#define MAX_MOBLNAME_LEN   (usint)256
#define MAX_MOBKEY_LEN     (usint)256

#define MAX_SPECIES_LEN  (usint)2
#define MAX_MOBHP_LEN    (usint)21
#define MAX_MOBDAM_LEN   (usint)21

typedef struct _mobType
{
  uint mobNumber;  // mob number

  stringNode *keywordListHead;  // keyword list head

  char mobShortName[MAX_MOBSNAME_LEN + 1];  // short name of mob
  char mobLongName[MAX_MOBLNAME_LEN + 1];   // long name of mob

  stringNode *mobDescHead;  // head of mob description

  uint actionBits;
  uint aggroBits;
  uint aggro2Bits;
  uint aggro3Bits;
  uint affect1Bits;
  uint affect2Bits;
  uint affect3Bits;
  uint affect4Bits;

  int alignment;

  char mobSpecies[MAX_SPECIES_LEN + 1];
  int mobHometown;
  uint mobClass;
  uint mobSpec;

  int level;
  int thac0;
  int ac;
  char hitPoints[MAX_MOBHP_LEN + 1];
  char mobDamage[MAX_MOBDAM_LEN + 1];

  uint exp;

  uint copper;
  uint silver;
  uint gold;
  uint platinum;

  int position;
  int defaultPos;
  int sex;

  int size;

  quest *questPtr;
  shop *shopPtr;

  bool defaultMob;
} mobType;


#define _MOB_H_
#endif
