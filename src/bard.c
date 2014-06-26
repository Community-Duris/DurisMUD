/*
 * ***************************************************************************
 * *  File: bard.c                                             Part of Duris *
 * *  Usage: bard's singing, etc.
 * *   Copyright  1994, 1995, 2003 - Markus Stenberg, Michal Rembiszewski
 * *                                                  and Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "sound.h"
#include "damage.h"
#include "utils.h"

/*
 * External variables
 */

extern P_char character_list;
extern P_room world;
extern const struct stat_data stat_factor[];
extern P_index mob_index;
extern bool has_skin_spell(P_char);

#define INSTRUMENT_OFFSET (FIRST_INSTRUMENT-184)
extern Skill skills[];
void     event_bardsong(P_char, P_char, P_obj, void *);
extern void disarm_char_events(P_char, event_func_type);

#define IS_SONG(song) (song >= FIRST_SONG && song <= LAST_SONG)
#define SONG_AGGRESSIVE           BIT_1
#define SONG_ALLIES               BIT_2

/*
 * Main code.. muhahahahaa
 */

int get_bard_pulse(P_char ch)
{
  return IS_AFFECTED(ch,
                     AFF_HASTE) ? PULSE_BARD_UPDATE - 10 : PULSE_BARD_UPDATE;
}

typedef void song_function(int, P_char, P_char, int);

void     bard_sleep(int, P_char, P_char, int);
void     bard_charm(int, P_char, P_char, int);
void     bard_harming(int, P_char, P_char, int);
void     bard_healing(int, P_char, P_char, int);
void     bard_revelation(int, P_char, P_char, int);
void     bard_forgetfulness(int, P_char, P_char, int);
void     bard_peace(int, P_char, P_char, int);
void     bard_calm(int, P_char, P_char, int);
void     bard_heroism(int, P_char, P_char, int);
void     bard_cowardice(int, P_char, P_char, int);
void     bard_chaos(int, P_char, P_char, int);
void     bard_storms(int, P_char, P_char, int);
void     bard_protection(int, P_char, P_char, int);
void     bard_flight(int, P_char, P_char, int);
void     bard_dragons(int, P_char, P_char, int);
void     bard_discord(int, P_char, P_char, int);
void     bard_harmony(int, P_char, P_char, int);
void     bard_drifting(int, P_char, P_char, int);
void     bard_dissonance(int, P_char, P_char, int);

struct song_description
{
  char    *name;
  song_function *funct;
  int      instrument;
  int      song;
  int      flags;
} songs[] =
{
  {
  "harmony", bard_harmony, INSTRUMENT_FLUTE, SONG_HARMONY, 0},
  {
  "discord", bard_discord, INSTRUMENT_FLUTE, SONG_DISCORD, 0},
  {
  "sleep", bard_sleep, INSTRUMENT_FLUTE, SONG_SLEEP, SONG_AGGRESSIVE},
  {
  "charming", bard_charm, INSTRUMENT_FLUTE, SONG_CHARMING, SONG_AGGRESSIVE},
  {
  "harming", bard_harming, INSTRUMENT_LYRE, SONG_HARMING, SONG_AGGRESSIVE},
  {
  "healing", bard_healing, INSTRUMENT_LYRE, SONG_HEALING, SONG_ALLIES},
  {
  "revelation", bard_revelation, INSTRUMENT_MANDOLIN, SONG_REVELATION, SONG_ALLIES},
  {
  "forgetfulness", bard_forgetfulness, INSTRUMENT_MANDOLIN,
      SONG_FORGETFULNESS, 0},
  {
  "peace", bard_peace, INSTRUMENT_HARP, SONG_PEACE, 0},
  {
  "calming", bard_calm, INSTRUMENT_HARP, SONG_CALMING, 0},
  {
  "heroism", bard_heroism, INSTRUMENT_DRUMS, SONG_HEROISM, SONG_ALLIES},
  {
  "cowardice", bard_cowardice, INSTRUMENT_DRUMS, SONG_COWARDICE,
      SONG_AGGRESSIVE},
  {
  "chaos", bard_chaos, INSTRUMENT_DRUMS, SONG_CHAOS, SONG_AGGRESSIVE},
  {
  "storms", bard_storms, INSTRUMENT_DRUMS, SONG_STORMS, SONG_AGGRESSIVE},
  {
  "protection", bard_protection, INSTRUMENT_HORN, SONG_PROTECTION,
      SONG_ALLIES},
  {
  "flight", bard_flight, INSTRUMENT_HORN, SONG_FLIGHT, 0},
  {
  "dragons", bard_dragons, INSTRUMENT_HORN, SONG_DRAGONS, SONG_ALLIES},
  {
  "drifting", bard_drifting, INSTRUMENT_DRUMS, SONG_DRIFTING, SONG_ALLIES},
  {
  "dissonance", bard_dissonance, INSTRUMENT_HORN, SONG_DISSONANCE, 0},
  {
  0}
};

struct echo_details
{
  int      song;
  int      room;
};

int SINGING(P_char ch)
{
  /*if(IS_NPC(ch))
    return 0;*/
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return 0;
  }

  if(!IS_AFFECTED3(ch, AFF3_SINGING))
  {
    return 0;
  }
  return 1;
}

void stop_singing(P_char ch)
{
  struct affected_type *af, *next_af;
  P_char   tch;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    logit(LOG_EXIT, "stop_singing in bard.c called without ch");
    raise(SIGSEGV);
  }

  for (af = ch->affected; af; af = next_af)
  {
    next_af = af->next;
    if(af &&
      af->bitvector3 == AFF3_SINGING)
      {
        affect_remove(ch, af);
      }
  }
  while (tch = get_linking_char(ch, LNK_SONG))
  {
    unlink_char(tch, ch, LNK_SONG);
  }
  if(get_scheduled(ch, event_bardsong))
  {
    disarm_char_events(ch, event_bardsong);
  }
}

void bard_aggro(P_char ch, P_char victim)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  int in_room;
  
  if(!(ch) ||
    !(victim) ||
    !IS_ALIVE(ch) ||
    !IS_ALIVE(victim))
  {
    return;
  }
  
  appear(ch);

  if(!CAN_SEE(ch, victim) ||
     ch->in_room != victim->in_room)
  {
    return;
  }
  
  if(IS_NPC(ch) &&
     !IS_PC_PET(ch) &&
     IS_NPC(victim) &&
     !IS_PC_PET(victim))
  {
    return;
  }
  
  if(IS_PC(victim) && !IS_FIGHTING(victim) && !IS_DESTROYING(victim) )
  {
    // justice_witness(victim, ch, CRIME_ATT_MURDER);
    // strcpy(Gbuf1, GET_NAME(victim));
    
    set_fighting(victim, ch);
  }
  else if(IS_NPC(victim) && !IS_FIGHTING(victim) && !IS_DESTROYING(victim) )
  {
    MobStartFight(ch, victim);
    remember(victim, ch);
  }
  else if(IS_NPC(victim))
  {
    remember(victim, ch);
  }
}

P_obj has_instrument(P_char ch)
{
  int      i;

  /*
   * okay... a bit of explanation on the OR with CAN_SEE_OBJ... A
   * proficient musician is able to play their instruments without
   * seeing them.  Note that if they have the instrument skill higher
   * then 40, they can always play it in the dark.  Considering how high
   * level a person has to be to get a skill to 40... this seems
   * reasonable
   */
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    logit(LOG_EXIT, "has_instrument in bard.c called without ch");
    raise(SIGSEGV);
  }

  for (i = WIELD; i <= HOLD; i++)
    if(ch->equipment[i] && (ch->equipment[i]->type == ITEM_INSTRUMENT) &&
       (CAN_SEE_OBJ(ch, ch->equipment[i]) ||
       (has_innate(ch, INNATE_BLINDSINGING) &&
       GET_CHAR_SKILL(ch, ch->equipment[i]->value[0] + INSTRUMENT_OFFSET) >= 70)))
    {
      return ch->equipment[i];
    }

  return NULL;
}

int bard_get_type(int skill)
{
  int      i;

  for (i = 0; songs[i].name; i++)
  {
    if(songs[i].song == skill)
      return songs[i].instrument;
  }

  return 0;
}

int bard_calc_chance(P_char ch, int song)
{
  P_obj    instrument;
  int      c;

  /*if(IS_NPC(ch))
    return 0;*/
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return 0;
  
  if(IS_TRUSTED(ch) ||
    IS_NPC(ch))
  {
    return 101;
  }
  
  if(!CAN_SING(ch))
  {
    return 0;
  }

  c = GET_CHAR_SKILL(ch, song);
  instrument = has_instrument(ch);
  if(!instrument)
  {
    return 0;
  }
  else if(IS_ARTIFACT(instrument))
  {
    c = 101;
  }
  else if(bard_get_type(song) == instrument->value[0] + INSTRUMENT_OFFSET)
  {
    c = MAX(c * 3 / 2,
            ((bard_get_type(song) == instrument->value[0] + INSTRUMENT_OFFSET)
             && (GET_LEVEL(ch) >=
                 instrument->value[3])) ? c * GET_CHAR_SKILL(ch,
                                                             instrument->
                                                             value[0] +
                                                             INSTRUMENT_OFFSET)
            / 2 : 0);
  }
  else
  {
    return 0;
  }
  // level out the chance to between 80 and 110% (extra 10 for +max stat)
  c = BOUNDED(80, c, 110);

  // modify chance by other 'distractions' the bard might be experiencing
  if(IS_CASTING(ch))
  {
    c = c * 85 / 100;           // 15% chance reduction for casting
  }
  else if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    c = c * 93 / 100;           // 7% chance reduction while fighting...
  }

  if(GET_LEVEL(ch) > 46)
  {
    c = (int) (c * 1.2);
  }
  if(GET_C_AGI(ch) > number(0, 70))
  {
    c = (int) (c * 1.3);
  }
  if(GET_C_DEX(ch) > number(0, 85))
  {
    c = (int) (c * 1.3);
  }
  if(GET_C_LUK(ch) > number(0, 100))
  {
    c = (int) (c * 1.1);
  }
  return IS_ARTIFACT(instrument) ? 101 : BOUNDED(80, c, 99);

  return 0;
}

int bard_song_level(P_char ch, int song)
{
  P_obj    instrument;
  int      c;
  double   i_factor = get_property("bard.instrumentFactor", 0.35);
  
  if(!ch)
  {
    logit(LOG_EXIT, "bard_song_level in bard.c called without ch");
    raise(SIGSEGV);
  }
  if(ch)
  {
    if(!SINGING(ch))
    {
      return 1;
    }
    if(!(instrument = has_instrument(ch)))
    {
      c = GET_LEVEL(ch) / 3;
    }
    else
    {
      c = (int) ((1. - i_factor) * GET_LEVEL(ch));
      if((bard_get_type(song) == instrument->value[0] + INSTRUMENT_OFFSET &&
           GET_LEVEL(ch) >= instrument->value[3]) || IS_ARTIFACT(instrument))
        c += (int) (i_factor * instrument->value[1]);
    }
    return c;
  }
  return 0;
}

int bard_saves(P_char ch, P_char victim, int song)
{
  int smod, save;
  
  if(!ch)
  {
    logit(LOG_EXIT, "bard_saves in bard.c called without ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  {
    if(IS_ALIVE(ch) ||
      !victim ||
      !IS_ALIVE(victim))
  
// Saves are bounded to 4. Here are some examples:
// Sept08 -Lucrot
// Level bard / Level instrument / Level victim / save
// 56/56/56/6 ~ bounded to 4
// 40/40/40/4
// 40/56/56/-3
// 15/30/56/-18
// 40/56/62/-6
    if(ch && victim)
    {
      smod = (int) MIN((((bard_song_level(ch, song)) / 15) + ((GET_LEVEL(ch) - GET_LEVEL(victim)) / 2)), 4);
      save = NewSaves(victim, SAVING_SPELL, smod);
      return save;
    }
  }
  else
  {
    return 0;
  }
  // if(ch && victim)
    // return NewSaves(victim, SAVING_SPELL,
                    // (((bard_song_level(ch, song) + GET_LEVEL(ch)) / 2) -
                     // GET_LEVEL(victim)) / 7);
  // else
    // return 0;
}

void do_bardsing(P_char ch, char *arg)
{
  P_char   foo;
  char     Gbuf4[MAX_STRING_LENGTH];
  
  if(!ch)
  {
    logit(LOG_EXIT, "do_bardsing in bard.c called without ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  {
    if(!IS_ALIVE(ch))
    {
      return;
    }
    if(!arg || !*arg)
    {
      send_to_char("Sing what?\r\n", ch);
      return;
    }
    arg = skip_spaces(arg);
    if(!arg || !*arg)
    {
      send_to_char("Sing what?\r\n", ch);
      return;
    }
    for (foo = world[ch->in_room].people; foo; foo = foo->next_in_room)
      if(foo != ch)
      {
        sprintf(Gbuf4, "$n sings %s '%s'", language_known(ch, foo),
                language_CRYPT(ch, foo, arg));
        act(Gbuf4, FALSE, ch, 0, foo, TO_VICT);
      }
    sprintf(Gbuf4, "You sing %s '%s'", language_known(ch, ch), arg);
    act(Gbuf4, FALSE, ch, 0, 0, TO_CHAR);
  }
}

void do_bardcheck_action(P_char ch, char *arg, int cmd)
{
  if(!ch)
  {
    logit(LOG_EXIT, "do_bardcheck_action in bard.c called without ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  {
    if(!IS_ALIVE(ch))
    {
      return;
    }
    if(/*IS_PC(ch) &&*/ GET_CLASS(ch, CLASS_BARD))
    {
      do_bardsing(ch, arg);
    }
    else
    {
      do_action(ch, arg, cmd);
    }
  }
}

void bard_drifting(int l, P_char ch, P_char victim, int song)
{
  int skill = GET_CHAR_SKILL(ch, SONG_DRIFTING);

  if(!(ch) || !IS_ALIVE(ch))
    return;

  if(skill > number(1, 300) || IS_TRUSTED(ch) )
    spell_group_teleport(l, ch, 0, 0, victim, 0);
}

/*
*void bard_healing(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);

  if(!(ch) ||
     !IS_ALIVE(ch))
        return;

  if(IS_NPC(ch))
  {
    empower += 100;
  }
  
  if(GET_SPEC(ch, CLASS_BARD, SPEC_MINSTREL))
  {
    if(IS_AFFECTED(victim, AFF_BLIND) &&
        GET_CHAR_SKILL(ch, SONG_HEALING) >= 90)
    {
      spell_cure_blind(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, NULL);
    }
    if(IS_AFFECTED2(victim, AFF2_POISONED) &&
        GET_CHAR_SKILL(ch, SONG_HEALING) >= 50)
    {
      spell_remove_poison(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, NULL);
    }
    if(GET_CHAR_SKILL(ch, SONG_HEALING) >= 70 &&
      (affected_by_spell(victim, SPELL_DISEASE) ||
      affected_by_spell(victim, SPELL_PLAGUE)))
    {
      spell_cure_disease(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, NULL);
    }
  }
  else if(GET_SPEC(ch, CLASS_BARD, SPEC_DISHARMONIST))
  {
    if(IS_AFFECTED2(ch, AFF2_SILENCED) &&
        GET_CHAR_SKILL(ch, SONG_HEALING) >= 90)
    {
      affect_from_char(ch, SPELL_SILENCE);
    }
  }
  
  if(is_linked_to(ch, victim, LNK_SONG))
  {
    return;
  }
  
  if(!affected_by_spell(victim, SONG_HEALING))
  {
    memset(&af, 0, sizeof(af));
    af.bitvector4 = AFF4_REGENERATION;
    af.type = SONG_HEALING;
    af.location = APPLY_HIT_REG;
    af.modifier = (int) (11 * l * get_property("song.bard.healing.mod", 1.000) + (empower / 10));

    linked_affect_to_char(victim, &af, ch, LNK_SONG);
  }
  
  if(!affected_by_spell(ch, SONG_HEALING))
  {
    memset(&af, 0, sizeof(af));
    af.bitvector4 = AFF4_REGENERATION;
    af.type = SONG_HEALING;
    af.location = APPLY_HIT_REG;
    af.modifier = (int) (11 * l * get_property("song.bard.healing.mod", 1.000) + (empower / 10));

    linked_affect_to_char(ch, &af, ch, LNK_SONG);
  }
}
*/

void bard_healing(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
 if(!affected_by_spell(victim, SONG_HEALING) ||
     (affected_by_spell(victim, SONG_HEALING) &&  (get_linked_char(victim, LNK_SNG_HEALING) == ch)))
  {
  int healed, old_hits = GET_HIT(ch);
  //healed = l * 3 * number(40, 80) / 100;
  //spell_heal(l, ch, 0, 0, victim, NULL);
  healed = GET_C_CHA(ch) / 3;
  healed = healed + GET_LEVEL(ch);
  
   act("&+WYour body feels restored by the power of $n's soothing song!", FALSE, ch, 0, victim, TO_VICT);
  if(ch == victim)
  {
    act("&+WYour body feels restored by the power of your soothing song!", FALSE, ch, 0, victim, TO_CHAR);
    heal(victim, ch, healed , GET_MAX_HIT(victim) - number(1, 4));
  }
  else
  {
   healed = (healed * .65);
   heal(victim, ch, healed , GET_MAX_HIT(victim) - number(1, 4));
  }


  update_pos(victim);
  

   if(GET_SPEC(ch, CLASS_BARD, SPEC_MINSTREL))
  {
    if(IS_AFFECTED(victim, AFF_BLIND) &&
        GET_CHAR_SKILL(ch, SONG_HEALING) >= 90)
    {
      spell_cure_blind(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, NULL);
    }
    if(IS_AFFECTED2(victim, AFF2_POISONED) &&
        GET_CHAR_SKILL(ch, SONG_HEALING) >= 50)
    {
      spell_remove_poison(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, NULL);
    }
    if(GET_CHAR_SKILL(ch, SONG_HEALING) >= 70 &&
      (affected_by_spell(victim, SPELL_DISEASE) ||
      affected_by_spell(victim, SPELL_PLAGUE)))
    {
      spell_cure_disease(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, NULL);
    }
  }
  else if(GET_SPEC(ch, CLASS_BARD, SPEC_DISHARMONIST))
  {
    if(IS_AFFECTED2(ch, AFF2_SILENCED) &&
        GET_CHAR_SKILL(ch, SONG_HEALING) >= 90)
    {
      affect_from_char(ch, SPELL_SILENCE);
    }
  }

     memset(&af, 0, sizeof(af));
    af.type = SONG_HEALING;
    af.duration = PULSE_VIOLENCE * 3;
    af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;


    linked_affect_to_char(victim, &af, ch, LNK_SNG_HEALING);

  }
  else
  {
   send_to_char("Your song has no power, as they are already affected by a healing song!\r\n", ch);
   return;
  }

}

void bard_charm(int l, P_char ch, P_char victim, int song)
{
  P_obj    tmp_obj;
  struct affected_type af;
  struct char_link_data *cld;
  int      i;
  struct follow_type *k;

  /* re-enable charm */
  /* return; *//* charm shouldn't be working .. */

  if( GET_MASTER(victim) || GET_MASTER(ch) )
    return;


  if(IS_PC(victim))
  return;
 /*
    if(GET_LEVEL(victim) > (GET_LEVEL(ch) - 5) && IS_PC(ch))
    return;

  if(GET_LEVEL(ch) < (GET_LEVEL(victim)) && IS_PC(ch)) 
    return;
 */
    if(GET_LEVEL(victim) > 50 && IS_PC(ch))
	return;

    if(GET_LEVEL(victim) > GET_LEVEL(ch) && IS_PC(ch))
	return;
    
  if(victim == ch)
    return;
  if(circle_follow(victim, ch))
    return;
  /*
  if(victim->player.m_class &
      (CLASS_SORCERER | CLASS_PSIONICIST | CLASS_CLERIC |
       CLASS_CONJURER | CLASS_WARLOCK | CLASS_ILLUSIONIST) && IS_PC(ch))
    return;
  */
  if(victim->player.m_class &
	(CLASS_WARLOCK | CLASS_ILLUSIONIST) && IS_PC(ch))
	return;

   if(bard_saves(ch, victim, song) && !IS_NPC(ch))
    return;

  if(mob_index[GET_RNUM(victim)].func.mob == shop_keeper || (mob_index[GET_RNUM(victim)].qst_func == shop_keeper))
    {
       return;
    }


  for (tmp_obj = victim->carrying; tmp_obj; tmp_obj = tmp_obj->next_content)
    if(IS_SET(tmp_obj->extra_flags, ITEM_NOCHARM))
      return;
  for (i = 0; (i < MAX_WEAR); i++)
    if(victim->equipment[i] &&
        IS_SET(victim->equipment[i]->extra_flags, ITEM_NOCHARM))
      return;



  if(count_pets(ch) >= 3)
    return;
  if(victim->following && (victim->following != ch))
    stop_follower(victim);
  if(!victim->following)
    add_follower(victim, ch);
  setup_pet(victim, ch, GET_C_INT(victim) ? 1240 / GET_C_INT(victim) : 4, 0);
  act("You stand enthralled by $n's charming song...",
      FALSE, ch, 0, victim, TO_VICT);
  if(IS_FIGHTING(victim))
    stop_fighting(victim);
  if(IS_DESTROYING(victim))
    stop_destroying(victim);
  StopMercifulAttackers(victim);
  if(IS_NPC(victim))
    victim->only.npc->aggro_flags = 0;
}

void song_broken(struct char_link_data *cld)
{
  switch (cld->affect->type)
  {
  case SONG_SLEEP:
    REMOVE_BIT(cld->linking->specials.affected_by, AFF_SLEEP);
    do_wake(cld->linking, NULL, CMD_WAKE);
    do_alert(cld->linking, NULL, CMD_ALERT);
    break;
  }
}

void bard_sleep(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
  P_obj    tmp_obj;
  int      i, level;

  if(IS_TRUSTED(victim))
    return;
    
  if(affected_by_spell(victim, song))
    return;
  
  if(IS_AFFECTED(ch, AFF_INVISIBLE))
    appear(ch);
  
  for (tmp_obj = victim->carrying; tmp_obj; tmp_obj = tmp_obj->next_content)
    if(IS_SET(tmp_obj->extra_flags, ITEM_NOSLEEP))
    {
      if(IS_PC(victim))
        send_to_char("&+yYou stifle a yawn.\r\n", victim);
      return;
    }
    
  for (i = 0; i < MAX_WEAR; i++)
    if(victim->equipment[i] &&
        IS_SET(victim->equipment[i]->extra_flags, ITEM_NOSLEEP))
    {
      if(IS_PC(victim))
        send_to_char("You stifle a yawn.\r\n", victim);
      return;
    }

  if(IS_SHOPKEEPER(victim) ||
     IS_GREATER_RACE(victim) ||
     IS_ELITE(victim))
  {
    bard_aggro(victim, ch);
    return;
  }

  if(GET_LEVEL(victim) > 45)
  {
   if(number(1, 280) > GET_C_CHA(ch))
    {
    bard_aggro(victim, ch);
    act("&+B$n stifles a yawn, but manages stay awake!", TRUE, victim, 0, 0, TO_ROOM);
    return;
    }
  }

  if(bard_saves(ch, victim, song))
    return;

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
    act("&+G$n falls sleep.", TRUE, victim, 0, 0, TO_ROOM);
    SET_POS(victim, GET_POS(victim) + STAT_SLEEPING);
  }
 
  affect_join(victim, &af, FALSE, FALSE); 
  StopMercifulAttackers(victim);
}

void bard_calm(int l, P_char ch, P_char victim, int song)
{
  if(victim->specials.fighting)
  {
    if(!bard_saves(ch, victim, song) ||
       IS_TRUSTED(ch))
    {
      stop_fighting(victim);
      if( IS_DESTROYING(victim) )
        stop_destroying(victim);
      clearMemory(victim);
      send_to_char("A sense of calm comes upon you.\r\n", victim);
    }
  }
}

void bard_revelation(int l, P_char ch, P_char victim, int song)
{
  bool flag = FALSE;
  struct affected_type af;
  int x = GET_LEVEL(ch);
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);

  if(IS_NPC(ch))
    empower += 100;

//  if(affected_by_spell(victim, song))
//    return;
  /*
   * assuming song at 20th, min l is 6, max is 25 + instrument (at 50th)
   */
  if((l > 7) &&
    !IS_AFFECTED2(victim, AFF2_DETECT_MAGIC))
  {
    bzero(&af, sizeof(af));
    af.type = song;
    af.duration = x;
    af.bitvector2 = AFF2_DETECT_MAGIC;
    affect_to_char(victim, &af);
    flag = TRUE;
    act("&+mMagical energies are now visible.", FALSE, victim, 0, 0, TO_CHAR);
  }
  if(l > 13)
  {  
    if(!IS_AFFECTED2(victim, AFF2_DETECT_GOOD))
    {
      bzero(&af, sizeof(af));
      af.type = song;
      af.duration = x;
      af.bitvector2 = AFF2_DETECT_GOOD;
      affect_to_char(victim, &af);
      flag = TRUE;
    }
    if(!IS_AFFECTED2(victim, AFF2_DETECT_EVIL))
    {
      bzero(&af, sizeof(af));
      af.type = song;
      af.duration = x;
      af.bitvector2 = AFF2_DETECT_EVIL;
      affect_to_char(victim, &af);
      flag = TRUE;
    }
  }
  if((l > 30) &&
  !IS_AFFECTED(victim, AFF_DETECT_INVISIBLE))
  {
    bzero(&af, sizeof(af));
    af.type = song;
    af.duration = x;
    af.bitvector = AFF_DETECT_INVISIBLE;
    affect_to_char(victim, &af);
    flag = TRUE;
    act("&+cYour eyes start to tingle.", FALSE, victim, 0, 0, TO_CHAR);
  }
  if((l > 30) &&
    !IS_AFFECTED(victim, AFF_SENSE_LIFE))
  {
    bzero(&af, sizeof(af));
    af.type = song;
    af.duration = x;
    af.bitvector = AFF_SENSE_LIFE;
    affect_to_char(victim, &af);
    flag = TRUE;
  }
  if((l > 21) &&
    !IS_AFFECTED(victim, AFF_FARSEE) &&
    number(0, 2))
  {
    bzero(&af, sizeof(af));
    af.type = song;
    af.duration = x;
    af.bitvector = AFF_FARSEE;
    affect_to_char(victim, &af);
    flag = TRUE;
    act("&+yYour pupils contract and expand. Your vision is enhanced.", FALSE,
      victim, 0, 0, TO_CHAR);
  }
}

void bard_harming(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
  struct damage_messages messages = { 0, 0, 0, 0, 0, 0, 0 };
  int dam;
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);

  if(!ch) // Hrm something amiss... Nov08 -Lucrot
  {
    logit(LOG_EXIT, "bard_harming called in bard.c with no ch");
    raise(SIGSEGV);
  }
  
  if(ch &&
     victim) // Just making sure...
  {
    if(!IS_ALIVE(ch) ||
      !IS_ALIVE(victim))
    {
      return;
    }
    
    if(IS_NPC(ch))
    {
      empower += 100;
    }
    
    if(resists_spell(ch, victim)) // Added. Nov08 -Lucrot
    {
      return;
    }
    
    dam = (int) (l * 3 + empower / 4 + number(-4, 4)); // Adjusted. Nov08 -Lucrot
    dam = (int) dam * get_property("song.bard.harming.mod", 1.000);
    if(spell_damage(ch, victim, dam, SPLDAM_SOUND,
       SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages) != DAM_NONEDEAD)
    {
      return;
    }

    if(GET_LEVEL(ch) > 40 &&
      !NewSaves(victim, SAVING_SPELL, 0) &&
      !IS_AFFECTED(victim, AFF_BLIND) &&
      !EYELESS(victim))
    {
      blind(ch, victim, (int) (l / 12 * WAIT_SEC));
    }
    
    if((GET_LEVEL(ch) > 50) &&
      (!IS_AFFECTED2(victim, AFF2_POISONED)))
    {
      spell_poison(l, ch, 0, 0, victim, 0);
    }
    
    if(GET_SPEC(ch, CLASS_BARD, SPEC_MINSTREL))
    {
      if((GET_CHAR_SKILL(ch, SONG_HARMING) >= 70) &&
        !affected_by_spell(victim, SPELL_WITHER)) 
      {
        spell_wither(l , ch, 0, 0, victim, 0);
      }
    }
  }
}

void bard_flight(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);

  if(IS_NPC(ch))
    empower += 100;

  if(is_linked_to(ch, victim, LNK_SONG))
    return;
    
  if(!IS_AFFECTED(victim, AFF_FLY))
  {
    bzero(&af, sizeof(af));
    af.type = song;
    af.duration = l;
    af.bitvector = AFF_FLY;
    affect_to_char(victim, &af);
    act("&+WYou fly through the air, free as a &+ybird!",
        FALSE, victim, 0, 0, TO_CHAR);
    act("&+W$n flies through the air, free as a &+ybird!",
        TRUE, victim, 0, 0, TO_ROOM);
  }
  
  if(!IS_AFFECTED(ch, AFF_FLY))
  {
    bzero(&af, sizeof(af));
    af.type = song;
    af.duration = l;
    af.bitvector = AFF_FLY;
    affect_to_char(ch, &af);
    act("&+WYou fly through the air, free as a &+ybird!",
        FALSE, ch, 0, 0, TO_CHAR);
    act("&+W$n flies through the air, free as a &+ybird!",
        TRUE, ch, 0, 0, TO_ROOM);
  }

  memset(&af, 0, sizeof(af));
  af.type = song;
  af.location = APPLY_MOVE_REG;
  af.modifier = (int)(l / 3 + (empower / 10));

  linked_affect_to_char(victim, &af, ch, LNK_SONG);
  linked_affect_to_char(ch, &af, ch, LNK_SONG);
}

void bard_protection(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);

  if(IS_NPC(ch))
	empower += 100;

  if(GET_LEVEL(ch) >= 51 &&
    !IS_AFFECTED2(victim, AFF2_GLOBE))
  {
    //if(ch == victim)
      spell_globe(l, ch, 0, 0, victim, NULL);
    //else if(!number(0, 9))
      //spell_globe(l, ch, 0, 0, victim, NULL);
  }
  else if(GET_LEVEL(ch) >= 31 &&
          !IS_AFFECTED(victim, AFF_MINOR_GLOBE))
              spell_minor_globe(l, ch, 0, 0, victim, NULL);
  
  if(GET_LEVEL(ch) >= 46 &&
     !has_skin_spell(victim))
      if (IS_UNDEAD(victim) || IS_ANGEL(ch))
	spell_prot_undead(l, ch, 0, 0, victim, NULL);
      else
	spell_stone_skin(l, ch, 0, 0, victim, NULL);

  if(!affected_by_spell(victim, song))
  {
    memset(&af, 0, sizeof(af));
    af.type = song;
    af.location = APPLY_ARMOR;
    af.modifier = - l - (empower / 2);
    linked_affect_to_char(victim, &af, ch, LNK_SONG);
    send_to_char("&+WBands of AMAZINGLY strong armor wrap around you.\r\n", victim);
  }
}

void bard_heroism(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);

  if(IS_NPC(ch))
	empower += 100;

  if(GET_LEVEL(ch) >= 21 &&
    !IS_AFFECTED(victim, AFF_HASTE))
      spell_haste(l, ch, 0, 0, victim, NULL);

  if(!affected_by_spell(victim, SONG_HEROISM))
  {
    memset(&af, 0, sizeof(af));
    af.type = SONG_HEROISM;
    af.location = APPLY_COMBAT_PULSE;
    af.modifier = -1;
    linked_affect_to_char(victim, &af, ch, LNK_SONG);
    af.location = APPLY_SPELL_PULSE;
    af.modifier = -1;
    linked_affect_to_char(victim, &af, ch, LNK_SONG);
    if(IS_MELEE_CLASS(victim))
      send_to_char("&+yYour combat maneuvers seem faster...\r\n", victim);
    else
      send_to_char("&+cYour spell casting seems faster...\r\n", victim);
  }

  if(!affected_by_spell(victim, SONG_HEROISM))
  {
    memset(&af, 0, sizeof(af));
    af.duration = l / 3;
    af.type = SONG_HEROISM;
    af.location = APPLY_DAMROLL;
    af.modifier = (l / 10) + (empower / 15);
    affect_to_char(victim, &af);

    af.location = APPLY_HITROLL;
    af.modifier = (l / 10) + (empower / 15);
    affect_to_char(victim, &af);
    send_to_char("&+WA sense of &+yheroism &+Wgrows in your &+rheart.\r\n&N", victim);
  }
  
  if(!affected_by_spell(ch, SONG_HEROISM))
  {
    memset(&af, 0, sizeof(af));
    af.type = SONG_HEROISM;
    af.location = APPLY_COMBAT_PULSE;
    af.modifier = -1 * l / 20;
    linked_affect_to_char(ch, &af, ch, LNK_SONG);
    af.location = APPLY_SPELL_PULSE;
    af.modifier = -1 * l / 20;
    linked_affect_to_char(ch, &af, ch, LNK_SONG);
    if(IS_MELEE_CLASS(ch))
      send_to_char("&+yYour combat maneuvers seem faster...\r\n", victim);
    else
      send_to_char("&+cYour spell casting seems faster...\r\n", victim);
  }

  if(!affected_by_spell(ch, SONG_HEROISM))
  {
    memset(&af, 0, sizeof(af));
    af.duration = l / 3;
    af.type = SONG_HEROISM;
    af.location = APPLY_DAMROLL;
    af.modifier = (l / 10) + (empower / 15);
    affect_to_char(ch, &af);

    af.location = APPLY_HITROLL;
    af.modifier = (l / 10) + (empower / 15);
    affect_to_char(ch, &af);
    send_to_char("&+WA sense of &+yheroism &+Wgrows in your &+rheart.\r\n&N", victim);
  }
}

void bard_cowardice(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);

  if ((number(0, 100) < (int)get_property("song.bard.cowardice.flee.chance", 10.000)) &&
      (number(0, 100) < GET_CHAR_SKILL(ch, SONG_COWARDICE)) &&
      ((GET_LEVEL(ch)+MIN(5, (empower/20))) >= GET_LEVEL(victim)) &&
      !bard_saves(ch, victim, song) &&
      !IS_TRUSTED(victim) &&
      !fear_check(victim))
  {
    send_to_char("You succumb to the terror of the voice.\n", victim);
    do_flee(victim, 0, 0);
  }
  
  if(affected_by_spell(victim, song))
    return;

  bzero(&af, sizeof(af));
  af.type = song;
  af.modifier = 0 - MAX(2, (l / 8) + (empower / 10));
  af.location = APPLY_HITROLL;
  linked_affect_to_char(victim, &af, ch, LNK_SONG);
  af.modifier = 0 - MAX(2, (l / 8) + (empower / 10));
  af.location = APPLY_DAMROLL;
  linked_affect_to_char(victim, &af, ch, LNK_SONG);
  send_to_char
    ("&+rA sense of terrible &+Lweakness &+rfills you!\r\n", victim);
    act("$N looks visibly shakened!", TRUE, ch, 0, victim, TO_ROOM);
}

void bard_forgetfulness(int l, P_char ch, P_char victim, int song)
{
  if(bard_saves(ch, victim, song) &&
     !IS_TRUSTED(ch))
        return;
  clearMemory(victim);
}

void bard_peace(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;

  if(affected_by_spell(victim, song))
    return;
  if(bard_saves(ch, victim, song) &&
     !IS_TRUSTED(ch))
        return;
        
  bzero(&af, sizeof(af));
  af.type = song;
  af.duration = 1;
  affect_to_char(victim, &af);
  send_to_char("A sense of serenity and peace fills your heart.\r\n", victim);
}

void bard_harmony(int l, P_char ch, P_char victim, int song)
{
  if(ch != victim)
    if(!grouped(ch, victim))
      spell_dispel_evil((int) l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
}

void bard_discord(int l, P_char ch, P_char victim, int song)
{
  if(ch != victim)
    if(!grouped(ch, victim))
      spell_dispel_good((int) l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
}

void bard_storms(int l, P_char ch, P_char victim, int song)
{
  int num_strikes = 0, i = 0;
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);


  if(IS_NPC(ch))
	empower += 100;

  for (i = 0; i < l / 6; i++)
  {
    if(number(0, 2))
      break;
    num_strikes++;
  }
  while (--num_strikes >= 0)
  {
    spell_lightning_bolt(l * 2, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    if(!char_in_list(victim))
      return;
  }
  if(GET_SPEC(ch, CLASS_BARD, SPEC_MINSTREL))
  {
    if(GET_CHAR_SKILL(ch, SONG_STORMS) >= 70 )
    {
      if(!number(0, 4))
        spell_cyclone(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, 0);
    }
  }
  if((empower > 90) && (!number(0, 4)))
    spell_call_lightning(GET_LEVEL(ch), ch, victim, 0);
}

void bard_chaos(int l, P_char ch, P_char victim, int song)
{
  int room = ch->in_room, random;

  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(victim) ||
     !IS_ALIVE(ch))
  {
    return;
  }

  if(GET_SPEC(ch, CLASS_BARD, SPEC_MINSTREL))
  {
    if(GET_CHAR_SKILL(ch, SONG_CHAOS) >= 70 )
    {
      if(!number(0, 2))
      {
        random = number(0, 7);

        if(random == 0)
        {
          spell_fireball(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(random == 1)
        {
          spell_chill_touch(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(random == 2)
        {
          spell_lightning_bolt(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(random == 3)
        {
          spell_dispel_magic(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(random == 4)
        {
          spell_earthquake(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(random == 5)
        {
          spell_poison(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
          spell_disease(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(random == 6)
        {
          spell_curse(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
        }
        if(random == 7)
        {
          spell_wither(l, ch, 0, SPELL_TYPE_SPELL, victim, 0);
        }
      }
    }
  }

  if(is_char_in_room(victim, room) &&
     spell_damage(ch, victim, dice(5, 20), SPLDAM_SOUND, SPLDAM_NODEFLECT, 0) != DAM_NONEDEAD)
      return;

  if(!bard_saves(ch, victim, song) &&
     number(0, 3) == 3)
  {
    spell_teleport(30, ch, 0, 0, victim, 0);
  }
}

void bard_dragons(int l, P_char ch, P_char victim, int song)
{
  struct affected_type af;
  int x = GET_LEVEL(ch);
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);

  if(!(ch) ||
     !IS_ALIVE(victim) ||
     !IS_ALIVE(ch))
        return;
  
  if(IS_NPC(ch))
  {
    empower += 100;
  }

  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_FIRE))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_FIRE;
    af.duration = (int) (x / 3);
    af.bitvector = AFF_PROT_FIRE;
    affect_to_char(victim, &af);
    send_to_char("&+RYou feel protected from the fire!\r\n", victim);
  }
  
  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_COLD) &&
    (l > 17) &&
    (number(0, 3)))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_COLD;
    af.duration = (int) (x / 3);
    af.bitvector2 = AFF2_PROT_COLD;
    affect_to_char(victim, &af);
    send_to_char("&+WYou feel protected from the cold!\r\n", victim);
  }
  
  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_GAS) &&
    (l > 19) && 
    (number(0, 3)))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_GAS;
    af.duration = (int) (x / 3);
    af.bitvector2 = AFF2_PROT_GAS;
    affect_to_char(victim, &af);
    send_to_char("&+GYou feel protected from the poisonous gasses!\r\n", victim);
  }
  
  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_ACID) &&
    (l > 21) &&
    (number(0, 3)))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_ACID;
    af.duration = (int) (x / 3);
    af.bitvector2 = AFF2_PROT_ACID;
    affect_to_char(victim, &af);
    send_to_char("&+MYou feel protected from acid!\r\n", victim); 
  }
  
  if(!affected_by_spell(victim, SPELL_PROTECT_FROM_LIGHTNING) &&
    (l > 23) &&
    (number(0, 3)))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_PROTECT_FROM_LIGHTNING;
    af.duration = (int) (x / 3);
    af.bitvector2 = AFF2_PROT_LIGHTNING;
    affect_to_char(victim, &af);
    send_to_char("&+CYou feel protected from the lightning!\r\n", victim);
  }

  if(!affected_by_spell(victim, SONG_DRAGONS))
  {
    memset(&af, 0, sizeof(af));
    af.type = song;
    linked_affect_to_char(victim, &af, ch, LNK_SONG);
  }
  
  if(!affected_by_spell(ch, SONG_DRAGONS))
  {
    memset(&af, 0, sizeof(af));
    af.type = song;
    linked_affect_to_char(ch, &af, ch, LNK_SONG);
  }
} 

// The song has two affects. The first breaks pet links and the
// second forces other bards to stop singing.

void bard_dissonance(int l, P_char ch, P_char victim, int song)
{
  P_char   t, t_next;
  int empower = GET_CHAR_SKILL(ch, SKILL_EMPOWER_SONG);
  int in_room, c_roll, t_roll;

  if(!(ch) ||
    !IS_ALIVE(ch))
  {
    return;
  }

  c_roll = empower / 3 + GET_LEVEL(ch) + GET_C_CHA(ch) / 2 + GET_C_LUK(ch) / 5;
  
  for (t = world[ch->in_room].people; t; t = t_next)
  {
    t_next = t->next_in_room;
    
    if(!should_area_hit(ch, t))
    {
      continue;
    }
    
    if(IS_TRUSTED(t))
    {
      continue;
    }
    
    if(GET_SPEC(ch, CLASS_BARD, SPEC_DISHARMONIST))
    {
      c_roll += (int) (c_roll * 1.20);
    }
    
    if(!grouped(ch, t) &&
       GET_CLASS(t, CLASS_BARD) &&
       IS_AFFECTED3(t, AFF3_SINGING))
    {
      t_roll =  (GET_CHAR_SKILL(t, SKILL_EMPOWER_SONG) / 3) +
                GET_LEVEL(t) +
                (GET_C_CHA(t) / 2) +
                (GET_C_LUK(t) / 5);
               
  
      if(c_roll + number(0, 50) > t_roll + number(0, 50))
      {

        act("You have &+ym&+Ya&+yn&+Yg&+yl&+Ye&+yd&n $N's singing!",
          TRUE, ch, 0, t, TO_CHAR);        
        act("$n's &+Chorn blasting&n &+ym&+Ya&+yn&+Yg&+yl&+Ye&+ys&n your song...",
          TRUE, ch, 0, t, TO_VICT);
        act("$N's &+Csong&n is &+ym&+Ya&+yn&+Yg&+yl&+Ye&+yd&n!",
          TRUE, ch, 0, t, TO_NOTVICT);

        CharWait(t, PULSE_VIOLENCE);
          
        stop_singing(t);
        break;
      }
    }
    
    if(!grouped(ch, t) &&
       IS_NPC(t) &&
       IS_AFFECTED3(ch, AFF3_SINGING) &&
       get_linked_char(t, LNK_PET) &&
       get_linked_char(t, LNK_PET)->in_room == ch->in_room &&
       c_roll > number(1, 2000))
    {
      act("$N &+Wshudders and quivers&n for a moment...",
        TRUE, ch, 0, t, TO_ROOM);
      act("$N &+Wshudders and quivers&n for a moment.",
        TRUE, ch, 0, t, TO_CHAR); 
        
      clear_all_links(t);
    }
  }
}

struct song_text
{
  int      num; 
  char     tochar[100];
  char     toroom[100];
};

static struct song_text songwords[] = {
  {SONG_DISCORD,
   "You sing a song, cursing all that is good.",
   "$n plays a song, cursing all that is good."},
  {SONG_HARMONY,
   "You play a song to punish all that is wicked.",
   "$n plays a song, punishing all that is wicked."},
  {SONG_CHARMING,
   "You play a song the Pied Piper would have envied.",
   "$n tries to charm $s listeners with $s song."},
  {SONG_SLEEP,
   "You sing a _VERY_ boring song, droning on endlessly..",
   "$n keeps singing a song of a _VERY_ boring nature."},
  {SONG_CALMING,
   "You sing a soothing tune to everyone in the room.",
   "$n plays a peaceful, soothing tune."},
  {SONG_HEALING,
   "You sing a song to heal all wounds.",
   "$n sings a song so well you feel your pain and suffering ebbing away."},
  {SONG_REVELATION,
   "&+WYou play a song to make you more aware of what is around you.&n",
   "&+W$n keeps singing of uncovered secrets..&n"},
  {SONG_HARMING,
   "You play a discordant song, one so bad it actually hurts people to listen to it.",
   "$n sings so badly it makes you hurt all over."},
  {SONG_FLIGHT,
   "You sing a song to lift the spirits high.",
   "$n plays a song to lift $s spirit off the ground."},
  {SONG_PROTECTION,
   "You sing a song to protect you from the world.",
   "$n sings a song to protect $m from the world."},
  {SONG_HEROISM,
   "You sing a song that makes your heart swell with pride.",
   "$n sings a song to make your heart swell with pride."},
  {SONG_COWARDICE,
   "You sing an absolutely terrifying song. It even makes you afraid.",
   "$n sings an especially frightening song."},
  {SONG_FORGETFULNESS,
   "You sing a song, digressing, hoping that it will take people's mind off of other things.",
   "$n sings a song, and you can't quite keep your mind on what you're doing."},
  {SONG_PEACE,
   "You play a song that makes you feel perfectly at peace.",
   "$n sings a song to make everyone at peace with the world."},
  {SONG_CHAOS,
   "You play a song that boggles the senses.",
   "$n sings a song that disorients you and boggles your senses."},
  {SONG_DRAGONS,
   "You play a song to protect your friends from &+rdragons.",
   "$n sings a song that makes you feel safe from &+rdragons."},
  {SONG_STORMS,
   "You play a song to call the storms.",
   "$n sings a song to summon the storms."},
  {SONG_DRIFTING,
   "With every beat of the drum, you feel a strange tugging.",
   "With every beat of $n's drum, the air seems to ripple a bit."},
  {SONG_DISSONANCE,
   "Your horn blares and radiates a most disharmonist noise.",
   "$n's horn blares and emits a most disharmonist noise."},
  {0}
};

void sing_verses(P_char ch, int song)
{
  int      i;

  if(!(ch))
  {
    logit(LOG_EXIT, "sing_verses called in bard.c without ch");
    raise(SIGSEGV);
  }
  if(ch &&
    IS_ALIVE(ch))
  {
    for (i = 0; songwords[i].num; i++)
    {
      if(songwords[i].num == song)
      {
        act(songwords[i].tochar, FALSE, ch, 0, 0, TO_CHAR);
        act(songwords[i].toroom, FALSE, ch, 0, 0, TO_ROOM);
        break;
      }
    }
  }
}

void event_echosong(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char   victim2;
  int i = 0, songLevel = 0, room = 0, song = 0;
  struct song_description *songDescrip = 0;
  struct echo_details *echoDetails = 0;
  
  if(!(ch))
  {
    logit(LOG_EXIT, "event_echosong called in bard.c without ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  {
    // get the arguments of the echo
    echoDetails = (struct echo_details *) data;

    // get the room the song was sung in
    room = echoDetails->room;

    // show a message to the room for the echo
    char     echoText[100];

    sprintf(echoText, "&+LYou hear the faint echo of music...\r\n", GET_NAME(ch));
    send_to_room(echoText, room);

    // get the song
    song = echoDetails->song;

    // get a reduced song level for the application of the echo
    songLevel = bard_song_level(ch, song) / 2;

    // 1 is the minimum song level
    if(songLevel <= 0)
    {
      songLevel = 1;
    }
    // get the song information
    for (i = 0; songs[i].name; i++)
      if(songs[i].song == song)
      {
        songDescrip = &songs[i];
        break;
      }
    // for each person in the room
    for (victim = world[room].people; victim; victim = victim2)
    {
      victim2 = victim->next_in_room;

      // immortals are not effected
      if(IS_TRUSTED(victim))
      {
        continue;
      }
      if((ch == victim &&
        !(songDescrip->flags & SONG_AGGRESSIVE)) ||
        (grouped(ch, victim) &&
        !(songDescrip->flags & SONG_AGGRESSIVE)) ||
        (ch != victim && !grouped(ch, victim) &&
        !(songDescrip->flags & SONG_ALLIES)))
      {
        if((songDescrip->flags & SONG_AGGRESSIVE) &&
          bard_saves(ch, victim, song) &&
          IS_ALIVE(ch) &&
          victim &&
          IS_ALIVE(victim))
        {
          bard_aggro(victim, ch);
        }
        
        if(is_char_in_room(victim, room) &&
          IS_ALIVE(ch) &&
          victim &&
          IS_ALIVE(victim))
        {
          (songDescrip->funct) (songLevel, ch, victim, song);
        }
      }
    }
  }
}

void event_bardsong(P_char ch, P_char victim, P_obj obj, void *data)
{
  int    echoChance = 0,song, l, room, i, terrainType = SECT_INSIDE;
  P_obj  instrument = NULL;
  struct affected_type *af, *af2;
  struct char_link_data *cld, *next_cld;
  struct echo_details echoDetails;
  struct song_description *sd;
  P_char tch, next;

  if( !(ch) || !IS_ALIVE(ch) )
    return;

  song = *((int *) data);

  if(!SINGING(ch))
    return;

  for (cld = ch->linked; cld; cld = next_cld)
  {
    next_cld = cld->next_linked;
    if(cld->type == LNK_SONG && cld->affect &&
        (cld->affect->type != song || ch->in_room != cld->linking->in_room))
    {
      affect_remove(cld->linking, cld->affect);
    }
  }
  if(!CAN_SING(ch))
  {
    stop_singing(ch);
    return;
  }
  if(number(1, 90) > bard_calc_chance(ch, song))
  {
    act("Uh oh.. how did the song go, anyway?", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stutters in $s song, and falls silent.", FALSE, ch, 0, 0,
        TO_ROOM);
    set_short_affected_by(ch, FIRST_INSTRUMENT,  PULSE_VIOLENCE);
    do_action(ch, 0, CMD_BLUSH);
    stop_singing(ch);
    return;
  }

  if(CHAR_IN_NO_MAGIC_ROOM(ch))
  {
    send_to_char("&+BThe power of song has no effect here.\r\n", ch);
    stop_singing(ch);
    return;
  }

  sing_verses(ch, song);

  l = bard_song_level(ch, song) + number(-2, 2);

  if(l <= 0)
  {
    l = 1;
  }
  for (i = 0; songs[i].name; i++)
  {
    if(songs[i].song == song)
    {
      sd = &songs[i];
      break;
    }
  }
  room = ch->in_room;

  for (tch = world[room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if(IS_TRUSTED(tch))
    {
      continue;
    }
    
    if((ch == tch &&
      !(sd->flags & SONG_AGGRESSIVE)) ||
      (grouped(ch, tch) && !(sd->flags & SONG_AGGRESSIVE)) ||
      (ch != tch &&
      !grouped(ch, tch) &&
      !(sd->flags & SONG_ALLIES)))
    {
      if((sd->flags & SONG_AGGRESSIVE) &&
        bard_saves(ch, tch, song))
      {
        if(!IS_FIGHTING(tch))
        {
          bard_aggro(tch, ch);
        }
      }
      if(is_char_in_room(tch, room))
      {
        (sd->funct) (l, ch, tch, song);
      }
    }
  }

  notch_skill(ch, song, 50);
  if((instrument = has_instrument(ch)))
  {
    if(bard_get_type(song) == instrument->value[0] + INSTRUMENT_OFFSET)
      notch_skill(ch, instrument->value[0] + INSTRUMENT_OFFSET, 50);
  }
  for (af = ch->affected; af; af = af2)
  {
    af2 = af->next;
    if(af->bitvector3 == AFF3_SINGING && af->duration == 1)
    {
      act("&+BYou finish your song.", FALSE, ch, 0, 0, TO_CHAR);
      act("&+B$n finishes $s song.", FALSE, ch, 0, 0, TO_ROOM);
      affect_remove(ch, af);
      return;
    }
  }

  add_event(event_bardsong, get_bard_pulse(ch), ch, 0, 0, 0, &song, sizeof(song));
  terrainType = world[ch->in_room].sector_type;

  // cannot echo underwater
  if(IS_UNDERWATER(ch))
  {
    return;
  }
  // whether they like it or not, Disharmonists get an echo to their songs
  if(has_innate(ch, INNATE_ECHO))
  {
    echoChance = 20;

    if(terrainType == SECT_MOUNTAIN)
    {
      echoChance += 10;
    }
    else if(terrainType == SECT_HILLS)
    {
      echoChance += 5;
    }
    else if(terrainType == SECT_FOREST)
    {
      echoChance -= 5;
    }
    if(echoChance >= number(1, 100))
    {
      echoDetails.song = song;
      echoDetails.room = ch->in_room;
      if(!get_scheduled(ch, event_echosong))
      {
        add_event(event_echosong, (get_bard_pulse(ch) / 6), ch, 0, 0, 0,
                  &echoDetails, sizeof(echoDetails));
      }
    }
  }
}

/*
 * Mainly copy of do_cast, with few slight differences
 */

void do_play(P_char ch, char *arg, int cmd)
{
  int      s, level, i;
  P_obj    instrument;
  struct affected_type af;

  /* muhaahaha
  if(IS_NPC(ch))
    return;*/
  if(!(ch))
  {
    logit(LOG_EXIT, "do_play in bard.c called without ch");
    raise(SIGSEGV);
  }

  if(!CAN_SING(ch))
  {
    send_to_char("&+WYou are unable to sing while in this state.\r\n", ch);
    return;

  }
  if(affected_by_spell(ch, SPELL_FEEBLEMIND))
  {
    send_to_char("&+WThe words to this song are ... what ... you can't remember!\r\n", ch);
    return;
  }
  if(affected_by_spell(ch, FIRST_INSTRUMENT))
  {
    send_to_char("&+yYou haven't regained your composure.\r\n", ch);
    return;
  }
  if(IS_TRUSTED(ch) &&
    GET_LEVEL(ch) < OVERLORD)
  {
    wizlog(GET_LEVEL(ch), "%s plays %s [%d]",
           GET_NAME(ch), arg, world[ch->in_room].number);
    logit(LOG_WIZ, "%s plays %s [%d]",
          GET_NAME(ch), arg, world[ch->in_room].number);
  }
  if(!arg || !*arg)
  {
    if(IS_AFFECTED3(ch, AFF3_SINGING))
    {
      send_to_char("You stop your song.\r\n", ch);
      act("$n stops singing abruptly.", FALSE, ch, 0, 0, TO_ROOM);
      stop_singing(ch);
      return;
    }
    send_to_char("Sing/play what song?\r\n", ch);
    return;
  }
  arg = skip_spaces(arg);
  if(!arg || !*arg)
  {
    send_to_char("Sing/play what song?\r\n", ch);
    return;
  }

  s = -1;
  for (i = 0; songs[i].name && s == -1; i++)
    if(is_abbrev(arg, songs[i].name))
    {
      s = songs[i].song;
    }
  if(s == -1 || GET_CHAR_SKILL(ch, s) == 0)
  {
    send_to_char("You don't know that song.\r\n", ch);
    return;
  }

  stop_singing(ch);

  instrument = has_instrument(ch);
  if(!instrument && (!IS_NPC(ch)))
  {
    act("You start singing aloud, but that wont make any effect.",
      FALSE, ch, 0, 0, TO_CHAR);
    act("$n starts to sing aloud.",
      FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
  if(IS_TRUSTED(ch) ||
    IS_NPC(ch))
  {
    act("&+GYou start singing aloud with a beautiful voice.",
      FALSE, ch, instrument, 0, TO_CHAR);
    act("&+G$n starts singing aloud with a beautiful voice.",
     FALSE, ch, instrument, 0, TO_ROOM);
    level = GET_LEVEL(ch);
  }
  else
  {
    if((bard_get_type(s) != instrument->value[0] + INSTRUMENT_OFFSET) &&
        !IS_ARTIFACT(instrument))
    {
      act
        ("&+rYou start playing your $q&+r, but this instrument won't work for this song.",
         FALSE, ch, instrument, 0, TO_CHAR);
      act("$n starts playing $p and singing aloud.",
        FALSE, ch, instrument, 0, TO_ROOM);
//        play_sound(SOUND_HARP, NULL, ch->in_room, TO_ROOM);
      return;
    }
    else
    {
      act("&+WYou start playing your $q &+Wand singing aloud.",
        FALSE, ch, instrument, 0, TO_CHAR);
      act("&+W$n starts playing $p &+Wand singing aloud.",
        FALSE, ch, instrument, 0, TO_ROOM);
      level = GET_LEVEL(ch) / 2;
//        play_sound(SOUND_HARP, NULL, ch->in_room, TO_ROOM);
    }
  }
  if(number(1, 101) > bard_calc_chance(ch, s))
  {
    act("Uh oh.. how did the song go, anyway?",
      FALSE, ch, 0, 0, TO_CHAR);
    act("$n stutters in $s song, and falls silent.",
      FALSE, ch, 0, 0, TO_ROOM);
    set_short_affected_by(ch, FIRST_INSTRUMENT, 3 * PULSE_VIOLENCE);
    do_action(ch, 0, CMD_BLUSH);
    return;
  }
  if(!IS_AFFECTED3(ch, AFF3_SINGING))
  {
    bzero(&af, sizeof(af));
    af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
    af.duration = (int) (level / 10 + 2);
    af.bitvector3 = AFF3_SINGING;
    affect_to_char(ch, &af);
  }

  disarm_char_events(ch, event_bardsong);

  if(!(get_scheduled(ch, event_bardsong)))
  {
    add_event(event_bardsong, get_bard_pulse(ch), ch, 0, 0, 0, &s, sizeof(s));
  }
}

void do_riff(P_char ch, char *arg, int cmd)
{
  int s, level, i, l, room;
  struct affected_type *af, *af2;
  struct char_link_data *cld, *next_cld;
  struct echo_details echoDetails;
  struct song_description *sd;
  P_char tch, next;

  if( !(ch) || !IS_ALIVE(ch) )
    return;

  if( affected_by_spell(ch, SKILL_RIFF) )
  {
    send_to_char("You havent recovered yet from that last wild &+Criff&n!\n", ch);
    return;
  }

  if( affected_by_spell(ch, FIRST_INSTRUMENT) )
  {
    send_to_char("&+yYou haven't regained your composure.\r\n", ch);
    return;
  }

  if( !CAN_SING(ch) )
  {
    send_to_char("&+WYou are unable to sing while in this state.\r\n", ch);
    return;

  }

  if(!IS_AFFECTED3(ch, AFF3_SINGING))
  {
    send_to_char("&+mYou must actively be &+Mplaying &+ma song in order to &+Criff&+m!&n\n", ch);
    return;
  }

  if(CHAR_IN_NO_MAGIC_ROOM(ch))
  {
    send_to_char("&+BThe power of song has no effect here.\r\n", ch);
    return;
  }

  arg = skip_spaces(arg);
  if( !arg || !*arg )
  {
    send_to_char("&+YWhat &+Wsong &+Ywould you like to sing a quick verse from?\r\n", ch);
    return;
  }

  if(number(1, 100) < GET_CHAR_SKILL(ch, SKILL_RIFF))
  {
    notch_skill(ch, SKILL_RIFF, get_property("skill.notch.offensive", 15)); 
  }

  if(number(1, 90) > GET_CHAR_SKILL(ch, SKILL_RIFF))
  {
    act("$n &+ctries to sing a quick &+Cverse&+c, but they cannot seem to recall the lines.&n", FALSE, ch, 0, 0, TO_ROOM);
    act("&+cYou &+ctry to sing a quick &+Cverse&+c, but cannot seem to recall the lines.&n", FALSE, ch, 0, 0, TO_CHAR);
    set_short_affected_by(ch, SKILL_RIFF, (int) (2 * PULSE_VIOLENCE));
    do_action(ch, NULL, CMD_COUGH);
    return;
  }

  s = -1;
  for (i = 0; songs[i].name && s == -1; i++)
    if(is_abbrev(arg, songs[i].name))
      s = songs[i].song;

  if(s == -1 || GET_CHAR_SKILL(ch, s) == 0)
  {
    send_to_char("You don't know that song.\r\n", ch);
    return;
  }

  if(ch && IS_ALIVE(ch))
  {
    for (i = 0; songwords[i].num; i++)
    {
      if(songwords[i].num == s)
      {
        act("$n &+Csuddenly breaks out a &+Wquick verse&+C from their &+Crepitiore &+Cof songs...&n", FALSE, ch, 0, 0, TO_ROOM);
        act("&+CYou suddenly break out a &+Wquick verse&+C from your &+Crepitiore &+Cof songs...&n", FALSE, ch, 0, 0, TO_CHAR);

        act(songwords[i].tochar, FALSE, ch, 0, 0, TO_CHAR);
        act(songwords[i].toroom, FALSE, ch, 0, 0, TO_ROOM);
        if(number(1, 10) > 5)
        {
          do_action(ch, 0, CMD_DANCE);
        }
        else
        {
          do_action(ch, 0, CMD_TWIRL);
        }
        break;
      }
    }
  }
  l = bard_song_level(ch, s) + number(-2, 2);

  if(l <= 0)
  {
    l = 1;
  }
  for (i = 0; songs[i].name; i++)
  {
    if(songs[i].song == s)
    {
      sd = &songs[i];
      break;
    }
  }
  room = ch->in_room;

  for (tch = world[room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if(IS_TRUSTED(tch))
      continue;

    if((ch == tch &&
      !(sd->flags & SONG_AGGRESSIVE)) ||
      (grouped(ch, tch) && !(sd->flags & SONG_AGGRESSIVE)) ||
      (ch != tch &&
      !grouped(ch, tch) &&
      !(sd->flags & SONG_ALLIES)))
    {
      if((sd->flags & SONG_AGGRESSIVE) &&
        bard_saves(ch, tch, s))
      {
        if(!IS_FIGHTING(tch))
        {
          bard_aggro(tch, ch);
        }
      }
      if(is_char_in_room(tch, room))
      {
        (sd->funct) (l, ch, tch, s);
      }
    }
  }

  set_short_affected_by(ch, SKILL_RIFF, (int) (4 * PULSE_VIOLENCE));
  CharWait(ch, (int)(PULSE_VIOLENCE));
}
