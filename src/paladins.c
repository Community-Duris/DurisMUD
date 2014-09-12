#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "objmisc.h"
#include "damage.h"
#include "guard.h"

#include "paladins.h"

char _buff[MAX_STRING_LENGTH];

struct aura_description
{
  char    *name;
  char  *glow_name;
} auras[] =
{
  {"Protection", "&+Bblue glow&n"},
  {"Precision", "&+Ggreen glow&n"},
  {"Battle Lust", "&+rblood-red glow&n"},
  {"Improved Healing", "&+Wwhite glow&n"},
  {"Endurance", "&+Yyellow glow&n"},
  {"Vigor", "&+mpurple glow&n"},
  {"Spell Protection", "&+Mmagenta glow&n"}
};

/*
 hook from the linked affect api, will be called when link is broken
*/
void aura_broken(struct char_link_data *cld)
{
  P_char   ch = cld->linking;
  P_char   target = cld->linked;
  int   aura_type = ( cld->affect ? cld->affect->type : -1 );

  if( aura_type >= FIRST_AURA && aura_type <= LAST_AURA ) {
    sprintf(_buff, "The %s surrounding you fades away.\n", auras[aura_type-FIRST_AURA].glow_name );
    send_to_char(_buff, ch);
    
    bool has_other_auras = false;
    for( struct affected_type* aff = ch->affected; aff; aff = aff->next ) {
      if( aff->type >= FIRST_AURA && aura_type <= LAST_AURA ) has_other_auras = true;
    }
    
    if( !has_other_auras )
      REMOVE_BIT(ch->specials.affected_by3, AFF3_PALADIN_AURA);	
  }
}

const char *aura_name(int aura) {
  if( aura < FIRST_AURA || aura > LAST_AURA ) return 0;
  return auras[aura-FIRST_AURA].name;
}

/* determines if player has an aura of the specified type */
bool has_aura(P_char ch, int aura_type) {
  for( struct affected_type* aff = ch->affected; aff; aff = aff->next ) {
    if( aff->type == aura_type ) 
      return TRUE;
  }
  return FALSE;
}

/* returns the modifier for that aura, sometimes is the level of the paladin */
int aura_mod(P_char ch, int aura_type) {
  for( struct affected_type* aff = ch->affected; aff; aff = aff->next ) {
    if( aff->type == aura_type ) 
      return aff->modifier;
  }
  return 0;
}

/*
 the function called by the innate (i.e., the caster)
*/
void do_aura(P_char ch, int aura)
{
  purge_linked_auras(ch);

  sprintf(_buff, "You recite a chant and a %s radiates from your finger tips.", auras[aura-FIRST_AURA].glow_name);
  act(_buff, FALSE, ch, 0, ch, TO_CHAR);

  sprintf(_buff, "$n recites a chant and a %s radiates from $s finger tips.", auras[aura-FIRST_AURA].glow_name);
  act(_buff, TRUE, ch, 0, ch, TO_NOTVICTROOM);

  apply_aura(ch, ch, aura);
  apply_aura_to_group(ch, aura);

  add_event(event_apply_group_auras, WAIT_SEC * 5, ch, 0, 0, 0, &aura, sizeof(aura));
}

/*
 clear all existing paladin auras from current character
*/
void purge_linked_auras(P_char ch)
{
  struct affected_type *af, *next_af;
  P_char   tch;

  while (tch = get_linking_char(ch, LNK_PALADIN_AURA))
    unlink_char(tch, ch, LNK_PALADIN_AURA);

  disarm_char_events(ch, event_apply_group_auras);
}

/*
 event that is periodically called to add auras to new group members/ppl who have just walked back in
*/
void event_apply_group_auras(P_char ch, P_char victim, P_obj obj, void *data) {
  int aura = *((int*)data);

  if( GET_STAT(ch) == STAT_NORMAL && GET_POS(ch) == POS_STANDING )
    apply_aura_to_group(ch, aura);

  add_event(event_apply_group_auras, WAIT_SEC * 5, ch, 0, 0, 0, &aura, sizeof(aura));
}

/* apply current aura to everyone in group */
void apply_aura_to_group(P_char ch, int aura) {
  for( struct group_list* tgl = ch->group; tgl && tgl->ch; tgl = tgl->next ) {
    if( ch != tgl->ch && tgl->ch->in_room == ch->in_room ) 
      apply_aura( ch, tgl->ch, aura );
  }
}

/* actually apply the aura to victim char */
void apply_aura(P_char ch, P_char victim, int aura)
{
  if( is_linked_to(ch, victim, LNK_PALADIN_AURA) || has_aura(victim, aura) )
  {
    return;
  }

  struct affected_type af;
  memset(&af, 0, sizeof(af));

  af.type = aura;
  af.duration = -1;
  af.flags = AFFTYPE_NODISPEL;

  switch( aura ) {
    case AURA_PROTECTION:
      af.location = APPLY_AC;
      af.modifier =
        (int) (-1 * get_property("innate.paladin_aura.protection_ac_mod", 1.0) * GET_LEVEL(ch) );
      break;
    case AURA_PRECISION:
      af.location = APPLY_HITROLL;
      af.modifier =
        (int) (get_property("innate.paladin_aura.precision_hitroll_mod", 0.2) * GET_LEVEL(ch));
      break;
    case AURA_BATTLELUST:
      af.modifier = GET_LEVEL(ch);
      break;
    case AURA_HEALING:
      af.modifier = GET_LEVEL(ch);
      break;
    case AURA_ENDURANCE:
      af.location = APPLY_MOVE;
      af.modifier =
        (int) (get_property("innate.paladin_aura.endurance_move_mod", 1.0) * GET_LEVEL(ch));
      break;
    case AURA_VIGOR:
      af.location = APPLY_HIT;
      af.modifier =
        (int) (get_property("innate.paladin_aura.vigor_hp_mod", 1.0) * GET_LEVEL(ch));
      break;
    case AURA_SPELL_PROTECTION:
      af.modifier =
        (int) (get_property("innate.paladin_aura.protection_spell_mod", 0.5) * GET_LEVEL(ch) )-3;
      break;
    default:
      debug( "apply_aura: Error - invalid aura to apply (%d)", aura );
      logit(LOG_DEBUG, "Error in apply_aura, invalid aura to apply (%d)", aura);
      break;
  }

  linked_affect_to_char(victim, &af, ch, LNK_PALADIN_AURA);
  SET_BIT(victim->specials.affected_by3, AFF3_PALADIN_AURA);

  sprintf(_buff, "You are surrounded by a %s.", auras[aura-FIRST_AURA].glow_name);
  act(_buff, FALSE, victim, 0, victim, TO_CHAR);
}

/* hook from show_visual_status() in actinf.c, to show auras on glance/look listing */
void send_paladin_auras(P_char ch, P_char tar_ch) {
  for( struct affected_type* aff = tar_ch->affected; aff; aff = aff->next ) {
    if( aff->type >= FIRST_AURA && aff->type <= LAST_AURA ) {
      sprintf(_buff, "$E is surrounded by a %s.", auras[aff->type-FIRST_AURA].glow_name);
      act(_buff, FALSE, ch, 0, tar_ch, TO_CHAR);
    }
  }
}

extern P_room world;

void spell_vortex_of_fear(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int num_hit = 0;

  act("&+LA swirling vortex of blackness billows from $n's outstretched hands.", FALSE, ch, 0, 0, TO_ROOM); 
  act("&+LA swirling vortex of blackness billows from your outstretched hands.", FALSE, ch, 0, 0, TO_CHAR); 

  for (P_char vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
  {
    if (vict == ch)
      continue;
    if (IS_DRAGON(vict) || IS_UNDEAD(vict) || (GET_RACE(vict) == RACE_GOLEM) || IS_TRUSTED(vict))
      continue;
    if (should_area_hit(ch, vict) && !NewSaves(vict, SAVING_FEAR, (int) level/11 - 3) && !fear_check(vict))
    {
      do_flee(vict, 0, 1);
      num_hit++;
    }
  }

  int healpoints = 70 + number(0,10) + dice(num_hit, 10);

  struct group_list *gl;
      
  if (ch && ch->group)
  {     
    for (struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if (gl->ch->in_room == ch->in_room)
      {
        if( GET_HIT(gl->ch) < GET_MAX_HIT(gl->ch) )
        {
          heal(gl->ch, ch, healpoints, GET_MAX_HIT(gl->ch) - number(1,4));
          update_pos(gl->ch);
          
          if( num_hit > 0 )
            send_to_char("&+LYou feel a grim satisfaction as the power of your enemy's fear flows into your soul!", gl->ch);
          else
            send_to_char("&+WYou feel better.\n", gl->ch);
        }  
      }
    }
  }
}

bool is_wielding_paladin_sword(P_char ch)
{
  if( !ch || !ch->equipment[WIELD] )
    return FALSE;

  P_obj weapon = ch->equipment[WIELD];

  if( IS_PALADIN_SWORD(weapon) )
    return TRUE;
  
  return FALSE;
}

bool cleave(P_char ch, P_char victim)
{
  if( is_wielding_paladin_sword(ch) &&
      !affected_by_spell(victim, SKILL_CLEAVE) &&
      ( notch_skill(ch, SKILL_CLEAVE, get_property("skill.notch.cleave", 10)) ||
        number(5,100) < GET_CHAR_SKILL(ch, SKILL_CLEAVE) ) )
  {
    act("You bring down $p in a mighty slash, cleaving $N from $S target!", TRUE,
        ch, ch->equipment[WIELD], victim, TO_CHAR);
    act("$n brings down $p in a mighty slash, cleaving $N from $S target!", TRUE,
        ch, ch->equipment[WIELD], victim, TO_NOTVICT);
    act("$n brings down $p in a mighty slash, cleaving you from your target!", TRUE,
        ch, ch->equipment[WIELD], victim, TO_VICT);

    int dam = dice( GET_LEVEL(ch) / 2, 10 );

    if( damage(ch, victim, dam, SKILL_CLEAVE) != DAM_VICTDEAD )
      set_short_affected_by(victim, SKILL_CLEAVE, 4 * PULSE_VIOLENCE);
    return TRUE;
  }

  return FALSE;
}

void event_smite_evil(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char tch;

  if( is_wielding_paladin_sword(ch) )
  {
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
      if (!grouped(tch, ch) && GET_HIT(tch) < GET_MAX_HIT(tch)/10 && IS_ALIVE(tch)
        && GET_HIT(tch) < 15 && IS_EVIL(tch) && (GET_RACEWAR(tch) != 1)
        && (notch_skill(ch, SKILL_SMITE_EVIL, get_property("skill.notch.smiteEvil", 10))
        || GET_CHAR_SKILL(ch, SKILL_SMITE_EVIL)/2 > number(0,100)))
      {
        act("A stern look appears on $n's face, and $e rushes to extinguish the evil being!", FALSE, ch, 0, tch, TO_NOTVICT);
        act("You exploit the evil creature's momentary weakness, and dive in for the killing blow!", FALSE, ch, 0, 0, TO_CHAR);
        act("$n looks at you sternly, and then dives in, $s blade rushing for your heart!", FALSE, ch, 0, tch, TO_VICT);
        damage(ch, tch, number(5,25), SKILL_SMITE_EVIL);
      }
    }
  }

  add_event(event_smite_evil,
    get_property("skill.timer.secs.smiteEvil", 5) * WAIT_SEC, ch, 0, 0, 0, 0, 0);
}

void spell_righteous_aura(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if( affected_by_spell(ch, SPELL_RIGHTEOUS_AURA) )
  {
    send_to_char("&+WYour god has already infused you with his power.&n\n", ch);
    return;
  }
  else
  {
    struct affected_type af;
    bzero(&af, sizeof(af));
    af.type = SPELL_RIGHTEOUS_AURA;
    if(IS_PC(ch))
    {
      af.duration = 5;
    }
    else
    {
      af.duration = 25;
    }
    affect_to_char(ch, &af);
    add_event(event_righteous_aura_check, WAIT_SEC, ch, 0, 0, 0, 0, 0);
    act("&+WYou are bathed in bright light as a powerful barrier of holiness surrounds you!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n &+Wis suddenly surrounded by blinding, peaceful light.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
}  

void event_righteous_aura_check(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type af;
  
  if( !affected_by_spell(ch, SPELL_RIGHTEOUS_AURA) )
  {
    return;
  }
  
  if(AWAKE(ch) &&
    !IS_IMMOBILE(ch)) 
  {
    P_char tch;
    LOOP_THRU_PEOPLE(tch, ch)
    {
      if(victim != ch &&
        affected_by_spell( tch, SPELL_BLEAK_FOEMAN ) &&
        GET_OPPONENT(ch) != tch &&
        !IS_TRUSTED(ch) &&
        !IS_TRUSTED(tch) )
      { // Soulshield up!
        if(!IS_AFFECTED2(ch, AFF2_SOULSHIELD) &&
           GET_ALIGNMENT(ch) >= 950 &&
           !IS_AFFECTED4(ch, AFF4_NEG_SHIELD))
        {
          bzero(&af, sizeof(af));
          af.type = SPELL_SOULSHIELD;
          if(IS_PC(ch))
          {
            af.duration =  (int) (5 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5);
          }
          else
          {
            af.duration = 25;
          }
          af.bitvector2 = AFF2_SOULSHIELD;
          affect_to_char(ch, &af);
          
          act("&+WA holy aura forms around $n!&n",
            TRUE, ch, 0, 0, TO_ROOM);
          act("&+WA holy aura forms around you!&n",
            TRUE, ch, 0, 0, TO_CHAR);
        } 
        // Dharma up!
        
        if(!IS_AFFECTED5(ch, AFF5_HOLY_DHARMA) &&
           IS_AFFECTED2(ch, AFF2_SOULSHIELD))
        {
          bzero(&af, sizeof(af));
          af.type = SPELL_HOLY_DHARMA;
          if(IS_PC(ch))
          {
            af.duration =  (int) (5 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5);
          }
          else
          {
            af.duration = 25;
          }
          af.bitvector5 = AFF5_HOLY_DHARMA;
          affect_to_char(ch, &af);
          
          send_to_char("&+cYou cast your soul into the cosmos seeking &+Wrighteousness.\r\n", ch);
          
          add_event(event_holy_dharma, (PULSE_VIOLENCE * number(3, 6)),
            ch, ch, 0, 0, 0, 0);
        }

        act("&+WThe aura surrounding your body flares for a moment, and drives you forth to\n"
            "&+Weliminate the &+Lvile knight&+W!", FALSE, ch, NULL, tch, TO_CHAR);
        act("&+WThe aura surrounding $n &+Wflares suddenly, and $e valiantly charges toward you.\n"
            "&+WNow $e must &+RDIE&+W!", FALSE, ch, NULL, tch, TO_VICT);
        act("&+WThe &+Lauras &+Wsurrounding $n &+Wand $N &+Wsuddenly &+Lflare&+W, and your heart races as an\n"
            "&+Lepic battle &+Wbegins!&n", FALSE, ch, 0, tch, TO_NOTVICT);
        
        // ATTACK!
        attack(tch, ch);
        break;
      }
    }
  }
    
  if( IS_FIGHTING(ch) )
  {
    P_char opponent = GET_OPPONENT(ch);

    if( GET_OPPONENT( opponent ) != ch &&
       CAN_ACT(opponent) &&
       (GET_ALIGNMENT(opponent) <= -500) )
    {      
      act("&+WThe holy aura surrounding your body flares brightly, and&n $N &+Wrushes towards\n"
          "&+Wyou fiercely!", FALSE, ch, 0, opponent, TO_CHAR);      
      act("&+WThe purity surrounding&n $n &+Wis too much, and you are compelled to extinguish\n"
          "&+Whis foul goodness!", FALSE, ch, 0, opponent, TO_VICT);
      act("$N &+Wgrimaces with pure hatred, and suddenly focuses his &+RRAGE &+Wat&n $n&+W!", FALSE, ch, 0, opponent, TO_NOTVICT);      
      // Check if they are fighting
      if(IS_FIGHTING(opponent))
      {
        // Set they target the paladin automatically
        opponent->specials.fighting = ch;
      }
      else
      {
        // otherwise, they go through the normal procedure as if they typed kill paladin
        attack(opponent, ch);
      }
    }
  }
  
  add_event(event_righteous_aura_check, WAIT_SEC, ch, 0, 0, 0, 0, 0);
}

void spell_bleak_foeman(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if( affected_by_spell(ch, SPELL_BLEAK_FOEMAN) )
  {
    send_to_char("&+LTo grow any more vile would require you to sprout horns and a tail.&n\n", ch);
    return;
  }
  else
  {
    struct affected_type af;
    bzero(&af, sizeof(af));
    af.type = SPELL_BLEAK_FOEMAN;
    if(IS_PC(ch))
    {
      af.duration = 5;
    }
    else
    {
      af.duration = 25;
    }
    affect_to_char(ch, &af);
    add_event(event_bleak_foeman_check, WAIT_SEC, ch, 0, 0, 0, 0, 0);
    act("&+LAs you complete your incantation, you are surrounded by a sickly black cloud.\n"
        "&+LThe cloud permeates your senses, and you let out an unearthly &+rROAR&+L as pure\n"
        "&+Levil seeps into your very soul!", FALSE, ch, 0, 0, TO_CHAR);
    act("&+LA sickly black cloud seems to coalesce in the room and flow towards $n&+L.\n"
        "&+LSuddenly, $e lets out an unearthly &+rROAR&+L that leaves you shaken.&n", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
  
}

void event_bleak_foeman_check(P_char ch, P_char victim, P_obj obj, void *data)
{
  if( !affected_by_spell(ch, SPELL_BLEAK_FOEMAN) )
    return;
  
  if(CAN_ACT(ch) &&
     AWAKE(ch) &&
    !IS_IMMOBILE(ch)) 
  {  
    P_char tch;
    LOOP_THRU_PEOPLE(tch, ch)
    {
      if( tch != ch && affected_by_spell( tch, SPELL_RIGHTEOUS_AURA ) && GET_OPPONENT(ch) != tch && !IS_TRUSTED(ch) && !IS_TRUSTED(tch) )
      {
        act("&+LAs the vile paragon of justice enters the room, the evil magic infusing your\n"
            "&+Lbody compels you to strike $M &+Ldown!&n", FALSE, ch, 0, tch, TO_CHAR);
        act("&+LSuddenly the aura surrounding $n &+Lflares pitch black, and $e charges towards you,\n"
            "&+La look of maddening hatred in $s eyes!&n", FALSE, ch, 0, tch, TO_VICT);
        act("&+LThe &+Wauras &+Lsurrounding $n &+Land $N &+Lsuddenly &+Wflare&+L, and your heart races as an\n"
            "&+Wepic battle &+Lbegins!&n", FALSE, ch, 0, tch, TO_NOTVICT);

        attack(ch, tch);
        break;
      }
    }  
  }
  
  add_event(event_bleak_foeman_check, WAIT_SEC, ch, 0, 0, 0, 0, 0);
}

void spell_dread_blade(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  P_obj weap;
  
  if (affected_by_spell(ch, SPELL_DREAD_BLADE))
    return;
  
  if(!(weap = victim->equipment[WIELD]))
    if(!weap && !(weap = victim->equipment[WIELD2]))
      if(!weap && !(weap = victim->equipment[WIELD3]))
        if(!weap && !(weap = victim->equipment[WIELD4]))
        {
          send_to_char("You must be wielding a weapon.\r\n", ch);
          return;
        }
        
  if(weap)
  {
    send_to_char("&+LPure &+revil&+L coalesces in a sickly black cloud, and seeps into your weapon!\n",              ch);
    
    memset(&af, 0, sizeof(af));
    
    af.type = SPELL_DREAD_BLADE;
    af.duration =  16;
    
    affect_to_char(victim, &af);
  }
}

bool dread_blade_proc(P_char ch, P_char victim)
{
  int num, room = ch->in_room, save, pos;
  P_obj wpn;
  
  typedef void (*spell_func_type) (int, P_char, char *, int, P_char, P_obj);
  spell_func_type spells[5] = {
    spell_disease,
    spell_blindness,
    spell_wither,
    spell_harm,
    spell_unholy_word
  };
  spell_func_type spell_func;
  
  if (!IS_FIGHTING(ch) ||
      !(victim = ch->specials.fighting) ||
      !IS_ALIVE(victim) ||
      !(room) ||
      number(0, 32)) // 3%
    return false;
    
  P_char vict = victim;
 
  for (wpn = NULL, pos = 0; pos < MAX_WEAR; pos++)
  {
    if((wpn = ch->equipment[pos]) &&
        wpn->type == ITEM_WEAPON &&
        wpn->type != ITEM_FIREWEAPON &&
        CAN_SEE_OBJ(ch, wpn))
      break;
  }

  if(wpn == NULL)
    return false;

  save = victim->specials.apply_saving_throw[SAVING_SPELL];
  victim->specials.apply_saving_throw[SAVING_SPELL] += 15;
  
  act("&+LYour&n $p releases a torrent of pure foul &+revil!&n",
    TRUE, ch, wpn, vict, TO_CHAR);
  act("$n's $p &+Lturns black and spews a torrent of pure foul &+revil&n &+Lat $N!",
    TRUE, ch, wpn, vict, TO_NOTVICT);
  act("$n's $p &+Lturns midnight black and spews a torrent of foul &+revil&n &+Lat you.",
    TRUE, ch, wpn, vict, TO_VICT);
  
  if(!(IS_MAGIC_DARK(room)))
  {
    spell_darkness(GET_LEVEL(ch), ch, 0, 0, 0, 0);
  }
  
  num = number(0, 4);
  
  spell_func = spells[num];
  
  spell_func(number(1, GET_LEVEL(ch)), ch, 0, 0, victim, 0);
  
  return !is_char_in_room(ch, room) || !is_char_in_room(victim, room);
  victim->specials.apply_saving_throw[SAVING_SPELL] = save;
}

bool holy_weapon_proc(P_char ch, P_char victim)
{
  int num, room, lvl;
  P_obj wpn;

  typedef void (*spell_func_type) (int, P_char, char *, int, P_char, P_obj);
  spell_func_type spells[] = {
    spell_cure_light,   // 0
    spell_cure_light,   // 1
    spell_cure_light,   // 2
    spell_cure_serious, // 3
    spell_cure_serious, // 4
    spell_cure_critic,  // 5
    spell_cure_critic,  // 6
    spell_heal,         // 7
    spell_heal,         // 8
    spell_full_heal,    // 9
    spell_full_heal     // 10
  };

  if( !IS_ALIVE(ch) || !IS_FIGHTING(ch) || !(victim = ch->specials.fighting)
    || !IS_ALIVE(victim) || !(room = ch->in_room) || number(0, 14)) // 6.67%
  {
    return FALSE;
  }

  // Level ranges from 0 to spells[].size - 2.
  lvl = GET_LEVEL(ch) == 56 ? 8 :
    (GET_LEVEL(ch) >= 53 ? 7 : (GET_LEVEL(ch) >= 50 ? 6 : (GET_LEVEL(ch) >= 40 ? 5 :
    (GET_LEVEL(ch) >= 30 ? 4 : (GET_LEVEL(ch) >= 20 ? 3 : (GET_LEVEL(ch) >= 15 ? 2 : 
    (GET_LEVEL(ch) >= 10 ? 1 : 0)))))));

  // Add a bit of randomness.
  num = number(0, 2) + lvl;

  // wpn = wield slot iff it's a paladin sword.
  if( (wpn = ch->equipment[WIELD]) && (!IS_PALADIN_SWORD(wpn) || !CAN_SEE_OBJ(ch, wpn)) )
  {
    wpn = NULL;
  }

  if( wpn == NULL )
  {
    return FALSE;
  }

  act("&+WThe holy power of your&n $p &+Wreplenishes your &+ysoul&+W with holy &+Yenergy&+W!&n",
    TRUE, ch, wpn, NULL, TO_CHAR);
  act("$n's $p &+Weminates a soft &+yg&+Yl&+Wow and $n &+Wlooks &+Yrefreshed&+W!",
    TRUE, ch, wpn, NULL, TO_ROOM);

  if( !(IS_MAGIC_LIGHT(room)) )
  {
    spell_continual_light(GET_LEVEL(ch), ch, 0, 0, 0, 0);
  }

/* WTH was this for? uber proc'ing on the victim?
  save = victim->specials.apply_saving_throw[SAVING_SPELL];
  victim->specials.apply_saving_throw[SAVING_SPELL] += 15;
*/
  spells[num](number(GET_LEVEL(ch)/2, GET_LEVEL(ch)), ch, 0, 0, ch, 0);
  /* And this is the undoing of the uber proc.. which isn't used.
  return !is_char_in_room(ch, room) || !is_char_in_room(victim, room);
  victim->specials.apply_saving_throw[SAVING_SPELL] = save;
  */
  return TRUE;
}
