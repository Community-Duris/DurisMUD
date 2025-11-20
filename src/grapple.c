#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <string>
#include <vector>
#include <list>
using namespace std;

#include "structs.h"
#include "spells.h"
#include "comm.h"
#include "interp.h"
#include "prototypes.h"
#include "utils.h"
#include "db.h"
#include "sql.h"
#include "events.h"
#include "grapple.h"
#include "damage.h"

extern P_room world;
extern int check_shields(P_char, P_char, int, int);

int grapple_check_entrapment(P_char ch)
{
  int gcskl, percent;
  P_char attacker;

  for (attacker = world[ch->in_room].people; attacker; attacker = attacker->next_in_room)
  {
    if (has_innate(attacker, INNATE_ENTRAPMENT) && 
        CAN_ACT(attacker) &&
        CAN_SEE(ch, attacker) && 
        (GET_POS(attacker) == POS_STANDING) && (GET_OPPONENT(ch) == attacker))
    {
      // Percent of a level 50 grappler with 100 skill in grappler combat is 35% to prevent flee
      gcskl = GET_CHAR_SKILL(attacker, SKILL_GRAPPLER_COMBAT);
      percent = (int)(((GET_LEVEL(attacker)/2) + (gcskl/10)) * (float)get_property("grapple.entrapment.chance.mod", 1.00));

      if (number(1, 100) < percent)
      {
        act("You try to flee, but $N grabs you and throws you back in the fight!", TRUE, ch, 0, attacker, TO_CHAR);
        act("$n tries to flee, but you grab $m and throw $m back into the fight!", TRUE, ch, 0, attacker, TO_VICT);
        act("$n tries to flee, but $N grabs $m and throws $m back into the fight!", TRUE, ch, 0, attacker, TO_NOTVICT);
        
        CharWait(ch, PULSE_VIOLENCE);

        return TRUE;
      }
    }
  }
  return FALSE;
}

int grapple_check_hands(P_char ch)
{
  if (ch->equipment[HOLD] ||
      ch->equipment[WIELD] ||
      ch->equipment[WIELD2])
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

int grapple_flee_check(P_char ch)
{
  P_char grappler, victim;

  if( !IS_FIGHTING(ch) )
  {
    return FALSE;
  }
  else
  {
    if(IS_BEARHUG(ch) || IS_HEADLOCK(ch))
      return TRUE;
  }

  return FALSE;
}

P_char grapple_attack_check(P_char ch)
{
  P_char grappler;

  grappler = get_linking_char(ch, LNK_GRAPPLED);
  if (!grappler)
  {
    grappler = get_linked_char(ch, LNK_GRAPPLED);
  }

  return grappler;
}

int grapple_misfire_chance(P_char ch, P_char victim, int type)
{
  int chance;

    if (IS_BEARHUG(victim) || IS_BEARHUG(grapple_attack_check(victim)))
    {
      chance = (int)get_property("grapple.misfire.bearhug", 30);
    }
    else if (IS_SHEADLOCK(victim) || IS_SHEADLOCK(grapple_attack_check(victim)))
    {
      chance = (int)get_property("grapple.misfire.headlock", 20);
    }
    else if (IS_SARMLOCK(victim) || IS_SARMLOCK(grapple_attack_check(victim)))
    {
      chance = (int)get_property("grapple.misfire.armlock", 10);
    }
    else if (IS_SLEGLOCK(victim) || IS_SLEGLOCK(grapple_attack_check(victim)))
    {
      chance = (int)get_property("grapple.misfire.leglock", 10);
    }
    else if (type == 1) // Someone is casting
    {
      chance = (int)get_property("grapple.misfire.casting", 50);
    }
    else
    {
      chance = 0;
    }

  return chance;
}

void do_bearhug(P_char ch, char *argument, int cmd)
{
  P_char victim;
  struct affected_type af;
  int percent, duration, type, gclvl;

  if (!GET_CHAR_SKILL(ch, SKILL_BEARHUG))
  {
    send_to_char("The best you could do is a great big warm hug.  How cute!\r\n", ch);
    return;
  }
  
  if (affected_by_spell(ch, SKILL_BEARHUG))
  {
    send_to_char("You haven't quite reoriented yourself for another attempt.\n", ch);
    return;
  }

  victim = ParseTarget(ch, argument);

  if (!victim)
  {
    send_to_char("Bearhug who?\r\n", ch);
    set_short_affected_by(ch, SKILL_BEARHUG, PULSE_VIOLENCE);
    return;
  }

  if (victim == ch)
  {
    send_to_char("You hug yourself.\r\n", ch);
    return;
  }
  
  if (!CAN_SEE(ch, victim))
  {
    send_to_char("You can't see anything, let alone someone to bearhug!\r\n", ch);
    return;
  }

  if (grapple_check_hands(ch) == FALSE)
  {
    send_to_char("You can't grapple while holding something!\r\n", ch);
    return;
  }

  if (GRAPPLE_TIMER(ch))
  {
    send_to_char("You need to regain your strength before you grapple again.\r\n", ch);
    return;
  }

  if (victim)
  {
    if (IS_TRUSTED(victim))
    {
      send_to_char("You're no match for a God!\r\n", ch);
      return;
    }
    
    if (IS_GRAPPLED(ch))
    {
      send_to_char("You're already in a hold!\r\n", ch);
      return;
    }

    if (IS_GRAPPLED(victim))
    {
      act("$E is already in a hold!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    if (IS_AFFECTED(victim, AFF_WRAITHFORM))
    {
      act("You're arms pass right through $M and you end up hugging yourself.", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
    
	if (IS_RIDING(victim))
	{
	  act("Bearhugging someone on a horse seems a bit silly, doesn't it?", TRUE, ch, 0, victim, TO_CHAR);
	  return;
	}

    if (GET_POS(victim) != POS_STANDING)
    {
      act("$E dosn't seem to be in a good position for that.", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(victim) + (int)get_property("grapple.bearhug.size.down", 1))) && IS_PC(victim))
    {
      send_to_char("They'd slip right through your arms.\r\n", ch);
      return;
    }
    if ((GET_ALT_SIZE(ch) < (GET_ALT_SIZE(victim) - (int)get_property("grapple.bearhug.size.up", 1))) && IS_PC(victim))
    {
      send_to_char("You couldn't get your arms around that to try.\r\n", ch);
      return;
    }

    if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(victim) + (int)get_property("grapple.bearhug.size.mob", 3))) && IS_NPC(victim))
    {
      send_to_char("They'd slip right through your arms.\r\n", ch);
      return;
    }
    if ((GET_ALT_SIZE(ch) < (GET_ALT_SIZE(victim) - (int)get_property("grapple.bearhug.size.mob", 3))) && IS_NPC(victim))
    {
      send_to_char("You couldn't get your arms around that to try.\r\n", ch);
      return;
    }

    if (!IS_HUMANOID(victim))
    {
      send_to_char("You can't seem to grab it.\r\n", ch);
      return;
    }

    // success chance
    // CHECK_MODIFY
    percent = BOUNDED(0, GET_CHAR_SKILL(ch, SKILL_BEARHUG), 100);
    gclvl = (int)(GET_CHAR_SKILL(ch, SKILL_GRAPPLER_COMBAT)/10);

    if( !IS_BEARHUG(victim) && (percent > number(1, 101)
      || notch_skill(ch, SKILL_BEARHUG, (int)get_property("skill.notch.bearhug", 12.5))
      || notch_skill(ch, SKILL_GRAPPLER_COMBAT, (int)get_property("skill.notch.grapplercombat", 17)) ))
    {
      if (BOUNDED(0, (number(0,12)-gclvl), 10))
      {
        type = TAG_BEARHUG;
        act("$n wraps his arms around you but you manage to keep your arms free!", TRUE, ch, 0, victim, TO_VICT);
        act("You wrap your arms around $N but $E manages to keep $M arms free!", TRUE, ch, 0, victim, TO_CHAR);
        act("$n wraps his arms around $N but $E manages to keep $M arms free!", TRUE, ch, 0, victim, TO_NOTVICT);
      }
      else
      {
        type = SKILL_BEARHUG;
        act("$n wraps his arms around you and squeezes with all his might!", TRUE, ch, 0, victim, TO_VICT);
        act("You wrap your arms around $N and squeeze with all your might!", TRUE, ch, 0, victim, TO_CHAR);
        act("$n wraps his arms around $N and squeezes with all his might!", TRUE, ch, 0, victim, TO_NOTVICT);
      }
      
      memset(&af, 0, sizeof(af));
      
      af.type = type;
      af.duration = (int)(PULSE_VIOLENCE * (float)get_property("grapple.bearhug.duration", 3.00));
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      linked_affect_to_char(victim, &af, ch, LNK_GRAPPLED);
      
      af.type = TAG_GRAPPLE;
      // the -3 is so the grapple can move into a combination hold.
      af.duration = (int)((PULSE_VIOLENCE * (float)get_property("grapple.bearhug.duration", 3.00))-3);
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      affect_to_char(ch, &af);
     
      add_event(event_bearhug, PULSE_VIOLENCE/2, ch, victim, 0, 0, 0, 0);
      
      engage( ch, victim );
      CharWait(ch, (int)(PULSE_VIOLENCE * (float)get_property("grapple.bearhug.duration", 3.00))-3);
      CharWait(victim, (int)(PULSE_VIOLENCE * (float)get_property("grapple.bearhug.duration.victim", 2.00)));
    }
    else
    {
      act("You try to grab $N in a bearhug but $E slips away.", TRUE, ch, 0, victim, TO_CHAR);
      act("$n tries to grab you in a bearhug but you manage to slip away.", TRUE, ch, 0, victim, TO_VICT);
      act("$n tries to grab $N in a bearhug but $E manages to slip away.", TRUE, ch, 0, victim, TO_NOTVICT);
      CharWait(ch, (int)(PULSE_VIOLENCE * (float)get_property("grapple.bearhug.duration", 3.00)));

      engage( ch, victim );
    }
  }
}

void event_bearhug(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type af;

  struct damage_messages messages = {
    "$N's face turns &+Rbright red&n as you squeeze the life out of $M.",
    "Your face turns &+Rbright red&n as $n squeezes the life out of you.",
    "$N's face turns &+Rbright red&n as $n squeezes the life out of $M.",
    "$N's head hunches over lifeless as your last squeeze does $M in!",
    "All goes blank as $n literally squeezes you to death!",
    "$N's head hunches over lifeless after $n squeezes to hard!"
  };

  if (!victim || !IS_ALIVE(victim) || !ch || !IS_ALIVE(ch) || !CanDoFightMove(ch, victim))
    return;

    
  if (!IS_TGRAPPLE(ch) || 
      !IS_BEARHUG(victim) || 
       (GET_POS(victim) != POS_STANDING) ||
       (GET_POS(ch) != POS_STANDING) ||
       (GET_STAT(ch) != STAT_NORMAL) ||
       (ch->in_room != victim->in_room))
  {
    // Not in a headlock combination
    if (!IS_SHEADLOCK(victim) && !IS_GROUNDSLAM(victim))
    {
      act("$n releases $s bearhug on you and lets go.", TRUE, ch, 0, victim, TO_VICT);
      act("You release your bearhug from around $N.", TRUE, ch, 0, victim, TO_CHAR);
      act("$n releases $s bearhug on $N.", TRUE, ch, 0, victim, TO_NOTVICT);
    
      if (IS_TGRAPPLE(ch))
      {
        affect_from_char(ch, TAG_GRAPPLE);
      }
      
      unlink_char(ch, victim, LNK_GRAPPLED);

      memset(&af, 0, sizeof(af));
      af.type = SKILL_GRAPPLER_COMBAT;
      af.duration = (WAIT_SEC * (int)get_property("grapple.cooldown.timer", 15));
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      affect_to_char(ch, &af);

      return;
    }
      
    if (IS_SBEARHUG(victim))
    {
      affect_from_char(victim, SKILL_BEARHUG);
    }
    if (IS_TBEARHUG(victim))
    {
      affect_from_char(victim, TAG_BEARHUG);
    }
    return;
  }
  else
  {
    int dam = BOUNDED(40, (int)(GET_LEVEL(ch) + (3 * (int)(GET_C_STR(ch) - GET_C_STR(victim))) * (float)get_property("grapple.bearhug.dmgmod", 1.00) * ((float)GET_CHAR_SKILL(ch, SKILL_BEARHUG) / 100)), 300);
    dam += number(-20, 20); 
    melee_damage(ch, victim, dam, PHSDAM_TOUCH, &messages);
    notch_skill(ch, SKILL_BEARHUG, (int)get_property("skill.notch.bearhug", 12.5));
    //check_shields(ch, victim, dam, RAWDAM_DEFAULT);
    if (IS_ALIVE(ch) && IS_ALIVE(victim))
      add_event(event_bearhug, PULSE_VIOLENCE/2, ch, victim, 0, 0, 0, 0);
  }
}

void do_headlock(P_char ch, char *argument, int cmd)
{
  P_char victim;
  struct affected_type af;
  int percent, duration, mod;

  if (!GET_CHAR_SKILL(ch, SKILL_HEADLOCK))
  {
    send_to_char("You don't know how!\r\n", ch);
    return;
  }

  victim = ParseTarget(ch, argument);

  if (!victim)
  {
    send_to_char("Headlock who?\r\n", ch);
    return;
  }

  if (victim == ch)
  {
    send_to_char("You choke yourself.\r\n", ch);
    return;
  }

  if (!CAN_SEE(ch, victim))
  {
    send_to_char("You can't see anything, let alone someone to headlock!\r\n", ch);
    return;
  }

  if (grapple_check_hands(ch) == FALSE)
  {
    send_to_char("You can't grapple while holding something!\r\n", ch);
    return;
  }
  
  if (GRAPPLE_TIMER(ch))
  {
    send_to_char("You need to regain your strength before you grapple again.\r\n", ch);
    return;
  }

  if (victim)
  {
    if (IS_TRUSTED(victim))
    {
      send_to_char("You're no match for a God!\r\n", ch);
      return;
    }
    
    // If im in some hold and im not doing the hold
    if (IS_GRAPPLED(ch) && !IS_TGRAPPLE(ch))
    {
      send_to_char("You are already in a hold!\r\n", ch);
      return;
    }
    // Otherwise we are doing the holding
    
    // If im in holding already and the hold on victim isn't mine
    if (IS_TGRAPPLE(ch) && (grapple_attack_check(ch) != victim))
    {
      send_to_char("Your busy holding someone else!\r\n", ch);
      return;
    }
        
    // If you have a hold on victim and it's not a bearhug.. you can't do the combination
    if (IS_GRAPPLED(victim) && !IS_BEARHUG(victim))
    {
      act("$E is already in a hold!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    // If the target is in a hold and it's not your hold
    if (IS_GRAPPLED(victim) && (grapple_attack_check(victim) != ch))
    {
      act("$E is already being held by someone else!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    if (IS_AFFECTED(victim, AFF_WRAITHFORM))
    {
      act("You're can't seem to grab ahold of their throat.", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
    
    if (GET_POS(victim) != POS_STANDING)
    {
      act("$E dosn't seem to be in a good position for that.", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(victim) + (int)get_property("grapple.headlock.size.down", 2))) && IS_PC(victim))
    {
      send_to_char("They'd slip right through your arms.\r\n", ch);
      return;
    }
    if ((GET_ALT_SIZE(ch) < (GET_ALT_SIZE(victim) - (int)get_property("grapple.headlock.size.up", 1))) && IS_PC(victim))
    {
      send_to_char("You couldn't get your arms around that to try.\r\n", ch);
      return;
    }

    if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(victim) + (int)get_property("grapple.headlock.size.mob", 3))) && IS_NPC(victim))
    {
      send_to_char("They'd slip right through your arms.\r\n", ch);
      return;
    }
    if ((GET_ALT_SIZE(ch) < (GET_ALT_SIZE(victim) - (int)get_property("grapple.headlock.size.mob", 3))) && IS_NPC(victim))
    {
      send_to_char("You couldn't get your arms around that to try.\r\n", ch);
      return;
    }

    if (!IS_HUMANOID(victim))
    {
      send_to_char("You can't seem to grab it.\r\n", ch);
      return;
    }

    // success chance.
    // CHECK_MODIFY
    percent = BOUNDED(0, GET_CHAR_SKILL(ch, SKILL_HEADLOCK), 100);

    if (!IS_HEADLOCK(victim) &&  ( (percent > number(1, 101))
      || notch_skill(ch, SKILL_HEADLOCK, (int)get_property("skill.notch.headlock", 17))
      || notch_skill(ch, SKILL_GRAPPLER_COMBAT, (int)get_property("skill.notch.grapplercombat", 17)) ))
    {
      if (IS_BEARHUG(victim))
      {
        if (IS_SBEARHUG(victim))
        {
          affect_from_char(victim, SKILL_BEARHUG);
        }
        if (IS_TBEARHUG(victim))
        {
          affect_from_char(victim, TAG_BEARHUG);
        }
        if (IS_GRAPPLED(ch))
        {
          affect_from_char(ch, TAG_GRAPPLE);
        }

        mod = HOLD_IMPROVED; // Combination, improved damage and chance to knock out
        act("$n releases his bearhug and wraps his arms around your neck and squeezes.", TRUE, ch, 0, victim, TO_VICT);
        act("You release your bearhug and wrap your arms around $N's neck and squeeze.", TRUE, ch, 0, victim, TO_CHAR);
        act("$n releases his bearhug and wraps his arms around $N's neck and squeezes.", TRUE, ch, 0, victim, TO_NOTVICT);
      }
      else
      {
        mod = HOLD_NORMAL; // normal
        act("$n wraps his arms around your neck and squeezes.", TRUE, ch, 0, victim, TO_VICT);
        act("You wrap your arms around $N's neck and squeeze.", TRUE, ch, 0, victim, TO_CHAR);
        act("$n wraps his arms around $N's neck and squeezes.", TRUE, ch, 0, victim, TO_NOTVICT);
      }
      
      memset(&af, 0, sizeof(af));
      
      af.type = SKILL_HEADLOCK;
      af.duration = (int)(PULSE_VIOLENCE * (float)get_property("grapple.headlock.duration", 2.00));
      af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
      af.modifier = mod;
      linked_affect_to_char(victim, &af, ch, LNK_GRAPPLED);
      
      af.type = TAG_GRAPPLE;
      affect_to_char(ch, &af);
      
      add_event(event_headlock, PULSE_VIOLENCE/2, ch, victim, 0, 0, 0, 0);
      
      CharWait(ch, (int)(PULSE_VIOLENCE * (float)get_property("grapple.headlock.duration", 2.00)));

      engage( ch, victim );
    }
    else
    {
      act("You try to catch $N in a headlock but $E slips out of it.", TRUE, ch, 0, victim, TO_CHAR);
      act("$n tries to cath you in a headlock but you manage to slip out of it.", TRUE, ch, 0, victim, TO_VICT);
      act("$n tries to catch $N in a headlock but $E manages to slip out of it.", TRUE, ch, 0, victim, TO_NOTVICT);
      CharWait(ch, (int)(PULSE_VIOLENCE * (float)get_property("grapple.headlock.duration", 2.00)));
      
      engage( ch, victim );
    }
  }
}

void event_headlock(P_char ch, P_char victim, P_obj obj, void *data)
{
  int type, percent, mod, gclvl;
  float damage;
  bool knockedout = FALSE;
  struct affected_type af, *aft;
  struct damage_messages messages = {
    "$N strains to &+Cbreathe&n as you squeeze the life out of $M.",
    "You strain to &+Cbreathe&n as $n squeezes the life out of you.",
    "$N strains to &+Cbreathe&n as $n squeezes the life out of $M.",
    "$N stops struggling and goes limp!",
    "You stop struggling for air as blackness ensues...",
    "$N stops struggling and goes limp!"
  };

  if (!victim || !IS_ALIVE(victim) || !ch || !IS_ALIVE(ch) || !CanDoFightMove(ch, victim) ||
      (ch->in_room != victim->in_room))
    return;

  if ((aft = get_spell_from_char(victim, SKILL_HEADLOCK)) != NULL)
  {
    if (aft->modifier == HOLD_NORMAL)
    {
      type = HOLD_NORMAL;
      damage = (float)get_property("grapple.headlock.dmgMultiplier.norm", 2.00);
    }
    else if (aft->modifier == HOLD_IMPROVED)
    {
      type = HOLD_IMPROVED;
      damage = (float)get_property("grapple.headlock.dmgMultiplier.combo", 3.00);
    }
  }
  else
  {
    type = HOLD_NONE;
  }

  if (!type || (GET_POS(victim) != POS_STANDING) || (GET_POS(ch) != POS_STANDING) || GET_STAT(ch) != STAT_NORMAL)
  {
    act("$n releases $s headlock on you.", TRUE, ch, 0, victim, TO_VICT);
    act("You release your headlock from around $N's neck.", TRUE, ch, 0, victim, TO_CHAR);
    act("$n releases $s headlock on $N.", TRUE, ch, 0, victim, TO_NOTVICT);

    if (IS_TGRAPPLE(ch))
    {
      affect_from_char(ch, TAG_GRAPPLE);
    }
    if (IS_SHEADLOCK(victim))
    {
      affect_from_char(victim, SKILL_HEADLOCK);
    }
    unlink_char(ch, victim, LNK_GRAPPLED);
  
    memset(&af, 0, sizeof(af));
    af.type = SKILL_GRAPPLER_COMBAT;
    af.duration = (WAIT_SEC * (int)get_property("grapple.cooldown.timer", 15));
    af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
    affect_to_char(ch, &af);
  
    return;
  }
  else
  {
    gclvl = GET_CHAR_SKILL(ch, SKILL_GRAPPLER_COMBAT)/20;
    percent = BOUNDED(0, (GET_C_STR(ch) - GET_C_STR(victim)), 5+gclvl) + number(0, 2);
    
    if ((type == HOLD_IMPROVED) && (percent >= number(1, 100)) && (GET_LEVEL(ch) >= 51))
    {
      act("You give in finally from the lack of oxygen and pass out!", TRUE, ch, 0, victim, TO_VICT);
      act("You squeezed $N's neck so hard $E passed out!", TRUE, ch, 0, victim, TO_CHAR);
      act("$n releases $N as $E stops struggling and goes limp.", TRUE, ch, 0, victim, TO_NOTVICT);
      
      if (IS_TGRAPPLE(ch))
      {
        affect_from_char(ch, TAG_GRAPPLE);
      }
      if (IS_SHEADLOCK(victim))
      {
        affect_from_char(victim, SKILL_HEADLOCK);
      }
      unlink_char(ch, victim, LNK_GRAPPLED);

      stop_fighting(victim);
      if( IS_DESTROYING(victim) )
        stop_destroying(victim);
      StopMercifulAttackers(victim);

      memset(&af, 0, sizeof(af));
     
      af.type = SKILL_GRAPPLER_COMBAT;
      af.duration = (WAIT_SEC * (int)get_property("grapple.cooldown.timer", 15));
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      affect_to_char(ch, &af);
     
      af.type = TAG_HEADLOCK;
      af.bitvector = AFF_KNOCKED_OUT;
      af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
      af.duration = (WAIT_SEC * (int)get_property("grapple.headlock.duration.knockout", 10));

      affect_to_char_with_messages(victim, &af,
        "&+GThe world seems to come back into focus.",
        "$n seems to have come back to his senses.");

      SET_POS(victim, POS_PRONE + GET_STAT(victim));
      
      knockedout = TRUE;
      
      return;
    }
    else
    {
      knockedout = FALSE;
    }

    if (!knockedout)
    {
    	int dam = (int)((GET_C_DEX(ch)/7)*damage);
      raw_damage(ch, victim, dam, RAWDAM_DEFAULT, &messages);
      notch_skill(ch, SKILL_HEADLOCK, (int)get_property("skill.notch.headlock", 17));
      check_shields(ch, victim, dam, RAWDAM_DEFAULT);
      if (IS_ALIVE(ch) && IS_ALIVE(victim))
        add_event(event_headlock, PULSE_VIOLENCE/2, ch, victim, 0, 0, 0, 0);
    }
  }
}

// Due to fight.c's function calling this in a wierd way, we name the characters
// attacker and grappler to clear things up here.
void armlock_check(P_char attacker, P_char grappler)
{
  int reflextype, percent, str, gclvl, dam;
  struct affected_type af;

  struct damage_messages messages = {
    "You grab $N's arm and bend it the wrong way!",
    "$n grabs your arm and bends it the wrong way!",
    "$n grabs $N's arm in an armlock!",
    "You bend $N's arm with all your might and it snaps in half!",
    "The pain is too much to bare as your arm snaps in half!",
    "The pain was too much to bare for $N as $S arm snaps in half!"
  };
  
  struct damage_messages breakmsg = {
    "$N's arm snaps from the strain you've placed on it!",
    "Your arm snaps under the preasure $n placed on it!",
    "$N's arm snaps under the preasure $n placed on it!",
    "You bend $N's arm with all your might and it snaps in half!",
    "The pain is too much to bare as your arm snaps in half!",
    "The pain was too much to bare for $N as $S arm snaps in half!"
  };
  
  if (!GET_CHAR_SKILL(grappler, SKILL_ARMLOCK))
  {
    return;
  }
  
  if (!CAN_SEE(grappler, attacker))
  {
    return;
  }

  if (grapple_check_hands(grappler) == FALSE)
  {
    return;
  }

  if (IS_GRAPPLED(grappler) || IS_GRAPPLED(attacker))
  {
    return;
  }
  
  if (!IS_HUMANOID(attacker))
  {
    return;
  }

  gclvl = GET_CHAR_SKILL(grappler, SKILL_GRAPPLER_COMBAT)/10;
  percent = GET_CHAR_SKILL(grappler, SKILL_ARMLOCK);
  
  if( ((percent > number(1, 101)) && !number(0, 20-gclvl))
    || notch_skill(grappler, SKILL_ARMLOCK, (int)get_property("skill.notch.armlock", 17))
    || notch_skill(grappler, SKILL_GRAPPLER_COMBAT, (int)get_property("skill.notch.grapplercombat", 17)) )
  {
    reflextype = number(0, ARMREFLEX_MAX);
  }
  else
  {
    return;
  }
  
  memset(&af, 0, sizeof(af));
  str = (int)(GET_C_STR(grappler)/10);
  bool brokenarm = FALSE;

  switch (reflextype)
  {
    case ARMREFLEX_HOLD:
      dam = (int)(GET_C_DEX(grappler)/15*4*(float)get_property("grapple.armlock.dmgmod", 1.00));
      raw_damage(grappler, attacker, dam, RAWDAM_DEFAULT, &messages);
      check_shields(grappler, attacker, dam, RAWDAM_DEFAULT);
      
      if (!IS_ALIVE(grappler) || !IS_ALIVE(attacker))
        return;
     
      struct affected_type *afp;
      for (afp = attacker->affected; afp; afp = afp->next)
      {
        if (afp->type == TAG_ARMLOCK)
           brokenarm = TRUE;
      }
 
      if ((number(1, 100) <= (int)get_property("grapple.armlock.break.chance", 10)) && (GET_LEVEL(grappler) >= 51)
           && (brokenarm == FALSE))
      {
        af.type = TAG_ARMLOCK;
        af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL;
        af.duration = -1;
        affect_to_char(attacker, &af);
        
        int dam = (int)(((GET_C_DEX(grappler)/10)+str)*4*(float)get_property("grapple.armlock.break.dmgmod", 1.00));
        raw_damage(grappler, attacker, dam, RAWDAM_DEFAULT, &breakmsg);
        check_shields(grappler, attacker, dam, RAWDAM_DEFAULT);
      
        if (!IS_ALIVE(grappler) || !IS_ALIVE(attacker))
          return;
      }
      else
      {
        af.type = SKILL_ARMLOCK;
        af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
        af.duration = (int)(PULSE_VIOLENCE * (float)get_property("grapple.armlock.duration", 1.00));
        af.modifier = HOLD_NORMAL;
        linked_affect_to_char(attacker, &af, grappler, LNK_GRAPPLED);

        af.type = TAG_GRAPPLE;
        af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
        af.duration = (int)(PULSE_VIOLENCE * (float)get_property("grapple.armlock.duration", 1.00));
        affect_to_char(grappler, &af);
        
        add_event(event_armlock, 0, grappler, attacker, 0, 0, 0, 0);
      }
      break;

    case ARMREFLEX_OFFBALANCE:
      if (GET_POS(attacker) != POS_STANDING)
      {
        break;
      }
 
      act("You grab and pull $N's arm sending him off balance.", TRUE, grappler, 0, attacker, TO_CHAR);
      act("$n grabs your arm and pulls, sending you off balance!", TRUE, grappler, 0, attacker, TO_VICT);
      act("$n grabs $N's arm and pulls, sending him off balance!", TRUE, grappler, 0, attacker, TO_NOTVICT);
      
      af.type = SKILL_ARMLOCK;
      af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
      af.duration = (int)(PULSE_VIOLENCE * (float)get_property("grapple.armlock.duration.offbal", 2.00));
      af.modifier = HOLD_IMPROVED;
      linked_affect_to_char(attacker, &af, grappler, LNK_GRAPPLED);
      
      break;

    default:
      break;
  }  
}

void event_armlock(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *af;

  if (!victim || !IS_ALIVE(victim) || !ch || !IS_ALIVE(ch) || !CanDoFightMove(ch, victim))
    return;
  
  if ((ch->in_room != victim->in_room) ||
      !IS_SARMLOCK(victim) || !IS_TGRAPPLE(ch))
  {
    if ((af = get_spell_from_char(victim, SKILL_ARMLOCK)) != NULL)
    {
      if (af->modifier == HOLD_NORMAL)
      {
        act("You let go of $N's arm.", TRUE, ch, 0, victim, TO_CHAR);
        act("$n lets go of your arm.", TRUE, ch, 0, victim, TO_VICT);
      }
    }
         
    if (IS_TGRAPPLE(ch))
    {
      affect_from_char(ch, TAG_GRAPPLE);
    }
    if (IS_SARMLOCK(victim))
    {
      affect_from_char(victim, SKILL_ARMLOCK);
    }
    unlink_char(ch, victim, LNK_GRAPPLED);
  }
  else
  {
    add_event(event_armlock, 0, ch, victim, 0, 0, 0, 0);
  }
}

void grapple_heal(P_char ch)
{
  if (IS_TARMLOCK(ch))
  {
    affect_from_char(ch, TAG_ARMLOCK);
    send_to_char("Your arm is mended to health.\n", ch);
  }
  if (IS_TLEGLOCK(ch))
  {
    affect_from_char(ch, TAG_LEGLOCK);
    send_to_char("Your leg is mended to health.\n", ch);
  }
}

void do_leglock(P_char ch, char *argument, int cmd)
{
  P_char victim;
  struct affected_type af;
  int percent, duration, mod, lag;
  
  if (!GET_CHAR_SKILL(ch, SKILL_LEGLOCK))
  {
    send_to_char("You don't know how!\r\n", ch);
    return;
  }

  victim = ParseTarget(ch, argument);

  if (!victim)
  {
    send_to_char("Leglock who?\r\n", ch);
    return;
  }

  if (GRAPPLE_TIMER(ch))
  {
    send_to_char("You need to regain your strength before you grapple again.\r\n", ch);
    return;
  }

  if (victim == ch)
  {
    send_to_char("You grab ahold of your legs.\r\n", ch);
    return;
  }

  if (!CAN_SEE(ch, victim))
  {
    send_to_char("You can't see anything, let alone someone to leglock!\r\n", ch);
    return;
  }

  if (GET_POS(ch) > POS_SITTING)
  {
    send_to_char("You're not in the correct position for that.\r\n", ch);
    return;
  }

  if (grapple_check_hands(ch) == FALSE)
  {
    send_to_char("You can't grapple while holding something!\r\n", ch);
    return;
  }

  if (victim)
  {
    if (IS_TRUSTED(victim))
    {
      send_to_char("You're no match for a God!\r\n", ch);
      return;
    }
    
    // If im in some hold and im not doing the hold
    if (IS_GRAPPLED(ch) && !IS_TGRAPPLE(ch))
    {
      send_to_char("You are already in a hold!\r\n", ch);
      return;
    }
    // Otherwise we are doing the holding
    
    // If im in holding already and the hold on victim isn't mine
    if (IS_TGRAPPLE(ch))
    {
      send_to_char("Your busy holding someone!\r\n", ch);
      return;
    }
        
    // If you have a hold on victim and it's not a bearhug.. you can't do the combination
    if (IS_GRAPPLED(victim))
    {
      act("$E is already in a hold!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    if (IS_AFFECTED(victim, AFF_WRAITHFORM) || IS_IMMATERIAL(victim))
    {
      act("They don't appear to be solid enough for grabbing.", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
    
    if (GET_POS(victim) == POS_STANDING)
    {
      act("$E dosn't seem to be in a good position for that.", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(victim) + (int)get_property("grapple.leglock.size.down", 2))) && IS_PC(victim))
    {
      send_to_char("Those legs are too small for you to grapple.\r\n", ch);
      return;
    }
    if ((GET_ALT_SIZE(ch) < (GET_ALT_SIZE(victim) - (int)get_property("grapple.leglock.size.up", 1))) && IS_PC(victim))
    {
      send_to_char("Those legs are too big for you to grapple.\r\n", ch);
      return;
    }
    
    if ((GET_ALT_SIZE(ch) > (GET_ALT_SIZE(victim) + (int)get_property("grapple.leglock.size.mob", 3))) && IS_NPC(victim))
    {
      send_to_char("Those legs are too small for you to grapple.\r\n", ch);
      return;
    }
    if ((GET_ALT_SIZE(ch) < (GET_ALT_SIZE(victim) - (int)get_property("grapple.leglock.size.mob", 3))) && IS_NPC(victim))
    {
      send_to_char("Those legs are too big for you to grapple.\r\n", ch);
      return;
    }
    
    if (!IS_HUMANOID(victim))
    {
      send_to_char("You can't seem to grab it.\r\n", ch);
      return;
    }

    percent = BOUNDED(0, GET_CHAR_SKILL(ch, SKILL_LEGLOCK), 100);
    lag = 0;
    
    if( (percent > number(1, 101))
      || notch_skill(ch, SKILL_LEGLOCK, (int)get_property("skill.notch.leglock", 17))
      || notch_skill(ch, SKILL_GRAPPLER_COMBAT, (int)get_property("skill.notch.grapplercombat", 17)) )
    {
      if (!IS_GROUNDSLAM(victim) || (grapple_attack_check(victim) != ch))
      {
        mod = HOLD_NORMAL;
      }
      if (IS_GROUNDSLAM(victim) && (grapple_attack_check(victim) == ch))
      {
        mod = HOLD_IMPROVED;
        lag += 1;
      }
        
      act("You grab $N's legs and twist them around yours.", TRUE, ch, 0, victim, TO_CHAR);
      act("$n grabs your legs and twists them around $s.", TRUE, ch, 0, victim, TO_VICT);
      act("$n grabs $N's legs and twists them around $s.", TRUE, ch, 0, victim, TO_NOTVICT);

      memset(&af, 0, sizeof(af));
      af.type = SKILL_LEGLOCK;
      af.duration = (int)(PULSE_VIOLENCE * ((float)get_property("grapple.leglock.duration", 2.00)+lag));
      af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
      af.modifier = mod;
      linked_affect_to_char(victim, &af, ch, LNK_GRAPPLED);
      CharWait(victim, (int)(PULSE_VIOLENCE * (float)get_property("grapple.leglock.duration", 2.00)+lag));

      af.type = TAG_GRAPPLE;
      affect_to_char(ch, &af);
      CharWait(ch, (int)(PULSE_VIOLENCE * ((float)get_property("grapple.leglock.duration", 2.00)+lag)));
      
      add_event(event_leglock, PULSE_VIOLENCE/2, ch, victim, 0, 0, 0, 0);

      engage( ch, victim );
    }
    else
    {
      act("You grab $N's legs but $E manages to kick you away.", TRUE, ch, 0, victim, TO_CHAR);
      act("$n grabs your legs but you manage to kick $m away.", TRUE, ch, 0, victim, TO_VICT);
      act("$n grabs $N's legs but $E manages to kick $m away.", TRUE, ch, 0, victim, TO_NOTVICT);
      CharWait(ch, PULSE_VIOLENCE * (2+lag));

      engage( ch, victim );
    }
  }
}

void event_leglock(P_char ch, P_char victim, P_obj obj, void *data)
{
  int type, percent, mod, gclvl, str, legbreak;
  float damage;
  bool legbroke = FALSE;
  struct affected_type af, *aft;
  
  struct damage_messages messages = {
    "$N winces in pain as you twist $S leg.",
    "You wince in pain as $n twists your leg.",
    "$N winces in pain as $n twists $S leg.",
    "The pain in $N's leg proved too much to bare!",
    "The pain in your leg proved too much to bare...",
    "The pain in $N's leg proved too much to bare!",
  };

  struct damage_messages breakmsg = {
    "$N's leg snaps from the strain you place on it!",
    "Your leg snaps from the strain $n places on it!",
    "$N's leg snaps from the strain $n places on it!",
    "The pain in $N's leg proved too much to bare!",
    "The pain in your leg proved too much to bare...",
    "The pain in $N's leg proved too much to bare!",
  };

  if (!victim || !IS_ALIVE(victim) || !ch || !IS_ALIVE(ch) || !CanDoFightMove(ch, victim))
    return;

  if ((aft = get_spell_from_char(victim, SKILL_LEGLOCK)) != NULL)
  {
    if (aft->modifier == HOLD_NORMAL)
    {
      type = HOLD_NORMAL;
      damage = (float)get_property("grapple.leglock.dmgMultiplier.norm", 2.00);
    }
    else if (aft->modifier == HOLD_IMPROVED)
    {
      type = HOLD_IMPROVED;
      damage = (float)get_property("grapple.leglock.dmgMultiplier.combo", 3.00);
    }
  }
  else
  {
    type = HOLD_NONE;
  }

  bool brokenleg = FALSE;

  if ((!type) ||
      (GET_POS(victim) == POS_STANDING) ||
      (GET_POS(ch) == POS_STANDING) ||
      (GET_STAT(ch) != STAT_NORMAL))
  {
    act("$n releases $s leglock on you.", TRUE, ch, 0, victim, TO_VICT);
    act("You release your leglock from $N.", TRUE, ch, 0, victim, TO_CHAR);
    act("$n releases $s leglock on $N.", TRUE, ch, 0, victim, TO_NOTVICT);

    if (IS_TGRAPPLE(ch))
    {
      affect_from_char(ch, TAG_GRAPPLE);
    }
    if (IS_SLEGLOCK(victim))
    {
      affect_from_char(victim, SKILL_LEGLOCK);
    }
    unlink_char(ch, victim, LNK_GRAPPLED);
    
    memset(&af, 0, sizeof(af));
    af.type = SKILL_GRAPPLER_COMBAT;
    af.duration = (WAIT_SEC * (int)get_property("grapple.cooldown.timer", 15));
    af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
    affect_to_char(ch, &af);

    return;
  }
  else
  {
    gclvl = GET_CHAR_SKILL(ch, SKILL_GRAPPLER_COMBAT)/20;
    percent = BOUNDED(0, (GET_C_STR(ch) - GET_C_STR(victim)), 5+gclvl) + number(0, 2);
    str = (int)(GET_C_STR(ch)/10);

    struct affected_type *afp;
    for (afp = victim->affected; afp; afp = afp->next)
    {
      if (afp->type == TAG_ARMLOCK)
         brokenleg = TRUE;
    }
    if ((type == HOLD_IMPROVED) && (percent >= number(1, 100)) && (GET_LEVEL(ch) >= 51) && (brokenleg == FALSE))
    {
      
      if (IS_TGRAPPLE(ch))
      {
        affect_from_char(ch, TAG_GRAPPLE);
      }
      if (IS_SLEGLOCK(victim))
      {
        affect_from_char(victim, SKILL_LEGLOCK);
      }
      unlink_char(ch, victim, LNK_GRAPPLED);

      memset(&af, 0, sizeof(af));
      af.type = TAG_LEGLOCK;
      af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL;
      af.duration = -1;
      affect_to_char(victim, &af);

      int dam = (int)(((GET_C_DEX(ch)/10)+str)*4*(float)get_property("grapple.leglock.break.dmgmod", 1.00));
      raw_damage(ch, victim, dam, RAWDAM_DEFAULT, &breakmsg);
      notch_skill(ch, SKILL_LEGLOCK, (int)get_property("skill.notch.leglock", 17));
      check_shields(ch, victim, dam, RAWDAM_DEFAULT);

      if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
          return;

      af.type = SKILL_GRAPPLER_COMBAT;
      af.duration = (WAIT_SEC * (int)get_property("grapple.cooldown.timer", 15));
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      affect_to_char(ch, &af);
      
      legbreak = TRUE;
      
      return;
    }
    else
    {
      legbreak = FALSE;
    }

    if (!legbreak)
    {
      int dam = (int)((GET_C_DEX(ch)/7)*damage);
      raw_damage(ch, victim, dam, RAWDAM_DEFAULT, &messages);
      notch_skill(ch, SKILL_LEGLOCK, (int)get_property("skill.notch.leglock", 17));
      check_shields(ch, victim, dam, RAWDAM_DEFAULT);
      if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
          return;
      add_event(event_leglock, PULSE_VIOLENCE/2, ch, victim, 0, 0, 0, 0);
    }
  }
}

void do_groundslam(P_char ch, char *argument, int cmd)
{
  int percent, offbalance = FALSE;
  P_char victim;
  struct affected_type af, *aft;

  struct damage_messages messages = {
    "You pick up $N and SLAM $M to the ground!",
    "$n picks you up and SLAMS you on the ground!",
    "$n picks up $N and SLAMS $M on the ground!",
    "You pick up $N and SLAM $M to the ground!",
    "$n picks you up and SLAMS you on the ground!",
    "$n picks up $N and SLAMS $M on the ground!"
  };
  
  if (!GET_CHAR_SKILL(ch, SKILL_GROUNDSLAM))
  {
    send_to_char("You don't know how!\r\n", ch);
    return;
  }

  victim = ParseTarget(ch, argument);

  if (!victim)
  {
    send_to_char("Groundslam who?\r\n", ch);
    return;
  }

  if (victim == ch)
  {
    send_to_char("Groundslam yourself?  I think not.\r\n", ch);
    return;
  }

  if (!CAN_SEE(ch, victim))
  {
    send_to_char("You can't see anything, let alone someone to groundslam!\r\n", ch);
    return;
  }

  if (grapple_check_hands(ch) == FALSE)
  {
    send_to_char("You can't grapple while holding something!\r\n", ch);
    return;
  }

  if (victim)
  {
    if (IS_TRUSTED(victim))
    {
      send_to_char("You're no match for a God!\r\n", ch);
      return;
    }
    
    if (IS_GRAPPLED(ch) && !IS_TGRAPPLE(ch))
    {
      send_to_char("You're in a hold!\r\n", ch);
      return;
    }

    if ((aft = get_spell_from_char(victim, SKILL_ARMLOCK)) != NULL)
    {
      if (aft->modifier == HOLD_IMPROVED)
      {
        offbalance = TRUE;
      }
    }
    
    if (!IS_BEARHUG(victim) && !offbalance)
    {
      send_to_char("You don't have a good hold for that.\r\n", ch);
      return;
    }
    
    if (IS_BEARHUG(victim) && (grapple_attack_check(victim) != ch))
    {
      send_to_char("You don't have a good hold for that.\r\n", ch);
      return;
    }

    if (GET_POS(victim) != POS_STANDING)
    {
      act("$E dosn't seem to be in a good position for that.", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }

    if (GET_ALT_SIZE(ch) > (GET_ALT_SIZE(victim) + (int)get_property("grapple.groundslam.size.down", 2)))
    {
      send_to_char("You'd hurt yourself falling on top of that!\r\n", ch);
      return;
    }

    if ((GET_ALT_SIZE(ch) < (GET_ALT_SIZE(victim) - (int)get_property("grapple.groundslam.size.up", 2))) || has_innate(victim, INNATE_HORSE_BODY) || has_innate(victim, INNATE_SPIDER_BODY))
    {
      send_to_char("They'd squash you if you tried!\r\n", ch);
      return;
    }

    if (!IS_HUMANOID(victim))
    {
      send_to_char("You can't seem to grab it.\r\n", ch);
      return;
    }

    percent = GET_CHAR_SKILL(ch, SKILL_GROUNDSLAM);

    if( (number(1, 101) < percent)
      || notch_skill(ch, SKILL_GROUNDSLAM, (int)get_property("skill.notch.groundslam", 17))
      || notch_skill(ch, SKILL_GRAPPLER_COMBAT, (int)get_property("skill.notch.grapplercombat", 17)) )
    {
      // Add weight based damge here
      int dam = (int)((GET_WEIGHT(ch)/5)*(float)get_property("grapple.groundslam.dmgmod", 1.00));
      raw_damage(ch, victim, dam, RAWDAM_DEFAULT, &messages);
      check_shields(ch, victim, dam, RAWDAM_DEFAULT);
      if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
        return;
      SET_POS(ch, POS_PRONE + GET_STAT(ch));
      CharWait(ch, (int)(PULSE_VIOLENCE * (float)get_property("grapple.groundslam.duration", 2.00)-0.5));
      SET_POS(victim, POS_PRONE + GET_STAT(victim));
      CharWait(victim, (int)(PULSE_VIOLENCE * (float)get_property("grapple.groundslam.duration", 2.00)));
      
      memset(&af, 0, sizeof(af));
      af.type = SKILL_GROUNDSLAM;
      af.duration = (int)(PULSE_VIOLENCE * (float)get_property("grapple.groundslam.duration", 2.00));
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      linked_affect_to_char(victim, &af, ch, LNK_GRAPPLED);

      if (IS_SARMLOCK(victim))
      {
        affect_from_char(victim, SKILL_ARMLOCK);
      }
      if (IS_SBEARHUG(victim))
      {
        affect_from_char(victim, SKILL_BEARHUG);
      }
      if (IS_TBEARHUG(victim))
      {
        affect_from_char(victim, TAG_BEARHUG);
      }
      return;
    }
    else
    {
      act("In an attempt to slam $N to the ground, you buckle under the weight and fall over.", TRUE, ch, 0, victim, TO_CHAR);
      act("In an attempt to slam you to the ground, $n buckles under the weight and falls over.", TRUE, ch, 0, victim, TO_VICT);
      act("In an attempt to slam $N on the ground, $n buckles under the weight and falls over.", TRUE, ch, 0, victim, TO_NOTVICT);
      SET_POS(ch, POS_PRONE + GET_STAT(ch));
      CharWait(ch, (int)(PULSE_VIOLENCE * (float)get_property("grapple.groundslam.duration", 2.00)));
      return;
    }
  }  
}
