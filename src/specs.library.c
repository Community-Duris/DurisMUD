/*
 ***************************************************************************
 *  File: specs.object.c                                     Part of Duris *
 *  Usage: special procedures for objects                                    *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "assocs.h"
#include "damage.h"
#include "graph.h"
#include "justice.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "weather.h"

/*
   external variables
 */
extern P_char      character_list;
extern P_desc      descriptor_list;
extern P_index     mob_index;
extern P_index     obj_index;
extern P_room      world;
extern char       *coin_names[];
extern char       *command[];
extern const char *dirs[];
extern const char *race_types[];
// extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int                    innate_abilities[];
extern int                    planes_room_num[];
extern int                    top_of_zone_table;
extern struct command_info    cmd_info[MAX_CMD_LIST];
extern struct dex_app_type    dex_app[52];
extern struct time_info_data  time_info;
extern struct zone_data      *zone;
extern struct zone_data      *zone_table;

P_nevent get_scheduled(P_obj obj, event_func func);

#define PROCLIB_PARAM_DELIM '&'

// some kind of 'find next token' function
#define ISQUOTE(a) ((((a) == '\'') || ((a) == '"')) ? a : 0)

// util function similar to one_argument, but will parse a string surrounded with single
// or double quotes as a single argument.
char *proclib_getNext_string(char *source, char *nextString)
{
	if (!nextString)
	{
		return source;
	}
	nextString[0] = '\0';
	if (!source)
		return NULL;

	char *p1 = source;
	while (*p1 && isspace(*p1))
		p1++;

	char quote = ISQUOTE(*p1);
	if (!quote)
	{
		// just return the next word
		return one_argument(source, nextString);
	}
	// find the matching quote or EOL
	int nIdx = 0;

	while (*(++p1))
	{
		if (quote == *p1)
		{
			p1++;
			break;
		}
		nextString[nIdx++] = *p1;
	}
	nextString[nIdx++] = '\0';
	return p1;
}

// actual proc for 'hummer'
int proclibobj_hummer(P_obj obj, P_char ch, int cmd, char *argument)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (cmd)
		return FALSE;

	hummer(obj);
	return TRUE;
}

// param parser for actroom proc
char *proclibobj_parse_actroom(char *argument)
{
	char  arg[MAX_STRING_LENGTH], params[MAX_STRING_LENGTH];
	int   chance = 0;
	char *pRet   = NULL;

	argument = proclib_getNext_string(argument, arg);
	if (arg[0])
	{
		chance = atoi(arg);
		if (chance)
		{
			argument = proclib_getNext_string(argument, arg);
			if (arg[0])
			{
				if (!strstr(arg, "%p") && !strstr(arg, "%q"))
					return NULL;

				while (strchr(arg, '%'))
					*(strchr(arg, '%')) = '$';

				snprintf(params, MAX_STRING_LENGTH, "%d\xFF%s", chance, arg);
				CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
				strcpy(pRet, params);
				return pRet;
			}
		}
	}
	return NULL;
}

// actual proc for actroom
int proclibobj_actroom(P_obj obj, P_char ch, int cmd, char *params)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd || !OBJ_ROOM(obj))
		return FALSE;

	char *pAct;
	long  chance = strtol(params, &pAct, 10);

	if (chance && number(0, chance - 1))
		return FALSE;

	pAct = strchr(pAct, 0xFF);

	if (!chance || !pAct || !(*pAct))
	{
		char buf[500];
		snprintf(buf, 500, "Malformed _proclib_actroom description on object %d", obj_index[obj->R_num].virtual_number);
		debug(buf);
		wizlog(58, buf);
		return FALSE;
	}
	pAct++;
	// send the 'act' to the room
	act(pAct, TRUE, NULL, obj, 0, TO_ROOM);
	return TRUE;
}

// param parser for actworn
char *proclibobj_parse_actworn(char *argument)
{
	char  arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH], params[MAX_STRING_LENGTH];
	int   chance = 0;
	char *pRet   = NULL;

	argument = proclib_getNext_string(argument, arg1);
	if (arg1[0])
	{
		chance = atoi(arg1);
		if (chance)
		{
			argument = proclib_getNext_string(argument, arg1);
			if (arg1[0])
			{
				if (!strstr(arg1, "%p") && !strstr(arg1, "%q"))
					return NULL;
				if (!strstr(arg1, "%n"))
					return NULL;

				while (strchr(arg1, '%'))
					*(strchr(arg1, '%')) = '$';

				argument = proclib_getNext_string(argument, arg2);
				if (arg2[0])
				{
					if (!strstr(arg2, "%p") && !strstr(arg2, "%q"))
						return NULL;

					while (strchr(arg2, '%'))
						*(strchr(arg2, '%')) = '$';

					snprintf(params, MAX_STRING_LENGTH, "%d\xFF%s\xFF%s", chance, arg1, arg2);
					CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
					strcpy(pRet, params);
					return pRet;
				}
			}
		}
	}
	return NULL;
}

// actual proc for actworn
int proclibobj_actworn(P_obj obj, P_char ch, int cmd, char *params)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (cmd || !OBJ_WORN(obj))
		return FALSE;
	ch = obj->loc.wearing;

	char *pAct;
	long  chance = strtol(params, &pAct, 10);
	char  buf[500]; // need a buffer to hold the first (to room) string

	if (chance && number(0, chance - 1))
		return FALSE;

	pAct = strchr(pAct, 0xFF);
	if (pAct)
	{
		pAct++;
		// parse out that first string *somehow* into buf
		char *delim = strchr(pAct, 0xFF);
		if (chance && delim && ((delim - pAct) < 500))
		{

			*delim = '\0';     // temp make the delim be EOS
			strcpy(buf, pAct); // copy it
			*delim = 0xFF;     // put delim bac
			// note: the above method is faster then using strncpy and then manually adding a null

			pAct = delim + 1;
			while (*pAct && isspace(*pAct))
				pAct++;

			// we are all set!
			if (pAct && (*pAct))
			{
				act(buf, TRUE, ch, obj, NULL, TO_ROOM);
				act(pAct, TRUE, ch, obj, 0, TO_CHAR);
				return TRUE;
			}
		}
	}
	snprintf(buf, 500, "Malformed _proclib_actworn description on object %d", obj_index[obj->R_num].virtual_number);
	debug(buf);
	wizlog(58, buf);
	return FALSE;
}

// default parameter parser
char *proclibobj_parse_default(char *)
{
	char *pRet;
	CREATE(pRet, char, 2, MEM_TAG_EXDESCD);
	*pRet       = ' ';
	*(pRet + 1) = '\0';
	return pRet;
}

struct ObjProcLib
{
	int (*func)(P_obj, P_char, int, char *);
	char *(*parse_params)(char *arguments);
	const char procName[20];
	const char procDesc[100];
	const char procHelp[300];
} object_proc_libs[] = {
	{proclibobj_actroom,
     proclibobj_parse_actroom, "actroom",
     "Object 'acts' to the room when on the ground.",     "        Params: chance act_room\n"
     "          chance: Act will have 1 in 'chance' odds of occurring apprx every 5 seconds.\n"
     "          act_room: Actor string sent to room which must contain %p or %q."                                                                },
	{proclibobj_actworn,
     proclibobj_parse_actworn, "actworn",
     "Object 'acts' while being worn/equiped by a character.", "        Params: chance act_room act_wearer\n"
 "          chance: Act will have 1 in 'chance' odds of occurring apprx every 5 seconds.\n"
 "          act_room: Actor string sent to room which must contain %p or %q, AND %n.\n"
 "          act_wearer: Actor string sent to room which must contain %p or %q.\n"                                                       },
	{ proclibobj_hummer, proclibobj_parse_default,  "hummer", "Items 'hums' - similar to some artifact weapons.",                                                             "        No parameters."},
};

int proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument);

// event func - just redirects to generic proclib_obj_proc (which is in obj proc format)
void proclib_obj_event(P_char, P_char, P_obj obj, void *) { proclib_obj_proc(obj, NULL, 0, NULL); }

// the "hub" for all proclib object procs.  This function dispatches proclibs
// for all objects
int proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!obj)
		return FALSE;

	int bRet = FALSE, bResetPeriodic = FALSE;

	struct extra_descr_data *ed = obj->ex_description;
	while (ed)
	{
		if (ed->keyword && ed->description)
		{
			if (!strn_cmp(ed->keyword, "_proclib_", 9))
			{
				for (int i = 0; i < (sizeof(object_proc_libs) / sizeof(ObjProcLib)); i++)
				{
					if (!strn_cmp(ed->keyword + 9, object_proc_libs[i].procName, strlen(object_proc_libs[i].procName)) && object_proc_libs[i].func)
					{
						if (!cmd)
						{
							if (object_proc_libs[i].func(obj, ch, cmd, ed->description))
							{
								bRet = bResetPeriodic = TRUE;
								break;
							}
							else if (!bResetPeriodic && object_proc_libs[i].func(obj, NULL, -10, NULL))
								bResetPeriodic = TRUE;
						}
						else if (object_proc_libs[i].func(obj, ch, cmd, argument))
							return TRUE; // no need to break the for loop - this wasn't a periodic
							             // call, so don't need to reset it.
					}
				}
			}
		}
		ed = ed->next;
	}
	if (bResetPeriodic)
		add_event(proclib_obj_event, PULSE_MOBILE + number(-4, 4), NULL, NULL, obj, 0, NULL, 0);
	return bRet;
}

// sends simple 'usage' info for object procs
void proclibUsage(P_char ch)
{
	if (ch)
		send_to_char("Usage: proclib <mob|obj|room> <target> <add|del> <procname> [proc params]\n", ch);
}

// generic function for adding a proclib to an object.  This is NOT an interactive
// function, but is designed to be called from both the read_object() interface, as well
// as the in-mud proclib command.
//
// 0 - success.  <0 is a procname issue,  >0 is a params issue (and the retVal is the idx+1
int proclibObj_add(P_obj obj, char *procName, char *args)
{
	int libIdx = -1;
	for (libIdx = (sizeof(object_proc_libs) / sizeof(ObjProcLib)) - 1; libIdx >= 0; libIdx--)
	{
		if (!strn_cmp(procName, object_proc_libs[libIdx].procName, strlen(object_proc_libs[libIdx].procName)) && object_proc_libs[libIdx].func)
			break;
	}
	if (-1 == libIdx)
		return -1;

	char *params = object_proc_libs[libIdx].parse_params(args);
	if (!params)
		return (libIdx + 1);

	// find a suffix to use...
	int                      suffix = 0;
	struct extra_descr_data *ed     = obj->ex_description;
	while (ed)
	{
		if (ed->keyword)
		{
			if (!strn_cmp(ed->keyword, "_proclib_", 9) && !strn_cmp(ed->keyword + 9, object_proc_libs[libIdx].procName, strlen(object_proc_libs[libIdx].procName)))
			{
				int tempSuff = atoi(ed->keyword + (9 + strlen(object_proc_libs[libIdx].procName)));
				if (tempSuff > suffix)
					suffix = tempSuff;
			}
		}
		ed = ed->next;
	}
	char keyword[50];
	snprintf(keyword, 50, "_proclib_%s%d", object_proc_libs[libIdx].procName, suffix + 1);

	CREATE(ed, struct extra_descr_data, 1, MEM_TAG_EXDESCD);
	ed->next            = obj->ex_description;
	obj->ex_description = ed;
	CREATE(ed->keyword, char, strlen(keyword) + 1, MEM_TAG_EXDESCD);
	strcpy(ed->keyword, keyword);
	ed->description = params;
	obj->str_mask |= STRUNG_EDESC;
	SET_BIT(obj->extra_flags, ITEM_PROCLIB);

	if ((NULL == get_scheduled(obj, proclib_obj_event)) && object_proc_libs[libIdx].func(obj, NULL, CMD_SET_PERIODIC, NULL))
	{
		add_event(proclib_obj_event, PULSE_MOBILE + number(-4, 4), NULL, NULL, obj, 0, NULL, 0);
	}

	return 0;
}

// object specific version of proclib cmd processing
void do_proclibObj(P_char ch, char *argument)
{
	// parameters should be: target_obj add|del procname [proc_params]

	bool bAdd = false; // false means that its a delete

	char argBuf[MAX_STRING_LENGTH];

	wizlog(GET_LEVEL(ch), "%s: proclibObj %s", GET_NAME(ch), argument);
	logit(LOG_WIZ, "%s: proclibObj %s", GET_NAME(ch), argument);

	// parse object
	argument = one_argument(argument, argBuf);
	// find the object referenced by argBuf
	P_obj obj;

	if (!(obj = get_obj_in_list_vis(ch, argBuf, ch->carrying)))
	{
		if (!(obj = get_obj_in_list_vis(ch, argBuf, world[ch->in_room].contents)))
		{
			if (ch)
				send_to_char("Unable to find the specified object\n", ch);
			return;
		}
	}
	// parse cmd: add|delete
	argument = one_argument(argument, argBuf);
	if (is_abbrev(argBuf, "add"))
		bAdd = true;
	else if (!is_abbrev(argBuf, "delete"))
	{
		proclibUsage(ch);
		return;
	}

	if (!bAdd)
	{
		send_to_char("deleting of proclibs isn't *yet* supported\n", ch);
		return;
	}
	// parse out the proc name
	argument = one_argument(argument, argBuf);

	int addRet = proclibObj_add(obj, argBuf, argument);
	if (addRet < 0)
	{ // error with proc name
		proclibUsage(ch);
		send_to_char("Available proclibs for objects: \n", ch);
		for (int libIdx = (sizeof(object_proc_libs) / sizeof(ObjProcLib)) - 1; libIdx >= 0; libIdx--)
		{
			send_to_char(object_proc_libs[libIdx].procName, ch);
			send_to_char(" - ", ch);
			send_to_char(object_proc_libs[libIdx].procDesc, ch);
			send_to_char("\n", ch);
		}
		send_to_char("\n", ch);
		return;
	}
	if (addRet)
	{
		send_to_char("Error in proc parameters.\n", ch);
		send_to_char(object_proc_libs[addRet - 1].procHelp, ch);
		send_to_char("\n", ch);
		return;
	}
	send_to_char("Proc successfully added.\n", ch);
}

// mobile specific version of proclib cmd processing
void do_proclibMob(P_char ch, char *argument) { send_to_char("Mob proclibs not yet supported\n", ch); }

// room specific version of proclib cmd processing
void do_proclibRoom(P_char ch, char *argument) { send_to_char("Room proclibs not yet supported\n", ch); }

// main proclib cmd (in mud) processer
void do_proclib(P_char ch, char *argument, int cmd)
{
	char type[MAX_STRING_LENGTH];

	argument = one_argument(argument, type);
	if (!*type)
		proclibUsage(ch);
	else if (is_abbrev(type, "object"))
	{
		do_proclibObj(ch, argument);
	}
	else if (is_abbrev(type, "character") || is_abbrev(type, "mobile"))
	{
		do_proclibMob(ch, argument);
	}
	else if (is_abbrev(type, "room"))
	{
		do_proclibRoom(ch, argument);
	}
	else
	{
		proclibUsage(ch);
	}
}
