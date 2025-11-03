/*
 * ***************************************************************************
 * *  File: actnew.c                                           Part of Duris *
 * *  Usage: New commands that don't fit any easily defined category.
 * * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "arena.h"
#include "events.h"
#include "interp.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "mm.h"
#include "damage.h"
#include "objmisc.h"
#include "listen.h"
#include "disguise.h"
#include "config.h"
#include "sql.h"
#include "vnum.obj.h"

/*
 * external variables
 */
extern P_char combat_list;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *command[];
extern char *dirs2[];
extern int hometown_arena[][3];
extern const struct stat_data stat_factor[];
extern struct command_info cmd_info[];
extern struct dex_app_type dex_app[];
extern struct shapechange_struct shapechange_name_list[];
extern struct str_app_type str_app[];
extern const char *item_types[];
extern const char *apply_types[];
extern struct arena_data arena;
extern const char *game_type[];
extern const char *death_type[];
extern const char *map_name[];
extern const flagDef extra_bits[];
extern const flagDef anti_bits[];
extern const char *spells[];
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern struct race_names race_names_table[];
extern const int rev_dir[];
extern const int innate2_abilities[];
extern const int class_innates2[][5];
extern const char *game_type[];
extern struct randomeq_slots slot_data[];
extern struct randomeq_material material_data[];
extern struct zone_data *zone_table;
extern Skill skills[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern int top_of_zone_table;

void yank_make_item(P_char, P_obj);
void lore_item( P_char ch, P_obj obj );
void     set_keywords(P_obj t_obj, const char *newKeys);
void     set_short_description(P_obj t_obj, const char *newShort);

int voting_enabled = 0;

// This sets players to forego all their attacks. It is useful for
// caster type classes which do not want their opponent to riposte or
// other tactical situations.
void do_offensive(P_char ch, char *arg, int cmd)
{

  char Gbuf2[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) || IS_NPC(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch) ||
      GET_STAT(ch) <= STAT_INCAP)
  {
    send_to_char("You have other things to worry about at the moment.\r\n", ch);
    return;
  }

  one_argument(arg, Gbuf2);

  if(!str_cmp(Gbuf2, "off"))
  {
    send_to_char("&+MYou will now stop all your attacks! You will fail all attacks!&n\r\n", ch);

    if(!IS_SET(ch->specials.affected_by5, AFF5_NOT_OFFENSIVE))
    {
      SET_BIT(ch->specials.affected_by5, AFF5_NOT_OFFENSIVE);
      CharWait(ch, PULSE_VIOLENCE);
    }
  }
  else if(*Gbuf2 == '\0')
  {
    send_to_char("&+RYour combat prowess is normal. Kill the enemy!&n\r\n", ch);

    if(IS_SET(ch->specials.affected_by5, AFF5_NOT_OFFENSIVE))
    {
      REMOVE_BIT(ch->specials.affected_by5, AFF5_NOT_OFFENSIVE);
      CharWait(ch, PULSE_VIOLENCE);
    }
  }

  return;
}


/*
 * ** Set PC's "aggressivity" so that PC will attack aggressive monsters
 * ** instanteously when s/he enters a room.
 */

void do_aggr(P_char ch, char *arg, int cmd)
{
  char     Gbuf2[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(arg, Gbuf2);

  if(*Gbuf2 == '\0')
  {
    if(ch->only.pc->aggressive == -1)
    {
      send_to_char("You are not aggressive to monsters.\r\n", ch);
      return;
    }
    snprintf(Gbuf2, MAX_STRING_LENGTH,
        "You will be aggressive unless hp < %d.\r\n",
        ch->only.pc->aggressive);

    send_to_char(Gbuf2, ch);
    return;
  }

#if 0                           /*
                                 * disabled at forger request
                                 */
  /*
   * paladins are good, therefore can't be aggro --TAM
   */
  if (GET_CLASS(ch, CLASS_PALADIN))
  {
    send_to_char("Be an aggressive paladin?!  Not in this lifetime!\r\n", ch);
    send_to_char("You'll have to use better judgement than that.\r\n", ch);
    return;
  }
#endif
  if (*Gbuf2 == '\0')
  {                             /*
                                 * Check Aggressivity
                                 */

    if (ch->only.pc->aggressive == -1)
    {
      send_to_char("You are not aggressive to monsters.\r\n", ch);
      return;
    }
    snprintf(Gbuf2, MAX_STRING_LENGTH, "You will be aggressive unless hp < %d.\r\n", ch->only.pc->aggressive);

    send_to_char(Gbuf2, ch);
    return;
  }
  else if (!str_cmp(Gbuf2, "off") || (atoi(Gbuf2) >= GET_MAX_HIT(ch)))
  {
    /*
     * Turn off aggressivity
     */
    /*
     * anti-paladins are evil, therefore always aggressive. JAB
     */
    /*
     * Added quirk: Player cannot cheat by setting aggro over their
     * max hps. this can be avoided with vitality, but not worth it to
     * fix it too
     */

    /*
       if (GET_CLASS(ch, CLASS_ANTIPALADIN)) {
       send_to_char("What?  Let somebody else get the first blow?  Not a chance!\r\n", ch);
       return;
       } else { */
    ch->only.pc->aggressive = -1;
    send_to_char("You are no longer aggressive to monsters.\r\n", ch);
    return;
    //    }
  }
  else
  {                             /*
                                 * Turn on aggressivity
                                 */

    int      hp = atoi(Gbuf2);

    if (hp < 0)
    {
      send_to_char("Aggressive while dying?  Not likely!\r\n", ch);
      return;
    }
    ch->only.pc->aggressive = (short) hp;
    send_to_char("OK.\r\n", ch);
    return;
  }
}

/*
 * ** Same as "say", except that it only sends what is said ** to people
 * in your group, and also works if group members ** are in different
 * rooms (sort of like conference).
 */
void do_gsay(P_char ch, char *arg, int cmd)
{
  struct group_list *gl;
  int     *rm_checked;
  int      i, j, k, numb, skip;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (IS_MORPH(ch))
  {
    send_to_char("You are unable to speak in this form\r\n", ch);
    return;
  }
  for (i = 0; arg[i] == ' '; i++) ;

  if (arg[i] == '\0')
  {
    send_to_char("Yes, but WHAT do you want to gsay?\r\n", ch);
    return;
  }
  if (!ch->group)
  {
    send_to_char("But you are a member of no group?!\r\n", ch);
    return;
  }
  if( IS_ROOM(ch->in_room, ROOM_UNDERWATER) && !IS_TRUSTED(ch))
  {
    send_to_char("You cannot group say while underwater...\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You cannot speak in that form!\r\n", ch);
    return;
  }
  if (IS_AFFECTED2(ch, AFF2_SILENCED) || affected_by_spell(ch, SPELL_SUPPRESSION))
  {
    send_to_char("Nothing happens.\r\n", ch);
    return;
  }
  for (gl = ch->group; gl; gl = gl->next)
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "&+G$n&+G group-says %s'%s'", language_known(ch, gl->ch),
        language_CRYPT(ch, gl->ch, arg + i));
    if (!is_silent(gl->ch, FALSE))
      act(Gbuf1, FALSE, ch, 0, gl->ch, TO_VICT | ACT_SILENCEABLE | ACT_PRIVATE);
  }

  if (IS_SET(ch->specials.act, PLR_ECHO))
  {
    if (!is_silent(ch, TRUE) && can_talk(ch))
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "&+GYou group-say '%s'\r\n", arg + i);
      send_to_char(Gbuf1, ch, LOG_PRIVATE);
    }
  }
  else if (!is_silent(ch, TRUE) && can_talk(ch))
    send_to_char("Ok.\r\n", ch);

  // New gsay listen routine
  listen_broadcast(ch, (arg + i), LISTEN_GSAY);

}

/*
 * Give consent to another player.  Effect of consent: you get no saving
 * throw against ANY spell cast by person you are giving consent.  And it
 * is not revoked by them abusing it to attack you.  All penalties for
 * attacking another player still apply though.  JAB
 */
void do_consent(P_char ch, char *argument, int cmd)
{
  P_char   target;
  char     Gbuf3[MAX_STRING_LENGTH];
  struct char_link_data *cld;
  P_desc   d;

  one_argument(argument, Gbuf3);

  if (!*Gbuf3)
  {
    send_to_char("You no longer feel generous and revoke your consent.\r\n",
        ch);
    clear_links(ch, LNK_CONSENT);
    return;
  }
  if (!strcmp(Gbuf3, "who"))
  {
    send_to_char("The following players have given you their consent:\r\n",
        ch);
    for (cld = ch->linked; cld; cld = cld->next_linked)
    {
      send_to_char(cld->linking->player.name, ch);
      send_to_char("\r\n", ch);
    }
    return;
  }

  target = NULL;
  /*
   * switching to descriptor list, rather than get_char_vis, since it
   * was lagging hell out of things. JAB
   */
  for (d = descriptor_list; d; d = d->next)
  {
    if (!d->character || d->connected || !d->character->player.name)
      continue;
    if (!isname(d->character->player.name, Gbuf3))
      continue;
    if (!CAN_SEE_Z_CORD(ch, d->character))
      continue;
    target = d->character;
    break;
  }

  if (target && !IS_TRUSTED(ch) && !IS_TRUSTED(target))
    if ((racewar(ch, target) && !IS_DISGUISE(target)) ||
        //(IS_DISGUISE(target) && (EVIL_RACE(ch) != EVIL_RACE(target))) ||
        (IS_ILLITHID(ch) && !IS_ILLITHID(target)))
      target = NULL;

  /*

     if ((target = get_char_vis(ch, Gbuf3)) == NULL) {
     send_to_char("You attempt to give consent...\r\n", ch);
     return;
     } */
  /*
     if (racewar(ch, target)) {
     send_to_char("No one by that name here...\r\n", ch);
     return;
     } */

  if (!target)
  {
    send_to_char("No one by that name here...\r\n", ch);
    return;
  }
  if (IS_NPC(target))
  {
    send_to_char("You attempt to give consent...\r\n", ch);
    return;
  }

  if (IS_ILLITHID(ch) && !IS_ILLITHID(target) && !IS_TRUSTED(ch))
  {
    send_to_char("You attempt go give consent...\r\n", ch);
    return;
  }

  if (IS_DISGUISE(target) && ch->in_room != target->in_room)
  {
    send_to_char("You attempt to give consent...\r\n", ch);
    return;
  }
  if (ch == target)
  {
    send_to_char("You can't give yourself consent!\r\n", ch);
    return;
  }
  /*
   * If you want to remove consent, just type "consent" and it will revoke
   *
   if (ch->specials.consent) {

//   snprintf(Gbuf3, MAX_STRING_LENGTH,
//   "You no longer give consent to %s.\r\n", GET_NAME(ch->specials.consent));
//   send_to_char(Gbuf3, ch);

act("You attempt to give consent...", FALSE, ch, 0, ch->specials.consent, TO_CHAR);
}
*/
/*  snprintf(Gbuf3, MAX_STRING_LENGTH, "You now give consent to %s.\r\n", GET_NAME(target));
    send_to_char(Gbuf3, ch); */
act("You attempt to give consent...", FALSE, ch, 0, target, TO_CHAR);
link_char(ch, target, LNK_CONSENT);
act("$n has just given you $s consent.", FALSE, ch, 0, target, TO_VICT);
}

void do_stampede(P_char ch, char *arg, int cmd)
{
  int      missed;
  int      count;
  P_char   vict, next_vict = 0;

  /*  struct group_list *gl; */

  if (!SanityCheck(ch, "do_stampede"))
    return;
  if (!has_innate(ch, INNATE_STAMPEDE))
  {
    send_to_char("You're not shaped right for that.\r\n", ch);
    return;
  }
  if (!HAS_FOOTING(ch))
  {
    send_to_char("You have no footing here!\r\n", ch);
    return;
  }
  if (IS_ROOM(ch->in_room, ROOM_SINGLE_FILE))
  {
    send_to_char("It's too cramped in here to stampede!\r\n", ch);
    return;
  }
  if (!on_front_line(ch))
  {
    send_to_char("Stampeding from way back here? I think not.\r\n", ch);
    return;
  }
  count = 0;
  appear(ch);
  send_to_char("You go madly crashing around the room!\r\n", ch);
  act("$n crashes wildly around!", TRUE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;
    if (count >= number(2, 3))
      continue;
    if (vict == ch)
      continue;
    if (!CAN_SEE(ch, vict))
      continue;

    /*  replaced with more proper check below
        if ((vict->following == ch) || (ch->following == vict))
        continue;

        if (vict->group == ch->group)
        continue;*/

    if (!should_area_hit(ch, vict))
      continue;

    /* can't stampede some beings */

    if ((GET_RACE(vict) == RACE_GHOST) ||
        (GET_RACE(vict) == RACE_A_ELEMENTAL) ||
        (GET_RACE(vict) == RACE_EFREET) ||
        (IS_ELITE(vict)) ||
        (IS_AFFECTED4(vict, AFF4_PHANTASMAL_FORM)))
      continue;

    if (GET_OPPONENT(vict))
      continue;

    missed = 0;

    if(!number(0, 9))
    { }
    else if (number(1, 100) >
        (GET_LEVEL(ch) - GET_LEVEL(vict) - STAT_INDEX(GET_C_AGI(vict)) +
         dice(10, IS_AFFECTED(vict, AFF_AWARE) ? 5 : 10)) ||
        !on_front_line(vict))
    {
      missed = 1;
      act("$n jumps nimbly out of the way!", 0, vict, 0, 0, TO_ROOM);
      act("You jump out of the way of $n's stampede!", 0, ch, 0, vict,
          TO_VICT);
    }
    if (!missed)
    {
      act("$n crashes wildly into $N!", 0, ch, 0, vict, TO_ROOM);
      act("You crash wildly into $N!", 0, ch, 0, vict, TO_CHAR);
      act("$n crashes wildly into you!", 0, ch, 0, vict, TO_VICT);
      SET_POS(vict, POS_PRONE + GET_STAT(vict));
      Stun(vict, ch, PULSE_VIOLENCE / 2, TRUE);
      count++;
    }
    set_fighting(vict, ch);
  }

  count = MIN(2, count);

  if (char_in_list(ch))
    CharWait(ch, PULSE_VIOLENCE * MIN(3, (count / 2)));

  return;
}

// SPEC SKILL FOR WARRIOR - Kvark
void do_war_cry(P_char ch, char *arg, int cmd)
{
  int      hpoints = (GET_CHAR_SKILL(ch, SKILL_WAR_CRY) / 2);
  struct group_list *gl;
  int      dampoints = (GET_CHAR_SKILL(ch, SKILL_WAR_CRY) / 20);
  int      skl;
  struct affected_type af;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if (!GET_CHAR_SKILL(ch, SKILL_WAR_CRY))
  {
    send_to_char("You don't know how to war cry.\r\n", ch);
    return;
  }

  send_to_char("You unleash a &+rpowerful &+RSCREAM!!&n\r\n", ch);
  act("$n lets loose with a &+rpowerful &+RSCREAM!!&n", TRUE, ch, 0, 0, TO_ROOM);

  if( ch->group )
  {
    for( gl = ch->group; gl; gl = gl->next )
    {
      if( gl->ch->in_room == ch->in_room )
      {
        if( !affected_by_spell(gl->ch, SKILL_WAR_CRY) )
        {
          bzero(&af, sizeof(af));
          af.type = SKILL_WAR_CRY;
          af.duration = (dampoints);
          //HPS
          af.modifier = hpoints;
          af.location = APPLY_HIT;
          affect_to_char(gl->ch, &af);
          //HIT
          af.modifier = dampoints;
          af.location = APPLY_HITROLL;
          affect_to_char(gl->ch, &af);
          //DAM
          af.modifier = dampoints;
          af.location = APPLY_DAMROLL;
          affect_to_char(gl->ch, &af);
          update_pos(gl->ch);
          send_to_char("You feel like you could &+rfight&n forever.\r\n", gl->ch);
          act("&+L$n becomes alert and ready to &+rfight&n.", TRUE, gl->ch, 0, 0, TO_ROOM);
          notch_skill( ch, SKILL_WAR_CRY, 10 );
        }
      }
    }
  }
  else
  {
    if( !affected_by_spell(ch, SKILL_WAR_CRY) )
    {
      bzero(&af, sizeof(af));
      af.type = SKILL_WAR_CRY;
      af.duration = (dampoints / 2);
      //HPS
      af.modifier = hpoints;
      af.location = APPLY_HIT;
      affect_to_char(ch, &af);
      //HIT
      af.modifier = dampoints;
      af.location = APPLY_HITROLL;
      affect_to_char(ch, &af);
      //DAM
      af.modifier = dampoints;
      af.location = APPLY_DAMROLL;
      affect_to_char(ch, &af);
      update_pos(ch);
      send_to_char("You feel like you could &+rfight&n forever.\r\n", ch);
      act("&+L$n becomes alert and ready to &+rfight&n.", TRUE, ch, 0, 0, TO_ROOM);
      notch_skill(ch, SKILL_WAR_CRY, 25);
    }
  }

  CharWait(ch, PULSE_VIOLENCE * 2);
}


int do_roar_of_heroes(P_char ch)
{

  struct group_list *gl;
  struct affected_type af;

  if( !IS_ALIVE(ch) )
  {
    return 0;
  }

  if(IS_NPC(ch) ||
      GET_LEVEL(ch) < 56)
  {
    //send_to_char("Mmmm, in your wildest dreams...\r\n", ch);
    return 0;
  }

  if(affected_by_spell(ch, SKILL_ROAR_OF_HEROES))
  {
    return 0;
  }

  send_to_char("&+LYour mighty &+rbattle cry &+Lrallies your comrades and instills &+wfear\n&+Linto the hearts of your enemies.&n\r\n", ch); 
  act("&+L$n's mighty &+rbattle cry &+Lrallies $s comrades and instills fear\n&+Linto the hearts of $s enemies.&n", TRUE, ch, 0, 0, TO_ROOM);

  if (ch && ch->group)
  {
    gl = ch->group;
    /* leader first */
    if (gl->ch->in_room == ch->in_room)
    {
      if (!affected_by_spell(gl->ch, SKILL_ROAR_OF_HEROES))
      {
        bzero(&af, sizeof(af));
        af.type = SKILL_ROAR_OF_HEROES;
        af.duration = 5;
        af.modifier = af.modifier = number(9, 13);
        af.location = APPLY_CON_MAX;
        affect_to_char(gl->ch, &af);

        update_pos(gl->ch);
        send_to_char("&+rAdrenaline burns your veins as the battle cry rings in your ears.&n\r\n",
            gl->ch);

        // Commenting out the spam portion. Apr09 -Lucrot
        // switch (number(1,6))
        // {
        // case 1:
        // {
        // act("A fire is lit in $n's eyes and a grin spreads across $s lips\nas the fever of battle takes hold.", TRUE, gl->ch, 0,
        // 0, TO_ROOM);

        // break;
        // }
        // case 2:
        // {
        // act("A look of insanity flashes across $n's eyes.", TRUE, gl->ch, 0,
        // 0, TO_ROOM);
        // do_action(gl->ch, 0, CMD_CACKLE);
        // break;
        // }
        // case 3:
        // {
        // act("$n's eyes harden with resolve as the strength of the battle cry courses through $s spirit.", TRUE, gl->ch, 0,
        // 0, TO_ROOM);
        // break;
        // }
        // case 4:
        // {
        // act("A sense of calmness spreads across $n's features as $e prepares for battle.", TRUE, gl->ch, 0,
        // 0, TO_ROOM);
        // break;
        // }
        // case 5:
        // {
        // do_action(gl->ch, 0, CMD_GRIN);
        // break;
        // }
        // case 6:
        // {
        // act("A look of insanity flashes across $n's eyes.", TRUE, gl->ch, 0,
        // 0, TO_ROOM);
        // do_say(gl->ch, "Wake up, time to die.", -4);
        // break;
        // }
        // default:
        // break;
        // }
      }
    }
    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if (gl->ch->in_room == ch->in_room)
      {
        if (!affected_by_spell(gl->ch, SKILL_ROAR_OF_HEROES))
        {
          bzero(&af, sizeof(af));
          af.type = SKILL_ROAR_OF_HEROES;
          af.duration = 5;
          af.modifier = af.modifier = number(9, 13);

          af.location = APPLY_CON_MAX;
          affect_to_char(gl->ch, &af);

          update_pos(gl->ch);
          send_to_char("&+rAdrenaline burns your veins as the battle cry rings in your ears.&n\r\n",
              gl->ch);

          // switch(number(1,6))
          // {
          // case 1:
          // {
          // act("A fire is lit in $n's eyes and a grin spreads across $s lips\nas the fever of battle takes hold.", TRUE, gl->ch, 0,
          // 0, TO_ROOM);

          // break;
          // }
          // case 2:
          // {
          // act("A look of insanity flashes across $n's eyes.", TRUE, gl->ch, 0,
          // 0, TO_ROOM);
          // do_action(gl->ch, 0, CMD_CACKLE);
          // break;
          // }
          // case 3:
          // {
          // act("$n's eyes harden with resolve as the strength of the battle cry courses through $s spirit.", TRUE, gl->ch, 0,
          // 0, TO_ROOM);
          // break;
          // }
          // case 4:
          // {
          // act("A sense of calmness spreads across $n's features as $e prepares for battle.", TRUE, gl->ch, 0,
          // 0, TO_ROOM);
          // break;
          // }
          // case 5:
          // {
          // do_action(gl->ch, 0, CMD_GRIN);
          // break;
          // }
          // case 6:
          // {
          // act("A look of insanity flashes across $n's eyes.", TRUE, gl->ch, 0,
          // 0, TO_ROOM);
          // do_say(gl->ch, "Wake up, time to die.", -4);
          // break;
          // }
          // default:
          // break;

          // }
        }
      }
    }
  }
  else
  {
    if (!affected_by_spell(ch, SKILL_ROAR_OF_HEROES))
    {
      bzero(&af, sizeof(af));
      af.type = SKILL_ROAR_OF_HEROES;
      af.duration = 5;
      af.modifier = af.modifier = number(9, 13);
      af.location = APPLY_CON_MAX;
      affect_to_char(ch, &af);
      update_pos(ch);
      send_to_char("&+rAdrenaline burns your veins as the battle cry rings in your ears.&n\r\n",
          ch);

      // switch (number(1,6))
      // {
      // case 1:
      // {
      // act("A fire is lit in $n's eyes and a grin spreads across $s lips\nas the fever of battle takes hold.", TRUE, ch, 0,
      // 0, TO_ROOM);

      // break;
      // }
      // case 2:
      // {
      // act("A look of insanity flashes across $n's eyes.", TRUE, ch, 0,
      // 0, TO_ROOM);
      // do_action(ch, 0, CMD_CACKLE);
      // break;
      // }
      // case 3:
      // {
      // act("$n's eyes harden with resolve as the strength of the battle cry courses through $s spirit.", TRUE, ch, 0,
      // 0, TO_ROOM);
      // break;
      // }
      // case 4:
      // {
      // act("A sense of calmness spreads across $n's features as $e prepares for battle.", TRUE, ch, 0,
      // 0, TO_ROOM);
      // break;
      // }
      // case 5:
      // {
      // do_action(ch, 0, CMD_GRIN);
      // break;
      // }
      // case 6:
      // {
      // act("A look of insanity flashes across $n's eyes.", TRUE, ch, 0,
      // 0, TO_ROOM);
      // do_say(ch, "Wake up, time to die.", -4);
      // break;
      // }
      // default:
      // break;

      // }
    }
  }

  return 1;
  CharWait(ch, PULSE_VIOLENCE * 1);
}

void do_flurry_of_blows(P_char ch, char *arg)
{
  P_char   tch = NULL, next_tch = NULL;
  char     buf[MAX_STRING_LENGTH];
  int	   num_attacks, max_num_attacks, hit_all, num_targets = 0, count = 0, num_hits_per_target = 0;

  if (!GET_CHAR_SKILL(ch, SKILL_FLURRY_OF_BLOWS))
  {
    send_to_char("You don't know how to.\r\n", ch);
    return;
  }
  if (is_in_safe(ch))
  {
    send_to_char("You feel ashamed to try to disrupt the tranquility"
        " of this place.\r\n", ch);
    return;
  }
  if (IS_ROOM(ch->in_room, ROOM_SINGLE_FILE))
  {
    send_to_char("Sorry, it's too cramped here for nasty maneuvers!\r\n", ch);
    return;
  }

  if (affected_by_spell(ch, SKILL_FLURRY_OF_BLOWS))
  {
    send_to_char("You need to catch your breath before trying again!\n", ch);
    return;
  }

  one_argument(arg, buf);
  hit_all = !str_cmp(buf, "all");
  num_attacks = MonkNumberOfAttacks(ch);
  max_num_attacks = num_attacks * 2;

  /* Get number of targets in room */

  LOOP_THRU_PEOPLE(tch, ch)
  {
    if (!should_area_hit(ch, tch))
      continue;

    if (!hit_all && (GET_OPPONENT(tch) != ch))
      continue;

    num_targets++;
  }

  if (!num_targets)
  {
    send_to_char("Seems as though noone is around to fight you.\n", ch);
    return;
  }

  num_targets = MAX(1, num_targets);
  num_hits_per_target = int (max_num_attacks / num_targets);
  num_hits_per_target = BOUNDED(1, num_hits_per_target, num_attacks);

  act("&+gFlowing into the stance of the &+Gviper &+gyou unleash a flurry of blows!&n", FALSE, ch, 0, 0, TO_CHAR);
  act("Placing $s weight low $n shifts into a more aggressive stance.", FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[ch->in_room].people;tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;

    if (!should_area_hit(ch, tch))
      continue;

    if (!hit_all && (GET_OPPONENT(tch) != ch))
      continue;

    if (!CAN_SEE(ch, tch))
    {
      if (!notch_skill(ch, SKILL_BLINDFIGHTING,
            get_property("skill.notch.blindFighting", 1)) && 
          (number(1,101) > (IS_PC(ch) ? (1 + GET_CHAR_SKILL(ch, SKILL_BLINDFIGHTING)) : (MIN(100,GET_LEVEL(ch))))))
        continue;
    }
    if (!notch_skill(ch, SKILL_FLURRY_OF_BLOWS, get_property("skill.notch.offensive", 7))
      && (number(1,101) > (IS_PC(ch) ? (1 + GET_CHAR_SKILL(ch, SKILL_FLURRY_OF_BLOWS)) : (MIN(100, GET_LEVEL(ch))))))
    {
      continue;
    }

    /* Ok we got this far.. lets try and hit them! */

    for (int i=1;i <= num_hits_per_target && count <= max_num_attacks ; i++, count++)
    {
      if (!should_not_kill(ch, tch) && (GET_STAT(tch) != STAT_DEAD))
#ifndef NEW_COMBAT
        hit(ch, tch, NULL);

#else
      hit(ch, tch, NULL, TYPE_UNDEFINED, number(0, 10),       /* fix up for real later.. */
          TRUE, FALSE);
#endif  
    }

  }
  CharWait(ch, PULSE_VIOLENCE * 3);
  set_short_affected_by(ch, SKILL_FLURRY_OF_BLOWS, 10 * WAIT_SEC);
}


/*
 * ** Warrior skill "hitall"
 */

void do_hitall(P_char ch, char *arg, int cmd)
{
  byte  percent;
  int count, hit_all;
  P_char   mob, next_mob;
  char  Gbuf2[MAX_STRING_LENGTH];

  if(!ch)
  {
    logit(LOG_EXIT, "do_hitall call in actnew.c with no ch");
    raise(SIGSEGV);
  }

  if(!SanityCheck(ch, "do_hitall"))
  {
    return;
  }

  if (GET_CHAR_SKILL(ch, SKILL_FLURRY_OF_BLOWS))
  {
    do_flurry_of_blows(ch, arg);
    return;
  }

  if (!GET_CHAR_SKILL(ch, SKILL_HITALL))
  {
    send_to_char("You don't know how to.\r\n", ch);
    return;
  }
  if (is_in_safe(ch))
  {
    send_to_char("You feel ashamed to try to disrupt the tranquility"
        " of this place.\r\n", ch);
    return;
  }
  if (IS_ROOM( ch->in_room, ROOM_SINGLE_FILE))
  {
    send_to_char("Sorry, it's too cramped here for nasty maneuvers!\r\n", ch);
    return;
  }
  /* Find out whether to hit "all" or just aggressive monsters */

  one_argument(arg, Gbuf2);
  hit_all = !str_cmp(Gbuf2, "all");

  if (hit_all)
  {
    send_to_char("You madly swing your weapon around, trying to hit all of your opponents...\r\n", ch);
    act("$n madly swings $s weapon...", TRUE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    send_to_char("You swing your weapon wide, trying to hit your opponents...\r\n", ch);
    act("$n swings $s weapon wide...", TRUE, ch, 0, 0, TO_ROOM);
  }

  /* Hit all aggressive monsters in room */

  count = 0;

  for (mob = world[ch->in_room].people; mob; mob = next_mob)
  {
    next_mob = mob->next_in_room;

    if (!should_area_hit(ch, mob))
      continue;

    if (!has_innate(ch, INNATE_EYELESS) && !CAN_SEE(ch, mob))
      if (number(1, 101) >
          (IS_PC(ch) ? GET_CHAR_SKILL(ch, SKILL_BLINDFIGHTING) : 90))
        continue;
      else
        notch_skill(ch, SKILL_BLINDFIGHTING, 5);

    if (IS_NPC(ch) && (GET_OPPONENT(mob) != ch))
      continue;

    percent = number(1, 101);

    if (!hit_all && !IS_AGGRESSIVE(mob))
      continue;

    if (GET_CHAR_SKILL(ch, SKILL_HITALL) >= percent)
      if (!should_not_kill(ch, mob))
#ifndef NEW_COMBAT
        hit(ch, mob, ch->equipment[PRIMARY_WEAPON]);
    if (GET_CLASS(ch, CLASS_BERSERKER) && GET_STAT(mob) != STAT_DEAD)
    {
      if (affected_by_spell(ch, SKILL_BERSERK))
      {
        hit(ch, mob, ch->equipment[PRIMARY_WEAPON]);
      }
    }                           // new zerker stuff
#else
    hit(ch, mob, ch->equipment[WIELD], TYPE_UNDEFINED, number(0, 10),       /* fix up for real later.. */
        TRUE, FALSE);
    if (GET_CLASS(ch, CLASS_BERSERKER))
    {
      if (affected_by_spell(ch, SKILL_BERSERK))
      {
        hit(ch, mob, ch->equipment[WIELD], TYPE_UNDEFINED, number(0, 10),
            TRUE, FALSE);
      }

    }                           // same as above
#endif

    // riposte, damage shield, etc can kill the character
    // and it appears we've had a crash due to this, so adding this
    // check here
    if(!IS_ALIVE(ch))
      break;

    count++;
  }

  if (char_in_list(ch))
  {
    if (!count)
    {
      send_to_char("...but fail to impress anyone!\r\n", ch);
      act("...but in the end $e fails to impress anyone.", TRUE, ch, 0, 0, TO_ROOM);
    }

    notch_skill(ch, SKILL_HITALL, get_property("skill.notch.offensive", 7));
    if (GET_CLASS(ch, CLASS_BERSERKER))
    {
      if (affected_by_spell(ch, SKILL_BERSERK))
      {
        CharWait(ch, 2 * PULSE_VIOLENCE);
      }
      else
      {
        CharWait(ch, PULSE_VIOLENCE * MAX(2, (count / 2)));
      }
    }
    else
    {
      CharWait(ch, PULSE_VIOLENCE * MAX(2, (count / 2)));
    }
  }
}

#if TRAPS
/*
 * ** Rogue skill "trap"
 */
void do_trap(P_char ch, char *arg, int cmd)
{
  P_room   room;
  byte     percent;
  P_char   monster.next_ch;

  /* Check to see if "ch" is allowed to lay a trap */

  if (IS_NPC(ch) || GET_CHAR_SKILL(ch, SKILL_TRAP) == 0)
  {
    send_to_char("You don't know how to lay a trap.\r\n", ch);
    return;
  }
  if (IS_RIDING(ch))
  {
    send_to_char("While mounted?  I don't think so...\r\n", ch);
    return;
  }
  room = world + ch->in_room;

  if (CHAR_IN_SAFE_ROOM(ch))
  {
    send_to_char("You can't lay a trap in here!\r\n", ch);
    return;
  }
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("You cannot possibly lay a trap while fighting!\r\n", ch);
    return;
  }
  /* Now, it is legal to lay a trap .. check to see if */
  /* rogue can indeed lay the trap based on how learned s/he */
  /* at setting trap */

  percent = number(1, 101);     /* 101 is a total failure */

  if (GET_CHAR_SKILL(ch, SKILL_TRAP) < percent)
  {
    send_to_char("You fail to lay a trap.\r\n", ch);
  }
  else
  {

    send_to_char("You skillfully lay down a trap.\r\n", ch);
    act("$n skillfully sets up a trap.", 1, ch, 0, 0, TO_ROOM);

    /* SET_TRAP(ch, room); */

    /*    LOOP_THRU_PEOPLE(monster, ch)  */

    int      dice;

    for (monster = world[ch->in_room].people; monster; monster = ch_next)
    {
      ch_next = monster->next_in_room;

      if (GET_STAT(ch) == STAT_DEAD)
      {
        break;
      }
      if (ch->in_room != monster->in_room)
      {
        break;
      }
      if (IS_PC(monster))
        continue;

      if (!MIN_POS(monster, POS_STANDING, STAT_RESTING))
        continue;

      /* Decide if monster should attack */

      if( IS_FIGHTING(monster) || IS_DESTROYING(monster) )
        continue;

      if (!CAN_SEE(monster, ch))
        continue;

      /* Now roll the dice to see if it should attack */

      dice = number(1, 10);

      if (dice < 4)
      {
        continue;
      }
      /* ATTACK!!!!!!!!!! */
      act("$n notices $N setting a trap, and attacks in anger!", 0, monster,
          0, ch, TO_NOTVICT);
      act("$N notices you setting a trap, and attacks!", 0, ch, 0, monster,
          TO_CHAR);

      hit(monster, ch, NULL);
    }
  }

  notch_skill(ch, SKILL_TRAP, 20);
}

#endif

/*
 * A mechanical list of all the commands.  Added many options, cause spam
 * was awful with ~500 commands.  JAB
 */

void do_commands(P_char ch, char *arg, int cmd)
{
  int      no, i, mode, t_pos;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
    return;

  if (!*arg)
    mode = 0;                   /*
                                 * commands legal to level and current
                                 * position
                                 */
  else if (is_abbrev(arg, "all"))
    mode = 1;                   /*
                                 * same as old list, everything
                                 */
  else if (is_abbrev(arg, "socials"))
    mode = 2;                   /*
                                 * only commands that have 'do_action' as
                                 * their function
                                 */
  else if (is_abbrev(arg, "nosocials"))
    mode = 3;                   /*
                                 * old list, not including socials
                                 */
  else
  {
    send_to_char("Syntax:  commands [all | socials | nosocials]\r\n", ch);
    return;
  }

  send_to_char("The following commands are available:\r\n\r\n", ch);
  *Gbuf1 = '\0';

  if (IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    t_pos = POS_PRONE + STAT_DEAD;
  else
    t_pos = ch->specials.position;

  for (no = 1, i = 0; *command[i] != '\n'; i++)
  {
    if ((int) GET_LEVEL(ch) >= cmd_info[i + 1].minimum_level)
    {
      switch (mode)
      {
        case 0:
          if ((cmd_info[i + 1].command_pointer == do_action) ||
              ((cmd_info[i + 1].minimum_position & 3) > (t_pos & 3)) ||
              ((cmd_info[i + 1].minimum_position & STAT_MASK) >
               (t_pos & STAT_MASK)))
            continue;
          break;
        case 1:
          break;
        case 2:
          if ((cmd_info[i + 1].command_pointer != do_action))
            continue;
          break;
        case 3:
          if ((cmd_info[i + 1].command_pointer == do_action))
            continue;
          break;
      }

      snprintf(Gbuf1 + strlen(Gbuf1), MAX_STRING_LENGTH - strlen(Gbuf1), "%-16s", command[i]);
      if (!(no % 5))
        strcat(Gbuf1, "\r\n");
      no++;
    }
  }
  snprintf(Gbuf1 + strlen(Gbuf1), MAX_STRING_LENGTH - strlen(Gbuf1),
      "\r\n\r\nCommands listed:  %d of %d total.   (Use 'commands all' to see a full list)\r\n",
      no - 1, i);
  page_string(ch->desc, Gbuf1, 1);
}

/*
 * ** Rogue's skill subterfuge.  Basically, confuse NPC's with ** memories
 * as to erase their entire memory.
 */
void do_subterfuge(P_char ch, char *arg, int cmd)
{
  P_char   npc;                 /* Which NPC */
  char     name[MAX_INPUT_LENGTH];
  byte     percent;

  if (ch->only.pc->skills[SKILL_SUBTERFUGE].learned == 0)
  {
    send_to_char("You don't know how.\r\n", ch);
    return;
  }
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("No way!! You simply are not able to concentrate.\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_BOUND))
  {
    send_to_char("You're too bound to try.\r\n", ch);
    return;
  }
  /* Get here iff. no restriction on "subterfuge" */

  one_argument(arg, name);
  if (!(npc = get_char_room_vis(ch, name)))
  {
    send_to_char("But who is your victim?\r\n", ch);
    return;
  }
  if (IS_PC(npc) || (npc->only.npc->memory == NULL) ||
      !IS_SET(npc->specials.act, ACT_MEMORY))
  {
    send_to_char
      ("Your target simply does not have the intellect to remember.\r\n", ch);
    return;
  }
  percent = number(1, 101) * 6 / 5;

  if (ch->only.pc->skills[SKILL_SUBTERFUGE].learned < percent)
  {
    send_to_char("You failed to confuse your target.\r\n", ch);
  }
  else if ((GET_LEVEL(npc) - GET_LEVEL(ch)) * 5 > number(1, 101))
  {
    act("$N's mind is too clear to be confused.", FALSE, ch, 0, npc, TO_CHAR);
  }
  else
  {
    act("$N looks pretty confused..", FALSE, ch, 0, npc, TO_NOTVICT);
    act("$N looks pretty confused..", FALSE, ch, 0, npc, TO_CHAR);
    clearMemory(npc);
    witness_destroy(npc);

    return;
  }

  notch_skill(ch, SKILL_SUBTERFUGE, 3);

  if (CAN_SEE(npc, ch))
  {
    remember(npc, ch);
    if( !IS_FIGHTING(npc) && !IS_DESTROYING(npc) )
      MobStartFight(npc, ch);
  }
}

void do_disarm(P_char ch, char *arg, int cmd)
{
  int      pos, percent, rnd_num, bits;
  char     obj_name[128], vict_name[128];
  P_obj    obj, trap;
  P_char   victim;

  if( (GET_CHAR_SKILL(ch, SKILL_DISARM) == 0) && !IS_TRUSTED(ch) )
  {
    send_to_char("Heh, you might end up losing a finger.\n", ch);
    return;
  }

  arg = one_argument(arg, vict_name);
  if( *vict_name == '\0' )
  {
    if( IS_FIGHTING(ch) )
    {
      send_to_char("Disarm who?\r\n", ch);
    }
    else
    {
      send_to_char("Disarm what?\r\n", ch);
    }
    return;
  }
  /* begin paste in of trap disarming */
  if( (bits = generic_find(vict_name, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &trap))
    && (GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF) || GET_CLASS(ch, CLASS_THIEF)) )
  {
    if( trap->trap_charge )
    {
      if( GET_CHAR_SKILL(ch, SKILL_DISARM) > number(1, 91) )
      {
        send_to_char("Success! The vile trap is now disarmed.\r\n", ch);
        trap->trap_charge = 0;
        trap->trap_eff = 0;
        trap->trap_dam = 0;
        return;
      }
      else
      {
        send_to_char("Oops...\r\n", ch);
        trapdamage(ch, trap);
      }
    }
  }
  /* end trap disarming */
  /* we now return you to your regularly scheduled code */

  one_argument(arg, obj_name);
  if( *obj_name == '\0' )
  {
    send_to_char("Disarm what?\r\n", ch);
    return;
  }

  if( !ch->equipment[WIELD] )
  {
    send_to_char("You must be wielding some kind of weapon.\r\n", ch);
    return;
  }

  if( !(victim = get_char_room_vis(ch, vict_name)) )
  {
    send_to_char("That creature isn't present.\r\n", ch);
    return;
  }

  if( victim == ch )
  {
    send_to_char("Be serious... use remove to disarm yourself.\r\n", ch);
    return;
  }

  percent = 0;
  if( !IS_FIGHTING(ch) || GET_OPPONENT(ch) != victim )
  {
    if( GET_SPEC(ch, CLASS_WARRIOR, SPEC_SWASHBUCKLER) )
    {
      if( !IS_FIGHTING(ch) )
      {
        act("You attempt to enter battle with a disarm.", FALSE, ch, 0, victim, TO_CHAR);
        percent = -15;
      }
      else
      {
        act("You attempt to disarm $N while fighting someone else.", FALSE, ch, 0, victim, TO_CHAR);
        percent = -30;
      }
    }
    else
    {
      act("You must engage in combat with $N before $E can be disarmed.", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
  }
  for( obj = NULL, pos = 0; pos < MAX_WEAR; pos++ )
  {
    if( (obj = victim->equipment[pos]) && isname(obj_name, obj->name) && CAN_SEE_OBJ(ch, obj) )
    {
      break;
    }
  }

  if( obj == NULL )
  {
    act("You can't seem to find it on $N.", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

  if( obj->type != ITEM_WEAPON && obj->type != ITEM_FIREWEAPON )
  {
    act("You can only disarm weapons.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  percent += (GET_CHAR_SKILL(ch, SKILL_DISARM) + STAT_INDEX(GET_C_DEX(ch)) -
      STAT_INDEX(GET_C_DEX(victim)) + GET_LEVEL(ch) - GET_LEVEL(victim)) / 2;

  rnd_num = number(1, 100);

  if( percent > rnd_num )
  {
    act("$n successfully knocks $N's weapon from $S grips!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n forces your weapon out of your hands with a fancy disarming maneuver.",
       FALSE, ch, 0, victim, TO_VICT);
    act("You make a great effort, and send $N's weapon out of control..",
        FALSE, ch, 0, victim, TO_CHAR);
    notch_skill(ch, SKILL_DISARM, 10);
    obj_to_char(unequip_char(victim, pos), victim);
    strip_holy_sword( victim );
    set_short_affected_by(victim, SKILL_DISARM, 3 * PULSE_VIOLENCE);
    // Engage ch only if successful disarm..
    if( !IS_FIGHTING(ch) )
    {
      set_fighting(ch, victim);
    }
  }
  else if (rnd_num >= 90)
  {
    act("$n fails $s disarming maneuver so badly, $e fumbles $s own weapon.",
        FALSE, ch, 0, victim, TO_NOTVICT);
    act("In a vain attempt, $n tries to disarm you, but instead, fumbles $s weapon.",
       FALSE, ch, 0, victim, TO_VICT);
    act("You fail miserably in your attempt to disarm $N.", FALSE, ch, 0,
        victim, TO_CHAR);
    obj_to_char(unequip_char(ch, WIELD), ch);
    strip_holy_sword( ch );
    set_short_affected_by(ch, SKILL_DISARM, 3 * PULSE_VIOLENCE);
  }
  else if (rnd_num >= 60)
  {
    act("$n failingly attempts a complex disarming technique on $N.", FALSE,
        ch, 0, victim, TO_NOTVICT);
    act("$e begins to fumble $s own weapon, loosing complete control over it.",
       FALSE, ch, 0, 0, TO_NOTVICT);
    act("$n starts to fumble $s weapon in a vain attempt to disarm you.",
        FALSE, ch, 0, victim, TO_VICT);
    act("You make a grave error in judgement, and lose control of your weapon.",
       FALSE, ch, 0, 0, TO_CHAR);
    notch_skill(ch, SKILL_DISARM, 7);
  }
  else
  {
    act("$n tries to disarm $N, but $E neutralizes $s attempt.", FALSE, ch, 0,
        victim, TO_NOTVICT);
    act("You neutralize $n's attempt to disarm you!", FALSE, ch, 0, victim,
        TO_VICT);
    act("Your disarming maneuver had no effect on $N.", FALSE, ch, 0, victim,
        TO_CHAR);
  }
  // Engage victim regardless of outcome.
  if( !IS_FIGHTING(victim) )
  {
    set_fighting(victim, ch);
  }
  CharWait(ch, PULSE_VIOLENCE * 2);
}

void event_meditation(P_char ch, P_char victim, P_obj obj, void *data)
{
  if (GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION)/2 > number(0,100)) {
    if (IS_AFFECTED(ch, AFF_BLIND)) {
      spell_cure_blind(50, ch, 0, 0, ch, 0);
      notch_skill(ch, SKILL_ADVANCED_MEDITATION, 2);
      return;
    }
    if (GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION) > 50 && !number(0,2) &&
        IS_AFFECTED2(ch, AFF2_POISONED)) {
      poison_common_remove(ch);
      notch_skill(ch, SKILL_ADVANCED_MEDITATION, 2);
      send_to_char("You were able to fight the poison in your body!\n", ch);
      return;
    }
    if (GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION) > 90 &&
        (affected_by_spell(ch, SPELL_DISEASE) || affected_by_spell(ch, SPELL_PLAGUE)))
    {
      spell_cure_disease (GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, NULL); 
      notch_skill(ch, SKILL_ADVANCED_MEDITATION, 2);
      return;
    }  
  }

  add_event(event_meditation, get_property("timer.sec.advancedMeditation", 3) * WAIT_SEC,
      ch, 0, 0, 0, 0, 0);
}

void stop_meditation(P_char ch)
{
  affect_from_char(ch, SKILL_MEDITATE);
  REMOVE_BIT(ch->specials.affected_by, AFF_MEDITATE);
  disarm_char_nevents(ch, event_meditation);
}

void do_meditate(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if (IS_NPC(ch) || GET_CHAR_SKILL(ch, SKILL_MEDITATE) == 0)
  {
    send_to_char("You don't know how to meditate.\r\n", ch);
    return;
  }
  if (GET_POS(ch) != POS_SITTING)
  {
    send_to_char("You must sit down before meditating!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_MEDITATE))
  {
    return;
  }

  notch_skill(ch, SKILL_MEDITATE, (int) get_property("skill.notch.meditate", 1));

  if (GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION))
  {
    memset(&af, 0, sizeof(af));
    af.type = SKILL_MEDITATE;
    af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
    af.bitvector = AFF_MEDITATE;
    af.location = APPLY_HIT_REG;
    af.modifier = 25 + GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION)/2;
    af.duration = -1;
    affect_to_char(ch, &af);
    if (GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION) > 50)
    {
      af.location = APPLY_MOVE_REG;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION) - 50) / 10;
      affect_to_char(ch, &af);
      send_to_char("You feel completely aligned with your spirit as you sink in meditation...\n", ch);
      CharWait(ch, PULSE_VIOLENCE/2);
      StartRegen(ch, EVENT_MOVE_REGEN);
    }
    else
    {
      send_to_char("You sink in meditation...\n", ch);
      CharWait(ch, PULSE_VIOLENCE);
    }
    StartRegen(ch, EVENT_HIT_REGEN);
    add_event(event_meditation, get_property("timer.sec.advancedMeditation", 3) * WAIT_SEC, ch, 0, 0, 0, 0, 0);
  }
  else
  {
    send_to_char("You start meditating...\n", ch);
    CharWait(ch, PULSE_VIOLENCE);
  }
  SET_BIT(ch->specials.affected_by, AFF_MEDITATE);
  StartRegen(ch, EVENT_MANA_REGEN);
}

/*
 * Druid's shape change skill. --TAM 04/09/94
 */

P_char morph(P_char ch, int rnum, int mode)
{
  P_char   mob;
  snoop_by_data *snoop_by_ptr;
  int      is_avatar = FALSE, virt;

  if (!ch)
  {
    logit(LOG_EXIT, "morph: Bogus ch passed");
    raise(SIGSEGV);
  }
  if (IS_NPC(ch) || IS_MORPH(ch))
  {
    return NULL;
  }
  if (mode == REAL)
    mob = read_mobile(rnum, REAL);
  else
    mob = read_mobile(rnum, VIRTUAL);

  virt = mob_index[GET_RNUM(mob)].virtual_number;
  if (virt == EVIL_AVATAR_MOB || virt == GOOD_AVATAR_MOB)
    is_avatar = TRUE;

  if (!mob)
  {
    logit(LOG_EXIT, "morph: Unable to load mob %d", rnum);
    raise(SIGSEGV);
  }
  while (mob->affected)
    affect_remove(mob, mob->affected);

  clearMemory(mob);

  /*
   * "fix" their npcact flags so we don't get any "unwanted"
   * behaivors... These are the only allowable NPC acts
   */
  mob->specials.act &= ACT_ISNPC | ACT_NICE_THIEF |     /*NPC_OUTLAW | */
    ACT_CANSWIM | ACT_CANFLY | ACT_BREAK_CHARM | ACT_MOUNT;

  /*
   * and force this one...
   */
  mob->specials.act |= ACT_NICE_THIEF;

  GET_PLATINUM(mob) = 0;
  GET_GOLD(mob) = 0;
  GET_SILVER(mob) = 0;
  GET_COPPER(mob) = 0;

  /* TWEAK THIS
   * Settings hps same as player makes it useless for creating tanks
   * But high level druids will be able to make tanks with 4000+ hps atm
   * maybe hps /= 2, 3 or 4 would be better
   */
#if 1
  /*
   * the new forms BASE hitpoints will be the same as the player.  Note
   * that con mobs, eq, etc, will made the new hps look different
   */
  if (!is_avatar)
  {
    mob->points.base_armor = 0 - (GET_LEVEL(mob) * 4);
    mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 4;
    mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 5;
    MonkSetSpecialDie(mob);

    /* let's set moblevel to charlevel down here to make damage, hitroll,
       armor based on mob level */

    mob->player.level = MAX(11, GET_LEVEL(mob));
    mob->points.base_hit /= 2;

    mob->player.m_class = CLASS_WARRIOR;

    /* if the mob dies as a result of its affects, we're screwed */
  }
#endif

  if (affect_total(mob, TRUE))
    return NULL;

  /* no division by zero here! */
  if (GET_HIT(ch) < 1)
    GET_HIT(ch) = 1;
  if (GET_MAX_HIT(ch) < 1)
    GET_MAX_HIT(ch) = 1;
  if (GET_VITALITY(ch) < 1)
    GET_VITALITY(ch) = 1;
  if (GET_MAX_VITALITY(ch) < 1)
    GET_MAX_VITALITY(ch) = 1;

  if (!is_avatar)
  {
    /* set their hitpoints PROPORTIONALLY to what they had.. */
    GET_HIT(mob) = ((GET_HIT(ch) + 1) * 100 / GET_MAX_HIT(ch)) *
      GET_MAX_HIT(mob) / 100;

    /* set their moves the same way */
    GET_VITALITY(mob) = ((GET_VITALITY(ch) * 100) / GET_MAX_VITALITY(ch)) *
      GET_MAX_VITALITY(mob) / 100;
  }
  mob->only.npc->default_pos = POS_STANDING + STAT_NORMAL;
  ClearCharEvents(mob);

  if (ch->desc)
  {
    ch->desc->character = mob;
    ch->desc->original = ch;
    mob->desc = ch->desc;
    ch->desc = NULL;
    /*    if (mob->desc->snoop.snoop_by)
          mob->desc->snoop.snoop_by->desc->snoop.snooping = mob;
          */
    snoop_by_ptr = mob->desc->snoop.snoop_by_list;
    while (snoop_by_ptr)
    {
      snoop_by_ptr->snoop_by->desc->snoop.snooping = mob;

      snoop_by_ptr = snoop_by_ptr->next;
    }
  }
  ch->only.pc->switched = mob;
  SET_BIT(ch->specials.act, PLR_MORPH);
#if 0
  mob->only.npc->memory = ch;   /*
                                 * hackish way to keep track of who is the
                                 * original player if they drop link
                                 */
#else

  /* too hackish for me.  let's make a new var and put orig char in
     there instead */

  mob->only.npc->orig_char = ch;
#endif
  /*
   * Void is probably a safe place to PC for time being... assuming no
   * idiot gods don't fuck off in there
   *  Clav: Update, there is fucking around in the void, going to limbo(1)
   */
  if (!is_avatar)
  {
    char_from_room(ch);
    char_to_room(ch, 1, 0);
    char_to_room(mob, real_room(ch->specials.was_in_room), 0);
  }
  else
    char_to_room(mob, ch->in_room, 0);

  /*
   * force a save at this point, simply because its a smart thing
   */
  writeCharacter(ch, 1, NOWHERE);

  justice_witness(ch, NULL, CRIME_SHAPE_CHANGE);

  return mob;
}

P_char un_morph(P_char mob)
{
  P_char   ch;
  int      in_rm, is_avatar = FALSE, virt;
  snoop_by_data *snoop_by_ptr, *next;

  if (!mob || IS_PC(mob) || !IS_MORPH(mob))
  {
    logit(LOG_EXIT, "un_morph: Bogus morph passed");
    raise(SIGSEGV);
  }

  virt = mob_index[GET_RNUM(mob)].virtual_number;
  if (virt == EVIL_AVATAR_MOB || virt == GOOD_AVATAR_MOB)
    is_avatar = TRUE;

#if 1
  ch = mob->only.npc->orig_char;
#else
  ch = (P_char) mob->only.npc->memory;
#endif
  /* the "owning" player. We use this instead of desc->original just in
   * case they are linkless */
  if (!ch)
  {
    logit(LOG_EXIT, "un_morph: redundant orig pointer is null. Argh!");
    raise(SIGSEGV);
  }
  if (mob->desc)
  {

    /*
     * sanity check
     */
    if (ch != mob->desc->original)
    {
      logit(LOG_EXIT, "un_morph: redundant original pointers don't match!");
      raise(SIGSEGV);
    }
    /*
     * move a snoop from the morph to the owning player
     */

    /*
       if (mob->desc->snoop.snoop_by)
       mob->desc->snoop.snoop_by->desc->snoop.snooping = mob->desc->original;
       */
    snoop_by_ptr = mob->desc->snoop.snoop_by_list;
    while (snoop_by_ptr)
    {
      //      snoop_by_ptr->snoop_by->desc->snoop.snooping = mob->desc->original;
      if (is_avatar)
        send_to_char
          ("&+RYour diety has returned from whence it came, you can no longer sight link with it.&n\r\n",
           snoop_by_ptr->snoop_by);
      snoop_by_ptr->snoop_by->desc->snoop.snooping = 0;
      snoop_by_ptr = snoop_by_ptr->next;
    }
    snoop_by_ptr = mob->desc->snoop.snoop_by_list;
    while (snoop_by_ptr)
    {
      next = snoop_by_ptr->next;
      FREE(snoop_by_ptr);
      snoop_by_ptr = next;
    }
    mob->desc->snoop.snoop_by_list = 0;



    if (mob->in_room == 1 &&
        world[mob->specials.was_in_room].number != NOWHERE)
    {
      in_rm = real_room(mob->specials.was_in_room);
      send_to_char("Pulling your shapechanged form out of Void.\r\n", mob);
    }
    else if (mob->in_room == 1 &&
        world[mob->specials.was_in_room].number == NOWHERE)
    {
      in_rm = real_room(ch->player.hometown);
      send_to_char
        ("I don't know where to put you, besides in your hometown.\r\n", mob);
      send_to_char("Please tell a god about this.\r\n", mob);
    }
    else
      in_rm = mob->in_room;

    /*
     * desc points at the original char...
     */
    mob->desc->character = ch;
    /*
     * and the character gets his/her desc back
     */
    ch->desc = mob->desc;
    mob->desc = NULL;
    /*
     * clean up the desc...
     */
    ch->desc->original = 0;
  }
  else
    in_rm = mob->in_room;

  /*
   * cleanup the morph related stuff on the original char
   */
  ch->only.pc->switched = NULL;
  REMOVE_BIT(ch->specials.act, PLR_MORPH);

  clearMemory(mob);
  mob->only.npc->orig_char = NULL;

  /*
   * set their hitpoints PROPORTIONALLY to what they had..
   */

  if (GET_HIT(mob) < 1)
    GET_HIT(mob) = 1;
  if (GET_VITALITY(mob) < 1)
    GET_VITALITY(mob) = 1;

  if (!is_avatar)
  {
    GET_HIT(ch) = ((GET_HIT(mob) + 1) * 100 / GET_MAX_HIT(mob)) *
      GET_MAX_HIT(ch) / 100 + 1;

    /*
     * set their moves the same way
     */
    GET_VITALITY(ch) = ((GET_VITALITY(mob) * 100) / GET_MAX_VITALITY(mob)) *
      GET_MAX_VITALITY(ch) / 100 + 1;
  }
  /*
   * get rid of the morph
   */
  extract_char(mob);

  /*
   * move the char back to their room
   */
  //  if(!is_avatar) {
  char_from_room(ch);
  char_to_room(ch, in_rm, -1);
  //  }                           /* shouldn't trigger agg
  //                               */
  do_save_silent(ch, 1);

  justice_witness(ch, NULL, CRIME_SHAPE_CHANGE);

  return ch;
}

struct char_shapechange_data *shapechange_getShape(P_char ch, int shapeNum)
{

  int      i;

  struct char_shapechange_data *shape;

  shape = ch->only.pc->knownShapes;
  i = 1;
  while ((shape != NULL) && (i < shapeNum))
  {
    shape = shape->next;
    i++;
  }
  if (shape == NULL)
  {
    logit(LOG_DEBUG, "%s shapechange event with invalid shapeNum %d",
        GET_NAME(ch), shapeNum);
    return NULL;
  }

  return shape;

}

bool shapechange_canShapechange(P_char ch)
{
  if (IS_RIDING(ch))
  {
    send_to_char("While mounted?  I don't think so...\r\n", ch);
    return FALSE;
  }
  if (get_linking_char(ch, LNK_RIDING))
  {
    send_to_char("You can't change your form when you have a rider!\r\n", ch);
    return FALSE;
  }
  if (IS_ROOM( ch->in_room, ROOM_NO_MOB) ||
      IS_ROOM( ch->in_room, ROOM_SAFE))
  {
    send_to_char("Something in the air prevents yer magics!\r\n", ch);
    return FALSE;
  }
  if (!has_innate(ch, INNATE_SHAPECHANGE) && !IS_TRUSTED(ch) && !IS_MORPH(ch))
  {
    send_to_char("You don't know how to change your form!\r\n", ch);
    return FALSE;
  }

  /*  if ((GET_LEVEL(ch) < 11) && !IS_MORPH(ch)) //Instead of this check, this innate is just gived at lvl 11
      {
      send_to_char("You aren't experienced enough yet!\r\n", ch);
      return FALSE;
      }*/

  /* assume form changed into will never have snooping capabilities */
  if (ch->desc->snoop.snooping)
  {
    send_to_char("No snooping AND shapechanging at any one time.\r\n", ch);
    return FALSE;
  }
  if (GET_OPPONENT(ch))
  {
    send_to_char
      ("You can't muster the mental energy to do that right now.\r\n", ch);
    return FALSE;
  }

  if (IS_DISGUISE(ch))
  {
    send_to_char("You must shift back into your own shape first!\r\n", ch);
    return FALSE;
  } 

  return TRUE;
}

#if 0
void shapechange_return(P_char ch)
{
  if (!ch->desc->original)
  {
    if (GET_CLASS(ch, CLASS_DRUID))
      send_to_char("But you're already in your natural form.\r\n", ch);
    else
      send_to_char("You don't know how!\r\n", ch);
    return;
  }
  /*
   * can't change back when fighting a PC
   */
  if (GET_OPPONENT(ch))
  {
    send_to_char("You can't concentrate on that right now.\r\n", ch);
    return;
  }
  if (!IS_MORPH(ch))
  {
    send_to_char("You need to switch back using 'return' command.\r\n", ch);
    return;
  }
  if (IS_AFFECTED4(ch, AFF4_NO_UNMORPH))
  {
    send_to_char
      ("Some mysterious force prevents you from returning to your normal form.\r\n",
       ch);
    return;
  }
  send_to_char("You return to your original body.\r\n", ch);
  act("$n &nbegins to writhe and twist, re-shaping into $N.",
      FALSE, ch, 0, ch->desc->original, TO_ROOM);

  un_morph(ch);
}

bool shapechange_godShapechange(P_char ch, int mob_num)
{
  if (!mob_num)
  {
    send_to_char("What vnum?\r\n", ch);
    return FALSE;
  }
  if ((mob_num = real_mobile0(mob_num)) == 0)
  {
    send_to_char("Invalid vnum\r\n", ch);
    return FALSE;
  }
  if (mob_index[mob_num].func.mob)
  {
    send_to_char("Can't change into mobs with special procs!\r\n", ch);
    return FALSE;
  }

  return TRUE;
}

void shapechange_changeTo(P_char ch, int mob_num)
{

  send_to_char
    ("You feel yourself metamorphing into your desired new form.\r\n", ch);
  if (ch->only.pc->switched)
  {
    return;
  }

  if (morph(ch, mob_num, VIRTUAL))
  {
    act("$N's body begins to re-form into $n.", FALSE,
        ch->only.pc->switched, 0, ch, TO_ROOM);
  }
  else
  {
    act("$N's body begins to re-form, then suddenly stops.", FALSE,
        ch, 0, 0, TO_ROOM);
    send_to_char("You suddenly stop reforming!\r\n", ch);
  }
}

#define TIME_BETWEEN_SHAPECHANGES SECS_PER_MUD_DAY

void shapechange_showShape(struct char_shapechange_data *curShape,
    int listNum, char *buf)
{
  char     buf2[MAX_STRING_LENGTH];
  P_char   studiedMob;

  studiedMob = read_mobile(real_mobile0(curShape->mobVnum), REAL);
  if (studiedMob == NULL)
  {
    logit(LOG_DEBUG, "Unable to read mob in shapechange_showShape");
    return;
  }

  snprintf(buf2, MAX_STRING_LENGTH, "&+g[%d] &+mStudied:%3d - &n%s&n",
      listNum++,
      curShape->timesResearched, studiedMob->player.short_descr);

  strcat(buf, buf2);

  if (curShape->lastShapechanged + TIME_BETWEEN_SHAPECHANGES > time(0))
  {
    snprintf(buf2, MAX_STRING_LENGTH, " (%d hours rest required)",
        (curShape->lastShapechanged + TIME_BETWEEN_SHAPECHANGES - time(0))
        / SECS_PER_MUD_HOUR);
    strcat(buf, buf2);
  }
  strcat(buf, "\r\n");

  curShape = curShape->next;

  extract_char(studiedMob);

}

bool shapechange_mobExists(int vnum)
{
  return (real_mobile0(vnum) != 0);
}

struct char_shapechange_data *shapechange_removeShape(P_char ch,
    struct
    char_shapechange_data
    *curShape)
{

  struct char_shapechange_data *lastShape;

  if (ch->only.pc->knownShapes == NULL)
  {
    logit(LOG_DEBUG, "Removing a shape, but there are none.");
    return NULL;
  }

  if (curShape == ch->only.pc->knownShapes)
  {
    ch->only.pc->knownShapes = ch->only.pc->knownShapes->next;
    FREE(curShape);
    return ch->only.pc->knownShapes;
  }
  else
  {

    lastShape = ch->only.pc->knownShapes;
    while ((lastShape->next != NULL) && (lastShape->next != curShape))
      lastShape = lastShape->next;
    if (lastShape->next == NULL)
    {
      logit(LOG_DEBUG,
          "Unable to find shape to remove in shapechange_removeShape");
      return NULL;
    }
    lastShape->next = curShape->next;
    FREE(curShape);
    return lastShape->next;
  }
}

void shapechange_listKnownShapes(P_char ch)
{
  char     buf[MAX_STRING_LENGTH];

  if (ch->only.pc->knownShapes == NULL)
  {
    strcpy(buf, "You have not yet researched any animals.\r\n");
    strcat(buf,
        "Try approaching some critter and typing \"shapechange <critter>\".\r\n");
  }
  else
  {
    int      listNum = 1;
    struct char_shapechange_data *curShape;

    strcpy(buf, "You have researched the following animals:\r\n");
    curShape = ch->only.pc->knownShapes;
    while (curShape != NULL)
    {
      if (shapechange_mobExists(curShape->mobVnum))
      {
        shapechange_showShape(curShape, listNum++, buf);
        send_to_char(buf, ch);
        buf[0] = '\0';
        curShape = curShape->next;
      }
      else
        curShape = shapechange_removeShape(ch, curShape);
    }
  }

  send_to_char(buf, ch);

}
#endif // 0
int shapechange_levelNeeded(int race)
{
  switch (race)
  {
    case RACE_PRIMATE:
      return 10;
    case RACE_ANIMAL:
    case RACE_HERBIVORE:
    case RACE_CARNIVORE:
      return 15;
    case RACE_ARACHNID:
    case RACE_REPTILE:
    case RACE_SNAKE:
      return 20;
    case RACE_AQUATIC_ANIMAL:
      return 25;
    case RACE_QUADRUPED:
      return 30;
    case RACE_PLANT:
    case RACE_FLYING_ANIMAL:
      return 40;
    case RACE_PWORM:
    case RACE_INSECT:
    case RACE_SLIME:
    case RACE_PARASITE:
      return 51;
      /*  case RACE_F_ELEMENTAL:
          case RACE_A_ELEMENTAL:
          case RACE_W_ELEMENTAL:
          case RACE_E_ELEMENTAL:
          return 51;
          case RACE_DRAGON:*/
    default:
      return 57;
  }
}

void event_learn_shape(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type af, *afp;
  int vnum;

  if (ch->in_room != victim->in_room) {
    send_to_char("Uh oh, where did your subject go?\n", ch);
    return;
  }

  vnum = mob_index[GET_RNUM(victim)].virtual_number;

  for (afp = ch->affected; afp; afp = afp->next)
    if (afp->type == TAG_KNOWN_SHAPE && afp->modifier == vnum) {
      send_to_char("You refresh the knowledge of this creature.\n", ch);
      afp->duration = get_property("innate.shapechange.memory.time", 500);
      return;
    }

  memset(&af, 0, sizeof(af));
  af.type = TAG_KNOWN_SHAPE;
  af.modifier = mob_index[GET_RNUM(victim)].virtual_number;
  af.flags = AFFTYPE_PERM | AFFTYPE_NOSHOW | AFFTYPE_NOAPPLY | AFFTYPE_NODISPEL;
  af.duration = get_property("innate.shapechange.memory.time", 500);

  affect_to_char(ch, &af);
  act("Sinking deeper in meditation you experience contact with\n"
      "eternal wisdom of mother nature as it flows into you.\n"
      "&+WYou are now able to take on shape of &n$N.",
      FALSE, ch, 0, victim, TO_CHAR);
}

void shapechange_learn(P_char ch, char *mobname)
{
  P_char mob;

  if (!(mob = get_char_room_vis(ch, mobname))) {
    send_to_char("You can't see anything here by this name.\n", ch);
    return;
  }

  if (GET_ALT_SIZE(ch) != GET_ALT_SIZE(mob)) {
    send_to_char("This creature does not have appropriate size.\n", ch);
    return;
  }

  if (!is_natural_creature(mob)) {
    send_to_char("You can only take on shape of a natural creature.\n", ch);
    return;
  }

  int chLevel = GET_LEVEL(ch);
  // multi-classes have a serious penalty to what shapes they can take
  // while forest specs get a slight bonus
  // non druids receive a sall penalty
  if (IS_MULTICLASS_PC(ch))
    chLevel -= 10;

  if (!GET_CLASS(ch, CLASS_DRUID))
    chLevel -= 5;

  if (GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND)) 
    chLevel++;

  if ((chLevel < shapechange_levelNeeded(GET_RACE(mob))) && !IS_TRUSTED(ch)) {
    send_to_char("You are not experienced enough to change your shape into this creature, yet.\n", ch);
    return;
  }

  if (mob->only.npc->str_mask & (STRUNG_KEYS | STRUNG_DESC2 | STRUNG_DESC1)) {
    send_to_char("Something about this creature makes it impossible "
        "for you to study it closer.\n", ch);
    return;
  }

  act("You come over to $N and touch it. As it calms down\n"
      "you begin to sense its spirit and slowly align with it.",
      FALSE, ch, 0, mob, TO_CHAR);

  add_event(event_learn_shape, 3 * PULSE_VIOLENCE, ch, mob, 0, 0, 0, 0);
}

void do_shapechange(P_char ch, char *arg, int cmd)
{
  char     mobname[MAX_INPUT_LENGTH], *rest, buf[256];
  struct affected_type *af;
  P_char   mob;
  int count;

  if (!ch->desc)
    return;

  if (!has_innate(ch, INNATE_SHAPECHANGE) && !IS_TRUSTED(ch) && !IS_MORPH(ch))
  {
    send_to_char("You don't know how to change your form!\r\n", ch);
    return;
  }

  rest = one_argument(arg, mobname);

  if (!strcmp(mobname, "learn")) {
    shapechange_learn(ch, rest);
    return;
  }

  if (!str_cmp(mobname, "me"))
  {
    if (IS_DISGUISE_SHAPE(ch))
    {
      send_to_char("&+WYou shift back into your own shape.&N\r\n", ch);
      act("&+W$n shifts back into $s own image.&N", FALSE,
          ch, 0, ch, TO_ROOM);
      justice_witness(ch, NULL, CRIME_DISGUISE);
      CharWait(ch, PULSE_VIOLENCE * 3);
      remove_disguise(ch, FALSE);
    }
    return;
  }


  if (!mobname[0] || !*arg) {
    char *how_learned[5] = {
      "about to forget", "vaguely", "average", "good", "mastered"};
    send_to_char("You have studied the following creatures:\n", ch);
    for (af = ch->affected, count = 1; af; af = af->next)
      if (af->type == TAG_KNOWN_SHAPE)
      {
        if( real_mobile(af->modifier) < 0 )
          snprintf(buf, 256, "[%d] %s &n(&+W%s&n)\n", count++, "Unknown",
              how_learned[BOUNDED(0, (5 * af->duration - 1)/get_property("innate.shapechange.memory.time", 500), 4)]);
        else
          snprintf(buf, 256, "[%d] %s &n(&+W%s&n)\n", count++, mob_index[real_mobile(af->modifier)].desc2,
              how_learned[BOUNDED(0, (5 * af->duration - 1)/get_property("innate.shapechange.memory.time", 500), 4)]);
        send_to_char(buf, ch);
      }
    return;
  }

  if (!shapechange_canShapechange(ch))
    return;

  if ((count = atoi(mobname)) <= 0) {
    send_to_char("Please specify a number of the creature.\n", ch);
    return;
  }

  for (af = ch->affected; af; af = af->next) {
    if (af->type == TAG_KNOWN_SHAPE && --count == 0)
      break;
  }

  if (!af) {
    send_to_char("You don't know that many shapes.\n", ch);
    return;
  }

  if (!(mob = read_mobile(af->modifier, VIRTUAL)))
  {
    send_to_char("You cannot take on shape of this creature.\n", ch);
    return;
  }

  /*  if (GET_SIZE(mob) != GET_SIZE(ch))  // This check is already performed at learning stage
      send_to_char("You are not of appropriate size to take on shape of this creature now.\n", ch);
      goto cleanup;
      }*/

stop_riding(ch);
stop_riding(get_linking_char(ch, LNK_RIDING));

CharWait(ch, PULSE_VIOLENCE * 2);
if (IS_DISGUISE(ch))
  remove_disguise(ch, FALSE);
  IS_DISGUISE_PC(ch) = FALSE;
  IS_DISGUISE_NPC(ch) = TRUE;
  IS_DISGUISE_ILLUSION(ch) = FALSE;
  IS_DISGUISE_SHAPE(ch) = TRUE;
  if( GET_CLASS( ch, CLASS_BLIGHTER ) )
  {
    snprintf(mobname, MAX_STRING_LENGTH, "skeleton %s", GET_NAME(mob) );
    ch->disguise.title = str_dup(mobname);
    snprintf(mobname, MAX_STRING_LENGTH, "a skeleton of %s&n", mob->player.short_descr );
    ch->disguise.name = str_dup(mobname);
    snprintf(mobname, MAX_STRING_LENGTH, "A skeleton of %s&n stands here.", mob->player.short_descr );
    ch->disguise.longname = str_dup(mobname);
    ch->disguise.race = RACE_SKELETON;
  }
  else
  {
    ch->disguise.title = str_dup(GET_NAME(mob));
    ch->disguise.name = str_dup(mob->player.short_descr);
    ch->disguise.longname = str_dup(mob->player.long_descr);
    ch->disguise.race = GET_RACE(mob);
  }
  ch->disguise.m_class = mob->player.m_class;
  ch->disguise.racewar = GET_RACEWAR(mob);
  ch->disguise.hit = GET_LEVEL(ch) * 2;
  snprintf(mobname, MAX_STRING_LENGTH, "&+WYou shift into the form of %s!\r\n", ch->disguise.name );
  send_to_char(mobname, ch);
  snprintf(mobname, MAX_STRING_LENGTH, "&+WThe image of %s &Nshifts&+W into the form of %s!\r\n",
    GET_NAME(ch), ch->disguise.name );
  act(mobname, FALSE, ch, 0, NULL, TO_ROOM);
  SET_BIT(ch->specials.act, PLR_NOWHO);
  justice_witness(ch, NULL, CRIME_DISGUISE);

  balance_affects(ch);

cleanup:
  extract_char(mob);

}

void do_dirttoss(P_char ch, char *arg, int cmd)
{
  P_char   vict = NULL;
  int      skl_lvl = 0, sect;
  int      i = 0;

  if (!ch)
    return;

  sect = world[ch->in_room].sector_type;

  if (!HAS_FOOTING(ch) ||
      ((sect == SECT_UNDERWATER) ||
       (sect == SECT_FIREPLANE) ||
       ((sect >= SECT_AIR_PLANE) && (sect <= SECT_ASTRAL))))
  {
    send_to_char("This isn't the place to be grabbing at the ground.\r\n",
        ch);
    return;
  }
  if (ch->group && !on_front_line(ch))
  {
    send_to_char("You couldn't hit them from here!\r\n", ch);
    return;
  }

  if (!IS_FIGHTING(ch))
  {
    vict = ParseTarget(ch, arg);
    if (!vict)
    {
      send_to_char("Toss it at who?\r\n", ch);
      return;
    }
  }
  else
  {
    vict = GET_OPPONENT(ch);
    if (!vict)
    {
      stop_fighting(ch);
      return;
    }
  }

  skl_lvl = (int) (GET_CHAR_SKILL(ch, SKILL_DIRTTOSS));

  if (skl_lvl <= 0)
    return;

  appear(ch);

  if (has_innate(vict, INNATE_EYELESS))
  {
    send_to_char("How are you going to blind the eyeless?\r\n", ch);
    return;
  }

  if (!on_front_line(ch) || !on_front_line(vict))
  {
    send_to_char("You can't reach them!\r\n", ch);
    return;
  }

  if (!skl_lvl)
  {
    act("You toss a clump of dirt at $N.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tosses a bit of dirt at you.", TRUE, ch, 0, vict, TO_VICT);
    act("$n tosses a bit of dirt at $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    return;
  }
  if (!CanDoFightMove(ch, vict))
    return;

  if (affected_by_spell(ch, SKILL_BASH))
  {
    send_to_char
      ("You're kinda busy defending yourself. Try again when you have a free second.\r\n",
       ch);
    return;
  }

  justice_witness(ch, vict, CRIME_ATT_MURDER);

  i = skl_lvl - (GET_C_AGI(vict) / 6);

  act
    ("You reach for the ground, quickly tossing a clump of dirt at $N's face!",
     FALSE, ch, 0, vict, TO_CHAR);
  act
    ("$n reaches for the ground, quickly tossing a clump of dirt at your face!",
     TRUE, ch, 0, vict, TO_VICT);
  act
    ("$n reaches for the ground, quickly tossing a clump of dirt at $N's face!",
     TRUE, ch, 0, vict, TO_NOTVICT);

  if (number(1, 100) < i)
    blind(ch, vict, 6 * PULSE_VIOLENCE);

  notch_skill(ch, SKILL_DIRTTOSS, get_property("skill.notch.offensive", 7));
  CharWait(ch, PULSE_VIOLENCE * 2);

  if (IS_NPC(vict) && CAN_SEE(vict, ch))
  {
    remember(vict, ch);
    if (!IS_FIGHTING(vict) && !IS_DESTROYING(vict))
      MobStartFight(vict, ch);
  }
  return;
}

char *get_str_zone( P_obj obj )
{
  static char buffer[200];
  int zone = 0;

  while( zone < top_of_zone_table
      && world[zone_table[zone].real_bottom].number <= obj_index[obj->R_num].virtual_number )
    zone++;

  return zone_table[zone-1].name;
}

void do_lore(P_char ch, char *arg, int cmd)
{
  int      i, percent = 0, skl_lvl;
  bool     found;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH], Gbuf3[256];
  char     name[MAX_STRING_LENGTH];
  P_obj    obj = NULL;
  P_char   target = NULL;
  bool     is_in_room = FALSE;
  //---------------------------------------------
  // 1-only inv obj, 2-players, 3-something nice
  int 	   lore_power = 1;

  if ( GET_SPEC(ch, CLASS_BARD, SPEC_MINSTREL) )
    lore_power = 3;
  else if (GET_CLASS(ch, CLASS_BARD))
    lore_power = 2;
  //---------------------------------------------

  if (!ch)
    return;

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("You can't concentrate on that right now.\r\n", ch);
    return;
  }
  if (!*arg)
  {
    send_to_char("Who or what do you want to know about?\r\n", ch);
    return;
  }
  one_argument(arg, name);

  // always check for item in inventory first
  if (!(obj = get_obj_in_list_vis(ch, name, ch->carrying)))
  {
    // ok no object in inv, now check for players
    if ( lore_power < 2 )
    {
      send_to_char("You can't recall any legends or stories ever told about that!\r\n", ch);
      return;
    }

    //---------------------------------------
    // lore char in room
    P_char   tmp_char;
    P_obj    tmp_object;
    if ( generic_find(name, FIND_CHAR_ROOM, ch, &tmp_char, &tmp_object) && tmp_char )
    {
      snprintf(name, MAX_STRING_LENGTH,"%s",GET_NAME(tmp_char));
      act("With but a quick glance, you are suddenly aware of the exploits of $N's life, past and present.",
          TRUE, ch, 0, tmp_char, TO_CHAR);
      act("$n nonchalantly glances at $N, his features taking a stern look of concentration.",
          TRUE, ch, 0, tmp_char, TO_NOTVICT);
      act("$n gives you a quick glance, taking in your traits and features.",
          TRUE, ch, 0, tmp_char, TO_VICT);
      is_in_room = TRUE;
    }
    //---------------------------------------

    target = (struct char_data *) mm_get(dead_mob_pool);
    target->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

    if ( (restoreCharOnly(target, name) < 0) || !target )
    {
      if (target)
      {
        free_char(target);
        target = NULL;
      }
      send_to_char("You can't recall any legends or stories ever told about that!\r\n", ch);
      return;
    }
  }


  skl_lvl = GET_CHAR_SKILL(ch, SKILL_LORE);
  if (IS_TRUSTED(ch) || cmd == 999)
    skl_lvl = 200;

  percent = number(1, 101);

  if( obj )
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "This item is from the zone: %s.\n", get_str_zone(obj) );
    send_to_char( Gbuf1, ch );
  }

  if (percent > skl_lvl)
  {
    notch_skill(ch, SKILL_LORE, 3);
    if( obj )
      send_to_char("That's all you can recall about this item.\r\n", ch);
    else
      send_to_char("You can't recall any legends or stories ever told about that!\r\n", ch);
    CharWait(ch, 1);
    return;
  }

  if (target)
  {
    if (GET_LEVEL(target) >= AVATAR)
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You know that %s %s is a God of Duris\r\n",
          GET_NAME(target),
          GET_TITLE(target) == NULL ? "" : GET_TITLE(target));
      send_to_char(Gbuf1, ch);
    }
    else if ( (GET_LEVEL(target) >= 50 && lore_power > 1) || // regular bards know only famous persons
        (GET_LEVEL(target) >= 40 && lore_power > 2)    // masters knows more
        )
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "Using your abilities to cast your knowledge far into the realm,\nyou glean that it is %s (%s) %s %s\n",
          GET_NAME(target),
          race_names_table[(int) GET_RACE(target)].ansi,
          get_class_string(target, Gbuf3),
          GET_TITLE(target) == NULL ? "" : GET_TITLE(target)
          );
      send_to_char(Gbuf1, ch);
    }
    else
    {
      send_to_char("You haven't heard anything about that person!\n", ch);
    }
    if (target)
      free_char(target);
    CharWait(ch, 3);
    return;
  }
  if (obj)
  {
    lore_item( ch, obj );
    CharWait(ch, 3);
  }
}

void lore_item( P_char ch, P_obj obj )
{
  char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH], Gbuf3[256];
  int percent, i;
  bool found;

  if( !ch || !obj )
    return;

  if (IS_SET(obj->extra_flags, ITEM_NOIDENTIFY))
  {
    if (GET_LEVEL(ch) < 50)
    {
      send_to_char("You can't recall any legends or stories ever told about this item.\n\r", ch);
      return;
    }
  }
  snprintf(Gbuf1, MAX_STRING_LENGTH, "'%s'\r\nWeight %d, Item type: ", obj->short_description, GET_OBJ_WEIGHT(obj));
  sprinttype(GET_ITEM_TYPE(obj), item_types, Gbuf2);
  strcat(Gbuf1, Gbuf2);
  strcat(Gbuf1, "\r\n");
  send_to_char(Gbuf1, ch);

  if( obj->bitvector  ||
      obj->bitvector2 ||
      obj->bitvector3 ||
      obj->bitvector4 ||
      obj->bitvector5 )
  {
    *Gbuf2 = '\0';

    send_to_char("Item will give you following abilities:  ", ch);

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

  send_to_char("Item is: ", ch);
  sprintbitde(obj->extra_flags, extra_bits, Gbuf1);
  strcat(Gbuf1, "\r\n");
  send_to_char(Gbuf1, ch);

  switch (GET_ITEM_TYPE(obj))
  {

    case ITEM_SCROLL:
    case ITEM_POTION:
      send_to_char("Contains spells of: ", ch);

      if (obj->value[1] >= 1)
      {
        sprinttype(obj->value[1], spells, Gbuf1);
        strcat(Gbuf1, "\r\n");
        send_to_char(Gbuf1, ch);
      }

      if (obj->value[2] >= 1)
      {
        sprinttype(obj->value[2], spells, Gbuf1);
        strcat(Gbuf1, "\r\n");
        send_to_char(Gbuf1, ch);
      }

      if (obj->value[3] >= 1)
      {
        sprinttype(obj->value[3], spells, Gbuf1);
        strcat(Gbuf1, "\r\n");
        send_to_char(Gbuf1, ch);
      }
      break;

    case ITEM_WAND:
    case ITEM_STAFF:
      if (obj->value[1] <= 0)
        percent = 0;
      else
        percent =
          100 - (100 / obj->value[1]) * (obj->value[1] - obj->value[2]);

      snprintf(Gbuf1, MAX_STRING_LENGTH,
          "%d%% of its charges remain, and it contains the spell of: ",
          percent);
      send_to_char(Gbuf1, ch);

      if (obj->value[3] >= 1)
      {
        sprinttype(obj->value[3], spells, Gbuf1);
        strcat(Gbuf1, "\r\n");
        send_to_char(Gbuf1, ch);
      }
      break;

    case ITEM_WEAPON:
      snprintf(Gbuf1, MAX_STRING_LENGTH, "Damage Dice is '%dD%d'\r\n", obj->value[1],
          obj->value[2]);
      send_to_char(Gbuf1, ch);
      break;
    case ITEM_INSTRUMENT:
      snprintf(Gbuf1, MAX_STRING_LENGTH, "This instrument has level %d.\r\n", obj->value[1]);
      send_to_char(Gbuf1, ch);
      break;

    case ITEM_ARMOR:
      snprintf(Gbuf1, MAX_STRING_LENGTH, "AC-apply is %d\r\n", obj->value[0]);
      send_to_char(Gbuf1, ch);
      break;

  }

  found = FALSE;
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if ((obj->affected[i].location != APPLY_NONE) &&
        (obj->affected[i].modifier != 0))
    {
      if (found)
        send_to_char(" and", ch);
      else
      {
        send_to_char("This item will also affect your", ch);
        found = TRUE;
      }
      sprinttype(obj->affected[i].location, apply_types, Gbuf2);

      if ((obj->affected[i].location >= APPLY_SAVING_PARA) &&
          (obj->affected[i].location <= APPLY_SAVING_SPELL))
      {
        if (obj->affected[i].modifier < 0)
          strcpy(Gbuf3, "positively");
        else if (obj->affected[i].modifier > 0)
          strcpy(Gbuf3, "negatively");
        else
          strcpy(Gbuf3, "not at all");
      }
      else if (obj->affected[i].location != APPLY_FIRE_PROT)
      {
        if (obj->affected[i].modifier > 0)
          strcpy(Gbuf3, "positively");
        else if (obj->affected[i].modifier < 0)
          strcpy(Gbuf3, "negatively");
        else
          strcpy(Gbuf3, "not at all");
      }
      else
        Gbuf3[0] = 0;

      snprintf(Gbuf1, MAX_STRING_LENGTH, " %s %s", Gbuf2, Gbuf3);
      send_to_char(Gbuf1, ch);
    }
  }
  if( found )
    send_to_char( ".\n\r", ch );

  if( GET_ITEM_TYPE(obj) == ITEM_TOTEM )
  {
    send_to_char( "This totem grants:", ch );
    int val0 = obj->value[0];
    bool bFound = FALSE;

    if( IS_SET(val0, TOTEM_LESS_ANIM) )
    {
      send_to_char_f(ch, "%s&+yLesser Animal&n", bFound ? ", " : " " );
      bFound = TRUE;
    }
    if( IS_SET(val0, TOTEM_GR_ANIM) )
    {
      send_to_char_f(ch, "%s&+yGreater Animal&n", bFound ? ", " : " " );
      bFound = TRUE;
    }
    if( IS_SET(val0, TOTEM_LESS_ELEM) )
    {
      send_to_char_f(ch, "%s&+rLesser Elemental&n", bFound ? ", " : " " );
      bFound = TRUE;
    }
    if( IS_SET(val0, TOTEM_GR_ELEM) )
    {
      send_to_char_f(ch, "%s&+rGreater Elemental&n", bFound ? ", " : " " );
      bFound = TRUE;
    }
    if( IS_SET(val0, TOTEM_LESS_SPIR) )
    {
      send_to_char_f(ch, "%s&+WLesser Spiritual&n", bFound ? ", " : " " );
      bFound = TRUE;
    }
    if( IS_SET(val0, TOTEM_GR_SPIR) )
    {
      send_to_char_f(ch, "%s&+WGreater Spiritual&n", bFound ? ", " : " " );
      bFound = TRUE;
    }
    if( bFound )
    {
      send_to_char( ".\n", ch );
    }
    else
    {
      send_to_char( " No spheres?!\n", ch );
    }
  }

  snprintf(Gbuf2, MAX_STRING_LENGTH, "$p &nhas an item value of &+W%d&n.", itemvalue(obj));
  act(Gbuf2, FALSE, ch, obj, 0, TO_CHAR);
}

const char *MAKE_FORMAT =
"\r\n"
"Current items one can make:\r\n"
"&+L---------------------------&n\r\n"
"lock <object requiring new lock>\r\n"
"key <lock needing a key> (only works for newly created locks)\r\n"
"&+L---------------------------&n\r\n"
"(&+LMany require special materials, skills, classes, and/or levels to perform&n)\r\n"
"\r\n";

void do_make(P_char ch, char *arg, int cmd)
{
  char     arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  if (!arg[0] || !*arg)
    send_to_char(MAKE_FORMAT, ch);

  half_chop(arg, arg1, arg2);

  if (is_abbrev(arg1, "lock"))
    make_lock(ch, arg2);
  else if (is_abbrev(arg1, "key"))
    make_key(ch, arg2);
  else
    send_to_char(MAKE_FORMAT, ch);
}

void yank_make_item(P_char ch, P_obj obj)
{
  P_obj    o;

  if (ch->equipment[HOLD] && (ch->equipment[HOLD] == obj))
  {
    unequip_char(ch, HOLD);
    extract_obj(obj, TRUE); // Templates aren't artifacts, but ok.
  }
  else
  {
    for (o = ch->carrying; o; o = o->next_content)
    {
      if (o == obj)
      {
        obj_from_char(obj);
        extract_obj(obj, TRUE); // Templates aren't artifacts, but ok.
        break;
      }
    }
  }
  obj = NULL;
}

void make_lock(P_char ch, char *arg)
{
  int      door, other_room, retval;
  struct room_direction_data *back;
  P_obj    obj, templ;
  P_char   victim;
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  /*  return; */

  argument_interpreter(arg, Gbuf2, Gbuf3);


  if (!(templ = has_key(ch, VOBJ_TEMPLATE_LOCK)))
  {
    send_to_char("You don't seem to have a lock template to work with.\r\n", ch);
    return;
  }
  if( !*Gbuf2 )
  {
    send_to_char(MAKE_FORMAT, ch);
    return;
  }
  if( IS_TRUSTED(ch) )
  {
    retval = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj);
  }
  else
  {
    retval = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &obj);
  }
  /* this is an object */
  if( retval != 0 )
    if ((obj->type != ITEM_CONTAINER) && (obj->type != ITEM_STORAGE))
      send_to_char("That's not a lockable container.\r\n", ch);
    else if (!IS_SET(obj->value[1], CONT_CLOSED))
      send_to_char("Maybe you should close it first...\r\n", ch);
    else if (IS_SET(obj->value[1], CONT_LOCKED))
      send_to_char("It is locked already.\r\n", ch);
    else
    {
      SET_BIT(obj->value[1], CONT_LOCKED);
      obj->value[2] = 1;
      yank_make_item(ch, templ);
      send_to_char("You fashion a workable lock!\r\n", ch);
      act("$n locks $p with a homemade lock.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if ((door = find_door(ch, Gbuf2, Gbuf3)) >= 0)
    /* a door, perhaps */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("You have to close it first, I'm afraid.\r\n", ch);
    else if (!has_key(ch, EXIT(ch, door)->key) && !IS_TRUSTED(ch))
      send_to_char("You don't have the proper key.\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It's already locked!\r\n", ch);
    else
    {
      SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      EXIT(ch, door)->key = 1;  /* so its now a lockable, but keyless object */
      yank_make_item(ch, templ);
      if (EXIT(ch, door)->keyword)
        act("$n locks the $F with a homemade lock.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      else
        act("$n locks the door with a homemade lock.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You fashion a crude, but workable lock.\r\n", ch);
      /* now for locking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if ((back = world[other_room].dir_option[rev_dir[door]]))
          if (back->to_room == ch->in_room)
          {
            SET_BIT(back->exit_info, EX_LOCKED);
            back->key = 1;      /* so its now a lockable, but keyless object */
          }
    }
}

void make_key(P_char ch, char *arg)
{
  int      door, retval;
  P_obj    obj, templ, template2;
  P_char   victim;
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  argument_interpreter(arg, Gbuf2, Gbuf3);

  if (!(templ = has_key(ch, VOBJ_TEMPLATE_KEY)))
  {
    send_to_char("You don't seem to have a blank key to work with.\r\n", ch);
    return;
  }
  template2 = ch->equipment[HOLD];
  if (!template2 || (template2->type != ITEM_PICK))
  {
    send_to_char("You need to be able to pick a lock, before you can fashion a key.\r\n", ch);
    return;
  }
  if (!*Gbuf2)
  {
    send_to_char(MAKE_FORMAT, ch);
    return;
  }
  if( IS_TRUSTED(ch) )
  {
    retval = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj);
  }
  else
  {
    retval = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &obj);
  }

  /* this is an object */
  if( retval != 0 )
  {
    if ((obj->type != ITEM_CONTAINER) && (obj->type != ITEM_STORAGE))
      send_to_char("You fail to find a proper lock on that.\r\n", ch);
    else
    {
      obj->value[2] = obj_index[obj->R_num].virtual_number;
      templ->str_mask = (STRUNG_DESC1 | STRUNG_DESC2);
      templ->short_description = str_dup("&+ca small custom-made key&n");
      templ->description = str_dup("A small, custom-made key lies here, forgotten.");
      send_to_char("You fashion a key for this lock!\r\n", ch);
      if( (template2->value[1] > -1) && (number(1, 101) < template2->value[1]) )
      {
        act("Damn!  You broke your $p too!", FALSE, ch, template2, 0, TO_CHAR);
        act("$n begins cursing under $s breath as $s $p snaps.", FALSE, ch, template2, 0, TO_ROOM);
        if (ch->equipment[HOLD] && (ch->equipment[HOLD] == template2))
          unequip_char(ch, HOLD);
        extract_obj(template2, TRUE); // Not that there are any artifact templates, but ok.
      }
    }
  }
  else if ((door = find_door(ch, Gbuf2, Gbuf3)) >= 0)
    /* a door, perhaps */
#if 0
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else
    {
      EXIT(ch, door)->key = 1;
      template->str_mask = (STRUNG_DESC1 | STRUNG_DESC2);
      template->short_description = str_dup("&+ca small custom-made key&n");
      template->description =
        str_dup("A small, custom-made key lies here, forgotten.");
      send_to_char("You fashion a key for this lock!\r\n", ch);
    }
#else
  /* not sure of best method for doors, so disable */
  send_to_char("You're just not quite sure of how to handle this one.\r\n",
      ch);
#endif
}

void do_throat_crush(P_char ch, char *arg, int cmd)
{
  P_char   vict = NULL;
  struct affected_type af;
  int      skl_lvl = 0;
  int      i = 0;

  struct damage_messages messages = {
    "You lunge at $N, crushing $S throat.",
    "$n lunges at you, crushing your windpipe.",
    "$n lunges at $N, crushing $S windpipe!",
    "You lunge at $N, crushing $S throat totally!",
    "$n lunges at you, crushing your windpipe totally.",
    "$n lunges at $N, crushing $S windpipe totally!"
  };

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_DESTROYING(ch) )
  {
    send_to_char( "You can't throat crush an object.\n", ch );
    return;
  }

  if(!IS_FIGHTING(ch))
  {
    vict = ParseTarget(ch, arg);
    if (!vict)
    {
      send_to_char("Crush whose throat?\r\n", ch);
      return;
    }
  }
  else
  {
    vict = GET_OPPONENT(ch);
    if (!vict)
    {
      stop_fighting(ch);
      return;
    }
  }

  if(IS_PC(ch))
  {
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_THROAT_CRUSH);
  }
  else if(IS_NPC(ch) &&
      GET_CLASS(ch, CLASS_MERCENARY))
  {
    skl_lvl = GET_LEVEL(ch) * 2;
  }

  if(!skl_lvl)
  {
    send_to_char("You cannot throat crush.\r\n", ch);
    return;
  }

  if(!CanDoFightMove(ch, vict))
  {
    return;
  }
  /*
     if (ch->equipment[WIELD] && ch->equipment[SECONDARY_WEAPON]) {
     send_to_char("You need a free hand to do this.\r\n", ch);
     return;
     }
     */
  CharWait(ch, PULSE_VIOLENCE * 1);

  if(affected_by_spell_flagged(ch, SKILL_THROAT_CRUSHER, 0))
  {
    send_to_char("Get in position first.\r\n", ch);
    return;
  }

  if(affected_by_spell_flagged(vict, SKILL_THROAT_CRUSH, AFFTYPE_CUSTOM1))
  {
    send_to_char("This person's throat is already sore.\r\n", ch);
    CharWait(ch, PULSE_VIOLENCE * 1);

    return;
  }

  if(!IS_HUMANOID(vict) ||
      IS_GREATER_RACE(vict) ||
      IS_ELITE(vict))
  {
    act("Such a maneuver appears to be useless against $N!", FALSE, ch, 0,
        vict, TO_CHAR);
    act("$n close in on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  if(!on_front_line(vict))
  {
    send_to_char("You can't reach their throat.\r\n", ch);
    return;
  }

  if(GET_ALT_SIZE(vict) > GET_ALT_SIZE(ch) + 1)
  {
    act("You cannot reach $N's throat!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n close in on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  if(GET_ALT_SIZE(vict) < GET_ALT_SIZE(ch) - 1)
  {
    send_to_char("They are too small for you.\r\n", ch);
    act("$n close in on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  i = (int)(skl_lvl - (GET_C_AGI(vict) / 10));

  if (GET_C_DEX(ch) > GET_C_DEX(vict))
  {
    i = (int) (i * 1.1);
  }
  else
  {
    i = (int) (i * 0.9);
  }

  if (GET_C_LUK(ch) > GET_C_LUK(vict))
  {
    i = (int) (i * 1.1);
  }
  else
  {
    i = (int) (i * 0.9);
  }

  skl_lvl = MIN(i, 90);

  if(notch_skill(ch, SKILL_THROAT_CRUSH, get_property("skill.notch.offensive", 4) ||
        number(1, 100) > i))
  {
    send_to_char("You miss their throat!\r\n", ch);
    act("$n, lunges at $N, but misses!", FALSE, ch, 0, vict, TO_NOTVICT);
    act("$n, lunges at you, but misses!", FALSE, ch, 0, vict, TO_VICT);
    CharWait(ch, PULSE_VIOLENCE * 2);
    return;
  }

  melee_damage(ch, vict, 30, PHSDAM_TOUCH, &messages);
  StopCasting(vict);

  memset(&af, 0, sizeof(af));
  af.type = SKILL_THROAT_CRUSH;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_CUSTOM1;
  af.duration = number(1, 3) * PULSE_VIOLENCE;
  affect_to_char(vict, &af);

  af.type = SKILL_THROAT_CRUSHER;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_CUSTOM2;
  af.duration = number(2, 4) * PULSE_VIOLENCE;
  affect_to_char(ch, &af);
}

void do_hamstring(P_char ch, char *arg, int cmd)
{
  struct affected_type *af, afs;
  P_char   vict = NULL;
  int      skl_lvl = 0, sect;
  int      i = 0;

  if (!ch)
    return;

  if( IS_DESTROYING(ch) )
  {
    send_to_char( "You can't hamstring an object.\n", ch );
    return;
  }

  //  if(!GET_SPEC(ch, CLASS_ASSASSIN,  SPEC_ASSMASTER))
  //   return;

  sect = world[ch->in_room].sector_type;

  if (!HAS_FOOTING(ch) ||
      ((sect == SECT_UNDERWATER) ||
       (sect == SECT_FIREPLANE) ||
       ((sect >= SECT_AIR_PLANE) && (sect <= SECT_ASTRAL))))
  {
    send_to_char
      ("It's just a little too hard to hamstring someone when you don't have any footing..\r\n",
       ch);
    return;
  }

  if (!IS_FIGHTING(ch))
  {
    vict = ParseTarget(ch, arg);
    if (!vict)
    {
      send_to_char("Hamstring who?\r\n", ch);
      return;
    }
  }
  else
  {
    vict = GET_OPPONENT(ch);
    if (!vict)
    {
      stop_fighting(ch);
      return;
    }
  }

  if (IS_PC(ch))
  {
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_HAMSTRING);
  }
  else
    skl_lvl = MAX(100, GET_LEVEL(ch) * 3);

  if (IS_NPC(ch))               // && !GET_CLASS(ch, CLASS_ASSASSIN) && !GET_CLASS(ch, CLASS_THIEF))
    return;

  if (!IS_NPC(ch) && (skl_lvl <= 0))
  {
    send_to_char("You don't know how!\r\n", ch);
    return;
  }

  appear(ch);

  if (IS_AFFECTED(vict, AFF_AWARE) && IS_AWAKE(vict))
    skl_lvl -= 10;
  else if (IS_AWAKE(vict) && IS_AFFECTED(vict, AFF_SKILL_AWARE))
  {
    for (af = vict->affected; af; af = af->next)
    {
      if (af->type == SKILL_AWARENESS)
      {
        break;
      }
    }
    if (af)
      skl_lvl -= af->modifier;
    else
      logit(LOG_DEBUG, "aware, but no affected structure");
  }

  /* at this point target is pissed regardless, since dingledorf tried */

  if (IS_NPC(vict) && CAN_SEE(vict, ch))
  {
    remember(vict, ch);
    if (!IS_FIGHTING(vict))
      MobStartFight(vict, ch);
    if (!char_in_list(ch) || !char_in_list(vict))
      return;
  }

  if (!skl_lvl)
  {
    act("You sweep downward and try to slice the back of $N's knees.", FALSE,
        ch, 0, vict, TO_CHAR);
    act("$n swings low and tries to slice the back of your knees.", TRUE, ch,
        0, vict, TO_VICT);
    act("$n does a fancy move and tries to slice the back of $N's knees.",
        TRUE, ch, 0, vict, TO_NOTVICT);
    return;
  }
  if (!CanDoFightMove(ch, vict))
    return;

  if (!ch->equipment[WIELD] ||
      (!IS_SWORD(ch->equipment[WIELD]) &&
       !IS_BACKSTABBER(ch->equipment[WIELD])))
  {
    send_to_char
      ("You need to wield the right kind of weapon to even try..\r\n", ch);
    return;
  }

  if (GET_RACE(vict) == RACE_GHOST)
  {
    send_to_char("How can you hamstring a ghost?\r\n", ch);
    return;
  }
  if (IS_AFFECTED4(vict, AFF4_PHANTASMAL_FORM))
  {
    send_to_char("How can you hamstring someone in phantasmal form?\r\n", ch);
    return;
  }

  if (IS_DEMON(vict) || IS_DRAGON(vict) || 
      (GET_RACE(vict) == RACE_PLANT))
  {
    act("Such a maneuver appears to be useless against $N!", FALSE, ch, 0,
        vict, TO_CHAR);
    send_to_char("And just for the attempt, you fall on yer ass.\r\n", ch);
    act("$n tries a tricky maneuver on you, and winds up flat on $s ass.",
        TRUE, ch, 0, vict, TO_VICT);
    act("$n tries some fancy maneuver on $N, but winds up flat on $s ass.",
        TRUE, ch, 0, vict, TO_ROOM);
    CharWait(ch, PULSE_VIOLENCE * 2);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    update_pos(ch);
    return;
  }
  if (!IS_HUMANOID(vict) || IS_ILLITHID(vict) || IS_PILLITHID(vict))
  {
    send_to_char("Does that thing even HAVE a hamstring?!\r\n", ch);
    return;
  }

  /*
     if (GET_ALT_SIZE(vict) > GET_ALT_SIZE(ch) + 2)
     {
     act
     ("$n makes a futile attempt to hamstring $N, but $e but misses the sweet spot.",
     FALSE, ch, 0, vict, TO_ROOM);
     act("You slip as you try to get behind that huge leg! ", FALSE, ch, 0,
     vict, TO_CHAR);
     SET_POS(ch, POS_SITTING + GET_STAT(ch));
     CharWait(ch, PULSE_VIOLENCE * 2);
     update_pos(ch);
     return;
     }
     if (GET_ALT_SIZE(vict) < GET_ALT_SIZE(ch) - 1)
     {
     send_to_char("How can you hamstring something so small?\r\n", ch);
     return;
     }
     */
  justice_witness(ch, vict, CRIME_ATT_MURDER);

  i = (skl_lvl) - (GET_C_AGI(vict) / 10);

  act("You sweep downward and try to slice the back of $N's knees!",
      FALSE, ch, 0, vict, TO_CHAR);
  act("$n swings low and tries to slice the back of your knees!",
      TRUE, ch, 0, vict, TO_VICT);
  act("$n does a fancy move and tries to slice the back of $N's knees!",
      TRUE, ch, 0, vict, TO_NOTVICT);

  if ((number(1, 100) < i) && !IS_TRUSTED(vict))
  {
    act("&+r$n&+r stumbles as blood gushes from $s knee!", TRUE, vict, 0, 0,
        TO_ROOM);
    send_to_char
      ("&+rYou feel an intense pain in your leg and stumble slightly!\r\n",
       vict);

    /*
       bzero(af, sizeof(affected_type));
       af->location = APPLY_MOVE;
       */

    i = number(40, 70);
    if ((GET_VITALITY(vict) - i) < 20)
      i = GET_VITALITY(vict) - 20;

    GET_VITALITY(vict) -= i;

    StartRegen(vict, EVENT_MOVE_REGEN);

    if (!affected_by_spell(vict, SKILL_AWARENESS))
    {
      bzero(&afs, sizeof(afs));
      afs.type = SKILL_AWARENESS;
      afs.duration = 5;
      afs.bitvector = AFF_AWARE;
      afs.flags = AFFTYPE_NODISPEL;
      affect_to_char(vict, &afs);
    }
    /*
       af.modifier = -((GET_MAX_VITALITY(vict) / 2) + dice(5, 15));
       af.duration = 3;
       affect_to_char(vict, &af);
       */
  }
  notch_skill(ch, SKILL_HAMSTRING, get_property("skill.notch.offensive", 7));
  CharWait(ch, PULSE_VIOLENCE * 2);

  if (IS_NPC(vict) && CAN_SEE(vict, ch))
  {
    remember(vict, ch);
    if (!IS_FIGHTING(vict))
      MobStartFight(vict, ch);
  }
  return;
}


void do_arena(P_char ch, char *arg, int cmd)
{
  int      rm, i, j, f = 0;
  char     strn[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
  char     arg1[256], arg2[256];
  P_char   c;


  if (!ch)
    return;                     /* ? */

  rm = world[ch->in_room].number;
  if (!CHAR_IN_ARENA(ch) && !IS_TRUSTED(ch))
  {
    send_to_char("Pardon?\r\n", ch);
    return;
  }

  if (!*arg)
  {
    show_stats_to_char(ch);
    if (IS_SET(arena.flags, FLAG_SEENAME))
    {

    }
    else
    {
      snprintf(strn, MAX_STRING_LENGTH,
          "&+YGOODIES: &+W%-10d &N&+rEVILS: &+W%-10d &+LUNDEAD: &+W%d&N\r\n",
          arena.team[GOODIE].score, arena.team[EVIL].score,
          arena.team[UNDEAD].score);
      send_to_char(strn, ch);
      if (IS_SET(arena.flags, FLAG_ENABLED))
      {
        send_to_char("&+GArena is open&N\r\n", ch);
      }
      else
      {
        send_to_char("&+LArena is closed&N\r\n", ch);
      }
    }
  }

  if (!IS_TRUSTED(ch))
    return;

  if (isname(arg, "on"))
  {
    if (IS_SET(arena.flags, FLAG_ENABLED))
    {
      send_to_char("The Arena is already on!\r\n", ch);
      return;
    }

    SET_BIT(arena.flags, FLAG_ENABLED);
    arena.timer[0] = DEFAULT_TIMER_OPEN;
    arena.stage = STAGE_OPEN;
    send_to_char("Arena has been enabled.\r\n", ch);
    return;
  }

  if (isname(arg, "off"))
  {
    if (!IS_SET(arena.flags, FLAG_ENABLED))
    {
      send_to_char("The Arena is already off!\r\n", ch);
      return;
    }
    SET_BIT(arena.flags, FLAG_SHUTTING_DOWN);
    send_to_char
      ("Arena has been set to shutdown, all players being evicted.\r\n", ch);
    return;
  }
  half_chop(arg, arg1, arg2);
  if (isname(arg1, "type t"))
  {
    if (is_number(arg2))
    {
      i = atoi(arg2);
      if (i < 0 || i > MAX_ARENA_TYPE)
      {
        send_to_char("Invalid type!\r\n", ch);
        return;
      }
      arena.type = i;
      snprintf(strn, MAX_STRING_LENGTH, "Arena mode changed to: %s\r\n", game_type[i]);
      send_to_char(strn, ch);
      return;
    }
    else
    {
      send_to_char("Enter number!\r\n", ch);
      return;
    }
  }
}


void do_vote(P_char ch, char *arg, int cmd)
{
  char     vote_opts[4096];
  int      votes, i;
  char     vote_str[4096];
  char     voted[4096];
  int      vote_serial = 9;
  int      max_vote = 0;
  FILE    *f = NULL;
  const char *ip;
  char ipbuf[255];
  time_t   lastConnect, lastDisconnect;
  char    *vote_options[] = {
    "nothing",
    "Follow through on the Miniwipe",
    "Completely abandon the group-cap idea.",
    "Wait until Sept 12th to test 5-man group caps.",
    ""
  };

  snprintf(vote_opts, 4096, "\r\n");

  for (i = 1; '\0' != vote_options[i][0]; i++)
  {
    snprintf(vote_opts, 4096, "%s%d)  %s\r\n", vote_opts, (i), vote_options[i]);
    max_vote = i;
  }

  if (IS_NPC(ch))
  {
    send_to_char("Mobs can't vote, silly.\r\n", ch);
    return;
  }
  if (GET_LEVEL(ch) < MINIMUM_VOTE_LEVEL)
  {
    send_to_char("You can't vote, you're not high enough.\r\n", ch);
    return;
  }
  if (!voting_enabled)
  {
    send_to_char("Sorry, the polls are closed!\r\n", ch);
    return;
  }
  if (!is_number(arg))
  {
    if( GET_LEVEL(ch) >= FORGER && !str_cmp(skip_spaces(arg), "close") )
    {
      voting_enabled = 0;
      snprintf(vote_str, 4096, "&=Ly%s has closed the voting polls!\n\r", J_NAME(ch) );
      send_to_all( vote_str );
      return;
    }
    send_to_char("\r\nHow do you feel about the miniwipe/group caps?\r\n", ch);
    send_to_char("------", ch);
    send_to_char(vote_opts, ch);
    send_to_char("------\r\n", ch);
    if( GET_LEVEL(ch) >= FORGER )
    {
      send_to_char( "&+yOr you can 'vote close' to close the polls.&n\n\r", ch );
    }
    return;
  }

  if ((ch->only.pc->vote == vote_serial) && !isname(GET_NAME(ch), "io"))
  {
    send_to_char("You've already voted in this election!\r\n", ch);
    return;
  }

  votes = atoi(arg);
  if ((strlen(arg) != 1) || (votes < 1) || (votes > max_vote))
  {
    send_to_char("Hey, you left hanging chads!  Try again!\r\n", ch);
    send_to_char("The proper format is: vote x  'Replace x with the number of your choice.\r\n", ch);
    return;
  }

  snprintf(voted, 4096, "You voted for: %s.\r\n", vote_options[votes]);
  f = fopen("lib/etc/vote.miniwipe", "a");
  if( !f )
  {
    send_to_char("There was a problem recording your vote, notify a Forger.\r\n", ch);
    return;
  }
  ip = sql_select_IP_info( ch, ipbuf, sizeof(char)*255, &lastConnect, &lastDisconnect );
  fprintf(f, "%s\t%s\t%s\n", ip, J_NAME(ch), vote_options[votes]);
  ch->only.pc->vote = vote_serial;

  if( f )
  {
    fclose(f);
  }

  send_to_char(voted, ch);

}

void do_craft(P_char ch, char *argument, int cmd)
{
  char     buf1[MAX_STRING_LENGTH];
  char     first[MAX_INPUT_LENGTH];
  char     second[MAX_INPUT_LENGTH];
  char     rest[MAX_INPUT_LENGTH];
  int i = 0;
  int choice = 0;
  P_obj hammer, foundry;

  /***DISPLAYRECIPES STUFF***/
  char buf[256], *buff, buf2[256];
  char Gbuf1[MAX_STRING_LENGTH], selectedrecipe[MAX_STRING_LENGTH];
  char tempdesc [MAX_INPUT_LENGTH];
  char short_desc[MAX_STRING_LENGTH];
  char keywords[MAX_INPUT_LENGTH];
  char buffer[256];
  FILE *f;
  FILE *recipelist;
  int line, recfind;
  unsigned long	linenum = 0;
  long recnum, choice2;
  long selected = 0;
  P_obj tobj;

  /* Dunno the difference between this and above?
     P_obj    craft_obj1, craft_obj2, craft_obj3, obj;
     P_obj    t_obj, nextobj;
     int      i, bits, j, in_room, material_type, item_type, howmany,
     weapon_types, slot;
     bool     equipped;
     P_char   victim = NULL;
     char     Gbuf1[MAX_STRING_LENGTH];
     char     Gbuf2[MAX_STRING_LENGTH];
     char    *r_str;

     equipped = FALSE;
     obj = 0;
     */

  if( !GET_CHAR_SKILL(ch, SKILL_CRAFT) )
  {
    act("You do not know how to &+rcraft&n items.",
        FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  //Create buffers for name
  strcpy(buf, GET_NAME(ch));
  // Name should all be lowercase except first initial, but ok.
  for( buff = buf; *buff; buff++ )
  {
    *buff = LOWER(*buff);
  }
  //buf[0] snags first letter of name
  snprintf(Gbuf1, MAX_STRING_LENGTH, "Players/Tradeskills/%c/%s.crafting", buf[0], buf);
  recipelist = fopen(Gbuf1, "r");
  if( !recipelist )
  {
    send_to_char("You dont know any recipes yet.\r\n", ch);
    return;
  }

  // If no arguments, display syntax and list of known recipes.
  if( !argument || !*argument )
  {
    send_to_char("&+wcraft Syntax:\n&n", ch);
    send_to_char("&+w(craft info <number> - list required materials to craft the item.)\n&n", ch);
    send_to_char("&+w(craft stat <number> - display properties of the item.)\n&n", ch);
    send_to_char("&+w(craft make <number> - create the item.)\n&n", ch);
    send_to_char("&+yYou know the following recipes:\n&n", ch);
    send_to_char("----------------------------------------------------------------------------\n", ch);
    send_to_char("&+BRecipe Number		              &+MItem&n\n\r", ch);
    // Walk through each item in recipe book, and display it's number and short description.
    while( (fscanf(recipelist, "%ld", &recnum)) != EOF )
    {
      // Load the object so we can get its short description.
      tobj = read_object(recnum, VIRTUAL);
      snprintf(buffer, 256, "   &+W%-22ld&n%s&n\n", recnum, tobj->short_description);
      page_string(ch->desc, buffer, 1);
      send_to_char("----------------------------------------------------------------------------\n", ch);
      extract_obj(tobj);
    }
    fclose(recipelist);
    return;
  }

  // If no argument, no sense in chopping it up, so moved this past the !arg check.
  half_chop(argument, first, rest);
  half_chop(rest, second, rest);
  choice2 = atoi(second);

  // Walk through the list of recipes and look for choice2 (if there was a 2nd argument that was a valid number).
  if( choice2 )
  {
    while( (fscanf(recipelist, "%ld", &recnum)) != EOF )
    {
      // If we find the recipe, mark it and stop looking.
      if( recnum == choice2 )
      {
        selected = choice2;
        break;
      }
      /* debug
      char bufbug[MAX_STRING_LENGTH];
      snprintf(bufbug, MAX_STRING_LENGTH, "choice is: %d\r\n", selected);
      send_to_char(bufbug, ch);
      if( recnum == choice2 )
      {
        send_to_char("The one below here is selected.\r\n", ch);
      }
      */
    }
  }
  fclose(recipelist);

  // If they picked a recipe that wasn't in their book...
  if( choice2 != 0 && selected == 0 )
  {
    send_to_char("You dont appear to have that &+Wrecipe&n in your list.&n\n", ch);
    return;
  }

  if( is_abbrev(first, "stat") )
  {
    if( choice2 == 0 )
    {
      send_to_char("What &+Wrecipe&n would you like &+ystatistics&n about?\n", ch);
      return;
    }
    tobj = read_object(selected, VIRTUAL);
    send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
    spell_identify(GET_LEVEL(ch), ch, 0, 0, 0, tobj);
    extract_obj(tobj);
    return;
  }
  else if( is_abbrev(first, "info") )
  {
    if(choice2 == 0)
    {
      send_to_char("What &+Wrecipe&n would you like &+yinformation&n about?\n", ch);
      return;
    }

    tobj = read_object(selected, VIRTUAL);
    if( !tobj )
    {
      debug( "Couldn't load object vnum %d.", selected );
      send_to_char( "Couldn't create the object. :(\n\r", ch );
      return;
    }

    // Minimum value is iVal 1 -> 1 Max quality material, 0 Min quality materials to create.
    int iVal = itemvalue(tobj);
    int lowQualityMaterialVnum = get_matstart(tobj);

    if( lowQualityMaterialVnum <= 0 )
    {
      send_to_char( "Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch );
      debug( "Couldn't get start material for object: '%s' %d.", tobj->short_description, selected );
      extract_obj(tobj);
      return;
    }

    // The number of highest quality materials is 1 per 5 itemvalue (with starting point 1@1).
    int numHighest = (iVal + 4) / 5;
    // The following gets the remainder.
    int numLowest = (iVal + 4) - numHighest * 5;

    P_obj matLowest, matHighest;

    matLowest = read_object(lowQualityMaterialVnum, VIRTUAL);
    matHighest = read_object(lowQualityMaterialVnum + 4, VIRTUAL);

    if( matLowest == NULL || matHighest == NULL )
    {
      send_to_char( "Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch );
      debug( "Couldn't load a material: matLowest(%s) or matHighest(%s) for object '%s' %d.",
        (matLowest == NULL) ? "NULL" : matLowest->short_description,
        (matHighest == NULL) ? "NULL" : matHighest->short_description, tobj->short_description, selected );
      extract_obj(tobj);
      return;
    }

    if( numLowest == 0 )
    {
      send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
      snprintf(buf1, MAX_STRING_LENGTH, "To craft this item, you will need %d of %s.\r\n&n", numHighest, matHighest->short_description);
      page_string(ch->desc, buf1, 1);
    }
    else
    {
      send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
      snprintf(buf1, MAX_STRING_LENGTH, "To craft this item, you will need %d of %s and %d of %s.\r\n&n",
        numHighest, matHighest->short_description, numLowest, matLowest->short_description);
      page_string(ch->desc, buf1, 1);
    }

/* It will never happen that we don't need at least one of matHighest since min itemvalue is 1,
 *   then we add 4 (total 5) and divide by 5 (result minimum 1).
 * But, in case we change that, I'm leaving the code here. 2/10/2015
      send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
      snprintf(buf1, MAX_STRING_LENGTH, "To craft this item, you will need %d of %s.\r\n&n", numLowest, matLowest->short_description);
      page_string(ch->desc, buf1, 1);
*/

    if( has_affect(tobj) )
    {
      send_to_char("...as well as &+W1 &nof &+ma &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml &+messence&n due to the &+mmagical &nproperties this item possesses.\r\n", ch);
    }

    // It's safe to assume tobj exists since we checked after the read_object call.
    extract_obj(tobj);
    // The same follows for matLowest and matHighest.
    extract_obj(matLowest);
    extract_obj(matHighest);
    return;
  }
  else if (is_abbrev(first, "make"))
  {
    if( choice2 == 0 )
    {
      send_to_char("What &+Witem&n are you attempting to craft?\n", ch);
      return;
    }

    tobj = read_object(selected, VIRTUAL);
    if( !tobj )
    {
      debug( "Couldn't load object vnum %d.", selected );
      send_to_char( "Couldn't create the object. :(\n\r", ch );
      return;
    }

    // Minimum value is iVal 1 -> 1 Max quality material, 0 Min quality materials to create.
    int iVal = itemvalue(tobj);
    if( iVal > 100 || IS_OBJ_STAT2(tobj, ITEM2_QUESTITEM) )
    {
      act("You look at the recipe for $p&n, but can't seem to discern how to make it.  &+mHow strange.&N",
        FALSE, ch, tobj, 0, TO_CHAR);
      extract_obj(tobj);
      return;
    }

    int lowQualityMaterialVnum = get_matstart(tobj);
    int highQualityMaterialVnum = lowQualityMaterialVnum + 4;

    if( lowQualityMaterialVnum <= 0 )
    {
      send_to_char( "Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch );
      debug( "Couldn't get start material for object: '%s' %d.", tobj->short_description, selected );
      extract_obj(tobj);
      return;
    }

    // The number of highest quality materials is 1 per 5 itemvalue (with starting point 1@1).
    int numHighest = (iVal+4)/5;
    // The following gets the remainder.
    int numLowest = iVal - numHighest * 5;

    P_obj matLowest, matHighest;

    matLowest = read_object(lowQualityMaterialVnum, VIRTUAL);
    matHighest = read_object(highQualityMaterialVnum, VIRTUAL);

    if( matLowest == NULL || matHighest == NULL )
    {
      send_to_char( "Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch );
      debug( "Couldn't load a material: matLowest(%s) or matHighest(%s) for object '%s' %d.",
        (matLowest == NULL) ? "NULL" : matLowest->short_description,
        (matHighest == NULL) ? "NULL" : matHighest->short_description, tobj->short_description, selected );
      extract_obj(tobj);
      return;
    }

    // If we're going to require multiple essences, we need to change this to int numAffects ...
    bool hasAffect = has_affect(tobj);

    int invLowMats = 0; // Number of lowest quality materials in inventory.
    int invHighMats = 0; // Number of highest quality materials in inventory.
    int invEssences = 0; // Number of magical essences in inventory.
    int invTools = 0; // Number of crafting tools in inventory
    int invVnum = 0; // Vnum of the current inventory object.
    for( P_obj inventory = ch->carrying; inventory; inventory = inventory->next_content )
    {
      invVnum = OBJ_VNUM(inventory);

      if( invVnum == lowQualityMaterialVnum )
        invLowMats++;
      else if( invVnum == highQualityMaterialVnum )
        invHighMats++;
      else if( invVnum == VOBJ_CRAFTING_ESSENCE )
        invEssences++;
      else if( invVnum == VOBJ_CRAFTING_TOOLS )
        invTools++;
    }

    // Check mats in inventory vs mats needed.
    if( invLowMats < numLowest || invHighMats < numHighest )
    {
      send_to_char("You do not have the required &+ysalvaged &+Ymaterials &nin your inventory.\r\n", ch);
      extract_obj(tobj);
      extract_obj(matLowest);
      extract_obj(matHighest);
      return;
    }
    // If for some reason we want more than 1 box of tools, change the 1 below.
    if( invTools < 1)
    {
      send_to_char("You must have &+ma &+ybox &+mof &+Rgnomish &+rcrafting &+mtools&n to create your item.\r\n", ch);
      extract_obj(tobj);
      extract_obj(matLowest);
      extract_obj(matHighest);
      return;
    }
    // If we're going to require multiple essences, need to edit this if statement.
    if( invEssences < 1 && hasAffect )
    {
      send_to_char("You must have &+W1 &nof &+ma &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml &+messence&n due to the &+mmagical &nproperties this item possesses.\r\n", ch);
      extract_obj(tobj);
      extract_obj(matLowest);
      extract_obj(matHighest);
      return;
    }

    // Remove the materials from inventory...  Since we are changing the inventory
    //   list, we need to use nextObj instead of just going to inventory->next_content.
    P_obj nextObj;
    // If for some reason we want more than 1 box of tools, change gotTools from boolean to int...
    //   For right now, gotTools -> TRUE when we extract a set of tools from inventory.
    bool gotTools = FALSE;
    for( P_obj inventory = ch->carrying; inventory; inventory = nextObj )
    {
      nextObj = inventory->next_content;
      invVnum = OBJ_VNUM(inventory);

      if( (numLowest > 0) && (invVnum == lowQualityMaterialVnum) )
      {
        obj_from_char(inventory);
        extract_obj(inventory);
        numLowest--;
      }
      else if( (numHighest > 0) && (invVnum == highQualityMaterialVnum) )
      {
        obj_from_char(inventory);
        extract_obj(inventory);
        numHighest--;
      }
      // If we're requiring multiple essences, need to change this if clause.
      else if( hasAffect && (invVnum == VOBJ_CRAFTING_ESSENCE) )
      {
        obj_from_char(inventory);
        extract_obj(inventory);
        hasAffect = FALSE;
      }
      // If we're requiring multiple tools, need to change this if clause.
      else if( !gotTools && (invVnum == VOBJ_CRAFTING_TOOLS) )
      {
        obj_from_char(inventory);
        extract_obj(inventory);
        gotTools = TRUE;
      }
    }

    notch_skill(ch, SKILL_CRAFT, 50);

    SET_BIT(tobj->extra2_flags, ITEM2_CRAFTED);
    SET_BIT(tobj->extra_flags, ITEM_NOREPAIR);
    REMOVE_BIT(tobj->extra_flags, ITEM_SECRET);
    randomizeitem(ch, tobj);
    snprintf(keywords, MAX_STRING_LENGTH, "%s %s tradeskill", tobj->name, GET_NAME(ch));

    snprintf(tempdesc, MAX_STRING_LENGTH, "%s", tobj->short_description);
    snprintf(short_desc, MAX_STRING_LENGTH, "%s &+ymade by&n &+r%s&n", tempdesc, GET_NAME(ch));
    set_keywords(tobj, keywords);
    set_short_description(tobj, short_desc);

    // Rewards here: tobj is the crafted item, so no need to load another.
    wizlog(56, "%s crafted '%s' (%d) ival %d.", GET_NAME(ch), tobj->short_description, selected, itemvalue(tobj) );

    obj_to_char(tobj, ch);
    act("&+W$n &+Ldelicately opens their &+ybox &+mof &+Rgnomish &+rcrafting &+mtools&+L and starts their work...\r\n"
      "&+W$n &+Lremoves the &+Wim&+wpur&+Lities &+Lfrom their &+ymaterials &+Land gently assembles a masterpiece...\r\n"
      "&+L...hands shaking, &+W$n &+Lraises their head and &+Ysmiles&+L, admiring their new $p.&N",
      TRUE, ch, tobj, 0, TO_ROOM);
    act("You &+Ldelicately open your &+ybox &+mof &+Rgnomish &+rcrafting &+mtools&+L and get to work...\r\n"
      "you &+Lremove the &+Wim&+wpur&+Lities &+Lfrom your &+ymaterials &+Land gently assemble a masterpiece...\r\n"
      "&+L...hands shaking, &+Wyou &+Lraise your head and &+Ysmile&+L, admiring your new $p.&N",
      FALSE, ch, tobj, 0, TO_CHAR);

    gain_exp(ch, NULL, iVal*1000, EXP_BOON);
    extract_obj(matLowest);
    extract_obj(matHighest);
    // Save the character! 1 -> in game.
    do_save_silent(ch, 1);
  }

  /*
     argument = one_argument(argument, Gbuf1);
     howmany = 0;
     i = 0;
     craft_obj1 = 0;
     craft_obj2 = 0;
     craft_obj3 = 0;

     for (t_obj = ch->carrying; t_obj; t_obj = nextobj)
     {
     nextobj = t_obj->next_content;


     if (isname("piece", t_obj->name) && !isname("tail", t_obj->name) &&
     obj_index[t_obj->R_num].virtual_number == RANDOM_EQ_VNUM)
     {
     i++;

     if (i == 1)
     {
     craft_obj1 = t_obj;
     }
     if (i == 2)
     {
     craft_obj2 = t_obj;
     }
     if (i == 3)
     {
     craft_obj3 = t_obj;
     }
     }

     }
     slot = -1;

     while (*argument == ' ')
     argument++;


     for (i = 0; i < MAX_SLOT; i++)
     {
     snprintf(Gbuf2, MAX_STRING_LENGTH, "%s\0", strip_ansi(slot_data[i].m_name).c_str());
     if (!strcmp(argument, Gbuf2))
     {
     howmany = slot_data[i].numb_material;
     slot = i;
     break;
     }
     }

     if (slot == -1 || howmany > 3)
     {
     act("You don't know how to create that...", FALSE, ch, 0, 0, TO_CHAR);
     return;
     }
     if (craft_obj1 == 0)
     {
     act("You do not have enough material, need to have a piece in inventory",
     FALSE, ch, 0, 0, TO_CHAR);
     return;
     }
     if (craft_obj2 == 0 && howmany == 2)
     {
     act
     ("You do not have enough material, need to have two pieces in inventory.",
     FALSE, ch, 0, 0, TO_CHAR);
     return;
     }
     if (craft_obj3 == 0 && howmany == 3)
     {
     act
  ("You do not have enough material need to have three pieces in inventory.",
   FALSE, ch, 0, 0, TO_CHAR);
  return;
}


if (howmany == 2)
{
  if (craft_obj1->material != craft_obj2->material)
  {
    act("Mixing materials like that might be dangerous.", FALSE, ch, 0, 0,
        TO_CHAR);
    return;
  }
}

if (howmany == 3)
{
  if (craft_obj1->material != craft_obj2->material ||
      craft_obj1->material != craft_obj3->material)
  {
    act("Mixing materials like that might be dangerous.", FALSE, ch, 0, 0,
        TO_CHAR);
    return;
  }
}

for (i = 0; i <= MAXMATERIAL; i++)
{
  if (material_data[i].m_number == craft_obj1->material)
  {
    material_type = i;
  }
}


*/
/*        wizlog(56, "%s crafted.", GET_NAME(ch));

          if (howmany > 0)
          extract_obj(craft_obj1);
          if (howmany > 1)
          extract_obj(craft_obj2);
          if (howmany > 2)
          extract_obj(craft_obj3);
          obj = create_random_eq_new(ch, ch, slot, material_type);
          if (!obj)
          {
          act("&+yCrafting NOT COMPLETED CONTACT A GOD!&n", FALSE, ch, 0, 0,
          TO_CHAR);
          return;
          }
          obj_to_char(obj, ch);
//notch_skill(ch, SKILL_CRAFT, 30);
act("&+W$n crafted a $q&N.", TRUE, ch, obj, 0, TO_NOTVICT);
act("&+WYou crafted a $q&N", TRUE, ch, obj, 0, TO_CHAR);

return;
*/
}


void do_smith(P_char ch, char *argument, int cmd)
{

}


int chance_throw_potion(P_char ch, P_char victim)
{
  int      chance = 0;

  if (IS_PC(ch))
  {
    chance = GET_CHAR_SKILL(ch, SKILL_THROW_POTIONS);
  }
  else if (GET_PRIME_CLASS(ch, CLASS_ALCHEMIST))
  {
    chance = GET_LEVEL(ch) * 2;
  }

  if(number(1, 140) > GET_C_AGI(ch))
    chance = 1;

  return (int) chance;

}

bool throw_potion(P_char ch, P_obj potion, P_char victim, P_obj obj)
{
  int      i, bits, j, in_room;
  bool     equipped = FALSE;
  P_char   tch = NULL;
  P_char   temp = NULL;
  int      the_room;
  int      chance = 0;
  float      lag;


  if( !(chance = chance_throw_potion(ch, victim)) )
  {
    act("Well, trying might not hurt.", FALSE, ch, 0, 0, TO_CHAR);
    return FALSE;
  }

  if(!isname(GET_NAME(ch), potion->short_description) && !IS_TRUSTED(ch))
  {
    send_to_char("&nThrowing potions of unknown origin might be hazardous to your health... better not!&n\r\n", ch);
    return FALSE;
  }

  if (potion == ch->equipment[HOLD])
    equipped = TRUE;

  lag = get_property("alchemist.throwp.reorient.rounds", 1.75);

  if (IS_AFFECTED2(ch, AFF2_FLURRY))
  {
    lag *= 0.5;
  }
  else if (IS_AFFECTED(ch, AFF_HASTE))
  {
    lag *= 0.8;
  }

  CharWait(ch, (int) (get_property("alchemist.throwp.lag.rounds", 0.75) * PULSE_VIOLENCE) );
  set_short_affected_by(ch, SKILL_THROW_POTIONS, (int) (lag * PULSE_VIOLENCE));

  notch_skill(ch, SKILL_THROW_POTIONS, 7);

  if (victim)
  {

    if(chance == 1)
    {
      send_to_char("As you slip......\r\n", ch);
      if (equipped)
        unequip_char(ch, HOLD);
      obj_from_char(potion);
      send_to_char("&+YYou aim your throw a little too high, sending your potion flying across the room!\r\n&n", ch);
      act("$n slips as $e throws a $p, sending it bouncing along the ground!",
         TRUE, ch, potion, 0, TO_ROOM);
      extract_obj(potion);
      return FALSE;
    }

    if (number(0, chance))
    {
      victim = victim;
    }
    else
    {
      the_room = ch->in_room;
      for (tch = world[the_room].people; tch; tch = temp)
      {
        temp = tch->next_in_room;
        if (number(0, 1))
        {
          victim = temp;
          break;
        }
      }
      send_to_char("As you slip......\r\n", ch);
      if (!victim)
        victim = ch;

      if (victim == ch)
      {
        if (equipped)
          unequip_char(ch, HOLD);
        obj_from_char(potion);
        obj_to_room(potion, ch->in_room);
        send_to_char("&+YYou aim your throw a little too high, sending your potion flying across the room!\r\n&n", ch);
        act("$n slips as $e throws a $p, sending it bouncing along the ground!",
           TRUE, ch, potion, 0, TO_ROOM);
        return FALSE;
      }
    }
  }

  if (victim)
  {
    act("&+W$N&n &+Wpales&N &+Las $p &+Lthrown by&n&+m $n&n&+L hits $M&n &+Lin the face.&n",
       FALSE, ch, potion, victim, TO_NOTVICT);
    act("&+W$N's&n&+L face turns&+W pale &+Las $p &+Lhits $M dead on!&n ",
        FALSE, ch, potion, victim, TO_CHAR);
    act("&+LYour face &+Wpales&N&+L as $p &+Lthrown by&n&+m $n &n&+Lhits you dead on.&N",
       FALSE, ch, potion, victim, TO_VICT);
  }
  else if (obj)
  {
    act("&+LYou throw a potion at&n $p&n&+L...&n", FALSE, ch, obj, 0, TO_CHAR);
    act("&+W$n&n &+Lthrows a potion at&n $p&n&+L...&n", FALSE, ch, obj, 0, TO_ROOM);
  }
  else
  {
    send_to_char("Throw it at whom?\r\n", ch);
    return FALSE;
  }

  if (equipped)
  {
    unequip_char(ch, HOLD);
  }

  if (IS_ROOM( ch->in_room, ROOM_NO_MAGIC) && !IS_TRUSTED(ch))
  {
    send_to_char("Nothing seems to happen.\r\n", ch);
  }
  else if (victim && (victim != ch) && !IS_TRUSTED(ch) &&
      (IS_ROOM( ch->in_room, ROOM_SINGLE_FILE)) &&
      !AdjacentInRoom(ch, victim))
  {
    send_to_char("You can't get a clear line of sight!\r\n", ch);
  }
  else
  {
    for (i = 1; i < 4; i++)
    {
      if ((potion->value[i] >= 1) && (victim || obj))
      {
        j = potion->value[i];
        if ((j != -1) && (skills[j].spell_pointer != NULL))
        {
          if (IS_AGG_SPELL(j) && victim && (ch != victim))
          {
            if( IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_MINOR_INVIS) )
              appear(ch);
          }
          in_room = ch->in_room;

          if (potion->value[0] > GET_LEVEL(ch))
          {
            send_to_char("The magic is too complex for you to understand.\r\n", ch);
            return FALSE;
          }

          if (IS_SET(skills[j].targets, TAR_CHAR_ROOM) && victim &&
              !(IS_SET(skills[j].targets, TAR_CHAR_ROOM) && (ch == victim)))
            ((*skills[j].spell_pointer) ((int) potion->value[0], ch, 0,
              SPELL_TYPE_SPELL, victim, obj));
          else if (IS_SET(skills[j].targets, TAR_SELF_ONLY) && victim &&
              (victim == ch))
            ((*skills[j].spell_pointer) ((int) potion->value[0], ch, 0,
              SPELL_TYPE_SPELL, victim, obj));
          else
            if ((IS_SET(skills[j].targets, TAR_OBJ_ROOM) ||
                  IS_SET(skills[j].targets, TAR_OBJ_INV)) && obj)
              ((*skills[j].spell_pointer) ((int) potion->value[0], ch, 0,
                SPELL_TYPE_SPELL, victim, obj));
            else if (IS_SET(skills[j].targets, TAR_IGNORE))
              ((*skills[j].spell_pointer) ((int) potion->value[0], ch, 0,
                SPELL_TYPE_SPELL, victim, obj));



          /* best thing to do if victim dies is just extract the obj and quit out, since many
             spells kill the mud w/o a victim
             besides, what if the char IS the victim?  heh. */
          if( (victim && !char_in_list(victim)) || ((victim != ch) && !char_in_list(ch)) )
          {
            extract_obj(potion);
            return FALSE;
          }
          else if (IS_AGG_SPELL(j) && victim && (ch != victim))
          {
            if (affected_by_spell(victim, SPELL_SLEEP))
              affect_from_char(victim, SPELL_SLEEP);

            if (GET_STAT(victim) == STAT_SLEEPING)
            {
              send_to_char("Your rest is violently disturbed!\r\n", victim);
              act("Your potion disturbs $N's beauty sleep!\r\n", FALSE, ch, 0,
                  victim, TO_CHAR);
              act("$n's potion disturbs $N's beauty sleep!\r\n", FALSE, ch, 0,
                  victim, TO_NOTVICT);
              SET_POS(victim, GET_POS(victim) + STAT_NORMAL);
            }
          }
        }
      }
    }
  }

  if (GET_CHAR_SKILL(ch, SKILL_REMIX))
  {
    if ((victim && !obj && IS_NPC(victim)) &&
        number(1, 120) < GET_CHAR_SKILL(ch, SKILL_REMIX))
    {
      act("&+WYou quickly &+rremix&+W another potion!&n", FALSE, ch, 0, victim, TO_CHAR);
    }
    else
    {
      notch_skill(ch, SKILL_REMIX, 1);
      extract_obj(potion);
    }

  }
  else
    extract_obj(potion);

  return FALSE;
}


void do_throw_potion(P_char ch, char *argument, int cmd)
{
  P_obj    scroll, obj;
  int      i, j, in_room;
  bool     equipped;
  P_char   victim = NULL;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      bits = 0;

  equipped = FALSE;
  obj = 0;

  if( !IS_TRUSTED(ch) && affected_by_spell(ch, SKILL_THROW_POTIONS) )
  {
    act("Your arm is still too tired to throw another potion!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

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
  if (scroll->type != ITEM_POTION)
  {
    act("That's not a potion!", FALSE, ch, 0, 0,
        TO_CHAR);
    return;
  }

  if (*argument)
  {
    bits = generic_find(argument, FIND_CHAR_ROOM, ch, &victim, &obj);

    if (bits == 0)
    {
      if( IS_TRUSTED(ch) )
      {
        bits = generic_find(argument, FIND_OBJ_ROOM, ch, &victim, &obj);
      }
      else
      {
        bits = generic_find(argument, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &obj);
      }
    }

    if( bits == 0 )
      bits = generic_find(argument, FIND_OBJ_INV, ch, &victim, &obj);

    if (bits && obj && GET_ITEM_TYPE(obj) == ITEM_CORPSE)
    {
      send_to_char("Sorry, you can't throw potions at corpses.\r\n", ch);
      return;
    }

    if (bits == 0)
    {
      send_to_char("No such thing around.\r\n", ch);
      return;
    }
  }
  else if (GET_OPPONENT(ch))
  {
    victim = GET_OPPONENT(ch);
  }

  if (!victim && !obj)
  {
    send_to_char("No such thing around.\r\n", ch);
    return;
  }

  if (IS_CARRYING_N(ch) > CAN_CARRY_N(ch))
  {
    act
      ("&+LBut you carry &+Wtoo many&+L items in your inventory to handle potion throwing!&n",
       FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (scroll->value[0] > GET_LEVEL(ch))
  {
    send_to_char("The magic is too complex for you to understand.\r\n", ch);
    return;
  }


  throw_potion(ch, scroll, victim, obj);
}


void do_home(P_char ch, char *argument, int cmd)
{

  char     buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char     sign;
  int      cost, plat, in_room;

  cost = (int)(1000 * (int)get_property("price.home.plat", 50.000));

  if(IS_NPC(ch))
    return;

  if(IS_ROOM( ch->in_room, ROOM_SAFE))
  {
    send_to_char("Try somewhere else...\r\n", ch);
    return;
  }

  if (IS_ILLITHID(ch) && GET_LEVEL(ch) < 25)
  {
    send_to_char("Don't make me kill you.\n", ch);
    return;
  }

  if (!IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_TOWN))
  {
    send_to_char("You must be in a town to make it your home.\r\n", ch);
    return;
  }

  plat = cost/1000;

  if (GET_MONEY(ch) < cost)
  {
    snprintf(buf2, MAX_STRING_LENGTH, "&+RIt costs &+W%d platinum&+R to change your home, you need more money!&n\r\n", plat);
    act(buf2, TRUE, ch, 0, 0, TO_CHAR);
    return;
  } 
  else {
    wizlog(MINLVLIMMORTAL, "do_home(): Non-good/evil race attempting to home.  If a new racewar is back in the game, fix this.");
    send_to_char("Get out of here!", ch);
  }

  // Only way I can figure to tell if a town is evil or good for now.
  if (IS_RACEWAR_GOOD(ch) && IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_EVILHOME))
  {
    send_to_char("You can't really see yourself living in such an awful place as this.\n", ch);
    return;
  }
  else if (IS_RACEWAR_EVIL(ch) && IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_GOODHOME))
  {
    send_to_char("You can't really see yourself living in such an awful place as this.\n", ch);
    return;
  }

  SUB_MONEY(ch, cost, 0);

  snprintf(buf, MAX_STRING_LENGTH, "char %s home %d", J_NAME(ch),
      world[ch->in_room].number);
  do_setbit(ch, buf, CMD_SETHOME);

  snprintf(buf, MAX_STRING_LENGTH, "char %s orighome %d", J_NAME(ch),
      world[ch->in_room].number);
  do_setbit(ch, buf, CMD_SETHOME);

  snprintf(buf, MAX_STRING_LENGTH, "char %s origbp %d", J_NAME(ch),
      world[ch->in_room].number);
  do_setbit(ch, buf, CMD_SETHOME);

  send_to_char
    ("\r\n&+WThank you for the payment and welcome to your new birth home whenever you die you will return here.&n\r\n",
     ch);
  return;

}

void do_thrust(P_char ch, char *argument, int cmd)
{
}

void do_unthrust(P_char ch, char *argument, int cmd)
{
}
