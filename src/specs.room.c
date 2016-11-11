/*43341 446 445 starting room 43341 85606
   ***************************************************************************
   *  File: specs.room.c                                       Part of Duris *
   *  Usage: special procedures for rooms                                      *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
   *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.             *
   ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "justice.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "damage.h"
#include "files.h"
#include "sql.h"
#include "specs.winterhaven.h"
#include "guildhall.h"
#include "assocs.h"

/*
   external variables
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_event current_event;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern const char *dirs[];
extern const char rev_dir[];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int planes_room_num[];
extern int innate_abilities[];
extern const int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern const char *specdata[][MAX_SPEC];
extern const struct class_names class_names_table[];

void mobPatrol_SetupNew(P_char ch);

int berserker_proc_room(int room, P_char ch, int cmd, char *arg)
{
  if( (cmd == CMD_SET_PERIODIC) || !IS_ALIVE(ch) || IS_NPC(ch)
    || IS_MORPH(ch) || !((cmd == CMD_BATTLERAGER)) )
  {
    return FALSE;
  }

  if( IS_FIGHTING(ch) || IS_IMMOBILE(ch) || !IS_AWAKE(ch) )
  {
    send_to_char("&+yYou are unable to use this command at this time...\r\n", ch);
    return TRUE;
  }

  if( GET_RACE(ch) != RACE_MOUNTAIN && GET_RACE(ch) != RACE_DUERGAR )
  {
    send_to_char("&+rYour race receives no benefit from this altar...\r\n", ch);
    return TRUE;
  }

  if(GET_CLASS(ch, CLASS_BERSERKER))
  {
    send_to_char("&+RYour blood boils and your eyes &+Yflame &+Ras you recant the legends of old.\r\n", ch);
    return TRUE;
  }

  if( !arg || !(*arg) || !isname(arg, "confirm") )
  {
    send_to_char("You must type 'battlerager confirm' for the transformation.\r\nIt's a permanent transformation.\r\n&+RYOU WILL LOSE ALL YOUR EPIC SKILLS!&n\r\n&+RYOU WILL LOSE ALL YOUR EPIC SKILLS!\r\n", ch);
    return TRUE;
  }

  if(GET_LEVEL(ch) < (get_property("berserker.quest.level.req.min", 41)))
  {
    send_to_char("You need more &+ylevels&n to undergo the &+Rbattlerager&n transformation.\r\n", ch);
    return TRUE;
  }

  if(ch->only.pc->epics < 3)
  {
    send_to_char("You need more &+yepic&n experience to undergo the &+Rbattlerager&n transformation.\r\n", ch);
    return TRUE;
  }

  ch->points.max_mana = 0;
  ch->points.base_vitality = 0;
  uint played = ch->player.time.played;
  do_start(ch, 1);
  ch->player.time.played = played;

  act("$n's eyes turn &+Lblack&n then &+rblood red&n as some &+Wunseen force&n dominates $s existence.",
    FALSE, ch, 0, 0, TO_ROOM);

  act("\r\nYou fall into a deep trance, and the legends of the &+Rbattleragers&n flood your thoughts.\r\n"
    "The ancient ways from so long ago &+Wcascade&n into everything that you are...&n\r\n"
    "\r\n&+cA mystical force flows into and through your body.&n\r\n\r\n"
    "&+CCold anger envelops your soul!&n\r\n",
    FALSE, ch, 0, 0, TO_CHAR);

  ch->player.m_class = CLASS_BERSERKER;
  ch->player.spec = 0;
  forget_spells(ch, -1);
  qry("DELETE FROM zone_trophy WHERE pid = %d", GET_PID(ch));
  do_restore(ch, GET_NAME(ch), 0);
  berserk(ch, 500);

  return TRUE;
}

extern Skill skills[];
void save_epic_skills(P_char ch, int *skls)
{
  for (int i = 0; i < MAX_SKILLS; i++)
  {
     if (IS_EPIC_SKILL(i))
         skls[i] = ch->only.pc->skills[i].learned;
     else
         skls[i] = 0;
  }
}
void restore_epic_skills(P_char ch, int* skls)
{
  for (int i = 0; i < MAX_SKILLS; i++)
  {
     if (skls[i] > 0)
         ch->only.pc->skills[i].taught = ch->only.pc->skills[i].learned = skls[i];
  }
}

int multiclass_proc(int room, P_char ch, int cmd, char *arg)
{
  int      i;
  int skls[MAX_SKILLS];
  int min_level = get_property("multiclass.level.req.min", 50);

  if( (cmd == CMD_SET_PERIODIC) || (ch == NULL) || IS_NPC(ch) || IS_MORPH(ch)
    || cmd != CMD_MULTICLASS )
  {
    return FALSE;
  }

// SET_BIT(ch->specials.act2, PLR2_HARDCORE_CHAR);
  if( GET_RACE(ch) != RACE_ORC && GET_RACE(ch) != RACE_HUMAN )
  {
    send_to_char("Your race is not versatile enough to learn that much.\n", ch);
    return FALSE;
  }

  if (IS_MULTICLASS_PC(ch))
  {
    return FALSE;
  }
  else if( GET_LEVEL(ch) < min_level )
  {
    do_multiclass(ch, "", -1);  // show list of available secondary classes
  }
  else
  {
    while( *arg == ' ' )
      arg++;

    if( ch->only.pc->epics < get_property("multiclass.epic.req.min", 250) )
    {
      send_to_char("You need more &+yepic&n experience.\r\n", ch);
      return FALSE;
    }
    if (!*arg)
    {
      do_multiclass(ch, "", -1);        // show list of available secondary classes
    }
    else
    {
      for( i = 1; i <= CLASS_COUNT; i++ )
      {
        if( is_abbrev(arg, class_names_table[i].normal) )
          break;
      }

      if( i > CLASS_COUNT )
      {
        send_to_char("That's no class, bubba.\r\n", ch);
      }
      else if( can_char_multi_to_class(ch, i ) )
      {
          ch->points.max_mana = 0;
          ch->points.base_vitality = 0;
          uint played = ch->player.time.played;
// These aren't needed anymore since do_start is now smarter and doesn't reset epic skills/tradeskills.
//          save_epic_skills(ch, skls);
          do_start(ch, cmd);
//          restore_epic_skills(ch, skls);
          ch->player.time.played = played;
          send_to_char("&+WWith a suddenness that stops your breath, you feel oddly different..  less powerful, and yet with the potential to be something much greater than you ever were before.\r\n", ch);
          ch->player.secondary_class = 1 << (i - 1);
          ch->player.spec = 0;
          do_restore(ch, GET_NAME(ch), 0);
          forget_spells(ch, -1);
          qry("DELETE FROM zone_trophy WHERE pid = %d", GET_PID(ch));
      }
      else
      {
        send_to_char("You cannot multiclass to that particular class.\r\n", ch);
      }
    }
  }

  return TRUE;
}

int inn(int room, P_char ch, int cmd, char *arg)
{
  struct zone_data *zone;
  P_char   tch, next_ch;
  char timestr[MAX_STRING_LENGTH];
  time_t ct;

  /* check for periodic event calls */
  if(cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( !ch || IS_NPC(ch) || cmd != CMD_RENT )
  {
    return FALSE;
  }

  if( room > 0 && room < top_of_world )
  {
    if( IS_SET(ch->specials.affected_by, AFF_KNOCKED_OUT) )
    {
      send_to_char("Being knocked out, renting is not really an option for you.\r\n", ch);
      return FALSE;
    }
    if( IS_STUNNED(ch) )
    {
      send_to_char("You're too stunned to think of renting!\r\n", ch);
      return FALSE;
    }
    if( IS_MORPH(ch) )
    {
      send_to_char( "&+YNot in this form!\n\r", ch );
      return TRUE;
    }

    if( IS_AFFECTED2(ch, AFF2_FLURRY) )
    {
      send_to_char("You are too deep in battle madness!\r\n", ch);
      return FALSE;
    }
    if( (PC_JUSTICE_FLAGS(ch) == JUSTICE_IS_OUTCAST) || justice_is_criminal(ch) || IS_AFFECTED4(ch, AFF4_LOOTER) )
    {
      send_to_char("The innkeeper says 'I don't serve outlaws like you! Begone!'\r\n", ch);
      return TRUE;
    }
    // Allowing Illithids to rent in their own gh.
    if( IS_NOTWELCOME(ch) ) // || GET_RACE(ch) == RACE_ILLITHID )
    {
      send_to_char("You are NOT welcome here!\r\n", ch);
      return TRUE;
    }

    Guildhall *gh = Guildhall::find_by_vnum(world[ch->in_room].number);
    if( gh && (!IS_MEMBER(GET_A_BITS(ch)) || GET_ASSOC(ch) != gh->guild) )
    {
      send_to_char("You're just a guest here, so you should probably stay awake!\r\n", ch);
      return TRUE;
    }

// old guildhalls (deprecated)
//      for(tch = world[ch->in_room].people; tch; tch = next_ch)
//      {
//        next_ch = tch->next_in_room;
//        if(tch &&
//          IS_ASSOC_GOLEM(tch)) // Let's not allow inns in the guard room -Lucrot Oct08
//        {
//          send_to_char
//            ("What innkeeper? The golem standing right here has shut this place down!\r\n",
//             ch);
//          return true;
//        }
//      }
    if( IS_FIGHTING(ch) )
    {
      send_to_char("The innkeeper is too busy hiding behind the counter to help.\r\n", ch);
      return TRUE;
    }
    if( IS_IMMOBILE(ch) )
    {
      send_to_char("&+WNo can do!&n Try back when you can move.\r\n", ch);
      return TRUE;
    }

    if( GET_STAT(ch) == STAT_SLEEPING )
    {
      send_to_char("Why go to an inn? You are plenty comfortable right here.\r\n", ch);
      return TRUE;
    }

    if( IS_BLIND(ch) )
    {
      send_to_char("You bump into stuff... Where the heck is everyone?\r\n", ch);
      return TRUE;
    }

    if(affected_by_spell(ch, TAG_PVPDELAY))
    {
      send_to_char("There is too much adrenaline pumping through your body right now.\r\n", ch);
      return FALSE;
    }

    zone = &zone_table[world[ch->in_room].zone];
    if( zone->status > ZONE_NORMAL && (GET_LEVEL(ch) > 25)
      && (zone_table[world[ch->in_room].zone].number != WINTERHAVEN) )
    {
      send_to_char("The receptionist turns and says, '&+RThe town is under attack!&n'\n", ch);
      send_to_char("  '&+WTry to get a portal out of here, or run to another exit!&n'\n", ch);
      send_to_char("  '&+WPlease hurry great adventurer, the town needs you.&n'\n", ch);
      send_to_char("The receptionist wishes you good luck. \n", ch);

      add_event(event_justice_raiding, 200, ch, 0, 0, 0, &room, sizeof(room));
      return FALSE;
    }

    if( IS_AFFECTED4(ch, AFF4_TUPOR) )
    {
      send_to_char( "&+cYour mind ceases its trance.&n\n\r", ch );
      REMOVE_BIT(ch->specials.affected_by4, AFF4_TUPOR);
    }

    send_to_char("The innkeeper stores your stuff in the safe and shows you to your room.\r\n", ch);
    act("The innkeeper stores $n&n's stuff in the safe and shows $m to $s room.", TRUE, ch, 0, 0, TO_ROOM);

    if( ch->following )
    {
      do_dismiss(ch, NULL, CMD_RENT);
    }

    GET_HOME(ch) = world[ch->in_room].number;

    if( !writeCharacter(ch, RENT_CRASH, ch->in_room) )
    {
      send_to_char("Failed to save this character, most likely too much eq.\r\n", ch);
      wizlog(56, "%s was unable to rent [specs.room.c()].", GET_NAME(ch));
      return TRUE;
    }
    else
    {
      writeCharacter(ch, RENT_INN, ch->in_room);
    }

    if( !ch )
    {
      /* HTF is the mud supposed to send a message or get the name of a char that doesn't exist?!?
      send_to_char("You are unable to rent for some strange reason. Contact the imms.\r\n", ch);
      wizlog(56, "%s was unable to rent [specs.room.c()].", GET_NAME(ch));
      */
      return TRUE;
    }
    ct = time(NULL);
    // Convert to EST. - this sux.. gotta be a be'er way.
    ct -= 4*60*60;
    sprintf( timestr, "%s", asctime(localtime( &ct )) );
    sprintf( timestr + strlen(timestr) - 1, " EST" );

    loginlog( ch->player.level, "%s [%s] has rented out in [%d] @ %s.",
      GET_NAME(ch), (ch->desc) ? ch->desc->host : "LINKDEAD", world[ch->in_room].number, timestr );
    sql_log(ch, CONNECTLOG, "Rented Out");
    // If it's not an immortal.
    if( IS_PC(ch) && (GET_LEVEL( ch ) < MINLVLIMMORTAL) )
    {
      update_ingame_racewar( -GET_RACEWAR(ch) );
    }
    extract_char(ch);
    ch = NULL;

    return TRUE;
  }
  return FALSE;
}

int undead_inn(int room, P_char ch, int cmd, char *arg)
{

  /* check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch == NULL)
    return (FALSE);

  if (IS_NPC(ch) || IS_MORPH(ch))
    return (FALSE);

  if (cmd != CMD_RENT)
    return (FALSE);

  if (IS_SET(ch->specials.affected_by, AFF_KNOCKED_OUT))
  {
    send_to_char
      ("Being knocked out, renting is not really an option for you.\r\n", ch);
    return FALSE;
  }
  if (IS_STUNNED(ch))
  {
    send_to_char("You're too stunned to think of renting!\r\n", ch);
    return FALSE;
  }
  if (!IS_RACEWAR_UNDEAD(ch))
  {
    send_to_char
      ("The innkeeper grins slyly and moans, 'We have no beds here!'\r\n",
       ch);
    return TRUE;
  }
  if ((PC_JUSTICE_FLAGS(ch) == JUSTICE_IS_OUTCAST) ||
      justice_is_criminal(ch) || IS_AFFECTED4(ch, AFF4_LOOTER))
  {
    send_to_char
      ("The innkeeper says 'I don't serve outlaws like you! Begone!'\r\n",
       ch);
    return TRUE;
  }
  if (cmd == CMD_RENT)
  {
    if (IS_FIGHTING(ch))
    {
      send_to_char
        ("The innkeeper is too busy hiding behind the counter to help.\r\n",
         ch);
      return TRUE;
    }
    send_to_char
      ("The innkeeper shows you to a rotted coffin, you climb in and shut the lid.\r\n",
       ch);
    act
      ("The innkeeper shows $n&n to a rotted coffin. $n climbs in and shuts the lid.",
       TRUE, ch, 0, 0, TO_ROOM);

    if (IS_AFFECTED4(ch, AFF4_TUPOR))
      REMOVE_BIT(ch->specials.affected_by4, AFF4_TUPOR);

    GET_HOME(ch) = world[ch->in_room].number;
    if (IS_PC(ch))
      writeCharacter(ch, 3, ch->in_room);
    loginlog(ch->player.level, "%s [%s] has rented out in [%d].",
             GET_NAME(ch), (ch->desc) ? ch->desc->host : "LINKDEAD",
             world[ch->in_room].number);
    sql_log(ch, CONNECTLOG, "Rented Out");
    // If it's not an immortal.
    if( IS_PC(ch) && (GET_LEVEL( ch ) < MINLVLIMMORTAL) )
    {
      update_ingame_racewar( -GET_RACEWAR(ch) );
    }
    extract_char(ch);
    ch = NULL;
  }
  return (TRUE);
}

int GithyankiCave(int room, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return (FALSE);

  if (cmd != CMD_DOWN)
    return (FALSE);

  if (!IS_TRUSTED(ch) && !IS_ILLITHID(ch))
    return (FALSE);

  char_from_room(ch);
  char_to_room(ch, real_room0(96524), -1);

  return (TRUE);
}

int TiamatThrone(int room, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return (FALSE);

  if (IS_NPC(ch))
    return FALSE;

  if (cmd != CMD_WEST)
    return (FALSE);

  char_from_room(ch);
  char_to_room(ch, real_room(19623), -1);

  return (TRUE);
}


int GlyphOfWarding(int room, P_char ch, int cmd, char *arg)
{
  int      Damage = 50;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return (FALSE);

  if (cmd != CMD_WEST)
    return (FALSE);

  if ((GET_RACE(ch) == RACE_DROW) || (GET_RACE(ch) == RACE_DUERGAR) ||
      (GET_RACE(ch) == RACE_TROLL) || (GET_RACE(ch) == RACE_OGRE) ||
      !IS_TRUSTED(ch))
  {
    act
      ("$n is blasted by a bright blue bolt of magical energy emanating from the dwarven glyph of warding as $e attempts to leave west!",
       FALSE, ch, 0, 0, TO_ROOM);
    act
      ("A powerful bolt of bright blue energy blasts you backwards several feet from a dwarven glyph of warding just above the western doorway!",
       FALSE, ch, 0, 0, TO_CHAR);
    if (IS_AFFECTED(ch, AFF_MINOR_GLOBE) || IS_AFFECTED2(ch, AFF2_GLOBE))
      Damage = 25;
    if (GET_HIT(ch) > Damage)
      GET_HIT(ch) -= Damage;
    else
      GET_HIT(ch) = 1;
    StartRegen(ch, EVENT_HIT_REGEN);
  }
  else
  {
    send_to_char
      ("You feel the slight tingle of magic from the glyph as you leave west.\r\n",
       ch);
    return FALSE;
  }

  return (TRUE);
}

int akh_elamshin(int room, P_char ch, int cmd, char *arg)
{
  char     buf[MAX_INPUT_LENGTH], god[MAX_INPUT_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch == NULL)
    return (FALSE);

  if (cmd != CMD_WORSHIP)
    return (FALSE);

  if (GET_OPPONENT(ch))
    return (FALSE);

  send_to_char
    ("&+mYou and the statue both begin to glow with an aura of violet energy!\r\n"
     "&+mYour aura &=LCflashes&N&+m, and you slowly fade out of exsistance..\r\n",
     ch);
  if (GET_RACE(ch) != RACE_DROW)
  {
    send_to_char
      ("&+rThe aura around you suddenly turns &+Rbright red!\r\n&+rYou shiver as you "
       "can feel presence of her majesty, the spider queen &+mLloth..\r\n&+rYour "
       "worship of her has allowed her into your heart, to touch your soul.\r\n&+rAn "
       "embarrassing feeling of passionate pleasure washes over you for a moment.\r\n"
       "&+rYou collapse and start losing consciousness, overcome with joyous evil.\r\n",
       ch);
    GET_HIT(ch) = 0;
    if ((GET_RACE(ch) != RACE_DROW) && (GET_RACE(ch) != RACE_DUERGAR) &&
        (GET_RACE(ch) != RACE_OGRE) && (GET_RACE(ch) != RACE_TROLL))
    {
      GET_ALIGNMENT(ch) -= 100;
      if (GET_ALIGNMENT(ch) < -1000)
        GET_ALIGNMENT(ch) = -1000;
    }
    send_to_char
      ("\r\n.\r\n.\r\nYou slowly materialize outside the great gates of Verzanan, and pass out...\r\n",
       ch);
    KnockOut(ch, 80);           /*
                                   20 seconds
                                 */
  }
  else
  {
    send_to_char
      ("\r\n.\r\n.\r\nYou slowly materialize outside the great gates of Verzanan, and pass out...\r\n",
       ch);
  }
  char_from_room(ch);
  char_to_room(ch, real_room(3600), 0);
  send_to_room
    ("&+LThere is a low hum as the statue begins to vibrate in a deep harmony\r\n",
     real_room(8531));
  sprintf(buf,
          "&+mBoth %s and the statue are surrounded by a &+Mviolet&N&+m aura of energy!\r\n",
          GET_NAME(ch));
  send_to_room(buf, real_room(8531));

  if (GET_SEX(ch) == 1)
    sprintf(god, "he");
  else
    sprintf(god, "she");
  if (GET_RACE(ch) != RACE_DROW)
  {
    sprintf(buf,
            "&+rThe aura around %s suddenly turns red and %s passes out!\r\n",
            GET_NAME(ch), god);
    send_to_room(buf, real_room(8531));
  }
  sprintf(buf,
          "&+MThe aura &=LCflashes&N&+M, and %s slowly fades out of exsistance..\r\n",
          god);
  send_to_room(buf, real_room(8531));
  act("&+m$n materializes from a shower of &+Mviolet&N&+m sparks.", FALSE, ch,
      0, 0, TO_NOTVICT);

  return (TRUE);
}

/*
   A room proc
 */

int dump(int room, P_char ch, int cmd, char *arg)
{
  P_obj    obj, next_obj;
  P_char   tmp_char;
  int      value = 0;
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return (FALSE);

  for( obj = world[ch->in_room].contents; obj; obj = next_obj )
  {
    next_obj = obj->next_content;

    if( IS_ARTIFACT(obj) )
    {
      continue;
    }
    sprintf(Gbuf1, "The %s vanishes in a puff of smoke.\r\n", FirstWord(obj->name));
    for( tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room )
      if (CAN_SEE_OBJ(tmp_char, obj))
        send_to_char(Gbuf1, tmp_char);
    extract_obj(obj, TRUE); // Not an arti, but 'in game.'
  }

  if (cmd != CMD_DROP)
    return (FALSE);

  do_drop(ch, arg, 0);

  value = 0;

  for( obj = world[ch->in_room].contents; obj; obj = next_obj )
  {
    next_obj = obj->next_content;

    if( IS_ARTIFACT(obj) )
    {
      continue;
    }

    sprintf(Gbuf1, "The %s vanishes in a puff of smoke.\r\n", FirstWord(obj->name));
    for( tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room )
      if (CAN_SEE_OBJ(tmp_char, obj))
        send_to_char(Gbuf1, tmp_char);
    if (obj->type == ITEM_MONEY)
      value = 1;
    else
      value += MAX(1, MIN(50, obj->cost / 10));

    extract_obj(obj, TRUE); // Not an arti, but 'in game.'
  }

  if (value > 0)
  {
    act("You are awarded for outstanding performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    ADD_MONEY(ch, value);
  }
  return TRUE;
}

int count_patrol(int vnum)
{
    P_char   patrol;
    int i = 0;
    int count = 0;

    for (patrol = character_list; patrol; patrol = patrol->next)

    {

      if (IS_NPC(patrol) &&
          vnum == GET_VNUM(patrol))

        count++;
    }


return count;

}

//Buy a patrol! mobPatrol_SetupNew(CH);
int patrol_shops(int room, P_char ch, int cmd, char *arg)
{
  char     buf[MAX_INPUT_LENGTH];
  char     name[MAX_STRING_LENGTH], master[MAX_STRING_LENGTH];
  int      pet_room, val, temp, count = 0;
  P_char   pet = NULL, mount = NULL;
  P_obj    ticket;

  /* check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return (FALSE);

  pet_room = ch->in_room + 1;

  if (cmd == CMD_LIST)
  {                             /* List */
    send_to_char("Available patrols are:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room)
    {
      count++;
      temp = GET_EXP(pet) / 2;
      sprintf(buf, "[%d] %-25s for %s, currently %d patroling.\r\n", count, (pet->player.short_descr),
              coin_stringv(temp), count_patrol(GET_VNUM(pet)));
      send_to_char(buf, ch);
    }
    return (TRUE);
  }
  else if (cmd == CMD_BUY)
  {                             /* Buy */

    arg = one_argument(arg, buf);

    if (!(pet = get_char_room(buf, pet_room)))
      if (atoi(buf))
        for (pet = world[pet_room].people; pet; pet = pet->next_in_room)
          if (++count == atoi(buf))
            break;

    if (!pet)
    {
      send_to_char("There is no such patrol!\r\n", ch);
      return (TRUE);
    }

   
    if(count_patrol(GET_VNUM(pet)) > 8){
      send_to_char("Enough patrolmen are currently protecting the roads..\r\n", ch);
      return (TRUE);
    }
    
    temp = GET_EXP(pet) / 2;

    if (GET_MONEY(ch) < temp)
    {
      send_to_char("You dont have enough money!\r\n", ch);
      return (TRUE);
    }
    if (!(pet = read_mobile(GET_RNUM(pet), REAL)))
    {
      send_to_char("Sorry, we seem to be out of stock!\r\n", ch);
      return TRUE;
    }
    SUB_MONEY(ch, temp, 0);

    char_to_room(pet, pet_room+1, 0);
    do_flee(pet, 0, 0);
    send_to_char("You hire a guardian of the area.\r\n", ch);
    act("$n bought $N as patrol.", FALSE, ch, 0, pet, TO_ROOM);
    mobPatrol_SetupNew(pet);
    wizlog(56, "%s bought %s as patrol!", GET_NAME(ch), pet->player.short_descr);
    return (TRUE);
  }
  
  return (FALSE);
}

/* silly little funcs, used only in pet_shop */
int mount_rent_cost(P_char mount)
{
  return (GET_LEVEL(mount) * 100);
}

int pet_shops(int room, P_char ch, int cmd, char *arg)
{
  char     buf[MAX_INPUT_LENGTH];
  char     name[MAX_STRING_LENGTH], master[MAX_STRING_LENGTH];
  int      pet_room, val, temp, count;
  P_char   pet = NULL, mount = NULL;
  P_obj    ticket;

  /* check for periodic event calls */
  if( cmd == CMD_SET_PERIODIC )
    return FALSE;

  if( !IS_ALIVE(ch) )
    return  FALSE;

  pet_room = ch->in_room + 1;

  if( cmd == CMD_LIST )
  {
    send_to_char("Available hirelings are:\r\n", ch);
    for( pet = world[pet_room].people, count = 0; pet; pet = pet->next_in_room )
    {
      temp = 8 * GET_EXP(pet);
      sprintf(buf, "[%d] %s for %s.\r\n", ++count, pad_ansi(pet->player.short_descr, 25, TRUE).c_str(), coin_stringv(temp));
      send_to_char(buf, ch);
    }
    return TRUE;
  }
  else if( cmd == CMD_BUY )
  {

    arg = one_argument(arg, buf);

    if( !(pet = get_char_room(buf, pet_room)) )
    {
      if( (count = atoi(buf)) > 0 )
      {
        for( pet = world[pet_room].people; pet; pet = pet->next_in_room )
        {
          if( --count == 0 )
          {
            break;
          }
        }
      }
    }

    if( !pet )
    {
      send_to_char("There is no such pet!\r\n", ch);
      return TRUE;
    }
    if( (GET_LEVEL(pet) + 5) > GET_LEVEL(ch))
    {
      send_to_char("You not experienced enough for a pet like that!\r\n", ch);
      return (TRUE);

    }
    if (count_pets(ch) >= 2)
    {
      send_to_char("You may only have 2 pets\r\n", ch);
      return (TRUE);
    }
    temp = GET_EXP(pet) * 8;
    if (GET_MONEY(ch) < temp)
    {
      send_to_char("You dont have enough money!\r\n", ch);
      return (TRUE);
    }
    if (!(pet = read_mobile(GET_RNUM(pet), REAL)))
    {
      send_to_char("Sorry, we seem to be out of stock!\r\n", ch);
      return TRUE;
    }
    SUB_MONEY(ch, temp, 0);

    if( !isname("_hireling_", GET_NAME(pet)) )
    {
      // Allowing pets to be warriors.
      if( pet->player.m_class != CLASS_WARRIOR )
        pet->player.m_class = CLASS_NONE;
    }

    setup_pet(pet, ch, -1, PET_NOCASH);
    char_to_room(pet, ch->in_room, 0);
    add_follower(pet, ch);

    send_to_char("May you enjoy your pet.\r\n", ch);
    act("$n bought $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return (TRUE);
  }
  else if (cmd == CMD_RENT)
  {
#if 0
    send_to_char("Stables are out of order temporarily.\r\n", ch);
    return TRUE;
#endif
    one_argument(arg, name);
    if (!*name)
      return FALSE;
    if (!(mount = get_char_room_vis(ch, name)))
    {
      send_to_char("Rent which mount?\r\n", ch);
      return TRUE;
    }
    if (GET_MASTER(mount) != ch)
    {
      send_to_char("You can only rent your own permanent pet.\r\n", ch);
      return TRUE;
    }
    val = mount_rent_cost(mount);
    if (val > GET_MONEY(ch))
    {
      send_to_char("Ye don't have the monies to rent yer pet.\r\n", ch);
      return TRUE;
    }
    if (IS_FIGHTING(mount))
    {
      send_to_char("Yer pet is too busy fighting!\r\n", ch);
      return TRUE;
    }
    SUB_MONEY(ch, val, 0);

    /* okay to stable */

    if (writePet(mount))
    {
      act
        ("A stable-hand grins, 'I will take very good care of yer pet until ye return.'",
         FALSE, ch, 0, 0, TO_CHAR);
      act("$N follows a stable-hand into the back of the house.", FALSE, ch,
          0, mount, TO_ROOM);

      /* create a claim ticket */
      if (!(ticket = read_object(5, VIRTUAL)))
      {
        logit(LOG_DEBUG, "Obj #5 (ticket) not loaded via stable proc.");
        send_to_char("Damn, out of claim tickets today. Try again later.",
                     ch);
        return TRUE;
      }
      ticket->str_mask =
        (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3);
      ticket->name = str_dup("ticket claim paper note");
      ticket->short_description = str_dup("a claim ticket");
      ticket->description = str_dup("A small piece of paper lies discarded.");
      ticket->cost = 10 * GET_LEVEL(mount);
      ticket->value[0] = mount_rent_cost(mount);
      ticket->value[1] = GET_IDNUM(mount);
      ticket->value[2] = world[ch->in_room].number;
      ticket->value[3] = time(NULL);

      act("A stable-hand hands you a ticket, saying 'Pay when ya pick up.'",
          FALSE, ch, 0, 0, TO_CHAR);
      act("A stable-hand hands $n a ticket, saying 'Pay when ya pick up.'",
          FALSE, ch, 0, 0, TO_ROOM);
      obj_to_char(ticket, ch);
      extract_char(mount);
      mount = NULL;
      return TRUE;
    }
    else
    {
      send_to_char("Error in stables, sorry.\r\n", ch);
      wizlog(AVATAR, "Cannot write pet file.\r\n");
      return TRUE;
    }
  }
  else if (cmd == CMD_OFFER)
  {
    one_argument(arg, name);
    if (!(mount = get_char_room_vis(ch, name)))
    {
      send_to_char("Which pet?\r\n", ch);
      return TRUE;
    }
    if (GET_MASTER(mount) != ch)
    {
      send_to_char("Ye can only rent yer own pet.\r\n", ch);
      return TRUE;
    }
    val = mount_rent_cost(mount);
    sprintf(buf, "A stable-hand says, 'That pet will cost ye %s to rent.'",
            coin_stringv(val));
    act(buf, FALSE, ch, 0, 0, TO_CHAR);
    return TRUE;
  }
  else if (cmd == CMD_GIVE)
  {                             /*  unrent pet */
    half_chop(arg, name, master);
    if (!*name /* || !*master */ )      /* future addition of mob in room */
      return FALSE;
    if (!(ticket = get_obj_in_list_vis(ch, name, ch->carrying)))
    {
      return FALSE;
    }
#if 0
    /* This is cost per real life day */
    val = ((ticket->value[3] - time(NULL) / SECS_PER_REAL_DAY) * ticket->value[0]);
#else
    /* and this is the flat rate price */
    val = ticket->value[0];
#endif
    if (GET_MONEY(ch) < val)
    {
      send_to_char("Ye dont have enough cash!\r\n", ch);
      return TRUE;
    }
    if (ticket->value[2] != world[ch->in_room].number)
    {
      return FALSE;
    }
    sprintf(buf, "%s%d", GET_NAME(ch), ticket->value[1]);
//    petrestore(ch, buf);
    SUB_MONEY(ch, val, 0);
    obj_from_char(ticket);
    extract_obj(ticket, TRUE); // Not an arti, but 'in game.'

    send_to_char("A stable-hand brings yer pet from around back.\r\n", ch);
    act("A stable-hand returns $n's pet.", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  return (FALSE);
}

/* Idea of the LockSmith is functionally similar to the Pet Shop */
/* The problem here is that each key must somehow be associated  */
/* with a certain player. My idea is that the players name will  */
/* appear as the another Extra description keyword, prefixed     */
/* by the words 'item_for_' and followed by the player name.     */
/* The (keys) must all be stored in a room which is (virtually)  */
/* adjacent to the room of the lock smith.                       */

int pray_for_items(int room, P_char ch, int cmd, char *arg)
{
  int      key_room, gold;
  bool     found;
  P_obj    obj;
  struct extra_descr_data *ext;
  char     Gbuf4[MAX_STRING_LENGTH];

  /* check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_PRAY)          /* You must pray to get the stuff */
    return FALSE;

  key_room = 1 + ch->in_room;

  strcpy(Gbuf4, "item_for_");
  strcat(Gbuf4, GET_NAME(ch));

  gold = 0;
  found = FALSE;

  for (obj = world[key_room].contents; obj && !found; obj = obj->next_content)
    for (ext = obj->ex_description; ext && !found; ext = ext->next)
      if (str_cmp(Gbuf4, ext->keyword) == 0)
      {
        if (gold == 0)
        {
          gold = 1;
          act("$n kneels and at the altar and chants a prayer to Odin.",
              FALSE, ch, 0, 0, TO_ROOM);
          act("You notice a faint light in Odin's eye.",
              FALSE, ch, 0, 0, TO_CHAR);
        }
        obj_from_room(obj);
        act("$p slowly fades into existence.", FALSE, ch, obj, 0, TO_ROOM);
        act("$p slowly fades into existence.", FALSE, ch, obj, 0, TO_CHAR);
        gold += obj->cost;
        obj_to_room(obj, ch->in_room);
        found = TRUE;
      }
  if (found)
  {
    SUB_MONEY(ch, gold, 0);
    return TRUE;
  }
  return FALSE;
}

int kings_hall(int room, P_char ch, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_PRAY)
    return (0);

  do_action(ch, arg, CMD_PRAY);

  send_to_char("You feel as if some mighty force has been offended.\r\n", ch);
  send_to_char
    ("You are torn out of reality!\r\nYou roll and tumble through endless voids"
     " for what seems like eternity...\r\n\r\n"
     "After a time, a new reality comes into focus... you are elsewhere.\r\n",
     ch);

  act("$n is struck by an intense beam of light and vanishes.",
      TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, real_room(1420), 0); /*
                                           behind the altar
                                         */
  return (1);
}

// A locked door that eats the key to the lock.  Must open another way heh.
// Not currently in game. - 7/4/2015
int feed_lock(int room, P_char ch, int cmd, char *arg)
{
  P_obj    obj;
  int      door;
  char     type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_UNLOCK)
    return FALSE;

  if (cmd == CMD_UNLOCK)
  {
    argument_interpreter(arg, type, dir);
    if (!(door = find_door(ch, type, dir)))
      return FALSE;
    if (!(obj = find_key(ch, EXIT(ch, door)->key)))
      return FALSE;
    if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
    {
      do_unlock(ch, arg, 0);
      act("The Lock says 'yummie... I like to eat keys.'\r\nthe Lock looks at the key.\r\nthe Lock grins evily.\r\n\r\n",
         TRUE, ch, obj, 0, TO_ROOM);
      act("The Lock says 'yummie... I like to eat keys.'\r\nthe Lock looks at the key.\r\nthe Lock grins evily.\r\n\r\n",
         TRUE, ch, obj, 0, TO_CHAR);
      act("Hey!. The Lock has eaten $p.\r\n", TRUE, ch, obj, 0, TO_ROOM);
      act("Hey!. The Lock has eaten your key!.\r\n", TRUE, ch, obj, 0, TO_CHAR);
      act("The Lock says 'mo food ..mo food.. I'm still hungry'\r\n", TRUE, ch, obj, 0, TO_ROOM);
      act("The Lock says 'mo food ..mo food.. I'm still hungry'\r\n", TRUE, ch, obj, 0, TO_CHAR);

      extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}

/*
   This function slams the trapdoor closed behind characters who descend
   * from Xexos' Emporium into the Hidden Cellar with the automaton.  The
   * idea is that not only will players have to fight the automaton (who's
   * quite nasty), but they will no longer be able to easily sneak in and out
   * of the cellar with Xexos's goodies.
   * -- DTS 2/21/95
 */

int automaton_trapdoor(int room, P_char ch, int cmd, char *arg)
{
  P_obj    i;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch))
    return FALSE;

  /*
     change all instances of 12158/12159 if v-nums of rooms are changed!
   */
  if (room != real_room(12158))
    return FALSE;

  if (cmd == CMD_DOWN)
  {
    if (!world[real_room(12158)].dir_option[DIR_DOWN])
    {
      logit(LOG_EXIT,
            "Xexos room [%d] does not have down exit! Fix automaton_switch()",
            world[real_room(12158)].number);
      raise(SIGSEGV);
    }
    else
      if (IS_SET
          (world[real_room(12158)].dir_option[DIR_DOWN]->exit_info, EX_CLOSED) ||
          IS_SET(world[real_room(12158)].dir_option[DIR_DOWN]->exit_info,
                 EX_BLOCKED))
    {
      return FALSE;
    }
    else
    {
      /*
         slam the trapdoor
       */
      SET_BIT(world[real_room(12158)].dir_option[DIR_DOWN]->exit_info, EX_CLOSED);
      SET_BIT(world[real_room(12159)].dir_option[DIR_UP]->exit_info, EX_CLOSED);

      /*
         let players know what's happening
       */
      act("The trapdoor swings shut behind you as you descend!", FALSE, ch, 0,
          0, TO_CHAR);
      act("The trapdoor swings shut behind $n as $e descends.", TRUE, ch, 0,
          0, TO_ROOM);

      char_from_room(ch);
      char_to_room(ch, real_room(12159), 0);

      act("The trapdoor swings shut behind $n as $e enters from above.", TRUE,
          ch, 0, 0, TO_ROOM);

      /*
         Now comes the tricky part.  We need to check for the existence of *
         the appropriate switch in the room below (12159).  If the switch  *
         exists, block the exit back up...
       */
      for (i = world[real_room(12159)].contents; i; i = i->next_content)
      {
        if ((i->type == ITEM_SWITCH) && (i->value[1] == 12158))
        {
          SET_BIT(world[real_room(12158)].dir_option[DIR_DOWN]->exit_info,
                  EX_BLOCKED);
          SET_BIT(world[real_room(12159)].dir_option[DIR_UP]->exit_info,
                  EX_BLOCKED);
          act("You hear a metallic clunk from the trapdoor as it closes.",
              FALSE, ch, 0, 0, TO_CHAR);
          act("You hear a faint metallic clunk from the ceiling.", TRUE, ch,
              0, 0, TO_ROOM);
          send_to_room
            ("You hear a faint metallic clunk from underground.\r\n",
             real_room(12158));
          break;
        }
      }
      return TRUE;
    }
  }
  return FALSE;
}

int fw_warning_room(int room, P_char ch, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || (cmd > -40))
    return FALSE;

  act("\r\nAs you enter this section of the forest,\r\n"
      "a shimmering form coalesces into existence!\r\n"
      "The form appears to be that of a wild barbaric man,\r\n"
      "wearing thick furs and wielding a spear inlaid with glowing runes.\r\n"
      "As your body prepares for an attack, a voice sounds inside your head:\r\n"
      "'Puny intruders!  Begone from this wood of mine, a place you do not belong.\r\n"
      "For this is my home, and I care not for trespassers.  You have been WARNED!\r\n'"
      "When the figure has finished speaking, he raises his spear on high,\r\n"
      "and you see a vision of a human head impaled upon it.\r\n"
      "\r\n"
      "Moments later, you gather your senses as you shake off a feeling of dizziness.",
      FALSE, ch, 0, 0, TO_CHAR);

  return TRUE;
}

/*
   this is for player guilds, they die, or recall in without their key, and can't get out again, so, this
   proc traps 'unlock' and magically unlocks the outer door.  This version is specific to the first room
   I'm installing it in, but, like the generic guildguard special, it can be expanded to work anywhere.
   JAB
 */

int keyless_unlock(int room, P_char ch, int cmd, char *arg)
{
  if (!room || !ch || (cmd != CMD_UNLOCK) || !arg || !*arg)
    return FALSE;

  if (room != real_room(9301))
    return FALSE;

  if (!isname("door", arg))
    return FALSE;

  if (IS_SET(world[room].dir_option[DIR_UP]->exit_info, EX_LOCKED))
  {
    send_to_char("You twist the bolt and unlock the door.\r\n", ch);
    act("$n unlocks the door.", TRUE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT(world[room].dir_option[DIR_UP]->exit_info, EX_LOCKED);
  }
  else
  {
    send_to_char
      ("You twist the bolt and realize the door was already unlocked!\r\n",
       ch);
    act("$n fumbles with the door, then looks sheepish.", TRUE, ch, 0, 0,
        TO_ROOM);
  }

  return TRUE;
}

int squid_arena(int room, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_WILL)
  {
    send_to_char
      ("The &+MElder Brain&N stops you from enforcing your will.\n\r", ch);
    return TRUE;
  }
  return FALSE;
}

int duergar_guild(int room, P_char ch, int cmd, char *arg)
{
  P_char   killer;
  hunt_data data;
  int      i;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!room || !ch)
    return FALSE;

  if (IS_NPC(GET_PLYR(ch)) || IS_TRUSTED(ch) || GET_LEVEL(ch) > 10)
    return FALSE;

  if (cmd > CMD_DOWN)
    return FALSE;

  if (number(1, 101) > 25)
    return FALSE;

  if (GET_OPPONENT(ch))
    return FALSE;

  killer = read_mobile(17260, VIRTUAL);

  if (!killer)
    return FALSE;

  killer->player.level = GET_LEVEL(ch) + 1;
  if (GET_LEVEL(killer) > 50)
    killer->player.level = 50;
  killer->player.m_class = CLASS_WARRIOR;
  GET_HIT(killer) = GET_MAX_HIT(killer) = dice(GET_LEVEL(killer), 8);
  GET_DAMROLL(killer) = number(4, 6);
  killer->points.damnodice = 1;
  killer->points.damsizedice = BOUNDED(1, GET_LEVEL(killer) * 2, 20);
  GET_EXP(killer) = 0;
  SET_BIT(killer->specials.act, ACT_BREAK_CHARM);
  REMOVE_BIT(killer->specials.act, ACT_MEMORY);
  killer->only.npc->aggro_flags = killer->only.npc->aggro2_flags = killer->only.npc->aggro3_flags = 0;
  clearMemory(killer);

  for (i = 1; i <= MAX_CIRCLE; i++)
    killer->specials.undead_spell_slots[i] =
      spl_table[GET_LEVEL(killer)][i - 1];

  if (GET_CLASS(killer, CLASS_CLERIC))
  {
    killer->player.m_class = CLASS_WARRIOR;
    data.hunt_type = HUNT_HUNTER;
    data.targ.victim = ch;
    add_event(event_mob_hunt, PULSE_MOB_HUNT, killer, NULL, NULL, 0, &data, sizeof(hunt_data));
    //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, killer, data);
  }
  act("$N jumps from the shadows, surprising you!", FALSE, ch, 0, killer,
      TO_CHAR);
  act("$N jumps from the shadows, surprising $n!", FALSE, ch, 0, killer,
      TO_ROOM);
  char_to_room(killer, ch->in_room, -2);
  if (killer->in_room == ch->in_room)
    MobStartFight(killer, ch);
  return TRUE;
}

int mortal_heaven(int room, P_char ch, int cmd, char *arg)
{
  P_char   tch, next;

  /*
     check for periodic event calls
   */

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_SUICIDE )
  {
    send_to_char("You feel too comfortable here.\n\r", ch );
    return TRUE;
  }

  if( cmd )
  {
    return FALSE;
  }

  for (tch = world[real_room(room)].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if (IS_TRUSTED(tch) || IS_NPC(tch))
      continue;

    if (tch->only.pc->pc_timer[PC_TIMER_HEAVEN] <= time(NULL))
    {
      send_to_char("Your soul is torn from the afterlife, eternal rest denied...\n\r", tch);
      act("$n is torn from the afterlife.", FALSE, tch, 0, 0, TO_ROOM);
      writeCharacter(tch, RENT_DEATH, NOWHERE);
      // If it's not an immortal.
      if( IS_PC(tch) && (GET_LEVEL( tch ) < MINLVLIMMORTAL) )
      {
        update_ingame_racewar( -GET_RACEWAR(tch) );
      }
      extract_char(tch);
      if( !tch->desc )
      {
        free_char(tch);
      }
    }
  }

  return TRUE;
}




int player_council_room(int room, P_char ch, int cmd, char *argument)
{
  int      i;
  P_char   vict;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!cmd || cmd != CMD_SAY)
    return FALSE;

  for (i = 0; *(argument + i) == ' '; i++) ;

  if (!*(argument + i))
    send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
  else
  {
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    {
      if (vict != ch)
      {
        sprintf(Gbuf2, "%s projects '%s'\r\n", GET_NAME(ch), argument + i);
        send_to_char(Gbuf2, vict);
      }
    }


    sprintf(Gbuf1, "You project '%s'\r\n", argument + i);

    send_to_char(Gbuf1, ch);

    return TRUE;
  }

  return FALSE;

}

// Just some sadistic fun with caged people.
int cage_room1(int room, P_char ch, int cmd, char *argument)
{
  static int tries = 0;
  int knob;

  if( cmd == CMD_WIGGLE )
  {
    if( (sscanf(argument, "%d.knob", &knob) == 1) && knob > 0 && knob < 18 )
    {
      if( number( 1, 1000 ) == 666 )
      {
        send_to_char("You finally find the right knob.  As you turn it, you blink and find yourself elsewhere.\n", ch);
        char_from_room(ch);
        if( !char_to_room( ch, real_room(42), -2 ) )
        {
          send_to_char("Wait.. this is the same room!", ch );
          char_to_room( ch, real_room(1134), -2 );
        }
      }
      else
      {
        if( ++tries > number(5, 10) )
        {
          send_to_char( "The knobs seem to move around.\n", ch );
          tries = 1;
        }
        if( tries == 1 )
        {
          send_to_char( "Nope, that knob doesn't seem to work.\n", ch );
        }
        else
        {
          switch( number(1, 20) )
          {
            case  1:
            case  2:
            case  3:
            case  4:
            case  5:
            case  6:
            case  7:
              send_to_char( "That one doesn't work either.\n", ch );
              break;
            case  8:
            case  9:
              send_to_char( "This one shocks you as you touch it!\n", ch );
              break;
            case 10:
            case 11:
              send_to_char( "This knob is really cold.\n", ch );
              break;
            case 12:
            case 13:
            case 14:
              send_to_char( "Something about this knob feels right.\n", ch );
              break;
            case 15:
            case 16:
            case 17:
            case 18:
              send_to_char( "Something about this knob just doesn't feel right.\n", ch );
              break;
            case 19:
            default:
              send_to_char( "Your hand slips off the knob!\n", ch );
              break;
            case 20:
              send_to_char( "This one seems to move a bit.\n", ch );
              break;
          }
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

// Really bad boy tried to escape!
int cage_room2(int room, P_char ch, int cmd, char *argument)
{
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }
  // Randomly eat 1/8th of commands.
  if( ch && cmd != CMD_PERIODIC && (number(1, 8) == 6) )
  {
    return TRUE;
  }
  if( cmd != CMD_PERIODIC )
  {
    return FALSE;
  }
  room = real_room(room);
  if( room < 0 || world[room].people == NULL )
  {
    return FALSE;
  }
  switch( number(1, 40) )
  {
    case  1:
    case  2:
    case  3:
    case  4:
      send_to_room( "&+LYou hear the sounds of &+yfootsteps&+L moving quickly towards you.&n\n", room );
      break;
    case  5:
    case  6:
    case  7:
      send_to_room( "&+LYou hear &+Cpiercing &+Lcries from behind you!&n\n", room );
    case  8:
    case  9:
    case 10:
    case 11:
    case 12:
      send_to_room( "&+LYou hear horrible &+rwailing&+L.&n\n", room );
      break;
    case 13:
    case 14:
      send_to_room( "&+LThe &+wsilence&+L here is unbearable.&n\n", room );
      break;
    case 15:
    case 16:
      send_to_room( "&+LWhen will the &+rscreams&+L stop?!&n\n", room );
      break;
    case 17:
    case 18:
    case 19:
      send_to_room( "&+LYou hear quiet &+bsobs&+L, then realize it's you who's crying.&n\n", room );
      break;
    case 20:
      send_to_room( "&+LYou break into a &+ccold&+L sweat.&n\n", room );
      break;
    default:
      break;
  }
  return TRUE;
}
