/****************************************************************************
 *
 *  File: fight.c                                            Part of Duris
 *  Usage: Combat system and messages.
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <set>

#include "sql.h"
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
#include "utility.h"
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "weather.h"
#include "sound.h"
#include "damage.h"
#include "objmisc.h"
#include "world_quest.h"
#include "paladins.h"
#include "epic.h"
#include "reavers.h"
#include "necromancy.h"
#include "disguise.h"
#include "grapple.h"
#include "map.h"
#include "dreadlord.h"
#include "outposts.h"
#include "boon.h"
#include "ctf.h"
#include "tether.h"
#include "achievements.h"
#include "siege.h"
#include "vnum.obj.h"
/*
 * external variables //
 */

extern P_char character_list;
extern P_event current_event;
extern P_desc descriptor_list;
extern P_event event_type_list[];
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char debug_mode;
extern const struct race_names race_names_table[];
extern void set_long_description(P_obj t_obj, const char *newDescription);
extern void set_short_description(P_obj t_obj, const char *newDescription);
extern void event_wait(P_char ch, P_char victim, P_obj obj, void *data);
void release_mob_mem(P_char ch, P_char victim, P_obj obj, void *data);

//extern const int material_absorbtion[][];
extern const struct stat_data stat_factor[];
extern float fake_sqrt_table[];
extern int pulse;
extern int arena_hometown_location[];
extern struct arena_data arena;
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern const int class_tohit_mod[];
extern Skill skills[];
extern long new_exp_table[];  // Arih: Fixed type mismatch bug - was int, should be long
extern struct wis_app_type wis_app[];

int pv_common(P_char, P_char, const P_obj);
bool monk_superhit(P_char, P_char);
extern char *arena_death_msg(P_obj p_weapon);
extern void send_to_arena(char *msg, int race);
extern int get_numb_chars_in_group(struct group_list *group);
extern int num_group_members_in_room(P_char ch);
extern int get_number_allies_in_room(P_char ch, int room_index);
extern P_char misfire_check(P_char ch, P_char spell_target, int flag);
extern bool check_reincarnate(P_char ch);
bool     handle_imprison_damage(P_char, P_char, int);
extern bool innate_two_daggers(P_char);
extern bool divine_blessing_parry(P_char, P_char);
extern int get_honing(P_obj);
extern void apply_honing(P_obj, int);
extern void holy_crusade_check(P_char, P_char);
extern int is_wearing_necroplasm(P_char);
extern int top_of_zone_table;

/* Structures */

//extern struct mm_ds *wdead_trophy_pool;
P_char   combat_list = 0;       /* head of l-list of fighting chars  */
P_char   destroying_list = 0;   /* head of l-list of destroying chars  */
P_char   combat_next_ch = 0;    /* Next in combat global trick    */
float    dam_factor[LAST_DF + 1];
float    racial_spldam_offensive_factor[LAST_RACE + 1][LAST_SPLDAM_TYPE];
float    racial_spldam_defensive_factor[LAST_RACE + 1][LAST_SPLDAM_TYPE];

#define SOUL_TRAP_SINGLE 0
#define SOUL_TRAP_GROUP 1

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] = {
  {"punch", "punches", "punched"},              /* TYPE_HIT      */
  {"bludgeon", "bludgeons", "bludgeoned"},      /* TYPE_BLUDGEON */
  {"pierce", "pierces", "pierced"},             /* TYPE_PIERCE   */
  {"slash", "slashes", "slashed"},              /* TYPE_SLASH    */
  {"whip", "whips", "whipped"},                 /* TYPE_WHIP     */
  {"claw", "claws", "clawed"},                  /* TYPE_CLAW     */
  {"bite", "bites", "bitten"},                  /* TYPE_BITE     */
  {"sting", "stings", "stung"},                 /* TYPE_STING    */
  {"crush", "crushes", "crushed"},              /* TYPE_CRUSH    */
  {"maul", "mauls", "mauled"},                  /* TYPE_MAUL     */
  {"thrash", "thrashes", "thrashed"}            /* TYPE_THRASH   */
};

struct melee_death_messages
{
  char    *attacker;
  char    *victim;
  char    *room;
} melee_death_messages_table[] =
{
  // HIT
  {"Your fearsome punch hits $N, smashing $M to death.",
    "$n's final punch sends you to meet your maker.",
    "$n punches $N to death."},
  {"You hit $N in the throat, $E chokes, gasps and dies.",
    "$n smashes your throat.  Gasping and choking, you descend into darkness.",
    "$n punches $N in the throat, $E chokes, gasps and dies."},
  // BLUDGEON
  {"You bludgeon $N, killing $M instantly.",
    "You are bludgeoned to death by $n.",
    "$n ruthlessly bludgeons $N to death."},
  {"Your wicked bludgeon crushed $N to death.",
    "A wicked bludgeon from $n sends you spiraling into darkness.",
    "A blood spray erupts from $N as $E is bludgeoned to death by $n."},
  // PIERCE
  {"You successfully stab $N in the heart, killing $M instantly.",
    "$n stabs you directly in the heart, killing you instantly.",
    "$n stabs the heart of $N who falls to the ground clutching at the wound."},
  {"Your stab cuts a vital artery and causes $N to fall before your feet.",
    "$n's stab causes massive blood-loss and you fall to the ground, lifeless.",
    "$n stabs viciously at $N, whose lifeless body falls to the ground."},
  // SLASH
  {"You beautifully slash $N into two parts - both dead.",
    "Your upper body is disconnected from your legs as $n slashes you.",
    "$N is slashed into two by a master stroke performed by $n."},
  {"Your final slash sends $N's head bouncing along the ground.",
    "Your vision spins into darkness as your head falls towards the ground.",
    "$n neatly performs a coup de grace and beheads $N."},
  // WHIP
  {"Your whip opens a fatal wound and $N falls to the ground dead.",
    "$n's lashing whip causes immense blood-loss, effectively killing you.",
    "With a final stunning whip, $n ends $N's life."},
  {"The crack of your whip is the last thing $N hears before death.",
    "A stinging whip from $n spills your entrails onto the ground.",
    "The gash opened up by $n's whip fatally wounds $N."},
  // CLAW
  {
    "Your claw opens a fatal wound, and $N gives up the fight for good.",
    "$n's claw finds your vital organs and rips them out, killing you instantly.", 
    "With a final swipe of a claw, $n ends $N's life for good."},
  {"You swipe a vicious claw at $N, causing a fatal gash.",
    "A fearsome swipe of a claw from $n ends your existence.",
    "A final fearsome swipe of a claw from $n kills $N quickly."},
  // BITE
  {"Your bite opens a fatal wound, and $N gives up the fight for good.",
    "$n's final bite crushes your throat.",
    "With a final chomp, $n ends $N's life."},
  {"You bite down hard upon $N and have the satisfaction of killing $M!",
    "A final bite from $n chews through you and your life slips away.",
    "$n nearly chomps $N in half with a wicked bite, gruesomely killing $M."},
  // STING
  {"Your sting finally reached a vital organ killing $N instantly.",
    "You begin to blackout as overwhelming pain expands outward from $n's sting.",
    "$n's sting reached $N's heart killing $M instantly."},
  {
    0, 0, 0},
  // CRUSH
  {"Your final crushing blow caves in $N's chest, and $E dies rather quickly.",
    "$n crushes your chest, and all your hopes of escaping this battle alive.",
    "Snapping ribs can be heard as $n caves in $N's chest with a fearsome blow."},
  {"You crushed $N's skull, I'm afraid $E's dead.",
    "$n's crushing blow to your head makes you see stars, then nothing.",
    "$n crushes $N's skull.  $N instantly collapses, lifeless."},
  // MAUL
  {"Your maul hits $N squarely, smashing the life from $M.",
    "$n's final devastating maul sends you to meet your maker.",
    "$n smashes $N to death with a vicious maul."},
  {
    0, 0, 0},
  // THRASH
  {"$N's chest falls into itself as your hooves land on it!",
    "$n's hooves crush your chest completely.",
    "$N's chest falls into itself as $n's hooves land on it!"},
  {
    0, 0, 0},
  // TOUCH
  {"$N's life is drained away by your supernatural touch!",
    "$n's touch drains the last life force from your body!",
    "$N's touch leaves behind a lifeless husk of what once was $n!"},
};

/* Location of attack texts */
struct attack_hit_type location_hit_text[] = {
  {"", " body", ""},            /* extras represent percent chance */
  {"", " body", ""},
  {"", " body", ""},
  {"", " body", ""},
  {" on $S left leg", " left leg", ""},
  {" on $S right leg", " right leg", ""},
  {" on $S left arm", " left arm", ""},
  {" on $S right arm", " right arm", ""},
  {" on $S shoulder", " shoulder", ""},
  {" on $S chest", " chest", ""},
  {" on $S chest", " chest", ""},
  {" on $S stomach", " stomach", ""},
  {" on $S head", " head", ""}
};

/* items worn on those slots by the victim will be checked for defensive procs,
   order is important - if weapon absorbs, ioun cant deflect etc. */
int      proccing_slots[] = {
  PRIMARY_WEAPON,
  SECONDARY_WEAPON,
  THIRD_WEAPON,
  FOURTH_WEAPON,
  WEAR_SHIELD,
  WEAR_HANDS,
  WEAR_HANDS_2,
  WEAR_BODY,
  WEAR_ABOUT,
  WEAR_IOUN,
  WEAR_HEAD,
  WEAR_HORN,
  WEAR_NECK_1,
  WEAR_NECK_2,
  WEAR_ARMS,
  WEAR_ARMS_2,
  WEAR_WRIST_LR,
  WEAR_WRIST_LL,
  WEAR_FINGER_R,
  WEAR_FINGER_L,
  WEAR_NOSE,
  WEAR_TAIL,
  WEAR_LEGS,
  WEAR_FEET,
  WEAR_EYES,
  WEAR_FACE,
  WEAR_EARRING_R,
  WEAR_EARRING_L,
  WEAR_WAIST,
  WEAR_QUIVER,
  GUILD_INSIGNIA
};

int      on_front_line(P_char);

// LAST_SPLDAM_TYPE == number of types, range 0 .. LAST_SPLDAM_TYPE-1
const char *spldam_types[LAST_SPLDAM_TYPE] = {
  "generic",
  "fire",
  "cold",
  "lightning",
  "gas",
  "acid",
  "negative",
  "holy",
  "psionic",
  "spiritual",
  "sound",
  "earth"
};

void update_racial_dam_factors()
{
  char buf[256];
  int race, type;

  // Skip RACE_NONE.
  for( race = 1; race <= LAST_RACE; race++ )
  {
    for( type = 0; type < LAST_SPLDAM_TYPE; type++ )
    {
      snprintf(buf, 256, "damage.spellTypeMod.offensive.racial.%s.%s", race_names_table[race].no_spaces, spldam_types[type]);
      racial_spldam_offensive_factor[race][type] = get_property(buf, 1.00);

      snprintf(buf, 256, "damage.spellTypeMod.defensive.racial.%s.%s", race_names_table[race].no_spaces, spldam_types[type]);
      racial_spldam_defensive_factor[race][type] =  get_property(buf, 1.00);
    }
  }
}

/* The Fight related routines */
void update_dam_factors()
{
  dam_factor[DF_SWASHBUCKLER_DEFENSE] = get_property("damage.reduction.swashbuckler", 0.800);
  dam_factor[DF_SWASHBUCKLER_OFFENSE] = get_property("damage.increase.swashbuckler", 1.25);
  dam_factor[DF_SANC] = get_property("damage.reduction.sanctuary", 0.8);
  dam_factor[DF_TROLLSKIN] = get_property("damage.reduction.trollskin", 0.8);
  dam_factor[DF_BARKSKIN] = get_property("damage.reduction.barkskin", 0.90);
  dam_factor[DF_BERSERKMELEE] = get_property("damage.reduction.berserk", 0.10);
  dam_factor[DF_SOULMELEE] = get_property("damage.reduction.soulshield.melee", 0.8);
  dam_factor[DF_SOULSPELL] = get_property("damage.reduction.soulshield.spell", 0.8);
  dam_factor[DF_NEG_SHIELD_SPELL] = get_property("damage.reduction.negshield.spell", 0.8);
  dam_factor[DF_PROTLIVING] = get_property("damage.reduction.protLiving", 0.95);
  dam_factor[DF_PROTANIMAL] = get_property("damage.reduction.protAnimal", 0.8);
  dam_factor[DF_PROTECTION] = get_property("damage.reduction.protElement", 0.75);
  dam_factor[DF_PROTECTION_TROLL] = get_property("damage.reduction.protFire.Troll", 0.90);
  dam_factor[DF_ELSHIELDRED_TROLL] = get_property("damage.reduction.fireColdShield.Troll", 0.80);
  dam_factor[DF_ELSHIELDRED] = get_property("damage.reduction.fireColdShield", 0.55);
  dam_factor[DF_IRONWILL] = get_property("damage.reduction.towerOfIronWill", 0.5);
  dam_factor[DF_TIGERPALM] = get_property("damage.reduction.tigerpalm", 0.65);
  dam_factor[DF_ELAFFINITY] = get_property("damage.reduction.elementalAffinity", 0.25);
  dam_factor[DF_COLDWRITHE] = get_property("damage.increase.coldWrithe", 2.0);
  dam_factor[DF_BARKFIRE] = get_property("damage.increase.barkskin", 1.15);
  dam_factor[DF_IRONWOOD] = get_property("damage.increase.ironwood", 1.80);
  dam_factor[DF_BERSERKSPELL] = get_property("damage.increase.berserk", 1.100);
  dam_factor[DF_BERSERKEREXTRA] = get_property("damage.increase.berserk.Berserkers", 1.100);
  dam_factor[DF_ELEMENTALIST] = get_property("damage.increase.elementalist", 1.15);
  dam_factor[DF_ELSHIELDINC] = get_property("damage.increase.fireColdShield", 2.0);
  dam_factor[DF_PHANTFORM] = get_property("damage.increase.phantasmalForm", 1.100);
  dam_factor[DF_VULNCOLD] = get_property("damage.increase.vulnCold", 2.0);
  dam_factor[DF_VULNFIRE] = get_property("damage.increase.vulnFire", 1.15);
  dam_factor[DF_ELSHIELDDAM] = get_property("damage.shield.fireCold", 0.5);
  dam_factor[DF_NEGSHIELD] = get_property("damage.shield.neg", 0.25);
  dam_factor[DF_SOULSHIELDDAM] = get_property("damage.shield.soul", 0.2);
  dam_factor[DF_MONKVAMP] = get_property("vamping.vampiricTouch.monk", 0.05);
  dam_factor[DF_TOUCHVAMP] = get_property("vamping.vampiricTouch", 0.4);
  dam_factor[DF_TRANCEVAMP] = get_property("vamping.vampiricTrance", 0.2);
  dam_factor[DF_HFIREVAMP] = get_property("vamping.hellfire", 0.14);
  dam_factor[DF_UNDEADVAMP] = get_property("vamping.innateUndead", 0.03);
  dam_factor[DF_NPCVAMP] = get_property("vamping.undeadNpc", 0.1);
  dam_factor[DF_NPCTOPC] = get_property("damage.modifier.npcToPc", 1.0);
  dam_factor[DF_WEAPON_DICE] = get_property("damage.modifier.weaponDice", 1.0);
  dam_factor[DF_WETFIRE] = get_property("damage.reduction.wet", 0.8);
  dam_factor[DF_BATTLETIDEVAMP] = get_property("vamping.battletide", 0.125);
  dam_factor[DF_SLSHIELDINCREASE] = get_property("damage.soulnegshield.increase", 1.5);
  dam_factor[DF_CHAOSSHIELD] = get_property("damage.reduction.chaosshield.mod", 0.90);
  dam_factor[DF_BERSERKRAGE] = get_property("damage.increase.berserk.rage", 1.350);
  dam_factor[DF_RAGED] = get_property("damage.increase.rage", 2.000);
  dam_factor[DF_ENERGY_CONTAINMENT] = get_property("damage.reduction.EnergyContainment", 0.750);
  dam_factor[DF_GUARDIANS_BULWARK] = get_property("damage.reduction.guardians.bulwark", 0.850);
  dam_factor[DF_DAMROLL_MOD] = get_property("damroll.mod", 1.0);
  dam_factor[DF_MELEEMASTERY] = get_property("damage.modifier.meleemastery", 1.100);
  dam_factor[DF_DRACOLICHVAMP] = get_property("vamping.dracolich", 0.500);
  dam_factor[DF_NEG_AC_MULT] = get_property("damage.neg.armorclass.multiplier", 0.500);
  dam_factor[DF_DODGE_AGI_MODIFIER] = get_property("damage.dodge.agi.multiplier", 1.500);
  dam_factor[DF_ARROWVAMP] = get_property("vamping.vampiricTouch.arrow", 0.05);
  dam_factor[DF_ANTIPALADINVAMP] = get_property("vamping.vampiricTouch.antipaladin", 0.05);
  dam_factor[DF_MERCENARYVAMP] = get_property("vamping.vampiricTouch.mercenary", 0.100);
  dam_factor[DF_WARRIORVAMP] = get_property("vamping.vampiricTouch.warrior", 0.100);
  dam_factor[DF_BERSERKERVAMP] = get_property("vamping.vampiricTouch.berserker", 0.100);
  dam_factor[DF_ROGUEVAMP] = get_property("vamping.vampiricTouch.rogue", 0.100);
  dam_factor[DF_PALADINVAMP] = get_property("vamping.vampiricTouch.paladin", 0.100);
  dam_factor[DF_RANGERVAMP] = get_property("vamping.vampiricTouch.ranger", 0.100);
  dam_factor[DF_DLORDAVGRVAMP] = get_property("vamping.vampiricTouch.dreadlord.or.avenger", 0.100);
  dam_factor[DF_GOOD_MODIFIER] = get_property("damage.modifier.good", 1.000);
  dam_factor[DF_EVIL_MODIFIER] = get_property("damage.modifier.evil", 1.000);
  dam_factor[DF_UNDEAD_MODIFIER] = get_property("damage.modifier.undead", 1.000);
  dam_factor[DF_NEUTRAL_MODIFIER] = get_property("damage.modifier.neutral", 1.000);
  dam_factor[DF_TWOHANDED_MODIFIER] = get_property("damage.modifier.twohanded", 1.500);
  dam_factor[DF_KNEELING] = get_property("damage.modifier.kneeling", 1.150);
  dam_factor[DF_SITTING] = get_property("damage.modifier.sitting", 1.300);
  dam_factor[DF_PRONE] = get_property("damage.modifier.prone", 1.500);

}

// The swashbuckler is considered the victim. // May09 -Lucrot
bool rapier_dirk(P_char victim, P_char attacker) 
{
  int chance, i, f;

  // The assumption made here is that the victim has 100 agi and wis.
  chance = ((GET_C_AGI(victim) + GET_C_WIS(victim)) / 2 + GET_LEVEL(victim));

  // Rapier_dirk_check() verifies the correct weapons in each hand. We now confirm
  // there are two weapon objects.
  P_obj wep1 = victim->equipment[PRIMARY_WEAPON];
  P_obj wep2 = victim->equipment[SECONDARY_WEAPON];

  // Off-hand special riposte
  if( rapier_dirk_check(victim) && chance > number(1, 1000) && MIN_POS(victim, POS_STANDING + STAT_NORMAL)
    && !IS_DRAGON(attacker) && GET_OPPONENT(victim) == attacker )
  {
    if(number(0, 1))
    {
      if(GET_CLASS(victim, CLASS_BARD) || GET_SPEC(victim, CLASS_ROGUE, SPEC_THIEF))
      {
        act("$n's $q &=LCflashes&n into the path of $N's attack, then $n delivers a graceful counter-attack!",
            TRUE, victim, wep1, attacker, TO_NOTVICT);
        act("With &=LWunbelievable speed&n, $n's $q intercepts your attack, and then $n steps wide to deliver a graceful counter-attack!",
            TRUE, victim, wep1, attacker, TO_VICT);
        act("With superb grace and ease, you intercept $N's attack with $q and counter-attack!",
            TRUE, victim, wep1, attacker, TO_CHAR);
      }
      else
      {
        act("$n's $q &=LCflashes&n into the path of $N's attack, then $n delivers a graceful counter-attack!",
            TRUE, victim, wep2, attacker, TO_NOTVICT);
        act("With &=LWunbelievable speed&n, $n's $q intercepts your attack, and then $n steps wide to deliver a graceful counter-attack!",
            TRUE, victim, wep2, attacker, TO_VICT);
        act("With superb grace and ease, you intercept $N's attack with $q and counter-attack!",
            TRUE, victim, wep2, attacker, TO_CHAR);
      }
    }
    else
    {
      act("$n's intercepts $N's attack with $q with eloquent ease.",
          TRUE, victim, wep1, attacker, TO_NOTVICT);
      act("$n's skillfully shifts $s stance and $q &+Wglistens&n with insane speed to thwart your attack!",
          TRUE, victim, wep1, attacker, TO_VICT);
      act("You play with $N before lunging into an offensive routine!",
          TRUE, victim, wep1, attacker, TO_CHAR);
    }

    hit(victim, attacker, wep1);
    if( !IS_ALIVE(victim) || !IS_ALIVE(attacker) )
    {
      return TRUE;
    }

    //if(wep1->craftsmanship == OBJCRAFT_HIGHEST &&
    if(!number(0, 3) && CanDoFightMove(victim, attacker))
    {
      act("$p slices through the air with incredible ease!",
          TRUE, victim, wep1, attacker, TO_CHAR);
      act("$p slices through the air with incredible ease!",
          TRUE, victim, wep1, attacker, TO_ROOM);

      f = number(1, (int) (GET_LEVEL(victim) / 28));

      for (i = 0; i < f && IS_ALIVE(victim) && IS_ALIVE(attacker); i++)
      {
        hit(victim, attacker, wep1);
      }
    }
    else
    {
      hit(victim, attacker, wep1);
    }

    return TRUE;
  }

  return FALSE;
}

bool opposite_racewar(P_char ch, P_char victim)
{
  return IS_PC(ch) && IS_PC(victim) && GET_RACEWAR(ch) != GET_RACEWAR(victim);
}

int vamp(P_char ch, double fhits, double fcap)
{
  struct affected_type *af;
  static char buf[16];
  P_char   tch;
  struct group_list *gl;
  int hits = (int) fhits, cap = (int) fcap, blocked;

  if( !IS_ALIVE(ch) )
  {
    return 0;
  }

  if(affected_by_spell(ch, TAG_BUILDING))
    return 0;

  if(hits <= 0)
  {
    return 0;
  }

  if( (af = get_spell_from_char(ch, SPELL_PLAGUE)) && !IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE) )
  {
    blocked = (int) (MIN(hits * (((float) number(30, 40)) / 100), af->modifier));
    hits -= blocked;
    if(af->modifier <= blocked)
    {
      affect_remove(ch, af);
      wear_off_message(ch, af);
    }
    else
    {
      af->modifier -= blocked;
    }
  }
  else if( IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE) )
  {
    blocked = (int) (hits * (((float)number(50, 60)) / 100));
    hits -= blocked;
  }
  else if( af = get_spell_from_char(ch, SPELL_BMANTLE) )
  {
    blocked = (int) (MIN(hits * (GET_LEVEL(ch) / 100), af->modifier));
    hits -=blocked;
    if( af->modifier <= blocked )
    {
      affect_remove(ch, af);
      wear_off_message(ch, af);
    }
    else
    {
      af->modifier -= blocked;
    }
  }

  if( IS_PC(ch) && IS_AFFECTED3(ch, AFF3_PALADIN_AURA) && has_aura(ch, AURA_HEALING) )
  {
    // This stops the massive healing when used with the following: Nov08 -Lucrot
    if( !IS_SET(ch->specials.affected_by4, AFF4_REGENERATION) && !affected_by_spell(ch, SPELL_ACCEL_HEALING)
      && !affected_by_skill(ch, SKILL_REGENERATE) )
    {
      hits += (int) (hits * ((get_property("innate.paladin_aura.healing_mod", 0.2) * aura_mod(ch, AURA_HEALING) ) / 100 ));
    }
  }

  hits = MAX(0, MIN(hits, cap - GET_HIT(ch)));
  GET_HIT(ch) = GET_HIT(ch) + hits;

  /* if(hits > 1)
     {
  //snprintf(buf, 16, "%s healed: %d\n", GET_NAME(ch), hits);
  // only send buf if it actually filled
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
#ifndef TEST_MUD
if (IS_TRUSTED(tch) && IS_SET(tch->specials.act2, PLR2_HEAL))
#endif
  //send_to_char(buf, tch);
  }*/

  //Client
  for( gl = ch->group; gl; gl = gl->next )
  {
    if( gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP )
    {
      gl->ch->desc->last_group_update = 1;
    }
  }

  return hits;
}

void heal(P_char ch, P_char healer, int hits, int cap)
{
  P_char   victim;
  int exp;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_HARDCORE(healer))
  {
    hits = (int) (hits * 1.20);
  }

  if(get_spell_from_char(ch, SPELL_BMANTLE))
  {
    hits = (int) (hits * get_property("blackmantle.healing.mod", .75));
  }
  if(IS_AFFECTED3(healer, AFF3_ENHANCE_HEALING))
  {
    hits = (int) (hits * get_property("enhancement.healing.mod", 1.2));
  }
  /*
     if(affected_by_spell(ch, TAG_NOMISFIRE)) //misfire code - drannak
     {
     hits = hits;
  //send_to_char("damage output normal\r\n", ch);
  }
  else
  {
  hits = (hits * .5);
  //send_to_char("&+Rdamage output halved\r\n", ch);
  }
  */
  hits = vamp(ch, hits, cap);
  update_achievements(healer, ch, hits, 1);

  // debug("Hitting heal function in fight with (%d) hits.", hits);
  if(IS_PC(healer) && IS_FIGHTING(ch))
  {
    gain_exp(healer, ch, hits, EXP_HEALING);
  }

}

bool soul_trap(P_char ch, P_char victim)
{
  int hps = GET_LEVEL(victim) * 2 * GET_CHAR_SKILL(ch, SKILL_SOUL_TRAP) / 100;
  bool himself = false;
  struct group_list *gl;

  if (GET_CHAR_SKILL(ch, SKILL_SOUL_TRAP))
  {
    if (!notch_skill(ch, SKILL_SOUL_TRAP, get_property("skill.notch.soulTrap", 1)) &&
        number(1, 100) >= GET_CHAR_SKILL(ch, SKILL_SOUL_TRAP) / 3)
      return false;
    himself = true;
  }
  else
  {
    for (gl = ch->group; gl; gl = gl->next) {
      if (GET_CHAR_SKILL(gl->ch, SKILL_SOUL_TRAP) &&
          (notch_skill(gl->ch, SKILL_SOUL_TRAP,
                       get_property("skill.notch.soulTrap", 100) * 3) ||
           GET_CHAR_SKILL(gl->ch, SKILL_SOUL_TRAP)/5 > number(1,100))) {
        ch = gl->ch;
        break;
      }
    }
    if (!gl)
      return false;
  }

  vamp(ch, hps, (int)(GET_MAX_HIT(ch) * 1.1));

  if (himself)
  {
    act("&+LThe room seems to darken as&n $n &+Rslams&n $p &+Lthrough&n $N's &+Lchest!&n",
        FALSE, ch, 0, victim, TO_ROOM);
    act("&+LAs $E slumps to the ground, bleeding, screaming, begging for mercy,&n $n &+Ltwist the weapon and draws the soul from the dying body.",
        FALSE, ch, 0, victim, TO_ROOM);
    act("&+LThe air seems to vibrate with &+Wenergy &+Lwhich causes&n $n &+Lto throw back $s head and let out a mocking laughter that freezes your &+Rblood.&n",
        FALSE, ch, 0, victim, TO_ROOM);

    act("&+LThe room seems to darken as you slam&n $p &+Lthrough&n $N's &+Lchest!&n",
        FALSE, ch, 0, victim, TO_CHAR);
    act("&+LAs $E slumps to the ground, bleeding, screaming, begging for mercy, you twist the weapon and draw the soul from the dying body.",
        FALSE, ch, 0, victim, TO_CHAR);
    act("&+LA sweetness fills your being and you cannot help but to let out a &=LWmocking laughter.&n&n",
        FALSE, ch, 0, victim, TO_CHAR);
  }
  else
  {
    act("With a &+mmocking smile&n lacking any &+ywarmth&n $n takes a step towards $s &+yhelpless victim&n \n"
        "and &+Rdrives&n $p &+Rthrough&n $N's &+Rchest.&n",
        FALSE, ch, 0, victim, TO_ROOM);
    act("$s &+msmile deepens&n as $e watches the life quickly seep out of $N's body.",
        FALSE, ch, 0, victim, TO_ROOM);
    act("&+LStrengthened by the suffering around $m&n $n &+Ljerks $s weapon free causing the now lifeless husk to crumble in a heap.&n",
        FALSE, ch, 0, victim, TO_ROOM);

    act("&+YAmused, &+yyou take a step forward and drive&n $p &+ythrough&n $N's &+ychest.&n",
        FALSE, ch, 0, victim, TO_CHAR);
    act("&+yAs $E slumps to the ground screaming for &+Wmercy&+y, you twist the blade causing $S &+Llifeblood &+yto pour from $S body.&n",
        FALSE, ch, 0, victim, TO_CHAR);
    act("&+yStrengthened by the suffering you jerk your weapon free causing the now lifeless husk to crumble in a heap.&n",
        FALSE, ch, 0, victim, TO_CHAR);
  }

  return true;
}

void appear(P_char ch, bool removeHide )
{
  P_char master;

  if( !ch )
  {
    logit(LOG_EXIT, "appear called in fight.c without ch");
    raise(SIGSEGV);
  }

  // If someone is going vis via being ordered to do something, have the person doing the ordering go vis as well.
  if( (master = GET_MASTER(ch)) != NULL )
  {
    if( IS_AFFECTED5(master, AFF5_ORDERING) )
    {
      appear( master, removeHide );
    }
  }

  // CMD_FIRE (do_fire) handles its own hide stuff.
  if( removeHide )
  {
    REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
  }

  if( (!IS_SET(ch->specials.affected_by, AFF_INVISIBLE)
    && !IS_SET(ch->specials.affected_by2, AFF2_MINOR_INVIS)
    && !IS_SET(ch->specials.affected_by3, AFF3_ECTOPLASMIC_FORM)
    && !IS_SET(ch->specials.affected_by3, AFF3_NON_DETECTION)) )
  {
      return;
  }

  affect_from_char(ch, SPELL_INVISIBLE);
  affect_from_char(ch, TAG_PERMINVIS);
  affect_from_char(ch, SPELL_INVIS_MAJOR);
  affect_from_char(ch, SPELL_ECTOPLASMIC_FORM);

  REMOVE_BIT(ch->specials.affected_by, AFF_INVISIBLE);
  REMOVE_BIT(ch->specials.affected_by2, AFF2_MINOR_INVIS);
  REMOVE_BIT(ch->specials.affected_by3, AFF3_ECTOPLASMIC_FORM);

  if( IS_SET(ch->specials.affected_by3, AFF3_NON_DETECTION) )
  {
    struct affected_type *afp;

    for( afp = ch->affected; afp; afp = afp->next )
    {
      // Need to remove the mind blank effect without removing the cooldown.
      if( afp->type == SPELL_MIND_BLANK && afp->bitvector3 == AFF3_NON_DETECTION )
      {
        affect_remove(ch, afp);
        // If you decide not to break here, for whatever reason, you need to modify the loop.
        break;
      }
    }
    REMOVE_BIT(ch->specials.affected_by3, AFF3_NON_DETECTION);
  }

  act("$n snaps into visibility.", TRUE, ch, 0, 0, TO_ROOM);
  act("You snap into visibility.", FALSE, ch, 0, 0, TO_CHAR);

}

// This function figures a time for victim to sit around in heaven (and sets it).
// Right now it has a base of level+20 sec + 3 minutes for every PvP death within the hour.
void setHeavenTime( P_char victim )
{
  int time_in_heaven, counter, i;
  int kill_ids[20];
  MYSQL_RES *res;
  MYSQL_ROW row;

  // Victim must be a real PC, but doesn't have to be alive.
  if( !victim || IS_NPC(victim) )
  {
    return;
  }

  /* level / 10 minutes in heaven, rounded up */
  //time_in_heaven = ((GET_LEVEL(victim) - 1) / 10 + 1) * 60;
  // New value: level + 20 seconds in heaven.
  time_in_heaven = GET_LEVEL(victim) + 20;

  // Query the DB for the latest 20 deaths of victim in pvp
  qry( "SELECT event_id FROM pkill_info WHERE pid=%d AND pk_type='VICTIM' ORDER BY id DESC LIMIT 20", GET_PID(victim) );
  res = mysql_store_result(DB);
  if( res )
  {
    counter = 0;
    i = -1;
    // Walk through and record each pkill id battle number.
    while( (row = mysql_fetch_row(res)) != NULL )
    {
      kill_ids[++i] = atoi(row[0]);
    }

    mysql_free_result(res);

    while( i >= 0 )
    {
      // If this death was within the last 60 min, increase the counter.
      // Note: qry should always return true (successful attempt; no errors),
      //   and res should have 0 or 1 rows, depending on whether the event was within the 60 minutes.
      if( qry( "SELECT * from pkill_event WHERE id=%d AND TIMESTAMPDIFF( MINUTE, stamp, NOW() ) < 60", kill_ids[i--]) )
      {
        res = mysql_store_result(DB);
        if( mysql_num_rows(res) > 0 )
        {
          counter++;
        }
        mysql_free_result(res);
      }
    }

    // Double the time for every death over 1 within the hour.
    while( --counter > 0 )
      time_in_heaven *= 2;
    // If it's greater than 30 mins (then it'll reset while they're sitting there, so we want to cage them maybe?)
    // For right now, make it 12 hrs+ <--- should stop feeding for a day at least.
    if( time_in_heaven > 30 * 60 )
      time_in_heaven *= 24;
  }

  victim->only.pc->pc_timer[PC_TIMER_HEAVEN] = time(NULL) + time_in_heaven;
  debug( "%s time in heaven: until %s = %d seconds, recent deaths = %d.", J_NAME(victim),
    asctime(localtime(&(victim->only.pc->pc_timer[PC_TIMER_HEAVEN]))), time_in_heaven, counter );
}

void AddFrags(P_char ch, P_char victim)
{
  P_char   tch;
  int allies, recfrag, frag_gain, loss;
  char buffer[1024];
  struct affected_type af, *afp, *next_af;

  float gain, real_gain;

  if( IS_NPC(ch) )
  {
    if( ch->following && IS_PC(ch->following) && ch->in_room == ch->following->in_room && grouped(ch, ch->following) )
    {
      ch = ch->following;
    }
    else
    {
      return;
    }
  }

  for( tch = world[ch->in_room].people, allies = 0; tch; tch = tch->next_in_room )
  {
    if( IS_PC(tch) && !opposite_racewar(ch, tch) && !IS_TRUSTED(tch) )
    {
      allies++;
    }
  }

  gain = 100.0 / allies;

  if( EVIL_RACE(ch) && GOOD_RACE(victim) )
  {
    gain = gain * get_property("frag.evil.penalty", 0.666);
  }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if( (IS_PC(tch) && (grouped(ch, tch)) || ch == tch) )
    {
      /*  Code for recent frags to allow blood task to be fulfilled within a set
       *   period of time indicated in epic.frag.thrill.duration.  This allows for
       *   multiple frags to add up to enough to fulfill blood task - Jexni 12/1/08
       */
      recfrag = 0;
      for( afp = tch->affected; afp; afp = next_af )
      {
        next_af = afp->next;
        if( afp->type == TAG_PLR_RECENT_FRAG )
        {
          recfrag = afp->modifier;
          break;
        }
      }

      if( fragWorthy(tch, victim) )
      {
        if( GET_LEVEL(tch) > GET_LEVEL(victim) + 5 )
          real_gain = (gain * get_property("frag.leveldiff.modifier.low", 0.500));
        else if( GET_LEVEL(tch) + 5 < GET_LEVEL(victim) )
          real_gain = (gain * get_property("frag.leveldiff.modifier.high", 1.200));
        else
          real_gain = gain;

        snprintf(buffer, 1024, "You just gained %.02f frags!\r\n", real_gain/100.0);
        send_to_char(buffer, tch, LOG_PUBLIC);

        tch->only.pc->oldfrags = tch->only.pc->frags;
        tch->only.pc->frags += (long)real_gain;
        sql_modify_frags(tch, (int)real_gain);

        memset(&af, 0, sizeof(af));
        af.type = TAG_PLR_RECENT_FRAG;
        af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
        af.modifier = recfrag + (int)real_gain;
        af.duration = get_property("epic.frag.thrill.duration", 45) * WAIT_SEC;

        // affect_from_char doesn't care if ch has the spell or not, so just attempt to remove.
        affect_from_char( tch, TAG_PLR_RECENT_FRAG );
        // Then reapply the new af.
        affect_to_char(tch, &af);

        if( real_gain + recfrag >= get_property("epic.frag.threshold", 0.10)*100 )
        {
          frag_gain = (int) ((real_gain/100.00) * (float) (get_property("epic.frag.amount", 20.000)));
          epic_frag(tch, GET_PID(victim), frag_gain);
        }

        if( GET_RACE(tch) == RACE_HALFLING || GET_CLASS(tch, CLASS_MERCENARY) )
        {
          char     tmp[1024];
          snprintf(tmp, 1024, "You get %s in blood money.\r\n", coin_stringv(10000 * real_gain));
          send_to_char(tmp, tch);
          ADD_MONEY(tch, 10000 * real_gain);
        }

        if( (tch->only.pc->frags / 100) > (tch->only.pc->oldfrags / 100) )
        {
          checkFragList(tch);
        }

        if( IS_ILLITHID(tch) )
        {
          illithid_advance_level(tch);
        }

        check_boon_completion(tch, victim, ((double)(real_gain/100)), BOPT_FRAG);
        check_boon_completion(tch, victim, ((double)(real_gain/100)), BOPT_FRAGS);
      }
    }
  }

  if( GET_LEVEL(ch) > GET_LEVEL(victim) + 5 )
    loss = (int)(gain * get_property("frag.leveldiff.modifier.low", 0.500));
  else if( GET_LEVEL(ch) + 5 < GET_LEVEL(victim) )
    loss = (int)(gain * get_property("frag.leveldiff.modifier.high", 1.200));
  else
    loss = gain;

  sql_modify_frags(victim, -loss);
  victim->only.pc->frags -= loss;
  snprintf(buffer, MAX_STRING_LENGTH, "You just lost %.02f frags!\r\n", ((float) loss) / 100);
  send_to_char(buffer, victim);
  debug( "%s just fragged %s for %.02f/%.02f frags!", J_NAME(ch), J_NAME(victim), ((float) real_gain) / 100, ((float) loss) / 100);
  checkFragList(victim);

  if(IS_PC(victim))
  {
    memset(&af, 0, sizeof(af));
    af.type = TAG_RECENTLY_FRAGGED;
    af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
    af.duration = 60 * WAIT_SEC;
    affect_to_char(victim, &af);
  }

  // When a player with a blood tasks dies, they now satisfy the pvp spill blood task.
  if( afp = get_epic_task(victim) )
  {
    if( (abs( afp->modifier ) == SPILL_BLOOD) && (loss > 0) )
    {
      send_to_char("The &+yGods of Duris&n are very pleased with YOUR &+Rblood&n, too!!!\n", victim);
      send_to_char("You can now progress further in your quest for epic power!\n", victim);
      affect_remove(victim, afp);
    }
  }
}

unsigned int calculate_ch_state(P_char ch)
{
  if(ch)
  {
    if (GET_HIT(ch) < -10)
      return STAT_DEAD;
    else if (GET_HIT(ch) <= -6)
      return STAT_DYING;
    else if (GET_HIT(ch) <= -3)
      return STAT_INCAP;
    else if (((GET_STAT(ch) == STAT_SLEEPING) || (GET_STAT(ch) == STAT_RESTING))
        && (IS_FIGHTING(ch) || NumAttackers(ch) || IS_DESTROYING(ch)))
      return STAT_NORMAL;
    else if (GET_STAT(ch) < STAT_SLEEPING)
      return STAT_RESTING;
    else
      return GET_STAT(ch);
  }
  return STAT_DEAD;
}

void update_pos(P_char ch)
{
  int      pos, stat, tmp;
  P_char   mount;

  if (!ch)
  {
    logit(LOG_EXIT, "assert: update_pos() - no ch");
    raise(SIGSEGV);
  }
  if ((IS_NPC(ch) && ch->only.npc == NULL) ||
      (IS_PC(ch) && ch->only.pc == NULL))
    return;

  if (IS_FIGHTING(ch))
    if (ch->in_room != GET_OPPONENT(ch)->in_room)
      stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    if( ch->in_room != ch->specials.destroying_obj->loc.room)
      stop_destroying(ch);

  mount = get_linked_char(ch, LNK_RIDING);
  if (mount && mount->in_room != ch->in_room)
    stop_riding(ch);

  stat = calculate_ch_state(ch);
  pos = GET_POS(ch);

  /*
   * quadrupeds are much more stable than bipeds, so they skip this
   * little indignity. JAB
   */

  if ((IS_HUMANOID(ch) || IS_GIANT(ch)) && (stat < STAT_RESTING))
  {
    tmp = 0;
    switch (GET_POS(ch))
    {
      case POS_PRONE:
        /*
         * nada
         */
        break;
      case POS_KNEELING:
        /*
         * fairly stable, (except riding) but there is that chance...
         */
        switch (stat)
        {
          case STAT_DEAD:
            if (IS_RIDING(ch))
              tmp = 1;
            else
            {
              if (!number(0, 9))
                tmp = 1;
            }
            break;
          case STAT_DYING:
          case STAT_INCAP:
          case STAT_SLEEPING:
            if (IS_RIDING(ch))
            {
              if (!number(0, 1))
                tmp = 1;
            }
            else
            {
              if (!number(0, 9))
                tmp = 1;
            }
            break;
        }
        break;
      case POS_SITTING:
        /*
         * moderately stable, esp. if mounted, but not completely...
         */
        switch (stat)
        {
          case STAT_DEAD:
            if (IS_RIDING(ch))
            {
              if (!number(0, 1))
                tmp = 1;
            }
            else
            {
              if (!number(0, 9))
                tmp = 1;
            }
            break;
          case STAT_DYING:
          case STAT_INCAP:
          case STAT_SLEEPING:
            if (IS_RIDING(ch))
            {
              if (!number(0, 4))
                tmp = 1;
            }
            else
            {
              if (!number(0, 7))
                tmp = 1;
            }
            break;
        }
        break;
      case POS_STANDING:
        /*
         * very unstable
         */
        switch (stat)
        {
          case STAT_DEAD:
            /*
             * they can die on their feet, but they aren't staying
             * standing...
             */
            tmp = 1;
            break;
          case STAT_DYING:
          case STAT_INCAP:
          case STAT_SLEEPING:
            if (IS_RIDING(ch))
              tmp = 1;
            else
            {
              if (!number(0, 5))
                tmp = 1;
            }
            break;
        }
        break;
    }
    if (tmp)
    {
      tmp = 0;
      /*
       * they involuntarily changed position, can be real bad if
       * they were mounted at the time. (damage + stun).  JAB
       */
      if (mount)
      {
        tmp = 1;
        switch (GET_POS(mount))
        {
          case POS_PRONE:
          case POS_KNEELING:
          case POS_SITTING:
            /*
             * not far to fall, no damage, just stop riding
             */
            break;
          case POS_STANDING:
            /*
             * ouchness
             */
            Stun(ch, ch, dice(3, 4) * 4, FALSE);     /* 3 rounds max, avg, ~1.5 */
            break;
        }
        act("$n falls off $s mount!", TRUE, ch, 0, 0, TO_ROOM);
        stop_riding(ch);
      }
      else
      {
        switch (pos)
        {
          case POS_PRONE:
          case POS_KNEELING:
          case POS_SITTING:
            /*
             * no damage, just go prone.
             */
            break;
          case POS_STANDING:
            /* if they are dying, DON'T have them take damage from
               this slumping BS, or we end up with chars that are
               "dead" until hit_regen kills them */
            if (stat <= STAT_DYING)
              break;

            /*
             * minor owie, maybe
             */
            tmp = 0;
            if (!number(0, 2))
            {
              if (GET_STAT(ch) > STAT_INCAP)
                Stun(ch, ch, dice(2, 3) * 4, FALSE); /*
                                                      * 1.5 rounds max,
                                                      * avg, ~1
                                                      */
              tmp = 1;            /*
                                   * will wake normal sleepers
                                   */
            }
            act("$n slumps to the ground.", TRUE, ch, 0, 0, TO_ROOM);
            break;
        }
      }
      pos = POS_PRONE;

      /*
       * since they can take damage in this routine, check again.
       * JAB
       */
      if (GET_HIT(ch) < -10)
        stat = STAT_DEAD;
      else if (GET_HIT(ch) <= -6)
        stat = STAT_DYING;
      else if (GET_HIT(ch) <= -3)
        stat = STAT_INCAP;
      else
        if( ((GET_STAT(ch) == STAT_SLEEPING) || (GET_STAT(ch) == STAT_RESTING)) 
            && (IS_FIGHTING(ch) || NumAttackers(ch) || IS_DESTROYING(ch)) )
          stat = STAT_NORMAL;
        else if (GET_STAT(ch) < STAT_SLEEPING)
          stat = STAT_RESTING;
        else
          stat = GET_STAT(ch);    /*
                                   * SLEEPING/RESTING/NORMAL
                                   */

      /*
       * if they are just normally asleep, falling will wake them
       * (mostly), if they were magically asleep, taking damage from
       * falling down will break the spell.  If they wind up worse
       * than sleeping, ah well...
       */

      if (tmp && (stat < STAT_RESTING))
      {
        if (affected_by_spell(ch, SPELL_SLEEP))
        {
          REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
          affect_from_char(ch, SPELL_SLEEP);
        }
        if(affected_by_spell(ch, SONG_SLEEP))
        {
          REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
          affect_from_char(ch, SONG_SLEEP);
        }        
        if (stat == STAT_SLEEPING)
        {
          stat = STAT_NORMAL;
          send_to_char
            ("Huh?  What!?  You find yourself laying on the ground!\r\n", ch);
        }
      }
    }
  }
  if ((GET_STAT(ch) == STAT_SLEEPING) && (stat > STAT_SLEEPING))
  {
    act("$n has a RUDE awakening!", TRUE, ch, 0, 0, TO_ROOM);
    if(affected_by_spell(ch, SPELL_SLEEP))
    {
      REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
      affect_from_char(ch, SPELL_SLEEP);
    }
    if(affected_by_spell(ch, SONG_SLEEP))
    {
      REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
      affect_from_char(ch, SONG_SLEEP);
    }
    do_wake(ch, 0, -4);
    if(IS_NPC(ch))
    {
      do_stand(ch, 0, 0);
      do_alert(ch, 0, 0);
    }

  }
  /*
   * finally, set new position and status
   */

  SET_POS(ch, pos + stat);
  if (stat == STAT_DEAD)
  {
    if (IS_FIGHTING(ch))
      stop_fighting(ch);
    if( IS_DESTROYING(ch))
      stop_destroying(ch);
    StopAllAttackers(ch);
    stat = STAT_DYING;          /*
                                 * reason being, killing people in
                                 * update_pos, would cause horrible
                                 * logistic nightmares.  If they are dead,
                                 * setting them dying will handle most
                                 * cases. If THIS causes problems, may
                                 * have to bite the bullet and change the
                                 * code in LOTS of places. Hopefully this
                                 * will suffice.  JAB
                                 */
    /*    SET_POS(ch, pos + stat);*/
  }
  if (pos != POS_STANDING)
  {
    clear_links(ch, LNK_FLANKING);
    clear_links(ch, LNK_CIRCLING);
  }

  if(IS_IMMOBILE(ch))
  {
    if (IS_FIGHTING(ch))
      stop_fighting(ch);
    if( IS_DESTROYING(ch) )
      stop_destroying(ch);
    StopMercifulAttackers(ch);
  }
  /*
   * final check for mobs, if they can assume their default position
   */
  /*
   * added check - DTS 7/11/95
   */
  if( IS_NPC(ch) && (stat > STAT_SLEEPING) && !IS_FIGHTING(ch) && CAN_ACT(ch)
      && ((ch->only.npc->default_pos & STAT_MASK) >= STAT_SLEEPING)
      && (!HAS_MEMORY(ch) || !GET_MEMORY(ch)) && !IS_DESTROYING(ch) )
    ch->specials.position = ch->only.npc->default_pos;
}

/*
 * used to restrict targeting in SINGLE_FILE rooms, was simple, but got
 * uglier when I started skipping immortals.  Even worse now that I have
 * to skip 'wraithform' chars too.  JAB
 */
bool AdjacentInRoom(P_char ch, P_char ch2)
{
  P_char   t_ch, t_ch2;

  if (!ch || !ch2 || (ch->in_room != ch2->in_room) ||
      (ch->in_room == NOWHERE))
    return FALSE;

  if (IS_TRUSTED(ch) || IS_TRUSTED(ch2) ||
      IS_AFFECTED(ch, AFF_WRAITHFORM) || IS_AFFECTED(ch2, AFF_WRAITHFORM))
    return TRUE;

  t_ch = world[ch->in_room].people;

  if (!t_ch)
    return FALSE;

  /*
   * find first of ch/ch2 in room list
   */

  while (t_ch && (t_ch != ch) && (t_ch != ch2))
    t_ch = t_ch->next_in_room;

  if (!t_ch)
    return FALSE;

  t_ch2 = t_ch->next_in_room;

  /*
   * find the second of ch/ch2 in room list, skipping immorts
   */

  while (t_ch2 && (IS_TRUSTED(t_ch2) || IS_AFFECTED(t_ch2, AFF_WRAITHFORM)))
    t_ch2 = t_ch2->next_in_room;

  if (!t_ch2)
    return FALSE;

  /*
   * yup, they are effectively adjacent
   */
  if (((t_ch == ch) && (t_ch2 == ch2)) || (t_ch2 == ch))
    return TRUE;

  return FALSE;
}

P_obj make_corpse(P_char ch, int loss)
{
  P_obj    corpse, o, money;
  P_char   rider;
  char     buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int      i, e_time;
  int      random_zone_map_room = 0;

  corpse = read_object(2, VIRTUAL);
  if (!corpse)
  {
    logit(LOG_EXIT, "make_corpse: no valid corpse object found");
    raise(SIGSEGV);
  }

  corpse->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3);

  if( IS_PC(ch) )
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s", GET_NAME(ch), "corpse _pcorpse_");
  }
  else
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s", GET_NAME(ch), "corpse _npcorpse_");
  }

  corpse->name = str_dup(buf);

  if (IS_PC(ch))
  {
    snprintf(buf2, MAX_STRING_LENGTH, "%s %s", index("AEIOU", race_names_table[ch->player.race].normal[0]) == NULL ? "a" : "an",
      race_names_table[ch->player.race].normal);
  }
  snprintf(buf, MAX_STRING_LENGTH, "The corpse of %s is lying here.", IS_PC(ch) ? buf2 : ch->player.short_descr);
  DECAP(buf + 14);

  corpse->description = str_dup(buf);

  snprintf(buf, MAX_STRING_LENGTH, "the corpse of %s", IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch));
  corpse->short_description = str_dup(buf);

  /*
   * for animate dead and resurrect
   */
  snprintf(buf, MAX_STRING_LENGTH, "%s", IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch));
  corpse->action_description = str_dup(buf);

  /*
   * changed this, add everything to ch's inven before xferring to
   * corpse, this makes the corpse weight correct.  (also simplifies
   * things.)
   */

  if( !IS_TRUSTED(ch) )
  {
    money_to_inventory(ch);
  }

  corpse->value[CORPSE_LEVEL] = GET_LEVEL(ch);     /* for animate dead */

  if( GET_LEVEL(ch) > 1 )
    corpse->value[CORPSE_EXP_LOSS] = -loss;

  /*
   * have to change the 'loc.carrying' pointers to 'loc.inside' pointers
   * for the whole object list, else ugly problems occur later.
   */
  unequip_all(ch);
  corpse->contains = ch->carrying;
  ch->carrying = NULL;

  for( o = corpse->contains; o; o = o->next_content )
  {
    o->loc_p = LOC_INSIDE;
    o->loc.inside = corpse;
    if( IS_ARTIFACT(o) && IS_PC(ch) )
      if( !remove_owned_artifact_sql(o, GET_PID(ch)) )
        wizlog(56, "couldn't unflag arti after %s died", GET_NAME(ch));
  }

  if (IS_NPC(ch))
  {
    e_time = get_property("timer.decay.corpse.npc", 120) * WAIT_MIN;
    corpse->weight = IS_CARRYING_W(ch, rider) * 2;
    corpse->value[CORPSE_WEIGHT] = IS_CARRYING_W(ch, rider);
    corpse->value[CORPSE_FLAGS] = NPC_CORPSE;
    if (ch->only.npc)
      corpse->value[CORPSE_VNUM] = mob_index[GET_RNUM(ch)].virtual_number;
    else
      corpse->value[CORPSE_VNUM] = 0;
    corpse->value[CORPSE_RACEWAR] = 0;
  }
  else
  {
    e_time = get_property("timer.decay.corpse.pc", 120) * WAIT_MIN;
    corpse->weight = GET_WEIGHT(ch) + IS_CARRYING_W(ch, rider);
    corpse->value[CORPSE_WEIGHT] = IS_CARRYING_W(ch, rider);       /* contains */
    corpse->value[CORPSE_FLAGS] = PC_CORPSE;
    corpse->value[CORPSE_PID] = GET_PID(ch);

    if (IS_RACEWAR_UNDEAD(ch))
    {
      corpse->value[CORPSE_RACEWAR] = 3;
    }
    else if (EVIL_RACE(ch))
    {
      corpse->value[CORPSE_RACEWAR] = 2;
    }
    else
    {
      corpse->value[CORPSE_RACEWAR] = 1;
    }

    /* value[6] is reserved for saved file id - Tharkun */
    corpse->value[CORPSE_SAVEID] = 0;

    if (IS_HUMANOID(ch))
      corpse->value[CORPSE_FLAGS] |= HUMANOID_CORPSE;      /* for carving */
  }

  corpse->value[CORPSE_RACE] = GET_RACE(ch);

  IS_CARRYING_N(ch) = 0;
  GET_CARRYING_W(ch) = 0;

  set_obj_affected(corpse, e_time, TAG_OBJ_DECAY, 0);

  if (ch->in_room == NOWHERE)
  {
    if (real_room(ch->specials.was_in_room) != NOWHERE)
      obj_to_room(corpse, real_room(ch->specials.was_in_room));
    else
    {
      // No place sane to put it
      obj_to_room(corpse, real_room(CORPSE_STORAGE_II));
    }
  }
  else
  {
    if (IS_PC(ch))
    {
      obj_to_room(corpse, ch->in_room);
    }
    else
      obj_to_room(corpse, ch->in_room);
  }
  /*
   * added by DTS 8/1/95 - ghosts and wraiths shouldn't leave corpses...
   * just dump contents of corpse to room and extract corpse
   */
  if (IS_NPC(ch) && IS_NOCORPSE(ch))
  {
    if (corpse->contains)
    {
      while (corpse->contains)
      {
        o = corpse->contains;
        obj_from_obj(o);
        // Should always be true, but just in case.
        if( OBJ_ROOM(corpse) )
        {
          obj_to_room(o, corpse->loc.room);
        }
        else
        {
          extract_obj(o, TRUE); // Yep, if the arti doesn't have a place to go..
        }
      }
    }

    switch (GET_RACE(ch))
    {
      case RACE_GHOST:
        act("$n dissolves into thin air.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_F_ELEMENTAL:
      case RACE_EFREET:
        act("$n disappears in a burst of fire.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_W_ELEMENTAL:
        act("$n sinks into the ground.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_A_ELEMENTAL:
        act("$n vanishes into thin air.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_E_ELEMENTAL:
        act("$n crumbles to dust.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_UNDEAD:
      case RACE_LICH:
      case RACE_VAMPIRE:
        act("$n crumbles to dust.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_SKELETON:
        act("$n falls to the ground, leaving a pile of inanimate bones.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_ZOMBIE:
        act("$n shambles for a moment, and falls to pieces.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_WRAITH:
      case RACE_SPECTRE:
        act("$n howls loudly as $e dissipates into the wind.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_SHADOW:
        act("$n explodes into all corners of the room, covering it in darkness!", TRUE, ch, 0, 0, TO_ROOM);
        spell_darkness(GET_LEVEL(ch), ch, 0, 0, 0, 0);
        break;
      case RACE_DEVA:
        act("$n explodes into all corners of the room, covering it in pure light!", TRUE, ch, 0, 0, TO_ROOM);
        spell_continual_light(GET_LEVEL(ch), ch, 0, 0, 0, 0);
        break;
      case RACE_V_ELEMENTAL:
        act("$n howls, and returns to the void which spawned it.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_I_ELEMENTAL:
        act("A blank look overcomes $n's face for a moment, before $e shatters into tiny fragments!", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case RACE_ARCHON:
      case RACE_ASURA:
      case RACE_TITAN:
      case RACE_AVATAR:
      case RACE_GHAELE:
      case RACE_BRALANI:
      case RACE_ELADRIN:
        act("$n &+Wglows white&n &+wbefore &+Ldisappearing...&n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        act("$n &+gquickly returns to the plane from whence they were summoned...&n", TRUE, ch, 0, 0, TO_ROOM);
        break;
    }

    obj_from_room(corpse);
    extract_obj(corpse);
    corpse = NULL;
  }
  if (corpse && IS_PC(ch))
    writeCorpse(corpse);

  return corpse;
}

void make_bloodstain(P_char ch)
{
  P_obj    blood, obj, next_obj;
  char     buf[MAX_STRING_LENGTH];
  int      msgnum;
  const char *long_desc[] = {
    "&+rFresh blood splatters cover the area.&n",
    "&+rA few drops of fresh blood are scattered around the area.&n",
    "&+rPuddles of fresh blood cover the ground.&n",
    "&+rFresh blood covers everything in the area.&n"
  };

  if( !HAS_FOOTING(ch) )
    return;

  if( GET_OPPONENT(ch) && (IS_UNDEADRACE(GET_OPPONENT(ch)) || IS_ANGEL(GET_OPPONENT(ch))) )
    return;

  if (IS_UNDEADRACE(ch) || IS_ANGEL(ch))
    return;

  if (world[ch->in_room].contents)
  {
    for (obj = world[ch->in_room].contents; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if( obj->R_num == real_object(VOBJ_BLOOD) )
      {
        obj_from_room(obj);
        extract_obj( obj );
      }
    }
  }

  blood = read_object(4, VIRTUAL);
  if( !blood )
  {
    return;
  }

  blood->str_mask = (STRUNG_DESC1);

  msgnum = number(0, 3);
  blood->value[0] = msgnum;
  blood->value[1] = BLOOD_FRESH;
  snprintf(buf, MAX_STRING_LENGTH, "%s", long_desc[msgnum]);
  blood->description = str_dup(buf);

  // 15 minutes, changes to regular blood at 3 minutes and dry blood at 7 minutes.
  // Becomes NOSHOW at 90 seconds.
  set_obj_affected(blood, 3600, TAG_OBJ_DECAY, 0);

  if( ch->in_room == NOWHERE )
  {
    if( real_room(ch->specials.was_in_room) != NOWHERE )
      obj_to_room(blood, real_room(ch->specials.was_in_room));
    else
    {
      extract_obj(blood);
      blood = NULL;
    }
  }
  else
  {
    obj_to_room(blood, ch->in_room);
  }
}

/*
 * When ch kills victim
 */
void change_alignment(P_char ch, P_char victim)
{
  int      i, a_al, v_al, change = 0;
  P_obj    obj;


  if (CHAR_IN_ARENA(ch) || CHAR_IN_ARENA(victim) || IS_NPC(ch))
    return;

  a_al = GET_ALIGNMENT(ch);
  v_al = GET_ALIGNMENT(victim);

  /*
   * global modifiers
   */

  if (IS_NPC(victim))
  {
    if (IS_SET(victim->only.npc->aggro_flags, AGGR_ALL))
      v_al -= 100;

    if (IS_SET(victim->only.npc->aggro_flags, AGGR_GOOD_ALIGN))
      if (IS_GOOD(ch))
        v_al -= 125;
      else
        v_al -= 50;

    if (IS_SET(victim->only.npc->aggro_flags, AGGR_NEUTRAL_ALIGN))
      if (IS_NEUTRAL(ch))
        v_al -= 25;
      else
        v_al -= 50;

    if (IS_SET(victim->only.npc->aggro_flags, AGGR_EVIL_ALIGN))
      if (IS_EVIL(ch))
        v_al += 125;
      else
        v_al += 75;

    if (IS_DEMON(victim))
      change += 2;

    if (IS_SET(victim->specials.act, ACT_NICE_THIEF))
      v_al += 25;

    if (IS_SET(victim->specials.act, ACT_STAY_ZONE))
      v_al += 25;

    if (IS_SET(victim->specials.act, ACT_WIMPY))
      v_al += 10;
  }
  else
  {
    if (victim->only.pc->aggressive > -1)
      v_al -= 100;

    if (IS_SET(victim->only.pc->law_flags, PLR_VICIOUS))
      v_al -= 25;

    if(GET_SPEC(victim, CLASS_ROGUE, SPEC_ASSASSIN) ||
        GET_CLASS(ch, CLASS_ASSASSIN))
      change += 2;

    if (GET_CLASS(victim, CLASS_ANTIPALADIN))
      change += 5;

    if (GET_CLASS(victim, CLASS_PALADIN))
      change -= 5;

    if (is_linked_to(ch, victim, LNK_CONSENT))
      change -= 10;
  }

  /*
   * at this point v_al will range from -1275 to 1135 depending on
   * various flags and such, this will allow for the possibility of
   * alignment shifts even when ch is -1000 or 1000, without having to
   * pick nits.
   */

  /*
   * ok, 'good' alignment is the most vulnerable to change, neutral not
   * nearly so, and evil is very tough to change (except to become more
   * evil)
   */

  if (a_al > 350)
  {

    /*
     * ch is 'good'
     */

    if (v_al > 350)
    {

      /*
       * victim is also 'good', this is by far the most drastic
       * shift
       */

      if (v_al > a_al)
      {

        /*
         * victim is 'more' good
         */
        change += ((a_al - v_al) / 40 - 9);     /*
                                                 * -28 to -9
                                                 */
      }
      else
      {

        /*
         * victim is 'less' good, but still good
         */
        change += ((v_al - a_al) / 50 - 6);     /*
                                                 * -18 to -6
                                                 */
      }

    }
    else if (v_al > -351)
    {

      /*
       * victim is neutral, not near as bad
       */

      change += (v_al / -80);   /*
                                 * -4 to 4
                                 */

    }
    else
    {

      /*
       * victim is evil, adds to align, but not alot
       */

      change += (v_al / -150);  /*
                                 * 8 to 2
                                 */

    }
  }
  else if (a_al > -351)
  {

    /*
     * ch is 'neutral'
     */

    /*
     * neutral chars don't change much for killing either extreme,
     * instead, killing more 'moderate' alignments has the largest
     * effect
     */

    if (v_al > 0)
      change -= ((1135 - v_al) / 100);  /*
                                         * 0 to -11
                                         */
    else
      change += ((1275 + v_al) / 100);  /*
                                         * 12 to 0
                                         */

  }
  else
  {

    /*
     * ch is 'evil'
     */

    /*
     * almost ANYTHING an evil kills can be regarded as an evil act.
     * The only exception is killing demons, and killing things MORE
     * evil than they are currently, but not a lot in any case.
     */

    if (v_al < a_al)
    {

      /*
       * victim is more evil, very small shift towards neutral
       */

      change += ((v_al - a_al) / -200); /*
                                         * 4 to 0
                                         */
    }
    else
    {

      /*
       * victim is 'less' evil
       */

      change += ((v_al - a_al) / -100); /*
                                         * -21 to 0
                                         */
    }
  }

  /*
   * unless I really screwed something up, change now ranges for -36 to
   * 20, since we want to minimize alignment shifts, we are going to use
   * those as a basis for 'chance of shift'.  Max actual change will be
   * -4 to 2, and it's going to take concerted effort over a LONG time
   * to change it much at all.
   */

  /*
   * final mod, nothing tougher to change than 'old' evil and nothing
   * faster than a fall from grace
   */

  if (IS_EVIL(ch) && (change > 1)) {
    change = BOUNDED(1, change - (GET_LEVEL(ch) / 15), change);
  }
  if (IS_GOOD(ch) && (change < 0)) 
  {
    change -= (GET_LEVEL(ch) / 10);
  }

  if (EVIL_RACE(ch) && GOOD_RACE(victim))
  {                                        // Lets try to keep evil race folks
    change += (-number(50, 500));         // from having a free ride when they
  }                                        // obtain good alignment

  /*
   * The Druid balance will flux a lot.  However, it will balance itself
   * as druids commune in forests.
   */

  if (GET_CLASS(ch, CLASS_DRUID) || (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID))) {
    if (world[ch->in_room].sector_type != SECT_FOREST) {
      change *= 2;
    }
  }

  if (change < 0)
  {
    while (change < 0)
    {
      a_al -= (number(0, 9) < MIN(10, -(change)));
      change += 7;
    }
  }
  else
  {
    while (change > 0)
    {
      a_al += (number(0, 9) < MIN(10, change));
      change -= 10;
    }
  }

  GET_ALIGNMENT(ch) = BOUNDED(-1000, a_al, 1000);

}

void death_cry(P_char ch)
{
  int      door, was_in, room;
  char     buf[MAX_INPUT_LENGTH];

  switch(number(1,5))
  {
    case 1:
      act ("&+rYou feel the bloodlust in your heart as you hear the death cry of&N $n.&n", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      act ("$n&N&+r's death cry reverberates in your head as $e falls to the ground.&n", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      act ("&+rThe last gasps of&n $n &n&+rcause a sickening chill to run up your spine.&n", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act  ("&+rThe unmistakable scent of fresh blood can be smelled as&N $n &N&+rdies in agony.&n", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act    ("&+rA look of horror and a silent scream are&n $n&N&+r's last actions in this world.&n", FALSE, ch, 0, 0, TO_ROOM);
      break;
  }
  was_in = ch->in_room;
  play_sound(SOUND_DEATH_CRY, NULL, was_in, TO_ROOM);

  add_track(ch, NUM_EXITS);

  if (was_in != NOWHERE)
    for (door = 0; door <= (NUM_EXITS - 1); door++)
    {
      if (VIRTUAL_CAN_GO(was_in, door))
      {
        room = world[ch->in_room].dir_option[door]->to_room;
        switch(number(1, 3))
        {
          case 1:
            snprintf(buf, MAX_INPUT_LENGTH,
                "&+rThe unmistakable sound of something dying reverberates from nearby.\r\n");
            break;
          case 2:
            snprintf(buf, MAX_INPUT_LENGTH,
                "&+rYour spine tingles as a rattling death cry reaches your senses from nearby.\r\n");
            break;
          case 3:
            snprintf(buf, MAX_INPUT_LENGTH,
                "&+rA nearby death cry rings out loudly, heightening your bloodlust.\r\n");
            break;
        }
        send_to_room(buf, room);
        play_sound(SOUND_DEATH_CRY, NULL, room, TO_ROOM);
      }
    }
}

void death_rattle(P_char ch)
{
  int      door, was_in, room;
  char     buf[MAX_INPUT_LENGTH];

  act("&+rYou feel a carnal satisfaction as $n&+r's gurgling and choking signals $s demise.&n", FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;
  play_sound(SOUND_DEATH_CRY, NULL, was_in, TO_ROOM);

  add_track(ch, NUM_EXITS);

  if (was_in != NOWHERE)
    for (door = 0; door <= (NUM_EXITS - 1); door++)
    {
      if (VIRTUAL_CAN_GO(was_in, door))
      {
        room = world[ch->in_room].dir_option[door]->to_room;
        snprintf(buf, MAX_INPUT_LENGTH,
            "&+rYou hear a shrill death rattle nearby!\r\n");
        send_to_room(buf, room);
        play_sound(SOUND_DEATH_CRY, NULL, room, TO_ROOM);
      }
    }
}
/*
 * this routine was repeated all over, made it a function, basically
 * handles switched gods and shapechangers, so that the right body dies.
 * Returns the right char, or NULL, if there is a problem.  JAB
 */
P_char ForceReturn(P_char ch)
{
  P_char   true_id, t_ch = ch;
  int      is_avatar = FALSE, virt;

  if (!t_ch)
    return NULL;

  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    BackToUsualForm(ch);
    return ch;
  }
  /*
   * morph'ed players go back...
   */
  if (IS_MORPH(t_ch))
  {
    virt = mob_index[GET_RNUM(t_ch)].virtual_number;
    if (virt == EVIL_AVATAR_MOB || virt == GOOD_AVATAR_MOB)
      is_avatar = TRUE;
    true_id = MORPH_ORIG(t_ch);
    if (!is_avatar)
    {
      act("$n's dying body slowly changes back into $N.",
          FALSE, t_ch, 0, true_id, TO_ROOM);
      send_to_char
        ("As the last of life leaves you, your body resumes its natural form.\r\n",
         t_ch);
    }
    else
    {
      act("$n has been vanquished!", FALSE, t_ch, 0, 0, TO_ROOM);
      send_to_char
        ("As the last of life leaves your form, you return to your normal body.\r\n",
         t_ch);
    }
    if (GET_OPPONENT(t_ch))
      stop_fighting(t_ch);
    if( IS_DESTROYING(t_ch) )
      stop_destroying(t_ch);
    un_morph(t_ch);
    return true_id;
  }

  if (ch && IS_PC(ch) && ch->only.pc->switched &&
      IS_MORPH(ch->only.pc->switched))
  {
    t_ch = ch->only.pc->switched;
    send_to_char
      ("&+RYour mind can no longer hold this form as your true body has been destroyed!!&n\r\n",
       t_ch);
    act
      ("&+BThe soul holding $n &+Bto this realm has perished!\r\n$n&+B vanishes.",
       FALSE, t_ch, 0, 0, TO_ROOM);
    if (GET_OPPONENT(t_ch))
      stop_fighting(t_ch);
    if( IS_DESTROYING(t_ch) )
      stop_destroying(t_ch);
    un_morph(t_ch);
    return ch;
  }

  /*
   * switched god
   */

  if (t_ch->desc && t_ch->desc->original && IS_PC(t_ch->desc->original) &&
      t_ch->desc->original->only.pc->switched)
  {
    do_return(t_ch, 0, CMD_DEATH);
  }
  return (t_ch);
}

/* These two functions are no longer used.  No idea when they went out of style.
 *   Discovered them on 6/29/2015 while moving arti code from file-based to DB-based.
void update_all_arti_blood(P_char ch, int mod)
{
  int      i, count;
  char     tmp_buf[MAX_STRING_LENGTH];
  P_obj    obj;
  P_char tch;

  //dont feed if misfire check active.

  for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
  {
    if( (ch == tch || grouped(tch, ch)) && affected_by_spell(tch, TAG_NOMISFIRE) )
    {
      break;
    }
  }

  if( !tch )
  {
    send_to_char("&+RYour artifact(s) do not seem happy with this kind of blood.&n\r\n", ch);
    return;
  }

  count = 0;
  for (i = 0; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
    {
      if( IS_ARTIFACT(ch->equipment[i]) || CAN_WEAR(obj, ITEM_WEAR_IOUN) )
      {
        count++;
      }
    }
  }
  P_obj tobj = ch->carrying;
  while (tobj)
  {
    if( IS_ARTIFACT(ch->equipment[i]) || CAN_WEAR(obj, ITEM_WEAR_IOUN) )
      count++;
    tobj = tobj->next_content;
  }

  for (i = 0; i < MAX_WEAR; i++)
  {
    obj = ch->equipment[i];
    if( obj )
    {
      if( (IS_ARTIFACT(ch->equipment[i]) && isname("unique", obj->name)) || CAN_WEAR(obj, ITEM_WEAR_IOUN) )
      {
        UpdateArtiBlood(ch, ch->equipment[i], mod);
      }
      else if( IS_ARTIFACT(ch->equipment[i]) )
      {
        UpdateArtiBlood(ch, ch->equipment[i], (int)(mod/count));
      }
    }
  }
}

void perform_arti_update(P_char ch, P_char victim)
{
  float    frags;

  // Check ch's group, see if any members' artis feed

  if (ch->group)
  {
    struct group_list *gl;

    gl = ch->group;

    frags = gl->ch->only.pc->frags - gl->ch->only.pc->oldfrags;

    while (gl)
    {
      if (IS_PC(gl->ch) && (gl->ch->in_room == ch->in_room) && fragWorthy(gl->ch, victim))
      {
        update_all_arti_blood(gl->ch, gl->ch->only.pc->frags - gl->ch->only.pc->oldfrags);
      }

      gl = gl->next;
    }
  }
  else
  {
    if (IS_PC(ch) && fragWorthy(ch, victim) && ((ch->only.pc->frags) > (ch->only.pc->oldfrags)))
      update_all_arti_blood(ch, ch->only.pc->frags - ch->only.pc->oldfrags);
  }
}
*/

// in_their_zone is designed to stop pets/mounts/etc from dropping recipes.
// This function returns TRUE iff a mob is in the zone that it loads in.
bool in_their_zone( P_char mob )
{
  int vnum;

  // PCs never in their zone.
  if( !IS_NPC(mob) )
    return FALSE;

  vnum = mob_index[GET_RNUM(mob)].virtual_number;

  // If vnum falls outside the vnums for the zone it's in.
  //  vnum too small, or mob not in last zone and vnum too big
  if( vnum < zone_table[world[mob->in_room].zone].number * 100
    || (world[mob->in_room].zone < top_of_zone_table
      && vnum > zone_table[world[mob->in_room].zone+1].number * 100) )
    return FALSE;

  return TRUE;

}

void kill_gain(P_char ch, P_char victim);
void die(P_char ch, P_char killer)
{
  char buf[MAX_STRING_LENGTH], abuf[10], buf2[MAX_STRING_LENGTH];
  P_char tmp_ch, eth_ch;
  P_obj tempobj;
  struct affected_type *af, *next_af;
  P_obj corpse = NULL;
  int loss = 0, diff, x, i, j;

  if( !ch )
  {
    logit(LOG_EXIT, "die called in fight.c with no ch");
    raise(SIGSEGV);
  }

  if( !killer )
  {
    return;
  }

  // Upon death, we want to kill followers.
  if( IS_PC(ch) && ch->followers )
  {
    do_dismiss(ch, NULL, CMD_DEATH);
  }

  int oldlev = GET_LEVEL(ch);

#if defined(CTF_MUD) && (CTF_MUD == 1)
  if (affected_by_spell(ch, TAG_CTF))
  {
    int stat = GET_STAT(ch);
    SET_POS(ch, GET_POS(ch) + STAT_NORMAL);
    drop_ctf_flag(ch);
    SET_POS(ch, GET_POS(ch) + stat);
  }
#endif

  REMOVE_BIT(ch->specials.affected_by3, AFF3_PALADIN_AURA);
  clear_links(ch, LNK_PALADIN_AURA);

  /* switched god */
  if(ch->desc && ch->desc->original && ch->desc->original->only.pc->switched)
  {
    do_return(ch, 0, -4);
  }

  ch = ForceReturn(ch);
  /* count xp gained by killer */

  /* make mirror images disappear */
  if(IS_ALIVE(ch) && IS_NPC(ch) && GET_VNUM(ch) == 250)
  {
    if( ch == killer )
    {
      act("&+LYou disappear into thin air.&n", TRUE, ch, 0, 0, TO_CHAR);
      act("&+L$n&+L disappears into thin air.&n", TRUE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      act("Upon being struck, you disappear into thin air.", TRUE, ch, 0, 0, TO_CHAR);
      act("Upon being struck, $n disappears into thin air.", TRUE, ch, 0, 0, TO_ROOM);
    }
    extract_char(ch);
    return;
  }

  /* drop any disguise */
  if(IS_DISGUISE(ch))
  {
    remove_disguise(ch, TRUE);
  }

  if( check_outpost_death(ch, killer) )
  {
    return;
  }

  // PCs and NPCs without a die proc.  Note: Uses lazy eval since PC->specials.act ACT_SPEC_DIE
  //   is actually PLR_SMARTPROMPT (which isn't implemented as of 5/16/2015).
  if( IS_PC(ch) || !IS_SET(ch->specials.act, ACT_SPEC_DIE) )
  {
    act("$n is dead! &+RR.I.P.&n", TRUE, ch, 0, 0, TO_ROOM);
    // Only show death messages for hardcore characters - Arih
    if (IS_PC(ch) && IS_HARDCORE(ch))
    {
      act("&-L&+rYou feel yourself falling to the ground.&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&-L&+rYour soul leaves your body in the cold sleep of death...&n", FALSE, ch, 0, 0, TO_CHAR);
    }
    // Do nothing for PCs and !exp mobs.
    if( IS_PC(ch) || GET_EXP(ch) <= 0 )
    {
    }
    // Check Thanksgiving first.
    else if( GET_LEVEL(ch) > 44 && get_property("thanksgiving", 0.000)
      && (number(0, 100) < get_property("thanksgiving.turkey.chance", 5.000)) )
    {
      thanksgiving_proc(ch);
    }
    // Then Christmas.
    else if( GET_LEVEL(ch) > 35 &&get_property("christmas", 0.000)
      && (number(0, 100) < get_property("christmas.elf.chance", 5.000)) )
    {
      christmas_proc(ch);
    }
    // NPCs that are worth exp and not PC pets may load a random item.
    if( IS_NPC(ch) && !IS_PC_PET(ch) && GET_EXP(ch) > 0 )
      enhancematload(ch, killer);

  }

 /* This is where we were saving the newbies from being killed by high lvls.
  if( (IS_PC(ch)) && !IS_HARDCORE(ch) && ((GET_LEVEL(killer) - GET_LEVEL(ch)) > 15)
    && (IS_PC(killer) || (IS_NPC(killer) && IS_PC_PET(killer))) && (equipped_value(ch) < 250) )
   {
     newbie_reincarnate(ch);
     return;
   }
  */

  // For innate resurrection which I've never heard of.
  if( check_reincarnate(ch) )
  {
    return;
  }

  if( get_linked_char(ch, LNK_ETHEREAL) || get_linking_char(ch, LNK_ETHEREAL))
  {
    if( get_linked_char(ch, LNK_ETHEREAL) )
    {
      eth_ch = get_linked_char(ch, LNK_ETHEREAL);
    }
    else if(get_linking_char(ch, LNK_ETHEREAL))
    {
      eth_ch = get_linking_char(ch, LNK_ETHEREAL);
    }
    clear_links(eth_ch, LNK_ETHEREAL);
    clear_links(ch, LNK_ETHEREAL);
  }

  if( !killer )
  {
    return;
  }

  holy_crusade_check(killer, ch);
  soul_taking_check(killer, ch);

  // Changed the order on this to take advantage of C's lazy evaluation.
  // You don't get exp for killing someone who's LD?  Odd..
  if( ch && killer && (IS_NPC(ch) || ch->desc) && (killer != ch) && !IS_TRUSTED(killer) )
  {
    kill_gain(killer, ch);

    if( IS_NPC(ch) && IS_SET(ch->specials.act, ACT_ELITE) && GET_LEVEL(ch) > 49 )
    {
      group_gain_epic(killer, EPIC_ELITE_MOB, GET_VNUM(ch), (GET_LEVEL(ch) - 49));
    }
  }

  /* victim is pc */
  if( killer && IS_PC(ch) && !IS_TRUSTED(ch) && !IS_TRUSTED(killer) )
  {
    logit(LOG_DEATH, "%s killed by %s at %s", GET_NAME(ch),
      (IS_NPC(killer) ? killer->player.short_descr : GET_NAME(killer)), world[ch->in_room].name);
    statuslog(ch->player.level, "%s killed by %s at [%d] %s", GET_NAME(ch), (IS_NPC(killer)
      ? killer->player.short_descr : GET_NAME(killer)), world[ch->in_room].number, world[ch->in_room].name);

    // If killer is a PC, or a pet with master group and in room.. then we have PvP
    if( (IS_PC(killer) || ( IS_PC_PET(killer) && (GET_MASTER(killer)->in_room == killer->in_room)
      && killer->group && killer->group == GET_MASTER(killer)->group )) && (killer != ch) )
    {
      // It's important that this is before sql_save_pkill, 'cause we don't want to count this death as recent.
      setHeavenTime(ch);
      if( IS_PC_PET( killer ) )
      {
        if( opposite_racewar( ch, GET_MASTER(killer) ) )
        {
          debug( "die: ch: %s, killer %s, MASTER %s.", J_NAME(ch), J_NAME(killer), J_NAME(GET_MASTER(killer)));
          sql_save_pkill(GET_MASTER(killer), ch);
        }
      }
      else
      {
        if( opposite_racewar(ch, killer) )
        {
          sql_save_pkill(killer, ch);
        }
      }
    }
    if(CHAR_IN_ARENA(ch))
    {
      ;
    }
    else if( IS_PC(killer) && fragWorthy(killer, ch) && !affected_by_spell(ch, TAG_RECENTLY_FRAGGED) )
    {
      AddFrags(killer, ch);
    }
    else if( IS_PC_PET(killer) && fragWorthy(GET_MASTER(killer), ch) && !affected_by_spell(ch, TAG_RECENTLY_FRAGGED) )
    {
      AddFrags(killer, ch);
    }
  }

  if( IS_NPC(killer) && CAN_ACT(killer) && killer != ch && MIN_POS(killer, POS_STANDING + STAT_RESTING) )
  {
    add_event(retarget_event, PULSE_VIOLENCE - 1, killer, NULL, NULL, 0, NULL, 0);
  }
  /* count xp loss for victim and apply */
  if( IS_PC(ch) && !CHAR_IN_ARENA(ch) && (GET_LEVEL(ch) > 1) && !IS_TRUSTED(ch) )
    // && (GET_RACEWAR(ch) != 1)) Goods lose exp again.
  {
    if( IS_PC(ch) && (GET_RACE(ch) == RACE_LICH) )
    {
      long tmp = loss = GET_EXP(ch);
      float percentage = new_exp_table[GET_LEVEL(ch)] / new_exp_table[GET_LEVEL(ch)+1];
      lose_level(ch);
      // This is complicated because 10M exp at 51 is not the same as 10M exp at 50/52/etc.
      tmp = tmp * percentage;
      GET_EXP(ch) = MAX( 1, tmp );
      // Amount of exp lost is all exp to lose level + the portion lost into the level below.
      loss += new_exp_table[GET_LEVEL(ch)] - GET_EXP(ch);
      loss *= -1;
    }
    else
    {
      loss = gain_exp(ch, NULL, 0, EXP_DEATH);
    }
    debug( "&+RDeath&n: %s lost %d experience from death.", J_NAME(ch), loss );
  }

/*
  for (i = GET_LEVEL(ch) + 1; i > minlvl && (new_exp_table[i] <= GET_EXP(ch)); i++)
  {
    GET_EXP(ch) -= new_exp_table[i];
    advance_level(ch);
  }
*/

  if(IS_PC(killer))
  {
    nq_char_death(killer, ch);
  }
  if (GET_OPPONENT(ch))
  {
    stop_fighting(ch);
  }
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  StopAllAttackers(ch);

  REMOVE_BIT(ch->specials.act2, PLR2_WAIT);

  if(!ch || !killer)
  {
    return;
  }

  if( IS_PC(ch) || !IS_SET(ch->specials.act, ACT_SPEC_DIE) )
  {
    if( !CAN_SPEAK(ch) )
    {
      death_rattle(ch);
    }
    else
    {
      death_cry(ch);
    }
  }

  // Dragon mobs now will drop a dragon scale
  // No longer includes !exp mobs like dragon illusions.
  if( GET_RACE(ch) == RACE_DRAGON && GET_EXP(ch) > 0 )
  {
    P_obj tempobj = read_object(VOBJ_DRAGON_SCALE, VIRTUAL);
    obj_to_char(tempobj, ch);
  }

  if(IS_NPC(ch) && (GET_LEVEL(ch) > 51) && !IS_PC_PET(ch) && !affected_by_spell(ch, TAG_CONJURED_PET)) //soul shard - Drannak
  {
    int dchance = 5;

    if(IS_ELITE(ch))
      dchance +=5;

    if(number(1, 250) < dchance)
    {
      P_obj teobj = read_object(400230, VIRTUAL);
      obj_to_char(teobj, ch);
      act("&+LAs &+R$n &+Lfalls to the ground, a small shard of their &+rlifeforce&n manifests&+L.&N",
          FALSE, ch, 0, 0, TO_ROOM);
    }
  }

  //possibility to find a recipe for the items in the zone.
  // Only find recipes from mobs inside their own zone.
  if( IS_PC(killer) && !IS_PC_PET(ch) && in_their_zone(ch)
    && !affected_by_spell(ch, TAG_CONJURED_PET) )
    random_recipe(killer, ch);

  // object code - Normal kills.  Kvark
  if((IS_PC(killer) || IS_PC_PET(killer)) && IS_NPC(ch) && IS_ALIVE(killer))
  {

    // if(GET_LEVEL(ch) < 30 || GET_LEVEL(killer) < 20)
    //   {
    if(check_random_drop(killer, ch, TRUE))
    {
      if(!number(0, 25))// &&
        // (GET_LEVEL(ch) > 51))
      {
        tempobj = create_stones(ch);
      }
      else
      {
        //if(GET_LEVEL(ch) < 30 || GET_LEVEL(killer) < 20) //removing level restriction, adding racewar check goods only - drannak
        //if(GET_RACEWAR(killer) == RACEWAR_GOOD || GET_RACEWAR(killer) == RACEWAR_EVIL) //allowing evils to get randoms 1/26/13 drannak
        if(GET_LEVEL(killer) > 0)
        {
          tempobj = create_random_eq_new(killer, ch, -1, -1);
          send_to_char("It appears you were able to salvage a piece of equipment from your enemy.\n", killer);
        }
        else
        {
          tempobj = create_material(killer, ch);
          send_to_char("It appears you were able to salvage a piece of material from your enemy.\n", killer);
        }
      }
      if(tempobj && ch)
      {
        obj_to_char(tempobj, ch);
      }
    }
    if(check_random_drop(killer, ch, FALSE))
    {
      if(!number(0, 25))// &&
        // (GET_LEVEL(ch) > 51))
      {
        tempobj = create_stones(ch);
      }
      else
      {
        tempobj = create_material(killer, ch);
      }

      if(tempobj)
      {
        obj_to_char(tempobj, ch);
      }
    }
    // }
  }

  update_pos(ch);
  SET_POS(ch, GET_POS(ch) + STAT_DEAD);
  update_pos(ch);

  if(!CHAR_IN_ARENA(ch) || IS_NPC(ch))
  {
    //world quest hook
    if((IS_PC(killer) || IS_PC_PET(killer)) && killer->in_room >= 0 && !affected_by_spell(ch, TAG_CONJURED_PET))
    {

      quest_kill(killer , ch);
      if (killer->group)
      {
        for(tmp_ch = world[killer->in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
        {
          if(killer->group == tmp_ch->group && tmp_ch != killer && tmp_ch != ch)
          {
            quest_kill(tmp_ch , ch);
          }
        }
      }
    }

    if(IS_NPC(ch) && (ch->specials.act & ACT_SPEC_DIE) && (ch->specials.act & ACT_SPEC))
    {
      if(!mob_index[GET_RNUM(ch)].func.mob)
      {
        snprintf(buf, MAX_STRING_LENGTH, "%s %d", ch->player.name, GET_RNUM(ch));
        logit(LOG_STATUS, buf);
        logit(LOG_STATUS, "No special function for ACT_SPEC_DIE");
        REMOVE_BIT(ch->specials.act, ACT_SPEC_DIE);
        corpse = make_corpse(ch, loss);
      }
      else
      {
        (*mob_index[GET_RNUM(ch)].func.mob) (ch, killer, CMD_DEATH, 0);
        // If this mob has been extracted and been given a release mem event.
        if( ch->nevents && ch->nevents->func == release_mob_mem )
          return;
      }
    }
    else
    {
      corpse = make_corpse(ch, loss);
    }

    if( corpse && killer != ch && ( has_innate(killer, INNATE_MUMMIFY) || has_innate(killer, INNATE_REQUIEM)))
    {
      mummify(killer, ch, corpse);
    }

    if(corpse && killer != ch && ( affected_by_spell(killer, SPELL_SPAWN)
      || has_innate(killer, INNATE_SPAWN) || has_innate(killer, INNATE_ALLY)))
    {
      // Original: if((IS_NPC(ch) && !number(0, 2)) || !number(0, 3))
      //   !0,2 -> 1/3 || !0,3 -> 1/4 --> 1/3 || 1/4 == 1/3 + 2/3 * 1/4 == 1/3 + 1/6 == 3 / 6 == 1/2
      //   So, changing "!number(0, 2)) || !number(0, 3)" to "!number(0, 1)"
      //   Below makes more sense 'cause 1/2 NPC and 1/4 PC.
      //   But IS_PC check on NPCs 1/2 the time, and no longer 2 calls to number() for NPCs 1/2 the time.
      //   Conclusion: IS_PC == 2 dereferences (->specials.act) + bit check VS number() == 3 nested function calls+
      //     so should be same on NPC 1/2 the time and faster the other 1/2 and slower on a PC all the time.
      //     A lot more NPCs die than PCs, and the slowdown is just the 2 dereferences on a PC 100% of the time,
      //     so should be faster overall.
      if( (IS_NPC(ch) && !number(0, 1)) || (IS_PC(ch) && !number(0, 3)) )
      {
        if( IS_PC(killer) && !affected_by_spell(killer, SPELL_SPAWN) && !affected_by_spell(killer, TAG_SPAWN) )
        {
          send_to_char("You are not willing to summon pets from death blows.\n", killer);
        }
        else
          spawn_raise_undead(killer, ch, corpse);
      }
    }

    if(IS_PC(ch))
    {
      if(!CHAR_IN_ARENA(ch))
      {
        ch->only.pc->numb_deaths++;
      }
      // Hardcore chars die after 5 deaths.
      if((ch->only.pc->numb_deaths > 0) &&
          IS_HARDCORE(ch))
      {
        update_pos(ch);
        checkHallOfFame(ch, GET_NAME(killer));
        act("&+LThe &+rhand &+Lof &+WGod &+Lgrabs &+R$n &+Lby the &+cthroat&+L.&N",
            FALSE, ch, 0, 0, TO_ROOM);
        act("&+LThe &+rhand &+Lof &+WGod &+Ltears &+R$n&+L's &+wsoul &+Lfrom this &+cplane &+Lof existence.&N",
            FALSE, ch, 0, 0, TO_ROOM);
        act("&+L$n's &+cbody &+Llands on the ground in a crumpled heap, &+wsoul &+Lgone forever.&N",
            FALSE, ch, 0, 0, TO_ROOM);

        send_to_char
          ("&+LThe &+rhand &+Lof &+WGod &+Lgrabs &+RYou &+Lby the &+cthroat&+L.&N\r\n",
           ch);
        send_to_char
          ("&+LThe &+rhand &+Lof &+WGod &+Ltears &+RYour&+L &+wsoul &+Lfrom this &+cplane &+Lof existence.&N\r\n",
           ch);
        send_to_char
          ("&+WGod&+L stands before you and says '&+wYou have died your last death, Your existence shall not continue.&+L'\r\n",
           ch);
        send_to_char
          ("&+WGod&+L says '&+wYou have died a miserable soul with a value of &+R&+w.&+L'&N\r\n",
           ch);

        statuslog(ch->player.level,
            "%s's existence on Duris was just ended...by %s!",
            GET_NAME(ch), GET_NAME(killer));

        // If it's not an immortal.
        if( GET_LEVEL(ch) < MINLVLIMMORTAL )
        {
          update_ingame_racewar( -GET_RACEWAR(ch) );
        }

#ifdef USE_ACCOUNT
        // With account system, return to account menu instead of disconnecting
        if (ch->desc && ch->desc->account)
        {
          P_desc d = ch->desc;

          // Send death messages BEFORE showing account menu - Arih
          send_to_char("&-L&+rYou feel yourself falling to the ground.&n\r\n", ch);
          send_to_char("&-L&+rYour soul leaves your body in the cold sleep of death...&n\r\n\r\n", ch);

          // Delete character file
          deleteCharacter(ch);

          // Manually disconnect descriptor before calling extract_char to prevent
          // extract_char from showing account menu and calling free_char - Arih
          ch->desc = NULL;

          // extract_char cleans up followers, equipment, etc.
          // With ch->desc = NULL, it won't show menu or call free_char
          extract_char(ch);

          // Now manually free and show our custom menu
          free_char(ch);

          // Return to account menu
          d->character = NULL;
          d->term_type = TERM_ANSI;  // Preserve ANSI mode
          SEND_TO_Q("&+RYour character has been permanently deleted.&n\r\n\r\n", d);
          STATE(d) = CON_DISPLAY_ACCT_MENU;
          display_account_menu(d, NULL);
          return;
        }
#endif

        // Without account system, or no descriptor, just disconnect
        if (ch->desc)
        {
          close_socket(ch->desc);
        }
        extract_char(ch);
        deleteCharacter(ch);
        free_char(ch);
        ch = NULL;

        return;
      }

      // This code has never been tested, so commenting out. Torgal 11/21/12
      //if (!IS_PC_PET(ch))
      //{
      //	check_boon_completion(killer, ch, 0, BOPT_MOB);
      //	check_boon_completion(killer, ch, 0, BOPT_RACE);
      //}    

      GET_MANA(ch) = 0;

      for(af = ch->affected; af; af = next_af)
      {
        next_af = af->next;
        if (!(af->flags & AFFTYPE_PERM))
        {
          affect_remove(ch, af);
        }
        else if (af->type == TAG_MEMORIZE)
        {
          af->flags &= ~MEMTYPE_FULL;
        }
      }

      check_saved_corpse(ch);

      ClearCharEvents(ch);
      ch->specials.conditions[DISEASE_TYPE] = 0;
      ch->specials.conditions[POISON_TYPE] = 0;

      /* remove all undead/druid/harpy spells */
      for(i = 1; i <= MAX_CIRCLE; i++)
      {
        ch->specials.undead_spell_slots[i] = 0;
      }

      update_pos(ch);
      SET_POS(ch, STAT_DEAD + GET_POS(ch));
      update_pos(ch);

      /* even things out so it doesn't barf when removing perm affect items */
      affect_total(ch, FALSE);
    }
  }
  /*
   * this WAS in extract_char, but only death removes you from mobs mem
   * now. renting won't do it, as long as the game is up and neither
   * char nor mob dies, they WILL remember!
   */

  if(IS_PC(ch) && !CHAR_IN_ARENA(ch))
  {
    P_char   mob, mob_next;

    for (mob = character_list; mob; mob = mob_next)
    {
      mob_next = mob->next;
      forget(mob, ch);
    }
  }
  if(IS_PC(ch))
  {
    REMOVE_BIT(ch->specials.act2, PLR2_SPEC_TIMER);
    GET_HIT(ch) = 1;
    if(!CHAR_IN_ARENA(ch))
    {
      writeCharacter(ch, 4, NOWHERE);
    }
    ch->only.pc->pc_timer[1] = 0;       // reset flee timer
  }

  add_track(ch, NUM_EXITS);

  if(!CHAR_IN_ARENA(ch) || IS_NPC(ch))
  {
    if(IS_NPC(ch))
    {
      extract_char(ch);
      ch = NULL;
    }
    else
    {
      // If it's not an immortal.
      if( GET_LEVEL(ch) < MINLVLIMMORTAL )
      {
        update_ingame_racewar( -GET_RACEWAR(ch) );
      }
      extract_char(ch);
      if (!ch->desc)
      {
        free_char(ch);
      }
    }
  }
  else
  {
    int      i, nr;
    char     strn[MAX_STRING_LENGTH];

    if(ch == killer)
    {
      snprintf(strn, MAX_STRING_LENGTH, "&+W%s killed %s own dumb self!&N\r\n", GET_NAME(ch),
          HSHR(ch));
      send_to_arena(strn, -1);
      ARENA_PLAYER(ch).frags -= 1;
    }
    else
    {
      snprintf(strn, MAX_STRING_LENGTH, "&+R%s was %s by %s!&N\r\n", GET_NAME(ch),
          arena_death_msg(killer->equipment[WIELD]), GET_NAME(killer));
      send_to_arena(strn, -1);
      ARENA_PLAYER(killer).frags += 2;
      arena.team[arena_team(killer)].score += 1;
      send_to_char("&+GYou gain 2 frags for scoring a primary kill!&N\r\n",  killer);
      if (arena.type != TYPE_DEATHMATCH ||
          arena.type != TYPE_KING_OF_THE_HILL)
      {
        for (tmp_ch = world[killer->in_room].people; tmp_ch;
            tmp_ch = tmp_ch->next_in_room)
        {
          if (killer->group)
          {
            if (killer->group == tmp_ch->group && tmp_ch != ch &&
                killer != tmp_ch)
            {
              if (arena_id(tmp_ch) != -1)
              {
                ARENA_PLAYER(tmp_ch).frags += 1;
                send_to_char("&+GYou gain a frag from an assist kill!&N\r\n",
                    ch);
              }
            }
          }
        }
      }
    }

    /* figure out which arena ch is in */
    if(arena_id(ch) == -1)
    {
      GET_HIT(ch) = GET_MAX_HIT(ch);
      update_pos(ch);
      send_to_char("&+WYou do not belong in the arena!&N\r\n", ch);
      char_from_room(ch);
      char_to_room(ch, real_room(ch->player.birthplace), -1);
      return;
    }

    switch (arena.deathmode)
    {
      case DEATH_TOLL:
      case DEATH_EVEN_TRADE:
      case DEATH_WINNER_TAKES_ALL:
      case DEATH_FREE:
        char_from_room(ch);
        if (ARENA_PLAYER(ch).lives > 0)
        {
          ARENA_PLAYER(ch).lives--;
          arena_char_spawn(ch);
          snprintf(strn, MAX_STRING_LENGTH,
              "&+WYou breathe new air as you respawn.\r\nLives remaining: %d\r\n",
              ARENA_PLAYER(ch).lives);
          send_to_char(strn, ch);
        }
        else
        {
          char_to_room(ch, real_room(arena_hometown_location[arena_team(ch)]),
              -1);
          snprintf(strn, MAX_STRING_LENGTH, "&+C%s has been vanquished!\r\n&N", GET_NAME(ch));
          send_to_arena(strn, -1);
          SET_BIT(ARENA_PLAYER(ch).flags, PLAYER_DEAD);
        }
        GET_HIT(ch) = GET_MAX_HIT(ch);
        update_pos(ch);
        break;
      default:
        break;
    }
    if (arena_team_count(arena_team(ch)) < 1)
    {
      if(arena.type != TYPE_DEATHMATCH ||
          arena.type != TYPE_KING_OF_THE_HILL)
      {
        send_to_arena("&+LOne side has been completely vanquished!&N\r\n", -1);
        arena.stage++;
      }
    }

    return;
    /*
       for (i = 0; i <= LAST_HOME; i++)
       {
       nr = world[ch->in_room].number;

       if ((nr >= hometown_arena[i][1]) &&
       (nr <= hometown_arena[i][2]))
       {
       char_from_room(ch);
       char_to_room(ch, real_room(hometown_arena[i][0]), -1);

       GET_HIT(ch) = GET_MAX_HIT(ch) / 4;
       update_pos(ch);

       act("&+W$n suddenly appears with a flash.", TRUE, ch, 0, 0, TO_ROOM);
       send_to_char("&+WYou are restored to health, albeit a bit disoriented.\r\n", ch);

       CharWait(ch, 30);

       return;
       }
       }
       */
    send_to_char("couldn't find yer arena, this is bad.\r\n", ch);
  }
}
// end of die

void kill_gain(P_char ch, P_char victim)
{
  int gain, XP;
  struct group_list *gl;
  int group_size = 0;
  int highest_level = 0;
  struct affected_type *afp;

  if( IS_PC(victim) )
  {
    if( GOOD_RACE(ch) && GOOD_RACE(victim) || EVIL_RACE(ch) && EVIL_RACE(victim) )
    {
      gain = 1;
    }
    else
    {
      gain = (new_exp_table[GET_LEVEL(victim)] / 2);
    }
  }
  else
  {
    gain = GET_EXP(victim);
  }

  if( !ch->group )
  {
    send_to_char("You receive your share of experience.\r\n", ch);
    gain_exp(ch, victim, gain, EXP_KILL);
    if(IS_PC(ch))
      add_bloodlust(ch, victim);

    // This is for all kinds of kill-type achievements
    update_achievements(ch, victim, 0, 2);

    // Addicted to blood stuff:
    update_addicted_to_blood(ch, victim);

    if((GET_LEVEL(victim) > 30) && !IS_PC(victim) && !affected_by_spell(victim, TAG_CONJURED_PET))
    {
      if((number(1, 5000) < GET_C_LUK(ch)) || (GET_RACE(victim) == RACE_DRAGON && (GET_VNUM(victim) > 10) && (GET_VNUM(victim) != 1108) && (GET_LEVEL(victim) > 49)))
      {
        send_to_char("&+cAs your body absorbs the &+Cexperience&+c, you seem to feel a bit more epic!\r\n", ch);
        gain_epic(ch, EPIC_RANDOMMOB, 0, 1 );
      }
    }
    change_alignment(ch, victim);

    if(!IS_PC(victim) && affected_by_spell(victim, SPELL_CONTAIN_BEING) && GET_CLASS(ch, CLASS_SUMMONER) && IS_SPECIALIZED(ch) && IS_PC(ch))
    {
      if(!valid_conjure(ch, victim))
      {
        send_to_char("You cannot learn to summon a being outside of your area of expertise.\r\n", ch);
        return;
      }
      else
      {
        if(ch && victim)
        {
          int chance = BOUNDED(1, GET_C_CHA(ch), 230);
          chance -= GET_LEVEL(victim);
          if((number(1, GET_C_INT(victim)) < chance) && (GET_VNUM(victim) != 1255))
            learn_conjure_recipe(ch, victim);
        }
      }
    }
    return;
  }

  for (gl = ch->group; gl; gl = gl->next)
  {
    // Ppl out of room still count against exp gain? Erm... no
    if( IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) && (ch->in_room == gl->ch->in_room) )
    {
      group_size++;
    }

    if( IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) && (ch->in_room == gl->ch->in_room)
      && GET_LEVEL(gl->ch) > highest_level )
    {
        highest_level = GET_LEVEL(gl->ch);
    }
  }


  /* This prevents group from ganking solo racewar victims and gaining
  // tremendous exps. For PVP, the exps are divided amongst the group.
  if((IS_PC(ch) ||
  IS_PC_PET(ch)) &&
  IS_PC(victim))
  exp_divider = MAX(group_size, 1);
  else
  exp_divider = 1; // enabled and tweaked exp divider  -Odorf */

  /*if( ( RACE_GOOD(ch) && get_property("exp.groupLimit.good", 10) &&
    group_size > get_property("exp.groupLimit.good", 10) ) ||
    ( RACE_EVIL(ch) && get_property("exp.groupLimit.evil", 8) &&
    group_size > get_property("exp.groupLimit.evil", 8) ) )
    {
    exp_divider *= 10;
    }  //removed group cap for exp  -Odorf*/

  // Group exp groupexp function - For searching.
  // exp gain drops slower than group size increases to avoid people being unable to get in groups  -Odorf
  // +2/3 - Groupsize:exp ratio - 1: 1, 2: 3/4, 3: 3/5, 4: 1/2, 5: 3/7, .. , 10:1/4, .. , 16:1/6.. and so on.
  // +3/4 - Groupsize:exp ratio - 1: 1, 2: 4/5, 3: 2/3, 4: 4/7, 5: 1/2, .. ,  9:1/3, .. , 17:1/5.. and so on.
  float exp_divider = ((float)group_size + 3.0) / 4.0;

  for( gl = ch->group; gl; gl = gl->next )
  {
    if( IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) && (gl->ch->in_room == ch->in_room) )
    {
      XP = (int) (((float)GET_LEVEL(gl->ch) / (float)highest_level) * ((float)gain / exp_divider));

      /* power leveler stopgap measure */
      if( (GET_LEVEL(gl->ch) + 40) < highest_level )
        XP /= 5000;
      else if( (GET_LEVEL(gl->ch) + 30) < highest_level )
        XP /= 1000;
      else if( (GET_LEVEL(gl->ch) + 20) < highest_level )
        XP /= 150;
      else if( (GET_LEVEL(gl->ch) + 15) < highest_level )
        XP /= 40;

      if( XP && IS_PC(gl->ch) )
      {
        logit(LOG_EXP, "%s: %d, group kill of: %s [%d]", GET_NAME(gl->ch), XP,
            GET_NAME(victim), ( IS_NPC(victim) ? GET_VNUM(victim) : 0));
      }

      send_to_char("You receive your share of experience.\r\n", gl->ch);
      gain_exp(gl->ch, victim, XP, EXP_KILL);
      if( IS_PC(gl->ch) )
      {
        add_bloodlust(gl->ch, victim);
      }
      //this is for all kinds of kill-type quests
      update_achievements(gl->ch, victim, 0, 2);

      // Addicted to blood stuff:
      update_addicted_to_blood(gl->ch, victim);

      if((GET_LEVEL(victim) > 30) && !IS_PC(victim) && !affected_by_spell(victim, TAG_CONJURED_PET))
      {
        if((number(1, 5000) < GET_C_LUK(gl->ch)) || (GET_RACE(victim) == RACE_DRAGON && (GET_VNUM(victim) != 1108) && (GET_VNUM(victim) > 10) && (GET_LEVEL(victim) > 49)))
        {
          send_to_char("&+cAs your body absorbs the &+Cexperience&+c, you seem to feel a bit more epic!\r\n", gl->ch);
          P_char recipient = gl->ch;
          gain_epic(gl->ch, EPIC_RANDOMMOB, 0, 1 );
        }
      }

      change_alignment(gl->ch, victim);

      if(!IS_PC(victim) && affected_by_spell(victim, SPELL_CONTAIN_BEING) && GET_CLASS(gl->ch, CLASS_SUMMONER) && IS_SPECIALIZED(gl->ch) && IS_PC(gl->ch))
      {
        if(!valid_conjure(gl->ch, victim))
        {
          send_to_char("You cannot learn to summon a being outside of your area of expertise.\r\n", gl->ch);
        }
        else
        {
          if(gl->ch && victim)
          {
            int chance = BOUNDED(1, GET_C_CHA(gl->ch), 230);
            chance -= GET_LEVEL(victim);
            if((number(1, GET_C_INT(victim)) < chance) && (GET_VNUM(victim) != 1255))
              learn_conjure_recipe(gl->ch, victim);
          }
        }
      }
    }
  }
}

void dam_message(double fdam, P_char ch, P_char victim, struct damage_messages *messages)
{
  int      dam = (int) fdam;
  P_obj    wield;
  P_obj    wpn;
  char    *buf, buf_char[160], buf_vict[160], buf_notvict[160];
  int      w_percent, h_percent, max_dam = 0, w_loop, h_loop, dam2;
  int      msg_flags = messages->type;
  static int dam_ref[] = { 0, 2, 7, 10, 15, 25, 40, 55, 70, 85, 9999 };
  const char *weapon_damage[] = {
    "",
    " feeble",
    " weak",
    " crude",
    " decent",
    " fine",
    " impressive",
    " powerful",
    " mighty",
    " awesome",
    " amazing",
  };

  const char *victim_damage[] = {
    "grazes $w",
    "wounds $w",
    "strikes $w",
    "strikes $w hard",
    "strikes $w very hard",
    "seriously wounds $w",
    "enshrouds $w in a mist of blood",
    "causes $w to grimace in pain",
    "grievously wounds $w",
    "critically injures $w",
    "hits $w"
  };

  const char *victim_damage2[] = {
    "graze $w",
    "wound $w",
    "strike $w",
    "strike $w hard",
    "strike $w very hard",
    "seriously wound $w",
    "cause $w to grimace in pain",
    "enshroud $w in a mist of blood",
    "grievously wound $w",
    "critically injure $w",
    "hit $w"
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( ch->equipment[WIELD] )
  {
    wield = ch->equipment[WIELD];
    max_dam = ch->equipment[WIELD]->value[1] * ch->equipment[WIELD]->value[2];
  }
  // else if(messages->obj)
  // {
  // wield = messages->obj;

  // if((messages->obj->value[1] * messages->obj->value[2]) >= 0)
  // max_dam = messages->obj->value[1] * messages->obj->value[2];
  // else
  // max_dam = 0;
  // }
  else
  {
    max_dam = ch->points.damnodice * ch->points.damsizedice;
  }

  h_percent = BOUNDED(0, (int) ((dam * 100) / (GET_HIT(victim) + dam + 10)), 100);
  w_percent = BOUNDED(0, (int) ((dam * 100) / (max_dam + dam + 10)), 100);

  for (h_loop = 0; (h_percent > dam_ref[h_loop]); h_loop++) ;
  if (h_loop > 10)
    h_loop = 10;                /* h and w require reverse. dont ask why */
  for (w_loop = 0; (w_percent > dam_ref[w_loop]); w_loop++) ;
  if (w_loop > 10)
    w_loop = 0;

  if (!number(0, 3))
    w_loop = 0;

/* 
  char showdam[MAX_STRING_LENGTH];
  snprintf(showdam, MAX_STRING_LENGTH, " [&+wDamage: %d&n] ", dam);
*/
  if (msg_flags & DAMMSG_HIT_EFFECT)
  {
    snprintf(buf_char, 160, messages->attacker, weapon_damage[w_loop],
        victim_damage[h_loop]);
    snprintf(buf_vict, 160, messages->victim, weapon_damage[w_loop],
        victim_damage[h_loop]);
    snprintf(buf_notvict, 160, messages->room, weapon_damage[w_loop],
        victim_damage[h_loop]);
  }
  else if (msg_flags & DAMMSG_EFFECT_HIT)
  {
    snprintf(buf_char, 160, messages->attacker, victim_damage2[h_loop],
        weapon_damage[w_loop]);
    snprintf(buf_vict, MAX_STRING_LENGTH, messages->victim, victim_damage[h_loop],
        weapon_damage[w_loop]);
    snprintf(buf_notvict, MAX_STRING_LENGTH, messages->room, victim_damage[h_loop],
        weapon_damage[w_loop]);
  }
  else if ((msg_flags & DAMMSG_EFFECT))
  {
    snprintf(buf_char, MAX_STRING_LENGTH, messages->attacker, victim_damage[h_loop]);
    snprintf(buf_vict, MAX_STRING_LENGTH, messages->victim, victim_damage[h_loop]);
    snprintf(buf_notvict, MAX_STRING_LENGTH, messages->room, victim_damage[h_loop]);
  }
  else if (msg_flags & DAMMSG_HIT)
  {
    snprintf(buf_char, MAX_STRING_LENGTH, messages->attacker, weapon_damage[w_loop]);
    snprintf(buf_vict, MAX_STRING_LENGTH, messages->victim, weapon_damage[w_loop]);
    snprintf(buf_notvict, MAX_STRING_LENGTH, messages->room, weapon_damage[w_loop]);
  }
  /* if (IS_PC(ch) && IS_SET(ch->specials.act2, PLR2_DAMAGE) )
     strcat(buf_char, showdam);*/

#if ENABLE_TERSE
  act(buf_notvict, FALSE, ch, messages->obj, victim,
      TO_NOTVICTROOM | ACT_NOTTERSE);
#else
  act(buf_notvict, FALSE, ch, messages->obj, victim, TO_NOTVICTROOM);
#endif

  if (IS_PC(ch) && IS_SET(ch->specials.act2, PLR2_BATTLEALERT) &&
      !IS_SET(ch->specials.act2, PLR2_TERSE))
  {
    strcat(buf_char, "&+G]=-&N");
    send_to_char("&+G-=[&N", ch);
  }
#if ENABLE_TERSE
  act(buf_char, FALSE, ch, messages->obj, victim, TO_CHAR | ACT_NOTTERSE);
#else
  act(buf_char, FALSE, ch, messages->obj, victim, TO_CHAR);
#endif

  if (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_BATTLEALERT) &&
      !IS_SET(victim->specials.act2, PLR2_TERSE))
  {
    strcat(buf_vict, "&+R]=-&N");
    send_to_char("&+R-=[&N", victim);
  }
#if ENABLE_TERSE
  act(buf_vict, FALSE, ch, messages->obj, victim, TO_VICT | ACT_NOTTERSE);
#else
  act(buf_vict, FALSE, ch, messages->obj, victim, TO_VICT);
#endif
}

/*
 * used for stone skin type spells, decreases modifier by 1 and
 * shows wear off message when it reaches 0
 * function returns true if a corresponding affect structure was found
 */
bool decrease_skin_counter(P_char ch, unsigned int skin)
{
  struct affected_type *af, *af2;

  for (af = ch->affected; af; af = af2)
  {
    af2 = af->next;
    if (af->type == skin)
    {
      af->modifier--;
      if (af->modifier <= 0)
      {
        wear_off_message(ch, af);
        affect_remove(ch, af);
      }
      return TRUE;
    }
  }

  return FALSE;
}

// Attempt by a berserker to slam weapon aside and deal damage, possibly disarming opponent in the process.
// Similar to riposte, does not require a successful parry though.
// ch is the mangler, and victim is the person attacking (that might be mangled).  It's important
//   to have it this way for the help files to be correct.
bool mangleSucceed( P_char ch, P_char victim, P_obj weap )
{
  bool skill_notch = FALSE;
  int skl, wloc;
  struct damage_messages messages =
  {
    "You &+rmangle&n $S forearm with $q.",
    "$n &+rmangles&n you with $q.",
    "$n &+rmangles&n $N with $q.",
    "You &+Rmangle&N $N to death.",
    "$n &+Rmangles&n you to death.",
    "$n &+Rmangles&n $N to death.",
    DAMMSG_TERSE, weap
  };

  // Can't mangle vs a non-weapon.
  if( !weap || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
  {
    return FALSE;
  }

  // Make sure weap is wielded.
  if( weap == victim->equipment[WIELD] )
  {
    wloc = WIELD;
  }
  else if( weap == victim->equipment[WIELD2] )
  {
    wloc = WIELD2;
  }
  else if( weap == victim->equipment[WIELD3] )
  {
    wloc = WIELD3;
  }
  else if( weap == victim->equipment[WIELD4] )
  {
    wloc = WIELD4;
  }
  else
    return FALSE;

  /* Checked above now, and has weapon as argument to function.
  // Find a weapon to disarm (must exist and be a weapon and not be cursed).
  if( (weap = victim->equipment[WIELD]) == NULL || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
    if( (weap = victim->equipment[WIELD2]) == NULL || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
      if( (weap = victim->equipment[WIELD3]) == NULL || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
        if( (weap = victim->equipment[WIELD4]) == NULL || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
          return FALSE;
  */

  skl = GET_CHAR_SKILL(ch, SKILL_MANGLE);

  if( IS_TRUSTED(victim) || IS_IMMOBILE(ch) || !IS_HUMANOID(victim) || !MIN_POS(ch, POS_STANDING + STAT_NORMAL)
    || (skl < 20 && !( skill_notch = notch_skill(ch, SKILL_MANGLE, get_property( "skill.notch.defensive", 17 )) )) )
  {
    return FALSE;
  }

  if( !skill_notch )
  {
    // Epic parry now reduces mangle percentage. Jan08 -Lucrot
    if( GET_CHAR_SKILL(victim, SKILL_EXPERT_PARRY) )
    {
      if( (skl -= GET_CHAR_SKILL( victim, SKILL_EXPERT_PARRY ) / 2) < 20 )
      {
        return FALSE;
      }
    }

    // Elite mobs are not affected as much. Jan08 -Lucrot
    if( IS_ELITE(victim) )
    {
      skl = 20;
    }

    if( IS_ELITE(ch) )
    {
      skl += 100;
    }

    if( (skl -= ( GET_C_DEX(victim) - GET_C_DEX(ch) ) * 5) < 20 )
    {
      return FALSE;
    }

    skl /= 20;
    skl = BOUNDED(0, skl, 5);

    // 5% max chance.
    if( number(1, 100) > skl )
    {
      return FALSE;
    }
  }

  act("$n blocks your attack and slashes viciously at your arm.", TRUE, ch, 0, victim, TO_VICT);
  act("$n blocks $N's attack and slashes viciously at $S arm.", TRUE, ch, 0, victim, TO_NOTVICT);
  act("You block $N's attack and slash viciously at $S arm.", TRUE, ch, 0, victim, TO_CHAR);

  // 2-8 damage - can be mitigated.
  if( melee_damage(ch, victim, 4. * dice(2, 4), PHSDAM_NONE, &messages) != DAM_NONEDEAD )
    return TRUE;

  // 25% chance to actually disarm.
  if( weap && !number(0, 3) && !affected_by_spell(victim, SPELL_COMBAT_MIND) )
  {
    send_to_char("&=LYYou swing at your foe _really_ badly, losing control of your weapon!\r\n", victim);
    act("$n stumbles with $s attack, losing control of $s weapon!", TRUE, victim, 0, 0, TO_ROOM);

    set_short_affected_by(victim, SKILL_DISARM, 2 * PULSE_VIOLENCE);

    unequip_char(victim, wloc);
    obj_to_char(weap, victim);

    char_light(victim);
    room_light(victim->in_room, REAL);
  }
  else
  {
    send_to_char("You stumble, but recover in time!\r\n", victim);
  }

  return TRUE;
}

int try_riposte(P_char ch, P_char victim, P_obj wpn)
{
  int expertriposte = 0, victim_dead;
  int randomnumber = number(1, 1000);
  double skl;
  bool npcepicriposte = FALSE;

  if( !IS_ALIVE(victim) || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  // Innate two daggers is static at 5%.
  if( innate_two_daggers(ch) && !number(0, 19) )
  {
    act("$n flourishes $s dagger, intercepting $N's attack, and gracefully counters!",
        TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n thrusts forth $s dagger, intercepting your attack, and counters it gracefully!",
        TRUE, ch, 0, victim, TO_VICT);
    act("You brandish your offhand dagger, intercepting $N's blow, and countering his attack!",
        TRUE, ch, 0, victim, TO_CHAR);

    hit(ch, victim, ch->equipment[PRIMARY_WEAPON]);
    if( char_in_list(victim) && char_in_list(ch) )
    {
      hit(ch, victim, ch->equipment[SECONDARY_WEAPON]);
    }
    return TRUE;
  }


  // No longer able to riposte without the skill.
  if( (skl = GET_CHAR_SKILL(ch, SKILL_RIPOSTE)) < 1 )
  {
    return FALSE;
  }

  // Notching the skill means failing the riposte.
  if( notch_skill(ch, SKILL_RIPOSTE, get_property("skill.notch.defensive", 17)) )
  {
    return FALSE;
  }

  // Skill range is 1 to 100.
  expertriposte = GET_CHAR_SKILL(ch, SKILL_EXPERT_RIPOSTE);

  // Elite mobiles receive the expert riposte skill.
  if(IS_ELITE(ch))
  {
    if(GET_CLASS(ch, CLASS_WARRIOR) ||
        GET_CLASS(ch, CLASS_ANTIPALADIN) ||
        GET_CLASS(ch, CLASS_DREADLORD) ||
        GET_CLASS(ch, CLASS_AVENGER))
    {
      skl += 100;
      npcepicriposte = TRUE;
    }
  }

  /*  Blademasters are slightly better, though they don't get
   *  the full riposte skill
   */
  if(GET_SPEC(ch, CLASS_RANGER, SPEC_BLADEMASTER))
  {
    skl *= 1.05;
  }

  /*  Hey, lets do the same for Swashbucklers */
  if(GET_SPEC(ch, CLASS_WARRIOR, SPEC_SWASHBUCKLER))
  {
    skl *= 1.10;
  }

  /* lucky or unlucky? */
  if( number(0, GET_C_LUK(ch)) > number(0, GET_C_LUK(victim)) )
  {
    skl *= 1.05;
  }
  else
  {
    skl *= 0.95;
  }

  // Harder to riposte while stunned.
  if(IS_STUNNED(ch))
  {
    skl *= 0.50;
  }

  // Easier to riposte versus stunned attacker.
  if(IS_STUNNED(victim))
  {
    skl *= 1.15;
  }
  // Much harder to riposte while knocked down.
  if(!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    skl *= 0.50;
  }

  // Much harder to riposte something you are not fighting.
  if(GET_OPPONENT(ch) != victim)
  {
    skl *= 0.50;
  }

  if(IS_AFFECTED5(ch, AFF5_DAZZLEE))
  {
    skl *= 0.95;
  }

  // Expert riposte.
  if( expertriposte )
  {
    skl += expertriposte;
  }

  // Simple comparison.
  if(randomnumber > skl)
  {
    return FALSE;
  }

  if(ch->equipment[FOURTH_WEAPON] && !number(0, 4))
    wpn = ch->equipment[FOURTH_WEAPON];
  else if(ch->equipment[THIRD_WEAPON] && !number(0, 3))
    wpn = ch->equipment[THIRD_WEAPON];
  else if(ch->equipment[SECONDARY_WEAPON] && !number(0, 2))
    wpn = ch->equipment[SECONDARY_WEAPON];
  else
    wpn = ch->equipment[PRIMARY_WEAPON];

  if( expertriposte > number(1, 500) && GET_OPPONENT(ch) == victim )
  {
    act("$n &+wslams aside&n $N's attack and then pounds $M!", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n &+wslams aside&n your attack and counters!", TRUE, ch, 0, victim, TO_VICT);
    act("You &+wslam aside&n $N's attack and counter!", TRUE, ch, 0, victim, TO_CHAR);

    hit(ch, victim, wpn);

    if( expertriposte > number(1, 500) && IS_ALIVE(ch) && IS_ALIVE(victim) )
    {
      hit(ch, victim, wpn);
    }
  }
  else if( (npcepicriposte == TRUE) && !number(0, 4) && GET_OPPONENT(ch) == victim )
  {
    act("$n &+wslams aside&n $N's attack and then pounds $M!", TRUE, ch, 0, victim,
        TO_NOTVICT);
    act("$n &+wslams aside&n your attack and counters!", TRUE, ch, 0, victim,
        TO_VICT);
    act("You &+wslam aside&n $N's attack and counter!", TRUE, ch, 0, victim,
        TO_CHAR);

    hit(ch, victim, wpn);

    if( !number(0, 1) && IS_ALIVE(ch) && IS_ALIVE(victim) )
    {
      hit(ch, victim, wpn);
    }
  }
  else
  {
    act("$n deflects $N's blow and strikes back at $M!", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n deflects your blow and strikes back at YOU!", TRUE, ch, 0, victim, TO_VICT);
    act("You deflect $N's blow and strike back at $M!", TRUE, ch, 0, victim, TO_CHAR);
  }

  hit(ch, victim, wpn);


#ifndef NEW_COMBAT

  if( char_in_list(ch) && char_in_list(victim) && GET_CLASS(ch, CLASS_BERSERKER) )
  {
    if( affected_by_spell(ch, SKILL_BERSERK) )
    {
      hit(ch, victim, wpn);
    }
  }                             // new zerker stuff


#else

  hit(ch, victim, ch->equipment[WIELD], getBodyTarget(ch), TRUE, FALSE);

  if (GET_CLASS(ch, CLASS_BERSERKER))
  {
    if (affected_by_spell(ch, SKILL_BERSERK))
    {
      hit(ch, victim, ch->equipment[WIELD], getBodyTarget(ch), TRUE, FALSE);
    }
  }                             // same as above
#endif

  if( char_in_list(ch) && char_in_list(victim) && (skl = GET_CHAR_SKILL(ch, SKILL_FOLLOWUP_RIPOSTE)) > 0 )
  {
    notch_skill(ch, SKILL_FOLLOWUP_RIPOSTE, get_property("skill.notch.defensive", 17));

    if(number(0, 1) == 0)
    {
      if(skl / 3 > number(0, 100))
      {
        act("Before $N can recover $n steps forward and slams $s fist into $S face.", TRUE, ch, 0, victim, TO_NOTVICT);
        act("Before $E can recover you step forward and slam your fist into $S face.", TRUE, ch, 0, victim, TO_CHAR);
        act("Before you can recover $n steps forward and slams $s fist into your face!", TRUE, ch, 0, victim, TO_VICT);
        victim_dead = damage(ch, victim, GET_DAMROLL(ch) * ch->specials.damage_mod, SKILL_FOLLOWUP_RIPOSTE);

        if( !victim_dead && skl / 3 > number(0, 100) )
        {
          act("As $N staggers back $n follows-up with a well-placed kick.", TRUE, ch, 0, victim, TO_NOTVICT);
          act("As $E staggers back you follow-up with a well-placed kick.", TRUE, ch, 0, victim, TO_CHAR);
          act("As you stagger from the blow $n follows-up with a well-placed kick.", TRUE, ch, 0, victim, TO_VICT);
          melee_damage(ch, victim, dice(20, 6), 0, 0);
        }
      }
    }
    else
    {
      if( skl / 3 > number(0, 100) )
      {
        act("Spinning around $n slams his elbow into $N's throat.", TRUE, ch, 0, victim, TO_NOTVICT);
        act("Before you can react $n spins around and slams $s elbow into your throat.", TRUE, ch, 0, victim, TO_VICT);
        act("Spinning around you slam your elbow into $N's throat.", TRUE, ch, 0, victim, TO_CHAR);
        victim_dead = damage(ch, victim, GET_DAMROLL(ch) * ch->specials.damage_mod, SKILL_FOLLOWUP_RIPOSTE);

        if( !victim_dead && skl / 3 > number(0, 100) )
        {
          act("...then steps forward and brutally slams $s head into $N's face.", TRUE, ch, 0, victim, TO_NOTVICT);
          act("As $E gasps for breath you step forward and slam your head into $S face. ", TRUE, ch, 0, victim, TO_CHAR);
          act("As you stagger and gasp for breath $e steps forward and slams $s head into your face. Yikes!", TRUE, ch, 0, victim, TO_VICT);
          damage(ch, victim, dice(20, 6), SKILL_FOLLOWUP_RIPOSTE);
        }

      }
    }
  }
  return TRUE;
}

/*
 * This routine is here to solve some message timing problems, called from
 * several places in damage(), checks to see if victim should start
 * fighting ch.  JAB
 * Returns one of DAM_NONEDEAD, DAM_VICTDEAD, DAM_CHARDEAD, DAM_BOTHDEAD.
 */
int attack_back(P_char ch, P_char victim, int physical)
{
  if( !IS_ALIVE(ch) )
  {
    if( !IS_ALIVE(victim) )
    {
      return DAM_BOTHDEAD;
    }
    return DAM_CHARDEAD;
  }
  if( victim )
  {
    update_pos(victim);
  }
  if( !IS_ALIVE(victim) )
  {
    return DAM_VICTDEAD;
  }
  if( IS_FIGHTING(victim) || IS_DESTROYING(victim) )
  {
    return DAM_NONEDEAD;
  }

  if( ch->in_room != victim->in_room || ch->specials.z_cord != victim->specials.z_cord )
  {
    if (IS_NPC(victim))
    {
      MobRetaliateRange(victim, ch);
    }
  }
  // Can't very well attack back if either ch or victim is back ranked!
  else if( !physical && (IS_PC(ch) || IS_PC_PET(ch)) && IS_PC(victim)
    && (!IS_SET(ch->specials.act, PLR_VICIOUS) || IS_BACKRANKED(ch) || IS_BACKRANKED(victim)) )
  {
    return DAM_NONEDEAD;
  }
  else if( !IS_IMMOBILE(ch) )
  {
    set_fighting(victim, ch);
  }

  if( !IS_ALIVE(ch) )
  {
    if( !IS_ALIVE(victim) )
    {
      return DAM_BOTHDEAD;
    }
    return DAM_CHARDEAD;
  }
  if( !IS_ALIVE(victim) )
  {
    return DAM_VICTDEAD;
  }
  return DAM_NONEDEAD;
}


/* new function to check max attacker */
bool can_hit_target(P_char ch, P_char vict)
{
  int      table_1[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };
  int      table_2[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };

  P_char   opponent, next_ch;
  int      size_total = 0;
  int      size_max = 0;
  int      num_opponents = 0;

  if (IS_NPC(ch) || IS_NPC(vict))
    return TRUE;

  size_max = table_2[GET_ALT_SIZE(vict)];

  for (opponent = combat_list; opponent; opponent = next_ch)
  {
    next_ch = opponent->specials.next_fighting;
    if ((GET_OPPONENT(opponent) == vict) && (opponent != ch) && IS_PC(ch)
        && IS_PC(opponent))
    {
      num_opponents++;
      size_total += table_1[GET_ALT_SIZE(opponent)];
    }
  }

  if (num_opponents > 3)
    return FALSE;

  if (size_total > size_max)
    return FALSE;

  return TRUE;
}


/*
 * This function is now merely a wrapper translating old-style arguments to the new
 * damage API. It is provided for backward compatibility and should not be
 * used in the new code, call directly spell_damage and melee_damage instead.
 */
bool damage(P_char ch, P_char victim, double dam, int attacktype)
{
  struct damage_messages tmsg;
  int      spelltype, type, i, flags, circle;

  memset(&tmsg, 0, sizeof(struct damage_messages));

  if (IS_SPELL_S(attacktype))
  {
    if (attacktype == SPELL_HOLY_WORD || attacktype == SPELL_UNHOLY_WORD || 
        attacktype == SPELL_VOICE_OF_CREATION)
      type = SPLDAM_HOLY;
    else
      type = SPLDAM_GENERIC;

    if (IS_AGG_SPELL(attacktype))
      flags = 0;
    else
      flags = SPLDAM_NODEFLECT;

    circle = GetLowestSpellCircle_p(attacktype);
    if (circle < 4)
      flags |= SPLDAM_MINORGLOBE;
    if (circle < 5)
      flags |= SPLDAM_SPIRITWARD;
    if (circle < 6)
      flags |= SPLDAM_GRSPIRIT;
    if ((circle < 7) && (attacktype != SPELL_DETONATE))
      flags |= SPLDAM_GLOBE;
    if (attacktype == SPELL_NEG_ENERGY_BARRIER)
      flags |= SPLDAM_GRSPIRIT;

    if( (attacktype >= SPELL_FIRE_BREATH && attacktype <= SPELL_LIGHTNING_BREATH)
      || attacktype == SPELL_SHADOW_BREATH_1 || attacktype == SPELL_SHADOW_BREATH_2 )
      flags |= SPLDAM_BREATH;

    return spell_damage(ch, victim, dam, type, flags, &tmsg);
  }
  else if (raw_damage(ch, victim, dam, RAWDAM_DEFAULT, &tmsg) == DAM_VICTDEAD)
  {
    return TRUE;
  }
  else
  {
    if( !IS_FIGHTING(ch) && !IS_DESTROYING(ch) && (ch->in_room == victim->in_room) )
    {
      set_fighting(ch, victim);
      attack_back(ch, victim, attacktype > FIRST_SKILL);
    }
    return FALSE;
  }
}

/**
 * this function performs checks for globe blocking, shrug, deflects, defensive nuke procs etc.
 * also recalculates the damage depending on type and some protecting spells like prot-cold, soulshield
 * explanation for some arguments:
 *
 * flags - a value describing damage type, used to check protections, vulnerabilities
 *        one of SPLDAM_FIRE, SPLDAM_COLD, SPLDAM_GAS, SPLDAM_ACID, SPLDAM_GENERIC,
 *        SPLDAM_LIGHTING, SPLDAM_NEGATIVE, SPLDAM_HOLY
 *        maybe be combined with one of the flags: SPLDAM_ISBREATH, SPLDAM_NOSHRUG
 *        for example SPLDAM_GAS | SPLDAM_ISBREATH for gaseous breath
 *        SPLDAM_NOSHRUG decides whether damage can be deflected, shrugged, absorbed with proc
 *        in most cases not set, exceptions are damage from soulhield, fireshield, throw lightning
 *        some item procs
 * magic_circle - spell's circle, used in globe checks
 *
 * TO DO:
 *   consider implementing globe checks as extra flags like SPLDAM_MINORGLOBEBLOCKS,
 *   SPLDAM_SPIRITWARDBLOCKS etc
 *   replace SPLDAM_ prefix with something else, it may collide with constants
 *   for spells. SPLDAM, PHSDAM
 */
int spell_damage(P_char ch, P_char victim, double dam, int type, uint flags, struct damage_messages *messages)
{
  struct damage_messages dummy_messages;
  struct affected_type *af;
  struct proc_data data;
  P_char vict_group_member, next, eth_ch;
  P_obj vict_weapon, item;
  int  result, circle, awe, i;
  double levelmod = 1.0;

  // Just making sure.
  if(!ch || !victim)
    return DAM_NONEDEAD;

  if(messages == NULL)
  {
    memset(&dummy_messages, 0, sizeof(struct damage_messages));
    messages = &dummy_messages;
  }

  /* Pets take double damage from PC spells */
  if( get_linked_char(victim, LNK_PET) && IS_PC(ch) )
  {
    dam = (int) (dam * get_property("damage.pcs.vs.pets", 2.000));
  }

  // This is for Lich spell damage vamping.
  flags |= SPLDAM_SPELL;

  /* Being berserked incurs more damage from spells. Ouch. */
  if(affected_by_spell(victim, SKILL_BERSERK))
  {
    { // 110%
      dam *= dam_factor[DF_BERSERKSPELL];
    }

    if(GET_CLASS(ch, CLASS_BERSERKER))
    { // This is an additional penalty for berserkers since they are a hitter
      // class but tend to have in excess of 1.5k hitpoints.
      // + 10%
      dam *= dam_factor[DF_BERSERKEREXTRA];
    }

    if(affected_by_spell(victim, SKILL_RAGE)) 
    { // Being in means taking more spell damage.
      // + 35% ... they are flurried and doing insane damage!
      dam *= dam_factor[DF_BERSERKRAGE];
    }
  }

  // Lom: as I moved elementalist dam update here, so vamping(fire elemental from fire spell)
  //      will be correctly increased as he does more dmg
  if(ELEMENTAL_DAM(type) && has_innate(ch, INNATE_ELEMENTAL_POWER) && GET_LEVEL(ch) >= 35)
  {
    dam *= dam_factor[DF_ELEMENTALIST];
  }

  if( ELEMENTAL_DAM(type) && affected_by_spell(victim, SPELL_ENERGY_CONTAINMENT) )
    dam *= dam_factor[DF_ENERGY_CONTAINMENT];

  // Lom: honestly I would like put globe/sphere/deflect/etc checks first,
  //      so could not heal globed fire elemental with fireball or burning hands

  // vamping from spells might happen
  if( type == SPLDAM_FIRE && ch != victim && ((IS_AFFECTED2(victim, AFF2_FIRE_AURA)
    && IS_AFFECTED2(victim, AFF2_FIRESHIELD)) || GET_RACE(victim) == RACE_F_ELEMENTAL) )
  {
    act("$N&+R absorbs your spell!", FALSE, ch, 0, victim, TO_CHAR);
    act("&+RYou absorb&n $n's&+R spell!", FALSE, ch, 0, victim, TO_VICT);
    act("$N&+R absorbs&n $n's&+R spell!", FALSE, ch, 0, victim, TO_NOTVICT);
    vamp(victim,  dam / 4, GET_MAX_HIT(victim) * 1.1);

    // Solving issue of fire elementals not unmorting after vamping from fire spell
    update_pos(victim);
    if (IS_NPC(victim))
      do_alert(victim, NULL, 0);

    return DAM_NONEDEAD;
  }
  if( type == SPLDAM_COLD && ch != victim && ((IS_AFFECTED4(victim, AFF4_ICE_AURA)) &&  IS_AFFECTED3(victim, AFF3_COLDSHIELD)) )
  {
    act("$N&+C absorbs your spell!", FALSE, ch, 0, victim, TO_CHAR);
    act("&+CYou absorb&n $n's&+C spell!", FALSE, ch, 0, victim, TO_VICT);
    act("$N&+C absorbs&n $n's &+Cspell!", FALSE, ch, 0, victim, TO_NOTVICT);
    vamp(victim, dam / 4, GET_MAX_HIT(victim) * 1.1);

    update_pos(victim);
    if (IS_NPC(victim))
      do_alert(victim, NULL, 0);

    return DAM_NONEDEAD;
  }

  // end of vamping(is non agro now)
  // Lom: I think should set memory here, before messages
  // Lom: also might put globe check prior damage messages

  // victim remembers attacker
  remember(victim, ch);

  if( IS_PC_PET(ch) && GET_MASTER(ch)->in_room == ch->in_room && CAN_SEE(victim, GET_MASTER(ch)) )
  {
    remember(victim, GET_MASTER(ch));
  }

  if( has_innate(victim, INNATE_EVASION) && GET_SPEC(victim, CLASS_MONK, SPEC_WAYOFDRAGON) )
  {
    if ((((int) get_property("innate.evasion.removechance", 15.000))) > number(1,100))
    {
      send_to_char("You twist out of the way avoiding the harmful magic!\n", victim);
      act ("$n twists out of the way avoiding the harmful magic!", FALSE, victim, 0, ch, TO_ROOM);
      return DAM_NONEDEAD;
    }
  }
  // globes check
  if( ch != victim )
  {
    if( (IS_AFFECTED3(victim, AFF3_SPIRIT_WARD) && (flags & SPLDAM_SPIRITWARD))
      || (IS_AFFECTED3(victim, AFF3_GR_SPIRIT_WARD) && (flags & SPLDAM_GRSPIRIT)) )
    {
      act("&+WThe globe around your body flares as it bears the brunt of&n $n&+W's assault!",
          FALSE, ch, 0, victim, TO_VICT | ACT_NOTTERSE);
      act("&+WThe globe around&n $N&+W's body flares as it bears the brunt of your assault!",
          FALSE, ch, 0, victim, TO_CHAR | ACT_NOTTERSE);
      attack_back(ch, victim, FALSE);
      return DAM_NONEDEAD;
    }
    if(((IS_AFFECTED(victim, AFF_MINOR_GLOBE)) &&
          (flags & SPLDAM_MINORGLOBE)) ||
        (IS_AFFECTED2(victim, AFF2_GLOBE) &&
         (flags & SPLDAM_GLOBE)))
    {
      act("&+RThe globe around your body flares as it bears the brunt of&n $n&+R's assault!",
          FALSE, ch, 0, victim, TO_VICT | ACT_NOTTERSE);
      act("&+RThe globe around&n $N&+R's body flares as it bears the brunt of your assault!",
          FALSE, ch, 0, victim, TO_CHAR | ACT_NOTTERSE);
      attack_back(ch, victim, FALSE);
      return DAM_NONEDEAD;
    }
  }
  // end of globes check

  /* check for deflectable spells - basically all but shields damage and already deflected spells */
  if( (ch != victim) && !IS_SET(flags, SPLDAM_NODEFLECT) )
  {
    /* deflection */
    if( IS_AFFECTED4(victim, AFF4_DEFLECT) && IS_ALIVE(ch) )
    {
      act("&+cA &+Ctranslucent&n&+c field &+Wflashes&n&+c around your body upon contact with&n $n&n&+c's assault, deflecting it back at $m!",
          FALSE, ch, 0, victim, TO_VICT);
      act("&+cA &+Ctranslucent&n&+c field &+Wflashes&n&+c around&n $N&n&+c's body upon contact with&n $n&n&+c's assault, deflecting it back at $m!",
          FALSE, ch, 0, victim, TO_NOTVICT);
      act("&+cA &+Ctranslucent&n&+c field &+Wflashes&n&+c around&n $N&n&+c's body upon contact with your assault, deflecting it back at YOU!",
          FALSE, ch, 0, victim, TO_CHAR);

      affect_from_char(victim, SPELL_DEFLECT);
      result = spell_damage(victim, ch, dam * 0.7, type, flags | SPLDAM_NODEFLECT, messages);

      if (result == DAM_NONEDEAD)
      {
        attack_back(ch, victim, FALSE);
        return DAM_NONEDEAD;
      }
      else
      {
        return DAM_CHARDEAD;
      }
    }

    if(!IS_ALIVE(victim))
    {
      if(IS_ALIVE(ch))
        return DAM_VICTDEAD;
      else
        return DAM_BOTHDEAD;
    }
    else if(!IS_ALIVE(ch))
      return DAM_CHARDEAD;

    /* defensive spell hook for equipped items - Tharkun */
    for (i = 0; i < sizeof(proccing_slots) / sizeof(int); i++)
    {
      if( (item = victim->equipment[proccing_slots[i]]) == NULL )
        continue;

      if( IS_PC_PET(victim) && OBJ_VNUM(item) == 1251 )
        continue;

      if( obj_index[item->R_num].func.obj != NULL )
      {
        data.victim = ch;
        data.dam = (int) dam;
        data.attacktype = type;
        data.flags = flags;
        data.messages = messages;

        if ((*obj_index[item->R_num].func.obj) (item, victim, CMD_GOTNUKED, (char *) &data))
        {
          if (GET_STAT(victim) == STAT_DEAD)
            return DAM_VICTDEAD;
          else if (GET_STAT(ch) == STAT_DEAD)
            return DAM_CHARDEAD;
          else
            return DAM_NONEDEAD;
        }
      }
    }

    /* defensive spell hook for mob proc - Torgal */
    if( victim && IS_NPC(victim) && mob_index[GET_RNUM(victim)].func.mob
      && !affected_by_spell(victim, TAG_CONJURED_PET) )
    {
      data.victim = ch;
      data.dam = (int) dam;
      data.attacktype = type;
      data.flags = flags;
      data.messages = messages;

      if( (*mob_index[GET_RNUM(victim)].func.mob) (victim, ch, CMD_GOTNUKED, (char *) &data) )
      {
        if (GET_STAT(victim) == STAT_DEAD)
          return DAM_VICTDEAD;
        else if (GET_STAT(ch) == STAT_DEAD)
          return DAM_CHARDEAD;
        else
          return DAM_NONEDEAD;
      }
    }

    if (IS_AFFECTED4(victim, AFF4_STORNOGS_SPHERES))
    {
      act("&+RThe sphere circling you darts in front of&n $n&+R's assault, absorbing its magic and leaving you unharmed!",
          FALSE, ch, 0, victim, TO_VICT);
      act("&+RThe sphere circling&n $N&+R's body darts in front of&n $n&+R's assault!",
          FALSE, ch, 0, victim, TO_NOTVICT);
      act("&+RThe sphere circling&n $N&+R's body darts in front of your assault!",
          FALSE, ch, 0, victim, TO_CHAR);
      af = get_spell_from_char(victim, SPELL_STORNOGS_SPHERES);

      if (af && --af->modifier == 0)
      {
        affect_from_char(victim, SPELL_STORNOGS_SPHERES);
        REMOVE_BIT(victim->specials.affected_by4, AFF4_STORNOGS_SPHERES);
        send_to_char("The last sphere protecting you disappears.\r\n", victim);
      }
      attack_back(ch, victim, FALSE);
      return DAM_NONEDEAD;
    }

    if(GET_CHAR_SKILL(victim, SKILL_ARCANE_RIPOSTE) > 0 && !IS_TRUSTED(victim))
    {
      int skill_lvl = GET_CHAR_SKILL(victim, SKILL_ARCANE_RIPOSTE);

      if(!IS_STUNNED(victim) && (dam > 10
        && notch_skill(victim, SKILL_ARCANE_RIPOSTE, get_property("skill.notch.arcane", 10)))
        || (number(1, 100) < skill_lvl / 4))
      {
        act("$N frowns in &+cconcentration&n as $E intercepts $n's spell and &+Churls it back at $m!&n",
            TRUE, ch, 0, victim, TO_NOTVICT);
        act("$N frowns in &+cconcentration&n as $E intercepts your spell and &+Churls it back at you!&n",
            TRUE, ch, 0, victim, TO_CHAR);
        act("With &+cgreat mastery&n you intercept $n's spell and &+Churl it back at $m!&n",
            TRUE, ch, 0, victim, TO_VICT);

        result = spell_damage(victim, ch, (skill_lvl * dam) / 100, type, flags, messages);

        if (result == DAM_VICTDEAD)
        {
          return DAM_CHARDEAD;
        }
        else if (result == DAM_CHARDEAD)
        {
          return DAM_VICTDEAD;
        }
        else
          return result;
      }
    }

    if(GET_CHAR_SKILL(victim, SKILL_ARCANE_BLOCK) > 0 &&
        !IS_TRUSTED(victim) &&
        !IS_STUNNED(victim) &&
        !IS_IMMOBILE(victim))
    {
      if(dam > 15
        && (notch_skill(victim, SKILL_ARCANE_BLOCK, get_property("skill.notch.arcane", 10))
        || number(1, 250) <= (GET_LEVEL(victim) + GET_C_LUK(victim) / 10 + GET_CHAR_SKILL(victim, SKILL_ARCANE_BLOCK))
        || ((IS_ELITE(victim) || IS_GREATER_RACE(victim)) && !number(0, 4))))
      {
        act("$N raises hands performing an &+Marcane gesture&n and some of $n's &+mspell energy&n is dispersed.",
            TRUE, ch, 0, victim, TO_NOTVICT);
        act("$N raises hands performing an &+Marcane gesture&n and some of your &+mspell's energy&n is dispersed.",
            TRUE, ch, 0, victim, TO_CHAR);
        act("You perform an &+Marcane gesture&n dispersing some of $n's &+mspell energy.&n",
            TRUE, ch, 0, victim, TO_VICT);
        dam = dam - number(1, (get_property("skill.arcane.block.dam.reduction", .004) * GET_CHAR_SKILL(victim, SKILL_ARCANE_BLOCK) * dam));
      }
    }

    if(GET_CHAR_SKILL(victim, SKILL_DISPERSE_FLAMES) &&
        type == SPLDAM_FIRE &&
        GET_CHAR_SKILL(victim, SKILL_DISPERSE_FLAMES) > number(0, 100) &&
        !IS_TRUSTED(victim))
    {
      if( (dam > 10 && notch_skill(victim, SKILL_DISPERSE_FLAMES, get_property("skill.notch.pyrokinetics", 2))) ||
        (!number(0, 2) && number(1, 56) <= GET_LEVEL(victim)) )
      {
        act("$N &+rsmiles slightly as the &+Yflames &+rdwindle and die before reaching $M.&n",
            TRUE, ch, 0, victim, TO_NOTVICT);
        act("$N &+rsmiles slightly as the &+Yflames &+rdwindle and die before reaching $M.&n",
            TRUE, ch, 0, victim, TO_CHAR);
        act("&+rYou take control over the &+Yflames &+rand will them to dwindle and die.&n",
            TRUE, ch, 0, victim, TO_VICT);
        return DAM_NONEDEAD;
      }
    }

    if(GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) &&
        type == SPLDAM_FIRE &&
        GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) > number(0, 100) &&
        !IS_TRUSTED(victim))
    {
      if ((dam > 10 && notch_skill(victim, SKILL_FLAME_MASTERY, get_property("skill.notch.pyrokinetics", 2)))
        || (!number(0, 10) && number(1, 56) <= GET_LEVEL (ch)))
      {
        act("$N &+rsmiles slightly as $E stops the &+Yflames &+rsummoned by&n $n &+rand hurls them back at $m!&n",
            TRUE, ch, 0, victim, TO_NOTVICT);
        act("&+rTo your horror you see&n $N &+rsmile slightly as $E halts the &+Yflames &+rand hurls them back at you!&n",
            TRUE, ch, 0, victim, TO_CHAR);
        act("&+rYou laugh as you send &+Yflames &+rand burning &+Wectoplasm &+rback towards&n $n.&n",
            TRUE, ch, 0, victim, TO_VICT);
        result = spell_damage(victim, ch, GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) * dam / 80, type, flags, messages);
        if (result == DAM_VICTDEAD)
        {
          return DAM_CHARDEAD;
        }
        else if (result == DAM_CHARDEAD)
        {
          return DAM_VICTDEAD;
        }
        else
          return result;
      }
    }
  }                             /* end deflectable */

  /* Guess we'll put the new ether spells here */
  if (type == SPLDAM_FIRE && affected_by_spell(victim, SPELL_ICE_ARMOR))
  {
    act("&+C$N&+C's &+WI&+Cc&+wy &+BShield &+Rmelts &+Caround $M from $n's &+Rf&+ri&+Yer&+Ry &+rassault, &+Cbut leaves $M unharmed!",
        TRUE, ch, 0, victim, TO_NOTVICT);
    act("&+C$N&+C's &+WI&+Cc&+wy &+BShield &+Rmelts &+Caround $M from your &+Rf&+ri&+Yer&+Ry &+rassault, &+Cbut leaves $M unharmed!",
        TRUE, ch, 0, victim, TO_CHAR);
    act("&+CYour &+Wi&+Cc&+wy &+Bshield &+Rmelts &+Caround you from the &+Rf&+ri&+Yer&+Ry &+rassault, &+Cbut leaves you unharmed!&n",
        TRUE, ch, 0, victim, TO_VICT);
    affect_from_char(victim, SPELL_ICE_ARMOR);
    return DAM_NONEDEAD;
  }

  if (type == SPLDAM_HOLY && affected_by_spell(victim, SPELL_NEG_ARMOR))
  {
    act("$n's &+WH&+wo&+Ll&+Wy &+wspell &+Ldisperses the negative shield surrounding $N&+L, but leaves $M unharmed!&n",
        TRUE, ch, 0, victim, TO_NOTVICT);
    act("&+LThe &+WH&+wo&+Ll&+Wy &+wspell &+Ldisperses the negative shield surrounding $N&+L, but leaves $M unharmed!&n",
        TRUE, ch, 0, victim, TO_CHAR);
    act("&+LThe barrier of negative energy &=LWFLASHES&n &+Las the &+WH&+wo&+Ll&+Wy &+wspell &+Ldisperses your shield but leaves you unharmed!&n",
        TRUE, ch, 0, victim, TO_VICT);
    affect_from_char(victim, SPELL_NEG_ARMOR);
    return DAM_NONEDEAD;
  }
  /* End Ethermancer Absorb Spells */

  if( !IS_SET(flags, SPLDAM_NOSHRUG) )
  {
    if( resists_spell(ch, victim) )
    {
      return attack_back(ch, victim, FALSE);
    }

    // @50dam: 1/10 chance, @100dam: 1/5chance, @499dam: 5/6 chance, @500dam: 100% chance to pass 1st check.
    // Also, 1% chance per 2 levels: 28% at lvl 56 with 500+dam.
    // So, 2.8% chance @lvl 56 with 50-99 dam, 5.6% chance @lvl 56 with 100-149 dam (most common).
    if( (ch != victim) && (dam / 50) > number(0, 9) && has_innate(victim, INNATE_SPELL_ABSORB)
      && number(1, 100) <= (GET_LEVEL(victim) / 2) && USES_SPELL_SLOTS(victim) )
    {
      for( circle = get_max_circle(victim); circle >= 1; circle-- )
      {
        if( victim->specials.undead_spell_slots[circle] < max_spells_in_circle(victim, circle) )
        {
          act("&+L$N&+L absorbs the power of your spell!&n", FALSE, ch, 0, victim, TO_CHAR);
          act("&+LYou absorb the power of $n&+L's spell!&n", FALSE, ch, 0, victim, TO_VICT);
          act("&+L$N&+L absorbs the power of $n&+L's spells!&n", FALSE, ch, 0, victim, TO_NOTVICT);
          send_to_char_f( victim, "&+LYou regain a %d%s circle slot.&n\r\n", circle, ((circle == 3) ? "rd" :
            ( (circle == 2) ? "nd" : ((circle == 1) ? "st" : "th") )) );
          victim->specials.undead_spell_slots[circle]++;
          return attack_back(ch, victim, FALSE);
        }
      }
    }
  }
  /* Commenting this out for now - vision was never fully implemented - Drannak 1/8/13
  // Shrug now works as MR
  if( !(flags & SPLDAM_NOSHRUG) && has_innate(victim, INNATE_MAGIC_RESISTANCE) )
  {
  dam *= (100 - number(0, get_innate_resistance(victim) ) );
  dam /= 100;
  }
  */
  if(!(flags & SPLDAM_NODEFLECT) && IS_AFFECTED4(victim, AFF4_HELLFIRE) && !number(0, 5))
  {
    act("&+LYour spell is absorbed by&n $N's &+Rhellfire!",
        FALSE, ch, 0, victim, TO_CHAR);
    act("$n's&+L spell is absorbed by your &+Rhellfire!",
        FALSE, ch, 0, victim, TO_VICT);
    act("$n's&+L spell is absorbed by&n $N's &+Rhellfire!",
        FALSE, ch, 0, victim, TO_NOTVICT);
    vamp(victim, dam * get_property("vamping.hellfire.absorb", 0.14), (int)(GET_MAX_HIT(victim) * 1.3));
    return DAM_NONEDEAD;
  }

  // apply damage modifiers
  switch( type )
  {
    case SPLDAM_GENERIC:
      if (has_innate(victim, MAGICAL_REDUCTION))
        dam *= 0.80;
      break;
    case SPLDAM_FIRE:
      if ( IS_AFFECTED4(victim, AFF4_ICE_AURA) )
      {
        act("&+rYour fiery spell causes&n $N to &+rsmolder and spasm in pain!&n",
            TRUE, ch, 0, victim, TO_CHAR);
        act("$n's &+fiery spell causes you smolder and spasm in pain!&n",
            TRUE, ch, 0, victim, TO_VICT);
        act("$n's &+rfiery spell causes&n $N &n&+rto smolder and spasm in pain!&n",
            TRUE, ch, 0, victim, TO_NOTVICT);
        dam *= dam_factor[DF_VULNFIRE];
      }

      if(IS_NPC(ch) && !IS_PC_PET(ch))
      {
        dam = (int) (dam * get_property("damage.mob.bonus", 1.0));
        dam = MIN(dam, 800);
      }

      if(ch && affected_by_spell(ch, TAG_BLOODLUST) && !IS_PC_PET(victim) && IS_NPC(victim) && !CHAR_IN_JUSTICE_AREA(ch))
      {
        int dammod;
        struct affected_type *findaf, *next_af;  //initialize affects
        for(findaf = ch->affected; findaf; findaf = next_af)
        {
          next_af = findaf->next;
          if((findaf && findaf->type == TAG_BLOODLUST))
            dammod = findaf->modifier;
        }
        dam = (int) (dam * (1 + (dammod * .01)));
      }

      if(has_innate(victim, INNATE_VULN_FIRE))
      {
        dam *= dam_factor[DF_VULNFIRE];

        if(IS_AFFECTED2(victim, AFF2_FIRESHIELD))
          dam *= dam_factor[DF_ELSHIELDRED_TROLL];
        else if (IS_AFFECTED(victim, AFF_PROT_FIRE))
          dam *= dam_factor[DF_PROTECTION_TROLL];

        send_to_char("&+RThe flames cause your skin to &+Lsmoke!\r\n", victim);
        act("&+rThe intense &+Rflames &+rcause&n $n's skin to &+rsmolder and &+Yburn!&n",
            FALSE, victim, 0, 0, TO_VICT);
        act("&+rThe intense &+Rflames &+rcause&n $n's skin to &+rsmolder and &+Yburn!&n",
            FALSE, victim, 0, 0, TO_NOTVICT);

        if(!affected_by_spell(ch, TAG_TROLL_BURN))
        {
          struct affected_type af;

          bzero(&af, sizeof(af));
          af.type = TAG_TROLL_BURN;
          af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
          af.duration = WAIT_SEC * 30;
          affect_to_char(victim, &af);
        }
        else
        {
          struct affected_type *af1;
          for (af1 = victim->affected; af1; af1 = af1->next)
          {
            if(af1->type == TAG_TROLL_BURN)
            {
              af1->duration =  WAIT_SEC * 30;
            }
          }
        }
      }
      else if(IS_AFFECTED2(victim, AFF2_FIRESHIELD))
        dam *= dam_factor[DF_ELSHIELDRED];
      else if (IS_AFFECTED(victim, AFF_PROT_FIRE))
        dam *= dam_factor[DF_PROTECTION];

      if (IS_AFFECTED3(victim, AFF3_COLDSHIELD))
        dam *= dam_factor[DF_ELSHIELDINC];

      if( IS_AFFECTED(victim, AFF_BARKSKIN)
        || IS_AFFECTED5(victim, AFF5_THORNSKIN) )
        dam *= dam_factor[DF_BARKFIRE];

      if (affected_by_spell(victim, SPELL_IRONWOOD))
        dam *= dam_factor[DF_IRONWOOD];

      if (IS_AFFECTED5(victim, AFF5_WET))
      {
        dam *= dam_factor[DF_WETFIRE];
        if (ilogb(dam) > number(6, 10))
        {
          make_dry(victim);
          send_to_char("The heat of the spell dried up your clothes completely!\n", victim);
        }
      }

      break;
    case SPLDAM_COLD:
      if (GET_RACE(victim) == RACE_F_ELEMENTAL || IS_EFREET(victim) )
      {
        act("&+BYour icy spell makes&n $N &+Bwrithe in agony!&n",
            TRUE, ch, 0, victim, TO_CHAR);
        act("$n's &+Bicy spell causes you to writhe in agony!&n",
            TRUE, ch, 0, victim, TO_VICT);
        act("$n's &+Bicy spell causes&n $N to writhe in agony!&n",
            TRUE, ch, 0, victim, TO_NOTVICT);
        dam *= (IS_EFREET(victim) ? (.75 * dam_factor[DF_VULNCOLD]) : (dam_factor[DF_VULNCOLD]));
      }
      else if ( IS_AFFECTED2(victim, AFF2_FIRE_AURA) )
      {
        act("&+BYour icy spell makes&n $N &+Bwrithe in agony!&n",
            TRUE, ch, 0, victim, TO_CHAR);
        act("$n's &+Bicy spell causes you to writhe in agony!&n",
            TRUE, ch, 0, victim, TO_VICT);
        act("$n's &+Bicy spell causes&n $N to writhe in agony!&n",
            TRUE, ch, 0, victim, TO_NOTVICT);
        dam *= dam_factor[DF_VULNCOLD];
      }

      if (IS_AFFECTED3(victim, AFF3_COLDSHIELD))
        dam *= dam_factor[DF_ELSHIELDRED];
      else if (IS_AFFECTED2(victim, AFF2_PROT_COLD))
        dam *= dam_factor[DF_PROTECTION];

      if(GET_RACE(victim) == RACE_BARBARIAN)
        dam *= .6;

      if(IS_AFFECTED2(victim, AFF2_FIRESHIELD))
      {
        if(GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) &&
            GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) > number(1, 101) &&
            !IS_STUNNED(victim) &&
            CAN_SEE(victim, ch))
        {
          send_to_char("Your shift the &+Yflames&n around your body.\r\n", victim);
          break;
        }
        dam *= dam_factor[DF_ELSHIELDINC];
      }
      break;
    case SPLDAM_GAS:
      if (IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
        dam *= dam_factor[DF_ELSHIELDINC];
      else if (IS_AFFECTED2(victim, AFF2_PROT_GAS))
        dam *= dam_factor[DF_PROTECTION];
      break;
    case SPLDAM_ACID:
      if (IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
        dam *= dam_factor[DF_ELSHIELDINC];
      else if (IS_AFFECTED2(victim, AFF2_PROT_ACID))
        dam *= dam_factor[DF_PROTECTION];
      break;
    case SPLDAM_LIGHTNING:
      if (IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
        dam *= dam_factor[DF_ELSHIELDRED];
      else if (IS_AFFECTED2(victim, AFF2_PROT_LIGHTNING))
        dam *= dam_factor[DF_PROTECTION];
      break;
    case SPLDAM_HOLY:
      if(victim &&
          IS_UNDEADRACE(victim))
      {
        // PC and NPC vampire innate that starts at level 46. Nov08 -Lucrot
        if(has_innate(victim, INNATE_SACRILEGIOUS_POWER))
        {
          if(GET_LEVEL(victim) >= 46)
          {
            levelmod = 1.75;
          }
          if(GET_LEVEL(victim) >= 51)
          {
            levelmod = 1.5;
          }
          if(GET_LEVEL(victim) >= 56)
          {
            levelmod = 1.25;
          }
          dam = (int) (dam * levelmod);
        }  
        //dam *= get_property("damage.holy.increase.modifierVsUndead", 2.000);

        if(GET_LEVEL(victim) < 56) // Message is not displayed versus level 56 and greater vampires.
        {
          act("$N&+W wavers in agony, as the positive energies purge $s undead essence!&n",
              FALSE, ch, 0, victim, TO_CHAR);
          act("&+WNooo! The holy power of&n $n&+W is almost too much...&n", FALSE, ch, 0,
              victim, TO_VICT);
          act("$N&+W wavers in agony, as the positive energies sent by&n $n&+W purge $s essence!&n",
              FALSE, ch, 0, victim, TO_NOTVICT);
        }
      }

      if(IS_AFFECTED2(victim, AFF2_SOULSHIELD))
      {
        dam *= dam_factor[DF_SOULSPELL];
      }

      if(IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
      {
        dam *= dam_factor[DF_SLSHIELDINCREASE];
      }

      break;
    case SPLDAM_PSI:
      if (IS_AFFECTED3(victim, AFF3_TOWER_IRON_WILL))
        dam *= dam_factor[DF_IRONWILL];
      if (get_spell_from_char(victim, SKILL_TIGER_PALM))
        dam *= dam_factor[DF_TIGERPALM];
      if(GET_RACE(victim) == RACE_THRIKREEN)
        dam *= .7;
      break;
    case SPLDAM_NEGATIVE:
      if (victim && IS_ANGEL(victim))
        dam *= get_property("damage.neg.increase.modifierVsAngel", 1.500);
      if (IS_AFFECTED2(victim, AFF2_SOULSHIELD))
        dam *= dam_factor[DF_SLSHIELDINCREASE];
      if (IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
        dam *= dam_factor[DF_NEG_SHIELD_SPELL];
      break;
    default:
      break;
  }
  // end of apply damage modifiers
  /*
  //misfire damage reduction code for spells - drannak 8-12-2012 disabling to tweak
  if(affected_by_spell(ch, TAG_NOMISFIRE))
  {
  dam = dam;
  //send_to_char("damage output normal\r\n", ch);
  }
  else
  {
  dam = (dam * .5);
  //send_to_char("&+Rdamage output halved\r\n", ch);
  }
  */
  if (has_innate(victim, MAGIC_VULNERABILITY) && (GET_RACE(victim) == RACE_OGRE))
    dam *= 1.10;

  if(has_innate(victim, MAGIC_VULNERABILITY) && (GET_RACE(victim) == RACE_FIRBOLG))
    dam *= 1.10;

  if(affected_by_spell(ch, ACH_DRAGONSLAYER) && (GET_RACE(victim) == RACE_DRAGON))
    dam *=1.10;

  if(affected_by_spell(ch, ACH_DEMONSLAYER) && (GET_RACE(victim) == RACE_DEMON))
    dam *=1.10;

  if(affected_by_spell(victim, SPELL_SOULSHIELD) && GET_CLASS(victim, CLASS_PALADIN) || GET_CLASS(victim, CLASS_ANTIPALADIN))
    dam *= .85;

  //statupdate2013 - drannak
  double modifier = MAGICRES(ch);
  if( modifier >=1 && !(flags & SPLDAM_NOSHRUG) )
  {
    float redmod = 100;

    if (modifier <= 20)
      redmod -= (number(1, modifier));
    else if (modifier <= 40)
      redmod -= (number(20, modifier));
    else
      redmod -= (number(40, modifier));

    redmod *= .01;
    dam *= redmod;
  }

  //statupdate2013 - drannak - imp spell damage
//  int mdambonus = (MAGICDAMBONUS(ch) <= 100) ? 100 : (number(1, 10) + MAGICDAMBONUS(ch) - 10);
//  dam = (dam * mdambonus) / 100;
  // Real bonus is 1-10 % random up to MAGICDAMBONUS % damage.
  dam = dam * (MAGICDAMBONUS(ch) <= 100 ? 100 : MAGICDAMBONUS(ch) - 10 + number(1, 10) )/100;

  if((af = get_spell_from_char(victim, SPELL_ELEM_AFFINITY)) && ELEMENTAL_DAM(type))
  {
    char    *colors[5] = { "rfire", "Bcold", "Ylightning", "ggas", "Gacid" };
    char     buf[128];

    if (af->modifier == type)
    {
      dam *= dam_factor[DF_ELAFFINITY];
    }
    else
    {
      snprintf(buf, 128, "You feel less vulnerable to &+%s!&n\n", colors[type - 2]);
      send_to_char(buf, victim);
      af->modifier = type;
    }
  }

  // For the platemail of awe from Hall of the Ancients
#define HOA_AWE_VNUM 77746

  if (victim->equipment[WEAR_BODY] && 
      (victim->equipment[WEAR_BODY]->R_num == real_object(HOA_AWE_VNUM)))
  {
    awe = TRUE;
  }
  else
  {
    awe = FALSE;
  }

  if(ELEMENTAL_DAM(type) && IS_AFFECTED4(victim, AFF4_PHANTASMAL_FORM) && !awe)
  {
    char buf[128];
    dam *= dam_factor[DF_PHANTFORM];
  }

  if(has_innate(victim, INNATE_VULN_COLD) && type == SPLDAM_COLD)
  {
    dam *= dam_factor[DF_VULNCOLD];
    send_to_char("&+CThe freezing cold causes you intense pain!\n", victim);
    act("&+CThe freezing cold causes&n $n&+C intense pain!&n",
        FALSE, victim, 0, 0, TO_ROOM);


    /*  if(IS_PC(victim) &&
        !NewSaves(victim, SAVING_PARA, 2))
        {
        struct affected_type af;

        send_to_char("&+CThe intense cold causes your entire body to freeze!\n", victim);
        act("&+CThe intense cold causes&n $n&+C's entire body to freeze!&n",
        FALSE, victim, 0, 0, TO_ROOM);
        act("$n &+Mceases to move... frozen in place, still and lifeless.&n",
        FALSE, victim, 0, 0, TO_ROOM);
        send_to_char
        ("&+LYour body becomes like stone as &+Cparalysis &+Lsets in.&n\n",
        victim);

        bzero(&af, sizeof(af));
        af.type = SPELL_MAJOR_PARALYSIS;
        af.flags = AFFTYPE_SHORT;
        af.duration = WAIT_SEC * 3;
        af.bitvector2 = AFF2_MAJOR_PARALYSIS;

        affect_to_char(victim, &af);

        if(IS_FIGHTING(victim))
        stop_fighting(victim);
        if(IS_DESTROYING(victim))
        stop_destroying(victim);
        }
        }*/

    if(!NewSaves(victim, SAVING_PARA, 2))
    {
      struct affected_type af;

      send_to_char("&+CThe intense cold causes your entire body slooooow doooowwwnn!\n", victim);
      act("&+CThe intense cold causes&n $n&+C's entire body to slooooow doooowwwnn!&n",
          FALSE, victim, 0, 0, TO_ROOM);

      bzero(&af, sizeof(af));
      af.type = SPELL_SLOW;
      af.flags = AFFTYPE_SHORT;
      af.duration = 60;
      af.bitvector2 = AFF2_SLOW;

      affect_to_char(victim, &af);

    }
  }

  if (parse_chaos_shield(ch, victim))
  {
    dam *= dam_factor[DF_CHAOSSHIELD];
  }

  if(affected_by_spell(victim, SKILL_SPELL_PENETRATION))
  {
    int damageReductionMod = number(20, 70);
    dam *= (double) damageReductionMod / 100.0;
    // Use this properties file to make fine adjustments to this from now on.
    dam *= (double) get_property("skill.spellPenetration.damageReductionMod", 1.00);
    affect_from_char(victim, SKILL_SPELL_PENETRATION);
  }

  // racial spell damage modifiers
  if( GET_RACE(ch) > RACE_NONE && GET_RACE(ch) <= LAST_RACE && type >= 0 && type < LAST_SPLDAM_TYPE )
  {
    dam *= racial_spldam_offensive_factor[GET_RACE(ch)][type];
  }

  if( GET_RACE(victim) > RACE_NONE && GET_RACE(victim) <= LAST_RACE && type >= 0 && type < LAST_SPLDAM_TYPE )
  {
    dam *= racial_spldam_defensive_factor[GET_RACE(victim)][type];
  }

  // adjust damage based on zone difficulty
  if( IS_NPC(ch) && !affected_by_spell(ch, TAG_CONJURED_PET) )
  {
    int zone_difficulty = BOUNDED(1, zone_table[world[real_room0(GET_BIRTHPLACE(ch))].zone].difficulty, 10);

    if( zone_difficulty > 1 )
    {
      float dam_mod = 1.0 + ((float) get_property("damage.zoneDifficulty.spells.factor", 0.200) * zone_difficulty);
      dam = (float) ( dam * dam_mod );
    }
  }

int necropets[] = {3, 4, 5, 6, 7, 8, 9, 10, 78, 79, 80, 81, 82, 83, 84, 85};
if (IS_NPC(ch) && IS_PC(victim))
{
  for (int r = 0; r < 16; r++)
  {
    if (mob_index[GET_RNUM(ch)].virtual_number == NECROPET ||
        mob_index[GET_RNUM(ch)].virtual_number == necropets[r])
    {
      dam = dam/2;
      break;
    }
  }
}

if ((IS_NPC(ch) && IS_PC(victim)) && affected_by_spell(ch, TAG_CONJURED_PET))
  dam = dam/2;

  // across the board spell damage multiplier
  dam = (int) ( dam * get_property("damage.spell.multiplier", 1.0) );

if (get_linked_char(victim, LNK_ETHEREAL) || get_linking_char(victim, LNK_ETHEREAL))
{
  if (get_linked_char(victim, LNK_ETHEREAL))
  {
    eth_ch = get_linked_char(victim, LNK_ETHEREAL);
  }
  else if (get_linking_char(victim, LNK_ETHEREAL))
  {
    eth_ch = get_linking_char(victim, LNK_ETHEREAL);
  }

  if( IS_ALIVE(eth_ch) )
  {
    dam = (int) dam * 0.5;

    send_to_char("&+cThe damage passes through the &+Wether&+c being absorbed by your alliance!\n", victim);
    send_to_char("&+cYou feel weak as your &+Wethereal alliance&+c fills you with pain!\n", eth_ch);

    raw_damage(ch, eth_ch, dam, RAWDAM_DEFAULT ^ flags, messages);
  }
}

  // Aura of spell protection reduces damage by mod percent.
  if( IS_AFFECTED3( victim, AFF3_PALADIN_AURA ) && has_aura(victim, AURA_SPELL_PROTECTION ) )
  {
    dam *= (100 - aura_mod(victim, AURA_SPELL_PROTECTION));
    dam /= 100;
  }

  dam = MAX(1, dam);

  // ugly hack - we smuggle damage_type for eq poofing messages on 8 highest bits
  messages->type |= type << 24;
  result = raw_damage(ch, victim, dam, RAWDAM_DEFAULT ^ flags, messages);

  // Tether code here
/*
  if( GET_CLASS( ch, CLASS_CABALIST ) )
  {
    tetherheal( ch, dam );
  }
*/

  if(type == SPLDAM_ACID && !number(0, 3))
  {
    DamageStuff(victim, SPLDAM_ACID);
  }

  if(result == DAM_NONEDEAD)
  {
    if(ilogb(dam) > number(3, 60))
    {
      DamageStuff(victim, type);
    }

    attack_back(ch, victim, FALSE);

    if(IS_AFFECTED5(victim, AFF5_WET) && type == SPLDAM_LIGHTNING && ilogb(dam) > number(0, 10))
    {
      struct affected_type shock_af;

      act("The &+Celectricity&n surges through $n's wet clothes immobilizing $m momentarily!",
        FALSE, victim, 0, 0, TO_ROOM);
      act("The &+Celectricity&n surges through your wet clothes paralyzing you in a shock!",
        FALSE, victim, 0, 0, TO_CHAR);
      memset(&shock_af, 0, sizeof(shock_af));
      shock_af.type = SPELL_LIGHTNING_BOLT;
      shock_af.flags = AFFTYPE_SHORT;
      shock_af.duration = (int) (get_property("shock.duration", 2.000 * WAIT_SEC));
      affect_to_char(victim, &shock_af);
    }
  }
  return result;
}

// End spell_damage()

int check_shields(P_char ch, P_char victim, int dam, int flags)
{
  int result = DAM_NONEDEAD;
  double soulshielddam = get_property("damage.shield.soulshield", 0.400);
  double negshielddam = get_property("damage.shield.negativeshield", 0.450);
  double fshield = get_property("damage.shield.fireshield", 0.750);
  double cshield = get_property("damage.shield.coldshield", 0.750);
  double lshield = get_property("damage.shield.lightningshield", 0.750);
  double ifshield = get_property("damage.shield.infernalfuryshield", 0.90);

  uint  sflags =
    SPLDAM_GLOBE | SPLDAM_NODEFLECT | RAWDAM_TRANCEVAMP;

  char buf[256];

  struct damage_messages lightningshield = {
    "$N &+Ygets zapped as $E hits you!&n",
    "&+YYou get a huge shock as you hit&n $n.",
    "$N &+Yis shocked as $E hits&n $n.",
    "$N &+Yis shocked to death as $E hits you!&n",
    "&+YAs you hit&n $n, &+Ya gigantic shock kills you!&n",
    "$N &+Yis shocked to death after hitting&n $n."
  };
  struct damage_messages coldshield = {
    "$N &+Bshivers from the unnatural cold as $E hits you!&n",
    "&+BYou shiver from the unnatural cold as you hit&n $n.",
    "$N &+Bshivers as $E hits&n $n.",
    "$N &+Bis frozen to death as $E hits you!&n",
    "&+BBRR! As you hit&n $n&+B, deadly cold fills your bones..&n",
    "$N &+Bis frozen to the bones after hitting&n $n."
  };
  struct damage_messages fireshield = {
    "$N &+ris burned severely as $E hits you!",
    "&+rYou are burned as you hit&n $n!",
    "$N &+ris burned severely as $E hits $n!",
    "&+rOnly a charred corpse remains after&n $N &+rtouches the &+Rflaming aura&N around you.",
    "&+rPHEW! Wasn't that a hot experience - the &+Raura&+r around&n $n &+r_fries_ you!",
    "&+rAs &n$N &+rtouches the &+Rfiery aura&N around $n&+r's body, $E is burned fatally."
  };
  struct damage_messages negshield = {
    "$N&+r ignites into &+Lblack flames&n&+r as $E hits you!",
    "&+rYou ignite into &+Lblack flames&n&+r as you hit&n $n!",
    "$N&+r ignites into &+Lblack flames&n&+r as $E hits&n $n!",
    "$N&+L is dissolved completely upon contact with your barrier!",
    "&+LYou are dissolved completely upon contact with&n $n's&+L barrier!",
    "$N&+L is dissolved completely upon contact with&n $n's&+L barrier!"
  };
  struct damage_messages soulshield = {
    "$N &+wsuffers from contact with the aura about you.",
    "&+wYour spirit suffers from contact with&n $n's &+waura!",
    "$N &+wsuffers from contact with the aura about&n $n.",
    "&+wThe aura of power around your body dissolves&n $N &+wcompletely!",
    "&+wYour body is dissolved by the aura around&n $n!",
    "$N &+wis dissolved completely by the aura around&n $n!"
  };
  struct damage_messages soulshield_spec = {
    "$N suffers from contact with the &+Wholy&n aura about you.",
    "Your spirit suffers from contact with $n's &+Wholy&n aura!",
    "$N suffers from contact with the &+Wholy&n aura about $n.",
    "The &+WHOLY&n aura of power around your body dissolves $N completely!",
    "Your body is dissolved by the &+WHOLY&n aura around $n!",
    "$N is dissolved completely by the &+WHOLY&n aura around $n!"
  };
  struct damage_messages acid_blood = {
    "$N &+Lwrithes in agony the black blood greedily eats into $S &+rflesh.&n",
    "&+LThe world &+rexp&+Rlo&+rdes in p&+Ra&+rin &+Las the black blood greedily eats into your &+rflesh.",
    "$N &+Lwrithes in agony the black blood greedily eats into $S &+rflesh.&n",
    "&+LWhat was once&n $N, &+Lbut now only a mass of burnt flesh, crumbles in a heap on the ground.",
    "&+LA pain beyond imagination overwhelms you as the black blood eats its way into your heart.",
    "&+LWhat was once&n $N, &+Lbut now only a mass of burnt flesh, crumbles in a heap on the ground."
  };
  struct damage_messages thornskin = {
    "As $N strikes you, you smile as $E tears $S &+Rfl&+res&+Rh&n!",
    "As you strike $n, you are &+rscratched&n by $s &+ythorny skin&n!",
    "As $N strikes $n, $e smiles as $N tears $S &+Rfl&+res&+Rh&n!",
    "$N is &+rscratched&n to death by your hide.",
    "You are &+rscratched&n to death by $n's hide.",
    "$N is &+rscratched&n to death by $n's hide.",
  };
  struct damage_messages infernalfury = {
	  "&+yThe &+Rinf&+rer&+Lnal e&+rner&+Rgies &+ysurrounding you &+Wsear &+yinto&n $N &+yas $E hits you!&n",
	  "&+yYou are engulfed by &+Rinf&+rer&+Lnal e&+rner&+Rgies &+yas you hit&n $n&+y!&n",
	  "$N &+yis is engulfed by &+Rinf&+rer&+Lnal e&+rner&+Rgies &+yas $E hits&n $n&+y!&n",
	  "&+yThe &+Rinf&+rer&+Lnal e&+rner&+Rgies &+Wsear &+ythe life out of&n $N &+yas $E touches you for the last time.&n",
	  "&+yThe &+Wpower &+yof the &+RNin&+Le He&+Rlls &+yconsumes you!&n",
	  "&+yAs &n$N &+Wtouches &+ythe &+Rinf&+rer&+Lnal e&+rner&+Rgies &+yaround&n $n, &+y$E falls lifeless to the ground."};

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return 0;
  }

  // if you ever change soulshield or neg shield damage make sure to check the
  // better one first here
  if(IS_AFFECTED4(victim, AFF4_NEG_SHIELD) && !IS_UNDEADRACE(ch))
  {
    result =
      spell_damage(victim, ch, (int) (dam * negshielddam),
          SPLDAM_NEGATIVE, sflags | SPLDAM_GRSPIRIT, &negshield);
  }
  else if(IS_AFFECTED2(victim, AFF2_SOULSHIELD) &&
      ((IS_EVIL(ch) && IS_GOOD(victim)) ||
       (IS_GOOD(ch) && IS_EVIL(victim)) ||
       opposite_racewar(ch, victim)))
  {
    if (GET_SPEC(victim, CLASS_CLERIC, SPEC_HOLYMAN))
    { 
      result =
        spell_damage(victim, ch, (int) (dam * soulshielddam),
            SPLDAM_HOLY, SPLDAM_NODEFLECT | RAWDAM_TRANCEVAMP | SPLDAM_GRSPIRIT,
            &soulshield_spec);

      // Little bonus for devotion. Jan08 -Lucrot
      int rnumber = 9;
      rnumber -= GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 50;

      // Elite and greater races get a similar bonus also. Jan08 -Lucrot
      if(IS_NPC(ch) && !affected_by_spell(ch, TAG_CONJURED_PET) &&
          GET_LEVEL(ch) >= 51 &&
          (IS_ELITE(ch) ||
           IS_GREATER_RACE(ch)))
      {
        rnumber = 6;
      }

      if(result == DAM_NONEDEAD &&
          !number(0, rnumber)) /*&&
                                 !IS_AFFECTED3(ch, AFF3_GR_SPIRIT_WARD))*/
      {
        struct affected_type af;

        memset(&af, 0, sizeof(af));
        af.flags = AFFTYPE_SHORT;
        switch (number(1, 9))
        {
          case 1:
            if(!IS_ELITE(ch) &&
                !IS_GREATER_RACE(ch) &&
                (GET_LEVEL(ch) + 5) > GET_LEVEL(victim))
            {
              af.type = SPELL_MINOR_PARALYSIS;
              af.bitvector2 = AFF2_MINOR_PARALYSIS;
              af.duration = PULSE_VIOLENCE;
              affect_to_char(ch, &af);
              act("&+wYour entire body freezes upon contact with the &+Wholy aura&n &+wsurrounding&n $N!",
                  FALSE, ch, 0, victim, TO_CHAR);
              act("$n's &+wentire body freezes upon contact with the &+Wholy aura&n &+wsurrounding&n $N!",
                  FALSE, ch, 0, victim, TO_NOTVICT);
              act("$n's &+wentire body freezes upon contact with the &+Wholy aura&n &+wsurrounding you!&n",
                  FALSE, ch, 0, victim, TO_VICT);
              update_pos(ch);
              break;
            }
          case 2:
            if(!EYELESS(ch) &&
                !affected_by_spell(ch, SPELL_BLINDNESS))
            {
              snprintf(buf, MAX_STRING_LENGTH, "$N &+wmutters a prayer to %s, and then heavy &+Ldarkness &+wshrouds your vision.&n", get_god_name(victim));
              act(buf, FALSE, ch, 0, victim, TO_CHAR);
              snprintf(buf, MAX_STRING_LENGTH, "$n &+wgropes around as if blind as $N mutters a prayer to %s.", get_god_name(victim));
              act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
              snprintf(buf, MAX_STRING_LENGTH, "&+wYou send a prayer to %s, to shroud your foes in &+Ldarkness.&n", get_god_name(victim));
              act(buf, FALSE, ch, 0, victim, TO_VICT);
              blind(victim, ch, 2 * PULSE_VIOLENCE);
              break;
            }
          case 3:
            if(!affected_by_spell(ch, SPELL_CURSE))
            {
              af.type = SPELL_CURSE;
              af.location = APPLY_SAVING_SPELL;
              af.modifier = 10;
              af.duration = 7 * PULSE_VIOLENCE;
              affect_to_char(ch, &af);
              act("&+wYour strength dwindles as&n $N &+wgrabs you by your forehead and calls down the wraith of $S deity.&n",
                  FALSE, ch, 0, victim, TO_CHAR);
              act("$N &+wgrabs&n $n &+wby his forehead and calls down the might of $S deity.&n",
                  FALSE, ch, 0, victim, TO_NOTVICT);
              act("&+wYou touch&n $n's &+wforehead and call down the might of your deity.&n",
                  FALSE, ch, 0, victim, TO_VICT);
              break;
            }
          case 4:
            if(IS_CLERIC(ch) &&
                !IS_ELITE(ch) &&
                !IS_GREATER_RACE(ch))
            {
              snprintf(buf, MAX_STRING_LENGTH, "&+YBright light falls from above and&n $N &+Ysends forth divine power!&n");
              act(buf, FALSE, ch, 0, victim, TO_CHAR);
              snprintf(buf, MAX_STRING_LENGTH, "&+w%s&+w sends a ray of light down upon&n $N.", get_god_name(victim));
              act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
              snprintf(buf, MAX_STRING_LENGTH, "&+wYou send a prayer to %s&+w who instills you with &+Rwrath!&n", get_god_name(victim));
              act(buf, FALSE, ch, 0, victim, TO_VICT);
              spell_silence(GET_LEVEL(victim), victim, 0, 0, ch, 0);
              break;
            }
          case 5:
            act("$N &+wmutters a silent prayer to $S gods to aid $M in battle.&n",
                FALSE, ch, 0, victim, TO_CHAR);
            act("&+wBright light encases&n $N &+was $e mutters a prayer to $S deity.&n",
                FALSE, ch, 0, victim, TO_NOTVICT);
            act("&+wYour god rewards your faithfulness by sending aid from the Heavens!&n",
                FALSE, ch, 0, victim, TO_VICT);
            spell_flamestrike(GET_LEVEL(victim), victim, 0, 0, ch, 0);
            break;
          case 6:
            if((GET_VITALITY(victim) + 30) < GET_MAX_VITALITY(victim))
            {
              act("&+wBright light encases&n $N &+was $E mutters a prayer to $S god.&n",
                  FALSE, ch, 0, victim, TO_CHAR);
              act("&+wBright light encases&n $N &+was $E mutters a prayer to $S god.&n",
                  FALSE, ch, 0, victim, TO_NOTVICT);
              act("&+wYou send a prayer to the Heavens to invigorate your spirit.&n",
                  FALSE, ch, 0, victim, TO_VICT);
              spell_vigorize_critic(GET_LEVEL(victim), victim, 0, 0, victim, 0);
              break;
            }
          case 7:

            if(!IS_AFFECTED4(ch, AFF4_NOFEAR) &&
                !IS_ELITE(ch) &&
                !IS_GREATER_RACE(ch))
            {
              act("$N &+wresponds to your attack with a vicious look of pure hatred!&n",
                  FALSE, ch, 0, victim, TO_CHAR);
              act("$N &+wassumes a nasty look and tries to instill fear in $n!",
                  FALSE, ch, 0, victim, TO_NOTVICT);
              act("&+wYou muster up the nastiest look you can to scare off your foe.&n",
                  FALSE, ch, 0, victim, TO_VICT);
              spell_fear(GET_LEVEL(victim), victim, 0, 0, ch, 0);
              break;
            }
          case 8:
            if(GET_HIT(victim) + 50 < GET_MAX_HIT(victim))
            {
              snprintf(buf, MAX_STRING_LENGTH, "&+wBright light falls from above and&n $N&+w's wounds begin to heal!&n");
              act(buf, FALSE, ch, 0, victim, TO_CHAR);
              snprintf(buf, MAX_STRING_LENGTH, "&+w%s&+w sends a ray of light down upon&n $N&+w, healing some of $S wounds.", get_god_name(victim));
              act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
              snprintf(buf, MAX_STRING_LENGTH, "&+wYou send a prayer to %s&+w, who smiles upon you and mends some of your wounds.", get_god_name(victim));
              act(buf, FALSE, ch, 0, victim, TO_VICT);
              spell_heal(GET_LEVEL(victim), victim, 0, 0, victim, 0);
              break;
            }
          case 9:
            snprintf(buf, MAX_STRING_LENGTH, "$N &+wmutters a prayer to %s &+was $e stares coldly at you.&n", get_god_name(victim));
            act(buf, FALSE, ch, 0, victim, TO_CHAR);
            snprintf(buf, MAX_STRING_LENGTH, "&+w%s&+w sends a mighty wrath upon&n $n &+was &n$N&+w's prayers are heard.", get_god_name(victim));
            act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
            snprintf(buf, MAX_STRING_LENGTH, "&+wYou send a prayer to %s&+w, who responds with a bolt from the Heavens!", get_god_name(victim));
            act(buf, FALSE, ch, 0, victim, TO_VICT);
            spell_full_harm(GET_LEVEL(victim), victim, 0, 0, ch, 0);
            break;

          default:
            break;
        }
      }
    }
    else
    {
      dam = MAX(1, (int) (dam * soulshielddam));
      result = spell_damage(victim, ch, dam, SPLDAM_HOLY, sflags | SPLDAM_GRSPIRIT, &soulshield);
    }
  }
  if( result == DAM_NONEDEAD && IS_AFFECTED2(victim, AFF2_FIRESHIELD) )
  {
    result = spell_damage(victim, ch, (int) (dam * fshield), SPLDAM_FIRE, sflags, &fireshield);
  }
  else if(result == DAM_NONEDEAD && IS_AFFECTED3(victim, AFF3_COLDSHIELD))
  {
    result = spell_damage(victim, ch, (int) (dam * cshield), SPLDAM_COLD, sflags, &coldshield);
  }
  else if(result == DAM_NONEDEAD && IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
  {
    result = spell_damage(victim, ch, (int) (dam * lshield), SPLDAM_LIGHTNING, sflags, &lightningshield);
  }
  else if( result == DAM_NONEDEAD && IS_AFFECTED5(victim, AFF5_THORNSKIN) )
  {
	struct affected_type* afp = get_spell_from_char(victim, SPELL_THORNSKIN);
	double thornDamage = afp == NULL ? 2 : dice(2, MIN(2, afp->level / 10)); // 2 dam for bit, 2d(level/10, min 2) - 2d2 at 1, 2d5 at 50
	result = raw_damage(victim, ch, thornDamage, RAWDAM_DEFAULT | flags, &thornskin);
  }
  else if( result == DAM_NONEDEAD && IS_AFFECTED(victim, AFF_INFERNAL_FURY) )
  {
    result = spell_damage(victim, ch, (int) (dam * ifshield), SPLDAM_NEGATIVE, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &infernalfury);
  }


  if( (result == DAM_NONEDEAD) && has_innate(victim, INNATE_ACID_BLOOD) && (dam > ( (flags & PHSDAM_TOUCH) ? 9 : 5 ))
    && !number(0, 9) )
  {
    act("&+LBlack blood &+wspurts from your wound as&n $N's &+wweapon &+wrips your &+rflesh.&n",
      FALSE, victim, 0, ch, TO_CHAR);
    act("&+LBlack blood &+wspurts from&n $n &+was your weapon &+wrips $s &+rflesh.&n",
      FALSE, victim, 0, ch, TO_VICT);
    act("&+LBlack blood &+wspurts from $n &+was&n $N's &+Lweapon &+wrips $s &+rflesh.&n",
      FALSE, victim, 0, ch, TO_NOTVICT);
    if ((15 + GET_C_AGI(ch) / 6) > number(0, 100))
    {
      act
        ("$N &+wjumps out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
         FALSE, victim, 0, ch, TO_CHAR);
      act
        ("&+wYou jump out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
         FALSE, victim, 0, ch, TO_VICT);
      act
        ("$N &+wjumps out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
         FALSE, victim, 0, ch, TO_NOTVICT);
    }
    else
    {
      result =
        spell_damage(victim, ch, 40 + number(1, 40), SPLDAM_ACID,
            SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &acid_blood);
    }
  }
  return result;
}

/* melee_damage
 * This functions handles physical damage and modifies it against stoneskin type spells,
 * calculates vamping from melee damage and also resolves damage from fireshield type
 * spells to the attacker.
 */
int melee_damage(P_char ch, P_char victim, double dam, int flags, struct damage_messages *messages)
{
  struct damage_messages dummy_messages;
  unsigned int skin;
  int      vamp_dam, i, result, shld_result, ac;
  float    reduction;
  char     buffer[MAX_STRING_LENGTH];
  bool     dragonfist;

  // float    f_cur_hit, f_max_hit, f_skill = 0;  <-- ill use those for max_str later

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return 0;
  }

  if(messages == NULL)
  {
    memset(&dummy_messages, 0, sizeof(struct damage_messages));
    messages = &dummy_messages;
  }

  if(!IS_FIGHTING(ch) && !IS_DESTROYING(ch)
    && !(flags & PHSDAM_NOENGAGE) && ch->in_room == victim->in_room)
  {
    set_fighting(ch, victim);
  }

  dragonfist = FALSE;
  if (!(flags & PHSDAM_NOREDUCE))
  {
    /*dam -= BOUNDED(0,
      (int) ((dam *
      (100 -
      BOUNDED(-100, BOUNDED(-100, GET_AC(victim), 100),
      100))) / 800), (int) ((dam - 1)));
      */
    // If they have fist up, the have a 33% chance of hitting through ac.
    if( get_spell_from_char(ch, SKILL_FIST_OF_DRAGON) && !number(0,2) )
    {
      dragonfist = TRUE;
      // If the ac is > 0, apply it (increases damage), otherwise skip.
      if( (ac = calculate_ac( victim )) > 0 )
        dam += ( dam * (ac / 1000.00) );
    }
    else
    {
      dam += ( dam * calculate_ac(victim) / 1000.00 );
    }

    if(has_innate(victim, INNATE_TROLL_SKIN))
    {
      dam *= dam_factor[DF_TROLLSKIN];
    }
    /*
        if(affected_by_spell(ch, TAG_NOMISFIRE)) //new misfire code - drannak 8-12-2012
        {
        dam = dam;
    //send_to_char("damage output normal\r\n", ch);
    }
    else
    {
    dam = (dam * .5);
    //send_to_char("&+Rdamage output halved\r\n", ch);
    }
    */

    if(has_innate(victim, INNATE_GUARDIANS_BULWARK)
      && victim->equipment[WEAR_SHIELD])
    {
      dam *= dam_factor[DF_GUARDIANS_BULWARK];
    }

    if(affected_by_spell(victim, SKILL_BERSERK))
    {
      dam *= dam_factor[DF_BERSERKMELEE];
    }

    if(rapier_dirk_check(victim))
    {
      dam *= dam_factor[DF_SWASHBUCKLER_DEFENSE];
    }

    if(rapier_dirk_check(ch))
    {
      dam *= dam_factor[DF_SWASHBUCKLER_OFFENSE];
    }

    if(IS_RIDING(ch) && GET_SPEC(ch, CLASS_ANTIPALADIN, SPEC_DEMONIC)
      || IS_RIDING(ch) && GET_SPEC(ch, CLASS_PALADIN, SPEC_CAVALIER))
    {
      dam *= 1.20;
    }

    if( IS_AFFECTED2(victim, AFF2_SOULSHIELD)
      && (((IS_EVIL(ch) && IS_GOOD(victim))
      || (IS_GOOD(ch) && IS_EVIL(victim))
      || opposite_racewar(ch, victim))) )
    {
        dam *= dam_factor[DF_SOULMELEE];
    }

    if(IS_AFFECTED4(victim, AFF4_PROT_LIVING) && !IS_UNDEADRACE(ch))
    {
      dam *= dam_factor[DF_PROTLIVING];
    }
  }

  if(!(flags & PHSDAM_NOPOSITION))
  {
    if( MIN_POS(victim, POS_STANDING + STAT_NORMAL) )
    {
      if (get_linked_char(ch, LNK_FLANKING) == victim)
      {
        dam = (dam * get_property("damage.modifier.flank", 1.5));
      }
      if (get_linked_char(ch, LNK_CIRCLING) == victim)
      {
        dam = (dam * get_property("damage.modifier.circle", 2.0));
      }
    }
    // They get 1/2 the bonus damage if target not standing.
    else
    {
      if (get_linked_char(ch, LNK_FLANKING) == victim)
      {
        dam = dam * ( 1 + get_property("damage.modifier.flank", 1.5) ) / 2;
      }
      if (get_linked_char(ch, LNK_CIRCLING) == victim)
      {
        dam = dam * ( 1 + get_property("damage.modifier.circle", 2.0) ) / 2;
      }
    }

    if( MIN_POS(victim, POS_STANDING + STAT_NORMAL) )
    {
      // No bonus.
    }
    else if( MIN_POS(victim, POS_KNEELING + STAT_DEAD) && !GROUNDFIGHTING_CHECK(victim) )
    {
      dam *= dam_factor[DF_KNEELING];
    }
    else if( MIN_POS(victim, POS_SITTING + STAT_DEAD) && !GROUNDFIGHTING_CHECK(victim) )
    {
      dam *= dam_factor[DF_SITTING];
    }
    else if( MIN_POS(victim, POS_PRONE + STAT_DEAD) && !GROUNDFIGHTING_CHECK(victim) )
    {
      dam *= dam_factor[DF_PRONE];
    }
  }

  if( GET_CLASS(ch, CLASS_BERSERKER) && (GET_HIT(ch) > 0)
    && affected_by_spell(ch, SKILL_BERSERK) )
  {
    if(IS_NPC(ch))
    {
      dam = (dam * BOUNDED(1, ((GET_MAX_HIT(ch) / GET_HIT(ch)) / 4), 2)); 
    }
    else
    {
      dam = (dam * BOUNDED(1, ((GET_MAX_HIT(ch) / GET_HIT(ch)) / 2), 3));
    }

    // Since rage no longer flurries, this is going to be a damage increase
    // while the berserker is raged.
    if(affected_by_spell(ch, SKILL_RAGE))
    {
      dam *= dam_factor[DF_RAGED];
    }
  }

  // Earth elementals ignore earth aura.
  if(IS_AFFECTED2(victim, AFF2_EARTH_AURA)
    && !(flags & PHSDAM_NOREDUCE) && !number(0, 5) && GET_RACE(ch) != RACE_E_ELEMENTAL)
  {
    dam = 0;
    act("$n's &+yattack glances off of your stone hide!",
        TRUE, ch, NULL, victim, TO_VICT);
    act("$n's &+yattack glances off of&n $N's &+ystone hide!",
        TRUE, ch, NULL, victim, TO_NOTVICT);
    act("&+yYour attack glances off of&n $N's &+ystone hide!",
        TRUE, ch, NULL, victim, TO_CHAR);
  }

  // Combat mind bypasses displacement.
  if(affected_by_spell(victim, SPELL_DISPLACEMENT) && (!number(0, 5)) && !(flags & PHSDAM_NOREDUCE)
    && !affected_by_spell(ch, SPELL_COMBAT_MIND))
  {
    dam = 0;
    act("&+WThe aura of displacement around&n $n&+W absorbs most of the assault!&n",
        FALSE, victim, 0, 0, TO_ROOM);
    act("$e THOUGHT $s was going to hit you...&n", FALSE, victim, 0, 0, TO_CHAR);
  }

  if (affected_by_spell(victim, SPELL_STONE_SKIN) && !(flags & PHSDAM_NOREDUCE))
  {
    reduction = 1. - get_property("damage.reduction.stoneSkin", 0.75);
    skin = SPELL_STONE_SKIN;
  }
  else if (affected_by_spell(victim, SPELL_BIOFEEDBACK) && !(flags & PHSDAM_NOREDUCE))
  {
    reduction = 1. - get_property("damage.reduction.biofeedback", 0.65);
    skin = SPELL_BIOFEEDBACK;
  }
  else if (affected_by_spell(victim, SPELL_SHADOW_SHIELD) && !(flags & PHSDAM_NOREDUCE))
  {
    reduction = 1. - get_property("damage.reduction.stoneSkin", 0.75);
    skin = SPELL_SHADOW_SHIELD;
  }
  else if (affected_by_spell(victim, SPELL_IRONWOOD) && !(flags & PHSDAM_NOREDUCE))
  {
    reduction = 1. - get_property("damage.reduction.ironwood", 0.80);
    skin = SPELL_IRONWOOD;
  }
  else if (affected_by_spell(victim, SPELL_ICE_ARMOR) && !(flags & PHSDAM_NOREDUCE))
  {
    reduction = 1. - get_property("damage.reduction.icearmor", .75);
    skin = SPELL_ICE_ARMOR;
  }
  else if (affected_by_spell(victim, SPELL_NEG_ARMOR) && !(flags & PHSDAM_NOREDUCE))
  {
    reduction = 1. - get_property("damage.reduction.negarmor", .75);
    skin = SPELL_NEG_ARMOR;
  }
  else
  {
    skin = 0;
  }

  //-------------------------------
  // ranged stuff, moved from range.c
  // check for flags & PHSDAM_ARROW
  /*
     if((IS_PC(victim) && dam < 120) || (IS_NPC(victim) && dam < 75))
     {
     act("The missile is slowed by your defensive magic.", FALSE, ch, 0, victim, TO_VICT | ACT_NOTTERSE);
     act("Your missile deflects harmlessly off of $N!", FALSE, ch, 0, victim, TO_CHAR | ACT_NOTTERSE);
     }
     else
     {
     act("The arrow finds a weak point in your defensive magic and strikes true.", FALSE, ch, 0, victim, TO_VICT | ACT_NOTTERSE);
     act("Your missile finds a weak point in&n $N's &ndefensive magic!", FALSE, ch, 0, victim, TO_CHAR | ACT_NOTTERSE);
     }
     */
  //-------------------------------

  if( skin && !dragonfist )
  {
    dam -= get_property("damage.reduction.skinSpell.deduct", 46);

    if( dam > 10 && !(flags & PHSDAM_NOREDUCE) )
      dam -= dam * reduction;

    // for mobs flagged with a skin spell that aren't pets, 1% chance to break it.  pets are more likely to have it broken (5%)
    decrease_skin_counter(victim, skin);
  }

  if(affected_by_spell(ch, ACH_DRAGONSLAYER) && (GET_RACE(victim) == RACE_DRAGON))
  {
    dam *= 1.10;
  }

  if(affected_by_spell(ch, SKILL_DREADNAUGHT) && !(flags & PHSDAM_NOREDUCE))
  {
    dam *= .2;
  }

  if(affected_by_spell(victim, SKILL_DREADNAUGHT) && !(flags & PHSDAM_NOREDUCE))
  {
    dam *= .4;
  }

  dam = MAX(1, dam);

  messages->type |= 1 << 24;

  result = raw_damage(ch, victim, dam, RAWDAM_DEFAULT | flags, messages);

  if(result != DAM_NONEDEAD)
  {
    return result;
  }

  if( ilogb(dam) / 2 > number(1, 100) )
  {
    DamageStuff(victim, SPLDAM_GENERIC);
  }

  if( dam <= 5 || (flags & PHSDAM_NOSHIELDS) )
  {
    if( !(flags & PHSDAM_NOENGAGE) )
      attack_back(ch, victim, TRUE);
    return result;
  }

  // If they actually took damage, check for coldshield/fireshield/etc.
  if( dam > 0 )
  {
    shld_result = check_shields(ch, victim, dam, flags);
  }
  else
  {
    shld_result = DAM_NONEDEAD;
  }

  if(shld_result == DAM_CHARDEAD)
  {
    return DAM_VICTDEAD;
  }
  else if(shld_result == DAM_VICTDEAD)
  {
    return DAM_CHARDEAD;
  }
  else if(shld_result == DAM_NONEDEAD && !(flags & PHSDAM_NOENGAGE))
  {
    return attack_back(ch, victim, TRUE);
  }

  return shld_result;
}

void check_vamp(P_char ch, P_char victim, double fdam, uint flags)
{
  int dam = (int) fdam, vamped = 0, wdam;
  struct group_list *group;
  P_char   tch;
  double temp_dam, bt_gain, fcap, fhits, sac_gain, can_mana;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || (dam < 1) )
  {
    return;
  }

  // Allowing cannibalize through all weapons.
  if( IS_AFFECTED3(ch, AFF3_CANNIBALIZE) && dam > 2 )
  {
    can_mana = MAX(10, dam);
    can_mana = BOUNDED(0, can_mana, 100);
    can_mana *= GET_LEVEL(ch) / 30.;
    can_mana = BOUNDED(0, GET_MANA(victim), can_mana);

    GET_MANA(ch) += (int)can_mana;
    if( GET_MANA(ch) > GET_MAX_MANA(ch) )
    {
      GET_MANA(ch) = GET_MAX_MANA(ch);
    }

    if( GET_MANA(ch) < GET_MAX_MANA(ch) )
    {
      StartRegen(ch, EVENT_MANA_REGEN);
    }

    GET_MANA(victim) -= (int)can_mana;
    // BOUNDED(0, number(1, can_mana), GET_MANA(victim));

    if( GET_MANA(victim) < GET_MAX_MANA(victim) )
    {
      StartRegen(victim, EVENT_MANA_REGEN);
    }
  }

  // Negative energy barrier prevents all forms of hitpoint vamp.
  if(affected_by_spell(victim, SPELL_NEG_ENERGY_BARRIER))
  {
    return;
  }

  // So does not being alive!
  if(GET_RACE(victim) == RACE_CONSTRUCT)
  {
    return;
  }

  if(flags & SPLDAM_NOVAMP)
  {
    return;
  }

  // Arrow type vamp. Apr09 -Lucrot
  if( (flags & PHSDAM_ARROW) && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH)
    && IS_PC(ch) && !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) && dam >= 4 )
  {
    vamped = vamp(ch, dam * dam_factor[DF_ARROWVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
  }

  // Physical type actions that vamp
  // PC vamp touch.
  if( (flags & PHSDAM_TOUCH) && !vamped && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH)
    && IS_PC(ch) && !IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) ) // && !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) )
  {
    // Illithids get full regular touch vamp (since they have lousy str).
    if( IS_ILLITHID(ch) )
    {
      vamped = vamp(ch, dam * dam_factor[DF_TOUCHVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    // The class order makes a difference to multiclass chars.
    else if(GET_CLASS(ch, CLASS_ANTIPALADIN))
    {
      vamped = vamp(ch, dam * dam_factor[DF_ANTIPALADINVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else if(GET_CLASS(ch, CLASS_MONK))
    {
      vamped = vamp(ch, dam * dam_factor[DF_MONKVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else if(GET_CLASS(ch, CLASS_MERCENARY))
    {
      vamped = vamp(ch, dam * dam_factor[DF_MERCENARYVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else if(GET_CLASS(ch, CLASS_WARRIOR))
    {
      vamped = vamp(ch, dam * dam_factor[DF_WARRIORVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else if(GET_CLASS(ch, CLASS_BERSERKER))
    {
      vamped = vamp(ch, dam * dam_factor[DF_BERSERKERVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else if(GET_CLASS(ch, CLASS_ROGUE))
    {
      vamped = vamp(ch, dam * dam_factor[DF_ROGUEVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else if(GET_CLASS(ch, CLASS_PALADIN))
    {
      vamped = vamp(ch, dam * dam_factor[DF_PALADINVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else if(GET_CLASS(ch, CLASS_RANGER))
    {
      vamped = vamp(ch, dam * dam_factor[DF_RANGERVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else if(GET_CLASS(ch, CLASS_AVENGER) || GET_CLASS(ch, CLASS_DREADLORD))
    {
      vamped = vamp(ch, dam * dam_factor[DF_DLORDAVGRVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    else
    {
      //vamped = vamp(ch, dam * dam_factor[DF_TOUCHVAMP], GET_MAX_HIT(ch) * 1.10); //113?
      vamped = vamp(ch, dam * dam_factor[DF_TOUCHVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
  }

  // NPC vamp touch.
  // This overrides undead vamp.
  if( (flags & PHSDAM_TOUCH) && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH)
    && IS_NPC(ch) && !IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) )
  {
    vamped = vamp(ch, dam * dam_factor[DF_TOUCHVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
  }
  // end TOUCHVAMP

  // Battle X section:
  // btx vamp(also short_vamp) - Jexni

  // This is battle x vamp from your own attacks:

  if( !vamped && ch != victim && !IS_AFFECTED4(victim, AFF4_HOLY_SACRIFICE)
    && ((flags & RAWDAM_BTXVAMP) || (flags & RAWDAM_SHRTVMP))
    && (IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) || affected_by_spell(ch, SKILL_SHORT_VAMP)) )
  {
    temp_dam = 0;
    temp_dam = number(1, (int) (temp_dam));

    if( IS_PC(ch) )
      // && GET_LEVEL(ch) >= 46)
    {
      temp_dam = dam * get_property("vamping.self.battleEcstasy", 0.150);
      //vamp(ch, temp_dam, GET_MAX_HIT(ch) * get_property("vamping.BTX.self.HP.PC", 1.10));
      //vamp(ch, temp_dam, GET_MAX_HIT(ch) * 1.10); 
      vamp(ch, temp_dam, GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }

    if(IS_NPC(ch))
    {
      temp_dam = dam * get_property("vamping.self.NPCbattleEcstasy", 0.050);
      vamp(ch, temp_dam, GET_MAX_HIT(ch) * 1.1);
    }
  }

  // This is battle x vamp for PC group hits and damage spells.
  if( dam >= 10 && ((flags & RAWDAM_BTXVAMP) || (flags & PHSDAM_ARROW)) )
  {
    if( IS_PC(ch) || IS_PC_PET(ch) )
    {
      for(group = ch->group; group; group = group->next)
      {
        tch = group->ch;

        if( IS_AFFECTED4(tch, AFF4_BATTLE_ECSTASY) && tch->in_room == ch->in_room && tch != ch )
        {
          // Have to use BOUNDEDF here for floats.. *sigh*
          vamp(tch, dam * get_property("vamping.battleEcstasy", .140), BOUNDEDF(1.10, (GET_C_POW(tch) / 90.0), 2.20) * GET_MAX_HIT(tch) );
        }
      }
    }

    // NPCs do not group, thus we use a room search and separate code.
    if( IS_NPC(ch) && !IS_PC_PET(ch) )
    {
      temp_dam = number(1, (int) (temp_dam));

      for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {

        if( IS_NPC(tch) && !IS_PC_PET(tch)
          && IS_SET((tch)->specials.affected_by4, AFF4_BATTLE_ECSTASY)
          && tch->in_room == ch->in_room && tch != ch )
        {
          vamp(tch, temp_dam, GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
        }
      }
    }
  }

  if( !vamped && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH)
    && (flags & RAWDAM_TRANCEVAMP) && (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM)) )
  {
    vamped = vamp(ch, dam * dam_factor[DF_TRANCEVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
  }

  // hellfire vamp
  if( !vamped && (flags & PHSDAM_HELLFIRE) && IS_AFFECTED4(ch, AFF4_HELLFIRE) )
  {
    P_obj weapon = ch->equipment[PRIMARY_WEAPON];
    wdam = 1;

    if( IS_NPC(ch) )
    {
      wdam = MIN(dam, dice(ch->points.damnodice, MAX(1, ch->points.damsizedice)));
    }
    else if( IS_PC(ch) && weapon )
    {
      wdam = MIN(dam, dice(weapon->value[1], MAX(1, weapon->value[2])));
    }
    if( wdam )
    {
      vamped = vamp(ch, wdam * dam_factor[DF_HFIREVAMP], GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
  }

  // BATTLETIDE VAMP
  if( !vamped && (flags & PHSDAM_BATTLETIDE) && IS_AFFECTED4(ch, AFF4_BATTLETIDE)
    && !affected_by_spell(ch, SPELL_PLAGUE) )
  {
    bt_gain = dam * dam_factor[DF_BATTLETIDEVAMP];

    for(group = ch->group; group; group = group->next)
    {
      tch = group->ch;
      if(tch->in_room == ch->in_room && tch != ch)
      {
        vamped = vamp(tch, bt_gain, GET_MAX_HIT(tch));
      }
    }
  }

  // Tranced vamping
  if( !vamped && (flags & RAWDAM_TRANCEVAMP) && (IS_UNDEADRACE(ch) || IS_ANGEL(ch))
    && !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) )
  {
    if( IS_DRACOLICH(ch) )
    {
      fhits = dam * dam_factor[DF_DRACOLICHVAMP];
      vamped = vamp(ch, fhits, GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    if( IS_NPC(ch) )
    {
      fhits = dam * dam_factor[DF_NPCVAMP];
      vamped = vamp(ch, fhits, GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    }
    // 10 dam * .3 = 3 points of vamp minimum. 'cause I said so.
    else if( dam >= 12 && IS_PC(ch) )
    {
      fhits = dam * dam_factor[DF_UNDEADVAMP];
      fcap = GET_MAX_HIT(ch);
      // Liches vamp from spells.
      if( flags & SPLDAM_SPELL && GET_RACE(ch) == RACE_LICH )
        fcap *= VAMPPERCENT(ch);
      vamped = vamp(ch, fhits, fcap);
    }
  }

  // Illesarus vamp through weapon, but only if they haven't vamped previous to this - Jexni 12/20/08
#define HOA_ILLESARUS_VNUM 77738
  if( !vamped && ch->equipment[WIELD]
    && (obj_index[ch->equipment[WIELD]->R_num].virtual_number == HOA_ILLESARUS_VNUM) )
  {
    vamped = vamp(ch, MIN(dam, number(2, 7)), GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
  }

  if( (dam >= 2 && !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY)
    && IS_AFFECTED4(victim, AFF4_HOLY_SACRIFICE)
    && (flags & RAWDAM_HOLYSAC) && !affected_by_spell(victim, SPELL_BMANTLE)
    && !affected_by_spell(victim, SPELL_PLAGUE)) )
/* Taking this out as it doesn't seem necessary.
    && ((GOOD_RACE(victim) && !GOOD_RACE(ch))
    || (EVIL_RACE(victim) && !EVIL_RACE(ch))))
*/
  {
    sac_gain = dam * get_property("vamping.holySacrifice", 0.035);
    for (group = victim->group; group; group = group->next)
    {
      tch = group->ch;

      if( tch->in_room == victim->in_room && tch != victim
        && !IS_AFFECTED4(tch, AFF4_HOLY_SACRIFICE)
        && !affected_by_spell(tch, SPELL_PLAGUE) )
      {
        vamp(tch, sac_gain, GET_MAX_HIT(tch)); // Holy Sac only vamps to max hp - Jexni 12/9/10
      }
    }
  }
}


/*
 * returns TRUE if victim is no longer attackable (dead/fled/linkdead rescue/etc),
 * else FALSE.  With proper
 * checks in calling routines, this should make us much more
 * stable. messages should provide set of messages for attacker, victim
 * and room in non-killing and killing version. messages->type is a flag
 * which can be 0, DAMMSG_TERSE, DAMMSG_MELEE or a combination of those.
 * DAMMSG_TERSE means message will not be shown for people with terse on,
 * DAMMSG_MELEE is reserved for hit() messages which contain information
 * about victim's condition after damage is dealt.
 *
 * do not call any version of damage with dam == 0, if you want to display
 * a miss message, do it yourself, if you want to engage, use set_fighting()
 *
 * TO DO: change return type to:
 *   VICT_DEAD == 1
 *   CHAR_DEAD == -1
 *   BOTH_DEAD == -2
 *   NONE_DEAD == 0
 * adapt code using damage(). it will allow us to avoid a lot
 * of expensive char_in_list() calls
 * probably the only case where damage can return -2 is when a dying
 * mob procs on death, spell_damage and melee_damage can quite
 * often return -1, on deflects and shields
 */
int raw_damage(P_char ch, P_char victim, double dam, uint flags, struct damage_messages *messages)
{
  struct affected_type *af, *next_af;
  struct group_list *gl;
  char     buffer[MAX_STRING_LENGTH];
  P_char   tch, orig;
  int i, nr, max_hit, diff, room, new_stat, act_flag, soulWasTrapped = 0;
  int group_size = num_group_members_in_room(victim);
  float mod, hpperc, zerkmod;

  if( !ch )
  {
    logit(LOG_EXIT, "raw_damage in fight.c called without ch");
    raise(SIGSEGV);
  }

  if( !victim )
  {
    return DAM_NONEDEAD;
  }

  switch( GET_RACEWAR(ch) )
  {
    case RACEWAR_GOOD:
      dam *= dam_factor[DF_GOOD_MODIFIER];
    case RACEWAR_EVIL:
      dam *= dam_factor[DF_EVIL_MODIFIER];
    case RACEWAR_UNDEAD:
      dam *= dam_factor[DF_UNDEAD_MODIFIER];
    case RACEWAR_NEUTRAL:
      dam *= dam_factor[DF_NEUTRAL_MODIFIER];
  }

  if(ch && victim) // Just making sure.
  {

    //Client
    for (gl = victim->group; gl; gl = gl->next)
    {
      if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
      {
        gl->ch->desc->last_group_update = 1;
      }
    }

    /* we crash, because victim has most likely been extracted and its memory may now belong to something else */
    if (GET_STAT(victim) == STAT_DEAD)
    {
      logit(LOG_DEBUG, "damage called on dead character (%s)!", GET_NAME(victim));
      return DAM_VICTDEAD;
    }

    // CMD_FIRE (do_fire) handles its own hide removal.  This is important for shadow archery.
    appear(ch, (flags & !PHSDAM_ARROW) );
    appear(victim);

    if(victim != ch)
    {
      if(CHAR_IN_SAFE_ROOM(ch))
      {
        return DAM_NONEDEAD;
      }

      if (should_not_kill(ch, victim))
      {
        return DAM_NONEDEAD;
      }

      if( IS_TRUSTED(victim) && IS_SET(victim->specials.act, PLR_AGGIMMUNE) )
      {
        return DAM_NONEDEAD;
      }

      //justice_witness(ch, victim, CRIME_ATT_MURDER);

      if(victim->following == ch)
      {
        stop_follower(victim);
      }

      if(IS_NPC(ch) && !IS_PC_PET(ch))
      {
        dam = (int) (dam * get_property("damage.mob.bonus", 1.0));
        dam = MIN(dam, 800);
      }

      if(affected_by_spell(ch, TAG_BLOODLUST) && !IS_PC_PET(victim) && IS_NPC(victim))
      {
        int dammod;
        struct affected_type *findaf, *next_af;  //initialize affects
        for(findaf = ch->affected; findaf; findaf = next_af)
        {
          next_af = findaf->next;
          if((findaf && findaf->type == TAG_BLOODLUST))
          {
            dammod = findaf->modifier;
          }
        }
        dam = (int) (dam * (1 + (dammod * .01)));
      }

      if (IS_HARDCORE(ch))
      {
        dam = (int) dam *1.09;
      }

      if (IS_HARDCORE(victim))
      {
        dam = (int) dam *0.91;
      }

      if(IS_GOOD(ch) && affected_by_spell(ch, SPELL_HOLY_SWORD) && IS_EVIL(victim))
      {
        dam = ((int) dam + dice(2, 6));
      }

      if(IS_EVIL(ch) && affected_by_spell(ch, SPELL_HOLY_SWORD) && IS_GOOD(victim))
      {
        dam = ( (int) dam - dice(2, 6) );
      }

      if( IS_AFFECTED4(victim, AFF4_SANCTUARY) && (flags & RAWDAM_SANCTUARY)
        && (GET_CLASS(victim, CLASS_PALADIN)) )
      {
        dam *= dam_factor[DF_SANC];
        float group_mod = 1.0 - ( (float) group_size * 
            (float) get_property("damage.reduction.sanctuary.paladin.groupMod", 0.02) );
        //capped at 35% of total damage taken with group of 16.
        dam *= MAX(.45, group_mod);
      }

      if( IS_AFFECTED4(victim, AFF4_SANCTUARY) && (flags & RAWDAM_SANCTUARY)
        && GET_CLASS(victim, CLASS_CLERIC) )
      {
        dam *= dam_factor[DF_SANC];
      }

      if( IS_AFFECTED4(victim, AFF4_SANCTUARY) && (flags & RAWDAM_SANCTUARY)
        && (!GET_CLASS(victim, CLASS_CLERIC) && !GET_CLASS(victim, CLASS_PALADIN)) )
      {
        dam *= dam_factor[DF_SANC];
      }

      if( ((IS_EVIL(ch) && !IS_EVIL(victim)) || (IS_GOOD(ch) && !IS_GOOD(victim))) && !(flags & PHSDAM_NOREDUCE)
        && (af = get_spell_from_char(victim, SPELL_VIRTUE)) )
      {
        // Reduces up to 7% at lvl 56.  Not bad for 6th circle spell.
        dam *= 1 - af->modifier/800.0;
      }

      if( get_spell_from_room(&world[ch->in_room], SPELL_CONSECRATE_LAND) && !(flags & PHSDAM_NOREDUCE))
      {
        dam = ((int) dam) >> 1;
      }

      if( get_spell_from_room(&world[ch->in_room], SPELL_BINDING_WIND) && !(flags & PHSDAM_NOREDUCE))
      {
        dam = (int) dam *0.80;
      }

      if(IS_AFFECTED3(victim, AFF3_PROT_ANIMAL) && IS_ANIMAL(ch) && !(flags & PHSDAM_NOREDUCE))
      {
        dam = dam_factor[DF_PROTANIMAL] * dam;
      }

      if(IS_AFFECTED3( ch, AFF3_PALADIN_AURA ) && has_aura(ch, AURA_BATTLELUST ) && !(flags & PHSDAM_NOREDUCE))
      {
        dam += dam *
          (( get_property("innate.paladin_aura.battlelust_mod", 0.2 ) * aura_mod(ch, AURA_BATTLELUST) ) / 100 );
      }

      if (has_innate(ch, INNATE_WARCALLERS_FURY))
      {
        double bonus = BOUNDEDF(.02, (group_size / 100) + (GET_LEVEL(ch) / 1120), (double).15);

        dam += dam * bonus;
        if( has_innate(ch, INNATE_WARCALLERS_FURY) && ch->group )
        {
          int count = 0;
          for(struct group_list *gl = ch->group; gl; gl = gl->next)
          {
            if( ch != gl->ch && IS_PC(gl->ch)
              && ch->in_room == gl->ch->in_room
              && has_innate(gl->ch, INNATE_WARCALLERS_FURY) )
            {
              count++;
            }
          }
          if( count > 0 )
          {
            count = MIN(count, (int) get_property("innate.warcallersfury.maxSteps", 3));
            dam += dam * ((double)count / 30);
          }
        }
      }

    }

    if( !(flags & PHSDAM_NOREDUCE) )
    {
      dam = ((int) dam) >> 1;
    }

    if(IS_NPC(ch) && !IS_PC_PET(ch) && !IS_MORPH(ch)
      && (IS_PC(victim) || IS_PC_PET(victim) || IS_MORPH(victim)))
    {
      dam = dam * (dam_factor[DF_NPCTOPC] / 2);
      if (GET_RACEWAR(victim) == RACEWAR_GOOD)
      {
        dam = dam * (float)get_property("damage.modifier.npcToPc.good", 1.000);
      }
      if (GET_RACEWAR(victim) == RACEWAR_EVIL)
      {
        dam = dam * (float)get_property("damage.modifier.npcToPc.evil", 1.000);
      }
    }
    else if( !(flags & PHSDAM_NOREDUCE) )
    {
      dam = ((int) dam) >> 1;
    }

    dam = BOUNDED(1, (int) dam, 32766);

    check_blood_alliance(victim, (int)dam);

    if( IS_AFFECTED5(victim, AFF5_IMPRISON) && (flags & RAWDAM_IMPRISON)
      && handle_imprison_damage(ch, victim, (int) dam) )
    {
      return DAM_NONEDEAD;
    }

    if( IS_AFFECTED5(victim, AFF5_VINES) && (flags & RAWDAM_VINES)
      && (af = get_spell_from_char(victim, SPELL_VINES)) )
    {
      bool  vine_success = FALSE;
      if( number((20 + af->modifier / 4), 100) )
      {
        act("The &+Gvines&n protecting you bear the brunt of the assault.",
            FALSE, ch, 0, victim, TO_VICT);
        act("The &+Gvines&n protecting $N bear the brunt of the assault.",
            FALSE, ch, 0, victim, TO_NOTVICT);
        act("The &+Gvines&n surrounding $N bear the brunt of your attack.",
            FALSE, ch, 0, victim, TO_CHAR);
        vine_success = TRUE;
      }

      af->modifier -= (int) dam;

      if( af->modifier <= 0 )
      {
        act("The &+Gvines&n surrounding you wither and die.",
            FALSE, ch, 0, victim, TO_VICT);
        act("The &+Gvines&n surrounding $N wither and die.",
            FALSE, ch, 0, victim, TO_NOTVICT);
        act("The &+Gvines&n surrounding $N wither and die.",
            FALSE, ch, 0, victim, TO_CHAR);
        REMOVE_BIT(victim->specials.affected_by5, AFF5_VINES);
        affect_from_char(victim, SPELL_VINES);
      }

      if (vine_success)
      {
        return (DAM_NONEDEAD);
      }
    }

    if( !IS_TRUSTED(victim) )
    {
      if( flags & RAWDAM_NOKILL )
      {
        if( GET_HIT(victim) > 1 )
        {
          if( GET_HIT(victim) - dam < 1 )
          {
            dam = GET_HIT(victim) - 1;
          }
        }
        else
        {
          dam = 0;
        }
      }
      else if( GET_HIT(victim) - dam < -11 )
      {
        dam = GET_HIT(victim) + 11;
      }
      GET_HIT(victim) -= dam;
    }

    if( IS_PC(victim) && victim->desc && (IS_SET(victim->specials.act, PLR_SMARTPROMPT)
      || IS_SET(victim->specials.act, PLR_OLDSMARTP)) )
    {
      victim->desc->prompt_mode = TRUE;
    }
    // Switched monster for example
    else if( victim->desc && (orig = victim->desc->original) != NULL )
    {
      if( IS_SET(orig->specials.act, PLR_SMARTPROMPT)
        || IS_SET(orig->specials.act, PLR_OLDSMARTP) )
      {
        orig->desc->prompt_mode = TRUE;
      }
    }

    // Exps for damage
    // only getting damage exp once from the same mob, to prevent cheese
    if( IS_NPC(victim) && GET_HIT(victim) < GET_LOWEST_HIT(victim) )
    {
      if(!(flags & RAWDAM_NOEXP))
      {
        gain_exp(ch, victim, MIN(dam, GET_LOWEST_HIT(victim) - GET_HIT(victim)), EXP_DAMAGE);
      }
      GET_LOWEST_HIT(victim) = GET_HIT(victim);
    }

    snprintf(buffer, MAX_STRING_LENGTH, "&+w[Damage: %2d ] &n", (int) dam);

    if( IS_PC(ch) && !IS_TRUSTED(ch) && IS_SET(ch->specials.act2, PLR2_DAMAGE) )
    {
      send_to_char(buffer, ch);
    }

    for (tch = world[victim->in_room].people; tch; tch = tch->next_in_room)
    {
      if( IS_TRUSTED(tch) && IS_SET(tch->specials.act2, PLR2_DAMAGE) )
      {
        send_to_char(buffer, tch);
      }
      // If it's a charmie, charmed by a PC with PET_DAMAGE toggled on.
      // Note: We want pet damage to show for illusionist / no-order pets which are
      //   affected by SPELL_CHARM_PERSON, but not AFF_CHARM.
      else if( (IS_AFFECTED( ch, AFF_CHARM ) || affected_by_spell( ch, SPELL_CHARM_PERSON ))
        && IS_PC(tch) && IS_SET(tch->specials.act3, PLR3_PET_DAMAGE) && tch == GET_MASTER(ch) )
      {
        send_to_char(buffer, tch);
      }
    }

    if( messages && messages->type & DAMMSG_TERSE )
    {
      act_flag = ACT_NOTTERSE;
    }
    else
    {
      act_flag = 0;
    }
/*
    char showdam[MAX_STRING_LENGTH];
    snprintf(showdam, MAX_STRING_LENGTH, " [&+wDamage: %d&n] ", (int) dam);
*/
    new_stat = calculate_ch_state(victim);

    if(new_stat == STAT_DEAD)
    {
      for (af = victim->affected; af; af = af->next)
      {
        if(af->type == TAG_RACE_CHANGE)
        {
          victim->player.race = af->modifier;
          affect_remove(victim, af);
        }
      }
    }

    if( !messages )
    {
      if (new_stat == STAT_DEAD)
        soul_trap(ch, victim);
    }
    else if (new_stat == STAT_DEAD)
    {
      if (!soul_trap(ch, victim))
      {
        act(messages->death_attacker, FALSE, ch, messages->obj, victim, TO_CHAR);
        act(messages->death_victim, FALSE, ch, messages->obj, victim, TO_VICT);
        act(messages->death_room, FALSE, ch, messages->obj, victim, TO_NOTVICTROOM);
      }
    }
    else if( messages->type & (DAMMSG_EFFECT_HIT | DAMMSG_EFFECT | DAMMSG_HIT_EFFECT) )
    {
      dam_message(dam, ch, victim, messages);
    }
    else
    {
      act(messages->attacker, FALSE, ch, messages->obj, victim, TO_CHAR | act_flag);
      act(messages->victim, FALSE, ch, messages->obj, victim, TO_VICT | act_flag);
      act(messages->room, FALSE, ch, messages->obj, victim, TO_NOTVICTROOM | act_flag);
    }

    if( GET_STAT(victim) == STAT_SLEEPING && new_stat != STAT_DEAD )
    {
      act("$n has a RUDE awakening!", TRUE, victim, 0, 0, TO_ROOM);
      affect_from_char(victim, SPELL_SLEEP);
      if(IS_AFFECTED(victim, AFF_SLEEP))
      {
        REMOVE_BIT(victim->specials.affected_by, AFF_SLEEP);
      }
      SET_POS(victim, GET_POS(victim) + STAT_NORMAL);
    }

    /* instant nuke of minor para if hit */
    if( IS_AFFECTED2(victim, AFF2_MINOR_PARALYSIS) && ch != victim )
    {
      act("$n's crushing blow frees $N from a magic which held $M motionless.", FALSE, ch, 0, victim, TO_ROOM);
      act("$n's blow shatters the magic paralyzing you!", FALSE, ch, 0, victim, TO_VICT);
      act("Your blow disrupts the magic keeping $N frozen.", FALSE, ch, 0, victim, TO_CHAR);
      for (af = victim->affected; af; af = next_af)
      {
        next_af = af->next;
        if (af->bitvector2 & AFF2_MINOR_PARALYSIS)
        {
          affect_remove(victim, af);
        }
      }
      REMOVE_BIT(victim->specials.affected_by2, AFF2_MINOR_PARALYSIS);
    }

    /* if char is bound there is a chance the dam cut the binding */
    if (IS_AFFECTED(victim, AFF_BOUND))
    {
      if (number(0, 100) <= dam)
      {
        /* ok char if free */
        REMOVE_BIT(victim->specials.affected_by, AFF_BOUND);
        act("$N's bindings are cut free!", FALSE, ch, 0, victim, TO_NOTVICT);
        act("Your blow cuts through $N's bindings.", FALSE, ch, 0, victim, TO_CHAR);
        send_to_char("Your bindings are cut from the damage, you're free!\n", victim);
      }
    }


    /* make mirror images disappear */
    if( victim && IS_NPC(victim) && !(flags & RAWDAM_NOKILL)
      && GET_VNUM(victim) == 250 && (GET_HIT(victim) < 15) )
    {
      act("Upon being struck, $n disappears into thin air.", TRUE, victim, 0, 0, TO_ROOM);
      extract_char(victim);
      return DAM_VICTDEAD;
    }

    if( (ch != victim) )
    {
      check_vamp(ch, victim, dam, flags);
    }

    if( victim && IS_DISGUISE(victim) )
    {
      if( (victim->disguise.hit < 0) || affected_by_spell(victim, SPELL_MIRAGE) )
      {
        remove_disguise(victim, TRUE);
      }
      else if( victim )
      {
        victim->disguise.hit -= (int) dam;
      }
    }

    if(new_stat == STAT_DEAD)
    {
      P_char   killer = NULL;

      if( ch && IS_NPC(ch) )
      {
        if( GET_MASTER(ch) && IS_PC(GET_MASTER(ch)) )
        {
          killer = GET_MASTER(ch);
        }
      }
      else
      {
        killer = ch;
      }

      if( !affected_by_spell(victim, TAG_PVPDELAY) && IS_PC(victim) )
      {
        char bufpc[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];

        //send_to_char("no pvp here! die and make portal\r\n", victim);
        P_obj portal;
        portal = read_object(400220, VIRTUAL);
        portal->value[0] = world[victim->in_room].number;
        snprintf(bufpc, MAX_STRING_LENGTH, "%s %s", GET_NAME(victim), "corpseportal portal");
        portal->name = str_dup(bufpc);
        snprintf(buffer, MAX_STRING_LENGTH, "%s %s&n", portal->description, GET_NAME(victim));
        set_long_description(portal, buffer); 
        set_short_description(portal, buffer);
        obj_to_room(portal, real_room(400000));
      }
      if(victim && killer && IS_PC(victim) && opposite_racewar(killer, victim)
        && !IS_TRUSTED(killer) && !IS_TRUSTED(victim) && (messages->type & 0xff000000))
      {
        DestroyStuff(victim, (messages->type & 0xff000000) >> 24);
        remove_soulbind(victim);
      }
    }

    if (new_stat < STAT_SLEEPING)
    {
      switch (new_stat)
      {
        case STAT_DEAD:
          break;
        case STAT_DYING:
          act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
          if (!number(0, 1))
          {
            act("&+rYou feel your pulse begin to slow and you realize you are mortally wounded...&n", FALSE, victim, 0, 0, TO_CHAR);
          }
          else
          {
            act("&+rYour consciousness begins to fade in and out as your mortality slips away.....&n", FALSE, victim, 0, 0, TO_CHAR);
          }
          break;
        case STAT_INCAP:
          act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
          if( !number(0, 1) )
          {
            act("&+RYou watch as the world spins around your gruesomely cut body.\n"
              "&+RYou feel your strength wane, leaving you for the carrion crawlers to\n"
              "&+Rdevour.&n", FALSE, victim, 0, 0, TO_CHAR);
          }
          else
          {
            act("&+RYour blood rushes out of your veins, as you slowly run out of life.\n"
              "&+RBlood pours from your many serious wounds and your strength fails.\n"
              "&+RYou realize this may have been your last battle, and prepare for oblivion.&n", FALSE, victim, 0, 0, TO_CHAR);
          }
          break;
      }
      if (new_stat != STAT_DEAD)
      {
        StartRegen(victim, EVENT_HIT_REGEN);
      }
    }
    else
    {
      StartRegen(victim, EVENT_HIT_REGEN);
      max_hit = GET_MAX_HIT(victim);

      if( dam > GET_HIT(victim) && ch != victim )
      {
        act("&=LCYIKES!&n&-L  Another hit like that, and you've had it!!", FALSE, victim, 0, 0, TO_CHAR);
      }
      else if( dam > (max_hit / 10) && ch != victim )
      {
        act("&+MOUCH!&n  That really did &+MHURT!&n", FALSE, victim, 0, 0, TO_CHAR);
      }

      if( GET_HIT(victim) < (max_hit / 8) && ch != victim )
      {
        send_to_char("You wish that your wounds would stop &+RBLEEDING&n so much!\r\n", victim);
      }

      if (GET_HIT(victim) < 2 && new_stat > STAT_INCAP && number(0, 1))
      {
        Stun(victim, ch, PULSE_VIOLENCE, FALSE);
      }
    }

    /* new, unless vicious, no auto attacks on helpless targets */

    if( !IS_AWAKE(victim) || (GET_HIT(victim) < -2) || IS_IMMOBILE(victim) )
    {
      if(IS_FIGHTING(victim))
      {
        stop_fighting(victim);
      }
      if(IS_DESTROYING(victim))
      {
        stop_destroying(victim);
      }
      StopMercifulAttackers(victim);
    }

    update_pos(victim);

    if( GET_HIT(victim) < 0 && affected_by_spell(victim, TAG_BUILDING) )
    {
      debug("%s killed building %s.", J_NAME(ch), J_NAME(victim));
      new_stat = STAT_DEAD;
    }

    if (new_stat == STAT_DEAD)
    {
      room = ch->in_room;

      die(victim, ch);
      if( !is_char_in_room(ch, room) )
      {
        return DAM_BOTHDEAD;
      }
      else
      {
        return DAM_VICTDEAD;
      }
    }

    if( IS_MINOTAUR(victim) && !number(0, 5)
      && (GET_HIT(victim) < GET_MAX_HIT(victim) / 6)
      && !affected_by_spell(victim, SKILL_BERSERK) )
    {
      berserk(victim, 6 * PULSE_VIOLENCE);
    }

    if( GET_CLASS(victim, CLASS_BERSERKER)
      && (GET_HIT(victim) < GET_MAX_HIT(victim) / 4)
      && !affected_by_spell(victim, SKILL_BERSERK) )
    {
      berserk(victim, 10 * PULSE_VIOLENCE);
    }

    /*
       if (GET_SPEC(victim, CLASS_BERSERKER, SPEC_RAGELORD)
       && (GET_HIT(victim) < GET_MAX_HIT(victim) / 2))
       if (!affected_by_spell(victim, SKILL_BERSERK))
       {
       int duration = 3 * (MAX(24, (GET_CHAR_SKILL(victim, SKILL_BERSERK) + GET_LEVEL(victim))));
       berserk(victim, MAX(duration, 8 * PULSE_VIOLENCE));
       }
       */

    if(has_innate(victim, INNATE_EMBRACE_DEATH))
    {
      do_innate_embrace_death(victim);
    }

    return DAM_NONEDEAD;
  }
  return 0;
}

int calculate_ac(P_char ch)
{
  int victim_ac = GET_AC(ch);

  // Only drop PCs ac since we don't want to mess with the ac set in zone files.
  if( victim_ac < 0 && IS_PC(ch) )
  {
    victim_ac = ((float)victim_ac * dam_factor[DF_NEG_AC_MULT]);
  }

  // This is strange since load_modifier ranges from 75-300.
//  if( GET_AC(ch) < 1 && load_modifier(ch) > 50 )
  // This ranges from 0 to 225, much more reasonable.
  victim_ac += load_modifier(ch) - 75;

  if(GET_CLASS(ch, CLASS_MONK))
  {
    victim_ac += MonkAcBonus(ch);
  }

  victim_ac += io_agi_defense(ch);

  if( has_innate(ch, INNATE_RRAKKMA) && ch->group )
  {
    for(struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if( ch != gl->ch && IS_PC(gl->ch) && ch->in_room == gl->ch->in_room && has_innate(gl->ch, INNATE_RRAKKMA) )
      {
        victim_ac -= 10;
      }
    }
  }

  return BOUNDED(-750, victim_ac, 100);
}

int calculate_thac_zero(P_char ch, int skill)
{
  int to_hit = 0;

  if(IS_GREATER_RACE(ch) ||
      IS_SET(ch->specials.act, ACT_PROTECTOR))
  {
    to_hit = get_property("to.hit.GreaterRaceTypes", 13);
  }
  else if( GET_CLASS(ch, CLASS_WARRIOR) || GET_CLASS(ch, CLASS_DREADLORD)
    || GET_CLASS(ch, CLASS_AVENGER) || GET_CLASS(ch, CLASS_PALADIN)
    || GET_CLASS(ch, CLASS_ANTIPALADIN) || affected_by_spell(ch, SPELL_COMBAT_MIND)
    || is_wearing_necroplasm(ch) )
  {
    to_hit = get_property("to.hit.WarriorTypes", 10);
  }
  else if( GET_CLASS(ch, CLASS_MERCENARY) || GET_CLASS(ch, CLASS_REAVER)
    || GET_CLASS(ch, CLASS_RANGER) || GET_CLASS(ch, CLASS_BERSERKER)
    || GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER)
    || GET_SPEC(ch, CLASS_THEURGIST, SPEC_THAUMATURGE)
    || GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) )
  {
    to_hit = get_property("to.hit.HitterTankTypes", 8);
  }
  else if( GET_CLASS(ch, CLASS_THIEF) || GET_CLASS(ch, CLASS_BARD)
    || GET_CLASS(ch, CLASS_ROGUE) || GET_CLASS(ch, CLASS_ASSASSIN)
    || GET_CLASS(ch, CLASS_MONK) )
  {
    to_hit = get_property("to.hit.RogueTypes", 7);
  }
  else if( GET_CLASS(ch, CLASS_CLERIC) || GET_CLASS(ch, CLASS_DRUID)
    || GET_CLASS(ch, CLASS_BLIGHTER) || GET_CLASS(ch, CLASS_SHAMAN)
    || GET_CLASS(ch, CLASS_WARLOCK) || GET_CLASS(ch, CLASS_ETHERMANCER)
    || GET_CLASS(ch, CLASS_ALCHEMIST) || GET_CLASS(ch, CLASS_PSIONICIST) )
  {
    to_hit = get_property("to.hit.ClericTypes", 6);
  }
  else if( GET_CLASS(ch, CLASS_SORCERER) || GET_CLASS(ch, CLASS_CONJURER)
    || GET_CLASS(ch, CLASS_NECROMANCER) || GET_CLASS(ch, CLASS_THEURGIST)
    || GET_CLASS(ch, CLASS_ILLUSIONIST) || GET_CLASS(ch, CLASS_MINDFLAYER)
    || GET_CLASS(ch, CLASS_SUMMONER) )
  {
    to_hit = get_property("to.hit.MageTypes", 4);
  }
  else if( IS_NPC(ch) )
  {
    to_hit = get_property("to.hit.npcTypes", 10);
  }
  else
  {
    if( IS_PC(ch) && !number(0, 100))
    {
      statuslog(AVATAR, "The THAC0 for %s's class is not defined, update calculate_thac_zero().", GET_NAME(ch));
    }
    to_hit = get_property("to.hit.npcTypes", 10);
  }

  to_hit = (int) (to_hit * GET_LEVEL(ch) / 6);

  to_hit += IS_NPC(ch) ? BOUNDED(3, (GET_LEVEL(ch) * 2), 95) : skill;

  to_hit = MAX((2 * to_hit) / 3, 30);

  to_hit += BOUNDED(-10, GET_HITROLL(ch) * 2, 90);
  // hard cap for benefit to hitroll(45hr max) Jexni 10/05/08

  if( ch->equipment[PRIMARY_WEAPON]
    && IS_SET(ch->equipment[PRIMARY_WEAPON]->extra2_flags, ITEM2_BLESS) )
  {
    to_hit = (int) (to_hit * get_property("to.hit.BlessBonus", 1.100));
  }

  if( GET_C_LUK(ch) / 2 > number(0, 100) )
  {
    to_hit = (int) (to_hit * get_property("to.hit.LuckBonus", 1.100));
  }

  if( to_hit < 0 )
  {
    wizlog(56, "(%s) has less than 0 thac0: fight.c calculate_thac_zero()", GET_NAME(ch));
    to_hit = 0; // Shouldn't need this, but rather debug the code than pass errors.
  }

  return to_hit;
}

int chance_to_hit(P_char ch, P_char victim, int skill, P_obj weapon)
{
  int to_hit, victim_ac;
  struct affected_type *af;

  if( !IS_ALIVE(ch) )
  {
    return 0;
  }

  // New function to calculate thac0 - Dec08 - Lucrot
  to_hit = calculate_thac_zero(ch, skill);

  if( ((IS_EVIL(ch) && !IS_EVIL(victim)
    && IS_AFFECTED(victim, AFF_PROTECT_EVIL))
    || (IS_GOOD(ch) && !IS_GOOD(victim)
    && IS_AFFECTED(victim, AFF_PROTECT_GOOD))) )
  {
    to_hit -= GET_LEVEL(victim) / 4;
  }

  // This makes no sense commenting it out. Dec08 - Lucrot
  // if((IS_EVIL(ch) && IS_EVIL(victim)) ||
  // (IS_GOOD(ch) && IS_GOOD(victim)))
  // {
  // if (affected_by_spell(ch, SPELL_VIRTUE))
  // {
  // to_hit = (int) (to_hit * get_property("to.hit.VirtueBonus", 1.030));
  // }
  // }

  if( (IS_EVIL(ch) && !IS_EVIL(victim)) || (IS_GOOD(ch) && !IS_GOOD(victim)) )
  {
    if(affected_by_spell(ch, SPELL_VIRTUE))
    {
      to_hit = (int) (to_hit * get_property("to.hit.VirtueBonus", 1.100));
    }
  }

  if( IS_UNDEADRACE(ch) &&  !IS_UNDEADRACE(victim)
    && IS_AFFECTED5(victim, AFF5_PROT_UNDEAD) )
  {
    to_hit = (int) to_hit * get_property("to.hit.ProtUndead", 0.700);
  }

  if( !CAN_SEE(ch, victim) )
  {
    if( IS_NPC(ch) )
    {
      to_hit -= 40 * MIN(100, (120 - 2 * GET_LEVEL(ch))) / 100;
    }
    else
    {
      to_hit -= 40 * (120 - GET_CHAR_SKILL(ch, SKILL_BLINDFIGHTING)) / 100;
      notch_skill(ch, SKILL_BLINDFIGHTING, get_property("skill.notch.blindFighting", 6.25));
    }
  }

  victim_ac = MAX(-100, MIN(calculate_ac(victim), 100));

  if( IS_AFFECTED(victim, AFF_BLIND) )
  {
    victim_ac += (int) (40 *
        (120 - GET_CHAR_SKILL_P(victim, SKILL_BLINDFIGHTING)) / 100);
  }

#ifdef FIGHT_DEBUG
  snprintf(buf, MAX_STRING_LENGTH, "&+Rvictim ac: %d&n ", victim_ac);
  send_to_char(buf, ch);
#endif

  if( !weapon && affected_by_spell(ch, SPELL_VAMPIRIC_TOUCH)
    && !GET_CLASS(ch, CLASS_MONK) )
  {
    to_hit = (int) (to_hit * get_property("to.hit.VampTouch", 1.150));
  }

  if( !IS_AWAKE(victim) || IS_IMMOBILE(victim) )
  {
    to_hit += 100;
  }

  if( GET_POS(victim) < POS_STANDING )
  {
    to_hit += (POS_STANDING - GET_POS(victim)) * 10;
  }

  if( GET_POS(ch) < POS_STANDING )
  {
    to_hit -= (POS_STANDING - GET_POS(ch)) * 15;
  }

  return BOUNDED(1, (to_hit + (victim_ac * 85 / 100)), 100);
}

bool monk_critic(P_char ch, P_char victim)
{
  struct affected_type aff, *af;
  struct damage_messages messages = {
    "$N screams as you sink five fingers into soft spots in $S shoulder.",
    "You feel on fire as $n's hard fingers strike a nerve in your shoulder.",
    "$N screams as $n sinks five fingers into soft spots in $S shoulder.",
    "$N dies as you sink five fingers into soft spots in $S shoulder.",
    "You feel on fire as $n's hard fingers strike a nerve in your shoulder.",
    "$N dies as $n sinks five fingers into soft spots in $S shoulder."
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || IS_CONSTRUCT(victim))
  {
    return FALSE;
  }

  send_to_char("You sneak in and deliver a strike to a pressure point!\r\n", ch);

  if(GET_SPEC(ch, CLASS_MONK, SPEC_WAYOFSNAKE) ||
      (GET_CLASS(ch, CLASS_MONK) && IS_NPC(ch) && GET_LEVEL(ch) > 50))
  {
    af = get_spell_from_char(victim, TAG_PRESSURE_POINTS);
    if (!af)
    {
      memset(&aff, 0, sizeof(aff));
      aff.type = TAG_PRESSURE_POINTS;
      aff.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
      aff.modifier = 1;
      aff.duration = (10 * WAIT_SEC);
      affect_to_char(victim, &aff);
      return FALSE;
    }

    af->modifier++;

    if(af->modifier == 2)
    {
      if(!IS_AFFECTED2(victim, AFF2_SLOW))
      {
        memset(&aff, 0, sizeof(aff));
        aff.type = SPELL_SLOW;
        aff.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
        aff.duration = (4 * WAIT_SEC);
        aff.bitvector2 = AFF2_SLOW;
        affect_to_char(victim, &aff);

        act("&+m$n &+mbegins to sllooowwww down.&n", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char("&+mYou feel yourself slowing down.\n", victim);
      }
    }

    if(af->modifier == 3 &&
        !IS_BLIND(victim))
    {
      blind(ch, victim, (5 * WAIT_SEC));
    }

    if(af->modifier == 4)
    {
      CharWait(victim, (3 * WAIT_SEC));
      act("$n strikes you hard at the side of the neck.", TRUE, ch, 0, victim, TO_VICT);
      act("$n deals a crippling blow to the side of $N's neck.", TRUE, ch, 0, victim, TO_NOTVICT);
      act("You deal a crippling blow to the side of $N's neck.", TRUE, ch, 0, victim, TO_CHAR);
    }

    if(af->modifier == 5)
    {
      if(DAM_NONEDEAD !=
          raw_damage(ch, victim, 240 + dice(4,40), RAWDAM_DEFAULT, &messages))
      {
        return TRUE;
      }
    }
    if(af->modifier == 6)
    {
      affect_from_char( ch, TAG_PRESSURE_POINTS );
    }
  }
  return FALSE;
}

void event_tainted_blade(P_char ch, P_char victim, P_obj obj, void *data)
{
  int blade_skill = GET_CLASS(ch, CLASS_AVENGER) ? SKILL_HOLY_BLADE : SKILL_TAINTED_BLADE;
  struct affected_type *af;
  struct damage_messages tainted_messages = {
    "$N &+Lstruggles against the &+wtaint &+Lcoursing through $S body.&n",
    "Beads of sweat form on your forehead as you battle the taint.",
    "$N &+Lstruggles against the &+wtaint &+Lcoursing through $S body.&n",
    "$N &+Lfalls to the ground thrashing wildly, $S &+wsoul &+Lfinally de&+wvo&+wu&+wr&+Led.",
    "&+LYour screams ar&+we your onl&+Wy thing ke&+weping you compa&+Lny as you fall into oblivion.&n",
    "$N &+Lfalls to the ground thrashing wildly, $S soul finally devoured."
  };
  struct damage_messages holy_messages = {
    "&+rThe wound burns with a &+wsoft white glow.",
    "&+rYour wound &+yburns &+rlike it was on &+Rfire.",
    "$N's &+rwound burns with a &+wsoft white glow.",
    "$N &+Lfalls to the ground thrashing wildly, $S &+wsoul &+Lfinally de&+wvo&+wu&+wr&+Led.",
    "&+LYour screams ar&+we your onl&+Wy thing ke&+weping you compa&+Lny as you fall into oblivion.&n",
    "$N &+Lfalls to the ground thrashing wildly, $S soul finally devoured."
  };
  struct damage_messages *messages = GET_CLASS(ch, CLASS_AVENGER) ? &holy_messages : &tainted_messages;

  af = get_spell_from_char(victim, blade_skill);
  if( !af )
  {
    return;
  }

  // At 56, Min = MAX(40, 3) = 10dam, Simplified Avg = MAX(40, 57) = 14dam, Max = MAX(40, 112) = 28dam
  //  Note: The real average is more complicated since for all X in Y: Y=dice(3, (2*lvl)/3) < 40: X = 40 or whatever.
  if( raw_damage(ch, victim, MAX(40, dice(3, (2*GET_LEVEL(ch))/3)), RAWDAM_DEFAULT ^ RAWDAM_IMPRISON, messages) != DAM_NONEDEAD)
    return;

  if( af->modifier-- > 0 )
  {
    add_event(event_tainted_blade, (int) (IS_AFFECTED(victim, AFF_SLOW_POISON) ? 1.5 : 1) *
      PULSE_VIOLENCE, ch, victim, 0, 0, 0, 0);
  }
  else
  {
    affect_remove(victim, af);
  }
}

bool tainted_blade(P_char ch, P_char victim)
{
  int blade_skill = GET_CLASS(ch, CLASS_AVENGER) ? SKILL_HOLY_BLADE : SKILL_TAINTED_BLADE;
  struct affected_type af, *old_af;
  struct damage_messages tainted_messages = {
    "$N &+wpales &+Las your &+wtainted &+Lweapon strikes&n $M.",
    "&+LYou &+rscream &+Las&n $n's&+L $q slams into your body.&n",
    "$N &+wpales &+Land &+wshivers &+Las&n $n's &+Ltainted weapon strikes&n $M.&n",
    "$N &+Lfalls to the ground &+wspasming &+Las the &+wtaint &+Lconsumes $S soul.&n",
    "&+LAs the &+wfoul taint &+Lfills your every fiber you feel your hold on &+wlife &+Lslipping away&+w...&n",
    "$N &+Lfalls to the ground &+wspasming &+Las the &+wtaint &+Lconsumes $S soul.&n",
    0, ch->equipment[WIELD]
  };
  struct damage_messages holy_messages = {
    "&+WThe holy aura surrounding $q burns into&n $N.",
    "&+WSeering pain flows through you as you are struck by&n $n's $q.",
    "$n's $q &+yglows &+Wbright white&n as it strikes $N.",
    "&+WThe holy aura surrounding $q burns into&n $N.",
    "&+RSeering pain flows through you as you are struck by&n $n's $q.",
    "$n's $q &+yglows &+Wbright white as it strikes&n $N.",
    0, ch->equipment[WIELD]
  };
  struct damage_messages *messages = GET_CLASS(ch, CLASS_AVENGER) ? &holy_messages : &tainted_messages;

  if( IS_CONSTRUCT(victim) || !ch->equipment[WIELD] )
  {
    return FALSE;
  }

  if( raw_damage(ch, victim, 60, RAWDAM_DEFAULT, messages) == DAM_NONEDEAD )
  {
    if (old_af = get_spell_from_char(victim, blade_skill))
    {
      old_af->modifier = 1 + GET_CHAR_SKILL(ch, blade_skill) / 33;
      return FALSE;
    }
    memset(&af, 0, sizeof(af));
    af.type = blade_skill;
    af.duration = 1;
    af.modifier = 1 + GET_CHAR_SKILL(ch, blade_skill) / 33;
    affect_to_char(victim, &af);
    add_event(event_tainted_blade, PULSE_VIOLENCE, ch, victim, 0, 0, 0, 0);
  }
  else
  {
    return TRUE;
  }

  return FALSE;
}

bool frightening_presence(P_char ch, P_char victim)
{
  int      chance;

  if( GET_OPPONENT(victim) == ch )
  {
    return FALSE;
  }

  if( GET_LEVEL(victim) >= GET_LEVEL(ch) )
  {
    chance = 15 + dice(1, 10);
  }
  else
  {
    chance = 15;
  }

  if( number(0, 100) < chance && !fear_check(ch) )
  {
    act("&+rTerror&+L unlike anything you have ever felt overwhelms you.",
        TRUE, ch, 0, victim, TO_CHAR);
    act("Unable to stand the aura of fear surrounding $N $n turns to flee.",
        TRUE, ch, 0, victim, TO_ROOM);

    do_flee(ch, 0, 0);
    return TRUE;
  }

  return FALSE;
}

int      battle_frenzy(P_char, P_char);

int anatomy_strike(P_char ch, P_char victim, int msg, struct damage_messages *messages, int dam)
{
  int skl = GET_CHAR_SKILL(ch, SKILL_ANATOMY);
  struct affected_type af;

  memset(&af, 0, sizeof(af));
  af.type = SKILL_ANATOMY;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_SHORT;

  if( IS_CONSTRUCT(victim) )
    goto regular;

  switch(number(0, 6))
  {
    case 0:
      if( IS_HUMANOID(victim) )
      {
        snprintf(messages->attacker, MAX_STRING_LENGTH, "Your%%s %s hits $N on the torso making $M grimace in pain.",
          attack_hit_text[msg].singular);
        snprintf(messages->victim, MAX_STRING_LENGTH, "$n's%%s %s hits $N on the torso making $M grimace in pain.",
          attack_hit_text[msg].singular);
        snprintf(messages->room, MAX_STRING_LENGTH, "$n's%%s %s hits $N on the torso making $M grimace in pain.",
          attack_hit_text[msg].singular);
        messages->type = DAMMSG_HIT_EFFECT;
      }
      return (int) (dam * 1.1);
    case 1:
      if( !LEGLESS(victim) )
      {
        snprintf(messages->attacker, MAX_STRING_LENGTH, "Your%%s %s hits $N across the leg, resulting in a limp stride.",
          attack_hit_text[msg].singular);
        snprintf(messages->victim, MAX_STRING_LENGTH, "$n's%%s %s hits $N across the leg, resulting in a limp stride.",
          attack_hit_text[msg].singular);
        snprintf(messages->room, MAX_STRING_LENGTH, "$n's%%s %s hits $N across the leg, resulting in a limp stride.",
          attack_hit_text[msg].singular);
        messages->type = DAMMSG_HIT_EFFECT;
        GET_VITALITY(victim) -= 5;
      }
      return dam;
    case 2:
      act("You surprise $N with a blow to the back, leaving $M momentarily confused.",
        FALSE, ch, 0, victim, TO_CHAR);
      act("$n surprises you with a blow to the back, leaving you momentarily confused.",
        FALSE, ch, 0, victim, TO_VICT);
      act("$n surprises $N with a blow to $S back, leaving $M momentarily confused.",
        FALSE, ch, 0, victim, TO_NOTVICT);
      victim->specials.combat_tics = (int)victim->specials.base_combat_round;
      goto regular;
    case 3:
      if( IS_HUMANOID(victim) )
      {
        snprintf(messages->attacker, MAX_STRING_LENGTH, "Your%%s %s reached $N's arm severing tendons and muscles.",
          attack_hit_text[msg].singular);
        snprintf(messages->victim, MAX_STRING_LENGTH, "$n's%%s %s reached your arm severing tendons and muscles.",
          attack_hit_text[msg].singular);
        snprintf(messages->room, MAX_STRING_LENGTH, "$n's%%s %s reached $N's arm severing tendons and muscles.",
          attack_hit_text[msg].singular);
        messages->type = DAMMSG_HIT_EFFECT;
        af.duration = victim->specials.combat_tics + 1;
        af.modifier = -10 - skl/10;
        af.location = APPLY_DAMROLL;
        affect_to_char(victim, &af);
      }
      return dam;
    case 4:
      if( IS_HUMANOID(ch) )
      {
        act("You strike viciously at $N's wrist causing $M to swing about madly.",
          FALSE, ch, 0, victim, TO_CHAR);
        act("$n strikes viciously at your wrist causing you to swing about madly.",
          FALSE, ch, 0, victim, TO_VICT);
        act("$n strikes viciously at $N's wrist causing $M to swing about madly.",
          FALSE, ch, 0, victim, TO_NOTVICT);
        af.duration = victim->specials.combat_tics + 1;
        af.modifier = -15 - skl/10;
        af.location = APPLY_HITROLL;
        affect_to_char(victim, &af);
      }
      goto regular;
    case 5:
      if( IS_CASTING(victim) && IS_HUMANOID(victim) )
      {
        act("You viciously strike at $N's face, causing them to lose their concentration!",
          FALSE, ch, 0, victim, TO_CHAR);
        act("$n viciously strikes at your face, causing you to lose your concentration!",
          FALSE, ch, 0, victim, TO_VICT);
        act("$n viciously strikes at $N's face, causing them to lose their concentration!",
          FALSE, ch, 0, victim, TO_NOTVICT);
        StopCasting(victim);
      }
      goto regular;
    case 6:
      if( IS_HUMANOID(victim) )
      {
        snprintf(messages->attacker, MAX_STRING_LENGTH, "Your%%s %s reached $N's ear causing a gush of blood.",
          attack_hit_text[msg].singular);
        snprintf(messages->victim, MAX_STRING_LENGTH, "$n's%%s %s reached your ear causing a gush of blood.",
          attack_hit_text[msg].singular);
        snprintf(messages->room, MAX_STRING_LENGTH, "$n's%%s %s reached $N's ear causing a gush of blood.",
          attack_hit_text[msg].singular);
        messages->type = DAMMSG_HIT_EFFECT;
        af.duration = 10 * PULSE_VIOLENCE;
        af.bitvector4 = AFF4_DEAF;
        affect_to_char(victim, &af);
      }
      goto regular;
    default:
      goto regular;
  }

regular:

  snprintf(messages->attacker, MAX_STRING_LENGTH, "Your%%s %s %%s.", attack_hit_text[msg].singular);
  snprintf(messages->victim, MAX_STRING_LENGTH, "$n's%%s %s %%s.", attack_hit_text[msg].singular);
  snprintf(messages->room, MAX_STRING_LENGTH, "$n's%%s %s %%s.", attack_hit_text[msg].singular);
  messages->type = DAMMSG_HIT_EFFECT | DAMMSG_TERSE;

  return dam;
}

#ifndef NEW_COMBAT
int required_weapon_skill(P_obj wpn)
{

  if( !wpn )
  {
    return SKILL_UNARMED_DAMAGE;
  }
  else if( wpn->type != ITEM_WEAPON )
  {
    return 0;
  }

  switch( wpn->value[0] )
  {
    case WEAPON_AXE:
    case WEAPON_SHORTSWORD:
    case WEAPON_2HANDSWORD:
    case WEAPON_SICKLE:
    case WEAPON_LONGSWORD:
      return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_2H_SLASHING : SKILL_1H_SLASHING;
      break;
    case WEAPON_DAGGER:
    case WEAPON_HORN:
      if(IS_SET(wpn->extra_flags, ITEM_TWOHANDS) )
      {
        char Gbuf[MAX_STRING_LENGTH];
        snprintf(Gbuf, MAX_STRING_LENGTH, "Weapon '%s' [%d] has 2h flag set and is a %s (%d).",
          wpn->short_description, OBJ_VNUM(wpn), (wpn->value[0] == WEAPON_DAGGER) ? "Dagger" : "Horn", wpn->value[0] );
        debug( Gbuf );
        logit( LOG_OBJ, Gbuf );
      }
      return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? 0 : SKILL_1H_PIERCING;
      break;
    case WEAPON_HAMMER:
    case WEAPON_CLUB:
    case WEAPON_SPIKED_CLUB:
    case WEAPON_MACE:
    case WEAPON_SPIKED_MACE:
    case WEAPON_STAFF:
    case WEAPON_LANCE:
      return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_2H_BLUDGEON : SKILL_1H_BLUDGEON;
      break;
    case WEAPON_WHIP:
    case WEAPON_FLAIL:
    case WEAPON_NUMCHUCKS:
      return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_2H_FLAYING : SKILL_1H_FLAYING;
      break;
    case WEAPON_TRIDENT:
    case WEAPON_SPEAR:
      return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_REACH_WEAPONS : SKILL_1H_PIERCING;
      break;
    case WEAPON_POLEARM:
      return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_REACH_WEAPONS : SKILL_1H_SLASHING;
      break;
    default:
      return 0;
      break;
  }

}
#endif
/*
 * this performs an attack of ch on victim using weapon currently
 * wielded as primary.
 * the function performs check if attacker missed,
 * the hit cannot be dodged/parried/blocked since those are tested in
 * pv_common.
 *
 * TO DO: (These might be done already)
 *   1) Add return value similar to proposed for damage
 *   2) Make hit take slot with weapon as an argument or weapon itself
 *        to avoid silly swapping.
 */
bool hit(P_char ch, P_char victim, P_obj weapon)
{
  P_char   tch, mount, gvict;
  int      msg, victim_ac, to_hit, diceroll, wpn_skill, sic, tmp, wpn_skill_num;
  double   dam;
  int      room, pos;
  int      vs_skill = GET_CHAR_SKILL(ch, SKILL_VICIOUS_STRIKE);
  struct affected_type aff, ir;
  struct affected_type *af;
  char     attacker_msg[512];
  char     victim_msg[512];
  char     room_msg[512];
  struct damage_messages messages;
  struct obj_affect *o_af;
  int      i, blade_skill, chance;
  static bool vicious_hit = false;
  int      devcrit = number(1, 100);

#ifdef FIGHT_DEBUG
  char     buf[512];
#endif

  if( !victim || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( IS_AFFECTED(ch, AFF_BOUND) )
  {
    send_to_char("Your binds are too tight for that!\r\n", ch);
    return FALSE;
  }

  if( IS_IMMOBILE(ch) )
  {
    send_to_char("Ugh, you are unable to move!\r\n", ch);
    return FALSE;
  }

  if( GET_STAT(victim) == STAT_DEAD )
  {
    send_to_char("aww, leave them alone, they are dead already.\r\n", ch);
    statuslog(AVATAR, "%s hitting dead %s", GET_NAME(ch), GET_NAME(victim));
    return FALSE;
  }

  if( ch->in_room != victim->in_room || ch->specials.z_cord != victim->specials.z_cord )
  {
    send_to_char("Who?\r\n", ch);
    return FALSE;
  }

  room = ch->in_room;
  mount = get_linked_char(victim, LNK_RIDING);

  if( mount )
  {
    if( GET_CHAR_SKILL(victim, SKILL_MOUNTED_COMBAT)
      && (notch_skill(victim, SKILL_MOUNTED_COMBAT, get_property("skill.notch.defensive", 17))
      || GET_CHAR_SKILL_P(victim, SKILL_MOUNTED_COMBAT) * 0.3 > number(0, 100)) )
    {
      return hit(ch, mount, weapon);
    }
    /*else if (is_natural_mount(victim, mount) && (GET_LEVEL(victim) * 0.3 > number(0, 100))) // for natural mounts skill is equal to level
      {
      return hit(ch, mount, weapon);
      }*/
  }

  if( !can_hit_target(ch, victim) )
  {
    send_to_char("Seems that it's too crowded!\r\n", ch);
    return FALSE;
  }

  if( weapon && weapon->type != ITEM_WEAPON )
  {
    weapon = NULL;
  }

  if( (IS_PC(ch) || IS_PC_PET(ch)) && IS_PC(victim)
    && !IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE) )
  {
    if( on_front_line(ch) )
    {
      if( !on_front_line(victim) && !(weapon && IS_REACH_WEAPON(weapon)) )
      {
        act("$N tries to attack $n but can't quite reach!", TRUE, victim, 0, ch, TO_NOTVICT);
        act("You try to attack $n but can't quite reach!", TRUE, victim, 0, ch, TO_VICT);
        act("$N tries to attack you but can't quite reach!", TRUE, victim, 0, ch, TO_CHAR);
        return FALSE;
      }
    }
    else
    {
      send_to_char("Sorry, you can't quite reach them!\r\n", ch);
      return FALSE;
    }
  }

  //  play_sound("!!SOUND(battle* P=100)", NULL, ch->in_room, TO_ROOM);

  if( weapon )
  {
    msg = get_weapon_msg(weapon);
  }
  else if( IS_NPC(ch) && ch->only.npc->attack_type )
  {
    msg = ch->only.npc->attack_type;
  }
  else
  {
    msg = MSG_HIT;
  }

  wpn_skill_num = required_weapon_skill(weapon);
  wpn_skill = (IS_PC(ch) || IS_AFFECTED(ch, AFF_CHARM)) ?
    GET_CHAR_SKILL(ch, wpn_skill_num) : MIN(100, GET_LEVEL(ch) * 2);
  // Thieves only get 1h slash for shortswords.
  if( wpn_skill_num == SKILL_1H_SLASHING && GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF)
    && weapon->value[0] != WEAPON_SHORTSWORD )
  {
    wpn_skill = 0;
  }
  to_hit = chance_to_hit(ch, victim, wpn_skill, weapon);

  diceroll = number(1, 100);

/* Making this linear.  might should be less than linear. - Lohrr
  int rollmod = 6; //statupdate2013 - drannak
  if (GET_C_INT(ch) < 90)
    rollmod = 7;
  else if (GET_C_INT(ch) > 140)
    rollmod = 4;
  //an increased change to critical hit if affected by rage
  // if (affected_by_spell(ch, SKILL_RAGE))
  //   diceroll -= (GET_CHAR_SKILL(ch, SKILL_RAGE) / 10);

  if (affected_by_spell(ch, SKILL_RAGE))
    rollmod -= 1;

  int critroll = (int) (GET_C_INT(ch) / rollmod);
*/
  // At 100 int : 5% crit, at 200 int : 25% crit
  int critroll = CRITRATE(ch);
  if(critroll >= number(1, 100))
  {
    sic = -1;
  }
  else if( diceroll < 96 )
  {
    sic = 0;
  }
  // Fumble
  else
  {
    sic = 1;
  }

  /* if (diceroll < 5) //crit
     sic = -1;
     else if (diceroll < 96) //fumble
     sic = 0;
     else
     sic = 1;*/

  if( (sic == -1) && has_innate(victim, INNATE_AMORPHOUS_BODY) )
  {
    act("You try to find a vital spot on your enemy, but $S body is too amorphous!",
        FALSE, ch, NULL, victim, TO_CHAR);
    act("$e tried to find a vital spot on you, but your body is too amorphous for that!",
        FALSE, ch, NULL, victim, TO_VICT);
    sic = 0;
  }

  wpn_skill = BOUNDED(GET_LEVEL(ch) / 2, wpn_skill, 95);

  // Crit to miss if they fail a weaponskill + luck check
  if( sic == -1 && number(30, 101) > wpn_skill + (GET_C_LUK(ch) / 4) )
  {
    sic = 0;
  }
  // Fumble to miss if they make a weaponskill + agi check.
  if( sic == 1 && number(1, 101) <= wpn_skill + (GET_C_AGI(ch) / 2) )
  {
    sic = 0;
  }

  // Quickstep stops crits if victim can respond
  bool bIsQuickStepMiss = FALSE;
  if( sic == -1 && !IS_AFFECTED2(victim, AFF2_STUNNED) && !IS_IMMOBILE(victim)
    && (notch_skill(victim, SKILL_QUICK_STEP, get_property("skill.notch.criticalAttack", 10))
    || GET_CHAR_SKILL(victim, SKILL_QUICK_STEP) > number(1, 100)) )
  {
    bool qs = FALSE;
    if( GET_POS(victim) == POS_STANDING )
    {
      act("$n attempts to powerfully strike you down, but you cunningly side step and escape the attack.", FALSE, ch, NULL, victim, TO_VICT);
      act("You lash out with a powerful strike, but $N cunningly sidesteps and escapes the attack.", FALSE, ch, NULL, victim, TO_CHAR);
      act("$N cunningly sidesteps &n's attack.", FALSE, ch, NULL, victim, TO_NOTVICT);

      qs = TRUE;
    }
    else if( GET_POS(victim) >= POS_KNEELING )
    {
      act("You tuck and roll away from the attack.", FALSE, ch, NULL, victim, TO_VICT);
      act("$N tucks and rolls away from your attack.", FALSE, ch, NULL, victim, TO_CHAR);
      act("$N tucks and rolls away from &n's attack.", FALSE, ch, NULL, victim, TO_NOTVICT);

      qs = TRUE;
    }

    if( qs )
    {
      if( ((GET_LEVEL(victim) / 5) + STAT_INDEX(GET_C_AGI(victim))) > number(0, 40) )
      {
        // need to know this, as quick-step crit misses won't fumble to ground
        bIsQuickStepMiss = TRUE;
        sic = 1;
      }
      else
        sic = 0;
    }
  }

  /* if (sic == -1 &&
     GET_CHAR_SKILL(ch, SKILL_SNEAKY_STRIKE) / 10 > number(0, 49) &&
     !is_preparing_for_sneaky_strike(ch))
     {
     sneaky_strike(ch, victim);
     sic = 0;
     }*/

  if( sic == 1 && !affected_by_spell(ch, SPELL_COMBAT_MIND) )
  {
    switch( number(1, 5) )
    {
      case 1:
        if( weapon && GET_LEVEL(ch) > 1 && !IS_SET(weapon->extra_flags, ITEM_NODROP) )
        {
          for( pos = 0; pos < MAX_WEAR; pos++ )
          {
            if( ch->equipment[pos] == weapon )
            {
              break;
            }
          }
          if( pos < MAX_WEAR )
          {
            P_obj weap = unequip_char(ch, pos);
            if( weap )
            {
              if( bIsQuickStepMiss )
              {
                act("&-L&+YYou swing at your foe _really_ badly, losing control of your&n $q&-L&+Y!\r\n", FALSE, ch, weap, victim, TO_CHAR);
                act("$n stumbles with $s attack, losing control of $s weapon!", TRUE, ch, 0, 0, TO_ROOM);
                obj_to_char(weap, ch);
              }
              else
              {
                act("&-L&+YYou swing at your foe _really_ badly, sending your&n $q&-L&+Y flying!\r\n", FALSE, ch, weap, victim, TO_CHAR);
                act("$n stumbles with $s attack, sending $s weapon flying!", TRUE, ch, 0, 0, TO_ROOM);
                obj_to_room(weap, ch->in_room);
              }
            }
            char_light(ch);
            room_light(ch->in_room, REAL);
          }
        }
        else
        {
          send_to_char("You stumble, but recover in time!\r\n", ch);
        }
        return FALSE;
      case 2:
        stop_fighting(ch);
        tch = get_random_char_in_room(ch->in_room, NULL, 0);
        if( tch != ch )
        {
          act("$n stumbles, and jabs at $N!", TRUE, ch, 0, tch, TO_NOTVICT);
          act("You stumble in your attack, and jab at $N!", TRUE, ch, 0, tch,  TO_CHAR);
          act("$n stumbles, and jabs at YOU!!", TRUE, ch, 0, tch, TO_VICT);
          return FALSE;
        }
        else
        {
          act("$n stumbles, hitting $mself!", TRUE, ch, 0, tch, TO_NOTVICT);
          act("You stumble in your attack, and hit yourself!", TRUE, ch, 0, tch, TO_CHAR);
          return FALSE;
        }
        break;
      default:
        break;
    }
  }

  if( IS_GRAPPLED(victim) )
  {
    gvict = grapple_attack_check(victim);
    if( gvict && (gvict != ch) )
    {
      chance = grapple_misfire_chance(ch, gvict, 0);
      if( number(1, 100) <= chance )
      {
        victim = gvict;
        act("$n's attack misses $s target and hits $N instead!", TRUE, ch, 0, victim, TO_NOTVICT);
        act("Your hold causes you to get in the way of $n's attack!", TRUE, ch, 0, victim, TO_VICT);
        act("Your attack misses your target and hits $N instead!", TRUE, ch, 0, victim, TO_CHAR);
      }
    }
  }

  if( !IS_FIGHTING(ch) && !IS_DESTROYING(ch) )
  {
    set_fighting(ch, victim);
  }

  /* The blind and eyeless cannot see "flashing lights." - Lucrot */
  if( IS_AFFECTED4(victim, AFF4_DAZZLER) && !IS_AFFECTED5(ch, AFF5_DAZZLEE)
    && !NewSaves(ch, SAVING_SPELL, 5) && !has_innate(ch, INNATE_EYELESS)
    && !IS_AFFECTED(ch, AFF_BLIND) && !IS_GREATER_RACE(victim) )
  {
    struct affected_type af;

    send_to_char("&+YSpa&+Wrk&+Ys&+L...  &=LWflashing lights&n&+L...  &nyou cannot concentrate!\r\n", ch);
    act("$n suddenly appears a bit confused!", TRUE, ch, 0, 0, TO_ROOM);
    act("You watch in glee as $N looks dazzled.", FALSE, victim, 0, ch, TO_CHAR);

    memset(&af, 0, sizeof(af));
    af.type = SPELL_DAZZLE;
    af.bitvector5 = AFF5_DAZZLEE;
    af.duration = PULSE_VIOLENCE * 3;
    af.flags = AFFTYPE_SHORT;
    affect_to_char_with_messages(ch, &af, "You no longer see spots.", NULL);
  }

  // Reflections/images never actually hit (even on a crit):
  if( IS_NPC(ch) && GET_VNUM(ch) == 250 )
  {
    to_hit = 0;
    sic = 0;
  }

  if( diceroll >= to_hit && sic != -1 )
  {
    act("$n misses $N.", FALSE, ch, NULL, victim, TO_NOTVICT | ACT_NOTTERSE);
    act("You miss $N.", FALSE, ch, NULL, victim, TO_CHAR | ACT_NOTTERSE);
    act("$n misses you.", FALSE, ch, NULL, victim, TO_VICT | ACT_NOTTERSE);
    // damage tier going to 0 due to miss.
    if (affected_by_spell(ch, SPELL_CEGILUNE_BLADE))
    {
      get_spell_from_char(ch, SPELL_CEGILUNE_BLADE)->modifier = 0;
    }
    remember(victim, ch);
    attack_back(ch, victim, TRUE);
    return FALSE;
  }

  /***** we managed to hit the opponent *****/
  if( weapon && (IS_SET(weapon->extra2_flags, ITEM2_BLESS)) && !number(0, 99) )
  {
    act("A &+Cblessed glow&n around your $q fades.", FALSE, ch, weapon, 0, TO_CHAR);
    affect_from_obj(weapon, SPELL_BLESS);
    REMOVE_BIT(weapon->extra2_flags, ITEM2_BLESS);
  }

  if( !weapon && affected_by_spell(ch, SPELL_VAMPIRIC_TOUCH)
    && !IS_UNDEADRACE(victim) && !IS_CONSTRUCT(victim) && !NewSaves(victim, SAVING_PARA, 0) )
  {
    act("You touch $N with your bare hands, draining $S life force.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n touches you with $s bare hands, draining your life force.", FALSE, ch, 0, victim, TO_VICT);
    damage(ch, victim, to_hit, SPELL_VAMPIRIC_TOUCH);
    affect_from_char(ch, SPELL_VAMPIRIC_TOUCH);
    vamp(ch, to_hit, GET_MAX_HIT(ch) * VAMPPERCENT(ch) );
    return FALSE;
  }

  if (sic == -1)
  {
    if( GET_SPEC(victim, CLASS_BERSERKER, SPEC_RAGELORD) && IS_AFFECTED2(victim, AFF2_FLURRY) )
    {
      send_to_char("&+rYour RaGe overwhelms you, making you blind to the enemies battering assaults.\n", victim);
      sic = 0;
    }
  }

  if (sic == -1)
  {
    if( GET_CHAR_SKILL(victim, SKILL_INDOMITABLE_RAGE) > number(50, 160) && affected_by_spell(victim, SKILL_BERSERK) )
    {
      act("$n&+W's critical hit sends you into a &+rpure &+Rberserk &+rtrance! &+RROARRRRRRR!&n\n", TRUE, ch, 0, victim, TO_VICT);
      act("$N lets out a fearsome &+RROAR&n!\n", TRUE, ch, 0, victim, TO_CHAR);
      sic = 0;

      bzero(&ir, sizeof(ir));
      ir.type = SKILL_INDOMITABLE_RAGE;
      ir.duration = 5;
      ir.flags = AFFTYPE_SHORT;
      //hitroll
      ir.modifier = 5;
      ir.location = APPLY_HITROLL;
      affect_to_char(ch, &ir);
      //damroll
      ir.modifier = 5;
      ir.location = APPLY_DAMROLL;
      affect_to_char(ch, &ir);
      //save fear
      ir.modifier = -5;
      ir.location = APPLY_SAVING_FEAR;
      affect_to_char(ch, &ir);
    }
  }

  if( (sic == -1) && (GET_CHAR_SKILL(ch, SKILL_DEVASTATING_CRITICAL) > devcrit) )
  {
    send_to_char("&=LWYou score a DEVASTATING HIT!!!!!&N\r\n", ch);
    make_bloodstain(ch);

  }
  else if( sic == -1 && (!GET_CLASS(ch, CLASS_MONK) || GET_LEVEL(ch) <= 50) )
  {
    send_to_char("&=LWYou score a CRITICAL HIT!!!!!&N\r\n", ch);

    if(!number(0, 9))
      make_bloodstain(victim);
  }

  if (sic == -1
    && ( notch_skill(ch, SKILL_CRITICAL_ATTACK, get_property("skill.notch.criticalAttack", 10))
    || (1 * GET_CHAR_SKILL(ch, SKILL_CRITICAL_ATTACK)) > number(1, 100)) )
  {
    critical_attack(ch, victim, msg);
  }

  if( has_innate(ch, INNATE_BATTLE_FRENZY) && !number(0, 20) && IS_HUMANOID(victim)
    && (battle_frenzy(ch, victim) != DAM_NONEDEAD) )
  {
    return FALSE;
  }

  if( GET_CHAR_SKILL(ch, SKILL_VICIOUS_ATTACK) > 0 && !affected_by_spell(ch, SKILL_WHIRLWIND)
    && !vicious_hit && GET_POS(ch) == POS_STANDING && GET_RACE(victim) != RACE_CONSTRUCT )
  {
    if( notch_skill(ch, SKILL_VICIOUS_ATTACK, get_property("skill.notch.offensive.auto", 4))
      || 0.1 * GET_CHAR_SKILL(ch, SKILL_VICIOUS_ATTACK) > number(0, 100) )
    {
      act("$n slips beneath $N's guard dealing a vicious attack!!", TRUE, ch, 0, victim, TO_NOTVICT);
      act("You slip beneath $N's guard dealing a vicious attack!", TRUE, ch, 0, victim, TO_CHAR);
      act("$n slips beneath your guard, dealing you a vicious attack!", TRUE, ch, 0, victim, TO_VICT);

      vicious_hit = TRUE;
      hit(ch, victim, weapon);
      vicious_hit = FALSE;
      if( !(is_char_in_room(ch, room) && is_char_in_room(victim, room)) )
      {
        return FALSE;
      }
    }
  }

  /* calculate the damage */

  if( IS_NPC(ch) )
  {
    dam = dice(ch->points.damnodice, ch->points.damsizedice);
  }
  else if( GET_CLASS(ch, CLASS_MONK) )
  {
    dam = MonkDamage( ch );
  }
  else if( GET_CLASS(ch, CLASS_PSIONICIST) && affected_by_spell(ch, SPELL_COMBAT_MIND) )
  {
    dam = dice(ch->points.damnodice, ch->points.damsizedice);
  }
  else
  {
    dam = number(1, 4);         /* 1d4 dam with bare hands */
    // Those with unarmed damage get an additional 4 damage with skill at 100.
    if(  GET_CHAR_SKILL(ch, SKILL_UNARMED_DAMAGE) )
    {
      dam += GET_CHAR_SKILL(ch, SKILL_UNARMED_DAMAGE) / 25;
    }
  }

  if( weapon )
  {
    if( IS_SET(weapon->extra_flags, ITEM_TWOHANDS) )
    {
      if( IS_PC(ch) || IS_PC_PET(ch) )
      {
        dam = dam_factor[DF_TWOHANDED_MODIFIER] * dice(weapon->value[1], weapon->value[2]);
      }
      else
      {
        dam += dam_factor[DF_TWOHANDED_MODIFIER] * dice(weapon->value[1], weapon->value[2]);
      }
    }
    else
    {
      if( IS_PC(ch) || IS_PC_PET(ch) )
      {
        dam = dice(weapon->value[1], weapon->value[2]);
      }
      else
      {
        dam += dice(weapon->value[1], weapon->value[2]);
      }
    }

    dam *= dam_factor[DF_WEAPON_DICE];
    /* Guess healing blade is out.
    if( IS_SWORD(weapon) && affected_by_spell(ch, SPELL_HEALING_BLADE) )
    {
      vamp(ch, number(1, 4), GET_MAX_HIT(ch));
    }
    */
  }

  tmp = TRUE_DAMROLL(ch) * dam_factor[DF_DAMROLL_MOD];
  // Randomize a bit by dropping between 0 and 10% of the damage incurred via damroll mods.
  tmp = (tmp * number(90,100))/100;
  dam += tmp;

  if( sic == -1 && GET_CHAR_SKILL(ch, SKILL_DEVASTATING_CRITICAL) > devcrit )
  {
    // Caps at 200% - 267% more damage
    dam = (int) (dam * (300 + GET_CHAR_SKILL(ch, SKILL_DEVASTATING_CRITICAL)) / 150 );
  }
  else if( sic == -1 )
  {
    dam = (int) (dam * 2.0);
  }

  // What's this diceroll < level-40 ?  Monks get additional crits?
  if( GET_CLASS(ch, CLASS_MONK) && GET_LEVEL(ch) > 30 && (sic == -1 || diceroll < GET_LEVEL(ch) - 40) )
  {
    if(sic != -1)
    {
      dam *= 1.5;
    }
    // Monk special critical hits don't work against a variety of victims.
    else if( IS_HUMANOID(victim) && !IS_UNDEADRACE(victim)
      && !IS_ANGEL(victim) && !IS_GREATER_RACE(victim) && !IS_ELITE(victim)
      && monk_critic(ch, victim) )
    {
      return FALSE;
    }
  }

  /* WTF is this BS?  Randomly drop damage by up to 70%?!?
   *   Commenting this out atm, although it does randomize damage a good bit.
   * It would be of better use in randomizing the damroll damage instead of both dice and damroll.
   *   And so, I'm utilizing a similar approach above in the damroll damage.
  // Weapon skill check, used to be offense.
  if( sic != -1 )
  {
    dam = (int) (dam * number(3, 10) / 10);
  }
  */

  if( has_divine_force(ch) )
  {
    dam *= get_property("damage.modifier.divineforce", 1.250);
  }

  /* Just commenting this out.. it's handled down below in minotaur_race_proc().
  // Dropped this to 30sec and made it not stack.
  if( (GET_RACE(victim) == RACE_MINOTAUR) && !number(0, 25) && !affected_by_spell(ch, TAG_MINOTAUR_RAGE) )
  {
    struct affected_type af;
    act("&+LAs $n&+L strikes you, the power of your &+rance&+Lstor&+rs&+L fill you with &+rR&+RAG&+RE&+L!&n", TRUE, ch, 0, victim, TO_VICT);
    act("&+LAs &n$n&+L strikes $N&+L, the power of &n$N's&+L &+rance&+Lstor&+rs&+L fill them with &+rR&+RAG&+RE&+L!&n", TRUE, ch, 0, victim, TO_NOTVICT);

    memset(&af, 0, sizeof(af));
    af.type = TAG_MINOTAUR_RAGE;
    af.location = APPLY_COMBAT_PULSE;
    af.modifier = -1;
    af.duration = 30;
    af.flags = AFFTYPE_SHORT;
    affect_to_char(victim, &af);

    memset(&af, 0, sizeof(af));
    af.type = TAG_MINOTAUR_RAGE;
    af.location = APPLY_SPELL_PULSE;
    af.modifier = -1;
    af.duration = 30;
    af.flags = AFFTYPE_SHORT;
    affect_to_char(victim, &af);
  }
  */

  // Replaced with innate intercept: intercept_defensiveproc below.
  if( has_innate( victim, INNATE_INTERCEPT ) && intercept_defensiveproc(victim, ch) )
  {
    // Not sure this is necesary, but if the attack is intercepted.. it shoiuld not complete, right?
    return FALSE;
  }

  if( has_innate(ch, INNATE_MELEE_MASTER) )
  {
    dam *= dam_factor[DF_MELEEMASTERY];
  }

  memset(&messages, 0, sizeof(struct damage_messages));
  messages.attacker = attacker_msg;
  messages.victim = victim_msg;
  messages.room = room_msg;
  messages.obj = weapon;

  if( vs_skill > 0 )
  {
    if( GET_CHAR_SKILL(ch, SKILL_ANATOMY) > 0 )
    {
      vs_skill += (int)(GET_CHAR_SKILL(ch, SKILL_ANATOMY) / 2);
    }

    if( IS_NPC(ch) && IS_ELITE(ch) )
      vs_skill += number(25, 100);
  }

  // PC 15% max per attack with 100 anatomy.
  if( vs_skill > 0
    && (notch_skill(ch, SKILL_VICIOUS_STRIKE, get_property("skill.notch.offensive.vicious.strike", 17))
    || vs_skill > number(1, 1000)) )
  {
    if( IS_PC(ch) )
    {
      dam += get_property("damage.modifier.vicious.strike", 1.050) * GET_CHAR_SKILL(ch, SKILL_VICIOUS_STRIKE);
    }
    else if(IS_ELITE(ch))
    {
      dam += (int) GET_LEVEL(ch) * 1.5;
    }
    else if(IS_GREATER_RACE(ch))
    {
      dam += GET_LEVEL(ch) * 1.25;
    }
    else if(IS_NPC(ch))
    {
      dam += GET_LEVEL(ch);
    }

    snprintf(attacker_msg, MAX_STRING_LENGTH, "You feel a powerful rush of &+rAnG&+RE&+rr&n as your%%s %s %%s.", attack_hit_text[msg].singular);
    snprintf(victim_msg, MAX_STRING_LENGTH, "A sense of &+RWi&+rLD H&+RAt&+rE &nsurrounds $n as $s%%s %s %%s.", attack_hit_text[msg].singular);
    snprintf(room_msg, MAX_STRING_LENGTH, "A sense of &+RWi&+rLD H&+RAt&+rE &nsurrounds $n as $s%%s %s %%s.", attack_hit_text[msg].singular);
    messages.type = DAMMSG_HIT_EFFECT;
  }
  else if (notch_skill(victim, SKILL_BOILING_BLOOD, get_property("skill.notch.defensive", 17))
    || GET_CHAR_SKILL(victim, SKILL_BOILING_BLOOD) / 10 > number(1, 100) )
  {
    snprintf(attacker_msg, MAX_STRING_LENGTH, "$N is so overcome with bloodlust, your %s barely grazes $M!", attack_hit_text[msg].singular);
    snprintf(victim_msg, MAX_STRING_LENGTH, "You are so overcome with bloodlust, $n's %s barely grazes you!", attack_hit_text[msg].singular);
    snprintf(room_msg, MAX_STRING_LENGTH, "$N is so overcome with bloodlust, $n's %s barely grazes $M!", attack_hit_text[msg].singular);
    dam = 1;
  }
  else if( get_linked_char(ch, LNK_FLANKING) == victim )
  {
    snprintf(attacker_msg, MAX_STRING_LENGTH, "You %%s as your%%s %s reaches $S unprotected flank.", attack_hit_text[msg].singular);
    snprintf(victim_msg, MAX_STRING_LENGTH, "$n %%s as $s%%s %s reaches your unprotected flank.", attack_hit_text[msg].singular);
    snprintf(room_msg, MAX_STRING_LENGTH, "$n %%s as $s%%s %s reaches $S unprotected flank.", attack_hit_text[msg].singular);
    messages.type = DAMMSG_EFFECT_HIT;
  }
  else if (get_linked_char(ch, LNK_CIRCLING) == victim)
  {
    snprintf(attacker_msg, MAX_STRING_LENGTH, "$N screams in pain as your %s tears into $S flesh", attack_hit_text[msg].singular);
    snprintf(victim_msg, MAX_STRING_LENGTH, "You scream in pain as $n's %s tears into your flesh.", attack_hit_text[msg].singular);
    snprintf(room_msg, MAX_STRING_LENGTH, "$N screams in pain as $n's %s tears into $S flesh.", attack_hit_text[msg].singular);
    messages.type = DAMMSG_EFFECT_HIT;
  }
  /* Set property skill.anatomy.ratio to determine how often to check anatomy_strike().
   *  100 skill is approximately 4 percent for players and 5% for 
   *  elite mobs. Apr09 -Lucrot
   */
  // 5% for elite mobs.
  else if( IS_NPC(ch) && IS_ELITE(ch) && get_property("skill.anatomy.NPC", 5.000) <= number(1, 100)
    && IS_HUMANOID(victim) && IS_HUMANOID(ch) )
  {
    dam = anatomy_strike(ch, victim, msg, &messages, (int) dam);
  }
  else if( GET_CHAR_SKILL(ch, SKILL_ANATOMY)
    && IS_HUMANOID(victim) && IS_HUMANOID(ch)
    && GET_CHAR_SKILL(ch, SKILL_ANATOMY) / 25 >= (number(1, 100)) )
  {
    dam = anatomy_strike(ch, victim, msg, &messages, (int) dam);
  }
  else
  {
    snprintf(attacker_msg, MAX_STRING_LENGTH, "Your%%s %s %%s.", attack_hit_text[msg].singular);
    snprintf(victim_msg, MAX_STRING_LENGTH, "$n's%%s %s %%s.", attack_hit_text[msg].singular);
    snprintf(room_msg, MAX_STRING_LENGTH, "$n's%%s %s %%s.", attack_hit_text[msg].singular);
    messages.type = DAMMSG_HIT_EFFECT | DAMMSG_TERSE;
  }

  dam *= ch->specials.damage_mod;

  if( GET_RACE(ch) == RACE_ORC )
  {
    dam = orc_horde_dam_modifier(ch, dam, TRUE);
  }
  else if( GET_RACE(victim) == RACE_ORC )
  {
    dam = orc_horde_dam_modifier(victim, dam, FALSE);
  }

  if( weapon && IS_SLAYING(weapon, victim) )
  {
    dam *= get_property("damage.modifier.slaying", 1.050);
  }

  dam = BOUNDED(1, (int) dam, 32766);

  if( has_innate(victim, INNATE_WEAPON_IMMUNITY) )
  {
    if( weapon )
    {
      if (!IS_SET(weapon->extra2_flags, ITEM2_MAGIC))
      {
        dam = 1;
      }
    }
    else if( GET_LEVEL(ch) < 51 && (!ch->equipment[WEAR_HANDS]
      || !IS_SET(ch->equipment[WEAR_HANDS]->extra2_flags, ITEM2_MAGIC)) )
    {
      dam = 1;
    }
  }

  if( weapon && IS_PC(ch) && ilogb(dam) > number(5, 400) )
  {
    DamageOneItem(ch, 1, weapon, FALSE);
  }

  tmp = melee_death_messages_table[2 * msg + 1].attacker ? number(0, 1) : 0;
  messages.death_attacker = melee_death_messages_table[2 * msg + tmp].attacker;
  messages.death_victim = melee_death_messages_table[2 * msg + tmp].victim;
  messages.death_room = melee_death_messages_table[2 * msg + tmp].room;

  //!!!
  // | RAWDAM_NOEXP,   // hitting yields normal exp -Odorf &messages)
  if( melee_damage(ch, victim, dam, (msg == MSG_HIT ? PHSDAM_TOUCH : PHSDAM_HELLFIRE | PHSDAM_BATTLETIDE), &messages) != DAM_NONEDEAD )
  {
    return TRUE;
  }

  if( IS_NPC(victim) && (GET_POS(victim) < POS_STANDING) )
  {
    do_alert(victim, 0, 0);
    do_stand(victim, 0, 0);
  }

  if( reaver_hit_proc(ch, victim, weapon) )
  {
    return TRUE;
  }

  if( affected_by_spell(ch, SPELL_DREAD_BLADE) && dread_blade_proc(ch, victim) )
  {
    return TRUE;
  }

  if( GET_CLASS(ch, CLASS_PALADIN) && holy_weapon_proc(ch, victim) )
  {
    return TRUE;
  }

  if( GET_RACE(ch) == RACE_MINOTAUR )
  {
    minotaur_race_proc(ch, victim);
  }

  if( affected_by_spell(ch, ACH_YOUSTRAHDME) && IS_UNDEADRACE(victim)
    && lightbringer_proc(ch, victim, TRUE) )
  {
    return TRUE;
  }

  blade_skill = GET_CLASS(ch, CLASS_AVENGER) ? SKILL_HOLY_BLADE : SKILL_TAINTED_BLADE;
  if( notch_skill(ch, blade_skill, get_property("skill.notch.offensive.auto", 4))
    || (GET_CHAR_SKILL(ch, blade_skill) >= number(1, 100) && !number(0, 19)) )
  {
    if( tainted_blade(ch, victim) )
    {
      return TRUE;
    }
  }

  if( weapon && weapon->value[4] != 0 )
  {
    if( IS_POISON(weapon->value[4]) )
    {
      (skills[weapon->value[4]].spell_pointer) (10, ch, 0, 0, victim, 0);
    }
    else
    {
      poison_lifeleak(10, ch, 0, 0, victim, 0);
    }
    weapon->value[4] = 0;       /* remove on success */
  }

  if( weapon && is_char_in_room(ch, room) && is_char_in_room(victim, room) && IS_ALIVE(victim)
    && (!IS_PC_PET( ch ) || OBJ_VNUM(weapon) != 1251) )
  {
    weapon_proc(weapon, ch, victim);
  }

  return TRUE;
}

bool weapon_proc(P_obj obj, P_char ch, P_char victim)
{
  struct extra_descr_data *ex;
  int      spells[3];
  int      room;
  int      count;

  if( !obj->value[5] || obj->value[7] <= 0 )
  {
    if (obj_index[obj->R_num].func.obj != NULL)
    {
      return (*obj_index[obj->R_num].func.obj) (obj, ch, CMD_MELEE_HIT, (char *) victim);
    }
    else
    {
      return FALSE;
    }
  }

  //int test = number(0, obj->value[7] - 1);
  //debug( "Val7: %d, Val7-1: %d, Test: %d", obj->value[7], obj->value[7]-1, test );
  if( number(0, obj->value[7] - 1) )
  {
    return FALSE;
  }

  for( ex = obj->ex_description; ex; ex = ex->next )
  {
    if (isname("_char_msg", ex->keyword))
    {
      act(ex->description, FALSE, ch, obj, victim, TO_CHAR | ACT_NOEOL);
    }
    else if (isname("_victim_msg", ex->keyword))
    {
      act(ex->description, FALSE, ch, obj, victim, TO_VICT | ACT_NOEOL);
    }
    else if (isname("_room_msg", ex->keyword))
    {
      act(ex->description, FALSE, ch, obj, victim, TO_NOTVICT | ACT_NOEOL);
    }
  }

  count = 0;
  room = ch->in_room;
  if (spells[0] = obj->value[5] % 1000)
  {
    count++;
  }
  if (spells[1] = obj->value[5] % 1000000 / 1000)
  {
    count++;
  }
  if (spells[2] = obj->value[5] % 1000000000 / 1000000)
  {
    count++;
  }

  if( !count )
  {
    return FALSE;
  }

  if( obj->value[5] > 999999999 )
  {
    count = number(0, count - 1);
    if( skills[spells[count]].spell_pointer )
    {
      if( IS_AGG_SPELL(spells[count]) )
      {
        ((*skills[spells[count]].spell_pointer) ((int) obj->value[6], ch, 0, SPELL_TYPE_SPELL, victim, obj));
      }
      else if( !affected_by_spell(ch, spells[count]) )
      {
        ((*skills[spells[count]].spell_pointer) ((int) obj->value[6], ch, 0, SPELL_TYPE_SPELL, ch, obj));
      }
    }
  }
  else
  {
    while( count-- && is_char_in_room(ch, room) && is_char_in_room(victim, room) )
    {
      if( skills[spells[count]].spell_pointer )
      {
        if( IS_AGG_SPELL(spells[count]) )
        {
          ((*skills[spells[count]].spell_pointer) ((int) obj->value[6], ch, 0, SPELL_TYPE_SPELL, victim, obj));
        }
        else if( !affected_by_spell(ch, spells[count]) )
        {
          ((*skills[spells[count]].spell_pointer) ((int) obj->value[6], ch, 0, SPELL_TYPE_SPELL, ch, obj));
        }
      }
    }
  }
  return TRUE;
}

void StopAllAttackers(P_char ch)
{
  P_char   t_ch, hold;

  if (!ch)
    return;

  for (t_ch = combat_list; t_ch; t_ch = hold)
  {
    hold = t_ch->specials.next_fighting;
    if (GET_OPPONENT(t_ch) == ch)
    {
      t_ch->specials.was_fighting = ch;
      stop_fighting(t_ch);
    }
  }
}

void StopMercifulAttackers(P_char ch)
{
  P_char   t_ch, hold;

  if (!ch)
    return;

  for (t_ch = combat_list; t_ch; t_ch = hold)
  {
    hold = t_ch->specials.next_fighting;
    if ((GET_OPPONENT(t_ch) == ch) &&
        !affected_by_spell(t_ch, SKILL_BERSERK) &&
        ((IS_PC(t_ch) && !IS_SET(t_ch->specials.act, PLR_VICIOUS)) ||
         (IS_NPC(t_ch) && !is_aggr_to(t_ch, ch))))
    {
      if (GET_CLASS(t_ch, CLASS_PALADIN) && ch->specials.alignment < -349)
        continue;
      stop_fighting(t_ch);
    }
  }
}

/* start one char fighting another (yes, it is horrible, I know... ) */
void set_fighting(P_char ch, P_char vict)
{
  P_char   victim = vict;
  P_char   tch;
  char     Gbuf[10];

  if( (ch == victim) || !SanityCheck(ch, "set_fighting - ch")
    || !SanityCheck(victim, "set_fighting - victim") )
  {
    return;
  }

  if( IS_FIGHTING(ch) || ch->specials.next_fighting )
  {
    logit(LOG_EXIT, "assert: set_fighting() when already fighting");
    raise(SIGSEGV);
  }
  if( IS_DESTROYING(ch) )
  {
    logit(LOG_EXIT, "assert: set_fighting() when destroying object");
    raise(SIGSEGV);
  }

  /*
   * new reality mode, unless they are adjacent in a SINGLE_FILE room,
   * they can't hit each other, except with spells/breath, so they don't
   * start fighting either.  Make sure this check is first, as it can
   * change the intended victim. JAB
   */

  if ((IS_PC(ch) || IS_PC_PET(ch)) &&   // IS_PC(vict) &&
      (!on_front_line(ch) || !on_front_line(vict)) &&
      !(ch->equipment[PRIMARY_WEAPON] &&
        IS_REACH_WEAPON(ch->equipment[PRIMARY_WEAPON])))
    return;

  if( IS_ROOM(ch->in_room, ROOM_SINGLE_FILE) && !AdjacentInRoom(ch, victim) )
  {
    if (IS_PC(ch) || !(victim = PickTarget(ch)))
      return;
  }

  if (!can_hit_target(ch, victim))
  {
    send_to_char("You can't seem to find room!\r\n", ch);
    return;
  }

  if(IS_IMMOBILE(ch) || !IS_AWAKE(ch))
  {
    return;
  }

  if( IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_AGGIMMUNE))
    return;

  if( IS_TRUSTED(victim) && IS_SET(victim->specials.act, PLR_AGGIMMUNE))
  {
    if (IS_FIGHTING(victim))
      stop_fighting(victim);
    return;
  }
  if (affected_by_spell(ch, SONG_CHARMING) &&
      ((ch->following && (ch->following == victim)) ||
       (victim->following && (victim->following == ch))))
    affect_from_char(ch, SONG_CHARMING);

  if (affected_by_spell(ch, SONG_SLEEP))
    affect_from_char(ch, SONG_SLEEP);

  if (affected_by_spell(ch, SPELL_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  if (IS_AFFECTED(ch, AFF_SLEEP))
    REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);

  if (IS_AFFECTED(ch, AFF_SNEAK))
  {
    if (affected_by_spell(ch, SKILL_SNEAK))
      affect_from_char(ch, SKILL_SNEAK);
    if (IS_AFFECTED(ch, AFF_SNEAK))
      REMOVE_BIT(ch->specials.affected_by, AFF_SNEAK);
  }
  if (IS_AFFECTED(ch, AFF_HIDE))
  {
    act("$n comes out of hiding!", TRUE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
  }

  appear(ch);

  if (IS_AFFECTED(victim, AFF_MEDITATE))
  {
    act("$n is disrupted from meditation.", TRUE, victim, 0, 0, TO_ROOM);
    REMOVE_BIT(victim->specials.affected_by, AFF_MEDITATE);
  }

  if (affected_by_spell(ch, SPELL_CEGILUNE_BLADE)) {
    struct affected_type *afp = get_spell_from_char(ch, SPELL_CEGILUNE_BLADE);
    afp->modifier = 0;
  }

  /*
   * paging mode was doing weird things when you got attacked, so nuke
   * it (for victim) when attacked.  JAB
   */
  if (victim->desc && victim->desc->showstr_vector)
  {
    strcpy(Gbuf, "q\n");
    show_string(victim->desc, Gbuf);
  }
  GET_OPPONENT(ch) = victim;
  ch->specials.next_fighting = combat_list;
  combat_list = ch;
  stop_memorizing(ch);
  stop_memorizing(victim);

  /* call for initial 'dragon fear' check.  -JAB */

  if( IS_NPC(ch) && (IS_DRAGON(ch) || IS_AVATAR(ch)) && !IS_MORPH(ch) && !IS_PC_PET(ch) )
  {
    if(!number(0, 2)) // Attack and roar.
    {
      DragonCombat(ch, TRUE);
    }
    else
    {
      DragonCombat(ch, FALSE); // Attack but don't roar.
    }
  }

  /* Code for memory */

  if (ch && victim)
  {
    if (HAS_MEMORY(victim))
    {
      if (IS_PC(ch))
      {
        if (!(IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_AGGIMMUNE)))
          if (GET_STAT(victim) > STAT_INCAP)
            remember(victim, ch);
      }
      else if (IS_PC_PET(ch) && (GET_MASTER(ch)->in_room == ch->in_room) &&
          CAN_SEE(victim, GET_MASTER(ch)))
      {
        if (!
            (IS_TRUSTED(GET_MASTER(ch)) &&
             IS_SET(GET_MASTER(ch)->specials.act, PLR_AGGIMMUNE)))
          if (GET_STAT(victim) > STAT_INCAP)
            remember(victim, GET_MASTER(ch));
      }
    }
  }
  /*
   * used to change position directly to POSITION_FIGHTING, causing some
   * strange things.  This is better.  JAB
   */

  if (GET_STAT(ch) == STAT_SLEEPING)
  {
    send_to_char("You are VERY rudely awakened!\r\n", ch);
    act("$n has a RUDE awakening!", TRUE, ch, 0, 0, TO_ROOM);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    do_wake(ch, 0, -4);
  }
  if (P_char mount = get_linked_char(ch, LNK_RIDING))
  {
    if (!GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT)/* && !is_natural_mount(ch, mount)*/)
    {
      send_to_char("I'm afraid you aren't quite up to mounted combat.\r\n", ch);
      act("$n quickly slides off $N's back.", TRUE, ch, 0, mount, TO_NOTVICT);
      stop_riding(ch);
    }
  }

  if (has_innate(ch, INNATE_ANTI_EVIL) && IS_EVIL(vict))
    do_innate_anti_evil(ch, vict);
  if (has_innate(ch, INNATE_DECREPIFY))
    do_innate_decrepify(ch,vict);
  if (GET_CHAR_SKILL(ch, SKILL_DREAD_WRATH))
    do_dread_wrath(ch,vict);
}

// Attack that object!
void set_destroying(P_char ch, P_obj obj)
{
  P_char   tch;
  char     Gbuf[10];

  if( !SanityCheck(ch, "set_destroying - ch") )
    return;

  if( IS_DESTROYING(ch) || ch->specials.next_destroying )
  {
    logit(LOG_EXIT, "assert: set_fighting() when already destroying object");
    raise(SIGSEGV);
  }
  if( IS_FIGHTING(ch) )
  {
    logit(LOG_EXIT, "assert: set_fighting() when fighting");
    raise(SIGSEGV);
  }

  if(IS_IMMOBILE(ch) || !IS_AWAKE(ch))
    return;

  if( IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_AGGIMMUNE) )
    return;

  if (affected_by_spell(ch, SONG_SLEEP))
    affect_from_char(ch, SONG_SLEEP);

  if (affected_by_spell(ch, SPELL_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  if (IS_AFFECTED(ch, AFF_SLEEP))
    REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);

  if (IS_AFFECTED(ch, AFF_SNEAK))
  {
    if (affected_by_spell(ch, SKILL_SNEAK))
      affect_from_char(ch, SKILL_SNEAK);
    if (IS_AFFECTED(ch, AFF_SNEAK))
      REMOVE_BIT(ch->specials.affected_by, AFF_SNEAK);
  }
  if (IS_AFFECTED(ch, AFF_HIDE))
  {
    act("$n comes out of hiding!", TRUE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
  }

  appear(ch);


  if (affected_by_spell(ch, SPELL_CEGILUNE_BLADE)) {
    struct affected_type *afp = get_spell_from_char(ch, SPELL_CEGILUNE_BLADE);
    afp->modifier = 0;
  }

  ch->specials.destroying_obj = obj;
  ch->specials.next_destroying = destroying_list;
  destroying_list = ch;
  stop_memorizing(ch);

  if (GET_STAT(ch) == STAT_SLEEPING)
  {
    send_to_char("You are VERY rudely awakened!\r\n", ch);
    act("$n has a RUDE awakening!", TRUE, ch, 0, 0, TO_ROOM);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    do_wake(ch, 0, -4);
  }
  if (P_char mount = get_linked_char(ch, LNK_RIDING))
  {
    if (!GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT)/* && !is_natural_mount(ch, mount)*/)
    {
      send_to_char("I'm afraid you aren't quite up to mounted combat.\r\n", ch);
      act("$n quickly slides off $N's back.", TRUE, ch, 0, mount, TO_NOTVICT);
      stop_riding(ch);
    }
  }
}

void MoveAllAttackers(P_char ch,P_char v)
{
  P_char   t_ch, hold;

  if (!ch || !v)
    return;

  for (t_ch = combat_list; t_ch; t_ch = hold)
  {
    hold = t_ch->specials.next_fighting;
    if (GET_OPPONENT(t_ch) == ch)
    {
      t_ch->specials.was_fighting = ch;
      stop_fighting(t_ch);
      set_fighting(t_ch,v);
    }
  }
}


/* picks a new random target for mayhem */

void retarget_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char   target;
  char     buf[255];

  if (!ch) {
    return;
  }

  /* no need to retarget when target _Does_ exist */
  if (GET_OPPONENT(ch))
    return;

  /* retarget blues: */
  if ((ch->in_room < 0) /* || (ch->in_room > 65536) */ )
    return;
  if (!MIN_POS(ch, POS_STANDING + STAT_RESTING))
    return;
  target = PickTarget(ch);
  if (!target)
    return;
  if (IS_NPC(ch))
    MobStartFight(ch, target);
  else
  {
    snprintf(buf, 255, "hit %s", GET_NAME(target));
    command_interpreter(ch, buf);
  }
}



int WeaponSkill(P_char ch, P_obj weapon)
{
  if(IS_NPC(ch))
    return BOUNDED(0, (GET_LEVEL(ch) * 7 / 3), 98);
  else
    return GET_CHAR_SKILL(ch, required_weapon_skill(weapon));
}


int leapSucceed(P_char victim, P_char attacker)
{
  int chance;

  chance = 0;
  if(!IS_THRIKREEN(victim) &&
      !IS_HARPY(victim))       /* only bugs */
    return false;

  if(!MIN_POS(victim, POS_STANDING + STAT_NORMAL) ||
      affected_by_spell(victim, SKILL_GAZE))
    return false;

  chance = (GET_C_AGI(victim) / 7);
  chance -= load_modifier(victim) / 100;
  chance += (GET_LEVEL(victim) - GET_LEVEL(attacker)) / 2;
  chance = BOUNDED(1, chance, 20);

  if(number(1, 100) > chance)
    return false;

  if (IS_THRIKREEN(victim))
  {
    act("You leap into the air, avoiding $n's attack.", FALSE, attacker, 0,
        victim, TO_VICT);
    act("$N leaps into the air, avoiding your attack.", FALSE, attacker, 0,
        victim, TO_CHAR);
    act("$N leaps over $n's attack.", FALSE, attacker, 0, victim, TO_NOTVICT);
  }
  else
  {
    act("You move with lightning speed, quickly evading $n's attack.", FALSE,
        attacker, 0, victim, TO_VICT);
    act("$N moves with lightning speed, evading your attack.", FALSE,
        attacker, 0, victim, TO_CHAR);
    act("$N moves with lightning speed, evading $n's attack.", FALSE,
        attacker, 0, victim, TO_NOTVICT);
  }
  return true;
}

/*
 * This function does all the necessary details for implementation 
 * of skill "dodging".  Details include print out messages.
 * Return 1 if successful dodge, or 0 otherwise.
 */

int dodgeSucceed(P_char char_dodger, P_char attacker, P_obj wpn)
{
  P_char   mount;
  int percent = 0, learned = 0, minimum = 0;

  if(!(char_dodger) || !(attacker) || IS_IMMOBILE(char_dodger))
  {
    return 0;
  }

  mount = get_linked_char(char_dodger, LNK_RIDING);

  if(mount &&
      MIN_POS(mount, POS_STANDING + STAT_NORMAL) &&
      MIN_POS(char_dodger, POS_STANDING + STAT_NORMAL) &&
      !IS_STUNNED(mount) &&
      !IS_BLIND(mount) &&
      !IS_STUNNED(char_dodger) &&
      !IS_BLIND(char_dodger) &&
      (notch_skill(char_dodger, SKILL_SIDESTEP, get_property("skill.notch.defensive", 17)) ||
       GET_CHAR_SKILL(char_dodger, SKILL_SIDESTEP) / 5 > number(0, 100)))
  {
    act("Your mount sidesteps $n's blow.", FALSE, attacker, 0, char_dodger,
        TO_VICT | ACT_NOTTERSE);
    act("$N sidesteps your attack causing you to miss.", FALSE, attacker, 0,
        mount, TO_CHAR | ACT_NOTTERSE);
    act("$N's mount sidesteps $n's blow.", FALSE, attacker, 0, char_dodger,
        TO_NOTVICT | ACT_NOTTERSE);
    return 1;
  }

  // Cannot dodge while on the ground except with groundfighting.
  if( (GET_POS(char_dodger) < POS_STANDING) && !GROUNDFIGHTING_CHECK(char_dodger) )
  {
    return 0;
  }

  if( affected_by_spell(char_dodger, SKILL_RAGE) && attacker != GET_OPPONENT(char_dodger) )
  {
    return 0;
  }

  //Notching dodge fails dodge check.
  /* -Changing dodge to an innate skill, with c_agility as basis for check - Drannak 12/12/2012
     if(notch_skill
     (char_dodger, SKILL_DODGE, get_property("skill.notch.defensive", 17)))
     {
     return 0;
     }
     */
  //Generating base dodge value.
  /*
     learned = (int) ((GET_CHAR_SKILL(char_dodger, SKILL_DODGE)) * 1.25) -
     (WeaponSkill(attacker, wpn));
     */
  learned = (int) ((GET_C_AGI(char_dodger)) * dam_factor[DF_DODGE_AGI_MODIFIER]) - (WeaponSkill(attacker, wpn));

  //Dwarves now get the DnD 3.5 dodgeroll bonus vs giant races
  if( has_innate( char_dodger, INNATE_GIANT_AVOIDANCE )
    && ((GET_RACE(attacker) == RACE_TROLL) || (GET_RACE(attacker) == RACE_OGRE) || (GET_RACE(attacker) == RACE_GIANT)))
  {
    learned *= 1.10;
  }

  // Everybody receives these values. -maybe change this? Drannak
  learned +=  (int) (((STAT_INDEX(GET_C_AGI(char_dodger))) -
        (STAT_INDEX(GET_C_DEX(attacker)))) / 2);

  // Simple level comparison adjustment.
  learned += (int) (GET_LEVEL(char_dodger) - GET_LEVEL(attacker));

  // NPCs receive a dodge bonus.
  /* Nah - Drannak
     if(IS_NPC(char_dodger))
     {
     learned += (int) (GET_LEVEL(char_dodger) / 2);
     }
     */
  // Minimum dodge is 1/10th of the skill.
  /*
     minimum = (int) (GET_CHAR_SKILL(char_dodger, SKILL_DODGE) / 10);
     */
  minimum = (int) (GET_C_AGI(char_dodger) / 10);

  percent = BOUNDED( minimum, learned, 50);

  // Modifiers
  if( (has_innate(char_dodger, INNATE_GROUNDFIGHTING) && !MIN_POS(char_dodger, POS_STANDING + STAT_NORMAL))
    || affected_by_spell(char_dodger, SKILL_GAZE) )
  {
    percent = (int) (percent * 0.50);
  }

  // 1/5 the chance if dazzled (typical 2% max 10%).
  if(IS_AFFECTED5(char_dodger, AFF5_DAZZLEE) )
  {
    percent /= 5;
  }

  // 6% of 50% is 3% chance to dodge stunned (6% of 10% is 0%).
  if( IS_STUNNED(char_dodger) )
  {
    percent = (6 * percent) / 100;
  }


  if(GET_CLASS(char_dodger, CLASS_MONK))
  {
    percent = (int) (percent * 1.20);
  }

  // Improved Zealot/Scoundrel dodge.
  if(GET_SPEC(char_dodger, CLASS_CLERIC, SPEC_ZEALOT)
    || GET_SPEC(char_dodger, CLASS_BARD, SPEC_SCOUNDREL))
  {
    percent += 5;
  }


  if(IS_STUNNED(attacker))
  {
    percent = (int) (percent * 1.10);
  }

  // Weight affects dodge. This simulates mobility.
  // Harder to dodge when char_dodger's weight is increased.
  // Easier to dodge when attacker's weight is increased.
  // Tweak as needed.
  /*
     percent -= (int) (load_modifier(char_dodger) / 100);
     percent += (int) (load_modifier(attacker) / 100);
     */

  // Drows receive special dodge bonus now based on level.
  // Level 56 drow has 10% innate dodge.  Excessive weight will negate this bonus.
  // This is actually a 9% dodge for all !stunned drow within weight limit..
  //   I set the weight limit to 100 since the range for load_modifier is between 75 and 300.
  if( GET_RACE(char_dodger) == RACE_DROW && !IS_STUNNED(char_dodger)
    && (load_modifier(char_dodger) < 100) && !number(0, 10))
  {
    //debug("Drow dodge (%s).", GET_NAME(char_dodger));
  }
  else if(affected_by_spell(char_dodger, SKILL_BATTLE_SENSES))
  {
    //debug("dodging (%s) affected by battle senses.", GET_NAME(char_dodger));
  }
  else if(number(1, 101) > percent)   // Dodge success or failure.
  {
    return 0; // Failed dodge.
  }

  // Dodge success.
  if(GET_CLASS(char_dodger, CLASS_MONK))
  {
    if(number(0, 1))
    {
      act("You easily lean out of range of $n's vicious attack.", FALSE, attacker, 0,
          char_dodger, TO_VICT | ACT_NOTTERSE);
      act("$N nimbly swivels out of the path of your attack.", FALSE,
          attacker, 0, char_dodger, TO_CHAR | ACT_NOTTERSE);
      act("$N nimbly swivels out of the path of $n's attack.", FALSE,
          attacker, 0, char_dodger, TO_NOTVICT | ACT_NOTTERSE);
    }
    else
    {
      act("You whirl around, just avoiding $n's vicious attack.", FALSE,
          attacker, 0, char_dodger, TO_VICT | ACT_NOTTERSE);
      act("$N gracefully whirls around your attack.", FALSE, attacker, 0,
          char_dodger, TO_CHAR | ACT_NOTTERSE);
      act("$N gracefully whirls around $n's attack.", FALSE, attacker, 0,
          char_dodger, TO_NOTVICT | ACT_NOTTERSE);
    }
  }
  else
  {
    act("You dodge $n's vicious attack.", FALSE, attacker, 0, char_dodger,
        TO_VICT | ACT_NOTTERSE);
    act("$N dodges your futile attack.", FALSE, attacker, 0, char_dodger,
        TO_CHAR | ACT_NOTTERSE);
    act("$N dodges $n's attack.", FALSE, attacker, 0, char_dodger,
        TO_NOTVICT | ACT_NOTTERSE);
  }
  return 1;
}

int blockSucceed(P_char victim, P_char attacker, P_obj wpn)
{
  int learned, attackerweaponskill, percent;
  int room = victim->in_room;
  P_obj shield;

  if(!(attacker) ||
      !(victim) ||
      !IS_ALIVE(victim) ||
      !IS_ALIVE(attacker))
  {
    return false;
  }

  if(!GET_CHAR_SKILL(victim, SKILL_SHIELD_BLOCK) ||
      !(shield = victim->equipment[WEAR_SHIELD]))
    return false;

  if (affected_by_spell(victim, SKILL_RAGE) && attacker != GET_OPPONENT(victim))
    return false;

  if(notch_skill(victim, SKILL_SHIELD_BLOCK, get_property("skill.notch.defensive", 17)))
    return false;

  learned = GET_CHAR_SKILL(victim, SKILL_SHIELD_BLOCK) / 4;
  learned += dex_app[STAT_INDEX(GET_C_DEX(victim))].reaction * 2;

  if(GET_CLASS(attacker, CLASS_MONK))
    learned = (int) (learned * 1.75);

  // Shield block works well versus an attacker that is using a whip/flail.
  if((wpn && IS_FLAYING(wpn))
    || (attacker->equipment[WIELD] && IS_FLAYING(attacker->equipment[WIELD])))
    learned = (int) (learned * 1.5);

  if(IS_AFFECTED5(victim, AFF5_DAZZLEE))
    learned -= 10;
  // Victim benefits from having a shield and not standing.
  // This is intentional as parry and dodge values are
  // greatly reduced when victim is not standing. The victim
  // focuses their defense with the shield, if present.
  if(!MIN_POS(victim, POS_STANDING + STAT_NORMAL))
    learned = (int) (learned * 1.2);

  if(!MIN_POS(attacker, POS_STANDING + STAT_NORMAL))
    percent = (int) (learned * 1.1);

  if(GET_C_LUK(victim) / 10 > number (0, 100))
    percent += number(1, 4);

  /* Bless spell modifier can be adjusted on the fly - Lucrot */
  if(IS_SET(shield->extra2_flags, ITEM2_BLESS))
    percent = (int) (percent * get_property("skill.shieldBlock.blessBonus", 1.2));

  attackerweaponskill = WeaponSkill(attacker, wpn);
  attackerweaponskill += str_app[STAT_INDEX(GET_C_STR(attacker))].tohit * 4;

  /* Standardizing values to a 1 to 100 scale. May need tweaking. -Lucrot */

  if(number(0, attackerweaponskill) > number(0, learned))
  {
    return false;
  }

  if(affected_by_spell(victim, SPELL_STORMSHIELD) &&
      !number(0, 2))
  { 
    act("Your shield &+Yflares&n up and discharges a bolt of &+Blightning&n towards $n.",
        FALSE, attacker, 0, victim, TO_VICT);
    act("$N's shield &+Yflares&n up and discharges a bolt of &+Blightning&n towards YOU!",
        FALSE, attacker, 0, victim, TO_CHAR);
    act("$N's shield &+Yflares&n up and discharges a bolt of &+Blightning&n towards $n.",
        FALSE, attacker, 0, victim, TO_NOTVICT);

    spell_lightning_bolt(GET_LEVEL(victim), victim, 0, SPELL_TYPE_SPELL, attacker, 0);
    if(!is_char_in_room(attacker, room) ||
        !is_char_in_room(victim, room))
      return true;
  }

  /* Bless wearing off shield. */
  if(IS_SET(shield->extra2_flags, ITEM2_BLESS) &&
      !number(0,299))
  {
    act("A &+Cblessed glow&n around your $q fades.", FALSE, victim, shield, 0,
        TO_CHAR);
    affect_from_obj(shield, SPELL_BLESS);
    REMOVE_BIT(shield->extra2_flags, ITEM2_BLESS);
  }

  if(GET_CHAR_SKILL(victim, SKILL_IMPROVED_SHIELD_COMBAT) &&
      number(1, 400) < MAX(20, GET_CHAR_SKILL(victim, SKILL_IMPROVED_SHIELD_COMBAT)) &&
      MIN_POS(victim, POS_STANDING + STAT_NORMAL))
  {
    struct damage_messages messages = 
    {
      "You block $N's lunge at you and immediately slam $M with your shield!",
      "$n blocks your lunge and then slams $s shield into you.",
      "$n blocks $N's lunge and then immediately slams $M with $s shield!",
      "You block $N and then slam your shield into $M, crushing $S skull.",
      "$n blocks your attack and then slams $s shield into you! The lights go out.",
      "$n blocks $M's attack and then slams $s shield into $S body crunching it like a soft egg."
    };

    /* Improved shield combat property can be adjusted on the fly - Lucrot */
    int dmg = number(40,
        MAX(41,
          GET_CHAR_SKILL(victim, SKILL_IMPROVED_SHIELD_COMBAT) *
          (int) get_property("skill.improvedShieldCombat.damageMultiplier", 1)));

    if(melee_damage(victim, attacker, dmg, 0, &messages) != DAM_NONEDEAD)
      return true;
    else
      return false;
  } 
  else if(learned > 45 &&
      !number(0, 2))
  {
    act("You expertly block $n's lunge at you.", FALSE, attacker, 0, victim,
        TO_VICT | ACT_NOTTERSE);
    act("$N easily blocks your futile lunge at $M.", FALSE, attacker, 0, victim,
        TO_CHAR | ACT_NOTTERSE);
    act("$N easily blocks $n's lunge at $M.", FALSE, attacker, 0, victim,
        TO_NOTVICT | ACT_NOTTERSE);
    return true;
  }    
  else if(learned < 20)
  {
    act("You barely block $n's lunge at you.", FALSE, attacker, 0, victim,
        TO_VICT | ACT_NOTTERSE);
    act("$N just barely gets $S shield up to block your attack.",
        FALSE, attacker, 0, victim, TO_CHAR | ACT_NOTTERSE);
    act("$N barely blocks $n's lunge at $M.", FALSE, attacker, 0, victim,
        TO_NOTVICT | ACT_NOTTERSE);
    return true;
  }
  else
  {
    act("You block $n's lunge at you.", FALSE, attacker, 0, victim,
        TO_VICT | ACT_NOTTERSE);
    act("$N blocks your futile lunge at $M.", FALSE, attacker, 0, victim,
        TO_CHAR | ACT_NOTTERSE);
    act("$N blocks $n's lunge at $M.", FALSE, attacker, 0, victim,
        TO_NOTVICT | ACT_NOTTERSE);
    return true;
  }  
  return false;
}

int MonkRiposte(P_char victim, P_char attacker, P_obj wpn)
{
  int percent, learned;
  int skl = GET_CHAR_SKILL(victim, SKILL_MARTIAL_ARTS);

  if(!(attacker) ||
      !(victim) ||
      !GET_CLASS(victim, CLASS_MONK) ||
      !IS_ALIVE(victim) ||
      !(skl) ||
      !IS_ALIVE(attacker) ||
      IS_IMMOBILE(victim) ||
      IS_BLIND(victim) ||
      !IS_AWAKE(victim) ||
      IS_STUNNED(victim))
  {
    return 0;
  }

  if(IS_PC(victim) &&
      notch_skill(victim, SKILL_MARTIAL_ARTS, get_property("skill.notch.defensive", 17)))
    return 0;

  if(IS_ELITE(victim))
    skl = (int)(skl * 1.1);

  percent = (int)(skl / 2) - (int)(WeaponSkill(attacker, wpn) / 3);

  percent += dex_app[STAT_INDEX(GET_C_DEX(victim))].reaction * 2;
  percent += str_app[STAT_INDEX(GET_C_STR(victim))].tohit * 1.5;
  percent -= (str_app[STAT_INDEX(GET_C_STR(attacker))].tohit +
    str_app[STAT_INDEX(GET_C_STR(attacker))].todam);

  if(!MIN_POS(victim, POS_STANDING + STAT_NORMAL))
  {

    // This allows monks a chance to regain their feet based on agil and 
    // martial arts skill. Aug09 -Lucrot

    // All this does is set someone up to be repeatedly bashed and lagged,
    // utterly and insanely stupid. -- Jexni 1/21/11

    if( GET_C_AGI(victim) > number(1, 1000) && !get_spell_from_char(victim, SKILL_MARTIAL_ARTS)
      && GET_CHAR_SKILL(victim, SKILL_MARTIAL_ARTS) > number(1, 100))
    {
      act("$n tucks in $s arms, rolls quickly away, then thrust $s feet skywards, leaping back to $s feet!",
          TRUE, victim, 0, attacker, TO_NOTVICT);
      act("$n tucks in $s arms, rolls away from $N's attack, then thrust $s feet skywards, and leaps to $s feet!",
          TRUE, victim, 0, attacker, TO_VICT);
      act("You tuck in your arms, roll away from $N's blow, then leap to your feet!",
          TRUE, victim, 0, attacker, TO_CHAR);
      SET_POS(victim, POS_STANDING + GET_STAT(victim));
      // Clear the lag!
      disarm_char_nevents(victim, event_wait);
      set_short_affected_by(victim, SKILL_MARTIAL_ARTS, PULSE_VIOLENCE);
      REMOVE_BIT(victim->specials.act2, PLR2_WAIT);
      update_pos(victim);
      return FALSE;
    }

    percent = 5;
  }

  if(!MIN_POS(attacker, POS_STANDING + STAT_NORMAL))
    percent *= 1.1;

  percent = BOUNDED(learned / 25, percent, 15);

  if(IS_GREATER_RACE(attacker) || IS_ELITE(attacker))
    percent /= 2;

  if(number(1, 150) > percent)
    return 0;

  if(number(1, 150) > skl)
  {
    act("$n arches around $N's blow, and deals a brutal counterattack!", TRUE,
        victim, 0, attacker, TO_NOTVICT);
    act("$n arches around your blow, and deals a brutal counterattack to YOU!",
        TRUE, victim, 0, attacker, TO_VICT);
    act("You arch around $N's blow, and deal a brutal counterattack!", TRUE,
        victim, 0, attacker, TO_CHAR);
    hit(victim, attacker, NULL);
    return false;
  }

  act("$n completely sidesteps $N's blow, shifts $s balance, and strikes back at $M!",
      TRUE, victim, 0, attacker, TO_NOTVICT);
  act("$n completely sidesteps your blow, shifts $s balance, and strikes back at YOU!",
      TRUE, victim, 0, attacker, TO_VICT);
  act("You completely sidestep $N's blow, shift your balance, and strike back at $M!",
      TRUE, victim, 0, attacker, TO_CHAR);

  hit(victim, attacker, NULL);
  return true;
}

int parrySucceed(P_char victim, P_char attacker, P_obj wpn)
{
  int learnedvictim = GET_CHAR_SKILL(victim, SKILL_PARRY);
  int learnedattacker;
  int blindfightskl = GET_CHAR_SKILL(victim, SKILL_BLINDFIGHTING);
  bool npcepicparry = false;
  int expertparry = 0;

  if( !IS_ALIVE(victim) || !IS_ALIVE(attacker) )
    return FALSE;

  if( GET_POS(victim) != POS_STANDING )
    return FALSE;


  if( affected_by_spell(victim, SPELL_COMBAT_MIND) && GET_CLASS(victim, CLASS_PSIONICIST) )
    learnedvictim += GET_LEVEL(victim);

  if(learnedvictim < 1)
    return FALSE;

  // Monks and immaterial (ghosts, phantoms, etc...) may be parried when attacker
  // has a weapon.
  if(GET_CLASS(attacker, CLASS_MONK) || IS_IMMATERIAL(attacker))
    if(!attacker->equipment[WIELD] && !attacker->equipment[WIELD2])
      return FALSE;

  // Ensure the victim has a weapon for parrying.
  // May want to expand this in the future by adding modifiers
  // based on weapon types (e.g. maces are harder to parry with or
  // parry against).
  if( !victim->equipment[WIELD] && !victim->equipment[WIELD2]
    && !victim->equipment[WIELD3] && !victim->equipment[WIELD4] )
    return FALSE;

  // Flaying weapons are !parry.
  if( (wpn && IS_FLAYING(wpn)) ||
      (victim->equipment[WIELD] && IS_FLAYING(victim->equipment[WIELD])) )
    return FALSE;

  if( affected_by_spell(victim, SKILL_RAGE) && attacker != GET_OPPONENT(victim) )
    return FALSE;

  // Notching the parry skill fails the parry check.
  if( notch_skill(victim, SKILL_PARRY, get_property("skill.notch.defensive", 17)) &&
      !affected_by_spell(victim, SPELL_COMBAT_MIND) )
    return FALSE;

  // Victim's parry:

  if(affected_by_spell(victim, SPELL_COMBAT_MIND) &&
      GET_CLASS(victim, CLASS_PSIONICIST))
    learnedvictim += (int)(GET_LEVEL(victim) / 2);
  learnedvictim += dex_app[STAT_INDEX(GET_C_DEX(victim))].reaction * 15;
  learnedvictim += wis_app[STAT_INDEX(GET_C_WIS(victim))].bonus * 5;

  if( learnedvictim < 1 )
    return FALSE;

  // Attacker's parry:
  learnedattacker = WeaponSkill(attacker, wpn);
  learnedattacker += str_app[STAT_INDEX(GET_C_STR(attacker))].tohit * 10;
  learnedattacker += str_app[STAT_INDEX(GET_C_STR(attacker))].todam * 15;

  // If attacker is significantly stronger than the defender, parry is reduced.
  // This will benefit giants, dragons, etc... which is logical.
  if( GET_C_STR(attacker) > GET_C_STR(victim) + 35 )
    learnedattacker += GET_C_STR(attacker) - 35 - GET_C_STR(victim); 

  // Harder to parry incoming attacks when not standing.
  if( !MIN_POS(victim, POS_STANDING + STAT_NORMAL) )
    learnedvictim = (int) (learnedvictim * 0.75);

  // Attackers are easier to parry when they are not standing.
  if( !MIN_POS(attacker, POS_STANDING + STAT_NORMAL) )
    learnedattacker = (int) (learnedattacker * 0.65); //old 0.75

  if(IS_AFFECTED5(victim, AFF5_DAZZLEE))
    learnedvictim = (int) (learnedvictim * 0.00);

  if(affected_by_spell(victim, SKILL_GAZE))
    learnedvictim = (int) (learnedvictim * 0.80);

  // adding blindfighting check - Jexni 1/21/11
  if(IS_BLIND(victim))
    learnedvictim = (int) (blindfightskl > 0 ? (learnedvictim * (blindfightskl / 200)) : (learnedvictim * 0.05));

  if(IS_STUNNED(victim))
    learnedvictim = (int) (learnedvictim * 0.90);

  // Elite warriors and paladins have maximum expert parry.  
  if(IS_ELITE(victim))
  {
    if(GET_CLASS(victim, CLASS_WARRIOR) ||
        GET_CLASS(victim, CLASS_PALADIN) ||
        GET_CLASS(victim, CLASS_RANGER))
    {
      learnedvictim = (int) (learnedvictim * 1.25);
      npcepicparry == TRUE;
    }
  }

  // Player expert parry.
  if(!IS_NPC(victim) &&
      GET_CHAR_SKILL(victim, SKILL_EXPERT_PARRY))
  {
    expertparry = GET_CHAR_SKILL(victim, SKILL_EXPERT_PARRY);
    // 125 percent max bonus

    learnedvictim = (int) (learnedvictim * (1 + expertparry/400));
  }

  //  Blademasters and swashbucklers have a better chance.
  if(GET_SPEC(victim, CLASS_RANGER, SPEC_BLADEMASTER) ||
      GET_SPEC(victim, CLASS_WARRIOR, SPEC_SWASHBUCKLER))
    learnedvictim = (int) (learnedvictim * 1.10);

  //  Better chance for ap's
  if((GET_CLASS(victim, CLASS_ANTIPALADIN) || GET_CLASS(victim, CLASS_PALADIN)) &&
      is_wielding_paladin_sword(victim))
    learnedvictim = (int) (learnedvictim * 1.15);

  // Harder to parry a swashbuckler.
  if(GET_SPEC(attacker, CLASS_WARRIOR, SPEC_SWASHBUCKLER))
    learnedattacker = (int) (learnedattacker * 0.80);

  // Random 5% change based on random luck comparison.
  if(number(0, GET_C_LUK(victim)) > number(0, GET_C_LUK(attacker)))
    learnedvictim = (int) (learnedvictim * 1.05);

  // Dragons are more difficult to parry, but not impossible.
  if(IS_GREATER_RACE(attacker))
    learnedattacker += GET_LEVEL(attacker);

  // Much harder to parry with fireweapons like a bow, but not impossible.
  P_obj weapon = victim->equipment[WIELD];

  if(weapon && GET_ITEM_TYPE(weapon) == ITEM_FIREWEAPON)
  {
    learnedvictim /= 10;
  }

  if(weapon)
  {
    learnedattacker += (GET_OBJ_WEIGHT(weapon) * 2);
  }

  // Harder to parry something you are not fighting.
  if( IS_PC(victim) && GET_OPPONENT(victim) != attacker )
  {
    learnedvictim = (int) (learnedvictim * 0.90);
  }

  // Generate attacker and victim ranges.
  int defroll, attroll;
  defroll = MAX(5, number(1, learnedvictim));
  attroll = MAX(5, number(1, learnedattacker));

  // debug("Defroll (%d), Attroll (%d)", defroll, attroll);

  // Simple parry success check via comparison of two constrained random numbers.
  if(attroll > defroll)
    return FALSE;

  if(rapier_dirk(victim, attacker))
  {
    return TRUE;
  }

  // Riposte check.
  if( try_riposte(victim, attacker, wpn) )
    return TRUE;

  if( expertparry > number(1, 250) && !IS_NPC(victim) )
  {
    act("You anticipate $n's maneuver and &+wmasterfully&n parry the attack.", FALSE, attacker, 0, victim,
        TO_VICT | ACT_NOTTERSE);
    act("$N anticipates your attack and &+wmasterfully&n parries your blow.", FALSE, attacker, 0, victim,
        TO_CHAR | ACT_NOTTERSE);
    act("$N anticipates $n's attack and &+wmasterfully&n parries the incoming blow.", FALSE, attacker, 0, victim,
        TO_NOTVICT | ACT_NOTTERSE);
  }
  else if( ((npcepicparry == true) && !number(0, 12)) && IS_NPC(victim) )
  {
    //  act("You anticipate $n's maneuver and &+wmasterfully&n parry the attack.", FALSE, attacker, 0, victim,
    //      TO_VICT | ACT_NOTTERSE);
    act("$N anticipates your attack and &+wmasterfully&n parries your blow.", FALSE, attacker, 0, victim,
        TO_CHAR | ACT_NOTTERSE);
    act("$N anticipates $n's attack and &+wmasterfully&n parries the incoming blow.", FALSE, attacker, 0, victim,
        TO_NOTVICT | ACT_NOTTERSE);
  }
  else
  {
    act("You parry $n's lunge at you.", FALSE, attacker, 0, victim,
        TO_VICT | ACT_NOTTERSE);
    act("$N parries your futile lunge at $M.", FALSE, attacker, 0, victim,
        TO_CHAR | ACT_NOTTERSE);
    act("$N parries $n's lunge at $M.", FALSE, attacker, 0, victim,
        TO_NOTVICT | ACT_NOTTERSE);
  }
  return TRUE;
}

/* old mob-based trophy; deprecated in favor of zone trophy */
/*
   void do_trophy_mob(P_char ch, char *arg, int cmd)
   {
   int      real, clear_it = FALSE;
   char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
   char     Gbuf3[MAX_STRING_LENGTH];
   P_char   who, rch;
   struct trophy_data *tr;

#ifndef TROPHY
send_to_char("temp. disabled...\r\n", ch);
return;
#endif
rch = GET_PLYR(ch);

if (IS_NPC(rch))
{
send_to_char("DUH.\r\n", ch);
return;
}

argument_interpreter(arg, Gbuf2, Gbuf3);

if (IS_TRUSTED(rch))
{
if (!(who = get_char_vis(rch, Gbuf2)))
{
send_to_char("They don't appear to be in the game.\r\n", ch);
return;
}

if (*Gbuf3 && isname("clear", Gbuf3))
{
struct trophy_data *temp;

//      tr = who->only.pc->trophy;
//      who->only.pc->trophy = NULL;
for (; tr; tr = temp)
{
temp = tr->next;
mm_release(dead_trophy_pool, tr);
}
send_to_char("Ok, trophy cleared.\r\n", ch);
return;
}
else if( *Gbuf3 && isname("frags", Gbuf3))
{
show_frag_trophy(ch, who);
return;
}
}
else
{
who = rch;
}

if( isname("frags", Gbuf2) )
{
show_frag_trophy(ch, ch);
}
else
{
snprintf(Gbuf1, MAX_STRING_LENGTH, "&+WTrophy data:&n\r\n");
for (tr = who->only.pc->trophy; tr; tr = tr->next)
{
char     let;

if (tr->kills < 1000)
let = 'G';
else if (tr->kills < 2500)
let = 'Y';
else
let = 'R';
real = real_mobile0(tr->vnum);
snprintf(Gbuf1 + strlen(Gbuf1), MAX_STRING_LENGTH - strlen(Gbuf1),
    "&+W(&n&+%c%2d.%02d&+W)&n %s\r\n", let,
    tr->kills / 100, tr->kills % 100,
    real ? mob_index[real].desc2 : "Unknown");

}
strcat(Gbuf1, "\r\n");
page_string(ch->desc, Gbuf1, 1);
return;
}

}
*/
/* remove a char from the list of fighting chars */

void stop_fighting(P_char ch)
{
  P_char   tmp;

  if (!SanityCheck(ch, "stop_fighting") || !IS_FIGHTING(ch))
    return;

  if (IS_SET(ch->specials.affected_by3, AFF3_TRACKING)) // needed for new track *Alv*
    REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING);

  ch->specials.was_fighting = GET_OPPONENT(ch);

  if (ch == combat_next_ch)
    combat_next_ch = ch->specials.next_fighting;
  if (combat_list == ch)
    combat_list = ch->specials.next_fighting;
  else
  {
    for (tmp = combat_list; tmp && (tmp->specials.next_fighting != ch);
        tmp = tmp->specials.next_fighting) ;
    if (!tmp)
    {
      logit(LOG_EXIT, "%s not found in combat_list stop_fighting()",
          GET_NAME(ch));
      raise(SIGSEGV);
    }
    tmp->specials.next_fighting = ch->specials.next_fighting;
  }

  ch->specials.next_fighting = NULL;
  GET_OPPONENT(ch) = NULL;

  if (affected_by_spell(ch, SPELL_CEGILUNE_BLADE)) {
    struct affected_type *afp = get_spell_from_char(ch, SPELL_CEGILUNE_BLADE);
    afp->modifier = 0;
  }

  if ( GET_CHAR_SKILL(ch, SKILL_LANCE_CHARGE) != 0 )
    set_short_affected_by(ch, SKILL_LANCE_CHARGE, PULSE_VIOLENCE / 2); // to prevent flee/charge


  update_pos(ch);
}

void stop_destroying(P_char ch)
{
  P_char tmp;

  if( !SanityCheck(ch, "stop_destroying") || !IS_DESTROYING(ch) )
    return;

  if( destroying_list == ch )
  {
    destroying_list = ch->specials.next_destroying;
  }
  else
  {
    for (tmp = destroying_list; tmp && (tmp->specials.next_destroying != ch);
        tmp = tmp->specials.next_destroying) ;
    if (!tmp)
    {
      logit(LOG_EXIT, "%s not found in destroying_list stop_destroying()",
          GET_NAME(ch));
      raise(SIGSEGV);
    }
    tmp->specials.next_destroying = ch->specials.next_destroying;
  }

  ch->specials.next_destroying = NULL;
  ch->specials.destroying_obj = NULL;
}

void event_windstrom(P_char ch, P_char vict, char *args)
{
  int      hits;
  struct damage_messages messages = {
    "$N &+RSCR&+rEA&+RMS &nin pain as their body is filled with a sudden rush of &+rWa&+yrm&+rth!",
    "You howl in pain as the intense &+rWa&+yrm&+rth fills your body with intense &+MPAIN!",
    "$N &+RSCR&+rEA&+RMS &nin pain as their body is filled with a sudden rush of &+rWa&+yrm&+rth!",
    "$N quivers for a moment...then shatters into a thousand pieces!",
    "The &+Bchilling &+Ccold &nproves too much for you, and your body disintegrates.",
    "$N quivers for a moment...then shatters into a thousand pieces!"
  };


  if (sscanf(args, "%d", &hits) != 1)
    return;

  spell_damage(ch, vict, hits * GET_LEVEL(ch) / 4, SPLDAM_COLD,
      SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
}


#ifndef NEW_COMBAT
#   define ADD_ATTACK(slot) (attacks[number_attacks++] = (slot))

int calculate_attacks(P_char ch, int attacks[])
{
  P_char rider;
  int number_attacks = 0;
  P_obj weapon;

  if( IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE) || !IS_ALIVE(GET_OPPONENT(ch)) )
  {
    return 0;
  }

  if (GET_CLASS(ch, CLASS_MONK))
  {
    bool fighting_pc = GET_OPPONENT(ch) ? IS_PC(GET_OPPONENT(ch)) : FALSE;
    int num_atts = MonkNumberOfAttacks(ch);
    int weight_threshold = GET_C_STR(ch) / 2;

    if( !IS_TRUSTED(ch) && (IS_CARRYING_W(ch, rider) >= weight_threshold) && IS_PC(ch) )
    {
      // if (affected_by_spell(ch, SPELL_STONE_SKIN))
      // num_atts -=
      // MIN(num_atts - 1, (int) ((IS_CARRYING_W(ch, rider) - 20) / 10));
      // else
      num_atts -= MIN(num_atts - 1, (int) ((IS_CARRYING_W(ch, rider) - 30) / 10));

      //if (!number(0, 4))
      send_to_char("&+LYou feel weighed down, which is causing you to lose attacks.&n\r\n", ch);
    }

    if (IS_AFFECTED2(ch, AFF2_SLOW) && num_atts > 1)
      num_atts /= 2;

    while( num_atts-- )
    {
      // We're not on the last attack and up to 35% chance at level 56 (% max health for 2 hits damage).
      if( fighting_pc && (num_atts > 0) && (( (GET_LEVEL( ch ) / 2 + 9) ) >= number( 1, 100 )) )
      {
        // Costs 3 attacks worth.
        num_atts--;
        // Hack for %max health damage.
        ADD_ATTACK(WEAR_NONE);
      }
      else
      {
        ADD_ATTACK(PRIMARY_WEAPON);
      }
    }
  }
  else
  {                           // not MONK
    if( !IS_AFFECTED2(ch, AFF2_SLOW) || IS_AFFECTED(ch, AFF_HASTE) )
    {
      ADD_ATTACK(PRIMARY_WEAPON);
      // Can swing both primary hands at once - 50% for newbs, 100% for maxxed dual wield.
      if( HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON]
        && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50 ) >= number( 1, 100 )) )
      {
        ADD_ATTACK(THIRD_WEAPON);
      }
    }

    // I hate it when both my hands are trying to swing the same 1h sword.
    if( ch->equipment[PRIMARY_WEAPON] && ch->equipment[SECONDARY_WEAPON]
      && (ch->equipment[PRIMARY_WEAPON] != ch->equipment[SECONDARY_WEAPON]) )
    {
      if( notch_skill(ch, SKILL_DUAL_WIELD, get_property("skill.notch.offensive.auto", 4))
        || number(1, 100) < GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) )
      {
        ADD_ATTACK(SECONDARY_WEAPON);
        // Can swing both secondary hands at once - 50% for newbs, 100% for maxxed dual wield.
        if( HAS_FOUR_HANDS(ch) && ch->equipment[FOURTH_WEAPON]
          && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50 ) >= number( 1, 100 )) )
        {
          ADD_ATTACK(FOURTH_WEAPON);
        }

        if( GET_CHAR_SKILL(ch, SKILL_IMPROVED_TWOWEAPON) >= number(1, 100) )
        {
          ADD_ATTACK(SECONDARY_WEAPON);
          // Can swing both secondary hands at once - 50% for newbs, 100% for maxxed dual wield.
          if( HAS_FOUR_HANDS(ch) && ch->equipment[FOURTH_WEAPON]
            && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50 ) >= number( 1, 100 )) )
          {
            ADD_ATTACK(FOURTH_WEAPON);
          }
        }
      }
    }

    /*
        if(ch->player.race == RACE_KOBOLD || ch->player.race == RACE_GNOME)
        {
        if (number(1, 160) < GET_C_AGI(ch))
        {
        ADD_ATTACK(PRIMARY_WEAPON);
        send_to_char("&nYou move swiftly and execute an extra attack against your foe!&n\n\r", ch);
        }
        }

        if(ch->player.race == RACE_GOBLIN || ch->player.race == RACE_HALFLING)
        {
        if (number(1, 220) < GET_C_AGI(ch))
        {
        ADD_ATTACK(PRIMARY_WEAPON);
        send_to_char("&nYou move swiftly and execute an extra attack against your foe!&n\n\r", ch);
        }
        }
        */

/* No longer giving minos an extra attack.
    //loop through affects - Drannak
    struct affected_type *findaf, *next_af;  //initialize affects

    for(findaf = ch->affected; findaf; findaf = next_af)
    {
      next_af = findaf->next;
      if(findaf && findaf->type == TAG_MINOTAUR_RAGE)
        ADD_ATTACK(PRIMARY_WEAPON);
    }
*/

    // This randomness might should go too.
    if( GET_SPEC(ch, CLASS_ANTIPALADIN, SPEC_VIOLATOR) )
    {
      ADD_ATTACK(PRIMARY_WEAPON);
    }

    if(GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
    {
      int zealproc;
      struct affected_type af;
      if (GET_C_STR(ch) > number(1, 340))
      {
        zealproc = (number(1,5));
        switch (zealproc)
        {
          case 1:
            bzero(&af, sizeof(af));
            int bonus;
            if(GET_C_STR(ch) >= 300)
              bonus = 0;
            else
              bonus = number(5, 10);
            af.duration = 50;
            af.location = APPLY_STR_MAX;
            af.modifier = bonus;
            af.flags = AFFTYPE_SHORT;
            affect_to_char(ch, &af);

            act("&nThe power of your &+Wgod&n suddenly fills your body and you feel &+BMUCH&n stronger!&n",
              FALSE, ch, 0, 0, TO_CHAR);
            act("&n$n gasps suddenly as their body is &+Benhanced&n by some &+Wother-worldly &npower!&n",
              FALSE, ch, 0, 0, TO_ROOM);
            do_say( ch, "Fill me with your strength!", CMD_SAY );
            break;
          case 2:
            act("&+WYour devotion to your god causes you to unleash a &+rF&+Rlurr&+ry&+W of attacks against $N!&n",
                FALSE, ch, NULL, GET_OPPONENT(ch), TO_CHAR);
            act("$n says 'The non-believer must be punished!", FALSE, ch, 0, 0, TO_ROOM);
            ADD_ATTACK(PRIMARY_WEAPON);
            ADD_ATTACK(PRIMARY_WEAPON);
            do_say( ch, "What once was lost, is now found upon your skull!", CMD_SAY );
            break;
          case 3:
            bzero(&af, sizeof(af));
            if(GET_C_STR(ch) >= 300)
              bonus = 0;
            else
              bonus = number(5, 10);
            af.duration = 50;
            af.location = APPLY_STR_MAX;
            af.modifier = bonus;
            af.flags = AFFTYPE_SHORT;
            affect_to_char(ch, &af);

            act("&nThe power of your &+Wgod&n suddenly fills your body and you feel &+BMUCH&n stronger!&n",
              FALSE, ch, 0, 0, TO_CHAR);
            act("&n$n gasps suddenly as their body is &+Benhanced&n by some &+Wother-worldly &npower!&n",
              FALSE, ch, 0, 0, TO_NOTVICT);
            act("&+WYour devotion to your god causes you to unleash a &+rF&+Rlurr&+ry&+W of attacks against $N!&n",
              FALSE, ch, 0, GET_OPPONENT(ch), TO_CHAR);
            do_say( ch, "The non-believer must be punished!", CMD_SAY );
            ADD_ATTACK(PRIMARY_WEAPON);
            ADD_ATTACK(PRIMARY_WEAPON);
            break;
          case 4:
            act("&+WYour devotion to your god causes you to unleash a &+rF&+Rlurr&+ry&+W of attacks against $N!&n",
                FALSE, ch, 0, GET_OPPONENT(ch), TO_CHAR);
            ADD_ATTACK(PRIMARY_WEAPON);
            do_say( ch, "Beg for forgiveness and be saved!", CMD_SAY );
            ADD_ATTACK(PRIMARY_WEAPON);
            break;
          case 5:
            act("&+WYour devotion to your god causes you to unleash a &+rF&+Rlurr&+ry&+W of attacks against $N!&n",
                FALSE, ch, 0, GET_OPPONENT(ch), TO_CHAR);
            do_say( ch, "May your bloodshed be a willing sacrifice!", CMD_SAY );
            ADD_ATTACK(PRIMARY_WEAPON);
            ADD_ATTACK(PRIMARY_WEAPON);
            break;
        }
      }
    }

    if( GET_CLASS(ch, CLASS_PSIONICIST) && affected_by_spell(ch, SPELL_COMBAT_MIND) )
    {
      ADD_ATTACK(PRIMARY_WEAPON);

      if( GET_LEVEL(ch) > 51 )
        ADD_ATTACK(PRIMARY_WEAPON);
    }

    if( notch_skill(ch, SKILL_DOUBLE_ATTACK, get_property("skill.notch.offensive.auto", 4) )
      || GET_CHAR_SKILL(ch, SKILL_DOUBLE_ATTACK) >= number(1, 100))
    {
      ADD_ATTACK(PRIMARY_WEAPON);
      // Can swing both primary hands at once - 99% chance max.
      if( HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON]
        && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50 ) > number( 1, 100 )) )
      {
        ADD_ATTACK(THIRD_WEAPON);
      }
      // Can swing second secondary hand - 94% chance max.
      if( HAS_FOUR_HANDS(ch) && ch->equipment[FOURTH_WEAPON]
        && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 45 ) > number( 1, 100 )) )
      {
        ADD_ATTACK(FOURTH_WEAPON);
      }
    }

    if (notch_skill(ch, SKILL_TRIPLE_ATTACK, get_property("skill.notch.offensive.auto", 4))
      || GET_CHAR_SKILL(ch, SKILL_TRIPLE_ATTACK) >= number(1, 100))
    {
      ADD_ATTACK(PRIMARY_WEAPON);
      // Can swing both primary hands at once - 94% chance max.
      if( HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON]
        && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 45 ) > number( 1, 100 )) )
      {
        ADD_ATTACK(THIRD_WEAPON);
      }
      // Can swing primary secondary hand - 94% chance max.
      if( HAS_FOUR_HANDS(ch) && ch->equipment[SECONDARY_WEAPON]
        && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 45 ) > number( 1, 100 )) )
      {
        ADD_ATTACK(SECONDARY_WEAPON);
      }
    }

    if( notch_skill(ch, SKILL_QUADRUPLE_ATTACK, get_property("skill.notch.offensive.auto", 4))
      || GET_CHAR_SKILL(ch, SKILL_QUADRUPLE_ATTACK) > number(1, 100) )
    {
      ADD_ATTACK(PRIMARY_WEAPON);
      // Can swing both primary hands at once - 94% chance max.
      if( HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON]
        && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 45 ) > number( 1, 100 )) )
      {
        ADD_ATTACK(THIRD_WEAPON);
      }
    }
  }

  // both monks and others below
  if (IS_AFFECTED(ch, AFF_HASTE))
  {
    if( !IS_AFFECTED2(ch, AFF2_SLOW) )
    {
      ADD_ATTACK(PRIMARY_WEAPON);
      // Can swing both primary hands at once - 100% chance max.
      if( HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON]
        && (( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50 ) >= number( 1, 100 )) )
      {
        ADD_ATTACK(THIRD_WEAPON);
      }
    }
  }

  // High dex now grants extra attacks (does not include bare hands).
  // Dex primary weapon
  weapon = ch->equipment[PRIMARY_WEAPON];
  if( (weapon == NULL) || (weapon->type == ITEM_WEAPON) )
  {
    int actpct = (100 * ( (weapon == NULL) ? 0 : GET_OBJ_WEIGHT(weapon) )) / GET_C_STR(ch);

    if( (( actpct <= 6 ) && ( GET_C_DEX(ch) >= 125 ))
      || (( actpct <= 20 ) && ( GET_C_DEX(ch) >= 150 )) )
    {
      if( number(1, GET_C_DEX(ch)) > 60 )
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
    }
  }
  // Dex secondary weapon
  weapon = ch->equipment[SECONDARY_WEAPON];
  if( (weapon == NULL) || (weapon->type == ITEM_WEAPON) )
  {
    int actpct = (100 * ( (weapon == NULL) ? 0 : GET_OBJ_WEIGHT(weapon) )) / GET_C_STR(ch);

    if( (actpct <= 6) && (GET_C_DEX(ch) >= 150) )
    {
      if( number(1, GET_C_DEX(ch)) > 60 )
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(SECONDARY_WEAPON);
      }
    }
  }

/* Keeping the old 3 tier code for dex attacks, but reducing it to two.
  if(actpct <= 5)
  {
    if(GET_C_DEX(ch) >= 155)
    {
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
    }
    else if(GET_C_DEX(ch) >=140)
    {
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
    }
    else if(GET_C_DEX(ch) >=125)
    {
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
    }
  }
  else if(actpct <= 10)
  {
    if(GET_C_DEX(ch) >= 155)
    {
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
    }
    else if(GET_C_DEX(ch) >=140)
    {
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
    }
  }
  else if(actpct <= 20)
  {
    if(GET_C_DEX(ch) >= 155)
    {
      if(number(1, GET_C_DEX(ch)) > 60)
      {
        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
        ADD_ATTACK(PRIMARY_WEAPON);
      }
    }
  }
*/

  if(GET_CLASS(ch, CLASS_CLERIC) &&
      affected_by_spell(ch, SPELL_DIVINE_FURY))
    ADD_ATTACK(PRIMARY_WEAPON);

  if (GET_CLASS(ch, CLASS_DREADLORD | CLASS_AVENGER) &&
      GET_CHAR_SKILL(ch, required_weapon_skill(ch->equipment[PRIMARY_WEAPON])) > 69)
    ADD_ATTACK(PRIMARY_WEAPON);

  if (affected_by_spell(ch, SPELL_HOLY_SWORD))
    ADD_ATTACK(PRIMARY_WEAPON);

  if (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM))
  {
    ADD_ATTACK(PRIMARY_WEAPON);
    if (number(0, 2))
      ADD_ATTACK(PRIMARY_WEAPON);
  }

  if (get_linking_char(ch, LNK_ESSENCE_OF_WOLF))
  {
    int      extra = number(2, 4);

    extra = MIN(extra, GET_LEVEL(ch) / 10);

    if (ch->equipment[SECONDARY_WEAPON])
    {
      ADD_ATTACK(SECONDARY_WEAPON);
      if (number(0, 1))
      {
        extra--;
        ADD_ATTACK(SECONDARY_WEAPON);
      }
    }

    while (extra-- > 0)
      ADD_ATTACK(PRIMARY_WEAPON);
  }

  // This is a reaver-only bonus, as it goddamned should have been originally
  if (affected_by_spell(ch, SPELL_KANCHELSIS_FURY) && GET_CLASS(ch, CLASS_REAVER))
  {
    ADD_ATTACK(PRIMARY_WEAPON);

    if (GET_LEVEL(ch) >= 41 && ch->equipment[SECONDARY_WEAPON] &&
        number(1, 100) < GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD))
      ADD_ATTACK(SECONDARY_WEAPON);

    if (GET_LEVEL(ch) >= 51)
      ADD_ATTACK(PRIMARY_WEAPON);

    if (GET_LEVEL(ch) >= 56)  // Long into wipe, give them something to strive for
      ADD_ATTACK(SECONDARY_WEAPON);
  }

  if (IS_AFFECTED2(ch, AFF2_FLURRY) && number_attacks > 4)
  {
    int maxattacks = number_attacks;
    number_attacks = number(4, maxattacks);
  }

  if (IS_AFFECTED3(ch, AFF3_BLUR))
  {
    ADD_ATTACK(PRIMARY_WEAPON);
    if ((GET_CLASS(ch, CLASS_RANGER) || GET_SECONDARY_CLASS(ch, CLASS_RANGER)) &&
        number(1, 100) < GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) &&
        ch->equipment[SECONDARY_WEAPON])
      ADD_ATTACK(SECONDARY_WEAPON);

    int blurattackchance = (GET_LEVEL(ch) / 2);
    if (GET_CLASS(ch, CLASS_RANGER) && (number(1, 100) < blurattackchance) &&
        ch->equipment[SECONDARY_WEAPON])
      ADD_ATTACK(SECONDARY_WEAPON);
  }

  // Not-standing? half attacks - Drannak 7/22/13
  // Don't you mean less attacks?
  if( !MIN_POS(ch, POS_STANDING + STAT_NORMAL) && !GROUNDFIGHTING_CHECK(ch) )
  {
    if(GET_POS(ch) == POS_KNEELING)
      number_attacks = (int)(number_attacks - (number_attacks * .70));
    else
      number_attacks = (int)(number_attacks - (number_attacks / 2));
  }

  return number_attacks;
}
#   undef ADD_ATTACK

// ATTACK_DIVISOR is the amount of attacks lost due to inert barrier, armlock, etc.
//   ie 2 -> loss of 1/2 of attacks, 3 -> loss of 2/3 of attacks, etc.
// Note: The current limit for one round of battle is 256 attacks for one character.
#define ATTACK_DIVISOR   2
#define MAX_ATTACKS    256
void perform_violence(void)
{
  P_char   ch, opponent;
  char     GBuf1[MAX_STRING_LENGTH];
  struct affected_type *af, *next_af;
  struct affected_type aff;
  int      attacks[MAX_ATTACKS];
  int      number_attacks, real_attacks, div_attacks;
  int      num_hits;
  long     time_now;
  int      i, room, skill;
  std::set<int> room_rnums;
  std::set<int>::iterator it;
  int      door, nearby_room;
  P_char   tmp_ch;
  bool     melee_exp_pulse;
  // loop through everyone fighting

  time_now = time(0);
  melee_exp_pulse = ( (pulse % PULSE_VIOLENCE) == 0 );


  for( ch = destroying_list; ch; ch = ch->specials.next_destroying )
  {
    if (ch->specials.combat_tics > 0)
    {
      ch->specials.combat_tics--;
    }
    else
    {
      ch->specials.combat_tics = (int)ch->specials.base_combat_round;
#ifdef SIEGE_ENABLED
      multihit_siege( ch );
#endif
    }
  }

  for( ch = combat_list; ch; ch = combat_next_ch )
  {
    combat_next_ch = ch->specials.next_fighting;

    if( !IS_ALIVE(ch) )
    {
      continue;
    }

    room_rnums.insert( ch->in_room );

    opponent = GET_OPPONENT(ch);

    if( !opponent )
    {
      if( ch->in_room != NOWHERE )
      {
        debug( "perform_violence: %s fighting null opponent in %s (%d)!", GET_NAME(ch),
          world[ch->in_room].name, world[ch->in_room].number);
        logit(LOG_DEBUG, "perform_violence: %s fighting null opponent in %s (%d)!", GET_NAME(ch),
          world[ch->in_room].name, world[ch->in_room].number);
        extract_char(ch);
        return;
      }
      else
      {
        debug( "%s fighting null opponent in NOWHERE!", GET_NAME(ch) );
        logit(LOG_DEBUG, "%s fighting null opponent in NOWHERE!", GET_NAME(ch) );
        stop_fighting(ch);
        continue;
      }
    }

    // If someone's hitting on opponent (ch is) then give opponent melee exp.
    if( melee_exp_pulse && opponent && !IS_IMMOBILE(ch) && (opponent != ch) && IS_PC(opponent)
      && (opponent->in_room == ch->in_room) )
    {
      // Make sure we're gaining a positive amount.
      if( GET_LEVEL(ch) * 2 > GET_LEVEL(opponent) )
        gain_exp(opponent, ch, GET_LEVEL(ch) * 2 - GET_LEVEL(opponent), EXP_MELEE);
    }

    if( ch->specials.combat_tics-- > 0 )
    {
      continue;
    }
    else
    {
      ch->specials.combat_tics = (int)ch->specials.base_combat_round;
      if (affected_by_spell(ch, SKILL_WHIRLWIND))
      {
        GET_VITALITY(ch) -= get_property("skill.whirlwind.movement.drain", 10);
        if( GET_VITALITY(ch) > 0 )
        {
          ch->specials.combat_tics -= (ch->specials.combat_tics >> 1);
        }
        else
        {
          affect_from_char( ch, SKILL_WHIRLWIND );
        }
      }
    }

    if( IS_PC(ch) && IS_PC(opponent) )
    {
      startPvP( ch, GET_RACEWAR(ch) != GET_RACEWAR(opponent) );
      startPvP( opponent, GET_RACEWAR(ch) != GET_RACEWAR(opponent) );
    }

    if( !FightingCheck(ch, opponent, "perform_violence") )
    {
      continue;
    }

    /* misfire */
    if( IS_PC(ch) )
    {
      opponent = misfire_check(ch, opponent, DISALLOW_SELF | DISALLOW_BACKRANK);
    }

    if(HOLD_CANT_ATTACK(ch))
    {
      continue;
    }

    /** handle paralysis and slowness --TAM 2/94 **/

    if(IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
    {
      act("You remain paralyzed and can't do a thing to defend yourself.",
          FALSE, ch, 0, 0, TO_CHAR);
      act("$n strains to respond to $N's attack, but the paralysis is too overpowering.",
         FALSE, ch, 0, opponent, TO_ROOM);
      continue;
    }

    else if(IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS))
    {
      act("You couldn't budge a feather in your present condition.", FALSE,
          ch, 0, 0, TO_CHAR);
      act("$n is too preoccupied with $s nervous system problem to fight.",
          FALSE, ch, 0, 0, TO_ROOM);
      continue;
    }

    if(IS_AFFECTED2(ch, AFF2_CASTING) && !affected_by_spell(ch, SPELL_BATTLEMAGE))
    {
      continue;
    }

    if(IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE))
    {
      if(!number(0, 4))
      {
        send_to_char("You are being non-offensive... just a random reminder.\r\n", ch);
      }
      continue;
    }

    number_attacks = calculate_attacks(ch, attacks);
    if( number_attacks > MAX_ATTACKS )
      number_attacks = MAX_ATTACKS;
    if( number_attacks <= 0 )
    {
      send_to_char("You can't seem to get a single swing in.\n", ch);
      continue;
    }

    /*   (!GET_CLASS(ch, CLASS_PSIONICIST) && IS_AFFECTED3(ch, AFF3_INERTIAL_BARRIER) ) ||*/
    if(IS_AFFECTED3(opponent, AFF3_INERTIAL_BARRIER) || IS_ARMLOCK(ch) || affected_by_spell(ch, TAG_INTERCEPT))
    {
      // We add one because we don't want to drop to 0 attacks: 1:1, 2:1, 3:2, 4:2, 5:3 ...
      real_attacks = (number_attacks + ATTACK_DIVISOR - 1) / ATTACK_DIVISOR;
      div_attacks = ATTACK_DIVISOR;
    }
    else
    {
      real_attacks = number_attacks;
      div_attacks = 1;
    }

    if(!affected_by_spell(opponent, SKILL_BATTLE_SENSES) && GET_CHAR_SKILL(opponent, SKILL_BATTLE_SENSES)
      && GET_POS(opponent) == POS_STANDING && !IS_STUNNED(opponent) && !IS_BLIND(opponent))
    {
      if(notch_skill(opponent, SKILL_BATTLE_SENSES, get_property("skill.notch.defensive", 17)))
      { }
      else if((1 + (GET_CHAR_SKILL(opponent, SKILL_BATTLE_SENSES) / 10 )) >= number(1, 100))
      {
        act("&+wA sense of awareness &+bflows &+wover you as you move into &+rbattle.&n",
            FALSE, opponent, 0, 0, TO_CHAR);
        act("$n maneuvers around you like a &+yviper!&n", FALSE, opponent, 0, 0, TO_VICT);
        act("$n &+bflows into battle with the speed of a &+yviper.&n",
            FALSE, opponent, 0, 0, TO_NOTVICT);

        set_short_affected_by(opponent, SKILL_BATTLE_SENSES, 1);
      }
    }

    if(!IS_SUNLIT(ch->in_room))
    {
      if(GET_CHAR_SKILL(ch, SKILL_SHADOW_MOVEMENT) && !IS_BLIND(ch) && !IS_STUNNED(ch) && GET_POS(ch) == POS_STANDING
        && (notch_skill(ch, SKILL_SHADOW_MOVEMENT, get_property("skill.notch.offensive.auto", 4))
        || (1 + GET_CHAR_SKILL(ch, SKILL_SHADOW_MOVEMENT) / 4 > number(1, 100))))
      {
        act("$n &+wblinks out of existence ... then reappears &+ybehind&n $N!&n",
            FALSE, ch, 0, opponent, TO_NOTVICT);
        act("&+wYou blink out of existence and reappear behind&n $N.&n",
            FALSE, ch, 0, opponent, TO_CHAR);
        if(!IS_BLIND(opponent))
          act("$n &+wblinks out of existence ... then reappears &+ybehind you!&n",
              FALSE, ch, 0, opponent, TO_VICT);
        set_short_affected_by(ch, SKILL_SHADOW_MOVEMENT, 4);
      }
    }

    if(has_innate(opponent, INNATE_FPRESENCE))
    {
      frightening_presence(ch, opponent);
    }

    room = ch->in_room;

    if(room == NOWHERE)
    {
      logit(LOG_DEBUG, "perfom_violence by (%s) in fight.c in NOWHERE!", GET_NAME(opponent));
    }

    num_hits = 0;

    for (i = 0; i < real_attacks; i++)
    {
      if(!GET_OPPONENT(ch) || !is_char_in_room(ch, room) || !is_char_in_room(opponent, room))
      {
        break;
      }
      // Monk % maxhealth damage.
      if( attacks[i / div_attacks] == WEAR_NONE )
      {
        if( monk_superhit( ch, opponent ) )
          num_hits++;
      }
      else if(pv_common(ch, opponent, ch->equipment[attacks[i / div_attacks]]))
      {
        num_hits++;
      }
    }

    if(!is_char_in_room(opponent, room) || !is_char_in_room(ch, room))
    {
      continue;
    }

    if(is_char_in_room(ch, room) && IS_NPC(ch) && IS_AWAKE(ch) && CAN_ACT(ch))
    {
      MobCombat(ch);
    }

    appear(ch);
    appear(opponent);

    snprintf(GBuf1, MAX_STRING_LENGTH, "%sYou attack $N.%s [&+R%d&n hits]",
        (IS_PC(ch) &&
         IS_SET(ch->specials.act2, PLR2_BATTLEALERT)) ? "&+G-=[&n" : "",
        (IS_PC(ch) &&
         IS_SET(ch->specials.act2, PLR2_BATTLEALERT)) ? "&+G]=-&n" : "",
        num_hits);
    act(GBuf1, FALSE, ch, 0, opponent, TO_CHAR | ACT_TERSE);
    snprintf(GBuf1, MAX_STRING_LENGTH, "%s$n attacks you.%s [&+R%d&n hits]",
      (IS_PC( opponent ) && IS_SET( opponent->specials.act2, PLR2_BATTLEALERT )) ? "&+R-=[&n" : "",
      (IS_PC( opponent ) && IS_SET( opponent->specials.act2, PLR2_BATTLEALERT )) ? "&+R]=-&n" : "", num_hits);
    act(GBuf1, FALSE, ch, 0, opponent, TO_VICT | ACT_TERSE);
    snprintf(GBuf1, MAX_STRING_LENGTH, "$n attacks $N. [&+R%d&n hits]", num_hits);
    act(GBuf1, FALSE, ch, 0, opponent, TO_NOTVICT | ACT_TERSE);
  }

  /* Now room_rnums contains rooms where combat took place. Handle
     each room once. We don't add any checks here, since they will
     be done by the flagged mobs. */
  for ( it = room_rnums.begin(); it != room_rnums.end(); it++ ) {
    for ( door = 0; door < NUM_EXITS; door++ )
    {
      if( !VIRTUAL_EXIT( *it, door ) )
      {
        continue;
      }
      nearby_room = VIRTUAL_EXIT( *it, door )->to_room;
      if( nearby_room == NOWHERE )
      {
        continue;
      }
      for ( tmp_ch = world[ nearby_room ].people; tmp_ch; tmp_ch = tmp_ch->next_in_room )
      {
        if( IS_NPC( tmp_ch ) )
        {
          SET_BIT( tmp_ch->specials.act2, ACT2_COMBAT_NEARBY );
        }
      }
    }
  }
}

#endif

void double_strike(P_char ch, P_char victim, P_obj wpn)
{
  P_char   tch;
  int      count;
  int      chosen;

  if (!wpn)
    return;

  if (wpn == ch->equipment[PRIMARY_WEAPON])
    wpn = ch->equipment[SECONDARY_WEAPON];
  else
    wpn = ch->equipment[PRIMARY_WEAPON];

  if (!wpn)
    return;

  for (tch = world[ch->in_room].people, count = 0; tch;
      tch = tch->next_in_room)
  {
    if (IS_TRUSTED(tch) || !on_front_line(tch) || tch == ch ||
        !IS_FIGHTING(tch) || tch == victim || (ch->group &&
          ch->group == tch->group))
      continue;
    count++;
  }

  if (count == 0)
    return;

  chosen = number(0, count - 1);

  for (tch = world[ch->in_room].people, count = 0; tch;
      tch = tch->next_in_room)
  {
    if (IS_TRUSTED(tch) || !on_front_line(tch) || tch == ch ||
        !IS_FIGHTING(tch) || tch == victim || (ch->group &&
          ch->group == tch->group))
      continue;
    if (count == chosen)
      break;
    count++;
  }

  if (tch == NULL)
    return;

  act("With a swift turn you lash at $N.", FALSE, ch, 0, tch,
      TO_CHAR | ACT_NOTTERSE);
  act("With a swift turn $n lashes at $N.", FALSE, ch, 0, tch,
      TO_NOTVICT | ACT_NOTTERSE);
  act("With a swift turn $n lashes at you!", FALSE, ch, 0, tch,
      TO_VICT | ACT_NOTTERSE);
  hit(ch, tch, wpn);
}

// This does % maxhealth damage, costs the monk 2 attacks and is not blockable.
bool monk_superhit( P_char ch, P_char victim )
{
  static struct damage_messages monk_superhit_messages = {
    "You hit a pressure point on $N with additional &+rforce&n.",
    "$n's finger drives &+Rdeep&N inside you.",
    "$n pokes $N &+Rreally&n hard, making you wince.",
    "As you reach inside $N, you feel $S life force fade.",
    "$n reaches inside you and things begin to &+wfade &+Rred &+rthen &+Lblack&+W.&N",
    "As $n reaches inside $N, $N collapses."
  };
  double dam;
  int mindam;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  if( !SanityCheck(ch, "monk_superhit") || !SanityCheck(victim, "monk_superhit") )
  {
    return FALSE;
  }

  if( !can_hit_target(ch, victim) )
  {
    send_to_char("Seems that it's too crowded!\r\n", ch);
    return FALSE;
  }

  if( (IS_PC(ch) || IS_PC_PET(ch)) && IS_PC(victim) && !IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE) )
  {
    if( on_front_line(ch) )
    {
      if( !on_front_line(victim) )
      {
        act("$N tries to attack $n but can't quite reach!", TRUE, victim, 0, ch, TO_NOTVICT);
        act("You try to attack $n but can't quite reach!", TRUE, victim, 0, ch, TO_VICT);
        act("$N tries to attack you but can't quite reach!", TRUE, victim, 0, ch, TO_CHAR);
        return FALSE;
      }
    }
    else
    {
      send_to_char("Sorry, you can't quite reach them!\r\n", ch);
      return FALSE;
    }
  }

  // 1/2% max health damage _squared_ (min 10 damage).
  // This equates to 100 (5%) damage to a 2k hp target, or 25 dam (2.5%) to a 1000 hp target.
  dam = GET_MAX_HIT(victim) * .005;
  dam *= dam;
  // We do at least the damage a regular hit would do.
  // Note: This attack cost 2 attacks though, so they do less damage to low hp targets overall.
  mindam = MonkDamage(ch);
  if( dam < mindam )
  {
    dam = mindam;
  }
  // Caps at 225 on a 3k hps target (7.5% max health).
  else if( dam > 225 )
  {
    dam = 225;
  }
  melee_damage( ch, victim, dam, PHSDAM_NOREDUCE, &monk_superhit_messages );

  return TRUE;
}

//
// called once per attack in perform_violence - returns false if the ch dies somehow or another
//
// ch        = attacker
// opponent  = target
// weaponpos = weapon pos used, if -1 nothing is swapped
// canDodge  = self-explanatory
// num_hits  = incremented if opponent did not dodge somehow (as such, it is only somewhat accurate, eh?)
//

int pv_common(P_char ch, P_char opponent, const P_obj wpn)
{
  int i, room, success = FALSE, wpn_skill, spell;
  P_obj item;
  struct proc_data data;

  if( !IS_ALIVE(ch) || !IS_ALIVE(opponent) )
  {
    return FALSE;
  }

  if( !SanityCheck(ch, "pv_common") || !SanityCheck(opponent, "pv_common") )
  {
    return FALSE;
  }

  room = ch->in_room;

  /* weapon skill notch, check for automatic defensive skills */
  if( !((wpn_skill = required_weapon_skill(wpn))
    && ((wpn_skill != SKILL_1H_FLAYING) || (wpn_skill != SKILL_2H_FLAYING))
    && notch_skill(ch, wpn_skill, get_property("skill.notch.offensive.auto", 4)))
    && GET_STAT(opponent) == STAT_NORMAL && !IS_IMMOBILE(ch)
    && (has_innate(ch, INNATE_EYELESS) || CAN_SEE(opponent, ch)
    || GET_CHAR_SKILL(opponent, SKILL_BLINDFIGHTING) / 3 > number(0, 100)))
  {
    if(affected_by_spell(ch, SKILL_SHADOW_MOVEMENT))
    {
    }
    else
    {
      if( !IS_IMMOBILE(opponent)
        && (mangleSucceed( opponent, ch, wpn )
        || parrySucceed( opponent, ch, wpn )
        || divine_blessing_parry( opponent, ch )
        || blockSucceed( opponent, ch, wpn )
        || dodgeSucceed( opponent, ch, wpn )
        || leapSucceed( opponent, ch )
        || MonkRiposte( opponent, ch, wpn )) )
      {
        //justice_witness(ch, opponent, CRIME_ATT_MURDER);
        return FALSE;
      }
    }
  }
  /* defensive hit hook for equipped items - Tharkun */
  for (i = 0; i < sizeof(proccing_slots) / sizeof(int); i++)
  {
    if( (item = opponent->equipment[proccing_slots[i]]) == NULL )
      continue;

    if( IS_PC_PET(opponent) && OBJ_VNUM(item) == 1251 )
      continue;

    if( obj_index[item->R_num].func.obj != NULL )
    {
      data.victim = ch;
      if( (*obj_index[item->R_num].func.obj) (item, opponent, CMD_GOTHIT, (char *) &data) )
      {
        return FALSE;
      }
    }
  }

  /* defensive hit hook for mob procs - Torgal */
  if( IS_ALIVE(opponent) && IS_NPC(opponent) && mob_index[GET_RNUM(opponent)].func.mob
    && !affected_by_spell(opponent, TAG_CONJURED_PET) )
  {
    data.victim = ch;

    if((*mob_index[GET_RNUM(opponent)].func.mob) (opponent, ch, CMD_GOTHIT, (char *) &data))
    {
      return FALSE;
    }
  }

  if(hit(ch, opponent, wpn))
    success = TRUE;

  if(success && IS_ALIVE(opponent) && GET_POS(opponent) == POS_STANDING && GET_CHAR_SKILL(opponent, SKILL_ARMLOCK))
    armlock_check(ch, opponent);

  if(GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) && success && IS_ALIVE(ch))
  {
    int devotion = GET_CHAR_SKILL(ch, SKILL_DEVOTION);
    int chance1 = 0, bonus1 = 0;

    if(IS_NPC(ch) && IS_ELITE(ch) && !IS_PC_PET(ch))
      devotion = GET_LEVEL(ch) * 2;

    if(devotion && devotion > 0)
    {
      if(devotion >= 100)
        bonus1 += 3;
      else if(devotion >= 50)
        bonus1 += 2;
      else
        bonus1 += 1;
    }

    chance1 = (int)(get_property("zealots.memHitChance", 3)) + bonus1;

    //    debug("chance to recover spell is (%d).", chance1);

    if(number(1, 100) <= chance1)
    {
      if(spell = memorize_last_spell(ch))
      {
        char buf[256];
        snprintf(buf, 256, "%s's essence &+Cempowers you&n and you are rewarded with &+G%s!\n",
            get_god_name(ch), skills[spell].name );
        send_to_char(buf, ch);
      }
    }
  }

  if(!is_char_in_room(ch, room))
  {
    return success;
  }
  else if(notch_skill(ch, SKILL_DOUBLE_STRIKE, get_property("skill.notch.offensive.auto", 4))
    || GET_CHAR_SKILL(ch, SKILL_DOUBLE_STRIKE) / 20 > number(0, 100)
    || (affected_by_spell(ch, SKILL_WHIRLWIND) && !number(0, 2)))
  {
    double_strike(ch, opponent, wpn);
  }

  return success;
}

int battle_frenzy(P_char ch, P_char victim)
{
  int      dam;
  struct damage_messages messages1 = {
    "You slam your knee into $N's stomach, winding $M.&N",
    "$n knee's you right in the stomach, knocking the wind out of you.&N",
    "$n knee's $N's stomach, knocking the wind out of $M.&N",
    "As you slam your knee into $N's stomach &+Rblood&n pours from $S mouth.&N",
    "As $n's knee hits your stomach, you spit &+Rblood&n, cough and die.&N",
    "As $n knee's $N's stomach, &+Rblood&n pours from $S mouth.&N"
  };
  struct damage_messages messages2 = {
    "You jam your elbow into $N's side, bruising $M!&N",
    "$n jams $s elbow in your side!&N",
    "$n elbows $N hard, bruising $S side!&N",
    "Your elbow jams into $N's side, breaking $S ribs and crushing $S heart!&n",
    "$n jams $s elbow in your side, breaking your ribs and crushing your heart!&N",
    "$n elbows $N hard, breaking $S ribs and crushing $S heart!&N"
  };

  dam = GET_DAMROLL(ch) * GET_LEVEL(ch) * number(1, 2) / 40;
  if (chance_to_hit(ch, victim, GET_CHAR_SKILL(ch, SKILL_UNARMED_DAMAGE), 0) >
      number(0, 100))
  {
    if (number(0, 1))
    {
      return melee_damage(ch, victim, dam, PHSDAM_TOUCH, &messages1);
    }
    else
    {
      return melee_damage(ch, victim, dam, PHSDAM_TOUCH, &messages2);
    }
  }
  else
  {
    act("$n attempts to knee you right in the stomach, but came up short.&N",
        TRUE, ch, NULL, victim, TO_VICT);
    act("$n tries to knee $N, but can't quite reach.&N", TRUE, ch, NULL,
        victim, TO_NOTVICT);
    act("You attempt to knee $N, but don't quite make it.&N", TRUE, ch, NULL,
        victim, TO_CHAR);
  }
  return 0;
}

void engage(P_char ch, P_char victim)
{
  if( !IS_FIGHTING(ch) && !IS_DESTROYING(ch) )
    set_fighting(ch, victim);

  if( !IS_FIGHTING(victim) && !IS_DESTROYING(victim) )
    set_fighting(victim, ch);
}

bool is_nopoof(P_obj obj)
{
  return IS_ARTIFACT(obj) || isname("quest", obj->name) ||
    isname("fun", obj->name);
}

void DestroyStuff(P_char victim, int type)
{
  int slot, poofed = 0, worn = 0;
  P_obj item;
  int poof_chance = get_property("pvp.eq.poof.chance", 10);
  //  int poof_chance_niceq_multiplier = (int)(get_property("pvp.eq.poof.niceeq.chance.multiplier", 2));

  if(!(victim))
  {
    logit(LOG_EXIT, "destroystuff called in fight.c with no victim");
    raise(SIGSEGV);
  }

  for(slot = 0; slot < sizeof(proccing_slots) / sizeof(int); slot++)
  {
    item = victim->equipment[proccing_slots[slot]];

    if(!item ||
        is_nopoof(item))
    {
      continue;
    }

    //    worn++;

    //poof_chance =
    // MIN((int)
    // (get_property("damage.minPoofChance", 5.) +
    // obj_index[item->R_num].number / 5),
    // (int) get_property("damage.maxPoofChance", 20.));

    // if(IS_SET(item->bitvector, AFF_STONE_SKIN) ||
    // IS_SET(item->bitvector, AFF_HASTE) ||
    // IS_SET(item->bitvector, AFF_AWARE) ||
    // IS_SET(item->bitvector, AFF_HIDE) ||
    // IS_SET(item->bitvector2, AFF2_FIRE_AURA) ||
    // IS_SET(item->bitvector2, AFF2_GLOBE) ||
    // IS_SET(item->bitvector3, AFF3_GR_SPIRIT_WARD) ||
    // IS_SET(item->bitvector4, AFF4_DAZZLER) ||
    // IS_SET(item->bitvector4, AFF4_HELLFIRE) ||
    // IS_SET(item->bitvector4, AFF4_REGENERATION) ||
    // obj_index[item->R_num].func.obj != NULL)
    // {
    // poof_chance = (int)(poof_chance_niceq_multiplier * poof_chance);
    // }

    if(poof_chance > number(1, 100))
    {
      //      poofed++;
      statuslog(AVATAR, "%s [%d] was destroyed.", item->short_description,
          (item->R_num >= 0) ? obj_index[item->R_num].virtual_number : 0);
      DamageOneItem(victim, type, item, TRUE);
    }
  }

  // if(worn)
  // {
  // if(!poofed)
  // {
  // do
  // {
  // slot = number(0, sizeof(proccing_slots) / sizeof(int) - 1);
  // }
  // while (!(item = victim->equipment[proccing_slots[slot]]) ||
  // is_nopoof(item));
  // statuslog(AVATAR, "%s [%d] was destroyed.", item->short_description,
  // (item->R_num >=
  // 0) ? obj_index[item->R_num].virtual_number : 0);
  // DamageOneItem(victim, type, item, TRUE);
  // }
  // }
  // else
  // {
  // statuslog(AVATAR, "%s died with no eq.", victim->player.name);
  // }
}

bool fear_check(P_char ch)
{
  if( !IS_ALIVE(ch) )
  {
    return TRUE;
  }

  if(IS_CONSTRUCT(ch))
    return FALSE;

  if(affected_by_spell(ch, SPELL_INDOMITABILITY))
  {
    act("&+WBeing blessed by protective spirits, you manage to withstand the fear.", FALSE, ch, 0, 0, TO_CHAR);
    act("&+WBeing blessed by protective spirits, $n&+W manages to withstand the fear.", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  if(IS_SET(ch->specials.act, ACT_ELITE) ||
      GET_SPEC(ch, CLASS_WARRIOR, SPEC_GUARDIAN) ||
      IS_AFFECTED4(ch, AFF4_NOFEAR))
  {
    act("&+WYou are simply fearless!", FALSE, ch, 0, 0, TO_CHAR);
    act("&+W$n&+W is simply fearless!", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  if(GET_CHAR_SKILL(ch, SKILL_INDOMITABLE_RAGE) > number(1, 150) &&
      affected_by_spell(ch, SKILL_BERSERK))
  {
    act("&+RWhat, and miss out on the glorious bloodbath? I think not....&n", FALSE, ch, 0, 0, TO_CHAR);
    act("&+R$n&+R snivels derisively for a moment, too caught up in his rage to acknowledge fear itself!&n", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  if(GET_CLASS(ch, CLASS_DREADLORD))
  {
    act("&+LBeing fear incarnate, you laugh derisively at the attempt to scare you into submission!&n", FALSE, ch, 0, 0, TO_CHAR);
    act("&+LThe fearful visage has no affect on $n&+L!&n", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  if(IS_GREATER_RACE(ch) &&
      GET_LEVEL(ch) >= 51)
  {
    act("&+yFear is for the weak willed... which you are not!&n",
        FALSE, ch, 0, 0, TO_CHAR);
    return TRUE;
  }

  return FALSE;
}

bool critical_attack(P_char ch, P_char victim, int msg)
{
  char attacker_msg[MAX_STRING_LENGTH];
  char victim_msg[MAX_STRING_LENGTH];
  char room_msg[MAX_STRING_LENGTH];
  int random;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || GET_RACE(victim) == RACE_CONSTRUCT )
  {
    return FALSE;
  }

  random = number(0, 3);

  if(random == 0)
  {
    if(!IS_HUMANOID(victim))
    { // Falls through.
      random = 2;
    }
    else
    {
      snprintf(attacker_msg, MAX_STRING_LENGTH, "Your attack penetrates $N's defense and strikes to the &+Wbone!&n&N");
      snprintf(victim_msg, MAX_STRING_LENGTH, "$n's attack causes you to gush &+Rblood!&n&N");
      act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);
      snprintf(room_msg, MAX_STRING_LENGTH, "$N's body &+yquivers&n as $n's hit strikes deep!&N");
      act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);

      if(GET_VITALITY(victim) > 20)
      {
        victim->points.vitality = 
          (int) (victim->points.vitality * 0.75);
      }

      return TRUE;
    }
  }
  if(random == 1 && !number(0, 2))
  {
    snprintf(room_msg, MAX_STRING_LENGTH, "$n's mighty %s knocks $N's weapon from $S grasp!&n",
        attack_hit_text[msg].singular);
    snprintf(attacker_msg, MAX_STRING_LENGTH, "Your mighty %s knocks $N's weapon from $S grasp!&n",
        attack_hit_text[msg].singular);
    snprintf(victim_msg, MAX_STRING_LENGTH, "$n's mighty %s knocks your weapon from your grasp!&n",
        attack_hit_text[msg].plural);
    if(critical_disarm(ch, victim))
    {
      act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);
      act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);
      act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);
    }
    else
    {
      return FALSE;
    }

    return TRUE;
  }
  if(random == 2)
  {
    if( affected_by_spell(victim, SPELL_STONE_SKIN) )
    {
      snprintf(attacker_msg, MAX_STRING_LENGTH, "Your mighty attack shatters $N's magical protection!&N");
      act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);

      snprintf(victim_msg, MAX_STRING_LENGTH, "The magic protecting your body shatters as $n %s you!&N",
          attack_hit_text[msg].plural);
      act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);

      snprintf(room_msg, MAX_STRING_LENGTH, "$n grins slightly as their %s drops $N's defenses!&N",
          attack_hit_text[msg].singular);
      act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);

      affect_from_char(victim, SPELL_STONE_SKIN);
    }
    else if( affected_by_spell(victim, SPELL_BIOFEEDBACK) )
    {
      snprintf(attacker_msg, MAX_STRING_LENGTH, "Your mighty attack shatters $N's magical protection!&N");
      act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);

      snprintf(victim_msg, MAX_STRING_LENGTH, "The magic protecting your body shatters as $n %s you!&N",
          attack_hit_text[msg].plural);
      act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);

      snprintf(room_msg, MAX_STRING_LENGTH, "$n grins slightly as their %s drops $N's defenses!&N",
          attack_hit_text[msg].singular);
      act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);

      affect_from_char(victim, SPELL_BIOFEEDBACK);
    }
    else if( affected_by_spell(victim, SPELL_SHADOW_SHIELD) )
    {
      snprintf(attacker_msg, MAX_STRING_LENGTH, "Your mighty attack shatters $N's magical protection!&N");
      act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);

      snprintf(victim_msg, MAX_STRING_LENGTH, "The magic protecting your body shatters as $n %s you!&N",
          attack_hit_text[msg].plural);
      act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);

      snprintf(room_msg, MAX_STRING_LENGTH, "$n grins slightly as $s %s drops $N's defenses!&N",
          attack_hit_text[msg].singular);
      act(room_msg, TRUE, ch, NULL, victim, TO_NOTVICT);

      affect_from_char(victim, SPELL_SHADOW_SHIELD);
    }

    return TRUE;
  }

  if(random == 3)
  {
    snprintf(attacker_msg, MAX_STRING_LENGTH, "You follow up your %s with a surprise attack!&N",
        attack_hit_text[msg].singular);
    act(attacker_msg, TRUE, ch, NULL, victim, TO_CHAR);

    snprintf(victim_msg, MAX_STRING_LENGTH, "$n swiftly follows up his devastating %s with a surprise attack!&N",
        attack_hit_text[msg].singular);
    act(victim_msg, TRUE, ch, NULL, victim, TO_VICT);

    snprintf(room_msg, MAX_STRING_LENGTH, "$n uses the momentum of $s previous strike against $N to land another attack!&N");
    hit(ch, victim, ch->equipment[SECONDARY_WEAPON]);

    return TRUE;

  }
  return FALSE;
}

bool critical_disarm(P_char ch, P_char victim)
{
  if( !ch || !victim )
    return FALSE;

  P_obj obj = NULL;
  int pos = 0;
  int weapon_positions[] = {WIELD, WIELD2, WIELD3, WIELD4, -1};

  for( int i = 0; weapon_positions[i] >= 0; i++ )
  {
    pos = weapon_positions[i];
    obj = victim->equipment[pos];

    if( obj && obj->type == ITEM_WEAPON )
      break;
  }

  if( !obj || obj->type != ITEM_WEAPON || IS_SET(obj->extra_flags, ITEM_NODROP) )
    return FALSE;

  obj = unequip_char(victim, pos);

  if( !IS_ARTIFACT(obj) && number(1,100) < get_property("skill.criticalAttack.disarm.dropChance", 5))
  {
    obj_to_room(obj, victim->in_room);
  }
  else
  {
    obj_to_char(obj, victim);
  }

  return TRUE;
}

double orc_horde_dam_modifier(P_char ch, double dam, int attacking)
{
  P_char horde, next;
  float c = 0.00;

  if (!ch)
    return dam;

  if (ch->in_room < 1)
    return dam;

  for (horde = world[ch->in_room].people; horde; horde = next)
  {
    next = horde->next_in_room;

    if (attacking)
    {
      if (GET_RACE(horde) == RACE_ORC &&
          ch != horde)
        c++;
    }
    else
    {
      if (GET_RACE(horde) == RACE_ORC)
        c--;
    }
  }

  // Remove ourself
  if (!attacking)
    c++;

  c *= get_property("orc.horde.bonus.modifier", 1.5);

  if (attacking)
  {
    return (dam * (1.0+(c/100.00)));
  }
  else
  {
    return (dam * (1.0-(c/100.00)));
  }
}
