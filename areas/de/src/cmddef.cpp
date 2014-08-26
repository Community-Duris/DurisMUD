//
//  File: cmddef.cpp    originally part of durisEdit
//
//  Usage: contains various lists of user commands
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


#include "command/command.h"

#include "command/maincomm.h"
#include "command/misccomm.h"
#include "command/setcomm.h"


// commands recognized by main prompt

command g_mainCommands[] =
{
  { "LOOK", COMM_LOOK },
  { "TELL", COMM_TELL },

  { "WEST", COMM_WEST },
  { "EAST", COMM_EAST },
  { "NORTH", COMM_NORTH },
  { "SOUTH", COMM_SOUTH },
  { "UP", COMM_UP },
  { "DOWN", COMM_DOWN },
  { "NORTHWEST", COMM_NORTHWEST },
  { "SOUTHWEST", COMM_SOUTHWEST },
  { "NORTHEAST", COMM_NORTHEAST },
  { "SOUTHEAST", COMM_SOUTHEAST },
  { "NW", COMM_NORTHWEST },
  { "SW", COMM_SOUTHWEST },
  { "NE", COMM_NORTHEAST },
  { "SE", COMM_SOUTHEAST },

  { "ALIAS", COMM_ALIAS },
  { "UNALIAS", COMM_UNALIAS },
  { "SET", COMM_SETVAR },
  { "UNSET", COMM_UNSETVAR },
  { "SETTEMPLATE", COMM_SETTEMPLATE },
  { "TOGGLE", COMM_TOGGLE },
  { "LIMIT", COMM_LIMIT },

  { "STAT", COMM_STAT },
  { "EXITS", COMM_EXITS },
  { "LOOKUP", COMM_LOOKUP },
  { "WHERE", COMM_WHERE },
  { "MASSSET", COMM_MASSSET },
  { "WHICH", COMM_WHICH },
  { "GOTO", COMM_GOTO },
  { "AT", COMM_AT },
  { "EDIT", COMM_EDIT },
  { "LIST", COMM_LIST },
  { "CREATE", COMM_CREATE },
  { "CLONE", COMM_CLONE },
  { "COPY", COMM_COPY },
  { "COMMANDS", COMM_COMMANDS },

  { "ZONEEXITS", COMM_ZONEEXITS },

  { "CREATEEDIT", COMM_CREATEEDIT },
  { "CE", COMM_CREATEEDIT },

  { "LOAD", COMM_LOAD },

  { "PURGE", COMM_PURGE },
  { "DELETE", COMM_DELETE },

  { "RANDOM", COMM_SETRAND },
  { "SETRANDOM", COMM_SETRAND },

  { "KEY", COMM_KEY },
  { "HEIGHT", COMM_SCREENHEIGHT },
  { "WIDTH", COMM_SCREENWIDTH },

  { "INVENTORY", COMM_INVENTORY },

  { "GET", COMM_GET },
  { "GETC", COMM_GETC },

  { "DROP", COMM_DROP },
  { "DROPC", COMM_DROPC },

  { "PUT", COMM_PUT },
  { "PUTC", COMM_PUTC },

  { "GIVE", COMM_PUT },
  { "GIVEC", COMM_PUTC },

  { "EQUIP", COMM_EQUIP },
  { "UNEQUIP", COMM_UNEQUIP },

  { "MOUNT", COMM_MOUNT },
  { "UNMOUNT", COMM_UNMOUNT },

  { "FOLLOW", COMM_FOLLOW },
  { "UNFOLLOW", COMM_UNFOLLOW },

  { "SAVE", COMM_SAVE },
  { "WRITE", COMM_SAVE },

  { "QUIT", COMM_QUIT },
  { "EXIT", COMM_QUIT },

  { "DUMPTEXT", COMM_DUMPTEXT },
  { "DUMPTEXTCOLOR", COMM_DUMPTEXTCOL },
  { "DTC", COMM_DUMPTEXTCOL },

  { "CHECK", COMM_CHECK },
  { "CHECKMAP", COMM_CHECKMAP },

  { "CONFIG", COMM_CONFIG },
  { "SETUP", COMM_CONFIG },
  { "DISPLAYCONFIG", COMM_EDITDISPLAY },
  { "CHECKCONFIG", COMM_EDITCHECK },

  { "DISPLAYQUEST", COMM_TELL },
  { "DISPQUEST", COMM_TELL },

  { "SWAPEXITS", COMM_SWAPEXITS },
  { "SWAPNS", COMM_SWAPEXITSNS },
  { "SWAPWE", COMM_SWAPEXITSWE },
  { "SWAPUD", COMM_SWAPEXITSUD },

  { "RENUMBER", COMM_RENUMBER },
  { "RENUMBERROOMS", COMM_RENUMBROOM },
  { "RENUMBEROBJECTS", COMM_RENUMBOBJECT },
  { "RENUMBERMOBS", COMM_RENUMBMOB },
  { "RENROOMS", COMM_RENUMBROOM },
  { "RENOBJECTS", COMM_RENUMBOBJECT },
  { "RENMOBS", COMM_RENUMBMOB },

  { "SETZONENUMB", COMM_SETZONENUMB },
  { "SZN", COMM_SETZONENUMB },

  { "FIXFLAGS", COMM_FIXFLAGS },
  { "FIXCOND", COMM_FIXCOND },
  { "FIXGUILDSTUFF", COMM_FIXGUILD },
  { "GUILD", COMM_GUILD },

  { "RECSIZE", COMM_RECSIZE },
  { "RS", COMM_RECSIZE },
  { "VNUMINFO", COMM_VNUMINFO },

  { "HELP", COMM_LONGHELP },
  { "?", COMM_SHORTHELP },

  { "VERSION", COMM_VERSION },

  { "GRID", COMM_GRID },
  { "LINKROOMS", COMM_LINKROOMS },

  { "MAXVNUM", COMM_MAXVNUM },

#ifdef GODMODE
  { "HUNT", COMM_HUNT },
#endif

  { 0 }
};


// arguments to 'lookup' command

command g_lookupCommands[] =
{
  { "ROOM", LOOKUPCMD_ROOM },
  { "OBJECT", LOOKUPCMD_OBJECT },
  { "MOB", LOOKUPCMD_MOB },
  { "CHAR", LOOKUPCMD_MOB },
  { 0 }
};


// arguments to 'create' command

command g_createCommands[] =
{
  { "ROOM", CREATECMD_ROOM },
  { "OBJECT", CREATECMD_OBJECT },
  { "OBJHERE", CREATECMD_OBJHERE },
  { "OH", CREATECMD_OBJHERE },
  { "MOB", CREATECMD_MOB },
  { "CHAR", CREATECMD_MOB },
  { "MOBHERE", CREATECMD_MOBHERE },
  { "MH", CREATECMD_MOBHERE },
  { "EXIT", CREATECMD_EXIT },
  { "ALIAS", CREATECMD_ALIAS },
  { "VARIABLE", CREATECMD_VARIABLE },
  { 0 }
};


// arguments to 'edit' command

command g_editCommands[] =
{
  { "ROOM", EDITCMD_ROOM },
  { "OBJECT", EDITCMD_OBJECT },
  { "MOB", EDITCMD_MOB },
  { "CHAR", EDITCMD_MOB },
  { "QUEST", EDITCMD_QUEST },
  { "SHOP", EDITCMD_SHOP },
  { "EXIT", EDITCMD_EXIT },
  { "ZONE", EDITCMD_ZONE },
  { "DESC", EDITCMD_DESC },
  { "DEFROOM", EDITCMD_DEFROOM },
  { "DEFOBJ", EDITCMD_DEFOBJ },
  { "DEFMOB", EDITCMD_DEFMOB },
  { "DEFEXIT", EDITCMD_DEFEXIT },
  { "DEFEXTRADESC", EDITCMD_DEFEDESC },
  { "DEFEDESC", EDITCMD_DEFEDESC },
  { "DEFQUEST", EDITCMD_DEFQUEST },
  { "DEFSHOP", EDITCMD_DEFSHOP },
  { "SECTOR", EDITCMD_SECTOR },
  { 0 }
};


// arguments to 'createedit' command

command g_createEditCommands[] =
{
  { "ROOM", CRTEDITCMD_ROOM },
  { "OBJECT", CRTEDITCMD_OBJECT },
  { "MOB", CRTEDITCMD_MOB },
  { "CHAR", CRTEDITCMD_MOB },
  { "EXIT", CRTEDITCMD_EXIT },
  { 0 }
};


// arguments to 'load' command

command g_loadCommands[] =
{
  { "OBJECT", LOADCMD_OBJECT },
  { "MOB", LOADCMD_MOB },
  { "CHAR", LOADCMD_MOB },
  { 0 }
};


// arguments to 'list' command

command g_listCommands[] =
{
  { "ROOM", LISTCMD_ROOM },
  { "EXIT", LISTCMD_EXIT },
  { "OBJECT", LISTCMD_OBJECT },
  { "MOB", LISTCMD_MOB },
  { "CHAR", LISTCMD_MOB },
  { "QUEST", LISTCMD_QUEST },
  { "SHOP", LISTCMD_SHOP },
  { "OBJHERE", LISTCMD_OBJHERE },
  { "MOBHERE", LISTCMD_MOBHERE },
  { "OH", LISTCMD_OBJHERE },
  { "MH", LISTCMD_MOBHERE },
  { "LOADED", LISTCMD_LOADED },
  { "LIMIT", LISTCMD_LOADED },
  { "LOOKUP", LISTCMD_LOOKUP },
  { "ALIAS", LISTCMD_ALIAS },
  { "VARIABLE", LISTCMD_VARIABLE },
  { "BOOLEAN", LISTCMD_BOOLEAN },
  { "TEMPLATE", LISTCMD_TEMPLATE },
  { "COMMANDS", LISTCMD_COMMANDS },
  { 0 }
};


// arguments to 'delete' command

command g_deleteCommands[] =
{
  { "ROOM", DELETECMD_ROOM },
  { "OBJECT", DELETECMD_OBJECT },
  { "MOB", DELETECMD_MOB },
  { "CHAR", DELETECMD_MOB },
  { "QUEST", DELETECMD_QUEST },
  { "SHOP", DELETECMD_SHOP },
  { "EXIT", DELETECMD_EXIT },
  { "UNUSED", DELETECMD_UNUSED },

  { "DEFROOM", DELETECMD_DEFROOM },
  { "DEFOBJECT", DELETECMD_DEFOBJECT },
  { "DEFMOB", DELETECMD_DEFMOB },
  { "DEFEXIT", DELETECMD_DEFEXIT },
  { "DEFEXTRADESC", DELETECMD_DEFEDESC },
  { "DEFEDESC", DELETECMD_DEFEDESC },
  { "DEFQUEST", DELETECMD_DEFQUEST },
  { "DEFSHOP", DELETECMD_DEFSHOP },

  { "OBJHERE", DELETECMD_OBJHERE },
  { "OH", DELETECMD_OBJHERE },
  { "MOBHERE", DELETECMD_MOBHERE },
  { "MH", DELETECMD_MOBHERE },

  { "ALIAS", DELETECMD_ALIAS },
  { "VARIABLE", DELETECMD_VARIABLE },

  { 0 }
};


// arguments to 'purge' command

command g_purgeCommands[] =
{
  { "ALL", PURGECMD_ALL },
  { "ALLMOB", PURGECMD_ALLMOB },
  { "ALLOBJECT", PURGECMD_ALLOBJ },
  { "INVENTORY", PURGECMD_INV },
  { 0 }
};


// arguments to 'stat' command

command g_statCommands[] =
{
  { "ROOM", STATCMD_ROOM },
  { "OBJECT", STATCMD_OBJECT },
  { "MOB", STATCMD_MOB },
  { "CHAR", STATCMD_MOB },
  { "ZONE", STATCMD_ZONE },
  { 0 }
};


// arguments to 'limit' command

command g_limitCommands[] =
{
  { "OBJECT", SETLIMCMD_OBJECT },
  { "MOB", SETLIMCMD_MOB },
  { "CHAR", SETLIMCMD_MOB },
  { 0 }
};


// arguments to 'clone' command

command g_cloneCommands[] =
{
  { "ROOM", CLONECMD_ROOM },
  { "OBJECT", CLONECMD_OBJECT },
  { "MOB", CLONECMD_MOB },
  { "CHAR", CLONECMD_MOB },
  { 0 }
};


// arguments to 'copy' command

command g_copyCommands[] =
{
  { "DEFAULT", COPYCMD_DEFAULT },
  { "DESC", COPYCMD_DESC },
  { 0 }
};


// arguments to 'copy desc' command

command g_copyDescCommands[] =
{
  { "ROOM", COPYDESCCMD_ROOM },
  { "MOB", COPYDESCCMD_MOB },
  { "CHAR", COPYDESCCMD_MOB },
  { 0 }
};


// arguments to 'copy default' command

command g_copyDefaultCommands[] =
{
  { "ROOM", COPYDEFCMD_ROOM },
  { "EXIT", COPYDEFCMD_EXIT },
  { "OBJECT", COPYDEFCMD_OBJECT },
  { "MOB", COPYDEFCMD_MOB },
  { "CHAR", COPYDEFCMD_MOB },
  { "EXTRADESC", COPYDEFCMD_EDESC },
  { "EDESC", COPYDEFCMD_EDESC },
  { "QUEST", COPYDEFCMD_QUEST },
  { "SHOP", COPYDEFCMD_SHOP },
  { 0 }
};


// valid .set file commands

command g_setCommands[] =
{
  { "SET", SETCMD_SET },
  { "ALIAS", SETCMD_ALIAS },
  { "LIMIT", SETCMD_LIMIT },
  { "RANDOM", SETCMD_RANDOM },
  { "SETTEMPLATE", SETCMD_SETTEMP },
  { 0 }
};
