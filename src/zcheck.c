/*
 * ***************************************************************************
 *   File: zcheck.c                                           Part of Duris
 *   Usage: zcheck [zone#] - read-only zone audit for builders.
 *
 *   An immortal command that answers, from the BOOTED world state, the
 *   questions a builder cannot eyeball on a 300-room zone: which rooms no
 *   player can walk to, which rooms trap a player who enters, which two-way
 *   passages lost their return exit, which reset-locked doors or blocked
 *   walls nothing can open, and which reset commands can never do what they
 *   say.  It is the runtime twin of the Duris Studio "Zone QA" report: the
 *   studio validates the AREA FILES before install; zcheck validates the
 *   LIVE result after boot, where the engine holds the ground truth the
 *   studio can only mirror (real exits after renum_world, real reset
 *   placements, real key / lock / pickproof bits).
 *
 *   STRICTLY READ-ONLY.  This file walks world[], zone_table[].cmd,
 *   obj_index[] and object_list and WRITES NOTHING: no world mutation, no
 *   resets, no file IO.  Work is bounded to one zone per invocation (plus
 *   one linear pass over object_list for the switch scan and one breadth-
 *   first walk over already-loaded rooms).
 *
 *   THE ENGINE FACTS EACH CHECK RESTS ON (measured on this tree):
 *     - renum_world (db.c:1389) turns every exit's to_room into a REAL room
 *       index and FREES dangling exits outright (db.c:1400-1403), so every
 *       dir_option seen here is walkable truth.
 *     - renum_zone_table (db.c:1408) turns reset args into real indexes and
 *       disables any command whose vnum did not resolve ('!' at
 *       db.c:1452-1469); reset_zone disables more at first run.  zcheck
 *       audits what remains live.
 *     - a door's PICK class comes from the .wld door-info, masked & 3 in
 *       setup_dir (db.c:1370): 0 = not a door at all, 1 = plain door,
 *       2 = EX_PICKABLE (compat only), 3 = EX_PICKPROOF (db.c:1374-1378).
 *     - a door's LOCKED / SECRET / BLOCKED state is reset choreography: the
 *       zone's D commands, applied IN ORDER by reset_zone (db.c:3604-3634;
 *       the last arg3 & 3 wins, the +4 secret / +8 blocked bits only ever
 *       accumulate), so zcheck folds duplicate D rows exactly that way.
 *     - the door-opening modes and their refusals:
 *         key       has_key() on EXIT->key (actmove.c:2476); do_unlock
 *                   refuses mortals when key < 0 ("no keyholes",
 *                   actmove.c:2708) - and a key vnum no object prototype
 *                   carries (real_object() == -1) can never be held.
 *         pick      needs EX_ISDOOR (actmove.c:2879), a keyhole (key >= 0,
 *                   actmove.c:2889), and not EX_PICKPROOF (actmove.c:2892).
 *         doorbash  racial innate; bounces off EX_PICKPROOF, a to_room of
 *                   NOWHERE, or a "_nobash_" door keyword (innates.c:2852).
 *         passdoor  AFF2_PASSDOOR walks a closed door unless it is LOCKED
 *                   *and* EX_PICKPROOF (actmove.c:1208).
 *       So EX_PICKPROOF with no obtainable key kills pick, doorbash and
 *       passdoor at once - that is the "unwinnable" tier.
 *     - a BLOCKED wall's only data-driven opener is an ITEM_SWITCH object:
 *       item_switch (specs.object.c:1331) requires the blocked bit
 *       (specs.object.c:1382 "Nothing happens.") and clears EX_BLOCKED on
 *       the near side only (specs.object.c:1405).  value[1] is the target
 *       room VNUM, value[2] the direction (specs.object.c:1361/:1368).
 *       do_search cannot even reveal a secret exit while it is blocked
 *       (actobj.c:5849 skips EX_BLOCKED).
 *     - reset choreography: an E (or G) before any M/Y/F/R has no mob to
 *       receive the object - reset_zone reads the object anyway and then
 *       only logs (db.c:3471/:3482): the item is left in limbo, never
 *       equipped.  A P into a non-container is refused by obj_can_nest
 *       (handler.c:2519; the container-type test at handler.c:2530-2531):
 *       obj_to_obj returns without placing (handler.c:2559) and only writes
 *       the exit log (utility.c:738-741 - log-only, nothing aborts), so the
 *       object silently never loads.  A P whose container has no live
 *       instance no-ops the same way: get_obj_num (handler.c:2013) returns
 *       NULL at reset time.
 *
 *   WHAT ZCHECK DELIBERATELY DOES NOT CLAIM: compiled C procs (specs.*.c)
 *   and studioproc (.trg) triggers can open doors and walls at runtime;
 *   zcheck scans neither.  Findings therefore say what the ZONE DATA
 *   cannot do - server code could still do more, and the report says so.
 *
 *   Registration follows do_zlist / do_rlist (the builder list commands
 *   this one lives beside): CMD_Y(..., IMMORTAL, FALSE) in interp.c, the
 *   same IS_NPC and PLR_PAGING_ON guards, output through the paged
 *   send_to_char stream.
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <vector>

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "utility.h"
#include "utils.h"

/* * external variables */

extern P_index           mob_index;
extern P_index           obj_index;
extern P_obj             object_list;
extern P_room            world;
extern const char       *dirs[];
extern const int         rev_dir[];
extern const int         top_of_world;
extern int               top_of_zone_table;
extern struct zone_data *zone_table;

/* how many finding lines one section may print before eliding */
#define ZC_MAX_LINES 30

/* one folded door-state per (room, dir) the zone's D commands touch:
 * reset_zone applies D rows in order (db.c:3604), the last & 3 wins and
 * the +4 / +8 bits accumulate - folded the same way here. */
struct zc_door_state
{
	int room; /* REAL room index (renum_zone_table)   */
	int dir;
	int lock;    /* last arg3 & 3                     */
	int secret;  /* any arg3 & 4                      */
	int blocked; /* any arg3 & 8                      */
};

/* one live ITEM_SWITCH instance, collected in a single object_list pass */
struct zc_switch
{
	int vnum;      /* prototype vnum                       */
	int room_rnum; /* real index of value[1], or -1        */
	int dir;       /* value[2]                             */
};

static const char *zc_dir_name(int dir)
{
	if (dir < 0 || dir >= NUM_EXITS)
		return "?dir?";
	return dirs[dir];
}

/* room vnum for a real index, defensively */
static int zc_vnum(int rnum)
{
	if (rnum < 0 || rnum > top_of_world)
		return -1;
	return world[rnum].number;
}

/* resolve the zone argument: no arg = the zone the wizard stands in; a
 * number matches zone_table[].number first (the builder-facing zone
 * number zlist shows), then falls back to the table index rlist uses. */
static int zc_resolve_zone(P_char ch, const char *arg)
{
	int i, n;

	if (!*arg)
	{
		if (ch->in_room < 0 || ch->in_room > top_of_world)
			return -1;
		return world[ch->in_room].zone;
	}
	if (!is_number(const_cast<char *>(arg)))
		return -1;
	n = atoi(arg);
	for (i = 0; i <= top_of_zone_table; i++)
		if (zone_table[i].number == n)
			return i;
	if (n >= 0 && n <= top_of_zone_table)
		return n;
	return -1;
}

/* ================================================================== *
 * do_zcheck - the audit                                              *
 * ================================================================== */
void do_zcheck(P_char ch, char *argument, int cmd)
{
	char arg[MAX_INPUT_LENGTH];
	int  i, r, d, zn;

	if (IS_NPC(ch))
	{
		return;
	}

	if (!IS_SET(ch->specials.act, PLR_PAGING_ON))
	{
		send_to_char("&+WThe zone report can be long, please tog page on to read it.\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg && (!str_cmp(arg, "?") || !str_cmp(arg, "help")))
	{
		send_to_char("&+WUsage:&n\r\n"
		             "  zcheck            audit the zone you are standing in\r\n"
		             "  zcheck <zone#>    audit that zone (zone number as in zlist;\r\n"
		             "                    a non-matching number is tried as a zone index)\r\n",
		             ch);
		return;
	}

	zn = zc_resolve_zone(ch, arg);
	if (zn < 0 || zn > top_of_zone_table)
	{
		send_to_char("No such zone - zcheck <zone#> takes a zone number as shown by zlist.\r\n", ch);
		return;
	}

	const zone_data &zd  = zone_table[zn];
	const int        rb  = zd.real_bottom;
	const int        rt  = zd.real_top;

	if (rb < 0 || rt < rb)
	{
		send_to_char("That zone has no rooms loaded - nothing to audit.\r\n", ch);
		return;
	}

	send_to_char_f(ch, "/== &+WZCHECK&n: zone &+Y%d&n [index %d] %s&n ==\\\r\n", zd.number, zn, zd.name ? zd.name : "(unnamed)");
	send_to_char_f(ch, "rooms #%d..#%d (%d rooms), file '%s'\r\n", zc_vnum(rb), zc_vnum(rt), rt - rb + 1, zd.filename ? zd.filename : "?");

	const int in_zone_lo = rb, in_zone_hi = rt;
	auto      in_zone    = [&](int rnum) { return rnum >= in_zone_lo && rnum <= in_zone_hi; };

	/* ---------------------------------------------------------------- *
	 * ENTRANCES - rooms of this zone that some OTHER zone's room exits
	 * into.  renum_world already freed every dangling exit
	 * (db.c:1400-1403), so each dir_option here is a real, walkable edge.
	 * ---------------------------------------------------------------- */
	std::vector<char> is_entrance((size_t)(top_of_world + 1), 0);
	std::vector<int>  entrances;

	for (r = 0; r <= top_of_world; r++)
	{
		if (in_zone(r))
			continue;
		for (d = 0; d < NUM_EXITS; d++)
		{
			const room_direction_data *ex = world[r].dir_option[d];
			if (!ex)
				continue;
			const int to = ex->to_room;
			if (to < 0 || to > top_of_world || !in_zone(to))
				continue;
			if (!is_entrance[to])
			{
				is_entrance[to] = 1;
				entrances.push_back(to);
			}
		}
	}

	if (entrances.empty())
	{
		send_to_char_f(ch, "Entrances: &+ynone found&n - no other zone exits into this one; starting the walk at the lowest room #%d instead.\r\n", zc_vnum(rb));
	}
	else
	{
		char buf[MAX_STRING_LENGTH];
		int  len = snprintf(buf, sizeof(buf), "Entrances (cross-zone ways in): ");
		for (i = 0; i < (int)entrances.size() && i < 8; i++)
			len += snprintf(buf + len, sizeof(buf) - (size_t)len, "%s#%d", i ? ", " : "", zc_vnum(entrances[i]));
		if ((int)entrances.size() > 8)
			len += snprintf(buf + len, sizeof(buf) - (size_t)len, " and %d more", (int)entrances.size() - 8);
		snprintf(buf + len, sizeof(buf) - (size_t)len, "\r\n");
		send_to_char(buf, ch);
	}

	/* ---------------------------------------------------------------- *
	 * REACHABILITY - directed breadth-first walk over the whole loaded
	 * world from the entrance set (or the lowest room), following every
	 * live exit.  Doors are traversable for reachability - closed,
	 * locked, secret or blocked, a player can still open, unlock, search
	 * or switch them; door state matters to the LOCK report, not here.
	 * The walk may leave the zone and come back: a room reachable only
	 * via a detour through a neighbour zone is correctly reachable.
	 * ---------------------------------------------------------------- */
	std::vector<char> reach((size_t)(top_of_world + 1), 0);
	{
		std::vector<int> queue;
		queue.reserve((size_t)(rt - rb + 1));
		if (entrances.empty())
		{
			reach[(size_t)rb] = 1;
			queue.push_back(rb);
		}
		else
		{
			for (int e : entrances)
			{
				reach[(size_t)e] = 1;
				queue.push_back(e);
			}
		}
		for (size_t head = 0; head < queue.size(); head++)
		{
			const int at = queue[head];
			for (d = 0; d < NUM_EXITS; d++)
			{
				const room_direction_data *ex = world[at].dir_option[d];
				if (!ex)
					continue;
				const int to = ex->to_room;
				if (to < 0 || to > top_of_world || reach[(size_t)to])
					continue;
				reach[(size_t)to] = 1;
				queue.push_back(to);
			}
		}
	}

	int high_findings = 0, low_findings = 0;

	/* ---------------------------------------------------------------- *
	 * 1. UNREACHABLE ROOMS
	 * ---------------------------------------------------------------- */
	{
		int shown = 0, total = 0;
		send_to_char("\r\n&+W1. UNREACHABLE ROOMS&n (no forward exit-path from the entrances)\r\n", ch);
		for (r = rb; r <= rt; r++)
		{
			if (reach[(size_t)r])
				continue;
			total++;
			low_findings++;
			if (shown < ZC_MAX_LINES)
			{
				send_to_char_f(ch, "   #%-7d %s&n\r\n", zc_vnum(r), world[r].name ? world[r].name : "(unnamed)");
				shown++;
			}
		}
		if (total > shown)
			send_to_char_f(ch, "   ... and %d more\r\n", total - shown);
		if (!total)
			send_to_char("   none - every room in the zone can be walked to.\r\n", ch);
	}

	/* ---------------------------------------------------------------- *
	 * 2. ONE-WAY TRAPS - a REACHABLE room whose exits lead to no other
	 * room (none at all, or only a loop back to itself): a player can
	 * walk in and never walk out.  Unreachable sinks are already section
	 * 1 findings, not two.
	 * ---------------------------------------------------------------- */
	{
		int shown = 0, total = 0;
		send_to_char("\r\n&+W2. ONE-WAY TRAPS&n (enterable, no exit out)\r\n", ch);
		for (r = rb; r <= rt; r++)
		{
			if (!reach[(size_t)r])
				continue;
			bool out = false, any = false;
			for (d = 0; d < NUM_EXITS && !out; d++)
			{
				const room_direction_data *ex = world[r].dir_option[d];
				if (!ex)
					continue;
				any = true;
				if (ex->to_room != r)
					out = true;
			}
			if (out)
				continue;
			total++;
			high_findings++;
			if (shown < ZC_MAX_LINES)
			{
				send_to_char_f(ch, "   &+R#%-7d&n %s&n - %s\r\n", zc_vnum(r), world[r].name ? world[r].name : "(unnamed)",
				               any ? "its only exit loops back to itself" : "it has no exit at all");
				shown++;
			}
		}
		if (total > shown)
			send_to_char_f(ch, "   ... and %d more\r\n", total - shown);
		if (!total)
			send_to_char("   none.\r\n", ch);
	}

	/* ---------------------------------------------------------------- *
	 * Fold this zone's D reset rows once - both the RECIPROCITY check
	 * (secret / blocked one-ways are deliberate) and the LOCK report
	 * (section 4) read them.
	 * ---------------------------------------------------------------- */
	std::vector<zc_door_state> doors;
	int                        disabled_cmds = 0;
	bool                       have_cmds     = (zd.cmd != NULL);

	if (have_cmds)
		for (i = 0; zd.cmd[i].command != 'S'; i++)
		{
			const reset_com &rc = zd.cmd[i];
			if (rc.command == '!')
			{
				disabled_cmds++;
				continue;
			}
			if (rc.command != 'D')
				continue;
			if (rc.arg1 < 0 || rc.arg1 > top_of_world)
				continue; /* renum disabled these; defensive */
			zc_door_state *slot = NULL;
			for (auto &ds : doors)
				if (ds.room == rc.arg1 && ds.dir == rc.arg2)
				{
					slot = &ds;
					break;
				}
			if (!slot)
			{
				doors.push_back(zc_door_state());
				slot       = &doors.back();
				slot->room = rc.arg1;
				slot->dir  = rc.arg2;
				slot->lock = slot->secret = slot->blocked = 0;
			}
			slot->lock = rc.arg3 & 0x03; /* last one wins (db.c:3612)   */
			if (rc.arg3 & 0x04)
				slot->secret = 1; /* accumulates (db.c:3629)     */
			if (rc.arg3 & 0x08)
				slot->blocked = 1; /* accumulates (db.c:3631)     */
		}

	auto reset_bits = [&](int rnum, int dir, int *secret, int *blocked)
	{
		*secret  = 0;
		*blocked = 0;
		for (const auto &ds : doors)
			if (ds.room == rnum && ds.dir == dir)
			{
				*secret  = ds.secret;
				*blocked = ds.blocked;
				return;
			}
	};

	/* ---------------------------------------------------------------- *
	 * 3. RECIPROCITY GAPS - for each intra-zone exit A --dir--> B, B
	 * should hold the reverse-direction exit back to A.  Only an
	 * ORDINARY passage whose reverse slot is simply EMPTY is flagged;
	 * the deliberate kinds stay silent:
	 *   - B's reverse slot is occupied by an exit elsewhere: the
	 *     builder's one-way mark.
	 *   - B returns to A by another direction: twisty but linked.
	 *   - the forward exit is SECRET or BLOCKED (live bit or this
	 *     zone's D reset): a hidden / one-way passage by design.
	 * ---------------------------------------------------------------- */
	{
		int shown = 0, total = 0;
		send_to_char("\r\n&+W3. RECIPROCITY GAPS&n (ordinary passage, empty return slot)\r\n", ch);
		for (r = rb; r <= rt; r++)
			for (d = 0; d < NUM_EXITS; d++)
			{
				const room_direction_data *ex = world[r].dir_option[d];
				if (!ex)
					continue;
				const int b = ex->to_room;
				if (b == r || b < 0 || b > top_of_world || !in_zone(b))
					continue;
				const int                  rd   = rev_dir[d];
				const room_direction_data *back = world[b].dir_option[rd];
				if (back && back->to_room == r)
					continue; /* reciprocal - fine        */
				if (back)
					continue; /* slot taken - deliberate  */
				bool returns = false;
				for (int bd = 0; bd < NUM_EXITS && !returns; bd++)
				{
					const room_direction_data *be = world[b].dir_option[bd];
					if (be && be->to_room == r)
						returns = true;
				}
				if (returns)
					continue; /* twisty but linked        */
				int rs, rblk;
				reset_bits(r, d, &rs, &rblk);
				if ((ex->exit_info & (EX_SECRET | EX_BLOCKED)) || rs || rblk)
					continue; /* hidden or walled: deliberate one-way */
				total++;
				low_findings++;
				if (shown < ZC_MAX_LINES)
				{
					send_to_char_f(ch, "   #%d --%s--> #%d, but #%d has no %s exit back\r\n",
					               zc_vnum(r), zc_dir_name(d), zc_vnum(b), zc_vnum(b), zc_dir_name(rd));
					shown++;
				}
			}
		if (total > shown)
			send_to_char_f(ch, "   ... and %d more\r\n", total - shown);
		if (!total)
			send_to_char("   none - every ordinary passage has its return.\r\n", ch);
	}

	/* ---------------------------------------------------------------- *
	 * Collect every live ITEM_SWITCH once (read_object auto-binds the
	 * item_switch proc to the type, db.c:2850-2851; the target room is
	 * value[1] as a VNUM, the direction value[2] - specs.object.c:1361,
	 * :1368).  One linear pass; a switch in a bag or on a mob counts,
	 * because a carried switch works when pulled in the target room
	 * (specs.object.c:1394-1401) and a floor switch works from its own
	 * room (specs.object.c:1386-1393).
	 * ---------------------------------------------------------------- */
	std::vector<zc_switch> switches;
	for (P_obj o = object_list; o; o = o->next)
	{
		if (o->type != ITEM_SWITCH)
			continue;
		zc_switch sw;
		sw.vnum      = obj_index[o->R_num].virtual_number;
		sw.room_rnum = real_room(o->value[1]);
		sw.dir       = o->value[2];
		switches.push_back(sw);
	}

	/* ---------------------------------------------------------------- *
	 * 4. LOCKED DOORS AND BLOCKED WALLS - enumerated from this zone's
	 * own D resets (the builder's choreography; a door a player locked
	 * by hand is not).  Severity = which of the engine's opening modes
	 * remain (the header table).
	 * ---------------------------------------------------------------- */
	{
		int shown = 0, listed = 0;
		send_to_char("\r\n&+W4. LOCKED DOORS / BLOCKED WALLS&n (from this zone's D resets)\r\n", ch);
		/* pair-dedupe: both sides of one door usually carry a D row each;
		 * report one line when both sides land on the same verdict AND the
		 * same key - sides with different wld facts are different doors. */
		struct zc_pair
		{
			int a, b, key, code;
		};
		std::vector<zc_pair> seen_pairs;

		for (const auto &ds : doors)
		{
			if (ds.dir < 0 || ds.dir >= NUM_EXITS)
				continue; /* section 5 reports D-BAD-DIR */
			const room_direction_data *ex = world[ds.room].dir_option[ds.dir];
			if (!ex)
			{
				listed++;
				high_findings++;
				if (shown < ZC_MAX_LINES)
				{
					send_to_char_f(ch, "   [&+RBAD D ROW&n] #%d %s: the D reset targets an exit that does not exist (reset_zone disables it and logs, db.c:3606).\r\n",
					               zc_vnum(ds.room), zc_dir_name(ds.dir));
					shown++;
				}
				continue;
			}

			/* ---- blocked wall (independent of any lock) ---- */
			if (ds.blocked)
			{
				bool found = false;
				for (const auto &sw : switches)
					if (sw.room_rnum == ds.room && sw.dir == ds.dir)
					{
						found = true;
						break;
					}
				if (!found)
				{
					listed++;
					high_findings++;
					if (shown < ZC_MAX_LINES)
					{
						send_to_char_f(ch, "   [&+RSEALED WALL&n] #%d %s: EX_BLOCKED wall and NO live ITEM_SWITCH targets it (item_switch is the only data-driven opener, specs.object.c:1331; it clears the near side only, :1405).%s A compiled proc or an unloaded switch prototype could still exist - zcheck cannot see those.\r\n",
						               zc_vnum(ds.room), zc_dir_name(ds.dir),
						               ds.secret ? " It is also SECRET: search cannot reveal it while blocked (actobj.c:5849)." : "");
						shown++;
					}
				}
			}

			/* ---- locked door mode model ---- */
			if (ds.lock < 2)
				continue;
			listed++;

			const int  key       = ex->key;
			const bool door      = (ex->exit_info & EX_ISDOOR) != 0;
			const bool pickproof = (ex->exit_info & EX_PICKPROOF) != 0;
			const bool nobash    = ex->keyword && isname("_nobash_", ex->keyword);
			const bool key_meant = door && key > 0;
			const bool key_exists = key_meant && real_object(key) != -1;
			const bool pick_ok   = door && !pickproof && key >= 0;
			const bool bash_ok   = door && !pickproof && !nobash && ex->to_room != NOWHERE;
			const bool pass_ok   = !pickproof;
			const bool any_mode  = pick_ok || bash_ok || pass_ok;

			const char *sev = NULL;
			if (key_meant && key_exists)
				continue; /* the intended key path can work: not a finding
				           * (obtainability is the studio's deeper pass)   */
			else if (key_meant && !key_exists && !any_mode)
				sev = "UNWINNABLE";
			else if (key_meant && !key_exists)
				sev = "KEY-BROKEN";
			else if (!any_mode)
				sev = "UNWINNABLE";
			else if (!pick_ok)
				sev = "NARROW";
			else
				continue; /* keyless pickable door: standard thief content */

			/* one line per physical door when both sides agree */
			const int to   = ex->to_room;
			zc_pair   pk;
			pk.a    = (ds.room < to) ? ds.room : to;
			pk.b    = (ds.room < to) ? to : ds.room;
			pk.key  = key;
			pk.code = (sev[0] == 'U' ? 1 : (sev[0] == 'K' ? 2 : 3));
			bool dup = false;
			for (const auto &p : seen_pairs)
				if (p.a == pk.a && p.b == pk.b && p.key == pk.key && p.code == pk.code)
				{
					dup = true;
					break;
				}
			if (dup)
				continue;
			seen_pairs.push_back(pk);

			if (shown >= ZC_MAX_LINES)
				continue;
			shown++;

			char modes[128];
			int  ml    = 0;
			modes[0]   = '\0';
			if (pick_ok)
				ml += snprintf(modes + ml, sizeof(modes) - (size_t)ml, "%spick", ml ? ", " : "");
			if (bash_ok)
				ml += snprintf(modes + ml, sizeof(modes) - (size_t)ml, "%sdoorbash", ml ? ", " : "");
			if (pass_ok)
				ml += snprintf(modes + ml, sizeof(modes) - (size_t)ml, "%spassdoor", ml ? ", " : "");

			if (!strcmp(sev, "UNWINNABLE"))
			{
				high_findings++;
				if (key_meant)
					send_to_char_f(ch, "   [&+RUNWINNABLE&n] #%d %s: resets LOCKED needing key #%d, but no object prototype has that vnum (real_object = -1, so has_key can never match) and it is PICKPROOF (wld door-info 3, db.c:1376), which kills pick (actmove.c:2892), doorbash (innates.c:2852) and passdoor (actmove.c:1208) at once. Only a compiled proc or a studioproc trigger could open it - zcheck scans neither.\r\n",
					               zc_vnum(ds.room), zc_dir_name(ds.dir), key);
				else
					send_to_char_f(ch, "   [&+RUNWINNABLE&n] #%d %s: resets LOCKED and NOTHING can open it - %s%s, and no mode survives: %s. Only a compiled proc or a studioproc trigger could open it - zcheck scans neither.\r\n",
					               zc_vnum(ds.room), zc_dir_name(ds.dir),
					               door ? (key < 0 ? "it has no keyhole (key < 0 refuses unlock, actmove.c:2708, and pick, actmove.c:2889)" : "no key is set")
					                    : "the exit is not even a door (wld door-info 0 - key, pick and doorbash all need EX_ISDOOR)",
					               pickproof ? "; it is PICKPROOF, which also kills doorbash and passdoor" : "",
					               door ? "the lock survives every natural attempt" : "passdoor is refused only on locked+pickproof, but nothing else applies to a non-door");
			}
			else if (!strcmp(sev, "KEY-BROKEN"))
			{
				low_findings++;
				send_to_char_f(ch, "   [&+YKEY-BROKEN&n] #%d %s: needs key #%d but no object prototype has that vnum - the intended path is dead. Still openable by: %s. The D reset re-locks it every repop, so those are per-visit, not a fix.\r\n",
				               zc_vnum(ds.room), zc_dir_name(ds.dir), key, modes);
			}
			else /* NARROW */
			{
				low_findings++;
				send_to_char_f(ch, "   [&+YNARROW&n] #%d %s: locked with no key path (%s)%s - only %s can open it. If that narrow availability is the design, this note is the receipt.\r\n",
				               zc_vnum(ds.room), zc_dir_name(ds.dir),
				               door ? (key < 0 ? "key < 0 = no keyhole, unlock and pick both refused" : "pickproof lock")
				                    : "not a door, so key/pick/doorbash are all refused",
				               nobash ? "; its keyword carries _nobash_, so doorbash bounces (innates.c:2852)" : "",
				               modes);
			}
		}
		if (!listed)
			send_to_char("   none - this zone's D resets leave nothing locked or walled that needs attention.\r\n", ch);
		else if (!shown)
			send_to_char("   every reset-locked door here has a working key path.\r\n", ch);
	}

	/* ---------------------------------------------------------------- *
	 * 5. RESET CHOREOGRAPHY - commands that can never do what they say.
	 * ---------------------------------------------------------------- */
	{
		int  shown = 0, total = 0;
		bool loader_seen = false;
		send_to_char("\r\n&+W5. RESET CHOREOGRAPHY&n\r\n", ch);
		if (have_cmds)
			for (i = 0; zd.cmd[i].command != 'S'; i++)
			{
				const reset_com &rc = zd.cmd[i];
				char             line[MAX_STRING_LENGTH];
				line[0] = '\0';

				switch (rc.command)
				{
					case 'M':
					case 'Y':
					case 'F':
					case 'R':
						loader_seen = true;
						break;

					case 'E':
					case 'G':
						if (rc.arg1 < 0)
							break;
						if (!loader_seen)
							snprintf(line, sizeof(line),
							         "   [&+R%c-NO-MOB&n] cmd %d: %c obj #%d - no M/Y/F/R precedes it, so there is never a mob to receive it; reset_zone still reads the object and only logs (db.c:%s) - it never reaches the world.\r\n",
							         rc.command, i, rc.command, obj_index[rc.arg1].virtual_number,
							         rc.command == 'E' ? "3482, and the loaded copy is left in limbo" : "3411, and the copy is extracted");
						else if (rc.command == 'E' && (rc.arg3 <= 0 || rc.arg3 > CUR_MAX_WEAR))
							snprintf(line, sizeof(line),
							         "   [&+RE-BAD-POS&n] cmd %d: E obj #%d wear-position %d is outside 1..%d - the equip test (db.c:3471) can never pass; the object loads and is left in limbo every reset.\r\n",
							         i, obj_index[rc.arg1].virtual_number, rc.arg3, CUR_MAX_WEAR);
						break;

					case 'P':
					{
						if (rc.arg1 < 0)
							break;
						if (rc.arg3 < 0)
						{
							snprintf(line, sizeof(line),
							         "   [&+RP-NO-CONTAINER&n] cmd %d: P obj #%d - the container prototype does not exist at all; the first reset disables this row (db.c:3355).\r\n",
							         i, obj_index[rc.arg1].virtual_number);
							break;
						}
						P_obj inst = NULL;
						for (P_obj o = object_list; o; o = o->next)
							if (o->R_num == rc.arg3)
							{
								inst = o;
								break;
							}
						if (!inst)
							snprintf(line, sizeof(line),
							         "   [&+YP-NO-INSTANCE&n] cmd %d: P obj #%d into #%d - no live copy of the container exists right now; whenever that is true at reset, get_obj_num (handler.c:2013) returns NULL and the P silently no-ops (the loaded object is left in limbo, db.c:3334).\r\n",
							         i, obj_index[rc.arg1].virtual_number, obj_index[rc.arg3].virtual_number);
						else if (inst->type != ITEM_CONTAINER && inst->type != ITEM_QUIVER && inst->type != ITEM_STORAGE && inst->type != ITEM_CORPSE)
							snprintf(line, sizeof(line),
							         "   [&+RP-NON-CONTAINER&n] cmd %d: P obj #%d into #%d (%s&n, type %d) - not a container: obj_can_nest refuses (handler.c:2530) and obj_to_obj returns without placing (handler.c:2559), writing only the exit log (utility.c:738 - nothing aborts). The object silently never loads.\r\n",
							         i, obj_index[rc.arg1].virtual_number, obj_index[rc.arg3].virtual_number,
							         inst->short_description ? inst->short_description : "?", (int)inst->type);
						break;
					}

					case 'D':
						if (rc.arg2 < 0 || rc.arg2 >= NUM_EXITS)
							snprintf(line, sizeof(line),
							         "   [&+RD-BAD-DIR&n] cmd %d: D room #%d direction %d is outside 0..%d - reset_zone indexes dir_option with it unchecked (db.c:3606).\r\n",
							         i, zc_vnum(rc.arg1), rc.arg2, NUM_EXITS - 1);
						break;

					default:
						break;
				}

				if (line[0])
				{
					total++;
					high_findings++;
					if (shown < ZC_MAX_LINES)
					{
						send_to_char(line, ch);
						shown++;
					}
				}
			}
		if (total > shown)
			send_to_char_f(ch, "   ... and %d more\r\n", total - shown);
		if (!total)
			send_to_char("   none - every reset command can do what it says.\r\n", ch);
		if (disabled_cmds)
			send_to_char_f(ch, "   (note: %d command%s already disabled by the loader at boot - bad vnums, see the boot log.)\r\n",
			               disabled_cmds, disabled_cmds == 1 ? " was" : "s were");
	}

	/* ---------------------------------------------------------------- */
	send_to_char("\r\n", ch);
	if (!high_findings && !low_findings)
		send_to_char_f(ch, "\\== zone %d looks &+Gclean&n ==/\r\n", zd.number);
	else
		send_to_char_f(ch, "\\== zone %d: &+R%d high&n / &+Y%d low&n finding%s ==/\r\n",
		               zd.number, high_findings, low_findings,
		               (high_findings + low_findings) == 1 ? "" : "s");
	send_to_char("Read-only report: nothing was changed. Compiled procs and studioproc\r\n"
	             "triggers are not scanned - they can open more than the data says.\r\n",
	             ch);
}
