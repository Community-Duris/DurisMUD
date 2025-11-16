/****************************************************************************
 *
 *  File: sillusionist.c                   Part of Durismud
 *  Usage: Illusionist spells woowoo!
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Copyright  2000, 2003 - durismud project
 *  Created by: Ilienze                   Date: 2002-08-30
 *****************************************************************************
 */

#ifndef _ILLUSIONIST_MAGIC_C_

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "mm.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "weather.h"
#include "sound.h"
#include "objmisc.h"
#include "specs.prototypes.h"
#include "damage.h"
#include "disguise.h"
#include "sql.h"
#include "ctf.h"
#include "vnum.mob.h"

/*
 * external variables
 */
extern P_index obj_index;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_char combat_list;
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern const char *apply_types[];
extern const flagDef extra_bits[];
extern const flagDef anti_bits[];
extern const char *item_types[];
extern const struct stat_data stat_factor[];
extern const char *dirs[], *dirs2[];
extern const int rev_dir[];
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int hometown[];
extern struct str_app_type str_app[];
extern struct con_app_type con_app[];
extern struct time_info_data time_info;
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern const char *undead_type[];
extern const int numCarvables;
extern const char *carve_part_name[];
extern const struct race_names race_names_table[];
extern const int carve_part_flag[];
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;

extern int cast_as_damage_area(P_char,
                               void (*func) (int, P_char, char *, int, P_char,
                                             P_obj), int, P_char, float,
                               float);


extern bool exit_wallable(int room, int dir, P_char ch);
extern bool create_walls(int room, int exit, P_char ch, int level, int type, int power, int decay, char *short_desc,
  char *desc, ulong flags);
extern bool has_skin_spell(P_char);

#define TITAN_NUMBER 650


/* Let's do some spells yah! */

void spell_phantom_armor(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool shown;

  if( !(victim && ch) )
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if( !IS_AFFECTED(victim, AFF_ARMOR) )
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PHANTOM_ARMOR;
    af.duration = 15;
    af.modifier = -(level);
    af.bitvector = AFF_ARMOR;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
    send_to_char("&+LShadowy &+Wphantoms&N blur your image to the outside!\r\n", victim);
  }
  else
  {
    struct affected_type *af1;

    shown = FALSE;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if (af1->type == SPELL_PHANTOM_ARMOR)
      {
        shown = TRUE;
        send_to_char("&+LShadowy &+Wphantoms&N blur your image to the outside!\r\n", victim);
        af1->duration = 15;
        break;
      }
    }
    if( !shown )
    {
      send_to_char( "&+WYou're already affected by an armor-type spell.&n\n", victim );
    }
  }
}

void spell_shadow_monster(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  int      summoned = 0;
  struct char_link_data *cld;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( CHAR_IN_SAFE_ROOM(ch) )
  {
    send_to_char("A mysterious force blocks your conjuring!\r\n", ch);
    return;
  }

  for(cld = ch->linked; cld; cld = cld->next_linked)
  {
    if (cld->type == LNK_PET &&
        IS_NPC(cld->linking) &&
        GET_VNUM(cld->linking) == 1106)
    {
      summoned++;
    }
  }

  // So, no shadow monster potions or scrolls then (save ones used by illusionists)?
  if(summoned >= MAX(1, GET_LEVEL(ch) / 14) || !GET_CLASS(ch, CLASS_ILLUSIONIST))
  {
    send_to_char("You cannot summon any more shadows!\r\n", ch);
    return;
  }

  mob = read_mobile(real_mobile(1106), REAL);
  if (!mob)
  {
    logit(LOG_DEBUG, "spell_shadow_monster(): mob %d not loadable", 1106);
    send_to_char("Bug in shadow monster.  Tell a god!\r\n", ch);
    return;
  }

  char_to_room(mob, ch->in_room, 0);
  act("$n &+wappears from nowhere!", TRUE, mob, 0, 0, TO_ROOM);

  mob->player.level = BOUNDED(1, number(level - 5, level + 1), 55);

  mob->player.m_class = 0;
  remove_plushit_bits(mob);

  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob), 3) + (GET_LEVEL(mob) * 2);

  setup_pet(mob, ch, 1, PET_NOORDER | PET_NOCASH);
  
  GET_EXP(mob) = 0;
  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(ch);
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(ch) / 2;
  mob->points.damnodice = (int) GET_LEVEL(ch) / 5;
  mob->points.damsizedice = (int) GET_LEVEL(ch) / 6;
// play_sound(SOUND_ELEMENTAL, NULL, ch->in_room, TO_ROOM);

  group_add_member(ch, mob);
  MobStartFight(mob, victim);
}


void spell_insects(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  int      summoned, i;
  struct char_link_data *cld;

  if( !ch || !victim )
  {
    return;
  }

  if( CHAR_IN_SAFE_ROOM(ch) )
  {
    send_to_char("A mysterious force blocks your conjuring!\r\n", ch);
    return;
  }

  summoned = 0;

  for(cld = ch->linked; cld; cld = cld->next_linked)
  {
    if (cld->type == LNK_PET &&
        IS_NPC(cld->linking) &&
        GET_VNUM(cld->linking) == 1107)
    {
      summoned++;
    }
  }

  for (i = MAX(1, GET_LEVEL(ch) / 14); i > summoned; i--)
  {
    mob = read_mobile(real_mobile(1107), REAL);

    if (!mob)
    {
      logit(LOG_DEBUG, "spell_insects(): mob %d not loadable", 1107);
      send_to_char("Bug in insects.  Tell a god!\r\n", ch);
      return;
    }

    char_to_room(mob, ch->in_room, 0);
    act("$n &+Lappears from nowhere!", TRUE, mob, 0, 0, TO_ROOM);

    mob->player.level = MIN(30, MAX(1, number(level - 7, level - 2)));

    SET_BIT(mob->specials.affected_by, AFF_HASTE);
    SET_BIT(mob->specials.act, ACT_SPEC_DIE);

    if (GET_LEVEL(ch) > 31)
    {
      SET_BIT(mob->specials.affected_by3, AFF3_BLUR);
    }
    if (GET_LEVEL(ch) > 50)
    {
      SET_BIT(mob->specials.affected_by2, AFF2_FLURRY);
    }

    mob->player.m_class = 0;
    remove_plushit_bits(mob);
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
      dice(GET_LEVEL(mob), 3) + (GET_LEVEL(ch));

    GET_EXP(mob) = 0;
    mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(ch);
    mob->points.base_damroll = mob->points.damroll = (int)(GET_LEVEL(ch) / 3);
    mob->points.damnodice = (int)(GET_LEVEL(ch) / 8);
    mob->points.damsizedice = (int)(GET_LEVEL(ch) / 9);

    setup_pet(mob, ch, 1, PET_NOORDER | PET_NOCASH);
    group_add_member(ch, mob);
    MobStartFight(mob, victim);

    if( !GET_CLASS(ch, CLASS_ILLUSIONIST) )
      return;
  }
}



void spell_boulder(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "Your huge &+Willusional boulder&n hits $N causing $M to scream out in agony!",
    "A huge &+Wboulder&n created by $n&n hits you causing you to scream out in agony!",
    "A huge &+Wboulder&n created by $n&n hits $N causing $M to scream out in agony!",
    "Your &+Willusional boulder&n rolls into $N's body, killing $M instantly!",
    "$n&n's giant &+Wboulder&N is the last thing you ever see.",
    "$n&n's &+Wboulder&N rolls into $N flattening $M!", 0
  };

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "Damm boulders it's messed up!");
    raise(SIGSEGV);
  }

  dam = dice(2 * level, 8);
  
  if(NewSaves(victim, SAVING_SPELL, 0))
    dam >>= 1;

  if (spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_GLOBE, &messages)
      != DAM_NONEDEAD)
    return;

   if(!IS_AFFECTED2(victim, AFF2_GLOBE) &&
     (GET_POS(victim) == POS_STANDING) &&
     !IS_ELITE(victim) &&
     !IS_GREATER_RACE(victim))
   {

     if(!number(0, 9))
     {
      act("&+LYour &+Wboulder&n &+Lhit's $N dead on, knocking $M to the ground!&n",
        0, ch, 0, victim, TO_CHAR);
      act("$N is knocked to the ground as $n's &+Wboulder hit's $M dead on!&n",
        0, ch, 0, victim, TO_NOTVICT);
      act("$n's &+Wboulder hit's dead on knocking you to the ground!&n",
        0, ch, 0, victim, TO_VICT);
      SET_POS(victim, POS_SITTING + GET_STAT(victim));
      CharWait(victim, (int)(PULSE_VIOLENCE * 0.5));
     }
   }
}


void spell_shadow_travel(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      location;
  char     buf[256] = { 0 };
  P_char   tmp = NULL;
  int      distance;

  if (!ch)
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  
  if(GET_SPEC(ch, CLASS_ILLUSIONIST, SPEC_DARK_DREAMER))
    CharWait(ch, 4);
  else if(IS_PC(ch))
    CharWait(ch, 36);
  else
    CharWait(ch, 5);

  if (!victim)
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }

  if (IS_AFFECTED3(victim, AFF3_NON_DETECTION))
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }
  
  if (IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) ||
      world[ch->in_room].sector_type == SECT_OCEAN ||
      IS_HOMETOWN(ch->in_room) )
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }
  
  if (IS_PC(victim) &&
      IS_SET(victim->specials.act2, PLR2_NOLOCATE) &&
      !is_introd(victim, ch))
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }

  location = victim->in_room;

  if (IS_ROOM(location, ROOM_NO_TELEPORT) ||
      IS_HOMETOWN(location) ||
      world[location].sector_type == SECT_CASTLE ||
      world[location].sector_type == SECT_CASTLE_GATE ||
      world[location].sector_type == SECT_CASTLE_WALL ||
      racewar(ch, victim) || (GET_MASTER(ch) && IS_PC(victim)))
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }
  P_char rider = get_linking_char(victim, LNK_RIDING);
  if( IS_NPC(victim) && rider && !IS_TRUSTED(ch) )
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  distance = (int)(level * 1.35);

  if(GET_SPEC(ch, CLASS_ILLUSIONIST, SPEC_DARK_DREAMER))
    distance += 15;

  if( !IS_TRUSTED(ch) && (( how_close(ch->in_room, victim->in_room, distance) < 0 )) )
//    || (how_close(victim->in_room, ch->in_room, distance) < 0)) )
  {
    send_to_char("&+yYou failed.\r\n", ch);
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
    if ((IS_AFFECTED(tmp, AFF_BLIND) ||
         (tmp->specials.z_cord != ch->specials.z_cord) || (tmp == ch) ||
         !number(0, 5)) && (IS_PC(ch) && !IS_TRUSTED(ch)))
      continue;
    if (CAN_SEE(tmp, ch))
      act
        ("&+L$n appears to become one with the shadows and silently starts to move away.",
         FALSE, ch, 0, tmp, TO_VICT);
    else
      act("&+LAn odd looking shadow forms and silently starts to move away.",
          FALSE, ch, 0, tmp, TO_VICT);
    send_to_char(buf, tmp);
  }

  char_from_room(ch);
  char_to_room(ch, location, -1);

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if ((IS_AFFECTED(tmp, AFF_BLIND) || (tmp == ch) || !number(0, 5)) &&
        (IS_PC(ch) && !IS_TRUSTED(ch)))
      continue;
    if (CAN_SEE(tmp, ch))
      act("&+LAn odd looking shadow silently arrives and slowly forms into $n.",
         FALSE, ch, 0, tmp, TO_VICT);
    else
      act("&+LAn odd looking shadow silently arrives then vanishes.",
        FALSE, ch, 0, tmp, TO_VICT);
    send_to_char(buf, tmp);
  }
}


void spell_stunning_visions(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int percent = level + number(-10, 10);

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || GET_HIT(victim) < 1
    || IS_TRUSTED(victim) || IS_ZOMBIE(victim) || GET_RACE(victim) == RACE_GOLEM
    || GET_RACE(victim) == RACE_PLANT || IS_GREATER_RACE(victim) )
  {
    send_to_char("&+rYour target is immune!\r\n", ch);
    return;
  }

  if(resists_spell(ch, victim))
  {
    return;
  }

  if( IS_STUNNED(victim) )
  {
    send_to_char("Your target is already stunned!\r\n", ch);
    return;
  }

  // If they're bashed/tripped/etc no stun until they at least come out of the lag.
  //   Allow stun if they failed bash, or springleap.
  if( GET_POS(victim) != POS_STANDING && IS_ACT2(victim, PLR2_WAIT)
    && !(affected_by_spell(victim, SKILL_BASH) || affected_by_spell(ch, SKILL_SPRINGLEAP)) )
  {
    act("&+Y$N&+Y seems to be preoccupied at the moment.&n\r\n", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

  if( NewSaves(ch, SAVING_SPELL, 0) )
  {
    percent /= 2;
  }

  if( IS_PC_PET(victim) )
  {
    percent *= 2;
  }

  percent += (level - GET_LEVEL(victim));

  //percent += (GET_C_POW(ch) - GET_C_POW(victim)) / 3;
  percent += (int)((GET_C_POW(ch) - GET_C_POW(victim)) *.8);

  act("&+cYou send a wave of incredible visions toward&n $N.", FALSE, ch, 0, victim, TO_CHAR);

  if( percent < 11 )
  {
    act("$n &+ctries to mesmerize you with some stunning visions, but fails miserably!\r\n",
      FALSE, ch, 0, victim, TO_VICT);
    act("&+cYour visions fail to impress your foe...", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

  if (percent > 90)
  {
    act("&+cYour wave of visions seems to completely entrance&n $N!", FALSE, ch, 0, victim, TO_CHAR);
    act("$n's &+cstunning visions seem to completely entrance&n $N!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n &+ccauses visions to dance before your eyes, causing you to fall in a deep trance!&n.",
      FALSE, ch, 0, victim, TO_VICT);

    if( IS_FIGHTING(victim) )
    {
      stop_fighting(victim);
    }
    if( IS_DESTROYING(victim) )
    {
      stop_destroying(victim);
    }

    stop_fighting(ch);
    Stun(victim, ch, PULSE_VIOLENCE * 2, FALSE);
    return;
  }
  else if (percent > 60)
  {
    act("&+cYour wave of visions seems to entrance&n $N!", FALSE, ch, 0, victim, TO_CHAR);
    act("$n's &+Cstunning visions seem to entrance&n $N!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n &+ccauses visions to dance before your eyes, causing you to fall into a trance!&n.",
      FALSE, ch, 0, victim, TO_VICT);

    if( IS_FIGHTING(victim) )
    {
      stop_fighting(victim);
    }
    if( IS_DESTROYING(victim) )
    {
      stop_destroying(victim);
    }

    stop_fighting(ch);
    Stun(victim, ch, PULSE_VIOLENCE, FALSE);
    return;
  }
  else if (percent > 40)
  {
    act("&+cYour wave of visions seems to mesmerize&n $N!", FALSE, ch, 0, victim, TO_CHAR);
    act("$n's &+cstunning visions seem to mesmerize&n $N!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n &+ccauses visions to dance before your eyes mesmerizing you!&n.",
      FALSE, ch, 0, victim, TO_VICT);

    if( IS_FIGHTING(victim) )
    {
      stop_fighting(victim);
    }
    if( IS_DESTROYING(victim) )
    {
      stop_destroying(victim);
    }

    stop_fighting(ch);
    Stun(victim, ch, PULSE_VIOLENCE / 2, FALSE);
    return;
  }
  else if (percent >= 25)
  {
    act("&+cYour wave of visions seems to slightly mesmerize&n $N!", FALSE, ch, 0, victim, TO_CHAR);
    act("$n's &+cstunning visions seem to slightly mesmerize&n $N!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n &+ccauses visions to dance before your eyes, bewildering you!&n.",
      FALSE, ch, 0, victim, TO_VICT);

    if( IS_FIGHTING(victim) )
    {
      stop_fighting(victim);
    }
    if( IS_DESTROYING(victim) )
    {
      stop_destroying(victim);
    }

    stop_fighting(ch);
    Stun(victim, ch, PULSE_VIOLENCE/4, FALSE);
    return;
  }
}


void spell_reflection(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  P_char   image, tch, tch2;
  struct char_link_data *cld;
  struct affected_type af;
  char Gbuf1[MAX_STRING_LENGTH];
  int numb, i, spot, room, targ;
  struct follow_type *k, *p, *l, *q;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_NPC(victim) && (IS_PC(ch) || IS_PC_PET(ch)) )
  {
    send_to_char("You cannot cast this spell on NPCs.\r\n", ch);
    return;
  }

  for( cld = ch->linked; cld; cld = cld->next_linked )
  {
    if( cld->type == LNK_PET && IS_NPC(cld->linking) && GET_VNUM(cld->linking) == TITAN_NUMBER )
    {
      send_to_char("&+LBut you cant support so many different illusions!&n\r\n", ch);
      return;
    }
  }

  for( k = ch->followers; k; k = p )
  {
    tch = k->follower;
    p = k->next;

    if( tch && IS_NPC(tch) && GET_VNUM(tch) == 250 )
    {
      stop_fighting(tch);
      if( IS_DESTROYING(tch) )
        stop_destroying(tch);
      StopAllAttackers(tch);
      extract_char(tch);
    }
  }

  if( ch != victim )
  {
    for( l = victim->followers; l; l = q )
    {
      tch2 = l->follower;
      q = l->next;

      if( tch2 && IS_NPC(tch2) && GET_VNUM(tch2) == 250 )
      {
        stop_fighting(tch2);
        if( IS_DESTROYING(tch2) )
          stop_destroying(tch2);
        StopAllAttackers(tch2);
        extract_char(tch2);
      }
    }
  }


  numb = BOUNDED(1, (level - 26) / 5, 4);

  spot = number(0, numb);
  targ = number(0, numb);

  act("&+L$n &+Cs&+Bs&+bs&+Cp&+Bp&+b&+Cl&+Bl&+bl&+Ci&+Bi&+bi&+Ct&+Bt&+bt&+Cs&+Bs&+bs&+L into many images!&N",
     FALSE, victim, 0, 0, TO_ROOM);
  send_to_char
    ("&+LYou &+Cs&+Bs&+bs&+Cp&+Bp&+b&+Cl&+Bl&+bl&+Ci&+Bi&+bi&+Ct&+Bt&+bt&+Cs&+Bs&+bs&+L into many images!&N\r\n",
     victim);

  for(i = 0; i < numb; i++)
  {
    image = read_mobile(real_mobile(250), REAL);

    if(!image)
    {
      send_to_char("Your mirror ain't imaging tonight bubba.  Let a god know.\r\n", victim);
      return;
    }
    if( !image->only.npc )
    {
      logit(LOG_DEBUG, "Error reading image for %s.", GET_NAME(ch));
      send_to_char("Error reading image mob, let a god know.\r\n", ch);
      extract_char(image);
      return;
    }

    if( spot == i )
    {
      room = victim->in_room;
      char_from_room(victim);
      char_to_room(victim, room, -2);
    }

    char_to_room(image, victim->in_room, 0);

    if( !IS_ALIVE(image) )
    {
      continue;
    }

    image->only.npc->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);

    image->points.base_hit = GET_MAX_HIT(image) = GET_HIT(image) =
      level * 4 + number(0, 50);

    while( image->affected )
    {
      affect_remove(image, image->affected);
    }

    if( !IS_SET(image->specials.act, ACT_MEMORY) )
    {
      clearMemory(image);
    }

    SET_BIT(image->specials.affected_by, AFF_DETECT_INVISIBLE);

    balance_affects(image);

    setup_pet(image, ch, 3, PET_NOCASH);

    add_follower(image, ch);

    /* string it */

    logit(LOG_DEBUG, "REFLECTION: (%s) casting on (%s).", GET_NAME(ch), GET_NAME(victim));

    snprintf(Gbuf1, MAX_STRING_LENGTH, "image %s %s", GET_NAME(victim),
            race_names_table[GET_RACE(victim)].normal);

    image->player.name = str_dup(Gbuf1);
    image->player.short_descr = str_dup(victim->player.name);

    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s stands here.\r\n", victim->player.name);

    image->player.long_descr = str_dup(Gbuf1);

    if( GET_TITLE(victim) )
    {
      image->player.title = str_dup(GET_TITLE(victim));
    }

    GET_RACE(image) = GET_RACE(victim);
    GET_RACEWAR(image) = GET_RACEWAR(victim);
    GET_SEX(image) = GET_SEX(victim);
    GET_ALIGNMENT(image) = GET_ALIGNMENT(victim);
    GET_SIZE(image) = GET_SIZE(victim);
    // Make them ugly so they work for blocking.
    GET_C_CHA(image) = 1;

    remove_plushit_bits(image);

    if( targ == i && targ != spot )
    {
      for( tch = world[victim->in_room].people; tch; tch = tch->next_in_room )
      {
        if( IS_FIGHTING(tch) )
        {
          if( GET_OPPONENT(tch) == victim )
          {
            stop_fighting(tch);
            stop_fighting(victim);
            set_fighting(tch, image);

            if( !IS_FIGHTING(image) )
            {
              set_fighting(image, tch);
            }
          }
        }
      }
    }
  }
}


void spell_mask(int level, P_char ch, char *arg, int type, P_char victim,
                P_obj tar_obj)
{
  bool     loss_flag = FALSE;
  bool     casting_on_self = FALSE;
  int      chance, l, found, clevel, ss_save, ss_roll, the_size;
  long     resu_exp;
  P_obj    obj_in_corpse, next_obj, t_obj, money;
  struct affected_type *af, *next_af;
  P_char   t_ch, target = NULL;
  char     tbuf[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
  return;

  target = (struct char_data *) mm_get(dead_mob_pool);
  ensure_pconly_pool();
  target->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

  if (isname(arg, "me") || isname(arg, "self"))
    casting_on_self = TRUE;

  if ((restoreCharOnly(target, arg) < 0) || !target)
  {
    if (target)
    {
      free_char(target);
      target = NULL;
    }
  }



  if ((target && IS_PC(target) && GET_PID(target) == GET_PID(ch)) || casting_on_self)
  {
    if (!is_illusion_char(ch))
    {
      send_to_char("&+LSeems like you're already yourself...&N\r\n",
                   ch);
      if (target)
        free_char(target);
      return;
    }
    else
    {
      send_to_char("&+WYou f&Nad&+Le back into your own image.&N\r\n", ch);
      act("&+W$n &+Wf&Nad&+Les back into $s &+Lown image.&N", FALSE,
          ch, 0, ch, TO_ROOM);
      CharWait(ch, PULSE_VIOLENCE * 3);
      remove_disguise(ch, FALSE);
      if (target)
        free_char(target);
      return;
    }
  }

  if (!target)
  {
    send_to_char("&+LEven you are not that creative!&N\r\n", ch);
    return;
  }

  if (!IS_TRUSTED(ch) && target && GET_LEVEL(target) > 56)
  {
    send_to_char
      ("&+LYou don't know what the divine look like to make their illusions.&N\r\n",
       ch);
    if (target)
      free_char(target);
    return;
  }
  if (IS_TRUSTED(ch))
  {
    if (GET_LEVEL(ch) < GET_LEVEL(target))
    {
      send_to_char("No you don't!\r\n", ch);
      if (target)
        free_char(target);
      return;
    }
  }

  if (target)
  {
    if ((GET_ALT_SIZE(ch) < (GET_ALT_SIZE(target) - 1)) && !IS_TRUSTED(ch))
    {
      send_to_char("You're too small for that!\r\n", ch);
      CharWait(ch, PULSE_VIOLENCE);
    }
    else if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(target) + 1)) && !IS_TRUSTED(ch))
    {
      send_to_char("You're too big for that!\r\n", ch);
      CharWait(ch, PULSE_VIOLENCE);
    }
    else
    {
      if (target)
      {
        if (is_illusion_char(ch))
          remove_disguise(ch, FALSE);
        IS_DISGUISE_PC(ch) = TRUE;
        IS_DISGUISE_ILLUSION(ch) = TRUE;
        IS_DISGUISE_SHAPE(ch) = FALSE;
        ch->disguise.name = str_dup(GET_NAME(target));
        ch->disguise.m_class = target->player.m_class;
        ch->disguise.race = GET_RACE(target);
        ch->disguise.level = GET_LEVEL(target);
        ch->disguise.hit = GET_LEVEL(ch) * 2;
        ch->disguise.racewar = GET_RACEWAR(target);
        if (GET_TITLE(target))
          ch->disguise.title = str_dup(GET_TITLE(target));
        snprintf(tbuf, MAX_STRING_LENGTH,
                "&+LYour image shifts and &+bb&+Blur&+bs&+L into %s!&N\r\n",
                target->player.name);
        send_to_char(tbuf, ch);
        snprintf(tbuf, MAX_STRING_LENGTH,
                "&+LThe image of %s &+Lshifts and &+bb&+Blur&+bs&+L into %s&+L!&N\r\n",
                GET_NAME(ch), GET_NAME(target));
        act(tbuf, FALSE, ch, 0, NULL, TO_ROOM);
        SET_BIT(ch->specials.act, PLR_NOWHO);

      }
    }

  }

  if (target)
    free_char(target);

  return;
}

void spell_watching_wall(int level, P_char ch, char *arg, int type,
                         P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if (CHAR_IN_TOWN(ch) && IS_DISGUISE_ILLUSION(ch))
  {
    send_to_char
      ("You cannot create any walls when disguised in any town!\r\n", ch);
    return;
  }

  if (create_walls(ch->in_room, var, ch, level, WATCHING_WALL, level, 1800,
                   "&+La greyish stone wall covered with hundreds of eyes&n",
                   "&+yA massive stone wall embedded with eyes is here to the %s.&n",
                   0))
  {
    SET_BIT(EXIT(ch, var)->exit_info, EX_BREAKABLE);
    SET_BIT(VIRTUAL_EXIT((world[ch->in_room].dir_option[var])->to_room, rev_dir[var])->exit_info, EX_BREAKABLE);
    snprintf(buf1, MAX_STRING_LENGTH,
            "&+yA massive eye-covered stone wall appears to the %s!&n\r\n",
            dirs[var]);
    snprintf(buf2, MAX_STRING_LENGTH,
            "&+yA massive eye-covered stone wall appears to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void spell_illusionary_wall(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if (CHAR_IN_TOWN(ch) && IS_DISGUISE_ILLUSION(ch))
  {
    send_to_char
      ("You cannot create any walls when disguised in any town!\r\n", ch);
    return;
  }

  if (create_walls(ch->in_room, var, ch, level, ILLUSIONARY_WALL, level, 1800,
                   "&+wan illusionary wall&n",
                   "&+LAn &Nil&+Wlusiona&Nry&+L wall is here to the %s.&n",
                   ITEM_SECRET))
  {
    snprintf(buf1, MAX_STRING_LENGTH, "&+LThe exit to the %s disappears!&n\r\n", dirs[var]);
    snprintf(buf2, MAX_STRING_LENGTH, "&+LThe exit to the %s disappears!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
    SET_BIT(EXIT(ch, var)->exit_info, EX_ILLUSION);
    SET_BIT(VIRTUAL_EXIT
            ((world[ch->in_room].dir_option[var])->to_room,
             rev_dir[var])->exit_info, EX_ILLUSION);
  }
}


void spell_nightmare(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  int      dam;
  int      temp;

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if (affected_by_spell(victim, SKILL_BERSERK) || resists_spell(ch, victim))
    return;

  if (IS_DEMON(victim) || IS_DRAGON(victim) || IS_TRUSTED(victim))
  {

    act("How can you give $N nightmares? Nothing scares $M!", FALSE, ch, 0,
        victim, TO_CHAR);

    return;

  }

  if (GET_STAT(ch) == STAT_DEAD)
    return;

  if (!saves_spell(victim, SAVING_SPELL))
  {

    switch (GET_RACE(victim))
    {
    case RACE_TROLL:
      act("You show $N a nightmare of $M taking a bath!", FALSE, ch, 0,
          victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare of you taking a bath!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_HUMAN:
      act("You show $N a nightmare of an ogre sitting on $S face!", FALSE, ch,
          0, victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare of an ogre sitting on your face!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_MOUNTAIN:
    case RACE_DUERGAR:
      act("You show $N a nightmare of $M getting $S beard cut off!", FALSE,
          ch, 0, victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare with you getting your beard cut off!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_HALFLING:
      act
        ("You show $N a nightmare of $M getting $S hand caught in a cookie jar!",
         FALSE, ch, 0, victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare with you getting your hand caught in a cookie jar!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_OGRE:
      act("You show $N a nightmare of a little white mouse.", FALSE, ch, 0,
          victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare of a little white mouse jumping in front of you!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_THRIKREEN:
      act("You show $N a nightmare of $M getting stuffed into a freezer.",
          FALSE, ch, 0, victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare of you getting stuffed into a freezer!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_CENTAUR:
      act
        ("You show $N a nightmare of a old lady storm giant trying to ride $M.",
         FALSE, ch, 0, victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare of a old lady storm giant trying to mount you!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_MINOTAUR:
      act("You show $N a nightmare of $M getting killed by a bull fighter!",
          FALSE, ch, 0, victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare of you getting killed by a bull fighter!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_GOBLIN:
    case RACE_GNOME:
      act("You show $N a nightmare of $M getting stepped upon by a giant foot!",
          FALSE, ch, 0, victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare with you getting stepped on!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    case RACE_LICH:
    case RACE_PVAMPIRE:
      act
        ("You show $N a nightmare of $M getting stabbed by a wooden stake through the heart!",
         FALSE, ch, 0, victim, TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a nightmare of you getting stabbed through the heart by a wooden stake!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    default:
      act("You show $N a horror filled nightmare!", FALSE, ch, 0, victim,
          TO_CHAR);
      act
        ("A wave of painful terror overcomes you as $n shows you a horrible nightmare!",
         FALSE, ch, 0, victim, TO_VICT);
      act("$n shows $N! his worst nightmare!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      break;
    }

/*
    temp = MIN(35, level);
    dam = dice(2 * temp, 3);
    dam = dam * DAMFACTOR;

    damage(ch, victim, dam, SPELL_NIGHTMARE);

    if (char_in_list(victim) && !courage_check(victim))
      do_flee(victim, 0, 2);
*/
    if (char_in_list(victim) && !fear_check(victim))
      do_flee(victim, 0, 2);


  }
  else
  {
    act
      ("$N doesn't look frightened of $n's nightmares.. $E DOES look pissed off though.",
       FALSE, ch, 0, victim, TO_CHAR);
    act("$N looks pissed at $n for some reason.", TRUE, ch, 0, victim,
        TO_NOTVICT);
    act("$n's nightmares do not impress you.", TRUE, ch, 0, victim, TO_VICT);
  }
  if (ch->in_room == victim->in_room)
  {

    if (IS_NPC(victim) && CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if (!IS_FIGHTING(victim) && !IS_DESTROYING(victim))
        MobStartFight(victim, ch);
    }
  }
}




void spell_shadow_shield(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  int      absorb = (level / 6) + number(1, 4);

  if (!has_skin_spell(victim))
  {
    act("&+LShadows start to swirl around and cover&n $n's &+Lskin.&n",
        TRUE, victim, 0, 0, TO_ROOM);
    act("&+LYou feel a strange shadowy mist cover your body.&n",
        TRUE, victim, 0, 0, TO_CHAR);
  }
  else
  {
    send_to_char("Their body is already covered with a magic shell!\r\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_SHADOW_SHIELD;
  af.duration = 4;
  af.modifier = absorb;
  affect_to_char(victim, &af);

}

void spell_vanish(int level, P_char ch, char *arg, int type, P_char victim,
                  P_obj obj)
{
  struct affected_type af;
  int room;

  if (!((victim || obj) && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  if (!affected_by_spell(victim, SPELL_INVIS_MAJOR))
  {

    act("&+L$n instantly vanishes out of existence.", TRUE, victim, 0, 0,
        TO_ROOM);
    send_to_char("&+LYou completely vanish.\r\n", victim);

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
      if (af1->type == SPELL_INVIS_MAJOR)
      {
        af1->duration = level / 2;
      }
  } 
  
  if(IS_WATER_ROOM(ch->in_room) ||
     world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("It's far too wet to vanish and hide behind anything...\r\n", ch);
  }
  else
    SET_BIT(ch->specials.affected_by, AFF_HIDE);

}


void spell_hammer(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "Your massive &+Bhammer&N causes $N to double over in pain!",
    "$n creates a &+Bhammer&N out of thin air that smashes into you!",
    "$n creates a &+Bhammer&N out of thin air that smashes into $N, bits of &+yflesh&n and &+Wbone&n fly everywhere!",
    "Your &+Billusional hammer&N smashes into $N's body, killing $M instantly!",
    "$n's massive &+Bhammer&N is the last thing you ever see.",
    "$n's &+Bhammer&N strikes $N smashing $M into the ground!"
  };
  int dam, temp, result;
  bool stunself = false;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( resists_spell(ch, victim) )
  {
    return;
  }

  if( affected_by_spell(victim, SPELL_DEFLECT) || IS_AFFECTED4(victim, AFF4_DEFLECT) )
  {
    stunself = 1;
  }

  // Imms casting hammer lvl reduced to 56 for testing.
  if( level > 56 && IS_PC(ch) )
  {
    level = 56;
  }
  dam = 9 * level + number(-25, 25);

  // Adding a save for this since 115 damage for 7th circle is absurd.
  //  This brings 115 down to 76, which is normal non-sorc 7th circle damage.
  if( NewSaves(victim, SAVING_SPELL, level/7) )
  {
    dam = (dam * 2) / 3;
  }

  if( spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD )
  {
    return;
  }

 if(GET_SPEC(ch, CLASS_ILLUSIONIST, SPEC_DECEIVER))
 {
  if(!number(0, 19))
  {
    if(stunself)
      Stun(ch, ch, PULSE_VIOLENCE, FALSE);
    else
      Stun(victim, ch, PULSE_VIOLENCE, TRUE);
  }
 }
}

void spell_detect_illusion(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct affected_type af;

  if (!((level >= 0) && victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if (IS_AFFECTED4(victim, AFF4_DETECT_ILLUSION))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_DETECT_ILLUSION)
      {
        af1->duration = MAX(level, 10);
      }
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_DETECT_ILLUSION;
  af.duration = MAX(level, 10);
  af.bitvector = AFF_DETECT_INVISIBLE;
  af.bitvector4 = AFF4_DETECT_ILLUSION;
  affect_to_char(victim, &af);

  send_to_char("&+WYour vision feels enhanced!\r\n", victim);

}

void spell_dream_travel(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      location;
  P_char   targ, mount;
  P_desc   i;
  struct group_list *group;

  if( !IS_ALIVE(ch) || ch->in_room < 0 )
  {
    return;
  }

  if( GET_SPEC(ch, CLASS_ILLUSIONIST, SPEC_DARK_DREAMER) )
    CharWait( ch, number(4*WAIT_SEC, 6*WAIT_SEC) );
  else if( IS_PC(ch) )
    CharWait(ch, 12*WAIT_SEC);
  else
    CharWait(ch, 5*WAIT_SEC);

  if( !can_relocate_to(ch, victim) )
  {
    return;
  }

  if(victim == ch)
  {
    location = ch->in_room;

    for( group = ch->group; group; group = group->next )
    {
      targ = group->ch;

      if( IS_ALIVE(targ) && CAN_SEE(ch, targ) )
      {

        if( IS_NPC(targ) || is_linked_to(ch, targ, LNK_CONSENT) )
        {
          if( (IS_ROOM(targ->in_room, ROOM_NO_TELEPORT) || IS_HOMETOWN(targ->in_room) ) && !IS_TRUSTED(ch) )
          {
            send_to_char("You feel slightly sleepy, but shake it off.\r\n", ch);
            continue;
          }

          if( IS_FIGHTING(targ) )
          {
            send_to_char("You feel slightly drowsy, but it's too noisy to sleep!\r\n", targ);
            continue;
          }

          if( (mount = GET_MOUNT(targ)) != NULL )
          {
            act( "You feel really tired, and fall off $N.", FALSE, targ, 0, mount, TO_CHAR);
            stop_riding(targ);
          }

          act("&+LYou &+Wblink&+L and $n &+Lis gone! &+CIt was just a &+Wdream&+C; $e &+Cwas never here to begin with.&N",
             FALSE, targ, 0, 0, TO_ROOM);

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(targ) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", targ);
      drop_ctf_flag(targ);
    }
#endif

          send_to_char("&+WTh&Ne &+Lworld becomes surreal, everything becomes a &Nblur&+L and you cannot tell\r\n", targ);
          send_to_char("&+Lthe difference between reality and dreams.  You are &+Wflying&N, &+rc&+Ro&+Yl&+Wo&+Cr&+Bs&+b f&+Bl&+Co&+Ww p&Na&+Lst\r\n", targ);
          send_to_char("&+Lyou like paint in a river.  Then, as if something &+wchanged&+L, you feel a sense of urgency\r\n", targ);
          send_to_char("&+Las you begin &+Wfalling &+Lrapidly towards the ground below.  You close your eyes as the &+Gground\r\n", targ);
          send_to_char("&+Lrushes towards you...&n\r\n", targ);

          if( GET_STAT(targ) == STAT_SLEEPING )
             SET_POS(targ, GET_POS(targ) + STAT_NORMAL);
          send_to_char("&+WYou open your eyes and see:&N\r\n", targ);

          targ->specials.z_cord = ch->specials.z_cord;
          char_from_room(targ);
          char_to_room(targ, location, -1);
          if( GET_STAT(targ) > STAT_SLEEPING )
          {
             SET_POS(targ, GET_POS(targ) + STAT_SLEEPING);
          }
          send_to_char("&+WAre you really here?  It must be a dream, you must be sleeping.&N\r\n", targ);
          act("$n falls asleep.", FALSE, targ, 0, 0, TO_ROOM);

          if( GET_SPEC(ch, CLASS_ILLUSIONIST, SPEC_DARK_DREAMER) )
            CharWait(targ, 6*WAIT_SEC);
          else
            CharWait(targ, 12*WAIT_SEC);
        }
      }
    }
  }
  else
  {
    location = victim->in_room;

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

    act("&+LYou &+Wblink&+L and $n &+Lis gone! &+CIt was just a &+Wdream&+C, $e &+Cwas never here to begin with.&N",
       FALSE, ch, 0, 0, TO_ROOM);
    act("&+LYou drift into the world of dreams...&N",
       FALSE, ch, 0, 0, TO_CHAR);

    char_from_room(ch);
    if( GET_STAT(ch) > STAT_SLEEPING && !IS_TRUSTED(ch) )
    {
      SET_POS(ch, GET_POS(ch) + STAT_SLEEPING);
    }
    char_to_room(ch, location, -1);

    act("$n falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
  }
}


/*
Help function to detect illusions one for
mobile/player illusions and one for object..
- Kvark
*/

int is_illusion_char(P_char ch)
{
  if (!ch)
    return 0;

//Quick solution until we get a bit that i can look for.....
  if (IS_DISGUISE(ch) && IS_DISGUISE_ILLUSION(ch))
    return 1;

// Check for images....add a struct if ya plan to have more mobs you want to detect..
  if (IS_NPC(ch))
    if (GET_VNUM(ch) == 250)
      return 1;

// OK nothing found soo let's leave it as a false!
  return 0;
}

int is_illusion_obj(P_obj obj)
{
  if(!obj)
    return 0;

//Check for illusionary wall.
  if(obj_index[obj->R_num].virtual_number == 759)
    return 1;

  return 0;

//Add what ever might wanna go in here!

}

void spell_clone_form(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool     loss_flag = FALSE;
  bool     casting_on_self = FALSE;
  int      chance, l, found, clevel, ss_save, ss_roll, the_size;
  long     resu_exp;
  P_obj    obj_in_corpse, next_obj, t_obj, money;
  struct affected_type *af, *next_af;
  P_char   t_ch, tch, next_ch, target = NULL;
  char     tbuf[MAX_STRING_LENGTH];

  if( (level < 0) || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if( IS_NPC(ch) )
    return;

  if( !IS_ALIVE(ch) )
    return;

  if( !IS_TRUSTED(ch) && GET_LEVEL(victim) > 56 )
  {
    send_to_char("&+LTry to find something that is a lower level to clone.&N\r\n", ch);
    return;
  }

  if (isname(arg, "me") || isname(arg, "self"))
  {
    casting_on_self = TRUE;
  }

  if( IS_NPC(victim) && victim != ch && GET_VNUM(victim) == 250 )
  {
    send_to_char("&+WYour magic is unable to duplicate the appearance of an illusion.&N\r\n", ch);
    return;
  }

  if( casting_on_self && is_illusion_char(ch) )
  {
    send_to_char("&+WYou f&Nad&+Le back into your own image.&N\r\n", ch);
    act("&+W$n &+Wf&Nad&+Les back into $s &+Lown image.&N", FALSE,
        ch, 0, ch, TO_ROOM);
    CharWait(ch, PULSE_VIOLENCE * 3);
    remove_disguise(ch, FALSE);
    return;
  }

  target = victim;

  if( target == ch )
  {
    if (!IS_DISGUISE(ch))
    {
      send_to_char("&+LMaking you looking like yourself is beyond you.&N\r\n",
                   ch);
      return;
    }

    else
    {
      for (tch = world[ch->in_room].people; tch; tch = next_ch)
      {
        next_ch = tch->next_in_room;
        if ((tch != ch) && isname(arg, GET_NAME(tch)))
        {
          target = tch;
          break;
        }
      }
      if (target == ch)
      {
        send_to_char("&+LYou just tried to clone into yourself.&N\n\r", ch);
        send_to_char("&+LTo avoid problems, use arguments \"me\" or \"self\" if you want to return to your normal form.&N\r\n", ch);
        return;
      }
    }

  }


  if( IS_NPC(target) )
  {
    if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(target) + 1)) && !IS_TRUSTED(ch))
    {
      send_to_char("You're too big to even imagine trying to look like that!\r\n", ch);
    }
    else
    {
      if (is_illusion_char(ch))
        remove_disguise(ch, FALSE);
      t_ch = target;
      // Handle NPC Corpses
      IS_DISGUISE_PC(ch) = FALSE;
      IS_DISGUISE_NPC(ch) = TRUE;
      IS_DISGUISE_ILLUSION(ch) = TRUE;
      IS_DISGUISE_SHAPE(ch) = FALSE;
      ch->disguise.title = str_dup(GET_NAME(t_ch));
      ch->disguise.name = str_dup(t_ch->player.short_descr);
      ch->disguise.longname = str_dup(t_ch->player.long_descr);
      ch->disguise.m_class = t_ch->player.m_class;
      ch->disguise.racewar = GET_RACEWAR(target);
      ch->disguise.race = GET_RACE(t_ch);
      ch->disguise.hit = GET_LEVEL(ch) * 2;
      snprintf(tbuf, MAX_STRING_LENGTH, "&+LYou &+Bblur&N and take on the form of %s!\r\n", t_ch->player.short_descr);
      send_to_char(tbuf, ch);
      snprintf(tbuf, MAX_STRING_LENGTH, " &+LThe image of %s &Ndisappears&+L, and is replaced by %s!\r\n",
        GET_NAME(ch), t_ch->player.short_descr);
      act(tbuf, FALSE, ch, 0, NULL, TO_ROOM);
      SET_BIT(ch->specials.act, PLR_NOWHO);
    }
  }
  else
  {

    if( !IS_TRUSTED(ch) && target && GET_LEVEL(target) > MAXLVLMORTAL )
    {
      send_to_char("&+LYou don't know what the divine look like to make their illusions.&N\r\n", ch);
      return;
    }
    if (IS_TRUSTED(ch))
    {
      if (GET_LEVEL(ch) < GET_LEVEL(target))
      {
        send_to_char("No you don't!\r\n", ch);
        return;
      }
    }
    if (target)
    {
      if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(target) + 1)) && !IS_TRUSTED(ch))
      {
        send_to_char("You're too big to even think of making yourself look like that!\r\n", ch);
        return;
      }
      else
      {
        if (target)
        {
          if (is_illusion_char(ch))
            remove_disguise(ch, FALSE);
          IS_DISGUISE_NPC(ch) = FALSE;
          IS_DISGUISE_PC(ch) = TRUE;
          IS_DISGUISE_ILLUSION(ch) = TRUE;
          IS_DISGUISE_SHAPE(ch) = FALSE;
          ch->disguise.name = str_dup(GET_NAME(target));
          ch->disguise.m_class = target->player.m_class;
          ch->disguise.race = GET_RACE(target);
          ch->disguise.level = GET_LEVEL(target);
          ch->disguise.hit = GET_LEVEL(ch) * 2;
          ch->disguise.racewar = GET_RACEWAR(target);
          if (GET_TITLE(target))
            ch->disguise.title = str_dup(GET_TITLE(target));
          snprintf(tbuf, MAX_STRING_LENGTH,
                  "&+LYour image shifts and &+bb&+Blur&+bs&+L into %s!&N\r\n",
                  target->player.name);
          send_to_char(tbuf, ch);
          snprintf(tbuf, MAX_STRING_LENGTH, "&+LThe image of %s &+Lshifts and &+bb&+Blur&+bs&+L into %s&+L!&N\r\n",
                  GET_NAME(ch), GET_NAME(target));
          act(tbuf, FALSE, ch, 0, NULL, TO_ROOM);
          SET_BIT(ch->specials.act, PLR_NOWHO);
          return;
        }
      }
    }
  }
}

void spell_imprison(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  char     buf[1024];
  P_obj    shell;
  int      shell_hps;

  if (IS_NPC(victim))
  {
    return;
  }

  if (IS_FIGHTING(victim) || IS_DESTROYING(victim))
  {
    send_to_char("With this much commotion you can't seem to get a fix on them!\r\n", ch);
    return;
  }
  
  if(victim == ch)
  {
    send_to_char
      ("&+CYou instantly disbelieve the illusion for what it really is...\r\n",
       ch);
    return;
  }
  
  for (shell = world[victim->in_room].contents; shell; shell = shell->next)
  {
    if (obj_index[shell->R_num].virtual_number == 1300)
    {
      if (shell->value[0] == GET_PID(victim))
      {
        break;
      }
    }
  }

  if (shell != NULL)
  {
    send_to_char
      ("This poor guy is already deeply convinced that they are imprisoned!\r\n",
       ch);
    return;
  }

  shell = read_object(1300, VIRTUAL);

  if (!shell)
  {
    send_to_char("Bug with imprisonment, report this immediately!\r\n", ch);
    return;
  }

  if (number(0, 1))
  {
    if (NewSaves(victim, SAVING_FEAR, 10))
    {
      act
        ("$N is almost encased in a $q, but $E believes it is just an illusion.",
         TRUE, victim, shell, victim, TO_ROOM);
      act("$n tries to encase you in a $q, but you disbelieve it!", FALSE, ch,
          shell, victim, TO_VICT);
      return;
    }
  }

  shell_hps = 400 + MAX(0, MIN(level, 53) - 46) * 40;

  if (NewSaves(victim, SAVING_FEAR, 3))
  {
    shell_hps = (int) (shell_hps * 0.65);
    act("$N blinks in &+cdi&+Csb&+cel&+Cie&+cf &nas a $q &nencases $M!", TRUE,
        victim, shell, victim, TO_ROOM);
    act
      ("You blink in &+cdi&+Csb&+cel&+Cie&+cf &nas a $q &ncreated by $n wraps around you!",
       FALSE, ch, shell, victim, TO_VICT);
  }
  else
  {
    act("$N freezes in &+ct&+Ce&+cr&+Cr&+co&+Cr &nas a $q &nencases $M!",
        TRUE, victim, shell, victim, TO_ROOM);
    act
      ("You freeze in &+ct&+Ce&+cr&+Cr&+co&+Cr &nas a $q &ncreated by $n wraps around you!",
       FALSE, ch, shell, victim, TO_VICT);
  }

  stop_fighting(victim);
  if( IS_DESTROYING(victim) )
    stop_destroying(victim);
  StopAllAttackers(victim);
  SET_BIT(victim->specials.affected_by5, AFF5_IMPRISON);

  SET_BIT(shell->str_mask, STRUNG_DESC1);

  snprintf(buf, 1024, shell->description,
          victim->player.short_descr ? victim->player.short_descr : "someone");

  shell->description = str_dup(buf);
  shell->value[0] = GET_PID(victim);
  shell->value[1] = shell_hps;
  obj_to_room(shell, ch->in_room);

  return;
}

/* called from raw_damage */
bool handle_imprison_damage(P_char ch, P_char victim, int dam)
{
  P_obj    t_obj;

  for (t_obj = world[victim->in_room].contents; t_obj;
       t_obj = t_obj->next_content)
  {
    if (obj_index[t_obj->R_num].virtual_number == 1300)
    {
      if (t_obj->value[0] == GET_PID(victim))
      {
        break;
      }
    }
  }

  if (t_obj == NULL)
  {
    REMOVE_BIT(victim->specials.affected_by5, AFF5_IMPRISON);
    return FALSE;
  }

  if (GET_OPPONENT(ch) == victim)
  {
    stop_fighting(victim);
    stop_fighting(ch);
  }

  if (t_obj->value[1] < dam)
  {
    act("A $q encasing $N suddenly blurs and fades into nothing under $n's assault!",
      TRUE, ch, t_obj, victim, TO_NOTVICT);
    act("A $q encasing $N suddenly blurs and fades into nothing under your assault!",
      TRUE, ch, t_obj, victim, TO_CHAR);
    act("A $q encasing you suddenly blurs and fades into nothing under $n's assault!",
      FALSE, ch, t_obj, victim, TO_VICT);
    REMOVE_BIT(victim->specials.affected_by5, AFF5_IMPRISON);
    extract_obj(t_obj, TRUE); // Not gonna be an arti.
    return TRUE;
  }

  t_obj->value[1] -= dam;
  act("A $q encasing $N flares brightly as it absorbs $n's assault!",
    TRUE, ch, t_obj, victim, TO_NOTVICT);
  act("A $q encasing $N flares brightly as it absorbs your assault!",
    TRUE, ch, t_obj, victim, TO_CHAR);
  act("A $q encasing you flares brightly as it absorbs $n's assault!",
    FALSE, ch, t_obj, victim, TO_VICT);

  return TRUE;
}


void spell_nonexistence(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int room;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  act("&+LYou shimmer in and out of view as the &+millusion&+L begins to cover you.", 0, ch, 0, victim, TO_CHAR);

  LOOP_THRU_PEOPLE(victim, ch)
  {
    act("&+L$N shimmers in and out of view as $n's &+millusion&+L starts to cover $M.", 0, ch, 0, victim, TO_NOTVICT);
    act("&+LYou shimmer in and out of existence as $n's &+millusion&+L starts to cover you.", 0, ch, 0, victim, TO_VICT);
    spell_improved_invisibility(level, ch, 0, 0, victim, 0);
  }

  if( IS_WATER_ROOM(ch->in_room) || world[ch->in_room].sector_type == SECT_OCEAN )
  {
    send_to_char("It's far too wet here for hide to take hold...\r\n", ch);
  }
  else
  {
    LOOP_THRU_PEOPLE(victim, ch)
    {
      SET_BIT(victim->specials.affected_by, AFF_HIDE);
    }
  }
  /* Guess this was too much?
  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    if( IS_SET(obj->wear_flags, ITEM_TAKE) )
      spell_improved_invisibility(level, ch, 0, 0, 0, obj);
  }
  */
}

void spell_dragon(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  int      summoned;
  struct char_link_data *cld;

  if (!ch || !victim)
    return;

  if( CHAR_IN_SAFE_ROOM(ch) )
  {
    send_to_char("A mysterious force blocks your casting!\r\n", ch);
    return;
  }

  for( summoned = 0, cld = ch->linked; cld; cld = cld->next_linked )
  {
    if( cld->type == LNK_PET && IS_NPC(cld->linking) && GET_VNUM(cld->linking) == VMOB_ILLUS_DRAGON )
    {
      summoned++;
    }
  }

  if( summoned >= MAX(1, GET_LEVEL(ch) / 14) )
  {
    send_to_char("You cannot summon any more dragons right now!\r\n", ch);
    return;
  }

  mob = read_mobile(real_mobile(VMOB_ILLUS_DRAGON), REAL);
  if( !mob )
  {
    logit(LOG_DEBUG, "spell_dragon(): mob %d not loadable", VMOB_ILLUS_DRAGON);
    send_to_char("Your dragon has a bug.  Tell a &+WGod&n before it spreads!\r\n", ch);
    return;
  }

  char_to_room(mob, ch->in_room, 0);
  act("$n &+wappears from nowhere!", TRUE, mob, 0, 0, TO_ROOM);

  mob->player.level = BOUNDED(1, number(level - 5, level + 1), 55);

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
  mob->player.m_class = CLASS_WARRIOR;
  mob->specials.act |= ACT_SPEC_DIE;
  remove_plushit_bits(mob);

  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = dice(GET_LEVEL(mob), 5) + (GET_LEVEL(mob) * 4);

  balance_affects(mob);

// play_sound(SOUND_ELEMENTAL, NULL, ch->in_room, TO_ROOM);

  setup_pet(mob, ch, 1, PET_NOORDER | PET_NOCASH);
  if( ch->group )
  {
    group_add_member(ch->group->ch, mob);
  }
  else
  {
    group_add_member(ch, mob);
  }
  MobStartFight(mob, victim);
}

void spell_titan(int level, P_char ch, char *arg, int type, P_char victim,
                 P_obj obj)
{
  struct follow_type *k;
  P_char   mob;
  char     Gbuf1[512];
  int      summoned = 0;
  struct char_link_data *cld;

  if (!ch || !victim)
    return;

  if (IS_NPC(ch) && (GET_VNUM(ch) == TITAN_NUMBER))
  {
    send_to_char("&+LNope. Illusions can't own other illusions.\r\n", ch);
    return;
  }

  if (CHAR_IN_SAFE_ROOM(ch))
  {
    send_to_char("A mysterious force blocks your conjuring!\r\n", ch);
    return;
  }

  for (cld = ch->linked; cld; cld = cld->next_linked)
  {
    if( cld->type == LNK_PET && IS_NPC(cld->linking) && GET_VNUM(cld->linking) == TITAN_NUMBER )
    {
      summoned++;
    }
  }

  if( summoned )
  {
    send_to_char("&+BThere can be only &+Yone&+B!&n\r\n", ch);
    return;
  }

  for( k = ch->followers; k; k = k->next )
  {
    mob = k->follower;
    if( IS_NPC(mob) && GET_VNUM(mob) == 250 )
    {
      send_to_char("&+LBut you cant support so many different illusions!&n\r\n", ch);
      return;
    }
  }

  mob = read_mobile(real_mobile(TITAN_NUMBER), REAL);
  if( !mob )
  {
    logit(LOG_DEBUG, "spell_titan(): mob %d not loadable", TITAN_NUMBER);
    send_to_char("Bug in titan.  Tell a god!\r\n", ch);
    return;
  }

  mob->player.level = BOUNDED(1, number(level - 5, level), 55);
  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);
  SET_BIT(mob->specials.affected_by, AFF_DETECT_INVISIBLE);
  mob->player.m_class = CLASS_ILLUSIONIST;

  remove_plushit_bits(mob);

  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = (int) (GET_MAX_HIT(ch) * 0.7);

  GET_SIZE(mob) = SIZE_GIANT;

  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 2;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 3;
  mob->points.damnodice = (int) GET_LEVEL(ch) / 6;
  mob->points.damsizedice = (int) GET_LEVEL(ch) / 8;
  mob->specials.act |= ACT_SPEC_DIE;

  mob->only.npc->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);
  snprintf(Gbuf1, 512, "titan huge illusion %s %s", GET_NAME(ch),
          race_names_table[GET_RACE(ch)].normal);
  mob->player.name = str_dup(Gbuf1);
  snprintf(Gbuf1, 512, "&+BA &+Yhuge&+B illusion of &+R%s&n", ch->player.name);
  mob->player.short_descr = str_dup(Gbuf1);
  snprintf(Gbuf1, 512, "&+BA &+Yhuge&+B illusion of &+R%s &+Bstands here.&n\r\n",
          ch->player.name);
  mob->player.long_descr = str_dup(Gbuf1);

  if (GET_TITLE(ch))
    mob->player.title = str_dup(GET_TITLE(ch));

  GET_RACE(mob) = RACE_GIANT;
  GET_SEX(mob) = GET_SEX(ch);
  GET_ALIGNMENT(mob) = GET_ALIGNMENT(ch);

  char_to_room(mob, ch->in_room, 0);
  act("$n &+Lappears from nowhere!", TRUE, mob, 0, 0, TO_ROOM);

// play_sound(SOUND_ELEMENTAL, NULL, ch->in_room, TO_ROOM);

  setup_pet(mob, ch, 1, PET_NOORDER | PET_NOCASH);
  group_add_member(ch, mob);
  MobStartFight(mob, victim);

}

void spell_delirium(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;
  int      percent = 0;
  int      save;

  save = victim->specials.apply_saving_throw[SAVING_SPELL];

  percent = BOUNDED( 0, (GET_C_POW(ch) - GET_C_INT(victim) + save + (GET_LEVEL(ch) - GET_LEVEL(victim))), 100);

  if (IS_NPC(ch))
  {
    act("It's confused enough!!&n", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }

  if (ch == victim)
  {
    act("Can't really, I mean who am I, and who is what and when?!&n", TRUE,
        ch, 0, victim, TO_CHAR);
    return;

  }

  act("$n sends a delirius ray streaking towards $N!", TRUE, ch, 0, victim,
      TO_NOTVICT);
  act("$n sends a delirius ray streaking YOU!", TRUE, ch, 0, victim, TO_VICT);
  act("You grin evilly as you send delirius ray streaking towards $N!", TRUE,
      ch, 0, victim, TO_CHAR);

  if (percent > 10)
  {
    //if (!saves_spell(victim, SAVING_SPELL) && (!number(0, 1))) {

    if (!affected_by_spell(victim, SPELL_DELIRIUM))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_DELIRIUM;
      	af.flags = AFFTYPE_SHORT;
	  af.duration = 40;
      af.bitvector5 = AFF5_DELIRIUM;
      affect_to_char(victim, &af);


      act("$N &+Wlooks a bit &+Gconfused!&n", TRUE, ch, 0, victim,
          TO_NOTVICT);
      act("&+WYou feel a bit &+Gconfused&n!", TRUE, ch, 0, victim, TO_VICT);
      act("$N &+Wlooks a bit &+Gconfused!&n", TRUE, ch, 0, victim, TO_CHAR);
    }
    else
    {
      act("It's confused enough!!&n", TRUE, ch, 0, victim, TO_CHAR);
    }
  }

}

void spell_flicker(int level, P_char ch, char *arg, int type, P_char victim,
                   P_obj obj)
{

  struct group_list *gl;

  if (ch && ch->group)
  {

    act("&+L$n creates a sh&+cim&+Cme&+cri&+Lng orb of light, casting s&+mha&+Mdo&+mw&+Ls over the room!&n", TRUE, ch, 0, victim,
      TO_NOTVICT);
    act("&+LYou create a sh&+cim&+Cme&+cri&+Lng orb of light, casting s&+mha&+Mdo&+mw&+Ls over the room!&n", TRUE,
      ch, 0, victim, TO_CHAR);

    act("&+LThe room is blanketed in da&+mnci&+Mng sh&+mado&+Lws.&n", 0, ch, 0, 0, TO_ROOM);
    act("&+LThe room is blanketed in da&+mnci&+Mng sh&+mado&+Lws.&n", 0, ch, 0, 0, TO_CHAR);
    SET_BIT(world[ch->in_room].room_flags, ROOM_TWILIGHT);

    gl = ch->group;
    /* leader first */
    if (gl->ch->in_room == ch->in_room)
      spell_shadow_shield((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if (gl->ch->in_room == ch->in_room)
        spell_shadow_shield((level / 3) * 2, ch, 0, 0, gl->ch, 0);
    }
  }
}


void spell_greater_flicker(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  send_to_char("This spell is not yet implemented.\r\n", ch);
}


void spell_obscuring_mist(int level, P_char ch, char *arg, int type,
                             P_char victim, P_obj obj)
{
  struct affected_type af;

  if (!IS_AFFECTED5(victim, AFF5_OBSCURING_MIST))
  {
    act("&+cA &npale grey &+cmist swirls around $n&+L!", TRUE,
        victim, 0, 0, TO_ROOM);
    act("&+cA &npale grey &+cmist swirls around you!", TRUE, victim, 0, 0,
        TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_OBSCURING_MIST;
    af.duration = (level / 4 + 1);
    af.modifier = level;
    af.bitvector5 = AFF5_OBSCURING_MIST;
    affect_to_char(victim, &af);

    affect_total(victim, FALSE);

  }
  else
    send_to_char
      ("You must are already surrounded by &+ctons of mist&n.\n",
       victim);
}


void spell_suppress_sound(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct group_list *gl;
  if (ch && ch->group)
  {
    gl = ch->group;
    if (gl->ch->in_room == ch->in_room)
      spell_sound_suppression(level, ch, NULL, SPELL_TYPE_SPELL, gl->ch, 0);
    for (gl = gl->next; gl; gl = gl->next) {
       if (gl->ch->in_room == ch->in_room)
       spell_sound_suppression(level, ch, NULL, SPELL_TYPE_SPELL, gl->ch, 0);
    }
  }
}


void spell_sound_suppression(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj obj)
{
  struct affected_type af;
  int counter;
  struct group_list *gl;

  if (!victim)
    return;

  for (gl = ch->group, counter = 0; gl; gl = gl->next)
  {
    counter++;
  }
    
  if (!affected_by_spell(victim, SPELL_SUPPRESSION))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_SUPPRESSION;
    af.duration = 2;
    af.flags = AFFTYPE_NODISPEL;
    // success = add sneak
    if (number(1, 100) > (counter*2))
    {
      af.bitvector = AFF_SNEAK;
      act("$n becomes deathly silent.&n", FALSE,
          victim, 0, 0, TO_ROOM);
      act("You are surrounded in silence.&n", FALSE,
          victim, 0, 0, TO_CHAR);
    }
    affect_to_char(victim, &af);
  }
}

void spell_shadow_merge(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  P_char t_ch;
  struct affected_type af;

  if (!affected_by_spell(ch, SPELL_SHADOW_MERGE)) {

    bzero(&af, sizeof(af));
    af.type = SPELL_SHADOW_MERGE;
    af.duration = 4;

    affect_to_char(ch, &af);

    if (ch->following) {

      for (t_ch = world[ch->in_room].people; t_ch; t_ch = t_ch->next_in_room) {

        if (t_ch == ch->following) {

        act("$n fades in to the &+Lshadow&n of $N.&n", FALSE,
          ch, 0, ch->following, TO_ROOM);
        act("You step back into the &+Lshadow&n of $N.&n", FALSE,
          ch, 0, ch->following, TO_CHAR);
        }
      }
    } else {

      act("$n fades in to the &+Lshadows&n.&n", FALSE,
        ch, 0, 0, TO_ROOM);
      act("You step back into the &+Lshadows&n.&n", FALSE,
        ch, 0, 0, TO_CHAR);

    }
    SET_BIT(ch->specials.affected_by, AFF_HIDE);
  }

}

void cast_ardgral(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  struct portal_settings set = {
      291, /* portal type  */
      -1,  /* from room */
      -1,  /* to room */
      0,   /* How many can pass before closes */
      0,   /* Timeout before anyone can enter after open */
      0,   /* Timeout before next person can enter */
      0,   /* Lag person gets when steps out portal */
      0    /* Portal decay timer */
  };
  struct portal_create_messages msg = {
    /*ch   */ "&+cSomething prevents your escapist dreams here&n.\r\n",
    /*ch r */ 0,
    /*vic  */ 0,
    /*vic r*/ 0,
    /*ch   */ "&+cThe winds of thought collect your &+Wdreams&+c and then quickly condense into $p&n&+c.",
    /*ch r */ "&+cThe winds of thought collect your &+Wdreams&+c and then quickly condense into $p&n&+c.",
    /*vic  */ 0,
    /*vic r*/ 0,
    /*npc  */ 0,
    /*bad  */ 0
  };

  int to_room;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  to_room = real_room0(number(30401, 30464));

  if( IS_ROOM(ch->in_room, ROOM_NO_GATE)
    || IS_HOMETOWN(ch->in_room) )
  {
    send_to_char(msg.fail_to_caster, ch);
    return;
  }

  if(ch && !is_Raidable(ch, 0, 0))
  {
    send_to_char("&+WYou are not raidable. The spell fails!\r\n", ch);
    return;
  }

  set.to_room = to_room;
  set.throughput         = get_property("portals.gate.maxToPass", -1);
  set.init_timeout       = get_property("portals.gate.initTimeout", 0);
  set.post_enter_timeout = get_property("portals.gate.postEnterTimeout", 0);
  set.post_enter_lag     = get_property("portals.gate.postEnterLag", 0);
  set.decay_timer        = get_property("portals.gate.decayTimeout", 240);

  // one way portal
  spell_general_portal(level, ch, 0, &set, &msg, TRUE);

  return;
}

struct sbb_data {
  int room;
  int waves;
};

void spell_shadow_burst(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct sbb_data sbbdata;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  sbbdata.waves = 2;
  sbbdata.room = ch->in_room;
  send_to_room("&+LThe room clouds with &nmists&+L as a v&+ror&+Rt&+re&+Lx starts to form..\n",
    ch->in_room);
  add_event(event_shadow_spawn, PULSE_VIOLENCE * 2, ch, 0, 0, 0, &sbbdata, sizeof(struct sbb_data));
  CharWait(ch, PULSE_VIOLENCE);
}

void event_shadow_spawn(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct sbb_data *sbbdata = (struct sbb_data*)data;
  int splat;

  if(!sbbdata)
  {
    debug("Passed null pointer to event_shadow_spawn_burst in sillusionist.c.");
    return;
  }

  if((sbbdata->room != ch->in_room) ||
    (sbbdata->waves < 1))
  {
    send_to_char("&+LThe swirling v&+ror&+Rt&+re&+Lx &+Lcollapses!\n", ch);;
    act("&+LThe swirling v&+ror&+Rt&+re&+Lx &+Lcollapses!",
      TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
 
  act("&+LThe v&+ror&+Rt&+re&+Lx splits open and horrific n&+mi&+Lghtm&+ma&+Lr&+me&+Ls explode into the room!",
    FALSE, ch, 0, 0, TO_CHAR);
  act("&+LThe v&+ror&+Rt&+re&+Lx splits open and horrific n&+mi&+Lghtm&+ma&+Lr&+me&+Ls explode into the room!",
    FALSE, ch, 0, 0, TO_ROOM);
  cast_as_damage_area(ch, spell_shadow_spawn, GET_LEVEL(ch), NULL,
    get_property("spell.area.minChance.spawn", 90),
    get_property("spell.area.chanceStep.spawn", 10));

  sbbdata->waves--;

  if(ch &&
    IS_NPC(ch) &&
    !GET_MASTER(ch))
  {
    splat = 1;
  }
  else
  {
    splat = 2;
  }
  
  if((sbbdata->waves > 0) &&
    !number(0, splat))
  {
    send_to_room("&+LA swirling v&+ror&+Rt&+re&+Lx continues to spill out n&+mi&+Lghtm&+ma&+Lr&+me&+Ls...\n", ch->in_room);
    add_event(event_shadow_spawn, PULSE_VIOLENCE * 2, ch, 0, 0, 0, sbbdata, sizeof(struct sbb_data));
  }
  else
  {
    send_to_char("&+LThe swirling v&+ror&+Rt&+re&+Lx &+Lcollapses!\n", ch);;
    act("&+LThe swirling v&+ror&+Rt&+re&+Lx &+Lcollapses!",
      TRUE, ch, 0, 0, TO_ROOM);
  }
}

void spell_shadow_spawn(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam, savemod, affmod, waves;
  struct damage_messages messages = {
    "&+LYou conjure up the worst &+Rfears&+L of your foes and hurl it at them!",
    "&+LMaddening &+Cvisions &+Lfrom your worst n&+Ri&+Lghtm&+Ra&+Lr&+Re&+Ls sweep over you!",
    "&+LWaves of &+rhorrific &+Lcreatures descend upon $N, who begins to &+Wscream!&n",
    "&+L$N turns &+Wpale white&+L at the n&+mi&+Lghtm&+ma&+Lr&+me&+Ls and dies!",
    "&+L$N turns &+Wpale white&+L at the n&+mi&+Lghtm&+ma&+Lr&+me&+Ls and dies!",
    0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  dam = level * 4 + number(-20, 40);
  // Meteorswarm damage is: 100 + level * 6 + number(1, 40);
  savemod = (int) (GET_LEVEL(ch) / 10);

  affmod = 5;

  // Level 56 npc have 50% to afflict each affect.
  if(IS_NPC(ch) &&
    !GET_MASTER(ch))
  {
    affmod -= 1;
  }
  if(GET_LEVEL(ch) > 49)
  {
    affmod -= 1;
  }
  if(GET_LEVEL(ch) > 53)
  {
    affmod -= 1;
  }
  if(GET_LEVEL(ch) > 55)
  {
    affmod -= 1;
  }

  // Undead just don't care too much about this.
  if(IS_UNDEAD(victim))
  {
    dam = (int) (dam / 5);
  }

  if(spell_damage(ch, victim, dam, SPLDAM_SPIRIT, 0, &messages) == DAM_NONEDEAD);
  {
  // Undead, elite, and greater races don't care about the affects.
    if((IS_ELITE(victim) ||
      IS_GREATER_RACE(victim) ||
      IS_UNDEAD(victim)))
    {
      return;
    }
    if(!NewSaves(victim, SAVING_FEAR, savemod))
    {
      /*
      if (!(number(0, 3))) {

         act("&+LThe n&+mi&+Lghtm&+ma&+Lr&+me&+L bomeces groteqsue and $N starts to vomit!&n", FALSE, ch, 0, victim, TO_CHAR);
         act("&+LThe n&+mi&+Lghtm&+ma&+Lr&+me&+L becomes grotesque and you start to vomit!&n", FALSE,
          ch, 0, victim, TO_VICT);
         act("&+LThe n&+mi&+Lghtm&+ma&+Lr&+me&+L becomes grotesque and $N starts to vomit!", FALSE, ch, 0, victim, TO_NOTVICT);
        spell_disease(46, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
      }
      */
      if(!number(0, affmod))
      {
        act("&+LThe n&+mi&+Lghtm&+ma&+Lr&+me&+L is too much for $N, whose eyes close!&n", FALSE, ch, 0, victim, TO_CHAR);
        act("&+LThe n&+mi&+Lghtm&+ma&+Lr&+me&+L is too much and you close your eyes!&n", FALSE,
         ch, 0, victim, TO_VICT);
        act("&+LThe n&+mi&+Lghtm&+ma&+Lr&+me&+L is too much for $N, whose eyes close!", FALSE, ch, 0, victim, TO_NOTVICT);
        if(ch &&
          victim &&
          !resists_spell(ch, victim))
        {
          blind(ch, victim, PULSE_VIOLENCE);
        }
      }      
      if(!number(0, affmod))
      {
        act("&+LThe n&+mi&+Lghtm&+ma&+Lr&+me&+L scares the bejesus out of $N!&n", FALSE, ch, 0, victim, TO_CHAR);
        act("&+LA wave of utter terror overcomes you as the n&+mi&+Lghtm&+ma&+Lr&+me&+L consumes you!&n", FALSE,
          ch, 0, victim, TO_VICT);
        act("&+LThe n&+mi&+Lghtm&+ma&+Lr&+me&+L scares the bejesus out of $N!", FALSE, ch, 0, victim, TO_NOTVICT);
        if(ch &&
          victim &&
          !resists_spell(ch, victim))
        {
          do_flee(victim, 0, 2);
        }
      }
    }
  }
}

// Event to handle asphyxiate spell.
void event_asphyxiate(P_char ch, P_char victim, P_obj obj, void *data)
{
  int rounds = *( (int *)data );
  int dam, level;
  struct damage_messages messages = {
    "&+cYou smirk as a pair of &+Wgh&+Los&+wtl&+Wy h&+Lan&+wds &+Rtighten &+caround $N&+c's throat!&n",
    "&+cYou gasp for &+Cair &+cas a pair of &+Wgh&+Los&+wtl&+Wy h&+Lan&+wds &+ctighten around your throat!&n",
    "&+c$N &+cgasps for &+Cair &+cas a pair of &+Wgh&+Los&+wtl&+Wy h&+Lan&+wds &+ctighten around $S throat!&n",
    "&+cYour &+Cill&+Wusi&+cona&+Cry &+Whands &+Cchoke &+cthe life out of $N&+c, who falls over &+rdead &+cand &+Llifeless&+c.&n",
    "&+LEverything suddenly goes black as a pair of &+Wgh&+Los&+wtl&+Wy h&+wa&+Lnd&+Ws &+Cchoke &+Lthe life out of you...&n",
    "&+c$N &+Cgasps &+cfor air, &+Weyes &+Mbulging&+c, then falls over, slowly becoming &+Lmotionless&+c.&n", 0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  level = GET_LEVEL(ch);
  if( level > 51 )
    level = 51;

  if( NewSaves(victim, SAVING_SPELL, GET_LEVEL(ch) / 10) )
  {
    // 45 +/- 5 dam at 51.
    dam = (5 * GET_LEVEL(ch) + 65) / 2 + number( 0, 40 );
  }
  else
  {
    // 70 +/- 5 dam at 56.
    dam = 5 * GET_LEVEL(ch) + number( 5, 45 );
  }

  if( spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD
    && (--rounds > 0) )
  {
    add_event(event_asphyxiate, 1, ch, victim, NULL, 0, &rounds, sizeof(int));
  }
}

// _very_ quick dot that does damage over 3 ticks.
void spell_asphyxiate(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int rounds = 3;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }
  if( !HAS_LUNGS(GET_RACE( victim )) )
  {
    act("But $N doesn't need to breathe!", TRUE, ch, NULL, victim, TO_CHAR);
    return;
  }
  act("&+LA pair of &+wGh&+Los&+Wt&+wly &+WH&+La&+wnd&+Ws &+Lappear before &+c$N&+L, close around $S neck and begin to &+Rsqueeze&+L!&n",
    FALSE, ch, NULL, victim, TO_CHAR);
  act("&+wGh&+Los&+Wt&+wly &+Wh&+La&+wnd&+Ws &+Lappear before &+c$N&+L, close around $S neck and begin to &+Rsqueeze&+L!&n",
    FALSE, ch, NULL, victim, TO_NOTVICT);
  act("&+wGh&+Los&+Wt&+wly &+Wh&+La&+wnd&+Ws &+Lappear before you, close around your neck and begin to &+Rsqueeze&+L!&n",
    FALSE, victim, NULL, NULL, TO_CHAR);

  event_asphyxiate(ch, victim, NULL, &rounds);
}

#   define _ILLUSIONIST_MAGIC_C_
#endif
