/***************************************************************************
 *  File: breath_weapons.c                                   Part of Duris *
 *  Usage: procedures for breath weapon attacks.                           *
 ***************************************************************************/

#include "prototypes.h"
#include "utils.h"
#include "damage.h"
#include "spells.h"
#include "comm.h"

float breath_dam_mod;
float breath_saved_multiplier;

// Same as spell_fire_breath, which should be removed eventually.
void breath_weapon_fire(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 8 ) + level) );

  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }

  if( NewSaves(victim, SAVING_BREATH, save) )
  {
    dam = (int) ((float)dam * breath_saved_multiplier);
  }
  // 5 - 250 damage.
  dam = BOUNDED(20, dam, 1000);

  if( spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) != DAM_NONEDEAD )
  {
    return;
  }

#if FALSE // Disabling item damage/drop code
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
#endif
}

// Same as spell_lightning_breath, which should be removed eventually.
void breath_weapon_lightning(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 8 ) + level) );
  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }
  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) ((float)dam * breath_saved_multiplier);

  if( IS_AFFECTED2(victim, AFF2_PROT_LIGHTNING) )
  {
    dam = (int) ((float)dam * .80);
  }

  dam = BOUNDED(20, dam, 1000);

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages);
}

// Same as spell_frost_breath, which should be removed eventually.
void breath_weapon_frost(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 8 ) + level) );
  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }

  if( NewSaves(victim, SAVING_BREATH, save) )
  {
    dam = (int) ((float)dam * breath_saved_multiplier);
  }
  dam = BOUNDED(20, dam, 1000);

  if( spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) != DAM_NONEDEAD )
  {
    return;
  }

#if FALSE // Disabling damage/drop code.
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
#endif
}

// Same as spell_acid_breath, which should be removed eventually.
void breath_weapon_acid(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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
  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 12 ) + level) );
  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }

  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) ((float)dam * breath_saved_multiplier);

  dam = BOUNDED(20, dam, 1000);

  if(IS_AFFECTED2(victim, AFF2_PROT_ACID) &&
    number(0, 4))
      return;

  if(spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) !=
      DAM_NONEDEAD)
    return;

#if 0
  // And now for the damage on equipment
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
        act("&+L$p corrodes.", FALSE, victim, victim->equipment[damaged], 0, TO_CHAR);
        GET_AC(victim) -= apply_ac(victim, damaged);
        victim->equipment[damaged]->value[0] -= number(1, 7);
        GET_AC(victim) += apply_ac(victim, damaged);
        victim->equipment[damaged]->cost = 0;
      }
    }
  }
#endif
}

// Same as spell_gas_breath, which should be removed eventually.
void breath_weapon_poison(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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
  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 2 ) + dice( level, 4 ) + level) );

  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }

  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) ((float)dam * breath_saved_multiplier);
  dam = BOUNDED(20, dam, 1000);

  if( spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD )
  {
    if( !IS_AFFECTED2(victim, AFF2_PROT_GAS) || !number(0, 4) )
      spell_poison(level, ch, 0, 0, victim, 0);
  }
}

// Just calling spell_sleep for now.
void breath_weapon_sleep(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_sleep( level, ch, arg, type, victim, obj );
}

// Just calling spell_fear for now.
void breath_weapon_fear(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_fear( level, ch, arg, type, victim, obj );
}

// Just calling spell_minor_paralysis for now.
void breath_weapon_paralysis(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_minor_paralysis( level, ch, arg, type, victim, obj );
}

// Same as spell_shadow_breath_1, which should be removed eventually.
void breath_weapon_shadow_1(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 5 ) + level) );
  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }

  if(NewSaves(victim, SAVING_BREATH, save))
    dam = (int) ((float)dam * breath_saved_multiplier);

  dam = BOUNDED(20, dam, 1000);

  spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages);
}

// Same as spell_shadow_breath_2, which should be removed eventually.
void breath_weapon_shadow_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 7 ) + level) );
  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }

  if( NewSaves(victim, SAVING_BREATH, save) )
    dam = (int) ((float)dam * breath_saved_multiplier);

  dam = BOUNDED(20, dam, 1000);

  if( !NewSaves(victim, SAVING_SPELL, save) )
  {
    spell_enervation(level, ch, NULL, 0, victim, obj);
    return;
  }
  spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages);
}

// Same as spell_blinding_breath, which should be removed eventually.
void breath_weapon_blind(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (( (7 * level) / 2 ) + dice( 4, 10 )) );
  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }
  if( NewSaves(victim, SAVING_BREATH, save) )
    dam = (int) ((float)dam * breath_saved_multiplier);;

  dam = BOUNDED(20, dam, 1000);

  if( spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) != DAM_NONEDEAD )
  {
    return;
  }

  if( (level < 0 || !NewSaves(victim, SAVING_BREATH, save))
    && !resists_spell(ch, victim) && !IS_TRUSTED(victim)
    && !affected_by_spell(victim, SPELL_BLINDNESS) )
  {
    act("&+gThe toxic gas seems to have blinded $n&n!", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&+gYour eyes sting as the toxic gas blinds you!\n", victim);
    blind(ch, victim, 60 * WAIT_SEC);
  }
}

// Same as spell_crimson_light, which should be removed eventually.
void breath_weapon_crimson(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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
//  P_obj    burn = NULL;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 9 ) + level) );
  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }

  if( NewSaves(victim, SAVING_BREATH, save) )
    dam = (int) ((float)dam * breath_saved_multiplier);

  dam = BOUNDED(20, dam, 1000);

  if( spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) != DAM_NONEDEAD )
  {
    return;
  }

#if FALSE // Disabling item damage/drop code
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
#endif
}

// Same as spell_jasper_light, which should be removed eventually.
void breath_weapon_jasper(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 7 ) + level) );
  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }
  if( NewSaves(victim, SAVING_BREATH, save) )
    dam = (int) ((float)dam * breath_saved_multiplier);

  dam = BOUNDED(20, dam, 1000);

  if( spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD )
  {
    if( !IS_AFFECTED2(victim, AFF2_PROT_GAS) || !number(0, 4) )
      spell_poison(level, ch, 0, 0, victim, 0);
  }
}

// Same as spell_azure_light, which should be removed eventually.
void breath_weapon_azure(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 8 ) + level) );

  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }
  if( NewSaves(victim, SAVING_BREATH, save) )
    dam = (int) ((float)dam * breath_saved_multiplier);

  dam = BOUNDED(20, dam, 1000);

  if( IS_AFFECTED2(victim, AFF2_PROT_LIGHTNING) )
  {
    dam = (int) ((float)dam * .80);
  }

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages);
}

// Same as spell_basalt_light, which should be removed eventually.
void breath_weapon_basalt(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
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

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  save = BREATH_WEAPON_SAVE( ch, victim );
  dam = (int) ( breath_dam_mod * (dice( level, 11 ) + level) );

  if( IS_PC_PET(ch) )
  {
    dam /= 4;
  }
  if( NewSaves(victim, SAVING_BREATH, save) )
    dam = (int) ((float)dam * breath_saved_multiplier);

  dam = BOUNDED(20, dam, 1000);

  if( IS_AFFECTED2(victim, AFF2_PROT_ACID) )
  {
    dam = (4 * dam) / 5;
  }

  spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_BREATH | SPLDAM_NODEFLECT, &messages);
}

// Under construction...
void breath_weapon_crimson_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_crimson_light(level, ch, arg, type, victim, obj);
}

// Under construction...
void breath_weapon_azure_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_azure_light(level, ch, arg, type, victim, obj);
}

// Under construction...
void breath_weapon_jasper_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_jasper_light(level, ch, arg, type, victim, obj);
}

// Under construction...
void breath_weapon_basalt_2(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  spell_basalt_light(level, ch, arg, type, victim, obj);
}

void update_breath_weapon_properties()
{
  breath_dam_mod = get_property("dragon.Breath.DamageMod", 1.0);
  breath_saved_multiplier = get_property("dragon.Breath.savedDamage", 0.5);
}
