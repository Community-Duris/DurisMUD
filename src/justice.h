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

  void justice_register_guard(P_char);
     Given a NPC, ch, decides if ch is "qualified" to be a justice
     guard, and registers it if so.  NOTE: Put all qualifications in
     this function, and NOT in db.c or elsewhere.  This gives a
     CENTRAL location for changing qualifications, instead of having
     shit spread all over gods creation. (one exception being that
     db.c might only call this function for mobs that load in
     hometowns)

  void justice_guard_remove(P_char);
     Given an NPC, this does the reverse of justice_register_guard().
     First checks to see if this mob was ever "registered", and if so,
     de-registers it.  Most often would be used in extract_char(), but
     also may be used in other circumstances where this guard is
     unable to come to the "call of duty" (ie: gets charmed).  Note
     that it is safe to call this function on a non-justice mob.

  void justice_hunt_cancel(P_char);
     Given an NPC, first determines if this mob is a justice
     registered mob.  If so, and if this guard is currently "on duty
     dealing with something", it stops the mob from being marked as
     "in use", so that it can be re-used for other justice functions.
     Note that this function should be called by
     justice_guard_remove() in order to keep counts straight...

  void justice_victim_remove(P_char);
     Given any char, ch, it finds any justice guards that might be
     "dealing" with them, and calls justice_hunt_cancel() for those
     guards.  This will most often be called when justice is "done"
     with a character (usually by death).

  void justice_action_invader(P_char);
     Given any char, ch, it determines if ch is an "invader" to the
     hometown (if any) they are in.  If so, and if they aren't already
     being dealt with, it deals with them.  Invaders are treated
     simply.  Hunt, and kill.


     */

#undef JUSTICE_DEBUG            /* define for lots of debugging spam */

#define JUSTICE_AGGRO            /* define to allow justice related attacks */

#define LOG_CRIMES "logs/log/crimes"

#define JUSTICE_HOUR 75
#define JUSTICE_DAY  JUSTICE_HOUR * 24

/* room where outcasts end up going */
#define OUTCAST_BIRTH 74124

/* returns the town num a character is in.  0 means not in a town.  To
   use the return of this macro in a index of the hometowns[] array,
   subtract 1 */
#define CHAR_IN_TOWN(a) (zone_table[world[(a)->in_room].zone].hometown)
#define CHAR_IN_JUSTICE_AREA(a) (world[(a)->in_room].justice_area)

/* Max number of guards that should chase each invader... */
#define JUSTICE_MAX_GROUP 5

/* flags for obj justice */
#define J_OBJ_NONE   0
#define J_OBJ_STEAL  1
#define J_OBJ_CORPSE 2

/* flags for hometowns */
#define JUSTICE_EVILHOME       BIT_1 /* aggro to good races */
#define JUSTICE_GOODHOME       BIT_2 /* aggro to evil races */
#define JUSTICE_LEVEL_HARSH    BIT_3 /* no concept of "wanted" just kill them */
#define JUSTICE_LEVEL_CHAOS    BIT_4 /* no "justice" at all.  just use invador code */
#define JUSTICE_UNDEADHOME     BIT_5
#define JUSTICE_NEUTRALHOME    BIT_6

/* for law_flags on PC's.  */
#define JUSTICE_IS_CITIZEN 0
#define JUSTICE_IS_NORMAL  1
#define JUSTICE_IS_CITIZEN_BUY 2
#define JUSTICE_IS_OUTCAST 3
#define JUSTICE_IS_LAST_FLAG 3

/* crimes are numbered 1 - 127.  IF YOU ADD, DELETE OR CHANGE
   CRIMES HERE, ALSO FIX THE crime_list VARIABLE IN JUSTCE.C!  */

#define CRIME_NONE         0

#define CRIME_FAKE_THEFT   0    /* will be remove */
#define CRIME_FAKE_FELON   0    /* will be remove */
#define CRIME_FAKE_OUTCAST 0    /* will be remove */
#define CRIME_LAST_FAKE    0    /* will be remove */
#define CRIME_LAST_NON_VIO 0    /* will be remove */

#define CRIME_ATT_THEFT    1    /* attempted theft */
#define CRIME_THEFT        2    /* theft */
#define CRIME_ATT_MURDER   3    /* attempted murder (aka assault) */
#define CRIME_ESCAPE       4    /* escape from jail or from arrest */
#define CRIME_MURDER       5    /* murder */
#define CRIME_S_WALL       6    /* casting of a wall spell */
#define CRIME_KIDNAPPING   7    /* if trying to capture a non wanted criminal */
#define CRIME_SHAPE_CHANGE 8    /* druid shape change */
#define CRIME_NOT_PAID     9    /* no payment of debt in time */
#define CRIME_AGAINST_TOWN 10   /* crime against town */
#define CRIME_CORPSE_LOOT  11   /* corpse looting */
#define CRIME_CORPSE_DRAG  12   /* corpse dragging */
#define CRIME_DISGUISE     13   /* disguising */
#define CRIME_SUMMON       14   /* summoning spells */

#define CRIME_NB           15    /* number of crimes */

/* status of crimes list in town (TASFALEN)*/
#define J_STATUS_NONE       0
#define J_STATUS_WANTED     1    /* pc wanted for this crime */
#define J_STATUS_DEBT       2    /* pc own money for this crime */
#define J_STATUS_PARDON     3    /* pc is pardon by victim for a crime against him */
#define J_STATUS_CRIME      4    /* crime commited */
#define J_STATUS_FALSE_REP  5    /* false report */
#define J_STATUS_IN_JAIL    6    /* pc in jail, not judge yet */
#define J_STATUS_JAIL_TIME  7    /* pc in jail doing time */
#define J_STATUS_BEEN_JUDGE 8    /* pc got a trial */
#define J_STATUS_DELETED    9    /* record will be erase */
#define J_STATUS_SENTENCE_DEATH 10    /* sentence to death */

/* sentence for crime */
#define SENTENCE_NONE      0    /* none, you are free */
#define SENTENCE_DEBT      1    /* you gonna pay some money */
#define SENTENCE_JAIL      2    /* jail time */
#define SENTENCE_OUTCAST   3    /* outcast */
#define SENTENCE_OUT_NO_EQ 4    /* outcast with lost of eq. */
#define SENTENCE_DEATH     5    /* death */

#define SENTENCE_NB        6     /* number of sentence */

/* For a given NPC, a, a->npc_only.spec[2] is a set of flags used
   excluseively for justice purposes.  This should be used to let the
   justice code if a mob has any justice purpose, and if so, what it
   is */

#define MOB_SPEC_JUSTICE    BIT_1  /* I'm controlled by justice as a
                                      justice guard! */
#define MOB_SPEC_J_REMOVE   BIT_2 /* I need to go to my barraks, and
                                     delete myself */
#define MOB_SPEC_J_OUTCAST  BIT_3 /* I'm doing outcast/invader duty */
#define MOB_SPEC_J_PK       BIT_4 /* I'm heading to a PK attempt */
#define MOB_SPEC_MURDERER   BIT_5 /* I'm dragging a murderer to be
                                     dealt with */
#define MOB_SPEC_ARREST1    BIT_6 /* heading to arrest someone */
#define MOB_SPEC_ARREST2    BIT_7 /* I just arrested someone, and am
                                     dragging them to jail */
#define MOB_SPEC_REPORTING  BIT_8 /* Witness is reporting a crime */
#define MOB_SPEC_GOING_BACK BIT_9 /* Witness is going back to where it was before reporting a crime */
#define MOB_INIT_RANDOM_EQ  BIT_10 /* Witness is going back to where it was before reporting a crime */

/* convienent structure used to keep independent linked lists of
   mobs.*/
struct justice_guard_list {
  P_char ch;
  struct justice_guard_list *next;
};

/* An array of of these structures called "hometowns" is created,
   containing one structure for each hometown configured.  */

struct hometown_data {
  ulong flags;                  /* see the JUSTICE_* flags above  */
  int report_room;              /* where crimes are reported */
  int guard_room[5];               /* where I'll load new guards */
  int guard_mob;                /* mob number of the guards I'm using */
  int jail_room;                /* Take a guess what this is ;) */
  crm_rec *crime_list;          /* towns crime list (TASFALEN) */

  int t_crime_punish[CRIME_NB]; /* level of punishment (hometown)*/
  int p_crime_punish[CRIME_NB]; /* level of punishment (patrol area)*/
  int sentence_min[SENTENCE_NB]; /* mim-max of each sentence */
  int sentence_max[SENTENCE_NB];

};

/* some externs to keep code that includes justice.h from having to
   declare them as well */
extern struct hometown_data hometowns[];
extern const char *town_name_list[];
extern const char *justice_flag_names[];

#define GET_CRIME_T(a,b) (hometowns[a-1].t_crime_punish[b])
#define GET_CRIME_P(a,b) (hometowns[a-1].p_crime_punish[b])
#define SENTENCE_MIN(a,b) (hometowns[a-1].sentence_min[b])
#define SENTENCE_MAX(a,b) (hometowns[a-1].sentence_max[b])

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

#define PC_TOWN_JUSTICE_FLAGS(a,b) ((a->only.pc->law_flags >> (((b) - 1) *2)) & 3)

#define PC_JUSTICE_FLAGS(a) (CHAR_IN_TOWN(a) ? \
                             PC_TOWN_JUSTICE_FLAGS(a, CHAR_IN_TOWN(a)) \
                             : 0)

#define PC_IS_CITIZEN(a) (!PC_JUSTICE_FLAGS(a))

#define NPC_IS_CITIZEN(a) (CHAR_IN_TOWN(a) && IS_NPC(a) && !IS_ANIMAL(a) \
                && GET_HOME(a) == zone_table[world[a->in_room].zone].hometown)

#define IS_CITIZEN(a) (IS_NPC(a) ? NPC_IS_CITIZEN(a) : PC_IS_CITIZEN(a))

#define IS_TOWN_INVADER(a,b) (IS_PC(a) ? PC_INVADER(a,b) : NPC_INVADER(a,b))
// similar to invader, but goods aren't weclome in evil towns (even if not invaders)
#define IS_TOWN_NOTWELCOME(a,b) (IS_PC(a) ? PC_NOTWELCOME(a,b) : NPC_INVADER(a,b))

  /* for mobs, I need to make a bunch of macro's to make it easier to
     read.  someof these might be useful elsewhere (ie: VNUM2TOWN) */


#define VNUM2TOWN(a) ((real_room(a)  == -1) ? 0 : \
                      zone_table[world[real_room(a)].zone].hometown)

#define BORNHERE(a,b) (VNUM2TOWN(GET_BIRTHPLACE(a)) && \
                       (VNUM2TOWN(GET_BIRTHPLACE(a)) == b))

/*#define NPC_INVADER(a,b) (IS_NPC(a) && !TRUSTED_NPC(a) && \
                           (!BORNHERE(a,b) || IS_MORPH(a) || \
                          (GET_RNUM(a) >= real_mobile(3) && GET_RNUM(a) <= real_mobile(6))) && \
                          (IS_SET(hometowns[b - 1].flags, \
                                  EVILRACE(a) ? JUSTICE_GOODHOME : \
                                  GOODRACE(a) ? JUSTICE_EVILHOME : 0)))*/

// removed check for dracos, it is performed very ineffectively, and why should alarm sound on just lesser dracos anyways? -Odorf
#define NPC_INVADER(a,b) (IS_NPC(a) && !TRUSTED_NPC(a) && \
                           (!BORNHERE(a,b) || IS_MORPH(a)) && \
                          (IS_SET(hometowns[b - 1].flags, \
                                  EVILRACE(a) ? JUSTICE_GOODHOME : \
                                  GOODRACE(a) ? JUSTICE_EVILHOME : 0)))


  /* need a special ma for dealing with elf-town and invaders */

#define IS_ELFIE(a) ((GET_RACE(a) == RACE_GREY) || \
                     (GET_RACE(a) == RACE_CENTAUR) || \
                     (GET_RACE(a) == RACE_HALFELF))

#define IS_CENTAURIE(a) ((GET_RACE(a) == RACE_GREY) || \
                         (GET_RACE(a) == RACE_CENTAUR))

/* below code used to be part of PC_INVADER macro..  was replaced with
   similar IS_SET(hometowns[b - 1]...) line */

/*
                        ((IS_SET(hometowns[b - 1].flags, \
                                  EVIL_RACE(a) ? JUSTICE_GOODHOME : \
                                  JUSTICE_EVILHOME)) || \
*/

#define PC_NOTWELCOME(a, b) ( IS_PC(a) && !TRUSTED_NPC(a) \
                            && ( (IS_SET(hometowns[b - 1].flags, JUSTICE_GOODHOME) && !IS_RACEWAR_GOOD(a)) \
                            || (IS_SET(hometowns[b - 1].flags, JUSTICE_EVILHOME) && !IS_RACEWAR_EVIL(a)) \
                            || (IS_SET(hometowns[b - 1].flags, JUSTICE_UNDEADHOME) && !IS_RACEWAR_UNDEAD(a)) \
/*                            || (IS_SET(hometowns[b - 1].flags, JUSTICE_NEUTRALHOME) && !RACE_NEUTRAL(a)) */ ) \
                            && !( (IS_ELFIE(a) && ((b) == HOME_CHARIN)) || \
                             (IS_CENTAURIE(a) && ((b) == HOME_MARIGOT)) ) )

#define PC_INVADER(a,b) (IS_PC(a) && !TRUSTED_NPC(a) && !IS_DISGUISE(a) \
  && ((IS_SET(hometowns[b - 1].flags, JUSTICE_GOODHOME) && (!GOOD_RACE(a))) \
   || (IS_SET(hometowns[b - 1].flags, JUSTICE_EVILHOME) && (!EVIL_RACE(a))) \
   || (!IS_ELFIE(a) && ((b) == HOME_CHARIN)) || (!IS_CENTAURIE(a) && ((b) == HOME_MARIGOT))) )

  /* special macros for justice that also check NPC races! */
#define EVILRACE(a) (IS_RACEWAR_EVIL(a) || \
                     (GET_RACE(a) == RACE_DEMON) || \
                     (GET_RACE(a) == RACE_DEVIL) || \
                     (IS_UNDEAD(a) ) || \
                     (GET_RACE(a) == RACE_GOBLIN) || \
                     (GET_RACE(a) == RACE_DRAGON) || \
                     (GET_RACE(a) == RACE_HALFORC) || \
                     (GET_RACE(a) == RACE_DEMON))

#define GOODRACE(a) (IS_RACEWAR_GOOD(a))

#define UNDEADRACE(a) (IS_RACEWAR_UNDEAD(a))

  /* invaders and outcasts can't hide behind shapechange, so use the
     GET_PLYR() macros. */
#define IS_NOTWELCOME(a) (CHAR_IN_TOWN(a) && \
                       IS_TOWN_NOTWELCOME(a, CHAR_IN_TOWN(a)) && \
                       !IS_AFFECTED(a, AFF_BOUND))

#define IS_INVADER(a) (CHAR_IN_TOWN(a) && \
                       IS_TOWN_INVADER(a, CHAR_IN_TOWN(a)) && \
                       !IS_AFFECTED(a, AFF_BOUND))

  /* special case here for NPC's and outcasts.  In non-chaotic
     hometowns, undead, demons, and dragons are considered outcast */

#define IS_OUTCAST(a) (IS_PC(a) ? (PC_JUSTICE_FLAGS(GET_PLYR(a)) == \
                                   JUSTICE_IS_OUTCAST) : 0)
//                       (CHAR_IN_TOWN(a) && !TRUSTED_NPC(a) && \
                      !IS_SET(hometowns[CHAR_IN_TOWN(a) - 1].flags, JUSTICE_LEVEL_CHAOS) && \
                        (IS_AFFECTED(a, AFF_CHARM) || (a->following && IS_PC(a->following)) || \
                         IS_AGGRESSIVE(a)) && \
                        (IS_DEMON(a) || IS_DRAGON(a))))

#endif


