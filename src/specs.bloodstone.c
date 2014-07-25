/*
   ***************************************************************************
   *  File: specs.bloodstone.c                                 Part of Duris *
   *  Usage: special procedures for bloodstone zones                           *
   *  Copyright  1995 - Duris Systems Ltd.                                   *
   *************************************************************************** 
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"

/*
   external variables 
 */

extern P_desc descriptor_list;
extern P_event current_event;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern const char *dirs[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int planes_room_num[];
extern int racial_base[];
extern int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

/*
   MOBILE PROCS 
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7100
   *Name:Citizen
 */
int bs_citizen(P_char ch, P_char pl, int cmd, char *arg)
{

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Hail stranger!  Tis a fine day, is it not?");
    return TRUE;
  case 1:
    mobsay(ch, "Damn!  Taxes were raised again!");
    return TRUE;
  case 2:
    mobsay(ch, "Have you seen my wife around here anywhere?");
    return TRUE;
  case 3:
    mobsay(ch, "I hear Verzanan is a nice place to visit.");
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_BURP);
    do_action(ch, 0, CMD_BLUSH);
    mobsay(ch, "Excuse me.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7101
   *Name:Common woman
 */
int bs_comwoman(P_char ch, P_char pl, int cmd, char *arg)
{

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Have you been to the garden?");
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "Now where did that boy go?");
    return TRUE;
  case 2:
    act("$n pulls out a shopping list and studies it carefully.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    act("$n looks at you, and winks suggestively.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_BLUSH);
    return TRUE;
  case 4:
    mobsay(ch, "Have you traveled far?  Bloodstone does offer a nice inn.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7102
   *Name:Little brat
 */
int bs_brat(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNICKER);
    mobsay(ch, "I am hiding from my mom!");
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_SNORT);
    mobsay(ch, "Yummy, something big!");
    return TRUE;
  case 2:
    act("$n looks at you and bursts into laughter!", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    act("$n picks $s nose and casually places the treasure in $s mouth.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "My pop could snap your neck instantly!");
    do_action(ch, 0, CMD_STRUT);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7103
   *Name:Holyman
 */
int bs_holyman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n raises $s unholy symbol, and cracks a wicked smile.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    mobsay(ch, "Praise be to &+yHoar&N, &+LThe Doombringer&N!");
    return TRUE;
  case 2:
    act("$n chants 'Fernum Acti Blas' and looks at you with contempt.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    mobsay(ch, "Leave me quickly, before I unleash my dark powers upon you!");
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_KNEEL);
    mobsay(ch, "Oh mighty &+yHoar&N, please grant me your dark powers!");
    act("A bolt of &+Bblue lightning&N strikes the holyman!",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch, "Praise be to &+yHoar&N!");
    do_action(ch, 0, CMD_STAND);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7104
   *Name:merchant
 */
int bs_merchant(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_PONDER);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_SMIRK);
    mobsay(ch, "Peasant trash!");
    return TRUE;
  case 2:
    mobsay(ch, "I must hurry, I have business with Waqar!");
    return TRUE;
  case 3:
    mobsay(ch, "A saber, a black silk sash...hmm what else was there?");
    do_action(ch, 0, CMD_PONDER);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7105    7308
   *Name:wino    wino second
 */
int bs_wino(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodici event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_BURP);
    mobsay(ch, "Ahhh an excellent vintage!");
    return TRUE;
  case 1:
    act("$n kneels down close to the ground and covers $s mouth.",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_PUKE);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_HUM);
    mobsay(ch, "How dry I am, how dry I am..");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "A roasted stirge would hit the spot!");
    do_action(ch, 0, CMD_DROOL);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_BOW);
    mobsay(ch, "At your service!");
    return TRUE;
  case 5:
    act("$n breaks out a bottle and takes a quick swig.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 6:
    mobsay(ch, "A tribute to the Baron!");
    do_action(ch, 0, CMD_FART);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7106
   *Name:watcher
 */
int bs_watcher(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 50))
  {
  case 0:
    mobsay(ch, "Move along, move along.");
    return TRUE;
  case 1:
    mobsay(ch, "Be careful you don't wander along the outside walls.");
    return TRUE;
  case 2:
    act("$n keeps a close eye on you.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    mobsay(ch, "I hear manticores make good mounts!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7107
   *Name:guard
 */
int bs_guard(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    mobsay(ch, "Beware the soultaker!");
    return TRUE;
  case 1:
    act("$n dazzles you with a sword technique.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "Now where did that apprentice go?  You're not him!");
    do_action(ch, 0, CMD_FROWN);
    return TRUE;
  case 3:
    mobsay(ch,
           "Like my equipment eh? Visit Waqar to get a fine saber like this!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7108
   *Name:squire
 */
int bs_squire(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    mobsay(ch, "I better get back to my master before I'm missed!");
    return TRUE;
  case 1:
    act("$n bungles a simple dagger technique.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_BLUSH);
    return TRUE;
  case 2:
    mobsay(ch, "With enough practice, I too can be a guard!");
    act("$n manages to execute a perfect dagger parry.",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_STRUT);
    return TRUE;
  case 3:
    mobsay(ch, "I used to be a wandering soul such as yourself.");
    mobsay(ch, "Now my life has purpose!");
    do_action(ch, 0, CMD_SMIRK);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7109
   *Name:peddler
 */
int bs_peddler(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    mobsay(ch, "Can I sell you something?");
    return TRUE;
  case 1:
    act("$n displays to you his wares.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "What?!  This is the finest merchandise in town!");
    do_action(ch, 0, CMD_FROWN);
    return TRUE;
  case 3:
    mobsay(ch, "Got something you don't want?  I'll buy most anything!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7110    7111     7112       7141
   *Name:vrock   hezrou   glabrezu   lurker
 */
int bs_critter(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  if (devour(ch, pl, cmd, arg))
    return TRUE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7113
   *Name:timid prisoner
 */
int bs_timid(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "I swear I didn't do it!");
    do_action(ch, 0, CMD_CRY);
    return TRUE;
  case 1:
    mobsay(ch, "This place is too dangerous for me!");
    do_action(ch, 0, CMD_SNIFF);
    return TRUE;
  case 2:
    mobsay(ch, "Let me out! Let me out!");
    return TRUE;
  case 3:
    mobsay(ch, "I wouldn't go upstairs if I were you!");
    do_action(ch, 0, CMD_WINCE);
    return TRUE;
  case 4:
    mobsay(ch, "I wish that I was back home in Verzanan!");
    do_action(ch, 0, CMD_SIGH);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7114
   *Name:shady prisoner
 */
int bs_shady(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "The food is terrible here!");
    return TRUE;
  case 1:
    mobsay(ch, "Hey you're pretty cute!");
    do_action(ch, 0, CMD_DROOL);
    return TRUE;
  case 2:
    mobsay(ch, "Outta my way punk!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 3:
    act("$n looks you over very carefully.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n winks at you in a seductive way.", TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "What do you say you and me get together?");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7115
   *Name:sinister prisoner
 */
int bs_sinister(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Ya, I killed him.  What of it?");
    return TRUE;
  case 1:
    mobsay(ch, "Hey, you're pretty cute!");
    do_action(ch, 0, CMD_DROOL);
    return TRUE;
  case 2:
    mobsay(ch, "Go away before I rip your throat out!");
    do_action(ch, 0, CMD_GLARE);
    return TRUE;
  case 3:
    mobsay(ch, "Care to spar with me, chump?");
    do_action(ch, 0, CMD_FLEX);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7116
   *Name:menacing prisoner
 */
int bs_menacing(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    mobsay(ch, "I feel like eating somebody's heart!");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7117
   *Name:executioner
 */
int bs_executioner(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Nothing like a fine axe blade, eh?");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  case 1:
    mobsay(ch, "She said, 'Sir, please don't kill my husband!'");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 2:
    act("$n swings $s heavy axe over $s shoulder.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    mobsay(ch, "You'd have to be pretty strong to wield this axe!");
    do_action(ch, 0, CMD_FLEX);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_SMIRK);
    mobsay(ch, "Stay out of trouble or I'll add ya to my necklace!");
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch, "Understand me?!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7118
   *Name:baron
 */
int bs_baron(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "Methinks it is time to raise taxes!");
    do_action(ch, 0, CMD_CHUCKLE);
    return TRUE;
  case 1:
    mobsay(ch, "Where are my servants?!");
    do_action(ch, 0, CMD_POUT);
    return TRUE;
  case 2:
    act("$n removes a sparkling ring, and then replaces it!",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    mobsay(ch, "If my brother could only see me now!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_GRUMBLE);
    mobsay(ch, "I have business to tend to.  Go purchase something!");
    mobsay(ch, "We have some of the finest shops around!");
    return TRUE;
  case 5:
    do_action(ch, 0, CMD_YAWN);
    mobsay(ch, "Servants!  Time for my nap.");
    do_action(ch, 0, CMD_SNAP);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7119
   *Name:soultaker
 */
int bs_soultaker(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

  /*
     spec die 
   */
  if (cmd == CMD_DEATH)
    return bs_undead_die(ch, pl, cmd, arg);

  if (!AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7120
   *Name:sparrow
 */
int bs_sparrow(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_HOP);
    return TRUE;
  case 1:
    act("$n turns $s eyes towards the ground searching for some food.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7121        7122
   *Name:squirrel    huge squirrel
 */
int bs_squirrel(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    act("$n forages for some food.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n looks up to see what's around.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7123
   *Name:crow
 */
int bs_crow(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_HOP);
    act("$n eyes the area for some food.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n stares in your direction with malicious intent.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7124
   *Name:mountainman
 */
int bs_mountainman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_YODEL);
    return TRUE;
  case 1:
    act("$n strikes the ground with $s iron pick.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "Hail stranger!  On your way to Bloodstone Keep?");
    return TRUE;
  case 3:
    mobsay(ch, "I'd keep clear of the manticore lair if I were you!");
    return TRUE;
  case 4:
    mobsay(ch, "To the far west is where the dead sleep.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7125
   *Name:salesman
 */
int bs_salesman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Pssst!  I've got some items you might want!");
    return TRUE;
  case 1:
    mobsay(ch, "Buy low, sell high...A sound method, is it not?");
    return TRUE;
  case 2:
    mobsay(ch, "I buy what you do not want!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7126
   *Name:nomad
 */
int bs_nomad(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Which way to the closest inn?  I am oh so very tired.");
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_WINCE);
    mobsay(ch,
           "I've seen many vile creatures in my days, but NONE as gruesome as you!");
    do_action(ch, 0, CMD_CRINGE);
    return TRUE;
  case 2:
    mobsay(ch, "This place is too dangerous for a wimp like you!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7127
   *Name:insane woman
 */
int bs_insane(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Who are you? What do you want? Leave me alone!");
    return TRUE;
  case 1:
    mobsay(ch, "Get them off of me!");
    do_action(ch, 0, CMD_HOP);
    return TRUE;
  case 2:
    mobsay(ch, "Kill them, kill them!!");
    do_action(ch, 0, CMD_STOMP);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_FIDGET);
    mobsay(ch, "Look!  A six foot stirge!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_CRY);
    mobsay(ch, "Damn elves!  Damn dwarves!  Damn them all!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7128
   *Name:homeless man
 */
int bs_homeless(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Spare a few coins?");
    do_action(ch, 0, CMD_BEG);
    return TRUE;
  case 1:
    mobsay(ch, "I haven't always been this way...");
    do_action(ch, 0, CMD_CRY);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_SHIVER);
    mobsay(ch, "It is so cold, could you spare some coins for a room?");
    do_action(ch, 0, CMD_BEG);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_CRY);
    mobsay(ch, "Please help me, I haven't eaten in three moons.");
    do_action(ch, 0, CMD_SNIFF);
    return TRUE;
  case 4:
    mobsay(ch, "Will work for food!");
    return TRUE;
  case 5:
    mobsay(ch, "Spare some clothing for a homeless man?");
    do_action(ch, 0, CMD_SHIVER);
    mobsay(ch, "It's terribly cold here at night.");
    return TRUE;
  case 6:
    do_action(ch, 0, CMD_COUGH);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7129
   *Name:baron's servant
 */
int bs_servant(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Time for the Baron's bath.");
    return TRUE;
  case 1:
    mobsay(ch, "The Baron is such a demanding master!");
    do_action(ch, 0, CMD_SNIFF);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_WINCE);
    mobsay(ch, "I hope that the Baron spares me from my daily beating.");
    mobsay(ch, "I have worked hard for him today!");
    do_action(ch, 0, CMD_PONDER);
    return TRUE;
  case 3:
    mobsay(ch, "Hmm..I wonder what the Baron will want to eat tonight?");
    do_action(ch, 0, CMD_PONDER);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7140
   *Name:wolf
 */
int bs_wolf(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  if (devour(ch, pl, cmd, arg))
    return TRUE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_BARK);
    return TRUE;
  case 3:
    act("$n lifts $s head up and lets out a piercing howl.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7142
   *Name:bear
   *Temp OBJ made here.
 */
int bs_bear(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    bearcrap = NULL;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 70))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    bearcrap = read_object(7150, VIRTUAL);
    if (!bearcrap)
    {
      logit(LOG_EXIT, "assert: bs_bear() failed to load object 7150");
      raise(SIGSEGV);
    }
    obj_to_room(bearcrap, ch->in_room);
    act("$n looks around, squats, and takes a huge crap.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7143
   *Name:gnoll
 */
int bs_gnoll(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    act("$n looks at you with malicious intent.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7144
   *Name:ettin
 */
int bs_ettin(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    act("$n scratches one of $s heads.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("$n begins picking $s teeth again.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7147
   *Name:griffon
 */
int bs_griffon(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    act("$n fans $s enormous wings.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("$n begins eyeing you quite intently.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    act("$n snaps the bone of a carcus $e is devouring.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7152
   *Name:wereboar
 */
int bs_boar(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  case 2:
    act("$n lifts $s head up and lets out a piercing howl.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7153
   *Name:manticore cub
 */
int bs_cub(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n rubs up against your leg.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_PURR);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  case 2:
    act("$n swats at a leaf turning circles in the wind.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7154
   *Name:manticore fierce
 */
int bs_fierce(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  if (devour(ch, pl, cmd, arg))
    return TRUE;

  switch (number(0, 100))
  {
  case 0:
    act("$n eyes you very carefully.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  case 2:
    act("$n lets out a tremendous roar.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    act("$n begins grooming itself.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7156
   *Name:mind flayer
 */
int bs_flayer(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n stops dancing to look at you.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n continues the ritual dance.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7160
   *Name:stirge
 */
int bs_stirge(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_HOP);
    return TRUE;
  case 1:
    act("$n turns $s eyes towards you in search of some fresh blood.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7162       7167
   *Name:spectre    spirit
 */
int bs_undead_with_die(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

  if (cmd == CMD_DEATH)
    return bs_undead_die(ch, pl, cmd, arg);

  if (!AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_MOAN);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROAN);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7161       7165      7166
   *Name:zombie     mummy     tranth    
 */
int bs_undead_without_die(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_MOAN);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROAN);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7164
   *Name:grave robber
 */
int bs_robber(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "That corpse won't be needing this!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 1:
    mobsay(ch, "You can make a fortune at grave robbing!");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  case 2:
    mobsay(ch, "Kali is around here someplace.  He has the key to the gate!");
    return TRUE;
  case 3:
    mobsay(ch, "Now where is that hole leading downwards into the tombs?");
    return TRUE;
  case 4:
    act("$n begins digging up a new grave.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 5:
    act("$n searches a body just harvested from the ground.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7170
   *Name:sick guard
 */
int bs_sickguard(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_COUGH);
    mobsay(ch, "I have to get better...");
    do_action(ch, 0, CMD_SNEEZE);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_MOAN);
    return TRUE;
  case 2:
    mobsay(ch, "My stomach is killing me...");
    do_action(ch, 0, CMD_GROAN);
    return TRUE;
  case 3:
    act("$n rolls over onto $s side, trying to get comfortable.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_COUGH);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7172
   *Name:Armor Shop keeper
 */
int bs_armor(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_HUM);
    mobsay(ch, "Hurry up and make a selection, I'm busy today.");
    return TRUE;
  case 1:
    act("$n turns towards the back of the shop and yells 'More shields!'",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "We have the finest armor in all the realm!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 3:
    act("$n drums $s fingers upon the counter looking very impatient.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_COUGH);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7173
   *Name:Eatery Shop keeper
 */
int bs_eatery(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMELL);
    mobsay(ch, "Please hurry, I think my roasts are burning.");
    return TRUE;
  case 1:
    act("$n begins wiping stirge grease off the countertop.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "Hmmm...nothing better than a roasted stirge.");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 3:
    act("$n tosses a couple more stirge into an oven.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_HUM);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7174
   *Name:Magic Shop keeper
 */
int bs_magic(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch, "Tioko ferisal gnalum.");
    return TRUE;
  case 1:
    act("$n finishes up on yet another scroll.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "Would you like to buy some of my scrolls?");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 3:
    act("$n rearranges $s magical item display.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_SMILE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7175
   *Name:Weapon Shop keeper
 */
int bs_weapon(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SIGH);
    mobsay(ch, "Hey, if you're just going to look, get the hell out!");
    return TRUE;
  case 1:
    act("$n runs $s thumb down the blade of a saber.",
        TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "This is good stuff!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "Steal something and I'll take your head.");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  case 3:
    act("$n looks at you annoyingly.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_HUM);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7176
   *Name:Stable Shop keeper
 */
int bs_stable(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "My horses are strong and durable.");
    return TRUE;
  case 1:
    act("$n nails a horseshoe onto the wall.", TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "This is for good luck!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "How about a fine stallion today?");
    return TRUE;
  case 3:
    act("$n reaches over and begins to groom a horse.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_WHISTLE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7177
   *Name:Inn keeper
 */
int bs_inn(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;


  if (!AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "Can I interest you in a clean room today?");
    return TRUE;
  case 1:
    act("$n begins sweeping the floor.", TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "Damn this dust!");
    do_action(ch, 0, CMD_FROWN);
    return TRUE;
  case 2:
    mobsay(ch, "You know, this is the finest inn in the realm!");
    return TRUE;
  case 3:
    act("$n straightens a picture hanging on the wall.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_WHISTLE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7178
   *Name:young man
 */
int bs_youngm(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "You look very lovely tonight, my dear.");
    return TRUE;
  case 1:
    mobsay(ch, "Someday we'll make it out of this terrible life.");
    do_action(ch, 0, CMD_SIGH);
    return TRUE;
  case 2:
    mobsay(ch, "This is just the way you like it right?");
    return TRUE;
  case 3:
    mobsay(ch, "Someday we'll be married.");
    return TRUE;
  case 4:
    act("$n runs $s tongue down the woman's bare back.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 5:
    act("$n continues to move $s hands slowly over the woman.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7179
   *Name:young woman
 */
int bs_youngw(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "A little more to the left.");
    return TRUE;
  case 1:
    mobsay(ch, "You feel so good tonight.");
    do_action(ch, 0, CMD_SIGH);
    return TRUE;
  case 2:
    mobsay(ch, "I bet you can't wait until it's your turn.");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_GIGGLE);
    mobsay(ch, "Hey, that tickled!");
    return TRUE;
  case 4:
    act("$n stretches out over the huge bed.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 5:
    act("$n looks over at you and gives a suggestive wink.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7180
   *Name:prostitute
 */
int bs_prostitute(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if (cmd)
    return prostitute_one(ch, pl, cmd, arg);

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_STRUT);
    mobsay(ch, "Lookin' for something honey?");
    return TRUE;
  case 1:
    mobsay(ch, "I know what you like.");
    do_action(ch, 0, CMD_GIGGLE);
    return TRUE;
  case 2:
    mobsay(ch, "I'm yours for a mere platinum coin.");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_GIGGLE);
    mobsay(ch, "I bet you know how to treat a lady.");
    return TRUE;
  case 4:
    act("$n walks over to you and feels your muscles.",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_GASP);
    return TRUE;
  case 5:
    act("$n looks over at you and gives a suggestive wink.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7189
   *Name:Pet shop keeper
 */
int bs_pet(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "Aren't my animals beautiful?");
    return TRUE;
  case 1:
    act("$n tosses a chunk of meat into a cage.", TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "Did you see him eat that!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "Before purchasing a pet, perhaps you should buy a book.");
    return TRUE;
  case 3:
    act("$n looks adoringly at $s animals.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Have you seen my husband Shargaas?");
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "Ahh..he's probably out trapping manticore today.");
    do_action(ch, 0, CMD_SMILE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7190
   *Name:greedy banker
 */
int bs_banker(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

  if ((cmd == CMD_EXCHANGE) || (cmd == CMD_LIST))
    return money_changer(ch, pl, cmd, arg);

  if (!AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch, "Money, money, money...");
    return TRUE;
  case 1:
    act("$n makes tall stacks out of platinum coins.",
        TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "Are you going to make a deposit?");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "I love money.");
    return TRUE;
  case 3:
    act("$n begins to write down some figures in the ledger.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Fill out a slip, and read the sign if you have questions.");
    do_action(ch, 0, CMD_SMILE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7191
   *Name:trenton 
 */
int bs_trenton(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Out trapping manticore today are you?");
    return TRUE;
  case 1:
    mobsay(ch, "We have the finest adventuring supplies around!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "You can never have enough equipment!");
    return TRUE;
  case 3:
    act("$n restocks the shelves behind $m.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "A torch is what you need!");
    do_action(ch, 0, CMD_SMILE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7192
   *Name:enochel the gemsmith 
 */
int bs_enochel(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Care to purchase a fine gem today?");
    return TRUE;
  case 1:
    mobsay(ch, "Please have a seat.  I will get to you shortly.");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "A purchase for a \"special\" friend perhaps?");
    do_action(ch, 0, CMD_WINK);
    return TRUE;
  case 3:
    act("$n polishes up a fine black gem.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Now, how can I be of service?");
    do_action(ch, 0, CMD_SMILE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7193
   *Name:veriallo 
 */
int bs_veriallo(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Barrels, barrels, barrels...");
    do_action(ch, 0, CMD_FROWN);
    return TRUE;
  case 1:
    mobsay(ch, "Please have a seat.  I will get to you shortly.");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "Please buy a barrel, we have so many!");
    do_action(ch, 0, CMD_BEG);
    return TRUE;
  case 3:
    act("$n stocks the wall behind $m with huge barrels.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Now, how can I be of service?");
    do_action(ch, 0, CMD_SMILE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7194
   *Name:cerrio 
 */
int bs_cerrio(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Business has been bad lately since the Dead Ogre opened.");
    do_action(ch, 0, CMD_FROWN);
    return TRUE;
  case 1:
    mobsay(ch, "Name your poison.");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "We have a great selection of beverages to choose from.");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 3:
    act("$n wipes the counter with a wet rag.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Have you visited the brothel?");
    do_action(ch, 0, CMD_WINK);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7195
   *Name:ghelian the baker 
 */
int bs_ghelian(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "I am all out of cookies right now.");
    do_action(ch, 0, CMD_FROWN);
    return TRUE;
  case 1:
    mobsay(ch, "Can I interest you in some fine eats?");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "Not as many items as Gynter, but I deliver!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 3:
    act("$n pushes $s wooden cart around.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Beware the stirge farm outside the Keep.");
    do_action(ch, 0, CMD_SMILE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7199
   *Name:male slave from verzanan 
 */
int bs_mslave(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Shine your weapon?");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 1:
    mobsay(ch, "Can I get you a drink?");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "How about a massage?");
    return TRUE;
  case 3:
    act("$n runs over to you to attend to your needs.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Oh how I miss my fair city of Verzanan.");
    do_action(ch, 0, CMD_SNIFF);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7200
   *Name:female slave 
 */
int bs_fslave(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Can I help you in any way today?");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 1:
    mobsay(ch, "Let me wipe the sweat from your face.");
    return TRUE;
  case 2:
    mobsay(ch, "How about a massage?");
    return TRUE;
  case 3:
    act("$n runs $s fingers through your coarse, sweaty hair.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "I am yours, if you wish to take me.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7201
   *Name:antipal master 
 */
int bs_antimaster(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Pay attention, you idiot!");
    do_action(ch, 0, CMD_GLARE);
    return TRUE;
  case 1:
    mobsay(ch,
           "If you wish to have your own statue, you must show me your worth.");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  case 2:
    mobsay(ch, "This is the correct way to hold that weapon.");
    return TRUE;
  case 3:
    act("$n dazzles you with a brilliant sword move.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch,
           "Black-hearted are we, hated by most.  They will feel our might.");
    do_action(ch, 0, CMD_CACKLE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7202
   *Name:antipal shopkeeper 
 */
int bs_antishop(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Fine weapons and armor is what I offer.");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 1:
    mobsay(ch,
           "Made from the finest alloys, ready to work for the evil hand.");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  case 2:
    mobsay(ch, "Now this is a fine weapon indeed!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 3:
    act("$n hangs a few more weapons upon the wall.", TRUE, ch, 0, 0,
        TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Spineless wimps live in Verzanan, do you not agree?");
    do_action(ch, 0, CMD_LAUGH);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7203
   *Name:antipal brothelkeeper 
 */
int bs_antibrothel(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Hurry up, you worthless whore!");
    return TRUE;
  case 1:
    mobsay(ch, "Hey boy, get over there and attend to that person!");
    return TRUE;
  case 2:
    mobsay(ch, "Good day, might I offer you a slave and a drink?");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 3:
    act("$n lashes out at the slaves with a whip.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "I'll need to go to Verzanan soon for some new slaves.");
    do_action(ch, 0, CMD_CACKLE);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7205
   *Name:sleeping guard 
 */
int bs_zzzguard(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNORE);
    return TRUE;
  case 1:
    mobsay(ch, "Mommy, mommy....");
    do_action(ch, 0, CMD_SNORE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7206
   *Name:sleeping citizen 
 */

int bs_zzzcitizen(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNORE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7209
   *Name:field boss 
 */
int bs_boss(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Are you some kind of sick freak or what?");
    do_action(ch, 0, CMD_GLARE);
    return TRUE;
  case 1:
    mobsay(ch, "I am going to slaughter your hide when I am done here.");
    return TRUE;
  case 2:
    mobsay(ch, "Oh god, please have mercy!");
    do_action(ch, 0, CMD_GRUNT);
    return TRUE;
  case 3:
    act("$n squeezes out a huge chunk of shit.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Damn! I forgot the toilet parchment!");
    do_action(ch, 0, CMD_MOAN);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7210
   *Name:tax knight 
   * This guy will demand tribute to the Baron of Bloodstone to allow entrance to
   * Bloodstone.  The cost is 5 platinum per person.
   * -- DTS 4/3/95
 */
int bs_tax(P_char ch, P_char pl, int cmd, char *arg)
{
  int      beforecash, aftercash;

  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if (ch->in_room == real_room(7680))
  {
    if (cmd == CMD_NORTH)
    {
      if (IS_PC(pl))
      {
        act("$n stops you.", FALSE, ch, 0, pl, TO_VICT);
        act("$n stops $N.", TRUE, ch, 0, pl, TO_NOTVICT);
        mobsay(ch,
               "You cannot enter the Baron's realm without first paying homage to my lord!");
        mobsay(ch, "The price to enter is 15 gold coins!");
        return TRUE;
      }
      else
      {
        act("$N gives $n some money.", TRUE, ch, 0, pl, TO_NOTVICT);
        mobsay(ch,
               "'Tis wise of you to part with gold so you do not have to part with your head.");
        mobsay(ch, "Welcome to the Barony of Bloodstone.");
        act("$n steps aside to let $N pass northwards.",
            TRUE, ch, 0, pl, TO_NOTVICT);
        char_from_room(pl);
        char_to_room(pl, real_room(7681), 0);
        act("$n arrives from the south.", TRUE, pl, 0, 0, TO_ROOM);
        return TRUE;
      }
    }
    else if (cmd == CMD_GIVE)
    {
      beforecash = GET_MONEY(ch);
      do_give(pl, arg, cmd);
      aftercash = GET_MONEY(ch);
      if ((aftercash - beforecash) < 1500)
      {
        mobsay(ch, "The cost is 15 gold coins, worm!  Try again.");
        do_action(ch, 0, CMD_GRIN);
        return TRUE;
      }
      else
      {
        mobsay(ch,
               "'Tis wise of you to part with gold so you do not have to part with your head.");
        mobsay(ch, "Welcome to the Barony of Bloodstone.");
        act("$n steps aside to let you pass northwards.",
            FALSE, ch, 0, pl, TO_VICT);
        act("$n steps aside to let $N pass northwards.",
            TRUE, ch, 0, pl, TO_NOTVICT);
        char_from_room(pl);
        char_to_room(pl, real_room(7681), 0);
        act("$n arrives from the south.", TRUE, pl, 0, 0, TO_ROOM);
        return TRUE;
      }
    }
    else if (cmd)
    {
      return FALSE;
    }
    else
    {
      switch (number(0, 80))
      {
      case 0:
        mobsay(ch, "Welcome to the Barony of Bloodstone!");
        do_action(ch, 0, CMD_SMILE);
        return TRUE;
      case 1:
        mobsay(ch, "I am the official tax collector of the Keep.");
        mobsay(ch,
               "You must pay me a tribute of 15 gold coins to the mighty Baron to enter.");
        return TRUE;
      case 2:
        mobsay(ch, "I will not let you pass unless you pay the tribute!");
        return TRUE;
      case 3:
        mobsay(ch, "The Keep is far to the north.");
        do_action(ch, 0, CMD_SMILE);
        return TRUE;
      case 4:
        act("$n holds out $s hand for some coins.", TRUE, ch, 0, 0, TO_ROOM);
        return TRUE;
      case 5:
        mobsay(ch, "Pay now, or die now; 'tis a simple choice, is it not?");
        do_action(ch, 0, CMD_CACKLE);
      default:
        return FALSE;
      }
    }
  }
  else
  {
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7220
   *Name:acolyte 
 */
int bs_acolyte(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n listens closely to $s master.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n lets out a quiet yawn.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("$n looks around the room as if $e was bored.",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7221
   *Name:clericpriest 
 */
int bs_clericpriest(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n slaps an acolyte viciously across the face.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n notices an acolyte is not paying attention.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7311
   *Name:necromancer master 
 */
int bs_necromaster(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "The way of the necromancer is what I will show you.");
    return TRUE;
  case 1:
    act("$n prepares some reagents.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "To command the dead is the ultimate power.");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "Who shall be my favorite apprentice?");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7314               
   *Name:thief master 
 */
int bs_thiefmaster(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "To be a successful thief is no trivial feat!");
    return TRUE;
  case 1:
    act("$n picks your pockets, and gives you back the stolen items.",
        TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "See how easily it is done, young one?");
    return TRUE;
  case 2:
    mobsay(ch, "Sneak, hide, and backstab are a thief's best strategies.");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "What items do you think the Baron has to steal today?");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7317
   *Name:assassin master 
 */
int bs_assmaster(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "Come and learn the ways of the assassin!");
    return TRUE;
  case 1:
    act("$n displays a deadly technique with a dagger.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "Kill first, ask questions later.");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch,
           "Inhabitants of Verzanan squeal like pigs when put under the knife.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7321
   *Name:mage master 
 */
int bs_magemaster(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "Come and learn the ways of the mystic!");
    return TRUE;
  case 1:
    act
      ("$n summons a horrid demon, and then sends it back to its own dimension.",
       TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch,
           "It is hard at first, but once a powerful spell caster you will rule the world.");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch,
           "The magic that you learn here is to be used for the cause of evil.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7322
   *Name:thief shopkeeper
 */
int bs_thiefshop(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "We have an excellent selection.");
    return TRUE;
  case 1:
    act("$n stocks the shelves behind $m with some thief wares.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "We frequently have excellent boots for sale.");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch, "A set of lockpicks can help you past stubborn doors.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7323
   *Name:assassin shopkeeper
 */
int bs_assshop(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "We have an excellent selection.");
    return TRUE;
  case 1:
    act("$n stocks the shelves behind $m with some wares.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "Tools of murder is what I specialize in.");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch, "One of my poisons can bring down the strongest man.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7324
   *Name:necro shopkeeper
 */
int bs_necroshop(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "We have an excellent selection.");
    return TRUE;
  case 1:
    act("$n stocks the shelves behind $m with some blackmoore.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "What can I sell you today?");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7325
   *Name:mage shopkeeper
 */
int bs_mageshop(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "We have an excellent selection.");
    return TRUE;
  case 1:
    act("$n stocks the shelves behind $m with some scrolls.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "You have traveled a lot further than you think!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7326
   *Name:payton tavernkeeper
 */
int bs_payton(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "Do you like my ogre trophies?");
    return TRUE;
  case 1:
    act("$n stocks the shelves behind $m with some bottles.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "Ogres, ogres, OGRES!  I hate those bastards.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7331
   *Name:dark priest
 */
int bs_darkpriest(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 60))
  {
  case 0:
    mobsay(ch, "Vichtil, senum, alitra!");
    return TRUE;
  case 1:
    mobsay(ch, "Mighty Orcus, grant us your black powers!");
    return TRUE;
  case 2:
    mobsay(ch, "Oh dark lord, send forth your minions. We are prepared!");
    return TRUE;
  case 3:
    mobsay(ch, "With an army of demons at my command, WE shall conquer all!");
    return TRUE;
  case 4:
    mobsay(ch, "We offer this pure soul, a newborn, to you master!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7335
   *Name:brothel bouncer
 */
int bs_bouncer(P_char ch, P_char pl, int cmd, char *arg)
{
  int      beforecash, aftercash;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if (ch->in_room == real_room(7431))
  {
    if (cmd == CMD_SOUTH)
    {
      if (IS_PC(pl))
      {
        act("$n stops you from entering the building to the south.",
            FALSE, ch, 0, pl, TO_VICT);
        act("$n stops $N from entering the building to the south.",
            TRUE, ch, 0, pl, TO_NOTVICT);
        mobsay(ch, "Sorry, friend, but you'll have to pay to enter!");
        mobsay(ch, "The price is 1 platinum coin.");
        return TRUE;
      }
      else
      {
        act("$N gives $n some money.", TRUE, ch, 0, pl, TO_NOTVICT);
        mobsay(ch, "That'll do!");
        act("$n steps aside to let $N enter.", TRUE, ch, 0, pl, TO_NOTVICT);
        char_from_room(pl);
        char_to_room(pl, real_room(7432), 0);
        act("$n enters from the north.", TRUE, pl, 0, 0, TO_ROOM);
        return TRUE;
      }
    }
    else if (cmd == CMD_GIVE)
    {
      beforecash = GET_MONEY(ch);
      do_give(pl, arg, cmd);
      aftercash = GET_MONEY(ch);
      if ((aftercash - beforecash) < 1000)
      {
        mobsay(ch, "Friend, the entry fee is 1 platinum coin.  Try again.");
        return TRUE;
      }
      else
      {
        mobsay(ch, "That'll do!");
        act("$n steps aside to let you enter.", FALSE, ch, 0, pl, TO_VICT);
        act("$n steps aside to let $N enter.", TRUE, ch, 0, pl, TO_NOTVICT);
        char_from_room(pl);
        char_to_room(pl, real_room(7432), 0);
        act("$n enters from the north.", TRUE, pl, 0, 0, TO_ROOM);
        return TRUE;
      }
    }
    else if (cmd)
    {
      return FALSE;
    }
    else
    {
      switch (number(0, 60))
      {
      case 0:
        mobsay(ch, "Wait a minute, you don't look old enough to enter!");
        return TRUE;
      case 1:
        mobsay(ch, "If I hear any complaints, I'll come in after ya!");
        return TRUE;
      case 2:
        mobsay(ch, "In or out!  Don't crowd the alley.");
        return TRUE;
      case 3:
        mobsay(ch,
               "HA!  Laughter is what you will hear if you shed those clothes.");
        return TRUE;
      case 4:
        mobsay(ch, "Be sure to carry a heavy purse.");
        return TRUE;
      default:
        return FALSE;
      }
    }
  }
  else
  {
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7336
   *Name:naked woman
 */
int bs_nakedwoman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 60))
  {
  case 0:
    mobsay(ch, "Are you ready for some excitement?");
    return TRUE;
  case 1:
    mobsay(ch, "What's the matter?  Not your type?");
    return TRUE;
  case 2:
    mobsay(ch, "I am yours to fulfill your wild pleasures.");
    return TRUE;
  case 3:
    mobsay(ch, "Shall I turn over?");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7338
   *Name:naked man
 */
int bs_nakedman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 60))
  {
  case 0:
    mobsay(ch, "What's your sign, baby?");
    return TRUE;
  case 1:
    mobsay(ch, "Was your father a thief?  He must have stolen stars");
    mobsay(ch, "from the heavens and given them to you as eyes.");
    return TRUE;
  case 2:
    mobsay(ch, "I am not much, but I am yours if you will have me.");
    return TRUE;
  case 3:
    mobsay(ch, "Your innocence strikes my curiosity.");
    return TRUE;
  default:
    return FALSE;
  }
}

int bs_undead_die(P_char ch, P_char pl, int cmd, char *arg)
{
  if (!ch)
  {
    logit(LOG_EXIT, "No ch to bs_undead_die()!");
    raise(SIGSEGV);
    return TRUE;
  }
  /*
     spec die 
   */
  if (cmd == CMD_DEATH)
    act("$n turns into a black vapor and seeps into the ground.",
        FALSE, ch, 0, 0, TO_ROOM);
  return FALSE;
}

/*
 *Bloodstone Zone 224 Mob Proc
 *Mob#:22432 (follow 22431)
 */
int bs_follow_22432(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22431)) &&
        (GET_RNUM(ch) == real_mobile(22432)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 224 Mob Proc
 *Mob#:22434 (follow 22433)
 */
int bs_follow_22434(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22433)) &&
        (GET_RNUM(ch) == real_mobile(22434)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 224 Mob Proc
 *Mob#:22436 (follow 22435)
 */
int bs_follow_22436(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22435)) &&
        (GET_RNUM(ch) == real_mobile(22436)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 224 Mob Proc
 *Mob#:22456 (follow 22455)
 */
int bs_follow_22456(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22455)) &&
        (GET_RNUM(ch) == real_mobile(22456)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 224 Mob Proc
 *Mob#:22458 (follow 22457)
 */
int bs_follow_22458(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22457)) &&
        (GET_RNUM(ch) == real_mobile(22458)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 224 Mob Proc
 *Mob#:22461 (follow 22460)
 */
int bs_follow_22461(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22460)) &&
        (GET_RNUM(ch) == real_mobile(22461)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 224 Mob Proc
 *Mob#:22466 (follow 22465)
 */
int bs_follow_22466(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22465)) &&
        (GET_RNUM(ch) == real_mobile(22466)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 224 Mob Proc
 *Mob#:22467 (follow 22465)
 */
int bs_follow_22467(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22465)) &&
        (GET_RNUM(ch) == real_mobile(22467)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 225 Mob Proc
 *Mob#:22508 (follow 22511)
 */
int bs_follow_22508(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22511)) &&
        (GET_RNUM(ch) == real_mobile(22508)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 225 Mob Proc
 *Mob#:22509 (follow 22511)
 */
int bs_follow_22509(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22511)) &&
        (GET_RNUM(ch) == real_mobile(22509)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 225 Mob Proc
 *Mob#:22510 (follow 22511)
 */
int bs_follow_22510(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22511)) &&
        (GET_RNUM(ch) == real_mobile(22510)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 225 Mob Proc
 *Mob#:22512 (follow 22511)
 */
int bs_follow_22512(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22511)) &&
        (GET_RNUM(ch) == real_mobile(22512)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *Bloodstone Zone 225 Mob Proc
 *Mob#:22513 (follow 22511)
 */
int bs_follow_22513(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (IS_NPC(tch) && (GET_RNUM(tch) == real_mobile(22511)) &&
        (GET_RNUM(ch) == real_mobile(22513)))
    {
      add_follower(ch, tch);
      group_add_member(tch, ch);
      return TRUE;
    }
  }
  return FALSE;
}

int bs_guildguard_necro(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     buf[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch->in_room != real_room(GET_BIRTHPLACE(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return TRUE;
  }
  if ((ch->in_room == real_room(7880)) && (cmd == CMD_WEST))
  {
    if (GET_CLASS(pl) == CLASS_NECROMANCER)
    {
      sprintf(buf, "door");
      do_open(ch, buf, 0);
      mobsay(ch, "Go forward, sibling.");
      act("You enter the guild, nodding to $n as you pass.",
          FALSE, ch, 0, pl, TO_VICT);
      act("$N enters the guild, nodding to $n as $E passes.",
          TRUE, ch, 0, pl, TO_NOTVICT);
      char_from_room(pl);
      char_to_room(pl, real_room(7881), 0);
      do_close(ch, buf, 0);
      return TRUE;
    }
    else
    {
      do_action(ch, 0, CMD_SMIRK);
      mobsay(ch,
             "I don't think you have the proper knowledge to pass this unholy seal.");
      return TRUE;
    }
  }
  return FALSE;
}

int bs_guildguard_thief(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     buf[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch->in_room != real_room(GET_BIRTHPLACE(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return TRUE;
  }
  if ((ch->in_room == real_room(7864)) && (cmd == CMD_WEST))
  {
    if (GET_CLASS(pl) == CLASS_ROGUE)
    {
      act
        ("$n looks you over carefully, and then turns to move a huge boulder to one side.",
         FALSE, ch, 0, pl, TO_VICT);
      act
        ("$n eyes $N thoughtfully before turning to move a huge boulder to one side.",
         TRUE, ch, 0, pl, TO_NOTVICT);
      sprintf(buf, "boulder");
      do_open(ch, buf, 0);
      act("You enter the guild.", FALSE, ch, 0, pl, TO_VICT);
      act("$N enters the guild.", TRUE, ch, 0, pl, TO_NOTVICT);
      char_from_room(pl);
      char_to_room(pl, real_room(7865), 0);
      do_close(ch, buf, 0);
      return TRUE;
    }
    else
    {
      act("$n shakes $s head in disgust and points $s finger upwards.",
          TRUE, ch, 0, 0, TO_ROOM);
      act("$n grunts at you, 'This is no place for you.'",
          FALSE, ch, 0, pl, TO_VICT);
      act("$n grunts at $N, 'This is no place for you.'",
          TRUE, ch, 0, pl, TO_NOTVICT);
      return TRUE;
    }
  }
  return FALSE;
}

int bs_guildguard_assassin(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     buf[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch->in_room != real_room(GET_BIRTHPLACE(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return TRUE;
  }
  if ((ch->in_room == real_room(7837)) && (cmd == CMD_WEST))
  {
    if(GET_SPEC(pl, CLASS_ROGUE, SPEC_ASSASSIN))
    {
      act("$n nods at you.", FALSE, ch, 0, pl, TO_VICT);
      act("$n nods at $N.", TRUE, ch, 0, pl, TO_NOTVICT);
      sprintf(buf, "door");
      do_open(ch, buf, 0);
      act("You enter the guild.", FALSE, ch, 0, pl, TO_VICT);
      act("$N enters the guild.", TRUE, ch, 0, pl, TO_NOTVICT);
      char_from_room(pl);
      char_to_room(pl, real_room(7843), 0);
      do_close(ch, buf, 0);
      return TRUE;
    }
    else
    {
      act("$n steps in front of the door.", TRUE, ch, 0, 0, TO_ROOM);
      mobsay(ch, "You will have to kill me to gain entrance here!");
      do_action(ch, 0, CMD_CACKLE);
      return TRUE;
    }
  }
  return FALSE;
}

int bs_guildguard_sorcconj(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     buf[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch->in_room != real_room(GET_BIRTHPLACE(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return TRUE;
  }
  if ((ch->in_room == real_room(7844)) && (cmd == CMD_EAST))
  {
    if( (GET_CLASS(pl) == CLASS_SORCERER) || (GET_CLASS(pl) == CLASS_SUMMONER)
      || (GET_CLASS(pl) == CLASS_CONJURER) )
    {
      act("$n lowers $s eyes before your gaze.", FALSE, ch, 0, pl, TO_VICT);
      act("$n lowers $s eyes at $N's gaze.", TRUE, ch, 0, pl, TO_NOTVICT);
      sprintf(buf, "door");
      do_open(ch, buf, 0);
      act("You enter the guild.", FALSE, ch, 0, pl, TO_VICT);
      act("$n enters the guild.", TRUE, ch, 0, pl, TO_NOTVICT);
      char_from_room(pl);
      char_to_room(pl, real_room(7845), 0);
      do_close(ch, buf, 0);
      return TRUE;
    }
    else
    {
      act("$n draws close to protect the door.", TRUE, ch, 0, 0, TO_ROOM);
      act
        ("$n growls, 'Leave now insect, these chains will not hold my fury!'",
         TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }
  return FALSE;
}

int bs_guildguard_clersham(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch->in_room != real_room(GET_BIRTHPLACE(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return TRUE;
  }
  if ((ch->in_room == real_room(7817)) && (cmd == CMD_DOWN))
  {
    if ((GET_CLASS(pl) == CLASS_CLERIC) || (GET_CLASS(pl) == CLASS_SHAMAN))
    {
      act("$n bows slightly, and allows you to pass.", FALSE, ch, 0, pl,
          TO_VICT);
      act("$n bows slightly, and allows $N to pass.", TRUE, ch, 0, pl,
          TO_NOTVICT);
      act("You enter the guild.", FALSE, ch, 0, pl, TO_VICT);
      act("$N enters the guild.", TRUE, ch, 0, pl, TO_NOTVICT);
      char_from_room(pl);
      char_to_room(pl, real_room(7818), 0);
      return TRUE;
    }
    else
    {
      act("$n grabs you by the throat and glares coldly into your eyes.",
          FALSE, ch, 0, pl, TO_VICT);
      act("$n grabs $N by the throat and glares coldly into $S eyes.", FALSE,
          ch, 0, pl, TO_NOTVICT);
      mobsay(ch, "Do not tempt me with your ignorance, fool!");
      act("$n tosses you aside.", FALSE, ch, 0, pl, TO_VICT);
      act("$n tosses $N aside.", FALSE, ch, 0, pl, TO_NOTVICT);
      return TRUE;
    }
  }
  return FALSE;
}

int bs_guildguard_antiwar(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot, dam;
  char     buf[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch->in_room != real_room(GET_BIRTHPLACE(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return TRUE;
  }
  if ((ch->in_room == real_room(7669)) && (cmd == CMD_NORTH))
  {
    if ((GET_CLASS(pl) == CLASS_WARRIOR) ||
        (GET_CLASS(pl) == CLASS_ANTIPALADIN))
    {
      mobsay(ch, "Aaah, an evil sibling!");
      mobsay(ch, "Enter, enter, the master will want to see you.");
      sprintf(buf, "door");
      do_open(ch, buf, 0);
      act("You enter the guild.", FALSE, ch, 0, pl, TO_VICT);
      act("$n enters the guild.", TRUE, ch, 0, pl, TO_NOTVICT);
      char_from_room(pl);
      char_to_room(pl, real_room(7670), 0);
      do_close(ch, buf, 0);
      return TRUE;
    }
    else
    {
      act("$n slaps you savagely across the face.",
          FALSE, ch, 0, pl, TO_VICT);
      act("$n slaps $N savagely across the face.",
          TRUE, ch, 0, pl, TO_NOTVICT);
      mobsay(ch, "You are not worthy, worm, now begone!");
      dam = number(1, 9);
      if ((GET_HIT(pl) - dam) < -10)
      {
        send_to_char("The force of the blow proves too much for you!\r\n",
                     ch);
        act("The force of the blow proves too much for $N!", TRUE, ch, 0, pl,
            TO_NOTVICT);
        logit(LOG_DEATH, "%s killed by %s's slap in room %d.", GET_NAME(pl),
              GET_NAME(ch), world[ch->in_room].number);
        die(pl);
        return TRUE;
      }
      GET_HIT(pl) -= dam;
      return TRUE;
    }
  }
  return FALSE;
}

int bs_guildguard_monk(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     buf[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch->in_room != real_room(GET_BIRTHPLACE(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return TRUE;
  }
  if ((ch->in_room == real_room(22487)) && (cmd == CMD_WEST))
  {
    if (GET_CLASS(pl) == CLASS_MONK)
    {
      sprintf(buf, "door");
      do_open(ch, buf, 0);
      act("The $n nods $s head at you and steps aside for you to pass.",
          FALSE, ch, 0, pl, TO_VICT);
      act("The $n nods $s head at $N and steps aside for $M to pass.", TRUE,
          ch, 0, pl, TO_NOTVICT);
      mobsay(ch, "Go forward, sibling.");
      act("You enter the guild, nodding to $n as you pass.", FALSE, ch, 0, pl,
          TO_VICT);
      act("$N enters the guild, nodding to $n as $E passes.", TRUE, ch, 0, pl,
          TO_NOTVICT);
      char_from_room(pl);
      char_to_room(pl, real_room(22502), 0);
      do_close(ch, buf, 0);
      return TRUE;
    }
    else
    {
      act("The $n looks deeply into your soul.", FALSE, ch, 0, pl, TO_VICT);
      act("The $n looks closely at $N.", TRUE, ch, 0, pl, TO_NOTVICT);
      mobsay(ch, "Sorry, only my brethren may pass through.");
      act
        ("The $n raises $s hand and directs you in the direction from which you came.",
         FALSE, ch, 0, pl, TO_VICT);
      act("The $n raises $s hand and directs $N back the way $S came.", FALSE,
          ch, 0, pl, TO_NOTVICT);
      return TRUE;
    }
  }
  return FALSE;
}

/*
   OBJECT PROCS 
 */

int bs_portal(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      to_room, from_room, temp;
  P_obj    dummyobj;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || !AWAKE(ch) || (cmd != CMD_ENTER))
    return FALSE;

  while (isspace(*arg))
    arg++;
  dummyobj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents);
  if (dummyobj != obj)
    return FALSE;

  from_room = ch->in_room;

  to_room = real_room(obj->value[0]);
  if (to_room == NOWHERE)
  {
    send_to_char("Bug with portal!  Tell a god ASAP!!\r\n", ch);
    logit(LOG_OBJ, "bs_portal() had portal with obj->value[0] of NOWHERE!");
    return TRUE;
  }
  if(!can_enter_room(ch, to_room, FALSE) ||
    (ch && !is_Raidable(ch, 0, 0)))
  {
    send_to_char("An unseen force pushes you back!\r\n", ch);
    return TRUE;
  }
  else
  {
    act("$p suddenly &+Yglows&n brightly!", FALSE, ch, obj, 0, TO_ROOM);
    act("$n enters $p and fades into the ether.", TRUE, ch, obj, 0, TO_ROOM);

    act("Your mind and body are overcome with seizures of pain!\r\n"
        "In the blink of an eye you are whisked away...",
        FALSE, ch, 0, 0, TO_CHAR);
    char_from_room(ch);
    act("You enter $p and reappear elsewhere.", FALSE, ch, obj, 0, TO_CHAR);
    char_to_room(ch, to_room, -1);
    act("$n slowly fades into view.", TRUE, ch, 0, 0, TO_ROOM);

    if (!IS_TRUSTED(ch))
    {
      temp = number(1, 20);
      if ((GET_HIT(ch) - temp) < -10)
      {
        send_to_char("The stress of the magic proves too much for you!\r\n",
                     ch);
        logit(LOG_DEATH, "%s died to portal in room %d.",
              GET_NAME(ch), world[from_room].number);
        die(ch);
        return TRUE;
      }
      else
      {
        GET_HIT(ch) -= temp;
      }

      GET_VITALITY(ch) -= number(1, 30);
      if (GET_VITALITY(ch) < 0)
        GET_VITALITY(ch) = 0;

#if 0
      GET_ALIGNMENT(ch) -= number(1, 5);
      if (GET_ALIGNMENT(ch) < -1000)
        GET_ALIGNMENT(ch) = -1000;

      act("You feel weakened and tainted by your passage through $p.",
          FALSE, ch, obj, 0, TO_CHAR);
#endif
      act("You feel weakened by your passage through $p.",
          FALSE, ch, obj, 0, TO_CHAR);

    }
    return TRUE;
  }
}

/*
   This spec proc "zaps" anyone trying to pick up or drag a child from a
   * sacrificial altar in Bloodstone, causing minor damage.  If ch is
   * really low on hit points, death ensues.
   *
   * NOTE: If a keyword besides "child" is added, change the check below.
   * -- DTS 2/22/95
 */
int bs_child_sacrifice(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam;
  char     buf[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || !AWAKE(ch))
    return FALSE;

  if ((cmd != CMD_TAKE) && (cmd != CMD_GET) && (cmd != CMD_DRAG))
    return FALSE;

  if (!OBJ_ROOM(obj))
    return FALSE;

  one_argument(arg, buf);
  arg = buf;

  if (!str_cmp(arg, "child") || !str_cmp(arg, "all"))
  {
    act("You attempt to pick up $p...\r\n"
        "Suddenly, a wave of revulsion rushes through you!\r\n"
        "You drop $p, leaving your hands &+rreddened&n and &+rsore&n.",
        FALSE, ch, obj, 0, TO_CHAR);
    act("As $n attempts to pick up $p, $e shudders and suddenly drops it.",
        TRUE, ch, obj, 0, TO_ROOM);

    dam = number(1, 9);
    if (GET_HIT(ch) - dam < -10)
    {
      act("Your wounds prove too much for you!", FALSE, ch, 0, 0, TO_CHAR);
      act("$n's wounds prove too much for $m!", TRUE, ch, 0, 0, TO_ROOM);
      logit(LOG_DEATH, "%s died while picking up %s in room %d.",
            GET_NAME(ch), obj->short_description, world[ch->in_room].number);
      die(ch);
      return TRUE;
    }
    else
      GET_HIT(ch) -= dam;

    return TRUE;
  }
  else
    return FALSE;
}

/*
   ROOM PROCS 
 */

int bs_dispersement_room(int room, P_char ch, int cmd, char *arg)
{

  P_char   temp, next;
  int      rroom;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch || cmd)
    return FALSE;

  rroom = real_room(room);
  if (rroom == NOWHERE)
    return FALSE;

  for (temp = world[rroom].people; temp; temp = next)
  {
    next = temp->next_in_room;

    if (IS_NPC(temp) && !IS_FIGHTING(temp))
    {
      if (room == 7205)
      {

        act("$n touches the wooden cross and is turned to dust.",
            TRUE, temp, 0, 0, TO_ROOM);
        char_from_room(temp);
        char_to_room(temp, real_room(7737), 0);
        return TRUE;

      }
      else if (room == 7656)
      {

        act("$n disappears into the thick forest.", TRUE, temp, 0, 0,
            TO_ROOM);
        char_from_room(temp);
        char_to_room(temp, real_room(7716), 0);
        return TRUE;

      }
      else
      {
        logit(LOG_DEBUG, "Undefined room in bs_dispersement_room().");
        return TRUE;
      }
    }
  }
  return FALSE;
}

/*
   This proc raises the gate to Bloodstone at 6 AM and lowers it again at
   * 9 PM, blocking passage into or out of the city.
   * -- DTS 2/23/95
 */
// this function isn't used anymore, commenting it out
//void bs_gate(void)
//{
//  if (!current_event || (current_event->type != EVENT_SPECIAL))
//  {
//    logit(LOG_EXIT, "Call to bs_gate with no current_event.");
//    raise(SIGSEGV);
//  }
//  /*
//     SAM 2-25-95, check to prevent core dump 
//   */
//  if ((real_room(22301) == -1) || (real_room(22302) == -1))
//  {
//    logit(LOG_DEBUG,
//          "Call bs_gate, but either room 22301 or 22302 doesn't exist.");
//    return;
//  }
//  if (time_info.hour == 6)
//  {
//
//    if (IS_SET(world[real_room(22301)].dir_option[NORTH]->exit_info,
//               EX_CLOSED))
//    {
//      REMOVE_BIT(world[real_room(22301)].dir_option[NORTH]->exit_info,
//                 EX_CLOSED);
//      send_to_room
//        ("The gate to the north raises slowly, permitting passage into the city for the day.\r\n",
//         real_room(22301));
//    }
//    if (IS_SET(world[real_room(22302)].dir_option[SOUTH]->exit_info,
//               EX_CLOSED))
//    {
//      REMOVE_BIT(world[real_room(22302)].dir_option[SOUTH]->exit_info,
//                 EX_CLOSED);
//      send_to_room
//        ("The gate to the south raises slowly, permitting passage out of the city for the day.\r\n",
//         real_room(22302));
//    }
//  }
//  else if (time_info.hour == 21)
//  {
//
//    if (!IS_SET(world[real_room(22301)].dir_option[NORTH]->exit_info,
//                EX_CLOSED))
//    {
//      SET_BIT(world[real_room(22301)].dir_option[NORTH]->exit_info,
//              EX_CLOSED);
//      send_to_room
//        ("The gate to the north lowers slowly, striking the ground with a dull clang.\r\n",
//         real_room(22301));
//    }
//    if (!IS_SET(world[real_room(22302)].dir_option[SOUTH]->exit_info,
//                EX_CLOSED))
//    {
//      SET_BIT(world[real_room(22302)].dir_option[SOUTH]->exit_info,
//              EX_CLOSED);
//      send_to_room
//        ("The gate to the south lowers slowly, striking the ground with a dull clang.\r\n",
//         real_room(22302));
//    }
//  }
//  current_event->timer += 1;
//}

int bs_altar(int room, P_char ch, int cmd, char *arg)
{
  int      dam, move_sub;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return (FALSE);

  if (!ch)
    return (FALSE);

  if (cmd == CMD_KNEEL)
  {
    dam = dice(2, 8) + 5;
    move_sub = dice(2, 20);
    act("As you kneel before this sigil of black power a feeling of", FALSE,
        ch, 0, 0, TO_CHAR);
    act("dizzyness overcomes you. You black out as you fall to the", FALSE,
        ch, 0, 0, TO_CHAR);
    act("cold marble floor...", FALSE, ch, 0, 0, TO_CHAR);
    act("$N kneels before the altar and is suddenly overcome by a", TRUE, ch,
        0, 0, TO_ROOM);
    act("wave dizziness. $N falls to the ground and disappears.", TRUE, ch, 0,
        0, TO_ROOM);
    SET_POS(ch, POS_PRONE + GET_STAT(ch));
    char_from_room(ch);
    char_to_room(ch, real_room(22495), -2);
    if ((GET_HIT(ch) - dam) < -10)
    {
      act("The power of the altar has destroyed your body...", FALSE, ch, 0,
          0, TO_CHAR);
      act("$N suddenly appears in the room, well $S corpse anyhow...", TRUE,
          ch, 0, 0, TO_ROOM);
      logit(LOG_DEATH, "%S killed by altar in %d.", GET_NAME(ch),
            world[ch->in_room].number);
      die(ch);
      return TRUE;
    }
    else
    {
      GET_HIT(ch) -= dam;
      if ((GET_VITALITY(ch) - move_sub) < 0)
        GET_VITALITY(ch) = 0;
      else
        GET_VITALITY(ch) -= move_sub;

      act("The unconscious form of $N suddenly appears in the room.", TRUE,
          ch, 0, 0, TO_ROOM);
      act("$N awakens.", TRUE, ch, 0, 0, TO_ROOM);
      act("You wake to find yourself in a darkened chamber.", FALSE, ch, 0, 0,
          TO_CHAR);
      return TRUE;
    }
  }
  return FALSE;
}

int bs_whirlpool(int room, P_char ch, int cmd, char *arg)
{
  P_char   victim, next_vict;
  int      dam;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd > 4)
    return FALSE;

  if (cmd == CMD_PERIODIC)
  {
    for (victim = world[real_room0(room)].people; victim; victim = next_vict)
    {
      next_vict = victim->next_in_room;
      if IS_NPC
        (victim)
      {
        act("You struggle furiously, but are dragged down by the whirlpool.",
            FALSE, victim, 0, 0, TO_CHAR);
        act("$n struggles furiously, but is dragged down into the whirlpool.",
            TRUE, victim, 0, 0, TO_ROOM);
        extract_char(victim);
        victim = NULL;
        continue;
      }
      if (!IS_TRUSTED(victim) && !IS_AFFECTED(victim, AFF_LEVITATE) &&
          !IS_AFFECTED(victim, AFF_FLY))
      {
        dam = dice(1, 7) + 6;
        if ((GET_HIT(victim) - dam) < -10)
        {
          act
            ("You struggle furiously, but are dragged down by the whirlpool.",
             FALSE, victim, 0, 0, TO_CHAR);
          act
            ("$n struggles furiously, but is dragged down into the whirlpool.",
             TRUE, victim, 0, 0, TO_ROOM);
          logit(LOG_DEATH, "%s killed by whirlpool in %d.", GET_NAME(victim),
                world[victim->in_room].number);
          char_from_room(victim);
          char_to_room(victim, real_room(22424), -2);
          act("The corpse of $n washes up on the shore.", TRUE, victim, 0, 0,
              TO_ROOM);
          die(victim);
          return TRUE;
        }
        else
        {
          act("The whirlpool tosses you violently about.", FALSE, victim, 0,
              0, TO_CHAR);
          act("The whirlpool tosses $n violently about.", TRUE, victim, 0, 0,
              TO_ROOM);
          GET_HIT(victim) -= dam;
          if ((GET_VITALITY(victim) - 10) < 0)
            GET_VITALITY(victim) = 0;
          else
            GET_VITALITY(victim) -= 10;
        }
      }
    }
  }
  else if (arg)
  {
    victim = ch;
    if (!IS_TRUSTED(victim) && !IS_AFFECTED(victim, AFF_LEVITATE) &&
        !IS_AFFECTED(victim, AFF_FLY))
    {
      if (dice(1, 100) > 85)
      {
        act("You manage to struggle free from the grasp of the whirlpool.",
            FALSE, victim, 0, 0, TO_CHAR);
        act("$n manages to struggle free from the grasp of the whirlpool.",
            TRUE, victim, 0, 0, TO_ROOM);
        return FALSE;
      }
      else
      {
        act("You struggle furiously to free yourself from the whirlpool.",
            FALSE, victim, 0, 0, TO_CHAR);
        act("$n struggles furiously to free $sself from the whirlpool.", TRUE,
            victim, 0, 0, TO_ROOM);
        CharWait(ch, 10);
        return TRUE;
      }
    }
  }
  return FALSE;
}
