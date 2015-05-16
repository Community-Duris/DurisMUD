
/*
 * ***************************************************************************
 * *  File: actoth.c                                           Part of Duris *
 * *  Usage: misc. command routines
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 * * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "sql.h"
#include "db.h"
#include "epic.h"
#include "events.h"
#include "interp.h"
#include "justice.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "damage.h"
#include "map.h"
#include "specializations.h"
#include "guard.h"
#include "specs.winterhaven.h"
#include "guildhall.h"
#include "achievements.h"

/*
 * external variables
 */

extern const struct class_names class_names_table[];

extern const char *target_locs[];
extern P_desc descriptor_list;
extern P_event current_event;
extern P_room world;
extern P_index mob_index;
extern bool command_confirm;
extern char msg_of_today[];
extern const char *coin_names[];
extern const char *dirs[];
extern const char *sector_types[];
extern const char *shot_types[];
extern const int shot_damage[];
extern const struct stat_data stat_factor[];
extern int forced_command;
extern int rev_dir[];
extern long reboot_time;
extern struct agi_app_type agi_app[];
extern struct command_info cmd_info[];
extern struct dex_app_type dex_app[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern struct time_info_data time_info;
extern int top_of_world;
extern P_index obj_index;
extern char *specdata[][MAX_SPEC];
extern P_char character_list;
extern Skill skills[];
extern const int new_exp_table[];


/*
 * allow players to 'set up camp' and rent out in the wilderness.  JAB
 */

void do_terrain(P_char ch, char *arg, int cmd)
{
  char     out[MAX_STRING_LENGTH];

  if (!ch)
    return;

  sprintf(out, "Your current terrain is:  %s.\r\n",
          sector_types[world[ch->in_room].sector_type]);
  send_to_char(out, ch);
}

// Actual multiclass code found in: specs.room.c in multiclass_proc.
//  This function just tells ch what he can multi as if anything.
void do_multiclass(P_char ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int found_one = FALSE, i;
  int min_level = get_property("multiclass.level.req.min", 41);

  if( IS_MULTICLASS_PC(ch) )
  {
    send_to_char("You've already chosen your secondary class..  it's too late now to change your mind.\r\n", ch);
    return;
  }

  if( IS_NPC(ch) )
  {
    send_to_char("No NPCs allowed.\r\n", ch);
    return;
  }

  if( GET_RACE(ch) != RACE_ORC && GET_RACE(ch) != RACE_HUMAN )
  {
    send_to_char("Your race is not versatile enough to learn that much.\n", ch);
    return;
  }

  if( GET_LEVEL(ch) < min_level )
  {
    sprintf(buf, "You cannot multiclass until you reach level %d.\r\nHowever, here is a list of your future choices:\r\n\r\n", min_level);
    send_to_char(buf, ch);
  }
  else if( cmd != -1 )           // indicates called from proc with no arg..  aren't i nice
  {
    send_to_char("You are not in the proper location to multiclass.\r\n"
      "However, here is a list of your possible choices:\r\n\r\n", ch);
  }

  for (i = 1; i <= CLASS_COUNT; i++)
  {
    if( can_char_multi_to_class(ch, i ) )
    {
      char     strn[2048];

      sprintf(strn, "   %s\r\n", (class_names_table[i].ansi));

      send_to_char(strn, ch);

      found_one = TRUE;
    }
  }

  if( !found_one )
    send_to_char("   None (don't despair, you can always specialize)\r\n", ch);

  send_to_char("\r\n\r\n", ch);
}

/* used so chars can see what their color codes will look like before actually slapping them on a ship or whatnot */

void do_testcolor(P_char ch, char *arg, int cmd)
{
  if (*arg == '\0')
  {
    send_to_char
      ("Specify the string to test as the argument.  You probably want to put color codes in it ('help ansi').\r\n",
       ch);
    return;
  }

  send_to_char(arg, ch);
  send_to_char("\r\n", ch);
}

/* valid targets are "head", "arms", "legs", "body" */
void do_target(P_char ch, char *arg, int cmd)
{
  int      loc;
  char     buf[256];

  if( GET_CHAR_SKILL(ch, SKILL_BATTLE_ORDERS) )
  {
    do_battle_orders(ch, arg, cmd);
    return;
  }
  else
  {
    send_to_char("You do not have the battle orders skill.\n\r", ch );
    send_to_char("We are not using body-part targetting battles atm...\n\r", ch);
    return;
  }

  if (*arg == '\0')
  {                             /* no arg */
    send_to_char("Valid target areas are:\n\r", ch);
    loc = 0;
    while (target_locs[loc][0] != '\n')
    {
      sprintf(buf, "    %s\n\r", target_locs[loc]);
      send_to_char(buf, ch);
      loc++;
    }
    return;
  }
  loc = search_block(arg, target_locs, FALSE);
  if (loc == -1)
  {
    send_to_char("Type &+Wtarget&N to see valid options.", ch);
    return;
  }
  send_to_char("You change your combat tactics slightly.\n\r", ch);
  return;
}

#if 0
/* for ordering followers to target */
void do_order_target(P_char ch, P_char vict, char *arg, int cmd)
{
  int      loc;
  char     buf[256];

  if (!ch || !vict)
    return;

  if (*arg == '\0')
  {                             /* no arg */
    send_to_char("Valid target areas are:\n\r", ch);
    loc = 0;
    while (target_locs[loc][0] != '\n')
    {
      sprintf(buf, "    %s\n\r", target_locs[loc]);
      send_to_char(buf, ch);
      loc++;
    }
    return;
  }
  loc = search_block(arg, target_locs, FALSE);
  if (loc == -1)
  {
    send_to_char("Type &+Wtarget&N to see valid options.", ch);
    return;
  }
  vict->player.combat_target_loc = loc;
  sprintf(buf, "%s changes their combat tactics slightly.\r\n", J_NAME(vict));
  send_to_char(buf, ch);
  return;
}
#endif





void do_camp(P_char ch, char *arg, int cmd)
{
  struct affected_type af;
  // old guildhalls (deprecated)
//  P_house house;
  if (!SanityCheck(ch, "do_camp"))
    return;

  if (IS_NPC(ch))
  {
    send_to_char("Ok, you are now a happy camper!\r\n", ch);
    return;
  }
  // old guildhalls (deprecated)
//  house = house_ch_is_in(ch);
//  if (house)
//      {
//      send_to_char("You cannot camp in a hall. Use inn like everyone else!\r\n", ch);
//      return;
//      }
  while (*arg == ' ')
    arg++;

  if( isname( arg, "abort" ) )
  {
    if( !IS_AFFECTED(ch, AFF_CAMPING) )
    {
      send_to_char( "You're not setting up camp atm?!?\n", ch );
    }
    else
    {
      send_to_char( "You quickly pack up your things and move on.\n", ch );
      affect_from_char(ch, SKILL_CAMP);
    }
    return;
  }

  if (IS_AFFECTED2(ch, AFF2_SCRIBING))
  {
    send_to_char("Sorry, you're quite busy with your scribing right now..\\r\n", ch);
    return;
  }
  if (IS_AFFECTED2(ch, AFF2_MEMORIZING))
  {
    send_to_char("Sorry, you're quite busy memorizing at the moment.\n", ch);
    return;
  }
  if( IS_FIGHTING(ch) )
  {
    act("Better finish dealing with $N first, bunky.", FALSE, ch, 0, ch->specials.fighting, TO_CHAR);
    return;
  }
  if( IS_DESTROYING(ch) )
  {
    act("Better finish dealing with $p first, bunky.", FALSE, ch, ch->specials.destroying_obj, NULL, TO_CHAR);
    return;
  }

  if( IS_TRUSTED(ch) )
  {
    ch->specials.was_in_room = world[ch->in_room].number;
    ch->in_room = ch->in_room;
    if (ch->desc && ch->desc->snoop.snooping)
    {
      send_to_char("You stop your snoop.\r\n", ch);
      rem_char_from_snoopby_list(&ch->desc->snoop.snooping->desc->snoop.
                                 snoop_by_list, ch->desc->character);
      ch->desc->snoop.snooping = 0;
    }
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);

    writeCharacter(ch, 3, ch->in_room);
    extract_char(ch);
    ch = NULL;
    return;
  }
  /* no loot/camp */
  if (IS_AFFECTED4(ch, AFF4_LOOTER))
  {
    send_to_char
      ("Aw, play fair. Give your last victim a few minutes to even the score before you whimper off, hiding like a dog.\r\n",
       ch);
    return;
  }
  if (IS_AFFECTED2(ch, AFF2_FLURRY))
  {
    send_to_char("You are too deep in battle madness!\r\n", ch);
    return;
  }

  /*
   * check out the general terrain
   */

  if(IS_WATER_ROOM(ch->in_room))
  {
    send_to_char("I've got just three words: Davy Jones's Locker.\r\n", ch);
    return;
  }

  if( world[ch->in_room].sector_type == SECT_FIREPLANE
    || world[ch->in_room].sector_type == SECT_WATER_PLANE
    || world[ch->in_room].sector_type == SECT_AIR_PLANE
    || world[ch->in_room].sector_type == SECT_EARTH_PLANE )
  {
    send_to_char("Camping here is not permitted.\r\n", ch);
    return;
  }

  switch( world[ch->in_room].sector_type )
  {
  case SECT_CITY:
  case SECT_UNDRWLD_CITY:
  case SECT_ROAD:
  case SECT_CASTLE_WALL:
  case SECT_CASTLE_GATE:
  case SECT_CASTLE:
    send_to_char
      ("Riiight, you'd get run over by a cart, or knifed in your sleep!  Go to an Inn!\r\n",
       ch);
    return;
    break;
  case SECT_SWAMP:
  case SECT_UNDRWLD_SLIME:
    send_to_char
      ("It's just a tad too wet to camp here, try to find a dry spot eh?\r\n",
       ch);
    return;
    break;
  case SECT_NO_GROUND:
  case SECT_UNDRWLD_NOGROUND:
    send_to_char
      ("The price of rolling over in bed is little high here.\r\n", ch);
    return;
    break;
  case SECT_UNDRWLD_LIQMITH:
  case SECT_LAVA:
    send_to_char("Its FAR too HOT to camp here!\r\n", ch);
    return;
    break;
  case SECT_NEG_PLANE:
  case SECT_PLANE_OF_AVERNUS:
    send_to_char("Where exactly are you going to set up camp here and feel safe?\r\n", ch);
    return;
    break;
  case SECT_FIELD:
  case SECT_FOREST:
  case SECT_HILLS:
  case SECT_MOUNTAIN:
  case SECT_UNDRWLD_WILD:
  case SECT_INSIDE:
  case SECT_UNDRWLD_INSIDE:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_UNDRWLD_MUSHROOM:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_SNOWY_FOREST:
  case SECT_ARCTIC:
  case SECT_DESERT:
    /*
     * these 5 are ok to camp in (barring other factors)
     */
    break;
  default:
    logit(LOG_DEBUG, "Bogus sector_type (%d) in do_camp",
          world[ch->in_room].sector_type);
    send_to_char("How strange!  This terrain doesn't seem to exist!\r\n", ch);
    return;
    break;
  }

  if (ch->specials.z_cord < 0)
  {
    send_to_char("I've got just three words: Davy Jones' Locker.\r\n", ch);
    return;
  }
  else if (ch->specials.z_cord > 0)
  {
    send_to_char
      ("The price of rolling over in bed is little high here.\r\n", ch);
    return;
  }
  /*
   * ok, terrain is good, let's see if there are any extenuating
   * circumstances (SINGLE_FILE, etc)
   */

  if (IS_SET(world[ch->in_room].room_flags, TUNNEL) ||
      IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    send_to_char("It's a little too cramped to camp in here.\r\n", ch);
    return;
  }
  if (IS_SET(world[ch->in_room].room_flags, UNDERWATER))
  {
    send_to_char("I've got just three words: Davy Jones' Locker.\r\n", ch);
    return;
  }
  if (IS_SET(world[ch->in_room].room_flags, JAIL))
  {
    send_to_char("Just relax Jailbird, you are gonna be here for a while.\r\n", ch);
    return;
  }
  if (IS_SET(world[ch->in_room].room_flags, GUILD_ROOM))
  {
    send_to_char("You're not allowed to camp here!\r\n", ch);
    return;
  }
  if (IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_TOWN))
  {
    send_to_char("Riiight, you'd get run over by a cart, or knifed in your sleep!  Go to an Inn!\r\n", ch);
    return;
  }
  if (IS_STUNNED(ch))
  {
    send_to_char("You're too stunned to think of camping!\r\n", ch);
    return;
  }
  if (IS_SET(ch->specials.affected_by, AFF_KNOCKED_OUT))
  {
    send_to_char("Being knocked out, camping is not really an option for you.\r\n", ch);
    return;
  }
  if (GET_HIT(ch) < 0)
  {
    send_to_char("You're bleeding too much to camp.\r\n", ch);
    return;
  }
  /*
   * all is right with the world, and we have a decent camping spot,
   * let's set it up.
   */

  if (IS_AFFECTED(ch, AFF_CAMPING))
  {
    struct affected_type *next;
    struct affected_type *aff;

    for (aff = ch->affected; aff; aff = next)
    {
      next = aff->next;

      if (aff->type == SKILL_CAMP)
      {
        break;
      }
    }

  	if (aff)
  	{
  		char buf[100];
  		int i = 0;
  		int j = 0;

      i = aff->duration;

      i = i*100;

      j = get_property("camp.timer", 9.00);

      i = (int) (i/j);

	    if (i > 80)
	      send_to_char("You have just begun your preparations.\r\n", ch);
  	  else
	    if (i > 60)
	      send_to_char("Your preparations are not quite complete.\r\n", ch);
	    else
	    if (i > 40)
	      send_to_char("Your camping preparations are halfway complete.\r\n", ch);
	    else
	    if (i > 20)
	      send_to_char("Your camping preparations should soon be finished.\r\n", ch);
	    else
	      send_to_char("You should finish your preparations anytime now!\r\n", ch);
	  }
	  else
	  {
      send_to_char("Your preparations are not quite complete...\r\n", ch);
	  }
	  return;
  }

  if (!RACE_PUNDEAD(ch))
  {
    send_to_char("You start setting up camp...\r\n", ch);
    act("$n begins to set up camp..", FALSE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    send_to_char("You start digging a shallow grave...\r\n", ch);
    act("$n begins digging a shallow grave..", FALSE, ch, 0, 0, TO_ROOM);
  }
  /*
   * really stretching the limits of affected_type, hope it doesn't
   * explode!
   */

  bzero(&af, sizeof(af));
  af.type = SKILL_CAMP;
  /*
   * this is short_affect_update time, about 140 seconds
   */
  af.duration = IS_TRUSTED(ch) ? 0 : 9;
  af.modifier = (int) ch->in_room;
  af.bitvector = AFF_CAMPING;
  affect_to_char(ch, &af);
}

void berserk(P_char ch, int duration)
{
  struct affected_type af;

  if(affected_by_spell(ch, SKILL_BERSERK))
    return;

  act("$n fills with &+rBloodLust&N", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char("Your instincts force you into a &+rBloodLust&N!\r\n", ch);

  memset(&af, 0, sizeof(af));
  af.type = SKILL_BERSERK;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
  af.duration = duration;

  if (GET_CLASS(ch, CLASS_BERSERKER))
    af.modifier = (ch->base_stats.Str * 1 / 7) + number(-5, 5);
  else
    af.modifier = (ch->base_stats.Str * 1 / 10) + number(-5, 5);

  af.location = APPLY_STR_MAX;
  affect_to_char(ch, &af);
  
  if(GET_CLASS(ch, CLASS_BERSERKER) &&
    (GET_RACE(ch) == RACE_MOUNTAIN ||
     GET_RACE(ch) == RACE_DUERGAR))
        af.modifier = ch->base_stats.Con * 3 / 2 + number(-5, 5);
  else
    af.modifier = ch->base_stats.Con + number(-5, 5);

  af.flags |= AFFTYPE_NOMSG;
  af.location = APPLY_CON_MAX;
  affect_to_char(ch, &af);

  if(!GET_CLASS(ch, CLASS_BERSERKER))
  {
  af.modifier = -(ch->base_stats.Int * 2 / 3);
  af.location = APPLY_INT;
  affect_to_char(ch, &af);
  }
  
  if(GET_CLASS(ch, CLASS_BERSERKER) &&
    (GET_RACE(ch) == RACE_MOUNTAIN ||
     GET_RACE(ch) == RACE_DUERGAR))
        af.modifier = -(ch->base_stats.Wis * 1 / 5);
  else
    af.modifier = -(ch->base_stats.Wis * 2 / 3);

  af.location = APPLY_WIS;
  affect_to_char(ch, &af);
  
  if(GET_CLASS(ch, CLASS_BERSERKER) &&
    (GET_RACE(ch) == RACE_MOUNTAIN ||
     GET_RACE(ch) == RACE_DUERGAR))
  {
    if(GET_CHAR_SKILL(ch, SKILL_BERSERK) == 100 &&
       GET_CHAR_SKILL(ch, SKILL_INDOMITABLE_RAGE) == 100)
        af.modifier = -2;
    else
      af.modifier = -1;
  }
  else if(!GET_CLASS(ch, CLASS_BERSERKER))
    af.modifier = -1;
  else
    af.modifier = 0;
  
  af.location = APPLY_COMBAT_PULSE;
  affect_to_char(ch, &af);

  if(GET_CLASS(ch, CLASS_BERSERKER) &&
    (GET_RACE(ch) == RACE_MOUNTAIN ||
     GET_RACE(ch) == RACE_DUERGAR))
        af.modifier = ch->base_stats.Pow + number(-5, 5);
  else if(!GET_CLASS(ch, CLASS_BERSERKER))
    af.modifier = (ch->base_stats.Pow / 2) + number(-5, 5);
  else 
    af.modifier = 0;
    
  af.location = APPLY_POW_MAX;
  affect_to_char(ch, &af);
  
  if(GET_CLASS(ch, CLASS_BERSERKER) &&
    (GET_RACE(ch) == RACE_MOUNTAIN ||
     GET_RACE(ch) == RACE_DUERGAR))
  {
    af.modifier = GET_LEVEL(ch) * 4;
    af.location = APPLY_HIT;
    affect_to_char(ch, &af);
  }
}

void do_berserk(P_char ch, char *argument, int cmd)
{
  int      duration;

  if(GET_CHAR_SKILL(ch, SKILL_BERSERK) < 1)
  {
    send_to_char("You are already crazy, go away.\r\n", ch);
    return;
  }

  if (affected_by_spell(ch, SKILL_BERSERK))
  {
    if (GET_CHAR_SKILL(ch, SKILL_BERSERK) < number(0, 100))
    {
      send_to_char("You are too deep in battle madness!\r\n", ch);
      return;
    }
    else if( IS_PC(ch) && affected_by_spell(ch, TAG_PVPDELAY) && GET_HIT(ch) < (GET_MAX_HIT(ch) * 0.30) )
    {
      send_to_char("Your &+rwounds&n are severe, you taste &+Rblood&n and &+ysweat&n, thus coming out of your &+Rbloodlust&n is impossible!\r\n", ch);
      return;
    }

    affect_from_char(ch, SKILL_BERSERK);

    send_to_char("Your blood cools, and you no longer see targets everywhere.\r\n", ch);
    act("$n seems to have overcome $s battle madness.", TRUE, ch, 0, 0, TO_ROOM);

    notch_skill(ch, SKILL_BERSERK, 4);

    if (GET_CLASS(ch, CLASS_BERSERKER))
    {
      CharWait(ch, 1 * PULSE_VIOLENCE);
    }
    else
    {
      CharWait(ch, 2 * PULSE_VIOLENCE);
    }
    return;
  }

  if (GET_CHAR_SKILL(ch, SKILL_BERSERK) < number(1, 100))
  {
    send_to_char("You fail to evoke the dreaded battle rage.\r\n", ch);
    notch_skill(ch, SKILL_BERSERK, 17);
  }
  else
  {
    duration = 5 * (MAX(25, (GET_CHAR_SKILL(ch, SKILL_BERSERK) + GET_LEVEL(ch))));
    
    if(GET_CLASS(ch, CLASS_BERSERKER) ||
       GET_RACE(ch) == RACE_MOUNTAIN ||
       GET_RACE(ch) == RACE_DUERGAR)
          duration *= 4;
    
    berserk(ch, duration);
  }

  if (GET_CLASS(ch, CLASS_BERSERKER))
  {
    if (GET_LEVEL(ch) < 10)
    {
      CharWait(ch, 2 * PULSE_VIOLENCE);
    }
    else if (GET_LEVEL(ch) < 20)
    {
      CharWait(ch, 1 * PULSE_VIOLENCE);
    }
    else if (GET_LEVEL(ch) >= 20)
    {
      CharWait(ch, 2);
    }
  }
/* Let's see if berserk is used more often other than
 * in zones with this tweak. -Lucrot
 */
  else if(GET_CLASS(ch, CLASS_WARRIOR) &&
           GET_LEVEL(ch) >= 51)
    CharWait(ch, 2);
  else
    CharWait(ch, 2 * PULSE_VIOLENCE);
}

void do_rampage(P_char ch, char *argument, int cmd)
{
  struct affected_type af;
  int      skl_lvl;
  int      percent;

  if (!affected_by_spell(ch, SKILL_BERSERK))
  {
    send_to_char("You are not quite angry enough..\r\n", ch);
    return;
  }
  if (affected_by_spell(ch, SPELL_HASTE) && GET_LEVEL(ch) < 56)
  {
    send_to_char("You already have a good rush going.\r\n", ch);
    return;
  }

  if (affected_by_spell(ch, SPELL_BLUR) && GET_LEVEL(ch) >= 56)
  {
    send_to_char("You already have a good rush going.\r\n", ch);
    return;
  }

  if (!CAN_ACT(ch))
  {
    return;
  }

  skl_lvl = GET_CHAR_SKILL(ch, SKILL_RAMPAGE);
  percent = number(1, 110);
  if (percent > skl_lvl)
  {
    act("Your blood boils for a second, and then the feeling passes.\r\n",
        FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  else
  {

    act("You can't seem to stay in &+Wcontrol&n..\n", FALSE, ch, 0, 0,
        TO_CHAR);
    send_to_char
      ("You lose all &+Wself control, &+ryour blood rushes fiercely!\r\n",
       ch);
    act("$n loses &+Wcontrol&n, &+rveins bulging with anger.\r\n", FALSE,
        ch, 0, 0, TO_ROOM);


    bzero(&af, sizeof(af));
    if (GET_LEVEL(ch) > 55)
    {
      af.type = SPELL_BLUR;
      af.bitvector3 = AFF3_BLUR;
    }
    else
    {
      af.type = SPELL_HASTE;
      af.bitvector = AFF_HASTE;
    }
    af.duration = 3;

    affect_to_char(ch, &af);
    notch_skill(ch, SKILL_RAMPAGE, 17);
    CharWait(ch, 8);

    return;
  }
}

void do_infuriate(P_char ch, char *argument, int cmd)
{
  struct affected_type af;
  P_char victim;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;

  if (!affected_by_spell(ch, SKILL_BERSERK))
  {
    send_to_char("You are not quite angry enough..\r\n", ch);
    return;
  }
  if (!GET_CHAR_SKILL(ch, SKILL_INFURIATE))
  {
    send_to_char("You don't know how!\r\n", ch);
    return;
  }

  if(!CAN_ACT(ch) ||
     IS_IMMOBILE(ch))
        return;

  if(IS_AFFECTED3(ch, AFF3_ENLARGE))
  {
    send_to_char("You would burst!\r\n", ch);
    return;
  }
  
  if (IS_AFFECTED5(ch, AFF5_TITAN_FORM))
  {
    send_to_char("Hrmm, lets not do that, shall we?\r\n", ch);
    return;
  }

  if (GET_CHAR_SKILL(ch, SKILL_INFURIATE) < number(70, 100))
  {
      bzero(&af, sizeof(af));
      af.type = SKILL_INFURIATE;
      af.duration = 3;
      af.bitvector3 = AFF3_ENLARGE;
      affect_to_char(ch, &af);

      af.bitvector3 = 0;
      af.modifier = (ch->base_stats.Str / 12);
      af.location = APPLY_STR_MAX;
      affect_to_char(ch, &af);

      af.modifier = (ch->base_stats.Con / 7);
      af.location = APPLY_CON_MAX;
      affect_to_char(ch, &af);

      af.modifier = -(ch->base_stats.Agi / 3);
      af.location = APPLY_AGI;
      affect_to_char(ch, &af);

      af.modifier = -(ch->base_stats.Dex / 3);
      af.location = APPLY_DEX;
      affect_to_char(ch, &af);
  send_to_char("Your &+rblood boils&n and you feel bigger in size!\r\n", ch);
  act
    ("$n is overwhelmed with &+RANGER&n, and starts to increase in size!\r\n",
     FALSE, ch, 0, 0, TO_ROOM);
  notch_skill(ch, SKILL_INFURIATE, 17);
  CharWait(ch, PULSE_VIOLENCE);
  }
  else
  {
    bzero(&af, sizeof(af));
    af.type = SKILL_INFURIATE;
    af.duration = 3;
    af.bitvector5 = AFF5_TITAN_FORM;
    affect_to_char(ch, &af);

    af.bitvector3 = 0;
    af.modifier = (ch->base_stats.Str / 10);
    af.location = APPLY_STR_MAX;
    affect_to_char(ch, &af);

    af.modifier = (ch->base_stats.Con / 5);
    af.location = APPLY_CON_MAX;
    affect_to_char(ch, &af);
    af.modifier = -(ch->base_stats.Agi / 5);
    af.location = APPLY_AGI;
    affect_to_char(ch, &af);

    af.modifier = -(ch->base_stats.Dex / 5);
    af.location = APPLY_DEX;
    affect_to_char(ch, &af);

    send_to_char("Your &+rblood boils&n and you feel your body grow to enormous proportions!\r\n", ch);
    act("$n is overwhelmed with &+RHATRED&n, and begins to grow to enormous proportions!\r\n",
    FALSE, ch, 0, 0, TO_ROOM);
    notch_skill(ch, SKILL_INFURIATE, 17);
    CharWait(ch, PULSE_VIOLENCE);
  }
}


void do_rage(P_char ch, char *argument, int cmd)
{
  struct affected_type af;
  int dura;

  if( !IS_ALIVE(ch) || !CAN_ACT(ch) || IS_IMMOBILE(ch) )
  {
    return;
  }

  if( !affected_by_spell(ch, SKILL_BERSERK) )
  {
    send_to_char("You are not quite angry enough..\r\n", ch);
    return;
  }

  if( !GET_CHAR_SKILL(ch, SKILL_RAGE) )
  {
    send_to_char("You are already crazy, go away.\r\n", ch);
    return;
  }

  if( affected_by_spell(ch, SKILL_RAGE) )
  {
    send_to_char("You are too deep in battle madness!\r\n", ch);
    return;
  }

  if( affected_by_spell(ch, SKILL_RAGE_REORIENT) )
  {
    send_to_char("You have not yet recovered from your last fit of &+rBloodLust&n!\r\n", ch);
    return;
  }

  if( number(1, 105) > GET_CHAR_SKILL(ch, SKILL_RAGE) )
  {
    send_to_char("&+RYou are unable to call forth the rage within you...\r\n", ch);
    CharWait(ch, 2 * PULSE_VIOLENCE);
    return;
  }

  notch_skill(ch, SKILL_RAGE, (int) get_property("skill.notch.offensive", 7));

  if(!(GET_SPEC(ch, CLASS_BERSERKER, SPEC_RAGELORD)))
  {
    CharWait(ch, 1.5 * PULSE_VIOLENCE);
  }
  else
  {
    CharWait(ch, (int)(0.5 * PULSE_VIOLENCE));
  }

  act("&+rYou feel a rage start to come from within...\n", FALSE, ch, 0, 0, TO_CHAR);
  send_to_char("&+rYou are filled with a HUGE rush of BLOODLUST!\r\n", ch);
  act("$n fills with a &+RSURGE&n of &+rBLoOdLuST! ROARRRRRRRR!!!\r\n", FALSE, ch, 0, 0, TO_ROOM);

  dura = (4 * PULSE_VIOLENCE * GET_CHAR_SKILL(ch, SKILL_RAGE)) / 100;

  ch->specials.combat_tics = 3;
  memset(&af, 0, sizeof(struct affected_type));
  af.type = SKILL_RAGE;
  af.flags = AFFTYPE_SHORT;
  af.bitvector2 = AFF2_FLURRY;
  af.duration = dura;
  affect_to_char(ch, &af);

  set_short_affected_by(ch, SKILL_RAGE_REORIENT, (dura * 3 ) / 2);
}

/*
 * forage - look for little food items in suitable terrain
 */

void do_forage(P_char ch, char *arg, int cmd)
{
  P_obj    foodobj, treeobj = NULL;
  int      chance = 0;
  int      chance2 = 0;
  char     sectname[64] = "through the area", buf[512], pois = TRUE;

  if (!SanityCheck(ch, "do_forage"))
    return;

  if (IS_NPC(ch))
  {
    send_to_char("You are far too NPC-like to even try.\r\n", ch);
    return;
  }
  if (IS_AFFECTED2(ch, AFF2_SCRIBING))
  {
    send_to_char
      ("Sorry, you're quite busy with your scribing right now..\r\n", ch);
    return;
  }
  if (IS_AFFECTED2(ch, AFF2_MEMORIZING))
  {
    send_to_char("Sorry, you're quite busy memorizing at the moment.\r\n",
                 ch);
    return;
  }
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    act("Forage while fighting?  Are you mad!?",
        FALSE, ch, 0, ch->specials.fighting, TO_CHAR);
    return;
  }

  /*
   * check out the general terrain
   */
  switch (world[ch->in_room].sector_type)
  {
  case SECT_OCEAN:
    send_to_char
      ("You manage to grab a fish, but it flops out of your hands.\r\n", ch);
    return;
  case SECT_CITY:
  case SECT_UNDRWLD_CITY:
  case SECT_INSIDE:
  case SECT_UNDRWLD_INSIDE:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_UNDRWLD_MOUNTAIN:
    send_to_char("There's nothing edible around here..\r\n", ch);
    return;
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_UNDRWLD_WATER:
  case SECT_UNDRWLD_NOSWIM:
  case SECT_UNDRWLD_SLIME:
    send_to_char("Fish, sure, forage?  Nah.\r\n", ch);
    return;
  case SECT_NO_GROUND:
  case SECT_UNDRWLD_NOGROUND:
    send_to_char("Not much food hanging around in midair, I'm afraid.\r\n",
                 ch);
    return;
  case SECT_UNDERWATER:
  case SECT_UNDERWATER_GR:
    send_to_char("Mmmm, kelp.....\r\n", ch);
    return;
  case SECT_UNDRWLD_LIQMITH:
  case SECT_FIREPLANE:
  case SECT_CASTLE_WALL:
  case SECT_CASTLE_GATE:
  case SECT_CASTLE:
  case SECT_NEG_PLANE:
  case SECT_PLANE_OF_AVERNUS:
  case SECT_AIR_PLANE:
  case SECT_WATER_PLANE:
  case SECT_EARTH_PLANE:
  case SECT_ETHEREAL:
  case SECT_ASTRAL:
  case SECT_LAVA:
    send_to_char
      ("Food? Here?! I don't think so.\r\n", ch);
    return;

    /* following just may have something */

  case SECT_SWAMP:
    if (GET_RACE(ch) != RACE_TROLL)
    {
      send_to_char
        ("You have no idea what around here is edible and what isn't..\r\n",
         ch);
      return;
    }

    chance = 30;
    pois = FALSE;
    break;
  case SECT_FIELD:
    chance = 15;
    chance2 = 30;
    break;
  case SECT_FOREST:
    if (GET_RACE(ch) == RACE_GREY)
      chance = 50;
    else if (GET_RACE(ch) == RACE_HALFELF)
      chance = 35;
    else
      chance = 25;

    if (GET_CLASS(ch, CLASS_RANGER))
      chance += 20;
    if (GET_CLASS(ch, CLASS_DRUID))
      chance += 25;

    chance2 = 100;

    break;
  case SECT_HILLS:
    if (GET_RACE(ch) == RACE_MOUNTAIN)
      chance = 25;
    else
      chance = 10;
    chance2 = 75;
    break;
  case SECT_MOUNTAIN:
    if (GET_RACE(ch) == RACE_MOUNTAIN)
      chance = 15;
    else
      chance = 5;
    chance2 = 50;
    break;
  case SECT_UNDRWLD_WILD:
    chance = 20;
    chance2 = 75;
    break;
  case SECT_UNDRWLD_MUSHROOM:
    chance = 45;
    chance2 = 30;
    break;
  case SECT_DESERT:
  case SECT_ROAD:
  case SECT_ARCTIC:
  case SECT_SNOWY_FOREST:
    chance = 3;
    break;
  default:
    logit(LOG_DEBUG, "Bogus sector_type (%d) in do_forage",
          world[ch->in_room].sector_type);
    send_to_char("How strange!  This terrain doesn't seem to exist!\r\n", ch);
    return;
  }
  if (ch->specials.z_cord < 0)
  {
    send_to_char("Fish, sure.  Forage?  Nah.\r\n", ch);
    return;
  }
  else if (ch->specials.z_cord > 0)
  {
    send_to_char("Not much food hanging around in midair, I'm afraid.\r\n",
                 ch);
    return;
  }
  /*
   * ok, terrain is good, let's see if there are any extenuating
   * circumstances (SINGLE_FILE, etc)
   */

  if (IS_SET(world[ch->in_room].room_flags, TUNNEL) ||
      IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    send_to_char("It's a little too cramped to forage in here.\r\n", ch);
    return;
  }
  if (IS_SET(world[ch->in_room].room_flags, UNDERWATER))
  {
    send_to_char("Fish, sure.  Forage?  Nah.\r\n", ch);
    return;
  }
  if (IS_STUNNED(ch))
  {
    send_to_char("You're too stunned to try and forage.\r\n", ch);
    return;
  }

  /* slight int bonus.. */

  if (GET_RACE(ch) == RACE_MOUNTAIN)
    chance = 15;

  chance += GET_C_INT(ch) / 15;

  if (GET_CLASS(ch, CLASS_ALCHEMIST))
    chance += 50;

  if (GET_C_LUK(ch) / 2 > number(0, 100)) {
    chance += 25;
  }

  if (number(0, 99) > chance)
  {
    send_to_char
      ("You forage about for a bit, but find nothing of any substance.\r\n",
       ch);
    act("$n forages around the area for a bit, but finds nothing.", TRUE,
        ch, 0, 0, TO_ROOM);
    CharWait(ch, PULSE_VIOLENCE * 1 / 2);
  }
  else
  {

    /* success!  let's give them delicious grub, whaddaya say */

    switch (world[ch->in_room].sector_type)
    {
    case SECT_SWAMP:
      foodobj = read_object(821 + number(0, 2), VIRTUAL);
      strcpy(sectname, "the surrounding swamp");
      break;

    case SECT_FIELD:
      foodobj = read_object(821 + number(0, 1), VIRTUAL);
      strcpy(sectname, "through the brush");
      break;

    case SECT_FOREST:
      foodobj = read_object(822 + number(0, 4), VIRTUAL);
      strcpy(sectname, "through the undergrowth");
      break;

    case SECT_HILLS:
      foodobj = read_object(825 + number(0, 3), VIRTUAL);
      strcpy(sectname, "through the rocky terrain");
      break;

    case SECT_MOUNTAIN:
      foodobj = read_object(826 + number(0, 2), VIRTUAL);
      strcpy(sectname, "through the rocky terrain");
      break;

    case SECT_UNDRWLD_WILD:
      foodobj = read_object(822 + number(0, 2), VIRTUAL);
      strcpy(sectname, "the surrounding area");
      break;

    case SECT_UNDRWLD_MUSHROOM:
      foodobj = read_object(822 + number(0, 3), VIRTUAL);
      strcpy(sectname, "the surrounding area");
      break;

    case SECT_DESERT:
      foodobj = read_object(820 + number(0, 1), VIRTUAL);
      strcpy(sectname, "the surrounding desert");
      break;

    default:
      send_to_char("error in forage..  tell somebody.\r\n", ch);
      return;
    }

    if (!foodobj)
    {
      send_to_char("you found something, but not really.  tell a god.\r\n",
                   ch);
      return;
    }

    obj_to_char(foodobj, ch);
    sprintf(buf, "Searching %s, you manage to find $p.", sectname);
    act(buf, FALSE, ch, foodobj, 0, TO_CHAR);
    act("Foraging around, $n comes up with $p.", TRUE, ch, foodobj, 0,
        TO_ROOM);
    if (!GET_CLASS(ch, CLASS_RANGER) && !GET_CLASS(ch, CLASS_DRUID) && pois)
    {
      if (!number(0, 6) && !foodobj->value[3])
        foodobj->value[3] = 10 + number(0, 10);
    }
    CharWait(ch, PULSE_VIOLENCE * 2);
  }

  if (IS_GIANT(ch))
  {
    chance2 += GET_C_INT(ch) / 15;

    if (number(0, 99) > chance2)
      return;

    switch (world[ch->in_room].sector_type)
    {
    case SECT_FIELD:
      treeobj = read_object(20, VIRTUAL);
      strcpy(sectname, "through the brush");
      break;

    case SECT_FOREST:
      treeobj = read_object(20, VIRTUAL);
      strcpy(sectname, "through the undergrowth");
      break;

    case SECT_HILLS:
      treeobj = read_object(20, VIRTUAL);
      strcpy(sectname, "through the rocky terrain");
      break;

    case SECT_MOUNTAIN:
      treeobj = read_object(20, VIRTUAL);
      strcpy(sectname, "through the rocky terrain");
      break;

    case SECT_UNDRWLD_WILD:
      treeobj = read_object(20, VIRTUAL);
      strcpy(sectname, "the surrounding area");
      break;

    case SECT_UNDRWLD_MUSHROOM:
      treeobj = read_object(20, VIRTUAL);
      strcpy(sectname, "the surrounding area");
      break;

    default:
      send_to_char("error in forage..  tell somebody.\r\n", ch);
      return;
    }
    obj_to_room(treeobj, ch->in_room);
    sprintf(buf, "Searching %s, you manage to unroot $p.", sectname);
    act(buf, FALSE, ch, treeobj, 0, TO_CHAR);
    act("Foraging around, $n unroots $p.", TRUE, ch, treeobj, 0, TO_ROOM);
    CharWait(ch, PULSE_VIOLENCE * 1);
  }
}


void do_qui(P_char ch, char *argument, int cmd)
{
  send_to_char("You have to write quit - no less, to quit!\r\n", ch);
  return;
}

void do_quit(P_char ch, char *argument, int cmd)
{
  int      i, l;
  P_obj    obj;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (!command_confirm)
  {

    /* check if they can currently do a quit at all */
    if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
    {
      send_to_char("No way! You are fighting.\r\n", ch);
      if (ch->desc)
        ch->desc->confirm_state = CONFIRM_NONE;
      return;
    }
    /* seems reasonable enuf, so ask them to confirm it */
    else if( ch->desc && !IS_TRUSTED(ch) )
    {
      send_to_char("Don't be such a wimp!\r\n", ch);
      return;
      ch->desc->confirm_state = CONFIRM_NONE;
    }
    else if (ch->desc)
      ch->desc->confirm_state = CONFIRM_NONE;

    if( !IS_TRUSTED(ch) )
      return;                   /* don't let em quit, dammit */
  }
  /* confirmed quit! */

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("No way! You are fighting.\r\n", ch);
    return;
  }
  act("Goodbye, friend.. Come back soon!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n has quit the game.", TRUE, ch, 0, 0, TO_ROOM);
  /* ugly, could get stuck in wraithform. JAB */
  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
    BackToUsualForm(ch);
  i = ch->in_room;
  loginlog(ch->player.level, "%s has quit in [%d].", GET_NAME(ch),
           world[ch->in_room].number);
  sql_log(ch, CONNECTLOG, "Quit game");

  /*
   * Mortals: drop everything but nodrop items, write char, then extract
   * Imps: write char, extract
   */

  if( GET_LEVEL(ch) < MINLVLIMMORTAL )
  {
    for (l = 0; l < MAX_WEAR; l++)
      if (ch->equipment[l])
      {
        obj = unequip_char(ch, l);
        if (IS_SET(obj->extra_flags, ITEM_TRANSIENT))
        {
          extract_obj(obj, TRUE);
          obj = NULL;
        }
        else                    /* if (!IS_SET (obj->extra_flags, ITEM_NODROP)) */
          obj_to_room(obj, ch->in_room);
      }
    if (ch->carrying)
    {
      P_obj    next_obj;

      for (obj = ch->carrying; obj != NULL; obj = next_obj)
      {
        next_obj = obj->next_content;
        if (IS_SET(obj->extra_flags, ITEM_TRANSIENT))
        {
          extract_obj(obj, TRUE);
          obj = NULL;
        }
        else
        {                       /* if (!IS_SET (obj->extra_flags, ITEM_NODROP)) */
          obj_from_char(obj, TRUE);
          obj_to_room(obj, ch->in_room);
        }
      }
    }
    ch->desc->connected = CON_PWDDCNF;
    extract_char(ch);
    ch = NULL;
  }
  else
  {
    ch->specials.was_in_room = world[i].number;
    ch->in_room = i;
    writeCharacter(ch, 3, i);
    extract_char(ch);
    ch = NULL;
  }
}

void event_autosave(P_char ch, P_char victim, P_obj obj, void *data)
{
  // Not sure how this is happening, but it'll stop crashes.
  if( !IS_ALIVE(ch) )
  {
    debug( "event_autosave: DEAD/NONEXISTANT char %s '%s'.", (ch==NULL) ? "!" : IS_NPC(ch) ? "NPC" : "PC",
      (ch==NULL) ? "NULL" : J_NAME(ch) );
    logit( LOG_DEBUG, "event_autosave: DEAD/NONEXISTANT char %s '%s'.", (ch==NULL) ? "!" : IS_NPC(ch) ? "NPC" : "PC",
      (ch==NULL) ? "NULL" : J_NAME(ch) );
    return;
  }
  do_save_silent(ch, 1);
  add_event(event_autosave, 1200, ch, 0, 0, 0, 0, 0);
}

void do_save_silent(P_char ch, int type)
{
  FILE    *f;
  char     tmp_buf[MAX_STRING_LENGTH], tmp_buf2[MAX_STRING_LENGTH];
  int      count, i;
  P_char   random_mob;
  P_obj    obj;

  if (!ch || !GET_NAME(ch) || (IS_NPC(ch) && !IS_MORPH(ch)))
    return;

  if (IS_HARDCORE(ch))
  {
    sprintf(tmp_buf, "NotDead %lu", ch->only.pc->numb_deaths);
    checkHallOfFame(ch, tmp_buf);
  }
  if (!IS_TRUSTED(ch))
  {
    checkLeaderBoard(ch);
  }

  update_achievements(ch, 0, 0, 0);
  set_surname(ch, 0);

  if ((ch->desc && !ch->desc->connected) || !ch->desc)
  {
    if (IS_SET(GET_PLYR(ch)->specials.act, PLR_SNOTIFY))
      send_to_char("Autosaving...\r\n", ch);
    if (ch->desc)
    {
      sprintf(tmp_buf, "../hosts/%d", ch->desc->descriptor);
      f = fopen(tmp_buf, "r");

      if (f != NULL)
      {
        fgets(tmp_buf, MAX_STRING_LENGTH, f);
        if (tmp_buf[0] != '\0')
        {
          fscanf(f, "Address:  %s\n", tmp_buf);
          fscanf(f, "\n");
          fgets(tmp_buf, MAX_STRING_LENGTH, f);
          if (tmp_buf[0] == 'N')
          {
            sscanf(tmp_buf, "Name:    %s\n", tmp_buf2);
            strncpy(ch->desc->host2, tmp_buf2, 128);
            sprintf(tmp_buf, "rm ../hosts/%d", ch->desc->descriptor);
            system(tmp_buf);
          }
        }

        fclose(f);
      }
    }
    if (!writeCharacter(ch, type, ch->in_room))
    {
      logit(LOG_DEBUG,
            "Problem saving player %s in do_save_silent()", GET_NAME(ch));
      send_to_char("Danger -- cannot save your character!\r\n", ch);
      send_to_char("Better contact an Implementor ASAP.\r\n", ch);
    }
  }
}

void do_save(P_char ch, char *argument, int cmd)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  float    blood = 0;
  int      i;
  char     buf[MAX_STRING_LENGTH];
  int      count;
  char     tmp_buf[MAX_STRING_LENGTH];
  P_obj    obj;

  if (!ch)
    return;


  if (!str_cmp("log", argument))
  {
    send_to_char("Manual log disabled.\r\n", ch);
    return;
    send_to_char("Saving your log...\r\n", ch);
    manual_log(ch);
    CharWait(ch, 20);
    return;

  }

  if (IS_HARDCORE(ch))
  {
    sprintf(tmp_buf, "NotDead %lu", ch->only.pc->numb_deaths);
    checkHallOfFame(ch, tmp_buf);

  }
  if (!IS_TRUSTED(ch))
  {
    checkLeaderBoard(ch);
  }

  update_achievements(ch, 0, 0, 0);
  set_surname(ch, 0);


  if (IS_NPC(ch) && !IS_MORPH(ch))
  {
    if (!ch->following || !IS_PC(ch->following))
    {
      wizlog(OVERLORD,
             "%s attempted to save in room %d, but was not a real pet.",
             GET_NAME(ch), world[ch->in_room].number);
      return;
    }

    return;                     /* too easy to refresh stone etc right now */

    if (!writePet(ch))
      wizlog(OVERLORD, "Pet %s did not save when ordered to.", GET_NAME(ch));
    else
      wizlog(OVERLORD, "Pet %s saved to file %ld!", GET_NAME(ch),
             GET_IDNUM(ch));
  }
  sprintf(Gbuf1, "Saving %s.\r\n", GET_NAME(GET_PLYR(ch)));
  send_to_char(Gbuf1, ch);
  update_pos(ch);

  if (!writeCharacter(ch, 1, ch->in_room))
  {
    send_to_char("Danger -- cannot save your character!\r\n", ch);
    send_to_char("Better contact an Implementator ASAP.\r\n", ch);
  }
}

void do_not_here(P_char ch, char *argument, int cmd)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}

void do_no_buy(P_char ch, char *argument, int cmd)
{
  if (!*argument)
    send_to_char("Sorry, can you be a bit more explicit?\r\n", ch);
  else
    send_to_char("Nobody seems interested in selling you that.\r\n", ch);
}

int test_atm_present(P_char ch)
{
  P_obj    atm;

  if( (atm = get_obj_in_list_num(real_object(3097), world[ch->in_room].contents)) && CAN_SEE_OBJ(ch, atm) )
  {
    return 1;
  }
  else if( (atm = get_obj_in_list_num(real_object(132581), world[ch->in_room].contents)) && CAN_SEE_OBJ(ch, atm) )
  {
    return 1;
  }
  else if( (atm = get_obj_in_list_num(real_object(GH_BANK_COUNTER_VNUM), world[ch->in_room].contents)) && CAN_SEE_OBJ(ch, atm) )
  {
    return 1;
  }

  return 0;
}

void do_balance(P_char ch, char *argument, int cmd)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
    return;

  if (test_atm_present(ch))
  {
#if 0
    /* Thieving bankers */
    if (!number(0, 5))
    {
      switch (number(1, 4))
      {
      case 1:
        if (GET_BALANCE_PLATINUM(ch))
          GET_BALANCE_PLATINUM(ch) -=
            (GET_BALANCE_PLATINUM(ch) / 100) * number(1, 10);
        break;
      case 2:
        if (GET_BALANCE_GOLD(ch))
          GET_BALANCE_GOLD(ch) -=
            (GET_BALANCE_GOLD(ch) / 100) * number(5, 15);
        break;
      case 3:
        if (GET_BALANCE_SILVER(ch))
          GET_BALANCE_SILVER(ch) -=
            (GET_BALANCE_SILVER(ch) / 100) * number(10, 20);
        break;
      case 4:
        if (GET_BALANCE_COPPER(ch))
          GET_BALANCE_COPPER(ch) -=
            (GET_BALANCE_COPPER(ch) / 100) * number(15, 25);
        break;
      }
    }
#endif
    sprintf(Gbuf1,
            "Your account contains:\r\n    %d &+Wplatinum&N, %d &+Ygold&N, %d silver, %d &+ycopper&N coins.\r\n",
            GET_BALANCE_PLATINUM(ch), GET_BALANCE_GOLD(ch),
            GET_BALANCE_SILVER(ch), GET_BALANCE_COPPER(ch));
    send_to_char(Gbuf1, ch);
  }
  else
  {
    send_to_char("I don't see an ATM machine around here.\r\n", ch);
  }
}

void do_deposit(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      money, ctype, ok;

  ok = 0;
  if (IS_NPC(ch))
    return;
  if (!test_atm_present(ch))
  {
    send_to_char("I don't see an ATM machine around here.\r\n", ch);
    return;
  }

  if (strstr("all", argument))
  {
    ok = (GET_COPPER(ch));
    money = ok;
    if (ok)
    {
      GET_COPPER(ch) -= money;
      GET_BALANCE_COPPER(ch) += money;
    }
    ok = (GET_SILVER(ch));
    money = ok;
    if (ok)
    {
      GET_SILVER(ch) -= money;
      GET_BALANCE_SILVER(ch) += money;
    }
    ok = (GET_GOLD(ch));
    money = ok;
    if (ok)
    {
      GET_GOLD(ch) -= money;
      GET_BALANCE_GOLD(ch) += money;
    }
    ok = (GET_PLATINUM(ch));
    money = ok;
    if (ok)
    {
      GET_PLATINUM(ch) -= money;
      GET_BALANCE_PLATINUM(ch) += money;
    }
    do_balance(ch, 0, -4);
    return;
  }




  half_chop(argument, arg, Gbuf1);
  ctype = coin_type(Gbuf1);

  if (!*arg || !isdigit(*arg) || (ctype == -1))
  {
    send_to_char
      ("Syntax: deposit # <x>coins\r\n  where <x> is 'c' for &+ycopper&N, 's' for silver, 'g' for &+Ygold&N, \r\n  and 'p' for &+Wplatinum&N.\r\n",
       ch);
  }
  else
  {
    money = atoi(arg);
    if (money <= 0)
    {
      send_to_char("Try an amount that makes sense, bozo.\r\n", ch);
      return;
    }
    switch (ctype)
    {
    case 0:
      ok = (money <= GET_COPPER(ch));
      if (ok)
      {
        GET_COPPER(ch) -= money;
        GET_BALANCE_COPPER(ch) += money;
      }
      break;
    case 1:
      ok = (money <= GET_SILVER(ch));
      if (ok)
      {
        GET_SILVER(ch) -= money;
        GET_BALANCE_SILVER(ch) += money;
      }
      break;
    case 2:
      ok = (money <= GET_GOLD(ch));
      if (ok)
      {
        GET_GOLD(ch) -= money;
        GET_BALANCE_GOLD(ch) += money;
      }
      break;
    case 3:
      ok = (money <= GET_PLATINUM(ch));
      if (ok)
      {
        GET_PLATINUM(ch) -= money;
        GET_BALANCE_PLATINUM(ch) += money;
      }
      break;
    }
    if (!ok)
    {
      send_to_char("You haven't got that much!\r\n", ch);
    }
    else
    {
      do_balance(ch, 0, -4);
    }
  }
}

void do_withdraw(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      money, ctype, ok;

  ok = 0;

  if (IS_NPC(ch))
    return;

  if (!test_atm_present(ch))
  {
    send_to_char("I don't see a bank around here.\r\n", ch);
    return;
  }
  half_chop(argument, arg, Gbuf1);
  ctype = coin_type(Gbuf1);

  if (!*arg || !isdigit(*arg) || (ctype == -1))
  {
    send_to_char
      ("Syntax: withdraw # <x>coins\r\n  where <x> is 'c' for &+ycopper&N, 's' for silver, 'g' for &+Ygold&N, \r\n  and 'p' for &+Wplatinum&N.\r\n",
       ch);
  }
  else
  {
    money = atoi(arg);
    if (money <= 0)
    {
      send_to_char("Try an amount that makes sense, bozo.\r\n", ch);
      return;
    }
    switch (ctype)
    {
    case 0:
      ok = (money <= GET_BALANCE_COPPER(ch));
      if (ok)
      {
        GET_BALANCE_COPPER(ch) -= money;
        GET_COPPER(ch) += money;
      }
      break;
    case 1:
      ok = (money <= GET_BALANCE_SILVER(ch));
      if (ok)
      {
        GET_BALANCE_SILVER(ch) -= money;
        GET_SILVER(ch) += money;
      }
      break;
    case 2:
      ok = (money <= GET_BALANCE_GOLD(ch));
      if (ok)
      {
        GET_BALANCE_GOLD(ch) -= money;
        GET_GOLD(ch) += money;
      }
      break;
    case 3:
      ok = (money <= GET_BALANCE_PLATINUM(ch));
      if (ok)
      {
        GET_BALANCE_PLATINUM(ch) -= money;
        GET_PLATINUM(ch) += money;
      }
      break;
    }
    if (!ok)
    {
      send_to_char("You haven't got that much in your account!\r\n", ch);
    }
    else
    {
      do_balance(ch, 0, -4);
    }
  }
}

void do_sneak(P_char ch, char *argument, int cmd)
{
  struct affected_type af;
  byte     percent;
  int      skl_lvl = 0;
  char     Gbuf1[MAX_STRING_LENGTH];
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }

  if(IS_RIDING(ch))
  {
    send_to_char("While mounted? I don't think so...\r\n", ch);
    return;
  }
  
  if(IS_NPC(ch))
  {
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_SNEAK);
    // skl_lvl = BOUNDED(1, (dice(5, 2) + (GET_LEVEL(ch) - 5) * 8), 95);
  }
  else if (IS_PC(ch))
  {
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_SNEAK);
  }

  if((skl_lvl == 0) &&
     !has_innate(ch, INNATE_SNEAK))
  {
    send_to_char("You better leave the art of sneaking to the thieves.\r\n", ch);
    return;
  }

  one_argument(argument, Gbuf1);
  
  if(*Gbuf1)
  {
    if(!str_cmp(Gbuf1, "off"))
    {
      if(affected_by_spell(ch, SKILL_SNEAK))
      {
        affect_from_char(ch, SKILL_SNEAK);
        send_to_char("Ok, you quit trying to sneak.\r\n", ch);
        return;
      }
      else if (IS_AFFECTED(ch, AFF_SNEAK))
      {
        if (has_innate(ch, INNATE_SNEAK))
        {
          send_to_char("You purposefully tred loudly.\r\n", ch);
          REMOVE_BIT(ch->specials.affected_by, AFF_SNEAK);
        }
        else
          send_to_char("You can't seem to quit.\r\n", ch);
        return;
      }
      else
      {
        send_to_char("You need to be sneaking before you can stop sneaking.\r\n", ch);
        return;
      }
    }
    else
    {
      send_to_char("Try \"sneak off\" to quit sneaking.\r\n", ch);
      return;
    }
    return;
  }

  if(has_innate(ch, INNATE_SNEAK))
  {
    if(!IS_AFFECTED(ch, AFF_SNEAK))
    {
      send_to_char("You resume your sneaky ways.\r\n", ch);
      SET_BIT(ch->specials.affected_by, AFF_SNEAK);
      return;
    }
  }
  
  if(GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF))
  {
    send_to_char("You merge with the shadows...\r\n", ch);
  }
  else
  {
    send_to_char("Ok, you'll try to move silently for a while.\r\n", ch);
  }
  
  if(affected_by_spell(ch, SKILL_SNEAK))
  {
    affect_from_char(ch, SKILL_SNEAK);
  }
  
  percent = number(1, 101);
  CharWait(ch, PULSE_VIOLENCE);

// Notching check moved to actmove.c Apr09 -Lucrot
//  notch_skill(ch, SKILL_SNEAK, 5);

  bzero(&af, sizeof(af));
  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch);
  
  if(GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF) &&
    (GET_LEVEL(ch) > 35))
  {
    af.bitvector = AFF_SNEAK;
  }
  else if(percent < (skl_lvl + agi_app[STAT_INDEX(GET_C_AGI(ch))].sneak))
  {
    af.bitvector = AFF_SNEAK;
  }
  
  affect_to_char(ch, &af);
}

void do_hide(P_char ch, char *argument, int cmd)
{
  byte     percent;
  int      skl_lvl = 0, vis_mode;
  bool     tried = FALSE;
  P_obj    obj_object, next_obj, tobj, next_tobj;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (!SanityCheck(ch, "do_hide"))
  {
    logit(LOG_DEBUG, "do_hide failed SanityCheck");
    return;
  }
  one_argument(argument, Gbuf1);

  if (!*Gbuf1 &&
      (!GET_CHAR_SKILL(ch, SKILL_HIDE) || (IS_NPC(ch) && !IS_THIEF(ch))))
  {
    send_to_char("What would you like to hide?\r\n", ch);
    return;
  }
  else if ((str_cmp(Gbuf1, "all") == 0) || strstr(Gbuf1, "all."))
  {
    send_to_char("Sorry, you can't hide everything at once!\r\n", ch);
    return;
  }
  else
    if ((str_cmp(Gbuf1, "me") == 0 || str_cmp(Gbuf1, "self") == 0 || !*Gbuf1))
  {
    if (IS_NPC(ch) && IS_THIEF(ch))
      skl_lvl = BOUNDED(1, (dice(5, 2) + (GET_LEVEL(ch) - 5) * 4), 95);
    else if (IS_PC(ch))
      skl_lvl = ch->only.pc->skills[SKILL_HIDE].learned;

    if (!skl_lvl)
    {
      send_to_char("You better leave the art of hiding to the thieves.\r\n",
                   ch);
      return;
    }
    if (ch->specials.z_cord > 0)
    {
      send_to_char("There's really nowhere to hide up here.\r\n", ch);
      return;
    }
    if (IS_RIDING(ch))
    {
      send_to_char("While mounted? I don't think so...\r\n", ch);
      return;
    }
    if (affected_by_spell(ch, SPELL_FAERIE_FIRE))
    {
      send_to_char
        ("How on earth are you going to hide with this stuff all over you?\r\n",
         ch);
      return;
    }
    
    if(!IS_WATERFORM(ch) &&
      (IS_WATER(ch->in_room) ||
       IS_WATER_ROOM(ch->in_room)))
    {
      send_to_char("It is too &+bwet&n to hide here. Go find dry land...\r\n", ch);
      return;
    }
    
    if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
    {
      send_to_char("Hide behind your weapon, you're a little busy for anything else.\r\n", ch);
      return;
    }
    send_to_char("You attempt to hide yourself.\r\n", ch);
    if (IS_AFFECTED(ch, AFF_HIDE))
      REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
    percent = number(1, 101);   /*
                                 * 101% is a complete failure
                                 */

    if (GET_C_LUK(ch) / 2 > number(0,100)) {
      percent = (int) (percent * 0.90);
    }

    if (GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF) && GET_CHAR_SKILL(ch, SKILL_HIDE) > 90)
    {
     send_to_char("Being a master cutpurse, you easily slip into the shadows.\r\n", ch);
    }
    else
      CharWait(ch, PULSE_VIOLENCE * 3);

      /*  destroy your tracks! */
      for (tobj = world[ch->in_room].contents; tobj; tobj = next_tobj)
      {
        next_tobj = tobj->next_content;
        if (tobj->R_num == real_object(1276))
        {
          extract_obj(tobj, TRUE);
          tobj = NULL;
        }
      }

    if (GET_RACE(ch) == RACE_PSBEAST)
    {
      if( IS_TWILIGHT_ROOM(ch->in_room) || !IS_LIGHT(ch->in_room) )
      {
        percent = 0;
        send_to_char("&+LYou blend into the shadows...&n\r\n", ch);
      }
    }
    notch_skill(ch, SKILL_HIDE, 17);
    if (percent > skl_lvl + agi_app[STAT_INDEX(GET_C_AGI(ch))].hide)
    {
      return;
    }
    SET_BIT(ch->specials.affected_by, AFF_HIDE);
    struct affected_type af;

    if (number(0, 101) < GET_CHAR_SKILL(ch, SKILL_AMBUSH))
    {
      send_to_char("&+LYou find a particulary good spot.&n\r\n", ch);
      affect_from_char(ch, SKILL_AMBUSH);
      bzero(&af, sizeof(af));
      af.type = SKILL_AMBUSH;
      af.duration = 10;
      affect_to_char(ch, &af);
    }
    return;

  }
  else if (sscanf(Gbuf1, "all.%s", Gbuf2) != 1)
  {                             /*
                                 * If not All.<obj>
                                 */
    /*
     * Scan Room for object - either <object> or #.<object>
     */
    obj_object = get_obj_in_list_vis(ch, Gbuf1, world[ch->in_room].contents);
    if (!obj_object)
    {                           /*
                                 * Can't find object
                                 */
      send_to_char("You don't see that here.\r\n", ch);
      return;
    }
    else
    {                           /*
                                 * Found object, try to hide it
                                 */
      try_to_hide(ch, obj_object);
      return;
    }
  }
  /*
   * Bury all.<object>
   */
  for (obj_object = world[ch->in_room].contents; obj_object;
       obj_object = next_obj)
  {
    next_obj = obj_object->next_content;
    if ((isname(Gbuf2, obj_object->name)) && (CAN_SEE_OBJ(ch, obj_object)))
    {
      try_to_hide(ch, obj_object);
      tried = TRUE;
    }
  }

  if (!tried)
  {                             /*
                                 * If we couldn't see or find anything
                                 */
    send_to_char("You don't see that here.\r\n", ch);
  }
  return;
}

#define CAN_LISTEN_BEHIND_DOOR(ch,dir)  \
                (GET_CLASS(ch, CLASS_ROGUE) && \
                (EXIT(ch, dir) && EXIT(ch, dir)->to_room != NOWHERE && \
                 IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)))

void listen(P_char ch, char *argument)
{
  P_char   tch, tch_next;
  int      dir, percent, found = 0;
  const char *heard_nothing = "You don't hear anything unusual.\r\n";
  const char *room_spiel = "$n seems to listen intently for something.";
  char     buf[MAX_STRING_LENGTH];

  if (!ch->desc || IS_NPC(ch))
    return;

  if (IS_SET(world[ch->in_room].room_flags, ROOM_SILENT))
  {
    send_to_char("What is the sound of one hand clapping?\r\n", ch);
    return;
  }
  percent = number(1, 101);

  one_argument(argument, buf);

  if (!*buf)
  {
    /*
     * no argument means that the character is listening for * hidden or
     * invisible beings in the room he/she is in
     */
    for (tch = world[ch->in_room].people; tch; tch = tch_next)
    {
      tch_next = tch->next_in_room;
      if ((tch != ch) && !CAN_SEE(ch, tch) && !IS_TRUSTED(ch))
        found++;
    }
    if (found)
    {
      if (GET_LEVEL(ch) >= 15)
      {
        /*
         * being a higher level is better
         */
        sprintf(buf,
                "You hear what might be %d creatures invisible, or hiding.\r\n",
                MAX(1, (found + number(0, 1) - number(0, 1))));
      }
      else
        sprintf(buf, "You hear an odd rustling in the immediate area.\r\n");
      send_to_char(buf, ch);
      notch_skill(ch, SKILL_LISTEN, 50);
    }
    else
      send_to_char(heard_nothing, ch);
    act(room_spiel, TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  else
  {
    /*
     * the argument must be one of the cardinal directions: north,  * south,
     * etc.
     */
    for (dir = 0; dir < NUM_EXITS; dir++)
    {
      if (!strncmp(buf, dirs[dir], strlen(buf)))
        break;
    }
    if (dir == NUM_EXITS)
    {
      send_to_char("Listen where?\r\n", ch);
      return;
    }
    if (CAN_GO(ch, dir) || CAN_LISTEN_BEHIND_DOOR(ch, dir))
    {
      for (tch = world[EXIT(ch, dir)->to_room].people; tch; tch = tch_next)
      {
        tch_next = tch->next_in_room;
        found++;
      }
      if (found)
      {
        if (GET_LEVEL(ch) >= 15)
        {
          sprintf(buf, "You hear what might be %d creatures %s%s.\r\n",
                  MAX(1, (found + number(0, 1) - number(0, 1))),
                  ((dir == 5) ? "below" : (dir == 4) ? "above" : "to the "),
                  ((dir == 5) ? "" : (dir == 4) ? "" : dirs[dir]));
        }
        else
          sprintf(buf, "You hear sounds from %s%s.\r\n",
                  ((dir == 5) ? "below" : (dir == 4) ? "above" : "the "),
                  ((dir == 5) ? "" : (dir == 4) ? "" : dirs[dir]));
        send_to_char(buf, ch);
        notch_skill(ch, SKILL_LISTEN, 50);
      }
      else
        send_to_char(heard_nothing, ch);
      act(room_spiel, TRUE, ch, 0, 0, TO_ROOM);
      return;
    }
    else
      send_to_char("You can't listen in that direction.\r\n", ch);
    return;
  }
  return;
}

void do_listen(P_char ch, char *argument, int cmd)
{
  int      dir, percent;
  const char *heard_nothing = "You don't hear anything unusual.\r\n";

  if (!ch->desc || IS_NPC(ch))
    return;

  if (IS_SET(world[ch->in_room].room_flags, ROOM_SILENT))
  {
    send_to_char("What is the sound of one hand clapping?\r\n", ch);
    return;
  }
  percent = number(1, 101);

  if (GET_C_LUK(ch) /2 > number(0, 100)) {
    percent = (int) (percent * 0.9);
  }

  if (GET_CHAR_SKILL(ch, SKILL_LISTEN) < percent)
  {
    send_to_char(heard_nothing, ch);
    return;
  }

  listen(ch, argument);
}

#undef CAN_LISTEN_BEHIND_DOOR


/* Okay, a good thief is gonna have base of 120-150 or so to steal,
   if person CAN see him.  So, we make it so a -75 mod results in
   still a 75% chance of success basically, a 50 mo difier isn't
   to bad, but a 120 modifier is very hard */

static int location_mod[] = { 75,       /* light */
  65,                           /* r finger */
  65,                           /* l finger */
  95,                           /* neck 1 */
  95,                           /* neck 2 */
  110,                          /* body */
  105,                          /* head */
  95,                           /* legs */
  145,                          /* feet */
  110,                          /* hands */
  100,                           /* arms */
  100,                          /* shield */
  100,                          /* about */
  70,                           /* waist */
  75,                           /* r wrist */
  75,                           /* l wrist */
  150,                          /* primary weapon */
  120,                          /* hold */
  100,                          /* eyes */
  100,                          /* face */
  50,                           /* R earring */
  50,                           /* L earring */
  90,                           /* quiver */
  50,                           /* badge */
  150,                          /* third wep */	
  150,                          /* 4th wep */
  100,                          /* back */
  50,                           /* belt1 */
  50,                           /* belt2 */
  50,                           /* belt3 */
  90,                           /* arms 2 */
  90,                           /* hands 2 */
  65,                           /* R wrist 2 */
  65,                           /* L Wrist 2 */
  130,                          /* horse body */
  100,                          /* rear legs */
  100,                          /* rear feet */
  80,                           /* nose */
  90,                           /* horn */
  95,                          /* Ioun */
  0,                            /* Not used */
  0, 0, 0
};

void do_steal(P_char ch, char *argument, int cmd)
{
  P_char   victim;
  P_obj    obj = NULL;
  char     victim_name[MAX_INPUT_LENGTH];
  char     obj_name[MAX_INPUT_LENGTH];
  int      percent, roll, i, type = 0;
  int      gold[4] = { 0, 0, 0, 0 }, eq_pos = 0, diff;
  bool     failed = FALSE, caught = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
    return;
  if(!IS_TRUSTED(ch))
  {
    send_to_char("Steal is temporarily disabled for rework. Go stab something.\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) < 10)
  {
    send_to_char("You're too inexperienced.. get some levels.\r\n", ch);
    return;
  }
  
  if (IS_RIDING(ch))
  {
    send_to_char("While mounted? I don't think so...\r\n", ch);
    return;
  }
  
/* 
 if((IS_AFFECTED(ch, AFF_INVISIBLE) ||
      IS_AFFECTED2(ch, AFF2_MINOR_INVIS)) &&
      !IS_AFFECTED(ch, AFF_DETECT_INVISIBLE))
  {
    send_to_char
      ("You can't even see your own hand. How do you plan on stealing?\r\n",
       ch);
    return;
  }
*/
  
  if (!GET_CHAR_SKILL(ch, SKILL_STEAL) &&
      !IS_TRUSTED(ch))
  {
    send_to_char("You better leave the art of stealing to the thieves.\r\n",
                 ch);
    return;
  }
  if (CHAR_IN_ARENA(ch))
  {
    send_to_char("Steal in the arena? Yeah right.\r\n", ch);
    return;
  }
  /*if (GET_ALIGNMENT(ch) > 349 && !IS_TRUSTED(ch)) {
     send_to_char("Your conscience gets the better of you. You must be getting soft in your old age.\r\n", ch);
     return;
     } */
  if ((IS_CARRYING_N(ch) + 1) > CAN_CARRY_N(ch))
  {
    send_to_char
      ("My! Aren't we the greedy one!  You couldn't carry anything more if it was just\r\nlaying around on the ground!\r\n",
       ch);
    return;
  }
  if (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch))
  {
    send_to_char("Sheesh!  With that load it's a wonder you can walk!\r\n",
                 ch);
    return;
  }

  if (affected_by_spell(ch, TAG_PVPDELAY))
  {
    send_to_char("There is too much adrenaline pumping through your body right now.\r\n", ch);
    return;
  }

  if( CHAR_IN_SAFE_ZONE(ch) && !IS_TRUSTED(ch) )
  {
    send_to_char
      ("Your conscience prevents you from stealing in such a peaceful place.\r\n",
       ch);
    return;
  }
  
  argument = one_argument(argument, obj_name);
  one_argument(argument, victim_name);

  if (!(victim = get_char_room_vis(ch, victim_name)))
  {
    send_to_char("Steal what from who?\r\n", ch);
    //CharWait(ch, PULSE_VIOLENCE);
    return;
  }
  else if (victim == ch)
  {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }
 /* if (IS_PC(victim) && (GET_LEVEL(ch) < 35) &&
      !GET_SPEC(ch, CLASS_THIEF, SPEC_CUTPURSE))
  {
    send_to_char("Maybe you should practice more...\r\n", ch);
    return;
  }*/
  if ((world[ch->in_room].room_flags & SINGLE_FILE) &&
      !AdjacentInRoom(ch, victim))
  {
    act("$N seems to be just a BIT out of reach.",
        FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  if (ch->group && !on_front_line(ch))
  {
    send_to_char("You can't quite reach...\r\n", ch);
    return;
  }
  if (victim->group && !on_front_line(victim))
    if (GET_SPEC(ch, CLASS_THIEF, SPEC_CUTPURSE))
    {
      if (number(0, GET_C_DEX(ch)) < (75 + STAT_INDEX(GET_C_DEX(victim))))
      {
        CharWait(ch, PULSE_VIOLENCE);
        send_to_char
          ("You can't quite reach your victim, but you try anyway...\r\n",
           ch);
        return;
      }
    }
    else
    {
      send_to_char("You can't quite reach...\r\n", ch);
      return;
    }

  if( IS_FIGHTING(victim) || IS_DESTROYING(victim) )
  {
    send_to_char("Yah, right, good way to lose a hand, or a head!\r\n", ch);
    return;
  }
  /*if (IS_PC(victim) && !victim->desc && GET_LEVEL(ch) <= MAXLVLMORTAL) {
     send_to_char("Not while they're link.dead, you don't...\r\n", ch);
     return;
     } */
  /*
   * percent chance to succeed
   */

  // Begin trying to figure out chance of success..
  // thief skill
  percent = GET_CHAR_SKILL(ch, SKILL_STEAL);
  // thief dex
  percent += dex_app[STAT_INDEX(GET_C_DEX(ch))].p_pocket;
  // victim mentals
  percent -=
    (STAT_INDEX(GET_C_WIS(victim)) + STAT_INDEX(GET_C_INT(victim))) - 19;
  // both thief and victim luck
  if (GET_C_LUK(victim) / 2 > number(0, 100))
    percent = (int) (percent * 0.85);
  if (GET_C_LUK(ch) / 2 > number(0, 100))
    percent = (int) (percent * 1.05);
  // modifier for level differences
  percent += (GET_LEVEL(ch) - GET_LEVEL(victim)) / 2;
  // no clue what this innate is
  if (has_innate(victim, INNATE_DRAGONMIND))
  percent = percent / 2;
  // at this point, ensure that percent is between 1 and 100
  percent = BOUNDED(1, percent, 100);

  // victim can't see?  that makes it much easier to steal...
	if (!CAN_SEE(victim, ch))
    percent += 40;
  // cutpurses get an additional bonus...
  if (GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF))
    percent += 25;

  if (IS_TRUSTED(ch) ||
      (GET_STAT(victim) < STAT_SLEEPING) ||
      IS_AFFECTED(victim, AFF_SLEEP) ||
      IS_IMMOBILE(victim))
    percent += 200;             /* ALWAYS SUCCESS */
  else if (IS_AFFECTED2(victim, AFF2_STUNNED))
    percent += 20;              /* nice bonus if target is stunned */
  else if (GET_STAT(victim) == STAT_SLEEPING)
    percent += 40;              /* hefty bonus if just normal sleeping */

  // AWARE victims get a massive bonus to "save"
  if (IS_AFFECTED(victim, AFF_AWARE) ||
      affected_by_spell(victim, SKILL_AWARENESS))
    percent -= 70;

  if(is_being_guarded(victim))
    percent = (int) (percent * 0.60);

  if( IS_FIGHTING(victim) )
    percent = (int) (percent * 0.60);

  if(affected_by_spell(victim, SPELL_GUARDIAN_SPIRITS))
  {
    percent = (int) (percent * 0.25);
    guardian_spirits_messages(ch, victim);
  }

  // certain people just can't be stolen from.  Ever.
  if( !IS_TRUSTED(ch) && (IS_TRUSTED(victim) || IS_SHOPKEEPER(victim)) )
    percent = 0;                /* Failure */

  roll = number(0, 100);

  if (!str_cmp(obj_name, "coins"))
    type = 3;
  else
  {
    if ((obj = get_obj_in_list(obj_name, victim->carrying)))
    {
      if (CAN_SEE_OBJ(ch, obj))
        type = 2;
    }
    else
    {
      for (eq_pos = 0; (eq_pos < MAX_WEAR); eq_pos++)
        if (victim->equipment[eq_pos] &&
            (isname(obj_name, victim->equipment[eq_pos]->name)) &&
            CAN_SEE_OBJ(ch, victim->equipment[eq_pos]))
        {
          obj = victim->equipment[eq_pos];
          break;
        }
      if (obj)
        type = 1;
    }
  }

  CharWait(ch, PULSE_VIOLENCE * 2);
  
 
 if (obj && IS_ARTIFACT(obj) && 
      !IS_TRUSTED(ch))
  {
    send_to_char("That item appears to be &+Mmagically &nbound to them, better try to steal something else.\r\n", ch);
    return;
  }


  switch (type)
  {
  case 0:
    /*
     * nothing found to steal, so it's an auto-fail, but chance of
     * getting caught is real low.
     */
/*
    failed = TRUE;
*/
    percent += 50;
    if (GET_SPEC(ch, CLASS_THIEF, SPEC_CUTPURSE))
      percent += 50;
    break;
  case 1:
    /* redoing this so each location has a flag mod */
    if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
    {
      send_to_char("Oooof!  Damn that's heavy!\r\n", ch);
      failed = TRUE;
    }
    if (GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF))
      percent -= location_mod[eq_pos] * 2 / 3;
    else
      percent -= location_mod[eq_pos];

    // roll is number(0,100) - ensure that percent is 1,101
    percent = BOUNDED(1, percent, 101); // percent 101 is a critical hit and
                                        // roll 0 is a crit miss
    if (roll && (GET_LEVEL(ch) > 40) && !failed && (roll < percent))
    {                           /* success */
      act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
      obj = unequip_char(victim, eq_pos);
      remove_owned_artifact(obj, victim, TRUE);
      obj_to_char(obj, ch);
      /*
       * success, but heavy stuff increases chance of getting caught
       */
      //percent -= GET_OBJ_WEIGHT(obj);
      if (IS_PC(victim))
      {
      	wizlog(MINLVLIMMORTAL, "%s &=LMjust stole &n%s (%d) from %s (%d) with percent (%d)\n",
               ch->player.name, obj->short_description,
               obj_index[obj->R_num].virtual_number, victim->player.name,
               world[ch->in_room].number,
               percent);
      	logit(LOG_STEAL, "%s just stole %s (%d) from %s (%d) with percent (%d)",
               ch->player.name, obj->short_description,
               obj_index[obj->R_num].virtual_number, victim->player.name,
               world[ch->in_room].number,
               percent);
       }
    }
    else
    {
      send_to_char("Nice try.  Not a successful try, but a nice one!\r\n",
                   ch);
      logit(LOG_WIZ,
            "STEAL:: %s just failed a steal attempt on %s (%d) from %s (%d) with percent (%d)\n",
            ch->player.name, obj->short_description,
            obj_index[obj->R_num].virtual_number, victim->player.name,
            world[ch->in_room].number,
            percent);
      failed = TRUE;
      caught = TRUE;
    }
    break;
  case 2:
    /* snitching something from target's inven */
    /* heavy items increase difficulty */
    //percent -= GET_OBJ_WEIGHT(obj);

    if (roll > MIN(percent, 99))
      failed = TRUE;
    else
    {
      /* Steal the item */
      if (!failed &&
          ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)))
      {
        send_to_char("Oooof!  Damn that's heavy!\r\n", ch);
        failed = TRUE;
        logit(LOG_WIZ,
              "STEAL:: %s just failed a steal attempt on %s(%d) from %s (%d) with percent (%d)\n",
              ch->player.name, obj->short_description,
              obj_index[obj->R_num].virtual_number, victim->player.name,
              world[ch->in_room].number,
              percent);
      }
      if (!failed)
      {
        send_to_char("Got it!\r\n", ch);

        if (IS_SET(obj->bitvector, AFF_INVISIBLE) &&
            affected_by_spell(victim, SKILL_PERMINVIS))
          affect_from_char(victim, SKILL_PERMINVIS);

        obj_from_char(obj, TRUE);
        obj_to_char(obj, ch);
        notch_skill(ch, SKILL_STEAL, 10);
        if (IS_PC(victim))
        {
          wizlog(MINLVLIMMORTAL, "%s &=LMjust stole &n%s (%d) from %s (%d) percent (%d)",
                 ch->player.name, obj->short_description,
                 obj_index[obj->R_num].virtual_number, victim->player.name,
                 world[ch->in_room].number,
                 percent);
          sql_log(ch, PLAYERLOG, "Stole %s &n[%d] from %s percent (%d)", obj->short_description, obj_index[obj->R_num].virtual_number, J_NAME(victim), percent);
        }
        
      }
    }
    break;
  case 3:
    /* Steal some coins */

    if (roll > MIN(percent, 99))
      failed = TRUE;
    else
    {
      int      ncoins, vcoins;

      /* Steal some coins */
      /* at best, they are only gonna get a handfull */
      ncoins = BOUNDED(0, ((percent / 10) + (GET_LEVEL(ch) / 4)), 22);
      ncoins = number((ncoins - 23), ((GET_LEVEL(ch) / 2) + 2));

      /*
       * base number range -23 to 27, heavily level dependant: level
       * 1:  -23 to 2, level 30: -6 to 17, level 50: -1 to 27. < 0
       * is a failure and also increases chances of being caught.
       */

      /* victim's total coins */
      vcoins =
        GET_COPPER(victim) + GET_SILVER(victim) + GET_GOLD(victim) +
        GET_PLATINUM(victim);
      if (vcoins < 1)
        failed = TRUE;

      if (ncoins < 1)
      {
        failed = TRUE;
        percent += ncoins;
      }
      if (ncoins > vcoins)
      {
        /* gonna clean him out */
        ncoins = vcoins;
      }
      percent -= ncoins;

      if (!failed)
      {
        i = ncoins;
        /*
         * ok, we cycle through the coins, as they collect them,
         * higher level/skill will skew the selection towards the
         * platinum end
         */
        diff = GET_LEVEL(ch) / 20;
        if (percent > 100)
          diff += 1;
        while (i)
        {
          ncoins = i;
          vcoins = BOUNDED(0, number(0, 3 + diff), 3);
          do
          {
            if (vcoins < 0)
              vcoins = 3;
            if (victim->points.cash[vcoins] > 0)
            {
              gold[vcoins]++;
              victim->points.cash[vcoins]--;
              i--;
            }
            else
              vcoins--;
          }
          while (ncoins == i);
        }

        GET_COPPER(ch) += gold[0];
        GET_SILVER(ch) += gold[1];
        GET_GOLD(ch) += gold[2];
        GET_PLATINUM(ch) += gold[3];

        sprintf(Gbuf1,
                "Bingo! You got %d &+Wplatinum&N, %d &+Ygold&N, %d silver, and %d &+ycopper&N coins!\r\n",
                gold[3], gold[2], gold[1], gold[0]);
        send_to_char(Gbuf1, ch);
        notch_skill(ch, SKILL_STEAL, 10);
      }
      else
        send_to_char("You couldn't get any coins...\r\n", ch);
    }
    break;
  }

  /* slap this delay on them to stop the incredibly annoying snatch and
   * run */
  if (GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF))
    CharWait(ch, 18);
  else
    CharWait(ch, 24);

  // player stealing is a PvP action.. though not nearly as serious as actual fighting.
  //  set the time to make it look like it was PvP action, but 20 seconds ago.  This
  //  will let them rent/locker in 40 seconds, and steal (again) in 10 seconds
  if( IS_PC(ch) && IS_PC(victim) )
  {
    affect_from_char(ch, TAG_PVPDELAY);
    set_short_affected_by(ch, TAG_PVPDELAY, 40 * WAIT_SEC);
  }

  /* successful heist is less likely to be detected */
  if ((percent < 0) || MIN(100, percent) < number(failed ? 10 : -60, 100))
    caught = TRUE;

  if (!number(0, 1))
    caught = TRUE;


  /*i
     if (!failed && caught && (type == 2 || type == 3) && obj) {
     obj->justice_status = J_OBJ_STEAL;
     obj->justice_name = str_dup(J_NAME(ch));
     }
   */
  if (!caught)
  {
/*    justice_witness (ch, victim, CRIME_FAKE_THEFT); */
    if (!failed)
      send_to_char("Heh heh, got away clean, too!\r\n", ch);
    else
      send_to_char("Well, at least nobody saw that!\r\n", ch);
    return;
  }
  /*
   * ok, thief has been caught (red-handed if didn't fail), pay the
   * piper time.  Can get flagged, and auto_attacked by mobs.
   */

  /* if the botch it, pull their asses out of hiding! -Zod */
  if (IS_AFFECTED(ch, AFF_HIDE))
  {
    REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
    act("$n has come out of hiding!", TRUE, ch, 0, 0, TO_ROOM);
  }

  justice_witness(ch, victim, failed ? CRIME_ATT_THEFT : CRIME_THEFT);

  if ((GET_STAT(victim) < STAT_SLEEPING) ||
      IS_AFFECTED(victim, AFF_SLEEP) ||
      IS_IMMOBILE(victim))
  {
    /*
     * victim is pretty well hosed, but witnesses could flag the thief
     * later
     */
    send_to_char("Good thing your victim is in no shape to catch you!\r\n",
                 ch);
    return;
  }
  else if (IS_AFFECTED2(victim, AFF2_STUNNED))
  {
    /*
     * have to be pretty clumsy to get caught in this case
     */
    send_to_char("Damn!  Hard to believe they let you into the guild!\r\n",
                 ch);
  }
  else if (GET_STAT(victim) == STAT_SLEEPING)
  {
    /*
     * normal sleep, they are gonna wake up and catch them!
     */
    send_to_char("Groping fingers disturb your rest!\r\n", victim);
    send_to_char
      ("Uh oh, looks like you weren't quite as careful as you should have been!\r\n",
       ch);
    do_wake(victim, 0, -4);
  }
  else
  {
    /* they are awake, and just caught the felonious miscreant in the
     * act! */
    send_to_char
      ("Ooops, better be more careful next time (assuming you survive...)\r\n",
       ch);
  }

  /* tell everybody of the evil deed! */

  // set the victim AWARE for a short bit (wouldn't YOU be paranoid?)
  /*
  struct affected_type *afp = get_spell_from_char(victim, SKILL_AWARENESS);
  if (!afp)
  {
    struct affected_type af;
    bzero(&af, sizeof(af));
    af.type = SKILL_AWARENESS;
    af.duration = 2;
    af.bitvector = AFF_AWARE;
    affect_to_char(victim, &af);
  }
  else
  {
    afp->duration = 1;
  }
  */
  if (!failed)
  {
    if (type == 3)
    {
      act("&+WHey! $n just swiped some of your hard-earned coins!&N", FALSE, ch, 0, victim, TO_VICT);
      act("$n just looted $N's purse!", TRUE, ch, 0, victim, TO_NOTVICT);
    }
    else
    {
      act("&+WHey! $n just stole your $p!&n", FALSE, ch, obj, victim,
          TO_VICT);
      act("$n just stole $p from $N!", TRUE, ch, obj, victim, TO_NOTVICT);
    }
  }
  else
  {
    if (type == 3)
    {
      act("&+WHey! $n just had $s hand in your purse!&n", FALSE, ch, 0, victim, TO_VICT);
      act("$n just tried to steal $N's money!", TRUE, ch, 0, victim, TO_NOTVICT);
    }
    else
    {
      if (obj)
      {
        act("&+WHey! $n just tried to steal your $q!&n",
            FALSE, ch, obj, victim, TO_VICT);
        act("$n just tried to steal something from $N!",
            TRUE, ch, obj, victim, TO_NOTVICT);
      }
      else
      {
        act
          ("&+WHey! $n just tried to steal your $q!  Lucky for you, $e an idiot.&n",
           FALSE, ch, obj, victim, TO_VICT);
        act
          ("$n tried to steal something from $N!  $e really botched it though.",
           TRUE, ch, obj, victim, TO_NOTVICT);
      }
    }
  }

  if (!CAN_SEE(victim, ch) || IS_PC(victim) ||
      IS_SET(victim->specials.act, ACT_NICE_THIEF))
    return;

  remember(victim, ch);

  if (!IS_FIGHTING(victim) && !IS_DESTROYING(victim) )
    MobStartFight(victim, ch);

}

#if 0   // Io's new steal code - not done yet
bool newsteal_CheckIfValid(P_char ch, const char *victim_name, const char *args)
{
  if (!ch || !victim)
    return false;

  if (IS_RIDING(ch))
  {
    send_to_char("While mounted? I don't think so...\r\n", ch);
    return false;
  }
  if (!CAN_SEE(ch, ch))
  {
    send_to_char("You can't even see your own hand. How do you plan on stealing?\r\n", ch);
    return false;
  }
  if ((IS_CARRYING_N(ch) + 1) > CAN_CARRY_N(ch))
  {
    send_to_char("My! Aren't we the greedy one!  You couldn't carry anything more if it was just\r\nlaying around on the ground!\r\n",
                 ch);
    return false;
  }
  if (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch))
  {
    send_to_char("Sheesh!  With that load it's a wonder you can walk!\r\n", ch);
    return false;
  }
  if (CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("Your conscience prevents you from stealing in such a peaceful place.\r\n",ch);
    return false;
  }
  if( IS_DESTROYING(ch) )
  {
    send_to_char( "You can't focus enough right now.\n", ch );
    return FALSE;
  }

  P_char victim = get_char_room_vis(ch, victim_name);
  if (!victim)
  {
    send_to_char("Steal what from who?\r\n", ch);
    return false;
  }
  if (victim == ch)
  {
    send_to_char("You want to steal from yourself?  That's silly!\r\n", ch);
    return false;
  }
  if ((world[ch->in_room].room_flags & SINGLE_FILE) &&
      !AdjacentInRoom(ch, victim))
  {
    act("$N seems to be just a BIT out of reach.", FALSE, ch, 0, victim, TO_CHAR);
    return false;
  }
  if (ch->group && !on_front_line(ch))
  {
    send_to_char("You can't quite reach...\r\n", ch);
    return false;
  }
  if (victim->group && !on_front_line(victim))
    if (GET_SPEC(ch, CLASS_THIEF, SPEC_CUTPURSE))
    {
      if (number(0, GET_C_DEX(ch)) < (75 + STAT_INDEX(GET_C_DEX(victim))))
      {
        send_to_char("You can't quite reach...\r\n", ch);
        return false;
      }
    }
    else
    {
      send_to_char("You can't quite reach...\r\n", ch);
      return false;
    }
  if (IS_FIGHTING(victim))
  {
    send_to_char("Yah, right, good way to lose a hand, or a head!\r\n", ch);
    return false;
  }

  return true;
}

void do_newsteal(P_char ch, char *argument, int cmd)
{
  char     victim_name[MAX_INPUT_LENGTH];
  char     obj_name[MAX_INPUT_LENGTH];
  // initial sanity checks
  if (IS_NPC(ch))
    return;
  if (GET_LEVEL(ch) < 10)
  {
    send_to_char("You're too inexperienced.. get some levels.\r\n", ch);
    return;
  }
  if (!GET_CHAR_SKILL(ch, SKILL_STEAL))
  {
    send_to_char("You better leave the art of stealing to the thieves.\r\n",
                 ch);
    return;
  }
  if (CHAR_IN_ARENA(ch))
  {
    send_to_char("Steal in the arena? Yeah right.\r\n", ch);
    return;
  }
  /*if (GET_ALIGNMENT(ch) > 349 && !IS_TRUSTED(ch)) {
     send_to_char("Your conscience gets the better of you. You must be getting soft in your old age.\r\n", ch);
     return;
     } */
  argument = one_argument(argument, obj_name);
  one_argument(argument, victim_name);

  P_char victim = get_char_room_vis(ch, victim_name);

  if (!victim)
  {
    send_to_char("Steal what from who?\r\n", ch);
    return;
  }
  if (IS_PC(victim) && (GET_LEVEL(ch) < 35) &&
      !GET_SPEC(ch, CLASS_THIEF, SPEC_CUTPURSE))
  {
    send_to_char("Maybe you should practice more...\r\n", ch);
    return;
  }
  if (!newsteal_CheckIfValid(ch, victim_name, obj_name))
    return;


}
#endif  // Io's new steal code - not done yet

void do_explist(P_char ch, char *argument, int cmd)
{
  double   result;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_STRING_LENGTH];
  /*
   * exp tables at immortal levels are just toooo gross, not even gonna
   * THINK about including them, meaningless anyway -JAB
   */

  if( IS_TRUSTED(ch) )
  {
    send_to_char("Thats a moot point. Don't worry about it!\r\n", ch);
    return;
  }
  /*
   * since exp values can get so huge, divide both by 100, just to scale
   * them back into sanity
   */

  if (GET_LEVEL(ch) >= (int)get_property("exp.min.lvl.see.numbers", 51))
  {
    sprintf(Gbuf1, "&+bExperience till level: &+W%d&n\r\n",
            (new_exp_table[GET_LEVEL(ch) + 1] - GET_EXP(ch)));
    send_to_char(Gbuf1, ch);
    return;
  }

  result = ((double)GET_EXP(ch) * 100) / new_exp_table[GET_LEVEL(ch) + 1];

  if (result < 0)
    sprintf(Gbuf1,
            "&+bYou have a long long way to go to your next level!&n\r\n");
  else if (result < 11)
    sprintf(Gbuf1,
            "&+bYou have just begun the trek to your next level!&n\r\n");
  else if (result < 21)
    sprintf(Gbuf1,
            "&+bYou are still a very long way from your next level.&n\r\n");
  else if (result < 31)
    sprintf(Gbuf1,
            "&+bYou have gained some progress, but still have a ways to "
            "&+bgo yet towards your next level.&n\r\n");
  else if (result < 41)
    sprintf(Gbuf1, "&+bYou have gained some progress, and are nearing the "
            "&+bhalf-way point in the trek to your next level.&n\r\n");
  else if (result < 49)
    sprintf(Gbuf1, "&+bYou are close to the half-way point in the journey "
            "&+btowards your next level.&n\r\n");
  else if (result < 53)
    sprintf(Gbuf1,
            "&+bYou are at the half-way point towards this next level!&n\r\n");
  else if (result < 61)
    sprintf(Gbuf1, "&+bYou have just passed the half-way point on the way "
            "&+btowards your next level.&n\r\n");
  else if (result < 71)
    sprintf(Gbuf1,
            "&+bYou are well on your way towards your next level.&n\r\n");
  else if (result < 81)
    sprintf(Gbuf1,
            "&+bYou are three quarters the way to your next level.&n\r\n");
  else if (result < 91)
    sprintf(Gbuf1,
            "&+bYou are almost ready to attain your next level!&n\r\n");
  else
    sprintf(Gbuf1, "&+bYou should level anytime now!&n\r\n");

  send_to_char(Gbuf1, ch);
}

void do_idea(P_char ch, char *argument, int cmd)
{
  FILE    *fl = 0;
  char     buf[MAX_STRING_LENGTH];
  int      result = 0;

  if (IS_NPC(ch))
  {
    send_to_char("The only idea monsters have is to attack things.\r\n", ch);
    return;
  }

  // skip whitespaces
  for (; isspace(*argument); argument++) ;

  if (!*argument)
  {
    send_to_char("Ok, but what is your idea?\r\n", ch);
    return;
  }

  sprintf(buf, "**%s: %s\n", GET_NAME(ch), argument);

  result = InsertIntoFile(IDEA_FILE, buf);
  if (result == 1)
  {
    send_to_char("Unable to allocate necessary memory.\r\n", ch);
    perror("do_idea");
  }
  else if (result == 2)
  {
    send_to_char("Could not open the idea file.\r\n", ch);
    perror("do_idea");
  }
  else
  {
    send_to_char("The idea has been logged - thanks for your input.\r\n", ch);
  }
}

void do_typo(P_char ch, char *argument, int cmd)
{
  FILE    *fl = 0;
  char     buf[MAX_STRING_LENGTH];
  int      result = 0;

  if (IS_NPC(ch))
  {
    send_to_char("Monsters can't spell - leave me alone.\r\n", ch);
    return;
  }

  // skip whitespaces
  for (; isspace(*argument); argument++) ;

  if (!*argument)
  {
    send_to_char("Ok, but what is the typo?\r\n", ch);
    return;
  }

  sprintf(buf, "**%s[%d]: %s\n", GET_NAME(ch), world[ch->in_room].number,
          argument);

  result = InsertIntoFile(TYPO_FILE, buf);
  if (result == 1)
  {
    send_to_char("Unable to allocate necessary memory.\r\n", ch);
    perror("do_typo");
  }
  else if (result == 2)
  {
    send_to_char("Could not open the idea file.\r\n", ch);
    perror("do_typo");
  }
  else
  {
    send_to_char("The typo has been logged - thanks for your input.\r\n", ch);
  }
}

void do_bug(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH];
  long     t = 0;
  int      result = 0;

  if (IS_NPC(ch))
  {
    send_to_char("You are a monster! Bug off!\r\n", ch);
    return;
  }

  // skip whitespaces
  for (; isspace(*argument); argument++) ;

  if (!*argument)
  {
    send_to_char("Ok, but what is the bug?\r\n", ch);
    return;
  }

  t = time(0);

  sprintf(buf, "%s **%s[%d]: %s\n", ctime(&t), GET_NAME(ch),
          world[ch->in_room].number, argument);

  result = InsertIntoFile(BUG_FILE, buf);
  if (result == 1)
  {
    send_to_char("Unable to allocate necessary memory.\r\n", ch);
    perror("do_bug");
  }
  else if (result == 2)
  {
    send_to_char("Could not open the bug file.\r\n", ch);
    perror("do_bug");
  }
  else
  {
    send_to_char("The bug has been logged - thanks for your input.\r\n", ch);
    send_to_char
      ("If you specified the symptoms of the bug and steps that led to it, it will be reviewed shortly.\r\n",
       ch);
  }
}

void do_cheat(P_char ch, char *argument, int cmd)
{
  FILE    *fl;
  long     t;
  char     Gbuf1[MAX_STRING_LENGTH];
  P_desc   i;
  char     Gbuf3[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
  {
    send_to_char("You are a monster! Bug off!\r\n", ch);
    return;
  }
  /*
   * skip whites
   */
  for (; isspace(*argument); argument++) ;

  if( !*argument )
  {
    send_to_char("The cheat command is used to describe a way to cheat.  It doesn't work without a description.\r\n", ch);
    return;
  }
  if (!(fl = fopen(BUG_CHEAT, "a")))
  {
    perror("do_cheat");
    send_to_char("Could not open the cheat-file.\r\n", ch);
    return;
  }
  t = time(0);
  sprintf(Gbuf1, "%s &+W%s[%d] reports following cheat:&+r %s&n\n",
          ctime(&t), GET_NAME(ch), world[ch->in_room].number, argument);
  fputs(Gbuf1, fl);
  fclose(fl);
  send_to_char("Ok.\r\n", ch);


  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && !is_silent(i->character, FALSE) &&
        (i->character != ch) &&
        IS_SET(i->character->specials.act, PLR_PETITION) &&
        IS_TRUSTED(i->character))
    {
      if (IS_TRUSTED(ch))
        act(Gbuf1, 0, ch, 0, i->character, TO_VICT);
      else
        act(Gbuf1, 0, ch, 0, i->character, TO_VICT);
    }
}

void do_area(P_char ch, char *argument, int cmd)
{
  char  buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

	if (IS_NPC(ch) && !IS_MORPH(ch))
	{
		send_to_char("You're nothing but a hound-dog!\r\n", ch);
    return;
  }

  sprintf(buf, "&+LZone Name: %s\r\n", pad_ansi(zone_table[world[ch->in_room].zone].name, 30).c_str());
  int zone_id, zone_number;
  struct zone_data *zone = 0;
  zone_id = world[ch->in_room].zone;
  zone = &zone_table[zone_id];


  sprintf(buf2, "&+LAverage &+rmob&+L level in zone: &n%d\r\n ",
            zone->avg_mob_level);
  int zdiff = (zone->avg_mob_level - GET_LEVEL(ch));
  if (zdiff > 15)
  {
  sprintf(buf2, "&-L&+RWarning&-L, some mobs in this zone may be very hazardous for you! Travel with care!&n\r\n");
  strcat (buf, buf2);
  }
  send_to_char(buf, ch);
  send_to_char(buf2, ch);
}

void do_quaff(P_char ch, char *argument, int cmd)
{
  P_obj    temp;
  int      i, j, chance;
  bool     equipped;
  char     Gbuf1[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
    return;

  equipped = FALSE;

  one_argument(argument, Gbuf1);

  if(!(temp = get_obj_in_list_vis(ch, Gbuf1, ch->carrying)))
  {
    temp = ch->equipment[HOLD];
    equipped = TRUE;

    if((temp == NULL) || !isname(Gbuf1, temp->name))
    {
      act("You do not have that item.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }

  if(temp->type != ITEM_POTION)
  {
    act("You can only quaff potions.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(obj_index[temp->R_num].virtual_number == 850 ||
     obj_index[temp->R_num].virtual_number == 860)
  {
    act("You cannot quaff this potion.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(affected_by_spell(ch, TAG_POTION_TIMER))
  {
    send_to_char("Your body cannot yet handle another jolt of magical influence!\r\n", ch);
    return;
  }

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    chance = 50;

    chance += ((GET_C_DEX(ch) + (GET_C_AGI(ch) / 2)) - 75) / 4;

    if (GET_C_LUK(ch) / 2 > number(0, 100))
      chance = (int) (chance * 1.1);

    if( has_innate(ch, INNATE_QUICK_THINKING) || affected_by_spell(ch, SPELL_COMBAT_MIND) )
      chance = (int)(chance * 1.25);

    if( number(0, 99) >= chance && GET_OBJ_VNUM(temp) != 400228 )
    {
      act("Whoops!  You spilled it!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n attempts to quaff $p, but spills it instead!", TRUE, ch, temp, 0, TO_ROOM);
      extract_obj(temp, TRUE);
      return;
    }
  }

  act("$n &+yquaffs&n $p.", TRUE, ch, temp, 0, TO_ROOM);
  act("As you quaff $p, the vial disappears in a bright &+Wflash of light!&n",
    FALSE, ch, temp, 0, TO_CHAR);

  CharWait(ch, PULSE_VIOLENCE);

  if(equipped)
    unequip_char(ch, HOLD);
  //epic potion
  if(GET_OBJ_VNUM(temp) == 400234)
  {
    gain_epic(ch, EPIC_BOTTLE, 0, 75);
    send_to_char("&+CYou suddenly feel.. epic!\r\n", ch);
    extract_obj(temp, TRUE);
    return;
  }

  struct affected_type af, *afp;
  memset(&af, 0, sizeof(af));
  af.type = TAG_POTION_TIMER;
  af.duration = 3;
  af.flags = AFFTYPE_NODISPEL;
  affect_to_char(ch, &af);

    /* value[5] specifies special functions for epic potions */
/*
  //make sure to add default spell to potion.
  if(temp->value[5] > 0)
  {
    if(temp->value[5] == 1337)
    {
      advance_level(ch);
      extract_obj(temp, TRUE);
      return;
    }
  }
*/
  /* value[4] specifies damage player takes from potion - considered non-magical */

  if(temp->value[4] > 0)
  {
    if(spell_damage(ch, ch, temp->value[4], SPLDAM_GENERIC, 0, 0))
    {
      extract_obj(temp, TRUE);
      return;
    }
  }


  if(IS_SET(world[ch->in_room].room_flags, NO_MAGIC))
  {
    send_to_char
      ("You feel a slight gathering of magic within you, but it fades.\r\n", ch);
  }
  else
    for (i = 1; i < 4; i++)
      if (temp->value[i] >= 1)
      {
        j = temp->value[i];
        if ((j != -1) && (skills[j].spell_pointer != NULL))
        {
          ((*skills[j].spell_pointer) ((int) temp->value[0], ch, 0,
                                       SPELL_TYPE_POTION, ch, 0));
          if (!char_in_list(ch))
            break;
        }
      }

  extract_obj(temp, TRUE);
}

void do_recite(P_char ch, char *argument, int cmd)
{
  P_obj    scroll, obj;
  int      i, bits, j, in_room;
  bool     equipped;
  P_char   victim = NULL;
  char     Gbuf1[MAX_STRING_LENGTH];
  struct spell_target_data target_data;

  equipped = FALSE;
  obj = 0;

  argument = one_argument(argument, Gbuf1);

  if (!(scroll = get_obj_in_list_vis(ch, Gbuf1, ch->carrying)))
  {
    scroll = ch->equipment[HOLD];
    equipped = TRUE;
    if ((scroll == 0) || !isname(Gbuf1, scroll->name))
    {
      act("You do not have that item.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (scroll->type != ITEM_SCROLL)
  {
    act("Recite is normally used for scrolls.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  act("You recite $p which turns to dust in your hands.", FALSE, ch, scroll,
      0, TO_CHAR);
  CharWait(ch, PULSE_VIOLENCE);

  if (GET_LEVEL(ch) > 56)
  {
    wizlog(GET_LEVEL(ch), "%s recites %s [%d]",
           GET_NAME(ch), scroll->short_description,
           world[ch->in_room].number);
    logit(LOG_WIZ, "%s recites %s [%d]", GET_NAME(ch),
          scroll->short_description, world[ch->in_room].number);
    sql_log(ch, WIZLOG, "Recited %s [%d]", scroll->short_description, obj_index[scroll->R_num].virtual_number);
  }

/*  if (victim) {
    act("$n recites $p while looking at $n.", FALSE, ch, scroll, victim, TO_ROOM);
    if ((IS_TRUSTED(victim)) && (GET_LEVEL(ch) < GET_LEVEL(victim))) {
      act("$N laughs at $n's silly words, and throws them back in $s face!"
          ,FALSE, ch, 0, victim, TO_NOTVICT);
      act("$N laughs at your silly words, and throws them back in your face!"
          ,FALSE, ch, 0, victim, TO_CHAR);
      act("$n foolishly tries to recite $p at you - how droll!", FALSE, ch, scroll, victim, TO_VICT);

      victim = ch;
    }
  } else*/
  act("$n recites $p.", FALSE, ch, scroll, 0, TO_ROOM);

  if (equipped)
    unequip_char(ch, HOLD);

  if (IS_SET(world[ch->in_room].room_flags, NO_MAGIC) && !IS_TRUSTED(ch))
    send_to_char("Nothing seems to happen.\r\n", ch);
  else
  {
    for (i = 1; i < 4; i++)
    {
      if ((scroll->value[i] >= 1))
      {
        target_data.ttype = j = scroll->value[i];
        if (!parse_spell_arguments(ch, &target_data, argument))
          continue;
        victim = target_data.t_char;
        obj = target_data.t_obj;

        if ((j != -1) && (skills[j].spell_pointer != NULL))
        {
          if (IS_AGG_SPELL(j) && victim && (ch != victim))
          {
            if (IS_AFFECTED(ch, AFF_INVISIBLE) ||
                IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
              appear(ch);

            if (IS_PC(victim) && should_not_kill(ch, victim))
            {
              act("$N tries to harm you by casting a malicious spell.",
                  FALSE, victim, 0, ch, TO_CHAR);
              continue;
            }
          }
          in_room = ch->in_room;
          ((*skills[j].spell_pointer) ((int) scroll->value[0], ch, 0,
                                       SPELL_TYPE_SPELL, victim, obj));

          /* best thing to do if victim dies is just extract the obj and quit out, since many
             spells kill the mud w/o a victim

             besides, what if the char IS the victim?  heh. */

          if ((victim && !char_in_list(victim)) ||
              ((victim != ch) && !char_in_list(ch)))
          {
            extract_obj(scroll, TRUE);
            return;
          }
          else if (IS_AGG_SPELL(j) && victim && (ch != victim))
          {
            if (affected_by_spell(victim, SPELL_SLEEP))
              affect_from_char(victim, SPELL_SLEEP);

            if (GET_STAT(victim) == STAT_SLEEPING)
            {
              send_to_char("Your rest is violently disturbed!\r\n", victim);
              act("Your spell disturbs $N's beauty sleep!", FALSE, ch, 0,
                  victim, TO_CHAR);
              act("$n's spell disturbs $N's beauty sleep!", FALSE, ch, 0,
                  victim, TO_NOTVICT);
              SET_POS(victim, GET_POS(victim) + STAT_NORMAL);
            }
          }
        }
      }
    }
  }
  extract_obj(scroll, TRUE);
}

void do_use(P_char ch, char *argument, int cmd)
{
  P_char   tmp_char = 0, next_ch;
  P_obj    tmp_object = 0, stick, next_obj;
  int      bits, i = 0;
  char     Gbuf1[MAX_STRING_LENGTH];
  struct spell_target_data target_data;
  int      spl;

  argument = one_argument(argument, Gbuf1);

  /*
   ** To avoid player killing, we restrict wands to be usable
   ** by either a PC, or an NPC who is not charmed.
   */

  if (CHAR_IN_NO_MAGIC_ROOM(ch))
  {
    send_to_char
      ("No magic exists around here for the wand to draw upon!\r\n", ch);
    return;
  }
  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
    return;

  if (!(stick = get_object_in_equip_vis(ch, Gbuf1, &i)))
  {
    send_to_char
      ("Use applies only to objects in your equipment list (usually held).\r\n",
       ch);
    return;
  }
  if (stick->type == ITEM_STAFF)
  {
    act("$n taps $p three times on the ground.", TRUE, ch, stick, 0, TO_ROOM);
    act("You tap $p three times on the ground.", FALSE, ch, stick, 0,
        TO_CHAR);

    if( IS_TRUSTED(ch) )
    {
      wizlog(GET_LEVEL(ch), "%s uses %s [%d]",
        GET_NAME(ch), stick->short_description, world[ch->in_room].number);
      logit(LOG_WIZ, "%s uses %s [%d]", GET_NAME(ch),
        stick->short_description, world[ch->in_room].number);
      sql_log(ch, WIZLOG, "Used %s [%d]", stick->short_description, obj_index[stick->R_num].virtual_number);
    }
    if ((stick->value[2] > 0) &&
        !IS_SET(world[ch->in_room].room_flags, NO_MAGIC))
    {
      stick->value[2]--;
    }
    else
    {
      send_to_char("The staff is completely burnt out!\r\n", ch);
      return;
    }

    spl = target_data.ttype = stick->value[3];
    if (skills[spl].spell_pointer)
    {
      cast_as_area(ch, spl, (int) stick->value[0], argument);

      if (!char_in_list(ch))
        return;

      CharWait(ch, PULSE_VIOLENCE * 3 / 2);
    }
    else
    {
      send_to_char("The staff seems powerless.\r\n", ch);
    }
  }
  else if (stick->type == ITEM_WAND)
  {

    if( IS_TRUSTED(ch) )
    {
      wizlog(GET_LEVEL(ch), "%s uses %s [%d]",
        GET_NAME(ch), stick->short_description, world[ch->in_room].number);
      logit(LOG_WIZ, "%s uses %s [%d]", GET_NAME(ch),
        stick->short_description, world[ch->in_room].number);
      sql_log(ch, WIZLOG, "Used %s [%d]", stick->short_description, obj_index[stick->R_num].virtual_number);
    }
    if ((stick->value[2] > 0) &&
        !IS_SET(world[ch->in_room].room_flags, NO_MAGIC))
    {
      stick->value[2]--;
    }
    else
    {
      send_to_char("There are no more charges in the wand!\r\n", ch);
      return;
    }

    spl = target_data.ttype = stick->value[3];
    if (skills[spl].spell_pointer)
    {
      if (IS_AGG_SPELL(spl) && tmp_char && (tmp_char != ch))
      {
        if (IS_AFFECTED(ch, AFF_INVISIBLE) ||
            IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
          appear(ch);

        act("$N tries to harm you by casting a malicious spell.", FALSE,
            tmp_char, 0, ch, TO_CHAR);

        if (IS_PC(tmp_char) && should_not_kill(ch, tmp_char))
          return;
        justice_witness(ch, tmp_char, CRIME_ATT_MURDER);
      }                         /*
                                 * * if
                                 */

      if (!parse_spell_arguments(ch, &target_data, argument))
        return;

      if (target_data.t_char != NULL)
      {
        tmp_char = target_data.t_char;
        act("$n points $p at $N!", TRUE, ch, stick, tmp_char, TO_ROOM);
        act("You direct $p at $N!", FALSE, ch, stick, tmp_char, TO_CHAR);
      }
      else if( IS_SET(skills[spl].targets, TAR_IGNORE) )
      {
        // Skip whitespace..
        while( isspace(*argument) )
          argument++;
        sprintf( Gbuf1, "You wave $p and say '%s'.", argument );
        act(Gbuf1, TRUE, ch, stick, NULL, TO_CHAR);
        sprintf( Gbuf1, "$n waves $p and says '%s'.", argument );
        act(Gbuf1, TRUE, ch, stick, NULL, TO_ROOM);
      }
      else
      {
        tmp_object = target_data.t_obj;
        act("$n points $p at $P.", TRUE, ch, stick, tmp_object, TO_ROOM);
        act("You direct $p at $P.", FALSE, ch, stick, tmp_object, TO_CHAR);
      }


      if( IS_SET(skills[spl].targets, TAR_IGNORE) )
        ((*skills[spl].spell_pointer) ((int) stick->value[0], ch, argument, SPELL_TYPE_SPELL, tmp_char, tmp_object));
      else
        ((*skills[spl].spell_pointer) ((int) stick->value[0], ch, 0, SPELL_TYPE_SPELL, tmp_char, tmp_object));


      if (char_in_list(ch))
        CharWait(ch, PULSE_VIOLENCE * 3 / 2);

      /* better not access ch's members after here..  he may be dead */

      if (tmp_char && char_in_list(tmp_char) && IS_AGG_SPELL(spl)
          && (ch != tmp_char))
      {
        if (affected_by_spell(tmp_char, SPELL_SLEEP))
          affect_from_char(tmp_char, SPELL_SLEEP);

        if (GET_STAT(tmp_char) == STAT_SLEEPING)
        {
          send_to_char("Your rest is violently disturbed!\r\n", tmp_char);
          act("Your spell disturbs $N's beauty sleep!", FALSE, ch, 0,
              tmp_char, TO_CHAR);
          act("$n's spell disturbs $N's beauty sleep!", FALSE, ch, 0,
              tmp_char, TO_NOTVICT);
          SET_POS(tmp_char, GET_POS(tmp_char) + STAT_NORMAL);
        }
      }
    }
    /*
     * has charges left
     */
    else
    {
      send_to_char("The wand seems powerless.\r\n", ch);
    }
  }
  else
  {
    send_to_char("Use is normally only for wands and staves.\r\n", ch);
  }
}

#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON"  : "OFF")
#define TOG_OFF 0
#define TOG_ON  1

void show_toggles(P_char ch)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_INPUT_LENGTH], Gbuf3[MAX_INPUT_LENGTH];
  P_char   send_ch = ch;

  if (IS_NPC(ch))
  {
    if (IS_MORPH(ch))
    {
      ch = MORPH_ORIG(ch);
    }
    else
    {
      return;
    }
  }

  if (GET_WIMPY(ch) == 0)
  {
    strcpy(Gbuf2, "OFF");
  }
  else
  {
    sprintf(Gbuf2, "%4d", GET_WIMPY(ch));
  }
  if (IS_PC(ch) && (ch->only.pc->screen_length > 0))
  {
    sprintf(Gbuf3, "%3d", ch->only.pc->screen_length);
  }
  else
  {
    strcpy(Gbuf3, " 24");
  }

  sprintf(Gbuf1,
          "&+y-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="
          "-=-=-=-=-=-=-=-=-=-=-=-=-=-&N\r\n"
          "                          &+r    STATUS of toggles.    "
          "                           &N\r\n"
          "&+y-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="
          "-=-=-=-=-=-=-=-=-=-=-=-=-=-&N\r\n"
          "&+r   Tell        :&+g %-3s    &+y|&N"
          "&+r     Brief Mode  :&+g %-3s    &+y|&N"
          "&+r     No Locate   :&+g %-3s    &+y|&N\r\n"
          "&+r   SmartPrompt :&+g %-3s    &+y|&N"
          "&+r     Compact Mode:&+g %-3s    &+y|&N"
          "&+r     Wimpy Level :&+g %-4s   &+y|&N\r\n"
          "&+r   Shout       :&+g %-3s    &+y|&N"
          "&+r     Echo        :&+g %-3s    &+y|&N"
          "&+r     Vicious     :&+g %-3s    &+y|&N\r\n"
          "&+r   Petition    :&+g %-3s    &+y|&N"
          "&+r     Paging      :&+g %-3s    &+y|&N"
          "&+r     Save Notify :&+g %-3s    &+y|&N\r\n"
          "&+r   Who List    :&+g %-3s    &+y|&N"
          "&+r     Screen Size :&+g %-3s    &+y|&N"
          "&+r     Terminal    :&+g %-4s   &+y|&N\r\n"
          "&+r   Map         :&+g %-3s    &+y|&N"
          "&+r     Old SmartP  :&+g %-3s    &+y|&N"
          "&+r     Show Titles :&+g %-3s    &+y|&N\r\n"
          "&+r   Battle Alert:&+g %-3s    &+y|&N"
          "&+r     Kingdom View:&+g %-3s    &+y|&N"
          "&+r     Ship Map    :&+g %-3s    &+y|&N\r\n"
          "&+r   Take        :&+g %-3s    &+y|&N"
          "&+r     Terse Battle:&+g %-3s    &+y|&N"
          "&+r     QuickChant  :&+g %-3s    &+y|&N\r\n"
          "&+r   Project     :&+g %-3s    &+y|&N"
          "&+r     AFK         :&+g %-3s    &+y|&N"
          "&+r     NChat       :&+g %-3s    &+y|&N\r\n"
          "&+r   Hint Channel:&+g %-3s    &+y|&N"
          "&+r     Group Needed:&+g %-3s    &+y|&N"
          "&+r     Showspec    :&+g %-3s    &+y|&N\r\n"
          "&+r   Web Info    :&+g %-3s    &+y|&N"
          "&+r     Show Quests :&+g %-3s    &+y|&N"
          "&+r     Boons       :&+g %-3s    &+y|&N\r\n"
          "&+r   Newbie EQ   :&+g %-3s    &+y|&N"
          "&+r     No Beep     :&+g %-3s    &+y|&n"
          "&+r     Underline   :&+g %-3s    &+y|&N\r\n"
          "&+r   Surname     :&+g %-3s    &+y|"
          "&+r     Damage      :&+g %-3s    &+y|&n"
          "&+r     No Level    :&+g %-3s    &+y|&n\r\n"
          "&+y-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="
          "-=-=-=-=-=-=-=-=-=-=-=-=-=-&N\r\n",
          ONOFF(!PLR_FLAGGED(ch, PLR_NOTELL)),
          ONOFF(PLR_FLAGGED(ch, PLR_BRIEF)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_NOLOCATE)),
          ONOFF(PLR_FLAGGED(ch, PLR_SMARTPROMPT)),
          ONOFF(PLR_FLAGGED(ch, PLR_COMPACT)),
          Gbuf2,
          ONOFF(!PLR_FLAGGED(ch, PLR_NOSHOUT)),
          ONOFF(PLR_FLAGGED(ch, PLR_ECHO)),
          ONOFF(PLR_FLAGGED(ch, PLR_VICIOUS)),
          ONOFF(PLR_FLAGGED(ch, PLR_PETITION)),
          ONOFF(PLR_FLAGGED(ch, PLR_PAGING_ON)),
          ONOFF(PLR_FLAGGED(ch, PLR_SNOTIFY)),
          ONOFF(PLR_FLAGGED(ch, PLR_NOWHO)),
          Gbuf3,
          (send_ch->desc->term_type == 1 ? "GEN " : (send_ch->desc->term_type == 2 ? "ANSI" : "MSP ")),
          ONOFF(PLR_FLAGGED(ch, PLR_MAP)),
          ONOFF(PLR_FLAGGED(ch, PLR_OLDSMARTP)),
          ONOFF(!PLR2_FLAGGED(ch, PLR2_NOTITLE)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_BATTLEALERT)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_KINGDOMVIEW)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_SHIPMAP)),
          ONOFF(!PLR2_FLAGGED(ch, PLR2_NOTAKE)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_TERSE)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_QUICKCHANT)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_PROJECT)),
          ONOFF(PLR_FLAGGED(ch, PLR_AFK)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_NCHAT)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_HINT_CHANNEL)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_LGROUP)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_SPEC)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_WEBINFO)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_SHOW_QUEST)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_BOON)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_NEWBIEEQ)),
          ONOFF(PLR3_FLAGGED(ch, PLR3_NOBEEP)),
          ONOFF(PLR3_FLAGGED(ch, PLR3_UNDERLINE)),
          ONOFF(PLR3_FLAGGED(ch, PLR3_NOSUR)),
          ONOFF(PLR2_FLAGGED(ch, PLR2_DAMAGE)),
          ONOFF(PLR3_FLAGGED(ch, PLR3_NOLEVEL)) );
  send_to_char(Gbuf1, send_ch);

  if (GET_LEVEL(ch) >= AVATAR)
  {
    sprintf(Gbuf1,
            "&+rWmsg:&+g %-3s "
            "&+rWlog:&+g %-3s "
            "&+rLogmsg:&+g %-3s "
            "&+rStat:&+g %-3s "
            "&+rAgg:&+g %-3s "
            "&+rNames:&+g %-3s "
            "&+rVnum:&+g %-3s "
            "&+rBan:&+g %-3s\r\n"
            "&+rExp:&+g %-3s "
            "&+rDebug:&+g %-3s "
            "&+rFog:&+g %-3s "
            "&+rHeal:&+g %-3s "
            "&+rEpic:&+g %-3s\n\r",
            ONOFF(!PLR_FLAGGED(ch, PLR_WIZMUFFED)),
            ONOFF(PLR_FLAGGED(ch, PLR_WIZLOG)),
            ONOFF(PLR_FLAGGED(ch, PLR_PLRLOG)),
            ONOFF(PLR_FLAGGED(ch, PLR_STATUS)),
            ONOFF(PLR_FLAGGED(ch, PLR_AGGIMMUNE)),
            ONOFF(PLR_FLAGGED(ch, PLR_NAMES)),
            ONOFF(PLR_FLAGGED(ch, PLR_VNUM)),
            ONOFF(PLR_FLAGGED(ch, PLR_BAN)),
            ONOFF(PLR2_FLAGGED(ch, PLR2_EXP)),
            ONOFF(PLR_FLAGGED(ch, PLR_DEBUG)),
            ONOFF(PLR_FLAGGED(ch, PLR_MORTAL)),
            ONOFF(PLR2_FLAGGED(ch, PLR2_HEAL)),
            ONOFF(PLR3_FLAGGED(ch, PLR3_EPICWATCH)));
    send_to_char(Gbuf1, send_ch);
    send_to_char
      ("&+y-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-&N\r\n",
       send_ch);
  }
}

static const char *toggles_list[] = {
  "?",                          /*
                                 * * 00
                                 */
  "brief",                      /*
                                 * * 01
                                 */
  "compact",                    /*
                                 * * 02
                                 */
  "who",                        /*
                                 * * 03
                                 */
  "vicious",                    /*
                                 * * 04
                                 */
  "tell",                       /*
                                 * * 05
                                 */
  "names",                      /*
                                 * * 06
                                 */
  "gcc",                        /*
                                 * * 07
                                 */
  "shout",                      /*
                                 * * 08
                                 */
  "anonymous",                  /*
                                 * * 09
                                 */
  "petition",                   /*
                                 * * 10
                                 */
  "paging",                     /*
                                 * * 11
                                 */
  "echo",                       /*
                                 * * 12
                                 */
  "wimpy",                      /*
                                 * * 13
                                 */
  "aggimmunity",                /*
                                 * * 14
                                 */
  "terminal",                   /*
                                 * * 15
                                 */
  "savenotify",                 /*
                                 * * 16
                                 */
  "wizmessages",                /*
                                 * * 17
                                 */
  "wizlog",                     /*
                                 * * 18
                                 */
  "status",                     /*
                                 * * 19
                                 */
  "vnum",                       /*
                                 * * 20
                                 */
  "screensize",                 /*
                                 * * 21
                                 */
  "smartprompt",                /*
                                 * * 22
                                 */
  "fog",                        /*
                                 * * 23
                                 */
  "map",                        /* 24 */
  "debug",                      /* 25 */
  "oldsmartprompt",             /* 26 */
  "ban",                        /* 27 */
  "logmsg",                     /* 28 */
  "nolocate",                   /* 29 */
  "titles",                     /* 30 */
  "battle",                     /* 31 */
  "kingdom",                    /* 32 */
  "shipmap",
  "take",
  "terse",
  "quickchant",
  "rwc",                        /* 37 */
  "project",                    /* 38 */
  "zzxyzz",                     /* 39 */
  "afk",                        /* 40 */
  "nchat",
  "damage",
  "spec1",
  "spec2",
  "spec3",                      /* 45 */
  "spec4",
  "spec_timer",
  "heal",
  "group needed",
  "experience",                 /* 50 */
  "showspec",
  "hint",                       /* 52 */
  "webinfo",
  "acc",
  "quest",                      /* 55 */
  "boon",
  "newbie",
  "beep",
  "underline",
  "surname",                    /* 60 */
  "nolevel",
  "epic",
  "\n"
};

static const char *tog_messages[][2] = {
  {"Help.",
   "Help!"},
  {"&+WBrief&N mode off.\r\n",
   "&+WBrief&N mode on.\r\n"},
  {"&+WCompact&N mode off.\r\n",
   "&+WCompact&N mode on.\r\n"},
  {"Others can now see you on the &+WWho&n list.\r\n",
   "You will no longer show up on the &+WWho&n list.\r\n"},
  {"You feel nice and turn &+WVicious&N mode off.\r\n",
   "You are now &+WVicious&N and will kill mortally wounded victims.\r\n"},
  {"You can now hear &+WTells&N.\r\n",
   "You are now deaf to &+WTells&N.\r\n"},
  {"You are now blind to &+WNames&N.\r\n",
   "You are now spammed by &+WNames&N.\r\n"},
  {"You are now deaf to &+Cguild chatter&N.\r\n",
   "You can now hear &+Cguild chatter&N.\r\n"},
  {"You can now hear &+WGlobal Shouts&N.\r\n",
   "You are now deaf to &+WGlobal Shouts&N.\r\n"},
  {"You are no longer &+WAnonymous&N.\r\n",
   "You are now &+WAnonymous&N.\r\n"},
  {"You are now deaf to &+WPetition&N.\r\n",
   "You can now hear &+WPetition&N.\r\n"},
  {"You have now turned &+WPaging&N mode off.\r\n",
   "You are now into &+WPaging&N mode.\r\n"},
  {"You turn off &+WEchoing&N.\r\n",
   "You turn &+WEchoing&N on.\r\n"},
  {"You now feel like a true hero, no auto-fleeing here! :-)\r\n",
   "You now flee at %s hit points or less!\r\n"},
  {"Aggressive mobiles WILL attack you now.\r\n",
   "Aggressive mobiles will not attack you now.\r\n"},
  {"Your Terminal mode is now: &+W%s&N.\r\n",
   "Your Terminal mode is now: &+W%s&N.\r\n"},
  {"You will no longer get notification from saving.\r\n",
   "You will now get a notification when saved.\r\n"},
  {"You tune into the &+WWizmessages&N.\r\n",
   "You will now not here any &+WWizmessages&N.\r\n"},
  {"You turn off the &+WWizlog&N.\r\n",
   "You tune into the &+WWizlog&N.\r\n"},
  {"You turn off the &+WStatus&N messages.\r\n",
   "You tune into the &+WStatus&N messages.\r\n"},
  {"&+WVnum&N toggled off.\r\n",
   "&+WVnum&N toggled on.\r\n"},
  {"Screen length set to default 24 lines.\r\n",
   "Screen length set to %s lines.\r\n"},
  {"Smartprompt toggled off.\r\n",
   "Smartprompt toggled on.\r\n"},
  {"Once again, you view the world through the rose colored eyes of immortality.\r\n",
   "Your vision is clouded through the fog that is mortality.\r\n"},
  {"Turning maps off.\r\n",
   "Maps will be shown.\r\n"},
  {"Code debugger messaging off.\r\n",
   "Code debugger messaging on.\r\n"},
  {"Old-style smartprompt toggled off.\r\n",
   "Old-style smartprompt toggled on.\r\n"},
  {"You turn off ban messages.\r\n",
   "You turn on ban messages.\r\n"},
  {"You turn off login/logout messages.\r\n",
   "You turn on login/logout messages.\r\n"},
  {"You turn your no-locate status off.\r\n",
   "You turn your no-locate status on.\r\n"},
  {"You will now see player titles.\r\n",
   "You will no longer see player titles.\r\n"},
  {"You turn your battle alert status off.\r\n",
   "You turn your battle alert status on.\r\n"},
  {"You will no longer see kingdom areas on the map.\r\n",
   "You will now see kingdom areas on the map.\r\n"},
  {"You will no longer see maps as ships move.\r\n",
   "You will now see maps as ships move.\r\n"},
  {"You will now accept items from people.\r\n",
   "You will no longer accept items from people.\r\n"},
  {"Terse battle mode disabled.\r\n",
   "Terse battle mode enabled.\r\n"},
  {"Quickchant is disabled.\r\n",
   "Quickchant is enabled.\r\n"},
  {"You will now not hear RaceWar Chatter.\r\n",
   "You now tune into the RaceWar Chatter.\r\n"},
  {"You ignore the will of the Elder Brain.\r\n",
   "You listen for the will of the Elder Brain.\r\n"},
  {"I'm sorry, you can't do that.\r\n",
   "I'm sorry, you can't do that.\r\n"},
  {"You are no longer AFK.\r\n",
   "You are now AFK.\r\n"},
  {"Newbie chat: -=&+ROFF&n=-\r\n",
   "Newbie chat: -=&+GON&n=-\r\n"},
  {"Damage Display is now OFF\r\n",
   "Damage Display is now ON\r\n"},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {"Heal Display is now OFF\r\n",
   "Heal Display is now ON\r\n"},
  {"Group needed is now OFF\r\n",
   "Group needed is now ON\r\n"},
  {"Experience Display is now OFF\r\n",
   "Experience Display is now ON\r\n"},
  {"Specialization Display is now OFF\r\n",
   "Specialization Display is now ON\r\n"},
  {"Hints are now turned OFF.\r\n",
   "Hints are now turned ON.\r\n"},
  {"Extended info about your character will &+rNOT&n be available on our webpage.\r\n",
   "Extended info about your character will &+WBE&n available on our webpage.\r\n"},
  {"You will no longer see the acc channel.\r\n",
   "You will now see the acc channel.\r\n"},
  {"Quest NPC's will no longer show a &+Y(Q)&n.\r\n",
   "Quest NPC's will show a &+Y(Q)&n.\r\n"},
  {"You will no longer be affected by boons.\r\n",
   "You will now be affected by boons.\r\n"},
  {"You will not load with newbie EQ when you die.\r\n",
   "You will now load with newbie EQ when you die.\r\n"},
  {"You can be beeped.\r\n",
   "You can not be beeped.\r\n"},
  {"You will receive blinking instead of underlined text.\r\n",
   "You will receive underlined instead of blinking text.\r\n"},
  {NULL, NULL }, // Surname does its own messaging.
  {"You will level at an epic stone.\r\n",
   "You will not level at an epic stone.\r\n"},
  {"You turn off the &+WEpic&N messages.\r\n",
   "You tune into the &+WEpic&N messages.\r\n"}
};

void do_more(P_char ch, char *arg, int cmd)
{
  if (!IS_SET(GET_PLYR(ch)->specials.act, PLR_PAGING_ON) && ch->desc)
  {
    SET_BIT(ch->specials.act, PLR_PAGING_ON);
    process_with_paging(ch, arg);
    REMOVE_BIT(ch->specials.act, PLR_PAGING_ON);
  }
  else
    command_interpreter(ch, arg);
}

void do_toggle(P_char ch, char *arg, int cmd)
{
  int      i, j, tog_nr = -1, result = -1, wimp_lev = -1;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH], buf[80];
  P_char   send_ch = ch;

  if( IS_NPC(ch) )
  {
    if( IS_MORPH(ch) )
    {
      ch = MORPH_ORIG(ch);
    }
    else
    {
      return;
    }
  }
  arg = one_argument(arg, Gbuf1);

  if( !*Gbuf1 )
  {
    // Doesn't care about morphs: just sending a message to send_ch.
    show_toggles(send_ch);
    return;
  }
  tog_nr = (old_search_block(Gbuf1, 0, strlen(Gbuf1), toggles_list, 0) - 1);

  if( tog_nr < 0 )
  {
    send_to_char("Wrong option.\r\n\r\n", send_ch);
    show_toggles(send_ch);
    return;
  }
  switch (tog_nr)
  {
  case 0:
    strcpy(Gbuf1, "Toggle options:\r\n");
    for (j = 0, i = 0; *toggles_list[i] != '\n'; i++)
    {
      if ((!strn_cmp(toggles_list[i], "wizlog", 6) ||
           !strn_cmp(toggles_list[i], "wizmessage", 10) ||
           !strn_cmp(toggles_list[i], "aggimmunity", 11) ||
           !strn_cmp(toggles_list[i], "vnum", 4) ||
           !strn_cmp(toggles_list[i], "status", 6) ||
           !strn_cmp(toggles_list[i], "names", 5) ||
           !strn_cmp(toggles_list[i], "debug", 5) ||
           !strn_cmp(toggles_list[i], "ban", 3) ||
           !strn_cmp(toggles_list[i], "fog", 3) ||
           !strn_cmp(toggles_list[i], "logmsg", 6) ||
           !strn_cmp(toggles_list[i], "heal", 4) ||
// Damage is now a mortal command.
//           !strn_cmp(toggles_list[i], "damage", 6) ||
           !strn_cmp(toggles_list[i], "epic", 4) ||
           !strn_cmp(toggles_list[i], "experience", 10)) && GET_LEVEL(ch) < MINLVLIMMORTAL)
      {
        continue;
      }
      else
      {
        sprintf(Gbuf1, "%s[%2d] %-15s%s", Gbuf1, i + 1, toggles_list[i], (!(++j % 3) ? "\r\n" : ""));
      }
    }
    strcat(Gbuf1, "\r\n");
    send_to_char(Gbuf1, send_ch);
    return;
    break;
  case 1:                      /*
                                 * brief
                                 */
    result = PLR_TOG_CHK(ch, PLR_BRIEF);
    break;
  case 2:                      /*
                                 * compact
                                 */
    result = PLR_TOG_CHK(ch, PLR_COMPACT);
    break;
  case 3:                      /*
                                 * no who
                                 */
// Enabled by Gellz - 22042015
//    send_to_char("Sorry, we'll have none of that here!\r\n", send_ch);
//    return;
    if (GET_LEVEL(ch) <= 29 &&
        !IS_SET(ch->specials.act, PLR_NOWHO)) {
      send_to_char("Sorry, you must be at least level 30 to toggle who!\r\n", send_ch);
      return;
    } 
      result = PLR_TOG_CHK(ch, PLR_NOWHO);
    break;
  case 4:                      /*
                                 * vicious
                                 */
    if (GET_CLASS(ch, CLASS_PALADIN) &&
        !IS_SET(ch->specials.act, PLR_VICIOUS))
    {
      send_to_char("Now that wouldn't be very nice, would it?\r\n", send_ch);
      return;
    }
    else
      result = PLR_TOG_CHK(ch, PLR_VICIOUS);
    break;
  case 5:                      /*
                                 * tell
                                 */
    result = PLR_TOG_CHK(ch, PLR_NOTELL);
    break;
  case 6:                      /*
                                 * name accept
                                 */
    if (GET_LEVEL(ch) < FORGER)
    {
      send_to_char("Watching for bad names is part of your job :P\r\n", ch);
      return;
    }
    result = PLR_TOG_CHK(ch, PLR_NAMES);
    break;
  case 7:                      /*
                                 * gcc
                                 */
    result = PLR_TOG_CHK(ch, PLR_GCC);
    break;
  case 8:                      /*
                                 * shout
                                 */
    result = PLR_TOG_CHK(ch, PLR_NOSHOUT);
    break;
  case 9:                      /*
                                 * anonymous
                                 */
    if (IS_TRUSTED(ch))
    {
      send_to_char("Sorry, Immortals cannot be anonymous.\r\n", send_ch);
      result = 0;               /*
                                 * in case some slipped through
                                 */
    }
    
// Players can no longer be anonymous.
    else if (IS_PC(ch))
    {
      send_to_char("Nobody can be anonymous on Duris.\r\n", send_ch);
      return;
    }
    // else
      // result = PLR_TOG_CHK(ch, PLR_ANONYMOUS);
    break;
  case 10:                     /*
                                 * petition
                                 */
    result = PLR_TOG_CHK(ch, PLR_PETITION);
    break;
  case 11:                     /*
                                 * paging
                                 */
    result = PLR_TOG_CHK(ch, PLR_PAGING_ON);
    break;
  case 12:                     /*
                                 * echo
                                 */
    result = PLR_TOG_CHK(ch, PLR_ECHO);
    break;
  case 13:                     /*
                                 * wimpy
                                 */

    arg = one_argument(arg, Gbuf1);

    if (isdigit(*Gbuf1))
    {
      if ((wimp_lev = atoi(Gbuf1)))
      {
        if (wimp_lev < 0)
        {
          send_to_char("Hehe... We are jolly funny today, eh?\r\n", send_ch);
          return;
        }
        if (wimp_lev > GET_MAX_HIT(ch))
        {
          send_to_char("That doesn't make much sense, now does it?\r\n",
                       send_ch);
          return;
        }
        GET_WIMPY(ch) = wimp_lev;
        result = TRUE;
        sprintf(Gbuf3, "%d", wimp_lev);
      }
      else if (*Gbuf1 == '0')
      {
        GET_WIMPY(ch) = 0;
        result = FALSE;
        strcpy(Gbuf3, "Off");
      }
    }
    else
    {
      send_to_char
        ("Specify at how many hit points you want to flee.  (0 to disable)\r\n",
         send_ch);
      return;
    }
    break;
  case 14:                     /*
                                 * aggimmune
                                 */
    if (IS_TRUSTED(ch))
      result = PLR_TOG_CHK(ch, PLR_AGGIMMUNE);
    else
    {
      send_to_char("Humf?!\r\n", send_ch);
      return;
    }
    break;
  case 15:                     /*
                                 * term
                                 */
    /*
     * code for Terminal setting!
     */
    one_argument(arg, Gbuf1);
    if (send_ch->desc->term_type == 1)
    {
      send_ch->desc->term_type = 2;
      strcpy(Gbuf3, "ANSI");
    }
    else if (send_ch->desc->term_type == 2)
    {
      send_ch->desc->term_type = 3;
      strcpy(Gbuf3, "MSP ");
    }
    else if (send_ch->desc->term_type == 3)
    {
      send_ch->desc->term_type = 1;
      strcpy(Gbuf3, "GEN ");
    }
    else
    {
      send_to_char("USAGE: TOGGLE terminal\r\n", send_ch);
      return;
    }
    result = TRUE;
    break;
  case 16:                     /*
                                 * savenotify
                                 */
    result = PLR_TOG_CHK(ch, PLR_SNOTIFY);
    break;
  case 17:                     /*
                                 * wizmessages
                                 */
    if (IS_TRUSTED(ch))
      result = PLR_TOG_CHK(ch, PLR_WIZMUFFED);
    else
    {
      send_to_char("Humf?!\r\n", send_ch);
      return;
    }
    break;
  case 18:                     /*
                                 * wizlog
                                 */
    if (IS_TRUSTED(ch))
      result = PLR_TOG_CHK(ch, PLR_WIZLOG);
    else
    {
      send_to_char("Humf?!\r\n", send_ch);
      return;
    }
    break;
  case 19:                     /*
                                 * status
                                 */
    if (IS_TRUSTED(ch))
      result = PLR_TOG_CHK(ch, PLR_STATUS);
    else
    {
      send_to_char("Humf?!\r\n", send_ch);
      return;
    }
    break;
  case 20:                     /*
                                 * status
                                 */
    if (IS_TRUSTED(ch))
      result = PLR_TOG_CHK(ch, PLR_VNUM);
    else
    {
      send_to_char("Humf?!\r\n", send_ch);
      return;
    }
    break;
  case 21:                     /*
                                 * screen length
                                 */

    arg = one_argument(arg, Gbuf1);

    if (is_number(Gbuf1) && (wimp_lev = atoi(Gbuf1)))
    {
      if ((wimp_lev < 12) || (wimp_lev > 48))
      {
        send_to_char("Screen length must be between 12 and 48 lines\r\n",
                     send_ch);
        return;
      }
      ch->only.pc->screen_length = wimp_lev;
      result = TRUE;
      sprintf(Gbuf3, "%d", wimp_lev);
    }
    else if( isname(Gbuf1, "off") || isname(Gbuf1, "default") )
    {
      result = FALSE;
      strcpy(Gbuf3, " 24");
    }
    else
    {
      send_to_char("Specify how many lines to display on a page (no effect without paging)\r\nor 'off' or 'default' to reset to standard 24 lines.\r\n", send_ch);
      return;
    }
    break;
  case 22:
    send_to_char( "Smartprompt was never fully implemented.  Sorry.\n\r", send_ch );
    return;
    // result = PLR_TOG_CHK(ch, PLR_SMARTPROMPT);
    // if (!IS_SET(ch->specials.act, PLR_SMARTPROMPT))
    // {
      // sprintf(buf, VT_HOMECLR);
      // send_to_char(buf, ch);
      // sprintf(buf, VT_MARGSET, 0, ch->only.pc->screen_length);
      // send_to_char(buf, ch);
    // }
    // else
      // InitScreen(ch);
    break;
  case 23:
    if (IS_MORPH(send_ch))
      return;
    if( GET_LEVEL(ch) >= MINLVLIMMORTAL )
      result = PLR_TOG_CHK(ch, PLR_MORTAL);
    else
    {
      send_to_char("Don't you wish it was that easy?\r\n", send_ch);
      return;
    }
    break;
  case 24:
    result = PLR_TOG_CHK(ch, PLR_MAP);
    break;
  case 25:
    if (IS_MORPH(send_ch))
      return;
    if( IS_TRUSTED(ch) )
      result = PLR_TOG_CHK(ch, PLR_DEBUG);
    else
    {
      send_to_char("Don't you wish it was that easy?\r\n", send_ch);
      return;
    }
    break;
  case 26:                     /* old smartprompt */
    result = PLR_TOG_CHK(ch, PLR_OLDSMARTP);
    break;
  case 27:                     /*
                                 * ban
                                 */
    if (IS_TRUSTED(ch))
      result = PLR_TOG_CHK(ch, PLR_BAN);
    else
    {
      send_to_char("Humf?!\r\n", send_ch);
      return;
    }
    break;
  case 28:
    if (IS_TRUSTED(ch))
      result = PLR_TOG_CHK(ch, PLR_PLRLOG);
    else
    {
      send_to_char("Say what?\r\n", send_ch);
      return;
    }
    break;

  case 29:
    if (RACE_EVIL(ch) || RACE_PUNDEAD(ch))
    {
      send_to_char("You wish life were that easy.\r\n", send_ch);
      return;
    }
    else
      result = PLR2_TOG_CHK(ch, PLR2_NOLOCATE);
    break;
  case 30:
    result = PLR2_TOG_CHK(ch, PLR2_NOTITLE);
    break;
  case 31:
    result = PLR2_TOG_CHK(ch, PLR2_BATTLEALERT);
    break;
  case 32:
    result = PLR2_TOG_CHK(ch, PLR2_KINGDOMVIEW);
    break;
  case 33:
    result = PLR2_TOG_CHK(ch, PLR2_SHIPMAP);
    break;
  case 34:
    result = PLR2_TOG_CHK(ch, PLR2_NOTAKE);
    break;
  case 35:
    result = PLR2_TOG_CHK(ch, PLR2_TERSE);
    break;
  case 36:
    result = PLR2_TOG_CHK(ch, PLR2_QUICKCHANT);
    break;
  case 37:
    result = PLR2_TOG_CHK(ch, PLR2_RWC);
    break;
  case 38:
    result = PLR2_TOG_CHK(ch, PLR2_PROJECT);
    break;
  case 39:
    result = PLR2_TOG_CHK(ch, PLR2_NPC_HOG);
    break;
  case 40:
    result = TOGGLE_BIT(ch->specials.act, PLR_AFK) & (PLR_AFK);
    break;
  case 41:
    /* if((GET_LEVEL(ch) > 31) &&
        (GET_LEVEL(ch) < 57) &&
        !IS_SET(PLR2_FLAGS(ch), PLR2_NCHAT) &&
        !IS_SET(PLR2_FLAGS(ch), PLR2_NEWBIE_GUIDE))
     {
       send_to_char
         ("Your level no longer qualifies you for newbie-chat, sorry.\r\n",
          ch);
       return;
     }*/
    result = PLR2_TOG_CHK(ch, PLR2_NCHAT);
    break;
  case 42:
    result = PLR2_TOG_CHK(ch, PLR2_DAMAGE);
    break;
  case 48:
    if( IS_TRUSTED(ch) )
    {
      result = PLR2_TOG_CHK(ch, PLR2_HEAL);
    }
    else
    {
      send_to_char("Humf?!\r\n", send_ch);
      return;
    }
    break;
  case 49:
    if ((GET_LEVEL(ch) > 50) && !IS_SET(PLR2_FLAGS(ch), PLR2_LGROUP))
    {
      send_to_char("You don't need to find a group.\r\n", ch);
      return;
    }
    result = PLR2_TOG_CHK(ch, PLR2_LGROUP);
    break;
  case 50:
    result = PLR2_TOG_CHK(ch, PLR2_EXP);
    break;
  case 51:
    result = PLR2_TOG_CHK(ch, PLR2_SPEC);
    break;
  case 52:
    result = PLR2_TOG_CHK(ch, PLR2_HINT_CHANNEL);
    break;
  case 53:
    result = PLR2_TOG_CHK(ch, PLR2_WEBINFO);
    sql_webinfo_toggle(ch);
    break;
  case 54:
    result = PLR2_TOG_CHK(ch, PLR2_ACC);
    break;
  case 55:
    result = PLR2_TOG_CHK(ch, PLR2_SHOW_QUEST);
    break;
  case 56:
    result = PLR2_TOG_CHK(ch, PLR2_BOON);
    break;
  case 57:
    result = PLR2_TOG_CHK(ch, PLR2_NEWBIEEQ);
    break;
  case 58:
    result = PLR3_TOG_CHK(ch, PLR3_NOBEEP);
    break;
  case 59:
    result = PLR3_TOG_CHK(ch, PLR3_UNDERLINE);
    break;
  case 60: //surname
    arg = one_argument(arg, Gbuf1);

   /* if (is_number(Gbuf1) && (wimp_lev = atoi(Gbuf1)))
    {
      if ((wimp_lev < 12) || (wimp_lev > 48))
      {
        send_to_char("Screen length must be between 12 and 48 lines\r\n",
                     send_ch);
        return;
      }*/
	  if(is_number(Gbuf1) && (wimp_lev = atoi(Gbuf1)))
	  {
	    set_surname(ch, wimp_lev);
  		return;
    }
  /*  else if (isname(Gbuf1, "off") || isname(Gbuf1, "default"))
    {
      result = FALSE;
      strcpy(Gbuf3, " 24");
    }*/
    else
    {
      display_surnames(ch);
      return;
    }
    break;
  case 61:
    result = PLR3_TOG_CHK(ch, PLR3_NOLEVEL);
    break;
  case 62:
    if( IS_TRUSTED(ch) )
    {
      result = PLR3_TOG_CHK(ch, PLR3_EPICWATCH);
    }
    else
    {
      send_to_char("Humf?!\r\n", send_ch);
      return;
    }
    break;
  default:
    break;
  }

  if( result )
  {
    sprintf(Gbuf1, tog_messages[tog_nr][TOG_ON], Gbuf3);
  }
  else
  {
    sprintf(Gbuf1, tog_messages[tog_nr][TOG_OFF], Gbuf3);
  }
  send_to_char(Gbuf1, send_ch);
}

void do_rub(P_char ch, char *argument, int cmd)
{
  return;
}

void do_split(P_char ch, char *argument, int cmd)
{
  char     gold_str[MAX_INPUT_LENGTH], typestr[MAX_INPUT_LENGTH];
  int      group_size = 0, ctype;
  long     gold, share, given;
  struct group_list *gl;
  char     Gbuf1[MAX_STRING_LENGTH];

  given = 0;

  half_chop(argument, gold_str, typestr);       /*
                                                 * Get amount of gold to
                                                 * split
                                                 */

  ctype = coin_type(typestr);

  if (!*gold_str || (ctype == -1))
  {                             /*
                                 * No amount specified
                                 */
    send_to_char("You must specify an amount to split!\r\n", ch);
    return;
  }
  if (strlen(gold_str) > 7)
  {                             /*
                                 * Don't let atol overflow
                                 */
    send_to_char("You must specify a number from 2 to 9999999\r\n", ch);
    return;
  }
  if (!(gold = atol(gold_str)))
  {                             /*
                                 * Don't let gold be zero
                                 */
    send_to_char("You must specify a number from 2 to 9999999\r\n", ch);
    return;
  }
  if (gold < 0)
  {                             /*
                                 * Don't let amount be negative
                                 */
    send_to_char("Sorry, you can't split on credit, pal!\r\n", ch);
    return;
  }
  /*
   * check for gold > coins on hand
   */
  switch (ctype)
  {
  case 0:
    if (gold > GET_COPPER(ch))
    {
      send_to_char
        ("How generous!  Too bad you don't have that much &+ycopper&N!!\r\n",
         ch);
      return;
    }
    break;
  case 1:
    if (gold > GET_SILVER(ch))
    {
      send_to_char
        ("How generous!  Too bad you don't have that much silver!!\r\n", ch);
      return;
    }
    break;
  case 2:
    if (gold > GET_GOLD(ch))
    {
      send_to_char
        ("How generous!  Too bad you don't have that much &+Ygold&N!!\r\n",
         ch);
      return;
    }
    break;
  case 3:
    if (gold > GET_PLATINUM(ch))
    {
      send_to_char
        ("How generous!  Too bad you don't have that much &+Wplatinum&N!!\r\n",
         ch);
      return;
    }
  }

  if (!ch->group)
  {
    send_to_char
      ("You aren't grouped with anyone to split the money with!\r\n", ch);
    return;
  }
  group_size = 0;

  for (gl = ch->group; gl; gl = gl->next)
    if ((ch->in_room == gl->ch->in_room) &&
        (CAN_SEE(ch, gl->ch) || (ch == gl->ch)) && 
        (IS_PC(gl->ch) || IS_MORPH(gl->ch)))
      group_size++;

  if (group_size < 2)
  {
    send_to_char
      ("With which imaginary friends are you trying to split your money?\r\n",
       ch);
    return;
  }
  if (gold < group_size)
  {                             /*
                                 * Make sure enough for 1 coin/player
                                 */
    send_to_char("There isn't enough money to go around!\r\n", ch);
    return;
  }
  act("$n splits some money with $s group.", 1, ch, 0, 0, TO_ROOM);

  share = gold / group_size;    /*
                                 * Calculate amount each player gets
                                 */

  group_size--;

  for (gl = ch->group; gl; gl = gl->next)
  {
    if ((ch->in_room == gl->ch->in_room) &&
        CAN_SEE(ch, gl->ch) && (ch != gl->ch) && 
        (IS_PC(gl->ch) || IS_MORPH(gl->ch)))
    {
      switch (ctype)
      {
      case 0:
        GET_COPPER(gl->ch) += share;
        GET_COPPER(ch) -= share;
        break;
      case 1:
        GET_SILVER(gl->ch) += share;
        GET_SILVER(ch) -= share;
        break;
      case 2:
        GET_GOLD(gl->ch) += share;
        GET_GOLD(ch) -= share;
        break;
      case 3:
        GET_PLATINUM(gl->ch) += share;
        GET_PLATINUM(ch) -= share;
        break;
      }
      given += share;
      sprintf(Gbuf1, "$n gives you your share:  %ld %s coins.",
              share, coin_names[ctype]);
      act(Gbuf1, 0, ch, 0, gl->ch, TO_VICT);

      sprintf(Gbuf1, "You give %s %ld %s coins.\r\n",
              GET_NAME(gl->ch), share, coin_names[ctype]);
      send_to_char(Gbuf1, ch);  /*
                                 * Tell splitter money was given
                                 */
    }
  }

  if (given == 0)
  {
    act
      ("$n then notices that no one in $s group is around, and smiles greedily.",
       1, ch, 0, 0, TO_ROOM);
  }
  sprintf(Gbuf1, "You keep %ld %s coins for yourself.\r\n",
          gold - given, coin_names[ctype]);
  send_to_char(Gbuf1, ch);
  return;
}

#if 0
void do_reboot(P_char ch, char *argument, int cmd)
{
  char    *timestr;
  char     rbtime[MAX_STRING_LENGTH];
  char     nwtime[MAX_STRING_LENGTH];
  long     timenow;
  long     uptime;
  long     updays = 0;
  long     uphours = 0;
  long     upmins = 0;

  timestr = asctime(localtime(&reboot_time));
  strcpy(rbtime, timestr);

  timenow = time(0);
  timestr = asctime(localtime(&timenow));
  strcpy(nwtime, timestr);

  sprintf(Gbuf4, "System last rebooted at %s\r\n", rbtime);
  send_to_char(Gbuf4, ch);
  sprintf(Gbuf4, "It is now %s\r\n", nwtime);
  send_to_char(Gbuf4, ch);

  /*
   * now figure out how long it has been up
   */
  uptime = timenow - reboot_time;

  updays = uptime / SECS_PER_REAL_DAY;
  uptime %= SECS_PER_REAL_DAY;
  uphours = uptime / SECS_PER_REAL_HOUR;
  uptime %= SECS_PER_REAL_HOUR;
  upmins = uptime / SECS_PER_REAL_MIN;

  sprintf(Gbuf4,
          "The system has been up for %d days, %d hours, %d mins.\r\n",
          updays, uphours, upmins);
  send_to_char(Gbuf4, ch);
}

#endif

void do_bury(P_char ch, char *argument, int cmd)
{
  bool     tried = FALSE;
  P_obj    obj_object, next_obj;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  one_argument(argument, Gbuf1);

  if (!*Gbuf1)
  {                             /*
                                 * If no arguments, i.e., just entered 'bury'
                                 */
    send_to_char("What would you like to bury?\r\n", ch);
    return;
  }
  if (str_cmp(Gbuf1, "all") == 0)
  {                             /*
                                 * If bury all
                                 */
    send_to_char("Sorry, you can't bury everything at once!\r\n", ch);
    return;
  }
  if ((str_cmp(Gbuf1, "me") == 0) || (str_cmp(Gbuf1, "self") == 0))
  {
    send_to_char("Having a bad day, are we?\r\n", ch);
    return;
  }
  if (sscanf(Gbuf1, "all.%s", Gbuf2) != 1)
  {                             /*
                                 * If not All.<obj>
                                 */
    /*
     * Scan Room for object - either <object> or #.<object>
     */
    obj_object = get_obj_in_list_vis(ch, Gbuf1, world[ch->in_room].contents);

    if (!obj_object)
    {                           /*
                                 * Can't find object
                                 */
      send_to_char("You don't see that here.\r\n", ch);
      return;
    }
    else
    {                           /*
                                 * Found object, try to bury it
                                 */
      try_to_bury(ch, obj_object);
      return;
    }
  }
  /*
   * Bury all.<object>
   */

  for (obj_object = world[ch->in_room].contents;
       obj_object; obj_object = next_obj)
  {
    next_obj = obj_object->next_content;

    if ((isname(Gbuf2, obj_object->name)) && (CAN_SEE_OBJ(ch, obj_object)))
    {
      try_to_bury(ch, obj_object);
      tried = TRUE;
    }
  }

  if (!tried)
  {                             /*
                                 * If we couldn't see or find anything
                                 */
    send_to_char("You don't see that here.\r\n", ch);
  }
  return;
}

void try_to_bury(P_char ch, P_obj obj_object)
{
  P_obj    tmp_object = NULL;
  bool     have_one = FALSE;

  if (ch->equipment[HOLD])
  {
    tmp_object = ch->equipment[HOLD];
    if (isname("shovel", tmp_object->name) ||
        isname("hoe", tmp_object->name) || isname("pick", tmp_object->name))
      have_one = TRUE;
  }
  if (!have_one)
  {
    send_to_char("Using what? Your fingers?\r\n", ch);
    return;
  }
  if (!CAN_WEAR(obj_object, ITEM_TAKE) && (obj_object->type != ITEM_CORPSE))
  {
    send_to_char("How can you bury something you can't take?! Putz!\r\n", ch);
    return;
  }
  if (GET_ITEM_TYPE(obj_object) == ITEM_CORPSE)
  {
    send_to_char
      ("Ye haven't the time, nor the energy to dig such a deep hole.\r\n",
       ch);
    return;
  }
  if (world[ch->in_room].sector_type == SECT_WATER_SWIM ||
      world[ch->in_room].sector_type == SECT_WATER_NOSWIM ||
      world[ch->in_room].sector_type == SECT_NO_GROUND ||
      world[ch->in_room].sector_type == SECT_FIREPLANE ||
      world[ch->in_room].sector_type == SECT_OCEAN ||
      world[ch->in_room].sector_type == SECT_UNDERWATER ||
      world[ch->in_room].sector_type == SECT_UNDERWATER_GR ||
      world[ch->in_room].sector_type >= SECT_UNDRWLD_WATER)
  {
    send_to_char("This appears to not be the best place fer digging.\r\n",
                 ch);
    return;
  }
  if (ch->specials.z_cord < 0 || ch->specials.z_cord > 0)
  {
    send_to_char("This appears to not be the best place fer digging.\r\n",
                 ch);
    return;
  }
  act("You bury $p.", FALSE, ch, obj_object, 0, TO_CHAR);
  act("$n buries $p.", FALSE, ch, obj_object, 0, TO_ROOM);
  CharWait(ch, 20);             /*
                                 * 20 second delay
                                 */
  SET_BIT(obj_object->extra_flags, ITEM_BURIED);
/*  writeSavedItem(obj_object);*/
  return;
}

void do_dig(P_char ch, char *argument, int cmd)
{
  P_obj    tmp_object = NULL, k;
  bool     found_something = FALSE, have_one = FALSE;

  if (ch->equipment[HOLD])
  {
    tmp_object = ch->equipment[HOLD];
    if (isname("shovel", tmp_object->name) ||
        isname("hoe", tmp_object->name) || isname("pick", tmp_object->name))
      have_one = TRUE;
  }
  if (!have_one)
  {
    send_to_char("Using what? Your fingers?\r\n", ch);
    return;
  }
  k = world[ch->in_room].contents;
  if (!k)
  {
    send_to_char("You dig up lots of dirt.\r\n", ch);
    CharWait(ch, 20);
    return;
  }
  else if (world[ch->in_room].sector_type == SECT_WATER_SWIM ||
           world[ch->in_room].sector_type == SECT_WATER_NOSWIM ||
           world[ch->in_room].sector_type == SECT_NO_GROUND ||
           world[ch->in_room].sector_type == SECT_FIREPLANE ||
           world[ch->in_room].sector_type == SECT_OCEAN ||
           world[ch->in_room].sector_type == SECT_UNDERWATER ||
           world[ch->in_room].sector_type == SECT_UNDERWATER_GR ||
           world[ch->in_room].sector_type >= SECT_UNDRWLD_WATER)
  {
    send_to_char("This appears to not be the best place fer digging.\r\n",
                 ch);
    return;
  }
  if (ch->specials.z_cord < 0 || ch->specials.z_cord > 0)
  {
    send_to_char("This appears to not be the best place fer digging.\r\n",
                 ch);
    return;
  }
  CharWait(ch, 20);
  for (; k && (!found_something || IS_TRUSTED(ch)); k = k->next_content)
  {
    if (IS_SET(k->extra_flags, ITEM_BURIED) &&
        (number(1, 100) < ((GET_C_LUK(ch) / 2) + number(1, 50))))
    {
      REMOVE_BIT(k->extra_flags, ITEM_BURIED);
/*      PurgeSavedItemFile(k);*/
      act("You dig up $p!", FALSE, ch, k, 0, TO_CHAR);
      act("$n digs up $p!", FALSE, ch, k, 0, TO_ROOM);
      found_something = TRUE;
    }
  }
  if (!found_something)
    send_to_char("You dig up lots of dirt.\r\n", ch);
}

void try_to_hide(P_char ch, P_obj obj_object)
{
  if (!CAN_WEAR(obj_object, ITEM_TAKE) && (obj_object->type != ITEM_CORPSE))
  {
    send_to_char("How can you hide something you can't take?! Putz!\r\n", ch);
    return;
  }
  if (GET_ITEM_TYPE(obj_object) == ITEM_CORPSE)
  {
    send_to_char
      ("Ye haven't the time, nor the energy to dig such a deep hole.\r\n",
       ch);
    return;
  }
  act("You hide $p.", FALSE, ch, obj_object, 0, TO_CHAR);
  act("$n hides $p.", FALSE, ch, obj_object, 0, TO_ROOM);

  CharWait(ch, PULSE_VIOLENCE << 1);

  SET_BIT(obj_object->extra_flags, ITEM_SECRET);
  return;
}

/*********************************************************
*                                                        *
*  Donate Command - by Myrrh 09/18/93                    *
*                                                        *
*********************************************************/

#define IN_WELL_ROOM(x) (world[(x)->in_room].number == WELL_ROOM)

void do_donate(P_char ch, char *argument, int cmd)
{
  P_obj    obj_object;
  P_obj    next_obj;
  bool     tried = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (ch->in_room == NOWHERE)
    return;

  if (!IN_WELL_ROOM(ch))
  {
    send_to_char("Sorry but you cannot donate here.\r\n", ch);
    return;
  }
  one_argument(argument, Gbuf1);

  if (!*Gbuf1)
  {                             /*
                                 * If no arguments, i.e., just entered
                                 * 'donate'
                                 */
    send_to_char("What would you like to donate?\r\n", ch);
    return;
  }
  if ((str_cmp(Gbuf1, "me") == 0) || (str_cmp(Gbuf1, "self") == 0))
  {
    send_to_char("Yeah, like you're worth much!\r\n", ch);
    return;
  }
  /*
   * If not donate all, or donate all.<object>
   */

  if ((str_cmp(Gbuf1, "all") != 0) && (sscanf(Gbuf1, "all.%s", Gbuf2) != 1))
  {
    /*
     * Scan Room for object - either <object> or #.<object>
     */

    obj_object = get_obj_in_list(Gbuf1, ch->carrying);

    if (!obj_object)
    {                           /*
                                 * Can't find object
                                 */
      send_to_char("You don't seem to have that.\r\n", ch);
      return;
    }
    else
    {                           /*
                                 * Found object, try to donate it
                                 */
      try_to_donate(ch, obj_object);
      return;
    }
  }
  /*
   * Donate all, or Donate all.<object>
   */

  for (obj_object = ch->carrying; obj_object; obj_object = next_obj)
  {
    next_obj = obj_object->next_content;

    if ((isname(Gbuf2, obj_object->name)) || (str_cmp(Gbuf1, "all") == 0))
    {

      /*
       * If name matches, or just 'all' was entered. Try to donate
       * it.
       */

      try_to_donate(ch, obj_object);
      tried = TRUE;
    }
  }

  if (!tried)
  {                             /*
                                 * If we couldn't find anything
                                 */
    if (str_cmp(Gbuf1, "all") == 0)
    {                           /*
                                 * and all was entered
                                 */
      send_to_char("You don't seem to have anything!\r\n", ch);
    }
    else
    {                           /*
                                 * or all.<obj> was entered
                                 */
      send_to_char("You don't seem to have any.\r\n", ch);
    }
  }
}

/****************************************************************
*                                                               *
*  try_to_donate:  tries to donate object, extracts object if   *
*                  successful, and puts it in well.             *
*                                                               *
****************************************************************/

void try_to_donate(P_char ch, P_obj obj_to_put)
{
  P_obj    obj_object, next_object, sub_object;
  int      dupes_in_well = 0;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
   * Get Well object
   */
  for (sub_object = world[ch->in_room].contents;
       sub_object && !isname("well", sub_object->name);
       sub_object = sub_object->next_content) ;
  if (!sub_object)
    return;

  /*
   * If not-droppable
   */
  if (IS_SET(obj_to_put->extra_flags, ITEM_NODROP))
  {
    sprintf(Gbuf3,
            "Donating %s?  How thoughtful....too bad it's CURSED!\r\n",
            obj_to_put->short_description);
    send_to_char(Gbuf3, ch);
    return;
  }

  if (IS_ARTIFACT(obj_to_put))
  {
    send_to_char("Donate an artifact?  What a waste!\r\n", ch);
    return;
  }

  if (GET_ITEM_TYPE(obj_to_put) == ITEM_FOOD)
  {                             /*
                                 * If food
                                 */
    sprintf(Gbuf3,
            "%s?  Donate equipment! - give food to the Homeless!\r\n",
            obj_to_put->short_description);
    send_to_char(Gbuf3, ch);
    return;
  }
  if (GET_ITEM_TYPE(obj_to_put) == ITEM_CORPSE || GET_ITEM_TYPE(obj_to_put) == ITEM_TRASH)
  {
    sprintf(Gbuf3, "%s isn't too valuable - just bury it!\r\n",
            obj_to_put->short_description);
    send_to_char(Gbuf3, ch);
    return;
  }
  /*
   * If item is a container with stuff in it
   */
  if ((GET_ITEM_TYPE(obj_to_put) == ITEM_CONTAINER ||
       GET_ITEM_TYPE(obj_to_put) == ITEM_STORAGE) && (obj_to_put->contains))
  {
    sprintf(Gbuf3, "You have to empty the %s before donating it.\r\n",
            FirstWord(obj_to_put->name));
    send_to_char(Gbuf3, ch);
    return;
  }
  /*
   * Check to see how many of the same objects are in well already
   */

  for (obj_object = sub_object->contains; obj_object;
       obj_object = next_object)
  {
    next_object = obj_object->next_content;

    /*
     * If there's already an item in the well, increment dupes_in_well
     * Unless it is a random drop - Jexni
     */
    if ((obj_object->R_num == obj_to_put->R_num) && 
        (!obj_index[obj_to_put->R_num].virtual_number == RANDOM_OBJ_VNUM))
    {
      dupes_in_well++;
    }
  }

  /*
   * Remove item from player's inventory
   */
  obj_from_char(obj_to_put, TRUE);

  /*
   * Send donate message to player.
   */
  if (IS_TRUSTED(ch))
  {
     wizlog(GET_LEVEL(ch), "%s donated %s into %s [%d]", J_NAME(ch), obj_to_put->short_description, sub_object->short_description, world[ch->in_room].number);
     logit(LOG_WIZ, "%s donated %s into %s [%d]",J_NAME(ch), obj_to_put->short_description, sub_object->short_description, world[ch->in_room].number);
     sql_log(ch, WIZLOG, "Donated %s into %s", obj_to_put->short_description, sub_object->short_description);
  }
  else
  {
    wizlog(MINLVLIMMORTAL, "%s donated %s into %s [%d]", J_NAME(ch), obj_to_put->short_description, sub_object->short_description, world[ch->in_room].number);
    logit(LOG_PLAYER, "%s donated %s into %s [%d]",J_NAME(ch), obj_to_put->short_description, sub_object->short_description, world[ch->in_room].number);
    sql_log(ch, PLAYERLOG, "Donated %s into %s", obj_to_put->short_description, sub_object->short_description);
  }

  act("You donate $p - thank you!", FALSE, ch, obj_to_put, 0, TO_CHAR);
  
  /*
   * If max_dupes in well not exceeded then put in well, otherwise nuke
   * object
   */
  if (dupes_in_well < MAX_DUPES_IN_WELL)
  {
    obj_to_obj(obj_to_put, sub_object);
  }
  else
  {
    extract_obj(obj_to_put, TRUE);
  }
}

void do_fly(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_INPUT_LENGTH], Gbuf1[MAX_INPUT_LENGTH];
  P_char   mount, rider = NULL;
  struct follow_type *k, *next_dude;
  struct zone_data *zone = 0;
  int      saw_map = 0, sect, oldz, newz;

  zone = &zone_table[world[ch->in_room].zone];
  /* mounted chars can use this to order their mounts as well */
  if (mount = get_linked_char(ch, LNK_RIDING))
  {
    ch = mount;                 /* simply change who were dealing with */
    rider = ch;
  }
  if (!buf)
  {
    send_to_char("Fly what, where why or who?!?\r\n", ch);
    return;
  }
  
  argument_interpreter(argument, buf, Gbuf1);

  if (!affected_by_spell(ch, SPELL_FLY) && !IS_AFFECTED(ch, AFF_FLY) &&
      !IS_TRUSTED(ch) && !(IS_AFFECTED(ch, AFF_LEVITATE) &&
                           !str_cmp(buf, "land")))
  {
    send_to_char("You don't have the means to fly!\r\n", ch);
    return;
  }
  
  if ( !str_cmp(buf, "up") && (!IS_MAP_ROOM(ch->in_room) || !OUTSIDE(ch) ) )
  {
    send_to_char("You must be outdoors to fly.\r\n", ch);
    return;
  }
  
  if (ch->specials.z_cord < 0)
  {
    send_to_char("Umm, try surfacing first.\r\n", ch);
    return;
  }
  sect = world[ch->in_room].sector_type;
  if ( /*(sect == SECT_CITY) */ CHAR_IN_TOWN(ch) && !str_cmp(buf, "up"))
  {
    send_to_char("You cannot fly in the city!\r\n", ch);
    return;
  }

  if ((sect == SECT_UNDERWATER) || (sect == SECT_UNDERWATER_GR) ||
      (sect == SECT_FIREPLANE) ||
      ((sect >= SECT_AIR_PLANE) && (sect <= SECT_ASTRAL)))
  {
    send_to_char
      ("Heh, explain how exactly that works and we'll let ya do it.\r\n", ch);
    return;
  }

  if (LIMITED_TELEPORT_ZONE(ch->in_room) && !str_cmp(buf, "up"))
  {
    send_to_char
      ("The winds are too strong..  Flying here would be folly.\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_BOUND))
  {
    send_to_char
      ("You try to flap your arms, but the ropes are too tight.\r\n", ch);
    return;
  }

  oldz = ch->specials.z_cord;

  if (!str_cmp(buf, "up"))
  {

    /* if there's already an up exit here, let's move em up it */
    if (EXIT(ch, UP))
    {
      do_move(ch, NULL, CMD_UP);
      ch->specials.z_cord = 0;
      return;
    }

    if ((ch->specials.z_cord >= 2) && !IS_TRUSTED(ch) /*&& !IS_NPC(ch) */ )
    {
      send_to_char
        ("The wind currents are much too strong at that altitude.\r\n", ch);
      return;
    }
    else
    {
      act("$n flies up higher.", TRUE, ch, 0, 0, TO_ROOM);
      
      int level = atoi(Gbuf1);
      
      if( level > 0 )
      {
        ch->specials.z_cord = level;
      }
      else
      {      
        ch->specials.z_cord++;
      }
      
      act("You fly up higher.", FALSE, ch, 0, 0, TO_CHAR);
      if (rider)
      {
        act("You fly up higher.", FALSE, ch, 0, rider, TO_VICT);
      }
      act("$n flies up from below.", TRUE, ch, 0, 0, TO_ROOM);
/*
      if (!IS_MAP_ROOM(ch->in_room) && ch->specials.z_cord > 2 && OUTSIDE(ch))
        if (real_room0(maproom_of_zone(world[zone->real_bottom].zone))) {
          char_from_room(ch);
          char_to_room(ch,
              real_room0(maproom_of_zone(world[zone->real_bottom].zone)), -4);

          saw_map = 1;
          if (rider) {
            char_from_room(rider);
            char_to_room(rider, ch->in_room, -4);
            saw_map = 1;
          }
        }
*/
    }
  }
  else if (!str_cmp(buf, "down"))
  {
    /* if there's already a down exit here, let's move em down it */
    if (EXIT(ch, DOWN))
    {
      do_move(ch, NULL, CMD_DOWN);
      ch->specials.z_cord = 0;
      return;
    }
    if (ch->specials.z_cord < 1)
    {
      send_to_char("Umm, you've landed already, bucko.\r\n", ch);
      return;
    }
    else
    {
      act("$n flies down lower.", TRUE, ch, 0, 0, TO_ROOM);
      ch->specials.z_cord--;
      act("You fly down lower.", FALSE, ch, 0, 0, TO_CHAR);
      if (rider)
        act("You fly down lower.", FALSE, ch, 0, rider, TO_VICT);
      act("$n flies down from above.", TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  else if (!str_cmp(buf, "land"))
  {
    if (ch->specials.z_cord < 1)
    {
      send_to_char("Umm, you've landed already, bucko.\r\n", ch);
      return;
    }
    else
    {
      act("$n flies down lower at a rapid clip.", TRUE, ch, 0, 0, TO_ROOM);
      ch->specials.z_cord = 0;
      act("You land upon the ground.", FALSE, ch, 0, 0, TO_CHAR);
      if (rider)
        act("You land upon the ground.", FALSE, ch, 0, rider, TO_VICT);
      act("$n lands $s flight.", TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  /* flying mounts, et al */
  if (rider)
    rider->specials.z_cord = ch->specials.z_cord;
/*
  if (!saw_map) {
    if (rider)
      do_look(rider, 0, -4);
    do_look(ch, 0, -4);
  }
*/
  /* Followers */
  if (ch->followers)
  {
    newz = ch->specials.z_cord;
    ch->specials.z_cord = oldz;

    for (k = ch->followers; k; k = next_dude)
    {
      next_dude = k->next;
      if ((affected_by_spell(k->follower, SPELL_FLY) ||
           IS_AFFECTED(k->follower, AFF_FLY)) &&
          (k->follower->in_room == ch->in_room) &&
          (k->follower->specials.z_cord == ch->specials.z_cord) &&
          !IS_FIGHTING(k->follower) &&
          (GET_POS(k->follower) == POS_STANDING) && CAN_SEE(k->follower, ch))
      {
/*        act("$N follows $n.", TRUE, ch, 0, k->follower, TO_ROOM);*/
//        k->follower->specials.z_cord = newz;
//        act("$N follows you.", TRUE, ch, 0, k->follower, TO_CHAR);
//        act("$N follows $n.", TRUE, ch, 0, k->follower, TO_NOTVICT);
        act("You follow $n.", TRUE, ch, 0, k->follower, TO_VICT);
        sprintf(Gbuf1, "fly %s", argument);
        command_interpreter(k->follower, Gbuf1);
      }
    }

    ch->specials.z_cord = newz;
  }
}


void do_swim(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_INPUT_LENGTH];
  P_char   mount, rider = NULL;
  struct follow_type *k, *next_dude;
  struct zone_data *zone = 0;
  int      saw_map = 0, sect;

  zone = &zone_table[world[ch->in_room].zone];
  /* mounted chars can use this to order their mounts as well */
  if (mount = get_linked_char(ch, LNK_RIDING))
  {
    ch = mount;
    rider = ch;
  }
  if (!buf)
  {
    send_to_char("Swim what, where why or who?!?\r\n", ch);
    return;
  }
  one_argument(argument, buf);
  if (GET_VITALITY(ch) < 10 && !IS_TRUSTED(ch))
  {
    send_to_char("You are much to exhausted to swim!\r\n", ch);
    return;
  }
  if (!IS_WATER_ROOM(ch->in_room))
  {
    if (ch->specials.z_cord < 0)        /* in case of errors */
      ch->specials.z_cord = 0;
    send_to_char("You must be in water to swim.\r\n", ch);
    return;
  }
  if (ch->specials.z_cord > 0)
  {
    send_to_char("Umm, try landing first.\r\n", ch);
    return;
  }
  sect = world[ch->in_room].sector_type;

  if ((sect >= SECT_AIR_PLANE) && (sect <= SECT_ASTRAL))
  {
    send_to_char
      ("Heh, explain exactly how that works and we'll let ya do it.\r\n", ch);
    return;
  }

  if (!str_cmp(buf, "up"))
  {
    /* if there's already an up exit here, let's move em up it */
    if (EXIT(ch, CMD_UP - 1))
    {
      do_move(ch, NULL, CMD_UP);
      ch->specials.z_cord = 0;
      return;
    }
    if (ch->specials.z_cord == 0)
    {
      send_to_char("You've already surfaced doofus!\r\n", ch);
      return;
    }
    else
    {
      act("$n swims up higher.", TRUE, ch, 0, 0, TO_ROOM);
      ch->specials.z_cord++;
      act("You swim up higher.", FALSE, ch, 0, 0, TO_CHAR);
      if (rider)
      {
        act("You swim up higher.", FALSE, ch, 0, rider, TO_VICT);
      }
      act("$n swims up higher.", TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  else if (!str_cmp(buf, "down"))
  {
    /* if there's already a down exit here, let's move em to it */
    if (EXIT(ch, CMD_DOWN - 1))
    {
      do_move(ch, NULL, CMD_DOWN);
      ch->specials.z_cord = 0;
      return;
    }
    if (ch->specials.z_cord == -(distance_from_shore(ch->in_room)))
    {
      send_to_char("Umm, you've reached the floor already, bucko.\r\n", ch);
      return;
    }
    else
    {
      act("$n swims down lower.", TRUE, ch, 0, 0, TO_ROOM);
      ch->specials.z_cord -= 1;
      act("You swim down lower.", FALSE, ch, 0, 0, TO_CHAR);
      if (rider)
        act("You swim down lower.", FALSE, ch, 0, rider, TO_VICT);
      act("$n swims down lower.", TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  else if (!str_cmp(buf, "surface"))
  {
    if (ch->specials.z_cord > -1)
    {
      send_to_char("Umm, you've surfaced already, bucko.\r\n", ch);
      return;
    }
    else
    {
      act("$n swims down lower.", TRUE, ch, 0, 0, TO_ROOM);
      ch->specials.z_cord = 0;
      act("You swim to the surface.", FALSE, ch, 0, 0, TO_CHAR);
      if (rider)
        act("You swim to the surface.", FALSE, ch, 0, rider, TO_VICT);
      act("$n bobs to the surface of the water.", TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  /* swiming mounts, et al */
  if (rider)
    rider->specials.z_cord = ch->specials.z_cord;
  /* Followers */
  if (ch->followers)
    for (k = ch->followers; k; k = next_dude)
    {
      next_dude = k->next;
      if (((IS_NPC(k->follower) &&
            IS_SET(k->follower->specials.act, ACT_CANSWIM)) ||
           (IS_PC(k->follower) && GET_VITALITY(k->follower) >= 10)) &&
          k->follower->in_room == ch->in_room)
      {
        act("$N follows $n.", TRUE, ch, 0, k->follower, TO_ROOM);
        k->follower->specials.z_cord = ch->specials.z_cord;
        act("$N follows you.", TRUE, ch, 0, k->follower, TO_CHAR);
        act("$N follows $n.", TRUE, ch, 0, k->follower, TO_ROOM);
        act("You follow $n.", TRUE, ch, 0, k->follower, TO_VICT);
      }
    }
  if (!saw_map)
  {
    if (rider)
      do_look(rider, 0, -4);
    do_look(ch, 0, -4);
  }
}


void do_suicide(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH];

  if (!ch)
    return;

  if (IS_NPC(ch))
  {
    send_to_char("Your not skilled enough to kill yourself\r\n", ch);
    return;
  }
 /* if (GET_LEVEL(ch) > 20)
  {
    send_to_char("Suicide is not an option at your level, sorry.\r\n", ch);
    ch->desc->confirm_state = CONFIRM_NONE;
    return;
  }*/

 if (IS_TRUSTED(ch))
  {
    send_to_char("Suicide is not an option at your level, sorry.\r\n", ch);
    ch->desc->confirm_state = CONFIRM_NONE;
    return;
  }

  if ((world[ch->in_room].room_flags & JAIL) || IS_AFFECTED(ch, AFF_BOUND))
  {
    send_to_char("You can't do that in your current state.\r\n", ch);
    ch->desc->confirm_state = CONFIRM_NONE;
    return;
  }

  if ((IS_AFFECTED(ch, AFF_SLEEP)) || (GET_STAT(ch) == STAT_SLEEPING))
  {
    send_to_char("Please wake up to kill yourself.\r\n", ch);
    return;
  }

  // No suiciding to escape being fragged.
  if( affected_by_spell(ch, TAG_PVPDELAY) )
  {
    send_to_char("There is too much adrenaline pumping through your body right now.\r\n", ch);
    return;
  }

  if (!command_confirm)
  {
    if (ch->desc)
    {
      sprintf(buf, "WARNING: You are about to take your own life.\r\n"
              "Please confirm that you wish to do this!(Yes/No) [No]:\r\n");
      send_to_char(buf, ch);
      return;
    }
  }
  else if (ch->desc)
    ch->desc->confirm_state = CONFIRM_NONE;

  statuslog(ch->player.level, "%s committed suicide at %s [%d]",
            GET_NAME(ch), world[ch->in_room].name, world[ch->in_room].number);
  die(ch, ch);
}

void do_climb(P_char ch, char *argument, int cmd)
{
  struct affected_type af;

  if (!ch || !ch->desc || !CAN_ACT(ch))
    return;

  if (IS_NPC(ch) || !GET_CHAR_SKILL(ch, SKILL_CLIMB))
  {
    send_to_char("You would probably fall, best not attempt this.\r\n", ch);
    return;
  }

  if (!affected_by_spell(ch, SKILL_CLIMB))
  {
    send_to_char("You survey the climb ahead...\r\n", ch);
    bzero(&af, sizeof(af));
    af.type = SKILL_CLIMB;
    af.duration = 10;
    affect_to_char(ch, &af);
    notch_skill(ch, SKILL_CLIMB, 17);
  }
}

P_obj blood_in_room_with_me(P_char ch)
{
  P_obj    obj;

  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    if (obj->R_num == real_object(4))
      return obj;
  }
  return NULL;
}

void do_lick(P_char ch, char *argument, int cmd)
{
  P_obj    obj;

  if (!ch)
    return;

  if ((GET_RACE(ch) != RACE_PVAMPIRE) || !(obj = blood_in_room_with_me(ch))
      || !argument || !isname(argument, "blood"))
  {
    do_action(ch, argument, cmd);
    return;
  }

  extract_obj(obj, TRUE);
  obj = NULL;

  act("$n licks some blood from the floor...", TRUE, ch, 0, ch, TO_NOTVICT);
  act("You lick some blood from the floor. Refreshing.", TRUE, ch, 0, ch,
      TO_VICT);

  spell_cure_serious(45, ch, 0, SPELL_TYPE_SPELL, ch, 0);

  CharWait(ch, 10);

  return;

}

void do_nothing(P_char ch, char *argument, int cmd)
{
  return;
}

void do_blood_scent(P_char ch, char *argument, int cmd)
{
  struct affected_type af;

  if (!GET_CHAR_SKILL(ch, SKILL_BLOOD_SCENT))
  {
    send_to_char("As if your senses are even remotely tuned to that sort of thing. Wierdo.\r\n", ch);
    return;
  }
  if (GET_CHAR_SKILL(ch, SKILL_BLOOD_SCENT) < number(1, 100))
  {
    send_to_char("You sniff around but cant smell anything special.\r\n", ch);
    notch_skill(ch, SKILL_BLOOD_SCENT, 7.69);
    CharWait(ch, (2 * PULSE_VIOLENCE));
    return;
  }

  
  if (affected_by_spell(ch, SKILL_BLOOD_SCENT))
  {
    send_to_char("Blood, blood, yes you can almost taste it!\r\n", ch);
    CharWait(ch, (2 * PULSE_VIOLENCE));
    return;
  }

  memset(&af, 0, sizeof(struct affected_type));

  act("You grin evilly as the smell of &+rblood&n fills you.", FALSE, ch, 0,
      0, TO_CHAR);
  act("$n licks his cracked lips and grins evilly.", FALSE, ch, 0, 0,
      TO_ROOM);
  CharWait(ch, PULSE_VIOLENCE);
  af.type = SKILL_BLOOD_SCENT;
  af.bitvector5 = AFF5_BLOOD_SCENT;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
  af.duration = WAIT_SEC * GET_CHAR_SKILL(ch, SKILL_BLOOD_SCENT);
  affect_to_char(ch, &af);

  /* I don't see the point of this...so lets comment it out!
  if (GET_CHAR_SKILL(ch, SKILL_BLOOD_SCENT) > number(1, 100))
  {
    af.flags |= AFFTYPE_CUSTOM1;
    af.duration >>= 1;
    affect_to_char(ch, &af);
    notch_skill(ch, SKILL_BLOOD_SCENT, 7.69);
  }*/ 
}

void ascend_theurgist(P_char ch)
{
  P_char teacher;
  char buff[64];
  int i;

  if (!ch)
  {
    logit(LOG_EXIT, "ascend_theurgist called in actoth.c with no ch");
    raise(SIGSEGV);
  }
  if (IS_NPC(ch))
    return;

  if (IS_TRUSTED(ch))
  {
    send_to_char("This would be really really really really really dumb........\n\r",ch);
    return;
  }

  if(!(teacher = FindTeacher(ch)))
  {
    send_to_char("You need a teacher to help you with this.......\n\r", ch);
    return;
  }

  if(ch->only.pc->epics < (int) get_property("ascend.epicCost.Eladrin", 250))
  {
    sprintf(buff, "It costs &+W%d&n epics to ascend...\n", (int) get_property("descend.epicCost.Eladrin", 10));
    send_to_char(buff, ch);
    return;
  }

  if((GET_CLASS(ch, CLASS_THEURGIST) && !GET_CLASS(teacher, CLASS_THEURGIST)))
  {
    send_to_char("How about finding the appropriate teacher first?\n", ch);
    return;
  }

  for (i = 0; i < MAX_SKILLS; i++)
  {
    if( !IS_TRADESKILL(i) && !IS_EPIC_SKILL(i) )
    {
      ch->only.pc->skills[i].learned = 0;
    }
  }
  NewbySkillSet(ch, FALSE);
  ch->points.max_mana = 0;
  do_start(ch, 1);

  int k = 0;
  P_obj temp_obj;
  for (k = 0; k < MAX_WEAR; k++)
  {
    temp_obj = ch->equipment[k];
    if(temp_obj)
      obj_to_char(unequip_char(ch, k), ch);
  }

  GET_SIZE(ch) = SIZE_MEDIUM;
  GET_RACE(ch) = RACE_ELADRIN;
  ch->player.m_class = CLASS_THEURGIST;

  send_to_char("You feel a chill and realize that you are naked.\r\n", ch);
  generate_desc(ch);

  // GET_AGE does not return a changeable variable.
  ch->player.time.birth = time(NULL) - 500 * SECS_PER_MUD_YEAR;

  GET_VITALITY(ch) =  GET_MAX_VITALITY(ch) = 120;
  forget_spells(ch, -1);
  ch->player.spec = 0;
  ch->player.secondary_class = 0;
  ch->only.pc->epics = MAX(0, ch->only.pc->epics - (int) get_property("ascend.epicCost.Eladrin", 10));
  // Lets not home them on the map.
  //GET_HOME(ch) = GET_BIRTHPLACE(ch) = GET_ORIG_BIRTHPLACE(ch) = ch->in_room;
}

void do_ascend(P_char ch, char *arg, int cmd)
{
  int spec;
  char buffer[256];
  
  if(!ch)
  {
    logit(LOG_EXIT, "do_ascend called in actoth.c with no ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  {
    if(!IS_NPC(ch))
    {
	send_to_char("&+WThis is too powerful an enchantment for you to master...&n\n\r",ch);
      return;
    }
    if(IS_TRUSTED(ch))
    {
      send_to_char
        ("This would be really really really really really dumb........\n\r",ch);
      return;
    }
    if (affected_by_spell(ch, TAG_RACE_CHANGE))
    {
      send_to_char("You cannot ascend until you're in your true form.\n\r", ch);
      return;
    }
    if (GET_CLASS(ch, CLASS_THEURGIST))
    {  
      ascend_theurgist(ch);
      return;
    }
    
    if(!GET_CLASS(ch, CLASS_PALADIN) &&
      !GET_CLASS(ch, CLASS_AVENGER))
    {
      send_to_char("You raise your hands towards the skies and await a miracle.\n", ch);
      return;
    }

    if(world[ch->in_room].number == 13272)
    {
      spec = SPEC_LIGHTBRINGER;
    }
    else if(world[ch->in_room].number == 75610)
    {
      spec = SPEC_INQUISITOR;
    }
    else
    {
      send_to_char("Ascension can happen only in a holy place.\n", ch);
      return;
    }

    if(GET_CLASS(ch, CLASS_AVENGER))
    {
      ch->player.spec = spec;
      send_to_char(
        "You pray to your god, asking for judgement over your past deeds,\n"
        "seeking further enlightment. A &+Wholy glow&n seems to encase you,\n"
        "lifting your spirits and heightening your awareness.\n"
        "Your prayers have been answered, as you ascend into the ranks of\n"
        "the holy army, from this day on you will be an "
        "&+WAvenger&n of divine law.\n\n", ch);
      sprintf(buffer,
        "You hear a loud voice exclaiming, '&+WWelcome my child, you shall\n"
        "&+Wnow be the avenging hand of %s,\n"
        "&+Wthe %s &+Wfor his enemies!'",
        get_god_name(ch),
        GET_SPEC_NAME(ch->player.m_class, spec-1));
      send_to_char(buffer, ch);
      ch->only.pc->epics = MAX(0, ch->only.pc->epics - (int) get_property("ascend.epicCost", 250));
      return;
    }

    if(GET_LEVEL(ch) < 40)
    {
      send_to_char("You hear a loud voice exclaiming, '&+WYou are not ready yet!&n'\n", ch);
      return;
    }
    if(ch->only.pc->epics < (int) get_property("ascend.epicCost", 10)) 
    {
      sprintf(buffer, "&+WYou must first prove yourself worthy! The transformation will consume &n%d&+W epic points.\n",
        (int) get_property("ascend.epicCost", 10));
      send_to_char(buffer, ch);
      return;
    }

    forget_spells(ch, -1);
    ch->player.spec = spec;
    ch->player.secondary_class = 0;
    GET_SIZE(ch) = SIZE_MEDIUM;
    GET_RACE(ch) = RACE_AGATHINON;
    generate_desc(ch);
    // GET_AGE does not return a changeable variable.
    ch->player.time.birth = time(NULL) - 500 * SECS_PER_MUD_YEAR;
    ch->player.m_class = CLASS_AVENGER;
    do_start(ch, 1);
    ch->only.pc->epics = MAX(0, ch->only.pc->epics - (int) get_property("ascend.epicCost", 250));
    send_to_char(
        "You pray to your god, asking for judgement over your past deeds,\n"
        "seeking further enlightment. A &+Wholy glow&n seems to encase you,\n"
        "lifting your spirits and heightening your awareness.\n"
        "Your prayers have been answered, as you ascend into the ranks of\n"
        "the holy army, from this day on you will be an "
        "&+WAvenger&n of divine law.\n\n", ch);
    sprintf(buffer,
        "You hear a loud voice exclaiming, '&+WWelcome my child, you shall\n"
        "&+Wnow be the avenging hand of %s,\n"
        "&+Wthe %s &+Wfor his enemies!'",
        get_god_name(ch),
        GET_SPEC_NAME(ch->player.m_class, spec-1));
    send_to_char(buffer, ch);
  }
}

void do_descend(P_char ch, char *arg, int cmd)
{
  P_char teacher;
  int SELECTION, i = 0;
  int cost = 0;
  char second_arg[MAX_INPUT_LENGTH], third_arg[MAX_INPUT_LENGTH];
  char buff[64];

    if(!GET_CLASS(ch, CLASS_NECROMANCER))
    {
      send_to_char("Your convictions aren't appropriate for this endeavour.\n", ch);
      return;
    }

    if(GET_LEVEL(ch) < 50)
    {
     send_to_char("You must be level &+L50&n in order to descend.\r\n", ch);
     return;
    }

  /*  if(GET_RACE(ch) == RACE_PLICH)
    {
      send_to_char("You could not get any &+Ldarker&n if you tried.\n", ch);
      return;
    }*/

  //Check Items
  int tome = vnum_in_inv(ch, 58424);
  int book = vnum_in_inv(ch, 500032);
  int orbs = vnum_in_inv(ch, 400231);

  if  (
	!tome ||
	!book ||
	!(orbs > 4))
   {
    send_to_char("You are missing a vital &+Lcomponent&n needed to &+cdescend&n farther into &+Ldarkness&n.\r\n", ch);
    return;
   }

  //Do descend
 if(GET_CLASS(ch, CLASS_NECROMANCER))
    {
      act
      ("&+L&+LDeath&n &+Lturns to you, $S stare piercing your very &+Wsoul&+L.\n"
       "&+LSoftly $E whispers '&+YI see your lust for the darkness is not quenched.  For your dedication\n"
       "&+YI shall grant you the unspeakable &+rpower &+Lof the &+Rdread&+Y.&+L'\n\n"
       "&+L&+LDeath&n &+Lapproaches and stands before you, softly chanting\n"
       "&+Lwhile waving a small, curved &+Csacrificial &+Wkris &+Lbefore your chest. The chanting of\n"
       "&+Lyour dark master rises to a crescendo as $E raises the &+wdagger &+Lhigh and brings it down\n" 
       "&+Lfiercely upon your breast.\n\n"
       "&+LIts sharp blade glides easily through your &+Rflesh&+L, freeing your &+rblood &+Lfrom the shell\n"
       "&+Lof your body. Your vision fades as your eyes fill with stygian clouds of smoke and vapor.\n"       
       "&+L&+RS&+re&+Ra&+rr&+Ri&+rn&+Rg pain &+Lrages through your body, awakening nerves and alerting the senses.\n\n"
       "&+L&+LDeath&n &+Lspeaks once more, '&+YLet &+Ldarkness &+Ybe your &+Wsight&+Y.\n"
       "&+YRely no longer on &+rmortal &+wsenses&+Y.&+L'\n\n"
       "&+LVisions of d&+we&+Wa&+wt&+Lh and de&+rst&+Rru&+rct&+Lion &+Lfill your mind and temper your resolve.\n"
       "&+LA wave of nausea overcomes your body, followed by a &+Rtingling &+Lin your head.\n"
       "&+LYou fight off the ailments and realize that your new &+wvision &+Lhas returned,\n"
       "&+Lalong with a sense of &+mpower &+Land &+rduty&+L.\n", FALSE, ch, 0, 0, TO_CHAR);
      act
      ("&+L&+LDeath&n &+Lturns to $n &+Land stares, piercing $s very &+Wsoul&+L.\n"
       "&+LSoftly $E whispers '&+YI see your lust for the darkness is not quenched.  For your dedication\n"
       "&+YI shall grant you the unspeakable &+rpower &+Lof the &+Rdread&+Y.&+L'\n\n"
       "&+L&+LDeath&n &+Lapproaches $n &+Land stands before $m&+L, softly\n"
       "&+Lchanting while waving a small, curved &+Csacrificial &+Wkris &+Lbefore $s chest. The\n"
       "&+Lchanting of $s dark master rises to a crescendo as $E raises the &+wdagger &+Lhigh and\n"
       "&+Lbrings it down fiercely upon $s breast.\n\n"
       "&+LIts sharp blade glides easily through $s &+Rflesh&+L, freeing $s &+rblood &+Lfrom the shell\n"
       "&+Lof $s body. $n's eyes cloud over with a stygian influx of smoke and vapor.\n"       
       "&+L$n &+Lwrithes in enormous pain as the transformation begins to take hold of $s body.\n\n"
       "&+LDeath&n &+Lspeaks once more, '&+YLet &+Ldarkness &+Ybe your &+Wsight&+Y... rely\n"
       "&+Yno longer on &+rmortal &+wsenses&+Y.&+L'\n\n"
       "&+LWith the rigors and torment of the &+rtransformation &+Lcomplete, $n &+Lraises $s head\n"
       "&+Land grins with newfound &+Rmalevolence&+L.", FALSE, ch, 0, 0, TO_ROOM);
    }

  vnum_from_inv(ch, 58424, 1);
  vnum_from_inv(ch, 500032, 1);
  vnum_from_inv(ch, 400231, 5);
    forget_spells(ch, -1);
    GET_HOME(ch) = GET_BIRTHPLACE(ch) = GET_ORIG_BIRTHPLACE(ch) = 98735;
    char_from_room(ch);
    char_to_room(ch, real_room(98735), 0);
    GET_RACEWAR(ch) = 3;
    GET_SIZE(ch) = SIZE_MEDIUM;
    GET_RACE(ch) = RACE_PLICH;
    ch->player.m_class = CLASS_NECROMANCER;
    ch->player.secondary_class = CLASS_SORCERER;
    secede_asc(ch); //leave your guild!
    ch->only.pc->frags = 0;
    ch->only.pc->epics = 0;

  for (i = 0; i < MAX_SKILLS; i++)
  {
    if( !IS_TRADESKILL(i) && !IS_EPIC_SKILL(i) )
    {
      ch->only.pc->skills[i].learned = 0;
    }
  }
  ch->points.max_mana = 0;
  ch->points.max_vitality = 0;
  NewbySkillSet(ch, FALSE);
  do_start(ch, 0);
  do_save_silent(ch, 1);
}

void do_old_descend(P_char ch, char *arg, int cmd)
{
  P_char teacher;
  int SELECTION, i = 0;
  const int INVALID = 0;
  const int WARRIOR = 1;
  const int MERC = 2;
  const int SORC  = 3;
  const int NECRO = 4;
  const int DREAD = 5;
  const int ASS = 6;
  const int THIEF = 7;
  const int CONJ = 8;
  const int  ILLU = 9;
  int cost = 0;
  char second_arg[MAX_INPUT_LENGTH], third_arg[MAX_INPUT_LENGTH];
  char buff[64];

  if(!ch)
  {
    logit(LOG_EXIT, "do_descend called in actoth.c with no ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  { 
    if(!IS_NPC(ch)) //disabling
    {
	send_to_char("&+LThese powers are too dark for you to master...&n\n\r",ch);
      return;
    }
    if(IS_TRUSTED(ch))
    {
      send_to_char
        ("This would be really really really really really dumb........\n\r",ch);
      return;
    }
    if (affected_by_spell(ch, TAG_RACE_CHANGE))
    {
      send_to_char("You cannot descend until you're in your true form.\n\r", ch);
      return;
    }
    if(!GET_CLASS(ch, CLASS_ANTIPALADIN) &&
      !GET_CLASS(ch, CLASS_NECROMANCER))
    {
      send_to_char("Your convictions aren't appropriate for this endeavour.\n", ch);
      return;
    }

    argument_interpreter(arg, second_arg, third_arg);

    if(GET_LEVEL(ch) < 40)
    {
      send_to_char("You do not have enough experience to descend into the depths of evil.\r\n" ,ch);
      return;
    }
    if((GET_CLASS(ch, CLASS_ANTIPALADIN) && 
	(ch->only.pc->epics < (int) get_property("descend.epicCost", 10))) ||
       (GET_CLASS(ch, CLASS_NECROMANCER) &&
	(ch->only.pc->epics < (int) get_property("descend.epicCost.Lich", 250))))
    {
      sprintf(buff, "It costs &+W%d&n epics to descend...\n", (GET_CLASS(ch, CLASS_ANTIPALADIN) ? (int) get_property("descend.epicCost", 10) : (int) get_property("descend.epicCost.Lich", 250))) ;
      send_to_char(buff, ch);
      return;
    }
    if(GET_CLASS(ch, CLASS_ANTIPALADIN))
    {
      SELECTION = DREAD;
      cost = get_property("descend.epicCost", 2500);
    }
    if(GET_CLASS(ch, CLASS_NECROMANCER))
    {
      SELECTION = NECRO;
      cost = get_property("descent.epicCost.Lich", 250);
    }
      /*
     if (!str_cmp(second_arg,  "Warrior")){
       wizlog(56, "1second_arg = %s", second_arg);
       if (GET_CLASS(ch, CLASS_NECROMANCER) && GET_LEVEL(ch) > 34){
         SELECTION = WARRIOR;

       }
       else
         SELECTION = INVALID;
     }

      if (!str_cmp(second_arg,  "sorcerer")){
       wizlog(56, "2second_arg = %s", second_arg);
        if (GET_CLASS(ch, CLASS_NECROMANCER) && GET_LEVEL(ch) > 34){
        SELECTION = SORC;
        }
        else
          SELECTION = INVALID;
      }

     if (!str_cmp(second_arg,  "mercenary")){
       wizlog(56, "3second_arg = %s", second_arg);
        if (GET_CLASS(ch, CLASS_NECROMANCER) && GET_LEVEL(ch) > 34){
          SELECTION = MERC;
        }
        else
        SELECTION = INVALID;

     } */

    /*
     if (!str_cmp(second_arg,  "anti-paladin"))
        SELECTION = ANTI;

    if (!str_cmp(second_arg,  "dreadlord")){
        if ((GET_RACE(ch) == RACE_PDKNIGHT || GET_RACE(ch) == RACE_WIGHT) &&
          GET_LEVEL(ch) > 49){
          SELECTION = DREAD;
        }
        else
          SELECTION = INVALID;
    }

    if (!str_cmp(second_arg,  "assassin")){
      wizlog(56, "4second_arg = %s", second_arg);
      if (GET_RACE(ch) == RACE_REVENANT && GET_LEVEL(ch) > 49){
        SELECTION = ASS;
      }
      else
        SELECTION = INVALID;
    }
   if (!str_cmp(second_arg,  "thief")){
     wizlog(56, "4second_arg = %s", second_arg);
     if (GET_RACE(ch) == RACE_REVENANT && GET_LEVEL(ch) > 49){
       SELECTION = THIEF;
     }
     else
       SELECTION = INVALID;
   }


   if (!str_cmp(second_arg,  "conjurer")){
      wizlog(56, "4second_arg = %s", second_arg);
      if (GET_RACE(ch) == RACE_PVAMPIRE && GET_LEVEL(ch) > 49){
        SELECTION = CONJ;
      }
      else
        SELECTION = INVALID;
    }
    if (!str_cmp(second_arg,  "illusionist")){
      wizlog(56, "4second_arg = %s", second_arg);
      if (GET_RACE(ch) == RACE_PVAMPIRE && GET_LEVEL(ch) > 49){
        SELECTION = ILLU;
      }
      else
        SELECTION = INVALID;
    }*/


    if(!SELECTION)
    {
      send_to_char("Invalid selection, pls read help descend once more..",ch);
      return;
    }
    if(!(teacher = FindTeacher(ch)))
    {
      send_to_char("You need a teacher to help you with this.......\n\r", ch);
      return;
    }
    if((GET_CLASS(ch, CLASS_ANTIPALADIN) && !GET_CLASS(teacher, CLASS_ANTIPALADIN)) ||
       (GET_CLASS(ch, CLASS_NECROMANCER) && !GET_CLASS(teacher, CLASS_NECROMANCER)))
    {
      send_to_char("How about finding the appropriate teacher first?\n", ch);
      return;
    }
    //DEMOTE CODE
    for (i = 0; i < MAX_SKILLS; i++)
    {
      if( !IS_TRADESKILL(i) && !IS_EPIC_SKILL(i) )
      {
        ch->only.pc->skills[i].learned = 0;
      }
    }
    NewbySkillSet(ch, FALSE);
    ch->points.max_mana = 0;
    do_start(ch, 1);

      int k = 0;
      P_obj temp_obj;
      for (k = 0; k < MAX_WEAR; k++)
      {
        temp_obj = ch->equipment[k];
        if(temp_obj)
          obj_to_char(unequip_char(ch, k), ch);
      }

      switch(SELECTION)
    {
      case WARRIOR:
        GET_SIZE(ch) = SIZE_HUGE;
        GET_RACE(ch) = RACE_WIGHT;
        ch->player.m_class = CLASS_WARRIOR;
        break;
      case SORC:
        GET_SIZE(ch) = SIZE_MEDIUM;
        GET_RACE(ch) = RACE_PVAMPIRE;
        ch->player.m_class = CLASS_SORCERER;
        break;
      case MERC:
        GET_SIZE(ch) = SIZE_LARGE;
        GET_RACE(ch) = RACE_REVENANT;
        ch->player.m_class = CLASS_MERCENARY;
        break;
      case NECRO:
        GET_SIZE(ch) = SIZE_MEDIUM;
        GET_RACE(ch) = RACE_PLICH;
        ch->player.m_class = CLASS_NECROMANCER;
        break;
      case DREAD:
        GET_SIZE(ch) = SIZE_MEDIUM;
        GET_RACE(ch) = RACE_PVAMPIRE;
        ch->player.m_class = CLASS_DREADLORD;
        break;
      case ASS:
        GET_SIZE(ch) = SIZE_MEDIUM;
        GET_RACE(ch) = RACE_PSBEAST;
        ch->player.m_class = CLASS_ASSASSIN;
        break;
      case THIEF:
        GET_SIZE(ch) = SIZE_SMALL;
        GET_RACE(ch) = RACE_SHADE;
        ch->player.m_class = CLASS_THIEF;
       break;
      case ILLU:
        GET_SIZE(ch) = SIZE_SMALL;
        GET_RACE(ch) = RACE_SHADE;
        ch->player.m_class = CLASS_ILLUSIONIST;
        break;
      case CONJ:
        GET_SIZE(ch) = SIZE_MEDIUM;
        GET_RACE(ch) = RACE_PHANTOM;
        ch->player.m_class = CLASS_CONJURER;
        break;
      default:
        send_to_char("Something wierd just happened, please contact a god ASAP.", ch);
        return;

    }

    if(!GET_CLASS(ch, CLASS_NECROMANCER))
    {
      act
      ("&+L$N &+Lturns to you, $S stare piercing your very &+Wsoul&+L.\n"
       "&+LSoftly $E whispers '&+YI see your lust for the darkness is not quenched.  For your dedication\n"
       "&+YI shall grant you the unspeakable &+rpower &+Lof the &+Rdread&+Y.&+L'\n\n"
       "&+L$N &+Lapproaches and stands before you, softly chanting\n"
       "&+Lwhile waving a small, curved &+Csacrificial &+Wkris &+Lbefore your chest. The chanting of\n"
       "&+Lyour dark master rises to a crescendo as $E raises the &+wdagger &+Lhigh and brings it down\n" 
       "&+Lfiercely upon your breast.\n\n"
       "&+LIts sharp blade glides easily through your &+Rflesh&+L, freeing your &+rblood &+Lfrom the shell\n"
       "&+Lof your body. Your vision fades as your eyes fill with stygian clouds of smoke and vapor.\n"       
       "&+L&+RS&+re&+Ra&+rr&+Ri&+rn&+Rg pain &+Lrages through your body, awakening nerves and alerting the senses.\n\n"
       "&+L$N &+Lspeaks once more, '&+YLet &+Ldarkness &+Ybe your &+Wsight&+Y.\n"
       "&+YRely no longer on &+rmortal &+wsenses&+Y.&+L'\n\n"
       "&+LVisions of d&+we&+Wa&+wt&+Lh and de&+rst&+Rru&+rct&+Lion &+Lfill your mind and temper your resolve.\n"
       "&+LA wave of nausea overcomes your body, followed by a &+Rtingling &+Lin your head.\n"
       "&+LYou fight off the ailments and realize that your new &+wvision &+Lhas returned,\n"
       "&+Lalong with a sense of &+mpower &+Land &+rduty&+L.\n", FALSE, ch, 0, teacher, TO_CHAR);
      act
      ("&+L$N &+Lturns to $n &+Land stares, piercing $s very &+Wsoul&+L.\n"
       "&+LSoftly $E whispers '&+YI see your lust for the darkness is not quenched.  For your dedication\n"
       "&+YI shall grant you the unspeakable &+rpower &+Lof the &+Rdread&+Y.&+L'\n\n"
       "&+L$N &+Lapproaches $n &+Land stands before $m&+L, softly\n"
       "&+Lchanting while waving a small, curved &+Csacrificial &+Wkris &+Lbefore $s chest. The\n"
       "&+Lchanting of $s dark master rises to a crescendo as $E raises the &+wdagger &+Lhigh and\n"
       "&+Lbrings it down fiercely upon $s breast.\n\n"
       "&+LIts sharp blade glides easily through $s &+Rflesh&+L, freeing $s &+rblood &+Lfrom the shell\n"
       "&+Lof $s body. $n's eyes cloud over with a stygian influx of smoke and vapor.\n"       
       "&+L$n &+Lwrithes in enormous pain as the transformation begins to take hold of $s body.\n\n"
       "$N &+Lspeaks once more, '&+YLet &+Ldarkness &+Ybe your &+Wsight&+Y... rely\n"
       "&+Yno longer on &+rmortal &+wsenses&+Y.&+L'\n\n"
       "&+LWith the rigors and torment of the &+rtransformation &+Lcomplete, $n &+Lraises $s head\n"
       "&+Land grins with newfound &+Rmalevolence&+L.", FALSE, ch, 0, teacher, TO_ROOM);
    }
    else
    {
      act("&+L$N &+Lcackles &+rmadly &+Land stares at you intently.  A faint glow of &+mmalevolence\n"
          "&+Lbegins to emanate from within $S bloodshot eyes.  Words of command begin to spill\n"
          "&+Lfrom $S mouth in a &+wbarely audible &+Lwhisper.  The aura surrounding $N\n"
          "&+Lpulsates as the words are spoken, growing in strength as the chant becomes louder.\n"
          "&+L$N &+Lraises $S arms high, holding a &+rbejeweled &+Rskull &+Lhigh in the musty air.\n"
          "&+LThe chanting continues unabated from a &+wsource unknown &+Las your master speaks to you,\n"
          "&+L'&+YDivide your soul from this mortality that binds it so.  Take in a breath of not\n"
          "&+Yair, but of the &+Rpower &+Lof the &+wgrave&+Y.  Focus upon the skull and receive the &+rdark gift\n"
          "&+Ywillingly.  Become what you desire...&+L'\n\n"
          "&+LWith the final words, your eyes ascend to the skull and your entire being is caught\n"
          "&+Lwithin the enchantment.  &+mWisps &+Lof &+mpu&+Mrp&+mle f&+Mlam&+me &+Ldance within the sockets and demand\n"
          "&+Lobeisance and attention.  &+LYou feel a drawing of the very &+Cbreath &+Lfrom your lungs, followed\n"
          "&+Lby &+bintense chill&+L.  Your heartbeat slows, murmurs and then falls eerily &+Wsilent &+Las the\n"
          "&+Ltransformation becomes complete.  Your body quivers as it adjusts to the vast changes\n"
          "&+Lwrought upon it.\n", FALSE, ch, 0, teacher, TO_CHAR);

      act("&+L$N &+Lcackles &+rmadly &+Land stares at $n &+Lintently.  A faint glow of &+mmalevolence\n"
          "&+Lbegins to emanate from within $S bloodshot eyes.  Words of command begin to spill\n"
          "&+Lfrom $S mouth in a &+wbarely audible &+Lwhisper.  The aura surrounding $N\n"
          "&+Lpulsates as the words are spoken, growing in strength as the chant becomes louder.\n"
          "&+L$N &+Lraises $S arms high, holding a &+rbejeweled &+Rskull &+Lhigh in the musty air.\n"
          "&+LThe chanting continues unabated from a &+wsource unknown &+Las $n&+L's master speaks to $m,\n"
          "&+L'&+YDivide your soul from this mortality that binds it so.  Take in a breath of not\n"
          "&+Yair, but of the &+Rpower &+Lof the &+wgrave&+Y.  Focus upon the skull and receive the &+rdark gift\n"
          "&+Ywillingly.  Become what you desire...&+L'\n\n"
          "&+LWith the final words, $n&+L's eyes ascend to the skull and $s entire being is caught\n"
          "&+Lwithin the enchantment.  &+mWisps &+Lof &+mpu&+Mrp&+mle f&+Mlam&+me &+Ldance within the sockets and demand\n"
          "&+L$s &+Lobeisance.  $n begins to gasp for breath and looks visibly &+wpale &+Las life\n"
          "&+Lis drawn from $s &+Lbody.  The vast power takes only a few moments before the\n"
          "&+Ltransformation becomes complete.  $n&+L's body quivers as it adjusts to the changes\n"
          "&+Lwrought upon it.", FALSE, ch, 0, teacher, TO_ROOM);
    }
    send_to_char("You feel a chill and realize that you are naked.\r\n", ch);
    generate_desc(ch);
    // GET_AGE does not return a changeable variable.
    ch->player.time.birth = time(NULL) - 1 * SECS_PER_MUD_YEAR;
    GET_VITALITY(ch) =  GET_MAX_VITALITY(ch) = 120;
    forget_spells(ch, -1);
    ch->player.spec = 0;
    ch->player.secondary_class = 0;
    ch->only.pc->epics = MAX(0, ch->only.pc->epics - cost);

    if(RACE_EVIL(ch))
    {
      GET_HOME(ch) = GET_BIRTHPLACE(ch) = GET_ORIG_BIRTHPLACE(ch) = 90363;
    }
    else
    {
      GET_HOME(ch) = GET_BIRTHPLACE(ch) = GET_ORIG_BIRTHPLACE(ch) = 55126;
    }
  }
}

