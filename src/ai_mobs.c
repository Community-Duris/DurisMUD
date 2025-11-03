/****************************************************************************
 *
 *  File: ai_mob.c                                           Part of Duris
 *  Usage: ai_mob.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark                   Date: 2003-10-24
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "mm.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "weather.h"
#include "sound.h"
#include "objmisc.h"

const int path[5] = {
  123113,
  125436,
  126849,
  129367
};

//Init function
int init()
{
  return 0;
}


int startMoving(P_char ch)
{
  return 0;
}


int ai_mob_proc(P_char ch, P_char vict, int cmd, char *arg)
{
//If i'm fighting what do i do
//If i see a P what do i do
//Let's move towards a new destination
// as this proc dosn't work yet, and completely destroys all commands in it's room
// let's not use this proc yet.
return FALSE;
startMoving(ch);

}
