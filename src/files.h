/*****************************************************************************
 * File: files.h - header info for save/restore.             Part of Duris *
 * Usage: common values needed for player save file (pfile) manipulation     *
 * Written by: John Bashaw                                    Copyright 1994 *
 *****************************************************************************/

#ifndef _SOJ_PFILES_H_
#define _SOJ_PFILES_H_

#include <ctype.h>
#include <errno.h>
#ifdef _SUN4_SOURCE
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "prototypes.h"
#include "skillrec.h"
#include "structs.h"
#include "utils.h"

/* defines */
#define ENABLE_JSON_PFILE

#define SAFE_STRING(str) str == NULL ? "" : str

#define SAV_STATVERS  47
#define SAV_SKILLVERS 2
#define SAV_ITEMVERS  35
#define SAV_AFFVERS   7
//#define SAV_MAXSIZE 65536
#define SAV_MAXSIZE 240000
#define SAV_SAVEVERS  5
#define SAV_WTNSVERS  2   /* Witness version */
#define SAV_HOUSEVERS 5

/* defines for flags for item saving/restoring */

#define O_F_WORN        1       /* item was equipped when saved */
#define O_F_CONTAINS    2       /* item was a container that had something in it */
#define O_F_UNIQUE      4       /* item was 'non-stock' in some way */
#define O_F_COUNT       8       /* more than 1 item of this type */
#define O_F_EOL        16       /* marks end of a 'contents' list */
#define O_F_AFFECTS    32       /* object had a decay event when saved */
#define O_F_SPELLBOOK  64       /* has spellbook array */

/* defines for O_F_UNIQUE saved items */

#define O_U_KEYS        BIT_1
#define O_U_DESC1       BIT_2
#define O_U_DESC2       BIT_3
#define O_U_DESC3       BIT_4
#define O_U_VAL0        BIT_5
#define O_U_VAL1        BIT_6
#define O_U_VAL2        BIT_7
#define O_U_VAL3        BIT_8
#define O_U_TYPE        BIT_9
#define O_U_WEAR        BIT_10
#define O_U_EXTRA       BIT_11
#define O_U_WEIGHT      BIT_12
#define O_U_COST        BIT_13
#define O_U_BV1         BIT_14
#define O_U_BV2         BIT_15
#define O_U_AFFS        BIT_16
#define O_U_TRAP        BIT_17
#define O_U_COND        BIT_18
#define O_U_ANTI        BIT_19
#define O_U_EXTRA2      BIT_20
#define O_U_TIMER       BIT_21
#define O_U_ANTI2       BIT_22
#define O_U_EDESC       BIT_23
#define O_U_MATERIAL    BIT_24
#define O_U_SPACE       BIT_25
#define O_U_VAL4        BIT_26
#define O_U_VAL5        BIT_27
#define O_U_VAL6        BIT_28
#define O_U_VAL7        BIT_29
#define O_U_BV3         BIT_30
#define O_U_BV4         BIT_31
#define O_U_BV5         BIT_32

/* on what was pfile saved */

#define RENT_CRASH              1
#define RENT_QUIT               2
#define RENT_INN                3
#define RENT_DEATH              4
#define RENT_LINKDEAD           5
#define RENT_CAMPED             6
#define RENT_CRASH2             7
#define RENT_POOFARTI           8
#define RENT_SWAPARTI           9
#define RENT_FIGHTARTI         10
/*
 * these macros are a very good idea, I salute whoever thought of it.
 * -JAB
 */

#define ADD_BYTE(buf, b) { *(char *)buf = b; buf++; }

#define ADD_SHORT(buf, s) { ush_int tmp_ = ((sh_int) (s)); \
                              bcopy(&tmp_, buf, short_size); \
                              buf += short_size; }

#define ADD_INT(buf, i) { uint tmp_ = ((unsigned) (i)); \
                            bcopy(&tmp_, buf, int_size); buf += int_size; }

#define ADD_LONG(buf, l) { ulong tmp_ = ((unsigned) (l)); \
                             bcopy(&tmp_, buf, long_size); \
                             buf += long_size; }

#define ADD_STRING(buf, s) { ush_int l_; \
                               if (s) { l_ = (ush_int)strlen(s); \
                                          ADD_SHORT(buf, l_); \
                                          bcopy(s, buf, (int)l_); buf += l_; \
                               } else { ADD_SHORT(buf, 0); } }

#define ADD_BYTE(buf, b) { *(char *)buf = b; buf++; }

#define GET_BYTE(buf) (*(char *)((buf)++))
#define GET_SHORT(buf) getShort(&buf)
#define GET_INTE(buf) getInt(&buf)
#if 0
#   define GET_INTE(bf)  (ntohl(*((int*)bf)++))
#endif
#define GET_LONG(buf) getLong(&buf)
#define GET_STRING(buf) getString(&buf)
  
#endif /* _SOJ_PFILES_H_ */
