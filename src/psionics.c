/* ***************************************************************************
 *  File: psionics.c                                         Part of Sojourn *
 *  Usage: procedures to create spell affects                                *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright  1994, 1995 - Sojourn Systems Ltd.                             *
 *************************************************************************** */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "sound.h"
#include "graph.h"
#include "damage.h"
#include "map.h"
#include "disguise.h"

/* external variables */

extern P_char character_list;
extern P_char combat_list;
extern P_obj object_list;
extern P_room world;
extern P_index obj_index;
extern const char *apply_types[];
extern const char *command[];
extern const flagDef extra_bits[];
extern const char *item_types[];
extern const int exp_table[];
extern const struct stat_data stat_factor[];
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];
extern int hometown[];
extern int top_of_world;
extern struct time_info_data time_info;
extern struct weather_data weather_info;
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern struct remember_data *remember_array[];
extern int cast_as_damage_area(P_char,
                               void (*func) (int, P_char, char *, int, P_char,
                                             P_obj), int, P_char, float,
                               float);
extern bool has_skin_spell(P_char);
extern P_index mob_index;

//#define POW_DIFF(att, def) (STAT_INDEX(GET_C_POW(def)) - STAT_INDEX(GET_C_POW(att)))

void event_illithid_feeding(P_char ch, P_char victim, P_obj obj, void *data)
{
  if ((!victim) || (ch->in_room != victim->in_room))
  {
    send_to_char("Alas, your victim doesn't seem to be around anymore.\r\n",
                 ch);
    return;
  }
  send_to_char("&+MYou suck the last bit of brain from your victim.\r\n", ch);
  send_to_char("&+MYou complete your feeding, feeling refreshed.\r\n", ch);
  act("$n releases $N&n, pulling $s tentacles out of $S ear.", TRUE, ch, 0,
      victim, TO_NOTVICT);
  act("$n releases you, pulling $s tentacles out of your ear.", TRUE, ch, 0,
      victim, TO_VICT);
  if (GET_MANA(ch) < GET_MAX_MANA(ch))
    GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_COND(ch, FULL) += 36;
  GET_COND(ch, THIRST) += 36;
  die(victim, ch);
  return;
}

void do_drain(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (GET_RACE(ch) != RACE_ILLITHID && \
     GET_RACE(ch) != RACE_PILLITHID)
  {
    send_to_char("What a sicko. Go home.\r\n", ch);
    return;
  }

  if (!*arg)
  {
    send_to_char("Who?!?!\r\n", ch);
    return;
  }

  if (!(victim = get_char_room_vis(ch, arg)))
  {
    send_to_char("I don't believe they are here.\r\n", ch);
    return;
  }

  if (victim == ch)
  {
    send_to_char("Wouldn't that be exciting?\r\n", ch);
    return;
  }

  if (GET_STAT(victim) >= STAT_SLEEPING)
  {
    send_to_char("Prep your food first!\r\n", ch);
    return;
  }

  add_event(event_illithid_feeding, GET_LEVEL(victim) / 2, ch, victim, 0, 0, 0, 0);

  act
    ("&+MYou grip your victim's head in your tentacles, sending one deep into $S brain.  The feeding begins!",
     FALSE, ch, 0, victim, TO_CHAR);
  act
    ("&+M$n grips $N&n&+M's head in $s tentacles, sending one deep into $S brain.",
     TRUE, ch, 0, victim, TO_NOTVICT);
  act
    ("&+RA wave of pain grips your brain as $N begins to slowly suck it out through your ear.",
     FALSE, ch, 0, victim, TO_VICT);
}

void event_githyanki_neckbite(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      get_mana = 0;

  if ((!victim) || (ch->in_room != victim->in_room))
  {
    send_to_char("Alas, your victim doesn't seem to be around anymore.\r\n",
                 ch);
    return;
  }

  send_to_char("&+RYou suck the last bit of blood from your victim.\r\n", ch);
  act("$n&n removes $s teeth from $N&n's neck, leaving a bloodless corpse.",
      TRUE, ch, 0, victim, TO_NOTVICT);
  act("$n&n removes $s teeth from your neck -- you feel terrible.", TRUE, ch,
      0, victim, TO_VICT);
  if (GET_HIT(ch) < GET_MAX_HIT(ch))
    GET_HIT(ch) += MIN(GET_LEVEL(victim) * 4, GET_MAX_HIT(ch) - GET_HIT(ch));

  if (GET_LEVEL(victim) < MAX(GET_LEVEL(ch) / 2, GET_LEVEL(ch - 10)))
  {
    get_mana = MIN(GET_LEVEL(victim) * 2, GET_MAX_MANA(ch) - GET_MANA(ch));
  }
  else
    get_mana = GET_MAX_MANA(ch) - GET_MANA(ch);

  GET_MANA(ch) += get_mana;

  if (GET_MANA(ch) < GET_MAX_MANA(ch)) ;
  send_to_char
    ("&+RSadly, your victim had too weak life force to fully feed you.\r\n",
     ch);

  die(victim, ch);
}

void do_gith_neckbite(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (GET_RACE(ch) != RACE_PVAMPIRE)
  {
    send_to_char("What a sicko. Go home.\r\n", ch);
    return;
  }

  if (!*arg)
  {
    send_to_char("Who?!?!\r\n", ch);
    return;
  }

  if (!(victim = get_char_room_vis(ch, arg)))
  {
    send_to_char("I don't believe they are here.\r\n", ch);
    return;
  }

  if (ch == victim)
  {
    send_to_char("Bite your own neck?\r\n", ch);
    return;
  }

  if (!IS_HUMANOID(victim))
  {
    send_to_char("You sure they have a neck to bite?\r\n", ch);
    return;
  }

  if ((GET_STAT(victim) >= STAT_SLEEPING))
  {
    send_to_char("Prep your food first!\r\n", ch);
    return;
  }

  add_event(event_githyanki_neckbite, 15, ch, victim, 0, 0, 0, 0);

  send_to_char
    ("&+RYou sink your teeth deeply into their neck, relishing in the feast that is just beginning..\r\n",
     ch);
  act
    ("&+R$n&+R sinks $s teeth into $N&n&+R's neck, clearly relishing the experience.",
     TRUE, ch, 0, victim, TO_NOTVICT);
  act
    ("&+RA wave of pain runs through your entire body as $n&+R sinks $s teeth into your neck.",
     FALSE, ch, 0, victim, TO_VICT);
}

void event_illithid_feeding2(P_char ch, P_char victim, P_obj obj, void *data)
{
  if ((!victim) || (ch->in_room != victim->in_room))
  {
    send_to_char("Alas, your victim doesn't seem to be around anymore.\r\n",
                 ch);
    return;
  }

  send_to_char
    ("&+GYou suck the last bit of free energy from your victim.\r\n", ch);
  send_to_char("&+GYou complete your feeding, feeling refreshed.\r\n", ch);
  act("&+G$n releases $N&+G, pulling $s tentacles out of $S ear.", TRUE, ch,
      0, victim, TO_NOTVICT);
  act("&+G$n releases you, pulling $s tentacles out of your ear.", TRUE, ch,
      0, victim, TO_VICT);
  if (GET_HIT(ch) < GET_MAX_HIT(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);

  die(victim, ch);
}

void do_absorbe(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (GET_RACE(ch) != RACE_ILLITHID && \
      GET_RACE(ch) != RACE_PILLITHID)
  {
    send_to_char("What a sicko. Go home.\r\n", ch);
    return;
  }

  if (!*arg)
  {
    send_to_char("Who?!?!\r\n", ch);
    return;
  }

  if (!(victim = get_char_room_vis(ch, arg)))
  {
    send_to_char("I don't believe they are here.\r\n", ch);
    return;
  }

  if (victim == ch)
  {
    send_to_char
      ("You send a tentacle into your brain..  R.I.P. you are dead!  go away..\r\n",
       ch);
    return;
  }

  if ((GET_STAT(victim) >= STAT_SLEEPING))
  {
    send_to_char("Your victim must be held in place first.\r\n", ch);
    return;
  }

  add_event(event_illithid_feeding2, GET_LEVEL(victim) >> 1, ch, victim, 0, 0, 0, 0);

  act
    ("&+GYou grip your victim's head in your tentacles, sending one deep into $S brain.  The feeding begins!",
     FALSE, ch, 0, victim, TO_CHAR);
  act
    ("&+G$n grips $N&+G's head in $s tentacles, sending one deep into $S brain.",
     TRUE, ch, 0, victim, TO_NOTVICT);
  act
    ("&+RA wave of pain grips your brain, as $N begins to slowly suck it out through your ear.",
     FALSE, ch, 0, victim, TO_VICT);
}

void spell_molecular_control(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;
  int dur;

  if (!((level >= 0) && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms in molecular control");
    raise(SIGSEGV);
  }
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = (int)(level / 4);
  else
    dur = 100;
    
  if (affected_by_spell(ch, SPELL_MOLECULAR_CONTROL))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_MOLECULAR_CONTROL)
      {
        af1->duration = dur;
      }
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_MOLECULAR_CONTROL;
  af.duration = dur;
  af.bitvector2 = AFF2_PASSDOOR;
  affect_to_char(ch, &af);

  act("&+YYou gain a keener understanding of your molecules!",
    FALSE, ch, 0, 0, TO_CHAR);
  act("&+W$n vibrates for a second, then is still.",
    TRUE, ch, 0, 0, TO_ROOM);

  return;
}
void spell_molecular_agitation(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int  dam;
  struct damage_messages messages = {
    "$N's entire body quivers in deliciously searing full-body pain.",
    "&+ROUCH!&n  Your whole body momentarily seems to be on fire!",
    "$N's body suddenly quivers for a moment - that looked painful.",
    "$N doubles over in intense pain and the mindless muscle spasms end only after death claims $M.",
    "The pain you're feeling is so intense you'd rather be dead. Ah, you are...",
    "$N suddenly doubles over due to what must be incredible pain. Only death can save $M. Wait, it did..."
  };
  
  if(!(ch) ||
     !(victim))
      return;

  if(number(0, 3) &&
     resists_spell(ch, victim))
      return;
  
  dam = (dice(((level / 10) +4 ), 5) * 8) + number(20, 60);
  
  if (StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    dam = (int) (dam / 1.5);

  spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NOSHRUG | SPLDAM_ALLGLOBES, &messages);
}

// Old molecular agitation
/* 
void spell_molecular_agitation(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{

  int      dam_each[61] = {
    0, 0, 0, 0, 0, 12, 15, 18, 21, 24,
    24, 24, 25, 25, 26, 26, 26, 27, 27, 27,
    28, 28, 28, 29, 29, 29, 30, 30, 30, 31,
    31, 31, 32, 32, 32, 33, 33, 33, 34, 34,
    34, 35, 35, 35, 36, 36, 36, 37, 37, 37,
    38, 38, 38, 38, 39, 39, 39, 39, 40, 40,
    41
  };
  int      dam;
  struct damage_messages messages = {
    "$N's entire body quivers in deliciously searing full-body pain.",
    "&+ROUCH!&n  Your whole body momentarily seems to be on fire!",
    "$N's body suddenly quivers for a moment - that looked painful.",
    "$N doubles over in intense pain and the mindless muscle spasms end only after death claims $M.",
    "The pain you're feeling is so intense you'd rather be dead. Ah, you are...",
    "$N suddenly doubles over due to what must be incredible pain. Only death can save $M. Wait, it did..."
  };

  level = MIN(level, (sizeof(dam_each) / sizeof(dam_each[0] - 1)));
  level = MAX(0, level);
  dam = number((dam_each[level] / 2), (dam_each[level] * 2));
  if (StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    dam = (int) (dam / 1.5);

  spell_damage(ch, victim, dam, SPLDAM_PSI, 0, &messages);

} */
void spell_adrenaline_control(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  struct affected_type af;
  int dur;
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = level + 5;
  else
    dur = 100;

  if (affected_by_spell(victim, SPELL_ADRENALINE_CONTROL))
  {
    bool renew = false;
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_ADRENALINE_CONTROL)
      {
        af1->duration = dur;
        renew = true;
      }
    }
    if(renew)
      send_to_char("Your &+cadrenaline rush&n is &+yrenewed!\r\n", victim);
    return;
  }
  
  bzero(&af, sizeof(af));
  af.type = SPELL_ADRENALINE_CONTROL;
  af.duration = dur;
  af.location = APPLY_STR;
  af.modifier = 5;
  af.bitvector = 0;
  affect_to_char(ch, &af);
  af.location = APPLY_CON;
  affect_to_char(ch, &af);

  send_to_char("&+CYou feel a massive rush of adrenaline!\r\n", ch);
  return;
}

void spell_aura_sight(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{
  struct affected_type af;
  int duration;
  
  if (!((level >= 0) && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms in aura sight");
    raise(SIGSEGV);
  }
  
  if(GET_CLASS(ch, CLASS_PSIONICIST | CLASS_MINDFLAYER))
  {
    if(IS_PC(ch) ||
       IS_PC_PET(ch))
          duration = level;
    else
      duration = 100;
  }
  else
    duration = (int)(level / 2);
    
  if(victim != ch)
    victim = ch;
  
  if(affected_by_spell(ch, SPELL_AURA_SIGHT))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_AURA_SIGHT)
      {
        af1->duration = duration;
      }
    send_to_char("Your mental vision of various auras is refreshed.\r\n", victim);
    return;
  }
  
  bzero(&af, sizeof(af));

  af.type = SPELL_AURA_SIGHT;
  af.duration = duration;

  af.bitvector2 = AFF2_DETECT_MAGIC;
  affect_to_char(victim, &af);

  if(GET_LEVEL(ch) > 20)
  {
    af.bitvector2 = AFF2_DETECT_GOOD;
    affect_to_char(victim, &af);

    af.bitvector2 = AFF2_DETECT_EVIL;
    affect_to_char(victim, &af);
  }

  if (GET_LEVEL(ch) > 25)
  {
    af.bitvector = AFF_SENSE_LIFE;
    af.bitvector2 = 0;
    affect_to_char(victim, &af);
  }

  if (GET_LEVEL(ch) > 36)
  {
    af.bitvector = AFF_DETECT_INVISIBLE;
    affect_to_char(victim, &af);
  }

  if (GET_LEVEL(ch) > 41)
  {
    af.bitvector = AFF_FARSEE;
    affect_to_char(victim, &af);
  }

  if(GET_CLASS(ch, CLASS_PSIONICIST | CLASS_MINDFLAYER) &&
     GET_LEVEL(ch) > 51)
  {
    af.bitvector4 = AFF4_DETECT_ILLUSION;
    affect_to_char(victim, &af);
  }
  
  send_to_char("&+MYour vision sharpens considerably.&n\r\n", victim);

}

void spell_sever_link(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{

    if (!ch || !victim)
    return;
    
    if (!victim->following) {
        send_to_char("There is no link for you to sever. This creature is already free!\r\n", ch);
        return;
    }
    
    send_to_char("You attempt to sever the link between the pet and the master.\r\n", ch);
    send_to_char("You feel as if someone is probing the bond to your pet.\r\n", victim->following);
    
    if (GET_LEVEL(victim) > 50) {
      send_to_char("But it seems that this time you are powerless.\r\n", ch);
      send_to_char("But it seems they are powerless.\r\n", victim->following);
      act("$N just tried to break the link between you and your master...", FALSE, ch, 0, victim, TO_CHAR);
      if (!IS_FIGHTING(victim))
         MobStartFight(victim, ch);
      return;
    }
    
    if (number(0, 99) < BOUNDED(10, (100 - GET_LEVEL(victim)), 90)) {
      act("$N jerks slightly as you tear through the bond.", FALSE, ch, 0, victim, TO_CHAR);
      send_to_char("A stabbing pain fills your head as the bond is torn to shreds!\r\n", victim->following);
      send_to_char("A sudden wave of blackness obscures your vision...\r\n", victim);
      send_to_char("...and when it dissipates you feel you are no longer being controlled.\r\n", victim);
      stop_follower(victim);
    } else
    MobStartFight(victim, ch);
}

void spell_awe(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int num = 0;
  int max_level = 0;
  int total_levels = 0;
  int max_total_levels = 0;
  struct affected_type af;
  struct follow_type *followers;

  if (!(ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if(!victim)
  {
    send_to_char("You must have a target!\r\n", ch);
    return;
  }

  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim) ||
    IS_TRUSTED(victim))
      return;

  for (followers = ch->followers; followers; followers = followers->next)
  { 
    if(affected_by_spell(followers->follower, SPELL_CHARM_PERSON))
    {
      num++;
      total_levels += GET_LEVEL(followers->follower);
    }
    
    if(!IS_TRUSTED(ch) &&
      num >= 5)
    {
      send_to_char("You are unable to handle more followers.\r\n", ch);
      return;
    }
  }

//  debug("part1: total_levels = %d", total_levels);

  max_total_levels = (int) (GET_LEVEL(ch) * 2);

  if(IS_ILLITHID(ch) || IS_PILLITHID(ch))
  {
    if(level > 51)
      max_level = level + 5;
    else if(level > 30)
      max_level = level;
    else if(level > 20)
      max_level = level - 4;
    else if(level > 15)
      max_level = level - 8;
    else
      max_level = 5;

    max_level = BOUNDED(5, max_level, 56);
  }
  else if(IS_TRUSTED(ch))
  {
    max_total_levels = 500;
    max_level = 65;
  }
  else
  {
    max_total_levels = 110;
    max_level = BOUNDED(5, level - 10, 36);
  }

  if(circle_follow(victim, ch))
    return;
  
  appear(ch);

  if(IS_NPC(victim) && 
    !GET_MASTER(victim))
  {
//    debug("Max level: %d Max_total_levels %d Victim level %d",
//    max_level, max_total_levels, GET_LEVEL(victim));
    
    if(total_levels + GET_LEVEL(victim) >= max_total_levels ||
      GET_LEVEL(victim) > max_level)
    {
      if(!IS_FIGHTING(victim))
      {
        act("$N&n seems really &+RPISSED OFF!&n", FALSE, ch, 0, victim, TO_CHAR);
        act("You see $n gazing at you, then charges $m!&n",
          FALSE, ch, 0, victim, TO_VICT);
        act("$N looks at $n, displays an &+rupset&n look, then charges $m!&n", FALSE,
          ch, 0, victim, TO_NOTVICT);
        set_fighting(victim, ch);
      }
      
      return;
    }

    // For the most part, awe only works on player-like races.
    if(!IS_GREATER_RACE(victim) &&
      !IS_ELITE(victim) &&
      !IS_UNDEAD(victim) &&
      !IS_TRUSTED(victim) &&
      (OLD_RACE_EVIL(victim) ||
      OLD_RACE_GOOD(victim) ||
      GET_RACE(victim) == RACE_SNOW_OGRE ||
      GET_RACE(victim) == RACE_RAKSHASA ||
      GET_RACE(victim) == RACE_HUMANOID))
    {

    // Awe save is psi's power versus victim's int adjusted by level difference.
    // There is a chance for critical success or critical failure.
      
      int save = BOUNDED(5, 
        GET_C_POW(ch) + level - GET_C_INT(victim) - GET_LEVEL(victim),
        95);
      
      if(save < number(0, 100) && 
        !IS_TRUSTED(victim) &&
        !IS_FIGHTING(victim))
        {
          set_fighting(victim, ch);
          return;
        }
        
      else if(!victim->following &&
              !IS_TRUSTED(victim))
      {
        send_to_char("&+MYou dominate it!&n\n", ch);
        add_follower(victim, ch);
        setup_pet(victim, ch, level, 0);
        act("&+MYour will is &+Ydominated!&n &+RUh oh...&n.", FALSE, ch, 0, victim, TO_VICT);
          
        if(IS_FIGHTING(victim))
        {
          stop_fighting(victim);
          StopMercifulAttackers(victim);
        }
      }
    }
    else
    {
      send_to_char("&+MYou are unable to dominate the mind of this strange creature!&n\n", ch);
    }
    
    return;
  }
}

void spell_mind_travel(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  int      num_rooms = 24;      /* match the array below - 1 */
  int      r_room = -1;
  int      to_room[] = { 96563, 96563, 11545, 15273, 96900,
    36171, 96563, 4109, 4437, 4109,
    96803, 17589, 17607, 96909, 3404,
    23812, 23805, 12535, 12528, 25458,
    25459, 25458, 12535, 12536, 12540
  };

  if (IS_FIGHTING(ch))
  {
    act
      ("$n travel with the speed of &+Clight&n and reappear at the same place...",
       FALSE, ch, obj, 0, TO_CHAR);
    act
      ("You travel with the speed of &+Clight&n and reappear at the same place",
       FALSE, ch, obj, 0, TO_NOTVICT);
    return;
  }

  if (IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
      world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("&+CYou failed.\r\n", ch);
    return;
  }
  char_from_room(ch);
  while (r_room == -1)
    r_room = real_room(to_room[number(0, num_rooms)]);
  act("$n travel with the speed of &+Clight&n and reappear elsewhere...bye!",
      FALSE, ch, obj, 0, TO_CHAR);
  act("You travel with the speed of &+Clight&n and reappear elsewhere...",
      FALSE, ch, obj, 0, TO_CHAR);
  char_to_room(ch, r_room, -1);
  CharWait(ch, 5);
  return;
}

void spell_ballistic_attack(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      dam, in_room, mod;
  
  if(!(ch) ||
     !(victim))
      return;
  
  if(IS_BLIND(ch))
  {
    send_to_char("You must be able to see!\r\n", ch);
    return;
  }
  
  if(IS_WATER_ROOM(ch->in_room) ||
     IS_OCEAN_ROOM(ch->in_room))
  {
    send_to_char("You seen a bunch of water, but no rocks!\r\n", ch);
    return;
  }
  
  dam = (dice(((level / 10 ) +4), 8) * 3);
 
  mod = MAX(1, level - GET_LEVEL(victim));
  
  act("With your telekinetic powers, you propel a large rock at $N&n.",
    TRUE, ch, 0, victim, TO_CHAR);
  act("$n looks over a large rock, then seems to send it crashing into you!",
    TRUE, ch, 0, victim, TO_VICT);
  act("$n looks over a large rock, then seems to send it crashing into $N!",
    TRUE, ch, 0, victim, TO_NOTVICT);

  if(!StatSave(victim, APPLY_AGI, mod))
  {
    act("The rock smashes into $N with a huge thud.",
      TRUE, ch, 0, victim, TO_CHAR);
    act("You are smashed by the large rock!",
      TRUE, ch, 0, victim, TO_VICT);
    act("$N is smashed by the large rock!",
      TRUE, ch, 0, victim, TO_NOTVICT);
    dam *= 2;
  }

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG, 0);
}

// Old ballistic attack
/* void spell_ballistic_attack(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  const int dam_each[61] = { 0,
    3, 4, 4, 5, 6, 6, 6, 7, 7, 7,
    7, 7, 8, 8, 8, 9, 9, 9, 10, 10,
    10, 11, 11, 11, 12, 12, 12, 13, 13, 13,
    14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
    17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
    21, 21, 21, 22, 22, 22, 23, 23, 23, 24
  };
  int      dam;

  level = MIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
  level = MAX(0, level);
  dam = number(dam_each[level] / 2, dam_each[level] * 4);

  act("&+LYou chuckle as a stone strikes $N&+L.", TRUE, ch, 0, victim,
      TO_CHAR);
  act("&+L$n&+L creates a boulder, striking you solidly!", TRUE, ch, 0,
      victim, TO_VICT);
  act("&+L$n&+L squints, and a boulder appears, striking $N&+L solidly!",
      TRUE, ch, 0, victim, TO_NOTVICT);

  if (!StatSave(victim, APPLY_DEX, 0))
  {
    act("&+LYour mental boulder smashes $N&+L really hard.", TRUE, ch, 0,
        victim, TO_CHAR);
    act("&+LYou are almost smashed by the force of the boulder.!", TRUE, ch,
        0, victim, TO_VICT);
    act("&+L$N&+L is almost smashed by the heavy projectile!", TRUE, ch, 0,
        victim, TO_NOTVICT);
    dam *= 2;
  }

  damage(ch, victim, dam, SPELL_BALLISTIC_ATTACK);
} */

void spell_biofeedback(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  struct affected_type af;

  if (has_skin_spell(victim) && IS_PC(victim))
  {
    send_to_char("Nothing happens.\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_BIOFEEDBACK;
  af.duration = 6;
  af.modifier = (level / 2 + number(1, 4));
  affect_to_char(victim, &af);

  send_to_char("&+GYou are surrounded by a green mist!&n\r\n", ch);
  act("&+G$n&+G is suddenly surrounded by a green mist!", TRUE, ch, 0, 0,
      TO_NOTVICT);
}


void spell_cell_adjustment(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if(!(ch) ||
     !IS_ALIVE(ch))
      return;
      
  if(victim != ch)
    victim = ch;

   if(level > 2 &&
     (affected_by_spell(victim, SPELL_POISON) ||
     IS_SET(victim->specials.affected_by2, AFF2_POISONED)))
  {
    act("&+GYour willpower has neutralized the poison in your bloodstream.&n",
        FALSE, ch, 0, victim, TO_CHAR);
    poison_common_remove(victim);
  }

  if (level > 9 &&
     affected_by_spell(victim, SPELL_CURSE))
  {
    act("&+YYou are able to detach the curse away from your inner being.&n",
      FALSE, ch, 0, victim, TO_CHAR);
    affect_from_char(victim, SPELL_CURSE);
  }
  
  if(level > 29 &&
    IS_AFFECTED(victim, AFF_BLIND))
  {
    act("&+WYour vision returns.&n",
      FALSE, ch, 0, victim, TO_CHAR);
    affect_from_char(victim, SPELL_BLINDNESS);
    
    if(IS_SET(victim->specials.affected_by, AFF_BLIND))
      REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
  }
  
  if(level > 35 &&
    affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT))
  {
    act("&+rYou will the removal of enfeeblement.&n",
        FALSE, ch, 0, victim, TO_CHAR);
    affect_from_char(victim, SPELL_RAY_OF_ENFEEBLEMENT);
  }

  if(level > 36 &&
     affected_by_spell(victim, SPELL_MALISON))
  {
    act("&+YYou are able to detach the curse away from your inner being.&n",
      FALSE, ch, 0, victim, TO_CHAR);
    affect_from_char(victim, SPELL_MALISON);
  }
  
  if(level > 41 &&
     affected_by_spell(victim, SPELL_SLOW))
  {
    act("&+mYou neutralize the slowness affecting you!&n",
        FALSE, ch, 0, victim, TO_CHAR);
    affect_from_char(victim, SPELL_SLOW);
  }
  
  if(level > 50 &&
    affected_by_spell(victim, SPELL_WITHER))
  {
    act("&+rYour sheer willpower causes your body to overcome the withering.&n",
        FALSE, ch, 0, victim, TO_CHAR);
    affect_from_char(victim, SPELL_WITHER);
  }

  if(level > 53 &&
    affected_by_spell(victim, SPELL_DISEASE))
  {
    act("&+rWith nothing but sheer willpower, you shrug off the putrid disease!&n",
        FALSE, ch, 0, victim, TO_CHAR);
    affect_from_char(victim, SPELL_DISEASE);
  }
  return;
}

void spell_combat_mind(int level, P_char ch, char *arg, int type, P_char victim,  P_obj obj)
{
  struct affected_type af;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
      return;
      
  if(victim != ch)
    victim = ch;

  if (affected_by_spell(ch, SPELL_COMBAT_MIND))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_COMBAT_MIND)
      {
        if(!GET_CLASS(victim, CLASS_PSIONICIST | CLASS_MINDFLAYER))
          af1->duration = (int)(level / 2);
        else
          af1->duration = level;
      }
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_COMBAT_MIND;
  af.flags = AFFTYPE_NODISPEL;
  
  if(!GET_CLASS(victim, CLASS_PSIONICIST | CLASS_MINDFLAYER))
  {
    af.duration = (int)(level / 2);
    af.modifier = (int)(level / 6);
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);

    af.duration = (int)(level / 2);
    af.modifier = (int)(level / 8);
    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);
  }
  else
  {
    af.duration = level;
    af.modifier = (int)(10 + level / 4);
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);

    af.duration = level;
    af.modifier = (int)(8 + level / 5);
    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);
  }

  act("&+yYour knowledge of &+Rbattle tactics&n increases!",
    FALSE, ch, 0, 0, TO_CHAR);
  return;
}

void spell_fire_aura(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct affected_type af;

  if (!ch)
    return;

  if (affected_by_spell(ch, SPELL_FIRE_AURA))
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }

  if (world[ch->in_room].sector_type == SECT_FIREPLANE) {
    act("&+rYour body glows with an aura of fire!", FALSE, ch, 0, 0, TO_CHAR);
    act("&+r$n's&+r body glows with an aura of fire!", FALSE, ch, 0, 0,
        TO_ROOM);
    bzero(&af, sizeof(af));
    af.type = SPELL_FIRE_AURA;
    af.duration = (SECS_PER_MUD_DAY / 60);

    af.location = APPLY_AC;
    affect_to_char(victim, &af);

    af.location = APPLY_STR_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX_MAX;
    af.modifier = -20;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI_MAX;
    af.modifier = -20;
    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = 25;
    affect_to_char(victim, &af);

    if (NewSaves(ch, SAVING_FEAR, 4))
    {
      act("&+RYour body bursts into flames as you achieve full form!", FALSE,
          ch, 0, 0, TO_CHAR);
      act("&+R$n's&+R body bursts into flames as $e achieves full form!",
          FALSE, ch, 0, 0, TO_ROOM);
      af.location = 0;
      af.modifier = 0;
      af.bitvector2 = AFF2_FIRE_AURA;
      affect_to_char(victim, &af);

      af.bitvector2 = AFF2_FIRESHIELD;
      affect_to_char(victim, &af);
    }
  } else 
  {
    act("&+LYou need to stand on &+Rfire&+L in order to gain some of its properties.", FALSE, ch, 0, 0, TO_CHAR);
  }
}

void spell_control_flames(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{
  char buf[MAX_STRING_LENGTH];
  int  dam;

  const int dam_each[61] = { 0,
    0, 0, 0, 0, 0, 0, 0, 16, 20, 24,
    28, 32, 35, 38, 40, 42, 44, 45, 45, 45,
    46, 46, 46, 47, 47, 47, 48, 48, 48, 49,
    49, 49, 50, 50, 50, 51, 51, 51, 52, 52,
    52, 53, 53, 53, 54, 54, 54, 55, 55, 55,
    56, 56, 56, 57, 57, 58, 58, 58, 59, 59
  };

  sprintf (buf, "torch");
  obj = get_obj_in_list_vis (ch, buf, victim->carrying);
  if (!obj)
    {
      act ("Your target must be carrying a torch.", 0, ch, 0, 0, TO_CHAR);
      return;
    }

  level = MIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
  level = MAX(0, level);
  dam = number(dam_each[level] / 2, dam_each[level] * 2);

  if (StatSave(victim, APPLY_POW, 0))
    dam /= 2;

      act ("&+r$N's $p suddenly flares up, engulfing $M in flames!", TRUE, ch, obj, victim, TO_CHAR);
      act ("&+rYour $p suddenly flares up, engulfing you in flames!", TRUE, ch, obj, victim, TO_VICT);
      act ("&+r$N's $p suddenly flares up, engulfing $M in flames!", TRUE, ch, obj, victim,
TO_NOTVICT);

  damage(ch, victim, dam, SPELL_EGO_BLAST);     // no messages in file, so this works

  return;
}

/* used to be control_flames */
void spell_ego_blast(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{
  int  dam;
  
  if(!(ch) ||
     !(victim))
      return;
  
  if(IS_BRAINLESS(victim))
  {
    act("$N lacks any mental capacity whatsoever!&n",
      FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  
  if(number(0, 3) &&
     resists_spell(ch, victim))
      return;
  
  dam = (dice(((level / 10) +4 ), 5) * 8) + number(20, 60);
  
  if(!StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    dam = (int) (dam * 1.25);

  act("&+MYou mentally blast $N's ego.",
    TRUE, ch, 0, victim, TO_CHAR);
  act("&+M$n mentally blasts your mind with stunning force!",
    TRUE, ch, 0, victim, TO_VICT);
  act("&+M$n squints and $N&+M screams out in great pain, holding $S head!",
      TRUE, ch, 0, victim, TO_NOTVICT);

  spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NOSHRUG | SPLDAM_ALLGLOBES, 0);
  return;
}

// Old ego whip
/* void spell_ego_blast(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{
  int      dam;
  const int dam_each[61] = { 0,
    0, 0, 0, 0, 0, 0, 0, 16, 20, 24,
    28, 32, 35, 38, 40, 42, 44, 45, 45, 45,
    46, 46, 46, 47, 47, 47, 48, 48, 48, 49,
    49, 49, 50, 50, 50, 51, 51, 51, 52, 52,
    52, 53, 53, 53, 54, 54, 54, 55, 55, 55,
    56, 56, 56, 57, 57, 58, 58, 58, 59, 59
  };

  // sprintf (buf, "torch");
  // obj = get_obj_in_list_vis (ch, buf, victim->carrying);
  // if (!obj)
    // {
      // act ("Your target must be carrying a torch.", 0, ch, 0, 0, TO_CHAR);
      // return;
    // }

  level = MIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
  level = MAX(0, level);
  dam = number(dam_each[level] / 2, dam_each[level] * 2);

  if (StatSave(victim, APPLY_POW, 0))
    dam /= 2;

      // act ("&+r$N's $p suddenly flares up, engulfing $M in flames!", TRUE, ch, obj, victim, TO_CHAR);
      // act ("&+rYour $p suddenly flares up, engulfing you in flames!", TRUE, ch, obj, victim, TO_VICT);
      // act ("&+r$N's $p suddenly flares up, engulfing $M in flames!", TRUE, ch, obj, victim, TO_NOTVICT);

  act("&+MYou mentally blast $N's ego.", TRUE, ch, 0, victim, TO_CHAR);
  act("&+M$n mentally blasts your mind with stunning force!", TRUE, ch, 0,
      victim, TO_VICT);
  act("&+M$n squints and $N&+M screams out in great pain, holding $S head!",
      TRUE, ch, 0, victim, TO_NOTVICT);

  damage(ch, victim, dam, SPELL_EGO_BLAST);     // no messages in file, so this works

  return;
} */

void
spell_create_sound(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  if (IS_SET(world[ch->in_room].room_flags, ROOM_SILENT))
  {
    act("&+WYour sonic boom of mental power dispels the silence!", 0, ch, 0,
        0, TO_CHAR);
    act("&+W$n sends out a blast of psionic sound!", FALSE, ch, 0, 0,
        TO_ROOM);
    REMOVE_BIT(world[ch->in_room].room_flags, ROOM_SILENT);
  }
  else
    send_to_char("Hmm, seems pretty noisy already.\r\n", ch);
}

void spell_psychic_crush(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      dam, spec_affect;
  char     buf[256];
  struct affected_type af;
  struct damage_messages messages = {
    "$N's &+meyes briefly cloud over as $S entire psyche is wracked with wonderfully intense pain.",
    "&+mAs your body is consumed with pain, pain, and more pain, you briefly go to that 'happy place' inside.",
    "$N's &+meyes briefly cloud over as $S body is painfully ripped apart from within.",
    "$N &+mcrumples to the ground, blood seeping from $S ears as your psychic crush tears $S mind to pieces.",
    "&+mYou crumple to the ground, blood seeping from your ears as your mind is torn to pieces.",
    "&+RBlood &+mseeps from&n $N's &+mears as $S mind is torn to &+wpieces.", 0
  };
  
  if(!(ch) || 
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
      return;
      
  if(IS_BRAINLESS(victim))
  {
    send_to_char("Your victim lacks a brain... you are attempting to crush nothing.\r\n", ch);
    return;
  }
  
// 40% harder to shrug.
  if(number(1, 100) < 40 &&
    resists_spell(ch, victim))
      return;
      
  dam = 11 * level + number(1, 75); 

// Pow difference stat save and it is harder (25% only) to do extra damage.
// Crushing hand does *2 damage on failed spell saves.
  // if(!StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)) &&
     // !number(0, 4))
        // dam = (int)(dam * 1.25);

  if(spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages) != DAM_NONEDEAD)
    return;
  
  if(!affected_by_spell(victim, SPELL_PSYCHIC_CRUSH))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PSYCHIC_CRUSH;
    af.duration =  1;
    af.modifier = (int) (-1 * (level / 3));

    act("$n &+msuddenly clutches $s head, as if trying to shrug off some unknown malady.&n",
        FALSE, victim, 0, 0, TO_ROOM);

    switch (number(0, 2))
    {
      case 0:
        send_to_char("&+wThat last attack was particularly invasive, and you feel your collective power decrease.&n\r\n", victim);
        af.location = APPLY_POW;
        break;
        
      case 1:
        send_to_char("&+wThat last attack was particularly invasive, and you feel your collective intelligence decrease.&n\r\n", victim);
        af.location = APPLY_INT;
        break;
        
      case 2:
        send_to_char("&+wThat last attack was particularly invasive, and you feel your collective wisdom decrease.&n\r\n", victim);
        af.location = APPLY_WIS;
        break;
      default:
        break;
    }
    
    affect_to_char(victim, &af);
  }
  
  //special affect on crush for +specced casters.
  if(((GET_SPEC(ch, CLASS_PSIONICIST, SPEC_ENSLAVER) ||
     GET_CLASS(ch, CLASS_MINDFLAYER))  &&
     GET_LEVEL(ch) > 46)  &&
     (victim) &&
     !IS_ELITE(victim) &&
     !IS_TRUSTED(victim))
  {
    spec_affect = number(1, 100);
    
    if(spec_affect < 10)
    {
      if(!IS_STUNNED(victim))
        Stun(victim, PULSE_VIOLENCE * 1);
      
      if(IS_HUMANOID(victim))
      {
        strcpy(buf, "&n&+wGeT tHiS &+MThiNg&n &+wOuT oF mY hEaD !!!&n");
        do_say(victim, buf, CMD_SHOUT);
      }
    }
    else if(spec_affect < 15)
    {
      SET_POS(victim, POS_KNEELING + GET_STAT(victim));
      act("&+yYou fall to your knees as uncontrollable pain engulfs your entire brain!&n",
         FALSE, ch, NULL, victim, TO_VICT);
      act("$N &+yfalls to their knees, grasping $s head and screaming in pain!&n",
         FALSE, ch, NULL, victim, TO_NOTVICT);
      act("$N &+yfalls to their knees, grasping $s head and screaming in pain!!&n",
         TRUE, ch, NULL, victim, TO_CHAR);
      do_action(victim, 0, CMD_CRY);
    }
    else if(IS_HUMANOID(victim) &&
           (spec_affect < 40))
    {
      do_action(victim, 0, CMD_PUKE);
    }
  }
}

void spell_single_death_field(int level, P_char ch, char *args, int type,
                              P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "", "", "",
    "$N's body shivers and falls &+wlifeless&n as a &+Lwave of death&n reaches $M.",
    "Your mind collapses as a &+Lwave of death&n runs through your body.&n",
    "$N's body shivers and falls &+wlifeless&n as a &+Lwave of death&n reaches $M.", 0
  };
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
      return;
      
  if(IS_BRAINLESS(victim))
    return;

  dam = 100 + level * 6 + number(1, 40);
    
  if(!StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    dam = (int)(dam * 1.15);
    
  if(IS_PC_PET(ch))
    dam = (int)(dam * 0.65);
      
  spell_damage(ch, victim, dam, SPLDAM_PSI, 0, &messages);
}


void spell_death_field(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  P_char   tch, tmp_victim;

/*   if (IS_PC(ch) &&
      (tch =
       stack_area(ch, SPELL_DEATH_FIELD,
                  (int) get_property("spell.area.stackTimer.deathField", 5))))
  {
    act("&+LYour deadly wave interferes with $N's field!", FALSE, ch, 0, tch,
        TO_CHAR);
    act("&+L$n sends out a wave of psionic death!", FALSE, ch, 0, 0, TO_ROOM);
    level /= 2;
  }
  else */
  {
    act("&+LYou send out a wave of psionic death!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n &+Lsends out a wave of psionic death!", FALSE, ch, 0, 0, TO_ROOM);
  }
  
  LOOP_THRU_PEOPLE(tmp_victim, ch)
    if((ch != tmp_victim) && 
       IS_DISGUISE(tmp_victim) &&
       ch->group != tmp_victim->group)
        remove_disguise(tmp_victim, TRUE);

  cast_as_damage_area(ch, spell_single_death_field, level, victim,
                      get_property("spell.area.minChance.deathField", 60),
                      get_property("spell.area.chanceStep.deathField", 30));

  zone_spellmessage(ch->in_room,
                    "&+LYour brain hurts as a black haze fills your psyche!\r\n",
                    "&+LYour brain hurts as a black haze coming from the %s fills your psyche!\r\n");
  if((ch) &&
     IS_ALIVE(ch))
        CharWait(ch, (int) (PULSE_SPELLCAST * 1.5));
}

void spell_detonate(int level, P_char ch, char *arg, int type, P_char victim,
               P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "You feel a warm sense of satisfaction as &+Rexplosions&N engulf $N.",
    "Tiny &+Yexplosions&N wrack your body with pain!",
    "Tiny &+Yexplosions&n wrack $N&n's body with pain!",
    "You &+Yexplode&n $N, satisfied yet? Good...",
    "Thousands of tiny &+Yexplosions&n blast your soul away from your body.",
    "$N &+Ydetonates&n, pieces of flesh splatter across the room."
  };
  
  if(!(ch) ||
     !(victim))
        return;
      
  dam = 9 * level + number(1, 75);
  
  if(victim == ch)
    dam = 1;
    
  // if(!StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    // dam = (int)(dam * 1.15);
    
  if(!NewSaves(victim, SAVING_SPELL, 0))
     dam = (int)(dam * 1.15);
  
  if(GET_SPEC(ch, CLASS_PSIONICIST, SPEC_PYROKINETIC))
  {
    dam = (int)(dam * get_property("spell.detonate.shrugModifier", 0.750));
    
    dam /= 2;
    
    debug("DETONATE PYRO: (%s) doing (%d) damage.", GET_NAME(ch), dam);
    
    spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
    
    if(IS_ALIVE(victim))
      spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
  }
  else
    spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NODEFLECT, &messages);
}

void spell_spinal_corruption(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
      return;

  if(victim &&
    !IS_ALIVE(victim))
  {
    act("&+MBut it's dead already!", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  
  if(CheckMindflayerPresence(ch))
  {
    send_to_char
      ("You sense an ethereal disturbance and abort your spell!\r\n", ch);
    return;
  }
  
  if (StatSave
      (victim, APPLY_POW,
       MIN(1, GET_LEVEL(ch) - GET_LEVEL(victim)) * POW_DIFF(ch, victim)))
  {
    send_to_char("&+RYou feel a twinge of pain along your spine.&n\r\n", victim);
    return;
  }
  appear(ch);

  act("&+RYou smile as you sense the agony $N&+R is feeling.",
    TRUE, ch, 0, victim, TO_CHAR);
  act("&+RYou collapse in agony as pure fire envelops your spine!&n",
    TRUE, ch, 0, victim, TO_VICT);
  act("&+R$N&+R suddenly collapses, screaming in agony!",
    TRUE, ch, 0, victim, TO_NOTVICT);

  if(!StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
  {
    switch (number(1, 5))
    {
      case 1:
      {
        act("$n &+Mfalls to the ground, trashing wildly!&n",
              FALSE, victim, 0, 0, TO_ROOM);
        send_to_char
            ("&+LYou feel your body stop responding to your commands, and you fall to the ground, thrashing.&n\r\n",
             victim);
        CharWait(victim, (int) (0.75 * PULSE_VIOLENCE));
        break;
      }
      case 2:
        if (GET_STAT(ch) > STAT_SLEEPING && !IS_TRUSTED(ch))
        {
          act("&+L$n suddenly loses control, and falls asleep.",
              FALSE, victim, 0, 0, TO_ROOM);
          send_to_char
            ("&+LYour are overwhelmed with a great desire to get some rest. Yawn.\r\n",
             victim);
          SET_POS(victim, GET_POS(victim) + STAT_SLEEPING);
          update_pos(victim);
        }
        break;
      case 3:
        spell_silence(60, ch, NULL, 0, victim, 0);
        break;
      case 4:
        if(!affected_by_spell(victim, SPELL_SPINAL_CORRUPTION))
        {
          bzero(&af, sizeof(af));
          af.type = SPELL_SPINAL_CORRUPTION;
          af.duration =  1;
          af.bitvector2 = AFF2_SLOW;
          affect_to_char(victim, &af);
          act("&+LThe power of your psychic assault renders&n $N &+Lunable to react as quickly!&n",
            TRUE, ch, 0, victim, TO_CHAR);
          send_to_char("&+LIt seems as though you aren't as spry as you once were...\r\n", victim);
        }
        break;
      case 5:
      {
        poison_lifeleak(level, ch, 0, 0, victim, 0);
        act("$n&n&+G shivers slightly.", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char("&+yYou feel very sick.\n", victim);
        break;
      }
      default:
        break;
    }
  }
  SET_POS(victim, POS_PRONE + GET_STAT(victim));
  update_pos(victim);
  return;
}

void spell_mental_anguish(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
      return;
  
  if(IS_BRAINLESS(victim))
  {
    send_to_char("Your victim is brainless, so anguishing is out of the question.\r\n", ch);
    return;
  }
  
  appear(ch);
  
  if (CheckMindflayerPresence(ch))
  {
    send_to_char
      ("You sense an ethereal disturbance and abort your spell!\r\n", ch);
    return;
  }
  if (affected_by_spell(victim, SPELL_MENTAL_ANGUISH))
  {
    act("You cannot cause anymore anguish to $m!",
      TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if (StatSave(victim, APPLY_POW, MIN(0, POW_DIFF(ch, victim))))
  {
    send_to_char("&+RYou feel a strange sensation inside your brain.&n\r\n", victim);
    send_to_char("&+RYou try, but fail to induce the anguish in your victim.\r\n", ch);
    return;
  }
  else
  {
    act("$N's eyes roll back as $S brain is filled with raw pain.",
      TRUE, ch, 0, victim, TO_CHAR);
    act("&+RYour brain is filled with pain, making it hard to concentrate!&n",
      TRUE, ch, 0, victim, TO_VICT);

    bzero(&af, sizeof(af));
    af.type = SPELL_MENTAL_ANGUISH;
    af.duration = MAX(1, (int)(level / 20));
    af.bitvector5 = AFF5_MENTAL_ANGUISH;
    affect_to_char(victim, &af);
    return;

  }
}

void spell_memory_block(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
      return;
     
  if(IS_BRAINLESS(victim))
  {
    send_to_char("Your victim is brainless and without any sort of memory.\r\n", ch);
    return;
  }
  
  if(CheckMindflayerPresence(ch))
  {
    send_to_char
      ("You sense an ethereal disturbance and abort your spell!\r\n", ch);
    return;
  }

  if(StatSave(victim, APPLY_POW, MIN(0, POW_DIFF(ch, victim))))
  {
    //if(StatSave (victim, APPLY_POW, MIN(1, GET_LEVEL(ch) - GET_LEVEL(victim))*POW_DIFF(ch, victim))) {
    send_to_char("&+RYou briefly forget why you're here.&n\r\n", victim);
    return;
  }
  
  appear(ch);

  act("$N stares around &+Wblankly.&n",
    TRUE, ch, 0, victim, TO_CHAR);
  act("You seem to have &+yforgotten&n why you are here!&n",
    TRUE, ch, 0, victim, TO_VICT);
  act("$N stares around &+Wblankly.&n",
    TRUE, ch, 0, victim, TO_NOTVICT);

  SET_BIT(victim->specials.affected_by5, AFF5_MEMORY_BLOCK);
  return;
}

void spell_psionic_cloud(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;
  struct affected_type af1;
  P_char   tch, next;
  int save;

  if (CheckMindflayerPresence(ch))
  {
    send_to_char
      ("You sense an ethereal disturbance and abort your spell!\r\n", ch);
    return;
  }

  act("&+LYou summon a dark haze of psionic energy with the power of your mind!",
      FALSE, ch, 0, 0, TO_CHAR);
  act("&+L$n &+Lsquints and a dark haze of psionic energy appears!", FALSE,
      ch, 0, 0, TO_ROOM);


  bzero(&af, sizeof(af));
  af.type = SPELL_PSIONIC_CLOUD;
  af.duration = 2;
  af.modifier = -1 * number(level / 4, 30);
  af.bitvector = 0;
  af.location = APPLY_POW;

  bzero(&af1, sizeof(af1));
  af1.type = SPELL_PSIONIC_CLOUD;
  af1.duration = 2;
  af1.modifier = -1 * number(level / 4, 25);
  af1.bitvector = 0;
  af1.location = APPLY_MOVE;

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if (should_area_hit(ch, tch))
    {
      if (affected_by_spell(tch, SPELL_PSIONIC_CLOUD))
	  {
		 act("$N &+Lis already affected by your psionic haze!&n",
         TRUE, ch, 0, tch, TO_CHAR);
        continue;
	  }

      save = BOUNDED(5, GET_C_POW(ch) + level - GET_C_INT(tch) - GET_LEVEL(tch), 95);

      if(save > number(0, 100))
	  {
      act("$N &+Lis engulfed by the psionic haze!&n",
          TRUE, ch, 0, tch, TO_CHAR);
      act("&+LYou feel much less virile as you are engulfed by &n$n&+L's psionic cloud...&n",
          TRUE, ch, 0, tch, TO_VICT);
      act("$N &+Lis engulfed by the psionic haze!&n",
          TRUE, ch, 0, tch, TO_NOTVICT);
      affect_to_char(tch, &af);
      affect_to_char(tch, &af1);
	  }
    }
  }

  zone_spellmessage(ch->in_room,
                    "&+LYou feel weakened as your psyche senses a massive energy influx!\r\n",
                    "&+LYou feel weakened as your psyche senses a massive energy influx coming from the %s!\r\n");

  return;
}

void spell_displacement(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  struct affected_type af;

  if (affected_by_spell(ch, SPELL_DISPLACEMENT))
  {
    send_to_char("You're already affected by displacement!\r\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_DISPLACEMENT;
  af.duration = MAX(2, level - 4);
  af.modifier = (-2 * level);
  af.bitvector = 0;
  af.location = APPLY_AC;
  affect_to_char(victim, &af);

  act("&+WYour form shimmers, and you appear displaced.",
    FALSE, ch, 0, victim, TO_CHAR);
  act("&+W$N shimmers and appears a few inches away.",
    TRUE, ch, 0, victim, TO_ROOM);

  return;
}

void spell_domination(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{
  if (victim == ch)
  {
    send_to_char("Dominate yourself? You're weird.\r\n", ch);
    return;
  }
  
  if(IS_BRAINLESS(victim))
  {
    send_to_char("Dominating that thing is impossible since it is brainless!\r\n", ch);
    return;
  }
  
  if (GET_MASTER(victim) || GET_MASTER(ch)
      || level < GET_LEVEL(victim) ||
      StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    return;
/*
   if (victim->leader)
   stop_follower(victim);
 */
  setup_pet(victim, ch, MAX(1, number(level - 20, level)), 0);
  add_follower(victim, ch);
  act("Your will dominates $N!", FALSE, ch, 0, victim, TO_CHAR);
  act("Your will is dominated by $n!", FALSE, ch, 0, victim, TO_VICT);

  return;
}

void spell_ectoplasmic_form(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if (!((victim || obj) && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms in ectoplasmic form");
    raise(SIGSEGV);
  }
 
// The spell is self only. No need to check object.
  // if (obj)
  // {
    // if (!IS_SET(obj->extra_flags, ITEM_INVISIBLE) &&
        // IS_SET(obj->wear_flags, ITEM_TAKE))
    // {
      // act("&+L$p turns invisible.", FALSE, ch, obj, 0, TO_CHAR);
      // act("&+L$p turns invisible.", TRUE, ch, obj, 0, TO_ROOM);
      // SET_BIT(obj->extra_flags, ITEM_INVISIBLE);
      // REMOVE_BIT(obj->extra_flags, ITEM_LIT);
    // }
  // }
  // else
  {                          
    if (!affected_by_spell(victim, SPELL_ECTOPLASMIC_FORM))
    {
      act("$n's &+Lform shimmers and then fades from sight.&N",
        TRUE, victim, 0, 0, TO_ROOM);
      send_to_char("&+LYou shimmer, then fade from sight.\r\n", victim);

      bzero(&af, sizeof(af));

      af.type = SPELL_ECTOPLASMIC_FORM;
      af.duration = level / 2;
      af.bitvector3 = AFF3_ECTOPLASMIC_FORM;
      affect_to_char(victim, &af);
    }
    else
    {
      struct affected_type *af1;

      for (af1 = victim->affected; af1; af1 = af1->next)
        if (af1->type == SPELL_ECTOPLASMIC_FORM)
        {
          af1->duration = level / 2;
        }
    }
  }
}

// Old Ego Whip
/* void spell_ego_whip(int level, P_char ch, char *arg, int type, P_char victim,
               P_obj obj)
{
  const int dam_each[61] = { 0,
    3, 4, 4, 5, 6, 6, 6, 7, 7, 7,
    7, 7, 8, 8, 8, 9, 9, 9, 10, 10,
    10, 11, 11, 11, 12, 12, 12, 13, 13, 13,
    14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
    17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
    21, 21, 21, 22, 22, 22, 23, 23, 23, 24
  };
  int      dam;

  level = MIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
  level = MAX(0, level);
  dam = number(dam_each[level] >> 1, dam_each[level] << 1);
  //dam = dice(4,6) * 2;

  if (StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    dam = (int) (dam / 1.5);
  if (IS_AFFECTED3(victim, AFF3_TOWER_IRON_WILL))
    dam /= 2;

  act("&+MYou mentally beat upon $N's ego.",
    TRUE, ch, 0, victim, TO_CHAR);
  act("&+M$n mentally beats upon your mind with stunning force!",
    TRUE, ch, 0, victim, TO_VICT);
  act("&+M$n squints and $N&+M screams out in pain, holding $S head!",
    TRUE, ch, 0, victim, TO_NOTVICT);

  damage(ch, victim, dam, SPELL_EGO_WHIP);
} */

void spell_ego_whip(int level, P_char ch, char *arg, int type, P_char victim,
               P_obj obj)
{
  int  dam;
  
  if(!(ch) ||
     !(victim))
      return;
  
  if(IS_BRAINLESS(victim))
  {
    act("$N lacks any mental capacity whatsoever!&n",
      FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  
  if(number(0, 3) &&
     resists_spell(ch, victim))
      return;
  
  dam = (dice(((level / 10) + 4), 5) * 5) + number(4, 20);
  
  if (StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    dam = (int) (dam / 1.5);
  
  act("&+MYou mentally beat upon $N's ego.",
    TRUE, ch, 0, victim, TO_CHAR);
  act("&+M$n mentally beats upon your mind with stunning force!",
    TRUE, ch, 0, victim, TO_VICT);
  act("&+M$n squints and $N&+M screams out in pain, holding $S head!",
    TRUE, ch, 0, victim, TO_NOTVICT);

  spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NOSHRUG | SPLDAM_ALLGLOBES, 0);
}

void spell_energy_containment(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;

  int dur;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;

  if(IS_PC(ch) ||
     IS_PC_PET(ch))
      dur = (int)(level / 2 + 7);
  else
    dur = 100;
    
  if(affected_by_spell(ch, SPELL_ENERGY_CONTAINMENT))
  {
    struct affected_type *af1;
    bool renew = false;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_ENERGY_CONTAINMENT)
      {
        af1->duration = dur;
        renew = true;
      }
    
    if(renew)
      send_to_char("&+yYou harden your will against elemental damage.\r\n", victim);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_ENERGY_CONTAINMENT;
  af.duration = dur;
  affect_to_char(victim, &af);
  
  send_to_char("&+yYou harden your will against elemental damage.\r\n", victim);

  return;
}

void spell_enhance_armor(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;
  int dur;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = (int)(level / 2 + 7);
  else
    dur = 100;
  
  if (!affected_by_spell(victim, SPELL_ENHANCE_ARMOR))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_ENHANCE_ARMOR;
    af.duration = dur;
    af.location = APPLY_AC;
    af.modifier = -1 * level;
    affect_to_char(victim, &af);
    send_to_char("&+BBands of psionic force surround you!\r\n", victim);
    
    act("$n becomes a little fuzzy as mist like bands surround $m.",
      TRUE, victim, 0, 0, TO_ROOM);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_ENHANCE_ARMOR)
      {
        af1->duration = 25;
      }
  }

}

void spell_enhanced_strength(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;
  int dur;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = level;
  else
    dur = 100;
  
  if (affected_by_spell(victim, SPELL_ENHANCED_STR))
  {
    bool renew = false;
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_ENHANCED_STR)
      {
        af1->duration = dur;
        renew = true;
      }
    }
    
    if(renew)
      send_to_char("Your enhanced &+Rstrength&n is &+yrenewed!\r\n", victim);
      
    return;
  }
  
  bzero(&af, sizeof(af));
  af.type = SPELL_ENHANCED_STR;
  af.duration = dur;
  
  af.location = APPLY_STR;
  af.modifier = 1 + (level / 4);
  affect_to_char(victim, &af);

  if(level > 51)
  {
    af.location = APPLY_STR_MAX;
    af.modifier = MAX(0, level - 50);
    affect_to_char(victim, &af);
  }
  
  send_to_char("Your muscles begin to expand.\r\n", victim);
  
  act("$n looks physically stronger.",
      TRUE, victim, 0, 0, TO_ROOM);

  return;
}

void spell_enhanced_dexterity(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  int dur;
  
  if(!(ch) ||
   !IS_ALIVE(ch))
      return;
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = level;
  else
    dur = 100;
  
  if(affected_by_spell(victim, SPELL_ENHANCED_DEX))
  {
    bool renew = false;
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_ENHANCED_DEX)
      {
        af1->duration = dur;
        renew = true;
      }
    }
    
    if(renew)
      send_to_char("Your enhanced &+ccoordination&n is &+yrenewed!\r\n", victim);
      
    return;
  }
  
  bzero(&af, sizeof(af));
  af.type = SPELL_ENHANCED_DEX;
  af.duration = dur;
  af.location = APPLY_DEX;
  af.modifier = 1 + (level / 4);
  affect_to_char(victim, &af);
  
  if(level > 51)
  {
    af.location = APPLY_DEX_MAX;
    af.modifier = MAX(0, level - 50);
    affect_to_char(victim, &af);
  }

  send_to_char("Your coordination increases greatly.\r\n", victim);

  return;
}

void spell_enhanced_agility(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;
  int dur;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = level;
  else
    dur = 100;
  
  if(affected_by_spell(victim, SPELL_ENHANCED_AGI))
  {
    bool renew = false;
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_ENHANCED_AGI)
      {
        af1->duration = dur;
        renew = true;
      }
    }
    
    if(renew)
      send_to_char("Your enhanced &+Bagility&n is &+yrenewed!\r\n", victim);
      
    return;
  }
  
  bzero(&af, sizeof(af));
  af.type = SPELL_ENHANCED_AGI;
  af.duration = dur;
  af.location = APPLY_AGI;
  af.modifier = 1 + (level / 4);
  affect_to_char(victim, &af);
  
  if(level > 51)
  {
    af.location = APPLY_AGI_MAX;
    af.modifier = MAX(0, level - 50);
    affect_to_char(victim, &af);
  }

  send_to_char("You feel more agile.\r\n", victim);

  return;
}

void spell_enhanced_constitution(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct affected_type af;
  int dur;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = level;
  else
    dur = 100;
  
  if(affected_by_spell(victim, SPELL_ENHANCED_CON))
  {
    bool renew = false;
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_ENHANCED_CON)
      {
        af1->duration = dur;
        renew = true;
      }
    }
    
    if(renew)
      send_to_char("Your enhanced &+Yconstitution&n is &+yrenewed!\r\n", victim);
      
    return;
  }
  
  bzero(&af, sizeof(af));
  af.type = SPELL_ENHANCED_CON;
  af.duration = dur;
  
  af.location = APPLY_CON;
  af.modifier = 1 + (level / 5);
  affect_to_char(victim, &af);
  
  if(level > 51)
  {
    af.location = APPLY_CON_MAX;
    af.modifier = MAX(0, level - 50);
    affect_to_char(victim, &af);
  }

  send_to_char("You feel more &+Cvitalized.\r\n", victim);
  return;
}

void spell_flesh_armor(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  struct affected_type af;
  int mod, dur;

  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = level;
  else
    dur = 100;
  
  if(IS_AFFECTED5(ch, AFF5_FLESH_ARMOR))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_FLESH_ARMOR)
      {
        af1->duration = dur;
      }
    send_to_char("&+yYour fleshy armor is renewed!\r\n", victim);
    return;
  }

  if(GET_CLASS(ch, CLASS_PSIONICIST | CLASS_MINDFLAYER))
    mod = -(level + 50);
  else
    mod = -(level);
    
  bzero(&af, sizeof(af));
  af.type = SPELL_FLESH_ARMOR;
  af.duration = dur;
  af.location = APPLY_AC;
  af.bitvector5 = AFF5_FLESH_ARMOR;
  af.modifier = mod;
  affect_to_char(victim, &af);

  send_to_char("&+RYour flesh gains a steel-like hardness.\r\n", victim);
  act("$N's &+Rflesh begins to shimmer, then gains a harder look to it.",
      TRUE, ch, 0, victim, TO_ROOM);

  return;
}

void spell_inertial_barrier(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
        
  if (affected_by_spell(ch, SPELL_INERTIAL_BARRIER))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_INERTIAL_BARRIER)
      {
        if(GET_LEVEL(ch) >= 57 ||
           IS_ELITE(ch))
              af1->duration = 100;
        else if (GET_LEVEL(ch) > 50)
          af1->duration = 30;
        else if (GET_LEVEL(ch) > 40)
          af1->duration = 15;
        else
          af1->duration = 10;
      }
    send_to_char("Your inertial barrier is reinforced!\r\n", victim);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_INERTIAL_BARRIER;
  if(GET_LEVEL(ch) >= 57 ||
     IS_ELITE(ch))
        af.duration = 100;
  else if (GET_LEVEL(ch) > 50)
    af.duration = 30;
  else if (GET_LEVEL(ch) > 40)
    af.duration = 15;
  else
    af.duration = 6;
  af.modifier = 0;
  af.bitvector3 = AFF3_INERTIAL_BARRIER;
  affect_to_char(victim, &af);

  return;
}

void spell_inflict_pain(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "$N &+Bscreams out in &+Rpain!&N",
    "&+BA horrible &+Rpain&N wracks your brain!",
    "$N &+Bscreams out in &+Rpain!&N"
  };
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;
        
  if(IS_BRAINLESS(victim))
  {
    send_to_char("Your victim lacks a brain!\r\n", ch);
    return;
  }
  
  if(number(0, 1) &&
     resists_spell(ch, victim))
      return;
      
  dam = (dice(((level / 10) + 4 ), 6) * 8);
  
  if(victim == ch)
    dam = 1;

  if(spell_damage(ch, victim, dam, SPLDAM_PSI,
    SPLDAM_NOSHRUG | SPLDAM_GLOBE | RAWDAM_NOKILL, &messages) ==
      DAM_NONEDEAD)
       if(!IS_STUNNED(victim) &&
         !IS_GREATER_RACE(victim) &&
         !IS_ELITE(victim) &&
         StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)) &&
         level > GET_LEVEL(victim) &&
         GET_C_POW(ch) > number(1, 1000))
          Stun(ch, PULSE_VIOLENCE * number(2, 5) / 2);
}

void spell_intellect_fortress(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  
  int dur;
  
  if(IS_PC(ch) ||
   IS_PC_PET(ch))
      dur = (int)(level / 2);
  else
    dur = 100;

  if (affected_by_spell(ch, SPELL_INTELLECT_FORTRESS))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_INTELLECT_FORTRESS)
      {
        af1->duration = dur;
      }
      
    send_to_char("Your mental &+Lfortress&n is renewed!\r\n", victim);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_INTELLECT_FORTRESS;
  af.duration = dur;
  af.location = APPLY_INT;
  af.modifier = (int)(1 + level/5);
  affect_to_char(ch, &af);

  send_to_char("&+WA mental fortress forms around you.\r\n", ch);

  return;
}

void spell_lend_health(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  int      hp;
  float    ratio;
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
        return;

  if (ch == victim)
  {
    send_to_char("Lend health to yourself? What a weirdo.\r\n", ch);
    return;
  }
  
  hp = MIN(25 + GET_LEVEL(ch) * 3 / 2 , GET_MAX_HIT(victim) - GET_HIT(victim));

  ratio = get_property("spell.lendHealth.healRatio", 1.000);
  
  if ((GET_HIT(ch) - (hp/ratio)) <= 1)
  {
    send_to_char("&+YYou realize this will kill you...\r\n", ch);
    return;
  }
  
  if ((GET_HIT(ch) - hp) <= 1)
  {
    send_to_char("You do not feel healthy enough yourself!\r\n", ch);
    return;
  }

  heal(victim, ch, (int)(ratio * hp), GET_MAX_HIT(victim) - number(1, 4));

  GET_HIT(ch) -= (int)(hp/ratio);
  
  if(GET_HIT(ch) > GET_MAX_HIT(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);
  
  // if(GET_HIT(ch) < 1)
  // {
    // int tmp = GET_HIT(ch)-1;
    // GET_HIT(ch) = 1;
    // act("&+WExhausted, $n &+Wpasses out!", FALSE, ch, 0, 0, TO_ROOM);
    // send_to_char("&+WHealth reserves exhausted, you pass out!\r\n", ch);
    // KnockOut(ch, -(int) tmp);
  // }
  
  if(GET_HIT(victim) > GET_MAX_HIT(victim))
    GET_HIT(victim) = GET_MAX_HIT(victim);

  update_pos(victim);
  update_pos(ch);

  act("&+WYou lend some of your health to $N.",
    0, ch, 0, victim, TO_CHAR);
  act("&+W$n lends you some of $s health.",
    0, ch, 0, victim, TO_VICT);
  act("&+W$n lends $N some of $s health.",
    0, ch, 0, victim, TO_VICT);

  return;
}

void spell_confuse(int level, P_char ch, char *arg, int type, P_char victim,
              P_obj obj)
{
  int max_level = 0;
  P_char   tch, next;
  P_char   cht, next_ch;

  if(!(ch) ||
     !IS_ALIVE(ch))
      return;

  if (victim == ch)
  {
    send_to_char("You suddenly decide against that, oddly enough.\r\n", ch);
    return;
  }
  
  appear(ch);

  act("&+LYou send out a wave of mental &+Yc&+yo&+Yn&+yf&+Yu&+ys&+Yi&+yo&+Yn!&n",
    FALSE, ch, 0, 0, TO_CHAR);
  act("$n &+Lprojects out a wave of &+Mos&+mci&+Mll&+mat&+Min&+mg &+cenergy!&n",
    FALSE, ch, 0, 0, TO_ROOM);

  max_level = level * 3;
  
  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;
    
    if(!should_area_hit(ch, tch))
      continue;
      
    if(IS_NPC(ch) &&
       !IS_PC_PET(ch) &&
       IS_NPC(tch) &&
       !IS_PC_PET(tch))
        continue;
      
    if(max_level <= 0)
      return;
      
    if(IS_BRAINLESS(tch) ||
       grouped(ch, tch) ||
       !AWAKE(tch) ||
       IS_IMMOBILE(tch) ||
       GET_HIT(tch) < 1)
          continue;
      
    if(IS_NPC(tch) &&
       GET_VNUM(tch) == 250)
          continue;
    
    if(!StatSave(tch, APPLY_POW, POW_DIFF(ch, tch)))
    {
      switch (number(1, 10))
      {
      case 1:
      case 2:
      case 3:
        Stun(tch, (2 * PULSE_VIOLENCE));
        break;
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
       
        send_to_char("&+YMaddening thoughts overcome you!\r\n", tch);
        if(IS_FIGHTING(tch))
          stop_fighting(tch);
        
        if(IS_PC(tch))
          do_flee(tch, 0, 2);
        else if(!number(0, 1))
                 do_flee(tch, 0, 2);
          
        break;
      case 9:
      case 10:
        if(IS_NPC(tch) &&
          !IS_PC_PET(tch))
              break;
        for (cht = world[ch->in_room].people; cht; cht = cht->next_in_room)
        {
          if (number(1, 101) <= 30)
          {
            if (cht != tch)
            {
              stop_fighting(tch);
              set_fighting(tch, cht);
              break;
            }
          }
        }
      }
      
      max_level -= GET_LEVEL(tch);
    }
    else if (IS_NPC(tch) && CAN_SEE(tch, ch))
    {
      remember(tch, ch);
      if (!IS_FIGHTING(tch))
        MobStartFight(tch, ch);
    }
  }
  if(GET_RACE(ch) == RACE_GITHYANKI)
    (GET_MANA(ch) -= 20);
}

void spell_enrage(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{

  if(!(ch) ||
     !IS_ALIVE(ch))
      return;

#if 0
  if (ch == victim)
  {
    send_to_char("You cannot do this to yourself!\r\n", ch);
    return;
  }
#endif

  appear(ch);

  if ((ch != victim) &&
      StatSave(victim, APPLY_POW, 0))
  {                          
    send_to_char("You feel a brief bit of &+Ranger&n, but it passes.\r\n", victim);
    return;
  }

  berserk(victim, 10);  /* flag to start regardless of skill */
  return;
}

void spell_mind_blank(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{
  struct affected_type af;
  int dur;

  if(!(ch) ||
     !IS_ALIVE(ch))
      return;
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = level;
  else
    dur = 100;
  
  if (CheckMindflayerPresence(ch))
  {
    send_to_char
      ("You sense an ethereal disturbance and abort your spell!\r\n", ch);
    return;
  }

  if (affected_by_spell(ch, SPELL_MIND_BLANK))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_MIND_BLANK)
      {
        af1->duration = dur;
      }
    send_to_char("Ah yes, you blank your mind of all thoughts...\r\n", victim);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_MIND_BLANK;
  af.duration = dur;
  af.bitvector3 = AFF3_NON_DETECTION;
  affect_to_char(ch, &af);

  act("&+YYour mind now protect you against most detection!",
    FALSE, ch, 0, 0, TO_CHAR);

  return;
}

void spell_cannibalize(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{
  struct affected_type af;
  int dur;

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus params in cannibalize");
    raise(SIGSEGV);
  }
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = level;
  else
    dur = 100;
    
  if (affected_by_spell(ch, SPELL_CANNIBALIZE))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_CANNIBALIZE)
      {
        af1->duration = dur;
      }
  
  act("You will leach &+Rpower&n from those who oppose you!",
    FALSE, ch, 0, 0, TO_CHAR);
  act("$n is surrounded by a &+Mmagenta haze&n quick dissipates quickly.",
    FALSE, ch, 0, 0, TO_CHAR);
    
    return;
  }
  bzero(&af, sizeof(af));
  af.type = SPELL_CANNIBALIZE;
  af.duration = dur;
  af.bitvector3 = AFF3_CANNIBALIZE;
  affect_to_char(ch, &af);

  act("&+YYou will now drain power from your victim!",
    FALSE, ch, 0, 0, TO_CHAR);

  return;
}

void spell_tower_iron_will(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;
  int dur;

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus params in tower of iron will");
    raise(SIGSEGV);
  }
  
  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        dur = (int)(level / 2);
  else
    dur = 100;
    
  if (affected_by_spell(ch, SPELL_TOWER_IRON_WILL))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_TOWER_IRON_WILL)
      {
        af1->duration = dur;
      }
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_TOWER_IRON_WILL;
  af.duration = dur;
  af.bitvector3 = AFF3_TOWER_IRON_WILL;
  affect_to_char(ch, &af);

  act("&+YYour mind will now be more resistant to psionic attack!",
      FALSE, ch, 0, 0, TO_CHAR);

  return;
}

void spell_innate_blast(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;
  int      lev = level;

  if (!((victim || obj) && ch))
  {
    return;
  }
  if (GET_STAT(ch) == STAT_DEAD)
    return;

  if (IS_TRUSTED(victim))
  {
    send_to_char("If only.\r\n", ch);
    return;
  }

  if (number(0, 1))
    appear(ch);

  if (!number(0, 2) &&
      !StatSave(victim, APPLY_POW,
                MAX(-3, (GET_LEVEL(ch) - GET_LEVEL(victim)) / 3)) &&
      !(IS_NPC(victim) && IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA)))
  {

    bzero(&af, sizeof(af));

    if ((GET_LEVEL(ch) > 25) && number(0, 1))
    {
      af.type = SPELL_MAJOR_PARALYSIS;
      af.bitvector2 = AFF2_MAJOR_PARALYSIS;
    }
    else
    {
      af.type = SPELL_MINOR_PARALYSIS;
      af.bitvector2 = AFF2_MINOR_PARALYSIS;
    }

    if (!IS_AFFECTED2(victim, AFF2_MAJOR_PARALYSIS) &&
        !IS_AFFECTED2(victim, AFF2_MINOR_PARALYSIS))
    {
      af.duration = (lev >> 5 + 1);
      if (af.duration < 1)
        af.duration = 1;

      affect_to_char(victim, &af);

      act("$n &+Mceases to move.. still and lifeless.",
          FALSE, victim, 0, 0, TO_ROOM);
      send_to_char
        ("&+LYou are paralyzed!\r\n",
         victim);
      if (IS_FIGHTING(victim))
        stop_fighting(victim);

      /*
       * stop all non-vicious/agg attackers
       */
      StopMercifulAttackers(victim);
    }

    if (CAN_SEE(victim, ch))
      remember(victim, ch);
  }
  else if (IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if (!IS_FIGHTING(victim))
      MobStartFight(victim, ch);
  }
}


void
spell_radial_navigation(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj tar_obj)
{
  char     arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  int      distance, i, dir, to_room, temp, curr_room;

  if (!ch)
    return;

  argument_interpreter(arg, arg1, arg2);

/* Bypassing class check. Lucrot 31Aug2008
 *  if (!IS_TRUSTED(ch) && !GET_CLASS(ch, CLASS_PSIONICIST))
 * {
 *   send_to_char("You cannot master that power!\r\n", ch);
 *   return;
 * }
*/
 
 if (!arg1)
  {
    send_to_char("You must specify a direction of travel: n,s,e,w\r\n", ch);
    return;
  }
  if (!arg2)
  {
    send_to_char("You must specify the distance you wish to travel.\r\n", ch);
    return;
  }
  switch (*arg1)
  {
  case 'n':
    dir = 0;
    break;
  case 'e':
    dir = 1;
    break;
  case 's':
    dir = 2;
    break;
  case 'w':
    dir = 3;
    break;
  default:
    send_to_char("That is not a valid direction.\r\n", ch);
    return;
  }
  if (!(is_number(arg2) && (distance = atoi(arg2))))
  {
    send_to_char("That is not a proper distance.\r\n", ch);
    return;
  }
  if (distance > (GET_LEVEL(ch) + 10))
  {
    send_to_char("Your meager mentality cannot carry you that far!\r\n", ch);
    return;
  }
  to_room = ch->in_room;
  bfs_clear_marks();
  for (i = 0; i < distance; i++)
  {
    if (!VALID_RADIAL_EDGE(to_room, dir))
      break;
    to_room = TOROOM(to_room, dir);
  }

  temp = world[ch->in_room].sector_type;
  if (!IS_TRUSTED(ch) &&
      ((to_room == NOWHERE) || (to_room == ch->in_room) ||
       !IS_MAP_ROOM(ch->in_room) || !IS_MAP_ROOM(to_room) ||
       IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
       IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
       (GET_MASTER(ch) && IS_PC(victim))))
  {
    act
      ("&+L$n's&+L outline begins to &n&+rvibrate&+L, then &+Bblur&+L.... then stabilize.&N",
       TRUE, ch, 0, 0, TO_ROOM);
    act("&+wYour will is not strong enough to carry you away.&n", TRUE, ch, 0,
        0, TO_CHAR);
  }
  else
  {
    act
      ("&+L$n's&+L outline begins to &n&+rvibrate&+L, then &+Bblur&+L, then.....    disappear.&N",
       TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, to_room, -1);
    send_to_char("&+LYou feel exhausted from the jaunt.&N\r\n", ch);
    act
      ("&+LFrom out of nowhere $n's&+L blurred form streaks into the room!&N",
       TRUE, ch, 0, 0, TO_ROOM);
  }

  /* subtract mana regardless of success..  snif */

  GET_MANA(ch) -= (distance * 2);
  CharWait(ch, 10);

  /* twould be best to add more to their knock out time if they're already knocked out, but f it */

  if ((GET_MANA(ch) < 0) && !IS_AFFECTED(ch, AFF_KNOCKED_OUT))
  {
    act("Exhausted, $n passes out!", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("Reserves exhausted, you pass out!\r\n", ch);
    KnockOut(ch, -(int) ((float) (GET_MANA(ch)) * 1.5));
  }
}


void spell_ethereal_rift(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  int      to_room, dam, temp, found = 0;
  int      z, room, mask;
  struct remember_data *chars_in_zone;
  P_char   tmp, tmp_next;


  if (!ch)
    return;

  if (!victim)
    victim = ch;

  if (IS_NPC(victim))
  {
    send_to_char("Nice try...\r\n", ch);
    return;
  }

  dam = 1;
  if (dam <= 0)
    dam = 1;

  do
  {
    to_room = number(zone_table[world[victim->in_room].zone].real_bottom,
                     zone_table[world[victim->in_room].zone].real_top);
    found++;
  }
  while ((IS_SET(world[to_room].room_flags, PRIVATE) ||
          IS_SET(world[to_room].room_flags, PRIV_ZONE) ||
          IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
          IS_HOMETOWN(to_room) ||
          world[to_room].sector_type == SECT_OCEAN) && found < 1000);

  if ((IS_SET(world[victim->in_room].room_flags, PRIVATE) ||
       IS_SET(world[victim->in_room].room_flags, PRIV_ZONE) ||
       IS_SET(world[victim->in_room].room_flags, NO_TELEPORT) ||
       IS_HOMETOWN(victim->in_room) ||
       world[victim->in_room].sector_type == SECT_OCEAN))
    to_room = ch->in_room;

  if ((IS_SET(world[ch->in_room].room_flags, PRIVATE) ||
       IS_SET(world[ch->in_room].room_flags, PRIV_ZONE) ||
       IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
       IS_HOMETOWN(ch->in_room) ||
       world[ch->in_room].sector_type == SECT_OCEAN))
    to_room = ch->in_room;

  if (found == 1000)
    to_room = ch->in_room;

  send_to_char
    ("Your mind catapults you through a tear in the ethereal fabric.\r\n",
     ch);

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp_next)
  {
    tmp_next = tmp->next_in_room;
    if (IS_AFFECTED(tmp, AFF_BLIND) || (tmp == ch))
      continue;
    act
      ("&+LA dark tear in reality opens, and $n shimmers and fades away!\r\n",
       FALSE, ch, 0, tmp, TO_VICT);
  }
  char_from_room(ch);
  ch->specials.z_cord = victim->specials.z_cord;
  char_to_room(ch, to_room, -1);
  send_to_char
    ("Your body aches as your molecules reassemble themselves. \r\n", ch);

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp_next)
  {
    tmp_next = tmp->next_in_room;
    if (IS_AFFECTED(tmp, AFF_BLIND) || (tmp == ch))
      continue;
    act("&+LA black cloud shimmers and slowly forms into $n!\r\n", FALSE, ch,
        0, tmp, TO_VICT);
  }

  zone_powerspellmessage(to_room,
                         "&+LYou sense a disturbance in the ethereal fabric.\r\n");

  CharWait(ch, 3);
  return;
}

void spell_ether_warp(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  int in_room;
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(ch->in_room))
        return;

  if(GET_SPEC(ch, CLASS_PSIONICIST, SPEC_PSYCHEPORTER))
    CharWait(ch, number(4, 10));
  else if(IS_PC(ch))
    CharWait(ch, 80);
  else
    CharWait(ch, 5);

  if(!can_relocate_to(ch, victim))
    return;
    
  act("&+WYour body explodes into &+Ylight.&n",
    FALSE, ch, 0, 0, TO_CHAR);
  act("&+YBeams of light emit from $n, and $n explodes into &+cpure energy!&n",
    FALSE, ch, 0, 0, TO_ROOM);
  
  char_from_room(ch);
  char_to_room(ch, victim->in_room, -1);
  
  act("&+yYour body reforms from the &+Ylight...&n",
    FALSE, ch, 0, 0, TO_CHAR);
  act("Beams of &+Ylight&n appear from nowhere, and $n materializes from nowhere!",
    FALSE, ch, 0, 0, TO_ROOM);
}

void spell_thought_beacon(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj tar_obj)
{
  P_obj    beacon;
  struct affected_type af, *afp;
  int duration = level * 4 * WAIT_MIN;

  if (IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
      world[ch->in_room].sector_type == SECT_OCEAN) {
    send_to_char("Something blocks your mental powers here.\n", ch);
    return;
  }

  if (afp = get_spell_from_char(ch, SPELL_THOUGHT_BEACON)) {
    beacon = get_obj_in_list_num(real_object(416), world[afp->modifier].contents);
    if (beacon)
      extract_obj(beacon, FALSE);
    afp->modifier = ch->in_room;
  } else {
    memset(&af, 0, sizeof(af));
    af.type = SPELL_THOUGHT_BEACON;
    af.flags = /*AFFTYPE_NOSHOW | AFFTYPE_NOSAVE |*/ AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
    af.modifier = ch->in_room;
    af.duration = duration / PULSES_IN_TICK;

    affect_to_char(ch, &af);
  }

  beacon = read_object(real_object(416), REAL);

  if (!beacon)
  {
    logit(LOG_DEBUG, "spell_thought_beacon(): obj x not loadable");
    return;
  }

  act("&+LYou sink into your &+Mtho&+Wug&+Mhts, &+Lamplifying the "
      "&+bpsy&+mch&+bic &+wene&+Wrgi&+wes&+L in this room.&n",
      FALSE, ch, beacon, 0, TO_CHAR);
  act("&+L$n seems to concentrate for a moment, and a dark mist coalesces "
      "in the center of the room.&n",
      FALSE, ch, beacon, 0, TO_ROOM);

  set_obj_affected(beacon, duration, TAG_OBJ_DECAY, 0);

  beacon->value[0] = GET_PID(ch);

  obj_to_room(beacon, ch->in_room);
}

void spell_wormhole(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct portal_settings set = {
      770, /* portal type  */
      -1,  /* from room */
      -1,  /* to room */
      0,   /* How many can pass before closes */
      0,   /* Timeout before anyone can enter after open */
      0,   /* Timeout before next person can enter */
      0,   /* Lag person gets when steps out portal */
      0    /* Portal decay timer */
  };
  struct portal_create_messages msg = {
    /*ch   */ "&+LThe rift swirls for a moment and disperses.\r\n",
    /*ch r */ "&+LA wormhole swirls for a moment and then disperses.",
    /*vic  */ 0,
    /*vic r*/ 0,
    /*ch   */ "&+LRipping through reality, a black rift opens up here.\r\n",
    /*ch r */ "&+LRipping through reality, a black rift opens up here.\r\n",
    /*vic  */ "&+LThe air before you seems to rend and tear, revealing a black rift.\r\n",
    /*vic r*/ "&+LThe air before you seems to rend and tear, revealing a black rift.\r\n",
    /*npc  */ "Can only open a wormhole to another player!\n",
    /*bad  */ 0
  };

   P_obj    beacon;
   struct affected_type *afp;
   int      to_room, from_room;
   int      count;
   int      distance;
   bool     success = true;

  if(!ch) // Something is amiss... Nov08 -Lucrot
  {
    logit(LOG_EXIT, "spell_wormhole called in psionics.c with no ch.");
    raise(SIGSEGV);
  }

   if (!victim)
   {
    victim = ch;
   }

   /*
   if(IS_ILLITHID(ch) && !IS_ILLITHID(victim) && !IS_TRUSTED(ch))
   {
      send_to_char("You can't target them!\r\n", ch);
      return;
   }
   */
   /*
   if (GET_RACE(ch) == RACE_GITHYANKI)
   {
      send_to_char("&+LYour mind is too puny to create such a rift!&N\r\n", ch);
      return;
   }
   */
   
   // disables BEACON
   /*if (victim == ch)
   {
      return;
   }
   */
  if(ch) // Just making sure... Nov08 -Lucrot
  {
     if (victim == ch)
     {
      afp = get_spell_from_char(ch, SPELL_THOUGHT_BEACON);
      if (afp)
      {
         to_room = afp->modifier;
         for (beacon = world[to_room].contents; beacon; beacon = beacon->next_content)
         {
          if( obj_index[beacon->R_num].virtual_number == 416 && beacon->value[0] == GET_PID(ch))
           break;
         }
         if (!beacon)
            success = false;
      }
      else
         success = false;
     }
     else
       to_room = victim->in_room;

     from_room = ch->in_room;
    
     if(IS_NPC(victim))
     {
       if(!has_innate(ch, INNATE_IMPROVED_WORMHOLE) ||
        IS_SET(victim->specials.act, ACT_NO_SUMMON) )
         success = false;
       else
       {
         distance = level;

         if( how_close(ch->in_room, to_room, distance) < 0 &&
           how_close(to_room, ch->in_room, distance) < 0)
          success = false;
       }
     }

#if 0
     if(!OUTSIDE(ch))
     {
        send_to_char("You must be outside to cast this spell!\r\n", ch);
        return;
     }
#endif
// Stopping the wayward Illithid griefing. Nov08 -Lucrot
    if(victim &&
      !IS_TRUSTED(ch) &&
      IS_PC(victim) &&
      IS_ILLITHID(ch) &&
      !IS_ILLITHID(victim) &&
      GET_LEVEL(victim) < 51)
    {
      send_to_char("&+CBother something else that isn't so weak!&n\r\n", ch);
      send_to_char("&+MThe Elder Brain is displeased and you feel numbness engulf you.&n\r\n", ch);
      wizlog(57,
             "%s attmpted to wormhole %s, and made the &+MElder Brain&n displeased.",
             GET_NAME(ch), GET_NAME(victim));
      CharWait(ch, (int)(PULSE_SPELLCAST * 10));
      return;
    }
    if(victim &&
      IS_NPC(victim) &&
      victim != ch &&
      !IS_TRUSTED(ch) &&
      has_innate(ch, INNATE_IMPROVED_WORMHOLE) &&
      IS_SET(victim->specials.act, ACT_NO_SUMMON))
    {
      act(msg.fail_to_caster, FALSE, ch, 0, 0, TO_CHAR);
      act(msg.fail_to_caster_room, FALSE, ch, 0, 0, TO_ROOM);
      return;
    }
     
     int specBonus = 0;
     set.to_room = to_room; 
     int maxToPass          = get_property("portals.wormhole.maxToPass", 3);
     set.init_timeout       = get_property("portals.wormhole.initTimeout", 3);
     set.post_enter_timeout = get_property("portals.wormhole.postEnterTimeout", 0);
     set.post_enter_lag     = get_property("portals.wormhole.postEnterLag", 0);
     set.decay_timer        = get_property("portals.wormhole.decayTimeout", 60 * 2);

     if(has_innate(ch, INNATE_IMPROVED_WORMHOLE))
     {
       specBonus = 3;
       set.decay_timer = (set.decay_timer / 2) * 3;
     }
     set.throughput = (int)( (ch->player.level-46)/2 ) + number( 1, maxToPass + specBonus);

     if(!can_do_general_portal(level, ch, victim, &set, &msg)
      || (!IS_TRUSTED(ch)       && (!success || GET_MASTER(ch))))
     {
       act(msg.fail_to_caster,      FALSE, ch, 0, 0, TO_CHAR);
       act(msg.fail_to_caster_room, FALSE, ch, 0, 0, TO_ROOM);
       return;
     }

     if(IS_NPC(victim) && !IS_TRUSTED(ch) && !has_innate(ch, INNATE_IMPROVED_WORMHOLE) )
     {
      send_to_char(msg.npc_target_caster, ch);
      return;
     }

     spell_general_portal(level, ch, victim, &set, &msg);
  }
}

void spell_excogitate(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int diff;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim)) 
        return;

  diff = GET_C_POW(ch) - GET_C_POW(victim);

  if(diff < -20)
  {
    act("$N's mental power is &+yfar greater than yours.&n",
      TRUE, ch, 0, victim, TO_CHAR);
    act("$n has probed your mind!", TRUE, ch, 0, victim, TO_VICT);
    return;
  }
  else if(diff < -10)
  {
    act("$N's mental power is &+ygreater&n than yours.",
      TRUE, ch, 0, victim, TO_CHAR);
    act("$n has probed your mind!", TRUE, ch, 0, victim, TO_VICT);
    return;
  }
  else if(diff < 10)
  {
    act("$N's mental power is roughly &+yequal&n to yours.", TRUE, ch, 0, victim, TO_CHAR);
    act("You feel something brush your thoughts!", TRUE, ch, 0, victim, TO_VICT);
    return;
  }
  else if(diff < 20)
  {
    act("$N's mental power is &+yless&n than yours.", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  else if(diff < 30)
  {
    act("$N's mental power is &+ymuch less&n than yours.", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  else
  {
    act("Your mental powers are &+Ymuch &+ygreater&n than $N's.&n", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
}

void event_psionic_wave_blast(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      level, dam, in_room;
  struct affected_type *af;
  struct damage_messages messages = {
    "",
    "",
    "",
    "",
    "",
    "", 0
  };

  level = *((int *) data);

  for (af = victim->affected; af && af->type != SPELL_PSIONIC_WAVE_BLAST; af = af->next)
    ;

  if(af == NULL)
  {
    if(IS_SET((ch)->specials.affected_by2, AFF2_CASTING))
      REMOVE_BIT((ch)->specials.affected_by2, AFF2_CASTING);
    return;
  }
  
  if(af &&
     ((GET_POS(ch) < POS_STANDING) ||
       IS_STUNNED(ch)))
  {
    send_to_char("\r\nYour mental assault was interrupted!!!\r\n", ch);
  
    if(IS_SET((ch)->specials.affected_by2, AFF2_CASTING))
      REMOVE_BIT((ch)->specials.affected_by2, AFF2_CASTING);
    
    if(get_scheduled(ch, event_psionic_wave_blast))
       disarm_char_events(ch, event_psionic_wave_blast);
       
    if(affected_by_spell(victim, SPELL_PSIONIC_WAVE_BLAST))
      affect_from_char(victim, SPELL_PSIONIC_WAVE_BLAST);
  }
  
  if((af->modifier)-- == 0)
  {
    send_to_char("You have reach the maximum capability of your mental assault.\n", victim);
    
    if(affected_by_spell(victim, SPELL_PSIONIC_WAVE_BLAST))
      affect_from_char(victim, SPELL_PSIONIC_WAVE_BLAST);
    
    if(IS_SET((ch)->specials.affected_by2, AFF2_CASTING))
      REMOVE_BIT((ch)->specials.affected_by2, AFF2_CASTING);
    
    return;
  }

  if(!number(0, 2) &&
     resists_spell(ch, victim))
  {
    if(!(IS_SET((ch)->specials.affected_by2, AFF2_CASTING)))
      SET_BIT((ch)->specials.affected_by2, AFF2_CASTING);
    
    GET_MANA(ch) -= 50;
    
    add_event(event_psionic_wave_blast, PULSE_VIOLENCE, ch,
      victim, NULL, 0, &level, sizeof(level));
    return;
  }
  
  dam = dice(level , 16);
  
  if(!StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
    dam = (int)(dam * 1.25);
  
  if(ch->in_room != victim->in_room)
    dam /= 3;

  if(spell_damage(ch, victim, dam, SPLDAM_PSI, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages)
      == DAM_NONEDEAD)
  {
    if(!IS_SET((ch)->specials.affected_by2, AFF2_CASTING))
      SET_BIT((ch)->specials.affected_by2, AFF2_CASTING);
    
    GET_MANA(ch) -= 50;
    
    add_event(event_psionic_wave_blast, PULSE_VIOLENCE, ch, victim, NULL, 0, &level,
              sizeof(level));
    return;
  }
  
  if(IS_SET((ch)->specials.affected_by2, AFF2_CASTING))
    REMOVE_BIT((ch)->specials.affected_by2, AFF2_CASTING);
}

void spell_psionic_wave_blast(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "You begin your mental assault upon $N.&n",
    "Something invades your mind. You feel at odds with yourself. Another entity is attempting to dominate you!&n",
    "$N begins to look wild, disheveled, and completely discombobulated!&n",
    "&n",
    "",
    "", 0
  };
  
  int dam;
  struct affected_type *af;
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
      return;
  
  if(IS_BRAINLESS(victim))
  {
    send_to_char("You probe your victim only to discover the lack of intelligence.\r\n", ch);
    if(IS_SET((ch)->specials.affected_by2, AFF2_CASTING))
      REMOVE_BIT((ch)->specials.affected_by2, AFF2_CASTING);
    return;
  }
  
  if((int)((GET_HIT(victim) / GET_MAX_HIT(victim)) * 100) < 20)
  {
    send_to_char("Your victim is in excruciating pain. You are unable to find the neural synapses to mentality assault!\r\n", ch);
    if(IS_SET((ch)->specials.affected_by2, AFF2_CASTING))
      REMOVE_BIT((ch)->specials.affected_by2, AFF2_CASTING);
    return;
  }
  
  dam = dice(level, 4);
  
  if(number(0, 2) &&  // 66 percent less shrugging of this spell at start.
    resists_spell(ch, victim))
      return;
  
  if(spell_damage(ch, victim, dam, SPLDAM_PSI, RAWDAM_NOKILL |
    SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, 0) != DAM_NONEDEAD)
      return;
    
  if(IS_ALIVE(victim))
  {
    struct affected_type new_affect;

    af = &new_affect;
    memset(af, 0, sizeof(new_affect));
    af->type = SPELL_PSIONIC_WAVE_BLAST;
    af->duration = 1;
    af->modifier = 6;
    affect_to_char(victim, af);
    add_event(event_psionic_wave_blast, 0, ch, victim, NULL, 0, &level, sizeof(level));
    SET_BIT((ch)->specials.affected_by2, AFF2_CASTING);
  }
}

void spell_depart(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char  t_ch;
  int tries = 0, to_room, dir;
  int range = get_property("spell.depart.range", 5);
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(ch->in_room))
        return;

  if((IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
      IS_HOMETOWN(ch->in_room) ||
      world[ch->in_room].sector_type == SECT_OCEAN) &&
      level < 60)
  {
    send_to_char("The magic in this room prevents you from leaving.\n", ch);
    return;
  }
  
  if(!victim)
    victim = ch;
    
  if((ch && !is_Raidable(ch, 0, 0)) ||
     (victim && !is_Raidable(victim, 0, 0)))
  {
    send_to_char("&+WYou are not raidable. The spell fails!\r\n", victim);
    return;
  }

  if(IS_MAP_ROOM(victim->in_room))
  {
    to_room = victim->in_room;

    for( int i = 0; i < range; i++ )
    {
      tries = 0;
      do
        dir = number(0,3);

      while(tries++ < 10 &&
             !VALID_TELEPORT_EDGE(to_room, dir, victim->in_room));

      if(tries < 10)
        to_room = TOROOM(to_room, dir);
    }
  }
  else
  {
    do
    {
      to_room = number(zone_table[world[victim->in_room].zone].real_bottom,
          zone_table[world[victim->in_room].zone].real_top);
      tries++;
    }
    while((IS_SET(world[to_room].room_flags, PRIVATE) ||
          IS_SET(world[to_room].room_flags, PRIV_ZONE) ||
          IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
          IS_HOMETOWN(to_room) ||
          world[to_room].sector_type == SECT_OCEAN) &&
          tries < 1000);
  }

  if(tries >= 1000)
    to_room = victim->in_room;

  if(LIMITED_TELEPORT_ZONE(victim->in_room))
  {
    if(how_close(victim->in_room, to_room, 5))
      send_to_char
        ("The magic gathers, but somehow fades away before taking effect.\n", victim);
    return;
  }
  
  act("You will yourself to be elsewhere...&n",
    FALSE, victim, 0, 0, TO_CHAR);
  act("A &+Lblack line&n forms from nowhere and $n falls through it!&n",
    FALSE, victim, 0, 0, TO_ROOM);
  
  if(IS_FIGHTING(victim))
    stop_fighting(victim);
  
  if(victim->in_room != NOWHERE)
    for (t_ch = world[victim->in_room].people; t_ch; t_ch = t_ch->next)
      if(IS_FIGHTING(t_ch) &&
         t_ch->specials.fighting == victim)
            stop_fighting(t_ch);
  
  if(victim->in_room != to_room)
  {
    char_from_room(victim);
    char_to_room(victim, to_room, -1);
  }
  
  act("and appear somewhere else!&n",
    FALSE, victim, 0, 0, TO_CHAR);  
  act("A &+Lblack line&n appears from nowhere and $n rises directly out of it.",
    FALSE, victim, 0, 0, TO_ROOM);
    
  CharWait(victim, (3 * WAIT_SEC));
}

void spell_celerity(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  int movepoints;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(ch->in_room))
        return;

  movepoints = dice(3, level/2);

  if (GET_VITALITY(ch) > (GET_MAX_VITALITY(ch) - (movepoints/2)))
  {
        send_to_char("You are quite capable of running right now, no need for extra boost.\n", ch);
        return;
  }

  act("&+WYou concentrate and attempt to transform part of your lifeforce....&n",
    FALSE, ch, 0, victim, TO_CHAR);
  act("$N &+Wconcentrates and pales visibly.&n",
    FALSE, ch, 0, victim, TO_NOTVICT);

  if(spell_damage(ch, ch, movepoints*4, SPLDAM_PSI, 
     SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, 0) != DAM_NONEDEAD)
    return;

  GET_VITALITY(ch) = MIN(GET_VITALITY(ch)+movepoints, GET_MAX_VITALITY(ch));

  act("&+yFresh energy pours through your body. &+WYou are invigorated!&n",
    FALSE, ch, 0, 0, TO_CHAR);
  act("$N &+Wappears invigorated.&n",
    FALSE, ch, 0, 0, TO_ROOM);

  update_pos(ch);       
}

