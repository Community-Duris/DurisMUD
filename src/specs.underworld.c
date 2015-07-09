
/*
   ***************************************************************************
   *  File: specs.underworld.c                                 Part of Duris *
   *  Usage: Special Procs for UnderWorld area                                 *
   *  Copyright  1993 - Rod Reed (Barlows)   john@cyberstore.ca                *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "sound.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "justice.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "range.h"
#include "damage.h"
#include "guildhall.h"
#include "buildings.h"
#include "nexus_stones.h"
#include "map.h"
#include "ctf.h"
#include "handler.h"

/*
   extern variables
 */

extern P_room world;
extern struct zone_data *zone_table;
extern const int top_of_world;
extern P_event current_event;
extern P_index obj_index;

int      range_scan_track(P_char ch, int distance, int type_scan);
extern bool has_skin_spell(P_char);
extern P_index mob_index;

/*
   item procs
 */

/*static void hummer(P_obj obj);

void
hummer(P_obj obj)
{
  if (!obj || number(0,9))
    return;
  if ((OBJ_WORN(obj) || OBJ_CARRIED(obj)) && !IS_AFFECTED(obj->loc.wearing, AFF_HIDE)) {
    act("&+LA faint hum can be heard from $p&+L carried by $n.",
        FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
    act("&+LA faint hum can be heard from $p&+L you are carrying.",
        FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
  } else if (OBJ_ROOM(obj)) {
    act("&+LA strange humming sound comes from $p&+L.",
        0, world[obj->loc.room].people, obj, 0, TO_ROOM);
    act("&+LA strange humming sound comes from $p&+L.",
        0, world[obj->loc.room].people, obj, 0, TO_ROOM);

  }
  return;
}
*/

void hammer_berserk_check(P_char ch)
{
  if(affected_by_spell(ch, SKILL_BERSERK) ||
     GET_CHAR_SKILL(ch, SKILL_BERSERK) < 1)
      return;
  int duration = 5 * (MAX(25, (GET_CHAR_SKILL(ch, SKILL_BERSERK) + (2 * GET_LEVEL(ch)))));

  berserk(ch, duration);
  
  return;
}

int hammer(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   t_vict;
  P_char   vict;
  P_char   temp;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  if( !dam || !IS_ALIVE(ch) || !IS_ALIVE(vict) || cmd != CMD_MELEE_HIT )
  {
    return FALSE;
  }

/*
  if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
    return (FALSE);
*/

  if( OBJ_WORN(obj) && obj->loc.wearing == ch )
  {
    // 1/20 chance.
    if( !number(0, 19) )
    {
      act("Your $q glows brightly as &=LBlightning bolts streak out of it.", FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
      act("$n's $q glows brightly as &=LBlightning bolts streak out of it!", FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
      spell_lightning_bolt(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
/*
      for (t_vict = world[ch->in_room].people; t_vict;
           t_vict = temp) {
        temp = t_vict->next_in_room;
        if (t_vict->group && (t_vict->group == ch->group)) continue;

        if ((t_vict != ch) && CAN_SEE(ch, t_vict) && !number(0, 3) && !IS_TRUSTED(t_vict) &&
            ((ch->specials.fighting == t_vict) || (t_vict->specials.fighting == ch))) {
          cast_lightning_bolt(51, ch, 0, SPELL_TYPE_SPELL, t_vict, 0);
        }
      }
*/
    }
    else
    {
      if( !ch->specials.fighting )
      {
        set_fighting(ch, vict);
      }
    }
  }
  if( ch->specials.fighting )
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

int torment(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      save;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  if( CheckMultiProcTiming(ch) )
  {
    // 1/20 chance.
    if(!number(0, 19))
    {
      act("Your $q glows dark and bites into $N's neck.", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q glows dark as it bites into your neck.", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q glows dark and bites into $N's neck.", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);

      save = vict->specials.apply_saving_throw[SAVING_SPELL];
      vict->specials.apply_saving_throw[SAVING_SPELL] += 5;
      spell_poison(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
      spell_blindness(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
      vict->specials.apply_saving_throw[SAVING_SPELL] = save;
    }
  }
  return FALSE;
}

int dragonkind(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   tchar1 = NULL, tchar2 = NULL;
  int      dam = cmd / 1000, curr_time;
  P_char   tch, tch_next;
  P_char   vict;
  P_char   temp;
  bool     chance = 0;
  int      rand;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) || (!OBJ_WORN_POS(obj, WIELD) && !OBJ_WORN_POS(obj, WIELD2)) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "protect me") )
    {
      curr_time = time(NULL);
      // 1 min timer.
      if( obj->timer[0] + 60 <= curr_time )
      {
        act("You say 'protect me'", FALSE, ch, 0, 0, TO_CHAR);
        act("&+WYour $q &+Wcalls upon the power of the dragonkind.", FALSE, ch, obj, obj, TO_CHAR);
        act("$n says 'protect me'", TRUE, ch, obj, NULL, TO_ROOM);
        act("&+WThe $q &+Whums as the aura of &+gdragonkind&+W embraces $n.", TRUE, ch, obj, NULL, TO_ROOM);

        spell_biofeedback(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_globe(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_fireshield(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_vitality(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
    return FALSE;
  }

  vict = (P_char) arg;
  if( !dam || !IS_ALIVE(vict) || !ch->specials.fighting )
  {
    return FALSE;
  }

  rand = number(1, 4);
  // 1/25 chance for breath.
  if( !number(0, 24) )
  {
    switch (rand)
    {
    // FIRE BREATH
    case 1:
      act("&+rYour veins pulse with dragon blood...\n&+LYour &+Rfire breath&+L fills the area!", 0, ch, obj, 0, TO_CHAR);
      act("$n &+Lpulses with the power of &+rdragons&+L...\n&+L$n &+Rbreathes fire&+L, filling the surrounding area!", 1, ch, obj, 0, TO_ROOM);
      for( vict = world[ch->in_room].people; vict; vict = temp )
      {
        temp = vict->next_in_room;

        if( IS_ALIVE(vict) )
        {
          if( (vict->group && (vict->group == ch->group)) || vict == ch )
          {
            continue;
          }
          spell_fire_breath(50, ch, NULL, 0, vict, 0);
        }
      }
      return TRUE;
      break;
    // FROST BREATH
    case 2:
      act("&+rYour veins pulse with dragon blood...\n&+LYour &+Bfrost breath&+L fills the area!", 0, ch, obj, 0, TO_CHAR);
      act("$n &+Lpulses with the power of &+rdragons&+L...\n&+L$n &+Bbreathes frost&+L, filling the surrounding area!", 1, ch, obj, 0, TO_ROOM);
      for( vict = world[ch->in_room].people; vict; vict = temp )
      {
        temp = vict->next_in_room;

        if( IS_ALIVE(vict) )
        {
          if( vict->group && (vict->group == ch->group) || vict == ch )
          {
            continue;
          }
          spell_frost_breath(50, ch, NULL, 0, vict, 0);
        }
      }
      return TRUE;
      break;
    // GAS BREATH
    case 3:
      act("&+rYour veins pulse with dragon blood...\n&+LYour &+ggaseous breath&+L fills the area!", 0, ch, obj, 0, TO_CHAR);
      act("$n &+Lpulses with the power of &+rdragons&+L...\n&+L$n &+gbreathes gas&+L, filling the surrounding area!", 1, ch, obj, 0, TO_ROOM);
      for (vict = world[ch->in_room].people; vict; vict = temp)
      {
        temp = vict->next_in_room;

        if( IS_ALIVE(vict) )
        {
          if( vict->group && (vict->group == ch->group) || vict == ch )
          {
            continue;
          }
          spell_gas_breath(50, ch, NULL, 0, vict, 0);
        }
      }
      return TRUE;
      break;
    //ACID BREATH
    case 4:
      act("&+rYour veins pulse with dragon blood...\n&+LYour &+Gacid breath&+L fills the area!", 0, ch, obj, 0, TO_CHAR);
      act("$n &+Lpulses with the power of &+rdragons&+L...\n&+L$n &+Gbreathes acid&+L, filling the surrounding area!", 1, ch, obj, 0, TO_ROOM);
      for (vict = world[ch->in_room].people; vict; vict = temp)
      {
        temp = vict->next_in_room;

        if( IS_ALIVE(vict) )
        {
          if( vict->group && (vict->group == ch->group) || vict == ch )
          {
            continue;
          }
          spell_acid_breath(50, ch, NULL, 0, vict, 0);
        }
      }
      return TRUE;
      break;
    }
    // Should never get here, but..
    return TRUE;
  }

  if( OBJ_WORN_BY(obj, ch) )
  {
    // 1/28 chance for tailswep proc.
    if( !number(0, 27) )
    {
      P_char   tch, tch_next;

      act("$n's $q &+Lsummons forth a visage of a &+gDRAGON\n&+LThe &+gdragon&+L visage of the &+Waxe&+L lashes out with its mighty tail!", TRUE, ch, obj, NULL, TO_ROOM);
      act("Your $q &+Lsummons forth a visage of a &+gDRAGON\n&+LThe &+gdragon&+L visage of the &+Waxe&+L lashes out with its mighty tail!", TRUE, ch, obj, vict, TO_CHAR);

      for (tch = world[ch->in_room].people; tch; tch = tch_next)
      {
        tch_next = tch->next_in_room;

        if( IS_NPC(tch) && (!tch->following || IS_NPC(tch->following))
          && (ch->specials.fighting != tch) && (tch->specials.fighting != ch) )
        {
          continue;
        }
        if (IS_TRUSTED(tch))
        {
          continue;
        }
        if( ((IS_FIGHTING(tch) && (tch->specials.fighting == ch)) ||
             !IS_FIGHTING(ch)) && !IS_DRAGON(tch) )
        {
          if( !StatSave(tch, APPLY_AGI, -2) )
          {
            /* fall down, go boom */
            SET_POS(tch, POS_SITTING + GET_STAT(tch));
            CharWait(tch, PULSE_VIOLENCE * 2);
            act("The powerful sweep sends you crashing to the ground!", FALSE, tch, 0, 0, TO_CHAR);
            act("$n crashes to the ground!", FALSE, tch, 0, 0, TO_ROOM);
            /* don't want to kill them with it, but can mess them up BAD! */
            damage(ch, tch, MIN(dice(2, (GET_LEVEL(ch) / 5)) + 5, GET_HIT(tch) + 8), TYPE_UNDEFINED);
          }
          else
          {
            send_to_char("You nimbly dodge the sweep!\n", tch);
            act("$N dodges your sweep.", 0, ch, 0, tch, TO_CHAR);
          }
        }
        if( !char_in_list(ch) )
        {
          return TRUE;
        }
      }
    }

    // 1/27 chance for roar proc.
    if( !number(0, 27) )
    {
      act("&+rYour veins pulse with dragon blood...\n&+LYour &+RROAR&+L fills your victims with sheer terror!", 0, ch, obj, 0, TO_CHAR);
      act("$n &+Lpulses with the power of &+rdragons&+L...\n&+L$n &+RROARS&+L, filling your heart with sheer terror!", 1, ch, obj, 0, TO_ROOM);

      for( tchar1 = world[ch->in_room].people; tchar1; tchar1 = tchar2 )
      {
        tchar2 = tchar1->next_in_room;

        if( ch == tchar1 || !IS_ALIVE(tchar1) )
        {
          continue;
        }

        /* transparent tower stone blocks roar */
        if( !IS_DRAGON(tchar1) && !IS_TRUSTED(tchar1)
          && (tchar1->specials.z_cord == ch->specials.z_cord) )
        {
          if( GET_LEVEL(tchar1) < (GET_LEVEL(ch) / 2) )
          {
            do_flee(tchar1, 0, 2);
            continue;
          }
          if( GET_RACE(tchar1) == RACE_CENTAUR && !fear_check(tchar1) )
          {
            do_flee(tchar1, 0, 2);
            continue;
          }
          if( tchar1->group && (tchar1->group == ch->group) || fear_check(tchar1) )
          {
            continue;
          }
          if( GET_LEVEL(tchar1) >= GET_LEVEL(ch) )
          {
            if( !NewSaves(tchar1, SAVING_PARA, -2) )
            {
              do_flee(tchar1, 0, 1);
            }
          }
          else if( !NewSaves(tchar1, SAVING_PARA, 1) )
          {
            do_flee(tchar1, 0, 1);
          }
        }
        if( ch->in_room != tchar1->in_room )
        {
          if( IS_FIGHTING(tchar1) )
          {
            stop_fighting(tchar1);
          }
        }
      }
      return FALSE;
    }
  }
  return FALSE;
}

int lightning(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      current_time = time(NULL);
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  // 500 sec timer.
  if( (cmd == CMD_RUB) && OBJ_WORN(obj) && (obj->timer[0] + 500 <= current_time) )
  {
    obj->timer[0] = current_time;
    act("You rub your $q and begin to whirl around.", FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
    act("$n rubs $s $q and begins to whirl around.", FALSE, obj->loc.wearing, obj, 0, TO_ROOM);

    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    {
      if (ch->group && vict->group && (ch->group == vict->group))
      {
        continue;
      }
      if (IS_TRUSTED(ch) || (ch == vict))
      {
        continue;
      }
      if( (number(0, (SIZE_GARGANTUAN + 2)) - 2) > GET_ALT_SIZE(vict) )
      {
        act("Your &+CWHIRLWIND picks up $N and tosses $M against the wall!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
        act("$n's &+CWHIRLWIND picks up $N and tosses $M against the wall!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
        act("$n's &+CWHIRLWIND picks you up and tosses you against the wall!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
        SET_POS(vict, POS_PRONE + GET_STAT(vict));
        stop_fighting(vict);
        if( IS_DESTROYING(vict) )
        {
          stop_destroying(vict);
        }
        CharWait(vict, (int) (PULSE_VIOLENCE * 0.7));
      }
    }
  }
  return FALSE;

  vict = (P_char) arg;
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) || obj->loc.wearing != ch || !IS_ALIVE(vict) )
  {
    return FALSE;
  }
  // 1/16 chance.
  if (!number(0, 15))
  {
    act("Your $q &+bsummons the power of air and lightning at $N.", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
    act("$n's $q &+bsummons the power of air and lightning at.", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
    act("$n's $q &+bsummons the power of air and lightning at $N.", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
    spell_cyclone(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    if( IS_ALIVE(vict) && IS_ALIVE(ch) )
    {
      spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( IS_ALIVE(vict) && IS_ALIVE(ch) )
    {
      spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( IS_ALIVE(vict) && IS_ALIVE(ch) )
    {
      spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    return TRUE;
  }
  return FALSE;
}

int dispator(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }
  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  vict = (P_char) arg;
  if( !dam || !IS_ALIVE(ch) || !IS_ALIVE(vict) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }
/*
   if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
    return (FALSE);
*/
  // 1/16 chance.
  if (!number(0, 15))
  {
    act("Your $q &+rsummons the power of hellfire at $N.", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
    act("$n's $q &+rsummons the power of hellfire at you.", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
    act("$n's $q &+rsummons the power of hellfire at $N.", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
    spell_flamestrike(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    if( char_in_list(vict) && char_in_list(ch) )
    {
      spell_fireball(45, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( char_in_list(vict) && char_in_list(ch) )
    {
      spell_fireball(45, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( char_in_list(vict) && char_in_list(ch) )
    {
      spell_fireball(45, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( char_in_list(vict) && char_in_list(ch) )
    {
      spell_flamestrike(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( !char_in_list(vict) || !char_in_list(ch) )
    {
      return TRUE;
    }
  }

  return FALSE;
}

int orb_of_the_sea(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  int      dam = cmd / 1000, loc;
  P_char   victim;
  int      new_room;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( OBJ_ROOM(obj) )
    {
      hummer(obj);
    }
    else if( OBJ_WORN(obj) && (ch = obj->loc.wearing) )
    {
      // Slippery when wet! 5% chance .. :)
      if( !IS_NPC(ch) && !can_prime_class_use_item(ch, obj) && !number(0,19) )
      {
        for( loc = 0; loc < MAX_WEAR; loc++ )
        {
          if( obj == ch->equipment[loc] )
          {
            act("&+b$p&+b slips out of your hands!&n", FALSE, ch, obj, NULL, TO_CHAR);
            act("&+b$p&+b slips out of $n&+b's hands.&n", TRUE, ch, obj, NULL, TO_ROOM);
            unequip_char( ch, loc );
            obj_to_char( obj, ch );
            break;
          }
        }
      }
    }
    return TRUE;
  }

  // Procs on hits or getting hit.. since it's a mage arti.
  if( (cmd != CMD_GOTHIT && cmd != CMD_MELEE_HIT) || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

/*
   if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
    return (FALSE);
*/
  // If we can't find the victim
  if( cmd == CMD_GOTHIT )
  {
    if( !((data = (struct proc_data *) arg) && (victim = data->victim)) )
    {
      debug("orb_of_the_sea: CMD_GOTHIT: no attacker.");
      return FALSE;
    }
  }
  else
  {
    if( !(victim = (P_char) arg) )
    {
      debug("orb_of_the_sea: CMD_MELEE_HIT: no victim.");
      return FALSE;
    }
  }

  if( OBJ_WORN_BY(obj, ch) && IS_ALIVE(victim) && !IS_SET(world[ch->in_room].room_flags, NO_TELEPORT)
    && !IS_GH_GOLEM(victim) && GET_RACE(victim) != RACE_CONSTRUCT && !(strstr(victim->player.name, "_no_move_")) )
  {
    // 1/30 chance.
    if( !number(0, 29) )
    {
      act("Your $q lets out a banshee wail at $N!", FALSE, obj->loc.wearing, obj, victim, TO_CHAR);
      act("$n's $q lets out a banshee wail at you!", FALSE, obj->loc.wearing, obj, victim, TO_VICT);
      act("$n's $q lets out a banshee wail at $N!", FALSE, obj->loc.wearing, obj, victim, TO_NOTVICT);

      // We don't poof the stone carriers to the WP 'cause we don't want them to drop to ground
      //   and not be retrievable when the carrier dies.
      if( IS_NPC(victim) && has_touch_stone(victim) )
      {
        act("&+r$N&+r grins evilly.&n", FALSE, obj->loc.wearing, obj, victim, TO_CHAR);
        act("&+rYou grin evilly.&n", FALSE, obj->loc.wearing, obj, victim, TO_VICT);
        act("&+r$N&+r grins evilly.&n", FALSE, obj->loc.wearing, obj, victim, TO_NOTVICT);
        return FALSE;
      }

      // 1/30 * 1/3 == 1/90 chance.
      if( !number(0, 2) )
      {
        act("&+WA ghostly banshee appears in a puff of mist, and points at $N!", FALSE, obj->loc.wearing, obj, victim, TO_CHAR);
        act("&+WA ghostly banshee appears in a puff of mist, and points at $N!", FALSE, obj->loc.wearing, obj, victim, TO_NOTVICT);
        act("&+WA ghostly banshee appears in a puff of mist, and points at YOU!", FALSE, obj->loc.wearing, obj, victim, TO_VICT);
        char_from_room(victim);
        new_room = new_room = real_room(number(WATERP_VNUM_BEGIN, WATERP_VNUM_END));
        char_to_room(victim, new_room, -1);
        return TRUE;
      }
    }
  }

  return FALSE;
}

int platemail_of_defense(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000, curr_time;
  P_char   vict;

  // Check for periodic event calls
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }
  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WEAR_BODY) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if (isname(arg, "stone"))
    {
      curr_time = time(NULL);

      if( curr_time >= obj->timer[0] + 60 )
      {
        act("You say 'stone'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'stone'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_stone_skin(60, ch, 0, 0, ch, NULL);
        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }
  return FALSE;
}

#ifdef THARKUN_ARTIS
int mace(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  struct damage_messages messages = {
    "Your $q &+Wslams &+L$N with a &+yhuge boulder&+L!",
    "$n's $q &+Wslams &+Lyou with a &+yhuge boulder&+L!",
    "$n's $q &+Wslams &+L$N with a &+yhuge boulder&+L!",
    "A &+yhuge boulder&+L from your $q &+Wsmashes $N to a &+rbloody pulp!",
    "A &+yhuge boulder&+L from $n's $q shoots right towards your face!",
    "A &+yhuge boulder&+L from $n's $q &+Wsmashes $N to a &+rbloody pulp!",
    0, obj
  };

  if (cmd != CMD_MELEE_HIT)
    return FALSE;

  vict = (P_char) arg;

  if (number(0, 24))
    return FALSE;

  act("Your $q &+ysummons the power of the earth and rock at $N.", FALSE,
      obj->loc.wearing, obj, vict, TO_CHAR);
  act("$n's $q &+ysummons the power of the earth and rock at you.", FALSE,
      obj->loc.wearing, obj, vict, TO_VICT);
  act("$n's $q &+ysummons the power of the earth and rock at $N.", FALSE,
      obj->loc.wearing, obj, vict, TO_NOTVICT);
  spell_earthquake(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
  if (is_char_in_room(vict, ch->in_room))
  {
    spell_damage(ch, vict, 406, SPLDAM_GENERIC,
        SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
  }
  return (FALSE);
}

int flamberge(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      room;
  P_char   temp_ch;
  int      curr_time;
  char	   *argument;
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }
// Modified and added keywords for invoke due to removed perm sanct. - gellz
// Keyword will give sanct for evils/hellfire for good/and random effect chance
  if((cmd == CMD_SAY) && (!(IS_FIGHTING(ch))))
  {
    argument = arg;
    while (*argument == ' ')
      argument++;
    if( !strcmp(argument, "timeit") && IS_TRUSTED(ch) )
    {
	    send_to_char_f(ch, "timer0 is: %d = %s\n", asctime(localtime(&obj->timer[0])));
	    return FALSE;
    }
    if (!strcmp(argument, "efreeti"))
    {
      curr_time = time(NULL);//get time
      if (obj->timer[0] + 300 <= curr_time) //IF timer has passed..
      {
        if (GET_RACEWAR(ch) == RACEWAR_GOOD)
        {
          send_to_room("&+wSm&+Lok&+we r&+Lis&+Wes&n from &+rthe holy flamberge&+w and slowly forms into the &+wim&+Wa&+Lge&n of an &+ref&+Rre&+ret&+Ri&n &+LUnq&+wu&+Lenchable&n who brings forth the &+rf&+Rir&+Re&+rs of &+Rhe&+rl&+Rl&n!.&n\n", ch->in_room);
          spell_hellfire(60, ch, 0, 0, ch, 0);
        }
        if (GET_RACEWAR(ch) == RACEWAR_EVIL)
        {
          send_to_room("A &+wh&+Lea&+wt h&+La&+Wze&n from &+rthe holy flamberge&+w materializes into the &+wim&+Wa&+Lge&n of an &+LOrd&+we&+Lr of &+RFi&+re&+Rry He&+ra&+Rrt&n who calls on the &+wforces of &+wl&+Wi&+Yg&+Wh&+wt for &+wpr&+Wot&+Yec&+Wti&+won&n!.&n\n", ch->in_room);
          spell_sanctuary(60, ch, 0, 0, ch, 0);
        }
        obj->timer[0] = curr_time;
      }
      else //END if TImer has passed.
      {
        send_to_room("&+MAn &+ref&+Rre&+ret&+Ri &+Macolyte&+w snaps into your vision momentarilty then quickly &+Ldisappears&+w again, leaving only a faint trace of &+rheat&n.&n\n", ch->in_room);
        spell_fireshield(60, ch, NULL, 0, ch, obj);
        obj->timer[0] = curr_time;
      }
      return TRUE;
    } //End efreeti Keywordi
    return FALSE; //GO BACK because didnt say keyword
  } //End cmd say and argument.
//End Modifications for Keyword

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  vict = (P_char)arg;
  // 1/30 chance.
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || number(0, 29) || !(room = ch->in_room) || !IS_ALIVE(vict ) )
  {
    return FALSE;
  }

  act("Your $q &+rsummons the power of fire and flames at $N.", FALSE, ch, obj, vict, TO_CHAR);
  act("$n's $q &+rsummons the power of fire and flames at you.", FALSE, ch, obj, vict, TO_VICT);
  act("$n's $q &+rsummons the power of fire and flames at $N.", FALSE, ch, obj, vict, TO_NOTVICT);

  spell_incendiary_cloud(60, ch, 0, 0, vict, 0);
  if( is_char_in_room(vict, room) && is_char_in_room(ch, room) )
  {
    spell_burning_hands(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
  }
  if( is_char_in_room(vict, room) && is_char_in_room(ch, room) )
  {
    spell_fireball(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
  }
  return TRUE;
}

int doombringer(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000, curr_time, i, room;
  P_char   vict;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( (OBJ_ROOM(obj) || OBJ_WORN(obj)) && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !(room = ch->in_room) )
  {
    return FALSE;
  }
/*
  if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
    return (FALSE);
*/
  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "stone"))
    {
      curr_time = time(NULL);
      // 60 sec timer.
      if (obj->timer[0] + 60 <= curr_time)
      {
        act("You say 'stone'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'stone'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_stone_skin(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }

  if( IS_FIGHTING(ch) )
  {
    vict = ch->specials.fighting;
  }
  else
  {
    vict = (P_char) arg;
  }
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(vict) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  // 1/25 chance to proc.  1/3 chance to override !multiple-procs-per-round code.
  if( !number(0, 24) && (CheckMultiProcTiming(ch) || !number(0, 2)) )
  {
    act("&+LYour $q blurs as it calls upon the elements of &+Blightning, &+rfire, &+Cand ice to strike $N.", FALSE, ch, obj, vict, TO_CHAR);
    act("&+L$n's $q blurs as it strikes you with &+Blightning, &+rfire, &+Cand ICE!", FALSE, ch, obj, vict, TO_VICT);
    act("&+L$n's $q blurs as it calls upon the elements of &+Blightning, &+rfire, &+Cand ice to strike $N.", FALSE, ch, obj, vict, TO_NOTVICT);

    act("&+LDoombringer continues to grow with a putrid power, and unleashes &=LBLIGHTNING&+L...", FALSE, ch, obj, vict, TO_CHAR);
    act("&+LFoul black &=LBLIGHTNING&+L surges forth from $q&+L...", FALSE, ch, obj, vict, TO_ROOM);

    if( spell_damage(ch, vict, number(100, 200), SPLDAM_LIGHTNING, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, 0) != DAM_NONEDEAD )
    {
      return TRUE;
    }

    act("&+LDoombringer continues to grow with a putrid power, and unleashes &=LRFIRE&+L...", FALSE, ch, obj, vict, TO_CHAR);
    act("&+LThe blade of $q &+Lcontinues to grow with a putrid power, and unleashes &=LRFIRE&+L...", FALSE, ch, obj, vict, TO_ROOM);
    if(spell_damage(ch, vict, number(100, 200), SPLDAM_FIRE, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, 0) != DAM_NONEDEAD)
    {
      return TRUE;
    }

    act("&+LDoombringer continues to grow with a putrid power, and unleashes &=LCICE&+L...", FALSE, ch, obj, vict, TO_CHAR);
    act("&+LSuddenly, the air surrounding $q&+L grows eerily cold, and &=LCICE&+L pours forth!", FALSE, ch, obj, vict, TO_ROOM);

    if( spell_damage(ch, vict, number(100, 200), SPLDAM_COLD, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, 0) != DAM_NONEDEAD )
    {
      return TRUE;
    }

    act("&+LYour $q blurs as it strikes $N.", FALSE, ch, obj, vict, TO_CHAR);
    act("&+L$n's $q blurs as it strikes you.", FALSE, ch, obj, vict, TO_VICT);
    act("&+L$n's $q blurs as it strikes $N.", FALSE, ch, obj, vict, TO_NOTVICT);

    for( i = 0; i < 3 && IS_ALIVE(ch) && IS_ALIVE(vict); i++ )
    {
      hit(ch, vict, obj);
    }
    if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
    {
      return TRUE;
    }
  }
  return FALSE;
}

int unholy_avenger_bloodlust(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int  dam;
  struct damage_messages messages = {
    "&+rBlood red energy travels into your hand and up your arm, seeping into your veins!",
    "You feel yourself becoming weak as the &+rblood is sucked out of you!",
    "&+rBlood red energy travels into $n's hand and up $s arm, apparently strengthening $m.",
    "", "", "", 0, obj};

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  vict = (P_char) arg;

  // 1/25 chance.
  if( !number(0, 24) && CheckMultiProcTiming(ch) && IS_ALIVE(vict) && !IS_UNDEADRACE(vict) )
  {
    dam = BOUNDED(0, (GET_HIT(vict) + 9), 100);
    act("Your $q turns &+rblood red as it slashes into $N!", FALSE, ch, obj, vict, TO_CHAR);
    act("$n's $q turns &+rblood red as it slashes into $N!", FALSE, ch, obj, vict, TO_NOTVICT);
    act("$n's $q turns &+rblood red as it slashes into you!", FALSE, ch, obj, vict, TO_VICT);
    spell_damage(ch, vict, 300, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | RAWDAM_NOKILL, &messages);

    vamp(ch, dam / 2, (int) (GET_MAX_HIT(ch) * 1.4));

    return TRUE;
  }

  return FALSE;
}

#else

int mace(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time;
  P_char   vict;
  struct damage_messages messages = {
    "Your $q &+Wslams &+L$N with a &+yhuge boulder&+L!",
    "$n's $q &+Wslams &+Lyou with a &+yhuge boulder&+L!",
    "$n's $q &+Wslams &+L$N with a &+yhuge boulder&+L!",
    "A &+yhuge boulder&+L from your $q &+Wsmashes $N to a &+rbloody pulp!",
    "A &+yhuge boulder&+L from $n's $q shoots right towards your face!",
    "A &+yhuge boulder&+L from $n's $q &+Wsmashes $N to a &+rbloody pulp!",
    0, obj
  };

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!ch && cmd == CMD_PERIODIC)
  {
    hummer(obj);
    return TRUE;
  }

  if (!ch)
    return (FALSE);

  if (!OBJ_WORN_POS(obj, WIELD))
    return (FALSE);

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "stone"))
    {
      curr_time = time(NULL);

      if (curr_time >= obj->timer[0] + 60)
      {
        act("You say 'stone'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'stone'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_group_stone_skin(45, ch, 0, 0, ch, NULL);
        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
    else if (isname(arg, "invisible"))
    {
      curr_time = time(NULL);

      if (curr_time >= obj->timer[1] + 60)
      {
        act("You say 'invisible'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'invisible'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_improved_invisibility(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[1] = curr_time;

        return TRUE;
      }
    }
  }

  if (cmd != CMD_MELEE_HIT)
    return FALSE;

  vict = (P_char) arg;

  if (OBJ_WORN_BY(obj, ch) && vict)
  {
    if (!number(0, 24))
    {
      act("Your $q &+ysummons the power of the earth and rock at $N.", FALSE,
          obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q &+ysummons the power of the earth and rock at you.", FALSE,
          obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q &+ysummons the power of the earth and rock at $N.", FALSE,
          obj->loc.wearing, obj, vict, TO_NOTVICT);
      if (char_in_list(vict))
        spell_earthquake(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      if (is_char_in_room(vict, ch->in_room))
      {
        if(GET_RACE(vict) != RACE_E_ELEMENTAL) 
          spell_damage(ch, vict, number(500, 600), SPLDAM_GENERIC,
                       SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
        else
        {
          act("&+yThe earth and rock heals $N!",
            FALSE, 0, obj, vict, TO_CHAR);
          act("$n &+yheals you with the power of the earth and rock.",
            FALSE, 0, obj, vict, TO_VICT);
          act("$n &+yheals $N &+ywith the power of earth and rock.",
            FALSE, 0, obj, vict, TO_NOTVICT);
          vamp(vict, 100, (int) (GET_MAX_HIT(ch) * 1.3));
        }
      }
    }
  }
  return (FALSE);
}

int unholy_avenger_bloodlust(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) || obj->loc.wearing != ch || IS_ALIVE(vict) )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  dam = BOUNDED(0, (GET_HIT(vict) + 9), 100);

  // 1/25 chance.
  if (!number(0, 24))
  {
    act("Your $q turns &+rblood red as it slashes into $N!", FALSE, ch, obj, vict, TO_CHAR);
    act("&+rBlood red energy travels into your hand and up your arm, seeping into your veins!", FALSE, ch, obj, vict, TO_CHAR);
    act("$n's $q turns &+rblood red as it slashes into $N!", FALSE, ch, obj, vict, TO_NOTVICT);
    act("&+rBlood red energy travels into $n's hand and up $s arm, apparently strengthening $m.", FALSE, ch, obj, vict, TO_NOTVICT);
    act("$n's $q turns &+rblood red as it slashes into you!", FALSE, ch, obj, vict, TO_VICT);
    act("You feel yourself becoming weak as the &+rblood is sucked out of you!", FALSE, ch, obj, vict, TO_VICT);

    vamp(ch, dam / 2, (int) (GET_MAX_HIT(ch) * 1.3));

    GET_HIT(vict) -= dam;
    update_pos(vict);

    return TRUE;
  }
  return FALSE;
}

int flamberge(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   vict;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!ch && cmd == CMD_PERIODIC)
  {
    hummer(obj);
    return TRUE;
  }

  if (!dam)                     /*
                                   if dam is not 0, we have been called when
                                   weapon hits someone
                                 */
    return (FALSE);

  if (!ch)
    return (FALSE);

  if (!OBJ_WORN_POS(obj, WIELD))
    return (FALSE);

  vict = (P_char) arg;

  if (OBJ_WORN_BY(obj, ch) && vict)
  {
    if (!number(0, 24))
    {
      act("Your $q &+rsummons the power of fire and flames at at $N.", FALSE,
          obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q &+rsummons the power of fire and flames at you.", FALSE,
          obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q &+rsummons the power of fire and flames at $N.", FALSE,
          obj->loc.wearing, obj, vict, TO_NOTVICT);
      spell_burning_hands(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      if (char_in_list(vict))
        spell_fireball(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      if (char_in_list(vict))
      {
        act
          ("&+rA &+Rse&+Yari&+Rng &+Yburst &+rleaps from your $q&+r and hits $N &+Rdead on!",
           FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
        act
          ("&+rA &+Rse&+Yari&+Rng &+Yburst &+rleaps from $n's $q&+r and hits you &+Rdead on!",
           FALSE, obj->loc.wearing, obj, vict, TO_VICT);
        act
          ("&+rA &+Rse&+Yari&+Rng &+Yburst &+rleaps from $n's $q&+r and hits $N &+Rdead on!",
           FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
        damage(ch, vict, 350, SPELL_IMMOLATE);


      }
    }
  }
  return (FALSE);
}
#endif

int nightbringer(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   vict;
  int      save;

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  vict = (P_char) arg;
  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_BY(obj, ch) || !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  // 1/30 chance.
  if (!number(0, 29))
  {
    act("Your $q glows brightly as it hits $N.", FALSE, ch, obj, vict, TO_CHAR);
    act("$n's $q glows brightly as it hits you.  You feel woozy.", FALSE, ch, obj, vict, TO_VICT);
    act("$n's $q glows brightly as it hits $N.", FALSE, ch, obj, vict, TO_NOTVICT);
    act("$N looks drowsy.", FALSE, ch, obj, vict, TO_NOTVICT);

    save = vict->specials.apply_saving_throw[SAVING_SPELL];
    vict->specials.apply_saving_throw[SAVING_SPELL] = 20;
    spell_sleep(54, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    vict->specials.apply_saving_throw[SAVING_SPELL] = save;
  }
  // 1/40 chance.
  if( !number(0,39) )
  {
    act("Billowing clouds of &+Ldarkness spill out of $p!", TRUE, ch, obj, 0, TO_ROOM);
    act("Billowing clouds of &+Ldarkness spill out of $p!", TRUE, ch, obj, 0, TO_CHAR);
    spell_darkness(51, ch, 0, 0, 0, 0);
  }

  return FALSE;
}

int avernus(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time, dam;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }
  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }
/*
   if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
    return (FALSE);
*/

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "stone"))
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 60 <= curr_time)
      {
        act("You say 'stone'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'stone'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_stone_skin(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }

  vict = (P_char) arg;
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(vict) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  dam = BOUNDED(0, (GET_HIT(vict) + 9), 200);
  // 1/25 chance.
  if( !number(0, 24) )
/*    && (CheckMultiProcTiming(ch) || !number(0, 2)) && !IS_UNDEADRACE(vict) && !IS_CONSTRUCT(vict))*/
  {
    act("&+LAvernus, the life stealer &+Wglows brightly in your hands as it dives into $N.", FALSE, ch, obj, vict, TO_CHAR);
    act("&+L$p draws the life force out of $N, feeding energy to you.", FALSE, ch, obj, vict, TO_CHAR);
    act("$n's sword &+Wglows with a bright light as it bites into $N.", FALSE, ch, obj, vict, TO_NOTVICT);
    act("$N &+Llooks withered and $n &+Wlooks revitalized.", FALSE, ch, obj, vict, TO_NOTVICT);
    act("$n's sword &+Wglows with a bright light as it bites into you.", FALSE, ch, obj, vict, TO_VICT);
    act("&+LYou feel your life flowing away and $n &+Wlooks revitalized.", FALSE, ch, obj, vict, TO_VICT);

    vamp(ch, dam / 2, (int) (GET_MAX_HIT(ch) * 1.8));

    spell_damage(ch, vict, (BOUNDED(0, (GET_HIT(vict) + 9), 150) * 4), SPLDAM_NEGATIVE, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, 0);
    return TRUE;
  }
  return FALSE;
}


int purple_worm( P_char ch, P_char pl, int cmd, char *arg )
{
  P_obj    obj, next_obj;
  P_char   vict;
  int      id;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC )
  {
    // Look for a corpse to poop out!
    for( obj = ch->carrying; obj; obj = obj->next_content )
    {
      // 10% chance to poop a PC corpse. average digest time == 1 min + 5*6 sec == 1 1/2 min.
      if( obj->type == ITEM_CORPSE && IS_SET(obj->value[1], PC_CORPSE)
        && obj->timer[0] < time(NULL) && !number(0, 9) )
      {
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        act("$n&n &+mst&+Mop&+ms s&+Mud&+mde&+Mnl&+my&n, then begins to &+yu&+Yn&+yd&+Yu&+yl&+Ya&+yt&+Ye&n.  $o&n drops from $s behind.",
          FALSE, ch, obj, 0, TO_ROOM);
        writeCorpse(obj);
        break;
      }
    }
    return TRUE;
  }

  if( cmd == CMD_DEATH )
  {
    act("$n falls to the ground and dissolves into nothing.", FALSE, ch, 0, 0, TO_ROOM);
    for (obj = ch->carrying; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      obj_from_char(obj);
      obj_to_room(obj, ch->in_room);
    }
    return FALSE;
  }

  if( !IS_FIGHTING(ch) && (ch->in_room != NOWHERE) && (MIN_POS(ch, POS_STANDING + STAT_NORMAL)) )
  {
    /* ok we check if there is any PC near */
    if (range_scan_track(ch, 15, SCAN_ANY))
    {
      InitNewMobHunt(ch);
      return FALSE;
    }
  }

  // pl is defined if called from command_interpreter
  if( ch->specials.fighting && !pl )
  {
    if( !number(0, 9) )
    {
      // swallow the bastard
      vict = ch->specials.fighting;

      if( !IS_TRUSTED(vict) && !(IS_NPC(vict) && IS_ELITE(vict)) && !IS_NEXUS_GUARDIAN(vict) && !IS_GH_GOLEM(vict) && !IS_OP_GOLEM(vict) )
      {
        if( vict->in_room == ch->in_room )
        {
          act("You open your mouth and swallow $N whole!", FALSE, ch, 0, vict, TO_CHAR);
          act("$n opens $s gaping maw, and the last thing you feel is\nthe odd sensation of sliding down $s throat.",
             FALSE, ch, 0, vict, TO_VICT);

          if( IS_PC(vict) )
          {
            statuslog(vict->player.level, "%s killed by purple worm swallow at [%d].", GET_NAME(vict),
              ((vict->in_room == NOWHERE) ? -1 : world[vict->in_room].number));
            logit(LOG_DEATH, "%s killed by purple worm swallow at [%d].", GET_NAME(vict),
              ((vict->in_room == NOWHERE) ? -1 : world[vict->in_room].number));
          }

          act("$n swallows $N whole!", FALSE, ch, 0, vict, TO_ROOM);

          if( IS_PC(vict) )
            id = GET_PID(vict);
          else
            id = GET_VNUM(vict);

          die(vict, ch);
          vict = NULL;

          for( P_obj obj = world[ch->in_room].contents; obj; obj = obj->next_content )
          {
            if( obj->value[3] == id )
            {
              obj_from_room(obj);
              obj_to_char(obj, ch);
              if (obj->type == ITEM_CORPSE && IS_SET(obj->value[1], PC_CORPSE))
              {
                writeCorpse(obj);
                // Corpse is digested for a minute
                obj->timer[0] = 60 + time(NULL);
              }
            }
          }
        }
      }
    }
  }
  return FALSE;
}

int piercer(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict = NULL, tmp_ch, next;
  int      dam;
  struct damage_messages messages = {
    "You gently pierce $N when you fall from the ceiling.",
    "$n falls from the ceiling and pierces you.",
    "$n falls from above, pierces $N, and blood spurts all over.",
    "You successfully pierce $N. The dead body falls to the ground.",
    "$n falls from the ceiling and pierces you. You are impaled.",
    "$n falls from the ceiling and impales $N whose body lifelessly falls to the ground."
  };
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd != 0)
  {
    return (FALSE);
  }
  /*
     if fighting, leave alone
   */
  if (!ch->specials.fighting && IS_AFFECTED(ch, AFF_HIDE) &&
      MIN_POS(ch, POS_STANDING + STAT_RESTING))
  {
    for (tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next)
    {
      next = tmp_ch->next_in_room;

      if (IS_PC(tmp_ch) && CAN_SEE(ch, tmp_ch) &&
          !IS_TRUSTED(tmp_ch) && !IS_AFFECTED(tmp_ch, AFF_HIDE))
      {
        vict = tmp_ch;
      }
      if (vict && (ch->in_room == vict->in_room))
      {
        dam = dice(GET_LEVEL(ch), 5) + GET_LEVEL(ch);
        /*
           this is NOT a backstab per se, but awareness gives them a save of
           sorts.
         */
        if ((IS_AFFECTED(vict, AFF_AWARE) ||
             IS_AFFECTED(vict, AFF_SKILL_AWARE)) &&
            StatSave(vict, APPLY_AGI, -10))
        {
          /*
             piercer missed, they dodged
           */
          act("$n crashes to the ground, missing you by a hair!",
              FALSE, ch, 0, vict, TO_VICT);
          act("$n crashes to the ground, missing $N by a hair!",
              FALSE, ch, 0, vict, TO_NOTVICT);
          act("You crash to the ground, missing $N by a hair!",
              FALSE, ch, 0, vict, TO_CHAR);
        }
        else
        {
          if (!IS_FIGHTING(ch))
            set_fighting(ch, vict);
          melee_damage(ch, vict, dam, 0, &messages);
        }
        /*
           piercer is one-shot per mob
         */
        REMOVE_BIT(ch->specials.act, ACT_SPEC);
        return (TRUE);          /*
                                   dont hit the poor guy again
                                 */
      }
    }
  }
  return (FALSE);
}

int elfgate(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      temp, to_room;
  char     Gbuf1[MAX_STRING_LENGTH];
  P_char   t_ch;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || !arg)
    return (FALSE);

  if (cmd != CMD_ENTER)
    return (FALSE);

  if (((GET_RACE(ch) != RACE_GREY) && (GET_RACE(ch) != RACE_HALFELF) &&
       GET_RACE(ch) != RACE_CENTAUR))
  {
    send_to_char
      ("You are not of TRUE faerie blood, you may not enter this gate.\n",
       ch);
    return (FALSE);
  }
  if (GET_LEVEL(ch) < 1)
  {
    send_to_char
      ("The gate flares briefly, but refuses to transport someone of your level.\n",
       ch);
    return (FALSE);
  }
  one_argument(arg, Gbuf1);
  if (!isname(Gbuf1, obj->name))
    return (FALSE);

  act("As you step into the $o, there is a blinding flash of light!", FALSE,
      ch, obj, 0, TO_CHAR);
  act
    ("You are ripped through a dark and star-filled void, pain sears through",
     FALSE, ch, obj, 0, TO_CHAR);
  act("your body!  When you again open your eyes, you are elsewhere...",
      FALSE, ch, obj, 0, TO_CHAR);
  act("$n wades into the $o.", FALSE, ch, obj, 0, TO_ROOM);

  temp = (number(0, 3));
#if 0
  teleport_to(ch, real_room(obj->value[temp]));
#else
  do
  {
    to_room = number(0, top_of_world);
  }
  while (IS_SET(world[to_room].room_flags, PRIVATE) ||
         IS_SET(world[to_room].room_flags, PRIV_ZONE) ||
         IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
         world[to_room].sector_type == SECT_OCEAN ||
         world[to_room].number < 110000 || world[to_room].number >= 210000);
/*         !IS_MAP_ROOM(to_room)); */
  if (IS_FIGHTING(ch))
    stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  if (ch->in_room != NOWHERE)
    for (t_ch = world[ch->in_room].people; t_ch; t_ch = t_ch->next)
      if (IS_FIGHTING(t_ch) && (t_ch->specials.fighting == ch))
        stop_fighting(t_ch);
  char_from_room(ch);
  char_to_room(ch, to_room, -1);
  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
#endif
  return (TRUE);
}

int nexus(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      temp, to_room;
  char     Gbuf1[MAX_STRING_LENGTH];
  P_char   t_ch;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || !arg)
    return (FALSE);

  if (cmd != CMD_ENTER)
    return (FALSE);

  if (GET_LEVEL(ch) < 1)
  {
    send_to_char
      ("The gate flares briefly, but refuses to transport someone of your level.\n",
       ch);
    return (FALSE);
  }
  one_argument(arg, Gbuf1);
  if (!isname(Gbuf1, obj->name))
    return (FALSE);

  act("As you step into the $o, there is a blinding flash of light!", FALSE,
      ch, obj, 0, TO_CHAR);
  act
    ("You are ripped through a dark and star-filled void, pain sears through",
     FALSE, ch, obj, 0, TO_CHAR);
  act("your body!  When you again open your eyes, you are elsewhere...",
      FALSE, ch, obj, 0, TO_CHAR);
  act("$n wades into the $o.", FALSE, ch, obj, 0, TO_ROOM);

  temp = (number(0, 3));
#if 0
  teleport_to(ch, real_room(obj->value[temp]));
#else
  do
  {
    to_room = number(0, top_of_world);
  }
  while (IS_SET(world[to_room].room_flags, PRIVATE) ||
         IS_SET(world[to_room].room_flags, PRIV_ZONE) ||
         IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
         world[to_room].sector_type == SECT_OCEAN ||
         !IS_SURFACE_MAP(to_room));
  if (IS_FIGHTING(ch))
    stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  if (ch->in_room != NOWHERE)
    for (t_ch = world[ch->in_room].people; t_ch; t_ch = t_ch->next)
      if (IS_FIGHTING(t_ch) && (t_ch->specials.fighting == ch))
        stop_fighting(t_ch);
  char_from_room(ch);
  char_to_room(ch, to_room, -1);
  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
#endif
  return (TRUE);
}

int magic_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = obj->value[1];
  char     Gbuf1[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_ENTER || !OBJ_ROOM(obj) || !IS_ALIVE(ch) || !arg )
  {
    return FALSE;
  }

  one_argument(arg, Gbuf1);
  // If not the right portal..
  if( obj != get_obj_in_list(Gbuf1, world[ch->in_room].contents) )
  {
    return FALSE;
  }

  if (real_room(obj->value[0]) == NOWHERE)
  {
    send_to_char
      ("Hmm...  Looks like it's busted.  Might wanna notify a god.\n", ch);
    return (FALSE);
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  act("As you step into the $o, there is a blinding flash of light!", FALSE,
      ch, obj, 0, TO_CHAR);
  act
    ("You are ripped through a dark and star-filled void, pain sears through",
     FALSE, ch, obj, 0, TO_CHAR);
  act("your body!  When you again open your eyes, you are elsewhere...",
      FALSE, ch, obj, 0, TO_CHAR);
  act("$n vanishes into the $o.", FALSE, ch, obj, 0, TO_ROOM);

  if (!IS_TRUSTED(ch))
  {
    if (GET_HIT(ch) > dam)
      GET_HIT(ch) -= dam;
    else
      GET_HIT(ch) = 1;
    StartRegen(ch, EVENT_HIT_REGEN);
  }
  teleport_to(ch, real_room(obj->value[0]), 0);

  return (TRUE);
}

// For the gate to ardgral spell.
int magic_map_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = obj->value[1];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      target_room;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_ENTER || !IS_ALIVE(ch) || !arg || !OBJ_ROOM(obj) )
  {
    return FALSE;
  }

  one_argument(arg, Gbuf1);
  // If not the right portal..
  if( obj != get_obj_in_list(Gbuf1, world[ch->in_room].contents) )
  {
    return FALSE;
  }

  target_room = real_room(random_map_room());

  while( world[target_room].sector_type == SECT_MOUNTAIN
    || world[target_room].sector_type == SECT_INSIDE
    || world[target_room].sector_type == SECT_OCEAN )
  {
    target_room = real_room(random_map_room());
  }


  if( target_room == NOWHERE )
  {
    debug( "magic_map_pool: Target room is NOWHERE for char '%s'.", J_NAME(ch) );
    send_to_char("Hmm...  Looks like it's busted.  Might wanna notify a god.\n", ch);
    return FALSE;
  }

  act("As you step into the $o, there is a blinding flash of light!", FALSE, ch, obj, 0, TO_CHAR);
  act("You are ripped through a dark and star-filled void, pain sears through", FALSE, ch, obj, 0, TO_CHAR);
  act("your body!  When you again open your eyes, you are elsewhere...", FALSE, ch, obj, 0, TO_CHAR);
  act("$n vanishes into the $o.", FALSE, ch, obj, 0, TO_ROOM);

  if( !IS_TRUSTED(ch) )
  {
    // Tighter like this.
    GET_HIT(ch) = (GET_HIT(ch) > dam) ? GET_HIT(ch) - dam : 1;
    StartRegen(ch, EVENT_HIT_REGEN);
  }
  teleport_to(ch, target_room, 0);

  return TRUE;
}

int random_map_room()
{
  return number(SURFACE_MAP_START, SURFACE_MAP_END);
}

int githyanki(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  P_char   temp;

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  vict = (P_char) arg;
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) || !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  if (!number(0, 35))
  {
    if( IS_PC(ch) )
    {
      obj->value[7]++;
    }
    if( obj->value[7] > 20 && !number(0, 4) )
    {
      // Kiss it goodbye
      act("A booming voice says 'Foolish mortal!! You can't contain the power of Gith for long!'", FALSE, ch, obj, vict, TO_CHAR);
      act("A booming voice says 'Foolish mortal!! You can't contain the power of Gith for long!'", FALSE, ch, obj, vict, TO_ROOM);
      act("Your $q explodes into a thousand pieces.", FALSE, ch, obj, 0, TO_CHAR);
      act("BOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM! You are thrown from your feet!", FALSE, ch, obj, vict, TO_CHAR);
      act("$n's $q explodes into a thousand pieces, knocking $m flat!", FALSE, ch, obj, 0, TO_ROOM);

      SET_POS(ch, POS_SITTING + GET_STAT(ch));
      unequip_char(ch, WIELD);
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      return TRUE;
    }
    switch (number(1, 5))
    {
    case 1:
    case 2:
      if (GET_LEVEL(vict) < 50)
      {
        act("$n's weapon releases a blinding flash of white light!", FALSE, ch, obj, vict, TO_ROOM);
        act("Your sword releases a blinding flash of white light!", FALSE, ch, obj, vict, TO_CHAR);
        act("Your $q sings through the air as it lops $N's head off!", FALSE, ch, obj, vict, TO_CHAR);
        act("$n's $q sings through the air as it lops $N's head off!", FALSE, ch, obj, vict, TO_ROOM);
        act("$n's $q glows brightly and you see it coming towards your neck at an alarming rate of speed!", FALSE, ch, obj, vict, TO_VICT);
        die(vict, ch);
        vict = NULL;
      }
      break;
    // Mob attempts to reclaim
    case 3:
    case 4:
      act("&+cYour $q &+Csings &+cas it slashes into $N!", FALSE, ch, obj, vict, TO_CHAR);
      act("&+cYou feel slightly invigorated!", FALSE, ch, obj, vict, TO_CHAR);
      act("$n&+c's $q &+Csings &+cas it slashes into $N!", FALSE, ch, obj, vict, TO_NOTVICT);
      act("$n&+c's $q &+Csings &+cas it slashes into you!", FALSE, ch, obj, vict, TO_VICT);
      vamp(ch, 200, (int) (GET_MAX_HIT(ch) * 1.3));
      GET_HIT(vict) -= 200;
      update_pos(vict);
      break;
    case 5:
      if (IS_PC(ch) && (temp = read_mobile(19790, VIRTUAL)))
      {
          // Change to accomodate new agg code, move him in, skip agg check, do his acts, then start a fight with wielder.  JAB
          char_to_room(temp, ch->in_room, -1);
          act("BOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!  There is a great clap of thunder!", FALSE, temp, obj, 0, TO_ROOM);
          act("A deadly looking Githyanki Knight slowly fades into existence..", FALSE, temp, obj, 0, TO_ROOM);
          act("$n screams 'I have come to reclaim our Silver Sword for Gith!'", FALSE, temp, obj, 0, TO_ROOM);
          act("$n growls 'You have no right to possess it! DIE!!!'", FALSE, temp, obj, 0, TO_ROOM);
          MobStartFight(temp, ch);
      }
      break;
    default:
      break;
    }
  }
  else
  {
    if( !ch->specials.fighting )
    {
      set_fighting(ch, vict);
    }
  }
  if( ch->specials.fighting )
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

int githpc_special_weap(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000, i, j, k;
  char     last_key[256] = "\0";
  P_char   vict, t_ch;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) )
  {
    return FALSE;
  }

  if( obj->loc.wearing == ch )
  {
    // 1/200 chance.
    if( ( !number(0, 199)) && !CHAR_IN_JUSTICE_AREA(ch) && !CHAR_IN_TOWN(ch) && strlen(obj->name) )
    {
      /* get the last keyword of the keyword list */
      for( i = strlen(obj->name) - 1; (i >= 0) && (obj->name[i] != ' '); i-- )
      {
        ;
      }

      for( k = i + 1, j = 0; obj->name[k]; k++ )
      {
        last_key[j] = obj->name[k];
        j++;
      }
      last_key[j] = '\0';

      /* make sure we actually got something..  you never know */
      if( j )
      {
        last_key[j - 1] = '\0'; /* get rid of extra letter */

        vict = get_char(last_key);
        if( IS_ALIVE(vict) && IS_PC(vict) && (vict->in_room != ch->in_room) )
        {
          // 1/7 chance.
          if( !number(0, 6) )
          {
            /* time to summon */
            if( IS_FIGHTING(vict) )
            {
              stop_fighting(vict);
            }
            if( IS_DESTROYING(vict) )
            {
              stop_destroying(vict);
            }
            if( vict->in_room != NOWHERE )
            {
              for( t_ch = world[vict->in_room].people; t_ch; t_ch = t_ch->next_in_room )
              {
                if( IS_FIGHTING(t_ch) && (t_ch->specials.fighting == vict) )
                {
                  stop_fighting(t_ch);
                }
              }
            }

            act("&+W$n suddenly disappears in a flash of light!", TRUE, vict, 0, 0, TO_ROOM);
            send_to_char("&+WSuddenly, a flash of light obscures your vision.  You feel as though you are falling at an incredible rate, but yet you can hear nothing.  You find yourself elsewhere...\n", vict);
            char_from_room(vict);
            char_to_room(vict, ch->in_room, -1);
            act("&+WIn a flash of light, $n suddenly arrives.", FALSE, vict, 0, 0, TO_ROOM);
          }
          else
          {
            act("&+WYour weapon glows with &+Bpower... &+Wthen suddenly disappears in a flash of light!", FALSE, ch, 0, 0, TO_CHAR);
            act("&+W$n's&+W weapon glows with &+Bpower... &+Wthen suddenly disappears in a flash of light!", FALSE, ch, 0, 0, TO_ROOM);
            obj_to_char(unequip_char(ch, WIELD), ch);
            obj_from_char(obj);
            obj_to_char(obj, vict);
            act("A bright flash of light erupts from your hands!!! When the light dies down, you realize $p has returned to your posession!", FALSE, vict, 0, obj, TO_CHAR);
            act("A bright flash of light erupts from $n's hands!!!", FALSE, vict, 0, 0, TO_ROOM);
            return TRUE;
          }
        }
        else                    /* let them know that it tried */
        {
          act("&+W$n's $q&+W flares up briefly, then returns to normal.", FALSE, ch, obj, 0, TO_ROOM);
          act("&+WYour $q&+W flares up briefly, then returns to normal.", FALSE, ch, obj, 0, TO_CHAR);
        }
      }
    }
    else if( !number(0, 399) )
    {
      vict = ch->specials.fighting;
      if (vict && (GET_LEVEL(vict) < 51) && !IS_TRUSTED(vict))
      {
        /* sword */
        if( obj->R_num == real_object(18) )
        {
          act("&+WIn a blinding burst of light, $n&+W's $q&+W suddenly flashes out at incredible speed, slashing at the head of $N&+W and instantly beheading $M!", FALSE, ch, obj, vict, TO_NOTVICT);
          act("&+WIn a blinding burst of light, your $q&+W suddenly flashes out at incredible speed, slashing at the head of $N&+W and instantly beheading $M!", FALSE, ch, obj, vict, TO_CHAR);
          act("&+WIn a blinding burst of light, $n&+W's $q&+W suddenly flashes out at incredible speed, slashing at your head and resulting in your instant beheading!", FALSE, ch, obj, vict, TO_VICT);

          die(vict, ch);
        }
        /* staff */
        else if( obj->R_num == real_object(19) )
        {
          act("&+WIn a blinding burst of light, $n&+W's $q&+W suddenly stabs at $N&+W's head with incredible speed, instantly beheading $M!", FALSE, ch, obj, vict, TO_NOTVICT);
          act("&+WIn a blinding burst of light, your $q&+W suddenly stabs at $N&+W's head with incredible speed, instantly beheading $M!", FALSE, ch, obj, vict, TO_CHAR);
          act("&+WIn a blinding burst of light, $n&+W's $q&+W suddenly stabs at your head with incredible speed, instantly beheading you!", FALSE, ch, obj, vict, TO_VICT);

          die(vict, ch);
        }
        /* hammer */
        else if( obj->R_num == real_object(418) )
        {
          act("&+WIn a blinding burst of light, $n&+W's $q&+W suddenly smashes $N&+W's head with incredible speed, instantly beheading $M!", FALSE, ch, obj, vict, TO_NOTVICT);
          act("&+WIn a blinding burst of light, your $q&+W suddenly smashes $N&+W's head with incredible speed, instantly beheading $M!", FALSE, ch, obj, vict, TO_CHAR);
          act("&+WIn a blinding burst of light, $n&+W's $q&+W suddenly smashes your head with incredible speed, instantly beheading you!", FALSE, ch, obj, vict, TO_VICT);

          die(vict, ch);
        }
        /* bug */
        else
        {
          wizlog(MINLVLIMMORTAL, "githpc_special_weap: Item '%s' vnum (%d) not recognized.\n", obj->short_description, GET_OBJ_VNUM(obj) );
        }
        return TRUE;
      }
    }
    else
    {
      vict = (P_char) arg;

      if( !ch->specials.fighting && vict )
      {
        set_fighting(ch, vict);
      }
    }
  }

  if( ch->specials.fighting )
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

int tiamat(P_char ch, P_char pl, int cmd, char *arg)
{
  char     bufs[6][100];
  struct damage_messages messages = {
    bufs[0], bufs[1], bufs[2], bufs[3], bufs[4], bufs[5], 0
  };
  char     colors[5][16] = {
    "&+rred", "&+Lblack", "&+bblue", "&+ggreen", "&+Wwhite"
  };
  P_char vict, next_ch;
  P_obj t_obj, next;
  int damtype, i;
  void (*funct) (int, P_char, char *, int, P_char, P_obj);

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
    
  if(IS_IMMOBILE(ch))
    return false;
    
  if(cmd == CMD_DEATH)
  {
    debug("&+LTiamat death called.");
    act("&+WWith her very last breath, &+LTiamat &+Wcloses her eyes.\n"
        "&+LTiamat's &+Wbody begins to shimmer brilliantly, her flesh becoming more and more translucent!\n"
        "&+WAs the light subsides, only a few pieces of her once great body remain.", FALSE, ch, 0, 0, TO_ROOM);

    for (t_obj = ch->carrying; t_obj; t_obj = next)
    {
      next = t_obj->next_content;
      obj_from_char(t_obj);
      obj_to_room(t_obj, ch->in_room);
    }
    
    for (i = 0; i < MAX_WEAR; i++)
      if (ch->equipment[i])
        obj_to_room(unequip_char(ch, i), ch->in_room);
    
    P_obj obj = read_object(55080, VIRTUAL);
    obj_to_room(obj, ch->in_room);
    obj->value[0] = SECS_PER_MUD_DAY / PULSE_MOBILE * WAIT_SEC;

    return true;
  }

  if (ch->in_room == real_room(19617))
  {
    if(cmd == CMD_SOUTH &&
      !IS_TRUSTED(pl))
    {
      act("$N &+Rgrowls in anger, as you approach $S treasure!", FALSE, pl, 0,
          ch, TO_CHAR);
      act
        ("$N &+Rgrowls in anger, as $n&+R makes an attempt at $S treasure!",
         FALSE, pl, 0, ch, TO_ROOM);
      
      MobStartFight(ch, pl);
      
      return TRUE;
    }
    
    if ((cmd == CMD_SLAP))
    {
      MobStartFight(ch, pl);
      return FALSE;
    }
    
    if (affected_by_spell(ch, SPELL_SILENCE))
    {
      act("&+LWith a mighty &+RROAR&+l, &+LTiamat shreds the blanket of silence!", 0,
          ch, 0, 0, TO_ROOM);
      affect_from_char(ch, SPELL_SILENCE);
    }
    
    if(!IS_SET(world[ch->in_room].room_flags, MAGIC_LIGHT))
    {
      act
        ("A beam of light radiates from &+LTiamats eyes, lighting a sconce upon the wall.",
         0, ch, 0, 0, TO_ROOM);
      SET_BIT(world[ch->in_room].room_flags, MAGIC_LIGHT);
    }                           /*
                                   below here, is not room specific
                                 */
  }

  if (cmd != 0)
    return FALSE;

  switch (number(1, 3))
  {
  case 1:                      /* Knock em all on their ass */
    act("&+LTiamat lashes out with her mighty tail!", FALSE, ch, 0, 0,
        TO_ROOM);
    for (vict = world[ch->in_room].people; vict; vict = next_ch)
    {
      next_ch = vict->next_in_room;
      
      if(IS_TRUSTED(vict))
        continue;

      if(!IS_DRAGON(vict) &&
         !affected_by_spell(vict, SKILL_BERSERK) &&
         IS_FIGHTING(vict) &&
         (vict->specials.fighting == ch))
      {
        if (!StatSave(vict, APPLY_AGI, -4))
        {
          SET_POS(vict, POS_SITTING + GET_STAT(vict));
          
          CharWait(vict, PULSE_VIOLENCE * 2);
          
          act("&+LThe powerful sweep sends you crashing to the ground!",
              FALSE, vict, 0, 0, TO_CHAR);
          act("$n&+L crashes to the ground!", FALSE, vict, 0, 0, TO_ROOM);
        }
        else
          act("&+LYou nimbly dodge the sweep!", FALSE, vict, 0, 0, TO_CHAR);
      }
    }

    break;

  case 2:
    if (!(vict = char_in_room(ch->in_room)))    /* Bitch slap someone */
    {
      vict = ch->specials.fighting;
    }
    
    if(vict &&
      !IS_DRAGON(vict) &&
      !affected_by_spell(vict, SKILL_BERSERK) &&
      IS_FIGHTING(vict) &&
      (vict->specials.fighting == ch))
    {
      if (!StatSave(vict, APPLY_AGI, -4))
      {
        struct damage_messages msgs = {
          "", "&+LHer immense tail broadsides you in the face!",
          "$N&+L is slapped by Tiamat's tail, and crashes to the ground!",
          "", msgs.victim, msgs.room
        };
        
        SET_POS(vict, POS_SITTING + GET_STAT(vict));
        
        CharWait(vict, PULSE_VIOLENCE * 2);
        
        melee_damage(ch, vict, dice(50, 50) + 5, 0, &msgs);
      }
      else
      {
        act("&+LYou nimbly jump $n's tail sweep!", FALSE, ch, 0, vict,
            TO_VICT);
        act("$N &+Lnimbly jumps $n's fierce tail sweep!", TRUE, ch, 0, vict,
            TO_NOTVICT);
      }
    }

    break;
  case 3:
  default:
    if (!(vict = char_in_room(ch->in_room)))    /* Bitch stab someone */
    {
      vict = ch->specials.fighting;
    }
    
    {
      if(vict &&
        !IS_DRAGON(vict) &&
        !affected_by_spell(vict, SKILL_BERSERK) &&
        IS_FIGHTING(vict) &&
        (vict->specials.fighting == ch) &&
        !IS_TRUSTED(vict))
      {
        struct damage_messages msgs = {
          "", "&+RHer tail lashes out, stabbing you deep in the back!",
          "$N makes a strange sound as &+LTiamat's tail stabs $M in the back!",
          msgs.attacker, msgs.victim, msgs.room
        };

        melee_damage(ch, vict, dice(10, 30), PHSDAM_NOREDUCE, &msgs);

        if(IS_ALIVE(vict))
        {
          spell_poison(59, ch, 0, 0, vict, NULL);
        }

      }
    }
    break;
  }
  /* End of tail */

  for (i = 1; i < 6; i++)
  {                             /* Loop through the 5 heads */
    switch (number(1, 3))
    {
    case 1:
    case 2:                    /* Bite someone          */
      if (!(vict = char_in_room(ch->in_room)))
        vict = ch->specials.fighting;
      if (vict)
      {
        sprintf(bufs[0], "You bite $N with your %s head.", colors[i - 1]);
        sprintf(bufs[1], "The %s head of $n lashes out and bites you.",
                colors[i - 1]);
        sprintf(bufs[2], "$n bites $N with $m %s head.", colors[i - 1]);
        sprintf(bufs[3],
                "You bite $N with your %s head until they are dead.",
                colors[i - 1]);
        sprintf(bufs[4], "The %s head of $n bites you... to DEATH!",
                colors[i - 1]);
        sprintf(bufs[5], "$n bites $N in two with $m %s head! $N is dead.",
                colors[i - 1]);
        switch (i)
        {
        case 1:
          damtype = SPLDAM_FIRE;
          act("&+RFire courses through your blood as she bites deep!", FALSE,
              vict, 0, 0, TO_CHAR);
          break;
        case 2:
          damtype = SPLDAM_GENERIC;
          act
            ("&+BYour heart skips a beat or three as her mighty teeth clamp down upon your arm!",
             FALSE, vict, 0, 0, TO_CHAR);
          break;
        case 3:
          damtype = SPLDAM_COLD;
          act
            ("&+WShe rips into your skin, sending shivers of intense cold through your body!",
             FALSE, vict, 0, 0, TO_CHAR);
          break;
        case 4:
          damtype = SPLDAM_ACID;
          act
            ("&+LShe lashes out at you, the acid from her fangs burning immensly!",
             FALSE, vict, 0, 0, TO_CHAR);
          break;
        case 5:
          damtype = SPLDAM_ACID;
          act
            ("&+GYou feel a wave of poisonous nausea, as her teeth sink deep!",
             FALSE, vict, 0, 0, TO_CHAR);
          break;
        }
        spell_damage(ch, vict, dice(6, 6), damtype,
                     SPLDAM_BREATH | SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
                     &messages);
      }
      break;

    case 3:
      switch (i)
      {
      case 1:
        act("&+LTiamat's&+R red head breathes fire!", 1, ch, 0, 0, TO_ROOM);
        funct = spell_fire_breath;
        break;
      case 2:
        act("&+LTiamat's&+B blue head breathes &=LBelectricity!", 1, ch, 0,
            0, TO_ROOM);
        funct = spell_lightning_breath;
        break;
      case 3:
        act("&+LTiamat's&+W white head breathes frost!", 1, ch, 0, 0,
            TO_ROOM);
        funct = spell_frost_breath;
        break;
      case 4:
        act("&+LTiamat's black head breathes acid!", 1, ch, 0, 0, TO_ROOM);
        funct = spell_acid_breath;
        break;
      case 5:
        act("&+LTiamat's black head breathes blackness!", 1, ch, 0, 0, TO_ROOM);
        funct = spell_shadow_breath_2;
        break;
      default:
        act("&+LTiamat's&+g green head breathes poison gas!", 1, ch, 0, 0,
            TO_ROOM);
        funct = spell_gas_breath;
        break;

      }

      for (vict = world[ch->in_room].people; vict; vict = next_ch)
      {
        if (!is_char_in_room(vict, ch->in_room))
        {
          break;
        }
        
        next_ch = vict->next_in_room;

        if(!IS_DRAGON(vict))
        {                       /* items, lets mess with some prot.  */
          funct(60, ch, NULL, 0, vict, 0);      /* also, since we ARE the queen :) */
          
          if(!is_char_in_room(vict, ch->in_room))
          {
            continue;
          }
          
          switch (i)
          {
          case 1:
            if (IS_AFFECTED(vict, AFF_BARKSKIN))
            {
              struct damage_messages msgs = {
                "$N &+Rcatches on fire!",
                "&+RThe burst of flame causes your bark-like skin to catch fire!",
                "$N &+Rcatches on fire!",
                "$N &+Rcatches on fire!",
                "&+RThe burst of flame causes your bark-like skin to catch fire!",
                "$N &+Rcatches on fire!"
              };
              spell_damage(ch, vict, dice(5, 20), SPLDAM_FIRE,
                           SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &msgs);
            }
            break;
          case 2:
            if (IS_AFFECTED(vict, AFF_HASTE))
            {
              struct damage_messages msgs = {
                "$N's&+B face turns a shade of blue, as $s heart stops for a moment!",
                "&+BBetween your spell of haste and &+LTiamat's &+Belectrical discharge, your heart cracks under the strain!",
                "$N's&+B face turns a shade of blue, as $s heart stops for a moment!",
                "$N's&+B face turns a shade of blue, as $s heart stops for a moment!",
                "&+BBetween your spell of haste and &+LTiamat's &+Belectrical discharge, your heart cracks under the strain!",
                "$N's&+B face turns a shade of blue, as $s heart stops for a moment!"
              };
              spell_damage(ch, vict, dice(20, 20), SPLDAM_LIGHTNING,
                           SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &msgs);
            }
            break;
          case 3:
            if (has_skin_spell(vict))
            {
              struct damage_messages msgs = {
                "$N &+Wscreams in pain as $S stone-like skin cracks under the intense cold!",
                "&+WThe intense coldness cracks your stone-like skin!",
                "$N &+Wscreams in pain as $S stone-like skin cracks under the intense cold!",
                "$N &+Wscreams in pain as $S stone-like skin cracks under the intense cold!",
                "&+WThe intense coldness cracks your stone-like skin!",
                "$N &+Wscreams in pain as $S stone-like skin cracks under the intense cold!"
              };
              spell_damage(ch, vict, dice(5, 100), SPLDAM_COLD,
                           SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &msgs);
            }
            break;
          case 4:
          if (!number(0, 5))
          {
            {
              act("&+LTiamat &+RROARS &+Lloudly, and her tailsweep sends you crashing into the wall!",
                  FALSE, ch, 0, vict, TO_VICT);
              act("&+LTiamat &+RROARS &+Lloudly, and in a frightening display, whips her tail deftly about the room!",
                  FALSE, ch, 0, vict, TO_NOTVICT);
              SET_POS(vict, POS_SITTING + GET_STAT(vict));
              
              stop_fighting(vict);
              if( IS_DESTROYING(vict) )
                stop_destroying(vict);
              
              if(CAN_ACT(vict))
              {                 // prevent cumulative stun/lag
                Stun(vict, ch, PULSE_VIOLENCE * 2, TRUE);
                CharWait(vict, PULSE_VIOLENCE * 3);
              }
            }
          }
            break;
          case 5:
            break;
          default:
            break;
          }
        }
      }
      break;

    }
  }
  return TRUE;
}

int dranum_jurtrem(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != -2)
    return (FALSE);

  if (!ch->specials.fighting)
    return (FALSE);

  vict = ch->specials.fighting;

  if (!number(0, 19) && vict != ch && !IS_TRUSTED(vict))
  {
    call_solve_sanctuary(ch, vict);
  }
  return TRUE;
}

int bulette(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd != 0)                 /*
                                   mobact.c
                                 */
    return (FALSE);

  switch (number(0, 40))
  {
  case 0:
    mobsay(ch, "Plumber!");
    return (TRUE);

  case 1:
    mobsay(ch, "Candygram!");
    return (TRUE);

  case 2:
    mobsay(ch, "Flowers!");
    return (TRUE);

  case 3:
    mobsay(ch, "I'm just a dolphin, Ma'am.");
    return (TRUE);

  default:
    return (FALSE);
  }
}

int tiamat_stinger(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }
  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  vict = (P_char) arg;
  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || !IS_ALIVE(vict) )
  {
    return FALSE;
  }
/*
   if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
    return (FALSE);
*/
  // 1/16 chance.
  if( !number(0, 15) )
  {
    act("Your $q &+Glashes out in a blur of speed at $N.", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
    act("$n's $q &+Gstrikes at you with a blur of speed!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
    act("$n's $q &+Gstrikes out in a blur of speed at $N.", FALSE, obj->loc.wearing, obj, vict, TO_ROOM);
    spell_poison(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    if( char_in_list(vict) && !isname("tiamat", GET_NAME(vict)) )
    {
      spell_major_paralysis(56, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
  }
  return FALSE;
}

#ifdef THARKUN_ARTIS
int holy_mace(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam;
  struct proc_data *data;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !ch && cmd == CMD_PERIODIC)
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if (cmd == CMD_MELEE_HIT)
  {
   /*
    if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
      return (FALSE);
   */
    vict = (P_char) arg;

    if( OBJ_WORN_BY(obj, ch) && IS_ALIVE(vict) && CheckMultiProcTiming(ch) )
    {
      // 1/15 chance.
      if( !number(0, 14) )
      {
        act("Your $q &+Rcalls down the power of Kossuth on $N!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
        act("$n's $q &+Rcalls down the power of Kossuth at you!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
        act("$n's $q &+Rcalls down the power of Kossuth on $N!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
        spell_flamestrike(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        if( char_in_list(vict) && char_in_list(ch) )
        {
          spell_full_harm(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        }
      }
    }
    return FALSE;
  }
  else if (cmd == CMD_GOTNUKED && !number(0, 3))
  {
    if( !(data = (struct proc_data *) arg) )
    {
      return FALSE;
    }
    dam = data->dam;

    act("$n's $p glows brilliantly, and a healing eldritch fire surrounds $s body!", FALSE, ch, obj, NULL, TO_ROOM);
    act("Your $p glows brilliantly, and a healing eldritch fire surrounds you!", FALSE, ch, obj, vict, TO_CHAR);
    vamp(ch, 0.1 * dam, GET_MAX_HIT(ch));

    return TRUE;
  }

  return FALSE;
}
#else

int holy_mace(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam;
  struct proc_data *data;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if (cmd == CMD_MELEE_HIT)
  {
    if( !OBJ_WORN_POS(obj, WIELD) )
    {
      return (FALSE);
    }

    vict = (P_char) arg;

    // 1/25 chance.
    if( OBJ_WORN_BY(obj, ch) && IS_ALIVE(vict) && CheckMultiProcTiming(ch) && !number(0, 24) )
    {
      act("Your $q &+Rcalls down the power of Kossuth on $N!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q &+Rcalls down the power of Kossuth at you!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q &+Rcalls down the power of Kossuth on $N!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);

      spell_flamestrike(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      if( IS_ALIVE(vict) && IS_ALIVE(ch) )
      {
        spell_full_harm(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      if( !IS_ALIVE(vict) || !IS_ALIVE(ch) )
      {
        return TRUE;
      }
    }
    return FALSE;
  }
  else if( cmd == CMD_GOTNUKED && !number(0, 3) )
  {
    if( !(data = (struct proc_data *) arg) )
    {
      return TRUE;
    }
    vict = data->victim;
    dam = data->dam;
    if( !IS_ALIVE(vict) )
    {
      return TRUE;
    }

    if( IS_NPC(vict) )
    {
      GET_HIT(ch) += dam;
    }
    else
    {
      GET_HIT(ch) += (int) (dam * 0.25);
    }
    if( GET_HIT(ch) > GET_MAX_HIT(ch) )
    {
      GET_HIT(ch) = GET_MAX_HIT(ch);
    {
    act("$n's $p glows brilliantly, and a healing eldritch fire surrounds $s body!", FALSE, ch, obj, vict, TO_NOTVICT);
    act("$n's $p glows brilliantly, and a healing eldritch fire surrounds $s body!", FALSE, ch, obj, vict, TO_VICT);
    act("Your $p glows brilliantly, and a healing eldritch fire surrounds you!", FALSE, ch, obj, vict, TO_CHAR);

    return TRUE;
  }
  return FALSE;
}
#endif

int elvenkind_cloak(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char   tch;
  int      count, it;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // 1/4 chance to deflect nuke
  if( cmd != CMD_GOTNUKED || number(0, 3) )
  {
    return FALSE;
  }

  data = (struct proc_data *) arg;

  if( !data || ch == data->victim )
  {
    return FALSE;
  }

  for( tch = world[ch->in_room].people, count = 0; tch; tch = tch->next_in_room )
  {
    if( !IS_TRUSTED(tch) )
    {
      count++;
    }
  }

  if( count < 2 )
  {
    return FALSE;
  }

  it = number(0, count - 2);

  for( tch = world[ch->in_room].people, count = 0; tch; tch = tch->next_in_room )
  {
    if( (tch == ch) || IS_TRUSTED(tch) )
    {
      continue;
    }
    if( count++ == it )
    {
      break;
    }
  }

  if( tch )
  {
    act("$n's $q &+gflashes brilliantly and suddenly flares in front of the spell, deflecting it to YOU!",
      FALSE, ch, obj, tch, TO_VICT);
    act("$n's $q &+gflashes brilliantly and suddenly flares in front of the spell, deflecting it to $N!",
      FALSE, ch, obj, tch, TO_NOTVICT);
    act("Your $q &+gflashes brilliantly and suddenly flares in front of the spell, deflecting it to $N!",
      FALSE, ch, obj, tch, TO_CHAR);
    spell_damage(ch, tch, data->dam, data->attacktype, data->flags | SPLDAM_NODEFLECT, data->messages);
    return TRUE;
  }

  return FALSE;
}


int deflect_ioun(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char   tch;
  int      count, it;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_GOTNUKED || number(0, 3) )
  {
    return FALSE;
  }

  data = (struct proc_data *) arg;

  if( !data || ch == data->victim || !ch->in_room )
  {
    return FALSE;
  }

  for( tch = world[ch->in_room].people, count = 0; tch; tch = tch->next_in_room )
  {
    if( !IS_TRUSTED(tch) )
    {
      count++;
    }
  }

  if( count < 2 )
  {
    return FALSE;
  }

  it = number(0, count - 2);

  for( tch = world[ch->in_room].people, count = 0; tch; tch = tch->next_in_room )
  {
    if( (tch == ch) || IS_TRUSTED(tch) )
    {
      continue;
    }
    if( count++ == it )
    {
      break;
    }
  }

  if( tch )
  {
    act("$n's $p flashes brilliantly and suddenly darts in front of the spell, deflecting it to YOU!",
      FALSE, ch, obj, tch, TO_VICT);
    act("$n's $p flashes brilliantly and suddenly darts in front of the spell, deflecting it to $N!",
      FALSE, ch, obj, tch, TO_NOTVICT);
    act("Your $p flashes brilliantly and suddenly darts in front of the spell, deflecting it to $N!",
      FALSE, ch, obj, tch, TO_CHAR);
    spell_damage(ch, tch, data->dam, data->attacktype, data->flags | SPLDAM_NODEFLECT, data->messages);
    return TRUE;
  }

  return FALSE;
}

struct _BarbProcArtiData
{
  P_obj hammer;
  P_char wielder;
  int damage;
  uint flags;
  int attacktype;
  struct damage_messages messages;
  bool messages_set;
};

void event_dwarven_ancestor_death(P_char ch, P_char victim, P_obj obj, void *data)
{
  act("$n &+rdisappears as &+Lquickly&+r as it came, fading into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
  extract_char(ch);
}

void barb_proc_dwarven_ancestor(int level, P_char ch, P_char victim)
{
  P_char   mob;
  int duration, lvl, i;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  mob = read_mobile(DWARVEN_ANCESTOR_VNUM, VIRTUAL);

  if (!mob)
  {
    logit(LOG_DEBUG, "barb_proc_dwarven_ancestor(): mob %d not loadable", DWARVEN_ANCESTOR_VNUM);
    send_to_char("Bug in artifact procedure.  Tell a god!\n", ch);
    return;
  }

  mob->player.level = GET_LEVEL(ch);
  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
  SET_BIT(mob->specials.affected_by, AFF_DETECT_INVISIBLE);
  mob->player.m_class = CLASS_BERSERKER;

  remove_plushit_bits(mob);

  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(ch);
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(ch);
  mob->points.damnodice = (int) (GET_LEVEL(ch) / 4);
  mob->points.damsizedice = (int) (GET_LEVEL(ch) / 4);
  mob->specials.act |= ACT_SPEC_DIE;
  mob->base_stats.Str = 100;
  mob->base_stats.Dex = 100;
  mob->base_stats.Agi = 100;

  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = (int) (GET_MAX_HIT(ch) * 1.5);
  GET_SIZE(mob) = GET_SIZE(victim);

  char_to_room(mob, ch->in_room, -1);
  act("&+rIn a column of &+RROARING&+r fire, $N&+r makes $S entrance.",
      FALSE, ch, 0, mob, TO_CHAR);
  act("&+rIn a column of &+RROARING&+r fire, $N&+r makes $S entrance.",
      FALSE, ch, 0, mob, TO_ROOM);

  if(!IS_AFFECTED4(mob, AFF4_NOFEAR))
    SET_BIT(mob->specials.affected_by4, AFF4_NOFEAR);

  duration = setup_pet(mob, ch, 1, PET_NOCASH | PET_NOORDER | PET_NOAGGRO);
  add_follower(mob, ch);

  if (duration >= 0)
  {
    duration = MAX(6, (int)(GET_LEVEL(ch) / 4)) * WAIT_SEC;
    add_event(event_dwarven_ancestor_death, duration, mob, NULL, NULL, 0, NULL, 0);
  }

  MonkSetSpecialDie(mob);
  group_add_member(ch, mob);
  berserk(mob, 500);

  for (i = number(1, 2); i; i--)
  {
    if(is_char_in_room(ch, ch->in_room) &&
       is_char_in_room(victim, ch->in_room))
      switch (number(0, 2))
      {
        case 0:
          if(isMaulable(mob, victim))
            maul(mob, victim);
          else if(isKickable(mob, victim) &&
                   number(0, 1))
                      do_kick(mob, 0, 0);
          else if(GET_CHAR_SKILL(mob, SKILL_INFURIATE))
            do_infuriate(mob, NULL, CMD_INFURIATE);
          break;
        case 1:
          act("$n &+rflows into battle!",
            FALSE, mob, 0, 0, TO_ROOM);
          do_rage(mob, NULL, CMD_RAGE);
          break;
        case 2:
          act("$n &+Rlets out an ear-shattering war cry!",
            FALSE, mob, 0, 0, TO_ROOM);
          do_war_cry(mob, 0, 0);
          break;
        default:
          break;
      }
      update_pos(victim);
  }

  if(!IS_FIGHTING(mob) && victim)
    MobStartFight(mob, victim);

  return;
}

// This is the proc for the famous name-chaning mystical warhammer.
int barb( P_obj obj, P_char ch, int cmd, char *arg )
{

  P_char   vict;
  char     buf1[MAX_STRING_LENGTH];
  time_t   curr_time;
  int      damage, type, flags, result;
  struct proc_data *data;
  struct affected_type *af1;

  static int curr_race = RACE_NONE;
  // Whom ever did this is a very bad man; how do we know to reset when they rent/die/etc?
  //   We must use some code in extract_obj (or free_obj is better) and call this function with the flag.
  static struct _BarbProcArtiData BarbProcData =
    // hammer, wielder, damage, flags, attacktype, messages(char* x 6, type, obj), messages_set
    { NULL, NULL, 0, 0, 0, {NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL}, FALSE };

  // Deflect a spell with bad messages.
  struct damage_messages messages_deflect = {
    "&+bA wave of &+Bmagical &+Cenergy&+b released from your weapon strikes down at $N!",
    "&+bYou are hit with a wave of &+Bmagical &+Cenergy!",
    "&+bA wave of &+Bmagical &+Cenergy&+b strikes down at $N!",
    "&+bA wave of &+Bmagical &+Cenergy&+b release from your weapon overwhelms $N, and he is no more!",
    "&+bA wave of &+Bmagical &+Cenergy&+b overwhelms you, the rest is darkness...",
    "&+bA wave of &+Bmagical &+Cenergy&+b overwhelms $N, an he is no more.",
    0
  };

  // Set periodic 'cause we hum.
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  // We hum if on ground or if on char.
  if( cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  // Hammer being removed from game; we need to reset variables.
  if( cmd == CMD_BARB_REMOVE )
  {
    // hammer, wielder, damage, flags, attacktype, messages(char* x 6, type, obj), messages_set
    BarbProcData.hammer = NULL;
    BarbProcData.wielder = NULL;
    BarbProcData.damage = 0;
    BarbProcData.flags = 0;
    BarbProcData.attacktype = 0;
    if( BarbProcData.messages_set )
    {
      str_free( BarbProcData.messages.attacker );
      BarbProcData.messages.attacker = NULL;
      str_free( BarbProcData.messages.victim );
      BarbProcData.messages.victim = NULL;
      str_free( BarbProcData.messages.room );
      BarbProcData.messages.room = NULL;
      str_free( BarbProcData.messages.death_attacker );
      BarbProcData.messages.death_attacker = NULL;
      str_free( BarbProcData.messages.death_victim );
      BarbProcData.messages.death_victim = NULL;
      str_free( BarbProcData.messages.death_room );
      BarbProcData.messages.death_room = NULL;
      BarbProcData.messages_set = FALSE;
    }
    return TRUE;
  }

  // If hammer isn't wielded or ch isn't the one wielding it.
  if( !OBJ_WORN_POS(obj, WIELD) || obj->loc.wearing != ch )
  {
    return FALSE;
  }

  if( curr_race != GET_RACE(ch) )
  {
    curr_race = GET_RACE(ch);

    // Make it 8d6 for ogres and 2h. rawr.
    if( GET_RACE(ch) == RACE_OGRE )
    {
      // These will only occur once now instead of a zillion times.
      act("$q begins to twist and writhe in $n's hands, his corruption seeping into it!", FALSE, ch, obj, NULL, TO_ROOM);
      act("$q begins to twist and writhe in your hands, your will corrupting it!", FALSE, ch, obj, NULL, TO_CHAR);

      SET_BIT(obj->extra_flags, ITEM_TWOHANDS);
      obj->value[1] = 8;
      obj->value[2] = 6;
    }
    else
    {
      REMOVE_BIT(obj->extra_flags, ITEM_TWOHANDS);
      obj->value[1] = 6;
      obj->value[2] = 5;
    }

    if( GET_RACE(ch) == RACE_OGRE )
    {
      obj->short_description = NULL;
      // TODO: Need to test this; does it solve a mem leak or cause a crash?
      if( IS_SET( obj->str_mask, STRUNG_DESC2) )
      {
        str_free( obj->short_description );
      }
      obj->str_mask |= STRUNG_DESC2;
      obj->short_description = str_dup( "&+bthe mighty warhammer of the ogre chiefs&n" );
    }
    else if( GET_RACE(ch) == RACE_BARBARIAN )
    {
      obj->short_description = NULL;
      obj->str_mask |= STRUNG_DESC2;
      obj->short_description = str_dup( "&+ythe mystical warhammer of the barbarian kings&n" );
    }
    else if( GET_RACE(ch) == RACE_MOUNTAIN || GET_RACE(ch) == RACE_DUERGAR )
    {
      obj->short_description = NULL;
      obj->str_mask |= STRUNG_DESC2;
      obj->short_description = str_dup( "&+Lthe warhammer of the &+yancient dwarven &+Rb&+ra&+Ltt&+Rl&+re&+Lr&+ra&+Rge&+Lr&+rs&n" );
    }
    else if( GET_CLASS(ch, CLASS_BERSERKER) )
    {
      obj->short_description = NULL;
      obj->str_mask |= STRUNG_DESC2;
      obj->short_description = str_dup( "&+rthe great warhammer of the &+RRa&+rGe&+Rlo&+rRd&n" );
    }
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if (isname(arg, "power"))
    {
      curr_time = time(NULL);
      // 1 min timer.
      if( (obj->timer[0] + 60 <= curr_time) && (GET_RACE(ch) == RACE_BARBARIAN) )
      {
        act("You say 'power'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums as the spirits of your ancestors bestow you with power.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'power'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q bestows them with power.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_stone_skin(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_spirit_armor(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_spirit_ward(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_greater_spirit_ward(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      else if( (obj->timer[0] + 60 <= curr_time) && (GET_RACE(ch) == RACE_OGRE) )
      {
        act("You say 'power'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums and you fill with &+RRAGE!", FALSE, ch, obj, obj, TO_CHAR);
        act("$n says 'power'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q bestows them with power.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_stone_skin(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_indomitability(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_hawkvision(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_elephantstrength(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      else if( obj->timer[0] + 60 <= curr_time && (GET_RACE(ch) == RACE_MOUNTAIN || GET_RACE(ch) == RACE_DUERGAR) && !GET_CLASS(ch, CLASS_BERSERKER) )
      {
        act("You say 'power'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q fills you with the power of the &+yancient dwarven battleragers!", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n says 'power'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q bestows them with power.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_stone_skin(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_combat_mind(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);

        if(!affected_by_spell(ch, SPELL_DISPLACEMENT))
        {
          spell_displacement(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        else
        {
          for( af1 = ch->affected; af1; af1 = af1->next )
          {
            if(af1->type == SKILL_DISPLACEMENT)
            {
              af1->duration = 50;
            }
          }
        }
        hammer_berserk_check(ch);
      }
      else if( (obj->timer[0] + 60 <= curr_time) && (GET_CLASS(ch, CLASS_BERSERKER)) )
      {
        act("You say 'power'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q fills you with a &+rSuRGe of &+RBl&+rOOdL&+RusT!!!", FALSE, ch, obj, obj, TO_CHAR);
        act("$n says 'power'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q fills them with a &+rSuRGe of &+RBl&+rOOdL&+RusT!!!", TRUE, ch, obj, NULL, TO_ROOM);
        spell_haste(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_purify_spirit(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        hammer_berserk_check(ch);
      }

      obj->timer[0] = curr_time;
      return TRUE;
    }
  }

  // 1/6 chance.
  if( (cmd == CMD_GOTNUKED) && (GET_RACE(ch) == RACE_MOUNTAIN || GET_RACE(ch) == RACE_DUERGAR) && (!number(0, 5)) )
  {
    hammer_berserk_check(ch);
    if( !(data = (struct proc_data *) arg) )
    {
      return FALSE;
    }
    vict = data->victim;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }
    damage = data->dam;
    type = data->attacktype;
    flags = data->flags;

    if( BarbProcData.wielder != ch ) /* the hammer changed hands. Or just came back from storage */
    {
      if( BarbProcData.hammer )
      {
        if( BarbProcData.hammer != obj )
        {
          wizlog(MINLVLIMMORTAL, "Possibly more than one instance of barbarian warhammer present in game!");
          logit(LOG_DEBUG, "Possibly more than one instance of barbarian warhammer present in game!");
          BarbProcData.hammer = obj;
        }
      }
      else
      {
        BarbProcData.hammer = obj;
      }
      BarbProcData.wielder = ch;
      BarbProcData.damage = 0;
      BarbProcData.flags = 0;
      BarbProcData.attacktype = 0;
    }

    act("&+cA &+Ctranslucent&+c field &+Wflashes&+c briefly around $n&+c as $s $o&+c absorbs your assault...", FALSE, ch, obj, vict, TO_VICT);
    act("&+cA &+Ctranslucent&+c field &+Wflashes&+c briefly around $n&+c as $s $o&+c absorbs $N&+c's assault...", FALSE, ch, obj, vict, TO_NOTVICT);
    act("&+cA &+Ctranslucent&+c field &+Wflashes&+c briefly around you as your $o&+c absorbs $N&+c's assault...", FALSE, ch, obj, vict, TO_CHAR);

    if( BarbProcData.damage )
    {
      act("&+c...and releases a previously stored &+Bmagical energy&+c straight back at &+Wyou!", FALSE, ch, obj, vict, TO_VICT);
      act("&+c...and releases a previously stored &+Bmagical energy&+c straight back at $N!", FALSE, ch, obj, vict, TO_NOTVICT);
      act("&+c...and releases a previously stored &+Bmagical energy&+c straight back at $N!", FALSE, ch, obj, vict, TO_CHAR);

      result = spell_damage(ch, vict, BarbProcData.damage, BarbProcData.attacktype, BarbProcData.flags | SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
                     BarbProcData.messages_set ? &BarbProcData.messages : &messages_deflect);

      // If old meesages are set, release them after they've been used.
      if( BarbProcData.messages_set )
      {
        str_free( BarbProcData.messages.attacker );
        BarbProcData.messages.attacker = NULL;
        str_free( BarbProcData.messages.victim );
        BarbProcData.messages.victim = NULL;
        str_free( BarbProcData.messages.room );
        BarbProcData.messages.room = NULL;
        str_free( BarbProcData.messages.death_attacker );
        BarbProcData.messages.death_attacker = NULL;
        str_free( BarbProcData.messages.death_victim );
        BarbProcData.messages.death_victim = NULL;
        str_free( BarbProcData.messages.death_room );
        BarbProcData.messages.death_room = NULL;

        BarbProcData.messages_set = FALSE;
      }

/*      wizlog(MINLVLIMMORTAL,"setting new values, old dam = %d, new dam = %d", BarbProcData.damage, damage);*/
      if( result == DAM_NONEDEAD )
      {
        attack_back(vict, ch, FALSE);
      }
    }

    BarbProcData.damage = damage;
    BarbProcData.attacktype = type;
    BarbProcData.flags = flags;

    // If spell being cast has good messages, set them for use next time.
    if( data->messages && data->messages->attacker && data->messages->victim
      && data->messages->room && data->messages->death_attacker
      && data->messages->death_victim && data->messages->death_room )
    {
      BarbProcData.messages.attacker = str_dup( data->messages->attacker );
      BarbProcData.messages.victim = str_dup( data->messages->victim );
      BarbProcData.messages.room = str_dup( data->messages->room );
      BarbProcData.messages.death_attacker = str_dup( data->messages->death_attacker );
      BarbProcData.messages.death_victim = str_dup( data->messages->death_victim );
      BarbProcData.messages.death_room = str_dup( data->messages->death_room );

      BarbProcData.messages_set = TRUE;
    }
    return TRUE;
  }

  if( cmd != CMD_MELEE_HIT )
  {
    return FALSE;
  }

  vict = (P_char) arg;

  if( IS_ALIVE(vict) )
  {
    // 4% proc
    if( !number(0, 24) && (GET_RACE(ch) == RACE_BARBARIAN) && CheckMultiProcTiming(ch) )
    {
      act("Your $q &+Bcalls down the lightning of the barbarian kings on $N!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q &+Bcalls down the lightning of the barbarian kings at you!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q &+Bcalls down the lightning of the barbarian kings on $N!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);

      spell_chain_lightning(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);

      // 1/25 * 1/4 = 1/100 chance.
      if( IS_ALIVE(ch) && IS_ALIVE(vict) && !number(0, 3) )
      {
        spell_forked_lightning((int) (GET_LEVEL(ch) - 15), ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      // 1/20 chance * ~100/500 = 1/5 chance == about 1/100 chance.
      if( IS_ALIVE(ch) && !number(0, 19) && GET_C_LUK(ch) < number(0, 500) )
      {
        spell_restoration((int) (GET_LEVEL(ch) - 5), ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
    }
    else if( !number(0, 24) && GET_RACE(ch) == RACE_OGRE && CheckMultiProcTiming(ch) )
    {
      act("Your $q &+bfills you with the &+WPOWER&+b of the ogre chieftains!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q &+bfills them with the &+WPOWER&+b of the ogre chieftains!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q &+bfills them with the &+WPOWER&+b of the ogre chieftains!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);

      hit(ch, vict, obj);
      if( IS_ALIVE(vict) && IS_ALIVE(ch) )
        hit(ch, vict, obj);
      if( IS_ALIVE(vict) && IS_ALIVE(ch) && number(1, 100) <= 30 )
      {
        if( MIN_POS(vict, POS_STANDING + STAT_NORMAL) )
        {
          act("$q &+bsmashes into $N&+b, sending them reeling into the wall!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
          act("$n's $q &+bsmashes into you, sending you flying towards the wall!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
          act("$n's $q &+bsmashes into $N&+b, sending them flying into the wall!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);

          SET_POS(vict, POS_SITTING + GET_STAT(vict));
          stop_fighting(vict);
          if( IS_DESTROYING(vict) )
          {
            stop_destroying(vict);
          }
          CharWait(vict, PULSE_VIOLENCE);
        }
        // if not standing, hit them again.
        else
        {
          hit(ch, vict, obj);
        }
      }
    } // 2% proc (this can stack with the regular berserker proc if wielded by a
      // dwarf/duergar berserker. Only one proc per round allowed, though.
    else if( (!number(0, 49) && (GET_RACE(ch) == RACE_MOUNTAIN || GET_RACE(ch) == RACE_DUERGAR)) && CheckMultiProcTiming(ch) )
    {
      act("Your $q &+Lfills you with the &+RPOWER&+L of the dwarven battleragers!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q &+Lfills them with the &+RPOWER&+L of the dwarven battleragers!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q &+Lfills them with the &+RPOWER&+L of the dwarven battleragers!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);

      hammer_berserk_check(ch);
      // counter = 0;
      // bool ancestor_was_here = false;
      if( BarbProcData.damage && IS_ALIVE(vict) )
      {
        result = spell_damage(ch, vict, BarbProcData.damage, BarbProcData.attacktype, BarbProcData.flags,
                             BarbProcData.messages_set ? &BarbProcData.messages : &messages_deflect);
        BarbProcData.damage = 0;
        BarbProcData.attacktype = 0;
        BarbProcData.flags = 0;

        if( result == DAM_VICTDEAD )
        {
          do_action(ch, 0, CMD_ROAR);
          return DAM_VICTDEAD;
        }
      }
      update_pos(vict);
/* Disabling this for now. It seems that no messages are provided.
      if(!affected_by_spell(vict, SPELL_WITHER) &&
         !affected_by_spell(vict, SPELL_RAY_OF_ENFEEBLEMENT))
            spell_wither(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
*/
      barb_proc_dwarven_ancestor(ch->player.level, ch, vict);
      return true;
    }
// Reduced the spam. Simply having one ancient berserker summoned for an
// extended duration. Old code is below. Ensure to enable counter and
// ancestor_was_here.
      // do 
      // {
        // if (!number(0, counter) && BarbProcData.damage && IS_ALIVE(vict))
        // {
          // result = spell_damage(ch, vict, BarbProcData.damage, BarbProcData.attacktype, BarbProcData.flags,
                               // BarbProcData.messages_set ? &BarbProcData.messages : &messages_deflect);
          // BarbProcData.damage = 0;
          // BarbProcData.attacktype = 0;
          // BarbProcData.flags = 0;

          // counter++;

          // if (result == DAM_CHARDEAD)
          // {
            // do_action(ch, 0, CMD_ROAR);
            // return DAM_CHARDEAD;
          // }
        // }
        // update_pos(vict);

        // if(!ancestor_was_here ||
           // !number(0, counter) &&
           // IS_ALIVE(vict))
        // {
          // barb_proc_dwarven_ancestor(ch->player.level, ch, vict);
          // update_pos(vict);
          // counter++;
          // ancestor_was_here = true;
        // }

        // if(!number(0, counter) &&
           // IS_ALIVE(vict))
        // {
          // if(!affected_by_spell(vict, SPELL_WITHER))
            // spell_wither(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          // counter++;
        // }

      // } while (counter < 2);

      // return true;
    // } // 4%
    else if( !number(0, 24) && GET_CLASS(ch, CLASS_BERSERKER) && GET_CHAR_SKILL(ch, SKILL_BERSERK) > 0 && CheckMultiProcTiming(ch) )
    {
      act("Your $q &+rfills you with BlOOdLUsT, causing you to strike down $N!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q &+rsmashes into you, nearly knocking you off your feet!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q &+rsmashes into $N&+r, causing them to scream in pain!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);

      hammer_berserk_check(ch);
      if( is_char_in_room(vict, ch->in_room) )
      {
        spell_damage(ch, vict, 200, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);

        if( !has_skin_spell(ch) )
        {
          spell_stone_skin(GET_LEVEL(ch), ch, 0, 0, ch, 0);
        }
      }
      // Adding this to prevent perpetual stunning with mauls.
      if( !number(0, 2) && MIN_POS(vict, POS_STANDING + STAT_NORMAL) )
      {
        Stun(vict, ch, (int) (PULSE_VIOLENCE * .5), TRUE);
      }
      if( affected_by_spell(vict, SPELL_STONE_SKIN) )
      {
        act("$n shatters $N's &+Lstone skin!&n", FALSE, ch, obj, vict, TO_ROOM);
        act("$n shatters your &+Lstone skin!&n", FALSE, ch, obj, vict, TO_VICT);
        act("You shatter $N's &+Lstone skin!&n", FALSE, ch, obj, vict, TO_CHAR);
        affect_from_char(vict, SPELL_STONE_SKIN);
      }
      else if( affected_by_spell(vict, SPELL_BIOFEEDBACK) )
      {
        act("$n pierces through $N's &+Gmisty barrier!&n", FALSE, ch, obj, vict, TO_ROOM);
        act("$n pierces through your &+Gmisty barrier!&n", FALSE, ch, obj, vict, TO_VICT);
        act("You pierce through $N's &+Gmisty barrier!&n", FALSE, ch, obj, vict, TO_CHAR);
        affect_from_char(vict, SPELL_BIOFEEDBACK);
      }
      else if( affected_by_spell(vict, SPELL_SHADOW_SHIELD) )
      {
        act("$n obliterates $N's &+Lshadowy shield!&n", FALSE, ch, obj, vict, TO_ROOM);
        act("$n obliterates your &+Lshadowy shield!&n", FALSE, ch, obj, vict, TO_VICT);
        act("You obliterate $N's &+Lshadowy shield!&n", FALSE, ch, obj, vict, TO_CHAR);
        affect_from_char(vict, SPELL_SHADOW_SHIELD);
      }
    }
  }
  return FALSE;
}

void gfstone_event(P_char ch, P_char vict, P_obj obj, void *data)
{
#define SHIVA 1
#define CEREBUS 2
#define CARBUNCLE 3
#define IFRIT 4
#define DOOMTRAIN 5
#define TOTAL_GF 5

  int      gf, count, room, objnum, i, save;
  char     owner[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  P_char   next;
  //P_obj    obj;

  if (sscanf((const char *) data, "%d %d %s %d %d", &gf, &count, owner, &room, &objnum) == 5)
  {
    ch = get_char_room(owner, room);
    if( !IS_ALIVE(ch) )
    {
      ch = get_char(owner);
    }
    if( !IS_ALIVE(ch) || !ch->in_room )
    {
      return;
    }
    obj = get_obj_num(objnum);

    if( (count <= 0) || IS_TRUSTED(ch) )
    {
      room = ch->in_room;
      switch (gf)
      {
      case SHIVA:
        send_to_char("&+WYour summon is complete!\n", ch);
        send_to_room("&+CA huge icicle breaks through the ground, showering ice everywhere!\n", room);
        send_to_room("&+CThe icicle splits apart, revealing Keja, the Queen of Ice.\n", room);
        send_to_room("&+CShe suddenly shatters into a million shards of ice, swirling around like an angry blizzard!\n", room);
        spell_ice_storm(60, ch, 0, SPELL_TYPE_SPELL, NULL, 0);
        if( IS_ALIVE(ch) )
        {
          spell_ice_storm(60, ch, 0, SPELL_TYPE_SPELL, NULL, 0);
        }
        if( IS_ALIVE(ch) )
        {
          spell_ice_storm(60, ch, 0, SPELL_TYPE_SPELL, NULL, 0);
        }
        send_to_room("&+CThe shards dissipate and are gone.\n", room);
        CharWait(ch, PULSE_VIOLENCE);
        obj->value[gf] += 20;
        if( obj->value[gf + 1] < 500 )
        {
          obj->value[gf + 1] += 20;
        }
        if( obj->value[gf] > 1000 )
        {
          obj->value[gf] = 1000;
        }
        break;

      case IFRIT:
        send_to_char("&+WYour summon is complete!\n", ch);
        send_to_room("&+RThe ground splits open and Jera, the hell goddess, floats out of the chasm.\n", room);
        if( OUTSIDE(ch) )
        {
          send_to_room("&+RJera rises high into the air, spewing curses about whiners.\n", room);
          send_to_room("&+RBOOM!! Jera throws meteors at the whiners!\n", room);
          for( vict = world[room].people; vict; vict = next )
          {
            next = vict->next_in_room;
            if( (vict != ch) && !(ch->group && (ch->group == vict->group)) && number(0, 1) )
            {
              GET_HIT(vict) -= 50;
              update_pos(vict);
            }
          }
        }
        else
        {
          send_to_room("&+RJera roars 'Who summons me indoors? Don't you know I'm claustrophobic?!'\n", room);
          send_to_room("&+RBOOM!! Jera explodes into jets of fire and the ground begins to shake!\n", room);
          spell_earthquake(60, ch, 0, SPELL_TYPE_STAFF, NULL, 0);
          if( IS_ALIVE(ch) )
          {
            spell_firestorm(60, ch, 0, SPELL_TYPE_STAFF, NULL, 0);
          }
          send_to_room("&+RThe flames form back into Jera.\n", room);
        }
        send_to_room("&+RJera slowly sinks back into the ground and disappears.\n", room);
        if( IS_ALIVE(ch) )
        {
          CharWait(ch, PULSE_VIOLENCE * 2);
        }
        obj->value[gf] += 30;
        if( gf <= TOTAL_GF )
        {
          if( obj->value[gf + 1] < 500 )
          {
            obj->value[gf + 1] += 40;
          }
          for( i = 1; i <= TOTAL_GF; i++ )
          {
            if( obj->value[i] > 200 )
            {
              obj->value[i] -= 20;
            }
          }
        }
        if( obj->value[gf] > 1000 )
        {
          obj->value[gf] = 1000;
        }
        break;

      case DOOMTRAIN:
        send_to_char("&+WYour summon is complete!\n", ch);
        send_to_room("&+rCython&+L steps out of the shadows.\n", room);
        send_to_room("Cython gets a &+RBIG RED BUTTON from his &+La portable hole.\n", room);
        send_to_room("Cython presses a &+RBIG RED BUTTON.\n", room);
        send_to_room("&+Y  __   _   _  _  _\n", room);
        send_to_room("&+Y  |_) | | | | |\\/| |||\n", room);
        send_to_room("&+Y  |_) |_| |_| |  || ...\n", room);
        for( vict = world[room].people; vict; vict = next )
        {
          next = vict->next_in_room;
          if( (vict != ch) && !(ch->group && (ch->group == vict->group)) && !number(0, 4) )
          {
            save = vict->specials.apply_saving_throw[SAVING_SPELL];
            vict->specials.apply_saving_throw[SAVING_SPELL] = 20;
            spell_wither(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            spell_slow(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            spell_poison(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            spell_blindness(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            vict->specials.apply_saving_throw[SAVING_SPELL] = save;
            spell_curse(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            spell_sleep(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          }
        }
        send_to_room("&+rCython&+L disappears into the shadows.\n", room);
        CharWait(ch, PULSE_VIOLENCE * 3);
        obj->value[gf] += 30;
        if (gf <= TOTAL_GF)
        {
          if (obj->value[gf + 1] < 500)
            obj->value[gf + 1] += 40;

          for (i = 1; i <= TOTAL_GF; i++)
          {
            if (obj->value[i] > 200)
              obj->value[i] -= 20;
          }
        }
        if (obj->value[gf] > 1000)
        {
          obj->value[gf] = 1000;
        }
        if( (obj->value[CEREBUS] < 500) || (obj->value[CARBUNCLE] < 500) )
        {
          send_to_char("&+rCython, seeing that Raxxel and Dakta hate you, gets pissed off.\n", ch);
          obj->value[DOOMTRAIN] = 200;
        }
        break;

      case CEREBUS:
        send_to_char("&+WYour summon is complete!\n", ch);
        send_to_room("&+LDakta, the lord of Lust and perversion, appears before you.\n", room);
        send_to_room("Dakta says 'I can make you faster, stronger, and longer lasting for\n", room);
        send_to_room("when you're mine!'\n", room);

        for( vict = world[room].people; vict; vict = next )
        {
          next = vict->next_in_room;
          spell_haste(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          spell_pantherspeed(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          spell_bless(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          spell_lionrage(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        }
        send_to_room("&+LYou feel really REALLY wierd.\n", room);

        CharWait(ch, PULSE_VIOLENCE);

        obj->value[gf] += 30;
        if (gf <= TOTAL_GF)
        {
          if (obj->value[gf + 1] < 500)
          {
            obj->value[gf + 1] += 30;
          }
          for (i = 2; i <= TOTAL_GF; i++)
          {
            if (obj->value[i] > 200)
            {
              obj->value[i] -= 10;
            }
          }
        }
        if (obj->value[gf] > 1000)
        {
          obj->value[gf] = 1000;
        }
        break;

      case CARBUNCLE:
        send_to_char("&+WYour summon is complete!\n", ch);
        send_to_room("&+cA small hole appears in the ground and a small rainbow colored creature\n", room);
        send_to_room("&+cwith a ruby on its forhead pops out!\n", room);
        send_to_room("&+cRaxxel, the imp of mischief is here!\n", room);
        send_to_room("Raxxel says 'Don't let anyone know I'm doing this!'\n", room);
        cast_as_area(ch, SPELL_ARMOR, 51, NULL);
        cast_as_area(ch, SPELL_GLOBE, 51, NULL);
        cast_as_area(ch, SPELL_SOULSHIELD, 51, NULL);
        send_to_room("&+cRaxxel pops back into the hole, and it disappears.\n", room);

        CharWait(ch, PULSE_VIOLENCE);

        obj->value[gf] += 30;
        if( gf <= TOTAL_GF )
        {
          if( obj->value[gf + 1] < 500 )
          {
            obj->value[gf + 1] += 30;
          }
          for( i = 2; i <= TOTAL_GF; i++ )
          {
            if (obj->value[i] > 200)
            {
              obj->value[i] -= 10;
            }
          }
        }
        if (obj->value[gf] > 1000)
        {
          obj->value[gf] = 1000;
        }
        break;

      default:
        send_to_char("ERROR IN EVENT FUNCTION, DIE!!\n", ch);
        break;
      }
      obj->value[0] = 0;
      return;
    }
    send_to_char("Summoning: ", ch);
    for (i = 1; i <= count; i++)
    {
      send_to_char("*", ch);
    }
    send_to_char("\n", ch);
    count -= 1;
    sprintf(buf, "%d %d %s %d %d", gf, count, owner, room, obj->R_num);
    add_event(gfstone_event, 4, NULL, NULL, NULL, 0, buf, strlen(buf)+1);
    //AddEvent(EVENT_SPECIAL, 4, TRUE, gfstone_event, buf);
    return;
  }
  return;
}

int gfstone(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    curr_time, gf, count, i;
  char   params[MAX_STRING_LENGTH], owner[MAX_STRING_LENGTH],
         buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch)
    || (!OBJ_WORN_POS(obj, HOLD) && !OBJ_WORN_POS(obj, SECONDARY_WEAPON)) )
  {
    return FALSE;
  }

  gf = -1;

  sscanf(obj->name, "%s %s %s", buf, buf2, owner);

  obj->str_mask |= STRUNG_KEYS;

  if( strcmp(GET_NAME(ch), owner) != 0 )
  {
    sprintf(buf3, "%s %s %s", buf, buf2, GET_NAME(ch));
    obj->name = str_dup(buf3);
    for( i = 1; i <= TOTAL_GF; i++ )
    {
      obj->value[i] = 0;
    }
    obj->value[SHIVA] = 500;
    obj->value[0] = 0;
    return FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "stone") )
    {
      curr_time = time(NULL);
      // Every 1 min.
      if( obj->timer[0] + 60 <= curr_time )
      {
        act("You say 'stone'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'stone'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_stone_skin(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }

  if (arg && (cmd == CMD_RUB))
  {
    if( isname(arg, "keja") )
    {
      gf = SHIVA;
      act("&+CYou start summoning Keja.&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+C$n starts summoning Keja.&n", TRUE, ch, obj, NULL, TO_ROOM);
    }

    if( isname(arg, "jera") )
    {
      gf = IFRIT;
      if( obj->value[gf] < 500 )
      {
        act("&+RJera ignores you.&n", FALSE, ch, 0, 0, TO_CHAR);
        return FALSE;
      }
      act("&+RYou start summoning Jera.&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+R$n starts summoning Jera.&n", TRUE, ch, obj, NULL, TO_ROOM);
    }

    if( isname(arg, "cython") )
    {
      gf = DOOMTRAIN;
      if( obj->value[gf] < 500)
      {
        act("&+rCython laughs at you.&n", FALSE, ch, 0, 0, TO_CHAR);
        return FALSE;
      }
      act("&+rYou start summoning Cython.&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+r$n starts summoning Cython.&n", TRUE, ch, obj, NULL, TO_ROOM);
    }

    if( isname(arg, "dakta") )
    {
      gf = CEREBUS;
      if (obj->value[gf] < 500)
      {
        act("&+LDakta ignores your plea.&n", FALSE, ch, 0, 0, TO_CHAR);
        return FALSE;
      }
      act("&+LYou start summoning Dakta.&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+L$n starts summoning Dakta.&n", TRUE, ch, obj, NULL, TO_ROOM);
    }

    if( isname(arg, "raxxel") )
    {
      gf = CARBUNCLE;
      if (obj->value[gf] < 500)
      {
        act("&+cRaxxel fails to comply.&n", FALSE, ch, 0, 0, TO_CHAR);
        return FALSE;
      }
      act("&+cYou start summoning Raxxel.", FALSE, ch, 0, 0, TO_CHAR);
      act("&+c$n starts summoning Raxxel.", TRUE, ch, obj, NULL, TO_ROOM);
    }

    if( isname(arg, "godmode") && IS_TRUSTED(ch) )
    {
      obj->value[0] = 0;
      for (i = 1; i <= TOTAL_GF; i++)
      {
        obj->value[i] = 1000;
      }
      send_to_char("You have been givin divine compatability!\n", ch);
    }
  }

  if( arg && (cmd == CMD_LOOK) )
  {
    if( isname(arg, "stone god") )
    {
      sprintf(buf, "&+WCompatability with: %s\n", owner);
      send_to_char(buf, ch);

      for( i = 1; i <= TOTAL_GF; i++ )
      {
        if( (obj->value[i] >= 500) || (i == SHIVA) )
        {
          switch (i)
          {
          case SHIVA:
            sprintf(buf, "&+CKeja");
            break;
          case IFRIT:
            sprintf(buf, "&+RJera");
            break;
          case DOOMTRAIN:
            sprintf(buf, "&+rCython");
            break;
          case CEREBUS:
            sprintf(buf, "&+LDakta");
            break;
          case CARBUNCLE:
            sprintf(buf, "&+cRaxxel");
            break;
          default:
            sprintf(buf, "&+WERROR");
            break;
          }
          sprintf(buf2, "%-17s: &+W%-5d\n", buf, obj->value[i]);
          send_to_char(buf2, ch);
        }
      }
      return TRUE;
    }
  }

  if( gf > -1 )
  {
    if( obj->value[0] > 0 )
    {
      send_to_char("&+WYou are already summoning!\n", ch);
      return TRUE;
    }
    count = (int) (obj->value[gf] / 100);
    count = 11 - count;
    sprintf(buf, "%d %d %s %d %d", gf, count, owner, ch->in_room, obj->R_num);

    add_event(gfstone_event, 4, NULL, NULL, NULL, 0, buf, strlen(buf)+1);
    //AddEvent(EVENT_SPECIAL, 4, TRUE, gfstone_event, buf);
    CharWait(ch, PULSE_VIOLENCE);
    obj->value[0] = 1;
    return TRUE;
  }
  return FALSE;
}

/***********************************************************************/
/* testing out code for tsunami artifact.  should not be included yet. */
/***********************************************************************/

void event_tsunamiwave(P_char ch, P_char victim, P_obj obj, void *data)
{
  int room = *((int*)data);
  bool success;
  for (victim = world[room].people; victim; victim = victim->next_in_room)
  {
    success = true;
    if (should_area_hit(ch, victim)) {
      if (IS_WATER_ROOM(room) && number(0,2)) {
        act("&+BA HUGE wall of water crashes into the room pushing $n &+Bbelow"
            " the surface.", FALSE, victim, 0, 0, TO_ROOM);
        act("&+BA HUGE wave sweeps into the room knocking you over pushing you "
            "below the surface.", FALSE, victim, 0, 0, TO_CHAR);
      } else if (HAS_FOOTING(victim) && number(0,1)) {
        act("&+LThe ground splits apart as a &+Bmass of water&+L erupts "
            "from below knocking $n off $s feet.", FALSE, victim, 0, 0, TO_ROOM);
        act("&+LThe ground splits apart as a &+Bmass of water&+L erupts "
            "from below knocking you off your feet.", FALSE, victim, 0, 0, TO_CHAR);
      } else if (!HAS_FOOTING(victim) && !IS_WATER_ROOM(room) && !number(0,2)) {
        act("&+CStorm winds sweep through the room tearing at $n &+Cforcing "
            "$m to $s knees.", FALSE, victim, 0, 0, TO_ROOM);
        act("&+CStorm winds slam into you forcing you kneel or be blown off "
            "your feet.", FALSE, victim, 0, 0, TO_CHAR);
      } else {
        success = false;
      }
      if (success)
        SET_POS(victim, POS_PRONE + GET_STAT(victim));
    }
  }
}


int SeaKingdom_Tsunami(P_obj obj, P_char ch, int cmd, char *arg)
{

  int      dam = cmd / 1000, curr_time;
  struct affected_type *af;
  P_char   vict;

  curr_time = time(NULL);
  /* check for periodic event calls */

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!ch && cmd == CMD_PERIODIC)
  {
    hummer(obj);
    return TRUE;
  }

  if (!ch)
    return (FALSE);

  if (!OBJ_WORN_POS(obj, WIELD))
    return (FALSE);
  /*
     if (curr_time >= obj->timer[0] + 15)
     {
     act("$q &+Ysuddenly glows brightly!", FALSE, ch, obj, obj, TO_CHAR);
     act("$n's $q glows brightly!", TRUE, ch, obj, obj, TO_ROOM);
     cast_mass_heal(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
     cast_mass_heal(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
     obj->timer[0] = curr_time;
     }
   */

  if (arg && (isname(arg, "trident") || isname(arg, "tsunami")))
  {
    if (cmd == CMD_TAP)
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 300 <= curr_time)
      {

        act("You tap $q on the ground three times.", FALSE, ch, obj, vict,
            TO_CHAR);
        act("$n taps $q on the ground three times.", FALSE, ch, obj, vict,
            TO_ROOM);

        send_to_room
          ("\n&+BT&+Csun&+Wami&+C, t&+Bhe &+btri&+Bden&+Ct o&+Wf s&+Ctor&+Bm s&+budd&+Benl&+Cy e&+Wrup&+Cts &+Bint&+bo a&+B gu&+Cshi&+Wng &+Cflo&+Bw o&+bf p&+Bure&+C wh&+Wite&+C,\n",
           ch->in_room);
        send_to_room
          ("&+Bfro&+Cthi&+Wng &+Cwat&+Ber,&+b co&+Bale&+Csin&+Wg i&+Cnto&+B a &+bnea&+Brly&+C pe&+Wrfe&+Cct &+Bfor&+bm r&+Bese&+Cmbl&+Wing&+C Po&+Bsei&+bdon&+B,  &+Crul&+Wer\n",
           ch->in_room);
        send_to_room
          ("&+bof&+B th&+Ce S&+Wea &+CKin&+Bgdo&+bm. &+B  H&+Cis &+Wmou&+Cth &+Bope&+bn i&+Bn a&+C si&+Wlen&+Ct s&+Bcre&+bam &+Bof &+Cpro&+Wtes&+Ct, &+Bthe&+b sh&+Bape&+C of\n",
           ch->in_room);
        send_to_room
          ("&+Bt&+bhe &+Bfal&+Clen&+W Go&+Cd e&+Bxpl&+bode&+Bs i&+Cnto&+W je&+Ct s&+Btre&+bams&+B of&+C vi&+Wtal&+Cizi&+Bng &+bwat&+Ber,&+C sh&+Woot&+Cing&+B ab&+broa&+Bd i&+Cn\n",
           ch->in_room);
        send_to_room
          ("&+Ball&+b di&+Brec&+Ctio&+Wns &+Cand&+B en&+bgul&+Bfin&+Cg y&+Wour&+C al&+Blie&+bs w&+Bith&+C  t&+Whe &+Clon&+Bg f&+borg&+Bott&+Cen &+Wpow&+Cers&+B of&+b ol&+Bd.\n\n",
           ch->in_room);

        for (vict = world[ch->in_room].people; vict;
             vict = vict->next_in_room)
        {
          if (!affected_by_spell(vict, SPELL_VITALITY))
          {
            if ((ch->group && (ch->group == vict->group)))
            {
              act
                ("&+W$N&+L is &+Bvit&+Cal&+Wiz&+Ced by &+WTsu&+Bna&+bmi's&+W power&+W!",
                 FALSE, ch, obj, vict, TO_ROOM);
              spell_vitality(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            }
            else if (vict == ch)
            {
              act
                ("&+W$n&+L is &+Bvit&+Cal&+Wiz&+Ced by &+WTsu&+Bna&+bmi's&+W power&+W!",
                 FALSE, ch, obj, vict, TO_ROOM);
              spell_vitality(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            }
          }
          else
          {
            struct affected_type *af1;
            for (af1 = vict->affected; af1; af1 = af1->next)
            {
              if(af1->type == SPELL_VITALITY)
              {
                 af1->duration = 15;
              }
            }
            if (vict == ch)
            {
              act("&+bThe &+Bwater&+b from $q&+b flows around &+Wyou&+b, refreshing your &+Wvitality&+b!",
                 TRUE, ch, obj, NULL, TO_CHAR);
            }
            else
            {
              act("&+bThe &+Bwater &+bfrom $q&+b flows around &+W$N&+b, refreshing your &+Wvitality&+b!",
                 TRUE, ch, obj, vict, TO_CHAR);
            }
            
            act("&+bThe &+Bwater &+bfrom $q &+bflows around &+W$N&+b, refreshing your &+Wvitality&+b!",
               TRUE, ch, obj, vict, TO_ROOM);
          }
        }
        send_to_room
          ("\n&+WTsu&+Bna&+bmi&+L,&+b the trident of &+cstorms &+b vibrates &+Bviolently&+b and suddenly becomes &+Lstill.\n",
           ch->in_room);
        obj->timer[0] = curr_time;
        return TRUE;
      }

      else
      {
        act
          ("You &+Ltap and tap and &+Wtap but no &+Bwa&+bter's comin outta &+Wthis &+Bs&+Ch&+Wa&+Cf&+Bt.",
           TRUE, ch, obj, NULL, TO_CHAR);
        return TRUE;
      }
    } else if (obj->timer[1] + 500 > time(NULL) && !IS_TRUSTED(ch)) {
      return FALSE;
    } else if (cmd == CMD_THRUST && HAS_FOOTING(ch) && !IS_WATER_ROOM(ch->in_room)) {
      act("You thrust $q deep into the ground invoking the name of Poseidon, King of the Sea.",
          FALSE, ch, obj, 0, TO_CHAR);
      act("With great strength $n thrusts $q deep into the ground as he invokes the name of Poseidon.",
          FALSE, ch, obj, 0, TO_ROOM);
      add_event(event_tsunamiwave, WAIT_SEC, ch, 0, 0, 0, &(ch->in_room), sizeof(ch->in_room));
      obj->timer[1] = time(NULL);
      return TRUE;
    } else if (cmd == CMD_RAISE && IS_WATER_ROOM(ch->in_room)) {
      act("You raise your $q skyward calling for the aid of Poseidon, ruler of the Sea.",
          FALSE, ch, obj, 0, TO_CHAR);
      act("$n raises $s $q skyward while shouting a prayer to Poseidon, ruler of the Sea.",
          FALSE, ch, obj, 0, TO_ROOM);
      add_event(event_tsunamiwave, WAIT_SEC, ch, 0, 0, 0, &(ch->in_room), sizeof(ch->in_room));
      obj->timer[1] = time(NULL);
      return TRUE;
    } else if (cmd == CMD_RAISE && !IS_WATER_ROOM(ch->in_room) &&
        !HAS_FOOTING(ch)) {
      act("You raise your $q skyward calling for the aid of Poseidon, ruler of the Sea.",
          FALSE, ch, obj, 0, TO_CHAR);
      act("$n raises $s $q skyward while shouting a prayer to Poseidon, ruler of the Sea.",
          FALSE, ch, obj, 0, TO_ROOM);
      add_event(event_tsunamiwave, WAIT_SEC, ch, 0, 0, 0, &(ch->in_room), sizeof(ch->in_room));
      obj->timer[1] = time(NULL);
      return TRUE;
    }
  }
  return FALSE;
}

int sevenoaks_longsword(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int i, room;
  struct damage_messages messages = {
    "Your $q &+rhums briefly as it unleashes a &+Cr&+Wai&+Cn of sharp &+Wic&+Ci&+Bc&+Cl&+Wes at $N!",
    "$n's $q &+rhums briefly as it unleashes a &+Cr&+Wai&+Cn of sharp &+Wic&+Ci&+Bc&+Cl&+Wes at you!",
    "$n's $q &+rhums briefly as it unleashes a &+Cr&+Wai&+Cn of sharp &+Wic&+Ci&+Bc&+Cl&+Wes at $N!",
    "", "", "", 0, obj};

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  room = ch->in_room;

  if( !IS_ALIVE(vict) || !room )
  {
    return FALSE;
  }

  // 1/33 chance.
  /* if(CheckMultiProcTiming(ch) && */
  if(!number(0, 32))
  {
    act("&+LYour $q blurs as it strikes $N.", FALSE, ch, obj, vict, TO_CHAR);
    act("&+L$n's $q blurs as it strikes you.", FALSE, ch, obj, vict, TO_VICT);
    act("&+L$n's $q blurs as it strikes $N.", FALSE, ch, obj, vict, TO_NOTVICT);

    for( i = 0; i < 2 && IS_ALIVE(ch) && IS_ALIVE(vict); i++ )
    {
      hit(ch, vict, obj);
    }
    return TRUE;
  }

  // 1/33 chance.
  if( !number(0, 32)
    && spell_damage(ch, vict, dice(10, 24), SPLDAM_COLD, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT | RAWDAM_NOKILL, &messages) != DAM_NONEDEAD )
  {
    return TRUE;
  }

  return FALSE;
}

int dagger_of_wind(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int i, room;
  struct damage_messages messages = {
    "Your $q &+ccalls forth the &+Cessen&+Wce of the Wind &+cas you strike at $N...",
    "$n's $q &+ccalls forth the &+Cessen&+Wce of the Wind &+cas they strike at you...",
    "$n's $q &+ccalls forth the &+Cessen&+Wce of the Wind &+cas they strike at $N...",
    "", "", "", 0, obj};

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  room = ch->in_room;

  if( !IS_ALIVE(vict) || !room )
  {
    return FALSE;
  }

  // 1/25 chance.
  /* if(CheckMultiProcTiming(ch) && */
  if(!number(0, 24))
  {
    if( IS_FIGHTING(ch) )
    {
      stop_fighting(ch);
    }
    for( vict = world[ch->in_room].people; vict; vict = vict->next_in_room )
    {
      if( vict->specials.fighting == ch )
      {
        act("&+C...suddenly, $n &+Cis engulfed in a sw&+cwi&+Wrl&+cin&+Cg &+Ltornado &+Cof &+Wwind... sweeping them from &+rbattle&+c!", FALSE, ch, 0, vict, TO_ROOM);
        act("&+C...suddenly, $n &+Cis engulfed in a sw&+cwi&+Wrl&+cin&+Cg &+Ltornado &+Cof &+Wwind... sweeping them from &+rbattle&+c!", FALSE, ch, 0, vict, TO_VICT);
        act("&+C...suddenly, you &+Care engulfed in a sw&+cwi&+Wrl&+cin&+Cg &+Ltornado &+Cof &+Wwind... and swept from &+rbattle&+c!", FALSE, ch, 0, vict, TO_CHAR);
        stop_fighting(vict);
      }
    }
    return TRUE;
  }
  return FALSE;
}

int sevenoaks_mace(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   vict;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!dam)                     /*
                                   if dam != 0, we have been called when
                                   weapons hits someone
                                 */
    return (FALSE);

  if (!ch)
    return (FALSE);

  if (!OBJ_WORN_POS(obj, WIELD))
    return (FALSE);

  vict = (P_char) arg;

  if (OBJ_WORN_BY(obj, ch) && vict)
  {
    if (!number(0, 30))
    {
      act("Your $q &+Bvibrates then becomes very &+Cchilly!", FALSE,
          obj->loc.wearing, obj, vict, TO_CHAR);
      act
        ("$n's $q &+Bvibrates violently, directing &+Ccold&+B energy at you!",
         FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act
        ("$n's $q &+Bvibrates violently then explodes in &+Cchilly &+Bforce!",
         FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
      spell_ice_storm(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
  }
  return (FALSE);
}

int tendrils(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  char     w_align, e_pos;
  int      t_align = -99999;
  P_char   t_vict;
  P_char   vict = NULL;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!obj)
    return FALSE;
/*
  if (cmd == CMD_PERIODIC)
  {
    if (!number(0, 9))
    {
      hummer(obj);
    }

    if (OBJ_WORN(obj))
      ch = obj->loc.wearing;
    else if (OBJ_CARRIED(obj))
      ch = obj->loc.carrying;
    else
      return FALSE;
  
    w_align = (RACE_EVIL(ch) && !(GET_RACE(ch) == RACE_PHANTOM)) ? 0 :
      RACE_PUNDEAD(ch) ? 0 : GET_CLASS(ch, CLASS_MONK) ? 3 : 2;
    if (w_align == 2)
      return (FALSE);

    if (w_align != 3)
    {
      LOOP_THRU_PEOPLE(t_vict, ch)
      {
        int      ta = GET_ALIGNMENT(t_vict);

        if (GET_CLASS(t_vict, CLASS_MONK))
          ta += (GET_LEVEL(t_vict) * 100);
        if ((t_vict != ch) && (ta > t_align))
        {
          t_align = ta;
          vict = t_vict;
        }
      }
      if (OBJ_WORN(obj))
      {
        e_pos =
          ((obj->loc.wearing->equipment[WEAR_HANDS] ==
            obj) ? WEAR_HANDS : (obj->loc.wearing->equipment[HOLD] ==
                                 obj) ? HOLD : 0);
        obj_to_char(unequip_char(ch, e_pos), ch);
      }
      obj_from_char(obj);
    }

    switch (w_align)
    {
    case 0:
      act("$p screams in outrage at your evil touch!",
          FALSE, ch, obj, 0, TO_CHAR);
      act("$p screams in outrage at $n's evil touch!",
          FALSE, ch, obj, 0, TO_ROOM);
      if (t_align < 351)
      {
        act
          ("$p &=LCshimmers, blasts you with power and vanishes from your hand!",
           FALSE, ch, obj, 0, TO_CHAR);
        act("$p &=LCshimmers and blasts $n before vanishing!", FALSE, ch,
            obj, 0, TO_ROOM);
        obj_to_room(obj, number(zone_table[0].real_top + 1, top_of_world));
      }
      else
      {
        act("$p &=LCshimmers, blasts you with power and leaps to $N!",
            FALSE, ch, obj, vict, TO_CHAR);
        act("$p &=LCshimmers and blasts $n as it leaps to $N!",
            FALSE, ch, obj, vict, TO_NOTVICT);
        act("$p &=LCshimmers and blasts $n as it leaps to you!",
            FALSE, ch, obj, vict, TO_VICT);
        obj_to_char(obj, vict);
      }
      GET_HIT(ch) -= MAX(1, GET_HIT(ch) - 200);
      return TRUE;

    case 1:
      send_to_char("You suddenly feel lighter!\n", ch);
      if (t_align < 351)
      {
        obj_to_room(obj, ch->in_room);
      }
      else
      {
        obj_to_char(obj, vict);
        act("$p suddenly appears in your inventory!",
            FALSE, ch, obj, vict, TO_VICT);
      }
      return TRUE;
    }

  }
*/
  if (cmd != CMD_GOTHIT || number(0, 7))
    return FALSE;


  data = (struct proc_data *) arg;
  vict = data->victim;
  if (GET_STAT(vict) == STAT_DEAD)
    return FALSE;

  act("$n sidesteps $N's lunge only to slam $s face with an elbow!", TRUE,
      ch, 0, vict, TO_NOTVICT);
  act
    ("$n completely sidesteps your lunge only to slam $s elbow into your face!",
     TRUE, ch, 0, vict, TO_VICT);
  act
    ("You completely sidestep $N's lunge, only to slam your elbow into $s face!",
     TRUE, ch, 0, vict, TO_CHAR);
/*
  if ((damage(ch, vict, dice(5, 20), TYPE_UNDEFINED) != DAM_NONEDEAD))
  {
    return TRUE;
  }
*/
  if(!number(0, 7))
  {
    act("Before $N can recover $n grabs $M by the throat and utters a single &+Wword.\n"
        "$N spasms in &+Rpain as tiny &+Bbolts of &+Blight surge through $S body!", FALSE, ch, 0, vict, TO_NOTVICT);
    act("Before your can recover $n grabs your throat and utters a single &+Wword.\n"
     "You spasm in &+Rpain as tiny &+Bbolts of &+Blight surge through your body!", TRUE, ch, 0, vict, TO_VICT);
    act("Before $N can recover you grab $M by the throat and utter a single &+Wword.\n"
        "$N spasms in &+Rpain as tiny &+Bbolts of &+Blight surge through $S body!", FALSE, ch, 0, vict, TO_CHAR);
    spell_stone_skin(60, ch, 0, 0, ch, 0);
    Stun(vict, ch, PULSE_VIOLENCE * 2, TRUE);
    damage(ch, vict, dice(10, 45), TYPE_UNDEFINED);
  }
  return FALSE;
}


int Einjar(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char   vict;
  int      curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if (cmd == CMD_SAY && arg && isname(arg, "Einjar"))
  {
    curr_time = time(NULL);
    // 3 min timer.
    if( obj->timer[0] + 180 <= curr_time )
    {
      act("You tap three times on $q.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n taps three times on $q.", FALSE, ch, obj, 0, TO_ROOM);
      for( vict = world[ch->in_room].people; vict; vict = vict->next_in_room )
      {
        if( (ch->group && (ch->group == vict->group)) || ch == vict )
        {
          spell_heal(50, ch, 0, 0, vict, 0);
        }
      }
      obj->timer[0] = curr_time;
      return TRUE;
    }
  }
  // Shield block code was updated and hits far more frequently.
  // Einjar proc rate reduced from 33% to 20%.
  if( cmd != CMD_GOTHIT || number(0, 4) || !(data = (struct proc_data *) arg)  )
  {
    return FALSE;
  }
  vict = data->victim;
  if( !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  act("You block $N's vicious attack.", FALSE, ch, 0, vict, TO_CHAR | ACT_NOTTERSE);
  act("$n blocks your futile attack.", FALSE, ch, 0, vict, TO_VICT | ACT_NOTTERSE);
  act("$n blocks $N's attack.", FALSE, ch, 0, vict, TO_NOTVICT | ACT_NOTTERSE);

  // 1/15 * 1/5 == 1/75 chance.
  if( !number(0, 14) && GET_POS(vict) == POS_STANDING && !IS_TRUSTED(ch) )
  {
    act("Your bash knocks $N to the ground!", FALSE, ch, 0, vict, TO_CHAR);
    act("You are knocked to the ground by $n's mighty bash!", FALSE, ch, 0,
        vict, TO_VICT);
    act("$N is knocked to the ground by $n's mighty bash!", FALSE, ch, 0,
        vict, TO_NOTVICT);
    SET_POS(vict, POS_SITTING + GET_STAT(vict));
    CharWait(vict, PULSE_VIOLENCE * 2);
  }

  return TRUE;
}


