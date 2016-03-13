/*
   ***************************************************************************
   *  File: specs.undermountain.c                             Part of Duris  *
   *  Usage: Undermountain, parts I, II, III, and Skullport                    *
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
#include "structs.h"
#include "utils.h"
#include "specs.prototypes.h"
#include "weather.h"
#include "map.h"
#include "reavers.h"
#include "damage.h"

/*
   external variables 
 */

extern P_char char_in_room(int);
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const int top_of_world;
extern char *coin_names[];
extern char *command[];
extern const char *dirs[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int planes_room_num[];
extern int racial_base[];
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

/*
static void hummer(P_obj obj);

void
hummer(P_obj obj)
{
  if (!obj || number(0,9))
    return;
  if (OBJ_WORN(obj) || OBJ_CARRIED(obj)) {
    act("&+LA faint hum can be heard from&N $p&+L carried by $n&n.",
        FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
    act("&+LA faint hum can be heard from&N $p&+L you are carrying.",
        FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
  } else if (OBJ_ROOM(obj)) {
    act("&+LA strange humming sound comes from&N $p&+L.",
        0, world[obj->loc.room].people, obj, 0, TO_ROOM);
    act("&+LA strange humming sound comes from&N $p&+L.",
        0, world[obj->loc.room].people, obj, 0, TO_ROOM);

  }
  return;
}

*/

/***** Objects *****/
int blade_of_paladins(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict, tmp_vict;
  char     e_pos;
  int      in_battle, bad_owner;

  vict = (P_char) arg;
  in_battle = cmd / 1000;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !obj)
    return FALSE;

/** Who's in the room? Careful here, or it will spam player **/
  if (!in_battle && OBJ_WORN(obj))
  {
    LOOP_THRU_PEOPLE(tmp_vict, ch)
    {
      if (IS_UNDEAD(tmp_vict) && (tmp_vict != ch))
        act("Your blade &+Wglows brightly&N as it senses undead nearby.\r\n",
            FALSE, ch, obj, 0, TO_CHAR);
    }
  }

/** Check alignment and class first off, burn em, and let em keep it **/

  if ((!GET_CLASS(ch, CLASS_PALADIN) || (GET_ALIGNMENT(ch) != 1000)) &&
      OBJ_WORN_BY(obj, ch))
  {
    act
      ("&+BThe blade briefly flashes with &N&+Yintense light&N&+B, burning your hand severely!&N",
       TRUE, ch, obj, vict, TO_CHAR);
    act("$N screams in pain as $S blade burns into $S skin!", FALSE, ch, obj,
        0, TO_ROOM);
    GET_HIT(ch) -= 5;
    update_pos(ch);
    bad_owner = TRUE;
  }
  else
    bad_owner = FALSE;

  if (cmd < 1000)
    return FALSE;

  if (!OBJ_WORN(obj))
    return FALSE;

  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);

  /*
     must be wielded 
   */
  if (!e_pos)
    return (FALSE);

/** Are we fighting undead? Cool, lets rock on them! **/
  if (IS_UNDEAD(vict) && !bad_owner)
  {
    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 4;
    obj->affected[1].location = APPLY_DAMROLL;
    obj->affected[1].modifier = 4;

/** If we scored a _really_ nice hit, let's do some spell shit **/

    if (!number(0, 30))
    {
      act
        ("&+WYour sword flares up upon hitting the vile undead creature!&N\r\n",
         FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      spell_holy_word(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
  }
  else
  {                             /*
                                   Not undead, but fighting still. 
                                 */
    obj->affected[0].location = APPLY_NONE;
    obj->affected[1].location = APPLY_NONE;
  }
  return TRUE;
}

int iron_flindbar(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  char     e_pos;
  int      vict_equip;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || IS_PC(ch) || IS_PC_PET(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  // If it must be wielded, use this 
  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);
  if( !e_pos || !IS_ALIVE(vict) || !IS_FIGHTING(ch) || ch->in_room != vict->in_room )
  {
    return FALSE;
  }

  // 1/30 chance.
  if( !number(0, 29) )
  {
    if (vict->equipment[WIELD] || vict->equipment[SECONDARY_WEAPON])
    {
      act("Your &+Lflindbar&N vibrates upon hitting $N's weapon, knocking it from $S grip, and sending $M reeling!", FALSE, ch, obj, vict, TO_CHAR);
      obj = vict->equipment[WIELD] ? vict->equipment[WIELD] : vict->equipment[SECONDARY_WEAPON];
      vict_equip = vict->equipment[WIELD] ? WIELD : SECONDARY_WEAPON;
      act("$n knocks $p from your grasp!!..and you lose your balance and fall on the ground.", FALSE, ch, obj, obj, TO_VICT);
      act("$n's &+Lflindbar&N vibrates upon hitting $N's $p, making $S fall sprawling to the ground and dropping $S weapon.", TRUE, ch, obj, vict, TO_ROOM);

      SET_POS(vict, POS_SITTING + GET_STAT(vict));
      obj_to_room(unequip_char(vict, vict_equip), ch->in_room);
      CharWait(vict, PULSE_VIOLENCE * 2);
      CharWait(ch, PULSE_VIOLENCE);
    }
  }
  return TRUE;
}

int fade_drusus(P_obj obj, P_char ch, int cmd, char *argument)
{
  P_char   vict;
  char     e_pos, arg[MAX_STRING_LENGTH];
  int      in_battle;

/*  vict = (P_char) arg;*/

  in_battle = cmd / 1000;

  if (cmd == CMD_SET_PERIODIC)               /*
                                   Events have priority 
                                 */
    return FALSE;

  if (!ch || !obj)              /*
                                   If the player ain't here, why are we? 
                                 */
    return FALSE;

  if (!OBJ_WORN(obj))           /*
                                   Most things don't work in a sack... 
                                 */
    return FALSE;

/*
   If it must be wielded, use this 
 */
  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);

  if (!e_pos)
    return FALSE;

/*
   Any powers activated by keywords? Right here, bud. 
 */

  if (argument && (cmd == CMD_SAY))
  {
    argument = one_argument(argument, arg);
    if (!strcmp(arg, "invisible"))
    {
      if (IS_OBJ_STAT(obj, ITEM_LIT))   /* Like to do this throughout */
        REMOVE_BIT(obj->extra_flags, ITEM_LIT);
      if (time_info.day != obj->timer[0])
      {
        act("You say 'invisible'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q glows, and you feel power grow within it.", FALSE, ch,
            obj, obj, TO_CHAR);
        obj->timer[0] = time_info.day;
        act("$n says 'invisible' to $p.", TRUE, ch, obj, obj, TO_ROOM);
        spell_improved_invisibility(35, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_detect_invisibility(35, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        return TRUE;
      }
    }
  }

  if (!in_battle)               /* Past here, and you're fighting */
    return FALSE;

  if (!number(0, 30))
  {
    vict = ch->specials.fighting;
    if (!vict)
      return FALSE;             /* can't be too careful */

    act("&=LWYou score a CRITICAL HIT!!!!!&n", TRUE, ch, 0, 0, TO_CHAR);
    act
      ("&+LA wisp of black shadow shoots forth, striking $N &+Lin the eyes!&N",
       TRUE, ch, 0, vict, TO_CHAR);
    act
      ("&+LA wisp of black shadow shoots forth from $p, &+Lhitting you in the eyes!&N",
       TRUE, ch, obj, vict, TO_VICT);
    spell_pword_blind(35, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    SET_BIT(obj->extra_flags, ITEM_LIT);

    return TRUE;
  }
  return FALSE;
}

int magebane_falchion(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict, tmp_vict;
  char     e_pos;
  int      in_battle;
  int      i;

  vict = (P_char) arg;
  in_battle = cmd / 1000;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!obj)
  {
    logit(LOG_EXIT, "magebane_falchion() called with null obj");
    raise(SIGSEGV);
  }
   /*
     okay.  Deal with the periodic calls first.... periodics are known as ch,
     and cmd will be 0 
   */

  if (!OBJ_WORN(obj))
    return FALSE;

  if (!ch && cmd == CMD_PERIODIC)
  {
    ch = obj->loc.wearing;
    if(GET_STAT(ch) == STAT_SLEEPING)
      return FALSE;
    LOOP_THRU_PEOPLE(tmp_vict, ch)
      if (IS_MAGE(tmp_vict) && (tmp_vict != ch))
         break;

    for (i = 0; i < NUM_EXITS; i++)
      if (!tmp_vict && (world[ch->in_room].dir_option[i]))
      {
        int      r = world[ch->in_room].dir_option[i]->to_room;

        if (r == NOWHERE)
          continue;
        for (tmp_vict = world[r].people; tmp_vict; tmp_vict = tmp_vict->next_in_room)
          if (IS_MAGE(tmp_vict))
            break;
      }
    
    if (tmp_vict && !number(0, 30))
    {
      send_to_char
        ("Your blade &+Wglows brightly&N as it senses spellcasters nearby...\r\n",
         ch);
      return TRUE;
    }
    return FALSE;
  }
  /*
     okay.. if we are this far, its NOT a periodic call. 
   */

  if (cmd < 1000)               /*
                                   Umm... 
                                 */
    return FALSE;

/*
   If it must be wielded, use this for the damage procs 
 */
  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);
  if (!e_pos)
    return FALSE;

  if (IS_MAGE(vict))
  {
    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 4;
    obj->affected[1].location = APPLY_DAMROLL;
    obj->affected[1].modifier = 4;
  }
  else
  {
    obj->affected[0].location = APPLY_NONE;
    obj->affected[1].location = APPLY_NONE;
  }

/** If we scored a _really_ nice hit, let's do some spell shit **/

  if ((!number(0, 30)) && IS_MAGE(vict))
  {
    act("&+wYour $q &+Wflashes brightly &+was it connects with $N&+w!&n", FALSE, ch, obj, vict, TO_CHAR);
    spell_feeblemind((GET_LEVEL(ch) - 5), ch, 0, SPELL_TYPE_SPELL, vict, 0);
  }
  return FALSE;
}


int lightning_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  P_obj    corpse, next_obj, temp;
  char     e_pos;
  int      dam, vict_level;
  bool     hasCorpse;

  if( cmd != CMD_MELEE_HIT )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  // 1/33 chance.
  if( !IS_ALIVE(ch) || !IS_FIGHTING(ch) || !IS_ALIVE(vict) || !OBJ_WORN(obj) || CheckMultiProcTiming(ch) || number(0, 32) )
  {
    return FALSE;
  }

  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ? SECONDARY_WEAPON : 0);
  if( !e_pos )
  {
    return FALSE;
  }

  dam = dice(obj->value[1], obj->value[2]);
  dam += TRUE_DAMROLL(ch);

  dam = (int) (dam * ch->specials.damage_mod);

  if( IS_PC(ch) && IS_PC(vict) )
  {
    dam /= 4;
  }

  if( GET_HIT(vict) < dam )
  {

    spell_haste(15, ch, 0, SPELL_TYPE_SPELL, ch, 0);
    act("&+BA huge &+Ylightning bolt &+Bstreaks forth from the blade of your sword and blasts $N &+Binto tiny pieces!!!&N", FALSE, ch, 0, vict, TO_CHAR);
    act("&+B$p releases a &+Ythunderclap&+L, and blows $N &+Binto tiny pieces!&N", FALSE, ch, obj, vict, TO_NOTVICT);
    act("&+BA huge thunder racing towards you is the last thing you see before falling in the cold sleep of death...&N", FALSE, vict, 0, 0, TO_CHAR);

    vict_level = GET_LEVEL(vict);
    hasCorpse = !IS_NOCORPSE(vict);
    die(vict, ch);

    // Only destroy the corpse if it makes one.
    if( hasCorpse )
    {
      // It will be the first corpse in the room.
      for( corpse = world[ch->in_room].contents; corpse; corpse = next_obj )
      {
        next_obj = corpse->next_content;
        if( (GET_ITEM_TYPE(corpse) == ITEM_CORPSE) )
        {
          for (temp = corpse->contains; temp; temp = next_obj)
          {
            next_obj = temp->next_content;
            obj_from_obj(temp);
            obj_to_room(temp, ch->in_room);
            if( IS_SET(corpse->value[1], PC_CORPSE) )
            {
              logit(LOG_CORPSE, "%s dropped item %s [%d] in room %d.", corpse->short_description, temp->short_description,
                obj_index[temp->R_num].virtual_number, ROOM_VNUM(ch->in_room) );
            }
          }
          if( IS_SET(corpse->value[1], PC_CORPSE) )
          {
            logit(LOG_CORPSE, "%s destroyed by Lightning Sword in %d.", corpse->short_description, ROOM_VNUM(ch->in_room));
          }
          extract_obj(corpse, TRUE); // Empy corpse, but 'in game.'

          if( vict_level > (GET_LEVEL(ch) - 10) )
          {
            spell_chain_lightning(16, ch, NULL, SPELL_TYPE_SPELL, 0, 0);
          }
          return TRUE;
        }
      }
    }
  }

  act("You score a CRITICAL HIT!!!!!\r\n", FALSE, ch, obj, vict, TO_CHAR);
  spell_chain_lightning(16, ch, NULL, SPELL_TYPE_SPELL, 0, 0);

  return FALSE;
}

int woundhealer_scimitar(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  char     e_pos;
  int      in_battle, hits;

  vict = (P_char) arg;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || !(cmd/1000) )
  {
    return FALSE;
  }

  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);
  if( !e_pos )
  {
    return FALSE;
  }

  in_battle = BOUNDED(0, (GET_MAX_HIT(vict) - GET_HIT(vict)), number(1, 8));
  hits = GET_MAX_HIT(ch) - GET_HIT(ch);
  // If we're not full hps/vamped already.
  if( hits > 0 )
  {
    // Don't heal over max hps.
    in_battle = MIN( in_battle, hits );
  }
  else
  {
    // Don't heal someone who's not hurt.
    return FALSE;
  }


  if ((obj->loc.wearing == ch) && vict)
  {
    // act("&+rBlood is around the corner", FALSE, ch, 0, 0, TO_CHAR); 
    GET_HIT(ch) += in_battle;
    //GET_HIT(vict) -= in_battle;
  }
  return FALSE;

}


int flame_of_north_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  char     e_pos;
  int      in_battle, working;

  vict = (P_char) arg;
  in_battle = cmd / 1000;

  if (cmd == CMD_SET_PERIODIC)               /*
                                   Events have priority
                                 */
    return FALSE;

  if (!ch || !obj)              /*
                                   If the player ain't here, why are we?
                                 */
    return FALSE;

  if (!OBJ_WORN(obj))           /*
                                   Most things don't work in a sack...
                                 */
    return FALSE;

/*
   If it must be wielded, use this
 */
  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);
  if (!e_pos)
    return FALSE;

  if (IS_OBJ_STAT(obj, ITEM_LIT))       /*
                                          works only when flame on
                                         */
    working = TRUE;
  else
    working = FALSE;

/*
   Any powers activated by keywords? Right here, bud.
 */
  if (arg && (cmd == CMD_SAY))
  {
    if ((isname(arg, "fly")) && working)
      if (time_info.day != obj->timer[0])
      {
        act("You say 'fly'", FALSE, ch, 0, vict, TO_CHAR);
        act("&+YThe flames of your&N $q &+Yintensify.&N", FALSE, ch, obj, obj, TO_CHAR);
        act("$n says 'fly' to $p.", FALSE, ch, obj, obj, TO_ROOM);
        act("$n's&N $q &+Yglows intently!", TRUE, ch, obj, vict, TO_ROOM);
        spell_fly(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = time_info.day;
        return TRUE;
      }
    if (isname(arg, "light"))
      if (!IS_OBJ_STAT(obj, ITEM_LIT))
      {
        act("$n says 'light' to $p.", FALSE, ch, obj, vict, TO_ROOM);
        act("You say 'light'", FALSE, ch, 0, 0, TO_CHAR);
        act("$p &+Rflares up!&N", FALSE, ch, obj, 0, TO_CHAR);
        act("$n's $q &+Rflares up!&N", FALSE, ch, obj, 0, TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_LIT);
        return TRUE;
      }
    if (isname(arg, "dark"))
      if (IS_OBJ_STAT(obj, ITEM_LIT))
      {
        act("$n says 'dark' to $p.", FALSE, ch, obj, vict, TO_ROOM);
        act("You say 'dark'", FALSE, ch, 0, 0, TO_CHAR);
        act("The flames covering $p die down.", FALSE, ch, obj, 0, TO_CHAR);
        act("The flames covering $n's $q die down.", FALSE, ch, obj, 0, TO_ROOM);
        REMOVE_BIT(obj->extra_flags, ITEM_LIT);
        return TRUE;
      }
  }
  if (!in_battle)               /*
                                   Past here, and you're fighting
                                 */
    return FALSE;

  if ((!number(0, 30)) && working)
  {
    act("&=LWYou score a CRITICAL HIT!!!!!&n", FALSE, ch, 0, 0, TO_CHAR);
    spell_flamestrike(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    return TRUE;
  }
  return FALSE;
}


int staff_of_power(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  char     e_pos;
  int      working, curr_time;

  vict = (P_char) arg;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

/*
   If it must be wielded, use this 
 */
  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);

  if( !e_pos )
  {
    return FALSE;
  }

  if( IS_OBJ_STAT(obj, ITEM_LIT) )
  {
    working = TRUE;
  }
  else
  {
    working = FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if (isname(arg, "aura"))
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 60 <= curr_time)
      {
        act("You say 'aura'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);
        act("$n says 'aura'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_aura_sight(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
    else if ((isname(arg, "flight")) && working)
    {
      // 1 mud day timer.
      if( time_info.day != obj->value[0] )
      {
        act("You say 'flight'", FALSE, ch, 0, NULL, TO_CHAR);
        act("&+YThe flames of your&N $q &+Yintensify.&N", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n says 'flight' to $p.", FALSE, ch, obj, NULL, TO_ROOM);
        act("$n's&N $q &+Yglows intently!", TRUE, ch, obj, NULL, TO_ROOM);
        if( ch->group )
        {
          cast_as_area(ch, SPELL_FLY, 60, 0);
        }
        else
        {
          spell_fly(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        obj->value[0] = time_info.day;
        return TRUE;
      }
    }
    else if( isname(arg, "flesh") && working )
    {
      if( time_info.day != obj->value[0] )
      {
        act("&+bThe flames of your&N $q &+Bintensify.&N", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n says 'flesh' to $p.", FALSE, ch, obj, NULL, TO_ROOM);
        act("$n's&N $q &+Bglows intently!", TRUE, ch, obj, NULL, TO_ROOM);
        if( ch->group )
        {
          cast_as_area(ch, SPELL_FLESH_ARMOR, 60, 0);
        }
        else
        {
          spell_flesh_armor(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        obj->value[0] = time_info.day;
        return TRUE;
      }
    }
    else if( isname(arg, "bio") && working )
    {
      if( time_info.day != obj->value[0] )
      {
        act("You say 'bio'", FALSE, ch, 0, NULL, TO_CHAR);
        act("&+RThe flames of your&N $q &+Bintensify.&N", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n says 'bio' to $p.", FALSE, ch, obj, NULL, TO_ROOM);
        act("$n's&N $q &+Gglows with a GREEN LIGHT!!&N", TRUE, ch, obj, NULL, TO_ROOM);
        if( ch->group )
        {
          cast_as_area(ch, SPELL_BIOFEEDBACK, 60, 0);
        }
        else
        {
          spell_biofeedback(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        obj->value[0] = time_info.day;
        return TRUE;
      }
    }
    else if( isname(arg, "tower") && working )
    {
      if( time_info.day != obj->value[0] )
      {
        act("You say 'tower'", FALSE, ch, obj, NULL, TO_CHAR);
        act("&+CThe flames of your&N $q &+Cintensify.&N", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n says 'tower' to $p.", FALSE, ch, obj, NULL, TO_ROOM);
        act("$n's&N $q &+Wglows and &+Cbands of energy &+Wflow from the staff!&N", TRUE, ch, obj, NULL, TO_ROOM);
        if( ch->group )
        {
          cast_as_area(ch, SPELL_TOWER_IRON_WILL, 60, 0);
        }
        else
        {
          spell_tower_iron_will(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        obj->value[0] = time_info.day;
        return TRUE;
      }
    }
    if( isname(arg, "ecto") && working )
    {
      if (time_info.day != obj->value[0])
      {
        act("You say 'ecto'", FALSE, ch, obj, NULL, TO_CHAR);
        act("&+YThe flames of your&N $q &+Yglow brightly.&N", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n says 'ecto' to $p.", FALSE, ch, obj, NULL, TO_ROOM);
        act("$n's&N $q &+Yglows brightly!", TRUE, ch, obj, NULL, TO_ROOM);
        if( ch->group )
        {
          cast_as_area(ch, SPELL_ECTOPLASMIC_FORM, 60, 0);
        }
        else
        {
          spell_ectoplasmic_form(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        obj->value[0] = time_info.day;
        return TRUE;
      }
    }
    if( isname(arg, "control") && working )
    {
      if( time_info.day != obj->value[0] )
      {
        act("You say 'control'", FALSE, ch, obj, NULL, TO_CHAR);
        act("&+LThe flames of your&N $q &+Lturn pitch black.&N", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n says 'control' to $p.", FALSE, ch, obj, NULL, TO_ROOM);
        act("$n's&N $q &+Yburns with energy!", TRUE, ch, obj, NULL, TO_ROOM);
        if( ch->group )
        {
          cast_as_area(ch, SPELL_EGO_BLAST, 60, 0);
        }
        else
        {
          spell_ego_blast(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        obj->value[0] = time_info.day;
        return TRUE;
      }
    }
    if( isname(arg, "light") )
    {
      if( !IS_OBJ_STAT(obj, ITEM_LIT) )
      {
        act("$n says 'light' to $p.", FALSE, ch, obj, NULL, TO_ROOM);
        act("You say 'light'", FALSE, ch, obj, NULL, TO_CHAR);
        act("$p &+Rflares up!&N", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n's $q &+Rflares up!&N", FALSE, ch, obj, NULL, TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_LIT);
        return TRUE;
      }
    }
    else if( isname(arg, "dark") )
    {
      if (IS_OBJ_STAT(obj, ITEM_LIT))
      {
        act("$n says 'dark' to $p.", FALSE, ch, obj, NULL, TO_ROOM);
        act("You say 'dark'", FALSE, ch, obj, NULL, TO_CHAR);
        act("The flames covering $p die down.", FALSE, ch, obj, NULL, TO_CHAR);
        act("The flames covering $n's $q die down.", FALSE, ch, obj, NULL, TO_ROOM);
        REMOVE_BIT(obj->extra_flags, ITEM_LIT);
        return TRUE;
      }
    }
  }

  if( IS_ALIVE(vict) && cmd == CMD_MELEE_HIT )
  {
    // 1/16 chance.
    if( !number(0, 15) )
    {
      act("&+LYour $q shoots out beams of black energy at $N.", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("$n's $q &+Lradiates a bolt of black energy at you.", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
      act("$n's $q &+Lradiates a blot of black energy at $N.", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
      spell_psychic_crush(61, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
  }
  return FALSE;
}

int staff_of_blue_flames(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict;
  bool staff = false;
  int curr_time;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !obj)
    return FALSE;

  if (!ch && cmd == CMD_PERIODIC)
  {
    hummer(obj);
    return TRUE;
  }
  
  if(!arg)
    return false;

  if((ch->equipment[PRIMARY_WEAPON] &&
     (ch->equipment[PRIMARY_WEAPON] == obj)) ||
     (ch->equipment[HOLD] &&
     (ch->equipment[HOLD]) == obj))
  {
    staff = true;
  }
  else
    return FALSE;
  
  if (cmd != CMD_SAY && cmd != CMD_USE)
    return FALSE;

  if(cmd == CMD_SAY &&
     staff)
  {
    if (isname(arg, "light") && !IS_OBJ_STAT(obj, ITEM_LIT)) 
    {
      act("$n says 'light' to $p.", FALSE, ch, obj, 0, TO_ROOM);
      act("You say 'light'", FALSE, ch, 0, 0, TO_CHAR);
      act("$p &+Rflares up!&N", FALSE, ch, obj, 0, TO_CHAR);
      act("$n's $q &+Rflares up!&N", FALSE, ch, obj, 0, TO_ROOM);
      SET_BIT(obj->extra_flags, ITEM_LIT);
      return TRUE;
    } 
    else if (isname(arg, "dark") && IS_OBJ_STAT(obj, ITEM_LIT)) 
    {
      act("$n says 'dark' to $p.", FALSE, ch, obj, 0, TO_ROOM);
      act("You say 'dark'", FALSE, ch, 0, 0, TO_CHAR);
      act("The flames covering $p die down.", FALSE, ch, obj, 0, TO_CHAR);
      act("The flames covering $n's $q die down.", FALSE, ch, obj, 0, TO_ROOM);
      REMOVE_BIT(obj->extra_flags, ITEM_LIT);
      return TRUE;
    }

    if (!IS_OBJ_STAT(obj, ITEM_LIT))
      return FALSE;
    
    if (isname(arg, "fly")) 
    {
      act("You say 'fly'", FALSE, ch, 0, 0, TO_CHAR);
      act("&+YThe flames of your&N $q &+Yintensify.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n says 'fly' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("$n's&N $q &+Yglows intently!", TRUE, ch, obj, 0, TO_ROOM);
      if (ch->group)
        cast_as_area(ch, SPELL_FLY, 60, 0);
      else
        spell_fly(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    } 
    else if (isname(arg, "haste")) 
    {
      curr_time = time(NULL);

      act("&+bThe flames of your&N $q &+Bintensify.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n says 'haste' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("$n's&N $q &+Bglows intently!", TRUE, ch, obj, 0, TO_ROOM);
      if (ch->group && (obj->timer[1] + 60 <= curr_time) || IS_TRUSTED(ch))
	  {
           cast_as_area(ch, SPELL_HASTE, 60, 0);
           obj->timer[1] = curr_time;
	  }
      else
        spell_haste(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    } 
    else if (isname(arg, "fire")) 
    {
      act("You say 'fire'", FALSE, ch, 0, 0, TO_CHAR);
      act("&+RThe flames of your&N $q &+Bintensify.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n says 'fire' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("$n's&N $q &+Rglows with FIRE!&N", TRUE, ch, obj, 0, TO_ROOM);
      if (ch->group)
        for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
          if (grouped(vict, ch))
            spell_fireshield(60, vict, 0, SPELL_TYPE_SPELL, vict, 0);
      return TRUE;
    } 
    else if (isname(arg, "ice")) 
    {
      act("You say 'ice'", FALSE, ch, 0, 0, TO_CHAR);
      act("&+CThe flames of your&N $q &+Cturn &+Bdark blue.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n says 'ice' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("$n's&N $q &+Cflashes with freezing cold!&N", TRUE, ch, obj, 0, TO_ROOM);
      if (ch->group)
        for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
          if (grouped(vict, ch))
            spell_coldshield(60, vict, 0, SPELL_TYPE_SPELL, vict, 0);
      return TRUE;
    } 
    else if (isname(arg, "grow")) 
    {
      curr_time = time(NULL);

      act("You say 'grow'", FALSE, ch, 0, 0, TO_CHAR);
      act("&+YThe flames of your&N $q &+Yglow brightly.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n says 'grow' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("$n's&N $q &+Yglows brightly!", TRUE, ch, obj, 0, TO_ROOM);
      if (ch->group && (obj->timer[2] + 60 <= curr_time) || IS_TRUSTED(ch))
	  {
            cast_as_area(ch, SPELL_ENLARGE, 60, 0);
	    obj->timer[2] = curr_time;
	  }
      else
       spell_enlarge(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    } 
    else if (isname(arg, "invisible")) 
    {
      act("You say 'invisible'", FALSE, ch, 0, 0, TO_CHAR);
      act("&+LThe flames of your&N $q &+Lturn pitch black.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n says 'invisible' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("$n's&N $q &+Lturns pitch black!", TRUE, ch, obj, 0, TO_ROOM);
      if (ch->group) 
        spell_mass_invisibility(60, ch, 0, 0, 0, 0);
      else
        spell_improved_invisibility(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    } 
    else if (isname(arg, "stone")) 
    {
      curr_time = time(NULL);

      act("You say 'stone'", FALSE, ch, 0, 0, TO_CHAR);
      act("&+bThe flames of your&N $q &+Bintensify.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n says 'stone' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("$n's&N $q &+Bglows intently!", TRUE, ch, obj, 0, TO_ROOM);
      if (ch->group && (obj->timer[5] + 60 <= curr_time) || IS_TRUSTED(ch))
	  {
           cast_as_area(ch, SPELL_STONE_SKIN, 60, 0);
           obj->timer[5] = curr_time;
	  }
      else
        spell_stone_skin(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    } 
    else if (isname(arg, "globe")) 
    {
      curr_time = time(NULL);

      act("You say 'globe'", FALSE, ch, 0, 0, TO_CHAR);
      act("&+bThe flames of your&N $q &+Bintensify.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n says 'globe' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("$n's&N $q &+Bglows intently!", TRUE, ch, obj, 0, TO_ROOM);
      if (ch->group && (obj->timer[4] + 60 <= curr_time) || IS_TRUSTED(ch))
	  {
           cast_as_area(ch, SPELL_GLOBE, 60, 0);
           obj->timer[4] = curr_time;
	  }
      else
        spell_globe(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    } 
  }
  
  if (arg && (cmd == CMD_USE) && staff)
  {
    if (isname(arg, "flames") || isname(arg, "staff") || isname(arg, "blue"))
    {
    act("&+bYou raise $p &+bhigh and invoke the power of &-L&+Blightning&+B!", TRUE, ch, obj, 0, TO_CHAR);
    act("&+bArcs of &-L&+Blightning &+bleap from your $q&+B!", FALSE, ch, obj, 0, TO_CHAR);
    act("$n &+braises $p &+bhigh and invokes the power of &-L&+Blightning&+B!", FALSE, ch, obj, 0, TO_ROOM);
    act("&+bArcs of &-L&+Blightning &+bleap from $q&+B!", FALSE, ch, obj, 0, TO_ROOM);
    cast_as_area(ch, SPELL_CHAIN_LIGHTNING, 46, 0);
    CharWait(ch, (int) 2.75 * PULSE_VIOLENCE);
    return TRUE;
    }
  }

  return false;
}


#define DECAYABLE(r) (!IS_UNDERWORLD(r) && IS_SUNLIT(r))

int generic_drow_eq(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      in_battle;

  vict = (P_char) arg;
  in_battle = cmd / 1000;

  if (cmd == CMD_SET_PERIODIC)               /* Events have priority */
    return FALSE;

  if (!ch || !obj)              /* If the player ain't here, why are we? */
    return FALSE;

  if (DECAYABLE(ch->in_room))
  {
    act("Your $q quickly decays in the surface air!", TRUE, ch, obj, vict, TO_CHAR);
    act("$n's $q quickly decays in the surface air!", TRUE, ch, obj, vict, TO_ROOM);
    extract_obj(obj, TRUE); // Not an arti, but 'in game.'
  }
  return FALSE;
}

#undef DECAYABLE

int elfdawn_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict, tmp_vict;
  char     e_pos;
  int      in_battle, bad_owner;

  vict = (P_char) arg;
  in_battle = cmd / 1000;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!OBJ_WORN(obj))
  {
    return FALSE;
  }
/** Who's in the room? Careful here, or it will spam player **/
  if (!in_battle)
  {
    LOOP_THRU_PEOPLE(tmp_vict, ch)
      if (((IS_AFFECTED(tmp_vict, AFF_INVISIBLE)) ||
           (IS_AFFECTED(tmp_vict, AFF_HIDE))) && (tmp_vict != ch))
      act("$p &+Wglows brightly&N as it senses hidden life in the room.",
          FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
  }
  if (!ch || !obj)
    return FALSE;

/** Check class first off, burn em, and let em keep it **/

  if (!GET_CLASS(ch, CLASS_RANGER) && (OBJ_WORN_BY(obj, ch)))
  {
    act
      ("&+gThe blade briefly flashes with &N&+Yintense light&N&+g, burning your hand severely!&N",
       TRUE, ch, obj, vict, TO_CHAR);
    act
      ("$n's &+gblade briefly flashes with &N&+Yintense light&N&+g, burning $n hand severely!&N",
       TRUE, ch, obj, vict, TO_ROOM);
    GET_HIT(ch) -= 5;
    bad_owner = TRUE;
  }
  else
    bad_owner = FALSE;

  if (!OBJ_WORN(obj))
    return FALSE;

  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);

  /*
     must be wielded 
   */
  if (!e_pos)
    return (FALSE);

  if (!IS_FIGHTING(ch))
  {
    LOOP_THRU_PEOPLE(tmp_vict, ch)
      if (IS_EVIL(tmp_vict) && (tmp_vict != ch) && (IS_NPC(tmp_vict)) &&
          (!bad_owner))
    {
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 10;   /* First blow, bound to hit */
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 5;    /* And good damage, make up */
      attack(ch, tmp_vict);     /* for always attacking */
      return TRUE;
    }
  }
  if (IS_FIGHTING(ch))
  {                             /* Normally, only plus +3 */
    if (IS_EVIL(GET_OPPONENT(ch)) && (!bad_owner))
    {
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 3;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 3;
    }
    else
    {
      obj->affected[0].location = APPLY_NONE;   /* And then, only if you */
      obj->affected[1].location = APPLY_NONE;   /* battle evil */
    }
  }
  if (cmd < 1000)
    return FALSE;

  return TRUE;
}

bool trident_charm(P_char ch, P_char victim)
{
  int      level;

  level = GET_LEVEL(ch);

  if (victim->following && (victim->following == ch))
    return FALSE;

  if (IS_NPC(victim) &&
      !GET_MASTER(victim) &&
      !NewSaves(victim, SAVING_PARA, (GET_LEVEL(ch) - GET_LEVEL(victim)) / 7)
      && ((GET_LEVEL(ch) - 10) > GET_LEVEL(victim)))
  {

    if (victim->following)
      stop_follower(victim);
    add_follower(victim, ch);

    setup_pet(victim, ch, level / 4, 0);

    if (IS_FIGHTING(victim))
      stop_fighting(victim);
    if( IS_DESTROYING(victim))
      stop_destroying(victim);

    StopMercifulAttackers(victim);

    return TRUE;

  }
  else if (IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if (!IS_FIGHTING(victim))
      MobStartFight(victim, ch);
    return FALSE;
  }
}

int undead_trident(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, tmp_vict;
  char   e_pos;
  bool   bad_owner;

  vict = (P_char) arg;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) || cmd < 1000 )
  {
    return FALSE;
  }

  /** Check class first off, burn em, and let em keep it **/
  if( GET_ALIGNMENT(ch) != -1000 && OBJ_WORN_BY(obj, ch) )
  {
    /*  act("&+LThe trident briefly flashes with &N&+Yintense light&N&+L, burning your hand severely!&N", TRUE, ch, obj, vict, TO_CHAR);
       act("$n screams in pain, as $s blade burns into $s skin!", FALSE, ch, obj,
       NULL, TO_ROOM);
       GET_HIT(ch) -= 5; */
    bad_owner = TRUE;
  }
  else
  {
    bad_owner = FALSE;
  }

  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);
  /* must be wielded */
  if( !e_pos || !IS_ALIVE(vict) || vict == ch )
  {
    return FALSE;
  }

  if( IS_FIGHTING(ch) && !bad_owner )
  {
    if( IS_UNDEAD(vict) )
    {
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 7;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 7;
      // 1/30 chance.
      if( !number(0, 29) )
      {
        if( trident_charm(ch, vict) )
        {
          act("$n &+L raises $s $p three times and $N suddenly starts following him as if charmed by some powerful magic.&n", FALSE, ch, obj, vict, TO_ROOM);
          act("&+LYou see $n raise $s $p and you can't resist an urge to follow him...&n", FALSE, ch, obj, vict, TO_VICT);
          act("&+LThe magic of your trident enslaves $N and traps $M under your command!&n", FALSE, ch, obj, vict, TO_CHAR);
          return TRUE;
        }
        else
        {
          act("&+LYou try to charm $N with the magic of your trident, but fail miserably.&n", FALSE, ch, 0, vict, TO_CHAR);
          act("&+L$n tries to pull some dirty magical trick on you, but you easily avoid it.&n", FALSE, ch, 0, vict, TO_VICT);
        }
      }
    }
    else if( IS_DEMON(vict) )
    {
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 5;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 5;
    }
    else if( IS_ELEMENTAL(vict) )
    {
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 3;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 3;
    }
    else
    {
      obj->affected[0].location = APPLY_NONE;
      obj->affected[1].location = APPLY_NONE;
    }
  }
  return FALSE;
}

int martelo_mstar(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  char     e_pos;
  int      bad_owner;

  vict = (P_char) arg;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !obj )
  {
    return FALSE;
  }

  /** Check class first off, burn em, and let em keep it **/
  if( !GET_CLASS(ch, CLASS_CLERIC) && OBJ_WORN_BY(obj, ch) )
  {
    act("&+BThe star briefly flashes with &N&+Yintense light&N&+B, burning your hand severely!&N", TRUE, ch, obj, ch, TO_CHAR);
    act("$N screams in pain, as $S star burns into $S skin!", FALSE, ch, obj, ch, TO_ROOM);
    GET_HIT(ch) -= 5;
    update_pos(ch);
    bad_owner = TRUE;
  }
  else
  {
    bad_owner = FALSE;
  }

  /* Clerics get the bonus, no one else does */
  if( !bad_owner )
  {
    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 5;
    obj->affected[1].location = APPLY_DAMROLL;
    obj->affected[1].modifier = 5;
  }
  else
  {
    obj->affected[0].location = APPLY_NONE;
    obj->affected[1].location = APPLY_NONE;
  }

  // Full heal on keyword, 1 per day
  if( arg && cmd == CMD_SAY && !bad_owner )
  {
    if( isname(arg, "martelo") )
    {
      if( time_info.day != obj->value[0] )
      {
        act("You say 'martelo'", FALSE, ch, 0, vict, TO_CHAR);
        act("$n says 'martelo' to $p.", FALSE, ch, obj, obj, TO_ROOM);
        spell_full_heal(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        act("$n's&N $q &+Yglows intently!", TRUE, ch, obj, vict, TO_ROOM);
        obj->value[0] = time_info.day;
        return TRUE;
      }
    }
  }

  e_pos = OBJ_WORN_POS( obj, WIELD ) ? WIELD : OBJ_WORN_POS( obj, SECONDARY_WEAPON ) ? SECONDARY_WEAPON : 0;

  if( cmd < 1000 || !OBJ_WORN(obj) || !e_pos )
  {
    return FALSE;
  }

  // 1/30 chance.
  if( !number(0, 29) && !bad_owner )
  {
    act("&=LWYou score a CRITICAL HIT!!!!!&n", TRUE, ch, 0, 0, TO_CHAR);
    act("&+WAn intense vibration washes through your morningstar, as you strike $N &+Wsquare across the face!&N", TRUE, ch, 0, vict, TO_CHAR);
    act("$n's $q &+Wstrikes you square across the face, vibrating intensely!&N", TRUE, ch, obj, vict, TO_VICT);
    spell_harm(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    return TRUE;
  }
  return FALSE;
}

/*********************** Mobs ******************************/

int um_durnan(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 50))
  {
  case 1:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "Welcome to the Yawning Portal!");
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_GRIN);
    mobsay(ch, "Ahhh, so you've come to test your might in Undermountain?");
    return TRUE;
  case 3:
    mobsay(ch, "Many have gone into Undermountain, but few return alive.");
    return TRUE;
  case 4:
    act("$n looks you up and down.", 0, ch, 0, 0, TO_ROOM);
    mobsay(ch, "Perhaps I can sell you a few extra items for your journey.");
    return TRUE;
  case 5:
    mobsay(ch, "Deep within Undermountain, they say Skullport lies.");
    mobsay(ch, "'Tis and underground city of pirates and thieves...");
    return TRUE;
  case 6:
    mobsay(ch, "Undermountain is a very dangerous place.");
    mobsay(ch, "It is best to travel in large groups.");
    return TRUE;
  case 7:
    mobsay(ch, "They say that Drow and Grey Elves mingle in Skullport.");
    mobsay(ch, "But I'll only believe it when I see it...");
    return TRUE;
  case 8:
    mobsay(ch, "It is said that Halaster took nine apprentices...");
    mobsay(ch, "Each one added to the flavor of the dungeon.");
    return TRUE;
  case 9:
    mobsay(ch, "Some say Halaster still roams the Halls of Undermountain.");
    return TRUE;
  case 10:
    mobsay(ch, "Old Halaster enslaved many races to complete the dungeon.");
    return TRUE;
  case 11:
    mobsay(ch, "I remember when I discovered Undermountain...");
    mobsay(ch, "That first trip into the dark halls was terrifying...");
    mobsay(ch, "But exhillerating.");
    return TRUE;
  case 12:
    mobsay(ch, "Each apprentice designed a powerful weapon and hid it...");
    return TRUE;
  case 13:
    mobsay(ch, "They say there are Nine Great Weapons of Undermountain...");
    mobsay(ch, "Hmm...Lemme try to remember what each weapon was...");
    return TRUE;
  case 14:
    mobsay(ch, "The Blade of Paladins is a Paladin's sword...");
    return TRUE;
  case 15:
    mobsay(ch, "Elfdawn is a Ranger's sword...");
    return TRUE;
  case 16:
    mobsay(ch, "Devil's Thorn is an Anti-Paladin's unholy weapon...");
    return TRUE;
  case 17:
    mobsay(ch, "Woodsong is a Druid's Scimitar...");
    return TRUE;
  case 18:
    mobsay(ch, "The Staff of Hamanathu is a powerful mage weapon...");
    return TRUE;
  case 19:
    mobsay(ch, "The Sword of the High Duke Ygasryil...");
    mobsay(ch, "'Tis a mighty weapon for Warriors...");
    return TRUE;
  case 20:
    mobsay(ch, "Shenahl created a Dancing Cutlass for Bards...");
    return TRUE;
  case 21:
    mobsay(ch, "The Blood Blade of Relgata is a fierce weapon for Thieves.");
    return TRUE;
  case 22:
    mobsay(ch, "Martelo's Morningstar is a strong Cleric's weapon...");
    return TRUE;
  case 23:
    act("$n wipes down the bar.", 0, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 24:
    act("$n ponders as he thinks of more tales of Undermountain...",
        0, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 25:
    do_action(ch, 0, CMD_CHUCKLE);
    mobsay(ch, "I remember the first time old Kevlar entered Undermountain.");
    mobsay(ch, "He whined like a baby when he returned...");
    mobsay(ch, "About all sorts of monsters and traps...");
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

int um_mhaere(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 8))
  {
  case 1:
    act("$n smiles warmly at you.", 0, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("$n serves a few drinks to a bar patron.", 0, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    act("$n looks lovingly at Durnan.", 0, ch, 0, 0, TO_ROOM);
    mobsay(ch, "Durnan just loves to tell tales of Undermountain.");
    return TRUE;
  case 4:
/*    do_action(ch, "durnan", CMD_FLIRT); */
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

int um_regular(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 5))
  {
  case 1:
    mobsay(ch, "Durnan! Another round for me friends.");
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_BURP);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_SING);
    return TRUE;
  case 4:
    mobsay(ch, "Few enter, fewer survive...");
    return TRUE;
  case 5:
    act("$n looks at you.", 0, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_LAUGH);
  default:
    return FALSE;
  }
  return FALSE;
}

int um_gambler(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch, "The game is five card draw!");
    return TRUE;
  case 2:
    mobsay(ch, "My four aces, beats your pair!");
    return TRUE;
  case 3:
    mobsay(ch, "My straight flush beats your three of a kind!");
    return TRUE;
  case 4:
    mobsay(ch, "My full house beats you two pair!");
    return TRUE;
  case 5:
    mobsay(ch, "The game is five card stud!");
    return TRUE;
  case 6:
    act("$n prays to Tymora for luck.", 0, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 7:
    mobsay(ch, "Perhaps you should pray to Tymora for luck!");
    return TRUE;
  case 8:
    mobsay(ch, "Wanna play a hand of poker?");
    return TRUE;
  case 9:
    mobsay(ch, "Luck is all you need to survive Undermountain!");
    return TRUE;
  case 10:
    do_action(ch, 0, CMD_EYEBROW);
    mobsay(ch, "I never cheat, I'm just lucky.");
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

int um_tamsil(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 10))
  {
  case 1:
    mobsay(ch, "Can I get you anything? Drinks perhaps?");
    return TRUE;
  case 2:
    mobsay(ch, "Be careful when you enter Undermountain!");
    return TRUE;
  case 3:
    mobsay(ch, "My father was the first to enter Undermountain!");
    return TRUE;
  case 4:
    mobsay(ch,
           "Daddy used to lead the group 'A Company of Crazed Adventurers.'");
    return TRUE;
  case 5:
    mobsay(ch, "Come back and see us soon!");
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

int um_kevlar(P_char ch, P_char pl, int cmd, char *arg)
{
  int      i;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  for (i = 0; i < NUM_EXITS; i++)
  {
    if (EXIT(ch, i) && (EXIT(ch, i)->to_room != NOWHERE) &&
        IS_SET(EXIT(ch, i)->exit_info, EX_CLOSED))
    {
      if (IS_SET(EXIT(ch, i)->exit_info, EX_LOCKED) &&
          !IS_SET(EXIT(ch, i)->exit_info, EX_PICKPROOF))
      {
        do_action(ch, 0, CMD_PONDER);
        act("$n skillfully picks the lock!", 1, ch, 0, 0, TO_ROOM);
        REMOVE_BIT(EXIT(ch, i)->exit_info, EX_LOCKED);
      }
/*      do_open(ch, "door", 0);
   increase
   capability
   here someday 
 */
    }
  }

  switch (number(1, 10))
  {
  case 1:
    mobsay(ch, "Ah, I see you are worthy adventurers of Undermountain!");
    return TRUE;
  case 2:
    mobsay(ch, "If there is anything we can do for you, just ask.");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_SMILE);
    mobsay(ch, "Well met, heroes!");
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_PEER);
    mobsay(ch, "Be careful, Undermountain is a dangerous place!");
    return TRUE;
  case 5:
    do_action(ch, 0, CMD_PONDER);
    act("$n rubs his black bald head.", 0, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 6:
    mobsay(ch, "Hmm, have you seen a half-elven ranger named Isaac?");
    mobsay(ch, "We seem to have lost him.");
    return TRUE;
  case 7:
    mobsay(ch, "Bah!");
    mobsay(ch, "Why did Halaster have to make this dungeon teleport-proof?");
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

int um_thorn(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   leader;
  struct affected_type af;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  LOOP_THRU_PEOPLE(leader, ch)
  {
    if (IS_NPC(leader) && (GET_VNUM(leader) == 92020) &&
        !ch->following && (GET_VNUM(ch) == (92021)))
    {
      if (ch->following)
        stop_follower(ch);
      add_follower(ch, leader); /*
                                   Follow 
                                   and
                                   assist 
                                   leader 
                                 */
      setup_pet(ch, leader, 9999, 0);
    }
  }

  switch (number(1, 10))
  {
  case 1:
/*    do_action(ch, "kevlar", CMD_POKE); */
    mobsay(ch, "Lets go! I'm tired of standing here.");
    do_action(ch, 0, CMD_TAP);
    return TRUE;
  case 2:
/*    do_action(ch, "kevlar", CMD_ROLL); */
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_SIGH);
    mobsay(ch, "Kevlar! You talk too much!");
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_EYEBROW);
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

int um_korelar(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   leader;
  struct affected_type af;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  LOOP_THRU_PEOPLE(leader, ch)
  {
    if (IS_NPC(leader) && (GET_VNUM(leader) == (92020)) &&
        !ch->following && (GET_VNUM(ch) ==(92022)))
    {
      if (ch->following)
        stop_follower(ch);
      add_follower(ch, leader); /*
                                   Follow 
                                   and
                                   assist 
                                   leader 
                                 */
      setup_pet(ch, leader, 9999, 0);
    }
  }

  switch (number(1, 10))
  {
  case 1:
    mobsay(ch, "Ah, I see you like Thorn's Dragon Armor!");
    mobsay(ch, "That was my best work!");
    return TRUE;
  case 2:
/*    do_action(ch, "thorn", CMD_LOOK); */
    return TRUE;
  case 3:
/*    do_action(ch, "kevlar", CMD_LOOK); */
    mobsay(ch, "Kevlar, can I borrow your spell books?");
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_YAWN);
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

int um_essra(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || !IS_FIGHTING(ch))
    return FALSE;

  switch (number(1, 10))
  {
  case 1:
    mobsay(ch, "Infidel! You will die today!");
    return TRUE;
  case 2:
    mobsay(ch, "You will see true power today!");
    mobsay(ch, "You will see the power of Salvetarm!");
    return TRUE;
  case 3:
/*    do_action(ch, "elite", CMD_LOOK); */
    mobsay(ch, "Guards! Kill them!");
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_LAUGH);
    mobsay(ch, "You puny abilities are no match for the power of Salvetarm!");
    return TRUE;
  case 5:
    mobsay(ch, "You will fall today!");
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

int um_mezzoloth(P_char ch, P_char pl, int cmd, char *arg)
{
  static int hired;
  int      gold;
  struct affected_type af;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if (pl && (pl != ch))
  {
    if (cmd == CMD_GIVE)
    {
      gold = GET_MONEY(ch);
      do_give(pl, arg, 0);
      if ((gold = (GET_MONEY(ch) - gold)))
      {
        if (gold < 100000)
          mobsay(ch,
                 "A generous offer indeed, but my services are worth more than that.");
        else
        {
          if (ch->following)    /*
                                   Should
                                   another be
                                   allowed to 
                                 */
            return FALSE;       /*
                                   buy your merc. out
                                   from under you? 
                                 */
          /*
             stop_follower(ch); 
           */
          mobsay(ch, "Excellent! I shall serve thee well.");
          add_follower(ch, pl);
          setup_pet(ch, pl, 9999, 0);
          hired = TRUE;
        }
        return TRUE;
      }
    }
  }
  if (!hired)
  {
    switch (number(1, 4))
    {
    case 1:
      mobsay(ch, "Perhaps you could use some extra help?");
      return TRUE;
    case 2:
      mobsay(ch, "I am for hire, for the right price.");
      return TRUE;
    case 3:
      mobsay(ch, "For 1000 gold, I'll aid you in exploring Undermountain!");
      return TRUE;
    case 4:
      act("$n looks you up and down, while assessing your skills.",
          0, ch, 0, 0, TO_ROOM);
      return TRUE;
    default:
      return FALSE;
    }
  }
  return FALSE;
}

int um_goblin_leader(P_char ch, P_char pl, int cmd, char *arg)
{
  // is a mimic
  return FALSE;
}

int flying_dagger(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    dagger, i, temp, next_obj;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  // Special die aspect
  if (cmd == CMD_DEATH)
  {
    dagger = read_object(92044, VIRTUAL);
    act("A final blow, and the $n spins no more.", 1, ch, 0, 0, TO_ROOM);
    obj_to_room(dagger, ch->in_room);
    return TRUE;
  }
  for (i = world[ch->in_room].contents; i; i = i->next_content)
  {
    if (GET_ITEM_TYPE(i) == ITEM_CORPSE)
    {
      for (temp = i->contains; temp; temp = next_obj)
      {
        next_obj = temp->next_content;
        obj_from_obj(temp);
        obj_to_room(temp, ch->in_room);
      }

      if (IS_SET(i->value[1], PC_CORPSE))
      {
        logit(LOG_CORPSE, "%s destroyed by flying daggers in %d.", i->short_description, world[i->loc.room].number);
      }
      act("$n shreds the $o, &+rblood flies everywhere!", FALSE, ch, i, 0, TO_ROOM);
      extract_obj(i, TRUE); // Empty corpse, but 'in game.'
      return TRUE;
    }
  }
  return FALSE;
}

int ochre_jelly(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    i, temp, next_obj;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;

  if( cmd != CMD_PERIODIC )
    return FALSE;

  for (i = world[ch->in_room].contents; i; i = i->next_content)
  {
    if (GET_ITEM_TYPE(i) == ITEM_CORPSE)
    {
      for (temp = i->contains; temp; temp = next_obj)
      {
        next_obj = temp->next_content;
        obj_from_obj(temp);
        obj_to_room(temp, ch->in_room);
      }
      if (IS_SET(i->value[1], PC_CORPSE))
      {
        logit(LOG_CORPSE, "%s devoured by an Ochre Jelly in %d.", i->short_description, world[i->loc.room].number);
      }
      act("$n cleans the area.", FALSE, ch, i, 0, TO_ROOM);
      extract_obj(i, TRUE); // Empty corpse, but 'in game.'
      return TRUE;
    }
  }
  return FALSE;
}

int animated_sword(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    sword;

  sword = read_object(92087, VIRTUAL);

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
  {                             /*
                                   special die aspect 
                                 */
    act("A final blow, and the $n dances no more.", 1, ch, 0, 0, TO_ROOM);
    obj_to_room(sword, ch->in_room);
    return TRUE;
  }
  return FALSE;
}

int helmed_horror(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    helm;

  helm = read_object(92091, VIRTUAL);

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
  {                             /*
                                   special die aspect 
                                 */
    act("A final blow, and the $n drops to the floor, an empty suit.", 1, ch,
        0, 0, TO_ROOM);
    obj_to_room(helm, ch->in_room);
    return TRUE;
  }
  return FALSE;
}

int malodine_one(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || !IS_FIGHTING(ch))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    vict = ch->specials.fighting;

    act("$n rises slowly into the air, and turns to face $N.", 1, ch, 0, vict,
        TO_NOTVICT);
    act("$n rises slowly into the air, and turns to face YOU!", 1, ch, 0,
        vict, TO_VICT);
    act("You rise slowly into the air, and turn to face $N.", 1, ch, 0, vict,
        TO_CHAR);
    act
      ("A gem it uses for a tooth, begins to glow, as a &+Yray of light shoots forth!",
       1, ch, 0, vict, TO_NOTVICT);
    act
      ("A gem it uses for a tooth, begins to glow, as a &+Yray of light shoots forth!",
       1, ch, 0, vict, TO_CHAR);

    spell_pword_kill(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    stop_fighting(ch);
    stop_fighting(vict);

    act("Its anger sated, the $n drops back down, and is lifeless again", 1,
        ch, 0, vict, TO_ROOM);
    act("Its anger sated, the $n drops back down, and is lifeless again", 1,
        ch, 0, vict, TO_CHAR);

    GET_HIT(ch) = GET_MAX_HIT(ch);      /*
                                           unkillable 
                                         */
    return TRUE;
  }
  return FALSE;
}

int malodine_two(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   ghost;

  ghost = read_mobile(92096, VIRTUAL);

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
  {                             /*
                                   special die aspect 
                                 */
    char_to_room(ghost, ch->in_room, 0);
    return TRUE;
  }
  return FALSE;
}

int malodine_three(P_char ch, P_char pl, int cmd, char *arg)
{

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || !pl)
    return FALSE;

  if (IS_FIGHTING(ch) && !number(0, 2) && !resists_spell(ch, pl))
  {
    AgeChar(pl, dice(2, 20));
    send_to_char("You feel a bit older all of a sudden!\r\n", pl);
    return TRUE;
  }
  return FALSE;
}

int black_pudding(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   split, split2;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
  {                             /*
                                   special die aspect 
                                 */
    if (GET_VNUM(ch) ==(92607))
    {
      split = read_mobile(92629, VIRTUAL);
      split2 = read_mobile(92629, VIRTUAL);
    }
    else if (GET_VNUM(ch) ==(92629))
    {
      split = read_mobile(92630, VIRTUAL);
      split2 = read_mobile(92630, VIRTUAL);
    }
    else if (GET_VNUM(ch) ==(92630))
    {
      split = read_mobile(92631, VIRTUAL);
      split2 = read_mobile(92631, VIRTUAL);
    }
    else if (GET_VNUM(ch) ==(92631))
    {
      split = read_mobile(92632, VIRTUAL);
      split2 = read_mobile(92632, VIRTUAL);
    }
    else if (GET_VNUM(ch) ==(92632))
    {
      split = read_mobile(92633, VIRTUAL);
      split2 = read_mobile(92633, VIRTUAL);
    }
    else if (GET_VNUM(ch) ==(92633))
    {
      split = read_mobile(92633, VIRTUAL);
      split2 = read_mobile(92633, VIRTUAL);
    }
    char_to_room(split, ch->in_room, 0);
    char_to_room(split2, ch->in_room, 0);
    return TRUE;
  }
  return FALSE;
}

#define NUM_PROCCING_SLOTS 31
extern int proccing_slots[];

void event_flame_of_north(P_char ch, P_char victim, P_obj obj, void *data);

int flame_of_north(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    // 1/15 chance to hum
    if( !number(0, 14) && OBJ_ROOM(obj) )
	  {
	    act("&+rStrange, magical &+Rflames&+r seem to dance along the blade of the $q&+r!&n", TRUE, 0, obj, 0, TO_ROOM);
	  }
    return FALSE;
  }

  if( cmd == CMD_REMOVE && arg )
  {
    if( isname(arg, obj->name) || isname(arg, "all") )
    {
      if( affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD) )
      {
        affect_from_char(ch, SPELL_ILIENZES_FLAME_SWORD);
      }
      if( affected_by_spell(ch, SPELL_CEGILUNE_BLADE) )
      {
        affect_from_char(ch, SPELL_CEGILUNE_BLADE);
      }
      send_to_char("&+rThe flames surrounding you slowly burn out...&n\r\n", ch);
    }
   return FALSE;
  }

  if( !get_scheduled(ch, event_flame_of_north) )
  {
    // Character doesn't have the event scheduled, so fire the event which handles the affect and will renew itself.
    add_event(event_flame_of_north, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
  }
  return FALSE;
}

void event_flame_of_north(P_char ch, P_char victim, P_obj obj, void *data)
{
  // first check to make sure the item is still on the character
  bool has_item = FALSE;
  int dam;
  char buf[256];

  // search through all of the possible proc spots of character
  // and check to see if the item is equipped
  for( int i = 0; i < NUM_PROCCING_SLOTS; i++ )
  {
    P_obj item = ch->equipment[proccing_slots[i]];

    if( item && obj_index[item->R_num].func.obj == flame_of_north )
    {
      has_item = TRUE;
      break;
    }
  }

  if( !has_item )
  {
	  return;
  }

  if( !IS_SET(world[ch->in_room].room_flags, NO_MAGIC) )
  {
     if( !affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD) && !number(0, 3) )
     {
       spell_ilienzes_flame_sword(70, ch, 0, SPELL_TYPE_SPELL, ch, 0);
     }
     if( !affected_by_spell(ch, SPELL_CEGILUNE_BLADE) && affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD) && !number(0, 4) )
     {
       spell_cegilunes_searing_blade(70, ch, 0, SPELL_TYPE_SPELL, ch, 0);
     }
  }
  if( affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD) && !number(0, 15) && !GET_SPEC(ch, CLASS_REAVER, SPEC_FLAME_REAVER)
    && !IS_TRUSTED(ch) && GET_HIT(ch) > 50 )
  {
    act("$n&+R winces in pain, as the flames surrounding $s sword are too much for $m to handle!&n", TRUE, ch, 0, victim, TO_NOTVICT);
    act("&+RIn your unskilled hands, the flames surrounding the Flame of the North cause you great pain!&n", TRUE, ch, 0, victim, TO_CHAR);

    dam = number(20, 50);
    if( affected_by_spell(ch, SPELL_CEGILUNE_BLADE) )
    {
      dam += number(5, 15);
    }
    spell_damage(ch, ch, dam, SPLDAM_FIRE, SPLDAM_NOSHRUG | SPLDAM_NOVAMP | SPLDAM_NODEFLECT | RAWDAM_NOKILL, 0);
  }

  add_event(event_flame_of_north, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
}

