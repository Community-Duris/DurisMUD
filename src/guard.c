#include "structs.h"
#include "events.h"
#include "comm.h"
#include "spells.h"
#include "prototypes.h"
#include "defines.h"
#include "utils.h"

#include "guard.h"

extern P_room world;

P_char guarding(P_char ch)
{
  struct char_link_data *cld;

  for (cld = ch->linking; cld; cld = cld->next_linking)
  {
    if (cld->type == LNK_GUARDING)
      return cld->linked;
  }

  return NULL;
}

P_char guarding2(P_char ch, int n)
{
  int i = 0;
  struct char_link_data *cld;

  if (number_guarding(ch) < n)
    return NULL;

  for (cld = ch->linking; cld; cld = cld->next_linking)
  {
    if (cld->type == LNK_GUARDING)
      i++;

    if (i == n)
      return cld->linked;
  }
  return NULL;
}

P_char guarded_by(P_char ch)
{
  struct char_link_data *cld;

  for (cld = ch->linked; cld; cld = cld->next_linked)
  {
    if (cld->type == LNK_GUARDING)
      return cld->linking;
  }

  return NULL;
}

bool is_guarding(P_char guard, P_char target)
{
  if( is_linked_to(target, guard, LNK_GUARDING) )
    return TRUE;
  else
    return FALSE;
}

bool is_being_guarded(P_char ch)
{
  struct char_link_data *cld;

  for (cld = ch->linking; cld; cld = cld->next_linking)
  {
    if (cld->type == LNK_GUARDING)
      return TRUE;
  }

  return FALSE;
}

int number_guarding(P_char ch)
{
  struct char_link_data *cld;
  int i = 0;

  for (cld = ch->linking; cld; cld = cld->next_linking)
  {
    if (cld->type == LNK_GUARDING)
      i++;
  }
  return i;
}

void drop_one_guard(P_char ch)
{
  P_char tch;

  tch = guarding2(ch, number_guarding(ch));;
  unlink_char(ch, tch, LNK_GUARDING);

  return;
}


void do_guard(P_char ch, char *argument, int cmd)
{     
  P_char   target;
      
  if (!ch)
    return;
  
  if(IS_PC_PET(ch))
  {
    return;
  }
  
  if (!GET_CHAR_SKILL(ch, SKILL_GUARD))
  {
    send_to_char("You don't know how to guard!\n", ch);
    return;
  }
  
  if( isname(argument, "off") )
  {
    clear_links(ch, LNK_GUARDING);
    send_to_char("You are no longer guarding anyone.\n", ch);
    return;
  }

  target = get_char_room_vis(ch, argument);
  if (!target)
  {
    send_to_char("Guard who?\n", ch);
    return;
  }
  
  if (IS_NPC(target))
  {
    send_to_char("You can't guard mobs!\n", ch);
    return;
  }
 
  if( target == ch )
  {
    send_to_char("You can't guard yourself!\n", ch);
    return;
  }
 
  P_char guard = guarded_by(target);
  if( guard && guard != ch )
  {
    send_to_char("They are already being guarded!\n", ch);
    return;
  }
  else if( guard && guard == ch )
  {
    send_to_char("You are already guarding them!\n", ch);
    return;
  }

  if( GET_CHAR_SKILL(target, SKILL_GUARD) )
  {
    send_to_char("They can take care of themselves!\n", ch);
    return;
  }

  if( (CAN_MULTI_GUARD(ch)) &&
      (number_guarding(ch) >= (int)get_property("skill.guard.max.multi", 2)) )
  {
    send_to_char("You're guarding too many people, so you drop one.\n", ch);
    drop_one_guard(ch);
  }
  else if(!CAN_MULTI_GUARD(ch) && number_guarding(ch) >= (int)get_property("skill.guard.max", 1))
  {
    clear_links(ch, LNK_GUARDING);
  }
 
  link_char(ch, target, LNK_GUARDING);
  
  act("You are now guarding $N!", FALSE, ch, 0, target, TO_CHAR);
  act("$n is now guarding you!", FALSE, ch, 0, target, TO_VICT);

  return;
}

void guard_broken(struct char_link_data *cld)
{
  P_char ch = cld->linking;
  P_char target = cld->linked;

  act("You are no longer guarding $N", FALSE, ch, 0, target, TO_CHAR);
  act("You are no longer being guarded by $n.", FALSE, ch, 0, target, TO_VICT);
}

P_char guard_check(P_char attacker, P_char victim)
{
  P_char guard;

  if( !victim || !attacker )
  {
    return victim;
  }

  if( IS_TRUSTED(attacker) )
  {
    return victim;
  }
  
  if( attacker->specials.fighting == victim )
  {
    return victim;
  }
  
  guard = get_linking_char(victim, LNK_GUARDING);

  if( !guard || 
      guard->in_room != victim->in_room ||
      guard == attacker ||
      GET_POS(guard) != POS_STANDING ||
      GET_STAT(guard) != STAT_NORMAL ||
      !IS_AWAKE(guard) ||
      affected_by_spell(guard, SKILL_GUARD) ||
      IS_IMMOBILE(guard) ||
      IS_BLIND(guard))
  {
    return victim;
  }
  
  if(IS_SET(world[attacker->in_room].room_flags, SINGLE_FILE))
  {
    act("You attempt to dive in front of $N, but this place is too cramped!!!",
      FALSE, guard, 0, victim, TO_CHAR);
    clear_links(victim, LNK_GUARDING);
    return victim;
  }
  
  //debug("guard_check: %s vs %s protecting %s", attacker->player.name, guard->player.name, victim->player.name);
  
  int sneak_chance = BOUNDED( 0, (GET_CHAR_SKILL(attacker, SKILL_SNEAK) - GET_CHAR_SKILL(guard, SKILL_GUARD)), 100);
    
  if( sneak_chance > 0 )
  {
    //debug("attacker sneak skill: %d", sneak_chance);
    if( IS_AFFECTED(guard, AFF_AWARE) || IS_AFFECTED(guard, AFF_SKILL_AWARE) )
      sneak_chance -= 25;

    if( GET_C_LUK(victim) / 2 > number(0, 100) )
      sneak_chance -= 10;
  
    if( guard->specials.fighting )
      sneak_chance += 5;
 
    sneak_chance = BOUNDED(5, sneak_chance, 75);

    //debug("sneakchance: %d", sneak_chance);
    if( sneak_chance > 0 && number(0, 100) < sneak_chance )
    {
      //debug("snuck around guard");
      act("You sneak around $N's guard!", FALSE, attacker, 0, victim, TO_CHAR);
      return victim;
    }
  }

  int chance = GET_CHAR_SKILL(guard, SKILL_GUARD);
  //debug("guard skill: %d", chance);
 
  if( chance <= 0 )
  {
    logit(LOG_DEBUG, "guard_check(): somehow %s's guard got checked, but they don't have guard skill", guard->player.name);
    //debug("guard_check(): somehow %s's guard got checked, but they don't have guard skill", guard->player.name);
    return victim;
  }

  //debug("attacker agi: %d, guard agi: %d", GET_C_AGI(attacker), GET_C_AGI(guard));
  
  // agi mod
  chance += BOUNDED(-get_property("skill.guard.agiDiffMaxBonus", 5), 
                    (GET_C_AGI(guard) - GET_C_AGI(attacker)), 
                    get_property("skill.guard.agiDiffMaxBonus", 5) );

  // penalty if the guard is already engaged
  //if( guard->specials.fighting )
  //  chance -= get_property("skill.guard.guardFightingPenalty", 5);
  
  // and if the victim is lucky, they stay a bit safer :)
  if( GET_C_LUK(victim) / 2 > number(0, 100) )
    chance += 5;
  
  if( GET_C_LUK(attacker) / 2 > number(0, 100) )
    chance -= 5;

  if( IS_AFFECTED(guard, AFF_AWARE) || IS_AFFECTED(guard, AFF_SKILL_AWARE) )
    chance += 10;

  // always a chance to fail or succeed
  chance = BOUNDED(5, chance, 95);

  // Lets solve pets guarding too well?
  if(IS_NPC(guard) &&
     IS_PC_PET(guard))
  {
    chance = (int) (chance * get_property("skill.PC.PET.Guard", 0.750));
  }

  //debug("final chance: %d", chance);
  
  if( !notch_skill(guard, SKILL_GUARD, (int) get_property("skill.notch.guard", 17) )
    && number(0, 100) > chance )
  {
    //debug("guard failed");
    
    // guard failed
    act("You try to step in front of $N, but don't make it in time.", FALSE, guard,
        0, victim, TO_CHAR);
    act("$n tries to step in front of you, but doesn't make it in time.", FALSE, guard,
        0, victim, TO_VICT);
    act("$n tries to step in front of $N, but doesn't make it in time.", FALSE, guard,
        0, victim, TO_NOTVICT);
    
    return victim;
  } 
  else 
  {
    //debug("guard succeeded");

    // success!
    set_short_affected_by(guard, SKILL_GUARD, 1 * PULSE_VIOLENCE);
    
    act("You bravely step in front of $N, shielding $M from harm!", FALSE, guard,
        0, victim, TO_CHAR);
    act("$n bravely steps in front of you, shielding you from harm!", FALSE, guard,
        0, victim, TO_VICT);
    act("$n bravely steps in front of $N, shielding $M from harm!", FALSE, guard,
        0, victim, TO_NOTVICT);
    
    return guard;
  }

}
