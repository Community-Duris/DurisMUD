/*
 * ***************************************************************************
 * *  File: innates.c                                          Part of Duris *
 * *
 * *
 * *    Copyright  2003 - Duris Systems Ltd.
 * *    Author     Tharkun
 * *    Created    2003.04.04
 * *
 * ***************************************************************************
 *
 * to add a new innate define INNATE_* constant for it in structs.h
 * and update innate_date array below. if innate is automatic ie,
 * code just checks for it somewhere with has_innate(ch, INNATE_)
 * put just name and NULL, otherwise give a function which executes
 * innate code. this function must have the same signature as all
 * command functions: it's void func(P_char ch, char *args, int cmd)
 */
#include <signal.h>
#include <string.h>
#include <ctype.h>

#include "db.h"
#include "config.h"
#include "structs.h"
#include "prototypes.h"
#include "interp.h"
#include "damage.h"
#include "comm.h"
#include "events.h"
#include "spells.h"
#include "utils.h"
#include "weather.h"
#include "objmisc.h"
#include "paladins.h"
#include "necromancy.h"
#include "map.h"
#include "guard.h"

#define ADD_RACIAL_INNATE(innate, race, level) (racial_innates[(innate)][(race)] = (level))
#define ADD_CLASS_INNATE(innate, ch_class, level, spec) {(class_innates[(innate)][flag2idx(ch_class)-1][(spec)] = (level));SET_BIT(class_innates_at_all[(innate)], ch_class);}

float regen_factor[REG_MAX + 1];

extern Skill skills[];
extern P_room world;
extern P_event current_event;
extern P_index mob_index;
extern const char *dirs[];
extern int rev_dir[];
extern P_char character_list;
extern struct time_info_data time_info;
char racial_innates[LAST_INNATE + 1][LAST_RACE + 1];
char class_innates[LAST_INNATE + 2][CLASS_COUNT][5];
unsigned int class_innates_at_all[LAST_INNATE + 2];
extern const struct race_names race_names_table[];
extern void event_immolate(P_char, P_char, P_obj, void *);
extern void spell_single_incendiary_cloud(int, P_char, char *, int, P_char, P_obj);
int  cast_as_damage_area(P_char, void (*func) (int, P_char, char *, int, P_char,
                         P_obj), int, P_char, float, float);
extern const int race_hatred_data[][MAX_HATRED];
extern bool epic_summon(P_char, char *);
extern const struct class_names class_names_table[];
extern char *specdata[][MAX_SPEC];
extern int racial_shrug_data[];

void     do_levitate(P_char, char *, int);
void     do_darkness(P_char, char *, int);
void     do_faerie_fire(P_char, char *, int);
void     do_ud_invisibility(P_char, char *, int);
void     do_strength(P_char, char *, int);
void     do_summon_book(P_char, char *, int);
void     do_summon_totem(P_char, char *, int);
void     do_blast(P_char, char *, int);
void     do_shift_astral(P_char, char *, int);
void     do_shift_prime(P_char, char *, int);
void     do_bite(P_char, char *, int);
void     do_enlarge(P_char, char *, int);
void     do_reduce(P_char, char *, int);
void     do_project_image(P_char, char *, int);
void     do_fireball(P_char, char *, int);
void     do_fireshield(P_char, char *, int);
void     do_firestorm(P_char, char *, int);
void     do_plane_shift(P_char, char *, int);
void     do_charm_animal(P_char, char *, int);
void     do_innate_hide(P_char, char *, int);
void     do_dispel_magic(P_char, char *, int);
void     do_globe_of_darkness(P_char, char *, int);
void     do_flurry(P_char, char *, int);
void     do_shapechange(P_char, char *, int);
void     do_throw_lightning(P_char, char *, int);
void     do_stone_skin(P_char, char *, int);
void     do_phantasmal_form(P_char, char *, int);
void     do_shade_movement(P_char, char *, int);
void     do_dimension_door(P_char, char *, int);
void     do_god_call(P_char, char *, int);
void     do_battle_rage(P_char, char *, int);
void     do_mass_dispel(P_char, char *, int);
void     do_disappear(P_char, char *, int);
void     do_dispel_magic(P_char, char *, int);
void     do_conjure_water(P_char, char *, int);
void     do_foundry(P_char, char *, int);
void     do_webwrap(P_char, char *, int);
void     do_summon_imp(P_char, char *, int);
void     do_innate_gaze(P_char, char *, int);
void     do_innate_embrace_death(P_char, char *, int);
void     do_lifedrain(P_char, char *, int);
void     do_immolate(P_char, char *, int);
void     do_summon_warg(P_char, char *, int);
void     do_shift_ethereal(P_char, char *, int);
void     do_fade(P_char, char *, int);

void       do_aura_protection(P_char, char *, int);
void       do_aura_spell_protection(P_char, char *, int);
void       do_aura_precision(P_char, char *, int);
void       do_aura_battlelust(P_char, char *, int);
void       do_aura_endurance(P_char, char *, int);
void       do_aura_healing(P_char, char *, int);
void       do_aura_vigor(P_char, char *, int);
void       do_divine_force(P_char, char *, int);
void       do_wall_climbing(P_char, char *, int);
void       do_list_innates( P_char, char * );
void       class_has_innate( int, int );

int        get_relic_num(P_char ch);
int        fight_in_room(P_char ch);

int        bite_poison(P_char, P_char, int);

bool GOOD_FOR_GAZING(P_char ch, P_char victim);

extern const struct innate_data innates_data[];
const struct innate_data innates_data[LAST_INNATE + 1] =
{
  {
  "horse body", NULL, SKILL_NONE},
  {
  "levitate", do_levitate, SKILL_NONE},
  {
  "darkness", do_darkness, SKILL_NONE},
  {
  "faerie fire", do_faerie_fire, SKILL_NONE},
  {
  "invisibility", do_ud_invisibility, SKILL_NONE},
  {
  "strength", do_strength, SKILL_NONE},
  {
  "doorbash", do_doorbash, SKILL_NONE},
  {
  "infravision", NULL, SKILL_NONE},
  {
  "summon horde", do_summon_orc, SKILL_NONE},
  {
  "ultravision", NULL, SKILL_NONE},
  {
  "outdoor sneak", NULL, SKILL_NONE},
  {
  "bodyslam", do_bodyslam, SKILL_NONE},
  {
  "summon mount", do_summon_mount, SKILL_NONE},
  {
  "anti-good", NULL, SKILL_NONE},
  {
  "anti-evil", NULL, SKILL_NONE},
  {
  "ogre roar", do_ogre_roar, SKILL_NONE},
  {
  "blast", do_blast, SKILL_NONE},
  {
  "ud-sneak", NULL, SKILL_NONE},
  {
  "shift_astral", do_shift_astral, SKILL_NONE},
  {
  "shift_prime", do_shift_prime, SKILL_NONE},
  {
  "vampiric touch", NULL, SKILL_NONE},
  {
  "bite", do_bite, SKILL_NONE},
  {
  "leap", NULL, SKILL_NONE},
  {
  "doorkick", do_doorkick, SKILL_NONE},
  {
  "stampede", do_stampede, SKILL_NONE},
  {
  "charge", do_charge, SKILL_NONE},
  {
  "waterbreath", NULL, SKILL_NONE},
  {
  "enlarge", do_enlarge, SKILL_NONE},
  {
  "regeneration", NULL, SKILL_NONE},
  {
  "reduce", do_reduce, SKILL_NONE},
  {
  "breathe", do_breathe, SKILL_NONE},
  {
  "project image", do_project_image, SKILL_NONE},
  {
  "fireball", do_fireball, SKILL_NONE},
  {
  "fireshield", do_fireshield, SKILL_NONE},
  {
  "firestorm", do_firestorm, SKILL_NONE},
  {
  "protection from fire", NULL, SKILL_NONE},
  {
  "tupor", do_tupor, SKILL_NONE},
  {
  "sneak", NULL, SKILL_NONE},
  {
  "protection from lightning", NULL, SKILL_NONE},
  {
  "plane shift", do_plane_shift, SKILL_NONE},
  {
  "charm animal", do_charm_animal, SKILL_NONE},
  {
  "burrow", do_innate_hide, SKILL_NONE},
  {
  "dispel", do_dispel_magic, SKILL_NONE},
  {
  "globe of darkness", do_globe_of_darkness, SKILL_NONE},
  {
  "mass dispel", do_mass_dispel, SKILL_NONE},
  {
  "disappear", do_disappear, SKILL_NONE},
  {
  "flurry", do_flurry, SKILL_NONE},
  {
  "shapechange", do_shapechange, SKILL_NONE},
  {
  "battle frenzy", NULL, SKILL_NONE},
  {
  "throw lightning", do_throw_lightning, SKILL_NONE},
  {
  "fly", NULL, SKILL_NONE},
  {
  "stone skin", do_stone_skin, SKILL_NONE},
  {
  "phantasmal form", do_phantasmal_form, SKILL_NONE},
  {
  "farsee", NULL, SKILL_NONE},
  {
  "shade movement", do_shade_movement, SKILL_NONE},
  {
  "shadow door", do_dimension_door, SKILL_NONE},
  {
  "god call", do_god_call, SKILL_NONE},
  {
  "forest sight", NULL, SKILL_NONE},
  {
  "battlerage", do_battle_rage, SKILL_NONE},
  {
  "damage spread", NULL, SKILL_NONE},
  {
  "troll skin", NULL, SKILL_NONE},
  {
  "dayvision", NULL, SKILL_NONE},
  {
  "spell absorb", NULL, SKILL_NONE},
  {
  "vulnerable to fire", NULL, SKILL_NONE},
  {
  "vulnerable to cold", NULL, SKILL_NONE},
  {
  "eyeless", NULL, SKILL_NONE},
  {
  "wildmagic", NULL, SKILL_NONE},
  {
  "knight", NULL, SKILL_NONE},
  {
  "sense weakness", NULL, SKILL_NONE},
  {
  "acid blood", SKILL_NONE},
  {
  "conjure water", do_conjure_water, SKILL_NONE},
  {
  "barter", NULL, SKILL_NONE},
  {
  "weapon immunity", NULL, SKILL_NONE},
  {
  "magic resistance", NULL, SKILL_NONE},
  {
  "battlefield aid", NULL, SKILL_NONE},
  {
  "perception", NULL, SKILL_NONE},
  {
  "dayblind", NULL, SKILL_NONE},
  {
  "summon book", do_summon_book, SKILL_NONE},
  {
  "quick thinker", NULL, SKILL_NONE},
  {
  "resurrection", NULL, SKILL_NONE},
  {
  "improved healing", NULL, SKILL_NONE},
  {
  "gamblers luck", NULL, SKILL_NONE},
  {
  "blood scent", NULL, SKILL_NONE},
  {
  "unholy alliance", NULL, SKILL_NONE},
  {
  "mummify", NULL, SKILL_NONE},
  {
  "frightening presence", NULL, SKILL_NONE},
  {
  "blindsinging", NULL, SKILL_NONE},
  {
  "improved flee", NULL, SKILL_NONE}, // Deceiver Spec
  {
  "echo", NULL, SKILL_NONE},
  {
  "branch", do_branch, SKILL_NONE},
  {
  "webwrap", do_webwrap, SKILL_NONE},
  {
  "summon imp", do_summon_imp, SKILL_NONE},
  {
  "hammer master", NULL, SKILL_NONE},
  {
  "axe master", NULL, SKILL_NONE},
  {
  "gaze", do_innate_gaze, SKILL_NONE},
  {
  "embrace death", NULL, SKILL_NONE},
  {
  "drain life", do_lifedrain, SKILL_NONE},
  {
  "pyrokinesis", do_immolate, SKILL_NONE},
  {
  "vulnerable to sun", NULL, SKILL_NONE},
  {
  "decrepify", NULL, SKILL_NONE},
  {
  "groundfighting", NULL, SKILL_GROUNDFIGHTING},
  {
  "bow mastery", NULL, SKILL_NONE},
  {
  "summon warg", do_summon_warg, SKILL_NONE},
  {
  "hatred", NULL, SKILL_NONE},
  {
  "evasion", NULL, SKILL_NONE},
  {
  "mind of the dragon", NULL, SKILL_NONE},
  {
  "shift ethereal", do_shift_ethereal, SKILL_NONE},
  {
  "astral affinity", NULL, SKILL_NONE},
  {
  "two daggers", NULL, SKILL_NONE},
  {
  "holy light", NULL, SKILL_NONE},
  {
  "command aura", NULL, SKILL_NONE},
  {
  "deceptive flee", NULL, SKILL_NONE},
  {"miner", NULL, SKILL_NONE},
  {"foundry", do_foundry, SKILL_NONE},
  {"fade", do_fade, SKILL_NONE},
  {"spacial focus", NULL, SKILL_NONE},
  {"lay hands", do_layhand, SKILL_NONE},
  {"holy crusade", NULL, SKILL_NONE},
  {"magical reduction", NULL, SKILL_NONE},
  {"aura_of_protection", do_aura_protection, SKILL_NONE},
  {"aura_of_precision", do_aura_precision, SKILL_NONE},
  {"aura_of_battlelust", do_aura_battlelust, SKILL_NONE},
  {"aura_of_endurance", do_aura_endurance, SKILL_NONE},
  {"aura_of_improved_healing", do_aura_healing, SKILL_NONE},
  {"aura_of_vigor", do_aura_vigor, SKILL_NONE},
  {"speedy", NULL, SKILL_NONE},
  {"dauntless", NULL, SKILL_NONE},
  {"summon totem", do_summon_totem, SKILL_NONE},
  {"entrapment", NULL, SKILL_NONE},
  {"protection from cold", NULL, SKILL_NONE},
  {"protection from acid", NULL, SKILL_NONE},
  {"fire aura", NULL, SKILL_NONE},
  {"spawn", do_spawn, SKILL_NONE},
  {"warcallers fury", NULL, SKILL_NONE},
  {"spirit of the rrakkma", NULL, SKILL_NONE},
  {"diseased bite", NULL, SKILL_NONE},
  {"divine force", do_divine_force, SKILL_NONE},
  {"undead fealty", NULL, SKILL_NONE},
  {"call of the grave", do_call_grave, SKILL_NONE},
  {"sacrilegious power", NULL, SKILL_NONE}, // Vampire innate
  {"blur", NULL, SKILL_NONE}, // Not passive, but no supporting func anymore.
  {"rapier and dirk", NULL, SKILL_NONE}, // Swashbuckler
  {"elemental body", NULL, SKILL_NONE},
  {"amorphous body", NULL, SKILL_NONE},
  {"engulf", do_engulf, SKILL_NONE},
  {"slime", do_slime, SKILL_NONE},
  {"dual wielding master", NULL, SKILL_NONE},
  {"speed", NULL, SKILL_NONE},
  {"ice aura", NULL, SKILL_NONE},
  {"requiem", NULL, SKILL_NONE},
  {"ally", do_spawn, SKILL_NONE},
  {"summon host", do_summon_host, SKILL_NONE},
  {"spider body", NULL, SKILL_NONE},
  {"swamp sneak", NULL, SKILL_NONE},
  {"calming", NULL, SKILL_NONE},
  {"longsword master", NULL, SKILL_NONE},
  {"melee mastery", NULL, SKILL_NONE},
  {"bulwark", NULL, SKILL_NONE},
  {"wall climbing", NULL, SKILL_NONE},
  {"woodland renewal", NULL, SKILL_NONE},
  {"natural movement", NULL, SKILL_NONE},
  {"magic vulnerability", NULL, SKILL_NONE},
  {"two-handed sword mastery", NULL, SKILL_NONE},
  {"holy combat", NULL, SKILL_NONE},
  {"giant avoidance", NULL, SKILL_NONE},
  {"seadog", NULL, SKILL_NONE},
  {"aura_of_spell_protection", do_aura_spell_protection, SKILL_NONE},
  {"vision of the dead", NULL, SKILL_NONE},
  {"remort", do_remort, SKILL_NONE},
  {"elemental power", NULL, SKILL_NONE},
  {"intercept", NULL, SKILL_NONE},
  {"detect subversion", NULL, SKILL_NONE},
  {"living stone", NULL, SKILL_NONE},
  {"invisibility", NULL, SKILL_NONE }
};

string list_innates(int race, int cls, int spec)
{
  string return_str;
  char level[5];
  int innate, found = 0;

  if (!race && !cls)
  {
    debug("list_innates called with no race or class.");
    return return_str;
  }
  for (innate = 0; innate <= LAST_INNATE; innate++)
  {
    if ((race && racial_innates[innate][race]) ||
	(cls && class_innates[innate][cls - 1][spec]))
    {
      found = 1;
      if (innates_data[innate].func)
	return_str += " ";
      else
	return_str += "*";

      return_str += "&+c";
      return_str += string(innates_data[innate].name);
      if ((race && racial_innates[innate][race] > 1) ||
	  (cls && class_innates[innate][cls - 1][spec] > 1))
      {
        return_str += " &n(obtained at level &+c";
        if (race)
	  snprintf(level, 5, "%d", racial_innates[innate][race]);
	else if (cls)
          snprintf(level, 5, "%d", class_innates[innate][cls - 1][spec]);
	return_str += string(level);
	return_str += "&n)";
      }
      return_str += "&n\n";
    }
  }
  if (!found)
  {
    return_str += "&nNone.\n";
  }
  else
    return_str += "'*' Designates passive ability.\n";
  
  return return_str;
}

bool has_innate(P_char ch, int innate)
{
  int      i, race;
  struct affected_type *af, *af2;

  if (class_innates_at_all[innate] & ch->player.m_class)
    for (i = 0; i < CLASS_COUNT; i++)
      if (GET_CLASS(ch, 1 << i))
        if (class_innates[innate][i][0] &&
            GET_LEVEL(ch) >= class_innates[innate][i][0])
          return TRUE;
        else
          if (((IS_SPECIALIZED(ch) && class_innates[innate][i][ch->player.spec]))
              && GET_LEVEL(ch) >= class_innates[innate][i][ch->player.spec])
          return TRUE;

  race = ch->player.race;

  if (IS_DISGUISE_SHAPE(ch))
  {
    race = ch->disguise.race;
  }
  else if ((af = get_spell_from_char(ch, TAG_RACE_CHANGE)) != NULL)
  {
    if ((af2 = get_spell_from_char(ch, SPELL_CORPSEFORM)) != NULL)
    {
      if (af2->modifier == CORPSEFORM_REG)
      {
        race = af->modifier;
      }
    }
  }

  if( race < 0 || race > LAST_RACE )
  {
    logit(LOG_DEBUG, "Invalid race (%d) for %s", race, GET_NAME(ch));
    debug("Invalid race (%d) for %s", race, GET_NAME(ch));
    return FALSE;
  }

  if (racial_innates[innate][race])
    return GET_LEVEL(ch) >= racial_innates[innate][race];
  else
    return FALSE;
}

void assign_innates()
{
  memset(racial_innates, 0, sizeof(racial_innates));
  memset(class_innates, 0, sizeof(class_innates));
  memset(class_innates_at_all, 0, sizeof(class_innates_at_all));

  /* Racial Innates, Lets split this alphanumerically, and then by racewar */
  /* Good Races */
  /* List of Agathinon Innates      */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_AGATHINON, 1);
  ADD_RACIAL_INNATE(INNATE_DIVINE_FORCE, RACE_AGATHINON, 1);
  /* List of Barbarian Innates      */
  //ADD_RACIAL_INNATE(INNATE_BARB_BRATH, RACE_BARBARIAN, 11);
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_BARBARIAN, 1);
  ADD_RACIAL_INNATE(INNATE_DAUNTLESS, RACE_BARBARIAN, 41);
  ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_BARBARIAN, 1);
  ADD_RACIAL_INNATE(INNATE_GROUNDFIGHTING, RACE_BARBARIAN, 21);
  ADD_RACIAL_INNATE(INNATE_PROT_COLD, RACE_BARBARIAN, 31);
  /* List of Centaur Innates        */
  ADD_RACIAL_INNATE(INNATE_HORSE_BODY, RACE_CENTAUR, 1);
  ADD_RACIAL_INNATE(INNATE_DOORKICK, RACE_CENTAUR, 1);
  ADD_RACIAL_INNATE(INNATE_STAMPEDE, RACE_CENTAUR, 21);
  ADD_RACIAL_INNATE(TWO_HANDED_SWORD_MASTERY, RACE_CENTAUR, 31);
  /* List of Mountain Dwarf Innates */
  ADD_RACIAL_INNATE(INNATE_STRENGTH, RACE_MOUNTAIN, 1);
  ADD_RACIAL_INNATE(INNATE_GIANT_AVOIDANCE, RACE_MOUNTAIN, 1);
  ADD_RACIAL_INNATE(INNATE_INFRAVISION, RACE_MOUNTAIN, 1);
  ADD_RACIAL_INNATE(INNATE_AXE_MASTER, RACE_MOUNTAIN, 11);
  ADD_RACIAL_INNATE(INNATE_HAMMER_MASTER, RACE_MOUNTAIN, 31);
  ADD_RACIAL_INNATE(INNATE_HATRED, RACE_MOUNTAIN, 21);
  ADD_RACIAL_INNATE(MAGICAL_REDUCTION , RACE_MOUNTAIN, 1);
  //ADD_RACIAL_INNATE(INNATE_MINER, RACE_MOUNTAIN, 46);
  /* List of Eladrin Innates        */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_ELADRIN, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_ELADRIN, 1);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_ELADRIN, 1);
  ADD_RACIAL_INNATE(INNATE_UNDEAD_FEALTY, RACE_ELADRIN, 30);
  ADD_RACIAL_INNATE(INNATE_SPELL_ABSORB, RACE_ELADRIN, 21);
  ADD_RACIAL_INNATE(INNATE_PROT_COLD, RACE_ELADRIN, 1);
  /* List of Firbolg Innates        */
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_FIRBOLG, 1);
  ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_FIRBOLG, 1);
  ADD_RACIAL_INNATE(INNATE_FOREST_SIGHT, RACE_FIRBOLG, 1);
  ADD_RACIAL_INNATE(MAGIC_VULNERABILITY, RACE_FIRBOLG, 1);
  /* List of Tiefling Innates -- Eikel */
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_TIEFLING, 1);
  ADD_RACIAL_INNATE(INNATE_INFRAVISION, RACE_TIEFLING, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_TIEFLING, 1);
  ADD_RACIAL_INNATE(INNATE_PROT_FIRE, RACE_TIEFLING, 1);
  /* List of Githzerai Innates      */
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_GITHZERAI, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_GITHZERAI, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_GITHZERAI, 1);
  ADD_RACIAL_INNATE(INNATE_LEVITATE, RACE_GITHZERAI, 11);
  ADD_RACIAL_INNATE(INNATE_RRAKKMA, RACE_GITHZERAI, 21);
  ADD_RACIAL_INNATE(INNATE_SHIFT_PRIME, RACE_GITHZERAI, 1);
  ADD_RACIAL_INNATE(INNATE_SHIFT_ASTRAL, RACE_GITHZERAI, 45); 
  /* List of Gnome Innates          */
  ADD_RACIAL_INNATE(INNATE_INFRAVISION, RACE_GNOME, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_GNOME, 1);
  ADD_RACIAL_INNATE(INNATE_FARSEE, RACE_GNOME, 21);
  //ADD_RACIAL_INNATE(INNATE_MINER, RACE_GNOME, 51);
  /* List of Grey Elf Innates       */
  ADD_RACIAL_INNATE(INNATE_INFRAVISION, RACE_GREY, 1);
  ADD_RACIAL_INNATE(INNATE_OUTDOOR_SNEAK, RACE_GREY, 21);
  ADD_RACIAL_INNATE(INNATE_FOREST_SIGHT, RACE_GREY, 11);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_GREY, 1);
  ADD_RACIAL_INNATE(INNATE_LONGSWORD_MASTER, RACE_GREY, 11);
  /* List of Half-Elf Innates       */
  ADD_RACIAL_INNATE(INNATE_INFRAVISION, RACE_HALFELF, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_HALFELF, 1);
  ADD_RACIAL_INNATE(INNATE_QUICK_THINKING, RACE_HALFELF, 30);
  ADD_RACIAL_INNATE(INNATE_LONGSWORD_MASTER, RACE_HALFELF, 11);
  /* List of Halfling Innates       */
  ADD_RACIAL_INNATE(INNATE_HIDE, RACE_HALFLING, 1);
  ADD_RACIAL_INNATE(INNATE_PERCEPTION, RACE_HALFLING, 11);
  //ADD_RACIAL_INNATE(INNATE_FLURRY, RACE_HALFLING, 50);  
  //ADD_RACIAL_INNATE(INNATE_BARTER, RACE_HALFLING, 1);
  ADD_RACIAL_INNATE(INNATE_QUICK_THINKING, RACE_HALFLING, 1);
  /* List of Human Innates          */
  ADD_RACIAL_INNATE(INNATE_SEADOG, RACE_HUMAN, 1);
  /* List of Wood Elf Innates       */
  ADD_RACIAL_INNATE(INNATE_OUTDOOR_SNEAK, RACE_WOODELF, 11);
  ADD_RACIAL_INNATE(INNATE_FOREST_SIGHT, RACE_WOODELF, 11);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_WOODELF, 1);
  ADD_RACIAL_INNATE(INNATE_LONGSWORD_MASTER, RACE_WOODELF, 11);

  /* Evil Races */
  /* List of Drider Innates         */
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_DRIDER, 1);
  //ADD_RACIAL_INNATE(INNATE_WEBWRAP, RACE_DRIDER, 1);
  ADD_RACIAL_INNATE(INNATE_SPIDER_BODY, RACE_DRIDER, 1);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_DRIDER, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_DRIDER, 1);
  /* List of Drow Elf Innates       */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_DROW, 1);
  ADD_RACIAL_INNATE(INNATE_LEVITATE, RACE_DROW, 11);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_DROW, 1);
  ADD_RACIAL_INNATE(INNATE_FAERIE_FIRE, RACE_DROW, 1);
  //ADD_RACIAL_INNATE(INNATE_DARKNESS, RACE_DROW, 26);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_DROW, 1);
  ADD_RACIAL_INNATE(INNATE_GLOBE_OF_DARKNESS, RACE_DROW, 26);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_DROW, 1);
  ADD_RACIAL_INNATE(INNATE_LONGSWORD_MASTER, RACE_DROW, 11);
//  ADD_RACIAL_INNATE(INNATE_MASS_DISPEL, RACE_DROW, 53);
//  ADD_RACIAL_INNATE(INNATE_FIREBALL, RACE_DROW, 21);
  /* List of Duergar Dwarf Innates   */
  ADD_RACIAL_INNATE(MAGICAL_REDUCTION , RACE_DUERGAR, 1);
  ADD_RACIAL_INNATE(INNATE_STRENGTH, RACE_DUERGAR, 6);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_DUERGAR, 1);
  ADD_RACIAL_INNATE(INNATE_UD_INVISIBILITY, RACE_DUERGAR, 1);
  ADD_RACIAL_INNATE(INNATE_UD_SNEAK, RACE_DUERGAR, 31);
  ADD_RACIAL_INNATE(INNATE_ENLARGE, RACE_DUERGAR, 26);
  ADD_RACIAL_INNATE(INNATE_BATTLE_FRENZY, RACE_DUERGAR, 16);
  ADD_RACIAL_INNATE(INNATE_BATTLE_RAGE, RACE_DUERGAR, 31);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_DUERGAR, 1);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_DUERGAR, 1);
  //ADD_RACIAL_INNATE(INNATE_MINER, RACE_DUERGAR, 46);
  /* List of Goblin Innates          */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_GOBLIN, 1);
  ADD_RACIAL_INNATE(INNATE_DISAPPEAR, RACE_GOBLIN, 31);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_GOBLIN, 1);
  ADD_RACIAL_INNATE(INNATE_SUMMON_WARG, RACE_ORC, 16);
  ADD_RACIAL_INNATE(INNATE_SUMMON_TOTEM, RACE_GOBLIN, 26);
  //ADD_RACIAL_INNATE(INNATE_FLURRY, RACE_GOBLIN, 36);
  /* List of Kobold Innates                  */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_KOBOLD, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_KOBOLD, 1);
  //ADD_RACIAL_INNATE(INNATE_MINER, RACE_KOBOLD, 51);
  ADD_RACIAL_INNATE(INNATE_CALMING, RACE_KOBOLD, 1);
  ADD_RACIAL_INNATE(INNATE_UD_SNEAK, RACE_KOBOLD, 1);
  /* List of Kuo Toa Innates         */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_KUOTOA, 1);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_KUOTOA, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_KUOTOA, 1);
  ADD_RACIAL_INNATE(INNATE_THROW_LIGHTNING, RACE_KUOTOA, 30);
  ADD_RACIAL_INNATE(INNATE_WATERBREATH, RACE_KUOTOA, 15);
  ADD_RACIAL_INNATE(INNATE_SWAMP_SNEAK, RACE_KUOTOA, 1);
  /* List of Githyanki Innates       */
  ADD_RACIAL_INNATE(INNATE_SHIFT_ASTRAL, RACE_GITHYANKI, 1);
  ADD_RACIAL_INNATE(INNATE_SHIFT_PRIME, RACE_GITHYANKI, 1);
  ADD_RACIAL_INNATE(INNATE_LEVITATE, RACE_GITHYANKI, 11);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_GITHYANKI, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_GITHYANKI, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_GITHYANKI, 1);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_GITHYANKI, 1); 
  /* List of (P)Illithid Innate      */
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_PILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_PILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_LEVITATE, RACE_PILLITHID, 10);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_PILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_PILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_SHIFT_ASTRAL, RACE_PILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_SHIFT_PRIME, RACE_PILLITHID, 1);
  /* List of (P)Lich Innates*/
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_LICH, 1);
  ADD_RACIAL_INNATE(INNATE_SPELL_ABSORB, RACE_LICH, 21);
  //ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_LICH, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_LICH, 1);
  //ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_LICH, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_LICH, 1);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_LICH, 1);
  ADD_RACIAL_INNATE(INNATE_PROT_COLD, RACE_LICH, 1);
  ADD_RACIAL_INNATE(INNATE_CALL_GRAVE, RACE_LICH, 1);
  ADD_RACIAL_INNATE(INNATE_UNDEAD_FEALTY, RACE_LICH, 1);
  /* List of Ogre Innates            */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_OGREROAR, RACE_OGRE, 11);
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_OGRE, 1);
  ADD_RACIAL_INNATE(MAGIC_VULNERABILITY, RACE_OGRE, 1);
  /* List of Orc Innates             */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_ORC, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_ORC, 1);
  ADD_RACIAL_INNATE(INNATE_SUMMON_HORDE, RACE_ORC, 11);
  ADD_RACIAL_INNATE(INNATE_SEADOG, RACE_ORC, 1);
  /* List of Swamp Troll Innates     */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_TROLL, 1);
  ADD_RACIAL_INNATE(INNATE_REGENERATION, RACE_TROLL, 1);
  ADD_RACIAL_INNATE(INNATE_TROLL_SKIN, RACE_TROLL, 21);
  ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_TROLL, 1);
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_TROLL, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_TROLL, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_TROLL, 1);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_TROLL, 1);
  ADD_RACIAL_INNATE(INNATE_SWAMP_SNEAK, RACE_TROLL, 11);
  /* List of (P)Vampire Innates */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_PVAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_BITE, RACE_PVAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_VAMPIRIC_TOUCH, RACE_PVAMPIRE, 1);
  //ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_PVAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_PVAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_PVAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_SACRILEGIOUS_POWER, RACE_PVAMPIRE, 46);

  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_SKELETON, 1);

  /* Neutral Races */
 
  /* List of Minotaur Innates        */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_MINOTAUR, 1);
  ADD_RACIAL_INNATE(INNATE_CHARGE, RACE_MINOTAUR, 11);
  ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_MINOTAUR, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_MINOTAUR, 1);
  /* List of ThriKreen Innates       */
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_THRIKREEN, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_THRIKREEN, 1);
  ADD_RACIAL_INNATE(INNATE_LEAP, RACE_THRIKREEN, 21);
  ADD_RACIAL_INNATE(INNATE_BITE, RACE_THRIKREEN, 11);
  ADD_RACIAL_INNATE(INNATE_VULN_COLD, RACE_THRIKREEN, 1);
  
  /* NPC Races */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_OROG, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_OROG, 1);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_OROG, 1);
  ADD_RACIAL_INNATE(INNATE_WARCALLERS_FURY, RACE_OROG, 21);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_SNOW_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_SNOW_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_OGREROAR, RACE_SNOW_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_SNOW_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_PROT_COLD, RACE_SNOW_OGRE, 1);
  ADD_RACIAL_INNATE(INNATE_REGENERATION, RACE_DRAGONKIN, 1);
  ADD_RACIAL_INNATE(INNATE_REGENERATION, RACE_DRAGON, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_GARGOYLE, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_ILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_LEVITATE, RACE_ILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_BLAST, RACE_ILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_SHIFT_ASTRAL, RACE_ILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_SHIFT_PRIME, RACE_ILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_ILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_ILLITHID, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_SHADE, 1);
  ADD_RACIAL_INNATE(INNATE_SHADE_MOVEMENT, RACE_SHADE, 21);
  ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_SHADE, 1);
  ADD_RACIAL_INNATE(INNATE_DARKNESS, RACE_SHADE, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_REVENANT, 1);
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_REVENANT, 1);
  ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_REVENANT, 1);
  ADD_RACIAL_INNATE(INNATE_SHADOW_DOOR, RACE_REVENANT, 25);
  ADD_RACIAL_INNATE(INNATE_REGENERATION, RACE_REVENANT, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_REVENANT, 1);
  //ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_DRAGONKIN, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_PDKNIGHT, 1);
  ADD_RACIAL_INNATE(INNATE_FIRESHIELD, RACE_PDKNIGHT, 31);
  ADD_RACIAL_INNATE(INNATE_FIRESHIELD, RACE_F_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_FIRE_AURA, RACE_F_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_FIRESTORM, RACE_PDKNIGHT, 26);
  ADD_RACIAL_INNATE(INNATE_PROT_FIRE, RACE_PDKNIGHT, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_PDKNIGHT, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_VAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_BITE, RACE_VAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_VAMPIRIC_TOUCH, RACE_VAMPIRE, 1);
  //ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_VAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_VAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_SUN, RACE_VAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_SACRILEGIOUS_POWER, RACE_VAMPIRE, 46);
  ADD_RACIAL_INNATE(INNATE_GAZE, RACE_VAMPIRE, 1);
  ADD_RACIAL_INNATE(INNATE_GAZE, RACE_BRALANI, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_PSBEAST, 1);
  ADD_RACIAL_INNATE(INNATE_STRENGTH, RACE_PSBEAST, 1);
  ADD_RACIAL_INNATE(INNATE_UD_SNEAK, RACE_PSBEAST, 1);
  ADD_RACIAL_INNATE(INNATE_ENLARGE, RACE_PSBEAST, 1);
  ADD_RACIAL_INNATE(INNATE_PROJECT_IMAGE, RACE_PSBEAST, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_PSBEAST, 1);
  //ADD_RACIAL_INNATE(INNATE_INFRAVISION, RACE_SGIANT, 1);
  //ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_SGIANT, 1);
  //ADD_RACIAL_INNATE(INNATE_THROW_LIGHTNING, RACE_SGIANT, 20);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_WIGHT, 1);
  ADD_RACIAL_INNATE(INNATE_DOORBASH, RACE_WIGHT, 1);
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_WIGHT, 1);
  ADD_RACIAL_INNATE(INNATE_STONE, RACE_WIGHT, 25);
  ADD_RACIAL_INNATE(INNATE_STONE, RACE_E_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_WIGHT, 1);
  //ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_WIGHT, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_PHANTOM, 1);
  ADD_RACIAL_INNATE(INNATE_SHIFT_PRIME, RACE_PHANTOM, 1);
  ADD_RACIAL_INNATE(INNATE_SHIFT_ASTRAL, RACE_PHANTOM, 1);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_PHANTOM, 21);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_WRAITH, 21);
  ADD_RACIAL_INNATE(INNATE_PHANTASMAL_FORM, RACE_PHANTOM, 20);
  ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_PHANTOM, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_PHANTOM, 1);
  //ADD_RACIAL_INNATE(INNATE_DAYBLIND, RACE_PHANTOM, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_HARPY, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_HARPY, 1);
  ADD_RACIAL_INNATE(INNATE_PERCEPTION, RACE_HARPY, 1);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_HARPY, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_DRAGONKIN, 1);
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_GARGOYLE, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_GARGOYLE, 1);
  ADD_RACIAL_INNATE(INNATE_PERCEPTION, RACE_GARGOYLE, 1);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_GARGOYLE, 1);
  ADD_RACIAL_INNATE(INNATE_HORSE_BODY, RACE_QUADRUPED, 1);
  ADD_RACIAL_INNATE(INNATE_DOORKICK, RACE_QUADRUPED, 1);
  ADD_RACIAL_INNATE(INNATE_STAMPEDE, RACE_QUADRUPED, 26);
  ADD_RACIAL_INNATE(INNATE_WILDMAGIC, RACE_FAERIE, 41);
  ADD_RACIAL_INNATE(INNATE_WATERBREATH, RACE_AQUATIC_ANIMAL, 1);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_FLYING_ANIMAL, 1);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_ANGEL, 21);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_FAERIE, 21);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_EFREET, 1);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_DRAGON, 31);
  ADD_RACIAL_INNATE(INNATE_PROT_FIRE, RACE_EFREET, 1);
  ADD_RACIAL_INNATE(INNATE_FIRE_AURA, RACE_EFREET, 1);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_PLANT, 1);
  ADD_RACIAL_INNATE(INNATE_BRANCH, RACE_PLANT, 1);
  //ADD_RACIAL_INNATE(INNATE_SLIME, RACE_SLIME, 1);
  //ADD_RACIAL_INNATE(INNATE_ENGULF, RACE_SLIME, 21);
  ADD_RACIAL_INNATE(INNATE_AMORPHOUS_BODY, RACE_SLIME, 1);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_SLIME, 1);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_GOLEM, 1);
  ADD_RACIAL_INNATE(INNATE_WEAPON_IMMUNITY, RACE_GOLEM, 40);
  ADD_RACIAL_INNATE(INNATE_WEAPON_IMMUNITY, RACE_DEVIL, 40);
  ADD_RACIAL_INNATE(INNATE_WEAPON_IMMUNITY, RACE_DEMON, 20);
  ADD_RACIAL_INNATE(INNATE_WEAPON_IMMUNITY, RACE_DRAGON, 56);
  ADD_RACIAL_INNATE(INNATE_WEAPON_IMMUNITY, RACE_ANGEL, 51);
  ADD_RACIAL_INNATE(INNATE_WEAPON_IMMUNITY, RACE_GHOST, 51);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_ANGEL, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_DRAGON, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_DRAGONKIN, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_DEVIL, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_DRACOLICH, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_UNDEAD, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_BEHOLDER, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_BEHOLDERKIN, 51);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_DEMON, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_A_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_E_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_W_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_F_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_V_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_I_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_ICE_AURA, RACE_I_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_ILLITHID, 1);  
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_WRAITH, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_SHADOW, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_DEVA, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_SPECTRE, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_BRALANI, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_ASURA, 1);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_TITAN, 1);
  ADD_RACIAL_INNATE(INNATE_HASTE, RACE_TITAN, 51);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_TITAN, 51);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_AVATAR, 1);
  ADD_RACIAL_INNATE(INNATE_HASTE, RACE_AVATAR, 51);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_AVATAR, 51);
  ADD_RACIAL_INNATE(INNATE_VULN_FIRE, RACE_PLANT, 1);
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_GIANT, 1);
  ADD_RACIAL_INNATE(INNATE_BITE, RACE_SNAKE, 1);
  ADD_RACIAL_INNATE(INNATE_BITE, RACE_INSECT, 1);
  ADD_RACIAL_INNATE(INNATE_BITE, RACE_ARACHNID, 1);
  ADD_RACIAL_INNATE(INNATE_WEBWRAP, RACE_ARACHNID, 25);
  ADD_RACIAL_INNATE(INNATE_IMMOLATE, RACE_F_ELEMENTAL, 25);
  ADD_RACIAL_INNATE(INNATE_TWO_DAGGERS, RACE_RAKSHASA, 1);
  ADD_RACIAL_INNATE(INNATE_GAMBLERS_LUCK, RACE_HALFLING, 26);
  ADD_RACIAL_INNATE(INNATE_BITE, RACE_PARASITE, 1);
  ADD_RACIAL_INNATE(INNATE_DISEASED_BITE, RACE_PARASITE, 26);
  ADD_RACIAL_INNATE(INNATE_HASTE, RACE_DRAGON, 46);
  ADD_RACIAL_INNATE(INNATE_BLUR, RACE_DRAGON, 56);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_DRACOLICH, 1);
  ADD_RACIAL_INNATE(INNATE_HASTE, RACE_DRACOLICH, 51);
  ADD_RACIAL_INNATE(INNATE_INVISIBILITY, RACE_A_ELEMENTAL, 1);

/* Demons and devils */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_DEMON, 1);
  ADD_RACIAL_INNATE(INNATE_BODYSLAM, RACE_DEMON, 1);
  ADD_RACIAL_INNATE(INNATE_REGENERATION, RACE_DEMON, 1);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_DEMON, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_DEMON, 1);
  ADD_RACIAL_INNATE(INNATE_HASTE, RACE_DEMON, 41);
  
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_DEVIL, 1);  
  ADD_RACIAL_INNATE(INNATE_ACID_BLOOD, RACE_DEVIL, 1);
  ADD_RACIAL_INNATE(INNATE_REGENERATION, RACE_DEVIL, 1);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_DEVIL, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_DEVIL, 1);
  ADD_RACIAL_INNATE(INNATE_SUMMON_IMP, RACE_DEVIL, 35);
  ADD_RACIAL_INNATE(INNATE_HASTE, RACE_DEVIL, 41);
  ADD_RACIAL_INNATE(INNATE_INFERNAL_FURY, RACE_DEVIL, 56);
/* Succubus, SubCategory: Devil */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_SUCCUBUS, 1);
  ADD_RACIAL_INNATE(INNATE_ACID_BLOOD, RACE_SUCCUBUS, 21);
  ADD_RACIAL_INNATE(INNATE_REGENERATION, RACE_SUCCUBUS, 31);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_SUCCUBUS, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_SUCCUBUS, 21);
  ADD_RACIAL_INNATE(INNATE_HASTE, RACE_SUCCUBUS, 51);
/* Incubus, SubCategory: Devil */
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_SUCCUBUS, 1);
  ADD_RACIAL_INNATE(INNATE_ACID_BLOOD, RACE_SUCCUBUS, 21);
  ADD_RACIAL_INNATE(INNATE_REGENERATION, RACE_SUCCUBUS, 31);
  ADD_RACIAL_INNATE(INNATE_FLY, RACE_SUCCUBUS, 1);
  ADD_RACIAL_INNATE(INNATE_DAYVISION, RACE_SUCCUBUS, 21);
  ADD_RACIAL_INNATE(INNATE_HASTE, RACE_SUCCUBUS, 51);
/* END DEVILS AND DEMONS */

  ADD_RACIAL_INNATE(INNATE_ELEMENTAL_BODY, RACE_A_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_ELEMENTAL_BODY, RACE_E_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_ELEMENTAL_BODY, RACE_W_ELEMENTAL, 1);
  ADD_RACIAL_INNATE(INNATE_ELEMENTAL_BODY, RACE_F_ELEMENTAL, 1);

 


  /* List of Ghaele Innates*/
  ADD_RACIAL_INNATE(INNATE_ULTRAVISION, RACE_GHAELE, 1);
  ADD_RACIAL_INNATE(INNATE_SPELL_ABSORB, RACE_GHAELE, 21);
  ADD_RACIAL_INNATE(INNATE_MAGIC_RESISTANCE, RACE_GHAELE, 1);
  ADD_RACIAL_INNATE(INNATE_PROT_COLD, RACE_GHAELE, 1);
  ADD_RACIAL_INNATE(INNATE_UNDEAD_FEALTY, RACE_GHAELE, 1);
  ADD_RACIAL_INNATE(INNATE_VAMPIRIC_TOUCH, RACE_GHAELE, 1);
  ADD_RACIAL_INNATE(INNATE_SACRILEGIOUS_POWER, RACE_GHAELE, 46);
  ADD_RACIAL_INNATE(INNATE_EYELESS, RACE_GHAELE, 1); 
  
  /* class innates */

  ADD_CLASS_INNATE(INNATE_WOODLAND_RENEWAL, CLASS_RANGER, 30, SPEC_HUNTSMAN); 
  ADD_CLASS_INNATE(INNATE_NATURAL_MOVEMENT, CLASS_RANGER, 46, SPEC_HUNTSMAN);
  
  ADD_CLASS_INNATE(INNATE_RAPIER_DIRK, CLASS_WARRIOR, 1, SPEC_SWASHBUCKLER);
  ADD_CLASS_INNATE(INNATE_RAPIER_DIRK, CLASS_MERCENARY, 1, 0);
  ADD_CLASS_INNATE(INNATE_RAPIER_DIRK, CLASS_ROGUE, 1, SPEC_THIEF);
// Bards need an instrucment, so they wont ever dual, removing this - gellz
//  ADD_CLASS_INNATE(INNATE_RAPIER_DIRK, CLASS_BARD, 1, 0);
  ADD_CLASS_INNATE(INNATE_MELEE_MASTER, CLASS_WARRIOR, 1, SPEC_SWORDSMAN);

  ADD_CLASS_INNATE(INNATE_TWO_DAGGERS, CLASS_ROGUE, 1, SPEC_ASSASSIN);

  ADD_CLASS_INNATE(INNATE_SEADOG, CLASS_WARRIOR, 30, SPEC_SWASHBUCKLER);

  ADD_CLASS_INNATE(INNATE_DET_SUBVERSION, CLASS_MERCENARY, 30, SPEC_BOUNTY);

  ADD_CLASS_INNATE(INNATE_GUARDIANS_BULWARK, CLASS_WARRIOR, 41, SPEC_GUARDIAN);

 // ADD_CLASS_INNATE(INNATE_MINER, CLASS_ALCHEMIST, 36, SPEC_BLACKSMITH);
  ADD_CLASS_INNATE(INNATE_FOUNDRY, CLASS_ALCHEMIST, 41, SPEC_BLACKSMITH);
  ADD_CLASS_INNATE(INNATE_EVASION, CLASS_MONK, 36, SPEC_WAYOFDRAGON);
  ADD_CLASS_INNATE(INNATE_EVASION, CLASS_MONK, 36, SPEC_WAYOFSNAKE);
//  ADD_CLASS_INNATE(INNATE_EVASION, CLASS_ROGUE, 41, SPEC_SHARPSHOOTER);
  ADD_CLASS_INNATE(INNATE_CALMING, CLASS_MONK, 1, 0);

  ADD_CLASS_INNATE(INNATE_DUAL_WIELDING_MASTER, CLASS_RANGER, 41, 0);
  ADD_CLASS_INNATE(INNATE_OUTDOOR_SNEAK, CLASS_RANGER, 1, 0);
  ADD_CLASS_INNATE(INNATE_SNEAK, CLASS_RANGER, 1, SPEC_HUNTSMAN);

  ADD_CLASS_INNATE(INNATE_IMMOLATE, CLASS_REAVER, 31, SPEC_FLAME_REAVER);
  ADD_CLASS_INNATE(INNATE_PROT_FIRE, CLASS_REAVER,31, SPEC_FLAME_REAVER);

  ADD_CLASS_INNATE(INNATE_CHARM_ANIMAL, CLASS_SHAMAN, 31, SPEC_ANIMALIST);
  ADD_CLASS_INNATE(INNATE_SHAPECHANGE, CLASS_SHAMAN, 41, SPEC_ANIMALIST);
  ADD_CLASS_INNATE(INNATE_RESURRECTION, CLASS_SHAMAN, 56, SPEC_SPIRITUALIST);
  ADD_CLASS_INNATE(INNATE_IMPROVED_HEAL, CLASS_SHAMAN, 36, SPEC_SPIRITUALIST);
  ADD_CLASS_INNATE(INNATE_ELEMENTAL_POWER, CLASS_SHAMAN, 35, SPEC_ELEMENTALIST);

  ADD_CLASS_INNATE(INNATE_CHARM_ANIMAL, CLASS_DRUID, 21, 0);
  ADD_CLASS_INNATE(INNATE_SHAPECHANGE, CLASS_DRUID, 11, 0);
  ADD_CLASS_INNATE(INNATE_PLANE_SHIFT, CLASS_DRUID, 41, 0);
  ADD_CLASS_INNATE(INNATE_FOREST_SIGHT, CLASS_DRUID, 1, 0);

  ADD_CLASS_INNATE(INNATE_SHAPECHANGE, CLASS_BLIGHTER, 11, 0);
  ADD_CLASS_INNATE(INNATE_PLANE_SHIFT, CLASS_BLIGHTER, 41, 0);
  ADD_CLASS_INNATE(INNATE_FOREST_SIGHT, CLASS_BLIGHTER, 1, 0);

  ADD_CLASS_INNATE(INNATE_WILDMAGIC, CLASS_SORCERER, 41, SPEC_WILDMAGE);
  ADD_CLASS_INNATE(INNATE_SUMMON_BOOK, CLASS_SORCERER, 30, SPEC_WIZARD);

  ADD_CLASS_INNATE(HOLY_COMBAT, CLASS_PALADIN, 21, 0);

  ADD_CLASS_INNATE(INNATE_SUMMON_MOUNT, CLASS_PALADIN, 8, 0);
  ADD_CLASS_INNATE(INNATE_ANTI_EVIL, CLASS_PALADIN, 1, 0);
  ADD_CLASS_INNATE(INNATE_LAY_HANDS, CLASS_PALADIN, 1, 0);
  ADD_CLASS_INNATE(INNATE_KNIGHT, CLASS_PALADIN, 30, SPEC_CAVALIER);
  ADD_CLASS_INNATE(INNATE_HOLY_CRUSADE, CLASS_PALADIN, 30, SPEC_CRUSADER);
  ADD_CLASS_INNATE(INNATE_AURA_PROTECTION, CLASS_PALADIN, 1, 0);
  ADD_CLASS_INNATE(INNATE_AURA_PROTECTION, CLASS_CLERIC, 1, SPEC_HOLYMAN);
  ADD_CLASS_INNATE(INNATE_AURA_SPELL_PROTECTION, CLASS_CLERIC, 1, SPEC_HOLYMAN);
  ADD_CLASS_INNATE(INNATE_AURA_PRECISION, CLASS_PALADIN, 10, 0);
  ADD_CLASS_INNATE(INNATE_AURA_ENDURANCE, CLASS_PALADIN, 15, 0);
  ADD_CLASS_INNATE(INNATE_AURA_HEALING, CLASS_PALADIN, 30, 0);
  ADD_CLASS_INNATE(INNATE_AURA_BATTLELUST, CLASS_PALADIN, 45, 0);

  ADD_CLASS_INNATE(INNATE_VISION_OF_THE_DEAD, CLASS_NECROMANCER, 31, SPEC_NECROLYTE);
  ADD_CLASS_INNATE(INNATE_VISION_OF_THE_DEAD, CLASS_THEURGIST, 31, SPEC_TEMPLAR);
  ADD_CLASS_INNATE(INNATE_UNHOLY_ALLIANCE, CLASS_NECROMANCER, 31, SPEC_NECROLYTE);
  ADD_CLASS_INNATE(INNATE_UNHOLY_ALLIANCE, CLASS_THEURGIST, 31, SPEC_TEMPLAR);
  ADD_CLASS_INNATE(INNATE_MUMMIFY, CLASS_NECROMANCER, 41, SPEC_DIABOLIS);
  ADD_CLASS_INNATE(INNATE_REQUIEM, CLASS_THEURGIST, 41, SPEC_MEDIUM);
  ADD_CLASS_INNATE(INNATE_SPAWN, CLASS_NECROMANCER, 41, SPEC_NECROLYTE);
  ADD_CLASS_INNATE(INNATE_ALLY, CLASS_THEURGIST, 41, SPEC_TEMPLAR);
  ADD_CLASS_INNATE(INNATE_SUMMON_HOST, CLASS_THEURGIST, 31, SPEC_MEDIUM);
  ADD_CLASS_INNATE(INNATE_REMORT, CLASS_THEURGIST, 46, SPEC_THAUMATURGE);
  ADD_CLASS_INNATE(INNATE_REMORT, CLASS_NECROMANCER, 46, SPEC_REAPER);

  ADD_CLASS_INNATE(INNATE_FPRESENCE, CLASS_DREADLORD, 46, SPEC_SHADOWLORD);
  ADD_CLASS_INNATE(INNATE_FADE, CLASS_DREADLORD, 51, SPEC_SHADOWLORD);
  ADD_CLASS_INNATE(INNATE_EMBRACE_DEATH, CLASS_DREADLORD, 31, SPEC_DEATHLORD);
  ADD_CLASS_INNATE(INNATE_CALL_GRAVE, CLASS_DREADLORD, 30, SPEC_DEATHLORD);

  ADD_CLASS_INNATE(INNATE_SUMMON_MOUNT, CLASS_ANTIPALADIN, 8, 0);
  ADD_CLASS_INNATE(INNATE_ANTI_GOOD, CLASS_ANTIPALADIN, 1, 0);
  ADD_CLASS_INNATE(INNATE_VAMPIRIC_TOUCH, CLASS_ANTIPALADIN, 1, 0);
  ADD_CLASS_INNATE(INNATE_KNIGHT, CLASS_ANTIPALADIN, 30, SPEC_DEMONIC);
  ADD_CLASS_INNATE(INNATE_SENSE_WEAKNESS, CLASS_ANTIPALADIN, 30, SPEC_DARKKNIGHT);
  ADD_CLASS_INNATE(INNATE_DECREPIFY, CLASS_ANTIPALADIN, 1, 0);
  ADD_CLASS_INNATE(INNATE_LIFEDRAIN, CLASS_ANTIPALADIN, 36, 0);
  ADD_CLASS_INNATE(INNATE_AURA_PROTECTION, CLASS_ANTIPALADIN, 1, 0);
  ADD_CLASS_INNATE(INNATE_AURA_PRECISION, CLASS_ANTIPALADIN, 10, 0);
  ADD_CLASS_INNATE(INNATE_AURA_ENDURANCE, CLASS_ANTIPALADIN, 15, 0);
  ADD_CLASS_INNATE(INNATE_AURA_VIGOR, CLASS_ANTIPALADIN, 30, 0);
  ADD_CLASS_INNATE(INNATE_AURA_BATTLELUST, CLASS_ANTIPALADIN, 45, 0);

  ADD_CLASS_INNATE(INNATE_GOD_CALL, CLASS_CLERIC, 1, 0);
  //ADD_CLASS_INNATE(INNATE_BLOOD_SCENT, CLASS_CLERIC, 1, SPEC_HEALER);

  ADD_CLASS_INNATE(INNATE_FLY, CLASS_ETHERMANCER, 1, 0);
  ADD_CLASS_INNATE(INNATE_FLY, CLASS_CONJURER, 30, SPEC_AIR);
  ADD_CLASS_INNATE(INNATE_HASTE, CLASS_CONJURER, 30, SPEC_AIR);
  ADD_CLASS_INNATE(INNATE_PROT_FIRE, CLASS_CONJURER, 30, SPEC_FIRE);
  ADD_CLASS_INNATE(INNATE_PROT_FIRE, CLASS_CONJURER, 30, SPEC_WATER);
  ADD_CLASS_INNATE(INNATE_PROT_COLD, CLASS_CONJURER, 30, SPEC_WATER);
  ADD_CLASS_INNATE(INNATE_WATERBREATH, CLASS_CONJURER, 30, SPEC_WATER);
  ADD_CLASS_INNATE(INNATE_CONJURE_WATER, CLASS_CONJURER, 30, SPEC_WATER);
  ADD_CLASS_INNATE(INNATE_REGENERATION, CLASS_CONJURER, 36, SPEC_WATER);
  ADD_CLASS_INNATE(INNATE_LIVING_STONE, CLASS_CONJURER, 30, SPEC_EARTH);
  ADD_CLASS_INNATE(INNATE_STONE, CLASS_CONJURER, 30, SPEC_EARTH);
  ADD_CLASS_INNATE(INNATE_STONE, CLASS_REAVER, 36, SPEC_EARTH_REAVER);
  ADD_CLASS_INNATE(INNATE_REGENERATION, CLASS_SUMMONER, 30, SPEC_NATURALIST);
  ADD_CLASS_INNATE(INNATE_STONE, CLASS_SUMMONER, 30, SPEC_NATURALIST);
  ADD_CLASS_INNATE(INNATE_LIVING_STONE, CLASS_SUMMONER, 30, SPEC_NATURALIST);
  ADD_CLASS_INNATE(INNATE_HASTE, CLASS_SUMMONER, 30, SPEC_CONTROLLER);
  ADD_CLASS_INNATE(INNATE_PROT_COLD, CLASS_SUMMONER, 30, SPEC_MENTALIST);
  ADD_CLASS_INNATE(INNATE_PROT_FIRE, CLASS_SUMMONER, 30, SPEC_MENTALIST);

  ADD_CLASS_INNATE(INNATE_BATTLEAID, CLASS_MERCENARY, 41, SPEC_BOUNTY);
  ADD_CLASS_INNATE(INNATE_PERCEPTION, CLASS_MERCENARY, 31, SPEC_BOUNTY);
  ADD_CLASS_INNATE(INNATE_INTERCEPT, CLASS_MERCENARY, 31, SPEC_BOUNTY);

  ADD_CLASS_INNATE(INNATE_SNEAK, CLASS_ROGUE, 1, SPEC_THIEF);
//  ADD_CLASS_INNATE(INNATE_SNEAK, CLASS_ROGUE, 1, SPEC_THIEF);
//  ADD_CLASS_INNATE(INNATE_QUICK_THINKING, CLASS_THIEF, 36, SPEC_TRICKSTER); No longer in game.
  ADD_CLASS_INNATE(INNATE_WALL_CLIMBING, CLASS_ROGUE, 56, SPEC_THIEF);

  ADD_CLASS_INNATE(INNATE_EYELESS, CLASS_DREADLORD, 1, 0);
  ADD_CLASS_INNATE(INNATE_ACID_BLOOD, CLASS_DREADLORD, 1, 0);
  ADD_CLASS_INNATE(INNATE_COMMAND_AURA, CLASS_DREADLORD, 36, 0);

  ADD_CLASS_INNATE(INNATE_EYELESS, CLASS_AVENGER, 1, 0);
  ADD_CLASS_INNATE(INNATE_MAGIC_RESISTANCE, CLASS_AVENGER, 1, 0);
  ADD_CLASS_INNATE(INNATE_HOLY_LIGHT, CLASS_AVENGER, 1, 0);
  ADD_CLASS_INNATE(INNATE_GOD_CALL, CLASS_AVENGER, 21, 0);
  ADD_CLASS_INNATE(INNATE_COMMAND_AURA, CLASS_AVENGER, 36, 0);

  ADD_CLASS_INNATE(INNATE_IMPROVED_FLEE, CLASS_BARD, 1, SPEC_SCOUNDREL);

  ADD_CLASS_INNATE(INNATE_BLINDSINGING, CLASS_BARD, 1, SPEC_MINSTREL);

  ADD_CLASS_INNATE(INNATE_ECHO, CLASS_BARD, 31, SPEC_DISHARMONIST);

  ADD_CLASS_INNATE(INNATE_HAMMER_MASTER, CLASS_BERSERKER, 31, SPEC_RAGELORD);

  ADD_CLASS_INNATE(INNATE_FIRESHIELD, CLASS_PSIONICIST, 30, SPEC_PYROKINETIC);
  ADD_CLASS_INNATE(INNATE_PROT_FIRE, CLASS_PSIONICIST, 30, SPEC_PYROKINETIC);
  ADD_CLASS_INNATE(INNATE_DRAGONMIND, CLASS_MONK, 30, SPEC_WAYOFDRAGON);
  ADD_CLASS_INNATE(INNATE_ASTRAL_NATIVE, CLASS_PSIONICIST, 30, SPEC_PSYCHEPORTER);
  ADD_CLASS_INNATE(INNATE_IMPROVED_WORMHOLE, CLASS_PSIONICIST, 46, SPEC_PSYCHEPORTER);
  ADD_CLASS_INNATE(INNATE_IMPROVED_WORMHOLE, CLASS_MINDFLAYER, 46, 0);

  ADD_CLASS_INNATE(INNATE_DECEPTIVE_FLEE, CLASS_ILLUSIONIST, 31, SPEC_DECEIVER);

  ADD_CLASS_INNATE(INNATE_ENTRAPMENT, CLASS_MERCENARY, 45, 0);  
  
//  ADD_CLASS_INNATE(INNATE_LEVITATE, CLASS_ETHERMANCER, 1, 0);
}

P_char parse_victim(P_char ch, char *arg, uint flags)
{
  P_char   victim = NULL;

  if ((flags & PRSVCT_ENGFIRST) && GET_OPPONENT(ch))
    return GET_OPPONENT(ch);

  if (arg && *arg)
    victim = get_char_room_vis(ch, arg);

  if (!victim && !(flags & PRSVCT_NOENG))
    return GET_OPPONENT(ch);
  else
    return victim;
}

void get_property_format(const char *input, char *formatted)
{
  char c;
  bool was_space = false;

  while (c = *input++) {
    if (isspace(c))
      was_space = true;
    else if (was_space) {
      *formatted++ = toupper(c);
      was_space = false;
    } else
      *formatted++ = c;
  }

  *formatted = '\0';
}

int get_innate_property(int innate, const char *prefix, int default_value)
{
  char innate_name[128], property_string[256];

  get_property_format(innates_data[innate].name, innate_name);
  snprintf(property_string, 256, "%s.%s", prefix, innate_name);

  return get_property(property_string, default_value);
}

// an additional function, for when one needs to check the innate timer without altering it
bool can_use_innate(P_char ch, int innate)
{
  struct affected_type af, *afp;

  for (afp = ch->affected; afp; afp = afp->next)
  {
    if( afp->type == TAG_INNATE_TIMER && afp->location == innate )
    {
      break;
    }
  }

  if( afp && afp->modifier == 1 )
  {
    return FALSE;
  }

  return TRUE;
}

bool check_innate_time(P_char ch, int innate, int duration)
{
  struct affected_type af, *afp;
  int timer;
  int default_duration;

  for( afp = ch->affected; afp; afp = afp->next )
  {
    if( afp->type == TAG_INNATE_TIMER && afp->location == innate )
    {
      break;
    }
  }
  if( afp )
  {
    if (afp->modifier == 1)
    {
      return FALSE;
    }
    else
    {
      afp->modifier--;
      return TRUE;
    }
  }

  if ( IS_PC(ch) && !affected_by_spell(ch, TAG_PVPDELAY))
  {
  //  to be replaced with check to see if innate.pvp.timer exists
  }

  // in seconds. default is 12 mins = 12*60 = 720
  default_duration = get_property("innate.timer.secs.default", 720);

  timer = (!duration) ? get_innate_property(innate, "innate.timer.secs", default_duration) : duration;
  memset(&af, 0, sizeof(af));
  af.type = TAG_INNATE_TIMER;
  af.location = innate;
  af.modifier = get_innate_property(innate, "innate.uses", 1);
  af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
  af.duration = timer * WAIT_SEC;

  affect_to_char(ch, &af);

  return TRUE;
}

bool check_reincarnate(P_char ch)
{
  // innate resurrect! shaman spec
  if (has_innate(ch, INNATE_RESURRECTION))
  {
    if (!number(0, 8))          // this may be tweeked later
    {
      P_char   t, t_next;

      if (ch->in_room == 1) // an attempt to fix problems with death from transfer_wellness()
      {
        wizlog(MINLVLIMMORTAL, "INNATE_RESURRECTION just tried to resurrect %s in Limbo.", GET_NAME(ch));
       	if (int old_room = ch->specials.was_in_room)
       	{
      	  char_from_room(ch);
      	  char_to_room(ch, real_room(old_room), -1);
      	  wizlog(MINLVLIMMORTAL, "%s returned to the room he was earlier - %d", GET_NAME(ch), world[old_room].number);
      	}
      	else
      	{
      	  wizlog(MINLVLIMMORTAL, "%s returned to the birthplace.", GET_NAME(ch));
      		char_from_room(ch);
      		char_to_room(ch, real_room(GET_BIRTHPLACE(ch)), -1);
      	}
      }
      
      if (CAN_SPEAK(ch))
      {
         death_cry(ch);
      }

      if (GET_OPPONENT(ch))
      {
        stop_fighting(ch);
      }
      
      if( IS_DESTROYING(ch) )
      {
        stop_destroying(ch);
      }

      for (t = world[ch->in_room].people; t; t = t_next)
      {
        t_next = t->next_in_room;
        if (GET_OPPONENT(t) == ch)
        {
          stop_fighting(t);
          clearMemory(t);
        }
      }

      GET_HIT(ch) = BOUNDED(10, GET_MAX_HIT(ch), 100);
      update_pos(ch);
      CharWait(ch, dice(2, 2) * 4);

      act
        ("&+M$n's broken body unexpectedly returns to life again!  The worst of $s wounds quickly knit themselves.&N",
         TRUE, ch, 0, 0, TO_ROOM);
      act
        ("&+W$n comes to life again! Taking a deep breath, $n opens $s eyes!&n",
         TRUE, ch, 0, 0, TO_ROOM);
      act
        ("&+MYour soul quickly departs your broken body, only to be quickly ushered back by your spirit guide. After opening your eyes you discover that the worst of your wounds are healed!&N",
         FALSE, ch, 0, 0, TO_CHAR);

      return TRUE;
    }
  }
  return FALSE;
}

void do_disappear(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if (has_innate(ch, INNATE_DISAPPEAR))
  {
    if (!fight_in_room(ch))
    {
      send_to_char("You are not scared enough to hide.\n", ch);
      return;
    }

    act("You suddenly disappear.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n suddenly disappears.", TRUE, ch, 0, 0, TO_ROOM);
    SET_BIT(ch->specials.affected_by, AFF_HIDE);
    CharWait(ch, PULSE_VIOLENCE);
  }
  else
  {
    act("&+Gho ho ho, &+Rfun fun fun&+Y and still _not_&n", FALSE, ch, 0, 0,
        TO_CHAR);
  }
}

void event_embrace_death(P_char ch, P_char victim, P_obj obj, void *data)
{
  float hp_state = ((float)GET_HIT(ch))/GET_MAX_HIT(ch);
  struct affected_type *afp;

  afp = get_spell_from_char(ch, TAG_EMBRACE_DEATH);

  if (afp)
    if (hp_state > .75) {
      affect_remove(ch, afp);
      send_to_char("Something leaves you as death is no longer close.\n", ch);
    }
    else {
      if (hp_state > .45)
        afp->modifier = 5;
      else if (hp_state > .25)
        afp->modifier = 10;
      balance_affects(ch);
      add_event(event_embrace_death, 2 * WAIT_SEC, ch, 0, 0, 0, 0, 0);
    }
}

void do_innate_embrace_death(P_char ch)
{
  float hp_state = ((float)GET_HIT(ch))/GET_MAX_HIT(ch);
  struct affected_type af, *afp;
  int bonus = 0;

  afp = get_spell_from_char(ch, TAG_EMBRACE_DEATH);

  if (afp)
    bonus = afp->modifier;

  if (hp_state <= .25 && bonus < 15)
  {
    bonus = 15;
    send_to_char("&+LYou begin grin&+rning fever&+Lishly as death draws nearer!&n\r\n", ch);
  }
  else if (hp_state <= .45 && bonus < 10)
  {
    bonus = 10;
    send_to_char("&+LYour bla&+rck blo&+Lod races with excitement as the pain washes over you!&n\r\n", ch);
  }
  else if (hp_state <= .75 && bonus < 5)
  {
    bonus = 5;
    send_to_char("&+LPain w&+rracks your bod&+Ly in angry spasms.  It feels... good.&n\r\n", ch);
  }
  else if (hp_state > .75 && afp)
  {
    affect_remove(ch, afp);
  }

  if (!afp && bonus) {
    memset(&af, 0, sizeof(af));
    af.type = TAG_EMBRACE_DEATH;
    af.duration = 1;
    af.location = APPLY_STR_MAX;
    af.modifier = bonus;
    affect_to_char(ch, &af);
    add_event(event_embrace_death, 2 * WAIT_SEC, ch, 0, 0, 0, 0, 0);
  } else if (afp) {
    balance_affects(ch);
  }
}

void event_paladin_vit(P_char ch, P_char victim, P_obj obj, void *data) {
  struct affected_type *af;

  victim = GET_OPPONENT(ch);

  if (!victim || !IS_EVIL(victim)) {
    af = get_spell_from_char(ch, TAG_PALADIN_VIT);
    if (af)
      if (af->modifier > 2)
        af->modifier -= 2;
      else {
        affect_remove(ch, af);
        wear_off_message(ch, af);
        return;
      }
    else
      return;
    balance_affects(ch);
  }

  add_event(event_paladin_vit, 2 * WAIT_SEC, ch, 0, 0, 0, 0, 0);
}

void do_innate_anti_evil(P_char ch, P_char vict)
{
  struct affected_type af, *afp;

  if (GET_ALIGNMENT(ch) < 900) {
    return;
  }

  afp = get_spell_from_char(ch, TAG_PALADIN_VIT);
  if (!afp)
  {
    act ("&+WA halo of white light settles over&n $n &+Was $e charges into battle.&n",
       FALSE, ch, 0, vict, TO_NOTVICT);
    act ("&+WA halo of white light descends upon&n $n &+Yburning &+Wyour soul.&n",
       FALSE, ch, 0, vict, TO_VICT);
    act("&+WDivine power encases you as you turn to battle evil.&n",
        FALSE, ch, 0, vict, TO_CHAR);
    memset(&af, 0, sizeof(af));
    af.type = TAG_PALADIN_VIT;
    af.flags = AFFTYPE_NOSAVE;
    af.location = APPLY_HIT;
    af.bitvector = AFF_AWARE;
    af.duration = 2;
    af.modifier = GET_LEVEL(vict) * 2;
    affect_to_char(ch, &af);
    add_event(event_paladin_vit, 2 * WAIT_SEC, ch, 0, 0, 0, 0, 0);
  }
  else
  {
    afp->duration = 2;
    afp->modifier = GET_LEVEL(vict) * 2;
    balance_affects(ch);
  }
}

void event_decrepify(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *af;
  P_char curr_vict;

  curr_vict = GET_OPPONENT(ch);

  if (curr_vict != victim)
  {
    af = get_spell_from_char(victim, TAG_DECREPIFY);

    if (af)
    {
      affect_remove(victim, af);
      wear_off_message(victim, af);
      return;
    }
  }
  else
  {
    if (curr_vict)
    {
      af = get_spell_from_char(curr_vict, TAG_DECREPIFY);
      if (af)
      {
        af->duration = 1;
        balance_affects(curr_vict);
        add_event(event_decrepify, 2 * WAIT_SEC, ch, curr_vict, 0, 0, 0, 0);
      }
    }
  }

}

void do_innate_decrepify(P_char ch, P_char victim)
{
  struct affected_type af, *afp;

  if(!ch ||
    !victim)
  {
    logit(LOG_EXIT, "do_innate_decrepify called in innates.c with no ch or victim");
    raise(SIGSEGV);
  }
  if(ch &&
    victim)
  {
    if(GET_OPPONENT(ch) != victim)
    {
      return;
    }

    afp = get_spell_from_char(victim, TAG_DECREPIFY);

    if(!afp)
    {
      act ("A dark shroud encases $n as $e charges into battle.&n",
             FALSE, ch, 0, victim, TO_NOTVICT);
      act ("You are briefly surrounded by a red aura!",
             FALSE, ch, 0, victim, TO_VICT);
      act ("You feel unholy power coursing through your limbs!",
              FALSE, ch, 0, victim, TO_CHAR);

      memset(&af, 0, sizeof(af));
      af.type = TAG_DECREPIFY;
      af.flags = AFFTYPE_NOSAVE;
      af.location = APPLY_CURSE;
      af.modifier = BOUNDED(1,(int) (GET_LEVEL(ch) / 10),5);
      af.duration = 1;
      affect_to_char(victim, &af);
      add_event(event_decrepify, 2 * WAIT_SEC, ch, victim, 0, 0, 0, 0);
    }
    else
    {
      afp->duration = 1;
      balance_affects(victim);
    }
  }
  return;
}


void event_throw_lightning(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      dam;
  struct damage_messages messages =
  {
    "&+bA &+BGIANT&n &+bbolt of &-L&+Blightning&n&+L streaks from above, "
      "striking $N head on!",
    "&+bA &+BGIANT&n &+bbolt of &-L&+Blightning&n&+L streaks from above, "
      "striking you head on!",
    "&+bA &+BGIANT&n &+bbolt of &-L&+Blightning&n&+L streaks from above, "
      "striking $N head on!",
    "&+LYour furious &+Bbolt of lightning&+L strikes $N&+L, "
      "leaving only a charred corpse!&n",
    "&+LYou see a great &+Bflash of light&n from the sky... "
      "you can smell your own charred flesh before you die.&n",
    "&+LA furious &+Bbolt of lightning&+L strikes $N&+L, "
      "leaving only a charred corpse!&n"
  };

  victim = GET_OPPONENT(ch);

  if (!victim)
  {
    send_to_room("&+BThe sparks disperse.&N\n", ch->in_room);
    return;
  }

  dam = dice(3 * GET_LEVEL(ch), 5);
  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, 0, &messages);
}

void do_throw_lightning(P_char ch, char *argument, int cmd)
{
  if (!check_innate_time(ch, INNATE_THROW_LIGHTNING))
  {
    send_to_char("You're too tired to be electrifying.\n", ch);
    return;
  }

  if (!IS_FIGHTING(ch))
  {
    send_to_char("You are not in the heat of combat!  You cannot feel the call!\n", ch);
    return;
  }

  act("&+BYou begin charging for an electric discharge!&N", FALSE, ch, 0, 0,
      TO_CHAR);
  act("&+BSparks begin to arc between $n and $N!&N", TRUE, ch, 0,
      GET_OPPONENT(ch), TO_ROOM);

  add_event(event_throw_lightning, 2 * PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_levitate(P_char ch, char *arg, int cmd)
{
  if (IS_AFFECTED(ch, AFF_LEVITATE))
    send_to_char("You are already floating around!\n", ch);
  else if (!check_innate_time(ch, INNATE_LEVITATE))
  {
    send_to_char("You are too tired to do that right now!\n", ch);
  }
  else
  {
    act("$n screws $s face in concentration..", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("After a period of brief concentration, ", ch);
    spell_levitate(GET_LEVEL(ch), ch, NULL, 0, ch, 0);
  }
  CharWait(ch, PULSE_VIOLENCE);
}

void vampire_bite(P_char ch, P_char victim)
{
  struct affected_type af;
  struct damage_messages messages = {
    "You savagely bite $N's neck!",
    "$n savagely bites at YOUR NECK!",
    "$n savagely bites $N's neck!",
    "Your savage bite opens a deadly wound in $N's neck $S!",
    "$n savagely bites at YOUR NECK!\nYou slowly fall into darkness "
      "as the last drops of blood are sucked out from you..",
    "$n's savage bite opens a deadly wound in $N's neck!"
  };

  CharWait(ch, 3 * PULSE_VIOLENCE);

  if (melee_damage(ch, victim, dice(2, GET_LEVEL(ch)), 0, &messages) !=
      DAM_NONEDEAD || affected_by_spell(victim, TAG_VAMPIRE_BITE))
    return;

  if (!IS_STUNNED(victim) && number(0, 100) < 30)
  {
    send_to_char(
      "You are stunned and unable to defend yourself properly!\n", victim);
    Stun(victim, ch, PULSE_VIOLENCE, TRUE);
  }

  memset(&af, 0, sizeof(af));
  af.type = TAG_VAMPIRE_BITE;
  af.duration = number(2, 4);
  af.location = APPLY_CON;
  af.modifier = 0 - MAX(1, GET_LEVEL(ch) / 10);
  affect_to_char(victim, &af);
}

/* replaces spell_poison() in insect and snake bites, to prevent magical shrug working on physical bites/poisons */
int bite_poison(P_char ch, P_char victim, int mod)
{
  int was_poisoned;

  if(NewSaves(victim, SAVING_SPELL, mod))
    return FALSE;

  was_poisoned = IS_SET(victim->specials.affected_by2, AFF2_POISONED);

  if(!IS_TRUSTED(victim) && !IS_UNDEADRACE(victim))
  {
    (skills[number(FIRST_POISON, LAST_POISON)].spell_pointer) (GET_LEVEL(ch), ch, 0, 0, victim, 0);
    act("&+G$n shivers slightly.", TRUE, victim, 0, 0, TO_ROOM);
    if(was_poisoned)
      send_to_char("&+GYou feel even more ill.\n", victim);
    else
      send_to_char("&+GYou feel very sick.\n", victim);
  }
  
  return TRUE;
}

void insectbite(P_char ch, P_char victim)
{
  struct damage_messages messages = {
    "You leap towards $N and sink your jaws dripping with venom deep in $S flesh.",
    "$n leaps towards you and sinks $s jaws dripping with venom deep in your flesh.",
    "$n leaps towards $N and sinks $s jaws dripping with venom deep in $S flesh.",
    "You leap towards $N and sink your jaws dripping with venom deep in $S flesh.",
    "$n leaps towards you and sinks $s jaws dripping with venom deep in your flesh.",
    "$n leaps towards $N and sinks $s jaws dripping with venom deep in $S flesh."};
  int i;

  if ((GET_LEVEL(ch) + STAT_INDEX(GET_C_AGI(ch))) < number(1, GET_LEVEL(victim) + 2*STAT_INDEX(GET_C_AGI(victim))))
  {
    send_to_char("You miss them by inches with your jaws!\n", ch);
    act("$n tries to sting $s foe with venom but misses.", FALSE, ch, 0, 0, TO_ROOM);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  if( raw_damage(ch, victim, number(GET_LEVEL(ch), GET_LEVEL(ch)*3), RAWDAM_DEFAULT, &messages) == DAM_NONEDEAD )
  {
    i = 1 + GET_LEVEL(ch)/12;
    while (i-- && !bite_poison(ch, victim, i))
	  ;
/*	replaced by the above statement, to prevent magical shrug working on non-magical innate */
/*    while (i-- && !IS_AFFECTED2(victim, AFF2_POISONED))
      spell_poison(GET_LEVEL(ch), ch, 0, 0, victim, 0);*/
    if( IS_ALIVE(ch) )
    {
      CharWait(ch, PULSE_VIOLENCE);
    }
  }
}

void event_snakebite(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct damage_messages messages = {
    "Your shape blurs as you lash towards $N and sink your fangs in $S flesh.",
    "$n's shape blurs as $e lashes towards you and sinks $s fangs in your flesh.",
    "$n's shape blurs as $e lashes towards $N and sinks $s fangs in $S flesh.",
    "Your shape blurs as you lash towards $N and sink your fangs in $S flesh.",
    "$n's shape blurs as $e lashes towards you and sinks $s fangs in your flesh.",
    "$n's shape blurs as $e lashes towards $N and sinks $s fangs in $S flesh.",
  };
  int i;

  if (GET_STAT(ch) <= STAT_INCAP)
    return;

  if (!is_char_in_room(victim, ch->in_room))
    return;

  if ((GET_LEVEL(ch) + STAT_INDEX(GET_C_AGI(ch))) < number(1, GET_LEVEL(victim) + 2*STAT_INDEX(GET_C_AGI(victim))))
  {
    act("$n misses $N by inches with $s fangs!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n misses you by inches with $s fangs!", FALSE, ch, 0, victim, TO_VICT);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  if( raw_damage(ch, victim, number(GET_LEVEL(ch), GET_LEVEL(ch)*3), RAWDAM_DEFAULT, &messages) == DAM_NONEDEAD )
  {
    i = 1 + GET_LEVEL(ch)/12;
    while (i-- && !bite_poison(ch, victim, i))
	  ;
/*	replaced by the above statement, to prevent magical shrug working on non-magical innate */
/*    while (i-- && !IS_AFFECTED2(victim, AFF2_POISONED))
      spell_poison(GET_LEVEL(ch), ch, 0, 0, victim, 0);*/
    if( IS_ALIVE(ch) )
    {
      CharWait(ch, PULSE_VIOLENCE);
    }
  }
}

void snakebite(P_char ch, P_char victim)
{
  send_to_char("You hiss preparing to attack..\n", ch);
  act("$n hisses dangerously!", FALSE, ch, 0, 0, TO_ROOM);

  add_event(event_snakebite, PULSE_VIOLENCE, ch, victim, 0, 0, 0, 0);
}

int parasitebite(P_char ch, P_char victim)
{
  int level, damage, mod;

  struct damage_messages messages = {
    "You leap towards $N and bite $M savagely.",
    "$n leaps towards you and bites you savagely!",
    "$n leaps towards $N and bites $M savagely!",
    "You leap towards $N and bite $M savagely.",
    "$n leaps towards you and bites you savagely!",
    "$n leaps towards $N and bites $M savagely!"};

  struct damage_messages messages_with_latch = {
    "You leap towards $N and bite $M savagely, attempting to latch to $S body.",
    "$n leaps towards you and bites you savagely, attempting to latch to your body!",
    "$n leaps towards $N and bites $M savagely, attempting to latch to $S body!",
    "You leap towards $N and bite $M savagely, tearing off a big chunk of $S body.",
    "$n leaps towards you and bites you savagely, tearing off a big chunk of your body!",
    "$n leaps towards $N and bites $M savagely, tearing off a big chunk of $S body"};


  level = GET_LEVEL(ch);

  damage = 40 + dice( 4, level );
  if( IS_NPC(ch) )
    damage += dice( 4, level );

  if (!StatSave(victim, APPLY_AGI, BOUNDED(-3, (GET_LEVEL(victim) - GET_LEVEL(ch) -
                                           STAT_INDEX(GET_C_AGI(ch))/2) / 5, 3)))
  {
/*  if (has_innate(ch, INNATE_LATCH) && !IS_ATTACHED(ch) && !number(0, 7-mod))
    {
      melee_damage(ch, victim, (int) (damage * 1.5), 0, &messages_with_latch);
      latch(ch, victim);
    }
    else FOR LATER USE*/
      melee_damage(ch, victim, damage , 0, &messages);
  }
  else
  {
    melee_damage(ch, victim, damage >> 1 , 0, &messages);
  }

  if (IS_ALIVE(victim) && has_innate(ch, INNATE_DISEASED_BITE) && !number(0, 7-mod))
    spell_disease(level, ch, 0, SPELL_TYPE_SPELL, victim, 0);

  if( IS_ALIVE(ch) )
  {
    CharWait(ch, PULSE_VIOLENCE);
  }
/*if (IS_ALIVE(victim) && has_innate(ch, INNATE_INFEST) && !number(0, 7-mod) && IS_ATTACHED_TO(ch, victim))
    infest(ch, victim); FOR LATER USE */

  return 0;
}

void bite(P_char ch, P_char victim)
{
  bool     dead;

  if (isname("_nobite_", GET_NAME(ch)))
  {
   return;
  }

  if (GET_RACE2(ch) == RACE_SNAKE) {
    snakebite(ch, victim);
    return;
  }

  if (GET_RACE2(ch) == RACE_PARASITE) {
    parasitebite(ch, victim);
    return;
  }

  if( GET_STAT(ch) < STAT_RESTING )
  {
    if( GET_STAT(ch) == STAT_SLEEPING )
      send_to_char( "You wish you could, if only you were awake.\n", ch );
    else
      send_to_char( "Mmm.. food.  If only you weren't bleeding to death already.\n", ch );
    return;
  }

  if (GET_RACE2(ch) == RACE_PVAMPIRE || GET_RACE2(ch) == RACE_VAMPIRE)
  {
    vampire_bite(ch, victim);
    return;
    if (victim == ch)
    {
      send_to_char("What a ridiculous notion to bite yourself!\n", ch);
      return;
    }
    if ((GET_LEVEL(ch) + number(0, 6)) >= GET_LEVEL(victim))
    {
      act
        ("&+LYou lunge to bite $N&+L... &+Rand sink your teeth into $S flesh!&n",
         FALSE, ch, 0, victim, TO_CHAR);
      act
        ("&+L$n&+L lunges to bite you... &+Rand sinks $s teeth into your flesh!&n",
         FALSE, ch, 0, victim, TO_VICT);
      act
        ("&+L$n&+L lunges to bite $N&+L... &+Rand sinks $s teeth into $S flesh!.&n",
         FALSE, ch, 0, victim, TO_NOTVICT);
      dead = damage(ch, victim, GET_LEVEL(ch) * 4, TYPE_UNDEFINED);
      GET_HIT(ch) += GET_LEVEL(ch) * 2;
      if (!dead && IS_NPC(victim) && GET_LEVEL(victim) <= GET_LEVEL(ch) - 20
          && !StatSave(victim, APPLY_POW, POW_DIFF(ch, victim)))
      {
        struct follow_type *followers;
        int      num = 0;

        for (followers = ch->followers; followers;
             followers = followers->next)
          if (affected_by_spell(followers->follower, SPELL_AWE))
            num++;
        if (num < 2 && !circle_follow(victim, ch))
        {
          if (victim->following && victim->following != ch)
            stop_follower(victim);
          if (!victim->following)
            add_follower(victim, ch);
          setup_pet(victim, ch, GET_LEVEL(ch), 0);
          act("&+LA glazed look sweeps over $N's face...&n", FALSE, ch, 0,
              victim, TO_CHAR);
          act
            ("&+LYou feel your will creep away... you are $n's&+L new slave.",
             FALSE, ch, 0, victim, TO_VICT);
          act("&+LA glazed look sweeps over $N's face...&n", FALSE, ch, 0,
              victim, TO_NOTVICT);
          do_say(victim, "I do your bidding, my master.", CMD_SAY);
          if (IS_FIGHTING(victim))
            stop_fighting(victim);
          if (IS_DESTROYING(victim))
            stop_destroying(victim);
          StopMercifulAttackers(victim);
        }
      }
    }
    else
    {
      act("&+LYou lunge to bite $N&+L, but miss completely.&n", FALSE, ch, 0,
          victim, TO_CHAR);
      act("&+L$n&+L lunges to bite you, but misses completely.&n", FALSE, ch,
          0, victim, TO_VICT);
      act("&+L$n&+L lunges to bite $N&+L, but misses completely.&n", FALSE,
          ch, 0, victim, TO_NOTVICT);
      engage(ch, victim);
    }
    CharWait(ch, PULSE_VIOLENCE * 3);
    return;
  }

  engage(ch, victim);
  insectbite(ch, victim);
}

void do_bite(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (!(victim = parse_victim(ch, arg, 0)))
  {
    send_to_char("Target who?\n", ch);
    return;
  }

  if( !check_innate_time(ch, INNATE_BITE, get_property( "innate.timer.bite", 12 )) )
  {
    send_to_char("You have no &+gvenom&N left!\n", ch);
    return;
  }

  bite(ch, victim);
}

void innate_gaze(P_char ch, P_char victim)
{
  P_char   tch;

  tch = get_random_char_in_room(ch->in_room, victim, DISALLOW_UNGROUPED | DISALLOW_SELF);

  if( !tch )
  {
    tch = victim;
  }

  act("You direct your &+Lcold&N gaze in $N's direction..", TRUE, ch, 0, tch, TO_CHAR);
  act("$n directs $s &+Lcold&N gaze in your direction..", TRUE, ch, 0, tch, TO_VICT);
  act("$n's &+reyes&n glow &+Rred&N as $e gazes at $N!", TRUE, ch, 0, tch, TO_NOTVICT);

  if( !GOOD_FOR_GAZING(ch, tch) )
  {
    act("&+w$N&+w laughs at your &+Leyelashes&+w.&n", TRUE, ch, 0, tch, TO_CHAR );
    act("&+wYou laugh at $s pretty &+Leyelashes&+w.&n", TRUE, ch, 0, tch, TO_VICT );
    act("&+w$N&+w laughs at $s pretty &+Leyelashes&+w.&n", TRUE, ch, 0, tch, TO_NOTVICT );
  }
  else if( saves_spell(tch, SAVING_FEAR) )
  {
    act("&+w$N&+w avoids your gaze.&n", TRUE, ch, 0, tch, TO_CHAR );
    act("You avert your eyes!", TRUE, ch, 0, tch, TO_VICT);
  }
  else
  {
    if( tch != victim )
    {
      attack( victim, tch );
    }
    else
    {
      stop_fighting( victim );
      CharWait( victim, PULSE_VIOLENCE );
      act("&+MOooh, the pretty &+Rflames&+M.&n", TRUE, ch, 0, victim  , TO_VICT);
    }
  }
}

void do_innate_gaze(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (!(victim = parse_victim(ch, arg, 0)))
  {
    send_to_char("Gaze at who?\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_GAZE))
  {
    send_to_char("You are not quite up to it yet!\n", ch);
    return;
  }

  innate_gaze(ch, victim);
}

void do_darkness(P_char ch, char *arg, int cmd)
{
  if (!check_innate_time(ch, INNATE_DARKNESS))
  {
    send_to_char("You're too tired to do that right now!\n", ch);
  }
  else
  {
    act("$n screws $s face in concentration..", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("After a period of brief concentration, ", ch);
    spell_darkness(GET_LEVEL(ch), ch, 0, 0, 0, 0);
  }

  CharWait(ch, PULSE_VIOLENCE);
}

void do_shift_astral(P_char ch, char *arg, int cmd)
{
  
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("You're too busy fighting!\n", ch);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  if (affected_by_spell(ch, TAG_PVPDELAY))
  {
    send_to_char("There is too much adrenaline pumping through your body right now.\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_SHIFT_ASTRAL))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  cast_plane_shift(GET_LEVEL(ch), ch, "astral", SPELL_TYPE_SPELL, NULL, NULL);
  CharWait(ch, PULSE_VIOLENCE);
}


void do_shift_ethereal(P_char ch, char *arg, int cmd)
{
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("You're too busy fighting!\n", ch);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  if (affected_by_spell(ch, TAG_PVPDELAY))
  {
    send_to_char("There is too much adrenaline pumping through your body right now.\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_SHIFT_ETHEREAL))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  cast_plane_shift(GET_LEVEL(ch), ch, "ethereal", SPELL_TYPE_SPELL, NULL, NULL);
  CharWait(ch, PULSE_VIOLENCE);
}



#define PLANE_ZONETARG_ROOMNUMB  19701  /* astral */

void do_shift_prime(P_char ch, char *arg, int cmd)
{
  int      astral = world[real_room(PLANE_ZONETARG_ROOMNUMB)].zone, r_room;
  const int githyanki_room[] = {
    33366, 2381, 11545, 42950, 23691, 36171, 93001, 4437, 90803, 96803, 619036,
      17607, 6405, 88122, 53122
  };
  const int githzerai_room[] = {
    41767, 20484, 6224, 66651, 7117, 93665, 16805, 5759, 2018, 99295, 93919, 16302, 574427, 557620
  };
  const int phantom_room[] = {
    5927, 98731, 14373, 99465, 18305, 217481, 15100, 8486, 8430
  };

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("You're too busy fighting!\n", ch);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

        if (world[ch->in_room].zone != astral)
  {
    send_to_char("Your shifting will not work here!\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_SHIFT_PRIME))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

   if (GET_RACE(ch) == RACE_GITHYANKI || GET_RACE(ch) == RACE_PILLITHID)
  {
    do
      r_room =
        real_room(githyanki_room
                  [number(0, (sizeof(githyanki_room) / sizeof(int)) - 1)]);
    while (r_room == -1);
  }
  else if (GET_RACE(ch) == RACE_GITHZERAI)
  {
    do
      r_room =
        real_room(githzerai_room
	          [number(0, (sizeof(githzerai_room) / sizeof(int)) - 1)]);
    while (r_room == -1);
  }
  else if (GET_RACE(ch) == RACE_PHANTOM)
  {
    do
      r_room =
        real_room(phantom_room
                  [number(0, (sizeof(phantom_room) / sizeof(int)) - 1)]);
    while (r_room == -1);
  }
  else if (GET_RACE(ch) == RACE_ILLITHID)
  {
    do
      r_room =
        real_room(githyanki_room
	          [number(0, (sizeof(githyanki_room) / sizeof(int)) -1)]);
    while (r_room == -1);
  }

  act("$n slowly fades away...", 0, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  send_to_char("You materialize elsewhere!\n", ch);
  if (!char_to_room(ch, r_room, -1))
    return;
  act("$n slowly materializes...", 0, ch, 0, 0, TO_ROOM);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_blast(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (!(victim = parse_victim(ch, arg, 0)))
  {
    send_to_char("Target who?\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_BLAST))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  send_to_char("&+rYou send a wave of mental energy out at your foe!\n",
               ch);
  send_to_char
    ("&+rYou feel something attempting to grab hold of your insides..\n",
     victim);
/*spell_major_paralysis(GET_LEVEL(ch), ch, victim, 0);*/
  if (!IS_SHOPKEEPER(victim))
    spell_innate_blast(GET_LEVEL(ch), ch, 0, 0, victim, 0);
}

void do_faerie_fire(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (!(victim = parse_victim(ch, arg, 0)))
  {
    send_to_char("Target who?\n", ch);
    return;
  }

  /*if (!check_innate_time(ch, INNATE_FAERIE_FIRE))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }*/

  spell_faerie_fire(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_ud_invisibility(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    return;

  if (!check_innate_time(ch, INNATE_UD_INVISIBILITY))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  if (!IS_UD_MAP(ch->in_room) && !IS_UNDERWORLD(ch->in_room))
  {
    send_to_char("There don't appear to be enough shadows here.\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_INVISIBLE;
  af.duration = 45;
  af.bitvector = AFF_INVISIBLE;
  affect_to_char(ch, &af);
  send_to_char("You find a deep shadow and meld into it.\n", ch);
}

void do_strength(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if (!check_innate_time(ch, INNATE_STRENGTH))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  if (affected_by_spell(ch, SPELL_STRENGTH))
  {
    send_to_char("You're already affected by strength enhancement!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_STRENGTH;
  af.duration = 5;
  af.location = APPLY_STR_MAX;
  af.modifier = 7;
  affect_to_char(ch, &af);
  send_to_char("You feel much stronger!\n", ch);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_reduce(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if (!check_innate_time(ch, INNATE_REDUCE))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  if (GET_SIZE(ch) == SIZE_MINIMUM)
    return;

  if (!affected_by_spell(ch, SPELL_REDUCE))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_REDUCE;
    af.duration = 15;
    af.bitvector3 = AFF3_REDUCE;
    affect_to_char(ch, &af);

    act("You shrink to about half your normal size!", FALSE, ch, 0, NULL,
        TO_CHAR);
    act("$n shrinks to about half $s normal size!", TRUE, ch, 0, NULL,
        TO_ROOM);
  }
  CharWait(ch, PULSE_VIOLENCE);
}

void do_enlarge(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if (!check_innate_time(ch, INNATE_ENLARGE))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  if( IS_AFFECTED3(ch, AFF3_ENLARGE) || IS_AFFECTED5(ch, AFF5_TITAN_FORM) )
  {
    act("You try to grow, but, alas, you're already as big as you're going to get.", TRUE, ch, 0, 0, TO_CHAR);
    act("$N screws up $S face trying to grow!", TRUE, ch, 0, 0, TO_ROOM);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  if( !affected_by_spell(ch, SPELL_ENLARGE) )
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_ENLARGE;
    af.duration = 5;
    af.bitvector3 = AFF3_ENLARGE;
    affect_to_char(ch, &af);

    act("You grow to about twice your normal size!", TRUE, ch, 0, 0, TO_CHAR);
    act("$N grows to about twice $S normal size!", TRUE, ch, 0, ch, TO_NOTVICT);
  }
  CharWait(ch, PULSE_VIOLENCE);
}

void do_wall_climbing(P_char ch, char *arg, int cmd)
{
	struct affected_type af;
	act("&+LThis &+Wability &+Lis always active.", TRUE, ch, 0, 0, TO_CHAR);
}

void do_flurry(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if (!check_innate_time(ch, INNATE_FLURRY))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  if (IS_AFFECTED2(ch, AFF2_FLURRY))
  {
    send_to_char("You're already in a flurry of madness as it is.\n", ch);
    return;
  }

  if(GET_RACE(ch) == RACE_HALFLING)
  {
    send_to_char("&+WYou start to pick up momentum as your pulse quickens and the world slows down around you!\r\n", ch);
    act("$n's limbs start to move with &+cincredible speed.&n You can barely keep $s in focus!&n", FALSE, ch, 0, 0, TO_ROOM); 
  }
  else
  {
    send_to_char("A rush of energy fills your veins as your limbs become a flurry of activity!\r\n", ch);
    act("$n visibly becomes more energetic as the movement of $s limbs becomes a blurred flurry!&n", FALSE, ch, 0, 0, TO_ROOM);
  }
  
  memset(&af, 0, sizeof(struct affected_type));
  af.type = SKILL_RAGE;
  af.flags = AFFTYPE_SHORT;
  af.bitvector2 = AFF2_FLURRY;
  af.duration = 4 * PULSE_VIOLENCE;
  affect_to_char(ch, &af);

  //CharWait(ch, PULSE_VIOLENCE);
}

void do_plane_shift(P_char ch, char *arg, int cmd)
{
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("You're too busy fighting!\n", ch);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  if (!has_innate(ch, INNATE_PLANE_SHIFT))
  {
    send_to_char("You don't know how to do that.\n", ch);
    return;

  }

  if (affected_by_spell(ch, TAG_PVPDELAY))
  {
    send_to_char("There is too much adrenaline pumping through your body to do that.\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_PLANE_SHIFT))
  {
    send_to_char("You're too tired right now.\n", ch);
    return;
  }

  cast_plane_shift(GET_LEVEL(ch), ch, arg, SPELL_TYPE_SPELL, NULL, NULL);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_charm_animal(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (!arg || !*arg || !(victim = get_char_room_vis(ch, arg)))
  {
    send_to_char("Target who?\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_CHARM_ANIMAL))
  {
    send_to_char("You're too tired right now.\n", ch);
    return;
  }

  spell_charm_animal(GET_LEVEL(ch), ch, 0, 0, victim, 0);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_dispel_magic(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (!(victim = parse_victim(ch, arg, 0)))
  {
    send_to_char("Dispel who?\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_DISPEL_MAGIC))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  spell_dispel_magic(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  CharWait(ch, PULSE_VIOLENCE * 3);
}

void do_mass_dispel(P_char ch, char *arg, int cmd)
{
  P_char   tch, next;

  if (!check_innate_time(ch, INNATE_MASS_DISPEL))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;
    if (tch != ch)
      spell_dispel_magic(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, tch, 0);
  }

  CharWait(ch, PULSE_VIOLENCE * 4);
}

void do_globe_of_darkness(P_char ch, char *arg, int cmd)
{
  if (!check_innate_time(ch, INNATE_GLOBE_OF_DARKNESS))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  spell_globe_of_darkness(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, 0);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_innate_hide(P_char ch, char *arg, int cmd)
{
  if (IS_AFFECTED(ch, AFF_HIDE))
  {
    send_to_char("You're well-hidden as it is.\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_HIDE))
  {
    send_to_char("You're too tired to dig again.\n", ch);
    return;
  }

  send_to_char
    ("&+yYou drop flat to the &+yground&n perfectly melting in with its irregularities.\n", ch);
  act("$n suddenly suddenly falls to the &+yground&n and vanishes from sight.", FALSE, ch, 0, 0, TO_ROOM);
  ch->specials.affected_by |= AFF_HIDE;
//  send_to_char("&+LYou find a shadow and meld deeply into it.\n", ch);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_project_image(P_char ch, char *arg, int cmd)
{
  P_char   image;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (!ch)
    return;
  image = read_mobile(real_mobile(250), REAL);
  if (!image)
    return;

  send_to_char("&+LYou project an image of yourself into the room.&n\n",
               ch);
  while (image->affected)
    affect_remove(image, image->affected);
  if (!IS_SET(image->specials.act, ACT_MEMORY))
    clearMemory(image);

  image->only.npc->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);
  image->player.name = str_dup("image");
  image->player.short_descr = str_dup(ch->player.name);

  snprintf(Gbuf1, MAX_STRING_LENGTH, "%s stands here.\n", ch->player.name);
  image->player.long_descr = str_dup(Gbuf1);
  if (GET_TITLE(ch))
    image->player.title = str_dup(GET_TITLE(ch));

  GET_RACE(image) = GET_RACE(ch);
  GET_SEX(image) = GET_SEX(ch);
  GET_ALIGNMENT(image) = GET_ALIGNMENT(ch);
  GET_SIZE(image) = GET_SIZE(ch);
  remove_plushit_bits(image);

  if (char_to_room(image, ch->in_room, 0) && arg && *arg)
  {
    int      dir = search_block(arg, dirs, FALSE);

    if (dir > -1)
    {
      command_interpreter(image, arg);
    }
  }
  CharWait(ch, PULSE_VIOLENCE);
}

void do_fireball(P_char ch, char *arg, int cmd)
{
  P_char   victim;

  if (!(victim = parse_victim(ch, arg, 0)))
  {
    send_to_char("Throw it at who?\n", ch);
    return;
  }

  /*if (!check_innate_time(ch, INNATE_FIREBALL))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }*/

  spell_fireball(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
  if (char_in_list(ch))
    CharWait(ch, PULSE_VIOLENCE * 5);
}

void do_fireshield(P_char ch, char *arg, int cmd)
{
  spell_fireshield(GET_LEVEL(ch), ch, NULL, 0, ch, 0);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_firestorm(P_char ch, char *arg, int cmd)
{
  spell_firestorm(GET_LEVEL(ch), ch, NULL, 0, 0, 0);
  if (char_in_list(ch))
    CharWait(ch, PULSE_VIOLENCE * 4);
}

void do_dimension_door(P_char ch, char *arg, int cmd)
{
  uint     bits;
  P_char   victim;
  P_obj    obj;

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("You can't do this while fighting.\n", ch);
    return;
  }

  if (affected_by_spell(ch, TAG_PVPDELAY))
  {
    send_to_char("There is too much adrenaline pumping through your body right now.\n", ch);
    return;
  }


  bits = generic_find(arg, FIND_CHAR_WORLD, ch, &victim, &obj);

  if (!*arg || !victim || !CAN_SEE(ch, victim))
  {
    send_to_char("Travel to who?\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_SHADOW_DOOR))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  send_to_char("&+LYou begin to travel the shadows..\n", ch);
  spell_dimension_door(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, 0);
}

void do_battle_rage(P_char ch, char *arg, int cmd)
{
  if (!check_innate_time(ch, INNATE_BATTLE_RAGE))
  {
    send_to_char("You are too tired to do that right now!\n", ch);
    return;
  }
  CharWait(ch, PULSE_VIOLENCE * 2);
  act("&+L$n suddenly enters a &+rfe&+Rro&+rci&+Rou&+rs &+Rrage&+L!&N", TRUE,
      ch, 0, 0, TO_ROOM);
  send_to_char
    ("&+LYou feel the &+rR&+RAG&+rE &+Lbegin to &+rp&+Ru&+rm&+Rp &+Lthrough your &+rveins&+L.&N\n",
     ch);
  spell_haste(60, ch, 0, 0, ch, 0);
}

void do_phantasmal_form(P_char ch, char *arg, int cmd)
{
  struct affected_type af, *afp;

  if (IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM))
  {
    afp = get_spell_from_char(ch, TAG_PHANTASMAL_FORM);
    affect_from_char(ch, TAG_PHANTASMAL_FORM);
    wear_off_message(ch, afp);
    return;
  }

  if (!check_innate_time(ch, INNATE_PHANTASMAL_FORM))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  send_to_char
    ("&+LYou feel some of your attachment to the world lessen.&n\n", ch);
  act("&+L$n seems to slightly fade as $s body becomes a phantasmal form.&n",
      FALSE, ch, 0, 0, TO_ROOM);

  memset(&af, 0, sizeof(af));
  af.type = TAG_PHANTASMAL_FORM;
  af.flags = AFFTYPE_SHORT;
  af.duration = 980;
  af.bitvector = AFF_INVISIBLE;
  af.bitvector2 = AFF2_PASSDOOR;
  af.bitvector4 = AFF4_PHANTASMAL_FORM;
  affect_to_char(ch, &af);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_stone_skin(P_char ch, char *arg, int cmd)
{
  if( !check_innate_time(ch, INNATE_STONE, 300) )
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }
  spell_stone_skin(GET_LEVEL(ch), ch, 0, 0, ch, 0);
  CharWait(ch, PULSE_VIOLENCE * 3);
}

void do_shade_movement(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if ((IS_SUNLIT(ch->in_room)) && (!IS_TWILIGHT_ROOM(ch->in_room)))
  {
    send_to_char("It's way to lit!\n", ch);
    return;
  }

  if (IS_AFFECTED5(ch, AFF5_SHADE_MOVEMENT))
  {
    send_to_char("You're already in your alterd form.\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_SHADE_MOVEMENT))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  send_to_char("&+LYou slowly &+wdispate&+L into the shadows.&n\n", ch);
  act("&+L$n seems to slowly &+wdissipates&+L into the shadows&n", FALSE, ch, 0,
      0, TO_ROOM);
  bzero(&af, sizeof(af));
  af.duration = 1 + (int) (GET_LEVEL(ch) / 10);
  af.bitvector5 = AFF5_SHADE_MOVEMENT;
  affect_to_char(ch, &af);

  SET_BIT(ch->specials.affected_by, AFF_HIDE);
  SET_BIT(ch->specials.affected_by, AFF_SNEAK);
}

void event_tempus(P_char ch, P_char victim, P_obj obj, void *args)
{
  struct affected_type af;
  struct affected_type af1;
  char     buf[256];
  int level, dura, room;

  if( !IS_ALIVE(ch) || ch->in_room == NOWHERE )
    return;

  if(affected_by_spell(ch, SPELL_DIVINE_FURY))
  {
    send_to_char("You are already divinely furious!\n", ch);
    return;
  }

  level = GET_LEVEL(ch);
  
  if(!IS_SPECIALIZED(ch))
    level = level / 2;

  if(GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
    dura = (int)(get_property("innate.timer.dura.godCall.Tempus.Cleric", 5));
  else
    dura = (int)(get_property("innate.timer.dura.godCall.Tempus.Other", 3));

  memset(&af, 0, sizeof(af));
  af.type = SPELL_DIVINE_FURY;
  af.duration = dura;
  af.modifier = (int)(level / 5);
  af.flags = AFFTYPE_NODISPEL;
  
  af.location = APPLY_DAMROLL;
  affect_to_char(ch, &af);
  
  af.location = APPLY_HITROLL;
  affect_to_char(ch, &af);
  
  if(GET_CLASS(ch, CLASS_AVENGER) &&
     !IS_SET(ch->specials.affected_by4, AFF4_HOLY_SACRIFICE))
  {
    memset(&af1, 0, sizeof(af1));
    af1.bitvector4 = AFF4_HOLY_SACRIFICE;
    af1.duration = dura;
    af1.modifier = 5;
    af1.flags = AFFTYPE_NODISPEL | AFFTYPE_NOMSG;
    af1.location = APPLY_STR;
    affect_to_char(ch, &af1);
  }

  send_to_char("Your fury rages as your deity infuses you with eternal power!\n", ch);
  snprintf(buf, 256, "%s$n %sis briefly surrounded by a%s glow!",
          IS_EVIL(ch) ? "&+R" : "&+W", IS_EVIL(ch) ? "&+R" : "&+W",
          IS_EVIL(ch) ? "n unholy" : " holy");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
}

void event_lathander(P_char ch, P_char victim, P_obj obj, void *args)
{
  P_char   tch;
  struct   affected_type af;
  int      level;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch || grouped(tch, ch))
    {
      if IS_AFFECTED3(tch, AFF3_ENHANCE_HEALING)
      {
        return;
      }

      level = GET_LEVEL(tch);
      memset(&af, 0, sizeof(af));
      af.type = TAG_ENHANCE_HEALING;
      af.bitvector3 = AFF3_ENHANCE_HEALING;
      af.duration = level / 15;
      affect_to_char(tch, &af);
      send_to_char("&+WSlowly the light engulfs your body, as a greater being starts to watch over you.&n\n", tch);
    }
  }
}

void event_torm(P_char ch, P_char victim, P_obj obj, void *args)
{
  P_char   tch;
  struct affected_type af;
  char buffer[256];

  if (GET_SPEC(ch, CLASS_AVENGER, SPEC_LIGHTBRINGER)) {
    act("&+WA strong feeling of peace seems to radiate from $n.",
        FALSE, ch, 0, 0, TO_ROOM);
    snprintf(buffer, 256, "You feel the power of %s protect you.",
        get_god_name(ch));
    act(buffer, FALSE, ch, 0, 0, TO_CHAR);
    memset(&af, 0, sizeof(af));
    af.type = SPELL_DIVINE_FURY;
    af.bitvector2 = AFF2_SOULSHIELD;
    af.bitvector4 = AFF4_SANCTUARY;
    af.duration = GET_LEVEL(ch)/18;
    affect_to_char_with_messages(ch, &af,
        "Your god no longer protects you.", "");
    return;
  }
/* Allowing multiclass clerics get the forget special. Jan08 -Lucrot
  if (!IS_SPECIALIZED(ch))
  {
    send_to_room("&+WA strong feeling of peace seems to radiate from&n "
                 "&+Wwithin the room.&n\n", ch->in_room);
    StopAllAttackers(ch);
    return;
  }
*/
  if(victim == ch ||
    !IS_SPECIALIZED(ch)) 
  {
    for (tch = character_list; tch; tch = tch->next)
    {
      if(IS_NPC(tch))
      {
        forget(tch, ch);
      }
    }
    send_to_char
      ("&+WPraise Gods, for all your tresspasses have been forgiven!&n\n",
       ch);
    return;
  }

  send_to_room("&+WA strong feeling of peace seems to radiate from&n "
               "&+Wwithin the room.&n\n", ch->in_room);

  for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    stop_fighting(tch);
    if( IS_DESTROYING(tch) )
      stop_destroying(tch);
    update_pos(tch);
    
    if(IS_NPC(tch) &&
       !number(0, 1) &&
       !IS_ELITE(tch))
    {
      act("As a &+Wgentle light&n from above touches $n, $e looks reborn.",
          FALSE, tch, 0, 0, TO_ROOM);
      clearMemory(tch);
    }
  }
}

struct god_list_data_struct
{
  int      race;
  const char *healer;
  const char *holyman;
  const char *zealot;
  const char *theurgist;
};

struct god_list_data_struct god_list_data[] = {
  {RACE_HUMAN,
   "&+cLathander the Morninglord&N", "&+YTorm the True&N",
   "&+rTempus, &+RGod of War&N", ""},

  {RACE_GREY,
   "&+CLabelas &+cEnoreth&N", "&+CFen&+cmarel &+CMesta&+crine&N",
   "&+cAerdrie &+LFaenya&N", ""},

  {RACE_HALFLING,
   "&+YYondalla&N", "&+YUro&+ygal&+wen&N", "&+yArvo&+wre&+Len&N", ""},

  {RACE_GNOME,
   "&+ySe&+wgoj&+Lan &+yEarth&+Lcaller&N", "&+RGaerdal &+wIron&+Lhand&N",
   "&+RGa&+rrl Glitte&+Rrgold&N", ""},

  {RACE_DUERGAR,
   "&+rLadu&+Lguer&N", "&+wDiin&+Lkara&+rzan&N", "&+rDii&+wrin&+rka&N", ""},

  {RACE_DROW,
   "&+MEil&+mistr&+Laee&N", "&+mLloth &+Lthe Spider &+mQueen&N",
   "&+LZinze&+mrena&N", ""},

  {RACE_ORC,
   "&+LLuthic&N", "&+LYurt&+wrus&N", "&+gBahg&+Ltru&N", ""},

  {RACE_OROG,
   "&+yYurtrus&N", "&+GShargaas&N", "&+rIlneval&N", ""},

  {RACE_GOBLIN,
   "&+GMaglub&+giyet&N", "&+gKhurg&+worba&+Leyag&N", "&+GBa&+grgriv&+Lyek&N", ""},

  {RACE_MOUNTAIN,
   "&+yBerronar &+wTrue&+Wsilver&N", "&+YClangeddin &+WSilver&+wbeard&N", "&+YMoradin&N", ""},
   
  {RACE_BARBARIAN,
   "&+cA&+Cu&+cr&+Ci&+cl&N", "&+GBhal&+gla&n", "&+RUt&+rhg&+Lar&n", ""},
   
  {RACE_TROLL,
   "&+cLathander the Morninglord&N", "&+YTorm the True&N", "&+GGranf&+galkor&n", ""},
   
  {RACE_GITHYANKI,
   "&+cLathander the Morninglord&N", "&+YTorm the True&N", "&+WIkkra&+ctalic&n", ""},
   
  {RACE_GITHZERAI,
   "&+cLathander the Morninglord&N", "&+GAlixxak&+Wprok&n", "&+RGizar&+rkromik&n", ""},

  {RACE_NONE,
   "&+cLathander the Morninglord&N", "&+YTorm the True&N",  "&+rTempus, &+RGod of War&N", "&+CHeir&+Wo&+Cni&+Wo&+Cus"}

};

const char *get_god_name(P_char ch)
{
  int i;

  if( GET_CLASS( ch, CLASS_BLIGHTER) )
  {
    return "&+yFa&+Lluz&+yure&n";
  }

  for (i = 0; god_list_data[i].race != RACE_NONE; i++)
    if (GET_RACE(ch) == god_list_data[i].race)
      break;

  if (GET_SPEC(ch, CLASS_CLERIC, SPEC_HEALER))
    return god_list_data[i].healer;
  else if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
    return god_list_data[i].zealot;
  else if (GET_SPEC(ch, CLASS_CLERIC, SPEC_HOLYMAN))
    return god_list_data[i].holyman;

  if (GET_SPEC(ch, CLASS_AVENGER, SPEC_INQUISITOR))
    return god_list_data[i].zealot;
  else if (GET_SPEC(ch, CLASS_AVENGER, SPEC_LIGHTBRINGER))
    return god_list_data[i].holyman;
  
  if (GET_CLASS(ch, CLASS_THEURGIST))
    return god_list_data[RACE_NONE].theurgist;
  
  return god_list_data[i].healer;
}

void do_god_call(P_char ch, char *args, int cmd)
{
  int          choice;
  const char  *god_name;
  char         Gbuf1[MAX_STRING_LENGTH];
  int          duration = 0;

  if(IS_NPC(ch)) // NPC torm god causing crashes. Removing npc godcall for now.
  {
    return;
  }
  
  god_name = get_god_name(ch);

  if (GET_CLASS(ch, CLASS_CLERIC) && IS_SPECIALIZED(ch))
     duration = get_property("innate.timer.secs.godCallSpec", 600);

  if(!check_innate_time(ch, INNATE_GOD_CALL, duration))
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH,
        "%s &+Ccannot answer your prayers. Trust your own strength.&n\n",
        god_name);
    send_to_char(Gbuf1, ch);
    return;
  }

  if (GET_SPEC(ch, CLASS_AVENGER, SPEC_INQUISITOR))
    choice = SPEC_ZEALOT;
  else if (GET_SPEC(ch, CLASS_AVENGER, SPEC_LIGHTBRINGER))
    choice = SPEC_HOLYMAN;
  else if (IS_SPECIALIZED(ch))
    choice = ch->player.spec;
  else
    choice = number(1, 3);

  switch (choice)
  {
  case SPEC_HEALER:
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "&+CAs $n raises $s hands sending a prayer to %s, &+Ca &+Wsoft glow&n &+Cdescends from above.&n",
            god_name);
    act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "&+CAs you raise your hands and send a prayer to %s, &+Ca &+Wsoft glow&n &+Cdescends from above.&n",
            god_name);
    act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
    add_event(event_lathander, PULSE_VIOLENCE / 2, ch, get_char_room_vis(ch, args), 0, 0, 0, 0);
    break;
  case SPEC_HOLYMAN:
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "&+wAs $n raises $s hands calling for aid of %s, &+wa &+Wsoft glow&n &+wdescends from above bringing peace and safety.&n",
            god_name);
    act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "&+wAs you raise your hands calling for aid of %s, &+wa &+Wsoft glow&n &+wdescends from above bringing peace and safety.&n",
            god_name);
    act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
    if (ch == get_char_room_vis(ch, args))
      add_event(event_torm, PULSE_VIOLENCE / 2, ch, ch, 0, 0, 0, 0);
    else
      add_event(event_torm, PULSE_VIOLENCE / 2, ch, 0, 0, 0, 0, 0);
    break;
  case SPEC_ZEALOT:
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "&+wAs $n raises $s hands calling for aid of %s, &+wa &+rpurple haze&n &+wdescends from above waking &+RA&+rnG&+ReR&n and &+RF&+ruR&+Ry!&n",
            god_name);
    act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "&+wAs you raise your hands calling for aid of %s, &+wa &+rpurple haze&n &+wdescends from above waking &+RA&+rnG&+ReR&n and &+RF&+ruR&+Ry!&n",
            god_name);
    act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
    add_event(event_tempus, PULSE_VIOLENCE / 2, ch, ch, 0, 0, 0, 0);
    break;
  default:
    break;
  }
}

void do_doorbash(P_char ch, char *arg, int cmd)
{
  char     buf[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      dir, chance, a, b;

  if (!ch)
    return;

  if (!has_innate(ch, INNATE_DOORBASH))
  {
    send_to_char("Um.. sorry, I think you don't feel massive enough!\n",
                 ch);
    return;
  }
  if (!ch->desc)
    return;

  if (IS_RIDING(ch))
  {
    send_to_char("While mounted? I don't think so...\n", ch);
    return;
  }
  one_argument(arg, buf);
  if (!*buf)
  {
    send_to_char("Which door you wish to bash down?\n", ch);
    return;
  }

  dir = dir_from_keyword(buf);
  if (dir == -1)
  {
    send_to_char("Duh, _which_ door did you want to bash, anyway?\n", ch);
    return;
  }

  if (special(ch, exitnumb_to_cmd(dir), 0))
    return;

  if (!EXIT(ch, dir))
  {
    send_to_char("You see no exit in that direction!\n", ch);
    return;
  }
  if (!IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) ||
      (!IS_TRUSTED(ch) &&
       (IS_SET(EXIT(ch, dir)->exit_info, EX_SECRET) ||
        IS_SET(EXIT(ch, dir)->exit_info, EX_BLOCKED))))
  {
    send_to_char("You see no exit in that direction!\n", ch);
    return;
  }
  if (!IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED))
  {
    send_to_char("You feel stupid, trying to bash an open door down?\n",
                 ch);
    return;
  }
  send_to_char
    ("You charge the door, fully intent on getting it open, FOR GOOD!\n",
     ch);
  snprintf(Gbuf1, MAX_STRING_LENGTH,
          "$n&n screws $s eyes in concentration, and decides that the %s to the %s is in $s way!",
          "door", dirs[dir]);
  act(Gbuf1, TRUE, ch, 0, 0, TO_ROOM);
  act("$n&n charges it, fully intent on nuking it for good!", TRUE, ch, 0, 0,
      TO_ROOM);
  /*
   * Even dumb-ish warriortypes _don't_ go thru some doors..
   */

  chance = 40 + GET_LEVEL(ch);

  if (IS_SET(EXIT(ch, dir)->exit_info, EX_PICKPROOF) ||
      (EXIT(ch, dir)->to_room == NOWHERE) ||
      isname("_nobash_", EXIT(ch, dir)->keyword))
  {
    send_to_char("The door doesn't budge, but your body does!\n", ch);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "$n charges the %s to the %s at full swing, yet bounces back, door intact, body hurting.",
            "door", dirs[dir]);
    act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);

    if (damage(ch, ch, dice(3, number(2, 10)), TYPE_UNDEFINED))
      return;
  }
  else if (number(1, 100) <= chance)
  {
    send_to_char
      ("You did it!  The door budges under your weight, and you charge into a new place!\n",
       ch);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "The %s gives way with a nice crash, and $n&n charges to the %s with the remnants of the door still clinging to $m!",
            "door", dirs[dir]);
    act(Gbuf1, TRUE, ch, 0, 0, TO_ROOM);

    REMOVE_BIT(EXIT(ch, dir)->exit_info, EX_LOCKED);
    REMOVE_BIT(EXIT(ch, dir)->exit_info, EX_CLOSED);

    a = EXIT(ch, dir)->to_room;
    b = ch->in_room;
    char_from_room(ch);
    if (!char_to_room(ch, a, -1))
      return;                   /* have to fix later */

    act
      ("With a great crash, $n&n charges into room, splinters of a door still clinging to $m!",
       TRUE, ch, 0, 0, TO_ROOM);

    if ((EXIT(ch, rev_dir[dir])) && (EXIT(ch, rev_dir[dir])->to_room == b))
    {
      REMOVE_BIT(EXIT(ch, rev_dir[dir])->exit_info, EX_LOCKED);
      REMOVE_BIT(EXIT(ch, rev_dir[dir])->exit_info, EX_CLOSED);
      REMOVE_BIT(EXIT(ch, rev_dir[dir])->exit_info, EX_SECRET);
    }
    if (damage(ch, ch, dice(2, number(1, 10)), TYPE_UNDEFINED))
      return;
  }
  else
  {
    send_to_char
      ("You charge it, but it doesn't seem much damaged compared to you!\n",
       ch);

    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "$n charges the %s to the %s, but it withstands the attack!",
            "door", dirs[dir]);
    act(Gbuf1, TRUE, ch, 0, 0, TO_ROOM);
    if (damage(ch, ch, dice(3, number(2, 8)), TYPE_UNDEFINED))
      return;
  }

  CharWait(ch, 2 * PULSE_VIOLENCE);
}

/* doorkick (for centaurs) - like doorbash, except centaur takes a bit less
   damage and doesn't move through door */

void do_doorkick(P_char ch, char *arg, int cmd)
{
  char     buf[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      dir, chance, a, b;

  if (!ch)
    return;

  if (!has_innate(ch, INNATE_DOORKICK))
  {
    send_to_char
      ("Um.. sorry, your rear legs just couldn't take the strain.\n", ch);
    return;
  }
  if (!ch->desc)
    return;

  if (IS_RIDING(ch))
  {
    send_to_char("While mounted? I don't think so...\n", ch);
    return;
  }
  one_argument(arg, buf);
  if (!*buf)
  {
    send_to_char("What direction is the door you wish to kick down?\n", ch);
    return;
  }

  dir = dir_from_keyword(buf);
  if (dir == -1)
  {
    send_to_char("Duh, _what_ door did you want to kick, anyway?\n", ch);
    return;
  }

  if (special(ch, exitnumb_to_cmd(dir), 0))
    return;

  if (!EXIT(ch, dir))
  {
    send_to_char("You see no exit in that direction!\n", ch);
    return;
  }
  if (!IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) ||
      (!IS_TRUSTED(ch) &&
       (IS_SET(EXIT(ch, dir)->exit_info, EX_SECRET) ||
        IS_SET(EXIT(ch, dir)->exit_info, EX_BLOCKED))))
  {
    send_to_char("You see no exit in that direction!\n", ch);
    return;
  }
  if (!IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED))
  {
    send_to_char("You feel stupid, trying to kick an open door down?\n",
                 ch);
    return;
  }
  send_to_char
    ("You kick the door with your hind legs, fully intent on getting it open, FOR GOOD!\n",
     ch);
  snprintf(Gbuf1, MAX_STRING_LENGTH,
          "$n&n's hindquarters tense up, preparing to kick the %s to the %s...",
          "door", dirs[dir]);
  act(Gbuf1, TRUE, ch, 0, 0, TO_ROOM);
  act("$n&n's rear legs slam into the door with incredible force!", TRUE, ch,
      0, 0, TO_ROOM);
  /*
   * Even dumb-ish warriortypes _don't_ go thru some doors..
   */

  chance = 45 + GET_LEVEL(ch);

  if (IS_SET(EXIT(ch, dir)->exit_info, EX_PICKPROOF) ||
      (EXIT(ch, dir)->to_room == NOWHERE) ||
      isname("_nobash_", EXIT(ch, dir)->keyword))
  {
    send_to_char
      ("The door doesn't budge, and your rear legs feel none the better..\n",
       ch);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "$n&n's rear legs bounce off the %s with no noticable result.",
            "door");
    act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
#if 0
    GET_HIT(ch) -= dice(2, number(2, 10));      /*
                                                 * Fellow _CAN_ get fairly
                                                 * badly hurt if not
                                                 * lucky.
                                                 */
    StartRegen(ch, EVENT_HIT_REGEN);
#endif
    if (damage(ch, ch, dice(2, number(2, 10)), TYPE_UNDEFINED))
      return;
  }
  else if (number(1, 100) <= chance)
  {
    send_to_char
      ("You did it!  The door shatters under the enormous force of your kick!\n",
       ch);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "The %s gives way under the incredible force of $n&n's kick!",
            "door");
    act(Gbuf1, TRUE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT(EXIT(ch, dir)->exit_info, EX_LOCKED);
    REMOVE_BIT(EXIT(ch, dir)->exit_info, EX_CLOSED);

    /* the player doesn't actually move when kicking, but to make things
       easy, move him/her/it temporarily, then move him/her/it back */

    a = EXIT(ch, dir)->to_room;
    b = ch->in_room;

    char_from_room(ch);
    if (!char_to_room(ch, a, -1))
      return;

    snprintf(Gbuf1, MAX_STRING_LENGTH, "With a great crash, the door to the %s is destroyed!",
            dirs[rev_dir[dir]]);
    act(Gbuf1, TRUE, ch, 0, 0, TO_ROOM);

    if ((EXIT(ch, rev_dir[dir])) && (EXIT(ch, rev_dir[dir])->to_room == b))
    {
      REMOVE_BIT(EXIT(ch, rev_dir[dir])->exit_info, EX_LOCKED);
      REMOVE_BIT(EXIT(ch, rev_dir[dir])->exit_info, EX_CLOSED);
      REMOVE_BIT(EXIT(ch, rev_dir[dir])->exit_info, EX_SECRET);
    }
    char_from_room(ch);
    if (!char_to_room(ch, b, -1))
      return;

/*
    GET_HIT(ch) -= dice(2, 5);
    StartRegen(ch, EVENT_HIT_REGEN);
*/
    if (damage(ch, ch, dice(2, 5), TYPE_UNDEFINED))
      return;
  }
  else
  {
    send_to_char
      ("You kick the door with all your might, but alas, it seems unfazed.\n",
       ch);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "$n kicks the %s to the %s, but alas, it seems unaffected.",
            "door", dirs[dir]);
    act(Gbuf1, TRUE, ch, 0, 0, TO_ROOM);
/*
    GET_HIT(ch) -= dice(2, number(1, 10));
    StartRegen(ch, EVENT_HIT_REGEN);
*/
    if (damage(ch, ch, dice(2, number(1, 10)), TYPE_UNDEFINED))
      return;
  }
  CharWait(ch, 2 * PULSE_VIOLENCE);
}

/*
 * This is halfling dreaded social-thievery table.  otherwise usual
 * WEAR-flags, except 0 = random instead of WEAR_LIGHT
 */

static int steal_chance[][21] = {
  {
   9, WEAR_FACE,
   50, WEAR_EYES,
   70, WEAR_EARRING_L,
   80, WEAR_EARRING_R,
   90, WEAR_NECK_1,
   95, WEAR_NECK_2, 100},
  {
   24, WEAR_HANDS,
   20, WEAR_FINGER_L,
   30, WEAR_FINGER_R,
   40, WEAR_WAIST,
   60, WEAR_WRIST_L,
   70, WEAR_WRIST_R,
   80, PRIMARY_WEAPON, 100},
  {
   29, WEAR_HANDS,
   30, WEAR_FINGER_L,
   45, WEAR_FINGER_R,
   60, WEAR_WRIST_L,
   80, WEAR_WRIST_R, 100},
  {CMD_HUG, 0, 100},
  {CMD_POKE, 0, 100},
  {CMD_GROPE, 0, 100},
  {CMD_NIBBLE, 0, 100},
  {CMD_RUFFLE, 0, 100},
  {CMD_SLAP, 0, 100},
  {CMD_SQUEEZE, 0, 100},
  {CMD_KISS, 0, 100},
  {CMD_COMB, 0, 100},
  {CMD_MASSAGE, 0, 100},
  {CMD_TICKLE, 0, 100},
  {CMD_PAT, 0, 100},
  {CMD_NUDGE, 0, 100},
  {CMD_PUNCH, 0, 100},
  {CMD_SPANK, 0, 100},
  {CMD_TACKLE, 0, 100},
  {CMD_BONK, 0, 100},
  {CMD_PINCH, 0, 100},
  {CMD_PUSH, 0, 100},
  {CMD_SHOVE, 0, 100},
  {CMD_STRANGLE, 0, 100},
  {CMD_TAP, 0, 100},
  {CMD_TWEAK, 0, 100},
  {CMD_CARESS, 0, 100},
  {CMD_SWEEP, 0, 100},
  {CMD_TOUCH, 0, 100},
  {CMD_SCRATCH, 0, 100},
  {CMD_BATHE, 0, 100},
  {CMD_EMBRACE, 0, 100},
  {CMD_TUG, 0, 100},
  {CMD_HI5, 0, 100},
  {CMD_WHAP, 0, 100},
  {CMD_ROLL, 0, 100},
  {CMD_DROPKICK, 0, 100},
  {CMD_NOOGIE, 0, 100},
  {CMD_MELT, 0, 100},
  {CMD_BIRD, 0, 100},
  {CMD_BITE, 0, 100},
  {CMD_SULK, 0, 100},
  {CMD_SWAT, 0, 100},
  {CMD_CHEEK, 0, 100},
  {CMD_HUG, 0, 100},
  {CMD_TANGO, 0, 100},
  {CMD_CUDDLE, 0, 100},
  {CMD_NUZZLE, 0, 100},
  {CMD_FONDLE, 0, 100},
  {CMD_HAND, 0, 100},
  {CMD_COMFORT, 0, 100},
  {CMD_SNUGGLE, 0, 100},
  {CMD_SNAP, 0, 100},
  {0}
};

/*
void halfling_stealaction(P_char ch, char *arg, int cmd)
{
  char buf[MAX_INPUT_LENGTH];
  P_char vict;
  P_obj obj = NULL;
  int percent, roll, loc, a, b;
  bool failed, caught, victout = FALSE;

  if (!HAS_INNATE(ch, INNATE_SOCIAL_STEAL)) {
    return;
  }
  if (!CAN_SEE(ch, ch))
    return;

  if (!number(0, 1))
    return;

  one_argument(arg, buf);
  if (!*buf) {
    return;
  }
  if (!(vict = get_char_room_vis(ch, buf))) {
    return;
  }
  if (vict == ch) {
    return;
  }
  if (IS_PC(vict) && !vict->desc && GET_LEVEL(ch) <= MAXLVLMORTAL) {
    send_to_char("Not while they're link-dead, you don't...\n", ch);
    return;
  }
  a = 0;
  for (b = 0; (steal_chance[b][0] && !a); b++)
    if (steal_chance[b][0] == cmd)
      a = b + 1;
  if (!a)
    return;
  if ((IS_RIDING(ch)) ||
      ((GET_LEVEL(vict) > MAXLVLMORTAL) && IS_PC(vict)) ||
      ((IS_CARRYING_N(ch) + 1) > CAN_CARRY_N(ch)) ||
      (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch)) ||
      (CHAR_IN_SAFE_ZONE(ch)) || IS_TRUSTED(ch) ||
      (IS_ROOM(ch->in_room, ROOM_SINGLE_FILE) &&
    !AdjacentInRoom(ch, vict)) || IS_FIGHTING(ch) || IS_FIGHTING(vict) ||
      (!IS_NPC(vict) && !vict->desc && (GET_LEVEL(ch) <= MAXLVLMORTAL)))
    return;
  percent = (GET_LEVEL(ch) * 100 / (GET_LEVEL(ch) + GET_LEVEL(vict)));
  percent += dex_app[STAT_INDEX(GET_C_DEX(ch))].p_pocket;
  percent -= (STAT_INDEX(GET_C_WIS(vict)) + STAT_INDEX(GET_C_INT(vict))) - 19;
  if (IS_AFFECTED(vict, AFF_AWARE) ||
      affected_by_spell(vict, SKILL_AWARENESS))
    percent -= 100;
  if (!CAN_SEE(vict, ch))
    percent += 40;
  if ((GET_STAT(vict) < STAT_SLEEPING) || IS_AFFECTED(vict, AFF_SLEEP) ||
      IS_AFFECTED(vict, AFF_BOUND) ||
      IS_AFFECTED(vict, AFF_KNOCKED_OUT) ||
      IS_AFFECTED2(vict, AFF2_MINOR_PARALYSIS) ||
      IS_AFFECTED2(vict, AFF2_MAJOR_PARALYSIS))
    percent += 200;
  else if (IS_AFFECTED2(vict, AFF2_STUNNED))
    percent += 20;
  else if (GET_STAT(vict) == STAT_SLEEPING)
    percent += 40;
  if (GET_LEVEL(vict) > MAXLVLMORTAL)
    percent = 0;
  if (!GET_CLASS(ch, CLASS_THIEF))
    percent = percent / 2;

  roll = number(1, 100);
  caught = FALSE;
  failed = FALSE;
  loc = 0;
  for (b = 1; steal_chance[a - 1][b * 2] < roll; b++);
  loc = steal_chance[a - 1][b * 2 - 1] + 1;
  if (loc < 1)
    return;
  if (loc == 1)
    loc = number(2, WEAR_QUIVER + 1);
  loc--;
  if (!vict->equipment[loc]) {
    send_to_char("You cannot resist searching for items, yet you find nothing of interest!\n", ch);
    failed = TRUE;
    percent += 50;
  } else {
    obj = vict->equipment[loc];
    if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
      failed = TRUE;
    if (!failed && (percent > 175)) {
      act("You suddenly feel like relieving $N of $S spare equipment.. ",
          FALSE, ch, obj, vict, TO_CHAR);
      act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
      obj_to_char(unequip_char(vict, loc), ch);
      percent -= GET_OBJ_WEIGHT(obj);
      notch_skill(ch, SKILL_STEAL, 5);
    } else {
      send_to_char("Uh huh.. You think your instincts got better of you!\n", ch);
      failed = TRUE;
      caught = TRUE;
    }
  }
  CharWait(ch, 24);

  if ((percent < 0) || MIN(100, percent) < number(-60, 100))
    caught = TRUE;
  if (!caught) {
    if (!failed)
      send_to_char("Heh heh, got away clean too!\n", ch);
    else
      send_to_char("Well, at least nobody saw that!\n", ch);
    return;
  }
  if (!IS_NPC(ch)) {
  }
  justice_witness(ch, vict, failed ? CRIME_ATT_THEFT : CRIME_THEFT);

  if ((GET_STAT(vict) < STAT_SLEEPING) || IS_AFFECTED(vict, AFF_SLEEP) ||
      IS_AFFECTED(vict, AFF_KNOCKED_OUT) ||
      IS_AFFECTED2(vict, AFF2_MINOR_PARALYSIS) ||
      IS_AFFECTED2(vict, AFF2_MAJOR_PARALYSIS)) {
    send_to_char("Good thing your victim is in no shape to catch you!\n", ch);
    victout = TRUE;
//    return;
  } else if (IS_AFFECTED2(vict, AFF2_STUNNED)) {
    send_to_char("Damn!  Hard to believe they let you into the guild!\n", ch);
  } else if (GET_STAT(vict) == STAT_SLEEPING) {
    send_to_char("Groping fingers disturb your rest!\n", vict);
    send_to_char("Uh oh, looks like you weren't quite as careful as you should have been!\n", ch);
    do_wake(vict, 0, 0);
  } else {
    send_to_char("Ooops, better be more careful next time (assuming you survive...)\n", ch);
  }
  if (!failed) {
    if (!victout) act("&+WHey! $n just stole your $q!&n", FALSE, ch, obj, vict, TO_VICT);
    act("$n just stole $p from $N!", TRUE, ch, obj, vict, TO_NOTVICT);
  } else if (obj) {
    act("&+WHey! $n just tried to steal your $q!&n", FALSE, ch, obj, vict, TO_VICT);
    act("$n just tried to steal something from $N!",
        TRUE, ch, obj, vict, TO_NOTVICT);
  }
  if (!CAN_SEE(vict, ch) || IS_PC(vict) ||
      IS_SET(vict->specials.act, ACT_NICE_THIEF))
    return;
  remember(vict, ch);
  if( !IS_FIGHTING(vict) && !IS_DESTROYING(vict) )
    MobStartFight(vict, ch);
}
*/

void do_tupor(P_char ch, char *arg, int cmd)
{
  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( !has_innate(ch, INNATE_TUPOR) )
  {
    send_to_char("You have no idea how to even begin.\n", ch);
    return;
  }
  if( IS_AWAKE(ch) )
  {
    send_to_char("You must be sleeping to begin your trance.\n", ch);
    return;
  }
  if( IS_AFFECTED4(ch, AFF4_TUPOR) )
  {
    send_to_char("You are already in your trance.\n", ch);
    return;
  }

  SET_BIT(ch->specials.affected_by4, AFF4_TUPOR);
  StartRegen(ch, EVENT_MANA_REGEN);
  send_to_char("You slip into a death-like trance...\n", ch);
  act("$n slips into a death-like trance...", TRUE, ch, 0, 0, TO_ROOM);
}

void do_breathe(P_char ch, char *arg, int cmd)
{
  P_char   victim;
  char     buf[80];
  int      dir;
  struct damage_messages messages = {
    "Your lungs burst forth with the fury of the &+CNorth Wind&n!\n",
    "A burst of &+Cfreezing wind&n suddenly erupts from $n's mouth, hitting you!",
    "A burst of &+Cfreezing wind&n suddenly erupts from $n's mouth, hitting $N!",
    "Your breath kills $N instantly!  You say a quick silent prayer to the Northern Deities.",
    "That breath is quite painful - darkness overwhelms you..",
    "$N is killed instantly!  That's gotta hurt."
  };

  if( has_innate(ch, INNATE_BARB_BREATH) )
  {
    if( !(victim = parse_victim(ch, arg, 0)) )
    {
      send_to_char("Target who?\n", ch);
      return;
    }

    if( ch == victim )
    {
      send_to_char("Your breath suddenly fails you..  Perhaps the Great Gods of the North have other plans for you.\n", ch);
      return;
    }
    if( !check_innate_time(ch, INNATE_BARB_BREATH) )
    {
      send_to_char("You're too tired to do that right now.\n", ch);
      return;
    }
    spell_damage(ch, victim, dice(GET_LEVEL(ch), 4), SPLDAM_COLD,
      SPLDAM_NOSHRUG | SPLDAM_NODEFLECT | SPLDAM_BREATH, &messages);
    act("The wind dissipates, but a horrible stench remains..", TRUE, ch, 0, 0, TO_NOTVICT);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  if( !CAN_BREATHE(ch) )
  {
    if( IS_TRUSTED(ch) )
    {
      arg = one_argument(arg, buf);
      if( !(victim = parse_victim(ch, arg, 0)) )
      {
        send_to_char("Target who?\n", ch);
        return;
      }
      if( !strcmp( buf, "fire" ) )
      {
        breath_weapon_fire( GET_LEVEL(ch), ch, arg, 0, victim, NULL );
        return;
      }
      if( !strcmp( buf, "lightning" ) )
      {
        breath_weapon_lightning( GET_LEVEL(ch), ch, arg, 0, victim, NULL );
        return;
      }
      if( !strcmp( buf, "frost" ) )
      {
        breath_weapon_frost( GET_LEVEL(ch), ch, arg, 0, victim, NULL );
        return;
      }
      if( !strcmp( buf, "acid" ) )
      {
        breath_weapon_acid( GET_LEVEL(ch), ch, arg, 0, victim, NULL );
        return;
      }
      if( !strcmp( buf, "poison" ) )
      {
        breath_weapon_poison( GET_LEVEL(ch), ch, arg, 0, victim, NULL );
        return;
      }
    }
    send_to_char("Tis an involuntary action for most, but ne'ertheless, you breathe just to make sure.\n", ch);
    return;
  }
  else
  {
    if (arg || *arg)
    {
      one_argument(arg, buf);
      dir = search_block(buf, dirs, FALSE);
    }
    else
      dir = -1;
    BreathWeapon(ch, dir);
    if (IS_MORPH(ch))
      CharWait(ch, 9);
    return;
  }
}

void do_stomp(P_char ch, char *arg, int cmd)
{
  /*
   * This could very well be expanded to include a char skill
   */
  if (IS_PC(ch))
  {
    send_to_char
      ("You stomp on the ground like a sumo wrestler..\n",
       ch);
    return;
  }
  else if (GET_RACE(ch) == RACE_TITAN ||
           GET_RACE(ch) == RACE_AVATAR)
  {
    StompAttack(ch);
    if (IS_PC_PET(ch))
      CharWait(ch, PULSE_VIOLENCE);
    return;
  }
  else
  {
    send_to_char
      ("You stomp away...\n",
       ch);
    return;
  }
}

void do_sweep(P_char ch, char *arg, int cmd)
{

  // This could very well be expanded to include a char skill
  if( IS_PC(ch) )
  {
    send_to_char("You attempt a nifty martial-art move, and fall on your face.\n", ch);
    return;
  }
  else if( IS_DRAGON(ch) )
  {
    SweepAttack(ch);
    if( IS_PC_PET(ch) )
    {
      CharWait(ch, PULSE_VIOLENCE);
    }
    return;
  }
  else
  {
    send_to_char("If I knew the length of your tail, perhaps I would allow this...\n", ch);
    return;
  }
}

void do_innate(P_char ch, char *arg, int cmd)
{
  int      i;
  char     innate_name[MAX_STRING_LENGTH], innate_args[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH];

  if (cmd == CMD_SUMMON)
  {
    if (epic_summon(ch, arg))
    {
      return;
    }
    else
    {
      strcpy(innate_name, "summon ");
    }
  }
  else
  {
    innate_name[0] = '\0';
  }

  argument_interpreter(arg, innate_name + strlen(innate_name), innate_args);
  if (!innate_name || *innate_name == '\0')
  {
    /*
     * List available racial abilities
     */
    send_to_char("Your innate abilities: ('*' marked innates are always active)\n", ch);
    for( i = 0; i <= LAST_INNATE; i++ )
    {
      if( has_innate(ch, i) )
      {
        if( innates_data[i].func )
        {
          snprintf(buf, MAX_STRING_LENGTH, "   %s", innates_data[i].name);
        }
        else
        {
          snprintf(buf, MAX_STRING_LENGTH, "  *%s", innates_data[i].name);
        }

        if (can_use_innate(ch, i))
        {
          strcat(buf, "\n");
        }
        else
        {
          strcat(buf, " [too tired to use again]\n");
        }
        send_to_char(buf, ch);
      }
    }
    return;
  }

  if( is_abbrev(innate_name, "list") )
  {
    do_list_innates( ch, innate_args );
    return;
  }

  for (i = 0; i <= LAST_INNATE; i++)
  {
    if (is_abbrev(innate_name, innates_data[i].name) && has_innate(ch, i))
    {
      break;
    }
  }

  if (i <= LAST_INNATE)
  {
    if (innates_data[i].func)
    {
      (innates_data[i].func) (ch, innate_args, CMD_INNATE);
    }
    else
    {
      send_to_char("This innate is always active.\n", ch);
    }
    return;
  }
  else
    send_to_char("You know no such ability!\n", ch);
}

void event_conjure_water(P_char ch, P_char vict, P_obj obj, void *data)
{
  P_obj    spring;

  send_to_room
    ("A &+bsmall spring&n shoots from the &+yground&n obeying a powerful summoning.\n",
     ch->in_room);
  spring = read_object(93300, VIRTUAL);
  if (!spring)
  {
    logit(LOG_DEBUG, "spell_create_spring(): obj 750 (spring) not loadable");
    send_to_char("Tell someone to make a spring object ASAP!\n", ch);
    return;
  }
  obj_to_room(spring, ch->in_room);
  set_obj_affected(spring, 15 * 60 * 4, TAG_OBJ_DECAY, 0);
}

void do_conjure_water(P_char ch, char *arg, int cmd)
{
  if (!check_innate_time(ch, INNATE_CONJURE_WATER))
  {
    send_to_char("You're too tired to do that right now.\n", ch);
    return;
  }

  act
    ("&+LThe earth rumbles as &N$n's&+L incantation summons &+bliving water&+L from the depths.",
     TRUE, ch, 0, ch, TO_ROOM);
  send_to_char
    ("&+LThe earth rumbles as your incantation summons &+bliving water&+L from the depths.\n",
     ch);
  add_event(event_conjure_water, 2 * PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
  CharWait(ch, 2 * PULSE_VIOLENCE);
}

// ------ Foundry ---------

void event_foundry(P_char ch, P_char vict, P_obj obj, void *data)
{
  P_obj    forge;

  act("&+LWithin just a few moments you have finished setting up a small yet useful work center.&n", FALSE, ch, 0, 0, TO_CHAR);
  act("&+LWithin just a few moments $n &+Lhas finished setting up a small yet useful work center.&n", TRUE, ch, 0, ch, TO_ROOM);
  forge = read_object(361, VIRTUAL);
  if (!forge)
  {
    logit(LOG_DEBUG, "create_forge(): obj 361 (forge/furnace) not loadable");
    send_to_char("Tell someone to make a forge object ASAP!\n", ch);
    return;
  }
  obj_to_room(forge, ch->in_room);
  set_obj_affected(forge, 15 * 60 * 4, TAG_OBJ_DECAY, 0);
}

void do_foundry(P_char ch, char *arg, int cmd)
{

 if (!check_innate_time(ch, INNATE_FOUNDRY))
 {
    send_to_char("&+LYou are still too tired to construct a foundry.&n", ch);
    return;
 }

  act("&+L$n &+Lopens his toolkit and hastily begins construction.&n", TRUE, ch, 0, ch, TO_ROOM);
  act("&+LYou open your toolkit and eagerly begin construction on a work center.&n", FALSE, ch, 0, 0, TO_CHAR);
  add_event(event_foundry, 2 * PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
  CharWait(ch, 2 * PULSE_VIOLENCE);
}

// ------ End Foundry ---------

int attuned_to_terrain(P_char ch)
{
  if (!has_innate(ch, INNATE_ELEMENTAL_BODY))
    return 0;

  switch (GET_RACE(ch))
  {
    case RACE_F_ELEMENTAL:
      switch (world[ch->in_room].sector_type)
      {
        case SECT_FIREPLANE:
          return 8;
        case SECT_PLANE_OF_AVERNUS:
          return 7;
        case SECT_LAVA:
          return 6;
      }
      if (IS_UNDERWATER(ch))
        return 0;
        
      return 1;

    case RACE_A_ELEMENTAL:
      switch (world[ch->in_room].sector_type)
      {
        case SECT_AIR_PLANE:
          return 8;
        case SECT_NO_GROUND:
          return 4;
        case SECT_EARTH_PLANE:
          return 0;
      }
      return 1;

    case RACE_W_ELEMENTAL:
      switch (world[ch->in_room].sector_type)
      {
        case SECT_WATER_PLANE:
          return 8;
        case SECT_UNDRWLD_NOSWIM:
        case SECT_OCEAN:
        case SECT_UNDERWATER:
        case SECT_UNDERWATER_GR:
        case SECT_WATER_NOSWIM:
          return 5;
       case SECT_UNDRWLD_WATER: 
       case SECT_WATER_SWIM:
          return 4;
       case SECT_SWAMP:
          return 3;
       case SECT_FIREPLANE:
       case SECT_LAVA:
          return 0;
      }
      return 1;

    case RACE_E_ELEMENTAL:
      switch (world[ch->in_room].sector_type)
      {
        case SECT_EARTH_PLANE:
          return 8;
        case SECT_UNDRWLD_MOUNTAIN:
        case SECT_UNDRWLD_LOWCEIL:
          return 5;
        case SECT_MOUNTAIN:
          return 4;
        case SECT_HILLS:
        case SECT_UNDRWLD_WILD:
          return 2;
        case SECT_AIR_PLANE:
          return 0;
      }
      return 1;
    default:
      return 0;
  }
  return 0;
}

int get_innate_regeneration(P_char ch)
{
  int mult = 1, terr;

  switch(GET_RACE(ch))
  {
    case RACE_TROLL:
      if( affected_by_spell(ch, TAG_TROLL_BURN) )
        return 1;
      else
        mult += (int)regen_factor[REG_TROLL];
      break;
    case RACE_REVENANT:
        mult += regen_factor[REG_REVENANT];
      break;
    default:
      break;
  }

  if( GET_SPEC(ch, CLASS_RANGER, SPEC_HUNTSMAN) && IS_FOREST_ROOM(ch->in_room) )
    mult += regen_factor[REG_HUNTSMAN];

  if( GET_SPEC(ch, CLASS_CONJURER, SPEC_WATER) )
    mult += regen_factor[REG_WATERMAGUS];

  mult += attuned_to_terrain(ch);

  return GET_LEVEL(ch) * mult;
}

int get_innate_resistance(P_char ch)
{
  int      res, lvl = GET_LEVEL(ch);
  char     buf[128];

  if( !IS_ALIVE(ch) )
    return 0;

  res = racial_shrug_data[GET_RACE(ch)];
  res -= MIN(6, 56 - lvl);
  res = (int) (res * MIN(1., ((float) lvl) / 50));
  res = MAX(5, res);

  if( has_innate(ch, INNATE_RRAKKMA) && ch->group )
  {
  //  debug("shrug pct before rrakkma: %d", res);
    int count = 0;
    for(struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if( ch != gl->ch && 
          IS_PC(gl->ch) &&
          ch->in_room == gl->ch->in_room &&
          has_innate(gl->ch, INNATE_RRAKKMA) )
        count++;
    }
    
    if( count > 0 )
    {
      count = MIN(count, (int) get_property("innate.rrakkma.maxSteps", 5));
      res += count * (int) get_property("innate.rrakkma.stepPc", 5);
    } 
  //  debug("after: %d", res);
  }
//  debug("resistance: %d", res);
  return (res >= 100) ? 100 : res;
}

bool resists_spell(P_char caster, P_char victim)
{
  int skill;

  // Caster missing? Really bad.  Dead?  Hrmm.. that could be bad!  But it could be a death proc.
  if( !caster )
  {
    logit(LOG_DEBUG, "resists_spell()bogus parms missing ch!! (victim %s'%s').",
      IS_ALIVE(victim) ? "" : "Dead ", victim ? J_NAME(victim) : "Null" );
    raise(SIGSEGV);
  }

  // Dead/missing victims don't shrug, although we log a message just in case.
  if( !IS_ALIVE(victim) )
  {
    logit(LOG_DEBUG, "resists_spell()bogus parms ch %s'%s', victim %s'%s'.",
      IS_ALIVE(caster) ? "" : "Dead ", J_NAME(caster),
      IS_ALIVE(victim) ? "" : "Dead ", victim ? J_NAME(victim) : "Null" );
    debug("resists_spell()bogus parms ch %s'%s', victim %s'%s'.",
      IS_ALIVE(caster) ? "" : "Dead ", J_NAME(caster),
      IS_ALIVE(victim) ? "" : "Dead ", victim ? J_NAME(victim) : "Null" );
    return FALSE;
  }

  if( caster == victim || is_linked_to(caster, victim, LNK_CONSENT) )
  {
    return FALSE;
  }

  if( IS_TRUSTED(caster) && !IS_TRUSTED(victim) )
  {
    return FALSE;
  }

  if( affected_by_spell(victim, SKILL_SPELL_PENETRATION) )
  {
    return FALSE;
  }

  skill = GET_CHAR_SKILL(caster, SKILL_SPELL_PENETRATION)/2;
  if( has_innate(victim, INNATE_MAGIC_RESISTANCE) )
  {
    int shrugroll = number(1, 101);
    //debug("shrug check roll: %d", shrugroll);
    if(shrugroll > get_innate_resistance(victim))
    {
      return FALSE;
    }
    if( get_spell_from_room(&world[victim->in_room], SPELL_DESECRATE_LAND) )
    {
      // 10% penalty to shrug if desecrate land is active.
      if( shrugroll > get_innate_resistance(victim) - 10 )
      {
        return FALSE;
      }
    }
    if( GET_RACE(victim) == RACE_BEHOLDER )
    {
      act("&+W$n&+W's central eye glows brightly as it negates your spell!",
          TRUE, victim, 0, caster, TO_VICT);
      act("&+WYour central eye glows brightly as it negates $N&+W's spell!",
          TRUE, victim, 0, caster, TO_CHAR);
      act("&+W$n&+W's central eye glows brightly as it negates $N&+W's spell!",
         TRUE, victim, 0, caster, TO_NOTVICT);
      return TRUE;
    }
    if( skill || (IS_NPC(caster) && (IS_ELITE(caster) || IS_GREATER_RACE(caster))) )
    {

      if( IS_NPC(caster) && !IS_PC_PET(caster) )
      {
        skill = GET_LEVEL(caster); // Added Nov08 -Lucrot
      }

      if( number(0, 110) < BOUNDED(10, skill, (int)get_property("skill.spellPenetration.highEndPercent", 60.000))
        && caster->in_room == victim->in_room )
      {
        struct affected_type af;

        memset(&af, 0, sizeof(af));
        af.type = SKILL_SPELL_PENETRATION;
        af.flags = AFFTYPE_NOAPPLY | AFFTYPE_SHORT;
        af.duration = 0;
        affect_to_char(victim, &af);

        act("&+CYour pure arcane focus causes your spell to burst through&n $N&+C's magical resistance!&n",
          TRUE, caster, 0, victim, TO_CHAR);
        act("$n&+C seems to focus for a moment, and $s spell bursts through your magical barrier!&n",
          TRUE, caster, 0, victim, TO_VICT);
        act("$n&+C seems to focus for a moment, and $s spell bursts through&n $N&+C's magical barrier!&n",
          TRUE, caster, 0, victim, TO_NOTVICT);
        return FALSE;
      }
    }

    act("&+MYour spell flows around&n $N&+M, leaving $M unharmed!",
        TRUE, caster, 0, victim, TO_CHAR);
    act("&+MYou resist the effects of&n $n&+M's spell!",
      TRUE, caster, 0, victim, TO_VICT);
    act("$n's &+Mspell flows around&n $N&+M, leaving $M unharmed!",
        TRUE, caster, 0, victim, TO_NOTVICT);

    return TRUE;
  }
  return FALSE;
}

void engulf(P_char ch, P_char victim)
{
}

void do_engulf(P_char ch, char *argument, int cmd)
{
}

void slime(P_char ch, P_char victim)
{
}

void do_slime(P_char ch, char *argument, int cmd)
{
}

void do_branch(P_char ch, char *argument, int cmd)
{
  P_char   victim = NULL;

  victim = parse_victim(ch, argument, 0);
  
  if (victim)
    branch(ch, victim);
  else
    CharWait(ch, 1);
}

void branch(P_char ch, P_char victim)
{
  int chance, random_factor;
  struct damage_messages messages = {
    "You lift $N lightly and &+rsmash&n $M against the &+yground!&n",
    "$n lifts you lightly and &+rsmashes&n you against the &+yground!&n",
    "$n lifts $N lightly and &+rsmashes&n $M against the &+yground!&n",
    "You lift $N lightly and &+rsmash&n $M against the &+yground!&n",
    "$n lifts you lightly and &+rsmashes&n you against the &+yground!&n",
    "$n lifts $N lightly and &+rsmashes&n $M against the &+yground!&n",
  };
  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || ch->in_room != victim->in_room)
    return;

  victim = guard_check(ch, victim);

  act("One of your branches reaches towards $N.", FALSE, ch, 0, victim, TO_CHAR);
  act("One of $n's branches reaches towards $N.", FALSE, ch, 0, victim, TO_NOTVICT);
  act("One of $n's branches reaches towards you.", FALSE, ch, 0, victim, TO_VICT);
  if( get_takedown_size(victim) > get_takedown_size(ch) )
  {
    act("You fail to pick $N up.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n fails to pick $N up.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n failed to pick you up.", FALSE, ch, 0, victim, TO_VICT);
    return;
  }

  random_factor = number(-15, 15);

  chance = (100 + random_factor) * GET_LEVEL(ch) / GET_LEVEL(victim);

  chance *= GET_C_STR(ch);
  chance /= GET_C_CON(victim);
  chance *= MAX(GET_C_DEX(ch), 40);
  chance /= GET_C_AGI(victim);

  chance += 10*(get_takedown_size(ch)-get_takedown_size(victim));

  chance = (int) (chance * get_property("innate.branchDumper", 0.90));

  chance = BOUNDED(40, chance, 95);

  if( (GET_POS( victim ) < POS_STANDING) || (chance < number( 1, 100 )) )
  {
    act("$N narrowly avoids your grasp.", FALSE, ch, 0, victim, TO_CHAR);
    act("$N narrowly avoids $n's grasp.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You narrowly avoid $n's grasp.", FALSE, ch, 0, victim, TO_VICT);
  }
  else
  {
    CharWait(victim, 2 * PULSE_VIOLENCE);
    melee_damage(ch, victim, 2 * GET_LEVEL(ch), PHSDAM_NOENGAGE | PHSDAM_NOSHIELDS, &messages);
    SET_POS(victim, POS_PRONE + GET_STAT(victim));
  }
  if( IS_NPC(ch) && IS_PC_PET(ch) )
    CharWait(ch, 4 * PULSE_VIOLENCE);
  else
    CharWait(ch, 2 * PULSE_VIOLENCE);
}

void webwrap(P_char ch, P_char victim)
{
  if( !ch || !victim )
    return;

  struct affected_type af;
  int chance;

  if( get_takedown_size(ch) < get_takedown_size(victim) - 1 )
  {
  	send_to_char("You are too small to immobilize your victim!\n", ch);
    return;
  }

  chance = 30 + 2 * (GET_LEVEL(ch) - GET_LEVEL(victim));
  chance = (chance * 100 / GET_C_AGI(victim));

  if (chance < number(0, 100))
  {
    send_to_char("You failed to immobilize your victim!\n", ch);
    return;
  }

  act("$n grabs $N and ignoring $S struggles wraps $M tightly in a sticky web.", FALSE, ch, 0, victim, TO_NOTVICT);
  act("You grab $N and ignoring $S struggles wrap $M tightly in a sticky web.", FALSE, ch, 0, victim, TO_CHAR);
  act("$n grabs you and ignoring your struggles wraps you tightly in a sticky web.", FALSE, ch, 0, victim, TO_VICT);

  memset(&af, 0, sizeof(af));
  af.type = SPELL_MINOR_PARALYSIS;
  af.bitvector2 = AFF2_MINOR_PARALYSIS;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
  af.duration = number(5,10) * PULSE_VIOLENCE;

  affect_to_char(victim, &af);

  CharWait(ch, PULSE_VIOLENCE);
  stop_fighting(victim);
  if( IS_DESTROYING(victim) )
    stop_destroying(victim);
}

void do_webwrap(P_char ch, char *argument, int cmd)
{
  P_char   victim = NULL;

  victim = parse_victim(ch, argument, 0);
  webwrap(ch, victim);
}

#define DEVIL_IMP 63

void do_summon_imp(P_char ch, char *argument, int cmd)
{
  P_char imp, foe;
  struct char_link_data *cld;

  if( !IS_ALIVE(ch) )
    return;

  if(IS_NPC(ch))
  {
    if(GET_VNUM(ch) == 63)
    {
      send_to_char("Imps cannot summon more imps!", ch);
      return;
    }
  }

  for (cld = ch->linked; cld; cld = cld->next_linked)
  {
    if (cld->type == LNK_PET &&
        IS_NPC(cld->linking) &&
        mob_index[GET_RNUM(cld->linking)].virtual_number == DEVIL_IMP)
    {
      return;
    }
  }

  imp = read_mobile(DEVIL_IMP, VIRTUAL);

  if(!imp)
  {
    return;
  }
  
  if(world[ch->in_room].sector_type == SECT_PLANE_OF_AVERNUS | SECT_FIREPLANE)
  {
    imp->player.level = (int) (GET_LEVEL(ch) * 0.9);
  }
  else
  {
    imp->player.level = (int) (GET_LEVEL(ch) * 0.6);
  }

  act("$n &+Rdraws a &+Wglowing portal &+Rwith $s claw in the air.", FALSE,
    ch, 0, 0, TO_ROOM);
  act("&+RMoments later &n$N &+Rpops out and charges at $n's foes!&n", FALSE,
    ch, 0, imp, TO_ROOM);
      
  imp->points.base_hitroll += 25;
  imp->points.base_damroll += 25;
  
  char_to_room(imp, ch->in_room, 0);

  setup_pet(imp, ch, -1, 0);
  
  if(foe = GET_OPPONENT(ch))
  {
    MobStartFight(imp, foe);
  }
}

#undef DEVIL_IMP

void event_lifedrain(P_char ch, P_char victim, P_obj obj, void *data)
{
  int gain = *((int*)data);

  if (ch->in_room != victim->in_room || gain <= 0) {
    act("&+LA dark beam of draining energy between you and&n $N &+Lfades.&n",
        FALSE, ch, 0, victim, TO_CHAR);
    act("&+LYou feel better as a dark beam from&n $n &+Lfades.&n",
        FALSE, ch, 0, victim, TO_VICT);
    return;
  }

  if (GET_HIT(ch) >= 1.1 * GET_MAX_HIT(ch)) {
    send_to_char("You cannot drain any more life forces!\n", ch);
    act("&+LYou feel better as a dark beam from $n fades.&n",
        FALSE, ch, 0, victim, TO_VICT);
    return;
  }

  if (GET_HIT(victim) <= 6) {
    send_to_char("There is not enough life left in your victim.\n", ch);
    return;
  }

  GET_HIT(victim) -= 5;
  if (victim->desc)
    victim->desc->prompt_mode = TRUE;
  GET_HIT(ch) += 5;
  if (ch->desc)
    ch->desc->prompt_mode = TRUE;
  gain -= 5;

  add_event(event_lifedrain, WAIT_SEC, ch, victim, 0, 0, &gain, sizeof(gain));
  attack_back(ch, victim, FALSE);
}

void do_lifedrain(P_char ch, char *argument, int cmd)
{
  P_char   victim = NULL;
  int gain = GET_LEVEL(ch) * 2;

  if (get_scheduled(ch, event_lifedrain)) {
    send_to_char("You are draining someone's life already!\n", ch);
    return;
  }

  victim = parse_victim(ch, argument, 0);
  if (!victim) {
    send_to_char("Drain who?\n", ch);
    return;
  }

  if (!check_innate_time(ch, INNATE_LIFEDRAIN)) {
    send_to_char("You are still too tired to drain another victim.", ch);
    return;
  }

  act("&+LYou send a dark beam of draining energy towards&n $N &+Land begin harvesting $S lifeforce.&n",
      FALSE, ch, 0, victim, TO_CHAR);
  act("&+LA dark beam shoots from&n $n &+Ltowards you and begins draining your lifeforce!&n",
      FALSE, ch, 0, victim, TO_VICT);
  act("&+LA dark beam of energy shoots from&n $n &+Ltowards&n $N.&n", FALSE, ch, 0, victim, TO_NOTVICT);
  add_event(event_lifedrain, WAIT_SEC, ch, victim, 0, 0, &gain, sizeof(gain));
}

void do_immolate(P_char ch, char *argument, int cmd)
{
  P_char   victim = NULL;
  struct affected_type *afp;

  if( !IS_ALIVE(ch) )
  {
    return;
  }
  
  victim = parse_victim(ch, argument, 0);
  
  if(!IS_AFFECTED2(ch, AFF2_FIRESHIELD))
  {
    send_to_char("You are not fiery enough!\n", ch);
    return;
  }

  if(!check_innate_time(ch, INNATE_IMMOLATE))
  {
    send_to_char("You need to rest some to regain your inner fire.\n", ch);
    return;
  }

  act("You raise your hands and let &+Rflames engulfing you &+Yexplode&n cleansing your foes!&n",
      FALSE, ch, 0, victim, TO_CHAR);
  act("As $n raises $s hands &+Ra fiery inferno &nof &+Ydancing flames&n flows from $m.",
      FALSE, ch, 0, victim, TO_ROOM);
  
  if(afp = get_spell_from_char(ch, SPELL_FIRESHIELD))
  {
    wear_off_message(ch, afp);
    affect_remove(ch, afp);
  }

  cast_as_damage_area(ch, spell_single_incendiary_cloud, GET_LEVEL(ch), victim,
                      get_property("spell.area.minChance.incendiaryCloud",
                                   50),
                      get_property("spell.area.chanceStep.incendiaryCloud",
                                   20));
  
  CharWait(ch, PULSE_VIOLENCE * 4);
  
  return;
}

void event_halfling_check(P_char ch, P_char victim, P_obj obj, void *data)
{
  if( !ch->equipment[WEAR_FEET] && !affected_by_spell(ch, TAG_BAREFEET) )
  {
    struct affected_type af;
    memset(&af, 0, sizeof(af));
    af.type = TAG_BAREFEET;
    af.location = APPLY_MOVE_REG;
    af.modifier = 6;
    af.duration = 10;
    affect_to_char(ch, &af);
    send_to_char("&+yYou stretch your toes feeling much more comfortable "
        "with unconstrained feet.&n\n", ch);
  }
  else if (ch->equipment[WEAR_FEET] &&
          affected_by_spell(ch, TAG_BAREFEET))
  {
    affect_from_char(ch, TAG_BAREFEET);
  }

  add_event(event_halfling_check, 3 * WAIT_SEC, ch, 0, 0, 0, 0, 0);
}

void event_hatred_check(P_char ch, P_char victim, P_obj obj, void *data)
{
  //int CH_RACE = GET_RACE(ch);
  //LOOP_ROOM....
  //for each ch in room diffrent then CH
  //loop your table and figure out if hatred should be updated.
  struct affected_type af, *afp, *next_af;
  P_char tch = NULL;
  int    x=0, i=0, z=1;
  int   found=FALSE;

  if (!has_innate(ch, INNATE_HATRED))
    return;

  for (i = 0,x = race_hatred_data[i][0];x == GET_RACE(ch) || x == -1 || found;i++, x = race_hatred_data[i][0] )
  {
    if (x == -1 || found)
      break;

    LOOP_THRU_PEOPLE(tch, ch)
    {
      if (tch == ch)
        continue;
      if (!CAN_SEE(ch, tch))
        continue;

      for (z = 1;z < MAX_HATRED +1 && !found; z++)
      {
        if (race_hatred_data[i][z] == GET_RACE(tch))
        {
          found = TRUE;
          for (afp = ch->affected; afp; afp = next_af)
          {
            next_af = afp->next;
            if (afp->type == TAG_HATRED)
              afp->duration = 1;
          }

          if (!get_spell_from_char(ch, TAG_HATRED))
          {
            memset(&af, 0, sizeof(af));
            af.type = TAG_HATRED;
            af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL;
            af.location = APPLY_COMBAT_PULSE;
            af.modifier = -1;
            af.duration = 1;
            affect_to_char(ch, &af);
            af.location = APPLY_SPELL_PULSE;
            af.modifier = -1;
            af.duration = 1;
            affect_to_char(ch, &af);
            act("At the sight of your nemesis the hatred of old generations &+rBURN&n in your blood!", FALSE, ch, 0, tch, TO_CHAR);
          }

        }
      }
    }
  }
  if (!found)
  {
    x=0;
    for (afp = ch->affected; afp; afp = next_af)
    {
      next_af = afp->next;

      if (afp->type == TAG_HATRED)
      {
        affect_from_char(ch, TAG_HATRED);
        x++;
        if (x>1)
          wear_off_message(ch, afp);
      }
    }
  }

  add_event(event_hatred_check,
      get_property("innate.timer.hatred", WAIT_SEC),
      ch, 0, 0, 0, 0, 0);
}

bool rapier_dirk_check(P_char ch) //now known as swashbuckling, single weapon.
{
  P_obj weap1, weap2;

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  weap1 = ch->equipment[PRIMARY_WEAPON];
  weap2 = ch->equipment[SECONDARY_WEAPON];

//  if(GET_CLASS(ch, CLASS_BARD) || GET_CLASS(ch, CLASS_MERCENARY) || GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF))
  if( has_innate(ch, INNATE_RAPIER_DIRK) && weap1 && IS_SWORD(weap1) && weap2 && IS_DIRK(weap2) )
  {
    return TRUE;
  }
  return FALSE;
}

bool innate_two_daggers(P_char ch)
{
  P_obj weap1, weap2;

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  weap1 = ch->equipment[PRIMARY_WEAPON];
  weap2 = ch->equipment[SECONDARY_WEAPON];

  if( has_innate(ch, INNATE_TWO_DAGGERS) && weap1 && IS_DAGGER(weap1) && weap2 && IS_DAGGER(weap2) )
    return TRUE;

  return FALSE;
}

struct fade_data {
  int room;
  int counter;
};

void event_fade(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct fade_data *fdata = (struct fade_data*)data;

  if( !ch->desc || IS_FIGHTING(ch) || (GET_STAT(ch) < STAT_SLEEPING)
    || IS_DESTROYING(ch) || IS_SET(ch->specials.affected_by, AFF_HIDE))
  {
    send_to_char
      ("You fade into existance confused and in the same place.\n", ch);
    return;
  }

  if (fdata->counter > 0) {
    fdata->counter--;
    add_event(event_fade, WAIT_SEC, ch, 0, 0, 0, fdata, sizeof(*fdata));
  } else {
    char_from_room(ch);
    char_to_room(ch, fdata->room, -1);

    act("&+L$n steps out of the darkness.", TRUE, ch, 0, 0, TO_ROOM);
    act("&+LYou step out of the darkness.", TRUE, ch, 0, 0, TO_CHAR);
  }
}

void do_fade(P_char ch, char *argument, int cmd)
{
  char     back[128];
  struct affected_type af, *afp;
  struct fade_data fdata;

  if (!has_innate(ch, INNATE_FADE))
  {
    send_to_char
      ("You click your shoes together 3 times, but nothing happens.\n", ch);
    return;
  }

  if (IS_ROOM(ch->in_room, ROOM_NO_RECALL) ||
      (world[ch->in_room].sector_type == SECT_OCEAN))
  {
    send_to_char("You cannot fade from this room.\n", ch);
    return;
  }

  if (get_scheduled(ch, event_fade))
  {
    send_to_char("You are already fading in and out.\n", ch);
    return;
  }

  one_argument(argument, back);

  if (strstr(back, "return") != NULL) {
    if (!(afp = get_spell_from_char(ch, SPELL_MOONSTONE)))
    {
      send_to_char("You do not have a location to fade back to.\n", ch);
      return;
    } else {
      fdata.room = afp->modifier;
      fdata.counter = get_property("innate.fade.secs.duration", 180);
      send_to_char
        ("You start to fade from existance and become dimmer and dimmer.\n",
         ch);
      act("$n starts to fade from existance and becomes dimmer and dimmer.",
          FALSE, ch, 0, ch, TO_ROOM);

      add_event(event_fade, WAIT_SEC, ch, 0, 0, 0, &fdata, sizeof(fdata));
      return;
    }
  }

  memset(&af, 0, sizeof(af));

  af.type = SPELL_MOONSTONE;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOSAVE;
  af.duration = -1;
  af.modifier = ch->in_room;
  affect_to_char(ch, &af);

  send_to_char
    ("You start to fade from existance and become dimmer and dimmer.\n",
     ch);
  act("$n starts to fade from existance and becomes dimmer and dimmer.",
      FALSE, ch, 0, ch, TO_ROOM);

  fdata.room = real_room(GET_BIRTHPLACE(ch));
  fdata.counter = get_property("innate.fade.secs.duration", 180);
  add_event(event_fade, WAIT_SEC, ch, 0, 0, 0, &fdata, sizeof(fdata));
}

void do_layhand(P_char ch, char *argument, int cmd)
{
  struct affected_type *afp;
  time_t   now;
  int      tmp_time, healamt;
  P_char   vict = NULL;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     name[MAX_INPUT_LENGTH];
  P_nevent ne;
  int time = -1;
  int timer = get_property("innate.timer.layHands", 16 * 60 * WAIT_SEC);
  int healpoints = ((GET_LEVEL(ch) / 10) * GET_C_CHA(ch) + number(1, 100));
  float rested;

  /* Okay, lets change layhands a bit to make it more charisma based, 
   * like D&D
   */

  if (GET_RACE(ch) ==  RACE_REVENANT)
    healpoints *= 2;

  if (IS_NPC(ch))
  {
    send_to_char("You can't lay hand!\r\n", ch);
    return;
  }

  if (!has_innate(ch, INNATE_LAY_HANDS))
  {
    send_to_char("You don't have the ability to lay hand.\r\n", ch);
    return;
  }

  if (!IS_GOOD(ch))
  {
    send_to_char("You don't possess enough moral fortitude to do that.\r\n", ch);
    return;
  }

  one_argument(argument, name);

  if( *name )
  {
    vict = get_char_room_vis(ch, name);
  }

  if( !vict )
  {
     send_to_char("&+WLay hand who?\n", ch);
     return;
  }

/*
  // calculate timer for informative messages :)
  LOOP_EVENTS_CH( ne, ch->nevents )
  {
    if (ne->func == event_short_affect) {
      struct event_short_affect_data *esad =
        (struct event_short_affect_data *)ne->data;
      if (esad->af->type == TAG_INNATE_TIMER &&
          esad->af->location == INNATE_LAY_HANDS)
          time = ne_event_time(ne);
    }
  }

  rested = 1.0 - ((float)time) / timer;

  // Lom: enabled thus very informative messages
  if (time != -1)
  {
     if (rested < 0.33) {
        send_to_char("&+YYou need a lot of rest before laying hand.\r\n", ch);
        return;
     } else if (rested < 0.66) {
        send_to_char("&+yYou need some more rest before laying hand.\r\n", ch);
        return;
     } else {
        send_to_char("&+WYou're almost ready to lay hands.\r\n", ch);
        return;
     }
  }*/

    if( !check_innate_time(ch, INNATE_LAY_HANDS) )
    {
      send_to_char("You need more rest before laying hands.\r\n", ch);
      return;
    }

  /* Lom: dont need as now fixed informative messages
    if (time != -1)
    {
      send_to_char("You need more rest before laying hand.\r\n", ch);
      return;
    }
  */

  /*
  if (GET_OPPONENT(ch) && (vict != ch))
  {
    send_to_char("You can't lay hands on others while fighting.\r\n", ch);
    return;
  }
  */

  //  act("$n's hands glow as $e lays $s hand on $N.", FALSE, ch, 0, vict, TO_NOTVICT);
  if( ch != vict )
  {
    act("Your hands &+Wglow&n as you lay your hands on $N.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n's hands &+Wglow&n as $e lays $s hands on you.", FALSE, ch, 0, vict, TO_VICT);
    act("$n's hands &+Wglow&n as $e lays $s hands on $N.", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    act("Your hands &+Wglow&n as you lay your hands on yourself.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n's hands &+Wglow&n as $e lays $s hands on $mself.", FALSE, ch, 0, vict, TO_NOTVICT);
  }

/* lom: probably too good, and dont need it
  if (IS_UNDEAD(vict) || IS_AFFECTED4(vict, AFF4_REV_POLARITY))
  {
    act("&+L$n &+RSCREAMS&+L in pain as the &+Wgoodness&+L touches $m!",
        FALSE, vict, 0, 0, TO_ROOM);
    act("&+LYou &+RSCREAM&+L in pain as the &+Wgoodness&+L touches you!",
        FALSE, ch, 0, vict, TO_VICT);
    damage(ch, vict, (healpoints * 2), SPLDAM_HOLY);
    return;
  }
*/
//  send_to_char("&+WYou feel better!&n\r\n", vict);

  heal(vict, ch, healpoints, GET_MAX_HIT(vict));

  healCondition(vict, healamt);

  update_pos(vict);


  if (IS_AFFECTED4(vict, AFF4_CARRY_PLAGUE))
    REMOVE_BIT(vict->specials.affected_by4, AFF4_CARRY_PLAGUE);
/*
  if (affected_by_spell(vict, SPELL_DISEASE) ||
      affected_by_spell(vict, SPELL_PLAGUE))
  {
    affect_from_char(vict, SPELL_DISEASE);
    affect_from_char(vict, SPELL_PLAGUE);
  }
*/
  if (IS_AFFECTED(vict, AFF_BLIND))
  {
     spell_cure_blind(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, NULL);
  }
  
  if (affected_by_spell(vict, SPELL_POISON) || IS_AFFECTED2(vict, AFF2_POISONED))
  {
     spell_remove_poison(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, NULL);
  }
  
  if (affected_by_spell(vict, SPELL_DISEASE) || affected_by_spell(vict, SPELL_PLAGUE))
  {
     spell_cure_disease (GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, NULL);
  }
}

void holy_crusade_check(P_char ch, P_char victim)
{
  if (!has_innate(ch, INNATE_HOLY_CRUSADE) || !IS_EVIL(victim))
    return;

  if (vamp(ch, 2 * GET_LEVEL(victim), 1.1 * GET_MAX_HIT(ch))) {
    act("As you execute the killing blow on the evil being, you feel the \n"
        "hand of your God reach down and imbue with heavenly might!",
        FALSE, ch, 0, 0, TO_CHAR);
    act("A sense of peace fills the room as $n eliminates the evil force!", FALSE, ch, 0, 0, TO_ROOM);
  }
}

void do_aura_protection(P_char ch, char *arg, int cmd) {
        do_aura(ch, AURA_PROTECTION);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_aura_spell_protection(P_char ch, char *arg, int cmd) {
        do_aura(ch, AURA_SPELL_PROTECTION);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_aura_precision(P_char ch, char *arg, int cmd) {
        do_aura(ch, AURA_PRECISION);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_aura_battlelust(P_char ch, char *arg, int cmd) {
        do_aura(ch, AURA_BATTLELUST);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_aura_healing(P_char ch, char *arg, int cmd) {
        do_aura(ch, AURA_HEALING);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_aura_endurance(P_char ch, char *arg, int cmd) {
        do_aura(ch, AURA_ENDURANCE);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_aura_vigor(P_char ch, char *arg, int cmd) {
        do_aura(ch, AURA_VIGOR);
  CharWait(ch, PULSE_VIOLENCE);
}

void do_divine_force(P_char ch, char *arg, int cmd)
{
  struct affected_type af;
  struct affected_type af1;
  int timer, timer1;
  
  if(!ch ||
     !IS_ALIVE(ch))
  {
    return;
  }
  
  if(!has_innate(ch, INNATE_DIVINE_FORCE))
  {
    send_to_char("You pray, and pray, and pray ...\n", ch);
    return;
  }

  if(affected_by_spell(ch, TAG_DIVINE_FORCE_AFFECT))
  {
    send_to_char("You are alread empowered by divine force.\n", ch);
    return;
  }

  if(affected_by_spell(ch, TAG_DIVINE_FORCE_TIMER))
  {
    send_to_char("Divinity is a precious gift. You must consider waiting a bit longer.\n", ch);
    return;
  }
    
  timer1 = get_property("innate.timer.secs.divineforceAffect", 20);
  
  memset(&af1, 0, sizeof(af1));
  af1.type = TAG_DIVINE_FORCE_AFFECT;
  af1.location = INNATE_DIVINE_FORCE;
  af1.duration = timer1;
  af1.flags = AFFTYPE_SHORT | AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
  affect_to_char(ch, &af1);
  
  timer = get_property("innate.timer.secs.divineforceTimer", 600);
  
  memset(&af, 0, sizeof(af));
  af.type = TAG_DIVINE_FORCE_TIMER;
  af.location = INNATE_DIVINE_FORCE;
  af.duration = timer;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY; 
  affect_to_char(ch, &af);
  
  act("&+YYou summon a surge of &+Cdivine &+Yforce!", FALSE, ch, 0, NULL, TO_CHAR); 
  act("$n &+Ysummons a surge of &+Cdivine &+Yforce to assist $m!", FALSE, ch, 0, NULL, TO_ROOM); 
  
}

bool has_divine_force(P_char ch)
{
  struct affected_type *afp;

  if(affected_by_spell(ch, TAG_DIVINE_FORCE_AFFECT))
  {
    return true;
  }
  else
  {
    return false;
  }
}

struct innate_item
{
  char *name;
  int   num;
};

int innate_cmp(const void *va, const void *vb)
{
  return (str_cmp( ((innate_item *)va)->name, ((innate_item *)vb)->name ));
}

// Shows all innates to ch and who can use them.
void do_list_innates( P_char ch, char *args )
{
  int h, i, j, k;
  char Gbuf1[MAX_STRING_LENGTH];
  bool racefound, classfound, fulllist, playeronly;

  struct innate_item innate_order[LAST_INNATE+1];

  // Options: 'all', 'player', 'partial'.
  fulllist = is_abbrev( args, "all" ) ? TRUE : FALSE;
  playeronly = is_abbrev( args, "player" ) ? TRUE : FALSE;

  send_to_char( "&+WListing of Innates:&n\n", ch );

  for( h = 0; h <= LAST_INNATE; h++ )
  {
    innate_order[h].name = innates_data[h].name;
    innate_order[h].num = h;
  }
  qsort( innate_order, LAST_INNATE+1, sizeof(struct innate_item), innate_cmp );

  for( h = 0; h <= LAST_INNATE; h++ )
  {
    i = innate_order[h].num;
    // Enter list of races (skip RACE_NONE).
    racefound = FALSE;
    for( j = 1; j <= (playeronly ? RACE_PLAYER_MAX : LAST_RACE); j++ )
    {
      // Player races that aren't in game currently.  This will need updating somehow..
      if( playeronly && (j == RACE_ILLITHID || j == RACE_SHADE
        || j == RACE_LICH || j == RACE_PVAMPIRE || j == RACE_PDKNIGHT
        || j == RACE_PSBEAST || j == RACE_SGIANT || j == RACE_WIGHT
        || j == RACE_PHANTOM || j == RACE_HARPY || j == RACE_OROG
        || j == RACE_PILLITHID || j == RACE_KUOTOA || j == RACE_WOODELF) )
      {
        continue;
      }
      // If the race has the innate.
      if( racial_innates[i][j] )
      {
        if( !racefound )
        {
          snprintf(Gbuf1, MAX_STRING_LENGTH, "&+W%s:\n", innates_data[i].name );
          send_to_char( Gbuf1, ch );
          send_to_char( "&+B Races:&n", ch );
        }
        snprintf(Gbuf1, MAX_STRING_LENGTH, "%s%s", racefound ? ", " : " ", race_names_table[j].ansi );
        racefound = TRUE;
        send_to_char( Gbuf1, ch );
      }
    }
    if( racefound )
    {
      send_to_char( "\n", ch );
    }
    // Enter list of classes. class_innates_at_all might include !player classes only.
    if( class_innates_at_all[i] )
    {
      classfound = FALSE;
      for( j = 1; j <= CLASS_COUNT; j++ )
      {
        // Not sure how to automate this either.. non player classes omitted.
        if( playeronly && (j == flag2idx(CLASS_ASSASSIN) || j == flag2idx(CLASS_THIEF)
          || j == flag2idx(CLASS_MINDFLAYER) || j == flag2idx(CLASS_ALCHEMIST) || j == flag2idx(CLASS_DREADLORD)
          || j == flag2idx(CLASS_AVENGER) || j == flag2idx(CLASS_THEURGIST) || j == flag2idx(CLASS_WARLOCK)) )
        {
          continue;
        }
//debug( "j: %d, flag2idx: %d, class_names_table[j]: '%s'.", j, flag2idx(CLASS_DREADLORD), class_names_table[j].ansi );
        // If the class has the innate.
        if( class_innates[i][j-1][0] )
        {
          if( !racefound && !classfound )
          {
            snprintf(Gbuf1, MAX_STRING_LENGTH, "&+W%s:\n", innates_data[i].name );
            send_to_char( Gbuf1, ch );
          }
          if( !classfound )
          {
            send_to_char( "&+B Classes:&n", ch );
          }
          snprintf(Gbuf1, MAX_STRING_LENGTH, "%s%s", classfound ? ", " : " ", class_names_table[j].ansi );
          classfound = TRUE;
          send_to_char( Gbuf1, ch );
        }
        else
        {
          for( k = 1; k <= MAX_SPEC; k++ )
          {
            if( class_innates[i][j-1][k] )
            {
              if( !racefound && !classfound )
              {
                snprintf(Gbuf1, MAX_STRING_LENGTH, "&+W%s:\n", innates_data[i].name );
                send_to_char( Gbuf1, ch );
              }
              if( !classfound )
              {
                send_to_char( "&+B Classes:&n", ch );
              }
              snprintf(Gbuf1, MAX_STRING_LENGTH, "%s%s", classfound ? ", " : " ", specdata[j][k-1] );
              classfound = TRUE;
              send_to_char( Gbuf1, ch );
            }
          }
        }
      }
      if( classfound )
      {
        send_to_char( "\n", ch );
      }
    }
    // If !classes and !races and we want to see all options..
    else if( !racefound && fulllist )
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "&+W%s:\n", innates_data[i].name );
      send_to_char( Gbuf1, ch );
    }
  }
}

void do_squidrage(P_char ch, char *arg, int cmd)
{
  int level;
  struct affected_type af;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( (GET_RACE(ch) != RACE_ILLITHID) || IS_CASTER(ch) || GET_CLASS(ch, CLASS_MINDFLAYER | CLASS_PSIONICIST) )
  {
    send_to_char( "Pardon?\n", ch );
    return;
  }

  if( level < 41 )
  {
    send_to_char( "Nothing happens.\n", ch );
    return;
  }

  if( affected_by_spell( ch, TAG_SQUIDRAGE ) )
  {
    send_to_char( "You're still too tired to try this again.\n", ch );
    return;
  }

  send_to_char( "You &+rr&+Rag&+re&n against the &+MElder Brain&n!\n", ch );
  memset(&af, 0, sizeof(af));
  af.type = TAG_SQUIDRAGE;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
  af.duration = 24;

  affect_to_char(ch, &af);

  level = GET_LEVEL(ch);
  // 1st
  spell_adrenaline_control(level, ch, "", 0, ch, NULL);
  spell_combat_mind(level, ch, "", 0, ch, NULL);
  if( level < 42 )
  {
    return;
  }
  // 2nd
  spell_enhanced_agility(level, ch, "", 0, ch, NULL);
  spell_enhanced_constitution(level, ch, "", 0, ch, NULL);
  spell_enhanced_dexterity(level, ch, "", 0, ch, NULL);
  spell_enhanced_strength(level, ch, "", 0, ch, NULL);
  if( level < 43 )
  {
    return;
  }
  // 3rd
  spell_aura_sight(level, ch, "", 0, ch, NULL);
  if( level < 44 )
  {
    return;
  }
  // 4th
  spell_energy_containment(level, ch, "", 0, ch, NULL);
  if( level < 45 )
  {
    return;
  }
  // 5th
  spell_flesh_armor(level, ch, "", 0, ch, NULL);
  spell_fly(level, ch, "", 0, ch, NULL);
  spell_intellect_fortress(level, ch, "", 0, ch, NULL);
  if( level < 46 )
  {
    return;
  }
  // 6th
  spell_enhance_armor(level, ch, "", 0, ch, NULL);
  if( level < 48 )
  {
    return;
  }
  // 8th
  spell_displacement(level, ch, "", 0, ch, NULL);
}

void update_regen_properties()
{
  regen_factor[REG_TROLL] = get_property("hit.regen.Troll", 8.000);
  regen_factor[REG_REVENANT] = get_property("hit.regen.Revenant", 4.000);
  regen_factor[REG_HUNTSMAN] = get_property("hit.regen.Huntsman", 4.000);
  regen_factor[REG_WATERMAGUS] = get_property("hit.regen.WaterMagus", 4.000);
}

int get_innate_from_skill( int skill )
{
  for( int innate = 0; innate <= LAST_INNATE; innate++ )
  {
    if( innates_data[innate].skill == skill )
    {
      return innate;
    }
  }
  return -1;
}

int get_level_from_innate( P_char ch, int innate )
{
  int race;
  int level;
  bool specd = IS_SPECIALIZED(ch);
  struct affected_type *af, *af2;

  // Start above maximum lvl.
  level = MAXLVL + 1;

  if( class_innates_at_all[innate] & ch->player.m_class )
  {
    for( int i = 0; i < CLASS_COUNT; i++ )
    {
      if( GET_CLASS(ch, 1 << i) )
      {
        // If there's a spec level, then it takes precedence.
        if( class_innates[innate][i][ch->player.spec] )
        {
          level = MIN( level, class_innates[innate][i][ch->player.spec] );
        }
        // Otherwise, if the spec didn't lose the innate and the class has the innate
        else if( !specd && class_innates[innate][i][0] )
        {
          level = MIN( level, class_innates[innate][i][0] );
        }
      }
    }
  }

  race = ch->player.race;
  // If they're corpseformed without the innates change.
  if( (( af = get_spell_from_char(ch, TAG_RACE_CHANGE) ) != NULL)
    && (( af2 = get_spell_from_char(ch, SPELL_CORPSEFORM) ) != NULL) )
  {
    if( af2->modifier == CORPSEFORM_REG )
    {
      // Use their regular race's innates.
      race = af->modifier;
    }
  }

  if( racial_innates[innate][race] )
  {
    return level = MIN( level, racial_innates[innate][race] );
  }

  if( level <= MAXLVL )
    return level;

  return 0;
}
