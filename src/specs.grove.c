/*
   Procs for the shady grove orc hometown
 */

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

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
   external vars
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_event current_event;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern const struct stat_data stat_factor[];
extern int pulse;
extern int top_of_zone_table;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

int troll_slave(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 5))
      {
      case 2:
        act("$n fidgets with his bonds.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        mobsay(ch, "I'll soon be free!");
        act("$n tugs and pulls at $s chains madly!", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int stray_dog(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 5))
      {
      case 2:
        act("$n barks loudly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n sniffs the ground looking for something.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n looks at you warily.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int hardworking_fisherman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 5))
      {
      case 2:
        act("$n tosses $s line in the lake.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n snorts loudly as $e notices you standing here.", TRUE, ch, 0,
            0, TO_ROOM);
        mobsay(ch, "What do you want?! I haven't caught anything yet!");
        break;
      case 4:
        act("$n puts bait on $s line.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int orcish_jailkeeper(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 5))
      {
      case 2:
        act("$n peers around the room.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n looks at you curiously.", TRUE, ch, 0, 0, TO_ROOM);
        mobsay(ch, "What are you doing here?");
        break;
      case 4:
        act("$n checks around the jail.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int orcish_woman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return FALSE;
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 5))
      {
      case 2:
        act("$n mourns the lost of her dead.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n looks at you with tears in her eyes.", TRUE, ch, 0, 0,
            TO_ROOM);
        mobsay(ch, "Those hoomans did it, I know they did!");
        break;
      case 4:
        act("$n wipes tears from her eye.", TRUE, ch, 0, 0, TO_ROOM);
        mobsay(ch, "I'll get them for it I swear!");
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

/* specs.for vellas bordelo */


int topless_prostitute(P_char ch, P_char pl, int cmd, char *arg)
{
  struct affected_type af;
  int      gold;
  P_char   k;
  P_char   c_obj = 0;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl && (pl != ch))
  {
    if (cmd == CMD_GIVE)
    {                           /*
                                   give her the money 
                                 */
      gold = GET_MONEY(ch);
      do_give(pl, arg, 0);
      if ((gold = (GET_MONEY(ch) - gold)))
      {
        if (gold < 500)
          mobsay(ch,
                 "You STILL need to pay me 500 copper coins, baby cakes.");
        else
        {
          strcpy(Gbuf1, GET_NAME(pl));
          do_action(ch, Gbuf1, CMD_SMILE);      /*
                                                   smile 
                                                 */
          k = world[ch->in_room].people;
          while ((k != ch) && (k != pl) && k)
            k = k->next_in_room;
          if (!k)
          {


            logit(LOG_DEBUG, "Prostitute error 1!");
            return (TRUE);
          }
          act("$N walks up to you and gives you a passionate french kiss.",
              FALSE, pl, 0, ch, TO_CHAR);
          act
            ("$N whispers to you, 'I'm all yours, baby. Take me anywhere, I'll follow.",
             FALSE, pl, 0, ch, TO_CHAR);
          act
            ("$N walks up to $n, engages in a brief exchange, and kisses $m eagerly.",
             TRUE, pl, 0, ch, TO_NOTVICT);
          setup_pet(ch, pl, 10, 0);

          if (ch->following)
            stop_follower(ch);
          add_follower(ch, pl);
        }
      }
      return (TRUE);
    }
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 7))
      {
      case 2:
        act("$n wiggles $s chest at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        mobsay(ch, "Hey baby don't stare, take what you want.");
        break;
      case 4:
        act("$n looks you up and down, judging your abilities.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 5:
        act("$n whistles sexily at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        mobsay(ch, "You're not a two minute man are you?");
        act("$n snickers softly.", TRUE, ch, 0, 0, TO_ROOM);
      case 7:
        act("$n purposely bends down in front of you.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  /*
     These things will let the mob act based on what people do it it. 
   */
  if (cmd != 0)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);
    if (*Gbuf1)
      c_obj = get_char_room(Gbuf1, ch->in_room);
    if (IS_AGG_CMD(cmd))
    {
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      act("$N screams at you, 'Oh not good enough for you huh!'", FALSE, pl,
          0, ch, TO_CHAR);
      act("$N screams, 'Not good enough huh?!'", TRUE, pl, 0, ch, TO_ROOM);
      if (ch->following)
        stop_follower(ch);
      return FALSE;
    }
    switch (cmd)
    {
    case CMD_KISS:
    case CMD_FRENCH:
    case CMD_UNDRESS:
    case CMD_CARESS:
    case CMD_SEDUCE:
    case CMD_FONDLE:
    case CMD_GROPE:
    case CMD_DREAM:
    case CMD_DROOL:
    case CMD_OGLE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N whispers to you, '5 gold for the time of your life baby.'",
          FALSE, pl, 0, ch, TO_CHAR);
      act("$N whispers something to $n.", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_SMILE:
    case CMD_WINK:
      if (!IS_AWAKE(ch))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$n smiles evilly, rubbing $mself.", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
      break;
    case CMD_PUKE:
    case CMD_INSULT:
    case CMD_SLAP:
    case CMD_SPIT:
    case CMD_CURSE:
    case CMD_PUNCH:
    case CMD_CHOKE:
    case CMD_STRANGLE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N tells you, 'You faggot you're in the wrong place.'", FALSE, pl,
          0, ch, TO_CHAR);
      act("$N tells $n to find a place where faggots go.", TRUE, pl, 0, ch,
          TO_ROOM);
      return TRUE;
      break;
    case CMD_HUG:
    case CMD_SNUGGLE:
    case CMD_CUDDLE:
    case CMD_NUZZLE:
    case CMD_LICK:
    case CMD_LOVE:
    case CMD_NIBBLE:
    case CMD_SQUEEZE:
    case CMD_STARE:
    case CMD_MASSAGE:
    case CMD_SPANK:
    case CMD_WORSHIP:
    case CMD_PINCH:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N whispers to you, 'OoOOooOOOoo' excitedly.", FALSE, pl, 0, ch,
          TO_CHAR);
      act("$N whispers something to $n excitedly", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_WAKE:
      if (c_obj != ch)
        return FALSE;
      if (GET_STAT(ch) != STAT_SLEEPING)
        act("$N is not sleeping.", FALSE, pl, 0, ch, TO_CHAR);
      else
      {
        act
          ("Your gentle nudging awakens $N, smiles and says 'You ready for more?'",
           FALSE, pl, 0, ch, TO_CHAR);
        act("$n nudges $N awake.", FALSE, pl, 0, ch, TO_ROOM);
        SET_POS(ch, POS_STANDING + STAT_NORMAL);
      }
      return TRUE;
      break;
    case CMD_POKE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, arg, CMD_POKE);
      act("$N says 'HooHoo.'", FALSE, pl, 0, ch, TO_CHAR);
      act("$N says 'HooHoo.'", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    default:
      return FALSE;
      break;
    }
  }
  return (FALSE);
}

int sex_crazed_prostitute(P_char ch, P_char pl, int cmd, char *arg)
{
  struct affected_type af;
  int      gold;
  P_char   k;
  P_char   c_obj = 0;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl && (pl != ch))
  {
    if (cmd == CMD_GIVE)
    {                           /*
                                   give her the money 
                                 */
      gold = GET_MONEY(ch);
      do_give(pl, arg, 0);
      if ((gold = (GET_MONEY(ch) - gold)))
      {
        if (gold < 500)
          mobsay(ch,
                 "You STILL need to pay me 500 copper coins, baby cakes.");
        else
        {
          strcpy(Gbuf1, GET_NAME(pl));
          do_action(ch, Gbuf1, CMD_SMILE);      /*
                                                   smile 
                                                 */
          k = world[ch->in_room].people;
          while ((k != ch) && (k != pl) && k)
            k = k->next_in_room;
          if (!k)
          {


            logit(LOG_DEBUG, "Prostitute error 1!");
            return (TRUE);
          }
          act("$N walks up to you and gives you a passionate french kiss.",
              FALSE, pl, 0, ch, TO_CHAR);
          act
            ("$N whispers to you, 'I'm all yours, baby. Take me anywhere, I'll follow.'",
             FALSE, pl, 0, ch, TO_CHAR);
          act
            ("$N walks up to $n, engages in a brief exchange, and kisses $m eagerly.",
             TRUE, pl, 0, ch, TO_NOTVICT);
          setup_pet(ch, pl, 10, 0);
          if (ch->following)
            stop_follower(ch);
          add_follower(ch, pl);
        }
      }
      return (TRUE);
    }
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 7))
      {
      case 2:
        act("$n twitches as you walk by.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        mobsay(ch, "I want you! I want you now!");
        break;
      case 4:
        act("$n lifts a finger and vigorous motions you to near her.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 5:
        act("$n shakes violently trying to control herself.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 6:
        mobsay(ch, "Stop playing around and come here now!");
        act("$n grits her teeth in frustration.", TRUE, ch, 0, 0, TO_ROOM);
      case 7:
        act("$n grasps her legs to contain her emotions.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  /*
     These things will let the mob act based on what people do it it. 
   */
  if (cmd != 0)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);
    if (*Gbuf1)
      c_obj = get_char_room(Gbuf1, ch->in_room);
    if (IS_AGG_CMD(cmd))
    {
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      act("$N screams at you, 'Oh not good enough for you huh!'", FALSE, pl,
          0, ch, TO_CHAR);
      act("$N screams, 'Not good enough huh?!'", TRUE, pl, 0, ch, TO_ROOM);
      if (ch->following)
        stop_follower(ch);
      return FALSE;
    }
    switch (cmd)
    {
    case CMD_KISS:
    case CMD_FRENCH:
    case CMD_UNDRESS:
    case CMD_CARESS:
    case CMD_SEDUCE:
    case CMD_FONDLE:
    case CMD_GROPE:
    case CMD_DREAM:
    case CMD_DROOL:
    case CMD_OGLE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N whispers to you, '5 gold for the time of your life baby.'",
          FALSE, pl, 0, ch, TO_CHAR);
      act("$N whispers something to $n.", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_SMILE:
    case CMD_WINK:
      if (!IS_AWAKE(ch))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$n smiles evilly, rubbing herself.", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
      break;
    case CMD_PUKE:
    case CMD_INSULT:
    case CMD_SLAP:
    case CMD_SPIT:
    case CMD_CURSE:
    case CMD_PUNCH:
    case CMD_CHOKE:
    case CMD_STRANGLE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N tells you, 'You faggot! You're in the wrong place!'", FALSE, pl,
          0, ch, TO_CHAR);
      act("$N tells $n to find a place where faggots go.", TRUE, pl, 0, ch,
          TO_ROOM);
      return TRUE;
      break;
    case CMD_HUG:
    case CMD_SNUGGLE:
    case CMD_CUDDLE:
    case CMD_NUZZLE:
    case CMD_LICK:
    case CMD_LOVE:
    case CMD_NIBBLE:
    case CMD_SQUEEZE:
    case CMD_STARE:
    case CMD_MASSAGE:
    case CMD_SPANK:
    case CMD_WORSHIP:
    case CMD_PINCH:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N whispers to you, 'OoOOooOOOoo' excitedly.", FALSE, pl, 0, ch,
          TO_CHAR);
      act("$N whispers something to $n excitedly", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_WAKE:
      if (c_obj != ch)
        return FALSE;
      if (GET_STAT(ch) != STAT_SLEEPING)
        act("$N is not sleeping.", FALSE, pl, 0, ch, TO_CHAR);
      else
      {
        act
          ("Your gentle nudging awakens $N, smiles and says 'You ready for more?'",
           FALSE, pl, 0, ch, TO_CHAR);
        act("$n nudges $N awake.", FALSE, pl, 0, ch, TO_ROOM);
        SET_POS(ch, POS_STANDING + STAT_NORMAL);
      }
      return TRUE;
      break;
    case CMD_POKE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, arg, CMD_POKE);
      act("$N says 'HooHoo.'", FALSE, pl, 0, ch, TO_CHAR);
      act("$N says 'HooHoo.'", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    default:
      return FALSE;
      break;
    }
  }
  return (FALSE);
}

int well_built_prostitute(P_char ch, P_char pl, int cmd, char *arg)
{
  struct affected_type af;
  int      gold;
  P_char   k;
  P_char   c_obj = 0;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl && (pl != ch))
  {
    if (cmd == CMD_GIVE)
    {                           /*
                                   give her the money 
                                 */
      gold = GET_MONEY(ch);
      do_give(pl, arg, 0);
      if ((gold = (GET_MONEY(ch) - gold)))
      {
        if (gold < 500)
          mobsay(ch,
                 "You STILL need to pay me 500 copper coins, baby cakes.");
        else
        {
          strcpy(Gbuf1, GET_NAME(pl));
          do_action(ch, Gbuf1, CMD_SMILE);      /*
                                                   smile 
                                                 */
          k = world[ch->in_room].people;
          while ((k != ch) && (k != pl) && k)
            k = k->next_in_room;
          if (!k)
          {


            logit(LOG_DEBUG, "Prostitute error 1!");
            return (TRUE);
          }
          act("$N walks up to you and gives you a passionate french kiss.",
              FALSE, pl, 0, ch, TO_CHAR);
          act
            ("$N whispers to you, 'I'm all yours, baby. Take me anywhere, I'll follow.",
             FALSE, pl, 0, ch, TO_CHAR);
          act
            ("$N walks up to $n, engages in a brief exchange, and kiss eagerly.",
             TRUE, pl, 0, ch, TO_NOTVICT);
          setup_pet(ch, pl, 10, 0);
          if (ch->following)
            stop_follower(ch);
          add_follower(ch, pl);
        }
      }
      return (TRUE);
    }
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 7))
      {
      case 2:
        act("$n flexes showing her bulging muscles.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        mobsay(ch, "Lets do things different, How about I benchpress you.");
        break;
      case 4:
        act("$n does ten push-ups really quick thinking it's attractive.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 5:
        mobsay(ch,
               "Promise me that you're not a two minute man like Maegor.");
        break;
      case 6:
        mobsay(ch, "You're kinda flimsy, can you handle me?");
        act("$n smirks arrogantly.", TRUE, ch, 0, 0, TO_ROOM);
      case 7:
        act("$n bends down and stretches to touch her toes.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  /*
     These things will let the mob act based on what people do it it. 
   */
  if (cmd != 0)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);
    if (*Gbuf1)
      c_obj = get_char_room(Gbuf1, ch->in_room);
    if (IS_AGG_CMD(cmd))
    {
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      act("$N screams at you, 'Oh not good enough for you huh!'", FALSE, pl,
          0, ch, TO_CHAR);
      act("$N screams, 'Not good enough huh?!'", TRUE, pl, 0, ch, TO_ROOM);
      if (ch->following)
        stop_follower(ch);
      return FALSE;
    }
    switch (cmd)
    {
    case CMD_KISS:
    case CMD_FRENCH:
    case CMD_UNDRESS:
    case CMD_CARESS:
    case CMD_SEDUCE:
    case CMD_FONDLE:
    case CMD_GROPE:
    case CMD_DREAM:
    case CMD_DROOL:
    case CMD_OGLE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N whispers to you, '5 gold for the time of your life baby.'",
          FALSE, pl, 0, ch, TO_CHAR);
      act("$N whispers something to $n.", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_SMILE:
    case CMD_WINK:
      if (!IS_AWAKE(ch))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$n smiles evilly, rubbing herself.", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
      break;
    case CMD_PUKE:
    case CMD_INSULT:
    case CMD_SLAP:
    case CMD_SPIT:
    case CMD_CURSE:
    case CMD_PUNCH:
    case CMD_CHOKE:
    case CMD_STRANGLE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N tells you, 'Listen punk! I'll bodyslam you!'", FALSE, pl, 0, ch,
          TO_CHAR);
      act("$N tells $n that she knows how to bodyslam.", TRUE, pl, 0, ch,
          TO_ROOM);
      return TRUE;
      break;
    case CMD_HUG:
    case CMD_SNUGGLE:
    case CMD_CUDDLE:
    case CMD_NUZZLE:
    case CMD_LICK:
    case CMD_LOVE:
    case CMD_NIBBLE:
    case CMD_SQUEEZE:
    case CMD_STARE:
    case CMD_MASSAGE:
    case CMD_SPANK:
    case CMD_WORSHIP:
    case CMD_PINCH:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N whispers to you, 'OoOOooOOOoo' excitedly.", FALSE, pl, 0, ch,
          TO_CHAR);
      act("$N whispers something to $n excitedly", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_WAKE:
      if (c_obj != ch)
        return FALSE;
      if (GET_STAT(ch) != STAT_SLEEPING)
        act("$N is not sleeping.", FALSE, pl, 0, ch, TO_CHAR);
      else
      {
        act
          ("Your gentle nudging awakens $N, smiles and says 'You ready for more?'",
           FALSE, pl, 0, ch, TO_CHAR);
        act("$n nudges $N awake.", FALSE, pl, 0, ch, TO_ROOM);
        SET_POS(ch, POS_STANDING + STAT_NORMAL);
      }
      return TRUE;
      break;
    case CMD_POKE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, arg, CMD_POKE);
      act("$N says 'HooHoo.'", FALSE, pl, 0, ch, TO_CHAR);
      act("$N says 'HooHoo.'", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    default:
      return FALSE;
      break;
    }
  }
  return (FALSE);
}

int sleezy_prostitute(P_char ch, P_char pl, int cmd, char *arg)
{
  struct affected_type af;
  int      gold;
  P_char   k;
  P_char   c_obj = 0;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl && (pl != ch))
  {
    if (cmd == CMD_GIVE)
    {                           /*
                                   give her the money 
                                 */
      gold = GET_MONEY(ch);
      do_give(pl, arg, 0);
      if ((gold = (GET_MONEY(ch) - gold)))
      {
        if (gold < 500)
          mobsay(ch,
                 "You STILL need to pay me 500 copper coins, baby cakes.");
        else
        {
          strcpy(Gbuf1, GET_NAME(pl));
          do_action(ch, Gbuf1, CMD_SMILE);      /*
                                                   smile 
                                                 */
          k = world[ch->in_room].people;
          while ((k != ch) && (k != pl) && k)
            k = k->next_in_room;
          if (!k)
          {


            logit(LOG_DEBUG, "Prostitute error 1!");
            return (TRUE);
          }
          act("$N walks up to you and gives you a passionate french kiss.",
              FALSE, pl, 0, ch, TO_CHAR);
          act
            ("$N whispers to you, 'I'm all yours, baby. Take me anywhere, I'll follow.",
             FALSE, pl, 0, ch, TO_CHAR);
          act
            ("$N walks up to $n, engages in a brief exchange, and kiss eagerly.",
             TRUE, pl, 0, ch, TO_NOTVICT);
          setup_pet(ch, pl, 10, 0);

          if (ch->following)
            stop_follower(ch);
          add_follower(ch, pl);

        }
      }
      return (TRUE);
    }
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 7))
      {
      case 2:
        act("$n pulls up her stocking.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        mobsay(ch, "OOoOooh you look kinda sexy there.");
        break;
      case 4:
        act("$n slowly caresses herself in front of you.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 5:
        act("$n bends down and strokes her back.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        mobsay(ch, "Gimme a kiss and see what I do to you.");
        act("$n giggles.", TRUE, ch, 0, 0, TO_ROOM);
      case 7:
        act("$n touches you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  /*
     These things will let the mob act based on what people do it it. 
   */
  if (cmd != 0)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);
    if (*Gbuf1)
      c_obj = get_char_room(Gbuf1, ch->in_room);
    if (IS_AGG_CMD(cmd))
    {
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      act("$N screams at you, 'Oh not good enough for you huh!'", FALSE, pl,
          0, ch, TO_CHAR);
      act("$N screams, 'Not good enough huh?!'", TRUE, pl, 0, ch, TO_ROOM);
      if (ch->following)
        stop_follower(ch);
      return FALSE;
    }
    switch (cmd)
    {
    case CMD_KISS:
    case CMD_FRENCH:
    case CMD_UNDRESS:
    case CMD_CARESS:
    case CMD_SEDUCE:
    case CMD_FONDLE:
    case CMD_GROPE:
    case CMD_DREAM:
    case CMD_DROOL:
    case CMD_OGLE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N whispers to you, '5 gold for the time of your life baby.'",
          FALSE, pl, 0, ch, TO_CHAR);
      act("$N whispers something to $n.", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_SMILE:
    case CMD_WINK:
      if (!IS_AWAKE(ch))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$n smiles evilly, rubbing herself.", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
      break;
    case CMD_PUKE:
    case CMD_INSULT:
    case CMD_SLAP:
    case CMD_SPIT:
    case CMD_CURSE:
    case CMD_PUNCH:
    case CMD_CHOKE:
    case CMD_STRANGLE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N tells you, 'You faggot you're in the wrong place.'", FALSE, pl,
          0, ch, TO_CHAR);
      act("$N tells $n to find a place where faggots go.", TRUE, pl, 0, ch,
          TO_ROOM);
      return TRUE;
      break;
    case CMD_HUG:
    case CMD_SNUGGLE:
    case CMD_CUDDLE:
    case CMD_NUZZLE:
    case CMD_LICK:
    case CMD_LOVE:
    case CMD_NIBBLE:
    case CMD_SQUEEZE:
    case CMD_STARE:
    case CMD_MASSAGE:
    case CMD_SPANK:
    case CMD_WORSHIP:
    case CMD_PINCH:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N whispers to you, 'OoOOooOOOoo' excitedly.", FALSE, pl, 0, ch,
          TO_CHAR);
      act("$N whispers something to $n excitedly", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_WAKE:
      if (c_obj != ch)
        return FALSE;
      if (GET_STAT(ch) != STAT_SLEEPING)
        act("$N is not sleeping.", FALSE, pl, 0, ch, TO_CHAR);
      else
      {
        act
          ("Your gentle nudging awakens $N, smiles and says 'You ready for more?'",
           FALSE, pl, 0, ch, TO_CHAR);
        act("$n nudges $N awake.", FALSE, pl, 0, ch, TO_ROOM);
        SET_POS(ch, POS_STANDING + STAT_NORMAL);
      }
      return TRUE;
      break;
    case CMD_POKE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, arg, CMD_POKE);
      act("$N says 'HooHoo.'", FALSE, pl, 0, ch, TO_CHAR);
      act("$N says 'HooHoo.'", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    default:
      return FALSE;
      break;
    }
  }
  return (FALSE);
}

int Vella_slut(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 3))
      {
      case 2:
        act("$n looks at you wondering why you're in here.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        mobsay(ch, "I'm not for sale get out!");
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int Padh_bouncer(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 3))
      {
      case 2:
        act("$n sizes you up and down.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        mobsay(ch,
               "Don't even think about it! Try your luck on those women!");
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int Vem_rouge(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 4))
      {
      case 2:
        act("$n twirls a dagger and watches you closely.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        mobsay(ch, "I get some all the time!");
        act("$n grins evily.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        mobsay(ch, "When you have money they give you honey.");
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int tired_young_man(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch))
    return FALSE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 3))
      {
      case 2:
        mobsay(ch, "Oh boy don't mess with the well built prostitute.");
        break;
      case 3:
        mobsay(ch, "You have to try the sex crazed one.");
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}


int shimmering_longsword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   vict;


  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !(!OBJ_WORN_POS(obj, WIELD) && !OBJ_WORN_POS(obj, SECONDARY_WEAPON)) )
  {
    return FALSE;
  }
  vict = (P_char) arg;
  if( !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  if( OBJ_WORN_BY(obj, ch) )
  {
    if( !number(0, 29) )
    {
      act("As $p slashes $N's armor, a &+Bbright blue force of "
        "mystic power streaks out the blade&n, bombarding $N body!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("As $n's $q slashes your armor, a &+Bbright blue force "
        "of mystic power streaks out the blade&n, bombarding your body!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act("As $n's $q slashes $N's armor, a &+Bbright blue force "
        "of mystic power streaks out the blade&n, bombarding $N's body!", FALSE, obj->loc.wearing, obj, vict, TO_ROOM);
      spell_lightning_bolt(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
  }
  return FALSE;
}
