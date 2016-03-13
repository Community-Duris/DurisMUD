/*
 * ***************************************************************************
 * *  File: range.c                                           Part of Duris *
 * *  Usage: Range weapon
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "justice.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "range.h"
#include "sound.h"
#include "damage.h"
#include "objmisc.h"

/*
 * external variables
 */

extern P_desc descriptor_list;
extern P_event current_event;
extern P_room world;
extern P_index mob_index;
extern P_index obj_index;
extern const char *dirs[];
extern const char *dirs2[];
extern const char *shot_types[];
extern const int shot_damage[];
extern int rev_dir[];
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;

extern P_char misfire_check(P_char ch, P_char spell_target, int flag);

void     do_cover(P_char, char *, int);

#define                 RANGE_DAMAGE_MULTIPLIER  18

#define                 ARROW_NONE  0
#define                 ARROW_MARK  1
#define                 ARROW_EFFECT 2

/*
* Fire a fire_weapon
*
* FIRE <victim> [direction]
*/

void do_gather(P_char ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  P_char tmp_char;
  P_obj corpse, tobj, quiver, next_obj;
  int bits, i, j, g, type;
  int weight = 0, full = 0;

  one_argument(argument, name);

  // function uses GET_PID, so lets not let npc's use it...
  if (IS_NPC(ch))
    return;

  for (i = 0, quiver = NULL; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
    {
      if (ch->equipment[i]->type == ITEM_QUIVER)
      {
        quiver = unequip_char(ch, i);
	//GET_CARRYING_W(ch) -= GET_OBJ_WEIGHT(quiver);
	break;
      }
    }
  }

  if (!quiver)
  {
    send_to_char("You need to be wearing a quiver to perform this action.\n", ch);
    return;
  }

  if (!*name)
  {
    tobj = world[ch->in_room].contents;
    type = 1;
  }
  else
  {
    if( IS_TRUSTED(ch) )
    {
      bits = generic_find(name, FIND_OBJ_ROOM, ch, &tmp_char, &corpse);
    }
    else
    {
      bits = generic_find(name, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &tmp_char, &corpse);
    }

    if (!corpse)
    {
      send_to_char("You don't see that here.\n", ch);
      equip_char(ch, quiver, i, FALSE);
      return;
    }
    else if (corpse->type != ITEM_CORPSE)
    {
      send_to_char("You can't gather from that.\n", ch);
      equip_char(ch, quiver, i, FALSE);
      return;
    }
    tobj = corpse->contains;
    type = 2;
  }

  for (j = 0, g = 0;tobj;tobj = next_obj)
  {
    next_obj = tobj->next_content;
    
    if (tobj->type != ITEM_MISSILE)
      continue;
    if (tobj->timer[0] != GET_PID(ch))
      continue;

    if (type == 1)
      obj_from_room(tobj);
    else if (type == 2)
      obj_from_obj(tobj);
    
    obj_to_obj(tobj, quiver);
    j++;
    quiver->value[3]++;
    weight += GET_OBJ_WEIGHT(tobj);
    g = TRUE;
    
    if (quiver->value[3] + 1 > quiver->value[0])
    {
      full = TRUE;
      break;
    }
  }

  
  //GET_CARRYING_W(ch) += (int)(weight/2);
  equip_char(ch, quiver, i, FALSE);

  if (!g)
  {
    if (type == 1)
    {
      send_to_char("There are no arrows to gather here.\n", ch);
    }
    else if (type == 2)
    {
      send_to_char("There are no arrows to gather in that corpse.\n", ch);
    }
  }
  else if (g)
  {
    sprintf(buf, "You gathered %d arrow(s) into your quiver.\n", j);
    act("$n gathers up his arrows.&n", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(buf, ch);
  }
  if (full)
  {
    send_to_char("Your quiver is full.\n", ch);
  }
}

int arrow_spell_fire(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "&+RFlames &+rlick at $N's &+mwounds&+r as an arrow &+rcollides with $S &+Rflesh&+r!&n",
    "&+RFlames &+rlick at your &+mwounds&+r as an arrow &+rcollides with your &+Rflesh&+r!&n",
    "&+RFlames &+rlick at $N's &+mwounds&+r as an arrow &+rcollides with $S &+Rflesh&+r!&n",
    "&+rA burning feeling is $N's &+rfinal reward as an arrow rips through $S neck!&n",
    "&+rA burning feeling is your final reward as an arrow rips through your neck!&n",
    "&+rA burning feeling is $N's &+rfinal reward as an arrow rips through $S neck!&n", 0
  };

  dam = number(4, 16) * get_property("archery.enchantArrows.damage.mod", 1);

  return spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT, &messages);
}

int arrow_spell_lightning(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "&+WArcs of &+yelec&+Ytri&+ycity &+Wleap from an arrow &+Ysearing &+W$N's &+Wflesh!&n",
    "&+WArcs of &+yelec&+Ytri&+ycity &+Wleap from an arrow &+Ysearing &+Wyour &+Wflesh!&n",
    "&+WArcs of &+yelec&+Ytri&+ycity &+Wleap from an arrow &+Ysearing &+W$N's &+Wflesh!&n",
    "&+WArcing &+yelec&+Ytri&+ycity &+wstops &+W$N's &+Wheart, and $E falls to the ground, &+Ydead&+W.&n",
    "&+WArcing &+yelec&+Ytri&+ycity &+wstops &+Wyour heart, and you fall to the ground, &+Ydead&+W.&n",
    "&+WArcing &+yelec&+Ytri&+ycity &+wstops &+W$N's &+Wheart, and $E falls to the ground, &+Ydead&+W.&n", 0
  };

  dam = number(4, 16) * get_property("archery.enchantArrows.damage.mod", 1);

  return spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_NODEFLECT, &messages);
}

int arrow_spell_cold(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "&+CA bitter &+Bchill &+Cflows through $N &+Cas an arrow &+Bpenetrates &+C$S flesh!&n",
    "&+CA bitter &+Bchill &+Cflows through you as an arrow &+Bpenetrates &+Cyour flesh!&n",
    "&+CA bitter &+Bchill &+Cflows through $N &+Cas an arrow &+Bpenetrates &+C$S flesh!&n",
    "&+C$N's &+Cblood &+bfreezes&+C solid in $S veins as an arrow &+brips &+Cthrough $S neck!&n",
    "&+CYour blood &+bfreezes&+C solid in your veins as an arrow &+brips &+Cthrough your neck!&n",
    "&+C$N's &+Cblood &+bfreezes&+C solid in $S veins as an arrow &+brips &+Cthrough $S neck!&n", 0
  };

  dam = number(4, 16) * get_property("archery.enchantArrows.damage.mod", 1);

  return spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_NODEFLECT, &messages);
}

int arrow_spell_acid(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "&+GAcid&+g seeps into $N's &+mwounds&+g as an arrow &+Gpunctures &+g$S flesh!&n",
    "&+GAcid&+g seeps into your &+mwounds&+g as an arrow &+Gpunctures &+gyour flesh!&n",
    "&+GAcid&+g seeps into $N's &+mwounds&+g as an arrow &+Gpunctures &+g$S flesh!&n",
    "&+gThe &+Gsearing pain &+gfrom an arrow's &+Gacid &+gis the last thing $N feels.&n",
    "&+gThe &+Gsearing pain &+gfrom an arrow's &+Gacid &+gis the last thing you feel.&n",
    "&+gThe &+Gsearing pain &+gfrom an arrow's &+Gacid &+gis the last thing $N feels.&n", 0
  };

  dam = number(4, 16) * get_property("archery.enchantArrows.damage.mod", 1);

  return spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_NODEFLECT, &messages);
}

void event_enchant_arrow(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_obj arrow;

  arrow = read_object(obj->R_num, REAL);

  obj->name = arrow->name;
  obj->short_description = arrow->short_description;
  obj->timer[5] = ARROW_NONE;
}

int enchant_arrows(P_char ch, P_char vict, P_obj arrow, int cmd)
{
  int duration;
  char buf[256], buf2[256];

  if( !IS_ALIVE(ch) || !IS_ALIVE(vict) || !arrow )
  {
    return IS_ALIVE(ch) ? (IS_ALIVE(vict) ? DAM_NONEDEAD : DAM_VICTDEAD) :
      IS_ALIVE(vict) ? DAM_CHARDEAD : DAM_BOTHDEAD;
  }

  if( !IS_SET(arrow->extra2_flags, ITEM2_MAGIC) && arrow->condition > 70 )
  {
    SET_BIT(arrow->extra2_flags, ITEM2_MAGIC);
  }

  switch( cmd )
  {
    case ARROW_NONE:
      break;
    case ARROW_MARK:
      if( arrow->timer[5] == ARROW_MARK )
      {
        return DAM_NONEDEAD;
      }
      sprintf(buf2, "&n of &+L%s&n", GET_NAME(ch));
      sprintf(buf, "%s", arrow->short_description);
      strcat(buf, buf2);
      arrow->short_description = str_dup(buf);

      sprintf(buf, "%s a%s", arrow->name, GET_NAME(ch));
      arrow->name = str_dup(buf);

      arrow->timer[5] = ARROW_MARK;
      duration = WAIT_SEC * (int)get_property("archery.enchantArrows.mark.duration", 300);
      add_event(event_enchant_arrow, duration, 0, 0, arrow, 0, 0, 0);
      break;
    case 2:
      if( GET_CHAR_SKILL(ch, SKILL_ENCHANT_ARROWS) >= number(60, 180) )
      {
        return arrow_spell_fire(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      break;
    case 3:
      if( GET_CHAR_SKILL(ch, SKILL_ENCHANT_ARROWS) >= number(60, 180) )
      {
        return arrow_spell_cold(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      break;
    case 4:
      if( GET_CHAR_SKILL(ch, SKILL_ENCHANT_ARROWS) >= number(80, 180) )
      {
        return arrow_spell_lightning(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      break;
    case 5:
      if( GET_CHAR_SKILL(ch, SKILL_ENCHANT_ARROWS) >= number(80, 190) )
      {
        return arrow_spell_acid(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      break;
    case 6:
      if( GET_CHAR_SKILL(ch, SKILL_ENCHANT_ARROWS) > number(99, 199) )
      {
        spell_major_paralysis(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      break;
    default:
      break;
  }

  return IS_ALIVE(ch) ? (IS_ALIVE(vict) ? DAM_NONEDEAD : DAM_VICTDEAD) :
    IS_ALIVE(vict) ? DAM_CHARDEAD : DAM_BOTHDEAD;
}

/*
 * weapon values:
 * 0 - max speed
 * 1 - max strength (max range = str / strPerRoomRange)
 * 2 - bonus to damage
 * 3 - missile type used
 * missile values:
 * 1 -
 * 2 -
 * 3 - missile type
 * quiver values:
 * 1 -
 * 2 -
 * 3 - number of missiles inside
 */
// change properties archery.diceFactor to 1.0!!!
void do_fire(P_char ch, char *argument, int cmd)
{
  char tararg[256], dirarg[256];
  P_char victim, mount;
  P_obj weapon, quiver, missile, shield;
  bool should_retaliate = FALSE, is_fighting_check = FALSE;
  bool shield_blocked = FALSE;
  int i, j, dir = -1, to_hit, room, range, result;
  int shots, speed_per_shot;
  double dam = 0;
  int wallcheck = 0, weight = 0;
  int speed, strength, carrow, maxluck, actual;
  float delay;
  char buf[256];
  char vict_msg[256];
  char room_msg[256];
  char vict_death_msg[256];
  char room_death_msg[256];
  struct damage_messages *messages;
  struct damage_messages room_messages = {
   "Your $p's hit strikes $N!",
   "$n's $p's hit strikes you!",//"$p fired by $n hits you!",
   "$n's $p's hit strikes $N.",//$p fired by $n hits $N!",
   "Your $p went right through $N's throat killing $M instantly.",
   "$p fired by $n pierces your throat killing you instantly.",
   "$p fired by $n went right through $N's throat killing $M instantly.",
  };
  struct damage_messages range_messages = {
   "Your $p finds its mark as its%s hit strikes $N!",
   vict_msg,
   room_msg,
   "Your $p went right through $N's throat killing $M instantly.",
   vict_death_msg,
   room_death_msg,
  };
  struct damage_messages blocked_messages = {
   "Your $p's hit strikes $N's shield!",
   "$n's $p's hit strikes your shield!",
   "$n's $p's hit strikes $N's shield.",
   "Your $p went right through $N's throat killing $M instantly.",
   "$p fired by $n pierces your throat killing you instantly.",
   "$p fired by $n went right through $N's throat killing $M instantly.",
   0, 0
  };

  if( !IS_ALIVE(ch) )
  {
    logit(LOG_EXIT, "do_fire: bogus ch: '%s' %d.", (ch==NULL) ? "NULL" : J_NAME(ch),
      (ch==NULL) ? -1 : (IS_NPC(ch)) ? GET_VNUM(ch) : GET_PID(ch) );
    raise(SIGSEGV);
  }

  if( IS_DESTROYING(ch) )
  {
    send_to_char( "You're too busy destroying something.\n", ch );
    return;
  }

  weapon = ch->equipment[WIELD];

  if( !weapon )
  {
    send_to_char("You must wield the ranged weapon you want to fire.\n", ch);
    return;
  }

  if( GET_ITEM_TYPE(weapon) != ITEM_FIREWEAPON )
  {
    act( "What?  Maybe you should try &+Wthrowing&n $p instead?", FALSE, ch, weapon, ch, TO_CHAR );
    return;
  }

  // check quiver slot first...
  quiver = ch->equipment[WEAR_QUIVER];
  // If it exists, is a quiver, and has stuff in it, then we got a good one.
  if( quiver && GET_ITEM_TYPE(quiver) == ITEM_QUIVER && quiver->contains )
  {
    ;
  }
  // Otherwise, search through worn eq for a quiver in a weird slot (like on back).
  else
  {
    for( i = 0, quiver = NULL; i < MAX_WEAR; i++ )
    {
      if (ch->equipment[i])
      {
        if( GET_ITEM_TYPE(ch->equipment[i]) == ITEM_QUIVER && ch->equipment[i]->contains )
        {
          quiver = ch->equipment[i];
          break;
        }
      }
    }
  }

  if( !quiver )
  {
    send_to_char("You need to be wearing something with missiles.\n", ch);
    return;
  }

  missile = quiver->contains;
  if( missile->value[3] != weapon->value[3] )
  {
    sprintf( buf, "%s doesn't fit right in $p.", missile->short_description );
    act( buf, FALSE, ch, weapon, ch, TO_CHAR );
    return;
  }

   half_chop(argument, tararg, dirarg);
// victim = ParseTarget(ch, argument);
// if (!*tararg)
// {
//   send_to_char("Usage: fire <victim> [direction]\n", ch);
//   return;
// }

  // find target in range (by dir)
  if( *dirarg )
  {
    dir = dir_from_keyword(dirarg);
    if (dir == -1)
    {
      sprintf(buf, "'%s' is not a valid direction.\n", dirarg);
      send_to_char( buf, ch );
      return;
    }

    if( !(victim = get_char_ranged(tararg, ch, 10, dir)) )
    {
      sprintf(buf, "Could not find target '%s' to the %s.\n", tararg, dirs[dir]);
      send_to_char( buf, ch );
      return;
    }

    // If passed, than there's no wall, or we're going to arc, so check for walls directly.
    wallcheck = check_wall(ch->in_room, dir);
  }
  // Find target in room
  else if( !IS_FIGHTING(ch) )
  {
    victim = get_char_room_vis(ch, tararg);
    if( !victim )
    {
      send_to_char("You don't see them here.\n", ch);
      return;
    }
  }
  // Target who is fighting shooter
  else
  {
    victim = get_char_room_vis(ch, tararg);
    if( !victim )
    {
      if( !(victim = GET_OPPONENT(ch)) )
      {
        send_to_char("You don't see them here.\n", ch);
        return;
      }
    }
  }

  if( ch == victim )
  {
    send_to_char("Your mother would be so sad... \n", ch);
    return;
  }

  if( (world[victim->in_room].room_flags & SINGLE_FILE) && !AdjacentInRoom(ch, victim) )
  {
    if( victim->in_room != ch->in_room )
      send_to_char("It's too cramped in there to find a target.\n", ch);
    else
      send_to_char("It's too cramped in here to fire accurately.\n", ch);
    return;
  }

  if( GET_ZONE(victim) != GET_ZONE(ch) )
  {
    send_to_char("You just can't seem to get a good shot in...\n", ch);
    return;
  }

  // Lom: disabled mounted archery .. Umm... someone handled it actually, but ok.
  if( (mount = get_linked_char(ch, LNK_RIDING)) )
  {
    if( !GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT) && !is_natural_mount(ch, mount) )
    {
      send_to_char("I'm afraid you aren't quite up to mounted combat.\r\n", ch);
      act("$n quickly slides off $N's back.", TRUE, ch, 0, mount, TO_NOTVICT);
      stop_riding(ch);
    }
  }

  if( IS_FIGHTING(ch) )
  {
    // Max 100/130 -> about a 77% chance.
    if( notch_skill(ch, SKILL_POINT_BLANK_SHOT, 10)
      || number(1, 130) <= GET_CHAR_SKILL(ch, SKILL_POINT_BLANK_SHOT) )
    {
      send_to_char("You take aim, and fire upon your enemy!\n", ch);
      is_fighting_check = TRUE;
    }
    else
    {
       send_to_char("You're too busy fighting to fire your weapon!\n", ch);
       CharWait(ch, (int) (PULSE_VIOLENCE * 0.5));
       return;
    }
  }

  // Lets make firing arrows fraggable shall we?
  victim = misfire_check(ch, victim, DISALLOW_SELF | DISALLOW_BACKRANK);

  // Calculate number of shots and delay.
  // When speed increases, you fire faster until you gain extra arrow, then you fire at base pulse etc.
  // Value0 determines the fastest you can fire the weapon (where dex sets speed).
  speed = MIN(weapon->value[0], (int)((float)GET_C_DEX(ch)*1.5));
  // Skill can slow you down, but not speed you up past the dex/weapon limit.
  speed = MIN(speed, 2 * GET_CHAR_SKILL(ch, SKILL_ARCHERY));
  // Strength limit is value1.
  strength = MIN(weapon->value[1], GET_C_STR(ch));

  speed_per_shot = get_property("archery.speedPerShot", 30);

  if( IS_AFFECTED(ch, AFF_HASTE) )
  {
    speed += 20;
  }
  if (IS_AFFECTED3(ch, AFF3_BLUR))
  {
    speed += 20;
  }

  // Sharpshooter spec bonuses.
  if( GET_SPEC(ch, CLASS_ROGUE, SPEC_SHARPSHOOTER) )
  {
    j = get_property("archery.speedBonusSharpshooter", 20);
    i = GET_LEVEL(ch);
    // This formula equates to lvl 31:1, 36:2, 41:3, 46:4, 51:5, 56:6, 61:7
    //   Note: level 61 is an Imm level, but I wanted to include it just fyi.
    i = (i - 26) / 5;
    if( i > 0)
    {
      speed += i*j;
    }
    // At archery skill 70, we get another speed bonus.
    if( GET_CHAR_SKILL(ch, SKILL_ARCHERY) > 69 )
    {
      speed += j;
    }
  }

  if( IS_FIGHTING(ch) )
  {
    speed -=60;
  }

  // allow at least one shot total.
  speed = MAX(speed, speed_per_shot);

  // ok here we finally count number of shots in one fire action
  shots = speed / speed_per_shot;

  if( ch->in_room != victim->in_room )
  {
    if( dir == -1 )
    {
      send_to_char("You can't get a clear shot.\n", ch);
      return;
    }
    // calculate chance to hit when ranged shooting
    range = strength/get_property("archery.strPerRoomRange", 40);

    for( i = 1, room = ch->in_room; i <= range; i++ )
    {
      room = world[room].dir_option[dir]->to_room;
      if( room == victim->in_room )
      {
         break;
      }
    }

    // This causes an automatic miss.
    if( room != victim->in_room )
    {
      send_to_char("It's a long shot, but you try anyway!\n", ch);
    }

    to_hit = chance_to_hit(ch, victim, (int)(GET_CHAR_SKILL(ch, SKILL_ARCHERY) *
      get_property("archery.hitSkill.percentage", 0.7) - i * get_property("archery.rangePenalty", 10)), 0);
  }
  else
  {
    room = victim->in_room;
    to_hit = chance_to_hit(ch, victim, (int)(GET_CHAR_SKILL(ch, SKILL_ARCHERY) *
      get_property("archery.hitSkill.percentage", 0.7)), 0);
  }

  // If this is deemed overpowered, let's make it hit someone else if it fails instead of just missing.
  if( wallcheck )
  {
    act( "You aim high, arcing $p skyward.\r\n", FALSE, ch, weapon, ch, TO_CHAR );
    // A 77% chance at 100 skill.
    if( !notch_skill(ch, SKILL_INDIRECT_SHOT, 1)
      && GET_CHAR_SKILL(ch, SKILL_INDIRECT_SHOT) < number(1, 130) )
    {
      to_hit = 0;
    }
  }
  // Spell room effects that change accuracy (binding wind hurts, wind tunnel helps).
  if( get_spell_from_room(&world[ch->in_room], SPELL_BINDING_WIND) )
  {
    to_hit = (to_hit * get_property("spell.bindingWind.archery", 0.7));
  }
  if( get_spell_from_room(&world[ch->in_room], SPELL_WIND_TUNNEL) )
  {
    to_hit = (to_hit * get_property("spell.windTunnel.archery", 1.3));
  }


  actual = 0;
  // and finally shooting begins
  for( i = 0; i < shots; i++ )
  {
    if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    {
      break;
    }

    missile = quiver->contains;
    if( !missile )
    {
      send_to_char("Looks like you ran out of missiles.\n", ch);
      break;
    }

    // Cursed is auto return arrows
    if( notch_skill(ch, SKILL_CURSED_ARROWS, 10) || number(1, 100)
      <= (int)(GET_CHAR_SKILL(ch, SKILL_CURSED_ARROWS) * get_property("archery.cursedArrows.percentage", 0.5)) )
    {
      carrow = TRUE;
    }
    else
    {
      carrow = FALSE;
    }

    // For the gather command
    if( IS_PC(ch) )
    {
      missile->timer[0] = GET_PID(ch);
    }

    // For get all.playername
    if( number(1, 100) <= GET_CHAR_SKILL(ch, SKILL_ENCHANT_ARROWS)
      || notch_skill(ch, SKILL_ENCHANT_ARROWS, 10) )
    {
       enchant_arrows(ch, victim, missile, ARROW_MARK);
    }

    if( !carrow )
    {
       //GET_CARRYING_W(ch) -= GET_OBJ_WEIGHT(quiver);
       obj_from_obj(missile);
       //GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(quiver);
       weight += GET_OBJ_WEIGHT(missile);
       quiver->value[3]--;
    }

    // Immobile victims are guaranteed hits, if we have any chance (within range).
    // If we hit!
    if( room == victim->in_room && to_hit && (IS_IMMOBILE(victim)
      || notch_skill( ch, SKILL_ARCHERY, get_property("skill.notch.offensive.auto", 4) )
      || number(1, 100) <= to_hit) )
    {
      if( IS_PC(ch) && IS_PC(victim) )
      {
        affect_from_char( ch, TAG_PVPDELAY );
        set_short_affected_by(ch, TAG_PVPDELAY, WAIT_PVPDELAY);
        affect_from_char( victim, TAG_PVPDELAY );
        set_short_affected_by(victim, TAG_PVPDELAY, WAIT_PVPDELAY);
      }
      if( !affected_by_spell(ch, TAG_FIRING) )
      {
        set_short_affected_by(ch, TAG_FIRING, 5 * WAIT_SEC);
        // play_sound(SOUND_ARROW3, NULL, ch->in_room, TO_ROOM);
      }

      // Check for shield block.
      shield = victim->equipment[WEAR_SHIELD];
      if( shield && GET_ITEM_TYPE(shield) == ITEM_SHIELD && !IS_IMMOBILE(victim) && IS_AWAKE(victim) )
      {
        // Maxxed out char with 100 stats => 100 + 50 + 50 + 20 + 20 = 240
        int block_chance = (int)( GET_CHAR_SKILL(victim, SKILL_SHIELD_BLOCK) + (GET_C_AGI(victim) / 2)
          + (GET_CHAR_SKILL(victim, SKILL_SHIELD_COMBAT) / 2) + (GET_C_DEX(victim) / 5) + (GET_C_LUK(victim) / 5) );

        if( IS_FIGHTING(victim) )
        {
          block_chance /= 4;
        }
        debug("do_fire: shield block value %d.", block_chance);

        // We want around a 15% chance for 100 agi/dex & 100 skills: 240 / .15 = 1600
        if( number(1, 1600) <= block_chance )
        {
          shield_blocked = TRUE;

          if( ch->in_room != victim->in_room )
          {
            sprintf(vict_msg, "$p fired from %s%%s, but hits your shield!", dirs[rev_dir[dir]]);
            sprintf(vict_death_msg, "$p fired from %s goes through your throat killing you instantly.",
              dirs[rev_dir[dir]]);

            sprintf(room_msg, "$n fires $p %s!", dirs[dir]);
            act(room_msg, FALSE, ch, missile, ch, TO_NOTVICT | ACT_NOTTERSE);

            sprintf(room_msg, "$p fired from %s%%s, but hits $N's shield!", dirs[rev_dir[dir]]);
            sprintf(room_death_msg, "$p fired from %s went right through $N's throat killing $M instantly.",
              dirs[rev_dir[dir]]);

            messages = &range_messages;
          }
          else
          {
            messages = &blocked_messages;
          }
        }
      }

      // Ranged hit, no shield.
      if( ch->in_room != victim->in_room && !shield_blocked )
      {
        //play_sound(SOUND_ARROW3, NULL, victim->in_room, TO_ROOM);
        sprintf(room_msg, "$n fires $p %s!", dirs[dir]);
        act(room_msg, FALSE, ch, missile, ch, TO_NOTVICT | ACT_NOTTERSE);

        sprintf(vict_msg, "$p fired from %s%%s hits you!", dirs[rev_dir[dir]]);
        sprintf(vict_death_msg, "$p fired from %s goes through your throat killing you instantly.",
          dirs[rev_dir[dir]]);
        sprintf(room_msg, "$p fired from %s%%s hits $N!", dirs[rev_dir[dir]]);
        sprintf(room_death_msg, "$p fired from %s went right through $N's throat killing $M instantly.",
          dirs[rev_dir[dir]]);

        messages = &range_messages;
      }
      // Non-ranged hit.
      else if( !shield_blocked )
      {
        messages = &room_messages;
      }

      messages->obj = missile;
      messages->type = DAMMSG_TERSE | DAMMSG_HIT_EFFECT;

      // initial damage calculation by arrow dice
      dam = dice(missile->value[1], MAX(1, missile->value[2]));
      dam *= get_property("archery.diceFactor", 1.000);
      // aditional mods
      // damroll bonus: 35% of damroll
      dam += GET_DAMROLL(ch) * get_property("damroll.mod", 1.0) * get_property("archery.damrollFactor", 0.350);
      // hitroll bonus: 1/4 of hitroll
      dam += GET_HITROLL(ch) * get_property("archery.hitrollFactor", 0.250);
      // Minor mods by luck and dex.
      dam += number(0, GET_C_LUK(ch)/40) + number(0, GET_C_DEX(ch)/25);
      // Weapon bonus
      dam += weapon->value[2] * get_property("archery.weaponFactor", 0.500);
      // +0-4 damage at 100 skill.
      dam += number(0, GET_CHAR_SKILL(ch, SKILL_ARCHERY) / 25);

      // Low skill might reduce damage by 0 to 70%.
      if( GET_CHAR_SKILL(ch, SKILL_ARCHERY) < number(15, 40) )
      {
        dam = (dam * number(3, 10)) / 10.0;
      }

      // Apply damage mod for race/class/etc.
      dam *= ch->specials.damage_mod;

      // Binding wind decreases accuracy and damage done.
      if( get_spell_from_room(&world[ch->in_room], SPELL_BINDING_WIND) )
      {
        dam *= (get_property("spell.bindingWind.archery", 0.7));
      }
      // Wind tunnel helps accuracy and damage done.
      if( get_spell_from_room(&world[ch->in_room], SPELL_WIND_TUNNEL) )
      {
        dam *= (get_property("spell.windTunnel.archery", 1.3));
      }

      // High level well equipped archers can fire 7 shots a round. A high level halfling with good equipment
      // has approximately 200 luck. The critical shot skill + (luck -100) results to around 200. 200 out of 1000
      // is approximately 20 percent, which is incredibly high.
      // I have lowered the critical chance to 7 percent and lowered the damage bonus from 75 percent to 50 percent.
      // With maxxed skill and 200 luck:
      // 200 >= 1..3000 -> 200/1..3000 >= 1 -> 200/1..200 vs 200/201..3000 -> 200/3000 possibilities = 6.67%
      // 100 + (luck-100) -> luck / 1..3000 >= 1: (luck/30) / ((1..3000)/30) -> 1% for each 30 luck.
      //   So, for 90 luck: 3%, 180 luck: 6%, 210 luck: 7%, 270 luck: 9%.  Seems reasonable..
      if( GET_CHAR_SKILL(ch, SKILL_CRITICAL_SHOT) > 0
        && (GET_CHAR_SKILL(ch, SKILL_CRITICAL_SHOT) + GET_C_LUK(ch) - 100) >= number(1, 3000) )
      {
        send_to_char("&=LWYou score a CRITICAL SHOT!!!&N\n", ch);
        dam *= get_property("archery.crit.bonus", 1.500);
        notch_skill(ch, SKILL_CRITICAL_SHOT, 5);
      }

      // Cursed arrows don't take damage and auto-return.
      if( !carrow )
      {
        if( number(0, 1) )
        {
          missile->condition -= number(1, 5);
          if( missile->condition <= 0 )
          {
            MakeScrap(victim, missile);
          }
          else
          {
            obj_to_char(missile, victim);
          }
        }
        else
        {
          obj_to_char(missile, victim);
        }
      }

      // How can it be shield blocked without a shield?  But ok.
      // 10% chance to damage shield.
      if( shield_blocked && shield && !number(0, 9) && !IS_ARTIFACT(shield) )
      {
        shield->condition -= number(1, 3);
        send_to_char("Your shield is damaged!\r\n", victim);
        if( shield->condition < 1 )
        {
          MakeScrap(ch, victim->equipment[WEAR_SHIELD]);
        }
      }

      // lom: ever master if shooting !magic arrows, cannot hurt special magic beings
      // lom: same as with weapons, so get magic arrows
      if( CAN_HURT(ch, missile, victim) && !shield_blocked )
      {
        dam = BOUNDED(1, (int) dam, get_property("archery.arrow.max.damage", 100));
        result = melee_damage(ch, victim, (int)dam, PHSDAM_NOENGAGE | PHSDAM_NOSHIELDS | PHSDAM_NOPOSITION | PHSDAM_ARROW, messages);
      }
      // If the person isn't hurt-able or uses shield, we show messages, and result none dead.
      else
      {
        act(messages->attacker, FALSE, ch, messages->obj, victim, TO_CHAR | ACT_NOTTERSE);
        act(messages->victim, FALSE, ch, messages->obj, victim, TO_VICT | ACT_NOTTERSE);
        act(messages->room, FALSE, ch, messages->obj, victim, TO_NOTVICTROOM | ACT_NOTTERSE);
        result = DAM_NONEDEAD;
      }
      actual++;

      // Wakes up if hit and alive.
      if( result == DAM_NONEDEAD )
      {
        REMOVE_BIT(victim->specials.affected_by, AFF_SLEEP);
        if( affected_by_spell(victim, SPELL_SLEEP) )
        {
          affect_from_char(victim, SPELL_SLEEP);
        }
        // At level 56, we have major para arrows.  Otherwise, they just have fire/cold/lightning/acid.
        if( !shield_blocked )
        {
          result = enchant_arrows(ch, victim, missile, number(2, (GET_LEVEL(ch) >= MAXLVLMORTAL) ? 6 : 5));
        }
      }

      if( result != DAM_NONEDEAD )
      {
        should_retaliate = FALSE;
        break;
      }
      else
      {
        should_retaliate = TRUE;
      }
    }
    // So we missed, let them all enjoy nice miss messages
    else
    {
//      play_sound(SOUND_ARROW1, NULL, ch->in_room, TO_ROOM);
      act("You fire $p at $N and miss!", FALSE, ch, missile, victim, TO_CHAR | ACT_NOTTERSE);

      // Start with an arrow that was fired at an out of range target.
      if( (ch->in_room != victim->in_room) && (room != victim->in_room))
      {
        sprintf(buf, "$n lets $p fly %sward, but it falls far short of a target!", dirs[dir]);
        act(buf, FALSE, ch, missile, 0, TO_ROOM | ACT_NOTTERSE);

        if( world[room].people )
        {
//          play_sound(SOUND_ARROW1, NULL, room, TO_ROOM);
          if( room != ch->in_room )
          {
            sprintf(buf, "$p fired from %s drops to the ground.\n", dirs2[rev_dir[dir]]);
            act(buf, FALSE, world[room].people, missile, ch, TO_ROOM | ACT_NOTTERSE);
          }
        }
      }
      // Missing a ranged shot at a target that's in range.
      else if( ch->in_room != victim->in_room )
      {
//        play_sound(SOUND_ARROW1, NULL, victim->in_room, TO_ROOM);

        sprintf(buf, "$n fires $p %sward!", dirs[dir]);
        act(buf, FALSE, ch, missile, 0, TO_ROOM | ACT_NOTTERSE);

        sprintf(buf, "$p fired from %s misses you!", dirs2[rev_dir[dir]]);
        act(buf, FALSE, 0, missile, victim, TO_VICT | ACT_NOTTERSE);

        sprintf(buf, "$p fired from %s misses $N!", dirs2[rev_dir[dir]]);
        act(buf, FALSE, ch, missile, victim, TO_NOTVICTROOM | ACT_NOTTERSE);
      }
      // Missing with a shot at target in same room.
      else
      {
        act("$p fired by $n misses you!", FALSE, ch, missile, victim, TO_VICT | ACT_NOTTERSE);
        act("$p fired by $n misses $N!", FALSE, ch, missile, victim, TO_NOTVICT | ACT_NOTTERSE);
      }

      if( !carrow )
      {
        obj_to_room(missile, room);
      }
    }

    if( carrow )
    {
         act("$p &+mhums&n briefly before returning to you.", FALSE, ch, missile, victim, TO_CHAR | ACT_NOTTERSE);
    }
  } // End of for loop: shots in this one fire action

  sprintf(buf, "%sYou fire at $N.%s [&+R%d&n hits]",
    (IS_PC(ch) && IS_SET(ch->specials.act2, PLR2_BATTLEALERT)) ? "&+G-=[&n" : "",
    (IS_PC(ch) && IS_SET(ch->specials.act2, PLR2_BATTLEALERT)) ? "&+G]=-&n" : "", actual);
  act(buf, FALSE, ch, 0, victim, TO_CHAR | ACT_TERSE);

  sprintf(buf, "%s$n fires at you.%s [&+R%d&n hits]",
    (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_BATTLEALERT)) ? "&+R-=[&n" : "",
    (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_BATTLEALERT)) ? "&+R]=-&n" : "", actual);
   act(buf, FALSE, ch, 0, victim, TO_VICT | ACT_TERSE);

   sprintf(buf, "$n fires at $N. [&+R%d&n hits]", actual);

  if( victim->in_room != NOWHERE )
  {
    act(buf, FALSE, ch, 0, victim, TO_NOTVICTROOM | ACT_TERSE);

    if(victim->in_room != ch->in_room)
    {
      act(buf, FALSE, ch, 0, victim, TO_ROOM | ACT_TERSE);
    }
  }

  // WTH does this do?  Loss of weight of arrow?  What about cursed arrows?
  GET_CARRYING_W(ch) -= (int)((weight+(weight%2))/2);

  if( should_retaliate && IS_NPC(victim) )
  {
    MobRetaliateRange(victim, ch);
    /* Lom: should we agro others when range?
     * maybe. and probably can have some epic skill that reduces chance to range agro others
     * Dunno why this is commented out, but ok..
    if( ch->in_room != victim->in_room && (IS_PC(ch) || IS_PC_PET(ch)) )
    {
      P_char tmp_next;
      for( P_char tmpch = world[victim->in_room].people; tmpch; tmpch = tmp_next )
      {
        tmp_next = tmpch->next_in_room;
        if( IS_NPC(tmpch) )
        {
          MobRetaliateRange(tmpch, ch);
        }
        justice_witness(ch, tmpch, CRIME_ATT_MURDER);
        if( !char_in_list(ch) )
        {
          break;
        }
      }
    }
     */
  }

  // lag shooter (by number of shots done in this one fire action)
  delay = i / get_property("archery.lag.per.arrow.mod", 3.000);
/* I don't see a reason for this... why lag someone for a minimum amount?
 * If they killed in 1/7 arrows, let it be a 1/7 pulse.
  if( delay < 1 )
  {
    delay = 1;
  }
*/
  // aditional lag if shooting hidden
  if( IS_AFFECTED(ch, AFF_HIDE) )
  {
    delay *= get_property("archery.lag.hiding.mod", 2.4);
  }
  // aditional lag if shooting when fighting
  if( is_fighting_check )
  {
    delay *= get_property("archery.lag.isfighting.mod", 1.4);
  }

  // Delay is a multiplier for the combat pulse of ch.
  CharWait( ch, WAIT_SEC + delay * (int)ch->specials.base_combat_round );

  // Shadow archery allows one to stay hidden.
  if( IS_AFFECTED(ch, AFF_HIDE) )
  {
    if( notch_skill(ch, SKILL_SHADOW_ARCHERY, 7)
      || (GET_CHAR_SKILL(ch, SKILL_SHADOW_ARCHERY) / 2) > number(1, 105) )
     {
     }
     else
     {
debug( "Skill %d.", GET_CHAR_SKILL(ch, SKILL_SHADOW_ARCHERY) );
debug( "Skill %d.", GET_CHAR_SKILL(ch, SKILL_SHADOW_ARCHERY) / 2 );
       send_to_char("&+WOops, that wasn't too stealthy...\r\n&n", ch);
       REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
     }
   }
}

void event_poisoned_dart(P_char ch, P_char vict, P_obj obj, void *args)
{
 int      dam;
 int      poison;
 struct damage_messages messages = {
   "$N suddenly turns &+ggreen &nas your poison reaches $S &+Wvital &norgans.",
   "You suddenly feel &+gsick &nas $n's poison reaches your &+Wvital &norgans.",
   "$N suddenly turns &+ggreen &nas your poison reaches $S &+Wvital &norgans.",
   "$N suddenly turns &+Ggreen &nholds $S throat and vomits &+Rblood &nas $S soul leaves the body forever.",
   "A &+gsickening &nwave going up your throat is the last thing you feel..",
   "$N suddenly turns &+Ggreen &nholds $S throat and vomits &+Rblood &nas $S soul leaves the body forever.",
 };

 poison = *((int *) args);
 dam = 200 + 2 * poison + number(0, 30);

 if (IS_AFFECTED(vict, AFF_SLOW_POISON))
   dam = (int) (dam * 0.7);

 raw_damage(ch, vict, dam, RAWDAM_DEFAULT, &messages);
}

/*
* Throw a weapon
*
* THROW <weapon> <victim> [direction]
*/
void do_throw(P_char ch, char *argument, int cmd)
{
 char     arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 char     arg3[MAX_INPUT_LENGTH], argt[MAX_INPUT_LENGTH];
 char     buf[MAX_STRING_LENGTH];
 char     buf2[MAX_STRING_LENGTH];
 char     buf3[MAX_STRING_LENGTH];
 char     attacker[MAX_STRING_LENGTH];
 char     victim[MAX_STRING_LENGTH];
 char     room[MAX_STRING_LENGTH];
 char     death_attacker[MAX_STRING_LENGTH];
 char     death_victim[MAX_STRING_LENGTH];
 char     death_room[MAX_STRING_LENGTH];
 P_obj    weapon, t_obj;
 P_char   vict = NULL, tch;
 int      target_room = -1;
 int /*source_room = -1, */ victroom = -1;
 int      far_room = 0;
 int      max_range = 0;
 int      to_hit, dmg = 0;
 int      nb_attack, i;
 int      result;
 struct damage_messages messages = {
   attacker, victim, room,
   death_attacker, death_victim, death_room
 };

 half_chop(argument, arg1, argt);
 half_chop(argt, arg2, arg3);

 /*
  * arg1 = weapon arg2= victim arg3= direction
  */

 if (!*arg1 || !*arg2)
 {
   send_to_char("Usage: THROW <weapon> <victim> [direction]\r\n", ch);
   return;
 }

 if (IS_AFFECTED(ch, AFF_BOUND))
 {
   send_to_char("Your binds are too tight for that!\r\n", ch);
   return;
 }

 if (CHAR_IN_SAFE_ZONE(ch))
 {
   send_to_char("This room just has such a peaceful, easy feeling...\r\n",
                ch);
   return;
 }

 switch (world[ch->in_room].sector_type)
 {
 case SECT_WATER_PLANE:
 case SECT_UNDERWATER:
 case SECT_UNDERWATER_GR:
   send_to_char("One cannot throw this weapon underwater!\r\n", ch);
   return;
   break;
 }
 if (ch->specials.z_cord < 0)
 {
   send_to_char("One cannot throw this weapon underwater!\r\n", ch);
   return;
 }

 weapon = find_throw(ch, arg1, TRUE);
 if (!weapon)
   for (weapon = ch->carrying; weapon; weapon = weapon->next_content)
   {
     if (IS_DART(weapon) && isname(arg1, weapon->name))
       break;
   }

 if (!weapon)
 {
   sprintf(buf, "You're not wielding the %s!\r\n", arg1);
   send_to_char(buf, ch);
   return;
 }

 nb_attack = number_throw(ch, arg1);
 if (nb_attack == 0)
   return;

 if (*arg3)
 {                             /*
                                * attempt to fire direction x
                                */
   far_room = dir_from_keyword(arg3);
   if (far_room == -1)
   {
     send_to_char
       ("You have to specify a direction as one of north, east, west, east, NW, NE, SW, SE, D, U\r\n",
        ch);
     return;
   }

   max_range = IS_OBJ_STAT(weapon, ITEM_CAN_THROW2) ? 2 : 1;

   if ((vict = get_char_ranged(arg2, ch, max_range, far_room)))
     target_room = vict->in_room;
   else
   {
     send_to_char("Your target doesn't seem to be here.\r\n", ch);
     return;
   }
 }
 else
 {
   vict = get_char_room_vis(ch, arg2);
   if ((!*arg2) || (!vict))
   {
     act("Who are you trying to throw $p at?", FALSE, ch, weapon, 0,
         TO_CHAR);
     return;
   }
   target_room = ch->in_room;
 }

 if ((world[vict->in_room].room_flags & SINGLE_FILE) &&
     !AdjacentInRoom(ch, vict))
 {
   if (vict->in_room != ch->in_room)
     send_to_char("It's too cramped in there to find a target.\r\n", ch);
   else
     send_to_char("It's too cramped in here to throw accurately.\r\n", ch);
 }

 if (vict == ch)
 {
   send_to_char("That would be quite a feat, I'd like to see it done.\r\n",
                ch);
   return;
 }

 to_hit = chance_to_hit(ch, vict, GET_CHAR_SKILL(ch, SKILL_RANGE_WEAPONS), 0);
 to_hit += dex_app[STAT_INDEX(GET_C_DEX(ch))].miss_att * 5;

 if (GET_C_LUK(ch) / 2 > number(0, 100)) {
         to_hit = (int) (to_hit * 1.1);
       }

 strcpy(buf2, dirs[rev_dir[far_room]]);
 strcpy(buf3, dirs2[rev_dir[far_room]]);

  for (i = 1; i <= nb_attack; i++)
  {
    if (to_hit >= number(1, 100))
    {
      if (IS_PC(ch) && IS_PC(vict))
		  {
        affect_from_char( ch, TAG_PVPDELAY );
        set_short_affected_by(ch, TAG_PVPDELAY, WAIT_PVPDELAY);
        affect_from_char( vict, TAG_PVPDELAY );
        set_short_affected_by(vict, TAG_PVPDELAY, WAIT_PVPDELAY);
		  }

      sprintf(messages.attacker, "You hit $N with $p!");
      sprintf(messages.death_attacker,
             "Your skilfully thrown $p cuts right through $N's artery. $E tries to stop the &+rblood&n fountain but alas!");
      if (ch->in_room != vict->in_room)
      {
       sprintf(messages.victim, "$p thrown from %s hits you!", buf3);
       sprintf(messages.death_victim,
               "$p thrown from %s cuts right through your artery. You try to stop the &+rblood&n fountain but alas!",
               buf3);
       sprintf(buf, "$N throws $p %s!", dirs[far_room]);
       act(buf, FALSE, ch, weapon, ch, TO_NOTVICT);
       sprintf(messages.room, "$p thrown from %s hits $N!", buf3);
       sprintf(messages.death_room,
               "$p thrown from %s cuts right through $N's artery. $E tries to stop the &+rblood&n fountain but alas!",
               buf3);
     }
     else
     {
       sprintf(messages.victim, "$p thrown by $n hits you!");
       sprintf(messages.room, "$p thrown by $n hits $N!");
       sprintf(messages.death_room,
               "$p thrown by $n cuts right through $N's artery. $E tries to stop the &+rblood&n fountain but alas!");
       sprintf(messages.death_victim,
               "$p thrown by $n cuts right through your artery. You try to stop the &+rblood&n fountain but alas!");
     }

     dmg = dice(weapon->value[1], MAX(1, weapon->value[2]));
     dmg += TRUE_DAMROLL(ch);

     if (!CAN_HURT(ch, weapon, vict)) ;
     dmg = 1;

     victroom = vict->in_room;
     messages.obj = weapon;
     result =
       melee_damage(ch, vict, dmg, PHSDAM_NOENGAGE | PHSDAM_NOSHIELDS,
                    &messages);
     if (result == DAM_NONEDEAD)
     {
       if (IS_DART(weapon) && weapon->value[4] &&
           GET_CHAR_SKILL(ch, SKILL_DARTS) > number(0, 100))
       {
         add_event(event_poisoned_dart,
                   number(PULSE_VIOLENCE / 2, 2 * PULSE_VIOLENCE), ch, vict,
                   0, 0, &(weapon->value[4]), sizeof(int));
         weapon->value[4] = 0;
         break;
       }
     }
     else
       break;
   }
   else
   {
     sprintf(buf, "You throw $p at $N and miss!");
     act(buf, FALSE, ch, weapon, vict, TO_CHAR);
     if (ch->in_room != vict->in_room)
     {
       sprintf(buf, "$p thrown from %s misses you!", buf3);
       act(buf, FALSE, vict, weapon, ch, TO_CHAR);
       sprintf(buf, "$N throws $p %s!", dirs[far_room]);
       act(buf, FALSE, ch, weapon, ch, TO_NOTVICT);
       sprintf(buf, "$p thrown from %s misses $N!", buf3);
       act(buf, FALSE, vict, weapon, vict, TO_NOTVICT);
     }
     else
     {
       sprintf(buf, "$p thrown by $N misses you!");
       act(buf, FALSE, vict, weapon, ch, TO_CHAR);
       sprintf(buf, "$p thrown by $n misses $N!");
       act(buf, FALSE, ch, weapon, vict, TO_NOTVICT);
     }

     victroom = vict->in_room;
     remember(vict, ch);
     attack_back(ch, vict, TRUE);
   }

   if (IS_OBJ_STAT(weapon, ITEM_RETURNING))
   {
     sprintf(buf, "$p returns to your hand!");
     act(buf, FALSE, ch, weapon, vict, TO_CHAR);
     sprintf(buf, "$p returns to $N's hand!");
     act(buf, FALSE, vict, weapon, ch, TO_CHAR);
     if (ch->in_room != vict->in_room)
     {
       sprintf(buf, "$p flies from %s and returns to $n's hand!",
               dirs[far_room]);
       act(buf, FALSE, ch, weapon, ch, TO_NOTVICT);
       sprintf(buf, "$p flies %s!", buf3);
       act(buf, FALSE, vict, weapon, vict, TO_NOTVICT);
     }
     else
     {
       sprintf(buf, "$p returns to $n's hand!");
       act(buf, FALSE, ch, weapon, vict, TO_NOTVICT);
     }
   }
   else
     break;
 }

 if (!IS_OBJ_STAT(weapon, ITEM_RETURNING))
 {
   if (weapon == ch->equipment[PRIMARY_WEAPON])
     obj_to_char(unequip_char(ch, PRIMARY_WEAPON), ch);
   else if (weapon == ch->equipment[SECONDARY_WEAPON])
     obj_to_char(unequip_char(ch, SECONDARY_WEAPON), ch);

   obj_from_char(weapon);
   obj_to_room(weapon, (target_room == -1) ? ch->in_room : target_room);
 }


 if ((victroom != NOWHERE) && IS_PC(ch) && IS_NPC(vict))
 {
   for (vict = world[victroom].people; vict; vict = tch)
   {
     tch = vict->next_in_room;
     if (IS_NPC(vict))
     {
       remember(vict, ch);
       MobRetaliateRange(vict, ch);
     }
   }
 }

 CharWait(ch, PULSE_VIOLENCE);
}



void do_load_weapon(P_char ch, char *argument, int cmd)
{
 /*
  * arg1 = fire weapon arg2 = missiles
  */

 char     arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 P_obj    missile, weapon;
 int      num_needed = 0, num_ammo = 0;

 weapon = ch->equipment[HOLD];
 half_chop(argument, arg1, arg2);

 if (!*arg1 || !*arg2)
 {
   send_to_char("Usage: RELOAD <weapon> <ammo>\r\n", ch);
   return;
 }
 if (!weapon)
 {
   send_to_char("You must hold the weapon to load it.\r\n", ch);
   return;
 }
 if (GET_ITEM_TYPE(weapon) != ITEM_FIREWEAPON)
 {
   send_to_char("That item doesn't use ammunition!\r\n", ch);
   return;
 }
 missile = get_obj_in_list_vis(ch, arg2, ch->carrying);
 if (!missile)
 {
   send_to_char("What are you trying to use as ammunition?\r\n", ch);
   return;
 }
 if (GET_ITEM_TYPE(missile) != ITEM_MISSILE)
 {
   send_to_char("That isn't ammunition!\r\n", ch);
   return;
 }
 if (missile->value[3] != weapon->value[3])
 {
   send_to_char("The ammunition won't fit in the weapon!\r\n", ch);
   return;
 }
 num_needed = weapon->value[1] - weapon->value[2];
 if (!num_needed)
 {
   send_to_char("It's already fully loaded.\r\n", ch);
   return;
 }
 num_ammo = missile->value[2];
 if (!num_ammo)
 {
   /*
    * shouldn't really get here.. this one's for Murphy :)
    */
   send_to_char("It's empty!\r\n", ch);
   extract_obj(missile, TRUE); // Arti bullets?
   return;
 }
 if (num_ammo <= num_needed)
 {
   weapon->value[2] += num_ammo;
   extract_obj(missile, TRUE); // Arti bullets?
 }
 else
 {
   weapon->value[2] += num_needed;
   missile->value[2] -= num_needed;
 }
 act("You load $p", FALSE, ch, weapon, 0, TO_CHAR);
 act("$n loads $p", FALSE, ch, weapon, 0, TO_ROOM);


}


/* fonction to scan for specific type and return direction+1 if found else return 0*/

int range_scan(P_char ch, P_char target, int distance, int type_scan)
{
 int      i, door;
 int      source_room, target_room;
 P_char   t_ch, vict;

 if (!ch)
   return -1;

 if (distance < 1)
   return -1;

 source_room = ch->in_room;

 for (door = 0; door < NUM_EXITS; door++)
 {
   for (i = 1; i <= distance; i++)
   {
     if (EXIT(ch, door))
     {
       if (CAN_GO(ch, door) &&
           !check_wall(ch->in_room, door) &&
           !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB) &&
           !IS_SET(world[EXIT(ch, door)->to_room].room_flags, MAGIC_DARK))
       {
         target_room = world[ch->in_room].dir_option[door]->to_room;
         ch->in_room = target_room;

         switch (type_scan)
         {
         case SCAN_TARGET:
           if (!target)
             return -1;
           vict = get_char_room_vis(ch, J_NAME(target));
           if (vict)
           {
             ch->in_room = source_room;
             return door;
           }
           break;
         case SCAN_RANGE_TARGET:
           break;
         case SCAN_EVILRACE:
           LOOP_THRU_PEOPLE(t_ch, ch)
           {
             if (IS_PC(t_ch) && RACE_EVIL(t_ch))
             {
               ch->in_room = source_room;
               return door;
             }
           }
           break;
         case SCAN_GOODRACE:
           LOOP_THRU_PEOPLE(t_ch, ch)
           {
             if (IS_PC(t_ch) && RACE_GOOD(t_ch))
             {
               ch->in_room = source_room;
               return door;
             }
           }
           break;
         case SCAN_COMBAT:
           LOOP_THRU_PEOPLE(t_ch, ch)
           {
             if (IS_FIGHTING(t_ch))
             {
               ch->in_room = source_room;
               return door;
             }
           }
           break;
         case SCAN_ANY:
           LOOP_THRU_PEOPLE(t_ch, ch)
           {
             if (IS_PC(t_ch))
             {
               ch->in_room = source_room;
               return door;
             }
           }
           break;
         }
       }
     }
     else
       break;
   }
   ch->in_room = source_room;
 }
 ch->in_room = source_room;
 return -1;
}

int range_scan_track(P_char ch, int distance, int type_scan)
{
 int      i, door;
 int      source_room, target_room;
 P_char   t_ch, vict;

 if (!ch)
   return FALSE;

 if (distance < 1)
   return FALSE;

 source_room = ch->in_room;

 for (door = 0; door < NUM_EXITS; door++)
 {
   for (i = 1; i <= distance; i++)
   {
     if (EXIT(ch, door))
     {
       if (CAN_GO(ch, door) &&
           !check_wall(ch->in_room, door) &&
           !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB) &&
           !IS_SET(world[EXIT(ch, door)->to_room].room_flags, MAGIC_DARK))
       {
         target_room = world[ch->in_room].dir_option[door]->to_room;
         ch->in_room = target_room;

         switch (type_scan)
         {
         case SCAN_EVILRACE:
           LOOP_THRU_PEOPLE(t_ch, ch)
           {
             if (IS_PC(t_ch) && RACE_EVIL(t_ch))
             {
               remember(ch, t_ch);
               ch->in_room = source_room;
               return TRUE;
             }
           }
           break;
         case SCAN_GOODRACE:
           LOOP_THRU_PEOPLE(t_ch, ch)
           {
             if (IS_PC(t_ch) && RACE_GOOD(t_ch))
             {
               remember(ch, t_ch);
               ch->in_room = source_room;
               return TRUE;
             }
           }
           break;
         case SCAN_COMBAT:
           LOOP_THRU_PEOPLE(t_ch, ch)
           {
             if (IS_FIGHTING(t_ch))
             {
               remember(ch, t_ch);
               ch->in_room = source_room;
               return TRUE;
             }
           }
           break;
         case SCAN_ANY:
           LOOP_THRU_PEOPLE(t_ch, ch)
           {
             if (IS_PC(t_ch))
             {
               remember(ch, t_ch);
               ch->in_room = source_room;
               return TRUE;
             }
           }
           break;
         }
       }
     }
     else
       break;
   }
   ch->in_room = source_room;
 }
 ch->in_room = source_room;
 return FALSE;
}

/* fonction to check if mob can use range attack */

bool mob_can_range_att(P_char ch, P_char victim)
{
 int      spell_attack = FALSE;
 int      direction, /*temp, */ distance;
 int      lvl = 0, spl = 0;
 char     buf[256];

 buf[0] = '\0';

 /* ok first find where the target is and how far */

 direction = range_scan(ch, victim, 3, SCAN_TARGET);
 if (direction < 0)
 {
   direction = range_scan(ch, victim, 2, SCAN_TARGET);
   if (direction < 0)
   {
     direction = range_scan(ch, victim, 1, SCAN_TARGET);
     if (direction < 0)
     {
       return FALSE;
     }
     else
       distance = 1;
   }
   else
     distance = 2;
 }
 else
   distance = 3;

 /* ok first we check if able to cast some nice range spell  */

 lvl = GET_LEVEL(ch);

 /* should check for best spell, rather than going by class, but fuck
    it */

 if (GET_CLASS(ch, CLASS_SORCERER))
 {
   if (!spl && npc_has_spell_slot(ch, SPELL_CHAIN_LIGHTNING))
     spl = SPELL_CHAIN_LIGHTNING;
   if (!spl && npc_has_spell_slot(ch, SPELL_ICE_STORM))
     spl = SPELL_ICE_STORM;
/*
     if (!spl && npc_has_spell_slot(ch, SPELL_COLOR_SPRAY))
       spl = SPELL_COLOR_SPRAY;
*/
   if (!spl && npc_has_spell_slot(ch, SPELL_FIREBALL))
     spl = SPELL_FIREBALL;
   if (!spl && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT))
     spl = SPELL_LIGHTNING_BOLT;
/*
     if (!spl && npc_has_spell_slot(ch, SPELL_MAGIC_MISSILE))
       spl = SPELL_MAGIC_MISSILE;
*/
 }

 if (GET_CLASS(ch, CLASS_NECROMANCER))
 {
   if (!spl && npc_has_spell_slot(ch, SPELL_ICE_STORM))
     spl = SPELL_ICE_STORM;
/*
     if (!spl && npc_has_spell_slot(ch, SPELL_MAGIC_MISSILE))
       spl = SPELL_MAGIC_MISSILE;
*/
 }

 if( GET_CLASS(ch, CLASS_CONJURER) || GET_CLASS(ch, CLASS_SUMMONER) )
 {
   if (!spl && npc_has_spell_slot(ch, SPELL_ICE_STORM))
     spl = SPELL_ICE_STORM;
   if (!spl && npc_has_spell_slot(ch, SPELL_COLOR_SPRAY))
     spl = SPELL_COLOR_SPRAY;
   if (!spl && npc_has_spell_slot(ch, SPELL_FIREBALL))
     spl = SPELL_FIREBALL;
   if (!spl && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT))
     spl = SPELL_LIGHTNING_BOLT;
/*
     if (!spl && npc_has_spell_slot(ch, SPELL_MAGIC_MISSILE))
       spl = SPELL_MAGIC_MISSILE;
*/
 }

 if (GET_CLASS(ch, CLASS_SHAMAN))
 {
   if (!spl && npc_has_spell_slot(ch, SPELL_ARIEKS_SHATTERING_ICEBALL))
     spl = SPELL_ARIEKS_SHATTERING_ICEBALL;
/*
     if (!spl && npc_has_spell_slot(ch, SPELL_MOLTEN_SPRAY))
       spl = SPELL_MOLTEN_SPRAY;
     if (!spl && npc_has_spell_slot(ch, SPELL_ICE_MISSILE))
       spl = SPELL_ICE_MISSILE;
*/
 }

 if (GET_CLASS(ch, CLASS_CLERIC))
 {
   if (!spl && npc_has_spell_slot(ch, SPELL_FLAMESTRIKE))
     spl = SPELL_FLAMESTRIKE;
 }

 if (GET_CLASS(ch, CLASS_DRUID) || (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))
 {
   if (!spl && npc_has_spell_slot(ch, SPELL_CYCLONE))
     spl = SPELL_CYCLONE;
   if (!spl && npc_has_spell_slot(ch, SPELL_FIRESTORM))
     spl = SPELL_FIRESTORM;
   if (!spl && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT))
     spl = SPELL_LIGHTNING_BOLT;
 }

 if (GET_CLASS(ch, CLASS_BARD))
 {
   if (!spl && npc_has_spell_slot(ch, SPELL_FIREBALL))
     spl = SPELL_FIREBALL;
/*
     if (!spl && npc_has_spell_slot(ch, SPELL_COLOR_SPRAY))
       spl = SPELL_COLOR_SPRAY;
*/
   if (!spl && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT))
     spl = SPELL_LIGHTNING_BOLT;
/*
     if (!spl && npc_has_spell_slot(ch, SPELL_MAGIC_MISSILE))
       spl = SPELL_MAGIC_MISSILE;
*/
 }

 if (GET_CLASS(ch, CLASS_RANGER))
 {
   if (!spl && npc_has_spell_slot(ch, SPELL_FIRESTORM))
     spl = SPELL_FIRESTORM;
   if (!spl && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT))
     spl = SPELL_LIGHTNING_BOLT;
 }

 if (spl && ch && victim)
   return (MobCastSpell(ch, victim, 0, spl, lvl));

 /* check if range weapon on me */
 if (!spell_attack)
 {
   /* hmm, looks like a good spot. I didn't cast a range spell,
      so lemme try to get my z_cord the same */
   /* Flying is no escape! */
   if (ch->specials.z_cord < victim->specials.z_cord)
   {
     if (!IS_AFFECTED(ch, AFF_FLY))
     {
       if (npc_has_spell_slot(ch, SPELL_FLY))
         MobCastSpell(ch, ch, 0, SPELL_FLY, GET_LEVEL(ch));
       else if (npc_has_spell_slot(ch, SPELL_RAVENFLIGHT))
         MobCastSpell(ch, ch, 0, SPELL_RAVENFLIGHT, GET_LEVEL(ch));
       else if (npc_has_spell_slot(ch, SPELL_GREATER_RAVENFLIGHT))
         MobCastSpell(ch, ch, 0, SPELL_GREATER_RAVENFLIGHT, GET_LEVEL(ch));
     }
     else
     {
       strcat(buf, "up");
       do_fly(ch, buf, 0);
     }
   }
   else if (ch->specials.z_cord > victim->specials.z_cord)
   {
     strcat(buf, "down");
     do_fly(ch, buf, 0);
   }
 }

 return FALSE;
}

P_obj find_throw(P_char ch, char *name, int first)
{
 P_obj    i;
 char     tmpname[MAX_STRING_LENGTH];
 char    *tmp;

 if (!name || !*name)
   return NULL;

 strcpy(tmpname, name);
 tmp = tmpname;

 i = ch->equipment[PRIMARY_WEAPON];
 if (i)
 {
   if (isname(tmp, i->name))
     if (CAN_SEE_OBJ(ch, i) || IS_NOSHOW(i))
       if ((i) &&
           ((IS_OBJ_STAT(i, ITEM_CAN_THROW1)) ||
            (IS_OBJ_STAT(i, ITEM_CAN_THROW2))))
         return (i);
 }
/* THIS CRASHES US LEFT EM ONLY CAST PRIME WEAPONS FOR NOW - Kvark
 i = ch->equipment[SECONDARY_WEAPON];
 if (i) {
   if (isname (tmp, i->name))
     if (CAN_SEE_OBJ (ch, i) || IS_NOSHOW (i))
       if ((i) && ((IS_OBJ_STAT2(i, ITEM2_CAN_THROW1)) || (IS_OBJ_STAT2(i, ITEM2_CAN_THROW2))))
         return (i);
 }
*/

 if (first == FALSE)
 {
   for (i = ch->carrying; i; i = i->next_content)
     if (isname(tmp, i->name))
       if (CAN_SEE_OBJ(ch, i) || IS_NOSHOW(i))
         return (i);
 }

 return NULL;
}

/* fonction to find the number of attack for throwable weapon */

int number_throw(P_char ch, char *name)
{
 P_obj    i;
 char     tmpname[MAX_STRING_LENGTH];
 char    *tmp;
 int      nb_att = 0;
 int      primary = 0, secondary = 0;
 int      pri_ret = 0, sec_ret = 0;
 int      hAtt = 0, dW = 0;

 if (!name || !*name)
   return 0;

 strcpy(tmpname, name);
 tmp = tmpname;

 i = ch->equipment[PRIMARY_WEAPON];
 if (i)
   if (isname(tmp, i->name))
     if (CAN_SEE_OBJ(ch, i) || IS_NOSHOW(i))
       if (IS_OBJ_STAT(i, ITEM_CAN_THROW1) ||
           IS_OBJ_STAT(i, ITEM_CAN_THROW2))
         if (IS_OBJ_STAT(i, ITEM_RETURNING))
         {
           primary = 1;
           pri_ret = 1;
         }
         else
           primary = 1;

 i = ch->equipment[SECONDARY_WEAPON];
 if (i)
   if (isname(tmp, i->name))
     if (CAN_SEE_OBJ(ch, i) || IS_NOSHOW(i))
       if (IS_OBJ_STAT(i, ITEM_CAN_THROW1) ||
           IS_OBJ_STAT(i, ITEM_CAN_THROW2))
         if (IS_OBJ_STAT(i, ITEM_RETURNING))
         {
           secondary = 1;
           sec_ret = 1;
         }
         else
           secondary = 1;

 if (!primary && !secondary)
 {
   return 1;
 }

 nb_att = 1;
 hAtt = (IS_AFFECTED(ch, AFF_HASTE)) ? 1 : 0;

 if (primary && secondary)
 {
   nb_att += (hAtt && (GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) > 50)) ? 1 : 0;
 }

 nb_att += hAtt + dW;

 if (primary)
 {
   nb_att +=
     GET_CHAR_SKILL(ch, SKILL_DOUBLE_ATTACK) > number(0, 100) ? 1 : 0;
   nb_att +=
     GET_CHAR_SKILL(ch, SKILL_TRIPLE_ATTACK) > number(0, 100) ? 1 : 0;
 }

 if (IS_AFFECTED2(ch, AFF2_SLOW))
 {
   nb_att /= 2;
   nb_att = MAX(1, nb_att);
 }

 return nb_att;
}

/* function to see if there is a wall in the room blocking the direction */

int check_wall(int room, int direction)
{

 P_obj    tobj, next;
 int      dir, obj_num;

 for (tobj = world[room].contents; tobj; tobj = next)
 {
   next = tobj->next_content;

   obj_num = (tobj->R_num >= 0) ? obj_index[tobj->R_num].virtual_number : 0;

   if ((obj_num >= 753) && (obj_num <= 767))
   {
     dir = tobj->value[1];
     if (direction == dir)
       return TRUE;
   }
 }
 return FALSE;
}

/* returns true if there is a wall in given direction and character can see it. */

int check_visible_wall(P_char ch, int direction)
{

 P_obj    tobj, next;
 int      dir, obj_num, room;

 room = ch->in_room;
 
 for (tobj = world[room].contents; tobj; tobj = next)
 {
   next = tobj->next_content;

   obj_num = (tobj->R_num >= 0) ? obj_index[tobj->R_num].virtual_number : 0;

   if ((obj_num >= 753) && (obj_num <= 767))
   {
     dir = tobj->value[1];
     if (direction == dir && CAN_SEE_OBJ(ch, tobj))
       return TRUE;
   }
 }
 return FALSE;
}

/* returns pointer to a wall in given direction from character, assuming there is one */

P_obj get_wall_dir(P_char ch, int dir)
{
 P_obj    tobj, next;
 int      wall_dir, obj_num, room;

 room = ch->in_room;
 
 for (tobj = world[room].contents; tobj; tobj = next)
 {
   next = tobj->next_content;

   obj_num = (tobj->R_num >= 0) ? obj_index[tobj->R_num].virtual_number : 0;

   if ((obj_num >= 753) && (obj_num <= 767))
   {
     wall_dir = tobj->value[1];
     if (wall_dir == dir)
       return tobj;
   }
 }
 return FALSE;
}

/* to take cover when someone fire from above */
void do_cover(P_char ch, char *argument, int cmd)
{

 int      chance;
 struct affected_type af;
 char     buf[MAX_STRING_LENGTH];

 if (!ch)
   return;

 if (IS_AFFECTED3(ch, AFF3_COVER))
 {
   send_to_char("You're already hiding from flying people.\r\n", ch);
   return;
 }

 if (IS_RIDING(ch))
 {
   send_to_char("Better dismount first!\r\n", ch);
   return;
 }

 if (ch->specials.z_cord > 0)
 {
   send_to_char("Cannot hide when you fly.\r\n", ch);
   return;
 }

 if (IS_SET(world[(ch)->in_room].room_flags, INDOORS))
 {
   send_to_char("Not very usefull inside.\r\n", ch);
   return;
 }

 switch (world[ch->in_room].sector_type)
 {
 case SECT_INSIDE:
 case SECT_CITY:
 case SECT_FOREST:
 case SECT_UNDRWLD_CITY:
 case SECT_UNDRWLD_INSIDE:
   chance = 99;
   break;
 case SECT_HILLS:
 case SECT_MOUNTAIN:
 case SECT_EARTH_PLANE:
 case SECT_SWAMP:
 case SECT_UNDRWLD_MOUNTAIN:
 case SECT_UNDRWLD_LOWCEIL:
 case SECT_UNDRWLD_MUSHROOM:
   chance = 90;
   break;
 case SECT_UNDRWLD_WILD:
 case SECT_ETHEREAL:
 case SECT_ASTRAL:
 case SECT_FIREPLANE:
 case SECT_LAVA:
   chance = 50;
   break;
 case SECT_FIELD:
 case SECT_DESERT:
 case SECT_ARCTIC:
   chance = 25;
   break;
 case SECT_WATER_PLANE:
 case SECT_WATER_SWIM:
 case SECT_WATER_NOSWIM:
 case SECT_NO_GROUND:
 case SECT_OCEAN:
 case SECT_AIR_PLANE:
 case SECT_UNDERWATER:
 case SECT_UNDERWATER_GR:
 case SECT_UNDRWLD_WATER:
 case SECT_UNDRWLD_NOSWIM:
 case SECT_UNDRWLD_NOGROUND:
 case SECT_UNDRWLD_LIQMITH:
 case SECT_UNDRWLD_SLIME:
   chance = 1;
   break;
 }
 if (ch->specials.z_cord)
   chance = 1;

 if (number(1, 100) <= chance)
 {
   sprintf(buf, "You take cover!");
   act(buf, FALSE, ch, 0, 0, TO_CHAR);
   sprintf(buf, "$n take cover!");
   act(buf, FALSE, ch, 0, 0, TO_ROOM);

   bzero(&af, sizeof(af));
   af.duration = 5;
   af.bitvector3 = AFF3_COVER;
   affect_to_char(ch, &af);

 }
 else
 {
   sprintf(buf, "Panic! You cannot find anywhere to take cover!");
   act(buf, FALSE, ch, 0, 0, TO_CHAR);
   sprintf(buf, "$n runs in circle trying to take cover!");
   act(buf, FALSE, ch, 0, 0, TO_ROOM);
 }
}

void return_home(P_char ch, P_char victim, P_obj obj, void *data)
{

 P_nevent  ev;
 hunt_data h_data;

 if( !IS_ALIVE(ch) )
   return;

 if (IS_PC(ch))
   return;

 if (real_room(ch->player.birthplace) == NOWHERE)
   return;

 if (!IS_SET(ch->specials.act, ACT_SENTINEL) && !IS_SHOPKEEPER(ch))
   return;

 if (world[ch->in_room].number == GET_BIRTHPLACE(ch))
   return;

 LOOP_EVENTS_CH(ev, ch->nevents)
 {
   if (ev->func == mob_hunt_event)
   {
     add_event(return_home, 30, ch, 0, 0, 0, 0, 0);
     return;
   }
 }

 if (IS_FIGHTING(ch))
 {
   add_event(return_home, 30, ch, 0, 0, 0, 0, 0);
   return;
 }

 h_data.hunt_type = HUNT_JUSTICE_SPECROOM;
 h_data.targ.room = real_room(GET_BIRTHPLACE(ch));
 add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &h_data, sizeof(hunt_data));
 //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, h_data);
 add_event(return_home, 30, ch, 0, 0, 0, 0, 0);
}
