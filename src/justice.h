/*
 * blah, blah, blah
 */

#ifndef _SOJ_JUSTICE_H_
#define _SOJ_JUSTICE_H_

#ifndef _SOJ_STRUCTS_H_
#include "structs.h"
#endif

#ifndef _SOJ_UTILS_H_
#include "utils.h"
#endif

/* okay...  some "rules".  All justice related macros shall begin with
   J_.  ie:  J_IS_GUARD.  All justice functions shall either begin
   with a j_, or contain the word "justice" in their name.

   */

/* keepers for publi interface:

  void justice_guard_remove(P_char);
     Given an NPC, this does the reverse of justice_register_guard().
     First checks to see if this mob was ever "registered", and if so,
     de-registers it.  Most often would be used in extract_char(), but
     also may be used in other circumstances where this guard is
     unable to come to the "call of duty" (ie: gets charmed).  Note
     that it is safe to call this function on a non-justice mob.

  void justice_action_invader(P_char);
     Given any char, ch, it determines if ch is an "invader" to the
     hometown (if any) they are in.  If so, and if they aren't already
     being dealt with, it deals with them.  Invaders are treated
     simply.  Hunt, and kill.


     */

#undef JUSTICE_DEBUG /* define for lots of debugging spam */

#define JUSTICE_AGGRO /* define to allow justice related attacks */

#define JUSTICE_HOUR 75
#define JUSTICE_DAY  JUSTICE_HOUR * 24

/* returns the town num a character is in.  0 means not in a town.  To
   use the return of this macro in a index of the hometowns[] array,
   subtract 1 */
#define CHAR_IN_TOWN(a)         (zone_table[world[(a)->in_room].zone].hometown)
#define CHAR_IN_JUSTICE_AREA(a) (world[(a)->in_room].justice_area)

/* Max number of guards that should chase each invader... */
#define JUSTICE_MAX_GROUP 5

/* flags for hometowns */
#define JUSTICE_EVILHOME    BIT_1 /* aggro to good races */
#define JUSTICE_GOODHOME    BIT_2 /* aggro to evil races */
#define JUSTICE_LEVEL_HARSH BIT_3 /* no concept of "wanted" just kill them */
#define JUSTICE_LEVEL_CHAOS BIT_4 /* no "justice" at all.  just use invador code */
#define JUSTICE_UNDEADHOME  BIT_5
#define JUSTICE_NEUTRALHOME BIT_6

/* For a given NPC, a, a->npc_only.spec[2] is a set of flags used
   excluseively for justice purposes.  This should be used to let the
   justice code if a mob has any justice purpose, and if so, what it
   is */

#define MOB_SPEC_JUSTICE                                                                                                                                                                               \
	BIT_1 /* I'm controlled by justice as a                                                                                                                                                            \
	         justice guard! */

/* convienent structure used to keep independent linked lists of
   mobs.*/
struct justice_guard_list
{
	P_char                     ch;
	struct justice_guard_list *next;
};

/* An array of of these structures called "hometowns" is created,
   containing one structure for each hometown configured.  */

struct hometown_data
{
	ulong flags;         /* see the JUSTICE_* flags above  */
	int   report_room;   /* where crimes are reported */
	int   guard_room[5]; /* where I'll load new guards */
	int   guard_mob;     /* mob number of the guards I'm using */
	int   jail_room;     /* Take a guess what this is ;) */
};

/* some externs to keep code that includes justice.h from having to
   declare them as well */
extern struct hometown_data hometowns[];
extern const char          *town_name_list[];

/* okay... having a fucking huge list of macros for different flags in
   different city's is just plain fucking stupid.  My way, which is
   considerably harder to read, and therefore better, makes a couple
   of assumptions:
     1) no more then 16 hometowns will ever exist
     2) a player cannot be WANTED, _and_ KNOWN, etc.  Basically, you
        have only one designation for any town

   How does it work?  Well, given the flags below for wanted, known,
   etc, a play can only have 1 of 4 states in a given town.  Those 4
   states can be put into a mere 2 bits.  The first hometown goes into
   the first 2 bits.. the second hometown into the 2nd two, and so on.

   */

#define IS_TOWN_INVADER(a, b) (IS_PC(a) ? PC_INVADER(a, b) : NPC_INVADER(a, b))
// similar to invader, but goods aren't weclome in evil towns (even if not invaders)
#define IS_TOWN_NOTWELCOME(a, b) (IS_PC(a) ? PC_NOTWELCOME(a, b) : NPC_INVADER(a, b))

/* for mobs, I need to make a bunch of macro's to make it easier to
   read.  someof these might be useful elsewhere (ie: VNUM2TOWN) */

#define VNUM2TOWN(a) ((real_room(a) == -1) ? 0 : zone_table[world[real_room(a)].zone].hometown)

#define BORNHERE(a, b) (VNUM2TOWN(GET_BIRTHPLACE(a)) && (VNUM2TOWN(GET_BIRTHPLACE(a)) == b))

/*#define NPC_INVADER(a,b) (IS_NPC(a) && !TRUSTED_NPC(a) && \
                           (!BORNHERE(a,b) || IS_MORPH(a) || \
                          (GET_RNUM(a) >= real_mobile(3) && GET_RNUM(a) <= real_mobile(6))) && \
                          (IS_SET(hometowns[b - 1].flags, \
                                  EVILRACE(a) ? JUSTICE_GOODHOME : \
                                  GOODRACE(a) ? JUSTICE_EVILHOME : 0)))*/

// removed check for dracos, it is performed very ineffectively, and why should alarm sound on just lesser dracos anyways? -Odorf
#define NPC_INVADER(a, b) (IS_NPC(a) && !TRUSTED_NPC(a) && (!BORNHERE(a, b) || IS_MORPH(a)) && (IS_SET(hometowns[b - 1].flags, EVILRACE(a) ? JUSTICE_GOODHOME : GOODRACE(a) ? JUSTICE_EVILHOME : 0)))

#define IS_TOWN_RAIDED(ch) (zone_table[world[ch->in_room].zone].last_raid + 200 > get_time())

/* need a special ma for dealing with elf-town and invaders */

#define IS_ELFIE(a) ((GET_RACE(a) == RACE_GREY) || (GET_RACE(a) == RACE_CENTAUR) || (GET_RACE(a) == RACE_HALFELF))

#define IS_CENTAURIE(a) ((GET_RACE(a) == RACE_GREY) || (GET_RACE(a) == RACE_CENTAUR))

/* below code used to be part of PC_INVADER macro..  was replaced with
   similar IS_SET(hometowns[b - 1]...) line */

/*
                        ((IS_SET(hometowns[b - 1].flags, \
                                  EVIL_RACE(a) ? JUSTICE_GOODHOME : \
                                  JUSTICE_EVILHOME)) || \
*/

#define PC_NOTWELCOME(a, b)                                                                                                                                                                            \
	(IS_PC(a) && !TRUSTED_NPC(a) &&                                                                                                                                                                    \
	 ((IS_SET(hometowns[b - 1].flags, JUSTICE_GOODHOME) && !IS_RACEWAR_GOOD(a)) || (IS_SET(hometowns[b - 1].flags, JUSTICE_EVILHOME) && !IS_RACEWAR_EVIL(a)) ||                                        \
	  (IS_SET(hometowns[b - 1].flags, JUSTICE_UNDEADHOME) && !IS_RACEWAR_UNDEAD(a)) /*                            || (IS_SET(hometowns[b - 1].flags, JUSTICE_NEUTRALHOME) && !RACE_NEUTRAL(a)) */) &&  \
	 !((IS_ELFIE(a) && ((b) == HOME_CHARIN)) || (IS_CENTAURIE(a) && ((b) == HOME_MARIGOT))))

#define PC_INVADER(a, b)                                                                                                                                                                               \
	(IS_PC(a) && !TRUSTED_NPC(a) && !IS_DISGUISE(a) &&                                                                                                                                                 \
	 ((IS_SET(hometowns[b - 1].flags, JUSTICE_GOODHOME) && (!GOOD_RACE(a))) || (IS_SET(hometowns[b - 1].flags, JUSTICE_EVILHOME) && (!EVIL_RACE(a))) || (!IS_ELFIE(a) && ((b) == HOME_CHARIN)) ||      \
	  (!IS_CENTAURIE(a) && ((b) == HOME_MARIGOT))))

/* special macros for justice that also check NPC races! */
#define EVILRACE(a)                                                                                                                                                                                    \
	(IS_RACEWAR_EVIL(a) || (GET_RACE(a) == RACE_DEMON) || (GET_RACE(a) == RACE_DEVIL) || (IS_UNDEAD(a)) || (GET_RACE(a) == RACE_GOBLIN) || (GET_RACE(a) == RACE_DRAGON && !IS_PC_PET(a)) ||            \
	 (GET_RACE(a) == RACE_DRAGON && IS_PC_PET(a) && get_linked_char(a, LNK_PET) != NULL ? GET_RACEWAR(get_linked_char(a, LNK_PET)) == RACEWAR_EVIL : FALSE) || (GET_RACE(a) == RACE_HALFORC) ||        \
	 (GET_RACE(a) == RACE_DEMON))

#define GOODRACE(a) (IS_RACEWAR_GOOD(a))

#define UNDEADRACE(a) (IS_RACEWAR_UNDEAD(a))

/* invaders and outcasts can't hide behind shapechange, so use the
   GET_PLYR() macros. */
#define IS_NOTWELCOME(a) (CHAR_IN_TOWN(a) && IS_TOWN_NOTWELCOME(a, CHAR_IN_TOWN(a)) && !IS_AFFECTED(a, AFF_BOUND))

#define IS_INVADER(a) (CHAR_IN_TOWN(a) && IS_TOWN_INVADER(a, CHAR_IN_TOWN(a)) && !IS_AFFECTED(a, AFF_BOUND))

#endif
