
/*
 * ***************************************************************************
 * *  File: actcomm.c                                          Part of Duris *
 * *  Usage: Communication with other players.
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "assocs.h"
#include "graph.h"
#include "events.h"
#include "world_quest.h"
#include "sql.h"
#include "epic.h"
#include "listen.h"
#include "map.h"

/* external variables */
extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern P_index obj_index;
extern P_index mob_index;
extern int rev_dir[];
extern struct zone_data *zone_table;
extern struct race_names race_names_table[];
extern const char *dirs2[];

struct social_messg
{
  int      act_nr;
  int      hide;
  int      min_victim_position; /* Position of victim */
  /* No argument was supplied */
  char    *char_no_arg;
  char    *others_no_arg;
  /* An argument was there, and a victim was found */
  char    *char_found;          /* if NULL, read no further, ignore args */
  char    *others_found;
  char    *vict_found;
  /* An argument was there, but no victim was found */
  char    *not_found;

  /*
   * The victim turned out to be the character
   */
  char    *char_auto;
  char    *others_auto;
}       *soc_mess_list = NULL;

struct pose_type
{
  int      level;               /*
                                 * minimum level for poser
                                 */
  char    *poser_msg[4];        /*
                                 * message to poser
                                 */
  char    *room_msg[4];         /*
                                 * message to room
                                 */
} pose_messages[MAX_MESSAGES];

static int list_top = -1;

/*
 * This updates values for everybody in the character list which
 */
/*
 * control their ability to "speak" without overloading the system
 */

#ifdef OVL
void update_ovl(void)
{
  P_desc   d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->character && IS_PC(d->character) && !d->connected &&
        (d->character->only.pc->ovl_count > 0))
    {
      if (++(d->character->only.pc->ovl_timer) > OVL_PULSE)
      {
        d->character->only.pc->ovl_timer = 0;
        d->character->only.pc->ovl_count -= OVL_NORMAL_LIMIT;
      }
    }
  }
}

#endif

#ifdef OVL
bool is_overload(P_char ch)
{
  if (IS_PC(ch) && (++ch->only.pc->ovl_count > OVL_NORMAL_LIMIT))
  {
    send_to_char
      ("SHUT UP ALREADY!!! Your actions lose meaning at this speed!\r\n", ch);
    /*
     * So that nobody will be locked out for too long
     */
    ch->only.pc->ovl_count =
      MIN(ch->only.pc->ovl_count, 2 * OVL_NORMAL_LIMIT);
    return (TRUE);
  }
  return (FALSE);
}
#endif

bool is_silent(P_char ch, bool showit)
{
  // Makes more sense for a non-existant char to not be able to speak/hear.
  if( !ch )
  {
    return TRUE;
  }

  if( ch->in_room != NOWHERE
    && ((IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT)
    || IS_SET(world[ch->in_room].room_flags, ROOM_SILENT)
    || IS_AFFECTED2(ch, AFF2_SILENCED) || (IS_PC(ch) && IS_SET(ch->specials.act, PLR_SILENCE))
    || affected_by_spell_flagged(ch, SKILL_THROAT_CRUSH, AFFTYPE_CUSTOM1))
    && !IS_TRUSTED(ch)) )
  {
    if( showit )
    {
      send_to_char("The world is deathly silent, and no noise forms...\r\n", ch);
    }
    return (TRUE);
  }
  return (FALSE);
}

bool can_talk(P_char ch)
{
  if (CAN_SPEAK(ch))
    return (TRUE);
  act("$n makes an undiscernable attempt to communicate.", TRUE, ch, 0, 0,
      TO_ROOM);
  act("You motion your lips, attempting to communicate..", FALSE, ch, 0, 0,
      TO_CHAR);
  return (FALSE);
}

/*
 * this routine adds some error checking to mob's speech, and also
 * eliminates the 'discards const' compile errors.  Errors were not
 * dangerous, but if do_say() had attempted to change the text passed to
 * it, we get mystery crashes, better safe than sorry.  Use this when you
 * have fixed text that the NPC says, mob specials were the worst
 * offenders, all converted now. JAB
 */

void mobsay(P_char ch, const char *msg)
{
  char     Gbuf[MAX_INPUT_LENGTH];

  if (!SanityCheck(ch, "mobsay"))
    return;

  if (!msg || !*msg)
  {
    logit(LOG_EXIT, "No text in mobsay()");
    raise(SIGSEGV);
  }
  if (strlen(msg) > (MAX_INPUT_LENGTH - 1))
  {
    logit(LOG_EXIT, "text too long in mobsay()");
    raise(SIGSEGV);
  }
  strcpy(Gbuf, msg);
  do_say(ch, Gbuf, -4);
}

void check_magic_doors(P_char ch, const char *word)
{
  int      door, other_room;
  char     arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  char    *arg;
  struct room_direction_data *back;

  for (door = 0; door < NUM_EXITS; door++)
  {
    if (EXIT(ch, door) && (EXIT(ch, door)->key == -2) &&
        IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED) &&
        EXIT(ch, door)->keyword)
    {
      /*
       * now search to see if last word in keyword is the one
       */
      arg = EXIT(ch, door)->keyword;
      half_chop(arg, arg1, arg2);
      while (*arg2)
      {
        arg = arg2;
        half_chop(arg, arg1, arg2);
      }

      if (!*arg1)
        return;

      if (isname(word, arg1))
      {                         /*
                                 * matching for door
                                 */
        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
        if (IS_SET(EXIT(ch, door)->exit_info, EX_SECRET))
          REMOVE_BIT(EXIT(ch, door)->exit_info, EX_SECRET);
        if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        {
          if ((back = world[other_room].dir_option[rev_dir[door]]))
          {
            if (back->to_room == ch->in_room)
            {
              REMOVE_BIT(back->exit_info, EX_LOCKED);
              if (IS_SET(back->exit_info, EX_SECRET))
                REMOVE_BIT(back->exit_info, EX_SECRET);
            }
          }
        }
        sprintf(Gbuf1, "The %s begins to hum, then glow brightly.",
                FirstWord(EXIT(ch, door)->keyword));
        act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
        act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
/*
 * send_to_room(Gbuf1, (EXIT(ch, door)->to_room));
 */
        sprintf(Gbuf1, "A magical force unlocks the %s.",
                FirstWord(EXIT(ch, door)->keyword));
        act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
        act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
        return;
      }
    }
  }
}

void do_petition(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if(IS_NPC(ch) ||
     IS_SET(ch->specials.act, PLR_SILENCE) ||
     IS_SET(ch->specials.act2, PLR2_B_PETITION))
  {
    send_to_char("You are not permitted to use petition!\r\n", ch);
    return;
  }
  
  if(!IS_SET(ch->specials.act, PLR_PETITION))
  {
    send_to_char("You can't petition!\r\n", ch);
    return;
  }
/*  if (GET_LEVEL(ch) < 25) {
    send_to_char("Sorry, you can't petition until level 25!\r\n", ch);
    return;
  }*/
  while (*argument == ' ' && *argument != '\0')
    argument++;

  if (!*argument)
    send_to_char("Petition what?\r\n", ch);
  else if (can_talk(ch) /*|| IS_ILLITHID(ch) */ )
  {
    if (IS_SET(ch->specials.act, PLR_ECHO))
    {
      sprintf(Gbuf1, "&+rYou petition '%s'\r\n"
              "&+RThe petition channel is not for general conversation. Use the idea, typo, or bug commands.&n\r\n",
              argument);
      send_to_char(Gbuf1, ch);
    }
    else
      send_to_char("Ok.\r\n", ch);
    
    logit(LOG_PETITION, "(%s) petitioned (%s).", GET_NAME(ch), argument);
    if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s petitioned '%s'", GET_NAME(ch), argument);
    
    sprintf(Gbuf1, "&+r$n petitions '%s'&N", argument);
    sprintf(Gbuf2, "&+r%s petitions '%s'&N", GET_NAME(ch), argument);

    for(i = descriptor_list; i; i = i->next)
      if(!i->connected && !is_silent(i->character, FALSE) &&
          (i->character != ch) &&
          IS_SET(i->character->specials.act, PLR_PETITION) &&
          IS_TRUSTED(i->character))
      {
        if(IS_TRUSTED(ch))
          act(Gbuf1, 0, ch, 0, i->character, TO_VICT | ACT_PRIVATE);
        else
          act(Gbuf2, 0, ch, 0, i->character, TO_VICT | ACT_PRIVATE);
      }
      
   //CharWait(ch, PULSE_VIOLENCE);
  }
}

void send_to_gods(char *argument)
{
  P_desc   i;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character &&
        IS_SET(i->character->specials.act, PLR_PETITION) &&
        IS_TRUSTED(i->character))
      send_to_char(argument, i->character);
}

/*
 * ch is char saying whatever is in arg
 */

void send_to_avatar(P_char ch, const char *arg)
{
  P_char   i;
  char     Gbuf[MAX_STRING_LENGTH];

  if (!ch)
    return;
  if (!(ch->desc) || !(ch->desc->snoop.snooping))
  {
    send_to_char
      ("Being knocked unconscious strictly limits what you can do.\r\n", ch);
    return;
  }
  sprintf(Gbuf, "\r\n&+m%s projects, '&+M%s&n&+m'&n\r\n", GET_NAME(ch), arg);
  send_to_char(Gbuf, ch->desc->snoop.snooping);
}

void do_say(P_char ch, char *argument, int cmd)
{
  SanityCheck(ch, "do_say");

  say(ch, argument);
}

int say(P_char ch, const char *argument)
{
  int      i;
  P_char   kala;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH],
    Gbuf3[MAX_STRING_LENGTH];

  for (i = 0; *(argument + i) == ' '; i++) ;

  if (!*(argument + i))
    send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_KNOCKED_OUT))
  {
    send_to_avatar(ch, argument + i);
    return TRUE;
  }
  else if ((IS_SET(world[ch->in_room].room_flags, UNDERWATER) ||
            ch->specials.z_cord < 0) && !IS_TRUSTED(ch) && !IS_NPC(ch))
  {
    send_to_char("You try to say something in the water...", ch);
    act("$n's mouth moves, but all that comes forth is bubbles...", TRUE, ch,
        0, 0, TO_ROOM);
    return FALSE;
  }
  else if ( ( IS_AFFECTED2(ch, AFF2_SILENCED) || affected_by_spell(ch, SPELL_SUPPRESSION) ) && !IS_ILLITHID(ch)  && !IS_PILLITHID(ch))
  {
    send_to_char("You move your lips, but no sound comes forth!\r\n", ch);
    act("$n seems to be trying to say something, but you hear no words...",
        TRUE, ch, 0, 0, TO_ROOM);
    return FALSE;
  }
  else if(affected_by_spell_flagged(ch, SKILL_THROAT_CRUSH, AFFTYPE_CUSTOM1))
  {
    send_to_char("Your throat hurts far too much to speak!\r\n", ch);
    
    act("$n seems to hoarsely grumble something unintelligable.", TRUE, ch, 0, 0, TO_ROOM);
    
    return FALSE;
  }
  else if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You cannot speak in that form!\r\n", ch);
    return FALSE;
  }
  else if (!is_silent(ch, TRUE) && can_talk(ch))
  {
    for (kala = world[ch->in_room].people; kala; kala = kala->next_in_room)
    {
      if ((kala != ch) && (ch->specials.z_cord == kala->specials.z_cord))
      {
        if (IS_ILLITHID(ch) || IS_PILLITHID(ch) || !strcmp(GET_NAME(ch), "Xzthu"))
        {
          if (IS_ILLITHID(kala) || IS_PILLITHID(kala) || IS_TRUSTED(kala))
            sprintf(Gbuf2, "$n invades your mind with '%s'", argument + i);
          else
            sprintf(Gbuf2, "$n projects '%s'", argument + i);
        }
        else
        {
          sprintf(Gbuf3, "%s", argument + i);

          if (IS_THRIKREEN(ch))
          {
            sprintf(Gbuf2, "$n chitters %s'%s'", language_known(ch, kala),
                    language_CRYPT(ch, kala, Gbuf3));
          }
          else
            sprintf(Gbuf2, "$n says %s'%s'", language_known(ch, kala),
                    language_CRYPT(ch, kala, Gbuf3));
        }

        act(Gbuf2, FALSE, ch, 0, kala, TO_VICT | ACT_SILENCEABLE);
      }
    }

    if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
    {
      if (IS_ILLITHID(ch) || IS_PILLITHID(ch) || !strcmp(GET_NAME(ch), "Id"))
        sprintf(Gbuf1, "You project '%s'\r\n", argument + i);
      else
        sprintf(Gbuf1, "You %s %s'%s'\r\n",
                IS_THRIKREEN(ch) ? "chitter" : "say", language_known(ch, ch),
                argument + i);
      send_to_char(Gbuf1, ch);
    }
    else
      send_to_char("Ok.\r\n", ch);
    
    if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s says '%s'", GET_NAME(ch), argument + i);
    
    listen_broadcast(ch, (argument + i), LISTEN_SAY);
    
    check_magic_doors(ch, argument + i);
  }

  return TRUE;
}

void do_channel(P_char ch, char *argument, int cmd)
{
  send_to_char("Use toggle command from now on!\r\n", ch);
  return;
}

/*
bool
CAN_GCC(P_char ch)
{
  if (IS_TRUSTED(ch))
    return TRUE;
  if ((ch->equipment[WEAR_HEAD]) &&
      (obj_index[ch->equipment[WEAR_HEAD]->R_num].virtual == 101))
    return TRUE;

  return FALSE;
}
*/

void do_gcc(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  P_char   to_ch;
  ush_int  from_guild, to_guild;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     guild_name[1024];
  FILE    *f;

  from_guild = GET_A_NUM(ch);

  if( IS_NPC(ch) )
  {
    // Unguilded NPCs can't gcc (Golems/Magic Mouths can)..
    if( !from_guild )
    {
      send_to_char("Your a dumb unguilded mob, wth?!\r\n", ch);
      return;
    }
  }
  // The specials.act flags are different for NPCs thus the need for the else
  else if( IS_SET(ch->specials.act, PLR_SILENCE) || !IS_SET(ch->specials.act, PLR_GCC) )
  {
    send_to_char("You can't use the GCC channel!\r\n", ch);
    return;
  }

  if( affected_by_spell(ch, SPELL_SUPPRESSION) || is_silent(ch, TRUE) )
  {
    send_to_char("If you can't 'say', 'shout', or 'emote', what makes you think you can 'gcc'?\r\n", ch);
    return;
  }

  // This is pretty old, but ok (still works)..
  // Multi chars' primary level stays the same, allow multis to gcc at secondary levels 1-24..
  if( ch->player.level < 25 )
  {
    send_to_char("You cannot join a guild until level 25, thus you cannot GCC until level 25.\r\n", ch);
    return;
  }

  if( IS_AFFECTED(ch, AFF_WRAITHFORM) )
  {
    send_to_char("You can't speak in this form!\r\n", ch);
    return;
  }
  // If not guilded, not a member, or a PC who's not above parole rank.
  if( !from_guild || !IS_MEMBER(GET_A_BITS(ch)) || ( IS_PC(ch) && !GT_PAROLE(GET_A_BITS(ch)) ) )
  {
    send_to_char("Try becoming part of a guild first!  DUH!\r\n", ch);
    return;
  }

  // Skip leading spaces..
  while( *argument == ' ' )
  {
    argument++;
  }
  // If they gcc'd without content..
  if( !*argument )
  {
    send_to_char("GCC? Yes! Fine! Chat we must, but WHAT??\r\n", ch);
  }
  // All guilded NPCs can talk, can_talk() checks race (shapechanged into an animal -> no speaking).
  else if( IS_NPC(ch) || can_talk(ch) )
  {
    // Include shapechanged, switched, etc.
    if( ch->desc )
    {
      // NPCs always see their guild chatter (in case of switch, etc).
      if( IS_NPC(ch) || IS_SET(ch->specials.act, PLR_ECHO) )
      {
        sprintf(Gbuf1, "&+cYou tell your guild '&+C%s&n&+c'\r\n", argument);
        send_to_char(Gbuf1, ch, LOG_PRIVATE);
      }
      else
      {
        send_to_char("Ok.\r\n", ch);
      }

      if( get_property("logs.chat.status", 0.000) && IS_PC(ch) )
      {
        logit(LOG_CHAT, "%s gcc's '%s'", GET_NAME(ch), argument);
      }
    }

    sprintf(Gbuf1, "%sasc.%u", ASC_DIR, from_guild);
    if( (f = fopen(Gbuf1, "r")) )
    {
      fgets(guild_name, MAX_STR_NORMAL, f);
      fclose(f);
      sprintf(guild_name + strlen(guild_name) - 1, "&n");
    }
    else
    {
      sprintf( guild_name, "Unknown Guild" );
    }

    for( i = descriptor_list; i; i = i->next )
    {
      if( !(to_ch = i->character) )
      {
        continue;
      }
      if( IS_NPC(to_ch) )
      {
        logit( LOG_DEBUG, "do_gcc: Character (%s) is on descriptor_list but is a NPC!", J_NAME(to_ch) );
        continue;
      }

      // Skip ch, ppl at menu/char creation/etc, and deaf ppl.
      if( to_ch == ch || i->connected != CON_PLYNG || is_silent( to_ch, FALSE )
        || IS_AFFECTED4(i->character, AFF4_DEAF) )
      {
        continue;
      }

      // Dereference once for ease and speed.
      to_guild = GET_A_NUM(to_ch);
      if( IS_TRUSTED(to_ch) )
      {
        // If they'r governing a diff't association or they have GCC toggled off, or ignoring ch.
        if( (to_guild && to_guild != from_guild) || !PLR_FLAGGED(to_ch, PLR_GCC)
          || to_ch->only.pc->ignored == ch )
        {
          continue;
        }
      }
      // Mortals need GCC on, and must be a guilded in the same guild, must be a member, not on parole.
      //   And can't be ignoring ch.
      else if( !PLR_FLAGGED(to_ch, PLR_GCC) || to_guild != from_guild
        || !IS_MEMBER(GET_A_BITS(i->character)) || !GT_PAROLE(GET_A_BITS(to_ch))
        || to_ch->only.pc->ignored == ch )
      {
        continue;
      }
      sprintf(Gbuf1, "&+c%s&n&+c tells %s '&+C%s&n&+c'\r\n", PERS(ch, to_ch, FALSE),
        guild_name, language_CRYPT(ch, to_ch, argument));
      send_to_char(Gbuf1, to_ch, LOG_PRIVATE);
    }
  }
}

void send_to_guild(int asc, char *name, char *arg)
{
  P_desc i;
  char Gbuf1[MAX_STRING_LENGTH];

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected &&
        !is_silent(i->character, FALSE) &&
        IS_SET(i->character->specials.act, PLR_GCC) &&
        IS_MEMBER(GET_A_BITS(i->character)) &&
	(GET_A_NUM(i->character) == asc) &&
        (!(IS_AFFECTED4(i->character, AFF4_DEAF))) &&
        (GT_PAROLE(GET_A_BITS(i->character))))
    {
      sprintf(Gbuf1, "&+c%s&n&+c tells your guild '&+C%s&n&+c'\r\n", name, arg);
      send_to_char(Gbuf1, i->character, LOG_PRIVATE);
    }
}

void do_rwc(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (!ch)
    return;

  send_to_char("This channel has been removed.\r\n", ch);
  return;

  if (IS_NPC(ch))
  {
    send_to_char("You can't use the RWC channel!\r\n", ch);
    return;
  }
  else if (IS_SET(ch->specials.act, PLR_SILENCE) ||
           !IS_SET(ch->specials.act2, PLR2_RWC) || is_silent(ch, TRUE))
  {
    send_to_char("You can't use the RWC channel!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You can't speak in this form!\r\n", ch);
    return;
  }
  if (IS_AFFECTED2(ch, AFF2_SILENCED))
  {
    send_to_char
      ("If you can't 'say', 'shout', or 'emote', what makes you think you can 'gcc'?\r\n",
       ch);
    return;
  }
  while (*argument == ' ' && *argument != '\0')
    argument++;

  if (!*argument)
    send_to_char("RWC? Yes! Fine! Chat we must, but WHAT??\r\n", ch);
  else if (!is_silent(ch, TRUE) && (IS_NPC(ch) || can_talk(ch)))
  {
    if (ch->desc)
    {
      if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
      {
        sprintf(Gbuf1, "&+mYou RWC '&+M%s&n&+m'\r\n&N", argument);
        send_to_char(Gbuf1, ch);
      }
      else
        send_to_char("Ok.\r\n", ch);
    }
    for (i = descriptor_list; i; i = i->next)
      if ((i->character != ch) && !i->connected &&
          !is_silent(i->character, FALSE) &&
          IS_SET(i->character->specials.act2, PLR2_RWC))
      {
        sprintf(Gbuf1, "&+m(RWC) %s - '&+M%s&n&+m'\r\n&N",
                GET_NAME(ch), argument);
        send_to_char(Gbuf1, i->character);
      }
  }
}

void do_project(P_char ch, char *argument, int cmd)
{
  P_desc desc;
  char   Gbuf1[MAX_STRING_LENGTH];

  if( IS_NPC(ch) )
  {
    if( !IS_ILLITHID(ch) && !IS_PILLITHID(ch) )
    {
      send_to_char("You're not an illithid ..\r\n", ch);
      return;
    }
  }
  else if( IS_SET(ch->specials.act, PLR_SILENCE) )
  {
    send_to_char("Alas, the gods have taken away your ability to transmit..\r\n", ch);
    return;
  }
  if( !IS_TRUSTED(ch) && !IS_ILLITHID(ch) && !IS_PILLITHID(ch) )
  {
    send_to_char("Might not hurt if you knew _HOW_...\r\n", ch);
    return;
  }
  if (GET_LEVEL(ch) < 5)
  {
    send_to_char("You do not yet have enough control over your thoughts.\r\n", ch);
    return;
  }
  if( !IS_SET(ch->specials.act2, PLR2_PROJECT) )
  {
    send_to_char("You do not feel close enough to the Elder Brain to communicate in this fashion...\r\n", ch);
    return;
  }

  while (*argument == ' ' && *argument != '\0')
    argument++;

  if (!*argument)
  {
    send_to_char("In order to be telepathic, you must have something to say..\r\n", ch);
    return;
  }
  else if (ch->desc)
  {
    sprintf(Gbuf1, "&+mYou project '&+M%s&n&+m' across the ether.&n\n", argument);

    if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
    {
      send_to_char(Gbuf1, ch);
    }
    else
      send_to_char("Ok.\r\n", ch);

    write_to_pc_log(ch, Gbuf1, LOG_PRIVATE);

    if( get_property("logs.chat.status", 0.000) && IS_PC(ch) )
      logit(LOG_CHAT, "%s projects '%s'", GET_NAME(ch), argument);
  }

  sprintf(Gbuf1, "&+m%s&+m projects '&+M%s&n&+m' accross the ether.&n\n", GET_NAME(ch), argument);
  for( desc = descriptor_list; desc; desc = desc->next )
  {
    if( (desc->character != ch) && !desc->connected
      && ((GET_RACE(desc->character) == RACE_ILLITHID) || IS_PILLITHID(desc->character)
      || IS_TRUSTED(desc->character)) && (IS_SET(desc->character->specials.act2, PLR2_PROJECT)) )
    {
      act(Gbuf1, 0, ch, 0, desc->character, TO_VICT | ACT_IGNORE_ZCOORD);
      write_to_pc_log(desc->character, Gbuf1, LOG_PRIVATE);
    }
  }
}

void do_page(P_char ch, char *argument, int cmd)
{
  char    *buf;

  if (GET_LEVEL(ch) < GREATER_G)
  {
    send_to_char("No, dammit, you can't!\r\n", ch);
    return;
  }
  send_to_char("Re-reading news file...\r\n", ch);
  logit(LOG_STATUS, "re-reading newsfile.");
  news = get_mud_info("news");

  send_to_char("Re-reading motd file...\r\n", ch);
  logit(LOG_STATUS, "re-reading motd.");
  motd = get_mud_info("motd");

  send_to_char("Re-reading wiz motd file...\r\n", ch);
  logit(LOG_STATUS, "re-reading wizmotd.");
  wizmotd = get_mud_info("wizmotd");

  send_to_char("Re-reading credits file...\r\n", ch);
  logit(LOG_STATUS, "re-eading credits.");
  buf = file_to_string(CREDITS_FILE);
  if (buf)
  {
    FREE(credits);
    credits = buf;
  }
  else
    send_to_char("&+R&-LFAILED!\r\n", ch);

  send_to_char("&+WDone.&N\r\n", ch);
}

void do_shout(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  char     Gbuf1[MAX_STRING_LENGTH];

  if ((IS_PC(ch) && IS_SET(ch->specials.act, PLR_SILENCE)) ||
      is_silent(ch, TRUE))
  {
    send_to_char("You can't shout!\r\n", ch);
    return;
  }
  if (IS_PC(ch) && IS_SET(ch->specials.act, PLR_NOSHOUT))
  {
    send_to_char("You are not tuned into the 'shout' channel??\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You cannot speak in that form!\r\n", ch);
    return;
  }
  if ((IS_SET(world[ch->in_room].room_flags, UNDERWATER) ||
       ch->specials.z_cord < 0) && !IS_TRUSTED(ch) && !IS_NPC(ch))
  {
    send_to_char
      ("You shout at the top of your lungs and inhale some water!\r\n", ch);
    act("$n mouth opens wide and huge bubbles come forth...", TRUE, ch, 0, 0,
        TO_ROOM);
    return;
  }
  for (; *argument == ' '; argument++) ;

  if (!(*argument))
    send_to_char("Shout? Yes! Fine! Shout we must, but WHAT??\r\n", ch);
  else if (!is_silent(ch, TRUE) && can_talk(ch))
  {
    if (ch->desc)
    {
      if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
      {
        sprintf(Gbuf1, "&+cYou shout across the world '%s'\r\n", argument);
        send_to_char(Gbuf1, ch);
      }
      else
        send_to_char("Ok.\r\n", ch);
      
      if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
        logit(LOG_CHAT, "%s gshout's '%s'", GET_NAME(ch), argument);
    }
    for (i = descriptor_list; i; i = i->next)
      if ((i->character != ch) && !i->connected &&
          !is_silent(i->character, FALSE) &&
          !IS_SET(i->character->specials.act, PLR_NOSHOUT))
      {
        if (IS_TRUSTED(ch))
          sprintf(Gbuf1, "&+c$n shouts from the heavens %s'%s'&N",
                  language_known(ch, i->character),
                  language_CRYPT(ch, i->character, argument));
        else
          sprintf(Gbuf1, "&+c$n shouts across the world %s'%s'&N",
                  language_known(ch, i->character),
                  language_CRYPT(ch, i->character, argument));
        act(Gbuf1, 0, ch, 0, i->character, TO_VICT);
      }
  }
  if (ch->desc)
    ch->desc->last_input[0] = '\0';
}

void do_reply(P_char ch, char *argument, int cmd)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
    return;

  if (!ch->only.pc->last_tell)
  {
    send_to_char("You don't have anyone to whom to reply!\r\n", ch);
    return;
  }

  sprintf(Gbuf1, "%s %s", ch->only.pc->last_tell, argument);
  do_tell(ch, Gbuf1, cmd);
  return;
}

void do_tell(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  P_desc   d;
  char     name[MAX_INPUT_LENGTH], message[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];

  half_chop(argument, name, message);

  if (IS_NPC(ch) && (IS_AFFECTED(ch, AFF_CHARM) || IS_MORPH(ch)))
  {
    send_to_char("If only you could tell, you would..\r\n", ch);
    return;
  }

  if (IS_SET(ch->specials.act, PLR_NOTELL) && (!IS_TRUSTED(ch)))
  {
    send_to_char
      ("You can't tell to someone if you have tell toggled off!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You cannot speak in that form!\r\n", ch);
    return;
  }
  if (!*name || !*message)
  {
    send_to_char("Who do you wish to tell what??\r\n", ch);
    return;
  }
  if ((IS_SET(world[ch->in_room].room_flags, UNDERWATER) ||
       ch->specials.z_cord < 0) && !IS_TRUSTED(ch))
  {
    send_to_char("Your species doesn't know how to communicate underwater...",
                 ch);
    return;
  }
  if (IS_AFFECTED2(ch, AFF2_SILENCED))
  {
    send_to_char
      ("You can't seem to communicate much of anything right now.\r\n", ch);
    return;
  }
  if (is_silent(ch, TRUE) || !can_talk(ch))
    return;

  vict = NULL;
  /*
   * switching to descriptor list, rather than get_char_vis, since it
   * was lagging hell out of things. JAB
   */
  for (d = descriptor_list; d; d = d->next)
  {
    if (!d->character || d->connected || !d->character->player.name)
      continue;
    if (!isname(d->character->player.name, name))
      continue;
    if (!CAN_SEE_Z_CORD(ch, d->character))
      continue;
    vict = d->character;
    break;
  }

  if (vict && !IS_TRUSTED(ch) && !IS_TRUSTED(vict))
    if (racewar(ch, vict) && !IS_DISGUISE(vict) ||
        (IS_DISGUISE(vict) && (EVIL_RACE(ch) != EVIL_RACE(vict))) ||
        (GET_RACE(ch) == RACE_ILLITHID && !IS_ILLITHID(vict)))
      vict = NULL;

  if (!vict)
    send_to_char("No-one by that name here...\r\n", ch);
  else if (ch == vict)
    send_to_char("You try to tell yourself something.\r\n", ch);
  else if (!vict->desc || (IS_AFFECTED4(vict, AFF4_DEAF)))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR);
  else if (((IS_SET(world[vict->in_room].room_flags, UNDERWATER)) ||
            vict->specials.z_cord < 0) && !IS_TRUSTED(vict))
  {
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  else if (!IS_TRUSTED(ch) &&
           (is_silent(vict, FALSE) || (GET_STAT(vict) < STAT_RESTING) ||
            (IS_SET(vict->specials.act, PLR_NOTELL) &&
             !is_linked_from(vict, ch, LNK_CONSENT))))
  {
    act("No-one by that name here...", FALSE, ch, 0, vict, TO_CHAR);
    if (IS_NPC(ch) && IS_SET(vict->specials.act, PLR_NOTELL))
      act
        ("$n attempted to tell you something, but you're not paying attention!",
         FALSE, ch, 0, vict, TO_VICT);
  }
  else if (!IS_TRUSTED(ch) && (GET_STAT(ch) < STAT_RESTING))
    send_to_char("You can't do that right now...\r\n", ch);
  else if ((vict->only.pc->ignored == ch) && (!IS_TRUSTED(ch)))
  {
    act("$N is ignoring you at the moment.  No dice.", FALSE, ch, 0, vict,
        TO_CHAR);
  }
  else
  {
/*
    if ((GET_MANA (ch) < 5) && !IS_TRUSTED (ch) && !IS_TRUSTED (vict))
    {
      send_to_char ("You do not have enough mana.\r\n", ch);
      return;
    }

    if (!IS_TRUSTED (ch) && !IS_TRUSTED (vict)) GET_MANA (ch) -= 5;
    if (GET_MANA (ch) < GET_MAX_MANA(ch))
     StartRegen(ch, EVENT_MANA_REGEN);
*/

    if (ch->desc)
    {
      if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
      {
        sprintf(Gbuf1, "&+WYou tell %s %s'%s'\r\n", GET_NAME(vict),
                language_known(ch, ch), message);
        send_to_char(Gbuf1, ch, LOG_PRIVATE);
      }
      else
      {
        send_to_char("Ok.\r\n", ch);
      }
      if (IS_SET(vict->specials.act, PLR_AFK))
        act("$E is away from $S keyboard right now..", FALSE, ch, 0, vict,
            TO_CHAR);
      if (!CAN_SEE(vict, ch))
	act("&+L$E cannot see you..&n", FALSE, ch, 0, vict, TO_CHAR);

      if (get_property("logs.chat.status", 0.000) && IS_PC(ch) && IS_PC(vict))
        logit(LOG_CHAT, "%s tells %s '%s'", GET_NAME(ch), GET_NAME(vict), message);
    }


    sprintf(Gbuf1, "&+W%s&+W tells you %s'%s'&N\r\n",
            ((CAN_SEE(vict, ch) || racewar(vict, ch)) ?
             (IS_PC(ch) ? (ch)->player.name : (ch)->player.
              short_descr) : "Someone"), language_known(ch, vict),
            language_CRYPT(ch, vict, message));
    send_to_char(Gbuf1, vict, LOG_PRIVATE);

    if (IS_SET(vict->specials.act, PLR_AFK))
      act("$n sent you a tell, and your &+RAFK&N is toggled on!", FALSE, ch,
          0, vict, TO_VICT);



    if (IS_PC(vict))
      vict->only.pc->last_tell = ch->player.name;

/*
 * sprintf(Gbuf1, "&+W$n tells you %s'%s'&N", language_known(ch, vict),
 * language_CRYPT(ch, vict, message)); act(Gbuf1, 0, ch, 0, vict,
 * TO_VICT);
 */
  }
}

void do_whisper(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  char     name[MAX_INPUT_LENGTH], message[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH], dispname[MAX_STRING_LENGTH];

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char("Who do you want to whisper to.. and what??\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, name)))
    send_to_char("No-one by that name here..\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You cannot speak in that form!\r\n", ch);
    return;
  }
  else if ((is_silent(ch, TRUE)) || (!can_talk(ch) && !IS_MORPH(ch)))
  {
    return;
  }
  else if ((IS_SET(world[ch->in_room].room_flags, UNDERWATER) ||
            ch->specials.z_cord < 0) && !IS_TRUSTED(ch) && !IS_NPC(ch))
  {
    send_to_char
      ("That would be too difficult considering the circumstances...", ch);
    return;
  }
  else if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM) && !IS_MORPH(ch))
  {
    send_to_char("If only..\r\n", ch);
    return;
  }
  else if (IS_AFFECTED4(vict, AFF4_DEAF))
  {
    act("$n whispers you something but you can't hear what!", FALSE, vict, 0,
        ch, TO_VICT);
  }
  else if (vict == ch && !IS_ILLITHID(ch) && !IS_PILLITHID(ch))
  {
    act("$n whispers quietly to $mself.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("You can't get your mouth close enough to your ear...\r\n",
                 ch);
  }
  else
  {
    if (IS_NPC(vict))
      strcpy(dispname, vict->player.short_descr);
    else if ((racewar(ch, vict) && !IS_ILLITHID(ch)) || !is_introd(vict, ch))
      strcpy(dispname, race_names_table[GET_RACE(vict)].ansi);
    else
      strcpy(dispname, GET_NAME(vict));

    if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
    {
      if (IS_ILLITHID(ch) || IS_PILLITHID(ch))
        sprintf(Gbuf1, "You softly project '%s' to %s\r\n", message,
                dispname);
      else if (IS_NPC(ch) && !(GET_CLASS(vict, CLASS_DRUID)))
      {
        send_to_char("They wouldn't understand you anyway!\r\n", ch);
        return;
      }
      sprintf(Gbuf1, "You whisper '%s' to %s\r\n", message, dispname);
      send_to_char(Gbuf1, ch);  
    }
    else
      send_to_char("Ok.\r\n", ch);
    
    if (get_property("logs.chat.status", 0.000) && IS_PC(ch) && IS_PC(vict))
      logit(LOG_CHAT, "%s whispers to %s '%s'", GET_NAME(ch), GET_NAME(vict), message);
    if (IS_ILLITHID(ch) || IS_PILLITHID(ch))
    {
      sprintf(Gbuf1, "A soft voice in your head whispers '%s'", message);
      act(Gbuf1, FALSE, ch, 0, vict, TO_VICT);
    }
    else
    {
      if (IS_NPC(ch) && !(GET_CLASS(vict, CLASS_DRUID)))
      {
        send_to_char("They wouldn't understand you!\r\n", ch);
        return;
      }
      sprintf(Gbuf1, "$n whispers to you, %s'%s'", language_known(ch, vict),
              language_CRYPT(ch, vict, message));
      act(Gbuf1, FALSE, ch, 0, vict, TO_VICT);
      act("$n whispers something to $N.", FALSE, ch, 0, vict, TO_NOTVICT);
      nq_action_check(ch, vict, message);
    }
  }
}

void do_ask(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  char     name[MAX_INPUT_LENGTH], message[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];

  half_chop(argument, name, message);

  if (!*name || !*message)
  {
    send_to_char("Who do you want to ask something... and what??\r\n", ch);
    return;
  }
  else if (!(vict = get_char_room_vis(ch, name)))
  {
    send_to_char("No-one by that name here...\r\n", ch);
    return;
  }
  else if ((IS_SET(world[ch->in_room].room_flags, UNDERWATER) ||
            ch->specials.z_cord < 0) && !IS_TRUSTED(ch) && !IS_NPC(ch))
  {
    send_to_char("Too difficult to do here...", ch);
    return;
  }
  else if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You cannot speak in that form!\r\n", ch);
    return;
  }
  else if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
  {
    send_to_char("You're too charmed to ask anyone anything..\r\n", ch);
    return;
  }
  else if (vict == ch && !IS_ILLITHID(ch) && !IS_PILLITHID(ch))
  {
    act("$n quietly asks $mself a question.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("You think about it for a while...\r\n", ch);
  }
  else if (!is_silent(ch, TRUE) && can_talk(ch))
  {
    if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
    {
/*
      if (IS_NPC(vict))
        sprintf(Gbuf1, "You ask %s '%s'\r\n", vict->player.short_descr, message);
      else
        sprintf(Gbuf1, "You ask %s '%s'\r\n",
                (racewar(ch, vict) || is_introd(ch,vict)) ? race_names_table[GET_RACE(vict)].ansi :
                 GET_NAME(vict), message);
      send_to_char(Gbuf1, ch);
*/
      sprintf(Gbuf1, "You ask $N '%s'", message);
      act(Gbuf1, FALSE, ch, 0, vict, TO_CHAR);
    }
    else
      send_to_char("Ok.\r\n", ch);
      
    if (get_property("logs.chat.status", 0.000) && IS_PC(ch) && IS_PC(vict))
      logit(LOG_CHAT, "%s asks %s '%s'", GET_NAME(ch), GET_NAME(vict), message);
    
    if (IS_ILLITHID(ch) || IS_PILLITHID(ch))
    {
      sprintf(Gbuf1, "A voice in your head asks '%s'", message);
      act(Gbuf1, FALSE, ch, 0, vict, TO_VICT);
    }
    else
    {
      sprintf(Gbuf1, "$n asks you %s'%s'", language_known(ch, vict),
              language_CRYPT(ch, vict, message));
      act(Gbuf1, FALSE, ch, 0, vict, TO_VICT | ACT_SILENCEABLE);
      act("$n asks $N a question.", FALSE, ch, 0, vict, TO_NOTVICT);
    }
    
	quest_ask(ch, vict);
	nq_action_check(ch, vict, message);
  }
}

#define MAX_NOTE_LENGTH 1000    /* arbitrary */

void do_write(P_char ch, char *argument, int cmd)
{
  P_obj    paper = 0, pen = 0;
  char     papername[MAX_INPUT_LENGTH], penname[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];

  argument_interpreter(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername)
  {                             /* nothing was delivered */
    send_to_char("Write? with what? ON what? what are you trying to do??\r\n",
                 ch);
    return;
  }
  if (*penname)
  {                             /* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
    {
      sprintf(Gbuf1, "You have no %s.\r\n", papername);
      send_to_char(Gbuf1, ch);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying)))
    {
      sprintf(Gbuf1, "You have no %s.\r\n", papername);
      send_to_char(Gbuf1, ch);
      return;
    }
  }
  else
  {                             /* there was one arg.let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
    {
      sprintf(Gbuf1, "There is no %s in your inventory.\r\n", papername);
      send_to_char(Gbuf1, ch);
      return;
    }
    if (paper->type == ITEM_PEN)
    {                           /* oops, a pen..  */
      pen = paper;
      paper = 0;
    }
    else if (paper->type != ITEM_NOTE)
    {
      send_to_char("That thing has nothing to do with writing.\r\n", ch);
      return;
    }
    /* one object was found. Now for the other one.  */
    if (!ch->equipment[HOLD])
    {
      sprintf(Gbuf1, "You can't write with a %s alone.\r\n", papername);
      send_to_char(Gbuf1, ch);
      return;
    }
    if (!CAN_SEE_OBJ(ch, ch->equipment[HOLD]))
    {
      send_to_char("The stuff in your hand is invisible! Yeech!!\r\n", ch);
      return;
    }
    if (pen)
      paper = ch->equipment[HOLD];
    else
      pen = ch->equipment[HOLD];
  }

  /* ok.. now let's see what kind of stuff we've found */
  if (pen->type != ITEM_PEN)
  {
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  }
  else if (paper->type != ITEM_NOTE)
  {
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  }
  else
  {
    /* we can write - hooray! */
    /* this is the PERFECT code example of how to set up:
     * a) the text editor with a message already loaed
     * b) the abort buffer if the player aborts the message
     */
    ch->desc->backstr = NULL;
    send_to_char("Write your note.  (/s saves /h for help)\r\n", ch);
    /* ok, here we check for a message ALREADY on the paper */
    if (paper->action_description)
    {
      /* we str_dup the original text to the descriptors->backstr */
      ch->desc->backstr = str_dup(paper->action_description);
      /* send to the player what was on the paper (cause this is already */
      /* loaded into the editor) */
      send_to_char(paper->action_description, ch);
    }
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    /* assign the descriptor's->str the value of the pointer to the text */
    /* pointer so that we can reallocate as needed (hopefully that made */
    /* sense :>) */
    paper->str_mask |= STRUNG_DESC3;
    ch->desc->str = &paper->action_description;
    ch->desc->max_str = MAX_NOTE_LENGTH;
  }
}

#undef MAX_NOTE_LENGTH

char    *fread_action(FILE * fl)
{
  /*
   * definately leave this, fread_action is touchy enough
   */
  char     buf[MAX_STRING_LENGTH], *rslt;

  for (;;)
  {
    fgets(buf, MAX_STRING_LENGTH, fl);
    if (feof(fl))
    {
      logit(LOG_EXIT, "Fread_action - unexpected EOF.");
      raise(SIGSEGV);
    }
    if (*buf == '#')
      return (0);
    {
      *(buf + strlen(buf) - 1) = '\0';
      rslt = str_dup(buf);
      return (rslt);
    }
  }
}

void boot_social_messages(void)
{
  FILE    *fl;
  int      tmp, hide, min_pos;

  if (!(fl = fopen(SOCMESS_FILE, "r")))
  {
    logit(LOG_EXIT, "boot_social_messages");
    raise(SIGSEGV);
  }
  for (;;)
  {
    tmp = -1;
    fscanf(fl, " %d ", &tmp);
    if (tmp < 0)
      break;
    fscanf(fl, " %d ", &hide);
    fscanf(fl, " %d \n", &min_pos);

    /*
     * Gond == lazy, rather than change the poses file (have to
     * eventually) xlate the old min_pos to new one (roughly).
     */
    switch (min_pos)
    {
    case 0:                    /*
                                 * was POSITION_DEAD
                                 */
      min_pos = STAT_DEAD + POS_PRONE;
      break;
    case 1:                    /*
                                 * was POSITION_MORTALLYW
                                 */
      min_pos = STAT_DYING + POS_PRONE;
      break;
    case 2:                    /*
                                 * was POSITION_INCAP
                                 */
      min_pos = STAT_INCAP + POS_PRONE;
      break;
    case 3:                    /*
                                 * was POSITION_STUNNED
                                 */
      min_pos = STAT_SLEEPING + POS_PRONE;
      break;
    case 4:                    /*
                                 * was POSITION_SLEEPING
                                 */
      min_pos = STAT_SLEEPING + POS_PRONE;
      break;
    case 5:                    /*
                                 * was POSITION_RESTING
                                 */
      min_pos = STAT_RESTING + POS_KNEELING;
      break;
    case 6:                    /*
                                 * was POSITION_SITTING
                                 */
      min_pos = STAT_RESTING + POS_SITTING;
      break;
    case 7:                    /*
                                 * was POSITION_FIGHTING
                                 */
    case 8:                    /*
                                 * was POSITION_STANDING
                                 */
      min_pos = STAT_NORMAL + POS_STANDING;
      break;
    default:
      min_pos = STAT_DEAD + POS_PRONE;
      break;
    }

    /*
     * alloc a new cell
     */
    if( !soc_mess_list )
    {
      CREATE(soc_mess_list, social_messg, 1, MEM_TAG_SOCMSG);
      list_top = 0;
    }
    else
    {
      RECREATE(soc_mess_list, social_messg, (++list_top + 1));
    }
    /*
     * read the stuff
     */
    soc_mess_list[list_top].act_nr = tmp;
    soc_mess_list[list_top].hide = hide;
    soc_mess_list[list_top].min_victim_position = min_pos;

    soc_mess_list[list_top].char_no_arg = fread_action(fl);
    soc_mess_list[list_top].others_no_arg = fread_action(fl);

    soc_mess_list[list_top].char_found = fread_action(fl);

    /*
     * if no char_found, the rest is to be ignored
     */
    if (!soc_mess_list[list_top].char_found)
      continue;

    soc_mess_list[list_top].others_found = fread_action(fl);
    soc_mess_list[list_top].vict_found = fread_action(fl);

    soc_mess_list[list_top].not_found = fread_action(fl);

    soc_mess_list[list_top].char_auto = fread_action(fl);

    soc_mess_list[list_top].others_auto = fread_action(fl);
  }
  fclose(fl);
}

// Standard log n hunt for cmd.
int find_action( int cmd )
{
  int      bot, top, mid;

  bot = 0;
  top = list_top;

  if( top < 0 )
    return -1;

  for (;;)
  {
    mid = (bot + top) >> 1;

    if( soc_mess_list[mid].act_nr == cmd )
    {
      return mid;
    }
    if( bot >= top )
    {
      return -1;
    }
    if( soc_mess_list[mid].act_nr > cmd )
    {
      top = --mid;
    }
    else
    {
      bot = ++mid;
    }
  }
}

#if 1                           /*
                                 * old do_action, new version (Brett?) *
                                 keeps outputting wierd garbage. JAB
                                 */

void do_action(P_char ch, char *argument, int cmd)
{
  int      act_nr;

  /*
   * leave tmp as well, do_action called from lots of places too.
   */
  char     buf[MAX_INPUT_LENGTH];
  struct social_messg *act_mesg;
  P_char   vict;

  /*** First section will intercept commands, and reroute them if needed ***/
  if(cmd == CMD_ROAR)
  {
	  if(do_roar_of_heroes(ch))
    {
	    return;
    }
  }
  if (cmd == CMD_ROAR && IS_DRAGON(ch))
  {
    DragonCombat(ch, TRUE);     /* calls roar section only */
    if( IS_PC_PET(ch) )
    {
      CharWait(ch, PULSE_VIOLENCE);
    }
    return;
  }
  if( cmd == CMD_ROAR && (IS_AVATAR(ch) || IS_TITAN(ch)) )
  {
    DragonCombat(ch, TRUE);
    if (IS_PC_PET(ch))
    {
      CharWait(ch, PULSE_VIOLENCE);
    }
    return;
  }
  if( cmd == CMD_TRIP )
  {
    do_trip(ch, argument, cmd);
    return;
  }
  /*** true social action follows ***/
  if( (act_nr = find_action(cmd)) < 0 )
  {
    send_to_char("That action is not supported.\r\n", ch);
    return;
  }
  act_mesg = &soc_mess_list[act_nr];

  if( argument && act_mesg->char_found )
  {
    one_argument(argument, buf);
  }
  else
  {
    *buf = '\0';
  }

  if (!*buf)
  {
/*    send_to_char(act_mesg->char_no_arg, ch);
    send_to_char("\r\n", ch);*/
    act(act_mesg->char_no_arg, FALSE, ch, 0, 0, TO_CHAR);
    act(act_mesg->others_no_arg, act_mesg->hide, ch, 0, 0, TO_ROOM);
    return;
  }
  if (!(vict = get_char_room_vis(ch, buf)))
  {
//    send_to_char(act_mesg->not_found, ch);
//    send_to_char("\r\n", ch);
    act(act_mesg->not_found, FALSE, ch, 0, 0, TO_CHAR);
  }
  else if (vict == ch)
  {
//    send_to_char(act_mesg->char_auto, ch);
//    send_to_char("\r\n", ch);
    act(act_mesg->char_auto, FALSE, ch, 0, 0, TO_CHAR);
    act(act_mesg->others_auto, act_mesg->hide, ch, 0, 0, TO_ROOM);
#   if 0
  }
  else if (!(obj = get_obj_room_vis(ch, buf)))
  {
    for (ch_ptr = buf; *ch_ptr != '\0'; ch_ptr++)
    {
      if (*ch_ptr == '$')
        switch (*(ch_ptr + 1))
        {
        case '+':
        case '-':
        case '=':
          send_to_char("Pardon? No ansi chars allowed as input.\r\n", ch);
          break;
        }
    }
#   endif
  }
  else
  {
    if (!MIN_POS(vict, act_mesg->min_victim_position))
    {
      act("$N is not in a proper position for that.", FALSE, ch, 0, vict,
          TO_CHAR);
    }
    else
    {
      act(act_mesg->char_found, 0, ch, 0, vict, TO_CHAR);
      act(act_mesg->others_found, act_mesg->hide, ch, 0, vict, TO_NOTVICT);
      act(act_mesg->vict_found, act_mesg->hide, ch, 0, vict, TO_VICT);
    }
  }
//  if (GET_RACE(ch) == RACE_HALFLING)
//    halfling_stealaction(ch, argument, cmd);
}

#else /*
       * new version below, but buggy
       */

void do_action(P_char ch, char *argument, int cmd)
{
  int      act_nr;

  /*
   * leave tmp as well, do_action called from lots of places too.
   */
  char     buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  char     TmpBuf[MAX_INPUT_LENGTH], a_buf[MAX_INPUT_LENGTH];
  struct social_messg *act_mesg;
  P_char   vict;

  if ((act_nr = find_action(cmd)) < 0)
  {
    send_to_char("That action is not supported.\r\n", ch);
    return;
  }
  act_mesg = &soc_mess_list[act_nr];

  if (act_mesg->char_found)
  {
    /*
     * one_argument(a_buf, buf1);
     */
    /*
     * argument_interpreter(a_buf, buf1, buf2);
     */
    /*
     * this a_buf dealie allows for argument being a constant, like
     * do_action(ch "maid", CMD_PINCH); but all these strcpy's will
     * add to cpu load, hopefully not much.  JAB
     */
    if (argument && *argument)
      strcpy(a_buf, argument);
    else
      a_buf[0] = 0;
    half_chop(a_buf, buf1, buf2);
  }
  else
    *buf1 = '\0';

  if (!*buf1)
  {
    send_to_char(act_mesg->char_no_arg, ch);
    send_to_char("\r\n", ch);
    act(act_mesg->others_no_arg, act_mesg->hide, ch, 0, 0, TO_ROOM);
    return;
  }
  if (!(vict = get_char_room_vis(ch, buf1)))
  {
/*
 * send_to_char(act_mesg->not_found, ch);
 */
    send_to_char(act_mesg->char_no_arg, ch);
    send_to_char("\r\n", ch);
    *TmpBuf = '\0';
    strcpy(TmpBuf, act_mesg->others_no_arg);
    strcat(TmpBuf, " '");
    CAP(a_buf);
    strcat(TmpBuf, a_buf);
    strcat(TmpBuf, "'");
    act(TmpBuf, act_mesg->hide, ch, 0, 0, TO_ROOM);
  }
  else if (vict == ch)
  {
    send_to_char(act_mesg->char_auto, ch);
    send_to_char("\r\n", ch);
    *TmpBuf = '\0';
    strcpy(TmpBuf, act_mesg->char_found);
    if (*buf2)
    {
      strcat(TmpBuf, " '");
      CAP(buf2);
      strcat(TmpBuf, buf2);
      strcat(TmpBuf, "'");
    }
    act(TmpBuf, act_mesg->hide, ch, 0, 0, TO_ROOM);
    /*
     * act(act_mesg->others_auto, act_mesg->hide, ch, 0, 0, TO_ROOM);
     */
  }
  else
  {
    if (!MIN_POS(vict, act_mesg->min_victim_position))
    {
      act("$N is not in a proper position for that.", FALSE, ch, 0, vict,
          TO_CHAR);
    }
    else
    {
      *TmpBuf = '\0';
      strcpy(TmpBuf, act_mesg->char_found);
      if (*buf2)
      {
        strcat(TmpBuf, " '");
        CAP(buf2);
        strcat(TmpBuf, buf2);
        strcat(TmpBuf, "'");
      }
      /*
       * act(act_mesg->char_found, 0, ch, 0, vict, TO_CHAR);
       */
      act(TmpBuf, 0, ch, 0, vict, TO_CHAR);
      *TmpBuf = '\0';
      strcpy(TmpBuf, act_mesg->others_found);
      if (*buf2)
      {
        strcat(TmpBuf, " '");
        CAP(buf2);
        strcat(TmpBuf, buf2);
        strcat(TmpBuf, "'");
      }
      /*
       * act(act_mesg->others_found, act_mesg->hide, ch, 0, vict,
       * TO_NOTVICT);
       */
      act(TmpBuf, act_mesg->hide, ch, 0, vict, TO_NOTVICT);
      *TmpBuf = '\0';
      strcpy(TmpBuf, act_mesg->vict_found);
      if (*buf2)
      {
        strcat(TmpBuf, " '");
        CAP(buf2);
        strcat(TmpBuf, buf2);
        strcat(TmpBuf, "'");
      }
      /*
       * act(act_mesg->vict_found, act_mesg->hide, ch, 0, vict,
       * TO_VICT);
       */
      act(TmpBuf, act_mesg->hide, ch, 0, vict, TO_VICT);
    }
  }
}
#endif

void do_insult(P_char ch, char *argument, int cmd)
{
/*  static char buf[100];*/
  P_char   victim;
  char     Gbuf1[MAX_STRING_LENGTH];

  one_argument(argument, Gbuf1);

  if (*Gbuf1)
  {
    if (!(victim = get_char_room_vis(ch, Gbuf1)))
    {
      send_to_char("Can't hear you!\r\n", ch);
    }
    else
    {
      if (victim != ch)
      {
/*        sprintf(buf, "You insult %s.\r\n", GET_NAME(victim));
        send_to_char(buf, ch);*/
        act("You insult $N.", FALSE, ch, 0, victim, TO_CHAR);

        switch (number(0, 2))
        {
        case 0:
          {
            if (GET_SEX(ch) == SEX_MALE)
            {
              if (GET_SEX(victim) == SEX_MALE)
                act("$n accuses you of fighting like a woman!", FALSE,
                    ch, 0, victim, TO_VICT);
              else
                act("$n says that women can't fight.",
                    FALSE, ch, 0, victim, TO_VICT);
            }
            else
            {                   /*
                                 * Ch == Woman
                                 */
              if (GET_SEX(victim) == SEX_MALE)
                act("$n accuses you of having the smallest.... (brain?)",
                    FALSE, ch, 0, victim, TO_VICT);
              else
                act
                  ("$n tells you that you'd lose a beauty contest against a troll.",
                   FALSE, ch, 0, victim, TO_VICT);
            }
          }
          break;
        case 1:
          {
            act("$n calls your mother a bitch!",
                FALSE, ch, 0, victim, TO_VICT);
          }
          break;
        default:
          {
            act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT);
          }
          break;
        }                       /*
                                 * end switch
                                 */

        act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT);
      }
      else
      {                         /*
                                 * ch == victim
                                 */
        send_to_char("You feel insulted.\r\n", ch);
      }
    }
  }
  else
    send_to_char("Sure you don't want to insult everybody.\r\n", ch);
}

void boot_pose_messages(void)
{
  FILE    *fl;
  int      counter, m_class;

  if (!(fl = fopen(POSEMESS_FILE, "r")))
  {
    logit(LOG_EXIT, "boot_pose_messages");
    raise(SIGSEGV);
  }
  for (counter = 0; counter < MAX_MESSAGES; counter++)
    pose_messages[counter].level = -1;

  for (counter = 0;; counter++)
  {
    fscanf(fl, " %d ", &pose_messages[counter].level);
    if (pose_messages[counter].level < 0)
      break;
    for (m_class = 0; m_class < 4; m_class++)
    {
      pose_messages[counter].poser_msg[m_class] = fread_action(fl);
      pose_messages[counter].room_msg[m_class] = fread_action(fl);
    }
  }

  fclose(fl);
}

void do_pose(P_char ch, char *argument, int cmd)
{
  byte     to_pose;
  byte     counter;
  int      m_class;

  if (IS_NPC(ch))
  {
    send_to_char("You can't do that.\r\n", ch);
    return;
  }
  for (counter = 0;
       (pose_messages[(int) counter].level < GET_LEVEL(ch)) &&
       (pose_messages[(int) counter].level > 0); counter++) ;
  counter--;

  if (counter > -1)
    to_pose = number(0, counter);
  else
  {
    send_to_char("Sorry, no poses for you.\r\n", ch);
    return;
  }

  if (GET_CLASS(ch, CLASS_WARRIOR) || GET_CLASS(ch, CLASS_RANGER) ||
      GET_CLASS(ch, CLASS_BERSERKER) || GET_CLASS(ch, CLASS_REAVER) ||
      GET_CLASS(ch, CLASS_PALADIN) || GET_CLASS(ch, CLASS_ANTIPALADIN))
    m_class = 3;
  else if (GET_CLASS(ch, CLASS_CLERIC) || GET_CLASS(ch, CLASS_MONK) ||
           GET_CLASS(ch, CLASS_DRUID) || GET_CLASS(ch, CLASS_SHAMAN))
    m_class = 1;
  else if (GET_CLASS(ch, CLASS_SORCERER) || GET_CLASS(ch, CLASS_NECROMANCER)
           || GET_CLASS(ch, CLASS_CONJURER) || GET_CLASS(ch, CLASS_PSIONICIST)
           || GET_CLASS(ch, CLASS_MINDFLAYER) || GET_CLASS(ch, CLASS_SUMMONER)
           || GET_CLASS(ch, CLASS_ILLUSIONIST))
    m_class = 0;
  else
    m_class = 2;                // bard, rogue, assassin, merc, anything else

/*
  switch (GET_CLASS(ch)) {
  case CLASS_WARRIOR:
  case CLASS_RANGER:
  case CLASS_PALADIN:
  case CLASS_ANTIPALADIN:
    m_class = 3;
    break;
  case CLASS_CLERIC:
  case CLASS_MONK:
  case CLASS_DRUID:
  case CLASS_SHAMAN:
    class = 1;
    break;
  case CLASS_SORCERER:
  case CLASS_NECROMANCER:
  case CLASS_CONJURER:
  case CLASS_SUMMONER:
    class = 0;
    break;
  case CLASS_BARD:
  case CLASS_ROGUE:
  case CLASS_ASSASSIN:
  case CLASS_MERCENARY:
  default:
    class = 2;
  }
*/

  act(pose_messages[(int) to_pose].poser_msg[(int) m_class], 0, ch, 0, 0,
      TO_CHAR);
  act(pose_messages[(int) to_pose].room_msg[(int) m_class], 0, ch, 0, 0,
      TO_ROOM);
}

/*
 * Function to yell things in a zone
 */
void do_yell(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];
  int range;

  /*
   * Check to see if the player is silenced by the gods
   */
  if (IS_PC(ch) && IS_SET(ch->specials.act, PLR_SILENCE))
  {
    send_to_char("You can't shout!\r\n", ch);
    return;
  }
  if (IS_THRIKREEN(ch) && !IS_TRUSTED(ch))
  {
    send_to_char("It's hard to shout without proper lungs..\r\n", ch);
    return;
  }
  if (!can_talk(ch))
    return;
  if (IS_AFFECTED2(ch, AFF2_SILENCED))
  {
    send_to_char("You strain, but with no result.\r\n", ch);
    return;
  }
  if ((IS_ILLITHID(ch) || IS_PILLITHID(ch)) && !IS_TRUSTED(ch))
  {
    send_to_char
      ("You cannot project your thoughts to so many that you cannot see.\r\n",
       ch);
    return;
  }
  if ((IS_SET(world[ch->in_room].room_flags, UNDERWATER) ||
       ch->specials.z_cord < 0) && !IS_TRUSTED(ch) && !IS_NPC(ch))
  {
    send_to_char
      ("You shout at the top of your lungs and inhale some water!\r\n", ch);
    act("$n mouth opens wide and huge bubbles come forth...", TRUE, ch, 0, 0,
        TO_ROOM);
    return;
  }
  range = get_property("map.shout.range", 40);
  range += GET_SIZE(ch) * get_property("map.shout.sizeFactor", 4);
  range = range * range;

  for (; *argument == ' '; argument++) ;
  if (!(*argument))
    send_to_char("Yell?  Yes!  Fine!  Shout we must, but WHAT??\r\n", ch);
  else if (!is_silent(ch, TRUE) && can_talk(ch))
  {
    /* Send message to player yelling that he has indeed yelled */
    if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
    {
      sprintf(Gbuf4, "You shout '%s'\r\n", argument);
      send_to_char(Gbuf4, ch);
    }
    else
      send_to_char("Ok.\r\n", ch);
      
    if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s shouts '%s'", GET_NAME(ch), argument);

    /* Load buffer with shout message */
    /* Send the message to everyone in the zone */
    for (i = descriptor_list; i; i = i->next)
    {
      if (i->character && (i->character != ch) &&
          !is_silent(i->character, FALSE) &&
          !IS_SET(i->character->specials.act, PLR_NOSHOUT) && !i->connected &&
          (!(world[ch->in_room].room_flags & INDOORS) ||
           (i->character->in_room == ch->in_room)))
      {
        if (world[i->character->in_room].zone != world[ch->in_room].zone)
          continue;
        if (IS_MAP_ROOM(ch->in_room) && 
            calculate_map_distance(ch->in_room, i->character->in_room) > range)
          continue;
/*
        if (racewar(ch, i->character))
        {
          act("$n shouts something but you can't understand.", FALSE, ch, 0,
              i->character, TO_VICT);
        }
        else
*/
        //{
          sprintf(Gbuf1, "$n shouts %s'%s'",
              language_known(ch, i->character), language_CRYPT(ch,
                i->
                character,
                argument));
          act(Gbuf1, 0, ch, 0, i->character, TO_VICT | ACT_SILENCEABLE);
        //}
        /* zone to zone method */
      }
    }
  }
  if (ch->desc)
    ch->desc->last_input[0] = '\0';
}

void do_beep(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  P_desc   d;
  char     name[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];

  one_argument(argument, name);

  if( IS_NPC(ch) && (IS_AFFECTED(ch, AFF_CHARM) || IS_MORPH(ch)) )
  {
    send_to_char("If only you could beep, you would..\r\n", ch);
    return;
  }

  if( PLR3_FLAGGED(ch, PLR3_NOBEEP) && (!IS_TRUSTED(ch)) )
  {
    send_to_char("You can't beep to someone if you have beep toggled off!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You cannot beep in that form!\r\n", ch);
    return;
  }
  if( !*name )
  {
    send_to_char("Who do you wish to beep?\r\n", ch);
    return;
  }
  if( (IS_SET(world[ch->in_room].room_flags, UNDERWATER)
    || ch->specials.z_cord < 0) && !IS_TRUSTED(ch))
  {
    send_to_char("Your species doesn't know how to beep underwater...", ch);
    return;
  }
  if( is_silent(ch, TRUE) )
  {
    send_to_char("You can't seem to beep much of anything right now.\r\n", ch);
    return;
  }

  vict = NULL;
  // Using descriptor list as in do_tell.
  for (d = descriptor_list; d; d = d->next)
  {
    if( !d->character || d->connected || !d->character->player.name
      || !isname(d->character->player.name, name)
      || !CAN_SEE_Z_CORD(ch, d->character) )
    {
      continue;
    }
    vict = d->character;
    break;
  }

  // No beeping across racewar sides, nor NPCs (stops crashbug).
  if( vict && ((!IS_TRUSTED(ch) && !IS_TRUSTED(vict)
    && GET_RACEWAR(ch) != GET_RACEWAR(vict)) || IS_NPC(vict)) )
  {
      vict = NULL;
  }
  if( isname("me", name)
    || isname("self", name) )
  {
    vict = ch;
  }
  if( !vict )
  {
    send_to_char( "No-one by that name here...\r\n", ch );
  }
  else if (ch == vict)
  {
    send_to_char("You sneak off to the corner and beep yourself.\a\r\n", ch);
  }
  else if( !vict->desc || (IS_AFFECTED4(vict, AFF4_DEAF)) )
  {
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR);
  }
  else if( (IS_SET(world[vict->in_room].room_flags, UNDERWATER)
    || vict->specials.z_cord < 0) && !IS_TRUSTED(vict) )
  {
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  else if( !IS_TRUSTED(ch)
    && ((IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT)
    || IS_SET(world[ch->in_room].room_flags, ROOM_SILENT))
    || GET_STAT(vict) < STAT_RESTING || (PLR3_FLAGGED(vict, PLR3_NOBEEP)
    && !is_linked_from(vict, ch, LNK_CONSENT))))
  {
    act("No-one by that name here...", FALSE, ch, 0, vict, TO_CHAR);
    if( IS_NPC(ch) && IS_SET(vict->specials.act, PLR_NOTELL) )
    {
      act("$n attempted to beep you!", FALSE, ch, 0, vict, TO_VICT);
    }
    return;
  }
  else if( !IS_TRUSTED(ch) && (GET_STAT(ch) < STAT_RESTING) )
  {
    send_to_char("You can't do that right now...\r\n", ch);
  }
  else if( vict->only.pc->ignored == ch && !IS_TRUSTED(ch) )
  {
    act("$N is ignoring you at the moment.  No dice.", FALSE, ch, 0, vict, TO_CHAR);
  }
  else
  {
    if( ch->desc )
    {
      if( IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch) )
      {
        sprintf( Gbuf1, "&+WYou beep %s!\r\n", J_NAME(vict) );
        send_to_char( Gbuf1, ch, LOG_NONE );
      }
      else
      {
        send_to_char("Ok.\r\n", ch);
      }
      CharWait(ch, PULSE_VIOLENCE);
      if( IS_SET(vict->specials.act, PLR_AFK) )
      {
        act("$E is away from $S keyboard right now..", FALSE, ch, 0, vict, TO_CHAR);
      }
      if( !CAN_SEE(vict, ch) )
      {
      	act("&+L$E cannot see you..&n", FALSE, ch, 0, vict, TO_CHAR);
      }
      if( get_property("logs.chat.status", 0.000) && IS_PC(ch) && IS_PC(vict) )
      {
        logit( LOG_CHAT, "%s beeps %s.", J_NAME(ch), J_NAME(vict) );
      }
    }

    sprintf(Gbuf1, "&+W%s&+W beeps you.\a\r\n", CAN_SEE(vict, ch) ? J_NAME(ch) : "Someone" );
    send_to_char(Gbuf1, vict, LOG_NONE);
    if (IS_SET(vict->specials.act, PLR_AFK))
    {
      act("$n beeped you, and your &+RAFK&N is toggled on!", FALSE, ch, 0, vict, TO_VICT);
    }
  }
}

