/*
 * ***************************************************************************
 * *  File: magic.c                                            Part of Duris *
 * *  Usage: procedures to create spell affects
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 * * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "graph.h"
#include "guildhall.h"
#include "interp.h"
#include "new_combat_def.h"
#include "structs.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "utils.h"
#include "weather.h"
#include "sound.h"
#include "assocs.h"
#include "justice.h"
#include "mm.h"
#include "damage.h"
#include "objmisc.h"
#include "vnum.obj.h"
#include "utils.h"
#include "defines.h"
#include "necromancy.h"
#include "disguise.h"
#include "grapple.h"
#include "map.h"
#include "sql.h"
#include "graph.h"
#include "outposts.h"
#include "ctf.h"
#include "achievements.h"
#include "alliances.h"

/*
 * external variables
 */

extern const char *command[];
extern Skill skills[];
extern char *spells[];
extern P_index obj_index;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_char combat_list;
extern P_event current_event;
extern P_event event_list;
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern const char *apply_types[];
extern const flagDef extra_bits[];
extern const flagDef anti_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern const char *item_types[];
extern const struct stat_data stat_factor[];
extern int rev_dir[];
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int hometown[];
extern const int top_of_world;
extern struct str_app_type str_app[];
extern struct con_app_type con_app[];
extern struct time_info_data time_info;
extern struct weather_data weather_info;
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern const struct race_names race_names_table[];
extern const int numCarvables;
extern const char *carve_part_name[];
extern const int carve_part_flag[];
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern void set_long_description(P_obj t_obj, const char *newDescription);
extern void set_short_description(P_obj t_obj, const char *newDescription);
extern const struct golem_description golem_data[];
// THE NEXT PERSON THAT OUTRIGHT COPIES A SPELL JUST TO CHANGE THE NAME/MESSAGES
// IT OUTPUTS IS GOING TO BE CASTRATED BY ME AND FORCED TO EAT THEIR OWN GENITALIA.
// There is no reason to do this other than to make a headache for another coder.
// If you feel the need to have a "different" spell than one already in the game
// for racewar purposes or whatever, MAKE CHANGES TO THE ORIGINAL SPELL and call
// THAT SPELL with a command in interp.c.  There is no reason to have 3253232 different
// functions for the exact same spell(transmute/ethereal grounds etc.) - Jexni 3/28/11

void affect_to_end(P_char ch, struct affected_type *af);
int conjure_terrain_check(P_char, P_char);

int      portal_id;
extern bool identify_random(P_obj);
int      cast_as_damage_area(P_char,
                             void (*func) (int, P_char, char *, int, P_char,
                                           P_obj), int, P_char, float, float);
int      cast_as_damage_area(P_char,
                             void (*func) (int, P_char, char *, int, P_char,
                                           P_obj), int, P_char, float, float,
                             bool (*s_func) (P_char, P_char));
 
void do_nothing_spell(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
  {
	return;
  }

// Universal call to determine if victim can be relocated/warped/dreamed. 
bool can_relocate_to(P_char ch, P_char victim)
{
  int location = victim->in_room;

  if( !(victim) || !(location) || !can_enter_room(ch, location, FALSE)
    || (( racewar(ch, victim) || IS_NPC(victim)
    || IS_SET(world[ch->in_room].room_flags, SINGLE_FILE)
    || IS_SET(world[victim->in_room].room_flags, SINGLE_FILE) ) && !IS_TRUSTED(ch)) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return FALSE;
  }

  if( IS_NPC(ch) && IS_PC_PET(ch) )
  {
    return FALSE;
  }

  if( !IS_TRUSTED(ch) && IS_TRUSTED(victim) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return FALSE;
  }

  if( IS_AFFECTED3(victim, AFF3_NON_DETECTION) || (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE)
    && !is_linked_to(ch, victim, LNK_CONSENT)  && !IS_TRUSTED(ch)) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return FALSE;
  }

  if( IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE) && !is_introd(victim, ch) && !IS_TRUSTED(ch) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return FALSE;
  }

  if( IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) || IS_HOMETOWN(ch->in_room) )
  {
    send_to_char("The magic in this room prevents you from leaving.\n", ch);
    return FALSE;
  }

  if( IS_PC_PET(ch) && IS_PC(victim) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return FALSE;
  }

  if( IS_SET(world[location].room_flags, NO_TELEPORT) || IS_HOMETOWN(location)
    || world[location].sector_type == SECT_OCEAN )
  {
    send_to_char("&+CYou failed.\n", ch);
    return FALSE;
  }

  if( !is_Raidable(ch, 0, 0) )
  {
    send_to_char("&+WYou are not raidable. The spell fails!\r\n", ch);
    return FALSE;
  }

  if( IS_PC(ch) && IS_PC(victim) && !is_Raidable(victim, 0, 0) )
  {
    send_to_char("&+WYour target is not raidable. The spell fails!\r\n", ch);
    return FALSE;
  }

  if( ch->specials.z_cord > 0 || ch->specials.z_cord < 0 )
  {
    send_to_char("You must be firmly on the ground.\r\n", ch);
    return FALSE;
  }

  if( victim->specials.z_cord > 0 || victim->specials.z_cord < 0 )
  {
    send_to_char("Your target is either swimming or flying too high.\r\n", ch);
    return FALSE;
  }

  return TRUE;
}

bool has_air_staff_arti(P_char ch)
{
  P_obj staff1, staff2;
  
  if(IS_NPC(ch))
  {
    return false;
  }
  
  staff1 = ch->equipment[HOLD];
  staff2 = ch->equipment[WIELD];
  
  if((staff1 &&
     staff1->R_num == real_object(67207)) ||
    (staff2 &&
     staff2->R_num == real_object(67207)))
  {
    return true;
  }
  
  return false;
}

/*
 * this is utility function for area spells
 * it checks if the room character is in is affected
 * by the given spell cast by character or someone grouped
 * with him. if so, it returns the P_char pointing to
 * the original caster, otherwise it return NULL and
 * sets affect on the room for the given duration in seconds
*/
P_char stack_area(P_char ch, int spell, int duration)
{
  struct room_affect af, *afp;
  P_room   room = &world[ch->in_room];

  for (afp = room->affected; afp; afp = afp->next)
  {
    if(afp->type == spell && char_in_list(afp->ch) &&
        ((ch->group && ch->group == afp->ch->group) || ch == afp->ch))
      return afp->ch;
  }

  memset(&af, 0, sizeof(struct room_affect));
  af.duration = duration * WAIT_SEC;
  af.type = spell;
  af.ch = ch;
  affect_to_room(ch->in_room, &af);

  return NULL;
}

// New function for spell components - Lucrot 31Aug2008
int get_spell_component(P_char ch, int vnum, int max_components)
{
  P_obj    t_obj, next_obj;
  int      found = 0;

  for( t_obj = ch->carrying; t_obj && found < max_components; t_obj = next_obj )
  {
    next_obj = t_obj->next_content;
    if(obj_index[t_obj->R_num].virtual_number == vnum)
    {
      extract_obj(t_obj, TRUE); // Spell components shouldn't be artis.
      found++;
    }
  }
  return found;
}

/*
 * Offensive Spells
 */

#define RAY_YELLOW  BIT_1
#define RAY_ORANGE  BIT_2
#define RAY_VIOLET  BIT_3
#define RAY_GREEN   BIT_4
#define RAY_BLUE    BIT_5
#define RAY_INDIGO  BIT_6
#define RAY_AZURE   BIT_7
#define RAY_RED     BIT_8
#define RAY_COUNT   8

void prepare_ray_messages(char *color_string, char *ch_buffer,
                          char *vict_buffer, char *room_buffer)
{
  sprintf(ch_buffer, "You send a %s shaft of light streaking towards $N!",
          color_string);
  sprintf(vict_buffer, "$n sends a %s shaft of light streaking towards YOU!",
          color_string);
  sprintf(room_buffer, "$n sends a %s shaft of light streaking towards $N!",
          color_string);
}

void show_ray_messages(char *color_string, P_char ch, P_char victim)
{
  char     buffer[512];

  sprintf(buffer, "You send a %s shaft of light streaking towards $N!",
          color_string);
  act(buffer, FALSE, ch, 0, victim, TO_CHAR);
  sprintf(buffer, "$n sends a %s shaft of light streaking towards YOU!",
          color_string);
  act(buffer, FALSE, ch, 0, victim, TO_VICT);
  sprintf(buffer, "$n sends a %s shaft of light streaking towards $N!",
          color_string);
  act(buffer, FALSE, ch, 0, victim, TO_NOTVICT);
}


void spell_single_prismatic_ray(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  int      dam, ray_type;
  char     char_message[512], victim_message[512], room_message[512];
  struct damage_messages messages = {
    char_message, victim_message, room_message,
    "Your &+rmu&+Rlt&+Yi&+gco&+bl&+Bo&+Mr&+med&n rays of light shatter $N's existence into a million pieces.",
    "&+rMu&+Rlt&+Yi&+gco&+bl&+Bo&+Mr&+med&n rays of light shatter your existence into a million pieces.",
    "&+rMu&+Rlt&+Yi&+gco&+bl&+Bo&+Mr&+med&n rays of light shatter $N's existence into a million pieces.",
      0
  };

  if(arg)
    ray_type = *((int *) arg);
  else
    ray_type = 0;

  switch (ray_type)
  {
  case 0:
    prepare_ray_messages("&+rsh&+Ri&+Ym&n&+gm&+be&+Br&+Mi&n&+mng&+L",
                         char_message, victim_message, room_message);
    spell_damage(ch, victim, dice(level, 9), SPLDAM_GENERIC, 0, &messages);
    break;
  case RAY_RED:
    dam = 400 + dice(level, 2);
    prepare_ray_messages("&+rred&+w", char_message, victim_message,
                         room_message);
    if(NewSaves(victim, SAVING_SPELL, 0))
      dam >>= 1;
    spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
    break;
  case RAY_ORANGE:
    dam = 275 + dice(level, 2);
    prepare_ray_messages("&+Rorange&n", char_message, victim_message,
                         room_message);
    if(NewSaves(victim, SAVING_SPELL, 0))
      dam >>= 1;
    spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
    break;
  case RAY_BLUE:
    dam = 150 + dice(level, 2);
    prepare_ray_messages("&+Bblue&n", char_message, victim_message,
                         room_message);
    if(NewSaves(victim, SAVING_SPELL, 0))
      dam >>= 1;
    spell_damage(ch, victim, dam, SPLDAM_COLD, 0, &messages);
    break;
  case RAY_YELLOW:
    show_ray_messages("&+Yyellow&n", ch, victim);
    spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
    break;
  case RAY_INDIGO:
    /*show_ray_messages("&+bindigo&n", ch, victim);
    spell_feeblemind(level, ch, NULL, 0, victim, NULL);
    break; -This is crashing us and I cant figure out why - Drannak 3/19/14*/
    show_ray_messages("&+Yyellow&n", ch, victim);
    spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
    break;
  case RAY_GREEN:
    show_ray_messages("&+Ggreen&n", ch, victim);
    spell_poison(level, ch, 0, 0, victim, NULL);
    break;
  case RAY_VIOLET:
    show_ray_messages("&+mviolet&n", ch, victim);
    spell_dispel_magic(level, ch, 0, SPELL_TYPE_SPELL, victim, NULL);
    break;
  case RAY_AZURE:
    show_ray_messages("&+Bazure&n", ch, victim);
    spell_blindness(level, ch, 0, 0, victim, NULL);
    break;
  }
}

void spell_prismatic_ray(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  int      dam, rays, ray_type, room, i = 0;
  uint     ray_flag;
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(room = ch->in_room))
  {
    return;
  }
 
  spell_single_prismatic_ray(level, ch, 0, 0, victim, 0);

  for (rays = 2, ray_flag = 0; rays;)
  {
    if(!is_char_in_room(victim, room) || !is_char_in_room(ch, room))
      break;
    ray_type = 1 << number(0, RAY_COUNT - 1);
    if(ray_flag & ray_type)
      continue;
    rays--;
    ray_flag |= ray_type;
    spell_single_prismatic_ray(level, ch, (char *) &ray_type, 0, victim, 0);
    i++;
    if(GET_STAT(victim) == STAT_DEAD)
      break;
  }
}

void spell_color_spray(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      ray_type;
  P_char   tch, next;

  send_to_char("You send beams of &+Gcol&+Bor spr&+Ray&n from your hands.\n",
               ch);
  act("$n sends beams of &+Gcol&+Bor spr&+Ray&n from $s hands!", FALSE, ch, 0,
      0, TO_VICTROOM);

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;
    if(should_area_hit(ch, tch))
    {
      ray_type = 1 << number(0, 3);
      spell_single_prismatic_ray(level, ch, (char *) &ray_type, 0, tch, 0);
    }
  }
}

void spell_prismatic_spray(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  P_char   tch, next;
  int      room, rays, ray_type;

  send_to_char
    ("You send a &+Rr&+Ba&+Yi&+Gn&+Cb&n&+co&+bw&n of prismatic spray from your hands.\n",
     ch);
  act
    ("$n sends a &+Rr&+Ba&+Yi&+Gn&+Cb&n&+co&+bw&n of prismatic spray from $s hands!",
     FALSE, ch, 0, 0, TO_VICTROOM);

  room = ch->in_room;

  for (tch = world[room].people; tch; tch = next)
  {
    next = tch->next_in_room;
    if(should_area_hit(ch, tch))
    {
      for (rays = number(1, 2); rays; rays--)
      {
        ray_type = 1 << number(0, RAY_COUNT - 1);
        spell_single_prismatic_ray(level, ch, (char *) &ray_type, 0, tch, 0);
        if(!is_char_in_room(tch, room) || (GET_STAT(tch) == STAT_DEAD))
          break;
      }
    }
  }

  zone_spellmessage(room,
    "&+CC&+co&+Cl&+co&+Cr&+cf&+Cu&+cl&N &+Crays of &+Wlight &+Cstreak throughout the sky!&n\n\r",
    "&+CC&+co&+Cl&+co&+Cr&+cf&+Cu&+cl&N &+Crays of &+Wlight &+Cstreak throughout the sky to the %s!&n\n\r");
}


#undef RAY_RED
#undef RAY_BLUE
#undef RAY_GREEN
#undef RAY_YELLOW
#undef RAY_ORANGE
#undef RAY_INDIGO
#undef RAY_AZURE
#undef RAY_VIOLET

void spell_anti_magic_ray(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int dam, temp, save;
  struct damage_messages messages = {
    "&+BReality &nseems to twist and bend as your &+Yray&n collides with $N's body!",
    "$n laughs maniacally as their &+Ldevastating &+Yray&n of pure &+Benergy &ncollides with your body!",
    "$n points at $N, and a powerful &+Yray&n of &+Lanti-&+Bmagic &nlances forth from their fingertip!",
    "Your &+Yray&n strips the very last bit of the magic called '&+CLife&n' from $N's body.",
    "You feel your very soul ceasing to be, as $n's &+Yray&n saps the last of the magic from your body.",
    "$N turns &+wpale&n, and suddenly collapses as $n's &+Yray&n saps the remainder of their lifeforce!", 0
  };
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;

  temp = MIN(46, (level + 1));
  dam = dice(5 * temp, 10);
  
  if(saves_spell(victim, SAVING_SPELL))
    dam >>= 1;

  // if vict doesn't die, hit with dispel magic 50% of the time

  if(spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages) ==
      DAM_NONEDEAD && (number(0, 1)))
      
  {
    save = victim->specials.apply_saving_throw[SAVING_SPELL];
    victim->specials.apply_saving_throw[SAVING_SPELL] += 15;
    spell_dispel_magic(level, ch, 0, SPELL_TYPE_SPELL, victim, obj);
    victim->specials.apply_saving_throw[SAVING_SPELL] = save;
  }
}


void spell_magic_missile(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "You watch with self-pride as the &+Ymagic missile&N hits $N.",
    "You stagger as a &+Ymagic missile&N from $n hits you.",
    "$n throws a &+Ymagic missile&N at $N, who staggers under the blow.",
    "The &+Ymagic missile&N tears away the remaining life of $N.",
    "You only have time to notice $n uttering strange sounds before everything is dark.",
    "The &+Ymagic missile&N sent by $n causes $N to stagger and collapse in a lifeless heap."
  };
  int dam;

  int num_missiles = BOUNDED(1, (level / 3), 5);
  dam = (dice(1, 4) * 4 + number(1, level));

  // play_sound(SOUND_MMISSILE, NULL, ch->in_room, TO_ROOM);

  while (num_missiles-- &&
         spell_damage(ch, victim, dam, SPLDAM_GENERIC,
                       SPLDAM_ALLGLOBES, &messages) == DAM_NONEDEAD) ;
}

void spell_chill_touch(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "You &+Bchill&N $N.",
    "You feel your life flowing away as $n &+Bchills&N you.",
    "$n &+Bchills&N $N who suddenly seems less lively.",
    "You &+Bchill&N $N.  Remember to put flowers on $S grave.",
    "You feel &+Bchilled&N by $n and then you feel no more - RIP.",
    "$n touches $N who slumps to the ground as a dead lump, rather chilly, isn't it?",
      0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  int dam = (dice(1, 6) + 5 * 4 + level) ;

  bool failed_save = !NewSaves(victim, SAVING_SPELL, level/7);

  if( failed_save )
    dam <<= 1;

  // wizlog(56,"chill touch damage = %d", dam);
  if(spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_ALLGLOBES, &messages) == DAM_NONEDEAD )
  {
    if( (victim) && (ch) && failed_save
      && !affected_by_spell(victim, SPELL_CHILL_TOUCH)
      && !IS_ELITE(victim) && !IS_GREATER_RACE(victim)
      && !IS_AFFECTED3(victim, AFF3_COLDSHIELD)
      && !IS_AFFECTED4(victim, AFF4_ICE_AURA) )
    {
      act("&+BThe chilling cold causes $N&+B to stammer, apparently weakened.&n",
        FALSE, ch, 0, victim, TO_CHAR);
      act("&+BThe cold goes right to the bone, you feel yourself weakening!&n",
        FALSE, ch, 0, victim, TO_VICT);
      act("&+B$N &+Bsags, apparently weakened from the frigid cold!&n",
        FALSE, ch, 0, victim, TO_NOTVICT);

      dam /= 4;
    //  wizlog(56,"chill touch stat damage = %d", dam);
      struct affected_type af;
      memset(&af, 0, sizeof(af));
      af.type = SPELL_CHILL_TOUCH;
      af.duration = 1;

      af.location = APPLY_STR;
      af.modifier = -(number(1, dam));
      affect_to_char(victim, &af);

      if (GET_CLASS(ch, CLASS_NECROMANCER) && IS_SPECIALIZED(ch))
      {
        af.location = APPLY_AGI;
        af.modifier = -(number(1, dam));
        affect_to_char(victim, &af);

        af.location = APPLY_DEX;
        af.modifier = -(number(1, dam));
        affect_to_char(victim, &af);
      }
    }
  }
}

void spell_frostbite(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct damage_messages messages = {
    "&+C$N&+C screams in agony as your biting cold freezes $S soul!",
    "&+CYou scream in agony as $n's&+C biting cold freezes your soul!",
    "&+C$N&+C screams in agony as $n's&+C biting cold freezes $S soul!",
    "&+CYour intense cold has claimed $N's life!",
    "&+C$n's&+C intense cold has claimed your life!",
    "&+C$n's&+C intense cold has claimed $N's&+C life!", 0
  };

  int dam = (int) (MIN(40, level) * 5) + number(1, 20);

  if(spell_damage(ch, victim, dam, SPLDAM_COLD, 0, &messages) != DAM_VICTDEAD )
  {
    if( !affected_by_spell(victim, SPELL_FROSTBITE) && !NewSaves(victim, SAVING_SPELL, 0) )
    {
      act("&+BThe chilling cold causes $N&+B to stammer, apparently weakened.&n", FALSE, ch, 0, victim, TO_CHAR);
      act("&+BThe cold goes right to the bone, you feel yourself weakening!&n", FALSE, ch, 0, victim, TO_VICT);
      act("&+B$N &+Bsags, apparently weakened from the frigid cold!&n", FALSE, ch, 0, victim, TO_NOTVICT);

      struct affected_type af;
      memset(&af, 0, sizeof(af));
      af.type = SPELL_FROSTBITE;
      af.duration =  1;

      af.modifier = -number(10,15);
      af.location = APPLY_STR;
      affect_to_char(victim, &af);
    }
  }
}

void spell_burning_hands(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "You &+Yburned&n $N.",
    "You cry out in pain as $n burns you.",
    "$N cries out as $n burns $M.",
    "You &+Yburned&n $N to death.",
    "You have been burned to death by $n.",
    "$n has burned $N to death.", 0
  };

  int num_dice = (level / 10);
  int dam = (dice((num_dice+5), 6) * 4);

  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_ALLGLOBES, &messages);
}

void spell_shocking_grasp(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "You get a good hold of the shocked $N.",
    "You get a shock as $n gets too close to you.",
    "$N looks shocked as $n grasps at $M.",
    "$N dies while looking rather shocked.",
    "$n shocks you.  Unfortunately, your heart can't stand it...",
    "$n reveals with a shock that $N has left $S body.", 0
  };

  int num_dice = (level / 6);
  int dam = (dice(num_dice+5, 6) * 4);

  if( !NewSaves(victim, SAVING_SPELL, 0) )
    dam = (int) (dam * 2);

  // play_sound(SOUND_SHOCKING_GRASP, NULL, ch->in_room, TO_ROOM);

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_ALLGLOBES,
               &messages);
}

void spell_lightning_bolt(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "The &=LBlightning bolt&N hits $N with full impact.",
    "YOU'RE HIT!  A &=LBlightning bolt&N from $n has reached its goal.",
    "$N wavers under the impact of the &=LBlightning bolt&N sent by $n.",
    "Your &=LBlightning bolt&N shatters $N to pieces.",
    "You feel enlightened by the &=LBlightning bolt&N $n sends, and then all is dark - RIP.",
    "$N receives the full &+yblast&N of a &=LBlightning bolt&+B from $n ... and is no more.",
      0
  };

  int num_dice = (level / 5);
  int dam = (dice(num_dice+5, 6) * 4);

  if(!NewSaves(victim, SAVING_SPELL, 0))
    dam = (int) (dam * 1.33);

  // play_sound(SOUND_SHOCKWAVE, NULL, ch->in_room, TO_ROOM);

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING,
               SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);
}

void spell_cone_of_cold(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "&+BYour blast of cold strikes $N &+Bdead on, who shudders from the pain.",
    "&+BA blast of cold sent by $n &+Bstrikes you dead on, freezing up your limbs! BRRR.",
    "$n &+Bfires a blast of cold at $N&+B, who screams out in pain!",
    "&+BYour blast of cold&N freezes $N &+Bsolid, who lifelessly falls to the ground.",
    "Your body turns to ice as $n &+Bfires a blast of cold at you, AARRGHGHHGH!!",
    "$n fires a &+Bblast of cold at $N&+B, who freezes solid and dies instantly!",
    0
  };

  int num_dice = (level / 4);
  int dam = (dice( num_dice+5, 6) * 4);

  // play_sound(SOUND_SPELL3, NULL, ch->in_room, TO_ROOM);

  if(spell_damage
      (ch, victim, dam, SPLDAM_COLD, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages) == DAM_NONEDEAD )
  {
    bool is_wet = IS_AFFECTED5(victim, AFF5_WET) ? TRUE : FALSE; 
    if(!affected_by_spell(victim, SPELL_CONE_OF_COLD) && !NewSaves(victim, SAVING_SPELL, is_wet ? 5 : 0))
    {
      int duration = (int) (WAIT_SEC * 1.5 * level / 50);
      int modifier = -number(5, 10);
      
      struct affected_type af;
      memset(&af, 0, sizeof(af));
      
      if(is_wet)
      {
        act("&+BThe chilling cold causes $N&+B to stammer, apparently weakened and slowed.&n", FALSE, ch, 0, victim, TO_CHAR);
        act("&+BThe cold goes right to the bone, you feel yourself weakening and slowing down!&n", FALSE, ch, 0, victim, TO_VICT);
        act("&+B$N &+Bsags, apparently weakened and slowed from the frigid cold!&n", FALSE, ch, 0, victim, TO_NOTVICT);                 
        duration += 2;
        modifier -= number(5, 10);
        af.bitvector2 = AFF2_SLOW;
      }
      else
      {
        act("&+BThe chilling cold causes $N&+B to stammer, apparently weakened.&n", FALSE, ch, 0, victim, TO_CHAR);
        act("&+BThe cold goes right to the bone, you feel yourself weakening!&n", FALSE, ch, 0, victim, TO_VICT);
        act("&+B$N &+Bsags, apparently weakened from the frigid cold!&n", FALSE, ch, 0, victim, TO_NOTVICT);
      }
      

      af.type = SPELL_CONE_OF_COLD;
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSAVE;
      af.duration = duration;
      af.location = APPLY_STR;
      af.modifier = modifier;

      affect_to_char(victim, &af);
    }
  }

}

void spell_invoke_negative_energy(int level, P_char ch, char *arg, int type,
                                  P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "&+LYou unleash the negative material powers on $N.",
    "&+L$n&+L unleashes the negative material powers on you.",
    "&+L$n&+L unleash the negative material powers on $N.",
    "&+L$N&+L howls in pain as $S essence is unmade!",
    "&+LYou howl as $n's&+L spell unmakes you!",
    "&+L$N&+L howls in pain as $n's&+L unmakes $m!"
  };

  dam = dice(5, 10) + GET_LEVEL(ch);
  spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_GLOBE, &messages);
}

void spell_restore_spirit(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj tar_obj)
{
  int result, dam;

  struct damage_messages messages = {
    "You restore your soul with the help of $N's soul.&n",
    "You feel less &+renergetic&n as $n uses your soul to restore his spirit.",
    "A &n&+Wholy &n&+Yglow&n around $n &n&+yre&n&+Ypl&n&+Weni&n&+Ysh&n&+yes&n their health and endurance.",
    "$N crumples as you &+Rkill&n $M by draining $S &+rspirit.&n",
    "&+LYour neurons fry as&n $n &+Rspirit seeps away!&n",
    "$n drains $N's spirit, draining $M of every last bit of $S &+rspirit!&n"
  };

  bool saved = FALSE;

  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim) ||
     victim == ch)
  {
    return;
  }

  if(IS_ANGEL(victim))
  {
    send_to_char("&+wThat wouldn't be very nice, they are angelic.\r\n", ch);
    return;
  }

  if(resists_spell(ch, victim))
  {
    return;
  }

  dam = (int) ((level * 2.5) + number(-10, 10));
  
  if(IS_PC(ch) &&
    !(GET_CLASS(ch, CLASS_THEURGIST | CLASS_PALADIN)))
  {
    send_to_char("&+rLacking the proper training in divinity, you do not utilize the full potential of the spell!\r\n", ch);
    dam = (int)(dam*0.80);
  }
  
  if(IS_AFFECTED4(victim, AFF4_DEFLECT))
  {
    if(GET_LEVEL(ch) >= 50)
    {
      dam <<= 1;
    }
    
    spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &messages);
    return;
  }

  if(saves_spell(victim, SAVING_SPELL))
  {
    saved = TRUE;
    dam >>= 1;
  }
  
  if(GET_LEVEL(ch) >= 50)
  {
    dam <<= 1;
  }
  
  vamp(ch, (int)(dam / 4), (int) (GET_MAX_HIT(ch) * (double)(BOUNDED(110, ((GET_C_POW(ch) * 10) / 9), 220) * .01)));

  if(GET_VITALITY(victim) >= 10 &&
     !IS_AFFECTED2(victim, AFF2_SOULSHIELD))
  {
    GET_VITALITY(victim) = MAX(10, GET_VITALITY(victim) - 5);
    GET_VITALITY(ch) += 10;
  }

  StartRegen(ch, EVENT_MOVE_REGEN);
  StartRegen(victim, EVENT_MOVE_REGEN);
      
  result = spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_NOSHRUG,
                 &messages);

  if (result == DAM_NONEDEAD && !saved)
  {
      if (IS_AFFECTED2(victim, AFF2_SOULSHIELD))
      {
        send_to_char("&+LYour soulshield protects you from lasting effects from the restore spell!&n\r\n", victim);
        send_to_char("&+LYour victim is too well protected against divine power - no lingering effects of the restore spell will hold...&n\r\n", ch);
        return;
      }
      struct affected_type af;
      memset(&af, 0, sizeof(af));
      af.type = SPELL_RESTORE_SPIRIT;

      af.duration = number(GET_LEVEL(ch)/4, saved ? (GET_LEVEL(ch) / 2) : GET_LEVEL(ch)) * PULSE_VIOLENCE;
      af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;

      af.location = APPLY_MOVE;
      af.modifier = -(MIN(saved ? 7 : 15, GET_VITALITY(victim)));

      if (affected_by_spell(victim, SPELL_RESTORE_SPIRIT))
      {
        send_to_char("&+LThey're already affected by a restore spell - you only prolong and enhance the buzz!", ch);
        send_to_char("&+LYour buzz is enhanced as another restore spell hits you!", victim);
        affect_join(victim, &af, FALSE, FALSE);
        return;
      affect_to_char_with_messages(victim, &af, "&+LYour life energy was drained, leaving you a bit shaken.", "&+LYou manage to shake off negative effects of the restore spell.");
      }
  }
}

void spell_enervation(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int result, dam;

  struct damage_messages messages = {
    "You enervate $N, &+Ldraining&n $M of some of $S &+renergy.&n",
    "You feel less &+renergetic&n as $n enervates you.",
    "$n enervates $N, leaving $M &+yvisibly shaken!&n",
    "$N crumples as you &+Rkill&n $M by draining $S &+renergy.&n",
    "&+LYour neurons fry as&n $n &+Renervates &+Lthem and drains you drains your last bit of energy!&n",
    "$n enervates $N, draining $M of every last bit of $S &+renergy!&n"
  };

  bool saved = FALSE;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || victim == ch)
  {
    return;
  }

  if(IS_UNDEADRACE(victim))
  {
    send_to_char("&+LEnervating the undead is impossible.\r\n", ch);
    return;
  }

  // Shrug
  if( resists_spell(ch, victim) )
  {
    return;
  }

  dam = (int) ((level * 2.75) + number(-10, 10));

  if( IS_PC(ch) && !(GET_CLASS(ch, CLASS_NECROMANCER | CLASS_ANTIPALADIN)) )
  {
    send_to_char("&+rLacking the proper training in necromancy, you do not utilize the full potential of the spell!\r\n", ch);
    dam = (int)(dam*0.80);
  }

  if(IS_AFFECTED4(victim, AFF4_DEFLECT))
  {
    if(GET_LEVEL(ch) >= 50)
    {
      dam <<= 1;
    }
    spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);
    return;
  }

  // Made it harder to save against.
  if(NewSaves(victim, SAVING_SPELL, level/7))
  {
    saved = TRUE;
    dam >>= 1;
  }

  if(GET_LEVEL(ch) >= 50)
  {
    dam <<= 1;
  }

  vamp(ch, (int)(dam / 4), (int) (GET_MAX_HIT(ch) * (double)(BOUNDED(110, ((GET_C_POW(ch) * 10) / 9), 220) * .01)));

  if( GET_VITALITY(victim) >= 10 && !IS_AFFECTED4(victim, AFF4_NEG_SHIELD) )
  {
    GET_VITALITY(victim) = MAX(10, GET_VITALITY(victim) - 5);
    GET_VITALITY(ch) += 10;
  }

  StartRegen(ch, EVENT_MOVE_REGEN);
  StartRegen(victim, EVENT_MOVE_REGEN);

  result = spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG, &messages);

  if (result == DAM_NONEDEAD && !saved)
  {
      if (IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
      {
        send_to_char("&+LYour negative energy shield protects you from lasting effects from the enervation spell!&n\r\n", victim);
        send_to_char("&+LYour victim is too well protected against necromancy - no lingering effects of the enervation spell will hold...&n\r\n", ch);
        return;
      }
      struct affected_type af;
      memset(&af, 0, sizeof(af));
      af.type = SPELL_ENERVATION;

      af.duration = number(GET_LEVEL(ch)/4, saved ? (GET_LEVEL(ch) / 2) : GET_LEVEL(ch)) * PULSE_VIOLENCE;
      af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;

      af.location = APPLY_MOVE;
      af.modifier = saved ? -7 : -15;
      // Do not put them below 1 max movement point: the prompt does not like that.
      if( af.modifier < -GET_MAX_VITALITY(victim) + 1 )
        af.modifier = -GET_MAX_VITALITY(victim) + 1;

      if (affected_by_spell(victim, SPELL_ENERVATION))
      {
        send_to_char("&+LThey're already affected by enervation - you only prolong and enhance the suffering!\n\r", ch);
        send_to_char("&+LYour suffering is enhanced as another enervation spell hits you!\n\r", victim);
        affect_join(victim, &af, FALSE, FALSE);
        return;
      }

      affect_to_char_with_messages(victim, &af, "&+LYour life energy was drained, leaving you a bit shaken.", "&+LYou manage to shake off the negative effects of the enervation spell.");
  }
}

void spell_life_leech(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int mana, dam, result, moves;
  bool saved  = FALSE;
  
  struct damage_messages messages = {
    "&+LYou reach out and touch $N, &+rleeching &+Lsome of $S's &+Llife&+wfor&+Wce.&n",
    "&+LYour &+rlife &+force seems to slip away as&n $n &+Ltouches you.&n",
    "$n &+Lseems to suck the &+rlife &+Lright out of&n $N!",
    "$N &+rapparently has no more &+Rlife &+rto leech!&n",
    "&+LAs&n $n &+Ltouches you, you feel the last bit of your &+rlife &+Lseep out of you.&n",
    "$n &+Lsucks the last bit of &+rlife &+Lout of &n$N &+Lwho falls to the &+yground &+Llifeless."
  };

  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim) ||
     victim == ch)
  {
    return;
  }

  if(resists_spell(ch, victim))
    return;

  dam = dice(1.5 * level, 5);
  
  
  if(IS_AFFECTED4(victim, AFF4_DEFLECT))
  {
    spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NOVAMP | SPLDAM_NODEFLECT, &messages);
    return;
  }

  if(NewSaves(victim, SAVING_SPELL, 0))
  {
    saved = TRUE;
    dam = (int)(dam * 0.80);
  }
  
  if(GET_LEVEL(victim) <= (level / 10))
  {
    /*
     * Kill the sucker
     */
    act(messages.death_attacker, FALSE, ch, 0, victim, TO_CHAR);
    act(messages.death_victim, FALSE, ch, 0, victim, TO_VICT);
    act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
    die(victim, ch);
    victim = NULL;
  }
  else
  {
    if(!IS_AFFECTED4(victim, AFF4_NEG_SHIELD) &&
       !IS_UNDEADRACE(victim))
    {
      if(IS_PC(ch) || 
         IS_PC_PET(ch))
      {
        vamp(ch, (int)(dam / 5), (int) (GET_MAX_HIT(ch) * 1.00));
      }
      else
        vamp(ch, (int)(dam / 2), (int) (GET_MAX_HIT(ch) * 1.00));
	 
    }

    StartRegen(ch, EVENT_MOVE_REGEN);
    StartRegen(victim, EVENT_MOVE_REGEN);
      
// Vamping still occurs as above. We do not want double vamping undead. 
    spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG | SPLDAM_NOVAMP, &messages);
  }

  if(!IS_ALIVE(victim) ||
     !IS_ALIVE(ch))
        return;

}





void spell_energy_drain(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int mana, dam, result, moves;
  bool saved  = FALSE;

  struct damage_messages messages = {
    "You drain $N of some of $S &+Wenergy.&n",
    "&+LYou feel less energetic as&n $n &+Ldrains you.&n",
    "$n &+rdrains&n $N - what a waste of energy!",
    "$N &+rcrumples as you kill&n $M by draining $S energy.",
    "&+LAs&n $n &+Ldrains your last bit of energy, you look forward to the peace of the graveyard.&n",
    "$n &+Ldrains the &+Wenergy&n of $N who crumbles into a lifeless husk."
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || victim == ch)
  {
    return;
  }

  if(resists_spell(ch, victim))
    return;

  dam = dice(3 * level, 5);

  if( GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER) || GET_SPEC(ch, CLASS_THEURGIST, SPEC_THAUMATURGE) )
  {
    level = (int)(level * 1.25);
  }

  if(IS_AFFECTED4(victim, AFF4_DEFLECT))
  {
    spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NOVAMP | SPLDAM_NODEFLECT, &messages);
    return;
  }

  if( NewSaves(victim, SAVING_SPELL, 0) )
  {
    saved = TRUE;
    dam = (int)(dam * 0.80);
  }

  if( GET_LEVEL(victim) <= (level / 10) )
  {
    /*
     * Kill the sucker
     */
    act(messages.death_attacker, FALSE, ch, 0, victim, TO_CHAR);
    act(messages.death_victim, FALSE, ch, 0, victim, TO_VICT);
    act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
    die(victim, ch);
    victim = NULL;
  }
  else
  {
    if( !IS_AFFECTED4(victim, AFF4_NEG_SHIELD) && !IS_UNDEADRACE(victim) )
    {
      if( IS_PC(ch) || IS_PC_PET(ch) )
      {
        vamp(ch, (int)(dam / 5), (int) (GET_MAX_HIT(ch) * (double)(BOUNDED(110, ((GET_C_POW(ch) * 10) / 9), 220) * .01)));
      }
      else
      {
        vamp(ch, (int)(dam / 2), (int) (GET_MAX_HIT(ch) * (double)(BOUNDED(110, ((GET_C_POW(ch) * 10) / 9), 220) * .01)));
      }
    }


    if(GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER))
    {
      send_to_char("&+LYour life energy is &+rtapped&+L.\n", victim);
      if( GET_VITALITY(victim) >= 25 && !IS_AFFECTED4(victim, AFF4_NEG_SHIELD) )
      {
        moves = number(5, 30); //old value (5, level)

        if(affected_by_spell(victim, SPELL_ENERGY_DRAIN))
        {
          moves /= 2;
        }

        GET_VITALITY(victim) = MAX(1, (GET_VITALITY(victim) - moves));
        GET_VITALITY(ch) += moves;
        debug("E DRAIN: (%s&n) loses and (%s&n) gained (%d) moves.", J_NAME(victim), J_NAME(ch), moves);
      }
      StartRegen(ch, EVENT_MOVE_REGEN);
      StartRegen(victim, EVENT_MOVE_REGEN);
    }

    // Vamping still occurs as above. We do not want double vamping undead.
    spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG | SPLDAM_NOVAMP, &messages);
  }

  if( !IS_ALIVE(victim) || !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_AFFECTED4(victim, AFF4_NEG_SHIELD) )
  {
    send_to_char("&+LYour negative energy shield protects you from lasting effects from the energy drain!&n\r\n", victim);
    send_to_char("&+LYour victim is too well protected against necromancy - no lingering effects of the energy drain will hold.&n\r\n", ch);
    return;
  }

  if(affected_by_spell(victim, SPELL_ENERGY_DRAIN))
  {
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_ENERGY_DRAIN)
      {
        af1->duration += 1;
      }
    }
  }
  else if( !saved )
  {
    //send_to_char("&+LYour spell saps the energy from your foe, leaving you invigorated in return.\r\n", ch);
    //send_to_char("&+LYour energy is sapped! You feel sluggish and weak...\r\n", victim);
    struct affected_type af;
    memset(&af, 0, sizeof(af));
    af.type = SPELL_ENERGY_DRAIN;
    af.flags = AFFTYPE_NODISPEL | AFFTYPE_SHORT;
    af.duration = (level / 10) * WAIT_SEC;

    af.location = APPLY_MOVE_REG;
    af.modifier = -(level / 20);
    affect_to_char(victim, &af);

    af.location = APPLY_HIT_REG;
    af.modifier = -(level / 20);
    affect_to_char(victim, &af);
  }
  send_to_char("&+LYour life energy was drained, leaving you a bit shaken.\r\n", victim);
}

// Old edrain below. -Lucrot Jul09
// Shows exactly why Lucrot shouldn't have been touching code... seriously, nice commenting.
/*
 * Drain XP, MANA, HP - caster gains HP and MANA
 */
// void spell_energy_drain(int level, P_char ch, char *arg, int type,
                        // P_char victim, P_obj obj)
// {
  // int xp, mana, dam;
  // struct damage_messages messages = {
    // "You drain $N of some of $S energy.",
    // "You feel less energetic as $n drains you.",
    // "$n drains $N - what a waste of energy!",
    // "$N crumples as you kill $M by draining $S energy.",
    // "As $n drains your last bit of energy, you look forward to the peace of the graveyard.",
    // "$n drains the energy of $N who crumbles into a lifeless husk."
  // };

  // if(!(ch) ||
     // !(victim) ||
     // !IS_ALIVE(ch) ||
     // !IS_ALIVE(victim) ||
     // victim == ch)
  // {
    // return;
  // }

  // if(resists_spell(ch, victim))
  // {
    // return;
  // }

//  GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch) - 2);

  // dam = (int) ((level * 2.5) + number(-10, 10));
  
  // if(IS_PC(ch) &&
    // !(GET_CLASS(ch, CLASS_NECROMANCER | CLASS_ANTIPALADIN)))
  // {
    // dam /= 4;
  // }
  
  // if(IS_AFFECTED4(victim, AFF4_DEFLECT))
  // {
    // if(GET_LEVEL(ch) >= 50)
    // {
      // dam <<= 1;
    // }
    
    // spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);
    // return;
  // }

  // if(saves_spell(victim, SAVING_SPELL))
  // {
    // dam /= 2;
  // }
  
//  GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch) - 4);
  // if(GET_LEVEL(victim) <= 2)
  // {
    // /*
     // * Kill the sucker
     // */
    // act(messages.death_attacker, FALSE, ch, 0, victim, TO_CHAR);
    // act(messages.death_victim, FALSE, ch, 0, victim, TO_VICT);
    // act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
    // die(victim, ch);
    // victim = NULL;
  // }
  // else
  // {
    // xp = GET_LEVEL(ch) * 1000;
    // xp = MIN(xp, GET_EXP(victim));
    
    // if(GET_LEVEL(ch) >= 50)
    // {
      // dam <<= 1;
    // }
  
    // if(!IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
    // {
      // mana = MIN(GET_MANA(victim), 100);
      // GET_MANA(victim) -= mana;
      // StartRegen(victim, EVENT_MANA_REGEN);
      // GET_MANA(ch) += mana >> 1;
    // }

    // if(IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
    // {
      // vamp(ch, (int)(dam / 6), (int) (GET_MAX_HIT(ch) * 1.25));
    // }
    // else
    // {
      // vamp(ch, (int)(dam / 2), (int) (GET_MAX_HIT(ch) * 1.25));
    // }

    // if(GET_LEVEL(ch) <= 50)
    // {
      // send_to_char("&+LYour life energy is drained!\n",
        // victim);
    // }
    // else
    // {
      // send_to_char("&+LYour life energy is &+rtapped&+L.\n",
        // victim);
    // }

    // if(GET_VITALITY(victim) >= 10 &&
      // !IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
    // {
      // GET_VITALITY(victim) = MAX(10, GET_VITALITY(victim) - 15);
      // GET_VITALITY(ch) += 10;
    // }

    // StartRegen(ch, EVENT_MOVE_REGEN);
    // StartRegen(victim, EVENT_MOVE_REGEN);
      
    // spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG,
                 // &messages);
  // }
// }


void spell_wither(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int percent, dam;
  struct affected_type af;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( victim == ch )
  {
    send_to_char("You may not wither yourself.", ch);
    return;
  }

  if( IS_CONSTRUCT(victim) )
  {
    act("&+LBeing an artificial construct, $E &+Lis &+Wimmune&+L to your spell!", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if( IS_UNDEADRACE(victim) )
  {
    if( !number(0, 1) && resists_spell(ch,victim) )
    {
      send_to_char("Your victim resisted your &+Lwither&n attempt!\r\n", ch);
      return;
    }

    struct damage_messages wither_undead = {
      "$N &+Lwavers under the power of your will, as $S &+Lundead flesh withers away!",
      "$n's &+Lpure will causes your undead flesh to wither away!",
      "$N &+Lwavers under the power of&n $n's &+Lwill, as $S &+Lundead flesh withers away!",
      "$N &+Lwithers away and is completely destroyed!",
      "$n's &+Lorders your flesh to wither away - and it obeys $m! &+LYou return into the cold sleep of death...",
      "$N&+L is completely withered away by&n $n's &+Lraw will!", 0
    };

    dam = dice(MIN(level, 46), 10);
    spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &wither_undead);
    return;
  }

 /* if(resists_spell(ch,victim))
    return;*/

  percent = victim->specials.apply_saving_throw[SAVING_SPELL] * number(1, 5);

  percent += (int) (GET_C_POW(ch) - GET_C_POW(victim));
  percent += (int) (GET_LEVEL(ch) - GET_LEVEL(victim));

  if( NewSaves(victim, SAVING_FEAR, 0) )
  {
    percent = (3 * percent) / 4;
  }

  percent = BOUNDED(0, percent, 100);

  if( IS_TRUSTED(victim) || percent < 1 )
  {
    send_to_char("They are too powerful to wither this way!\n", ch);
    return;
  }

  if( affected_by_spell(victim, SPELL_WITHER) )
  {
    send_to_char("If you withered them any more, they'd be some sort of filthy prune.\n", ch);
    return;
  }

  if( affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT) )
  {
    act("$E is already a &+ywithered prune.&n Enfeebling $M is not possible.", TRUE, ch, 0, victim, TO_CHAR);
    act("&+LLuckily for you, you cannot possibly get more feeble!&n", TRUE, ch, 0, victim, TO_VICT);
    return;
  }

  if( percent > 0 && (IS_AFFECTED4(victim, AFF4_NEG_SHIELD) || IS_AFFECTED2(victim, AFF2_SOULSHIELD))
    || IS_UNDEADRACE(victim) || IS_GREATER_RACE(victim) )
  {
    percent = (int)(percent * 0.75);
  }

  bzero(&af, sizeof(af));

  if( percent > 90 )
  {
    act("$N is &+Lwithered&n COMPLETELY by $n's touch!", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n &+Lwithers&n you with his touch. You feel your muscles COMPLETELY shrink!", FALSE, ch, 0, victim, TO_VICT);
    act("$N is &+Lwithered&n COMPLETELY by your touch!", FALSE, ch, 0, victim, TO_CHAR);
    af.type = SPELL_WITHER;
    af.duration = 1;
    af.modifier = 0 - (int) (GET_HITROLL(victim) / 4);
    af.location = APPLY_HITROLL;
    af.bitvector2 = AFF2_SLOW;
    affect_to_char(victim, &af);
    af.modifier = 0 - (int) (GET_DAMROLL(victim) / 4);
    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);
    af.modifier = +100;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
  }
  else if( percent > 70 )
  {
    act("$N is &+Lwithered&n and starts to FADE with $n's touch!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n &+Lwithers&n you with his touch. You feel your body FADE!", FALSE, ch, 0, victim, TO_VICT);
    act("$N is &+Lwithered&n and starts to FADE by your touch!", FALSE, ch, 0, victim, TO_CHAR);
    af.type = SPELL_WITHER;
    af.duration = 1;
    af.modifier = 0 - (int) (GET_HITROLL(victim) / 4);
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);
    af.modifier = 0 - (int) (GET_DAMROLL(victim) / 4);
    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);
    af.modifier = +50;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
  }
  else if( percent > 40 )
  {
    act("$N is &+Lwithered&n and starts to DIMINISH with $n's touch!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n &+Lwithers&n you with his touch. You feel your body DIMINISH!", FALSE, ch, 0, victim, TO_VICT);
    act("$N is &+Lwithered&n and starts to DIMINISH by your touch!", FALSE, ch, 0, victim, TO_CHAR);
    af.type = SPELL_WITHER;
    af.duration = 1;
    af.modifier = 0 - (int) (GET_HITROLL(victim) / 5);
    af.location = APPLY_HITROLL;
    //af.bitvector2 = AFF2_SLOW;
    affect_to_char(victim, &af);
    af.modifier = 0 - (int) (GET_DAMROLL(victim) / 5);
    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);
    af.modifier = +25;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
  }
  else if( percent > 10 )
  {
    act("$N is &+Lwithered&n and starts to decrease in size with $n's touch!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n &+Lwithers&n you with his touch. You begin to DECREASE in size!", FALSE, ch, 0, victim, TO_VICT);
    act("$N is &+Lwithered&n and begins to DECREASE in size by your touch!", FALSE, ch, 0, victim, TO_CHAR);
    af.type = SPELL_WITHER;
    af.duration = 1;
    af.modifier = 0 - (int) (GET_HITROLL(victim) / 8);
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);
    af.modifier = 0 - (int) (GET_DAMROLL(victim) / 8);
    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);
    af.modifier = +20;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
  }
  else
  {
    act("$N &+Lseems unaffected by your attempt to wither.", FALSE, ch, 0, victim, TO_CHAR);
  }
}

P_char make_mirror (P_char);
void spell_mirror_image(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   image, tmpch, j;

/*  int numbp = 0, newslot; */
/*
   P_char oldchnext, oldtchnext, prev = NULL, prevtch = NULL, tch, prevch = NULL;
 */
  int      numb, i, c, c2, placement;
  struct follow_type *k;

  if( IS_NPC(ch) && (victim = GET_MASTER(ch)))
  {
    act("$n slaps you savagely across the face, and then kicks you in your pubic area.", FALSE, victim, 0, ch, TO_VICT);
    send_to_char( "You feel lame now, don't you?  You should.\n\r", victim );
    return;
  }

  if(IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    send_to_char("Ain't enough room here to do that, bubba.\n", ch);
    return;
  }
  for (k = ch->followers; k; k = k->next)
  {
    victim = k->follower;
    if(IS_NPC(victim) && GET_RNUM(victim) == real_mobile(250))
    {
      send_to_char("You can only have one set of mirror images at a time.\n",ch);
      return;
    }
  }

  for (tmpch = world[ch->in_room].people; tmpch; tmpch = tmpch->next_in_room)
  {
    if(IS_NPC(tmpch) && (GET_RNUM(tmpch) == real_mobile(250)) &&
        (GET_RACEWAR(tmpch) == GET_RACEWAR(ch)))
    {
      send_to_char("The area is fairly cluttered as it is.\n", ch);
      return;
    }
  }

  numb = BOUNDED( 1, level / 6, 4 );

  for (i = 0; i < numb; i++)
  {
    if(!(image = make_mirror(ch)))
    {
      send_to_char("Your mirror ain't imaging tonight bubba.  let a god know.\n", ch);
      return;
    }
    // reset our variables
    c = 0;
    c2 = 0;
    placement = 0;
    // count people in room for random placement
    for( j = world[ch->in_room].people; j; j = j->next_in_room )
    {
      c++;
    }
    placement = number(0, c);

    // If we are placing it after the first person
    if( placement > 0 )
    {
      for( j = world[ch->in_room].people; j; j = j->next_in_room )
      {
        c2++;
        if( c2 == placement )
        {
          image->next_in_room = j->next_in_room;
          j->next_in_room = image;
          break;
        }
      }
    }
    else   // Otherwise, we are placing at position 1, which is the beginning of the room
    {
      image->next_in_room = world[ch->in_room].people;
      world[ch->in_room].people = image;
    }
    image->in_room = ch->in_room;

    act("A spitting image of $n suddenly rises from the ground!", TRUE,ch, 0, image, TO_NOTVICT);
    send_to_char("A spitting image of you suddenly rises from the ground!\n",ch);
    add_follower(image, ch);
  }
}

// Utility function...  This is not a spell.  -- Dalreth
P_char make_mirror( P_char ch )
{
  char     Gbuf1[512];
  P_char image = NULL;

  image = read_mobile(real_mobile(250), REAL);
  if(!image)
  {
    return image;
  }

  image->specials.act |= ACT_SPEC_DIE;

  int duration = setup_pet(image, ch, 30, PET_NOCASH);
  /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
  if(duration >= 0)
  {
    duration += number(1,10);
    add_event(event_pet_death, (duration+1) * 60 * 4, image, NULL, NULL, 0, NULL, 0);
  }

  /* string it */
  image->only.npc->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);
  sprintf(Gbuf1, "image %s %s", GET_NAME(ch),race_names_table[GET_RACE(ch)].normal);
  image->player.name = str_dup(Gbuf1);
  image->player.short_descr = str_dup(ch->player.name);

  sprintf(Gbuf1, "%s stands here.\n", ch->player.name);
  image->player.long_descr = str_dup(Gbuf1);

  if( GET_TITLE(ch) )
  {
    image->player.title = str_dup(GET_TITLE(ch));
  }

  GET_RACE(image) = GET_RACE(ch);
  GET_RACEWAR(image) = GET_RACEWAR(ch);
  GET_SEX(image) = GET_SEX(ch);
  GET_ALIGNMENT(image) = GET_ALIGNMENT(ch);
  GET_SIZE(image) = GET_SIZE(ch);
  // Make them ugly!
  GET_C_CHA(image) = 1;

  return image;
}

bool can_conjure_lesser_elem(P_char ch, int level)
{
  struct follow_type *k;
  P_char   victim;
  int i, j;

  for (k = ch->followers, i = 0, j = 0; k; k = k->next)
  {
    victim = k->follower;
/*    
    if(IS_ELEMENTAL(victim))
    {
      if(!IS_GREATER_ELEMENTAL(victim))
      {
        i++;
      }
      else
      {
        j++;
      }
    }*/
    if(IS_ELEMENTAL(victim) || IS_GREATER_ELEMENTAL(victim))
    i++;
  }
/*
  if(GET_LEVEL(ch) >= 56)
  {
    j--;
    i--;
  }
  
  if(GET_LEVEL(ch) >= 60)
  {
    j--;
    i--;
  }
  
  if(GET_C_CHA(ch) >= 200)
  {
    send_to_char("Your ability to inspire is amazing.\r\n", ch);
    j--;
    i--;
  }

  if(j && i >= 2)
    return FALSE;
*/
  if(i >= 3)
    return FALSE;

  if(GET_LEVEL(ch) >= 41 && i >= 3)
    return FALSE;
  if((GET_LEVEL(ch) >= 31) && (GET_LEVEL(ch) < 41) && i >= 2)
    return FALSE;
  if((GET_LEVEL(ch) >= 21) && (GET_LEVEL(ch) < 31) && i >= 1)
    return FALSE;
  if(GET_LEVEL(ch) < 21)
    return FALSE;

  return TRUE;
}

int can_call_woodland_beings(P_char ch, int level)
{
  struct char_link_data *cld;
  int    pets, allowed;

  for (cld = ch->linked, pets = 0; cld; cld = cld->next_linked)
    if(cld->type == LNK_PET)
      pets++;

  switch (world[ch->in_room].sector_type)
  {
    case SECT_CITY:
    case SECT_DESERT:
    case SECT_ROAD:
      allowed = 1;
      break;
    case SECT_FIELD:
    case SECT_HILLS:
    case SECT_MOUNTAIN:
    case SECT_SWAMP:
    case SECT_UNDRWLD_WILD:
    case SECT_UNDRWLD_MOUNTAIN:
    case SECT_UNDRWLD_SLIME:
      allowed = 2;
      break;
    case SECT_FOREST:
    case SECT_SNOWY_FOREST:
    case SECT_UNDRWLD_MUSHROOM:
      allowed = 3;
      break;
    default:
      return FALSE;
  }

  if(GET_LEVEL(ch) >= 51)
    allowed++;

  return pets < allowed;
}

void spell_call_woodland_beings(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char        mob;
  int   sum, mlvl, lvl;

  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {1144, "$n &+ywanders in from the wilderness&n."},
    {1145, "$n &+cflies in, trailed by &+Wsparkles&n."},
    {1146, "$n &+ywanders in from the wilderness&n."},
    {1147, "$n &+ywanders in from the wilderness&n."},
    {1148, "$n &+ywanders in from the wilderness&n."},
    {1149, "$n &+ywanders in from the wilderness&n."},
    {1150, "$n &+ywanders in from the wilderness&n."},
    {1151, "$n &+ywanders in from the wilderness&n."},
    {1152, "$n &+ywanders in from the wilderness&n."},
    {1051, "$n &+ywanders in from the wilderness&n."},
    {1052, "$n &+ywanders in from the wilderness&n."},
    {1053, "$n &+ywanders in from the wilderness&n."},
    {1054, "$n &+ywanders in from the wilderness&n."},
    {1055, "$n &+ywanders in from the wilderness&n."},
    {1056, "$n &+ywanders in from the wilderness&n."},
    {1057, "$n &+ywanders in from the wilderness&n."},
    {1058, "$n &+ywanders in from the wilderness&n."},
    {20, "$n &+ywanders in from the wilderness&n."},
    {21, "$n &+ywanders in from the wilderness&n."},
    {25, "$n &+ywanders in from the wilderness&n."},
    {26, "$n &+ywanders in from the wilderness&n."},
    {28, "$n &+ywanders in from the wilderness&n."},
    {400, "$n &+ywanders in from the wilderness&n."},
    {401, "$n &+ywanders in from the wilderness&n."},
    {402, "$n &+ywanders in from the wilderness&n."},
    {403, "$n &+ywanders in from the wilderness&n."},
    {404, "$n &+ywanders in from the wilderness&n."},
    {405, "$n &+ywanders in from the wilderness&n."},
    {406, "$n &+ywanders in from the wilderness&n."},
    {407, "$n &+ywanders in from the wilderness&n."},
    {408, "$n &+ywanders in from the wilderness&n."},
    {409, "$n &+ywanders in from the wilderness&n."},
    {410, "$n &+ywanders in from the wilderness&n."},
    {411, "$n &+ywanders in from the wilderness&n."},
    {412, "$n &+ywanders in from the wilderness&n."},
    {418, "$n &+ywanders in from the wilderness&n."},
    {419, "$n &+ywanders in from the wilderness&n."}
  };

  if(!can_call_woodland_beings(ch, level))
  {
    send_to_char("No more woodland beings will come to your aid!\n", ch);
    return;
  }

  sum = number(0, 36);

  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);
  if(!mob)
  {
    logit(LOG_DEBUG, "spell_call_woodland(): mob %d not loadable",
        summons[sum].mob_number);
    send_to_char("Bug in call woodland beings.  Tell a god!\n", ch);
    return;
  }
  GET_SIZE(mob) = SIZE_MEDIUM;
  mob->player.m_class = CLASS_WARRIOR;

  char_to_room(mob, ch->in_room, 0);
  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);
  justice_witness(ch, NULL, CRIME_SUMMON);

  mlvl = (level / 5) * 2;
  lvl = number(mlvl, mlvl * 3);

  mob->player.level = BOUNDED(10, lvl, 42);

  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob) / 2, 8) + GET_LEVEL(mob);

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);

  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 3;
  MonkSetSpecialDie(mob);       /* 2d6 to 4d5 */
  mob->points.damsizedice = (int)(0.5 * mob->points.damsizedice);

  act("$N acts all friendly around $n!'", TRUE, ch, 0,
      mob, TO_ROOM);
  act("$N acts all friendly around you!'", TRUE, ch, 0,
      mob, TO_CHAR);
  int duration = setup_pet(mob, ch, 100 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
  add_follower(mob, ch);

}


void event_elemental_swarm_death(P_char ch, P_char victim, P_obj obj, void *data)
{
  act("$n &+rdisappears as &+Lquickly&+r as it came, fading back to its home plane!", TRUE, ch, 0, 0, TO_ROOM);
  extract_char(ch);

}

void spell_elemental_swarm(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  int      lvl;

  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your conjuring!\n", ch);
    return;
  }

  mob = read_mobile( number(69, 72), VIRTUAL );
  if(!mob)
  {
    logit(LOG_DEBUG, "spell_conjure_elemental(): mob(s) not loadable");
    send_to_char("Bug in conjure elemental.  Tell a god!\n", ch);
    return;
  }
  GET_SIZE(mob) = SIZE_MEDIUM;
  //mob->player.m_class = 0;

  char_to_room(mob, ch->in_room, 0);
  act("$n &+Lgrunts, and comes forth to join the swarm!!.", TRUE, mob, 0, 0, TO_ROOM);
  justice_witness(ch, NULL, CRIME_SUMMON);

  lvl = number(level - 3, level + 3);
  mob->player.level = BOUNDED(1, lvl, 51);

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
  SET_BIT(mob->specials.act, ACT_SPEC_DIE);
  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob), 7) + (GET_LEVEL(mob) * 2);
  GET_EXP(mob) = 0;
  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) + 10;
  MonkSetSpecialDie(mob);       /* 2d6 to 4d5 */
  apply_achievement(mob, TAG_CONJURED_PET);

  if(!can_conjure_lesser_elem(ch, level))
  {
    act("$N &+Lis NOT pleased at being suddenly summoned with this many &+Celementals&+L in the room!&n",
      TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+Lis NOT pleased with you summon $S with this many &+Celementals&+L in the room!",
      TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
    return;
  }
  else
  {
    // play_sound(SOUND_ELEMENTAL, NULL, ch->in_room, TO_ROOM);
    int duration = setup_pet(mob, ch, 1, PET_NOCASH | PET_NOORDER | PET_NOAGGRO);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if(duration >= 0)
    {
      duration = number(5,30) * WAIT_SEC;
      add_event(event_elemental_swarm_death, duration, mob, NULL, NULL, 0, NULL, 0);
    }
  }

  if(victim)
  {
    MobStartFight(mob, victim);
  }
  group_add_member(ch, mob);
}

void spell_conjour_elemental(int level, P_char ch, char *arg, int type,
                             P_char victim, P_obj obj)
{
  P_char   mob;
  int      life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
  int      charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
  int      sum, mlvl, lvl, duration, room = ch->in_room;
  int      good_terrain = 0;
  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {
    1100, "$n &+Rarrives in a burst of fire."},
    {
    1101, "$n &+yforms from beneath your feet."},
    {
    1102, "&+CA gust of wind solidifies into&n $n."},
    {
    1103, "$n &+Bforms from a puddle in front of you."}
  };

  if( !IS_ALIVE(ch) || !(room) )
  {
    return;
  }

  if( IS_PC_PET( ch ) )
  {
    send_to_char( "Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET) );
    return;
  }

  if( CHAR_IN_SAFE_ZONE(ch) )
  {
    send_to_char("A mysterious force blocks your conjuring!\n", ch);
    return;
  }

  if( !can_conjure_lesser_elem(ch, level) )
  {
    send_to_char("You cannot control any more elementals!\n", ch);
    return;
  }

  if( GET_RACE(ch) == RACE_PLICH )
  {
    return;
  }

  if(IS_SPECIALIZED(ch))
  {
    switch (ch->player.spec)
    {
    case 1:
      sum = 2;
      break;
    case 2:
      sum = 3;
      break;
    case 3:
      sum = 0;
      break;
    case 4:
      sum = 1;
      break;
    default:
      debug( "Invalid spec (%d) on char '%s'.", ch->player.spec, J_NAME(ch) );
      return;
      break;
    }
  }
  else
  {
    sum = number(0, 3);
  }

  if(IS_SPECIALIZED(ch) && GET_CLASS(ch, CLASS_SUMMONER) && (IS_PC(ch) || IS_PC_PET(ch)))
  {
   send_to_char("Specialized &+Rsummoners&n use the &+cconjure&n command to manage their minions.\r\n", ch);
   return;
  }

  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);

  if( !mob )
  {
    logit( LOG_DEBUG, "spell_conjure_elemental(): mob %d not loadable", summons[sum].mob_number );
    send_to_char( "Bug in conjure elemental.  Tell a god!\n", ch );
    return;
  }

  GET_SIZE(mob) = SIZE_MEDIUM;
  mob->player.m_class = CLASS_WARRIOR;

  char_to_room(mob, room, 0);
  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);
  justice_witness(ch, NULL, CRIME_SUMMON);

  // Reworking level code for conj pets.
  /*
  mlvl = (level / 5) * 2;
  lvl = number(mlvl, mlvl * 3);

  mob->player.level = BOUNDED(10, lvl, 45);
  */

  if (number(1, 100) < 20)
    lvl = level + number(2, 5);
  else
    lvl = level - number(-1, 5);

  mob->player.level = BOUNDED(10, lvl, 45);

  MonkSetSpecialDie(mob);

  if(!IS_SET(mob->specials.affected_by, AFF_INFRAVISION))
  {
    SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
  }

  apply_achievement(mob, TAG_CONJURED_PET);

  good_terrain = conjure_terrain_check(ch, mob);

  if(good_terrain == 1)
  {
    if(IS_SET(mob->specials.affected_by2, AFF2_SLOW))
    {
      REMOVE_BIT(mob->specials.affected_by2, AFF2_SLOW);
    }

    if(!IS_SET(mob->specials.affected_by, AFF_HASTE))
    {
      SET_BIT(mob->specials.affected_by, AFF_HASTE);
    }

    mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
    mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 2;

    mob->base_stats.Str = 100;
    mob->base_stats.Dex = 100;
    mob->base_stats.Agi = 100;
    mob->base_stats.Pow = 100;

    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      (int) (dice(GET_LEVEL(mob) / 2, 12) + 6 * GET_LEVEL(mob) + life + charisma);

    if(GET_C_CHA(ch) > number(0, 400))
    {
      GET_SIZE(mob) = SIZE_LARGE;
      mob->player.spec = 2; // Guardian spec
    }
  }
  else
  {
    mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
    mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 3;
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      dice(GET_LEVEL(mob) / 2, 10) + 3 * GET_LEVEL(mob) + life + charisma;
    mob->points.damsizedice = (int)(0.8 * mob->points.damsizedice);
  }

  if( IS_PC(ch) && GET_LEVEL(mob) > GET_LEVEL(ch)
    && charisma < number(10, (int) (get_property("summon.lesser.elemental.charisma", 140.000)))
    && !has_air_staff_arti(ch) && GET_LEVEL(ch) < 50 )
  {
    act("$N is NOT pleased at being suddenly summoned against $S will!",
        TRUE, ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
  }
  else
  {                             /* Under control */
    act("$N sulkily says 'Your wish is my command, $n!'", TRUE, ch, 0,
        mob, TO_ROOM);
    act("$N sulkily says 'Your wish is my command, master!'", TRUE, ch, 0,
        mob, TO_CHAR);
    // play_sound(SOUND_ELEMENTAL, NULL, ch->in_room, TO_ROOM);
    duration = setup_pet(mob, ch, 400 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if(duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

void event_living_stone_death(P_char ch, P_char victim, P_obj obj, void *data)
{
  act("$n &+rdisappears as &+Lquickly&+r as it came, fading into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
  extract_char(ch);
}

void spell_living_stone(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  int      lvl;

  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }
  
  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your conjuring!\n", ch);
    return;
  }

  mob = read_mobile(real_mobile(1104), REAL);
  if(!mob)
  {
    logit(LOG_DEBUG, "spell_living_stone(): mob 1104 not loadable");
    send_to_char("Bug in spell_living_stone.  Tell a god!\n", ch);
    return;
  }
  GET_SIZE(mob) = SIZE_MEDIUM;
  mob->player.m_class = 0;

  char_to_room(mob, ch->in_room, 0);
  act("$n &+Lcomes to life!.", TRUE, mob, 0, 0, TO_ROOM);
  justice_witness(ch, NULL, CRIME_SUMMON);

  lvl = number(level - 3, level + 3);

//  GET_LEVEL(mob) = BOUNDED(10, lvl, 45);
  mob->player.level = BOUNDED(1, lvl, 56);

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
  SET_BIT(mob->specials.act, ACT_SPEC_DIE);
  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob), 3) + (GET_LEVEL(mob) * 2);
  GET_EXP(mob) = 0;
  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) + 10;
  MonkSetSpecialDie(mob);       /* 2d6 to 4d5 */

  if(!can_conjure_lesser_elem(ch, level))
  {
    act("$N &+Lis NOT pleased at being suddenly summoned with this many &+Rstones&+L in the room!&n",
       TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+Lis NOT pleased with you summon $S with this many &+Rstone&+L in the room!",
       TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
    return;
  }
  else
  {
    // play_sound(SOUND_ELEMENTAL, NULL, ch->in_room, TO_ROOM);
    int duration = setup_pet(mob, ch, 1, PET_NOCASH | PET_NOORDER | PET_NOAGGRO);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if(duration >= 0)
    {
      duration = number(5,10) * WAIT_SEC;
      add_event(event_living_stone_death, duration, mob, NULL, NULL, 0, NULL, 0);
    }
  }

  if(victim)
    MobStartFight(mob, victim);
  group_add_member(ch, mob);
}

void spell_greater_living_stone(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  P_char   mob;
  int      lvl;

  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }
  
  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your conjuring!\n", ch);
    return;
  }

  mob = read_mobile(real_mobile(1105), REAL);
  if(!mob)
  {
    logit(LOG_DEBUG, "spell_greater_living_stone(): mob 1105 not loadable");
    send_to_char("Bug in spell_greater_living_stone.  Tell a god!\n", ch);
    return;
  }
  GET_SIZE(mob) = SIZE_MEDIUM;
  mob->player.m_class = 0;

  char_to_room(mob, ch->in_room, 0);
  act("$n &+Lcomes to life!.", TRUE, mob, 0, 0, TO_ROOM);
  justice_witness(ch, NULL, CRIME_SUMMON);

  lvl = number(level - 3, level + 3);

  mob->player.level = BOUNDED(1, lvl, 56);

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
  SET_BIT(mob->specials.act, ACT_SPEC_DIE);

  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob), 3) + (GET_LEVEL(mob) * 2);

  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) + 15;
  mob->points.damnodice = 15;
  mob->points.damsizedice = 14;

//  play_sound(SOUND_ELEMENTAL, NULL, ch->in_room, TO_ROOM);
  int duration = setup_pet(mob, ch, 1, PET_NOCASH | PET_NOORDER | PET_NOAGGRO);
  add_follower(mob, ch);
  /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
  if(duration >= 0)
  {
    duration = number(7,12) * WAIT_SEC;
    add_event(event_living_stone_death, duration, mob, NULL, NULL, 0, NULL, 0);
  }

  if(victim)
    MobStartFight(mob, victim);

  group_add_member(ch, mob);
}

bool can_conjure_greater_elem(P_char ch, int level)
{
  int j = 0;
  struct follow_type *k;
  P_char   victim;

  for (k = ch->followers, j = 0; k; k = k->next)
  {
    victim = k->follower;
      if(IS_ELEMENTAL(victim) || IS_GREATER_ELEMENTAL(victim))
      j++;
  }
/*
  if(GET_LEVEL(ch) >= 56)
    j--;

  if(GET_LEVEL(ch) >= 60)
    j--;

  if(GET_C_CHA(ch) >= 200)
  {
    send_to_char("Your ability to inspire is amazing.\r\n", ch);
    j--;
  }


  if(GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR) &&
     has_air_staff_arti(ch) &&
     j <= 2)
  {
    return true;
  }
  else if(IS_SPECIALIZED(ch) &&
          GET_CLASS(ch, CLASS_CONJURER) &&
          j <= 1)
  {
    return true;
  }
  else if(j > 0)
  {
    return FALSE;
  }

  return TRUE;
*/
  if(j >= 3)
    return FALSE;

  if(GET_LEVEL(ch) >= 41 && j >= 3)
    return FALSE;
  if((GET_LEVEL(ch) >= 31) && (GET_LEVEL(ch) < 41) && j >= 2)
    return FALSE;
  if((GET_LEVEL(ch) >= 21) && (GET_LEVEL(ch) < 31) && j >= 1)
    return FALSE;
  if(GET_LEVEL(ch) < 21)
    return FALSE;

  return TRUE;
}

/* The conjure_terrain_check() function checks if it is a specialized conjurer  */
/* trying to conjure on terrain appropriate for the elemental type, and handles */
/* appropriate messages. Bonuses/maluses themselves are attributed in the       */
/* conjour_elemental() and conjure_specialized() functions.                     */
/* Return values from this function are:                                        */
/* -1 - bad terrain for the elemental type                                      */
/* 0 - neutral terrain                                                          */ 
/* 1 - good terrain for the conjurer elemental type                             */

int conjure_terrain_check(P_char ch, P_char mob)
{
  int room = ch->in_room;

  if (!ch || !mob || !IS_ALIVE(ch))
    return 0;

  if (!GET_CLASS(ch, CLASS_CONJURER))
	  return 0;

  if (!ch->player.spec)
	  return 0;

  switch (ch->player.spec)
  {
	case 1: /* AIR conjurer*/

	if (world[room].sector_type == SECT_AIR_PLANE)
	{
	  act("$N &+Labsorbs vast quantities of &+Cair &+Lfrom the surrounding area!",
          TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+Labsorbs vast quantities of &+Cair &+Lfrom the surrounding area!",
          TRUE, ch, 0, mob, TO_CHAR);
	  return 1;
	}
	else if (world[room].sector_type == SECT_EARTH_PLANE)
	{
	  act("$N &+Lfeebly tries to draw &+Cair &+Lbut there is so little...",
          TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+Lfeebly tries to draw &+Cair &+Lbut there is so little...",
          TRUE, ch, 0, mob, TO_CHAR);
      return -1;
	}
	else
  {
	  return 0;
  }
	break;

	case 2: /* WATER conjurer*/
    if( IS_FIRE(room) || world[room].sector_type == SECT_FIREPLANE
      || world[room].sector_type == SECT_UNDRWLD_LIQMITH
      || world[room].sector_type == SECT_LAVA )
    {
	  act("$N &+rfeebly tries to draw &+Bwater &+Lbut there is so little...",
          TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+rfeebly tries to draw &+Bwater &+Lbut there is so little...",
          TRUE, ch, 0, mob, TO_CHAR);
      return -1;
	}
    else if (IS_WATER_ROOM(room) || world[room].sector_type == SECT_OCEAN)
	{
	  act("$N &+Labsorbs vast quantities of &+Bwater &+Lfrom the surrounding area!",
          TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+Labsorbs vast quantities of &+Bwater &+Lfrom the surrounding area!",
          TRUE, ch, 0, mob, TO_CHAR);
	  return 1;
	}
    else
      return 0;

	break;

	case 3: /* FIRE conjurer*/

	  if (IS_FIRE(room) || world[room].sector_type == SECT_FIREPLANE
      || world[room].sector_type == SECT_UNDRWLD_LIQMITH
      || world[room].sector_type == SECT_LAVA )
	{
	  act("$N &+Labsorbs vast quantities of &+Rfire &+Lfrom the surrounding area!",
          TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+Labsorbs vast quantities of &+Rfire &+Lfrom the surrounding area!",
          TRUE, ch, 0, mob, TO_CHAR);
      return 1;
	}
    else if (IS_WATER_ROOM(room) || world[room].sector_type == SECT_OCEAN)
	{
	  act("$N &+bfeebly tries to draw &+Rfire &+bbut there is so little...",
          TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+bfeebly tries to draw &+Rfire &+bbut there is so little...",
          TRUE, ch, 0, mob, TO_CHAR);
      return -1;
	}
    else
      return 0;

    break;

	case 4: /* EARTH conjurer*/
		
	if (world[room].sector_type == SECT_EARTH_PLANE)
	{
	  act("$N &+Labsorbs vast quantities of &+yearth &+Lfrom the surrounding area!",
          TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+Labsorbs vast quantities of &+yearth &+Lfrom the surrounding area!",
          TRUE, ch, 0, mob, TO_CHAR);
	  return 1;
	}
	else if (world[room].sector_type == SECT_AIR_PLANE)
	{
	  act("$N &+Lfeebly tries to draw &+yearth &+Lbut there is so little...",
          TRUE, ch, 0, mob, TO_ROOM);
    act("$N &+Lfeebly tries to draw &+yearth &+Lbut there is so little...",
          TRUE, ch, 0, mob, TO_CHAR);
	  return -1;
	}
	else		
	  return 0;

	break;
  }

  return 0;
}

void conjure_specialized(P_char ch, int level)
{
  P_char   mob;
  int      summoned, room;
  int      life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
  int      charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
  int      good_terrain = 0;
  char    *summons[] = {
    "&+CA HUGE gust of wind solidifies into&n $n.",
    "$n &+Bforms from a nearby lake in front of you.",
    "$n &+Rarrives in a HUGE burst of flames!",
    "$n &+yforms from a HUGE chunk of earth!"
  };
  static struct
  {
    int      vnum;
    int      hits;
    int      damroll;
  } pets[] = // If you add or remove mobs, make sure to adjust 
              // IS_GREATER_ELEMENTAL define in utils.h -Lucrot
  {
    {
    1130, 500, 20},
    {
    1131, 400, 20},
    {
    1132, 600, 20},             // AIR 
    {
    1140, 600, 25},
    {
    1141, 600, 20},
    {
    1142, 600, 20},             // WATER 
    {
    1110, 600, 20},
    {
    1111, 700, 25},
    {
    1112, 550, 20},             // FIRE 
    {
    1120, 600, 20},
    {
    1121, 800, 30},
    {
    1122, 700, 25},             // EARTH 
/* These are reg pets.
    {
    43, 500, 20},
    {
    43, 400, 20},
    {
    43, 600, 20},             // AIR
    {
    44, 600, 25},
    {
    44, 600, 20},
    {
    44, 600, 20},             // WATER
    {
    41, 600, 20},
    {
    41, 700, 25},
    {
    41, 650, 20},             // FIRE
    {
    42, 600, 20},
    {
    42, 800, 30},
    {
    42, 700, 25},             // EARTH
*/

  };

  summoned = 3 * (ch->player.spec - 1) + number(0, 2);
  mob = read_mobile(real_mobile(pets[summoned].vnum), REAL);
  if( ch )
  {
    room = ch->in_room;
  }

  if( !IS_ALIVE(ch) || !(mob) || !(room) )
  {
    logit(LOG_DEBUG, "conjure_specialized(): mob %d not loadable", pets[summoned].vnum);
    if( ch )
    {
      send_to_char("Bug in conjour greater elemental.  Tell a god!\n", ch);
    }
    // Don't waste memory.
    if( mob )
    {
      extract_char( mob );
    }
    return;
  }

  char_to_room(mob, room, 0);
  act(summons[ch->player.spec - 1], TRUE, mob, 0, 0, TO_ROOM);

  if(!IS_SET(mob->specials.affected_by, AFF_INFRAVISION))
  {
    SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
  }

  apply_achievement(mob, TAG_CONJURED_PET);

  if(GET_LEVEL(ch) > 55)
  {
    mob->player.level = (ubyte)number(51,55);
  }
  else
  {
    mob->player.level = (ubyte)number(49,53);
  }

  //whew, big bonus for high level mobs!
  if(mob->player.level > 53)
  {
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    pets[summoned].hits * 2 + number(0, 50) + (life * 3) + charisma;
  }
  else if(mob->player.level > 49)
  {
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      pets[summoned].hits + number(0, 50) + (life * 3) + charisma;
  }
  else
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      310 + number(0,50) + (life * 3) + charisma;

  GET_MAX_HIT(mob) = GET_HIT(mob) = (int) (GET_MAX_HIT(mob) * .66);

  mob->points.base_hitroll = mob->points.hitroll =
    pets[summoned].damroll + number(0, 5);
  mob->points.base_damroll = mob->points.damroll =
    pets[summoned].damroll + number(0, 5);
  MonkSetSpecialDie(mob);
  mob->points.damsizedice = (int)(0.8 * mob->points.damsizedice);

  good_terrain = conjure_terrain_check(ch, mob);

  if(good_terrain == -1)
  {
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      (int) (50 + number(1, 100) + (life * 2) + (charisma));
    GET_SIZE(mob) = SIZE_MEDIUM;
  }
  else if(good_terrain == 1)
  {

    if(IS_SET(mob->specials.affected_by2, AFF2_SLOW))
    {
      REMOVE_BIT(mob->specials.affected_by2, AFF2_SLOW);
    }

    if(!IS_SET(mob->specials.affected_by, AFF_HASTE))
    {
      SET_BIT(mob->specials.affected_by, AFF_HASTE);
    }

    mob->points.base_hitroll = mob->points.hitroll =
      pets[summoned].damroll + number(20, 30);
    mob->points.base_damroll = mob->points.damroll =
      pets[summoned].damroll + number(20, 30);
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      (int) (GET_LEVEL(ch) * 30 + number(1, 100) + (life * 4) + (charisma * 2));
    GET_SIZE(mob) = SIZE_HUGE;
    mob->base_stats.Str = 100;
    mob->base_stats.Dex = 100;
    mob->base_stats.Agi = 100;
    mob->base_stats.Pow = 100;

    if( !IS_MULTICLASS_NPC(mob) && !IS_SPECIALIZED(mob) && GET_CLASS(mob, CLASS_WARRIOR))
    {
      if(number(0, 3))
      {
        mob->player.spec = 2; // Guardian
      }
      else if(number(0, 3))
      {
        mob->player.spec = 1; // Swordsman
      }
      else
      {
        mob->player.spec = 3; // Swashbuckler
      }
    }
  }

  if( IS_PC(ch) && !IS_TRUSTED(ch) && !(has_air_staff_arti(ch))
    && charisma < number(10, (int) (get_property("summon.greater.elemental.charisma", 140.000))) )
  {
    act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE,
        ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
  }
  else
  {
    act("$N says 'I shall serve you for a short time $n!'", TRUE, ch, 0, mob,
        TO_ROOM);
    act("$N says 'I shall serve you for a short time!'", TRUE, ch, 0, mob,
        TO_CHAR);

    int duration = setup_pet(mob, ch, 400 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if(duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

void spell_conjour_greater_elemental(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  int      sum, duration, room;
  int      life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
  int      charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {
    41, "$n &+Rarrives in a HUGE burst of flames!"},
    {
    42, "$n &+yforms from a HUGE chunk of earth!"},
    {
    43, "&+CA HUGE gust of wind solidifies into&n $n."},
    {
    44, "$n &+Bforms from a nearby lake in front of you."}
  };

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_PC_PET( ch ) )
  {
    send_to_char( "Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET) );
    return;
  }

  room = ch->in_room;

  if( !(room) || CHAR_IN_SAFE_ZONE(ch) )
  {
    send_to_char("A mysterious force blocks your conjuring!\n", ch);
    return;
  }

  if( !can_conjure_greater_elem(ch, level) )
  {
    send_to_char("You may not control more HUGE elementals!\n", ch);
    return;
  }

  if( IS_SPECIALIZED(ch) && GET_CLASS(ch, CLASS_SUMMONER) && (IS_PC(ch) || IS_PC_PET(ch)) )
  {
   send_to_char("Specialized &+Rsummoners&n use the &+cconjure&n command to manage their minions.\r\n", ch);
   return;
  }

  if( GET_CLASS(ch, CLASS_CONJURER) && IS_SPECIALIZED(ch) )
  {
    conjure_specialized(ch, level);
    return;
  }

  sum = number(0, 3);

  if(has_air_staff_arti(ch))
  {
    sum = 2;
  }

  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);

  if(!mob)
  {
    logit(LOG_DEBUG, "spell_conjure_greater_elemental(): mob %d not loadable",
          summons[sum].mob_number);
    send_to_char("Bug in conjure greater elemental.  Tell a god!\n", ch);
    return;
  }

  GET_SIZE(mob) = SIZE_LARGE;
  char_to_room(mob, room, 0);
  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);

  mob->player.level = number(49, 53);
  if(mob->player.level == 49)
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = 450 + number(0,50) + (life * 3) + charisma;
  else if(mob->player.level == 50)
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = 500 + number(0,50) + (life * 3) + charisma;
  else if(mob->player.level == 51)
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = 550 + number(0,50) + (life * 3) + charisma;
  else if(mob->player.level == 52)
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      600 + number(0,50) + (life * 3) + charisma;
  else
          //big bonus for highest level pet, since it's rare
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      700 + number(0,50) + (life * 3) + charisma;

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);

  apply_achievement(mob, TAG_CONJURED_PET);

  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 4;

  MonkSetSpecialDie(mob);       /* 2d6 to 4d5 */
  mob->points.damsizedice = (int)(0.8 * mob->points.damsizedice);

  if( IS_PC(ch) && !has_air_staff_arti(ch) && !IS_TRUSTED(ch)
    && (charisma + number(0, GET_LEVEL(ch)) < number(10, (int) (get_property("summon.greater.elemental.charisma", 140.000)))) )
  {
    act("$N is NOT pleased at being suddenly summoned against $S will!",
      TRUE, ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
  }
  else
  {                             /* Under control */
    act("$N says 'I shall serve you for a short time $n!'",
      TRUE, ch, 0, mob, TO_ROOM);
    act("$N says 'I shall serve you for a short time!'",
      TRUE, ch, 0, mob, TO_CHAR);

    duration = setup_pet(mob, ch, 400 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if(duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}


void spell_summon_greater_demon(int level, P_char ch, P_char victim,
                                P_obj obj)
{
  P_char   mob;
  P_obj    weapon;
  int      sum, mlvl, lvl;
  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {
    30, "$n &+rappears in a shower of blood!"},
    {
    31, "$n &+Rbreaks through from beneath the ground!"},
    {
    32, "&+RA huge fireball falls from the sky, forming into &N$n"}
  };

  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your summoning!\n", ch);
    return;
  }
  if(!can_conjure_greater_elem(ch, level))
  {
    send_to_char("You cannot control any more Demons!\n", ch);
    return;
  }
  sum = number(0, 2);

  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);
  if(!mob)
  {
    logit(LOG_DEBUG, "spell_summon_greater_demon(): mob %d not loadable",
          summons[sum].mob_number);
    send_to_char("Bug in summon greater demon.  Tell a god!\n", ch);
    return;
  }
  GET_SIZE(mob) = SIZE_LARGE;
  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);
  mob->points.base_mana = 1000;
  mob->points.mana = 1000;
  SET_BIT(mob->specials.act, ACT_SENTINEL);
  SET_BIT(mob->specials.act, ACT_MEMORY);
  if(IS_SET(mob->specials.act, ACT_IGNORE))
  {
    REMOVE_BIT(mob->specials.act, ACT_IGNORE);
  }

  mlvl = (level / 4) * 2;

  lvl = MIN(50, number(mlvl, mlvl * 3));

  mob->player.level = BOUNDED(10, lvl, 50);

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);

  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 2;
  MonkSetSpecialDie(mob);

  char_to_room(mob, ch->in_room, 0);

  if(IS_PC(ch) &&              /*(GET_LEVEL(mob) > number((level - i * 4), level * 3 / 2)) */
      (weapon && (weapon->R_num != real_object(67207)))
      && !number(0, 300) && !IS_TRUSTED(ch))
  {
    act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE,
        ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
  }
  else
  {                             /* Under control */
    act
      ("$N says 'I shall serve you for a short time $n, and then I shall have you!'",
       TRUE, ch, 0, mob, TO_ROOM);
    act
      ("$N says 'I shall serve you for a short time, before taking your soul!'",
       TRUE, ch, 0, mob, TO_CHAR);

    int duration = setup_pet(mob, ch, 30, PET_NOCASH);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if(duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

void spell_earthen_maul(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      dam, temp, dam_flag;
  struct damage_messages messages = {
    0,
    0,
    0,
    "Your &+yearthen fist&N leaves only a battered corpse of $N behind!",
    "The &+yearthen fist&N drives the last remnants of life from you!",
    "$n's &+yearthen fist&N leaves only a battered corpse of $N behind!"
  };

  temp = MIN(20, (level / 2 + 1));
  dam = dice(5 * temp, 9);

  if(level > 50)
  {
    dam = dice(6 * temp, 9);
  }

  if(NewSaves(victim, SAVING_SPELL, 1.5))
    dam >>= 1;

  dam_flag = 0;

  /*
   * special nasty for casting this underground or in buildings, damage
   * is MUCH higher (from falling objects) and not limited to opponents
   * only! (In other words, extremely dangerous to cast when not
   * outside).
   */

  /* default dam_flag is 0, so don't bother checking for those cases */

  switch (world[ch->in_room].sector_type)
  {
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_NO_GROUND:
  case SECT_UNDERWATER:
  case SECT_FIREPLANE:
  case SECT_OCEAN:
    dam_flag = 0;
    break;                      /*
                                 * what earth to move?
                                 */
  case SECT_CITY:
  case SECT_FIELD:
  case SECT_FOREST:
  case SECT_HILLS:
  case SECT_UNDERWATER_GR:
  case SECT_ROAD:
  case SECT_DESERT:
  case SECT_ARCTIC:
  case SECT_SNOWY_FOREST:
    dam_flag = 1;
    break;                      /*
                                 * normal damage
                                 */
  case SECT_MOUNTAIN:
  case SECT_UNDRWLD_WILD:
  case SECT_UNDRWLD_CITY:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_UNDRWLD_SLIME:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_UNDRWLD_LIQMITH:
  case SECT_UNDRWLD_MUSHROOM:
    dam_flag = 2;               /*
                                 * dangerous, and slightly higher dam from
                                 * landslides
                                 */
    break;
  case SECT_INSIDE:
  case SECT_UNDRWLD_INSIDE:
    dam_flag = 3;
    break;                      /* dangerous, and added damage from falling debris
                                 */
  case SECT_LAVA:
    dam_flag = 4;               // Molten lava -> fire damage and more hurt.
    break;
  }


  if((dam_flag == 1) && !OUTSIDE(ch))
    dam_flag = 3;

  if(ch->specials.z_cord != 0)
    dam_flag = 0;

  switch (dam_flag)
  {
  case 0:
    send_to_char("No earth to move here, try a different spell.\n", ch);
    return;
    break;
  case 1:
    messages.attacker =
      "&+yYou cause the &+yEARTH to reach up and maul your opponent!";
    messages.victim = messages.room =
      "$n causes the &=LyEARTH&n to rise up in the shape of a fist!";
    break;
  case 2:
    messages.attacker =
      "&+yYou cause the &+yEARTH to reach up and maul your opponent!\n"
      "&+yThe unstable nature of your surroundings, causes extreme amounts of extra debris!";
    messages.victim = messages.room =
      "$n causes the &=LyEARTH&n to rise up in the shape of a fist!";
    break;
  case 3:
    messages.attacker =
      "&+yYou cause the &+yEARTH to reach up and maul your opponent!!\n"
      "&+yAs the ceiling begins to buckle under the stress, you realize this may not have been a good idea!";
    messages.victim = messages.room =
      "$n causes the &=LyEARTH&n to rise up in the shape of a fist!";
    break;
  case 4:
    messages.attacker =
      "&+yYou cause the &+rmolten EARTH&+y to reach up and maul your opponent!";
    messages.victim = messages.room =
      "&+y$n&+y causes the &+rmolten &=RyEARTH&n&+y to rise up in the shape of a fist!&n";
    messages.death_attacker = "&+yYour &+rmolten fist&+y leaves only a battered corpse of $N&+y behind!&n";
    messages.death_victim = "&+yThe &+rmolten fist&+y drives the last remnants of life from you!&n";
    messages.death_room = "&+y$n&+y's &+rmolten fist&+y leaves only a battered corpse of $N&+y behind!&n";
    break;
  default:
    send_to_char("As you are about to utter the last syllable, you choke it off,\n"
      "realizing you could well bury yourself alive!\n", ch);
    return;
    break;
  }

  dam = (int) (dam + 0.1*dam*dam_flag);

 // play_sound(SOUND_EARTHQUAKE1, NULL, ch->in_room, TO_ROOM);
  if(spell_damage(ch, victim, dam, (dam_flag==4) ? SPLDAM_FIRE : SPLDAM_GENERIC, 0, &messages) != DAM_NONEDEAD)
    return;

  /*
     if(number(0, 3) || (GET_CHAR_SKILL(victim, SKILL_SAFE_FALL) >= number(1, 101))) {
     act("$n almost dodges the &+yearth&n's maul, staying on $s feet!", TRUE, victim, 0, 0, TO_ROOM);
     act("You almost dodge the &+yearth&n's maul, staying on your feet!", TRUE, victim, 0, 0, TO_CHAR);
     if(IS_PC(victim)) notch_skill(victim, SKILL_SAFE_FALL, 3);
     } else {
     act("You are almost swallowed by the earth and injure yourself!",
     FALSE, ch, 0, victim, TO_VICT);
     act("$n crashes to the ground!", TRUE, victim, 0, 0, TO_ROOM);
     SET_POS(victim, number(0, 2) + GET_STAT(victim));
     if(GET_POS(victim) == POS_PRONE)
     Stun(victim, ch, PULSE_VIOLENCE * 2, TRUE);
     CharWait(victim, PULSE_VIOLENCE);
     play_sound(SOUND_EARTHQUAKE2, NULL, ch->in_room, TO_ROOM);
     }
   */
}


void spell_bigbys_clenched_fist(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "Your giant &+Yfist of force&N causes $N to stagger in agony!",
    "$n's mighty &+Ymagic fist&N slams into you!",
    "$n's &+Yfist&N beats the life out of $N, &+rblood&N pours from $S body!",
    "Your &+Ymighty fist&N smashes into $N's body, killing $M instantly!",
    "$n's giant &+Yfist of force&N is the last thing you ever see.",
    "$n's &+Ymagic fist&N punches $N into so much pulp!", 0
  };

  int dam = 10 * level + number(1, 25);

  if(!NewSaves(victim, SAVING_SPELL, 0))
    dam = (int) (dam * 2.0);

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_channel_negative_energy(int level, P_char ch, char *arg, int type,
                                   P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "&+rYou unleash a torrent of &n&+braw negative energy&+r on $N&n&+r, who begins to &+Bdissolve!",
    "&+r$n&n&+r unleashes a torrent of &n&+braw negative energy&+r on you, you begin to &+Bdissolve!",
    "&+r$n&n&+r unleashes a torrent of &n&+braw negative energy&+r on $N&n&+r, who begins to &+Bdissolve!",
    "&+L$N's&+L body completely &+REXPLODES&+L upon contact with too much &n&+bnegative energy!",
    "&+LYour body completely &+REXPLODES&+L upon contact with too much &n&+bnegative energy!",
    "&+L$N's&+L body completely &+REXPLODES&+L upon contact with too much &n&+bnegative energy!"
  };
  int      dam;

  if(saves_spell(victim, SAVING_SPELL))
    dam = 150;
  else
    dam = 300;

  spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, RAWDAM_NOKILL, &messages);
}

void spell_bigbys_crushing_hand(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "$N is grabbed by your giant fist, which begins &+Ycrushing&n $M.",
    "A huge fist sent by $n grabs you, and begins &+Ycrushing&n your body.",
    "$N is encircled by a huge fist sent by $n, which begins to &+Ycrush&n $S body.",
    "Your fist &+Ycrushes&n $N&n, leaving nothing but a smear of &+rblood&n on the ground.",
    "A huge fist &+Ycrushes&n you completely, leaving nothing but a wet smear.",
    "$N's head &+Yexplodes&n from the pressure of a huge fist sent by $n, and bits of $M &+Gooze&n between its fingers.",
      0
  };

  int dam = 12 * level + number(1, 50);

  if(!NewSaves(victim, SAVING_SPELL, 0))
    dam = (int) (dam * 1.75);

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

int spell_solbeeps_single_missile(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj tar_obj)
{
  struct damage_messages fulldam_messages = {
    "A &+Yhuge&n missile of &+Wforce&n departs from your fingertips, making a loud &+Lthud&n as it hits $N.",
    "$n's &+Yhuge&n missile of &+Wforce&n impacts on your chest with a loud &+Lthud&n, causing you to reel in pain.",
    "A &+Yhuge&n bolt of &+Wforce&n sent by $n impacts on $N's chest with a loud &+Lthud&n.",
    "Your &+Wforce&n missile slams into $N, leaving nothing but a bloody mess.",
    "$n's grin is the last thing you see before their &+Wforce&n missile bashes you into a pulpy mess.",
    "$n grins as their &+Wforce&n missile bashes $N into a pulpy mess.",
    0
  };

  struct damage_messages halfdam_messages = {
    "Your missile of &+Wforce&n hits $N soundly.",
    "$n's &+Wforce&n missile slams into you.",
    "A bolt of &+Wforce&n sent by $n hits $N soundly.",
    "Your &+Wforce&n missile slams into $N, leaving nothing but a bloody mess.",
    "$n's grin is the last thing you see before their &+Wforce&n missile bashes you into a pulpy mess.",
    "$n grins as their &+Wforce&n missile bashes $N into a pulpy mess.",
    0
  };

  // Return true for next missile if available.
  // Return false to stop barrage.
  if(!(ch) &&
     !(victim) &&
     !IS_ALIVE(ch) &&
     !IS_ALIVE(victim)) // Just making sure.
        return false;
        
  if(resists_spell(ch, victim))
    return true;

  int dam = dice (30, 11); // average ~45 per missile, so from 135+ at 51 to 180 at 55 plus 25% chance of 225 at 56
                           // made damage level-independent, since average number of missiles grows with level

  bool saved = true;
  if(!NewSaves(victim, SAVING_SPELL, 0))
  {
    dam = (int) (dam * 1.5);
    saved = false;
  }

  if(spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG, saved ? &halfdam_messages : &fulldam_messages) == DAM_NONEDEAD)
  {
   /*
    if(!saved && GET_SIZE(victim) < SIZE_HUGE &&
      !StatSave(victim, APPLY_AGI, -2 * (SIZE_LARGE - GET_SIZE(victim))) &&
      !IS_AFFECTED4(victim, AFF4_DEFLECT))
    {
      act("$N goes flying and crashes into the wall!", FALSE, ch, 0,
          victim, TO_CHAR);
      act("You are sent flying and crash into the wall!", FALSE, ch, 0,
          victim, TO_VICT);
      act("$N goes flying and crashes into the wall!", FALSE, ch, 0,
          victim, TO_NOTVICT);
      SET_POS(victim, POS_PRONE + GET_STAT(victim));
      
      stop_fighting(victim);
      if( IS_DESTROYING(victim) )
        stop_destroying(victim);
      CharWait(victim, PULSE_VIOLENCE * 1);


        int door = number(0, 9);

        if((CAN_GO(victim, door)) && (!check_wall(victim->in_room, door)))
        {
          act("$N goes flying out of the room!", FALSE, ch, 0,
            victim, TO_CHAR);
          act("You go flying out of the room!", FALSE, ch, 0,
            victim, TO_VICT);
          act("$N goes flying out of the room!", FALSE, ch, 0,
            victim, TO_NOTVICT);
          int target_room = world[victim->in_room].dir_option[door]->to_room;
          char_from_room(victim);
          if(char_to_room(victim, target_room, -1))
          {
            act("$n flies in, crashing on the floor!", TRUE, victim, 0, 0,
              TO_ROOM);
            SET_POS(victim, POS_PRONE + GET_STAT(victim));
            update_pos(victim);
            stop_fighting(victim);
            if( IS_DESTROYING(victim) )
              stop_destroying(victim);
            CharWait(victim, PULSE_VIOLENCE * 1);
          }
          return FALSE;
        }
    }
  */
    return true;
  }
  return false;
}


void spell_solbeeps_missile_barrage(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj tar_obj)
{
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;

  int num_missiles = 3, i = 0;

  if (level >= 56 || !number(0, 55 - level))
    num_missiles++;

 /* 
    if (level >= 56 && !number(0, 3))
    num_missiles++;
 */
  while( i < num_missiles &&
      IS_ALIVE(victim) &&
      IS_ALIVE(ch) &&
      victim)
  {
    if(!spell_solbeeps_single_missile(level, ch, arg, type, victim, tar_obj) )
      break;

    i++;
  }
}

void spell_fireball(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj tar_obj)
{
  struct damage_messages messages = {
    "You throw a &+rfireball&N at $N and have the satisfaction of seeing $M enveloped in flames.",
    "You are enveloped in &+rflames from a fireball&N sent by $n - OUCH!",
    "$n smirks as $s &+rfireball&N explodes into the face of $N.",
    "Your &+rfireball&N hits $N with full force, causing an immediate death.",
    "$n grins evilly as you burst into &+rflames&N and die.",
    "The heat from $n's &+rfireball&N turns $N into a charred corpse.", 0
  };

  int dam = (dice(((int)(level/3) + 5), 6) * 4);

  if(!NewSaves(victim, SAVING_SPELL, 0))
    dam = (int) (dam * 2);

  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_GLOBE, &messages);
}

/*
 * cast_as_damage_area passes pointer to the index of hit victim as arg
 */
void spell_single_chain_lightning(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      dam, order;
  struct damage_messages primary_messages = {
    "and &=LBblasts&n into $N!",
    "and &=LBblasts&n into YOU!",
    "and &=LBblasts&n into $N!",
    "and turns $N into a charred &=LBsparkling&n corpse!",
    "and you die as a &=LBflashing light&n explodes in your face!",
    "and turns $N into a charred &=LBsparkling&n corpse!", 0
  };
  struct damage_messages secondary_messages = {
    "then leaps and &=LBblasts&n into $N!",
    "then leaps and &=LBblasts&n into YOU!",
    "then leaps and &=LBblasts&n into $N!",
    "then leaps and turns $N into a charred &=LBsparkling&n corpse!",
    "and you die as a &=LBflashing light&n explodes in your face!",
    "then leaps and turns $N into a charred &=LBsparkling&n corpse!", 0
  };

  order = *((int *) arg);
  dam = 8 * MIN(51, level) + number(level/3, level) + 30;
  while( order-- )
  {
    dam = (int) (dam * 0.8);
  }

  if( IS_PC(ch) && IS_PC(victim) )
  {
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);
  }
  dam = dam * get_property("spell.area.damage.factor.chainlightning", 1.000);

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, 0, *((int *) arg) ? &secondary_messages : &primary_messages);
}

void spell_chain_lightning(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      hit, room;

  room = ch->in_room;
  send_to_char("A writhing &=LBbolt of lightning&n leaves your hands...\n", ch);
  act("A writhing &=LBbolt of lightning&n leaves $n's hands...", FALSE, ch, 0, 0, TO_ROOM);
  zone_spellmessage(room,
                       "&=LBThe sky lights up with brilliant lightning flashes!\n",
                       "&=LBThe sky to the %s lights up with brilliant lightning flashes!\n");

  hit = cast_as_damage_area(ch, spell_single_chain_lightning, level, victim,
                            get_property ("spell.area.minChance.chainLightning", 0),
                            get_property ("spell.area.chanceStep.chainLightning", 25));

  if( !hit )
  {
    send_to_room("and grounds harmlessly.\n", room);
  }
  else
  {
    send_to_room("and finally fizzles out.\n", room);
  }
}

void spell_single_lightning_ring(int level, P_char ch, char *arg, int type,
                                 P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "and &+Bsurges&n into $N!",
    "and &+Bsurges&n into YOU!",
    "and &+Bsurges&n into $N!",
    "and turns $N into a charred &=LBsparkling&n corpse!",
    "and you die as a &=LBflashing light&n explodes in your face!",
    "and turns $N into a charred &=LBsparkling&n corpse!", 0
  };

  dam = 6 * MIN(51, level) + number(1, level);

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, 0, &messages);
}

void spell_ring_lightning(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  send_to_char
    ("An intense &=LBelectrical bolt&n leaps forth from your fingers... \n",
     ch);
  act("A wild electrical surge emits from $n's fingertips...", FALSE, ch, 0,
      0, TO_ROOM);

  cast_as_damage_area(ch, spell_single_lightning_ring, level, victim,
                      get_property("spell.area.minChance.lightningRing", 30),
                      get_property("spell.area.chanceStep.lightningRing", 15));
}

void spell_cyclone(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  int dam, temp_coor, dead = 0;
  int svchance, affchance;
  struct damage_messages messages = {
    "&+WThe cyclone whirls and rips about, sending $N &+Wreeling!",
    "$n's &+Wcyclone hammers at your body!",
    "$n's &+Wcyclone tears at $N, &+Wpounding and rending flesh!",
    "&+WThe violent winds prove too much for $N!",
    "$n's &+Wcyclone has proven too much for your now soft and pulpy body.",
    "$n's &+Wcyclone tears and pummels $N until $N's lifeless body collapses."
  };

  if(victim == ch)
  {
    send_to_char("You suddenly decide against that, oddly enough.\n", ch);
    return;
  }

       // dam = dice(level * 2, 9);
	dam = dice((int) (MIN(level, 40) * 2), 10);

/*  play_sound(SOUND_WIND3, NULL, ch->in_room, TO_ROOM); */

  svchance = (int) (level / 12);

  if(IS_AFFECTED(victim, AFF_FLY) &&
    !NewSaves(victim, SAVING_PARA, svchance) &&
    !IS_ELITE(victim))
  {
    affchance = number(1, 100);

    if(affchance <= 50)         /* && (!check_wall(tch->in_room, door)) */
    {
      act("The gail force of your spell sends $N flying through the room!",
          FALSE, ch, 0, victim, TO_CHAR);
      act("The gail force of $n's spell sends you flying through the room!",
          FALSE, ch, 0, victim, TO_VICT);
      act("The gail force of $n's spell sends $N flying through the room!",
          FALSE, ch, 0, victim, TO_NOTVICT);
      //SET_POS(victim, POS_SITTING + GET_STAT(victim));

      if(!IS_STUNNED(victim))
      {
        Stun(victim, ch, PULSE_VIOLENCE / 2, TRUE);
      }
    }
    else if(affchance <= 5)
    {
      act("Your gail force sends $N flying through the room.", FALSE,
          ch, 0, victim, TO_CHAR);
      act("The gail force of $n's spell sends you flying through the room.",
          FALSE, ch, 0, victim, TO_VICT);
      act("The gail force of $n's spell sends $N flying through the room.",
          FALSE, ch, 0, victim, TO_NOTVICT);
      //SET_POS(victim, POS_PRONE + GET_STAT(victim));

      if(!IS_STUNNED(victim) && !number(0, 2))
      {
        Stun(victim, ch, PULSE_VIOLENCE, TRUE);
      }
    }
  }

  if(!StatSave(victim, APPLY_AGI, (GET_LEVEL(victim) - GET_LEVEL(ch)) / 5) &&
           !IS_ELITE(victim))
  {
    spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
  }
  else
  {
    spell_damage(ch, victim, dam >> 1, SPLDAM_GENERIC, 0, &messages);
  }
}

void spell_negative_energy_vortex(int level, P_char ch, char *arg, int type,
                                  P_char victim, P_obj obj)
{
  LOOP_THRU_PEOPLE(victim, ch)
  {
    if(ch->specials.z_cord == victim->specials.z_cord)
      spell_heal(GET_CLASS(ch, CLASS_WARLOCK) ? level : -1, ch, 0, 0, victim,
                 0);
  }
}

void spell_single_meteorswarm(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "You smash $N with your controlled meteors.",
    "$n smashes you with $s swarm of meteors, stunning you.",
    "$n massacres $N to little pieces with $s meteor swarm.",
    "$N is whacked by your swarm of meteors.",
    "You are whacked by a swarm of $n's meteors.",
    "$n obliterates $N with a swarm of meteors.", 0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  dam = 80 + level * 7 + number(1, 40);
  if( IS_PC(ch) && IS_PC(victim) )
  {
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);
  }
  dam = dam * get_property("spell.area.damage.factor.meteorSwarm", 1.000);
  if(GET_SPEC(ch, CLASS_SORCERER, SPEC_WIZARD))
  {
    dam = dam * 1.4;
  }
  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_meteorswarm(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  if(!OUTSIDE(ch))
  {
    send_to_char("You must be outside to cast this spell!\n", ch);
    return;
  }

/*   if(IS_PC(ch) &&
      stack_area(ch, SPELL_METEOR_SWARM,
                 (int) get_property("spell.area.stackTimer.meteorSwarm", 5)))
  {
    send_to_char
      ("Someone just summoned meteors from the above sky, but you try anyway.\n",
       ch);
    act("&+r$n tries to conjure a deadly meteor swarm, but only few fall.",
        FALSE, ch, 0, 0, TO_ROOM);
    level /= 2;
  }
  else */
  {
    act("&+rYou've conjured up a fearsome meteor swarm!", FALSE, ch, 0, 0,
        TO_CHAR);
    act("&+r$n conjures up a fearsome meteor swarm!", FALSE, ch, 0, 0,
        TO_ROOM);
  }

  zone_spellmessage(ch->in_room,
                       "&+rThe sky is full of &+Rflaming meteors!\r\n",
                       "&+rThe sky to %s is full of &+Rflaming meteors!\r\n");
  cast_as_damage_area(ch, spell_single_meteorswarm, level, victim,
                      get_property("spell.area.minChance.meteorSwarm", 50),
                      get_property("spell.area.chanceStep.meteorSwarm", 20));
}

void spell_entropy_storm(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  int      healpoints;
  P_char   tch, next;

  act
    ("&+LAs you open a rift to the &n&+bnegative material plane&+L, black vapors drift through, engulfing all nearby!",
     FALSE, ch, 0, 0, TO_CHAR);
  act
    ("&+LAs $n&+L opens a rift to the &n&+bnegative material plane&+L, black vapors drift through, engulfing all nearby!",
     FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[ch->in_room].people; tch != NULL; tch = next)
  {
    next = tch->next_in_room;

    if( tch == ch )
      continue;
    
    if(IS_NPC(ch) && IS_NPC(tch))
      continue;

    if(IS_UNDEADRACE(tch))
    {
      healpoints = 70;
      heal(tch, ch, healpoints, GET_MAX_HIT(tch));
      // healCondition(tch, healpoints);
      update_pos(tch);
      send_to_char
        ("&+LYou feel the black vapors infusing you with negative energy!\n",
         tch);
    }
    else
      if(!
          (IS_DRAGON(tch) || (GET_RACE(tch) == RACE_GOLEM) || IS_TRUSTED(tch))
          && !NewSaves(tch, SAVING_FEAR, 5))
    {
      if(fear_check(tch))
      {
        continue;
      }
      else
      {
        send_to_char("&+LArgh, those vapors are reaching right for you!",
                     tch);
        do_flee(tch, 0, 1);
      }
    }
  }

}

void spell_earthquake(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int save, dam_flag = 0, dam;
  P_char   tch, next;

  /*
   * special nasty for casting this underground or in buildings, damage
   * is MUCH higher (from falling objects) and not limited to opponents
   * only! (In other words, extremely dangerous to cast when not
   * outside).
   */
  if( !ch )
  {
    logit(LOG_EXIT, "spell_earthquake called in magic.c with no ch");
    raise(SIGSEGV);
  }
  if( ch->in_room > 0 )
  {
    switch (world[ch->in_room].sector_type)
    {
      case SECT_EARTH_PLANE:
        dam_flag = 3;
        break;
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_NO_GROUND:
      case SECT_UNDERWATER:
      case SECT_FIREPLANE:
      case SECT_LAVA:
      case SECT_OCEAN:
        dam_flag = 0;
        break;                      /*
                                     * what earthquake?
                                     */
      case SECT_CITY:
      case SECT_FIELD:
      case SECT_FOREST:
      case SECT_HILLS:
      case SECT_ROAD:
        dam_flag = 1;
        break;                      /*
                                     * normal damage
                                     */
      case SECT_MOUNTAIN:
        dam_flag = 2;               /*
                                     * dangerous, and slightly higher dam from
                                     * landslides
                                     */
        break;
      case SECT_INSIDE:
        dam_flag = 3;               /*
                                     * dangerous, and added damage from
                                     * falling debris
                                     */
        break;
      default:
        dam_flag = 1;
        break;
    }

    if( (dam_flag == 1) && !OUTSIDE(ch) )
    {
      dam_flag = 3;
    }
    if( ch->specials.z_cord != 0 )
    {
      dam_flag = 0;
    }
    switch( dam_flag )
    {
      case 0:
        send_to_char("No earth to quake here, try a different spell.\n", ch);
        return;
        break;
      case 1:
        send_to_char("&+yYou cause the earth to shake, crack and buckle!\n", ch);
        act("$n causes an &=LyEARTHQUAKE!", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        send_to_char("&+yYou cause the earth to shake, crack and buckle!\n", ch);
        send_to_char("&+yThe unstable nature of your surroundings, causes extreme amounts of extra debris!\n", ch);
        act("$n causes an &=LyEARTHQUAKE!", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        send_to_char("&+yYou cause the earth to shake, crack and buckle!\n", ch);
        send_to_char("&+yAs the ceiling begins to cave in on you, you realize this may not have been a good idea!\n", ch);
        act("$n causes an &=LyEARTHQUAKE!", FALSE, ch, 0, 0, TO_ROOM);
        break;
      default:
        send_to_char("As you are about to utter the last syllable, you choke it off,\nrealizing you could well bury yourself alive!\n", ch);
        return;
        break;
    }

    for( tch = world[ch->in_room].people; tch; tch = next )
    {
      next = tch->next_in_room;

      if( tch == ch || IS_TRUSTED(tch) )
      {
        continue;
      }
      if( !IS_ALIVE(tch) || !IS_ALIVE(ch) )
      {
        continue;
      }

      if( IS_GREATER_RACE(tch) )
      {
        save = 0;
      }
      else
      {
        save = 4;
      }

      // Expanded the immune races. Oct08 -Lucrot
      if( GET_RACE(tch) == RACE_FLYING_ANIMAL || GET_RACE(tch) == RACE_FAERIE || LEGLESS(tch) || IS_IMMATERIAL(tch) )
      {
        act("The ground rumbles beneath you, but affects you not at all.", FALSE, ch, 0, tch, TO_VICT);
        act("$n seems unaffected by the quake.", TRUE, tch, 0, 0, TO_ROOM);
        continue;
      }
      if( !should_area_hit(ch, tch) )
      {
        if( GET_POS(tch) < POS_STANDING )
        {
          continue;
        }

        if( StatSave(tch, APPLY_AGI, save) )
        {
          act("&+LYou stagger, but manage to keep your balance!&n", FALSE, ch, 0, tch, TO_VICT);
          act("$n&n &+wstaggers slightly but manages to keep $s balance.&n", TRUE, tch, 0, 0, TO_ROOM);
        }
        else
        {
          act("&+mYou stagger and fall to your knees!&n", FALSE, ch, 0, tch, TO_VICT);
          act("$n&n &+mstaggers and falls to $s knees!&n", TRUE, tch, 0, 0, TO_ROOM);
          SET_POS(tch, POS_KNEELING + GET_STAT(tch));
          CharWait(tch, PULSE_VIOLENCE * 1);
        }
      }
      else
      {
        if(GET_POS(tch) < POS_STANDING)
        {
          continue;
        }

        if(tch && !StatSave(tch, APPLY_AGI, save))
        {
          if(IS_PC(tch) && GET_CHAR_SKILL(tch, SKILL_SAFE_FALL) >= number(1, 101))
          {
            act("$n&n &+Gdoes a &+ydouble sommersault &+Gand lands on $s feet!&n", TRUE, tch, 0, 0, TO_ROOM);
            act("&+GYou do a &+Ydouble sommersault &+Gand land on your feet!", TRUE, tch, 0, 0, TO_CHAR);
            notch_skill(tch, SKILL_SAFE_FALL, 3);
          }
          else
          {
            act("&+WYou fall and injure yourself!&n", FALSE, ch, 0, tch, TO_VICT);
            act("$n&n &+Wcrashes to the ground!&n", TRUE, tch, 0, 0, TO_ROOM);
            if( level >= 0 )
            {
              dam = (int) (dice(1, 30) + level);
              if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
              {
                dam = (int) (dam * 1.25);
              }

              if(spell_damage(ch, tch, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0) == DAM_NONEDEAD);
              {
                SET_POS(tch, number(0, 2) + GET_STAT(tch));
                if(GET_POS(tch) == POS_PRONE && !number(0, 1))
                {
                  Stun(tch, ch, PULSE_VIOLENCE * 1, FALSE);
                  CharWait(tch, PULSE_VIOLENCE);
                }
              }
            }
            // Lvl < 0 -> from earthen rain so we do takedown only no damage.
            else
            {
              SET_POS(tch, number(0, 2) + GET_STAT(tch));
              if(GET_POS(tch) == POS_PRONE && !number(0, 1))
              {
                Stun(tch, ch, PULSE_VIOLENCE * 1, FALSE);
                CharWait(tch, PULSE_VIOLENCE);
              }
            }
          }
        }
        else if(tch && ch->specials.z_cord == tch->specials.z_cord )
        {
          act("&+LYou stagger and almost break your leg!&n", FALSE, ch, 0, tch, TO_VICT);
          act("$n&n &+Lstaggers and almost falls!&n", TRUE, tch, 0, 0, TO_ROOM);
          dam = (int) (dice(1, 4) + dam_flag * level / 2);
          // lvl < 0 -> don't damage them as this is from earthen rain -> fall only.
          if( (level >= 0) && spell_damage(ch, tch, dam, SPLDAM_EARTH,
            SPLDAM_NOSHRUG | SPLDAM_BREATH | SPLDAM_NODEFLECT, 0) != DAM_NONEDEAD )
          {
            return;
          }
        }
      }
    }
    if(IS_ALIVE(ch))
    {
      zone_spellmessage(ch->in_room,
        "&+yThe ea&+Lrt&+yh tr&+Lemb&+yle&+Ls an&+yd sh&+Liv&+yers!\n",
        "&+yThe ea&+Lrt&+yh tr&+Lemb&+yle&+Ls an&+yd sh&+Liv&+yers &+Lto the %s!\n");
    }
  }
}

void spell_single_firestorm(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "You engulf $N with a red hot &+rfirestorm&N!",
    "$n surrounds you with &+Rsearing flames&N!",
    "$n bathes $N in cleansing &+rflame&N, but $E does not look thankful.",
    "$N is flash-fried, oooohh, crispy critter!",
    "$n kills and cremates you with a single spell, how efficient!",
    "$N is turned to ash by $n's &+rfirestorm&N!", 0
  };

  dam = dice(level, 10);
  if(NewSaves(victim, SAVING_SPELL, 0))
    dam >>= 1;
  
  if (IS_PC(ch) && IS_PC(victim))
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);
  
  dam = dam * get_property("spell.area.damage.factor.fireStorm", 1.000);
  spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
}

void spell_firestorm(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct room_affect raf;
  int room = ch->in_room;
  P_char tch;

  // play_sound(SOUND_FIRESTORM, NULL, ch->in_room, TO_ROOM);

  send_to_char("You call a &+Rraging&n &+rfirestorm&n to engulf your foes!\n",
               ch);
  act("$n creates a &+Rraging&n &+rfirestorm&n!", FALSE, ch, 0, 0,
      TO_VICTROOM);
  zone_spellmessage(ch->in_room,
                    "&+YYou feel a blast of &+Rheat!\n",
                    "&+YYou feel a blast of &+Rheat &+Yfrom the %s!\n");
  cast_as_damage_area(ch, spell_single_firestorm, level, victim,
                      get_property("spell.area.minChance.fireStorm", 90),
                      get_property("spell.area.chanceStep.fireStorm", 10));

  memset(&raf, 0, sizeof(raf));
  raf.type = SPELL_FIRESTORM;
  raf.duration = (int)(1.5 * PULSE_VIOLENCE);
  affect_to_room(room, &raf);

  for (tch = world[room].people; tch; tch = tch->next_in_room)
    if(IS_AFFECTED5(tch, AFF5_WET)) {
      send_to_char("The heat of the firestorm dried up your clothes.\n", tch);
      make_dry(tch);
    }
}

void spell_dispel_good(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "A &+rred aura&N surrounds you as you shoot out a &+Lblack bolt of energy&N from your hands to destroy $N.",
    "A bolt of evil energy sent by $n engulfs you, dissolving your very soul!",
    "A &+rred aura&N surrounds $n as a &+Lblack bolt of energy&N shoots out from $s hands to dispel $N.",
    "You cackle in triumph as the essence of $N is utterly destroyed by your evil power!",
    "You scream in horror as your soul is purged from existance by the evil forces of $n!",
    "$N screams in terror as $S soul is ripped to shreds by the vile power of $n's spell!"
  };

/*  if(IS_GOOD(ch))
      victim = ch; */// Removed because of bard song
  if(!IS_GOOD(victim))
  {
    act("$N basks in your ignorance.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n is clueless.", FALSE, ch, 0, victim, TO_VICT);
    act("$N chuckles at $n's cluelessness.", FALSE, ch, 0, victim,
        TO_NOTVICT);
    return;
  }
  dam = dice(level, 8);

  if(saves_spell(victim, SAVING_SPELL))
    dam >>= 1;

  spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_ALLGLOBES, &messages);
}

void spell_dispel_evil(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "$N shivers, and suffers from $S evilness!",
    "$n makes your soul hurt and suffer!",
    "$n makes $N's evil spirit shiver and suffer!",
    "$N is dissolved by your goodness.",
    "$n dissolves you, you regret having been so evil, and die...",
    "$n completely dissolves $N."
  };

/*  if(IS_EVIL(ch))
      victim = ch; */// Removed because of bard song
  if(!IS_EVIL(victim))
  {
    act("$N chuckles at your foolishness.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n is clueless.", FALSE, ch, 0, victim, TO_VICT);
    act("$N chuckles at $n's cluelessness.", FALSE, ch, 0, victim,
        TO_NOTVICT);
    return;
  }
  dam = dice((level + 1), 5);

  if(saves_spell(victim, SAVING_SPELL))
    dam >>= 1;

  spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_ALLGLOBES, &messages);
}

void spell_flamestrike(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "You call down a roaring &+rflamestrike&N which hits $N dead on!",
    "$n calls down a roaring &+rflamestrike&N which hits you dead on!",
    "$n calls down a roaring &+rflamestrike&N at $N who starts to melt!",
    "$N turns into a pile of ash as your &+rflamestrike&N hits!",
    "Your body turns to ash as $n calls down a &+rflamestrike&N on you!",
    "$n calls down a &+rflamestrike&N on $N, who turns into a pile of ash!", 0
  };
  int num_dice = (level / 5);
  int dam = (dice( num_dice, 6) * 4);

  if( !NewSaves(victim, SAVING_SPELL, 0) )
    dam = (int) (dam * 2);

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    dam = (int) (dam * 1.25);
  }

  spell_damage(ch, victim, (dam / 2), SPLDAM_HOLY, RAWDAM_NOKILL, 0);
  if(IS_ALIVE(victim))
  {
    spell_damage(ch, victim, (dam / 2), SPLDAM_FIRE, 0, &messages);
  }
}

struct call_lightning_data
{
  int room;
  int waves;
  int level;
};

void event_call_lightning(P_char ch, P_char vict, P_obj obj, void *data)
{
  struct call_lightning_data *clData = (struct call_lightning_data *)data;
  int result, dam = dice(MIN(40, clData->level),9);
  struct damage_messages messages = {
    "A &+WMASSIVE&n &=LBlightning bolt&N strikes $N from the sky!",
    "A &+WMASSIVE&n &=LBlightning bolt&N from the heavens strikes you!",
    "A &+WMASSIVE&n &=LBlightning bolt&N strikes $N from the sky!",
    "Your &+WMASSIVE&n &=LBlightning bolt&N shatters $N to pieces.",
    "Your last vision is that of little kites circling your head, all being struck by lightning.",
    "$N is utterly shattered from the force of a &=LBlightning bolt&n from the sky.",
      0
  };

  if(!vict ||
     vict->in_room != clData->room)
  {
    vict = get_random_char_in_room(clData->room, ch,
        DISALLOW_SELF | DISALLOW_GROUPED);
  }
  
  if(!vict)
  {
    return;
  }
  
  result = spell_damage(ch, vict, dam, SPLDAM_LIGHTNING, 0, &messages);

  if(result == DAM_BOTHDEAD)
  {
    return;
  }
  else if(!fear_check(vict) &&
          !IS_GREATER_RACE(vict) &&
          !IS_ELITE(vict) &&
          result != DAM_VICTDEAD &&
          !NewSaves(vict, SAVING_FEAR, 0))
  {
    send_to_char("The &+Rma&+rss&+Riv&+re &+mbolt&n of &+Clig&+ch&+Wtn&+Cing&n is intimidating!\n", vict);
    do_flee(vict, 0, 0);
  }
  else if(result != DAM_VICTDEAD &&
          !IS_ELITE(vict) &&
          !IS_GREATER_RACE(vict))
  {
    Stun(vict, ch, (int) (PULSE_VIOLENCE / 2), TRUE);
  }
  
  if(result != DAM_CHARDEAD &&
    (clData->waves++ < 2))
  {
    if(number(0, 1))
    {
      zone_spellmessage(ch->in_room,
        "&+wThe air is filled with &+c&+Ce&+cl&+Ce&+cc&+Ct&+cr&+Ci&+cc &+Cs&+ct&+Ca&+ct&+Ci&+cc.\n",
        "&+wThe air to the %s is filled with &+c&+Ce&+cl&+Ce&+cc&+Ct&+cr&+Ci&+cc &+Cs&+ct&+Ca&+ct&+Ci&+cc.\n");
    }
    else
    {
      zone_spellmessage(ch->in_room,
       "&+WA clap of &+Lthunder&n &+Wbellows off in the distance.\n",
       "&+WA clap of &+Lthunder&n &+Wbellows off to the %s.\n");
    }
    
    add_event(event_call_lightning, (int) ( PULSE_VIOLENCE / 2),
      ch, vict, NULL, 0, clData, sizeof(struct call_lightning_data));
  }
  else
  {
    send_to_room("&+LThe clouds overhead disperse.\n", clData->room);
  }
}

void spell_call_lightning(int level, P_char ch, P_char victim, P_obj obj)
{
  struct call_lightning_data clData;

  if(!OUTSIDE(ch))
  {
    send_to_char("You must be outdoors to summon lightning!\n", ch);
    return;
  }

  zone_spellmessage(ch->in_room,
    "&+LA storm is brewing nearby...\n",
    "&+LA storm is brewing to the %s...\n");
  
  send_to_room("&+LDark and ominous clouds aggregate overhead.\n",
               ch->in_room);

  clData.room = ch->in_room;
  clData.waves = 0;
  clData.level = level;

  add_event(event_call_lightning, PULSE_VIOLENCE, ch, victim, NULL, 0, &clData,
            sizeof(struct call_lightning_data));
}

void spell_destroy_undead(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "$N wavers under your deity's power!",
    "Piercing light from $n's holy symbol hurts you!",
    "$N wavers under the onslaught caused by $n's deity's power!",
    "$N is disintegrated by the unleashed power of your deity!",
    "You see $n's raised holy symbol, and nothing more.",
    "$N is dispelled entirely by $n's deity's power!", 0
  };

  struct damage_messages notcleric_msgs = {
    "$N wavers under the power of your will!",
    "$n's pure will causes you horrible pain!",
    "$N wavers under the power of $n's will!",
    "$N is completely destroyed by your mastery of the walking dead!",
    "$n's mastery over you is complete as he wills you to exist no more!",
    "$N is completely destroyed by $n's raw will!", 0
  };

  if(!ch)
  {
    logit(LOG_EXIT, "spell_destroy_undead called in magic.c with no ch");
    raise(SIGSEGV);
    return;
  }

  if(ch &&
    victim &&
    !IS_UNDEADRACE(victim) &&
    !IS_AFFECTED(victim, AFF_WRAITHFORM))
  {
    send_to_char("Your victim isn't even dead, much less undead!\n", ch);
    return;
  }

  dam = 15 * MIN(level, 56) + number(-40, 40);
// dam = 13 * level + number(0, level);
  if(saves_spell(victim, SAVING_SPELL))
    dam >>= 1;

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    dam = (int) (dam * 1.25);
  }

  spell_damage(ch, victim, dam, SPLDAM_HOLY, 0,
               IS_CLERIC(ch) ? &messages : &notcleric_msgs);
}

void spell_harm(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "The power of your god causes $N's body to suffer from your harm spell!",
    "A mighty force from above rips you apart as a harm spell hits you.",
    "$n calls upon the power of the gods to tear apart the insides of $N with a mighty harm spell!",
    "Your harm spell reaches deep into the soul of $N and crushes it!",
    "$n calls upon down the power of the gods to crush you in a harm spell...death soon follows..",
    "$N is slowly ripped into many tiny bits as the harm spell of $n, ends his life.",
      0
  };

  if(saves_spell(victim, SAVING_SPELL))
    dam = 100;
  else
    dam = 200;

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    dam = (int) (dam * 1.25);
  }

  spell_damage(ch, victim, dam, SPLDAM_HOLY, RAWDAM_NOKILL, &messages);
}

void spell_full_harm(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "&+WYou call down the FULL wrath of your god on $N&+W!",
    "&+W$n&N calls down a holy blast of might at you, ouch!",
    "&+WA beam of pure holy wrath is called down on $N &+Wby $n!",
    0, 0, 0, 0
  };

    dam = (level / 3 * 11);
  if( !NewSaves(victim, SAVING_SPELL, 0) )
    dam = (dam * 2);

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    dam = (int) (dam * 1.25);
  }

  spell_damage(ch, victim, dam, SPLDAM_HOLY, RAWDAM_NOKILL, &messages);
}

void event_fleshdecay(P_char ch, P_char victim, P_obj obj, void *data)
{
  int dam;
  int count = *((int*)data);
  int rand1 = number(1, 100);

  dam = dice(5, 10);

  if ((GET_HIT(ch) - dam) > 0)
  {
    GET_HIT(ch) -= dam;
    
    if(rand1 > 50)
    {
    send_to_char("&nA &+gslimy &npiece of your &+rflesh&n breaks apart from your body and falls to the &+yground&n.\n", ch);
    act("A &+gslimy&n piece of $n's &+rflesh &nbreaks apart from their body and falls to the &+yground&n\n", TRUE, ch, NULL, NULL,
        TO_NOTVICT);
    make_bloodstain(ch);
    }
    else
    {
    send_to_char("&nYour skin continues to &+gde&+Gca&+Ly &nas the spell consumes your &+rflesh&n.\n", ch);
    act("$n's skin continues to &+gde&+Gca&+Ly &nas the spell consumes their &+rflesh&n.", TRUE, ch, NULL, NULL,
        TO_NOTVICT);
    make_bloodstain(ch);
    }
  }

  if (count >= 0)
  {
    count--;
    add_event(event_fleshdecay, PULSE_VIOLENCE, ch, 0, 0, 0, &count, sizeof(count));
  }
  
}

void spell_decaying_flesh(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct affected_type af;

  if(ch == victim)
   {
	send_to_char("You decide that would not be the best use of your dark arts.&n\n", ch);
       return;
   }

    if(!victim)
    victim = ch;
  
  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
  {
    return;
  }
  if(IS_AFFECTED5(victim, AFF5_DECAYING_FLESH))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_DECAYING_FLESH && af1->modifier >= 5)
      {
        
        send_to_char("&nTheir flesh cannot be afflicted any further.\r\n",
                     ch);
		return;
      }
   
  }

  memset(&af, 0, sizeof(af));
  af.type = SPELL_DECAYING_FLESH;
  af.bitvector5 = AFF5_DECAYING_FLESH;
  af.duration = 100;
  af.flags = AFFTYPE_SHORT;

  if(!IS_AFFECTED5(victim, AFF5_DECAYING_FLESH))
  {
    act("&+R$n &nraises their hand and points directly at &+L$N\n"
	     "&+L$N &nsuddenly turns &+ggreen &nas their &+Rflesh &nbegins to &+Lwither&n and &+rrot&n.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You &nraise your hand and point directly at &+L$N\n"
	     "&+L$N &nsuddenly turns &+ggreen &nas their &+Rflesh &nbegins to &+Lwither&n and &+rrot&n.", FALSE, ch, 0, victim, TO_CHAR);
   act("&+R$n &nraises their hand and points directly at &+LYOU&n!\n"
	          "Your &+Rskin&n suddenly turns &+ggreen &nand starts &+Lwithering &nand &+rrotting &nright before your eyes!", FALSE, ch, 0, victim, TO_VICT);
    af.modifier = 1;
    affect_to_char(victim, &af);
    int numb = number(2, 6);
    add_event(event_fleshdecay, PULSE_VIOLENCE, victim, 0, 0, 0, &numb, sizeof(numb));

	
  }
  else
  {
    act("&+R$n &nagain points at &+L$N &ncausing the existing &+gdecay&n to worsen&n.", FALSE, ch, 0, victim, TO_ROOM);
    act("&+RYou &nagain point at &+L$N &ncausing the existing &+gdecay&n to worsen&n.", FALSE, ch, 0, victim, TO_CHAR);
    //af.modifier += 1;
   // af->modifier ++;
  
     struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_DECAYING_FLESH)
      {
        
        af1->modifier = af1->modifier++;
        af1->duration = af1->duration + 100;
      }
  }  
  attack_back(ch, victim, TRUE);
}


/*
 * spells2.c - Not directly offensive spells
 */
void spell_fortitude(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;
  int      temp;

  temp = (int) (level / 10);
  if(!affected_by_spell(victim, SPELL_FORTITUDE))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_FORTITUDE;
    af.duration = 2 * (1 + temp);
    af.location = APPLY_CON;
    af.modifier = BOUNDED(1, GET_LEVEL(ch) / 4, 5);
    affect_to_char(victim, &af);
    send_to_char("&+WYour will to live grows stronger!\n", victim);
  }
}

void event_nova(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      room = *((int*)data);

  if(!number(0, 2))
  {
    send_to_room
      ("&+WA massive &+Yball of light&+W slowly throbs, sucking all light from every corner of this area...\n",
       room);

    add_event(event_nova, PULSE_VIOLENCE * 2, ch, 0, 0, 0, &room, sizeof(room));
    return;
  }

  act
    ("&+LYour immense &+Wgathering of light&+w comes to fruition, &+Yexploding with violent force!",
     FALSE, ch, 0, 0, TO_CHAR);
  act
    ("&+L$n's&+L immense &+Wgathering of light&+w comes to fruition, &+Yexploding with violent force!",
     FALSE, ch, 0, 0, TO_ROOM);
     
  zone_spellmessage(ch->in_room,
    "&+YT&+yh&+Yi&+yn &+Yr&+ya&+Yy&+ys of &+Yli&+ygh&+Yt &+rex&+Rplo&+rde &+ythroughout the &+Warea!\n",
    "&+YT&+yh&+Yi&+yn &+Yr&+ya&+Yy&+ys of &+Yli&+ygh&+Yt &+rex&+Rplo&+rde &+ythroughout the &+Warea to the %s!\n");
     
  cast_as_damage_area(ch, spell_sunray,
      IS_NPC(ch) ? GET_LEVEL(ch)
      : (int) (GET_LEVEL(ch) * 0.6), NULL,
      get_property("spell.area.minChance.nova", 90),
      get_property("spell.area.chanceStep.nova", 10));
}

void spell_nova(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int room = ch->in_room;

  send_to_room("&+YA point of light appears in the middle of the room, slowly growing larger!\n", ch->in_room);

  add_event(event_nova, PULSE_VIOLENCE * 2, ch, 0, 0, 0, &room, sizeof(room));
}

void spell_harmonic_resonance(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if(!ch)
    return;

  send_to_room("&+cA &+Cchorus &+cof &+Cresonant &+Wsound &+Cinteracts &+cwith your &+Csurroundings!\n", ch->in_room);
  switch (world[ch->in_room].sector_type)
  {
  case SECT_FOREST:
    LOOP_THRU_PEOPLE(victim, ch)
    {
      if( should_area_hit(ch, victim) )
      {
        spell_cdoom(level, ch, 0, SPELL_TYPE_SPELL, victim, NULL);
      }
    }
    break;
  case SECT_FIELD:
    cast_call_lightning(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
    break;
  case SECT_HILLS:
    spell_earthquake(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
    break;
  case SECT_MOUNTAIN:
  case SECT_EARTH_PLANE:
    spell_earthen_rain(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
    break;
  case SECT_SWAMP:
    LOOP_THRU_PEOPLE(victim, ch)
    {
      if(should_area_hit(ch, victim))
        spell_entangle(level, ch, 0, SPELL_TYPE_SPELL, victim, NULL);
    }
    break;
  case SECT_OCEAN:
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_WATER_PLANE:
        spell_miracle(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
        break;
  case SECT_DESERT:
        spell_firestorm(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
        break;
  case SECT_UNDERWATER:
    spell_tranquility(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
    break;
  case SECT_NO_GROUND:
  case SECT_AIR_PLANE:
    LOOP_THRU_PEOPLE(victim, ch)
    {
      if(should_area_hit(ch, victim))
        spell_cyclone(level, ch, 0, SPELL_TYPE_SPELL, victim, NULL);
    }
    break;
  case SECT_FIREPLANE:
    spell_nova(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
    break;
  case SECT_LAVA:
    spell_firestorm(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
    break;
  }
}
void spell_siren_song(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  if(!ch || !victim)
    return;

  if(ch->in_room == victim->in_room)
  {
    send_to_char("That person is already near you.\n", ch);
    return;
  }
  if( how_close(ch->in_room, victim->in_room, (int)get_property("spell.siren.song.dist", 30)) <= 0 )
  {
    send_to_char("Your voice strains from the distance to reach your target.\n", ch);
    return;
  }
//CANT CAST FROM OR TO GUILDHALLROOMS
  if((IS_SET(world[ch->in_room].room_flags, GUILD_ROOM)) ||
      (IS_SET(world[victim->in_room].room_flags, GUILD_ROOM)))
  {
    send_to_char("Your siren song stops abruptly!\n", ch);
    return;
  }
  act("&+g$n&+g emits an &+Genchanting &+Csong &+gthat tugs at your &+Csoul.",
      FALSE, ch, 0, 0, TO_ROOM);
  act("&+gYou emit an &+Genchanting &+Csong &+gthat fills the area.", FALSE,
      ch, 0, 0, TO_CHAR);

  act
    ("&+gAn &+Genchanting &+Csong fills the area... &+git tugs on your &+Csoul.",
     FALSE, victim, 0, 0, TO_CHAR);
  act
    ("&+gAn &+Genchanting &+Csong fills the area... &+git tugs on your &+Csoul.",
     FALSE, victim, 0, 0, TO_ROOM);

  if(!IS_FIGHTING(victim) && !IS_PATROL(victim) && !NewSaves(victim, SAVING_SPELL, GET_C_CHA(ch) - GET_C_POW(victim)))
  {
    if( IS_DESTROYING(victim) )
      stop_destroying(victim);
    send_to_char
      ("&+cYou can resist no longer! The &+Csong draws your nearer, nearer...\n",
       victim);
    act("$n wanders out of the room in a daze!", FALSE, victim, 0, 0,
        TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, ch->in_room, -1);
    act("$n wanders into the room in a daze!", FALSE, victim, 0, 0, TO_ROOM);
  }
}

void spell_elemental_aura(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!ch)
    return;

  if( affected_by_spell(ch, SPELL_ELEMENTAL_AURA)
    || IS_AFFECTED2(ch, AFF2_EARTH_AURA)
    || IS_AFFECTED2(ch, AFF2_WATER_AURA)
    || IS_AFFECTED2(ch, AFF2_FIRE_AURA)
    || IS_AFFECTED2(ch, AFF2_AIR_AURA)
    || IS_AFFECTED4(ch, AFF4_ICE_AURA) )
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_ELEMENTAL_AURA;
  af.duration = (SECS_PER_MUD_DAY / 60);

/* We do not want it going off in a non-plane room
  af.location = APPLY_AC;
  affect_to_char(victim, &af);*/

  switch (world[ch->in_room].sector_type)
  {
  case SECT_FIREPLANE:
    act("&+rYour body glows with an aura of fire!", FALSE, ch, 0, 0, TO_CHAR);
    act("&+r$n's&+r body glows with an aura of fire!", FALSE, ch, 0, 0, TO_ROOM);
    af.location = APPLY_STR_MAX;
    af.modifier = 50;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX_MAX;
    af.modifier = -20;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI_MAX;
    af.modifier = -10;
    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = 25;
    affect_to_char(victim, &af);

    if(NewSaves(ch, SAVING_FEAR, 4) || IS_NPC(ch))
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
    break;
  case SECT_WATER_PLANE:
    act("&+bYour body flows with an aura of water!", FALSE, ch, 0, 0,
        TO_CHAR);
    act("&+b$n's&+b body flows with an aura of water!", FALSE, ch, 0, 0,
        TO_ROOM);
    af.location = APPLY_STR_MAX;
    af.modifier = -20;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX_MAX;
    af.modifier = 50;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI_MAX;
    af.modifier = 25;
    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = -10;
    affect_to_char(victim, &af);

    if(NewSaves(ch, SAVING_FEAR, 4) || IS_NPC(ch))
    {
      act("&+BYour body &+bliquifies&+B as you achieve full form!", FALSE, ch,
          0, 0, TO_CHAR);
      act("&+B$n's&+B body &+bliquifies&+B as $e achieves full form!", FALSE,
          ch, 0, 0, TO_ROOM);
      af.location = 0;
      af.modifier = 0;
      af.bitvector2 = AFF2_WATER_AURA;
      affect_to_char(victim, &af);
    }
    break;
  case SECT_AIR_PLANE:
    act("&+cYour body fumes with an aura of air!", FALSE, ch, 0, 0, TO_CHAR);
    act("&+c$n's&+c body fumes with an aura of air!", FALSE, ch, 0, 0,
        TO_ROOM);
    af.location = APPLY_STR_MAX;
    af.modifier = -25;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX_MAX;
    af.modifier = 25;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI_MAX;
    af.modifier = 100;
    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = -20;
    affect_to_char(victim, &af);

    if(NewSaves(ch, SAVING_FEAR, 4) || IS_NPC(ch))
    {
      act("&+CYour body vaporizes as you achieve full form!", FALSE, ch, 0, 0,
          TO_CHAR);
      act("&+C$n's&+C body vaporizes as $e achieves full form!", FALSE, ch, 0,
          0, TO_ROOM);
      af.location = 0;
      af.modifier = 0;
      af.bitvector2 = AFF2_AIR_AURA;
      affect_to_char(victim, &af);
    }
    break;
  case SECT_EARTH_PLANE:
    act("&+yYour body hardens with an aura of earth!", FALSE, ch, 0, 0,
        TO_CHAR);
    act("&+y$n's&+y body hardens with an aura of earth!", FALSE, ch, 0, 0,
        TO_ROOM);
    af.location = APPLY_STR_MAX;
    af.modifier = 75;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX_MAX;
    af.modifier = -25;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI_MAX;
    af.modifier = -20;
    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = 75;
    affect_to_char(victim, &af);

    if(NewSaves(ch, SAVING_FEAR, 4) || IS_NPC(ch))
    {
      act("&+yYour body turns to &+Lstone&+y as you achieve full form!",
          FALSE, ch, 0, 0, TO_CHAR);
      act("&+y$n's&+y body turns to &+Lstone&+y as $e achieve full form!",
          FALSE, ch, 0, 0, TO_ROOM);
      af.location = 0;
      af.modifier = 0;
      af.bitvector2 = AFF2_EARTH_AURA;
      affect_to_char(victim, &af);
    }
    break;

  default:
    send_to_char("Nothing seems to happen.\n", ch);
  }

}

void spell_armor(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  double mod;
  bool shown;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_HOLYMAN) )
  {
    mod = 1.25;
  }
  else
  {
    mod = 1;
  }

  if( !IS_AFFECTED(victim, AFF_ARMOR) )
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_ARMOR;
    af.duration = 20;
    af.modifier = (int) (-1 * mod * level - number(0, 10));
    af.location = APPLY_AC;
    af.bitvector = AFF_ARMOR;
    affect_to_char(victim, &af);
    send_to_char("&+WBands of magic armor wrap around you!\n", victim);
  }
  else
  {
    struct affected_type *af1;
    shown = FALSE;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if( af1->type == SPELL_ARMOR )
      {
        if( !shown )
        {
          send_to_char("&+WThe bands of magic armor glow as new!\n", victim);
          shown = TRUE;
        }
        af1->duration = 20;
      }
    }
    if( !shown )
    {
      send_to_char( "&+WYou're already affected by an armor-type spell.\n", victim );
    }
  }
}

// end spell_armor

void spell_virtue(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_VIRTUE))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_VIRTUE;
    af.modifier = level;
    af.duration = level/2;
    affect_to_char(victim, &af);
    send_to_char("&+WYou feel overwhelmed to honor the gods!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_VIRTUE)
      {
        af1->duration = level/2;
      }
    }
  }
}


void spell_dispel_lifeforce(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(IS_UNDEADRACE(victim))
  {
    send_to_char("&+LYour victim is not among the living.&n\n", ch);
    return;
  }

  if(!affected_by_spell(victim, SPELL_DISPEL_LIFEFORCE) &&
      !NewSaves(victim, SAVING_PARA, 0))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_DISPEL_LIFEFORCE;
    af.duration =  25;
    af.modifier = 30;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
    af.modifier = -30;
    af.location = APPLY_STR;
    affect_to_char(victim, &af);
    af.modifier = -10;
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);
    af.modifier = -10;
    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);

    act("&+L$n&+L is completely sapped of $s lifeforce!!", FALSE, victim, 0,
        0, TO_ROOM);
    act("&+LYou are completely sapped of your lifeforce!!", FALSE, victim, 0,
        0, TO_CHAR);
  }
}

void spell_alter_energy_polarity(int level, P_char ch, char *arg, int type,
                                 P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_ALTER_ENERGY_POLARITY) &&
      !NewSaves(victim, SAVING_PARA, 0))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_ALTER_ENERGY_POLARITY;
    af.duration = 7;
    af.bitvector4 = AFF4_REV_POLARITY;
    affect_to_char(victim, &af);

    act("&+L$n&+L fades from view for a split second, then reappears.", FALSE,
        victim, 0, 0, TO_ROOM);
    act("&+LYour vision goes black for a brief moment..... hrmmm.", FALSE,
        victim, 0, 0, TO_CHAR);
  }
}

void spell_sense_holiness(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_SENSE_HOLINESS))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_SENSE_HOLINESS;
    af.duration =  25;
    af.bitvector4 = AFF4_SENSE_HOLINESS;
    affect_to_char(victim, &af);
    send_to_char("&+WYou can now sense holy auras!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SENSE_HOLINESS)
      {
        af1->duration = 25;
      }
  }

}

void spell_wizard_eye(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  int      target, distance;
  char     Gbuf1[MAX_STRING_LENGTH];

  if(!ch)
    return;

  if(!victim)
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  target = victim->in_room;
  
  distance = MAX(level*2, (int) get_property("spell.wizardEye.maxDistance", 100));

  if( IS_TRUSTED(victim) || racewar(ch, victim)
    || !(world[ch->in_room].zone == world[victim->in_room].zone)
    || IS_NPC(victim)
    || (IS_MAP_ROOM(ch->in_room) && IS_MAP_ROOM(victim->in_room) &&
       (calculate_map_distance(ch->in_room, victim->in_room) > (distance*distance))) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  strcpy(Gbuf1, "&+WYou cast your sights far out into the zone...\n");
  send_to_char(Gbuf1, ch);
  new_look(ch, NULL, CMD_LOOKAFAR, target);
  // play_sound(SOUND_FARSITE, NULL, ch->in_room, TO_ROOM);

  if(NewSaves(victim, SAVING_SPELL, 0))
    send_to_char("&+LYou feel strangely like you are being watched..\n",
                 victim);
}

void spell_clairvoyance(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int      target, where;
  char     Gbuf1[MAX_STRING_LENGTH];

  if(!ch)
    return;

  if(!victim)
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  // Disabling this spell to stop ppl from cheating.
  if(TRUE)
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  where = world[victim->in_room].number;
  target = victim->in_room;
  if(number(0, 3))
    target = real_room0(where + number(-2, 2));

  if(!target)
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  if((GET_RACE(ch) != RACE_ILLITHID) && !IS_NPC(victim))
  {
    if(racewar(ch, victim))
    {
      send_to_char("&+CYou failed.\n", ch);
      return;
    }
  }
  if(IS_DISGUISE(victim))
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  if( IS_TRUSTED(victim) || (IS_NPC(victim)))
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  if(IS_AFFECTED3(victim, AFF3_NON_DETECTION))
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  strcpy(Gbuf1, "&+WYou cast your sights far out into the realms...\n");
  send_to_char(Gbuf1, ch);
  new_look(ch, NULL, CMD_LOOKAFAR, target);

  if(NewSaves(victim, SAVING_SPELL, 0) && victim->in_room == target)
    send_to_char("&+LYou feel strangely like you are being watched..\n",
                 victim);
}

void spell_teleimage(int level, P_char ch, P_char victim, P_obj obj)
{
  int      target;
  P_char   tmp_victim;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if(!victim)
  {
    send_to_char("And just who are you trying to look for?\n", ch);
    return;
  }
  target = world[victim->in_room].number;
  strcpy(Gbuf1,
         "$n conjures up a cloud which thickens, solidifies, shimmers, and\n"
         "then ... suddenly goes transparent and shows ...\n");
  act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
  act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
  sprintf(Gbuf2, "%d look", target);
  for (tmp_victim = world[ch->in_room].people; tmp_victim;
       tmp_victim = tmp_victim->next_in_room)
  {
    if(IS_PC(tmp_victim))
      do_at(tmp_victim, Gbuf2, -4);
  }
}

void spell_dimension_door(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      location;
  char     buf[256] = { 0 };
  P_char   tmp = NULL;
  int      distance;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  if(GET_SPEC(ch, CLASS_SORCERER, SPEC_SHADOW))
    CharWait(ch, WAIT_SEC);
  else if(IS_PC(ch))
    CharWait(ch, WAIT_SEC * 9);
  else
    CharWait(ch, WAIT_SEC * 1 + 1);

  if( IS_AFFECTED3(victim, AFF3_NON_DETECTION) || IS_SET(world[ch->in_room].room_flags, NO_TELEPORT)
    || IS_HOMETOWN(ch->in_room) || world[ch->in_room].sector_type == SECT_OCEAN
    || world[victim->in_room].sector_type == SECT_CASTLE || world[victim->in_room].sector_type == SECT_CASTLE_WALL
    || world[victim->in_room].sector_type == SECT_CASTLE_GATE )
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  if( IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE) && !is_introd(victim, ch) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }
  P_char rider = get_linking_char(victim, LNK_RIDING);
  if( IS_NPC(victim) && rider )
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  if( !IS_TRUSTED(ch) && IS_TRUSTED(victim) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  location = victim->in_room;

  if( IS_SET(world[location].room_flags, NO_TELEPORT) || IS_HOMETOWN(location)
    || racewar(ch, victim) || world[location].sector_type == SECT_OCEAN || (IS_PC_PET(ch) && IS_PC(victim)) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  if( !is_Raidable(ch, 0, 0) )
  {
    send_to_char("&+WYou are not raidable. The spell fails!\r\n", ch);
    return;
  }

  if( IS_PC(ch) && IS_PC(victim) && !is_Raidable(victim, 0, 0) )
  {
    send_to_char("&+WYour target is not raidable. The spell fails!\r\n", ch);
    return;
  }

  distance = (int)(level * 1.35);

  if(GET_SPEC(ch, CLASS_SORCERER, SPEC_SHADOW))
    distance += 15;

  if(!IS_TRUSTED(ch) &&
      (how_close(ch->in_room, victim->in_room, distance) < 0) &&
      (how_close(victim->in_room, ch->in_room, distance) < 0))
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if( IS_AFFECTED(tmp, AFF_BLIND) || (tmp->specials.z_cord != ch->specials.z_cord) || (tmp == ch) )
    {
      continue;
    }
    if(CAN_SEE(tmp, ch))
      act("&+LA black two-dimensional door appears next to $n, who steps into it and vanishes along with the door.",
        FALSE, ch, 0, tmp, TO_VICT);
    else
      act("&+LA black two-dimensional door appears, then vanishes without a sound.",
        FALSE, ch, 0, tmp, TO_VICT);
    send_to_char(buf, tmp);
  }

  char_from_room(ch);
  char_to_room(ch, location, -1);

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if( IS_AFFECTED(tmp, AFF_BLIND) || (tmp->specials.z_cord != ch->specials.z_cord) || (tmp == ch) )
      continue;
    if(CAN_SEE(tmp, ch))
      act("&+LA black rift in space opens next to you, and&n $n &+Lsteps out of it grinning.&n",
        FALSE, ch, 0, tmp, TO_VICT);
    else
      act("&+LA black two-dimensional door appears, then vanishes without a sound.",
        FALSE, ch, 0, tmp, TO_VICT);
    send_to_char(buf, tmp);
  }
}

void spell_dark_compact(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int      location;
  /*
  if(!IS_TRUSTED(ch) && !IS_MAP_ROOM(ch->in_room))
  {
    send_to_char("You must be on the map to complete this incanation.\n", ch);
    return;
  }

  if(1)
  {
    send_to_char("this spell is currently disabled while working on new maps\n", ch);
    return;
  }

  if(!IS_TRUSTED(ch) && IS_PC(ch))
    CharWait(ch, 60);
  else
    CharWait(ch, 5);

  if(IS_SET(world[ch->in_room].room_flags, NO_TELEPORT))
  {
    send_to_char("The magic in this room prevents you from leaving.\n", ch);
    return;
  }
  if(world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("While swimming? I think not.\n", ch);
    return;
  }

  if(world[ch->in_room].zone != world[MAX(0, real_room(210000))].zone)
  {
    location = real_room0(number(210000, 214000));
    while ((world[location].sector_type == SECT_OCEAN) ||
           IS_SET(world[location].room_flags, NO_TELEPORT) ||
           IS_SET(world[location].room_flags, NO_GATE))
      location = real_room0(number(210000, 214000));
  }
  else
  {
    location = real_room0(number(140000, 159999));
    while ((world[location].sector_type == SECT_OCEAN) ||
           IS_SET(world[location].room_flags, NO_TELEPORT) ||
           IS_SET(world[location].room_flags, NO_GATE))
      location = real_room0(number(140000, 159999));
  }

  act("$n begins chanting softly...", FALSE, ch, 0, 0, TO_ROOM);
  act("&+rA blood red globe appears and $n steps into it.", FALSE, ch, 0, 0,
      TO_ROOM);
  send_to_char("&+rA blood red globe appears and you step into it.\n", ch);

  if(!number(0, 20) || (location == NOWHERE) ||
      !can_enter_room(ch, location, FALSE))
  {
    send_to_char("OH NO!!  Something has gone wrong!  You feel lost!\n", ch);
    spell_teleport(level, ch, 0, 0, ch, 0);
    return;
  }
  else
  {
    char_from_room(ch);
    char_to_room(ch, location, -1);
    act("&+rA blood red globe appears and $n steps out of it.", FALSE, ch, 0,
        0, TO_ROOM);
  }
  */
}

void spell_relocate(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(ch->in_room))
        return;
        
  if(GET_SPEC(ch, CLASS_SORCERER, SPEC_SHADOW))
    CharWait(ch, 4);
  else if(IS_PC(ch))
    CharWait(ch, 80);
  else
    CharWait(ch, 5);
    
  if(!can_relocate_to(ch, victim))
    return;

  act("&+W$n starts to become less solid &+Luntil $e fades into nothing!",
    FALSE, ch, 0, 0, TO_ROOM);
  act("&+WYou start to become less solid, then fade into nothing!",
    FALSE, ch, 0, 0, TO_CHAR);

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  char_from_room(ch);
  char_to_room(ch, victim->in_room, -1);

  act("&+WA coalescing of the ethereal substances causes your vision to blur...\n&+WWhen it at last clears&n $n &+Wstands before you!&n",
       FALSE, ch, 0, 0, TO_ROOM);
  act("&+WYou materialize elsewhere!&n",
    FALSE, ch, 0, 0, TO_CHAR);
}

void spell_group_teleport(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int from_room, dir, to_room, tries;
  P_char   targetChar;
  struct group_list *gl = 0;
  //int      range = get_property("spell.teleport.range", 30);
  int	range = 800000;
  if((ch && !is_Raidable(ch, 0, 0)) ||
     (victim && !is_Raidable(victim, 0, 0)))
  {
    send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
    return;
  }

  // make sure the room allows teleportation
  if(IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
      IS_HOMETOWN(ch->in_room) ||
      world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("The magic in this room prevents you from leaving.\n", ch);
    return;
  }

  // find a suitable room in the zone to teleport to
  if(IS_MAP_ROOM(ch->in_room))
  {
    to_room = ch->in_room;

    for( int i = 0; i < range; i++ )
    {
      tries = 0;
       do
    {
      to_room = number(zone_table[world[ch->in_room].zone].real_bottom,
                       zone_table[world[ch->in_room].zone].real_top);
      tries++;
    }
    while ((IS_SET(world[to_room].room_flags, PRIVATE) ||
          IS_SET(world[to_room].room_flags, PRIV_ZONE) ||
          IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
          IS_HOMETOWN(to_room) ||
          world[to_room].sector_type == SECT_OCEAN) && tries < 1000);
    }
/*
do
      {
        dir = number(0, 3);
      } while( tries++ < 20 && !VALID_TELEPORT_EDGE(to_room, dir, ch->in_room) );

      if( tries < 20 )
        to_room = TOROOM(to_room, dir);
*/
  }
  else
  {
    tries = 0;
    do
    {
      to_room = number(zone_table[world[ch->in_room].zone].real_bottom,
                       zone_table[world[ch->in_room].zone].real_top);
      tries++;
    }
    while ((IS_SET(world[to_room].room_flags, PRIVATE) ||
          IS_SET(world[to_room].room_flags, PRIV_ZONE) ||
          IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
          IS_HOMETOWN(to_room) ||
          world[to_room].sector_type == SECT_OCEAN) && tries < 1000);
  }

  // if no suitable room was found, teleport back to the same room they're in
  if(tries == 1000)
    to_room = ch->in_room;

/*
  // if this zone limits teleports, check to see if the teleportation range is too great
  if(LIMITED_TELEPORT_ZONE(ch->in_room))
  {
    if(how_close(ch->in_room, to_room, 5))
      send_to_char
        ("The magic gathers, but somehow fades away before taking effect.\n",
         ch);
    return;
  }
*/
  // get the room the teleport is taking place so we don't have to move the teleporter last
  from_room = ch->in_room;

  // if the teleporter is grouped
  if(ch->group)
  {

    // get the character's group list
    gl = ch->group;

    // teleport the group members in the character's room
    for (gl; gl; gl = gl->next)
    {
      if(gl->ch->in_room == from_room)
      {

        // show the room they're fading
        act("$n slowly fades out of existence.", FALSE, gl->ch, 0, 0,
            TO_ROOM);

        // if they're fighting, break it up
        if(IS_FIGHTING(gl->ch))
          stop_fighting(gl->ch);
        if(IS_DESTROYING(gl->ch))
          stop_destroying(gl->ch);

        // move the char
        char_from_room(gl->ch);
        char_to_room(gl->ch, to_room, -1);

        // show the new room they've arrived
        act("$n slowly fades into existence.", FALSE, gl->ch, 0, 0, TO_ROOM);
      }
    }
  }
  else
  {
    // show the room they're fading
    act("$n slowly fades out of existence.", FALSE, ch, 0, 0, TO_ROOM);

    // if they're fighting, break it up
    if(IS_FIGHTING(ch))
      stop_fighting(ch);
    if(IS_DESTROYING(ch))
      stop_destroying(ch);

    // move the char
    char_from_room(ch);
    char_to_room(ch, to_room, -1);

    // show the new room they've arrived
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  }
}

void spell_teleport(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int from_room, dir, to_room;
  P_char   vict, t_ch;

  if((IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
      IS_HOMETOWN(ch->in_room) ||
      world[ch->in_room].sector_type == SECT_OCEAN) && level < 60)
  {
    send_to_char("The magic in this room prevents you from leaving.\n", ch);
    return;
  }

  if( !victim )
  {
    vict = ch;
  }
  else
  {
    vict = victim;
  }
  if( (ch && !is_Raidable(ch, 0, 0)) || (victim && !is_Raidable(victim, 0, 0)) )
  {
    send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
    return;
  }

  int range = get_property("spell.teleport.range", 30);
  to_room = vict->in_room;
  if( IS_MAP_ROOM(vict->in_room) )
  {
    for( int i = 0; i < range; i++ )
    {
      int tries = 0;
      dir = number(0,3);
      do
      {
        // Give a 67% chance to continue on the direction chosen
        dir = number(0,2) ? dir : number(0,3);
      } while( tries++ < 15 && !VALID_TELEPORT_EDGE(to_room, dir, vict->in_room) );

      if( tries < 10 )
        to_room = TOROOM(to_room, dir);
    }
  }
  else
  {
    int tries = 0;
    do
    {
      to_room = number(zone_table[world[vict->in_room].zone].real_bottom,
          zone_table[world[vict->in_room].zone].real_top);
      tries++;
    }
    while ((IS_SET(world[to_room].room_flags, PRIVATE) ||
          IS_SET(world[to_room].room_flags, PRIV_ZONE) ||
          IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
          IS_HOMETOWN(to_room) ||
          world[to_room].sector_type == SECT_OCEAN) && tries < 1000);
    if(tries >= 1000)
      to_room = vict->in_room;
  }


  if(LIMITED_TELEPORT_ZONE(vict->in_room))
  {
    if( how_close(vict->in_room, to_room, 5) )
    {
      send_to_char("The magic gathers, but somehow fades away before taking effect.\n", vict);
    }
    return;
  }
  act("$n slowly fades out of existence.", FALSE, vict, 0, 0, TO_ROOM);
  if(IS_FIGHTING(vict))
    stop_fighting(vict);
  if(IS_DESTROYING(vict))
    stop_destroying(vict);
  if(vict->in_room != NOWHERE)
    for (t_ch = world[vict->in_room].people; t_ch; t_ch = t_ch->next)
      if(IS_FIGHTING(t_ch) && (t_ch->specials.fighting == vict))
        stop_fighting(t_ch);
  if(vict->in_room != to_room) {
#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif
    char_from_room(vict);
    char_to_room(vict, to_room, -1);
  }
  act("$n slowly fades into existence.", FALSE, vict, 0, 0, TO_ROOM);
}

/* flag indicates if it's a guild door, 1 if outside door, 2 if
   inside door, other is normal teleporter */

void teleport_to(P_char ch, int to_room, int flag)
{
  if(to_room == NOWHERE)
  {
    send_to_char
      ("teleport_to(): tried to dump you in NOWHERE.  tell a god.\n", ch);
    return;
  }
  if(flag == 1)
    flag = flag;
/*    act("$n enters a nearby building through the doorway.", FALSE, ch, 0, 0, TO_ROOM); */
/*  else if(flag == 2)
    act("$n leaves the building through the doorway.", FALSE, ch, 0, 0, TO_ROOM); */
  else
    act("$n slowly fades out of existence.", FALSE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, to_room, 0);

  /* we'd hate to get stuck in an infinite loop, wouldn't we?  4 isn't currently
     used as a flag value */

  if(flag == 1)
    act("$n enters the building through the doorway.", FALSE, ch, 0, 0,
        TO_ROOM);
  else if(flag == 2)
    act("$n exits from a nearby building through the doorway.", FALSE, ch, 0,
        0, TO_ROOM);
  else
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
}

void spell_lifelust(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_LIFELUST))
  {
    bzero(&af, sizeof(af));
    send_to_char("&+LYou begin to drool... &+rmust kill the living!\n",
                 victim);
    af.type = SPELL_LIFELUST;
    af.duration = MAX(5, level / 2);
    af.modifier = ((int) (level / 25)) + 2;
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);
    af.location = APPLY_DAMROLL;
    af.modifier = ((int) (level / 25)) + 2;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_LIFELUST)
      {
        af1->duration = MAX(5, level / 2);
      }
  }
}

void spell_aid(int level, P_char ch, char *arg, int type, P_char victim,
               P_obj obj)
{
  struct affected_type *af, *next_af_dude;
  int temp = (int) (level / 10);

  if(!ch)
  {
    logit(LOG_EXIT, "spell_aid called in magic.c with no ch");
    raise(SIGSEGV);
  }
  if(!victim)
  {
    return;
  }
  if(ch &&
    victim) // Just making sure.
  {
    for (af = victim->affected; af; af = next_af_dude)
    {
      next_af_dude = af->next;
      
      if(af->flags & AFFTYPE_SHORT)
      {
        continue;
      }      
      if((af->type == SPELL_CURSE) ||
        (af->type == SPELL_APOCALYPSE) ||
        (af->type == SPELL_BLINDNESS) ||
        (af->type == SPELL_DISEASE) ||
        (af->type == SPELL_MINOR_PARALYSIS) ||
        (af->type == SPELL_MAJOR_PARALYSIS) ||
        (af->type == SPELL_RAY_OF_ENFEEBLEMENT))
      {
        af->duration = MAX(1, af->duration - (2 * (1 + temp)));
      }
    }
   // 1 in 6 chance to instantly cure the poison at level 50
    if(IS_SET(victim->specials.affected_by2, AFF2_POISONED) && !number(0,5))
    {
      if(poison_common_remove(victim))
      {
        act("&+WYou neutralize the poison!", FALSE, ch, 0, 0, TO_CHAR);
        act("&+WThe poison in your bloodstream disappears!", FALSE, 0, 0, victim, TO_VICT);
      }
    } 
    if(GET_LEVEL(ch) > 16)
    {
      spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
      grapple_heal(victim);
    }
    if(!affected_by_spell(victim, SPELL_AID))
    {
      struct affected_type af;
      bzero(&af, sizeof(af));
      send_to_char("&+WYou suddenly feel blessed by the protective spirits of the nature!\n", victim);
      af.type = SPELL_AID;
      af.duration = MAX(3, (level + 4 / 10));
      affect_to_char(victim, &af);
    }
    send_to_char("&+GYou feel the power of nature course through your veins.\n", victim);
  }
}

void spell_bless(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(obj)
  {
    if((5 * level > (GET_OBJ_WEIGHT(obj) / 4)) && (!IS_FIGHTING(ch)))
    {
      act("&+W$p briefly glows.", FALSE, ch, obj, 0, TO_CHAR);
      set_obj_affected_extra(obj, -1, SPELL_BLESS, 50, ITEM2_BLESS);
      if (obj->type == ITEM_DRINKCON)
      {
        if (RACE_GOOD(ch))
          obj->value[2] = LIQ_HOLYWATER;
        else if (RACE_EVIL(ch))
          obj->value[2] = LIQ_UNHOLYWAT;
      }
    }
  }
  else
  {

    // play_sound(SOUND_BLESS, NULL, ch->in_room, TO_ROOM);

    if(!affected_by_spell(victim, SPELL_BLESS))
    {

      bzero(&af, sizeof(af));
      send_to_char("&+WYou suddenly feel blessed!\n", victim);
      af.type = SPELL_BLESS;
      af.duration = MAX(5, level / 2);
      af.modifier = ((int) (level / 20)) + 1;
      af.location = APPLY_HITROLL;
      affect_to_char(victim, &af);

      af.location = APPLY_SAVING_SPELL;
      af.modifier = -((int) (level / 30) + 1);  /* Make better */
      affect_to_char(victim, &af);
    }
    else
    {
      struct affected_type *af1;

      for (af1 = victim->affected; af1; af1 = af1->next)
        if(af1->type == SPELL_BLESS)
        {
          af1->duration = MAX(5, level / 2);
        }
    }
  }
}

/* Seeya!
void spell_healing_blade(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  P_obj    wpn;

  if(affected_by_spell(ch, SPELL_HEALING_BLADE))
  {
    send_to_char("You are already on &+Rfire!!\n", ch);
    return;
  }

  if(!(wpn = ch->equipment[WIELD]) || !IS_SWORD(wpn))
  {
    send_to_char("You need to be wielding a slashing weapon!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_HEALING_BLADE;
  af.duration = level;
  af.location = APPLY_HITROLL;
  af.modifier = number(1, 4);

  affect_to_char(victim, &af);

  send_to_char("&+BA blue aura covers your blade with a healing power!&n\n",
               ch);

}
*/

void spell_blindness(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;

  if(GET_STAT(ch) == STAT_DEAD)
    return;
/*
  if(affected_by_spell(victim, SPELL_BLINDNESS))
    return;
*/

  if(IS_TRUSTED(victim))
    return;

  if(resists_spell(ch, victim))
    return;

  /*
   * negative level negates save
   */

  if((level < 0) || !saves_spell(victim, SAVING_SPELL))
  {
    blind(ch, victim, 75 * WAIT_SEC);
    return;
  }
  if(IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
    appear(ch);

  if(IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if( !IS_FIGHTING(victim) && !IS_DESTROYING(victim) )
      MobStartFight(victim, ch);
  }
}

void spell_control_weather(int level, P_char ch, P_char victim, P_obj obj)
{
  /* Control Weather is not possible here!!! */
  /* Better/Worse can not be transferred */
  /* (e.g. Its done in spells.c instead) */
}

void spell_minor_creation(int level, P_char ch, P_char victim, P_obj obj)
{
  SET_BIT(obj->extra2_flags, ITEM2_STOREITEM);
  obj_to_room(obj, ch->in_room);
  obj->z_cord = ch->specials.z_cord;
  act("$p &+Wsuddenly appears.", TRUE, ch, obj, 0, TO_ROOM);
  act("$p &+Wsuddenly appears.", TRUE, ch, obj, 0, TO_CHAR);
}


void spell_pulchritude(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;
  int      temp;

  temp = (int) (level / 10);
  if(affected_by_spell(victim, SPELL_PULCHRITUDE))
  {
    send_to_char("&+CYou can't get much prettier than this!\n", victim);
    return;
  }
  act("&+CYour features soften a bit, you feel friendlier.", FALSE, victim, 0,
      0, TO_CHAR);

  bzero(&af, sizeof(af));

  af.type = SPELL_PULCHRITUDE;
  af.duration =  level;
  af.modifier = 5 * (1 + temp);
  af.location = APPLY_CHA;

  affect_join(victim, &af, TRUE, FALSE);
}

void spell_flame_blade(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  P_obj    blade;

  blade = read_object(real_object(366), REAL);
  if(!blade)
  {
    logit(LOG_DEBUG, "spell_flame_blade(): obj 366 not loadable");
    return;
  }

  blade->extra_flags |= ITEM_NORENT;
  blade->bitvector = 0;
  /* how about some gay de procs for flame blade? Yeah baby! */
  if(GET_LEVEL(ch) >= 21)
      {
          blade->value[5] = 195;
          blade->value[6] = GET_LEVEL(ch);
          blade->value[7] = 40; //procs flameburst
      }
  if(GET_LEVEL(ch) >= 26)
      {
      SET_BIT(blade->bitvector, AFF_PROT_FIRE);
      }
  if(GET_LEVEL(ch) >= 31)
      {
      SET_BIT(blade->bitvector2, AFF2_FIRESHIELD);
      }
  if(GET_LEVEL(ch) >= 36)
      {
          blade->value[5] = 26;
          blade->value[6] = GET_LEVEL(ch);
          blade->value[7] = 40; //procs fireball
      }
  if(GET_LEVEL(ch) >= 41)
      {
      blade->value[5] = 21;
          blade->value[6] = GET_LEVEL(ch);
          blade->value[7] = 40; //procs flamestrike
      }
  if(GET_LEVEL(ch) >= 51)
      {
          blade->value[5] = 124;
          blade->value[6] = GET_LEVEL(ch);
          blade->value[7] = 40; //procs sunray
      }
  if(GET_LEVEL(ch) >= 56)
      {
      SET_BIT(blade->bitvector2, AFF2_FIRE_AURA);
      }
  act("$p &+Warrives in a burst of &n&+rfire.", TRUE, ch, blade, 0, TO_ROOM);
  act("$p &+Warrives in a burst of &n&+rfire.", TRUE, ch, blade, 0, TO_CHAR);
  blade->timer[0] = 180;
  if (IS_PC(ch))
    blade->timer[1] = GET_PID(ch);
  else
    blade->timer[1] = -1;

  obj_to_char(blade, ch);
}

void spell_serendipity(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;
  int      skl_lvl;

  skl_lvl = (int) (level / 10);

  if(affected_by_spell(victim, SPELL_SERENDIPITY))
  {
    send_to_char("&+CYou can't get much luckier than this!\n", victim);
    return;
  }
  send_to_char("&+CYou feel much luckier.\n", victim);

  bzero(&af, sizeof(af));

  af.type = SPELL_SERENDIPITY;
  af.duration =  level;
  af.modifier = 3 * (1 + skl_lvl);
  af.location = APPLY_LUCK;

  affect_join(victim, &af, TRUE, FALSE);

}

void spell_shield(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  P_obj    shield;

  shield = read_object(real_object(368), REAL);
  /*
     if(!hammer) {
     logit(LOG_DEBUG, "spell_shield(): obj 368 not loadable");
     return;
     }
   */
  if(!shield)
  {
    logit(LOG_DEBUG, "spell_shield(): obj 368 not loadable");
    return;
  }
  act("$p &+Wslowly materializes.", TRUE, ch, shield, 0, TO_ROOM);
  act("$p &+Wslowly materializes.", TRUE, ch, shield, 0, TO_CHAR);
  shield->timer[0] = 180;

  obj_to_char(shield, ch);
}

void spell_create_food(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  P_obj    food;

  food = read_object(real_object(364), REAL);

  if(!food)
  {
    logit(LOG_DEBUG, "spell_create_food(): obj 364 not loadable");
    return;
  }
  act("$p &+Wsuddenly appears.", TRUE, ch, food, 0, TO_ROOM);
  act("$p &+Wsuddenly appears.", TRUE, ch, food, 0, TO_CHAR);

  SET_BIT(food->extra_flags, ITEM_NOSELL);
  obj_to_room(food, ch->in_room);

}

void spell_create_water(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int      water;

  if(GET_ITEM_TYPE(obj) == ITEM_DRINKCON)
  {
    if((obj->value[2] != LIQ_WATER) && (obj->value[1] != 0))
    {

      name_from_drinkcon(obj);
      obj->value[2] = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);

    }
    else
    {
      water = 2 * level;

      /* Calculate water it can contain, or water created */
      water = MIN(obj->value[0] - obj->value[1], water);

      if(water > 0)
      {
        obj->value[2] = LIQ_WATER;
        obj->value[1] += water;

        weight_change_object(obj, water);

        name_from_drinkcon(obj);
        name_to_drinkcon(obj, LIQ_WATER);
        act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
  else
  {
    send_to_char("It is unable to hold water.\n", ch);
    return;
  }
}

void spell_cure_blind(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  if(IS_AFFECTED(victim, AFF_BLIND))
  {
    affect_from_char(victim, SPELL_BLINDNESS);
    REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
    send_to_char("&+WYour vision returns!\n", victim);
  }
}

void spell_wandering_woods(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct room_affect af;
  int      temp;

  if(GET_LEVEL(ch) < 50)
  {
    temp = 2;
  }
  else if((GET_LEVEL(ch) >= 50) && (GET_LEVEL(ch) <= 55))
  {
    temp = 3;
  }
  else if((GET_LEVEL(ch) == 56))
  {
    temp = 4;
  }
  else
  {
    temp = 4;
  }

  if(!ch)
    return;
  if(!
      (world[ch->in_room].sector_type == SECT_FOREST ||
       world[ch->in_room].sector_type == SECT_SWAMP) ||
      get_spell_from_room(&world[ch->in_room], SPELL_WANDERING_WOODS))
  {
    send_to_char("Nothing happens.\n", ch);
    return;
  }
  send_to_char
    ("&+GThe wilderness around you comes alive for just a moment!\n", ch);
  act("&+GThe land around you becomes dark and mysterious!", 0, ch, 0, 0,
      TO_ROOM);

  memset(&af, 0, sizeof(struct room_affect));
  af.type = SPELL_WANDERING_WOODS;
  af.duration = (1 + temp) * 40;
  af.ch = ch;
  affect_to_room(ch->in_room, &af);
}

void spell_consecrate_land(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct room_affect *raf;
  struct room_affect af;

  if(!ch || get_spell_from_room(&world[ch->in_room], SPELL_CONSECRATE_LAND))
    return;

  if( (raf = get_spell_from_room(&world[ch->in_room], SPELL_DESECRATE_LAND)) )
  {
    affect_room_remove(ch->in_room, raf);
    send_to_char("&+YYou destroy the &+Crunes&+Y laying about the area.&n\r\n", ch);
    act("&+Y$n&+Y's prayer shatters the &+Crunes&+Y laying around the area.&n",0, ch, 0, 0, TO_ROOM);
    return;
  }

  switch (world[ch->in_room].sector_type)
  {
  case SECT_INSIDE:
  case SECT_UNDRWLD_INSIDE:
    send_to_char("Try again, OUTDOORS this time.\r\n", ch);
    return;
    break;
  case SECT_CITY:
  case SECT_ROAD:
  case SECT_CASTLE_WALL:
  case SECT_CASTLE_GATE:
  case SECT_UNDRWLD_CITY:
  case SECT_CASTLE:
    send_to_char("Nothing happens.  Perhaps you need to be farther outdoors...\r\n", ch);
    return;
    break;
  case SECT_SWAMP:
  case SECT_UNDRWLD_SLIME:
  case SECT_FIELD:
  case SECT_FOREST:
  case SECT_HILLS:
  case SECT_MOUNTAIN:
  case SECT_UNDRWLD_WILD:
  case SECT_UNDRWLD_MUSHROOM:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_DESERT:
    send_to_char("&+GYour god &+Wblesses &+Gthis area with the awesome power of &+Ctranquility&+G.&n\r\n", ch);
    act("&+G$n&+G's prayer brings a &+Ctranquil &+Wlight &+Gfrom the &+WHe&N&+waven&+Ws.&n",0, ch, 0, 0, TO_ROOM);
    memset(&af, 0, sizeof(struct room_affect));
    af.type = SPELL_CONSECRATE_LAND;
    af.duration = (GET_LEVEL(ch) * 4);
    af.ch = ch;
    affect_to_room(ch->in_room, &af);
    break;
  case SECT_PLANE_OF_AVERNUS:
    send_to_char("There's far too much vileness for your meager will to overcome.\r\n", ch);
    return;
    break;
  case SECT_NO_GROUND:
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_UNDRWLD_NOSWIM:
  case SECT_UNDRWLD_WATER:
  case SECT_FIREPLANE:
  case SECT_UNDRWLD_LIQMITH:
  case SECT_NEG_PLANE:
  case SECT_UNDERWATER:
  case SECT_UNDRWLD_NOGROUND:
  case SECT_UNDERWATER_GR:
  case SECT_OCEAN:
    send_to_char("Consecrate _LAND_...  There is no land here!\r\n", ch);
    return;
  break;
  default:
    logit(LOG_DEBUG, "Bogus sector_type (%d) in consecrate_land",
          world[ch->in_room].sector_type);
    send_to_char("How strange!  This terrain doesn't seem to exist!\r\n", ch);
    return;
    break;
  }
}

void spell_summon_insects(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
    P_obj    t_obj, next_obj;
    P_obj    used_obj = NULL;
    struct   room_affect af;
    int      count, i;

    if(!ch || get_spell_from_room(&world[ch->in_room], SPELL_SUMMON_INSECTS))
        return;


    for (count = 0, t_obj = ch->carrying; t_obj; t_obj = next_obj)
    {
      next_obj = t_obj->next_content;

      if( obj_index[t_obj->R_num].virtual_number == VOBJ_INGRED_MANDRAKE )
      {
        used_obj = t_obj;
        break;
      }

      if( ++count > 1000 )
        break;
    }

    if(!used_obj)
    {
      send_to_char("You must have &+ya mandrake root&n in your inventory.\r\n",ch);
      return;
    }

    extract_obj(used_obj);

    send_to_char("&+yYou summon the &+minsects&+y of the area.&n\n", ch);
    act("&+y$n sprinkles some food around to summon the &+minsects&+y of the area.&n\n", 0,
            ch, 0, 0, TO_ROOM);

    memset(&af, 0, sizeof(struct room_affect));
    af.type = SPELL_SUMMON_INSECTS;
    af.duration = 250;
    af.ch = ch;
    affect_to_room(ch->in_room, &af);
}

void spell_binding_wind(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct room_affect af;
        struct room_affect *afp;

  if(!ch || get_spell_from_room(&world[ch->in_room], SPELL_BINDING_WIND))
    return;

  if(!OUTSIDE(ch))
  {
        send_to_char("&+CYou cannot manipulate winds where there are none! Next time try it outside.\n", ch);
    return;
  }

  send_to_char("&+CYou summon up the wind speed so that it is hard to move!\n", ch);
  act("&+C$n waves his hands and the wind picks up!&n", 0, ch, 0, 0, TO_ROOM);

  memset(&af, 0, sizeof(struct room_affect));
  af.type = SPELL_BINDING_WIND;
  af.duration = 250;
  af.ch = ch;
  affect_to_room(ch->in_room, &af);

  if(afp = get_spell_from_room(&world[ch->in_room], SPELL_WIND_TUNNEL))
  {
    affect_room_remove(ch->in_room, afp);
  }
  return;
}

void spell_wind_tunnel(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct room_affect af;
  struct room_affect *afp;

  send_to_char("&+cYou summon up the wind speed so that its easy to move!\n", ch);
  act("&+c$n waves his hands and the wind picks up! Hey, it's easier to move!&n", 0, ch, 0, 0, TO_ROOM);

  memset(&af, 0, sizeof(struct room_affect));
  af.type = SPELL_WIND_TUNNEL;
  af.duration = 250;
  af.ch = ch;
  affect_to_room(ch->in_room, &af);

  if(afp = get_spell_from_room(&world[ch->in_room], SPELL_BINDING_WIND))
  {
    affect_room_remove(ch->in_room, afp);
  }
  return;
}

void spell_vitalize_mana(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  int      trans;

  trans = MIN(GET_VITALITY(ch), number(33, GET_MAX_MANA(ch)));

  act("You feel more energized!", FALSE, ch, 0, 0, TO_CHAR);

  ch->points.mana += trans;
  ch->points.mana = MIN(GET_MANA(ch), GET_MAX_MANA(ch));
  ch->points.vitality -= trans;
}

void spell_rejuvenate_minor(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj tar_obj)
{
  struct affected_type af;
  int      t_age;

  act("You feel younger.", FALSE, victim, 0, 0, TO_CHAR);
  t_age = dice(2, level) / 2;

  bzero(&af, sizeof(af));

  af.type = SPELL_REJUVENATE_MINOR;
  af.duration =  t_age;
  af.modifier = -t_age;

  af.location = APPLY_AGE;

  affect_join(victim, &af, TRUE, FALSE);
  
  af.location = APPLY_MOVE_REG;
  af.modifier = BOUNDED(1, GET_LEVEL(ch) / 10, 5);
  affect_to_char(victim, &af);

  af.location = APPLY_HIT_REG;
  af.modifier = GET_LEVEL(ch) / 5;
  affect_to_char_with_messages(victim, &af, "You feel older again.", "$n suddenly looks a little older than a moment before.");
}

void spell_ray_of_enfeeblement( int level, P_char ch, char *arg, int type, P_char victim, P_obj obj )
{
  struct affected_type af;
  int base_str = victim->base_stats.Str;
  int curr_str = GET_C_STR(victim);
  int mod = level;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  if( victim == ch )
  {
    send_to_char("You may not enfeeble yourself.", ch);
    return;
  }

  if( resists_spell(ch, victim) )
    return;

  if( IS_ELITE(victim) || IS_GREATER_RACE(victim) )
    return;

  if( affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT) )
  {
    act("$E is already pretty feeble.", TRUE, ch, 0, victim, TO_CHAR);
    act("&+LYou cannot possible get more feeble!&n", TRUE, ch, 0, victim, TO_VICT);
    return;
  }

  if( affected_by_spell(victim, SPELL_WITHER) )
  {
    act("$E is withered and unaffected by enfeeblment.", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if( mod > base_str )
    mod = base_str;

  if( mod > curr_str )
    mod = curr_str;

  if( level < 1 )
    level = 1;

  mod = (int)(mod * 0.50);

  int success_chance = BOUNDED(50, 70 + GET_LEVEL(ch) - GET_LEVEL(victim), 80);

  if( success_chance > number(1, 100) )
  {
    send_to_char("A wave of weakness sweeps over you!\n", victim);
    act("$n pales, and seems to sag.", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));
    af.type = SPELL_RAY_OF_ENFEEBLEMENT;

    if(!NewSaves(victim, SAVING_PARA, 0))
    {
      af.duration = 2;
      af.modifier = -(mod);
      af.location = APPLY_STR;
      affect_to_char(victim, &af);

      af.modifier = -1 * level / 10 - number(1, 6);
      // Don't go below 0 or it'll jump up to 255 'cause damroll is an unsigned byte.
      if( af.modifier < 0 - victim->points.damroll )
        af.modifier = 0 - victim->points.damroll;
      af.location = APPLY_DAMROLL;
      affect_to_char(victim, &af);
    }
    else
    {
      af.duration = 1;
      af.modifier = -1 * mod / 10 - number(1, 6);
      // Don't go below 0 or it'll jump up to 255 'cause damroll is an unsigned byte.
      if( af.modifier < 0 - victim->points.damroll )
        af.modifier = 0 - victim->points.damroll;
      af.location = APPLY_STR;
      affect_to_char(victim, &af);

      af.modifier = MIN(-1, (int)(-1 * level / 10 - number(0, 5)));
      af.location = APPLY_DAMROLL;
      affect_to_char(victim, &af);
    }
  }
  if(IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if( !IS_FIGHTING(victim) && !IS_DESTROYING(victim) )
      MobStartFight(victim, ch);
  }
}

void AgeChar(P_char ch, int years)
{
  long     played, secs;
  time_t   curr_time = time(NULL);

  if(IS_NPC(ch))
    return;

  secs = years * SECS_PER_MUD_YEAR;
  played = curr_time - ch->player.time.birth;
  ch->player.time.birth = curr_time - (played + secs);
  if(ch->player.time.birth > curr_time)
    ch->player.time.birth = curr_time;
}

void spell_age(int level, P_char ch, char *arg, int type, P_char victim,
               P_obj obj)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  
  if(!(ch) ||
    !(victim) ||
    !IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
      return;

  if(IS_NPC(victim))
    return;
  
  if((IS_PC(ch) ||
      IS_PC_PET(ch))  &&
      IS_PC(victim))
  {
    if(!is_linked_to(ch, victim, LNK_CONSENT) &&
      (ch != victim) &&
      !IS_TRUSTED(ch))
    {
      sprintf(Gbuf1, "%s has not given %s consent to you.\n",
              GET_NAME(victim), HSHR(victim));
      send_to_char(Gbuf1, ch);
      return;
    }
  }
  
  AgeChar(victim, dice(2, 8));
  send_to_char("You feel a bit older all of a sudden!\n", victim);
}

void spell_rejuvenate_major(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj tar_obj)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  
  if(!(ch) ||
    !(victim) ||
    !IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
      return;

  if(IS_NPC(victim))
    return;

  if(GET_RACE(victim) == RACE_ILLITHID)
    return;

  if(!is_linked_to(ch, victim, LNK_CONSENT) &&
    (ch != victim) &&
    !IS_TRUSTED(ch))
  {
    sprintf(Gbuf1, "%s has not given %s consent to you.\n",
            GET_NAME(victim), HSHR(victim));
    send_to_char(Gbuf1, ch);
    return;
  }
  AgeChar(victim, -(dice(2, 4)));
  send_to_char("You feel a bit younger all of a sudden!\n", victim);
}

void spell_cure_serious(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int      healpoints;

  healpoints = dice(3, 8);
  heal(victim, ch, healpoints, GET_MAX_HIT(victim));
  // healCondition(victim, healpoints);

  send_to_char("&+WYou feel a lot better!\n", victim);

  update_pos(victim);
}

void spell_cure_critic(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      healpoints;

  healpoints = dice(3, 10) + 10;
  heal(victim, ch, healpoints, GET_MAX_HIT(victim));
  // healCondition(victim, healpoints);
  send_to_char("&+WYou feel MUCH better!\n", victim);
  update_pos(victim);
}

void spell_cure_light(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  int      healpoints;

  healpoints = number(2, 10);
  heal(victim, ch, healpoints, GET_MAX_HIT(victim));
  // healCondition(victim, healpoints);
  update_pos(victim);
  send_to_char("&+WYou feel a little better!\n", victim);
}

void spell_curse(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!victim)
    victim = ch;

  if(victim)
    if(!IS_TRUSTED(ch) && resists_spell(ch, victim))
      return;

  if(IS_TRUSTED(victim) || affected_by_spell(victim, SPELL_CURSE))
  {
    send_to_char("Aren't they already cursed enough?\r", ch);
    return;
  }

  if(obj)
  {
    SET_BIT(obj->extra_flags, ITEM_NODROP);
    /* LOWER ATTACK DICE BY -1 */
    if(obj->type == ITEM_WEAPON)
      obj->value[2]--;
    if(obj->value[2] < 1)
      obj->value[2] = 1;
    act("&+r$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
  }
  else if(IS_GREATER_RACE(victim) || IS_ELITE(victim))
  {
     if(NewSaves(victim, SAVING_SPELL, 5))
     send_to_char("Your victim has saved against your curse spell!&n\n", ch);
       return;
  }
  else if(IS_NPC(victim) && NewSaves(victim, SAVING_SPELL, 10))
  {
   send_to_char("Your victim has saved against your curse spell!&n\n", ch);
      return;
  }
  else if(IS_PC(victim) && NewSaves(victim, SAVING_SPELL, 0))
  {
    send_to_char("Your victim has saved against your curse spell!&n\n", ch);
     return;
  }
  else
  {

    bzero(&af, sizeof(af));

    af.type = SPELL_CURSE;
    af.duration = GET_LEVEL(ch);
    af.modifier = (IS_NPC(victim) ? -10 : -5);
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);
    af.modifier = 10;
    af.location = APPLY_CURSE;
    affect_to_char(victim, &af);

    act("&+r$n &+rbriefly reveals a red aura!", FALSE, victim, 0, 0, TO_ROOM);
    act("&+rYou suddenly feel very uncomfortable.", FALSE, victim, 0, 0, TO_CHAR);
  }
}
void spell_nether_touch(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!victim)
    victim = ch;

  if((GET_RACE(victim) == RACE_DRAGON) ||
      (GET_RACE(victim) == RACE_DEVIL) ||
      (GET_RACE(victim) == RACE_UNDEAD) ||
      (GET_RACE(victim) == RACE_GHOST) ||
      (GET_RACE(victim) == RACE_DEMON) || (GET_RACE(victim) == RACE_PLANT))
  {
    send_to_char("You sense that they would be unaffected by your touch.\n",
                 ch);
    return;
  }

  if(saves_spell(victim, SAVING_SPELL) ||
      affected_by_spell(victim, SPELL_NETHER_TOUCH))
    return;

  bzero(&af, sizeof(af));

  af.type = SPELL_NETHER_TOUCH;
  af.duration =  100;
  af.flags = AFFTYPE_SHORT;
  af.modifier = -3;
  af.location = APPLY_HITROLL;
  affect_to_char(victim, &af);
  af.modifier = 10;
  af.location = APPLY_CURSE;
  affect_to_char(victim, &af);
  af.modifier = 2;
  af.location = APPLY_SAVING_PARA;
  affect_to_char(victim, &af);

  act("&+r$n briefly reveals a &+Lblack&n&+r aura!", FALSE, victim, 0, 0,
      TO_ROOM);
  act("&+rYou get a strong sinking feeling.", FALSE, victim, 0, 0, TO_CHAR);
}

void spell_detect_evil(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_DETECT_EVIL))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_DETECT_EVIL)
      {
        af1->duration = KludgeDuration(ch, level, level);
      }
    return;
  }
  bzero(&af, sizeof(af));

  af.type = SPELL_DETECT_EVIL;
  af.duration = KludgeDuration(ch, level, level);
  af.bitvector2 = AFF2_DETECT_EVIL;

  affect_to_char(victim, &af);

  send_to_char("&+rYour eyes tingle.\n", victim);
}

void spell_detect_good(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_DETECT_GOOD))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_DETECT_GOOD)
      {
        af1->duration = KludgeDuration(ch, level, level);
      }
    return;
  }
  bzero(&af, sizeof(af));

  af.type = SPELL_DETECT_GOOD;
  af.duration = KludgeDuration(ch, level, level);
  af.bitvector2 = AFF2_DETECT_GOOD;

  affect_to_char(victim, &af);

  send_to_char("&+YYour eyes tingle.\n", victim);
}

void spell_detect_magic(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_DETECT_MAGIC))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_DETECT_MAGIC)
      {
        af1->duration = KludgeDuration(ch, level, level);
      }
    return;
  }
  bzero(&af, sizeof(af));
  af.type = SPELL_DETECT_MAGIC;
  af.duration = KludgeDuration(ch, level, level);
  af.bitvector2 = AFF2_DETECT_MAGIC;

  affect_to_char(victim, &af);
  send_to_char("&+bYour eyes tingle.\n", victim);
}

void spell_detect_invisibility(int level, P_char ch, char *arg, int type,
                               P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_DETECT_INVISIBLE))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_DETECT_INVISIBLE)
      {
        af1->duration = MAX(level, 10);
      }
    return;
  }
  bzero(&af, sizeof(af));
  af.type = SPELL_DETECT_INVISIBLE;
  af.duration = MAX(level, 10);
  af.bitvector = AFF_DETECT_INVISIBLE;

  affect_to_char(victim, &af);

  send_to_char("&+WYour eyes tingle.\n", victim);
}

void spell_detect_poison(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  if(victim)
  {
    if(victim == ch)
      if(IS_SET(ch->specials.affected_by2, AFF2_POISONED))
        send_to_char("&+GYou can sense poison in your blood.\n", ch);
      else
        send_to_char("&+WYou feel healthy.\n", ch);
    else if(IS_SET(victim->specials.affected_by2, AFF2_POISONED))
    {
      act("&+GYou sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
    }
    else
    {
      act("&+WYou sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }
  else
  {                             /*
                                 * It's an object
                                 */
    if((obj->type == ITEM_DRINKCON) || (obj->type == ITEM_FOOD))
    {
      if(obj->value[3])
        act("&+GPoisonous fumes are revealed.", FALSE, ch, 0, 0, TO_CHAR);
      else
        send_to_char("&+WIt looks very delicious.\n", ch);
    }
  }
}

void spell_unmaking(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      clevel;
  P_obj    cobj, next_obj;

  if(!OBJ_IN_ROOM(obj, ch->in_room))
  {
    send_to_char("Corpse is nowhere to be found, tell a god.\n", ch);
    return;
  }
  if(GET_ITEM_TYPE(obj) == ITEM_CORPSE)
  {
    clevel = obj->value[2];
    if(IS_SET(obj->value[1], PC_CORPSE) && clevel < 0)
      clevel = -clevel;
    /* dump items on ground */
    for (cobj = obj->contains; cobj; cobj = next_obj)
    {
      next_obj = cobj->next_content;
      obj_from_obj(cobj);
      obj_to_room(cobj, ch->in_room);
    }

    if (GET_CLASS(ch, CLASS_THEURGIST))
    {
      act("The $p turns to &+ydust&n and &+wb&+Llow&+ws&n away as its &+Wsoul&n is returned to whence it came.", FALSE, ch, obj, 0, TO_CHAR);
      act("The $p turns to &+ydust&n and &+wb&+Llow&+ws&n away as its &+Wsoul&n is returned to whence it came.", FALSE, ch, obj, 0, TO_ROOM);
    }
    else
    {
      act("&+L$p&+L begins to &n&+gwither&+L and &n&+yrot&+L as you absorb its essence.",
         FALSE, ch, obj, 0, TO_CHAR);
      act("&+L$p&+L begins to &n&+gwither&+L and &n&+yrot&+L as $n&+L absorbs its essence.",
         FALSE, ch, obj, 0, TO_ROOM);
    }

    if(GET_MAX_HIT(ch) > GET_HIT(ch))
      GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + (clevel * 4) + (level * 2));
    extract_obj(obj);
    update_pos(ch);
  }
  else
  {
    send_to_char("That is not a corpse!\n", ch);
  }
}

void spell_enchant_weapon(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int  i;
  bool affected;

  if( (GET_ITEM_TYPE(obj) == ITEM_WEAPON) && !IS_SET(obj->extra2_flags, ITEM2_MAGIC) )
  {
    SET_BIT(obj->extra2_flags, ITEM2_MAGIC);

    for( i = 0, affected = FALSE; i < MAX_OBJ_AFFECT; i++ )
    {
      if( obj->affected[i].location != APPLY_NONE && obj->affected[i].modifier != 0 )
      {
        affected = TRUE;
      }
    }
    // If it already has effects, but isn't flagged magic, add 1 or 2 (@ lvl 31+) hitroll (3 for Overlords).
    if( affected )
    {
      for( i = 0, affected = FALSE; i < MAX_OBJ_AFFECT; i++ )
      {
        if( obj->affected[i].modifier == 0 )
        {
          obj->affected[i].location = APPLY_HITROLL;
          obj->affected[i].modifier = 1 + (level / 31);
          break;
        }
      }
    }
    else
    {
      // At level 31: 2 hit, at 51: 3 hit (and 4 hit for Forgers+).
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 1 + (level - 11) / 20;

      // At level 36: 2 dam, at 56: 3 dam.
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 1 + (level - 16) / 20;
    }

    if(IS_GOOD(ch))
    {
      act("&+B$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
    }
    else if(IS_EVIL(ch))
    {
      act("&+r$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
    }
    else
    {
      act("&+Y$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
  else
  {
    send_to_char("&+wNothing seems to happen.&n\n", ch);
  }
}

void spell_full_heal(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  int num_dice = 1;

  if( level >= 36 )
    num_dice += 2;

  if( level >= 41 )
    num_dice += 2;

  if( level >= 46 )
    num_dice += 2;

  if( level >= 51 )
    num_dice += 2;

  if( GET_SPEC( ch, CLASS_CLERIC, SPEC_HEALER) )
  {
    if( level >= 52 )
      num_dice += 1;

    if( level >= 53 )
      num_dice += 1;

    if( level >= 54 )
      num_dice += 1;

    if( level >= 55 )
      num_dice += 1;

    if( level >= 56 )
      num_dice += 1;

  }

  int healpoints = 250 + dice(num_dice, 20);

  /*if(GET_CLASS(victim, CLASS_ANTIPALADIN) && affected_by_spell(victim, SPELL_HELLFIRE) && (GET_RACEWAR(ch) == GET_RACEWAR(victim)))
  healpoints *= .3;*/

  /* Removing holy destruction spells from holymen.
  if(GET_SPEC(ch, CLASS_CLERIC, SPEC_HOLYMAN) && IS_PC(ch) && (ch != victim) && ((GET_RACEWAR(ch) != GET_RACEWAR(victim) && !IS_PC_PET(victim)) || (IS_NPC(victim) && !IS_PC_PET(victim))))
  {
    struct damage_messages messages = {
      "&+cYou call upon the &+Cmight&+c of your &+Wgod &+cto &+rde&+Rst&+Wroy &+cthe &+rbody &+cof $N&+c, who stumbles from the &+Cimpact&+c!",
      "&+cThe &+Cmight&+c of &n$n's&+W god &+cis thrust upon your &+rbody&+c, causing massive &+Cdamage&+c!",
      "&+cThe &+Cmight&+c of &n$n's&+W god &+cis thrust upon &n$N&+c, causing massive &+Cdamage&+c!",
      "&+cYou &+Cdestroy &+cwhat little there is left of &n$N's &+cbody, leaving only a pool of &+rblood &+cand &+ysinew&+c!",
      "&+cThe &+Cpower &+cof &n$n's&+W god&+c is the last thing your &+rbody &+cfeels before &+Cexploding &+cinto chunky bits.",
      "&+cThe &+Cpower &+cof &n$n's&+W god&+c destroys &n$N's &+rbody&+c which explodes, leaving only a pool of &+rblood &+cand &+ysinew&+c!", 0  };

    int dam = 10 * level + number(1, 25);

    if(!NewSaves(victim, SAVING_SPELL, 0))
      dam = (int) (dam * 2.0);

    dam = (int) (dam * .75);

    spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
  }
  else
  {
  */

  if(type == SPELL_TYPE_SPELL)
  {
    if( GET_CHAR_SKILL(ch, SKILL_ANATOMY)
      && ( (GET_CHAR_SKILL(ch, SKILL_ANATOMY) + 5) / 10 ) > number(0, 100) )
    {
      act("$n quickly diagnoses your wounds.", FALSE, ch, 0, victim, TO_VICT);
      act("$n quickly diagnoses $N&n's wounds.", FALSE, ch, 0, victim, TO_NOTVICT);
      act("You quickly diagnose $N&n's wounds and apply accurate healing.", FALSE, ch, 0, victim, TO_CHAR);
      healpoints += number(10, 2 * GET_CHAR_SKILL(ch, SKILL_ANATOMY));
    }
  }

  if( ch == victim )
  {
    act("&+WA torrent of divine energy flows into your body, and your wounds begin to heal!", FALSE, ch, 0, victim, TO_CHAR);
  }
  else
  {
    act("&+WA torrent of divine energy flows into $N&+W's body, and $S wounds begin to heal!", FALSE, ch, 0, victim, TO_CHAR);
  }

  act("A torrent of divine energy flows from $n &ninto your body, and your wounds begin to heal!", FALSE, ch, 0, victim, TO_VICT);
  act("&+WA torrent of divine energy flows from $n &+Winto $N&+W's body, and $S wounds begin to heal!", FALSE, ch, 0, victim, TO_NOTVICT);

  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  grapple_heal(victim);
  heal(victim, ch, healpoints, GET_MAX_HIT(victim) - number(1, 4));
  update_pos(victim);
}

void spell_heal(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int num_dice = 1, healpoints;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(ch->in_room != victim->in_room)
  {
    act("You cannot find $N!", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

  if(GET_SPEC(ch, CLASS_CLERIC, SPEC_HEALER))
  {
    if(level >= 33)
      num_dice += 1;

    if(level >= 39)
      num_dice += 1;

    if(level >= 43)
      num_dice += 1;

    if(level >= 51)
      num_dice += 1;

    if(level >= 56)
      num_dice += 1;
  }
  else
  {
    if(level >= 26)
      num_dice += 1;

    if(level >= 31)
      num_dice += 1;

    if(level >= 36)
      num_dice += 1;

    if(level >= 41)
      num_dice += 1;
  }

  healpoints = 100 + dice(num_dice, 5);

 /* if(GET_CLASS(victim, CLASS_ANTIPALADIN) && affected_by_spell(victim, SPELL_HELLFIRE) && (GET_RACEWAR(ch) == GET_RACEWAR(victim)))
  healpoints *= .3;*/

  /* Removing holy destruction spells from holymen.
  if(GET_SPEC(ch, CLASS_CLERIC, SPEC_HOLYMAN) && IS_PC(ch) && ((GET_RACEWAR(ch) != GET_RACEWAR(victim) && !IS_PC_PET(victim)) || (IS_NPC(victim) && !IS_PC_PET(victim))))
  {
    struct damage_messages messages = {
      "&+cYou call upon the &+Cmight&+c of your &+Wgod &+cto &+rde&+Rst&+Wroy &+cthe &+rbody &+cof $N&+c, who stumbles from the &+Cimpact&+c!",
      "&+cThe &+Cmight&+c of &n$n's&+W god &+cis thrust upon your &+rbody&+c, causing &+Wsignificant &+Cdamage&+c!",
      "&+cThe &+Cmight&+c of &n$n's&+W god &+cis thrust upon &n$N&+c, causing &+Wsignificant &+Cdamage&+c!",
      "&+cYou &+Cdestroy &+cwhat little there is left of &n$N's &+cbody, leaving only a pool of &+rblood &+cand &+ysinew&+c!",
      "&+cThe &+Cpower &+cof &n$n's&+W god&+c is the last thing your &+rbody &+cfeels before &+Cexploding &+cinto chunky bits.",
      "&+cThe &+Cpower &+cof &n$n's&+W god&+c destroys &n$N's &+rbody&+c which explodes, leaving only a pool of &+rblood &+cand &+ysinew&+c!", 0  };

    int dam = 5 * level + number(1, 25);

    if(!NewSaves(victim, SAVING_SPELL, 0))
      dam = (int) (dam * 2);

    dam = (int) (dam * .75);

    spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
  }
  else
  {
  */

  if(type == SPELL_TYPE_SPELL)
  {
    if( GET_CHAR_SKILL(ch, SKILL_ANATOMY)
      && ((GET_CHAR_SKILL(ch, SKILL_ANATOMY) + 5) / 10 ) > number(0, 100) )
    {
      act("$n quickly diagnoses your wounds.", FALSE, ch, 0, victim, TO_VICT);
      act("$n quickly diagnoses $N&n's wounds.", FALSE, ch, 0, victim, TO_NOTVICT);
      act("You quickly diagnose $N&n's wounds and apply accurate healing.", FALSE, ch, 0, victim, TO_CHAR);
      healpoints += number(1, (GET_CHAR_SKILL(ch, SKILL_ANATOMY) / 2));
    }
  }

  if(ch == victim)
  {
    act("&+WA warm &+yfeeling&n &+Wfills your body!", FALSE, ch, 0, victim, TO_CHAR);
  }
  else
  {
    act("&+WYou heal $N.", FALSE, ch, 0, victim, TO_CHAR);
  }

  act("&+WHealing energy flows from $n into your body!", FALSE, ch, 0, victim, TO_VICT);
  act("&+WHealing energy flows from $n into $N's body!", FALSE, ch, 0, victim, TO_NOTVICT);

  if(IS_BLIND(victim))
  {
    spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  }

  grapple_heal(victim);
  heal(victim, ch, healpoints, GET_MAX_HIT(victim) - number(1, 4));
  update_pos(victim);
}
/* OLD HEAL - 21 Sep 08 -Lucrot
{
   int      healpoints = 100;


  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  if(GET_HIT(victim) > GET_MAX_HIT(victim))
    return;

  heal(victim, ch, healpoints, GET_MAX_HIT(victim) - number(1, 4));
  update_pos(victim);
  if(RACE_PUNDEAD(victim))
    send_to_char("&+WYou feel the powers of darkness strengthen you!\n",
                 victim);
  else
    send_to_char("&+WA warm feeling fills your body.\n", victim);
  grapple_heal(victim);

}
*/
void spell_natures_touch(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  int healpoints;
  int duration = (int) (WAIT_SEC * 1.5 * 56 / level);

  if(!IS_ALIVE(victim) ||
    !IS_ALIVE(ch))
      return;

  healpoints = level * 5 / 2;
  
  if(!GET_CLASS(ch, CLASS_DRUID))
    healpoints /= 2;
  
  if(GET_CLASS(ch, CLASS_DRUID) &&
     IS_BLIND(victim))
        spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  
  grapple_heal(victim);

  switch (world[ch->in_room].sector_type)
  {
  case SECT_CITY:
    healpoints -= 20;
    break;
  case SECT_FIELD:
    healpoints += 40;
    break;
  case SECT_FOREST:
    healpoints += 60;
    break;
  case SECT_HILLS:
    healpoints += 30;
    break;
  case SECT_UNDERWATER_GR:
  case SECT_MOUNTAIN:
    healpoints += 10;
    break;
  case SECT_UNDRWLD_WILD:
  case SECT_UNDRWLD_CITY:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_UNDRWLD_SLIME:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_UNDRWLD_LIQMITH:
  case SECT_UNDRWLD_MUSHROOM:
    healpoints += 5;
    break;
  default:
    break;
  }

  if (!(GET_CLASS(ch, CLASS_DRUID) || GET_CLASS(ch, CLASS_RANGER)))
    healpoints /= 3;

  if(IS_NPC(victim))
  {
    heal(victim, ch, healpoints, GET_MAX_HIT(victim) - number(1, 4));
    update_pos(victim);
  }
  else
  {
    memset(&af, 0, sizeof(af));
    af.type = SPELL_NATURES_TOUCH;
    af.flags = AFFTYPE_SHORT | AFFTYPE_NOSAVE;
    af.duration = duration;
    af.location = APPLY_HIT_REG;
    af.modifier = SECS_PER_MUD_HOUR * WAIT_SEC / duration * healpoints;
    af.bitvector4 = AFF4_REGENERATION;
    affect_to_char(victim, &af);
    update_achievements(ch, victim, healpoints, 1);
    
    if(IS_FIGHTING(victim))
    {
      gain_exp(ch, victim, MIN(healpoints, GET_MAX_HIT(victim) - GET_HIT(victim)), EXP_HEALING);
      update_pos(victim);
    }
  }
  
  if(ch == victim)
    act("&+GThe warmth of nature fills your body.", FALSE, ch, 0, victim, TO_CHAR);
  else
  {
     act("&+GThe warmth of nature fills your body.", FALSE, ch, 0, victim, TO_VICT);
     act("&+GYou gently touch $N&+G's body, and $S wounds begin to heal!",
       FALSE, ch, 0, victim, TO_CHAR);
  }
  act("&+G$n &+Ggently touches $N&+G's body, and $S wounds begin to heal!",
    FALSE, ch, 0, victim, TO_NOTVICT);
}

void spell_water_to_life(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  int      healpoints;

  obj = world[ch->in_room].contents;


  if (!IS_WATER_ROOM(ch->in_room))
  {
     while (obj != NULL)
      {
        if(GET_ITEM_TYPE(obj) == ITEM_DRINKCON &&
          (obj->wear_flags == 0) &&
          (obj->value[2] == LIQ_WATER || obj->value[2] == LIQ_LOTSAWATER))
        {
          break;
        }
        else
        {
          obj = obj->next_content;
        }
      }

         if(obj == NULL)
     {
       send_to_char("There is not enough water around!\n", ch);
       return;
     }
  }

  if(ch != victim && GET_RACE(victim) != RACE_W_ELEMENTAL &&
      !IS_WATERFORM(victim))
  {
    send_to_char("You will drown them instead!\n", ch);
    return;
  }

  healpoints = level * 2 + MAX(0, level - 50) * 20;

  heal(victim, ch, healpoints, GET_MAX_HIT(victim));
  update_pos(victim);

  if(ch == victim)
  {
    act
      ("&+bThe purifying power of the &+Bwater&+b flows through your body.&n",
       FALSE, ch, 0, victim, TO_CHAR);
  }
  else
  {
    act
      ("&+bThe purifying power of the &+Bwater&+b flows through your body.&n",
       FALSE, ch, 0, victim, TO_VICT);
    act("&+bWater absorbed from the surrounding area flows around $N.&n",
        FALSE, ch, 0, victim, TO_CHAR);
  }
  act("&+bWater absorbed from the surrounding area flows around $N.&n", FALSE,
      ch, 0, victim, TO_NOTVICT);

}

void event_healing_salve(P_char ch, P_char vict, P_obj obj, void *data)
{
  int waves, x;

  waves = *((int *) data);

  if(GET_HIT(vict) < GET_MAX_HIT(vict))
    send_to_char("You feel a &+Wwarm wave&n going through your body.\n", vict);

  x = vamp(vict, number(GET_LEVEL(vict), (GET_LEVEL(vict) * 2)), GET_MAX_HIT(vict));
  
  update_pos(vict);
  
  if(x > 0 && IS_FIGHTING(vict))
    gain_exp(ch, vict, x, EXP_HEALING);
  
  if(waves-- == 0)
    return;

  add_event(event_healing_salve, (int) (PULSE_VIOLENCE * 0.8), ch, vict, 0, 0,
            &waves, sizeof(waves));
}

void spell_healing_salve(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  int      waves = 3;

  act("Upon $n's touch a &+Csoft glow&n flows from $s hands and surrounds $N.",
     FALSE, ch, 0, victim, TO_NOTVICT);
  act("Upon $n's touch a &+Csoft glow&n flows from $s hands and surrounds you.",
     FALSE, ch, 0, victim, TO_VICT);
  act("You touch $N invoking a &+Chealing energy&n to cure $S wounds.", FALSE,
      ch, 0, victim, TO_CHAR);
  
  add_event(event_healing_salve, (int) (PULSE_VIOLENCE * 0.1), ch, victim, 0, 0,
            &waves, sizeof(waves));

  //if(affected_by_spell(victim, SPELL_PLAGUE))
  //  affect_from_char(victim, SPELL_PLAGUE);

  if(IS_AFFECTED4(victim, AFF4_CARRY_PLAGUE))
    REMOVE_BIT(victim->specials.affected_by4, AFF4_CARRY_PLAGUE);

  //if(affected_by_spell(victim, SPELL_WITHER))
  //  affect_from_char(victim, SPELL_WITHER);
}

void spell_sticks_to_snakes(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  int      snakes, arrowSnakes, dam, room;
  struct damage_messages arrow_messages = {
    "You turn $N's &+yarrow into a &+gsnake &nand send it against $M!",
    "Your own &+yarrow&n turns into a &+gsnake and bites you, &+Lvanishing afterwards&n!",
    "$N's own &+yarrow&n turns into a &+gsnake and bites&n $N, &+Lvanishing afterwards&n!",
    "You turn $N's &+yarrow into a &+gsnake &nand send it against $N!",
    "Your own &+yarrow&n turns into a &+gsnake and bites you, &+Lvanishing afterwards&n!",
    "$N's own &+yarrow&n turns into a &+gsnake and bites&n $N, &+Lvanishing afterwards&n!",
  };
  struct damage_messages messages = {
    "&+yYou turn a stick into a &+gsnake &+yand send it against $N!",
    "A &+ystick&n turns into a &+gsnake &nand bites you, &+Lvanishing afterwards&n!",
    "A &+ystick&n turns into a &+gsnake &nand bites $N, &+Lvanishing afterwards&n!",
    "&+yYou turn a stick into a &+gsnake &+yand send it against $N!",
    "A &+ystick&n turns into a &+gsnake &nand bites you, &+Lvanishing afterwards&n!",
    "A &+ystick&n turns into a &+gsnake &nand bites $N, &+Lvanishing afterwards&n!"
  };
  snakes = arrowSnakes = 0;

  room = victim->in_room;

  if(level > 50)
    dam = dice(8, 8);
  else if(level > 30)
    dam = dice(7, 7);
  else if(level > 20)
    dam = dice(6, 6);
  else
    dam = dice(5, 5);

  switch (world[ch->in_room].sector_type)
  {
  case SECT_CITY:
  case SECT_ROAD:
    snakes = number(1, 2);
    break;
  case SECT_FIELD:
    snakes = number(1, 5);
    break;
  case SECT_FOREST:
    snakes = number(1, 6);
    break;
  case SECT_HILLS:
    snakes = number(1, 4);
    break;
  case SECT_UNDERWATER_GR:
  case SECT_MOUNTAIN:
    snakes = number(1, 3);
    break;
  case SECT_UNDRWLD_WILD:
  case SECT_UNDRWLD_CITY:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_UNDRWLD_SLIME:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_UNDRWLD_LIQMITH:
  case SECT_UNDRWLD_MUSHROOM:
    snakes = number(1, 2);
    break;
  case SECT_INSIDE:
  case SECT_UNDRWLD_INSIDE:
  default:
    snakes = 1;
    break;
  }

  obj = victim->carrying;

  while( obj = get_obj_in_list("arrow", victim->carrying) )
  {
    if( IS_ARTIFACT(obj) )
    {
      break;
    }
    extract_obj(obj, TRUE); // Artifact arrows?
    arrowSnakes++;
  }

  while (snakes + arrowSnakes > 0 && is_char_in_room(victim, room))
  {
    if(arrowSnakes > 0)
    {
      spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_ALLGLOBES, &arrow_messages);
      arrowSnakes--;
    }
    else
    {
      spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_ALLGLOBES, &messages);
      snakes--;
    }
  }
}

void spell_dispel_invisible(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if(victim)
    if(resists_spell(ch, victim))
      return;

  if(obj)
  {
    if(IS_SET(obj->extra_flags, ITEM_INVISIBLE))
    {
      act("$p fades into visibility.", FALSE, ch, obj, 0, TO_CHAR);
      act("$p fades into visibility.", TRUE, ch, obj, 0, TO_ROOM);
      REMOVE_BIT(obj->extra_flags, ITEM_INVISIBLE);
    }
  }
  else
  {
    if(affected_by_spell(victim, SPELL_INVISIBLE))
    {
      act("&+W$n slowly fades into existance.", TRUE, victim, 0, 0, TO_ROOM);
      send_to_char("&+WYou turn visible.\n", victim);
      affect_from_char(victim, SPELL_INVISIBLE);
      if(IS_SET(victim->specials.affected_by, AFF_INVISIBLE))
        REMOVE_BIT(victim->specials.affected_by, AFF_INVISIBLE);
      if(IS_SET(victim->specials.affected_by2, AFF2_MINOR_INVIS))
        REMOVE_BIT(victim->specials.affected_by2, AFF2_MINOR_INVIS);
    }
  }
}

void spell_shadow_projection(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;

  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
    
  if(affected_by_spell(ch, SPELL_FAERIE_FIRE))
  {
    act("&+MThe magical fire surrounding you negates the spell!&n",
      FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(!affected_by_spell(ch, SPELL_SHADOW_PROJECTION))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_SHADOW_PROJECTION;
    af.duration = 1;
    af.bitvector = AFF_SNEAK;
    af.bitvector = AFF_HIDE;
    af.bitvector2 = AFF2_PASSDOOR;
    affect_to_char(ch, &af);

    act("&+L$n &+Lfades into nothingness, and simply ceases to exist.&n", FALSE,
        ch, 0, 0, TO_ROOM);
    act("&+LYou blend into the shadows, seemingly ceasing to exist in this realm.&n", FALSE,
        ch, 0, 0, TO_CHAR);
  }
}

void spell_invisibility(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int room;
  struct affected_type af;

  if(obj)
  {
    act("&+L$p flares up for a second, then is still again.", FALSE, ch, obj,
        0, TO_CHAR);
    act("&+L$p flares up for a second, then is still again.", TRUE, ch, obj,
        0, TO_ROOM);
  }
  else
  {                             /*
                                 * Then it is a PC | NPC
                                 */
    if(!affected_by_spell(victim, SPELL_INVISIBLE))
    {

      act("&+L$n slowly fades out of existence.", TRUE, victim, 0, 0,
          TO_ROOM);
      send_to_char("&+LYou vanish.\n", victim);

      bzero(&af, sizeof(af));

      af.type = SPELL_INVISIBLE;
      af.duration = level / 2;

      if(GET_SPEC(ch, CLASS_SORCERER, SPEC_SHADOW) && 
         ch == victim &&
         !IS_WATER_ROOM(ch->in_room) &&
         world[room].sector_type != SECT_OCEAN)
      {
        af.bitvector = AFF_HIDE;
      }
      
      af.bitvector2 = AFF2_MINOR_INVIS;
      affect_to_char(victim, &af);
    }
    else
    {
      struct affected_type *af1;

      for (af1 = victim->affected; af1; af1 = af1->next)
        if(af1->type == SPELL_INVISIBLE)
        {
          af1->duration = level / 2;
        }
    }
  }
}

void spell_improved_invisibility(int level, P_char ch, char *arg, int type,
                                 P_char victim, P_obj obj)
{
  struct affected_type af;

  if(obj)
  {
    if(!IS_SET(obj->extra_flags, ITEM_INVISIBLE) &&
        IS_SET(obj->wear_flags, ITEM_TAKE))
    {
      act("&+L$p turns invisible.", FALSE, ch, obj, 0, TO_CHAR);
      act("&+L$p turns invisible.", TRUE, ch, obj, 0, TO_ROOM);
      SET_BIT(obj->extra_flags, ITEM_INVISIBLE);
      REMOVE_BIT(obj->extra_flags, ITEM_LIT);
    }
  }
  else
  {                             /*
                                 * Then it is a PC | NPC
                                 */
    if(!affected_by_spell(victim, SPELL_INVIS_MAJOR))
    {

      act("&+L$n slowly fades out of existence.", TRUE, victim, 0, 0,
          TO_ROOM);
      send_to_char("&+LYou vanish.\n", victim);

      bzero(&af, sizeof(af));

      af.type = SPELL_INVIS_MAJOR;
      af.duration = level / 2;
      af.bitvector = AFF_INVISIBLE;
      affect_to_char(victim, &af);
    }
    else
    {
      struct affected_type *af1;

      for (af1 = victim->affected; af1; af1 = af1->next)
        if(af1->type == SPELL_INVIS_MAJOR)
        {
          af1->duration = level / 2;
        }
    }
  }
}


void spell_disease(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  struct affected_type af;
  int      temp;

  temp = (int) (level / 10);

  if(get_spell_from_room(&world[ch->in_room], SPELL_SUMMON_INSECTS)) {
      temp += 3;
  }

  if(get_spell_from_room(&world[ch->in_room], SPELL_CONSECRATE_LAND)) {
      temp -= 3;
  }

  if(NewSaves(victim, SAVING_PARA, temp))
    return;

  if(IS_UNDEADRACE(victim))
    return;

  if(!affected_by_spell(victim, SPELL_DISEASE))
  {

    bzero(&af, sizeof(af));
    send_to_char("&+yYou suddenly don't feel so well!\n", victim);
    act("&+y$n &+ysuddenly does not look so well.", FALSE, victim, 0, 0,
        TO_ROOM);
    af.type = SPELL_DISEASE;
    af.duration = 3 * (1 + temp);
    af.modifier = -(5 * (1 + temp));
    af.location = APPLY_STR;
    affect_to_char(victim, &af);
    af.location = APPLY_DEX;
    affect_to_char(victim, &af);
    af.location = APPLY_AGI;
    affect_to_char(victim, &af);
    af.location = APPLY_CON;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_DISEASE)
      {
        af1->duration = MAX(5, level / 4);
      }
    send_to_char("&+yYou suddenly feel the disease in your body growing stronger!\n", victim);
    act("&+y$n &+ysuddenly looks even worse than before.", FALSE, victim, 0, 0, TO_ROOM);
  }


}

void spell_poison(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool was_poisoned;

  if( victim )
  {
    if( !IS_ALIVE(victim) || IS_TRUSTED(victim) || resists_spell(ch, victim) )
    {
      return;
    }

    if( (level > 0) && NewSaves(victim, SAVING_SPELL, (level - GET_LEVEL(victim) )/3) )
    {
      return;
    }

    was_poisoned = IS_SET(victim->specials.affected_by2, AFF2_POISONED);

    if( !IS_TRUSTED(victim) && !IS_UNDEADRACE(victim) )
    {
      level = abs(level);
      (skills[number(FIRST_POISON, LAST_POISON)].spell_pointer) (level, ch, 0, 0, victim, 0);

      act("&+G$n shivers slightly.", TRUE, victim, 0, 0, TO_ROOM);
      if( was_poisoned )
        send_to_char("&+GYou feel even more ill.\n", victim);
      else
        send_to_char("&+GYou feel very sick.\n", victim);

    }
    appear(ch);

    if( IS_NPC(victim) && CAN_SEE(victim, ch) )
    {
      remember(victim, ch);
      if( !IS_FIGHTING(victim) && !IS_DESTROYING(victim) )
      {
        MobStartFight(victim, ch);
      }
    }
  }
  else
  {                             /* Object poison */
    if( (obj->type == ITEM_DRINKCON) || (obj->type == ITEM_FOOD) )
    {
      obj->value[3] = number(1, MAX(1, GET_LEVEL(ch) / 10));
    }
    else
      send_to_char("The spell seems to have no effect on that object.\n", ch);
  }
}

/** TAM 1/94 -- paralyze a PC/MOB so they can't perform actions which     **/
/**             require actual physical movement AND not respond to       **/
/**             attacker until wears off **/

void spell_major_paralysis(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct affected_type af;
  int      lev = level;

  if(!((victim || obj) && ch))
  {
    return;
  }
  if(GET_STAT(ch) == STAT_DEAD)
    return;

  appear(ch);

  if(!IS_TRUSTED(ch) && (resists_spell(ch, victim) ||
                          (IS_NPC(victim) &&
                           IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA))))
    return;

  /*
   * calling with negative level negates save
   */
  /*
   * indeed it does, but magic resistance counts even to that.
   */

  if(IS_TRUSTED(ch) || (lev < 0) || !NewSaves(victim, SAVING_PARA, -4))
  {
    if(lev < 0)
      lev = -lev;
    bzero(&af, sizeof(af));
    af.type = SPELL_MAJOR_PARALYSIS;
    af.flags = AFFTYPE_SHORT;
    af.duration = 50 * WAIT_SEC / 2;
    af.bitvector2 = AFF2_MAJOR_PARALYSIS;

    affect_to_char(victim, &af);

    act("$n &+Mceases to move.. still and lifeless.",
        FALSE, victim, 0, 0, TO_ROOM);
    send_to_char
      ("&+LYour body becomes like stone as the paralyzation takes effect.\n",
       victim);
    if(IS_FIGHTING(victim))
      stop_fighting(victim);
    if(IS_DESTROYING(victim))
      stop_destroying(victim);

    /*
     * stop all non-vicious/agg attackers
     */
    StopMercifulAttackers(victim);

    remember(victim, ch);
  }
  else if(IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }
}

/** TAM 1/94 -- paralyze a PC/MOB so they can't perform actions which require **/
/**             actual physical movement BUT wear off when attacked.          **/

void spell_minor_paralysis(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct affected_type af;

  if(resists_spell(ch, victim) ||
      (IS_NPC(victim) && IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA)))
    return;

  if(!NewSaves(victim, SAVING_PARA, 0))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_MINOR_PARALYSIS;
    af.flags = AFFTYPE_SHORT;
    af.duration = (int) (level * WAIT_SEC * 0.75);
    af.bitvector2 = AFF2_MINOR_PARALYSIS;

    affect_to_char(victim, &af);

    act ("$n &+Wturns pale as some magical force occupies $s body, causing all motion to halt.",
       FALSE, victim, 0, 0, TO_ROOM);
    send_to_char ("&+LYour body becomes like stone as the paralyzation takes effect.\n",
       victim);
    if(IS_FIGHTING(victim))
      stop_fighting(victim);
    if(IS_DESTROYING(victim))
      stop_destroying(victim);

    /*
     * stop all non-vicious/agg attackers
     */
    StopMercifulAttackers(victim);
  }
}                               /*
                                 * spell_paralyze
                                 */

/** TAM 1/94 -- slow a PC/MOB down  so that commands are processed at a reduced rate **/

void spell_slow(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{
  struct affected_type af;

  if(GET_STAT(ch) == STAT_DEAD)
    return;
  
  appear(ch);
    
  if(IS_AFFECTED2(victim, AFF2_SLOW))
  {
    act("Just between you and me, $N looks pretty slow already.",
      FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

  if(GET_CLASS(victim, CLASS_MONK))
  {
    act("$N's intense concentration means that $E cannot be slowed!",
      TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if(resists_spell(ch, victim))
    return;

  if(!saves_spell(victim, SAVING_PARA))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_SLOW;
    af.duration = (level >> 4 + 1);
    af.modifier = 2;
    af.bitvector2 = AFF2_SLOW;

    affect_to_char(victim, &af);

    act("&+m$n begins to sllooowwww down.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&+mYou feel yourself slowing down.\n", victim);
  }

  if(IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }
}                               /*
                                 * spell_slow
                                 */

void spell_stornogs_lowered_magical_res(int level, P_char ch, char *arg,
                                        int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(GET_STAT(ch) == STAT_DEAD)
    return;

  if(resists_spell(ch, victim))
    return;

//  if(!NewSaves(victim, SAVING_SPELL, 0)) {
  if(!saves_spell(victim, SAVING_PARA))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_STORNOGS_LOWERED_RES;
    af.duration = level / 10;
    af.location = APPLY_SAVING_SPELL;
    af.modifier = (level / 10) + 3;

    affect_to_char(victim, &af);

    act("&+mYou sense that $n&n&+m's magical resistance has been affected.",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&+mYou feel more susceptible to magic.\n", victim);
  }

  if(IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
    appear(ch);

  if(IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }
}


void spell_protection_from_evil(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_EVIL;
    af.duration = level;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char(victim, &af);
    send_to_char("&+YYou feel protected from the evil of the world!\n",
                 victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_EVIL)
      {
        af1->duration = level;
      }
  }
}

void spell_protection_from_good(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_GOOD;
    af.duration = level;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char(victim, &af);
    send_to_char
      ("&+rYou feel able to withstand the holy goodness of the vile world!\n",
       victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_GOOD)
      {
        af1->duration = level;
      }
  }
}

void spell_remove_curse(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_obj    t_obj, nextobj;
  int      e_pos;

  if (IS_NPC(ch) && (GET_VNUM(ch) == 63))
  return;

  if(obj)
  {
    if(IS_SET(obj->extra_flags, ITEM_NODROP))
    {
      act("&+B$p briefly glows blue.", TRUE, ch, obj, 0, TO_CHAR);

      REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
    }
  }
  else
  {
    /*
     * Then it is a PC | NPC
     */
    if(affected_by_spell(victim, SPELL_CURSE))
    {
      act("$n briefly &+rglows red, &+bthen blue.", FALSE, victim, 0, 0,
          TO_ROOM);
      act("&+WYou feel better.", FALSE, victim, 0, 0, TO_CHAR);
      affect_from_char(victim, SPELL_CURSE);
    }
    /*
     * the nifty new bit, cursed items worn by target get zapped off,
     * but remain cursed. -JAB
     */

    if(!is_linked_to(ch, victim, LNK_CONSENT) && (ch != victim) &&
        !IS_TRUSTED(ch) && IS_PC(ch))
    {
      send_to_char("target must consent to you to zap items in equip.\n", ch);
      return;
    }
    for (e_pos = 0; e_pos < MAX_WEAR; e_pos++)
      if((t_obj = victim->equipment[e_pos]) &&
          IS_SET(t_obj->extra_flags, ITEM_NODROP))
      {
        act("Your $q &+Bflashes blue&n and falls to the ground.",
            FALSE, victim, t_obj, 0, TO_CHAR);
        act("$n's $q &+Bflashes blue&n and falls to the ground.",
            FALSE, victim, t_obj, 0, TO_ROOM);
        if(obj_index[t_obj->R_num].virtual_number == 67243)
        {
          act
            ("&+LUpon being torn from $n&+L, $p &+Llets out a ghastly &n&+rSHRIEK!!!",
             FALSE, victim, t_obj, 0, TO_ROOM);
          act
            ("&+LUpon being torn from you, $p &+Llets out a ghastly &n&+rSHRIEK!!!",
             FALSE, victim, t_obj, 0, TO_CHAR);
          affect_from_char(ch, SPELL_VAMPIRE);
          spell_dispel_magic(60, victim, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(IS_TRUSTED(victim))
        {
          wizlog(GET_LEVEL(victim), "%s drops %s [%d]",
                 GET_NAME(victim), t_obj->short_description,
                 world[victim->in_room].number);
          logit(LOG_WIZ, "%s drops %s [%d]",
                GET_NAME(victim), t_obj->short_description,
                world[victim->in_room].number);
          sql_log(victim, WIZLOG, "Dropped %s", t_obj->short_description);
        }
        obj_to_room(unequip_char(victim, e_pos), victim->in_room);
      }
    for (t_obj = victim->carrying; t_obj; t_obj = nextobj)
    {
      nextobj = t_obj->next_content;

      if(IS_SET(t_obj->extra_flags, ITEM_NODROP))
      {
        act("Your $q &+Bflashes blue&n and falls to the ground.",
            FALSE, victim, t_obj, 0, TO_CHAR);
        act("$n's $q &+Bflashes blue&n and falls to the ground.",
            FALSE, victim, t_obj, 0, TO_ROOM);
        if(obj_index[t_obj->R_num].virtual_number == 67243)
        {
          act
            ("&+LUpon being torn from $n&+L, $p &+Llets out a ghastly &n&+rSHRIEK!!!",
             FALSE, victim, t_obj, 0, TO_ROOM);
          act
            ("&+LUpon being torn from you, $p &+Llets out a ghastly &n&+rSHRIEK!!!",
             FALSE, victim, t_obj, 0, TO_CHAR);
          affect_from_char(ch, SPELL_VAMPIRE);
          spell_dispel_magic(60, victim, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(IS_TRUSTED(victim))
        {
          wizlog(GET_LEVEL(victim), "%s drops %s [%d]",
                 GET_NAME(victim), t_obj->short_description,
                 world[victim->in_room].number);
          logit(LOG_WIZ, "%s drops %s [%d]",
                GET_NAME(victim), t_obj->short_description,
                world[victim->in_room].number);
          sql_log(victim, WIZLOG, "Dropped %s", t_obj->short_description);
        }
        obj_from_char(t_obj);
        obj_to_room(t_obj, victim->in_room);
      }
    }
  }
}

void spell_remove_poison(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type *af, *next;

  if(victim)
  {
    
    if(poison_common_remove(victim))
    {
      act("&+WThe power of your god removes the poison.", FALSE, ch, 0, 0, TO_CHAR);
      act("&+WYou suddenly feel healthier!", FALSE, ch, 0, victim, TO_VICT);
    }
    else
      send_to_char("You do your best, but it seems there is no poison to cure.\n", ch);
  }
  else
  {
    if(obj->value[3])
    {
      if((obj->type == ITEM_DRINKCON) || (obj->type == ITEM_FOOD))
      {
        obj->value[3] = 0;
        act("The $q steams briefly.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
    else
      send_to_char("It seems unchanged.\n", ch);
  }
}

void spell_group_stone_skin(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct group_list *gl;

  if(ch && ch->group)
  {
    gl = ch->group;
    /* leader first */
    if(gl->ch->in_room == ch->in_room)
      spell_stone_skin((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if(gl->ch->in_room == ch->in_room)
        spell_stone_skin((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    }
  }
}
/* if it looks alot like gstone...that's because i mercilessly ripped it off of that! */
void spell_group_haste(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct group_list *gl;

  if(ch && ch->group)
  {
    gl = ch->group;
    /* leader first */
    if(gl->ch->in_room == ch->in_room)
      spell_haste((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if(gl->ch->in_room == ch->in_room)
        spell_haste((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    }
  }
}


bool has_skin_spell(P_char ch)
{
  if(!affected_by_spell(ch, SPELL_STONE_SKIN) &&
     !IS_AFFECTED(ch, AFF_STONE_SKIN) &&
     !affected_by_spell(ch, SPELL_SHADOW_SHIELD) &&
     !affected_by_spell(ch, SPELL_BIOFEEDBACK) &&
     !IS_AFFECTED(ch, AFF_BIOFEEDBACK) &&
     !affected_by_spell(ch, SPELL_IRONWOOD) &&
     !affected_by_spell(ch, SPELL_VINES) &&
     !IS_AFFECTED5(ch, AFF5_VINES) &&
	 !affected_by_spell(ch, SPELL_ICE_ARMOR) &&
	 !affected_by_spell(ch, SPELL_NEG_ARMOR))
    return false;
  else
    return true;
}
#define MACE_OF_EARTH_VNUM 23805
void spell_stone_skin(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  int      absorb = (level / 4) + number(1, 4);

  if(!has_skin_spell(victim))
  {
    if( has_innate(ch, INNATE_LIVING_STONE) || (victim->equipment[WIELD]
      && (obj_index[victim->equipment[WIELD]->R_num].virtual_number == MACE_OF_EARTH_VNUM)) )
    {
      absorb = (int) (absorb * 1.5);
      act("&+LLiving stone sprouts up and covers $n's flesh.",
        TRUE, victim, 0, 0, TO_ROOM);
      act("&+LLiving stone sprouts up and covers your flesh.",
        TRUE, victim, 0, 0, TO_CHAR);
    }
   else if(GET_CLASS(ch, CLASS_CONJURER))
    {
      absorb = (int) (absorb * 1.2);
      act("&+L$n's flesh melds with conjured rocks, turning it to stone.",
        TRUE, victim, 0, 0, TO_ROOM);
      act("&+LYour flesh melds with conjured rocks, turning it to stone.",
        TRUE, victim, 0, 0, TO_CHAR);
    }
   else if(GET_CLASS(ch, CLASS_SORCERER))
    {
      absorb = (int) (absorb * 1.0);
      act("&+L$n's flesh magically hardens, turning to stone.",
        TRUE, victim, 0, 0, TO_ROOM);
      act("&+LYour flesh magically hardens, turning to stone.",
        TRUE, victim, 0, 0, TO_CHAR);
    }
    else
    {
      absorb = (int) (absorb * .8);
      act("&+L$n&+L's skin seems to turn to stone.",
        TRUE, victim, 0, 0, TO_ROOM);
      act("&+LYou feel your skin harden to stone.",
        TRUE, victim, 0, 0, TO_CHAR);
    }
  }
  else
  {
    send_to_char("Their skin is already hard as a rock!\n", ch);
    return;
  }

  if(GET_OPPONENT(victim))
    gain_exp(ch, victim, 50 + GET_LEVEL(ch) * 2, EXP_HEALING); // stoning the tank equal to small heal in exp -Odorf

  bzero(&af, sizeof(af));
  af.type = SPELL_STONE_SKIN;
  af.duration = 4;
  af.modifier = absorb;
  affect_to_char(victim, &af);
}

void spell_ironwood(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  int absorb = (level / 5) + number(1, 4);

  if( !IS_AFFECTED(victim, AFF_BARKSKIN) )
  {
    send_to_char("They're not even made of wood! How can you begin to make it resemble iron!\n", ch);
    return;
  }
  if( has_skin_spell(victim) )
  {
    send_to_char("Their skin is already hard as a rock!\n", ch);
    return;
  }

  act("&+y$n's &+ybarkskin seems to take on the texture of &+Liron.", TRUE, victim, 0, 0, TO_ROOM);
  act("&+yYou feel your barkskin harden to &+Liron.", TRUE, victim, 0, 0, TO_CHAR);

  // Stoning the tank equals to heal in exp -Odorf
  if( GET_OPPONENT(victim) )
    gain_exp(ch, victim, absorb, EXP_HEALING);

  bzero(&af, sizeof(af));
  af.type = SPELL_IRONWOOD;
  af.duration = 4;
  af.modifier = absorb;
  affect_to_char(victim, &af);
}

void spell_sleep(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{
  struct affected_type af;
  int      i;

  if(GET_STAT(ch) == STAT_DEAD)
   {
   send_to_char("They are already... quite... asleep... for good.&n\n", ch);
    return;
   }

  if(IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
    appear(ch);

  if(resists_spell(ch, victim))
  { 
    send_to_char("Your victim resists your attempt to make them sleep.&n\n", ch);
    return;
  }

  if(level > 0)
    for (i = 0; i < MAX_WEAR; i++)
    {
      if(victim->equipment[i] &&
          IS_SET(victim->equipment[i]->extra_flags, ITEM_NOSLEEP))
      {
        send_to_char("&+CYour target appears to be protected against sleeping!\n", ch);
        if(IS_PC(victim))
          send_to_char("You stifle a yawn.\n", victim);
        if(IS_NPC(victim) && CAN_SEE(victim, ch))
        {
          remember(victim, ch);
          if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
            MobStartFight(victim, ch);
          send_to_char("Your victim does not want to sleep right now!&n\n", ch);
        }
        return;
      }
    }
  if((level < 0) ||
      (!saves_spell(victim, SAVING_SPELL) && (GET_LEVEL(victim) < 56) &&
       !IS_DEMON(victim) && !IS_UNDEADRACE(victim) && !IS_ELEMENTAL(victim)))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_SLEEP;
    af.duration = 4 + (level < 0 ? -level : level);
    af.duration /= 10;
    if(af.duration > 1)
      af.duration--;
    else
      af.duration = 1;

    af.bitvector = AFF_SLEEP;

    act("&+LYou feel very sleepy ..... zzzzzz", FALSE, victim, 0, 0, TO_CHAR);
    if(victim->specials.fighting)
      stop_fighting(victim);
    if( IS_DESTROYING(victim) )
      stop_destroying(victim);
    if(GET_STAT(victim) > STAT_SLEEPING)
    {
      act("&+W$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      SET_POS(victim, GET_POS(victim) + STAT_SLEEPING);
    }
    affect_join(victim, &af, FALSE, FALSE);
    /*
     * stop all non-vicious/agg attackers
     */
    StopMercifulAttackers(victim);
    return;
  }
  if(IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
   send_to_char("Your victim does not want to sleep right now!&n\n", ch);
  }
}

void spell_dexterity(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_DEXTERITY))
  {
    send_to_char("&+CYou can't get more dexterous than this!\n", victim);
    return;
  }
  act("&+gYou feel more dexterous.", FALSE, victim, 0, 0, TO_CHAR);

  bzero(&af, sizeof(af));

  af.type = SPELL_DEXTERITY;
  af.duration = level;
  af.modifier = 10;
  af.location = APPLY_DEX;

  affect_join(victim, &af, TRUE, FALSE);
}

void spell_agility(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
      return;
      
  if(affected_by_spell(victim, SPELL_AGILITY))
  {
    send_to_char("&+BYou can't get more agile than this!\n", victim);
    return;
  }
  
  act("&+BYou feel more agile.&n",
    FALSE, victim, 0, 0, TO_CHAR);

  bzero(&af, sizeof(af));

  af.type = SPELL_AGILITY;
  af.duration = level;
  af.modifier = 10;
  af.location = APPLY_AGI;

  affect_join(victim, &af, TRUE, FALSE);
}

void spell_strength(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_STRENGTH))
  {
    send_to_char("&+CYou can't get much stronger than this!\n", victim);
    return;
  }
  act("&+CYou feel stronger.", FALSE, victim, 0, 0, TO_CHAR);

  bzero(&af, sizeof(af));

  af.type = SPELL_STRENGTH;
  af.duration = level;
  af.modifier = 10;
  af.location = APPLY_STR;

  affect_join(victim, &af, TRUE, FALSE);
}

void spell_ventriloquate(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  /*
   * Not possible!! No argument!
   */
}

void spell_word_of_recall(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      loc_nr, e_pos, heavy;
  int      a, b = 0;

  if(!SanityCheck(ch, "spell_word_of_recall") ||
      !SanityCheck(victim, "spell_word_of_recall"))
    return;

  if(IS_NPC(victim) && (!GET_BIRTHPLACE(victim)))
    return;

  if(IS_NPC(victim) && IS_PC_PET(victim))
   return;

  if(IS_SET(world[ch->in_room].room_flags, ROOM_SILENT))
  {
    send_to_char("No sound can be heard.\n", ch);
    return;
  }

  if(affected_by_spell(ch, SKILL_BEARHUG))
  {
    send_to_char("You can't seem to get enough breath to speak!", ch);
    return;
  }

  if(IS_SET(world[ch->in_room].room_flags, NO_RECALL))
     //||(world[ch->in_room].sector_type == SECT_OCEAN))
  {
    if(ch == victim)
      act("$n utters a single word.", TRUE, ch, 0, 0, TO_ROOM);
    else
    {
      act("$n utters a single word.", TRUE, ch, 0, victim, TO_NOTVICT);
      act("You utter a single word.", TRUE, ch, 0, victim, TO_CHAR);
    }
    return;
  }
  if(IS_FIGHTING(ch))
  {
    if(IS_PC(ch) && IS_PC(ch->specials.fighting) && !number(0, 2))
    {
      if(ch == victim)
        act("$n utters a single word.", TRUE, ch, 0, 0, TO_ROOM);
      else
      {
        act("$n utters a single word.", TRUE, ch, 0, victim, TO_NOTVICT);
        act("You utter a single word.", TRUE, ch, 0, victim, TO_CHAR);
      }
      return;
    }
  }
  if(!GET_BIRTHPLACE(ch))
  {
    for (a = 12; (a > 0) && !b; a--)
      if(avail_hometowns[a][(int) GET_RACE(ch)])
        b = a;
    if(!b)
    {
      send_to_char("You don't know any sort of home anymore..snif\n", ch);
      return;
    }
    if(IS_PC(ch))
      a = guild_locations[b][flag2idx(ch->player.m_class)];
    if(a < 0)
    {
      send_to_char("You don't know any sort of home anymore..sniff!f\n", ch);
      return;
    }
  }
  else
    a = GET_BIRTHPLACE(ch);
  loc_nr = real_room(a);
  if((loc_nr == NOWHERE) || (loc_nr > top_of_world))
  {
    send_to_char("You are completely lost.\n", victim);
    return;
  }
  if(ch == victim)
    act("&+W$n utters a single word and disappears.",
        TRUE, victim, 0, 0, TO_ROOM);
  else
  {
    act("&+W$n utters a single word and $N disappears.",
        TRUE, ch, 0, victim, TO_NOTVICT);
    act("&+WYou utter a single word and $N disappears.",
        TRUE, ch, 0, victim, TO_CHAR);
  }
  if( IS_DESTROYING(victim) )
    stop_destroying(victim);
  if( IS_PC( victim ) )
    sql_log(victim, PLAYERLOG, "Word of recalled", world[victim->in_room].number);

  /* Exceeding wieght limit? */
  e_pos = heavy = 0;
  do
  {
    if(IS_CARRYING_W(victim) > ((CAN_CARRY_W(victim) / 100) * 70))
      if(victim->equipment[e_pos])
      {
        logit(LOG_RECALL, "WORD OF RECALL: (%s) drops (%s) in [%d].",
          GET_NAME(victim), victim->equipment[e_pos]->short_description,
          world[victim->in_room].number);
          
        logit(LOG_WIZ, "WORD OF RECALL: (%s) drops (%s) in [%d].",
          GET_NAME(victim), victim->equipment[e_pos]->short_description,
          world[victim->in_room].number);
        if( IS_PC( victim ) )
        {
          sql_log(victim, PLAYERLOG, "Dropped %s&n [%d] while word of recalling.", 
            victim->equipment[e_pos]->short_description, 
            obj_index[victim->equipment[e_pos]->R_num].virtual_number,
            world[victim->in_room].number);
        }

        obj_to_room(unequip_char(victim, e_pos), victim->in_room);
        heavy = TRUE;
      }
    e_pos++;
  }
  while (e_pos < MAX_WEAR);

  logit(LOG_RECALL, "WORD OF RECALL: (%s) recalled from [%d].",
    GET_NAME(victim), world[victim->in_room].number);

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  char_from_room(victim);
  
  if(heavy)
  {
    send_to_char
      ("Oof, what an effort that was! Too bad you had to leave something behind.\n", ch);
    if (ch != victim)
      send_to_char
        ("&+WOops, seems you was too weak and left some of your stuff behind!&n\n", victim);
  }
  
  char_to_room(victim, loc_nr, -1);
  act("&+W$n suddenly fades into this reality, muttering a word of thanks.",
      TRUE, victim, 0, 0, TO_ROOM);
}


void spell_group_recall(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct group_list *gl;

  if(ch->group)
  {
#if 0
    gl = ch->group;
    /* leader first */
    if(gl->ch->in_room == ch->in_room)
      spell_word_of_recall((level / 3) * 2, ch, gl->ch, 0);
    /* followers */
#endif
    if(IS_BACKRANKED(ch))
    {
      send_to_char("How can you do that from back here?!\n", ch);
      return;
    }
    if(IS_FIGHTING(ch))
    {
      send_to_char("You are fighting for your life!\n", ch);
      return;
    }

  if(IS_NPC(ch) && IS_PC_PET(ch))
   return;

    for (gl = /*gl->next */ ch->group; gl; gl = gl->next)
    {
      if((gl->ch->in_room == ch->in_room) &&
          (ch->specials.z_cord == gl->ch->specials.z_cord))
        spell_word_of_recall((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    }
  }
}


int Summonable(P_char ch)
{
  int      target;

  if(!ch)
    return FALSE;

  if(IS_NPC(ch) &&
    (IS_SET(ch->specials.act, ACT_NO_SUMMON) ||
    IS_SHOPKEEPER(ch)))
      return FALSE;

  if(IS_SET(world[ch->in_room].room_flags, NO_SUMMON))
    return FALSE;

  for (target = 0; target < MAX_WEAR; target++)
    if(ch->equipment[target] &&
       IS_SET(ch->equipment[target]->extra_flags, ITEM_NOSUMMON))
          return FALSE;

  if(P_char rider = GET_RIDER(ch))
    if(IS_PC(rider))
      return FALSE;
  
  return TRUE;
}

void spell_summon(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  int      target, max_summon_level = 0;
  struct affected_type af;
  P_char   t_ch, mob;
  int      distance;

  if(!victim)
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  /*
   * cannot be summoned or summon to or from a NO_SUMMON room - Allenbri
   */

  if(IS_PC(ch) &&
     !IS_SET(world[ch->in_room].room_flags, NO_SUMMON) &&
     !number(0, 9))
  {
    mob = read_mobile(real_mobile(200), REAL);
    if(!mob)
    {
      logit(LOG_DEBUG, "spell_summon(): mob 200 (shadow) not loadable");
      send_to_char("Bug in summon. tell a god!\n", ch);
      return;
    }
    if(!IS_SET(mob->specials.act, ACT_MEMORY))
      clearMemory(mob);

    act("A sudden darkness engulfs the room, and a shape coalesces in it..\n",
        TRUE, ch, 0, 0, TO_ROOM);
    CharWait(mob, 2 * PULSE_VIOLENCE);
    char_to_room(mob, ch->in_room, 0);
    justice_witness(ch, NULL, CRIME_SUMMON);

    remember(mob, ch);

    remove_plushit_bits(mob);

    act("$n appears in a puff of acrid smoke!", TRUE, mob, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));
    af.type = SPELL_SUMMON;
    af.duration = 4;
    affect_join(mob, &af, TRUE, FALSE);

    return;
  }
  
  if(IS_SET(world[ch->in_room].room_flags, NO_SUMMON) ||
     IS_SET(world[victim->in_room].room_flags, NO_SUMMON) ||
    (IS_NPC(victim) && IS_SHOPKEEPER(victim)) ||
    (IS_PC(victim) &&
    IS_SET(victim->specials.act2, PLR2_NOLOCATE) &&
    !is_introd(victim, ch)))
  {
    if(!IS_TRUSTED(ch))
    {
      send_to_char("&+CYou failed.\n", ch);
      act("You feel a magical force tugging on you, which slowly dissipates.",
          FALSE, ch, 0, victim, TO_VICT);
      return;
    }
  }
  
  distance = (int) (level * 1.5);

  if((how_close(ch->in_room, victim->in_room, distance) < 0) &&
     (how_close(victim->in_room, ch->in_room, distance) < 0))
  {
    send_to_char("&+CYou failed.\n", ch);
    act("You feel a wrenching sensation.", FALSE, ch, 0, victim, TO_VICT);
    return;
  }
  
  if(racewar(ch, victim) ||
    (IS_PC_PET(ch) && IS_PC(victim)))
  {
    send_to_char("&+CYou failed.\n", ch);
    act("You feel a wrenching sensation.", FALSE, ch, 0, victim, TO_VICT);
    return;
  }
  
  if(!Summonable(victim) &&
     !(is_linked_to(ch, victim, LNK_CONSENT)))
  {
    send_to_char("&+CYou failed.\n", ch);
    send_to_char("You feel a wrenching sensation.\n", victim);
    return;
  }
  
  if(!IS_TRUSTED(ch) &&
      IS_NPC(victim) &&
      is_aggr_to(victim, ch))
  {
/*      (IS_SET(victim->specials.act, ACT_AGGRESSIVE) ||
    (IS_SET(victim->specials.act, ACT_AGGRESSIVE_EVIL) && IS_EVIL(ch)) ||
    (IS_SET(victim->specials.act, ACT_AGGRESSIVE_GOOD) && IS_GOOD(ch)) ||
       (IS_SET(victim->specials.act, ACT_AGGRESSIVE_NEUTRAL) &&
  IS_NEUTRAL(ch)) ||
     (IS_SET(victim->specials.act, ACT_AGG_RACEEVIL) && RACE_EVIL(ch)) ||
    (IS_SET(victim->specials.act, ACT_AGG_RACEGOOD) && RACE_GOOD(ch)))) {*/

    send_to_char("You feel a sudden surge of hatred and halt the spell.\n",
                 ch);
    return;
  }
  
  if(IS_PC(ch))
    CharWait(ch, 48);
  else
    CharWait(ch, 2 * PULSE_VIOLENCE);

  max_summon_level = level + 3;

  if((GET_LEVEL(victim) > MIN(MAXLVLMORTAL, max_summon_level)) ||
      CHAR_IN_PRIV_ZONE(ch) || CHAR_IN_SAFE_ZONE(victim) ||
     (!is_linked_to(ch, victim, LNK_CONSENT) &&
      NewSaves(victim, SAVING_SPELL, 0)))
  {
    send_to_char("&+CYou failed.\n", ch);
    send_to_char("You feel a wrenching sensation.\n", victim);
    return;
  }
  
  act("&+W$n is summoned away!", TRUE, victim, 0, 0, TO_ROOM);

  target = ch->in_room;
  
  if(IS_FIGHTING(victim))
    stop_fighting(victim);
  if(IS_DESTROYING(victim))
    stop_destroying(victim);
  
  if(victim->in_room != NOWHERE)
    for (t_ch = world[victim->in_room].people; t_ch; t_ch = t_ch->next)
      if(IS_FIGHTING(t_ch) && (t_ch->specials.fighting == victim))
        stop_fighting(t_ch);
  
  if(IS_PC(victim) &&
     IS_RIDING(victim))
        stop_riding(victim);
  
  if(P_char rider = GET_RIDER(victim))
     stop_riding(rider);
        
  act("$n &+Whas summoned you!", FALSE, ch, 0, victim, TO_VICT);
  char_from_room(victim);
  char_to_room(victim, target, -1);
  victim->specials.z_cord = ch->specials.z_cord;
  act("$n &+Warrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
}

void charm_generic(int level, P_char ch, P_char victim)
{
  struct affected_type af;
  int      i;
  int      c;                   // count of druid followers
  bool     failed = FALSE;
  struct follow_type *followers;
  int      num;

  if(GET_STAT(ch) == STAT_DEAD)
    return;

  if(resists_spell(ch, victim))
    return;

  if(victim == ch)
  {
    send_to_char("You contemplate how charming you are.\n", ch);
    return;
  }
  if(circle_follow(victim, ch))
  {
    send_to_char("Sorry, following in circles cannot be allowed.\n", ch);
    return;
  }
  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("You wouldn't even consider that, would you?\n", ch);
    return;
  }
  if(IS_PC(ch))
    CharWait(ch, PULSE_VIOLENCE);

  if(!CAN_SEE(victim, ch))
    failed = TRUE;

  if(IS_SHOPKEEPER(victim))
    failed = TRUE;

  num = 0;
  c = 0;
  /* can only have 1 charmie now... because the charmies have full mob
     stats */
  for (i = 0; (i < MAX_WEAR) && !failed; i++)
    if(victim->equipment[i] &&
        IS_SET(victim->equipment[i]->extra_flags, ITEM_NOCHARM))
      failed = TRUE;

  // druids get 15 lvls and below animals, everyone else 2/3 lvl
  if(GET_CLASS(ch, CLASS_DRUID) || (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))
  {
    if((GET_LEVEL(victim) > (GET_LEVEL(ch) - 10)))
    {
      failed = TRUE;
    }
  }
  else if((level * 2 / 3) < (int) GET_LEVEL(victim))
  {
    failed = TRUE;
  }

  /*
   * Moved the # of followers check below - Clav
   */

  if(!failed && GET_MASTER(victim))
  {
    /*
     * several possibilities if victim is already charmed
     */
    if(!victim->following)
    {
      /*
       * should only happen when bit is set by other than spell
       */
      failed = TRUE;
    }
    else if(level > (int) GET_LEVEL(victim->following))
    {
      /*
       * victim gets another save versus initial charm (with bonus)
       */
      if(NewSaves(victim, SAVING_PARA, GET_LEVEL(victim->following) - level))
      {
        clear_links(victim, LNK_PET);
      }
      else
        failed = TRUE;
    }
    else
      failed = TRUE;
  }
  if(!failed && saves_spell(victim, SAVING_PARA))
    failed = TRUE;

  /*
   * If a druid is 25 levels higher than an animal, it'll be an
   * automatic innate charm animal
   */
  if((GET_CLASS(ch, CLASS_DRUID) || (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID))) &&
      ((GET_LEVEL(victim) < (GET_LEVEL(ch) - 25))) && 
      is_natural_creature(victim))
  {
      failed = FALSE;
  }


  // druids can have more then one charmed animal, and it's level based!!
  for (followers = ch->followers; followers; followers = followers->next)
  {
    if(GET_CLASS(ch, CLASS_DRUID) || (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))
    {
      c++;
      if(GET_LEVEL(ch) <= 30)
      {
        if(c >= 4)
        {
          failed = TRUE;
        }
      }
      if((GET_LEVEL(ch) > 30) && (GET_LEVEL(ch) <= 50))
      {
        if(c >= 5)
        {
          failed = TRUE;
        }
      }

      if((GET_LEVEL(ch) >= 51) && (GET_LEVEL(ch) < 56))
      {
        if(c >= 6)
        {
          failed = TRUE;
        }
      }

      if((GET_LEVEL(ch) == 56))
      {
        if(c >= 7)
        {
          failed = TRUE;
        }
      }
    }
    else if(followers->follower &&
             affected_by_spell(followers->follower, SPELL_CHARM_PERSON))
    {
      failed = TRUE;
      /*    everyone else can have one follower .. */
    }
  }

  if( IS_TRUSTED(ch) )
  {
      failed = FALSE;
  }

  if(failed)
  {
    send_to_char
      ("Your victim doesn't seem to find you particularly charming...\n", ch);
    act("$n tried to charm you, but failed!", FALSE, ch, 0, victim, TO_VICT);

    if(CAN_SEE(victim, ch))
    {
      /* if they fail, wham! */
      remember(victim, ch);

#ifndef NEW_COMBAT
      hit(victim, ch, ch->equipment[PRIMARY_WEAPON]);
#else
      hit(victim, ch, victim->equipment[WIELD], TYPE_UNDEFINED,
          getBodyTarget(victim), TRUE, FALSE);
#endif
    }
    return;
  }
  /*
   * if get here, spell worked, so do the nasty thing
   */

  if(victim->following && (victim->following != ch))
    stop_follower(victim);

  // Uncharm victim's pets.
  while( victim->followers )
  {
    clear_links(victim->followers->follower, LNK_PET);
  }

  if(!victim->following)
    add_follower(victim, ch);

  /* the duration maxes out at GET_LEVEL(ch)/10 mud days. */
  if(GET_C_INT(victim))
    num = MIN((GET_LEVEL(ch) / 10 * 24) + 1, 200 / STAT_INDEX(GET_C_INT(victim)));
  else
    num = 4;

  if( IS_TRUSTED(ch) )
    num = (num > 40) ? num : 40;

  setup_pet(victim, ch, num, 0);

  act("You stand enthralled by $n's charming personality...",
      FALSE, ch, 0, victim, TO_VICT);

  if(IS_FIGHTING(victim))
    stop_fighting(victim);
  if(IS_DESTROYING(victim))
    stop_destroying(victim);

  /*
   * stop all non-vicious/agg attackers
   */
  StopMercifulAttackers(victim);
}

void spell_sense_life(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_SENSE_LIFE))
  {
    send_to_char("&+LYour feel your awareness improve.\n", ch);

    bzero(&af, sizeof(af));
    af.type = SPELL_SENSE_LIFE;
    af.duration = level;
    af.bitvector = AFF_SENSE_LIFE;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SENSE_LIFE)
      {
        af1->duration = level;
      }
  }

}

void spell_blur(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{
  struct affected_type af;
  struct affected_type *af1;

  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }

  if(GET_CLASS(victim, CLASS_MONK))
  {
    send_to_char("Your monkliness makes blur non-blur-like.\n", victim);
    return;
  }
  
  if(!affected_by_spell(victim, SPELL_BLUR))
  {
    send_to_char("&+BYour skin crackles with lightning as your form starts to &N&+Wblur.&N\n", victim);
    act("$n becomes a &+bBLUR&N of speed!", TRUE, victim, 0, 0, TO_ROOM);
    bzero(&af, sizeof(af));
    af.type = SPELL_BLUR;
    af.duration = 10;
    af.bitvector3 = AFF3_BLUR;
    affect_to_char(victim, &af);
  }
  else if(affected_by_spell(victim, SPELL_BLUR))
  {
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_BLUR)
      {
        send_to_char("Your blurring speed is revivified.&n\n", victim);
        af1->duration = 10;
      }
    }
  }
  return;
}


void spell_lodestone_vision(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{
  if (IS_AFFECTED5(ch, AFF5_MINE))
  {
    if (ch == victim)
      send_to_char("You are already under the influence of miners sight.\r\n", ch);
    act("$N is already under the influence of miners sight.", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  
  if (!affected_by_spell(victim, SPELL_LODESTONE))
  {
    act("A faint glimmer passes across your eyes as you feel your ability to detect minerals improve.", FALSE, ch, 0, victim, TO_CHAR);
    struct affected_type af;
    bzero(&af, sizeof(af));
    af.type = SPELL_LODESTONE;
    af.bitvector5 = AFF5_MINE;
    af.duration = number(5, 7);
    affect_to_char(victim, &af);
  }
}

void spell_haste(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{

  if(affected_by_spell(victim, SPELL_SLOW))
  {
    affect_from_char(victim, SPELL_SLOW);

    act("$n regains $s former speed.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&+mYou feel your speed return to normal.\r\n", victim);
    return;
  }

  if(IS_AFFECTED(victim, AFF_HASTE))
  {
    act("$N is already under the influence of &+Yhaste.&n",
      TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if(!affected_by_spell(victim, SPELL_HASTE))
  {
    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR) )
    {
      send_to_char("&+RYou feel the speed of the &+Cwind&+R rushing through your heart!\n", victim);
      act("$n &+Rstarts to move with the speed of the &+Cwind&+R!", TRUE, victim, 0, 0, TO_ROOM);
    }
    else
    {
      send_to_char("&+RYou feel your heart start to race REAL FAST!\n", victim);
      act("$n &+Rstarts to move with uncanny speed!", TRUE, victim, 0, 0, TO_ROOM);
    }

    struct affected_type af;
    bzero(&af, sizeof(af));
    af.type = SPELL_HASTE;
    af.bitvector = AFF_HASTE;
    
    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR))
      af.duration = (int)(level/2);
    else if(GET_CLASS(ch, CLASS_RANGER) ||
             GET_CLASS(ch, CLASS_REAVER) ||
             GET_CLASS(ch, CLASS_CONJURER) ||
             GET_CLASS(ch, CLASS_SORCERER))
      af.duration = 15;
    else
      af.duration = 10;

    affect_to_char(victim, &af);
  }
}

void spell_recharger(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj tar_obj)
{
  struct affected_type af;
  int      mod, m_points;

  if(affected_by_spell(victim, SPELL_RECHARGER))
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }
  if(victim == ch)
  {
    send_to_char("How can you recharge yourself, silly?\n", ch);
    return;
  }
/*  if(resists_spell(ch, victim))
   return;
 */

  mod = (GET_LEVEL(ch) - GET_LEVEL(victim)) / 2;

  bzero(&af, sizeof(af));
  af.type = SPELL_RECHARGER;
  af.duration = (((mod > 0 ? mod : -mod) + 1));

  if(!NewSaves(victim, SAVING_PARA, mod))
  {
    send_to_char("You feel drained.\n", victim);
    send_to_char("You feel recharged.\n", ch);
    m_points = BOUNDED(0, (GET_MANA(victim) / 2 + mod * 2), GET_MANA(victim));

    victim->points.mana -= m_points;
    ch->points.mana +=
      number(GET_LEVEL(ch) / 2, GET_LEVEL(ch)) * m_points / 100;

    affect_to_char(victim, &af);
  }
  else
  {                             /*
                                 * Muhahahaha!
                                 */
    mod = -mod - 2;
    if(!NewSaves(ch, SAVING_SPELL, mod))
    {
      send_to_char("BACKFIRE!!  you feel your power draining away!\n", ch);
      send_to_char("You feel recharged!\n", victim);
      m_points = BOUNDED(0, (GET_MANA(ch) / 3 + mod), GET_MANA(ch));

      victim->points.mana += m_points;
      ch->points.mana -= m_points;

      af.duration = MAX(1, ((mod > 0 ? mod : -mod) - 1));
      affect_to_char(ch, &af);
    }
    else
    {
      send_to_char("You fail to drain any mana\n", ch);
      send_to_char("You feel a brief inner tug, which passes quickly.\n",
                   victim);
    }
  }
}

/* void spell_vigorize_light(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = number(4, 15);

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;

  update_pos(victim);

  send_to_char("You feel a bit more invigorated!\n", victim);
} */

void spell_vigorize_critic(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  spell_invigorate(level, ch, 0, SPELL_TYPE_SPELL, victim, 0);
  return;
}

void spell_vigorize_serious(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
// For backwards compatibility for various potions and others. 
  spell_invigorate((int)(level * 0.66), ch, 0, SPELL_TYPE_SPELL, victim, 0);
  return;
}

void spell_vigorize_light(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
// For backwards compatibility for various potions and others.
  spell_invigorate((int)(level * 0.33), ch, 0, SPELL_TYPE_SPELL, victim, 0);
  return;
}

/* void spell_vigorize_serious(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = dice(3, (level / 3));

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;

  send_to_char("You feel much more invigorated!\n", victim);

  update_pos(victim);
} */

// This single spell replaces all the old vigorize spells, which was not a real word.

void spell_invigorate(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int movepoints, in_room;
  
  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim) ||
     ch->in_room != victim->in_room)  
      return;

  movepoints = dice(3, level);
  
  if(GET_CLASS(ch, CLASS_CLERIC))
    movepoints = (int)(movepoints * 1.5);
  else if(GET_CLASS(ch, CLASS_RANGER))
    movepoints = (int)(movepoints * 1.15);
  else if(GET_CLASS(ch, CLASS_PALADIN | CLASS_ANTIPALADIN))
    movepoints = (int)(movepoints * .95);
  else
    movepoints = (int)(movepoints * 0.80);

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
  {
/* Old debugging message:
    if( GET_VITALITY(victim) != GET_MAX_VITALITY(victim) )
    {
      debug("INVIGORATE: Movement points (%d) %s to %s.",
        GET_MAX_VITALITY(victim) - GET_VITALITY(victim), GET_NAME(ch), GET_NAME(victim));
    }
*/
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  }
  else
  {
/* Old debugging message:
    debug("INVIGORATE: Movement points (%d) %s to %s.", movepoints, GET_NAME(ch), GET_NAME(victim));
*/
    GET_VITALITY(victim) += movepoints;
  }

  act("You &+Winvigorate&n $N with renewed &+cenergy.&n",
    FALSE, ch, 0, victim, TO_CHAR);
  act("$N &+Wappears invigorated.&n",
    FALSE, ch, 0, victim, TO_NOTVICT);
  act("&+yFresh energy pours through your body. &+WYou are invigorated!&n",
    FALSE, ch, 0, victim, TO_VICT);

  update_pos(victim);
}


/* void spell_vigorize_critic(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = (level / 2) + dice(4, (level / 4));

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;
    
    act("",
       FALSE, ch, 0, 0, TO_ROOM);
    act
      ("&+WThe orb of light begins to take shape... a mounting sense of awe grips the area.",
       FALSE, ch, 0, 0, TO_CHAR);

  send_to_char("You feel invigorated!\n", victim);
} */

void spell_devitalize(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = dice(3, (level / 3));

  if((GET_VITALITY(victim) - movepoints) < 20)
  {
    GET_VITALITY(victim) = 20;
    return;
  }
  GET_VITALITY(victim) -= movepoints;

  act("&+L$n&+L drains $N's&+L stamina!", FALSE, ch, 0, victim, TO_NOTVICT);
  act("&+LYou drain $N's &+Lstamina!", FALSE, ch, 0, victim, TO_CHAR);
  act("&+L$n&+L drains your stamina!", FALSE, ch, 0, victim, TO_VICT);

  update_pos(victim);
}

void spell_channel(int level, P_char ch, P_char victim, P_obj obj)
{
  char     Gbuf[MAX_STRING_LENGTH];
  P_char   vict, avatar;
  P_event  ev, e_save;
  struct affected_type new_af;
  int      room;
  snoop_by_data *snoop_by_ptr;

  if(!ch || !victim || !obj)
    return;

  room = ch->in_room;

  obj->timer[0]++;

  if(obj->timer[0] > 5 && obj->timer[0] < 10)
  {
    act("$p &+Cbegins to form small particles... a shape can now be seen...",
        FALSE, ch, 0, 0, TO_ROOM);
    act("$p &+Cbegins to form small particles... a shape can now be seen...",
        FALSE, ch, 0, 0, TO_CHAR);
  }
  else if(obj->timer[0] > 10 && obj->timer[0] < 20)
  {
    if(IS_EVIL(ch))
    {
      act("&+LThe orb of darkness begins to take shape... a mounting sense of dread grips the area.",
         FALSE, ch, 0, 0, TO_ROOM);
      act("&+LThe orb of darkness begins to take shape... a mounting sense of dread grips the area.",
         FALSE, ch, 0, 0, TO_CHAR);
    }
    else
    {
      act("&+WThe orb of light begins to take shape... a mounting sense of awe grips the area.",
         FALSE, ch, 0, 0, TO_ROOM);
      act("&+WThe orb of light begins to take shape... a mounting sense of awe grips the area.",
         FALSE, ch, 0, 0, TO_CHAR);
    }
  }
  else
  {
    act("$p &n&+bbriefly flickers into view...", FALSE, ch, 0, 0, TO_ROOM);
    act("$p &n&+bbriefly flickers into view...", FALSE, ch, 0, 0, TO_CHAR);
  }

  if(obj->timer[0] > 13 && ch == victim)
  {
    // The shit hits the fan here

    // Now switch the leader into the avatar!
    if(IS_EVIL(ch))
      avatar = morph(ch, EVIL_AVATAR_MOB, VIRTUAL);
    else
      avatar = morph(ch, GOOD_AVATAR_MOB, VIRTUAL);

    if(!avatar)
    {
      send_to_char("You are unable to complete the channeling.\n", ch);
      return;
    }

    // get rid of avatar obj
    act("$n &+Bis born from the &n$p!!!", FALSE, avatar, obj, victim, TO_ROOM);
    extract_obj(obj);

    act("&+Y$n's spirit leaves $s body and enters &n$N", FALSE, ch, 0, avatar, TO_ROOM);
    bzero(&new_af, sizeof(new_af));
    new_af.type = SPELL_CHANNEL;
    new_af.duration = 24;
    new_af.location = APPLY_NONE;
    new_af.flags = AFFTYPE_NODISPEL;
    affect_join(avatar, &new_af, FALSE, FALSE);

    // Make helpers in room abort spell, and knock them out
    LOOP_THRU_PEOPLE(vict, ch)
    {
      if(ch == vict ||
          (is_linked_to(ch, victim, LNK_CONSENT) &&
           GET_CLASS(vict, CLASS_CLERIC)))
      {
        struct affected_type af;

        if(IS_CASTING(vict))
          StopCasting(vict);
        act("Severely drained by the summoning, you collapse.", FALSE, vict,
            0, 0, TO_CHAR);
        act("$n is severely drained by the summoning, and collapses.", FALSE,
            vict, 0, 0, TO_ROOM);
        stop_fighting(vict);
        if( IS_DESTROYING(vict) )
          stop_destroying(vict);
        StopMercifulAttackers(vict);
        bzero(&af, sizeof(af));
        af.type = SPELL_CHANNEL;
        af.duration = 2;
        af.location = APPLY_NONE;
        af.bitvector = AFF_KNOCKED_OUT;
        af.flags = AFFTYPE_NODISPEL;
        affect_join(vict, &af, FALSE, FALSE);
        SET_POS(vict, POS_PRONE + GET_STAT(vict));
        vict->only.pc->pc_timer[3] = time(NULL);
        if(ch != vict && avatar->desc)
        {
          vict->desc->snoop.snooping = avatar;
          CREATE(snoop_by_ptr, snoop_by_data, 1, MEM_TAG_SNOOP);
          bzero(snoop_by_ptr, sizeof(snoop_by_data));
          snoop_by_ptr->next = avatar->desc->snoop.snoop_by_list;
          snoop_by_ptr->snoop_by = vict;
          avatar->desc->snoop.snoop_by_list = snoop_by_ptr;
        }
      }
    }
  }
}

void spell_miracle(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  struct group_list *gl;

  if(ch && ch->group)
  {
    gl = ch->group;
    /* leader first */
    if(gl->ch->in_room == ch->in_room)
      spell_vitality(50, ch, NULL, 0, gl->ch, 0);
    spell_armor(50, ch, 0, 0, gl->ch, 0);
    spell_bless(50, ch, 0, 0, gl->ch, 0);
    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if(gl->ch->in_room == ch->in_room)
        spell_vitality(50, ch, NULL, 0, gl->ch, 0);
      spell_armor(50, ch, 0, 0, gl->ch, 0);
      spell_bless(50, ch, 0, 0, gl->ch, 0);
    }
  }
}

void spell_death_blessing(int level, P_char ch, char *args, int type,
                          P_char victim, P_obj obj)
{
  struct group_list *gl;

  if(ch && ch->group)
  {
    gl = ch->group;
    /* leader first */
    if(gl->ch->in_room == ch->in_room)
      spell_vitalize_undead(56, ch, NULL, 0, gl->ch, 0);
    spell_protection_from_living(50, ch, 0, 0, gl->ch, 0);
    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if(gl->ch->in_room == ch->in_room)
        spell_vitalize_undead(50, ch, NULL, 0, gl->ch, 0);
      spell_protection_from_living(50, ch, 0, 0, gl->ch, 0);

    }
  }
}


void spell_vitality(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;
  int  healpoints = 4 * level, duration = 1;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
      return;
  
  if(IS_NPC(victim) &&
     GET_VNUM(victim) == IMAGE_REFLECTION_VNUM)
      return;

  if(affected_by_spell(victim, SPELL_MIELIKKI_VITALITY))
  {
    send_to_char("&+GThe Goddess Mielikki is aiding your health, and prevents the vitality spell from functioning...\r\n", victim);
    return;
  }

  if(affected_by_spell(victim, SPELL_FALUZURES_VITALITY))
  {
    send_to_char("&+LThe God &+yFa&+Lluz&+yure&+L is aiding your health, and prevents the vitality spell from functioning...\r\n", victim);
    return;
  }

  if(affected_by_spell(victim, SPELL_ESHABALAS_VITALITY))
  {
    send_to_char("&+rThe blessings of the vitality spell are denied by Eshabala!\r\n", victim);
    return;
  }

  if(!IS_PC(ch) && !IS_PC(victim))
    duration = 30;
  else
    duration = (int)(MAX(10, GET_LEVEL(ch) / 4));

  if(!affected_by_spell(victim, SPELL_VITALITY))
  {
    send_to_char("&+BYou feel vitalized.\n", victim);
    act("$N looks vitalized.&n",
      TRUE, ch, 0, victim, TO_NOTVICT);

    if(ch != victim)
      act("You vitalize $N.&n",
        TRUE, ch, 0, victim, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_VITALITY;
    af.duration = duration;
    af.modifier = healpoints;
    af.location = APPLY_HIT;

    affect_to_char(victim, &af);

    update_pos(victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_VITALITY)
      {
        send_to_char("&+WYou feel a slight &+cmagical surge&+W that reinforces and refreshes your &+Bvitality.\r\n&n", 
          victim);
        af1->duration = duration;
      }
  }
}

void spell_vitalize_undead(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct affected_type af;
  int healpoints = 2 * level;

// Modied 02/05/15 - was returning false positives. 
   if( !IS_UNDEADRACE(victim) && !IS_ANGEL(victim)
    && GET_RACE(victim) != RACE_GOLEM && !GET_CLASS(victim, CLASS_NECROMANCER)) 
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }
  if(IS_PC(victim))
    healpoints = healpoints;

  if(!affected_by_spell(victim, SPELL_VITALIZE_UNDEAD))
  {
    if (GET_CLASS(ch, CLASS_THEURGIST))
    {
      act("The &+Wholy powers&n of &+WH&+Yei&+Wr&+Yo&+Rn&+Yiou&+Rs&n strengthens $n's physical being.", FALSE, victim, 0, 0, TO_ROOM);
      send_to_char("&+WYou feel your spirit gain strength.\n", victim);
    }
    else
    {
      act
      ("&+LDarkness seems to encompass $n&+L, and begins to permeate $s rotting flesh!&n",
        FALSE, victim, 0, 0, TO_ROOM);
      send_to_char("&+WYou feel your bones gain strength.\n", victim);
    }

    bzero(&af, sizeof(af));
    af.type = SPELL_VITALIZE_UNDEAD;
    af.duration = KludgeDuration(ch, 25, 10);
    af.modifier = healpoints;
    af.location = APPLY_HIT;

    affect_to_char(victim, &af);

    update_pos(victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_VITALIZE_UNDEAD)
      {
        af1->duration = KludgeDuration(ch, 25, 10);
      }
  }

}

void spell_farsee(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_FARSEE))
  {
    act("&+CYour eyes begin to tingle.", TRUE, victim, 0, 0, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_FARSEE;
    af.duration = level * 2;
    af.bitvector = AFF_FARSEE;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_FARSEE)
      {
        af1->duration = level * 2;
      }
  }

}

void spell_group_globe(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct group_list *gl;

  if(ch && ch->group)
  {
    gl = ch->group;
    /* leader first */
    if(gl->ch->in_room == ch->in_room)
      spell_globe((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if(gl->ch->in_room == ch->in_room)
        spell_globe((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    }
  }
}


void spell_group_stornog(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct group_list *gl;

  if(ch && ch->group)
  {
    gl = ch->group;
    /* leader first */
    if(gl->ch->in_room == ch->in_room)
      spell_stornogs_spheres(level, ch, 0, 0, gl->ch, 0);
    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if(gl->ch->in_room == ch->in_room)
        spell_stornogs_spheres(level, ch, 0, 0, gl->ch, 0);
    }
  }
}



void spell_globe(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED2(victim, AFF2_GLOBE))
    return;

  if(!affected_by_spell(victim, SPELL_GLOBE))
  {
    act("&+R$n &+Rbegins to shimmer.", TRUE, victim, 0, 0, TO_ROOM);
    act("&+RYou begin to shimmer.", TRUE, victim, 0, 0, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_GLOBE;
    if(GET_CLASS(ch, CLASS_CONJURER))
      af.duration = 15;
    else if(GET_CLASS(ch, CLASS_SUMMONER))
      af.duration = 12;
    else
      af.duration = 8;
    af.bitvector2 = AFF2_GLOBE;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_GLOBE)
      {
        af1->duration = 8;
      }
  }

}

void spell_minor_globe(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED(victim, AFF_MINOR_GLOBE))
    return;

  if(!affected_by_spell(victim, SPELL_MINOR_GLOBE))
  {
    act("&+r$n &+rbegins to shimmer.", TRUE, victim, 0, 0, TO_ROOM);
    act("&+rYou begin to shimmer.", TRUE, victim, 0, 0, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_MINOR_GLOBE;
    af.duration = 6;
    af.bitvector = AFF_MINOR_GLOBE;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_MINOR_GLOBE)
      {
        af1->duration = 6;
      }
  }

}

void spell_deflect(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED4(victim, AFF4_DEFLECT))
    return;

  if(IS_NPC(ch) &&
     GET_VNUM(ch) == 250)
  {
    act("&+cA &+Ctranslucent&n&+c field surrounds&n $n &+cthen the field crumbles into &+Rsparks!&n",
       TRUE, victim, 0, 0, TO_ROOM);
      return;
  }
  
  if(!affected_by_spell(victim, SPELL_DEFLECT))
  {
    act
      ("&+cA &+Ctranslucent&n&+c field flashes around $n&n&+c, then vanishes.",
       TRUE, victim, 0, 0, TO_ROOM);
    act
      ("&+cA &+Ctranslucent&n&+c field briefly flashes around your body, then fades.",
       TRUE, victim, 0, 0, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_DEFLECT;
    af.duration = -1;
    af.bitvector4 = AFF4_DEFLECT;
    affect_to_char(victim, &af);
  }
}

void spell_air_form(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED2(victim, AFF2_AIR_AURA))
    return;

  if(!affected_by_spell(victim, SPELL_AIR_FORM))
  {
    act
      ("&+CThe image of &+W$n &+Wb&+cl&+wu&+Cr&+ws &+Cand starts to &+cfade &+Win &+Cand &+Lout &+Cof e&+wx&+ci&+Ws&+ct&+we&+Cn&+wc&+Ce.&N",
       TRUE, victim, 0, 0, TO_ROOM);
    act
      ("&+CYou feel as if your &+cm&+Wo&+wl&+We&+cc&+Wu&+wl&+ce&+Ws &+Cstart dr&+cift&+Ling.&N",
       TRUE, victim, 0, 0, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_AIR_FORM;
    af.duration = 8;
    af.bitvector2 = AFF2_AIR_AURA;
    affect_to_char(victim, &af);
  }
}

/*
void event_plague(P_char ch, P_char vict, P_obj obj, void *data)
{
  P_char target;
  int timer, both = FALSE;
  struct affected_type af;

  if(!affected_by_spell(ch, SPELL_PLAGUE) && !IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE))
    return;

  if(affected_by_spell(ch, SPELL_PLAGUE) && affected_by_spell(ch, SPELL_DISEASE))
  {
    act("$n's &+rin&+Rfec&+rtio&+Rus w&+roun&+Rds&n blister and pop spreading the &+Gplague&n everywhere!", TRUE, ch, 0, 0, TO_ROOM);
    act("Your &+rin&+Rfec&+rtio&+Rus w&+roun&+Rds&n blister and pop spreading the &+Gplague&n everywhere!", TRUE, ch, 0, 0, TO_CHAR);
    both = TRUE;
  }

  if(affected_by_spell(ch, SPELL_PLAGUE) && !affected_by_spell(ch, SPELL_DISEASE) &&
      !IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE))
  {
    spell_disease(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, 0);
  }

  for (target = world[ch->in_room].people; target; target = target->next_in_room)
  {
    if(IS_TRUSTED(target))
      continue;
    if(IS_UNDEADRACE(target))
      continue;

    if(!affected_by_spell(target, SPELL_PLAGUE) &&
        number(0, 100) < ((int)get_property("spell.plague.spread.perc", 30)+(both ? 30 : 0)))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_PLAGUE;
      af.duration = (int)get_property("spell.plague.duration", 10);
      af.modifier = 500;
      affect_to_char(target, &af);

      if(!get_scheduled(target, event_plague))
        add_event(event_plague, WAIT_SEC * 1, target, 0, 0, 0, 0, 0);
    }
  }

  timer = (int)get_property("spell.plague.eventTime", 60);
  if(affected_by_spell(ch, SPELL_DISEASE))
    timer -= 20;
  timer += number(-10, 10);
  if(IS_FIGHTING(ch))
    timer -= 30;
  if(timer < 5)
    timer = 5;

  add_event(event_plague, WAIT_SEC * timer, ch, 0, 0, 0, 0, 0);
}

*/

void spell_plague(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  struct affected_type af;
  //int timer;
  
  if(!ch)
  {
    logit(LOG_EXIT, "spell_plague called in magic.c with no ch");
    raise(SIGSEGV);
  }

  if(!IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  if(ch == victim)
  {
    send_to_char("Your god refuses your wish!\r\n", ch);
    return;
  }
  
  if(affected_by_spell(victim, SPELL_PLAGUE))
  {
    send_to_char("This person is all sick already!\n", ch);
    return;
  }
  
  if(IS_NPC(victim) && !NewSaves(victim, SAVING_SPELL, 5))
  {  
    act("$n's skin &+cpales&n and sweat drips down $s body.",
      TRUE, victim, 0, 0, TO_ROOM);
    act("Upon $n's touch you suddenly feel sick.",
      TRUE, ch, 0, victim, TO_VICT);

    bzero(&af, sizeof(af));
    af.type = SPELL_PLAGUE;
    af.duration = level / 12;
    af.modifier = 3500;
    affect_to_char(victim, &af);
  }
  else if(IS_PC(victim) && !NewSaves(victim, SAVING_SPELL, 1))
  {  
    act("$n's skin &+cpales&n and sweat drips down $s body.",
      TRUE, victim, 0, 0, TO_ROOM);
    act("Upon $n's touch you suddenly feel sick.",
      TRUE, ch, 0, victim, TO_VICT);

    bzero(&af, sizeof(af));
    af.type = SPELL_PLAGUE;
    af.duration = (int)get_property("spell.plague.duration", 2);
    af.modifier = 500;
    affect_to_char(victim, &af);
  }
  else
  {
    act("&+LYour plague fails to afflict $N", FALSE, ch, 0, victim, TO_CHAR);
  }

  //timer = (int)get_property("spell.plague.spread.time", 60);
  //if(!get_scheduled(victim, event_plague))
  //  add_event(event_plague, WAIT_SEC * number(timer-20, timer+20), victim, 0, 0, 0, 0, 0);
}

/*
void spell_windstrom_blessing(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_WINDSTROM_BLESSING))
  {
    send_to_char("&+CWindstrom&n has blessed your blade already!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_WINDSTROM_BLESSING;
  af.duration = 5;
  affect_to_char(ch, &af);
}
*/
void spell_fear(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{

  if(GET_STAT(ch) == STAT_DEAD)
    return;

  if(affected_by_spell(victim, SKILL_BERSERK) || resists_spell(ch, victim))
    return;

  /*
   * Added by DTS 7/18/95
   */
  if(IS_DEMON(victim) || IS_DRAGON(victim) || IS_UNDEADRACE(victim) ||
      IS_TRUSTED(victim))
  {

    act("Your attempt to scare $N only makes $M laugh!", FALSE, ch, 0, victim,
        TO_CHAR);

    return;
  }
  if(!NewSaves(victim, SAVING_FEAR, (IS_ELITE(ch) ? 20 : 0)) && !fear_check(victim))
  {

    act("You scare the bejesus out of $N!", FALSE, ch, 0, victim, TO_CHAR);
    act("A wave of utter terror overcomes you as $n's power fully hits you!", FALSE,
        ch, 0, victim, TO_VICT);
    act("$n scares the bejesus out of $N!", FALSE, ch, 0, victim, TO_NOTVICT);

    do_flee(victim, 0, 2);
  }
  else
  {
    act("$N doesn't look frightened..  $E DOES look kinda mad though.",
        FALSE, ch, 0, victim, TO_CHAR);
    act("$N &+wsteels $sself against the fearsome visage and presses $s attack.",
        TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n &+Lseems to grow in size and power, but you ignore the danger and press your attack.",
        TRUE, ch, 0, victim, TO_VICT);
  }
  if(ch->in_room == victim->in_room)
  {
    /*
     * they didn't flee, know what happens when you corner a scared
     * rat?
     */
    if(IS_NPC(victim) && CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
        MobStartFight(victim, ch);
    }
  }
}


void spell_reveal_true_name(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  char     newname = FALSE;
  char     *eol;

  if( obj_index[obj->R_num].virtual_number == VOBJ_RANDOM_WEAPON && identify_random(obj) )
  {
    act("You discover that you are in fact in possession of $p!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if(isname("_id_", obj->name) && !obj->str_mask)
  {                             /* some special ID info */
    struct extra_descr_data *ex;

    for (ex = obj->ex_description; ex; ex = ex->next)
    {                           /* find new name */
      if(isname("_id_name_", ex->keyword))
        break;
    }
    if(ex)
    {                           /* restring name */
      obj->name = str_dup(ex->description);
      if(eol = rindex(obj->name, '\r'))
        *eol = '\0';
      SET_BIT(obj->str_mask, STRUNG_KEYS);
      newname = TRUE;
    }
    for (ex = obj->ex_description; ex; ex = ex->next)
    {                           /* find new short */
      if(isname("_id_short_", ex->keyword))
        break;
    }
    if(ex)
    {                           /* restring short desc */
      /* free(obj->short_description); */
      obj->short_description = str_dup(ex->description);
      if(eol = rindex(obj->short_description, '\r'))
        *eol = '\0';
      SET_BIT(obj->str_mask, STRUNG_DESC2);
      newname = TRUE;
    }
    for (ex = obj->ex_description; ex; ex = ex->next)
    {                           /*find new desc */
      if(isname("_id_desc_", ex->keyword))
        break;
    }
    if(ex)
    {                           /* restring description */
      /*free(obj->description); */
      obj->description = str_dup(ex->description);
      if(eol = rindex(obj->description, '\r'))
        *eol = '\0';
      SET_BIT(obj->str_mask, STRUNG_DESC1);
      newname = TRUE;
    }
  }
  else
  {
    send_to_char("No more information can be gleaned about that item.\n", ch);
    return;
  }

  if( newname )
  {
    send_to_char("You glean the true nature of the item and rename it thusly.\n", ch);
  }
  else                          // this could happen..
  {
    send_to_char("No more information can be gleaned about that item.\n", ch);
  }
}

#define STRARR_ELEM   64
#define STRARR_LEN   128

void spell_identify(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  int      i, currelem, temp, inacc;
  bool     found;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH],
    Gbuf3[MAX_STRING_LENGTH];
  char     strarr[STRARR_ELEM][STRARR_LEN];

  if(obj)
  {
    bzero(strarr, STRARR_ELEM * STRARR_LEN);
    currelem = 0;

    /* based on intelligence and level of caster, inaccuracies will pop up */

    if( level < 60 )
                {
        inacc = (110 - GET_C_INT(ch)) + (number(0, 40) - level) + number(0, 3);
        if(inacc < 0)
        inacc = 0;
        if(inacc > 20)
        inacc = 20;
                } else {
                        inacc = 0;
                }

    if(level < 60 && IS_SET(obj->extra_flags, ITEM_NOIDENTIFY))
    {
      if(level < 51)
      {
        send_to_char
          ("You cannot seem to glean any information about that item.\n", ch);
        return;
      }
      else
        send_to_char
          ("This item has an enchantment designed to make it unidentifiable, but with your superior experience, you divine its true nature regardless.\n",
           ch);
    }
    sprintf(Gbuf1, "%s&n weighs %d pounds and is worth roughly %s.\n",
            obj->short_description, GET_OBJ_WEIGHT(obj),
            coin_stringv(obj->cost));
    send_to_char(Gbuf1, ch);

    if(obj->bitvector || obj->bitvector2 || obj->bitvector3 ||
        obj->bitvector4 || obj->bitvector5)
    {
      send_to_char
        ("The following abilities are granted when using this item:\n", ch);

      *Gbuf2 = '\0';

      if(obj->bitvector)
        sprintbitde(obj->bitvector, affected1_bits, Gbuf2);

      if(obj->bitvector2)
      {
        sprintbitde(obj->bitvector2, affected2_bits, Gbuf1);
        strcat(Gbuf2, Gbuf1);
      }

      if(obj->bitvector3)
      {
        sprintbitde(obj->bitvector3, affected3_bits, Gbuf1);
        strcat(Gbuf2, Gbuf1);
      }

      if(obj->bitvector4)
      {
        sprintbitde(obj->bitvector4, affected4_bits, Gbuf1);
        strcat(Gbuf2, Gbuf1);
      }

      if(obj->bitvector5)
      {
        sprintbitde(obj->bitvector5, affected5_bits, Gbuf1);
        strcat(Gbuf2, Gbuf1);
      }

      strcat(Gbuf2, "\n");
      send_to_char(Gbuf2, ch);
    }

    if(IS_SET(obj->extra2_flags, ITEM2_MAGIC))
      strcpy(strarr[currelem++], "magical");

    if(IS_SET(obj->extra_flags, ITEM_ARTIFACT))
      strcpy(strarr[currelem++], "an artifact");

    if(IS_SET(obj->extra_flags, ITEM_NOSLEEP))
      strcpy(strarr[currelem++], "protection against being slept");

    if(IS_SET(obj->extra_flags, ITEM_NOCHARM))
      strcpy(strarr[currelem++], "protection against being charmed");

    if(IS_SET(obj->extra_flags, ITEM_NOSUMMON))
      strcpy(strarr[currelem++], "protection against being summoned");

    if(IS_SET(obj->extra_flags, ITEM_FLOAT))
      strcpy(strarr[currelem++], "able to float on water");

    if(IS_SET(obj->extra_flags, ITEM_LEVITATES))
      strcpy(strarr[currelem++], "able to levitate in mid-air");

    if(IS_SET(obj->extra_flags, ITEM_TWOHANDS))
      strcpy(strarr[currelem++], "two-handed");

    if(IS_SET(obj->extra_flags, ITEM_WHOLE_BODY))
      strcpy(strarr[currelem++], "whole-body");

    if(IS_SET(obj->extra_flags, ITEM_WHOLE_HEAD))
      strcpy(strarr[currelem++], "whole-head");

    if(IS_SET(obj->extra_flags, ITEM_NODROP))
      strcpy(strarr[currelem++], "cursed");

    if(IS_SET(obj->extra2_flags, ITEM2_BLESS))
      strcpy(strarr[currelem++], "blessed");

    if(IS_SET(obj->extra_flags, ITEM_LIT))
      strcpy(strarr[currelem++], "lit");

    if(IS_SET(obj->extra_flags, ITEM_NOLOCATE))
      strcpy(strarr[currelem++], "not locateable");

    if(IS_SET(obj->extra_flags, ITEM_CAN_THROW1) ||
        IS_SET(obj->extra_flags, ITEM_CAN_THROW2))
      strcpy(strarr[currelem++], "throwable");

    if(IS_SET(obj->extra_flags, ITEM_RETURNING))
      strcpy(strarr[currelem++], "automatically returning");

    if(currelem)
      send_to_char("The item is ", ch);

    for (i = 0; i < currelem; i++)
    {
      /* last entry */

      if(i == (currelem - 1))
      {
        if(currelem > 1)
          sprintf(Gbuf2, "and %s.\n", strarr[i]);
        else
          sprintf(Gbuf2, "%s.\n", strarr[i]);
      }
      else
      {
        if(currelem > 2)
          sprintf(Gbuf2, "%s, ", strarr[i]);
        else
          sprintf(Gbuf2, "%s ", strarr[i]);
      }
      send_to_char(Gbuf2, ch);
    }

    if((obj_index[obj->R_num].func.obj && obj_index[obj->R_num].virtual_number != VOBJ_RANDOM_ARMOR) ||
        (GET_ITEM_TYPE(obj) == ITEM_WEAPON && obj->value[5] &&
         (GET_LEVEL(ch) < 56 ||
          !GET_CLASS(ch, CLASS_CONJURER))))
      send_to_char
        ("This item appears to be imbued with some magic beyond your comprehension!\n",
         ch);

    if((isname("_id_", obj->name) && !obj->str_mask) ||
        (obj_index[obj->R_num].virtual_number == VOBJ_RANDOM_WEAPON && obj->value[5] &&
         !strstr(obj->short_description, " of ")))
      act("You feel as if there is something more to $p that you can't quite reveal!",
          FALSE, ch, obj, 0, TO_CHAR);

    switch (GET_ITEM_TYPE(obj))
    {
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(Gbuf1,
              "It appears to be a %s these level %d spells:\n",
              (GET_ITEM_TYPE(obj) == ITEM_SCROLL) ?
              "scroll charged with" : "potion that grants",
              obj->value[0] + (inacc ? number(-inacc, inacc) : 0));
      send_to_char(Gbuf1, ch);

      if(obj->value[1] >= 1)
      {
        if(inacc && !number(0, 4))
          i = (obj->value[1] - number(-1, 1));
        else
          i = obj->value[1];

        if(i < 1)
          i = 0;
        else if(i > LAST_SPELL)
          i = LAST_SPELL;

        sprinttype(i, (const char **) spells, Gbuf1);
        strcat(Gbuf1, " ");
        send_to_char(Gbuf1, ch);
      }
      if(obj->value[2] >= 1)
      {
        if(inacc && !number(0, 4))
          i = (obj->value[2] - number(-1, 1));
        else
          i = obj->value[2];

        if(i < 1)
          i = 1;
        else if(i > LAST_SPELL)
          i = LAST_SPELL;

        sprinttype(i, (const char **) spells, Gbuf1);
        strcat(Gbuf1, " ");
        send_to_char(Gbuf1, ch);
      }
      if(obj->value[3] >= 1)
      {
        if(inacc && !number(0, 4))
          i = (obj->value[3] - number(-1, 1));
        else
          i = obj->value[3];

        if(i < 1)
          i = 1;
        else if(i > LAST_SPELL)
          i = LAST_SPELL;

        sprinttype(i, (const char **) spells, Gbuf1);
        send_to_char(Gbuf1, ch);
      }
      send_to_char("\n", ch);
      break;

    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(Gbuf1,
              "It appears to be a %s with %d out of %d charges left, granting the\n",
              ((GET_ITEM_TYPE(obj) == ITEM_WAND) ? "wand" : "staff"),
              obj->value[2], obj->value[1]);
      send_to_char(Gbuf1, ch);

      if(obj->value[3] >= 1)
      {
        if(inacc && !number(0, 4))
          i = (obj->value[3] - number(-1, 1));
        else
          i = obj->value[3];

        if(i < 1)
          i = 1;
        else if(i > LAST_SPELL)
          i = LAST_SPELL;

        sprinttype(i, (const char **) spells, Gbuf2);

        sprintf(Gbuf1, "level %d spell \"%s\"\n",
                obj->value[0] + (inacc ? number(-inacc, inacc) : 0), Gbuf2);
        send_to_char(Gbuf1, ch);
      }
      break;

    case ITEM_FIREWEAPON:
      sprintf(Gbuf1,
          "You magically sense that this weapons rate of fire is &+W%d&n "
          "and it's range is &+W%d&n\n", obj->value[0], obj->value[1]);
      send_to_char(Gbuf1, ch);
      break;
        case ITEM_MISSILE:
      sprintf(Gbuf1,
              "You magically sense that the damage dice for this type of arrow are '%dD%d'\n",
              obj->value[1], obj->value[2]);
      send_to_char(Gbuf1, ch);
          break;
    case ITEM_WEAPON:
      sprintf(Gbuf1,
              "You magically sense that the damage dice for this weapon are '%dD%d'\n",
              obj->value[1], obj->value[2]);
      send_to_char(Gbuf1, ch);
      if(obj_index[obj->R_num].func.obj == NULL && obj->value[5] &&
          GET_LEVEL(ch) >= 56 && GET_CLASS(ch, CLASS_CONJURER))
      {
        int      spells[3];
        char     spell_list[512];

        spells[0] = obj->value[5] % 1000;
        spells[1] = obj->value[5] % 1000000 / 1000;
        spells[2] = obj->value[5] % 1000000000 / 1000000;

        if(skills[spells[0]].name)
          strcpy(spell_list, skills[spells[0]].name);
        if(spells[1] && skills[spells[1]].name)
          sprintf(spell_list + strlen(spell_list), "&n and &+W%s",
                  skills[spells[1]].name);
        if(spells[2] && skills[spells[2]].name)
          sprintf(spell_list + strlen(spell_list), "&n and &+W%s",
                  skills[spells[2]].name);

        sprintf(Gbuf1,
                "You recognize the forces of &+W%s &nimbued within this item.\n",
                spell_list);
        send_to_char(Gbuf1, ch);
      }
      break;

    case ITEM_ARMOR:
    case ITEM_WORN:
      /* include obj affects for armor and worn items.. */

#if 0
      if(GET_ITEM_TYPE(obj) == ITEM_ARMOR)
        temp = obj->value[0];
      else
        temp = 0;
      if(temp < 0)
        strcpy(Gbuf2, "negatively");
      else if(temp == 0)
        strcpy(Gbuf2, "not at all");
      else if(temp < 6)
        strcpy(Gbuf2, "roughly half a notch");
      else if(temp < 10)
        strcpy(Gbuf2, "almost a full notch");
      else if(temp < 20)
        strcpy(Gbuf2, "at least a notch");
      else if(temp < 30)
        strcpy(Gbuf2, "at least two notches");
      else if(temp < 40)
        strcpy(Gbuf2, "at least three notches");
      else if(temp < 50)
        strcpy(Gbuf2, "at least four notches");
      else if(temp < 60)
        strcpy(Gbuf2, "at least five notches");
      else if(temp < 70)
        strcpy(Gbuf2, "at least six notches");
      else
        strcpy(Gbuf2, "well over six notches");

      sprintf(Gbuf1,
              "You mystically sense that this item will affect your AC by %s%d.\n",
              temp > 0 ? "-" : "+", temp);
      send_to_char(Gbuf1, ch);
#endif
      break;
    }

    found = FALSE;

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      if((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0))
      {

        if(!found)
        {
          send_to_char("This item also affects your:\n", ch);
          found = TRUE;
        }
        sprinttype(obj->affected[i].location, apply_types, Gbuf2);

        switch (obj->affected[i].location)
        {
        case APPLY_STR:
        case APPLY_DEX:
        case APPLY_INT:
        case APPLY_WIS:
        case APPLY_CON:
        case APPLY_AGI:
        case APPLY_POW:
        case APPLY_CHA:
        case APPLY_KARMA:
        case APPLY_LUCK:
        case APPLY_STR_MAX:
        case APPLY_DEX_MAX:
        case APPLY_INT_MAX:
        case APPLY_WIS_MAX:
        case APPLY_CON_MAX:
        case APPLY_AGI_MAX:
        case APPLY_POW_MAX:
        case APPLY_CHA_MAX:
        case APPLY_KARMA_MAX:
        case APPLY_LUCK_MAX:
        case APPLY_HIT:
          if(obj->affected[i].modifier < 0)
            strcpy(Gbuf3, "negatively by ");
          else
            strcpy(Gbuf3, "positively by ");

          temp = obj->affected[i].modifier;

          if(temp > 30)
            strcat(Gbuf3, "a whole hell of a lot");
          else if(temp > 20)
            strcat(Gbuf3, "an impressive amount");
          else if(temp > 10)
            strcat(Gbuf3, "quite a bit");
          else if(temp > 5)
            strcat(Gbuf3, "enough to mean something");
          else if(temp > 2)
            strcat(Gbuf3, "a small amount");
          else
            strcat(Gbuf3, "a negligible amount");

          break;

        case APPLY_HITROLL:
        case APPLY_DAMROLL:
          if(obj->affected[i].modifier < 0)
            strcpy(Gbuf3, "negatively by ");
          else
            strcpy(Gbuf3, "positively by ");

          temp = obj->affected[i].modifier;

          if(temp > 8)
            strcat(Gbuf3, "an impressive amount");
          else if(temp > 5)
            strcat(Gbuf3, "quite a bit");
          else if(temp > 2)
            strcat(Gbuf3, "a fair amount");
          else
            strcat(Gbuf3, "a small amount");

          break;

        case APPLY_SAVING_PARA:
        case APPLY_SAVING_ROD:
        case APPLY_SAVING_FEAR:
        case APPLY_SAVING_BREATH:
        case APPLY_SAVING_SPELL:
          if(obj->affected[i].modifier > 0)
            strcpy(Gbuf3, "negatively by ");
          else
            strcpy(Gbuf3, "positively by ");

          temp = obj->affected[i].modifier;

          if(temp > 8)
            strcat(Gbuf3, "an impressive amount");
          else if(temp > 5)
            strcat(Gbuf3, "quite a bit");
          else if(temp > 2)
            strcat(Gbuf3, "a fair amount");
          else
            strcat(Gbuf3, "a small amount");

          break;

        default:
          if(obj->affected[i].modifier < 0)
            strcpy(Gbuf3, "negatively by ");
          else
            strcpy(Gbuf3, "positively by ");

          temp = obj->affected[i].modifier;

          if(temp > 8)
            strcat(Gbuf3, "an impressive amount");
          else if(temp > 5)
            strcat(Gbuf3, "quite a bit");
          else if(temp > 2)
            strcat(Gbuf3, "a fair amount");
          else
            strcat(Gbuf3, "a small amount");

          break;
        }

        sprintf(Gbuf1, "  %s %s%d\n", Gbuf2, temp > 0 ? "by +" : "by ", temp);
        send_to_char(Gbuf1, ch);
      }
    }
    sprintf( Gbuf1, "$p &nhas an item value of &+W%d&n.", itemvalue(obj) );
    act( Gbuf1, FALSE, ch, obj, 0, TO_CHAR );
  }
  else
  {
    send_to_char("This spell only works on objects, sorry.\n", ch);
    return;
  }
}

#undef STRARR_ELEM
#undef STRARR_LEN

void spell_lore(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{
  int      i, percent = 0;
  bool     found;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH], Gbuf3[256];

  if(obj)
  {
    if(IS_SET(obj->extra_flags, ITEM_NOIDENTIFY))
    {
      if(GET_LEVEL(ch) < AVATAR)
      {
        send_to_char
          ("You recall no legends nor stories ever told about this item.",
           ch);
        return;
      }
    }
    sprintf(Gbuf1, "'%s'\nWeight %d, Item type: ",
            obj->short_description, GET_OBJ_WEIGHT(obj));
    sprinttype(GET_ITEM_TYPE(obj), item_types, Gbuf2);
    strcat(Gbuf1, Gbuf2);
    strcat(Gbuf1, "\n");
    send_to_char(Gbuf1, ch);

    if(obj->bitvector || obj->bitvector2)
    {
      if(obj->bitvector)
        sprintbitde(obj->bitvector, affected1_bits, Gbuf1);

      if(obj->bitvector2)
      {
        sprintbitde(obj->bitvector2, affected2_bits, Gbuf2);
        strcat(Gbuf1, Gbuf2);
      }

      send_to_char("Item will give you following abilities:  ", ch);
      strcat(Gbuf1, "\n");
      send_to_char(Gbuf1, ch);
    }
    send_to_char("Item is: ", ch);
    sprintbitde(obj->extra_flags, extra_bits, Gbuf1);
    strcat(Gbuf1, "\n");
    send_to_char(Gbuf1, ch);

    switch (GET_ITEM_TYPE(obj))
    {

    case ITEM_SCROLL:
    case ITEM_POTION:
      send_to_char("Contains spells of: ", ch);
      if(obj->value[1] >= 1)
      {
        sprinttype(obj->value[1], (const char **) spells, Gbuf1);
        strcat(Gbuf1, "\n");
        send_to_char(Gbuf1, ch);
      }
      if(obj->value[2] >= 1)
      {
        sprinttype(obj->value[2], (const char **) spells, Gbuf1);
        strcat(Gbuf1, "\n");
        send_to_char(Gbuf1, ch);
      }
      if(obj->value[3] >= 1)
      {
        sprinttype(obj->value[3], (const char **) spells, Gbuf1);
        strcat(Gbuf1, "\n");
        send_to_char(Gbuf1, ch);
      }
      break;

    case ITEM_WAND:
    case ITEM_STAFF:
/*
   if(!obj->value[1])
   return;
 */
      percent =
        100 - (obj->value[1] ? (100 / obj->value[1]) : 100) * (obj->value[1] -
                                                               obj->value[2]);
      sprintf(Gbuf1,
              "%d%% of its charges remain, and it contains the spell of: ",
              percent);
      send_to_char(Gbuf1, ch);

      if(obj->value[3] >= 1)
      {
        sprinttype(obj->value[3], (const char **) spells, Gbuf1);
        strcat(Gbuf1, "\n");
        send_to_char(Gbuf1, ch);
      }
      break;

    case ITEM_WEAPON:
      sprintf(Gbuf1, "Damage Dice is '%dD%d'\n",
              obj->value[1], obj->value[2]);
      send_to_char(Gbuf1, ch);
      break;

    case ITEM_ARMOR:
      sprintf(Gbuf1, "AC-apply is %d\n", obj->value[0]);
      send_to_char(Gbuf1, ch);
      break;

    }

    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      if((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0))
      {
        if(found)
          send_to_char(" and ", ch);
        else
        {
          send_to_char("This item will also affect your", ch);
          found = TRUE;
        }
        sprinttype(obj->affected[i].location, apply_types, Gbuf2);

        if((obj->affected[i].location >= APPLY_SAVING_PARA) &&
            (obj->affected[i].location <= APPLY_SAVING_SPELL))
        {
          if(obj->affected[i].modifier < 0)
            strcpy(Gbuf3, "positively");
          else if(obj->affected[i].modifier > 0)
            strcpy(Gbuf3, "negatively");
          else
            strcpy(Gbuf3, "not at all");
        }
        else if(obj->affected[i].location != APPLY_FIRE_PROT)
        {
          if(obj->affected[i].modifier > 0)
            strcpy(Gbuf3, "positively");
          else if(obj->affected[i].modifier < 0)
            strcpy(Gbuf3, "negatively");
          else
            strcpy(Gbuf3, "not at all");
        }
        else
          Gbuf3[0] = 0;
        sprintf(Gbuf1, " %s %s", Gbuf2, Gbuf3);
        send_to_char(Gbuf1, ch);
      }
    }
    if(found)
      send_to_char(".\n", ch);

    sprintf(Gbuf1, "Estimated Value: %s\n", coin_stringv(obj->cost));
    send_to_char(Gbuf1, ch);
    return;
  }
  /* no object, must be mob/player */
  act("The tales have heard nothing on $n.", FALSE, ch, 0, 0, TO_CHAR);
}

/*
 * ***************************************************************************
 * *                     NPC spells..
 * * *
 * *************************************************************************
 */

// Basic functions that I didn't want to repeat
// eight times. Oct08 -Lucrot
int calc_dragon_breath_save(P_char ch, P_char victim)
{
  int dragonlevel, victlevel;
  int save = 0;
  dragonlevel = GET_LEVEL(ch);
  victlevel = GET_LEVEL(victim);

  save = dragonlevel + get_property("dragon.Breath.SavingThrowMod", 0) - victlevel;

  return save;
}

bool are_we_still_alive(P_char ch, P_char victim)
{
  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
      return false;
  else
    return true;
}

void spell_shadow_breath_1(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  int save, dam;
  struct damage_messages messages = {
    "$N &+wis enclosed in &+Lblack shadows!&n",
    "$n &+Lconsumes you in blackness.&n",
    "$N &+Llooks&n&+W pale&n&+L, as $n&n&+L breathes blackness.&n",
    "$N &+ris dead! &+LConsumed by the blackness of your breath!&n",
    "&+LYou are &+wtotally consumed &+Lby utter blackness as $n breathes on you!&n",
    "$n &+wtotally consumes&n $N &+Lwith blackness.&n", 0
  };
  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 5) + level);

  if(!are_we_still_alive(ch , victim))
    return;

  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));

  dam = BOUNDED(1, dam, 80);

  if(spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages)!=
    DAM_NONEDEAD);
      return;
}

void spell_shadow_breath_2(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "$N &+wis enclosed in &+Lblack shadows!&n",
    "$n &+Lconsumes you in blackness.&n",
    "$N &+Llooks&n&+W pale&n&+L, as $n&n&+L breathes blackness.&n",
    "$N &+ris dead! &+LConsumed by the blackness of your breath!&n",
    "&+LYou are &+wtotally consumed &+Lby utter blackness as $n breathes on you!&n",
    "$n &+wtotally consumes&n $N &+Lwith blackness.&n", 0
  };
  int save, dam;

  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 7) + level);

  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));

  dam = BOUNDED(1, dam, 80);

  if(!NewSaves(victim, SAVING_SPELL, save))
  {
    spell_enervation(level, ch, NULL, 0, victim, obj);
    return;
  }
  if(spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) !=
    DAM_NONEDEAD)
      return;
}

void spell_fire_breath(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "$N is hit.",
    "$n burns you.",
    "$N is partly turned to ashes, as $n breathes &+rfire&N.",
    "$N is dead, flash-fried by your &+rfirebreath&N.",
    "You are burned to ashes as $n breathes on you.",
    "$n's breath turns $N to ashes.", 0
  };
  int save, dam;
  P_obj    burn = NULL;

  // Rofl.. shouldn't this just be a macro?
  if( !are_we_still_alive(ch, victim) )
  {
    return;
  }

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 8) + level);

  if( IS_PC_PET(ch) )
  {
    dam /= 2;
  }

  if( NewSaves(victim, SAVING_BREATH, save) )
  {
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));
  }
  dam = BOUNDED(1, dam, 80);

  if( spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) != DAM_NONEDEAD )
  {
    return;
  }

  return; // Disabling item damage/drop code

  if( (IS_AFFECTED(victim, AFF_PROT_FIRE) && number(0, 4)) || IS_NPC(victim) )
  {
    return;
  }

  // This prevents items from being dropped into water and disappearing.
  if( IS_WATER_ROOM(ch->in_room) )
  {
    act("&+rThe fire is &+Rhot&+r, but the water in the room keeps your inventory cool.&n", FALSE, victim, NULL, 0, TO_CHAR);
    return;
  }

  // And now for the damage on inventory (Why just one item?)
  // High level breathers get a greater chance, victim can save, and !arena damage.
  if( number(0, TOTALLVLS) < GET_LEVEL(ch) && !NewSaves(victim, SAVING_BREATH, save)
    && !CHAR_IN_ARENA(victim) )
  {
    // While you could obfuscate things by putting all the checks in the for loop,
    //   this is much easier to manage and understand.
    for( burn = victim->carrying; burn; burn = burn->next_content )
    {
      type = burn->type;
      // Skip artifacts and transient items.
      if( IS_ARTIFACT(burn) || IS_SET(burn->extra_flags, ITEM_TRANSIENT) )
      {
        continue;
      }
      // look for a destroyable type.
      // Dereference once (Stealing space, and decreasing run time).
      if( type == ITEM_SCROLL || type == ITEM_WAND
        || type == ITEM_STAFF || type == ITEM_NOTE )
      {
        break;
      }
      // 1/3 chance to burn up an item regardless?
      if( !number(0, 2) )
      {
        break;
      }
    }
    if( burn )
    {
      act("&+r$p&+r heats up and you drop it!&n", FALSE, victim, burn, 0, TO_CHAR);
      act("&+r$p&+r catches fire and $n&+r drops it!&n", FALSE, victim, burn, 0, TO_ROOM);
      obj_from_char(burn);
      obj_to_room(burn, ch->in_room);
    }
  }
}

void spell_frost_breath(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int save, dam, mod;
  P_obj    frozen = NULL;
  struct affected_type af;
  struct damage_messages messages = {
    "$N is partially turned to ice.",
    "$n freezes you.",
    "$N is partly turned to ice, as $n breathes frost.",
    "$N is killed, encased in the ice that you breathed on $M.",
    "You are frozen to ice, as $n breathes on you.",
    "$n turns $N to ice.", 0
  };

  if( !are_we_still_alive(ch , victim) )
  {
    return;
  }

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 8) + level);
  if( IS_PC_PET(ch) )
  {
    dam /= 2;
  }

  if( NewSaves(victim, SAVING_BREATH, save) )
  {
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));
  }
  dam = BOUNDED(1, dam, 80);

  if( spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) != DAM_NONEDEAD )
  {
    return;
  }

  return; // Disabling damage/drop code.

  if( (IS_AFFECTED2(victim, AFF2_PROT_COLD) && number(0, 4)) || IS_NPC(victim) )
  {
    return;
  }

  // This prevents items from being dropped into water and disappearing.
  if( IS_WATER_ROOM(ch->in_room) )
  {
    act("&+cThe water is really &+Bcold&+c, but your fingers aren't quite numb.&n", FALSE, victim, NULL, 0, TO_CHAR);
    return;
  }

  // And now for the damage on inventory (Why just one item?)
  // High level breathers get a greater chance, victim can save, and !arena damage.
  if( number(0, TOTALLVLS) < GET_LEVEL(ch) && !NewSaves(victim, SAVING_BREATH, save)
    && !CHAR_IN_ARENA(victim) )
  {
    // While you could obfuscate things by putting all the checks in the for loop,
    //   this is much easier to manage and understand.
    for( frozen = victim->carrying; frozen; frozen = frozen->next_content )
    {
      // Skip artifacts and transient items.
      if( IS_ARTIFACT(frozen) || IS_SET(frozen->extra_flags, ITEM_TRANSIENT) )
      {
        continue;
      }
      // look for a destroyable type.
      // Dereference once (Stealing space, and decreasing run time).
      type = frozen->type;
      if( type == ITEM_DRINKCON || type == ITEM_FOOD || type == ITEM_POTION )
      {
        break;
      }
      // 1/3 chance to shatter an item regardless?
      if( !number(0, 2) )
      {
        break;
      }
    }
    if( frozen )
    {
      act("&+C$p&+c gets really cold.  You drop it!&n", FALSE, victim, frozen, 0, TO_CHAR);
      act("&+C$p&+c frosts over and $n&+c drops it!&n", FALSE, victim, frozen, 0, TO_ROOM);
      obj_from_char(frozen);
      obj_to_room(frozen, ch->in_room);
    }
  }
}

void spell_acid_breath(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int save, dam;
  struct damage_messages messages = {
    "$N is partially corroded by your acid breath.",
    "$n corrodes you.",
    "$N is corroded by $n's acidic breath.",
    "$N is corroded to nothing by your acid breath.",
    "You are corroded to nothing as $n breathes on you.",
    "$n corrodes $N to nothing.", 0
  };
  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 12) + level);
  if(IS_PC_PET(ch))
    dam /= 2;
  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));

  dam = BOUNDED(1, dam, 80);

  if(IS_AFFECTED2(victim, AFF2_PROT_ACID) &&
    number(0, 4))
      return;

  if(spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) !=
      DAM_NONEDEAD)
    return;

  /*
   * And now for the damage on equipment
   */

#if 0
  if(number(0, TOTALLVLS) < GET_LEVEL(ch))
  {
    if(!NewSaves(victim, SAVING_BREATH, save) &&
      !CHAR_IN_ARENA(victim))
    {
      for (damaged = 0; (damaged < MAX_WEAR) &&
           !((victim->equipment[damaged]) &&
             (victim->equipment[damaged]->type == ITEM_ARMOR) &&
             (victim->equipment[damaged]->value[0] > 0) &&
             number(0, 1)); damaged++) ;
      if(damaged < MAX_WEAR)
      {
        act("&+L$p corrodes.", FALSE, victim, victim->equipment[damaged], 0,
            TO_CHAR);
        GET_AC(victim) -= apply_ac(victim, damaged);
        victim->equipment[damaged]->value[0] -= number(1, 7);
        GET_AC(victim) += apply_ac(victim, damaged);
        victim->equipment[damaged]->cost = 0;
      }
    }
  }
#endif
}

void spell_gas_breath(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int save, dam;
  struct damage_messages messages = {
    "$N is hit by your &+Ggas&N.",
    "$n gases you.",
    "$N is gassed by $n.",
    "$N is killed by the &+Ggas&N you breathe on $M.",
    "You die from the &+Ggas&N $n breathes on you.",
    "$n kills $N with a &+Ggas&N breath.", 0
  };
  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 2) + 
        ((level  + get_property("dragon.Breath.DamageMod", 1))/ 4));
  if(IS_PC_PET(ch))
    dam /= 2;
  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));
  dam = BOUNDED(1, dam, 80);

  if(spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) ==
      DAM_NONEDEAD)
  {
    if(!IS_AFFECTED2(victim, AFF2_PROT_GAS) ||
      !number(0, 4))
        spell_poison(level, ch, 0, 0, victim, 0);
  }
}

void spell_lightning_breath(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int save, dam;
  struct damage_messages messages = {
    "$N is hit by your lightning breath.",
    "$n hits you with a lightning breath.",
    "$N is hit by $n's lightning breath.",
    "$N is killed by your lightning breath.",
    "You are killed by $n's lightning breath.",
    "$n kills $N with a lightning breath.", 0
  };
  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 8) + level);
  if(IS_PC_PET(ch))
    dam /= 2;
  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));

  if(IS_AFFECTED2(victim, AFF2_PROT_LIGHTNING) &&
    number(0, 4))
      return;

  dam = BOUNDED(1, dam, 80);

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages);
}

void spell_blinding_breath(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct affected_type af;
  int save, dam;
  struct damage_messages messages = {
    "$N is hit by your &+Ggas&N.",
    "$n gases you.",
    "$N is gassed by $n.",
    "$N is killed by the &+Ggas&N you breathe on $M.",
    "You die from the &+Ggas&N $n breathes on you.",
    "$n kills $N with a &+Ggas&N breath.", 0
  };
  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (180 + dice(4, 10) + ((level + get_property("dragon.Breath.DamageMod", 1)) / 2));
  if(IS_PC_PET(ch))
    dam /= 2;
  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));;

  dam = BOUNDED(1, dam, 80);

  if(spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) !=
      DAM_NONEDEAD)
    return;

  if((level < 0 || !NewSaves(victim, SAVING_BREATH, save)) &&
      !resists_spell(ch, victim) && !IS_TRUSTED(victim) &&
      !affected_by_spell(victim, SPELL_BLINDNESS))
  {
    act("&+gThe toxic gas seems to have blinded $n&n!", TRUE, victim, 0, 0,
        TO_ROOM);
    send_to_char("&+gYour eyes sting as the toxic gas blinds you!\n", victim);
    blind(ch, victim, 60 * WAIT_SEC);
  }
}

// Under construction?
void spell_basalt_light_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_basalt_light(level, ch, arg, type, victim, obj);
  if (!number(0, 4))
  {
    
  }
}

void spell_basalt_light(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int save, dam;
  struct damage_messages messages = {
    "$N is partially corroded by your &+Lbasalt &+wlight&n.",
    "$n emits a &+Lbasalt &+wlight&n that corrodes you.",
    "$N is corroded by $n's &+Lbasalt &+wlight&n.",
    "$N is corroded to nothing by your &+Lbasalt &+wlight&n.",
    "You are corroded to nothing as $n emits a &+Lbasalt &+wlight&n.",
    "$n corrodes $N to nothing.", 0
  };
  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 11) + level);
  if(IS_PC_PET(ch))
    dam /= 2;
  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));

  dam = BOUNDED(1, dam, 80);

  if(IS_AFFECTED2(victim, AFF2_PROT_ACID) &&
    number(0, 4))
      return;

  if(spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) !=
      DAM_NONEDEAD)
    return;
}

// Under construction?
void spell_jasper_light_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_jasper_light(level, ch, arg, type, victim, obj);
  if (!number(0, 3))
  {
    
  }
}

void spell_jasper_light(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int save, dam;
  struct damage_messages messages = {
    "$N is hit by your &+Gjasper &+glight&n.",
    "$n emits a &+Gjasper&+g light&n.",
    "$N chokes as $n emits a &+Gjasper &+glight&n.",
    "$N chokes to death.",
    "You die from the &+Gjasper &+glight&n.",
    "$n kills $N with $s &+Gjasper &+glight&n.", 0
  };
  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 7) + level);
  if(IS_PC_PET(ch))
    dam /= 2;
  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));

  dam = BOUNDED(1, dam, 80);

  if(spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) ==
      DAM_NONEDEAD)
  {
    if(!IS_AFFECTED2(victim, AFF2_PROT_GAS) ||
      !number(0, 4))
        spell_poison(level, ch, 0, 0, victim, 0);
  }
}

// Under construction?
void spell_azure_light_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_azure_light(level, ch, arg, type, victim, obj);
  if (!number(0, 3))
  {
    
  }
}

void spell_azure_light(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int save, dam;
  struct damage_messages messages = {
    "$N is hit.",
    "$n emits an &+bazure &+Blight&n which shocks you!",
    "$N looks shocked as $n emits a &+bazure &+Blight&n.",
    "$N is killed by your &+bazure &+Blight&n.",
    "You are killed by $n's &+bazure &+Blight&n.",
    "$n kills $N with his &+bazure &+Blight&n.", 0
  };
  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 8) + level);
  if(IS_PC_PET(ch))
    dam /= 2;
  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));

  dam = BOUNDED(1, dam, 80);

  if(IS_AFFECTED2(victim, AFF2_PROT_LIGHTNING) &&
    number(0, 4))
      return;

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages);
}

// Under construction?
void spell_crimson_light_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_crimson_light(level, ch, arg, type, victim, obj);
  if (!number(0, 3))
  {
    
  }
}

void spell_crimson_light(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "$N is hit.",
    "$n burns you.",
    "$N bakes in the &+Rcrimson &+rlight&n, as $n &+Wradiates&N.",
    "$N is dead, flash-fried by your &+Rcrimson light&N.",
    "You are burned to ashes as $n .",
    "$n's &+Rcrimson light&n turns $N to ashes.", 0
  };
  int save, dam;
  P_obj    burn = NULL;

  if(!are_we_still_alive(ch , victim))
    return;

  save = calc_dragon_breath_save(ch, victim);
  dam = (int) (dice(level + get_property("dragon.Breath.DamageMod", 1), 9) + level);

  if(IS_PC_PET(ch))
    dam /= 2;

  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) (dam / get_property("dragon.Breath.savedDamage", 2));

  dam = BOUNDED(1, dam, 80);

  if(spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) !=
    DAM_NONEDEAD)
      return;

  return; // Disabling item damage/drop code

  if(IS_AFFECTED(victim, AFF_PROT_FIRE) && number(0, 4))
      return;

  // This prevents items from being dropped into water and disappearing.
  if( IS_WATER_ROOM(ch->in_room) )
  {
    act("&+rThe fire is &+Rhot&+r, but the water in the room keeps your inventory cool.&n", FALSE, victim, burn, 0, TO_CHAR);
    return;
  }

  // And now for the damage on inventory (Why just one item?)
  // High level breathers get a greater chance, victim can save, and !arena damage.
  if( number(0, TOTALLVLS) < GET_LEVEL(ch) && !NewSaves(victim, SAVING_BREATH, save)
    && !CHAR_IN_ARENA(victim) )
  {
    // While you could obfuscate things by putting all the checks in the for loop,
    //   this is much easier to manage and understand.
    for( burn = victim->carrying; burn; burn = burn->next_content )
    {
      // Skip artifacts and transient items.
      if( IS_ARTIFACT(burn) || IS_SET(burn->extra_flags, ITEM_TRANSIENT) )
      {
        continue;
      }
      // look for a destroyable type.
      // Dereference once (Stealing space, and decreasing run time).
      type = burn->type;
      if( type == ITEM_SCROLL || type == ITEM_WAND
        || type == ITEM_STAFF || type == ITEM_NOTE )
      {
        break;
      }
      // 1/3 chance to burn up an item regardless?
      if( !number(0, 2) )
      {
        break;
      }
    }
    if( burn )
    {
      act("&+r$p&+r heats up and you drop it!&n", FALSE, victim, burn, 0, TO_CHAR);
      act("&+r$p&+r catches fire and $n&+r drops it!&n", FALSE, victim, burn, 0, TO_ROOM);
      obj_from_char(burn);
      obj_to_room(burn, ch->in_room);
    }
  }
}

void cont_light_dissipate_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      room, readd_dark, readd_twilight, had_light = TRUE;

  if(!data)
  {
    logit(LOG_EXIT, "Call to cont light dissipation with invalid data");
    raise(SIGSEGV);
  }

  /*
   * Ok, let's rock
   */

  sscanf((char *) data, "%d%d%d", &room, &readd_dark, &readd_twilight);

  if(room < 0 || room > top_of_world)
  {
    logit(LOG_DEBUG, "Cont light dissipation in invalid room. [%d]", room);
    return;
  }
  if(!IS_SET(world[room].room_flags, MAGIC_LIGHT))
    had_light = FALSE;

  REMOVE_BIT(world[room].room_flags, MAGIC_LIGHT);

  if(readd_dark)
    SET_BIT(world[room].room_flags, DARK);

  if(readd_twilight)
    SET_BIT(world[room].room_flags, TWILIGHT);

  if(had_light)
    send_to_room("The light in the area seems to dissipate a bit.\n", room);
  room_light(room, REAL);

  return;
}

void spell_continual_light(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  char     buff[64];
  int      readd_dark = FALSE, readd_twilight = FALSE;

  if(obj)
  {
    if(IS_OBJ_STAT(obj, ITEM_LIT))
    {
      act("$p &+Walready emits copious light.", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    if(IS_OBJ_STAT2(obj, ITEM2_MAGIC) || IS_OBJ_STAT(obj, ITEM_GLOW))
    {
      act("&+W$p glows brightly for a moment, but the glow fades away.",
          FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    act("&+W$p glows brilliantly!", FALSE, ch, obj, 0, TO_CHAR);
    SET_BIT(obj->extra_flags, ITEM_LIT);
  }
  else
  {
  /*
    send_to_char("&+WThe room lights up&n and then dims slightly.\n", ch);
    act("&+WThe room lights up.&n", 0, ch, 0, 0, TO_ROOM);
    SET_BIT(world[ch->in_room].room_flags, TWILIGHT);
  */

    if(IS_SET(world[ch->in_room].room_flags, MAGIC_LIGHT))
    {
      send_to_char("The room already appears to be lit, so nothing happens.\n", ch);
      return;
    }

    if(IS_SET(world[ch->in_room].room_flags, MAGIC_DARK))
    {
       REMOVE_BIT(world[ch->in_room].room_flags, MAGIC_DARK);
       send_to_char("The room becomes much less stygian.\n", ch);
       act("The room becomes much less stygian.", 0, ch, 0, 0, TO_ROOM);
    }
    else
    {
      if(IS_SET(world[ch->in_room].room_flags, DARK))
      {
        REMOVE_BIT(world[ch->in_room].room_flags, DARK);
        readd_dark = TRUE;
      }
      if(IS_SET(world[ch->in_room].room_flags, TWILIGHT))
      {
        REMOVE_BIT(world[ch->in_room].room_flags, TWILIGHT);
        readd_twilight = TRUE;
      }
      sprintf(buff, "%d %d %d", ch->in_room, readd_dark, readd_twilight);

      SET_BIT(world[ch->in_room].room_flags, MAGIC_LIGHT);

      send_to_char("&+WThe room lights up!\n", ch);
      act("&+WThe room lights up!", 0, ch, 0, 0, TO_ROOM);

      add_event(cont_light_dissipate_event, level * 50, NULL, NULL, NULL, 0, buff, strlen(buff)+1);
      //AddEvent(EVENT_SPECIAL, level * 50, TRUE, cont_light_dissipate_event, buff);
    }
  }

  char_light(ch);
  room_light(ch->in_room, REAL);
}

struct airy_water_data
{
  int      room;
  int      old_sect;
  int      readd_uw;
};

void event_airy_water_dissipate(P_char ch, P_char victim, P_obj obj,
                                void *data)
{
  struct airy_water_data *d = (struct airy_water_data *) data;
  P_char   tch, next;

  if((d->room < 0) || (d->room > top_of_world))
  {
    logit(LOG_EXIT, "Airy water dissipation in invalid room.");
    raise(SIGSEGV);
  }
  if(d->readd_uw)
    SET_BIT(world[d->room].room_flags, UNDERWATER);

  world[d->room].sector_type = d->old_sect;

  send_to_room
    ("The bubble slowly shrinks, allowing water to fill the area once more.\n",
     d->room);

  /* update everyone's underwater status */

  for (tch = world[d->room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    underwatersector(tch);
  }
}

void spell_airy_water(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct airy_water_data data;
  P_char   next, tch;

  if(!IS_UNDERWATER(ch))
  {
    send_to_char("This spell only works underwater.\n", ch);
    return;
  }
  send_to_char
    ("A large air bubble slowly expands around you, displacing all water in the area.\n",
     ch);
  act
    ("A large air bubble slowly expands around $n&n, displacing all water in the area.",
     FALSE, ch, 0, 0, TO_ROOM);

  data.readd_uw = IS_SET(world[ch->in_room].room_flags, UNDERWATER);
  data.old_sect = world[ch->in_room].sector_type;
  data.room = ch->in_room;

  REMOVE_BIT(world[ch->in_room].room_flags, UNDERWATER);
  world[ch->in_room].sector_type = SECT_WATER_SWIM;

  /* update everyone's underwater status */

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;
    underwatersector(tch);
  }

  add_event(event_airy_water_dissipate, level * 10, 0, 0, 0, 0, &data,
            sizeof(data));
}

void spell_levitate(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_LEVITATE))
  {
    if(!IS_AFFECTED(victim, AFF_LEVITATE))
    {
      act("&+WYou float up in the air!", FALSE, victim, 0, 0, TO_CHAR);
      act("$n &+Wfloats up in the air!", TRUE, victim, 0, 0, TO_ROOM);
    }
    memset(&af, 0, sizeof(af));
    af.type = SPELL_LEVITATE;
    af.duration = MAX(level, 10);
    af.bitvector = AFF_LEVITATE;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_LEVITATE)
      {
        af1->duration =  MAX(level, 10);
      }
  }

}

void spell_fly(int level, P_char ch, char *arg, int type, P_char victim,
               P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_FLY))
  {
    if(!IS_AFFECTED(victim, AFF_FLY))
    {
      act("&+WYou fly through the air, free as a bird!",
          FALSE, victim, 0, 0, TO_CHAR);
      act("$n &+Wflies through the air, free as a bird!",
          TRUE, victim, 0, 0, TO_ROOM);
    }
    bzero(&af, sizeof(af));
    af.type = SPELL_FLY;
    af.duration = level * 2;
    af.bitvector = AFF_FLY;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_FLY)
      {
        af1->duration =  level * 2;
      }
  }
}

/*
 * A routine for ITEM_TELEPORT objects
 */
int get_room_in_zone(int zone_room, P_char ch)
{
  int      to_room, low, high, level, start_room;

  level = 50;                   /*
                                 * give 50 chances
                                 */
  start_room = real_room(zone_room);
  low = MAX(2, zone_table[world[start_room].zone].real_bottom);
  high = zone_table[world[start_room].zone].real_top;

  do
  {
    to_room = number(low, high);
  }
  while ((!can_enter_room(ch, to_room, FALSE) ||
          IS_SET(world[to_room].room_flags, NO_TELEPORT) || IS_HOMETOWN(to_room)) && --level);

  if(level)
    start_room = to_room;

  return (start_room);
}

bool check_item_teleport(P_char ch, char *arg, int cmd)
{
  P_obj      obj = NULL, obj_next;
  P_char     dummy;
  int        timeleft;
  int        bits;
  int        room, to_room;
  int        pos;
  int        vnum;
  int        virt;
  int        guild_id;
  char       Gbuf1[100];

  room = ch->in_room;
  /* Some types are keywordless */
  if((cmd >= CMD_NORTH && cmd <= CMD_DOWN) ||
      (cmd >= CMD_NORTHWEST && cmd <= CMD_SE))
  {
    for (obj = world[ch->in_room].contents; obj; obj = obj_next)
    {
      obj_next = obj->next_content;
      if(obj->type == ITEM_TELEPORT && obj->value[1] == cmd)
        break;
    }
  }
  else
  {
    bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &dummy, &obj);
  }
  if(!obj)
    return FALSE;

  if((obj->type != ITEM_TELEPORT) || (obj->value[1] != cmd))
    return FALSE;

  vnum = obj_index[obj->R_num].virtual_number;

  // allow house/guild entrances to always work regardless of no-magic status
 /*
  if(IS_SET(world[ch->in_room].room_flags, NO_MAGIC) && (vnum != 11001) &&
      (vnum != 11007) && (vnum != 11008))
  {
    send_to_char("That doesn't seem to do anything!\n", ch);
    return FALSE;
  }
  */
    if(400220 == obj_index[obj->R_num].virtual_number &&
        !isname(GET_NAME(ch), obj->name))
    {
      send_to_char("You may not enter someone elses &+Gspirit&n portal!\r\n", ch);
      return TRUE;
    }

  /*
   * Ignore argument for now...eventually we could "throw dust Dbra"..
   */
  if(IS_NPC(ch))
  {
    virt = mob_index[GET_RNUM(ch)].virtual_number;
    if(virt == EVIL_AVATAR_MOB || virt == GOOD_AVATAR_MOB)
    {
      send_to_char("You are far too godly to use portals!\n", ch);
      return FALSE;
    }
  }


  to_room = obj->value[0];
  if(to_room == -1)
    to_room = get_room_in_zone(obj->value[3], ch);
  else
    to_room = real_room(to_room);

  // old guildhalls (deprecated)
//  if(IS_SET(world[ch->in_room].room_flags, ROOM_ATRIUM))
//  {
//    if(!House_can_enter(ch, world[ch->in_room].number, -1))
//    {
//      send_to_char("You may not enter this private house!\n", ch);
//      return TRUE;
//    }
//  }

  /* alternate (non-automated construction) guild-only teleporter checking */

//  if(obj->value[7] && (obj->value[7] != GET_A_NUM(ch)) && !IS_TRUSTED(ch))
//  {
//    send_to_char("Nothing happens.\n", ch);
//    return TRUE;
//  }

// New Guildhalls
  // If this is a portal to/from a guildhall/outpost.
  if( obj_index[obj->R_num].virtual_number == BUILDING_PORTAL )
  {
    // Get the proper guild id number.
    // If in a guildhall, call find_gh...
    if( IN_GH_ZONE( ch->in_room ) )
    {
      Guildhall *gh = find_gh_from_vnum( world[ch->in_room].number );
      if( gh )
        guild_id = gh->assoc_id;
      else
      {
        send_to_char( "Buggy guild portal.  Tell a God.\n", ch );
        return TRUE;
      }
    }
    else
    {
      Building *op = get_building_from_room( ch->in_room );
      if( op )
        guild_id = get_outpost_owner( op );
      else
      {
        send_to_char( "Buggy outpost portal.  Tell a God.\n", ch );
        return TRUE;
      }
    }
    // Now find a group member or ch that is in the assoc. or fail.
    struct group_list *tgroup = ch->group;
    struct alliance_data *alliance;
    // If ch is not in the proper guild,
    if( GET_A_NUM(ch) != guild_id )
    {
      alliance = get_alliance(GET_A_NUM(ch));
      if( !(alliance && (alliance->forging_assoc_id == guild_id
        || alliance->joining_assoc_id == guild_id)) )
      {
        // Check all group members
        while( tgroup && (GET_A_NUM(tgroup->ch) != guild_id
          || IS_APPLICANT(GET_A_BITS(tgroup->ch))) )
        {
          alliance = get_alliance(GET_A_NUM(tgroup->ch));
          if( alliance && (alliance->forging_assoc_id == guild_id
            || alliance->joining_assoc_id == guild_id) )
          {
            break;
          }
          tgroup = tgroup->next;
        }
        // If no guildie-group member found..
        if( !tgroup && !IS_TRUSTED(ch) )
        {
          send_to_char( "The vortex repels your cheesy butt.\n", ch );
          return TRUE;
        }
      }
    }
  }

  // old guildhalls (deprecated)
//  if(obj_index[obj->R_num].virtual_number == 11001)
//  {
//    /* guild teleporter */
//    house = house_ch_is_in(ch);
//    if(house)
//    {
//      struct group_list *tgroup;
//
//      for (tgroup = ch->group; tgroup; tgroup = tgroup->next)
//        if(GET_A_NUM(tgroup->ch) == house->owner_guild &&
//            !IS_APPLICANT(GET_A_BITS(tgroup->ch)))
//          break;
//      if((GET_A_NUM(ch) != house->owner_guild) && !tgroup &&
//          !(IS_TRUSTED(ch)))
//      {                         /* only guildies can use porters */
//        send_to_char("Nothing happens.\n", ch);
//        return TRUE;
//      }
//    }
//  }

  if(!obj->value[2] ||
      (IS_SET(world[ch->in_room].room_flags, ARENA) !=
       IS_SET(world[to_room].room_flags, ARENA)))
  {
    send_to_char("Nothing happens.\n\n", ch);
    return TRUE;
  }
  if(OBJ_CARRIED_BY(obj, ch))
  {
    act("&+W$p in $n's hands suddenly glows brightly!", FALSE, ch, obj, 0,
        TO_ROOM);
    act("&+W$p in your hands suddenly glows brightly!", FALSE, ch, obj, 0,
        TO_CHAR);
  }
  else if((obj_index[obj->R_num].virtual_number == 11007) ||   /* GH/house entry/exit points */
           (obj_index[obj->R_num].virtual_number == 11008))
  {
    if(IS_NPC(ch))
      return FALSE;
    if(obj_index[obj->R_num].virtual_number == 11007)
    {                           /* outside door */
      act("$n enters $p via the doorway.", FALSE, ch, obj, 0, TO_ROOM);
      act("You enter $p through the doorway.", FALSE, ch, obj, 0, TO_CHAR);
    }
    else
    {
      act("$n leaves the building through the doorway.", FALSE, ch, obj, 0,
          TO_ROOM);
      act("You leave the building through the doorway.", FALSE, ch, obj, 0,
          TO_CHAR);
    }
  }
  else if(obj_index[obj->R_num].virtual_number != 48000)
    act("&+W$p suddenly glows brightly!", FALSE, ch, obj, 0, TO_ROOM);

  /* different messages for guild doors */

  if ((obj_index[obj->R_num].virtual_number == 48000) &&
      (IS_FIGHTING(ch) || IS_DESTROYING(ch)) )
  {
    act("&+WYou cannot enter a guildhall in combat!", FALSE, ch, obj, 0, TO_CHAR);
    return TRUE;
  }

  if(obj_index[obj->R_num].virtual_number == 11007)
    teleport_to(ch, to_room, 1);
  else if(obj_index[obj->R_num].virtual_number == 11008)
    teleport_to(ch, to_room, 2);
  else
  {
    teleport_to(ch, to_room, 0);
    if(obj_index[obj->R_num].virtual_number == 11001)
    {
      struct follow_type *tfol, *next;
      P_char   tch;

      for (tfol = ch->followers; tfol; tfol = next)
      {
        next = tfol->next;
        tch = tfol->follower;

        if(room == tch->in_room)
        {
          act("You follow $N.", FALSE, tch, 0, ch, TO_CHAR);
          send_to_char("\n", tch);
          sprintf(Gbuf1, "%s %s", command[cmd - 1], arg);
          command_interpreter(tch, Gbuf1);
        }
      }
    }
  }

  if(obj->value[2] > 0)
  {
    if(!--obj->value[2])
    {
      if(OBJ_CARRIED_BY(obj, ch))
      {
        act("&+W$p in $n's hands shatters and the pieces disappear in smoke.",
            TRUE, ch, obj, 0, TO_ROOM);
        act("&+W$p in your hands shatters and the pieces disappear in smoke.",
            TRUE, ch, obj, 0, TO_CHAR);
      }
      else
        act("&+W$p shatters and the pieces disappear in smoke.", TRUE, ch, obj, 0, TO_ROOM);

      if(OBJ_WORN(obj))
      {
        for (pos = 0; pos < MAX_WEAR; pos++)
        {
          if(obj->loc.wearing->equipment[pos] == obj)
          {
            unequip_char(obj->loc.wearing, pos);
            break;
          }
        }
      }
      extract_obj(obj, TRUE); // Could make Dragonnia stone arti.
      obj = NULL;
    }
  }
  return TRUE;
}

int KludgeDuration(P_char ch, int baselevel, int baseduration)
{
#if 1
/* return baseduration;
   this isn't really what was originally intended, but it's based on caster's
   level */
  return MAX(1, (GET_LEVEL(ch) / baselevel) * baseduration);

#else
  if(GET_LEVEL(ch) < baselevel)
    return baseduration;

  return
    MAX(baseduration,
        (baseduration * number(75, (75 + GET_LEVEL(ch) - baselevel)) / 100));
#endif
}

void spell_barkskin(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af1;
  double mod = 1;
  bool shown;

  if( !ch )
  {
    logit(LOG_EXIT, "spell_barkskin called in magic.c with no ch");
    raise(SIGSEGV);
  }
  if(!IS_ALIVE(ch))
  {
    act("Lay still, you seem to be dead!", TRUE, ch, 0, 0, TO_CHAR);
    return;
  }
  if( !IS_ALIVE(victim) )
  {
    act("$N is not a valid target.", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  if(racewar(ch, victim))
  {
    if(NewSaves(victim, SAVING_PARA, 0))
    {
      act("$N evades your spell!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
  }
  if(GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND))
  {
    mod = 1.25;
  }
  else
  {
    mod = 1;
  }
  if( !IS_AFFECTED(victim, AFF_ARMOR) )
  {
    bzero(&af1, sizeof(af1));
    af1.type = SPELL_BARKSKIN;
    af1.duration =  25;
    af1.modifier =  (int) (-1 * mod * level);
    af1.location = APPLY_AC;
    af1.bitvector = AFF_BARKSKIN | AFF_ARMOR;

    affect_to_char(victim, &af1);
    act("$n's skin gains the texture and toughness of &+ybark.&n", FALSE,
      victim, 0, 0, TO_ROOM);
    act("Your skin gains the texture and toughness of &+ybark.&n", FALSE,
      victim, 0, 0, TO_CHAR);
  }
  else if( !IS_AFFECTED(victim, AFF_BARKSKIN) && !IS_AFFECTED5(victim, AFF5_THORNSKIN) )
  {
    bzero(&af1, sizeof(af1));
    af1.type = SPELL_BARKSKIN;
    af1.duration =  25;
    af1.bitvector = AFF_BARKSKIN;

    affect_to_char(victim, &af1);
    act("$n's skin gains the texture of &+ybark.&n", FALSE, victim, 0, 0, TO_ROOM);
    act("Your skin gains the texture of &+ybark.&n", FALSE, victim, 0, 0, TO_CHAR);
  }
  else
  {
    struct affected_type *af1;

    shown = FALSE;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_BARKSKIN)
      {
        if( !shown )
        {
          send_to_char( "&+yYour skin rehardens.\n", victim );
          shown = TRUE;
        }
        af1->duration = 25;
      }
    }
    if( !shown )
    {
      send_to_char( "&+WYou're already affected by an armor-type spell.\n", victim );
    }
  }
}

void spell_grow_spike(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      dam, temp, dam_flag;
  struct damage_messages messages = {
    0, 0, 0, 0, 0, 0
  };

  temp = MIN(20, (level / 2 + 1));
  dam = dice(6 * temp, 6);

  if(level > 50)
  {
    dam = dice(6 * temp, 7);
  }

  dam = dam;

  if(NewSaves(victim, SAVING_SPELL, (IS_AFFECTED(victim, AFF_FLY) ? -3 : 3)))
    dam >>= 1;

  dam_flag = 0;

  /* default dam_flag is 0, so don't bother checking for those cases */

  switch (world[ch->in_room].sector_type)
  {
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_NO_GROUND:
  case SECT_UNDERWATER:
  case SECT_FIREPLANE:
  case SECT_OCEAN:
    dam_flag = 0;
    break;                      /*
                                 * what earth to move?
                                 */
  case SECT_CITY:
  case SECT_FIELD:
  case SECT_FOREST:
  case SECT_HILLS:
  case SECT_UNDERWATER_GR:
  case SECT_ROAD:
    dam_flag = 1;
    break;                      /*
                                 * normal damage
                                 */
  case SECT_MOUNTAIN:
  case SECT_UNDRWLD_WILD:
  case SECT_UNDRWLD_CITY:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_UNDRWLD_SLIME:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_UNDRWLD_MUSHROOM:
    dam_flag = 2;               /*
                                 * dangerous, and slightly higher dam from
                                 * landslides
                                 */
    break;
  case SECT_INSIDE:
  case SECT_UNDRWLD_INSIDE:
    dam_flag = 3;               /*
                                 * dangerous, and added damage from
                                 * falling debris
                                 */
    break;
  case SECT_UNDRWLD_LIQMITH:
  case SECT_LAVA:
    dam_flag = 4;
    break;
  }


  if((dam_flag == 1) && !OUTSIDE(ch))
    dam_flag = 3;

  if(ch->specials.z_cord != 0)
    dam_flag = 0;

  switch (dam_flag)
  {
  case 0:
    send_to_char("No earth to grow spikes from here, try a different spell.\n", ch);
    return;
    break;
  case 1:
    messages.attacker =
      "&+yYou cause the earth to form into a &+yspike&n and rise up!";
    messages.victim = messages.room =
      "$n causes the earth to form into a &+yspike&n and rise up!!";
    break;
  case 2:
  case 3:
    messages.attacker =
      "&+yYou cause &+Ya HUGE SPIKE&+y to burst out of the ground at your opponent!&n";
    messages.victim = messages.room =
      "&+y$n&+y causes the earth to form into a &+Yspike&+y and rise up!&n";
    break;
  case 4:
    messages.attacker =
      "&+yYou cause &+ra MOLTEN SPIKE&+y to burst out of the ground at your opponent!&n";
    messages.victim = messages.room =
      "&+y$n&+y causes the earth to form into a &+rspike&+y and rise up!&n";
    break;
  default:
    send_to_char("As you are about to utter the last syllable, you choke it off,\n"
      "realizing you could well bury yourself alive!\n", ch);
    return;
    break;
  }

  dam = (int) (dam + 0.1*dam*dam_flag);

//  play_sound(SOUND_EARTHQUAKE1, NULL, ch->in_room, TO_ROOM);
  if(spell_damage(ch, victim, dam, (dam_flag==4) ? SPLDAM_FIRE : SPLDAM_GENERIC, dam_flag > 1 ? 0 : SPLDAM_GLOBE, &messages))
    return;

  /*
     if(number(0, 3) || (GET_CHAR_SKILL(victim, SKILL_SAFE_FALL) >= number(1, 101))) {
     act("$n almost dodges the &+yearth&n's maul, staying on $s feet!", TRUE, victim, 0, 0, TO_ROOM);
     act("You almost dodge the &+yearth&n's maul, staying on your feet!", TRUE, victim, 0, 0, TO_CHAR);
     if(IS_PC(victim)) notch_skill(victim, SKILL_SAFE_FALL, 3);
     } else {
     act("You are almost swallowed by the earth and injure yourself!",
     FALSE, ch, 0, victim, TO_VICT);
     act("$n crashes to the ground!", TRUE, victim, 0, 0, TO_ROOM);
     SET_POS(victim, number(0, 2) + GET_STAT(victim));
     if(GET_POS(victim) == POS_PRONE)
     Stun(victim, PULSE_VIOLENCE * 2, TRUE);
     CharWait(victim, PULSE_VIOLENCE);
     play_sound(SOUND_EARTHQUAKE2, NULL, ch->in_room, TO_ROOM);
     }
   */
}

void spell_entangle(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;
  int      skl_lvl;
  int      chance;

  chance = 5;
  // 56 / 5 - 1 = 10 => -50 svpara to always save
  //   .. and 0 svpara to save 50% of the time.
  //   .. and 50 svpara to never save.
  skl_lvl = MAX(1, ((level / 5) - 1));

  if( !IS_OUTSIDE(ch->in_room) )
  {
    send_to_char("Not too much to entangle yer opponent with here..\n", ch);
    return;
  }

  if( !IS_ALIVE(victim) || IS_TRUSTED(victim) )
  {
    return;
  }

  if( affected_by_spell(victim, SPELL_ENTANGLE)
    || IS_AFFECTED2(victim, AFF2_MINOR_PARALYSIS) )
  {
    send_to_char("Nothing happens.\n", ch);
    return;
  }

/*
if( (!(IS_NPC(victim)) && (world[ch->in_room].sector_type == SECT_FOREST)  && (CASTING_MOD(ch) > 2) )) {
    if(!(StatSave(victim, APPLY_AGI, (-1 * (-4+CASTING_MOD(ch))) ))) {
  SET_BIT(victim->specials.affected_by, AFF_BOUND);
  act("&+GVegetation bursts out of the ground, TOTALLY entangling $N!", TRUE, ch, 0, victim, TO_NOTVICT);
  act("&+GVegetation bursts out of the ground, TOTALLY entangling you!", TRUE, ch, 0, victim, TO_VICT);
  act("&+GYou call the vegetation to burst out and completely entangle $N.", TRUE, ch, 0, victim, TO_CHAR);
  CharWait(ch, 4 * PULSE_VIOLENCE);
  return; }
}
*/

  bzero(&af, sizeof(af));

  if( !NewSaves(victim, SAVING_PARA, skl_lvl)
    && !(IS_NPC(victim) && IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA)) )
  {
    send_to_char("&+GVegetation bursts out of the ground, entangling you to the point of paralysis.&N\n", victim);
    act("&+GVegetation bursts out of the ground, entangling $n!&N", TRUE, victim, 0, 0, TO_ROOM);

    if( IS_FIGHTING(victim) )
    {
      stop_fighting(victim);
    }
    if( IS_DESTROYING(victim) )
    {
      stop_destroying(victim);
    }
    StopMercifulAttackers(victim);

    if( world[ch->in_room].sector_type == SECT_FOREST
      && number(1, 100) < chance )
    {
      act("&+GThe vegetation closes tightly, completely entangling $n!", TRUE, victim, 0, 0, TO_ROOM);
      send_to_char("&+GThe vegetation closes tightly, completely entangling you!\n", victim);
      SET_BIT(victim->specials.affected_by, AFF_BOUND);
    }
    else
    {
      af.type = SPELL_ENTANGLE;
      // 56 / 2 - 5 = 23 sec and 12 / 2 - 5 = 1 sec
      af.duration = WAIT_SEC * (level / 2 - 5);
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW;
      af.bitvector2 = AFF2_MINOR_PARALYSIS;
//      af.bitvector2 = AFF2_SLOW;
      af.location = APPLY_NONE;
      af.modifier = 0;

      send_to_char("&+gVegetation &+Gbursts&+g from the ground, impeding your progress.&N\n", victim);
      act("&+gVegetation &+Gbursts&+g from the ground, impeding $n.&N", TRUE, victim, 0, 0, TO_ROOM);
      affect_to_char(victim, &af);
    }
  }
}

void spell_vampiric_touch(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct affected_type af;
  int duration_i = (int) (level / 5);

  if(IS_AFFECTED2(victim, AFF2_VAMPIRIC_TOUCH))
  {
    act("$N is already affected by the spell!", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  {
  bzero(&af, sizeof(af));
  af.type = SPELL_VAMPIRIC_TOUCH;
  af.location = APPLY_NONE;
  af.modifier = level;
  af.duration = BOUNDED(2, duration_i , 10);
  af.bitvector2 = AFF2_VAMPIRIC_TOUCH;
  affect_to_char(ch, &af);
  af.location = APPLY_HITROLL;
  af.modifier = 3 + level / 8;
  affect_to_char(ch, &af);
  }
  act("$n's hands start to glow &+RRED&n as blood..", FALSE, ch, 0, 0,
      TO_ROOM);
  act("Your hands start to glow &+RRED&n as blood..", FALSE, ch, 0, 0,
      TO_CHAR);
}

void spell_sanctuary(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED4(ch, AFF4_SANCTUARY))
  {
    act("You are already blessed with sanctuary!", FALSE, ch, 0, ch, TO_CHAR);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_SANCTUARY;
  af.location = APPLY_NONE;
  af.duration = 15;
  af.bitvector4 = AFF4_SANCTUARY;
  affect_to_char(ch, &af);

  act("&+W$n's&+W is encased in a solid white aura!", FALSE, ch, 0, 0, TO_ROOM);
  act("&+WYou are encased in a solid white aura!", FALSE, ch, 0, 0, TO_CHAR);
}

void spell_hellfire(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED4(ch, AFF4_HELLFIRE))
  {
    act("You are already burning with hatred!", FALSE, ch, 0, ch, TO_CHAR);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_HELLFIRE;
  af.location = APPLY_NONE;
  af.duration = 15;
  af.bitvector4 = AFF4_HELLFIRE;
  affect_to_char(ch, &af);

  act("&+R$n&+R bursts into a flaming mass!!", FALSE, ch, 0, 0, TO_ROOM);
  act("&+RYou burst into a flaming mass of hate!!", FALSE, ch, 0, 0, TO_CHAR);
}

void spell_battletide(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED4(victim, AFF4_BATTLETIDE))
  {
    act("Alas, the taste of battle is already with you.", FALSE, ch, 0, victim,
        TO_CHAR);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_BATTLETIDE;
  af.location = APPLY_DAMROLL;
  af.modifier = (level / 10) + dice(1, 10);
  af.duration = 10;
  af.bitvector4 = AFF4_BATTLETIDE;
  affect_to_char(ch, &af);

  act("&+R$n&+R thrusts $s arms skyward, &+rscreaming&+R forth a call to arms!&n", FALSE, ch, 0, 0, TO_ROOM);
  act("&+RYou thrust your arms skyward, rallying your comrades for battle!&n", FALSE, ch, 0, 0, TO_CHAR);
}

void spell_mend_soul(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      healpoints;

  if(!IS_ANGEL(victim) &&
      GET_RACE(victim) != RACE_GOLEM)
  {
    act
      ("$N chants something odd and takes a look at $n, a weird look in $S eyes.",
       TRUE, ch, 0, victim, TO_NOTVICT);
    act("Um... $N isn't angelic...", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  grapple_heal(victim);

  healpoints = number(150, (GET_LEVEL(ch) * 5));

  /*
  if(!GET_CLASS(ch, CLASS_WARLOCK))
    healpoints = MIN(healpoints, GET_LEVEL(ch) * 4);
  else
    healpoints = 100;*/

  heal(victim, ch, healpoints, GET_MAX_HIT(victim));

  // healCondition(victim, healpoints);
  if(healpoints)
    send_to_char("&+WHoly &+Renergy&n flows into you from the &+Wh&+yea&+Wv&+Ye&+Wns&n, mending your wounds!\n",
                 victim);
  if(victim != ch && healpoints)
  {
    act("&+Wholy &+Renergy&n flows into $N from the &+Wh&+Yea&+Wv&+Ye&+Wns&n, mending $S wounds.", FALSE, ch, 0, victim,
        TO_NOTVICT);
    act("&+Wholy &+Renergy&n flows into $N from the &+Wh&+Yea&+Wv&+Ye&+Wns&n, mending $S wounds.", FALSE, ch, 0, victim,
        TO_CHAR);
  }
  update_pos(victim);
}

void spell_heal_undead(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      healpoints;

  // GET_RACE2 -> shapeshifted into skeleton (Blighters).
  if( !IS_UNDEADRACE(victim) && !(GET_RACE2(victim) == RACE_SKELETON)
    && (GET_RACE(victim) != RACE_GOLEM) && !GET_CLASS(victim, CLASS_NECROMANCER) )
  {
    act("$N chants something odd and takes a look at $n, a weird look in $S eyes.",
       TRUE, ch, 0, victim, TO_NOTVICT);
    act("Um... $N isn't undead...", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  grapple_heal(victim);

  healpoints = number(150, (GET_LEVEL(ch) * 5));

  /*
  if(!GET_CLASS(ch, CLASS_WARLOCK))
    healpoints = MIN(healpoints, GET_LEVEL(ch) * 4);
  else
    healpoints = 100;*/

  heal(victim, ch, healpoints, GET_MAX_HIT(victim));

  // healCondition(victim, healpoints);
  if(healpoints)
  {
    send_to_char("&+WYou feel the powers of darkness strengthen you!\n", victim);
  }
  if(victim != ch && healpoints)
  {
    act("$n reaches out at $N, touching $M. ", FALSE, ch, 0, victim,
        TO_NOTVICT);
    act("You reach out at $N and touch $M.", TRUE, ch, 0, victim, TO_CHAR);
    act("$n appears to gain power from the sudden deadly chill around $m.",
        FALSE, victim, 0, 0, TO_ROOM);
  }
  update_pos(victim);
}

void spell_greater_heal_undead(int level, P_char ch, char *arg, int type,
                               P_char victim, P_obj obj)
{
  int      healpoints = 300;

  // GET_RACE2 -> shapeshifted into skeleton (Blighters).
  if(!IS_UNDEADRACE(victim) && IS_PC(ch) && IS_NPC(victim)
    && !GET_RACE2(victim) == RACE_SKELETON )
     // old guildhalls (deprecated)
//     && mob_index[GET_RNUM(victim)].virtual_number != WARRIOR_GOLEM_VNUM &&
//      mob_index[GET_RNUM(victim)].virtual_number != MAGE_GOLEM_VNUM &&
//      mob_index[GET_RNUM(victim)].virtual_number != CLERIC_GOLEM_VNUM
  {
    act
      ("$N chants something odd and takes a look at $n, a weird look in $S eyes.",
       TRUE, ch, 0, victim, TO_NOTVICT);
    act("Um... $N isn't undead...", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  grapple_heal(victim);

  heal(victim, ch, healpoints, GET_MAX_HIT(victim));

  // healCondition(victim, healpoints);
  if(healpoints)
    send_to_char("&+WYou feel the powers of darkness flow into you!!\n",
                 victim);
  if(victim != ch && healpoints)
  {
    act("$n reaches out at $N, touching $M. ", FALSE, ch, 0, victim,
        TO_NOTVICT);
    act("You reach out at $N and touch $M.", TRUE, ch, 0, victim, TO_CHAR);
    act("$n appears to gain power from the sudden deadly chill around $m.",
        FALSE, victim, 0, 0, TO_ROOM);
  }
  update_pos(victim);
}

void spell_prot_undead(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if( !IS_UNDEADRACE(victim) && !IS_ANGEL(victim)
    && GET_RACE(victim) != RACE_GOLEM && !GET_CLASS(victim, CLASS_NECROMANCER) )
  {
    send_to_char("The target is not undead!\r\n", ch);
    return;
  }

  if( level > 45 && !IS_AFFECTED2(victim, AFF2_GLOBE) )
  {
    spell_globe(level, ch, 0, 0, victim, obj);
  }

  /*
  if(has_skin_spell(victim) && IS_PC(victim))
  {
    act("$N is already protected by a &+cmagical barrier.&n",
      TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  */

  bzero(&af, sizeof(af));

  /* so that the spell actually wears off properly, set it to stone skin */

  af.type = SPELL_STONE_SKIN;
  af.modifier = (level / 4) + number(1, 4);
  af.duration = BOUNDED(1, GET_LEVEL(ch) / 5, 10);
  affect_to_char(victim, &af);

  if (GET_CLASS(ch, CLASS_THEURGIST))
  {
    act("With a quick &+Cge&+cs&+bt&+cu&+Cre&n, $N's skin hardens to the &+ydensity&n of &+Lstone&n.", FALSE, ch, 0, victim, TO_CHAR);
    act("With a quick &+Cge&+cs&+bt&+cu&+Cre&n from $n, $N's skin hardens to the &+ydensity&n of &+Lstone&n.", FALSE, ch, 0, victim, TO_ROOM);
    act("With a quick &+Cge&+cs&+bt&+cu&+Cre&n from $n, your skin hardens to the &+ydensity&n of &+Lstone&n.", FALSE, ch, 0, victim, TO_VICT);
  }
  else
  {
    act("A dark chill from beyond the &+Lgrave&n permeates the air around $n.&n",
       FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("You feel protected by powers from beyond the &+Lgrave.\r\n", victim);
  }
}

void spell_prot_from_undead(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_PROT_FROM_UNDEAD))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROT_FROM_UNDEAD)
      {
        af1->duration = BOUNDED(2, GET_LEVEL(ch) / 3, 20);
      }
    return;
  }
  if(IS_UNDEADRACE(victim))
  {
    send_to_char("The target is undead! DUH!\n", ch);
    return;
  }
  bzero(&af, sizeof(af));
  af.type = SPELL_PROT_FROM_UNDEAD;
  af.modifier = GET_LEVEL(ch);
  af.duration = BOUNDED(2, GET_LEVEL(ch) / 3, 20);
  af.bitvector5 = AFF5_PROT_UNDEAD;
  affect_to_char(ch, &af);
  act("$N is now protected from undead!", TRUE, ch, 0, victim, TO_CHAR);
}


void spell_create_spring(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  P_obj    spring;

/*  if(!OUTSIDE(ch)) {
    send_to_char("Fountain indoors? Putz!\n", ch);
    return;
  }*/

  if (world[ch->in_room].sector_type == SECT_NO_GROUND ||
      world[ch->in_room].sector_type == SECT_UNDRWLD_NOGROUND ||
      world[ch->in_room].sector_type == SECT_OCEAN)
  {
  	send_to_char("&+bA spring usually needs more solid ground for support!\n", ch);
  	return;
  }

  spring = read_object(750, VIRTUAL);
  if(!spring)
  {
    logit(LOG_DEBUG, "spell_create_spring(): obj 750 (spring) not loadable");
    send_to_char("Tell someone to make a spring object ASAP!\n", ch);
    return;
  }
  spring->value[0] = GET_LEVEL(ch);
  send_to_room("&+bA spring shoots up from the ground!\n", ch->in_room);
  set_obj_affected(spring, 60 * 10, TAG_OBJ_DECAY, 0);
  obj_to_room(spring, ch->in_room);
}

void spell_regeneration(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;
  char     Gbuf1[100];
  int      skl_lvl;

  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
      return;

  if(affected_by_spell(victim, SPELL_ACCEL_HEALING) ||
    affected_by_spell(victim, SKILL_REGENERATE) ||
    affected_by_spell(victim, SPELL_REGENERATION))
  {
    send_to_char("You can't possibly heal any faster.\n", victim);
    return;
  }
  
  skl_lvl = MAX(3, ((level / 10) - 1));

  sprintf(Gbuf1, "You begin to regenerate rapidly.\n");

  bzero(&af, sizeof(af));
  af.type = SPELL_REGENERATION;
  af.duration = skl_lvl + 1;
  af.location = APPLY_HIT_REG;
  af.modifier = 3 * level;
  send_to_char(Gbuf1, victim);
  affect_to_char(victim, &af);
}

void spell_endurance(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  struct affected_type af;
  char     Gbuf1[100];
  int skl_lvl;

  if(affected_by_spell(victim, SPELL_ENDURANCE))
  {
    send_to_char("You can't possibly regain movement any faster.\n", victim);
    return;
  }

  if(affected_by_spell(ch, SPELL_MIELIKKI_VITALITY) &&
     !GET_CLASS(ch, CLASS_DRUID) &&
     !GET_SPEC(ch, CLASS_RANGER, SPEC_HUNTSMAN))
  {
    send_to_char("&+GThe Goddess Mielikki is aiding your health, and prevents the endurance spell from functioning...\r\n", victim);
    return;
  }

  skl_lvl = (int)(MAX(3, ((level / 4) - 1)) * get_property("spell.endurance.modifiers", 1.000));

  sprintf(Gbuf1, "You feel energy begin to surge through your limbs.\n");

  bzero(&af, sizeof(af));
  af.type = SPELL_ENDURANCE;
  af.location = APPLY_MOVE_REG;
  af.duration = skl_lvl;
  af.modifier = skl_lvl;
  send_to_char(Gbuf1, victim);
  affect_to_char(victim, &af);
}

void BackToUsualForm(P_char ch)
{
  P_event  e, save_ce = current_event;

  act("The mists in the room coalesce into $n's form...", TRUE, ch, 0, 0,
      TO_ROOM);
  act("You return to your ordinary form...", TRUE, ch, 0, 0, TO_CHAR);
  
  // looks like this stuff was remove elsewhere, so commenting this out
  // but the function is used elsewhere, just no event related stuff anymore

  /*if(!current_event || (current_event->type != EVENT_CHAR_EXECUTE) ||
      (current_event->target.t_func != BackToUsualForm))
  {
    save_ce = current_event;
    for (e = ch->events; e; e = e->next)
      if((e->type == EVENT_CHAR_EXECUTE) &&
          (e->target.t_func == BackToUsualForm))
        break;
    if(e)
    {
      current_event = e;
      RemoveEvent();
    }
    current_event = save_ce;
  }*/

  affect_from_char(ch, SPELL_WRAITHFORM);
}


void spell_elemental_form(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  int      type_mental;

  if(IS_AFFECTED3(victim, AFF3_ENLARGE))
  {
    send_to_char("You're quite large already.\n", victim);
    return;
  }
  if(!IS_AFFECTED3(victim, AFF3_ELEMENTAL_FORM))
  {
    char     buf1[500], buf2[500];

    type_mental = number(1, 4);

    switch (type_mental)
    {
    case 1:                    /* earth */
      strcpy(buf1,
             "$n begins to grow, turning into a large &+yearth elemental form!&n");
      send_to_char
        ("You call upon the power of the &+yelemental plane of earth!&N\n",
         ch);
      strcpy(buf2,
             "You grow, your body becoming more elemental than humanoid!&n");
      break;
    case 2:                    /* fire */
      strcpy(buf1,
             "$n begins to grow, turning into a large &+rfire elemental form!&n");
      send_to_char
        ("You call upon the power of the &+relemental plane of fire!&N\n",
         ch);
      strcpy(buf2,
             "You grow, your body becoming more elemental than humanoid!&n");
      break;
    case 3:                    /* water */
      strcpy(buf1,
             "$n begins to grow, turning into a large &+bwater elemental form!&n");
      send_to_char
        ("You call upon the power of the &+belemental plane of water!&N\n",
         ch);
      strcpy(buf2,
             "You grow, your body becoming more elemental than humanoid!&n");
      break;
    case 4:                    /* air */
      strcpy(buf1,
             "$n begins to grow, turning into a large &+Cair elemental form!&n");
      send_to_char
        ("You call upon the power of the &+Celemental plane of air!&N\n", ch);
      strcpy(buf2,
             "You grow, your body becoming more elemental than humanoid!&n");
      break;
    }

    act(buf1, TRUE, victim, 0, 0, TO_ROOM);
    act(buf2, TRUE, victim, 0, 0, TO_CHAR);

    switch (type_mental)
    {
    case 1:                    /* earth */
      spell_stone_skin(level, ch, 0, 0, ch, 0);
      bzero(&af, sizeof(af));
      af.type = SPELL_ARMOR;
      af.duration = 25;
      af.modifier = -30;
      af.location = APPLY_AC;
      affect_to_char(victim, &af);
      break;

    case 2:                    /* fire */
      if(!IS_AFFECTED2(victim, AFF2_FIRESHIELD))
        spell_fireshield(level, ch, NULL, 0, ch, 0);
      if(!IS_AFFECTED(victim, AFF_PROT_FIRE))
        spell_protection_from_fire(level, ch, 0, 0, ch, 0);
      break;

    case 3:                    /* water */
      if(!IS_AFFECTED(victim, AFF_WATERBREATH))
        spell_waterbreath(level, ch, NULL, 0, ch, 0);
      bzero(&af, sizeof(af));
      af.type = SPELL_ARMOR;
      af.duration = 25;
      af.modifier = -20;
      af.location = APPLY_AC;
      affect_to_char(victim, &af);
      af.modifier = 5;
      af.location = APPLY_DAMROLL;
      affect_to_char(victim, &af);
      break;

    case 4:                    /* air */
      if(!IS_AFFECTED(victim, AFF_FLY))
        spell_fly(level, ch, NULL, 0, ch, 0);
      if(!affected_by_spell(victim, SPELL_DETECT_INVISIBLE))
        spell_detect_invisibility(level, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
      if(!affected_by_spell(victim, SPELL_INVISIBLE))
        spell_improved_invisibility(level, ch, 0, 0, ch, 0);
      bzero(&af, sizeof(af));
      af.type = SPELL_ARMOR;
      af.duration = 25;
      af.modifier = -20;
      af.location = APPLY_AC;
      affect_to_char(victim, &af);

      break;
    }

    bzero(&af, sizeof(af));
    af.type = SPELL_ELEMENTAL_FORM;
    af.duration =  6;
    af.bitvector3 = AFF3_ELEMENTAL_FORM;
    affect_to_char(victim, &af);

    af.modifier = (victim->base_stats.Str / 5);
    af.location = APPLY_STR_MAX;
    affect_to_char(victim, &af);

    af.modifier = (victim->base_stats.Con / 2);
    af.location = APPLY_CON_MAX;
    affect_to_char(victim, &af);

    af.modifier = -(victim->base_stats.Agi / 2);
    af.location = APPLY_AGI;
    affect_to_char(victim, &af);

    af.modifier = -(victim->base_stats.Dex / 2);
    af.location = APPLY_DEX;
    affect_to_char(victim, &af);

  }
}

void spell_greater_wraithform(int level, P_char ch, P_char victim, char *arg)
{
  /*
   * this is _THE_ king of all kludges.. for npcs, race (not so
   * important) is saved in affect.. for pcs, its in arena_hits. the
   * difference is mostly so pcs wouldn't screw themselves _too_ much in
   * some cases. :-P Actual removal of wraithform is handled as event,
   * surprise surprise.
   */
  struct affected_type af;
  int /*i, */ dr;

/*  P_obj o1, o2; */

  dr = 0;
  /*
   * ok, now we _can_ use this beauty.. muhahahahahaa!
   */
  if(ch->specials.fighting)
    stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  StopAllAttackers(ch);
  act("$n fades into thin air..", TRUE, ch, 0, 0, TO_ROOM);
  act("You feel your senses blur.. and then recover.", TRUE, ch, 0, 0,
      TO_CHAR);
  act("Suddenly you have no body anymore!", TRUE, ch, 0, 0, TO_CHAR);

  if(!affected_by_spell(victim, SPELL_WRAITHFORM))
  {
    bzero(&af, sizeof(af));
    af.duration =  10;
    af.type = SPELL_WRAITHFORM;
    af.bitvector = AFF_WRAITHFORM;
    af.flags = AFFTYPE_NODISPEL;
    affect_to_char(ch, &af);
  }
  CharWait(ch, 3 * PULSE_VIOLENCE);
  /*
   * 12 minutes for a level 50 caster (12 hours)
   */
  /*
   * AddEvent(EVENT_CHAR_EXECUTE, GET_LEVEL(ch) * 200, TRUE, ch,
   * BackToUsualForm);
   */
}

void spell_wraithform(int level, P_char ch, P_char victim, char *arg)
{
  /*
   * this is _THE_ king of all kludges.. for npcs, race (not so
   * important) is saved in affect.. for pcs, its in arena_hits. the
   * difference is mostly so pcs wouldn't screw themselves _too_ much in
   * some cases. :-P Actual removal of wraithform is handled as event,
   * surprise surprise.
   */
  struct affected_type af;
  int    i;
  bool   dr;

  dr = FALSE;
  /*
   * ok, now we _can_ use this beauty.. muhahahahahaa!
   */
  send_to_char("Disabled!\n", ch);
  return;

  if(IS_RIDING(ch))
  {
    send_to_char("Get off your mount first.\n", ch);
    return;
  }
#if 0
  if( !IS_TRUSTED(ch) )
  {
    for (i = 0; i < CUR_MAX_WEAR; i++)
      if(ch->equipment[i])
      {
        obj_to_room(unequip_char(ch, i), ch->in_room);
        dr = TRUE;
      }
    for (o1 = ch->carrying; o1; o1 = o2)
    {
      o2 = o1->next_content;
      obj_from_char(o1);
      obj_to_room(o1, ch->in_room);
      dr = TRUE;
    }
    if(dr)
      send_to_char("Some of your equipment falls to the ground!\n", ch);
    act("As $n starts to fade, some equipment drops to the ground!", TRUE, ch,
        0, 0, TO_ROOM);
  }
#endif
  if(ch->specials.fighting)
    stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  StopAllAttackers(ch);
  act("$n fades into thin air..", TRUE, ch, 0, 0, TO_ROOM);
  act("You feel your senses blur.. and then recover.", TRUE, ch, 0, 0,
      TO_CHAR);
  act("Suddenly you have no body anymore!", TRUE, ch, 0, 0, TO_CHAR);
#if 0
  for (foll = ch->followers; foll; foll = next_foll)
  {
    next_foll = foll->followerfoll->next;

    if(IS_AFFECTED(foll->follower, AFF_CHARM))
      stop_follower(foll->follower);
  }
#endif

  if(!affected_by_spell(victim, SPELL_WRAITHFORM))
  {
    bzero(&af, sizeof(af));
    af.duration =  10;
    af.type = SPELL_WRAITHFORM;
    af.bitvector = AFF_WRAITHFORM;
    af.flags = AFFTYPE_NODISPEL;
    affect_to_char(ch, &af);
  }
  CharWait(ch, 3 * PULSE_VIOLENCE);
  /*
   * 12 minutes for a level 50 caster (12 hours)
   */
  /*
   * AddEvent(EVENT_CHAR_EXECUTE, GET_LEVEL(ch) * 200, TRUE, ch,
   * BackToUsualForm);
   */
}

void spell_command_undead(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!victim)
  {
    return;
  }
  if( /*(GET_CLASS(ch) != CLASS_NECROMANCER) || */ resists_spell(ch, victim))
    return;

  if(IS_GOOD(ch))
  {
    send_to_char
      ("You don't even _consider_ such an evil act, meddling with undead!",
       ch);
    return;
  }
  if(!IS_UNDEADRACE(victim))
  {
    act("$n just tried to command you, what a moron!", TRUE, ch, 0, victim,
        TO_VICT);
    act("$N thinks $n is really strange.", TRUE, ch, 0, victim, TO_NOTVICT);
    act("Um... $N isn't undead...", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  /* okay, we can charm them IF they are a 1201 undead and not charmed, OR
     they fail a saving throw versus spell */

  if(victim->following && (victim->following == ch))
    return;                     /* they're already following us */

  if((IS_NPC(victim) && (GET_RNUM(victim) == real_mobile(1201)) &&
       !GET_MASTER(victim)) &&
      !NewSaves(victim, SAVING_PARA, -4) &&
      ((GET_LEVEL(ch) - 10) > GET_LEVEL(victim)))
  {                             /* para save is deathmagic
       save right? *//* got em */
    if(victim->following)
      stop_follower(victim);
    add_follower(victim, ch);
    setup_pet(victim, ch, level / 4, 0);
    if(IS_FIGHTING(victim))
      stop_fighting(victim);
    if(IS_DESTROYING(victim))
      stop_destroying(victim);
    StopMercifulAttackers(victim);
  }
  else if(IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }
}

void spell_command_horde(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  P_char   tar, tar2;

  if(IS_GOOD(ch))
  {
    send_to_char
      ("You don't even _consider_ such an evil act, meddling with undead!",
       ch);
    return;
  }
  for (tar = world[ch->in_room].people; tar; tar = tar2)
  {
    tar2 = tar->next_in_room;
    if(!IS_UNDEADRACE(tar))
      continue;
    spell_command_undead(level, ch, arg, type, tar, obj);
  }
}

void spell_turn_undead(int level, P_char ch, char *arg, int type, P_char tch,
                       P_obj obj)
{
  int      diff;
  P_char   victim, next;

  act("$n raises $s holy symbol and shouts 'Begone!'", TRUE, ch, 0, 0,
      TO_ROOM);
  act("You raise your holy symbol and shout 'Begone!'", TRUE, ch, 0, 0,
      TO_CHAR);
  for (victim = world[ch->in_room].people; victim; victim = next)
  {
    next = victim->next_in_room;

    if((IS_UNDEADRACE(victim) || IS_ANGEL(victim)) && should_area_hit(ch, victim))
    {
      diff = level - GET_LEVEL(victim);
      if(diff <= 0 && !IS_PC_PET(victim))
      {
        act("You are powerless to affect $N!", TRUE, ch, 0, victim, TO_CHAR);
        return;
      }
      else if(diff <= 30)
      {
        if(!NewSaves(victim, SAVING_FEAR, 3 + diff / 4) && !fear_check(victim))
        {
          act("$n forces $N from this room.", TRUE, ch, 0, victim,
              TO_NOTVICT);
          act("You force $N from this room.", TRUE, ch, 0, victim, TO_CHAR);
          act("$n forces you from this room.", TRUE, ch, 0, victim, TO_VICT);
          do_flee(victim, 0, 2);
        }
        else
        {
          act("You laugh at $n.", TRUE, ch, 0, victim, TO_VICT);
          act("$N laughs at $n.", TRUE, ch, 0, victim, TO_NOTVICT);
          act("$N laughs at you.", TRUE, ch, 0, victim, TO_CHAR);
        }
      }
      else if(diff > number(0, 100))
      {
        act("As the magic that once brought $n to life has been dispelled, "
            "$e falls apart.", FALSE, victim, 0, 0, TO_ROOM);
        act("As a powerful force emanating from the symbol drains the magic "
            "animating you, your body starts falling apart!",
            FALSE, victim, 0, 0, TO_CHAR);
        die(victim, ch);
      }
      else
      {
        do_flee(victim, 0, 2);
      }
    }
  }
}

void spell_holy_light(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!IS_AFFECTED4(victim, AFF4_MAGE_FLAME) &&
      !IS_AFFECTED4(victim, AFF4_GLOBE_OF_DARKNESS))
  {
    act("&+WA bright, white light suddenly appears over $n&+W's head!",
        TRUE, victim, 0, 0, TO_ROOM);
    act("&+WA bright, white light appears over your head!", TRUE, victim,
        0, 0, TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_HOLY_LIGHT;
    af.duration = (level / 2 + 3);
    af.modifier = level;
    af.bitvector4 = AFF4_MAGE_FLAME;
    affect_to_char(victim, &af);

    affect_total(victim, FALSE);

    char_light(victim);
    room_light(victim->in_room, REAL);
  }
  else
    send_to_char
      ("You must get rid of your existing holy light or globe of darkness before you can create a holy light.\n",
       victim);
}

void spell_mage_flame(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!IS_AFFECTED4(victim, AFF4_MAGE_FLAME) &&
      !IS_AFFECTED4(victim, AFF4_GLOBE_OF_DARKNESS))
  {
    act("&+WA bright, unwavering torch suddenly appears over $n&+W's head!",
        TRUE, victim, 0, 0, TO_ROOM);
    act("&+WA bright, unwavering torch appears over your head!", TRUE, victim,
        0, 0, TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_MAGE_FLAME;
    af.duration = (level / 2 + 3);
    af.modifier = level;
    af.bitvector4 = AFF4_MAGE_FLAME;
    affect_to_char(victim, &af);

    affect_total(victim, FALSE);

    char_light(victim);
    room_light(victim->in_room, REAL);
  }
  else
    send_to_char
      ("You must get rid of your existing mage flame or globe of Osrell before you can create a new mage flame.\n",
       victim);
}

void spell_globe_of_darkness(int level, P_char ch, char *arg, int type,
                             P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!IS_AFFECTED4(victim, AFF4_MAGE_FLAME) &&
      !IS_AFFECTED4(victim, AFF4_GLOBE_OF_DARKNESS))
  {
    act("&+LA pitch-black sphere suddenly appears over $n&+L's head!", TRUE,
        victim, 0, 0, TO_ROOM);
    act("&+LA pitch-black sphere appears over your head!", TRUE, victim, 0, 0,
        TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_GLOBE_OF_DARKNESS;
    af.duration = (level / 4 + 1);
    af.modifier = level;
    af.bitvector4 = AFF4_GLOBE_OF_DARKNESS;
    affect_to_char(victim, &af);

    affect_total(victim, FALSE);

    char_light(victim);
    room_light(victim->in_room, REAL);

  }
  else
    send_to_char
      ("You must get rid of your existing mage flame or globe of darkness before you can create a globe of darkness.\n",
       victim);
}
void spell_fireshield(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!IS_AFFECTED2(victim, AFF2_FIRESHIELD) &&
      !IS_AFFECTED3(victim, AFF3_COLDSHIELD) &&
      !IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
  {
    act("&+R$n &+Ris surrounded by burning flames!", TRUE, victim, 0, 0, TO_ROOM);
    act("&+RYou are surrounded by an aura of burning flames!&n", TRUE, victim, 0, 0, TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_FIRESHIELD;
    af.duration =  6;
    af.bitvector2 = AFF2_FIRESHIELD;
    affect_to_char(victim, &af);
  }
}

void spell_coldshield(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!IS_AFFECTED2(victim, AFF2_FIRESHIELD) &&
      !IS_AFFECTED3(victim, AFF3_COLDSHIELD) &&
      !IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
  {
    act("&+B$n &+Bis surrounded by an aura of deadly cold!&n", TRUE, victim, 0, 0, TO_ROOM);
    act("&+BYou are surrounded by freezing cold!&n", TRUE, victim, 0, 0, TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_COLDSHIELD;
    af.duration =  6;
    af.bitvector3 = AFF3_COLDSHIELD;
    affect_to_char(victim, &af);
  }
}

void spell_lightning_shield(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!IS_AFFECTED2(victim, AFF2_FIRESHIELD) &&
      !IS_AFFECTED3(victim, AFF3_COLDSHIELD) &&
      !IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
  {
    act("&+Y$n &+Yis surrounded by a crackling maelstrom of energy!", TRUE, victim, 0, 0, TO_ROOM);
    act("&+YYou are surrounded by an crackling maelstrom of energy!&n", TRUE, victim, 0, 0, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_LIGHTNINGSHIELD;
    af.duration =  6;
    af.bitvector3 = AFF3_LIGHTNINGSHIELD;
    affect_to_char(victim, &af);
  }
}

void spell_negative_energy_barrier(int level, P_char ch, char *arg, int type,
                                   P_char victim, P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED2(victim, AFF2_SOULSHIELD))
  {
        send_to_char("&+LUsing this in conjuction with the putrid &+Wholy&+L energy is absurd.&n\n", ch);
        return;
  }

  if(!IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
  {
    act("&+L$n&+L is surrounded by a deadly aura of &n&+bnegative energy!",
        TRUE, victim, 0, 0, TO_ROOM);
    act("&+LYou are surrounded by a deadly aura of &n&+bnegative energy!",
        TRUE, victim, 0, 0, TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_NEG_ENERGY_BARRIER;
    af.duration =  10;
    af.bitvector4 = AFF4_NEG_SHIELD;
    affect_to_char(victim, &af);
  }
}

void spell_slow_poison(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_SLOW_POISON))
  {
    act("$n looks slightly more healthy.", TRUE, victim, 0, 0, TO_ROOM);
    act("You feel slightly more healthy.", TRUE, victim, 0, 0, TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_SLOW_POISON;
    af.duration = level/2;
    af.bitvector = AFF_SLOW_POISON;
    affect_to_char(victim, &af);
  }
}

void spell_comprehend_languages(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_COMPREHEND_LANGUAGES))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_COMPREHEND_LANGUAGES;
    af.duration = level * 2;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_COMPREHEND_LANGUAGES)
      {
        af1->duration = level * 2;
      }
  }

  send_to_char
    ("&+WYou feel your understanding of the languages of Duris improve!\n",
     victim);
}

void spell_single_obtenebration(int level, P_char ch, char *arg, int type,
                                   P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "&+LYour voice fills the living with dread and despair!&n",
    "&+LA wave of blackness sweeps over you, eating your flesh!&n",
    "&+LA pitch-darkness sweeps across the area, scalding the living!&n",
    "&+LYour voice disconnects every bit of $N's being.&n",
    "&+LA wave of blackness utterly consumes you.&n",
    "&+LA pitch-darkness consumes $N!&n"
  };
  
  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  dam = 100 + level * 6 + number(1, 40);
  dam = dam * get_property("spell.area.damage.factor.obtenebration", 1.000);

  spell_damage(ch, victim, dam, SPLDAM_SOUND, 0, &messages);

  if (IS_ALIVE(ch) && number(1, 100) < 20)
  {
    act("&+LYou direct a cloud of &+yu&+Lm&+yb&+Lr&+ya&+Ll &+yd&+Lu&+ys&+Lt in $N's face, blinding them!&n", 
	FALSE, ch, 0, victim, TO_CHAR);
    act("&+LA cloud of &+yu&+Lm&+yb&+Lr&+ya&+Ll &+yd&+Lu&+ys&+Lt kicks up, blinding $N!",
	FALSE, ch, 0, victim, TO_ROOM);
    act("&+LA cloud of &+yu&+Lm&+yb&+Lr&+ya&+Ll &+yd&+Lu&+ys&+Lt kicks up, blinding YOU!",
	FALSE, ch, 0, victim, TO_VICT);
    blind(ch, victim, GET_LEVEL(ch) / 3);
  }

  if (IS_ALIVE(ch) && number(1, 100) < 1)
  {
    act("&+LYou cause a rift in the spacetime continuum, which eats $N!&n",
	FALSE, ch, 0, victim, TO_CHAR);
    act("&+LA void of nothingness appears behind $N whom vanishes with a soft pop!&n", 
	FALSE, ch, 0, victim, TO_ROOM);
    act("&+LYour atoms explode and disperse, reappearing elsewhere!",
	FALSE, ch, 0, victim, TO_VICT);
    spell_teleport(GET_LEVEL(ch), ch, 0, 0, victim, 0);
  }
}

void spell_obtenebration(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  P_char   tch;
  int room = ch->in_room;
  struct room_affect raf;

  {
    send_to_char
      ("&+rYou begins chanting in a demonic voice.\n",
       ch);
    act("&+r$n begin chanting in a demonic voice!",
        FALSE, ch, 0, 0, TO_ROOM);
  }

  zone_spellmessage(ch->in_room,
    "&+LOff in the distance there is a &+wpiercing vibration.\n",
    "&+LOff in the distance to the %s there is a &+wpiercing vibration.\n");
 
  cast_as_damage_area(ch, spell_single_obtenebration, level, victim,
                      get_property("spell.area.minChance.obtenebration", 50),
                      get_property("spell.area.chanceStep.obtenebration", 20));
}

void spell_single_incendiary_cloud(int level, P_char ch, char *arg, int type,
                                   P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "You wave the gases towards $N and have the satisfaction of seeing $M enveloped in &+rflames&N.",
    "You are enveloped in a cloud of incendiary gases sent by $n - OUCH!!",
    "$n cackles as $s incendiary cloud torches $N.",
    "Your wall of incendiary gases torches $N instantly, causing immediate death!",
    "As the gases surround you, $n grins evilly -  then you burst into flames and die.",
    "The incendiary cloud from $n turns $N into a charred corpse.", 0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  dam = dice(3 * level, 7)/2;

  if( !NewSaves(victim, SAVING_SPELL, 0) )
  {
    dam = dam * 1.5;
  }

  if (IS_PC(ch) && IS_PC(victim))
  {
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);
  }
  dam = dam * get_property("spell.area.damage.factor.incendiaryCloud", 1.000);
  if(GET_SPEC(ch, CLASS_SORCERER, SPEC_WIZARD))
  {
   dam = dam * 1.4;
  }

  spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
}

void spell_incendiary_cloud(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  P_char   tch;
  int room = ch->in_room;
  struct room_affect raf;

/*   if(IS_PC(ch) &&
      (tch =
       stack_area(ch, SPELL_INCENDIARY_CLOUD,
                  (int) get_property("spell.area.stackTimer.incendiaryCloud",
                                     5))))
  {
    act("&+rYour incendiary gases add to $N's raging inferno.", FALSE, ch, 0,
        tch, TO_CHAR);
    act
      ("&+rClouds of incendiary gases pour from $n's fingertips and add to the raging inferno!",
       FALSE, ch, 0, 0, TO_ROOM);
  }
  else */
  {
    send_to_char
      ("&+rBillowing clouds of incendiary gases pour from your fingertips.\n",
       ch);
    act("&+rBillowing clouds of incendiary gases pour from $n's fingertips!",
        FALSE, ch, 0, 0, TO_ROOM);
  }

  zone_spellmessage(ch->in_room,
    "&+yOff in the distance there is a &+Ythundering &+Rroar &+yand &+wbillowing &+Lsmoke.\n",
    "&+yOff in the distance to the %s there is a &+Ythundering &+Rroar &+yand &+wbillowing &+Lsmoke.\n");
 
  cast_as_damage_area(ch, spell_single_incendiary_cloud, level, victim,
                      get_property("spell.area.minChance.incendiaryCloud", 50),
                      get_property("spell.area.chanceStep.incendiaryCloud", 20));

  memset(&raf, 0, sizeof(raf));
  raf.type = SPELL_INCENDIARY_CLOUD;
  raf.duration = int(1.5 * PULSE_VIOLENCE);
  affect_to_room(room, &raf);

  for (tch = world[room].people; tch; tch = tch->next_in_room)
    if(IS_AFFECTED5(tch, AFF5_WET)) {
      send_to_char("The heat of the cloud dried up your clothes.\n", tch);
      make_dry(tch);
    }
}

void unequip_char_dale(P_obj kala)
{
  int      a, b;

  b = -1;
  for (a = 0; a < MAX_WEAR; a++)
    if(OBJ_WORN_POS(kala, a))
      b = a;
  if(b != -1)
    unequip_char(kala->loc.wearing, b);
}

void spell_disintegrate(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int i, dam;
  P_obj x;
  struct damage_messages messages = {
    "You smile happily as your disintegration ray hits $N!",
    "Your body quivers and shakes as $n hits you with a disintegration ray, but you manage to stay together.",
    "$n cackles as $s disintegration ray strikes $N hard!",
    "You disintegrate $N into small bits!",
    "You scream as you fly into a million pieces. $n grins evilly.",
    "$n disintegrates $N into a pile of dust!", 0
  };
  struct damage_messages eqburnmsg = {
    "$p turns red hot, $N screams as $p burns him.",
    "$p turns red hot and burns you!",
    "$p turns red hot and burns $N!",
    "$p turns red hot and burns the last bit of life out of $N!",
    "$p turns red hot and burns the last bit of life out of you!",
    "$p turns red hot and burns the last bit of life out of $N!", 0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  dam = dice(level, 13);

  act("$n sends a bright &+Ggreen ray of light&n streaking towards&n $N!",
    TRUE, ch, 0, victim, TO_NOTVICT);
  act("$n sends a &+Ggreen ray of light&n coming .. straight towards YOU!",
    TRUE, ch, 0, victim, TO_VICT);
  act("You grin evilly as you send a &+Ggreen beam of disintegration&n streaking towards&n $N!",
    TRUE, ch, 0, victim, TO_CHAR);

  if(resists_spell(ch, victim))
    return;

  if(!saves_spell(victim, SAVING_SPELL))
  {
    if(!IS_AFFECTED4(victim, AFF4_NEG_SHIELD) &&
       !IS_UNDEADRACE(victim))
    {
      dam = (dam * 2);

      if(!IS_SET(world[victim->in_room].room_flags, ARENA))
      {
        i = 0;
        do
        {                         /* could make this check the carried EQ as well...  */
          if(victim->equipment[i])
          {
            obj = victim->equipment[i];

            if(!NewSaves(victim, SAVING_SPELL, -5) && !CHAR_IN_ARENA(victim) && !IS_ARTIFACT(obj) && !IS_NOSHOW(obj) )
            {
              spell_damage(ch, victim, (int)get_property("spell.disintegrate.burn.dmg", 3), SPLDAM_FIRE, 0, &eqburnmsg);
              obj->condition -= BOUNDED(0, number(1, (int)get_property("spell.disintegrate.max.eq.dmg", 10)),
                (obj->condition-1));
              act("$p cracks from the heat.", FALSE, ch, obj, victim, TO_VICT);
              /* Getting rid of this spammy useless message crap - Jexni 7/17/08
              statuslog(AVATAR, "%s just disintegrated %s from %s at [%d]", GET_NAME(ch),
                obj->short_description, GET_NAME(victim), world[ch->in_room].number);

              act("$p turns red hot, $N screams, then it disappears in a puff of smoke!",
                 TRUE, ch, obj, victim, TO_CHAR);

              if( obj->loc.wearing || obj->loc.carrying)
              {
                act("$p, held by $N, disappears in a puff of smoke!", TRUE, ch, obj, victim, TO_ROOM);
              }

              // remove the obj
              if(OBJ_CARRIED(obj))
              {
                obj_from_char(obj);
              }
              else if(OBJ_WORN(obj))
              {
                unequip_char_dale(obj);
              }
              else if(OBJ_INSIDE(obj))
              {
                obj_from_obj(obj);
              }
              if(obj->contains)
              {
                while (obj->contains)
                {
                  x = obj->contains;
                  obj_from_obj(x);
                  obj_to_room(x, ch->in_room);
                }
              }
              if(obj)
              {
                extract_obj(obj);
                obj = NULL;
              }
            }
            else
            {
              if(obj)
              {
                act("$p resists the disintegration ray completely!", TRUE, ch,
                    obj, victim, TO_VICT);
                act("$p, carried by $N, resists the disintegration ray!", TRUE,
                    ch, obj, victim, TO_ROOM);
              }
            */
            }
          }
          i++;
        }
        while (i < MAX_WEAR);
      }
    }
  }                             /* else dam = 0; */
  spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG, &messages);
}

void spell_shatter(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      i, dam, savemod;

  struct damage_messages messages = {
    "&+C$N screams as $E is hit by your ghastly wave of sound!",
    "$n &+Cbombards you with a massive wave of sound. You feel as if your head will explode!",
    "&+CYou wince as $n &+Csends out an agonizing wave of sound at $N!",
    "&+LYour sound wave has utterly shattered $N's &+Linternal organs!",
    "&+LYou feel your internal organs burst as $n &+Lemits a vile sound.",
    "$N doubles over as $S internal organs burst and begin leaking out of $S body! $n snorts lewdly.",
      0
  };

  // Corresponds to approx bigbys + 10 damage saved (This is 9th circle vs bigbys 8th for Bards).
  dam = 15 * level + dice(3, level);
  // 92 - 100 pow has no bonus, < 92 pow means easier to save, > 100 pow means harder to save.
  savemod = STAT_INDEX( GET_C_POW(ch) ) - 15;

  act("$n &+Lglares at $N&+L, and begins emanating a &+RHORRIBLE&+L sound!", TRUE, ch, 0, victim, TO_NOTVICT);
  act("$n &+Lglares at you, and begins to emanate a deep, vicious sound!", TRUE, ch, 0, victim, TO_VICT);
  act("&+LYou glare at $N&+L, and begin to generate a wretched sound of death!", TRUE, ch, 0, victim, TO_CHAR);
  if( !NewSaves(victim, SAVING_SPELL, savemod) )
  {
    if( !CHAR_IN_ARENA(ch) && !CHAR_IN_ARENA(victim) )
    {
      i = 0;
      do
      {
        if(victim->equipment[i])
        {
          obj = victim->equipment[i];
          // Hits 2 out of 3 non-artifact eq'd items.
          if( number(0, 2) && !IS_ARTIFACT(victim->equipment[i]) )
          {
            // 4% chance to destroy it outright.
            int destroy = !number(0, 24);

            DamageOneItem(victim, SPLDAM_SOUND, obj, destroy);
            if( destroy )
            {
              statuslog(AVATAR, "%s just shattered %s from %s at [%d]",
                GET_NAME(ch), obj->short_description, GET_NAME(victim), world[ch->in_room].number);
            }
          }
        }
        i++;
      }
      while( i < MAX_WEAR );
      // Could make this check the carried EQ as well...
    }
  }
  // If they saved
  else
  {
    dam = (dam * 2)/3;
  }
  spell_damage(ch, victim, dam, SPLDAM_SOUND, SPLDAM_NOSHRUG, &messages);
}

void spell_cause_light(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "Your touch sends shivers of pain through $N's body.",
    "You are filled with pain as $n touches you.",
    "$n touches $N, who shivers in pain.",
    "Your light wounds are enough to send $N over the brink....",
    "For a moment, $n wracks you with pain.   Then, there is nothing.",
    "The wounds inflicted by $n are enough to end the life of $N."
  };

  dam = 4 * dice(1, 8);

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    dam = (int) (dam * 1.25);
  }

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_cause_serious(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "$N's body is wracked with spasms as you touch $M!",
    "Blinding &+Rpain&n shoots through your body as $n touches you!",
    "$n's touch doesn't seem so casual as $N doubles over in pain!",
    "You seriously wound $N unto the point of death.",
    "$n sends blistering pain down your spine.  Then there is only darkness.",
    "$n's spell causes sufficient wounds to kill $N."
  };

  dam = 4 * dice(2, 8);

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    dam = (int) (dam * 1.25);
  }

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_unholy_wind(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "&+LYou send a ghastly wind out to rot $N's&+L flesh.",
    "&+L$n&+L sends a ghastly wind out to rot your flesh.",
    "&+L$n&+L send a ghastly wind out to rot $N's&+L flesh.",
    "&+LYour evil wind kills $N&+L, leaving only a hump of blackness.",
    "&+L$n's&+L evil wind destroys you, leaving only a hump of blackness.",
    "&+L$n's&+L evil wind kills $N&+L, leaving only a hump of blackness."
  };

  if(IS_UNDEADRACE(victim))
  {
    send_to_char("&+LYour victim is not among the living.&n\n", ch);
    return;
  }

  dam = (dice(1, 6) + 5 * 4 + level) ;

  bool failed_save = !NewSaves(victim, SAVING_SPELL, 0);

  if (failed_save)
    dam <<= 1;

  spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_ALLGLOBES, &messages);
}

void spell_purge_living(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int      dam, temp;
  struct damage_messages messages = {
    "&+MYou direct a &+Ldark beam&+M at $N attempting to purge $S soul!",
    "&+M$n&+M directs a &+Ldark beam&+M at you attempting to purge your soul!",
    "&+M$n&+M direct a &+Ldark beam&+M at $N&+M attempting to purge $S soul!",
    "&+MYou have purged the realms of yet another living soul!",
    "&+M$n&+M has purged the realms of yet another living soul... YOU!",
    "&+M$n&+M has purged the realms of yet another living soul!"
  };

  if(IS_UNDEADRACE(victim))
  {
    send_to_char("&+LYour victim is not among the living.&n\n", ch);
    return;
  }
  temp = MIN(35, (level + 1));
  dam = dice(temp, 10);


  spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);
}

void spell_cause_critical(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "Your touch sends wracking pains through $N's body.",
    "You are almost dissolved by pain as $n touches you.",
    "$N screams as $n critically injures $M.",
    "You kill $N with a critical touch.",
    "You are wracked with searing pain from $n. Then, no more.",
    "$n's touch critically wounds $N, who dies from the pain."
  };

  dam = dice(12, 8) + 40;

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    dam = (int) (dam * 1.25);
  }

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}


void spell_acid_stream(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "You direct a stream of &+Gacid&n into $N; $E cries out as flesh that was once $S is now lost to the elements.",
    "$n sends a stream of &+Gacid&n at you, burning away skin and muscle that you rather miss.",
    "$n sends a stream of &+Gacid&n into $N, vaporizing fragile carbon compounds and creating a horrible stench.",
    "You direct a stream of &+Gacid&n into $N; $S life is quickly terminated.",
    "$n sends a stream of &+Gacid&n at you, coating your entire body! You don't feel so good.",
    "$n sends a perfectly-aimed stream of &+Gacid&n arcing towards $N, turning $M into so many base compounds!"
  };

  dam = dice(MIN(51, level - 4) + 3, 14);
  if(spell_damage(ch, victim, dam, SPLDAM_ACID, 0, &messages) ==
      DAM_NONEDEAD)
    spell_slow(level, ch, 0, 0, victim, obj);
}

void spell_acid_blast(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "$N screams in pain as your &+Gacid blast&n burns $M.",
    "$n blasts you with acid... OUCH!",
    "$n &+Gblasts&n $N with &+Gacid,&n you cringe.",
    "$N is dissolved into a sticky ooze.",
    "You are hit by a &+Gblast of acid&n from $n. Goodbye cruel world.",
    "$n turns $N into a &+Gsticky puddle.&n", 0
  };

  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim) ||
     ch->in_room != victim->in_room)
        return;

  int dam = dice(MIN(level, 21), 10) + level/2;

  if(!NewSaves(victim, SAVING_SPELL, 0))
    dam = (int) (dam * 1.25);

  spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_ALLGLOBES, &messages);
}

void spell_single_icestorm(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "You crush $N with your &+Cstorm of ice.",
    "$n bashes you with a &+Cstorm of ice.",
    "$n crushes $N with a &+Cstorm of ice.",
    "$N is ripped apart by your &+Cstorm of ice.",
    "You are ripped to shreds by $n's &+Cice storm.",
    "$N is ripped to shreds by $n's &+Cice storm.", 0
  };

  int num_dice = MIN(level, 36);
  int dam = dice(num_dice, 8);
  dam = dam * get_property("spell.area.damage.factor.iceStorm", 1.000);

  if(is_hot_in_room(victim->in_room)) {
    send_to_char("&+bYou are splashed by water&n!\n", victim);
    act("$n is splashed with &+bwater&n!", FALSE, victim, 0, 0, TO_ROOM);
    make_wet(victim, WAIT_MIN);
  } else {
    if (IS_PC(ch) && IS_PC(victim))
      dam = dam * get_property("spell.area.damage.to.pc", 0.5);

    send_to_char("You are blasted by the storm!\n", victim);
    spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_GLOBE, &messages);
  }
}

void spell_ice_storm(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  bool     rain;

  if(victim == ch)
  {
    send_to_char("You suddenly decide against that, oddly enough.\n", ch);
    return;
  }

  if(is_hot_in_room(ch->in_room)) {
    send_to_char("Your storm of ice turns into a fountain of &+bwater&n from the heat&n!\n", ch);
    act("$n conjures an ice storm!", FALSE, ch, 0, 0, TO_ROOM);
    act("The heat in here makes all ice melt!", FALSE, ch, 0, 0, TO_ROOM);
  } else {
    send_to_char("&+WYou conjure a storm of ice&n!\n", ch);
    act("$n conjures an ice storm!", FALSE, ch, 0, 0, TO_ROOM);
  }

  cast_as_damage_area(ch, spell_single_icestorm, level, victim,
                      get_property("spell.area.minChance.iceStorm", 90),
                      get_property("spell.area.chanceStep.iceStorm", 10));
  zone_spellmessage(ch->in_room,
                    "&+CYou feel a blast of &+Bcold!\n",
                    "&+CYou feel a blast of &+Bcold &+Cfrom the %s!\n");
}

void spell_faerie_fire(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af, *af2, *af3;
  int      a;

  if(GET_STAT(ch) == STAT_DEAD)
    return;

  if(affected_by_spell(victim, SPELL_FAERIE_FIRE))
  {
    send_to_char("Nothing new seems to happen.\n", ch);
    return;
  }
  act("$n points at $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  act("You point at $N.", TRUE, ch, 0, victim, TO_CHAR);
  act("$n points at you.", TRUE, ch, 0, victim, TO_VICT);
  if(affected_by_spell(ch, SPELL_INVISIBLE))
    for (af2 = victim->affected; af2; af2 = af3)
    {
      af3 = af2->next;
      if(af2->type == SPELL_INVISIBLE)
        affect_remove(victim, af2);
    }
  a = 0;
  if(IS_SET(victim->specials.affected_by, AFF_INVISIBLE) ||
      IS_AFFECTED2(victim, AFF2_MINOR_INVIS))
  {
    a = 1;
    REMOVE_BIT(victim->specials.affected_by, AFF_INVISIBLE);
    REMOVE_BIT(victim->specials.affected_by2, AFF2_MINOR_INVIS);
  }
  else if(!number(0, 1) && IS_DISGUISE(victim))
  {
    remove_disguise(victim, TRUE);
    justice_witness(victim, NULL, CRIME_DISGUISE);
  }
  act("$N is surrounded by the dancing outline of &+mpurplish flames!&n",
      TRUE, ch, 0, victim, TO_NOTVICT);
  act("$N is surrounded by the dancing outline of &+mpurplish flames!&n",
      TRUE, ch, 0, victim, TO_CHAR);
  act("You are surrounded by the dancing outline of &+mpurplish flames!&n",
      TRUE, ch, 0, victim, TO_VICT);
  if(a)
    SET_BIT(victim->specials.affected_by2, AFF2_MINOR_INVIS);
  bzero(&af, sizeof(af));
  af.type = SPELL_FAERIE_FIRE;
  af.duration =  5;
  af.modifier = 20;
  af.location = APPLY_ARMOR;

  affect_to_char(victim, &af);

  if(IS_NPC(victim) && CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
      MobStartFight(victim, ch);
  }
}

void spell_faerie_fog(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  ulong    a, a2;
  P_char   tmp_victim;

  act
    ("&+m$n&n&+m snaps $s fingers, and a cloud of purple smoke billows forth!",
     TRUE, ch, 0, 0, TO_ROOM);
  act
    ("&+mYou snap your fingers, causing a cloud of purple smoke to billow forth.",
     TRUE, ch, 0, 0, TO_CHAR);

  LOOP_THRU_PEOPLE(tmp_victim, ch)
  {
    if((ch != tmp_victim) && !IS_TRUSTED(tmp_victim) &&
        (IS_AFFECTED(tmp_victim, AFF_INVISIBLE) ||
         IS_AFFECTED(tmp_victim, AFF_HIDE) ||
         IS_AFFECTED2(tmp_victim, AFF2_MINOR_INVIS)))
    {
      if(NewSaves(tmp_victim, SAVING_SPELL, (int)(level - GET_LEVEL(tmp_victim))))
      {
        a = tmp_victim->specials.affected_by;
        a2 = tmp_victim->specials.affected_by2;
        if(IS_AFFECTED2(tmp_victim, AFF2_MINOR_INVIS))
          REMOVE_BIT(tmp_victim->specials.affected_by2, AFF2_MINOR_INVIS);
        if(IS_AFFECTED(tmp_victim, AFF_INVISIBLE))
          REMOVE_BIT(tmp_victim->specials.affected_by, AFF_INVISIBLE);
        if(IS_AFFECTED(tmp_victim, AFF_HIDE))
          REMOVE_BIT(tmp_victim->specials.affected_by, AFF_HIDE);
        act("$n is briefly revealed, but disappears again.", TRUE, tmp_victim,
            0, 0, TO_ROOM);
        act("You are briefly revealed, but disappear again.", TRUE,
            tmp_victim, 0, 0, TO_CHAR);
        tmp_victim->specials.affected_by = a;
        tmp_victim->specials.affected_by2 = a2;
      }
      else
      {
        if(IS_AFFECTED(tmp_victim, AFF_INVISIBLE) ||
            IS_AFFECTED2(tmp_victim, AFF2_MINOR_INVIS))
          affect_from_char(tmp_victim, SPELL_INVISIBLE);
        if(IS_AFFECTED2(tmp_victim, AFF2_MINOR_INVIS))
          REMOVE_BIT(tmp_victim->specials.affected_by2, AFF2_MINOR_INVIS);
        if(IS_AFFECTED(tmp_victim, AFF_INVISIBLE))
          REMOVE_BIT(tmp_victim->specials.affected_by, AFF_INVISIBLE);
        if(IS_AFFECTED(tmp_victim, AFF_HIDE))
          affect_from_char(tmp_victim, SKILL_HIDE);
        if(IS_AFFECTED(tmp_victim, AFF_HIDE))
          REMOVE_BIT(tmp_victim->specials.affected_by, AFF_HIDE);
        act("$n is revealed!", TRUE, tmp_victim, 0, 0, TO_ROOM);
        act("You are revealed!", TRUE, tmp_victim, 0, 0, TO_CHAR);
      }
    }
    else if(ch != tmp_victim && IS_DISGUISE(tmp_victim))
    {
      if(!number(0,1))
      {
        remove_disguise(tmp_victim, TRUE);
      }
      else if(IS_DISGUISE_ILLUSION(tmp_victim))
      {
        act("$n is cloaked in illusion!", TRUE, tmp_victim, 0, 0, TO_ROOM);
        act("Your illusion has been noticed!", TRUE, tmp_victim, 0, 0, TO_CHAR);
      }
      else if(IS_DISGUISE_SHAPE(tmp_victim))
      {
        act("$n is not in their true form!", TRUE, tmp_victim, 0, 0, TO_ROOM);
        act("Your true form has been noticed!", TRUE, tmp_victim, 0, 0, TO_CHAR);
      }
      else
      {
        act("$n is wearing a disguise!", TRUE, tmp_victim, 0, 0, TO_ROOM);
        act("Your disguise has been noticed!", TRUE, tmp_victim, 0, 0, TO_CHAR);
      }
      justice_witness(tmp_victim, NULL, CRIME_DISGUISE);
    }
  }
}

void spell_blackmantle(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if( !ch )
  {
    logit(LOG_EXIT, "spell_blackmantle called in magic.c with no ch");
    raise(SIGSEGV);
  }

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( ch == victim )
  {
    send_to_char("Stop wasting time and go kill someone!\r\n", ch);
    return;
  }

  if( affected_by_spell(victim, SPELL_BMANTLE) )
  {
    send_to_char("They are already afflicted by blackmantle!!\n", ch);
    return;
  }

  // Made the save level based for PC casters.. maxxes at 14 (lvl 56) instead of hard 3.
  if( !NewSaves(victim, SAVING_SPELL, IS_NPC(ch) ? 16 : level / 4) )
  {
    act("&+LA blanketing shroud of &+bnegative energy &+Lcoalesces around $N&+L...", FALSE, ch, 0, victim, TO_CHAR);
    act("&+LA blanketing shroud of &+bnegative energy &+Lcoalesces around $N&+L...", FALSE, ch, 0, victim, TO_NOTVICT);
    act("&+LA cloud of &+bnegative energy &+Ldroplets cloak you, slowly draining the &+wlife &+Lfrom your body...", FALSE, ch, 0, victim, TO_VICT);

    bzero(&af, sizeof(af));
    af.type = SPELL_BMANTLE;
    // Maxxes at 8 ticks at lvl 56.
    af.duration = level / 7;
    // Modifier is how many hps the blackmantle can absorb.
    af.modifier = IS_NPC(ch) ? 4000 : 60*level;
    affect_to_char(victim, &af);
  }
  else
  {
    act("&+LYour spell fails to afflict $N&+L!", FALSE, ch, 0, victim, TO_CHAR);
  }
}

void spell_pword_kill(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  int dam;

  struct damage_messages messages = {
    "$N's life force is drained slightly by the power of your word.",
    "$n's word of power causes you to sag, and you feel your vitality draining away!",
    "$N seems to sag slightly, as $n viciously attacks $S life force.",
    "$N dies instantly from the power of your word.",
    "You hear a word of power, and die instantly.",
    "$N hears $n's word of power, and nothing more."
  };

  if(!(ch) ||
     !IS_ALIVE(ch))
    return;

// The spell no longer functions against the undead races.

  if(IS_UNDEADRACE(victim))
  {
    send_to_char("&+LThe living dead are no longer affected by such an incantation...&n\r\n", ch);
    return;
  }

// This is now a straight line percentage check for the victim to pass
// a save versus spell or die().
// If the caster is more than 15 levels higher than the victim, the spell
// save check is automatically triggered.

  if(!IS_GREATER_RACE(victim) &&
     !IS_TRUSTED(victim) &&
     !IS_ELITE(victim) &&
     (GET_HIT(victim) < 1500) &&
     !NewSaves(victim, SAVING_SPELL, 15) &&
     (((int)(level - GET_LEVEL(victim)) >= number(0, 99)) || level > GET_LEVEL(victim) + 15))
  {
    act("&+Y$N &+Ydies instantly from the power of your word.&n", FALSE, ch, 0, victim, TO_CHAR);
    act("&+YYou hear a word of power, and die instantly.&n", FALSE, ch, 0, victim, TO_VICT);
    act("&+Y$N &+Yhears $n&+Y's word of power, and nothing more.&n", FALSE, ch, 0, victim, TO_NOTVICT);
    die(victim, ch);
  }
  else
  {
   dam = ((dice(3, 6) + level) * 4);
   
   if(IS_PC_PET(ch))
     dam /= 2;
   
   spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
  }
}

#define BANISHMENT_HOLY_WORD 1
#define BANISHMENT_UNHOLY_WORD 2
#define WISPY_BAND_VNUM 424

void astral_banishment(P_char ch, P_char victim, int hwordtype, int level)
{
  // hwordtype: 1 = holy word
  //            2 = unholy word
  P_obj wispy_band = read_object(WISPY_BAND_VNUM, REAL);
  int new_room, lev = GET_LEVEL(victim), found;

  if(GET_RACE(victim) == (hwordtype == BANISHMENT_HOLY_WORD ? RACE_GITHYANKI : RACE_GITHZERAI) &&
     number(0,100) < 2 + BOUNDED(-2, level-lev, 2) &&
     !IS_HOMETOWN(ch->in_room) &&
     !IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) &&
     !(world[ch->in_room].sector_type == SECT_OCEAN))
  {
    if(wispy_band &&
      OBJ_WORN_BY(wispy_band, victim))
    {
      act("You are protected by a powerful force, and the banishment fails.", FALSE, ch, 0, victim, TO_VICT);
      return;
    }
    
    if(hwordtype == BANISHMENT_HOLY_WORD)
    {
      act("You banish the evil $N to the astral plane with your holy word.", FALSE, ch, 0, victim, TO_CHAR);
      act("The power of the holy word causes you to flee to astral plane.", FALSE, ch, 0, victim, TO_VICT);
    }
    if(hwordtype == BANISHMENT_UNHOLY_WORD)
    {
      act("You banish the good $N to the astral plane with your unholy word.", FALSE, ch, 0, victim, TO_CHAR);
      act("The power of the unholy word causes you to flee to astral plane.", FALSE, ch, 0, victim, TO_VICT);
    }
    act("Terror flashes in $N's eyes and $E is no more.", FALSE, ch, 0, victim, TO_NOTVICT);
    affect_from_char(victim, TAG_PVPDELAY);
    char_from_room(victim);
    new_room = real_room(number(ASTRAL_VNUM_BEGIN,ASTRAL_VNUM_END));
    char_to_room(victim, new_room, -1);
  }
}

void spell_ghastly_touch(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj obj)
{
  struct damage_messages messages = {
    "&+LYou direct a &+wghast &+Ltowards&n $N&+L, and its shadowy fingers brush $m.",
    "&+LAn incorporeal ghastly figure touches you!",
    "&+LAn incorporeal ghastly figure brushes&n $N &+Lwith its shadowy &+wfingers.",
    "$N &+Lconvulses and dies a quick and quiet &+rdeath.",
    "&+LYou feel spectral fingers sap the last bit of &+clifeforce &+Lfrom you.",
    "$N &+Lquietly collapses and &+rdies!", 0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( IS_UNDEADRACE(victim) )
  {
    act("&+LYour ghast balks at attacking another &+rundead &+Land disperses quickly.&n", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

  if( GET_LEVEL(victim) < (level / 5) )
  {
    spell_damage(ch, victim, 10000, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT, &messages);
    return;
  }

  int dam = (int) number(level * 5, level * 7);

  if( IS_PC(ch) && IS_PC(victim) )
  {
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);
  }
  dam = dam * get_property("spell.area.damage.factor.summonGhasts", 1.000);

  if( spell_damage (ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD )
  {
    if( GET_LEVEL(victim) < level / 2 )
    {
      spell_minor_paralysis((int) (level / 2), ch, NULL, 0, victim, NULL);
    }
  }
}

void spell_heavens_aid(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "&+LYou direct the &+Wholy beam &+Ltowards $N&+L.",
    "$n&+L's holy light passes over you, burning you with holy power.",
    "$n &+Ldirects a &+Wholy beam&n &+Ltowards $N&+L, burning $m with its &+Wholy power&+L.",
    "$N &+Lconvulses and dies a quick and quiet &+rdeath.",
    "&+LYou feel the &+Wholy beam&n sap the last bit of &+clifeforce &+Wfrom you.",
    "$N quietly collapses and &+rdies!", 0
  };

  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
      return;

  if(RACE_GOOD(victim) || IS_ANGEL(victim))
  {
    act("&+WThe light from above passes over $N &+Wwithout harm.&n", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

  if(GET_LEVEL(victim) < (level / 5) &&
    spell_damage(ch, victim, 10000, SPLDAM_HOLY, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD)
      return;

  int dam;
  dam = (int) number(level * 5, level * 7);

  if (IS_PC(ch) && IS_PC(victim))
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);

  dam = dam * get_property("spell.area.damage.factor.aidOfTheHeavens", 1.000);

  if(spell_damage (ch, victim, dam, SPLDAM_HOLY, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD)
  {
    if(level < (GET_LEVEL(victim) / 2))  
      spell_minor_paralysis((int) (level / 2), ch, NULL, 0, victim, NULL);
  }
}

void event_aid_of_the_heavens(P_char ch, P_char victim, P_obj obj, void *data)
{
  int room;
  room = *((int*)data);
  if(room != ch->in_room)
  {
    send_to_char("&+LThe light from above dissolves into nothing...\n", ch);
    act("&+LThe light from above fades out of existence...", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  
  if(!number(0, 3))
  {
    act("$n&+W's light from above glides about the area.", FALSE, ch, 0, 0, TO_ROOM);
    add_event(event_aid_of_the_heavens, PULSE_VIOLENCE * 1, ch, 0, 0, 0, &room, sizeof(room));
    return;
  }
  
  act("$n&+L's summoned illumination stretches out!", FALSE, ch, 0, 0, TO_ROOM);
  act("&+LYour summoned illumination stretches out!.", FALSE, ch, 0, 0, TO_CHAR);  
  
  cast_as_damage_area(ch, spell_heavens_aid, GET_LEVEL(ch), NULL,
      get_property("spell.area.minChance.aidOfTheHeavens", 90),
      get_property("spell.area.chanceStep.aidOfTheHeavens", 10));
}

void event_summon_ghasts(P_char ch, P_char victim, P_obj obj, void *data)
{
  int room;
  room = *((int*)data);
  if(room != ch->in_room)
  {
    send_to_char("&+LThe incorporeal figures dissolve into nothing...\n", ch);;
    act("&+LThe ghastly creatures fade into oblivion...", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  
  if(!number(0, 3))
  {
    act("$n&+L's ghastly figures glide about the area.",FALSE, ch, 0, 0, TO_ROOM);
    add_event(event_summon_ghasts, PULSE_VIOLENCE * 1, ch, 0, 0, 0, &room, sizeof(room));
    return;
  }
  
  act("$n&+L's summoned creatures look towards $m &+Lfor guidance, before &+rattacking&+L!",FALSE, ch, 0, 0, TO_ROOM);
  act("&+LYour creatures from beyond the grave look towards you for guidance.",FALSE, ch, 0, 0, TO_CHAR);  
  
  cast_as_damage_area(ch, spell_ghastly_touch, GET_LEVEL(ch), NULL,
      get_property("spell.area.minChance.summonGhasts", 90),
      get_property("spell.area.chanceStep.summonGhasts", 10));
}

void spell_aid_of_the_heavens(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int room;
  room = ch->in_room;

  
  act("&+WA holy beam of light begins to &+Lco&+wa&+Wle&+ws&+Lce &+Waround $n.", FALSE, ch, 0, 0, TO_ROOM);

  act("&+WYou call down a holy beam of light from the heavens.", FALSE, ch, 0, 0, TO_CHAR);
  
  zone_spellmessage(ch->in_room,
    "&+LYou see a bright light shining on the horizon.\n",
    "&+LThe air to the %s &+Rwarms &+Land the &+Yholy shine&n enlightens your senses.\n");

  add_event(event_aid_of_the_heavens, PULSE_VIOLENCE * 1, ch, 0, 0, 0, &room, sizeof(room));

  CharWait(ch,(int) 1 * PULSE_VIOLENCE);
}

void spell_summon_ghasts(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int room;
  room = ch->in_room;
  send_to_room("&+LDeathly incorporeal ghasts enter the realm of the living...\n", ch->in_room);
  zone_spellmessage(ch->in_room,
    "&+LThe air &+cchills &+Land the odor of &+rdeath &+Land &+ydecay &+Lassault your senses.\n",
    "&+LThe air to the %s &+cchills &+Land the odor of &+rdeath &+Land &+ydecay &+Lassaults your senses.\n");

  add_event(event_summon_ghasts, PULSE_VIOLENCE * 1, ch, 0, 0, 0, &room, sizeof(room));

  CharWait(ch,(int) 1 * PULSE_VIOLENCE);
}

void single_unholy_word(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int in_room, lev;
  
  struct damage_messages messages = {
    "You send $N reeling with your word of power.",
    "You are sent reeling by $n's unholy word.",
    "$n sends $N reeling with an unholy word.",
    "$N dies instantly from the power of your unholy word.",
    "You hear a word of power, and die instantly.",
    "$N hears $n's word of power, and nothing more.", 0
  };
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  if(ch->in_room != victim->in_room)
    return;
  
if(IS_NPC(victim) && IS_AFFECTED4(victim, AFF4_HELLFIRE))
	{
	  int rand1 = (number(1, 100));
	  if (rand1 > victim->base_stats.Pow)
	    {
		act
    	      ("&+LThe &+rHells&+L seem to heed the &+Ccall&+L of your &+rword&+W...",
     	      FALSE, ch, 0, victim, TO_CHAR);
		act
    	      ("&+LThe &+rHells&+L seem to heed the &+Ccall&+L of $n&+L's &+rword&+W...",
     	       TRUE, ch, 0, victim, TO_NOTVICT);
		act
    	      ("&+LA &+Ldark &+Ylight &+Ldecends upon $N&+L, returning its &+rfl&+Ram&+Yes &+Wof &+Rhell &+Lback into the abyss.",
     	      FALSE, ch, 0, victim, TO_CHAR);
		act
		("&+LA &+Ldark &+Ylight &+Ldecends upon $N&+W, returning its &+rfl&+Ram&+Yes &+Wof &+Rhell &+Lback into the abyss.",
     	  	TRUE, ch, 0, victim, TO_NOTVICT);
	      REMOVE_BIT(victim->specials.affected_by4, AFF4_HELLFIRE);
	    }
	}

  if(GET_ALIGNMENT(victim) < 0 && !(IS_PC(victim) && opposite_racewar(victim, ch)))
  {
    act("$N is not good enough to be affected!", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if((lev = GET_LEVEL(victim)) < (level / 3))
  {                           /* < 7-15 death */
    if(spell_damage(ch, victim, 1000, SPLDAM_HOLY, 0, &messages) !=
        DAM_NONEDEAD)
      return;
  }
  else
  {
    int dam = level * 3 + 20;
    dam = GET_RACE(victim) == RACE_GITHZERAI ? (int) (dam * 1.5) : dam;
    dam = dam * get_property("spell.area.damage.factor.unholyWord", 1.000);

    if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
    {
      dam = (int) (dam * 1.25);
    }

    if(spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &messages) == DAM_NONEDEAD)
    {
      if(lev < (level / 2 + 2))        /* 14-27 blind */
        spell_blindness(level, ch, 0, 0, victim, NULL);      /* no save */
      if(lev < (level / 2 - 3))        /* 9-22 para */
        spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
    }
    astral_banishment(ch, victim, BANISHMENT_UNHOLY_WORD, level);
  }
}

void spell_unholy_word(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }
  
  if(GET_LEVEL(ch) < MINLVLIMMORTAL)
  {
    if(GET_ALIGNMENT(ch) > 0 && IS_PC(ch))
    {
      act("Your word of power kills you instantly.", FALSE, ch, 0, victim, TO_CHAR);
      act("$n's word of power destroys $m instantly.", FALSE, ch, 0, victim, TO_NOTVICT);
      die(ch, ch);
      return;
    }
    else if(GET_ALIGNMENT(ch) > -350 && IS_PC(ch))
    {
      send_to_char
        ("You feel foolish for even trying this, &+Ygoodie&n!\n", ch);
      return;
    }
    else if(GET_ALIGNMENT(ch) > -900)
    {
      send_to_char("You are not evil enough to utter such unholiness!\n", ch);
      return;
    }
  }
  
  cast_as_damage_area(ch, single_unholy_word, level, victim,
                      get_property("spell.area.minChance.unholyWord", 60),
                      get_property("spell.area.chanceStep.unholyWord", 20));
}

void single_voice_of_creation(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int in_room, lev;
  
  struct damage_messages messages = {
    "&n&+bR&n&+cec&n&+Cit&n&+cin&n&+bg&n a &n&+Wholy &n&+Yprayer&n, you send $N reeling with &n&+Cr&n&+ci&n&+Cght&n&+ceou&n&+Cs &n&+Rpower&n.",
    "You are sent reeling by the &n&+Cr&n&+ci&n&+Cght&n&+ceou&n&+Cs &n&+Rpower&n of $n's &n&+Wholy &n&+Yprayer&n.",
    "$n &n&+br&n&+cec&n&+Ci&n&+cte&n&+bs&n a &n&+Wholy &n&+Yprayer&n and sends $N reeling with &n&+Cr&n&+ci&n&+Cght&n&+ceou&n&+Cs &n&+Rpower&n.",
    "$N dies instantly from the power of your holy word.",
    "You hear a word of power, and die instantly.",
    "$N hears $n's word of power, and nothing more.", 0
  };
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  if(ch->in_room != victim->in_room)
    return;
    
  if(GET_ALIGNMENT(victim) > 0 && !(IS_PC(victim) && opposite_racewar(victim, ch)))
  {
    act("$N is not evil enough to be affected!", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if((lev = GET_LEVEL(victim)) < (level / 3))
  {                           /* < 7-15 death */
    if(spell_damage(ch, victim, 1000, SPLDAM_HOLY, 0, &messages) !=
        DAM_NONEDEAD)
      return;
  }
  else
  {
    int dam = level * 3 + 20;
    dam = GET_RACE(victim) == RACE_GITHYANKI ? (int) (dam * 1.5) : dam;
    if(spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &messages) == DAM_NONEDEAD)
    {      
       if(lev < (level / 2 + 2))        /* 14-27 blind */
          spell_blindness(level, ch, 0, 0, victim, NULL);      /* no save */
       if(lev < (level / 2 - 3))        /* 9-22 para */
          spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
    }
    astral_banishment(ch, victim, BANISHMENT_HOLY_WORD, level);
  }
}

void single_holy_word(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int in_room, lev;
  
  struct damage_messages messages = {
    "You send $N reeling with your word of power.",
    "You are sent reeling by $n's holy word.",
    "$n sends $N reeling with a holy word.",
    "$N dies instantly from the power of your holy word.",
    "You hear a word of power, and die instantly.",
    "$N hears $n's word of power, and nothing more.", 0
  };
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  if(ch->in_room != victim->in_room)
    return;

  if(IS_NPC(victim) && IS_AFFECTED4(victim, AFF4_HELLFIRE))
	{
	  int rand1 = (number(1, 100));
	  if (rand1 > victim->base_stats.Pow)
	    {
		act
    	      ("&+WThe Heavens seem to heed the &+Ccall&+W of your &+Lword&+W...",
     	      FALSE, ch, 0, victim, TO_CHAR);
		act
    	      ("&+WThe Heavens seem to heed the &+Ccall&+W of $n&+W's &+Lword&+W...",
     	       TRUE, ch, 0, victim, TO_NOTVICT);
		act
    	      ("&+CA br&+Wig&+Cht &+Ylight &+Wdecends upon $N&+W, causing its &+rfl&+Ram&+Yes &+Wof &+Rhell &+Wto slowly fade away.",
     	      FALSE, ch, 0, victim, TO_CHAR);
		act
		("&+CA br&+Wig&+Cht &+Ylight &+Wdecends upon $N&+W, causing its &+rfl&+Ram&+Yes &+Wof &+Rhell &+Wto slowly fade away.",
     	  	TRUE, ch, 0, victim, TO_NOTVICT);
	      REMOVE_BIT(victim->specials.affected_by4, AFF4_HELLFIRE);
	    }
	}
    
  if(GET_ALIGNMENT(victim) > 0 && !(IS_PC(victim) && opposite_racewar(victim, ch)))
  {
    act("$N is not evil enough to be affected!", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if((lev = GET_LEVEL(victim)) < (level / 3))
  {                           /* < 7-15 death */
    if(spell_damage(ch, victim, 1000, SPLDAM_HOLY, 0, &messages) !=
        DAM_NONEDEAD)
      return;
  }
  else
  {
    int dam = level * 3 + 20;
    dam = GET_RACE(victim) == RACE_GITHYANKI ? (int) (dam * 1.5) : dam;
    if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
    {
      dam = (int) (dam * 1.25);
    }

    if(spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &messages) == DAM_NONEDEAD)
    {
       if(lev < (level / 2 + 2))        /* 14-27 blind */
          spell_blindness(level, ch, 0, 0, victim, NULL);      /* no save */
       if(lev < (level / 2 - 3))        /* 9-22 para */
          spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
    }
    astral_banishment(ch, victim, BANISHMENT_HOLY_WORD, level);
  }
}

void spell_voice_of_creation(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }
  
  if(GET_LEVEL(ch) < MINLVLIMMORTAL)
  {
    if(GET_ALIGNMENT(ch) < 0 && IS_PC(ch))
    {
      act("The voice of creation kills you instantly.", FALSE, ch, 0, victim, TO_CHAR);
      act("$n hears the voice of creation and it kills $m instantly.", FALSE, ch, 0, victim, TO_NOTVICT);
      die(ch, ch);
      return;
    }
    else if(GET_ALIGNMENT(ch) < 350 && IS_PC(ch))
    {
      send_to_char
        ("You are far too &+revil&n to say anything as &+Wholy&n as this!\n",
         ch);
      //act(messages.death_victim, FALSE, ch, 0, victim, TO_CHAR);
      //act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
      //die(ch, ch);
      return;
    }
    else if(GET_ALIGNMENT(ch) < 900)
    {
      send_to_char
        ("You are not good enough to speak a word of such holiness!\n", ch);
    return;
    }
  }

  cast_as_damage_area(ch, single_voice_of_creation, level, victim,
                      get_property("spell.area.minChance.holyWord", 60),
                      get_property("spell.area.chanceStep.holyWord", 20));
}

void spell_holy_word(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }

  if(GET_LEVEL(ch) < MINLVLIMMORTAL)
  {
    if(GET_ALIGNMENT(ch) < 0 && IS_PC(ch))
    {
      act("Your word of power kills you instantly.", FALSE, ch, 0, victim, TO_CHAR);
      act("$n's word of power destroys $m instantly.", FALSE, ch, 0, victim, TO_NOTVICT);
      die(ch, ch);
      return;
    }
    else if(GET_ALIGNMENT(ch) < 350 && IS_PC(ch))
    {
      send_to_char
        ("You are far too &+revil&n to say anything as &+Wholy&n as this!\n",
         ch);
      //act(messages.death_victim, FALSE, ch, 0, victim, TO_CHAR);
      //act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
      //die(ch, ch);
      return;
    }
    else if(GET_ALIGNMENT(ch) < 900)
    {
      send_to_char
        ("You are not good enough to speak a word of such holiness!\n", ch);
    return;
    }
  }
       cast_as_damage_area(ch, single_holy_word, level, victim,
                      get_property("spell.area.minChance.holyWord", 60),
                      get_property("spell.area.chanceStep.holyWord", 20));

}

void spell_solar_flare(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "You send a burst of &+Rpure &+rf&+Ri&+rr&+Re&n towards&n $N &+Rburning $S flesh.",
    "A burst of &+Rpure &+rf&+Ri&+rr&+Re&n slams into your chest burning the skin of your body.",
    "A burst of &+Rpure &+rf&+Ri&+rr&+Re&n leaps from&n $n &+Rburning the flesh of&n $N.",
    "You send a burst of &+Rpure &+rf&+Ri&+rr&+Re&n towards&n $N &+Rturning $M into a pile of ashes.",
    "A burst of &+Rpure &+rf&+Ri&+rr&+Re&n slamming into your chest is the last thing you see..",
    "A burst of &+Rpure &+rf&+Ri&+rr&+Re&n leaps from&n $n &+Rturning&n $N &+Rinto a pile of ashes.",
      0
  };

  dam = 6 * level + number(1, 25);

  if(spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages))
    return;

  if(!number(0, 1) && !NewSaves(victim, SAVING_SPELL, 0))
    blind(ch, victim, 60 * WAIT_SEC);

  if(!number(0, 6))
    spell_immolate(level, ch, NULL, 0, victim, NULL);
}


void spell_sunray(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "&+WYou unleash &+Ylight&+W in a focused, searing &+Yray&+W at&n $N!",
    "$n&+W unleashes &+Ylight&+W in a focused, searing &+Yray&+W at you!",
    "$n&+W unleashes &+Ylight&+W in a focused, searing &+Yray&+W at&n $N!",
    "$N&+Y is struck by your ray of sunlight, and is destroyed instantly!",
    "&+YYou see a fatally blinding light!",
    "$N&+Y is struck by a blinding light from&n $n, &+Yand is utterly destroyed!",
      0
  };
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(victim) ||
     !IS_ALIVE(victim) ||
     ch->in_room != victim->in_room)
        return;
  
 
// A little more than iceball and does less damage when not outside.
// However, has a chance to blind victim for a while.
  int dam = dice((int)(level * 3), 6) - number(0, 40);
  
  if(IS_AFFECTED(victim, AFF_BLIND))
        dam = (int)(dam * 0.85);
  else if(!IS_OUTSIDE(victim->in_room))
        dam = (int)(dam * 0.95);

  int mod = BOUNDED(0, (GET_LEVEL(ch) - GET_LEVEL(victim)), 20);
  if(!NewSaves(victim, SAVING_SPELL, mod))
  {
      dam = (int)(dam * 1.30);
  }
    
  if(!NewSaves(victim, SAVING_SPELL, (int)(mod / 3)) &&
     !IS_BLIND(victim))
        blind(ch, victim, number((int)(level / 3), (int)(level / 2)) * WAIT_SEC);
 
  spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
}



void spell_silence(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  int save;

  if( GET_LEVEL(victim) > 57 )
  {
    if( ch != victim )
    {
      save = -15;
    }
    // For Imms testing..
    else
    {
      save += 15;
    }
  }
  else
  {
    save = victim->specials.apply_saving_throw[SAVING_SPELL];
  }

  int percent = BOUNDED( 0, (int) ((GET_C_WIS(ch) / 2) + (save * 2) + (GET_LEVEL(ch) - GET_LEVEL(victim))
    - GET_C_WIS(victim) / 4), 100);

//  debug("Silence percent is: %d", percent);

  if( (IS_TRUSTED(victim) && victim != ch) || (IS_GREATER_RACE(victim)) || (IS_ELITE(victim)) || (percent < 10) )
  {
    return;
  }

  if(IS_AFFECTED2(victim, AFF2_SILENCED))
  {
    send_to_char("They are already quiet!\n", ch);
    return;
  }

  if(resists_spell(ch, victim))
    return;

  struct affected_type af;
  bzero(&af, sizeof(af));

  if(percent > 90)
  {
    send_to_char("You suddenly feel completely quiet!\n", victim);
    act("$n is suddenly at a great loss for words.", TRUE, victim, 0, 0, TO_ROOM);

    af.type = SPELL_SILENCE;
    af.duration = 40;
    af.flags = AFFTYPE_SHORT;
    af.bitvector2 = AFF2_SILENCED;
    affect_to_char(victim, &af);
  }
  else if(percent > 70)
  {
    send_to_char("You suddenly feel much quieter!\n", victim);
    act("$n suddenly grows silent.", TRUE, victim, 0, 0, TO_ROOM);

    af.type = SPELL_SILENCE;
    af.duration = 30;
    af.flags = AFFTYPE_SHORT;
    af.bitvector2 = AFF2_SILENCED;
    affect_to_char(victim, &af);
  }
  else if(percent > 40)
  {
    send_to_char("You suddenly feel quiet!\n", victim);
    act("$n is suddenly at a loss for words.", TRUE, victim, 0, 0, TO_ROOM);

    af.type = SPELL_SILENCE;
    af.flags = AFFTYPE_SHORT;
    af.duration = 20;
    af.bitvector2 = AFF2_SILENCED;
    affect_to_char(victim, &af);
  }
  else if(percent > 5)
  {
    send_to_char("You feel your voice soften slightly!\n", victim);
    act("$n is suddenly at a slight loss for words.", TRUE, victim, 0, 0, TO_ROOM);

    af.type = SPELL_SILENCE;
    af.flags = AFFTYPE_SHORT;
    af.duration = 30;
    af.bitvector2 = AFF2_SILENCED;
    affect_to_char(victim, &af);
  }
}

void spell_feeblemind(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  char Gbuffer_1[24];
  int i = 0, j = 0, k = 0, percentch, percentvictim, save = 0, percent, intmod, wismod;

  if(ch) // Just making sure.
  {
    if(!IS_ALIVE(ch))
    {
      send_to_char("Take a look around. You're dead...\r\n", ch);
      return;
    }
    if(!victim ||
      !IS_ALIVE(victim))
    {
      act("Who/what do you wish to feeb?", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    if(IS_NPC(ch))
    return; //drannak - disabling while i debug

    if(affected_by_spell(victim, SPELL_FEEBLEMIND))
    {
      act("$N is already dumb!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
    if(IS_GREATER_RACE(victim) ||
      IS_ELITE(victim) ||
      IS_TRUSTED(victim))
    {
      act("$N is immune to your pathetic spell.", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
    if((GET_RACE(victim) == RACE_UNDEAD) ||
      (GET_RACE(victim) == RACE_PLANT))
    {
      act("$N's mind is too foreign for you to alter!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
    if(resists_spell(ch, victim))
    {
      return;
    }

    save = victim->specials.apply_saving_throw[SAVING_SPELL];
    
    if(save < 0)
    {
      save = (int) (save * 1.5);
    }
    if(IS_PC(ch) || 
      (IS_NPC(ch) &&
      GET_C_WIS(ch) >= 75))
    {
      percentch = (int) ((GET_C_WIS(ch) + GET_LEVEL(ch)) *
        get_property("spell.richness.feeblemind", 1.5));
    }
    else
    {
      percentch = (int) ((75 + GET_LEVEL(ch)) *
        get_property("spell.richness.feeblemind", 1.5));
    }

    percentvictim = GET_C_WIS(victim) + GET_LEVEL(victim) - save;
    percent = percentch + number(-15, 15) - percentvictim;

    if(percent < 30)
    {
      act("Your spell has no effect!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
 
    send_to_char("You feel &+creally, &+Creally &+cdumb!&n\n", victim);
    act("$n looks &+c_really_ dumb.&n", TRUE, victim, 0, 0, TO_ROOM);

    // At 100 int and level 50 caster, int drop approx 43.
    intmod = (int) ((GET_C_INT(victim) / 3) + (level / 5) + number(-15, 5));
    wismod = (int) ((GET_C_WIS(victim) / 3) + (level / 5) + number(-15, 5));

    struct affected_type af;
    bzero(&af, sizeof(af));
    af.type = SPELL_FEEBLEMIND;
    if(percent < 70)
    {
      af.flags = AFFTYPE_SHORT;
      af.duration = 80;
    }  
    else
    {
      af.duration = 1;
    }
    af.modifier = (-1 * intmod);
    af.location = APPLY_INT;
    affect_to_char(victim, &af);

    af.modifier = (-1 * wismod);
    af.location = APPLY_WIS;
    affect_to_char(victim, &af);

    // for i greater than 10, returns 1, 2, 3, ...
    // tweak divisor value higher for fewer spell losses.
    i = (int) (percent / get_property("spell.richness.feeblemind.spellLoss.divisor", 10));

    if(ch &&
      victim &&
      IS_ALIVE(ch) &&
      IS_ALIVE(victim) &&
      (i >= 1))
    {
      stop_memorizing(victim); // Need to stop otherwise possible crash.

      // Is it a slot caster?      
      if(USES_SPELL_SLOTS(victim))
      {
        if(!victim)
        {
          return;
        }
        for (BOUNDED(1, (int) (j = get_max_circle(victim)), 12); j >= 1; j--)
        {
          for (k = victim->specials.undead_spell_slots[j]; k >= 1; k--)
          {
            if(i >= 1)
            {
              victim->specials.undead_spell_slots[j]--;
              i--;
              continue;
            }
            else
            {
              break;
            }
          }
        }
      }
      else // We guessing it's a regular caster.
      {
        if(!victim || !IS_PC(victim))
        {
          return;
        }
        struct affected_type *af2, *next_af2;
        for(af2 = victim->affected; af2; af2 = next_af2)
        {
          if(!af2)
          break;

          next_af2 = af2->next;
          if(af2->type == TAG_MEMORIZE)
          {
            if(i >= 1)
            {
              affect_remove(victim, af2);
              i--;
              sprintf(Gbuffer_1, "You forget %s!\n", skills[af2->modifier].name);
              send_to_char(Gbuffer_1, victim);
              continue;
            }
            else
            {
              break;
            }
          }
        }
      }
    }
  }
}
// end spell_feeblemind

void spell_pword_blind(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  int percent = 0, save;

  if(!(ch) || !(victim) || !IS_ALIVE(ch) || !IS_ALIVE(victim))
  {
    return;
  }

  if(IS_BLIND(victim))
  {
    send_to_char("Your target is already blind!\r\n", ch);
    return;
  }

  if( has_innate(victim, INNATE_EYELESS) || IS_TRUSTED(victim) )
    return;

  int mindpower = ((GET_C_POW(ch) - GET_C_POW(victim)) / 2);
  //save = victim->specials.apply_saving_throw[SAVING_SPELL];
  save = NewSaves(victim, SAVING_SPELL, mindpower);

  debug("PWB: (%s) saving throw is (%d).", J_NAME(victim), save);

  if(save)//NewSaves(victim, SAVING_SPELL, number(0, mindpower)))
  {
    send_to_char("Your victim has saved against your spell!\r\n", ch);
    send_to_char("You have saved against your attacker's spell!\r\n", victim);
    return;
  }

/*
  if(save < 0)
    save = (int) (save * 1.5);

  percent = BOUNDED( 0, (GET_C_POW(ch) - GET_C_POW(victim) + save + (GET_LEVEL(ch) - GET_LEVEL(victim))), 100);
*/

  percent = BOUNDED( 0, (GET_C_POW(ch) - GET_C_POW(victim) + (GET_LEVEL(ch) - GET_LEVEL(victim))), 100);

  if(IS_PC_PET(ch))
    percent = (int)(percent * 0.5);

  debug("PWB: (%s) casted pwb at (%s) percent (%d) mindpower (%d).",
    GET_NAME(ch), GET_NAME(victim), percent, mindpower);
/*
  if(percent < 10)
  {
    send_to_char("They are too powerful to blind this way!\n", ch);
    return;
  }

  if(resists_spell(ch, victim))
    return;

  if(percent > 60)
  {
    blind(ch, victim, number(300, 600) * WAIT_SEC);
    act("$N gropes around, blinded after hearing $n's powerful word!",
      FALSE, ch, 0, victim, TO_NOTVICT);
    act("&+rSuddenly, the world goes &+Lblack!",
      FALSE, ch, 0, victim, TO_VICT);
    act("$N won't be seeing much in the near future...",
      TRUE, ch, 0, victim, TO_CHAR);
  }

  else if(percent > 40)
    blind(ch, victim, number(10, 15) * WAIT_SEC);

  else
    blind(ch, victim, number(5, 10) * WAIT_SEC);
*/
  act("$N gropes around, blinded after hearing $n's powerful word!",
    FALSE, ch, 0, victim, TO_NOTVICT);
  act("&+rSuddenly, the world goes &+Lblack!",
    FALSE, ch, 0, victim, TO_VICT);
  act("$N won't be seeing much in the near future...",
    TRUE, ch, 0, victim, TO_CHAR);
  // At least 2 sec, at most mindpower/2 secs.
  mindpower = MAX( WAIT_SEC, mindpower );
  blind( ch, victim, dice(2*WAIT_SEC, mindpower/WAIT_SEC) );

}

/* rocking spell now, stun is extremely unpleasant */

void spell_pword_stun(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  //int percent = 50;
  int percent;

  if(!(ch) || !(victim) || !IS_ALIVE(victim) || GET_HIT(victim) < 1
    || !IS_ALIVE(ch) || IS_TRUSTED(victim) || IS_ZOMBIE(victim)
    || GET_RACE(victim) == RACE_GOLEM || GET_RACE(victim) == RACE_PLANT
    || GET_RACE(victim) == RACE_CONSTRUCT || IS_GREATER_RACE(victim))
  {
    send_to_char("&+rYour target is immune!\r\n", ch);
    return;
  }

  if(resists_spell(ch, victim))
    return;

  if(IS_STUNNED(victim))
  {
    send_to_char("Your target is already stunned!\r\n", ch);
    return;
  }

  percent = (int) (GET_LEVEL(ch) - GET_LEVEL(victim));

  percent += (int) (GET_C_POW(ch) - GET_C_POW(victim)) * .75;

  if(IS_PC_PET(victim))
    percent *= 2;

  if(affected_by_spell(ch, SPELL_FEEBLEMIND))
    percent /= 3;

  if(affected_by_spell(victim, SPELL_FEEBLEMIND))
    percent *= 1.5;

  if(NewSaves(ch, SAVING_SPELL, number(-3, 3)))
  {
    percent = (int) percent * .75;
  }
  else
  {
    percent = (int) percent * 1.15;
  }

  if(percent < 30)
  {
    send_to_char("&+rYour spell has no effect!\n", ch);
    return;
  }

  if(percent > 90)
  {
    act("$N&n &+yis &+rstunned &+yinto complete submission by the power of&n $n's &+yword!&n", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n's&n &+yword of power sends you reeling in utter confusion and pain!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$N&n &+yis &+rstunned &+yinto complete submission by your powerful word!&n", TRUE, ch, 0, victim, TO_CHAR);
    Stun(victim, ch, (number(3, 4) * PULSE_VIOLENCE), FALSE);
  }
  else if(percent > 70)
  {
    act("$N&n &+yis heavily &+rstunned &+yby the power of&n $n's &+yyword!&n", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n's&n &+yword of power sends you reeling!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$N&n &+yis sent reeling by your very powerful word!&n", TRUE, ch, 0, victim, TO_CHAR);
    Stun(victim, ch, (number(2, 3) * PULSE_VIOLENCE), FALSE);
  }
  else if(percent > 50)
  {
    act("$N&n &+yis &+rstunned &+yby the power of&n $n's &+yword!&n", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n's&n &+yword of power confuses and disorients you!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$N&n &+yis sent reeling by your powerful word!&n", TRUE, ch, 0, victim, TO_CHAR);
    Stun(victim, ch, (number(1, 2) * PULSE_VIOLENCE), FALSE);
  }
  else
  {
    act("$N&n &+yis &+wdazed &+yby the power of&n $n's &+yword!&n", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n's&n &+yword of power &+wdazes &+yyou!&n", FALSE, ch, 0, victim, TO_VICT);
    act("$N&n &+yis &+wdazed &+yby your powerful word!&n", TRUE, ch, 0, victim, TO_CHAR);
    Stun(victim, ch, PULSE_VIOLENCE, FALSE);
  }
}

void spell_dispel_magic(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type *af, *next_af_dude;
  int      mod, success = 0, nosave = 0;
  P_obj    temp_wall, next_obj, obj2;
  P_event  e1 = NULL, e2;
  P_char   orig;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  /* victim target... */
  if( victim )
  {
    /*
     * no save when cast or on consenting target
     */

    if(ch == victim && level < 57 && !IS_TRUSTED(ch))
    {
      send_to_char("You dispel the very spell you are casting!\n", ch);
      return;
    }

    if( ch == victim || is_linked_to(ch, victim, LNK_CONSENT) || IS_TRUSTED(ch) )
    {
      nosave = 1;
    }

    mod = GET_LEVEL(ch) - GET_LEVEL(victim);
    if( IS_NPC(ch) && IS_PC(victim) && mod > 0 )
    {
      mod /= 3;
    }

    act("$n tries to dispel your magic!", FALSE, ch, 0, victim, TO_VICT);
    act("You try to dispel $N's magic.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n tries to dispel $N's magic!", FALSE, ch, 0, victim, TO_NOTVICT);

    for (af = victim->affected; af; af = next_af_dude)
    {
      next_af_dude = af->next;

      // Skip over the multiple affect spells.
      while( next_af_dude && next_af_dude->type == af->type )
      {
        next_af_dude = next_af_dude->next;
      }

      if( !IS_SET(af->flags, AFFTYPE_NODISPEL) && (af->type > 0) )
      {
        if( nosave || !NewSaves(victim, SAVING_SPELL, (IS_ELITE(ch) ? mod + 5 : mod)) )
        {
          if( !nosave && resists_spell(ch, victim) )
          {
            return;
          }

          success = 1;
          wear_off_message(victim, af);
          if((af->type == SPELL_CALL_OF_THE_WILD) && IS_MORPH(victim))
          {
            orig = victim->only.npc->orig_char;

            act("$n suddenly changes shape, reforming into $N.", FALSE,
                victim, NULL, orig, TO_NOTVICT);
            send_to_char("You suddenly feel yourself going back to normal.\n",
                         victim);

            send_to_char("and you succeed!\n", ch);
            act("and succeeds!", FALSE, ch, 0, victim, TO_NOTVICT);

            un_morph(victim);
            return;
          }
          affect_from_char(victim, af->type);
        }
      }
    }

    /*
     * special check to even things out with mobs, stone and globe can now
     * be dispeled even if there is no affected structure. -JAB
     */


    if(IS_NPC(victim) &&
       IS_AFFECTED(victim, AFF_MINOR_GLOBE) &&
       !affected_by_spell(victim, SPELL_MINOR_GLOBE) &&
       (nosave || !NewSaves(victim, SAVING_SPELL, mod)))
    {
      success = 1;
      REMOVE_BIT(victim->specials.affected_by, AFF_MINOR_GLOBE);
    }
    
    if(IS_AFFECTED2(victim, AFF2_GLOBE) &&
        !affected_by_spell(victim, SPELL_GLOBE) &&
        (nosave || !NewSaves(victim, SAVING_SPELL, mod)))
    {
      success = 1;
      REMOVE_BIT(victim->specials.affected_by2, AFF2_GLOBE);
    }
    
    if(!success)
    {
      send_to_char("&+Land you fail miserably...\n", ch);
      act("&+Land fails miserably...&n", FALSE, ch, 0, victim, TO_NOTVICT);
    }
    else
    {
      send_to_char("&+Yand you have some &+Gsuccess!\r\n", ch);
      act("&+Yand $e has some &+Gsuccess!&n", FALSE, ch, 0, victim, TO_NOTVICT);
    }

    if(!nosave &&
       (IS_AFFECTED(ch, AFF_INVISIBLE) ||
       IS_AFFECTED2(ch, AFF2_MINOR_INVIS)))
          appear(ch);

    if(!nosave && IS_NPC(victim) && CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
        MobStartFight(victim, ch);
    }
    balance_affects(victim);
    return;
  }
  /* okay.. must be an object target! */

  if(obj_index[obj->R_num].func.obj &&
      (*obj_index[obj->R_num].func.obj) (obj, ch, CMD_DISPEL, NULL))
    return;

  /* second deal with "special" objects (conjurerer wall spells) */
  if(obj->R_num == real_object(VOBJ_WALLS))
  {
    int dispelDmg = ( (obj->value[3] == WALL_OUTPOST) ? 1 : number(level/4, level) );
    
    if((number(0, 5) > (obj->value[4] - level)) ||
        IS_TRUSTED(ch) ||
        (IS_PC(ch) && obj->value[5] == GET_PID(ch)) ||
        (IS_NPC(ch) && obj->value[5] == GET_RNUM(ch) ) ||
        (dispelDmg >= obj->value[2]))
    {
      /* clear the other side */
      if(EXIT(ch, obj->value[1]))
      {
        for (temp_wall = world[EXIT(ch, obj->value[1])->to_room].contents;
             temp_wall; temp_wall = next_obj)
        {
          next_obj = temp_wall->next_content;
          if((temp_wall->R_num == obj->R_num) &&
              (temp_wall->value[1] == rev_dir[obj->value[1]]))
          {
            Decay(temp_wall);
          }
        }
      }
      Decay(obj);
    }
    else
      obj->value[2] -= dispelDmg;
    return;
  }

  /* casting is on a "normal" object.. only has an effect if the
     object has a MAGIC flag! */
  if(IS_SET(obj->extra2_flags, ITEM2_MAGIC))
  {
  
    if(IS_ARTIFACT(obj))
    {
      send_to_char("&+GIt is impossible to dispel an artifact!\r\n", ch);
      return;
    }
    
    /* 5% chance of totally destroying the object <Cackle> */
    if(!number(0, 4) && !IS_TRUSTED(ch))
    {
      send_to_char("&+YUh oh!&N\n", ch);

      Decay(obj);
    }
    else if((number(25, 59) < level) || IS_TRUSTED(ch))
    {
      act("&+bThe magic aura around&n $p&+b fades.",
          TRUE, ch, obj, 0, TO_CHAR);
      REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);
      obj->affected[0].location = APPLY_NONE;
      obj->affected[0].modifier = 0;
      obj->affected[1].location = APPLY_NONE;
      obj->affected[1].modifier = 0;
    }
  }
}

bool isCarved(P_obj corpse)
{

  int      i;

  for (i = 0; i < numCarvables; i++)
    if(corpse->value[1] & carve_part_flag[i])
      return TRUE;

  return FALSE;

}

void spell_resurrect(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool     loss_flag = FALSE;
  int      chance, l, found, clevel, ss_roll;
  long     resu_exp;
  P_obj    obj_in_corpse, next_obj, t_obj, money;
  struct affected_type *af, *next_af;
  P_char   t_ch;

  if( !IS_ALIVE(ch) || !obj )
  {
    return;
  }

  if( obj->type != ITEM_CORPSE )
  {
    send_to_char("You can only resurrect corpses!\n", ch);
    return;
  }

  if( !GET_CLASS(ch, CLASS_CLERIC) || level < 56 )
  {
    CharWait(ch, 25*WAIT_SEC);
  }
  else
  {
    CharWait(ch, 10*WAIT_SEC);
  }

  if( IS_NPC(ch) && IS_PC_PET(ch) )
  {
    return;
  }

  if( IS_SET(obj->value[1], NPC_CORPSE) )
  {
    if(!obj->value[3] || !IS_TRUSTED(ch))
    {
      send_to_char("You can't resurrect this corpse!\n", ch);
      return;
    }
    t_ch = read_mobile(obj->value[3], VIRTUAL);
    if( !t_ch )
    {
      logit(LOG_DEBUG, "spell_resurrect(): mob %d not loadable",
            obj->value[3]);
      send_to_char("You can't resurrect this corpse!\n", ch);
      return;
    }
    /*
     * res'ed mobs will mark as their birth the room they are res'ed
     * in -Neb
     */
    GET_BIRTHPLACE(t_ch) = world[ch->in_room].number;
    if( !IS_SET(t_ch->specials.act, ACT_MEMORY) )
    {
      clearMemory(t_ch);
    }
  }
  else
  {
    // Res a player
    found = 0;

    for (t_ch = character_list; t_ch; t_ch = t_ch->next)
    {
      if( t_ch && IS_PC(t_ch) && !str_cmp(t_ch->player.name, obj->action_description) )
      {
        if( t_ch == ch )
        {
          send_to_char("You can't resurrect your own corpse!\n", ch);
          return;
        }
        if( (obj->value[2] < 0) && !IS_TRUSTED(ch) )
        {
          send_to_char("This corpse is not resurrectable.\n", ch);
          return;
        }
        if( !is_linked_to(ch, t_ch, LNK_CONSENT) && !(IS_TRUSTED(ch) || IS_NPC(ch)) )
        {
          /* In AD&D, when a person dies, its not just a matter of losing
             xp, but a complete end to the char.  Things work differently
             in the mud, and often a res is a Bad Thing.  Therefore to stop
             people from PK'ing, res'ing, PK'ing, etc, just to fuck up the
             persons xp and stats, consent will be REQUIRED to res */
          send_to_char("That person must consent to you first.\n", ch);
          return;
        }
        found = 1;
        break;
      }
    }
    if( !found )
    {
      send_to_char("You can't find a soul to reunite with this corpse!\n", ch);
      return;
    }

    if( GET_PID(t_ch) != obj->value[3] )
    {
      if( IS_TRUSTED(ch) )
      {
        send_to_char("Different IDs, but you are godly..  enjoy.\n", ch);
      }
      else
      {
        send_to_char("Similar, but not the same!  You fail.\n", ch);
        return;
      }
    }

    if( isCarved(obj) )
    {
      if( IS_TRUSTED(ch) )
      {
        send_to_char("You regenerate the carved body parts.\n", ch);
      }
      else
      {
        send_to_char("The soul you found rejects this mutilated corpse.\n",  ch);
        return;
      }
    }

    /*
     * new bit, resurrectee must make a CON system shock save, if
     * they: make the save - spell works, they regain 80% of lost exp.
     * fail the save by < 50 - spell works, but they lose a semi-random
     * number of permanent stat points, usually from Con, but other
     * losses are possible. fail the spell by > 50 - spell fails,and
     * THIS corpse can never be resurrected (except by a god).  (No stat
     * losses, but they don't get any exp back either.)
     *
     * save maxes at 97% (for players), so it can fail on ANYONE, chance
     * has to fall below 50% before the spell can completely fail though
     * (Con (REAL) of < 40), stat losses will be fairly common though.
     * JAB
     */

//    ss_save = con_app[STAT_INDEX(GET_C_CON(t_ch))].shock;
    chance = 90 - (4 * (56 - GET_LEVEL(ch)));   /* 90% success at 56, 50% at 46 */
    ss_roll = number(1, 100);

    if( !IS_TRUSTED(ch) )
    {
      // if((ss_save + (100 - ss_save) / 2) < ss_roll) {
      if( ss_roll > chance )
      {
        // Complete failure, corpse is unressable.
        logit(LOG_DEATH, "%s ressed %s:  Failed roll: %3d Chance: %3d Level: %2d",
          GET_NAME(ch), GET_NAME(t_ch), ss_roll, chance, level);

#if 0
        act("The $q seems to shiver, and a fitful glow briefly surrounds it.",
            FALSE, ch, obj, 0, TO_ROOM);
        act("The $q seems to shiver, and a fitful glow briefly surrounds it.",
            FALSE, ch, obj, 0, TO_CHAR);
        act("You feel a brief moment of double vision, which passes quickly.\nYou feel a pang of loss.",
           FALSE, t_ch, 0, 0, TO_CHAR);
        GET_VITALITY(ch) = MIN(0, GET_VITALITY(ch));
        StartRegen(ch, EVENT_MOVE_REGEN);
        obj->value[2] = -obj->value[2]; /*
                                         * reverse level
                                         * as a flag
                                         */
        return;
      }
      else if(ss_save < ss_roll)
      {
        /*
         * partial success, stat loss
         */
        logit(LOG_DEATH, "%s res %s: Con: %2d(%3d)  Save: %2d  Roll %2d",
              GET_NAME(ch), GET_NAME(t_ch), t_ch->base_stats.Con,
              stat_factor[GET_RACE(t_ch)].Con * t_ch->base_stats.Con / 100,
              ss_save, ss_roll);
        loss_flag = TRUE;
        ss_roll -= ss_save;
        t_ch->base_stats.Con--;
        t_ch->base_stats.Con = MAX(1, t_ch->base_stats.Con);
        logit(LOG_DEATH, "%s lost a Con point from resurrect",
              GET_NAME(t_ch));
        ss_roll -= number(10, 15);
        while (ss_roll > 0)
        {
          ss_roll -= number(9, 15);
          switch (number(1, 9))
          {
          case 1:
          case 7:
          case 8:
          case 9:
            t_ch->base_stats.Con--;
            t_ch->base_stats.Con = MAX(1, t_ch->base_stats.Con);
            logit(LOG_DEATH, "%s lost a Con point from resurrect",
                  GET_NAME(t_ch));
            break;
          case 2:
            t_ch->base_stats.Str--;
            t_ch->base_stats.Str = MAX(1, t_ch->base_stats.Str);
            logit(LOG_DEATH, "%s lost a Str point from resurrect",
                  GET_NAME(t_ch));
            break;
          case 3:
            t_ch->base_stats.Dex--;
            t_ch->base_stats.Dex = MAX(1, t_ch->base_stats.Dex);
            logit(LOG_DEATH, "%s lost a Dex point from resurrect",
                  GET_NAME(t_ch));
            break;
          case 4:
            t_ch->base_stats.Agi--;
            t_ch->base_stats.Agi = MAX(1, t_ch->base_stats.Agi);
            logit(LOG_DEATH, "%s lost a Agi point from resurrect",
                  GET_NAME(t_ch));
            break;
          case 5:
            t_ch->base_stats.Int--;
            t_ch->base_stats.Int = MAX(1, t_ch->base_stats.Int);
            logit(LOG_DEATH, "%s lost a Int point from resurrect",
                  GET_NAME(t_ch));
            break;
          case 6:
            t_ch->base_stats.Wis--;
            t_ch->base_stats.Wis = MAX(1, t_ch->base_stats.Wis);
            logit(LOG_DEATH, "%s lost a Wis point from resurrect",
                  GET_NAME(t_ch));
            break;
          }
        }
#endif
      }
    }

    if( IS_PC(t_ch) && IS_RIDING(t_ch) )
    {
      stop_riding(t_ch);
    }

    act("$n &+rhowls &+win pain as $s body crumbles to &+Ldust&+w.&n",
      FALSE, t_ch, 0, 0, TO_ROOM);
    act("&+wYou &+rhowl &+win pain as your soul leaves your body to return to your &+Lcorpse&+w.&n",
      FALSE, t_ch, 0, 0, TO_CHAR);

    t_ch->only.pc->pc_timer[PC_TIMER_HEAVEN] = 0;

    if( t_ch->specials.fighting )
    {
      stop_fighting(t_ch);
    }
    if( IS_DESTROYING(t_ch) )
    {
      stop_destroying(t_ch);
    }
    StopAllAttackers(t_ch);

    for( af = t_ch->affected; af; af = next_af )
    {
      next_af = af->next;
      if( !(af->flags & AFFTYPE_NODISPEL) )
      {
        affect_remove(t_ch, af);
      }
    }

    if( GET_MONEY(t_ch) > 0 )
    {
      /*
       * make a 'pile of coins' object to hold victim's cash
       */

      money = create_money(GET_COPPER(t_ch), GET_SILVER(t_ch), GET_GOLD(t_ch), GET_PLATINUM(t_ch));
      SUB_MONEY(t_ch, GET_MONEY(t_ch), 0);
      obj_to_room(money, t_ch->in_room);
    }
    for( t_obj = t_ch->carrying; t_obj != NULL; t_obj = next_obj )
    {
      next_obj = t_obj->next_content;
      if(IS_SET(obj->extra_flags, ITEM_TRANSIENT))
      {
        extract_obj(t_obj, TRUE); // Transient artis?
        t_obj = NULL;
      }
      else
      {
        obj_from_char(t_obj);
        obj_to_room(t_obj, t_ch->in_room);
      }
    }

    /*
     * clear equipment_list
     */
    for( l = 0; l < MAX_WEAR; l++ )
    {
      if( t_ch->equipment[l] )
      {
        t_obj = unequip_char(t_ch, l);
        if( IS_SET(t_obj->extra_flags, ITEM_TRANSIENT) )
        {
          extract_obj(t_obj, TRUE); // Transient artis?
          t_obj = NULL;
        }
        else
        {
          obj_to_room(t_obj, t_ch->in_room);
        }
      }
    }
    char_from_room(t_ch);
  }
  char_to_room(t_ch, ch->in_room, -2);

  /*
   * move objects in corpse to victim's inventory
   */
  for( obj_in_corpse = obj->contains; obj_in_corpse; obj_in_corpse = next_obj )
  {
    next_obj = obj_in_corpse->next_content;
    obj_from_obj(obj_in_corpse);
    if( obj_in_corpse->type == ITEM_MONEY )
    {
      GET_COPPER(t_ch) += obj_in_corpse->value[0];
      GET_SILVER(t_ch) += obj_in_corpse->value[1];
      GET_GOLD(t_ch) += obj_in_corpse->value[2];
      GET_PLATINUM(t_ch) += obj_in_corpse->value[3];
      extract_obj(obj_in_corpse);
      obj_in_corpse = NULL;
    }
    else
    {
      obj_to_char(obj_in_corpse, t_ch);
    }
  }

  if( IS_EVIL(ch) )
  {
    act("&+wAn aura of intense &+Lbitter darkness &+wsurrounds&n $n &+wfor a moment.&n",
      TRUE, t_ch, 0, 0, TO_ROOM);
  }
  else
  {
    act("&+wAn aura of intensely &+Ybright &+Wlight &+wsurrounds&n $n &+wfor a moment.&n",
      TRUE, t_ch, 0, 0, TO_ROOM);
  }
  act("$n &+wcomes to &+Wlife &+wagain! Taking a &+Cdeep breath&+W, $n opens $s eyes!&n",
    TRUE, t_ch, 0, 0, TO_ROOM);
  act("&+wYou are &+cextremely tired &+wafter resurrecting $N.", TRUE, ch, 0, t_ch, TO_CHAR);

  if( loss_flag )
  {
    act("You feel drained!", TRUE, t_ch, 0, 0, TO_CHAR);
  }
  act("&+wYou are &+cextremely tired &+wafter being resurrected!&n", TRUE, t_ch, 0, 0, TO_CHAR);

// play_sound(SOUND_RESSURECTION, NULL, t_ch->in_room, TO_ROOM);

  GET_VITALITY(ch) = MIN(0, GET_VITALITY(ch));
  StartRegen(ch, EVENT_MOVE_REGEN);

  /*
   * restore lost exp from death
   */
  clevel = obj->value[2];


  if( IS_PC(t_ch) && !IS_TRUSTED(t_ch) )
  {
    resu_exp = obj->value[4];
    if( resu_exp == 0 )
    {
      //send_to_char("&-RERROR! Player corpse with zero exp, please notify a god!&n\r\n", ch);
      wizlog(56, "MEMORY ERROR: Player corpse with zero exp!");
    }

    if( !IS_TRUSTED(ch) )
    {
      if( GET_LEVEL(t_ch) >= 56 )
      {
         resu_exp = (long)(resu_exp * 0.500);
      }
      else if( EVIL_RACE(t_ch) )
      {
         resu_exp = (long)(resu_exp * (get_property("gain.exp.mod.res.evil", 0.750)));
      }
      else
      {
         resu_exp = (long)(resu_exp * get_property("gain.exp.mod.res.normal", 0.750));
      }
    }

    logit(LOG_EXP, "Resu debug: %s (%d) by %s (%d): old exp: %d, new exp: %d, +exp: %d",
          GET_NAME(t_ch), GET_LEVEL(t_ch), GET_NAME(ch), GET_LEVEL(ch),
          GET_EXP(t_ch), GET_EXP(t_ch) + resu_exp, resu_exp);
    debug("&+RResurrect&n: %s (%d) by %s (%d): old exp: %d, new exp: %d, +exp: %d",
          GET_NAME(t_ch), GET_LEVEL(t_ch), GET_NAME(ch), GET_LEVEL(ch),
          GET_EXP(t_ch), GET_EXP(t_ch) + resu_exp, resu_exp);

    gain_exp(t_ch, NULL, resu_exp, EXP_RESURRECT);
  }

  GET_HIT(t_ch) = GET_MAX_HIT(t_ch);
  GET_MANA(t_ch) = MAX(0, GET_MAX_MANA(t_ch) >> 2);
  GET_VITALITY(t_ch) = MIN(0, GET_MAX_VITALITY(t_ch));

  if(GET_COND(t_ch, FULL) > 0)
  {
    GET_COND(t_ch, FULL) = 0;
  }
  if(GET_COND(t_ch, THIRST) > 0)
  {
    GET_COND(t_ch, THIRST) = 0;
  }

  SET_POS(ch, POS_STANDING + STAT_NORMAL);

  StartRegen(t_ch, EVENT_MOVE_REGEN);
  StartRegen(t_ch, EVENT_MANA_REGEN);

  if( t_ch && affected_by_spell(t_ch, SPELL_POISON) )
  {
    affect_from_char(t_ch, SPELL_POISON);
  }
  if( t_ch && IS_AFFECTED2(t_ch, AFF2_POISONED) )
  {
    REMOVE_BIT(t_ch->specials.affected_by2, AFF2_POISONED);
  }

  /*
   * Added by DTS 7/30/95
   */
  if( IS_PC(t_ch) && !writeCharacter(t_ch, 1, t_ch->in_room) )
  {
    logit(LOG_DEBUG, "Problem saving player %s in spell_resurrect()", GET_NAME(t_ch));
    send_to_char("There was a problem saving your character!\n", t_ch);
    send_to_char("Contact an Implementor ASAP.\n", t_ch);
  }

  extract_obj(obj);
}

void spell_preserve(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  if(!(obj))
    return;

  if(obj->type != ITEM_CORPSE)
  {
    send_to_char("You can only preserve a corpse!\n", ch);
    return;
  }
  // find a decay affect
  struct obj_affect *af;
  af = get_obj_affect(obj, TAG_OBJ_DECAY);

  if(!af)
  {
    act("$p doesn't seem to be in any danger of decaying.",
        TRUE, ch, obj, 0, TO_CHAR);
    return;
  }
  else if(!IS_TRUSTED(ch))
  {
    unsigned timePres;
    timePres = MAX(10, (level/2));
    sprintf(Gbuf1, "$p is preserved for an additional %d hours.", timePres);
    act(Gbuf1, 0, ch, obj, 0, TO_CHAR);
    act("$p glows briefly.", 0, ch, obj, 0, TO_ROOM);

    // convert timePres into game pulses
    timePres *= (SECS_PER_MUD_HOUR * WAIT_SEC);
    // add the old event time to timePres
    timePres += obj_affect_time(obj, af);
    // remove the old affect
    affect_from_obj(obj, TAG_OBJ_DECAY);
    // and create a new one
    set_obj_affected(obj, timePres, TAG_OBJ_DECAY, 0);
  }
  else
  {
    // just remove the decay affect completely.
    affect_from_obj(obj, TAG_OBJ_DECAY);

    act("$p is preserved forever!", FALSE, ch, obj, 0, TO_CHAR);
    act("$p glows brightly!", FALSE, ch, obj, 0, TO_ROOM);
  }
  if(obj && (obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], PC_CORPSE))
    writeCorpse(obj);
}

void spell_lesser_resurrect(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool     loss_flag = FALSE;
  int      chance, l, found, clevel, ss_roll;
  long     resu_exp;
  P_obj    obj_in_corpse, next_obj, t_obj, money;
  struct affected_type *af, *next_af;
  P_char   t_ch;

  if( !IS_ALIVE(ch) || !(obj) )
    return;

  if(obj->type != ITEM_CORPSE)
  {
    send_to_char("You can only resurrect corpses!\n", ch);
    return;
  }

  if(IS_NPC(ch))
    return;

  if(GET_CHAR_SKILL(ch, SKILL_DEVOTION) <= 20 && !IS_TRUSTED(ch)
    || (GET_CLASS(ch, CLASS_SHAMAN) && level < 52))
  {
    CharWait(ch, 100);
  }

  if(IS_SET(obj->value[1], NPC_CORPSE))
  {
    if(!obj->value[3] || !IS_TRUSTED(ch))
    {
      send_to_char("You can't resurrect this corpse!\n", ch);
      return;
    }
    t_ch = read_mobile(obj->value[3], VIRTUAL);
    if(!t_ch)
    {
      logit(LOG_DEBUG, "spell_resurrect(): mob %d not loadable", obj->value[3]);
      send_to_char("You can't resurrect this corpse!\n", ch);
      return;
    }
    /*
     * res'ed mobs will mark as their birth the room they are res'ed
     * in -Neb
     */
    GET_BIRTHPLACE(t_ch) = world[ch->in_room].number;
    if(!IS_SET(t_ch->specials.act, ACT_MEMORY))
      clearMemory(t_ch);
  }
  else
  {

    /*
     * res a player
     */

    found = 0;

    for (t_ch = character_list; t_ch; t_ch = t_ch->next)
    {
      if(t_ch && IS_PC(t_ch) && !str_cmp(t_ch->player.name, obj->action_description))
      {
        if(t_ch == ch)
        {
          send_to_char("You can't resurrect your own corpse!\n", ch);
          return;
        }
        if((obj->value[2] < 0) && !IS_TRUSTED(ch))
        {
          send_to_char("This corpse is not resurrectable.\n", ch);
          return;
        }
        if(!is_linked_to(ch, t_ch, LNK_CONSENT) && !IS_TRUSTED(ch))
        {
          /* In AD&D, when a person dies, its not just a matter of losing
             xp, but a complete end to the char.  Things work differently
             in the mud, and often a res is a Bad Thing.  Therefore to stop
             people from PK'ing, res'ing, PK'ing, etc, just to fuck up the
             persons xp and stats, consent will be REQUIRED to res */
          send_to_char("That person must consent to you first.\n", ch);
          return;
        }
        found = 1;
        break;
      }
    }
    if(!found)
    {
      send_to_char("You can't find a soul to reunite with this corpse!\n", ch);
      return;
    }

    if(GET_PID(t_ch) != obj->value[3])
    {
      if(IS_TRUSTED(ch))
        send_to_char("Different IDs, but you are godly..  enjoy.\n", ch);
      else
      {
        send_to_char("Similar, but not the same!  You fail.\n", ch);
        return;
      }
    }

    if(isCarved(obj))
    {
      if(IS_TRUSTED(ch))
        send_to_char("You regenerate the carved body parts.\n", ch);
      else
      {
        send_to_char("The soul you found rejects this mutilated corpse.\n", ch);
        return;
      }
    }

    /*
     * new bit, resurrectee must make a CON system shock save, if
     * they: make the save - spell works, they regain 80% of lost exp.
     * fail the save by < 50 - spell works, but they lose a semi-random
     * number of permanent stat points, usually from Con, but other
     * losses are possible. fail the spell by > 50 - spell fails,and
     * THIS corpse can never be resurrected (except by a god).  (No stat
     * losses, but they don't get any exp back either.)
     *
     * save maxes at 97% (for players), so it can fail on ANYONE, chance
     * has to fall below 50% before the spell can completely fail though
     * (Con (REAL) of < 40), stat losses will be fairly common though.
     * JAB
     */

//    ss_save = con_app[STAT_INDEX(GET_C_CON(t_ch))].shock;
    chance = 90 - (4 * (56 - GET_LEVEL(ch)));   /* 90% success at 56, 50% at 46 */
    ss_roll = number(1, 100);

    if(!IS_TRUSTED(ch) &&
       level <= 50)
    {
      /*
         if((ss_save + (100 - ss_save) / 2) < ss_roll) {
       */
      if(ss_roll > chance)
      {
        /*
         * complete failure, corpse is unressable.
         */
        logit(LOG_DEATH, "%s ressed %s:  Failed roll: %3d Chance: %3d Level: %2d",
              GET_NAME(ch), GET_NAME(t_ch), ss_roll, chance, level);
      }
    }

    act("$n &+Mhowls in pain&n as $s body &+Lcrumbles to dust.&n",
      FALSE, t_ch, 0, 0, TO_ROOM);
    act("You &+Mhowl in pain&n as your &+wsoul&n leaves your body to return to your corpse.",
       FALSE, t_ch, 0, 0, TO_CHAR);

    t_ch->only.pc->pc_timer[PC_TIMER_HEAVEN] = 0;

    if(t_ch->specials.fighting)
      stop_fighting(t_ch);
    if( IS_DESTROYING(t_ch) )
      stop_destroying(t_ch);
    StopAllAttackers(t_ch);

    if(IS_PC(t_ch) &&
       IS_RIDING(t_ch))
          stop_riding(t_ch);

    for (af = t_ch->affected; af; af = next_af)
    {
      next_af = af->next;
      if(!(af->flags & AFFTYPE_NODISPEL))
        affect_remove(t_ch, af);
    }

    if(GET_MONEY(t_ch) > 0)
    {
      /*
       * make a 'pile of coins' object to hold victim's cash
       */

      money =
        create_money(GET_COPPER(t_ch), GET_SILVER(t_ch), GET_GOLD(t_ch),
                     GET_PLATINUM(t_ch));
      SUB_MONEY(t_ch, GET_MONEY(t_ch), 0);
      obj_to_room(money, t_ch->in_room);
    }
    for (t_obj = t_ch->carrying; t_obj != NULL; t_obj = next_obj)
    {
      next_obj = t_obj->next_content;
      // WHY ON EARTH WOULD WE WANT TO DO THIS? - KVARK
//      if(IS_SET(world[t_ch->in_room].room_flags, DEATH) || IS_SET(obj->extra_flags, ITEM_TRANSIENT))
      if(IS_SET(obj->extra_flags, ITEM_TRANSIENT))
      {
        extract_obj(t_obj, TRUE); // Transient artis?
        t_obj = NULL;
      }
      else
      {
        obj_from_char(t_obj);
        obj_to_room(t_obj, t_ch->in_room);
      }
    }

    /*
     * clear equipment_list
     */
    for (l = 0; l < MAX_WEAR; l++)
      if(t_ch->equipment[l])
      {

        t_obj = unequip_char(t_ch, l);
        /*
         * below used to contain: IS_SET(world[t_ch->in_room].room_flags, DEATH)
         * Alas, that is not good.
         * /
         */
        if(IS_SET(t_obj->extra_flags, ITEM_TRANSIENT))
        {
          extract_obj(t_obj, TRUE); // Transient artis?
          t_obj = NULL;
        }
        else
          obj_to_room(t_obj, t_ch->in_room);
      }
    char_from_room(t_ch);
  }
  char_to_room(t_ch, ch->in_room, -2);

  /*
   * move objects in corpse to victim's inventory
   */
  for (obj_in_corpse = obj->contains; obj_in_corpse; obj_in_corpse = next_obj)
  {
    next_obj = obj_in_corpse->next_content;
    obj_from_obj(obj_in_corpse);
    if(obj_in_corpse->type == ITEM_MONEY)
    {
      GET_COPPER(t_ch) += obj_in_corpse->value[0];
      GET_SILVER(t_ch) += obj_in_corpse->value[1];
      GET_GOLD(t_ch) += obj_in_corpse->value[2];
      GET_PLATINUM(t_ch) += obj_in_corpse->value[3];
      extract_obj(obj_in_corpse);
      obj_in_corpse = NULL;
    }
    else
      obj_to_char(obj_in_corpse, t_ch);
  }

  act("&+CAn aura of &+Wsoft&n &+Clight surrounds&n $n &+C for a moment.",
    TRUE, t_ch, 0, 0, TO_ROOM);
  act("$n &+Ccomes to life again!&+C Taking a deep breath,&n $n&+C opens $s eyes!",
    TRUE, t_ch, 0, 0, TO_ROOM);
  act("&+LYou are extremely tired after resurrecting&n $N.",
    TRUE, ch, 0, t_ch, TO_CHAR);
  if(loss_flag)
    act("You feel drained!", TRUE, t_ch, 0, 0, TO_CHAR);
  act("You are &+yextremely tired&n after being resurrected!",
    TRUE, t_ch, 0, 0, TO_CHAR);

  // play_sound(SOUND_RESSURECTION, NULL, t_ch->in_room, TO_ROOM);

  GET_VITALITY(ch) = MIN(0, GET_VITALITY(ch));
  StartRegen(ch, EVENT_MOVE_REGEN);

  /*
   * restore lost exp from death
   */
  clevel = obj->value[2];

  GET_HIT(t_ch) = GET_MAX_HIT(t_ch);
  GET_MANA(t_ch) = MAX(0, GET_MAX_MANA(t_ch) >> 2);
  GET_VITALITY(t_ch) = MIN(0, GET_MAX_VITALITY(t_ch));

  if(GET_COND(t_ch, FULL) > 0)
    GET_COND(t_ch, FULL) = 0;
  if(GET_COND(t_ch, THIRST) > 0)
    GET_COND(t_ch, THIRST) = 0;

  StartRegen(t_ch, EVENT_MOVE_REGEN);
  StartRegen(t_ch, EVENT_MANA_REGEN);

  /*
   * Added by DTS 7/30/95
   */
  if(IS_PC(t_ch) && !writeCharacter(t_ch, 1, t_ch->in_room))
  {
    logit(LOG_DEBUG, "Problem saving player %s in spell_resurrect()",
          GET_NAME(t_ch));
    send_to_char("There was a problem saving your character!\n", t_ch);
    send_to_char("Contact an Implementor ASAP.\n", t_ch);
  }
/*  if(clevel == 56)
    advance_level(t_ch);*/
  extract_obj(obj);
}

void spell_mass_invisibility(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{

  if( !IS_ALIVE(ch) )
    return;

  LOOP_THRU_PEOPLE(victim, ch)
    if(!IS_TRUSTED(ch) && !IS_TRUSTED(victim))
      spell_improved_invisibility(level, ch, 0, 0, victim, 0);
    else if(IS_TRUSTED(ch))
      spell_improved_invisibility(level, ch, 0, 0, victim, 0);     

  // for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  // {
    // if(IS_SET(obj->wear_flags, ITEM_TAKE))
      // spell_improved_invisibility(level, ch, 0, 0, 0, obj);
  // }
}

void spell_nether_gate(int level, P_char ch, P_char victim, P_obj obj)
{
  /**
   ** Main code for Nether gate spell is in spells.c
   **/
}
void spell_gate(int level, P_char ch, P_char victim, P_obj obj)
{
  /**
   ** Main code for Gate spell is in spells.c
   **/
}

void spell_plane_shift(int level, P_char ch, P_char victim, P_obj obj)
{
  /**
   ** Main code for Gate spell is in spells.c
   **/
}

void spell_waterbreath(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_WATERBREATH))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_WATERBREATH)
      {
        af1->duration = level;
      }
    return;
  }
  bzero(&af, sizeof(af));
  af.type = SPELL_WATERBREATH;
  af.duration =  level;
  af.bitvector = AFF_WATERBREATH;

  if(!IS_AFFECTED(victim, AFF_WATERBREATH))
  {
    act("You suddenly grow gills!", FALSE, victim, 0, 0, TO_CHAR);
    act("$n suddenly grows gills!", TRUE, victim, 0, 0, TO_ROOM);
  }
  affect_to_char(victim, &af);

}

void spell_protection_from_fire(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_FIRE))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_FIRE;
    af.duration = level;
    af.bitvector = AFF_PROT_FIRE;
    affect_to_char(victim, &af);
    send_to_char("You feel protected from the &+Rfire!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_FIRE)
      {
        af1->duration = level;
      }
  }

}

void spell_protection_from_cold(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_COLD))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_COLD;
    af.duration = level;
    af.bitvector2 = AFF2_PROT_COLD;
    affect_to_char(victim, &af);
    send_to_char("You feel protected from the &+ccold!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_COLD)
      {
        af1->duration = level;
      }
  }

}

void spell_protection_from_living(int level, P_char ch, char *arg, int type,
                                  P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_LIVING))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_LIVING;
    af.duration = level;
    af.bitvector4 = AFF4_PROT_LIVING;
    affect_to_char(victim, &af);
    send_to_char("&+LAn aura of death surrounds you!&n\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_LIVING)
      {
        af1->duration = level;
      }
  }

}

void spell_protection_from_animals(int level, P_char ch, char *arg, int type,
                                   P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_ANIMAL))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_ANIMAL;
    af.duration = level;
    af.bitvector3 = AFF3_PROT_ANIMAL;
    affect_to_char(victim, &af);
    send_to_char("You feel protected from the &+Ganimals of the world!\n",
                 victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_ANIMAL)
      {
        af1->duration = level;
      }
  }
}

void spell_animal_friendship(int level, P_char ch, char *arg, int type,
                             P_char victim, P_obj obj)
{
  act("You feel overwhelmed with &+Mcompassion&n...\n",
    FALSE, ch, 0, 0, TO_CHAR);

  if(is_natural_creature(victim))
  {
    if(IS_FIGHTING(victim))
    {
      if(victim->specials.fighting)
      {
        stop_fighting(victim);
        stop_fighting(ch);
        clearMemory(victim);
        return;
      }
    }
    else
    {
      charm_generic(level, ch, victim);
    }
  }
  else
    act("Apparently, this creature is not close enough to the nature!\n",
      FALSE, ch, 0, 0, TO_CHAR);
}



void spell_protection_from_gas(int level, P_char ch, char *arg, int type,
                               P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_GAS))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_GAS;
    af.duration = level;
    af.bitvector2 = AFF2_PROT_GAS;
    affect_to_char(victim, &af);
    send_to_char("You feel protected from the &+Gpoisonous gasses!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_GAS)
      {
        af1->duration = level;
      }
  }

}

void spell_protection_from_acid(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_ACID))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_ACID;
    af.duration = level;
    af.bitvector2 = AFF2_PROT_ACID;
    affect_to_char(victim, &af);
    send_to_char("You feel protected from &+yacid!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_ACID)
      {
        af1->duration = level;
      }
  }

}

void spell_protection_from_lightning(int level, P_char ch, char *arg,
                                     int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_LIGHTNING))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_LIGHTNING;
    af.duration = level;
    af.bitvector2 = AFF2_PROT_LIGHTNING;
    affect_to_char(victim, &af);
    send_to_char("You feel protected from the &+Clightning!\n", victim);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_PROTECT_FROM_LIGHTNING)
      {
        af1->duration = level;
      }
  }

}

void darkness_dissipate_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      room;

  if(!data)
  {
    logit(LOG_EXIT, "Call to darkness dissipation with invalid data");
    raise(SIGSEGV);
  }
  /*
   * Ok, let's rock
   */
  room = *((int*)data);
  if(room <= 0 || room > top_of_world )
  {
    logit(LOG_DEBUG, "Darkness dissipation in invalid room [%d]", room);
    return;
  }

  if(!IS_SET(world[room].room_flags, MAGIC_DARK))
    return;

  REMOVE_BIT(world[room].room_flags, MAGIC_DARK);
  send_to_room("&+LThe darkness seems to lift a bit.\n", room);
  room_light(room, REAL);

  return;
}

void spell_darkness(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  char     buff[64];

  if(obj)
  {
    if(IS_OBJ_STAT(obj, ITEM_LIT))
    {
      act("The illuminations of $p are negated.", FALSE, ch, obj, 0, TO_CHAR);
      REMOVE_BIT(obj->extra_flags, ITEM_LIT);
      if(OBJ_WORN(obj) || OBJ_CARRIED(obj))
        char_light(ch);
      room_light(ch->in_room, REAL);
      return;
    }
    act("$p &+Wglows brightly for a moment, but the glow fades away.",
        FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  else
  {
/*
    send_to_char("&+LThe room is carpeted in darkness!\n", ch);
    act("&+LThe room goes dark!", 0, ch, 0, 0, TO_ROOM);
    SET_BIT(world[ch->in_room].room_flags, TWILIGHT);
*/
    if(IS_SET(world[ch->in_room].room_flags, MAGIC_DARK))
    {
      send_to_char("Nothing happens.\n", ch);
      return;
    }

    if(IS_SET(world[ch->in_room].room_flags, MAGIC_LIGHT))
    {
       REMOVE_BIT(world[ch->in_room].room_flags, MAGIC_LIGHT);
       send_to_char("&+LThe room becomes much more stygian.\n", ch);
       act("&+LThe room becomes much more stygian.", 0, ch, 0, 0, TO_ROOM);
    }
    else
    {
      sprintf(buff, "%d", ch->in_room);
      SET_BIT(world[ch->in_room].room_flags, MAGIC_DARK);
      send_to_char("&+LThe room is carpeted in darkness!\n", ch);
      act("&+LThe room goes dark!", 0, ch, 0, 0, TO_ROOM);

      add_event(darkness_dissipate_event, level * 50, NULL, NULL, NULL, 0, &(ch->in_room), sizeof((ch->in_room)));
      //AddEvent(EVENT_SPECIAL, level * 50, TRUE, darkness_dissipate_event, buff);
    }
  }
  char_light(ch);
  room_light(ch->in_room, REAL);
}

void spell_innate_darkness(int level, P_char ch, P_char victim, P_obj obj)
{
  spell_darkness(level, ch, 0, 0, 0, 0);
}

void spell_infravision(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED(victim, AFF_INFRAVISION))
    return;

  if(!affected_by_spell(victim, SPELL_INFRAVISION))
  {
    send_to_char("Your feel your infrared vision improve.\n", victim);
    act("$n's eyes start to glow &+rred&n!", FALSE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));
    af.type = SPELL_INFRAVISION;
    af.duration =  level;
    af.bitvector = AFF_INFRAVISION;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_INFRAVISION)
      {
        af1->duration = level;
      }
    }
  }
}

void spell_mass_embalm(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  char     buf[MAX_STRING_LENGTH];

  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    if(obj->type == ITEM_CORPSE)
      spell_embalm(level, ch, 0, SPELL_TYPE_SPELL, victim, obj);
  }
}

void spell_mass_preserve(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  char     buf[MAX_STRING_LENGTH];

  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    if(obj->type == ITEM_CORPSE)
      spell_preserve(level, ch, 0, 0, victim, obj);
  }
}


void spell_embalm(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  char     buf[MAX_STRING_LENGTH];
  unsigned embalm_time;

  /*
   * Check to if it is a corpse.
   */
  if(obj->type != ITEM_CORPSE)
  {
    send_to_char("All you know how to preserve is corpses!\n", ch);
    return;
  }

  // find a decay affect
  struct obj_affect *af;
  af = get_obj_affect(obj, TAG_OBJ_DECAY);

  if(!af)
  {
    act("$p probably will not decay in the near future.", TRUE, ch,
        obj, 0, TO_CHAR);
    return;
  }
  embalm_time = MAX(50, level*2);

  sprintf(buf, "$p is preserved for an additional %d hours.", embalm_time);
  act(buf, 0, ch, obj, 0, TO_CHAR);
  act("$p glows &+rblood red&n briefly.", 0, ch, obj, 0, TO_ROOM);

  // convert embalm_time into game pulses
  embalm_time *= (SECS_PER_MUD_HOUR * WAIT_SEC);
  // add the old event time to embalm_time
  embalm_time += obj_affect_time(obj, af);
  // remove the old affect
  affect_from_obj(obj, TAG_OBJ_DECAY);
  // and create a new one
  set_obj_affected(obj, embalm_time, TAG_OBJ_DECAY, 0);

  if(obj && (obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], PC_CORPSE))
    writeCorpse(obj);

  return;
}

void spell_charm_person(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  if(!IS_HUMANOID(victim))
  {
    send_to_char("That doesn't look like a person to me...\n", ch);
    return;
  }
  charm_generic(level, ch, victim);
}

void spell_charm_animal(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  if(IS_HUMANOID(victim))
  {
    send_to_char
      ("Just because they SMELL like an animal doesn't mean they are one!\n",
       ch);
    return;
  }
  charm_generic(level, ch, victim);
}

void spell_divine_fury(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;
  char     strn[256];
  int room;
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(ch->in_room))
        return;

  if(affected_by_spell(ch, SPELL_DIVINE_FURY))
  {
    send_to_char("You are already &+Wdivinely furious!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_DIVINE_FURY;
  af.duration =  2;
  af.flags = AFFTYPE_NODISPEL | AFFTYPE_NOMSG;

  af.modifier = level / 5;
  af.location = APPLY_DAMROLL;
  affect_to_char(ch, &af);

  af.modifier = level / 5;
  af.location = APPLY_HITROLL;
  affect_to_char(ch, &af);
  
  send_to_char("&+RYour fury rages as you call upon your deity for power!\n", ch);
  sprintf(strn, "%s$n %sis briefly surrounded by a%s glow!",
          IS_EVIL(ch) ? "&+R" : "&+W", IS_EVIL(ch) ? "&+R" : "&+W",
          IS_EVIL(ch) ? "n unholy" : " holy");
  act(strn, TRUE, ch, 0, 0, TO_ROOM);
}

void spell_soulshield(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;
  if( !ch || !IS_ALIVE(ch) || !victim || !IS_ALIVE(victim) )
  {
    return;
  }
  if((GET_ALIGNMENT(ch) > -950) && (GET_ALIGNMENT(ch) < 950) && !IS_MULTICLASS_PC(ch))
  {
    send_to_char("Your beliefs aren't strong enough!\n", ch);
    return;
  }

  if(IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
  {
    send_to_char("&+WHoly &+Lenergies do not mix so well with those wrought of the Negative Plane.&n\n", ch);
    return;
  }
  if(!IS_AFFECTED2(victim, AFF2_SOULSHIELD))
  {
    char     buf1[500], buf2[500];

    if(IS_EVIL(ch))
    {
      strcpy(buf1, "&+rAn aura of malevolency forms around&n $n!");
      strcpy(buf2, "&+rAn aura of malevolency forms around you!&n");
    }
    else
    {
      strcpy(buf1, "&+WA holy aura forms around&n $n!");
      strcpy(buf2, "&+WA holy aura forms around you!&n");
    }
    act(buf1, TRUE, victim, 0, 0, TO_ROOM);
    act(buf2, TRUE, victim, 0, 0, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_SOULSHIELD;
    af.duration =  (int) (15 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5);
    af.bitvector2 = AFF2_SOULSHIELD;
    affect_to_char(victim, &af);
  }
}

void spell_holy_sacrifice(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch))
  {
    logit(LOG_EXIT, "spell_holy_sacrifice in magic.c no ch");
    raise(SIGSEGV);
    return;
  }

  if(!affected_by_spell(victim, SPELL_HOLY_SACRIFICE) &&
     !IS_SET(ch->specials.affected_by4, AFF4_HOLY_SACRIFICE))
  {
    char     buf1[500], buf2[500];

/*
    if(victim == ch){
*/
    strcpy(buf1, "&+WA feeling of peace emanates from&n $n!");
    strcpy(buf2, "&+WYou feel as if your pain might do some good!&n");
    act(buf1, TRUE, victim, 0, 0, TO_ROOM);
    act(buf2, TRUE, victim, 0, 0, TO_CHAR);
/*
    }else{
  strcpy(buf1, "&+WA beam of light links $n to $N, and then fades away&N.");
  strcpy(buf2, "&+WYou feel linked to $n.&N");
  act(buf1, TRUE, ch, 0, victim, TO_NOTVICT);
  act(buf2, TRUE, ch, 0, victim, TO_VICT);
  act(buf2, TRUE, victim, 0, ch, TO_VICT);
    }
*/
    bzero(&af, sizeof(af));
    af.type = SPELL_HOLY_SACRIFICE;
    af.duration = (int) (15 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 10);
/*    af.modifier = GET_PID(ch); */
    af.bitvector4 = /*(ch == victim)? */ AFF4_HOLY_SACRIFICE /*:0 */ ;
    affect_to_char(victim, &af);
  }
  else
  {
    send_to_char( "&+YYou are already holy enough.&n\n\r", ch );
  }
}
void spell_battle_ecstasy(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!(ch))
  {
    return;
  }

  if(!IS_ALIVE(ch))
  {
    return;
  }

  if(!IS_AFFECTED4(victim, AFF4_BATTLE_ECSTASY))
  {
    char     buf1[500], buf2[500];

    strcpy(buf1,
           "$n &+rbegins to drool, a bloodthirsty look passing over $s face.&n");
    strcpy(buf2, "&+rYour blood begins to boil-- where's the fight?&n");
    act(buf1, TRUE, victim, 0, 0, TO_ROOM);
    act(buf2, TRUE, victim, 0, 0, TO_CHAR);

    bzero(&af, sizeof(af));
    af.type = SPELL_BATTLE_ECSTASY;
    af.duration = (int) (15 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 10);
    af.bitvector4 = AFF4_BATTLE_ECSTASY;
    affect_to_char(victim, &af);
  }
}

void spell_mass_heal(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  P_char tch;
  int healed;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, tch, obj);
    grapple_heal(tch);
    if(GET_HIT(tch) > GET_MAX_HIT(tch))
      continue;

    int maxhits = IS_HARDCORE(ch) ? 110 : 100;

    healed = vamp(tch, (int) maxhits + number(1, level / 3), GET_MAX_HIT(tch) - number(1, 4));
    if(tch->specials.fighting) {
      gain_exp(ch, tch, healed, EXP_HEALING);
    }
    update_pos(tch);
    if(RACE_PUNDEAD(tch))
      send_to_char("&+WYou feel the powers of darkness strengthen you!\n",
          tch);
    else
      send_to_char("&+WA warm feeling fills your body.\n", tch);
  }
}

void spell_prayer(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char tch;
  P_char tvict;
  const char  *god_name;
  struct group_list *group;
  char Gbuf1[MAX_STRING_LENGTH];
  bool prayer;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  god_name = get_god_name(ch);
  sprintf(Gbuf1, "$n &+wopenly prays out to %s, asking for intervention!", god_name);
  act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
  sprintf(Gbuf1, "&+wYou pray to&n %s.", god_name);
  act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);

  if( !ch->group && !IS_NPC(ch) )
  {
    prayer = FALSE;

    for( tvict = character_list; tvict; tvict = tvict->next )
    {
      if( IS_NPC(tvict) )
      {
        if( forget(tvict, ch) )
        {
          prayer = TRUE;
        }
      }
    }

    if( prayer )
      send_to_char("&+WYour past transgressions will no longer haunt you.&n\n", ch);
    return;
  }

  for( group = ch->group; group; group = group->next )
  {
    tch = group->ch;
    prayer = FALSE;

    if( IS_PC(tch) && tch->in_room == ch->in_room )
    {
      for( tvict = character_list; tvict; tvict = tvict->next )
      {
        if( IS_NPC(tvict) )
        {
          if( forget(tvict, tch) )
          {
            prayer = TRUE;
          }
        }
      }
    }

    if( prayer )
    {
      send_to_char("&+WYour past transgressions will no longer haunt you.&n\n", tch);
    }
  }
}

void spell_mass_barkskin(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  LOOP_THRU_PEOPLE(victim, ch)
  {
    if(ch->specials.z_cord == victim->specials.z_cord)
      spell_barkskin(level, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }
}

void spell_tranquility(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{

  P_char   d = NULL;
  int      skl_lvl;
  P_char   oponent = NULL;

  skl_lvl = (int) (level * 1.15);

  act("&+g$n calls upon nature, &+ca calming wind passes through&n.\n",
      FALSE, ch, 0, 0, TO_ROOM);
  act("&+gYou call upon nature, &+ca calming wind passes through&n.\n",
      FALSE, ch, 0, 0, TO_CHAR);
  for (d = world[ch->in_room].people; d; d = d->next_in_room)
  {
    if(d->specials.fighting)
      if(number(1, 130) < skl_lvl)
      {
         oponent = d->specials.fighting;
         // Lom: made them forget each other in pairs. as StopMercifulAttackers dont clear memories 
         if(IS_PC(oponent))
            send_to_char("A sense of calm comes upon you.\n", oponent);
         if(IS_PC(d))
            send_to_char("A sense of calm comes upon you.\n", d);
         if(IS_NPC(d) && IS_PC(oponent))
            forget(d,oponent);
         if(IS_NPC(oponent) && IS_PC(d))
            forget(oponent,d);
         stop_fighting(oponent);
         stop_fighting(d);
         update_pos(oponent);
         update_pos(d);
//        StopMercifulAttackers(d);
      }

    if(IS_DESTROYING(d) && number(1, 130) < skl_lvl)
    {
      stop_destroying( d );
    }
  }
  CharWait(ch, PULSE_VIOLENCE);
}

void spell_true_seeing(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_TRUE_SEEING))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_TRUE_SEEING)
      {
        af1->duration = 72;
      }
    return;
  }
  bzero(&af, sizeof(af));

  af.type = SPELL_TRUE_SEEING;
  af.duration =  72;

  af.bitvector4 = AFF4_SENSE_HOLINESS;
  af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
  af.bitvector = AFF_SENSE_LIFE | AFF_DETECT_INVISIBLE;
  affect_to_char(ch, &af);

  send_to_char("&+MYour vision sharpens considerably.&n\n", ch);

}

void spell_tree(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_obj tobj, next_tobj;

  if(world[ch->in_room].sector_type != SECT_FOREST)
  {
    send_to_char("&+GTrees&+g don't grow here!&n\n", ch);
    return;
  }

                // DALRETH - 3/05
  if(ch->specials.z_cord > 0)
  {
    send_to_char("There isn't enough forest up here.\r\n", ch);
    return;
  }
  if(IS_RIDING(ch))
  {
    send_to_char("&+WYour mount prevents you from blending into the forest...&n\r\n", ch);
    return;
  }
  if(affected_by_spell(ch, SPELL_FAERIE_FIRE))
  {
    send_to_char("You can't hide while surrounded by &+Mflames!&n\n\r", ch);
    return;
  }

  if(IS_FIGHTING(ch) || IS_DESTROYING(ch))
  {
    send_to_char("Your magic isn't strong enough to hide you from battle!\r\n", ch);
    return;
  }

  if( IS_AFFECTED(ch, AFF_HIDE) )
    REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);

  CharWait(ch, PULSE_VIOLENCE * 3);

  /*  destroy your tracks! */
  for( tobj = world[ch->in_room].contents; tobj; tobj = next_tobj )
  {
    next_tobj = tobj->next_content;
    if( tobj->R_num == real_object(VNUM_TRACKS) )
    {
      extract_obj(tobj);
      tobj = NULL;
    }
  }

  send_to_char("&+wYou blend silently into the &+gforest.&n\r\n", ch);
  SET_BIT(ch->specials.affected_by, AFF_HIDE);
}

void spell_animal_vision(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_ANIMAL_VISION))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_ANIMAL_VISION)
      {
        af1->duration = 72;
      }
    return;
  }
  bzero(&af, sizeof(af));
  af.type = SPELL_ANIMAL_VISION;
  af.duration =  72;
  switch (world[ch->in_room].sector_type)
  {
  case SECT_FOREST:
    if(GET_LEVEL(ch) > 45)
    {
      af.bitvector = AFF_SENSE_LIFE | AFF_FARSEE | AFF_DETECT_INVISIBLE;
    }
    else
    {
      af.bitvector = AFF_SENSE_LIFE | AFF_FARSEE;
    }
    break;
  case SECT_MOUNTAIN:
    af.bitvector = AFF_SENSE_LIFE;
    af.bitvector2 = AFF2_DETECT_MAGIC;
    break;
  case SECT_FIELD:
  case SECT_HILLS:
    af.bitvector = AFF_FARSEE;
    af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
    break;
  case SECT_SWAMP:
    af.bitvector = AFF_SENSE_LIFE;
    af.bitvector2 = AFF2_DETECT_EVIL | AFF2_DETECT_GOOD;
    break;
  case SECT_UNDRWLD_WILD:
  case SECT_UNDRWLD_CITY:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_UNDRWLD_SLIME:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_UNDRWLD_LIQMITH:
  case SECT_UNDRWLD_MUSHROOM:
    af.bitvector = AFF_INFRAVISION;
    break;
  default:
    af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_EVIL | AFF2_DETECT_GOOD;
    break;
  }
  affect_to_char(ch, &af);
  send_to_char
    ("Your eyes turn &+Yj&n&+ye&+Yw&n&+ye&+Yl&n&+ye&+Yd&n like an &+yanimal&n.\n",
     ch);
  act
    ("$n's eyes turn &+Yj&n&+ye&+Yw&n&+ye&+Yl&n&+ye&+Yd&n like an &+yanimal&n!\n",
     FALSE, ch, 0, 0, TO_ROOM);

}

void spell_natures_blessing(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_NATURES_BLESSING)) {
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next) {
        if(af1->type == SPELL_NATURES_BLESSING) {
            af1->duration = 72;
        }
    }
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_NATURES_BLESSING;
  af.duration =  72;
  switch (world[ch->in_room].sector_type)
  {
      case SECT_FOREST:
          af.bitvector = AFF_PROT_FIRE;
          af.bitvector2 = AFF2_PROT_GAS | AFF2_PROT_ACID | AFF2_PROT_COLD | AFF2_PROT_LIGHTNING;
          break;
      case SECT_HILLS:
      case SECT_FIELD:
      case SECT_MOUNTAIN:
          af.bitvector2 = AFF2_PROT_COLD | AFF2_PROT_LIGHTNING;
          break;
      case SECT_SWAMP:
      case SECT_DESERT:
          af.bitvector = AFF_PROT_FIRE;
          af.bitvector2 = AFF2_PROT_ACID | AFF2_PROT_GAS;
          break;
      default:
          af.bitvector = AFF_PROTECT_GOOD | AFF_PROTECT_EVIL;
          break;
  }

  affect_to_char(ch, &af);
  send_to_char("You feel overwhelmed with the &+gwarmth &+Gof &+gnature&n.\n", ch);
  act("$n's skin &+Gflashes green&n for a moment, and then returns to normal.\n",
      FALSE, ch, 0, 0, TO_ROOM);
}

void spell_shadow_vision(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_SHADOW_VISION))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SHADOW_VISION)
      {
        af1->duration = 72;
      }
    return;
  }
  bzero(&af, sizeof(af));

  af.type = SPELL_SHADOW_VISION;
  af.duration =  72;

  af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
  af.bitvector = AFF_SENSE_LIFE | AFF_DETECT_INVISIBLE;
  affect_to_char(ch, &af);

  send_to_char("&+LA dark tint covers your vision.&n\n", ch);
  act("&+m$n's&+M eyes turn &+Lpitch black!", FALSE, ch, 0, 0, TO_ROOM);
}

void spell_rope_trick(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  struct affected_type af;
  int      howmany = 0, room;
  P_char   temp_vict;

  if(ch != victim)
    if(!is_linked_to(ch, victim, LNK_CONSENT))
      if(resists_spell(ch, victim))
        return;

  temp_vict = victim;
  
  if(IS_WATER_ROOM(ch->in_room) ||
    world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("It's too wet here to hide anything.\r\n", ch);
    return;
  }

  if(!NewSaves(victim, SAVING_SPELL, 0) || (ch == victim) ||
      is_linked_to(ch, victim, LNK_CONSENT))
  {

    LOOP_THRU_PEOPLE(temp_vict, ch)
    {
      if(affected_by_spell
          (temp_vict, SPELL_ROPE_TRICK && IS_AFFECTED(temp_vict, AFF_HIDE)))
        howmany++;
    }
    if(howmany > 2)
    {
      act
        ("&+LYou don't seem to be able to to conjure any more &+yropes&n&+L.&n",
         TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
    act
      ("&+LA long &+yrope&+L appears, dangling in mid-air.&N\n$N &+Lscrambles up the &+yrope&+L and vanishes from sight.&n",
       TRUE, ch, 0, victim, TO_CHAR);
    act
      ("&+LA long &+yrope&+L appears, dangling in mid-air.&N\n&+LYou scramble up the &+yrope&+L and hide.&n",
       TRUE, ch, 0, victim, TO_VICT);
    act
      ("&+LA long &+yrope&+L appears, dangling in mid-air.&N\n$N &+Lscrambles up the &+yrope&+L and vanishes from sight.&n",
       TRUE, ch, 0, victim, TO_NOTVICT);


    SET_BIT(victim->specials.affected_by, AFF_HIDE);
    bzero(&af, sizeof(af));
    af.type = SPELL_ROPE_TRICK;
    af.duration = 5;
    affect_to_char(victim, &af);
  }
}

void spell_animal_growth(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
   int size, size2;

   struct affected_type af;

   switch (GET_RACE(victim))
   {
     case RACE_HERBIVORE:
     case RACE_CARNIVORE:
     case RACE_REPTILE:
     case RACE_SNAKE:
     case RACE_INSECT:
     case RACE_ARACHNID:
     case RACE_AQUATIC_ANIMAL:
     case RACE_FLYING_ANIMAL:
     case RACE_QUADRUPED:
     case RACE_ANIMAL:
     case RACE_PRIMATE:
        break;
     default:
        send_to_char("Your target must be an animal!\n", ch);
        return;
        break;
   }
   size = GET_SIZE(victim);
   size2 = GET_ALT_SIZE(ch);

   if((size + 1 < size2) || (size - 1 > size2))
   {
     send_to_char("You get the feeling your body couldn't change quite that much.\r\n", ch);
     return;
   }
   else if(size < size2)
   {
     spell_reduce(GET_LEVEL(ch), ch,  0, SPELL_ANIMAL_GROWTH, ch, NULL);
   }
   else if(size > size2)
   {
     spell_enlarge(GET_LEVEL(ch), ch, 0, SPELL_ANIMAL_GROWTH, ch, NULL);
   }
   else if(size == size2)
   {
     send_to_char("It's the same size as you, duh!\r\n", ch);
   }

}

void spell_enlarge(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool animal_growth = FALSE;
  struct affected_type af;

  if(IS_AFFECTED3(victim, AFF3_ELEMENTAL_FORM) ||
      IS_AFFECTED5(victim, AFF5_TITAN_FORM))
  {
    send_to_char("Nah.  They're quite big already.\n", ch);
    return;
  }
  
  if(ch != victim)
    if(!is_linked_to(ch, victim, LNK_CONSENT))
      return;
  
  if(type == SPELL_ANIMAL_GROWTH)
    animal_growth = TRUE;
  
  if(!NewSaves(victim, SAVING_SPELL, 0) || (ch == victim) ||
      is_linked_to(ch, victim, LNK_CONSENT))
  {
    if(IS_SET(victim->specials.affected_by3, AFF3_REDUCE))
    {
      REMOVE_BIT(victim->specials.affected_by3, AFF3_REDUCE);
      if(affected_by_spell(victim, SPELL_REDUCE))
        affect_from_char(victim, SPELL_REDUCE);
      act("$N returns to $S normal size!", TRUE, ch, 0, victim, TO_CHAR);
      act("You return to your normal size!", TRUE, ch, 0, victim, TO_VICT);
      act("$N returns to $S normal size!", TRUE, ch, 0, victim, TO_NOTVICT);
      return;
    }
    if(GET_SIZE(victim) == SIZE_MAXIMUM)
      return;


    if(!affected_by_spell(victim, SPELL_ENLARGE))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_ENLARGE;
      af.duration = 3;
      af.bitvector3 = AFF3_ENLARGE;
      affect_to_char(victim, &af);

      af.bitvector3 = 0;
      af.modifier = (victim->base_stats.Str / 10);
      af.location = APPLY_STR_MAX;
      affect_to_char(victim, &af);

      af.modifier = (victim->base_stats.Con / 4);
      af.location = APPLY_CON_MAX;
      affect_to_char(victim, &af);

      af.modifier = -(victim->base_stats.Agi / 5);
      af.location = APPLY_AGI;
      affect_to_char(victim, &af);

      af.modifier = -(victim->base_stats.Dex / 5);
      af.location = APPLY_DEX;
      affect_to_char(victim, &af);
    
      if(animal_growth) /*  messages for animal growth */
      {
        act("You &+Gconcentrate&N on your target and &+Ytransform&n your body accordingly!", TRUE, ch, 0, 0, TO_CHAR);
        act("$n &+Gconcentrates&N for a few seconds as $s body takes on a &+Ynew size!&N", TRUE, ch, 0, 0, TO_ROOM);
      }
      else
      {
         act("$N grows to about twice $S normal size!", TRUE, ch, 0, victim, TO_CHAR);
         act("You grow to about twice your normal size!", TRUE, ch, 0, victim, TO_VICT);
         act("$N grows to about twice $S normal size!", TRUE, ch, 0, victim, TO_NOTVICT);
      }
    }
    else
    {
      struct affected_type *af1;

      for (af1 = victim->affected; af1; af1 = af1->next)
      {
        if(af1->type == SPELL_ENLARGE)
        {
          af1->duration = level;
        }
      }
    }

    if(IS_NPC(victim) && CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
        MobStartFight(victim, ch);
    }
  }
}

void spell_reduce(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool animal_growth = FALSE;
  struct affected_type af;

  if(type == SPELL_ANIMAL_GROWTH)
    animal_growth = TRUE;
  
  if(GET_SIZE(victim) == SIZE_TINY && !(victim->specials.affected_by3, AFF3_REDUCE))
    {
       send_to_char("Why would you want to reduce them? They are tiny enough!&n\n\r", ch);
	return; //check to make sure they are not racial tiny.
    }
  
  if(!NewSaves(victim, SAVING_SPELL, 0) || (ch == victim) ||
      is_linked_to(ch, victim, LNK_CONSENT))
  {
    if(IS_SET(victim->specials.affected_by3, AFF3_ENLARGE))
    {
      REMOVE_BIT(victim->specials.affected_by3, AFF3_ENLARGE);
      if(affected_by_spell(victim, SPELL_ENLARGE))
        affect_from_char(victim, SPELL_ENLARGE);
      act("$N returns to $S normal size!", TRUE, ch, 0, victim, TO_CHAR);
      act("You return to your normal size!", TRUE, ch, 0, victim, TO_VICT);
      act("$N returns to $S normal size!", TRUE, ch, 0, victim, TO_NOTVICT);
      return;
    }
    if(GET_SIZE(victim) == SIZE_MINIMUM)
      return;

    if(!affected_by_spell(victim, SPELL_REDUCE))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_REDUCE;
      af.duration = 3;
      af.bitvector3 = AFF3_REDUCE;
      affect_to_char(victim, &af);

      af.bitvector3 = 0;
      af.modifier = -(victim->base_stats.Str / 4);
      af.location = APPLY_STR;
      affect_to_char(victim, &af);

      af.modifier = -(victim->base_stats.Con / 5);
      af.location = APPLY_CON;
      affect_to_char(victim, &af);

      af.modifier = (victim->base_stats.Agi / 5);
      af.location = APPLY_AGI_MAX;
      affect_to_char(victim, &af);

      af.modifier = (victim->base_stats.Dex / 5);
      af.location = APPLY_DEX_MAX;
      affect_to_char(victim, &af);

      if(animal_growth) /*  messages for animal growth */
      {
        act("You &+Gconcentrate&N on your target and &+Ytransform&n your body accordingly!", TRUE, ch, 0, 0, TO_CHAR);
        act("$n &+Gconcentrates&N for a few seconds as $s body takes on a &+Ynew size!&N", TRUE, ch, 0, 0, TO_ROOM);
      }
      else
      {
      act("$N shrinks to about half $S normal size!", TRUE, ch, 0, victim, TO_CHAR);
      act("You shrink to about half your normal size!", TRUE, ch, 0, victim, TO_VICT);
      act("$N shrinks to about half $S normal size!", TRUE, ch, 0, victim, TO_NOTVICT);
      }
    }
    else
    {
      struct affected_type *af1;

      for (af1 = victim->affected; af1; af1 = af1->next)
        if(af1->type == SPELL_REDUCE)
        {
          af1->duration = level;
        }
    }
    if(IS_NPC(victim) && CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
        MobStartFight(victim, ch);
    }
  }
}

struct judgement_data
{
  int room;
  int data;
  int devotion;
};

void spell_oldjudgement(int level, P_char ch, P_char victim, P_obj obj)
{
  P_char   t, t_next;
  int lev, k, /*minalign, maxalign, dam, */ door, target_room, temp_coor,
    the_room, new_room, i, max_affected;
    
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }

  if(GET_LEVEL(ch) < MINLVLIMMORTAL)
  {
    if(GET_ALIGNMENT(ch) < 900)
    {
      send_to_char("Your god is not pleased with you, and reacts accordingly.", ch);
      die(ch, ch);
      return;
    }
    
    if((GET_ALIGNMENT(ch) > 899) && (GET_ALIGNMENT(ch) < 980))
    {
      send_to_char
        ("You have angered your god in some way, and refuses your request for aid!",
         ch);
      return;
    }
  }
  
  if(victim)
  {
    the_room = victim->in_room;
    temp_coor = victim->specials.z_cord;
  }
  else
  {
    the_room = ch->in_room;
    temp_coor = ch->specials.z_cord;
  }
  
  max_affected = (int) (get_property("spell.judgement.maxAffected", 4.000));
  
  i = 0;
  
  for(t = world[the_room].people; t; t = t_next)
  {
    t_next = t->next_in_room;
    
    if(should_area_hit(ch, t) &&
       !number(0, 2))
    {
      if(GET_ALIGNMENT(t) > -351)
      {
        act("$N is not evil enough to be affected!", TRUE, ch, 0, t, TO_CHAR);
      }
      else 
      {
        if((lev = GET_LEVEL(t)) < (level - 25))
        {                       /* < 26 death */
          die(t, ch);
          continue;
        }
        else if(!resists_spell(ch, t))
        {
          
          if(lev <= (46))      /* <46 para */
          {
            spell_major_paralysis(-level, ch, 0, 0, t, NULL);   /* no save */
          }
          
          if(lev <= (53))      /* <53 blind */
          {
            spell_blindness(-level, ch, 0, 0, t, NULL); /* no save */
          }

          if((GET_RACE(t) == RACE_DEMON) ||
            (GET_RACE(t) == RACE_DEVIL))
          {
            spell_slow(-level, ch, 0, 0, t, NULL); /* nasty spells if we're really evil demons and devils */
            spell_dispel_magic(-level, ch, 0, 0, t, NULL);
          }

          if(!NewSaves(t, SAVING_FEAR, MIN((GET_LEVEL(ch) - GET_LEVEL(t)), 5)))
          {
            door = number(0, NUM_EXITS - 1);
            
            if((CAN_GO(t, door)) &&
              (!check_wall(t->in_room, door)))
            {
              act("The power of your spell sends $N flying out of the room!",
                  FALSE, ch, 0, t, TO_CHAR);
              act("The power of $n's spell sends you flying out of the room!",
                  FALSE, ch, 0, t, TO_VICT);
              act("The power of $n's spell sends $N flying out of the room!",
                  FALSE, ch, 0, t, TO_NOTVICT);
              
              target_room = world[t->in_room].dir_option[door]->to_room;
              char_from_room(t);
              char_to_room(t, target_room, -1);
              
              act("$n flies in, crashing on the floor!", TRUE, t, 0,
                  0, TO_ROOM);
              
              SET_POS(t, POS_PRONE + GET_STAT(t));
              
              stop_fighting(t);
              if( IS_DESTROYING(t) )
                stop_destroying(t);
              if(CAN_ACT(t))
              { 
                Stun(t, ch, PULSE_VIOLENCE * 2, FALSE);
                CharWait(t, PULSE_VIOLENCE * 2);
              }
            }
            else
            {
              act("Your spell sends $N crashing into the wall!", FALSE,
                  ch, 0, t, TO_CHAR);
              act("The power of $n's spell sends you crashing into the wall!",
                  FALSE, ch, 0, t, TO_VICT);
              act("The power of $n's spell sends $N crashing into the wall!",
                  FALSE, ch, 0, t, TO_NOTVICT);
              
              SET_POS(t, POS_SITTING + GET_STAT(t));
              
              stop_fighting(t);
              if( IS_DESTROYING(t) )
                stop_destroying(t);
              
              if(CAN_ACT(t))
              {
                Stun(t, ch, PULSE_VIOLENCE * 2, FALSE);
                CharWait(t, PULSE_VIOLENCE * 3);
              }
            }
          }
          astral_banishment(ch, t, BANISHMENT_HOLY_WORD, level);
        }
        
        i++;
      }

      if(i >= max_affected)
      {
        break;
      }
    }

  }
  CharWait(ch, PULSE_VIOLENCE * 2);
  
}

void event_judgement(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type af;
  struct judgement_data *j = (struct judgement_data *) data;
  
  P_char   tch, sunray_target = NULL;
  int opponents, damage, vict_index, repeats, rnumber;
  bool handled;
  struct damage_messages messages = {
    "$N falls to $S knees screaming in pain and begging everyone to forgive $M.",
    "&+WYour &+wd&+Lar&+wk &+Wsoul screams out in pain as all &+wyour deeds &+Ware revealed!",
    "$N falls to $S knees screaming in pain and begging everyone to forgive $M.",
  };
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }

  if(j->data == 0)
  {
    j->data += 1;
    
    act("&+wAs &+Wheavenly &+Ylight &+wdescends upon the battlefield, both friend and foe stare skyward\n"
        "&+win awe. The tension builds until the very air crackles with &+Yenergy&+w.  A &+Cgust &+wof &+Wdivine\n"
        "&+wwind hurtles down from the &+WHeavens &+wseeking out the impure hearts of &+Levil.&n", FALSE, ch, 0, 0, TO_ROOM);
    act("&+wAs &+Wheavenly &+Ylight &+wdescends upon the battlefield, both friend and foe stare skyward\n"
        "&+win awe. The tension builds until the very air crackles with &+Yenergy&+w.  A &+Cgust &+wof &+Wdivine\n"
        "&+wwind hurtles down from the &+WHeavens &+wseeking out the impure hearts of &+Levil.&n", FALSE, ch, 0, 0, TO_CHAR);

    add_event(event_judgement, PULSE_VIOLENCE / 2, ch, 0, 0, 0, j, sizeof(*j));
    return;
  }

  if(number(0, 1))
  {
    act("&+WBOOO&+wOOOO&+LOOM! &+WDivine &+Ypower &+wsweeps through the room slamming into the ranks of\n"
        "&+wthe &+rfoul&+L-&+rhearted &+Lcreatures&+w, who writhe in agony.&n", FALSE, ch, 0, 0, TO_ROOM);
    act("&+WBOOO&+wOOOO&+LOOM! &+WDivine &+Ypower &+wsweeps through the room slamming into the ranks of\n"
        "&+wthe &+rfoul&+L-&+rhearted &+Lcreatures&+w, who writhe in agony.&n", FALSE, ch, 0, 0, TO_CHAR);
    
    spell_oldjudgement(GET_LEVEL(ch), ch, 0, 0);
    
    return;
  }

  if(ch->in_room != j->room)
  {
    send_to_char("&+WThe forces of light are unable to aid you!\n", ch);
    return;
  }
  
  opponents = 0;
  
  if(!IS_MAGIC_LIGHT(ch->in_room))
  {
    spell_continual_light(50, ch, 0, 0, NULL, NULL);
  }
  
  for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if(ch != tch &&
       !grouped(ch, tch) &&
       GET_ALIGNMENT(tch) < 0)
    {
      opponents++;
      sunray_target = tch;
    }
  }
  
  if(!opponents)
  {
    return;
  }
  
  if(opponents <=  get_property("spell.judgement.maxAffected", 4.000) &&
    should_area_hit(ch, sunray_target))
  {
    spell_holy_word(50, ch, NULL, 0, NULL, NULL);
  }
  else if(IS_FIGHTING(ch))
  {
    spell_sunray(-(GET_LEVEL(ch)), ch, NULL, 0, GET_OPPONENT(ch), 0);
  }
  else if(sunray_target)
  {
    spell_sunray(-(GET_LEVEL(ch)), ch, NULL, 0, sunray_target, 0);
  }
  
  if(!IS_ALIVE(ch))
  {
    return;
  }

  repeats = (opponents > 0) ? (1 + opponents / 3) : 0;

  while(repeats--)
  {
    vict_index = number(1, opponents);
    
    for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(ch != tch &&
        !grouped(ch, tch) &&
        GET_ALIGNMENT(tch) < 0)
      {
        if(--vict_index == 0)
        {
          break;
        }
      }
    }
// its possible (somehow) that the above for() loop is breaking
// because tch is NULL.  When that happens, the below code is crashing
// when the null pointer is dereferenced.  Deal with it:
    if(!tch ||
       IS_TRUSTED(tch))
    {
      continue;
    }
    
    victim = tch;
    
    handled = FALSE;

    damage = dice(20, (int) (GET_LEVEL(ch) * get_property("spell.judgement.dam.modifier", 0.500)));
    
    if(GET_CLASS(victim, CLASS_ANTIPALADIN | CLASS_NECROMANCER) &&
      IS_PC(ch))
    {
      send_to_char
        ("The &+Wholy light&n &+Rbur&+rns&n at your very existence!!!\n", victim);
      
      damage = (int) (damage * get_property("spell.judgement.dam.modifier.apnecro", 1.000));
      
      if(spell_damage(ch, victim, damage, SPLDAM_HOLY, RAWDAM_NOKILL, &messages))
      { 
        if(!IS_ALIVE(ch))
        {
          return;
        }
        
        opponents--;
      }
    }
    else
    {
      if(spell_damage(ch, victim, damage, SPLDAM_HOLY, RAWDAM_NOKILL, &messages))
      { 
        if(!IS_ALIVE(ch))
        {
          return;
        }
        
        opponents--;
      }
    }
    
    if(GET_RACE(victim) == RACE_TROLL)
    {
      if(!NewSaves(victim, SAVING_FEAR, 3))
      {
        bzero(&af, sizeof(af));

        af.type = SPELL_MAJOR_PARALYSIS;
        af.flags = AFFTYPE_SHORT;
        af.duration = (int) (1.5 * PULSE_VIOLENCE);
        af.bitvector2 = AFF2_MAJOR_PARALYSIS;

        affect_to_char(victim, &af);

        send_to_char
          ("You momentarily turn to &+Lstone&n as the &+Wholy light&n shines upon your &+gskin&N!\n",
           victim);
        act
          ("$n momentarily turns to &+Lstone&n as the &+Wholy light&n shines upon $s skin!&N",
           FALSE, victim, 0, 0, TO_ROOM);

        handled = TRUE;
      }
    }
    else if(IS_UNDEADRACE(victim) ||
           IS_PUNDEAD(victim))
    {
      if(!number(0, 2))
      {
        send_to_char
          ("&+WSuddenly a g&+wh&+Wa&+ws&+Wt&+wl&+Wy image appears in front of you, and you begin to remember...\n&n",
           victim);
        send_to_char
          ("Your &+WSOUL&n stares at you: '&+WWhy couldn't my body find rest? Sleep my poor thing.&n'\n",
           victim);
        
        rnumber = number(1, 3);
        
        bzero(&af, sizeof(af));
        
        af.type = SPELL_SLEEP;
        af.flags = AFFTYPE_SHORT;
        af.duration = rnumber * PULSE_VIOLENCE;
        af.bitvector = AFF_SLEEP;
        
        stop_fighting(victim);
        if( IS_DESTROYING(victim) )
          stop_destroying(victim);

        if(GET_STAT(victim) > STAT_SLEEPING)
        {
          act("$n &+Wenters tupor.", TRUE, victim, 0, 0, TO_ROOM);
          SET_POS(victim, GET_POS(victim) + STAT_SLEEPING);
        }
        
        affect_to_char(victim, &af);
        
        StopMercifulAttackers(victim);
        
        handled = TRUE;
      }
    }

    if(!handled)
    {
      if(!number(0, 2))
      {
        act("&+WYou suddenly realize how &+Lwrong &+Wyour life has been!\n"
            "&+WYou feel compelled to change your ways, but the feeling goes away...\n", TRUE, victim, 0, 0, TO_CHAR);
        act("As a ray of &+Wheavenly light&n shines upon $n, $e suddenly relaxes and sinks deeply into thought.&n", 
             TRUE, victim, 0, victim, TO_ROOM);
        
        stop_fighting(victim);
        if( IS_DESTROYING(victim) )
          stop_destroying(victim);
        
        CharWait(victim, PULSE_VIOLENCE * 3);
      }
      else if(!number(0, 2))
      {
        spell_damage(ch, victim, dice(10, GET_LEVEL(ch)), SPLDAM_HOLY, RAWDAM_NOKILL, &messages);
        
        if(!IS_ALIVE(ch))
        {
          return;
        }
        
        SET_POS(victim, POS_SITTING + GET_STAT(victim));
        
        CharWait(victim, PULSE_VIOLENCE * 2);
      }
      else
      {
        send_to_char("&+RJudged and humiliated, you desperately search for &+Wescape...!\n", victim);
        act("As a ray of &+Wheavenly light&n shines upon $n, $e tries to escape &+Wjustice&n.", 
             TRUE, victim, 0, victim, TO_ROOM);
        
        do_flee(victim, 0, 1);
        
        CharWait(victim, PULSE_VIOLENCE * 2);
      }
    }

    if(!is_char_in_room(victim, ch->in_room))
    {
      opponents--;
    }
  }
  
  if((GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5 > number (1, 100)) &&
     IS_ALIVE(ch) &&
     j->devotion < 3)
  {
    j->devotion += 1;
    
    act("&+WThe divine &+Ypower &+wcontinues to sweep through the area!!!&n", FALSE, ch, 0, 0, TO_ROOM);

    add_event(event_judgement, PULSE_VIOLENCE, ch, 0, 0, 0, j, sizeof(*j));
  }
}

void spell_judgement(int level, P_char ch, char *arg, int type, P_char vict, P_obj obj)
{
  struct judgement_data j;
  P_char tch;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }

  if(GET_LEVEL(ch) < MINLVLIMMORTAL)
  {
    if(GET_ALIGNMENT(ch) < 900)
    {
      act("A &+Rblood red&n ray from the &+Wheavens&n strikes $N!!!&n", 
          TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("Your god is not pleased with you, and reacts accordingly.", ch);
      die(ch, ch);
      return;
    }

    if((GET_ALIGNMENT(ch) > 899) && (GET_ALIGNMENT(ch) < 980))
    {
      send_to_char
        ("You have angered your god in some way, and he refuses your request for aid!",
         ch);
      return;
    }
  }

  appear(ch);
  
  for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
// Only one judgement per room for players. 2 Apr 09 -Lucrot
    if(IS_PC(ch) && 
       get_scheduled(tch, event_judgement))
    {
      send_to_char("Your call upon the &+Wforces&n of &+WLight&N fails!\n", ch);
      return;
    }
  }
  
  send_to_char("You call upon the &+Wforces&n of &+WLight&N to aid you.\n", ch);
  act("$n calls upon the &+Wforces&n of &+WLight&N!", FALSE, ch, 0, 0, TO_ROOM);
      
  j.room = ch->in_room;
  j.data = 0;
  j.devotion = 0;
  
  add_event(event_judgement, PULSE_VIOLENCE / 2, ch, 0, 0, 0, &j, sizeof(j));
}

void spell_cure_disease(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int i;

  if(affected_by_spell(victim, SPELL_DISEASE))
  {
    affect_from_char(victim, SPELL_DISEASE);
    send_to_char("You suddenly feel much much better.\n", victim);
    act("$n looks markedly better.", FALSE, victim, 0, 0, TO_ROOM);
  }
  else if(affected_by_spell(victim, SPELL_CONTAGION))
  {
    affect_from_char(victim, SPELL_CONTAGION);
    send_to_char("You suddenly feel much much better.\n", victim);
    act("$n looks markedly better.", FALSE, victim, 0, 0, TO_ROOM);
  }
  else
    send_to_char("There is no noticeable effect.\n", ch);
}

void event_immolate(P_char ch, P_char vict, P_obj obj, void *data)
{
  int dam, burntime, in_room;
  struct damage_messages messages = {
   "&+RF&+rl&+Ram&+Wes &+ysmother $N, &+Lse&+war&+Ling&+y and &+rba&+Rki&+rng&+y $M.",
    "&+RThe fire burns &+Wwhite &+Rhot as it consumes your flesh!",
    "&+RF&+Yir&+We &+Ls&+wea&+Lrs&+y and &+rb&+Rak&+res&n $N!",
    "$N screams in agony as &+Rthe flames&n consume $M completely.",
    "You scream in agony as &+Rthe flames&n consume you completely.",
    "$N screams in agony as &+Rthe flames&n consume $M completely.", 0
  };

  if(!ch)
  {
    logit(LOG_EXIT, "event_immolate called in magic.c with no ch");
    raise(SIGSEGV);
  }
  if(!vict)
  {
    return;
  }
  if(ch && // Just making sure.
    vict)
  {
    if(!IS_ALIVE(ch) ||
      !IS_ALIVE(vict))
    {
      return;
    }
    burntime = *((int *) data);
    if((burntime >= 3 && number(0, burntime)) || burntime == 6)
    {
      act("The &+Yfi&+Rer&+Yy&N &+rconflagration&n subsides!",
        FALSE, ch, 0, vict, TO_VICT);
      if(ch->in_room == vict->in_room)
        act("The &+Yfi&+Rer&+Yy&N &+rconflagration&N burning $N subsides!",
          FALSE, ch, 0, vict, TO_CHAR);
      act("The &+Yfi&+Rer&+Yy&N &+rconflagration&N burning $N subsides!",
        FALSE, ch, 0, vict, TO_NOTVICT);
      return;
    }
    else
    {
      burntime++;
    }
    dam = (int) GET_LEVEL(ch) * 2 + number(4, 20);

    if(burntime >= 3)
      dam = (int) GET_LEVEL(ch) * 2 - number(4, 20);

    if(burntime >= 4)
      dam = (int) GET_LEVEL(ch) + number(4, 12);

    if(IS_ALIVE(vict) &&
      spell_damage(ch, vict, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT, &messages) ==
        DAM_NONEDEAD)
    { 
      if(ch &&
        IS_ALIVE(vict)) // Added another check due to reported double death bug.
      {
        add_event(event_immolate, PULSE_VIOLENCE, ch, vict, NULL, 0,
          &burntime, sizeof(burntime));
      
        if(4 > number(1, 10))
          stop_memorizing(vict);
      }
    }
  }
}

void spell_immolate(int level, P_char ch, char *arg, int type, P_char victim,
     P_obj obj)
{
  int burn = 0;
  if(!ch)
  {
    logit(LOG_EXIT, "spell_immolate called in magic.c with no ch");
    raise(SIGSEGV);
  }
  if(!victim)
  {
    return;
  }
  if(ch && // Just making sure...
    victim)
  {
    if(!IS_ALIVE(ch) || !IS_ALIVE(victim))
    {
      return;
    }
    act("Your &+Yfi&+Rer&+Yy&N blast strikes $N full on!", TRUE, ch, 0, victim,
      TO_CHAR);
    act("A &+Yfi&+Rer&+Yy&N &+rconflagration&N spews from $n striking you full on!",
      FALSE, ch, 0, victim, TO_VICT);
    act("A &+Yfi&+Rer&+Yy&N &+rconflagration&N spews from $n striking $N full on!",
      FALSE, ch, 0, victim, TO_NOTVICT);
    if(ch &&
      victim)
    {
      engage(ch, victim);
    }
    if(IS_ALIVE(victim) &&
      (spell_damage(ch, victim, (int) GET_LEVEL(ch) * 4 + number(4, 20), SPLDAM_FIRE, SPLDAM_NODEFLECT, NULL) == DAM_NONEDEAD));
    {
      if(ch &&
        IS_ALIVE(victim)) // Adding another check.
      {
	//gain_exp(ch, victim, 0, EXP_DAMAGE);
        add_event(event_immolate, PULSE_VIOLENCE, ch, victim, NULL, 0, &burn, sizeof(burn));
      }
    }
  }
}

void event_dread_wave(P_char ch, P_char vict, P_obj obj, void *data)
{
  int    level, dam;
  struct affected_type *af;
  struct damage_messages messages = {
    "A &+Bdark blue, &+Cbitterly icy &+bwave&N flows over $N.",
    "A &+Bdark blue, &+Cbitterly icy&N &+bwave&N flows over you, choking the breath from your lungs.",
    "A &+Bdark blue, &+Cbitterly icy&N &+bwave&N flows over $N.",
    "A &+Bdark blue, &+Cbitterly icy&N &+bwave&N freezes $N to death!",
    "A &+Bdark blue, &+Cbitterly icy&N &+bwave&N freezed you to the core...",
    "A &+Bdark blue, &+Cbitterly icy&N &+bwave&N flows over $N, freezing $M to &=LRdeath!&n&n", 0
  };

  if(!IS_ALIVE(ch) || !IS_ALIVE(vict))
    return;
 
  level = *((int *) data);

  for(af = vict->affected; af && af->type != SPELL_DREAD_WAVE; af = af->next);
  
  if(af == NULL)
    return;
    
  if((af->modifier)-- == 0)
  {
    send_to_char("&+bThe wave slowly dissipates.&N\n", vict);
    act("The &+cchilling wave &ncovering $N recedes and &+cv&+Ba&+cp&+Bo&+cr&+Bi&+cz&+Be&+cs...&n",
        FALSE, ch, 0, vict, TO_CHAR);
    act("The &+cchilling wave &ncovering $N &+cv&+Ba&+cp&+Bo&+cr&+Bi&+cz&+Be&+cs...&n",
      FALSE, ch, 0, vict, TO_NOTVICT);
    affect_from_char(vict, SPELL_DREAD_WAVE);
    return;
  }

  dam = MAX(1, (int)((af->modifier+ 2) * level + number(-60, 0)));
  
  dam = (int)(dam * GET_LEVEL(ch) / 56);
  
  if(NewSaves(vict, SAVING_SPELL, 0) &&
     NewSaves(vict, SAVING_BREATH, 0))
      dam = (int)(dam * 0.66);
  
  if(number(0, 2) &&
     resists_spell(ch, vict))
  {
    dam = 1;
  }

  if(spell_damage(ch, vict, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages)
    == DAM_NONEDEAD)
  {
    add_event(event_dread_wave, PULSE_VIOLENCE, ch, vict, NULL, 0, &level,
      sizeof(level));
    if(!number(0, 9) &&
       !IS_GREATER_RACE(vict) &&
       !IS_ELITE(vict))
          StopCasting(vict);
  }  
}

void spell_dread_wave(int level, P_char ch, char *arg, int type, P_char vict,
                      P_obj obj)
{
  struct affected_type *af;

  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(vict))
  {
    return;
  }
  
  act("A &+Ldark, &+Cfreezing cold &+bwave&N sent by $n slowly covers $N making $M &+Lch&Nok&+Le &Nand freeze.",
    TRUE, ch, 0, vict, TO_NOTVICT);
  act("A &+Ldark, &+Cfreezing cold &+bwave&N sent by $n slowly covers you making it hard to &+Lbr&Nea&+Lth&Ne.",
    FALSE, ch, 0, vict, TO_VICT);
  act("You send a &+Ldark, &+Cbitterly cold &+bwave&N which slowly covers $N making $M &+Cfreeze and &+Lch&Nok&+Le.&N",
    FALSE, ch, 0, vict, TO_CHAR);
    
  if(GET_RACE(vict) == RACE_W_ELEMENTAL)
  {
    send_to_char("&+CThe wave has no effect on &+Bwater elementals!\r\n", ch);
    return;
  }

  if((af =(struct affected_type *) get_spell_from_char(vict, SPELL_DREAD_WAVE)) == NULL)
  {
    struct affected_type new_affect;

    af = &new_affect;
    memset(af, 0, sizeof(new_affect));
    af->type = SPELL_DREAD_WAVE;
    
    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_WATER))
    {
      af->modifier = number(1, 3);
    }
    else
      af->modifier = 0;
      
    af->duration = 1;
    af->modifier += 2;
    affect_to_char(vict, af);
    add_event(event_dread_wave, 0, ch, vict, NULL, 0, &level, sizeof(level));
  }
  else
  {
    send_to_char("&+BYour spell augments the &+Ldread waves&n &+Bcovering your victim!\r\n", ch);
    send_to_char("&+BThe bitterly icy flow surrounding you becomes more frigid!\r\n", vict);
    
    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_WATER))
    {
      af->modifier += 2;
    }
    else
      af->modifier += 1;
  }
}

void event_magma_burst(P_char ch, P_char vict, P_obj obj, void *data)
{
  int      level, dam;
  struct affected_type *af;
  struct damage_messages messages = {
    "&+RThe fire burns hot as it consumes $N&+R's flesh!",
    "&+RThe fire burns hot as it consumes your flesh!",
    0,
    "$N screams in agony as &+Rthe flames&n consume $M completely.",
    "You scream in agony as &+Rthe flames&n consume you completely.",
    "$N screams in agony as &+Rthe flames&n consume $M completely.", 0
  };

  if( !IS_ALIVE(vict) )
  {
    return;
  }

  level = *((int *) data);

  for( af = vict->affected; af && af->type != SPELL_MAGMA_BURST; af = af->next )
  {
    ;
  }

  if( af == NULL )
  {
    return;
  }

  if((af->modifier)-- == 0)
  {
    send_to_char("&+RThe flames engulfing your body subside.\n", vict);
    affect_from_char(vict, SPELL_MAGMA_BURST);
    return;
  }

  dam = 3 * level + number(0, 30);

  if( spell_damage(ch, vict, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD)
  {
    add_event(event_magma_burst, PULSE_VIOLENCE, ch, vict, NULL, 0, &level, sizeof(level));
    if( 5 > number(1, 10) )
    {
      stop_memorizing(vict);
    }
  }
}

void spell_magma_burst(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type *af;

  if((af = get_spell_from_char(victim, SPELL_MAGMA_BURST)) == NULL)
  {
    act("Two pillars of &+Wwhite flame &Nleap from $n's outstreached palms enveloping $N in a &+Rfi&+rery in&+Rferno&N.", TRUE, ch, 0, victim, TO_NOTVICT);
    act("Two pillars of &+Wwhite flame &Nleap from $n's outstreached palms enveloping you in a &+Rfi&+rery in&+Rferno&N.", TRUE, ch, 0, victim, TO_VICT);
    act("Two pillars of &+Wwhite flame &Nleap from your outstreached palms enveloping $N in a &+Rfi&+rery in&+Rferno&N.", TRUE, ch, 0, victim, TO_CHAR);

    struct affected_type new_affect;

    af = &new_affect;
    memset(af, 0, sizeof(new_affect));
    af->type = SPELL_MAGMA_BURST;
    af->duration = 1;
    af->modifier = 3;
    affect_to_char(victim, af);
    add_event(event_magma_burst, 0, ch, victim, NULL, 0, &level, sizeof(level));

    //if( IS_ALIVE(ch) && IS_ALIVE(victim) )
    //  gain_exp(ch, victim, 0, EXP_DAMAGE);
  }
  else
  {
    act("Two pillars of &+Wwhite flame &Nleap from $n's outstreached palms, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.", TRUE, ch, 0, victim, TO_NOTVICT);
    act("Two pillars of &+Wwhite flame &Nleap from $n's outstreached palms, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.", TRUE, ch, 0, victim, TO_VICT);
    act("Two pillars of &+Wwhite flame &Nleap from your outstreached palms, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.", TRUE, ch, 0, victim, TO_CHAR);
    af->modifier = ( af->modifier < 3 ) ? 3 : af->modifier + 1;
  }
}

void spell_single_cdoom_wave(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "&+LYou send &+ma wave of &+Linsects &+mand &+Larachnids &+magainst $N!",
    "&+mA sea of &+Larachnids &+mand &+Linsects &+mconsume and overwhelm you!",
    "&+mA sea of &+Larachnids &+mand &+Linsects &+mconsume and overwhelm $N!",
    "&+mA sea of &+Linsects &+mand &+Larachnids &+mconsumed $N &+mcompletely "
      "leaving nothing but a few &+Wbones&+L..",
    "&+LYou suffer a terrible death as &+ma sea of &+Linsects &+mand &+Larachnids "
      "&+mdevours you &+Lalive...",
    "&+mA sea of &+Linsects &+mand &+Larachnids &+mconsumed $N &+mcompletely "
      "leaving nothing but a few &+Wbones&+L..", 0
  };

  if(!IS_ALIVE(ch))
    return;

  int doomdam = 40 + level + number(0, 20);

  switch(world[victim->in_room].sector_type)
  {
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_NO_GROUND:
  case SECT_UNDRWLD_NOGROUND:
  case SECT_UNDERWATER:
  case SECT_FIREPLANE:
  case SECT_UNDERWATER_GR:
  case SECT_OCEAN:
  case SECT_UNDRWLD_LIQMITH:
  case SECT_LAVA:
    doomdam -= 20;
    break;
  case SECT_INSIDE:
  case SECT_UNDRWLD_INSIDE:
    doomdam -= 10;
    break;
  /*case SECT_CITY:
  case SECT_UNDRWLD_CITY:
  case SECT_ROAD:
    doomdam += 10;
    break;*/
  case SECT_HILLS:
  case SECT_MOUNTAIN:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_FIELD:
  case SECT_DESERT:
  case SECT_UNDRWLD_WILD:
    doomdam += 10;
    break;
  case SECT_FOREST:
  case SECT_SWAMP:
  case SECT_UNDRWLD_SLIME:
  case SECT_UNDRWLD_MUSHROOM:
    doomdam += 20;
    break;
  default:
    break;
  }

  if(get_spell_from_room(&world[victim->in_room], SPELL_SUMMON_INSECTS))
    doomdam += 20;

  if(GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND)) 
    doomdam += 20;

  if(IS_AFFECTED3(victim, AFF3_COLDSHIELD) ||
      IS_AFFECTED2(victim, AFF2_FIRESHIELD) ||
      IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
  {
    doomdam = (int) (doomdam * 0.75);
  }

  if (arg)  // area
  {
    if (IS_PC(ch) && IS_PC(victim))
      doomdam = doomdam * get_property("spell.area.damage.to.pc", 0.5);
  } 
  else  // single target, stays with target
  {
    doomdam = doomdam * 1.20;
  }
  
  doomdam = doomdam * get_property("spell.area.damage.factor.creepingDoom", 1.000);


  spell_damage(ch, victim, doomdam, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
}

struct CDoomData
{
  int level;
  int waves;
  int area;
};

void event_cdoom(P_char ch, P_char victim, P_obj obj, void *data)
{
  CDoomData *cDoomData = (CDoomData*) data;

  if (!cDoomData->area)
  {
    if( !IS_ALIVE(victim) || victim->in_room <= 0 )
    {
      cDoomData->waves = 0;
    }
  }

  if(cDoomData->waves == 0)
  {
    act("&+LThe sea of &+minsects &+Land &+marachnids &+Lfades away...",
      FALSE, ch, 0, victim, TO_CHAR);
    if (!cDoomData->area)
    {
      act("&+LThe sea of &+minsects &+Land &+marachnids &+Lfades away...",
	      FALSE, ch, 0, victim, TO_VICT);
    }
    else
    {
      act("&+LThe sea of &+minsects &+Land &+marachnids &+Lfades away...",
    	  FALSE, ch, 0, victim, TO_ROOM);
    }
    return;
  }
  else
    cDoomData->waves--;

  if (cDoomData->area)
  {
    act("&+LA wave of &+minsects &+Land &+marachnids &+Lcrawls about the area...", FALSE, ch, 0, victim, TO_CHAR);
    act("&+LA wave of &+minsects &+Land &+marachnids &+Lcrawls about the area...", FALSE, ch, 0, victim, TO_ROOM);
  }
  else
  {
    //act("&+LA wave of &+marachnids&+L crawls about $N...", FALSE, ch, 0, victim, TO_CHAR);
    //act("&+LA wave of &+marachnids&+L crawls about $N...", FALSE, victim, 0, victim, TO_ROOM);
    //act("&+LA wave of &+marachnids&+L crawls about you...", FALSE, ch, 0, victim, TO_VICT);
  }
  // if doom is single-target, replace with direct call to spell_single_cdoom_wave
  if (cDoomData->area)
  {
    cast_as_damage_area(ch, spell_single_cdoom_wave, cDoomData->level, victim,
      get_property("spell.area.minChance.creepingDoom", 50),
      get_property("spell.area.chanceStep.creepingDoom", 20));
    add_event(event_cdoom, PULSE_VIOLENCE, ch, 0, NULL, 0, cDoomData, sizeof(CDoomData));
  }
  else
  {
    spell_single_cdoom_wave(cDoomData->level, ch, 0, 0, victim, obj);
    if (IS_ALIVE(victim))
    {
      add_event(event_cdoom, PULSE_VIOLENCE, ch, victim, NULL, 0, cDoomData, sizeof(CDoomData));
    }
  }

}

bool has_scheduled_area_doom( P_char ch )
{
  P_nevent e;

  LOOP_EVENTS_CH( e, ch->nevents )
  {
    if( e->func == event_cdoom && ((CDoomData*)(e->data))->area )
    {
      return TRUE;
    }
  }

  return FALSE;
}

void spell_cdoom(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  // If ch already has an AREA doom going.. fail.
  if( has_scheduled_area_doom(ch) && !victim )
  {
    send_to_char( "You are already controlling all the bugs in the area!\n", ch );
    return;
  }

  if (victim)
  {
    if (!is_char_in_room(victim, ch->in_room))
    {
      send_to_char("Your victim is no longer here.\r\n", ch);
      return;
    }
    if (!CAN_SEE(ch, victim))
    {
      send_to_char("You cannot see your victim.\r\n", ch);
      return;
    }
  }

  CDoomData cDoomData;
  cDoomData.waves = number(4, 5);
  cDoomData.level = level;
  cDoomData.area = victim ? 0 : 1;

  /* either this or damage bonus in single wave
  if(GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND) ||
    (world[ch->in_room].sector_type == SECT_FOREST))
      cDoomData.waves++;*/

  act("&+LA &+gpl&+Lag&+gue &+Lof &+minsects and arachnids&+L flow like an ocean.", TRUE, ch, 0, victim, TO_ROOM);
  act("&+LYou send out a &+mwave of &+Linsects &+mand &+Larachnids&+m!", TRUE, ch, 0, victim, TO_CHAR);

  //engage(ch, victim);
  if (victim)
    add_event(event_cdoom, 0, ch, victim, NULL, 0, &cDoomData, sizeof(CDoomData));
  else
/* Moving doom back to a wave spell. - Lohrr
    {
        cast_as_damage_area(ch, spell_single_doom_aoe, level, victim,
                      get_property("spell.area.minChance.creepingDoom", 50),
                      get_property("spell.area.chanceStep.creepingDoom", 20));

    }
*/
	{
    add_event(event_cdoom, 0, ch, 0, NULL, 0, &cDoomData, sizeof(CDoomData));
    zone_spellmessage(ch->in_room,
      "&+LThe &+minsects &+Lof the &+yarea&+L seem to be called away...&n\r\n",
      "&+LThe &+minsects &+Lof the &+yarea&+L seem to be called away to the %s...&n\r\n");
	}
}

void spell_single_doom_aoe(int level, P_char ch, char *args, int type,
                              P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "", "", "",
    "$N's body turns to &+Wbone&n as the &+yinsects&n consume $M.",
    "Your body succumbs to the overwhelming &+yplague&n of &+ginsects&n.",
    "$N's body turns to &+Wbone&n as the &+yinsects&n consume $M.", 0
  };
  dam = 110 + level * 3 + number(1, 10);


  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_sense_follower(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!IS_AFFECTED4(victim, AFF4_SENSE_FOLLOWER))
  {
    send_to_char("&+LYou feel your awareness improve.\n", victim);

    bzero(&af, sizeof(af));
    af.type = SPELL_SENSE_FOLLOWER;
    af.duration = level;
    af.bitvector4 = AFF4_SENSE_FOLLOWER;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if(af1->type == SPELL_SENSE_FOLLOWER)
      {
        af1->duration = level;
      }
  }
}


void spell_pass_without_trace(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  struct affected_type af;

  if(IS_AFFECTED3(victim, AFF3_PASS_WITHOUT_TRACE))
    return;

  if(!affected_by_spell(victim, SPELL_PASS_WITHOUT_TRACE))
  {
    send_to_char("&+gThe world seems to open new paths for you.&n\n", victim);

    bzero(&af, sizeof(af));
    af.type = SPELL_PASS_WITHOUT_TRACE;
    af.duration = 10;
    af.bitvector3 = AFF3_PASS_WITHOUT_TRACE;
    affect_to_char(victim, &af);
  }
  else
  {
    send_to_char("Nothing seems to happen.\n", ch);
  }
}


void spell_starshell(int level, P_char ch, char *arg, int type, P_char vict, P_obj obj)
{
  char     a[64];
  struct room_affect af;

  if(!OUTSIDE(ch) ||
     !NORMAL_PLANE(ch->in_room))
  {
    send_to_char("This magic won't work here!\n", ch);
    return;
  }

  if(get_spell_from_room(&world[ch->in_room], SPELL_STARSHELL))
  {
    send_to_room("&+WThe &+Yblazing&N&+W shell of light overhead &+REXPLODES&n!\n", ch->in_room);
    af.duration = (120 + ((level - 46) * 8));
    return;
  }
  
  memset(&af, 0, sizeof(af));
  af.type = SPELL_STARSHELL;
  af.duration = (120 + ((level - 46) * 8));
  affect_to_room(ch->in_room, &af);

  send_to_room("&+WA &+Yblazing&N&+W shell of light appears overhead!\n", ch->in_room);
  return;
}

void spell_vampire(int level, P_char ch, char *arg, int type, P_char vict, P_obj obj)
{
  struct affected_type af, *afp;
  struct follow_type *foll, *next_foll;
  P_char tch;
  int circle;

  // Old pets no longer follow orders but still take same damage as PCs
  // and count towards pet limit to avoid cheesing: doesn't apply to mobs.
  if( !IS_ALIVE(ch) )
  {
    return;
  }
  if( IS_PC(ch) )
  {
   for( foll = ch->followers; foll; foll = next_foll )
   {
    next_foll = foll->next;
    tch = foll->follower;
    if( IS_PC_PET(tch) )
    {
      stop_follower(tch);
      setup_pet(tch, ch, 2, PET_NOORDER);
      //force pets to die after 30 minutes when trance is cast, to remove the pet links
      add_event(event_pet_death, 1800, tch, NULL, NULL, 0, NULL, 0);
    }
   }
  }

  if (GET_CLASS(ch, CLASS_THEURGIST))
  {
    act("&+WF&+Yea&+Wth&+Ye&+Wr&+Ye&+Wd w&+Yi&+Wngs&n sprout from your back and your &+Ysk&+yi&+Yn&n and &+Ye&+yy&+Yes&n gain a &+Ygolden", FALSE, ch, 0, 0, TO_CHAR);
    act("&+Yhu&+ye&n. You can feel the &+Wholy &+Rpower&n of &+Cr&+ci&+Cght&+ceou&+Csn&+ce&+Css&n flowing through you!", FALSE, ch, 0, 0, TO_CHAR);
    act("&+WF&+Yea&+Wth&+Ye&+Wr&+Ye&+Wd w&+Yi&+Wngs&n sprout from $n's back and their &+Ysk&+yi&+Yn&n and &+Yey&+ye&+Ys&n gain a", FALSE, ch, 0, 0, TO_ROOM);
    act("&+Ygolden hu&+ye&n. They are surrounded with a &+Wholy &+Yglow&n of &+Cr&+ci&+Cght&+ceou&+Cs &+Rpower&n!", FALSE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    send_to_char("You take on the shape of undead...\n", ch);
    act("A chill passes by as all the color drains from $n.", TRUE, ch, 0, 0, TO_ROOM);
  }

  if(affected_by_spell(ch, SPELL_VAMPIRE) || affected_by_spell(ch, SPELL_ANGELIC_COUNTENANCE))
  {
    for( afp = ch->affected; afp; afp = afp->next )
    {
      if(afp->type == SPELL_VAMPIRE || afp->type == SPELL_ANGELIC_COUNTENANCE)
      {
        afp->duration = 10;
      }
    }
    return;
  }

  bzero(&af, sizeof(af));
  af.type = (GET_CLASS(ch, CLASS_THEURGIST) ? SPELL_ANGELIC_COUNTENANCE : SPELL_VAMPIRE);
  af.duration = 10;
  af.modifier = (get_property("stats.str.Vampire", 100) - 100);
  if( af.modifier != 0 )
  {
    af.location = APPLY_STR_MAX;
    affect_to_char(ch, &af);
  }
  af.modifier = (get_property("stats.con.Vampire", 100) - 100);
  if( af.modifier != 0 )
  {
    af.location = APPLY_CON_MAX;
    affect_to_char(ch, &af);
  }
  af.location = APPLY_HITROLL;
  af.modifier = 15;
  affect_to_char(ch, &af);
  af.location = APPLY_DAMROLL;
  af.modifier = 10;
  af.bitvector2 = AFF2_VAMPIRIC_TOUCH;
  af.bitvector4 = AFF4_VAMPIRE_FORM;
  affect_to_char(ch, &af);


  if( !USES_SPELL_SLOTS(ch) )
  {
    return;
  }

  // The code below makes sure necro gets only as much assim slots as he had memorized spells.
  // If we ever remove ability of assimilating from tranced necros - junk it.
  for( circle = 0; circle <= MAX_CIRCLE; circle++ )
  {
    ch->specials.undead_spell_slots[circle] = 0;
  }

  for (afp = ch->affected; afp; afp = afp->next)
  {
    if(afp->type == TAG_MEMORIZE && (afp->flags & MEMTYPE_FULL))
    {
      ch->specials.undead_spell_slots[get_spell_circle(ch, afp->modifier)]++;
    }
  }
}

void spell_cloak_of_fear(int level, P_char ch, char *arg, int type,
                         P_char vict, P_obj obj)
{
  P_char   tch;

  if(!ch || !IS_ALIVE(ch))
  {
    return;
  }
  
  appear(ch);
  act("&+LThe very essence of death flows out from you...&N", 0, ch, 0, 0, TO_CHAR);
  act("&+L$n assumes a terrifying visage of death!", 1, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = tch)
  {
    tch = vict->next_in_room;
    if(vict == ch)
      continue;
    if(IS_GREATER_RACE(vict) ||
      IS_TRUSTED(vict) ||
      IS_ELITE(vict) ||
      IS_UNDEADRACE(vict) ||
      !IS_ALIVE(vict))
    {
      continue;
    }
    if(should_area_hit(ch, vict) &&
      !NewSaves(vict, SAVING_FEAR, (int) (level/11 - 2)) &&
      !fear_check(vict))
    {
      do_flee(vict, 0, 1);
    }
  }
}

struct apoc_data
{
  P_char   victim;
  int      stage;
  int      level;
  int      room;
  int      next_affect;
};

void event_apocalypse(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct apoc_data *d = (struct apoc_data *) data;
  P_char   vict, tch;
  int max_affected, i, opponents;
  
  struct damage_messages d_messages = {
    "$N screams in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs $M.",
    "You scream in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs you!",
    "$N screams in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs $M.",
    "$N screams in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n consumes $M completely!",
    "You scream in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs you completely!",
    "$N screams in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs $M completely!.",
      0
  };
  struct damage_messages p_messages = {
    "$N's face turns &+gg&+Lr&+ge&+Le&+gn&n as a &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon $M.",
    "A &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon you making you &+gc&+Lh&+go&+Lk&+ge&n and writhe in pain!",
    "$N's face turns &+gg&+Lr&+ge&+Le&+gn&n as a &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon $M.",
    "$N's dies screaming as a &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon $M.",
    "A &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon you and never lifts again..",
    "$N's dies screaming as a &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon $M."
  };

  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }
  
  if(d->next_affect == 4 ||
     ch->in_room != d->room)
  {
    send_to_room
      ("&+LAs the powerful summoning fades the r&+wi&+Ld&+we&+Lr&+ws &+Lof &+rh&+Le&+rl&+Ll return to the Abyss.&n\n\n",
       ch->in_room);
    
    return;
  }
  else if(d->stage == 0)
  {
    send_to_room("&+LA distant &+wroar &+Lcan be heard from the skies...\n\n",
                 ch->in_room);
                 
    zone_spellmessage(ch->in_room,
      "&+LA distant &+wroar &+Lcan be heard off in the distance...\n\n",
      "&+LA distant &+wroar &+Lcan be heard off from the %s...\n\n");

    d->stage++;
    
    add_event(event_apocalypse, (int) (PULSE_VIOLENCE / 2), ch, 0, 0, 0, d, sizeof(*d));

    return;
  }
  else if(d->stage == 1)
  {
    send_to_room
      ("&+L   ...the r&+wi&+Ld&+we&+Lr&+ws &+Lof &+rh&+Le&+rl&+Ll broke free again!\n\n",
       ch->in_room);
    
    d->stage++;
    
    add_event(event_apocalypse, (int) (PULSE_VIOLENCE / 2), ch, 0, 0, 0, d, sizeof(*d));
    
    return;
  }

  vict = d->victim;
  
  if(!vict ||
     !is_char_in_room(vict, ch->in_room))
  {
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(ch != tch &&
         should_area_hit(ch, tch) &&
         !number(0, 1))
      {
        vict = tch;
        d->victim = tch;
      }
    }
  }
  
// End event_apocalypse right now. We do not have a valid vict.
  if(!vict ||
     vict->in_room != ch->in_room)
  {
    send_to_room
      ("&+LAs the powerful summoning fades the r&+wi&+Ld&+we&+Lr&+ws &+Lof &+rh&+Le&+rl&+Ll return to the Abyss.\n",
       ch->in_room);
    return;
  }
  
  max_affected = (int) (get_property("spell.apocalypse.maxAffected", 4.000));
  
  if(d->next_affect == 0)
  {
    send_to_room
      ("&+LThe Horseman of D&+We&+La&+Wt&+Lh appears in the sky overhead!\n",
       ch->in_room);

    act("&+LThe Horseman of &+LD&+We&+La&+Wt&+Lh cackles and swings a &+wmassive &+Wdeadly &+Lscythe &+Lat $n!",
      TRUE, vict, 0, 0, TO_ROOM);
    act("&+LThe Horseman of &+LD&+We&+La&+Wt&+Lh cackles and swings a &+wmassive &+Wdeadly &+Lscythe &+Lat $N!",
      TRUE, vict, 0, 0, TO_CHAR);
    act("&+LThe Horseman of &+LD&+We&+La&+Wt&+Lh cackles and swings a &+wmassive &+Wdeadly &+Lscythe &+Lat you!",
      TRUE, vict, 0, 0, TO_VICT);
      
    if(!IS_MAGIC_DARK(ch->in_room))
    {
      spell_darkness(50, ch, 0, 0, NULL, NULL);
    }
    
    if(!number(0, 2) &&
       should_area_hit(ch, vict))
    {
      spell_cloak_of_fear(40, ch, 0, 0, NULL, 0);
    }
    
    if(ch->in_room == vict->in_room)
    {
      spell_damage(ch, vict, dice(20, 30), SPLDAM_PSI, 0, &d_messages);
    }
    
    if(!IS_ALIVE(ch))
    {
      return;
    }
    
    d->next_affect++;
  }
  else if(d->next_affect == 1)
  {
    send_to_room("The Horseman of &+RW&+ra&+Rr&N appears in the skies!\n",
                 ch->in_room);
    
    i = 0;
    
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(should_area_hit(ch, tch) &&
        !number(0, 2) &&
        !affected_by_spell(tch, SKILL_BERSERK))
      {
        if(NewSaves(tch, SAVING_SPELL, 2))
        {
          continue;
        }
        
        act("&+LThe Horseman of &+RW&+ra&+Rr &+Lglares with &+renraged &+Reyes &+Lat $n!&N",
          TRUE, tch, 0, 0, TO_ROOM);
        act("&+LThe Horseman of &+RW&+ra&+Rr &+Lglares with &+renraged &+Reyes &+Lat $N!",
          TRUE, tch, 0, 0, TO_CHAR);
        act("&+LThe Horseman of &+RW&+ra&+Rr &+Lglares with &+renraged &+Reyes &+Lat you!",
          TRUE, tch, 0, 0, TO_VICT);
        
        berserk(tch, 1 * PULSE_VIOLENCE);
        
        i++;
        
        if(i >= max_affected)
        {
          break;
        }
      }
    }
    
    opponents = 0;
    
    for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(ch != tch &&
         !grouped(ch, tch))
      {
        opponents++;
      }
    }
    
    if(opponents <=  get_property("spell.apocalypse.maxAffected", 4.000) &&
      should_area_hit(ch, vict))
    {
      spell_incendiary_cloud(25, ch, NULL, 0, vict, NULL);
    }
    else
    {
      spell_immolate(GET_LEVEL(ch), ch, NULL, 0, vict, NULL);
    }
    
    if(!IS_ALIVE(ch))
    {
      return;
    }
    
    d->next_affect++;
  }
  else if(d->next_affect == 2)
  {
    send_to_room
      ("&+LThe Horseman of &+yF&+Ya&+ym&+Yi&+yn&+Ye &+Lappears in the sky overhead...\n",
       ch->in_room);
    
    i = 0;
    
    for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(should_area_hit(ch, tch) &&
         !number(0, 2) &&
         !IS_STUNNED(tch))
      {
      
        act("The Horseman of &+yF&+Ya&+ym&+Yi&+yn&+Ye&N glares around!",
          TRUE, tch, 0, 0, TO_ROOM);
        act("The Horseman of &+yF&+Ya&+ym&+Yi&+yn&+Ye&N glares at $N!",
          TRUE, tch, 0, 0, TO_CHAR);
        
        act("&+LThe Horseman of &+yF&+Ya&+ym&+Yi&+yn&+Ye &+Lglares at you with &+rdeathly g&+Rl&+wo&+Ww&+wi&+Rn&+rg eyes&L&+Lcausing you to lose your &+Yconcentration.", FALSE, tch, 0, ch, TO_CHAR);
           
        Stun(tch, ch, PULSE_VIOLENCE * 1, TRUE);
        
        StopCasting(tch);
        
        stop_memorizing(tch);
        
        i++;
        
        if(i >= max_affected)
        {
          break;
        }
      }
    }
    d->next_affect++;
  }
  else if(d->next_affect == 3)
  {
    send_to_room
      ("The Horseman of &+gP&+Le&+gs&+Lt&+gi&+Ll&+ge&+Ln&+gc&+Le&N appears in the skies!\n",
       ch->in_room);
       
    i = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(should_area_hit(ch, tch) &&
         !number(0, 2))
      {
        act("The Horseman of &+gP&+Le&+gs&+Lt&+gi&+Ll&+ge&+Ln&+gc&+Le&N breaths on $n!&N",
          TRUE, tch, 0, 0, TO_ROOM);
        act("The Horseman of &+gP&+Le&+gs&+Lt&+gi&+Ll&+ge&+Ln&+gc&+Le&N breaths on $N!&N",
          TRUE, tch, 0, 0, TO_CHAR);
        act("The Horseman of &+gP&+Le&+gs&+Lt&+gi&+Ll&+ge&+Ln&+gc&+Le&N chokes you with its\n&+ywretched breath!",
          TRUE, tch, 0, 0, TO_VICT);
          
        if(!affected_by_spell(tch, SPELL_WITHER) &&
           !number(0, 2))
        {
          spell_wither(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, tch, 0);
        }
        
        if(!number(0, 2))
        {
          poison_weakness(GET_LEVEL(ch), ch, 0, 0, tch, 0);
        }
        
        if(!affected_by_spell(tch, SPELL_DISEASE) &&
           !number(0, 2))
        {
          spell_disease(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, tch, 0);
        }
        
        spell_damage(ch, tch, 3 * GET_LEVEL(ch), SPLDAM_GAS, 0, &p_messages);
         
        if(!IS_ALIVE(ch))
        {
          return;
        }
        
        i++;
        
        if(i >= max_affected)
        {
          break;
        }
      }
    }
    
    d->next_affect++;
  }

  else if(d->next_affect == 4)
  {
    i = 0;
    
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(should_area_hit(ch, tch) &&
        !number(0, 2))
      {
        astral_banishment(ch, tch, BANISHMENT_UNHOLY_WORD, d->level);
      }
      
      i++;
      
      if(i >= max_affected)
      {
        break;
      }
    }
// Setting stage 3 will exit from routine after the next event_apoc.
    d->stage = 3;
  }    

  if(IS_ALIVE(ch) &&
     d->next_affect <= 4)
  {
    add_event(event_apocalypse, PULSE_VIOLENCE, ch, 0, 0, 0, d, sizeof(*d));
  }
}

// d.stage controls messages. Stages 0 and 1 have messages.
// d.next_affect controls spells and only functions while d.stage is 2. 

void spell_apocalypse(int level, P_char ch, char *arg, int type, P_char vict, P_obj obj)
{
  struct apoc_data d;
  P_char tch;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }
  
  appear(ch);
  
  for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
// Only one apoc per room for players. 2 Apr 09 -Lucrot
    if(IS_PC(ch) && 
       get_scheduled(tch, event_apocalypse))
    {
      send_to_char("Your call upon the &+Lforces of darkness&N fails!\n", ch);
      return;
    }
  }
  
  send_to_char("You call upon the &+Lforces of darkness&N to aid you.\n", ch);
  act("$n calls upon the &+Lforces of darkness&N!", FALSE, ch, 0, 0, TO_ROOM);
  d.stage = 0;
  d.victim = vict;
  d.level = level;
  d.room = ch->in_room;
  d.next_affect = 0;

  add_event(event_apocalypse, (int) (PULSE_VIOLENCE / 2), ch, 0, 0, 0, &d, sizeof(d));
}

void spell_ether_sense(int level, P_char ch, char *arg, int type, P_char vict,
                       P_obj obj)
{
  int      elevel, ilevel, glevel;
  P_desc   d;
  char     buf[256];


  if(!IS_ILLITHID(ch) || !IS_PILLITHID(ch))
  {
    send_to_char
      ("A flood of strange images stream into your brain at a mind-numbing pace..  Woah man, the colors.  Alas, you can't make sense of any of it.\n",
       ch);
    return;
  }

  d = descriptor_list;
  elevel = ilevel = glevel = 0;
  while (d)
  {
    if(!d->connected &&
        (world[ch->in_room].zone == world[d->character->in_room].zone) &&
        !(IS_SET(world[ch->in_room].room_flags, GUILD_ROOM)))
    {
      /*found char in same zone */
      if((GET_LEVEL(d->character) > 25) && !IS_TRUSTED(d->character))
      {
        if(IS_ILLITHID(d->character) || IS_PILLITHID(ch))
        {
          ilevel += GET_LEVEL(d->character);
        }
        else
        {
          if(EVIL_RACE(d->character))
          {
            elevel += GET_LEVEL(d->character);
          }
          else
            glevel += GET_LEVEL(d->character);
        }
      }
    }
    d = d->next;
  }

  /* don't show them exact level.. */
/*
  if(elevel) elevel = 230;
  if(glevel) glevel = 230;
  if(ilevel) ilevel = 230;
*/
  if(EVIL_RACE(ch))
  {
    if(glevel == 0)
    {
      sprintf(buf,
              "&+WYou detect no good presence in the ether around you.\n");
    }
    else if(glevel < 100)
    {
      sprintf(buf,
              "&+WYou detect a good presence in the ether around you.\n");
    }
    else if(glevel < 250)
    {
      sprintf(buf,
              "&+WYou detect a good presence in the ether around you.\n");
    }
    else
    {
      sprintf(buf,
              "&+WYou detect a good presence in the ether around you.\n");
    }
    send_to_char(buf, ch);
  }
  if(!IS_ILLITHID(ch) && !IS_PILLITHID(ch))
  {
    if(ilevel == 0)
    {
      sprintf(buf,
              "&+mYou detect no planar presence in the ether around you.\n");
    }
    else if(ilevel < 50)
    {
      sprintf(buf,
              "&+mYou detect planar presence in the ether around you.\n");
    }
    else if(ilevel < 100)
    {
      sprintf(buf,
              "&+mYou detect planar presence in the ether around you.\n");
    }
    else
    {
      sprintf(buf,
              "&+mYou detect planar presence in the ether around you.\n");
    }
    send_to_char(buf, ch);
  }
  if((!EVIL_RACE(ch)) || IS_ILLITHID(ch))
  {                             /* show evils */
    if(elevel == 0)
    {
      sprintf(buf,
              "&+rYou detect no evil presence in the ether around you.\n");
    }
    else if(elevel < 100)
    {
      sprintf(buf,
              "&+rYou detect a evil presence in the ether around you.\n");
    }
    else if(elevel < 250)
    {
      sprintf(buf,
              "&+rYou detect a evil presence in the ether around you.\n");
    }
    else
    {
      sprintf(buf,
              "&+rYou detect a evil presence in the ether around you.\n");
    }
    send_to_char(buf, ch);
  }
}

void spell_stornogs_spheres(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  struct affected_type af;
  
  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
  {
    return;
  }
  if(IS_AFFECTED4(victim, AFF4_STORNOGS_SPHERES))
    return;

  memset(&af, 0, sizeof(af));
  af.type = SPELL_STORNOGS_SPHERES;
  af.bitvector4 = AFF4_STORNOGS_SPHERES;
  af.duration = -1;
  af.flags = AFFTYPE_NOSHOW;

  if(!IS_AFFECTED4(victim, AFF4_STORNOGS_SPHERES) &&
      (GET_CLASS(victim, CLASS_CONJURER)))
  {
    act("&+RSpheres begin rotating around $n.", TRUE, victim, 0, 0, TO_ROOM);
    act("&+RSpheres begin rotating around you.", TRUE, victim, 0, 0, TO_CHAR);
    af.modifier = MAX(1, (GET_LEVEL(victim) - 50)) / 2 + 1;
  }
  else
  {
    act("&+RSpheres begin rotating around $n.", TRUE, victim, 0, 0, TO_ROOM);
    act("&+RSpheres begin rotating around you.", TRUE, victim, 0, 0, TO_CHAR);
    af.modifier = 1;
  }
  affect_to_char(victim, &af);
}

void spell_dazzle(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  struct affected_type af;

  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
      return;

  if(IS_AFFECTED4(victim, AFF4_DAZZLER))
    return;
  else
  {
    send_to_char
      ("&+cYou feel a surge of energy, as &+wsparks &+carc between your fingers.&N\n",
       ch);
    act("$n begins to &+Wsparkle&N!", TRUE, victim, 0, 0, TO_ROOM);
    bzero(&af, sizeof(af));
    af.type = SPELL_DAZZLE;
    af.duration =  (int) (level / 2);
    af.bitvector4 = AFF4_DAZZLER;
    affect_to_char(victim, &af);
  }
  return;
}

void spell_accel_healing(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  int skl_lvl = (int) (MAX(41, ((level / 2) - 1)));
  
  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
        return;

  if(affected_by_spell(victim, SKILL_REGENERATE) ||
    affected_by_spell(victim, SPELL_REGENERATION))
  {
    act("$N can't possibly heal any faster.", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  
  if(affected_by_spell(victim, SPELL_ACCEL_HEALING))
  {
    struct affected_type *af1;
    bool found;
    
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_ACCEL_HEALING)
      {
        af1->duration = 20;
        found = true;
      }
    }
    
    if(found)
      send_to_char("Your &+caccelerated healing&n magic is &+Yrefreshed!\r\n", victim);
    return;
  }
 
  bzero(&af, sizeof(af));
  af.type = SPELL_ACCEL_HEALING;
  af.duration = skl_lvl + 1;
  af.location = APPLY_HIT_REG;
  af.modifier = (int) (2.5 * level);
  affect_to_char(victim, &af);

  send_to_char("You begin to heal faster.\r\n", victim);
}

void spell_pleasantry(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_PLEASANTRY))
  {
    send_to_char("You create a warm, pleasant feeling in your victim.\n", ch);
    send_to_char("You are suddenly overcome with a warm, pleasant feeling.\n",
                 victim);
    bzero(&af, sizeof(af));
    af.type = SPELL_PLEASANTRY;
    af.duration = 99;
    affect_to_char(victim, &af);
  }
  else
  {
    send_to_char("The bozo has already been made as pleasant as possible.\n", ch);
  }
}

void do_pleasantry(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  P_obj    dummy;
  char     buf[MAX_STRING_LENGTH];
  struct affected_type af;

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf)
    send_to_char("Usage: pleasant <player>\n", ch);
  else if(!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
    send_to_char("Couldn't find any such creature.\n", ch);
  else if(GET_LEVEL(vict) >= AVATAR)
    act("$E doesn't get any more pleasant than this.", 0, ch, 0, vict, TO_CHAR);
  else if(!affected_by_spell(vict, SPELL_PLEASANTRY))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PLEASANTRY;
    af.duration = 25;
    affect_to_char(vict, &af);
    act("You make $M tense up as they think about the game.", 0, ch, 0, vict, TO_CHAR);
  }
  else
  {
    act("You let $M relax.", 0, ch, 0, vict, TO_CHAR);
    affect_from_char( vict, SPELL_PLEASANTRY );
  }
}

void pleasantry(P_char ch)
{
  char     buf[256];
  int      x;

  x = number(1, 18);

  sprintf(buf, "say I'm buggy!");       /* just in case */
  switch (x)
  {
  case 1:
    sprintf(buf, "say Duris is the reason I can't pay child-support!");
    break;
  case 2:
    sprintf(buf,
            "say I think I'll donate 50 dollars for all the endless volunteer work that goes into Duris!");
    break;
  case 3:
    sprintf(buf, "say I love this game, I'm teaching my kids how to play!");
    break;
  case 4:
    sprintf(buf,
            "say I shouldn't be pressuring Zion, he knows what he's doing and it takes time.");
    break;
  case 5:
    sprintf(buf,
            "say I may die a virgin, but at least I'll be the best Duris player ever!");
    break;
  case 6:
    sprintf(buf, "say One of the immortals totally rick-rolled me.");
    break;
  case 7:
    sprintf(buf, "say Isn't this a REALLY cool zone?  Duris has the BEST areas!");
    break;
  case 8:
    sprintf(buf,
            "say I heard Torgal spends money on the Duris link. I can't believe I was such a twink as to complain about anything!");
    break;
  case 9:
    sprintf(buf, "dance");
    command_interpreter(ch, buf);
    sprintf(buf, "say Footloose, I gotta cut footloose!");
    command_interpreter(ch, buf);
    sprintf(buf, "dance");
    break;
  case 10:
    sprintf(buf, "sload");
    command_interpreter(ch, buf);
    sprintf(buf, "say Oops! I guess I was having TOO much fun on Duris!");
    command_interpreter(ch, buf);
    sprintf(buf, "blush");
    break;
  case 11:
    sprintf(buf,
            "say If I had to choose between sex and Duris, I choose Duris!");
    break;
  case 12:
    sprintf(buf,
            "say Has anyone seen Aycer? I want to zone, but I'm just too lazy to learn how to lead!");
    break;
  case 13:
    sprintf(buf, "omg");
    command_interpreter(ch, buf);
    sprintf(buf,
            "say Torgal rules! I hope I'm just like him when I grow up, minus the fu-man-chu!");
    break;
  case 14:
    sprintf(buf,
            "say Someday, I hope I'm remembered as a great player! Like Vuthen!");
    command_interpreter(ch, buf);
    sprintf(buf, "rofl me");
    break;
  case 15:
    sprintf(buf,
            "say People who whine about anything here should just be deleted!");
    break;

  case 16:
    sprintf(buf,
            "say Try not to get too good at Duris, or Torgal will code lag to your character!");
    break;

  case 17:
    sprintf(buf, "say Lions, Tigers and Cerif's, Oh My!");
    break;

  case 18:
    sprintf(buf, "say Thank god this isn't Toril!  I'd hate to play a game with no development!");
    break;
  }
  command_interpreter(ch, buf);
}

void spell_command(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{
  struct affected_type af;

  if(!NewSaves(victim, SAVING_SPELL, -((GET_LEVEL(victim) / 10) + 3)))
  {
    act("With but a single word, you stun $N into submission.", FALSE,
        ch, 0, victim, TO_CHAR);
    act("With a single indecipherable word, $n stuns you into submission!",
        FALSE, ch, 0, victim, TO_VICT);
    act("With a single indecipherable word, $n stuns $N into submission!",
        FALSE, ch, 0, victim, TO_NOTVICT);

    Stun(victim, ch, (GET_LEVEL(ch) > 50) ? PULSE_VIOLENCE * 2 : PULSE_VIOLENCE, FALSE);

    if(IS_NPC(victim) && CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
        MobStartFight(victim, ch);
    }
  }
  else
  {
    act("With but a single word, you... don't seem to do much of anything.",
        FALSE, ch, 0, victim, TO_CHAR);
    act("$n points at you while screaming.  You don't feel any different.",
        FALSE, ch, 0, victim, TO_VICT);
    act
      ("With a single indecipherable word, $n does nothing in particular to $N.",
       FALSE, ch, 0, victim, TO_NOTVICT);

    if(IS_NPC(victim) && CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if(!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
        MobStartFight(victim, ch);
    }
  }
}

void spell_group_heal(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  int healpoints = 70 + number(0, 10);

  if(!(ch) ||
     !IS_ALIVE(ch))
        return;
        
  if(GET_SPEC(ch, CLASS_CLERIC, SPEC_HEALER))
    healpoints = 120 + number(0, 20);

  if(GET_CHAR_SKILL(ch, SKILL_DEVOTION) > 0)
    healpoints += (int)(GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5);
    
  heal(ch, ch, healpoints, GET_MAX_HIT(ch) - number(1,4));
  send_to_char("&+WA warm feeling of peace fills your body.\n", ch);
    
  if(ch->group)
  {
    for (struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if(ch != gl->ch && gl->ch->in_room == ch->in_room)
      {
        if( GET_HIT(gl->ch) < GET_MAX_HIT(gl->ch) )
        {
          heal(gl->ch, ch, healpoints, GET_MAX_HIT(gl->ch) - number(1,4));
          update_pos(gl->ch);
          send_to_char("&+WA warm feeling of peace fills your body.\n", gl->ch);
        }
      }
    }
  }
}

void spell_chaos_volley(int level, P_char ch, char *arg, int type,
                                P_char victim, P_obj obj)
{
  int spldam;
  struct damage_messages messages = {
    "&+YC&+yr&+Ya&+yc&+Yk&+yl&+Ying magical &+Cbolts&+L erupt from your palms and slam into&n $N&+L!&n",
    "&+LPow&+rerf&+Lul ma&+rgic&+Lal &+Cbolts&+L slam into you from&n $n's&+L open palms!&n",
    "$n&n&+L has a blank expression as $s &+rC&+Rh&+rA&+RO&+rti&+RC&+C bolts&+L slam into&n $N!&n",
    "&+LYou nod in approval as your chaotic &+Cbolt&+L smashes&n $N&+L into &+rob&+Rli&+rvi&+Ron!&n",
    "&+LThe last thing you see as your &+Wlife&n &+Lflashes before your eyes is the &+yground! &+RR.I.P.&n",
    "$n&n&+L is greatly pleased as $s magical &+Cbolts&+L decimate&n $N.&n", 0
  };
  
  int luckmod;
  int cvdam = BOUNDED(0, (GET_MAX_HIT(ch) - GET_HIT(ch)), 250); //Health damage modifier
  int dam = MIN(level, 56) * 9 + number(1, 20); // Bigby's clenched fist * 10; Bigby's crushing hand *12
 
  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
      return;

  if(GET_HIT(ch) > (GET_MAX_HIT(ch) / 2)) // Drains hps up to 1/2 max!!!
    GET_HIT(ch) -= (dam / 30);

  dam = dam + cvdam; // Adding in current health modifier
  
  // Luck influences chaos
  luckmod = BOUNDED(-2, (GET_C_LUK(ch) - GET_C_LUK(victim)) / 10, 2);

  if(!NewSaves(victim, SAVING_SPELL, luckmod))
    dam *= 2; // Failed bigby's clenched and cruahsing do * 2 dam
 
  int dam1 = number(1 , dam); // This randomizes 1st damage amount
  int dam2 = dam - dam1; // This is the 2nd damage amount

  switch (number(1, 11))
    {
    case 1:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_GENERIC, SPLDAM_NOSHRUG, &messages);
      break;
    case 2:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_FIRE, SPLDAM_NOSHRUG, &messages);
      break;
    case 3:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_COLD, SPLDAM_NOSHRUG, &messages);
      break;
    case 4:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_LIGHTNING, SPLDAM_NOSHRUG, &messages);
      break;
    case 5:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_GAS, SPLDAM_NOSHRUG, &messages);
      break;
    case 6:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_ACID, SPLDAM_NOSHRUG, &messages);
      break;
    case 7:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG, &messages);
      break;
    case 8:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_HOLY, SPLDAM_NOSHRUG, &messages);
      break;
    case 9:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_PSI, SPLDAM_NOSHRUG, &messages);
      break;
    case 10:
      spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
      if(IS_ALIVE(ch) && IS_ALIVE(victim))
        spell_damage(ch, victim, dam2, SPLDAM_SPIRIT, SPLDAM_NOSHRUG, &messages);
      break;
    case 11:
      spell_damage(ch, victim, dam, SPLDAM_SOUND, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
      break;
    }
}

void spell_chaos_shield(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  if(affected_by_spell(ch, SPELL_CHAOS_SHIELD))
  {
    send_to_char("You're already protected by &+rchaotic forces&n.\n", ch);
    return;
  }

  struct affected_type af;
  bzero(&af, sizeof(af));
  af.type = SPELL_CHAOS_SHIELD;
  af.duration = GET_LEVEL(ch) / 3;
  affect_to_char(ch, &af);

  act("&+LA wild magical aura swirls up and around you.&n", TRUE, ch, 0, 0, TO_CHAR);
  act("&+LA wild magical aura swirlds up and around $n.&n", TRUE, ch, 0, 0, TO_NOTVICT);
}

#define MAX_CHAOSSHIELD_SPELL 15
  const int randomspell[MAX_CHAOSSHIELD_SPELL] = {
    SPELL_STRENGTH,
    SPELL_DEXTERITY,
    SPELL_COLDSHIELD,
    SPELL_FIRESHIELD,
    SPELL_HASTE,
    SPELL_FLY,
    SPELL_GLOBE,
    SPELL_FEEBLEMIND,
    SPELL_STONE_SKIN,
    SPELL_BALADORS_PROTECTION,
    SPELL_LIGHTNINGSHIELD,
    SPELL_FARSEE,
    SPELL_DETECT_INVISIBLE,
    SPELL_SLOW,
    SPELL_WITHER
  };

int parse_chaos_shield(P_char victim, P_char ch)
{
  int chance;

  if(!affected_by_spell(ch, SPELL_CHAOS_SHIELD))
    return FALSE;

  chance = (int)get_property("spell.chaosshield.perc", 10.000);

  if(chance < number(0, 101))
    return FALSE;

  act("&+LThe chaotic shield surrounding $n flares as it absorbs some of $N's spell.&n", FALSE, ch, 0, victim, TO_NOTVICTROOM);
  act("&+LThe chaotic shield surrounding $n flares as it absorbs some of your spell.&n", FALSE, ch, 0, victim, TO_VICT);
  act("&+LThe chaotic shield surrounding you flares as it absorbs some of $N's spell.&n", FALSE, ch, 0, victim, TO_CHAR);

  ((*skills[randomspell[number(0, MAX_CHAOSSHIELD_SPELL-1)]].spell_pointer) ((int) GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0));

  return TRUE;
}

void spell_knock(int cmd, P_char ch, char *argument, int type, P_char victim, P_obj obj)
{
  int      percent, door, other_room, chance, retval;
  struct room_direction_data *back;
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
  P_obj    found_obj;
  P_char   found_char;

  argument_interpreter(argument, Gbuf2, Gbuf3);

  chance = GET_LEVEL(ch);
  percent = number(1, 100);

  if( !Gbuf2 || !*Gbuf2 )
  {
    send_to_char("What requires unlocking here again?\n", ch);
    return;
  }

  if( IS_TRUSTED(ch) )
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &found_char, &found_obj);
  }
  else
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &found_char, &found_obj);
  }

  if( retval != 0)
  {
     if((found_obj->type != ITEM_CONTAINER) &&
           (found_obj->type != ITEM_STORAGE) && (found_obj->type != ITEM_QUIVER))
        send_to_char("That's not a container.\n", ch);
     else if(!IS_SET(found_obj->value[1], CONT_CLOSED))
        send_to_char("Unlocking something that's not even closed - how droll!\n", ch);
     else if(found_obj->value[2] < 0)
        send_to_char("Odd - you can't seem to find a keyhole.\n", ch);
     else if(!IS_SET(found_obj->value[1], CONT_LOCKED))
        send_to_char("Hey, it's not even locked!\n", ch);
     else
     {
        if(IS_SET(found_obj->value[1], CONT_PICKPROOF))
        {
           send_to_char("You focus your magic, but the container resists all attempts to open it.  You'd better find the key.\n", ch);
           return;
        }

        if((percent > chance))
        {
           send_to_char("You hear the brief sound of tumblers and pins turning, but alas, you lose concentration...\n", ch);
           CharWait(ch, 8);
           return;
        }
        else
        {
           REMOVE_BIT(found_obj->value[1], CONT_LOCKED);
           send_to_char("The sound of tumblers and pins clicking into place is music to your ears.\n", ch);
           act("$p rattles loudly, the sound of tumblers and pins locking into place!", FALSE, ch, obj, 0, TO_ROOM);
           CharWait(ch, 6);
           return;
        }
     }
  }
  return;
}


void spell_negative_concussion_blast(int level, P_char ch, char *arg,
                                     int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "&+LYour concussion &+Yblast&+L rips into $N, &+Wsh&+Lat&Nte&+Wri&+Lng $S&+r soul&+L!&N",
    "&+L$n's concussion &+Yblast&+L rips into you, &+Wsh&+Lat&Nte&+Wri&+Lng your &+rsoul&+L!&N",
    "&+L$n's concussion &+Yblast&+L rips into $N, &+Wsh&+Lat&Nte&+Wri&+Lng $S&+r soul&+L!&N",
    "&+LYour blast shatters $N &+Linto a million pieces!&N",
    "&+LYou scream as $n &+Lblasts you into the next life!&N",
    "$n &+Lblasts $N &+Linto the next life!&N"};
  int  dam, temp;

  if(!ch)
  {
    logit(LOG_EXIT, "spell_negative_concussion_blast called in magic.c with no ch");
    raise(SIGSEGV);
  }
  if(ch)
  {
    if(!IS_ALIVE(ch))
    {
      return;
    }

    if(resists_spell(ch, victim))
    {
      return;
    }

    dam = dice(2 * level, 6);

    spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG, &messages);
  }
}

void event_acidimmolate(P_char ch, P_char vict, P_obj obj, void *data)
{
  int dam, acidburntime, in_room;
  struct damage_messages messages = {
    "&+GG&+gr&+Ge&+ge&+Gn s&+gl&+Gi&+gm&+Ge eats away at $N&+G's skin!&N",
    "&+GG&+gr&+Ge&+ge&+Gn s&+gl&+Gi&+gm&+Ge eats away at your skin!&N",
    "&+GG&+gr&+Ge&+ge&+Gn s&+gl&+Gi&+gm&+Ge eats away at $N&+G's skin!&N",
    "$N &+gmelts into a pile of &+GGOO&n ... $E is no more!",
    "&+gThe &+Gs&+gl&+Gi&+gm&+Ge &+gconsuming your flesh devours you completely!",
    "$N &+gmelts into a pile of &+GGOO&n ... $E is no more!", 0
  };

  // Missing means that something went very wrong.
  if( !ch || !vict )
  {
    logit(LOG_EXIT, "event_acidimmolate called in magic.c with no ch");
    raise(SIGSEGV);
  }
  // Dead just means something died but hasn't 'gone to heaven' yet.
  if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
  {
    return;
  }
  acidburntime = *((int *) data);

  if( (acidburntime >= 4 && number(0, acidburntime--)) || acidburntime == 7 )
  {
    act("&+GThe &+Ymo&+yrd&+Yant &+Gsubstance oozes off you!", FALSE, ch, 0, vict, TO_VICT);

    if( ch->in_room == vict->in_room )
    {
      act("&+GThe &+Ymo&+yrd&+Yant &+Gsubstance oozes off of $N.", FALSE, ch, 0, vict, TO_CHAR);
    }
    act("&+GThe &+Ymo&+yrd&+Yant &+Gsubstance oozes off of $N.", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }
  else
  {
    acidburntime++;
  }

  //dam = (int) (GET_LEVEL(ch) * 2);
  dam = 1; //this is now just a pulling spell.
  if( !GET_CLASS(ch, CLASS_CONJURER) )
  {
    if( acidburntime >= 4 )
    {
      dam = (int) GET_LEVEL(ch) + number(4, 12);
    }
    else if( acidburntime == 3 )
    {
      dam = (int) (GET_LEVEL(ch) * 2 - number(4, 20));
    }
  }
  if( !number(0, 4) )
  {
    // Had to split this up 'cause !0 && Dead char leads to another call to spell_damage
    if( spell_damage(ch, vict, dam, SPLDAM_ACID, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD )
    {
      return;
    }
    add_event( event_acidimmolate, PULSE_VIOLENCE, ch, vict, NULL, 0, &acidburntime, sizeof(acidburntime) );
    if(8 > number(1, 10))
    {
      stop_memorizing(vict);
    }
  }
  // This is still strange, as it's almost identical to above, aside from the stop_memming chance.
  else if( spell_damage(ch, vict, dam, SPLDAM_ACID, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD )
  {
    add_event(event_acidimmolate, PULSE_VIOLENCE, ch, vict, NULL, 0, &acidburntime, sizeof(acidburntime) );
    if( 3 > number(1, 10) )
    {
      stop_memorizing(vict);
    }
  }
}

void spell_acidimmolate(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int acidburn = 0;
  
  if(!ch)
  {
    logit(LOG_EXIT, "spell_immolate called in magic.c with no ch");
    raise(SIGSEGV);
  }
  
  if(!(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  act("&+GYour bubbling spray of goo strikes $N&+G full on!&N", TRUE, ch, 0,
    victim, TO_CHAR);
  act("&+GA bubbling spray of goo spews from $n&+G striking you &+Gfull on!&N",
    FALSE, ch, 0, victim, TO_VICT);
  act("&+GA bubbling spray of goo spews from $n&+G striking $N &+Gfull on!&N",
    FALSE, ch, 0, victim, TO_NOTVICT);
  
  engage(ch, victim);

  if(spell_damage(ch, victim, (int) GET_LEVEL(ch) * 2 + number(20, 120),
      SPLDAM_ACID, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, NULL) == DAM_NONEDEAD);
  {
    if(IS_ALIVE(victim)) // Adding double check.
    {
      add_event(event_acidimmolate, PULSE_VIOLENCE, ch, victim, NULL, 0, &acidburn, sizeof(acidburn));
      //gain_exp(ch, victim, 0, EXP_DAMAGE);
    }
  }
}

#define RIPPLE_STUN     BIT_1
#define RIPPLE_BLIND    BIT_2
#define RIPPLE_FIREBALL BIT_3
#define RIPPLE_TENTACLE BIT_4
#define RIPPLE_PARA     BIT_5
#define RIPPLE_SWITCH   BIT_6
#define RIPPLE_BOLTS    BIT_7

void spell_chaotic_ripple(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int dam, rays, ray_flag;
  struct affected_type af;
  P_char   tch;
  struct damage_messages messages = {
    0, 0, 0,
    "&+LYour&n &+Cr&+ci&+Cp&+cp&+Cl&+ce&+Cs&n &+Lof&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy&n &+Lengulf&n $N &+Lblasting the &+wlife &+Lfrom $S body!",
    "&+LRipples of&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy &+Lengulf you\nblasting the life from your body.&n",
    "$n's&n &+Cr&+ci&+Cp&+cp&+Cl&+ce&+Cs&n &+Lof&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy&n &+Lengulf&n $N &+Lblasting the life from $S body!",
    0
  };
  if( !ch )
  {
    logit(LOG_EXIT, "spell_chaotic_ripple called in magic.c with no ch");
    raise(SIGSEGV);
  }
  messages.attacker =
    "&+LYou shatter the fabric of reality sending&n &+Cr&+ci&+Cp&+cp&+Cl&+ce&+Cs&n &+Lof&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy&n &+Lflowing into&n $N.";
  messages.victim =
    "&+LRipples of&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy &+Lslam into you as reality comes crashing down.&n";
  messages.room =
    "$n &+Lshatters the fabric of reality\n&+Lsending&n &+Cr&+ci&+Cp&+cp&+Cl&+ce&+Cs&n &+Lof&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy &+Lflowing into&n $N.";

  dam = 370 + 30 * MAX(1, level - 50) + number(1, 40);

  if(spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NODEFLECT |
    SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD)
  {
    return;
  }
  ray_flag = 0;
  rays = number(2, 4);

  while( rays > 0 )
  {
    int ray_type;

    if( !char_in_list(victim) )
    {
      break;
    }
    ray_type = 1 << number(0, 6);
    // If ray already used..
    if( ray_flag & ray_type )
    {
      continue;
    }
    rays--;
    ray_flag |= ray_type;
    memset(&af, 0, sizeof(struct affected_type));
    switch( ray_type )
    {
    case RIPPLE_STUN:
      if(!IS_GREATER_RACE(victim) && !IS_ELITE(victim))
      {
        act
          ("&+wClutching $S head $N tries to escape the\n&+rm&+wa&+rd&+wd&+re&+wn&+ri&+wn&+rg&n &+wimages circling $M.&n",
           TRUE, victim, 0, victim, TO_ROOM);
        act
          ("&+wClutching your head you try to escape the\n&+rm&+wa&+rd&+wd&+re&+wn&+ri&+wn&+rg&n &+Wimages circling you!&n",
           TRUE, victim, 0, victim, TO_CHAR);
        Stun(victim, ch, PULSE_VIOLENCE / 2, FALSE);
      }
      break;
    case RIPPLE_BLIND:
      act
        ("&+LDarkness&n flows from the rift encasing $N in an impenetrable shell.&n",
         TRUE, victim, 0, victim, TO_ROOM);
      act
        ("&+LDarkness&n flows from the rift encasing you in an impenetrable shell!&n",
         TRUE, victim, 0, victim, TO_CHAR);
      blind(ch, victim, PULSE_VIOLENCE);
      break;
    case RIPPLE_FIREBALL:
      messages.attacker = messages.room =
        "&+RBOOOOOOOOOM!&n\n$N explodes in flames as a &+rGIGANTIC &+Rfireball&n engulfs $m.";
      messages.victim =
        "&+RBOOOOOOOOOM!&n\nYou explode in flames as a &+rGIGANTIC &+Rfireball&n engulfs you.";
      if(spell_damage(ch, victim, (int) (0.4 * dam), SPLDAM_FIRE, 0, &messages)
          != DAM_NONEDEAD)
      {
        return;
      }
      break;
    case RIPPLE_TENTACLE:
      if(GET_C_AGI(victim) < number(0, 200))
      {
       act("&+LA tentacle of &+Wsolified m&+wi&+Wst &+Lcoils&n itself around $N\ntossing $M to the ground.", TRUE, victim, 0, victim, TO_ROOM);
       act("&+LA tentacle of &+Wsolified m&+wi&+Wst &+Lcoils&n itself around you\ntossing you to the ground.", TRUE, victim, 0, victim, TO_CHAR);
       SET_POS(victim, POS_SITTING + GET_STAT(victim));
       CharWait(victim, PULSE_VIOLENCE);
      }
      break;
    case RIPPLE_PARA:
      act
        ("&+CIce &+Wcrystals&n form around $N as &+Cintense cold&n causes $S body to &+Cfreeze.&n",
         TRUE, victim, 0, victim, TO_ROOM);
      act
        ("&+CIce &+Wcrystals&n form around you as &+Cintense cold&n causes your body to &+Cfreeze.&n",
         TRUE, victim, 0, victim, TO_CHAR);
      if(spell_damage(ch, victim, (int) (0.4 * dam), SPLDAM_COLD, 0, &messages)
          != DAM_NONEDEAD)
      {
        return;
      }
      if(!NewSaves(victim, SAVING_PARA, 0) &&
         !IS_GREATER_RACE(victim) &&
         !IS_ELITE(victim))
      {
        af.type = SPELL_MAJOR_PARALYSIS;
        af.flags = AFFTYPE_SHORT;
        af.bitvector2 = AFF2_MAJOR_PARALYSIS;
        af.duration = PULSE_VIOLENCE/2;
        affect_to_char(victim, &af);
      }
      break;
    case RIPPLE_SWITCH:
      if(IS_PC(victim) || IS_PC_PET(victim))
      {
        tch = get_random_char_in_room(victim->in_room, victim, DISALLOW_SELF);
        if(tch && on_front_line(tch) && victim->group == tch->group)
        {
          act
            ("A glint of &+rm&+wa&+rd&+wd&+re&+wn&+ri&+wn&+rg&n touches $N's eyes\nas unseen powers corrupt $M.",
             TRUE, victim, 0, victim, TO_ROOM);
          act
            ("A glint of &+rm&+wa&+rd&+wd&+re&+wn&+ri&+wn&+rg&n touches your eyes\nas unseen powers corrupt you.",
             TRUE, victim, 0, victim, TO_CHAR);
          attack(victim, tch);
        }
        break;
      } // Continue on to RIPPLE_BOLTS if target is a non-pet npc.
    case RIPPLE_BOLTS:
      messages.attacker = messages.room =
        "Twin &+Bbolts&n of writhing power slam into $N's chest.&n";
      messages.victim =
        "Twin &+Bbolts&n of writhing power slam into your chest.&n";
      
      if(spell_damage(ch, victim, (int) (.35 * dam), SPLDAM_GENERIC,
          SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD)
      {
        return;
      }
      break;
    }
  }
}

void spell_blink(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   tch;
  int      fall_chance;

  if(ch)
  {
    if(!IS_ALIVE(ch))
    {
      return;
    }
    send_to_char("You scatter your atoms to the wind only to reassemble nearby.\n", ch);

    if(IS_FIGHTING(ch))
      stop_fighting(ch);
    if(IS_DESTROYING(ch))
      stop_destroying(ch);
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(tch->specials.fighting == ch)
      {
        act("Without warning $n simply ceases to be. Doh! Where did he go?", FALSE, ch,
          0, tch, TO_ROOM);
        stop_fighting(tch);
        fall_chance = MAX(20, 50 - GET_C_AGI(tch) / 2);
        if(fall_chance > number(1, 100))
        {
          send_to_char
            ("Unable to reverse your swing you lose your balance and crash to your knee.\n",
             tch);
          SET_POS(tch, POS_SITTING + GET_STAT(tch));
          CharWait(tch, PULSE_VIOLENCE);
        }
      }
      else if(ch != tch)
      {
        act("$n suddenly ceases to be only to materialize nearby.", FALSE, ch,
            0, tch, TO_VICT);
      }
    }
  }
}

void spell_single_banish(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int chance;

  if( !ch )
  {
    logit(LOG_EXIT, "spell_single_banish called in magic.c with no ch");
    raise(SIGSEGV);
  }

  if( IS_TRUSTED(victim) || IS_PC(victim) )
  {
    return;
  }

  chance = (int) (((40 * level) + ( 10 * GET_CHAR_SKILL(ch, SKILL_DEVOTION))) / (GET_LEVEL(victim) + 1));

  if( GET_LEVEL(ch) > (10 + GET_LEVEL(victim)) )
  {
    chance = 95;
  }

  if( GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    chance = (int) (chance * 1.25);
  }

  if( IS_GREATER_DRACO(victim) || IS_GREATER_AVATAR(victim) )
  {
    chance = (int) (chance * 1 / 3);
  }

  chance = BOUNDED(1, chance, 95);

  debug("(%s) attempts to banish (%s) with (%d) chance.", GET_NAME(ch), GET_NAME(victim), chance);

  if(chance > number(1,100))
  {
    if(IS_UNDEADRACE(victim) ||	IS_ANGEL(victim))
    {
      act("You break the binding energies and watch as $N crumbles to dust.", FALSE, ch, 0, victim, TO_CHAR);
      act("$N &+Lsuddenly becomes lifeless once more and crumbles to dust.&n", FALSE, ch, 0, victim, TO_NOTVICT);
      check_saved_corpse(victim);
      extract_char(victim);
      return;
    }

    switch (GET_RACE(victim))
    {
      case RACE_W_ELEMENTAL:
        act("You banish $N back to the plane of water.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n banishes $N back to the plane of water.", FALSE, ch, 0, victim, TO_NOTVICT);
        break;
      case RACE_A_ELEMENTAL:
        act("You banish $N back to the plane of air.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n banishes $N back to the plane of air.", FALSE, ch, 0, victim, TO_NOTVICT);
        break;
      case RACE_E_ELEMENTAL:
        act("You banish $N back to the plane of earth.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n banishes $N back to the plane of earth.", FALSE, ch, 0, victim, TO_NOTVICT);
        break;
      case RACE_EFREET:
        act("You banish $N back to the plane of fire.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n banishes $N back to the plane of fire.", FALSE, ch, 0, victim, TO_NOTVICT);
        break;
      case RACE_F_ELEMENTAL:
        act("You banish $N back to the plane of fire.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n banishes $N back to the plane of fire.", FALSE, ch, 0, victim, TO_NOTVICT);
        break;
      case RACE_DRACOLICH:
        act("Arcs of divine flames leap from your hands reducing $N to ash.", FALSE, ch, 0, victim, TO_CHAR);
        act("Divine flames coil around $N reducing it to a pile of ash.", FALSE, ch, 0, victim, TO_NOTVICT);
        break;
      case RACE_GOLEM:
        act("Having lost its binding magic $N topples over and falls apart.", FALSE, ch, 0, victim, TO_CHAR);
        act("$N abruply stiffens and then simply falls apart.", FALSE, ch, 0, victim, TO_NOTVICT);
        break;
      default:
        act("The magic binding $N is unraveled and $E simply ceases to be.", FALSE, ch, 0, victim, TO_CHAR);
        act("Divine power sweeps over $N and $E is no more.", FALSE, ch, 0, victim, TO_NOTVICT);
        break;
    }
    check_saved_corpse(victim);
    extract_char(victim);
  }
}

bool can_banish(P_char ch, P_char victim)
{
  if(!ch) // Something is amiss.
  {
    logit(LOG_EXIT, "can_banish called in magic.c with no ch");
    raise(SIGSEGV);
  }

  if(IS_NPC(victim))
  {
    if(mob_index[GET_RNUM(victim)].virtual_number == 250 || 
       mob_index[GET_RNUM(victim)].virtual_number == 63)
    {
      return TRUE;
    }

    if(GET_RACE(victim) == RACE_E_ELEMENTAL &&
       world[victim->in_room].sector_type == SECT_EARTH_PLANE)
    {
      send_to_char("This is the earth plane. Your banish spell fails!", ch);
      return FALSE;
    }

    if( (GET_RACE(victim) == RACE_F_ELEMENTAL || GET_RACE(victim) == RACE_EFREET)
      && world[victim->in_room].sector_type == SECT_FIREPLANE )
    {
      send_to_char("This is the fire plane. Your banish spell fails!", ch);
      return FALSE;
    }

    if(GET_RACE(victim) == RACE_A_ELEMENTAL &&
       world[victim->in_room].sector_type == SECT_AIR_PLANE)
    {
      send_to_char("This is the air plane. Your banish spell fails!", ch);
      return FALSE;
    }

    if(GET_RACE(victim) == RACE_W_ELEMENTAL &&
       world[victim->in_room].sector_type == SECT_WATER_PLANE)
    {
      send_to_char("This is the water plane. Your banish spell fails!", ch);
      return FALSE;
    }

    if(IS_ANGEL(victim))
    {
      if(world[victim->in_room].sector_type == SECT_ETHEREAL)
      {
        send_to_char("This is the ethereal plane. Your banish spell fails!", ch);
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }

    if(IS_UNDEADRACE(victim))
    {
      if(world[victim->in_room].sector_type == SECT_NEG_PLANE)
      {
        send_to_char("This is the negative plane. Your banish spell fails!", ch);
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }

    switch (GET_RACE(victim))
    {
      case RACE_E_ELEMENTAL:
      case RACE_F_ELEMENTAL:
      case RACE_EFREET:
      case RACE_A_ELEMENTAL:
      case RACE_W_ELEMENTAL:
  	  case RACE_I_ELEMENTAL:
        return TRUE;
        break;
      default:
        if( affected_by_spell(victim, TAG_CONJURED_PET) )
        {
          return TRUE;
        }
        return FALSE;
        break;
    }
  }

  return FALSE;
}

void spell_banish(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char tch, next;
  char buf[256];

  sprintf(buf, "You raise your holy symbol and send a prayer to %s to aid you in battle.",get_god_name(ch));
  act(buf, FALSE, ch, 0, 0, TO_CHAR);
  sprintf(buf, "$n raises $s holy symbol sending a battle prayer to %s.", get_god_name(ch));
  act(buf, FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if(get_linked_char(tch, LNK_PET) &&
       can_banish(ch, tch))
    {
      attack_back(ch, tch, FALSE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if(get_linked_char(tch, LNK_PET) &&
       can_banish(ch, tch))
    {
      spell_single_banish(level, ch, arg, type, tch, 0);
    }
  }

  // cast_as_damage_area(ch, spell_single_banish, level, victim,
      // get_property("spell.area.minChance.banish", 10),
      // get_property("spell.area.chanceStep.banish", 25), can_banish);

}


void spell_doom_blade(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  P_obj    weapon;

  if (GET_CLASS(ch, CLASS_THEURGIST))
  {
    weapon = read_object(426, VIRTUAL);
    if(!weapon)
    {
      logit(LOG_DEBUG, "spell_doom_blade(): obj 426 not loadable");
      return;
    }
  
    act("As you call to the &n&+Wh&n&+Yea&n&+Wv&n&+Ye&n&+Wns&n for a weapon to slay the &n&+Lev&n&+ri&n&+Ll&n in the world, an", TRUE, ch, weapon, 0, TO_CHAR);
    act("&n&+Yang&n&+We&n&+Yl&n&+Ric&n figure &n&+Lmat&n&+wer&n&+Wia&n&+wli&n&+Lzes&n before you, handing you a &n&+Ygolden&n blade. The", TRUE, ch, weapon, 0, TO_CHAR);
    act("figure recites a short &n&+Yprayer&n before &n&+Wva&n&+wni&n&+Ls&n&+whi&n&+Wng&n as fast as it came.", TRUE, ch, weapon, 0, TO_CHAR);

    act("As $n calls to the &n&+Wh&n&+Yea&n&+Wv&n&+Ye&n&+Wns&n for a weapon to slay the &n&+Lev&n&+ri&n&+Ll&n in the world,", TRUE, ch, weapon, 0, TO_ROOM);
    act("An &n&+Yang&n&+We&n&+Yl&n&+Ric&n figure &n&+Lmat&n&+wer&n&+Wia&n&+wli&n&+Lzes&n before you, handing a &n&+Ygolden&n blade to $n.", TRUE, ch, weapon, 0, TO_ROOM);
    act("The figure recites a short &n&+Yprayer&n before &n&+Wva&n&+wni&n&+Ls&n&+whi&n&+Wng&n as fast as it came.", TRUE, ch, weapon, 0, TO_ROOM);
  }
  else
  {
    weapon = read_object(352, VIRTUAL);
    if(!weapon)
    {
      logit(LOG_DEBUG, "spell_doom_blade(): obj 352 not loadable");
      return;
    }
  
    act("$n &+wplunges $s clenched fist into the &+yground&+w and draws forth $p!&n", TRUE, ch, weapon, 0, TO_ROOM);
    act("&+wYou plunge your fist into the &+yground&+w and rip out $p!&n", TRUE, ch, weapon, 0, TO_CHAR);
    weapon->timer[0] = 1800;
  }

  obj_to_char(weapon, ch);
}

bool area_divine_blessing_check(P_char ch, P_char caster)
{
  P_char tch;
  struct group_list *gl;
  struct affected_type af;

  if(!ch->group || !affected_by_spell(ch, SPELL_DIVINE_BLESSING) ||
      GET_ALIGNMENT(ch) < get_property("spell.divineBlessing.areaShrugAlign", 970) ||
      !ch->equipment[WIELD])
    return false;

  memset(&af, 0, sizeof(af));
  af.type = TAG_IMMUNE_AREA;
  af.duration = 10;
  af.flags = AFFTYPE_SHORT;

  act("&+WA mighty rush of &+Ccourage&+W sweeps over you as your $q goes forth "
      "to absorb the mighty spell!", FALSE, ch, ch->equipment[WIELD], 0, TO_CHAR);
  act("$n &+Wbrandishes $s $q, and dives forth in combat, absorbing the brunt "
      "of $N's spell!", FALSE, ch, ch->equipment[WIELD], caster, TO_NOTVICT);
  act("$n &+Wbrandishes $s $q, and dives forth in combat, absorbing the brunt "
      "of your spell!", FALSE, ch, ch->equipment[WIELD], caster, TO_VICT);

  for (gl = ch->group; gl; gl = gl->next) {
    affect_to_char(gl->ch, &af);
  }

  return true;
}

bool divine_blessing_check(P_char ch, P_char tar_char, int spl)
{
  if((IS_SET(skills[spl].targets, TAR_IGNORE) ||
        IS_SET(skills[spl].targets, TAR_AREA)) &&
      number(0, 99) < get_property("spell.divineBlessing.areaShrugChance", 5)) {
    P_char tch;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
      if(should_area_hit(ch, tch) &&
          area_divine_blessing_check(tch, ch))
        break;
    }
  } else if(tar_char &&
      affected_by_spell(tar_char, SPELL_DIVINE_BLESSING) &&
      tar_char->equipment[WIELD] &&
      number(0, 99) < get_property("spell.divineBlessing.singleShrugChance", 10)) {
    act("&+wYou feel a &+Cf&+ci&+Wer&+cc&+Ce &+Wdetermination &+was your "
        "weapon thrusts itself upward, and absorbs $N's onslaught!",
        FALSE, tar_char, tar_char->equipment[WIELD], ch, TO_CHAR);
    act("&+wWith a &+Wsomber &+Cg&+cr&+Wa&+cc&+Ce&+w, $n holds $s $q high, absorbing $N's assault!",
        FALSE, tar_char, tar_char->equipment[WIELD], ch, TO_NOTVICT);
    act("&+wWith a &+Wsomber &+Cg&+cr&+Wa&+cc&+Ce&+w, $n holds $s $q high, absorbing your assault!",
        FALSE, tar_char, tar_char->equipment[WIELD], ch, TO_VICT);
    return TRUE;
  }

  return FALSE;
}

bool divine_blessing_parry(P_char ch, P_char victim)
{
  P_obj weapon = ch->equipment[WIELD];

  if(affected_by_spell(ch, SPELL_DIVINE_BLESSING) && weapon &&
      GET_ALIGNMENT(ch) > get_property("spell.divineBlessing.parryAlign", 990) &&
      number(0,99) < get_property("spell.divineBlessing.parryChance", 10)) {
    act("$n's $q moves with a blinding speed and successfully repels your attack!",
        FALSE, ch, weapon, victim, TO_VICT);
    act("$n's $q moves with a blinding speed and successfully repels $N's attack!",
        FALSE, ch, weapon, victim, TO_NOTVICT);
    act("Your $q moves with a blinding speed and successfully repels $N's attack!",
        FALSE, ch, weapon, victim, TO_CHAR);
    return true;
  } else
    return false;
}

void spell_divine_blessing(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  struct affected_type af;
  P_obj weapon = victim->equipment[WIELD];
  struct damage_messages messages = {
    "&+wYou incite the &+rR&+RA&+rG&+RE &+wof the &+CGods&+w as you attempt to invoke a blessing of &+WP&+Curi&+Wty",
    "&+wYou incite the &+rR&+RA&+rG&+RE &+wof the &+CGods&+w as you attempt to invoke a blessing of &+WP&+Curi&+Wty",
    "&+L$N screams in &+Rp&+rAi&+RN&+L as the &+CGods&+L punish $M for his arrogance!"
  };

  if(GET_ALIGNMENT(ch) < 950) {
    spell_damage(ch, ch, 600, SPLDAM_HOLY, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG |
      RAWDAM_NOKILL, &messages);
    return;
  }

  if(affected_by_spell(victim, SPELL_DIVINE_BLESSING))
  {
    send_to_char("Your gods are with you already!", victim);
    return;
  }
  else if(!weapon)
  {
    send_to_char("You need a weapon.\n", victim);
  } else
  {
    send_to_char
      ("&+wYou thrust your weapon skywards, and infuse it with your &+Wvirtue &+wand &+wm&+Wi&+bg&+wh&+Wt&+w!\n",
       victim);
    act("$n holds $p high, and it begins to &+Yb&+Wur&+Yn &+wwith a &+Ch&+Wol&+Cy &+Wlight&+w!",
        TRUE, victim, weapon, 0, TO_ROOM);
    memset(&af, 0, sizeof(af));
    af.type = SPELL_DIVINE_BLESSING;
    af.duration = 15;
    affect_to_char(victim, &af);
  }
}


void spell_natures_call(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int room = ch->in_room;

  send_to_room("&+gThe &+Gforces of nature&+g flows through the area...&n\n", ch->in_room);
  add_event(event_natures_call, PULSE_VIOLENCE, ch, 0, 0, 0, &room, sizeof(room));
}

void spell_natures_calling(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int num, effectiveness;

  if(!ch || !victim)
  {
    return;
  }

  num = dice(2, 6);
  effectiveness = (int) (level / 2);

  switch(num) {
    case 2:
      cast_vines(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      break;
    case 3:
      spell_stone_skin(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      break;
    case 4:
      spell_protection_from_fire(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      spell_protection_from_cold(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      spell_protection_from_lightning(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      spell_protection_from_gas(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      break;
    case 5:
     spell_virtue(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
     break;
    case 6:
     spell_armor(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
     break;
    case 7:
     spell_barkskin(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
     break;
    case 8:
     spell_armor(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
     break;
    case 9:
     spell_virtue(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
     break;
    case 10:
      spell_protection_from_fire(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      spell_protection_from_cold(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      spell_protection_from_lightning(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      spell_protection_from_gas(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      break;
    case 11:
      spell_stone_skin(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      break;
    case 12:
      cast_vines(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      break;
  }

  if(GET_ALIGNMENT(ch) > 350)
  {
     spell_bless(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }

  if(!(number(0,2)))
  {
    spell_protection_from_evil(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }
  if(!(number(0,2)))
  {
    spell_protection_from_animals(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }

  return;
}

void event_natures_call(P_char ch, P_char victim, P_obj obj, void *data)
{

  P_obj    t_obj, next_obj;
  P_obj    used_obj[32];
  int      count, num, level;
  int      room = *((int*)data);
  struct group_list *gl;

  if(!ch)
  {
    return;
  }
  if(ch)
  {
    level = GET_LEVEL(ch);

    if(!number(0, 1))
    {
      send_to_room("&+gThe &+Gforces of nature&+g come into balance, filling the room with life...\n", room);
      add_event(event_natures_call, PULSE_VIOLENCE, ch, 0, 0, 0, &room, sizeof(room));
    }

    act("&+gThe &+ycreatures of nature&+g in this area &+Gfill with energy!&n",
        FALSE, ch, 0, 0, TO_CHAR);

    act("&+gThe &+ycreatures of nature&+g in this area &+Gfill with energy!&n",
        FALSE, ch, 0, 0, TO_ROOM);

    if(ch && ch->group)
    {
      gl = ch->group;
      if(gl->ch->in_room == ch->in_room)
      {
        spell_natures_calling(level, ch, NULL, SPELL_TYPE_SPELL, gl->ch, 0);
      }
      for (gl = gl->next; gl; gl = gl->next)
      {
         if(gl->ch->in_room == ch->in_room)
         {
          spell_natures_calling(level, ch, NULL, SPELL_TYPE_SPELL, gl->ch, 0);
         }
      }
    }
  }
}

void spell_perm_increase_str(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Str >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your strength improve..\n", victim);
    victim->base_stats.Str = BOUNDED(1, victim->base_stats.Str +1, 95);
    victim->curr_stats.Str = BOUNDED(1, victim->curr_stats.Str +1, 95);
    balance_affects(victim);
}

void spell_perm_increase_agi(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Agi >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your agility improve..\n", victim);
    victim->base_stats.Agi = BOUNDED(1, victim->base_stats.Agi +1, 95);
    victim->curr_stats.Agi = BOUNDED(1, victim->curr_stats.Agi +1, 95);
    balance_affects(victim);
}

void spell_perm_increase_dex(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Dex >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your dexterity grow..\n", victim);
    victim->base_stats.Dex = BOUNDED(1, victim->base_stats.Dex +1, 95);
    victim->curr_stats.Dex = BOUNDED(1, victim->curr_stats.Dex +1, 95);
    balance_affects(victim);
}

void spell_perm_increase_con(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Con >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your constitition grow..\n", victim);
    victim->base_stats.Con = BOUNDED(1, victim->base_stats.Con +1, 95);
    victim->curr_stats.Con = BOUNDED(1, victim->curr_stats.Con +1, 95);
    balance_affects(victim);
}

void spell_perm_increase_luck(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Luk >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your luck grow..\n", victim);
    victim->base_stats.Luk = BOUNDED(1, victim->base_stats.Luk +1, 95);
    victim->curr_stats.Luk = BOUNDED(1, victim->curr_stats.Luk +1, 95);
    balance_affects(victim);
}
void spell_perm_increase_pow(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Pow >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your power grow..\n", victim);
    victim->base_stats.Pow = BOUNDED(1, victim->base_stats.Pow +1, 95);
    victim->curr_stats.Pow = BOUNDED(1, victim->curr_stats.Pow +1, 95);
    balance_affects(victim);
}

void spell_perm_increase_int(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Int >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your intelligence grow..\n", victim);
    victim->base_stats.Int = BOUNDED(1, victim->base_stats.Int +1, 95);
    victim->curr_stats.Int = BOUNDED(1, victim->curr_stats.Int +1, 95);
    balance_affects(victim);
}
void spell_perm_increase_wis(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Wis >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your wisdom grow..\n", victim);
    victim->base_stats.Wis= BOUNDED(1, victim->base_stats.Wis +1, 95);
    victim->curr_stats.Wis = BOUNDED(1, victim->curr_stats.Wis +1, 95);
    balance_affects(victim);
}
void spell_perm_increase_cha(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
    send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n", ch->in_room);

    if(victim->base_stats.Cha >= 95)
    {
        send_to_char("&+BNothing seem to happen..\n", victim);
        return;
    }

    send_to_char("&+BYou feel your charisma grow..\n", victim);
    victim->base_stats.Cha = BOUNDED(1, victim->base_stats.Cha +1, 95);
    victim->curr_stats.Cha = BOUNDED(1, victim->curr_stats.Cha +1, 95);
    balance_affects(victim);
}

//--------------------------------------------------------------
//                   PORTALS
//--------------------------------------------------------------

void spell_shadow_gate(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct portal_settings set = {
      782, /* portal type  */
      -1,  /* from room */
      -1,  /* to room */
      0,   /* How many can pass before closes */
      0,   /* Timeout before anyone can enter after open */
      0,   /* Timeout before next person can enter */
      0,   /* Lag person gets when steps out portal */
      0    /* Portal decay timer */
  };
  struct portal_create_messages msg = {
    /*ch   */ "&+LThe gate opens for a brief second and then closes.\n",
    /*ch r */ "&+LA shadow gate appears for a brief second, then closes.",
    /*vic  */ 0,
    /*vic r*/ 0,
    /*ch   */ "&+LA pitch black gate rises out of the ground!\n",
    /*ch r */ "&+LA pitch black gate rises out of the ground!\n",
    /*vic  */ "&+LA pitch black gate rises out of the ground!\n",
    /*vic r*/ "&+LA pitch black gate rises out of the ground!\n",
    /*npc  */ "You can only open a shadow gate to another player!\n",
    /*bad  */ 0
  };
  struct affected_type *afp;

  if(!ch)
    return;

  if(!victim)
    victim = ch;

  int specBonus = 0;
  set.to_room = victim->in_room;
  if(victim == ch)
  {
    if(affected_by_spell(ch, SPELL_BLOODSTONE))
    {
      afp = get_spell_from_char(ch, SPELL_BLOODSTONE);
      set.to_room = afp->modifier;
    }
  }
  int maxToPass          = get_property("portals.shadowportal.maxToPass", 5);
  set.init_timeout       = get_property("portals.shadowportal.initTimeout", 3);
  set.post_enter_timeout = get_property("portals.shadowportal.postEnterTimeout", 0);
  set.post_enter_lag     = get_property("portals.shadowportal.postEnterLag", 0);
  set.decay_timer        = get_property("portals.shadowportal.decayTimeout", 60 * 2);
  set.throughput = MAX(0, (int)( (ch->player.level-46) )) + number( 2, maxToPass + specBonus);

  if(    !can_do_general_portal(level, ch, victim, &set, &msg)
      //                || (!IS_TRUSTED(ch)     && (GET_MASTER(ch) && IS_PC(victim)) )
      //                || (!IS_TRUSTED(ch)     && (!OUTSIDE(ch) || !OUTSIDE(victim)) )
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

void spell_moonwell(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  P_obj moonstone;
  struct affected_type *afp;
    int      to_room, from_room;
    int      count;
    int      distance;
    bool     success = true;
  struct portal_settings set = {
      751, /* portal type  */
      -1,  /* from room */
      -1,  /* to room */
      0,   /* How many can pass before closes */
      0,   /* Timeout before anyone can enter after open */
      0,   /* Timeout before next person can enter */
      0,   /* Lag person gets when steps out portal */
      0    /* Portal decay timer */
  };
  struct portal_create_messages msg = {
      /*ch   */ "The well opens for a brief second and then closes.\n",
      /*ch r */ "A moonwell appears for a brief second, then closes.\n",
      /*vic  */ 0,
      /*vic r*/ 0,
      /*ch   */ "&+WSwirling, silvery mists fill the area, slowly forming a pool on the ground..\n",
      /*ch r */ "&+WSwirling, silvery mists fill the area, slowly forming a pool on the ground..\n",
      /*vic  */ "&+WSwirling, silvery mists fill the area, slowly forming a pool on the ground..\n",
      /*vic r*/ "&+WSwirling, silvery mists fill the area, slowly forming a pool on the ground..\n",
      /*npc  */ "You can only open a moonwell to another player!\n",
      /*bad  */ 0
  };

  if(!ch) return;

  if(!victim)
    victim = ch;

  if(IS_NPC(ch))
  return;

  if(IS_NPC(victim))
  return;

  if(victim == ch)
  {
    if(affected_by_spell(ch, SPELL_MOONSTONE))
    {
      afp = get_spell_from_char(ch, SPELL_MOONSTONE);
      to_room = afp->modifier;
      /* Removing this for now, as there's no need for checking this.. maybe later
       * for expanding the spell.
      for (moonstone = world[to_room].contents; moonstone; moonstone = moonstone->next_content)
      {
        if( moonstone && obj_index[moonstone->R_num].virtual_number == 419 && moonstone->value[0] == GET_PID(ch))
          break;
      }
    }
    if(!moonstone)
      //success = false;
    else
      success = false;
    */
    }
  }
  else
  {
    to_room = victim->in_room;
  }
  from_room = ch->in_room;

#if 0
  if(!IS_TRUSTED(ch) && (time_info.hour >= 6) && (time_info.hour <= 17))
  {
    send_to_char("&+WThe well opens for a brief second and is quickly evaporated by the sun.\n",
            ch);
    act("&+WA moonwell appears for a brief second, then is quickly evaporated by the sun.",
        FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
#endif
#if 0
  if(!OUTSIDE(ch))
  {
    send_to_char("You must be outside to cast this spell!\n", ch);
    return;
  }
#endif


  int specBonus = 0;
  set.to_room = to_room;
  int maxToPass          = get_property("portals.moonwell.maxToPass", 3);
  set.init_timeout       = get_property("portals.moonwell.initTimeout", 3);
  set.post_enter_timeout = get_property("portals.moonwell.postEnterTimeout", 0);
  set.post_enter_lag     = get_property("portals.moonwell.postEnterLag", 0);
  set.decay_timer        = get_property("portals.moonwell.decayTimeout", 60 * 2);

  //--------------------------------
  // spec affected changes
  //--------------------------------
  if( GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND)
      && world[ch->in_room].sector_type == SECT_FOREST )
  {
    specBonus = 2;
    set.decay_timer = (set.decay_timer / 2) * 3;
  }
  //--------------------------------
  //set.throughput = MAX(0, (int)( (ch->player.level-46)/2 )) + number( 2, maxToPass + specBonus);
  set.throughput = 20;

  if(    !can_do_general_portal(level, ch, victim, &set, &msg)
      //                || (!IS_TRUSTED(ch)     && (GET_MASTER(ch) && IS_PC(victim)) )
      //                || (!IS_TRUSTED(ch)     && (!OUTSIDE(ch) || !OUTSIDE(victim)) )
  )
  {
    act(msg.fail_to_caster,      FALSE, ch, 0, 0, TO_CHAR);
    act(msg.fail_to_caster_room, FALSE, ch, 0, 0, TO_ROOM);

    // play_sound(SOUND_MOONWELL, NULL, ch->in_room, TO_ROOM);
    return;
  }

  if(IS_NPC(victim) && !IS_TRUSTED(ch))
  {
    send_to_char(msg.npc_target_caster, ch);
    return;
  }

  spell_general_portal(level, ch, victim, &set, &msg);

  // play_sound(SOUND_MOONWELL, NULL, ch->in_room, TO_ROOM);
  // play_sound(SOUND_MOONWELL, NULL, set.to_room, TO_ROOM);
}

void spell_moonstone(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj tar_obj)
{
  P_obj    moonstone;
  struct affected_type af, *afp;
  int duration = level * 4 * WAIT_MIN;

  if(IS_NPC(ch))
  return;

  if(IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
      world[ch->in_room].sector_type == SECT_OCEAN) {
    send_to_char("The powers of nature ignore your call for serenity.\n", ch);
    return;
  }

  if(afp = get_spell_from_char(ch, SPELL_MOONSTONE))
  {
    moonstone = get_obj_in_list_num(real_object(419), world[afp->modifier].contents);
    if(moonstone)
      extract_obj(moonstone);
    afp->modifier = ch->in_room;
  }
  else
  {
    memset(&af, 0, sizeof(af));
    af.type = SPELL_MOONSTONE;
    af.flags = /*AFFTYPE_NOSHOW |*/ AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
    //af.flags = /*AFFTYPE_NOSHOW |*/ AFFTYPE_NOSAVE | AFFTYPE_NOAPPLY;
    af.modifier = ch->in_room;
    af.duration = duration / PULSES_IN_TICK;

    affect_to_char(ch, &af);
  }

  moonstone = read_object(real_object(419), REAL);

  if(!moonstone)
  {
    logit(LOG_DEBUG, "spell_moonstone(): obj 419 not loadable");
    return;
  }

  send_to_char
    ("&+BA shimmering stone begins to take shape....\n"
       "&+bThe stone rises into the &+cair&+b briefly, then shoots downward with amazing speed into the ground...\n"
       "&+CYou feel at one with the surroundings.\n",
        ch);
    act
    ("&+BA shimmering stone begins to take shape...\n"
     "&+bThe stone rises into the air briefly, then shoots downward with amazing speed into the ground.\n"
     "&+C$n glows with &n&+bpower.",
        FALSE, ch, 0, 0, TO_ROOM);

  set_obj_affected(moonstone, duration, TAG_OBJ_DECAY, 0);

  moonstone->value[0] = GET_PID(ch);

  obj_to_room(moonstone, ch->in_room);
}
bool can_do_general_portal( int level, P_char ch, P_char victim,
                            struct portal_settings *settings,
                            struct portal_create_messages *messages)
{
  int to_room;

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( ch == victim && !affected_by_spell(ch, SPELL_MOONSTONE)
    && !affected_by_spell(ch, SPELL_THOUGHT_BEACON)
    && !affected_by_spell(ch, SPELL_BLOODSTONE))
  {
    return FALSE;
  }

  to_room = settings->to_room;
  if(!IS_TRUSTED(ch) &&
       // target room check
       ((to_room == NOWHERE) || (to_room == ch->in_room) ||
        IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
        IS_HOMETOWN(ch->in_room) ||
        IS_HOMETOWN(to_room) ||
        IS_SET(world[to_room].room_flags, SINGLE_FILE) ||
        world[ch->in_room].sector_type == SECT_OCEAN ||
        IS_SET(world[to_room].room_flags, NO_MAGIC) ||
        IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
//        IS_NPC(ch) ||
        IS_PC_PET(ch) ||
        // victim check
        (victim &&
         IS_ELITE(victim) &&
         IS_GREATER_RACE(victim) &&
        ( IS_TRUSTED(victim) ||
          IS_AFFECTED3(victim, AFF3_NON_DETECTION) ||
          (IS_SET(victim->specials.act2, PLR2_NOLOCATE)
           && !is_linked_to(ch, victim, LNK_CONSENT)) ||
          (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE)
           && !is_introd(victim, ch)) ||
          (racewar(ch, victim) && !race_portal_check(ch, victim))
         )/* check used if victim is set */
        )
       )  /* check used if not trusted ch */
      )
   {
      return FALSE;
   }
  
/* We are allowing non-raidable ppl to port again. - Lohrr 7/29/2012
  if((ch && !is_Raidable(ch, 0, 0)) ||
     (victim && !is_Raidable(victim, 0, 0)))
  {
    send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
    return false;
  }
*/

   return TRUE;
}

struct portal_data {
  int pid;
  P_obj port1;
  P_obj port2;
  bool oneway;
};

void event_portal_owner_check(P_char ch, P_char vict, P_obj obj, void *data)
{
  struct portal_data *pdata = (struct portal_data*)data;
  P_char caster;
  P_obj portal1, portal2;
  char buf[MAX_STRING_LENGTH];

  if(!pdata)
  {
    debug("Passed null pointer to portal owner check.");
    return;
  }

  caster = find_player_by_pid(pdata->pid);
  portal1 = pdata->port1;
  portal2 = pdata->port2;

  if(!caster)
  {
    Decay(obj);
    return;
  }
  if( !pdata->oneway &&
       ((portal1->loc.room != caster->in_room) &&
        (portal2->loc.room != caster->in_room))
     )
  {
    Decay(obj);
    return;
  }
  if( pdata->oneway &&
       (portal1->loc.room != caster->in_room)
     )
  {
    Decay(obj);
    return;
  }
  add_event(event_portal_owner_check, WAIT_SEC, 0, 0, obj, 0, data, sizeof(struct portal_data));
}

//-----------------------------------------------------------------------
// this is called from portals hook functions
//-----------------------------------------------------------------------
bool spell_general_portal( int level, P_char ch, P_char victim, struct portal_settings *settings,
     struct portal_create_messages *messages, bool isOneWay )
{
  /* Portal obj settings
   * extra2_flags - not used anymore, was create time
   * value[0] - room number
   * value[1] - race
   * value[2] - throughput
   * value[3] - creator level
   * value[4] = init_timeout
   * value[5] = post_enter_timeout
   * value[6] = post_enter_lag
   * value[7] - portal ID
   * timer[0] - create time
   * timer[1] - last entry time
   */
  P_obj    portal1=NULL, portal2=NULL;
  int      to_room;
  char     logbuf[500];
  struct portal_data pdata;

  if( !ch )
  {
    return FALSE;
  }

  to_room = settings->to_room;
  // NOWHERE is a bad index, 0 is Limbo
  if( to_room == NOWHERE || to_room == 0 )
  {
    if( victim )
    {
      sprintf(logbuf, "Portal(%d) from %s(%d) [%d] to %s(%d) in [%s].", settings->R_num,
        J_NAME(ch), IS_NPC(ch) ? GET_VNUM(ch) : GET_PID(ch), world[ch->in_room].number,
        J_NAME(victim), IS_NPC(victim) ? GET_VNUM(victim) : GET_PID(victim), (to_room==NOWHERE) ? "NOWHERE" : "LIMBO");
      logit(LOG_PORTALS, logbuf);
      send_to_char("Spell messed up. contact someone.\n", ch);
      return FALSE;
    }
    else
    {
      sprintf(logbuf, "Portal(%d) from %s(%d) [%d] to [%s].", settings->R_num,
        J_NAME(ch), IS_NPC(ch) ? GET_VNUM(ch) : GET_PID(ch), world[ch->in_room].number,
        (to_room==NOWHERE) ? "NOWHERE" : "LIMBO");
      logit(LOG_PORTALS, logbuf);
      send_to_char("Spell messed up. contact someone.\n", ch);
      return FALSE;
    }
  }

  if( IS_CASTLE(ch->in_room) )
  {
    send_to_char("&+LThe nature of this room prevents you from creating a portal.&n\n", ch);
    return FALSE;
  }
  if( IS_CASTLE(to_room) )
  {
    send_to_char("&+LThe nature of that room prevents you from creating a portal.&n\n", ch);
    return FALSE;
  }

  portal1 = read_object(settings->R_num, VIRTUAL);
  if(!portal1)
  {
    sprintf(logbuf, "spell_portal(): obj %d not loadable", settings->R_num);
    logit(LOG_DEBUG, logbuf);
    send_to_char("Spell messed up. contact someone.\n", ch);
    return FALSE;
  }
  if(!isOneWay)
  {
    portal2 = read_object(settings->R_num, VIRTUAL);
    if(!portal2)
    {
      sprintf(logbuf, "spell_portal(): obj %d not loadable", settings->R_num);
      logit(LOG_DEBUG, logbuf);
      send_to_char("Spell messed up. contact someone.\n", ch);
      if(portal1)
      {
        extract_obj(portal1);
      }
      return FALSE;
    }
  }

  if(victim && !IS_TRUSTED(ch))
  {
    sprintf(logbuf, "Portal(%d) from %s(%d) in [%d] to %s(%d) in [%d].", settings->R_num,
      J_NAME(ch), IS_NPC(ch) ? GET_VNUM(ch) : GET_PID(ch), world[ch->in_room].number,
      J_NAME(victim), IS_NPC(victim) ? GET_VNUM(victim) : GET_PID(victim), world[to_room].number);
    logit(LOG_PORTALS, logbuf);
    sql_log(ch, PLAYERLOG, "Portal (%d) to %s in %d", settings->R_num, J_NAME(victim), world[to_room].number);
    // spam immo's if it looks like a possible camped target
    if( (world[to_room].number == GET_HOME(victim)) || (GET_LEVEL(victim) < 10) )
    {
      statuslog(57, logbuf);
    }
  }
  if( world[to_room].people )
  {
    if( victim && (ch!=victim) )
    {
      act(messages->open_to_victim_room, FALSE, ch, portal1, victim, TO_NOTVICTROOM);
      act(messages->open_to_victim,      FALSE, ch, portal1, victim, TO_VICT);
    }
    else
    {
      act(messages->open_to_victim_room, FALSE, world[to_room].people, portal1, 0, TO_ROOM);
      act(messages->open_to_victim_room, FALSE, world[to_room].people, portal1, 0, TO_CHAR);
    }
  }
  act(messages->open_to_caster_room, FALSE, ch, portal1, victim, TO_ROOM);
  act(messages->open_to_caster,      FALSE, ch, portal1, victim, TO_CHAR);

  portal1->value[0] = world[to_room].number;

  // set timers
  portal1->value[4] = settings->init_timeout;
  portal1->value[5] = settings->post_enter_timeout;
  portal1->value[6] = settings->post_enter_lag;

  set_obj_affected(portal1, settings->decay_timer, TAG_OBJ_DECAY, 0);

  if(!isOneWay)
  {
    portal2->value[0] = world[ch->in_room].number;
    // set timers
    portal2->value[4] = settings->init_timeout;
    portal2->value[5] = settings->post_enter_timeout;
    portal2->value[6] = settings->post_enter_lag;
     set_obj_affected(portal2, settings->decay_timer, TAG_OBJ_DECAY, 0);
  }

  set_up_portals(ch, portal1, portal2, settings->throughput);

  obj_to_room(portal1, ch->in_room);
  if(!isOneWay)
    obj_to_room(portal2, to_room);

  if(IS_PC(ch) && (settings->R_num != 752))
  {
    pdata.pid = GET_PID(ch);
    pdata.port1 = portal1;
    if(!isOneWay)
      pdata.port2 = portal2;
    pdata.oneway = isOneWay;
    add_event(event_portal_owner_check, WAIT_SEC, 0, 0, portal1, 0, &pdata, sizeof(pdata));
    add_event(event_portal_owner_check, WAIT_SEC, 0, 0, portal2, 0, &pdata, sizeof(pdata));
  }

  return TRUE;
}

//-----------------------------------------------------------------------
// best to call this only from spell_general_portal
//-----------------------------------------------------------------------
void set_up_portals(P_char ch, P_obj p1, P_obj p2, int charge)
{
   int temp;

   // set when portal is created, for initial stabilization
   p1->timer[0] = time(0);
   if(p2) p2->timer[0] = p1->timer[0];
   //----------------------------

   // reset entry timer, no stabilization for first entry
   p1->timer[1] = 0;
   if(p2) p2->timer[1] = 0;
   //----------------------------

   // [1]: set portal creator race
   p1->value[1] = GET_RACE(ch);
   if(p2) p2->value[1] = p1->value[1];
   //----------------------------

   // [2]: set portal charge (how many can pass, if -1 then no limit)
   p1->value[2] = charge;
   if(p2) p2->value[2] = charge;
   //----------------------------

   // set caster level
   p1->value[3] = GET_LEVEL(ch);
   if(p2) p2->value[3] = p1->value[3];
   //----------------------------

   // [7]: set portal id, relates portal1 with his another side portal2
   p1->value[7] = portal_id;
   if(p2) p2->value[7] = portal_id;
   portal_id++;
   //----------------------------
}

struct sb_data {
  int room;
  int spores;
};

void spell_spore_burst(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct sb_data sbdata;
  int garlic;

  switch (world[ch->in_room].sector_type)
   {
   case SECT_UNDRWLD_WILD:
   case SECT_SWAMP:
   case SECT_FOREST:
   case SECT_AIR_PLANE:
     break;
   default:
     send_to_char("You instinctively know this isn't ideal terrain...\n", ch);
     break;
   }
  
  garlic = get_spell_component(ch, VOBJ_INGRED_GARLIC, 1);
  if(!garlic)
  {
    send_to_char("You must have &+Wsome garlic&n in your inventory.\n", ch);
    act("&+W$n's&+G spell fizzles and dies before any growth can begin.\n", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  send_to_room("&+cA &+ysickly yellow &+ysphere &+cstarts to grow in the center of the room..\n", ch->in_room);

  sbdata.room = ch->in_room;
  sbdata.spores = 1;

  add_event(event_spore_burst, (int)(PULSE_VIOLENCE * 1.5), ch, 0, 0, 0, &sbdata, sizeof(struct sb_data));
  CharWait(ch,(int) 2.5 * PULSE_VIOLENCE);
}

void event_spore_burst(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct sb_data *sbdata = (struct sb_data*)data;
  int garlic;

  if(!sbdata)
  {
    debug("Passed null pointer to event_spore_burst.");
    return;
  }

  if(sbdata->room != ch->in_room)
  {
    send_to_char("&+GYour spell fizzles and the sphere of &+yspores &+Gcollapses!\n", ch);;
    act("&+W$n's&+G spell fizzles and the sphere of &+yspores &+Gcollapses!",
      TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
 
  garlic = 0;

  if(number(0, 1) && (sbdata->spores < 3))
  { 
    garlic = get_spell_component(ch, VOBJ_INGRED_GARLIC, 1);
    sbdata->spores++;
  }

  if(garlic)
  {
    send_to_room("&+CA mass of &+Yspores&+C coalesce into a growing sphere...\n", sbdata->room);
    add_event(event_spore_burst, (int)(PULSE_VIOLENCE), ch, 0, 0, 0, sbdata, sizeof(struct sb_data));
    return;  
  }
  else
  {
    send_to_room("&+gThe&+G immense &+ysphere of spores&+R erupts!\n", sbdata->room);
    cast_as_damage_area(ch, spell_spore_cloud, (int) sbdata->spores - 1 * 4 + GET_LEVEL(ch), NULL,
      get_property("spell.area.minChance.spore", 90),
      get_property("spell.area.chanceStep.spore", 10));
    return;
  }
}

void spell_spore_cloud(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages =
    {
    "&+ySickly spores&+c impale $N causing $m to writhe in pain!",
    "&+ySickly spores&+c rain down on you!",
    "&+cA cloud of &+yspores&+c engulfs $N!",
    "&+c$N is caught amidst a &+ycloud of spores&+c and is punctured to &+Rdeath!",
    "&+c$N is caught amidst a &+ycloud of spores&+c and is punctured to &+Rdeath!",
    0
    };

  if(!IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
    return;
 
  if(!NewSaves(victim, SAVING_BREATH, 0))
  {
    send_to_char("&+CYou scream in pain as &+yspores&+C lance into your flesh!\n", victim);
    act("$n &+Cscreams in pain as &+yhundreds of spores&+C lance into $m.", TRUE, victim,  0, 0, TO_ROOM);
    spell_disease( level, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }

  dam = 100 + level * 4 + number(1, 20);
 // Meteorswarm damage is: 100 + level * 6 + number(1, 40);
 
  switch (world[ch->in_room].sector_type)
  {
    case SECT_UNDERWATER:
    case SECT_UNDERWATER_GR:
    case SECT_UNDRWLD_WATER:
    case SECT_UNDRWLD_NOSWIM:
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
    case SECT_OCEAN:
    case SECT_FIREPLANE:
    case SECT_WATER_PLANE:
    case SECT_UNDRWLD_INSIDE:
    case SECT_INSIDE:
    case SECT_LAVA:
      dam = (int) (dam * 0.7);
      break;
    case SECT_CITY:
    case SECT_DESERT:
    case SECT_ROAD:
    case SECT_MOUNTAIN:
    case SECT_UNDRWLD_CITY:
    case SECT_EARTH_PLANE:
      dam = (int) (dam * 0.85);
      break;
    case SECT_UNDRWLD_WILD:
    case SECT_SWAMP:
    case SECT_FOREST:
    case SECT_AIR_PLANE:
      dam = (int) (dam * 1.15);
      break;
    default:
      dam = dam;
      break;
  }
  if(spell_damage(ch, victim, dam, SPLDAM_GAS, 0, &messages) != DAM_NONEDEAD)
    return;
}

void event_holy_dharma(P_char ch, P_char victim, P_obj obj, void *data)
{
  int hits = 0, maxhits = 0;
  P_char opponent;
  
  if(!ch) // Something amiss.
  {
    logit(LOG_EXIT, "event_holy_dharma called in magic.c without ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  {
    if(!IS_ALIVE(ch))
    {
      send_to_char("Lay still, you seem to be dead.\r\n", ch);
      return;
    }
    if(!IS_AFFECTED5(ch, AFF5_HOLY_DHARMA)) 
    {
      return;
    }
    if(!IS_AFFECTED2(ch, AFF2_SOULSHIELD))
    {
      send_to_char("Your soul is not properly prepared.\r\n", ch);
      if(IS_AFFECTED5(ch, AFF5_HOLY_DHARMA))
      {
        affect_from_char(ch, SPELL_HOLY_DHARMA);
      }
      return;
    }
    if(IS_AFFECTED5(ch, AFF5_HOLY_DHARMA)) 
    {
      if(IS_FIGHTING(ch))
      {
        opponent = ch->specials.fighting;

        if(GET_HIT(opponent) > GET_MAX_HIT(opponent) &&
           opponent)
        {
          hits = GET_HIT(opponent) - GET_MAX_HIT(opponent);
          maxhits = (int) (25 + (GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 2 *
            get_property("vamping.Dharma.maxhitMod", 1.000)));
          hits = BOUNDED(1, hits, maxhits);
          act("&+wYou have a marvelous &+Ycosmic &+wrevelation. An &+Cunseen force &+wflows into you!&n",
            FALSE, ch, 0, opponent, TO_CHAR);
          act("Your &+Llifeforce&n is assimilated into $n's body!", FALSE, ch, 0,
            opponent, TO_VICT);
          act("&+WTranquility&n spreads across $n's face.", FALSE, ch, 0,
            opponent, TO_NOTVICT);

          GET_HIT(opponent) -= hits;
          GET_HIT(ch) += hits;
        
          if(maxhits >= 45 &&
             opponent)
          {
            if(GET_CHAR_SKILL(ch, SKILL_DEVOTION) >= 20 &&
               IS_AFFECTED(ch, AFF_BLIND))
            {
              spell_cure_blind(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, 0);
            }
            if(GET_CHAR_SKILL(ch, SKILL_DEVOTION) >= 60 && 
               affected_by_spell(ch, SPELL_WITHER))
            {
              affect_from_char(ch, SPELL_WITHER);
            }
            if(GET_CHAR_SKILL(ch, SKILL_DEVOTION) >= 100 &&
               !IS_AFFECTED4(ch, AFF4_SANCTUARY))
            {
              spell_sanctuary(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, 0);
            }
          }
        }
      }
      if(IS_ALIVE(ch))
      {
        add_event(event_holy_dharma, PULSE_VIOLENCE * number(3, 8), ch,
            ch, 0, 0, 0, 0);
      }
    }
  }
}

void spell_holy_dharma(int level, P_char ch, char *arg, int type,
      P_char victim, P_obj obj)
{
  struct affected_type af;
      
  if(!ch) // Something amiss. Nov08 -Lucrot
  {
    logit(LOG_EXIT, "spell_holy_dharma called in magic.c without ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  {
    if(!IS_ALIVE(victim))
    {
      send_to_char("&+RLay still, you seem to be dead.\r\n", ch);
      return;
    }
    if(IS_AFFECTED5(victim, AFF5_HOLY_DHARMA)) 
    {
      send_to_char("&+cYour soul is already brushing upon the vast cosmos.\r\n", ch);
      return;
    }
    if(!IS_AFFECTED2(victim, AFF2_SOULSHIELD))
    {
      send_to_char("&+cYour soul is not properly prepared to embrace &+Crighteousness.\r\n", ch);
      return;
    }

    bzero(&af, sizeof(af));
    af.type = SPELL_HOLY_DHARMA;
    af.duration =  (int) (5 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5);
    af.bitvector5 = AFF5_HOLY_DHARMA;
    affect_to_char(victim, &af);
    
    send_to_char("&+cYou cast your soul into the cosmos seeking &+Wrighteousness.\r\n", ch);
    
    add_event(event_holy_dharma, (PULSE_VIOLENCE * number(3, 6)), ch,
      ch, 0, 0, 0, 0);

    return;
  }
}

void event_electrical_execution(P_char ch, P_char vict, P_obj obj, void *data)
{
  int dam, frytime;
  struct damage_messages messages = {
    "&+CElectrical arcs continue to &+Lblacken &+cand pincushion&n $N.",
    "The &+Celectrical arcs&n turn your exposed flesh &+Lblack!&n",
    "&+CElectrical arcs&n continue to &+Lblacken &+cand pincushion&n $N.",
    "$N's body &+Lsmokes&n and $M lifeless body turns &+Lblack.&n",
    "&+RYour brain overloads causing immediate death!!!&N",
    "$N's body &+Lsmokes&n as $M body is consumed by the &+Celectric arcs.&n",
    0
  };

  if(!ch)
  {
    logit(LOG_EXIT, "event_electrical_execution called in magic.c with no ch");
    raise(SIGSEGV);
  }
  if(!(vict) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(vict))
  {
    return;
  }
  
  frytime = *((int *) data);
  
  if((frytime >= 5 &&
     number(0, frytime)) ||
     frytime == 7)
  {
    act("&+cThe arcs of electricty surrounding you &+yground out!&n", FALSE, ch, 0, vict, TO_VICT);
    act("&+cThe arcs of electricity surrounding&n $N &+yground out!&n", FALSE, ch, 0, vict, TO_CHAR);
    act("&+cThe arcs of electricity surrounding&n $N &+yground out!&n", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }
  else
  {
    frytime++;
  }
  
  dam = (int) (GET_LEVEL(ch) * 2.5 + number(4, 20));

  if(frytime >= 2)
  {
    dam = (int) (GET_LEVEL(ch) * 1 - number(-20, 20));
  }
  
  if(frytime >= 4)
  {
    dam = (int) (GET_LEVEL(ch) * 0.75 + number(4, 12));
  }
  
  if(IS_ALIVE(vict) &&
    spell_damage(ch, vict, dam, SPLDAM_LIGHTNING, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD)
  { 
    if(vict &&
      IS_ALIVE(vict)) // Added another check due to reported double death bug.
    {
      add_event(event_electrical_execution, (int) (0.5 * PULSE_VIOLENCE), ch, vict, NULL, 0,
        &frytime, sizeof(frytime));
    
      if(6 > number(1, 10))
      {
        stop_memorizing(vict);
      }
    }
  }
}

void spell_electrical_execution(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int fry = 0;
  
  struct damage_messages messages = {
    "A huge shower of &=LBarcing electricity&n engulfs $N.",
    "A huge shower of &=LBarcing electricity&n from $n engulfs you.",
    "$N is engulfed by a shower of &=LBarcing electricity&n sent by $n.",
    "The shower of &=LBarcing electricity&n was more than $N could handle.",
    "Your hair and skin crackle and pop, then your brain activity stops!",
    "$N twitches and jerks violently to death from $n's shower of &=LBarcing electricity&n!",
      0
  };
  
  if(!(ch))
  {
    logit(LOG_EXIT, "spell_electrical_execution called in magic.c with no ch");
    raise(SIGSEGV);
  }
  
  if(!(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  if(ch &&
    victim)
  {
    engage(ch, victim);
  }

  if(IS_ALIVE(victim) &&
    spell_damage(ch, victim, (int) GET_LEVEL(ch) * 4 + number(4, 60),
      SPLDAM_LIGHTNING, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD);
  {
    if(ch &&
      IS_ALIVE(victim))
    {
      //gain_exp(ch, victim, 0, EXP_DAMAGE);
      add_event(event_electrical_execution, (int) (0.5 * PULSE_VIOLENCE), ch, victim,
        NULL, 0, &fry, sizeof(fry));
    }
  }
}

void spell_life_bolt(int level, P_char ch, char *arg, int type,
                             P_char victim, P_obj obj)
{
  struct damage_messages holy_messages = {
  "&+WYou sacrifice part of your lifeforce and send a pure white beam of &+Yholy energy&+W at $N&+W!",
  "&+wYou recoil in pain as $n &+wstretches out $s &+whands and a &+Wpure white&n&+w beam of &+Yholy energy&n&+w hits you dead-on!",
  "&+w$n stretches out $s hands and a &+Wpure white&n&+w beam of &+Yholy energy&n&+w hits $N &+wdead-on!",
  "&+WYou sacrifice part of your lifeforce and &+Rdisintegrate&n $N &+Wwith a pure white stream of &+Yholy energy&+W!",
  "&+w$n &+Rtears&+W your &+Ysoul&n&+W apart with a pure white stream of &+Yholy energy&+W beaming from $s outstretched hands!",
  "&+w$n stretches out $s hands and &+Rdisintegrates&n $N &+w with a &+Wpure white&n&+w beam of &+Yholy energy&n&+w!",
  };

  struct damage_messages unholy_messages = {
  "&+WYou sacrifice part of your lifeforce and send a &+Lblack beam&+W of &n&+munholy energy&+W at $N&+W!",
  "&+wYou recoil in pain as $n &+wstretches out $s &+whands and a &+Lblack beam of &+munholy energy&n&+w hits you dead-on!",
  "&+w$n stretches out $s hands and a &+Lwhite beam of &+munholy energy&n&+w hits $N &+wdead-on!",
  "&+WYou sacrifice part of your lifeforce and &+Rdisintegrate&n $N &+Wwith a stream of &+munholy energy&+W!",
  "&+w$n &+Rtears&+W your &+Ysoul&n&+W apart with a &+Lblack stream&n&+W of &+munholy energy&+W beaming from $s outstretched hands!",
  "&+w$n stretches out $s hands and &+Rdisintegrates&n $N &+w with a &+Lpure black&n&+w beam of &+munholy energy&n&+w!",
  };

  struct damage_messages neutral_messages = {
  "&+WYou sacrifice part of your lifeforce and send a white beam of &+Yholy energy&+W at $N&+W!",
  "&+wYou recoil in pain as $n &+wstretches out $s &+whands and a &+Wwhite&n&+w beam of &+Ypure energy&n&+w hits you dead-on!",
  "&+w$n stretches out $s hands and a &+Wwhite&n&+w beam of &+Yenergy&n&+w hits $N &+wdead-on!",
  "&+WYou sacrifice part of your lifeforce and &+Rdisintegrate&n $N &+Wwith a white stream of &+Yenergy&+W!",
  "&+w$n &+Rtears&+W your &+Ysoul&n&+W apart with a &+Wwhite stream of &+Yenergy&+W beaming from $s outstretched hands!",
  "&+w$n stretches out $s hands and &+Rdisintegrates&n $N &+w with a &+Wwhite&n&+w beam of &+Yenergy&n&+w!",
  };

  int dam, self_dam = 0, result;

  int num_missiles = BOUNDED(1, (level / 3), 5);
  
  bool opposing_align;
  
  dam = (dice(1, 4) * 4 + number(1, level))*num_missiles;

  if (!victim)
  {
    send_to_char("You need someone as a target to your spell.\r\n", ch);
    return;
  }

  if (get_property("spell.lifebolt.selfdam.lvl", 0.000) &&
      GET_LEVEL(ch) >= get_property("spell.lifebolt.selfdam.lvl", 0.000) &&
      GET_SPEC(ch, CLASS_THEURGIST, SPEC_TEMPLAR))
  {
    self_dam = dam;
    
    if (IS_PC(victim))
      self_dam >>= 2;
    else
      self_dam >>= 1;
 
    if (GET_HIT(ch) < self_dam)
    {
      send_to_char("&+WYou're too weak to sacrifice your life force in this way! You gain no bonus.\r\n", ch);
    }
    else if (self_dam)
    {
      send_to_char("&+WYou send a quick prayer to your Deity, offering your &+Rlifeforce&+W and asking for divine energy to flow through you!\r\n", ch);

      vamp(ch, self_dam/6, GET_MAX_HIT(ch) * (double)(BOUNDED(110, ((GET_C_POW(ch) * 10) / 9), 220) * .01));
      //if (spell_damage(ch, ch, self_dam, SPLDAM_HOLY, RAWDAM_NOKILL | SPLDAM_NOSHRUG, 0) != DAM_NONEDEAD)
	//        return;
    }
  }

  opposing_align = IS_OPPOSING_ALIGN(ch, victim);

  if (!opposing_align)
  { 
    dam >>= 1;
  }

  dam += self_dam;

  if (resists_spell(ch, victim))
    return;

  if (IS_EVIL(ch))
    spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &unholy_messages);
  else if (IS_GOOD(ch))
    spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &holy_messages);
  else
    spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &neutral_messages);

  if (opposing_align)
  { 
    if (IS_ALIVE(victim))
      send_to_char("&+WThe beam &+Rrends&+W your soul!\r\n", victim); 

    if (IS_ALIVE(ch))
      send_to_char("&+WYour victim seems to be in excruciating pain!\r\n", ch);
  }
}

void spell_mielikki_vitality(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool message = false;
  int healpoints = 3 * level + level / 2;

  if( affected_by_spell(ch, SPELL_ESHABALAS_VITALITY)
    || affected_by_spell(ch, SPELL_FALUZURES_VITALITY) )
  {
    send_to_char("&+GThe blessings of the Goddess Mielikki are denied!\r\n", victim);
    return;
  }

  if(affected_by_spell(ch, SPELL_VITALITY))
  {
    send_to_char("&+GThe Goddess Mielikki cannot further bless your vitality...\r\n", victim);
    return;
  }


  if(affected_by_spell(ch, SPELL_MIELIKKI_VITALITY))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_MIELIKKI_VITALITY)
      {
        af1->duration = 15;
        message = true;
      }

    if(message)
      send_to_char("&+GThe Goddess graces you.\r\n", victim);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_MIELIKKI_VITALITY;
  af.duration = 15;
  af.modifier = healpoints;
  af.location = APPLY_HIT;
  affect_to_char(victim, &af);

  if(GET_CLASS(ch, CLASS_DRUID))
  {
    af.modifier = level;
    af.location = APPLY_MOVE;
    affect_to_char(victim, &af);
  }

  if(GET_CLASS(ch, CLASS_RANGER) &&
     !affected_by_spell(ch, SPELL_REGENERATION) &&
     !affected_by_spell(ch, SPELL_ACCEL_HEALING))
  {
    af.modifier = number(20, 45);  
    af.location = APPLY_HIT_REG;
    affect_to_char(victim, &af);
  }

  /*  // They get endurance, no need for this.
  if((GET_CLASS(ch, CLASS_RANGER) &&
     !affected_by_spell(ch, SPELL_ENDURANCE)) ||
     GET_SPEC(ch, CLASS_RANGER, SPEC_HUNTSMAN))
  {
    af.modifier = number(25, 50);
    af.location = APPLY_MOVE_REG;
    affect_to_char(victim, &af);
  }
  */

  send_to_char("&+GYou feel the &+ywarm &+Gbreath of the Goddess Mielikki.\r\n", ch);
}

void spell_repair_one_item(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if(obj)
  {
    obj->condition = 100;
    act("$q glows a faint &+Yyellowish hue&n as a wave of energy covers its surface.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n's $q glows a faint &+Yyellowish hue&n as a wave of energy covers its surface.", FALSE, ch, obj, 0, TO_ROOM);    
  }
}

void spell_corpse_portal(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  P_obj    tobj;
  int      found = 0;
  int      location;

  // find most recent corpse
  for (tobj = object_list; tobj; tobj = tobj->next)
  {
    if(400220 == obj_index[tobj->R_num].virtual_number &&
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
      ("&+L$n attempts to call upon the spirit realm, but is unable to make a connection.&N",
       TRUE, ch, 0, 0, TO_ROOM);
    act
      ("&+gThe &+Gspirit &+grealm fails to answer your call for help.&N",
       FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  location = world[ch->in_room].number;

  act
    ("&+G$n's &+gbody begins to shake violently as if possessed by some &+Gother worldly &+gpower. With a blast of &+Gghastly &+Yflames&+g a portal suddenly materializes before them.&N",
     TRUE, ch, 0, 0, TO_ROOM);
  act
    ("&+gYour body calls out to the &+Gspirit&+g realm, begging for assistance.  Every fiber within your body begins to &+Gtingle&+g as a &+Gghastly &+gessence begins to fill the room. Suddenly and without warning, a &+Gmystic &+gportal materializes before you, beckoning you to enter.&N",
     FALSE, ch, 0, 0, TO_CHAR);
  obj_from_room(tobj);
  obj_to_room(tobj, real_room(location));

}

int has_soulbind(P_char ch)
{
  int result = 0;
  struct affected_type *findaf, *next_af;

  for( findaf = ch->affected; findaf; findaf = next_af )
  {
    next_af = findaf->next;
    if( findaf->type == TAG_SOULBIND )
    {
      result = findaf->modifier;
      break;
    }
  }
  return result;
}

void remove_soulbind(P_char ch)
{
  P_obj obj;
//  bool found = FALSE;

  // find any instance of their soulbound item and remove it
  for( obj = object_list; obj; obj = obj->next )
  {
    if( IS_SET((obj)->extra2_flags, ITEM2_SOULBIND) && isname(GET_NAME(ch), obj->name) )
    {
      extract_obj(obj);
    }
  }
}

void do_soulbind(P_char ch, char *argument, int cmd)
{
  P_obj    obj;
  P_char   victim;
  char     gbuf1[MAX_STRING_LENGTH], gbuf2[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH], gbuf3[MAX_STRING_LENGTH], bufbug[MAX_STRING_LENGTH];
	struct affected_type af, *findaf;

  argument_interpreter(argument, gbuf1, gbuf2);

  if( IS_TRUSTED(ch) )
  {
    if( !*gbuf1 || !(victim = ParseTarget(ch, gbuf1)) )
    {
      send_to_char("To Remove Character's Soulbound Status: soulbind <char>\r\n", ch);
      send_to_char("To Set Character's Soulbound Status: soulbind <char> <item>\r\n", ch);
      return;
    }

    for( findaf = victim->affected; findaf; findaf = findaf->next )
    {
      if( findaf->type == TAG_SOULBIND )
      {
        remove_soulbind(victim);
        affect_remove(victim, findaf);
        sprintf(buffer, "%s", GET_NAME(victim));
        sprintf(gbuf3, "Cleared soulbind status on %s.\r\n", buffer);
        send_to_char(gbuf3, ch);
        logit( LOG_WIZ, "%s cleared soulbind on %s", J_NAME(ch), J_NAME(victim));
        // If we're not setting a new soulbound item.
        if( !*gbuf2 )
        {
          return;
        }
        break;
      }
    }
  }
  else
  {
    victim = ch;
  }

  if( has_soulbind(victim) != 0 )
  {
    remove_soulbind(victim);
    send_to_char("&+yYour &+Ysoul &+ycalls out to the world to bring forth your &+ritem&+y...\r\n", victim);
    load_soulbind(victim);
    send_to_char("&+yAfter a brief moment, you feel &+Wwhole&+y once again, ready to &+rconquer &+ythe world.\r\n", victim);
    return;
  }

  // If victim doesn't have soulbind, and God isn't setting it for them.
  if( get_frags(victim) < 2000 && !IS_TRUSTED(ch) )
  {
    send_to_char("&+LYou have not yet earned the right to use that ability.\r\n", victim);
    return;
  }

  if( IS_TRUSTED(ch) ? *gbuf2 : *gbuf1 )
  {
    // We look for the item in the God/high fragger's inventory (not necessarily the victim).
    if( !(obj = get_obj_in_list(IS_TRUSTED(ch) ? gbuf2 : gbuf1, ch->carrying)) )
    {
      send_to_char("&+rYou must be carrying the item you wish to &+Wsoulbind&+r in your inventory.&n\r\n", ch);
      return;
    }

    // If the victim has a soulbound item already (note: for God setting for someone, it was cleared above).
    if( affected_by_spell(victim, TAG_SOULBIND) )
    {
      send_to_char("&+rA character may only &+Wsoulbind&+r once.&n\r\n", victim);
      return;
    }

    // Make sure object is valid type, not arti, etc.
    if( (obj->type == ITEM_CONTAINER || obj->type == ITEM_STORAGE || obj->type == ITEM_TREASURE
      || obj->type == ITEM_POTION ||  obj->type == ITEM_TELEPORT || obj->type == ITEM_WAND
      || obj->type == ITEM_KEY || obj->contains || obj->type == ITEM_FOOD
      || IS_OBJ_STAT2(obj, ITEM2_STOREITEM) || IS_OBJ_STAT2(obj, ITEM2_SOULBIND)
      || IS_OBJ_STAT(obj, ITEM_NOSELL) || IS_SET(obj->extra_flags, ITEM_ARTIFACT)) )
    {
      // This message goes to the function caller, not necessarily the victim.
      send_to_char("That item is not a valid type to &+Wsoulbind&n.\r\n", ch);
      return;
    }

    memset(&af, 0, sizeof(struct affected_type));
    af.type = TAG_SOULBIND;
    af.modifier = (obj_index[obj->R_num].virtual_number);
    af.duration = -1;
    af.location = 0;
    af.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM | AFFTYPE_NODISPEL;
    affect_to_char(victim, &af);

    // Restring item with chars name as a possible argument.
    sprintf(gbuf2, "%s %s", GET_NAME(victim), obj->name);
    // Free old name if strung.
    if( (obj->str_mask & STRUNG_KEYS) && obj->name )
    {
      str_free(obj->name);
    }
    obj->str_mask |= STRUNG_KEYS;
    obj->name = str_dup(gbuf2);

    sprintf(buffer, "%s &+Lbearing the &+Wsoul&+L of &+r%s&n", obj->short_description, GET_NAME(victim));
    set_short_description(obj, buffer);

    SET_BIT(obj->extra_flags, ITEM_NOSELL);
    SET_BIT(obj->extra_flags, ITEM_NORENT);
    SET_BIT(obj->extra2_flags, ITEM2_CRUMBLELOOT);
    SET_BIT(obj->extra2_flags, ITEM2_SOULBIND);
    // So our item updates right.
    do_save_silent(victim, 1);

    // Transfer the object to victim if necessary.
    if( ch != victim )
    {
      obj_from_char(obj);
      obj_to_char(obj, victim);
    }

    // Send messages to the victim.
    act("&+W$n &+rbegins to chant loudly, calling forth the &+Bblood &+rof their enemies. &+W$n's &+rhands begin to &+Rg&+rl&+Ro&+rw &+Rbrightly &+ras drops of blood begin to form.\r\n"
      "&+W$n &+rgently takes the &+Rblood&+r and begins to spread it about their $p&+r, which starts to glow with an &+Lun&+rho&+Lly &+Rlight.&N",
      TRUE, victim, obj, 0, TO_ROOM);
    act("&+rYou begin to chant loudly, calling forth the &+Bblood &+rof your enemies. Your &+rhands begin to &+Rg&+rl&+Ro&+rw &+Rbrightly &+ras drops of blood begin to form.\r\n"
      "&+rYou &+rgently take the &+Rblood&+r and begin to spread it about your $p&+r, which starts to glow with an &+Lun&+rho&+Lly &+Rlight.&N",
      FALSE, victim, obj, 0, TO_CHAR);
  }
  else
  {
    send_to_char("What item would you like to &+Wsoulbind&n?\r\n", ch);
  }
}

void load_soulbind(P_char ch)
{
  int item;
  P_obj obj;
    char     gbuf1[MAX_STRING_LENGTH], gbuf2[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];

  item = has_soulbind(ch);
  if(item == 0)
  {
   send_to_char("&+rYour &+Wsoul &+rhas not bound with anything yet.&n\r\n", ch);
   return;
  }
 /* sprintf(gbuf2, "%d", item);
  send_to_char(gbuf2, ch);*/
  obj = read_object(item, VIRTUAL);
           sprintf(gbuf2, "%s %s", GET_NAME(ch), obj->name);
  	    obj->name = str_dup(gbuf2);
	    sprintf(buffer, "%s &+Lbearing the &+Wsoul&+L of &+r%s&n", obj->short_description, GET_NAME(ch));
	   set_short_description(obj, buffer);
  obj_to_char(obj, ch); 
  SET_BIT(obj->extra_flags, ITEM_NOSELL);
  SET_BIT(obj->extra_flags, ITEM_NORENT);
  SET_BIT(obj->extra2_flags, ITEM2_CRUMBLELOOT);
  SET_BIT(obj->extra2_flags, ITEM2_SOULBIND); 
    REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
    REMOVE_BIT(obj->extra_flags, ITEM_INVISIBLE);
    SET_BIT(obj->extra_flags, ITEM_NOREPAIR);
    REMOVE_BIT(obj->extra_flags, ITEM_NODROP);

}

void spell_contain_being(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj obj)
{
  struct affected_type af;
 
 if(!victim)
 {
  send_to_char("You must specify a target for this spell!\r\n", ch);
  return;
 }
 
 if(victim == ch)
 {
  send_to_char("Haha, very funny.\r\n", ch);
  return;
 }

 if(IS_PC(victim))
 {
  send_to_char("You cannot learn to contain players.\r\n", ch);
  return;
 }
 
 if(IS_PC_PET(victim))
 {
  send_to_char("You cannot contain other's pets.\r\n", ch);
  return;
 }

   act("&n$n &npoints at &+L$N &nwhose form begins to &+Lp&+Mh&+Wa&+ms&+Be &nin and out from this plane of existence...", FALSE, ch, 0, victim, TO_NOTVICT);
   act("&nYou &npoint at &+L$N &nwhose form begins to &+Lp&+Mh&+Wa&+ms&+Be &nin and out from this plane of existence...", FALSE, ch, 0, victim, TO_CHAR);
   act("&n$n &npoints at &+L$N &nwhose form begins to &+Lp&+Mh&+Wa&+ms&+Be &nin and out from this plane of existence...", FALSE, ch, 0, victim, TO_VICT);

 
   memset(&af, 0, sizeof(af));
  af.type = SPELL_CONTAIN_BEING;
  af.duration = 100;
  af.flags = AFFTYPE_SHORT;
  affect_to_char_with_messages(victim, &af, "You your physical composure returns to normal.", "$n's &+Yphysical composure slowly returns to &+Lnormal&n.");

 
}
