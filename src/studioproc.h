/*
   ***************************************************************************
   *  File: studioproc.h                                      Part of Duris *
   *  Usage: data-driven procs for mobs, objects and rooms (world.trg)      *
   *                                                                        *
   *  Reads areas/world.trg at boot and binds ONE generic C proc to every   *
   *  mob / object / room vnum named in it, so that the engine's OWN        *
   *  dispatch (special(), CMD_TOROOM, CMD_GOTHIT, CMD_MOB_COMBAT,          *
   *  CMD_PERIODIC, CMD_DEATH ...) reaches builder-authored behaviour       *
   *  with no further hooks.                                                *
   *                                                                        *
   *  DESIGN LAW: this file contains PRIMITIVES only.  A "proc" is DATA.    *
   *  If a new behaviour needs a new C function, the primitive set is       *
   *  wrong - push the generality down into the primitive instead.          *
   *                                                                        *
   *  THREADING: every entry point below runs on the MAIN GAME THREAD only. *
   *  Nothing here is reachable from sql_pool / persistence_queue /         *
   *  locker_async workers (those only consume sealed SQL strings and never *
   *  touch P_char / P_obj / world[]).  studioproc_boot() records the main  *
   *  thread id and every dispatch entry point refuses to run on any other  *
   *  thread, so the invariant is enforced, not merely documented.          *
   ***************************************************************************
 */

#ifndef _STUDIOPROC_H_
#define _STUDIOPROC_H_

#include "structs.h"

#define STUDIOPROC_FILE "areas/world.trg"

/* ---- target prototype types ------------------------------------- */
#define SP_T_MOB   0
#define SP_T_OBJ   1
#define SP_T_ROOM  2
#define SP_NUM_T   3

/* ---- events ------------------------------------------------------ */
/* Event ids and names are frozen: .trg content in the wild depends on
   them.  Add new events before SP_NUM_EV; never renumber. */
#define SP_EV_DEATH    0   /* self died                                */
#define SP_EV_SPEECH   1   /* a PC spoke here, keyword matched         */
#define SP_EV_GIVE     2   /* self (mob) was handed an object          */
#define SP_EV_ENTER    3   /* a char entered self's room               */
#define SP_EV_PULSE    4   /* every <n> mob pulses                     */
#define SP_EV_FIGHT    5   /* one combat round                         */
#define SP_EV_HPBELOW  6   /* hp first fell below <pct> (latched)      */
#define SP_EV_LEAVE    7   /* a char is about to leave self's room     */
#define SP_EV_DAMAGED  8   /* self was hit (melee | spell | any)       */
#define SP_EV_KILL     9   /* self killed someone                      */
#define SP_EV_CMD     10   /* a char typed <verb> here (any command)   */
#define SP_EV_HOUR    11   /* game hour became <n> (or ANY)            */
#define SP_EV_REPOP   12   /* this instance was just created           */
#define SP_EV_SEARCH  13   /* found by 'search' (objects)              */
#define SP_EV_DECAY   14   /* object decay timer expired               */
#define SP_EV_HIT     15   /* self (a wielded object) struck someone   */
#define SP_NUM_EV     16

/* ---- actions (the primitive set) --------------------------------- */
#define SP_A_SAY       0
#define SP_A_EMOTE     1
#define SP_A_ECHO      2
#define SP_A_ZECHO     3
#define SP_A_CAST      4
#define SP_A_MLOAD     5
#define SP_A_OLOAD     6
#define SP_A_GIVE      7
#define SP_A_TRANSFER  8
#define SP_A_GOTO      9
#define SP_A_DAMAGE   10
#define SP_A_HEAL     11
#define SP_A_PURGE    12
#define SP_A_ATTACK   13   /* named attack: dice/type/scope/save/msgs  */
#define SP_A_AFFECT   14   /* apply an affect                          */
#define SP_A_UNAFFECT 15   /* strip an affect                          */
#define SP_A_DO       16   /* invoke ANY engine command as a character */
#define SP_A_SET      17   /* counter = n                              */
#define SP_A_ADD      18   /* counter += n                             */
#define SP_A_ONEOF    19   /* pick 1 of the next <n> actions           */
#define SP_A_EXIT     20   /* retarget / restate an exit               */
#define SP_A_BLOCK    21   /* swallow the triggering command           */
#define SP_A_RSET     22   /* remote: counter = n on another instance  */
#define SP_A_RADD     23   /* remote: counter += n on another instance */

/* ---- conditions --------------------------------------------------- */
#define SP_C_CARRYING  0
#define SP_C_WEARING   1
#define SP_C_CLASS     2
#define SP_C_RACE      3
#define SP_C_LEVEL     4
#define SP_C_ALIGN     5
#define SP_C_SEX       6
#define SP_C_HOUR      7
#define SP_C_HP        8
#define SP_C_GROUP     9
#define SP_C_ROOM     10
#define SP_C_ZONE     11
#define SP_C_AFFECT   12
#define SP_C_COUNTER  13
#define SP_C_PCS      14
#define SP_C_MOBS     15
#define SP_C_CHANCE   16
#define SP_C_ISPC     17
#define SP_C_FIGHTING 18

/* comparison operators for numeric conditions */
#define SP_OP_EQ   0
#define SP_OP_NE   1
#define SP_OP_LT   2
#define SP_OP_LE   3
#define SP_OP_GT   4
#define SP_OP_GE   5

/* who a condition or an action targets */
#define SP_WHO_ACTOR 0
#define SP_WHO_SELF  1
#define SP_WHO_ROOM  2
#define SP_WHO_ALLY  3   /* lowest-hp NPC in the room (self included) */

/* condition search scopes */
#define SP_SC_ROOM  0
#define SP_SC_ZONE  1
#define SP_SC_WORLD 2

/* attack target scopes */
#define SP_SCOPE_ONE      0
#define SP_SCOPE_ROOM     1   /* everyone hostile in the room          */
#define SP_SCOPE_GROUP    2   /* the target's group only               */
#define SP_SCOPE_NOTTANK  3   /* everyone in the room except the tank  */

/* ---- limits ------------------------------------------------------- */
#define SP_MAX_ACTIONS   24   /* actions per trigger                    */
#define SP_MAX_CONDS      8   /* condition lines per trigger            */
#define SP_MAX_TRIGS     32   /* T blocks per record                    */
#define SP_MAX_COUNTERS 250   /* distinct counter NAMES in world.trg    */
#define SP_MIN_PULSE      1   /* PULSE <n> floor, in mob pulses         */
#define SP_MAX_DEPTH      4   /* re-entrancy / 'do' recursion ceiling   */
#define SP_ROOM_SLOTS     8   /* per-room counter slots                 */

/* ---- per-instance state ------------------------------------------
   Counters live as ordinary affects flagged AFFTYPE_STORE, which the
   engine already documents as "used to store data only" (structs.h:125)
   and already uses that way (epic.c, nanny.c, specs.object.c).  This
   buys per-INSTANCE lifetime for free: the affect dies with the mob, so
   there is no pointer-keyed table, no free_char() hook and no chance of
   state bleeding onto a later mob that reused the address.

   The three type ids sit at the very top of the skills[] index space
   (skills[] is declared skills[MAX_AFFECT_TYPES + 1], MAX_AFFECT_TYPES
   == MAX_SKILLS + 200 == 2200, and the TAG_ list currently ends at
   2126).  Taking them from the top means new TAG_ constants can keep
   being appended without ever colliding, and spells.h is not edited. */
#define SP_TAG_COUNTER  2200  /* location = counter slot, modifier = n  */
#define SP_TAG_COOLDOWN 2199  /* location = action idx, modifier = when */
#define SP_TAG_TRIG     2198  /* location = trig idx, modifier = state  */

/* ---- boot / hook entry points ------------------------------------ */

/* db.c boot_db(), after assign_spell_pointers() (the parser resolves
   spell names through spells[], which is empty before that point). */
void studioproc_boot(void);

/* actcomm.c do_say(), last statement: a PC spoke in ch->in_room.  Also
   delivers CMD_SAY to object instance proclibs, which special() never
   does. */
void studioproc_speech(P_char ch, const char *text);

/* actobj.c do_give(), last statement: vict was just handed obj. */
void studioproc_give(P_char vict, P_obj obj, P_char giver);

/* fight.c die(), after the CMD_DEATH block: killer just killed victim. */
void studioproc_kill(P_char killer, P_char victim);

/* ---- the three generic procs ------------------------------------- */
/* Installed into mob_index[].func.mob / obj_index[].func.obj /
   world[].funct at boot.  Never called directly. */
int studioproc_mob(P_char mob, P_char actor, int cmd, char *arg);
int studioproc_obj(P_obj obj, P_char actor, int cmd, char *arg);
int studioproc_room(int room, P_char actor, int cmd, char *arg);

/* total triggers loaded; 0 == engine idle (cheap guard for the hooks) */
extern int studioproc_count;

#endif /* _STUDIOPROC_H_ */
