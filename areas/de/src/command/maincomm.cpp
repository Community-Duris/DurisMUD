//
//  File: maincomm.cpp   originally part of durisEdit
//
//  Usage: function called once command and arguments are known -
//         calls appropriate function
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


#include <stdlib.h>
#include <string.h>

#include "../fh.h"
#include "../types.h"
#include "../vardef.h"

#include "maincomm.h"
#include "../system.h"

extern variable *g_varHead;
extern command g_mainCommands[];
extern alias *g_aliasHead;

//
// commandQuit : returns true if user wants to quit
//

bool commandQuit(void)
{
  if (confirmChanges())
  {
    _outtext("\n\nQuitting...\n");

    return true;
  }

  return false;
}


//
// mainExecCommand
//

bool mainExecCommand(const usint command, const char *inargs)
{
  char strn[1024], args[1024];


  strncpy(args, inargs, 1023);
  args[1023] = '\0';

  switch (command)
  {
    case COMM_QUIT        : return commandQuit();

    case COMM_EXITS       : displayRoomExitInfo(); break;
    case COMM_SWAPEXITS   : swapExits(args); break;
    case COMM_SWAPEXITSNS : swapExitsNorthSouth(); break;
    case COMM_SWAPEXITSWE : swapExitsWestEast(); break;
    case COMM_SWAPEXITSUD : swapExitsUpDown(); break;

    case COMM_LOOKUP      : lookup(args); break;
    case COMM_WHICH       : which(args); break;
    case COMM_MASSSET     : massSet(args); break;
    case COMM_WHERE       : where(args); break;
    case COMM_AT          : at(args); break;

    case COMM_NORTH       :
    case COMM_SOUTH       :
    case COMM_WEST        :
    case COMM_EAST        :
    case COMM_UP          :
    case COMM_DOWN        :
    case COMM_NORTHWEST   :
    case COMM_SOUTHWEST   :
    case COMM_NORTHEAST   : // command IDs in same order as exit dir IDs
    case COMM_SOUTHEAST   : goDirection(command - COMM_NORTH); break;

    case COMM_GOTO        : gotoRoomStrn(args); break;
    case COMM_LOOK        : look(args); break;
    case COMM_TELL        : tell(args); break;

    case COMM_ZONEEXITS   : displayZoneExits(args); break;

    case COMM_CREATEEDIT  : createEdit(args); break;
    case COMM_EDIT        : editEntity(args); break;

    case COMM_PURGE       : purge(args); break;

    case COMM_LIST        : list(args); break;
    case COMM_COMMANDS    : displayCommands(g_mainCommands); break;
    case COMM_LOAD        : loadEntity(args); break;
    case COMM_CREATE      : createEntity(args); break;
    case COMM_DELETE      : deleteEntity(args); break;
    case COMM_STAT        : statEntity(args); break;
    case COMM_CLONE       : cloneEntity(args); break;
    case COMM_COPY        : copyCommand(args); break;
    case COMM_SETRAND     : setEntityRandomVal(args); break;

    case COMM_KEY         : showKeyUsed(args); break;
    case COMM_GUILD       : toggleGuildVar(); break;
    case COMM_SCREENHEIGHT: if (!varExists(g_varHead, VAR_SCREENHEIGHT_NAME) && !args[0])
                            {
                              sprintf(strn,
"\nScreen height var is not set - default is %u.\n\n", DEF_SCREENHEIGHT_VAL);
                              _outtext(strn);
                            }
                            else
                            {
                              sprintf(strn, "%s %s", VAR_SCREENHEIGHT_NAME, args);
                              varCmd(strn, true, &g_varHead);
                            }
                            break;
    case COMM_SCREENWIDTH : if (!varExists(g_varHead, VAR_SCREENWIDTH_NAME) && !args[0])
                            {
                              sprintf(strn,
"\nScreen width var is not set - default is %u.\n\n", DEF_SCREENWIDTH_VAL);
                              _outtext(strn);
                            }
                            else
                            {
                              sprintf(strn, "%s %s", VAR_SCREENWIDTH_NAME, args);
                              varCmd(strn, true, &g_varHead);
                            }
                            break;

    case COMM_INVENTORY   : showInventory(); break;

    case COMM_GET         : _outtext("\n");
                            takeEntityCmd(args, true);
                            _outtext("\n");
                            break;
    case COMM_GETC        : _outtext("\n");
                            takeEntityCmd(args, false);
                            _outtext("\n");
                            break;

    case COMM_DROP        : _outtext("\n");
                            dropEntityCmd(args, true);
                            _outtext("\n");
                            break;
    case COMM_DROPC       : _outtext("\n");
                            dropEntityCmd(args, false);
                            _outtext("\n");
                            break;

    case COMM_PUT         : _outtext("\n");
                            putEntityCmd(args, true);
                            _outtext("\n");
                            break;
    case COMM_PUTC        : _outtext("\n");
                            putEntityCmd(args, false);
                            _outtext("\n");
                            break;

    case COMM_EQUIP       : equipMob(args); break;
    case COMM_UNEQUIP     : unequipMob(args); break;

    case COMM_MOUNT       : mountMob(args); break;
    case COMM_UNMOUNT     : unmountMob(args); break;

    case COMM_FOLLOW      : followMob(args); break;
    case COMM_UNFOLLOW    : unfollowMob(args); break;

    case COMM_CONFIG      : editConfig(); break;
    case COMM_EDITDISPLAY : editDisplay(); break;

    case COMM_SHORTHELP   : _outtext("\n");
                            dumpTextFile(DE_SHORTHELP, true);
                            _outtext("\n");
                            break;
    case COMM_LONGHELP    : _outtext("\n");
                            dumpTextFile(DE_MAINHELP, true); 
                            _outtext("\n");
                            break;

    case COMM_RENUMBER    : renumberAll(args); break;
    case COMM_RENUMBROOM  : renumberRoomsUser(args); break;
    case COMM_RENUMBOBJECT: renumberObjectsUser(args); break;
    case COMM_RENUMBMOB   : renumberMobsUser(args); break;
    case COMM_SETZONENUMB : setZoneNumbStrn(args); break;

    case COMM_FIXFLAGS    : fixAllFlags(); break;
    case COMM_FIXCOND     : resetObjCond(); break;
    case COMM_FIXGUILD    : fixGuildStuff(); break;

    case COMM_RECSIZE     : displayRecordSizeInfo(); break;
    case COMM_VNUMINFO    : displayVnumInfo(); break;

    case COMM_CHECK       : checkAll(); break;
    case COMM_EDITCHECK   : editCheck(); break;
    case COMM_SAVE        : writeFiles(); break;

    case COMM_DUMPTEXT    : _outtext("\n");
                            dumpTextFile(args, false);
                            _outtext("\n");
                            break;

    case COMM_DUMPTEXTCOL : _outtext("\n");
                            dumpTextFile(args, true);
                            _outtext("\n");
                            break;

    case COMM_VERSION     : displayVersionInfo(); break;

    case COMM_ALIAS       : aliasCmd(args, true, &g_aliasHead); break;
    case COMM_UNALIAS     : unaliasCmd(args, &g_aliasHead); break;
    case COMM_SETVAR      : varCmd(args, true, &g_varHead); break;
    case COMM_UNSETVAR    : unvarCmd(args, &g_varHead); break;
    case COMM_TOGGLE      : toggleVar(&g_varHead, args); break;
    case COMM_LIMIT       : setLimitArgs(args); break;

    case COMM_SETTEMPLATE : setTemplateArgs(args, true, true); break;

    case COMM_GRID        : createGridInterp(args); break;
    case COMM_LINKROOMS   : linkRoomsInterp(args); break;
    case COMM_CHECKMAP    : checkMapTrueness(); break;

    case COMM_MAXVNUM     : changeMaxVnumUser(args); break;
#ifdef GODMODE
    case COMM_HUNT        : huntForOPEquipment(args); break;
#endif
  }

  return false;
}
