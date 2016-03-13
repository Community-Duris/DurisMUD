/**************************************************************************
*  File: specs.highway.c                                    Part of Duris  *
*  Copyright  1996 - Duris Systems Ltd.                                   *
***************************************************************************/

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
#include "structs.h"
#include "utils.h"
#include "specs.prototypes.h"
#include "weather.h"
#include "damage.h"
#include "vnum.obj.h"

/* external variables */

extern P_char char_in_room(int);
extern P_char character_list;
extern P_desc descriptor_list;
extern P_event event_type_list[];
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
extern int racial_base[];
extern const int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

/***** Objects *****/
int hewards_mystical_organ(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   target_mob = NULL;
  P_obj    target_obj = NULL;
  char     arg2[MAX_STRING_LENGTH], arg3[MAX_STRING_LENGTH];
  int      location;
  static int working = FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !obj)
    return FALSE;

  if (cmd == CMD_PERIODIC)
    return FALSE;

  if (GET_POS(ch) != POS_SITTING)
    return FALSE;

  if (arg && (cmd == CMD_PLAY))
    if (isname(arg, "organ"))
    {
      send_to_char
        ("As you begin playing a tune on the organ, a feeling of an almost...\r\n",
         ch);
      send_to_char
        ("astral nature overcomes you. Your eyes begin to tingle, and you\r\n",
         ch);
      send_to_char
        ("realize you have the powers to send your sights anywhere in this world.\r\n",
         ch);
      act
        ("A wondrous, almost magical music begins to pour forth from $p, as $n's hands seemingly float across it.",
         FALSE, ch, obj, obj, TO_ROOM);
      working = TRUE;
      return TRUE;
    }
  if (cmd == CMD_STAND)
    working = FALSE;

  if (arg && (cmd == CMD_LOOK) && working)
  {
    half_chop(arg, arg2, arg3);
    if ((target_mob = get_char_vis(ch, arg2)))
    {
      if (target_mob->in_room != NOWHERE)
      {
        location = target_mob->in_room;
      }
      else
        return FALSE;
    }
    else if ((target_obj = get_obj_vis(ch, arg2)))
    {
      if (OBJ_ROOM(target_obj))
        location = target_obj->loc.room;
      else
        return FALSE;
    }
    else
      return FALSE;
  }
  else
    return FALSE;

  if (!can_enter_room(ch, location, TRUE))
    return FALSE;

  /* a location has been found. */
  new_look(ch, NULL, CMD_LOOKAFAR, location);

  return TRUE;
}

int amethyst_orb(P_obj obj, P_char ch, int cmd, char *arg)
{


  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !obj)
    return FALSE;

  if (cmd == CMD_PERIODIC)
    return FALSE;

  if (!arg)
    return FALSE;

  if (!isname(arg, "orb") && !isname(arg, "amethyst"))
    return FALSE;

  if ((cmd != CMD_STARE) && (cmd != CMD_TOUCH) && (cmd != CMD_RUB) &&
      (cmd != CMD_GLANCE))
    return FALSE;

  if (cmd == CMD_STARE)
  {
    int      curr_time = time(NULL);

    if (!obj->value[0])
    {
      send_to_char("All you receive is static.\r\n", ch);
      return TRUE;
    }
    else if (obj->timer[0] + 30 > curr_time)
    {
      send_to_char("All you receive is a white haze.\r\n", ch);
      return TRUE;
    }
    else
    {
      act("You peer into $p and see...", 0, ch, obj, 0, TO_CHAR);
//              new_look(ch, NULL, CMD_LOOKAFAR, obj->value[0]);
      send_to_char(world[obj->value[0]].name, ch);
      send_to_char("\r\n", ch);
      if (world[obj->value[0]].description)
        send_to_char(world[obj->value[0]].description, ch);
      obj->timer[0] = curr_time;
      return TRUE;
    }

    return FALSE;
  }


  if (cmd == CMD_GLANCE)
  {

    if (!obj->value[0])
    {
      send_to_char("All you receive is static.\r\n", ch);
      return TRUE;
    }
    else
    {
      act("You glance at $p and see...", 0, ch, obj, 0, TO_CHAR);
      send_to_char(world[obj->value[0]].name, ch);
      send_to_char("\r\n", ch);
      return TRUE;
    }

    return FALSE;
  }

  if (cmd == CMD_RUB)
  {
    int      to_room = 0;
    int      safeguard = 0;

    if (IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) && IS_SET(obj->wear_flags, ITEM_TAKE))
      return TRUE;
    
    if (IS_HOMETOWN(ch->in_room))
      return TRUE;
    
    do
    {
      to_room = number(1, top_of_world);
      safeguard++;
    }
    while ((safeguard < 10000) &&
           ((world[to_room].sector_type == SECT_OCEAN) ||
            (world[to_room].zone == 83) ||
            (world[to_room].zone == 257) ||
            (world[to_room].zone == 1) ||
            (world[to_room].zone == 2) ||
            (world[to_room].zone == 4) ||
            (world[to_room].zone == 34) ||
            (world[to_room].zone == 50) ||
            (world[to_room].zone == 100) ||
            (world[to_room].zone == 120) ||
            (world[to_room].zone == 172) ||
            (world[to_room].zone == 173) ||
            (world[to_room].zone == 174) ||
            (world[to_room].zone == 183) ||
            (world[to_room].zone == 184) ||
            (world[to_room].zone == 192) ||
            (world[to_room].zone == 194) ||
            (world[to_room].zone == 198) ||
            IS_SET(world[to_room].room_flags, NO_MAGIC) ||
            IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
            IS_HOMETOWN(to_room)));

    if (safeguard >= 10000)
    {
      send_to_char
        ("Something buggy with amethyst orb proc - tell a coder.\r\n", ch);
      return TRUE;
    }

    obj->value[0] = to_room;
    act("You rub the surface of $p, and it briefly warms up.", FALSE, ch, obj,
        0, TO_CHAR);
    CharWait(ch, PULSE_VIOLENCE * 1 / 2);
    return TRUE;
  }

  if (cmd == CMD_TOUCH)
  {
    int      dam;

    dam = MIN(number(60, 160), GET_HIT(ch));

    if (obj->value[0])
    {
      act
        ("$n fiddles briefly with  $p suddenly disappears in a mushroom cloud.",
         FALSE, ch, obj, 0, TO_ROOM);
      char_from_room(ch);
      send_to_char
        ("As you touch the orb, you feel your body disassemble. After a short blackout, you find yourself elsewhere.\r\n",
         ch);
      char_to_room(ch, obj->value[0], -1);
      //raw_damage(ch, ch, dam, NULL);
      act("$n appears in a bright flash of light.", FALSE, ch, 0, 0, TO_ROOM);
      GET_HIT(ch) -= dam;
      update_pos(ch);
      return TRUE;
    }
    else
    {
      act("$n touches $p and it burns $s skin!", TRUE, ch, obj, 0, TO_ROOM);
      act("You touch $p and burn your skin!", FALSE, ch, obj, 0, TO_CHAR);
      //raw_damage(ch, ch, dam*2, NULL);
      GET_HIT(ch) -= dam;
      update_pos(ch);
      return TRUE;
    }

  }


  return FALSE;

}

int wand_of_wonder(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  P_char   mosquito;
  P_char   buffalo;
  P_char   rat;
  P_char   demon;
  P_obj    gems;
  int      level, i;
  static int gem_types[] = {    /* both low value, and 0 value gems */
    66034, 66035, 66036, 66037, 66038,  /* from Ashrumite */
    66039, 66040, 66041, 66042, 66043
  };

  level = number(20, 40);

  if (!ch || !obj)              /* If the player ain't here, why are we? */
    return FALSE;

  if (!OBJ_WORN(obj))           /* Most things don't work in a sack... */
    return FALSE;

  if ((obj->loc.wearing->equipment[HOLD] == obj) && (obj->value[2] > 0))
    if (cmd == CMD_USE)
    {
      if (!(vict = get_char_vis(ch, arg)))
        vict = ch;
      obj->value[2] -= 1;
      send_to_char("Magic gathers....\r\n", ch);
      switch (number(1, 20))
      {
      case 1:
        spell_minor_paralysis(level, ch, 0, SPELL_TYPE_WAND, vict, 0);
        break;
      case 2:
        spell_cyclone(level, ch, 0, SPELL_TYPE_WAND, vict, 0);
        break;
      case 3:
        spell_lightning_bolt(level, ch, 0, SPELL_TYPE_WAND, vict, 0);
        break;
      case 4:
        spell_darkness(level, ch, 0, SPELL_TYPE_WAND, 0, 0);
        break;
      case 5:
        spell_invisibility(level, ch, 0, SPELL_TYPE_WAND, vict, 0);
        break;
      case 6:
        spell_fireball(level, ch, 0, SPELL_TYPE_WAND, vict, 0);
        break;
      case 7:
        spell_invisibility(level, ch, 0, SPELL_TYPE_WAND, ch, 0);
        break;
      case 8:
        if ((rat = read_mobile(5710, VIRTUAL)))
          char_to_room(rat, ch->in_room, 0);
        break;
      case 9:
        if ((buffalo = read_mobile(5710, VIRTUAL)))
          char_to_room(buffalo, ch->in_room, 0);
        break;
      case 10:
        for (i = 0; i < level + 10; i++)
        {
          if ((mosquito = read_mobile(12803, VIRTUAL)))
            char_to_room(mosquito, ch->in_room, 0);
        }
        break;
      case 11:
        if ((demon = read_mobile(97514, VIRTUAL)))
          char_to_room(demon, ch->in_room, 0);
        break;
      case 12:
        for (i = 0; i < number(10, 40); i++)
        {
          gems = read_object(gem_types[number(0, 9)], VIRTUAL);
          if (!gems)
          {
            send_to_char("error: couldn't load gems..  tell god\r\n", ch);
          }
          else
          {
            REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
            obj_to_room(gems, ch->in_room);
          }
        }

        if (!damage(ch, vict, i, TYPE_UNDEFINED))       /* one hp per gem */
        {
          act("Gems shoot forth from $n's wand, striking you in the head!",
              TRUE, ch, 0, 0, TO_ROOM);
        }
        else
          return TRUE;

        break;
      case 13:
        send_to_room("&+GGrass begins growing under your feet.\r\n",
                     ch->in_room);
        break;
      case 14:
        send_to_room("&+WA thick fog shoots forth from the wand.\r\n",
                     ch->in_room);
        break;
      case 15:
      case 16:
        AgeChar(ch, number(1, 50));
        send_to_char
          ("Time seems to have caught up with you, aging you considerably.\r\n",
           ch);
        break;
      default:
        send_to_char("Nothing seems to happen.", ch);
        break;
      }
      return TRUE;
    }
  return FALSE;
}

int breale_townsfolk(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   TmpCh, next;
  char     TmpBuf[MAX_STRING_LENGTH];

  /* Check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || IS_FIGHTING(ch) || !IS_AWAKE(ch))
    return FALSE;

  for (TmpCh = world[ch->in_room].people; TmpCh && !IS_FIGHTING(ch);
       TmpCh = next)
  {
    next = TmpCh->next_in_room;

    if ((GET_CLASS(TmpCh, CLASS_SHAMAN) ||
         GET_CLASS(TmpCh, CLASS_SORCERER) ||
         GET_CLASS(TmpCh, CLASS_NECROMANCER) ||
         GET_CLASS(TmpCh, CLASS_SUMMONER) ||
         GET_CLASS(TmpCh, CLASS_CONJURER)) &&
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

// This proc isn't in game atm, and is found in specs.myranth.c as well?
int blackness_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      ankh_loc, t_val;
  char     Buf1[MAX_STRING_LENGTH], Buf2[MAX_STRING_LENGTH];
  bool     got_it;
  P_obj    ankh = NULL, sword;

  /* check for periodic event calls */
  if( cmd == CMD_SET_PERIODIC )
    return FALSE;

  if( (cmd != CMD_GET) && (cmd != CMD_TAKE) )
    return FALSE;

  if( !obj || !ch || !arg || !OBJ_ROOM(obj) )
    return FALSE;

  ankh_loc = WEAR_NONE;
  if( ch->equipment[WEAR_NECK_1] )
    if( OBJ_VNUM(ch->equipment[WEAR_NECK_1]) == VOBJ_HIGHWAY_ANKH_BLACKSWORD )
    {
      ankh_loc = WEAR_NECK_1;
      ankh = ch->equipment[WEAR_NECK_1];
    }
  if( ankh_loc == WEAR_NONE && ch->equipment[WEAR_NECK_2] )
    if( OBJ_VNUM(ch->equipment[WEAR_NECK_2]) == VOBJ_HIGHWAY_ANKH_BLACKSWORD )
    {
      ankh_loc = WEAR_NECK_2;
      ankh = ch->equipment[WEAR_NECK_2];
    }
  if( ankh_loc == WEAR_NONE )
    return FALSE;

  argument_interpreter(arg, Buf1, Buf2);

  if( *Buf2 )
    return FALSE;

  // If we're trying to 'get|take all' or 'get|take all.<sword keyword>'
  got_it = FALSE;
  if( !str_cmp(Buf1, "all") )
    got_it = TRUE;
  else if( (sscanf(Buf1, "all.%s", Buf2) == 1) && isname(Buf2, obj->name) )
    got_it = TRUE;
  else
  {
    if( isname(Buf1, obj->name) || ((sscanf(Buf1, "%d.%s", &t_val, Buf2) == 2) && isname(Buf2, obj->name)))
    {
      if( t_val < 1)
      {
        return FALSE;
      }
      for( sword = world[obj->loc.room].contents; sword; sword = sword->next_content )
      {
        if( isname(Buf2, sword->name) )
          t_val--;
        if( t_val <= 0 )
        {
          if( sword == obj )
          {
            got_it = TRUE;
          }
          break;
        }
      }
    }
  }

  if( !got_it )
    return FALSE;

  // If it's already flagged takeable.
  if( IS_SET(obj->wear_flags, ITEM_TAKE) )
    return FALSE;

  // Otherwise make it takeable and not cursed.
  REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
  SET_BIT(obj->wear_flags, ITEM_TAKE);

  // Ankh only works once, for whatever command. . . .
  unequip_char(ch, ankh_loc );
  extract_obj(ankh, TRUE); // Should poof if arti.
  send_to_char("The ankh around your neck disappears with a hollow pop.\n\n", ch);
  // Actually get the sword.
  obj_from_room(obj);
  obj_to_char(obj, ch);
  act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);

  return TRUE;
}

int kearonor_hide(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      mob_num = 0;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !obj )
    return FALSE;

  if( cmd && cmd == CMD_SHAPECHANGE )
  {
    act("Ye cannot remove the $q! It must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
    return TRUE;
  }
  if( !IS_MORPH(ch) && OBJ_WORN(obj) && IS_PC(ch) )
  {
    if( (mob_num = real_mobile0(41302)) == 0 )
    {
      logit(LOG_DEBUG, "Beast vnum non-existant for kearonor_hide proc.\r\n");
      return FALSE;
    }
    send_to_char("You feel pain wrack your body, as a strange metamorphasis engulfs you.\r\n", ch);

    if( morph(ch, mob_num, REAL) )
    {
      act("$N screams in pain as $S body begins to re-form into $n.", FALSE, ch->only.pc->switched, 0, ch, TO_ROOM);
    }
    return TRUE;
  }
  return FALSE;
}

int mir_spider(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    web = NULL, room_junk, room_junk_temp;

  /* check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  for (room_junk = world[ch->in_room].contents; room_junk;
       room_junk = room_junk_temp)
  {
    room_junk_temp = room_junk->next_content;
    if ((obj_index[room_junk->R_num].virtual_number == 41900) ||
        (obj_index[room_junk->R_num].virtual_number == 41901))
      return FALSE;
  }
  switch (number(0, 10))
  {
  case 1:
    web = read_object(41900, VIRTUAL);
    if (!web)
    {
      logit(LOG_EXIT, "assert: mir_spider() failed to load object 41900");
      raise(SIGSEGV);
    }
    obj_to_room(web, ch->in_room);
    act("$n &nspins a wonderous web across the thick trees.", 1, ch, 0, 0,
        TO_ROOM);
    return TRUE;
  case 2:
    web = read_object(41901, VIRTUAL);
    if (!web)
    {
      logit(LOG_EXIT, "assert: mir_spider() failed to load object 41901");
      raise(SIGSEGV);
    }
    obj_to_room(web, ch->in_room);
    act("$n &nbegins to spin a web.", 1, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

int mir_fire(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      i, room, factor;
  P_obj    room_junk = NULL, room_junk_temp = NULL, smoke;
  P_char   tch = NULL, next = NULL;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( !obj || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  factor = dice(3, 6);
  /* burn anyone in the room */
  if (world[obj->loc.room].people)
  {
    for (tch = world[obj->loc.room].people; tch; tch = next)
    {
      next = tch->next_in_room;
      if( NewSaves(tch, SAVING_SPELL, IS_AFFECTED2(tch, AFF2_FIRESHIELD) ? -2 : 0) )
        factor >>= 1;
      if( IS_AFFECTED2(tch, AFF2_FIRESHIELD) )
        factor <<= 1;
      if( IS_AFFECTED3(tch, AFF3_COLDSHIELD) )
        factor >>= 1;
      if( IS_AFFECTED(tch, AFF_PROT_FIRE) )
        factor >>= 1;
      if( !IS_TRUSTED(tch) )
      {
        GET_HIT(tch) -= MAX(1, MIN(GET_HIT(tch) - 5, factor));
      }
      send_to_char("The &+Yraging fire&n burns intensely!\r\n", tch);
    }
    return TRUE;
  }
  /* spread like wildfire */
  for( i = 0; i < NUM_EXITS; i++ )
  {
    if( VIRTUAL_CAN_GO(obj->loc.room, i) )
    {
      room = VIRTUAL_EXIT(obj->loc.room, i)->to_room;
      for( room_junk = world[room].contents; room_junk; room_junk = room_junk_temp )
      {
        room_junk_temp = room_junk->next_content;

        if( (obj_index[room_junk->R_num].virtual_number == 41900) ||
          (obj_index[room_junk->R_num].virtual_number == 41901) )
        {
          smoke = read_object(41908, VIRTUAL);
          if( !smoke )
          {
            logit(LOG_EXIT, "assert: mir_fire() failed to load object 41908");
            raise(SIGSEGV);
          }
          add_event(event_smoke_to_fire, number(1, 5), NULL, NULL, NULL, 0, &room, sizeof(room));
          //AddEvent(EVENT_SPECIAL, number(1, 5), TRUE, smoke_to_fire, room);
          obj_to_room(smoke, room);
          break;
        }
      }
    }
  }
  return FALSE;
}

void web_to_smoke(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      room;
  P_obj    room_junk = NULL, room_junk_temp = NULL, smoke, web;

  if (!current_event || (current_event->type != EVENT_SPECIAL))
  {
    logit(LOG_EXIT, "Call to web_to_smoke() with invalid event");
    raise(SIGSEGV);
  }
  /* fathom our location */
  room = atoi((char *) current_event->target.t_arg);
  if (room <= 0)
  {
    logit(LOG_EXIT, "web_to_smoke() in invalid room.");
    raise(SIGSEGV);
  }
  /* double check for the web, and replace with smoke if its there */
  for (room_junk = world[room].contents; room_junk;
       room_junk = room_junk_temp)
  {
    room_junk_temp = room_junk->next_content;
    if ((obj_index[room_junk->R_num].virtual_number == 41900) ||
        (obj_index[room_junk->R_num].virtual_number == 41901))
    {
      web = room_junk;
      smoke = read_object(41908, VIRTUAL);
      if (!smoke)
      {
        logit(LOG_EXIT, "assert: web_to_smoke() failed to load object 41908");
        raise(SIGSEGV);
      }
      send_to_room
        ("The spider webs begin to smolder, and just might burst into flames at any moment.\r\n",
         room);
      obj_from_room(web);
      obj_to_room(smoke, room);
      return;
    }
  }
  return;
}

void event_smoke_to_fire(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      room;
  P_obj    room_junk = NULL, room_junk_temp = NULL, smoke, fire;

  /* fathom our location */
  room = *((int*)data);
  if (room <= 0)
  {
    logit(LOG_EXIT, "smoke_to_fire() in invalid room.");
    raise(SIGSEGV);
  }
  /* double check for the smoke, and replace with fire if its there */
  for (room_junk = world[room].contents; room_junk;
       room_junk = room_junk_temp)
  {
    room_junk_temp = room_junk->next_content;
    if (obj_index[room_junk->R_num].virtual_number == 41908)
    {
      smoke = room_junk;
      fire = read_object(41907, VIRTUAL);
      if (!fire)
      {
        logit(LOG_EXIT,
              "assert: smoke_to_fire() failed to load object 41907");
        raise(SIGSEGV);
      }
      send_to_room("The smoldering webs burst into flame!\r\n", room);
      obj_from_room(smoke);
      obj_to_room(fire, room);
      return;
    }
  }
  return;
}

int red_wyrm_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 42173, 42174, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100, "&+rROARRRRRRRRRR!", NULL, helpers, 0, 0);
  return FALSE;
}

int blue_wyrm_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 42172, 42173, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100, "&+bROARRRRRRRRRR!", NULL, helpers, 0, 0);
  return FALSE;
}

int white_wyrm_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 42172, 42174, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100, "&+WROARRRRRRRRRR!", NULL, helpers, 0, 0);
  return FALSE;
}

int amphisbean(P_char ch, P_char tch, int cmd, char *arg)
{

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!IS_FIGHTING(ch))
    return FALSE;

  if (!ch->specials.fighting)
    return FALSE;

  if (!number(0, 9))
  {
    act("$n &+GBLURS &+gwith the speed of light...&n", FALSE, ch, 0, 0,
        TO_ROOM);
    spell_greater_pythonsting(60, ch, NULL, SPELL_TYPE_SPELL, 0, 0);
    return TRUE;
  }

  return FALSE;

}
