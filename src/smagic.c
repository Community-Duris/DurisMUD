
/*
 * ***************************************************************************
 * *  File: shaman_magic.c                                     Part of Duris *
 * *  Usage: procedures to create spell affects for shamans                  *
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.  *
 * * *  Copyright 1994 - 2008 - Duris Systems Ltd.                           *
 * *                                                                         *
 * ***************************************************************************
 */

/* shaman spells written by RMG/Tavril, btw...  enjoy! */
// Shaman spells updated by Lucrot Jun09

#ifndef _SHAMAN_MAGIC_C_

#   include <stdio.h>
#   include <string.h>
#   include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "structs.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "damage.h"
#include "disguise.h"
#include "graph.h"
#include "ctf.h"
#include "achievements.h"

/*
 * external variables
 */

extern P_char character_list;
extern P_char combat_list;
extern P_obj object_list;
extern P_index obj_index;
extern P_room world;
extern const int top_of_world;
extern P_index mob_index;
extern const char *apply_types[];
extern const flagDef extra_bits[];
extern const char *item_types[];
extern const struct stat_data stat_factor[];
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int hometown[];
extern struct time_info_data time_info;
extern struct weather_data weather_info;
extern struct zone_data *zone_table;
extern Skill skills[];
extern int cast_as_damage_area(P_char,
                               void (*func) (int, P_char, char *, int, P_char,
                                             P_obj), int, P_char, float,
                               float);
extern int apply_ac(P_char ch, int eq_pos);
extern void affect_remove(P_char, struct af *);
extern struct str_app_type str_app[];
extern struct con_app_type con_app[];
extern struct wis_app_type wis_app[];

// Spirit and soul type damage spells are modified by the following.
int shaman_shield_damage_adjustments(P_char ch, P_char victim, int dam)
{
  if(IS_AFFECTED2(victim, AFF2_SOULSHIELD))
  {
    send_to_char("&+WYour victim's soul is magically protected causing your spell to do less damage.\r\n", ch);
    return (int) (dam * 0.75);
  }
  
  if(IS_AFFECTED3(victim, AFF3_GR_SPIRIT_WARD))
  {
    send_to_char("&+WYour victim's spirit is greatly protected causing your spell to do less damage.\r\n", ch);
    return (int) (dam * 0.60);
  }
  
  if(IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
  {
    send_to_char("&+LYour victim's soul is exposed causing your spell to do more damage!\r\n", ch);
    return (int) (dam * 1.5);
  }
  return dam;
}

// Shaman spell saves are mostly wisdom based.
int shaman_wis_save_result(P_char ch, P_char victim)
{
  int level = GET_LEVEL(ch);
  int save = victim->specials.apply_saving_throw[SAVING_SPELL];
  
  if(IS_ANIMALIST(ch))
    level += number(10, (int)(GET_LEVEL(ch) / 2));

  return BOUNDED( 0,
          (int)(GET_C_WIS(ch) - (int)(GET_C_WIS(victim) * 0.5) +
          level - GET_LEVEL(victim) + save),
          120) + number(-10, 10);
}

// Use do_point() for most shaman spells.
void do_point(P_char ch, P_char victim)
{
  if(ch != victim)
  {
    act("$n&n points at $N&n.", TRUE, ch, 0, victim, TO_NOTVICT);
    act("You point at $N&n.", TRUE, ch, 0, victim, TO_CHAR);
    act("$n&n points at you.", TRUE, ch, 0, victim, TO_VICT);
  }
}

// Guardian spirit messages that is called from combat related functions.
void guardian_spirits_messages(P_char ch, P_char victim)
{
  act("&+rA spectre of something long gone rises over&n $N &+rin a foreboding &+mmist.&n",
    FALSE, ch, 0, victim, TO_CHAR);
  act("&+rYour forefathers manifest themselves in an attempt to protect you!&n",
    FALSE, ch, 0, victim, TO_VICT);
  act("&+rA spectre of something long gone rises over&n $N &+rin a foreboding &+mmist.&n",
    FALSE, ch, 0, victim, TO_NOTVICT);
}

// Only specialized shamans may cast guardian spirits.
void spell_guardian_spirits(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  int duration;
  
  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim) ||
     !GET_CLASS(ch, CLASS_SHAMAN))
      return;
  
  if(ch != victim)
  {
    send_to_char("&+rYou may not call upon the forefathers of others.\r\n", ch);
    return;
  }
  
  if(affected_by_spell(victim, SPELL_GUARDIAN_SPIRITS))
  {
    send_to_char("&+rYou call upon the spirits of your forefathers to continue watching over you.\r\n", ch);
    
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_GUARDIAN_SPIRITS)
      {
        af1->duration = MAX(4, level / 2 + number(-8, 0));
      }
    }
    return;
  }
  
  act("&+rYour spell summons forth the spirits of your forefathers who will now try to protect you from harm.&n",
    FALSE, ch, 0, victim, TO_CHAR);
  act("&+rGhostly spirits rise from the ground swirling around $N as $E finishes $S long incantation!",
    FALSE, ch, 0, victim, TO_ROOM);
    
  if(IS_PC(ch) ||
     IS_PC_PET(ch) ||
     IS_PC(victim) ||
     IS_PC_PET(victim))
      duration = MAX(4, level / 2 + number(-4, 0));
  else
    duration = 100;
    
  memset(&af, 0, sizeof(af));
  af.type = SPELL_GUARDIAN_SPIRITS;
  af.duration =  duration;

  af.modifier = -1 * number(150, 200);
  af.location = APPLY_AC;
  affect_to_char(victim, &af);
}
  
void essence_broken(struct char_link_data *cld)
{
  act("&+yYour essence leaves $N and returns to your body.&n",
    FALSE, cld->linking, 0, cld->linked, TO_CHAR);
  act("&+yThe essence of the wolf leaves your body.&n",
    FALSE, cld->linking, 0, cld->linked, TO_VICT);
}

void spell_essence_of_the_wolf(int level, P_char ch, char *arg, int type,
                               P_char victim, P_obj obj)
{
  if(!ch || !victim || ch == victim)
    return;

  if(get_linked_char(ch, LNK_ESSENCE_OF_WOLF) ||
      get_linking_char(victim, LNK_ESSENCE_OF_WOLF))
    return;

  if(IS_NPC(victim))
  {
    send_to_char("You cannot imbue that being with your essence!\n", ch);
    return;
  }
  
  do_point(ch, victim);

  act
    ("$n &+ythrows $s head back and thrusts $s chest out with a gasp of strain.\n"
     "Their muscles swell with an unseen power.  After &n$n &+yregains $s senses,\n"
     "$e looks around hungrily for a fight.&n", TRUE, victim, 0, 0, TO_ROOM);
  act
    ("&+yYour sight is filled with wolf eyes that permeating your own.  Mighty growling fills your ears and your chest swells as a second mighty spirit melds with your own.&n",
     FALSE, victim, 0, 0, TO_CHAR);

  link_char(ch, victim, LNK_ESSENCE_OF_WOLF);
}

void spell_spirit_walk(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  P_obj    tobj;
  int      found = 0;
  int      location;

  if(!ch || !victim)
    return;

  if(IS_NPC(ch))
  return;

  // check npc/consent
  if(IS_NPC(victim) || !is_linked_to(ch, victim, LNK_CONSENT))
  {
    // failure!
    act
      ("&+L$n quickly becomes dazed and immobile from a sudden shock to the core of his spirit.&N",
       TRUE, ch, 0, 0, TO_ROOM);
    act
      ("&+LYour spirit fails to break free of the bonds of the prime material plane.  Disorientation overwhelms you after your premature return to your body.&N",
       FALSE, ch, 0, 0, TO_CHAR);
    CharWait(ch, 40);
    return;
  }

  // find most recent corpse
  for (tobj = object_list; tobj; tobj = tobj->next)
  {
    if(2 == obj_index[tobj->R_num].virtual_number &&
        isname(GET_NAME(victim), tobj->name))
    {
      found = 1;
      break;
    }
  }

  // if no corpse, fail!
  if(!found || tobj->loc_p != LOC_ROOM)
  {
    act
      ("&+L$n quickly becomes dazed and immobile from a sudden shock to the core of his spirit.&N",
       TRUE, ch, 0, 0, TO_ROOM);
    act
      ("&+LYour spirit fails to break free of the bonds of the prime material plane.  Disorientation overwhelms you after your premature return to your body.&N",
       FALSE, ch, 0, 0, TO_CHAR);
    CharWait(ch, 40);
    return;
  }

  location = tobj->loc.room;

  // if room is !port, fail!
  if(IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
      IS_SET(world[location].room_flags, NO_TELEPORT) ||
      IS_HOMETOWN(ch->in_room) ||
      IS_HOMETOWN(location))
  {
    act
      ("&+L$n quickly becomes dazed and immobile from a sudden shock to the core of his spirit.&N",
       TRUE, ch, 0, 0, TO_ROOM);
    act
      ("&+LYour spirit fails to break free of the bonds of the prime material plane.  Disorientation overwhelms you after your premature return to your body.&N",
       FALSE, ch, 0, 0, TO_CHAR);
    CharWait(ch, 40);
    return;
  }
  // corpse is tobj.  Find a decay affect.  The corpse must both have
  // a decay event, and it must be less then the original decay time,
  // otherwise the corpse was probably preserved...

  struct obj_affect *a1 = get_obj_affect(tobj, TAG_OBJ_DECAY);
  unsigned timer = 0;
  if(a1)
    timer = (unsigned) obj_affect_time(tobj, a1);
  if(!timer || (timer >= (get_property("timer.decay.corpse.pc", 120) * WAIT_MIN)))
  {
    act
      ("&+L$n quickly becomes dazed and immobile from a sudden shock to the core of his spirit.&N",
       TRUE, ch, 0, 0, TO_ROOM);
    act
      ("&+WSomething has gone wrong!&n&+L  The corpse must have been tampered with.&N",
       FALSE, ch, 0, 0, TO_CHAR);
    CharWait(ch, 40);
    return;
  }

  if(!IS_TRUSTED(ch))
  {
    char logbuf[500];
    sprintf(logbuf, "SpiritWalk from %s[%d] to corpse of %s[%d]",
            GET_NAME(ch), world[ch->in_room].number,
            GET_NAME(victim), world[location].number);
    logit(LOG_PORTALS, logbuf);
    // because of the timer limits, less need to spam immos of this
    // statuslog(57, logbuf);
  }

  if(ch &&
     !is_Raidable(ch, 0, 0))
  {
    send_to_char("&+WYou are not raidable. The spell fails!\r\n", ch);
    return;
  }

  act
    ("&+C$n's form begins to blur and shift as $s spirit tears free of its physical bonds. With a blast of&N &+Wwi&N&+wn&N&+Wd&N &+Cand the sound of&N &+rs&N&+Ro&N&+rul&N&+Rs&N &+Cscreaming, $n disappears through a door of the&N &+Ldead.&N",
     TRUE, ch, 0, 0, TO_ROOM);
  act
    ("&+CYour spirit stretches beyond the physical planes and into the astral dimensions, seeking the essence of the departed.  The cosmos rushes by before your mind's eye as a sensation of quietly violent motion overtakes you...&N",
     FALSE, ch, 0, 0, TO_CHAR);

  // successful port
  char_from_room(ch);
  char_to_room(ch, location, -1);
  CharWait(ch, 40);

  act
    ("&+C$n re-enters the world of the living at a tortuously slow pace, floating over the deceased.&N",
     TRUE, ch, 0, 0, TO_ROOM);

  // 10% chance the corpse insta-rots
  if(!number(0, 9))
  {
    // rot!
    act
      ("&+LAlas, the $p is unable to withstand the strain of the linking of two spirits.  It decays before your eyes, leaving only a trace of dust and bone.&n",
       TRUE, ch, tobj, 0, TO_ROOM);
    act
      ("&+LAlas, the $p is unable to withstand the strain of the linking of two spirits.  It decays before your eyes, leaving only a trace of dust and bone.&n",
       TRUE, ch, tobj, 0, TO_CHAR);
    Decay(tobj);
  }
}

void spell_elemental_affinity(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_ELEM_AFFINITY))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_ELEM_AFFINITY)
        af1->duration = level / 2;

    return;
  }

  act
    ("&+YThe forces of &+gn&N&+Ga&N&+gt&N&+Gu&N&+gr&N&+Ge&N &+Ybecome one with $n's body.&N",
     TRUE, victim, 0, 0, TO_ROOM);
  act
    ("&+YEvery essence of&N &+gn&N&+Ga&N&+gt&N&+Gu&N&+gr&N&+Ge&N &+Ybinds itself to your body.&N",
     FALSE, victim, 0, 0, TO_CHAR);

  bzero(&af, sizeof(af));

  af.type = SPELL_ELEM_AFFINITY;
  af.duration = level / 2;
  af.modifier = 0;              // this will store the affinity type

  affect_to_char(victim, &af);
}

struct fury_data
{
  int      room;
  int      round;
};

void event_elemental_fury(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char   t, t_next;
  int in_room;
  struct fury_data *fdata;
  struct affected_type af;

  fdata = (struct fury_data *) data;

  if( ch->in_room != fdata->room )
  {
    send_to_char("&+GYour elemental fury is distrupted!\r\n", ch);
    return;
  }

  switch( fdata->round )
  {
  case 0:
  {
    act("&+cA soft breeze begins tugging at your clothes, quickly gaining strength.", FALSE, ch, 0, 0, TO_ROOM);
    act("&+WYou feel the elements heeding your call and command them to strike your foes with &+Cwind&+W, &n&+yearth&+W, &n&+rheat&+W, and &+Bcold&+W!",
      FALSE, ch, 0, 0, TO_CHAR);
    break;
  }
  case 1:
  {
    if(IS_WATER_ROOM(ch->in_room))
    {
      act("&+BA massive amount of water ciculates and tosses everything about!&n", FALSE, ch, 0, 0, TO_ROOM);
      act("&+BYou summon forth a massive wave of water ciculates and tosses everything about!&n",
        FALSE, ch, 0, 0, TO_CHAR);
      for (t = world[ch->in_room].people; t; t = t_next)
      {
        t_next = t->next_in_room;

        if( !should_area_hit(ch, t) )
        {
          continue;
        }

        if( GET_RACE(t) == RACE_W_ELEMENTAL || IS_TRUSTED(t) || IS_IMMATERIAL(t) || IS_ELITE(t) )
        {
          continue;
        }

        if( !StatSave(t, APPLY_AGI, -4) )
        {
          act("&+BA wall of water &=Cslams&n into you making you fall!&n", FALSE, ch, 0, t, TO_VICT);
          act("&+BA wall of water &=Cslams&n into $n making $m fall!&n", FALSE, ch, 0, t, TO_ROOM);

          SET_POS(t, number(0, 2) + GET_STAT(t));
          if( GET_POS(t) == POS_PRONE )
          {
            CharWait(t, PULSE_VIOLENCE);
          }

          if( make_wet(t, 3) )
          {
            Stun(t, ch,  PULSE_VIOLENCE, TRUE);
          }
        }
      }
    }
    else if( OUTSIDE(ch) )
    {
      act("&+yA deadly hail of stones sweeps through the room!&n", FALSE, ch, 0, 0, TO_ROOM);
      act("&+yA deadly hail of stones sweeps through the room!&n", FALSE, ch, 0, 0, TO_CHAR);

      spell_earthen_rain( GET_LEVEL(ch) - 10, ch, NULL, SPLDAM_EARTH, NULL, NULL);
    }
    else
    {
      spell_greater_earthen_grasp(GET_LEVEL(ch), ch, NULL, SPLDAM_EARTH, NULL, NULL);
    }
    break;
  }
  case 2:
    act("&+rWithout warning the fierce winds explode with heat!&n", FALSE, ch, 0, 0, TO_ROOM);
    act("&+rWithout warning the fierce winds explode with heat!&n", FALSE, ch, 0, 0, TO_CHAR);
    spell_scathing_wind( GET_LEVEL(ch) - 10, ch, NULL, SPLDAM_FIRE, NULL, NULL);
    break;
  case 3:
    act("&+CThe winds abruptly drop below freezing, blowing in the icy chill of the north!&n",
      FALSE, ch, 0, 0, TO_ROOM);
    act("&+CThe winds abruptly drop below freezing, blowing in the icy chill of the north!&n",
      FALSE, ch, 0, 0, TO_CHAR);
    spell_tempest( GET_LEVEL(ch) - 10, ch, NULL, SPELL_TYPE_SPELL, NULL, NULL);
    break;
  default:
    return;
  }

  fdata->round++;

  add_event(event_elemental_fury, PULSE_VIOLENCE, ch, 0, NULL, 0, fdata, sizeof(struct fury_data));
}

void spell_elemental_fury(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  struct fury_data fdata;
  int room;

  if( !IS_ALIVE(ch) || !(room = ch->in_room) )
  {
    return;
  }

  if( victim )
  {
    do_point(ch, victim);
  }

  act("&+CReaching upwards, $n calls upon the &+Wfury&+C of the elements!", FALSE, ch, 0, 0, TO_ROOM);
  act("&+CReaching upwards, you call upon the &+Wfury&+C of the elements!", FALSE, ch, 0, 0, TO_CHAR);

  fdata.round = 0;
  fdata.room = room;
  add_event( event_elemental_fury, PULSE_VIOLENCE, ch, 0, NULL, 0, &fdata, sizeof(fdata) );
}

void spell_ice_missile(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      temp;
  int      dam;
  struct affected_type af;
  struct damage_messages messages = {
    "Your &+Cice missile&n slams into $N&n with a soft thud.",
    "You wince in pain as an &+Cice missile&n sent by $n&n hits you.",
    "An &+Cice missile&n sent from $n&n's outstretched hands squarely hits $N&n.",
    "With a soft thud, the &+Cice missile&n terminates the life of $N&n.",
    "Briefly, you see a chunk of ice flying toward you, then everything is dark..",
    "A small chunk of ice flies from $n&n's fingertips, instantly killing $N&n."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);

  temp = BOUNDED(1, level / 4, 4);
  dam = (dice(1, 3) * 4 + level / 2);
  /* strangely, this spell is much like magic missile..  go figure */
  
  while (temp-- &&
         spell_damage(ch, victim, dam, SPLDAM_COLD,
                      SPLDAM_ALLGLOBES, &messages) == DAM_NONEDEAD) ;
}


void spell_corrosive_blast(int level, P_char ch, char *arg, int type,
                               P_char victim, P_obj obj)
  {
  struct damage_messages messages = {
    "&+G$N&+G screams in utter agony as your corrosive blast slams into $S body!",
    "&+GYou scream in utter agony as $n's&+G corrosive blast slams into your body!",
    "&+G$N&+G screams in utter agony as $n's&+G corrosive blast slams into $S body!",
    "&+GThe corrosive blast leaves nothing of $N&+G, but a melted husk.",
    "&+G$n's&+G corrosive blast coats you in searing agony. Suddenly, the pain is no more.",
    "&+G$n's&+G corrosive blast leaves nothing of $N&+G, but a melted husk.",
     0};
     
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
      return;

  do_point(ch, victim);
  
  int dam = dice(level, 3) * 4;

  if(affected_by_spell(victim, SPELL_CORROSIVE_BLAST))
  {
    send_to_char("&+GYour victim is dripping in acid causing your spell to do less damage.\r\n", ch);
    dam >>= 1;
  }
  
  if(spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_NOSHRUG, &messages) == DAM_VICTDEAD )
    return;
    
  
  if(!affected_by_spell(victim, SPELL_CORROSIVE_BLAST))
  {
    act("&+GThe blast of corrosive matter begins to eat through&n $N's &+Garmor!&n",
      FALSE, ch, obj, victim, TO_CHAR);
    act("&+GThe corrosive blast sears into your armor, quickly eating away at it!&n",
      FALSE, ch, obj, victim, TO_VICT);
    act("$n's &+Gblast of corrosive matter begins to eat through&n $N's &+Garmor!&n",
      FALSE, ch, obj, victim, TO_NOTVICT);

    int ran = number(level, level * 2);
    
    if(GET_AC(victim) + ran > 0)
      ran = MIN(GET_AC(victim) + ran, 0);
    
    struct affected_type af;
    memset(&af, 0, sizeof(af));
    af.type = SPELL_CORROSIVE_BLAST;
    af.duration =  1;
    af.modifier = ran;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
  }
}

void spell_restoration(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  int gain;
 
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
 
  gain = (int) (level * 5 + number(0, 40));

// Non-spiritualist
  if(!has_innate(ch, INNATE_IMPROVED_HEAL))
    gain = (int)(gain * 0.66);

  if(IS_AFFECTED4(victim, AFF4_REV_POLARITY))
  {
    act("&+L$N&+L wavers under the immense rush of life into $S body!", FALSE,
        ch, 0, victim, TO_CHAR);
    act("&+LYou waver under the immense rush of life into your body!", FALSE,
        ch, 0, victim, TO_VICT);
    act("&+L$N&+L wavers under the immense rush of life into $S body!", FALSE,
        ch, 0, victim, TO_NOTVICT);
    damage(ch, victim, gain, TYPE_UNDEFINED);
    return;
  }

  heal(victim, ch, gain, GET_MAX_HIT(victim));
  update_pos(victim);

  act("&+WYou raise your hands skyward, bestowing the brilliance of the Gods upon $N!&n",
    FALSE, ch, obj, victim, TO_CHAR);
  act("&+WYou begin to feel all your pain and suffering ebb away as $n's energy flows into you!&n",
    FALSE, ch, obj, victim, TO_VICT);
  act("&+W$N&+W takes on a much healthier countenance as $n&+W completes $s incantation.&n",
    FALSE, ch, obj, victim, TO_NOTVICT);
  
  poison_common_remove(victim);

  if(affected_by_spell(victim, SPELL_CURSE))
    affect_from_char(victim, SPELL_CURSE);

  if(affected_by_spell(victim, SPELL_MALISON))
    affect_from_char(victim, SPELL_MALISON);

  if(affected_by_spell(victim, SPELL_WITHER))
    affect_from_char(victim, SPELL_WITHER);

  if(affected_by_spell(victim, SPELL_BLOODTOSTONE))
    affect_from_char(victim, SPELL_BLOODTOSTONE);
  
  if(affected_by_spell(victim, SPELL_SHREWTAMENESS))
    affect_from_char(victim, SPELL_SHREWTAMENESS);

  if(affected_by_spell(victim, SPELL_MOUSESTRENGTH))
    affect_from_char(victim, SPELL_MOUSESTRENGTH);

  if(affected_by_spell(victim, SPELL_MOLEVISION))
    affect_from_char(victim, SPELL_MOLEVISION);

  if(affected_by_spell(victim, SPELL_SNAILSPEED))
    affect_from_char(victim, SPELL_SNAILSPEED);

  if(affected_by_spell(victim, SPELL_FEEBLEMIND))
    affect_from_char(victim, SPELL_FEEBLEMIND);
  
  if(affected_by_spell(victim, SPELL_SLOW))
    affect_from_char(victim, SPELL_SLOW);

  if(IS_AFFECTED(victim, AFF_BLIND))
  {
    affect_from_char(victim, SPELL_BLINDNESS);
    REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
  }

  if(IS_AFFECTED4(victim, AFF4_CARRY_PLAGUE))
    REMOVE_BIT(victim->specials.affected_by4, AFF4_CARRY_PLAGUE);

  if(affected_by_spell(victim, SPELL_DISEASE) ||
      affected_by_spell(victim, SPELL_PLAGUE))
  {
    affect_from_char(victim, SPELL_DISEASE);
    affect_from_char(victim, SPELL_PLAGUE);
  }
  
  if(affected_by_spell(victim, TAG_ARMLOCK))
      affect_from_char(victim, TAG_ARMLOCK);
        
  if(affected_by_spell(victim, TAG_LEGLOCK))
      affect_from_char(victim, TAG_LEGLOCK);
      
  if(affected_by_spell(victim, SPELL_ENERGY_DRAIN))
      affect_from_char(victim, SPELL_ENERGY_DRAIN);
      
  if(affected_by_spell(victim, SPELL_SLEEP))
    affect_from_char(victim, SPELL_SLEEP);
    
  if(affected_by_spell(victim, SONG_SLEEP))
    affect_from_char(victim, SONG_SLEEP);
    
  if(affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT))
    affect_from_char(victim, SPELL_RAY_OF_ENFEEBLEMENT);

  send_to_char("&+WYou feel &+csignificantly &+Wbetter.\r\n", victim);
         
}

void spell_flameburst(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "Your &+rflameburst&n collides silently with $N&n, who screams out in sudden pain.",
    "You scream out in sudden pain as a &+rflameburst&n sent by $n&n hits you.",
    "A &+rflameburst&n sent from $n&n's outstretched arms collides silently with $N&n.",
    "With a burst of heat, $N&n is burnt to a crisp.",
    "&+REverything seems so hot..  the pain, the paiiin...&n",
    "A sudden burst of &+rflame&n flies from $n&n's fingertips, burning $N&n to a crisp."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  int num_dice = (level / 10);
  dam = (dice((num_dice+4), 6) * 5);
  dam = BOUNDED(1, dam, 200);

  do_point(ch, victim);

  if(!NewSaves(victim, SAVING_SPELL, 0))
    dam = (int)(dam * 1.33);
  
  if(GET_RACE(victim) == RACE_W_ELEMENTAL)
  {
    spell_damage(ch, victim, (int)(dam * 1.25), SPLDAM_FIRE, SPLDAM_NOSHRUG | SPLDAM_ALLGLOBES, &messages);
    send_to_char("&+RYour f&+ri&+Re&+rr&+Ry b&+ru&+Rr&+rs&+Rt causes your victim to &+ysizzle &+Rquite nicely!\r\n", ch);
    return;
  }
  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_ALLGLOBES, &messages);
}


void spell_scalding_blast(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int  dam;
  struct damage_messages messages = {
    "A scalding column of &+Gacid&n flies from your outstretched arms, melting flesh off of $N&n.",
    "You scream in sudden pain as a scalding hot column of &+Gacid&n sent by $n&n impacts you squarely in the chest.",
    "$N&n screams out in sudden pain as a scalding column of &+Gacid&n sent by $n&n is absorbed by $S chest.",
    "Your scalding column of &+Gacid&n fatally dissolves $N&n's hide.",
    "An extremely acidic column of acid collides with your head, melting your brain.",
    "A scalding column of acid, sent by $n&n, melts $N&n's flesh until nothing but a steaming puddle of blood remains."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  int num_dice = (level / 10);
  dam = (dice((num_dice+4), 6) * 7);
  dam = BOUNDED(1, dam, 240);
  
  do_point(ch, victim);
  
  if(!NewSaves(victim, SAVING_SPELL, 0))
    dam = (int)(dam * 1.33);
  
  if(GET_RACE(victim) == RACE_E_ELEMENTAL)
  {
    spell_damage(ch, victim, (int)(dam * 1.25), SPLDAM_ACID, SPLDAM_NOSHRUG | SPLDAM_ALLGLOBES, &messages);
    
    if(IS_ALIVE(victim))
    {
      send_to_char("&+GThe &+gacid &+Gfrom your spell causes your victim to &+Lsmoke!\r\n", ch);
      send_to_char("&+GThe acid burns your earthly body!\r\n", victim);
    }
    
    return;
  }

  spell_damage(ch, victim, dam, SPLDAM_ACID,
               SPLDAM_ALLGLOBES ^ SPLDAM_MINORGLOBE, &messages);
}

void spell_scorching_touch(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "With your simple touch, $N&n screams and shudders from the sudden searing pain.",
    "Waves of painfully searing heat rush over your entire body as $n&n touches you.",
    "$N&n gasps in pain as $n&n touches $M with $s outstretched hand.",
    "Your scorching touch burns out the candle of $N&n years before $S time.",
    "$n&n grins, touching you with $s outstretched hand.  The last you remember is searing pain..",
    "$n&n's outstretched palm burns away whatever remained of $N&n."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  dam = dice(level, 2) * 4;

  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_GLOBE | SPLDAM_GRSPIRIT,
               &messages);
}


void spell_molten_spray(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int  dam;
  struct damage_messages messages = {
    "Your blast of &+Rmolten spray&n slams into $N&n, burning holes into $S flesh.",
    "$n&n fires a blast of &+Rmolten spray&n at you, causing unbearable pain.",
    "A blast of &+Rmolten spray&n sent by $n&n coats $N&n with searing agony.",
    "Your &+Rmolten spray&n melts what remains of $N&n into a puddle of liquid flesh.",
    "$n&n sends a blast of &+Rmolten spray&n into your face, and you slip away..",
    "A burst of &+Rmolten spray&n sent from $n&n's fingertips melts $N&n into a puddle of mush."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);
  
  dam = dice(level, 2) * 4;
  
  if(!NewSaves(victim, SAVING_SPELL, 2))
    dam = (int) (dam * 1.25);
  
  if(IS_UNDEADRACE(victim))
  {
    spell_damage(ch, victim, (int)(dam * 1.25), SPLDAM_FIRE, SPLDAM_NOSHRUG, &messages);
    
    if(IS_ALIVE(victim))
    {
      send_to_char("&+LThe vile creature &+Rb&+ru&+Rr&+rn&+Rs&n &+Land &+Ys&+yi&+Yz&+yz&+Yl&+ye&+Ys!\r\n", ch);
      send_to_char("&+RThe molten fire &=Lrbursts&n &+Rthrough your defenses and melts away your &+Lundead carcass!\r\n", victim);
    }
    return;
  }
    
  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_GLOBE, &messages);
}

void spell_soul_disturbance(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "$N&n's body spasms uncontrollably as $S soul is wracked with pain.",
    "Your soul is engulfed by pain as $n&n finishes incanting $s spell.",
    "$N&n's body spasms uncontrollably as $S soul is attacked by $n&n.",
    "With a gasp, $N&n realizes $S soul will disturb $M no more.",
    "Your soul begins to ache painfully, then suddenly all is nothing..",
    "With a gasp, $N&n realizes $S soul will disturb $M no more."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);
  
  if(IS_SOULLESS(victim))
  {
    send_to_char("&+YYour victim lacks a &+Wsoul!\r\n", ch);
    return;
  }
  
  if(number(0, 2) &&
   resists_spell(ch, victim))
    return;

  dam = dice(level * 3, 2);
  
  dam = shaman_shield_damage_adjustments(ch, victim, dam); 

  spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_GLOBE | SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,  &messages);
}

void spell_greater_soul_disturbance(int level, P_char ch, char *arg, int type,
                                    P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "$N&n's entire body spasms uncontrollably as $S soul is wracked with unbearable pain.",
    "Your soul is engulfed by unbearable pain as $n&n finishes incanting $s spell.",
    "$N&n's entire body spasms uncontrollably as $S soul is viciously attacked by $n&n.",
    "With a gasp, $N&n realizes $S soul will disturb $M no more.",
    "Your soul begins to ache painfully, then suddenly all is nothing..",
    "With a gasp, $N&n realizes $S soul will disturb $M no more."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);
  
  if(IS_SOULLESS(victim))
  {
    send_to_char("&+YYour victim lacks a &+Wsoul!\r\n", ch);
    return;
  }
  
  if(number(0, 2) &&
     resists_spell(ch, victim))
      return;
      
  dam = dice(level * 6, 2);
  
  dam = shaman_shield_damage_adjustments(ch, victim, dam); 

  spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NOSHRUG, &messages);
}

void spell_greater_spirit_anguish(int level, P_char ch, char *arg, int type,
                                  P_char victim, P_obj obj)
{
  int  dam;
  struct affected_type af;
  bool saved = TRUE;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim))
  {
    return;
  }

  do_point(ch, victim);

  if(IS_SOULLESS(victim))
  {
    send_to_char("&+YYour victim lacks a &+Wsoul!\r\n", ch);
    return;
  }

  dam = dice(level * 3, 6);

  if( IS_SPIRITUALIST(ch ))
  {
    dam += level * 4;
  }

  dam = shaman_shield_damage_adjustments(ch, victim, dam);

  if( !NewSaves(victim, SAVING_FEAR, IS_SPIRITUALIST(ch) ? level/8 : level/14) )
  {
    saved = FALSE;
    dam = (int) (dam * 1.20);
  }

  act("Your &+Wspiritual&N assault devastates $N from within, $E &+LSCREAMS&N in utter anguish!",
     TRUE, ch, 0, victim, TO_CHAR);
  act("$n's &+Wspiritual&N assault devastates you, causing you to &+LSCREAM&N in utter anguish!",
     TRUE, ch, 0, victim, TO_VICT);
  act("$N &+LSCREAMS&N in utter anguish, caused by $n's &+Wspiritual&N assault!",
     TRUE, ch, 0, victim, TO_NOTVICT);

  if(spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NOSHRUG, 0) == DAM_NONEDEAD && GET_SPEC(ch, CLASS_SHAMAN, SPEC_SPIRITUALIST))
  {
    if( (number(1,100) > 50) && !saved && !IS_AFFECTED2(victim, AFF2_SLOW))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_SLOW;
      af.duration = 1;
      af.modifier = 1;
      af.bitvector2 = AFF2_SLOW;

      affect_to_char(victim, &af);

      act("$n &+mbegins to sllooowwww down.", TRUE, victim, 0, 0, TO_ROOM);
      send_to_char("&+WYour will to live wavers! &+mYou feel yourself slowing&n down.\r\n", victim);
    }
  }
}

void spell_spirit_anguish(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "$N&n screams out in anguish as $S spirit rebels against $S body.",
    "You scream out in anguish as your spirit is viciously attacked by $n&n.",
    "$N&n screams out in anguish as $S spirit is viciously violated by $n&n.",
    "$N&n screams in pure anguish as $S spirit finally gives up the ghost.",
    "With an anguish previously unfelt, your spirit gives up the ghost...",
    "Screaming in pure anguish, $N&n collapses under the pain delivered by $n&n."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  do_point(ch, victim);
  
  if(IS_SOULLESS(victim))
  {
    send_to_char("&+YYour victim lacks a &+Wsoul!\r\n", ch);
    return;
  }

  dam = dice(level * 4, 2);
  
  if(IS_SPIRITUALIST(ch))
    dam += level;
  
  dam = shaman_shield_damage_adjustments(ch, victim, dam); 

  spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NODEFLECT | SPLDAM_GLOBE | SPLDAM_NOSHRUG, &messages);
}


void spell_gaseous_cloud(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  struct damage_messages messages = {
    "&+GWith a wave of your hand, you cause $N &+Gto choke painfully on your &+ggaseous cloud&n.",
    "$n &+Gconjures a &+ggaseous cloud &+Gand directs it at you!  A painful bout of choking is your reward.&N",
    "$n &+Gcreates a &+ggaseous cloud &+Gwith a wave of $s hand, directing it at $N and causing $M to choke painfully.&n",
    "&+GWith a wave of the hand, your &+ggaseous cloud &+Gchokes the life out of $N, then quickly dissipates.&n",
    "$n&+G, with a wave of $s hand, creates a &+ggaseous cloud &+Gand directs it at you; it seems to invade your very pores, and you collapse.&n",
    "$n&+G, with a wave of the hand, creates a &+ggaseous cloud &+Gand directs it at $N, killing $M - perhaps $E should have held $S breath?"
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  int dam = dice(level * 3, 6);
  
  int mod = BOUNDED(0, (GET_LEVEL(ch) - GET_LEVEL(victim)), 20);

  if(NewSaves(victim, SAVING_BREATH, mod))
    dam = (int) (dam * 0.75);

  do_point(ch, victim);
  
  if(spell_damage(ch, victim, dam, SPLDAM_GAS, 0, &messages) == DAM_NONEDEAD)
  {
    int was_poisoned = affected_by_spell(victim, SPELL_POISON);

    if(!IS_GREATER_RACE(victim) &&
       (GET_RACE(victim) != RACE_PLANT) &&
       (GET_RACE(victim) != RACE_GOLEM) &&
       (GET_RACE(victim) != RACE_CONSTRUCT) &&
       !IS_ELEMENTAL(victim) &&
       !IS_UNDEADRACE(victim) &&
       !NewSaves(victim, SAVING_PARA, 2))
    {
      level = abs(level);
      poison_lifeleak(level, ch, 0, 0, victim, 0);

      act("$n&n&+g shivers slightly.", TRUE, victim, 0, 0, TO_ROOM);
      if(was_poisoned)
        send_to_char("&+gYou feel even more ill.\n", victim);
      else
        send_to_char("&+gYou feel very sick.\n", victim);
    }
  }
}

void spell_arieks_shattering_iceball(int level, P_char ch, char *arg,
                                     int type, P_char victim, P_obj object)
{
  int room;
  P_obj    obj;
  struct damage_messages messages = {
    "Your &+Ciceball&n shatters upon impacting $N&n, rending flesh and sending blood flying.",
    "You reel in pain as $n&n's &+Ciceball&n shatters upon impacting you.",
    "$n&n's &+Ciceball&n shatters upon impacting $N&n, rending flesh and sending blood flying.",
    "Your &+Ciceball&n shatters any hopes $N&n had of living a long and happy life.",
    "As $n&n's &+Ciceball&n shatters in your face, you swear you hear taps..",
    "The force of $n&n's &+Ciceball&n shatters any hopes $N&n had of living a long and happy life."
  };

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim) ||
     ch->in_room != victim->in_room)
      return;
  
  
// A little less than sunray but without the blinding + larger damage fork
  int dam = dice(level * 3, 5) + number(1, 80); 

  int mod = BOUNDED(0, (GET_LEVEL(ch) - GET_LEVEL(victim)), 20);
  
  if(!NewSaves(victim, SAVING_SPELL, mod))
    dam = (int) (dam * 1.30);

  do_point(ch, victim);
  
  if(spell_damage(ch, victim, dam, SPLDAM_COLD, 0, &messages) != DAM_NONEDEAD)
    return;
    
  if(!(ch) ||
    !(room = ch->in_room) ||
    (room == NOWHERE))
      return;
  
  if(world[room].contents)
  {
    for (obj = world[room].contents; obj; obj = obj->next_content)
    {
      if(obj->R_num == real_object(50))
        return;
    }
  }
    
  obj = read_object(50, VIRTUAL);
  if(!obj || room == NOWHERE)
    return;

  set_obj_affected(obj, 800, TAG_OBJ_DECAY, 0);

  obj_to_room(obj, room);
}


/* owie! */

void spell_single_scathing_wind(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  int dam, mod = 2;
  struct damage_messages messages = {
    "$N&+W gasps in pain as your scathing wind burns flesh from $S body.",
    "&+WThe scathing gust of wind hits you, burning flesh from your body.",
    "&+WThe scathing gust of wind hits &n$N&+W, burning flesh from $S body.",
    "$N&+W only has time to draw a single breath before your scathing wind ends it all.",
    "&+WAn unbelievably hot gust of wind sent by &n$n&+W hits you, melting your flesh..",
    "&+WFlesh falls off in layers as a scathing gust of wind sent by &n$n&+W terminates the life of $N&n.",
    0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim))
  {
    return;
  }

  if(has_innate(ch, INNATE_ELEMENTAL_POWER))
  {
    mod += (int)(level / 10);
  }
  dam = dice(3 * level, 5);

  if (IS_PC(ch) && IS_PC(victim))
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);

  dam = dam * get_property("spell.area.damage.factor.scathingwind", 1.000);

  if(NewSaves(victim, SAVING_SPELL, mod))
    dam >>= 1;

  spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
}

void spell_scathing_wind(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct room_affect raf;
  P_char tch;
  int room = ch->in_room;

  send_to_char
    ("&+WA scathing gust of wind is released from your fingertips.&n\n",
     ch);
  act("&+WA scathing gust of wind bursts from &n$n&n&+W's fingertips!", FALSE,
      ch, 0, 0, TO_ROOM);
  zone_spellmessage(ch->in_room,
                    "&+WYou feel a slight gust of hot air.\n",
                    "&+WYou feel a slight gust of hot air from %s.\n");
  //radiate_message_from_room(ch->in_room, 
  //                          "&+WYou feel a slight gust of hot air.\n", 
  //                          7, // radius
  //                          (RMFR_FLAGS)(RMFR_RADIATE_ALL_DIRS|RMFR_CROSS_ZONE_BARRIER), // flags
  //                          0); // no perception check

  memset(&raf, 0, sizeof(raf));
  raf.type = SPELL_SCATHING_WIND;
  raf.duration = (int) (1.5 * PULSE_VIOLENCE);
  affect_to_room(room, &raf);

  cast_as_damage_area(ch, spell_single_scathing_wind, level, victim,
                      get_property("spell.area.minChance.scathingWind", 0),
                      get_property("spell.area.chanceStep.scathingWind", 20));

  for (tch = world[room].people; tch; tch = tch->next_in_room)
    if(IS_AFFECTED5(tch, AFF5_WET))
    {
      send_to_char("&+wThe hot wind dried up your clothes.\n", tch);
      make_dry(tch);
    }
}

void spell_single_earthen_rain(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "&+yA rain of earth and stone crushes &n$N&n.",
    "&+yA rain of earth and stone crushes you.",
    "&+yA rain of earth and stone crushes &n$N&n.",
    "$N&n&+y is crushed into a bloody pulp by your hail of earth and stone.",
    "&+yA hail of earth and stone crushes you to death.",
    "&+yA hail of earth and stone crushes &n$N&n&+y into a bloody pulp."
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( !OUTSIDE(ch) && !IS_UNDERWORLD(ch->in_room) && world[ch->in_room].sector_type != SECT_EARTH_PLANE )
  {
    send_to_char("You must be outside to cast this spell.\n", ch);
    return;
  }

  if( IS_OCEAN_ROOM(ch->in_room) )
  {
    send_to_char("&+yThere is not a piece of earth in sight.\n", ch);
    return;
  }

  dam = dice(level, 5) * 3;

  if(NewSaves(victim, SAVING_BREATH, 2))
  {
    dam = (int) (dam * 0.66);
  }

  if( number(0, 1) && resists_spell(ch, victim) )
  {
      return;
  }

  // Spell does SPLDAM_EARTH now.
  if( has_innate(ch, INNATE_ELEMENTAL_POWER) )
  {
     dam = (int) (dam * get_property("damage.increase.elementalist", 1.150));
  }

  if( IS_PC(ch) && IS_PC(victim) )
  {
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);
  }

  dam = dam * get_property("spell.area.damage.factor.earthenRain", 1.000);

  spell_damage(ch, victim, dam, SPLDAM_EARTH, SPLDAM_NOSHRUG, &messages);
}

void spell_earthen_rain(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( !OUTSIDE(ch) && !IS_UNDERWORLD(ch->in_room) && world[ch->in_room].sector_type != SECT_EARTH_PLANE )
  {
    send_to_char("You must be outside to cast this spell.\n", ch);
    return;
  }

  if( IS_OCEAN_ROOM(ch->in_room) )
  {
    send_to_char("&+yThere is not a piece of earth in sight.\n", ch);
    return;
  }

  send_to_char("&+yA &+Ldeadly rain&n &+yof rocks and &+Yearth &+yfalls from above!&n\n", ch);
  act("&+yA &+Ldeadly rain&n &+yof rocks and &+Yearth &+yfalls from above!", FALSE, ch, 0, 0, TO_ROOM);

  zone_spellmessage(ch->in_room, "&+ySmall bits of &+Yearth &+yand rock rain from above!\n",
    "&+ySmall bits of &+Yearth &+yand rock rain from %sern sky!\n");

  cast_as_damage_area(ch, spell_single_earthen_rain, level, victim,
    get_property("spell.area.minChance.earthenRain", 0), get_property("spell.area.chanceStep.earthenRain", 10));

  if( IS_ALIVE(ch) )
  {
    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_EARTH))
      level = (int) (level * 1.25);
    spell_earthquake(0-level, ch, NULL, SPELL_TYPE_SPELL, 0, 0);
  }
}

void earthen_grasp( int level, P_char ch, P_char victim )
{
  struct affected_type af;
  enum {EG_FAIL, EG_SHORT, EG_LONG} effect;
  int      dam_result, attdiff;
  struct damage_messages messages = {
    "$N&n&+y gasps for air as your earthen fist squeezes $M tightly.",
    "$n&n&+y's earthen fist tightens around your chest, causing you to gasp for air.",
    "$N&n&+y gasps for air as &n$n&n&+y's earthen fist squeezes $M tightly.",
    "&+yYour earthen fist squeezes the life out of &n$N&n&+y, causing a gush of entrails to fly from $S mouth.",
    "$n&n&+y's earthen fist squeezes the life out of you, ending what little life you had left..",
    "$n&n&+y's earthen fist squeezes the life out of &n$N&n&+y, causing a gush of entrails to fly from $S mouth."
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(affected_by_spell(victim, SPELL_EARTHEN_GRASP))
  {
    send_to_char("As if one fist isn't enough?!\n", ch);
    return;
  }
  // So, at 120 str difference, attdiff is maxxed/min'd.
  attdiff = ((GET_C_STR(ch) - GET_C_STR(victim)) / 3);
  // Randomize.. Subtract or add up to 1 sec.
  attdiff += number( -WAIT_SEC, WAIT_SEC );
  // Bind it to +/- 40 -> 10 sec.
  attdiff = BOUNDED(-40, attdiff, 40);

  if(IS_NPC(victim) && IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA))
  {
    effect = EG_FAIL;
  }
  // If they fail their str bonus save..
  else if(!NewSaves(victim, SAVING_PARA, attdiff))
  {
    // If they fail a regular save.
    if( !NewSaves(victim, SAVING_PARA, 0) )
      effect = EG_LONG;
    else
      effect = EG_FAIL;
  }
  else
  {
    // If they made str bonus and fail regular..
    if( !NewSaves(victim, SAVING_PARA, 0) )
      effect = EG_SHORT;
    else
      effect = EG_FAIL;
  }

  if(effect != EG_FAIL)
  {
    memset(&af, 0, sizeof(struct affected_type));
    af.type = SPELL_EARTHEN_GRASP;
    af.bitvector2 = AFF2_MINOR_PARALYSIS;
    af.flags = AFFTYPE_SHORT;
    if(effect == EG_SHORT)
    {
      send_to_char("&+yAn earthen fist bursts from the ground nearby!&N\n", victim);
      send_to_char("&+yThe fist attempts to grasp you tightly, but only manages a loose hold.&N\n", victim);
      act("&+yAn earthen fist bursts from the ground, grasping &n$n&n&+y!&N", FALSE, victim, 0, 0, TO_ROOM);
      act("$n&n&+y struggles valiantly, making a tighter grasp impossible.&N", FALSE, victim, 0, 0, TO_ROOM);
      // From minimum 1 sec to str-saved max = 10 sec.
      af.duration = (attdiff <= WAIT_SEC) ? WAIT_SEC : attdiff;
    }
    else
    {
      send_to_char("&+yAn earthen fist bursts from the ground, grasping you tightly around the chest.&N\n", victim);
      act("&+yAn earthen fist bursts from the ground, grasping &n$n&n&+y tightly!&N", FALSE, victim, 0, 0, TO_ROOM);
      // From minimum 5 sec to absolute max at 15 sec.
      af.duration = (attdiff <= 0) ? 5 * WAIT_SEC : attdiff + 5 * WAIT_SEC;
    }

    if( has_innate( ch, INNATE_ELEMENTAL_POWER ) )
    {
      level = (int) (level * get_property("damage.increase.elementalist", 1.150));
    }

    if(!NewSaves(victim, SAVING_SPELL, 0))
      dam_result =
        spell_damage(ch, victim, dice(level, 12), SPLDAM_EARTH, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages);
    else
      dam_result =
        spell_damage(ch, victim, dice(level, 7), SPLDAM_EARTH, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages);

    if(dam_result == DAM_NONEDEAD)
    {
      stop_fighting(victim);
      if( IS_DESTROYING(victim) )
        stop_destroying(victim);
      StopMercifulAttackers(victim);
      affect_to_char(victim, &af);
    }
  }
  else
  {
    send_to_char("&+yAn earthen fist bursts from the ground nearby!&N\n", victim);
    send_to_char("&+yThe fist attempts to grasp you tightly, but you avoid it.&N\n", victim);
    act("&+yAn earthen fist bursts from the ground, aiming at &n$n&n&+y but $e manages to avoid it.&N", FALSE, victim, 0, 0, TO_ROOM);
  }
}

void spell_earthen_grasp(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if(victim == ch)
  {
    send_to_char("You suddenly decide against that.\r\n", ch);
    return;
  }

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  earthen_grasp( level, ch, victim );
}


void spell_single_earthen_grasp(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if( !IS_ALIVE(ch) || !(ch->in_room))
  {
    return;
  }

  earthen_grasp( level, ch, victim );
}

void spell_greater_earthen_grasp(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   tch, next;
  int      room;

  // We need room because ch might die in the middle of the spell (via deflect or such).
  //   If this happens we still want the zone message.
  if( !IS_ALIVE(ch) || (room = ch->in_room) < 0 || room > top_of_world )
  {
    return;
  }

  if( victim == ch )
  {
    send_to_char("You suddenly decide against that.\r\n", ch);
    return;
  }

  send_to_char("&+yThe ground rumbles deeply as you finish your spell..&n\n", ch);
  act("&+yThe ground suddenly rumbles deeply..", FALSE, ch, 0, 0, TO_ROOM);

  cast_as_damage_area(ch, spell_single_earthen_grasp, level, victim,
    get_property("spell.area.minChance.gEarthGrasp", 100),
    get_property("spell.area.chanceStep.gEarthGrasp", 20));

  zone_spellmessage(room, "&+yThe ground rumbles &+Ldeeply &+ynearby...\n",
    "&+yThe ground rumbles &+Ldeeply &+yto the %s...\n");
}

void spell_pythonsting(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool     was_poisoned;
  int      dam, temp;
  struct damage_messages messages = {
    "&+gFrom nowhere, your python bites &n$N&n&+g in the leg, who reels in shock.",
    "&+gFrom nowhere, a python summoned by &n$n&n&+g bites you in the leg, disappearing as quickly as it appeared.",
    "&+gFrom nowhere, a python summoned by &n$n&n&+g bites &n$N&n&+g in the leg, disappearing as quickly as it appeared.",
    "&+gFrom nowhere, your python bites &n$N&n&+g in the leg.  &n$N&n&+g promptly dies.",
    "&+gFrom nowhere, a python bites you in the leg, sending you into blackness..",
    "&+gFrom nowhere, a python bites &n$N&n&+g in the leg, causing $M to promptly die."
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(IS_TRUSTED(victim) || resists_spell(ch, victim))
    return;

  was_poisoned = affected_by_spell(victim, SPELL_POISON);

  temp = level;
  dam = number(1, level * 4);
  dam = BOUNDED(1, dam, 160);

  temp = BOUNDED(-5, (GET_LEVEL(victim) - GET_LEVEL(ch)) / 5, 5);

  if(!StatSave(victim, APPLY_AGI, temp))
  {
    act
      ("$n&n&+g is not nimble enough and gets bitten in a particularly bad way!",
       TRUE, victim, 0, 0, TO_NOTVICT);
    act
      ("&+gYou are not nimble enough and get bitten in a particularly bad way!",
       TRUE, victim, 0, 0, TO_VICT);
    dam *= 2;
  }

  if (IS_PC(ch) && IS_PC(victim))
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);
  
  if( spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD )
    return;

  if(!IS_DRAGON(victim) &&
     !IS_DEMON(victim) &&
     !IS_ELEMENTAL(victim) &&
     !IS_UNDEAD(victim) &&
     !saves_spell(victim, SAVING_PARA))
  {
    level = abs(level);
    (skills[number(FIRST_POISON, LAST_POISON)].spell_pointer) (level, ch, 0,
                                                               0, victim, 0);

    act("$n&n&+g shivers slightly.", TRUE, victim, 0, 0, TO_ROOM);
    if(was_poisoned)
      send_to_char("&+gYou feel even more ill.\n", victim);
    else
      send_to_char("&+gYou feel very sick.\n", victim);
  }
}

void spell_greater_pythonsting(int level, P_char ch, char *arg, int type,
                               P_char victim, P_obj obj)
{
  P_char   tch, next;

  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
  
  if(victim == ch)
  {
    send_to_char("You suddenly decide against that, oddly enough.\n", ch);
    return;
  }
  
  cast_as_damage_area(ch, spell_pythonsting, level, victim,
                      get_property("spell.area.minChance.gPythonSting", 100),
                      get_property("spell.area.chanceStep.gPythonSting", 20));
}

void spell_bloodhound(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  if(!affected_by_spell(victim, SPELL_BLOODHOUND))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_BLOODHOUND;
    af.duration = level;
    affect_to_char(victim, &af);

    send_to_char("Your sense of smell becomes like that of a &+rbloodhound&n.\n", victim);
  }
}

void spell_spirit_armor(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;
  int duration = level;
  int armor = level;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  do_point(ch, victim);
  
  if(IS_SPIRITUALIST(ch))
  {
    duration += (int)(GET_LEVEL(ch) / 8);
    armor += GET_LEVEL(ch);
  }
    
  if(!affected_by_spell(victim, SPELL_SPIRIT_ARMOR))
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_SPIRIT_ARMOR;
    af.duration = duration;
    af.location = APPLY_AC;
    af.modifier = -(duration);
    af.bitvector = AFF_ARMOR;
    affect_to_char(victim, &af);

    if(level >= 50)
    {
      af.modifier = -2;
      af.location = APPLY_SAVING_SPELL;

      affect_to_char(victim, &af);
    }
    else if(level > 40)
    {
      af.modifier = -1;
      af.location = APPLY_SAVING_SPELL;

      affect_to_char(victim, &af);
    }
    send_to_char("&+mYou feel protective spirits watching over you.\n",
                 victim);
    act("&n$n&n&+m is briefly surrounded by a purple aura.&n", TRUE, victim,
        0, 0, TO_ROOM);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SPIRIT_ARMOR)
      {
        af1->duration = duration;
      }
  }
}

void spell_transfer_wellness(int level, P_char ch, char *arg, int type,
                             P_char victim, P_obj obj)
{
  int      to, from, pos;
  P_obj    corpse, t_obj, next_obj, money;
  int      room;
  int      char_number;
  bool     explode = FALSE;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);
  
  if(ch == victim)
  {
    send_to_char
      ("Transferring life from yourself to yourself?  That's a bit odd.\n", ch);
    return;
  }
  
  if(GET_HIT(victim) >= GET_MAX_HIT(victim))
  {
    act("Hmm, on second thought, $E looks as good as (if not better than) usual.",
       TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  
  if(GET_HIT(ch) <= 0)
  {
    send_to_char("Considering your condition, that really isn't a good idea..\n", ch);
    return;
  }
  
  to = (int) (level * 2 + dice(2, 8) + GET_HIT(ch) / 3);

  from = (int) (to / 2);
  
  if((GET_HIT(ch) - from) < 0)
  {
    send_to_char("You get the feeling that isn't such a great idea...\n", ch);
    explode = TRUE;
    from = GET_HIT(ch) + 10;
    if(to > GET_HIT(ch))
      to = GET_HIT(ch);
  }
  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + to);
  if(!explode)
    GET_HIT(ch) -= from;

  act("$N&n transfers some of $S &+Wlife force&n to you.",
    FALSE, victim, 0, ch, TO_CHAR);
  send_to_char("You &+Lsag&n under the strain of transferring your &+Wlife force.&n\n", ch);
  act("$n&n &+Lsags&n visibly as $e transfers some of $s &+Wlife force&n to &n$N&n.",
      TRUE, ch, 0, victim, TO_ROOM);

  if(!explode)
    return;

  send_to_char("&+LThe strain of transferring your life force was too much for you.&n\n", ch);
  send_to_char("&+LYou feel your blood starting to boil, and the rest is darkness...&n\n", ch);
  act("$n&n &+Rpales, then turns red, and suddenly explodes torn apart by the wild magic that took control over $s &+Rweakened spirit!&n.",
     FALSE, ch, 0, 0, TO_ROOM);

  for (victim = world[ch->in_room].people; victim;
       victim = victim->next_in_room)
  {
    if(ch != victim)
      if(StatSave(victim, APPLY_AGI, -5))
        act("&+LYou barely evade getting hit by $S &+Lflying body parts!&n",
            FALSE, victim, 0, ch, TO_CHAR);
      else
      {
        act("&+LYou get hit by $S &+Lflying body parts!&n",
          FALSE, victim, 0, ch, TO_CHAR);
        if(GET_HIT(victim) < from)
        {
          send_to_char
            ("&+LOne of the flying bones hits you in your head, and you pass out...&n\n",
             victim);
        }
        damage(ch, victim, from, TYPE_UNDEFINED);
      }
  }

  if(ch->in_room == NOWHERE)
  {
    if((room = real_room(ch->specials.was_in_room)) == NOWHERE)
    {
      wizlog(57,
             "Serious screw up in transfer wellness - tried to explode char %s in nowhere!",
             GET_NAME(ch));
      return;
    }
  }
  else
    room = ch->in_room;

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    obj_from_char(t_obj);
    obj_to_room(t_obj, room);
  }

  for (pos = 0; pos < MAX_WEAR; pos++)
  {
    if(ch->equipment[pos])
    {
      obj_to_room(unequip_char(ch, pos), room);
    }
  }

  if(GET_MONEY(ch) > 0)
  {
    money = create_money(GET_COPPER(ch), GET_SILVER(ch),
                         GET_GOLD(ch), GET_PLATINUM(ch));
    obj_to_room(money, room);
  }

  char_from_room(ch);
  char_to_room(ch, real_room(1), -2);

  die(ch, ch);

}

void spell_sustenance(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  /* add level / 2 hours to full, level / 4 to thirst, no higher than
     level hours full and level / 2 thirsty total */

/*
   if(IS_AFFECTED3(victim,AFF3_FAMINE))
   REMOVE_BIT(victim->specials.affected_by3,AFF3_FAMINE);
 */
  GET_COND(victim, FULL) += level >> 1;
  if((GET_COND(victim, FULL)) > level)
    GET_COND(victim, FULL) = level;

  GET_COND(victim, THIRST) += level >> 2;
  if((GET_COND(victim, THIRST)) > level >> 1)
    GET_COND(victim, THIRST) = level >> 1;

  send_to_char("You feel comfortably sated.\n", victim);
}


void spell_greater_sustenance(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  LOOP_THRU_PEOPLE(victim, ch)
  {
    spell_sustenance(level, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
    if(IS_AFFECTED3(victim, AFF3_FAMINE))
      affect_from_char(victim, SPELL_APOCALYPSE);
  }
}

void spell_wolfspeed(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
     
  do_point(ch, victim);
  
  if(IS_ANIMALIST(ch))
    level += number(10, (int)(GET_LEVEL(ch) / 2));

  if(affected_by_spell(victim, SPELL_WOLFSPEED))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_WOLFSPEED)
      {
        af1->duration = level + 1;
      }
    return;
  }
  
  if(SHAMAN_MOVE_SPELL(victim))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }
  else if(ch == victim)
  {
    send_to_char
      ("&+WSuddenly, a feeling of speed courses through your veins!&n\n",
       ch);
  }
  else
  {
    act("&+WSuddenly, a feeling of speed courses through your veins!&n",
        FALSE, ch, 0, victim, TO_VICT);
  }

  act("A wolfish grin briefly crosses $n&n's face.",
    TRUE, victim, 0, 0, TO_ROOM);

  bzero(&af, sizeof(af));

  af.type = SPELL_WOLFSPEED;
  af.duration = (level) + 1;

  af.location = APPLY_MOVE;
  af.modifier = (level / 2) + dice(2, 2);

  affect_to_char(victim, &af);
}

void spell_snailspeed(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;
  int percent = 0, maxpoints;
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  do_point(ch, victim);

  if(SHAMAN_MOVE_SPELL(victim))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }
  
  percent = shaman_wis_save_result(ch, victim);  
  
  if(IS_NPC(victim) &&
    CAN_SEE(victim, ch) &&
    (!IS_PC(ch) ||
     !IS_PC_PET(ch)))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }

  if(ch == victim)
  {
    send_to_char("&+LThe spirits do not heed your call!&n\n&n",ch);
    return;
  }

  maxpoints = GET_MAX_VITALITY(victim);

  if(IS_ANIMALIST(ch))
    level += number(10, (int)(GET_LEVEL(ch) / 2));
    
  int affect = (int)(GET_MAX_VITALITY(victim) / 2);
  
  if(percent < 10)
  {
    act("$N seems to shrug off your spell!", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }    
  else if(percent > 90)
  {
   act("&+LSuddenly, you feel extremely sluggish and slow!&n", FALSE, ch, 0, victim, TO_VICT);
   act("$n&n begins to move much more sluggishly.", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_SNAILSPEED;
    af.duration = 1;

    af.location = APPLY_MOVE;
    af.modifier = (int)(-1.25 * affect + number(0, 10));

    affect_to_char(victim, &af);
    
    if(IS_ANIMALIST(ch) &&
       !GET_CLASS(victim, CLASS_MONK) &&
       !IS_AFFECTED2(victim, AFF2_SLOW) &&
       !IS_GREATER_RACE(victim) &&
       !IS_ELITE(victim) &&
       !NewSaves(victim, SAVING_SPELL, 0))
    {
       
      bzero(&af, sizeof(af));
      af.type = SPELL_SLOW;
      af.duration = 1;
      af.modifier = 2;
      af.bitvector2 = AFF2_SLOW;

      affect_to_char(victim, &af);

      act("$n begins to &+yslow down&n as if affected by something unseen...",
        TRUE, victim, 0, 0, TO_ROOM);
      send_to_char("&+mYou feel yourself slowing down drastically.\r\n", victim);
    }
    
  return;
 }
 else if(percent > 70)
 {
    act("&+LSuddenly, you feel very sluggish and slow!&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("$n&n begins to move very sluggishly.", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_SNAILSPEED;
    af.duration = 1;

    af.location = APPLY_MOVE;
    af.modifier = (-1 * affect + number(0, 5));

    affect_to_char(victim, &af);
    return;
 }
 else if(percent > 30)
 {
    act("&+LSuddenly, you feel sluggish and slow!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n begins to move more sluggishly.", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_SNAILSPEED;
    af.duration = 1;

    af.location = APPLY_MOVE;
    af.modifier = (int)(-1 * affect * 0.66 + number(-5, 5));

    affect_to_char(victim, &af);
    return;
  }
  else
  {
    act("&+LSuddenly, you feel a bit more sluggish and slow!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n begins to move a bit more sluggishly.", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_SNAILSPEED;
    af.duration = 1;

    af.location = APPLY_MOVE;
    af.modifier = (int)(-1 * affect * 0.33 + number(-5, 5));

    affect_to_char(victim, &af);
  }
}

void spell_pantherspeed(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
        
  do_point(ch, victim);
  
  if(IS_ANIMALIST(ch))
    level += number(10, (int)(GET_LEVEL(ch) / 2));
    
  if(affected_by_spell(victim, SPELL_SNAILSPEED))
  {
    struct affected_type *af1;
    
    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SNAILSPEED)
      {
        send_to_char("Your snail-like speed is negated!\r\n", victim);
        affect_remove(victim, af1);
      }
    return;
  }
    
  if(affected_by_spell(victim, SPELL_PANTHERSPEED))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PANTHERSPEED)
      {
        send_to_char("Your panther-like speed is renewed.\r\n", victim);
        af1->duration = level + 1;
      }
    return;
  }

  if(SHAMAN_MOVE_SPELL(victim))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }

  if(ch == victim)
  {
    send_to_char("&+WSuddenly, a feeling of speed courses through your veins!&n\r\n", ch);
  }
  else
    act("&+WSuddenly, a feeling of speed courses through your veins!&n",
        FALSE, ch, 0, victim, TO_VICT);

  act("$n&n begins to move much more quickly.", TRUE, victim, 0, 0, TO_ROOM);

  bzero(&af, sizeof(af));

  af.type = SPELL_PANTHERSPEED;
  af.duration = (level) + 1;

  af.location = APPLY_MOVE;
  af.modifier = level + 5 + dice(3, 6);

  affect_to_char(victim, &af);
}

void spell_molevision(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;
  int percent = 0, hitroll = 0;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  if(ch == victim)
  {
    send_to_char("&+LYeah, good idea! Not!&n\n", ch);
    return;
  }
  
  do_point(ch, victim);
  
  if(affected_by_spell(victim, SPELL_MOLEVISION) ||
     affected_by_spell(victim, SPELL_HAWKVISION))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }

  percent = shaman_wis_save_result(ch, victim);
  
  if(IS_NPC(victim) &&
    CAN_SEE(victim, ch) &&
    (IS_PC(ch) ||
    !IS_PC_PET(ch)))
  {
    remember(victim, ch);
    
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }
  
  hitroll = (int)(GET_HITROLL(victim) +
    str_app[STAT_INDEX(GET_C_STR(victim))].tohit);
    
  if(percent < 10)
  {
    act("$N seems to shrug off your spell!", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  else if(percent > 110)
  {
    act("&+ySuddenly, your vision grows extremely blurry and &+Ldarkness&n &+y clouds your sight!&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("$n&n blinks trying to clear $s vision.", TRUE, victim, 0, 0, TO_ROOM);
   
    bzero(&af, sizeof(af));

    af.type = SPELL_MOLEVISION;
    af.duration = 1;

    af.location = APPLY_HITROLL;
    af.modifier = (int)(-1 * hitroll / 1.3);

    affect_to_char(victim, &af);
    return;
  }
  else if(percent > 90)
  {
    act("&+LSuddenly, your vision grows extremely blurry!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n blinks, and nearly runs into the wall seemingly trying to clear $s vision.", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_MOLEVISION;
    af.duration = 1;

    af.location = APPLY_HITROLL;
    af.modifier = (int)(-1 * hitroll / 1.7);

    affect_to_char(victim, &af);
    return;

  }
  else if(percent > 70)
  {
    act("&+LSuddenly, your vision very extremely blurry!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n blinks rapidly, seemingly trying to clear $s vision.", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_MOLEVISION;
    af.duration = 1;

    af.location = APPLY_HITROLL;
    af.modifier = (int)(-1 * hitroll / 2.1);

    affect_to_char(victim, &af);
  }
  else if(percent > 40)
  {
    act("&+LSuddenly, your vision grows blurry!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n blinks, seemingly trying to clear $s vision.", TRUE, victim, 0, 0, TO_ROOM);
  
    bzero(&af, sizeof(af));

    af.type = SPELL_MOLEVISION;
    af.duration = 1;

    af.location = APPLY_HITROLL;
    af.modifier = (int)(-1 * hitroll / 2.5);

    affect_to_char(victim, &af);
    return;
  }
  else
  {
    act("&+LSuddenly, your vision grows slightly blurry!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n blinks slightly, seemingly trying to clear $s vision.", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_MOLEVISION;
    af.duration = 1;

    af.location = APPLY_HITROLL;
    af.modifier = (int)(-1 * hitroll / 3);

    affect_to_char(victim, &af);
    return;
  }
}

void spell_mousestrength(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  int percent = 0;
  int base_str, mod, curr_str;

  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
      return;

  do_point(ch, victim);

  if(SHAMAN_STR_SPELL(victim))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }
      
  percent = shaman_wis_save_result(ch, victim);
  
  if(IS_NPC(victim) &&
    CAN_SEE(victim, ch) &&
    (IS_PC(ch) ||
    !IS_PC_PET(ch)))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }

  if(ch == victim)
    send_to_char ("&+wThe spirits do not heed your call.&n\n", ch);
    
  base_str = victim->base_stats.Str;
  curr_str = GET_C_STR(victim);
  mod = level;
  
  if(mod > base_str)
    mod = base_str;
  
  if(mod > curr_str)
    mod = curr_str;
    
  if(IS_ANIMALIST(ch))
    mod += number(0, (int)(GET_LEVEL(ch) / 5));

  mod = (int)(mod * 0.5);
  
  if(percent < 10)
  {
    act("$N seems to shrug off your spell!", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  else if(percent > 90)
  {
    act("&+ySuddenly, your muscles diminish, but you feel more dexterous and agile!&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("$n&n &+yvisibly greatly weakens but appears rather brisk.&n",
      TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));
    af.type = SPELL_MOUSESTRENGTH;
    
    af.duration = 2 + (number(0, 2));
    af.location = APPLY_STR;
    af.modifier = (int) (-1 * mod - dice(2, 5));
    affect_to_char(victim, &af);
    
    af.location = APPLY_DEX_MAX;
    af.modifier = (int)(level / 2) + number(0, 10);
    affect_to_char(victim, &af);
    
    af.location = APPLY_AGI_MAX;
    af.modifier = (int)(level / 2) + number(0, 1);
    affect_to_char(victim, &af);
    return;
  }
  else if(percent > 70)
  {
    act("&+LSuddenly, you feel &+wextremely weak&n &+L,but you are reacting faster.&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("$n&n &+yweakens visibly, yet seems extremely mobile.&n",
      TRUE, victim, 0, 0, TO_ROOM);
    
    bzero(&af, sizeof(af));
    af.type = SPELL_MOUSESTRENGTH;
    af.duration = 2 + (number(0, 2));
    
    af.location = APPLY_STR;
    af.modifier = (int) ( -1 * mod + dice(2, 3));
    affect_to_char(victim, &af);
    
    af.location = APPLY_DEX_MAX;
    af.modifier = (int)(level / 3) + number(0, 10);
    
    affect_to_char(victim, &af);
    af.location = APPLY_AGI_MAX;      /* why not */
    
    af.modifier = (int)(level / 3) + number(0, 10);
    affect_to_char(victim, &af);
    return;
  }
  else if(percent > 40)
  {
    act("&+LSuddenly, you feel weak but more energetic!&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("$n&n &+yvisibly weakens, but seems energetic.&n",
      TRUE, victim, 0, 0, TO_ROOM);
    
    bzero(&af, sizeof(af));
    af.type = SPELL_MOUSESTRENGTH;
    af.duration = 1;
    
    af.location = APPLY_STR;
    af.modifier = (int) (-1 * mod * 0.8 + dice(2, 3));
    affect_to_char(victim, &af);
    
    af.location = APPLY_DEX_MAX;
    af.modifier = (int)(level / 4) + number(0, 5);
    affect_to_char(victim, &af);
    
    af.location = APPLY_AGI_MAX;
    af.modifier = (int)(level / 4) + number(0, 5);
    affect_to_char(victim, &af);
    return;
  }
  else
  {
    act("&+LSuddenly, you feel slightly weaker, but more dexterous!&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("$n&n &+yweakens visibly, but still seems somewhat spry.&n",
      TRUE, victim, 0, 0, TO_ROOM);
    
    bzero(&af, sizeof(af));
    af.type = SPELL_MOUSESTRENGTH;
    af.duration = 1;
    
    af.location = APPLY_STR;
    af.modifier = (int) (-1 * mod * 0.5 + dice(2, 3));
    affect_to_char(victim, &af);
    
    af.location = APPLY_DEX;
    af.modifier = (int) (level / 5) + number(0, 5);
    affect_to_char(victim, &af);
    
    af.location = APPLY_AGI;
    af.modifier = (int) (level / 5) + number(0, 5);
    affect_to_char(victim, &af);
    return;
  }
}

void spell_hawkvision(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);
  
  if(IS_ANIMALIST(ch))
    level = level + 20;

  if(IS_AFFECTED4(victim, AFF4_HAWKVISION))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_HAWKVISION)
      {
        af1->duration = level / 2;
      }
    return;
  }
  
  if(affected_by_spell(victim, SPELL_MOLEVISION))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }

  if(ch == victim)
  {
    send_to_char
      ("You point at yourself. &+WSuddenly, your vision becomes sharper!&n\n", ch);
  }
  else
    act("&+WSuddenly, your vision becomes sharper!&n",
      FALSE, ch, 0, victim, TO_VICT);

  act("$n&n's pupils rapidly dilate until almost no iris is visible.",
    TRUE, victim, 0, 0, TO_ROOM);

  bzero(&af, sizeof(af));

  af.type = SPELL_HAWKVISION;
  af.duration = level/ 2;
  af.bitvector4 = AFF4_HAWKVISION;

  affect_to_char(victim, &af);
}

void spell_bearstrength(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  do_point(ch, victim);

  if( SHAMAN_STR_SPELL(victim) )
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }

  if(IS_ANIMALIST(ch))
    level = level + 20;

  if( ch != victim )
  {
    // Allow if consented, or if victim is master, or victim is consented to master.
    if( NewSaves(victim, SAVING_SPELL, 0) &&
      !(is_linked_to(ch, victim, LNK_CONSENT) || (IS_PC_PET(ch) && GET_MASTER(ch) == victim)
      || (IS_PC_PET(ch) && is_linked_to(GET_MASTER(ch), victim, LNK_CONSENT) )) )
    {
      return;
    }
  }
  else
    act("&+WSuddenly, you feel stronger!&n", TRUE, ch, 0, victim, TO_VICT);

  act("$n&n strengthens visibly.", TRUE, victim, 0, 0, TO_ROOM);

  if(IS_PC(ch) ||
     IS_PC(victim) ||
     IS_PC_PET(ch) ||
     IS_PC_PET(victim))
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_BEARSTRENGTH;
    af.duration = level + number(0, (int)(level / 10));

    af.location = APPLY_STR;
    af.modifier = (int)(level / 3 + number(0, (int)(level / 10)));

    affect_to_char(victim, &af);

    af.location = APPLY_DEX;
    af.modifier = -10;

    affect_to_char(victim, &af);
    
    af.location = APPLY_STR_MAX;
    af.modifier = (int) (level / 20 + 1);
    
    affect_to_char(victim, &af);
  }
  else
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_BEARSTRENGTH;
    af.duration = 100;

    af.location = APPLY_STR;
    af.modifier = (int)(level / 2);

    affect_to_char(victim, &af);
    
    af.location = APPLY_STR_MAX;
    af.modifier = (int) (level / 10 + 1);
    
    affect_to_char(victim, &af);
  }
}


void spell_elephantstrength(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct affected_type af;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(SHAMAN_STR_SPELL(victim))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }

  if(ch != victim)
  {
    // Allow if consented, or if victim is master, or victim is consented to master.
    if( NewSaves(victim, SAVING_SPELL, 0) &&
      !(is_linked_to(ch, victim, LNK_CONSENT) || (IS_PC_PET(ch) && GET_MASTER(ch) == victim)
      || (IS_PC_PET(ch) && is_linked_to(GET_MASTER(ch), victim, LNK_CONSENT) )) )
    {
      return;
    }
  }
  
  do_point(ch, victim);
 
  if(IS_ANIMALIST(ch))
    level += number(10, (int)(GET_LEVEL(ch) / 2));

  if(ch == victim)
    send_to_char("&+WSuddenly, you feel much stronger!&n\n", ch);
  else
    act("&+WSuddenly, you feel much stronger!&n",
      FALSE, ch, 0, victim,  TO_VICT);

  act("$n&n strengthens visibly.", TRUE, victim, 0, 0, TO_ROOM);
  
  if(IS_PC(ch) ||
     IS_PC(victim) ||
     IS_PC_PET(ch) ||
     IS_PC_PET(victim))
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_ELEPHANTSTRENGTH;
    af.duration = (level) + 1;

    af.location = APPLY_STR_MAX;
    af.modifier = (int)(level / 10) + dice(2, 2);

    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = af.modifier = (int)(level / 5) + dice(3, 3);
   
    affect_to_char(victim, &af);

    af.location = APPLY_DEX;
    af.modifier = -10 - number(0, 5);

    affect_to_char(victim, &af);

    af.location = APPLY_AGI;
    af.modifier = -10 - number(0, 5);
    
    af.location = APPLY_DAMROLL;
    af.modifier = (int)((level / 10) + 1);

    affect_to_char(victim, &af);
  }
  else
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_ELEPHANTSTRENGTH;
    af.duration = 100;

    af.location = APPLY_STR_MAX;
    af.modifier = (int)(level / 10) + dice(2, 2);

    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = af.modifier = (int)(level / 5) + dice(3, 3);
   
    affect_to_char(victim, &af);
    
    af.location = APPLY_DAMROLL;
    af.modifier = (int)((level / 10) + 1);
    affect_to_char(victim, &af);
  }
}


void spell_shrewtameness(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  int percent = 0;
  int curr_str, base_str, mod;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
      
  do_point(ch, victim);

  if((affected_by_spell(victim, SPELL_LIONRAGE)) ||
     (affected_by_spell(victim, SPELL_SHREWTAMENESS)))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }
      
  if(ch == victim)
    send_to_char ("&+wThe spirits do not heed your call.&n\n", ch);
    
  percent = shaman_wis_save_result(ch, victim);

  if(IS_NPC(victim) &&
    CAN_SEE(victim, ch) &&
    (IS_PC(ch) ||
    !IS_PC_PET(ch)))
  {
    remember(victim, ch);
    
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }

  base_str = victim->base_stats.Str;
  curr_str = GET_C_STR(victim);
  mod = level;
  
  if(mod > base_str)
    mod = base_str;
  
  if(mod > curr_str)
    mod = curr_str;

  if(IS_ANIMALIST(ch))
    mod += number(0, (int)(GET_LEVEL(ch) / 10));
  
  if(percent < 10)
  {
    act("$N seems to shrug off your spell!", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  else if(percent > 90)
  {
    act("&+LSuddenly, you huddle in the corner, weak and frightened.&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n &+ysuddenly huddles in the corner and appears &+rweak and frightened.&n", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_SHREWTAMENESS;
    af.duration = 1;

    af.location = APPLY_STR;
    af.modifier = (int)(-1 * mod / 2);

    affect_to_char(victim, &af);

    af.location = APPLY_DAMROLL;
    af.modifier = (int)(-1 * mod / 10);

    affect_to_char(victim, &af);
    return;
  }
  else if(percent > 70)
  {
    act("&+LSuddenly, you feel very weak and frightened.&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n &+wsuddenly appears very weak and frightened.&n", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_SHREWTAMENESS;
    af.duration = 1;

    af.location = APPLY_STR;
    af.modifier = (int) (-1 * mod / 3);

    affect_to_char(victim, &af);

    af.location = APPLY_DAMROLL;
    af.modifier = (int) (-1 * mod / 20);

    affect_to_char(victim, &af);
    return;
  }
  else if(percent > 40)
  {
    act("&+LSuddenly, you feel weak and frightened.&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n &+wsuddenly appears weak and frightened.&n", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_SHREWTAMENESS;
    af.duration = 1;

    af.location = APPLY_STR;
    af.modifier = (int)(-1 * mod / 4);

    affect_to_char(victim, &af);

    return;
  }
  else
  {
    act("&+LSuddenly, you feel slightly weaker and frightened.&n", FALSE, ch, 0, victim, TO_VICT);
    act("$n&n &+wsuddenly appears slightly weaker and frightened.&n", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));

    af.type = SPELL_SHREWTAMENESS;
    af.duration = 1;

    af.location = APPLY_STR;
    af.modifier = (int) (-1 * mod / 5);

    affect_to_char(victim, &af);
    return;
  }
}


void spell_lionrage(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;
  int duration;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }
  do_point(ch, victim);

  if( (affected_by_spell(victim, SPELL_LIONRAGE))
    || (affected_by_spell(victim, SPELL_SHREWTAMENESS)) )
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }

  if(ch != victim)
  {
    // Allow if consented, or if victim is master, or victim is consented to master.
    if( NewSaves(victim, SAVING_SPELL, 0) &&
      !(is_linked_to(ch, victim, LNK_CONSENT) || (IS_PC_PET(ch) && GET_MASTER(ch) == victim)
      || (IS_PC_PET(ch) && is_linked_to(GET_MASTER(ch), victim, LNK_CONSENT) )) )
    {
      return;
    }
  }
  
  if(ch == victim)
  {
    send_to_char
      ("&+WYou feel strength and power courses through your veins!\r\n", ch);
  }
  else
  {
    act("&+WSuddenly, a feeling of strength and power courses through your veins!&n",
       FALSE, ch, 0, victim, TO_VICT);
  }

  act("$n&n appears to grow tougher and more powerful.",
    TRUE, victim, 0, 0, TO_ROOM);
    
  if(GET_CLASS(ch, CLASS_SHAMAN))
    duration = level;
  else
    duration = (int)(level / 2 + 1);

  if(IS_PC(ch) ||
     IS_PC(victim) ||
     IS_PC_PET(ch) ||
     IS_PC_PET(victim))
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_LIONRAGE;
    af.duration = duration;

    af.location = APPLY_AGI;
    af.modifier = -10 + (number(-3, 3));

    affect_to_char(victim, &af);

    af.location = APPLY_STR;
    af.modifier = (int)((level / 5) + 1);
    
    af.location = APPLY_DAMROLL;
    af.modifier = (int)((level / 10) + 1);

    affect_to_char(victim, &af);
  }
  else
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_LIONRAGE;
    af.duration = 100;

    af.location = APPLY_STR;
    af.modifier = (int) ((level / 5) + 1);

    affect_to_char(victim, &af);
    
    af.location = APPLY_DAMROLL;
    af.modifier = (int)((level / 10) + 1);

    affect_to_char(victim, &af);
  }
}


void spell_ravenflight(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  if(IS_ANIMALIST(ch))
    level += number(10, (int)(GET_LEVEL(ch) / 2));
    
  if(!affected_by_spell(victim, SPELL_RAVENFLIGHT))
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_RAVENFLIGHT;
    af.duration = level;
    af.bitvector = AFF_FLY;

    if(!IS_AFFECTED(victim, AFF_FLY))
    {
      send_to_char
        ("&+yYou feel a strange sensation in the back of your shoulders.\n",
         victim);
      send_to_char
        ("&+ySuddenly, you feel yourself floating up into the air, free as a bird!\n",
         victim);
      act("$n&n&+y floats up into the air, free as a bird!", FALSE, victim, 0,
          0, TO_ROOM);
    }
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_RAVENFLIGHT)
      {
        af1->duration = level;
      }
  }
}

void spell_greater_ravenflight(int level, P_char ch, char *arg, int type,
                               P_char victim, P_obj obj)
{
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
        
  LOOP_THRU_PEOPLE(victim, ch)
    spell_ravenflight(level, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
}


void spell_call_of_the_wild(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct affected_type af;
  P_char   new_vict;
  int      mob_num;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);

  if(IS_TRUSTED(victim) || resists_spell(ch, victim) ||
      affected_by_spell(victim, SPELL_CALL_OF_THE_WILD) || IS_NPC(victim) ||
      IS_MORPH(victim) || (!IS_TRUSTED(ch) &&
                           !is_linked_to(ch, victim, LNK_CONSENT) &&
                           NewSaves(victim, SAVING_FEAR, 2)))
  {
    send_to_char("Nothing seems to happen.\n", ch);

    if(IS_NPC(victim) && CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
        MobStartFight(victim, ch);
    }
    return;
  }
  mob_num = number(1051, 1059);

  if((mob_num = real_mobile0(mob_num)) == 0)
  {
    send_to_char
      ("Oops!  There is an error in the list of this type of mob!\n", ch);
    logit(LOG_DEBUG,
          "Call of the wild: Error list shape data for mob numb %d", mob_num);
    return;
  }
  if(affected_by_spell(victim, SPELL_VITALITY))
  {
    affect_from_char(victim, SPELL_VITALITY);

    if(affect_total(victim, TRUE))
      return;
  }
  new_vict = morph(victim, mob_num, REAL);
  if(!new_vict)
    return;                     /* nutty */

  act("$N suddenly changes shape, reforming into $n!", FALSE, new_vict, 0,
      victim, TO_ROOM);
  act("\nYou've been changed into $n!", FALSE, new_vict, 0, 0, TO_CHAR);      /* extra CR/LF needed */

  bzero(&af, sizeof(af));

  af.type = SPELL_CALL_OF_THE_WILD;
  af.duration = (level / 25) + 1;
  af.bitvector4 = AFF4_NO_UNMORPH;

  affect_to_char(new_vict, &af);
}

void spell_malison(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  if(affected_by_spell(victim, SPELL_MALISON))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }

  if(IS_NPC(victim) &&
    CAN_SEE(victim, ch) &&
    (IS_PC(ch) ||
    !IS_PC_PET(ch)))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }

  if(NewSaves(victim, SAVING_SPELL, 2))
  {
	send_to_char("Your victim has saved against your malison spell!&n\n", ch);  
  	return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_MALISON;
  af.duration = (level / 3) + 1;

  af.location = APPLY_CURSE;
  af.modifier = (int) (level / 6);
  if(af.modifier <= 0)
    af.modifier = 1;

  affect_to_char(victim, &af);

  act("$n&n&+L briefly fades from sight, then reappears.&n",
    TRUE, victim, 0, 0, TO_ROOM);
/*  act ("$N&n&+L briefly fades from sight, then reappears.&n", FALSE, ch, 0, victim, TO_CHAR); */
  act("&+LThe world around you briefly fades, then quickly reappears.",
    FALSE, ch, 0, victim, TO_VICT);
  act("You suddenly feel less protected.", FALSE, ch, 0, victim, TO_VICT);

}


void spell_purify_spirit(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type *af, *next;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);

  act("$n&n&+W is briefly surrounded by a white aura.",
    TRUE, victim, 0, 0, TO_ROOM);
  act("&+WYou are briefly surrounded by a white aura.",
    TRUE, ch, 0, victim, TO_VICT);

  poison_common_remove(victim);

  if(affected_by_spell(victim, SPELL_DISEASE) ||
      affected_by_spell(victim, SPELL_PLAGUE))
  {
    affect_from_char(victim, SPELL_DISEASE);
    affect_from_char(victim, SPELL_PLAGUE);
    send_to_char("&+GThe rot afflicting you was removed.\r\n", victim);
  }

  if(affected_by_spell(victim, SPELL_CURSE))
    affect_from_char(victim, SPELL_CURSE);

  if(affected_by_spell(victim, SPELL_MALISON))
    affect_from_char(victim, SPELL_MALISON);

  if(IS_AFFECTED(victim, AFF_BLIND))
  {
    affect_from_char(victim, SPELL_BLINDNESS);
    REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
    send_to_char("&+WYou can see once again!\r\n", victim);
  }
  
  if(get_scheduled(victim, event_torment_spirits))
  {
    send_to_char("&+LThe maligned spirits cease attacking your inner self.\r\n", victim);
    disarm_char_nevents(victim, event_torment_spirits);
  }
}

void spell_spirit_sight(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  do_point(ch, victim);

  if(affected_by_spell(victim, SPELL_SPIRIT_SIGHT))
  {
    struct affected_type *af1;
    bool msg = false;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SPIRIT_SIGHT)
      {
        af1->duration = (level) + 3;
        msg = true;
      }
    
    if(msg)
    {
      act("&+WYour eyes glow and re-focus upon the realm of &+yspirits.&n",
          TRUE, ch, 0, victim, TO_VICT);
      return;
    }
    return;
  }
  
  bzero(&af, sizeof(af));

  af.duration = (level + 3);

  af.type = SPELL_SPIRIT_SIGHT;

  af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;

  if(GET_LEVEL(ch) > 53 &&
     !IS_AFFECTED(victim, AFF_DETECT_INVISIBLE))
  {
    af.bitvector = AFF_DETECT_INVISIBLE;
  }

  affect_to_char(victim, &af);
  send_to_char("Your eyes tingle, dim white spots floating just outside your field of view.\n",
     victim);
}


void spell_greater_spirit_sight(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !(victim))
      return;
  
  do_point(ch, victim);
    
  if(affected_by_spell(victim, SPELL_GREATER_SPIRIT_SIGHT))
  {
    struct affected_type *af1;
    bool msg = false;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_GREATER_SPIRIT_SIGHT)
      {
        af1->duration = (level) + 3;
        msg = true;
      }
      
    if(msg)
    {
      act("&+WYour visions of the &+yspirits&+W are refreshed!",
          TRUE, ch, 0, victim, TO_VICT);
      return;
    }
  }
  
  bzero(&af, sizeof(af));

  af.duration = (level + 3);

  af.type = SPELL_GREATER_SPIRIT_SIGHT;

  af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
  
  af.bitvector = AFF_DETECT_INVISIBLE;

  affect_to_char(victim, &af);

  send_to_char
    ("&+WYour eyes tingle and bright white spots float just outside your field of view.\n",
     victim);
}

void spell_sense_spirit(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;
  int duration;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
          
  do_point(ch, victim);
  
  if(!(GET_CLASS(ch, CLASS_SHAMAN)))
    duration = (int)(level / 2);
  else
    duration = level;
  
  if(!affected_by_spell(victim, SPELL_SENSE_SPIRIT))
  {
    send_to_char
      ("&+WYou feel your sense of the spirit in every thinking being improve.\n",
       victim);

    bzero(&af, sizeof(af));
    af.type = SPELL_SENSE_SPIRIT;

    af.duration = (duration + 5);
    af.bitvector = AFF_SENSE_LIFE;

    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SENSE_SPIRIT)
      {
        af1->duration = (duration) + 5;
        send_to_char("&+WYour sense of the spirit realm improves.\r\n", victim);
      }
  }
}

int can_summon_beast(P_char ch, int level)
{
  struct follow_type *k;
  int i, j, room, charisma = GET_C_CHA(ch);
  P_char   victim;

  if( !IS_ALIVE(ch) || !(ch->in_room) || IS_PC_PET(ch) )
  {
    return FALSE;
  }

  if( CHAR_IN_SAFE_ZONE(ch) )
  {
    send_to_char("A mysterious force blocks your conjuring!\n", ch);
    return FALSE;
  }

  for( k = ch->followers, i = 0, j = 0; k; k = k->next )
  {
    victim = k->follower;

    if( IS_ELEMENTAL(victim) || IS_UNDEADRACE(victim) )
    {
      send_to_char("You must fully attune yourself to the animal spirit. Abandon your otherworldly creatures.\r\n", ch);
      return FALSE;
    }
  }

  for( k = ch->followers, i = 0; k; k = k->next )
  {
    if( k->follower )
    {
      if( IS_NPC(k->follower) &&
        ((GET_VNUM(k->follower) >= 1051 && GET_VNUM(k->follower) <= 1062 ) ||
        (GET_VNUM(k->follower) >= 19 && GET_VNUM(k->follower) <= 29 )))
      {
        i++;
      }
    }
  }

  if( GET_SPEC(ch, CLASS_SHAMAN, SPEC_ANIMALIST) )
    charisma += GET_LEVEL(ch);

  if( i > MAX(1, (int)(charisma / 30)) )
  {
    return FALSE;
  }

  return TRUE;
}

P_char summon_beast_common(int mobnumb, P_char ch, int max_summon,
                           int dur, const char *appearsC,
                           const char *appears, int level, bool is_greater)
{
  struct affected_type af;
  P_char   mob;
  int j, sum, numb;
  int life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);

  if(!ch || (mobnumb < 0) || (dur <= 0) || (max_summon <= 0))
  {
    return NULL;
  }

  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your summoning!\n", ch);
    return NULL;
  }

  if( !can_summon_beast(ch, level) )
  {
    send_to_char("You cannot bind any more creatures to your control.\n", ch);
    return NULL;
  }

  mob = read_mobile(real_mobile(mobnumb), REAL);

  if( !mob )
  {
    logit(LOG_DEBUG, "summon_beast_common(): mob %d not loadable", mobnumb);
    send_to_char("Bug in summon_beast_common.  Tell a god!\n", ch);
    return NULL;
  }

  char_to_room(mob, ch->in_room, 0);

  if( GET_SPEC(ch, CLASS_SHAMAN, SPEC_ANIMALIST) || IS_ELITE(ch) )
  {
    if( is_greater )
    {
      sum = dice(level, 16) + (life * 5);
    }
    else
    {
      sum = dice(level, 12) + (life * 2);
    }
  }
  else if( is_greater )
  {
    sum = (int)((dice(level, 16) + (life * 5)) * 0.66);
  }
  else
  {
    sum = (int)((dice(level, 12) + (life * 2)) * 0.66);
  }

  if( !is_greater )
  {
    while( mob->affected )
    {
      affect_remove(mob, mob->affected);
    }
  }

  if( is_greater && IS_ANIMALIST(ch) )
  {
    sum = (int)(sum * 1.75);
    mob->player.level = GET_LEVEL(mob) + (int)(GET_LEVEL(ch) / 10 + life / 10 + number(0, 5));
    SET_BIT(mob->specials.affected_by, AFF_HASTE);
  }
  else if( is_greater && life )
  {
    mob->player.level = GET_LEVEL(mob) + (int)(life / 15) + number(0, 2);
  }
  else if( life )
  {
    mob->player.level = GET_LEVEL(mob) + (int)(life / 20) + number(0, 2);
  }

  if( !IS_SET(mob->specials.act, ACT_MEMORY) )
  {
    clearMemory(mob);
  }

  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = sum;

  GET_PLATINUM(mob) = 0;
  GET_GOLD(mob) = 0;
  GET_SILVER(mob) = 0;
  GET_COPPER(mob) = 0;

  if( GET_SPEC(ch, CLASS_SHAMAN, SPEC_ANIMALIST) || IS_ELITE(ch) )
  {
    mob->points.base_hitroll = mob->points.hitroll = (int)(GET_LEVEL(mob) / 1.2);
    mob->points.base_damroll = mob->points.damroll = (int)(GET_LEVEL(mob) / 1.2);
  }
  else
  {
    mob->points.base_hitroll = mob->points.hitroll = (int)(GET_LEVEL(mob) / 2.5);
    mob->points.base_damroll = mob->points.damroll = (int)(GET_LEVEL(mob) / 2.5);
  }

  MonkSetSpecialDie(mob);

  if( GET_SPEC(ch, CLASS_SHAMAN, SPEC_ANIMALIST) || IS_ELITE(ch) )
  {
    ch->points.damsizedice = (int)(1.5 * ch->points.damsizedice);
  }

  GET_EXP(mob) = 0;

  if( (mobnumb == 1060) || (mobnumb == 1053) || (mobnumb == 1054)
    || (mobnumb == 1060) || (mobnumb == 19) || (mobnumb == 27) )
  {
    if(!IS_SET(mob->specials.act, ACT_MOUNT))
    {
      SET_BIT(mob->specials.act, ACT_MOUNT);
    }
  }

  balance_affects(mob);

  act(appears, FALSE, ch, 0, ch, TO_ROOM);
  act(appearsC, FALSE, ch, 0, 0, TO_CHAR);

  apply_achievement(mob, TAG_CONJURED_PET);
  setup_pet(mob, ch, dur, PET_NOCASH);
  add_follower(mob, ch);

  return mob;
}

void spell_summon_beast(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int rand = number(1, 3), maxsumm = (level / 15) + 1, dur = 20 + (level / 5);

  if( !IS_ALIVE(ch) )
  {
    return;
  }
  if(IS_ANIMALIST(ch))
  {
    dur += GET_LEVEL(ch);
  }

  /* summon beast based on sector type.. */
  switch( world[ch->in_room].sector_type )
  {
  case SECT_FIELD:
    switch (rand)
    {
    case 1:
      /* antelope */
      summon_beast_common(1051, ch, maxsumm, dur,
                          "&+yAn antelope from a nearby field ambles up to you.",
                          "&+yAn antelope from a nearby field ambles up to $N.",
                          (level / 3) + 2, FALSE);
      return;
    case 2:
      /* buffalo */
      summon_beast_common(1052, ch, maxsumm, dur,
                          "&+yA buffalo from a nearby field plods up to you.",
                          "&+yA buffalo from a nearby field plods up to $N.",
                          (level / 3) + 3 + dice(2, 2), FALSE);
      return;
    case 3:
      /* wild horse */
      summon_beast_common(1053, ch, maxsumm, dur,
                          "&+yA horse from a nearby field gallops up to you.",
                          "&+yA horse from a nearby field gallops up to $N.",
                          (level / 3) + dice(1, 2), FALSE);
      return;
    default:
      send_to_char("Your attempt to summon a beast results in something nutty. Tell a God.\n", ch);
      return;
    }
  case SECT_FOREST:
  case SECT_SWAMP:
    switch (rand)
    {
    case 1:
      /* bear (gasp) */
      summon_beast_common(1054, ch, maxsumm, dur,
                          "&+LA bear, tearing through nearby underbrush, appears by your side.",
                          "&+LA bear, tearing through nearby underbrush, appears by $N's side.",
                          (level / 3) + 4 + dice(2, 2), FALSE);
      return;
    case 2:
      /* wolf */
      summon_beast_common(1055, ch, maxsumm, dur,
                          "&+yA wolf, appearing from the nearby underbrush, walks up to your side.",
                          "&+yA wolf, appearing from the nearby underbrush, walks up to $N's side.",
                          (level / 3) + 1 + dice(1, 3), FALSE);
      return;
    case 3:
      /* chipmunk - snicker */
      summon_beast_common(1056, ch, maxsumm, dur,
                          "&+yA cute little chipmunk climbs down from a tree and stands beside you.",
                          "&+yA cute little chipmunk climbs down from a tree and stands beside $N.",
                          3 + (level / 10), FALSE);
      return;
    default:
      send_to_char("Your attempt to summon a beast results in some nuttiness. Tell a God.\n", ch);
      return;
    }

  case SECT_HILLS:
  case SECT_MOUNTAIN:
    switch (rand)
    {
    case 1:
      /* mountain goat */
      summon_beast_common(1057, ch, maxsumm, dur,
                          "&+WA mountain goat ambles up to you.",
                          "&+WA mountain goat ambles up to $N's side.",
                          (level / 3) + dice(1, 2), FALSE);
      return;
    case 2:
      /* cougar */
      summon_beast_common(1058, ch, maxsumm, dur,
                          "&+YA well-muscled cougar silently pads up alongside you.",
                          "&+YA well-muscled cougar silently pads up to $N's side.",
                          (level / 3) + 2 + dice(2, 3), FALSE);
      return;
    case 3:
      /* a rock lizard - YES! */
      summon_beast_common(1059, ch, maxsumm, dur,
                          "&+LA little &n&+rrock lizard&+L flits from a nearby rock and stops near your foot.",
                          "&+LA little &n&+rrock lizard&+L flits from a nearby rock, stopping near $N's foot.",
                          2 + (level / 25), FALSE);
      return;
    default:
      send_to_char("Your attempt to summon a beast results in some oddness. Tell a God.\n", ch);
      return;
    }
  case SECT_UNDERWATER:
  case SECT_UNDERWATER_GR:
  case SECT_UNDRWLD_WATER:
  case SECT_UNDRWLD_NOSWIM:
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_OCEAN:
    send_to_char("Not too many water beasts about...\n", ch);
    return;
  case SECT_INSIDE:
  case SECT_CITY:
  case SECT_ROAD:
    send_to_char("You are not close enough to the wilderness to summon a wild beast.\n", ch);
    return;
    /* since so much of the underworld is defined incorrectly anyway..  just let
       them summon wherever they want */
  case SECT_UNDRWLD_CITY:
  case SECT_UNDRWLD_INSIDE:
  case SECT_UNDRWLD_WILD:
    switch( rand )
    {
    case 1:
      /* spider */
      summon_beast_common(1060, ch, maxsumm, dur,
                          "&+LA huge &n&+mcave spider&+L silently walks up beside you.",
                          "&+LA huge &n&+mcave spider&+L silently walks up beside $N.",
                          (level / 3) + 2, FALSE);
      return;
    case 2:
      /* cave lizard */
      summon_beast_common(1061, ch, maxsumm, dur,
                          "&+gA large &+Gcave lizard&n&+g, stepping out of the shadows, stands by your side.",
                          "&+gA large &+Gcave lizard&n&+g, stepping out of the shadows, stands next to $N.",
                          (level / 3) + dice(2, 2), FALSE);
      return;
    case 3:
      /* purple worm */
      summon_beast_common(1062, ch, maxsumm, dur,
                          "&+mA &+Mpurple worm&n&+m burrows out of the ground and slithers up to your leg.",
                          "&+mA &+Mpurple worm&n&+m burrows out of the ground, slithering up to $N.",
                          (level / 3) + 3 + dice(1, 2), FALSE);
      return;

    default:
      send_to_char("Your attempt to summon a beast results in something really nutty. Tell a God.\n", ch);
      return;
    }
  case SECT_UNDRWLD_NOGROUND:
  case SECT_NO_GROUND:
    send_to_char("Summoning something here may prove fatal to its health.\n", ch);
    return;
  case SECT_AIR_PLANE:
  case SECT_FIREPLANE:
  case SECT_WATER_PLANE:
  case SECT_EARTH_PLANE:
    send_to_char("Summoning beasts from an elemental plane?  Hmm.\n", ch);
    return;
  default:
    send_to_char("No beast respond to your summons!\n", ch);
    return;
  }
}

void spell_greater_summon_beast(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int rand = number(1, 3), maxsumm = (level / 15) + 1, dur = level + 10;

  if( !IS_ALIVE(ch) )
    return;

  if(IS_ANIMALIST(ch))
  {
    dur += GET_LEVEL(ch);
  }

  /* summon beast based on sector type.. */
  switch (world[ch->in_room].sector_type)
  {
  case SECT_FIELD:
    switch( rand )
    {
    case 1:
      /* warg */
      summon_beast_common(19, ch, maxsumm, dur,
                          "&+yA warg from a nearby field prowl up to your side.&n",
                          "&+yA warg from a nearby field prowl up to $N's side.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    case 2:
      /* eagle */
      summon_beast_common(20, ch, maxsumm, dur,
                          "&+yA majestic eagle soars down and lands on your shoulder.&n",
                          "&+yA majestic eagle soars down and lands on $N's shoulder.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    case 3:
      /* buffalo */
      summon_beast_common(21, ch, maxsumm, dur,
                          "&+yA buffalo from a nearby field plods up to you.",
                          "&+yA buffalo from a nearby field plods up to $N.",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    default:
      send_to_char("Your attempt to summon a beast results in something nutty. Tell a God.\n", ch);
      return;
    }

  case SECT_FOREST:
  case SECT_SWAMP:
    switch (rand)
    {
    case 1:
    case 2:
      /* owlbear */
      summon_beast_common(29, ch, maxsumm, dur,
                          "&+LAn owlbear, tearing through nearby underbrush, appears by your side.&n",
                          "&+LAn owlbear, tearing through nearby underbrush, appears by $N's side.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    case 3:
      /* treant */
      summon_beast_common(28, ch, maxsumm, dur,
                          "&+yA huge treant, crashing through the nearby underbrush, walks up to your side.&n",
                          "&+yA huge treant, crashing through the nearby underbrush, walks up to $N's side.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    default:
      send_to_char("Your attempt to summon a beast results in some nuttiness. Tell a God.\n", ch);
      return;
    }

  case SECT_HILLS:
  case SECT_MOUNTAIN:
    switch (rand)
    {
    case 1:
      /* rock beetle */
      summon_beast_common(25, ch, maxsumm, dur,
                          "&+wA swarm of rock beetles scurry up to you from under a nearby rock.&n",
                          "&+wA swarm of rock beetles scurry up to $N from under a nearby rock.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    case 2:
      /* mountain lion */
      summon_beast_common(26, ch, maxsumm, dur,
                          "&+YA well-muscled mountain lion silently pads up alongside you.&n",
                          "&+YA well-muscled mountain lion silently pads up alongside $N.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    case 3:
      /* wyvern */
      summon_beast_common(27, ch, maxsumm, dur,
                          "&+LA wyvern emerges from a nearby cavern glaring at you.&n",
                          "&+LA wyvern emerges from a nearby cavern glaring at $N.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    default:
      send_to_char("Your attempt to summon a beast results in some oddness. Tell a God.\n", ch);
      return;
    }
  case SECT_UNDERWATER:
  case SECT_UNDERWATER_GR:
  case SECT_UNDRWLD_WATER:
  case SECT_UNDRWLD_NOSWIM:
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_OCEAN:
    send_to_char("Not too many water beasts about...\n", ch);
    return;
  case SECT_INSIDE:
  case SECT_CITY:
  case SECT_ROAD:
    send_to_char("You are not close enough to the wilderness to summon a wild beast.\n", ch);
    return;
    /* since so much of the underworld is defined incorrectly anyway..  just let
       them summon wherever they want */
  case SECT_UNDRWLD_CITY:
  case SECT_UNDRWLD_INSIDE:
  case SECT_UNDRWLD_WILD:
    switch (rand)
    {
    case 1:
      /* tunnel worm */
      summon_beast_common(22, ch, maxsumm, dur,
                          "&+mA &+Mhuge tunnel worm&+m burrows out of the ground near you.&n",
                          "&+mA &+Mhuge tunnel worm&+m burrows out of the ground near $N.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    case 2:
      /* hook horror */
      summon_beast_common(23, ch, maxsumm, dur,
                          "&+LA &+wghastly &+Lhook horror skulks up to you.&n",
                          "&+LA &+wghastly &+Lhook horror skulks up to $N.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    case 3:
      /* roper */
      summon_beast_common(24, ch, maxsumm, dur,
                          "&+gA small &+Groper &+g, tentacles flailing, slithers toward you.&n",
                          "&+gA small &+Groper &+g, tentacles flailing, slithers toward $N.&n",
                          (level / 2) + 3 + dice(4, 3), TRUE);
      return;
    default:
      send_to_char("Your attempt to summon a beast results in something nutty. Tell a God.\n", ch);
      return;
    }

  case SECT_UNDRWLD_NOGROUND:
  case SECT_NO_GROUND:
    send_to_char
      ("Summoning something here may prove fatal to its health.\n", ch);
    return;
  case SECT_AIR_PLANE:
  case SECT_FIREPLANE:
  case SECT_WATER_PLANE:
  case SECT_EARTH_PLANE:
    send_to_char("Summoning beasts from an elemental plane?  Hmm.\n", ch);
    return;

  default:
    send_to_char("No beast respond to your summons!\n", ch);
    return;
  }
}

void spell_summon_spirit(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  send_to_char("This spell is not yet implemented.\n", ch);
  send_to_char( "&+gLohr pops up and goes, \"Eee Hee!\"!&n", ch );
}

void spell_wellness(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  P_char tch;
  int gain, in_room;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  gain = (int)(dice(4, 7 + (level / 10)));

  if( has_innate(ch, INNATE_IMPROVED_HEAL) )
  {
    gain += GET_LEVEL(ch);
  }
  act("&+GYou bring forth a sphere of pure healing energy.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n&+W brings forth a sphere of pure healing energy.", TRUE, ch, 0, 0, TO_ROOM);
  if( IS_SPIRITUALIST(ch) && GET_LEVEL(ch) > 53 && ch->group )
  {
    for( struct group_list *gl = ch->group; gl; gl = gl->next )
    {
      if( gl->ch->in_room == ch->in_room )
      {
        if( GET_HIT(gl->ch) < GET_MAX_HIT(gl->ch) )
        {
          heal(gl->ch, ch, gain, GET_MAX_HIT(gl->ch) - number(1,4));
          update_pos(gl->ch);
          send_to_char("&+WYou feel slightly better.\r\n", gl->ch);
        }
      }
    }
  }
  else
  {
    for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
    {
      if( GET_HIT(tch) >= GET_MAX_HIT(tch) )
      {
        continue;
      }

      heal(tch, ch, gain, GET_MAX_HIT(tch));
//      healCondition(tch, gain);
      update_pos(tch);
      send_to_char("&+WYou feel slightly better.\n", tch);
    }
  }
}

void spell_lesser_mending(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int gain;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  gain = (int) (level / 2 + number(0, 5));

  if(has_innate(ch, INNATE_IMPROVED_HEAL))
    gain += (int) (number((int) (level / 2), (int) (level * 1.5)));

  do_point(ch, victim);

  if(IS_AFFECTED4(victim, AFF4_REV_POLARITY))
  {
    act("&+L$N&+L wavers under the immense rush of life into $S body!", FALSE,
        ch, 0, victim, TO_CHAR);
    act("&+LYou waver under the immense rush of life into your body!", FALSE,
        ch, 0, victim, TO_VICT);
    act("&+L$N&+L wavers under the immense rush of life into $S body!", FALSE,
        ch, 0, victim, TO_NOTVICT);
    damage(ch, victim, gain, TYPE_UNDEFINED);
    return;
  }

  act("$N &+wis surrounded by a flashing &+ggreen &+waura!&n", FALSE, ch, 0, victim, TO_CHAR);
  act("&+wA light &+ggreen &+waura flashes around you and your wounds start to heal!&n", FALSE,
      ch, 0, victim, TO_VICT);
  act("$N &+wis surrounded by a flashing &+ggreen &+waura!", FALSE, ch, 0, victim, TO_NOTVICT);

  heal(victim, ch, gain, GET_MAX_HIT(victim));
//  healCondition(victim, gain);
  update_pos(victim);

  send_to_char("&+WYou feel a bit better.\n", victim);
}

void spell_mending(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  int gain;



  gain = (int) (level * 5 / 2.5 + number(-20, 20));

  if(has_innate(ch, INNATE_IMPROVED_HEAL))
    gain += (int) (number((int) (level / 2), (int) (level)));

  do_point(ch, victim);
  
  if(IS_AFFECTED4(victim, AFF4_REV_POLARITY))
  {
    act("&+L$N&+L wavers under the immense rush of life into $S body!", FALSE,
        ch, 0, victim, TO_CHAR);
    act("&+LYou waver under the immense rush of life into your body!", FALSE,
        ch, 0, victim, TO_VICT);
    act("&+L$N&+L wavers under the immense rush of life into $S body!", FALSE,
        ch, 0, victim, TO_NOTVICT);
    damage(ch, victim, gain, TYPE_UNDEFINED);
    
    return;
  }

  act("$N &+wis surrounded by a glowing &+Ggreen &+waura!&n",
    FALSE, ch, 0, victim, TO_CHAR);
  act("&+wA &+Ggreen &+waura surrounds you for a moment and your wounds start to heal!&n",
    FALSE, ch, 0, victim, TO_VICT);
  act("$N &+wis surrounded by a glowing &+Ggreen &+waura!",
    FALSE, ch, 0, victim, TO_NOTVICT);

  heal(victim, ch, gain, GET_MAX_HIT(victim));
//  healCondition(victim, gain);

  update_pos(victim);
}

void spell_greater_mending(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  int gain;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
      
  do_point(ch, victim);

  gain = (int) (level * 5 / 1.75 + number(-40, 40));

// Spiritualists
  if(has_innate(ch, INNATE_IMPROVED_HEAL))
  {
    gain += (int) (number((int) (level / 2), (int) (level)));
    if(level > 46)
    {
      if(affected_by_spell(victim, SPELL_DISEASE) ||
         affected_by_spell(victim, SPELL_PLAGUE))
      {
        affect_from_char(victim, SPELL_DISEASE);
        affect_from_char(victim, SPELL_PLAGUE);
      }
    }
  }

// All Shamans
  if(level > 46)
  {
    if(affected_by_spell(victim, TAG_ARMLOCK))
        affect_from_char(victim, TAG_ARMLOCK);
        
    if(affected_by_spell(victim, TAG_LEGLOCK))
        affect_from_char(victim, TAG_LEGLOCK);
  }

  if(affected_by_spell(victim, SPELL_BLINDNESS) &&
    level > 36)
  {
    if(affected_by_spell(victim, SPELL_BLINDNESS))
      affect_from_char(victim, SPELL_BLINDNESS);
    
    if(IS_SET(victim->specials.affected_by, AFF_BLIND))
      REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
  }

  if(IS_AFFECTED4(victim, AFF4_REV_POLARITY))
  {
    act("&+L$N&+L wavers under the immense rush of life into $S body!", 
      FALSE, ch, 0, victim, TO_CHAR);
    act("&+LYou waver under the immense rush of life into your body!",
      FALSE, ch, 0, victim, TO_VICT);
    act("&+L$N&+L wavers under the immense rush of life into $S body!",
      FALSE, ch, 0, victim, TO_NOTVICT);
    damage(ch, victim, gain, TYPE_UNDEFINED);
    return;
  }

  heal(victim, ch, gain, GET_MAX_HIT(victim));
//  healCondition(victim, gain);
  update_pos(victim);

  act("$N &+wshudders as an immense rush of &+Glife &+wflows into $S body!&n",
    FALSE, ch, 0, victim, TO_CHAR);
  act("&+wYou shudder as an immense rush of &+Glife &+wflows into your body!&n",
    FALSE, ch, 0, victim, TO_VICT);
  act("$N &+wshudders as an immense rush of &+Glife &+wflows into $S body!",
    FALSE, ch, 0, victim, TO_NOTVICT);
}

void spell_spirit_ward(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;
  int duration;
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  do_point(ch, victim);
  
  if(IS_SPIRITUALIST(ch))
    duration = level + 10;
  else if(GET_CLASS(ch, CLASS_SHAMAN))
    duration = level;
  else
    duration = (int)(level / 2);

  if(affected_by_spell(victim, SPELL_SPIRIT_WARD))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SPIRIT_WARD)
      {
        af1->duration = duration;
      }
    return;
  }
  
  if(IS_AFFECTED3(victim, AFF3_SPIRIT_WARD) ||
     IS_AFFECTED3(victim, AFF3_GR_SPIRIT_WARD))
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }
  
  act("$n&+W glows dimly as a faint white halo surrounds $m.",
    TRUE, victim, 0, 0, TO_ROOM);
  act("&+WYou begin to glow dimly as a faint white halo surrounds you.",
    FALSE, victim, 0, 0, TO_CHAR);

  bzero(&af, sizeof(af));

  af.type = SPELL_SPIRIT_WARD;
  af.duration = duration;
  af.bitvector3 = AFF3_SPIRIT_WARD;

  affect_to_char(victim, &af);
}

void spell_greater_spirit_ward(int level, P_char ch, char *arg, int type,
                               P_char victim, P_obj obj)
{
  struct affected_type af;
  int duration;
  
  do_point(ch, victim);
  
  if(IS_SPIRITUALIST(ch))
    duration = level + 10;
  else if(GET_CLASS(ch, CLASS_SHAMAN))
    duration = level;
  else
    duration = (int)(level / 2);

  if(affected_by_spell(victim, SPELL_GREATER_SPIRIT_WARD))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_GREATER_SPIRIT_WARD)
      {
        af1->duration = duration;
      }
    return;
  }
  
  if(IS_AFFECTED3(victim, AFF3_GR_SPIRIT_WARD))
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }
  act("$n&+W glows visibly as a faint white halo surrounds $m.", TRUE, victim,
      0, 0, TO_ROOM);
  act("&+WYou begin to glow as a faint white halo surrounds you.", FALSE,
      victim, 0, 0, TO_CHAR);

  bzero(&af, sizeof(af));

  af.type = SPELL_GREATER_SPIRIT_WARD;
  af.duration = duration;
  af.bitvector3 = AFF3_GR_SPIRIT_WARD;

  affect_to_char(victim, &af);
}

void spell_reveal_true_form(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  P_char   tmp_victim;

  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
  
  act("$n&n raises $s arms high into the air, motionless.",
    TRUE, ch, 0, 0, TO_ROOM);
  act("A dim white aura fills the area briefly, centered on $n&n.",
    TRUE, ch, 0, 0, TO_ROOM);
  act("Raising your arms, you cause a dim white aura to fill the area briefly.",
     TRUE, ch, 0, 0, TO_CHAR);

  LOOP_THRU_PEOPLE(tmp_victim, ch)
  {
    if((ch != tmp_victim) &&
        (IS_AFFECTED(tmp_victim, AFF_INVISIBLE) ||
         IS_AFFECTED(tmp_victim, AFF_HIDE) ||
         IS_AFFECTED2(tmp_victim, AFF2_MINOR_INVIS)))
    {
      if(IS_AFFECTED(tmp_victim, AFF_INVISIBLE) ||
        IS_AFFECTED2(tmp_victim, AFF2_MINOR_INVIS))
        affect_from_char(tmp_victim, SPELL_INVISIBLE);

      if(IS_AFFECTED2(tmp_victim, AFF2_MINOR_INVIS))
        REMOVE_BIT(tmp_victim->specials.affected_by2, AFF2_MINOR_INVIS);

      if(IS_AFFECTED(tmp_victim, AFF_INVISIBLE))
        REMOVE_BIT(tmp_victim->specials.affected_by, AFF_INVISIBLE);

      if(IS_AFFECTED(tmp_victim, AFF_HIDE))
      {
        affect_from_char(tmp_victim, SKILL_HIDE);
        REMOVE_BIT(tmp_victim->specials.affected_by, AFF_HIDE);
      }
      act("$n&n appears from hiding as the aura strikes $m.", FALSE,
          tmp_victim, 0, 0, TO_ROOM);
      act("As the aura passes over you, you appear from hiding!", FALSE,
          tmp_victim, 0, 0, TO_CHAR);
    }
    else if((ch != tmp_victim) && 
            IS_DISGUISE(tmp_victim))
    {
      remove_disguise(tmp_victim, TRUE);
    }
  }
}


void spell_cold_ward(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  do_point(ch, victim);
  
  if(!affected_by_spell(victim, SPELL_COLD_WARD))
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_COLD_WARD;
    af.duration = level + 3;
    af.bitvector2 = AFF2_PROT_COLD;
    affect_to_char(victim, &af);

    send_to_char("&+cYou feel protected from the cold!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_COLD_WARD)
      {
        send_to_char("&+cYour cold ward was renewed!\n", victim);
        af1->duration = level + 3;
      }
  }
}

void spell_fire_ward(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
  
  do_point(ch, victim);
  
  if(!affected_by_spell(victim, SPELL_FIRE_WARD))
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_FIRE_WARD;
    af.duration = level + 3;
    af.bitvector = AFF_PROT_FIRE;
    affect_to_char(victim, &af);

    send_to_char("&+rYou feel protected from fire!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_FIRE_WARD)
      {
        send_to_char("&+rYour fire ward was renewed!\n", victim);
        af1->duration = level + 3;
      }
  }
}

void spell_reveal_spirit_essence(int level, P_char ch, char *arg, int type,
                                 P_char victim, P_obj obj)
{
  int      align;
  char     buff[256], buff2[256], buff3[256];

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  do_point(ch, victim);

  align = GET_ALIGNMENT(victim);

  if(align == 1000)
  {
    act("$n&n &+Wbriefly glows pure white.", TRUE, victim, 0, 0, TO_ROOM);
  }
  else if(align >= 750)
  {
    act("$n&n &+Wbriefly glows extremely brightly.", TRUE, victim, 0, 0,
        TO_ROOM);
  }
  else if(align >= 350)
  {
    act("$n&n &+Wbriefly glows brightly.", TRUE, victim, 0, 0, TO_ROOM);
  }
  else if(align > 0)
  {
    act("A feeble white glow briefly surrounds $n&n.", TRUE, victim, 0, 0,
        TO_ROOM);
  }
  else if(align == 0)
  {
    act("Nothing visible happens.", TRUE, victim, 0, 0, TO_ROOM);
  }
  else if(align >= -350)
  {
    act("A feeble red glow briefly surrounds $n&n.", TRUE, victim, 0, 0,
        TO_ROOM);
  }
  else if(align >= -750)
  {
    act("&+rA bright red glow briefly surrounds $n&n&+r.", TRUE, victim, 0, 0,
        TO_ROOM);
  }
  else if(align > -1000)
  {
    act("&+RAn extremely bright red glow briefly surrounds $n&+R.", TRUE,
        victim, 0, 0, TO_ROOM);
  }
  else if(align == -1000)
  {
    act
      ("$n &+Rbriefly glows pure red, engulfing the area in an unholy light.",
       TRUE, victim, 0, 0, TO_ROOM);
  }
  else
  {
    send_to_char("something strange..  notify a coder\n", ch);
  }

  /* let the victim know he's been surrounded by a glow..  but only if he has */

  buff3[0] = '\0';

  if(align >= 350)
  {
    strcpy(buff, "white");
    strcpy(buff3, "&+W");
  }
  else if(align <= -350)
  {
    strcpy(buff, "red");
    strcpy(buff3, "&+R");
  }
  if(align >= 350 || align <= -350)
  {
    sprintf(buff2, "%sA %s glow surrounds you briefly.\n", buff3, buff);
  }
  else
  {
    buff2[0] = '\0';
  }

  if(buff2[0])
    send_to_char(buff2, victim);

  /* let's make the victim do some nifty stuff, shall we? */

  if(IS_NPC(victim))
  {
    if(IS_HUMANOID(victim) || IS_GIANT(victim))
    {
      switch (number(0, 15))
      {
      case 1:
        do_action(victim, 0, CMD_PEER);
        break;
      case 2:
        do_action(victim, 0, CMD_EYEBROW);
        break;
      case 3:
        do_action(victim, 0, CMD_GLARE);
        break;
      }
    }
    else if(IS_UNDEADRACE(victim))
    {
      switch (number(0, 7))
      {
      case 1:
        do_action(victim, 0, CMD_MOAN);
        break;
      }
    }
    else if(IS_DEMON(victim))
    {
      switch (number(0, 5))
      {
      case 1:
        do_action(victim, 0, CMD_GROWL);
        break;
      }
    }
  }
}


void spell_spirit_jump(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      location, in_room, distance;
  char     buf[256] = { 0 };
  P_char   tmp = NULL;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  if(IS_PC(ch) &&
     !IS_SPIRITUALIST(ch))
    CharWait(ch, 42);
  else
    CharWait(ch, 12);

  if(!(victim) ||
     !(victim->in_room) ||
     victim->in_room == NOWHERE)
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  if(IS_AFFECTED3(victim, AFF3_NON_DETECTION) ||
      IS_HOMETOWN(ch->in_room) ||
      IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
      world[victim->in_room].sector_type == SECT_CASTLE ||
      world[victim->in_room].sector_type == SECT_CASTLE_WALL ||
      world[victim->in_room].sector_type == SECT_CASTLE_GATE ||
      (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE)
       && !is_introd(victim, ch)))
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  P_char rider = get_linking_char(victim, LNK_RIDING);
  if(IS_NPC(victim) && rider)
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  location = victim->in_room;

  if(IS_SET(world[location].room_flags, NO_TELEPORT) ||
      IS_HOMETOWN(location) ||
      racewar(ch, victim) || (GET_MASTER(ch) && IS_PC(victim)))
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  
  distance = (int)(level * 1.35);
  
  if(IS_SPIRITUALIST(ch))
    distance += 15;
  
  if(!IS_TRUSTED(ch) &&
      (how_close(ch->in_room, victim->in_room, distance) < 0) &&
      (how_close(victim->in_room, ch->in_room, distance) < 0))
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  
  if(ch &&
     !is_Raidable(ch, 0, 0))
  {
    send_to_char("&+WYou are not raidable. The spell fails!\r\n", ch);
    return;
  }
  
  if(victim &&
     IS_PC(ch) &&
     IS_PC(victim) &&
     !is_Raidable(victim, 0, 0))
  {
    send_to_char("&+WYour target is not raidable. The spell fails!\r\n", ch);
    return;
  }
  
  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if((IS_AFFECTED(tmp, AFF_BLIND) ||
         (tmp->specials.z_cord != ch->specials.z_cord) ||
         (tmp == ch) || !number(0, 5)) && (IS_PC(ch) && !IS_TRUSTED(ch)))
      continue;
    if(CAN_SEE(tmp, ch))
    {
      if(IS_SPIRITUALIST(ch))
      {
        strcpy(buf, "&+Y$n &+Ydisappears in a &+Wradiant &=LYflash&n &+Yof light.");
      }
      else
        strcpy(buf, "&+Y$n &+Ydisappears in a bright flash of light.");
    }
    else
      strcpy(buf, "&+YA bright flash of light briefly lights the area.\n");
    
    act(buf, FALSE, ch, 0, tmp, TO_VICT);
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  char_from_room(ch);
  char_to_room(ch, location, -1);

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if((IS_AFFECTED(tmp, AFF_BLIND) || (tmp == ch) || !number(0, 5)) &&
        (IS_PC(ch) && !IS_TRUSTED(ch)))
      continue;
    if(CAN_SEE(tmp, ch))
    {
      if(IS_SPIRITUALIST(ch))
      {
        strcpy(buf, "&+Y$n &+Yappears in a &+Wradiant &=LYflash&n &+Yof light.");
      }
      else 
        strcpy(buf, "&+Y$n &+Yappears in a bright flash of light.");
    }
    else
      strcpy(buf, "&+YA bright flash of light briefly lights the area.");
    
    act(buf, FALSE, ch, 0, tmp, TO_VICT);
  }
}

/* Beastform is basicly self-only call_of_the_wild. A weak version of shapeshift */

void spell_beastform(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;
  P_char   new_vict;
  int      mob_num;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  if(affected_by_spell(ch, SPELL_CALL_OF_THE_WILD))
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }

  mob_num = number(1051, 1059);

  if((mob_num = real_mobile0(mob_num)) == 0)
  {
    send_to_char
      ("&+WOops!  There is an error in the list of this type of mob! Tell a god immediately!&n\n",
       ch);
    logit(LOG_DEBUG,
          "Beastform/Call of the wild: Error list shape data for mob numb %d",
          mob_num);
    return;
  }

  if(affected_by_spell(victim, SPELL_VITALITY))
  {
    affect_from_char(victim, SPELL_VITALITY);

    if(affect_total(victim, TRUE))
      return;
  }

  new_vict = morph(ch, mob_num, REAL);

  if(!new_vict)
    return;

  act
    ("&+G$n &+Gutters some incantations and suddenly his body starts twisting and bending, reforming as a wild beast!",
     TRUE, ch, 0, 0, TO_ROOM);
  send_to_char("&+GYou assume the shape of a wild beast...&n\n", ch);

  bzero(&af, sizeof(af));

  af.type = SPELL_CALL_OF_THE_WILD;
  af.duration = (level / 2) + 1;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;

  affect_to_char(new_vict, &af);
}

void spell_indomitability(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!ch || !victim)
    return;

  if(!affected_by_spell(victim, SPELL_INDOMITABILITY))
  {
    bzero(&af, sizeof(af));

    af.type = SPELL_INDOMITABILITY;
    af.duration = level / 10;
    af.bitvector4 = AFF4_NOFEAR;
    affect_to_char(victim, &af);

    act("&+WYour spiritual senses extend into realms beyond the grave...beyond your greatest fears.  Timidity and doubt fall away, replaced with tremendous courage.&N",
       FALSE, victim, 0, ch, TO_CHAR);
    act("&+WAn aura of valor and resolution seems to emanate from $n&N.",
        TRUE, victim, 0, ch, TO_ROOM);
  }
}

void event_firebrand(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      level, dam;
  struct affected_type *af;
  struct damage_messages messages = {
    "&+rF&+Rl&+yam&+Re&+rs&+L consume $N&+L, causing $M to writhe in delicious agony!&n",
    "&+L$n&+L's wicked &+rf&+Rl&+yam&+Re&+rs &+Lcontinue to consume your body; the pain, it almost feels...good.&n",
    "&+rF&+Rl&+yam&+Re&+rs&+L consume $N&+L, causing $M to writhe in delicious agony!&n",
    "&+rF&+Rl&+yam&+Re&+rs&+L completely consume $N&+L, leaving nothing but a smoldering carcass!&n",
    "&+LThe sweet agony finally subsides, as the &+rf&+Rl&+yam&+Re&+rs&+L usher you off to the cold sleep of death.",
    "&+rF&+Rl&+yam&+Re&+rs&+L completely consume $N&+L, leaving nothing but a smoldering carcass!&n", 0
  };

  level = *((int *) data);

  for (af = victim->affected; af && af->type != SPELL_FIREBRAND; af = af->next)
    ;

  if(af == NULL)
    return;

  if((af->modifier)-- == 0)
  {
    send_to_char("&+RThe flames slowly subside.&N\n", victim);
    affect_from_char(victim, SPELL_FIREBRAND);
    return;
  }

  if(resists_spell(ch, victim) &&
     number(0, 1))
      return;
  
  dam = dice(level , 4);

  if(spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages)
      == DAM_NONEDEAD)
  {
    if(6 > number(1, 10))
    {
      stop_memorizing(victim);
    }
    
    add_event(event_firebrand, PULSE_VIOLENCE, ch, victim, NULL, 0, &level,
              sizeof(level));
  }
}

void spell_firebrand(int level, P_char ch, char *arg, int type, P_char victim,
                      P_obj obj)
{
  int dam;
  struct affected_type *af;

  act("&+rF&+Rl&+yam&+Re&+rs &+Lburst from $n&+L's fingertips, &+Ben&+cc&+wi&+Brc&+cl&+wi&+Bng "
      "$N&+L in a towering &+WIn&+rfEr&+WNo&+L!&n", TRUE, ch, 0, victim, TO_NOTVICT);
  act("$n&+L's wicked &+rf&+Rl&+yam&+Re&+rs &+Lconsume your body, causing you to"
      "&+Lscream in pain!&n", FALSE, ch, 0, victim, TO_VICT);
  act("&+rF&+Rl&+yam&+Res&+r &+Lburst from your fingertips, &+Ben&+cc&+wi&+Brc&+cli&+Bng "
      "$N&+L in a towering &+WIn&+rfEr&+WNo&+L!&n", TRUE, ch, 0, victim, TO_CHAR);
  
  dam = level * 2 + number(0, 80);

  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, 0);

  if(IS_ALIVE(victim))
  {
    if((af =
         (struct affected_type *) get_spell_from_char(victim,
                                                      SPELL_FIREBRAND)) ==
        NULL)
    {
      struct affected_type new_affect;

      af = &new_affect;
      memset(af, 0, sizeof(new_affect));
      af->type = SPELL_FIREBRAND;
      af->duration = 1;
      af->modifier = 3;
      affect_to_char(victim, af);
      add_event(event_firebrand, 0, ch, victim, NULL, 0, &level, sizeof(level));
    }
    else
    {
      send_to_char("Your victim is already consumed by an &+WIn&+rfEr&+WNo&+L!&n\r\n", ch);
      af->modifier = 3;
    }
    
    engage(ch, victim);
  }
}

void spell_etherportal(int level, P_char ch, char *arg, int type,
              P_char victim, P_obj obj)
{
  struct portal_settings set = {
      780, /* portal type  */
      -1,  /* from room */
      -1,  /* to room */
      0,   /* How many can pass before closes */
      0,   /* Timeout before anyone can enter after open */
      0,   /* Timeout before next person can enter */
      0,   /* Lag person gets when steps out portal */
      0    /* Portal decay timer */
  };
  struct portal_create_messages msg = {
    /*ch   */ "&+YThe portal opens for a brief second and then closes.\n",
    /*ch r */ "&+YA blast of energy appears for a brief second, then vanishes.",
    /*vic  */ 0,
    /*vic r*/ 0,
    /*ch   */ "&+YThe air explodes around you, revealing a glowing portal of energy!\n",
    /*ch r */ "&+YThe air explodes around you, revealing a glowing portal of energy!\n",
    /*vic  */ "&+YThe air around you folds and shifts, finally falling in upon itself!\n",
    /*vic r*/ "&+YThe air around you folds and shifts, finally falling in upon itself!\n",
    /*npc  */ "Can only open a etherportal to another player!\n",
    /*bad  */ 0
  };

  int specBonus = 0;
  set.to_room = victim->in_room;
  int maxToPass          = get_property("portals.etherportal.maxToPass", 5);
  set.init_timeout       = get_property("portals.etherportal.initTimeout", 3);
  set.post_enter_timeout = get_property("portals.etherportal.postEnterTimeout", 0);
  set.post_enter_lag     = get_property("portals.etherportal.postEnterLag", 0);
  set.decay_timer        = get_property("portals.etherportal.decayTimeout", 60 * 2);

  //--------------------------------
  // spec affected changes
  //--------------------------------
  if(GET_SPEC(ch, CLASS_SHAMAN, SPEC_ELEMENTALIST))
  {
    specBonus = 2;
    set.decay_timer = (set.decay_timer / 2) * 3;
  }
  //--------------------------------
  set.throughput = MAX(0, (int)( (ch->player.level-46) )) + number( 2, maxToPass + specBonus);

  if(    !can_do_general_portal(level, ch, victim, &set, &msg)
//  || (!IS_TRUSTED(ch) && (GET_MASTER(ch) && IS_PC(victim)) )
    )
  {
    act(msg.fail_to_caster,      FALSE, ch, 0, 0, TO_CHAR);
    act(msg.fail_to_caster_room, FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if(IS_NPC(victim) && !IS_TRUSTED(ch))
  {
    send_to_char(msg.npc_target_caster, ch);
    return;
  }

  spell_general_portal(level, ch, victim, &set, &msg);
}

void spell_cascading_elemental_beam(int level, P_char ch, char *arg, int type, P_char victim, P_obj object)
{
  int dam, damaged, mod = 0;
  bool deflect = FALSE;
  struct affected_type af;

  struct damage_messages messages = {
  "You unleash &+Wp&+wu&+Wr&+we &+Rel&+rem&+yen&+Ltal en&+yer&+rgy&n at $N.",
  "&+wA &+Wb&+we&+Wa&+wm &nof &+Wp&+wu&+Wr&+we &+Rel&+rem&+yen&+Ltal en&+yer&+rgy&n slices into you!",
  "&+wA &+Wb&+we&+Wa&+wm of &+Wp&+wu&+Wr&+we &+Rel&+rem&+yen&+Ltal en&+yer&+rgy&n slices into $N.&n",
  "&+WP&+wu&+Wr&+we &+Rel&+rem&+yen&+Ltal en&+yer&+rgy&n disects $N into symmetrical pieces&n!",
  "&+WP&+wu&+Wr&+we &+Rel&+rem&+yen&+Ltal en&+yer&+rgy&n disects you into symmetrical pieces.",
  "&+WP&+wu&+Wr&+we &+Rel&+rem&+yen&+Ltal en&+yer&+rgy&n disects $N into symmetrical pieces.", 0
  };

  if(!(ch) ||
    !IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
      return;

  do_point(ch, victim);

  if(IS_AFFECTED4(victim, AFF4_DEFLECT)) // Simple defect toggle.
  {
    deflect = TRUE;
  }

  if(resists_spell(ch, victim)) // Resisting the spell prevents affects and damage.
  {
    return;
  }

  dam = dice((int) (level * 2.0), 7); // Less than iceball and sunray.

  if( GET_LEVEL(ch) >= 56 )
  {
    mod = 2; // NewSave modifier.
  }
  if( GET_LEVEL(ch) >= 60 )
  {
    mod = 4;
  }
  if( IS_NPC(ch) && IS_ELITE(ch) )
  {
    mod += 4;
  }

  if( IS_AFFECTED3(victim, AFF3_COLDSHIELD) && !ENJOYS_FIRE_DAM(victim) )
  {
    spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_NOSHRUG, &messages);
    // Chance to blind victim but affect can be deflected.
    // Same as sunray but with greater victim restrictions.
    if( IS_ALIVE(victim) && !(deflect) )
    {
      if(!NewSaves(victim, SAVING_SPELL, mod))
      {
        blind(ch, victim, (int) (number(level / 3, level)) * WAIT_SEC);
      }
      // Victims with deflect will also deflect blind affect.
    }
    else if( deflect && IS_ALIVE(ch) )
    {
      if(!NewSaves(victim, SAVING_SPELL, mod))
      {
        blind(ch, ch, (int) (number(level / 3, level)) * WAIT_SEC);
      }
    }
  }
  else if( IS_AFFECTED2(victim, AFF2_FIRESHIELD) || ENJOYS_FIRE_DAM(victim) || IS_COLD_VULN(ch) )
  {
    spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_NOSHRUG, &messages);

    if( (IS_ALIVE(victim) && !deflect) && !IS_AFFECTED2(victim, AFF2_SLOW) && !NewSaves(victim, SAVING_PARA, mod) )
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_SLOW;
      af.duration = 1;
      af.modifier = 2;
      af.bitvector2 = AFF2_SLOW;

      affect_to_char(victim, &af);

      act("&+m$n begins to sllooowwww down.", TRUE, victim, 0, 0, TO_ROOM);
      send_to_char("&+mYou feel yourself slowing down.\n", victim);
    }
  }
  else if( IS_AFFECTED5(victim, AFF5_WET) || IS_WATER_ROOM(victim->in_room) || IS_OCEAN_ROOM(victim->in_room) )
  {
    spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_NOSHRUG, &messages);
    // Chance to stun.
    if( IS_ALIVE(victim) && !deflect && !IS_AFFECTED2(victim, AFF2_STUNNED) )
    {
      Stun(victim, ch, PULSE_VIOLENCE * 2, TRUE);
    } // Victims with deflect will also deflect stun affect.
    else if( deflect && IS_ALIVE(ch) && !IS_AFFECTED2(ch, AFF2_STUNNED) )
    {
      Stun(ch, ch, PULSE_VIOLENCE * 2, TRUE);
    }
  }
  else if( IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD) )
  {
    spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_NOSHRUG, &messages);
    return;
  }
  else
  {
    spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_NOSHRUG, &messages);
    // Chance to damage eq.
    if( !deflect && IS_ALIVE(victim) )
    {
      if( number(0, TOTALLVLS) < GET_LEVEL(ch) )
      {
        for(damaged = 0; (damaged < MAX_WEAR) && !((victim->equipment[damaged])
          && (victim->equipment[damaged]->type == ITEM_ARMOR)
          && (victim->equipment[damaged]->value[0] > 0) && number(0, 1)); damaged++)
        {
          ;
        }
        if( (damaged < MAX_WEAR) && IS_ALIVE(victim) )
        {
          act("&+L$p corrodes.", FALSE, victim, victim->equipment[damaged], 0, TO_CHAR);
          GET_AC(victim) -= apply_ac(victim, damaged);
          victim->equipment[damaged]->value[0] -= number(1, 7);
          GET_AC(victim) += apply_ac(victim, damaged);
          victim->equipment[damaged]->cost = 0;
        }
      }
    }
  }
}

void event_torment_spirits(P_char ch, P_char victim, P_obj obj, void *data)
{
  int dam, ts;
  struct damage_messages messages = {
    "&+WYour ancestors plague &n$N&n &+Wwith anguish and &+Lpain...&n",
    "&+LThe &+wghosts &+Lof &n$n &+Llost ancestors shred your inner being!&N",
    "&+WYour ancestors plague &n$N&n &+Wwith anguish and &+Lpain...&n",
    "$N &+rcollapses in a heap, more dead than a doornail.&n",
    "Your will to live escapes you...",
    "$N &+rcollapses in a heap, $E is as dead as a doornail!&n",
    0};

  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
      return;

  ts = *((int *) data);
   
  if((ts >= 6 && number(0, ts--)) || ts == 10)
  {
    act("&+gRelief fills your &+wbones &+gas the &+Ltormenting &+gstops.&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("&+gYour ancestors depart, stopping their torment of&n $N.&n",
      FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  else
    ts++;
  
  if(!number(0, 1) &&
     resists_spell(ch, victim))
  {
    add_event(event_torment_spirits, 2 * PULSE_VIOLENCE,
      ch, victim, NULL, 0, &ts, sizeof(ts));
    return;
  }
  
  dam = number(GET_LEVEL(ch) / 2, GET_LEVEL(ch) * 2) + 8;

  if(IS_AFFECTED3(victim, AFF3_GR_SPIRIT_WARD))
    dam = (int) (dam * 0.75);
  
  if(IS_ALIVE(victim) &&
    spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NODEFLECT |
      SPLDAM_NOSHRUG, &messages) == DAM_NONEDEAD)
  {
    add_event(event_torment_spirits, 2 * PULSE_VIOLENCE,
      ch, victim, NULL, 0, &ts, sizeof(ts));
    
    stop_memorizing(victim);
    
    if(!affected_by_spell(victim, SPELL_MALISON))
    {
      spell_malison(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, 0);
    }
  }

  return;
}

void spell_torment_spirits(int level, P_char ch, char *arg, int type,
          P_char victim, P_obj object)
{
  int ts = 0;
  
  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
      return;
    
  act("You lock your &+rstare&n upon $N&n, close your eyes, and call upon the &+Wspirits of the &+Lother realms!&N",
    TRUE, ch, 0, victim, TO_CHAR);
  act("$n looks at you, then closes $s eyes, and &+Wbegins to chant...!&N",
    FALSE, ch, 0, victim, TO_VICT);
  act("$n gives $N a stern look, then $n closes $s eyes, and &+Wbegins to chant..&N",
    FALSE, ch, 0, victim, TO_NOTVICT);
  
  engage(ch, victim);

  add_event(event_torment_spirits, PULSE_VIOLENCE, ch, victim, NULL, 0, &ts, sizeof(ts));
}

#define _SHAMAN_MAGIC_C_
#endif
