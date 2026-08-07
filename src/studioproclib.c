/*
   ***************************************************************************
   *  File: studioproclib.c                                   Part of Duris *
   *  Usage: the 'sayresponse' and 'transporter' object proclibs            *
   ***************************************************************************
   *
   * Ported in behaviour from a 2020-era uncommitted patch that carried
   * both procs inside specs.library.c, with three corrections:
   *
   *  1. sprintf() -> snprintf() on both parameter builders.  The originals
   *     wrote two MAX_STRING_LENGTH inputs into one fixed buffer; the
   *     build runs -D_FORTIFY_SOURCE=0, so nothing would have caught it.
   *  2. proclibobj_transporter called char_light()/room_light() after
   *     char_to_room(); char_to_room() already does the light bookkeeping,
   *     and the second call on a NOWHERE char is a crash.  The redundant
   *     calls are gone.
   *  3. The original reached these procs by patching special() in
   *     interp.c to walk every ground object on every command.  Instead
   *     proclibObj_add() installs proclib_obj_cmd_bridge() as the
   *     prototype proc of any vnum that gains a proclib, so the engine's
   *     OWN dispatch (interp.c:2229, "special in object present?") does
   *     the work and interp.c is not touched.  It is also cheaper: only
   *     vnums that actually carry a proclib are ever consulted.
   */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "utils.h"
#include "utility.h"
#include "studioproclib.h"

extern P_index obj_index;
extern P_room  world;

/* defined in specs.library.c */
extern char *proclib_getNext_string(char *source, char *nextString);
extern int   proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument);

/* ------------------------------------------------------------------ */
/* sayresponse: 'keywords' 'reply text'                                */
/* ------------------------------------------------------------------ */

char *proclibobj_parse_sayresponse(char *argument)
{
	char  arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char  params[MAX_STRING_LENGTH * 2 + 2];
	char *pRet = NULL;

	argument = proclib_getNext_string(argument, arg1);
	if (arg1[0])
	{
		argument = proclib_getNext_string(argument, arg2);
		if (arg2[0])
		{
			snprintf(params, sizeof(params), "%s\xFF%s", arg1, arg2);
			CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
			strcpy(pRet, params);
			return pRet;
		}
	}
	return NULL;
}

/* Object in the room (or held by the speaker) replies when a player says
   any of the keywords.  Reached from studioproc_speech() with CMD_SAY,
   AFTER the say text has landed, so the reply reads as a reply. */
int proclibobj_sayresponse(P_obj obj, P_char ch, int cmd, char *argument)
{
	struct extra_descr_data *ed;
	char                     low[MAX_STRING_LENGTH];
	int                      room = -1, li, replied = FALSE;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;                        /* command-driven only */
	if (cmd != CMD_SAY || !obj || !ch || !argument || !*argument)
		return FALSE;

	if (OBJ_ROOM(obj))
		room = obj->loc.room;
	else if (OBJ_CARRIED(obj) && obj->loc.carrying)
		room = obj->loc.carrying->in_room;
	else if (OBJ_WORN(obj) && obj->loc.wearing)
		room = obj->loc.wearing->in_room;
	if (room < 0 || room != ch->in_room)
		return FALSE;

	for (li = 0; argument[li] && li < (int)sizeof(low) - 1; li++)
		low[li] = LOWER(argument[li]);
	low[li] = '\0';

	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		char       *delim;
		char        kw[MAX_INPUT_LENGTH];
		const char *p;
		int         hit = FALSE, k;
		char        buf[MAX_STRING_LENGTH];

		if (!ed->keyword || !ed->description || strn_cmp(ed->keyword, "_proclib_sayresponse", 20))
			continue;

		delim = strchr(ed->description, '\xFF');
		if (!delim || !*(delim + 1))
			continue;

		p = ed->description;
		while (p < delim && !hit)
		{
			while (p < delim && *p == ' ')
				p++;
			for (k = 0; p < delim && *p != ' ' && k < (int)sizeof(kw) - 1; p++, k++)
				kw[k] = LOWER(*p);
			kw[k] = '\0';
			if (k && strstr(low, kw))
				hit = TRUE;
		}
		if (!hit)
			continue;

		snprintf(buf, sizeof(buf) - 3, "%s replies, '%s'", obj->short_description ? obj->short_description : "something", delim + 1);
		CAP(buf);
		strcat(buf, "\r\n");
		send_to_room(buf, room);
		replied = TRUE;
	}
	return replied;
}

/* ------------------------------------------------------------------ */
/* transporter: keyword roomvnum                                       */
/* ------------------------------------------------------------------ */

char *proclibobj_parse_transporter(char *argument)
{
	char  arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char  params[MAX_STRING_LENGTH + 16];
	char *pRet = NULL;

	argument = proclib_getNext_string(argument, arg1);
	if (arg1[0] && !is_number(arg1))
	{
		argument = proclib_getNext_string(argument, arg2);
		if (arg2[0] && is_number(arg2) && atoi(arg2) > 0)
		{
			snprintf(params, sizeof(params), "%s\xFF%d", arg1, atoi(arg2));
			CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
			strcpy(pRet, params);
			return pRet;
		}
	}
	return NULL;
}

/* 'enter <keyword>' teleports the actor to the configured room, which is
   validated at fire time (an area can be renumbered under our feet). */
int proclibobj_transporter(P_obj obj, P_char ch, int cmd, char *argument)
{
	struct extra_descr_data *ed;
	char                     word[MAX_INPUT_LENGTH];
	int                      was_in;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;                        /* command-driven only */
	if (cmd != CMD_ENTER || !obj || !ch || !argument)
		return FALSE;

	/* the transporter must be on the ground in the actor's room */
	if (!OBJ_ROOM(obj) || obj->loc.room != ch->in_room)
		return FALSE;

	one_argument(argument, word);
	if (!*word)
		return FALSE;

	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		char *delim;
		char  kw[MAX_INPUT_LENGTH];
		int   klen, rnum;

		if (!ed->keyword || !ed->description || strn_cmp(ed->keyword, "_proclib_transporter", 20))
			continue;

		delim = strchr(ed->description, '\xFF');
		if (!delim || !*(delim + 1))
			continue;

		klen = (int)(delim - ed->description);
		if (klen <= 0 || klen >= (int)sizeof(kw))
			continue;
		strncpy(kw, ed->description, klen);
		kw[klen] = '\0';
		if (str_cmp(word, kw))
			continue;

		rnum = real_room(atoi(delim + 1));
		if (rnum < 0)
		{
			logit(LOG_STATUS, "proclib transporter: obj %d keyword '%s' leads to missing room %d", obj_index[obj->R_num].virtual_number, kw, atoi(delim + 1));
			send_to_char("It doesn't seem to lead anywhere.\r\n", ch);
			return TRUE;
		}
		if (rnum == ch->in_room)
		{
			send_to_char("You are already there.\r\n", ch);
			return TRUE;
		}

		act("$n steps into $p and vanishes.", TRUE, ch, obj, 0, TO_ROOM);
		act("You step into $p...", FALSE, ch, obj, 0, TO_CHAR);
		was_in = ch->in_room;
		char_from_room(ch);
		/* char_to_room() is bool and returns TRUE on SUCCESS
		   (handler.c:1039), so only a successful arrival gets the message
		   and the look. */
		if (char_to_room(ch, rnum, -1))
		{
			act("$n arrives in a swirl of mist.", TRUE, ch, 0, 0, TO_ROOM);
			if (IS_PC(ch))
			{
				char empty[2];

				empty[0] = '\0';
				do_look(ch, empty, CMD_LOOK);
			}
		}
		else if (char_in_list(ch) && IS_ALIVE(ch) && ch->in_room == NOWHERE)
		{
			/* Refused BEFORE placement, so step the character back out.
			   char_in_list() comes FIRST and is the only thing allowed to
			   touch a pointer char_to_room() has just returned FALSE for:
			   it walks character_list comparing POINTERS and never
			   dereferences its argument (utility.c:551) -- it is the same
			   liveness test char_to_room() itself uses to ask whether a room
			   proc extracted someone (handler.c:1317).  A FALSE from it means
			   the character was killed or extracted and freed, and the two
			   state reads that follow would be a use-after-free: the defect
			   xander-l caught in SP_A_GOTO, whose bad state test was modelled
			   on this very line.
			   Only past that gate is the STATE question legitimate, and it
			   still has to be asked: handler.c returns FALSE at three points
			   above its `ch->in_room = room' and TRUE/FALSE at many points
			   below, so a FALSE with ch still at NOWHERE was refused before
			   insertion and must be put back, while a FALSE from below leaves
			   ch in the destination, where re-inserting would be a duplicate. */
			char_to_room(ch, was_in, -1);
			send_to_char("Something bars the way, and you step back out.\r\n", ch);
		}
		return TRUE;
	}
	return FALSE;
}

/* ------------------------------------------------------------------ */
/* prototype bridge                                                    */
/* ------------------------------------------------------------------ */

/* Installed on the vnum of any object that gains a proclib, so that
   interp.c's existing "special in object present?" walk delivers real
   commands to instance proclibs.  Deliberately silent for:
     cmd <= 0   - CMD_PERIODIC / CMD_SET_PERIODIC etc.  Periodic ticking
                  is already owned by proclib_obj_event (scheduled inside
                  proclibObj_add); returning TRUE here would make db.c
                  schedule a SECOND periodic event and double-tick.
     CMD_SAY    - speech is delivered from studioproc_speech() after the
                  say text has landed, so replies read as replies and the
                  player's say is never swallowed. */
/* THE DISPLACED-PROC CHAIN.

   A vnum can already own an object proc - a hand-written one, or
   studioproc_obj from the world.trg engine.  Refusing to install the
   bridge in that case (the original behaviour) kept the existing proc
   safe but left every instance proclib on that vnum unreachable: a
   transporter added at runtime to such a vnum would never see CMD_ENTER,
   silently.  Refusing is not the only way to be safe, though - we can
   install the bridge AND keep the displaced proc, calling it first.

   Order is deliberate and matches the rule world.trg already documents:
   the hand-written C proc runs first and a TRUE return means the data
   never runs.  So existing content cannot change behaviour; a proclib
   only sees commands the incumbent declined.

   Sized by distinct vnums that gain a proclib, which is small (boot-time
   _proclib_ edescs plus whatever an immortal adds), and only ever grown.
   Main game thread only, like the rest of this file. */
struct proclib_chain_ent
{
	int rnum;
	int (*prev)(P_obj, P_char, int, char *);
};
static struct proclib_chain_ent *proclib_chain     = NULL;
static int                       proclib_chain_top = 0;
static int                       proclib_chain_cap = 0;

void proclib_chain_install(int rnum, int (*prev)(P_obj, P_char, int, char *))
{
	int i;

	if (rnum < 0 || !prev || prev == proclib_obj_cmd_bridge)
		return;
	for (i = 0; i < proclib_chain_top; i++)
		if (proclib_chain[i].rnum == rnum)
			return;                       /* already chained - never double-wrap */
	if (proclib_chain_top == proclib_chain_cap)
	{
		int newcap = proclib_chain_cap ? proclib_chain_cap * 2 : 32;
		struct proclib_chain_ent *grown =
		    (struct proclib_chain_ent *)realloc(proclib_chain, newcap * sizeof(*grown));

		if (!grown)
			return;                       /* out of memory: leave the incumbent alone */
		proclib_chain     = grown;
		proclib_chain_cap = newcap;
	}
	proclib_chain[proclib_chain_top].rnum = rnum;
	proclib_chain[proclib_chain_top].prev = prev;
	proclib_chain_top++;
}

static int (*proclib_chain_prev(int rnum))(P_obj, P_char, int, char *)
{
	int i;

	for (i = 0; i < proclib_chain_top; i++)
		if (proclib_chain[i].rnum == rnum)
			return proclib_chain[i].prev;
	return NULL;
}

int proclib_obj_cmd_bridge(P_obj obj, P_char ch, int cmd, char *argument)
{
	int (*prev)(P_obj, P_char, int, char *);

	if (!obj)
		return FALSE;

	/* the displaced proc keeps its original semantics, including the cmd
	   values this bridge itself ignores, so chaining cannot regress it */
	if (obj->R_num >= 0 && (prev = proclib_chain_prev(obj->R_num)) != NULL)
	{
		if (prev(obj, ch, cmd, argument))
			return TRUE;
	}

	if (cmd <= 0 || cmd == CMD_SAY)
		return FALSE;
	if (!IS_SET(obj->extra_flags, ITEM_PROCLIB))
		return FALSE;
	return proclib_obj_proc(obj, ch, cmd, argument);
}
