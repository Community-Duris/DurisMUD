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


#ifndef _MOBHERE_H_

#include "zone/zone.h"
#include "mob.h"
#include "obj/objhere.h"

#define EQ_WEARABLE      0  // eq is wearable, no error
#define EQ_SLOT_FILLED 127  // slot is already filled
#define EQ_RESCLASS    125  // can't wear because of class
#define EQ_NO_WEARBITS 124  // obj has no wear bits
#define EQ_ERROR       123  // mob pointer is null or some other silliness
#define EQ_WEAR_NOTSET 121  // "where" location specified, but appropriate
                            // wear bit not set
#define EQ_WRONGRACE   120  // wrong race - human trying to wear a
                            // horseshoe, whatever
#define EQ_RACECANTUSE 119  // thri-kreen can't use earrings etc etc
#define EQ_NOBELTATTCH 118  // trying to attach a beltable object but no belt
#define EQ_RESRACE     117  // restricted by race (anti/anti2 bits)

// "mob here" record - used for mobs in a room

typedef struct _mobHere
{
  uint mobNumb;       // number of mob
  mobType *mobPtr;     // points to address of mob type

  objectHere *inventoryHead;  // objects mob is carrying
  objectHere *equipment[WEAR_TRUEHIGH + 1];

  uint randomChance;

//  bool tableMob;     // if TRUE, mobNumb is number of table

  struct _mobHere *riding;     // mob this mob is riding, if any
  struct _mobHere *riddenBy;   // mob this mob is ridden by, if any

  struct _mobHere *following;  // mob this mob is following, if any

  struct _mobHere *Next;
} mobHere;


#define _MOBHERE_H_
#endif
