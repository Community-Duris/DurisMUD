/*
   ***************************************************************************
   *  File: specs.verzanan.c                                  Part of Duris *
   *  Usage: specials (mostly mobiles) for the city of Verzanan               *
   *  Copyright  1994, 1995 - Kristopher Kortright and Duris Systems Ltd.    *
   *************************************************************************** 
 */

/* Written by Kristopher Kortright (Miax) for use with Duris.       */
/* COPYRIGHT NOTICE: This code for Verzanan is Copyright(c) 1994, by    */
/* Kristopher Scott Kortright. This zone is NOT freeware or shareware,    */
/* and may NOT be used in any form without expressly written permission from */
/* the author. */

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


/*
   this routine is attached to the WD clock tower, if you wish to use this
   elsewhere: ct1, ct2, clock_zones need to change, and the event (db.c) needs
   to be added in a special manner.  -JAB
 */
int clock_tower(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      temp, zon;
  int      ct1 = 4, ct2 = 6;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  const int clock_zones[] = { 2700, 3001, 3200, 5500, 3500, 5300 };

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( !current_event || (current_event->type != EVENT_OBJ_SPECIAL) )
  {
    logit(LOG_EXIT, "Call to clock_tower with messed current_event");
    raise(SIGSEGV);
  }
  // Clock is off (event is rather), so reschedule for +1 hour
  if( time_info.hour % 3 )
  {
    current_event->timer = 1;
    return TRUE;
  }
  sprintf(Gbuf1, "%d%s.\r\n", (time_info.hour % 12) ? (time_info.hour % 12) : 12,
    (time_info.hour == 12) ? " noon" : (time_info.hour == 0) ? " midnight" :
    (time_info.hour > 11) ? "pm" : "am" );

  for( temp = 0; temp < ct1; temp++ )
  {
    zon = real_room(clock_zones[temp]);
    if( zon == NOWHERE )
    {
      continue;
    }
    zon = world[zon].zone;

    sprintf(Gbuf2, "The clock tower chimes the hour of %s", Gbuf1);
    send_to_zone_outdoor(zon, Gbuf2);
    sprintf(Gbuf2, "From outside, the faint chimes of the clock tower sound %s", Gbuf1);
    send_to_zone_indoor(zon, Gbuf2);
  }

  for (temp = ct1; temp < ct2; temp++)
  {
    zon = real_room(clock_zones[temp]);
    if( zon == NOWHERE )
    {
      continue;
    }
    zon = world[zon].zone;

    sprintf(Gbuf2, "From the city, the clock tower chimes %s", Gbuf1);
    send_to_zone_outdoor(zon, Gbuf2);
    sprintf(Gbuf2, "From outside in the direction of the city, the clock tower chimes %s", Gbuf1);
    send_to_zone_indoor(zon, Gbuf2);
  }

  current_event->timer = 3;
  return TRUE;
}

/* Proc for Lord Piergeiron, wandering the town and locking gates, etc. */
int piergeiron(P_char ch, P_char pl, int cmd, char *arg)
{
  static char piergeiron_open_path[] =
    "W0000l4mnop52333eO33221q4Egh53001O1111222S.";

  static char piergeiron_close_path[] =
    "W0001101ai000000000bj1111111AC333333300c00000BC22222k22333333FC111111d222222222d3233222S.";

  P_char   c_obj = 0;
  static char *piergeiron_path;
  static int piergeiron_index;
  static bool move = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH], buf[40];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!move)
  {
    if (ch->in_room != real_room(3306))
    {
      if ((GET_STAT(ch) > STAT_INCAP) && !IS_FIGHTING(ch))
        if (GET_STAT(ch) == STAT_SLEEPING)
          act("$n wakes up, looking extremely annoyed.", TRUE, ch, 0, 0,
              TO_ROOM);
      SET_POS(ch, POS_STANDING + STAT_NORMAL);
      act("$n says, 'Damn, lost again, and almost time for work too!", TRUE,
          ch, 0, 0, TO_ROOM);
      act("$n pulls a small device from a pocket and presses a button on it.",
          TRUE, ch, 0, 0, TO_ROOM);
      act("$n slowly fades out of existence..", TRUE, ch, 0, 0, TO_ROOM);
      char_from_room(ch);
      char_to_room(ch, real_room(3306), 0);
      act("$n slowly fades into existence, looking extremely annoyed.", TRUE,
          ch, 0, 0, TO_ROOM);
      act("$n sits behind his desk, and closes his eyes for a quick nap.",
          TRUE, ch, 0, 0, TO_ROOM);
      SET_POS(ch, POS_SITTING + STAT_SLEEPING);
      move = FALSE;
    }
    else if (time_info.hour == 8)
    {
      move = TRUE;
      ch->only.npc->default_pos = STAT_INCAP + POS_STANDING;    /*
                                                                   cheating 
                                                                 */
      piergeiron_path = piergeiron_open_path;
      piergeiron_index = 0;
    }
    else if (time_info.hour == 18)
    {
      move = TRUE;
      ch->only.npc->default_pos = STAT_INCAP + POS_STANDING;    /*
                                                                   cheating 
                                                                 */
      piergeiron_path = piergeiron_close_path;
      piergeiron_index = 0;
    }
    if ((move == FALSE) && (GET_STAT(ch) == STAT_SLEEPING) &&
        (ch->in_room == real_room(3306)))
    {
      act("$n snores loudly enough to shake the walls.", TRUE, ch, 0, 0,
          TO_ROOM);
      move = FALSE;
    }
  }                             /* end (!move) if loop */
  /* Pc-Mobile interaction engine */
  if (!move || (GET_STAT(ch) < STAT_SLEEPING) || IS_FIGHTING(ch))
    return FALSE;

  if ((cmd != 0) && pl)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);
    if (*Gbuf1)
      c_obj = get_char_room(Gbuf1, ch->in_room);
    switch (cmd)
    {
    case CMD_KISS:
    case CMD_SMILE:
    case CMD_DANCE:
    case CMD_HUG:
    case CMD_SNUGGLE:
    case CMD_CUDDLE:
    case CMD_NUZZLE:
    case CMD_CURTSEY:
    case CMD_FONDLE:
    case CMD_GROPE:
    case CMD_LICK:
    case CMD_LOVE:
    case CMD_MOAN:
    case CMD_NIBBLE:
    case CMD_SQUEEZE:
    case CMD_WINK:
    case CMD_FRENCH:
    case CMD_MASSAGE:
    case CMD_TICKLE:
    case CMD_DREAM:
    case CMD_GROVEL:
    case CMD_WORSHIP:
    case CMD_RUB:
    case CMD_DROOL:
    case CMD_OGLE:
    case CMD_PANT:
    case CMD_PINCH:
    case CMD_SEDUCE:
    case CMD_TEASE:
    case CMD_UNDRESS:
    case CMD_CARESS:
    case CMD_BATHE:
    case CMD_EMBRACE:
    case CMD_ENVY:
    case CMD_FLIRT:
    case CMD_MELT:
    case CMD_FLUTTER:
    case CMD_HICKEY:
    case CMD_ROSE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)) || (IS_FIGHTING(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      if (GET_SEX(pl) == SEX_FEMALE)
      {
        act("$n whispers to you, 'You're a fine lass if ever I did see one!'",
            FALSE, pl, 0, ch, TO_CHAR);
        act("$N winks suggestively at you.", FALSE, pl, 0, ch, TO_CHAR);
        do_action(ch, 0, CMD_HAND);
        act("$N whispers something to $n.", TRUE, pl, 0, ch, TO_ROOM);
        act("$N winks suggestively at $n.", TRUE, pl, 0, ch, TO_ROOM);
      }
      else
      {
        act
          ("$N tells you, 'Now, now. None of that mushy stuff with me, pal.'",
           FALSE, pl, 0, ch, TO_CHAR);
        act("$N tells $n, 'None of the mushy stuff with me, pal.'", TRUE, pl,
            0, ch, TO_ROOM);
      }
      return TRUE;
      break;
    case CMD_LAUGH:
    case CMD_PUKE:
    case CMD_GROWL:
    case CMD_INSULT:
    case CMD_POKE:
    case CMD_ACCUSE:
    case CMD_FART:
    case CMD_GLARE:
    case CMD_SLAP:
    case CMD_SNEEZE:
    case CMD_SPIT:
    case CMD_CURSE:
    case CMD_FUME:
    case CMD_PUNCH:
    case CMD_SNARL:
    case CMD_STEAM:
    case CMD_TAUNT:
    case CMD_SILENCE:
    case CMD_BONK:
    case CMD_CHOKE:
    case CMD_MOON:
    case CMD_SCOLD:
    case CMD_SNEER:
    case CMD_STRANGLE:
    case CMD_FLAME:
    case CMD_BANG:
    case CMD_WHAP:
    case CMD_GAG:
    case CMD_DROPKICK:
    case CMD_NOOGIE:
    case CMD_HISS:
    case CMD_BITE:
    case CMD_GOOSE:
    case CMD_IMPALE:
    case CMD_SWAT:
    case CMD_TRIP:
      if ((c_obj != ch) || (!IS_AWAKE(ch)) || (IS_FIGHTING(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act
        ("$N snarls at you, 'Watch it, bub, or I just might have to teach you a lesson!'",
         FALSE, pl, 0, ch, TO_CHAR);
      act
        ("$N snarls at $n, 'Watch it, bub, or I just might have to teach you a lesson!'",
         TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_SCREAM:
    case CMD_BURP:
    case CMD_SING:
    case CMD_WIGGLE:
    case CMD_BEG:
    case CMD_HOP:
    case CMD_SPANK:
    case CMD_TACKLE:
    case CMD_YODEL:
    case CMD_POSE:
    case CMD_CENSOR:
    case CMD_SCARE:
    case CMD_SHUSH:
    case CMD_SLOBBER:
    case CMD_BARK:
    case CMD_ROLL:
    case CMD_FOOL:
    case CMD_BIRD:
      if ((c_obj != ch) || (!IS_AWAKE(ch)) || (IS_FIGHTING(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act
        ("$N grumbles at you, 'Every damn place you go, someone has to be a clown.'",
         FALSE, pl, 0, ch, TO_CHAR);
      act
        ("$N grumbles at $n, 'Every damn place you go, someone has to be a clown.'",
         TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_GRIN:
    case CMD_HICCUP:
    case CMD_RUFFLE:
    case CMD_SNORE:
    case CMD_STRUT:
    case CMD_SNOWBALL:
    case CMD_COMB:
    case CMD_FLEX:
    case CMD_LEAN:
    case CMD_SPIN:
    case CMD_STRETCH:
    case CMD_TIP:
    case CMD_TWEAK:
    case CMD_TWIRL:
    case CMD_ARCH:
    case CMD_AMAZE:
    case CMD_ACK:
    case CMD_CHEER:
    case CMD_PILLOW:
    case CMD_MOSH:
    case CMD_HI5:
      if ((c_obj != ch) || (!IS_AWAKE(ch)) || (IS_FIGHTING(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N giggles at you, finding that amusing.", FALSE, pl, 0, ch,
          TO_CHAR);
      act("$N giggles at $n, finding that amusing.", TRUE, pl, 0, ch,
          TO_ROOM);
      return TRUE;
      break;
    default:
      return FALSE;
      break;
    }
  }
  if (cmd || !move || !piergeiron_path || IS_FIGHTING(ch))
    return FALSE;

  /*
     ok, something of a hack here, standard 1 move per 9-11 seconds is NOT
     sufficient to move Lord Spam on his rounds.  SO.  If NEXT step on his path 
     is a move ('0' thru '5'), and so is the current one, we nuke the current
     event (MOB_SPECIAL) now, and schedule the next one in 2 seconds. If it's
     not, we still speed things up a bit (8 seconds). Net affect, when he's
     moving, he moves once per 2 seconds (5x speed), and when he's not, he's
     still a bit faster (nothing gross though).  JAB 

     -rewrite it if you want mob back
   */

  switch (piergeiron_path[piergeiron_index])
  {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    do_move(ch, 0, exitnumb_to_cmd(piergeiron_path[piergeiron_index] - '0'));
    break;

  case 'W':
    SET_POS(ch, POS_STANDING + STAT_NORMAL);
    act("$n awakens and stretches his arms wide.", FALSE, ch, 0, 0, TO_ROOM);
    act("$n says, 'Time to take care of business!'", FALSE, ch, 0, 0,
        TO_ROOM);
    break;

  case 'S':
    ch->only.npc->default_pos = STAT_SLEEPING + POS_SITTING;
    SET_POS(ch, POS_SITTING + STAT_SLEEPING);
    act("$n lies down and quickly falls asleep behind his desk.",
        FALSE, ch, 0, 0, TO_ROOM);
    move = FALSE;
    break;

  case 'a':
    act("$n steps in front of the watchman and salutes.", FALSE, ch, 0, 0,
        TO_ROOM);
    act("$n says, 'Report, watchman!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n steps in front of the guard and salutes.", FALSE, ch, 0, 0,
        TO_ROOM);
    act("$n says, 'Report, guard. What's new this day?'", FALSE, ch, 0, 0,
        TO_ROOM);
    break;

  case 'c':
    act("$n says, 'Greetings, friends!  Another fine day in Verzanan.'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n smiles, looking very proud.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Keep up the good work men!'", FALSE, ch, 0, 0, TO_ROOM);
    act("The guard stands proudly at attention.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act
      ("$n says, 'Looks like another busy day in the port!  Keep the people in order, men.'",
       FALSE, ch, 0, 0, TO_ROOM);
    act("The guards salute, and nod in respect.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n smiles warmly at Kalara.", FALSE, ch, 0, 0, TO_ROOM);
    act("$n says, 'My greetings, m'lady. How's business?'", FALSE, ch, 0, 0,
        TO_ROOM);
    break;

  case 'g':
    act("Kalara says, 'Same as always, old man, and you come by anyway.'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("Kalara chuckles politely.", FALSE, ch, 0, 0, TO_ROOM);
    act("Kalara pokes Lord Piergeiron in the ribs.", FALSE, ch, 0, 0,
        TO_ROOM);
    break;

  case 'h':
    act("$n says, 'Yeah, yeah. Hey, it's my job, y'know.'", FALSE, ch, 0, 0,
        TO_ROOM);
    act("$n says, 'I'm off to find Khelben. Take care, Kalara.'", FALSE, ch,
        0, 0, TO_ROOM);
    act("$n bows deeply.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'i':
    act("The watchman says, 'Nothing happening here at the bazaar, sir.'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n says, 'Keep up the good work.'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'j':
    act("The watchman salutes.", FALSE, ch, 0, 0, TO_ROOM);
    act("The watchman says, 'Nothing unusual sir, same as always.'", FALSE,
        ch, 0, 0, TO_ROOM);
    break;

  case 'k':
    act
      ("$n says, 'Anyone here seen Khelben?  Damn slippery mages are impossible to find..'",
       FALSE, ch, 0, 0, TO_ROOM);
    act("$n sighs loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'l':
    act("$n says with good cheer, 'Have a good one, boys!'", FALSE, ch, 0, 0,
        TO_ROOM);
    act("The men cheer and raise their mugs.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'm':
    act("The waiter says, 'Your usual sir?'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n says, 'You got it, old man. Need it to wake up.'", FALSE, ch, 0,
        0, TO_ROOM);
    break;

  case 'n':
    act("The waiter pours a shot of whiskey. ", FALSE, ch, 0, 0, TO_ROOM);
    act("The waiter gives the shot to the tired Lord of Verzanan.", FALSE, ch,
        0, 0, TO_ROOM);
    break;

  case 'o':
    act("$n swallows the shot in a single gulp, and hands back the glass.",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n says, 'Ahhhh, that hit the spot.'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'p':
    act("$n says, 'Well, back to work, boys. See ya later on.'", FALSE, ch, 0,
        0, TO_ROOM);
    act("$n waves happily.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'q':
    act("$n says, 'Heya, sweet thing.'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n winks at the receptionist.", FALSE, ch, 0, 0, TO_ROOM);
    act("The receptionist smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'A':
    act("$n says, 'The east gate of Verzanan is now closed!'", FALSE, ch, 0,
        0, TO_ROOM);
    act("$n says, 'You may reopen the gates at sunrise, men.'", FALSE, ch, 0,
        0, TO_ROOM);
    act("The guards nod in acknowledgment.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'B':
    act("$n says, 'The north gate of Verzanan is now closed!'", FALSE, ch, 0,
        0, TO_ROOM);
    act("$n says, 'You may reopen the gates at sunrise, men.'", FALSE, ch, 0,
        0, TO_ROOM);
    act("The guards nod in acknowledgment.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'F':
    act("$n says, 'The west gate of Verzanan is now closed!'", FALSE, ch, 0,
        0, TO_ROOM);
    act("$n says, 'You may reopen the gates at sunrise, men.'", FALSE, ch, 0,
        0, TO_ROOM);
    act("The guards nod in acknowledgment.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    strcpy(buf, "gate");
    do_unlock(ch, buf, 0);
    do_open(ch, buf, 0);
    break;
  case 'C':
    strcpy(buf, "gate");
    do_close(ch, buf, 0);
    do_lock(ch, buf, 0);
    break;

  case '.':
    ch->only.npc->default_pos = STAT_SLEEPING + POS_SITTING;
    move = FALSE;
    break;

  }
  piergeiron_index++;

  /* Random action routine */
  if (!pl)
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (number(1, 20))
      {
      case 1:
        mobsay(ch, "God, I love this city.");
        mobsay(ch, "No where else in the world can you find such splendor!");
        break;
      case 2:
        mobsay(ch, "Ooo, smells like someone is cooking up a fine meal.");
        act("$n peers around with a hungry look on his face.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        act("$n smiles at you with a warm, fatherly look.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        mobsay(ch, "You ever hear of Tripod? That bastard assassin?");
        mobsay(ch, "Scum hides like a rat and corrodes my city from within.");
        act("$n frowns.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 5:
        act("$n seems distracted for a moment, as if pondering something.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        act("$n adjusts his uniform to look more authoritative.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 7:
        mobsay(ch,
               "I'll bet you anything Fafhrd has spies watching my every move.");
        mobsay(ch, "And hides when I'm around so I won't find him.");
        act("$n peers around.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 8:
        act("$n trips over a small stone, but recovers gracefully.", TRUE, ch,
            0, 0, TO_ROOM);
        act("$n blushes slightly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 9:
        act("$n looks you over for a moment.", TRUE, ch, 0, 0, TO_ROOM);
        mobsay(ch, "Ever consider a career in the military?");
        mobsay(ch, "We could use someone like you.");
        break;
      case 10:
        mobsay(ch, "I wonder if Kalara would join me for dinner..");
        act("$n ponders the thought with an evil twinkle in his eye.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 11:
        act("$n considers you casually for a moment.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 12:
        act("$n smirks arrogantly, obviously proud of his station.", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      case 13:
        mobsay(ch, "Hell of a day isn't it?");
        break;
      case 14:
        mobsay(ch, "Greetings, citizen!");
        act("$n bows deeply to you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 15:
        act("$n scans the area for something.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 16:
        mobsay(ch,
               "Well, well, well. You plan on staying out of trouble today?");
        break;
      case 17:
        mobsay(ch, "Hey look alive there!");
        act("$n rubs his chin while looking you over.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 18:
        act("$n wipes the sweat off his brow.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 19:
        mobsay(ch, "Need any help with anything citizen?");
        act("$n smiles warmly at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 20:
        mobsay(ch, "Isn't it a beautiful day today?");
        act("$n stares in the sky, covering his eyes.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
      return TRUE;
    }
  }
  if (move)
    return TRUE;

  return FALSE;
}

int piergeiron_guard(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Gbuf1[MAX_STRING_LENGTH], buf[MAX_INPUT_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!pl)
  {
    return (0);
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n fights with amazing skill and grace.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "You will pay for this outlaw!");
      break;
    case 3:
      strcpy(buf,
             "An outlaw has attacked me! Come help lay this villain low, brethren!");
      do_yell(ch, buf, 0);
      break;
    case 4:
      /*
         Attempt to disarm the opponent 
       */
      if (number(0, 3) == 0 && pl != ch && !IS_TRUSTED(pl))
      {
        act("$n performs a martial arts maneuver with blinding speed!", TRUE,
            ch, 0, 0, TO_ROOM);
        strcpy(Gbuf1, pl->player.name);
        do_mobdisarm(ch, Gbuf1, 0);
      }
      break;
    default:
      break;
    }
  }
  if ((MIN_POS(ch, POS_STANDING + STAT_NORMAL)) && !number(0, 4))
  {
    switch (dice(2, 5))
    {
    case 2:
      act
        ("$n looks around cautiously, watching for any possible threat to the lord.",
         TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      act("$n looks at you for a moment.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act("$n keeps his hand on his sword hilt while near you.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    case 5:
      act("$n scans the area.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int wanderer(P_char ch, P_char pl, int cmd, char *arg)
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
        act("$n examines the animal tracks on the ground.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        mobsay(ch, "God, I love the outdoors!");
        break;
      case 4:
        act("$n looks at you with a curious expression.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 5:
        act("$n gazes off onto the horizon, looking for something.", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int dog_one(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    i, temp, next_obj;
  P_event  e1;

  /* check for periodic event calls */
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
        act("$n whines as if looking for a handout.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        act("$n hears something in the distance and barks.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n wanders around, sniffing the ground.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }

  if (cmd || !IS_AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content)
  {
    if ((GET_ITEM_TYPE(i) == ITEM_CORPSE) || (GET_ITEM_TYPE(i) == ITEM_FOOD))
    {
      if (!(i))
      {
        logit(LOG_EXIT, "assert: dog_one");
        raise(SIGSEGV);
      }
      if (GET_ITEM_TYPE(i) == ITEM_CORPSE)
      {
        struct obj_affect *af = get_obj_affect(i, TAG_OBJ_DECAY);
        if (af && (obj_affect_time(i, af) > 2550))
          return TRUE;
      }
      for (temp = i->contains; temp; temp = next_obj)
      {
        next_obj = temp->next_content;
        obj_from_obj(temp);
        obj_to_room(temp, ch->in_room);
      }
      /*
         added logging to prevent player bitching -- DTS 2/1/95 
       */
      if (IS_SET(i->value[1], PC_CORPSE))
      {
        logit(LOG_CORPSE, "%s devoured in room %d.", i->short_description,
              world[i->loc.room].number);
      }
      act("$n savagely devours the $q.", FALSE, ch, i, 0, TO_ROOM);
      extract_obj(i, TRUE); // Empty corpse, but 'in game.'
      return (TRUE);
    }
  }
  return (FALSE);
}

int dog_two(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    i, temp, next_obj;
  P_event  e1;

  /* check for periodic event calls */
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
        act("$n growls dangerously.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n howls.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n looks your way and snarls.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 5:
        act("$n whirls, listening intently for some unheard sound.", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }

  if (cmd || !IS_AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content)
  {
    if ((GET_ITEM_TYPE(i) == ITEM_CORPSE) || (GET_ITEM_TYPE(i) == ITEM_FOOD))
    {
      if (!(i))
      {
        logit(LOG_EXIT, "assert: dog_two");
        raise(SIGSEGV);
      }
      if (GET_ITEM_TYPE(i) == ITEM_CORPSE)
      {
        struct obj_affect *af = get_obj_affect(i, TAG_OBJ_DECAY);
        if (af && (obj_affect_time(i, af) > 2550))
          return TRUE;
      }
      for (temp = i->contains; temp; temp = next_obj)
      {
        next_obj = temp->next_content;
        obj_from_obj(temp);
        obj_to_room(temp, ch->in_room);
      }
      /*
         added logging to prevent player bitching -- DTS 2/1/95 
       */
      if (IS_SET(i->value[1], PC_CORPSE))
      {
        logit(LOG_CORPSE, "%s devoured in room %d.", i->short_description,
              world[i->loc.room].number);
      }
      act("$n savagely devours the $q.", FALSE, ch, i, 0, TO_ROOM);
      extract_obj(i, TRUE); // Empty corpse, but 'in game.'
      return (TRUE);
    }
  }
  return (FALSE);
}

int drunk_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n stumbles and nearly falls, lost in his drunken stupor.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Heeeeyyyy, matie, got any whiskey?");
        break;
      case 3:
        act("$n mumbles something incoherent.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n turns green and nearly hurls, but amazingly recovers.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int drunk_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act
          ("$n begins singing loudly, though his awful tone makes you cringe.",
           TRUE, ch, 0, 0, TO_ROOM);
        act("Dogs can be heard howling in the distance.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch,
               "OOoohhh! Loookie what weee have  here, a worthless ball offf horse manuure..");
        break;
      case 3:
        act
          ("$n points at you and laughs uncontrollably for several minutes..",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act
          ("$n flips you the bird and mumbles something incoherent under his breath.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int drunk_three(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n shouts annoyingly, 'Where is that damn bartender!'", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Hey, pssssst, you. Yeah, you.");
        mobsay(ch, "Know of any good places to gamble around here?");
        break;
      case 3:
        act
          ("$n loses his balance and falls to the ground, cursing all the while.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act
          ("$n stares off into space, seemingly lost in some mindless thought.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int homeless_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n sniffs sadly, looking depressed.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Alms for the poor?");
        break;
      case 3:
        mobsay(ch, "Could you spare a few coins?");
        break;
      case 4:
        act("$n looks at you pleadingly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int homeless_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n holds out his hands, begging for food.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch,
               "Could ya spare a few coins? Just a few? I gots nuttin' ta eat tonight..");
        act("$n whimpers quietly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n is overcome with a fit of coughing. He doesn't look well.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n looks utterly miserable.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int cat_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

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
      case 5:
        act("$n approaches and bumps your leg, looking for attention.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n scratches at an itch.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n dives at something on the ground, playing.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n looks at you and mews, purring for attention.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int merchant_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n smirks arrogantly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch,
               "You wouldn't happen to know where the bazaar is, would you?");
        break;
      case 3:
        act("$n looks condescendingly at you, as if you're less than scum.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act
          ("$n looks you up and down, probably sizing up whether or not you're worth the effort.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int merchant_two(P_char ch, P_char pl, int cmd, char *arg)
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
    if ((MIN_POS(ch, POS_STANDING + STAT_NORMAL)) &&
        (ch->in_room == real_room(5400)))
    {
      switch (dice(2, 5))
      {
      case 5:
        act
          ("$n looks impatient, as if he's waited years for his ship to come in.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "GOD, where is that blasted ship!");
        break;
      case 3:
        act("$n stares out the door, scanning the harbor for his ship.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 4:
        mobsay(ch,
               "Receptionist! Get that damn ship here! I've been waiting forever!");
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int farmer_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n looks a bit timid in this huge city.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "I hate these big cities.");
        act("$n frowns.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n smiles warmly at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n looks a bit lost.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int baker_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n smiles warmly at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Do you have a reason to be here? Not that I mind.");
        break;
      case 3:
        act("$n looks around for something to clean.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n looks out the window at the glorious city.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int baker_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n runs around the room, playing wildly.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Hey, ma! Can we go outside and play?");
        break;
      case 3:
        act("$n crashes into a table while running around.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n looks around for something to play with.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int mage_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n studies his spellbook intently.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n attempts a spell.", TRUE, ch, 0, 0, TO_ROOM);
        mobsay(ch, "Tass Mohjak Tamarilon Deiliak!");
        act("$n frowns in frustration.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n stares blankly into space, contemplating something.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n looks at you curiously.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int cleric_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n sings a hymn in praise to the Gods. It is quite beautiful.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Go in peace, friend, all are welcome here.");
        act("$n smiles warmly at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n bows before you in reverence.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n performs a magical gesture of some kind.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int artillery_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n scans the horizon line intently.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n takes a long, deep breath as a cool breeze blows by.", TRUE,
            ch, 0, 0, TO_ROOM);
        mobsay(ch, "Hell of a day, isn't it..");
        break;
      case 3:
        act("$n checks the readiness of the catapult.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        mobsay(ch,
               "You should consider a career in the navy, strong as you are.");
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int warrior_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n shadow boxes, showing off his battle prowess.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Don't you wish you were as strong and mighty as I?");
        break;
      case 3:
        act("$n sizes you up, as if considering your battle capabilities.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n screws up a sword maneuver, blushing furiously.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int mercenary_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n eyes you suspiciously.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "If ya need a hired hand, I'm yer man.");
        break;
      case 3:
        act("$n keeps his hand on the hilt of his weapon while near you.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n stops suddenly as if having heard something odd.", TRUE, ch,
            0, 0, TO_ROOM);
        act("After a few moments, $n continues on his way.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int mercenary_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n casts you a wary glance.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch,
               "Get lost, kid, or I might decide to relieve you of your pathetic existence.");
        break;
      case 3:
        act("$n growls as you, resembling a not-so-trained Doberman.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n glares icily at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int mercenary_three(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n takes a long draught from his mug.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Hey, waiter, bring me another when you come around.");
        break;
      case 3:
        act("$n lets off a roaring belch that echoes around the room.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n gives you a casual glance.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int casino_one(P_char ch, P_char pl, int cmd, char *arg)
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
      switch (dice(2, 7))
      {
      case 2:
        act
          ("$n moves some gambling chips around so fast you almost can't follow his movements.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n shuffles the cards with the ease of a skilled pro.", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      case 4:
        mobsay(ch, "Dealer raises 20.");
        break;
      case 5:
        act("$n deals out a card to one of the gamblers.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 6:
        mobsay(ch, "Feel lucky tonight, boys?");
        break;
      case 7:
        act("$n makes a perfect poker face, looking as rigid as a board..",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int casino_two(P_char ch, P_char pl, int cmd, char *arg)
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
      switch (dice(2, 6))
      {
      case 2:
        mobsay(ch, "I'll raise 20.");
      case 3:
        act("$n studies his cards carefully.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        mobsay(ch, "C'mon, lady luck don't let me down!");
        break;
      case 5:
        act("$n makes an admirable poker face.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        act("$n nods his head.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int casino_three(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, evil;
  int      max_evil;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || IS_FIGHTING(ch))
    return (FALSE);

  if (ch->in_room != real_room(GET_HOME(ch)))
  {
    if (!real_room0(GET_HOME(ch)))
      return FALSE;

    /*
       Muahahahah 
     */
    act("$n looks around dazedily, and says 'Where the HELL am I?'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'Damn!  Luna'll KILL me, I gotta get back to work!'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n slowly fades from view.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(GET_HOME(ch)), -1);
    act("$n pops into view, with a sulphurous BANG!.",
        FALSE, ch, 0, 0, TO_ROOM);
    return FALSE;
  }
  max_evil = 1000;
  evil = 0;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch))
    {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
          (IS_NPC(tch) || IS_NPC(tch->specials.fighting)))
      {
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }
  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0))
  {
    if (IS_FIGHTING(evil))
      stop_fighting(evil);
    StopAllAttackers(evil);
    act("$n yells, 'HEY! No fighting in here, dead beat!'", FALSE, ch, 0,
        evil, TO_ROOM);
    act("The bouncer picks you and tosses you out of the bar!", FALSE, evil,
        0, ch, TO_CHAR);
    act("$n throws $N out into the street!", FALSE, ch, 0, evil, TO_NOTVICT);
    act("A loud thud can be heard.", FALSE, ch, 0, evil, TO_NOTVICT);
    char_from_room(evil);
    char_to_room(evil, real_room(3254), 0);
    act("$N is thrown from the tavern, landing in a heap on the ground!",
        FALSE, ch, 0, evil, TO_NOTVICT);
    act("The bouncer snarls at you, 'Next time I'll break your neck, punk!'",
        FALSE, evil, 0, ch, TO_CHAR);
    act("The bouncer snarls at $N, 'Next time I'll break your neck, punk!'",
        FALSE, ch, 0, evil, TO_NOTVICT);

    SET_POS(evil, POS_SITTING + GET_STAT(evil));
    return (TRUE);
  }
  return OutlawAggro(ch, "$n screams '%s! Fresh blood! Kill!'");
}

int casino_four(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (IS_FIGHTING(ch))
    {
      switch (dice(2, 5))
      {
      case 5:
        act("$n growls angrily, a look of rage on his face.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "You will pay for trespassing in my home!");
        break;
      case 3:
        act("$n says 'Die, worm!'", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n laughs at you as he viciously attacks.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 5))
      {
      case 5:
        act("$n growls angrily.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "You will pay for this insult!");
        break;
      case 3:
        act("$n looks around for something to fight with.'", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n glares at you..", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int guard_one(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, evil = NULL;
  int      max_evil = 1000;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 6))
    {
    case 6:
      act
        ("$n drinks deep from his bottle, and then lets out a roaring belch.",
         TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch,
             "How's about some entertainment, bartender! Where's that dancer.");
      act("$n grins evilly.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      act("$n sways slightly from being drunk.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act("$n says, 'Hey, bartender! Bring me another, dammit.'", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    case 5:
      act("$n laughs heartily, nearly falling off the barstool.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)))
    return (FALSE);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch))
    {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
          (IS_NPC(tch) || IS_NPC(tch->specials.fighting)))
      {
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }

  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0))
  {
    mobsay(ch,
           "Heeey!  You c-can't do that, I'ma..I'ma a guard of Wateeerdeepp!");
    act("$n jumps off the stool and joins in the fight with a drunken grin.",
        FALSE, ch, 0, 0, TO_ROOM);
    MobStartFight(ch, evil);
    return (TRUE);
  }
  return OutlawAggro(ch, "$n growls 'You're gonna pay, dead beat!'");
}

int guard_two(P_char ch, P_char pl, int cmd, char *arg)
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
    if (GET_STAT(ch) == STAT_SLEEPING)
    {
      switch (dice(2, 4))
      {
      case 4:
        act("$n snores loudly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n rolls over in bed, grunting.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n twitches in his sleep, probably having a dream.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int park_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 4:
        act("$n quacks loudly, running around in a circle.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        act("$n pecks at it's fur, cleaning itself.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        act("$n watches you warily.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int park_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 4:
        act("$n turns its head to look at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n runs along the ground looking for worms.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        act("$n chirps.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int park_three(P_char ch, P_char pl, int cmd, char *arg)
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
      case 4:
        act("$n lets out a little quack.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n looks about for its mother.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n runs about, playing.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int park_four(P_char ch, P_char pl, int cmd, char *arg)
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
      case 4:
        act("$n stands on its back legs to get a better look at you.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n eagerly devours a nut.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n dashes along the ground.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int park_five(P_char ch, P_char pl, int cmd, char *arg)
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
      case 4:
        act("$n's tail pops up as it considers you.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        act("$n digs in the grass for something.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n makes a little sound.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int park_six(P_char ch, P_char pl, int cmd, char *arg)
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
      case 4:
        act("$n runs along the ground, looking terrified.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        act("$n digs a small nut from the grass and begins eating it.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n wriggles its little tail while looking at you.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int youth_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n glares at you with contempt.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Piss off, ya big pile of horse dung.");
        break;
      case 3:
        act("$n looks at you with eyes both angry and hateful.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 4:
        act("$n spits at the ground in front of you.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int youth_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n shivers in fear.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Do-do you have anything I could eat?");
        break;
      case 3:
        act("$n looks at you pleadingly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n holds out a feeble hand.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int tailor_one(P_char ch, P_char pl, int cmd, char *arg)
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
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL) && !number(0, 2))
    {
      switch (dice(2, 5))
      {
      case 5:
        act("$n sorts through his many measuring tapes.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch,
               "Hello. You don't look like a cityguard, are you here for a fitting?");
        break;
      case 3:
        act("$n starts picking up small pieces of lint and thread.", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      case 4:
        act
          ("$n looks at you and says, 'You could stand to loose a few pounds.'",
           TRUE, ch, 0, 0, TO_ROOM);
        act("$n winks at you in amusement.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int shopper_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n browses through the goods for sale here.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Hi there!  Hope you're havin' more luck than me!");
        act("$n smiles at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act
          ("$n looks around frustrated, as if he can't find what he wants to buy.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n says, 'You can never find what you want in this damn bazaar!",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int shopper_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n smiles at you and says, 'Good day.'", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Hi there, having any luck today?");
        break;
      case 3:
        act("$n counts her money carefully.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n browses through the items for sale.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int rogue_one(P_char ch, P_char pl, int cmd, char *arg)
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
    if (IS_FIGHTING(ch))
    {
      switch (dice(2, 5))
      {
      case 5:
        act("$n says, 'You will DIE for intruding in my home!'", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "I will kill you ... slowly and painfully.");
        break;
      case 3:
        act("$n growls menacingly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n spits at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int prostitute_one(P_char ch, P_char pl, int cmd, char *arg)
{
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
          {                     /*
                                   Should not happen! 
                                 */
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
          if (ch->following)
            stop_follower(ch);
          add_follower(ch, pl);
          setup_pet(ch, pl, 45, 0);
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
        act("$n winks suggestively at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        mobsay(ch, "Hi there, honey. Wanna have some fun?");
        break;
      case 4:
        act("$n struts around in front of you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 5:
        act("$n looks around for an interesting prospect.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 6:
        mobsay(ch, "They say I'm the best around.");
        act("$n smirks arrogantly.", TRUE, ch, 0, 0, TO_ROOM);
      case 7:
        act("$n adjusts her stockings.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  /*
     Interactions between player and mobile 
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
      act
        ("$N screams in terror at you, 'Help, help! Someone get this creep off of me!'",
         FALSE, pl, 0, ch, TO_CHAR);
      act("$N screams, 'Help, help! Someone get this creep off of me!'", TRUE,
          pl, 0, ch, TO_ROOM);
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
      act("$N whispers to you, 'I'm yours for 5 gold, sweetie pie.'", FALSE,
          pl, 0, ch, TO_CHAR);
      act("$N whispers something to $n.", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_SMILE:
    case CMD_WINK:
      if (!IS_AWAKE(ch))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$n smiles evilly, looking for some fun..", TRUE, ch, 0, 0,
          TO_ROOM);
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
      act("$N tells you, 'piss off jerk.'", FALSE, pl, 0, ch, TO_CHAR);
      act("$N tells $n to piss off.", TRUE, pl, 0, ch, TO_ROOM);
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
      act("$N whispers to you, 'I like that.' in a sexy voice.", FALSE, pl, 0,
          ch, TO_CHAR);
      act("$N whispers something to $n in a sexy voice.", TRUE, pl, 0, ch,
          TO_ROOM);
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
          ("Your gentle nudging awakens $N, who yawns and clears her throat.",
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
      act("$N pokes you back, looking annoyed.", FALSE, pl, 0, ch, TO_CHAR);
      act("$N pokes $n back, looking annoyed.", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    default:
      return FALSE;
      break;
    }
  }
  return (FALSE);
}

int assassin_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n makes a lightning-fast move as he practices his backstab.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n bows before you.", TRUE, ch, 0, 0, TO_ROOM);
        mobsay(ch, "Walk in shadows, friend.");
        break;
      case 3:
        act("$n does a quick dodge in front of you, showing off his skill.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n watches you intently, a devious look in his eye.", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int brigand_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n looks at you with a curious expression.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Greetings, mate!");
        break;
      case 3:
        act("$n looks off onto the horizon.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n whistles a chipper tune.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int fisherman_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n casts his line into the harbor.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Damn fish ain't been biting all day.");
        break;
      case 3:
        act("$n stares off onto the horizon.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n slowly reels in his line.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int fisherman_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n mumbles something incoherent.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch,
               "I.. I.. I-I looove fishhhing..  I-It's sooooo relaxing, y'know?");
        mobsay(ch, "D-Do you like fishing?");
        break;
      case 3:
        act("$n burps loudly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n pukes over the size of the pier.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int sailor_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n gives you a casual glance.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Don't get in my way, mate. I got work to do.");
        break;
      case 3:
        act("$n looks across the dock for something or someone.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 4:
        act("$n looks as though he's been working hard all day.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int seaman_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n gives you an icy stare.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Out of my way, kid!");
        break;
      case 3:
        act("$n looks annoyingly at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n looks very proud of himself.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int naval_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n works diligently at his job.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "hey, could you hand some of those nails?");
        break;
      case 3:
        act("$n pounds at the ship plates.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n sweats from the strenuous work.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int naval_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n looks over the ship plans.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Looks good, boys. Keep it up.");
        break;
      case 3:
        act("$n inspects the underside of the ship for flaws.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 4:
        act("$n hands some nails to the worker.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int naval_three(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict, temp;
  char     Gbuf1[MAX_STRING_LENGTH];

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
      switch (dice(2, 7))
      {
      case 7:
        act("$n goes over some paperwork.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "What are you doing here?");
        break;
      case 3:
        act("$n looks out the window, across the harbor.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        mobsay(ch, "Do you have business here? I'm really quite busy.");
        break;
      case 5:
        act("$n files some paperwork away.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        act("$n gives you a stern glance.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
    else if (IS_FIGHTING(ch))
    {
      switch (dice(1, 4))
      {
      case 1:
        act("$n glares at you, putting more weight into her swing.", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "You have met your match, ignorant fool!");
        break;
      case 3:
        act("$n dances and twists about, showing amazing grace in combat.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        /*
           Attempt to solve the sanctuary of a player 
         */
        for (vict = world[ch->in_room].people; vict; vict = temp)
        {
          temp = vict->next_in_room;
          if (!number(0, 8) && (vict != ch) && !IS_TRUSTED(vict))
          {
            act("$n makes a powerful magical gesture.", TRUE, ch, 0, 0,
                TO_ROOM);
            act("A bolt of pure energy streaks out from her fingertips!",
                TRUE, ch, 0, 0, TO_ROOM);
            call_solve_sanctuary(ch, vict);
          }
        }
        /*
           Attempt to disarm the opponent 
         */
        if (vict && !number(0, 3) && (vict != ch) && !IS_TRUSTED(vict))
        {
          act("$n performs a martial arts maneuver with blinding speed!",
              TRUE, ch, 0, 0, TO_ROOM);
          strcpy(Gbuf1, vict->player.name);
          do_mobdisarm(ch, Gbuf1, 0);
        }
        break;
      }
    }
  }
  return (FALSE);
}

int naval_four(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n scans the horizon for sea vessels.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Howdy.");
        act("$n smiles warmly at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        mobsay(ch,
               "Just make sure you're not on the gates when I open them.");
        break;
      case 4:
        act("$n looks around the harbor, taking in everything.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

static int lighthouseone = 0;
static int lightcounter = 0;

int lighthouse_one(P_char ch, P_char pl, int cmd, char *arg)
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
  else if (time_info.hour == 8)
  {
    lighthouseone = 1;
    lightcounter = 0;
  }
  if ((lighthouseone == 1) && (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
    switch (lightcounter)
    {
    case 0:
      act("$n stares long across ocean, taking a deep breath.", FALSE, ch, 0,
          0, TO_ROOM);
      break;
    case 1:
      act("$n says, 'Beautiful view, isn't it?", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      act
        ("$n says, 'Been workin' here 30 years and never got over how lovely it is.'",
         FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      act("$n smiles at you with warm eyes.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act
        ("$n says, 'Only thing needs doin' now is to rebuild this old place.'",
         FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n says, 'Been here for a good hundred years, I reckin.'", FALSE,
          ch, 0, 0, TO_ROOM);
      break;
    case 6:
      act("$n says, 'Hehe.  Sometimes I'm surprised it's still standin.'",
          FALSE, ch, 0, 0, TO_ROOM);
      act("$n giggles softly.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 7:
      act
        ("$n says, 'This one used to be just fine, till they dug it all out.'",
         FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 8:
      act("$n grumbles, 'God knows what they were doin' down there..'", FALSE,
          ch, 0, 0, TO_ROOM);
      act("$n chuckles.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 9:
      act
        ("$n says, 'Course the military never tells us common folk what's really goin' on.'",
         FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 10:
      act("$n says, 'Aahhhh well, 'tis just the same anyway.", FALSE, ch, 0,
          0, TO_ROOM);
      act
        ("$n says, 'If they don't wanna tell us, it probably isn't important.",
         FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 11:
      act("$n says, 'You hear about the pirate traffic?'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 12:
      act("$n says, 'I heard it's gettin' worse, just what we need.'", FALSE,
          ch, 0, 0, TO_ROOM);
      break;
    case 13:
      act("$n snarls, 'More jackass brigands to deal with.'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 14:
      act("$n says, 'Ahh well, I'll shut up and enjoy the view.'", FALSE, ch,
          0, 0, TO_ROOM);
      act("$n smiles.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 15:
      act("$n stares long across ocean, taking a deep breath.", FALSE, ch, 0,
          0, TO_ROOM);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
      break;
    case 25:
      act("$n stares long across ocean, looking for ships.", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    default:
      break;
    }
  if (lightcounter == 30)
  {
    lightcounter = 0;
    lighthouseone = 0;
  }
  lightcounter++;
  return (FALSE);
}

int lighthouse_two(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

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
      case 5:
        act("$n smiles at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Howdy.");
        break;
      case 3:
        act("$n scans the horizon for sea going vessels.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n looks at you curiously.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
    if (IS_FIGHTING(ch))
    {
      switch (dice(1, 4))
      {
      case 1:
        act("$n growls with rage.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Why do you attack me? I have done nothing to you!");
        break;
      case 3:
        act("$n spits in your face.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n slaps you with a lightning fast move.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int seabird_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n flies close by, looking for a handout.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        act("$n chirps loudly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n pecks at something on the ground.", TRUE, ch, 0, 0, TO_ROOM);
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

int seabird_two(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n stares at you intently.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n notices something on the ground, and stares at it intently.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n flaps it's wings about on the ground.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n stands absolutely still, as if trying to look stoic.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int commoner_one(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n looks for a second, then looks away quickly.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Hello.");
        break;
      case 3:
        act("$n purposefully averts his gaze.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n whistles softly to himself.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int commoner_two(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

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
      case 5:
        act("$n smiles warmly at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Good day to you.");
        break;
      case 3:
        act("$n looks around for something or someone.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        mobsay(ch,
               "Have you seen a small boy, around eight years old, with red hair?");
        mobsay(ch, "I can't find him anywhere!");
        break;
      default:
        break;
      }
    }
    if (IS_FIGHTING(ch))
    {
      switch (dice(1, 4))
      {
      case 1:
        act("$n screams, 'Someone help me!'", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch,
               "Don't hurt me, please! I have three children to care for!");
        break;
      case 3:
        act("$n screams in terror.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n wails in agony.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int commoner_three(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n gazes long across the ocean, lost in thought.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Beautiful, isn't it?");
        act("$n smiles at you.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act
          ("$n takes a deep breath as the breeze blows in, looking very relaxed.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n closes her eyes, and looks deep in thought.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int commoner_four(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n grunts as he tries a difficult move.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "You think you can take me, eh?");
        break;
      case 3:
        act("$n spins around on the mat.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n growls.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int commoner_five(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n cheers enthusiastically!", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Get 'im!  Don't let get behind ya!");
        break;
      case 3:
        act("$n roots for her man.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n gasps as the struggle intensifies.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int commoner_six(P_char ch, P_char pl, int cmd, char *arg)
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
      case 5:
        act("$n hoots with joy as the struggle continues.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Go, dad, go!");
        break;
      case 3:
        act("$n runs around in excitement.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n cheers wildly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

static int crierhour1 = 0;
static int crierhour2 = 0;

int crier_one(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buf[MAX_INPUT_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (dice(2, 42))
      {
      case 2:
        act("$n gets ready for another bout of shouting.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 3:
        act("$n clears his throat.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        mobsay(ch, "Farmers east of Verzanan be advised!");
        mobsay(ch, "Orc raiding parties have been spotted.");
        break;
      case 5:
        act
          ("$n looks at you for a moment, seeing if his audience is listening.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        act("$n paces back and forth, apparently excited about his work.",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 7:
        mobsay(ch,
               "City officials report that unknown tunnels have been discovered in the graveyard!");
        mobsay(ch, "Best to be careful where you step there.");
        break;
      case 8:
        mobsay(ch,
               "I hear Gelian has the best deal in town for precious gemstones!");
        break;
      case 9:
        act
          ("$n says softly, 'Rumor has it Kalara and Lord Piergeiron are an item!'",
           TRUE, ch, 0, 0, TO_ROOM);
        act("$n giggles evilly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 10:
        mobsay(ch,
               "Lord Piergeiron is looking for volunteers to join the city's militia!");
        mobsay(ch, "Mostly for when we get hit by goblin raids and whatnot.");
        break;
      case 11:
        strcpy(buf, "If you're headin' south to Calimport, beware!");
        do_yell(ch, buf, 0);
        strcpy(buf,
               "Trolls, Giants, and many dangers may threaten your lives, go with caution.");
        do_yell(ch, buf, 0);
        break;
      case 12:
        act("$n nearly trips over his own feet.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 13:
        mobsay(ch,
               "Rumor has it that tunnels have been dug out beneath the city!");
        mobsay(ch, "Some say smugglers use it to traffic stolen goods.");
        break;
      case 14:
        act("The town crier burps loudly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 15:
        strcpy(buf,
               "Lord Piergeiron is looking for brave adventurers to fight off the trolls!");
        do_yell(ch, buf, 0);
        strcpy(buf, "If you can help, form a group and head south.");
        do_yell(ch, buf, 0);
        break;
      case 16:
        mobsay(ch,
               "The druids expect a storm sometime soon, look out for it!");
        break;
      case 17:
        mobsay(ch,
               "Hey, folks, I hear there are good things for sale in the bazaar!");
        break;
      case 18:
        mobsay(ch,
               "Announcement! Jakar has made a new secret formula for his ribs!");
        mobsay(ch, "Last one there is starving pig!");
        break;
      case 19:
        mobsay(ch,
               "There is rumor that burglars are wandering the rooftops of southern Verzanan!");
        mobsay(ch, "Be careful up there, I hear they're mighty tough!");
        break;
      case 20:
        mobsay(ch,
               "I hear the call girls south of here will show you a good time for 500 coins.");
        act("$n winks knowingly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 21:
        strcpy(buf,
               "Arena contests! All contenders report to the arena for a good fight!");
        do_yell(ch, buf, 0);
        act("$n cackles with anticipation of the blood feast.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      case 22:
        mobsay(ch, "Hey, folks! Need adventuring supplies?");
        mobsay(ch, "I hear Zakara has all you need at good prices..");
        break;
      case 23:
        mobsay(ch,
               "Remember to fill your barrels and ration up before leaving town!");
        mobsay(ch, "It's a hard and dangerous world out there.");
        break;
      case 24:
        act
          ("$n says softly, 'I hear the old fisherman out on Deepwater Isle has discount tickets to the Moonshaes!'",
           TRUE, ch, 0, 0, TO_ROOM);
        act("The guard glares at the town crier.", TRUE, ch, 0, 0, TO_ROOM);
        act("$n blushes, and backs off.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 25:
        mobsay(ch,
               "Rumor has it that there is woodsman east of Verzanan who has a tale to tell.");
        mobsay(ch, "People say it's important.");
        break;
      case 26:
        mobsay(ch, "Warning to all traveling south!");
        mobsay(ch, "The old ruin is said to be hollow..");
        mobsay(ch, "From what I hear, there is a portal to hell there!");
        act("$n shivers uncomfortably.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 27:
        mobsay(ch, "Be Warned!");
        mobsay(ch,
               "The Brotherhood of the Black Fire reports the end of the world is at hand!");
        mobsay(ch, "Looks like they're predicting earlier this year..");
        break;
      case 28:
        mobsay(ch,
               "Lord Piergeiron is asking for the return of the jewels stolen from his home recently.");
        mobsay(ch,
               "He promises that no harsh action will be taken against the thief.");
        mobsay(ch,
               "Which translates to: You wont be burned at the stake instantly, he'll give you a week and then cook ya.");
        mobsay(ch,
               "Anyone foolish enough to turn em in, inquire at the jail house.");
        break;
      case 29:
        mobsay(ch, "Oh! And one important thing I have to tell..");
        act
          ("$n is struck in the back of the head by an airborne tomato, though his assailant cannot be seen.",
           TRUE, ch, 0, 0, TO_ROOM);
        mobsay(ch, "HEY! That hurt!");
        break;
      case 30:
        mobsay(ch,
               "I hear the Tower of High Sorcery in the woods to the north is haunted.");
        mobsay(ch,
               "Many an adventurer is said to have entered, but few live to tell the tale.");
        break;
      case 31:
        mobsay(ch, "It's a long way to the Moonshaes!");
        mobsay(ch, "Only way to go is by ship, and even that takes a while.");
        break;
      case 32:
        mobsay(ch,
               "Scouts have reported a strange finding while at sea last week.");
        mobsay(ch,
               "A large rectangular object was discovered standing on top of the water!");
        mobsay(ch,
               "It's nature was unknown, though it was definitely magical.");
        break;
      case 33:
        strcpy(buf, "Bars are open all night boys!");
        do_yell(ch, buf, 0);
        break;
      case 34:
        mobsay(ch, "People say southern Verzanan, ");
        mobsay(ch, "is the roughest part of town..");
        break;
      case 35:
        mobsay(ch, "I hear a brothel is opening in southern Verzanan soon.");
        act("$n winks knowingly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 36:
        mobsay(ch,
               "Oooo, I haven't seen someone as ugly as you in a LONG time!");
        act("$n falls down laughing.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 37:
        mobsay(ch,
               "The sages of Verzanan have sometimes spoken of travelers who wander the land with great knowledge of the world..");
        mobsay(ch,
               "Some are great wizards who can teach rare and strange magic, while others know the way to hidden treasures.");
        mobsay(ch, "Seek them out, for they carry ancient wisdom.");
        break;
      case 38:
        mobsay(ch, "Oh! Lord Piergeiron wanted me to tell you.");
        mobsay(ch,
               "Make sure you give donations to the soup kitchen south of town, the poor need your help.");
        break;
      case 39:
        strcpy(buf, "Welcome merchants, welcome travelers!");
        do_yell(ch, buf, 0);
        strcpy(buf, "Welcome to Verzanan, City of Splendors!");
        do_yell(ch, buf, 0);
        send_to_zone_outdoor(12,
                             "A housewife screams, 'Shut up, you bloody idiot!\r\n'");
        break;
      case 40:
        act("$n drinks some water to clear his throat.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 41:
        mobsay(ch,
               "The Gods have brought good weather to the farmers again this year!");
        mobsay(ch, "They must be smiling upon us.");
        break;
      case 42:
        act
          ("A boy runs through the room, hitting the crier and knocking him over.",
           TRUE, ch, 0, 0, TO_ROOM);
        act("A boy says, 'Shut your stupid hole, fool!', and runs off", TRUE,
            ch, 0, 0, TO_ROOM);
        act("$n screams, 'Hey!  Stop that little shit!'", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        break;
      }
    }
    if (time_info.hour == 4)
      crierhour1 = 0;
    if (time_info.hour == 19)
      crierhour1 = 0;
    if (time_info.hour == 4)
      crierhour2 = 0;
    if (time_info.hour == 9)
      crierhour2 = 0;
    if ((time_info.hour == 3) && (crierhour2 == 0))
    {
      crierhour2 = 1;
      strcpy(buf,
             "The ship heading to the Moonshaes leaves in three hours, folks!");
      do_yell(ch, buf, 0);
    }
    if ((time_info.hour == 10) && (crierhour2 == 0))
    {
      crierhour2 = 1;
      strcpy(buf,
             "The ship heading for Calimport leaves in two hours, folks!");
      do_yell(ch, buf, 0);
    }
    if ((time_info.hour == 5) && (crierhour1 == 0))
    {
      crierhour1 = 1;
      strcpy(buf, "The shops start opening in one hour, folks!");
      do_yell(ch, buf, 0);
    }
    if ((time_info.hour == 18) && (crierhour1 == 0))
    {
      crierhour1 = 1;
      strcpy(buf, "Be advised, the shops will start closing soon!");
      do_yell(ch, buf, 0);
    }
  }
  if (IS_FIGHTING(ch))
  {
    strcpy(buf, "Help, help!  I'm being attacked!");
    do_yell(ch, buf, 0);
    send_to_zone_outdoor(world[ch->in_room].zone,
                         "People all over the city can be heard cheering.\r\n");
  }
  return (FALSE);
}

int verzanan_guard_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)))
    return (FALSE);

  if (cityguard(ch, pl, cmd, arg))
    return TRUE;

  if (!pl)
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL) && !number(0, 2))
    {
      switch (dice(2, 5))
      {
      case 5:
        act("$n scans the area for signs of trouble.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Good day, citizen!");
        break;
      case 3:
        act("$n looks at you intently for a moment, then smiles.", TRUE, ch,
            0, 0, TO_ROOM);
        break;
      case 4:
        act("$n looks around, observing everything for trouble.", TRUE, ch, 0,
            0, TO_ROOM);
        break;
      default:
        return FALSE;
        break;
      }
    }
    return (TRUE);
  }
  return FALSE;
}

int verzanan_guard_two(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)))
    return (FALSE);

  if (cityguard(ch, pl, cmd, arg))
    return TRUE;

  if (!pl)
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL) && !number(0, 2))
    {
      switch (dice(2, 5))
      {
      case 5:
        act("$n scans the landscape intently.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        mobsay(ch, "Hell of a view, isn't it?");
        break;
      case 3:
        act
          ("$n looks you up and down for a moment, then goes back to his duties.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n scans the area for signs of trouble.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      default:
        return FALSE;
        break;
      }
    }
    return TRUE;
  }
  return (FALSE);
}

int verzanan_guard_three(P_char ch, P_char pl, int cmd, char *arg)
{
  char     tbuf[40];
  int      i;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)))
    return (FALSE);

  if (pl && cityguard(ch, pl, cmd, arg))
    return TRUE;

  if (pl)
    return FALSE;

  if (MIN_POS(ch, POS_STANDING + STAT_NORMAL) && !number(0, 2))
  {
    switch (dice(2, 5))
    {
    case 5:
      act("$n scans the area for signs of trouble.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Hell of a day, isn't it?");
      break;
    case 3:
      act
        ("$n looks you up and down for a moment, then goes back to his duties.",
         TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act("$n stands tall and proud.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }

  return (FALSE);
}
int cell_drunk(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   c_obj = 0;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

#if 0
/***    kludge, will fix later   ***/
  if (IS_OUTLAW(ch))
    k->specials.jail_time = 120;
#endif

  /*
     Pc-Mobile interaction engine 
   */

  if ((GET_STAT(ch) < STAT_SLEEPING) || (IS_FIGHTING(ch)))
    return FALSE;

  if (cmd != 0)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);
    if (*Gbuf1)
      c_obj = get_char_room(Gbuf1, ch->in_room);
    switch (cmd)
    {
    case CMD_SMILE:
    case CMD_STRUT:
    case CMD_DREAM:
    case CMD_POSE:
    case CMD_CHEER:
    case CMD_HI5:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N vomits all over $n, giving him something to frown about..",
          TRUE, ch, 0, 0, TO_ROOM);
      act
        ("$N vomits all over your clothing, giving you something to frown about.",
         FALSE, pl, 0, ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_INSULT:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_insult(pl, arg, 0);
      act("$N humbles you with a greater insult.", FALSE, pl, 0, ch, TO_CHAR);
      act("$N snaps back with a greater insult.", FALSE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_POKE:
      /*
         An exchange of poking 
       */
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, arg, CMD_POKE);
      act("$N pokes you back.", FALSE, pl, 0, ch, TO_CHAR);
      act("$N pokes $n back.", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_KISS:
    case CMD_SNUGGLE:
    case CMD_CUDDLE:
    case CMD_CRY:
    case CMD_FONDLE:
    case CMD_GROPE:
    case CMD_LOVE:
    case CMD_NIBBLE:
    case CMD_SQUEEZE:
    case CMD_FRENCH:
    case CMD_SEDUCE:
    case CMD_UNDRESS:
    case CMD_BATHE:
    case CMD_FLIRT:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, arg, cmd);
      act("$N spits and coughs in disgust saying, 'Bleh!'.", TRUE, pl, 0, ch,
          TO_ROOM);
      act("$N spits and coughs in disgust saying, 'Bleh!'.", FALSE, pl, 0, ch,
          TO_CHAR);
      return TRUE;
      break;
    case CMD_PUKE:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N sees $n puke, and begins vomiting uncontrollably.", TRUE, ch, 0,
          0, TO_ROOM);
      act("$N sees you puke, and begins vomiting uncontrollably.", FALSE, pl,
          0, ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_CACKLE:
    case CMD_SCREAM:
    case CMD_APPLAUD:
    case CMD_SING:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N says, 'Will ya keep it down, $n? My head hurts!'.", TRUE, ch, 0,
          0, TO_ROOM);
      act("$N says, 'Will ya keep it down? My head hurts!'.", FALSE, pl, 0,
          ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_SNIFF:
    case CMD_WHINE:
    case CMD_WINCE:
    case CMD_ACK:
    case CMD_GRUMBLE:
    case CMD_WHATEVER:
    case CMD_HISS:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N says, 'Quit cryin' about it, we're all in this together.'",
          TRUE, ch, 0, 0, TO_ROOM);
      act("$N says, 'Quit cryin about it, we're all in this together.'",
          FALSE, pl, 0, ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_BURP:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N applauds and says with great cheer, 'Worthy! Good one!'.", TRUE,
          ch, 0, 0, TO_ROOM);
      act("$N applauds and says with great cheer, 'Worthy! Good one!", FALSE,
          pl, 0, ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_FART:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N says, 'Oh, god..  Will ya do that outside?!?'.", TRUE, ch, 0, 0,
          TO_ROOM);
      act("$N says, 'Oh, god..  Will ya do that outside?!?'.", FALSE, pl, 0,
          ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_SLAP:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N slaps $n back with great cheer, thinking it all a game.", TRUE,
          ch, 0, 0, TO_ROOM);
      act("$N slaps you back with great cheer, thinking it all a game.",
          FALSE, pl, 0, ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_LAUGH:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N frowns and says, 'What are YOU laughin' about, $n?'.", TRUE, ch,
          0, 0, TO_ROOM);
      act("$N frowns and says, 'What are YOU laughin' about, bub?'.", FALSE,
          pl, 0, ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_HUG:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N says, 'Don't hug me, $n, ya smell awful!'.", TRUE, ch, 0, 0,
          TO_ROOM);
      act("$N says, 'Don't hug me jailbait, ya smell awful!'.", FALSE, pl, 0,
          ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_GRIN:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N grins and whispers something to $n", TRUE, ch, 0, 0, TO_ROOM);
      act("$n grins and whispers to you, 'Got a plan mate?'.", FALSE, pl, 0,
          ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_GASP:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N says with alarm, 'What, WHAT?'.", TRUE, ch, 0, 0, TO_ROOM);
      act("$N says with alarm, 'What, WHAT?'.", FALSE, pl, 0, ch, TO_CHAR);
      return TRUE;
      break;
    case CMD_POUT:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$N says, 'Quit yer pouting, $n, and think of a way outta here'.",
          TRUE, ch, 0, 0, TO_ROOM);
      act("$N says, 'Quit yer poutin' and think of a way outta here'.", FALSE,
          pl, 0, ch, TO_CHAR);
      return TRUE;
      break;
    default:
      return FALSE;
      break;
    }
  }
  if (pl)
  {
    return (0);
  }
  else
  {
    if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      switch (number(2, 14))
      {
      case 2:
        mobsay(ch, "Jailer!! Lemme outta here, you worthless piece of shit!");
        break;
      case 3:
        mobsay(ch, "Ooohhh, my head hurts. I got such a hangover!");
        break;
      case 4:
        mobsay(ch, "Psssst. Hey, lets make a plan and bust outta here..");
        break;
      case 5:
        mobsay(ch, "Aarrrg. Aye, Jailer! I gotta take a dump, man!.");
        break;
      case 6:
        mobsay(ch, "Ammamma immma I'mma I'm thirsty, got any brew, mate?");
        break;
      case 7:
        mobsay(ch, "Wwaaa, whereee where am I?! Take me back to the bar!");
        break;
      case 8:
        mobsay(ch,
               "Hey, jailer! Gimme some ale, I'm dyin' a thirst in here!");
        break;
      case 9:
        mobsay(ch, "Damn! The cell door keeps moving on me!");
        break;
      case 10:
        mobsay(ch, "A-A-Anyone got some cards?");
        break;
      case 11:
        act("$n turns green.", TRUE, ch, 0, 0, TO_ROOM);
        mobsay(ch, "Maaan, I thhhink I'm gonna puke.");
        break;
      case 12:
        mobsay(ch, "Damn! S-Somebody needs a bath!");
        break;
      case 13:
        mobsay(ch, "You really suck..  You know that?");
        act("$n giggles", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 14:
        mobsay(ch, "I feel like I live here, maaaan.");
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int varon(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( !ch || !arg || (cmd != CMD_MURDER) )
  {
    return FALSE;
  }

  one_argument(arg, Gbuf1);
  if (!*Gbuf1)
  {
    send_to_char("Kill who?\r\n", ch);
    return TRUE;
  }
  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || !OBJ_WORN_POS(obj, WIELD) )
  {
    return FALSE;
  }

  if( !(victim = get_char_room_vis(ch, Gbuf1)) )
  {
    send_to_char("Kill who?\r\n", ch);
    return TRUE;
  }
  if( GET_LEVEL(ch) < 60 )
  {
    act("The disruptor hums, then blows up in your face!", FALSE, ch, 0, victim, TO_CHAR);
    act("$n attempts to fire the disruptor at $N!", TRUE, ch, 0, victim, TO_NOTVICT);
    act("The disruptor hums and blows up in $n's face!", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n attempts to fire the disruptor at you!", FALSE, ch, 0, victim, TO_VICT);
    act("The disruptor hums and blows up in $n's face!", FALSE, ch, 0, victim, TO_VICT);
    die(ch, ch);
    return TRUE;
  }
  if( ch == victim )
  {
    send_to_char("Your mother would be so sad.. :(\r\n", ch);
    return TRUE;
  }
  else
  {
    act("The mighty weapon hums softly for an instant, then glows brightly!", FALSE, ch, 0, victim, TO_CHAR);
    act("A multi-colored beam of radiant energy bursts forth from its tip, ", FALSE, ch, 0, victim, TO_CHAR);
    act("striking $N dead in the chest!  They scream in agony as the energy", FALSE, ch, 0, victim, TO_CHAR);
    act("dissolves their body, ripping their life force away.", FALSE, ch, 0, victim, TO_CHAR);
    act("In seconds, there is nothing left but a memory...", FALSE, ch, 0, victim, TO_CHAR);

    act("$n points $s disruptor at you, and pulls the trigger with a grin.", FALSE, ch, 0, victim, TO_VICT);
    act("The weapon glows, firing a bright beam of radiant energy directly ", FALSE, ch, 0, victim, TO_VICT);
    act("at you! You try to dodge, but too late. The beam strikes you dead", FALSE, ch, 0, victim, TO_VICT);
    act("in the chest! AAAAAAAAAAHHHHHHHHH!! Pain rips through your body ", FALSE, ch, 0, victim, TO_VICT);
    act("in waves, more intense than you've ever felt. As you look down, ", FALSE, ch, 0, victim, TO_VICT);
    act("the most horrible realization possible overcomes you as your", FALSE, ch, 0, victim, TO_VICT);
    act("entire body dissolves away, slowly enough for you to experience every", FALSE, ch, 0, victim, TO_VICT);
    act("ounce of pure agony. Darkness washes over you, and with your last", FALSE, ch, 0, victim, TO_VICT);
    act("glimpse of thought, you realize that death has won this day... ", FALSE, ch, 0, victim, TO_VICT);

    act("$n points his disruptor at $N, and pulls the trigger with a grin.", TRUE, ch, 0, victim, TO_NOTVICT);
    act("The disruptor glows and blasts forth a bright beam of radiant energy.", TRUE, ch, 0, victim, TO_NOTVICT);
    act("The beam strikes $N in the chest, who screams in unbelievable ", TRUE, ch, 0, victim, TO_NOTVICT);
    act("agony.  Their body begins to dissolve from the inside out, causing", TRUE, ch, 0, victim, TO_NOTVICT);
    act("the most exquisite pain imaginable. Within a few short seconds, ", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$N's body is gone, leaving only a horrible memory behind..", TRUE, ch, 0, victim, TO_NOTVICT);

    die(victim, ch);
    if( GET_LEVEL(ch) > 50 )
    {
      statuslog( ch->player.level, "%s got wasted by Miax's cool disruptor just now, aawww.", GET_NAME(victim) );
    }
    return TRUE;
  }
  return FALSE;
}

/*
   item proc 
 */
int gesen(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  if( cmd == CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) || obj->loc.wearing != ch || !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  // 1/10 chance.
  if( !number(0, 9) )
  {
    act("&+YThe hammer glows and flies from your hand, striking &n$N&+Y with a crash!\r\n"
      "&+yTwirling wildly, it arcs back around and returns to your hand..", FALSE, ch, obj, vict, TO_CHAR);
    act("&+Y$n's hammer glows and flies through the air, striking &n$N&+Y with a crash!\r\n"
      "&+yTwirling around in the air, it arcs and returns to $n's grasp.", FALSE, ch, obj, vict, TO_ROOM);
    spell_harm(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
  }
  else
  {
    if( !ch->specials.fighting )
    {
      set_fighting(ch, vict);
    }
  }
  // Do the normal hit damage as well.
  if (ch->specials.fighting)
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

int secret_door(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = obj->value[1];
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || !arg)
    return (FALSE);

  if (cmd != CMD_ENTER)
    return (FALSE);

  one_argument(arg, Gbuf1);
  if (!isname(Gbuf1, obj->name))
    return (FALSE);

  act("You step through the $q.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n steps through the $q.", FALSE, ch, obj, 0, TO_ROOM);

  if (!IS_TRUSTED(ch))
  {
    if (GET_HIT(ch) > dam)
      GET_HIT(ch) -= dam;
    else
      GET_HIT(ch) = dam;
    StartRegen(ch, EVENT_HIT_REGEN);
  }
  teleport_to(ch, real_room(obj->value[0]), 0);

  return (TRUE);
}

int verzanan_portal(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = obj->value[1];
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || !arg)
    return (FALSE);
  if (cmd != CMD_ENTER)
    return (FALSE);

  one_argument(arg, Gbuf1);
  if (!isname(Gbuf1, obj->name))
    return (FALSE);

  act("You step into the portal.\r\n"
      "Multi-colored lights flash and dance all about you!\r\n"
      "You feel yourself stretch and twist across an extra-dimensional plain, then..",
      FALSE, ch, obj, 0, TO_CHAR);
  act("$n steps through the portal.", FALSE, ch, obj, 0, TO_ROOM);

  if (!IS_TRUSTED(ch))
  {
    if (GET_HIT(ch) > dam)
      GET_HIT(ch) -= dam;
    else
      GET_HIT(ch) = dam;
    StartRegen(ch, EVENT_HIT_REGEN);
  }
  /*
     this is sort of a clunky way to do it -- DTS * teleport_to(ch,
     real_room(obj->value[0])); 
   */
  /*
     this is much nicer, and doesn't trigger the guildmasters 
   */
  char_from_room(ch);
  char_to_room(ch, real_room(obj->value[0]), -1);

  return (TRUE);
}

/*
   ******************************************* 
 */
/*
   Special procedures for the Verzanan guilds 
 */
/*
   ******************************************* 
 */

/*
   This proc is used to stop people from attacking guild guards. It will
   reduce them to 1 hit point, set their outlaw flag, and randomly teleport
   them away.                                               --MIAX          
 */

int guild_protection(P_char ch, P_char pl)
{
  int      Experience, Loss;

  if (!SanityCheck(ch, "guild_protection - guard") ||
      !SanityCheck(pl, "guild_protection - vict") ||
      !IS_FIGHTING(ch) || (ch->specials.fighting != pl))
    return 0;

  if (IS_NPC(pl))
    return 0;

  mobsay(ch,
         "Begone from here Outlaw! None are allowed to attack guild guardians!");
  act
    ("$n pressed a small metal pin on $s chest, which flares with brilliant blue light!",
     FALSE, ch, 0, 0, TO_ROOM);
  send_to_char
    ("&+WYou feel a wretching pain penetrate you and drain your lifefore away!\r\n&+WOuch! That really did HURT!\r\n",
     pl);

  spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, pl, 0);
  spell_curse(60, ch, NULL, SPELL_TYPE_SPELL, pl, 0);
  spell_poison(120, ch, 0, 0, pl, 0);
  spell_blindness(60, ch, 0, 0, pl, 0);
  spell_slow(60, ch, 0, 0, pl, 0);
  spell_harm(60, ch, NULL, 0, pl, 0);

#if 0
  if (IS_PC(pl))
  {
    if (!IS_SET(pl->only.pc->law_flags, PLR_OUTLAW))
      SET_BIT(pl->only.pc->law_flags, PLR_OUTLAW);
  }
  else
  {
    if (!IS_OUTLAW(pl))
      SET_BIT(pl->specials.act, NPC_OUTLAW);
  }
#endif
  spell_teleport(60, ch, 0, 0, pl, 0);
  return (1);
}

int guild_guard_one(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, evil = NULL;
  int      max_evil = 1000;
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if ((ch->in_room == real_room(5500)) && (cmd == CMD_EAST))
  {
    if (!GET_CLASS(pl, CLASS_PALADIN))
    {
      act
        ("The holy warrior steps in front of $n, blocking $s entry into the guild.",
         FALSE, pl, 0, 0, TO_ROOM);
      act
        ("The holy paladin says, 'I'm sorry, but only holy paladins may enter here.'",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The holy warrior steps in front of you, blocking your way.\r\n",
         pl);
      send_to_char
        ("The holy paladin says, 'I'm sorry, but only holy paladins may enter here.'\r\n",
         pl);
      return TRUE;
    }
    else
    {
      act("The holy warrior bows reverently before $n as $s enters.", FALSE,
          pl, 0, 0, TO_ROOM);
      act("The holy paladin tells $n, 'You are welcome, brethren..'", FALSE,
          pl, 0, 0, TO_ROOM);
      act("$N leaves east into the guild.", FALSE, ch, 0, pl, TO_NOTVICT);
      send_to_char
        ("The holy warrior bows reverently before you as you enter.\r\n", pl);
      send_to_char("The holy paladin says, 'You are welcome, brethren..'\r\n",
                   pl);
      sprintf(Gbuf3, "%s arrives from the west.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(5501));
      char_from_room(pl);
      char_to_room(pl, real_room(5501), 0);
      return TRUE;
    }
  }
  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)))
    return (FALSE);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch))
    {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
          (IS_NPC(tch) || IS_NPC(tch->specials.fighting)))
      {
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }

  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0))
  {
    act("$n screams 'PROTECT THE INNOCENT! BANZAI! CHARGE! ARARAGGGHH!'",
        FALSE, ch, 0, 0, TO_ROOM);
    MobStartFight(ch, evil);
    return (TRUE);
  }
  if (OutlawAggro(ch, "$n screams '%s! Fresh blood! Kill!'"))
    return (TRUE);

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n smiles warmly at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Greetings, brethren! Welcome to the temple of Paladins.");
      break;
    case 4:
      act("The holy warriors change positions at their post.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 5:
      act("$n looks at you curiously for a moment.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n snarls at you in confusion.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Why do you fight me? You stand no chance of victory!");
      break;
    case 3:
      act("$n performs a dazzling display of swordwork against you.", TRUE,
          ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch,
             "This fight is pointless! Surrender now before I am forced to kill you!");
      break;
    default:
      break;
    }
  }
  return FALSE;

}

int guild_guard_two(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if ((ch->in_room == real_room(5510)) && (cmd == CMD_EAST))
  {
    if (!GET_CLASS(pl, CLASS_MERCENARY))
    {
      act
        ("The minotaur slaps $n across the face as $s tries to enter the guild.",
         FALSE, pl, 0, 0, TO_ROOM);
      act
        ("The minotaur snarls, 'Get lost, idiot, or I'll have you for lunch!'",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char("The huge minotaur slaps you across the face! *SMACK*\r\n",
                   pl);
      send_to_char
        ("The minotaur snarls, 'Get lost, idiot, or I'll have you for lunch!'\r\n",
         pl);
      return TRUE;
    }
    else
    {
      act("The minotaur fixes $n with a stern eye as $s enters the guild.",
          FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The minotaur fixes you with a stern eye as you enter the guild.\r\n",
         pl);
      act("$N leaves east, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the west.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(5511));
      char_from_room(pl);
      char_to_room(pl, real_room(5511), 0);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n snarls at you with contempt.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Wanna fight, pig? I could use some exercise..");
      act("$n grins wickedly at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act("$n makes a face, looking incredibly bored.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 5:
      act("$n looks at you with a hungry glint in his eye.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n grows menacingly and foams at the mouth.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 2:
      mobsay(ch,
             "Die, worm!! I'm hungry, and you'll make a PERFECT lunchtime snack!");
      break;
    case 3:
      act("$n charges at you!", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch,
             "Ha! Come now, child, surely you can fight better than THAT!");
      act("$n laughs sarcastically at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  return FALSE;

}

int guild_guard_three(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if ((ch->in_room == real_room(5520)) && (cmd == CMD_SOUTH))
  {
    if (!GET_CLASS(pl, CLASS_MONK))
    {
      act
        ("The monk respectfully blocks $n's entry into the temple, and whisper something.",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The monk respectfully blocks your entry into the temple.\r\n", pl);
      send_to_char
        ("The monk whispers to you, 'Only pure warriors may enter here..'\r\n",
         pl);
      return TRUE;
    }
    else
    {
      act("The monk bows respectfully before $n as $s enters.", FALSE, pl, 0,
          0, TO_ROOM);
      send_to_char("The monk bows before you as you enter.\r\n", pl);
      act("$N leaves south, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the north.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(5521));
      char_from_room(pl);
      char_to_room(pl, real_room(5521), 0);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n smiles and bows respectfully to you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Good day, brother..");
      break;
    case 4:
      act("$n studies you carefully for a moment.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n whispers something to his companion.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n does a flip to the side, and attack from a different angle.",
          TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "The fight is pointless! You cannot win!");
      break;
    case 3:
      act("$n moves with blinding speed as he attacks.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 4:
      mobsay(ch, "What do you hope to accomplish by fighting me!");
      break;
    default:
      break;
    }
  }
  return FALSE;
}

int guild_guard_four(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, evil = NULL;
  int      g_prot, max_evil = 1000;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if (!pl)
    return FALSE;

  if ((ch->in_room == real_room(5534)) && (cmd == CMD_UP))
  {
    if (!GET_CLASS(pl, CLASS_BARD))
    {
      act("The bouncer roughly grabs $n and pushes $s back.", FALSE, pl, 0, 0,
          TO_ROOM);
      act
        ("The bouncer says, 'You don't have any business up there, pal, get lost.'",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char("The bouncer roughly grabs you and pushes you back.\r\n",
                   pl);
      send_to_char
        ("The bouncer says, 'You don't have any business up there, pal, get lost.\r\n",
         pl);
      return TRUE;
    }
    else
    {
      act("The bouncer steps aside as $n climbs up the staircase.", FALSE, pl,
          0, 0, TO_ROOM);
      send_to_char
        ("The bouncer steps aside as you climb up the staircase.\r\n", pl);
      act("$N leaves up, entering the guild.", FALSE, ch, 0, pl, TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from below.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(5535));
      char_from_room(pl);
      char_to_room(pl, real_room(5535), 0);
      return TRUE;
    }
  }
  if (cmd || !IS_AWAKE(ch))
    return (FALSE);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch))
    {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
          (IS_NPC(tch) || IS_NPC(tch->specials.fighting)))
      {
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }
  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0))
  {
    if (IS_FIGHTING(evil))
      stop_fighting(evil);
    StopAllAttackers(evil);
    act("$n yells, 'HEY! No fighting in here, dead beat!'", FALSE, ch, 0,
        evil, TO_ROOM);
    act("The bouncer picks you and drags you out of the bar!", FALSE, evil, 0,
        ch, TO_CHAR);
    act("$n drags $n out towards the street!", FALSE, ch, 0, evil,
        TO_NOTVICT);
    char_from_room(evil);
    char_to_room(evil, real_room(5533), 0);
    char_from_room(ch);
    char_to_room(ch, real_room(5533), 0);
    act("A bouncer storms into the room dragging $N by the collar", FALSE, ch,
        0, evil, TO_NOTVICT);
    act
      ("$N kicks and screams, all to no avail, as the bouncer drags $S east.",
       FALSE, ch, 0, evil, TO_NOTVICT);
    char_from_room(evil);
    char_to_room(evil, real_room(5531), 0);
    char_from_room(ch);
    char_to_room(ch, real_room(5531), 0);
    act("A bouncer storms into the room dragging $N by the collar", FALSE, ch,
        0, evil, TO_NOTVICT);
    act
      ("$N kicks and screams, all to no avail, as the bouncer drags $S east.",
       FALSE, ch, 0, evil, TO_NOTVICT);
    char_from_room(evil);
    char_to_room(evil, real_room(5530), 0);
    char_from_room(ch);
    char_to_room(ch, real_room(5530), 0);
    act("A bouncer storms into the room dragging $N by the collar", FALSE, ch,
        0, evil, TO_NOTVICT);
    act
      ("$N kicks and screams, all to no avail, as the bouncer drags $S east.",
       FALSE, ch, 0, evil, TO_NOTVICT);
    char_from_room(evil);
    char_to_room(evil, real_room(3258), 0);
    char_from_room(ch);
    char_to_room(ch, real_room(3258), 0);
    act("$N is thrown from the tavern and lands in a heap on the ground!",
        FALSE, ch, 0, evil, TO_NOTVICT);
    act("The bouncer snarls at you, 'Next time I'll break your neck, punk!'",
        FALSE, evil, 0, ch, TO_CHAR);
    act("The bouncer snarls at $N, 'Next time I'll break your neck, punk!'",
        FALSE, ch, 0, evil, TO_NOTVICT);
    act("The bouncer reenters the tavern.", FALSE, ch, 0, evil, TO_ROOM);
    SET_POS(evil, POS_SITTING + GET_STAT(evil));
    char_from_room(ch);
    char_to_room(ch, real_room(5534), 0);
    return (TRUE);
  }
  if (OutlawAggro(ch, "$n screams '%s! Fresh blood! Kill!'"))
    return (TRUE);

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n flexes his muscles, trying to look superior.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Hey, look at puny here!");
      act("The bouncer laughs at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act("$n stares you down, thinking himself to be hot shit.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    case 5:
      act("$n whispers something under his breath.", TRUE, ch, 0, 0, TO_ROOM);
      act("The bouncer laughs at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n snarls as he swings at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Come on, puny, you MUST be able to better than THAT!");
      break;
    case 3:
      act("$n swings lazily at you, as if bored.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "How long can you hold it, puny!?");
      break;
    default:
      break;
    }
  }
  return FALSE;

}

int guild_guard_five(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if (!pl)
    return FALSE;
  if ((ch->in_room == real_room(5540)) && (cmd == CMD_SOUTH))
  {
    if (!GET_CLASS(pl, CLASS_RANGER))
    {
      act("The unicorn walks in front of $n, preventing $s from going south.",
          FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The unicorn walks in front of you, preventing you from going south.\r\n",
         pl);
      send_to_char
        ("You hear a voice in your head, 'I'm sorry, but you cannot enter here, friend..'\r\n",
         pl);
      return TRUE;
    }
    else
    {
      act("The unicorn nudges $n affectionately as $s enters the guild.",
          FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The unicorn nudges you affectionately as you enter the guild.\r\n",
         pl);
      act("$N leaves south, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the north.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(5541));
      char_from_room(pl);
      char_to_room(pl, real_room(5541), 0);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n nibbles some grass near one of the trees.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 3:
      act("$n looks shyly at you for a moment.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act("$n lazily swats at a fly with its tail.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n walks in and about the trail, grazing.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 3))
    {
    case 1:
      act("You hear a voice in your head, 'Stop this madness!'", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    case 2:
      act("$n forces a suggestion into your mind, 'FLEE! RUN AWAY!'", TRUE,
          ch, 0, 0, TO_ROOM);
      send_to_char
        ("You flee in panic, running wildly under the unicorn's spell.", pl);
      do_flee(pl, 0, 2);
      send_to_char
        ("You flee in panic, running wildly under the unicorn's spell.", pl);
      do_flee(pl, 0, 2);
      send_to_char
        ("You flee in panic, running wildly under the unicorn's spell.", pl);
      do_flee(pl, 0, 2);
      send_to_char
        ("You flee in panic, running wildly under the unicorn's spell.", pl);
      do_flee(pl, 0, 2);
      break;
    case 3:
      act("$n moves with blinding speed as it attacks.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    default:
      break;
    }
  }
  return FALSE;
}

int guild_guard_six(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if (!pl)
    return FALSE;
  if ((ch->in_room == real_room(5560)) && (cmd == CMD_EAST))
  {
    if (!GET_CLASS(pl, CLASS_DRUID))
    {
      act
        ("The massive treant blocks $n from going east with a wall of branches.",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The massive treant blocks you from going east with a wall of branches.\r\n",
         pl);
      return TRUE;
    }
    else
    {
      act("The massive treant drops a leaf on $n as $s enters the grove.",
          FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("A leaf falls on your head from the treant above as you enter the grove.\r\n",
         pl);
      act("$N leaves east, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the west.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(5561));
      char_from_room(pl);
      char_to_room(pl, real_room(5561), 0);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 4))
    {
    case 2:
      act("You can hear leaves rustling in the branches of the trees above.",
          TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      act("You feel as if the massive treant is watching you.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    case 4:
      act("$n moves slightly.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n groans as it strikes at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "You DARE attack the trees! You shall PAY, worm!");
      break;
    case 3:
      act("$n strikes at you with dozens of branches.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 4:
      act("The massive treant issues a very low groan!", TRUE, ch, 0, 0,
          TO_ROOM);
      act
        ("The sound blasts your ears painfully, causing you to scream and flee!",
         TRUE, ch, 0, 0, TO_ROOM);
      send_to_char
        ("You flee in panic, running wildly under the treant's spell.", ch);
      do_flee(pl, 0, 2);
      send_to_char
        ("You flee in panic, running wildly under the treant's spell.", ch);
      do_flee(pl, 0, 2);
      break;
    default:
      break;
    }
  }
  return FALSE;
}

int guild_guard_seven(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if ((ch->in_room == real_room(5570)) && (cmd == CMD_EAST))
  {
    if (!GET_CLASS(pl, CLASS_NECROMANCER))
    {
      act("The flesh golem roughly grabs $n and tosses $s backwards.", FALSE,
          pl, 0, 0, TO_ROOM);
      send_to_char
        ("The flesh golem roughly grabs you and tosses you backwards!\r\n",
         pl);
      return TRUE;
    }
    else
    {
      act("The golem looks at $n for a moment as $s enters the dark grove.",
          FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The golem looks at you for a moment as you enter the dark grove.\r\n",
         pl);
      act("$N leaves east, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the west.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(5571));
      char_from_room(pl);
      char_to_room(pl, real_room(5571), 0);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n swings wildly with both arms at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Muuuggreerrra!");
      break;
    case 3:
      act("$n dives at you, swinging viciously.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act("$n moves around with lighting speed.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  return FALSE;
}

int guild_guard_eight(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH], tbuf[40];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if (!pl)
    return FALSE;
  if ((ch->in_room == real_room(5572)) && (cmd == CMD_EAST))
  {
    if (!GET_CLASS(pl, CLASS_NECROMANCER))
    {
      act
        ("The guard steps in front of $n, preventing $s entrance into the tower.",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The guard steps in front of you, preventing your entrance into the tower.\r\n",
         pl);
      return TRUE;
    }
    else
    {
      act("The guard bows deeply before $n, allowing $s entrance.", FALSE, pl,
          0, 0, TO_ROOM);
      send_to_char
        ("The guard bow deeply before you, allowing you entrance.\r\n", pl);
      strcpy(tbuf, "door");
      do_open(ch, tbuf, 0);
      act("$N leaves east, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the west.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(5573));
      char_from_room(pl);
      char_to_room(pl, real_room(5573), 0);
      do_close(ch, tbuf, 0);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n studies you curiously for a moment.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Greetings, brethren..");
      break;
    case 4:
      act("$n looks around, as if searching for something.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 5:
      act("$n casts a minor cantrip, probably to amuse himself.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n utters the words, 'Teleport!'", TRUE, ch, 0, 0, TO_ROOM);
      act("You slowly fade out, and rematerialize on Deepwater Island!", TRUE,
          ch, 0, 0, TO_ROOM);
      act("$N slowly fades out of existence.", FALSE, ch, 0, pl, TO_NOTVICT);
      char_from_room(pl);
      char_to_room(pl, real_room(5364), 0);
      act("$N slowly fades into existence.", FALSE, ch, 0, pl, TO_NOTVICT);
      break;
    case 2:
      mobsay(ch, "DIE, worm! You are no match for me!");
      break;
    case 3:
      act("$n prepares another incantation.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "How long can you last, worm?! I can go all day!");
      break;
    default:
      break;
    }
  }
  return FALSE;

}

int guild_guard_nine(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH], tbuf[40];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if (!pl)
    return FALSE;

  if ((ch->in_room == real_room(3038)) && (cmd == CMD_SOUTH))
  {
    if (GET_CLASS(pl, CLASS_SORCERER) || GET_CLASS(pl, CLASS_CONJURER) || GET_CLASS(pl, CLASS_SUMMONER))
    {
      act("The guard bows curtly before $n, and motions $s inside.", FALSE,
          pl, 0, 0, TO_ROOM);
      send_to_char
        ("The guard bows curtly before you, and motions you inside.\r\n", pl);
      strcpy(tbuf, "door");
      do_open(ch, tbuf, 0);
      act("$N leaves south, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the north.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(3039));
      char_from_room(pl);
      char_to_room(pl, real_room(3039), 0);
      do_close(ch, tbuf, 0);
      return TRUE;
    }
    else
    {
      act
        ("The guard steps in front of $n, barring $s from entering the tower.",
         FALSE, pl, 0, 0, TO_ROOM);
      act("The guard tells $n, 'You may not enter here!'", FALSE, pl, 0, 0,
          TO_ROOM);
      send_to_char
        ("The guard steps in front of you, barring you from entering the tower.\r\n",
         pl);
      send_to_char("The guard says, 'You may not enter here!'\r\n", pl);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n stares at you for a long moment.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Good day to you.");
      break;
    case 4:
      act("$n looks about for trouble.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n kicks a stone near the tower, looking very bored.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n dances wildly about as he fights.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Have at thee, nave! Thou cannot defeat me!");
      break;
    case 3:
      act("$n utters the words, 'PANIC!'", TRUE, ch, 0, 0, TO_ROOM);
      do_flee(pl, 0, 2);
      break;
    case 4:
      mobsay(ch, "Your attack is foolish, worm, be gone!");
      break;
    default:
      break;
    }
  }
  return FALSE;

}

int guild_guard_ten(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH], tbuf[40];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if ((ch->in_room == real_room(3067)) && (cmd == CMD_NORTH))
  {
    if (GET_CLASS(pl, CLASS_CLERIC) || GET_CLASS(pl, CLASS_SHAMAN))
    {
      act("The guild guardian smiles at $n, and allows $s inside.", FALSE, pl,
          0, 0, TO_ROOM);
      send_to_char
        ("The guild guardian smiles at you, and allows you inside.\r\n", pl);
      strcpy(tbuf, "door");
      do_open(ch, tbuf, 0);
      act("$N leaves north, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the south.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(3068));
      char_from_room(pl);
      char_to_room(pl, real_room(3068), 0);
      do_close(ch, tbuf, 0);
      return TRUE;
    }
    else
    {
      act("The guild guardian stops $n before $s can enter the temple.",
          FALSE, pl, 0, 0, TO_ROOM);
      act("The guild guardian says, 'Only those of holy faith may enter.'",
          FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The guild guardian stops you before you can enter the temple.\r\n",
         pl);
      send_to_char
        ("The guild guardian says, 'Only those of holy faith may enter.'\r\n",
         pl);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n smiles warmly at you, and appears to be nice fellow.", TRUE, ch,
          0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Good day to you, friend!");
      break;
    case 4:
      act("$n whistles quietly to himself.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n says, 'Hell of a day, isn't it?'", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n frowns at you, more out of pity than anger.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 2:
      mobsay(ch, "You should not be doing this, it is pointless!");
      break;
    case 3:
      act("$n looks annoyed by your assault, but little more.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Stop this foolish attack! It will get you nowhere!");
      break;
    default:
      break;
    }
  }
  return FALSE;
}

int guild_guard_eleven(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH], tbuf[40];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if ((ch->in_room == real_room(3055)) && (cmd == CMD_SOUTH))
  {
    if (GET_CLASS(pl, CLASS_WARRIOR) || GET_CLASS(pl, CLASS_ANTIPALADIN))
    {
      act("The guardian says to $n, 'Heya, mate, go right on in!", FALSE, pl,
          0, 0, TO_ROOM);
      send_to_char
        ("The guardian says to you, 'heya, mate, go right on in!\r\n", pl);
      strcpy(tbuf, "door");
      do_open(ch, tbuf, 0);
      act("$N leaves south, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the north.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(3056));
      char_from_room(pl);
      char_to_room(pl, real_room(3056), 0);
      do_close(ch, tbuf, 0);
      return TRUE;
    }
    else
    {
      act("The guardian steps in front of the door, barring $n's entrance.",
          FALSE, pl, 0, 0, TO_ROOM);
      act
        ("The guardian says, 'Only warriors can enter here! Now, get lost..'",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The guardian steps in front of the door, barring your entrance.\r\n",
         pl);
      send_to_char
        ("The guardian says, 'only warriors can enter here! Now, get lost..'\r\n",
         pl);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n smirks arrogantly.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Why are you here loitering about in front of my guild?");
      break;
    case 4:
      act("$n stares at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n whittles on a piece of wood.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n fights you in a perfect rhythm, dancing and dodging about.",
          TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch,
             "Boy! Yer not too bright, are ya! When you gonna give in and flee kid?");
      break;
    case 3:
      act("$n slaps you as he dances by, making you look like an idiot.",
          TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Give it up, fool. You cannot defeat me!");
      break;
    default:
      break;
    }
  }
  return FALSE;

}

int guild_guard_twelve(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH], tbuf[40];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if ((ch->in_room == real_room(3283)) && (cmd == CMD_EAST))
  {
    if (!GET_CLASS(pl, CLASS_ROGUE))
    {
      act
        ("The rogue slaps $n across the face as $s tries to enter the guild.",
         FALSE, pl, 0, 0, TO_ROOM);
      act("The rogue snarls at $n, 'Get lost, kid.'", FALSE, pl, 0, 0,
          TO_ROOM);
      send_to_char
        ("The rogue slaps you across the face as you try and enter the guild.\r\n",
         pl);
      send_to_char("The rogue snarls at you, 'Get lost, kid.'\r\n", pl);
      return TRUE;
    }
    else
    {
      act("The rogue nods to $n, and motions $s to enter.", FALSE, pl, 0, 0,
          TO_ROOM);
      send_to_char("The rogue nods to you, and motions you to enter.\r\n",
                   pl);
      strcpy(tbuf, "secret");
      do_open(ch, tbuf, 0);
      act("$N leaves east, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the west.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(3284));
      char_from_room(pl);
      char_to_room(pl, real_room(3284), 0);
      do_close(ch, tbuf, 0);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n gives you a long, cold stare.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "You lookin' for somethin', pal?");
      break;
    case 4:
      act("$n seems to stick close to the back wall.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 5:
      act("$n whispers something to one of the patrons.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n growls menacingly.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch,
             "You wanna die that bad, eh, fool? I'm only too happy to help!");
      break;
    case 3:
      act("$n fights with amazing agility.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Come now, fool, surely you can fight better than that!");
      break;
    default:
      break;
    }
  }
  return FALSE;

}

int guild_guard_thirteen(P_char ch, P_char pl, int cmd, char *arg)
{
  int      g_prot;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(GET_HOME(ch)))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    g_prot = guild_protection(ch, ch->specials.fighting);
    if (g_prot == 1)
      return (TRUE);
  }
  if ((ch->in_room == real_room(2951)) && (cmd == CMD_NORTH))
  {
    if (!GET_SPEC(pl, CLASS_ROGUE, SPEC_ASSASSIN))
    {
      act("The assassin trips $n as $s tries to go north.", FALSE, pl, 0, 0,
          TO_ROOM);
      send_to_char
        ("The assassin trips you as you try to go north. *ouch*\r\n", pl);
      SET_POS(pl, POS_KNEELING + GET_STAT(pl));
      return TRUE;
    }
    else
    {
      act
        ("The assassin bows, though never taking his eyes off $n as $s pass by him.",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("The assassin bows, though never taking his eyes off you as you pass by him.\r\n",
         pl);
      act("$N leaves north, entering the guild.", FALSE, ch, 0, pl,
          TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the south.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(2952));
      char_from_room(pl);
      char_to_room(pl, real_room(2952), 0);
      return TRUE;
    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n eyes you cautiously, waiting for some trick.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 3:
      mobsay(ch, "What's up?");
      break;
    case 4:
      act("$n leans against the wall, staring at you.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 5:
      act("$n absently flips a dagger through the air.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n cackles with glee as he attacks, obviously enjoying the fray..",
          TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "I'll bet you've come in search of Kang, eh?");
      mobsay(ch,
             "Well, they're gonna have a harder time searching for your corpse!");
      break;
    case 3:
      act("$n moves with deadly grace, and is obviously very skilled.", TRUE,
          ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "I WILL win you know, and then, your items are MINE!");
      act("$n cackles with insane glee.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  return FALSE;

}

int young_paladin_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n practices a cunning sword maneuver.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Good day to ya!");
      break;
    case 4:
      act("$n practices a sword maneuver, loses his balance, and falls over.",
          TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n smiles at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n fights furiously, although his inexperience shows.", TRUE, ch,
          0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Someone help me!");
      break;
    case 3:
      act("$n nearly trips and falls.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "I am a member of the holy paladins guild!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int wrestler_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n circles around the arena, looking for a good attack opening.",
          TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "You ain't no match fer me, dead beat!");
      break;
    case 4:
      act("$n dives at his opponent with a vengeance.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 5:
      act("$n grunts as his opponent gets the better of him.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n tries to pull a wrestling move on you.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 2:
      mobsay(ch, "I'll break yer damn neck, dead beat!");
      break;
    case 3:
      act("$n almost gets a solid hold on you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "C'mon, brothers, lets get this guy!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int young_mercenary_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n lokos at you with sly curiousity.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Hi.");
      break;
    case 4:
      act("$n keeps a wary eye on you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n tries to look impressive by practicing his backstab.", TRUE, ch,
          0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("A look of terror comes over $n.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "I ain't got nothin! Leave me alone!");
      break;
    case 3:
      act("$n tries to get in behind you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "You're gonna pay for this!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int young_monk_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n smiles at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "May you be at peace, brethren.");
      break;
    case 4:
      act("$n relights one of the candles.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n looks about for other spent candles.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n tries in vain to stop the combat.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Please do not continue this!");
      break;
    case 3:
      act("$n chops at you with her hands in a wicked hook pattern.", TRUE,
          ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Why?! Why do you attack me!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int selune_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n slips and falls off his barstool, burping loudly.", TRUE, ch, 0,
          0, TO_ROOM);
      mobsay(ch, "Ooooops, now I donnnne it..");
      break;
    case 3:
      mobsay(ch, "Hey, waiter! B-Bring me another dr-drink!");
      break;
    case 4:
      act("$n begins talking to himself.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n looks at you through a drunken haze.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n tries to flee and falls flat on his ass.", TRUE, ch, 0, 0,
          TO_ROOM);
      SET_POS(ch, POS_SITTING + GET_STAT(ch));
      break;
    case 2:
      mobsay(ch, "Heeeeeeeellllllppppp Mmmmmeeeeeeee!");
      break;
    case 3:
      act("$n makes a wild swing at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Get off me, man!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int selune_two(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n scans the bar, as if looking for someone.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Hey, could you bring us another round?");
      break;
    case 4:
      act("$n smiles at you, then goes back to his conversation.", TRUE, ch,
          0, 0, TO_ROOM);
      break;
    case 5:
      act("$n takes a long draught from his bottle.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n looks about for an escape.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Hey, Bouncer! Kick this asshole outta here!");
      break;
    case 3:
      act("$n swings, *SMACK* And hits you right across the jaw.", TRUE, ch,
          0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Whats up with you, man?!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int selune_three(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n giggles at some joke.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "You think so? I heard that too but wasn't sure..");
      break;
    case 4:
      act("$n exchanges glances with you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n takes a drink from her glass.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n lets out a terrified scream.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Someone help me!");
      break;
    case 3:
      act("$n tries to dodge your attacks.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Stop hitting me, you asshole!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int selune_four(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 7))
    {
    case 2:
      act("$n stumbles and nearly falls on his ass.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Hey! Where's that pretty dancing girl I saw!");
      mobsay(ch, "She had hooters like you wouldn't believe!");
      break;
    case 4:
      act("$n looks around bar for someone.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n takes a long draught from his bottle.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 6:
      act("$n tries to stumble into the other room, but fails.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    case 7:
      act("$n stumbles over a chair.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n falls on his ass as he tries to kill you.", TRUE, ch, 0, 0,
          TO_ROOM);
      SET_POS(ch, POS_SITTING + GET_STAT(ch));
      break;
    case 2:
      mobsay(ch, "Wha .. What?! Is someone attacking me?!");
      break;
    case 3:
      act("$n tries to flee, but fails miserably.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "I-I didn't do nothin! I'm innocent!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int selune_five(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n moves slowly through the room, dancing all the way.", TRUE, ch,
          0, 0, TO_ROOM);
      break;
    case 3:
      act("$n does a little dance in front of one of the patrons.", TRUE, ch,
          0, 0, TO_ROOM);
      break;
    case 4:
      act("$n smiles at you as she dances by.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n moves with the light-footed grace of a elven maiden.", TRUE, ch,
          0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n screams in terror!", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Bouncer! Bouncer! Get this creep offa me!");
      break;
    case 3:
      act("$n slaps you hard.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Get away from me you, jerk!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int selune_six(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n takes a small sip from his drink.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Damn, work's been killing me lately.");
      break;
    case 4:
      act("$n lets out a long sigh.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n smiles and laughs at his friend's joke.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n growls and throws a bottle at you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "Hey, bouncers! Get this guy, will ya!");
      break;
    case 3:
      act("$n attempts to dodge your strikes.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Get off me, you dung pile!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int young_druid_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n smiles at you warmly.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Greetings, friend! Another fine day in the forest, eh?");
      break;
    case 4:
      act("$n performs a holy ritual.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n looks out through the woods.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n calls upon the power of her Goddess.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      mobsay(ch, "You will pay for this!");
      break;
    case 3:
      act("$n attempts to cast a spell at you, but it fails.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 4:
      mobsay(ch, "Someone help me!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int young_necro_one(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 5))
    {
    case 2:
      act("$n casts a minor cantrip, and smiles with pride.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 3:
      mobsay(ch, "I love the art, don't you?");
      break;
    case 4:
      act("$n observes you briefly.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n practices some incantation.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (dice(1, 4))
    {
    case 1:
      act("$n casts a spell, but it fizzles out and fails.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 2:
      mobsay(ch, "The high master will kill you for this!");
      break;
    case 3:
      act("$n attempts to flee away, but fails.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      mobsay(ch, "I will kill you!");
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int bouncer_one(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, evil = NULL;
  int      bp_var = 0, max_evil = 1000;
  const int b_path[] = {
    5533, 5531, 5530, 3258, -1
  };

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)) || GET_MASTER(ch))
    return (FALSE);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch))
    {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
          (IS_NPC(tch) || IS_NPC(tch->specials.fighting)))
      {
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }
  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0))
  {
    while (evil && ch && (evil->in_room == ch->in_room) &&
           (real_room(b_path[bp_var]) == ch->in_room))
    {
      switch (bp_var)
      {
      case 0:
        if (IS_FIGHTING(evil))
          stop_fighting(evil);
        StopAllAttackers(evil);
        act("$n yells, 'HEY! No fighting in here, dead beat!'",
            FALSE, ch, 0, evil, TO_ROOM);
        act("The bouncer picks you up and drags your ass out of the tavern!",
            FALSE, evil, 0, ch, TO_CHAR);
        act("$n grabs $N by the collar and drags $S out of the tavern!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        break;
      case 1:
        act("A bouncer storms into the room, dragging $N by the collar!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("$N kicks and flails around as $s is dragged out.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("Many of the patrons look on in interest or laugh openly.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        send_to_char
          ("You are dragged through the tavern kicking and screaming.\r\n",
           ch);
        send_to_char("Many people stare or laugh at you as you go.\r\n", ch);
        break;
      case 2:
        act("A bouncer storms into the room, dragging $N by the collar!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("$N kicks and flails around as $s is dragged out.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("Many of the patrons look on in interest or laugh openly.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        send_to_char
          ("You are dragged through the tavern kicking and screaming.\r\n",
           ch);
        send_to_char("Many people stare or laugh at you as you go.\r\n", ch);
        break;
      case 3:
        act
          ("A bouncer throws $N from the tavern, who lands in a heap on the ground!",
           FALSE, ch, 0, evil, TO_NOTVICT);
        act
          ("The bouncer throws you onto the ground. *ouch* You land in a heap.",
           FALSE, evil, 0, ch, TO_CHAR);
        act
          ("The bouncer snarls at you, 'Next time I'll break your neck, punk!'",
           FALSE, evil, 0, ch, TO_CHAR);
        act
          ("The bouncer snarls at $N, 'Next time I'll break your neck, punk!'",
           FALSE, ch, 0, evil, TO_NOTVICT);
        act("The bouncer reenters the Smile.", FALSE, ch, 0, evil,
            TO_NOTVICT);
        act("The bouncer reenters the Smile.", FALSE, evil, 0, ch, TO_CHAR);
        break;
      }
      bp_var++;
      if (b_path[bp_var] != NOWHERE)
      {
        char_from_room(ch);
        char_to_room(ch, real_room(b_path[bp_var]), -1);
        char_from_room(evil);
        char_to_room(evil, real_room(b_path[bp_var]), -1);
      }
      else
      {
        SET_POS(evil, POS_SITTING + GET_STAT(evil));
        char_from_room(ch);
        char_to_room(ch, real_room(b_path[0]), -1);
        act("A bouncer walks back in, smiling smugly.",
            TRUE, ch, 0, 0, TO_ROOM);
        return (TRUE);
      }
    }
  }
  if ((ch->in_room != real_room(b_path[0])) && !GET_MASTER(ch))
  {
    act("$n looks around dazedily, and says 'Where the HELL am I?'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'Damn!  Luna'll KILL me, I gotta get back to work!'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n slowly fades from view.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(b_path[0]), -1);
    act("$n pops into view, with a sulphurous BANG!.",
        FALSE, ch, 0, 0, TO_ROOM);
  }
  return OutlawAggro(ch, "$n screams '%s! Fresh blood! Kill!'");
}

int bouncer_two(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, evil = NULL;
  int      bp_var = 0, max_evil = 1000;
  const int b_path[] = {
    5531, 5530, 3258, -1
  };

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)) || GET_MASTER(ch))
    return (FALSE);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch))
    {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
          (IS_NPC(tch) || IS_NPC(tch->specials.fighting)))
      {
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }
  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0))
  {
    while (evil && ch && (evil->in_room == ch->in_room) &&
           (real_room(b_path[bp_var]) == ch->in_room))
    {
      switch (bp_var)
      {
      case 0:
        if (IS_FIGHTING(evil))
          stop_fighting(evil);
        StopAllAttackers(evil);
        act("$n yells, 'HEY! No fighting in here, dead beat!'",
            FALSE, ch, 0, evil, TO_ROOM);
        act("The bouncer picks you up and drags your ass out of the tavern!",
            FALSE, evil, 0, ch, TO_CHAR);
        act("$n grabs $N by the collar and drags $S out of the tavern!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        break;
      case 1:
        act("A bouncer storms into the room, dragging $N by the collar!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("$N kicks and flails around as $s is dragged out.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("Many of the patrons look on in interest or laugh openly.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        send_to_char
          ("You are dragged through the tavern kicking and screaming.\r\n",
           ch);
        send_to_char("Many people stare or laugh at you as you go.\r\n", ch);
        break;
      case 2:
        act
          ("A bouncer throws $N from the tavern, who lands in a heap on the ground!",
           FALSE, ch, 0, evil, TO_NOTVICT);
        act
          ("The bouncer throws you onto the ground. *ouch* You land in a heap.",
           FALSE, evil, 0, ch, TO_CHAR);
        act
          ("The bouncer snarls at you, 'Next time I'll break your neck, punk!'",
           FALSE, evil, 0, ch, TO_CHAR);
        act
          ("The bouncer snarls at $N, 'Next time I'll break your neck, punk!'",
           FALSE, ch, 0, evil, TO_NOTVICT);
        act("The bouncer reenters the Smile.", FALSE, ch, 0, evil,
            TO_NOTVICT);
        act("The bouncer reenters the Smile.", FALSE, evil, 0, ch, TO_CHAR);
        break;
      }
      bp_var++;
      if (b_path[bp_var] != NOWHERE)
      {
        char_from_room(ch);
        char_to_room(ch, real_room(b_path[bp_var]), -1);
        char_from_room(evil);
        char_to_room(evil, real_room(b_path[bp_var]), -1);
      }
      else
      {
        SET_POS(evil, POS_SITTING + GET_STAT(evil));
        char_from_room(ch);
        char_to_room(ch, real_room(b_path[0]), -1);
        act("A bouncer walks back in, smiling smugly.",
            TRUE, ch, 0, 0, TO_ROOM);
        return (TRUE);
      }
    }
  }
  if ((ch->in_room != real_room(b_path[0])) && !GET_MASTER(ch))
  {
    act("$n looks around dazedily, and says 'Where the HELL am I?'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'Damn!  Luna'll KILL me, I gotta get back to work!'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n slowly fades from view.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(b_path[0]), -1);
    act("$n pops into view, with a sulphurous BANG!.",
        FALSE, ch, 0, 0, TO_ROOM);
  }
  return OutlawAggro(ch, "$n screams '%s! Fresh blood! Kill!'");
}

int bouncer_three(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, evil = NULL;
  int      bp_var = 0, max_evil = 1000;
  const int b_path[] = {
    5530, 3258, -1
  };

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)) || GET_MASTER(ch))
    return (FALSE);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch))
    {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
          (IS_NPC(tch) || IS_NPC(tch->specials.fighting)))
      {
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }
  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0))
  {
    while (evil && ch && (evil->in_room == ch->in_room) &&
           (real_room(b_path[bp_var]) == ch->in_room))
    {
      switch (bp_var)
      {
      case 0:
        if (IS_FIGHTING(evil))
          stop_fighting(evil);
        StopAllAttackers(evil);
        act("$n yells, 'HEY! No fighting in here, dead beat!'",
            FALSE, ch, 0, evil, TO_ROOM);
        act("The bouncer picks you up and drags your ass out of the tavern!",
            FALSE, evil, 0, ch, TO_CHAR);
        act("$n grabs $N by the collar and drags $S out of the tavern!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("A loud thud can be heard.", FALSE, ch, 0, evil, TO_NOTVICT);
      case 1:
        act
          ("A bouncer throws $N from the tavern, who lands in a heap on the ground!",
           FALSE, ch, 0, evil, TO_NOTVICT);
        act
          ("The bouncer throws you onto the ground. *ouch* You land in a heap.",
           FALSE, evil, 0, ch, TO_CHAR);
        act
          ("The bouncer snarls at you, 'Next time I'll break your neck, punk!'",
           FALSE, evil, 0, ch, TO_CHAR);
        act
          ("The bouncer snarls at $N, 'Next time I'll break your neck, punk!'",
           FALSE, ch, 0, evil, TO_NOTVICT);
        act("The bouncer reenters the Smile.", FALSE, ch, 0, evil,
            TO_NOTVICT);
        act("The bouncer reenters the Smile.", FALSE, evil, 0, ch, TO_CHAR);
        break;
      }
      bp_var++;
      if (b_path[bp_var] != NOWHERE)
      {
        char_from_room(ch);
        char_to_room(ch, real_room(b_path[bp_var]), -1);
        char_from_room(evil);
        char_to_room(evil, real_room(b_path[bp_var]), -1);
      }
      else
      {
        SET_POS(evil, POS_SITTING + GET_STAT(evil));
        char_from_room(ch);
        char_to_room(ch, real_room(b_path[0]), -1);
        act("A bouncer walks back in, smiling smugly.",
            TRUE, ch, 0, 0, TO_ROOM);
        return (TRUE);
      }
    }
  }
  if ((ch->in_room != real_room(b_path[0])) && !GET_MASTER(ch))
  {
    act("$n looks around dazedily, and says 'Where the HELL am I?'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'Damn!  Luna'll KILL me, I gotta get back to work!'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n slowly fades from view.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(b_path[0]), -1);
    act("$n pops into view, with a sulphurous BANG!.",
        FALSE, ch, 0, 0, TO_ROOM);
  }
  return OutlawAggro(ch, "$n screams '%s! Fresh blood! Kill!'");
}

int bouncer_four(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, evil = NULL;
  int      bp_var = 0, max_evil = 1000;
  const int b_path[] = {
    5532, 5531, 5530, 3258, -1
  };

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)) || GET_MASTER(ch))
    return (FALSE);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch))
    {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
          (IS_NPC(tch) || IS_NPC(tch->specials.fighting)))
      {
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }
  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0))
  {
    while (evil && ch && (evil->in_room == ch->in_room) &&
           (real_room(b_path[bp_var]) == ch->in_room))
    {
      switch (bp_var)
      {
      case 0:
        if (IS_FIGHTING(evil))
          stop_fighting(evil);
        StopAllAttackers(evil);
        act("$n yells, 'HEY! No fighting in here, dead beat!'",
            FALSE, ch, 0, evil, TO_ROOM);
        act("The bouncer picks you up and drags your ass out of the tavern!",
            FALSE, evil, 0, ch, TO_CHAR);
        act("$n grabs $N by the collar and drags $S out of the tavern!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        break;
      case 1:
        act("A bouncer storms into the room, dragging $N by the collar!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("$N kicks and flails around as $s is dragged out.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("Many of the patrons look on in interest or laugh openly.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        send_to_char
          ("You are dragged through the tavern kicking and screaming.\r\n",
           ch);
        send_to_char("Many people stare or laugh at you as you go.\r\n", ch);
        break;
      case 2:
        act("A bouncer storms into the room, dragging $N by the collar!",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("$N kicks and flails around as $s is dragged out.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        act("Many of the patrons look on in interest or laugh openly.",
            FALSE, ch, 0, evil, TO_NOTVICT);
        send_to_char
          ("You are dragged through the tavern kicking and screaming.\r\n",
           ch);
        send_to_char("Many people stare or laugh at you as you go.\r\n", ch);
        break;
      case 3:
        act
          ("A bouncer throws $N from the tavern, who lands in a heap on the ground!",
           FALSE, ch, 0, evil, TO_NOTVICT);
        act
          ("The bouncer throws you onto the ground. *ouch* You land in a heap.",
           FALSE, evil, 0, ch, TO_CHAR);
        act
          ("The bouncer snarls at you, 'Next time I'll break your neck, punk!'",
           FALSE, evil, 0, ch, TO_CHAR);
        act
          ("The bouncer snarls at $N, 'Next time I'll break your neck, punk!'",
           FALSE, ch, 0, evil, TO_NOTVICT);
        act("The bouncer reenters the Smile.", FALSE, ch, 0, evil,
            TO_NOTVICT);
        act("The bouncer reenters the Smile.", FALSE, evil, 0, ch, TO_CHAR);
        break;
      }
      bp_var++;
      if (b_path[bp_var] != NOWHERE)
      {
        char_from_room(ch);
        char_to_room(ch, real_room(b_path[bp_var]), -1);
        char_from_room(evil);
        char_to_room(evil, real_room(b_path[bp_var]), -1);
      }
      else
      {
        SET_POS(evil, POS_SITTING + GET_STAT(evil));
        char_from_room(ch);
        char_to_room(ch, real_room(b_path[0]), -1);
        act("A bouncer walks back in, smiling smugly.",
            TRUE, ch, 0, 0, TO_ROOM);
        return (TRUE);
      }
    }
  }
  if ((ch->in_room != real_room(b_path[0])) && !GET_MASTER(ch))
  {
    act("$n looks around dazedily, and says 'Where the HELL am I?'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'Damn!  Luna'll KILL me, I gotta get back to work!'",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$n slowly fades from view.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(b_path[0]), -1);
    act("$n pops into view, with a sulphurous BANG!.",
        FALSE, ch, 0, 0, TO_ROOM);
  }
  return OutlawAggro(ch, "$n screams '%s! Fresh blood! Kill!'");
}
