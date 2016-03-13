/*
 * ***************************************************************************
 * *  File: mobcombatt.c                                           Part of Duris *
 * *  Usage: Procedures generating 'intelligent' behavior in the mobiles.
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
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
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "graph.h"
#include "mm.h"
#include "assocs.h"
#include "guard.h"
#include "damage.h"

/*
 * external variables
 */

extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *dirs[];
extern const struct stat_data stat_factor[];
extern double lfactor[];
extern float fake_sqrt_table[];
extern int MobSpellIndex[MAX_SKILLS];
extern int equipment_pos_table[CUR_MAX_WEAR][3];
extern int no_specials;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern const char rev_dir[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;

const char demon_types[] [16] = {
   "babau",
   "balor",
   "bebilith",
   "dretch",
   "glabrezu",
   "hezrou",
   "incubus",
   "marilith",
   "nalfeshnee",
   "quasit",
   "retriever",
   "succubus",
   "vrock",
};

bool GOOD_FOR_GAZING(P_char ch, P_char victim);

void ZombieCombat(P_char ch, P_char victim)
{
  int random = 0, dam = 0;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

// Removed buggy switch.

  if(IS_MULTICLASS_NPC(ch))
  {
    random = number(1, 25);
  }
  else
  {
    random = number(1, 15);
  }
 
  if(random == 1)
  {
    act("$n&n twitches slightly as a &+gchunk of flesh&n falls off its arm.",
        TRUE, ch, 0, 0, TO_ROOM);
  }
  else if(random == 2)
  {
    act("A piece of &+Lrotted flesh&n drops from $n&n's body.",
      TRUE, ch, 0, 0, TO_ROOM);
  }
  else if(random == 3)
  {
    do_action(ch, 0, CMD_CACKLE);
    act("$n&n's face cracks a bit more.", TRUE, ch, 0, 0, TO_ROOM);
  }
  else if(random == 4)
  {
    do_action(ch, 0, CMD_CACKLE);
  }
  else if(random <= 6)
  {
    do_action(ch, 0, CMD_GROAN);
  }
  else if(random <= 10 &&
          GET_LEVEL(ch) >= 31)
  {
    do_action(ch, 0, CMD_LICK);
    act("$n savagely bites into your flesh!",
      TRUE, ch, 0, victim, TO_VICT);
    act("$n&n savagely bites into $N's flesh!",
      TRUE, ch, 0, victim, TO_NOTVICT);
    spell_disease(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }
  else if(!number(0, 4) && 
          (GET_LEVEL(ch) >= 46) &&
          !IS_UNDEAD(victim) &&
          !IS_UNDEADRACE(victim))
  {
    act("You feel your life force slipping away...",
      TRUE, ch, 0, victim, TO_VICT);
    act("$N recoils from $n!", TRUE, ch, 0, victim, TO_NOTVICT);
    
    dam = dice(5, 10) * 2;
    spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, 0);
  }
  else if(!number(0, 9) &&
          (GET_LEVEL(ch) >= 56) &&
          !affected_by_spell(victim, SPELL_PLAGUE))
  {
    spell_plague(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, 0);
  }
}

void SkeletonCombat(P_char ch, P_char victim)
{

  int random = 0, dam = 0;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

// Removed buggy switch.

  if(IS_MULTICLASS_NPC(ch))
  {
    random = number(1, 25);
  }
  else
  {
    random = number(1, 15);
  }

  if(random == 1)
  {
    act("You hear $n&n's bones rattle slightly, before it presses the attack.",
        TRUE, ch, 0, 0, TO_ROOM);
  }
  else if(random == 2)
  {
    act("Two eerie points of &+rred&n light flare in $n&n's hollow eye sockets.", TRUE, ch, 0,
        0, TO_ROOM);
  }
  else if(random == 3)
  {
    do_action(ch, 0, CMD_CACKLE);
  }
  else if(random <= 6)
  {
    do_action(ch, 0, CMD_GROAN);
  }
  else if(random <=10)
  {
    do_action(ch, 0, CMD_MOAN);
    act("Your attack dislodged one of $n&n's pale white bones, sending it flying into your flesh.", TRUE, ch, 0, victim, TO_VICT);
    act("$N&n's critical hit dislodged one of $n's bones, sending it tearing into $N's flesh.", TRUE, ch, 0, victim, TO_NOTVICT);
    damage(ch, victim, GET_LEVEL(ch), 0);
  }
  else if(!number(0, 4) &&  // 20%
          GET_LEVEL(ch) >= 50 &&
          !IS_UNDEAD(victim) &&
          !IS_UNDEADRACE(victim))
  {
    act("You feel your life force slipping away...", TRUE, ch, 0, victim, TO_VICT);
    act("$N recoils from $n!", TRUE, ch, 0, victim, TO_NOTVICT);
    
    dam = dice(5, 10) * 2;
    spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, 0);
  }
}

void SpectreCombat(P_char ch, P_char victim)
{
  int random = 0;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(IS_MULTICLASS_NPC(ch))
  {
    random = number(1, 25);
  }
  else
  {
    random = number(1, 15);
  }
  
  if(random == 1)
  {
    act("You hear a low groaning sound from $n.",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_GROAN);
  }
  else if(random <= 3)
  {
    do_action(ch, 0, CMD_HOWL);
  }
  else if(random <= 10)
  {
    act("$n suddenly becomes incorporeal and vanishes from your sight!",
      TRUE, ch, 0, victim, TO_ROOM);
    spell_blink(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
  }
  else if(!number(0, 9) &&  // 10%
          GET_LEVEL(ch) >= 56 &&
          !affected_by_spell(victim, SPELL_PLAGUE))
  {
    spell_plague(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, 0);
  }
}

void ShadowCombat(P_char ch, P_char victim)
{
int top;

// Multiclass will have other ability and specials to use.
  if(IS_MULTICLASS_NPC(ch))
  {
    top = 25;
  }
  else
  {
    top = 15; // 15 was the default value.
  }
  
  // High level caster shadows need an opportunity to cast.
  if(GET_LEVEL(ch) >= 51)
  {
    top += 10;
  }
  
  switch (number(1, top))
  {
  case 1:
    act("$n silently moves through the room, covering the area in darkness!.",
        TRUE, ch, 0, 0, TO_ROOM);
    spell_darkness(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
    break;
  case 2:
  case 3:
    do_action(ch, 0, CMD_CACKLE);
    break;
  case 4:
  case 5:
  case 6:
  case 10:
    if( victim )
    {
      act("$n suddenly flares black, and encases you in darkness!", TRUE, ch, 0, victim, TO_VICT);
      act("$n emits a pure sphere of darkness, enveloping $N!", TRUE, ch, 0, victim, TO_NOTVICT);
      blind(ch, victim, 10 * WAIT_SEC);
    }
    break;
  default:
    break;
  }
}

void WraithCombat(P_char ch, P_char victim)
{
int top;

// Multiclass will have other ability and specials to use.
  if(IS_MULTICLASS_NPC(ch))
  {
    top = 25;
  }
  else
  {
    top = 15; // 15 was the default value.
  }
  
  // High level caster wraiths need an opportunity to cast.
  if(GET_LEVEL(ch) >= 51)
  {
    top += 10;
  }
  
  switch (number(1, top))
  {
  case 1:
    act("$n flows gracefully in combat, with dreadful accuracy.",
        TRUE, ch, 0, 0, TO_ROOM);
    break;
  case 2:
  case 3:
    do_action(ch, 0, CMD_MOAN);
    break;
  case 4:
  case 5:
    do_action(ch, 0, CMD_SCREAM);
    break;
  case 6:
  case 10:
    if( victim )
    {
      act("$n lets out a long, &+Wfrightening &nhowl.\n", TRUE, ch, 0, victim, TO_VICT);
      if (!fear_check(victim))
      {
        if (GET_LEVEL(victim) < (GET_LEVEL(ch) / 2))
        {
          act("$n's &+Wunearthly howl scares the bejesus out of $N&+W!&n", TRUE, ch, 0, victim, TO_NOTVICT);
          act("&+WYou flee in sheer terror!&n", TRUE, ch, 0, victim, TO_VICT);
          do_flee(victim, 0, 2);
        }
        else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
        {
          if (!NewSaves(victim, SAVING_FEAR, 5))
          {
            act("$n's &+Wunearthly howl scares the bejesus out of $N&+W!&n", TRUE, ch, 0, victim, TO_NOTVICT);
            act("&+WYou flee in sheer terror!&n", TRUE, ch, 0, victim, TO_VICT);
            do_flee(victim, 0, 1);
          }
        }
        else if (!NewSaves(victim, SAVING_FEAR, 2))
        {
          act("$n's &+Wunearthly howl scares the bejesus out of $N&+W!&n", TRUE, ch, 0, victim, TO_NOTVICT);
          act("&+WYou flee in sheer terror!&n", TRUE, ch, 0, victim, TO_VICT);
          do_flee(victim, 0, 1);
        }
      }
    }
    break;
  default:
    break;
  }
}

int UndeadCombat(P_char ch)
{
  P_char victim = NULL;
  
  /*
   * this fellows undead.. undead do wicked things.
   * among 'features' we have.. - aging effect of ghosts - biting of
   * vampires - vampires' gaseous form
   */
  if (ch->in_room < 0)
    return TRUE;

  if(!IS_PC_PET(ch) &&
    (GET_RACE(ch) == RACE_GHOST || GET_RACE(ch) == RACE_SPECTRE ||
    GET_RACE(ch) == RACE_WRAITH || GET_RACE(ch) == RACE_SHADOW ||
    GET_RACE(ch) == RACE_BRALANI || GET_RACE(ch) == RACE_DEVA) &&
    (number(1, 100) <= 10))
  {
    GhostFearEffect(ch);
    return TRUE;
  }

  if (IS_DRACOLICH(ch) && IS_PC_PET(ch))
    DragonCombat(ch, FALSE);

  if( IS_FIGHTING(ch) )
    victim = ch->specials.fighting;
  
  if (IS_ZOMBIE(ch))
    ZombieCombat(ch, victim);

  if (IS_SKELETON(ch))
    SkeletonCombat(ch, victim);

  if ((IS_SPECTRE(ch) || IS_ASURA(ch)) && !IS_PC_PET(ch))
    SpectreCombat(ch, victim);

  if (IS_SHADOW(ch) || IS_DEVA(ch))
    ShadowCombat(ch, victim);

  if((IS_WRAITH(ch) || IS_BRALANI(ch)) && !IS_PC_PET(ch))
    WraithCombat(ch, victim);

  if( GET_RACE(ch) == RACE_VAMPIRE && victim && number(0,1)
    && GOOD_FOR_GAZING(ch, victim) )
  {
    innate_gaze(ch, victim);
  }

  return FALSE;
}

void ArchonCombat(P_char ch, P_char victim)
{}

void AsuraCombat(P_char ch, P_char victim)
{}

void DevaCombat(P_char ch, P_char victim)
{}

void BralaniCombat(P_char ch, P_char victim)
{}

int AngelCombat(P_char ch)
{
  P_char victim = NULL;
  
  if (ch->in_room < 0)
    return TRUE;

  if(!IS_PC_PET(ch) &&
   ( GET_RACE(ch) == RACE_BRALANI || GET_RACE(ch) == RACE_DEVA) &&
    (number(1, 100) <= 10))
  {
    GhostFearEffect(ch);
    return TRUE;
  }

  if ((IS_TITAN(ch) || IS_AVATAR(ch)) && IS_PC_PET(ch))
    DragonCombat(ch, FALSE);

  if( IS_FIGHTING(ch) )
    victim = ch->specials.fighting;
  
  if (IS_ARCHON(ch))
    ArchonCombat(ch, victim);

  if (IS_ASURA(ch) && !IS_PC_PET(ch))
    AsuraCombat(ch, victim);

  if (IS_DEVA(ch))
    DevaCombat(ch, victim);

  if(IS_BRALANI(ch) && !IS_PC_PET(ch))
    BralaniCombat(ch, victim);

  if (GET_RACE(ch) == RACE_BRALANI && victim && number(0,1)
    && GOOD_FOR_GAZING(ch, victim) )
  {
    innate_gaze(ch, victim);
  }

  return FALSE;
}

int BeholderCombat(P_char ch)
{
  /*
   * mostly based on AD&D beholders - get sleep, telekinesis, flesh to
   * stone (major para), disintegrate, fear, slow, and cause serious (more
   * like full harm on Duris)
   *
   * telekinesis picks up random objects lying in the ground (up to level *
   * 2 weight) and flings them at people.  watch out!
   *
   */

  int hitpc = FALSE, numbPCs = 0, luckyPC, currPC = 0, gotlucky = FALSE;
  int random;
  P_char   tch, nextch, firstPC = NULL;

  if(ch->in_room < 0)
  {
    return TRUE;
  }
/*  if (!number(0, 2))*/

  gotlucky = TRUE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if((tch != ch) &&
      ((IS_PC(tch) &&
      !IS_TRUSTED(tch)) ||
      (tch->specials.fighting == ch)))
    {
      numbPCs++;
      if(!firstPC)
      {
        firstPC = tch;
      }
    }
  }

  /* might not be any PCs, who knows */

  if(!numbPCs)
  {
    return FALSE;
  }
  
  if(numbPCs == 1)
  {
    luckyPC = 0;
  }
  else
  {
    luckyPC = number(0, numbPCs - 1);
  }
  
  for(tch = world[ch->in_room].people; tch; tch = nextch)
  {
    nextch = tch->next_in_room;

    /* if this PC is the lucky one, do some nasty stuff */

    if((tch != ch) &&
      ((IS_PC(tch) && !IS_TRUSTED(tch)) ||
      (tch->specials.fighting == ch)))
    {
      if(currPC == luckyPC)
      {
        hitpc = TRUE;

        /*
         * mostly based on AD&D beholders - get sleep, telekinesis, flesh to
         * stone (major para), disintegrate, fear, slow, and cause serious (more
         * like full harm on Duris)
         */
        random = number(0, 9);  // beholders will not proc every round - Jexni 5/15/09
        {
          /* sleep */
          if(random == 0)
          {
            act("A &+mpurple&n beam shoots from one of your eyestalks,"
                " squarely hitting $N&n.", TRUE, ch, 0, tch, TO_CHAR);
            act("A &+mpurple&n beam shoots from one of $n&n's eyestalks,"
                " striking you square in the chest.", TRUE, ch, 0, tch,
                TO_VICT);
            act("A &+mpurple&n beam shoots from one of $n&n's eyestalks,"
                " striking $N&n square in the chest.", TRUE, ch, 0, tch,
                TO_NOTVICT);
                
            spell_beholder_sleep(GET_LEVEL(ch), ch, tch, NULL);
          }  /* paralyze (flesh to stone) */
          if(random == 1)
          {
            act("A narrow &+Ggreen&n beam shoots from one of your eyestalks,"
                " squarely hitting $N&n.", TRUE, ch, 0, tch, TO_CHAR);
            act
              ("A narrow &+Ggreen&n beam shoots from one of $n&n's eyestalks,"
               " striking you square in the chest.", TRUE, ch, 0, tch,
               TO_VICT);
            act
              ("A narrow &+Ggreen&n beam shoots from one of $n&n's eyestalks,"
               " striking $N&n square in the chest.", TRUE, ch, 0, tch,
               TO_NOTVICT);

               spell_beholder_paralyze(GET_LEVEL(ch), ch, tch, NULL);
          } /* disintegrate */
          if(random == 2 &&
             !number(0, 5))
          {
            act
              ("A blinding &+Wwhite&n beam shoots from one of your eyestalks, "
               " squarely hitting $N&n.", TRUE, ch, 0, tch, TO_CHAR);
            act
              ("A blinding &+Wwhite&n beam shoots from one of $n&n's eyestalks,"
               " striking you square in the chest.", TRUE, ch, 0, tch,
               TO_VICT);
            act
              ("A blinding &+Wwhite&n beam shoots from one of $n&n's eyestalks,"
               " striking $N&n square in the chest.", TRUE, ch, 0, tch,
               TO_NOTVICT);
               
            spell_beholder_disintegrate(GET_LEVEL(ch), ch, tch, NULL);

          } /* fear */
          if(random == 3)
          {
            if(IS_AFFECTED4(tch, AFF4_NOFEAR))
            {
              random = 7;
            }
            else
            {
              act("A &+rdeep red&n beam shoots from one of your eyestalks, "
                  " squarely hitting $N&n.", TRUE, ch, 0, tch, TO_CHAR);
              act("A &+rdeep red&n beam shoots from one of $n&n's eyestalks,"
                  " striking you square in the chest.", TRUE, ch, 0, tch,
                  TO_VICT);
              act("A &+rdeep red&n beam shoots from one of $n&n's eyestalks,"
                  " striking $N&n square in the chest.", TRUE, ch, 0, tch,
                  TO_NOTVICT);
              
              spell_beholder_fear(GET_LEVEL(ch), ch, tch, NULL);
            }
          }  /* slowness */
          if(random == 4)
          {
            act
              ("An intense &+Yyellow&n beam shoots from one of your eyestalks, "
               " squarely hitting $N&n.", TRUE, ch, 0, tch, TO_CHAR);
            act
              ("An intense &+Yyellow&n beam shoots from one of $n&n's eyestalks,"
               " striking you square in the chest.", TRUE, ch, 0, tch,
               TO_VICT);
            act
              ("An intense &+Yyellow&n beam shoots from one of $n&n's eyestalks,"
               " striking $N&n square in the chest.", TRUE, ch, 0, tch,
               TO_NOTVICT);
            
            spell_beholder_slowness(GET_LEVEL(ch), ch, tch, NULL);
            
          }  /* damage (full harm-type) */
          if(random == 5)
          {
            act
              ("A crackling &+Bblue&n beam shoots from one of your eyestalks, "
               " squarely hitting $N&n.", TRUE, ch, 0, tch, TO_CHAR);
            act
              ("A crackling &+Bblue&n beam shoots from one of $n&n's eyestalks,"
               " striking you square in the chest.", TRUE, ch, 0, tch,
               TO_VICT);
            act
              ("A crackling &+Bblue&n beam shoots from one of $n&n's eyestalks,"
               " striking $N&n square in the chest.", TRUE, ch, 0, tch,
               TO_NOTVICT);
           
            spell_beholder_damage(GET_LEVEL(ch), ch, tch, NULL);
           
          }  /* wacky telekinesis thing - only useful in a room with doors */
          if(random == 6)
          {
            act("$n&+W's eye briefly flares brightly.", TRUE, ch, 0, 0,
                TO_ROOM);
            
            spell_beholder_telekinesis(GET_LEVEL(ch), ch, tch, NULL);
          }
          if(random == 7 &&
             !number(0, 1))
          {
            act
              ("A thin &+Ggreen&n beam shoots from one of your eyestalks, "
               " squarely hitting $N&n.", TRUE, ch, 0, tch, TO_CHAR);
            act
              ("A thin &+Ggreen&n beam shoots from one of $n&n's eyestalks,"
               " striking you square in the chest.", TRUE, ch, 0, tch,
               TO_VICT);
            act
              ("A thin &+Ggreen&n beam shoots from one of $n&n's eyestalks,"
               " striking $N&n square in the chest.", TRUE, ch, 0, tch,
               TO_NOTVICT);
            
            spell_beholder_dispelmagic(GET_LEVEL(ch), ch, tch, NULL);
          }
        }
      }                       /* if (currPC == luckyPC) */
      else if (!IS_TRUSTED(tch))
        currPC++;
    }                         /* end of IS_PC check */
  }                           /* end of for loop */

  if(gotlucky &&
    !hitpc &&
    firstPC &&
    char_in_list(firstPC))
  {
    send_to_char("&+WERROR:&n beholder combat is bad.  tell a god.\r\n",
                 firstPC);
  }

  return hitpc;
}

void DriderCombat(P_char ch, P_char victim)
{

int top;

// Multiclass will have other ability and specials to use.
  if(IS_MULTICLASS_NPC(ch))
  {
    top = 25;
  }
  else
  {
    top = 15; // 15 was the default value.
  }
  
  // High level caster driders need an opportunity to cast.
  if(GET_LEVEL(ch) >= 51)
  {
    top += 10;
  }
  
  switch (number(1, top))
  {
  case 1:
    act("$n&n hisses in drow 'Lloth has forsaken me!'",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_MOAN);
    break;
  case 2:
    if( victim && IS_ALIVE(victim) )
    {
      act("$n hisses at you, and strikes forth with devastating speed!", TRUE, ch, 0,
        0, TO_ROOM);
      hit(ch, victim, ch->equipment[PRIMARY_WEAPON]);
      break;
    }
  case 3:
  case 4:
    do_action(ch, 0, CMD_ROAR);
    break;
  case 5:
  case 6:
  case 10:
    if( victim )
    {
      do_action(ch, 0, CMD_HISS);
      act("$n begins to spin a web, attempting to encase you within it!", TRUE, ch, 0, victim, TO_VICT);
      act("$n&n attempts to encase $N in it's web!", TRUE, ch, 0, victim, TO_NOTVICT);
      spell_minor_paralysis(60, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
    }
    break;
  default:
    break;
  }
}

void PwormCombat(P_char ch, P_char victim)
{
int top;

// Multiclass will have other ability and specials to use.
  if(IS_MULTICLASS_NPC(ch))
  {
    top = 25;
  }
  else
  {
    top = 15; // 15 was the default value.
  }
  
  // High level caster pworm need an opportunity to cast.
  if(GET_LEVEL(ch) >= 51)
  {
    top += 10;
  }
  
  switch (number(1, top))
  {
  case 1:
  case 2:
    act("$n&n lets out a deafening &+RSHRIEK!",
        TRUE, ch, 0, 0, TO_ROOM);
    break;
  case 3:
  case 4:
  case 5:
    act("$n&n begins to salivate, &+Gslimy&n mucus flowing from it's maw.",
        TRUE, ch, 0, 0, TO_ROOM);
    break;
  case 10:
    if( victim && IS_ALIVE(victim) )
    {
      do_action(ch, 0, CMD_HISS);
      act("$n hisses loudly for a moment, and gurgles as it expels digestive enzymes at YOU!&n", TRUE, ch, 0, victim, TO_VICT);
      act("$n, in a horrific display, belches up putrid digestive enzymes all over $N!&n", TRUE, ch, 0, victim, TO_NOTVICT);
      spell_corrosive_blast(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, 0);
    }
    break;
  default:
    break;
  }
}

int GenMobCombat(P_char ch)
{
  P_char victim = NULL;
  
  /*
   * stole this from UndeadCombat, just use this for generic racial
   * mobcombat procs. try to be exciting! first inclusion is 
   * drider combat proc.
   */
  if(ch->in_room < 0)
  {
    return TRUE;
  }
  if(ch &&
    IS_FIGHTING(ch))
  {
    victim = ch->specials.fighting;
  }
  if(IS_DRIDER(ch))
  {
    DriderCombat(ch, victim);
  }
  if(IS_PWORM(ch))
  {
    PwormCombat(ch, victim);
  }
  return FALSE;
}

// Mob special currently used for dragons, but can be applied elsewhere.
// Mob stats and dice are increased via randomized rolls. Jan08 -Lucrot
// Chance is probability 
bool Mob_Furious(P_char ch, P_char victim, int chance)
{
  int random = number(1, 100);

  if( !IS_ALIVE(ch) || IS_PC(ch) )
  {
    return false;
  }

  if(random > chance)
  {
    return false;
  }
  
  long furious = (long) get_property("dragon.Furious.DamageMod", 2.000);
    
  act("$n&n appears &+RFURIOUS&n and gains mass and ferocity!&n", 1, ch, 0, 0, TO_ROOM);
  
  if(GET_C_STR(ch) < 100 &&
    !number(0, 3))
      ch->base_stats.Str = 100;

  if(GET_C_DEX(ch) < 100 &&
    !number(0, 3))
      ch->base_stats.Dex = 100;
  
  if(GET_C_AGI(ch) < 100 &&
    !number(0, 3))
      ch->base_stats.Agi = 100;
  
  if(GET_C_POW(ch) < 100 &&
    !number(0, 3))
      ch->base_stats.Pow = 100;
      
  if(GET_C_CON(ch) < 100 &&
    !number(0, 3))
      ch->base_stats.Con = 100;

  // Increases the size of the dice.
  
  if(!number(0, 9))
    ch->points.damnodice += (int) (GET_LEVEL(ch) / 10);


  return true;
}

/*
 * New routine to handle dragons in combat, this will make dragons without
 * unique specials.  Hopefully will make fighting ANY dragon (even babies!)
 * a real challenge.  Called from 2 places, start_fighting(), and
 * MobCombat(). The call from start_fighting invokes 'Dragon Fear'.  Returns
 * TRUE if dragon does something, else FALSE.  -JAB
 */

bool DragonCombat(P_char ch, int awe)
{
  int      i, breath_chance, attacktype = 0;
  P_char   tchar1 = NULL, tchar2 = NULL, vict, next_ch;
  void     (*funct) (int, P_char, char *, int, P_char, P_obj);
  bool     bIsPet = false;
  P_char   chMaster = NULL;

  /* check for non-dragon breathers .. */
  
  /* the rules are changing...  dragon pets (dracoliches) don't use specials
     unless the master is in the room.  If the necro wants specials used,
     they need to be in the room taking risks along with the pet! */
  if (IS_PC_PET(ch))
  {
    bIsPet = true;
    chMaster = GET_MASTER(ch);
    if (ch->in_room != chMaster->in_room)
    {
      return FALSE;
    }
  }

  // Dragons breathing = cool.  Other mobs breathing = ok... but not every round - Jexni 1/20/11
  breath_chance = (IS_DRAGON(ch) ? GET_LEVEL(ch) / 20 : 12 - (GET_LEVEL(ch) / 5));
  breath_chance = ((IS_ELITE(ch) && IS_DRAGON(ch)) ? breath_chance + 1 : breath_chance - 1);

  /* awe should never kill anyone, since DragonCombat is called from
     set_fighting() and assumes just that */

  if(awe)
  {
    /*
     * means we were called from start_fighting, dragons get a
     * special initial 'fear' and 'awe' attack at start on combat,
     * low levels are going to be sent fleeing in terror, and
     * maybe some not-so low levels too.  Even young dragons are
     * nasty opponents
     */

    /*
     * Example: red drag in sewers, level 19, characters will: 1-
     * 6  panic flee, no save 7-18  save vs para at +1 or flee in
     * terror 19-50  save vs para at -2 or flee
     */
    if (!IS_DRAGON(ch) && !IS_TITAN(ch) && !IS_AVATAR(ch))
    {                           
      send_to_char("No roaring for you, bud.\r\n", ch);
      return FALSE;
    }
    if (IS_MORPH(ch) ||
       IS_PC(ch))
    {
      CharWait(ch, 9);
    }
    
    act("Your ROAR fills your victims with sheer terror!",
      0, ch, 0, 0, TO_CHAR);
    act("$n&N &+RROARS&n, filling your heart with &+Lsheer terror!&n",
      1, ch, 0, 0, TO_ROOM);
      
    radiate_message_from_room(ch->in_room, "&+CA thundering roar echoes through the area!\r\n", 
	                            GET_LEVEL(ch)/10 + (GET_LEVEL(ch) > 30 ? (GET_LEVEL(ch)-30)/5 : 0), 
                              (RMFR_FLAGS) (RMFR_RADIATE_ALL_DIRS | RMFR_PASS_WALL | RMFR_PASS_DOOR | RMFR_CROSS_ZONE_BARRIER), 0);
    
    for (tchar1 = world[ch->in_room].people; tchar1; tchar1 = tchar2)
    {
      tchar2 = tchar1->next_in_room;
      
      if (grouped(tchar1, ch))
      {
        continue;
      }
      
      if (chMaster == tchar1)
      {
        continue;
      }
      
      if (affected_by_spell(tchar1, SONG_DRAGONS))
      {
        continue;
      }
      
      if (!IS_DRAGON(tchar1) && !IS_TITAN(tchar1) && !IS_AVATAR(tchar1) &&
          !IS_TRUSTED(tchar1) &&
         (tchar1->specials.z_cord == ch->specials.z_cord))
      {

        if (IS_NPC(tchar1) &&
            !GET_MASTER(tchar1) &&
            tchar1->group && (tchar1->group == ch->group))
        {
          continue;
        }
        
        if (IS_ILLITHID(tchar1))
        {
          continue;
        }
        
        if (bIsPet &&
           IS_NPC(tchar1))
        {
          if ((tchar1->following &&
              IS_NPC(tchar1->following)) || 
              (IS_SET(tchar1->specials.act, ACT_SENTINEL) &&
              !GET_MASTER(tchar1)))
          {
            continue;
          }
        }
        /* for non-pets: allow any non-PC-following NPCs to ignore it */
        else if (IS_NPC(tchar1) &&
                (!tchar1->following || IS_NPC(tchar1->following)) &&
                (ch->specials.fighting != tchar1) &&
                (tchar1->specials.fighting != ch))
        {
          continue;
        }
        
        if (fear_check(tchar1))
        {
          continue;
        }
        
        if (GET_LEVEL(tchar1) < (GET_LEVEL(ch) / 2))
        {
          do_flee(tchar1, 0, 2);        /* panic flee, no save */
        }
        
        if (GET_LEVEL(tchar1) >= GET_LEVEL(ch))
        {
          if (!NewSaves(tchar1, SAVING_FEAR, -5))
          {
            do_flee(tchar1, 0, 1);
          }
        }
        else if (!NewSaves(tchar1, SAVING_FEAR, -2))
        {
          do_flee(tchar1, 0, 1);        /* fear, but not panic */
        }
      }
      if (ch->in_room != tchar1->in_room)
      {
        if (IS_FIGHTING(tchar1))
          stop_fighting(tchar1);
        if (IS_DESTROYING(tchar1))
          stop_destroying(tchar1);
      }
    }
    return TRUE;
  }
  /*
   * all dragons can cast spells, but need mana to do so, just like
   * other mobs do now.  Breath is gonna be best attack, almost for
   * sure, so we make sure we do that first, just to give them
   * something to worry about.  Dragons will either breathe or cast,
   * every round.
   */
  
     if (IS_AFFECTED(ch,AFF_CHARM) && (ch->following) &&
         (ch->in_room == ch->following->in_room))
       return FALSE;
#if 0
    if (GET_MANA(ch) == GET_MAX_MANA(ch)) {
      GET_MANA(ch)++;
      /* * ok, dragons ALL breathe of first round */
    } else
    {
#endif
#if 0
      if (!IS_SET(world[ch->in_room].room_flags, NO_MAGIC) &&
          !IS_SET(world[ch->in_room].room_flags, ROOM_SILENT))
      {
        /* * if they have any mana, they cast 1 in 3 */
        if ((GET_MANA(ch) > 9) && !number(0, 2))
        {
          if (!isname("br_f", GET_NAME(ch)) && !isname("br_c", GET_NAME(ch)) &&
              !isname("br_g", GET_NAME(ch)) && !isname("br_a", GET_NAME(ch)) &&
              !isname("br_l", GET_NAME(ch)) && !isname("br_s", GET_NAME(ch)) &&
              !isname("br_b", GET_NAME(ch)))
          {
            /*
             * Assign a valid character for CastMageSpell() -
             * SKB 9 Apr 1995
             */
            for (tchar1 = world[ch->in_room].people; tchar1;
                 tchar1 = tchar1->next_in_room)
              if (tchar1->specials.fighting == ch)
                return (CastMageSpell(ch, tchar1));
          }
        }
      } elsetell
#endif
#if 0
      {
        /* can't cast, so breathe like mad */
        breath_chance = 1;
      }
#endif

      /*
       * fail casting check?  Ok, let's add a sweep with the tail 1
       * in 6
       */
      
      if(!number(0, 3))
  	{
    	 if (!isname("br_f", GET_NAME(ch)) && !isname("br_c", GET_NAME(ch)) &&
        !isname("br_g", GET_NAME(ch)) && !isname("br_a", GET_NAME(ch)) &&
        !isname("br_l", GET_NAME(ch)) && !isname("br_s", GET_NAME(ch)) &&
        !isname("br_b", GET_NAME(ch)) && IS_DRAGON(ch))
    	  {
          if (IS_TITAN(ch) || IS_AVATAR(ch))
	   StompAttack(ch);
          else
	   SweepAttack(ch);
          return TRUE;
         }
        }
      
      /*
       * failing all that, they do nothing special 3 in 4, else they
       * breathe
       */
      if (number(0, breath_chance))
        return FALSE;
/*    }*/

    BreathWeapon(ch, -1);

    return TRUE;
  } 

//Dont know how the whole casting handler for casting dragons got yanked, but re-adding. Below is the code i've replaced with the above code. - Drannak
/*
if (!IS_DRAGON(ch) && !IS_TITAN(ch) && !IS_AVATAR(ch) && CAN_BREATHE(ch))
  {
    if (number(0, breath_chance)) // chance is backwards for non-dragons, we want them to only
    {                             // breathe on a 0... this is by design - Jexni 2/5/11
      return FALSE;
    }

    BreathWeapon(ch, -1);
    return TRUE;
  }

  if(!number(0, 3))
  {
    if (!isname("br_f", GET_NAME(ch)) && !isname("br_c", GET_NAME(ch)) &&
        !isname("br_g", GET_NAME(ch)) && !isname("br_a", GET_NAME(ch)) &&
        !isname("br_l", GET_NAME(ch)) && !isname("br_s", GET_NAME(ch)) &&
        !isname("br_b", GET_NAME(ch)))
    {
      if (IS_TITAN(ch) || IS_AVATAR(ch))
	StompAttack(ch);
      else
	SweepAttack(ch);
      return TRUE;
    }
  }

  if (bIsPet)
  {
    return FALSE;
  }
  
  if(IS_DRAGON(ch) && CAN_BREATHE(ch) && number(0, breath_chance))
  {
     BreathWeapon(ch, -1);
     return TRUE;
  }

  return TRUE;
}
*/
   /*
    *  This function exists to block other functions from being added to
    *  a mob during combat.  This is particularly useful for demons/devils
    *  who will be summoning other demons/devils of similiar kind who we do
    *  NOT want to be able to summon as well.  - Jexni  5/16/09
    */

int dummy_function(P_char ch, P_char vict, int cmd, char *arg)
{
   return FALSE;
}

int DemonCombat(P_char ch)
{
   if(CastClericSpell(ch, 0, FALSE))
     return TRUE;
   else
     return FALSE;

/*  int subtype = 0, i = 0;

  if (!mob_index[GET_RNUM(ch)].func.mob)
  {
    for(i;i < 14;i++)
    {
      if(isname(demon_types[i], GET_NAME(ch)))
      {
        subtype = i;
        break;
      }
    }

    switch(subtype)
    {
      case 0:
        mob_index[GET_RNUM(ch)].func.mob = babau_combat;
        break;
      case balor:
        mob_index[GET_RNUM(ch)].func.mob = balor_combat;
      case bebilith:
        mob_index[GET_RNUM(ch)].func.mob = bebilith_combat;
      case dretch:
        mob_index[GET_RNUM(ch)].func.mob = dretch_combat;
      case glabrezu:
        mob_index[GET_RNUM(ch)].func.mob = glabrezu_combat;
      case hezrou:
        mob_index[GET_RNUM(ch)].func.mob = hezrou_combat;
      case marilith:
        mob_index[GET_RNUM(ch)].func.mob = marilith_combat;
      case nalfeshnee:
        mob_index[GET_RNUM(ch)].func.mob = nalfeshnee_combat;
      case quasit:
        mob_index[GET_RNUM(ch)].func.mob = quasit_combat;
      case retriever:
        mob_index[GET_RNUM(ch)].func.mob = retriever_combat;
      case succubus:
        mob_index[GET_RNUM(ch)].func.mob = succubus_combat;
      case incubus:
        mob_index[GET_RNUM(ch)].func.mob = incubus_combat;
      case vrock:
        mob_index[GET_RNUM(ch)].func.mob = vrock_combat;  
    }

     commented out function assignments until all are done, skeleton
     for the function works  - Jexni 5/16/09
    
    return TRUE;
  }
  return FALSE;  */
}

int babau_combat(P_char ch, P_char vict, int cmd, char* arg)
{
   int last_sum = 0, curr_time;
   P_char target = vict;

   if(cmd == CMD_SET_PERIODIC)
     return TRUE;
 
   if(!ch || !IS_ALIVE(ch) || !IS_AWAKE(ch))
     return FALSE;

   if(!IS_SET(ch->specials.act, ACT_HUNTER))
     SET_BIT(ch->specials.act, ACT_HUNTER);

   if(!IS_SET(ch->specials.affected_by, AFF_SNEAK));
     SET_BIT(ch->specials.affected_by, AFF_SNEAK);

   if(ch->specials.fighting)
     target = ch->specials.fighting;

   if(!number(0, 20) && IS_FIGHTING(ch))
   {
     do_flee(ch, 0, 1);
     do_hide(ch, 0, 1);
     return FALSE;
   }
   else if(IS_FIGHTING(ch) && number(0, 100) < 40 && (last_sum + 1800 < curr_time))
   {
     summon_new_demon(ch, 1);
     last_sum = curr_time;
     return FALSE;
   }
   
   return FALSE;
}

bool DevilCombat(P_char ch)
{
  if (CastClericSpell(ch, 0, FALSE))
    return TRUE;
  else
    return FALSE;
}

int summon_new_demon(P_char ch, int subtype)
{
   P_char tmp;

   if(!ch || !IS_ALIVE(ch))
     return FALSE;

   if(!(tmp = read_mobile(1006, VIRTUAL)))
   {
      logit(LOG_EXIT, "assert: error in summon_new_demon()");
      raise(SIGSEGV);
   }
   else
   {
     tmp->player.level = BOUNDED(1, GET_LEVEL(ch) - number(0, 5), 62);
     convertMob(tmp);
     mob_index[GET_RNUM(tmp)].func.mob = dummy_function;
     act("$n &+rmakes a strange gesture, and a &+Lportal &+rto another plane opens!\r\n"
         "$N &+rsteps out of the portal.&n", FALSE, ch, 0, tmp, TO_ROOM);
     char_to_room(tmp, ch->in_room, 0);
     setup_pet(tmp, ch, -1, PET_NOAGGRO);
     return TRUE;
   }

   return FALSE;
}

