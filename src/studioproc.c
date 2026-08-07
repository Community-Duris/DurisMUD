/*
   ***************************************************************************
   *  File: studioproc.c                                      Part of Duris *
   *  Usage: data-driven procs for mobs, objects and rooms (world.trg)      *
   ***************************************************************************
   *
   * WHAT THIS IS
   * ------------
   * One generic C proc per target type (mob / object / room), bound at boot
   * to every vnum named in areas/world.trg.  Behaviour is DATA.  The engine's
   * existing dispatch does all the work:
   *
   *   special()            interp.c:1808   every command a character types
   *   CMD_TOROOM           handler.c:1513  someone entered the mob's room
   *   CMD_FROMROOM         handler.c:981   someone is leaving the room
   *   CMD_GOTHIT           fight.c:10003   the mob was struck in melee
   *   CMD_GOTNUKED         fight.c:4508    the mob was struck by a spell
   *   CMD_MOB_COMBAT       mobact.c:5743   one combat round (needs ACT_SPEC)
   *   CMD_SET_PERIODIC     db.c:2713/3038  instance created; ask for a tick
   *   CMD_PERIODIC         mobact.c:10205  the periodic tick
   *   CMD_DEATH            fight.c:2566    the mob died (ACT_SPEC_DIE)
   *   CMD_FOUND / CMD_DECAY                object search / decay
   *
   * so only FOUR one-line hooks are added to existing files, for the
   * four things the engine genuinely never dispatches: boot, speech, give
   * and on-kill.  See howto_trg.txt for the builder-facing walkthrough.
   *
   * FILE FORMAT (areas/world.trg, framing mirrors world.qst)
   * --------------------------------------------------------
   *   #<vnum> <M|O|R>          target prototype: Mob / Obj / Room vnum
   *   T <EVENT> [args]         one trigger; many T blocks per record
   *   [chance <1-100>]         optional
   *   [if [!]<cond> ...]       0..8 condition lines, ALL must hold
   *   <action lines...>        one action per line
   *   ~                        ends the trigger
   *   S                        ends the record
   *   #~                       ends the file
   *
   * '*' starts a comment.  A missing file is fine.  Every parse error logs
   * zone+vnum+line and SKIPS the record -- a bad .trg must never stop a boot.
   *
   * EVENTS
   *   DEATH                     self died
   *   KILL                      self killed someone
   *   SPEECH <kw ...>           a PC said something containing a keyword
   *   GIVE <objvnum|ANY>        self (mob) was handed that object
   *   ENTER                     a char entered self's room
   *   LEAVE                     a char is leaving self's room (rooms)
   *   PULSE <n>                 every n mob pulses (jittered)
   *   FIGHT                     one combat round
   *   HPBELOW <pct>             hp first fell below pct (latched per instance)
   *   DAMAGED [melee|spell|any] self was struck
   *   CMD <verb> [kw ...]       a char typed <verb> here.  Covers look, open,
   *                             get, wear, remove, enter, push, pull, ... -
   *                             every verb the engine has, without new C.
   *   HOUR <0-23|ANY>           the game hour changed
   *   REPOP                     this instance was just created
   *   SEARCH                    an object was found by 'search'
   *   DECAY                     an object's decay timer expired
   *
   * CONDITIONS ('if' lines; prefix any with '!' to negate)
   *   carrying <ovnum>   wearing <ovnum>   class <name>   race <code>
   *   level <op> <n>     align <op> <n>    sex <m|f|n>    hour <op> <n>
   *   hp <op> <pct>      groupsize <op> <n>  room <vnum>  zone <n>
   *   affect <spell|tag> counter <name> <op> <n>   pcs <op> <n>
   *   mobs <mvnum> <op> <n>   chance <1-100>   ispc   fighting
   *   Each takes an optional trailing "on self|actor" (default: actor,
   *   falling back to self when there is no actor).
   *   <op> is one of = != < <= > >=
   *
   * ACTIONS  ($n = actor, $m = self, $$ = literal $)
   *   say / emote / echo / zecho <text>
   *   cast '<spell name>' [actor|self]
   *   mload <mvnum>   oload <ovnum>   give <ovnum>
   *   transfer <roomvnum>   goto <roomvnum>
   *   damage <NdS+B>        heal <amount|full>     purge [self|actor]
   *   attack <NdS+B> [type <t>] [scope one|room|group|nottank]
   *          [save <para|rod|fear|breath|spell> [half]] [cd <secs>]
   *          [hit "<msg>"] [miss "<msg>"] [room "<msg>"]
   *   affect <spell|tag> <ticks> [on self|actor] [save <type>]
   *          [apply <APPLY name|number> <modifier>] [aff <AFF name>]
   *          -- with neither apply nor aff the affect is an inert MARKER:
   *             'if affect <name>' reads it and 'affects' shows it, but it
   *             changes no number.  'apply AC -20' / 'aff BLIND' make it real.
   *   unaffect <spell|tag> [on self|actor]
   *   do [self|actor] <command line>
   *   set <counter> <n>     add <counter> <n>
   *   oneof <n>             -- the next n actions become a pool; one runs
   *   exit <roomvnum> <dir> [to <roomvnum>|none] [state open|closed|locked|secret]
   *   block                 -- swallow the command that triggered this
   *
   * SAFETY
   *   - max 24 actions and 8 conditions per trigger; max 32 triggers/record
   *   - a trigger never fires re-entrantly (per-trigger latch) and the whole
   *     engine has a depth ceiling of SP_MAX_DEPTH, so 'do' cannot recurse
   *     without bound
   *   - actions on a dead / extracted target are skipped
   *   - 'purge' must be the LAST action, and is refused for DEATH/HPBELOW/
   *     DAMAGED (self would be freed inside die()/raw_damage())
   *   - 'do' refuses any command whose cmd_info[].minimum_level is non-zero,
   *     i.e. everything an immortal-only or grantable command could reach
   *   - 'damage'/'attack' cap a single firing at 80% of current hp and pass
   *     RAWDAM_NOKILL, so a trigger can wound but never quietly execute
   *
   * THREADING
   *   Everything below runs on the main game thread.  studioproc_boot()
   *   latches pthread_self(); every dispatch entry refuses to run on any
   *   other thread.  No locks are needed and none are taken.
   */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "damage.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "spells.h"
#include "utils.h"
#include "utility.h"
#include "studioproc.h"

/* ------------------------------------------------------------------ */
/* engine globals we lean on                                          */
/* ------------------------------------------------------------------ */

extern P_room               world;
extern int                  top_of_world;
extern P_index              mob_index;
extern P_index              obj_index;
extern const char          *spells[];
extern Skill                skills[];
extern struct command_info  cmd_info[MAX_CMD_LIST];
extern const char          *command[];
extern const char          *dirs[];
extern struct time_info_data time_info;
extern const struct race_names  race_names_table[];
extern const struct class_names class_names_table[];
extern const char           *apply_types[];       /* common.c:811  */
extern flagDef               affected1_bits[];    /* common.c:630  */
extern flagDef               affected2_bits[];    /* common.c:668  */
extern flagDef               affected3_bits[];    /* common.c:704  */
extern flagDef               affected4_bits[];    /* common.c:740  */
extern flagDef               affected5_bits[];    /* common.c:776  */

/* declared in specs.library.c; no header carries it */
extern int proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument);

/* forward declarations */
static void sp_hour_event(P_char ch, P_char victim, P_obj obj, void *data);
static int  sp_last_hour = -1;

/* ------------------------------------------------------------------ */
/* data model                                                         */
/* ------------------------------------------------------------------ */

struct sp_cond
{
	int   kind;                 /* SP_C_xxx                            */
	int   neg;
	int   op;                   /* SP_OP_xxx                           */
	int   num;
	int   num2;
	int   who;                  /* SP_WHO_ACTOR / SP_WHO_SELF          */
	int   slot;                 /* interned counter slot               */
	char *text;
};

struct sp_action
{
	int   op;                   /* SP_A_xxx                            */
	char *text;                 /* message / command line / hit msg    */
	char *text2;                /* attack: miss message                */
	char *text3;                /* attack: room message                */
	int   num;                  /* vnum / spell / heal / counter value */
	int   num2;                 /* exit: destination vnum, -1 = leave  */
	int   dnum, dsize, dbonus;  /* NdS+B                               */
	int   who;                  /* SP_WHO_xxx                          */
	int   scope;                /* SP_SCOPE_xxx                        */
	int   dtype;                /* SPLDAM_xxx, 0 = untyped             */
	int   save;                 /* SAVING_xxx, -1 = none               */
	int   savehalf;
	int   cooldown;             /* seconds                             */
	int   slot;                 /* counter slot / exit direction       */
	int   count;                /* oneof: pool size                    */
	int   dur;                  /* affect duration, ticks              */
	int   state;                /* exit state                          */
	int   apply;                /* affect: APPLY_xxx, 0 = APPLY_NONE   */
	int   amod;                 /* affect: the modifier for that apply */
	int   affword;              /* affect: AFF word 1..5, 0 = no bit   */
	unsigned long affbit;       /* affect: the bit in that word        */
};

struct sp_trig
{
	int   event;
	int   chance;
	int   arg;                  /* GIVE vnum / PULSE n / HPBELOW pct / HOUR */
	int   arg2;                 /* DAMAGED mode: 0 any, 1 melee, 2 spell    */
	char *keywords;             /* SPEECH keywords / CMD keyword filter     */
	int   cmdnum;               /* CMD: resolved command index, -1 = any    */
	int   trig_index;
	int   running;
	int   num_conds;
	int   num_actions;
	struct sp_cond   conds[SP_MAX_CONDS];
	struct sp_action actions[SP_MAX_ACTIONS];
	struct sp_trig  *next;
};

struct sp_rec
{
	int   target;
	int   vnum;
	int   num_trigs;
	unsigned int events;        /* bitmask of SP_EV_xxx present        */
	struct sp_trig *trigs;
	/* the C proc this vnum already had, if any: never clobbered */
	mob_proc_type   prev_mob;
	obj_proc_type   prev_obj;
	room_proc_type  prev_room;
	struct sp_rec  *next;       /* hash chain                          */
};

#define SP_HASH 128
static struct sp_rec *sp_tab[SP_NUM_T][SP_HASH];
static int            sp_tcount[SP_NUM_T];
int                   studioproc_count = 0;

/* interned counter names -> ubyte slot (affect->location) */
static char *sp_cname[SP_MAX_COUNTERS];
static int   sp_ncounters = 0;

/* per-room counters: rooms are permanent, so a flat array is correct
   and needs no lifetime management at all.  Indexed
   [room * SP_ROOM_SLOTS + slot]. */
static int *sp_roomctr = NULL;

static pthread_t sp_main_thread;
static int       sp_have_thread = 0;
static int       sp_depth       = 0;

static struct sp_rec *sp_find(int targ, int vnum)
{
	struct sp_rec *r;

	if (targ < 0 || targ >= SP_NUM_T || !sp_tcount[targ])
		return NULL;
	for (r = sp_tab[targ][((unsigned int)vnum) & (SP_HASH - 1)]; r; r = r->next)
		if (r->vnum == vnum)
			return r;
	return NULL;
}

static int sp_on_game_thread(void)
{
	if (!sp_have_thread)
		return 1;
	return pthread_equal(pthread_self(), sp_main_thread) ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* per-instance state                                                 */
/*                                                                    */
/* Mobs: ordinary affects flagged AFFTYPE_STORE ("used to store data   */
/* only", structs.h:125).  Lifetime is the mob's, for free.           */
/* Objects: an instance extra-description, which is exactly how the    */
/* proclib mechanism already stores per-object parameters.            */
/* Rooms: a flat array; rooms never go away.                          */
/* ------------------------------------------------------------------ */

static int sp_intern_counter(const char *name)
{
	int i;

	if (!name || !*name)
		return -1;
	for (i = 0; i < sp_ncounters; i++)
		if (!str_cmp(sp_cname[i], name))
			return i;
	if (sp_ncounters >= SP_MAX_COUNTERS)
		return -1;
	sp_cname[sp_ncounters] = str_dup(name);
	return sp_ncounters++;
}

static struct affected_type *sp_find_store(P_char ch, int type, int slot)
{
	struct affected_type *af;

	if (!ch)
		return NULL;
	for (af = ch->affected; af; af = af->next)
		if (af->type == type && (int)af->location == slot)
			return af;
	return NULL;
}

static int sp_char_get(P_char ch, int type, int slot)
{
	struct affected_type *af = sp_find_store(ch, type, slot);

	return af ? af->modifier : 0;
}

static void sp_char_set(P_char ch, int type, int slot, int value)
{
	struct affected_type *af = sp_find_store(ch, type, slot);
	struct affected_type  na;

	if (af)
	{
		af->modifier = value;
		return;
	}
	memset(&na, 0, sizeof(na));
	na.type      = type;
	na.duration  = -1;                       /* permanent: dies with the mob */
	na.modifier  = value;
	na.location  = (ubyte)slot;
	na.flags     = AFFTYPE_STORE | AFFTYPE_NOMSG | AFFTYPE_NOSAVE;
	affect_to_char(ch, &na);
}

/* object instance state: keyword "_sp_<slot>", body is the decimal value */
static void sp_obj_key(char *buf, size_t sz, int type, int slot)
{
	snprintf(buf, sz, "_sp_%c%d", (type == SP_TAG_COUNTER) ? 'c' : (type == SP_TAG_COOLDOWN) ? 'd' : 't', slot);
}

static int sp_obj_get(P_obj obj, int type, int slot)
{
	struct extra_descr_data *ed;
	char                     key[32];

	if (!obj)
		return 0;
	sp_obj_key(key, sizeof(key), type, slot);
	for (ed = obj->ex_description; ed; ed = ed->next)
		if (ed->keyword && !str_cmp(ed->keyword, key) && ed->description)
			return atoi(ed->description);
	return 0;
}

static void sp_obj_set(P_obj obj, int type, int slot, int value)
{
	struct extra_descr_data *ed;
	char                     key[32], val[32];

	if (!obj)
		return;
	sp_obj_key(key, sizeof(key), type, slot);
	snprintf(val, sizeof(val), "%d", value);

	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		if (ed->keyword && !str_cmp(ed->keyword, key))
		{
			if (ed->description)
				FREE(ed->description);
			CREATE(ed->description, char, strlen(val) + 1, MEM_TAG_EXDESCD);
			strcpy(ed->description, val);
			obj->str_mask |= STRUNG_EDESC;
			return;
		}
	}
	CREATE(ed, struct extra_descr_data, 1, MEM_TAG_EXDESCD);
	memset(ed, 0, sizeof(*ed));
	CREATE(ed->keyword, char, strlen(key) + 1, MEM_TAG_EXDESCD);
	strcpy(ed->keyword, key);
	CREATE(ed->description, char, strlen(val) + 1, MEM_TAG_EXDESCD);
	strcpy(ed->description, val);
	ed->next            = obj->ex_description;
	obj->ex_description = ed;
	obj->str_mask |= STRUNG_EDESC;
}

/* ------------------------------------------------------------------ */
/* execution context                                                  */
/* ------------------------------------------------------------------ */

struct sp_ctx
{
	int     targ;
	P_char  self_ch;
	P_obj   self_obj;
	int     self_room;
	P_char  actor;
	P_obj   given;              /* GIVE: the object handed over        */
	int     self_dead_ok;
	int     blocked;            /* a 'block' action ran                */
	struct sp_rec *rec;
};

#define SP_X_FIRED    1
#define SP_X_SELFGONE 2

/* generic per-instance accessors that work for whichever self we have */
static int sp_state_get(struct sp_ctx *cx, int type, int slot)
{
	switch (cx->targ)
	{
		case SP_T_MOB:
			return sp_char_get(cx->self_ch, type, slot);
		case SP_T_OBJ:
			return sp_obj_get(cx->self_obj, type, slot);
		default:
			if (type == SP_TAG_COUNTER && sp_roomctr && cx->self_room >= 0 && cx->self_room <= top_of_world && slot >= 0 && slot < SP_ROOM_SLOTS)
				return sp_roomctr[cx->self_room * SP_ROOM_SLOTS + slot];
			return 0;
	}
}

static void sp_state_set(struct sp_ctx *cx, int type, int slot, int value)
{
	switch (cx->targ)
	{
		case SP_T_MOB:
			sp_char_set(cx->self_ch, type, slot, value);
			break;
		case SP_T_OBJ:
			sp_obj_set(cx->self_obj, type, slot, value);
			break;
		default:
			if (type == SP_TAG_COUNTER && sp_roomctr && cx->self_room >= 0 && cx->self_room <= top_of_world && slot >= 0 && slot < SP_ROOM_SLOTS)
				sp_roomctr[cx->self_room * SP_ROOM_SLOTS + slot] = value;
			break;
	}
}

static int sp_room_of(struct sp_ctx *cx)
{
	switch (cx->targ)
	{
		case SP_T_MOB:
			return cx->self_ch ? cx->self_ch->in_room : -1;
		case SP_T_OBJ:
			if (!cx->self_obj)
				return -1;
			if (OBJ_ROOM(cx->self_obj))
				return cx->self_obj->loc.room;
			if (OBJ_CARRIED(cx->self_obj) && cx->self_obj->loc.carrying)
				return cx->self_obj->loc.carrying->in_room;
			if (OBJ_WORN(cx->self_obj) && cx->self_obj->loc.wearing)
				return cx->self_obj->loc.wearing->in_room;
			return -1;
		default:
			return cx->self_room;
	}
}

static const char *sp_self_name(struct sp_ctx *cx)
{
	switch (cx->targ)
	{
		case SP_T_MOB:
			if (cx->self_ch && cx->self_ch->player.short_descr)
				return cx->self_ch->player.short_descr;
			if (cx->self_ch && cx->self_ch->player.name)
				return cx->self_ch->player.name;
			return "someone";
		case SP_T_OBJ:
			if (cx->self_obj && cx->self_obj->short_description)
				return cx->self_obj->short_description;
			return "something";
		default:
			if (cx->self_room >= 0 && cx->self_room <= top_of_world && world[cx->self_room].name)
				return world[cx->self_room].name;
			return "the room";
	}
}

/* Bounded append.  Used instead of snprintf("%s...%s") where both parts
   can be MAX_STRING_LENGTH: that form is correct but makes gcc emit
   -Wformat-truncation, and this file must not add warnings to the
   baseline build. */
static void sp_cat(char *dst, int dstsz, const char *src)
{
	int d;

	if (!dst || dstsz < 1)
		return;
	d = (int)strlen(dst);
	while (src && *src && d < dstsz - 1)
		dst[d++] = *src++;
	dst[d] = '\0';
}

/* expand $n (actor), $m (self), $$ */
static void sp_expand(const char *src, struct sp_ctx *cx, char *out, int outsz)
{
	const char *ins;
	int         o = 0;

	outsz -= 1;
	while (*src && o < outsz)
	{
		if (*src == '$' && *(src + 1))
		{
			ins = NULL;
			if (*(src + 1) == 'n')
				ins = (cx->actor ? J_NAME(cx->actor) : "someone");
			else if (*(src + 1) == 'm')
				ins = sp_self_name(cx);
			else if (*(src + 1) == '$')
				ins = "$";
			if (ins)
			{
				while (*ins && o < outsz)
					out[o++] = *ins++;
				src += 2;
				continue;
			}
		}
		out[o++] = *src++;
	}
	out[o] = '\0';
}

/* ------------------------------------------------------------------ */
/* conditions                                                         */
/* ------------------------------------------------------------------ */

static int sp_cmp(int lhs, int op, int rhs)
{
	switch (op)
	{
		case SP_OP_EQ: return lhs == rhs;
		case SP_OP_NE: return lhs != rhs;
		case SP_OP_LT: return lhs < rhs;
		case SP_OP_LE: return lhs <= rhs;
		case SP_OP_GT: return lhs > rhs;
		default:       return lhs >= rhs;
	}
}

/* the NPC in the room with the lowest hp fraction, self included: the
   target a healer/buffer mob wants.  Named 'ally' in the grammar. */
static P_char sp_find_ally(struct sp_ctx *cx)
{
	int    room = sp_room_of(cx);
	P_char k, best = NULL;
	long   bestpct = 1000, pct;

	if (room < 0 || room > top_of_world)
		return NULL;
	for (k = world[room].people; k; k = k->next_in_room)
	{
		if (!IS_NPC(k) || !IS_ALIVE(k) || GET_MAX_HIT(k) <= 0)
			continue;
		if (cx->actor && k == cx->actor)
			continue;
		pct = (100L * GET_HIT(k)) / GET_MAX_HIT(k);
		if (pct < bestpct)
		{
			bestpct = pct;
			best    = k;
		}
	}
	return best;
}

/* Resolve a target selector.  For an OBJECT self there is no self_ch, so
   'self' and a missing actor both fall back to whoever is holding or
   wearing the object - that is what makes a cursed item able to drain or
   teleport "the holder" from a PULSE trigger, where no actor exists. */
static P_char sp_who_char(struct sp_ctx *cx, int who)
{
	P_char holder = NULL;

	if (cx->targ == SP_T_OBJ && cx->self_obj)
	{
		if (OBJ_CARRIED(cx->self_obj))
			holder = cx->self_obj->loc.carrying;
		else if (OBJ_WORN(cx->self_obj))
			holder = cx->self_obj->loc.wearing;
	}
	if (who == SP_WHO_ALLY)
		return sp_find_ally(cx);
	if (who == SP_WHO_SELF)
		return cx->self_ch ? cx->self_ch : holder;
	if (cx->actor)
		return cx->actor;
	return cx->self_ch ? cx->self_ch : holder;
}

static int sp_has_obj(P_char ch, int vnum, int worn)
{
	P_obj o;
	int   i;

	if (!ch)
		return FALSE;
	if (worn)
	{
		for (i = 0; i < MAX_WEAR; i++)
			if (ch->equipment[i] && ch->equipment[i]->R_num >= 0 && obj_index[ch->equipment[i]->R_num].virtual_number == vnum)
				return TRUE;
		return FALSE;
	}
	for (o = ch->carrying; o; o = o->next_content)
		if (o->R_num >= 0 && obj_index[o->R_num].virtual_number == vnum)
			return TRUE;
	return FALSE;
}

static int sp_group_size(P_char ch)
{
	P_char              k;
	struct follow_type *f;
	int                 n = 0;

	if (!ch)
		return 0;
	for (k = world[ch->in_room].people; k; k = k->next_in_room)
		if (k->group && ch->group && k->group == ch->group)
			n++;
	if (n)
		return n;
	n = 1;
	for (f = ch->followers; f; f = f->next)
		n++;
	return n;
}

static int sp_cond_true(struct sp_cond *c, struct sp_ctx *cx)
{
	P_char who  = sp_who_char(cx, c->who);
	int    room = sp_room_of(cx);
	int    val  = 0;
	P_char k;
	int    i;

	switch (c->kind)
	{
		case SP_C_CARRYING:
			val = sp_has_obj(who, c->num, FALSE);
			break;
		case SP_C_WEARING:
			val = sp_has_obj(who, c->num, TRUE);
			break;
		case SP_C_CLASS:
			val = (who && c->num) ? (GET_CLASS(who, (uint)c->num) ? 1 : 0) : 0;
			break;
		case SP_C_RACE:
			val = (who && GET_RACE(who) == c->num);
			break;
		case SP_C_LEVEL:
			return c->neg ^ (who && sp_cmp(GET_LEVEL(who), c->op, c->num));
		case SP_C_ALIGN:
			return c->neg ^ (who && sp_cmp(GET_ALIGNMENT(who), c->op, c->num));
		case SP_C_SEX:
			val = (who && GET_SEX(who) == c->num);
			break;
		case SP_C_HOUR:
			return c->neg ^ sp_cmp((int)time_info.hour, c->op, c->num);
		case SP_C_HP:
			if (!who || GET_MAX_HIT(who) <= 0)
				return c->neg ? 1 : 0;
			val = (int)((100L * GET_HIT(who)) / GET_MAX_HIT(who));
			return c->neg ^ sp_cmp(val, c->op, c->num);
		case SP_C_GROUP:
			return c->neg ^ sp_cmp(sp_group_size(who), c->op, c->num);
		case SP_C_ROOM:
			val = (room >= 0 && room <= top_of_world && world[room].number == c->num);
			break;
		case SP_C_ZONE:
			val = (room >= 0 && room <= top_of_world && world[room].zone == c->num);
			break;
		case SP_C_AFFECT:
			val = (who && affected_by_spell(who, c->num));
			break;
		case SP_C_COUNTER:
			return c->neg ^ sp_cmp(sp_state_get(cx, SP_TAG_COUNTER, c->slot), c->op, c->num);
		case SP_C_PCS:
			if (room >= 0 && room <= top_of_world)
				for (k = world[room].people; k; k = k->next_in_room)
					if (IS_PC(k) && !IS_TRUSTED(k))
						val++;
			return c->neg ^ sp_cmp(val, c->op, c->num);
		case SP_C_MOBS:
			/* c->who doubles as the search scope for this condition */
			if (c->who == SP_SC_WORLD)
			{
				i   = real_mobile(c->num);
				val = (i >= 0) ? mob_index[i].number : 0;
			}
			else if (c->who == SP_SC_ZONE && room >= 0 && room <= top_of_world)
			{
				int z = world[room].zone, r;

				for (r = 0; r <= top_of_world; r++)
				{
					if (world[r].zone != z)
						continue;
					for (k = world[r].people; k; k = k->next_in_room)
						if (IS_NPC(k) && IS_ALIVE(k) && GET_VNUM(k) == c->num)
							val++;
				}
			}
			else if (room >= 0 && room <= top_of_world)
			{
				for (k = world[room].people; k; k = k->next_in_room)
					if (IS_NPC(k) && IS_ALIVE(k) && GET_VNUM(k) == c->num)
						val++;
			}
			return c->neg ^ sp_cmp(val, c->op, c->num2);
		case SP_C_CHANCE:
			val = (number(1, 100) <= c->num);
			break;
		case SP_C_ISPC:
			val = (who && IS_PC(who));
			break;
		case SP_C_FIGHTING:
			val = (who && GET_OPPONENT(who) != NULL);
			break;
		default:
			val = 1;
			break;
	}
	return c->neg ? !val : val;
}

static int sp_conds_pass(struct sp_trig *t, struct sp_ctx *cx)
{
	int i;

	for (i = 0; i < t->num_conds; i++)
		if (!sp_cond_true(&t->conds[i], cx))
			return FALSE;
	return TRUE;
}

/* ------------------------------------------------------------------ */
/* the attack primitive                                               */
/*                                                                    */
/* Formula (readable without C):                                      */
/*   raw   = NdS + B                                                   */
/*   save  : if a save type is named and the victim makes it, the      */
/*           damage is halved ('half') or dropped entirely             */
/*   cap   = 80% of the victim's CURRENT hp, so one firing can wound   */
/*           but never quietly execute; RAWDAM_NOKILL is a second belt */
/*   typed : SPLDAM_* damage runs through spell_damage(), so every     */
/*           existing resistance / shield / globe check applies        */
/*   untyped: raw_damage(), physical, unresisted                       */
/* ------------------------------------------------------------------ */

static void sp_attack_one(struct sp_ctx *cx, struct sp_action *a, P_char vict)
{
	P_char self = cx->self_ch;
	struct damage_messages msg;
	char   buf[MAX_STRING_LENGTH];
	double dam;
	int    cap;

	if (!vict || !IS_ALIVE(vict) || vict->in_room < 0)
		return;
	if (IS_TRUSTED(vict))
		return;

	dam = (double)(dice(a->dnum, a->dsize) + a->dbonus);

	if (a->save >= 0 && saves_spell(vict, a->save))
	{
		if (!a->savehalf)
		{
			if (a->text2)
			{
				sp_expand(a->text2, cx, buf, sizeof(buf));
				act(buf, FALSE, vict, 0, self, TO_CHAR);
			}
			return;
		}
		dam /= 2.0;
	}

	cap = (GET_HIT(vict) * 4) / 5;
	if (cap <= 0)
		return;
	if (dam > (double)cap)
		dam = (double)cap;
	if (dam < 1.0)
		return;

	if (a->text)
	{
		sp_expand(a->text, cx, buf, sizeof(buf));
		act(buf, FALSE, vict, 0, self, TO_CHAR);
	}
	if (a->text3)
	{
		sp_expand(a->text3, cx, buf, sizeof(buf));
		act(buf, TRUE, vict, 0, self, TO_ROOM);
	}

	memset(&msg, 0, sizeof(msg));
	if (a->dtype)
		spell_damage(self ? self : vict, vict, dam, a->dtype, RAWDAM_NOKILL | RAWDAM_NOEXP | SPLDAM_SPELL, &msg);
	else
		raw_damage(self ? self : vict, vict, dam, RAWDAM_NOKILL | RAWDAM_NOEXP, &msg);
}

static void sp_do_attack(struct sp_ctx *cx, struct sp_action *a, int aidx)
{
	int    room = sp_room_of(cx);
	P_char k, next_k, tank = NULL;
	P_char prime = cx->actor;
	long   now;

	if (room < 0 || room > top_of_world)
		return;

	/* cooldown: a stamp in seconds, held per instance */
	if (a->cooldown > 0)
	{
		now = (long)time(0);
		if ((long)sp_state_get(cx, SP_TAG_COOLDOWN, aidx) > now)
			return;
		sp_state_set(cx, SP_TAG_COOLDOWN, aidx, (int)(now + a->cooldown));
	}

	if (cx->self_ch)
		tank = GET_OPPONENT(cx->self_ch);
	if (!prime)
		prime = tank;

	if (a->scope == SP_SCOPE_ONE)
	{
		sp_attack_one(cx, a, prime);
		return;
	}

	for (k = world[room].people; k; k = next_k)
	{
		next_k = k->next_in_room;
		if (k == cx->self_ch)
			continue;
		if (cx->self_ch && IS_NPC(k) && IS_NPC(cx->self_ch))
			continue;                     /* mobs do not aoe each other */
		if (a->scope == SP_SCOPE_NOTTANK && k == tank)
			continue;
		if (a->scope == SP_SCOPE_GROUP && prime && !(k == prime || (k->group && prime->group && k->group == prime->group)))
			continue;
		sp_attack_one(cx, a, k);
	}
}

/* ------------------------------------------------------------------ */
/* the 'do' primitive: run any engine command as a character          */
/*                                                                    */
/* This is what makes bash / trip / disarm / kick / backstab / rescue  */
/* / flee / open / unlock / wear / wield ... all reachable from data.  */
/* Adding "skill #7" needs no C: the verb is already in command[].     */
/* ------------------------------------------------------------------ */

static int sp_do_command(P_char ch, const char *line)
{
	char        verb[MAX_INPUT_LENGTH];
	char        rest[MAX_INPUT_LENGTH];
	const char *p;
	int         cmd;

	if (!ch || !IS_ALIVE(ch) || !line || !*line)
		return FALSE;

	p = one_argument(line, verb);
	while (*p == ' ')
		p++;
	strncpy(rest, p, sizeof(rest) - 1);
	rest[sizeof(rest) - 1] = '\0';
	if (!*verb)
		return FALSE;

	cmd = old_search_block(verb, 0, (uint)strlen(verb), command, 2) - 1;
	if (cmd < 0 || cmd >= MAX_CMD_LIST || !cmd_info[cmd].command_pointer)
		return FALSE;
	/* never let data reach a trusted command */
	if (cmd_info[cmd].minimum_level > 0 || cmd_info[cmd].grantable)
		return FALSE;
	/* minimum_position packs STAT_* bits above a POS_* in the low two
	   bits; GET_POS() is masked &3 and never exceeds POS_STANDING, so a
	   raw compare refuses every command.  Gate the way interp.c's own
	   command loop does: MIN_POS() checks each half against its mask. */
	if (!MIN_POS(ch, cmd_info[cmd].minimum_position))
		return FALSE;

	(*cmd_info[cmd].command_pointer)(ch, rest, cmd);
	return TRUE;
}

/* ------------------------------------------------------------------ */
/* execution                                                          */
/* ------------------------------------------------------------------ */

static int sp_execute(struct sp_trig *t, struct sp_ctx *cx)
{
	int    i, room, rr, dam, cap, pick, n;
	int    ret = SP_X_FIRED;
	P_char self, actor, targ, m, victim;
	P_obj  o;
	char   buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	struct damage_messages tmsg;
	struct affected_type   na;

	for (i = 0; i < t->num_actions; i++)
	{
		struct sp_action *a = &t->actions[i];

		self  = (cx->targ == SP_T_MOB) ? cx->self_ch : NULL;
		actor = cx->actor;
		room  = sp_room_of(cx);

		/* self validity: never touch a dead / extracted target */
		if (cx->targ == SP_T_MOB && (!self || (!cx->self_dead_ok && !IS_ALIVE(self)) || self->in_room < 0))
			break;
		if (cx->targ == SP_T_OBJ && !cx->self_obj)
			break;
		if (room < 0 || room > top_of_world)
			continue;

		/* 'oneof <n>': keep one of the next n actions, skip the rest */
		if (a->op == SP_A_ONEOF)
		{
			n = a->count;
			if (n < 1)
				continue;
			if (i + n >= t->num_actions)
				n = t->num_actions - i - 1;
			if (n < 1)
				continue;
			pick = number(1, n);
			/* run only the chosen one, then jump past the pool */
			{
				struct sp_action *b = &t->actions[i + pick];
				struct sp_trig    one;

				memset(&one, 0, sizeof(one));
				one.num_actions = 1;
				one.actions[0]  = *b;
				one.chance      = 100;
				if (sp_execute(&one, cx) & SP_X_SELFGONE)
					return ret | SP_X_SELFGONE;
			}
			i += n;
			continue;
		}

		/* actions that need a live actor */
		if ((a->op == SP_A_GIVE || a->op == SP_A_TRANSFER || a->op == SP_A_DAMAGE) && (!actor || !IS_ALIVE(actor) || actor->in_room < 0))
			continue;

		switch (a->op)
		{
			case SP_A_SAY:
				sp_expand(a->text, cx, buf, sizeof(buf));
				if (self)
					do_say(self, buf, 0);
				else if (cx->targ == SP_T_OBJ)
				{
					buf2[0] = '\0';
					sp_cat(buf2, sizeof(buf2), sp_self_name(cx));
					sp_cat(buf2, sizeof(buf2), " says '");
					sp_cat(buf2, sizeof(buf2), buf);
					sp_cat(buf2, sizeof(buf2), "'\r\n");
					CAP(buf2);
					send_to_room(buf2, room);
				}
				else
				{
					strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
					send_to_room(buf, room);
				}
				break;

			case SP_A_EMOTE:
				sp_expand(a->text, cx, buf, sizeof(buf));
				if (self)
					do_emote(self, buf, 0);
				else if (cx->targ == SP_T_OBJ)
				{
					buf2[0] = '\0';
					sp_cat(buf2, sizeof(buf2), sp_self_name(cx));
					sp_cat(buf2, sizeof(buf2), " ");
					sp_cat(buf2, sizeof(buf2), buf);
					sp_cat(buf2, sizeof(buf2), "\r\n");
					CAP(buf2);
					send_to_room(buf2, room);
				}
				else
				{
					strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
					send_to_room(buf, room);
				}
				break;

			case SP_A_ECHO:
				sp_expand(a->text, cx, buf, sizeof(buf));
				strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
				send_to_room(buf, room);
				break;

			case SP_A_ZECHO:
				sp_expand(a->text, cx, buf, sizeof(buf));
				strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
				send_to_zone(world[room].zone, buf);
				break;

			case SP_A_CAST:                 /* mob targets only (parse-enforced) */
				if (!self)
					break;
				targ = self;
				if (a->who == SP_WHO_ALLY)
				{
					targ = sp_find_ally(cx);
					if (!targ)
						targ = self;
				}
				else if (a->who != SP_WHO_SELF && actor && IS_ALIVE(actor) && actor->in_room == self->in_room)
					targ = actor;
				MobCastSpell(self, targ, NULL, a->num, 60);
				break;

			case SP_A_MLOAD:
				if (real_mobile(a->num) >= 0 && (m = read_mobile(a->num, VIRTUAL)) != NULL)
					char_to_room(m, room, -2);
				break;

			case SP_A_OLOAD:
				if (real_object(a->num) >= 0 && (o = read_object(a->num, VIRTUAL)) != NULL)
					obj_to_room(o, room);
				break;

			case SP_A_GIVE:
				if (real_object(a->num) >= 0 && (o = read_object(a->num, VIRTUAL)) != NULL)
				{
					obj_to_char(o, actor);
					act("$p materializes in your hands.", FALSE, actor, o, 0, TO_CHAR);
					act("$p materializes in $n's hands.", TRUE, actor, o, 0, TO_ROOM);
				}
				break;

			case SP_A_TRANSFER:
				rr = real_room(a->num);
				if (rr >= 0 && actor->in_room != rr)
				{
					act("$n vanishes in a swirl of mist.", TRUE, actor, 0, 0, TO_ROOM);
					char_from_room(actor);
					/* Same rule as SP_A_GOTO below, and the same two defects
					   in one line: the return was read INVERTED, so a
					   successful transfer got no arrival echo and no room
					   description and then nulled cx->actor for the rest of
					   the walk; and every use of actor sat on the FALSE side,
					   which is exactly where char_to_room() may already have
					   extracted and freed it.  The echo, the IS_PC() test and
					   the look all move to TRUE; FALSE only retires the
					   pointer from the context.
					   Retiring rather than returning is safe here because
					   every later action re-reads cx->actor and is already
					   NULL-guarded: the GIVE/TRANSFER/DAMAGE gate at the top
					   of the loop, SP_A_CAST's `actor &&' test, and the three
					   `victim' tests (AFFECT, UNAFFECT, DO). */
					if (char_to_room(actor, rr, -1))
					{
						act("$n arrives in a swirl of mist.", TRUE, actor, 0, 0, TO_ROOM);
						if (IS_PC(actor))
						{
							buf[0] = '\0';
							do_look(actor, buf, CMD_LOOK);
						}
					}
					else
						cx->actor = NULL;      /* freed or displaced: never touch again */
				}
				break;

			case SP_A_GOTO:                 /* mob targets only (parse-enforced) */
				rr = real_room(a->num);
				if (self && rr >= 0 && self->in_room != rr)
				{
					act("$n departs in a swirl of mist.", TRUE, self, 0, 0, TO_ROOM);
					char_from_room(self);
					/* char_to_room() is bool and returns TRUE on SUCCESS
					   (handler.c:1039).  BRANCH ON THE RETURN, never on
					   self's state: char_to_room() may invoke the destination
					   room proc, and it returns FALSE when that proc moves,
					   kills or extracts the character -- extract_char() then
					   frees it outright (handler.c:3512/3519), so IS_ALIVE(self)
					   or self->in_room would be reading freed memory.  When a
					   callee can free its argument the return value is the ONLY
					   safe signal; a state test is itself a dereference of the
					   thing it is trying to ask about.
					   FALSE therefore nulls self out of the context and returns
					   immediately -- the next iteration's own validity gate
					   (!IS_ALIVE(self) || self->in_room < 0) would dereference
					   it too. */
					if (char_to_room(self, rr, -1))
						act("$n arrives in a swirl of mist.", TRUE, self, 0, 0, TO_ROOM);
					else
					{
						cx->self_ch = NULL;
						return ret | SP_X_SELFGONE;
					}
				}
				break;

			case SP_A_DAMAGE:
				/* Formula: NdS+B, capped at 80% of the actor's CURRENT hp
				   per firing, floored by RAWDAM_NOKILL as a second belt. */
				if (IS_TRUSTED(actor))
					break;
				dam = dice(a->dnum, a->dsize) + a->dbonus;
				cap = (GET_HIT(actor) * 4) / 5;
				if (cap <= 0 || dam <= 0)
					break;
				if (dam > cap)
					dam = cap;
				act("&+rYou are wracked by unseen forces!&n", FALSE, actor, 0, 0, TO_CHAR);
				memset(&tmsg, 0, sizeof(tmsg));
				raw_damage(actor, actor, (double)dam, RAWDAM_NOKILL | RAWDAM_NOEXP, &tmsg);
				break;

			case SP_A_ATTACK:
				sp_do_attack(cx, a, i);
				break;

			case SP_A_AFFECT:
				victim = (a->who == SP_WHO_SELF) ? self : actor;
				if (!victim || !IS_ALIVE(victim))
					break;
				if (a->save >= 0 && saves_spell(victim, a->save))
					break;
				if (affected_by_spell(victim, a->num))
					break;
				memset(&na, 0, sizeof(na));
				na.type     = a->num;
				na.duration = a->dur;
				na.location = (ubyte)a->apply;      /* APPLY_NONE when absent  */
				na.modifier = a->amod;
				na.level    = (unsigned short)(self ? GET_LEVEL(self) : 30);
				/* 'aff <NAME>' sets one bit in one of the five AFF words.
				   affect_to_char (affects.c:1955) ORs these onto the char
				   unless AFFTYPE_NOAPPLY, which we never set here -- a
				   builder affect is meant to DO something, not just mark. */
				switch (a->affword)
				{
					case 1: na.bitvector  = a->affbit; break;
					case 2: na.bitvector2 = a->affbit; break;
					case 3: na.bitvector3 = a->affbit; break;
					case 4: na.bitvector4 = a->affbit; break;
					case 5: na.bitvector5 = a->affbit; break;
					default: break;
				}
				affect_to_char(victim, &na);
				break;

			case SP_A_UNAFFECT:
				victim = (a->who == SP_WHO_SELF) ? self : actor;
				if (victim && IS_ALIVE(victim))
					affect_from_char(victim, a->num);
				break;

			case SP_A_DO:
				victim = (a->who == SP_WHO_SELF) ? self : actor;
				if (!victim || !IS_ALIVE(victim))
					break;
				sp_expand(a->text, cx, buf, sizeof(buf));
				sp_do_command(victim, buf);
				break;

			case SP_A_SET:
				sp_state_set(cx, SP_TAG_COUNTER, a->slot, a->num);
				break;

			case SP_A_ADD:
				sp_state_set(cx, SP_TAG_COUNTER, a->slot, sp_state_get(cx, SP_TAG_COUNTER, a->slot) + a->num);
				break;

			case SP_A_EXIT:
				rr = real_room(a->num);
				if (rr < 0 || a->slot < 0 || a->slot >= NUM_EXITS || !world[rr].dir_option[a->slot])
					break;
				if (a->num2 >= 0)
				{
					int to = real_room(a->num2);

					if (to >= 0)
						world[rr].dir_option[a->slot]->to_room = to;
				}
				if (a->state >= 0)
				{
					unsigned int f = world[rr].dir_option[a->slot]->exit_info;

					REMOVE_BIT(f, EX_CLOSED | EX_LOCKED | EX_SECRET);
					SET_BIT(f, (unsigned int)a->state);
					world[rr].dir_option[a->slot]->exit_info = f;
				}
				break;

			case SP_A_HEAL:                 /* mob targets only (parse-enforced) */
				if (!self)
					break;
				if (a->num < 0 || GET_HIT(self) + a->num >= GET_MAX_HIT(self))
					GET_HIT(self) = GET_MAX_HIT(self);
				else
					GET_HIT(self) = GET_HIT(self) + a->num;
				update_pos(self);
				act("$n glows briefly with renewed vigor.", TRUE, self, 0, 0, TO_ROOM);
				break;

			case SP_A_BLOCK:
				cx->blocked = TRUE;
				break;

			case SP_A_PURGE:                /* parse guarantees: LAST action */
				if (cx->targ == SP_T_MOB && self)
				{
					act("$n dissolves into wisps of nothing.", TRUE, self, 0, 0, TO_ROOM);
					extract_char(self);
					cx->self_ch = NULL;
					ret |= SP_X_SELFGONE;
				}
				else if (cx->targ == SP_T_OBJ && cx->self_obj)
				{
					buf2[0] = '\0';
					sp_cat(buf2, sizeof(buf2), sp_self_name(cx));
					sp_cat(buf2, sizeof(buf2), " crumbles to dust.\r\n");
					CAP(buf2);
					send_to_room(buf2, room);
					extract_obj(cx->self_obj, TRUE);
					cx->self_obj = NULL;
					ret |= SP_X_SELFGONE;
				}
				return ret;

			default:
				break;
		}
	}
	return ret;
}

/* chance roll + condition gate + re-entrancy latch */
static int sp_fire(struct sp_trig *t, struct sp_ctx *cx)
{
	int r;

	if (t->running)
		return 0;
	if (sp_depth >= SP_MAX_DEPTH)
		return 0;
	if (t->chance < 100 && number(1, 100) > t->chance)
		return 0;
	if (!sp_conds_pass(t, cx))
		return 0;

	t->running = TRUE;
	sp_depth++;
	r = sp_execute(t, cx);
	sp_depth--;
	t->running = FALSE;
	return r;
}

/* ------------------------------------------------------------------ */
/* keyword matching                                                   */
/* ------------------------------------------------------------------ */

static void sp_strlower(char *s)
{
	for (; *s; s++)
		*s = LOWER(*s);
}

static int sp_keyword_match(const char *keywords, const char *lowtext)
{
	char        kw[MAX_INPUT_LENGTH];
	const char *p = keywords;
	int         k;

	if (!keywords || !*keywords)
		return TRUE;                       /* no filter == match all */
	if (!lowtext)
		return FALSE;
	while (p && *p)
	{
		while (*p == ' ')
			p++;
		for (k = 0; *p && *p != ' ' && k < (int)sizeof(kw) - 1; p++, k++)
			kw[k] = *p;
		kw[k] = '\0';
		if (k && strstr(lowtext, kw))
			return TRUE;
	}
	return FALSE;
}

/* ------------------------------------------------------------------ */
/* the generic dispatch                                               */
/* ------------------------------------------------------------------ */

/* run every trigger in rec whose event == ev and whose extra filter
   passes; returns the OR of the sp_fire results */
static int sp_run(struct sp_rec *rec, int ev, struct sp_ctx *cx, const char *lowtext, int filt)
{
	struct sp_trig *t;
	int             r = 0, one;

	cx->rec = rec;
	for (t = rec->trigs; t; t = t->next)
	{
		if (t->event != ev)
			continue;

		switch (ev)
		{
			case SP_EV_SPEECH:
				if (!sp_keyword_match(t->keywords, lowtext))
					continue;
				break;
			case SP_EV_GIVE:
				if (t->arg != -1 && t->arg != filt)
					continue;
				break;
			case SP_EV_CMD:
				if (t->cmdnum >= 0 && t->cmdnum != filt)
					continue;
				if (t->keywords && !sp_keyword_match(t->keywords, lowtext))
					continue;
				break;
			case SP_EV_DAMAGED:
				if (t->arg2 && t->arg2 != filt)
					continue;
				break;
			case SP_EV_HOUR:
				if (t->arg >= 0 && t->arg != filt)
					continue;
				break;
			case SP_EV_HPBELOW:
			{
				int pct, fired;

				if (!cx->self_ch || GET_MAX_HIT(cx->self_ch) <= 0)
					continue;
				pct = (int)((100L * GET_HIT(cx->self_ch)) / GET_MAX_HIT(cx->self_ch));
				fired = sp_char_get(cx->self_ch, SP_TAG_TRIG, t->trig_index);
				if (pct >= t->arg)
				{
					if (fired && GET_HIT(cx->self_ch) >= GET_MAX_HIT(cx->self_ch))
						sp_char_set(cx->self_ch, SP_TAG_TRIG, t->trig_index, 0);
					continue;
				}
				if (fired)
					continue;
				sp_char_set(cx->self_ch, SP_TAG_TRIG, t->trig_index, 1);
				break;
			}
			case SP_EV_PULSE:
			{
				int ctr = sp_state_get(cx, SP_TAG_TRIG, t->trig_index);

				if (ctr <= 0)
					ctr = number(1, t->arg);   /* first sight: jitter */
				if (--ctr > 0)
				{
					sp_state_set(cx, SP_TAG_TRIG, t->trig_index, ctr);
					continue;
				}
				sp_state_set(cx, SP_TAG_TRIG, t->trig_index, t->arg);
				break;
			}
			default:
				break;
		}

		one = sp_fire(t, cx);
		r |= one;
		if (one & SP_X_SELFGONE)
			break;
		if (cx->targ == SP_T_MOB && (!cx->self_ch || (!cx->self_dead_ok && !IS_ALIVE(cx->self_ch))))
			break;
		if (cx->actor && !IS_ALIVE(cx->actor))
			cx->actor = NULL;
	}
	return r;
}

/* translate an engine cmd into our event, or -1 */
static int sp_event_for_cmd(int cmd, int *filt)
{
	*filt = 0;
	switch (cmd)
	{
		case CMD_DEATH:      return SP_EV_DEATH;
		case CMD_TOROOM:     return SP_EV_ENTER;
		case CMD_FROMROOM:   return SP_EV_LEAVE;
		case CMD_MOB_COMBAT: return SP_EV_FIGHT;
		case CMD_PERIODIC:   return SP_EV_PULSE;
		case CMD_FOUND:      return SP_EV_SEARCH;
		case CMD_DECAY:      return SP_EV_DECAY;
		case CMD_GOTHIT:     *filt = 1; return SP_EV_DAMAGED;
		case CMD_GOTNUKED:   *filt = 2; return SP_EV_DAMAGED;
		/* CMD_MELEE_HIT is delivered to the WIELDED weapon's prototype
		   proc with arg == (char *) the victim (fight.c:7782 via
		   weapon_proc, actoff.c:5609 on backstab).  That is the engine's
		   own offensive on-hit hook and it needs no new dispatch. */
		case CMD_MELEE_HIT:  return SP_EV_HIT;
		default:
			if (cmd > 0)
			{
				*filt = cmd;
				return SP_EV_CMD;
			}
			return -1;
	}
}

static int sp_dispatch(struct sp_rec *rec, struct sp_ctx *cx, int cmd, char *arg)
{
	int  ev, filt, r;
	char low[MAX_STRING_LENGTH];

	/* CMD_SET_PERIODIC does double duty: it is the engine asking whether
	   we want a periodic tick, and it is also the moment this instance
	   came into being -- i.e. REPOP. */
	if (cmd == CMD_SET_PERIODIC)
	{
		if (rec->events & (1u << SP_EV_REPOP))
			sp_run(rec, SP_EV_REPOP, cx, NULL, 0);
		return (rec->events & (1u << SP_EV_PULSE)) ? TRUE : FALSE;
	}

	ev = sp_event_for_cmd(cmd, &filt);
	if (ev < 0)
		return FALSE;
	if (!(rec->events & (1u << ev)) && !(ev == SP_EV_FIGHT && (rec->events & (1u << SP_EV_HPBELOW))))
		return FALSE;

	low[0] = '\0';
	if (ev == SP_EV_CMD && arg)
	{
		strncpy(low, arg, sizeof(low) - 1);
		low[sizeof(low) - 1] = '\0';
		sp_strlower(low);
	}

	r = sp_run(rec, ev, cx, low, filt);

	/* a combat round is also where HPBELOW is evaluated: it needs no
	   fight.c hook because the mob is already being asked every round */
	if (ev == SP_EV_FIGHT && !(r & SP_X_SELFGONE) && (rec->events & (1u << SP_EV_HPBELOW)))
		r |= sp_run(rec, SP_EV_HPBELOW, cx, NULL, 0);

	/* A block suppresses the command that triggered it. Data must not
	   be able to do that to a privileged command - otherwise a record
	   could trigger on 'goto' and wall an immortal out of its own zone.
	   The same minimum_level/grantable test sp_do_command() already
	   applies in the other direction. Ordinary blocks still apply to
	   trusted characters: a maze that transfers and then blocks must
	   suppress the original move for everyone, or the immortal walks a
	   broken maze. */
	if (cx->blocked && cmd >= 0 && cmd < MAX_CMD_LIST &&
	    (cmd_info[cmd].minimum_level > 0 || cmd_info[cmd].grantable))
		return FALSE;

	return cx->blocked ? TRUE : FALSE;
}

/* The HOUR timer cannot be armed from studioproc_boot(): boot_db() calls
   us at db.c:621 and the event pool is not created until ne_init_events()
   at db.c:647 (add_event would mm_get() a NULL pool and segfault).  So it
   is armed on the first dispatch instead.  That is guaranteed to happen
   at the right moment and costs no hook: ne_init_events() itself walks
   every room with a funct and calls it with CMD_SET_PERIODIC, immediately
   after creating the pool - and HOUR is parse-restricted to room targets. */
static int sp_hour_armed = 0;

static void sp_arm_hour(void)
{
	if (sp_hour_armed || !studioproc_count)
		return;
	sp_hour_armed = 1;
	sp_last_hour  = (int)time_info.hour;
	add_event(sp_hour_event, WAIT_SEC * 4, NULL, NULL, NULL, 0, NULL, 0);
}

int studioproc_mob(P_char mob, P_char actor, int cmd, char *arg)
{
	struct sp_rec *rec;
	struct sp_ctx  cx;
	char          *targ = arg;

	if (!mob || !sp_on_game_thread())
		return FALSE;
	sp_arm_hour();
	if (!(rec = sp_find(SP_T_MOB, GET_VNUM(mob))))
		return FALSE;

	/* never clobber a hand-written C proc: it runs first and wins */
	if (rec->prev_mob && (*rec->prev_mob)(mob, actor, cmd, arg))
		return TRUE;

	/* CMD_GOTHIT / CMD_GOTNUKED pass a struct proc_data, not a string */
	if (cmd == CMD_GOTHIT || cmd == CMD_GOTNUKED || cmd == CMD_MELEE_HIT)
		targ = NULL;

	memset(&cx, 0, sizeof(cx));
	cx.targ         = SP_T_MOB;
	cx.self_ch      = mob;
	cx.actor        = (actor != mob) ? actor : NULL;
	cx.self_dead_ok = (cmd == CMD_DEATH);
	return sp_dispatch(rec, &cx, cmd, targ);
}

int studioproc_obj(P_obj obj, P_char actor, int cmd, char *arg)
{
	struct sp_rec *rec;
	struct sp_ctx  cx;
	char          *targ = arg;

	if (!obj || obj->R_num < 0 || !sp_on_game_thread())
		return FALSE;
	sp_arm_hour();
	if (!(rec = sp_find(SP_T_OBJ, obj_index[obj->R_num].virtual_number)))
		return FALSE;

	if (rec->prev_obj && (*rec->prev_obj)(obj, actor, cmd, arg))
		return TRUE;

	memset(&cx, 0, sizeof(cx));
	cx.targ     = SP_T_OBJ;
	cx.self_obj = obj;
	cx.actor    = actor;

	/* these three do not pass a string in arg */
	if (cmd == CMD_GOTHIT || cmd == CMD_GOTNUKED)
		targ = NULL;                        /* struct proc_data *      */
	else if (cmd == CMD_MELEE_HIT)
	{
		/* arg is the VICTIM; the wielder arrives as actor.  Present the
		   victim as the actor so $n, 'attack' and every 'on actor'
		   condition read the way a builder expects. */
		P_char vict = (P_char)(void *)arg;

		targ = NULL;
		if (vict && IS_ALIVE(vict))
			cx.actor = vict;
	}
	return sp_dispatch(rec, &cx, cmd, targ);
}

int studioproc_room(int room, P_char actor, int cmd, char *arg)
{
	struct sp_rec *rec;
	struct sp_ctx  cx;

	if (!sp_on_game_thread())
		return FALSE;
	sp_arm_hour();

	/* CALL-SITE INCONSISTENCY, handled here rather than patched there:
	   every room-proc call site passes the room's REAL index -
	   special() (interp.c:2176), char_from_room() (handler.c:981),
	   ne_init_events() (new_events.c:891) - except room_event()
	   (events.c:1348), which passes room->number, the VNUM.  Rather than
	   touch events.c we accept both: an out-of-range value, or an
	   in-range value whose world[] slot does not carry our record, is
	   retried as a vnum.  Worth a one-line fix in events.c one day. */
	if (room < 0 || room > top_of_world)
	{
		room = real_room(room);
		if (room < 0)
			return FALSE;
	}
	if (!(rec = sp_find(SP_T_ROOM, world[room].number)))
	{
		int alt = real_room(room);

		if (alt < 0 || alt == room)
			return FALSE;
		room = alt;
		if (!(rec = sp_find(SP_T_ROOM, world[room].number)))
			return FALSE;
	}

	if (rec->prev_room && (*rec->prev_room)(room, actor, cmd, arg))
		return TRUE;

	memset(&cx, 0, sizeof(cx));
	cx.targ      = SP_T_ROOM;
	cx.self_room = room;
	cx.actor     = actor;
	return sp_dispatch(rec, &cx, cmd, arg);
}

/* ------------------------------------------------------------------ */
/* the four hook entry points                                         */
/* ------------------------------------------------------------------ */

/* actcomm.c do_say(): a PC spoke in ch->in_room.  Also delivers
   CMD_SAY to object instance proclibs, because special() never does
   (and delivering it there would put the reply BEFORE the speech). */
void studioproc_speech(P_char ch, const char *text)
{
	struct sp_rec *rec;
	struct sp_ctx  cx;
	P_char         k, next_k;
	P_obj          o, next_o;
	int            room, j;
	char           low[MAX_STRING_LENGTH], mut[MAX_STRING_LENGTH];

	if (!ch || !text || !*text || !IS_PC(ch) || !sp_on_game_thread())
		return;
	room = ch->in_room;
	if (room < 0 || room > top_of_world)
		return;

	strncpy(low, text, sizeof(low) - 1);
	low[sizeof(low) - 1] = '\0';
	sp_strlower(low);

	if (studioproc_count)
	{
		if ((rec = sp_find(SP_T_ROOM, world[room].number)) != NULL)
		{
			memset(&cx, 0, sizeof(cx));
			cx.targ      = SP_T_ROOM;
			cx.self_room = room;
			cx.actor     = ch;
			sp_run(rec, SP_EV_SPEECH, &cx, low, 0);
			if (!IS_ALIVE(ch) || ch->in_room != room)
				return;
		}

		if (sp_tcount[SP_T_MOB])
		{
			for (k = world[room].people; k; k = next_k)
			{
				next_k = k->next_in_room;
				if (k == ch || !IS_NPC(k) || !IS_ALIVE(k))
					continue;
				if (!(rec = sp_find(SP_T_MOB, GET_VNUM(k))))
					continue;
				memset(&cx, 0, sizeof(cx));
				cx.targ    = SP_T_MOB;
				cx.self_ch = k;
				cx.actor   = ch;
				sp_run(rec, SP_EV_SPEECH, &cx, low, 0);
				if (!IS_ALIVE(ch) || ch->in_room != room)
					return;
			}
		}

		if (sp_tcount[SP_T_OBJ])
		{
			for (o = world[room].contents; o; o = next_o)
			{
				next_o = o->next_content;
				if (o->R_num >= 0 && (rec = sp_find(SP_T_OBJ, obj_index[o->R_num].virtual_number)) != NULL)
				{
					memset(&cx, 0, sizeof(cx));
					cx.targ     = SP_T_OBJ;
					cx.self_obj = o;
					cx.actor    = ch;
					sp_run(rec, SP_EV_SPEECH, &cx, low, 0);
					if (!IS_ALIVE(ch) || ch->in_room != room)
						return;
				}
			}
			for (o = ch->carrying; o; o = next_o)
			{
				next_o = o->next_content;
				if (o->R_num >= 0 && (rec = sp_find(SP_T_OBJ, obj_index[o->R_num].virtual_number)) != NULL)
				{
					memset(&cx, 0, sizeof(cx));
					cx.targ     = SP_T_OBJ;
					cx.self_obj = o;
					cx.actor    = ch;
					sp_run(rec, SP_EV_SPEECH, &cx, low, 0);
					if (!IS_ALIVE(ch) || ch->in_room != room)
						return;
				}
			}
			/* worn as well - the same set special() walks for object
			   procs (interp.c:2184), and the same set the proclib tail
			   below already covers: a ring that answers its wearer
			   behaves like any other object proc */
			for (j = 0; j < MAX_WEAR; j++)
			{
				o = ch->equipment[j];
				if (!o || o->R_num < 0)
					continue;
				if ((rec = sp_find(SP_T_OBJ, obj_index[o->R_num].virtual_number)) == NULL)
					continue;
				memset(&cx, 0, sizeof(cx));
				cx.targ     = SP_T_OBJ;
				cx.self_obj = o;
				cx.actor    = ch;
				sp_run(rec, SP_EV_SPEECH, &cx, low, 0);
				if (!IS_ALIVE(ch) || ch->in_room != room)
					return;
			}
		}
	}

	/* instance-proclib objects in the room / on the speaker */
	strncpy(mut, text, sizeof(mut) - 1);
	mut[sizeof(mut) - 1] = '\0';
	for (o = world[room].contents; o; o = next_o)
	{
		next_o = o->next_content;
		if (IS_SET(o->extra_flags, ITEM_PROCLIB))
			proclib_obj_proc(o, ch, CMD_SAY, mut);
		if (!IS_ALIVE(ch) || ch->in_room != room)
			return;
	}
	for (o = ch->carrying; o; o = next_o)
	{
		next_o = o->next_content;
		if (IS_SET(o->extra_flags, ITEM_PROCLIB))
			proclib_obj_proc(o, ch, CMD_SAY, mut);
		if (!IS_ALIVE(ch) || ch->in_room != room)
			return;
	}
	for (j = 0; j < MAX_WEAR; j++)
	{
		if (ch->equipment[j] && IS_SET(ch->equipment[j]->extra_flags, ITEM_PROCLIB))
			proclib_obj_proc(ch->equipment[j], ch, CMD_SAY, mut);
		if (!IS_ALIVE(ch) || ch->in_room != room)
			return;
	}
}

/* actobj.c do_give(): vict was just handed obj (last statement, so a
   purging trigger leaves nothing behind for the caller to touch) */
void studioproc_give(P_char vict, P_obj obj, P_char giver)
{
	struct sp_rec *rec;
	struct sp_ctx  cx;
	int            ovnum;

	if (!studioproc_count || !vict || !IS_NPC(vict) || vict->in_room < 0 || !sp_on_game_thread())
		return;
	if (!(rec = sp_find(SP_T_MOB, GET_VNUM(vict))))
		return;
	ovnum = (obj && obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -2;

	memset(&cx, 0, sizeof(cx));
	cx.targ    = SP_T_MOB;
	cx.self_ch = vict;
	cx.actor   = giver;
	cx.given   = obj;
	sp_run(rec, SP_EV_GIVE, &cx, NULL, ovnum);
}

/* fight.c die(): killer just killed victim.  Fires the KILL triggers of
   the killer, of every object the killer has equipped, and of the room. */
void studioproc_kill(P_char killer, P_char victim)
{
	struct sp_rec *rec;
	struct sp_ctx  cx;
	int            i, room;

	if (!studioproc_count || !killer || !victim || !IS_ALIVE(killer) || !sp_on_game_thread())
		return;
	room = killer->in_room;
	if (room < 0 || room > top_of_world)
		return;

	if (IS_NPC(killer) && (rec = sp_find(SP_T_MOB, GET_VNUM(killer))) != NULL)
	{
		memset(&cx, 0, sizeof(cx));
		cx.targ    = SP_T_MOB;
		cx.self_ch = killer;
		cx.actor   = victim;
		cx.self_dead_ok = TRUE;             /* the victim is already dead */
		sp_run(rec, SP_EV_KILL, &cx, NULL, 0);
		if (!IS_ALIVE(killer))
			return;
	}

	if (sp_tcount[SP_T_OBJ])
	{
		for (i = 0; i < MAX_WEAR; i++)
		{
			P_obj o = killer->equipment[i];

			if (!o || o->R_num < 0)
				continue;
			if (!(rec = sp_find(SP_T_OBJ, obj_index[o->R_num].virtual_number)))
				continue;
			memset(&cx, 0, sizeof(cx));
			cx.targ     = SP_T_OBJ;
			cx.self_obj = o;
			cx.actor    = killer;
			cx.self_dead_ok = TRUE;
			sp_run(rec, SP_EV_KILL, &cx, NULL, 0);
			if (!IS_ALIVE(killer))
				return;
		}
	}

	if (sp_tcount[SP_T_ROOM] && (rec = sp_find(SP_T_ROOM, world[room].number)) != NULL)
	{
		memset(&cx, 0, sizeof(cx));
		cx.targ      = SP_T_ROOM;
		cx.self_room = room;
		cx.actor     = killer;
		sp_run(rec, SP_EV_KILL, &cx, NULL, 0);
	}
}

/* ------------------------------------------------------------------ */
/* the HOUR event: our own timer, so weather.c needs no hook           */
/* ------------------------------------------------------------------ */

static void sp_hour_event(P_char ch, P_char victim, P_obj obj, void *data)
{
	int            h = (int)time_info.hour;
	int            t, r;
	struct sp_rec *rec;
	struct sp_ctx  cx;

	if (studioproc_count && h != sp_last_hour)
	{
		sp_last_hour = h;
		for (t = 0; t < SP_NUM_T; t++)
		{
			if (!sp_tcount[t])
				continue;
			for (r = 0; r < SP_HASH; r++)
			{
				for (rec = sp_tab[t][r]; rec; rec = rec->next)
				{
					if (!(rec->events & (1u << SP_EV_HOUR)))
						continue;
					if (rec->target != SP_T_ROOM)
						continue;             /* mobs/objects are instances; rooms are not */
					memset(&cx, 0, sizeof(cx));
					cx.targ      = SP_T_ROOM;
					cx.self_room = real_room(rec->vnum);
					if (cx.self_room < 0)
						continue;
					sp_run(rec, SP_EV_HOUR, &cx, NULL, h);
				}
			}
		}
	}
	add_event(sp_hour_event, WAIT_SEC * 4, NULL, NULL, NULL, 0, NULL, 0);
}

/* ------------------------------------------------------------------ */
/* parser                                                             */
/* ------------------------------------------------------------------ */

static int sp_lineno = 0;

static char *sp_gets(FILE *fl, char *buf, int sz)
{
	int l;

	if (!fgets(buf, sz, fl))
		return NULL;
	sp_lineno++;
	l = (int)strlen(buf);
	while (l > 0 && (buf[l - 1] == '\n' || buf[l - 1] == '\r'))
		buf[--l] = '\0';
	return buf;
}

static void sp_err(int vnum, const char *msg, const char *line)
{
	logit(LOG_STATUS, "STUDIOPROC: parse error zone %d vnum %d line %d: %s [%s] -- record skipped", vnum / 100, vnum, sp_lineno, msg, line ? line : "");
}

static int sp_skip_record(FILE *fl)
{
	char buf[MAX_STRING_LENGTH];

	while (sp_gets(fl, buf, sizeof(buf)))
	{
		if (buf[0] == 'S' && (buf[1] == '\0' || buf[1] == ' '))
			return TRUE;
		if (!strcmp(buf, "#~"))
			return FALSE;
	}
	return FALSE;
}

static void sp_free_rec(struct sp_rec *rec)
{
	struct sp_trig *t, *next_t;
	int             i;

	if (!rec)
		return;
	for (t = rec->trigs; t; t = next_t)
	{
		next_t = t->next;
		if (t->keywords)
			FREE(t->keywords);
		for (i = 0; i < t->num_conds; i++)
			if (t->conds[i].text)
				FREE(t->conds[i].text);
		for (i = 0; i < t->num_actions; i++)
		{
			if (t->actions[i].text)
				FREE(t->actions[i].text);
			if (t->actions[i].text2)
				FREE(t->actions[i].text2);
			if (t->actions[i].text3)
				FREE(t->actions[i].text3);
		}
		FREE(t);
	}
	FREE(rec);
}

/* read one value: a "quoted string", a 'quoted string', or a bare word */
static const char *sp_qval(const char *p, char *val, int valsz)
{
	int i = 0;

	while (*p == ' ')
		p++;
	if (*p == '"' || *p == '\'')
	{
		char q = *p++;

		while (*p && *p != q && i < valsz - 1)
			val[i++] = *p++;
		if (*p == q)
			p++;
	}
	else
	{
		while (*p && *p != ' ' && i < valsz - 1)
			val[i++] = *p++;
	}
	val[i] = '\0';
	return p;
}

/*
 * Read ONE bare whitespace-delimited token, lowercased.
 *
 * THIS EXISTS BECAUSE one_argument() CANNOT BE USED TO READ A KEYWORD.
 * interp.c:2034 ends with `while (fill_word(first_arg));` and
 * interp.c:1110 is
 *     fill_words[] = {"in", "from", "with", "the", "on", "at", "to", "\n"};
 * so one_argument SILENTLY SKIPS those seven words and hands back the one
 * after.  That is right for a player typing "get sword from bag"; it is
 * fatal for a grammar that uses `on` and `to` as keywords:
 *
 *     affect curse 40 on actor        one_argument returns "actor", not "on"
 *     exit 5001 north to 5002         one_argument returns "5002", not "to"
 *
 * i.e. `on self` was silently ignored and `exit ... to <vnum>` silently
 * never retargeted anything.  Every keyword in this file is read with
 * sp_word(); one_argument stays only where the token is genuinely a
 * player-supplied argument.
 */
static const char *sp_word(const char *p, char *out, int outsz)
{
	int i = 0;

	if (!p)
	{
		*out = '\0';
		return NULL;
	}
	while (*p && isspace((unsigned char)*p))
		p++;
	while (*p && !isspace((unsigned char)*p) && i < outsz - 1)
	{
		out[i++] = LOWER(*p);
		p++;
	}
	out[i] = '\0';
	return p;
}

/* pull a "key value" pair, or a "key \"quoted value\"" pair */
static const char *sp_kv(const char *p, char *key, int keysz, char *val, int valsz)
{
	p = sp_word(p, key, keysz);
	return sp_qval(p, val, valsz);
}

static int sp_parse_op(const char *s, int *op)
{
	if (!strcmp(s, "=") || !strcmp(s, "=="))
		*op = SP_OP_EQ;
	else if (!strcmp(s, "!=") || !strcmp(s, "<>"))
		*op = SP_OP_NE;
	else if (!strcmp(s, "<"))
		*op = SP_OP_LT;
	else if (!strcmp(s, "<="))
		*op = SP_OP_LE;
	else if (!strcmp(s, ">"))
		*op = SP_OP_GT;
	else if (!strcmp(s, ">="))
		*op = SP_OP_GE;
	else
		return FALSE;
	return TRUE;
}

static int sp_spell_by_name(const char *name)
{
	char low[MAX_INPUT_LENGTH];
	int  n;

	strncpy(low, name, sizeof(low) - 1);
	low[sizeof(low) - 1] = '\0';
	sp_strlower(low);
	n = old_search_block(low, 0, (uint)strlen(low), spells, 0) - 1;
	if (n < 0 || n > MAX_AFFECT_TYPES)
		return -1;
	return n;
}

static int sp_save_by_name(const char *s)
{
	if (!str_cmp(s, "para"))   return SAVING_PARA;
	if (!str_cmp(s, "rod"))    return SAVING_ROD;
	if (!str_cmp(s, "fear"))   return SAVING_FEAR;
	if (!str_cmp(s, "breath")) return SAVING_BREATH;
	if (!str_cmp(s, "spell"))  return SAVING_SPELL;
	return -1;
}

static int sp_dtype_by_name(const char *s)
{
	if (!str_cmp(s, "fire"))      return SPLDAM_FIRE;
	if (!str_cmp(s, "cold"))      return SPLDAM_COLD;
	if (!str_cmp(s, "lightning")) return SPLDAM_LIGHTNING;
	if (!str_cmp(s, "gas"))       return SPLDAM_GAS;
	if (!str_cmp(s, "acid"))      return SPLDAM_ACID;
	if (!str_cmp(s, "negative"))  return SPLDAM_NEGATIVE;
	if (!str_cmp(s, "holy"))      return SPLDAM_HOLY;
	if (!str_cmp(s, "psi"))       return SPLDAM_PSI;
	if (!str_cmp(s, "spirit"))    return SPLDAM_SPIRIT;
	if (!str_cmp(s, "sound"))     return SPLDAM_SOUND;
	if (!str_cmp(s, "earth"))     return SPLDAM_EARTH;
	if (!str_cmp(s, "generic"))   return SPLDAM_GENERIC;
	return 0;                                    /* untyped = physical */
}

static int sp_race_by_code(const char *s)
{
	int i;

	for (i = 1; i <= LAST_RACE; i++)
		if (race_names_table[i].code && !str_cmp(race_names_table[i].code, s))
			return i;
	return -1;
}

/*
 * class_names_table[] (common.c:886) is 1-based against the CLASS_* bits:
 * slot 0 is "None", slot 1 is "Warrior", and CLASS_WARRIOR is BIT_1 == 1.
 * So the bit for slot i is 1 << (i - 1), and slot 0 has no bit at all.
 * The table is NULL-terminated one past CLASS_COUNT, hence i <= CLASS_COUNT.
 */
static int sp_class_by_name(const char *s)
{
	int i;

	for (i = 1; i <= CLASS_COUNT; i++)
		if (class_names_table[i].normal && !str_cmp(class_names_table[i].normal, s))
			return 1 << (i - 1);
	return 0;
}

/*
 * apply_types[] (common.c:811) is the ostat/oedit label table: index 0 is
 * "NONE", and the index IS the APPLY_xxx constant.  The table ends with a
 * "\n" sentinel.  A bare number is accepted too, so a builder is never
 * blocked by a label the engine spells differently from the header.
 */
static int sp_apply_by_name(const char *s)
{
	int i;

	if (isdigit((unsigned char)*s))
	{
		i = atoi(s);
		return (i >= APPLY_NONE && i <= APPLY_COMBAT_PULSE) ? i : -1;
	}
	for (i = 0; apply_types[i] && *apply_types[i] != '\n'; i++)
		if (!str_cmp(apply_types[i], s))
			return i;
	return -1;
}

/*
 * The five AFF words each have a flagDef table (common.c:630..776), each
 * NULL-terminated by a {0} row, each bit b of word w being 1UL << b.  A
 * name is looked up across all five words; *word comes back 1..5.
 */
static unsigned long sp_aff_by_name(const char *s, int *word)
{
	flagDef *tables[5];
	int      w, b;

	tables[0] = affected1_bits;
	tables[1] = affected2_bits;
	tables[2] = affected3_bits;
	tables[3] = affected4_bits;
	tables[4] = affected5_bits;

	for (w = 0; w < 5; w++)
		for (b = 0; b < 32 && tables[w][b].flagShort; b++)
			if (!str_cmp(tables[w][b].flagShort, s))
			{
				*word = w + 1;
				return 1UL << b;
			}
	*word = 0;
	return 0;
}

static int sp_dir_by_name(const char *s)
{
	int i;

	for (i = 0; i < NUM_EXITS; i++)
		if (dirs[i] && !str_cmp(dirs[i], s))
			return i;
	return -1;
}

/* --- T <EVENT> [args] --------------------------------------------- */
static struct sp_trig *sp_parse_event(int targ, int vnum, char *line)
{
	struct sp_trig *t;
	char            word[MAX_INPUT_LENGTH];
	const char     *p;
	int             n;

	p = sp_word(line + 1, word, sizeof(word));
	sp_strlower(word);

	CREATE(t, struct sp_trig, 1, MEM_TAG_BUFFER);
	memset(t, 0, sizeof(*t));
	t->chance = 100;
	t->arg    = -1;
	t->cmdnum = -1;

	if (!strcmp(word, "death"))
		t->event = SP_EV_DEATH;
	else if (!strcmp(word, "kill"))
		t->event = SP_EV_KILL;
	else if (!strcmp(word, "speech"))
	{
		t->event = SP_EV_SPEECH;
		while (*p == ' ')
			p++;
		if (!*p)
		{
			sp_err(vnum, "SPEECH needs at least one keyword", line);
			FREE(t);
			return NULL;
		}
		t->keywords = str_dup(p);
		sp_strlower(t->keywords);
	}
	else if (!strcmp(word, "give"))
	{
		t->event = SP_EV_GIVE;
		sp_word(p, word, sizeof(word));
		sp_strlower(word);
		if (!strcmp(word, "any"))
			t->arg = -1;
		else if ((n = atoi(word)) > 0 && real_object(n) >= 0)
			t->arg = n;
		else
		{
			sp_err(vnum, "GIVE needs an existing obj vnum or ANY", line);
			FREE(t);
			return NULL;
		}
	}
	else if (!strcmp(word, "enter"))
		t->event = SP_EV_ENTER;
	else if (!strcmp(word, "leave"))
		t->event = SP_EV_LEAVE;
	else if (!strcmp(word, "repop"))
		t->event = SP_EV_REPOP;
	else if (!strcmp(word, "search"))
		t->event = SP_EV_SEARCH;
	else if (!strcmp(word, "decay"))
		t->event = SP_EV_DECAY;
	else if (!strcmp(word, "hit"))
		t->event = SP_EV_HIT;
	else if (!strcmp(word, "fight"))
		t->event = SP_EV_FIGHT;
	else if (!strcmp(word, "damaged"))
	{
		t->event = SP_EV_DAMAGED;
		sp_word(p, word, sizeof(word));
		sp_strlower(word);
		if (!strcmp(word, "melee"))
			t->arg2 = 1;
		else if (!strcmp(word, "spell"))
			t->arg2 = 2;
		else
			t->arg2 = 0;
	}
	else if (!strcmp(word, "pulse"))
	{
		t->event = SP_EV_PULSE;
		sp_word(p, word, sizeof(word));
		n = atoi(word);
		if (n < SP_MIN_PULSE)
			n = SP_MIN_PULSE;
		t->arg = n;
	}
	else if (!strcmp(word, "hpbelow"))
	{
		t->event = SP_EV_HPBELOW;
		sp_word(p, word, sizeof(word));
		n = atoi(word);
		if (n < 1 || n > 99)
		{
			sp_err(vnum, "HPBELOW needs a percent 1-99", line);
			FREE(t);
			return NULL;
		}
		t->arg = n;
	}
	else if (!strcmp(word, "hour"))
	{
		t->event = SP_EV_HOUR;
		sp_word(p, word, sizeof(word));
		sp_strlower(word);
		t->arg = !strcmp(word, "any") ? -1 : atoi(word);
	}
	else if (!strcmp(word, "cmd"))
	{
		t->event = SP_EV_CMD;
		p        = sp_word(p, word, sizeof(word));
		sp_strlower(word);
		if (!*word)
		{
			sp_err(vnum, "CMD needs a verb", line);
			FREE(t);
			return NULL;
		}
		if (strcmp(word, "any"))
		{
			n = old_search_block(word, 0, (uint)strlen(word), command, 2) - 1;
			if (n < 0)
			{
				sp_err(vnum, "CMD: no such command verb", line);
				FREE(t);
				return NULL;
			}
			t->cmdnum = n;
		}
		while (*p == ' ')
			p++;
		if (*p)
		{
			t->keywords = str_dup(p);
			sp_strlower(t->keywords);
		}
	}
	else
	{
		sp_err(vnum, "unknown event", line);
		FREE(t);
		return NULL;
	}

	/* event / target-type validity */
	if (targ != SP_T_MOB && (t->event == SP_EV_DEATH || t->event == SP_EV_FIGHT || t->event == SP_EV_GIVE || t->event == SP_EV_HPBELOW))
	{
		sp_err(vnum, "event is only valid on a mob target", line);
		if (t->keywords)
			FREE(t->keywords);
		FREE(t);
		return NULL;
	}
	if (targ != SP_T_OBJ && (t->event == SP_EV_SEARCH || t->event == SP_EV_DECAY || t->event == SP_EV_HIT))
	{
		sp_err(vnum, "event is only valid on an object target", line);
		if (t->keywords)
			FREE(t->keywords);
		FREE(t);
		return NULL;
	}
	if (targ != SP_T_ROOM && t->event == SP_EV_LEAVE)
	{
		sp_err(vnum, "LEAVE is only valid on a room target", line);
		if (t->keywords)
			FREE(t->keywords);
		FREE(t);
		return NULL;
	}
	if (targ != SP_T_ROOM && t->event == SP_EV_HOUR)
	{
		sp_err(vnum, "HOUR is only valid on a room target", line);
		if (t->keywords)
			FREE(t->keywords);
		FREE(t);
		return NULL;
	}
	return t;
}

/* --- if <cond> ----------------------------------------------------- */
static int sp_parse_cond(int vnum, struct sp_trig *t, char *line)
{
	struct sp_cond *c;
	char            word[MAX_INPUT_LENGTH], w2[MAX_INPUT_LENGTH], w3[MAX_INPUT_LENGTH];
	const char     *p;
	int             n;

	if (t->num_conds >= SP_MAX_CONDS)
	{
		sp_err(vnum, "too many if lines in one trigger", line);
		return FALSE;
	}
	c = &t->conds[t->num_conds];
	memset(c, 0, sizeof(*c));
	c->op  = SP_OP_GE;
	c->who = SP_WHO_ACTOR;

	p = sp_word(line, word, sizeof(word));            /* eat the leading "if" */
	p = sp_word(p, word, sizeof(word));
	sp_strlower(word);
	if (word[0] == '!')
	{
		c->neg = 1;
		memmove(word, word + 1, strlen(word));
	}

	if (!strcmp(word, "carrying") || !strcmp(word, "wearing"))
	{
		c->kind = !strcmp(word, "carrying") ? SP_C_CARRYING : SP_C_WEARING;
		p       = sp_word(p, w2, sizeof(w2));
		c->num  = atoi(w2);
		if (c->num <= 0)
		{
			sp_err(vnum, "carrying/wearing needs an obj vnum", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "class"))
	{
		c->kind = SP_C_CLASS;
		p       = sp_word(p, w2, sizeof(w2));
		c->num  = sp_class_by_name(w2);
		if (!c->num)
		{
			sp_err(vnum, "unknown class name", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "race"))
	{
		c->kind = SP_C_RACE;
		p       = sp_word(p, w2, sizeof(w2));
		c->num  = sp_race_by_code(w2);
		if (c->num < 0)
		{
			sp_err(vnum, "unknown race code", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "sex"))
	{
		c->kind = SP_C_SEX;
		p       = sp_word(p, w2, sizeof(w2));
		sp_strlower(w2);
		c->num = (w2[0] == 'm') ? 1 : (w2[0] == 'f') ? 2 : 0;
	}
	else if (!strcmp(word, "room") || !strcmp(word, "zone"))
	{
		c->kind = !strcmp(word, "room") ? SP_C_ROOM : SP_C_ZONE;
		p       = sp_word(p, w2, sizeof(w2));
		c->num  = atoi(w2);
	}
	else if (!strcmp(word, "affect"))
	{
		c->kind = SP_C_AFFECT;
		/*
		 * The name is ONE value: a bare word or a 'quoted name'.  Read it with
		 * sp_qval, never sp_kv -- sp_kv would swallow the name as a key and then
		 * look the spell up by whatever followed it ("on", "self", a number).
		 */
		p       = sp_qval(p, w2, sizeof(w2));
		c->num  = sp_spell_by_name(w2);
		if (c->num < 0)
		{
			sp_err(vnum, "unknown affect / spell name", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "chance"))
	{
		c->kind = SP_C_CHANCE;
		p       = sp_word(p, w2, sizeof(w2));
		c->num  = atoi(w2);
		if (c->num < 1 || c->num > 100)
		{
			sp_err(vnum, "chance must be 1-100", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "ispc"))
		c->kind = SP_C_ISPC;
	else if (!strcmp(word, "fighting"))
		c->kind = SP_C_FIGHTING;
	else if (!strcmp(word, "counter"))
	{
		c->kind = SP_C_COUNTER;
		p       = sp_word(p, w2, sizeof(w2));
		c->slot = sp_intern_counter(w2);
		if (c->slot < 0)
		{
			sp_err(vnum, "counter name table full or empty name", line);
			return FALSE;
		}
		p = sp_word(p, w3, sizeof(w3));
		if (!sp_parse_op(w3, &c->op))
		{
			sp_err(vnum, "counter needs an operator (= != < <= > >=)", line);
			return FALSE;
		}
		p      = sp_word(p, w3, sizeof(w3));
		c->num = atoi(w3);
	}
	else if (!strcmp(word, "mobs"))
	{
		/* mobs <mvnum> [room|zone|world] <op> <n>   (default: room) */
		c->kind = SP_C_MOBS;
		c->who  = SP_SC_ROOM;
		p       = sp_word(p, w2, sizeof(w2));
		c->num  = atoi(w2);
		p       = sp_word(p, w3, sizeof(w3));
		sp_strlower(w3);
		if (!strcmp(w3, "room") || !strcmp(w3, "zone") || !strcmp(w3, "world"))
		{
			c->who = !strcmp(w3, "world") ? SP_SC_WORLD : !strcmp(w3, "zone") ? SP_SC_ZONE : SP_SC_ROOM;
			p      = sp_word(p, w3, sizeof(w3));
		}
		if (!sp_parse_op(w3, &c->op))
		{
			sp_err(vnum, "mobs needs an operator", line);
			return FALSE;
		}
		p       = sp_word(p, w3, sizeof(w3));
		c->num2 = atoi(w3);
		t->num_conds++;
		return TRUE;                          /* no trailing "on ..." */
	}
	else if (!strcmp(word, "level") || !strcmp(word, "align") || !strcmp(word, "hour") || !strcmp(word, "hp") || !strcmp(word, "groupsize") || !strcmp(word, "pcs"))
	{
		c->kind = !strcmp(word, "level") ? SP_C_LEVEL
		        : !strcmp(word, "align") ? SP_C_ALIGN
		        : !strcmp(word, "hour")  ? SP_C_HOUR
		        : !strcmp(word, "hp")    ? SP_C_HP
		        : !strcmp(word, "pcs")   ? SP_C_PCS
		                                 : SP_C_GROUP;
		p = sp_word(p, w2, sizeof(w2));
		if (!sp_parse_op(w2, &c->op))
		{
			sp_err(vnum, "numeric condition needs an operator (= != < <= > >=)", line);
			return FALSE;
		}
		p      = sp_word(p, w3, sizeof(w3));
		c->num = atoi(w3);
	}
	else
	{
		sp_err(vnum, "unknown condition", line);
		return FALSE;
	}

	/* optional trailing "on self|actor" -- sp_word, never one_argument: "on"
	   is a fill word and one_argument would hand back "self"/"actor" as the
	   FIRST token, the test below would fail, and the clause would be
	   silently ignored.  See the comment on sp_word(). */
	p = sp_word(p, w2, sizeof(w2));
	if (*w2)
	{
		if (!strcmp(w2, "on"))
		{
			p = sp_word(p, w2, sizeof(w2));
			c->who = !strcmp(w2, "self") ? SP_WHO_SELF : SP_WHO_ACTOR;
		}
		else
		{
			sp_err(vnum, "trailing junk after the condition -- want nothing, or 'on self|actor'", line);
			return FALSE;
		}
	}
	(void)n;
	t->num_conds++;
	return TRUE;
}

/* --- one action line ---------------------------------------------- */
static int sp_parse_action(int targ, int vnum, struct sp_trig *t, char *line)
{
	struct sp_action *a;
	char              word[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], val[MAX_STRING_LENGTH];
	const char       *p;
	char             *q1, *q2;
	int               n;

	if (t->num_actions >= SP_MAX_ACTIONS)
	{
		sp_err(vnum, "too many actions in one trigger", line);
		return FALSE;
	}
	if (t->num_actions > 0 && t->actions[t->num_actions - 1].op == SP_A_PURGE)
	{
		sp_err(vnum, "purge must be the LAST action", line);
		return FALSE;
	}

	a = &t->actions[t->num_actions];
	memset(a, 0, sizeof(*a));
	a->save  = -1;
	a->num2  = -1;
	a->state = -1;
	a->slot  = -1;
	a->scope = SP_SCOPE_ONE;
	a->who   = SP_WHO_ACTOR;

	p = sp_word(line, word, sizeof(word));
	sp_strlower(word);
	while (*p == ' ')
		p++;

	if (!strcmp(word, "say") || !strcmp(word, "emote") || !strcmp(word, "echo") || !strcmp(word, "zecho"))
	{
		if (!*p)
		{
			sp_err(vnum, "text action needs text", line);
			return FALSE;
		}
		a->op   = !strcmp(word, "say")   ? SP_A_SAY
		        : !strcmp(word, "emote") ? SP_A_EMOTE
		        : !strcmp(word, "echo")  ? SP_A_ECHO
		                                 : SP_A_ZECHO;
		a->text = str_dup(p);
	}
	else if (!strcmp(word, "cast"))
	{
		if (targ != SP_T_MOB || t->event == SP_EV_DEATH)
		{
			sp_err(vnum, "cast is only valid for living mob targets", line);
			return FALSE;
		}
		q1 = strchr((char *)p, '\'');
		q2 = q1 ? strchr(q1 + 1, '\'') : NULL;
		if (!q1 || !q2 || q2 == q1 + 1 || (q2 - q1) >= (int)sizeof(name))
		{
			sp_err(vnum, "cast needs a 'quoted spell name'", line);
			return FALSE;
		}
		strncpy(name, q1 + 1, q2 - q1 - 1);
		name[q2 - q1 - 1] = '\0';
		n = sp_spell_by_name(name);
		if (n < 0 || !IS_SPELL(n))
		{
			sp_err(vnum, "unknown spell name", line);
			return FALSE;
		}
		a->op  = SP_A_CAST;
		a->num = n;
		sp_word(q2 + 1, word, sizeof(word));
		sp_strlower(word);
		a->who = !strcmp(word, "self") ? SP_WHO_SELF : !strcmp(word, "ally") ? SP_WHO_ALLY : SP_WHO_ACTOR;
	}
	else if (!strcmp(word, "mload") || !strcmp(word, "oload") || !strcmp(word, "give") || !strcmp(word, "transfer") || !strcmp(word, "goto"))
	{
		n = atoi(p);
		if (!strcmp(word, "goto") && (targ != SP_T_MOB || t->event == SP_EV_DEATH))
		{
			sp_err(vnum, "goto is only valid for living mob targets", line);
			return FALSE;
		}
		if (!strcmp(word, "mload"))
		{
			a->op = SP_A_MLOAD;
			if (n <= 0 || real_mobile(n) < 0)
			{
				sp_err(vnum, "mload: no such mob vnum", line);
				return FALSE;
			}
		}
		else if (!strcmp(word, "oload") || !strcmp(word, "give"))
		{
			a->op = !strcmp(word, "oload") ? SP_A_OLOAD : SP_A_GIVE;
			if (n <= 0 || real_object(n) < 0)
			{
				sp_err(vnum, "no such obj vnum", line);
				return FALSE;
			}
		}
		else
		{
			a->op = !strcmp(word, "transfer") ? SP_A_TRANSFER : SP_A_GOTO;
			if (n <= 0 || real_room(n) < 0)
			{
				sp_err(vnum, "no such room vnum", line);
				return FALSE;
			}
		}
		a->num = n;
	}
	else if (!strcmp(word, "damage"))
	{
		a->op     = SP_A_DAMAGE;
		a->dbonus = 0;
		if (sscanf(p, "%dd%d+%d", &a->dnum, &a->dsize, &a->dbonus) < 2 || a->dnum < 1 || a->dnum > 100 || a->dsize < 1 || a->dsize > 1000 || a->dbonus < 0)
		{
			sp_err(vnum, "damage needs NdS or NdS+B", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "attack"))
	{
		a->op     = SP_A_ATTACK;
		a->dbonus = 0;
		if (sscanf(p, "%dd%d+%d", &a->dnum, &a->dsize, &a->dbonus) < 2 || a->dnum < 1 || a->dnum > 100 || a->dsize < 1 || a->dsize > 1000 || a->dbonus < 0)
		{
			sp_err(vnum, "attack needs NdS or NdS+B first", line);
			return FALSE;
		}
		p = sp_word(p, word, sizeof(word));             /* eat the dice */
		while (*p)
		{
			p = sp_word(p, word, sizeof(word));
			sp_strlower(word);
			if (!*word)
				break;
			if (!strcmp(word, "half"))         /* the one bare flag */
			{
				a->savehalf = 1;
				while (*p == ' ')
					p++;
				continue;
			}
			p = sp_qval(p, val, sizeof(val));
			if (!strcmp(word, "type"))
				a->dtype = sp_dtype_by_name(val);
			else if (!strcmp(word, "scope"))
			{
				sp_strlower(val);
				a->scope = !strcmp(val, "room")    ? SP_SCOPE_ROOM
				         : !strcmp(val, "group")   ? SP_SCOPE_GROUP
				         : !strcmp(val, "nottank") ? SP_SCOPE_NOTTANK
				                                   : SP_SCOPE_ONE;
			}
			else if (!strcmp(word, "save"))
			{
				sp_strlower(val);
				a->save = sp_save_by_name(val);
				if (a->save < 0)
				{
					sp_err(vnum, "attack save must be para|rod|fear|breath|spell", line);
					return FALSE;
				}
			}
			else if (!strcmp(word, "cd"))
				a->cooldown = atoi(val);
			else if (!strcmp(word, "hit"))
				a->text = str_dup(val);
			else if (!strcmp(word, "miss"))
				a->text2 = str_dup(val);
			else if (!strcmp(word, "room"))
				a->text3 = str_dup(val);
			else
			{
				sp_err(vnum, "unknown attack keyword", line);
				return FALSE;
			}
			while (*p == ' ')
				p++;
		}
		if (!a->text)
			a->text = str_dup("&+r$N strikes you!&n");
	}
	else if (!strcmp(word, "affect") || !strcmp(word, "unaffect"))
	{
		int isaff = !strcmp(word, "affect");

		a->op = isaff ? SP_A_AFFECT : SP_A_UNAFFECT;
		/*
		 * The spell/tag name is ONE value: a bare word or a 'quoted name'.
		 * Read it with sp_qval, never sp_kv -- sp_kv treats the name as a key
		 * and hands back the NEXT word as the value, so "affect curse 40 on
		 * actor" would look up a spell called "40", fail, and take the whole
		 * record down with it.  sp_qval also makes "affect 'cure serious' 30"
		 * work, which is the grammar PACK_DESIGN.md §1.1 documents.
		 */
		p      = sp_qval(p, name, sizeof(name));
		a->num = sp_spell_by_name(name);
		if (a->num < 0)
		{
			sp_err(vnum, "unknown affect / spell name", line);
			return FALSE;
		}
		if (isaff)
		{
			p      = sp_word(p, word, sizeof(word));
			a->dur = atoi(word);
			if (a->dur < 1)
				a->dur = 1;
		}
		while (*p)
		{
			p = sp_kv(p, word, sizeof(word), val, sizeof(val));
			if (!*word)
				break;
			if (!strcmp(word, "on"))
			{
				sp_strlower(val);
				a->who = !strcmp(val, "self") ? SP_WHO_SELF : SP_WHO_ACTOR;
			}
			else if (!strcmp(word, "save"))
			{
				sp_strlower(val);
				a->save = sp_save_by_name(val);
			}
			else if (!strcmp(word, "apply"))
			{
				/* apply <APPLY name|number> <modifier> -- two values, so the
				   modifier is read straight after the key's own value. */
				a->apply = sp_apply_by_name(val);
				if (a->apply < 0)
				{
					sp_err(vnum, "unknown apply type -- see apply_types[] (STR, AC, HITROLL, ...)", line);
					return FALSE;
				}
				p = sp_qval(p, val, sizeof(val));
				if (!*val)
				{
					sp_err(vnum, "apply needs a modifier after the apply type", line);
					return FALSE;
				}
				a->amod = atoi(val);
			}
			else if (!strcmp(word, "aff"))
			{
				a->affbit = sp_aff_by_name(val, &a->affword);
				if (!a->affword)
				{
					sp_err(vnum, "unknown AFF bit -- see affected1_bits..affected5_bits (BLIND, HASTE, ...)", line);
					return FALSE;
				}
			}
			else
			{
				sp_err(vnum, "unknown affect keyword -- want on / save / apply / aff", line);
				return FALSE;
			}
			while (*p == ' ')
				p++;
		}
		if (!isaff && (a->apply || a->affword))
		{
			sp_err(vnum, "apply / aff are only meaningful on 'affect', not 'unaffect'", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "do"))
	{
		a->op = SP_A_DO;
		sp_word(p, word, sizeof(word));
		sp_strlower(word);
		if (!strcmp(word, "self") || !strcmp(word, "actor"))
		{
			a->who = !strcmp(word, "self") ? SP_WHO_SELF : SP_WHO_ACTOR;
			p      = sp_word(p, word, sizeof(word));
			while (*p == ' ')
				p++;
		}
		else
			a->who = SP_WHO_SELF;
		if (!*p)
		{
			sp_err(vnum, "do needs a command line", line);
			return FALSE;
		}
		a->text = str_dup(p);
	}
	else if (!strcmp(word, "set") || !strcmp(word, "add"))
	{
		/* sp_word, not one_argument: a counter legitimately named "on",
		   "to" or "the" would otherwise vanish and the NEXT token would be
		   interned as the counter name.  See the comment on sp_word(). */
		a->op   = !strcmp(word, "set") ? SP_A_SET : SP_A_ADD;
		p       = sp_word(p, name, sizeof(name));
		a->slot = sp_intern_counter(name);
		if (a->slot < 0)
		{
			sp_err(vnum, "counter name table full or empty name", line);
			return FALSE;
		}
		sp_word(p, word, sizeof(word));
		a->num = atoi(word);
	}
	else if (!strcmp(word, "oneof"))
	{
		a->op    = SP_A_ONEOF;
		a->count = atoi(p);
		if (a->count < 2)
		{
			sp_err(vnum, "oneof needs a pool size of 2 or more", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "exit"))
	{
		a->op  = SP_A_EXIT;
		p      = sp_word(p, word, sizeof(word));
		a->num = atoi(word);
		if (a->num <= 0 || real_room(a->num) < 0)
		{
			sp_err(vnum, "exit needs an existing room vnum", line);
			return FALSE;
		}
		p       = sp_word(p, word, sizeof(word));
		a->slot = sp_dir_by_name(word);
		if (a->slot < 0)
		{
			sp_err(vnum, "exit needs a direction", line);
			return FALSE;
		}
		while (*p)
		{
			p = sp_kv(p, word, sizeof(word), val, sizeof(val));
			if (!*word)
				break;
			if (!strcmp(word, "to"))
			{
				sp_strlower(val);
				a->num2 = !strcmp(val, "none") ? -1 : atoi(val);
			}
			else if (!strcmp(word, "state"))
			{
				sp_strlower(val);
				a->state = !strcmp(val, "open")   ? 0
				         : !strcmp(val, "closed") ? (int)EX_CLOSED
				         : !strcmp(val, "locked") ? (int)(EX_CLOSED | EX_LOCKED)
				         : !strcmp(val, "secret") ? (int)(EX_CLOSED | EX_SECRET)
				                                  : -1;
			}
			while (*p == ' ')
				p++;
		}
	}
	else if (!strcmp(word, "heal"))
	{
		if (targ != SP_T_MOB || t->event == SP_EV_DEATH)
		{
			sp_err(vnum, "heal is only valid for living mob targets", line);
			return FALSE;
		}
		a->op = SP_A_HEAL;
		sp_word(p, word, sizeof(word));
		sp_strlower(word);
		if (!strcmp(word, "full"))
			a->num = -1;
		else if ((n = atoi(word)) > 0)
			a->num = n;
		else
		{
			sp_err(vnum, "heal needs an amount or 'full'", line);
			return FALSE;
		}
	}
	else if (!strcmp(word, "block"))
		a->op = SP_A_BLOCK;
	else if (!strcmp(word, "purge"))
	{
		if (targ == SP_T_ROOM)
		{
			sp_err(vnum, "purge is not valid for room targets", line);
			return FALSE;
		}
		if (t->event == SP_EV_DEATH || t->event == SP_EV_HPBELOW || t->event == SP_EV_DAMAGED)
		{
			sp_err(vnum, "purge is not allowed for DEATH/HPBELOW/DAMAGED triggers", line);
			return FALSE;
		}
		a->op = SP_A_PURGE;
	}
	else
	{
		sp_err(vnum, "unknown action", line);
		return FALSE;
	}

	t->num_actions++;
	return TRUE;
}

static int sp_parse_record(FILE *fl, struct sp_rec *rec)
{
	struct sp_trig *t = NULL, *tail = NULL;
	char            buf[MAX_STRING_LENGTH];
	int             in_actions = 0, saw_chance = 0;

	while (sp_gets(fl, buf, sizeof(buf)))
	{
		if (!buf[0] || buf[0] == '*')
			continue;

		if (!in_actions)
		{
			if (buf[0] == 'S' && (buf[1] == '\0' || buf[1] == ' '))
				return (rec->num_trigs > 0);
			if (buf[0] == 'T' && buf[1] == ' ')
			{
				if (rec->num_trigs >= SP_MAX_TRIGS)
				{
					sp_err(rec->vnum, "too many triggers in one record", buf);
					sp_skip_record(fl);
					return FALSE;
				}
				if (!(t = sp_parse_event(rec->target, rec->vnum, buf)))
				{
					sp_skip_record(fl);
					return FALSE;
				}
				t->trig_index = rec->num_trigs;
				if (tail)
					tail->next = t;
				else
					rec->trigs = t;
				tail = t;
				rec->num_trigs++;
				rec->events |= (1u << t->event);
				in_actions = 1;
				saw_chance = 0;
				continue;
			}
			sp_err(rec->vnum, "expected T, S or *comment", buf);
			sp_skip_record(fl);
			return FALSE;
		}

		if (buf[0] == '~' && buf[1] == '\0')
		{
			if (!t->num_actions)
			{
				sp_err(rec->vnum, "trigger has no actions", buf);
				sp_skip_record(fl);
				return FALSE;
			}
			in_actions = 0;
			continue;
		}
		if (!saw_chance && !t->num_actions && !strn_cmp(buf, "chance ", 7))
		{
			int c = atoi(buf + 7);

			if (c < 1 || c > 100)
			{
				sp_err(rec->vnum, "chance must be 1-100", buf);
				sp_skip_record(fl);
				return FALSE;
			}
			t->chance  = c;
			saw_chance = 1;
			continue;
		}
		if (!t->num_actions && !strn_cmp(buf, "if ", 3))
		{
			if (!sp_parse_cond(rec->vnum, t, buf))
			{
				sp_skip_record(fl);
				return FALSE;
			}
			continue;
		}
		if (!sp_parse_action(rec->target, rec->vnum, t, buf))
		{
			sp_skip_record(fl);
			return FALSE;
		}
	}
	sp_err(rec->vnum, "unexpected end of file inside record", "");
	return FALSE;
}

/* ------------------------------------------------------------------ */
/* boot                                                               */
/* ------------------------------------------------------------------ */

/* install the generic proc, remembering (never clobbering) any C proc
   this vnum already had */
static void sp_bind(struct sp_rec *rec)
{
	int rn;

	switch (rec->target)
	{
		case SP_T_MOB:
			if ((rn = real_mobile(rec->vnum)) < 0)
				return;
			if (mob_index[rn].func.mob && mob_index[rn].func.mob != studioproc_mob)
				rec->prev_mob = mob_index[rn].func.mob;
			mob_index[rn].func.mob = studioproc_mob;
			break;
		case SP_T_OBJ:
			if ((rn = real_object(rec->vnum)) < 0)
				return;
			if (obj_index[rn].func.obj && obj_index[rn].func.obj != studioproc_obj)
				rec->prev_obj = obj_index[rn].func.obj;
			obj_index[rn].func.obj = studioproc_obj;
			break;
		default:
			if ((rn = real_room(rec->vnum)) < 0)
				return;
			if (world[rn].funct && world[rn].funct != studioproc_room)
				rec->prev_room = world[rn].funct;
			world[rn].funct = studioproc_room;
			break;
	}
}

void studioproc_boot(void)
{
	FILE          *fl;
	struct sp_rec *rec;
	char           buf[MAX_STRING_LENGTH];
	char           tc;
	int            vnum, targ, nrec = 0, ok, nbound = 0;
	unsigned int   h;

	sp_main_thread = pthread_self();
	sp_have_thread = 1;

	memset(sp_tab, 0, sizeof(sp_tab));
	memset(sp_tcount, 0, sizeof(sp_tcount));
	studioproc_count = 0;
	sp_lineno        = 0;

	if (!sp_roomctr && top_of_world >= 0)
	{
		CREATE(sp_roomctr, int, SP_ROOM_SLOTS * (top_of_world + 1), MEM_TAG_BUFFER);
		memset(sp_roomctr, 0, sizeof(int) * SP_ROOM_SLOTS * (top_of_world + 1));
	}

	if (!(fl = fopen(STUDIOPROC_FILE, "r")))
	{
		logit(LOG_STATUS, "STUDIOPROC: no %s, proc engine idle.", STUDIOPROC_FILE);
		return;
	}

	while (sp_gets(fl, buf, sizeof(buf)))
	{
		if (!buf[0] || buf[0] == '*')
			continue;
		if (!strcmp(buf, "#~"))
			break;
		if (buf[0] != '#' || sscanf(buf, "#%d %c", &vnum, &tc) != 2)
		{
			sp_err(0, "expected #<vnum> <M|O|R> record header", buf);
			if (!sp_skip_record(fl))
				break;
			continue;
		}

		tc   = UPPER(tc);
		targ = (tc == 'M') ? SP_T_MOB : (tc == 'O') ? SP_T_OBJ : (tc == 'R') ? SP_T_ROOM : -1;
		ok   = (targ >= 0 && vnum > 0);
		if (ok)
		{
			if (targ == SP_T_MOB)
				ok = (real_mobile(vnum) >= 0);
			else if (targ == SP_T_OBJ)
				ok = (real_object(vnum) >= 0);
			else
				ok = (real_room(vnum) >= 0);
		}
		if (!ok)
		{
			sp_err(vnum, "bad target letter or vnum not in database", buf);
			if (!sp_skip_record(fl))
				break;
			continue;
		}

		CREATE(rec, struct sp_rec, 1, MEM_TAG_BUFFER);
		memset(rec, 0, sizeof(*rec));
		rec->target = targ;
		rec->vnum   = vnum;

		if (!sp_parse_record(fl, rec))
		{
			sp_free_rec(rec);
			continue;
		}

		h                = ((unsigned int)vnum) & (SP_HASH - 1);
		rec->next        = sp_tab[targ][h];
		sp_tab[targ][h]  = rec;
		sp_tcount[targ] += rec->num_trigs;
		studioproc_count += rec->num_trigs;
		nrec++;
		sp_bind(rec);
		nbound++;
	}
	fclose(fl);

	logit(LOG_STATUS, "STUDIOPROC: %d records, %d triggers, %d bound (%d mob, %d obj, %d room), %d counters.", nrec, studioproc_count, nbound, sp_tcount[SP_T_MOB],
	      sp_tcount[SP_T_OBJ], sp_tcount[SP_T_ROOM], sp_ncounters);
	fprintf(stderr, "--    STUDIOPROC: %d records, %d triggers.\r\n", nrec, studioproc_count);
	/* the HOUR timer is armed lazily, from the first dispatch - see
	   sp_arm_hour().  The event pool does not exist yet at this point. */
}
