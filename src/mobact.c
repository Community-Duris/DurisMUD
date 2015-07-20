/*
 * ***************************************************************************
 * *  File: mobact.c                                           Part of Duris *
 * *  Usage: Procedures generating 'intelligent' behavior in the mobiles.
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 * * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#undef RILDEBUG

#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "graph.h"
#include "mm.h"
#include "assocs.h"
#include "salchemist.h"
#include "objmisc.h"
#include "vnum.obj.h"
#include "nexus_stones.h"
#include "paladins.h"
#include "grapple.h"
#include "map.h"
#include "profile.h"
#include "guildhall.h"
#include "buildings.h"

/*
 * external variables
 */

extern Skill skills[];
extern P_desc descriptor_list;
extern P_event event_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *dirs[];
extern const struct stat_data stat_factor[];
extern double lfactor[];
extern float fake_sqrt_table[];
extern int MobSpellIndex[LAST_SPELL+1];
extern int equipment_pos_table[CUR_MAX_WEAR][3];
extern int no_specials;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern const int top_of_world;
extern const char rev_dir[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern const char *undead_type[];
extern struct potion potion_data[];
extern bool can_banish(P_char ch, P_char victim);
extern bool has_skin_spell(P_char);
extern bool has_wind_blade_wielded(P_char);
extern void event_wait(P_char ch, P_char victim, P_obj obj, void *data);

int CheckFor_remember(P_char ch, P_char victim);
int count_potions(P_char ch);
void try_wield_weapon(P_char ch);
int empty_slot_for_weapon(P_char ch);
int very_angry_npc(P_char, P_char, int, char *);
bool check_ch_nevents( P_char ch );
bool check_disruptive_blow(P_char ch);

#define UNDEAD_TYPES 6

extern const int undead_data[][7];

static P_char find_protector_target(P_char ch);
void check_for_wagon(P_char);
void do_npc_commune(P_char ch);

/* Combat Routines */
extern bool DragonCombat(P_char ch, int awe);
void     do_assist_core(P_char ch, P_char victim);
int      UndeadCombat(P_char ch);
int      GenMobCombat(P_char ch);
extern int DemonCombat(P_char ch);
static char *mem_str_dup(char *);
extern bool execute_quest_routine(P_char, int);
extern void event_spellcast(P_char, P_char, P_obj, void *);

extern int      cast_as_damage_area(P_char,
                             void (*func) (int, P_char, char *, int, P_char,
                                           P_obj), int, P_char, float, float);
extern int      cast_as_damage_area(P_char,
                             void (*func) (int, P_char, char *, int, P_char,
                                           P_obj), int, P_char, float, float,
                             bool (*s_func) (P_char, P_char));


struct remember_data
{
  P_char   c;
  struct remember_data *next;
} *remember_array[MAX_ZONES];

// Many mobiles are multiclass, and do not pick the best skin spell for protection.
// This function when called selects the best available skin spell for the mobile.
// Jun09 -Lucrot
int pick_best_skin_spell(P_char ch, P_char target)
{
  int spl = 0;
  
  if(has_skin_spell(ch))
    return 0;
    
  if(knows_spell(ch, SPELL_VINES) &&
     ch == target)
        spl = SPELL_VINES;
  else if
    (knows_spell(ch, SPELL_BIOFEEDBACK) &&
     ch == target)
        spl = SPELL_BIOFEEDBACK;
  else if
    (knows_spell(ch, SPELL_STONE_SKIN))
        spl = SPELL_STONE_SKIN;
  else if
    (!affected_by_spell(ch, SPELL_SHADOW_SHIELD) &&
     knows_spell(ch, SPELL_SHADOW_SHIELD) &&
     ch == target)
        spl = SPELL_SHADOW_SHIELD;
  else if( !affected_by_spell(ch, SPELL_IRONWOOD) && knows_spell(ch, SPELL_IRONWOOD)
    && IS_AFFECTED(ch, AFF_BARKSKIN) && ch == target )
    spl = SPELL_IRONWOOD;

  return spl;
}

/*
 * This is some heuristic for picking the best target for mob
 * AI.
 */
P_char pick_target(P_char ch, unsigned int flags)
{
  P_char tch, any = 0, weakest = 0, caster = 0, good = 0;
  int hits = 5000;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if(!is_aggr_to(ch, tch) && tch->specials.fighting != ch
      && ch->specials.fighting != tch)
    {
      continue;
    }
    if( !CAN_SEE_Z_CORD( ch, tch ) )
    {
      continue;
    }
    if(flags & PT_SIZE_TOLERANT)
    {
      if(!any || !number(0, 3))
      {
        any = tch;
      }
    }
    if((flags & PT_SMALLER) && get_takedown_size(ch) <= get_takedown_size(tch))
    {
      continue;
    }
    if((flags & PT_BASH_SIZE)
      && (get_takedown_size(ch) > get_takedown_size(tch) + 1
      || (get_takedown_size(ch) < get_takedown_size(tch) - 1)))
    {
      continue;
    }
    if((flags & PT_TRIP_SIZE)
      && (get_takedown_size(ch) > get_takedown_size(tch)
      || (get_takedown_size(ch) < get_takedown_size(tch) - 1)))
    {
      continue;
    }
    if(flags & PT_TOLERANT)
    {
      if(!any || !number(0, 3))
      {
        any = tch;
      }
    }
    if(IS_PC_PET(tch) && (flags & PT_WEAKEST))
    {
      continue;
    }
    if((flags & PT_NUKETARGET)
      && (IS_AFFECTED4(tch, AFF4_DEFLECT)
      || IS_AFFECTED4(tch, AFF4_STORNOGS_SPHERES)
      || (has_innate(tch, INNATE_MAGIC_RESISTANCE)
      && get_innate_resistance(tch) > number(0,100))))
    {
      continue;
    }
    if((flags & PT_STANDING) && GET_POS(tch) < POS_STANDING)
    {
      continue;
    }
    if((flags & PT_FRONT) && IS_BACKRANKED(tch))
    {
      continue;
    }
    good = tch;
    if((flags & PT_CASTER) && IS_CASTER(tch))
    {
      if(caster)
      {
        if(IS_CASTING(tch))
        {
          if(!IS_CASTING(caster))
            caster = tch;
          else if(number(0,1))
            caster = tch;
        }
        else if(!IS_CASTING(caster) && !number(0,2))
          caster = tch;
      }
      else
        caster = tch;
    }

    if(flags & PT_WEAKEST)
    {
      if((GET_HIT(tch) < hits && number(0,3)) || !weakest || !number(0,5))
      {
        weakest = tch;
        hits = GET_HIT(weakest);
      }
    }
  }

  if(caster && GET_C_INT(ch) > number(0,80))
    return caster;
  else if(weakest && GET_C_INT(ch) > number(0,100))
    return weakest;
  else if(good && GET_C_INT(ch) > number(0,50))
    return good;
  else
    return any;
}


int char_deserves_helping(const P_char ch, const P_char candidate,
                          int check_leader)
{
  struct follow_type *k;

  if(!IS_ALIVE(candidate) || !IS_ALIVE(ch) )
    return FALSE;

  if(candidate == ch)
    return TRUE;

  if(IS_PC(candidate))
    return FALSE;

  if(IS_MORPH(candidate))
    return FALSE;

  if(candidate->following && IS_PC(candidate->following))
    return FALSE;

  if(IS_PC_PET(ch))
    return FALSE;

  if((ch->specials.fighting == candidate) ||
      (candidate->specials.fighting == ch))
    return FALSE;

  /* if candidate fighting ch followers, don't help */

  for (k = ch->followers; k; k = k->next)
  {
    if(!char_deserves_helping(k->follower, candidate, FALSE))
      return FALSE;
  }

  /* if candidate fighting the guy you're following, don't help */

  if(ch->following && check_leader)
  {
    if(!char_deserves_helping(ch->following, candidate, TRUE))
      return FALSE;
  }

  /* don't spell up non-followers/groupees under level 15 */

  if((GET_LEVEL(candidate) < 15) &&
      !((candidate->following == ch) || (ch->following == candidate) ||
        (ch->group && (ch->group == candidate->group))))
    return FALSE;
    
  if((IS_NPC(ch) &&  
     GET_VNUM(ch) == WH_HIGH_PRIEST_VNUM) || 
     GET_VNUM(candidate) == WH_HIGH_PRIEST_VNUM) 
        return false;
        
  if((IS_NPC(ch) && 
     GET_VNUM(ch) == IMAGE_REFLECTION_VNUM) || 
     GET_VNUM(candidate) == IMAGE_REFLECTION_VNUM) 
      return false; 

  return TRUE;
}


int no_chars_in_room_deserve_helping(const P_char ch)
{
  P_char   temp;
  int      room = ch->in_room;

  if(room < 0)
    return TRUE;

  for (temp = world[room].people; temp; temp = temp->next_in_room)
    if(char_deserves_helping(ch, temp, TRUE))
    {
/*      send_to_room(GET_NAME(temp), room); */
      return FALSE;
    }
  return TRUE;
}


//
// used for NPCs when casting spells that affect evil-aligned enemies...
// enemy being any PC, a charmed NPC that is not following them, or anyone
// who is fighting them.  we'll see how it works out
//
// actually, just use char_deserves_helping and only cast on chars that
// don't deserve helping.  should work.
//

int room_has_evil_enemy(const P_char ch)
{
  P_char   temp;
  int      room = ch->in_room;

  if(room < 0)
    return FALSE;

  for (temp = world[room].people; temp; temp = temp->next_in_room)
  {
    if(!char_deserves_helping(ch, temp, TRUE))
    {
      if(IS_PC(temp))
      {
        if(RACE_EVIL(temp))
          return TRUE;
      }
      else if(GET_ALIGNMENT(temp) <= 0)
        return TRUE;
    }
  }

  return FALSE;
}


//
// same as above, but check for good-aligned enemies
//

int room_has_good_enemy(const P_char ch)
{
  P_char   temp;
  int      room = ch->in_room;

  if(room < 0)
    return FALSE;

  for (temp = world[room].people; temp; temp = temp->next_in_room)
  {
    if(!char_deserves_helping(ch, temp, TRUE))
    {
      if(IS_PC(temp))
      {
        if(RACE_GOOD(temp))
          return TRUE;
      }
      else if(GET_ALIGNMENT(temp) >= 0)
        return TRUE;
    }
  }

  return FALSE;
}

int GetLowestSpellCircle(int spl)
{
  return MobSpellIndex[spl];
}

int GetLowestSpellCircle_p(int spl)
{
  return (GetLowestSpellCircle(spl));
}

/*
 * one bit of cleverness here, if we can cast it, and we are facing
 * targets that are bristling with spells, dispel magic IS the spell of
 * choice to cast esp. if they have haste/globe/prots/bless/armor, the
 * usual, but we don't want to be dispeling slow, blind, curse, age,
 * wither, so let's find a likely target.  They will also dispel
 * themselves if it looks like a good idea (ie they have more unpleasant
 * affects than good ones). Assumes: 1 - ch is fighting 2 - ch can cast
 * dispel magic -JAB
 */

P_char FindDispelTarget(P_char ch, int lvl)
{
  P_char   tmp;
  int      i, j, k, l;
  int      scorecard[50];

  if(NumAttackers(ch) == 0)
    return NULL;

  if(lvl == -1)
    lvl = GET_LEVEL(ch);

  for (i = 0; i < 50; i++)
    scorecard[i] = 0;

  for (tmp = world[ch->in_room].people, i = 0; (i < 50) && tmp; tmp = tmp->next_in_room)
  {
    if(IS_FIGHTING(tmp) &&
       ((tmp->specials.fighting == ch) ||
       (tmp == ch)))
    {
      /* the plusses */
      if(IS_GLOBED(tmp))
        scorecard[i] += 3;
      if(IS_AFFECTED(tmp, AFF_HASTE))
        scorecard[i] += 3;
      if(IS_AFFECTED2(tmp, AFF2_PROT_LIGHTNING))
        scorecard[i] += 1;
      if(IS_AFFECTED2(tmp, AFF2_PROT_COLD))
        scorecard[i] += 1;
      if(IS_AFFECTED(tmp, AFF_PROT_FIRE))
        scorecard[i] += 1;
      if(IS_AFFECTED2(tmp, AFF2_PROT_GAS))
        scorecard[i] += 1;
      if(IS_AFFECTED2(tmp, AFF2_PROT_ACID))
        scorecard[i] += 1;
      if(IS_AFFECTED(tmp, AFF_FLY))
        scorecard[i] += 1;
      if(IS_AFFECTED(tmp, AFF_LEVITATE))
        scorecard[i] += 1;
      if(affected_by_spell(tmp, SPELL_VIRTUE))
        scorecard[i] += 10;
      if(affected_by_spell(tmp, SPELL_STONE_SKIN))
        scorecard[i] += 10;
      if(affected_by_spell(tmp, SPELL_BLUR))
        scorecard[i] += 10;
      if(affected_by_spell(tmp, SPELL_DAZZLE))
        scorecard[i] += 15;
      if(affected_by_spell(tmp, SPELL_ACCEL_HEALING | SPELL_REGENERATION))
        scorecard[i] += 20;
      if(affected_by_spell(tmp, SPELL_ESHABALAS_VITALITY))
        scorecard[i] += 20;
      if(affected_by_spell(tmp, SPELL_VITALITY))
        scorecard[i] += 20;
      if(IS_AFFECTED4(tmp, AFF4_BATTLE_ECSTASY | AFF4_SANCTUARY | AFF4_HELLFIRE))
        scorecard[i] += 25;
      if(affected_by_spell(tmp, SPELL_TRUE_SEEING))
        scorecard[i] += 30;

      /* and the minuses */
      if(affected_by_spell(tmp, SPELL_FEEBLEMIND))
        scorecard[i] -= 10;
      if(IS_AFFECTED(tmp, AFF_BLIND))
        scorecard[i] -= 10;
      if(IS_AFFECTED2(tmp, AFF2_SLOW))
        scorecard[i] -= 10;
      if(affected_by_spell(tmp, SPELL_FAERIE_FIRE))
        scorecard[i] -= 5;
      if(IS_IMMOBILE(tmp))
        scorecard[i] -= 500;
      if(IS_AFFECTED5(tmp, AFF5_IMPRISON))
        scorecard[i] -= 500;
      if(IS_PC_PET(tmp))
        scorecard[i] -= 900;

      if(tmp == ch)
        scorecard[i] = -scorecard[i];
      
      else if(scorecard[i] > 5)
      {
        /* modify score by saving throw */
        scorecard[i] += BOUNDED(0, (find_save(tmp, SAVING_SPELL) +
                        BOUNDED(-20, tmp->specials.apply_saving_throw[SAVING_SPELL] +
                        (lvl - GET_LEVEL(tmp)), 20)), 20) / 2;
      }
      i++;
    }
  }

  if(!i)
  {
    logit(LOG_STATUS, "No targets found in FindDispelTarget");
    return (NULL);
  }

  /* now let's find juiciest target for a dispel */
  l = 0;
  k = -999;
  for (j = 0; j < i; j++)
    if(scorecard[j] > k)
    {
      l = j;
      k = scorecard[l];
    }
  if(k > 5)
  {
    /* yup, we have a likely target, let's nail em */
    i = 0;
    for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
      if(IS_FIGHTING(tmp) && ((tmp->specials.fighting == ch) || (tmp == ch)))
        if(l == i++)
          return (tmp);
  }
  return (NULL);
}

int IS_CLERIC(P_char ch)
{
  return (GET_CLASS(ch, CLASS_DRUID) ||
          GET_CLASS(ch, CLASS_CLERIC) ||
          GET_CLASS(ch, CLASS_SHAMAN) ||
          GET_CLASS(ch, CLASS_ETHERMANCER) ||
          GET_CLASS(ch, CLASS_PALADIN));
}

int IS_HOLY(P_char ch)
{
  return (GET_CLASS(ch, CLASS_CLERIC) ||
         GET_CLASS(ch, CLASS_PALADIN) ||
         GET_CLASS(ch, CLASS_ANTIPALADIN) ||
         GET_CLASS(ch, CLASS_AVENGER));
}

int IS_MAGE(P_char ch)
{
  return (GET_CLASS(ch,
    CLASS_SORCERER |
    CLASS_CONJURER |
    CLASS_NECROMANCER |
    CLASS_BARD |
    CLASS_RANGER |
    CLASS_ILLUSIONIST |
    CLASS_REAVER |
    CLASS_ANTIPALADIN |
    CLASS_PSIONICIST) |
    CLASS_THEURGIST |
    CLASS_SUMMONER
);
}

int IS_THIEF(P_char ch)
{
  return (GET_CLASS(ch, CLASS_ROGUE) || 
         GET_CLASS(ch, CLASS_THIEF) ||
         GET_CLASS(ch, CLASS_ASSASSIN) ||
         GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF) ||
         GET_SPEC(ch, CLASS_ROGUE, SPEC_ASSASSIN));
}

int IS_WARRIOR(P_char ch)
{
  return (GET_CLASS(ch, CLASS_WARRIOR) ||
          GET_CLASS(ch, CLASS_ANTIPALADIN) ||
          GET_CLASS(ch, CLASS_PALADIN) ||
          GET_CLASS(ch, CLASS_MONK) ||
          GET_CLASS(ch, CLASS_BERSERKER) ||
          GET_CLASS(ch, CLASS_ALCHEMIST) ||
          GET_CLASS(ch, CLASS_RANGER) || 
          GET_CLASS(ch, CLASS_REAVER) ||
          GET_CLASS(ch, CLASS_AVENGER) ||
          GET_CLASS(ch, CLASS_MERCENARY));
}

bool In_Adjacent_Room(P_char ch, P_char vict)
{
  int      a;

  for (a = 0; a < NUM_EXITS; a++)
    if(CAN_GO(ch, a))
      if(EXIT(ch, a)->to_room == vict->in_room)
        return exitnumb_to_cmd(a);
  return FALSE;
}

bool MobCastSpell(P_char ch, P_char victim, P_obj object, int spl, int lvl)
{
  int      dura, skl;
  P_char   tch, tch2, kala, kala2;
  struct spellcast_datatype castdata;  /* for SpellCastProcess() */
  int      circle = 0, duration = 0;
  char     buf[MAX_STRING_LENGTH];

  // Crash iff: No caster, no target (and not area), or invalid spell.
  if( !ch || !(victim || object || IS_SET(skills[spl].targets, TAR_AREA | TAR_OFFAREA))
    || spl < FIRST_SPELL || spl > LAST_SPELL )
  {
    logit(LOG_EXIT, "MobCastSpell() bogus parms");
    raise(SIGSEGV);
  }

  if( !IS_NPC(ch) )              /* NPCs only should call this function */
  {
    return FALSE;
  }

  if( IS_CASTING(ch) )           /* NPC should not try to cast another spell */
  {
    return TRUE;                 /* if already in the process of casting one. */
  }

  if( (world[ch->in_room].room_flags & SINGLE_FILE) && !AdjacentInRoom(ch, victim) )
  {
    return FALSE;
  }

  if( affected_by_spell_flagged(ch, SKILL_THROAT_CRUSH, AFFTYPE_CUSTOM1) )
  {
    return FALSE;
  }

  if( ch->specials.z_cord > 0 && !IS_TRUSTED(ch) )
  {
    return FALSE;
  }
  if( IS_AFFECTED(ch, AFF_BOUND) )
  {
    return FALSE;
  }
  if( affected_by_spell(ch, SKILL_BERSERK) && !IS_TRUSTED(ch) )
  {
    return FALSE;
  }
  circle = GetLowestSpellCircle(spl);

#ifdef RILDEBUG
  {
    char     message[256];

    sprintf(message,
            "MobCastSpell():  spell = %s; target = %s; circle = %d; spelltype = %d\r\n",
            skills[spl].name, (victim ? GET_NAME(victim) : "unspecified"),
            circle, GET_SPELLTYPE(spl));
    send_to_char(message, ch);
  }
#endif

/*
 * If no available spells remaining in that particular circle, then NPC cannot
 * cast, sorry.  A buffered check exists higher up, but this one added here as
 * a precautionary measure. - SKB 21 Mar 1995
 */

  if( !GET_CLASS(ch, CLASS_PSIONICIST) && !GET_CLASS(ch, CLASS_MINDFLAYER) && (lvl < 60) )
  {
    if( ch->specials.undead_spell_slots[circle] <= 0 )
    {
      send_to_char("Sorry, out of spells in this circle.\r\n", ch);
      return (FALSE);
    }
  }
/*
 * Mark the beginning of the spellcast for the character as well as those
 * witnessing the cast.  Also impose a wait of length "duration" as prescribed
 * by the spell presently attempted.  All, of course, provided casting
 * permissible in present location. - SKB 30 Mar 1995
 */

  if( !GET_CLASS(ch, CLASS_PSIONICIST) && !GET_CLASS(ch, CLASS_MINDFLAYER))
  {
    if(IS_SET(world[ch->in_room].room_flags, ROOM_SILENT))
    {
      return FALSE;
    }

    send_to_char("&+cYou start chanting...\r\n&N", ch);

    if( IS_SET(world[ch->in_room].room_flags, NO_MAGIC) )
    {
      send_to_char("&+WThe magic gathers, then fades away.\r\n", ch);
      return FALSE;
    }
  }

  duration = SpellCastTime(ch, spl);
  CharWait(ch, duration);
  duration = MAX(1, duration);

  if( !GET_CLASS(ch, CLASS_PSIONICIST) && !GET_CLASS(ch, CLASS_MINDFLAYER) )
  {
    SpellCastShow(ch, spl);
  }

/*
 * The following if-block transplanted from sparser.c, do_cast() with some
 * modification.  Induces all eligible targets in room into combat against
 * the caster issuing a harmful area spell.  Previously not factored in and
 * coming here:  the requirement that the potential victim recognise the spell.
 * - SKB 24 Mar 1995
 */

  if( IS_AGG_SPELL(spl) )
  {
    appear(ch);

    if( IS_SET(skills[spl].targets, TAR_IGNORE)
      || IS_SET(skills[spl].targets, TAR_AREA) )
    {
      for (tch = world[ch->in_room].people; tch; tch = tch2)
      {
        tch2 = tch->next_in_room;

        if(tch == ch)
          continue;
        if(!CAN_SEE(tch, ch) || IS_FIGHTING(tch))
          continue;
        if(!should_area_hit(ch, tch))
          continue;
        if(GET_POS(tch) < POS_STANDING)
          continue;
        if(number(1, 125) > (GET_LEVEL(tch) + STAT_INDEX(GET_C_INT(tch))))
          continue;
        if(IS_NPC(tch))
          MobStartFight(tch, ch);
        else if( tch->only.pc->aggressive >= 0 && tch->only.pc->aggressive < GET_HIT(tch) )
        {
#ifndef NEW_COMBAT
          hit(tch, ch, tch->equipment[PRIMARY_WEAPON]);
#else
          hit(tch, ch, tch->equipment[WIELD], TYPE_UNDEFINED, getBodyTarget(tch), TRUE, FALSE);
#endif
        }

        if(!char_in_list(ch) || !char_in_list(tch))
          return FALSE;
      }

/*    } else if(IS_SET(skills[spl].targets, TAR_CHAR_RANGE)) {
   if(victim && (victim != ch) && IS_NPC(victim))
   MobRetaliateRange(victim, ch); */
    }
    else
    {
      if(victim && (victim != ch))
      {
        if(IS_NPC(victim) && !IS_FIGHTING(victim) && !IS_DESTROYING(victim)
          && CAN_SEE(victim, ch) && (GET_POS(ch) == POS_STANDING))
        {
          if(number(1, 150) <= (GET_LEVEL(victim) + STAT_INDEX(GET_C_INT(victim))))
          {
            if( IS_NPC(victim) || (victim->only.pc->aggressive >= 0 && victim->only.pc->aggressive < GET_HIT(victim)) )
              MobStartFight(victim, ch);
            if(!char_in_list(ch) || !char_in_list(victim))
              return FALSE;
          }
        }
      }
    }
  }
  /*
     The following constructs the requisite struct * spellcast_datatype
     for the function SpellCastProcess() in sparser.c.  The
     necessity of this structure at this moment depends solely on its
     use by SpellCastProcess() insofar as the choice to preserve its
     present mechanics.  I have modified the structure
     spellcast_datatype, defined in structs.h, to accommodate additional
     fields: (bool) needs_parsing, (P_char) victim, (P_obj) object,
     (int) spell. Addition of these fields avoids unnecessary,
     redundant, wasteful processing of information by
     SpellCastProcess() (e.g. parsing an * argument to extract spell and
     target information when that information already exists).  Flow
     passes to SpellCastProcess() upon completion of this structure. I
     am not convinced that the present flow from the issuance of the
     casting directive, target and spell selection (and/or parsing),
     chant delay, and through spell execution represents the most
     efficient one for handling both NPCs and players in parallel, but
     for the moment the primary concern remains implementation of an
     operational mob chant/mem system, with further dissections and
     refinements to follow afterwards. - SKB 26 Apr 1995
   */

  bzero(&castdata, sizeof(struct spellcast_datatype));

/*  castdata->timeleft = duration; */
  if(lvl < 60)
  {
    castdata.timeleft = ((number(1, 101) > (20 + 3 * GET_LEVEL(ch) / 2))
                          ? duration : (duration >> 1));
  }
  else
  {
    castdata.timeleft = -1;
  }

  castdata.spell = spl;
  castdata.object = object;

/*
 * Note the character as one in the process of spellcasting.  Various casting
 * checks, e.g. at the beginning of this function and in NewMobAct() disallow
 * various actions while character chants a spell. - SKB 24 Mar 1995
 */
  if( !IS_ALIVE(ch) )
  {
    debug( "Mob '%s' died while trying to cast.", J_NAME(ch) );
    return TRUE;
  }

  SET_BIT(ch->specials.affected_by2, AFF2_CASTING);

  if( IS_FIGHTING(ch) && check_disruptive_blow(ch) )
  {
    return TRUE;
  }

  if( (duration <= 4) || (lvl >= 60) )
    event_spellcast(ch, victim, object, &castdata);
  else
  {
    add_event(event_spellcast, 4, ch, victim, 0, 0, &castdata, sizeof(struct spellcast_datatype));
    if(victim)
    {
      if(IS_SET(skills[spl].targets, TAR_CHAR_WORLD) || IS_SET(skills[spl].targets, TAR_CHAR_RANGE))
      {
        link_char(ch, victim, LNK_CAST_WORLD);
      }
      else
      {
        link_char(ch, victim, LNK_CAST_ROOM);
      }
    }
  }

  return (TRUE);
}                               /*
                                 * End "New" MobCastSpell(), last modified
                                 * by SKB 26 Apr 1995
                                 */


bool CastIllusionistSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL, tmp = NULL;
  P_obj    obj = NULL;

  int      n_attk = 0, lvl = 0, spl = 0;

  if(IS_CASTING(ch))
    return TRUE;

  lvl = GET_LEVEL(ch);

  target = ch;

  if(!IS_FIGHTING(ch) && helping)
    target = victim;

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }

  if(affected_by_spell(ch, SKILL_BERSERK))
    return FALSE;

/* defensive spells */

  if(!spl &&
     npc_has_spell_slot(ch, SPELL_DETECT_ILLUSION) &&
     (ch == target) &&
     !IS_FIGHTING(ch) &&
     !IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION))
        spl = SPELL_DETECT_ILLUSION;

  if(!spl &&
    (!number(0, 3) || !IS_FIGHTING(ch)))
      spl = pick_best_skin_spell(ch, target);

  if( !spl && npc_has_spell_slot(ch, SPELL_PHANTOM_ARMOR) && (target == ch)
    && !IS_AFFECTED(target, AFF_ARMOR) && (!IS_FIGHTING(ch)) )
    spl = SPELL_PHANTOM_ARMOR;


  if(!spl && npc_has_spell_slot(ch, SPELL_FLY) &&
      !IS_AFFECTED(target, AFF_FLY) && !IS_FIGHTING(ch))
    spl = SPELL_FLY;

  if(!spl && (ch == target) && npc_has_spell_slot(ch, SPELL_VANISH)
      && !IS_FIGHTING(ch) && !IS_AFFECTED(ch, AFF_INVISIBLE)
      && !IS_AFFECTED2(ch, AFF2_MINOR_INVIS)
      && !IS_ACT(ch, ACT_TEACHER)
      && !mob_index[GET_RNUM(ch)].qst_func
      && (GET_HIT(target) < GET_MAX_HIT(target))
      && !CHAR_IN_TOWN(ch) && !number(0, 2))
    spl = SPELL_VANISH;

  /*if(!spl && (ch == target) && npc_has_spell_slot(ch, SPELL_REFLECTION)
     && !IS_FIGHTING(ch) && !IS_AFFECTED(ch, AFF_INVISIBLE)
     && !IS_AFFECTED2(ch, AFF2_MINOR_INVIS)
     && !IS_AFFECTED(ch, AFF_HIDE)
     && !mob_index[GET_RNUM(ch)].qst_func
     && (GET_HIT(target) < (GET_MAX_HIT(target)/2))
     && !CHAR_IN_TOWN(ch)
     && !number(0, 6))
     spl = SPELL_REFLECTION; */



  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    P_char   tch;
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastIllusionistSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in random number crap\r\n", ch);
    }
  }
  if((victim == ch) || helping)
    return (FALSE);

  /* mob in combat *Alv* */

  if(!victim && IS_FIGHTING(ch))
    target = ch->specials.fighting;
  else
    target = victim;

  if(!target)
    return (FALSE);

  /* cast semi-nasty stuff, but don't cast it all the time */
  if(!spl)
    n_attk = NumAttackers(ch);

  if((n_attk > 1) || has_help(target) ||
      no_chars_in_room_deserve_helping(ch))
  {
    if(!number(0, 7))
    {
      for (tmp = world[ch->in_room].people; tmp && !spl;
           tmp = tmp->next_in_room)
      {
        if(!((ch == tmp->specials.fighting) || are_together(target, tmp)))
          continue;

        /*if(!spl && npc_has_spell_slot(ch, SPELL_IMPRISON) && !(tmp == ch)
           && !number(0, 2) && !IS_FIGHTING(tmp))
           spl = SPELL_IMPRISON; */
           
        else if(!spl &&
                 npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) &&
                 !(tmp == ch) &&
                 number(0, 9) == 9)
          spl = SPELL_DISPEL_MAGIC;

        else if(!spl &&
                 npc_has_spell_slot(ch, SPELL_STUNNING_VISIONS) &&
                 !(tmp == ch) &&
                 !number(0, 2) &&
                 !IS_STUNNED(tmp))
          spl = SPELL_STUNNING_VISIONS;

        // else if(!spl &&
                 // npc_has_spell_slot(ch, SPELL_REFLECTION) &&
                 // !number(0, 2))
          // spl = SPELL_REFLECTION;

        else if(!spl &&
                 npc_has_spell_slot(ch, SPELL_NIGHTMARE) &&
                 !(tmp == ch) && 
                 number(0, 1))
                    spl = SPELL_NIGHTMARE;

        else if(!spl &&
                 npc_has_spell_slot(ch, SPELL_DELIRIUM) &&
                 !(tmp == ch) &&
                 number(0, 1) &&
                 !affected_by_spell(tmp, SPELL_DELIRIUM))
                    spl = SPELL_DELIRIUM;

        else if(!spl &&
                npc_has_spell_slot(ch, SPELL_BLINDNESS) &&
                !(tmp == ch) &&
                number(0, 1) &&
                !IS_BLIND(tmp) &&
                !EYELESS(tmp))
                    spl = SPELL_BLINDNESS;
      }
    }
    if(spl && ch && tmp)
      return (MobCastSpell(ch, tmp, 0, spl, lvl));

  }
  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl &&
      npc_has_spell_slot(ch, SPELL_DRAGON) &&
      number(0, 3))
  {
    spl = SPELL_DRAGON;
  }
  else if(!spl &&
           npc_has_spell_slot(ch, SPELL_HAMMER) &&
           number(0, 5))
  {
    spl = SPELL_HAMMER;
  }
  else if(!spl &&
           npc_has_spell_slot(ch, SPELL_SHADOW_MONSTER) &&
           number(0, 4) == 4)
  {
    spl = SPELL_SHADOW_MONSTER;
  }
  else if(!spl &&
           npc_has_spell_slot(ch, SPELL_BOULDER) &&
           number(0, 1) &&
           !IS_GLOBED(target))
  {
    spl = SPELL_BOULDER;
  }

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && npc_has_spell_slot(ch, SPELL_INSECTS) && number(0, 1))
    spl = SPELL_INSECTS;

  if(!spl && npc_has_spell_slot(ch, SPELL_BLINDNESS) && number(0, 1))
    spl = SPELL_BLINDNESS;

  if(!spl && npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) && number(0, 1))
    spl = SPELL_DISPEL_MAGIC;

  if(!spl && npc_has_spell_slot(ch, SPELL_BURNING_HANDS) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
    spl = SPELL_BURNING_HANDS;

  if(!spl && npc_has_spell_slot(ch, SPELL_MAGIC_MISSILE) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
    spl = SPELL_MAGIC_MISSILE;

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  return (FALSE);
}

bool CastMageSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL, tmp = NULL;
  P_obj    obj = NULL, best_corpse = NULL;
  P_event  e1 = NULL;

  int      dam = 0, lvl = 0, spl = 0, high_corpse = 0, pets;
  int      att_num_adjust[] = { 100, 50, 15, 0 };
  int      n_attk = 0;
  struct follow_type *k;

  lvl = GET_LEVEL(ch);

  target = ch;

  if(!IS_FIGHTING(ch) && helping)
    target = victim;

  if(helping)
  {
    dam = GET_MAX_HIT(victim) - GET_HIT(victim);
    target = victim;
  }
  else
  {
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    target = ch;
  }

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }
  
  if(affected_by_spell(ch, SKILL_BERSERK))
  {
    return FALSE;
  }
  
  /*  Trixie hobbitses and their fudging of code!  Jexni disapproves!
  if(IS_FIGHTING(ch) || (number(0, 9) != 8))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_HEAL_UNDEAD) && IS_UNDEAD(target) && (dam > 75))
     spl = SPELL_HEAL_UNDEAD;
  }
   */

  /*
   * Some rudimentary necromancy!  Why not capitalise upon carnage and raise a
   * protective legion of undead? - SKB 27 Apr 1995
   */
  if(!IS_GOOD(ch) &&
     !IS_FIGHTING(ch) &&
     !IS_DESTROYING(ch) &&
     !GET_MASTER(ch) &&
     !CHAR_IN_JUSTICE_AREA(ch) &&
     !CHAR_IN_TOWN(ch) &&
     (npc_has_spell_slot(ch, SPELL_ANIMATE_DEAD) ||
      npc_has_spell_slot(ch, SPELL_CALL_ARCHON) ||
      npc_has_spell_slot(ch, SPELL_CREATE_DRACOLICH) ||
     npc_has_spell_slot(ch, SPELL_CALL_TITAN) ||
     npc_has_spell_slot(ch, SPELL_CALL_AVATAR)) &&
     (!IS_AFFECTED(ch, AFF_HIDE) || IS_SET(ch->specials.act, ACT_SENTINEL)))
  {
    // find highest-level corpse

    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
    {
      if((GET_ITEM_TYPE(obj) == ITEM_CORPSE) && (obj->value[2] > high_corpse)
          && (obj->value[2] >= 4) && ((GET_LEVEL(ch) + 4) >= obj->value[2]))
      {
        best_corpse = obj;
        high_corpse = obj->value[2];
      }
    }

    // mobs can create their own corpses
    if(!best_corpse && GET_LEVEL(ch) > 10 &&
        !number(0, 100) && count_pets(ch) < 3) {
      best_corpse = read_object(2, VIRTUAL);
      best_corpse->value[2] = high_corpse =
        MIN(50, GET_LEVEL(ch)+number(0,8)-4);

      switch (number(0, 3)) {
      case 0:
        best_corpse->action_description =
          str_dup("&+La rotten corpse&n");
        break;
      case 1:
        best_corpse->action_description =
          str_dup("&+ma bloated carcass&n");
        break;
      case 2:
        best_corpse->action_description =
          str_dup("&+Lan &+rembalmed &+Lcadaver&n");
        break;
      case 3:
        best_corpse->action_description =
          str_dup("&+La &+rbeheaded &+Ladventurer&n");
        break;
      }
      best_corpse->str_mask = STRUNG_DESC3;
      set_obj_affected(best_corpse, 2 * WAIT_MIN, TAG_OBJ_DECAY, 0);
      obj_to_room(best_corpse, ch->in_room);
      act("$n finishes digging, revealing a terribly rotten corpse.",
        FALSE, ch, 0, 0, TO_ROOM);
      best_corpse = NULL;
    }

    if(best_corpse)
    {
      if(can_raise_draco(ch, lvl, false) && (high_corpse >= 40) &&
          (best_corpse->value[1] != PC_CORPSE) &&
          (npc_has_spell_slot(ch, SPELL_CREATE_DRACOLICH) ||
	  npc_has_spell_slot(ch, SPELL_CALL_TITAN)))
      {
        struct obj_affect *af;
        af = get_obj_affect(best_corpse, TAG_OBJ_DECAY);
        if(af)
        {
          unsigned newtime = obj_affect_time(best_corpse, af);
          newtime += (lvl * SECS_PER_MUD_HOUR * WAIT_SEC);
          affect_from_obj(best_corpse, TAG_OBJ_DECAY);
          set_obj_affected(best_corpse, newtime, TAG_OBJ_DECAY, 0);
        }

	if (GET_CLASS(ch, CLASS_THEURGIST))
	  spl = SPELL_CALL_TITAN;
	else
          spl = SPELL_CREATE_DRACOLICH;

        return (MobCastSpell(ch, 0, best_corpse, spl, lvl));
      }
      else if((npc_has_spell_slot(ch, SPELL_ANIMATE_DEAD) ||
                npc_has_spell_slot(ch, SPELL_RAISE_SPECTRE) ||
                npc_has_spell_slot(ch, SPELL_RAISE_WRAITH) ||
                npc_has_spell_slot(ch, SPELL_RAISE_VAMPIRE) ||
                npc_has_spell_slot(ch, SPELL_RAISE_LICH) ||
	        npc_has_spell_slot(ch, SPELL_CALL_ARCHON) ||
                npc_has_spell_slot(ch, SPELL_CALL_ASURA) ||
                npc_has_spell_slot(ch, SPELL_CALL_BRALANI) ||
                npc_has_spell_slot(ch, SPELL_CALL_KNIGHT) ||
                npc_has_spell_slot(ch, SPELL_CALL_LIBERATOR)) &&
               !(strstr(best_corpse->name, "undead")) &&
               ((pets = count_undead(ch)) < GET_LEVEL(ch) / 3) && pets >= 0)
      {
        if((npc_has_spell_slot(ch, SPELL_RAISE_LICH) ||
	    npc_has_spell_slot(ch, SPELL_CALL_LIBERATOR)) &&
            (high_corpse >= undead_data[5][0]))
        {
	  if (GET_CLASS(ch, CLASS_THEURGIST))
	    spl = SPELL_CALL_LIBERATOR;
	  else
            spl = SPELL_RAISE_LICH;
        }
        else if((npc_has_spell_slot(ch, SPELL_RAISE_VAMPIRE) ||
	         npc_has_spell_slot(ch, SPELL_CALL_KNIGHT)) &&
                 (high_corpse >= undead_data[4][0]))
        {
          if((npc_has_spell_slot(ch, SPELL_RAISE_WRAITH) ||
	      npc_has_spell_slot(ch, SPELL_CALL_BRALANI)) &&
              (number(0, 2) == 1) && (high_corpse >= undead_data[3][0]))
            if (GET_CLASS(ch, CLASS_THEURGIST))
	      spl = SPELL_CALL_BRALANI;
	    else
	      spl = SPELL_RAISE_WRAITH;
          else
	    if (GET_CLASS(ch, CLASS_THEURGIST))
	      spl = SPELL_CALL_KNIGHT;
	    else
	      spl = SPELL_RAISE_VAMPIRE;
        }
        else if((npc_has_spell_slot(ch, SPELL_RAISE_WRAITH) ||
	         npc_has_spell_slot(ch, SPELL_CALL_BRALANI)) &&
                 (high_corpse >= undead_data[3][0]))
        {
          if (GET_CLASS(ch, CLASS_THEURGIST))
	    spl = SPELL_CALL_BRALANI;
	  else
	    spl = SPELL_RAISE_WRAITH;
        }
        else if((npc_has_spell_slot(ch, SPELL_RAISE_SPECTRE) ||
	         npc_has_spell_slot(ch, SPELL_CALL_ASURA)) &&
                 (high_corpse >= undead_data[2][0]))
        {
	  if (GET_CLASS(ch, CLASS_THEURGIST))
	    spl = SPELL_CALL_ASURA;
	  else
            spl = SPELL_RAISE_SPECTRE;
        }
        else if(npc_has_spell_slot(ch, SPELL_ANIMATE_DEAD) ||
	        npc_has_spell_slot(ch, SPELL_CALL_ARCHON))
          if (GET_CLASS(ch, CLASS_THEURGIST))
	    spl = SPELL_CALL_ARCHON;
	  else
	    spl = SPELL_ANIMATE_DEAD;

        if(spl)
          return (MobCastSpell(ch, 0, best_corpse, spl, lvl));
      }
    }
  }
  /* hey..  if we have a guy who can cast conjure elem/conjure greater elem,
     let's go to town on this too */

  if(!IS_FIGHTING(ch) &&
     !IS_DESTROYING(ch) &&
     !GET_MASTER(ch) &&
     !CHAR_IN_JUSTICE_AREA(ch) &&
     !CHAR_IN_TOWN(ch))
  {
    if(npc_has_spell_slot(ch, SPELL_CONJURE_GREATER_ELEMENTAL) &&
       can_conjure_greater_elem(ch, lvl))
          return (MobCastSpell(ch, ch, 0, SPELL_CONJURE_GREATER_ELEMENTAL, lvl));
    else if(
      npc_has_spell_slot(ch, SPELL_CONJURE_ELEMENTAL) &&
      can_conjure_lesser_elem(ch, lvl))
          return (MobCastSpell(ch, ch, 0, SPELL_CONJURE_ELEMENTAL, lvl));
  }
  /*
   * * Defensive spells a high priority for the mage.  The order
   * reflects my assessment of a spell's importance insofar as
   * remaining active.  The default state of a mage should have each
   * of the following spells active, and should any one of them go
   * down immediately recast. - SKB 28 Mar 1995
   */
  if(!number(0,3))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_INVIS_MAJOR)
        && !IS_FIGHTING(ch) && !IS_AFFECTED(target, AFF_INVISIBLE)
        && !IS_DESTROYING(ch) && !IS_AFFECTED2(target, AFF2_MINOR_INVIS)
        && !IS_ACT(target, ACT_TEACHER) && !IS_FIGHTING(target)
        && (!IS_NPC(target) || !mob_index[GET_RNUM(target)].qst_func)
        && (GET_HIT(target) < GET_MAX_HIT(target)) && !CHAR_IN_TOWN(ch)
        && !CHAR_IN_TOWN(target))
      spl = SPELL_INVIS_MAJOR;
  }
  else
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_INVISIBLE)
        && !IS_FIGHTING(ch) && !IS_AFFECTED(target, AFF_INVISIBLE)
        && !IS_DESTROYING(ch) && !IS_AFFECTED2(target, AFF2_MINOR_INVIS)
        && !IS_ACT(target, ACT_TEACHER) && !IS_FIGHTING(target)
        && (!IS_NPC(target) || !mob_index[GET_RNUM(target)].qst_func)
        && (GET_HIT(target) < GET_MAX_HIT(target)) && !CHAR_IN_TOWN(ch)
        && !CHAR_IN_TOWN(target) && ((ch == target) && !number(0, 9)))
    spl = SPELL_INVISIBLE;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_DETECT_INVISIBLE)
      && !IS_FIGHTING(ch) && !IS_AFFECTED(target, AFF_DETECT_INVISIBLE))
    spl = SPELL_DETECT_INVISIBLE;

  if(!spl && (ch == target) &&
      npc_has_spell_slot(ch, SPELL_VAMPIRIC_TOUCH) && IS_EVIL(ch)
      && !IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH) && !IS_FIGHTING(ch))
    spl = SPELL_VAMPIRIC_TOUCH;

  if(!spl)
    n_attk = NumAttackers(ch);

  if(!spl && (ch == target) &&
      !IS_AFFECTED2(ch, AFF2_FIRESHIELD) &&
      !IS_AFFECTED3(ch, AFF3_COLDSHIELD) &&
      !IS_AFFECTED3(ch, AFF3_LIGHTNINGSHIELD) &&
      (!IS_FIGHTING(ch) || (number(0, 3) == 2)))
  {
    if(!number(0, 1) && npc_has_spell_slot(ch, SPELL_FIRESHIELD))
      spl = SPELL_FIRESHIELD;
    else if(npc_has_spell_slot(ch, SPELL_COLDSHIELD))
      spl = SPELL_COLDSHIELD;
    else if(npc_has_spell_slot(ch, SPELL_LIGHTNINGSHIELD))
      spl = SPELL_LIGHTNINGSHIELD;
  }
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_GLOBE) &&
     !IS_AFFECTED2(target, AFF2_GLOBE) &&
     !IS_ANIMAL(target) &&
     GET_RACE(target) != RACE_PLANT &&
     GET_LEVEL(target) > 21)
  {
    spl = SPELL_GLOBE;
  }
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_DEFLECT) &&
     (!IS_FIGHTING(ch) || !number(0, 3)) &&
     !IS_AFFECTED4(ch, AFF4_DEFLECT) &&
     (ch == target))
  {
    spl = SPELL_DEFLECT;
  }
  if(!spl && npc_has_spell_slot(ch, SPELL_STORNOGS_SPHERES) &&
      (!IS_FIGHTING(ch) || !number(0, 3)) && !IS_AFFECTED4(ch, AFF4_STORNOGS_SPHERES)
      && (ch == target))
  {
    spl = SPELL_STORNOGS_SPHERES;
  }
  if(!spl &&
     (!number(0, 8) || !IS_FIGHTING(ch)))
        spl = pick_best_skin_spell(ch, target);
  
  if(!spl && npc_has_spell_slot(ch, SPELL_PROT_UNDEAD)
      && !has_skin_spell(target) && IS_UNDEAD(target)
      && (!number(0, 8) || !IS_FIGHTING(ch)))
  {
    spl = SPELL_PROT_UNDEAD;
  }
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_VAMPIRE) &&
     !IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) &&
     !IS_FIGHTING(ch))
  {
    spl = SPELL_VAMPIRE;
  }
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_VITALIZE_UNDEAD) &&
     IS_UNDEAD(target) &&
     !affected_by_spell(target, SPELL_VITALIZE_UNDEAD) &&
     (!number(0, 5) || !IS_FIGHTING(ch)))
  {
  spl = SPELL_VITALIZE_UNDEAD;
  }
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_MINOR_GLOBE) &&
     !IS_ANIMAL(target) &&
     GET_RACE(target) != RACE_PLANT &&
     !(IS_AFFECTED2(target, AFF2_GLOBE) ||
      IS_AFFECTED(target, AFF_MINOR_GLOBE)))
  {
    spl = SPELL_MINOR_GLOBE;
  }
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_HASTE) &&
     !IS_AFFECTED(target, AFF_HASTE) &&
     (!IS_FIGHTING(ch) || !number(0, 4)))
  {
    if(!IS_PC_PET(target) &&
       GET_LEVEL(target) > 21)
          spl = SPELL_HASTE;
  }
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_FLY) &&
     !IS_AFFECTED(target, AFF_FLY) &&
     !IS_FIGHTING(ch))
        spl = SPELL_FLY;
  if(!spl && npc_has_spell_slot(ch, SPELL_LEVITATE) &&
      !IS_AFFECTED(target, AFF_LEVITATE) && !IS_AFFECTED(target, AFF_FLY) &&
      !IS_FIGHTING(ch))
    spl = SPELL_LEVITATE;
  /*
   * If protection spell selected, then forego further evaluation and proceed
   * to cast it. - SKB 28 Mar 1995
   */

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    P_char   tch;
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastMageSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in random number crap\r\n", ch);
    }
  }
  if((victim == ch) || helping)
    return (FALSE);

  /*
   * At this point the mage enters the realm of combat and seeks to make life
   * as unpleasant, to the greatest extent possible, for nuisance opponent(s).
   * If attacked by more than one opposing the NPC, area spells opted for.
   * Naturally we must ascertain the existence of a target before proceeding any
   * further since many of the realtime combat decisions depend on the status
   * of a given target. - SKB
   */

  if(!victim && IS_FIGHTING(ch))
    target = ch->specials.fighting;
  else
    target = victim;

  if(!target)
    return (FALSE);

  /* cast semi-nasty stuff, but don't cast it all the time */

  if((n_attk > 1) ||
      has_help(target) ||
      no_chars_in_room_deserve_helping(ch))
  {
    if(spl && ch && tmp)
      return (MobCastSpell(ch, tmp, 0, spl, lvl));

    /* only cast this stuff if there is more than one fella, or if the caster doesn't have
       one of the bigby's spells */

    /* update..  bigby's crushing hand does enough damage to kill a PC in one hit, so if the
       mob has that, cast it usually */

    if(((n_attk > 1) ||
        has_help(target) ||
        no_chars_in_room_deserve_helping(ch)) &&
        !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) &&
        (number(0, 3)))
    {
      if(!spl &&
        npc_has_spell_slot(ch, SPELL_METEOR_SWARM) &&
        OUTSIDE(ch))
      {
        spl = SPELL_METEOR_SWARM;
      }

      if(!spl &&
         npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) &&
         number(0, 5) == 3 &&
         IS_PC(target) &&
         affected_by_spell(target, SPELL_VITALITY))
      {
        spl = SPELL_DISPEL_MAGIC;
      }
      
      if(!spl &&
        npc_has_spell_slot(ch, SPELL_UNDEAD_TO_DEATH) &&
        number(0, 2) &&
        IS_UNDEADRACE(target))
      {
        spl = SPELL_UNDEAD_TO_DEATH;
      }
      
      if(!spl &&
         npc_has_spell_slot(ch, SPELL_SUMMON_GHASTS) &&
         !IS_UNDEADRACE(target))
      {
        spl = SPELL_SUMMON_GHASTS;
      }

      if(!spl &&
	 npc_has_spell_slot(ch, SPELL_AID_OF_THE_HEAVENS) &&
	 !IS_ANGEL(target))
      {
	spl = SPELL_AID_OF_THE_HEAVENS;
      }

      if(!spl &&
         npc_has_spell_slot(ch, SPELL_CLOAK_OF_FEAR) &&
         !number(0, 2))
      {
        spl = SPELL_CLOAK_OF_FEAR;
      }
      
      if(!spl &&
        npc_has_spell_slot(ch, SPELL_INCENDIARY_CLOUD) &&
        !number(0, 2))
      {
        spl = SPELL_INCENDIARY_CLOUD;
      }
      
      if(!spl &&
         npc_has_spell_slot(ch, SPELL_PRISMATIC_SPRAY))
      {
        spl = SPELL_PRISMATIC_SPRAY;
      }
      
      // if(!spl &&
         // npc_has_spell_slot(ch, SPELL_BLINK) &&
         // !number(0, 3))
      // {
        // spl = SPELL_BLINK;
      // }
      
      if(!spl &&
         npc_has_spell_slot(ch, SPELL_CHAIN_LIGHTNING) &&
         !number(0, 2))
        spl = SPELL_CHAIN_LIGHTNING;

      if(!spl &&
         npc_has_spell_slot(ch, SPELL_ICE_STORM) &&
         !IS_GLOBED(target) &&
         GET_LEVEL(ch) < 46 &&
         !number(0, 3))
            spl = SPELL_ICE_STORM;
    }
  }
  /*
     If an appropriate area spell selected, then forego further
     evaluation and cast it. - SKB 28 Mar 1995
   */
  if(spl && ch)
  {
    P_char nuke_target = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);
    return (MobCastSpell(ch, nuke_target ? nuke_target : target, 0, spl, lvl));
  }
  /*
   * Having passed over the area spells, the NPC selects the most effective or
   * potent spell for dealing with a single target. - SKB
   */

  if(!spl &&
     npc_has_spell_slot(ch, SPELL_DREAD_WAVE) &&
     (!number(0, 2) ||
     ENJOYS_FIRE_DAM(target) ||
     IS_AFFECTED2(target, AFF2_FIRESHIELD)))
  {
    spl = SPELL_DREAD_WAVE;
  }
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_CHAOTIC_RIPPLE) &&
     !number(0, 2))
  {
    spl = SPELL_CHAOTIC_RIPPLE;
  }
  else if(!spl &&
           npc_has_spell_slot(ch, SPELL_CHAOS_VOLLEY) &&
           number(0, 2) &&
           (GET_HIT(ch) < (int)(GET_MAX_HIT(ch) * 0.66)))
  {
    spl = SPELL_CHAOS_VOLLEY;
  }
  else if(!spl &&
          npc_has_spell_slot(ch, SPELL_UNDEAD_TO_DEATH) &&
          number(0, 2) &&
          IS_UNDEADRACE(target))
  {
    spl = SPELL_UNDEAD_TO_DEATH;
  }
  else if(!spl &&
          npc_has_spell_slot(ch, SPELL_ANTI_MAGIC_RAY) &&
          number(0, 1))
  {
    spl = SPELL_ANTI_MAGIC_RAY;
  }
  else if(!spl &&
         npc_has_spell_slot(ch, SPELL_MISSILE_BARRAGE) &&
         !number(0, 2))
  {
    spl = SPELL_MISSILE_BARRAGE;
  }
  else if(!spl &&
           npc_has_spell_slot(ch, SPELL_BIGBYS_CRUSHING_HAND) &&
           !number(0, 2))
  {
    spl = SPELL_BIGBYS_CRUSHING_HAND;
  }

  if(!spl &&
     npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) &&
     number(0, 9) == 9)
  {
    spl = SPELL_DISPEL_MAGIC;
  }
  
  else if(!spl &&
           npc_has_spell_slot(ch, SPELL_PWORD_STUN) &&
           (ch != target) &&
           !IS_STUNNED(target) &&
           (GET_C_POW(target) < GET_C_POW(ch)) &&
           number(0,1))
  {
    spl = SPELL_PWORD_STUN;
  }
  else if(!spl &&
           npc_has_spell_slot(ch, SPELL_PWORD_BLIND) &&
           target != ch &&
           !IS_AFFECTED(target, AFF_BLIND) &&
           ((GET_LEVEL(target) < GET_LEVEL(ch))) &&
           number(0,1) &&
           !EYELESS(target) &&
           GET_C_POW(ch) > GET_C_POW(target))
  {
    spl = SPELL_PWORD_BLIND;
  }
  else if(!spl &&
           npc_has_spell_slot(ch, SPELL_ACIDIMMOLATE) &&
           !number(0, 2))
  {
    spl = SPELL_ACIDIMMOLATE;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_PRISMATIC_RAY) && number(0, 4))
  {
    spl = SPELL_PRISMATIC_RAY;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_NEGATIVE_CONCUSSION_BLAST)
          && !IS_UNDEAD(target) && number(0, 3))
  {
    spl = SPELL_NEGATIVE_CONCUSSION_BLAST;
  }
  else if(!spl &&
         npc_has_spell_slot(ch, SPELL_ENERGY_DRAIN) &&
         !IS_UNDEAD(target) &&
         number(0, 2))
  {
    spl = SPELL_ENERGY_DRAIN;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_BIGBYS_CLENCHED_FIST) && number(0, 3))
  {
    spl = SPELL_BIGBYS_CLENCHED_FIST;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_EARTHEN_MAUL) && number(0, 3))
  {
    spl = SPELL_EARTHEN_MAUL;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_CHAIN_LIGHTNING) && number(0, 5))
  {
    spl = SPELL_CHAIN_LIGHTNING;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_SOLAR_FLARE) && number(0,1))
  {
    spl = SPELL_SOLAR_FLARE;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_MAGMA_BURST) && number(0,1))
  {
    spl = SPELL_MAGMA_BURST;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_DISINTEGRATE) && number(0,1))
  {
    spl = SPELL_DISINTEGRATE;
  }
  else if(!spl && npc_has_spell_slot(ch, SPELL_PRISMATIC_SPRAY) &&
      !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    spl = SPELL_PRISMATIC_SPRAY;
  }
 
  if(!spl && npc_has_spell_slot(ch, SPELL_SHATTER))
  {
    spl = SPELL_SHATTER;
  }
  if(!spl && npc_has_spell_slot(ch, SPELL_FROSTBITE))
  {
    spl = SPELL_FROSTBITE;
  }
  if(!spl && npc_has_spell_slot(ch, SPELL_FIREBALL) && !IS_GLOBED(target)
      && !(GET_RACE(target) == RACE_F_ELEMENTAL))
  {
    spl = SPELL_FIREBALL;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_CONE_OF_COLD) && !IS_GLOBED(target))
  {
    spl = SPELL_CONE_OF_COLD;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_COLOR_SPRAY) && !IS_GLOBED(target)
     && !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    spl = SPELL_COLOR_SPRAY;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_PWORD_KILL) && !(target == ch)
     && (GET_LEVEL(target) < GET_LEVEL(ch) + number(-5, 5)))
  {
    spl = SPELL_PWORD_KILL;
  }

  if(!spl &&
    npc_has_spell_slot(ch, SPELL_PWORD_BLIND) &&
    target != ch &&
    !IS_AFFECTED(target, AFF_BLIND) &&
    ((GET_LEVEL(target) < GET_LEVEL(ch))) &&
    number(0,1) &&
    !EYELESS(target) &&
    GET_C_POW(ch) > GET_C_POW(target))
  {
    spl = SPELL_PWORD_BLIND;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT) && !IS_GLOBED(target))
  {
    spl = SPELL_LIGHTNING_BOLT;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_ACID_BLAST) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
  {
    spl = SPELL_ACID_BLAST;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_SHOCKING_GRASP) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
  {
    spl = SPELL_SHOCKING_GRASP;
  }

  if(spl && ch && target) 
  {
    P_char nuke_target = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);
    return (MobCastSpell(ch, nuke_target ? nuke_target : target, 0, spl, lvl));
  }


  /*
   * At this point, with the significant damage spells unchosen, feeblemind
   * poses the greatest inconvenience to the opponents. - SKB
   *
   *
   * This block was previously commented out...  I believe feeblemind was deemed
   * too detrimental to a caster... let's change that and re-add the ability to
   * fuxor a caster royally once more - Jexni 11/25/08
   */

  if(!spl && npc_has_spell_slot(ch, SPELL_FEEBLEMIND) &&
     !IS_AFFECTED(target, SPELL_FEEBLEMIND) && (IS_MAGE(target) || IS_CLERIC(target)))
  {
    spl = SPELL_FEEBLEMIND;
  }
  else 
  { 
    for(target = world[ch->in_room].people; target; target = target->next_in_room)
    {
      if((target != ch) && should_area_hit(ch, target) &&
        (CAN_SEE(ch, target) || (target == ch->specials.fighting)) &&
        IS_PC(target) && !affected_by_spell(target, SPELL_FEEBLEMIND) && number(0, 2))
      {
        break;
      }
    }

    if(!target || !(IS_MAGE(target) || !(IS_CLERIC(target))))
    {
      spl = 0;

      if(!victim)
        target = ch->specials.fighting;
      else
        target = victim;
    }
    
    if(target == ch)
      return FALSE;
  }

  /*
   * At this point the NPC has only the lower circle spells to resort to.  As
   * earlier, the NPC conserves the spell if the appropriate barrier (globe)
   * protects the target - SKB
   */

  if(!number(0, 3))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_HEAL_UNDEAD) && IS_UNDEAD(target) && (dam > 75))
        spl = SPELL_HEAL_UNDEAD;
  }

  if(!spl && /*(lvl > 20) && */ !IS_AFFECTED2(target, AFF2_SLOW) && !number(0, 4))
  {
    spl = SPELL_SLOW;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_BURNING_HANDS) && (!IS_GLOBED(target) || !IS_MINGLOBED(target))
      && !(GET_RACE(target) == RACE_F_ELEMENTAL))
  {
    spl = SPELL_BURNING_HANDS;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_CHILL_TOUCH) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
    spl = SPELL_CHILL_TOUCH;

  if(!spl && npc_has_spell_slot(ch, SPELL_SLASHING_DARKNESS) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
    spl = SPELL_SLASHING_DARKNESS;

  if(!spl && npc_has_spell_slot(ch, SPELL_MAGIC_MISSILE) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
    spl = SPELL_MAGIC_MISSILE;

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  return (FALSE);
}

bool CastReaverSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL, tmp = NULL;
  P_obj    wpn;
  int      dam = 0, lvl = 0, spl = 0, n_attk = 0;


  if(IS_CASTING(ch))
    return FALSE;

  lvl = GET_LEVEL(ch);

  if(helping)
  {
    dam = GET_MAX_HIT(victim) - GET_HIT(victim);
    target = victim;
  }
  else
  {
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    target = ch;
  }

  if(MobShouldFlee(ch) &&
     !number(0, 9)) // Reavers are good hitters. No need to flee quickly.
  {
    do_flee(ch, 0, 0);
    return false;
  }
  else if(MobShouldFlee(ch))
  {
    return false;
  }

  if(affected_by_spell(ch, SKILL_BERSERK))
    return FALSE;

  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_KANCHELSIS_FURY) &&
      npc_has_spell_slot(ch, SPELL_KANCHELSIS_FURY) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_KANCHELSIS_FURY;

  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_THRYMS_ICERAZOR) &&
      (wpn = ch->equipment[WIELD]) &&
      wpn->type == ITEM_WEAPON && IS_BLUDGEON(wpn) &&
      npc_has_spell_slot(ch, SPELL_THRYMS_ICERAZOR) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_THRYMS_ICERAZOR;

  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_LLIENDILS_STORMSHOCK) &&
      (wpn = ch->equipment[WIELD]) &&
      wpn->type == ITEM_WEAPON && !IS_BLUDGEON(wpn) && !IS_AXE(wpn) &&
      npc_has_spell_slot(ch, SPELL_LLIENDILS_STORMSHOCK) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_LLIENDILS_STORMSHOCK;

  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_ILIENZES_FLAME_SWORD) &&
      (wpn = ch->equipment[WIELD]) &&
      wpn->type == ITEM_WEAPON && IS_SWORD(wpn) &&
      npc_has_spell_slot(ch, SPELL_ILIENZES_FLAME_SWORD) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_ILIENZES_FLAME_SWORD;

  if(!spl && (ch == target) &&
      npc_has_spell_slot(ch, SPELL_ESHABALAS_VITALITY) &&
      !affected_by_spell(target, SPELL_ESHABALAS_VITALITY) && !IS_FIGHTING(ch))
    spl = SPELL_ESHABALAS_VITALITY;

  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_CHILLING_IMPLOSION) &&
      affected_by_spell(target, SPELL_THRYMS_ICERAZOR) &&
      npc_has_spell_slot(ch, SPELL_CHILLING_IMPLOSION) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_CHILLING_IMPLOSION;

  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_STORMCALLERS_FURY) &&
      affected_by_spell(target, SPELL_LLIENDILS_STORMSHOCK) &&
      npc_has_spell_slot(ch, SPELL_STORMCALLERS_FURY) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_STORMCALLERS_FURY;

  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_CEGILUNE_BLADE) &&
      affected_by_spell(target, SPELL_ILIENZES_FLAME_SWORD) &&
      npc_has_spell_slot(ch, SPELL_CEGILUNE_BLADE) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_CEGILUNE_BLADE;

  if(!IS_FIGHTING(ch) && !spl && (ch == target))
  {
    if(!spl && !affected_by_spell(target, SPELL_BALADORS_PROTECTION) &&
        npc_has_spell_slot(ch, SPELL_BALADORS_PROTECTION))
      spl = SPELL_BALADORS_PROTECTION;

    if(!spl && !affected_by_spell(target, SPELL_FERRIX_PRECISION) &&
        npc_has_spell_slot(ch, SPELL_FERRIX_PRECISION))
      spl = SPELL_FERRIX_PRECISION;
  }

  if(!spl && IS_AFFECTED(ch, AFF_BLIND) &&
      room_has_valid_exit(ch->in_room) && !number(0, 5) && !fear_check(ch) )
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    P_char   tch;
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastReaverSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in CastReaverSpell\r\n", ch);
    }
  }

  if((victim == ch) || helping)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);

  n_attk = NumAttackers(ch);

  if((n_attk > 1) || has_help(target))
  {
    if(!number(0, 8))
    {
      for (tmp = world[ch->in_room].people; tmp && !spl; tmp = tmp->next_in_room)
      {
        if(!((ch == tmp->specials.fighting) || are_together(target, tmp)))
          continue;

        if(!spl && npc_has_spell_slot(ch, SPELL_FEEBLEMIND) && !(tmp == ch)
            && !affected_by_spell(tmp, SPELL_FEEBLEMIND)
            && (IS_MAGE(tmp) || IS_CLERIC(tmp)) && !number(0, 3))
        {
          spl = SPELL_FEEBLEMIND;
        }

        if(!spl && npc_has_spell_slot(ch, SPELL_RAY_OF_ENFEEBLEMENT) && !(tmp == ch)
            && !affected_by_spell(tmp, SPELL_RAY_OF_ENFEEBLEMENT) && !number(0, 3))
        {
          spl = SPELL_RAY_OF_ENFEEBLEMENT;
        }
      }
    }

    if(!spl &&
       !number(0, 9) &&
       npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC))
    {
      tmp = FindDispelTarget(ch, lvl);
      if(tmp && !(tmp == ch))
        spl = SPELL_DISPEL_MAGIC;
      else
        tmp = NULL;
    }

    if(spl && ch && tmp)
      return (MobCastSpell(ch, tmp, 0, spl, lvl));

    if(!spl && npc_has_spell_slot(ch, SPELL_CHAIN_LIGHTNING) &&
        !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) && !number(0, 6))
    {
      spl = SPELL_CHAIN_LIGHTNING;
    }
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_IMMOLATE) && number(0, 2))
    spl = SPELL_IMMOLATE;

  if(!spl && npc_has_spell_slot(ch, SPELL_RAY_OF_ENFEEBLEMENT) &&
      number(0, 2) && !affected_by_spell(target, SPELL_RAY_OF_ENFEEBLEMENT))
  {
    spl = SPELL_RAY_OF_ENFEEBLEMENT;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_FIREBALL) && number(0, 2))
    spl = SPELL_FIREBALL;

  if(!spl && npc_has_spell_slot(ch, SPELL_CHILL_TOUCH) && (!IS_GLOBED(target) || !IS_MINGLOBED(target))
     && !number(0, 2))
  {
    spl = SPELL_CHILL_TOUCH;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT) && (!IS_GLOBED(target) || !IS_MINGLOBED(target))
     && !number(0, 2))
  {
    spl = SPELL_LIGHTNING_BOLT;
  }

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  /*
   * reaver... randomly teleports if he is fighting and did not choose any other spell to cast
   * and his hp are below 1/8 and he rolls 1d15 and room is portable. *Alv*
   * the chance is really low.
   */

  if(!spl)
  {
    target = ch;
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
  }
  if(!spl && npc_has_spell_slot(ch, SPELL_TELEPORT) &&
      (dam > GET_MAX_HIT(ch) * 7 / 8) && (number(0, 15) == 3) &&
      !IS_SET(world[ch->in_room].room_flags, NO_TELEPORT))
    spl = SPELL_TELEPORT;

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  return (FALSE);

}

bool CastRangerSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL;
  P_obj    wpn;
  int      dam = 0, lvl = 0, spl = 0;

  if(IS_CASTING(ch))
  {
    return FALSE;
  }

  lvl = GET_LEVEL(ch);

  if(helping)
  {
    dam = GET_MAX_HIT(victim) - GET_HIT(victim);
    target = victim;
  }
  else
  {
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    target = ch;
  }

  /* make sure I'm even able to cast in this room! */

  if(MobShouldFlee(ch) &&
     !number(0, 9)) // Rangers are good hitters. No need to flee quickly.
  {
    do_flee(ch, 0, 0);
    return false;
  }
  else if(MobShouldFlee(ch))
  {
    return false;
  }
  
  if(affected_by_spell(ch, SKILL_BERSERK))
    return FALSE;

  /* always cast blur/dazzle if have them, since they rock all ass */

  /* well okay, not ALWAYS */

/*
  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_HEALING_BLADE) &&
      (wpn = ch->equipment[WIELD]) &&
      (wpn->type == ITEM_WEAPON) && IS_SWORD(wpn) &&
      npc_has_spell_slot(ch, SPELL_HEALING_BLADE) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_HEALING_BLADE;
*/
  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_DAZZLE) &&
      !IS_AFFECTED4(target, AFF4_DAZZLER) &&
      npc_has_spell_slot(ch, SPELL_DAZZLE) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_DAZZLE;

  if(!spl && (ch == target) &&
      !affected_by_spell(target, SPELL_BLUR) &&
      !IS_AFFECTED3(target, AFF3_BLUR) &&
      npc_has_spell_slot(ch, SPELL_BLUR) &&
      (!IS_FIGHTING(ch) || number(0, 1)))
    spl = SPELL_BLUR;

  if(!IS_FIGHTING(ch) || (number(0, 2) == 1))
  {
    if(!spl && !affected_by_spell(target, SPELL_HASTE) &&
        !IS_AFFECTED(target, AFF_HASTE) &&
        npc_has_spell_slot(ch, SPELL_HASTE))
      spl = SPELL_HASTE;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_INVISIBLE) && !IS_FIGHTING(target)
      && !IS_FIGHTING(ch) && !IS_AFFECTED2(target, AFF2_MINOR_INVIS)
      && !IS_AFFECTED(target, AFF_INVISIBLE)
      && !IS_ACT(target, ACT_TEACHER)
      && (!IS_NPC(target) || !mob_index[GET_RNUM(target)].qst_func)
      && (GET_HIT(target) < GET_MAX_HIT(target)) && !CHAR_IN_TOWN(ch)
      && !CHAR_IN_TOWN(target) && ((target == ch) && !number(0, 9)))
    spl = SPELL_INVISIBLE;

  if(!IS_FIGHTING(ch))
  {
    if( !spl && !IS_AFFECTED(target, AFF_BARKSKIN) && npc_has_spell_slot(ch, SPELL_BARKSKIN)
      && GET_RACE(target) != RACE_ANIMAL )
    {
      spl = SPELL_BARKSKIN;
    }

    if(!spl && !affected_by_spell(target, SPELL_BLESS) &&
        npc_has_spell_slot(ch, SPELL_BLESS) &&
        GET_RACE(target) != RACE_ANIMAL)
      spl = SPELL_BLESS;

    if(!spl && npc_has_spell_slot(ch, SPELL_PROTECT_FROM_EVIL) &&
        !IS_AFFECTED(target, AFF_PROTECT_EVIL))
      spl = SPELL_PROTECT_FROM_EVIL;

    if(!spl && !IS_AFFECTED(ch, AFF_SENSE_LIFE) && (ch == target) &&
        npc_has_spell_slot(ch, SPELL_SENSE_LIFE))
      spl = SPELL_SENSE_LIFE;

  }

  if(!spl && IS_AFFECTED(ch, AFF_BLIND) &&
      room_has_valid_exit(ch->in_room) && !number(0, 4) && !fear_check(ch) )
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }



  /* cure light is some weak shit, give rangers a chance to cast other stuff/track/whatever */

  if(!spl && npc_has_spell_slot(ch, SPELL_CURE_LIGHT) &&
      dam && !IS_FIGHTING(ch) && !number(0, 3))
    spl = SPELL_CURE_LIGHT;

  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    P_char   tch;
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastRangerSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in random number crap\r\n", ch);
    }
  }
  if((victim == ch) || helping)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);

  if(((NumAttackers(ch) >= 1) || has_help(target) ||
       no_chars_in_room_deserve_helping(ch)) &&
      !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_FIRESTORM))
      spl = SPELL_FIRESTORM;
  }
  /* prefer lightning bolt since it's lower circle than call lightning and
     does same damage */

  // never mind, rangers no longer get call lightning

  if(!spl && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT) && !IS_GLOBED(target) && number(0, 2))
    spl = SPELL_LIGHTNING_BOLT;

/*  if(!spl && npc_has_spell_slot(ch, SPELL_CALL_LIGHTNING) && !IS_GLOBED(target))
    spl = SPELL_CALL_LIGHTNING;*/

  if(!spl && npc_has_spell_slot(ch, SPELL_DISPEL_EVIL) && !IS_EVIL(ch) && IS_EVIL(target)
     && (!IS_GLOBED(target) || !IS_MINGLOBED(target)) && number(0, 1))
  {
    spl = SPELL_DISPEL_EVIL;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_CHILL_TOUCH) && (!IS_GLOBED(target) || !IS_MINGLOBED(target))
     && !number(0, 2))
  {
    spl = SPELL_CHILL_TOUCH;
  }

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  /*
   * well, time to scrape the bottom of the barrel
   */

  if(!spl)
  {
    target = ch;
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
  }
  if(!spl && npc_has_spell_slot(ch, SPELL_CURE_LIGHT) && (dam > 8) &&
      (number(0, 15) == 3))
    spl = SPELL_CURE_LIGHT;

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

/*
   if(!spl && (ch == victim))
   return (FALSE);

   if(!victim)
   target = ch->specials.fighting;
   else
   target = victim;

   if(!spl && npc_has_spell_slot(ch, SPELL_CAUSE_LIGHT) && (!IS_GLOBED(target) || !IS_MINGLOBED(target))
      && (GET_HIT(target) < 10))
   {
     spl = SPELL_CAUSE_LIGHT;
   }

   if(spl && ch && target)
   return (MobCastSpell(ch, target, 0, spl, lvl));
 */

  return (FALSE);
}


bool CastWarlockSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL;
  int      dam = 0, lvl = 0, spl = 0;

  /* make sure I'm even able to cast in this room! */

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }

  lvl = GET_LEVEL(ch);

  if(helping)
  {
    dam = GET_MAX_HIT(victim) - GET_HIT(victim);
    target = victim;
  }
  else
  {
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    target = ch;
  }

  // blind?  sweet lord!

  if(IS_AFFECTED(target, AFF_BLIND) &&
      npc_has_spell_slot(ch, SPELL_CURE_BLIND) &&
      (((ch == target) && number(0, 1)) || !IS_FIGHTING(ch)))
  {
    if(npc_has_spell_slot(ch, SPELL_CURE_BLIND))
      spl = SPELL_CURE_BLIND;
    else if(ch == target && !fear_check(ch) )
    {
      do_flee(ch, 0, 0);
      return FALSE;
    }
  }

  // cursed?  oh no!

  if(!spl && affected_by_spell(target, SPELL_CURSE) &&
      npc_has_spell_slot(ch, SPELL_REMOVE_CURSE) &&
      (!IS_FIGHTING(ch) || !number(0, 5)))
    spl = SPELL_REMOVE_CURSE;

  // lifelust is your buddy

  if(!spl && (!IS_FIGHTING(ch) || !number(0, 3)) &&
      !affected_by_spell(target, SPELL_LIFELUST) &&
      npc_has_spell_slot(ch, SPELL_LIFELUST))
    spl = SPELL_LIFELUST;

  // add checking for NPC corpses to use for unmaking..

  if(!spl && (!IS_FIGHTING(ch) || (number(1, 5) == 3)))
  {
    if((ch == target) &&
        !affected_by_spell(ch, SPELL_SHADOW_VISION) &&
        npc_has_spell_slot(ch, SPELL_SHADOW_VISION))
      spl = SPELL_SHADOW_VISION;
    else
      if(!IS_AFFECTED(target, AFF_PROT_FIRE) &&
          npc_has_spell_slot(ch, SPELL_PROTECT_FROM_FIRE))
      spl = SPELL_PROTECT_FROM_FIRE;
    else
      if(!IS_AFFECTED2(target, AFF2_PROT_COLD) &&
          npc_has_spell_slot(ch, SPELL_PROTECT_FROM_COLD))
      spl = SPELL_PROTECT_FROM_COLD;
    else
      if(npc_has_spell_slot(ch, SPELL_PROTECT_FROM_LIVING) &&
          !IS_AFFECTED4(target, AFF4_PROT_LIVING))
      spl = SPELL_PROTECT_FROM_LIVING;
    else
      if(npc_has_spell_slot(ch, SPELL_PROTECT_FROM_GOOD) &&
          !IS_AFFECTED(target, AFF_PROTECT_GOOD))
      spl = SPELL_PROTECT_FROM_GOOD;
    else if(npc_has_spell_slot(ch, SPELL_GREATER_HEAL_UNDEAD) && (dam > 275))
    {
      spl = SPELL_GREATER_HEAL_UNDEAD;
    }
  }

  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    P_char   tch;
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastWarlockSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in random number crap\r\n", ch);
    }
  }
  if((victim == ch) || helping)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);

  // offensive area check

  if(((NumAttackers(ch) >= 1) || has_help(target) ||
       no_chars_in_room_deserve_helping(ch)) &&
      !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_ENTROPY_STORM))
      spl = SPELL_ENTROPY_STORM;
  }

  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  // single-target spells..  alas, many of these are non-undead only

  if(!spl && npc_has_spell_slot(ch, SPELL_CHANNEL_NEG_ENERGY) &&
      spell_can_affect_char(target, SPELL_CHANNEL_NEG_ENERGY))
    spl = SPELL_CHANNEL_NEG_ENERGY;

  if(!spl && npc_has_spell_slot(ch, SPELL_FROSTBITE) &&
      spell_can_affect_char(target, SPELL_FROSTBITE))
    spl = SPELL_FROSTBITE;

  if(!spl && npc_has_spell_slot(ch, SPELL_PURGE_LIVING) &&
      !IS_UNDEAD(target) && spell_can_affect_char(target, SPELL_PURGE_LIVING))
    spl = SPELL_PURGE_LIVING;

  if(!spl && npc_has_spell_slot(ch, SPELL_DISPEL_GOOD) &&
      IS_GOOD(target) && !IS_GOOD(ch) &&
      spell_can_affect_char(target, SPELL_DISPEL_GOOD))
    spl = SPELL_DISPEL_GOOD;

  if(!spl && npc_has_spell_slot(ch, SPELL_INVOKE_NEG_ENERGY) &&
      spell_can_affect_char(target, SPELL_INVOKE_NEG_ENERGY))
    spl = SPELL_INVOKE_NEG_ENERGY;

  // now the sucky spells

  if(!spl && npc_has_spell_slot(ch, SPELL_ALTER_ENERGY_POLARITY) &&
      !affected_by_spell(target, SPELL_ALTER_ENERGY_POLARITY) &&
      !number(0, 1))
    spl = SPELL_ALTER_ENERGY_POLARITY;

  if(!spl && npc_has_spell_slot(ch, SPELL_DISPEL_LIFEFORCE) &&
      !affected_by_spell(target, SPELL_DISPEL_LIFEFORCE) &&
      !IS_UNDEAD(target) && !number(0, 1))
    spl = SPELL_DISPEL_LIFEFORCE;

  if(!spl && npc_has_spell_slot(ch, SPELL_NETHER_TOUCH) &&
      !affected_by_spell(target, SPELL_NETHER_TOUCH) &&
      !IS_UNDEAD(target) && !number(0, 1))
    spl = SPELL_NETHER_TOUCH;

  if(!spl && npc_has_spell_slot(ch, SPELL_UNHOLY_WIND) &&
      !IS_UNDEAD(target) && spell_can_affect_char(target, SPELL_UNHOLY_WIND)
      && !number(0, 1))
    spl = SPELL_UNHOLY_WIND;

  // drains movement, ultra-sucky

  if(!spl && npc_has_spell_slot(ch, SPELL_DEVITALIZE) &&
      GET_VITALITY(target) && !number(0, 3))
    spl = SPELL_DEVITALIZE;

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  return (FALSE);
}

bool CastDruidSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL;
  int      dam = 0, lvl = 0, spl = 0;

  /* make sure I'm even able to cast in this room! */

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }

  lvl = GET_LEVEL(ch);

  if(helping)
  {
    dam = GET_MAX_HIT(victim) - GET_HIT(victim);
    target = victim;
  }
  else
  {
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    target = ch;
  }

  if(!spl &&
    (!IS_FIGHTING(ch) || !number(0, 2)))
        spl = pick_best_skin_spell(ch, target);
        
  if(!spl && IS_AFFECTED(target, AFF_BLIND) &&
      npc_has_spell_slot(ch, SPELL_NATURES_TOUCH))
  {
    //natures touch ftw

    if(npc_has_spell_slot(ch, SPELL_NATURES_TOUCH))
      spl = SPELL_NATURES_TOUCH;
  }
  else if(!spl && IS_AFFECTED(ch, AFF_BLIND) &&
           room_has_valid_exit(ch->in_room) && !fear_check(ch) )
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  if(!spl && !IS_FIGHTING(ch))
  {
    if(npc_has_spell_slot(ch, SPELL_NATURES_TOUCH) && (dam > 90))
      spl = SPELL_NATURES_TOUCH;
    else if( !IS_AFFECTED(target, AFF_BARKSKIN) && npc_has_spell_slot(ch, SPELL_BARKSKIN)
      && GET_RACE(target) != RACE_ANIMAL )
      spl = SPELL_BARKSKIN;
  }

  if(!spl && (affected_by_spell(target, SPELL_POISON) || IS_AFFECTED2(target,
                                                                       AFF2_POISONED))
      && npc_has_spell_slot(ch, SPELL_AID))
    spl = SPELL_AID;

  if(!spl && (ch == target) && (!IS_FIGHTING(ch) || number(0, 1)) &&
      !IS_AFFECTED4(ch, AFF4_REGENERATION) &&
      !affected_by_spell(ch, SPELL_REGENERATION))
    spl = SPELL_REGENERATION;

  if(!spl && (!IS_FIGHTING(ch) || (number(1, 5) == 3)))
  {
    if( IS_AFFECTED(target, AFF_BARKSKIN) && !has_skin_spell(target) && npc_has_spell_slot(ch, SPELL_IRONWOOD) )
      spl = SPELL_IRONWOOD;
    else
      if(!affected_by_spell(ch, SPELL_STORMSHIELD) && ch->equipment[WEAR_SHIELD] &&
          npc_has_spell_slot(ch, SPELL_STORMSHIELD))
      spl = SPELL_STORMSHIELD;
    else
      if(!affected_by_spell(ch, SPELL_ENDURANCE) &&
          npc_has_spell_slot(ch, SPELL_ENDURANCE))
      spl = SPELL_ENDURANCE;
    else
      if(!affected_by_spell(ch, SPELL_ANIMAL_VISION) &&
          npc_has_spell_slot(ch, SPELL_ANIMAL_VISION))
      spl = SPELL_ANIMAL_VISION;
        else
          if(!affected_by_spell(ch, SPELL_ELEMENTAL_AURA) && (world[ch->in_room].sector_type == SECT_FIREPLANE ||
                  world[ch->in_room].sector_type == SECT_WATER_PLANE || world[ch->in_room].sector_type == SECT_AIR_PLANE ||
                  world[ch->in_room].sector_type == SECT_EARTH_PLANE))
          spl = SPELL_ELEMENTAL_AURA;
  }

  // more natures touching? why, thank you!

  if(!spl && !IS_FIGHTING(ch) && (dam > 0))
  {
    if(npc_has_spell_slot(ch, SPELL_NATURES_TOUCH))
      spl = SPELL_NATURES_TOUCH;
  }
  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    P_char   tch;
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastDruidSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in random number crap\r\n", ch);
    }
  }
  if((victim == ch) || helping)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);

  if(((NumAttackers(ch) > 1) || has_help(target) ||
       no_chars_in_room_deserve_helping(ch)) &&
      !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_NOVA) && !ch->specials.z_cord &&
        number(0, 2))
      spl = SPELL_NOVA;

    if(!spl && npc_has_spell_slot(ch, SPELL_FIRESTORM))
      spl = SPELL_FIRESTORM;

        if(!spl && npc_has_spell_slot(ch, SPELL_AWAKEN_FOREST) && world[ch->in_room].sector_type == SECT_FOREST)
      spl = SPELL_AWAKEN_FOREST;

        if(!spl && npc_has_spell_slot(ch, SPELL_HURRICANE) && number(0, 3))
          spl = SPELL_HURRICANE;

    if(!spl && npc_has_spell_slot(ch, SPELL_EARTHQUAKE) &&
        (number(0, 3) == 2))
      spl = SPELL_EARTHQUAKE;
  }
  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && OUTSIDE(ch) && npc_has_spell_slot(ch, SPELL_CALL_LIGHTNING) &&
    number(0,1))
    spl = SPELL_CALL_LIGHTNING;

  if(!spl && npc_has_spell_slot(ch, SPELL_ELEMENTAL_SWARM))
        spl = SPELL_ELEMENTAL_SWARM;

  if(!spl && npc_has_spell_slot(ch, SPELL_CDOOM))
    spl = SPELL_CDOOM;

  if(!spl && npc_has_spell_slot(ch, SPELL_BLOODTOSTONE))
        spl = SPELL_BLOODTOSTONE;

  if(!spl && npc_has_spell_slot(ch, SPELL_ACID_STREAM))
    spl = SPELL_ACID_STREAM;

  if(!spl && npc_has_spell_slot(ch, SPELL_SUNRAY))
    spl = SPELL_SUNRAY;

  if(!spl && npc_has_spell_slot(ch, SPELL_CYCLONE))
    spl = SPELL_CYCLONE;

  if(!spl && npc_has_spell_slot(ch, SPELL_GROW_SPIKES))
        spl = SPELL_GROW_SPIKES;

  if(!spl && npc_has_spell_slot(ch, SPELL_EARTHEN_MAUL))
    spl = SPELL_EARTHEN_MAUL;

  if(!spl && npc_has_spell_slot(ch, SPELL_DISEASE) && !affected_by_spell(target, SPELL_DISEASE))
    spl = SPELL_DISEASE;


//  if(!spl && !IS_GLOBED(target) && npc_has_spell_slot(ch, SPELL_CALL_LIGHTNING))
//    spl = SPELL_CALL_LIGHTNING;

  if(!spl && !IS_GLOBED(target) && npc_has_spell_slot(ch, SPELL_LIGHTNING_BOLT))
    spl = SPELL_LIGHTNING_BOLT;

  if(!spl && npc_has_spell_slot(ch, SPELL_POISON) && !number(0, 4))
    spl = SPELL_POISON;

  if(!spl && npc_has_spell_slot(ch, SPELL_STICKS_TO_SNAKES) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
    spl = SPELL_STICKS_TO_SNAKES;

  if(spl && ch && target)
  {
    P_char nuke_target = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);
    return (MobCastSpell(ch, nuke_target ? nuke_target : target, 0, spl, lvl));
  }

  /*
   * well, time to scrape the bottom of the barrel how about no?
   */

  if(!spl)
    target = ch;


  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

/*
   if(!spl && (ch == victim))
     return (FALSE);

   if(!victim)
     target = ch->specials.fighting;
   else
     target = victim;

   if(!spl && npc_has_spell_slot(ch, SPELL_CAUSE_LIGHT) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
     spl = SPELL_CAUSE_LIGHT;

   if(spl && ch && target)
     return (MobCastSpell(ch, target, 0, spl, lvl));
 */

  return (FALSE);
}

bool CastShamanSpell(P_char ch, P_char victim, int helping)
{
  P_char target = NULL, tempch;
  P_char nuke_target;

  /*
   * P_obj   obj=NULL;
   */
  int sect, dam = 0, lvl = 0, spl = 0;

  // Return FALSE because dead people don't cast.
  if( !IS_ALIVE(ch) )
  {
    debug( "CastShamanSpell: ch '%s' is not alive.", ch ? J_NAME(ch) : "NULL" );
    return FALSE;
  }

  lvl = GET_LEVEL(ch);

  // No one to help
  if( helping && !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  if( helping )
  {
    dam = GET_MAX_HIT(victim) - GET_HIT(victim);
    target = victim;
  }
  else
  {
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    target = ch;
  }

  /* make sure I'm even able to cast in this room! */
  // Cast in this room to remove afflictions, unless it is silent
  if( !IS_SET(world[ch->in_room].room_flags, (NO_MAGIC | ROOM_SILENT)) )
  {
    if( npc_has_spell_slot(ch, SPELL_RESTORATION)
      && DO_SHAM_RESTORATION(target) && !number(0, 2) )
    {
      spl = SPELL_RESTORATION;
    }
    else if( !spl && npc_has_spell_slot(ch, SPELL_PURIFY_SPIRIT)
      && (IS_AFFECTED2(target, AFF2_POISONED)
      || affected_by_spell(target, SPELL_DISEASE)
      || affected_by_spell(target, SPELL_PLAGUE)
      || affected_by_spell(target, SPELL_MALISON)
      || affected_by_spell(target, SPELL_CURSE)
      || get_scheduled(target, event_torment_spirits)
      || IS_AFFECTED(target, AFF_BLIND)))
    {
      spl = SPELL_PURIFY_SPIRIT;
    }
    if( spl )
    {
      return (MobCastSpell(ch, target, 0, spl, lvl));
    }
  }

  // If the shaman was able to remove the affliction, time to check if other conditions
  //   should cause the shaman to flee.
  if( MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  // Sort of weird here, but the idea is to not always cast healing
  //   spells when not fighting and wounded.
  if( !spl && (!IS_FIGHTING(ch) || (number(0, 9) == 8))
    && GET_LEVEL(target) > 10 && GET_RACE(target) != RACE_ANIMAL )
  {
    if( npc_has_spell_slot(ch, SPELL_RESTORATION) && DO_SHAM_RESTORATION(target) )
    {
      spl = SPELL_RESTORATION;
    }
    else if( npc_has_spell_slot(ch, SPELL_GREATER_MENDING) && dam > 75 )
    {
      spl = SPELL_GREATER_MENDING;
    }
    /* wellness is an option if no PCs/charmed PC pets are around */
    /* alas, wellness is currently in the same circle as greater mending.. but
       it can't hurt to check, eh.  <-- skipped and code deleted? */
    else if( npc_has_spell_slot(ch, SPELL_MENDING) && (dam > 40) )
    {
      spl = SPELL_MENDING;
    }
    else if( npc_has_spell_slot(ch, SPELL_LESSER_MENDING) && (dam > 20) )
    {
      spl = SPELL_LESSER_MENDING;
    }
  }

  if( !IS_FIGHTING(ch) && !spl )
  {
    if( !IS_AFFECTED3(target, AFF3_GR_SPIRIT_WARD) && !IS_AFFECTED3(target, AFF3_SPIRIT_WARD)
        && npc_has_spell_slot(ch, SPELL_GREATER_SPIRIT_WARD) )
    {
      spl = SPELL_GREATER_SPIRIT_WARD;
    }
    else if( !IS_AFFECTED3(target, AFF3_SPIRIT_WARD) && !IS_AFFECTED3(target, AFF3_GR_SPIRIT_WARD)
      && npc_has_spell_slot(ch, SPELL_SPIRIT_WARD) )
    {
      spl = SPELL_SPIRIT_WARD;
    }
    else if( !IS_AFFECTED(target, AFF_ARMOR) && npc_has_spell_slot(ch, SPELL_SPIRIT_ARMOR) )
    {
      spl = SPELL_SPIRIT_ARMOR;
    }
    else if( !IS_AFFECTED(target, AFF_PROT_FIRE) && npc_has_spell_slot(ch, SPELL_FIRE_WARD) )
    {
      spl = SPELL_FIRE_WARD;
    }
    else if( !IS_AFFECTED2(target, AFF2_PROT_COLD) && npc_has_spell_slot(ch, SPELL_COLD_WARD) )
    {
      spl = SPELL_COLD_WARD;
    }
    else if( !affected_by_spell(ch, SPELL_ELEM_AFFINITY) && target == ch
      && npc_has_spell_slot(ch, SPELL_ELEM_AFFINITY) )
    {
      spl = SPELL_ELEM_AFFINITY;
    }
    else if( !affected_by_spell(target, SPELL_GREATER_SPIRIT_SIGHT)
      && !IS_AFFECTED(target, AFF_DETECT_INVISIBLE)
      && npc_has_spell_slot(ch, SPELL_GREATER_SPIRIT_SIGHT) )
    {
      spl = SPELL_GREATER_SPIRIT_SIGHT;
    }
    // Let us cast greater ravenflight first.
    else if( npc_has_spell_slot(ch, SPELL_GREATER_RAVENFLIGHT) && !IS_AFFECTED(target, AFF_FLY) )
    {
      spl = SPELL_GREATER_RAVENFLIGHT;
    }
    else if( npc_has_spell_slot(ch, SPELL_RAVENFLIGHT) && !IS_AFFECTED(target, AFF_FLY) )
    {
      spl = SPELL_RAVENFLIGHT;
    }
    else if( !helping && npc_has_spell_slot(ch, SPELL_ELEPHANTSTRENGTH)
      && !affected_by_spell(target, SPELL_ELEPHANTSTRENGTH)
      && !affected_by_spell(target, SPELL_BEARSTRENGTH)
      && !affected_by_spell(target, SPELL_MOUSESTRENGTH) )
    {
      spl = SPELL_ELEPHANTSTRENGTH;
    }
    else if( !helping && npc_has_spell_slot(ch, SPELL_LIONRAGE)
      && !affected_by_spell(target, SPELL_LIONRAGE)
      && !affected_by_spell(target, SPELL_SHREWTAMENESS) )
    {
      spl = SPELL_LIONRAGE;
    }
    else if( !helping && npc_has_spell_slot(ch, SPELL_BEARSTRENGTH)
      && !affected_by_spell(target, SPELL_BEARSTRENGTH)
      && !affected_by_spell(target, SPELL_ELEPHANTSTRENGTH)
      && !affected_by_spell(target, SPELL_MOUSESTRENGTH) )
    {
      spl = SPELL_BEARSTRENGTH;
    }
    else if( npc_has_spell_slot(ch, SPELL_HAWKVISION)
      && !affected_by_spell(target, SPELL_HAWKVISION)
      && !affected_by_spell(target, SPELL_MOLEVISION)
      && !IS_AFFECTED4(ch, AFF4_HAWKVISION) )
    {
      spl = SPELL_HAWKVISION;
    }
    else if( npc_has_spell_slot(ch, SPELL_PANTHERSPEED)
      && !affected_by_spell(target, SPELL_PANTHERSPEED)
      && !affected_by_spell(target, SPELL_WOLFSPEED)
      && !affected_by_spell(target, SPELL_SNAILSPEED) )
    {
      spl = SPELL_PANTHERSPEED;
    }
    /* summon us some beasts */
    else if( !GET_MASTER(ch) && (!IS_AFFECTED(ch, AFF_HIDE)
      || IS_SET(ch->specials.act, ACT_SENTINEL)) && !helping )
    {
      sect = world[ch->in_room].sector_type;

      if( ((sect == SECT_FIELD) || (sect == SECT_FOREST)
        || (sect == SECT_HILLS) || (sect == SECT_MOUNTAIN)
        || (sect == SECT_UNDRWLD_CITY) || (sect == SECT_UNDRWLD_INSIDE)
        || (sect == SECT_UNDRWLD_WILD)) && can_summon_beast(ch, lvl)
        && GET_VNUM(ch) != WH_HIGH_PRIEST_VNUM )
      {
        if( npc_has_spell_slot(ch, SPELL_GREATER_SUMMON_BEAST) )
        {
          spl = SPELL_GREATER_SUMMON_BEAST;
        }
        else if(npc_has_spell_slot(ch, SPELL_SUMMON_BEAST))
        {
          spl = SPELL_SUMMON_BEAST;
        }
      }
    }
    if( !spl && !helping )
    {
      P_char   tch;
      int      numb = 0, lucky, curr = 0;

      for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
      {
        if( (ch != tch) && char_deserves_helping(ch, tch, TRUE) )
        {
          numb++;
        }
      }

      if( numb )
      {
        if(numb == 1)
        {
          lucky = 1;
        }
        else
        {
          lucky = number(1, numb);
        }

        for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
        {
          if( (ch != tch) && char_deserves_helping(ch, tch, TRUE) )
          {
            curr++;
            if( curr == lucky )
            {
              return (CastShamanSpell(ch, tch, TRUE));
            }
          }
        }
        debug("'%s' %d: Error in random number crap\r\n", J_NAME(ch), GET_VNUM(ch) );
      }
    }
  }

  if( spl )
  {
    if( IS_ALIVE(target) && helping )
    {
      return (MobCastSpell(ch, target, 0, spl, lvl));
    }
    else
    {
      return (MobCastSpell(ch, ch, 0, spl, lvl));
    }
  }

  // Switch to the offensive!
  if( helping )
  {
    return FALSE;
  }
  if( victim == ch )
  {
    return FALSE;
  }
  if( !victim )
  {
    // Yes, we want victim set too, because we jump back and forth between offensive/defensive.
    target = victim = ch->specials.fighting;
  }
  else
  {
    target = victim;
  }
  if( !IS_ALIVE(target) )
  {
    return FALSE;
  }

  // Just in case the mob is multiclass, let us hit the target
  // with a dispel magic. Jan08 -Lucrot
  if( npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) && !(number(0, 9))
    && no_chars_in_room_deserve_helping(ch)
    && (affected_by_spell(victim, SPELL_VITALITY)
    || IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY)
    || IS_AFFECTED4(victim, AFF4_SANCTUARY))
    && (GET_LEVEL(ch) + 5) >= GET_LEVEL(target) )
  {
    spl = SPELL_DISPEL_MAGIC;
  }
  else if( ((NumAttackers(ch) > 1) || has_help(target)
    || no_chars_in_room_deserve_helping(ch))
    && !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) )
  {
    // let's assume these spells are never blockable by globe, shall we?
    if( npc_has_spell_slot(ch, SPELL_ELEM_FURY) && OUTSIDE(ch) && !number(0, 2) )
    {
      spl = SPELL_ELEM_FURY;
    }
    else if( npc_has_spell_slot(ch, SPELL_GREATER_EARTHEN_GRASP) && !number(0, 1) )
    {
      spl = SPELL_GREATER_EARTHEN_GRASP;
    }
    else if( npc_has_spell_slot(ch, SPELL_SCATHING_WIND) && !number(0, 1) )
    {
      spl = SPELL_SCATHING_WIND;
    }
    else if( npc_has_spell_slot(ch, SPELL_EARTHEN_RAIN) && OUTSIDE(ch) && !number(0, 1) )
    {
      spl = SPELL_EARTHEN_RAIN;
    }
    else if( npc_has_spell_slot(ch, SPELL_ELEM_FURY) )
    {
      spl = SPELL_ELEM_FURY;
    }
    // Why not?  it'll be fun
    else if( !(number(0, 15)) && npc_has_spell_slot(ch, SPELL_GREATER_PYTHONSTING) )
    {
      spl = SPELL_GREATER_PYTHONSTING;
    }
  }
  if( spl )
  {
    return (MobCastSpell(ch, target, 0, spl, lvl));
  }

  /* 5% chance of casting something that disables the attacker a tad */
  if( !(number(0, 19)) )
  {
    // Randomly pick something, fall through to next if mob doesn't have slot available.
    switch( number(0, 4) )
    {
    case 0:
      if( npc_has_spell_slot(ch, SPELL_SHREWTAMENESS)
        && !affected_by_spell(target, SPELL_SHREWTAMENESS) )
      {
        spl = SPELL_SHREWTAMENESS;
      }
    case 1:
      if( !spl && npc_has_spell_slot(ch, SPELL_MOLEVISION)
        && !affected_by_spell(target, SPELL_MOLEVISION) )
      {
        spl = SPELL_MOLEVISION;
      }
    case 2:
      if( !spl && npc_has_spell_slot(ch, SPELL_MALISON)
        && !affected_by_spell(target, SPELL_MALISON) )
      {
        spl = SPELL_MALISON;
      }
    case 3:
      if( !spl && npc_has_spell_slot(ch, SPELL_PYTHONSTING)
        && !affected_by_spell(target, SPELL_POISON)
        && !IS_AFFECTED2(target, AFF2_POISONED) )
      {
        spl = SPELL_PYTHONSTING;
      }
    case 4:
      if( !spl && npc_has_spell_slot(ch, SPELL_MOUSESTRENGTH)
        && !affected_by_spell(target, SPELL_MOUSESTRENGTH) )
      {
        spl = SPELL_MOUSESTRENGTH;
      }
      break; // Stop here 'cause call of the wild is broken.
    case 5:
    case 6:
      if(!spl && npc_has_spell_slot(ch, SPELL_CALL_OF_THE_WILD) &&
          IS_PC(target) &&
          !IS_MORPH(target))
        spl = SPELL_CALL_OF_THE_WILD;
    default:
      break;
    }
  }

  if( spl )
  {}
  else if( !number(0, 2) && npc_has_spell_slot(ch, SPELL_FIREBRAND)
    && !ENJOYS_FIRE_DAM(target) && !IS_AFFECTED(target, AFF_PROT_FIRE) )
  {
    spl = SPELL_FIREBRAND;
  }
  else if( npc_has_spell_slot(ch, SPELL_CASCADING_ELEMENTAL_BEAM)
    && (IS_COLD_VULN(target) || COLDSHIELDED(target) || FIRESHIELDED(target)
    || LIGHTNINGSHIELDED(target) || number(0, 1)) )
  {
      spl = SPELL_CASCADING_ELEMENTAL_BEAM;
  }
  else if( npc_has_spell_slot(ch, SPELL_GASEOUS_CLOUD)
    && (affected_by_spell(target, SPELL_LIGHTNINGSHIELD) || number(0, 1)) )
  {
    spl = SPELL_GASEOUS_CLOUD;
  }
  // Molten spray does good damage versus undead (unless fire aura is up).
  else if( npc_has_spell_slot(ch, SPELL_MOLTEN_SPRAY)
    && IS_UNDEADRACE(target) && !IS_AFFECTED2(target, AFF2_FIRE_AURA) )
  {
    spl = SPELL_MOLTEN_SPRAY;
  }
  else if( npc_has_spell_slot(ch, SPELL_ARIEKS_SHATTERING_ICEBALL)
    && (IS_COLD_VULN(target) || number(0, 1)) )
  {
    spl = SPELL_ARIEKS_SHATTERING_ICEBALL;
  }
  // Corrosive blast damage is reduced if affected by a previous corrosive blast.
  else if( npc_has_spell_slot(ch, SPELL_CORROSIVE_BLAST)
    && !affected_by_spell(target, SPELL_CORROSIVE_BLAST) )
  {
    spl = SPELL_CORROSIVE_BLAST;
  }
  else if( npc_has_spell_slot(ch, SPELL_GREATER_SOUL_DISTURB)
    && spell_can_affect_char(target, SPELL_GREATER_SOUL_DISTURB) )
  {
    spl = SPELL_GREATER_SOUL_DISTURB;
  }
  else if( npc_has_spell_slot(ch, SPELL_SPIRIT_ANGUISH)
    && spell_can_affect_char(target, SPELL_SPIRIT_ANGUISH) )
  {
    spl = SPELL_SPIRIT_ANGUISH;
  }
  else if( npc_has_spell_slot(ch, SPELL_MOLTEN_SPRAY)
    && spell_can_affect_char(target, SPELL_MOLTEN_SPRAY) )
  {
    spl = SPELL_MOLTEN_SPRAY;
  }
  else if( npc_has_spell_slot(ch, SPELL_SCORCHING_TOUCH)
    && spell_can_affect_char(target, SPELL_SCORCHING_TOUCH) )
  {
    spl = SPELL_SCORCHING_TOUCH;
  }
  else if( npc_has_spell_slot(ch, SPELL_SCALDING_BLAST)
    && spell_can_affect_char(target, SPELL_SCALDING_BLAST) )
  {
    spl = SPELL_SCALDING_BLAST;
  }
  else if( npc_has_spell_slot(ch, SPELL_FLAMEBURST)
    && spell_can_affect_char(target, SPELL_FLAMEBURST))
  {
    spl = SPELL_FLAMEBURST;
  }

  /* heal up, the offense that's left sucks anyway */
  /* don't heal up all the time though..  the mob may still be doing
     good barehanded damage */
  if( !spl && !number(0, 9) )
  {
    // Ok, we only heal self at this point ffs (not the person we're fighting).
    target = ch;
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);

    if( npc_has_spell_slot(ch, SPELL_RESTORATION) && DO_SHAM_RESTORATION(target) )
    {
      spl = SPELL_RESTORATION;
    }
    else if( npc_has_spell_slot(ch, SPELL_GREATER_MENDING) && dam > 75 )
    {
      spl = SPELL_GREATER_MENDING;
    }
    else if( npc_has_spell_slot(ch, SPELL_WELLNESS) && (dam > 50) )
    {
      bool cast_it = TRUE;

      // For each person in the room,
      for( tempch = world[ch->in_room].people; tempch; tempch = tempch->next_in_room )
      {
        // If they are a PC or a PC pet.
        if( IS_PC(tempch) || IS_PC_PET(tempch) )
        {
          cast_it = FALSE;
          break;
        }
      }

      if( cast_it )
      {
        spl = SPELL_WELLNESS;
      }
    }
    // We have to start the 'else if' chain over because of the loop check for wellness spell.
    if( spl )
    {}
    else if( npc_has_spell_slot(ch, SPELL_MENDING) && (dam > 40) )
    {
      spl = SPELL_MENDING;
    }
    else if( npc_has_spell_slot(ch, SPELL_LESSER_MENDING) && (dam > 20) )
    {
      spl = SPELL_LESSER_MENDING;
    }
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_ICE_MISSILE)
    && spell_can_affect_char(victim, SPELL_ICE_MISSILE) )
  {
    spl = SPELL_ICE_MISSILE;
    target = victim;
  }

  if( spl && IS_ALIVE(target) )
  {
    if( IS_AGG_SPELL(spl) )
    {
      nuke_target = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);

      return (MobCastSpell(ch, nuke_target ? nuke_target : target, 0, spl, lvl));
    }
    else
    {
      return (MobCastSpell(ch, target, 0, spl, lvl));
    }
  }

/*
   if(!spl && (ch == victim))
     return (FALSE);

   if(!victim)
     target = ch->specials.fighting;
   else
     target = victim;

   if(!spl && npc_has_spell_slot(ch, SPELL_ICE_MISSILE) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
     spl = SPELL_ICE_MISSILE;

   if(spl && ch && target)
     return (MobCastSpell(ch, target, 0, spl, lvl));
 */

  return FALSE;
}

bool CastEtherSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL, tempch;
  int      sect;

  /*
   * P_obj   obj=NULL;
   */
  int      dam = 0, lvl = 0, spl = 0;


  lvl = GET_LEVEL(ch);

  if(helping && !victim)
    return FALSE;               /* no one to help */

  if(helping)
  {
    dam = GET_MAX_HIT(victim) - GET_HIT(victim);
    target = victim;
  }
  else
  {
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    target = ch;
  }

  /* make sure I'm even able to cast in this room! */

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }

  if(!spl && (affected_by_spell(target, SPELL_POISON) ||
               IS_AFFECTED2(target, AFF2_POISONED)) && npc_has_spell_slot(ch,
                                                                          SPELL_PURIFY_SPIRIT))
    spl = SPELL_PURIFY_SPIRIT;

  if(!spl && IS_AFFECTED(target, AFF_BLIND) &&
      npc_has_spell_slot(ch, SPELL_PURIFY_SPIRIT))
  {
    spl = SPELL_PURIFY_SPIRIT;
  }
  else if(!spl && IS_AFFECTED(ch, AFF_BLIND) &&
           room_has_valid_exit(ch->in_room) && !fear_check(ch) )
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  if((!IS_FIGHTING(ch) || !number(0, 3)) && !spl)
  {
    if(!affected_by_spell(ch, SPELL_ETHEREAL_FORM) &&
        npc_has_spell_slot(ch, SPELL_ETHEREAL_FORM) &&
        !IS_ACT(ch, ACT_TEACHER))
      spl = SPELL_ETHEREAL_FORM;

    /* sort of weird here, but the idea is to not always cast healing spells when not fighting and wounded */

    if(IS_FIGHTING(ch) || (number(0, 9) != 8))
    {
      if(!spl && npc_has_spell_slot(ch, SPELL_GREATER_ETHEREAL) && affected_by_spell(ch, SPELL_ETHEREAL_FORM) && (dam > 75))
        spl = SPELL_GREATER_ETHEREAL;


      if(!spl && npc_has_spell_slot(ch, SPELL_ETHEREAL_RECHARGE) && affected_by_spell(ch, SPELL_ETHEREAL_FORM) && (dam > 40))
        spl = SPELL_ETHEREAL_RECHARGE;

    }

    if(!spl && !IS_AFFECTED(target, AFF_ARMOR) &&
        npc_has_spell_slot(ch, SPELL_VAPOR_ARMOR))
      spl = SPELL_VAPOR_ARMOR;

    if(!spl && !affected_by_spell(target, SPELL_DETECT_MAGIC) &&
        npc_has_spell_slot(ch, SPELL_DETECT_MAGIC))
      spl = SPELL_DETECT_MAGIC;

    if(!spl && !affected_by_spell(target, SPELL_STORM_EMPATHY) &&
        npc_has_spell_slot(ch, SPELL_STORM_EMPATHY))
      spl = SPELL_STORM_EMPATHY;

        if(!spl && !affected_by_spell(target, SPELL_VAPOR_STRIKE) &&
        npc_has_spell_slot(ch, SPELL_VAPOR_STRIKE))
      spl = SPELL_VAPOR_STRIKE;

    if(!spl && !affected_by_spell(ch, SPELL_WIND_RAGE) &&
        target == ch &&
        npc_has_spell_slot(ch, SPELL_WIND_RAGE))
      spl = SPELL_WIND_RAGE;

        if(!spl && !affected_by_spell(ch, SPELL_PLANETARY_ALIGNMENT) &&
        target == ch &&
        npc_has_spell_slot(ch, SPELL_PLANETARY_ALIGNMENT))
      spl = SPELL_PLANETARY_ALIGNMENT;

    if(!spl && !affected_by_spell(target, SPELL_FAERIE_SIGHT) &&
        npc_has_spell_slot(ch, SPELL_FAERIE_SIGHT) &&
        !IS_FIGHTING(ch))
      spl = SPELL_FAERIE_SIGHT;

    if(!spl && !IS_FIGHTING(ch) && npc_has_spell_slot(ch, SPELL_BLUR) &&
        !IS_AFFECTED3(ch, AFF3_BLUR) && target == ch)
      spl = SPELL_BLUR;


    if(!spl && !helping && !IS_FIGHTING(ch))
    {
      P_char   tch;
      int      numb = 0, lucky, curr = 0;

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
          numb++;

      if(numb)
      {
        if(numb == 1)
          lucky = 1;
        else
          lucky = number(1, numb);

        for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
        {
          if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
          {
            curr++;
            if(curr == lucky)
              return (CastEtherSpell(ch, tch, TRUE));
          }
        }

        send_to_char("error in random number crap\r\n", ch);
      }
    }
  }

  if(spl && ch)
  {
    if(target && helping)
      return (MobCastSpell(ch, target, 0, spl, lvl));
    else
      return (MobCastSpell(ch, ch, 0, spl, lvl));
  }

  if(helping)
    return FALSE;

  if(victim == ch)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);

  if(((NumAttackers(ch) > 1) || has_help(target) ||
       no_chars_in_room_deserve_helping(ch)) &&
      !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {

    // let's assume these spells are never blockable by globe, shall we?
    if(!spl && npc_has_spell_slot(ch, SPELL_COSMIC_RIFT))
      spl = SPELL_COSMIC_RIFT;

    if(!spl && npc_has_spell_slot(ch, SPELL_POLAR_VORTEX))
      spl = SPELL_POLAR_VORTEX;

    if(!spl && npc_has_spell_slot(ch, SPELL_SUPERNOVA) && OUTSIDE(ch))
      spl = SPELL_SUPERNOVA;

    if(!spl && npc_has_spell_slot(ch, SPELL_RING_LIGHTNING))
      spl = SPELL_RING_LIGHTNING;

    /* why not?  it'll be fun */

    if(!spl && (number(0, 15) == 6) &&
        npc_has_spell_slot(ch, SPELL_ETHEREAL_DISCHARGE) && affected_by_spell(ch, SPELL_ETHEREAL_FORM))
      spl = SPELL_ETHEREAL_DISCHARGE;

    if(!spl && npc_has_spell_slot(ch, SPELL_TEMPEST))
      spl = SPELL_TEMPEST;
  }
  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !number(0, 9) &&
      (!ch->equipment[PRIMARY_WEAPON] || has_wind_blade_wielded(ch)) &&
      npc_has_spell_slot(ch, SPELL_WIND_BLADE))
    spl = SPELL_WIND_BLADE;

  if(!spl && npc_has_spell_slot(ch, SPELL_ARIEKS_SHATTERING_ICEBALL))
    spl = SPELL_ARIEKS_SHATTERING_ICEBALL;

  if(!spl && npc_has_spell_slot(ch, SPELL_COMET))
          spl = SPELL_COMET;

  if(!spl && npc_has_spell_slot(ch, SPELL_FORKED_LIGHTNING) &&
      spell_can_affect_char(target, SPELL_FORKED_LIGHTNING))
    spl = SPELL_FORKED_LIGHTNING;

  if(!spl && npc_has_spell_slot(ch, SPELL_CYCLONE) &&
      spell_can_affect_char(target, SPELL_CYCLONE))
    spl = SPELL_CYCLONE;

  if(!spl && npc_has_spell_slot(ch, SPELL_ARCANE_WHIRLWIND) &&
      spell_can_affect_char(target, SPELL_ARCANE_WHIRLWIND))
    spl = SPELL_ARCANE_WHIRLWIND;

  if(!spl && npc_has_spell_slot(ch, SPELL_INDUCE_TUPOR) &&
      spell_can_affect_char(target, SPELL_INDUCE_TUPOR))
    spl = SPELL_INDUCE_TUPOR;

  if(!spl && npc_has_spell_slot(ch, SPELL_FROSTBITE) &&
      spell_can_affect_char(target, SPELL_FROSTBITE))
    spl = SPELL_FROSTBITE;

  if(!spl && npc_has_spell_slot(ch, SPELL_FROST_BOLT) &&
      spell_can_affect_char(target, SPELL_FROST_BOLT))
    spl = SPELL_FROST_BOLT;

  /* heal up, the offense that's left sucks anyway */

  /* don't heal up all the time though..  the mob may still be doing
     good barehanded damage */

  if(!number(0, 3))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_GREATER_ETHEREAL) && affected_by_spell(ch, SPELL_ETHEREAL_FORM) && (dam > 75))
        spl = SPELL_GREATER_ETHEREAL;

    if(!spl && npc_has_spell_slot(ch, SPELL_ETHEREAL_RECHARGE) && affected_by_spell(ch, SPELL_ETHEREAL_FORM) && (dam > 40))
        spl = SPELL_ETHEREAL_RECHARGE;

  }
  if(!spl && npc_has_spell_slot(ch, SPELL_ICE_MISSILE) &&
      spell_can_affect_char(target, SPELL_ICE_MISSILE))
    spl = SPELL_ICE_MISSILE;

  if(spl && ch && target)
  {
    if(IS_AGG_SPELL(spl)) {
      P_char nuke_target = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);
      return (MobCastSpell(ch, nuke_target ? nuke_target : target, 0, spl, lvl));
    }
    else
      return (MobCastSpell(ch, target, 0, spl, lvl));
  }

  if(!spl)
    target = ch;


  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));


  return (FALSE);
}

bool CastClericSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL, tempch;
  int      dam = 0, lvl = 0, spl = 0;

  /* make sure I'm even able to cast in this room! */

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }

  lvl = GET_LEVEL(ch);

  if(helping)
  {
    target = victim;
    dam = GET_MAX_HIT(target) - GET_HIT(target);
  }
  else
  {
    target = ch;
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
  }

  /*
   * First priority: if I'm hurt bad, and I'm fighting, and I'm not at
   * my birthplace, get home!
   */

  if(!spl && IS_FIGHTING(ch) && (GET_HIT(ch) < (GET_MAX_HIT(ch) >> 2)) &&
      (world[ch->in_room].number != GET_BIRTHPLACE(ch)) &&
      npc_has_spell_slot(ch, SPELL_WORD_OF_RECALL) &&
      !IS_SET(world[ch->in_room].room_flags, NO_RECALL) &&
      !IS_SET(world[real_room0(GET_BIRTHPLACE(ch))].room_flags, NO_RECALL))
    spl = SPELL_WORD_OF_RECALL;

  if(!spl && (ch == target) && !IS_AFFECTED2(ch, AFF2_SOULSHIELD) &&
      !affected_by_spell(ch, SPELL_SOULSHIELD) && !IS_AFFECTED4(ch, AFF4_NEG_SHIELD) &&
      ((GET_ALIGNMENT(ch) <= -950) ||
      (GET_ALIGNMENT(ch) >= 950)) &&
      npc_has_spell_slot(ch, SPELL_SOULSHIELD) &&
      (!IS_FIGHTING(ch) || number(0, 2)))
  spl = SPELL_SOULSHIELD;

  if(!spl &&
    (affected_by_spell(target, SPELL_POISON) ||
    IS_AFFECTED2(target, AFF2_POISONED)) &&
    (!IS_FIGHTING(ch) || !number(0, 3)))
  {
    if(npc_has_spell_slot(ch, SPELL_PURIFY_SPIRIT))
    {
      spl = SPELL_PURIFY_SPIRIT;
    }
    else if(npc_has_spell_slot(ch, SPELL_REMOVE_POISON))
    {
      spl = SPELL_REMOVE_POISON;
    }
    else if(npc_has_spell_slot(ch, SPELL_SLOW_POISON))
    {
      spl = SPELL_SLOW_POISON;
    }
  }

  if(!spl && affected_by_spell(target, SPELL_CURSE) &&
      npc_has_spell_slot(ch, SPELL_REMOVE_CURSE) &&
      (!IS_FIGHTING(ch) || !number(0, 5)))
    spl = SPELL_REMOVE_CURSE;

  if(!spl && IS_AFFECTED(target, AFF_BLIND) &&
      npc_has_spell_slot(ch, SPELL_CURE_BLIND) &&
      (!IS_FIGHTING(ch) || !number(0, 2)))
    spl = SPELL_CURE_BLIND;
  else if(!spl && IS_AFFECTED(target, AFF_BLIND) &&
           npc_has_spell_slot(ch, SPELL_HEAL) &&
           (!IS_FIGHTING(ch) || !number(0, 2)))
    spl = SPELL_HEAL;
  else if(!spl && IS_AFFECTED(ch, AFF_BLIND) &&
           room_has_valid_exit(ch->in_room) && !fear_check(ch) )
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  /* only bother healing if we're not fighting */
  /* eh, let's also give em a slight chance to heal now and then */

  /* when not fighting, don't always cast heal spells..  other spells may need refreshing */

  if(!spl &&
    ((!IS_FIGHTING(ch) && (number(0, 9) != 8)) ||
    (IS_FIGHTING(ch) && (number(0, 9) == 2))))
  {
    if(npc_has_spell_slot(ch, SPELL_FULL_HEAL) && (dam > 200))
      spl = SPELL_FULL_HEAL;
    else if(npc_has_spell_slot(ch, SPELL_HEALING_SALVE) && (dam > 120))
      spl = SPELL_HEALING_SALVE;
    else if(npc_has_spell_slot(ch, SPELL_HEAL) && (dam > 60))
      spl = SPELL_HEAL;
    else if(npc_has_spell_slot(ch, SPELL_MASS_HEAL) && (dam > 90))
    {
      int      cast_it = TRUE;

      for (tempch = world[ch->in_room].people; tempch;
           tempch = tempch->next_in_room)
      {
        if(IS_PC(tempch) || (tempch->following && IS_PC(tempch->following)))
        {
          cast_it = FALSE;
          break;
        }
      }

      if(cast_it)
        spl = SPELL_MASS_HEAL;
    }

    if(!spl && npc_has_spell_slot(ch, SPELL_VITALITY) &&
        !affected_by_spell(target, SPELL_VITALITY) && (number(0, 1) ||
                                                       !IS_FIGHTING(ch)))
      spl = SPELL_VITALITY;

    // if not fighting, might as well cast the shit heal spells

    else if(!spl && !IS_FIGHTING(ch) && (dam > 0))
    {
      if(npc_has_spell_slot(ch, SPELL_CURE_CRITIC))
        spl = SPELL_CURE_CRITIC;
      else if(npc_has_spell_slot(ch, SPELL_CURE_SERIOUS))
        spl = SPELL_CURE_SERIOUS;
      else if(npc_has_spell_slot(ch, SPELL_CURE_LIGHT))
        spl = SPELL_CURE_LIGHT;
    }
  }

  if(!spl && (!IS_FIGHTING(ch) || (number(0, 4) == 2)))
  {
    if( !IS_AFFECTED(target, AFF_ARMOR) && npc_has_spell_slot(ch, SPELL_ARMOR)
      && GET_RACE(target) != RACE_ANIMAL )
      spl = SPELL_ARMOR;
    else if( !affected_by_spell(target, SPELL_BLESS) && npc_has_spell_slot(ch, SPELL_BLESS)
      && GET_RACE(target) != RACE_ANIMAL)
      spl = SPELL_BLESS;
  }

  /* this stuff is all of limited usefulness in combat (the prot spells are nice,
     but it's impossible to know which ones to cast), so only cast rarely while
     fighting */

  if(!IS_FIGHTING(ch) || (number(0, 10) == 3))
  {
    if(!spl && !affected_by_spell(ch, SPELL_TRUE_SEEING)
        && npc_has_spell_slot(ch, SPELL_TRUE_SEEING) && (ch == target))
      spl = SPELL_TRUE_SEEING;

    if(!spl && !IS_AFFECTED4(ch, AFF4_SANCTUARY) &&
        npc_has_spell_slot(ch, SPELL_LESSER_SANCTUARY) && (ch == target))
      spl = SPELL_LESSER_SANCTUARY;

    if(GET_RACE(target) != RACE_ANIMAL)
    {
      if(!spl &&
         !IS_AFFECTED(target, AFF_PROT_FIRE) &&
         npc_has_spell_slot(ch, SPELL_PROTECT_FROM_FIRE))
      {
        spl = SPELL_PROTECT_FROM_FIRE;
      }

      if(!spl &&
         !IS_AFFECTED2(target, AFF2_PROT_COLD) &&
         npc_has_spell_slot(ch, SPELL_PROTECT_FROM_COLD))
      {
        spl = SPELL_PROTECT_FROM_COLD;
      }

      if(!spl &&
         npc_has_spell_slot(ch, SPELL_PROTECT_FROM_EVIL))
      {
        if(IS_GOOD(target) &&
          !IS_AFFECTED(target, AFF_PROTECT_EVIL))
        {
          spl = SPELL_PROTECT_FROM_EVIL;
        }
        else if(IS_EVIL(target) &&
          !IS_AFFECTED(target, AFF_PROTECT_GOOD))
        {
          spl = SPELL_PROTECT_FROM_GOOD;
        }
      }

      if(!spl &&
         !IS_AFFECTED2(target, AFF2_PROT_LIGHTNING) &&
         npc_has_spell_slot(ch, SPELL_PROTECT_FROM_LIGHTNING))
      {
        spl = SPELL_PROTECT_FROM_LIGHTNING;
      }
      
      if(!spl &&
        !IS_AFFECTED2(target, AFF2_PROT_ACID) &&
        npc_has_spell_slot(ch, SPELL_PROTECT_FROM_ACID))
      {
        spl = SPELL_PROTECT_FROM_ACID;
      }
      
      if(!spl &&
         !IS_AFFECTED2(target, AFF2_PROT_GAS) &&
         npc_has_spell_slot(ch, SPELL_PROTECT_FROM_GAS))
      {
        spl = SPELL_PROTECT_FROM_GAS;
      }
    }
  }
  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    P_char   tch;
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastClericSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in random number crap\r\n", ch);
    }
  }
  if((victim == ch) || helping)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);

  /* holy/unholy word are good enough against one person, don't need a
     group */

  if(!spl &&
     number(0,1) &&
     !affected_by_spell(target, SPELL_PLAGUE) &&
     npc_has_spell_slot(ch, SPELL_PLAGUE))
  {
    spl = SPELL_PLAGUE;
  }

  if(!spl &&
     npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) &&
     number(0, 9) == 9)
  {
    spl = SPELL_DISPEL_MAGIC;
  }
  
  if(!spl &&
    (IS_EVIL(ch) || IS_GOOD(ch)) &&
    npc_has_spell_slot(ch, SPELL_HOLY_WORD) &&
    !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    if(IS_EVIL(ch) &&
       room_has_good_enemy(target))
    {
      spl = SPELL_UNHOLY_WORD;
    }
    else if(IS_GOOD(ch) &&
      room_has_evil_enemy(target))
    {
      spl = SPELL_HOLY_WORD;
    }
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_BANISH) && number(0,1))
  {
    P_char tch;
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if(IS_PC_PET(tch) && can_banish(ch, tch))
      {
        spl = SPELL_BANISH;
        break;
      }
  }

  if(!spl &&
      ((NumAttackers(ch) > 1) || has_help(target) ||
       no_chars_in_room_deserve_helping(ch)) &&
      !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    if(npc_has_spell_slot(ch, SPELL_EARTHQUAKE) && (number(0, 3) == 1))
      spl = SPELL_EARTHQUAKE;
  }

  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl &&
    npc_has_spell_slot(ch, SPELL_SUNRAY))
  {
    spl = SPELL_SUNRAY;
  }
  
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_DESTROY_UNDEAD) &&
     (GET_HIT(target) > 5) &&
     IS_UNDEADRACE(target))
  {
    spl = SPELL_DESTROY_UNDEAD;
  }
  
  /* randomly cast some semi-nasty stuff */

  if(!spl && !number(0, 4))
  {
    switch (number(0, 3))
    {
    case 0:
      if(npc_has_spell_slot(ch, SPELL_BLINDNESS) &&
          !IS_AFFECTED(target, AFF_BLIND))
        spl = SPELL_BLINDNESS;
      break;

    case 1:
      if(!spl && npc_has_spell_slot(ch, SPELL_SILENCE) &&
          !IS_AFFECTED2(target, AFF2_SILENCED))
        spl = SPELL_SILENCE;
      break;

    case 2:
      if(!spl && npc_has_spell_slot(ch, SPELL_CURSE) &&
          !affected_by_spell(target, SPELL_CURSE))
        spl = SPELL_CURSE;
      break;

    case 3:
      if(!spl && npc_has_spell_slot(ch, SPELL_FEAR))
        spl = SPELL_FEAR;
    }
  }
  if(!spl && npc_has_spell_slot(ch, SPELL_FULL_HARM) &&
      (GET_HIT(target) > 10))
    spl = SPELL_FULL_HARM;

  if(!spl && OUTSIDE(ch) && !IS_GLOBED(target) && npc_has_spell_slot(ch, SPELL_CALL_LIGHTNING))
  {
    spl = SPELL_CALL_LIGHTNING;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_FLAMESTRIKE) && !IS_GLOBED(target))
    spl = SPELL_FLAMESTRIKE;

  if(!spl && npc_has_spell_slot(ch, SPELL_DISPEL_EVIL) && !IS_EVIL(ch) && IS_EVIL(target)
     && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
  {
    spl = SPELL_DISPEL_EVIL;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_DISPEL_GOOD) && !IS_GOOD(ch) && IS_GOOD(target)
     && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
  {
    spl = SPELL_DISPEL_GOOD;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_HARM) && !IS_GLOBED(target))
    spl = SPELL_HARM;

  /* cause crit etc are only any good if barehand damage sucks */

  if(number(0, 6) == 3)
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_CAUSE_CRITICAL) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
      spl = SPELL_CAUSE_CRITICAL;

    if(!spl && npc_has_spell_slot(ch, SPELL_CAUSE_SERIOUS) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
      spl = SPELL_CAUSE_SERIOUS;
  }
  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  /*
   * well, time to scrape the bottom of the barrel
   */

  if(!spl)
    target = ch;

  if(TRUE)
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_FULL_HEAL) && (dam > 200) &&
        (!IS_FIGHTING(ch) || (number(0, 2) == 1)))
      spl = SPELL_FULL_HEAL;

    if(!spl && npc_has_spell_slot(ch, SPELL_HEALING_SALVE) && (dam > 125) &&
        (!IS_FIGHTING(ch) || (number(0, 2) == 1)))
      spl = SPELL_HEALING_SALVE;

    if(!spl && npc_has_spell_slot(ch, SPELL_HEAL) && (dam > 60) &&
        (!IS_FIGHTING(ch) || (number(0, 5) == 1)))
      spl = SPELL_HEAL;

    if(!spl && npc_has_spell_slot(ch, SPELL_MASS_HEAL) && (dam > 90) &&
        (!IS_FIGHTING(ch) || (number(0, 4) == 1)))
    {
      int      cast_it = TRUE;

      for (tempch = world[ch->in_room].people; tempch;
           tempch = tempch->next_in_room)
      {
        if(IS_PC(tempch) || (tempch->following && IS_PC(tempch->following)))
        {
          cast_it = FALSE;
          break;
        }
      }

      if(cast_it)
        spl = SPELL_MASS_HEAL;
    }
    /* these heal spells are pretty shitty..  don't bother with em unless the
       attacker is VERY low level */

    if(!IS_FIGHTING(ch) || (number(0, 6) == 1))
    {
      if(!spl && npc_has_spell_slot(ch, SPELL_CURE_CRITIC) && (dam > 25))
        spl = SPELL_CURE_CRITIC;

      if(!spl && npc_has_spell_slot(ch, SPELL_CURE_SERIOUS) && (dam > 12))
        spl = SPELL_CURE_SERIOUS;

      if(!spl && npc_has_spell_slot(ch, SPELL_CURE_LIGHT) && (dam > 8))
        spl = SPELL_CURE_LIGHT;
    }
  }
  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  /* nothing left, just slap the attacking player around with hand-to-hand
     damage */

  return FALSE;

/*
   if(!spl && (ch == victim))
     return (FALSE);

   if(!victim)
     target = ch->specials.fighting;
   else
     target = victim;

   if(!spl && npc_has_spell_slot(ch, SPELL_CAUSE_LIGHT) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
     spl = SPELL_CAUSE_LIGHT;

   if(spl && ch && target)
     return (MobCastSpell(ch, target, 0, spl, lvl));

   return (FALSE);
 */
}

bool CastPaladinSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL, tempch, tch;
  int      dam = 0, lvl = 0, spl = 0;

  lvl = GET_LEVEL(ch);

  if(helping)
  {
    target = victim;
    dam = GET_MAX_HIT(target) - GET_HIT(target);
  }
  else
  {
    target = ch;
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
  }

  /* make sure I'm even able to cast in this room! */

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }

  if(!spl && (affected_by_spell(target, SPELL_POISON) ||
               IS_AFFECTED2(target, AFF2_POISONED)) &&
      (!IS_FIGHTING(ch) || !number(0, 3)))
  {
    if(npc_has_spell_slot(ch, SPELL_REMOVE_POISON))
      spl = SPELL_REMOVE_POISON;
    else if(npc_has_spell_slot(ch, SPELL_SLOW_POISON))
      spl = SPELL_SLOW_POISON;
  }

  // sanctuary good, cast that early

  if(!spl && !IS_AFFECTED4(ch, AFF4_SANCTUARY) && (target == ch) &&
      npc_has_spell_slot(ch, SPELL_SANCTUARY) && (!IS_FIGHTING(ch) ||
                                                  number(0, 1)))
    spl = SPELL_SANCTUARY;

  if(!spl && IS_AFFECTED(target, AFF_BLIND) &&
      (npc_has_spell_slot(ch, SPELL_CURE_BLIND) ||
       npc_has_spell_slot(ch, SPELL_HEAL)) && (!IS_FIGHTING(ch) ||
                                               !number(0, 3)))
  {
    // prefer cure blind since lower circle..

    if(npc_has_spell_slot(ch, SPELL_CURE_BLIND))
      spl = SPELL_CURE_BLIND;
    else
      spl = SPELL_HEAL;
  }
  else if(!spl && IS_AFFECTED(ch, AFF_BLIND) && IS_FIGHTING(ch) &&
           room_has_valid_exit(ch->in_room) && !fear_check(ch) )
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  if(!spl && affected_by_spell(target, SPELL_CURSE) &&
      npc_has_spell_slot(ch, SPELL_REMOVE_CURSE) &&
      (!IS_FIGHTING(ch) || !number(0, 5)))
    spl = SPELL_REMOVE_CURSE;

    if(!spl && (ch == target) && !IS_AFFECTED2(ch, AFF2_SOULSHIELD) &&
      !affected_by_spell(ch, SPELL_SOULSHIELD) && !IS_AFFECTED4(ch, AFF4_NEG_SHIELD) &&
      ((GET_ALIGNMENT(ch) <= -950) ||
      (GET_ALIGNMENT(ch) >= 950)) &&
      npc_has_spell_slot(ch, SPELL_SOULSHIELD) &&
      (!IS_FIGHTING(ch) || number(0, 2)))
  spl = SPELL_SOULSHIELD;

  if(!spl && !affected_by_spell(target, SPELL_ACCEL_HEALING) &&
      (!IS_FIGHTING(ch) || number(0, 10) == 10))
    spl = SPELL_ACCEL_HEALING;

  if(!spl && !IS_AFFECTED4(ch, AFF4_HOLY_SACRIFICE) && (target == ch) &&
      npc_has_spell_slot(ch, SPELL_HOLY_SACRIFICE) &&
      (!IS_FIGHTING(ch) || number(0, 10) == 10))
    spl = SPELL_HOLY_SACRIFICE;
    
  if(!spl &&
     !affected_by_spell(ch, SPELL_HOLY_DHARMA) &&
     affected_by_spell(ch, SPELL_SOULSHIELD))
  {
    spl = SPELL_HOLY_DHARMA;
  }

  /* only bother healing if we're not fighting */
  /* eh, let's also give em a slight chance to heal now and then */

  /* while not fighting, don't always heal up */

  if((!IS_FIGHTING(ch) && (number(0, 8) != 3)) ||
      (IS_FIGHTING(ch) && (number(0, 19) == 19)))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_HEAL) && (dam > 60))
      spl = SPELL_HEAL;

    if(!spl && npc_has_spell_slot(ch, SPELL_MASS_HEAL) && (dam > 90))
    {
      int      cast_it = TRUE;

      for (tempch = world[ch->in_room].people; tempch;
           tempch = tempch->next_in_room)
      {
        if(IS_PC(tempch) || (tempch->following && IS_PC(tempch->following)))
        {
          cast_it = FALSE;
          break;
        }
      }

      if(cast_it)
        spl = SPELL_MASS_HEAL;
    }
  }

  // ch not fighting, break out weak-ass heal spells

  if(!spl && !IS_FIGHTING(ch) && (dam > 0) && (number(0, 4) != 3))
  {
    if(npc_has_spell_slot(ch, SPELL_CURE_CRITIC))
      spl = SPELL_CURE_CRITIC;
    else if(npc_has_spell_slot(ch, SPELL_CURE_SERIOUS))
      spl = SPELL_CURE_SERIOUS;
    else if(npc_has_spell_slot(ch, SPELL_CURE_LIGHT))
      spl = SPELL_CURE_LIGHT;
  }

  if(!IS_FIGHTING(ch) || (number(0, 4) == 2))
  {
    if(!spl && !IS_AFFECTED(target, AFF_ARMOR) && npc_has_spell_slot(ch, SPELL_ARMOR)
      && GET_RACE(target) != RACE_ANIMAL)
    {
      spl = SPELL_ARMOR;
    }

    if(!spl &&
       !affected_by_spell(target, SPELL_BLESS) &&
       npc_has_spell_slot(ch, SPELL_BLESS) &&
       GET_RACE(target) != RACE_ANIMAL)
    {
      spl = SPELL_BLESS;
    }
  }

  /* this stuff is all of limited usefulness in combat (the prot spells are nice,
     but it's impossible to know which ones to cast), so only cast rarely while
     fighting */

  if(!IS_FIGHTING(ch) || (number(0, 4) == 3))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_PROTECT_FROM_EVIL) &&
        (!IS_FIGHTING(ch) || (number(0, 2) == 1)) && IS_GOOD(target) &&
        !IS_AFFECTED(target, AFF_PROTECT_EVIL))
      spl = SPELL_PROTECT_FROM_EVIL;
  }
  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastPaladinSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in random number crap\r\n", ch);
    }
  }
  if((victim == ch) || helping)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);

  if(!spl && (GET_ALIGNMENT(ch) >= 980) && room_has_evil_enemy(ch) &&
      npc_has_spell_slot(ch, SPELL_JUDGEMENT) && number(0, 2))
  {
    spl = SPELL_JUDGEMENT;
  }

  /* holy/unholy word are good enough against one person, don't need a
     group */

  /* no they aren't - Jexni 11/25/08 */

  if(!spl && IS_GOOD(ch) && npc_has_spell_slot(ch, SPELL_HOLY_WORD) &&
      !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) &&
      room_has_evil_enemy(ch) && number(0, 2))
  {
    int evnum = 0;
    for(tch = world[ch->in_room].people;tch;tch = tch->next_in_room)
    {
      if(IS_EVIL(tch))
      evnum++;
    }
    if(evnum > 2)
      spl = SPELL_HOLY_WORD;
  }

  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && npc_has_spell_slot(ch, SPELL_DESTROY_UNDEAD) && 
     (IS_UNDEAD(target) || IS_AFFECTED(target, AFF_WRAITHFORM)))
  {
    spl = SPELL_DESTROY_UNDEAD;
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_TURN_UNDEAD) && IS_UNDEAD(target) && number(0, 1))
    spl = SPELL_TURN_UNDEAD;

  if(lvl < 26 && !spl && npc_has_spell_slot(ch, SPELL_DISPEL_EVIL) && !IS_EVIL(ch) && IS_EVIL(target)
     && (!IS_GLOBED(target) || !IS_MINGLOBED(target)) && !number(0, 2))
  {
    spl = SPELL_DISPEL_EVIL; // God I hope no mob ever has to use this spell - Jexni 11/25/08
  }

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  /*
   * well, time to scrape the bottom of the barrel
   */

  if(!spl)
    target = ch;

  if(!number(0, 3))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_HEAL) && (dam > 60))
      spl = SPELL_HEAL;

    if(!spl && npc_has_spell_slot(ch, SPELL_MASS_HEAL) && (dam > 90))
    {
      int      cast_it = TRUE;

      for (tempch = world[ch->in_room].people; tempch;
           tempch = tempch->next_in_room)
      {
        if(IS_PC(tempch) || (tempch->following && IS_PC(tempch->following)))
        {
          cast_it = FALSE;
          break;
        }
      }

      if(cast_it)
        spl = SPELL_MASS_HEAL;
    }
    /* these heal spells are pretty shitty..  don't bother with em except once in a blue moon */

    if(!number(0, 8))
    {
      if(!spl && npc_has_spell_slot(ch, SPELL_CURE_CRITIC) && (dam > 25))
        spl = SPELL_CURE_CRITIC;

      if(!spl && npc_has_spell_slot(ch, SPELL_CURE_SERIOUS) && (dam > 12))
        spl = SPELL_CURE_SERIOUS;

      if(!spl && npc_has_spell_slot(ch, SPELL_CURE_LIGHT) && (dam > 8))
        spl = SPELL_CURE_LIGHT;
    }
  }
  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  /* nothing left, just slap the attacking player around with hand-to-hand
     damage */

  return FALSE;
}

bool CastAntiPaladinSpell(P_char ch, P_char victim, int helping)
{
  P_char   target = NULL, tempch;
  int      dam = 0, lvl = 0, spl = 0;

  lvl = GET_LEVEL(ch);

  if(helping)
  {
    target = victim;
    dam = GET_MAX_HIT(target) - GET_HIT(target);
  }
  else
  {
    target = ch;
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
  }

  /* make sure I'm even able to cast in this room! */

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }
  /* alas, anti-pals have no way to cure blindness */

  if(!spl &&
     IS_AFFECTED(ch, AFF_BLIND) &&
     IS_FIGHTING(ch) &&
     room_has_valid_exit(ch->in_room) &&
     !fear_check(ch) &&
     !number(0, 2) &&
     !IS_SHOPKEEPER(ch))
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  if(!spl && (ch == target) && !IS_AFFECTED2(ch, AFF2_SOULSHIELD) &&
    !affected_by_spell(ch, SPELL_SOULSHIELD) && !IS_AFFECTED4(ch, AFF4_NEG_SHIELD) &&
    ((GET_ALIGNMENT(ch) <= -950) ||
    (GET_ALIGNMENT(ch) >= 950)) &&
    npc_has_spell_slot(ch, SPELL_SOULSHIELD) &&
    (!IS_FIGHTING(ch) || number(0, 2)))
  {
    spl = SPELL_SOULSHIELD;
  }
  
  // hellfire good, (almost) always cast that

  if(!spl && !affected_by_spell(ch, SPELL_HELLFIRE) && (ch == target) &&
      !IS_AFFECTED4(ch, AFF4_HELLFIRE) &&
      npc_has_spell_slot(ch, SPELL_HELLFIRE) && (!IS_FIGHTING(ch) ||
                                                 number(0, 2)))
    spl = SPELL_HELLFIRE;

  if(!spl && !affected_by_spell(ch, SPELL_SPAWN) && (ch == target) &&
      npc_has_spell_slot(ch, SPELL_SPAWN) && (!IS_FIGHTING(ch) ||
                                                 number(0, 2)))
    spl = SPELL_SPAWN;

  if(!spl && !affected_by_spell(ch, SPELL_DREAD_BLADE) && (ch == target) &&
      npc_has_spell_slot(ch, SPELL_DREAD_BLADE) && (!IS_FIGHTING(ch) ||
                                                 number(0, 2)))
    spl = SPELL_DREAD_BLADE;

  if(!IS_FIGHTING(ch) ||
     (number(0, 4) == 2) ||
     ((ch->following || ch->followers) &&
     number(0, 1)))
  {
    if(!spl &&
       !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) &&
       npc_has_spell_slot(ch, SPELL_BATTLE_ECSTASY) &&
       (target == ch))
    {
      spl = SPELL_BATTLE_ECSTASY;
    }
  }
  
  if(!IS_FIGHTING(ch) || (number(0, 4) == 2))
  {
    if( !spl && !IS_AFFECTED(target, AFF_ARMOR) && npc_has_spell_slot(ch, SPELL_ARMOR)
      && GET_RACE(target) != RACE_ANIMAL )
    {
      spl = SPELL_ARMOR;
    }
  }
  /* this stuff is all of limited usefulness in combat (the prot spells are nice,
     but it's impossible to know which ones to cast), so only cast rarely while
     fighting */

  if(!IS_FIGHTING(ch) || (number(0, 10) == 3))
  {
    if(IS_EVIL(target) &&
      !IS_AFFECTED(target, AFF_PROTECT_GOOD) &&
      npc_has_spell_slot(ch, SPELL_PROTECT_FROM_GOOD))
    {
      spl = SPELL_PROTECT_FROM_GOOD;
    }
  }
  
  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  if(!spl && !helping && !IS_FIGHTING(ch))
  {
    P_char   tch;
    int      numb = 0, lucky, curr = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        numb++;

    if(numb)
    {
      if(numb == 1)
        lucky = 1;
      else
        lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if((ch != tch) && char_deserves_helping(ch, tch, TRUE))
        {
          curr++;
          if(curr == lucky)
            return (CastAntiPaladinSpell(ch, tch, TRUE));
        }
      }

      send_to_char("error in random number crap\r\n", ch);
    }
  }
  if((victim == ch) || helping)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);

  /* apocalypse is a nice spell, it targets everyone..  no reason not to cast it
     if we got it */

  if(!spl &&
     npc_has_spell_slot(ch, SPELL_APOCALYPSE) &&
     number(0, 2) &&
     target != ch &&
     !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    spl = SPELL_APOCALYPSE;
  }
  /* holy/unholy word are good enough against one person, don't need a
     group */

  if(!spl &&
     IS_EVIL(ch) &&
     npc_has_spell_slot(ch, SPELL_UNHOLY_WORD) &&
     room_has_good_enemy(ch) &&
     target != ch &&
     !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) &&
     number(0, 2))
  {
    spl = SPELL_UNHOLY_WORD;
  }

  if(!spl &&
     npc_has_spell_slot(ch, SPELL_VORTEX_OF_FEAR) &&
     !number(0, 4) &&
     !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    spl = SPELL_VORTEX_OF_FEAR;
  }
  
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_WITHER) &&
     !affected_by_spell(target, SPELL_WITHER) &&
     !number(0, 2) &&
     target != ch &&
     GET_C_POW(ch) > GET_C_POW(target))
  {
    spl = SPELL_WITHER;
  }
  
  if(!spl &&
     npc_has_spell_slot(ch, SPELL_ENERGY_DRAIN) &&
     !IS_UNDEAD(target) &&
     !IS_PUNDEAD(target) &&
     target != ch &&
     number(0, 4))
  {
    spl = SPELL_ENERGY_DRAIN;
  }
  
  if(spl && ch)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  /* randomly cast some semi-nasty stuff */

  if(!spl && !number(0, 4))
  {
    switch (number(0, 2))
    {
    case 0:
      if(npc_has_spell_slot(ch, SPELL_BLINDNESS) &&
          !IS_AFFECTED(target, AFF_BLIND))
        spl = SPELL_BLINDNESS;

    case 1:
      if(!spl && npc_has_spell_slot(ch, SPELL_CURSE) &&
          !affected_by_spell(target, SPELL_CURSE))
        spl = SPELL_CURSE;

    case 2:
      if(!spl &&
         npc_has_spell_slot(ch, SPELL_FEAR) &&
         !fear_check(target))
      {
        spl = SPELL_FEAR;
      }
    }
  }
  
  if(lvl < 26 &&
     !spl &&
     npc_has_spell_slot(ch, SPELL_DISPEL_GOOD) &&
     !IS_GOOD(ch) &&
     IS_GOOD(target) &&
     (!IS_GLOBED(target) || !IS_MINGLOBED(target)) &&
     !number(0, 2))
  {
    spl = SPELL_DISPEL_GOOD; // this spell sucks balls, only cast it if you also suck balls - Jexni 11/25/08
  }

  if(!spl && npc_has_spell_slot(ch, SPELL_HARM) && !IS_GLOBED(target) && !number(0, 2) && IS_UNDEAD(target))
  {
    spl = SPELL_HARM; //  Don't use crap spells as a pal/anti unless you can do _some_ damage - Jexni 11/25/08
  }

  /* cause crit etc are only any good if barehand damage sucks */
  
  if((lvl < 26) && !number(0, 2) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)))
  {
    if(!spl && npc_has_spell_slot(ch, SPELL_CAUSE_CRITICAL))
      spl = SPELL_CAUSE_CRITICAL;

    if(!spl && npc_has_spell_slot(ch, SPELL_CAUSE_SERIOUS))
      spl = SPELL_CAUSE_SERIOUS;
  } // How about we put redundant checks at the first if there, eh morons?  - Jexni 11/25/08

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));

  /*
   * well, time to scrape the bottom of the barrel
   */

/*  if(!spl)
    target = ch;

  if(spl && ch && target)
    return (MobCastSpell(ch, target, 0, spl, lvl));*/

  /* nothing left, just slap the attacking player around with hand-to-hand
     damage */

  return FALSE;
}

bool WillPsionicistSpell(P_char ch, P_char victim)
{
  P_char   target = NULL;
  int      dam = 0, lvl = 0, spl = 0;

  if(IS_PC_PET(ch) &&
     GET_MASTER(ch)->in_room != ch->in_room)
      return false;
    
  lvl = GET_LEVEL(ch);

  if(GET_HIT(ch) < (int)(GET_MAX_HIT(ch) / 10) &&
     knows_spell(ch, SPELL_DEPART))
    if(!IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) &&
        !IS_HOMETOWN(ch->in_room) &&
        world[ch->in_room].sector_type != SECT_OCEAN &&
        IS_FIGHTING(ch))
          spl = SPELL_DEPART;
  
  if(!number(0, 4))
    if(!spl &&
       npc_has_spell_slot(ch, SPELL_ECTOPLASMIC_FORM) &&
       !affected_by_spell(ch, SPELL_ECTOPLASMIC_FORM) &&
       !IS_AFFECTED3(ch, AFF3_ECTOPLASMIC_FORM) &&
       !IS_FIGHTING(ch) &&
       !IS_AFFECTED(ch, AFF_INVISIBLE) &&
       !IS_AFFECTED2(ch, AFF2_MINOR_INVIS) &&
       !IS_ACT(ch, ACT_TEACHER) && 
       !mob_index[GET_RNUM(ch)].qst_func &&
       !CHAR_IN_TOWN(ch))
          spl = SPELL_ECTOPLASMIC_FORM;

  /* cannibalize is very important, especially in battle */

  // if(!spl &&
     // !affected_by_spell(ch, SPELL_CANNIBALIZE) &&
     // !IS_AFFECTED3(ch, AFF3_CANNIBALIZE) &&
     // knows_spell(ch, SPELL_CANNIBALIZE) &&
     // !IS_FIGHTING(ch))
        // spl = SPELL_CANNIBALIZE;

  if(!spl &&
     !IS_MELEE_CLASS(ch) &&
     !affected_by_spell(ch, SPELL_INERTIAL_BARRIER) &&
     !IS_AFFECTED3(ch, AFF3_INERTIAL_BARRIER) &&
     knows_spell(ch, SPELL_INERTIAL_BARRIER))
        spl = SPELL_INERTIAL_BARRIER;
  
  if(!spl &&
     !has_skin_spell(ch) &&
     (!IS_FIGHTING(ch) || (number(0, 1))))
        spl = pick_best_skin_spell(ch, ch);
  
  if(!spl &&
     !affected_by_spell(ch, SPELL_ENHANCE_ARMOR) &&
     knows_spell(ch, SPELL_ENHANCE_ARMOR) &&
     (!IS_FIGHTING(ch) || (number(0, 3) == 2)))
        spl = SPELL_ENHANCE_ARMOR;

  if(!spl &&
     !affected_by_spell(ch, SPELL_ENERGY_CONTAINMENT) &&
     knows_spell(ch, SPELL_ENERGY_CONTAINMENT))
        spl = SPELL_ENERGY_CONTAINMENT;

  if( !spl && !IS_AFFECTED(ch, AFF_ARMOR) && knows_spell(ch, SPELL_FLESH_ARMOR)
    && (!IS_FIGHTING(ch) || (number(0, 4) == 2)) )
        spl = SPELL_FLESH_ARMOR;

  if(!IS_FIGHTING(ch) || (number(0, 3) == 2))
  {
    if(!spl && !affected_by_spell(ch, SPELL_ADRENALINE_CONTROL) &&
        knows_spell(ch, SPELL_ADRENALINE_CONTROL))
      spl = SPELL_ADRENALINE_CONTROL;

    if(!spl &&
       !affected_by_spell(ch, SPELL_COMBAT_MIND) &&
       knows_spell(ch, SPELL_COMBAT_MIND))
          spl = SPELL_COMBAT_MIND;

    // this code should check level of psi since spell effects depend on level..

    if(!spl &&
      (affected_by_spell(ch, SPELL_POISON) ||
      (lvl > 2 && IS_AFFECTED2(ch, AFF2_POISONED)) ||
      (lvl > 35 && affected_by_spell(ch, SPELL_RAY_OF_ENFEEBLEMENT)) ||
      (lvl > 36 && affected_by_spell(ch, SPELL_MALISON)) ||
      (lvl > 41 && affected_by_spell(ch, SPELL_SLOW)) ||
      (lvl > 9 && affected_by_spell(ch, SPELL_CURSE)) ||
      (lvl > 50 && affected_by_spell(ch, SPELL_WITHER)) ||
      (lvl > 53 && affected_by_spell(ch, SPELL_DISEASE)) ||
      (lvl > 29 && IS_AFFECTED(ch, AFF_BLIND))) &&
      knows_spell(ch, SPELL_CELL_ADJUSTMENT))
        spl = SPELL_CELL_ADJUSTMENT;

    if(!spl && !affected_by_spell(ch, SPELL_POWERCAST_FLY) &&
        !IS_AFFECTED(ch, AFF_FLY) && knows_spell(ch, SPELL_POWERCAST_FLY))
      spl = SPELL_POWERCAST_FLY;

    if(!spl && !affected_by_spell(ch, SPELL_ENHANCED_CON) &&
        knows_spell(ch, SPELL_ENHANCED_CON))
      spl = SPELL_ENHANCED_CON;

    if(!spl && !affected_by_spell(ch, SPELL_ENHANCED_STR) &&
        knows_spell(ch, SPELL_ENHANCED_STR))
      spl = SPELL_ENHANCED_STR;

    if(!spl && !affected_by_spell(ch, SPELL_ENHANCED_AGI) &&
        knows_spell(ch, SPELL_ENHANCED_AGI))
      spl = SPELL_ENHANCED_AGI;

    if(!spl && !affected_by_spell(ch, SPELL_ENHANCED_DEX) &&
        knows_spell(ch, SPELL_ENHANCED_DEX))
      spl = SPELL_ENHANCED_DEX;

    if(!spl && !affected_by_spell(ch, SPELL_MOLECULAR_CONTROL) &&
        !IS_AFFECTED2(ch, AFF2_PASSDOOR) &&
        knows_spell(ch, SPELL_MOLECULAR_CONTROL))
      spl = SPELL_MOLECULAR_CONTROL;

    if(!spl && !affected_by_spell(ch, SPELL_INTELLECT_FORTRESS) &&
        knows_spell(ch, SPELL_INTELLECT_FORTRESS))
      spl = SPELL_INTELLECT_FORTRESS;

    if(!spl && !affected_by_spell(ch, SPELL_AURA_SIGHT) &&
        knows_spell(ch, SPELL_AURA_SIGHT))
      spl = SPELL_AURA_SIGHT;

    if(!spl && !affected_by_spell(ch, SPELL_TOWER_IRON_WILL) &&
        !IS_AFFECTED3(ch, AFF3_TOWER_IRON_WILL) &&
        knows_spell(ch, SPELL_TOWER_IRON_WILL))
      spl = SPELL_TOWER_IRON_WILL;
  }
  if(spl && ch)
    return (MobCastSpell(ch, ch, 0, spl, lvl));

  if(victim == ch)
    return (FALSE);

  if(!victim)
    target = ch->specials.fighting;
  else
    target = victim;

  if(!ch || !target)
    return (FALSE);
    
  if(GET_CLASS(ch, CLASS_MINDFLAYER) &&
     !number(0, 2) &&
     knows_spell(ch, SPELL_CONFUSE) &&
     GET_C_POW(ch) > GET_C_POW(target))
      spl = SPELL_CONFUSE;

  if(IS_BRAINLESS(target))
  {
    if(!spl && knows_spell(ch, SPELL_PYROKINESIS))
      spl = SPELL_PYROKINESIS;

    if(!spl && knows_spell(ch, SPELL_DETONATE))
      spl = SPELL_DETONATE;
    
    if(!spl && knows_spell(ch, SPELL_MOLECULAR_AGITATION))
      spl = SPELL_MOLECULAR_AGITATION;

    if(!spl && knows_spell(ch, SPELL_BALLISTIC_ATTACK))
      spl = SPELL_BALLISTIC_ATTACK;
  }
  else
  {
    if(!spl &&
       knows_spell(ch, SPELL_DEATH_FIELD) &&
       number(0, 1))
          spl = SPELL_DEATH_FIELD;

    if(!spl && knows_spell(ch, SPELL_PYROKINESIS))
      spl = SPELL_PYROKINESIS;

    if(!spl && knows_spell(ch, SPELL_PSYCHIC_CRUSH))
      spl = SPELL_PSYCHIC_CRUSH;

    if(!spl && knows_spell(ch, SPELL_DETONATE))
      spl = SPELL_DETONATE;

    if(!spl && knows_spell(ch, SPELL_INFLICT_PAIN))
      spl = SPELL_INFLICT_PAIN;

    if(!spl && knows_spell(ch, SPELL_MOLECULAR_AGITATION))
      spl = SPELL_MOLECULAR_AGITATION;

    if(!spl && knows_spell(ch, SPELL_BALLISTIC_ATTACK))
      spl = SPELL_BALLISTIC_ATTACK;

    if(!spl && knows_spell(ch, SPELL_EGO_WHIP))
      spl = SPELL_EGO_WHIP;
  }

  if(spl && ch && target)
  {
    P_char nuke_target = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);
    return (MobCastSpell(ch, nuke_target ? nuke_target : target, 0, spl, lvl));
  }

  return (FALSE);
}


void BreathWeapon(P_char ch, int dir)
{
  int      i = 0, room, orig_room = ch->in_room, distance;
  char     buf[MAX_STRING_LENGTH], waited = FALSE;
  P_char   tchar1 = NULL, tchar2 = NULL;
  void     (*funct) (int, P_char, char *, int, P_char, P_obj);
  P_char victim;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
        return;

  distance = BOUNDED(2, GET_LEVEL(ch) / 10, 6);

  if(isname("gold", GET_NAME(ch)))
    if(number(0, 1) == 0)
      i = 1;                    /* fire */
    else
      i = 5;                    /* gas */
  if(isname("brass", GET_NAME(ch)))    /* gas: sleep or fear    */
    if(number(0, 1) == 0)
      i = 6;                    /* sleep gas */
    else
      i = 7;                    /* fear gas */
  if(isname("bronze", GET_NAME(ch)))
    if(number(0, 1) == 0)
      i = 2;                    /* lightning */
    else
      i = 7;                    /* repulsion gas */
  if(isname("silver", GET_NAME(ch)))
    if(number(0, 1) == 0)
      i = 3;                    /* cold */
    else
      i = 8;                    /* para gas */
  if(isname("copper", GET_NAME(ch)))
    if(number(0, 1) == 0)
      i = 4;                    /* acid */
    else
      i = 5;                    /* gas */
  if(IS_ACT(ch, ACT_BREATHES_FIRE) || 
    isname("red", GET_NAME(ch)))     // || isname("br_f", GET_NAME(ch)))
      i = 1;                      /* fire */
  if(IS_ACT(ch, ACT_BREATHES_LIGHTNING) || 
    isname("blue", GET_NAME(ch)))       // || isname("br_l", GET_NAME(ch)))
      i = 2;                      /* lightning */
  if(IS_ACT(ch, ACT_BREATHES_FROST) ||
    isname("white", GET_NAME(ch)))  // || isname("br_c", GET_NAME(ch)))
      i = 3;                      /* cold */
  if(IS_ACT(ch, ACT_BREATHES_ACID) || 
    isname("black", GET_NAME(ch)))   // || isname("br_a", GET_NAME(ch)))
      i = 4;                      /* acid */
  if(IS_ACT(ch, ACT_BREATHES_GAS) || 
    isname("green", GET_NAME(ch)))    // || isname("br_g", GET_NAME(ch)))
      i = 5;                      /* gas */
  if(IS_ACT(ch, ACT_BREATHES_SHADOW) || 
    isname("shadow", GET_NAME(ch)))        // || isname("br_s", GET_NAME(ch)))
    if(number(1, 10) < 7)
      i = 9;
    else
      i = 10;
  if(IS_ACT(ch, ACT_BREATHES_BLIND_GAS))       // || isname("br_b", GET_NAME(ch)))
    i = 11;                     /* blinding gas */
  if(isname("crimson", GET_NAME(ch)))
      i = 12;                    /* crimson */
  if(isname("azure", GET_NAME(ch)))   
      i = 13;                    /* azure */
  if(isname("jasper", GET_NAME(ch)))
      i = 14;                    /* jasper */
  if(isname("basalt", GET_NAME(ch)))
      i = 15;                    /* basalt */
  if(isname("judgement", GET_NAME(ch)))
      i = 16;                    /* war */
  if(isname("justice", GET_NAME(ch)))
      i = 17;                    /* judgement */
  if(isname("war", GET_NAME(ch)))
      i = 18;                    /* vengeance */
  if(isname("vengeance", GET_NAME(ch)))
      i = 19;                    /* justice */
  if(i == 0)
    i = number(1, 5);

  switch (i)
  {
  case 1:
    act("$n breathes &+Rfire&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &+Rfire&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A blast of &+Rfire&n shoots in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_fire_breath;
    break;
  case 2:
    act("$n breathes &=LBlightning&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &=LBlightning&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A bolt of &=LBlightning&n crackles from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_lightning_breath;
    break;
  case 3:
    act("$n breathes &+Wfrost&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &+Wfrost&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A sudden &+Wfreezing gale&n blasts from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_frost_breath;
    break;
  case 4:
    act("$n breathes &+Lacid&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &+Lacid&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Lfrothing liquid&n streams in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_acid_breath;
    break;
  case 5:
    act("$n breathes &+gpoison gas&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &+gpoison gas&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A cloud of &+ggas&n billows in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_gas_breath;
    break;
  case 6:
    act("$n breathes &+wgas&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &+wgas&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A cloud of &+ggas&n billows in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_sleep;
    break;
  case 7:
    act("$n breathes &+rgas&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &+rgas&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A cloud of &+ggas&n billows in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_fear;
    break;
  case 8:
    act("$n breathes &+Mgas&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &+Mgas&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A cloud of &+ggas&n billows in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_minor_paralysis;
    break;
  case 9:
    act("&+LA billowing cloud of darkness erupts from $n&+L's mouth!&n", 1,
        ch, 0, 0, TO_ROOM);
    act("&+LA billowing cloud of darkness erupts from your mouth!&n", 0, ch,
        0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf,
              "A billowing &+Lcloud of darkness&n flows in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_shadow_breath_1;
    break;
  case 10:
    act("&+LA black beam shoots out of $n&+L's mouth!&n", 1, ch, 0, 0,
        TO_ROOM);
    act("&+LA black beam shoots out of your mouth!&n", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf,
              "A billowing &+Lcloud of darkness&n flows in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_shadow_breath_2;
    break;
  case 11:
    act("$n breathes &+Lgas&n!", 1, ch, 0, 0, TO_ROOM);
    act("You breathe &+Lgas&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A blast of &+ggas&n shoots in from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_blinding_breath;
    break;
  case 12:
    act("$n spreads $s wings and emits a &+Rshimmering &+rlight&n!", 1, ch, 0, 0, TO_ROOM);
    act("You spread your wings and emit a &+Rshimmering &+rlight&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Rshimmering&n light blasts through from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_crimson_light;
    break;
  case 13:
    act("$n spreads $s wings and emits a &+Bshimmering &+blight&n!", 1, ch, 0, 0, TO_ROOM);
    act("You spread your wings and emit a &+Bshimmering &+blight&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Gshimmering&n light blasts through from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_jasper_light;
    break;
  case 14:
    act("$n spreads $s wings and emits a &+Gshimmering &+glight&n!", 1, ch, 0, 0, TO_ROOM);
    act("You spread your wings and emit a &+Gshimmering &+glight&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Bshimmering&n light blasts through from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_azure_light;
    break;
  case 15:
    act("$n spreads $s wings and emits a &+Lshimmering &+wlight&n!", 1, ch, 0, 0, TO_ROOM);
    act("You spread your wings and emit a &+Lshimmering &+wlight&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Lshimmering&n light blasts through from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_basalt_light;
    break;
  case 16:
    act("$n spreads $s wings and emits a &+Rshimmering &+rlight&n!", 1, ch, 0, 0, TO_ROOM);
    act("You spread your wings and emit a &+Rshimmering &+rlight&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Rshimmering&n light blasts through from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_crimson_light_2;
    break;
  case 17:
    act("$n spreads $s wings and emits a &+Bshimmering &+blight&n!", 1, ch, 0, 0, TO_ROOM);
    act("You spread your wings and emit a &+Bshimmering &+blight&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Bshimmering&n light blasts through from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_azure_light_2;
    break;
  case 18:
    act("$n spreads $s wings and emits a &+Gshimmering &+glight&n!", 1, ch, 0, 0, TO_ROOM);
    act("You spread your wings and emit a &+Gshimmering &+glight&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Gshimmering&n light blasts through from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_jasper_light_2;
    break;
  case 19:
    act("$n spreads $s wings and emits a &+Lshimmering &+wlight&n!", 1, ch, 0, 0, TO_ROOM);
    act("You spread your wings and emit a &+Lshimmering &+wlight&n!", 0, ch, 0, 0, TO_CHAR);
    if(dir != -1)
      sprintf(buf, "A &+Lshimmering&n light blasts through from the %s!\r\n",
              dirs[(int) rev_dir[dir] - 1]);
    funct = spell_basalt_light_2;
    break;
  }

  dir = -1;

  if(dir == -1)
  { 
    if(IS_FIGHTING(ch))
        victim = ch->specials.fighting;
        
   /* cast_as_damage_area(ch, funct, GET_LEVEL(ch), victim,*/
	cast_as_damage_area(ch, funct, BOUNDED(1, GET_LEVEL(ch), 20), victim,
        get_property("dragon.Breath.area.minChance", 60),
        get_property("dragon.Breath.area.chanceStep", 20));
    
    CharWait(ch, PULSE_VIOLENCE * 2.5);
    if(GET_MASTER(ch))
      CharWait(ch, PULSE_VIOLENCE * 4);
    waited = TRUE;
  }
  else
  {
    /* begin looking in current room */
    if(VIRTUAL_CAN_GO(ch->in_room, dir))
      room = world[ch->in_room].dir_option[dir]->to_room;
    else
    {
      send_to_char("Umm. I don't believe there is anything that direction.\r\n", ch);
      return;
    }

    for (i = 0; i < distance; i++)
    {
      if(room != ch->in_room)
        send_to_room(buf, room);
        
      cast_as_damage_area(ch, funct, GET_LEVEL(ch), NULL,
          get_property("dragon.Breath.area.minChance", 60),
          get_property("dragon.Breath.area.chanceStep", 20));
          
      if(VIRTUAL_CAN_GO(room, dir))
        room = world[room].dir_option[dir]->to_room;
      else
      {
        if(GET_MASTER(ch))
          CharWait(ch, PULSE_VIOLENCE * 4);
        return;
      }
    }
  }

  if(!waited)
    if(GET_MASTER(ch))
      CharWait(ch, PULSE_VIOLENCE * 4);
}

void StompAttack(P_char ch)
{
  P_char   tch, tch_next, chMaster = NULL;
  
  if(!SanityCheck(ch, "StompAttack"))
  {
    return;
  }

  if (IS_PC_PET(ch))
  {
    chMaster = GET_MASTER(ch);
  }
    
  act("$n &=LWstomps&n the ground with $s giant foot!",
    0, ch, 0, 0, TO_ROOM);

  for (tch = world[ch->in_room].people; tch; tch = tch_next)
  {
    tch_next = tch->next_in_room;

    if(!(tch))
    {
      continue;
    }
    
    if (chMaster &&
        !IS_FIGHTING(ch))
    { 
      if (tch == chMaster ||
          tch == ch)
      {
        continue;
      }
    }
    else if(!IS_PC(tch) &&
            (!tch->following || IS_NPC(tch->following)) &&
            (ch->specials.fighting != tch) &&
            (tch->specials.fighting != ch))
    {
      continue;
    }
    
    if (IS_TRUSTED(tch))
    {
      continue;
    }
    
    if(IS_OP_GOLEM(tch) ||
       IS_GH_GOLEM(tch) ||
       IS_NEXUS_GUARDIAN(tch) || 
       IS_ELITE(tch) || 
       IS_IMMATERIAL(tch) ||
       IS_GREATER_RACE(tch) ||
       GET_POS(tch) != POS_STANDING)
    {
      continue;
    }
    
    if(ch->group != NULL &&
       ch->group == tch->group)
    {
      continue;
    }
    
    if(((IS_FIGHTING(tch) &&
        (tch->specials.fighting == ch)) ||
        !IS_FIGHTING(tch)) &&
        !IS_DRAGON(tch) &&
	!IS_TITAN(tch) &&
        !IS_AVATAR(tch))
    {
      if (!StatSave(tch, APPLY_AGI, (int) (-1 * GET_LEVEL(ch) / 10)))
      {
        SET_POS(tch, POS_SITTING + GET_STAT(tch));
        CharWait(tch, PULSE_VIOLENCE * 2);
        act("&+yThe powerful stomp sends out a shockwave knocking you to the &+Lground!&n",
          FALSE, tch, 0, 0, TO_CHAR);
        act("&+yThe shockwave sends $n &+ycrashing to the &+Lground!&n",
          FALSE, tch, 0, 0, TO_ROOM);

        damage(ch, tch,
               dice(4, (GET_LEVEL(ch) / 2)),
               TYPE_UNDEFINED);
      }
      else
      {
        send_to_char("You nimbly dodge the shockwave!\r\n", tch);
        act("$N dodges your stomp.", 0, ch, 0, tch, TO_CHAR);
      }
    }
    if (!char_in_list(ch))
    {
      return;
    }
  }
}

/*
 * newish attack type, this is a sweep (with a dragon's tail to start
 * with) which will knock down several opponents (like bash), if they fail
 * a save against Dex (at -2) -JAB
 */
void SweepAttack(P_char ch)
{
  P_char tch, tch_next, chMaster;
  int    chance;

  if( !SanityCheck(ch, "SweepAttack") )
  {
    return;
  }

  // Get master (PC only).
  if( (chMaster = GET_MASTER(ch)) && IS_NPC(chMaster) )
  {
    chMaster = NULL;
  }

  if( IS_DRACOLICH(ch) )
  {
    act("$n &=LWlashes&n out with $s bony tail!", 0, ch, 0, 0, TO_ROOM);
  }
  else
  {
    act("$n &=LWlashes&n out with $s mighty tail!", 0, ch, 0, 0, TO_ROOM);
  }

  // Normally, Dracos would have chance = -5, but nerfing them some.
  chance = IS_DRACOLICH(ch) ? 5 : GET_LEVEL(ch) / -10;

  for( tch = world[ch->in_room].people; tch; tch = tch_next )
  {
    tch_next = tch->next_in_room;

    if( !tch || tch == chMaster || tch == ch || IS_TRUSTED(tch) )
    {
      continue;
    }

    // If not a PC pet, and NPC, and tch is not a PC pet, and they're not fighting eachother, skip.
    if( !chMaster && IS_NPC(tch) && (!tch->following || IS_NPC(tch->following))
      && (ch->specials.fighting != tch) && (tch->specials.fighting != ch))
    {
      continue;
    }

    // If they're not sweepable, skip.
    if( IS_OP_GOLEM(tch) || IS_GH_GOLEM(tch) || IS_NEXUS_GUARDIAN(tch)
      || IS_ELITE(tch) || IS_IMMATERIAL(tch) || IS_GREATER_RACE(tch)
      || GET_POS(tch) != POS_STANDING )
    {
      continue;
    }

    // If they're grouped, skip
    if( ch->group != NULL && ch->group == tch->group )
    {
      continue;
    }

    // If target is fighting sweeper or not fighting.  And target not a dragon.
    if( ((IS_FIGHTING(tch) && (tch->specials.fighting == ch))
      || !IS_FIGHTING(tch)) && !IS_DRAGON(tch) )
    {
      if( !StatSave(tch, APPLY_AGI, chance) )
      {
        SET_POS(tch, POS_SITTING + GET_STAT(tch));
        CharWait(tch, PULSE_VIOLENCE * 2);
        act("&+yThe powerful sweep sends you crashing to the &+Lground!&n",
          FALSE, tch, 0, 0, TO_CHAR);
        act("$n &+ycrashes to the &+Lground!&n", FALSE, tch, 0, 0, TO_ROOM);

        damage(ch, tch, dice(4, MAX(1, GET_LEVEL(ch) / 4)), TYPE_UNDEFINED);
        // Dracos hurt themselves via tailsweeps 15-25 damage (not very signifigant since they vamp tho).
        if( IS_ALIVE(ch) && IS_DRACOLICH(ch) )
        {
          damage(tch, ch, dice(4, 11) + 56, TYPE_UNDEFINED);
        }
      }
      else
      {
        send_to_char("You nimbly dodge the sweep!\r\n", tch);
        act("$N dodges your sweep.", 0, ch, 0, tch, TO_CHAR);
      }
    }
    if( !char_in_list(ch) )
    {
      return;
    }
  }
}


bool MobAlchemist(P_char ch)
{
  P_char   tch, next_ch;
  P_obj    t_obj;
  char     Gbuf2[MAX_STRING_LENGTH];
  int      level, i;
  int      type, number_potions, potions = 0;
  int      n_atkr;

  level = GET_LEVEL(ch);

  potions = count_potions(ch);

  if(!potions && IS_FIGHTING(ch) && !number(0, 4))
  {
    do_flee(ch, 0, 0);
    return (TRUE);
  }

  if((!IS_FIGHTING(ch) || !number(0, 8)) && potions < 10)
  {
    switch (((level - 1) / 5) + 1)
    {
    case 1:

      break;
    case 2:
      number_potions = level - potions;

      for (i = 0; i < number_potions; i++)
        MobAlchemistGetPotions(ch, spl2potion(SPELL_NITROGEN), 1);

      break;
    case 3:
      number_potions = level - 2 - potions;

      for (i = 0; i < number_potions; i++)
        if(number(0, 4))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_NITROGEN), 1);
        else
          MobAlchemistGetPotions(ch, spl2potion(SPELL_DISPEL_MAGIC), 1);

      break;

    case 4:
      number_potions = level - 4 - potions;

      for (i = 0; i < number_potions; i++)
        if(number(0, 5))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_NITROGEN), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_DISPEL_MAGIC), 1);
        else
          MobAlchemistGetPotions(ch, spl2potion(SPELL_WITHER), 1);

      break;

    case 5:
      number_potions = 11 + number(0, 9) - potions;

      for (i = 0; i < number_potions; i++)
        if(number(0, 4))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_NITROGEN), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_DISPEL_MAGIC), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_WITHER), 1);
        else
          MobAlchemistGetPotions(ch, spl2potion(SPELL_SLOW), 1);
      break;
    case 6:
      number_potions = 12 + number(0, 9) - potions;

      for (i = 0; i < number_potions; i++)
        if(number(0, 3))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_NITROGEN), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_GREASE), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_DISPEL_MAGIC), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_WITHER), 1);
        else
          MobAlchemistGetPotions(ch, spl2potion(SPELL_SLOW), 1);

      break;
    case 7:
      number_potions = 13 + number(0, 9) - potions;

      for (i = 0; i < number_potions; i++)
        if(number(0, 3))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_NAPALM), 1);
        if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_NITROGEN), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_GREASE), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_DISPEL_MAGIC), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_WITHER), 1);
        else
          MobAlchemistGetPotions(ch, spl2potion(SPELL_SLOW), 1);

      break;
    case 8:
      number_potions = 14 + number(0, 9) - potions;

      for (i = 0; i < number_potions; i++)
        if(number(0, 4))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_GLASS_BOMB), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_GREASE), 1);
        if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_NAPALM), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_DISPEL_MAGIC), 1);
        else
          MobAlchemistGetPotions(ch, spl2potion(SPELL_SLOW), 1);

      break;
    case 9:
    case 10:
      number_potions = 15 + number(0, 9) - potions;

      for (i = 0; i < number_potions; i++)
        if(number(0, 5))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_STRONG_ACID), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_GREASE), 1);
        if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_GLASS_BOMB), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_DISPEL_MAGIC), 1);
        else
          MobAlchemistGetPotions(ch, spl2potion(SPELL_SLOW), 1);
      break;
    case 11:
    case 12:
    case 13:
      number_potions = 17 + number(0, 9) - potions;

      for (i = 0; i < number_potions; i++) {
        if(number(0, 3))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_STRONG_ACID), 1);
        else if(number(0, 2))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_GREATER_LIVING_STONE), 1);
        if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_GLASS_BOMB), 1);
        else if(number(0, 1))
          MobAlchemistGetPotions(ch, spl2potion(SPELL_DISPEL_MAGIC), 1);
        else
          MobAlchemistGetPotions(ch, spl2potion(SPELL_SLOW), 1);
      }

      break;
    default:
      wizlog(57, "mob %s failed to make any potions in [%d]", GET_NAME(ch),
             ch->in_room);
    }
    if(i > 0)
    {
      send_to_char("&+LYou've created some potions.&n\r\n", ch);
      act("$n&+L quickly mixes some potions...&n", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  if(!IS_FIGHTING(ch))
    return FALSE;

  t_obj = NULL;

  tch = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);

  if((tch || (tch = ch->specials.fighting)) && (t_obj = get_potion(ch)) &&
      t_obj)
    if(throw_potion(ch, t_obj, tch, 0))
    {
      CharWait(ch, PULSE_VIOLENCE);
      return TRUE;
    }
    else
      return FALSE;

  return FALSE;

}

bool MobMonk(P_char ch)
{
  P_char   victim = NULL;
  P_char   tch;
  int      n_atkr = 0;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return false;
  }
  
  if(CAN_SPEAK(ch))
  {
    if(GET_LEVEL(ch) >= 21 &&
      (affected_by_spell(ch, SPELL_CURSE) ||
      affected_by_spell(ch, SPELL_WITHER) ||
      affected_by_spell(ch, SPELL_POISON) ||
      IS_AFFECTED2(ch, AFF2_POISONED) ||
      affected_by_spell(ch, SPELL_BLINDNESS) ||
      IS_AFFECTED(ch, AFF_BLIND) ||
      affected_by_spell(ch, SPELL_DISEASE)) )
    {
      do_chant(ch, " cell adjustment", 0);
      return TRUE;
    }
    else if(GET_CHAR_SKILL(ch, SKILL_REGENERATE) &&
           GET_HIT(ch) < GET_MAX_HIT(ch) &&
           !number(0, 1) &&
           !affected_by_skill(ch, SKILL_REGENERATE) &&
           !affected_by_spell(ch, SPELL_REGENERATION) &&
           !affected_by_spell(ch, SPELL_ACCEL_HEALING))
    {
      do_chant(ch, " regen", 0);
      return TRUE;
    }
    else if(!number(0, 3) &&
            GET_CHAR_SKILL(ch, SKILL_HEROISM) &&
            !affected_by_skill(ch, SKILL_HEROISM) &&
            !affected_by_spell(ch, SONG_HEROISM))
    {
      do_chant(ch, " heroism", 0);
      return TRUE;
    }
  }
  if(IS_FIGHTING(ch) &&
     !number(0, 6) &&
     ((number(1, 100) < (GET_LEVEL(ch) * 3))) &&
     ((n_atkr > 1) ||
     has_help(ch->specials.fighting)))
  {
    if(GET_SPEC(ch, CLASS_MONK, SPEC_WAYOFSNAKE) )
    {
      if(!number(0,1) &&
        !affected_by_skill(ch, SKILL_FLURRY_OF_BLOWS) )
      {
        do_flurry_of_blows(ch, " all");
        return TRUE;
      }
    }

    P_char   juiciest = NULL;
    int      tenderness, t_tend;

    LOOP_THRU_PEOPLE(tch, ch)
    {
      if(IS_FIGHTING(tch) &&
         ((tch->specials.fighting == ch) ||
         are_together(tch, ch->specials.fighting)) &&
         CAN_SEE(ch, tch))
      {
        t_tend = GET_HIT(tch);

        if(IS_CASTER(tch))
          t_tend += GET_LEVEL(ch);
        else if(IS_SEMI_CASTER(ch))
          t_tend += GET_LEVEL(ch) / 2;
        if(has_skin_spell(tch))
          t_tend <<= 1;
        if(IS_PC(tch) &&
           HAS_MEMORY(ch) &&
           !CheckFor_remember(ch, tch))
        {
          t_tend >>= 1;
        }
        if(!juiciest ||
          (t_tend < tenderness))
        {
          juiciest = tch;
          tenderness = t_tend;
        }
      }
    }

    if(juiciest &&
       (juiciest != ch->specials.fighting))
    {
      attack(ch, juiciest);
    }
  }

  if(IS_FIGHTING(ch) && (victim = ch->specials.fighting))
  {
    if(GET_SPEC(ch, CLASS_MONK, SPEC_WAYOFSNAKE) ||
      (IS_ELITE(ch) && GET_CLASS(ch, CLASS_MONK)))
    {
      if(!number(0, 1) &&
         !affected_by_skill(ch, SKILL_JIN_TOUCH) )
      {
        do_chant(ch, " jin touch", 0);
        return TRUE;
      }
    }
    
    char     buf[100];

    buf[0] = '\0';

    switch (number(1, 12))
    {
    case 1:
      if( !affected_by_skill(ch, SKILL_COMBINATION) )
      {
        do_combination(ch, 0, 0);
        return TRUE;
      }

    case 2:
    case 3:
      if(chance_roundkick(ch, victim) > number(30, 50))
      {
        roundkick(ch, victim);
        return TRUE;
      }

    case 4:
      do_dragon_punch(ch, buf, 0);
      return TRUE;

    case 5:
      if(chance_kick(ch, victim) > number(30, 50))
      {
        kick(ch, victim);
        return TRUE;
      }

    case 6:
      if( GET_SPEC(ch, CLASS_MONK, SPEC_WAYOFDRAGON) && !affected_by_skill(ch, SKILL_FIST_OF_DRAGON) )
      {
        strcpy(buf, " fist of dragon");
        break;
      }

    case 7:
      if(NumAttackers(ch) > 1 && !affected_by_skill(ch, SKILL_BUDDHA_PALM) )
      {
        strcpy(buf, " buddha palm");
        break;
      }

    case 8:
      if( !affected_by_spell(ch, SKILL_QUIVERING_PALM) )
      {
        strcpy(buf, " quivering palm");
        break;
      }

    case 9:
      if(isSpringable(ch, ch->specials.fighting))
      {
        do_kneel(ch, 0, CMD_KNEEL);
        do_springleap(ch, buf, 0);
        return TRUE;
      }
    default:
      break;
    }

    if(*buf)
    {
      do_chant(ch, buf, 0);
      return TRUE;
    }
  }
  return FALSE;
}
// AI function that allows mobs to check target before attempting a gaze.
// Lucrot Oct08
bool GOOD_FOR_GAZING(P_char ch, P_char victim)
{
  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  if( GET_CHAR_SKILL(ch, SKILL_GAZE) < 1 && !has_innate(ch, INNATE_GAZE) )
  {
    return FALSE;
  }

  if( isBashable(ch, victim, TRUE) && !has_innate(victim, INNATE_EYELESS) && !IS_AFFECTED(victim, AFF_BLIND)
    && GET_POS(ch) == POS_STANDING && !affected_by_spell(ch, SKILL_BASH) && !affected_by_spell(victim, SKILL_GAZE) )
  {
    if( get_takedown_size(victim) > get_takedown_size(ch) + 1 )
    {
      return FALSE;
    }
    if( get_takedown_size(victim) < get_takedown_size(ch) - 2 )
    {
      return FALSE;
    }
    return TRUE;
  }

  return FALSE;
}

// AI function that allows mobs to check target before attempting to flank.
// Lucrot Oct08
bool GOOD_FOR_FLANKING(P_char ch)
{
  if(!(ch))
  {
    logit(LOG_EXIT, "GOOD_FOR_FLANKING called in mobact.c with no ch");
    raise(SIGSEGV);
    return false;
  }
  
  if(IS_ALIVE(ch) &&
    !IS_IMMOBILE(ch) &&
    GET_CHAR_SKILL(ch, SKILL_FLANK) &&
    GET_CHAR_SKILL(ch, SKILL_FLANK) > 0 &&
    GET_POS(ch) == POS_STANDING &&
    !IS_AFFECTED2(ch, AFF2_STUNNED) &&
    !get_linked_char(ch, LNK_FLANKING) &&
    !affected_by_spell(ch, SKILL_FLANK))
  {
    return true;
  }
  
  return false;
}

bool MobDreadlord(P_char ch)
{
  char buf[MAX_INPUT_LENGTH];
  P_char tch;

  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return false;
  }
  
  if(IS_PC(ch))
  {
    return false;
  }
  
  if(IS_SET(ch->only.npc->aggro_flags, AGGR_ALL))
  {
    for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(IS_PC(tch) &&
        IS_ALIVE(tch) &&
        !GET_MASTER(tch) &&
        (IS_MAGE(tch) || IS_CLERIC(tch)) &&
        number(0, 2)) 
      {
        MobStartFight(ch, tch);
        return true;
      }
    }
  }
  
  if(GOOD_FOR_GAZING(ch, ch->specials.fighting) &&
     number(0, 2))
  {
    gaze(ch, ch->specials.fighting);
  }
  
  if(GOOD_FOR_FLANKING(ch) &&
    number(0, 2))
  {
    flank(ch, ch->specials.fighting);
    return true;
  }

  return false;
}

bool MobBerserker(P_char ch)
{
  if(IS_FIGHTING(ch))
  {
    if(!IS_AFFECTED2(ch, AFF2_FLURRY) && !number(0, 1))
    {
      do_rage(ch, NULL, CMD_RAGE);
      return TRUE;
    }

    if( isBashable(ch, GET_OPPONENT(ch)) && !number(0, 1) && (GET_CHAR_SKILL(ch, SKILL_MAUL) > 0) )
    {
      maul( ch, GET_OPPONENT(ch) );
      return TRUE;
    }
  }

  if(!affected_by_spell(ch, SKILL_BERSERK) && !number(0,1) )
  {
    do_berserk(ch, NULL, CMD_BERSERK);
    return TRUE;
  }

  if(!affected_by_spell(ch, SKILL_WAR_CRY) && !number(0,3) )
  {
    do_war_cry(ch, NULL, CMD_WARCRY);
    return TRUE;
  }


  if(!affected_by_spell(ch, SKILL_INFURIATE) && !number(0,1) )
  {
    do_infuriate(ch, NULL, CMD_INFURIATE);
    return TRUE;
  }

  if( !affected_by_spell(ch, SPELL_HASTE) && !affected_by_spell(ch, SPELL_BLUR) && !number(0,2) )
  {
    do_rampage(ch, NULL, CMD_RAMPAGE);
    return TRUE;
  }

  return FALSE;
}

/* Lets try our hand at making bards a bit more intuitive in combat */
bool MobBard(P_char ch)
{
  P_char victim;
  int dam;

  dam = GET_MAX_HIT(ch) - GET_HIT(ch);

  if(MobShouldFlee(ch))
  {
    do_flee(ch, 0, 0);
    return false;
  }
  
  if(IS_FIGHTING(ch))
  {
   switch (number(1, 5))
   {
    case 1:
      {
        do_play(ch, " harming", CMD_PLAY);
        return TRUE;
      }
    case 2:
      if(!IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) ||
        !IS_HOMETOWN(ch->in_room))
      {
        do_play(ch, " chaos", CMD_PLAY);
        return TRUE;
      }
    case 3:
      if(GET_CHAR_SKILL(ch, SKILL_SHRIEK) > 0)
      {
        do_shriek(ch, NULL, CMD_SHRIEK);
        return TRUE;
      }
      case 4:
      if(OUTSIDE(ch) && !number(0, 1))
      {
        do_play(ch, " storms", CMD_PLAY);
        return TRUE;
      }
      case 5:
      {
        if(GET_LEVEL(ch) > 55 && IS_ELITE(ch))
        do_play(ch, " charming", CMD_PLAY);
        return TRUE;
      }
    default:
      break;
      
    return true;
   }
  }

  // if(!IS_AFFECTED(ch, AFF_PROT_FIRE) && !number(0,1) )
  // {
    // do_play(ch, " dragons", CMD_PLAY);
    // return TRUE;
  // }

  if(dam > 400)
  {
    do_flee(ch, 0, 0);
    do_play(ch, " healing", CMD_PLAY);
    return TRUE;
  }

  if(!affected_by_spell(ch, SPELL_STONE_SKIN) &&
    (GET_LEVEL(ch) > 50) &&
    !number(0, 1) &&
    !has_skin_spell(ch))
  {
    do_play(ch, " protection", CMD_PLAY);
    return TRUE;
  }

  if(!IS_AFFECTED(ch, AFF_FLY) &&
     !number(0, 1))
  {
    do_play(ch, " flight", CMD_PLAY);
    return TRUE;
  }

  if(!affected_by_spell(ch, SPELL_HASTE) &&
    !number(0,2) )
  {
    do_play(ch, " heroism", CMD_PLAY);
    return TRUE;
  }

  return FALSE;
}

/* Do specials warrior attacks, currently just bash/kick/hitall -JAB */

bool MobWarrior(P_char ch)
{
  P_char tch, next_ch;
  int n_atkr;

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  n_atkr = NumAttackers(ch);

  if( n_atkr > 1 && !number(0, 1) )
  {
    for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
    {
      if( (ch == tch->specials.fighting) && (IS_MAGE(tch) || IS_CLERIC(tch))
        && (GET_POS(ch) == POS_STANDING) && number(0, 1) )
      {
        break;
      }
    }

    if( (ch && tch) && (ch->in_room == tch->in_room) && isBashable(ch, tch)
      && (GET_POS(tch) == POS_STANDING) && !IS_IMMATERIAL(ch) )
    {
      // Switch to target instead of bashing.
      if( ch != tch->specials.fighting && number(0, 1))
      {
        attack(ch, tch);
      }
      else
      {
        bash(ch, tch);
      }
      return TRUE;
    }
  }

  if( isKickable(ch, ch->specials.fighting) && number(0, 2) )
  {
    do_kick(ch, 0, 0);
    return TRUE;
  }
  else if( !number(0, 3) && ch->specials.fighting
    && (GET_POS(GET_OPPONENT(ch)) == POS_STANDING) && isBashable(ch, GET_OPPONENT(ch)) )
  {
    do_bash(ch, 0, 0);

    if( !IS_ALIVE(ch) || !IS_ALIVE(GET_OPPONENT(ch)) || (GET_POS(ch) < POS_STANDING)
      || (GET_POS(GET_OPPONENT(ch)) < POS_STANDING))
    {
      return TRUE;
    }
  }
  else if( (n_atkr > 2) && (GET_LEVEL(ch) > 14) && number(0, 2) )
  {
    /*
     * psuedo hitall func, takes a swing at all chars fighting 'ch'
     */

    for (tch = world[ch->in_room].people; tch; tch = next_ch)
    {
      next_ch = tch->next_in_room;

      if((tch != ch) &&
        IS_FIGHTING(tch) &&
        ((tch->specials.fighting == ch) ||
        (ch->specials.fighting == tch)))
      {
        if(number(0, 135) > MAX(99, ((GET_LEVEL(ch) - 10) * 9)))
#ifndef NEW_COMBAT
          hit(ch, tch, ch->equipment[PRIMARY_WEAPON]);
#else
          hit(ch, tch, ch->equipment[WIELD], TYPE_UNDEFINED,
              getBodyTarget(ch), TRUE, FALSE);
#endif
      }
    }
    if(char_in_list(ch))
      CharWait(ch, MAX(PULSE_VIOLENCE * n_atkr, 3));
    return TRUE;
  }
  else if(((number(1, 100) < (GET_LEVEL(ch) * 3))) &&
         ((n_atkr > 1) || has_help(ch->specials.fighting)))
  {
    /*
     * The anti-tank clause.  The way players set things up, there is
     * a real good chance that the person we are currently beating on,
     * is the WORST person to be attacking (from our viewpoint), so
     * let's see about switching to a juicier target.  JAB
     */
    P_char   juiciest = NULL;
    int      tenderness, t_tend;

    LOOP_THRU_PEOPLE(tch, ch)
    {
      if(IS_FIGHTING(tch) &&
        ((tch->specials.fighting == ch) ||
        are_together(tch, ch->specials.fighting)) &&
        CAN_SEE(ch, tch))
      {
        t_tend = GET_HIT(tch) /*+ (100 - GET_AC(tch)) */ ;

        if(IS_CASTER(tch))
          t_tend += GET_LEVEL(ch);
        else if(IS_SEMI_CASTER(ch))
          t_tend += GET_LEVEL(ch) / 2;
        if(has_skin_spell(tch))
          t_tend <<= 1;
        if(IS_PC(tch) && HAS_MEMORY(ch) && !CheckFor_remember(ch, tch))
          t_tend >>= 1;
        if(!juiciest || (t_tend < tenderness))
        {
          juiciest = tch;
          tenderness = t_tend;
        }
      }
    }

    if(juiciest && (juiciest != ch->specials.fighting))
      attack(ch, juiciest);
  }
  return FALSE;
}


bool MobRanger(P_char ch)
{
  P_obj    t_obj;
  P_char   vict;
  P_char   tch, next_ch;
  int      n_atkr;

  n_atkr = NumAttackers(ch);
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return false;
  }

  if((n_atkr > 1) &&
     !number(0, 4))
  {
    tch = PickTarget(ch);
    if((ch && tch) &&
       (ch->in_room == tch->in_room) &&
       (tch != ch->specials.fighting))
    {
      attack(ch, tch);
      return (TRUE);
    }
  }

  if(number(0, 2) &&
     ch->specials.fighting &&
     isSpringable(ch, ch->specials.fighting))
  {
    do_kneel(ch, 0, CMD_KNEEL);
    do_springleap(ch, NULL, 0);
  }

  if(GET_CHAR_SKILL(ch, SKILL_WHIRLWIND) > 0 &&
     !affected_by_spell(ch, SKILL_WHIRLWIND) &&
     ch->specials.fighting &&
     !number(0, 2))
  {
    do_whirlwind(ch, 0, 0);
    return true;
  }

  if(!ch->specials.fighting ||
    (GET_POS(ch) < POS_STANDING) ||
    (GET_POS(ch->specials.fighting) < POS_STANDING))
  {
    return TRUE;
  }
  
  if(IS_MULTICLASS_NPC(ch) &&
     !number(0, 2))
  {
    return FALSE;
  }
  
  if(CastRangerSpell(ch, 0, FALSE))
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}


/*
 * Idea behind MobReaver - first some random chance for wild switch or chance for more thought switch
 * then bash/trip, and finally if all failed do CastReaverSpell
 */

bool MobMercenary(P_char ch)
{
  P_obj    t_obj;
  P_char   vict;
  P_char   tch, next_ch;
  int      n_atkr;
  struct affected_type *af;

  n_atkr = NumAttackers(ch);

  if((n_atkr > 1) && !number(0, 4))
  {
    tch = PickTarget(ch);
    if((ch && tch) && (ch->in_room == tch->in_room) &&
        (tch != ch->specials.fighting))
      attack(ch, tch);
    return (TRUE);
  }

  if(IS_FIGHTING(ch) && (vict = ch->specials.fighting) &&
      !IS_AFFECTED(ch, AFF_BLIND) && !number(0, 2))
  {
    if(number(0, 2) && !GRAPPLE_TIMER(ch) &&
        (grapple_check_hands(ch) != FALSE) &&
        !((GET_ALT_SIZE(vict) > GET_ALT_SIZE(ch) + 1) ||
          (GET_ALT_SIZE(vict) < GET_ALT_SIZE(ch) - 1)) )
    {
      if( (GET_POS(vict) == POS_STANDING) && (GET_POS(ch) == POS_STANDING) )
      {

        if((af = get_spell_from_char(vict, SKILL_ARMLOCK)) != NULL)
        {
          if(af->modifier == HOLD_IMPROVED)
          {
            do_groundslam(ch, 0, CMD_GROUNDSLAM);
            do_leglock(ch, 0, CMD_LEGLOCK);
            return TRUE;
          }
        }

        if(vict->player.m_class &
            (CLASS_SORCERER | CLASS_PSIONICIST | CLASS_CLERIC |
             CLASS_CONJURER | CLASS_WARLOCK | CLASS_ILLUSIONIST |
             CLASS_BARD | CLASS_DRUID | CLASS_ETHERMANCER | CLASS_SHAMAN |
             CLASS_NECROMANCER | CLASS_THEURGIST | CLASS_SUMMONER))
        {
          do_headlock(ch, 0, CMD_HEADLOCK);
        }
        else
        {
          do_bearhug(ch, 0, CMD_BEARHUG);
        }
        return TRUE;
      }
      else if( (GET_POS(vict) != POS_STANDING) )
      {
        do_kneel(ch, 0, CMD_KNEEL);
        do_leglock(ch, 0, CMD_KNEEL);
        return TRUE;
      }
    }

    if(number(0, 2) && !IS_SET(vict->specials.act, ACT_NO_BASH) &&
        !(IS_DEMON(vict) || IS_DRAGON(vict) || IS_GIANT(vict) ||
          (GET_RACE(vict) == RACE_PLANT)) &&
        !((GET_ALT_SIZE(vict) > GET_ALT_SIZE(ch) + 1) ||
          (GET_ALT_SIZE(vict) < GET_ALT_SIZE(ch) - 1)) &&
        HAS_FOOTING(ch) && (GET_POS(vict) == POS_STANDING))
    {
      do_tackle(ch, 0, CMD_TACKLE);
    }
    else if (!affected_by_spell_flagged(vict, SKILL_THROAT_CRUSH, AFFTYPE_CUSTOM1) && number(0, 1)
      && GET_CHAR_SKILL_P(ch, SKILL_THROAT_CRUSH) )
    {
      do_throat_crush(ch, 0, CMD_THROAT_CRUSH);
    }
    else
      do_headbutt(ch, 0, CMD_HEADBUTT);
    return TRUE;
  }

  return FALSE;
}

bool MobReaver(P_char ch)
{
  P_obj    t_obj;
  P_char   vict;
  P_char   tch, next_ch;
  int      n_atkr;

  n_atkr = NumAttackers(ch);

  if((n_atkr > 1) && !number(0, 4))
  {
    tch = PickTarget(ch);
    if((ch && tch) && (ch->in_room == tch->in_room) &&
        (tch != ch->specials.fighting))
      attack(ch, tch);
    return (TRUE);
  }

  if(IS_FIGHTING(ch) && (vict = ch->specials.fighting) &&
      !IS_AFFECTED(ch, AFF_BLIND) && !number(0, 2))
    /*if(number(0, 1))
    {
      if((MAX(100, GET_LEVEL(ch) * 2) > number(40, 60)) &&
          !IS_SET(vict->specials.act, ACT_NO_BASH) &&
          !(IS_DEMON(vict) || IS_DRAGON(vict) || IS_GIANT(vict) ||
            (GET_RACE(vict) == RACE_PLANT)) &&
          !((GET_ALT_SIZE(vict) > GET_ALT_SIZE(ch) + 1) ||
            (GET_ALT_SIZE(vict) < GET_ALT_SIZE(ch))) &&
          HAS_FOOTING(ch) && (GET_POS(vict) == POS_STANDING))
      {
        //wizlog(56,"%s tried to trip",GET_NAME(ch));
        do_trip(ch, 0, CMD_TRIP);
      }
    }*/
    /*else
      if((GET_POS(vict) == POS_STANDING) &&
          isBashable(ch, ch->specials.fighting))
    {
      //  wizlog(56,"%s tried to bash",GET_NAME(ch));
      do_bash(ch, 0, 0);
    }*/

  if(!ch || !ch->specials.fighting || (GET_POS(ch) < POS_STANDING) ||
      (GET_POS(ch->specials.fighting) < POS_STANDING))
    return TRUE;

  if(IS_MULTICLASS_NPC(ch) && !number(0, 2))
    return FALSE;

  if(CastReaverSpell(ch, 0, FALSE))
    return TRUE;
  else
    return FALSE;
}

/*
 * rogue mobs in combat, really suck, so we are gonna make them a just a
 * bit smarter.  JAB
 */

bool MobThief(P_char ch)
{
  P_obj    bs_weap = NULL;
  P_obj    t_obj;
  int      start_room = ch->in_room;
  bool     weapon_pos = FALSE;

  /*
   * basic strategy, flee/wield a backstabbing weapon if we have one/hide
   * and wait for attackers to come looking, then let memory or agg
   * flags handle backstabbing the miscreants.  Here we go...
   */

  /*
   * first check if they have a backstabbing weapon either in hand or in
   * inven.  If not, they may as well stand and fight, no advantage in
   * fleeing (wimpy can still kick in, this flee is tactical).
   */

  if((ch->equipment[WIELD]) &&
      IS_BACKSTABBER(ch->equipment[WIELD]) &&
      (CAN_WEAR(ch->equipment[WIELD], ITEM_TAKE)) &&
      (CAN_WEAR(ch->equipment[WIELD], ITEM_WIELD)))
  {
    bs_weap = ch->equipment[WIELD];
    weapon_pos = TRUE;
  }
  else if((ch->equipment[SECONDARY_WEAPON]) &&
           IS_BACKSTABBER(ch->equipment[SECONDARY_WEAPON]) &&
           (CAN_WEAR(ch->equipment[SECONDARY_WEAPON], ITEM_TAKE)) &&
           (CAN_WEAR(ch->equipment[SECONDARY_WEAPON], ITEM_WIELD)))
  {
    bs_weap = ch->equipment[SECONDARY_WEAPON];
    weapon_pos = TRUE;
  }
  else
  {
    /*
     * let's see if we are carrying something suitable
     */
    for (t_obj = ch->carrying; t_obj && !bs_weap; t_obj = t_obj->next_content)
      if(IS_BACKSTABBER(t_obj) &&
          (CAN_WEAR(t_obj, ITEM_TAKE)) && (CAN_WEAR(t_obj, ITEM_WIELD)))
        bs_weap = t_obj;
  }


  if(bs_weap)
  {

    /*
     * ok, we can backstab, good deal, let's work at it
     */

    if(!weapon_pos)
    {
      obj_from_char(bs_weap);
      /*
       * got to WIELD our bs weapon (maybe unWIELDing current)
       */
      if(!ch->equipment[WIELD])
        equip_char(ch, bs_weap, WIELD, FALSE);
      else if(!ch->equipment[SECONDARY_WEAPON] &&
               !IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_TWOHANDS))
        equip_char(ch, bs_weap, SECONDARY_WEAPON, FALSE);
      else
      {
        /*
         * got to clear a weapon slot
         */
        if(IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_TWOHANDS))
        {
          if(!IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_NODROP))
          {
            obj_to_char(unequip_char(ch, WIELD), ch);
            equip_char(ch, bs_weap, WIELD, FALSE);
          }
          else
            return TRUE;        /* hosed */
        }
        else if(!IS_SET(ch->equipment[SECONDARY_WEAPON]->extra_flags,
                         ITEM_NODROP))
        {
          obj_to_char(unequip_char(ch, SECONDARY_WEAPON), ch);
          equip_char(ch, bs_weap, SECONDARY_WEAPON, FALSE);
        }
        else if(!IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_NODROP))
        {
          obj_to_char(unequip_char(ch, WIELD), ch);
          equip_char(ch, bs_weap, WIELD, FALSE);
        }
        else
        {
          /* both weapon slots filled with cursed non-bs weapons, we be hosed */
          obj_to_char(bs_weap, ch);
          return TRUE;
        }
      }
    }
    if((!GET_MASTER(ch) ||
         (GET_MASTER(ch)->in_room != ch->in_room)) &&
        room_has_valid_exit(ch->in_room))
    {
      do_flee(ch, 0, 0);
      if(ch->in_room == start_room)
        return TRUE;            /* flee failed, not this time */

      /* we got away, let's get set up */
      if(!IS_AFFECTED(ch, AFF_SNEAK))
        do_sneak(ch, 0, 0);
    }
  }
  /* let's toss some dirt now and then */

  if(IS_FIGHTING(ch) && !IS_AFFECTED(ch->specials.fighting, AFF_BLIND) &&
      GET_CHAR_SKILL(ch, SKILL_DIRTTOSS) && !number(0, 6))
  {
    do_dirttoss(ch, GET_NAME(ch->specials.fighting), CMD_DIRTTOSS);
    return TRUE;
  }

  if(IS_FIGHTING(ch) && !number(0, 2) && GET_CLASS(ch, CLASS_THIEF) &&
      isBashable(ch, GET_OPPONENT(ch)) &&
      get_takedown_size(ch) <= get_takedown_size(GET_OPPONENT(ch)))

  {
    do_trip(ch, GET_NAME(GET_OPPONENT(ch)), CMD_TRIP);
    return TRUE;
  }
  /* now we be cooking with gas, escaped, got bs weapon in hand */

  if(!IS_AFFECTED(ch, AFF_HIDE))
    do_hide(ch, 0, 0);

  return TRUE;
}

void GhostFearEffect(P_char ch)
{
  P_char   tch, next;

  /*
   * what this does is.. go thru list of people in room, and if they
   * don't save, and they are pcs, _and_ not clerics (>10lvl) the fellow
   * gets screwed 'a bit'. :-)
   * over-all, meaning of this is triple-fold; first, it creates market
   * for potions. second, undead get nasty image. third, it makes people
   * consider once or twice before intentionally attacking a ghost.
   */

  if(isname("_nofear_", GET_NAME(ch)))
    return;

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if(IS_PC(tch) && (!GET_CLASS(tch, CLASS_CLERIC) || GET_LEVEL(tch) <= 25)
        && !IS_THRIKREEN(tch) && (!RACE_PUNDEAD(tch) && number(0, 2)))
      if(!NewSaves(tch, SAVING_FEAR, 0) && CAN_SEE(tch, ch) &&
          !IS_TRUSTED(tch))
      {
        act("$N is gripped by great fear after seeing $n's visage!", TRUE, ch,
            0, tch, TO_NOTVICT);
        act("$N is gripped by great fear after seeing your visage!", TRUE, ch,
            0, tch, TO_CHAR);
        act("You are gripped by great fear after seeing $n's visage!", TRUE,
            ch, 0, tch, TO_VICT);

        // age victim if he get unlucky
        if(!number(0, 3))
        {
           send_to_char("You feel older..\r\n", tch);
           AgeChar(tch, 1);
        }
        else
        {
           send_to_char("Focusing your will, you stave off the fearful visage, utilizing every last drop of courage!\r\n", tch);
           return;
        }
      }
  }
}


void MobCombat( P_char ch )
{
  P_char   tch;

  if( !ch )
  {
    logit(LOG_EXIT, "MobCombat called in mobact.c with no ch");
    raise(SIGSEGV);
    return;
  }

  if( IS_PC(ch) || !IS_ALIVE(ch) )
  {
    return;
  }

  if( TRUSTED_NPC(ch) )
  {
    return;
  }

  if( !CAN_ACT(ch) || IS_IMMOBILE(ch) || IS_CASTING(ch) )
  {
    return;
  }

  if( number(0, 2) && ch->specials.fighting
    && isSpringable(ch, ch->specials.fighting) )
  {
    do_kneel(ch, 0, CMD_KNEEL);
    do_springleap(ch, NULL, 0);
    return;
  }

  if( !MIN_POS(ch, POS_STANDING + STAT_NORMAL) )
  {
    do_stand(ch, 0, 0);
  }

  /*
   * Examine call for special procedure
   */
  if( !no_specials && IS_SET(ch->specials.act, ACT_SPEC) && !affected_by_spell(ch, TAG_CONJURED_PET) )
  {
    if( !mob_index[GET_RNUM(ch)].func.mob )
    {
      logit(LOG_MOB, "MobCombat: ACT_SPEC set, but no proc: %s #%d", J_NAME(ch),
        mob_index[GET_RNUM(ch)].virtual_number);
      REMOVE_BIT(ch->specials.act, ACT_SPEC);
      return;
    }
    else if( (*mob_index[GET_RNUM(ch)].func.mob) (ch, 0, CMD_MOB_COMBAT, 0) )
    {
      return;
    }
  }
  // Not much we can do, if they can't stand
  if(!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    return;
  }

  if( (number(1, 400) <= (GET_C_INT(ch) / 4 + GET_LEVEL(ch))) && CAN_ACT(ch) && !IS_ANIMAL(ch) )
  {
    tch = PickTarget(ch);

    if( tch && (tch != ch->specials.fighting) )
    {
      stop_fighting(ch);
      MobStartFight(ch, tch);
    }

    if( !CAN_ACT(ch) || !ch->specials.fighting )
      return;
  }
  else if( !ch->specials.fighting )
  {
    tch = PickTarget(ch);
    if( !tch )
    {
      return;
    }
    if(
#ifdef REALTIME_COMBAT
         ch->specials.combat
#else
         ch->specials.next_fighting
#endif
      )
    {
      stop_fighting(ch);
    }
    MobStartFight(ch, tch);
  }

  if( number(0, 4) && !IS_PC_PET(ch) )
  {
    try_wield_weapon(ch);
  }

  if( IS_DRIDER(ch) )
  {
    if( GenMobCombat(ch) )
    {
      return;
    }
  }

  if( IS_PWORM(ch) )
  {
    if( GenMobCombat(ch) )
    {
      return;
    }
  }

  if( IS_UNDEAD(ch) )
  {
    if( UndeadCombat(ch) )
    {
      return;
    }
  }

  if (IS_ANGEL(ch))
  {
    if( AngelCombat(ch) )
    {
      return;
    }
  }

  if( IS_DEMON(ch) )
  {
    if(DemonCombat(ch))
    {
      return;
    }
  }

  if(IS_PC_PET(ch) && ch->in_room == GET_MASTER(ch)->in_room
    && IS_AFFECTED(ch, AFF_CHARM))
  {
    return;
  }
  // infuse life (epic skill) allows players to not have their pets getting charmed. Huzzah!
/* Guess this is no longer valid.
  if(GET_LEVEL(ch) > 60 && ch->specials.fighting
    && (GET_CHAR_SKILL(GET_MASTER(ch->specials.fighting), SKILL_INFUSE_LIFE) < number(1, 100)))
  {
    if(recharm_ch(ch, ch->specials.fighting, TRUE,
      "$N suddenly appears overcome by $n's charming nature!"))
    {
      return;
    }
  }
*/

  // CAN_BREATHE() checks dragon-ness
  if( CAN_BREATHE(ch) )
  {
    if(DragonCombat(ch, FALSE))
    {
      return;
    }
  }

  // 5% chance to summon an imp.
  if( !number(0, 19) && has_innate(ch, INNATE_SUMMON_IMP) )
  {
    do_summon_imp(ch, 0, 0);
  }

  if( !number(0, 7) && has_innate(ch, INNATE_BLAST)
    && (tch = pick_target(ch, PT_CASTER | PT_TOLERANT)))
  {
    spell_innate_blast(GET_LEVEL(ch), ch, 0, 0, tch, 0);
    // High enough level illithids can innate blast alongside their normal actions.
    if( GET_LEVEL(ch) < 41 || number(0, BOUNDED(0, (60 - GET_LEVEL(ch)) / 3, 6)) )
    {
      return;
    }
  }

  if(!number(0, 5) && has_innate(ch, INNATE_BRANCH)
    && (tch = pick_target(ch, PT_CASTER | PT_TOLERANT | PT_STANDING | PT_SMALLER)))
  {
    branch(ch, tch);
    return;
  }

  if( !number(0, 9) && has_innate(ch, INNATE_STAMPEDE) && !IS_PC_PET(ch) )
  {
    do_stampede(ch, NULL, CMD_STAMPEDE);
    return;
  }

  if( !number(0,5) && has_innate(ch, INNATE_BITE) && (tch = pick_target(ch, PT_WEAKEST)) )
  {
    bite(ch, tch);
    return;
  }

  if( !number(0, 5) && has_innate(ch, INNATE_WEBWRAP)
    && (tch = pick_target(ch, PT_CASTER | PT_TOLERANT | PT_SMALLER)) )
  {
    webwrap(ch, tch);
    return;
  }

  if(IS_BEHOLDER(ch))
  {
    if(BeholderCombat(ch))
    {
      return;
    }
  }

  /* temp till we split them up */

  // combat casting, so go for offensive

  if( (GET_CLASS(ch, CLASS_SORCERER) || GET_CLASS(ch, CLASS_CONJURER)
    || GET_CLASS(ch, CLASS_NECROMANCER) || GET_CLASS(ch, CLASS_BARD)
    || GET_CLASS(ch, CLASS_THEURGIST) || GET_CLASS(ch, CLASS_SUMMONER))
    && (!IS_MULTICLASS_NPC(ch) || number(0, 2)) )
  {
    if( CastMageSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_ILLUSIONIST)
    && (!IS_MULTICLASS_NPC(ch) || number(0, 2)) )
  {
    if( CastIllusionistSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_PSIONICIST | CLASS_MINDFLAYER)
    && (!IS_MULTICLASS_NPC(ch) || number(0, 2)) )
  {
    if( WillPsionicistSpell(ch, 0) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_SHAMAN) && (!IS_MULTICLASS_NPC(ch) || number(0, 2)) )
  {
    if( CastShamanSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_ETHERMANCER) && (!IS_MULTICLASS_NPC(ch) || number(0, 2)) )
  {
    if( CastEtherSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_DRUID) && (!IS_MULTICLASS_NPC(ch) || number(0, 1)) )
  {
    if( CastDruidSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_BLIGHTER) && (!IS_MULTICLASS_NPC(ch) || number(0, 1)) )
  {
    if( CastBlighterSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_WARLOCK) && (!IS_MULTICLASS_NPC(ch) || number(0, 1)) )
  {
    if( CastWarlockSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_PALADIN) && (!IS_MULTICLASS_NPC(ch) || number(0, 1)) )
  {
    if( CastPaladinSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_ANTIPALADIN) && (!IS_MULTICLASS_NPC(ch) || number(0, 1)) )
  {
    if( CastAntiPaladinSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( GET_CLASS(ch, CLASS_CLERIC) && (!IS_MULTICLASS_NPC(ch) || number(0, 1)) )
  {
    if( CastClericSpell(ch, 0, FALSE) )
    {
      return;
    }
  }

  if( !ch->specials.fighting )
  {
    return;
  }

  // Lazy evaluation needed here.
  if( GET_CLASS(ch, CLASS_MONK) && MobMonk(ch) )
  {
    return;
  }

  // Lazy evaluation needed here.
  if( GET_CLASS(ch, CLASS_REAVER) && MobReaver(ch) )
  {
    return;
  }

  // Lazy evaluation needed here.
  if( GET_CLASS(ch, CLASS_RANGER) && MobRanger(ch) )
  {
    return;
  }

  // Lazy evaluation needed here.
  if( GET_CLASS(ch, CLASS_ALCHEMIST) && MobAlchemist(ch) )
  {
    return;
  }

  if( GET_CLASS(ch, CLASS_DREADLORD | CLASS_AVENGER) )
  {
    if( MobDreadlord(ch) )
    {
      return;
    }
  }

  // Lazy evaluation needed here.
  if( GET_CLASS(ch, CLASS_BERSERKER) && MobBerserker(ch) )
  {
    return;
  }

  // Lazy evaluation needed here.
  if( GET_CLASS(ch, CLASS_MERCENARY) && MobMercenary(ch) )
  {
    return;
  }

  // Lazy evaluation needed here.
  if( GET_CLASS(ch, CLASS_BARD) && MobBard(ch) )
  {
    return;
  }

  // Lazy evaluation needed here.
  if( IS_WARRIOR(ch) && MobWarrior(ch) )
  {
    return;
  }

  // Lazy evaluation needed here.
  if( IS_THIEF(ch) && MobThief(ch) )
  {
    return;
  }

  return;
}

int CountToughness(P_char ch, P_char victim)
{
  int      val, foo;

  if(IS_PC(ch))
  {
    return -1;
  }
  
  val = GET_HIT(victim);

  if(!GET_CLASS(ch, CLASS_PALADIN) &&
     !GET_CLASS(ch, CLASS_ANTIPALADIN) &&
    (IS_CLERIC(victim) ||
     IS_MAGE(victim)))
  {
    val = val * 2 / 3;
  }
  else if(IS_WARRIOR(victim))
  {
    val = val * 2;
  }
  
  foo = MAX(0, 60 - GET_LEVEL(victim));
  
  if(!IS_FIGHTING(victim))
  {
    val = val / (IS_THIEF(ch) ? 4 : 2);
  }
  if((IS_AFFECTED(victim, AFF_AWARE) ||
      IS_AFFECTED(victim, AFF_SKILL_AWARE)) &&
      IS_THIEF(ch))
  {
    val = (int) (val * 1.5);
  }
  
  val = (int) (val * 100 / number(100 - foo, 100 + foo));
  return val;
}

#define MAX_TARGETS 20

/*
 * used to pick a player to attack, checks all the piddly things that need
 * to be checked, and if it finds a likely target, returns pointer to that
 * ch, else returns NULL. Assumes: ch is agg (at least to some things), and
 * ch can fight.  JAB
 */

P_char PickTarget(P_char ch)
{
  P_char   t_ch;
  int      target_table[MAX_TARGETS + 1];
  P_char   target_addr[MAX_TARGETS + 1];
  int      a, b, c, d, n_a;

  if(!SanityCheck(ch, "PickTarget"))
  {
    return NULL;
  }
  
  if(IS_SET(world[ch->in_room].room_flags, SAFE_ZONE))
  {
    return NULL;
  }
  
  if(IS_STUNNED(ch) ||
     !AWAKE(ch) ||
     IS_IMMOBILE(ch))
  {
    return NULL;
  }
  
  a = 0;
  n_a = NumAttackers(ch);

  for (t_ch = world[ch->in_room].people; t_ch; t_ch = t_ch->next_in_room)
  {
    if(t_ch == ch)
      continue;
/*
 * if(n_a && (!IS_FIGHTING(t_ch) || t_ch->specials.fighting != ch))
 * continue;
 */
    if(!is_aggr_to(ch, t_ch))
    {
      continue;
    }

    // Reflection Kobolds don't get innate calming anymore.
    if( has_innate(t_ch, INNATE_CALMING)
      && number(0, 101) <= get_property("innate.calming.notarget.perc", 75)
      && !(IS_NPC(t_ch) && GET_VNUM(t_ch) == 250) )
    {
      continue;
    }

    //statupdate2013 - Drannak
    if( calmcheck(t_ch) )
    {
      continue;
    }

    if(a < MAX_TARGETS)
    {
      target_table[a] = CountToughness(ch, t_ch);
      target_addr[a] = t_ch;
      target_table[a + 1] = -1;
      target_addr[a + 1] = NULL;
      a++;
    }
    else
    {
      break;
    }
  }

  if(a == 0)
  {
    return NULL;
  }
  
  if(IS_PC(ch))
  {
    return (target_addr[number(0, (a - 1))]);
  }
  
  b = -2;
  c = -1;
  
  for (d = 0; d < a; d++)
  {
    if((target_table[d] < b) || (b == -2))
    {
      c = d;
      b = target_table[d];
    }
  }
  if(c != -1)
  {
    return target_addr[c];
  }

  /*
   * nope, no likely targets in room
   */
  return NULL;
}

/*
 * Use to start combat as nastily as possible, thieves will backstab,
 * warriors will bash, mages, clerics, daemons and dragons will be
 * set_fighting, then have an immediate call to MobCombat, so real likely
 * they will cast/breathe on 1st round.  If all else fails (could be from
 * any number of things), they will fall back on 'hit'. Assumes: vict is
 * legal target.  JAB
 */

void MobStartFight(P_char ch, P_char vict)
{
  char     buf[MAX_INPUT_LENGTH];
  bool     fudge_flag = FALSE;
  P_char   mount;

  if( !ch )
  {
    logit(LOG_EXIT, "MobStartFight called in mobact.c with no ch");
    raise(SIGSEGV);
    return;
  }

  if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
  {
    return;
  }

  if( IS_PC(ch) )
  {
    return;
  }

  if( ch->specials.z_cord != vict->specials.z_cord )
  {
    return;
  }

  if( IS_SET(world[ch->in_room].room_flags, SAFE_ZONE) )
  {
    return;
  }

  if( ch->specials.fighting )
  {
    return;
  }

  if( IS_CASTING(ch) )
  {
    return;
  }

  if( GET_STAT(vict) == STAT_DEAD )
  {
    logit(LOG_DEBUG, "MobStartFight: room %d(%s->%s): called with dead character as vict.",
          world[ch->in_room].number, GET_NAME(ch), GET_NAME(vict));
    return;
  }

  if( mob_index[GET_RNUM(ch)].virtual_number == 19870 )
  {
    if( has_innate(vict, INNATE_ASTRAL_NATIVE) && (GET_LEVEL(vict) > 50) )
    {
      return;
    }
  }

  if( mob_index[GET_RNUM(ch)].virtual_number == 19840 )
  {
    if( has_innate(vict, INNATE_ASTRAL_NATIVE) && (GET_LEVEL(vict) > 40) )
    {
      return;
    }
  }

  if( ch->in_room != vict->in_room || ch->specials.z_cord != vict->specials.z_cord )
  {
    MobRetaliateRange(ch, vict);
    // Added return here since ch may move or start casting or whatever. - Lohrr
    return;
  }

  if((world[ch->in_room].room_flags & SINGLE_FILE) && !AdjacentInRoom(ch, vict))
  {
    fudge_flag = TRUE;
  }

  if( ch && (mount = get_linked_char(ch, LNK_RIDING)) )
  {
    send_to_char("I'm afraid you aren't quite up to mounted combat.\r\n", ch);
    act("$n quickly slides off $N's back.", TRUE, ch, 0, mount, TO_NOTVICT);
    stop_riding(ch);
  }

  if( CAN_BREATHE(ch) && IS_DRAGON(ch) )
  {
    if( fudge_flag )
    {
      // Always roar in single file.
      if( DragonCombat(ch, !number(0, 1)) )
      {
        return;
      }
    }
    else
    {
      if(!IS_FIGHTING(ch) && !IS_DESTROYING(ch))
      {
        // May or may not roar depends on level.
        set_fighting(ch, vict);
      }
      if( vict->in_room == ch->in_room )
      {
        if( DragonCombat(ch, FALSE) )
        {
          return;
        }
      }
    }
  }
  if( !fudge_flag && IS_THIEF(ch)
    && ((ch->equipment[WIELD] && IS_BACKSTABBER(ch->equipment[WIELD]))
    || (ch->equipment[SECONDARY_WEAPON]
    && IS_BACKSTABBER(ch->equipment[SECONDARY_WEAPON])))
    && (GET_ALT_SIZE(ch) <= (GET_ALT_SIZE(vict) + 1))
    && ((GET_ALT_SIZE(ch) + 1) >= GET_ALT_SIZE(vict)) )
  {
    backstab(ch, vict);
    return;
  }
  if( !fudge_flag && GET_CLASS(ch, CLASS_WARRIOR) && has_innate(ch, INNATE_BODYSLAM)
    && GET_POS(vict) == POS_STANDING && get_takedown_size(ch) <= get_takedown_size(vict)+1
    && get_takedown_size(ch) >= get_takedown_size(vict) - 2 && !IS_BACKRANKED(vict)
    && !number(0,3) && HAS_FOOTING(ch))
  {
    bodyslam(ch, vict);
    return;
  }
  if( !fudge_flag && IS_WARRIOR(ch) && !IS_IMMATERIAL(ch) && (GET_CHAR_SKILL(ch, SKILL_BASH) > 0)
    && (GET_POS(ch) == POS_STANDING) && (GET_POS(vict) == POS_STANDING)
    && isBashable(ch, vict) && !number(0, 2) )
  {
    bash(ch, vict);
    // Due to certain cleverness in bash (if not likely to succeed, mob doesn't)
    //   return only if bashed (success/fail matters not). Let's make sure nobody died too.
    if( !IS_ALIVE(ch) || !IS_ALIVE(vict) || ch->specials.fighting )
      return;
  }
  /*
   * ok, either mob has no special abilities, or, none were set off (for
   * whatever reason), so we go to the old standby.
   */

  if( !fudge_flag && (vict->in_room == ch->in_room))
  {
    attack(ch, vict);
  }
}

/*
 * To put it simply, this routine does a couple of things. first, it does
 * check on if eq worn is crappier than some item in inventory. If not,
 * it'll try to store stuff in a container which is in inventory as well.
 * If not possible, it lurks. Solution about this needs to be found,
 * mayhap just cold-bloodedly make it junk stuff? hmm. Else mobs will be
 * fulla eq before too long, and that is _NOT_ how it should be.
 *
 * -Torm
 */

/*
 * Ha, new cleverness _was_ invented: It won't process new items more than
 * once, but the problem of stuff still lurking in the inventory needs to
 * be fixed perhaps one day. -Torm
 */

/*
 * Damn, I'm clever. Now it bypasses some dumbass checks, and loots stuff
 * nastier than ever! CACKLE. :) -Still the same
 */

#define IS_CONTAINER(obj) ((((obj)->type == ITEM_CONTAINER) && !IS_SET((obj)->value[1], CONT_CLOSED)) || ((obj)->type == ITEM_CORPSE))

int RateObject(P_char ch, int a, P_obj obj)
{
  int      manap, damp, hitp, value, tmp;

  value = 0;
  damp = 100;
  manap = 0;
  hitp = 75;

  if(obj == NULL)
    return -1;
  if(IS_MAGE(ch))
    manap += 75;
  if(IS_CLERIC(ch))
    manap += 75;
  if(IS_WARRIOR(ch) || IS_THIEF(ch))
  {
    hitp += 75;
    damp += 50;
  }
  /*
   * Ok, now all should be _at_max_ 150, and at min, 0.
   */

  /* consider special procs, heh */

  if(obj_index[obj->R_num].func.obj)
    value = 1000;

  if(IS_SET(obj->extra_flags, ITEM_NODROP))
    value -= 350;

  if(IS_SET(obj->extra_flags, ITEM_TRANSIENT))
    value -= 500;

  if((obj->type != ITEM_QUIVER) && (obj->type != ITEM_WORN) &&
      (obj->type < ITEM_SCROLL || obj->type > ITEM_FIREWEAPON))
    return value;               /*
                                 * Practically, 'toy' items that are
                                 * nodrop won't be ever used
                                 */
  switch (obj->type)
  {
    /*
     * Mob can presently only value armor, and weapon(s). Other eq's
     * value comes to mob from their +dam/+tohit/+mana
     */
  case ITEM_LIGHT:
    if(CAN_SEE(ch, ch))
      value += 100;
    break;
#if 1
  case ITEM_ARMOR:
    if(GET_AC(ch) > -100)
      value += 5 * obj->value[0];
    break;
#endif
  case ITEM_WORN:
  case ITEM_WEAPON:
    value += (obj->value[1] * obj->value[2]) * damp / 10;
    break;
  default:
    break;
  }
  for (tmp = 0; tmp < MAX_OBJ_AFFECT; tmp++)
  {
    switch (obj->affected[tmp].location)
    {
    case APPLY_DAMROLL:
      value += obj->affected[tmp].modifier * damp / 10;
      break;
    case APPLY_HITROLL:
      value += obj->affected[tmp].modifier * hitp / 10;
      break;
    case APPLY_MANA:
      value += obj->affected[tmp].modifier * manap / 3;
      break;
#if 1
    case APPLY_ARMOR:
      if(GET_AC(ch) > -100)
        value += (0 - obj->affected[tmp].modifier * 5);
      break;
#endif
    case APPLY_STR:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_CON:
      if(GET_LEVEL(ch) <= 50)
        value += obj->affected[tmp].modifier * 5;
      break;

      /*
       * This apply_hit thing rules: street cleaners etc will prefer
       * +hitpoint stuff, other mobs will go for nastier +dam etc.
       * :)
       */
    case APPLY_HIT:
      value += 400 * obj->affected[tmp].modifier / GET_MAX_HIT(ch);
      break;
    case APPLY_SAVING_PARA:
    case APPLY_SAVING_ROD:
    case APPLY_SAVING_FEAR:
    case APPLY_SAVING_BREATH:
    case APPLY_SAVING_SPELL:
      value += (0 - 30) * obj->affected[tmp].modifier;
    default:
      break;
    }
  }
  if(IS_THIEF(ch) && IS_BACKSTABBER(obj))
    value = value * 3 / 2;

  value = value * 100 / number(90, 110);
  return value;
}

int IsBetterObject(P_char ch, P_obj obj, int foo)
{
  int      a, l;
  P_obj    foob = NULL, foo2 = NULL;

  for (a = foo; a < CUR_MAX_WEAR; a++)
    if(CAN_WEAR(obj, equipment_pos_table[a][0]) &&
        can_char_use_item(ch, obj))
      if(ch->equipment[equipment_pos_table[a][2]] != NULL)
      {
        l = equipment_pos_table[a][2];
        foob = ch->equipment[l];
        if((RateObject(ch, a, obj) >= RateObject(ch, a, foob)) &&
            (obj->R_num != foob->R_num) &&
            !IS_SET(foob->extra_flags, ITEM_NODROP))
        {
          act("You stop using $p.", FALSE, ch, foob, 0, TO_CHAR);
          act("$n stops using $p.", TRUE, ch, foob, 0, TO_ROOM);
          foo2 = unequip_char(ch, l);
          obj_to_char(foo2, ch);
          wear(ch, obj, equipment_pos_table[a][1], 1);
          CharWait(ch, 1);
          IsBetterObject(ch, foo2, a + 1);      /*
                                                 * Nifty recursive
                                                 * call to make of
                                                 * use item if it
                                                 * can be used
                                                 * elsewhere
                                                 */
          return 1;
        }
      }
      else if(RateObject(ch, a, obj) >= 0)
        if(wear(ch, obj, equipment_pos_table[a][1], 1))
          return 1;
  return 0;
}

void CheckEqWorthUsing(P_char ch, P_obj obj)
{
  P_obj    ob = NULL, ob2 = NULL;

  if(!obj || !ch)
    return;

  if(IS_ANIMAL(ch) || IS_INSECT(ch))
    return;

  if(obj_index[obj->R_num].virtual_number == 101)      /* Dont wear the helm of ooc! */
    return;

#if 0
  if(IS_SET(obj->extra_flags, ITEM_NODROP))
  {
    /*
     * If a cleric, let's just simulate remove curse here - nothing as
     * complicated the real thing, just remove the flag + continue,
     * reducing the mana as we go.
     */
    if(IS_CLERIC(ch) && (GET_MANA(ch) >= 20))
    {
      GET_MANA(ch) -= 20;
      REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
    }
    return;
  }
#endif
  /*
   * Keep containers around, for our mobs 'collections'..
   * muhahahahahaaa!
   */
  if(obj->type == ITEM_CONTAINER)
  {
    /*
     * Just for sake of .. fun .., mobs archive stuff in containers.
     */
    for (ob = ch->carrying; ob; ob = ob2)
    {
      ob2 = ob->next_content;
      if(!IS_CONTAINER(ob))
        put(ch, ob, obj, 1);
    }
    return;
  }
  if(IS_SET(obj->extra_flags, ITEM_NODROP))
  {
    /*
     * If a cleric, let's just simulate remove curse here - nothing as
     * complicated the real thing, just remove the flag + continue,
     * reducing the mana as we go.
     */
    if(IS_CLERIC(ch) && npc_has_spell_slot(ch, SPELL_REMOVE_CURSE))
      REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
    else
      return;
  }
  if(IsBetterObject(ch, obj, 0))
    return;

  for (ob = ch->carrying; ob; ob = ob2)
  {
    ob2 = ob->next_content;
    if(ob->type == ITEM_CONTAINER)
      if(put(ch, obj, ob, 1))
        return;                 /*
                                 * problem solved, item in bag.
                                 */
  }

  /* horrible, horrible way to handle this..  if you really want the mob
     to use such behavior, flag the item as 'mobs won't take' and have
     the mob drop it */

#if 0
  if(RateObject(ch, 0, obj) <= 200)
    extract_obj(obj, TRUE); // Didn't bag, not worth much, get rid of
#endif
}

int ItemsIn(P_obj obj)
{
  P_obj    ob;
  int      a;

  a = 0;
  for (ob = obj->contains; ob; ob = ob->next_content)
    a++;
  return a;
}

/* Return TRUE if mob shouldn't do anything else this pulse, */
int handle_npc_assist(P_char ch)
{
  P_char   tmp_ch, Victim, foe = NULL;
  char     Gbuf1[MAX_STRING_LENGTH];
  struct follow_type *fol;

  if( IS_NPC(ch) && GET_VNUM(ch) == IMAGE_REFLECTION_VNUM )
  {
    return FALSE;
  }

  /*
   * Expanded NPC assistance added below.  Assumes one NPC following another as
   * sufficient for assistance of attacked NPC by the unattacked follower or
   * leader. - SKB 19 May 1995
   */

  if( (GET_POS(ch) > POS_SITTING) && (ch->following)
    && (ch->in_room == ch->following->in_room) && (IS_NPC(ch->following)
    || (GET_MASTER(ch) && GET_MASTER(ch) != GET_RIDER(ch)))
    && (ch->following->specials.fighting || NumAttackers(ch->following)) )
  {
    if( GET_CHAR_SKILL(ch, SKILL_RESCUE) > 0
      && (NumAttackers(ch) < NumAttackers(ch->following))
      && ((GET_HIT(ch) > GET_HIT(ch->following)) || number(0, 1)) )
    {
      if( IS_NPC(ch->following) || IS_PC_PET(ch) )
      {
        if( IS_UNDEADRACE(ch) )
        {
          if( GET_C_POW(ch->following) > number(1, 500) )// 100 pow ~= 20%
          {
            rescue(ch, ch->following, FALSE);
            return TRUE;
          }
        }
        else
        {
          if( GET_C_CHA(ch->following) > number(1, 500) )// 100 charisma ~= 20%
          {
            rescue(ch, ch->following, FALSE);
            return TRUE;
          }
        }
      }
    }

    if(!ch->specials.fighting)
    {
      if(CAN_SPEAK(ch))
      {
        switch(number(1, 4))
        {
          case 1:
            mobsay(ch, "I must assist my master!");
            break;
          case 2:
            mobsay(ch, "Today is a good day to die!");
            break;
          case 3:
            mobsay(ch, "Prepare to fight me, too!");
            break;
          case 4:
            mobsay(ch, "For glory!");
            break;
          default:
            break;
        }
      }

      foe = ch->following->specials.fighting;

      if(foe && CAN_SEE(ch, foe))
      {
        if(!CAN_SPEAK(ch))
        {
          strcpy(Gbuf1, GET_NAME(foe));
          do_action(ch, Gbuf1, CMD_GROWL);
        }
        MobStartFight(ch, foe);
      }
      else
      {
        do_assist_core(ch, ch->following);
      }
      return TRUE;
    }
  }
  /* check if need to assist followers */

  if((GET_POS(ch) > POS_SITTING) &&
     !IS_FIGHTING(ch) &&
     ch->followers)
  {
    for (fol = ch->followers; fol; fol = fol->next)
    {
      tmp_ch = fol->follower;
      if( IS_FIGHTING(tmp_ch) && (tmp_ch->in_room == ch->in_room)
        && IS_NPC(tmp_ch) && !(IS_MORPH(tmp_ch)) )
      {
        foe = tmp_ch->specials.fighting;
        if(CAN_SEE(ch, foe))
        {
          if(CAN_SPEAK(ch))
          {
            strcpy(Gbuf1, GET_NAME(foe));
            do_action(ch, Gbuf1, CMD_SNEER);
          }
          MobStartFight(ch, foe);
          return TRUE;
        }
      }
    }
  }


  if( CAN_ACT(ch) && GET_POS(ch) > POS_SITTING
    && IS_SET(ch->specials.act, ACT_PROTECTOR)
    && GET_STAT(ch) > STAT_SLEEPING && !IS_STUNNED(ch) )
  {
    if( (Victim = find_protector_target(ch)) != NULL )
    {
      foe = Victim->specials.fighting;
    }

    if( Victim && foe && CAN_SEE(ch, foe) )
    {
      act("You assist $N heroically.", FALSE, ch, 0, Victim, TO_CHAR);
      act("$n assists you heroically.", FALSE, ch, 0, Victim, TO_VICT);
      act("$n assists $N heroically.", FALSE, ch, 0, Victim, TO_NOTVICT);
      MobStartFight(ch, foe);
      if(char_in_list(ch))
        CharWait(ch, PULSE_VIOLENCE);
      return TRUE;
    }

    if(!IS_FIGHTING(ch))
    {
      switch (number(1, 10))
      {
      case 1:
        act("$n watches the battle in amusement.", TRUE, ch, 0, 0, TO_ROOM);
        break;

      case 2:
        act("$n stands to the side, evaluating the fray.", TRUE, ch, 0, 0,
            TO_ROOM);
        break;

      case 3:
        act("$n evaluates the struggle, watching your battle tactics.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;

      case 4:
        act("$n frowns in boredom at the obviously one-sided struggle.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;

      case 5:
        act
          ("$n watches the area, making sure the battle does not get out of hand.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        act
          ("$n is waving a platinum coin around wanting to make a bet on the outcome.",
           TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return false;
}

bool MobSpellUp(P_char ch)
{
    bool is_multiclass = IS_MULTICLASS_NPC(ch); // about 15% are multiclass

    // If they're a mage, and not multi or a multi mage and 2/3 of the time.
    if(GET_CLASS(ch, CLASS_SORCERER | CLASS_CONJURER | CLASS_NECROMANCER | CLASS_THEURGIST | CLASS_SUMMONER) && (is_multiclass ? number(0, 2) : 1))
    {
      // If they start a mage spell
      if(CastMageSpell(ch, ch, FALSE))
        return TRUE;
      // If they're not a multiclass
      if( !is_multiclass )
        return FALSE;
    }
    if( GET_CLASS(ch, CLASS_CLERIC) && (is_multiclass ? number(0, 2) : 1) )
    {
      if(CastClericSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_SHAMAN) && (is_multiclass ? number(0, 2) : 1))
    {
      if(CastShamanSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_PSIONICIST | CLASS_MINDFLAYER) && (is_multiclass ? number(0, 2) : 1))
    {
      if(WillPsionicistSpell(ch, ch))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_WARLOCK) && (is_multiclass ? number(0, 1) : 1))
    {
      if(CastWarlockSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_ANTIPALADIN) && (is_multiclass ? number(0, 1) : 1))
    {
      if(CastAntiPaladinSpell(ch, ch, FALSE))
        return TRUE;
      if(CastMageSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_RANGER) && (is_multiclass ? !number(0, 3) : !number(0, 1)))
    {
      if((GET_ALIGNMENT(ch) >= 350) && CastRangerSpell(ch, ch, FALSE))
        return TRUE;
      if(CastMageSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_DRUID) && (is_multiclass ? number(0, 1) : 1))
    {
      if(CastDruidSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_BLIGHTER) && (is_multiclass ? number(0, 1) : 1))
    {
      if(CastBlighterSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_PALADIN) && (is_multiclass ? number(0, 1) : 1))
    {
      if(CastPaladinSpell(ch, ch, FALSE) )
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_ILLUSIONIST) && (is_multiclass ? number(0, 2) : 1))
    {
      if(CastIllusionistSpell(ch, ch, FALSE))
        return TRUE;
      if(CastMageSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass) return FALSE;
    }
    if(GET_CLASS(ch, CLASS_ETHERMANCER) && (is_multiclass ? number(0, 2) : 1))
    {
      if(CastEtherSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_REAVER) && (is_multiclass ? !number(0, 3) : !number(0, 1)))
    {
      if(CastReaverSpell(ch, ch, FALSE))
        return TRUE;
      if(CastMageSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_ALCHEMIST) && (is_multiclass ? !number(0, 3) : !number(0, 1)))
    {
      if(MobAlchemist(ch))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_BERSERKER) && (is_multiclass ? !number(0, 3) : !number(0, 1)))
    {
      if(MobBerserker(ch))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_BARD) && (is_multiclass ? !number(0, 3) : !number(0, 1)))
    {
      if(MobBard(ch))
        return TRUE;
      if(CastMageSpell(ch, ch, FALSE))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    if(GET_CLASS(ch, CLASS_MONK) && (is_multiclass ? !number(0, 3) : !number(0, 1)))
    {
      if(MobMonk(ch))
        return TRUE;
      if (!is_multiclass)
        return FALSE;
    }
    return FALSE;
}



/*
 * this routine is called only by Events(), approx. once each PULSE_MOBILE
 * for every mob that might do something interesting.  ONLY put
 * PULSE_MOBILE routines in here, for generic combat stuff, there is now
 * MobCombat() for routines that are executed every PULSE_VIOLENCE (every
 * round).  The only exception, is new generic routines to initiate combat,
 * since MobCombat checks only the combat list.  Addendum, MobStartFight is
 * probably the place to add new combat initiation routines.  Returns TRUE
 * if a new event of same type should be scheduled.  -JAB
 */


/*  THIS FUNCTION IS THE HEART AND BOTTLENECK OF THE WHOLE GAME!
    MORE THAN HALF OF THE TIME IS SPENT HERE!
    IF YOU EVER ADD ANYTHING HERE, MAKE IT AS EFFECTIVE AS POSSIBLE!
    - No useless checks or macro's that include useless checks
    - No searches in lists if its possible to add direct pointer or pre-flag the mob
    - No sending messages to mobs
    - Combine checks when its possible
    - Always put the check that cuts off the most cases, first
    - Invent the check that will cut off the majority of your cases and put it around your code/call

        Odorf */


void event_mob_mundane(P_char ch, P_char victim, P_obj object, void *data)
{
  P_char   tmp_ch;
  P_obj    obj, obj2, best_obj, next_obj;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      door, moved, max, i;
  struct follow_type *fol, *k;
  bool CombatInRoom;
  int rnum;

  if( !IS_ALIVE(ch) || IS_PC(ch) )
  {
    return;
  }

  if( TRUSTED_NPC(ch) )
  {
    goto normal; // 0%
  }

  if( ch->in_room == NOWHERE )
  {
    char_to_room(ch, 0, -2);
    return;
  }

  if(!ch->only.npc)
  {
    wizlog(IMMORTAL, "PANIC! NewMobAct called with an invalid npc! #%d",
           mob_index[GET_RNUM(ch)].virtual_number);
    logit(LOG_DEBUG, "NewMobAct called with an invalid npc! #%d",
          mob_index[GET_RNUM(ch)].virtual_number);
    return;
  }
//  if(get_linked_char(ch, LNK_RIDING))
//    check_valid_ride(ch);

  /*
   * each mob that exists will have either an EVENT_MOB_MUNDANE or an
   * EVENT_MOB_SPECIAL event at it's birth, the only difference as far
   * as this routine is concerned, is mobs with EVENT_MOB_SPECIAL will
   * check the special first.
   */

  if(!CAN_ACT(ch) || IS_SET(ch->specials.affected_by2, AFF2_MAJOR_PARALYSIS | AFF2_MINOR_PARALYSIS | AFF2_CASTING))
  {// 0.01%
    /*
     * check bit more oft for acting if silly bugger cannot do shit
     * now
     */
    goto quick;
  }


PROFILE_START(mundane_quest);
  if(mob_index[ch->only.npc->R_num].qst_func && !IS_FIGHTING(ch))
  {
    // works in about 5% cases, but extremely ineffective!
    // returns true only in about 1/15 cases
    // TODO: replace binary search inside with direct pointer and extract number of pulses directly instead of sscanf!
    if (execute_quest_routine(ch, CMD_NONE))
    {
      goto normal;
    }
  }
PROFILE_END(mundane_quest);

  moved = FALSE;

#if 0  // since the body is commented out, commenting out the whole thing -Odorf 
  /* Remount if needed */
  if(ch->followers && !IS_FIGHTING(ch) && !IS_RIDING(ch))
  {
    for (fol = ch->followers; fol; fol = fol->next)
    {
      if(IS_SET(fol->follower->specials.act, ACT_MOUNT))
      {
//  link_char(ch, fol->follower, LNK_RIDING);
      }
    }
  }
#endif

PROFILE_START(mundane_autoinvis);
  if(GET_RACE(ch) == RACE_A_ELEMENTAL || IS_WRAITH(ch) || IS_BRALANI(ch))
  { // 2%
    if(!IS_SET(ch->specials.affected_by, AFF_INVISIBLE) && !IS_FIGHTING(ch) && !IS_CASTING(ch))
    {
       act("$n fades from your mortal viewing...", TRUE, ch, 0, 0, TO_ROOM);
       SET_BIT(ch->specials.affected_by, AFF_INVISIBLE);
    }
  }
PROFILE_END(mundane_autoinvis);

  /* If we are a vehicle navigator, lets get moving */
  // TODO: add the corresponding flag to the mob and check it here before running through the list  -Odorf
PROFILE_START(mundane_wagon);
  check_for_wagon(ch);
PROFILE_END(mundane_wagon);

  /*
   * quickie check, for sounds of combat in room waking 'normal'
   * sleepers.  JAB
   */
  // TODO: make it dependent on room fighting flag -Odorf
PROFILE_START(mundane_wakeup);
  if((GET_STAT(ch) == STAT_SLEEPING) && !ALONE(ch) &&
      /*(ch->in_room != NOWHERE) && - included in ALONE macro -Odorf */
      !IS_SET(world[ch->in_room].room_flags, ROOM_SILENT) &&
      !IS_AFFECTED(ch, AFF_SLEEP) && !IS_AFFECTED(ch, AFF_KNOCKED_OUT))
  { // 1.3%
    LOOP_THRU_PEOPLE(tmp_ch, ch)
    { // 5.4%
      if((ch != tmp_ch) && IS_FIGHTING(tmp_ch) && (number(0, 120) <= GET_LEVEL(ch)))
      {
        do_wake(ch, 0, -4);
        send_to_char("Who can sleep with all this noise?\r\n", ch);
PROFILE_END(mundane_wakeup);
        goto normal;
      }
    }
  }
PROFILE_END(mundane_wakeup);


PROFILE_START(mundane_justice);
  if(JusticeGuardAct(ch))  // Justice hook.
  {// 0%
PROFILE_END(mundane_justice);
    goto normal;
  }
PROFILE_END(mundane_justice);

PROFILE_START(mundane_commune);
  if(!IS_FIGHTING(ch) && !IS_DESTROYING(ch) && !number(0, 20)) // If not fighting, "mem"
  {// 5%
    // made a separate procedure for mobs, w/o sprintfs and all  -Odorf
    do_npc_commune(ch);
  }
PROFILE_END(mundane_commune);

  /*
   * mob is sitting because either a god forced em to, or because of a
   * battle maneuver.  if it's unnatural for the mob to be sitting,
   * don't continue with activity. --TAM
   */
PROFILE_START(mundane_autostand);
  if(GET_POS(ch) < POS_STANDING)
  {// 3%
    if(IS_FIGHTING(ch) || IS_DESTROYING(ch))
    {
      do_stand(ch, 0, 0);
PROFILE_END(mundane_autostand);
      goto normal;
    }
    else if((ch->only.npc->default_pos & 3) != GET_POS(ch))
    {
      do_stand(ch, 0, 0);       /* if not, opponent fled. have em stand up.  */
    }
  }
PROFILE_END(mundane_autostand);

  /* Examine call for special procedure */
/* We now use a different event event_mob_proc to call mob procs.
PROFILE_START(mundane_specproc);
  if(IS_SET(ch->specials.act, ACT_SPEC) && !no_specials )
  { // 8%
    if(!mob_index[ch->only.npc->R_num].func.mob)
    { // 0%
      logit(LOG_MOB, "SPEC set, but no proc: %s #%d",
          ch->player.name, mob_index[ch->only.npc->R_num].virtual_number);
      REMOVE_BIT(ch->specials.act, ACT_SPEC);
    }
    else if((*mob_index[ch->only.npc->R_num].func.mob) (ch, 0, CMD_MOB_MUNDANE, 0))
    { // 0.1%
PROFILE_END(mundane_specproc);
      goto normal;
    }
  }
PROFILE_END(mundane_specproc);
*/
  if(!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  { // 3%
    goto normal;                /* not much we can do, if they can't stand */
  }

PROFILE_START(mundane_mobcast);
  // If they're not a sentinel in a safe zone? and they're not a pet with master in room.
  if( !(IS_SET(ch->specials.act, ACT_SENTINEL) && CHAR_IN_SAFE_ZONE(ch))
    && !(IS_PC_PET(ch) && (ch->in_room == GET_MASTER(ch)->in_room)) )
  { // 100%
    if (GET_CLASS(ch,
        CLASS_CLERIC | CLASS_SHAMAN | CLASS_ETHERMANCER | CLASS_DRUID |
        CLASS_SORCERER | CLASS_CONJURER | CLASS_NECROMANCER |
        CLASS_THEURGIST | CLASS_ILLUSIONIST | CLASS_WARLOCK |
        CLASS_PSIONICIST | CLASS_MINDFLAYER | CLASS_SUMMONER |
        CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_BLIGHTER |
        CLASS_RANGER | CLASS_REAVER |
        CLASS_ALCHEMIST | CLASS_BARD |
        CLASS_BERSERKER | CLASS_MONK))
    {
        // If they start to spellup, goto normal
        if (MobSpellUp(ch))
        {
PROFILE_END(mundane_mobcast);
            goto normal;
        }
    }
  }
PROFILE_END(mundane_mobcast);

PROFILE_START(mundane_track);
  if(IS_SET(ch->specials.act, ACT_MEMORY) && !IS_FIGHTING(ch))
  { // 85%
    if(IS_SET(ch->specials.act, ACT_HUNTER)) 
    { // 38%
      if ( ch->only.npc != NULL && GET_MEMORY(ch) != NULL) // guardcheck (no hunt will happen if mob has no memory)  -Odorf
      {
PROFILE_START(mundane_track_1);
        if(InitNewMobHunt(ch))
        {
PROFILE_END(mundane_track_1);
PROFILE_END(mundane_track);
          goto normal;
        }
PROFILE_END(mundane_track_1);
      }
      else
      {
PROFILE_START(mundane_track_2);
        if (TryToGetHome(ch))  // if mob has no memory it might still want to go back  -Odorf
        {
PROFILE_END(mundane_track_1);
PROFILE_END(mundane_track);
          goto normal;
        }
PROFILE_END(mundane_track_2);
      }
    }

    if (GET_MEMORY(ch) != NULL) // guardcheck (no real action will happen in either function if mob has no memory) -Odorf
    {
PROFILE_START(mundane_track_3);
      if( CheckForRemember(ch) )
PROFILE_END(mundane_track_3);
//      if(/*!ch - no need -Odorf ||*/ !CAN_ACT(ch)) //  do we really need to check CAN_ACT here? -Odorf
      {
PROFILE_END(mundane_track);
        goto normal;
      }

PROFILE_START(mundane_track_4);
      mobact_memoryHandle(ch);
PROFILE_END(mundane_track_4);
      if(/*!ch - no need -Odorf ||*/ !CAN_ACT(ch)) //  do we really need to check CAN_ACT here? -Odorf
      {
PROFILE_END(mundane_track);
        goto normal;
      }
    }
  }
PROFILE_END(mundane_track);
  /* check for mobs that can break charm - DCL */

PROFILE_START(mundane_charmbreak);
  if(IS_AFFECTED(ch, AFF_CHARM) && IS_SET(ch->specials.act, ACT_BREAK_CHARM)
      && ((!number(0, 3) && NewSaves(ch, SAVING_PARA, 0)
      && ch->only.npc->R_num != real_mobile(6)) || IS_SHOPKEEPER(ch)))
  { // 0%
    clear_links(ch, LNK_PET);
    if(ch->following && CAN_SEE(ch, ch->following))
    {
      tmp_ch = ch->following;

      stop_follower(ch);

      if(HAS_MEMORY(ch))
        remember(ch, tmp_ch);
      if(ch->in_room == tmp_ch->in_room)
      {
        if(IS_HUMANOID(ch))
        {
          sprintf(Gbuf1, "point %s", GET_NAME(tmp_ch));
          command_interpreter(ch, Gbuf1);
          act
            ("\"You charmed me against my will.  Now you will pay!\" $n growls.",
             FALSE, ch, 0, 0, TO_ROOM);
        }
        MobStartFight(ch, tmp_ch);
        goto normal;
PROFILE_END(mundane_charmbreak);
      }
    }
  }
PROFILE_END(mundane_charmbreak);
  /*
   * new to cure poison
   */

PROFILE_START(mundane_curepoison);
  if(IS_AFFECTED2(ch, AFF2_POISONED))
  {
    if(!IS_FIGHTING(ch) && npc_has_spell_slot(ch, SPELL_REMOVE_POISON))
    {
      if(number(1, 101) > 90)
      {
        MobCastSpell(ch, ch, 0, SPELL_REMOVE_POISON, GET_LEVEL(ch));
PROFILE_END(mundane_curepoison);
        goto normal;
      }
      else
        do_action(ch, 0, CMD_CURSE);
    }
PROFILE_END(mundane_curepoison);
/* sadly, dispel magic no longer works on self, so this code doesn't work */

/*
   if(knows_spell(ch, SPELL_DISPEL_MAGIC) && npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC))
   {
   if(number(1, 101) > 70) {
   MobCastSpell(ch, ch, 0, SPELL_DISPEL_MAGIC, GET_LEVEL(ch));
   goto normal;
   } else
   do_action(ch, 0, CMD_CURSE);
   }
 */
  }
  /* remove blocking walls */
  // annoying, but not much to do here. Maybe should add a room flag WALLED?  -Odorf
PROFILE_START(mundane_wallbreak);
  if(!IS_PATROL(ch))
  { // 99.95%
    for (i = 0; i < NUM_EXITS; i++)
    { // x10
      // there's a wall!
      if(EXIT(ch, i) && IS_WALLED(ch->in_room, i))
      {
        if(MobDestroyWall(ch, i))
        {
PROFILE_END(mundane_wallbreak);
          goto normal;
        }
      }
    }
  }
PROFILE_END(mundane_wallbreak);
#if 0
  if(IS_SET(world[ch->in_room].room_flags, MAGIC_DARK))
  {
    /* ok, we're in a magically dark room, escape whatever way we can! */
    if(!IS_FIGHTING(ch))
    {
      if(knows_spell(ch, SPELL_CONTINUAL_LIGHT) &&
          npc_has_spell_slot(ch, SPELL_CONTINUAL_LIGHT))
      {
        MobCastSpell(ch, ch, 0, SPELL_CONTINUAL_LIGHT, GET_LEVEL(ch));
        goto normal;
      }
      if(!IS_SET(ch->specials.act, ACT_SENTINEL) &&
          !IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) &&
          knows_spell(ch, SPELL_TELEPORT) &&
          npc_has_spell_slot(ch, SPELL_TELEPORT))
      {
        MobCastSpell(ch, ch, 0, SPELL_TELEPORT, GET_LEVEL(ch));
        goto normal;
      }
    }
    else
    {
      if(!IS_SET(ch->specials.act, ACT_SENTINEL))
      {
        if(room_has_valid_exit(ch->in_room))
        {
          do_flee(ch, 0, 0);
          goto normal;
        }
      }
    }
  }
#endif
  /*
   * check for agg mobs attacking, this should only happen under a few
   * circumstances: mob woke up after target entered room. mob just
   * killed someone, now it goes after someone else. char became visible
   * after entering room (which includes sneaking in and still being
   * there at MOB_PULSE), normally agg mobs attack people entering room
   * (or when they enter room).  many things can prevent these initial
   * attacks.  JAB
   */

PROFILE_START(mundane_picktarget);
  if((GET_POS(ch) > POS_SITTING) && !IS_FIGHTING(ch) && !ALONE(ch) && (tmp_ch = PickTarget(ch)))
  {
      int calming = 0;
      int nocalming = 0;
      if ((RACE_EVIL(tmp_ch) && IS_SET(hometowns[VNUM2TOWN(world[tmp_ch->in_room].number)-1].flags, JUSTICE_GOODHOME)) ||
          (RACE_GOOD(tmp_ch) && IS_SET(hometowns[VNUM2TOWN(world[tmp_ch->in_room].number)-1].flags, JUSTICE_EVILHOME)))
      nocalming = 1;

      // Alright, if !elite ch and !nocalming.. check for calming:
      //   if ch's level <= target's level - 5 or 25% hit,
      //   and target has innate calming and isn't a reflection/image.
      if( !IS_ELITE(ch) && !nocalming
        && ( (GET_LEVEL(ch) - GET_LEVEL(tmp_ch) <= 5) || !number(0, 3) )
        && (has_innate(tmp_ch, INNATE_CALMING) && !(IS_NPC(tmp_ch) && GET_VNUM(tmp_ch) == 250)) )
      {
        calming = (int)get_property("innate.calming.delay", 10);
      }

      // Statupdate2013 - Drannak
      if( calmcheck(ch) )
      {
        int calmroll = CALMCHANCE(ch) / 2;
        calming = BOUNDED(1, calmroll, 20);
      }

    add_event(event_agg_attack, 1 + calming, ch, tmp_ch, 0, 0, 0, 0);
PROFILE_END(mundane_picktarget);
    goto normal;
  }
PROFILE_END(mundane_picktarget);
  /*
   * this is the 'switch_to_attacking_master_instead' bit, moved here
   * from damage() JAB
   */

PROFILE_START(mundane_attack);
  if(IS_FIGHTING(ch) &&
      MIN_POS(ch, POS_STANDING + STAT_RESTING) &&
      !IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) &&
      !IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
  { // 0%
    tmp_ch = ch->specials.fighting;
    if(IS_NPC(tmp_ch) && GET_MASTER(tmp_ch) &&
        (GET_MASTER(tmp_ch)->in_room == ch->in_room) &&
        CAN_SEE(ch, GET_MASTER(tmp_ch)) && StatSave(ch, APPLY_INT, 0))
    {

      /* * switch targets (if we can) */
      attack(ch, tmp_ch);
PROFILE_END(mundane_attack);
      goto normal;
    }
  }
PROFILE_END(mundane_attack);
  /*
   * ok, quick scan through room, just to see if combat is going on
   * there.
   */

  CombatInRoom = FALSE;

PROFILE_START(mundane_assist);
  LOOP_THRU_PEOPLE(tmp_ch, ch)
  { // 400%
    if(IS_FIGHTING(tmp_ch))
    {
      CombatInRoom = TRUE;
      break;
    }
  }
  /*
   * check for battle noise next door - DCL
   */

  if(IS_SET(ch->specials.act2, ACT2_COMBAT_NEARBY) && !CombatInRoom &&
      !IS_SET(ch->specials.act, ACT_SENTINEL) && !IS_PATROL(ch) &&
      IS_SET(ch->specials.act, ACT_PROTECTOR) && ((door = number(0, 10)) < 6)
      && (ch->in_room != NOWHERE) &&
      !IS_SET(world[ch->in_room].room_flags, ROOM_SILENT) &&
      !IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT) &&
      (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
  {
    for (door = 0; door < NUM_EXITS; door++)
    {
      if(CAN_GO(ch, door) && !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB) &&
        (world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone) &&
        !IS_SET(zone_table[world[EXIT(ch, door)->to_room].zone].flags, ZONE_SILENT) &&
        !IS_SET(world[EXIT(ch, door)->to_room].room_flags, ROOM_SILENT
        && (EXIT(ch, door)->to_room != NOWHERE) )
        /* &&!IS_SET(world[EXIT(ch, door)->to_room].room_flags, MAGIC_DARK) */)
      {
        P_char next;
        for (tmp_ch = world[EXIT(ch, door)->to_room].people; tmp_ch;
             tmp_ch = next)
        {
          next = tmp_ch->next_in_room;

          if(tmp_ch->specials.fighting)
          {
            ch->only.npc->last_direction = door;
            do_move(ch, 0, exitnumb_to_cmd(door));
            REMOVE_BIT(ch->specials.act2, ACT2_COMBAT_NEARBY );
PROFILE_END(mundane_assist);
            goto normal;
          }
        }
      }
    }
  }
  REMOVE_BIT(ch->specials.act2, ACT2_COMBAT_NEARBY);

  if(CombatInRoom)
    handle_npc_assist(ch);
PROFILE_END(mundane_assist);

  if(IS_HUMANOID(ch) && !IS_PATROL(ch))
  {
    for (obj = world[ch->in_room].contents; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if(IS_SET(obj->extra_flags, ITEM_SECRET))
        continue;
      if(obj->type == ITEM_MONEY && CAN_CARRY_W(ch) >= GET_OBJ_WEIGHT(obj))
      {
        strcpy(Gbuf1, "coins");
        do_get(ch, Gbuf1, CMD_GET);
      }
    }
  }

  if(IS_SET(ch->specials.act, ACT_SCAVENGER) && world[ch->in_room].contents
      && (!number(0, MAX(1, 15 - STAT_INDEX(GET_C_INT(ch))))) &&
      !IS_FIGHTING(ch) && !IS_ANIMAL(ch))
  {
    max = 1;
    best_obj = NULL;
    for (obj = world[ch->in_room].contents; obj; obj = next_obj)
    {
      next_obj = obj->next_content;  // THIS LINE IS WHERE CRASH OCCURS

      //
      // priority: containers with most stuff, and then eq from
      // ground, in order of value.
      //

      if( IS_SET(obj->extra_flags, ITEM_SECRET)
        || IS_SET(obj->extra_flags, ITEM_NOSHOW)
        || IS_SET(obj->extra_flags, ITEM_BURIED)
        || !CAN_WEAR(obj, ITEM_TAKE) )
        continue;

      if(CAN_GET_OBJ(ch, obj) || IS_CONTAINER(obj))
        if((CAN_CARRY_W(ch) >= GET_OBJ_WEIGHT(obj)) || IS_CONTAINER(obj))
          if(IS_CONTAINER(obj) && ItemsIn(obj) >= 1)
          {
            best_obj = obj;
            max = 0 - ItemsIn(obj);
          }
          else if((!IS_CONTAINER(obj) || CAN_GET_OBJ(ch, obj)) &&
                   (obj->weight <= 120) &&
                   ((obj->cost * 100 / number(75, 125)) > max) &&
                   (RateObject(ch, 0, obj) >= 0))
          {
            best_obj = obj;
            max = obj->cost;
          }
    }

    if(best_obj)
    {
      if(IS_CONTAINER(best_obj) && (max < 0))
      {
        if(best_obj->type == ITEM_CORPSE)
        {
          struct obj_affect *af;
          af = get_obj_affect(best_obj, TAG_OBJ_DECAY);
          if(af && (obj_affect_time(best_obj, af) > 2550))
            goto normal;
        }
//      act("$n examines $p.", FALSE, ch, best_obj, 0, TO_ROOM);
        for (obj = best_obj->contains;
             obj && (CAN_CARRY_N(ch) > IS_CARRYING_N(ch)); obj = obj2)
        {
          obj2 = obj->next_content;
          if( IS_SET(obj->extra_flags, ITEM_SECRET)
            || IS_SET(obj->extra_flags, ITEM_NOSHOW)
            || IS_SET(obj->extra_flags, ITEM_BURIED)
            || !CAN_WEAR(obj, ITEM_TAKE) )
            continue;
          if(obj->type == ITEM_MONEY)
          {
            get(ch, obj, best_obj, FALSE);
            act("$n gets some stuff from $p.", FALSE, ch, best_obj, 0,
                TO_ROOM);
            continue;
          }
          else
          {
            get(ch, obj, best_obj, FALSE);
            if(!OBJ_CARRIED_BY(obj, ch))
              continue;         // obj is notake, or too heavy
            act("$n gets some stuff from $p.", FALSE, ch, best_obj, 0,
                TO_ROOM);
            if(!IS_ANIMAL(ch) && !IS_DRAGON(ch) && !IS_UNDEAD(ch))
              CheckEqWorthUsing(ch, obj);
          }
        }
        goto normal;
      }
      else if( !IS_SET(best_obj->extra_flags, ITEM_SECRET)
             && !IS_SET(best_obj->extra_flags, ITEM_BURIED)
             && !IS_SET(best_obj->extra_flags, ITEM_NOSHOW)
             && CAN_WEAR(best_obj, ITEM_TAKE) )
      {
        if(OBJ_ROOM(best_obj))
          obj_from_room(best_obj);
        else
        {
          logit(LOG_DEBUG, "best_obj not in room for mob scav");
          goto normal;
        }

        obj_to_char(best_obj, ch);
        act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
        CheckEqWorthUsing(ch, best_obj);
        goto normal;
      }
    }
  } 
  
  if( !IS_ALIVE(ch) )
  {
    return;
  }
  /* random wanderings */

PROFILE_START(mundane_wander);
  if(!IS_FIGHTING(ch) && !IS_PATROL(ch) && !GET_MASTER(ch) &&
      !get_linking_char(ch, LNK_RIDING) && should_teacher_move(ch))
  { // 96%
    if((!IS_SET(ch->specials.act, ACT_SENTINEL) ||
         (IS_SET(world[ch->in_room].room_flags, SAFE_ZONE) &&
          IS_AGGRESSIVE(ch) &&
          GET_HIT(ch) > GET_MAX_HIT(ch) / 2)) &&
        (MIN_POS(ch, POS_STANDING + STAT_RESTING)) &&
        ((door = number(0, NUM_EXITS)) < NUM_EXITS) && CAN_GO(ch, door) &&
        !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB) &&
        world[EXIT(ch, door)->to_room].sector_type != SECT_NO_GROUND)
    { // 13%
      if(ch->only.npc->last_direction == door)
      {
        ch->only.npc->last_direction = -1;
      }
      else
      { // 11%
        if(!IS_SET(ch->specials.act, ACT_STAY_ZONE) ||
            (world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone))
        { // 11%
          /* Couldn't move, so let's try again fairly soon */
          if(IS_RANDOM_MOB(ch) &&
              (world[EXIT(ch, door)->to_room].sector_type !=
               world[ch->in_room].sector_type))
          { // 0.0001%
PROFILE_END(mundane_wander);
            goto quick;
          }
          ch->only.npc->last_direction = door;
          do_move(ch, 0, exitnumb_to_cmd(door));

          /* if mob died while moving (e.g., died through lightning curtain) just cancel and return */
          if( !IS_ALIVE(ch) )
          {
PROFILE_END(mundane_wander);
            return;
          }

          /* Road wandering mobs move faster */
          if(IS_RANDOM_MOB(ch) && EXIT(ch, door) &&
              (world[EXIT(ch, door)->to_room].sector_type == SECT_CITY))
          { // 0%
PROFILE_END(mundane_wander);
            goto quick;
          }
PROFILE_END(mundane_wander);
          goto normal;
        }
      }
    }
  }
  // 84.2%
PROFILE_END(mundane_wander);

normal:  // 99.999%
PROFILE_START(mundane_newevent);
  if (remember_array[world[ch->in_room].zone])
    add_event(event_mob_mundane, PULSE_MOBILE + number(-4,4), ch, 0, 0, 0, 0, 0);
  else
    add_event(event_mob_mundane, PULSE_MOBILE * PLAYERLESS_ZONE_SPEED_MODIFIER + number(-4,4), ch, 0, 0, 0, 0, 0);
PROFILE_END(mundane_newevent);
  return;

quick: // 0.001%
PROFILE_START(mundane_newevent);
  add_event(event_mob_mundane, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
PROFILE_END(mundane_newevent);
  return;
}

bool MobDestroyWall(P_char ch, int dir, bool bTryHit)
{
  // find which wall in ch->in_room is blocking dir, and forward to
  // MobDestroyWall(ch, wall)
  if(!ch || (ch->in_room == NOWHERE) ||
      !EXIT(ch, dir) || !IS_WALLED(ch->in_room, dir) || !CAN_ACT(ch))
    return false;

  if(!bTryHit && !npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) &&
      !IS_PATROL(ch))
    return false;

  P_obj obj, next_obj;
  for (obj = world[ch->in_room].contents; obj; obj = next_obj)
  {
    next_obj = obj->next_content;
    if((obj_index[obj->R_num].virtual_number == VOBJ_WALLS) &&
        (obj->value[1] == dir))
    {
      // try to destroy it
      return MobDestroyWall(ch, obj, bTryHit);
    }
  }
  return false;
}


bool MobDestroyWall(P_char ch, P_obj wall, bool bTryHit)
{
  bool bIsSecret = false;
  bool bImpossible = false;

  if(!ch)
  {
    logit(LOG_EXIT, "MobDestroyWall called in mobact.c with null ch");
    raise(SIGSEGV);;
  }
  if((ch->following) && ((CAN_SEE(ch, ch->following))))
  {
    return false;
  }
  if(wall->value[5] == GET_RNUM(ch))
  {
    return false;
  }
  if(IS_SET(wall->extra_flags, ITEM_SECRET))
  {
    bIsSecret = true;
    REMOVE_BIT(wall->extra_flags, ITEM_SECRET);
  }
  if(!CAN_SEE_OBJ(ch, wall))
  {
    bImpossible = true;
  }
  if(bIsSecret)
  {
    SET_BIT(wall->extra_flags, ITEM_SECRET);
  }
  if(!bImpossible)
  {
    if(bIsSecret)
    {
      do_search(ch, "", CMD_SEARCH);
    }
    else if(npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) &&
           !IS_FIGHTING(ch))
    {
      MobCastSpell(ch, 0, wall, SPELL_DISPEL_MAGIC, GET_LEVEL(ch));
    }
    else if(bTryHit && !IS_FIGHTING(ch) && !IS_DESTROYING(ch))
    {
      char cmdBuf[500];
      sprintf(cmdBuf, "wall %s", dirs[wall->value[1]]);
      // call the object proc (if any) with the cmd.  Calling the object proc
      // directly allows us to get the return value to see if the proc cared
      // about the cmd.
      if(obj_index[wall->R_num].func.obj)
      {
        bImpossible = !((*obj_index[wall->R_num].func.obj) (wall, ch, CMD_HIT, cmdBuf));
        // special for PATROLS - if the wall isn't hittable, then
        // use a special dispel magic
        if(bImpossible && IS_PATROL(ch))
        {
          act("$n utters a word of power while pointing at $p.", FALSE, ch,
              wall, NULL, TO_ROOM);
          spell_dispel_magic(MAX(50, GET_LEVEL(ch)), ch, 0, SPELL_TYPE_SPELL,
                             0, wall);
        }
      }
    }
    else
    {
      bImpossible = true;
    }
  }
  return !bImpossible;
}
// End mobdestroywall

void mobact_rescueHandle(P_char mob, P_char attacker)
{
#define comrade_inSameGroup(A, B)  ((IS_NPC(A) && IS_NPC(B) && (GET_RNUM(A) == GET_RNUM(B))))

  int      door, opp_dir, to_room;
  P_char   pl, next_pl;
  char     buf[MAX_STRING_LENGTH];      /*, buf2[MAX_STRING_LENGTH]; */

  return;


  if((GET_STAT(mob) < STAT_SLEEPING) || (GET_STAT(attacker) < STAT_SLEEPING))
    return;

  if(IS_FIGHTING(mob))
    return;

/*
 * if(!CHAR_IN_TOWN(mob))  act("$n screams 'HELP ME COMRADES... I AM
 * BEING ATTACKED!!'", FALSE, mob, 0, 0, TO_ROOM);
 */

  for (pl = world[mob->in_room].people; pl; pl = next_pl)
  {
    next_pl = pl->next_in_room;

    if(IS_PC(pl) || IS_MORPH(pl) || IS_FIGHTING(pl) ||
        (GET_POS(pl) < POS_STANDING))
      continue;
    if(!comrade_inSameGroup(mob, pl) || !IS_GUARD(pl) || IS_ANIMAL(pl))
      continue;

    if(pl != mob)
    {
      if(IS_GUARD(pl))
        sprintf(buf,
                "Argh! Fighting right in front of me? Have you a deathwish?\r\n");
      else
        sprintf(buf, "Hey! That's my pal you're messing with!\r\n");
      mobsay(pl, buf);
#ifndef NEW_COMBAT
      hit(pl, attacker, pl->equipment[PRIMARY_WEAPON]);
#else
      hit(pl, attacker, pl->equipment[WIELD], TYPE_UNDEFINED,
          getBodyTarget(pl), TRUE, FALSE);
#endif

      if(!char_in_list(attacker) || (attacker->in_room != pl->in_room))
        return;
    }
/*
   if(!attacker || attacker->in_room != pl->in_room)
   return;
 */
  }

  for (door = 0; door < NUM_EXITS; door++)
  {

    if((door == UP) || (door == DOWN))
      continue;
    if(!EXIT(mob, door))
      continue;
    to_room = EXIT(mob, door)->to_room;
    if(to_room == NOWHERE)
      continue;

/*    opp_dir = (door + 2) % 4; */
    opp_dir = rev_dir[door];

    if(!world[to_room].dir_option[opp_dir])
      continue;
    if(world[to_room].dir_option[opp_dir]->to_room != mob->in_room)
      continue;

    if(IS_SET(world[to_room].dir_option[opp_dir]->exit_info, EX_CLOSED))
      continue;

    for (pl = world[to_room].people; pl; pl = next_pl)
    {
      next_pl = pl->next_in_room;

      if(IS_PC(pl) || IS_MORPH(pl))
        continue;

      if(IS_FIGHTING(pl) || (GET_POS(pl) != POS_STANDING) ||
          IS_SET(pl->specials.act, ACT_SENTINEL))
        continue;

    }
  }
}


int MobCanGo(P_char ch, int dir)
{
  int      foo, a;

  foo = cmd_to_exitnumb(dir);

  if(foo == -1)
  {
    return FALSE;
  }

  if(IS_PC(ch) || !CAN_GO(ch, foo))
    return FALSE;

  if(IS_SET(world[EXIT(ch, foo)->to_room].room_flags, NO_MOB))
    return FALSE;

  if(IS_SET(ch->specials.act, ACT_STAY_ZONE) &&
      !IS_SET(ch->specials.act, ACT_HUNTER) &&
      (world[ch->in_room].zone != world[EXIT(ch, foo)->to_room].zone))
    return FALSE;

  if(IS_SET(ch->specials.act, ACT_SENTINEL) && !IS_SET(ch->specials.act,
                                                        ACT_HUNTER))
    return FALSE;

  if(ch->followers)
  {
    for( struct follow_type *k = ch->followers; k; k = k->next )
    {
      if( !CAN_GO(k->follower, foo) ||
          !CAN_ACT(k->follower) ||
          IS_CASTING(k->follower) )
        return FALSE;
    }
  }

  if(ch->player.birthplace &&
      (EXIT(ch, foo)->to_room != ch->player.birthplace) &&
      !IS_SET(ch->specials.act, ACT_HUNTER))
    return FALSE;

  a = EXIT(ch, foo)->to_room;

  if(!(world[a].dir_option[rev_dir[foo]]))
    return FALSE;
  if(IS_SET(world[a].dir_option[rev_dir[foo]]->exit_info, EX_CLOSED))
    return FALSE;
  if(world[a].dir_option[rev_dir[foo]]->to_room != ch->in_room)
    return FALSE;
  return TRUE;
}

void MobHuntCheck(P_char ch, P_char vict)
{
  P_char   tmp;
  int      a, ora;
  int      room;
  char     buf[10];
  P_obj    t_obj, bs_weap = NULL;

  if(IS_PC(ch))
    return;

  if(ch->in_room == vict->in_room)
    return;

  if(IS_AFFECTED2(ch, AFF2_CASTING))
    return;

  if(GET_LEVEL(ch) < 10 && IS_ANIMAL(ch))
    return;

  if(GET_HIT(ch) <= (GET_MAX_HIT(ch) / 3))
    return;

  if(ch->specials.fighting != NULL &&
      ch->specials.fighting->in_room == ch->in_room)
    return;

  /* mother fucking code needs some fucking sanity checks.  Here they
     come: */

  if(IS_SET(ch->specials.act, ACT_STAY_ZONE) &&
      (world[ch->in_room].zone != world[vict->in_room].zone))
    return;

  if(IS_SET(world[vict->in_room].room_flags, NO_MOB))
    return;

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
    if((tmp->specials.fighting == ch) && tmp != vict)
      return;
  if((tmp = PickTarget(ch)) != NULL)
    if(ch->specials.fighting == NULL)
    {
      MobStartFight(ch, tmp);

      if(GET_STAT(ch) == STAT_DEAD)
        return;

      if(ch->specials.fighting != NULL)
        return;
    }
  /*
   * Ok.. behavior: he'll try to follow char.. (sentinel mobs follow 1
   * pace if returning seems possible, hunter mobs follow about always,
   * and hunter+sentinel mobs follow as long as needed, and try to get
   * home after that)
   *
   * if not possible, tries to summon if cleric. if that neither
   * possible, gives up and sulks
   */

  if((a = In_Adjacent_Room(ch, vict)) && MobCanGo(ch, a))
    if(ch->in_room != vict->in_room)
      if(number(1, 100) <=
          MIN(IS_SET(ch->specials.act, ACT_HUNTER) ? 97 : 90,
              50 + GET_LEVEL(ch)))
      {
        if(IS_THIEF(ch))
        {                       /*
                                 * nice check to see if bs weapon
                                 * wielded
                                 */
          if(!
              ((ch->equipment[WIELD] && IS_BACKSTABBER(ch->equipment[WIELD]))
               || (ch->equipment[SECONDARY_WEAPON] &&
                   IS_BACKSTABBER(ch->equipment[SECONDARY_WEAPON]))))
          {
            for (t_obj = ch->carrying; t_obj && !bs_weap;
                 t_obj = t_obj->next_content)
              if(IS_BACKSTABBER(t_obj))
                bs_weap = t_obj;
            if(bs_weap)
            {
              if(!ch->equipment[WIELD])
              {
                obj_from_char(bs_weap);
                equip_char(ch, bs_weap, WIELD, FALSE);
              }
              else if(!ch->equipment[SECONDARY_WEAPON] &&
                       !IS_SET(ch->equipment[WIELD]->extra_flags,
                               ITEM_TWOHANDS))
              {
                obj_from_char(bs_weap);
                equip_char(ch, bs_weap, SECONDARY_WEAPON, FALSE);
              }
              else
              {
                /*
                 * got to clear a weapon slot
                 */
                if(IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_TWOHANDS))
                {
                  if(!IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_NODROP))
                  {
                    obj_from_char(bs_weap);
                    obj_to_char(unequip_char(ch, WIELD), ch);
                    equip_char(ch, bs_weap, WIELD, FALSE);
                  }
                }
                else if(!IS_SET(ch->equipment[SECONDARY_WEAPON]->extra_flags, ITEM_NODROP))
                {
                  obj_from_char(bs_weap);
                  obj_to_char(unequip_char(ch, SECONDARY_WEAPON), ch);
                  equip_char(ch, bs_weap, SECONDARY_WEAPON, FALSE);
                }
                else
                  if(!IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_NODROP))
                {
                  obj_from_char(bs_weap);
                  obj_to_char(unequip_char(ch, WIELD), ch);
                  equip_char(ch, bs_weap, WIELD, FALSE);
                }
              }
            }
          }
          if(!IS_AFFECTED(ch, AFF_SNEAK))
            do_sneak(ch, NULL, 0);
        }
        if(IS_SET(ch->specials.act, ACT_SENTINEL))
          if(!ch->player.birthplace)
          {
            ch->player.birthplace = world[ch->in_room].number;
            ora = ch->in_room;
          }
          else
            ora = 0;
        else
          ora = -1;
        do_move(ch, NULL, a);
        CharWait(ch, PULSE_VIOLENCE);
        if(!ora && ch->player.birthplace == ch->in_room)
          ch->player.birthplace = 0;
        return;
      }
  if(ch->in_room != vict->in_room && Summonable(vict))
  {
    if( /*IS_CLERIC(ch) && */ (npc_has_spell_slot(ch, SPELL_SUMMON)))
    {
      MobCastSpell(ch, vict, 0, SPELL_SUMMON, GET_LEVEL(ch));
      return;
    }
  }
  if(ch->in_room != vict->in_room &&
      npc_has_spell_slot(ch, SPELL_SIREN_SONG))
  {
    MobCastSpell(ch, vict, 0, SPELL_SIREN_SONG, GET_LEVEL(ch));
    return;
  }
  /* Flying is no escape! */
  if(ch->specials.z_cord < vict->specials.z_cord)
  {
    if(!IS_AFFECTED(ch, AFF_FLY))
    {
      if(npc_has_spell_slot(ch, SPELL_FLY))
        MobCastSpell(ch, ch, 0, SPELL_FLY, GET_LEVEL(ch));
      else if(npc_has_spell_slot(ch, SPELL_RAVENFLIGHT))
        MobCastSpell(ch, ch, 0, SPELL_RAVENFLIGHT, GET_LEVEL(ch));
      else if(npc_has_spell_slot(ch, SPELL_GREATER_RAVENFLIGHT))
        MobCastSpell(ch, ch, 0, SPELL_GREATER_RAVENFLIGHT, GET_LEVEL(ch));
      else if(knows_spell(ch, SPELL_POWERCAST_FLY))
        MobCastSpell(ch, ch, 0, SPELL_POWERCAST_FLY, GET_LEVEL(ch));
    }
    else
    {
      strcpy(buf, "up");
      do_fly(ch, buf, 0);
    }
  }
  else if(ch->specials.z_cord > vict->specials.z_cord)
  {
    if(!IS_AFFECTED(ch, AFF_FLY))
    {
      if(npc_has_spell_slot(ch, SPELL_FLY))
        MobCastSpell(ch, ch, 0, SPELL_FLY, GET_LEVEL(ch));
      else if(npc_has_spell_slot(ch, SPELL_RAVENFLIGHT))
        MobCastSpell(ch, ch, 0, SPELL_RAVENFLIGHT, GET_LEVEL(ch));
      else if(npc_has_spell_slot(ch, SPELL_GREATER_RAVENFLIGHT))
        MobCastSpell(ch, ch, 0, SPELL_GREATER_RAVENFLIGHT, GET_LEVEL(ch));
      else if(knows_spell(ch, SPELL_POWERCAST_FLY))
        MobCastSpell(ch, ch, 0, SPELL_POWERCAST_FLY, GET_LEVEL(ch));
    }
    else
    {
      if(vict->specials.z_cord == 0)
        strcpy(buf, "land");
      else
        strcpy(buf, "down");

      do_fly(ch, buf, 0);
    }
  }
  if(ch->in_room != vict->in_room) {
    if(!IS_SET(ch->specials.act, ACT_SENTINEL) &&      /*IS_MAGE(ch) && */
        !IS_SET(world[ch->in_room].room_flags, NO_TELEPORT) &&
        !IS_SET(world[vict->in_room].room_flags, NO_TELEPORT) &&
        world[ch->in_room].zone == world[vict->in_room].zone &&
        npc_has_spell_slot(ch, SPELL_DIMENSION_DOOR))
      MobCastSpell(ch, vict, 0, SPELL_DIMENSION_DOOR, GET_LEVEL(ch));

    return;
  }
  if(ch->in_room != vict->in_room)
    return;

  room = ch->in_room;
  if((tmp = PickTarget(ch)) != NULL)
  {
    MobStartFight(ch, tmp);
    if(is_char_in_room(ch, room))
      CharWait(ch, PULSE_VIOLENCE);
    return;
  }
  if(ch->specials.fighting == NULL && CAN_SEE(ch, vict))
  {
    MobStartFight(ch, vict);
    if(is_char_in_room(ch, room))
      CharWait(ch, PULSE_VIOLENCE);
    return;
  }
}

#define GET_CHAR_ZONE(ch) world[(ch)->in_room].zone
bool TryToGetHome(P_char ch)
{
  int      rr_birth;
  hunt_data data;
  P_nevent  ev;

  if(IS_PC(ch))
    return FALSE;

  /*
   * non-sentinel mobs are allowed to wander... but in order to keep
   * them from hunting way out in never-never land, and then staying in
   * a place they don't belong, we'll have them head back home unless
   * they are already in their own zone...
   */

  if(!IS_SET(ch->specials.act, ACT_SENTINEL))
    return FALSE;

  if(IS_SET(ch->specials.act, ACT_SENTINEL) && ch->following)
    return FALSE;

  if(IS_SET(ch->specials.act2, ACT2_NO_LURE))
    return FALSE;

  rr_birth = real_room(ch->player.birthplace);
  if(rr_birth == NOWHERE)
    return FALSE;

  if(ch->in_room == rr_birth)
    return FALSE;


  /*
   * don't allow duplicate hunt events!
   */
  LOOP_EVENTS_CH(ev, ch->nevents)
  {
    if( ev->func == mob_hunt_event )
    {
      return FALSE;
    }
  }
  data.hunt_type = HUNT_ROOM;
  data.targ.room = rr_birth;
  data.huntFlags = BFS_BREAK_WALLS;
  if(npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC))
    data.huntFlags |= BFS_CAN_DISPEL;
  if(IS_MAGE(ch) || IS_AFFECTED(ch, AFF_FLY))
    data.huntFlags |= BFS_CAN_FLY;
  if(!get_scheduled(ch, mob_hunt_event))
  {
    add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
  }
  //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, data);

  return TRUE;
}

/* defined in config.h */

//#define MAX_ZONES 512
        /*
         * * * For now, 256 max zones, making it 2kb
         * * * RAM
         */

void clearRememberArray(void)
{
  struct remember_data *foo, *foo2;
  int      a;

  for (a = 0; a < MAX_ZONES; a++)
    if(remember_array[a] != NULL)
    {
      for (foo = remember_array[a]; foo; foo = foo2)
      {
        foo2 = foo->next;
        FREE(foo);
      }
      remember_array[a] = NULL;
    }
}

void AddToRememberArray(P_char ch, int zone)
{
  struct remember_data *foo;

  if(IS_NPC(ch))
    return;

  if((zone < 0) || (zone >= MAX_ZONES))
    return;

  CREATE(foo, remember_data, 1, MEM_TAG_REMEMBD);

  foo->c = ch;
  foo->next = remember_array[zone];
  remember_array[zone] = foo;

}

void AddCharToZone(P_char ch)
{
  int      a;

  if(IS_NPC(ch))
    return;
  a = (ch->in_room == -1 ? -1 : world[ch->in_room].zone);
  if((a >= 0) && (a < MAX_ZONES))
    AddToRememberArray(ch, a);
}

void send_to_zone_func(int z, int mask, const char *msg)
{
  struct remember_data *foo;

  if(z <= 0)
    return;
  if(z >= MAX_ZONES)
    return;
  for (foo = remember_array[z]; foo; foo = foo->next)
  {
    if(mask > 0)
      if(!IS_SET(world[foo->c->in_room].room_flags, mask))
        continue;
    if(mask < 0)
      if(IS_SET(world[foo->c->in_room].room_flags, -mask))
        continue;
    send_to_char(msg, foo->c);
  }
}


/*
 * send a spell message to all in zone, EXCEPT those in 'room'. JAB
 */

static char buf[MAX_STRING_LENGTH];
void zone_spellmessage(int room, const char *msg, const char *msg_dir)
{
  int      z, mask;
  struct remember_data *chars_in_zone;

  if((room < 0) || (room > top_of_world))
    return;
  z = world[room].zone;

  bool indoors = IS_SET(world[room].room_flags, INDOORS);

  for (chars_in_zone = remember_array[z]; chars_in_zone;
       chars_in_zone = chars_in_zone->next)
  {
    int c_room = chars_in_zone->c->in_room;
    if(c_room == room)
      continue;
    if(indoors && !IS_SET(world[c_room].room_flags, INDOORS))
      continue;
    if(!indoors && IS_SET(world[c_room].room_flags, INDOORS))
      continue;

    if (IS_MAP_ROOM(room))
    {
      if (IS_MAP_ROOM(c_room) && world[room].map_section != 0 && world[room].map_section == world[c_room].map_section)
      {
        if (msg_dir != 0)
        {
          sprintf(buf, msg_dir, get_map_direction(c_room, room));
          send_to_char(buf, chars_in_zone->c);
        }
        else
          send_to_char(msg, chars_in_zone->c);
      }
      else
        continue;
    }
    else
    {
      send_to_char(msg, chars_in_zone->c);
    }
  }
}

void zone_powerspellmessage(int room, const char *msg)
{
  int      z, mask;
  struct remember_data *chars_in_zone;

  if((room < 0) || (room > top_of_world))
    return;
  z = world[room].zone;

  mask = (int) (IS_SET(world[room].room_flags, INDOORS) ? INDOORS : -INDOORS);

  for (chars_in_zone = remember_array[z]; chars_in_zone;
       chars_in_zone = chars_in_zone->next)
  {
    if(chars_in_zone->c->in_room == room)
      continue;
    if((mask > 0) &&
        !IS_SET(world[chars_in_zone->c->in_room].room_flags, mask))
      continue;
    if((mask < 0) &&
        IS_SET(world[chars_in_zone->c->in_room].room_flags, -mask))
      continue;
    if(StatSave(chars_in_zone->c, APPLY_POW, 0))
      send_to_char(msg, chars_in_zone->c);
  }
}


int CheckMindflayerPresence(P_char ch)
{
  int      z;
  int      room;
  struct remember_data *chars_in_zone;

  return 0;

  room = ch->in_room;
  z = world[room].zone;

  for (chars_in_zone = remember_array[z]; chars_in_zone;
       chars_in_zone = chars_in_zone->next)
  {
    if(IS_ILLITHID(chars_in_zone->c) && (chars_in_zone->c != ch) &&
        (GET_LEVEL(chars_in_zone->c) > 25))
      return 1;
  }
  return 0;
}

void DelCharFromZone(P_char ch)
{
  int      a;
  struct remember_data *foo, *foo2 = NULL;

  if(IS_NPC(ch))
    return;
  if(ch->in_room == -1)
    return;
  a = (world[ch->in_room].zone);
  if(!remember_array[a])
    return;
  if((a < 0) || (a >= MAX_ZONES))
    return;
  if(remember_array[a]->c == ch)
  {
    foo = remember_array[a];
    remember_array[a] = foo->next;
    FREE(foo);
  }
  else
  {
    for (foo = remember_array[a]; foo && !foo2; foo = foo->next)
      if(foo->next && foo->next->c == ch)
        foo2 = foo;
    if(foo2)
    {
      foo = foo2->next;
      foo2->next = foo->next;
      FREE(foo);
    }
  }

}

void SetRememberArray(void)
{
  P_desc   a;

  clearRememberArray();
  for (a = descriptor_list; a; a = a->next)
    if(!a->connected && a->character && (a->character->in_room >= 0) &&
        (GET_STAT(a->character) > STAT_DEAD))
      AddToRememberArray(a->character, world[a->character->in_room].zone);
}

bool CheckForRemember(P_char ch)
{
  struct remember_data *a;
  P_char   b = NULL, c = NULL, tmpch;
  int      real_birthroom;

  if( !IS_ALIVE(ch) || !IS_NPC(ch) )
  {
    return FALSE;
  }

  if( GET_MASTER(ch) )
  {
    return FALSE;
  }

  if(IS_NPC(ch) && (!HAS_MEMORY(ch) || (GET_MEMORY(ch) == NULL)))
  {
    return FALSE;
  }

  if(get_linking_char(ch, LNK_RIDING))
  {
    return FALSE;
  }

  if(GET_HIT(ch) <= (GET_MAX_HIT(ch) / 3))
  {
    real_birthroom = real_room0(ch->player.birthplace);
    if(IS_SET(ch->specials.act, ACT_SENTINEL) &&
       real_birthroom &&
       real_birthroom != ch->in_room)
    {
      TryToGetHome(ch);
    }
    
    if( !CAN_ACT(ch) || IS_IMMOBILE(ch) )
    {
      add_event(event_mob_mundane, PULSE_VIOLENCE, ch, NULL, NULL, 0, NULL, 0);
      //AddEvent(current_event->type, PULSE_VIOLENCE, TRUE, ch, 0);
      return TRUE;
    }
  }
  for (a = remember_array[world[ch->in_room].zone]; a; a = a->next)
  {
    tmpch = a->c;
    if(!tmpch)
      continue;
    if(GET_STAT(tmpch) == STAT_DEAD)
      continue;
    if(!SanityCheck(tmpch, "CheckForRemeber"))
      continue;
    if(CheckFor_remember(ch, tmpch))
    {
      if(GET_CHAR_ZONE(ch) == GET_CHAR_ZONE(tmpch))
      {
        if((!b || number(0, 1)) && (Summonable(tmpch) || !Summonable(b)))
          b = tmpch;
        if((!c || number(0, 1)) && In_Adjacent_Room(ch, tmpch))
          c = tmpch;
      }
    }
  }

  if(c && (number(1, 100) < 90))
    MobHuntCheck(ch, c);
  else if(b && (number(1, 100) < 30))
    MobHuntCheck(ch, b);

  return !CAN_ACT(ch);
}

/* find a player in mem to do nasty things to */

void mobact_memoryHandle(P_char mob)
{
  P_char   vict;
  Memory  *mem;

  if(CHAR_IN_SAFE_ZONE(mob))
    return;

  if(!HAS_MEMORY(mob) || IS_FIGHTING(mob) || ALONE(mob) ||
      !AWAKE(mob) || (NumAttackers(mob) > 0) || (mob->in_room == NOWHERE))
  {
    return;
  }
/*  found = FALSE;*/
  for (vict = world[mob->in_room].people; vict /*&& !found */ ;
       vict = vict->next_in_room)
  {
    if(IS_NPC(vict) || !CAN_SEE(mob, vict))
      continue;
    if(vict == GET_MASTER(mob))
      continue;
    for (mem = mob->only.npc->memory; mem; mem = mem->next)
      if(mem->pcID == GET_PID(vict))
      {
        if((IS_SET(world[mob->in_room].room_flags, SINGLE_FILE) &&
             !AdjacentInRoom(mob, vict)) || !is_aggr_to(mob, vict))
          continue;
        if(GET_HIT(mob) >= (GET_MAX_HIT(mob)) >> 2)
        {
          act("$n recognizes $N, and charges to attack!",
              FALSE, mob, 0, vict, TO_NOTVICT);
          act("$n recognizes you, and charges to attack!",
              FALSE, mob, 0, vict, TO_VICT);

          MobStartFight(mob, vict);
          return;
        }
        else if(!fear_check(mob))
        {
          do_flee(mob, 0, 0);
          return;
        }
      }
  }

}

bool npc_has_spell_slot(P_char mob, int spl)
{
  int      i, circle;
  int      min_circle = MAX_CIRCLE + 1;

  if(!IS_NPC(mob) || !IS_SPELL_S(spl))
    return (FALSE);

  if(!mob->player.m_class)
  {
    return FALSE;
  }

  if(IS_MULTICLASS_NPC(mob))
  {
    for (i = 0; i < CLASS_COUNT; i++)
    {
      if(mob->player.m_class & (1 << i))
      {
        circle = skills[spl].m_class[i].rlevel[0];
        if(circle && (circle < min_circle))
          min_circle = circle;
      }
    }    
  }
  else if(SKILL_DATA_ALL(mob, spl).maxlearn[0] || SKILL_DATA_ALL(mob, spl).maxlearn[mob->player.spec])
  {
    min_circle = MIN(SKILL_DATA_ALL(mob, spl).rlevel[0], SKILL_DATA_ALL(mob, spl).rlevel[mob->player.spec]);    
  }

  if(min_circle <= MAX_CIRCLE && mob->specials.undead_spell_slots[min_circle] > 0)
    return TRUE;    

  return FALSE;
}

bool InitNewMobHunt(P_char ch)
{
  P_nevent  ev = NULL;
  struct remember_data *a;
  P_char   tmpch;
  int      dummy;
  hunt_data data;

  if(!ch)
  {
    logit(LOG_EXIT, "InitNewMobHunt called with null ch");
    raise(SIGSEGV);;
  }
  if(IS_PC(ch))
    return FALSE;

  if(!IS_SET(ch->specials.act, ACT_HUNTER) || IS_FIGHTING(ch) ||
      GET_MASTER(ch) || IS_SET(ch->specials.act2, ACT2_NO_LURE))
  {
    return FALSE;
  }
  /*
   * check for existing hunt events.  If any exist, return false
   */

  LOOP_EVENTS_CH(ev, ch->nevents)
  {
    if(ev->func == mob_hunt_event)
    {
      return FALSE;
    }
  }
  /*
   * if a mob is injured too badly, don't bother hunting
   */
  if(GET_HIT(ch) <= (GET_MAX_HIT(ch) / 3))
  {
    return (TryToGetHome(ch));
  }
  /*
   * only initiate hunting people who are in my zone
   */
  for (a = remember_array[world[ch->in_room].zone]; a; a = a->next)
  {
    tmpch = a->c;

    if(!SanityCheck(tmpch, "InitNewMobHunt"))
      continue;

    if( GET_STAT(tmpch) == STAT_DEAD || !CAN_SEE(ch, tmpch) )
      continue;

    if(CheckFor_remember(ch, tmpch) &&
        (GET_CHAR_ZONE(ch) == GET_CHAR_ZONE(tmpch)))
    {
      /*
       * found one!  but make sure I can get to him...
       */
      if(!IS_AFFECTED(ch, AFF_FLY) && (tmpch->specials.z_cord > 0))
        return FALSE;

      long hunt_flags = (IS_MAGE(ch) || IS_AFFECTED(ch, AFF_FLY)) ? BFS_CAN_FLY : 0;
      if(((GET_LEVEL(ch) * 2) + GET_C_INT(ch))  >= 190)
        hunt_flags |= (npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) ? BFS_CAN_DISPEL : BFS_BREAK_WALLS);

      if(IS_SET(ch->specials.act, ACT_STAY_ZONE))
        hunt_flags |= BFS_STAY_ZONE;

      if(!get_scheduled(ch, mob_hunt_event) &&
         find_first_step(ch->in_room, tmpch->in_room, hunt_flags, 0, 0, &dummy) >= 0)
      {
        data.huntFlags = hunt_flags;
        data.hunt_type = HUNT_HUNTER;
        data.targ.victim = tmpch;
        add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
        
        //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, data);
        return TRUE;
      }
    }
  }

  /*
   * okay.. no victim was found :( ... time to go home...
   */
  return (TryToGetHome(ch));

}

/*
 * NewMobHunt.  This will only be called from an event. InitNewMobHunt
 * will setup the first EVENT_MOB_HUNT, and this code will deal with the
 * events (which should occur at PULSE_MOB_HUNT). Basically, just move the
 * mob one step closer to  the victim.  If Dimdoor is available, use it.
 * If relocate is available, use it.
 *
 * This code will deal with adding a new event for the next step.  return
 * TRUE if I did something, otherwise, FALSE
 */
void mob_hunt_event(P_char ch, P_char victim, P_obj obj, void *d)
//bool NewMobHunt(void)
{
  char     buf[MAX_STRING_LENGTH];
  byte     next_step;
  int      dummy, dummy2;
  hunt_data *data;
  P_char   vict;
  int      cur_room, targ_room;

  /*
   * hmm.. if I'm hunting for a justice purpose, and the hunt stops, I
   * need to tell the justice code about it... use justice_hunt_cancel()
   * for that.
   */

  if(IS_PC(ch))
    return;

  data = (hunt_data *)d;
  if(!data)
  {
    justice_hunt_cancel(ch);
    return;
  }

  cur_room = ch->in_room;

  if(data->hunt_type <= HUNT_LAST_VICTIM_TARGET)
  {
    vict = data->targ.victim;
    if(!vict || !IS_ALIVE(vict))
    {
      justice_hunt_cancel(ch);
      return;
    }
    targ_room = vict->in_room;
  }
  else
  {
    targ_room = data->targ.room;
    if( targ_room < 0 || targ_room > top_of_world )
    {
      int maliciousPID = GET_MEMORY(ch) ? GET_MEMORY(ch)->pcID : -1;

      if( mob_index[GET_RNUM(ch)].func.mob == very_angry_npc && maliciousPID > 0 )
      {
        for( P_desc d = descriptor_list; d; d = d->next )
        {
          if( d->character && d->connected == CON_PLYNG && GET_PID(d->character) == maliciousPID )
          {
            debug( "Found maliciousPID %d, calling very_angry_npc on '%s'...",
              GET_PID(d->character), J_NAME(d->character) );
            very_angry_npc( ch, d->character, CMD_SHOUT, NULL );
          }
        }
      }
      justice_hunt_cancel(ch);
      return;
    }
    vict = NULL;
  }

  if(targ_room == NOWHERE)
  {
    justice_hunt_cancel(ch);
    return;
  }
  /*
   * fighting mobs cant hunt...
   */
  if(IS_FIGHTING(ch))
  {
    justice_hunt_cancel(ch);
    return;
  }
  if(vict && (IS_AFFECTED(vict, AFF_WRAITHFORM)))
    return;

  /*
   * justice mobs won't have ACT_HUNTER, so won't fall in here...
   */
  if(data->hunt_type == HUNT_HUNTER)
  {

    /*
     * non-HUNTER mobs should NEVER fall in here
     */

    if(!IS_SET(ch->specials.act, ACT_HUNTER))
      return;

    if(IS_SET(ch->specials.act2, ACT2_NO_LURE))
      return;

    /*
     * SENTINEL and STAY_ZONE mobs don't leave their zone when hunting
     */

    if((IS_SET(ch->specials.act, ACT_SENTINEL) ||
         IS_SET(ch->specials.act, ACT_STAY_ZONE)) &&
        (world[cur_room].zone != world[targ_room].zone))
      return;

    /* TIMKEN LAG patch, only track people in our zone EVER */
    if(world[cur_room].zone != world[targ_room].zone)
      return;
    data->huntFlags |= BFS_STAY_ZONE;

    /*
     * if the mob doesn't hate the victim, then don't hunt them!
     */
    if(vict && !CheckFor_remember(ch, vict))
      return;

    /*
     * if a mob is injured too badly, don't bother hunting
     */
    if(GET_HIT(ch) <= (GET_MAX_HIT(ch) / 3))
      return;
  }


  if(!CAN_ACT(ch) || IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS) ||
      IS_CASTING(ch) || IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
      affected_by_spell(ch, SONG_SLEEP) || affected_by_spell(ch, SPELL_SLEEP))
  {
    /*
     * Okay.. if anything falls in here, they can't move right now, but
     * they should try later...
     */
    if(!get_scheduled(ch, mob_hunt_event))
    {
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
    }    
    return;
  }

  /*
   * if sleeping, but not magically, then wake up, and stand them up!
   */
  if(!AWAKE(ch))
  {
    do_wake(ch, NULL, 0);

  }
  if(!get_scheduled(ch, mob_hunt_event))
  {
    add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
    return;
  }
  if(GET_POS(ch) != POS_STANDING)
  {
    do_stand(ch, NULL, 0);
  }
  if(!get_scheduled(ch, mob_hunt_event))
  {
    add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
    return;
  }
  if(GET_STAT(ch) != STAT_NORMAL)
  {
    do_alert(ch, NULL, 0);
    if(!get_scheduled(ch, mob_hunt_event))
    {
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
    }
    return;
  }
  /*
   * mobs who don't live in a hometown, don't go in a hometown
   */
  if(CHAR_IN_TOWN(ch) != zone_table[world[targ_room].zone].hometown)
  {
    justice_hunt_cancel(ch);
    return;
  }
  /*
   * okay.. mage type mobs will keep levitate or fly active while
   * moving.  This will facilitate several problems: dimming or
   * relocating to a NO_GROUND room would be messy without levitate or
   * fly... as well, this allows me to run over WATER_NOSWIM and
   * NO_GROUND rooms..
   */

  if((cur_room != targ_room) &&
     (knows_spell(ch, SPELL_GREATER_RAVENFLIGHT) ||
      knows_spell(ch, SPELL_LEVITATE) ||
      knows_spell(ch, SPELL_FLY) ||
      knows_spell(ch, SPELL_POWERCAST_FLY) ||
      knows_spell(ch, SPELL_RAVENFLIGHT)) &&
     !IS_AFFECTED((ch), AFF_LEVITATE) &&
     !IS_AFFECTED((ch), AFF_FLY) &&
     !IS_SET(world[cur_room].room_flags, (NO_MAGIC | ROOM_SILENT)))
  {

    /*
     * if I got the slots, cast it.. prefer fly over levitate.. its
     * more stylish, and puts less stress on moves
     */
    if(npc_has_spell_slot(ch, SPELL_GREATER_RAVENFLIGHT))
    {
      MobCastSpell(ch, ch, 0, SPELL_GREATER_RAVENFLIGHT, GET_LEVEL(ch));
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
      return;
    }

    if(npc_has_spell_slot(ch, SPELL_FLY))
    {
      MobCastSpell(ch, ch, 0, SPELL_FLY, GET_LEVEL(ch));
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
      return;
    }
    else if(npc_has_spell_slot(ch, SPELL_POWERCAST_FLY))
    {
      MobCastSpell(ch, ch, 0, SPELL_POWERCAST_FLY, GET_LEVEL(ch));
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
      return;
    }
    else if(npc_has_spell_slot(ch, SPELL_LEVITATE))
    {
      MobCastSpell(ch, ch, 0, SPELL_LEVITATE, GET_LEVEL(ch));
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
      return;
    }
    else if(npc_has_spell_slot(ch, SPELL_RAVENFLIGHT))
    {
      MobCastSpell(ch, ch, 0, SPELL_RAVENFLIGHT, GET_LEVEL(ch));
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
      return;
    }
    /*
     * hmmm.. didn't have the slots.  Either I'm outta spells, or I'm
     * very low level.  Either way, I suck badly, and need to worry
     * about something other then hunting
     */
    justice_hunt_cancel(ch);
    return;
  }
  /*
   * cleric mobs should vigorize themselves...
   */
  if((GET_VITALITY(ch) <= (GET_MAX_VITALITY(ch) / 4)) &&       /*IS_CLERIC(ch) && */
      !IS_SET(world[cur_room].room_flags, (NO_MAGIC | ROOM_SILENT)))
  {
    if(npc_has_spell_slot(ch, SPELL_INVIGORATE))
    {
      MobCastSpell(ch, ch, 0, SPELL_INVIGORATE, GET_LEVEL(ch));
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
      return;
    }
  }
  /*
   * okay... now get a direction to go...
   */

  next_step = find_first_step(cur_room, targ_room, data->huntFlags, 0, 0, &dummy);

  /*
   * We've arrived. What type of hunt? take action.
   */

  /*
   * Was hunting a wanted criminal
   */
  if((next_step == BFS_ALREADY_THERE) &&
     (data->hunt_type == HUNT_JUSTICE_ARREST))
  {
    justice_action_arrest(ch, vict);
  }
  /* for special justice hunts.. */
  if((next_step == BFS_ALREADY_THERE) &&
      ((data->hunt_type == HUNT_JUSTICE_SPECVICT) ||
       (data->hunt_type == HUNT_JUSTICE_SPECROOM)))
  {
    JusticeGuardHunt(ch);
    return;
  }
  /*
   * Was either hunting an invader, or just a normal hunt
   */
  else if((next_step == BFS_ALREADY_THERE) &&
           ((data->hunt_type == HUNT_HUNTER) ||
            (data->hunt_type == HUNT_JUSTICE_INVADER)))
  {
    if(IS_DISGUISE(vict) && (data->hunt_type == HUNT_JUSTICE_INVADER))
    {
      justice_hunt_cancel(ch);
      return;
    }
    if(CAN_SEE(ch, vict) && !IS_AFFECTED(vict, AFF_HIDE))
    {
      MobStartFight(ch, vict);
      if(!char_in_list(ch))
        return;
    }
  /*  else if(number(0, 3) && // Mobs can no longer see hidden chars - Drannak 10/2012
           IS_AFFECTED(vict, AFF_HIDE))
    {
      do_search(ch, NULL, 0);  
    }*/
    else
      switch (number(0, CAN_SPEAK(ch) ? 114 : 110))
      {
      case 1:
        do_action(ch, 0, CMD_PEER);
        break;
      case 2:
        do_action(ch, 0, CMD_GLARE);
        break;
      case 3:
        if(!CAN_SPEAK(ch))
          do_action(ch, 0, CMD_GROWL);
        break;
      case 4:
        if(!LEGLESS(ch))
        {
          do_action(ch, 0, CMD_TAP);
        }
      case 5:
        if(!IS_AFFECTED4(ch, AFF4_NOFEAR))
        {
          do_flee(ch, 0, 2);
        }
      case 101:
        do_action(ch, 0, CMD_FROWN);
        break;
      case 102:
        do_action(ch, 0, CMD_CURSE);
        break;
      }
    if(!IS_FIGHTING(ch) && !IS_DESTROYING(ch))
    {
      /*
       * they flee?  whatever happened, stay on them...
       */
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
      return;
    }
    return;
  }
  /* okay.. adding a little something here to fuck with mobs and
     players alike.  If the mob isn't doing a justice hunt, and is low
     level, they might get lost <giggle>.  They might even just give
     up! */
  if(data->hunt_type == HUNT_HUNTER)
  {
    if(number(0, 120) > (GET_LEVEL(ch) + 60))
    {
      next_step = number(0, NUM_EXITS - 1);
      if(!world[ch->in_room].dir_option[(int) next_step])
      {
        add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
        return;
      }
    }
    if(!number(0, 500) &&
      (GET_LEVEL(ch) < 30))
    {
      if(CAN_SPEAK(ch))
        do_action(ch, 0, CMD_THROW);
      return;
    }
  }
  /*
   * mage mobs will dimdoor or relocate...
   * ... but only if they are far away... and then only 90% of the time
   * (which will keep them from looping in un forseen circumstances)
   */

  if((cur_room != targ_room) &&
     (dummy > 5) &&
     number(0, 10))
    if((IS_MAGE(ch) ||
       GET_CLASS(ch, CLASS_SHAMAN)) &&
       !IS_SET(world[cur_room].room_flags, NO_TELEPORT | NO_MAGIC | ROOM_SILENT) &&
       !IS_SET(world[targ_room].room_flags, NO_TELEPORT | NO_MAGIC | ROOM_SILENT))
    {

      /*
       * if we don't have a victim to dim to, lets try to find
       * one...
       */
      if(!vict)
        for (vict = world[targ_room].people; vict; vict = vict->next_in_room)
          if(!IS_TRUSTED(vict) && IS_PC(vict))
            break;

      if(vict)
      {
        if(world[cur_room].zone == world[vict->in_room].zone &&
           npc_has_spell_slot(ch, SPELL_DIMENSION_DOOR))
        {
          MobCastSpell(ch, vict, 0, SPELL_DIMENSION_DOOR, GET_LEVEL(ch));
          add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0,
            data, sizeof(hunt_data));
          return;
        }
        else if(world[cur_room].zone != world[vict->in_room].zone &&
                npc_has_spell_slot(ch, SPELL_SPIRIT_JUMP))
        {
          MobCastSpell(ch, vict, 0, SPELL_SPIRIT_JUMP, GET_LEVEL(ch));
          add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0,
            data, sizeof(hunt_data));
          return;
        }
        else if(world[cur_room].zone != world[vict->in_room].zone
                 && npc_has_spell_slot(ch, SPELL_RELOCATE))
        {
          MobCastSpell(ch, vict, 0, SPELL_RELOCATE, GET_LEVEL(ch));
          add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0,
            data, sizeof(hunt_data));
          return;
        }
        else if(world[cur_room].zone != world[vict->in_room].zone &&
                 npc_has_spell_slot(ch, SPELL_SHADOW_TRAVEL))
        {
          MobCastSpell(ch, vict, 0, SPELL_SHADOW_TRAVEL, GET_LEVEL(ch));
          add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0,
            data, sizeof(hunt_data));
          return;
        }
        else if(world[cur_room].zone != world[vict->in_room].zone &&
                 npc_has_spell_slot(ch, SPELL_ETHER_WARP))
        {
          MobCastSpell(ch, vict, 0, SPELL_ETHER_WARP, GET_LEVEL(ch));
          add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0,
            data, sizeof(hunt_data));
          return;
        }
      }
    }
  /*
   * cleric mobs can do word of recall.  Now, do this under two possible
   * circumstances.  Either their birthplace is where they are going in
   * the first place, or their birthplace is closer to where they are
   * going....
   */

  if(IS_CLERIC(ch) && (dummy > 6) &&
      !IS_SET(world[cur_room].room_flags, (NO_MAGIC | NO_RECALL | ROOM_SILENT))
      &&
      GET_BIRTHPLACE(ch) && (real_room(GET_BIRTHPLACE(ch)) != NOWHERE) &&
      !IS_SET(world[real_room0(GET_BIRTHPLACE(ch))].room_flags, NO_RECALL))
  {
    if(targ_room == real_room(GET_BIRTHPLACE(ch)))
      dummy2 = 0;
    else if(find_first_step(real_room(GET_BIRTHPLACE(ch)), targ_room, data->huntFlags,
                             0, 0, &dummy2) < 0)
      dummy2 = dummy + 1;       /*
                                 * if no path from recall room, make sure
                                 * dummy is smaller then dummy2
                                 */

    if((dummy2 < dummy) && npc_has_spell_slot(ch, SPELL_WORD_OF_RECALL))
    {
      MobCastSpell(ch, ch, 0, SPELL_WORD_OF_RECALL, GET_LEVEL(ch));
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
      return;
    }
  }
  /*
   * if I got an error on the find_path, just stop hunting this person
   */
  if(next_step < 0)
  {
    justice_hunt_cancel(ch);
    return;
  }
  /*
   * note: find_first_step shouldn't lead me through SECRET or LOCKED
   * doors.
     Unless you can just walk though it. Apr09 -Lucrot
   */
  
  if(IS_SET(world[(cur_room)].dir_option[(int) next_step]->exit_info, EX_CLOSED) &&
     !IS_AFFECTED2(ch, AFF2_PASSDOOR))
  {
    /* animals don't open doors! */
    if(!CAN_SPEAK(ch) &&
       !IS_GREATER_RACE(ch))
    {
      justice_hunt_cancel(ch);
      return;
    }
    /*
     * okay.. closed door in the way.. just open it :)
     *     */

    sprintf(buf, "%s", dirs[(int) next_step]);
    do_open(ch, buf, 0);
    
    add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
    return;
  }

  // Why duplicate the above written code? Apr09 -Lucrot
  // if(IS_SET(world[(cur_room)].dir_option[(int) next_step]->exit_info,
             // EX_CLOSED))
  // {
    // /* animals don't open doors! */
    // if(!CAN_SPEAK(ch))
    // {
      // justice_hunt_cancel(ch);
      // return;
    // }
    // /*
     // * okay.. closed door in the way.. just open it :)
     // */
    // sprintf(buf, "%s %s", EXIT(ch, (int) next_step)->keyword ?
            // FirstWord(EXIT(ch, (int) next_step)->keyword) : "door",
            // dirs[(int) next_step]);
    // do_open(ch, buf, 0);
    // add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
    // return;
  // }
  
  if(IS_WALLED(cur_room, next_step))
  {
    if(MobDestroyWall(ch, next_step,  IS_SET(data->huntFlags, BFS_BREAK_WALLS)))
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
    return;
  }

  /*
   * no obstacles... just go get 'em
   */

  /*
   * if, after trying to move, I'm still in the same room, something is
   * wrong.  I'll fail movement 4 or 5 times, and then give up trying to
   * move in that direction (but I won't give up the hunt!).
   */

  if((data->retry < 5) || (data->retry_dir != next_step))
  {
    if((data->hunt_type == HUNT_JUSTICE_SPECVICT) ||
        (data->hunt_type == HUNT_JUSTICE_SPECROOM))
      JusticeGuardMove(ch, NULL, exitnumb_to_cmd(next_step));
    else
      do_move(ch, NULL, exitnumb_to_cmd(next_step));

    if(!char_in_list(ch))
      return;

    if(ch->in_room == cur_room)
    {
      if(data->retry_dir != next_step)
        data->retry = 0;
      data->retry++;
      data->retry_dir = next_step;
    }
    else
    {
      data->retry = 0;
    }
  }
  add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, data, sizeof(hunt_data));
  return;
}

/* given a protector flagged mob that has nothing better to do, this
   function will return NULL if there isn't anyone in the room to
   assist, or a P_char of a person that would be a good assist target
 */

/* how it works... loop through everyone in my room.  For each person
   fighting, accumulate a 'score' for them.  Whoever has the highest
   'score' is the one we assist.  Score is determined by adding bit
   fields.  This ensures that if the "highest" condition is met, its
   not overruled by a combination of all the lower conditions.  The
   bit fields I chose to use were mostly arbitrary, but meant to allow
   room for further conditions.  Assuming "ch" is the PROTECTOR, t_ch
   is someone we are considering to assist, and 'vict' is the person
   that t_ch is fighting, the rules for scoring are:

   * if ch is following t_ch, add BIT_32 (highest)
   * if ch and t_ch have the same vnum, add BIT_31
   * if t_ch is NPC and vict is PC, add BIT_4 if vict is charmed, and
   BIT_5 if they aren't (so mobs fighting non-charmed PC's score higher)
   * if t_ch is NPC, and vict is charmed, add BIT_1.

   THE FOLLOWING RULES ONLY APPLY IF ch IS A JUSTICE GUARD!

   * if ch and t_ch have the same race, and ch and vict are different
   race, add BIT_20
   * if ch is not neutral, has the same alignment as t_ch, and a
   different alignment then vict, then add BIT_15

 */


P_char find_protector_target(P_char ch)
{
  P_char   t_ch, vict;
  P_char   best_target = NULL;  /* best yet found assist target */
  unsigned best_value = 0;      /* value for the best yet found target */
  unsigned cur_val;             /* value for currently examining
                                   target */
  int      is_guard = FALSE;    /* set to TRUE if ch is a justice
                                   guard */

  if( !ch || IS_FIGHTING(ch) || IS_DESTROYING(ch) || !CAN_ACT(ch) || ALONE(ch) || GET_MASTER(ch) )
  {
    return NULL;
  }

  if( CHAR_IN_TOWN(ch) && (hometowns[CHAR_IN_TOWN(ch) - 1].flags) && IS_GUARD(ch) )
  {
    is_guard = TRUE;
  }

  LOOP_THRU_PEOPLE(t_ch, ch)
  {
    cur_val = 0;

    if((ch == t_ch) || !IS_FIGHTING(t_ch) || !CAN_SEE(ch, t_ch) || IS_MORPH(t_ch) )
    {
      continue;
    }

    // don't assist undead unless ch and vict share group/follow
    if(!IS_UNDEAD(ch) && IS_UNDEAD(t_ch) && (t_ch->following != ch) && (ch->following != t_ch) && (!ch->group || (ch->group != t_ch->group)))
    {
      continue;
    }

    /* Don't assist animals... we're not all druids/hippies here */
    if(IS_ANIMAL(t_ch) && (t_ch->following != ch) && (ch->following != t_ch))
    {
      continue;
    }

    /* never assist charmies... UNLESS they are charmed by me.  Note
       that if AFF_CHARM is set, but they aren't following anyone,
       then they aren't really charmed  */
    if( IS_NPC(t_ch) && IS_PC_PET(t_ch) )
    {
      continue;
    }

    vict = t_ch->specials.fighting;

    // If both vict and t_ch are town guards, they do not engage eachother.
    if( vict && IS_GUARD(vict) && is_guard )
    {
      continue;
    }

    /* FIRST, if this is my "leader", then highest value... */
    if( ch->following == t_ch )
    {
      cur_val |= BIT_32;
    }

    /* SECOND, check for same vnum... */
    if( IS_NPC(t_ch) && GET_VNUM(ch) == GET_VNUM(t_ch) )
    {
      cur_val |= BIT_31;
    }

    /* special handling for guild golems */
    if( GET_A_NUM(ch) )
    {

      /* golems ALWAYS kill enemies */
      if(find_enemy(vict, (ush_int) GET_A_NUM(ch)))
        cur_val |= BIT_30;
      else if((GET_A_NUM(ch) == GET_A_NUM(t_ch)) &&
               (GET_A_NUM(ch) != GET_A_NUM(vict)))
        cur_val |= BIT_30;
      else
        continue;               /* don't let other protector code work */
    }
    /* now it splits, depending on if its justice related or not.. */
    if(is_guard && (IS_NPC(t_ch) || IS_NPC(vict)) && !IS_ANIMAL(vict))
    {

      /* if this is the same race as me (but the person they are
         fighting isn't) */
      if((GET_RACE(t_ch) == GET_RACE(ch)) &&
          (GET_RACE(vict) != GET_RACE(ch)))
        cur_val |= BIT_20;

      /* alignment preference, but only check extremes */
      if(IS_GOOD(ch) && IS_GOOD(t_ch) && !IS_GOOD(vict))
        cur_val |= BIT_20;

      if(IS_EVIL(ch) && IS_EVIL(t_ch) && !IS_EVIL(vict))
        cur_val |= BIT_20;

    }

    if(IS_PC(vict) && IS_NPC(t_ch))
    {
      if(GET_MASTER(vict))
        cur_val |= BIT_4;
      else if((GET_RACE(ch) != GET_RACE(vict)) ||
               (GET_RACE(ch) == GET_RACE(t_ch)))
        cur_val |= BIT_5;
      else if((t_ch->following == ch))
        cur_val |= BIT_4;
    }
    else if(IS_NPC(t_ch) && IS_AFFECTED(vict, AFF_CHARM))
      cur_val |= BIT_1;

    if(cur_val > best_value)
    {
      best_target = t_ch;
      best_value = cur_val;
    }
  }
  return best_target;
}

void MobRetaliateRange(P_char ch, P_char vict)
{
  char /*buf[MAX_INPUT_LENGTH], */ result;
  P_nevent  ev = NULL;
  int      dummy;
  hunt_data data;

/*  P_char hunter = NULL;
   int no_range_attack = TRUE; */
  struct affected_type af;

  if(!SanityCheck(ch, "MobRetaliateRange"))
    return;

  if(!ch || !vict)
    return;

  if(IS_PC(ch))
    return;

  if(GET_STAT(vict) == STAT_DEAD)
    return;

  if(!CAN_ACT(ch))
    return;

  // patrol mobs won't retaliate range if they aren't aggro to...
  if(IS_PATROL(ch) && !is_aggr_to(ch, vict))
    return;

  if(ch->in_room == vict->in_room &&
      ch->specials.z_cord == vict->specials.z_cord)
  {
    MobStartFight(ch, vict);
    return;
  }
  if(IS_CASTING(ch))
    if(is_casting_aggr_spell(ch))
    {
/*      wizlog(56,"We've just detected an aggro spell being cast and thus do not break spellcasting upon being ranged.");*/
      return;
    }
    else if(IS_FIGHTING(ch))
    {
      return;
    }
    else
      StopCasting(ch);

  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  /* Add to memory! */

  if(HAS_MEMORY(ch))
  {
    if(IS_PC(vict))
    {
      if(!(IS_TRUSTED(vict) && IS_SET(vict->specials.act, PLR_AGGIMMUNE)))
        if((GET_STAT(ch) > STAT_INCAP))
          remember(ch, vict);
    }
    else if(IS_PC_PET(vict) && CAN_SEE(ch, GET_MASTER(vict)))
    {
      if(!
          (IS_TRUSTED(GET_MASTER(vict)) &&
           IS_SET(GET_MASTER(vict)->specials.act, PLR_AGGIMMUNE)))
        if((GET_STAT(ch) > STAT_INCAP))
          remember(ch, GET_MASTER(vict));
    }
  }
  /* A few guaranteed calls */
#if 0
  if( /*IS_DRAGON(ch) */ CAN_BREATHE(ch))
  {

    /* if there's an error..  exit the function */

    result = find_first_step(ch->in_room, vict->in_room, 1, 0, 0, &dummy);
    if(result >= 0)
      BreathWeapon(ch, result);
    else
      return;
  }
#endif // no range breath anymore, circling that really sucks
  /* Higher wimpy set, as they know its tough to charge into an arrow */

  if(AWAKE(ch) && CAN_ACT(ch) && !IS_STUNNED(ch))
    if(IS_SET(ch->specials.act, ACT_WIMPY) &&
        (GET_HIT(ch) < (GET_LEVEL(ch) * 6)) &&
        room_has_valid_exit(ch->in_room))
      do_flee(ch, 0, 0);

  /* Next group will handle situation on their own */

  if(!mob_can_range_att(ch, vict) && !IS_SET(ch->specials.act2, ACT2_NO_LURE))
  {

    /* try to charge them */

    /* Are they hunting already? */
    LOOP_EVENTS_CH(ev, ch->nevents)
    {
      if(ev->func == mob_hunt_event)
      {
        return;
      }
    }

    /* Can they even get there? (rivers, etc) */
    if(find_first_step(ch->in_room, vict->in_room,
                        (IS_MAGE(ch) || IS_AFFECTED(ch, AFF_FLY)) ? BFS_CAN_FLY : 0,
                        0, 0, &dummy)
        >= 0)
    {
      data.hunt_type = HUNT_JUSTICE_INVADER;
      data.targ.victim = vict;
      data.huntFlags = (IS_MAGE(ch) || IS_AFFECTED(ch, AFF_FLY)) ? BFS_CAN_FLY : 0;
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
      //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, data);
      add_event(return_home, 30, ch, 0, 0, 0, 0, 0);
      return;
    }
    else
    {
      if(room_has_valid_exit(ch->in_room))
        do_flee(ch, 0, 0);
      if((!IS_AFFECTED3(ch, AFF3_COVER)))
      {
        bzero(&af, sizeof(af));
        af.duration = 10;
        af.bitvector3 = AFF3_COVER;
        affect_to_char(ch, &af);
      }
      else if(room_has_valid_exit(ch->in_room))
        do_flee(ch, 0, 0);
    }
  }
}

/* Mob Memory Routines */

void remember(P_char ch, P_char victim)
{
  remember(ch, victim, TRUE);
}

/* make ch remember victim */
void remember(P_char ch, P_char victim, bool check_group_remember)
{
  Memory  *tmp;
  struct group_list *gl;
  
  if(!(ch) ||
     !(victim) ||
     !IS_ALIVE(ch) ||
     !IS_ALIVE(victim) ||
     !IS_NPC(ch) ||
     IS_NPC(victim) ||
     IS_TRUSTED(victim) ||
     !HAS_MEMORY(ch))
  {
    return;
  }

  if(check_group_remember &&
     NPC_REMEMBERS_GROUP(ch) &&
     victim->group)
  {
    for (gl = victim->group; gl; gl = gl->next)
    {
      if (ch->in_room == gl->ch->in_room)
      {
        // logit(LOG_DEBUG,
          // "remembering group members, and the lucky one is: %s",
          // GET_NAME(gl->ch));
        remember(ch, gl->ch, FALSE);
      }
    }
  }

  /* don't let golems aggro their own guildees */
  if((IS_OP_GOLEM(ch) || IS_GH_GOLEM(ch)) && (GET_A_NUM(ch) == GET_A_NUM(victim)))
  {
    return;
  }

  for (tmp = ch->only.npc->memory; tmp; tmp = tmp->next)
  {
    if(tmp->pcID == GET_PID(victim))
      return;
  }

  CREATE(tmp, Memory, 1, MEM_TAG_MOBMEM);
  tmp->next = ch->only.npc->memory;
  tmp->pcID = GET_PID(victim);
  ch->only.npc->memory = tmp;

}


/* make ch forget victim */
void forget(P_char ch, P_char victim)
{
  Memory  *curr, *prev = NULL;


  if(!IS_NPC(ch))
    return;

  if(IS_NPC(victim))
   return; //cannot call get_pid on npc


  if(!(curr = ch->only.npc->memory))
    return;

  if(ch == victim)
  return;

  while (curr && (curr->pcID != GET_PID(victim)))
  {
    prev = curr;
    curr = curr->next;
  }

  if(!curr)
    return;                     /* person wasn't there at all. */

  if(curr == ch->only.npc->memory)
    ch->only.npc->memory = curr->next;
  else
    prev->next = curr->next;

  FREE(curr);
}

int CheckFor_remember(P_char ch, P_char victim)
{
  Memory  *tmp;


  if(!IS_NPC(ch) || IS_NPC(victim) || IS_TRUSTED(victim) || !HAS_MEMORY(ch))
    return FALSE;

  for (tmp = ch->only.npc->memory; tmp; tmp = tmp->next)
  {
    if(tmp->pcID == GET_PID(victim))
      return TRUE;
  }

  return FALSE;
}

/* erase ch's memory */
void clearMemory(P_char ch)
{
  Memory  *curr, *next;


  // HAS_MEMORY checks if is NPC..

  if(!HAS_MEMORY(ch))
    return;

  curr = ch->only.npc->memory;

  while (curr)
  {
    next = curr->next;
    FREE(curr);
    curr = next;
  }

  ch->only.npc->memory = NULL;
}


/*
 * entering a room sets a delayed agg attack for eligible movers and
 * occupants, when that event triggers, this routine gets called to see if
 * target is still legal, if it is, it starts combat.  JAB
 */

void event_agg_attack(P_char ch, P_char victim, P_obj obj, void *data)
{
  int door;

  if(!(ch) ||
     !(victim) ||
     victim->in_room == NOWHERE ||
     GET_STAT(victim) == STAT_DEAD ||
     ch->in_room == NOWHERE ||
     !MIN_POS(ch, POS_STANDING + STAT_RESTING) ||
     !is_aggr_to(ch, victim) ||
     IS_IMMOBILE(ch) ||
     !AWAKE(ch) ||
     IS_STUNNED(ch))
  {
    return;
  }
  
  if(ch->specials.z_cord != victim->specials.z_cord)
  {
    return;
  }
  
  if(ch->in_room == victim->in_room)
  {   
    if((IS_MAGE(victim) ||
       IS_CLERIC(victim)) &&
       !number(0, 2) &&
       !IS_PC_PET(victim) &&
       !IS_CASTING(ch) &&
       !IS_IMMOBILE(ch))
    {
        if(GOOD_FOR_GAZING(ch, victim) &&
           number(0, 2))
        {
          gaze(ch, victim);
        }
        else if(isMaulable(ch, victim) &&
                GET_CHAR_SKILL(ch, SKILL_MAUL) > 40 &&
                number(0, 2))
        {
          do_maul(ch, GET_NAME(victim), CMD_MAUL);
        }
        else if(isSpringable(ch, victim) &&
                GET_CHAR_SKILL(ch, SKILL_SPRINGLEAP) > 40 &&
                number(0, 2))
        {
          do_kneel(ch, 0, CMD_KNEEL);
          do_springleap(ch, GET_NAME(victim), 0);
        }
        else if( isBashable(ch, victim) && GET_CHAR_SKILL(ch, SKILL_BASH) > 40 && number(0, 2) )
        {
          bash(ch, victim);
        }
        else if(GET_CHAR_SKILL(ch, SKILL_SWITCH_OPPONENTS))
        {
          attack(ch, victim);
        }
    }
    
    if(IS_PC(ch))
    {
      send_to_char("Being the ferocious sort, you charge at the enemy!\r\n", ch);
    }
    
    if(IS_NPC(ch))
    {
      MobStartFight(ch, victim);
    }
    else
    {
      if(GET_CHAR_SKILL(ch, SKILL_BACKSTAB) &&
        ((ch->equipment[WIELD] && IS_BACKSTABBER(ch->equipment[WIELD])) ||
        (ch->equipment[SECONDARY_WEAPON] &&
        IS_BACKSTABBER(ch->equipment[SECONDARY_WEAPON]))))
      {
        backstab(ch, victim);
      }
      else
      {
        attack(ch, victim);
      }
    }
  }
  else
  {
    /* They moved before event triggered, let's see if they are nearby */
    if(IS_PC(ch))
    {
      return;                   /* PCs will have to track on their own */
    }
    
    if(IS_SET(ch->specials.act, ACT_SENTINEL) || IS_FIGHTING(ch))
    {
      return;                   /* damn, missed again */
    }
    
    for(door = 0; door < NUM_EXITS; door++)
      if(CAN_GO(ch, door) &&
        (victim->in_room == EXIT(ch, door)->to_room) &&
        CAN_SEE(ch, victim))
      {
        do_move(ch, 0, exitnumb_to_cmd(door));
        add_event(event_agg_attack, 1, ch, victim, 0, 0, 0, 0);
        return;
      }
  }
}

void event_mob_skin_spell(P_char ch, P_char vict, P_obj obj, void *data)
{
  struct affected_type af, *afp;

  if(IS_AFFECTED(ch, AFF_STONE_SKIN)) {
    if(!(afp = get_spell_from_char(ch, SPELL_STONE_SKIN))) {
      memset(&af, 0, sizeof(af));
      af.type = SPELL_STONE_SKIN;
      af.duration = -1;
      afp = affect_to_char(ch, &af);
      act("&+L$n's skin seems to turn to stone.", TRUE, ch, 0, 0,
          TO_ROOM);
      act("&+LYou feel your skin harden to stone.", TRUE, ch, 0, 0,
          TO_CHAR);
    }
    afp->modifier = GET_LEVEL(ch) + MAX(0, GET_LEVEL(ch) - 50) * 5;
    
    if(IS_PC_PET(ch))
      afp->modifier /= 2;
  }

  if(IS_AFFECTED(ch, AFF_BIOFEEDBACK)) {
    if(!(afp = get_spell_from_char(ch, SPELL_BIOFEEDBACK))) {
      memset(&af, 0, sizeof(af));
      af.type = SPELL_BIOFEEDBACK;
      af.duration = -1;
      af.modifier = (int) (1.5 * (GET_LEVEL(ch) + MAX(0, GET_LEVEL(ch) - 50) * 5));
      afp = affect_to_char(ch, &af);
      send_to_char("&+GYou are surrounded by a green mist!&n\r\n", ch);
      act("&+G$n&+G is suddenly surrounded by a green mist!", TRUE, ch, 0, 0,
          TO_NOTVICT);
    }
    afp->modifier = (int) (1.5 * (GET_LEVEL(ch) + MAX(0, GET_LEVEL(ch) - 50) * 5));
  }

  add_event(event_mob_skin_spell,
      get_property("timer.secs.mob.skinRefresh", 60) * WAIT_SEC,
      ch, 0, 0, 0, 0, 0);
}

void try_wield_weapon(P_char ch)
{
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     IS_PC_PET(ch))
        return;
  
  if(empty_slot_for_weapon(ch) == -1)
    return;

  P_obj obj = NULL;

  /* find a weapon */
  // Find artifact weapon and wield it.
  for(P_obj t_obj = ch->carrying; t_obj; t_obj = t_obj->next_content)
  {
    if(t_obj->type == ITEM_WEAPON )
    { // Try to wield an available artifact weapon first.
      if(IS_SET(t_obj->extra_flags, ITEM_ARTIFACT))
      {
        wear(ch, t_obj, 12, 1); // 12 means wield, 1 means display message to room
      }
    }
  }

// Find a nice weapon and wield it.
  for(P_obj t_obj = ch->carrying; t_obj; t_obj = t_obj->next_content)
  {
    if(t_obj->type == ITEM_WEAPON &&
      (IS_SET(t_obj->bitvector, AFF_STONE_SKIN) ||
      IS_SET(t_obj->bitvector, AFF_HASTE) ||
      IS_SET(t_obj->bitvector2, AFF2_FIRE_AURA) ||
      IS_SET(t_obj->bitvector2, AFF2_GLOBE) ||
      IS_SET(t_obj->bitvector2, AFF2_SOULSHIELD) ||
      IS_SET(t_obj->bitvector3, AFF3_GR_SPIRIT_WARD) ||
      IS_SET(t_obj->bitvector4, AFF4_DAZZLER) ||
      IS_SET(t_obj->bitvector4, AFF4_HELLFIRE)) ||
      obj_index[t_obj->R_num].func.obj != NULL)
    { 
      wear(ch, t_obj, 12, 1); // 12 means wield, 1 means display message to room
    }
  }

// Wield anything else...
  for(P_obj t_obj = ch->carrying; t_obj; t_obj = t_obj->next_content)
  {
    if(t_obj->type == ITEM_WEAPON )
    { 
      wear(ch, t_obj, 12, 1); // 12 means wield, 1 means display message to room
    }
  }
}

int empty_slot_for_weapon(P_char ch)
{
  if( HAS_FOUR_HANDS(ch) )
  {
    if( !ch->equipment[WIELD] )
      return WIELD;

    if( ch->equipment[WIELD2] )
      return WIELD2;

    if( !ch->equipment[WIELD2] )
      return WIELD2;

    if( !ch->equipment[WIELD] )
      return WIELD;
  }
  else
  {
    if( !ch->equipment[WIELD] )
      return WIELD;

    if( !ch->equipment[WIELD2] )
      return WIELD2;
  }

  return -1;
}

void give_proper_stat(P_char ch)
{
  if(ch->base_stats.Str < 80)
    ch->base_stats.Str = number(80, 100);
  if(ch->base_stats.Dex < 80)
    ch->base_stats.Dex = number(80, 100);
  if(ch->base_stats.Int < 80)
    ch->base_stats.Int = number(80, 100);
  if(ch->base_stats.Wis < 80)
    ch->base_stats.Wis = number(80, 100);
  if(ch->base_stats.Agi < 80)
    ch->base_stats.Agi = number(80, 100);
  if(ch->base_stats.Con < 80)
    ch->base_stats.Con = number(80, 100);

  affect_total(ch, FALSE);
}

bool should_teacher_move(P_char ch)
{
  P_char tch;

  // If it's not a teacher, who cares, let it move.
  if (!IS_ACT(ch, ACT_TEACHER))
    return true;

  LOOP_THRU_PEOPLE(tch, ch)
  {
    // If there's a player in the room trying to scribe, don't move.
    if (IS_PC(tch))
      return false;
  }
  return true;
}

bool CastBlighterSpell(P_char ch, P_char victim, bool helping)
{
  P_char   target = NULL;
  P_char   tch;
  int      dam = 0, lvl = 0, spl = 0;
  int      numb = 0, lucky, curr = 0;

  if( !ch || (helping && !victim ) )
  {
    logit(LOG_DEBUG, "CastBlighterSpell: bad args: ch '%s' %d, helping: %s, victim: %s %d.",
      ch ? J_NAME(ch) : "NULL", (ch && IS_NPC(ch)) ? GET_VNUM(ch) : -1, helping ? "TRUE" : "FALSE",
      victim ? J_NAME(victim) : "NULL", (victim && IS_NPC(victim)) ? GET_VNUM(victim) : -1 );
    debug( "CastBlighterSpell: bad args: ch '%s' %d, helping: %s, victim: %s %d.",
      ch ? J_NAME(ch) : "NULL", (ch && IS_NPC(ch)) ? GET_VNUM(ch) : -1, helping ? "TRUE" : "FALSE",
      victim ? J_NAME(victim) : "NULL", (victim && IS_NPC(victim)) ? GET_VNUM(victim) : -1 );
    return FALSE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  /* make sure I'm even able to cast in this room! */
  if( MobShouldFlee(ch) )
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  lvl = GET_LEVEL(ch);

  if( helping )
  {
    // Can't help the dead.
    if( !IS_ALIVE(victim) )
    {
      return FALSE;
    }
    dam = GET_MAX_HIT(victim) - GET_HIT(victim);
    target = victim;
  }
  else
  {
    dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    target = ch;
  }

  // Blind ch check: cure blindness first.
  if( IS_AFFECTED(ch, AFF_BLIND)
    && npc_has_spell_slot(ch, SPELL_DRAIN_NATURE) )
  {
    return MobCastSpell(ch, ch, 0, SPELL_DRAIN_NATURE, lvl);
  }
  else if( IS_AFFECTED(ch, AFF_BLIND)
    && room_has_valid_exit(ch->in_room) && !fear_check(ch) )
  {
    do_flee(ch, 0, 0);
    return FALSE;
  }

  // Check spellups.
  if( !IS_FIGHTING(ch) )
  {
    if( npc_has_spell_slot(ch, SPELL_DRAIN_NATURE) && (dam > 90) )
    {
      spl = SPELL_DRAIN_NATURE;
    }
    else if( !IS_AFFECTED(target, AFF_ARMOR)
      && npc_has_spell_slot(ch, SPELL_THORNSKIN) )
    {
      spl = SPELL_THORNSKIN;
    }
  }

  // Self only spells.
  if( !spl && (!IS_FIGHTING(ch) || (number(1, 5) == 3)) && ch == target )
  {
    if( !affected_by_spell(ch, SPELL_SAP_NATURE)
      && npc_has_spell_slot(ch, SPELL_SAP_NATURE) )
    {
      spl = SPELL_SAP_NATURE;
    }
    else if( !affected_by_spell(ch, SPELL_SHADOW_VISION) &&
      npc_has_spell_slot(ch, SPELL_SHADOW_VISION) )
    {
      spl = SPELL_SHADOW_VISION;
    }
    else if(!affected_by_spell(ch, SPELL_ELEMENTAL_AURA) && (world[ch->in_room].sector_type == SECT_FIREPLANE ||
      world[ch->in_room].sector_type == SECT_WATER_PLANE || world[ch->in_room].sector_type == SECT_AIR_PLANE ||
      world[ch->in_room].sector_type == SECT_EARTH_PLANE) )
    {
      spl = SPELL_ELEMENTAL_AURA;
    }
    else if( count_pets(ch) == 0 && npc_has_spell_slot(ch, SPELL_SHAMBLER) )
    {
      spl = SPELL_SHAMBLER;
    }
  }

  if( !spl && !IS_FIGHTING(ch) && (dam > 0) )
  {
    if( npc_has_spell_slot(ch, SPELL_DRAIN_NATURE) )
      spl = SPELL_DRAIN_NATURE;
  }

  if( spl )
  {
    return MobCastSpell(ch, target, 0, spl, lvl);
  }

  // If not helping or fighting, find someone to help.
  if( !helping && !IS_FIGHTING(ch) )
  {
    for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
    {
      if( (ch != tch) && char_deserves_helping(ch, tch, TRUE) )
      {
        numb++;
      }
    }
    if( numb )
    {
      lucky = number(1, numb);

      for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {
        if( (ch != tch) && char_deserves_helping(ch, tch, TRUE) )
        {
          curr++;
          if( curr == lucky )
          {
            // Make a recursive call..
            return CastBlighterSpell( ch, tch, TRUE );
          }
        }
      }
      logit( LOG_DEBUG, "CastBlighterSpell: Error in random number crap." );
      debug( "CastBlighterSpell: Error in random number crap." );
    }
  }

  // Now, we get violent.. (don't nuke self/friend)
  if( (victim == ch) || helping )
    return FALSE;

  target = victim ? victim : ch->specials.fighting;

  if( !IS_ALIVE(target) )
  {
    return FALSE;
  }

  // If we're fighting a group.. check areas..
  if( ((NumAttackers(ch) > 1) || has_help(target)
    || no_chars_in_room_deserve_helping(ch))
    && !IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) )
  {
    if( npc_has_spell_slot(ch, SPELL_SANDSTORM) && !ch->specials.z_cord
      && number(0, 2) )
    {
      spl = SPELL_SANDSTORM;
    }

    if( !spl && npc_has_spell_slot(ch, SPELL_ACID_RAIN) && IS_OUTSIDE(ch->in_room)
      && number(0, 1) )
    {
      spl = SPELL_ACID_RAIN;
      // Since acid rain can be both targetted and area, we need target = NULL for area.
      target = NULL;
    }

    if( !spl && npc_has_spell_slot(ch, SPELL_FIRESTORM) && !number(0, 2) )
    {
      spl = SPELL_FIRESTORM;
    }

    if( !spl && npc_has_spell_slot(ch, SPELL_EARTHQUAKE) && !number(0, 3) )
    {
      spl = SPELL_EARTHQUAKE;
    }
  }

  if( spl )
  {
    return (MobCastSpell(ch, target, 0, spl, lvl));
  }

  // Now we go to target spells..

  // Storm Bringer spec.
  if( !spl && npc_has_spell_slot(ch, SPELL_BLOODTOSTONE)
    && !affected_by_spell(target, SPELL_BLOODTOSTONE) && number( 0, 1) )
  {
    spl = SPELL_BLOODTOSTONE;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_IMPLOSION) )
    spl = SPELL_IMPLOSION;

  // Storm Bringer spec.
  if( OUTSIDE(ch) && npc_has_spell_slot(ch, SPELL_CALL_LIGHTNING)
    && number(0,1) )
  {
    spl = SPELL_CALL_LIGHTNING;
  }

  // Targetted acid rain.
  if( !spl && npc_has_spell_slot(ch, SPELL_ACID_RAIN) && IS_OUTSIDE(ch->in_room) )
  {
    spl = SPELL_ACID_RAIN;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_HORRID_WILTING))
  {
    spl = SPELL_HORRID_WILTING;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_ACID_STREAM) )
  {
    spl = SPELL_ACID_STREAM;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_TOXIC_FOG) )
  {
    spl = SPELL_TOXIC_FOG;
  }

  if( !spl && !IS_GLOBED(target) && npc_has_spell_slot(ch, SPELL_FIRELANCE) )
  {
    spl = SPELL_FIRELANCE;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_CONTAGION) )
  {
    spl = SPELL_CONTAGION;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_POISON) && !number(0, 2) )
  {
    spl = SPELL_POISON;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_WITHER) && !number(0, 2) )
  {
    spl = SPELL_WITHER;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_RAY_OF_ENFEEBLEMENT) && !number(0, 2) )
  {
    spl = SPELL_RAY_OF_ENFEEBLEMENT;
  }

  if( !spl && !IS_GLOBED(target) && npc_has_spell_slot(ch, SPELL_FLAME_SPHERE) )
  {
    spl = SPELL_FLAME_SPHERE;
  }

  // Well, time to scrape the bottom of the barrel..
  if( !spl && npc_has_spell_slot(ch, SPELL_ACID_BLAST) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)) )
    spl = SPELL_ACID_BLAST;

  if( !spl && npc_has_spell_slot(ch, SPELL_CHILL_TOUCH) && (!IS_GLOBED(target) || !IS_MINGLOBED(target))
    && !number(0, 3) )
  {
    spl = SPELL_CHILL_TOUCH;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_BURNING_HANDS) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)) )
  {
    spl = SPELL_BURNING_HANDS;
  }

  if( !spl && npc_has_spell_slot(ch, SPELL_CAUSE_LIGHT) && (!IS_GLOBED(target) || !IS_MINGLOBED(target)) )
  {
    spl = SPELL_CAUSE_LIGHT;
  }

  if( spl )
  {
    P_char nuke_target = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);
    return MobCastSpell(ch, nuke_target ? nuke_target : target, 0, spl, lvl);
  }

  return FALSE;
}

// Should just be called during a periodic event.
void event_mob_proc(P_char mob, P_char victim, P_obj object, void *data)
{
  P_nevent e;

  if( !IS_ALIVE(mob) )
  {
    debug("event_mob_proc: NULL/Dead mob: %s.", mob ? J_NAME(mob) : "NULL" );
    return;
  }

  if( object != NULL )
  {
    debug("event_mob_proc: non-NULL object: %s %d.", OBJ_SHORT(object), GET_OBJ_VNUM(object) );
  }

  // If mob is lagged, we re-schedule for after the lag wears off.
  if( !CAN_ACT(mob) )
  {
    if( (e = get_scheduled(mob, event_wait)) != NULL )
    {
      add_event(event_mob_proc, ne_event_time(e) + 1, mob, victim, NULL, 0, NULL, 0 );
      return;
    }
    else
    {
      REMOVE_BIT(mob->specials.act2, PLR2_WAIT);
    }
  }
  // Proc on PERIODIC.
  (*mob_index[GET_RNUM(mob)].func.mob) (mob, victim, CMD_PERIODIC, 0);

  // Add another event... should check SPEC_ACT and CMD_SET_PERIODIC, but *shrug* supposedly done it once.
  if( IS_ALIVE(mob) )
  {
    add_event(event_mob_proc, PULSE_MOBILE + number(-4,4), mob, victim, NULL, 0, NULL, 0 );
  }
}
