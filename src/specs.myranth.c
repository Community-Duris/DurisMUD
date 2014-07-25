
/*
   ***************************************************************************
   *  File: specs.myranth.c                                    Part of Duris *
   *  Usage: special procedures for myranthea zones                            *
   *  Copyright  1995 - Duris Systems Ltd.                                   *
   *************************************************************************** 
 */

#include <ctype.h>
#include <signal.h>
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
extern char *command[];
extern const char *dirs[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int planes_room_num[];
extern int innate_abilities[];
extern int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

int myr_alexander(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "bessandra";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I think I need some more Bastine Knights!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "If only I could find a sturdy fighter...");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "There seems to be a lack of worthy knights these days.");
    }
  case 4:
    {
      do_action(ch, 0, CMD_EYEBROW);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_POSE);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_SIGH);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, 0, CMD_THINK);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, 0, CMD_PONDER);
      return TRUE;
    }
  case 10:
    {
      do_action(ch, Cbuf, CMD_KISS);
      return TRUE;
    }
  case 11:
    {
      do_action(ch, Cbuf, CMD_FLIRT);
      return TRUE;
    }
  case 12:
    {
      do_action(ch, Cbuf, CMD_WINK);
      return TRUE;
    }
  case 13:
    {
      do_action(ch, 0, CMD_HAPPY);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_duke(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Hmph!! You should be bowing in my presence!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "You are the goo beneath my shoe!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Hah! You are but a ragged civilian!");
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_YAWN);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_BORED);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_EYEBROW);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, 0, CMD_SMIRK);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_questing(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "aligator", Cbuf2[] = "mist", Cbuf3[] = "muck", Cbuf4[] =
    "skeleton", Cbuf5[] = "swamp";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_SNARL);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_HUNGER);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_DROOL);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_SNEER);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_SLOBBER);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_SNORT);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, Cbuf3, CMD_BITE);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf2, CMD_BITE);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, Cbuf5, CMD_BITE);
      return TRUE;
    }
  case 10:
    {
      do_action(ch, Cbuf4, CMD_BITE);
      return TRUE;
    }
  case 11:
    {
      do_action(ch, Cbuf1, CMD_BITE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_bessandra(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_COMB);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf, CMD_KISS);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, Cbuf, CMD_MASSAGE);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, Cbuf, CMD_LOVE);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, Cbuf, CMD_CURTSEY);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, Cbuf, CMD_FLIRT);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_viznor(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_MOAN);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_GROAN);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_gowell(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Pah! I will avenge my brothers wounds!!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch,
             "Someday, I will kill Sir Lagarias, and avenge my brothers!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "I killed King Ventor, and I will do the same to Sir Lagarias ! ");
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, Cbuf, CMD_SALUTE);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf, CMD_BOW);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_gareth(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch,
             "If only my brother Sir Gowell and Sir Lagarias could get along.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch,
             "Someday, I will bring peace to my best friend and brother.");
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_gaheris(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I must challenge Sir Lagarias to another duel!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "How can I live with this embarassment?");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I must find a way to return my honor as a jouster!");
      return TRUE;
    }
  case 4:
    {
      mobsay(ch, "By the gods, I hate Sir Lagarias!");
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_SIGH);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 10:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_lagarias(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I am, indeed, the best jouster in all of Volheru!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch,
             "I shall impale all who stand against me in the jousting ring ! ");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "I have killed 40 knights with my pole, and am willing to add to that sum ! ");
      return TRUE;
    }
  case 4:
    {
      mobsay(ch, "I shall always be loyal to my king, His Majesty!");
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf, CMD_SALUTE);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, Cbuf, CMD_BOW);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_golahy(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I alone have passed the Test of Knights!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "I warn any against attempting the Test of Knights!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "My shield and sword are truly the best in the land.");
      return TRUE;
    }
  case 4:
    {
      mobsay(ch,
             "I shall not be stopped in any of my endeavors for the King!");
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 10:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_bors(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Hah! Golahy thinks he passed the Test of Knights alone!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch,
             "So easily has Sir Golahy forgotten my aid in the Test of Knights ! ");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I eat mages for breakfast!");
      return TRUE;
    }
  case 4:
    {
      mobsay(ch, "A true Bastine Knight carries many scars of battle!");
      return TRUE;
    }
  case 5:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_lyone(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I am one of the few who has completed the Knight's Test!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Sir Golahy is truly a braggart!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I miss my father-- he is a true warrior.");
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_SIGH);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_lucas(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "All must report their endeavors to me!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "I keep this castle in tip top shape!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Any attempts to gain fame must come through me first!");
      return TRUE;
    }
  case 4:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_bantin(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king", Cbuf3[] = "bantan";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "With the proper heart, anyone can be trained.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "If one thrusts forward too fast, they will lose balance.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "The Knight of Two Swords shall train any who are willing!");
      return TRUE;
    }
  case 4:
    {
      mobsay(ch, "Any enemy of the King shall earn a quick and easy death!");
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 10:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 11:
    {
      do_action(ch, Cbuf3, CMD_PUNCH);
      return TRUE;
    }
  case 12:
    {
      do_action(ch, Cbuf3, CMD_FLIP);
      return TRUE;
    }
  case 13:
    {
      do_action(ch, Cbuf3, CMD_IMPALE);
      return TRUE;
    }
  case 14:
    {
      do_action(ch, Cbuf3, CMD_TACKLE);
      return TRUE;
    }
  case 15:
    {
      do_action(ch, Cbuf3, CMD_DROPKICK);
      return TRUE;
    }
  case 16:
    {
      do_action(ch, 0, CMD_SWEAT);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_bantan(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king", Cbuf3[] = "bantin";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I can train any in the uses of a shield.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "One must move the feet properly to avoid blows.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "My brother and I spar all of the time to improve our skills.");
      return TRUE;
    }
  case 4:
    {
      mobsay(ch, "Speed is the key to the proper defense.");
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 10:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 11:
    {
      do_action(ch, Cbuf3, CMD_PUNCH);
      return TRUE;
    }
  case 12:
    {
      do_action(ch, Cbuf3, CMD_POUNCE);
      return TRUE;
    }
  case 13:
    {
      do_action(ch, Cbuf3, CMD_SWEEP);
      return TRUE;
    }
  case 14:
    {
      do_action(ch, Cbuf3, CMD_STRANGLE);
      return TRUE;
    }
  case 15:
    {
      do_action(ch, Cbuf3, CMD_THROW);
      return TRUE;
    }
  case 16:
    {
      do_action(ch, Cbuf3, CMD_TRIP);
      return TRUE;
    }
  case 17:
    {
      do_action(ch, 0, CMD_SWEAT);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_gracian(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Pah! All of these humans are but weaklings to my edge!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "I am homesick-- my heart yearns for Myranthea.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "I love to run my sword through weakling humans that defy the King ! ");
      return TRUE;
    }
  case 4:
    {
      mobsay(ch, "Despite my heritage, I love being a Bastine Knight.");
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_SIGH);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 10:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_bedivere(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_badovin(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king", Cbuf3[] = "ulphius";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_SMIRK);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_SCOLD);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, Cbuf3, CMD_PROTECT);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_ulphius(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_THINK);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_PONDER);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_brastius(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_dagonet(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_dynadan(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_mellyagraunce(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_gryfflette(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_bagdemagus(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_torre(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_accolon(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_ladynas(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 5:
    {
      mobsay(ch, "Someday, I will be the best jouster!");
      return TRUE;
    }
  case 6:
    {
      mobsay(ch, "Someday, I will be just like Sir Lagarias!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_placidas(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "lagarias", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_BOW);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 4:
    {
      mobsay(ch, "I wish I could joust like Sir Lagarias!");
      return TRUE;
    }
  case 5:
    {
      mobsay(ch, "I love the feeling of dislodging a foe!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_goblin(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_DROOL);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_ogre(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_SLOBBER);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_DUH);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_SNORT);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_esmerelda(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_CACKLE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_SMIRK);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_SNEER);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_GRUMBLE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_abigail(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_CACKLE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_SMIRK);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_SNEER);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_GRUMBLE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_pontif(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_EYEBROW);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_THINK);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_PONDER);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_witch(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_CACKLE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_ben(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "richard";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_STRETCH);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_WHISTLE);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_MUTTER);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, Cbuf, CMD_NOOGIE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_winston(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_LAUGH);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_BEER);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_ROFL);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_HUNGER);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_HAPPY);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_SMILE);

      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_castleguard(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "captain", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_jester(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "me", Cbuf2[] = "king", Cbuf3[] = "queen";
  char     Cbuf4[] = "butler", Cbuf5[] = "maid", Cbuf6[] = "noble";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_CACKLE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_BURP);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_BOUNCE);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, Cbuf1, CMD_BOUNCE);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_GIGGLE);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_HAPPY);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, 0, CMD_HOP);
      return TRUE;
    }
  case 8:
    {
      do_action(ch, 0, CMD_JUMP);
      return TRUE;
    }
  case 9:
    {
      do_action(ch, 0, CMD_LAUGH);
      return TRUE;
    }
  case 10:
    {
      do_action(ch, 0, CMD_ROFL);
      return TRUE;
    }
  case 11:
    {
      do_action(ch, 0, CMD_ROLL);
      return TRUE;
    }
  case 12:
    {
      do_action(ch, 0, CMD_SING);
      return TRUE;
    }
  case 13:
    {
      do_action(ch, 0, CMD_SKIP);
      return TRUE;
    }
  case 14:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 15:
    {
      do_action(ch, 0, CMD_TWIRL);
      return TRUE;
    }
  case 16:
    {
      do_action(ch, 0, CMD_SPIN);
      return TRUE;
    }
  case 17:
    {
      do_action(ch, 0, CMD_WIGGLE);
      return TRUE;
    }
  case 18:
    {
      do_action(ch, 0, CMD_YODEL);
      return TRUE;
    }
  case 19:
    {
      do_action(ch, Cbuf2, CMD_BONK);
      return TRUE;
    }
  case 20:
    {
      do_action(ch, Cbuf3, CMD_BONK);
      return TRUE;
    }
  case 21:
    {
      do_action(ch, Cbuf4, CMD_BONK);
      return TRUE;
    }
  case 22:
    {
      do_action(ch, Cbuf5, CMD_BONK);
      return TRUE;
    }
  case 23:
    {
      do_action(ch, Cbuf6, CMD_BONK);
      return TRUE;
    }
  case 24:
    {
      do_action(ch, Cbuf2, CMD_FLIP);
      return TRUE;
    }
  case 25:
    {
      do_action(ch, Cbuf3, CMD_FLIP);
      return TRUE;
    }
  case 26:
    {
      do_action(ch, Cbuf4, CMD_FLIP);
      return TRUE;
    }
  case 27:
    {
      do_action(ch, Cbuf5, CMD_FLIP);
      return TRUE;
    }
  case 28:
    {
      do_action(ch, Cbuf6, CMD_FLIP);
      return TRUE;
    }
  case 29:
    {
      do_action(ch, Cbuf2, CMD_FLIRT);
      return TRUE;
    }
  case 30:
    {
      do_action(ch, Cbuf3, CMD_FLIRT);
      return TRUE;
    }
  case 31:
    {
      do_action(ch, Cbuf4, CMD_FLIRT);
      return TRUE;
    }
  case 32:
    {
      do_action(ch, Cbuf5, CMD_FLIRT);
      return TRUE;
    }
  case 33:
    {
      do_action(ch, Cbuf6, CMD_FLIRT);
      return TRUE;
    }
  case 34:
    {
      do_action(ch, Cbuf2, CMD_IMITATE);
      return TRUE;
    }
  case 35:
    {
      do_action(ch, Cbuf3, CMD_IMITATE);
      return TRUE;
    }
  case 36:
    {
      do_action(ch, Cbuf4, CMD_IMITATE);
      return TRUE;
    }
  case 37:
    {
      do_action(ch, Cbuf5, CMD_IMITATE);
      return TRUE;
    }
  case 38:
    {
      do_action(ch, Cbuf6, CMD_IMITATE);
      return TRUE;
    }
  case 39:
    {
      do_action(ch, Cbuf2, CMD_MOON);
      return TRUE;
    }
  case 40:
    {
      do_action(ch, Cbuf3, CMD_MOON);
      return TRUE;
    }
  case 41:
    {
      do_action(ch, Cbuf4, CMD_MOON);
      return TRUE;
    }
  case 42:
    {
      do_action(ch, Cbuf5, CMD_MOON);
      return TRUE;
    }
  case 43:
    {
      do_action(ch, Cbuf6, CMD_MOON);
      return TRUE;
    }
  case 44:
    {
      do_action(ch, Cbuf2, CMD_FLASH);
      return TRUE;
    }
  case 45:
    {
      do_action(ch, Cbuf3, CMD_FLASH);
      return TRUE;
    }
  case 46:
    {
      do_action(ch, Cbuf4, CMD_FLASH);
      return TRUE;
    }
  case 47:
    {
      do_action(ch, Cbuf5, CMD_FLASH);
      return TRUE;
    }
  case 48:
    {
      do_action(ch, Cbuf6, CMD_FLASH);
      return TRUE;
    }
  case 49:
    {
      do_action(ch, Cbuf2, CMD_PINCH);
      return TRUE;
    }
  case 50:
    {
      do_action(ch, Cbuf3, CMD_PINCH);
      return TRUE;
    }
  case 51:
    {
      do_action(ch, Cbuf4, CMD_PINCH);
      return TRUE;
    }
  case 52:
    {
      do_action(ch, Cbuf5, CMD_PINCH);
      return TRUE;
    }
  case 53:
    {
      do_action(ch, Cbuf6, CMD_PINCH);
      return TRUE;
    }
  case 54:
    {
      do_action(ch, Cbuf2, CMD_NOOGIE);
      return TRUE;
    }
  case 55:
    {
      do_action(ch, Cbuf3, CMD_NOOGIE);
      return TRUE;
    }
  case 56:
    {
      do_action(ch, Cbuf4, CMD_NOOGIE);
      return TRUE;
    }
  case 57:
    {
      do_action(ch, Cbuf5, CMD_NOOGIE);
      return TRUE;
    }
  case 58:
    {
      do_action(ch, Cbuf6, CMD_NOOGIE);
      return TRUE;
    }
  case 59:
    {
      do_action(ch, Cbuf2, CMD_RUFFLE);
      return TRUE;
    }
  case 60:
    {
      do_action(ch, Cbuf3, CMD_RUFFLE);
      return TRUE;
    }
  case 61:
    {
      do_action(ch, Cbuf4, CMD_RUFFLE);
      return TRUE;
    }
  case 62:
    {
      do_action(ch, Cbuf5, CMD_RUFFLE);
      return TRUE;
    }
  case 63:
    {
      do_action(ch, Cbuf6, CMD_RUFFLE);
      return TRUE;
    }
  case 64:
    {
      do_action(ch, Cbuf2, CMD_SLAP);
      return TRUE;
    }
  case 65:
    {
      do_action(ch, Cbuf3, CMD_SLAP);
      return TRUE;
    }
  case 66:
    {
      do_action(ch, Cbuf4, CMD_SLAP);
      return TRUE;
    }
  case 67:
    {
      do_action(ch, Cbuf5, CMD_SLAP);
      return TRUE;
    }
  case 68:
    {
      do_action(ch, Cbuf6, CMD_SLAP);
      return TRUE;
    }
  case 69:
    {
      do_action(ch, Cbuf2, CMD_TICKLE);
      return TRUE;
    }
  case 70:
    {
      do_action(ch, Cbuf3, CMD_TICKLE);
      return TRUE;
    }
  case 71:
    {
      do_action(ch, Cbuf4, CMD_TICKLE);
      return TRUE;
    }
  case 72:
    {
      do_action(ch, Cbuf5, CMD_TICKLE);
      return TRUE;
    }
  case 73:
    {
      do_action(ch, Cbuf6, CMD_TICKLE);
      return TRUE;
    }
  case 74:
    {
      do_action(ch, Cbuf2, CMD_TOSS);
      return TRUE;
    }
  case 75:
    {
      do_action(ch, Cbuf3, CMD_TOSS);
      return TRUE;
    }
  case 76:
    {
      do_action(ch, Cbuf4, CMD_TOSS);
      return TRUE;
    }
  case 77:
    {
      do_action(ch, Cbuf5, CMD_TOSS);
      return TRUE;
    }
  case 78:
    {
      do_action(ch, Cbuf6, CMD_TOSS);
      return TRUE;
    }
  case 79:
    {
      do_action(ch, Cbuf2, CMD_TRIP);
      return TRUE;
    }
  case 80:
    {
      do_action(ch, Cbuf3, CMD_TRIP);
      return TRUE;
    }
  case 81:
    {
      do_action(ch, Cbuf4, CMD_TRIP);
      return TRUE;
    }
  case 82:
    {
      do_action(ch, Cbuf5, CMD_TRIP);
      return TRUE;
    }
  case 83:
    {
      do_action(ch, Cbuf6, CMD_TRIP);
      return TRUE;
    }
  case 84:
    {
      do_action(ch, Cbuf2, CMD_TUG);
      return TRUE;
    }
  case 85:
    {
      do_action(ch, Cbuf3, CMD_TUG);
      return TRUE;
    }
  case 86:
    {
      do_action(ch, Cbuf4, CMD_TUG);
      return TRUE;
    }
  case 87:
    {
      do_action(ch, Cbuf5, CMD_TUG);
      return TRUE;
    }
  case 88:
    {
      do_action(ch, Cbuf6, CMD_TUG);
      return TRUE;
    }
  case 89:
    {
      do_action(ch, Cbuf2, CMD_TWEAK);
      return TRUE;
    }
  case 90:
    {
      do_action(ch, Cbuf3, CMD_TWEAK);
      return TRUE;
    }
  case 91:
    {
      do_action(ch, Cbuf4, CMD_TWEAK);
      return TRUE;
    }
  case 92:
    {
      do_action(ch, Cbuf5, CMD_TWEAK);
      return TRUE;
    }
  case 93:
    {
      do_action(ch, Cbuf6, CMD_TWEAK);
      return TRUE;
    }
  case 94:
    {
      do_action(ch, Cbuf2, CMD_WINK);
      return TRUE;
    }
  case 95:
    {
      do_action(ch, Cbuf3, CMD_WINK);
      return TRUE;
    }
  case 96:
    {
      do_action(ch, Cbuf4, CMD_WINK);
      return TRUE;
    }
  case 97:
    {
      do_action(ch, Cbuf5, CMD_WINK);
      return TRUE;
    }
  case 98:
    {
      do_action(ch, Cbuf6, CMD_WINK);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_noble(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_STRUT);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_SMIRK);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_maid(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_GRUMBLE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_butler(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_GRUMBLE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_castlecook(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_GRUMBLE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_HUM);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_WHISTLE);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_SPIT);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_CURSE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_holyman(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_BOW);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_holywoman(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_BOW);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_doctor(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_PONDER);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_THINK);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_dying(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_BLEED);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_CHOKE);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_WHEEZE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_brealemerchant(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I sell the finest wares!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_myranmerchant(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "No elf can do without what I've got for sale!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_fishvendor(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Get your fresh fish here!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_fruitvendor(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Volheru's freshest fruit sold here!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_jeweler(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Get your fine jewelery!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_cityguard(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "captain";

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_grandpa(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "child";

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf, CMD_SPANK);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_grandma(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "child";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf, CMD_SPANK);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_brim(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_SWEAT);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_WHISTLE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_monty(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_BEER);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_BURP);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_HICCUP);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_palaceguard(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf1[] = "captain", Cbuf2[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf1, CMD_SALUTE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf2, CMD_SALUTE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_human(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_GROVEL);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_king(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "queen";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_HAPPY);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf, CMD_KISS);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, Cbuf, CMD_FLIRT);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_princess(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "me";

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, Cbuf, CMD_COMB);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_eileor(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "king";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_HAPPY);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_SMILE);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, Cbuf, CMD_KISS);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, Cbuf, CMD_FLIRT);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, Cbuf, CMD_TICKLE);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, Cbuf, CMD_CURTSEY);
      return TRUE;
    }
  case 7:
    {
      do_action(ch, Cbuf, CMD_LOVE);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_sick(P_char ch, P_char pl, int cmd, char *arg)
{
/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_PUKE);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, 0, CMD_WHEEZE);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_SNEEZE);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_SPIT);
      return TRUE;
    }
  case 5:
    {
      do_action(ch, 0, CMD_CHOKE);
      return TRUE;
    }
  case 6:
    {
      do_action(ch, 0, CMD_BURP);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_prisoner(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Cbuf[] = "me";

/*
   check for periodic event calls 
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      do_action(ch, 0, CMD_TARZAN);
      return TRUE;
    }
  case 2:
    {
      do_action(ch, Cbuf, CMD_TARZAN);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_BOUNCE);
      return TRUE;
    }
  case 4:
    {
      do_action(ch, 0, CMD_JUMP);
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int myr_hagatha(P_char ch, P_char pl, int cmd, char *arg)
{

  int      i = FALSE;
  int      got_it = FALSE;
  int      bracelet_vr = 71637;
  P_obj    bracelet = NULL, key = NULL;
  P_char   dummy;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

/*
   if ((ch->equipment[WEAR_WRIST_R] && 
   (obj_index[ch->equipment[WEAR_WRIST_R]->R_num].virtual == bracelet_vr)) ||
   (ch->equipment[WEAR_WRIST_L] && 
   (obj_index[ch->equipment[WEAR_WRIST_L]->R_num].virtual == bracelet_vr)))
   return FALSE;
 */

  if (!pl)
  {
    for (dummy = world[ch->in_room].people; dummy;
         dummy = dummy->next_in_room)
    {
      if ((dummy->equipment[WEAR_WRIST_R] &&
           (obj_index[dummy->equipment[WEAR_WRIST_R]->R_num].virtual ==
            bracelet_vr)) || (dummy->equipment[WEAR_WRIST_L] &&
                              (obj_index
                               [dummy->equipment[WEAR_WRIST_L]->R_num].
                               virtual == bracelet_vr)))
      {
        i = TRUE;
      }
      if (i)
      {
        pl = dummy;
        break;
      }
    }
  }
  if (!pl || !ch)
    return FALSE;

  if (!AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if (!got_it && pl->equipment[WEAR_WRIST_R])
  {
    if (obj_index[pl->equipment[WEAR_WRIST_R]->R_num].virtual == bracelet_vr)
    {
      got_it = TRUE;
      bracelet = pl->equipment[WEAR_WRIST_R];
    }
  }
  if (!got_it && pl->equipment[WEAR_WRIST_L])
  {
    if (obj_index[pl->equipment[WEAR_WRIST_L]->R_num].virtual == bracelet_vr)
    {
      got_it = TRUE;
      bracelet = pl->equipment[WEAR_WRIST_L];
    }
  }
  if (!got_it)
    return FALSE;

  if (got_it && (pl->in_room == ch->in_room))
  {
    send_to_char
      ("\nHagatha's eyes open wide as she notices the triangular bracelet on your wrist.\n"
       "Her bony hands reach out to your wrist and grasp it with amazing strength.\n\n"
       "Without a word, she rips the bracelet off of your wrist and shoves a strange\n"
       "key into your grips. Hagatha pauses only briefy before destroying the bracelet\n\n",
       pl);
    act("\nHagatha's eyes open wide as she sees the triangular bracelet on $n's\n" "wrist. Her bony hands reach out and grip $s arms, tearing the bracelet off\n" "with amazing strength. Without a word, she shoves a strange key into\n" "$n's hands.  Hagatha pauses on briefly before destroying the bracelet.\n", TRUE, pl, 0, 0, TO_NOTVICT);     /*
                                                                                                                                                                                                                                                                                                                                                           \n 
                                                                                                                                                                                                                                                                                                                                                           is 
                                                                                                                                                                                                                                                                                                                                                           intentional 
                                                                                                                                                                                                                                                                                                                                                           at 
                                                                                                                                                                                                                                                                                                                                                           end 
                                                                                                                                                                                                                                                                                                                                                         */
    unequip_char(pl,
                 OBJ_WORN_POS(bracelet,
                              WEAR_WRIST_R) ? WEAR_WRIST_R : WEAR_WRIST_L);
/*
   obj_to_char(bracelet, ch);
   wear(ch, bracelet, 11); 
         *//*
       11 is the keyword for location WRIST 
     */
    extract_obj(bracelet);
    key = read_object(70010, VIRTUAL);
    obj_to_char(key, pl);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
  return FALSE;
}

int breale_townsfolk(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   TmpCh;
  char     TmpBuf[MAX_STRING_LENGTH];

  /*
     Check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || IS_FIGHTING(ch) || !AWAKE(ch))
    return FALSE;

  for (TmpCh = world[ch->in_room].people; TmpCh && !IS_FIGHTING(ch);
       TmpCh = TmpCh->next_in_room)
  {
    if (!IS_SET(TmpCh->specials.act, PLR_ANONYMOUS) &&
        (GET_CLASS(TmpCh) == CLASS_SHAMAN ||
         GET_CLASS(TmpCh) == CLASS_SORCERER ||
         GET_CLASS(TmpCh) == CLASS_NECROMANCER ||
         GET_CLASS(TmpCh) == CLASS_SUMMONER ||
         GET_CLASS(TmpCh) == CLASS_CONJURER) &&
        !should_not_kill(ch, TmpCh) &&
        !IS_SET(TmpCh->specials.act, PLR_AGGIMMUNE) && CAN_SEE(ch, TmpCh))
    {
      sprintf(TmpBuf,
              "We hate your kind here, %s! Leave town, %s, or die by my hand!\r\n",
              GET_NAME(TmpCh),
              GET_SEX(TmpCh) == SEX_MALE ? "warlock" : "witch");
      mobsay(ch, TmpBuf);
      MobStartFight(ch, TmpCh);
      return TRUE;
    }
  }
  return FALSE;
}

/*
   object procs 
 */

/*
   Warning: These magic numbers are also buried in the procedure. 
 */
const int tree_keys[] = {
  70013,
  70014,
  70015,
  70039
};

#define NUM_MYRANTH_KEYS 4

int myranth_key(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      count = 0;
  int      i = 0;
  P_obj    tmp_obj = NULL, next_obj;
  P_obj    tree_key, t_obj = NULL;
  char     Buf1[MAX_STRING_LENGTH], Buf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || !cmd || !arg)
    return FALSE;

  if (cmd != CMD_GIVE && cmd != CMD_GET && cmd != CMD_REMOVE &&
      cmd != CMD_TAKE)
    return FALSE;

  argument_interpreter(arg, Buf1, Buf2);

  if (!*Buf1)
  {
    send_to_char("Get what?\n", ch);
    return TRUE;
  }
  switch (cmd)
  {
  case CMD_REMOVE:
    t_obj = get_object_in_equip(ch, arg, &i);
    if (t_obj != obj && !strn_cmp(Buf1, "all", 3))
      return FALSE;
    do_remove(ch, arg, CMD_REMOVE);
    break;
  case CMD_GIVE:
    do_give(ch, arg, CMD_GIVE);
    break;
  case CMD_GET:
  case CMD_TAKE:
    if (*Buf1 && !Buf2)
    {                           /*
                                   One arg, assume get in room 
                                 */
      t_obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents);
      if (t_obj != obj && !strn_cmp(Buf1, "all", 3))
        return FALSE;
    }                           /*
                                   Otherwise from a bag. . . run the check 
                                 */
    do_get(ch, arg, CMD_GET);
    break;
  default:
    send_to_char("Major goof with funct myranth_key in specs.object.c,\n"
                 "please notify a coding god ([C]) ASAP!\n", ch);
    return TRUE;
    break;
  }

/*
   if (cmd == CMD_GIVE && !OBJ_CARRIED_BY(obj, ch)) 
 */
  if (cmd == CMD_GIVE && OBJ_CARRIED(obj) && !OBJ_CARRIED_BY(obj, ch))
    ch = obj->loc.carrying;

  for (tmp_obj = ch->carrying; tmp_obj; tmp_obj = next_obj)
  {
    next_obj = tmp_obj->next_content;
    switch (obj_index[tmp_obj->R_num].virtual)
    {                           /*
                                   It's not pretty, but 
                                   a switch is more
                                   efficient than using 
                                   another for loop 
                                 */
    case 70013:
    case 70014:
    case 70015:
    case 70039:
      count += 1;
      break;
    default:
      break;
    }
  }
  if (count == NUM_MYRANTH_KEYS)
  {
    act("\nAs $n touches the key, it and other small items $e is carrying\n"
        "burst into flames, dissolving strangely away, only to be replaced\n"
        "by a different key.", TRUE, ch, obj, 0, TO_ROOM);
    act
      ("\nAs you touch the key, it and other keys you are carrying burst into\n"
       "flames, dissolving strangely away, only to be replaced by a different key.\n",
       TRUE, ch, t_obj, 0, TO_CHAR);
    for (tmp_obj = ch->carrying; tmp_obj; tmp_obj = next_obj)
    {
      next_obj = tmp_obj->next_content;

      switch (obj_index[tmp_obj->R_num].virtual)
      {
      case 70013:
      case 70014:
      case 70015:
      case 70039:
        obj_from_char(tmp_obj);
        extract_obj(tmp_obj);
        break;
      default:
        break;
      }
    }
    tree_key = read_object(70016, VIRTUAL);
    obj_to_char(tree_key, ch);
  }
  return TRUE;
}

int blackness_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      ankh_vr = 70133;     /*
                                   Virtual Number of object that sword keys off 
                                   of 
                                 */
  int      got_it = FALSE, t_val;
  char     Buf1[MAX_STRING_LENGTH], Buf2[MAX_STRING_LENGTH];
  P_obj    ankh = NULL, sword;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || !cmd || !arg || !OBJ_ROOM(obj))
    return FALSE;

  if ((cmd != CMD_GET) && (cmd != CMD_TAKE))
    return FALSE;

  if (ch->equipment[WEAR_NECK_1])
    if (obj_index[ch->equipment[WEAR_NECK_1]->R_num].virtual == ankh_vr)
    {
      got_it = TRUE;
      ankh = ch->equipment[WEAR_NECK_1];
    }
  if (!got_it && ch->equipment[WEAR_NECK_2])
    if (obj_index[ch->equipment[WEAR_NECK_2]->R_num].virtual == ankh_vr)
    {
      got_it = TRUE;
      ankh = ch->equipment[WEAR_NECK_2];
    }
  if (!got_it)
    return FALSE;

  argument_interpreter(arg, Buf1, Buf2);

  if (*Buf2)
    return FALSE;

  got_it = FALSE;
  if (!str_cmp(Buf1, "all"))
    got_it = TRUE;
  else if ((sscanf(Buf1, "all.%s", Buf2) == 1) && isname(Buf2, obj->name))
    got_it = TRUE;
  else
  {
    t_val = 0;
    if (isname(Buf1, obj->name) ||
        ((sscanf(Buf1, "%d.%s", &t_val, Buf2) == 2) &&
         isname(Buf2, obj->name)))
    {
      if (!t_val)
      {
        t_val = 1;
        strcpy(Buf2, Buf1);
      }
      for (sword = world[obj->loc.room].contents; sword;
           sword = sword->next_content)
      {
        if (isname(Buf2, sword->name))
          t_val--;
        if (t_val <= 0)
        {
          if (sword == obj)
            got_it = TRUE;
          break;
        }
      }
    }
  }

  if (!got_it)
    return FALSE;

  if (IS_SET(obj->wear_flags, ITEM_TAKE))
    return FALSE;

  REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
  SET_BIT(obj->wear_flags, ITEM_TAKE);

  /*
     Ankh only works once, for whatever command. . . . 
   */
  unequip_char(ch,
               OBJ_WORN_POS(ankh, WEAR_NECK_1) ? WEAR_NECK_1 : WEAR_NECK_2);
  extract_obj(ankh);
  send_to_char("The ankh around your neck disappears with a hollow pop.\n\n",
               ch);

  return FALSE;
}

/*
   room procs 
 */

#define NUM_VOLHERU 28
const int volheru[] = {
  70995,
  70990,
  70985,
  70980,
  70975,
  70970,
  70965,
  70960,
  70955,
  70950,
  70945,
  70940,
  70935,
  70930,
  70925,
  70920,
  70915,
  70910,
  70905,
  70900,
  70895,
  70890,
  70885,
  70880,
  70875,
  70870,
  70867,
  70866
};

#define NUM_MORLANTHRA 8
const int morlanthra[] = {
  71430,
  71425,
  71420,
  71415,
  71410,
  71405,
  71400,
  71270
};

#define NUM_QUESTING 8
const int questing[] = {
  71685,
  71680,
  71675,
  71670,
  71665,
  71660,
  71655,
  71650
};

int myranth_maze(int room, P_char ch, int cmd, char *arg)
{
#if 0
  int      i;

  if (!(cmd < -40))             /*
                                   Bit of magic here. When players enter a
                                   room, the spec proc is called with a cmd
                                   argument of -50 + dir. 
                                 */
    return FALSE;

  if ((real_room(room) >= 70866) && (real_room(room) <= 70995))
  {
    for (i = 0; i < 4; i++)
      if (world[room].dir_option[i])
        world[room].dir_option[i]->to_room =
          volheru[number(0, NUM_VOLHERU - 1)];
  }
  else if ((real_room(room) >= 71270) && (real_room(room) <= 71430))
  {
    for (i = 0; i < 4; i++)
      if (world[room].dir_option[i])
        world[room].dir_option[i]->to_room =
          morlanthra[number(0, NUM_MORLANTHRA - 1)];
  }
  else if ((real_room(room) >= 71650) && (real_room(room) <= 71685))
  {
    for (i = 0; i < 4; i++)
      if (world[room].dir_option[i])
        world[room].dir_option[i]->to_room =
          questing[number(0, NUM_QUESTING - 1)];
  }
#endif
  return FALSE;
}
