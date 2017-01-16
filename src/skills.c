/*
***************************************************************************
*  File: skills.c                                          Part of Duris *
*  Usage: core routines for handling spellcasting                         *
*  Copyright  1990, 1991 - see 'license.doc' for complete information.    *
*  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
***************************************************************************
*/
#include <string.h>
#include "defines.h"
#include "spells.h"
#ifndef _DE_
#include "structs.h"
#include "utils.h"
#include "paladins.h"
#include "reavers.h"
#include "rogues.h"
#include "avengers.h"
#include "blispells.h"
#include "epic.h"
#include "necromancy.h"

int      numSkills;
void update_racial_skills(P_char ch);
#else
#include "misc/misc.h"
int currentSkill = 0;
#endif


#if defined(_DE_) || defined(_PFILE_)
extern void initialize_skills();

int flag2idx(int);
#else
#include "prototypes.h"
extern int SortedSkills[];
#endif

Skill skills[MAX_AFFECT_TYPES+1];
//void initialize_skills_new();

#if defined(_DE_) || defined(_PFILE_)
#   define SPELL_CREATE_MSG(Name, Index, Beats, Targets, Spell_pointer, Wear_off) \
  skills[Index].name = (Name); \
  currentSkill = Index;

#   define SPELL_CREATE(Name, Index, Beats, Targets, Spell_pointer) \
  skills[Index].name = (Name); \
  currentSkill = Index;

#   define SKILL_CREATE(Name, Index, Type) \
  skills[Index].name = (Name); \
  currentSkill = Index;

#   define SKILL_CREATE_WITH_MESSAGES(Name, Index, Type, wearoff_char, wearoff_room) \
  SKILL_CREATE(Name, Index, Type)

#   define SPELL_CREATE2(Name, Index, Beats, Targets, Spell_pointer, Wear_off, wear_off_room) \
	SPELL_CREATE_MSG(Name, Index, Beats, Targets, Spell_pointer, Wear_off)

#   define SPEC_SPELL_ADD(Class, Level, Spec) \
  skills[currentSkill].minLevel[flag2idx(Class)-1] = (Level)
#   define SPELL_ADD(Class, Level) \
  skills[currentSkill].minLevel[flag2idx(Class)-1] = (Level)

#   define TAG_CREATE(Name, Index) \
  skills[Index].name = (Name)
#   define TAG_CREATE_WITH_MESSAGES(Name, Index, wear_off, wear_off_room) \
  skills[Index].name = (Name)
#   define POISON_CREATE(Name, Index, Spell_pointer) \
  skills[Index].name = (Name)
#if defined (_PFILE_)
#   define SPEC_SKILL_ADD(Class, Level,  MaxLearn,  Spec) \
    skills[numSkills].m_class[flag2idx(Class)-1].rlevel[Spec] = Level;\
    skills[numSkills].m_class[flag2idx(Class)-1].maxlearn[Spec] = MaxLearn;

#   define SKILL_ADD(Class, Level, MaxLearn) \
		for( int i = 0; i < MAX_SPEC+1; i++ ) { \
    skills[numSkills].m_class[flag2idx(Class)-1].rlevel[i] = Level;\
    skills[numSkills].m_class[flag2idx(Class)-1].maxlearn[i] = MaxLearn; }
#else
#   define SPEC_SKILL_ADD(Class, Level,  MaxLearn,  Spec) \
    skills[currentSkill].minLevel[flag2idx(Class)-1] = (Level);
#   define SKILL_ADD(Class, Level, MaxLearn) \
    skills[currentSkill].minLevel[flag2idx(Class)-1] = (Level);
#endif

#else
void SPELL_CREATE_MSG(const char *Name, int Index, int Beats,
                  unsigned int Targets,
                  void (*Spell_pointer) (int, P_char, char *, int, P_char,
                                         P_obj), const char *Wear_off)
{
  skills[Index].name = (Name);
  skills[Index].wear_off_char[0] = (Wear_off);
  skills[Index].spell_pointer = Spell_pointer;
  skills[Index].beats = (sh_int) Beats;
  skills[Index].targets = Targets | TAR_SPELL;
  SortedSkills[Index] = Index;
  numSkills = Index;
}

void SPELL_CREATE(const char *Name, int Index, int Beats,
                  unsigned int Targets,
                  void (*Spell_pointer) (int, P_char, char *, int, P_char,
                                         P_obj))
{
  SPELL_CREATE_MSG(Name, Index, Beats, Targets, Spell_pointer, NULL);
}

void SKILL_CREATE(const char *Name, int Index, unsigned int Type)
{
    skills[Index].name = (Name);
    SortedSkills[Index] = Index;
    skills[Index].targets = Type | TAR_SKILL;
    numSkills = Index;
}

#   define TAG_CREATE(Name, Index) SKILL_CREATE(Name, Index, 0)
#   define TAG_CREATE_WITH_MESSAGES(Name, Index, wear_off, wear_off_room)\
   SKILL_CREATE_WITH_MESSAGES(Name, Index, 0, wear_off, wear_off_room)
#   define POISON_CREATE(Name, Index, Spell_pointer)\
   SPELL_CREATE(Name, Index, 0, \
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_POISON,\
     (spell_func)Spell_pointer)

void SKILL_CREATE_WITH_MESSAGES(const char *Name, int Index, unsigned int Type,
                                  const char *wearoff_char,
                                  const char *wearoff_room)
{
  SKILL_CREATE(Name, Index, Type);
  skills[Index].wear_off_char[0] = wearoff_char;
  skills[Index].wear_off_room[0] = wearoff_room;
}

void SPEC_SKILL_ADD(int Class, int Level, int MaxLearn, int Spec)
{
  if(MaxLearn == 0 ||
     Level == 0)
  {
    Level = -1;
    MaxLearn = -1;
  }

  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].rlevel[Spec] = (byte) (Level);
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].maxlearn[Spec] = (ubyte) (MaxLearn);
}

void SKILL_ADD(int Class, int Level, int MaxLearn)
{
  for( int i = 0; i < MAX_SPEC+1; i++ )
  {
    skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].rlevel[i] =
      (byte) (Level);
    skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].maxlearn[i] =
      (ubyte) (MaxLearn);
  }
}

void SPELL_CREATE2(const char *Name, int Index, int Beats,
                   int Targets,
                   void (*Spell_pointer) (int, P_char, char *, int, P_char,
                                          P_obj), const char *Wear_off,
                   const char *wear_off_room)
{
  SPELL_CREATE_MSG(Name, Index, Beats, Targets,
               Spell_pointer, Wear_off);
  skills[Index].wear_off_room[0] = wear_off_room;
}

void SPEC_SPELL_ADD(int Class, int Level, int Spec)
{
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].rlevel[Spec] =
    (byte) (Level);
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].maxlearn[Spec] = 100;
}

void SPELL_ADD(int Class, int Level)
{
  for( int i = 0; i < MAX_SPEC+1; i++ ) {
    skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].rlevel[i] =
      (byte) (Level);
    skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].maxlearn[i] = 100;
  }
}

#endif

//#ifdef SKILLPOINTS
// Skill has dependency of needing MinDepend in Dependency.
//void SKILL_DEPEND( int Skill, int Dependency, int MinDepend )
//{
//  for( int i = 0; i < 5; i++ )
//  {
//    if( skills[Skill].dependency[i] == 0 )
//    {
//      skills[Skill].dependency[i] = Dependency;
//      skills[Skill].mintotrain[i] = MinDepend;
//      break;
//    }
//  }
//}
//#endif

void create_poisons();
void create_tags();

void initialize_skills()
{
  int i;

  memset(skills, 0, sizeof(Skill) * (MAX_AFFECT_TYPES+1));

  for( i = 0; i < MAX_AFFECT_TYPES; i++ )
  {
    skills[i].name = "Undefined";
  }

  //needed by setbit
  skills[MAX_AFFECT_TYPES].name = "\n";

  // Alchemist
  // Brawler

// Psionicist
  SKILL_CREATE("flame mastery", SKILL_FLAME_MASTERY, TAR_MENTAL);
  SPEC_SKILL_ADD(CLASS_PSIONICIST, 46, 100, SPEC_PYROKINETIC);

  SKILL_CREATE("disperse flames", SKILL_DISPERSE_FLAMES, TAR_MENTAL);
  SPEC_SKILL_ADD(CLASS_PSIONICIST, 41, 100, SPEC_PYROKINETIC);

  // Dreadlord
  SKILL_CREATE("fade", SKILL_FADE, TAR_PHYS );
  //SPEC_SKILL_ADD(CLASS_DREADLORD, 51, 100, SPEC_SHADOWLORD);

  SKILL_CREATE("shadow movement", SKILL_SHADOW_MOVEMENT, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_DREADLORD, 31, 100, SPEC_SHADOWLORD);

  SKILL_CREATE("soul trap", SKILL_SOUL_TRAP, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_DREADLORD, 36, 100, SPEC_DEATHLORD);


/* Mercenary Specs */
  // Opportunitist
  SKILL_CREATE("disruptive blow", SKILL_DISRUPTIVE_BLOW, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 46, 100, SPEC_BRIGAND);

  SKILL_CREATE("crippling strike", SKILL_CRIPPLING_STRIKE, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 41, 100, SPEC_BRIGAND);

  SKILL_CREATE("ambush", SKILL_AMBUSH, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 36, 100, SPEC_BRIGAND);

  SKILL_CREATE("mug", SKILL_MUG, TAR_PHYS);
//  SPEC_SKILL_ADD(CLASS_MERCENARY, 51, 100, SPEC_PROFITEER);

/* Assassin Specs */
  SKILL_CREATE("spinal tap", SKILL_SPINAL_TAP, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ASSASSIN, 35, 100, SPEC_ASSMASTER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 35, 100, SPEC_ASSASSIN);

  SKILL_CREATE("hamstring", SKILL_HAMSTRING, TAR_PHYS);
  SKILL_ADD(CLASS_THIEF, 51, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 100, SPEC_THIEF);

 /* SKILL_CREATE("instant kill", SKILL_INSTANT_KILL, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 56, 100, SPEC_ASSASSIN);*/
  
  SPELL_CREATE("repair one item", SPELL_REPAIR_ONE_ITEM, PULSE_SPELLCAST * 1,
    TAR_OBJ_INV | TAR_OBJ_EQUIP, spell_repair_one_item);

/* Conj Specs */
  SPELL_CREATE("magma burst", SPELL_MAGMA_BURST, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_magma_burst);
  SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_FIRE);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 10, SPEC_RUINER);

  SPELL_CREATE("solar flare", SPELL_SOLAR_FLARE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_solar_flare);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_FIRE);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 10, SPEC_STARMAGUS);

  SPELL_CREATE("water to life", SPELL_WATER_TO_LIFE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_water_to_life);
  SPEC_SPELL_ADD(CLASS_CONJURER, 7, SPEC_WATER);

  SPELL_CREATE_MSG("air form", SPELL_AIR_FORM, PULSE_SPELLCAST * 4,
                TAR_SELF_ONLY,
                spell_air_form, "Your molecules return to normal.");
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_AIR);

  SPELL_CREATE("ethereal grounds", SPELL_ETHEREAL_GROUNDS, PULSE_SPELLCAST * 4,
                TAR_IGNORE,
                spell_ethereal_grounds);
  SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_AIR);
 
  SPELL_CREATE("electrical execution", SPELL_ELECTRICAL_EXECUTION, (int)(PULSE_SPELLCAST * 1.5),
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_electrical_execution);
  SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_AIR);
  SPELL_ADD(CLASS_SORCERER, 9);
  
  SPELL_CREATE2("earthen tomb", SPELL_EARTHEN_TOMB, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_earthen_tomb, "&+yThe ground stops to &+Lrumble &+yand ceases to quake.&n", "&+yThe ground stops to &+Lrumble &+yand ceases to quake.&n");
  SPEC_SPELL_ADD(CLASS_CONJURER, 12, SPEC_EARTH);

  SPELL_CREATE("dread wave", SPELL_DREAD_WAVE, PULSE_SPELLCAST * 1,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_dread_wave);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_WATER);

/* Cleric Specs */
  SPELL_CREATE("lesser sanctuary", SPELL_LESSER_SANCTUARY, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY, spell_sanctuary);
  SPEC_SPELL_ADD(CLASS_CLERIC, 11, SPEC_HOLYMAN);

  SPELL_CREATE("aura of rebirth", SPELL_AURA_OF_REBIRTH, PULSE_SPELLCAST * 10,
	            TAR_IGNORE, cast_area_resurrect);
  SPEC_SPELL_ADD(CLASS_CLERIC, 12, SPEC_HEALER);

  SPELL_CREATE_MSG("battletide", SPELL_BATTLETIDE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_battletide, "The sound of &+rwar&n no longer rings in your ears.");
  SPEC_SPELL_ADD(CLASS_CLERIC, 12, SPEC_ZEALOT);

  SPELL_CREATE("prayer", SPELL_PRAYER, PULSE_SPELLCAST * 5,
	            TAR_IGNORE, spell_prayer);
  SPEC_SPELL_ADD(CLASS_CLERIC, 12, SPEC_HOLYMAN);
  
/* Warrior Specs */
  SKILL_CREATE("quadruple attack", SKILL_QUADRUPLE_ATTACK, TAR_PHYS);
  SKILL_ADD(CLASS_DREADLORD, 56, 80);
  SKILL_ADD(CLASS_AVENGER, 56, 80);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 56, 50, SPEC_VIOLATOR);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 56, 100, SPEC_SWORDSMAN);
  SPEC_SKILL_ADD(CLASS_RANGER, 55, 50, SPEC_BLADEMASTER);



  SKILL_CREATE("shield punch", SKILL_SHIELDPUNCH, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 46, 100, SPEC_GUARDIAN);

  SKILL_CREATE("dreadnaught", SKILL_DREADNAUGHT, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 30, 100, SPEC_GUARDIAN);


  //SPEC_SKILL_ADD(CLASS_WARRIOR, 36, 100, SPEC_SWORDSMAN);

  SKILL_CREATE("sweeping thrust", SKILL_SWEEPING_THRUST, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 31, 100, SPEC_SWORDSMAN);

  SKILL_CREATE("follow-up riposte", SKILL_FOLLOWUP_RIPOSTE, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 31, 100, SPEC_GUARDIAN);
  SPEC_SKILL_ADD(CLASS_RANGER, 51, 60, SPEC_BLADEMASTER);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 50, 60, SPEC_RAGELORD);

  SKILL_CREATE("critical attack", SKILL_CRITICAL_ATTACK, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 41, 100, SPEC_SWORDSMAN);

  SKILL_CREATE_WITH_MESSAGES("war cry", SKILL_WAR_CRY,
                             TAR_PHYS, "Your battle frenzy fades.",
                             NULL);
	SKILL_ADD(CLASS_BERSERKER, 26, 100);

  SKILL_CREATE_WITH_MESSAGES("roar of heroes", SKILL_ROAR_OF_HEROES,
                             TAR_PHYS, "Your heroic urges slowly fade away.",
                             NULL);

/* Anti-paladin Specs */
  SPELL_CREATE_MSG("spawn", SPELL_SPAWN, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY,
                spell_spawn, "&+LThe aura of death slowly fades&n.");

  SPELL_CREATE_MSG("dread blade", SPELL_DREAD_BLADE, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY,
                spell_dread_blade, "&+LThe putrid &+raura&+L surrounding your weapon fades.&n");
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 9, SPEC_DARKKNIGHT);

  SKILL_CREATE("skewer", SKILL_SKEWER, TAR_PHYS);
  SKILL_ADD(CLASS_ANTIPALADIN, 41, 80);
  SKILL_ADD(CLASS_PALADIN, 41, 80);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 36, 100, SPEC_DARKKNIGHT);
  SPEC_SKILL_ADD(CLASS_PALADIN, 36, 100, SPEC_CRUSADER);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 0, 0, SPEC_VIOLATOR);

  SKILL_CREATE("smite evil", SKILL_SMITE_EVIL, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_PALADIN, 30, 100, SPEC_CRUSADER);

  SKILL_CREATE("restrain", SKILL_RESTRAIN, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 30, 100, SPEC_VIOLATOR);

  SPELL_CREATE("group heal", SPELL_GROUP_HEAL, PULSE_SPELLCAST * 4,
                TAR_IGNORE, spell_group_heal);
 SPELL_ADD(CLASS_PALADIN, 10);
 SPEC_SPELL_ADD(CLASS_CLERIC, 11, SPEC_HEALER);

  SPELL_CREATE("apocalypse", SPELL_APOCALYPSE, (7/2) * PULSE_SPELLCAST,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO, spell_apocalypse);
  SPELL_ADD(CLASS_ANTIPALADIN, 11);

  SPELL_CREATE("judgement", SPELL_JUDGEMENT, (7/2) * PULSE_SPELLCAST,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO, spell_judgement);
  SPELL_ADD(CLASS_PALADIN, 11);

  SKILL_CREATE("lance charge", SKILL_LANCE_CHARGE, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 41, 100, SPEC_DEMONIC);
  SPEC_SKILL_ADD(CLASS_PALADIN, 41, 100, SPEC_CAVALIER);

  SKILL_CREATE("sidestep", SKILL_SIDESTEP, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 31, 100, SPEC_DEMONIC);
  SPEC_SKILL_ADD(CLASS_PALADIN, 31, 100, SPEC_CAVALIER);

  SKILL_CREATE("arcane riposte", SKILL_ARCANE_RIPOSTE, TAR_MENTAL);
  SPEC_SKILL_ADD(CLASS_SORCERER, 51, 100, SPEC_WIZARD);

  SKILL_CREATE("arcane block", SKILL_ARCANE_BLOCK, TAR_MENTAL);
  SPEC_SKILL_ADD(CLASS_SORCERER, 31, 100, SPEC_WIZARD);  

  SKILL_CREATE("spellweave", SKILL_SPELLWEAVE, TAR_MENTAL);
  SPEC_SKILL_ADD(CLASS_SORCERER, 41, 100, SPEC_WIZARD);
/*
  SKILL_CREATE("ground casting", SKILL_GROUND_CASTING, TAR_MENTAL);
  SKILL_ADD(CLASS_SORCERER, 41, 100);
*/

  SPELL_CREATE("implosion", SPELL_IMPLOSION, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_implosion);
  SPELL_ADD( CLASS_BLIGHTER, 12 );

  SPELL_CREATE("chaotic ripple", SPELL_CHAOTIC_RIPPLE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_chaotic_ripple);
  SPEC_SPELL_ADD(CLASS_SORCERER, 12, SPEC_WILDMAGE);

  SPELL_CREATE("chaos volley", SPELL_CHAOS_VOLLEY, PULSE_SPELLCAST * 5/3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_chaos_volley);
  SPEC_SPELL_ADD(CLASS_SORCERER, 9, SPEC_WILDMAGE);

  SPELL_CREATE("blink", SPELL_BLINK, PULSE_SPELLCAST,
                TAR_SELF_ONLY, spell_blink);
  SPEC_SPELL_ADD(CLASS_SORCERER, 6, SPEC_SHADOW);

  SPELL_CREATE_MSG("shadow projection", SPELL_SHADOW_PROJECTION, PULSE_SPELLCAST * 6,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_shadow_projection, "&+LYour material form realigns with this plane.");
// SPEC_SPELL_ADD(CLASS_SORCERER, 12, SPEC_SHADOW);
//  SPEC_SPELL_ADD(CLASS_ILLUSIONIST, 12, SPEC_DARK_DREAMER);
  
  SPELL_CREATE("obtenebration", SPELL_OBTENEBRATION, PULSE_SPELLCAST * 2.5,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO,
		spell_obtenebration);
  SPEC_SPELL_ADD(CLASS_SORCERER, 12, SPEC_SHADOW);

  SKILL_CREATE("control flee", SKILL_CONTROL_FLEE, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ASSASSIN, 45, 100, SPEC_ASSMASTER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 45, 100, SPEC_ASSASSIN);

  SKILL_CREATE("quick step", SKILL_QUICK_STEP, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 36, 100, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 52, 75, SPEC_THIEF);

/* Sharpshooter Spec Skills */
  SKILL_CREATE("point blank shot", SKILL_POINT_BLANK_SHOT, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 36, 100, SPEC_SHARPSHOOTER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 46, 90, SPEC_THIEF);

  SKILL_CREATE("critical shot", SKILL_CRITICAL_SHOT, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 41, 100, SPEC_SHARPSHOOTER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 46, 90, SPEC_ASSASSIN);

  SKILL_CREATE("expeditious retreat", SKILL_EXPEDITIOUS_RETREAT, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 46, 75, SPEC_SHARPSHOOTER);

  SKILL_CREATE("cursed arrows", SKILL_CURSED_ARROWS, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 100, SPEC_SHARPSHOOTER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 46, 90, SPEC_ASSASSIN);

  SKILL_CREATE("enchant arrows", SKILL_ENCHANT_ARROWS, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 31, 100, SPEC_SHARPSHOOTER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 46, 90, SPEC_THIEF);

  SKILL_CREATE("shadow archery", SKILL_SHADOW_ARCHERY, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 100, SPEC_SHARPSHOOTER);

  SKILL_CREATE("arc", SKILL_INDIRECT_SHOT, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 46, 100, SPEC_SHARPSHOOTER);

/* Monk Specs */
  SKILL_CREATE_WITH_MESSAGES("fist of dragon", SKILL_FIST_OF_DRAGON,
    TAR_MENTAL, "The touch of the dragon leaves your soul.","");
  SPEC_SKILL_ADD(CLASS_MONK, 46, 100, SPEC_WAYOFDRAGON);

  SKILL_CREATE_WITH_MESSAGES("tiger palm", SKILL_TIGER_PALM,
    TAR_MENTAL, "Your tiger palm concentration leaves you.","");
  SPEC_SKILL_ADD(CLASS_MONK, 46, 100, SPEC_WAYOFSNAKE);
  
  SKILL_CREATE("flurry of blows", SKILL_FLURRY_OF_BLOWS, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_MONK, 31, 100, SPEC_WAYOFSNAKE);

/* ranger specs */
  SKILL_CREATE("double strike", SKILL_DOUBLE_STRIKE, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_RANGER, 36, 100, SPEC_BLADEMASTER);

  SKILL_CREATE_WITH_MESSAGES("whirlwind", SKILL_WHIRLWIND,
                             TAR_PHYS, "You slow down exhausted.",
                             "$n slows down exhausted.");
  SPEC_SKILL_ADD(CLASS_RANGER, 31, 100, SPEC_BLADEMASTER);

  SPELL_CREATE("healing salve", SPELL_HEALING_SALVE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM, spell_healing_salve);
  SPEC_SPELL_ADD(CLASS_CLERIC, 6, SPEC_HEALER);

  SPELL_CREATE_MSG("plague", SPELL_PLAGUE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_plague, "Your body managed to defeat the disease.");
  SPEC_SPELL_ADD(CLASS_CLERIC, 11, SPEC_ZEALOT);

  SPELL_CREATE("banish", SPELL_BANISH, PULSE_SPELLCAST * 2,
                TAR_AREA | TAR_OFFAREA, spell_banish);
  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_ZEALOT);

  SKILL_CREATE("vicious attack", SKILL_VICIOUS_ATTACK, TAR_PHYS);
  SKILL_ADD(CLASS_RANGER, 56, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 54, 60, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 56, 70, SPEC_SWASHBUCKLER);

  SKILL_CREATE("parlay", SKILL_PARLAY, 0);
  SPEC_SKILL_ADD(CLASS_RANGER, 36, 100, SPEC_MARSHALL);

  SPELL_CREATE("natures calling", SPELL_NATURES_CALLING, PULSE_SPELLCAST * 3,
                TAR_IGNORE, spell_natures_call);
  SPEC_SPELL_ADD(CLASS_RANGER, 9, SPEC_MARSHALL);


  // Berserker Specs
  SKILL_CREATE("mangle", SKILL_MANGLE, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 51, 100, SPEC_MAULER);

  SKILL_CREATE("boiling blood", SKILL_BOILING_BLOOD, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 41, 100, SPEC_MAULER);

  SKILL_CREATE("grapple", SKILL_GRAPPLE, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 51, 100, SPEC_RAGELORD);

  // Necromancer Specs

  SKILL_CREATE("exhume", SKILL_EXHUME, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_NECROMANCER, 31, 100, SPEC_DIABOLIS);

  SPELL_CREATE("create golem", SPELL_CREATE_GOLEM, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_create_golem);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 9, SPEC_DIABOLIS);
  SPEC_SPELL_ADD(CLASS_THEURGIST, 9, SPEC_MEDIUM);

  SPELL_CREATE("raise shadow", SPELL_RAISE_SHADOW, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_raise_shadow);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 9, SPEC_REAPER);

  SPELL_CREATE("call deva", SPELL_CALL_DEVA, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_call_deva);
  SPEC_SPELL_ADD(CLASS_THEURGIST, 9, SPEC_THAUMATURGE);

  /* end specs */

  /* illusionist specs */

  SPELL_CREATE_MSG("obscuring mist", SPELL_OBSCURING_MIST, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
    spell_obscuring_mist, "The mist creeps away, revealing much of the world to you.");
  SPEC_SPELL_ADD(CLASS_ILLUSIONIST, 10, SPEC_DECEIVER);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 10, SPEC_SCOURGE);

  SPELL_CREATE_MSG("suppress sound", SPELL_SUPPRESSION, PULSE_SPELLCAST * 4,
                TAR_IGNORE,
                spell_suppress_sound, "The world comes alive with sound around you.");
  SPEC_SPELL_ADD(CLASS_ILLUSIONIST, 11, SPEC_DECEIVER);

  SPELL_CREATE("shadow merging", SPELL_SHADOW_MERGE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT, spell_shadow_merge);
  SPEC_SPELL_ADD(CLASS_ILLUSIONIST, 8, SPEC_DARK_DREAMER);

  SPELL_CREATE("gate to ardgral", SPELL_SHDW_GATE_ARDGRAL, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT, cast_ardgral);
  SPEC_SPELL_ADD(CLASS_ILLUSIONIST, 10, SPEC_DARK_DREAMER);

  SPELL_CREATE("shadow spawn", SPELL_SHADOW_SPAWN, PULSE_SPELLCAST * 3,
                TAR_OFFAREA | TAR_AGGRO, spell_shadow_burst);
  SPEC_SPELL_ADD(CLASS_ILLUSIONIST, 9, SPEC_DARK_DREAMER);

  SPELL_CREATE("asphyxiate", SPELL_ASPHYXIATE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_asphyxiate);
  SPELL_ADD(CLASS_ILLUSIONIST, 9);

  SPELL_CREATE("mirage", SPELL_MIRAGE, PULSE_SPELLCAST * 8,
                TAR_IGNORE | TAR_NOCOMBAT, spell_mirage);
  SPELL_ADD(CLASS_ILLUSIONIST, 12);

  /* end specs */

  SPELL_CREATE("pleasantry", SPELL_PLEASANTRY, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_AGGRO, spell_pleasantry);

  SPELL_CREATE("Auctions Disabled", SPELL_NOAUCTION, PULSE_SPELLCAST,
		  TAR_CHAR_ROOM | TAR_AGGRO, do_nothing_spell);

  SPELL_CREATE("Battle Mages Aura", SPELL_BATTLEMAGE, PULSE_SPELLCAST,
		  TAR_CHAR_ROOM | TAR_AGGRO, do_nothing_spell);

  SPELL_CREATE("contain being", SPELL_CONTAIN_BEING, PULSE_SPELLCAST * 3,
		  TAR_CHAR_ROOM | TAR_FIGHT_VICT, spell_contain_being);
  SPEC_SPELL_ADD(CLASS_SUMMONER, 5, SPEC_CONTROLLER);
  SPEC_SPELL_ADD(CLASS_SUMMONER, 5, SPEC_MENTALIST);
  SPEC_SPELL_ADD(CLASS_SUMMONER, 5, SPEC_NATURALIST);

  SPELL_CREATE("corpse portal", SPELL_CORPSE_PORTAL, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT, spell_corpse_portal);

  SPELL_CREATE("corpseform", SPELL_CORPSEFORM, PULSE_SPELLCAST * 7,
                TAR_OBJ_ROOM | TAR_NOCOMBAT,spell_corpseform);
  SPELL_ADD(CLASS_NECROMANCER, 11);
  SPELL_ADD(CLASS_THEURGIST, 11);

  SPELL_CREATE_MSG("blackmantle", SPELL_BMANTLE, PULSE_SPELLCAST * 4,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_blackmantle, "&+LThe &+bnegative energy droplets &+Ldissipate harmlessly.");
  SPELL_ADD(CLASS_NECROMANCER, 10);
  SPELL_ADD(CLASS_THEURGIST, 10);

  SPELL_CREATE("negative concussion blast", SPELL_NEGATIVE_CONCUSSION_BLAST, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_negative_concussion_blast);
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_THEURGIST, 8);
//  SPELL_ADD(CLASS_CABALIST, 8);

  SPELL_CREATE("create greater dracolich",
                SPELL_CREATE_GREATER_DRACOLICH, PULSE_SPELLCAST * 10,
                TAR_OBJ_ROOM | TAR_NOCOMBAT,
                spell_create_greater_dracolich);
  SPELL_ADD(CLASS_NECROMANCER, 12);

  SPELL_CREATE("call avatar",
                SPELL_CALL_AVATAR, PULSE_SPELLCAST * 10,
                TAR_OBJ_ROOM | TAR_NOCOMBAT,
                spell_call_avatar);
  SPELL_ADD(CLASS_THEURGIST, 12);

  SPELL_CREATE("undeath to death", SPELL_UNDEAD_TO_DEATH, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_undead_to_death);
  SPELL_ADD(CLASS_NECROMANCER, 9);
  SPELL_ADD(CLASS_THEURGIST, 9);


  SPELL_CREATE_MSG("regeneration", SPELL_REGENERATION, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_regeneration, "You stop regenerating.");
  SPELL_ADD(CLASS_DRUID, 6);
  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_HEALER);

  SPELL_CREATE("grow", SPELL_GROW, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT, cast_grow);
  SPEC_SPELL_ADD(CLASS_DRUID, 10, SPEC_WOODLAND);

  SPELL_CREATE("depressed earth", SPELL_DEPRESSED_EARTH, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT | TAR_SPIRIT,
                cast_depressed_earth);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 10, SPEC_SPIRITUALIST);

  SPELL_CREATE("transmute rock to mud", SPELL_TRANS_ROCK_MUD, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_transmute_rock_mud);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_EARTH);
  SPELL_ADD( CLASS_BLIGHTER, 10 );

  SPELL_CREATE("transmute mud to rock", SPELL_TRANS_MUD_ROCK, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_transmute_mud_rock);
  //SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_EARTH);
  //SPELL_ADD( CLASS_BLIGHTER, 10 );

  SPELL_CREATE("transmute mud to water", SPELL_TRANS_MUD_WATER, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_transmute_mud_water);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_WATER);

  SPELL_CREATE("transmute water to mud", SPELL_TRANS_WATER_MUD, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_transmute_water_mud);
  //SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_WATER);

  SPELL_CREATE("transmute water to air", SPELL_TRANS_WATER_AIR, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_transmute_water_air);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_AIR);

  SPELL_CREATE("transmute air to water", SPELL_TRANS_AIR_WATER, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_transmute_air_water);
  //SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_AIR);

  SPELL_CREATE("transmute rock to lava", SPELL_TRANS_ROCK_LAVA, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_transmute_rock_lava);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_FIRE);

  SPELL_CREATE("transmute lava to rock", SPELL_TRANS_LAVA_ROCK, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                cast_transmute_lava_rock);
  //SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_FIRE);

  SPELL_CREATE("vines", SPELL_VINES, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY, cast_vines);

  SPEC_SPELL_ADD(CLASS_DRUID, 9, SPEC_WOODLAND);

  SPELL_CREATE("awaken forest", SPELL_AWAKEN_FOREST, PULSE_SPELLCAST * 4,
                TAR_AREA | TAR_AGGRO, cast_awaken_forest);
  SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_WOODLAND);


  SPELL_CREATE("spike growth", SPELL_SPIKE_GROWTH, PULSE_SPELLCAST * 5,
                TAR_AREA | TAR_AGGRO, cast_spike_growth);
  SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_STORM);

  SPELL_CREATE("hurricane", SPELL_HURRICANE, PULSE_SPELLCAST * 5,
                TAR_AREA | TAR_AGGRO, cast_hurricane);
  SPEC_SPELL_ADD(CLASS_DRUID, 10, SPEC_STORM);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 11, SPEC_TEMPESTMAGUS);

  SPELL_CREATE_MSG("storm shield", SPELL_STORMSHIELD, PULSE_SPELLCAST * 5,
                TAR_SELF_ONLY,
                cast_storm_shield, "Your shield is no longer infused with pure energy.");

  SPEC_SPELL_ADD(CLASS_DRUID, 9, SPEC_STORM);

  SPELL_CREATE_MSG("blood to stone", SPELL_BLOODTOSTONE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                cast_bloodstone, "Your blood flows normally again.");
  SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_STORM);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 11, SPEC_STORMBRINGER);

  SPELL_CREATE_MSG("waves of fatigue", SPELL_WAVES_FATIGUE, PULSE_SPELLCAST * 4,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_waves_fatigue, "You feel like moving again.");
  SPELL_ADD( CLASS_BLIGHTER, 6 );

  SPELL_CREATE_MSG("accelerated healing", SPELL_ACCEL_HEALING, (PULSE_SPELLCAST * 3) / 2,
                TAR_CHAR_ROOM,
                spell_accel_healing, "You no longer regenerate.");
  SPELL_ADD(CLASS_CLERIC, 10);
  SPELL_ADD(CLASS_PALADIN, 11);

  SPELL_CREATE_MSG("endurance", SPELL_ENDURANCE, PULSE_SPELLCAST * 3 / 2,
                TAR_SELF_ONLY,
                spell_endurance, "Your endurance fades away.");
  SPELL_ADD(CLASS_DRUID, 6);
  SPEC_SPELL_ADD(CLASS_RANGER, 8, SPEC_HUNTSMAN);
  SPEC_SPELL_ADD(CLASS_CLERIC, 7, SPEC_HEALER);

  SPELL_CREATE_MSG("sap nature", SPELL_SAP_NATURE, PULSE_SPELLCAST * 3 / 2,
                TAR_SELF_ONLY, spell_sap_nature, "&+yThe &+Genergy&+y ceases to flow.&n");
  SPELL_ADD(CLASS_BLIGHTER, 6);

  SPELL_CREATE_MSG("fortitude", SPELL_FORTITUDE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_fortitude, "You feel less determined to go on with life.");
  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_BLIGHTER, 1);
	SPEC_SPELL_ADD(CLASS_RANGER, 7, SPEC_BLADEMASTER);

  SPELL_CREATE_MSG("armor", SPELL_ARMOR, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_armor, "You feel less &+Wprotected&n.");
  SPELL_ADD(CLASS_CLERIC, 1);
  SPELL_ADD(CLASS_PALADIN, 3);
  SPELL_ADD(CLASS_ANTIPALADIN, 3);


  SPELL_CREATE_MSG("virtue", SPELL_VIRTUE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_virtue, "You no longer feel the call of the &+Wgods&n.");
  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_HOLYMAN );
  SPEC_SPELL_ADD(CLASS_RANGER, 8, SPEC_MARSHALL);

  SPELL_CREATE("group teleport", SPELL_GROUP_TELEPORT, PULSE_SPELLCAST,
                TAR_IGNORE, spell_group_teleport);

  SPELL_CREATE("shatter", SPELL_SHATTER, PULSE_SPELLCAST * 1,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_shatter);

  SPEC_SPELL_ADD(CLASS_BARD, 9, SPEC_DISHARMONIST);

  SPELL_CREATE("enrage", SPELL_ENRAGE, PULSE_SPELLCAST/2,
                TAR_CHAR_ROOM + TAR_FIGHT_VICT | TAR_AGGRO, spell_enrage);
  SPELL_ADD(CLASS_MINDFLAYER, 9);
  SPELL_ADD(CLASS_PSIONICIST, 12);

  SPELL_CREATE("excogitate", SPELL_EXCOGITATE, PULSE_SPELLCAST/2, TAR_CHAR_ROOM, spell_excogitate);
  SPELL_ADD(CLASS_MINDFLAYER, 1);
  SPELL_ADD(CLASS_PSIONICIST, 1);

  SPELL_CREATE("detonate", SPELL_DETONATE, (PULSE_SPELLCAST * 3) / 2,
                TAR_CHAR_ROOM + TAR_FIGHT_VICT | TAR_AGGRO, spell_detonate);
  SPELL_ADD(CLASS_MINDFLAYER, 6);
  SPELL_ADD(CLASS_PSIONICIST, 7);

  SPELL_CREATE_MSG("fire aura", SPELL_FIRE_AURA, PULSE_SPELLCAST * 6,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_fire_aura, "&+BYour &+rfiery aura &+Breturns to normal.&N");
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 12, SPEC_PYROKINETIC);

  SPELL_CREATE("ether warp", SPELL_ETHER_WARP, PULSE_SPELLCAST * 3,
                TAR_CHAR_WORLD | TAR_NOCOMBAT, spell_ether_warp);
  SPELL_ADD(CLASS_MINDFLAYER, 7);
  SPELL_ADD(CLASS_PSIONICIST, 9);

  SPELL_CREATE("ethereal rift", SPELL_ETHEREAL_RIFT, PULSE_SPELLCAST * 2,
                TAR_CHAR_WORLD | TAR_NOCOMBAT, spell_ethereal_rift);
//  SPELL_ADD(CLASS_MINDFLAYER, 10);

  SPELL_CREATE("wormhole", SPELL_WORMHOLE, PULSE_SPELLCAST * 6,
                TAR_CHAR_WORLD | TAR_NOCOMBAT, spell_wormhole);
  SPELL_ADD(CLASS_MINDFLAYER, 9);
  SPELL_ADD(CLASS_PSIONICIST, 10);

  SPELL_CREATE("thought beacon", SPELL_THOUGHT_BEACON, PULSE_SPELLCAST * 5,
                TAR_IGNORE | TAR_NOCOMBAT, spell_thought_beacon);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 12, SPEC_PSYCHEPORTER);

  SPELL_CREATE("radial navigation", SPELL_RADIAL_NAV, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT,
                spell_radial_navigation);
  SPELL_ADD(CLASS_MINDFLAYER, 8);

  SPELL_CREATE_MSG("molecular control", SPELL_MOLECULAR_CONTROL, PULSE_SPELLCAST*2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_molecular_control, "You lose control of your molecules.");
  SPELL_ADD(CLASS_MINDFLAYER, 4);
  SPELL_ADD(CLASS_PSIONICIST, 8);

  SPELL_CREATE("molecular agitation", SPELL_MOLECULAR_AGITATION, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_molecular_agitation);
  SPELL_ADD(CLASS_MINDFLAYER, 3);
  SPELL_ADD(CLASS_PSIONICIST, 3);

  SPELL_CREATE("spinal corruption", SPELL_SPINAL_CORRUPTION, PULSE_SPELLCAST*3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_spinal_corruption);
  SPELL_ADD(CLASS_MINDFLAYER, 12);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 12, SPEC_ENSLAVER);

  SPELL_CREATE_MSG("adrenaline control", SPELL_ADRENALINE_CONTROL, PULSE_SPELLCAST*2,
                TAR_SELF_ONLY,
                spell_adrenaline_control, "&+rYour rush of adrenaline runs dry.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 1);
  SPELL_ADD(CLASS_PSIONICIST, 1);

  SPELL_CREATE_MSG("aura sight", SPELL_AURA_SIGHT, PULSE_SPELLCAST*2,
                TAR_SELF_ONLY,
                spell_aura_sight, "&+CThe auras in your vision disappear.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 3);
  SPELL_ADD(CLASS_PSIONICIST, 3);

  SPELL_CREATE_MSG("awe", SPELL_AWE, PULSE_SPELLCAST * 4,
                TAR_CHAR_ROOM,
                spell_awe, "You regain your senses.");
  SPELL_ADD(CLASS_MINDFLAYER, 4);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 6, SPEC_ENSLAVER);

  SPELL_CREATE("ballistic attack", SPELL_BALLISTIC_ATTACK, PULSE_SPELLCAST*2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO,
                spell_ballistic_attack);
  SPELL_ADD(CLASS_MINDFLAYER, 2);
  SPELL_ADD(CLASS_PSIONICIST, 2);

  SPELL_CREATE_MSG("mental anguish", SPELL_MENTAL_ANGUISH, PULSE_SPELLCAST/2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO,
                spell_mental_anguish, "Your mind recovers from the assault.");
 // Don't need to worry about this since we don't cast spells in the DE.
#ifndef _DE_
  skills[SPELL_MENTAL_ANGUISH].wear_off_room[0] = "$n straightens up and begins fighting more confidently.";
#endif
  SPELL_ADD(CLASS_MINDFLAYER, 8);

  SPELL_CREATE("memory block", SPELL_MEMORY_BLOCK, PULSE_SPELLCAST*2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO, spell_memory_block);
  SPELL_ADD(CLASS_MINDFLAYER, 9);

  SPELL_CREATE_MSG("biofeedback", SPELL_BIOFEEDBACK, (PULSE_SPELLCAST * 5)/2,
                TAR_SELF_ONLY,
                spell_biofeedback, "&+GThe green mist around your body fades.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 7);
  SPELL_ADD(CLASS_PSIONICIST, 11);

  SPELL_CREATE_MSG("displacement", SPELL_DISPLACEMENT, PULSE_SPELLCAST*2,
                TAR_SELF_ONLY,
                spell_displacement, "&+WYour body shifts back into this space-time.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 8);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 9, SPEC_PSYCHEPORTER);

  SPELL_CREATE("cell adjustment", SPELL_CELL_ADJUSTMENT, PULSE_SPELLCAST*2,
                TAR_SELF_ONLY,
                spell_cell_adjustment);
  SPELL_ADD(CLASS_MINDFLAYER, 2);
  SPELL_ADD(CLASS_PSIONICIST, 6);

  SPELL_CREATE_MSG("combat mind", SPELL_COMBAT_MIND, PULSE_SPELLCAST*2,
                TAR_SELF_ONLY,
                spell_combat_mind, "You forget your battle tactics.");
  SPELL_ADD(CLASS_MINDFLAYER, 1);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 1, SPEC_PSYCHEPORTER);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 1, SPEC_ENSLAVER);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 1, SPEC_PYROKINETIC);

/* what the hell is this? -Zod */
/* a fun joke! -Tavril */
  //SPELL_CREATE("area headbutt", SPELL_CREATE_EARTHEN_PROJ, PULSE_SPELLCAST,
    //            TAR_SELF_ONLY, spell_ego_blast);
  //SPELL_ADD(CLASS_PSIONICIST, 12);

  SPELL_CREATE("ego blast", SPELL_EGO_BLAST, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_ego_blast);
  SPELL_ADD(CLASS_PSIONICIST, 4);

  SPELL_CREATE("control flames", SPELL_CONTROL_FLAMES, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_control_flames);



  SPELL_CREATE("create sound", SPELL_CREATE_SOUND, PULSE_SPELLCAST*2,
                TAR_IGNORE, spell_create_sound);
  SPELL_ADD(CLASS_MINDFLAYER, 2);
  SPELL_ADD(CLASS_PSIONICIST, 6);

  SPELL_CREATE("death field", SPELL_DEATH_FIELD, PULSE_SPELLCAST * 3,
                TAR_AREA | TAR_OFFAREA, spell_death_field);
  SPELL_ADD(CLASS_MINDFLAYER, 10);
  SPELL_ADD(CLASS_PSIONICIST, 10);

  SPELL_CREATE("psionic cloud", SPELL_PSIONIC_CLOUD, PULSE_SPELLCAST * 2,
                TAR_OFFAREA | TAR_AREA, spell_psionic_cloud);
  SPELL_ADD(CLASS_MINDFLAYER, 10);

  SPELL_CREATE("ectoplasmic form", SPELL_ECTOPLASMIC_FORM, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_ectoplasmic_form);
  SPELL_ADD(CLASS_MINDFLAYER, 6);
  SPELL_ADD(CLASS_PSIONICIST, 8);

  SPELL_CREATE("ego whip", SPELL_EGO_WHIP, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO, spell_ego_whip);
  SPELL_ADD(CLASS_MINDFLAYER, 1);
  SPELL_ADD(CLASS_PSIONICIST, 1);

  SPELL_CREATE("psionic wave blast", SPELL_PSIONIC_WAVE_BLAST, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO, spell_psionic_wave_blast);
  // SPELL_ADD(CLASS_PSIONICIST, 11);
  // SPELL_ADD(CLASS_MINDFLAYER, 11);

  SPELL_CREATE_MSG("energy containment", SPELL_ENERGY_CONTAINMENT, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_energy_containment, "&+YYou can no longer absorb energy.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 4);
  SPELL_ADD(CLASS_PSIONICIST, 4);

  SPELL_CREATE_MSG("enhance armor", SPELL_ENHANCE_ARMOR, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_enhance_armor, "&+BThe psionic armor around you fades.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 6);

  SPELL_CREATE_MSG("enhance strength", SPELL_ENHANCED_STR, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_enhanced_strength, "&+CYour muscles return to normal.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 2);
  SPELL_ADD(CLASS_PSIONICIST, 2);

  SPELL_CREATE_MSG("enhance dexterity", SPELL_ENHANCED_DEX, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_enhanced_dexterity, "&+yYour coordination decreases.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 2);
  SPELL_ADD(CLASS_PSIONICIST, 2);

  SPELL_CREATE_MSG("enhance agility", SPELL_ENHANCED_AGI, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_enhanced_agility, "&+CYou feel a bit more clumsy.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 2);
  SPELL_ADD(CLASS_PSIONICIST, 2);

  SPELL_CREATE_MSG("enhance constitution", SPELL_ENHANCED_CON, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_enhanced_constitution, "&+YYour vitality drains away.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 2);
  SPELL_ADD(CLASS_PSIONICIST, 2);

  SPELL_CREATE_MSG("flesh armor", SPELL_FLESH_ARMOR, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_flesh_armor, "&+rYour flesh softens.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 5);
  SPELL_ADD(CLASS_PSIONICIST, 4);

  SPELL_CREATE_MSG("inertial barrier", SPELL_INERTIAL_BARRIER, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_inertial_barrier, "Your &+Winertial barrier&n fades away.");
  SPELL_ADD(CLASS_MINDFLAYER, 7);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 8, SPEC_PSYCHEPORTER);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 8, SPEC_ENSLAVER);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 8, SPEC_PYROKINETIC);

  SPELL_CREATE("inflict pain", SPELL_INFLICT_PAIN, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_inflict_pain);
  SPELL_ADD(CLASS_MINDFLAYER, 5);
  SPELL_ADD(CLASS_PSIONICIST, 5);

  SPELL_CREATE_MSG("intellect fortress", SPELL_INTELLECT_FORTRESS, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_intellect_fortress, "&+yYour virtual fortress crumbles.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 5);
  SPELL_ADD(CLASS_PSIONICIST, 6);

  SPELL_CREATE("lend health", SPELL_LEND_HEALTH, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT, spell_lend_health);
  SPELL_ADD(CLASS_MINDFLAYER, 3);
  SPELL_ADD(CLASS_PSIONICIST, 3);


  SPELL_CREATE_MSG("flight", SPELL_POWERCAST_FLY, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_fly, "&+CYour feet slowly descend to the ground.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 5);
  SPELL_ADD(CLASS_PSIONICIST, 6);
  SPEC_SPELL_ADD(CLASS_REAVER, 9, SPEC_SHOCK_REAVER);

  SPELL_CREATE("confuse", SPELL_CONFUSE, PULSE_SPELLCAST * 4,
                TAR_AREA | TAR_AGGRO, spell_confuse);
  SPELL_ADD(CLASS_MINDFLAYER, 9);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 12, SPEC_PYROKINETIC);


  SPELL_CREATE_MSG("mind blank", SPELL_MIND_BLANK, PULSE_SPELLCAST * 8,
                TAR_SELF_ONLY,
                spell_mind_blank, "&+YYour mind is not as concealed anymore!&N");
  SPELL_ADD(CLASS_MINDFLAYER, 12);

  SPELL_CREATE_MSG("psychic crush", SPELL_PSYCHIC_CRUSH, (5 * PULSE_SPELLCAST) / 2,
                TAR_CHAR_ROOM + TAR_FIGHT_VICT | TAR_AGGRO,
				spell_psychic_crush, "&+MYou feel your mind recover from the assault.&n");
  SPELL_ADD(CLASS_PSIONICIST, 9);

  SPELL_CREATE_MSG("pyrokinesis", SPELL_PYROKINESIS, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM + TAR_FIGHT_VICT | TAR_AGGRO,
				spell_pyrokinesis, "&+MYou feel your body recover from the heat.&n");
  //SPELL_ADD(CLASS_PSIONICIST, 11);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 11, SPEC_PYROKINETIC);

  SPELL_CREATE_MSG("cannibalize", SPELL_CANNIBALIZE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_cannibalize, "&+YYou can no longer drain your victim!&N");
  SPELL_ADD(CLASS_MINDFLAYER, 7);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 8, SPEC_PSYCHEPORTER);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 8, SPEC_ENSLAVER);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 8, SPEC_PYROKINETIC);

/* Removed
  SPELL_CREATE("sight link", SPELL_SIGHT_LINK, PULSE_SPELLCAST * 2,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT, spell_sight_link);
  SPELL_ADD(CLASS_MINDFLAYER, 11);
*/
  SPELL_CREATE("mind travel", SPELL_MIND_TRAVEL, PULSE_SPELLCAST * 6,
                TAR_SELF_ONLY | TAR_NOCOMBAT, spell_mind_travel);
  SPELL_ADD(CLASS_MINDFLAYER, 11);

  SPELL_CREATE_MSG("tower of iron will", SPELL_TOWER_IRON_WILL, PULSE_SPELLCAST * 4,
                TAR_SELF_ONLY,
                spell_tower_iron_will, "&+YYou just lost your tower of iron will!&N");
  SPELL_ADD(CLASS_MINDFLAYER, 7);
  SPELL_ADD(CLASS_PSIONICIST, 7);

  SPELL_CREATE("celerity", SPELL_CELERITY, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT, spell_celerity);
//  SPELL_ADD(CLASS_PSIONICIST, 5);

/***** End of psionics *****/

  SPELL_CREATE("magic missile", SPELL_MAGIC_MISSILE, PULSE_SPELLCAST * 2/3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO, spell_magic_missile);
  SPELL_ADD(CLASS_SORCERER, 1);
  SPELL_ADD(CLASS_CONJURER, 1);
  SPELL_ADD(CLASS_SUMMONER, 1);
//  SPELL_ADD(CLASS_RANGER, 3);
//  SPELL_ADD(CLASS_NECROMANCER, 1); removing it, since they now get slashing darkness
  SPELL_ADD(CLASS_BARD, 3);
  SPELL_ADD(CLASS_ILLUSIONIST, 1);
  SPELL_ADD(CLASS_REAVER, 2);
//  SPELL_ADD(CLASS_THEURGIST, 1);

  SPELL_CREATE_MSG("chill touch", SPELL_CHILL_TOUCH, PULSE_SPELLCAST * 7/9,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_chill_touch, "You feel less &+Bchilled&n.");
  SPELL_ADD(CLASS_NECROMANCER, 2);
  SPELL_ADD(CLASS_CONJURER, 2);
  SPELL_ADD(CLASS_SUMMONER, 2);
  SPELL_ADD(CLASS_SORCERER, 2);
//  SPELL_ADD(CLASS_RANGER, 5);
  SPELL_ADD(CLASS_ETHERMANCER, 2);
  SPELL_ADD(CLASS_REAVER, 3);
  SPELL_ADD(CLASS_THEURGIST, 2);
  SPELL_ADD(CLASS_BLIGHTER, 2);

  SPELL_CREATE("burning hands", SPELL_BURNING_HANDS, PULSE_SPELLCAST * 7/9,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_burning_hands);
  SPELL_ADD(CLASS_SORCERER, 2);
  SPELL_ADD(CLASS_CONJURER, 2);
  SPELL_ADD(CLASS_SUMMONER, 2);
  SPELL_ADD(CLASS_BARD, 4);
  SPELL_ADD(CLASS_ILLUSIONIST, 2);
//  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_ZEALOT);
  SPEC_SPELL_ADD(CLASS_REAVER, 5, SPEC_FLAME_REAVER);
  SPELL_ADD(CLASS_BLIGHTER, 2);

  SPELL_CREATE("shocking grasp", SPELL_SHOCKING_GRASP, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_shocking_grasp);
  SPELL_ADD(CLASS_SORCERER, 3);
  SPELL_ADD(CLASS_CONJURER, 3);
  SPELL_ADD(CLASS_SUMMONER, 3);
  SPEC_SPELL_ADD(CLASS_REAVER, 5, SPEC_SHOCK_REAVER);

  SPELL_CREATE_MSG("frostbite", SPELL_FROSTBITE, PULSE_SPELLCAST * 11/9,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO, spell_frostbite, "You feel less &+Cfrosty&n.");
  SPELL_ADD(CLASS_WARLOCK, 6);
  SPELL_ADD(CLASS_ETHERMANCER, 6);

  SPELL_CREATE("lightning bolt", SPELL_LIGHTNING_BOLT, PULSE_SPELLCAST * 1,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO, spell_lightning_bolt);
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_SUMMONER, 4);
  SPELL_ADD(CLASS_DRUID, 4);
//  SPELL_ADD(CLASS_RANGER, 7);
  SPELL_ADD(CLASS_ETHERMANCER, 4);
  SPELL_ADD(CLASS_BARD, 6);
  SPEC_SPELL_ADD(CLASS_REAVER, 7, SPEC_SHOCK_REAVER);

  SPELL_CREATE_MSG("cone of cold", SPELL_CONE_OF_COLD, PULSE_SPELLCAST * 4/3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO, spell_cone_of_cold,
                "You feel less &+Bcold&n.");
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_NECROMANCER, 6);
//  SPELL_ADD(CLASS_CABALIST, 6);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);
  SPELL_ADD(CLASS_ETHERMANCER, 4);
  SPELL_ADD(CLASS_THEURGIST, 6);


  SPELL_CREATE("enervation", SPELL_ENERVATION, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_enervation);
  SPELL_ADD(CLASS_NECROMANCER, 6);
  SPELL_ADD(CLASS_ANTIPALADIN, 8);

  SPELL_CREATE("restore spirit", SPELL_RESTORE_SPIRIT, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_restore_spirit);
  SPELL_ADD(CLASS_THEURGIST, 6);

  SPELL_CREATE("energy drain", SPELL_ENERGY_DRAIN, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_energy_drain);
  SPELL_ADD(CLASS_NECROMANCER, 9);
  SPELL_ADD(CLASS_ANTIPALADIN, 11);
  SPELL_ADD(CLASS_THEURGIST, 9);

  SPELL_CREATE("life leech", SPELL_LIFE_LEECH, PULSE_SPELLCAST * 4/ 3,
		  TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_life_leech);
  SPELL_ADD(CLASS_NECROMANCER, 4);
  SPELL_ADD(CLASS_BLIGHTER, 5);

  SPELL_CREATE_MSG("wither", SPELL_WITHER, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_wither, "You feel less &+Lwithered&n.");
  SPELL_ADD(CLASS_ANTIPALADIN, 8);
  SPELL_ADD(CLASS_NECROMANCER, 4);
  SPEC_SPELL_ADD(CLASS_CLERIC, 6, SPEC_ZEALOT);
  SPELL_ADD(CLASS_THEURGIST, 4);
  SPELL_ADD(CLASS_BLIGHTER, 4);

  SPELL_CREATE("living stone", SPELL_LIVING_STONE, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO, spell_living_stone);

  SPELL_CREATE("greater living stone", SPELL_GREATER_LIVING_STONE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO,
                spell_greater_living_stone);

  SPELL_CREATE("fireball", SPELL_FIREBALL, PULSE_SPELLCAST * 14/9,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO, spell_fireball);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);
  SPELL_ADD(CLASS_BARD, 7);
  SPEC_SPELL_ADD(CLASS_REAVER, 8, SPEC_FLAME_REAVER);

  SPELL_CREATE("meteorswarm", SPELL_METEOR_SWARM, PULSE_SPELLCAST * 3,
                TAR_AREA + TAR_OFFAREA | TAR_AGGRO, spell_meteorswarm);
  SPELL_ADD(CLASS_SORCERER, 10);


  SPELL_CREATE("entropy storm", SPELL_ENTROPY_STORM, PULSE_SPELLCAST * 2,
                TAR_OFFAREA | TAR_AGGRO, spell_entropy_storm);
  SPELL_ADD(CLASS_WARLOCK, 11);

  SPELL_CREATE("chain lightning", SPELL_CHAIN_LIGHTNING, PULSE_SPELLCAST * 2,
                TAR_AREA + TAR_OFFAREA /*| TAR_CHAR_RANGE */  | TAR_AGGRO,
                spell_chain_lightning);
  SPELL_ADD(CLASS_SORCERER, 10);

  SPELL_CREATE("lightning ring", SPELL_RING_LIGHTNING, (int) (PULSE_SPELLCAST * 2.5),
                TAR_AREA + TAR_OFFAREA /*| TAR_CHAR_RANGE */  | TAR_AGGRO, spell_ring_lightning);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_FROSTMAGUS);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_TEMPESTMAGUS);
  SPEC_SPELL_ADD(CLASS_REAVER, 10, SPEC_SHOCK_REAVER);

  SPELL_CREATE("bigbys clenched fist", SPELL_BIGBYS_CLENCHED_FIST, PULSE_SPELLCAST * 5/3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_bigbys_clenched_fist);
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_BARD, 8);
  SPELL_ADD(CLASS_REAVER, 11);

  SPELL_CREATE("solbeeps missile barrage", SPELL_MISSILE_BARRAGE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_solbeeps_missile_barrage);
	SPELL_ADD(CLASS_SORCERER, 11);
       SPEC_SPELL_ADD(CLASS_SORCERER, 0, SPEC_WIZARD);

  SPELL_CREATE("anti-magic ray", SPELL_ANTI_MAGIC_RAY, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_anti_magic_ray);
  SPEC_SPELL_ADD(CLASS_SORCERER, 11, SPEC_WIZARD);

  SPELL_CREATE("mordenkainens lucubration", SPELL_MORDENKAINENS_LUCUBRATION, PULSE_SPELLCAST * 4,
               TAR_SELF_ONLY, spell_mordenkainens_lucubration);
  SPEC_SPELL_ADD(CLASS_SORCERER, 12, SPEC_WIZARD);

  SPELL_CREATE("bigbys crushing hand", SPELL_BIGBYS_CRUSHING_HAND, PULSE_SPELLCAST * 19/9,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
               spell_bigbys_crushing_hand);
  SPELL_ADD(CLASS_SORCERER, 10);

  SPELL_CREATE("dimension door", SPELL_DIMENSION_DOOR, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT, spell_dimension_door);
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);
  SPELL_ADD(CLASS_NECROMANCER, 9);
  SPELL_ADD(CLASS_BARD, 7);
  SPELL_ADD(CLASS_THEURGIST, 9);
  SPELL_ADD(CLASS_REAVER, 11);

  SPELL_CREATE("dark compact", SPELL_DARK_COMPACT, PULSE_SPELLCAST,
                TAR_SELF_ONLY, spell_dark_compact);

  SPELL_CREATE("relocate", SPELL_RELOCATE, PULSE_SPELLCAST * 5,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT, spell_relocate);
  SPELL_ADD(CLASS_SORCERER, 9);
  SPELL_ADD(CLASS_CONJURER, 9);
  SPELL_ADD(CLASS_SUMMONER, 9);

  SPELL_CREATE_MSG("wizard eye", SPELL_WIZARD_EYE, PULSE_SPELLCAST,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT,
                spell_wizard_eye, "Your magical eyesight dissipates.");
  SPELL_ADD(CLASS_SORCERER, 4);

  SPELL_CREATE_MSG("clairvoyance", SPELL_CLAIRVOYANCE, PULSE_SPELLCAST * 2,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT,
                spell_clairvoyance, "Your clairvoyant vision dissipates.");
  SPELL_ADD(CLASS_SORCERER, 100);
  SPELL_ADD(CLASS_CONJURER, 100);
  SPELL_ADD(CLASS_SUMMONER, 100);
  SPELL_ADD(CLASS_BARD, 100);

  SPELL_CREATE_MSG("rejuvenate major", SPELL_REJUVENATE_MAJOR, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_rejuvenate_major, "You feel &+Lolder&n.");
  SPELL_ADD(CLASS_NECROMANCER, 6);
  SPELL_ADD(CLASS_THEURGIST, 6);

  SPELL_CREATE("age", SPELL_AGE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT, spell_age);
  SPELL_ADD(CLASS_NECROMANCER, 9);
  SPELL_ADD(CLASS_THEURGIST, 9);

  SPELL_CREATE_MSG("rejuvenate minor", SPELL_REJUVENATE_MINOR, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_rejuvenate_minor, "you feel &+Lolder&n.");
  SPELL_ADD(CLASS_NECROMANCER, 4);
  SPELL_ADD(CLASS_THEURGIST, 4);

  SPELL_CREATE_MSG("ray of enfeeblement", SPELL_RAY_OF_ENFEEBLEMENT, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_ray_of_enfeeblement, "You feel less feeble.");
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_SUMMONER, 4);
  SPELL_ADD(CLASS_BARD, 6);
  SPELL_ADD(CLASS_REAVER, 6);
  SPELL_ADD(CLASS_BLIGHTER, 4);

  SPELL_CREATE("earthquake", SPELL_EARTHQUAKE, PULSE_SPELLCAST * 2,
                TAR_OFFAREA | TAR_AGGRO, spell_earthquake);
  SPELL_ADD(CLASS_CLERIC, 3);
  SPELL_ADD(CLASS_DRUID, 5);
  SPELL_ADD(CLASS_BLIGHTER, 5);

  SPELL_CREATE("call woodland beings", SPELL_CALL_WOODLAND, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT,
                spell_call_woodland_beings);
  SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_WOODLAND);


  SPELL_CREATE("earthen maul", SPELL_EARTHEN_MAUL, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_earthen_maul);
  SPELL_ADD(CLASS_DRUID, 6);
  SPEC_SPELL_ADD(CLASS_CONJURER, 7, SPEC_EARTH);

  SPELL_CREATE("firelance", SPELL_FIRELANCE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_firelance);
  SPELL_ADD(CLASS_BLIGHTER, 6);

  SPELL_CREATE("earth spike", SPELL_GROW_SPIKES, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_grow_spike);
  SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_STORM);

  SPELL_CREATE("dispel evil", SPELL_DISPEL_EVIL, PULSE_SPELLCAST + 1,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_dispel_evil);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_PALADIN, 4);
//  SPELL_ADD(CLASS_RANGER, 5);

  SPELL_CREATE("dispel good", SPELL_DISPEL_GOOD, PULSE_SPELLCAST + 1,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_dispel_good);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_ANTIPALADIN, 4);
  SPELL_ADD(CLASS_WARLOCK, 4);

  SPELL_CREATE("call lightning", SPELL_CALL_LIGHTNING, PULSE_SPELLCAST * 2,
                TAR_AREA | TAR_OFFAREA | TAR_FIGHT_VICT  | TAR_AGGRO, cast_call_lightning);
  SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_STORM);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 7, SPEC_STORMBRINGER);
//  SPELL_ADD(CLASS_RANGER, 8);
  SPEC_SPELL_ADD(CLASS_REAVER, 9, SPEC_SHOCK_REAVER);


  SPELL_CREATE("harm", SPELL_HARM, PULSE_SPELLCAST * 4 / 3,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_harm);
  SPELL_ADD(CLASS_CLERIC, 6);
//  SPELL_ADD(CLASS_DRUID, 5);
  SPELL_ADD(CLASS_BLIGHTER, 5);
  SPELL_ADD(CLASS_ANTIPALADIN, 7);

  SPELL_CREATE("full harm", SPELL_FULL_HARM, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_full_harm);
  SPELL_ADD(CLASS_CLERIC, 10);

  SPELL_CREATE("destroy undead", SPELL_DESTROY_UNDEAD, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_destroy_undead);
  SPELL_ADD(CLASS_CLERIC, 5);
  SPELL_ADD(CLASS_PALADIN, 7);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 8, SPEC_DIABOLIS);
  SPEC_SPELL_ADD(CLASS_THEURGIST, 7, SPEC_TEMPLAR);
  SPELL_ADD(CLASS_AVENGER, 10);

  SPELL_CREATE("taint", SPELL_TAINT, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_taint);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 9, SPEC_NECROLYTE);
  SPEC_SPELL_ADD(CLASS_THEURGIST, 9, SPEC_MEDIUM);
  SPELL_ADD(CLASS_ANTIPALADIN, 11);
  SPELL_ADD(CLASS_CLERIC, 10);

  SPELL_CREATE("firestorm", SPELL_FIRESTORM, PULSE_SPELLCAST * 2,
                TAR_AREA | TAR_OFFAREA | TAR_FIGHT_VICT | TAR_AGGRO, spell_firestorm);
//  SPELL_ADD(CLASS_DRUID, 6);
	SPEC_SPELL_ADD(CLASS_DRUID, 9, SPEC_STORM);
//  SPELL_ADD(CLASS_RANGER, 10);
  SPELL_ADD(CLASS_BLIGHTER, 9);
  SPEC_SPELL_ADD(CLASS_REAVER, 10, SPEC_FLAME_REAVER);

  SPELL_CREATE("flame strike", SPELL_FLAMESTRIKE, PULSE_SPELLCAST + 1,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO, spell_flamestrike);
  SPELL_ADD(CLASS_CLERIC, 6);
  SPELL_ADD(CLASS_BLIGHTER, 6);

  SPELL_CREATE("teleport", SPELL_TELEPORT, PULSE_SPELLCAST,
                TAR_SELF_ONLY, spell_teleport);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_CONJURER, 6);
  SPELL_ADD(CLASS_SUMMONER, 6);
  SPELL_ADD(CLASS_MINDFLAYER, 4);
  /*SPELL_ADD(CLASS_REAVER, 9); */
  SPELL_ADD(CLASS_THEURGIST, 5);
  
  SPELL_CREATE("depart", SPELL_DEPART, PULSE_SPELLCAST,
                TAR_SELF_ONLY, spell_depart);
  //SPELL_ADD(CLASS_PSIONICIST, 7);
  
  SPELL_CREATE_MSG("mielikki vitality", SPELL_MIELIKKI_VITALITY, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_mielikki_vitality, "&+GMielikki's blessing fades.&n");
  SPELL_ADD(CLASS_DRUID, 7);
  //SPELL_ADD(CLASS_RANGER, 8);

  SPELL_CREATE_MSG("faluzures vitality", SPELL_FALUZURES_VITALITY, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_faluzures_vitality, "&+yFa&+Lluz&+yure&+L's blessing fades.&n");
  SPELL_ADD(CLASS_BLIGHTER, 7);
  
  SPELL_CREATE_MSG("bless", SPELL_BLESS, PULSE_SPELLCAST,
                TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_bless, "You feel less &+Wrighteous&n.");
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_PALADIN, 2);
//  SPELL_ADD(CLASS_RANGER, 3);
  SPELL_ADD(CLASS_AVENGER, 4);

  SPELL_CREATE_MSG("blindness", SPELL_BLINDNESS, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_blindness, "You feel a &+Lcloak &nof &+Lblindness &ndissolve.");
  SPELL_ADD(CLASS_CLERIC, 2);
  SPELL_ADD(CLASS_ANTIPALADIN, 6);
  SPELL_ADD(CLASS_ILLUSIONIST, 3);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE_MSG("minor globe of invulnerability", SPELL_MINOR_GLOBE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_minor_globe, "Your minor globe shimmers and fades into thin air.");
  SPELL_ADD(CLASS_NECROMANCER, 6);
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_CONJURER, 6);
  SPELL_ADD(CLASS_SUMMONER, 6);
  SPELL_ADD(CLASS_THEURGIST, 6);


  SPELL_CREATE_MSG("deflect", SPELL_DEFLECT, PULSE_SPELLCAST * 4,
                TAR_SELF_ONLY,
                spell_deflect, "Your sense your magical barrier fade.");
  SPELL_ADD(CLASS_SORCERER, 12);

  SPELL_CREATE_MSG("charm person", SPELL_CHARM_PERSON, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_SELF_NONO | TAR_AGGRO,
                spell_charm_person, "You regain your senses.");

  SPELL_CREATE_MSG("infravision", SPELL_INFRAVISION, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_infravision, "Your &+rinfravision&n dissipates.");
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_SUMMONER, 4);
  SPELL_ADD(CLASS_RANGER, 7);
  SPELL_ADD(CLASS_BARD, 6);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);

  SPELL_CREATE("harmonic resonance", SPELL_HARMONIC_RESONANCE, PULSE_SPELLCAST * 2,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO,
                spell_harmonic_resonance);
  //SPELL_ADD(CLASS_DRUID, 11);
  SPEC_SPELL_ADD(CLASS_RANGER, 10, SPEC_MARSHALL);

  SPELL_CREATE("nova", SPELL_NOVA, PULSE_SPELLCAST * 3,
                TAR_OFFAREA | TAR_AGGRO, spell_nova);
  SPELL_ADD(CLASS_DRUID, 12);

  SPELL_CREATE("sandstorm", SPELL_SANDSTORM, PULSE_SPELLCAST * 3,
                TAR_OFFAREA | TAR_AGGRO, spell_sandstorm);
  SPELL_ADD(CLASS_BLIGHTER, 12);

  SPELL_CREATE("spore burst", SPELL_SPORE_BURST, PULSE_SPELLCAST * 3,
                TAR_OFFAREA | TAR_AGGRO, spell_spore_burst);
  SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_WOODLAND);
  
  SPELL_CREATE("spore cloud", SPELL_SPORE_CLOUD, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_spore_cloud);
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_WOODLAND);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 8, SPEC_SCOURGE);


  SPELL_CREATE("control weather", SPELL_CONTROL_WEATHER, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT, cast_control_weather);
//     SPELL_ADD(CLASS_DRUID, 7);
//     SPELL_ADD(CLASS_RANGER, 8);

  SPELL_CREATE_MSG("mage flame", SPELL_MAGE_FLAME, (PULSE_SPELLCAST * 3) / 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_mage_flame, "Your mage flame slowly fades into nothingness.");
  SPELL_ADD(CLASS_CONJURER, 3);
  SPELL_ADD(CLASS_SUMMONER, 3);
  
  SPELL_CREATE_MSG("holy light", SPELL_HOLY_LIGHT, (PULSE_SPELLCAST * 3) / 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_holy_light, "Your holy light slowly fades into nothingness.");
  SPELL_ADD(CLASS_THEURGIST, 9);

  SPELL_CREATE("globe of darkness", SPELL_GLOBE_OF_DARKNESS,
                PULSE_SPELLCAST * 2, TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_globe_of_darkness);
  SPELL_ADD(CLASS_NECROMANCER, 9);

  SPELL_CREATE("flame blade", SPELL_FLAME_BLADE, PULSE_SPELLCAST * 2,
                TAR_IGNORE | TAR_NOCOMBAT, spell_flame_blade);
  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE("elemental swarm", SPELL_ELEMENTAL_SWARM, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_elemental_swarm);
  SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_STORM);

  SPELL_CREATE("serendipity", SPELL_SERENDIPITY, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT, spell_serendipity);
  SPELL_ADD(CLASS_DRUID, 4);


  SPELL_CREATE("minor creation", SPELL_MINOR_CREATION, PULSE_SPELLCAST,
                TAR_IGNORE | TAR_NOCOMBAT, cast_minor_creation);
  SPELL_ADD(CLASS_CONJURER, 1);
  SPELL_ADD(CLASS_SUMMONER, 1);
  SPELL_ADD(CLASS_SORCERER, 1);
  SPELL_ADD(CLASS_NECROMANCER, 1);
  SPELL_ADD(CLASS_BARD, 4);
  SPELL_ADD(CLASS_ILLUSIONIST, 1);
  SPELL_ADD(CLASS_THEURGIST, 1);


  SPELL_CREATE_MSG("pulchritude", SPELL_PULCHRITUDE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT | TAR_ANIMAL,
                spell_pulchritude, "&+yYour features harden.&n");
  SPELL_ADD(CLASS_DRUID, 1);
	SPEC_SPELL_ADD(CLASS_SHAMAN, 7, SPEC_ANIMALIST);

  SPELL_CREATE("create food", SPELL_CREATE_FOOD, PULSE_SPELLCAST,
                TAR_IGNORE | TAR_NOCOMBAT, spell_create_food);
//  SPELL_ADD(CLASS_DRUID, 1);
//  SPELL_ADD(CLASS_RANGER, 4);
//  SPELL_ADD(CLASS_CLERIC, 2);

  SPELL_CREATE("create water", SPELL_CREATE_WATER, PULSE_SPELLCAST,
                TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_NOCOMBAT, spell_create_water);
//  SPELL_ADD(CLASS_DRUID, 1);
//  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_RANGER, 4);

SPELL_CREATE("vigorize light", SPELL_VIGORIZE_LIGHT, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_vigorize_light);
  // SPELL_ADD(CLASS_CLERIC, 2);
  // SPELL_ADD(CLASS_DRUID, 2);
  // SPELL_ADD(CLASS_PALADIN, 3);
  // SPELL_ADD(CLASS_ANTIPALADIN, 3);
  // SPELL_ADD(CLASS_RANGER, 3);

  SPELL_CREATE("cure blind", SPELL_CURE_BLIND, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_cure_blind);
  SPELL_ADD(CLASS_CLERIC, 3);
  SPELL_ADD(CLASS_RANGER, 6);
  SPELL_ADD(CLASS_PALADIN, 4);
  SPELL_ADD(CLASS_WARLOCK, 6);

  SPELL_CREATE2("wandering woods", SPELL_WANDERING_WOODS, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                spell_wandering_woods, "The forest falls asleep again as your enchantment fades.",
                "The land around you grows a tad more familiar.");
  SPELL_ADD(CLASS_DRUID, 9);

  SPELL_CREATE_MSG("aid", SPELL_AID, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_aid, "&+gThe aid of nature leaves you.&n");
  SPELL_ADD(CLASS_DRUID, 2);

  SPELL_CREATE("devitalize", SPELL_DEVITALIZE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_devitalize);
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE("vigorize serious", SPELL_VIGORIZE_SERIOUS, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_vigorize_serious);
  // SPELL_ADD(CLASS_CLERIC, 3);
  // SPELL_ADD(CLASS_DRUID, 3);
  // SPELL_ADD(CLASS_PALADIN, 5);
  // SPELL_ADD(CLASS_ANTIPALADIN, 5);
  // SPELL_ADD(CLASS_RANGER, 5);

  SPELL_CREATE("invigorate", SPELL_INVIGORATE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_invigorate);
  SPELL_ADD(CLASS_PALADIN, 5);
  SPELL_ADD(CLASS_ANTIPALADIN, 5);
  SPELL_ADD(CLASS_RANGER, 5);
  SPELL_ADD(CLASS_CLERIC, 4);
                
  SPELL_CREATE("vigorize critic", SPELL_VIGORIZE_CRITIC, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_vigorize_critic);
//  SPELL_ADD(CLASS_CLERIC, 4);

  SPELL_CREATE_MSG("disease", SPELL_DISEASE, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_disease, "You feel much better.");
  SPELL_ADD(CLASS_DRUID, 2);
  SPEC_SPELL_ADD(CLASS_CLERIC, 3, SPEC_ZEALOT);

  SPELL_CREATE_MSG("contagion", SPELL_CONTAGION, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_contagion, "&+YYou feel much better.&n");

  SPELL_CREATE_MSG("curse", SPELL_CURSE, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_AGGRO, spell_curse,
               "Your heart feels lighter, as you feel the &+Lcurse&n lift.");
  SPELL_ADD(CLASS_CLERIC, 6);
  SPELL_ADD(CLASS_ANTIPALADIN, 6);

  SPELL_CREATE_MSG("dispel lifeforce", SPELL_DISPEL_LIFEFORCE, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_AGGRO, spell_dispel_lifeforce,
               "&+wYou feel the life surge back into you");
  SPELL_ADD(CLASS_WARLOCK, 8);

  SPELL_CREATE_MSG("alter energy polarity", SPELL_ALTER_ENERGY_POLARITY, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_AGGRO, spell_alter_energy_polarity,
               "&+WYou feel yourself shift back into balance.");
  SPELL_ADD(CLASS_WARLOCK, 9);


  SPELL_CREATE_MSG("nether touch", SPELL_NETHER_TOUCH, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_AGGRO,
                spell_nether_touch, "&+LA darkness lifts from your soul.");
  SPELL_ADD(CLASS_WARLOCK, 6);

  SPELL_CREATE_MSG("water breathing", SPELL_WATERBREATH, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_waterbreath, "Your gills retract back into your neck.");
  SPELL_ADD(CLASS_CLERIC, 7);
//  SPELL_ADD(CLASS_DRUID, 7);

  SPELL_CREATE_MSG("farsee", SPELL_FARSEE, PULSE_SPELLCAST * 5 / 2,
               TAR_SELF_ONLY | TAR_NOCOMBAT,
               spell_farsee, "You cannot see as far anymore.");
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_RANGER, 9);
  SPELL_ADD(CLASS_CONJURER, 10);
  SPELL_ADD(CLASS_SUMMONER, 10);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);

  SPELL_CREATE_MSG("fear", SPELL_FEAR, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_fear, "You feel less cowardly.");
  SPELL_ADD(CLASS_CLERIC, 6);
  SPELL_ADD(CLASS_ANTIPALADIN, 5);

  SPELL_CREATE("recharger", SPELL_RECHARGER, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_recharger);
/*  SPELL_ADD(CLASS_PSIONICIST, 2);*/

  SPELL_CREATE("cure light", SPELL_CURE_LIGHT, PULSE_SPELLCAST,
                TAR_CHAR_ROOM, spell_cure_light);
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_PALADIN, 3);
//  SPELL_ADD(CLASS_RANGER, 4);

  SPELL_CREATE("cure disease", SPELL_CURE_DISEASE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM, spell_cure_disease);
  SPELL_ADD(CLASS_CLERIC, 4);
  SPELL_ADD(CLASS_PALADIN, 8);

  SPELL_CREATE("cause light", SPELL_CAUSE_LIGHT, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_cause_light);
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_ANTIPALADIN, 3);
  SPELL_ADD(CLASS_BLIGHTER, 1);

  SPELL_CREATE("life bolt", SPELL_LIFE_BOLT, PULSE_SPELLCAST * 3/2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO, spell_life_bolt);
//  SPELL_ADD(CLASS_CLERIC, 1);
  SPELL_ADD(CLASS_THEURGIST, 1);

  SPELL_CREATE("cure serious", SPELL_CURE_SERIOUS, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM, spell_cure_serious);
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_PALADIN, 5);

  SPELL_CREATE("unholy wind", SPELL_UNHOLY_WIND, PULSE_SPELLCAST * 5 / 4,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_unholy_wind);
  SPELL_ADD(CLASS_WARLOCK, 1);
  SPELL_ADD(CLASS_BLIGHTER, 2);

  SPELL_CREATE("invoke negative energy", SPELL_INVOKE_NEG_ENERGY, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_invoke_negative_energy);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE("purge living", SPELL_PURGE_LIVING, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_purge_living);
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE("channel negative energy", SPELL_CHANNEL_NEG_ENERGY, PULSE_SPELLCAST * 5 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_channel_negative_energy);
  SPELL_ADD(CLASS_WARLOCK, 10);

  SPELL_CREATE("cause serious", SPELL_CAUSE_SERIOUS, PULSE_SPELLCAST * 5 / 4,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_cause_serious);
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_ANTIPALADIN, 5);
  SPELL_ADD( CLASS_BLIGHTER, 2 );

  SPELL_CREATE("cure critic", SPELL_CURE_CRITIC, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_cure_critic);
  SPELL_ADD(CLASS_CLERIC, 3);
  SPELL_ADD(CLASS_PALADIN, 6);

  SPELL_CREATE("cause critical", SPELL_CAUSE_CRITICAL, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_cause_critical);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_ANTIPALADIN, 6);
  SPELL_ADD(CLASS_BLIGHTER, 3);

  SPELL_CREATE("heal", SPELL_HEAL, PULSE_SPELLCAST * 3 / 2, TAR_CHAR_ROOM, spell_heal);
//  SPELL_ADD(CLASS_DRUID, 6);
  SPELL_ADD(CLASS_CLERIC, 5);
  SPELL_ADD(CLASS_PALADIN, 8);

  SPELL_CREATE("natures touch", SPELL_NATURES_TOUCH, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM, spell_natures_touch);
  SPELL_ADD(CLASS_DRUID, 5);
//  SPELL_ADD(CLASS_RANGER, 7);

  SPELL_CREATE("drain nature", SPELL_DRAIN_NATURE, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM, spell_drain_nature);
  SPELL_ADD(CLASS_BLIGHTER, 5);

  SPELL_CREATE("sticks to snakes", SPELL_STICKS_TO_SNAKES, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_sticks_to_snakes);
  SPELL_ADD(CLASS_DRUID, 2);

  SPELL_CREATE("full heal", SPELL_FULL_HEAL, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_full_heal);
  SPELL_ADD(CLASS_CLERIC, 7);

  SPELL_CREATE_MSG("vitalize undead", SPELL_VITALIZE_UNDEAD, PULSE_SPELLCAST * 7 / 3,
                TAR_CHAR_ROOM,
                spell_vitalize_undead, "Your enhanced strength drains away.");
  SPELL_ADD(CLASS_NECROMANCER, 6);

  SPELL_CREATE_MSG("vitalize soul", SPELL_VITALIZE_SOUL, PULSE_SPELLCAST * 7 / 3,
                TAR_CHAR_ROOM,
		spell_vitalize_undead, "Your enhanced spirit drains away.");
  SPELL_ADD(CLASS_THEURGIST, 6);

  SPELL_CREATE_MSG("vitality", SPELL_VITALITY, PULSE_SPELLCAST * 7 / 3,
                TAR_CHAR_ROOM,
                spell_vitality, "You feel less vitalized.");
  SPELL_ADD(CLASS_CLERIC, 5);

  SPELL_CREATE_MSG("miracle", SPELL_MIRACLE, PULSE_SPELLCAST * 7 / 3,
                TAR_IGNORE,
                spell_miracle, "You feel less vitalized.");
  SPELL_ADD(CLASS_CLERIC, 12);

  SPELL_CREATE("channel", SPELL_CHANNEL, PULSE_SPELLCAST * 3, TAR_IGNORE, cast_channel);

/* Divine fury currently does nothing due to poor coding */
  SPELL_CREATE_MSG("divine fury", SPELL_DIVINE_FURY, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_divine_fury, "&+RYo&+ru fe&+Rel le&+rss di&+Rvine&+rly fu&+Rrious.&n");
/*  SPELL_ADD(CLASS_CLERIC, 9); */

  SPELL_CREATE_MSG("detect invisibility", SPELL_DETECT_INVISIBLE, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_detect_invisibility, "Unseen things vanish from your sight.");
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_CONJURER, 8);
  SPELL_ADD(CLASS_SUMMONER, 8);
  SPELL_ADD(CLASS_BARD, 10);
  SPELL_ADD(CLASS_REAVER, 9);
  SPELL_ADD(CLASS_THEURGIST, 8);

  SPELL_CREATE_MSG("detect evil", SPELL_DETECT_EVIL, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_detect_evil, "You feel the &+rcrimson &nin your vision fade.");
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_RANGER, 3);

  SPELL_CREATE_MSG("detect good", SPELL_DETECT_GOOD, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_detect_good, "You feel the &+Ygold &nin your vision fade.");
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_RANGER, 3);

  SPELL_CREATE_MSG("detect magic", SPELL_DETECT_MAGIC, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_detect_magic, "Magical auras fade from your sight.");
  SPELL_ADD(CLASS_CLERIC, 1);
  SPELL_ADD(CLASS_SORCERER, 1);
  SPELL_ADD(CLASS_NECROMANCER, 1);
//  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_CONJURER, 1);
  SPELL_ADD(CLASS_SUMMONER, 1);
  SPELL_ADD(CLASS_RANGER, 3);
  SPELL_ADD(CLASS_REAVER, 3);
  SPELL_ADD(CLASS_ILLUSIONIST, 1);
  SPELL_ADD(CLASS_ETHERMANCER, 2);
  SPELL_ADD(CLASS_THEURGIST, 1);
  SPELL_ADD(CLASS_BLIGHTER, 1);

  SPELL_CREATE("plane shift", SPELL_PLANE_SHIFT, PULSE_SPELLCAST * 2,
                TAR_IGNORE | TAR_NOCOMBAT, cast_plane_shift);
  SPELL_ADD(CLASS_CLERIC, 9);
  SPELL_ADD(CLASS_WARLOCK, 9);
  SPELL_ADD(CLASS_ETHERMANCER, 9);
  //  SPELL_ADD(CLASS_DRUID, 9);

  SPELL_CREATE("gate", SPELL_GATE, PULSE_SPELLCAST * 3, TAR_IGNORE | TAR_NOCOMBAT, cast_gate);
  SPELL_ADD(CLASS_CONJURER, 9);
  SPELL_ADD(CLASS_SUMMONER, 9);
  SPELL_ADD(CLASS_NECROMANCER, 10);

  SPELL_CREATE("nether gate", SPELL_NETHER_GATE, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT, cast_nether_gate);
  SPELL_ADD(CLASS_WARLOCK, 10);

  SPELL_CREATE_MSG("sense holiness", SPELL_SENSE_HOLINESS, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_sense_holiness, "&+wYou can no longer sense holiness.");
  SPELL_ADD(CLASS_WARLOCK, 1);

  SPELL_CREATE_MSG("negative energy barrier", SPELL_NEG_ENERGY_BARRIER, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_negative_energy_barrier, "&+LYour energy barrier fades.&n");
  SPELL_ADD(CLASS_WARLOCK, 6);
  SPELL_ADD(CLASS_CLERIC, 6);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 6, SPEC_STARMAGUS);

  SPELL_CREATE("lesser resurrect", SPELL_LESSER_RESURRECT, PULSE_SPELLCAST * 5,
                TAR_OBJ_ROOM | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_lesser_resurrect);
  SPELL_ADD(CLASS_CLERIC, 7);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 9, SPEC_SPIRITUALIST);

  SPELL_CREATE("resurrect", SPELL_RESURRECT, PULSE_SPELLCAST * 10,
                TAR_OBJ_ROOM | TAR_NOCOMBAT | TAR_SPIRIT, spell_resurrect);
  SPELL_ADD(CLASS_CLERIC, 9);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_SPIRITUALIST);
  
  SPELL_CREATE("hand of death", SPELL_HANDOFDEATH, PULSE_SPELLCAST * 10,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_resurrect);
  SPELL_ADD(CLASS_WARLOCK, 10);


  SPELL_CREATE_MSG("death blessing", SPELL_DEATH_BLESSING, PULSE_SPELLCAST * 7 / 3,
                TAR_IGNORE,
                spell_death_blessing, "You feel less vitalized.");
  SPELL_ADD(CLASS_WARLOCK, 12);

  SPELL_CREATE("preserve", SPELL_PRESERVE, PULSE_SPELLCAST * 2,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_preserve);
  SPELL_ADD(CLASS_CLERIC, 2);
  SPELL_ADD(CLASS_NECROMANCER, 1);
  SPELL_ADD(CLASS_THEURGIST, 1);
//  SPELL_ADD(CLASS_DRUID, 2);

  SPELL_CREATE("mass preserve", SPELL_MASS_PRESERVE, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT, spell_mass_preserve);
  SPELL_ADD(CLASS_CLERIC, 11);

  SPELL_CREATE_MSG("mass invisibility", SPELL_MASS_INVIS, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT,
                spell_mass_invisibility, "You slowly fade back into visibility.");

  SPELL_ADD(CLASS_SORCERER, 9);
  SPELL_ADD(CLASS_ILLUSIONIST, 9);


  SPELL_CREATE("enchant weapon", SPELL_ENCHANT_WEAPON, PULSE_SPELLCAST * 8,
                TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_NOCOMBAT, spell_enchant_weapon);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_SUMMONER, 4);

  SPELL_CREATE("knock", SPELL_KNOCK, PULSE_SPELLCAST * 3,
                TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_knock);
  SPELL_ADD(CLASS_NECROMANCER, 7);
  SPELL_ADD(CLASS_THEURGIST, 7);

  SPELL_CREATE_MSG("lifelust", SPELL_LIFELUST, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_lifelust, "&+LThe &+Wlife&n&+rlust&+L drains out of you.");
  SPELL_ADD(CLASS_WARLOCK, 4);

  SPELL_CREATE("unmaking", SPELL_UNMAKING, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_unmaking);
  SPELL_ADD(CLASS_WARLOCK, 4);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 4, SPEC_DIABOLIS);
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 4, SPEC_VIOLATOR);
/*
  SPELL_CREATE("consume corpse", SPELL_CONSUME_CORPSE, PULSE_SPELLCAST * 3,
		  TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_unmaking);
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 4, SPEC_VIOLATOR);
*/
  // Same spell, different name.
  SPELL_CREATE("return soul", SPELL_RETURN_SOUL, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_unmaking);
  SPEC_SPELL_ADD(CLASS_THEURGIST, 4, SPEC_MEDIUM);

  SPELL_CREATE("dispel invisible", SPELL_DISPEL_INVISIBLE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP | TAR_NOCOMBAT,
                spell_dispel_invisible);
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_ILLUSIONIST, 7);
  SPELL_ADD(CLASS_THEURGIST, 8);

  SPELL_CREATE_MSG("improved invisibility", SPELL_INVIS_MAJOR, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP | TAR_NOCOMBAT, spell_improved_invisibility,
               "You slowly fade back into visibility.");
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_CONJURER, 8);
  SPELL_ADD(CLASS_SUMMONER, 8);
  SPELL_ADD(CLASS_THEURGIST, 8);

  SPELL_CREATE_MSG("concealment", SPELL_INVISIBLE, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP | TAR_NOCOMBAT,
                spell_invisibility, "You slowly fade back into visibility.");
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 3);
  SPELL_ADD(CLASS_CONJURER, 3);
  SPELL_ADD(CLASS_SUMMONER, 3);
//  SPELL_ADD(CLASS_RANGER, 6);
  SPELL_ADD(CLASS_BARD, 5);
  SPELL_ADD(CLASS_ILLUSIONIST, 3);
  SPELL_ADD(CLASS_THEURGIST, 5);

  SPELL_CREATE_MSG("poison", SPELL_POISON, PULSE_SPELLCAST * 2,
               TAR_CHAR_ROOM | TAR_SELF_NONO | TAR_OBJ_INV | TAR_OBJ_EQUIP |
               TAR_FIGHT_VICT | TAR_AGGRO, spell_poison,
               "You feel the &+gpoison &nin your &+Rblood &ndissipate.");
  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_BLIGHTER, 4);
  SPEC_SPELL_ADD(CLASS_CLERIC, 4, SPEC_ZEALOT);

  SPELL_CREATE_MSG("protection from evil", SPELL_PROTECT_FROM_EVIL, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_protection_from_evil, "You do not feel as safe from &+revil&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_PALADIN, 3);
  SPELL_ADD(CLASS_RANGER, 4);
  SPELL_ADD(CLASS_AVENGER, 5);

  SPELL_CREATE_MSG("protection from good", SPELL_PROTECT_FROM_GOOD, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_protection_from_good, "You do not feel as safe from &+Ygood&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_ANTIPALADIN, 3);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE("slashing darkness", SPELL_SLASHING_DARKNESS, PULSE_SPELLCAST * 2/3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO, spell_slashing_darkness);
  SPELL_ADD(CLASS_NECROMANCER, 1);
//  SPELL_ADD(CLASS_CABALIST, 1);

  SPELL_CREATE("raise spectre", SPELL_RAISE_SPECTRE, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_raise_spectre);
  SPELL_ADD(CLASS_NECROMANCER, 5);

  SPELL_CREATE("raise wraith", SPELL_RAISE_WRAITH, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_raise_wraith);
  SPELL_ADD(CLASS_NECROMANCER, 7);

  SPELL_CREATE("raise vampire", SPELL_RAISE_VAMPIRE, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_raise_vampire);
  SPELL_ADD(CLASS_NECROMANCER, 9);

  SPELL_CREATE("raise lich", SPELL_RAISE_LICH, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_raise_lich);
  SPELL_ADD(CLASS_NECROMANCER, 10);

  SPELL_CREATE("compact corpse", SPELL_COMPACT_CORPSE, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_compact_corpse);
  SPELL_ADD(CLASS_NECROMANCER, 10);

  SPELL_CREATE("call asura", SPELL_CALL_ASURA, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_call_asura);
  SPELL_ADD(CLASS_THEURGIST, 5);

  SPELL_CREATE("call bralani", SPELL_CALL_BRALANI, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_call_bralani);
  SPELL_ADD(CLASS_THEURGIST, 7);

  SPELL_CREATE("call knight", SPELL_CALL_KNIGHT, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_call_knight);
  SPELL_ADD(CLASS_THEURGIST, 9);

  SPELL_CREATE("call liberator", SPELL_CALL_LIBERATOR, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_call_liberator);
  SPELL_ADD(CLASS_THEURGIST, 10);

  SPELL_CREATE("animate dead", SPELL_ANIMATE_DEAD, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_animate_dead);
  SPELL_ADD(CLASS_NECROMANCER, 3);
  SPELL_ADD(CLASS_BLIGHTER, 5);

  SPELL_CREATE("call archon", SPELL_CALL_ARCHON, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_call_archon);
  SPELL_ADD(CLASS_THEURGIST, 3);

  SPELL_CREATE("cyclone", SPELL_CYCLONE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_cyclone);
  SPELL_ADD(CLASS_DRUID, 7);
  //SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_STORM);
  SPEC_SPELL_ADD(CLASS_CONJURER, 7, SPEC_AIR);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_FROSTMAGUS);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_TEMPESTMAGUS);

  SPELL_CREATE("toxic fog", SPELL_TOXIC_FOG, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_toxic_fog);
  SPELL_ADD(CLASS_BLIGHTER, 7);

  SPELL_CREATE("sirens song", SPELL_SIREN_SONG, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_CHAR_RANGE | TAR_AGGRO, spell_siren_song);
  SPELL_ADD(CLASS_BARD, 12);

  SPELL_CREATE("vortex of fear", SPELL_VORTEX_OF_FEAR, PULSE_SPELLCAST * 3,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO,
                spell_vortex_of_fear);
  SPELL_ADD(CLASS_ANTIPALADIN, 10);

  SPELL_CREATE_MSG("righteous aura", SPELL_RIGHTEOUS_AURA, PULSE_SPELLCAST * 3 / 2,
                   TAR_SELF_ONLY,
                   spell_righteous_aura, "&+WThe power of your god leaves your body, and you feel weaker.&n");
  SPELL_ADD(CLASS_PALADIN, 12);

  SPELL_CREATE_MSG("bleak foeman", SPELL_BLEAK_FOEMAN, PULSE_SPELLCAST * 3 / 2,
                   TAR_SELF_ONLY,
                   spell_bleak_foeman, "&+LAs the evil exits your body, you suddenly feel less compelled to strike down your enemies.");
  SPELL_ADD(CLASS_ANTIPALADIN, 12);

  SPELL_CREATE("remove curse", SPELL_REMOVE_CURSE, PULSE_SPELLCAST * 4,
                TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_remove_curse);
  SPELL_ADD(CLASS_PALADIN, 6);
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 5);
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE_MSG("ironwood", SPELL_IRONWOOD, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM,
                spell_ironwood, "You feel your skin once again take on a more barklike quality.");
  SPEC_SPELL_ADD(CLASS_RANGER, 11, SPEC_MARSHALL);
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_WOODLAND);

  SPELL_CREATE_MSG("stone skin", SPELL_STONE_SKIN, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM,
                spell_stone_skin, "You feel your skin soften and return to normal.");
  SPELL_ADD(CLASS_CONJURER, 6);
  SPELL_ADD(CLASS_SUMMONER, 6);

  SPELL_CREATE_MSG("group stone skin", SPELL_GROUP_STONE_SKIN, PULSE_SPELLCAST * 5,
                TAR_IGNORE,
                spell_group_stone_skin, "You fell your skin soften and return to normal.");
  SPELL_ADD(CLASS_CONJURER, 10);
  SPELL_ADD(CLASS_SUMMONER, 10);

  SPELL_CREATE_MSG("sleep", SPELL_SLEEP, PULSE_SPELLCAST * 2,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
               spell_sleep, "You awake from your magical slumber.");
  SPELL_ADD(CLASS_NECROMANCER, 2);
  SPELL_ADD(CLASS_SORCERER, 2);
  SPELL_ADD(CLASS_CONJURER, 2);
  SPELL_ADD(CLASS_SUMMONER, 2);
  SPELL_ADD(CLASS_WARLOCK, 1);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);
  SPELL_ADD(CLASS_REAVER, 4);
  SPELL_ADD(CLASS_THEURGIST, 2);

  SPELL_CREATE("dispel magic", SPELL_DISPEL_MAGIC, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_WALL | TAR_AGGRO, spell_dispel_magic);
  SPELL_ADD(CLASS_CONJURER, 3);
  SPELL_ADD(CLASS_SUMMONER, 3);
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_NECROMANCER, 3);
  SPELL_ADD(CLASS_CLERIC, 8);
  SPELL_ADD(CLASS_BARD, 6);
  SPELL_ADD(CLASS_DRUID, 7);
  SPELL_ADD(CLASS_BLIGHTER, 7);
  SPELL_ADD(CLASS_REAVER, 8);
  SPELL_ADD(CLASS_ILLUSIONIST, 2);
  SPELL_ADD(CLASS_WARLOCK, 8);
  SPELL_ADD(CLASS_ETHERMANCER, 7);
  SPELL_ADD(CLASS_THEURGIST, 3);

  SPELL_CREATE("tranquility", SPELL_TRANQUILITY, PULSE_SPELLCAST * 3,
                TAR_IGNORE, spell_tranquility);
  SPELL_ADD(CLASS_DRUID, 5);

  SPELL_CREATE_MSG("strength", SPELL_STRENGTH, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_strength, "You feel less &+Cstrong&n.");
  SPELL_ADD(CLASS_NECROMANCER, 4);
  SPELL_ADD(CLASS_SORCERER, 2);
  SPELL_ADD(CLASS_CONJURER, 3);
  SPELL_ADD(CLASS_SUMMONER, 3);
  SPELL_ADD(CLASS_THEURGIST, 4);

  SPELL_CREATE_MSG("dexterity", SPELL_DEXTERITY, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_dexterity, "You feel less &+Ldexterous&n.");
//  SPELL_ADD(CLASS_SORCERER, 3);
//  SPELL_ADD(CLASS_CONJURER, 3);
//  SPELL_ADD(CLASS_SUMMONER, 3);

  SPELL_CREATE_MSG("agility", SPELL_AGILITY, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_agility, "You feel less &+cagile&n.");
  SPELL_ADD(CLASS_SORCERER, 3);
  SPELL_ADD(CLASS_CONJURER, 3);
  SPELL_ADD(CLASS_SUMMONER, 3);

  SPELL_CREATE("summon", SPELL_SUMMON, PULSE_SPELLCAST * 2,
                TAR_CHAR_WORLD | TAR_NOCOMBAT, spell_summon);
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 4);

 // SPELL_CREATE("ventriloquate", SPELL_VENTRILOQUATE, 0,
   //             TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_SELF_NONO | TAR_NOCOMBAT, spell_ventriloquate);
//  SPELL_ADD(CLASS_SORCERER, 1);
//  SPELL_ADD(CLASS_CONJURER, 1);
//  SPELL_ADD(CLASS_SUMMONER, 1);
//  SPELL_ADD(CLASS_BARD, 3);

  SPELL_CREATE_MSG("dazzle", SPELL_DAZZLE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_dazzle, "You feel the energy seep out of you.");
  SPELL_ADD(CLASS_RANGER, 10);

  SPELL_CREATE_MSG("blur", SPELL_BLUR, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_blur, "&+BYour motions slow to normal.");
  SPELL_ADD(CLASS_RANGER, 11);
  SPELL_ADD(CLASS_ETHERMANCER, 11);

  SPELL_CREATE_MSG("pass without trace", SPELL_PASS_WITHOUT_TRACE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_pass_without_trace, "&+gThe forest closes in around you.&n");
  SPELL_ADD(CLASS_DRUID, 8);
  //SPEC_SPELL_ADD(CLASS_RANGER, 10, SPEC_HUNTSMAN);


  SPELL_CREATE_MSG("sanctuary", SPELL_SANCTUARY, PULSE_SPELLCAST,
                TAR_SELF_ONLY,
                spell_sanctuary, "&+WYour glowing sanctuary &n&+wfades.");
  SPELL_ADD(CLASS_PALADIN, 10);

  SPELL_CREATE_MSG("hellfire", SPELL_HELLFIRE, PULSE_SPELLCAST, TAR_SELF_ONLY,
                spell_hellfire, "&+RYour burning hellfire &n&+rfades.");
//  SPELL_ADD(CLASS_ANTIPALADIN, 10);
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 10, SPEC_DARKKNIGHT);
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 10, SPEC_DEMONIC);
//  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 10, SPEC_VIOLATOR);

  SPELL_CREATE_MSG("stornogs metamagic shroud", SPELL_STORNOGS_LOWERED_RES, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_stornogs_lowered_magical_res,
               "&+WYou feel your innate magical resistance return.");
  SPELL_ADD(CLASS_CONJURER, 7);

  SPELL_CREATE2("stornogs shimmering starshell", SPELL_STARSHELL, (PULSE_SPELLCAST * 3) / 2,
                TAR_IGNORE,
                spell_starshell, "&+WThe &+Yblazing&N&+W shell dissipates.",
                "&+WThe &+Yblazing&N&+W shell dissipates.");
  SPELL_ADD(CLASS_CONJURER, 10);

  SPELL_CREATE("stornogs spheres", SPELL_STORNOGS_SPHERES, PULSE_SPELLCAST * 4,
                TAR_SELF_ONLY,
                spell_stornogs_spheres);
  SPELL_ADD(CLASS_CONJURER, 11);

  SPELL_CREATE_MSG("decaying flesh", SPELL_DECAYING_FLESH, PULSE_SPELLCAST * 1,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_decaying_flesh, "The &+gdecay&n finally leaves your body.");
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 3, SPEC_VIOLATOR);

  SPELL_CREATE("group stornogs spheres", SPELL_STORNOGS_GREATER_SPHERES, PULSE_SPELLCAST * 7,
                TAR_SELF_ONLY, spell_group_stornog);
  SPELL_ADD(CLASS_CONJURER, 12);

  SPELL_CREATE("cloak of fear", SPELL_CLOAK_OF_FEAR, PULSE_SPELLCAST,
                TAR_OFFAREA | TAR_AGGRO, spell_cloak_of_fear);
  SPELL_ADD(CLASS_NECROMANCER, 10);
  SPELL_ADD(CLASS_THEURGIST, 10);

  SPELL_CREATE("acidimmolate", SPELL_ACIDIMMOLATE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_acidimmolate);
 //SPELL_ADD(CLASS_SORCERER, 9);
//  SPELL_ADD(CLASS_CONJURER, 9);
//  SPELL_ADD(CLASS_SUMMONER, 9);
  SPEC_SPELL_ADD(CLASS_SUMMONER, 9, SPEC_NATURALIST);
  SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_EARTH);

  SPELL_CREATE("immolate", SPELL_IMMOLATE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_immolate);
  SPELL_ADD(CLASS_SORCERER, 8);
  SPEC_SPELL_ADD(CLASS_SUMMONER, 9, SPEC_MENTALIST);
  SPEC_SPELL_ADD(CLASS_REAVER, 10, SPEC_FLAME_REAVER);

  SPELL_CREATE("creeping doom", SPELL_CDOOM, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AREA | TAR_AGGRO, spell_cdoom);
  SPELL_ADD(CLASS_DRUID, 10);
// SPEC_SPELL_ADD(CLASS_DRUID, 10, SPEC_WOODLAND);

  SPELL_CREATE("acid rain", SPELL_ACID_RAIN, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AREA | TAR_AGGRO, spell_acid_rain);
  SPELL_ADD(CLASS_BLIGHTER, 10);

  SPELL_CREATE("prismatic cube", SPELL_PRISMATIC_CUBE, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT, cast_prismatic_cube);
SPELL_ADD(CLASS_CONJURER, 11);
SPELL_ADD(CLASS_SUMMONER, 11);
//SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_WATER);
//SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_AIR);
//SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_FIRE);
//SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_EARTH);

  SPELL_CREATE_MSG("lodestone vision", SPELL_LODESTONE, PULSE_SPELLCAST,
               TAR_SELF_ONLY | TAR_NOCOMBAT, spell_lodestone_vision,
	       "The glimmer in your eyes fades as your vision returns to normal.");

  SPELL_CREATE_MSG("haste", SPELL_HASTE, (PULSE_SPELLCAST * 3) / 2,
               TAR_CHAR_ROOM, spell_haste,
               "The world speeds up around you.");
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_RANGER, 9);
  SPELL_ADD(CLASS_REAVER, 10);
  SPEC_SPELL_ADD(CLASS_REAVER, 8, SPEC_SHOCK_REAVER);
  SPELL_ADD(CLASS_THEURGIST, 8);

  SPELL_CREATE_MSG("group haste", SPELL_GROUP_HASTE, PULSE_SPELLCAST * 4,
               TAR_IGNORE, spell_group_haste,
               "The world speeds up around you.");
  SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_AIR);


  SPELL_CREATE("word of recall", SPELL_WORD_OF_RECALL, 0,
                TAR_SELF_ONLY, spell_word_of_recall);
//  SPEC_SPELL_ADD(CLASS_CLERIC, SPEC_HEALER, 9);
//  SPEC_SPELL_ADD(CLASS_CLERIC, SPEC_HOLYMAN, 9);
//  SPEC_SPELL_ADD(CLASS_CLERIC, SPEC_ZEALOT, 9);
  SPELL_ADD(CLASS_CLERIC, 9);
//  SPELL_ADD(CLASS_WARLOCK, 9);
//  SPELL_ADD(CLASS_DRUID, 9);

  SPELL_CREATE("group recall", SPELL_GROUP_RECALL, PULSE_SPELLCAST * 6,
                TAR_IGNORE, spell_group_recall);
  SPELL_ADD(CLASS_CLERIC, 11);
 // SPELL_ADD(CLASS_WARLOCK, 11);

  SPELL_CREATE("remove poison", SPELL_REMOVE_POISON, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_remove_poison);
  SPELL_ADD(CLASS_CLERIC, 3);
  SPELL_ADD(CLASS_PALADIN, 4);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE_MSG("minor paralysis", SPELL_MINOR_PARALYSIS, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_minor_paralysis, "You can move again.");
  SPELL_ADD(CLASS_SORCERER, 4);

  SPELL_CREATE_MSG("major paralysis", SPELL_MAJOR_PARALYSIS, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_major_paralysis, "You can move again.");

  SPELL_CREATE_MSG("slowness", SPELL_SLOW, PULSE_SPELLCAST * 2,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
               spell_slow, "You feel your induced slowness dissipate.");
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_CONJURER, 5);
  SPELL_ADD(CLASS_SUMMONER, 5);
  SPELL_ADD(CLASS_BARD, 7);
  SPELL_ADD(CLASS_WARLOCK, 6);

  SPELL_CREATE("shambler", SPELL_SHAMBLER, PULSE_SPELLCAST * 4, TAR_IGNORE, spell_shambler);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 11, SPEC_SCOURGE);

  SPELL_CREATE("conjure elemental", SPELL_CONJURE_ELEMENTAL, PULSE_SPELLCAST * 4,
                TAR_IGNORE,
                spell_conjour_elemental);
  SPELL_ADD(CLASS_CONJURER, 5);

  SPELL_CREATE("mirror image", SPELL_MIRROR_IMAGE, PULSE_SPELLCAST * 2,
                TAR_IGNORE, spell_mirror_image);
  SPELL_ADD(CLASS_CONJURER, 2);
  SPELL_ADD(CLASS_SUMMONER, 2);
  SPELL_ADD(CLASS_BARD, 2);
  SPELL_ADD(CLASS_ILLUSIONIST, 3);

  SPELL_CREATE("conjure greater elemental", SPELL_CONJURE_GREATER_ELEMENTAL, PULSE_SPELLCAST * 4,
                TAR_IGNORE,
                spell_conjour_greater_elemental);
  SPELL_ADD(CLASS_CONJURER, 11);
//SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_AIR);
//SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_FIRE);
//SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_WATER);
//SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_EARTH);

  SPELL_CREATE("vitalize mana", SPELL_VITALIZE_MANA, PULSE_SPELLCAST,
                TAR_SELF_ONLY, spell_vitalize_mana);


  SPELL_CREATE_MSG("sense follower", SPELL_SENSE_FOLLOWER, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_sense_follower, "You no longer seem to sense your followers.");
  SPELL_ADD(CLASS_NECROMANCER, 3);
  SPELL_ADD(CLASS_CONJURER, 5);
  SPELL_ADD(CLASS_SUMMONER, 5);
  SPELL_ADD(CLASS_THEURGIST, 3);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 4, SPEC_TEMPESTMAGUS);

  SPELL_CREATE_MSG("sense life", SPELL_SENSE_LIFE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_sense_life, "You no longer seem to sense other lifeforms.");
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_RANGER, 6);

  SPELL_CREATE("continual light", SPELL_CONTINUAL_LIGHT, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_AREA,
                spell_continual_light);
  SPELL_ADD(CLASS_CLERIC, 6);
//  SPELL_ADD(CLASS_DRUID, 6);
  SPELL_ADD(CLASS_PALADIN, 8);
  SPELL_ADD(CLASS_ILLUSIONIST, 2);
  SPEC_SPELL_ADD(CLASS_THEURGIST, 4, SPEC_THAUMATURGE);

  SPELL_CREATE2("consecrate land", SPELL_CONSECRATE_LAND, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_SPIRIT,
                spell_consecrate_land, "&+CYour magical runes vanish in a &+Lpuff of smoke.",
                "&+CThe magical runes vanish in a &+Lpuff of smoke.");
  SPELL_ADD(CLASS_DRUID, 9);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 9, SPEC_SPIRITUALIST);

  SPELL_CREATE2("desecrate land", SPELL_DESECRATE_LAND, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_SPIRIT,
                spell_desecrate_land, "&+LYour magical &+mrunes&+L vanish in a puff of smoke.&n",
                "&+LThe magical &+mrunes&+L vanish in a puff of smoke.&n");
  SPELL_ADD(CLASS_BLIGHTER, 9 );

  SPELL_CREATE2("forbiddance", SPELL_FORBIDDANCE, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_SPIRIT,
                spell_forbiddance, "&+LYour seal in this area is broken.&n",
                "&+LThe magical seal in this area is broken.&n");
  SPELL_ADD(CLASS_BLIGHTER, 7 );

  SPELL_CREATE2("summon insects", SPELL_SUMMON_INSECTS, PULSE_SPELLCAST * 3,
                TAR_IGNORE,
                spell_summon_insects, "&+mThe insects in the area scurry away.",
                "&+mThe insects in the area scurry away..");
  SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_WOODLAND);

  SPELL_CREATE("darkness", SPELL_DARKNESS, PULSE_SPELLCAST * 3,
                TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_AREA, spell_darkness);
//  SPELL_ADD(CLASS_DRUID, 5);
  SPELL_ADD(CLASS_BLIGHTER, 3);
  SPELL_ADD(CLASS_CLERIC, 6);
  SPELL_ADD(CLASS_ANTIPALADIN, 8);
//  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 4, SPEC_REAPER);
  SPELL_ADD(CLASS_WARLOCK, 6);
  SPELL_ADD(CLASS_ILLUSIONIST, 2);

  SPELL_CREATE_MSG("protection from fire", SPELL_PROTECT_FROM_FIRE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_protection_from_fire, "You no longer feel safe from &+rfire&n.");
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE_MSG("protection from cold", SPELL_PROTECT_FROM_COLD, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_protection_from_cold, "You no longer feel safe from &+Bcold&n.");
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_NECROMANCER, 2);
  SPELL_ADD(CLASS_WARLOCK, 2);
  SPELL_ADD(CLASS_THEURGIST, 2);

  SPELL_CREATE_MSG("protection from living", SPELL_PROTECT_FROM_LIVING, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_protection_from_living, "You no longer feel safe from the &+wliving&n.");
  SPELL_ADD(CLASS_WARLOCK, 2);
  SPELL_ADD(CLASS_NECROMANCER, 2);
  SPELL_ADD(CLASS_THEURGIST, 2);

  SPELL_CREATE_MSG("protection from animals", SPELL_PROTECT_FROM_ANIMAL, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_SPIRIT,
                spell_protection_from_animals, "You no longer feel safe from &+yanimals&n.");
  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_RANGER, 4);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 5, SPEC_ANIMALIST);
  
  SPELL_CREATE("animal friendship", SPELL_ANIMAL_FRIENDSHIP, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT,
                spell_animal_friendship);
  SPELL_ADD(CLASS_DRUID, 1);

  SPELL_CREATE_MSG("protection from gas", SPELL_PROTECT_FROM_GAS, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_protection_from_gas, "You no longer feel safe from &+gpoison gas&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE_MSG("protection from acid", SPELL_PROTECT_FROM_ACID, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_protection_from_acid, "You no longer feel safe from &+Gacid&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE_MSG("protection from lightning", SPELL_PROTECT_FROM_LIGHTNING, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_protection_from_lightning, "You no longer feel safe from &+Clightning&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE_MSG("levitate", SPELL_LEVITATE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_levitate, "You feel heavier and stop levitating.");
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_SUMMONER, 4);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);
  SPELL_ADD(CLASS_THEURGIST, 5);

  SPELL_CREATE_MSG("fly", SPELL_FLY, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_fly, "You feel heavier and stop levitating.");
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_CONJURER, 9);
  SPELL_ADD(CLASS_SUMMONER, 9);
  SPELL_ADD(CLASS_NECROMANCER, 10);
  SPELL_ADD(CLASS_ILLUSIONIST, 8);
  SPELL_ADD(CLASS_THEURGIST, 10);


  SPELL_CREATE("reveal true name", SPELL_REVEAL_TRUE_NAME, PULSE_SPELLCAST * 2,
                TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP | TAR_NOCOMBAT,
                spell_reveal_true_name);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_SUMMONER, 4);
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_NECROMANCER, 7);
  SPELL_ADD(CLASS_ILLUSIONIST, 7);
  SPELL_ADD(CLASS_THEURGIST, 7);

  SPELL_CREATE("istr", SPELL_PERM_INCREASE_STR, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_str);
  SPELL_CREATE("iagi", SPELL_PERM_INCREASE_AGI, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_agi);
  SPELL_CREATE("idex", SPELL_PERM_INCREASE_DEX, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_dex);
  SPELL_CREATE("icon", SPELL_PERM_INCREASE_CON, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_con);
  SPELL_CREATE("iluck", SPELL_PERM_INCREASE_LUCK, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_luck);
  SPELL_CREATE("ipow", SPELL_PERM_INCREASE_POW, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_pow);
  SPELL_CREATE("iint", SPELL_PERM_INCREASE_INT, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_int);
  SPELL_CREATE("iwis", SPELL_PERM_INCREASE_WIS, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_wis);
  SPELL_CREATE("icha", SPELL_PERM_INCREASE_CHA, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_perm_increase_cha);

  SPELL_CREATE("identify", SPELL_IDENTIFY, PULSE_SPELLCAST * 5,
                TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_identify);
  SPELL_ADD(CLASS_CONJURER, 5);
  SPELL_ADD(CLASS_SUMMONER, 5);

  SPELL_CREATE("item lore", SPELL_LORE, PULSE_SPELLCAST * 5,
                TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_lore);

  SPELL_CREATE("prismatic spray", SPELL_PRISMATIC_SPRAY, PULSE_SPELLCAST * 2,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO,
                spell_prismatic_spray);
  SPELL_ADD(CLASS_CONJURER, 8);
  SPELL_ADD(CLASS_SUMMONER, 8);
/*  SPELL_ADD(CLASS_SORCERER, 7);*/

  SPELL_CREATE_MSG("fireshield", SPELL_FIRESHIELD, PULSE_SPELLCAST * 3 / 2,
                TAR_SELF_ONLY,
                spell_fireshield, "The &+Rburning flames &naround your body die down and fade.");
  SPELL_ADD(CLASS_CONJURER, 5);
  SPELL_ADD(CLASS_SUMMONER, 5);
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPEC_SPELL_ADD(CLASS_REAVER, 6, SPEC_FLAME_REAVER);
  SPELL_ADD(CLASS_THEURGIST, 5);

  SPELL_CREATE("color spray", SPELL_COLOR_SPRAY, PULSE_SPELLCAST * 5 / 3,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO,
                spell_color_spray);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_CONJURER, 6);
  SPELL_ADD(CLASS_SUMMONER, 6);

  SPELL_CREATE("incendiary cloud", SPELL_INCENDIARY_CLOUD, (int) (PULSE_SPELLCAST * 3),
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO,
                spell_incendiary_cloud);
  SPELL_ADD(CLASS_SORCERER, 9);

  SPELL_CREATE("ice storm", SPELL_ICE_STORM, PULSE_SPELLCAST * 2,
                TAR_AREA | TAR_OFFAREA /*| TAR_CHAR_RANGE */  | TAR_AGGRO, spell_ice_storm);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);
  SPELL_ADD(CLASS_ETHERMANCER, 5);
  SPEC_SPELL_ADD(CLASS_REAVER, 10, SPEC_ICE_REAVER);

  SPELL_CREATE("prismatic ray", SPELL_PRISMATIC_RAY, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_prismatic_ray);
  SPELL_ADD(CLASS_SORCERER, 9);
  SPELL_ADD(CLASS_SUMMONER, 9);

  SPELL_CREATE("disintegrate", SPELL_DISINTEGRATE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_disintegrate);
  SPELL_ADD(CLASS_CONJURER, 8);
//  SPELL_ADD(CLASS_REAVER, 11);

  SPELL_CREATE("acid stream", SPELL_ACID_STREAM, PULSE_SPELLCAST * 5 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_acid_stream);
  //SPELL_ADD(CLASS_DRUID, 8);
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_STORM);
  SPELL_ADD(CLASS_BLIGHTER, 8 );

  SPELL_CREATE("gaseous cloud", SPELL_GASEOUS_CLOUD, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
                spell_gaseous_cloud);
  SPELL_ADD(CLASS_SHAMAN, 10);
  
  SPELL_CREATE_MSG("guardian spirits", SPELL_GUARDIAN_SPIRITS, PULSE_SPELLCAST * 20,
                TAR_SELF_ONLY | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_guardian_spirits, "&+RThe spirits of your forefathers are no longer watching over you.&n");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_SPIRITUALIST);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_ANIMALIST);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 12, SPEC_ELEMENTALIST);
  
  SPELL_CREATE("acid blast", SPELL_ACID_BLAST, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_acid_blast);
  SPELL_ADD(CLASS_SORCERER, 3);
  SPELL_ADD(CLASS_BLIGHTER, 3);

  SPELL_CREATE_MSG("faerie fire", SPELL_FAERIE_FIRE, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_faerie_fire, "The outline of &+Mpurple flames &naround your body fades.");
  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_BLIGHTER, 3);
  SPELL_ADD(CLASS_ETHERMANCER, 1);

  SPELL_CREATE("faerie fog", SPELL_FAERIE_FOG, PULSE_SPELLCAST * 2,
                TAR_IGNORE | TAR_NOCOMBAT, spell_faerie_fog);
  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_BLIGHTER, 3);
//  SPELL_ADD(CLASS_RANGER, 6);
  SPELL_ADD(CLASS_ETHERMANCER, 3);

  SPELL_CREATE("power word kill", SPELL_PWORD_KILL, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_pword_kill);
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_NECROMANCER, 11);
//  SPELL_ADD(CLASS_CABALIST, 11);
  SPELL_ADD(CLASS_THEURGIST, 11);

  SPELL_CREATE("power word blind", SPELL_PWORD_BLIND, PULSE_SPELLCAST/2,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_INSTACAST | TAR_AGGRO,
               spell_pword_blind);
  SPELL_ADD(CLASS_SORCERER, 7);

  SPELL_CREATE_MSG("power word stun", SPELL_PWORD_STUN, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_pword_stun, "The world stops spinning.");
  SPELL_ADD(CLASS_SORCERER, 7);

  SPELL_CREATE("unholy word", SPELL_UNHOLY_WORD, PULSE_SPELLCAST/2,
                TAR_AREA | TAR_INSTACAST | TAR_AGGRO, spell_unholy_word);
  SPELL_ADD(CLASS_CLERIC, 9);
  SPELL_ADD(CLASS_ANTIPALADIN, 9);
//  SPELL_ADD(CLASS_NECROMANCER, 11); Replaced by summon ghasts Sep08 - Lucrot

  SPELL_CREATE("voice of creation", SPELL_VOICE_OF_CREATION, PULSE_SPELLCAST/2,
                TAR_AREA | TAR_OFFAREA | TAR_INSTACAST | TAR_AGGRO, spell_voice_of_creation);
  SPELL_ADD(CLASS_THEURGIST, 9);
  
  SPELL_CREATE("holy word", SPELL_HOLY_WORD, PULSE_SPELLCAST/2,
                TAR_AREA | TAR_OFFAREA | TAR_INSTACAST | TAR_AGGRO, spell_holy_word);
  SPELL_ADD(CLASS_CLERIC, 9);
  SPELL_ADD(CLASS_PALADIN, 9);

  SPELL_CREATE("sunray", SPELL_SUNRAY, PULSE_SPELLCAST * 2,
                TAR_FIGHT_VICT | TAR_CHAR_ROOM | TAR_AGGRO, spell_sunray);
  SPELL_ADD(CLASS_DRUID, 9);

  SPELL_CREATE("horrid wilting", SPELL_HORRID_WILTING, PULSE_SPELLCAST * 2,
                TAR_FIGHT_VICT | TAR_CHAR_ROOM | TAR_AGGRO, spell_horrid_wilting);
  SPELL_ADD(CLASS_BLIGHTER, 9);

  SPELL_CREATE_MSG("feeblemind", SPELL_FEEBLEMIND, PULSE_SPELLCAST * 2,
                TAR_FIGHT_VICT | TAR_CHAR_ROOM | TAR_AGGRO,
                spell_feeblemind, "You feel your &+Rbludgeoned &nintellect recover.");
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_CONJURER, 6);
  SPELL_ADD(CLASS_SUMMONER, 6);
  SPELL_ADD(CLASS_REAVER, 9);
  SPELL_ADD(CLASS_BARD, 9);
  SPELL_ADD(CLASS_THEURGIST, 5);

/* Shitcanned Healing Blade and Windstrom Blessing on 11/07 -Zion */
  //SPELL_CREATE_MSG("healing blade", SPELL_HEALING_BLADE, PULSE_SPELLCAST * 4 / 3,
    //            TAR_SELF_ONLY | TAR_NOCOMBAT,
      //          spell_healing_blade, "&+BYour blade's healing aura dims then vanishes.&n");
//   SPELL_ADD(CLASS_RANGER, 11);

/*
  SPELL_CREATE_MSG("windstrom blessing", SPELL_WINDSTROM_BLESSING, PULSE_SPELLCAST * 4 / 3,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_windstrom_blessing, "&+BYour blade's aura dims then vanishes.&n");
*/
  SPELL_CREATE_MSG("silence", SPELL_SILENCE, PULSE_SPELLCAST * 2,
                TAR_FIGHT_VICT | TAR_CHAR_ROOM | TAR_AGGRO,
                spell_silence, "You are able to speak again.");
  SPELL_ADD(CLASS_CLERIC, 9);
//  SPELL_ADD(CLASS_DRUID, 6);

  SPELL_CREATE("turn undead", SPELL_TURN_UNDEAD, PULSE_SPELLCAST,
                TAR_IGNORE, spell_turn_undead);
  SPELL_ADD(CLASS_CLERIC, 1);
  SPELL_ADD(CLASS_PALADIN, 5);
  SPELL_ADD(CLASS_AVENGER, 8);

  SPELL_CREATE("command undead", SPELL_COMMAND_UNDEAD, 0,
                TAR_FIGHT_VICT | TAR_CHAR_ROOM, spell_command_undead);
  // SPELL_ADD(CLASS_NECROMANCER, 5);
  // SPELL_ADD(CLASS_ANTIPALADIN, 10);

  SPELL_CREATE("web", SPELL_WEB, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT, cast_web);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);

  SPELL_CREATE("wall of flames", SPELL_WALL_OF_FLAMES, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT, spell_wall_of_flames);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);
  SPELL_ADD(CLASS_BLIGHTER, 7);

  SPELL_CREATE("wall of ice", SPELL_WALL_OF_ICE, (PULSE_SPELLCAST * 9) / 2,
                TAR_IGNORE | TAR_NOCOMBAT, spell_wall_of_ice);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_FROSTMAGUS);
  //SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_TEMPESTMAGUS);
  

  SPELL_CREATE("life ward", SPELL_LIFE_WARD, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT, cast_life_ward);
  SPELL_ADD(CLASS_WARLOCK, 10);
  SPELL_CREATE("wall of stone", SPELL_WALL_OF_STONE, PULSE_SPELLCAST * 5,
                TAR_IGNORE | TAR_NOCOMBAT, spell_wall_of_stone);
  SPELL_ADD(CLASS_CONJURER, 5);

  SPELL_CREATE("wall of iron", SPELL_WALL_OF_IRON, (PULSE_SPELLCAST * 11) / 2,
                TAR_IGNORE | TAR_NOCOMBAT, spell_wall_of_iron);
  SPELL_ADD(CLASS_CONJURER, 6);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 8, SPEC_STARMAGUS);

  SPELL_CREATE("wall of force", SPELL_WALL_OF_FORCE, (PULSE_SPELLCAST * 5 ) / 2,
                TAR_IGNORE | TAR_NOCOMBAT, spell_wall_of_force);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_SUMMONER, 4);
 /* SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_AIR);
  SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_WATER);
  SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_EARTH);
  SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_FIRE);*/

  SPELL_CREATE("wall of bones", SPELL_WALL_OF_BONES, ( PULSE_SPELLCAST * 9 ) / 2,
                TAR_IGNORE | TAR_NOCOMBAT, spell_wall_of_bones);
  SPELL_ADD(CLASS_NECROMANCER, 7);
 // SPELL_ADD(CLASS_THEURGIST, 7);

  SPELL_CREATE("lightning curtain", SPELL_LIGHTNING_CURTAIN, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT, cast_lightning_curtain);
 // SPELL_ADD(CLASS_DRUID, 8);
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_STORM);

  SPELL_ADD(CLASS_ETHERMANCER, 6);

  SPELL_CREATE_MSG("self comprehension", SPELL_SELF_COMPREHENSION, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_comprehend_languages, "You can no longer understand languages.");
//   SPELL_ADD(CLASS_SORCERER, 4);
//   SPELL_ADD(CLASS_CONJURER, 4);
//   SPELL_ADD(CLASS_BARD, 4);
//   SPELL_ADD(CLASS_NECROMANCER, 4);
//   SPELL_ADD(CLASS_ILLUSIONIST, 4);
  SPELL_ADD(CLASS_CLERIC, 4);

  SPELL_CREATE_MSG("comprehend languages", SPELL_COMPREHEND_LANGUAGES, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_comprehend_languages, "You can no longer comprehend languages.");
//   SPELL_ADD(CLASS_CLERIC, 4);
//   SPELL_ADD(CLASS_WARLOCK, 4);

#if 0
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_CLERIC, 4);
  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_BARD, 6);
#endif

  SPELL_CREATE_MSG("slow poison", SPELL_SLOW_POISON, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_slow_poison, "Your resistance to &+gpoison &nlessens.&n");
  SPELL_ADD(CLASS_CLERIC, 2);

  SPELL_CREATE_MSG("coldshield", SPELL_COLDSHIELD, PULSE_SPELLCAST * 3 / 2,
                TAR_SELF_ONLY,
                spell_coldshield, "The &+Bkilling ice&N around your body melts.");
  SPELL_ADD(CLASS_CONJURER, 5);
  SPELL_ADD(CLASS_SUMMONER, 5);
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_ETHERMANCER, 4);
  SPEC_SPELL_ADD(CLASS_REAVER, 6, SPEC_ICE_REAVER);
  SPELL_ADD(CLASS_THEURGIST, 5);

  SPELL_CREATE_MSG("charm animal", SPELL_CHARM_ANIMAL, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_SELF_NONO,
                spell_charm_animal, "You regain your senses.");

  SPELL_CREATE_MSG("soulshield", SPELL_SOULSHIELD, PULSE_SPELLCAST * 3 / 2,
                TAR_SELF_ONLY,
                spell_soulshield, "&+WThe spiritual aura about your body fades&n.");
  SPELL_ADD(CLASS_CLERIC, 5);
  SPELL_ADD(CLASS_PALADIN, 7);
  SPELL_ADD(CLASS_ANTIPALADIN, 7);
  
  SPELL_CREATE_MSG("holy aura", SPELL_HOLY_AURA, PULSE_SPELLCAST * 3 / 2,
                TAR_SELF_ONLY,
                spell_soulshield, "&+WThe spiritual aura about your body fades&n.");
  SPELL_ADD(CLASS_THEURGIST, 7);

  SPELL_CREATE_MSG("dharma", SPELL_HOLY_DHARMA, PULSE_SPELLCAST * 2,
    TAR_SELF_ONLY,
    spell_holy_dharma, "&+cThe &+Cdivine &+cinspiration departs from your soul.&n");
  SPELL_ADD(CLASS_PALADIN, 9);

  SPELL_CREATE_MSG("holy sacrifice", SPELL_HOLY_SACRIFICE, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY,
                spell_holy_sacrifice, "You no longer feel as if your blood will aid anyone.");
  SPEC_SPELL_ADD(CLASS_PALADIN, 10, SPEC_CAVALIER);
  SPEC_SPELL_ADD(CLASS_PALADIN, 10, SPEC_CRUSADER);

  SPELL_CREATE_MSG("battle ecstasy", SPELL_BATTLE_ECSTASY, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY, spell_battle_ecstasy,
               "&+WYou feel your inner turmoil disappear&n.");
  //SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 9, SPEC_DARKKNIGHT);
  //SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 9, SPEC_DEMONIC);
  SPELL_ADD(CLASS_ANTIPALADIN, 8);
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 0, SPEC_VIOLATOR);

  SPELL_CREATE("mass heal", SPELL_MASS_HEAL, PULSE_SPELLCAST * 2,
                TAR_IGNORE, spell_mass_heal);
  SPELL_ADD(CLASS_CLERIC, 8);

  SPELL_CREATE_MSG("true seeing", SPELL_TRUE_SEEING, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_true_seeing, "&+MYour vision &n&+mdulls.&n");
  SPELL_ADD(CLASS_CLERIC, 11);

  SPELL_CREATE_MSG("shadow vision", SPELL_SHADOW_VISION, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_shadow_vision, "&+MYour vision &n&+mdulls.&n");
  SPELL_ADD(CLASS_WARLOCK, 11);
  SPELL_ADD(CLASS_BLIGHTER, 8);

  SPELL_CREATE_MSG("animal vision", SPELL_ANIMAL_VISION, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_animal_vision, "You vision &+ydulls.&n");
  SPELL_ADD(CLASS_DRUID, 8);
  SPELL_ADD(CLASS_RANGER, 11);
  
  SPELL_CREATE_MSG("scent of the bloodhound", SPELL_BLOODHOUND, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT | TAR_ANIMAL,
                spell_bloodhound, "Your nose feels &+Gclogged&n.&n");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 8, SPEC_ANIMALIST);

  SPELL_CREATE_MSG("tree", SPELL_TREE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_tree, "You feel more like yourself.&n");
  //SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_WOODLAND);
  SPELL_ADD(CLASS_RANGER, 10);

  SPELL_CREATE("animal growth", SPELL_ANIMAL_GROWTH, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_ANIMAL,
                spell_animal_growth);
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_WOODLAND);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 7, SPEC_ANIMALIST);

  SPELL_CREATE_MSG("natures blessing", SPELL_NATURES_BLESSING, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_natures_blessing, "You feel cold and alone for a moment.");
  SPELL_ADD(CLASS_DRUID, 10);

  SPELL_CREATE_MSG("enlarge", SPELL_ENLARGE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_enlarge, "&+MYou return to your normal size.&n");
  SPELL_ADD(CLASS_SORCERER, 11);
  SPELL_ADD(CLASS_BARD, 12);

  SPELL_CREATE_MSG("rope trick", SPELL_ROPE_TRICK, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_rope_trick, "&+WYou climb down the &+yrope&n.");
  SPELL_ADD(CLASS_BARD, 11);

  SPELL_CREATE_MSG("reduce", SPELL_REDUCE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_reduce, "&+MYou return to your normal size.&n");
  SPELL_ADD(CLASS_SORCERER, 11);

  SPELL_CREATE("word of command", SPELL_COMMAND, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_command);
  SPELL_ADD(CLASS_CLERIC, 2);


  /* shaman spells */

  /* animal spells first */

  SPELL_CREATE_MSG("wolfspeed", SPELL_WOLFSPEED, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_ANIMAL,
                spell_wolfspeed, "You feel less like a wolf and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 1);

  SPELL_CREATE("pythonsting", SPELL_PYTHONSTING, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ANIMAL,
                spell_pythonsting);
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE_MSG("snailspeed", SPELL_SNAILSPEED, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ANIMAL,
                spell_snailspeed, "You feel less like a snail and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE_MSG("molevision", SPELL_MOLEVISION, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ANIMAL,
                spell_molevision, "You feel less like a mole and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE_MSG("pantherspeed", SPELL_PANTHERSPEED, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_ANIMAL,
                spell_pantherspeed, "You feel less like a panther and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE_MSG("mousestrength", SPELL_MOUSESTRENGTH, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ANIMAL,
                spell_mousestrength, "You feel less like a mouse and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("summon beast", SPELL_SUMMON_BEAST, PULSE_SPELLCAST * 2,
                TAR_IGNORE | TAR_NOCOMBAT | TAR_ANIMAL, spell_summon_beast);
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("greater summon beast", SPELL_GREATER_SUMMON_BEAST, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT | TAR_ANIMAL,
                spell_greater_summon_beast);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 9, SPEC_ANIMALIST);

  SPELL_CREATE_MSG("hawkvision", SPELL_HAWKVISION, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_ANIMAL,
                spell_hawkvision, "You feel less like a hawk and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE_MSG("bearstrength", SPELL_BEARSTRENGTH, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_ANIMAL,
                spell_bearstrength, "You feel less like a bear and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE_MSG("shrewtameness", SPELL_SHREWTAMENESS, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ANIMAL,
                spell_shrewtameness, "You feel less like a shrew and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE_MSG("lionrage", SPELL_LIONRAGE, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_ANIMAL,
                spell_lionrage, "You feel less like a lion and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE_MSG("elephantstrength", SPELL_ELEPHANTSTRENGTH, PULSE_SPELLCAST * 4,
                TAR_CHAR_ROOM | TAR_ANIMAL,
                spell_elephantstrength, "You feel less like an elephant and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE_MSG("ravenflight", SPELL_RAVENFLIGHT, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_NOCOMBAT | TAR_ANIMAL,
                spell_ravenflight, "Shoulders tingling, you gently drop back to the ground.");
  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE("greater ravenflight", SPELL_GREATER_RAVENFLIGHT, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT | TAR_ANIMAL,
                spell_greater_ravenflight);
  SPELL_ADD(CLASS_SHAMAN, 9);

  SPELL_CREATE("greater pythonsting", SPELL_GREATER_PYTHONSTING, PULSE_SPELLCAST * 4,
                TAR_AREA | TAR_AGGRO | TAR_ANIMAL,
                spell_greater_pythonsting);
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE("call of the wild", SPELL_CALL_OF_THE_WILD, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_AGGRO | TAR_ANIMAL,
                spell_call_of_the_wild);
//  SPELL_ADD(CLASS_SHAMAN, 12);

  SPELL_CREATE("beastform", SPELL_BEASTFORM, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY | TAR_NOCOMBAT | TAR_ANIMAL,
                spell_beastform);
//  SPELL_ADD(CLASS_SHAMAN, 5);



  /* elemental spells */


  SPELL_CREATE_MSG("elemental affinity", SPELL_ELEM_AFFINITY, PULSE_SPELLCAST * 4,
                TAR_SELF_ONLY | TAR_NOCOMBAT | TAR_ELEMENTAL,
                spell_elemental_affinity, "&+BA slight&N &+Wch&N&+Cil&N&+Wl&N &+Bruns through you as you feel&N &+gn&N&+Ga&N&+gt&N&+Gu&N&+gr&N&+Ge's&N &+Yes&N&+Ws&N&+Yen&N&+Wce&N &+Bleave your body.&N");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 7, SPEC_ELEMENTALIST);
  SPELL_CREATE("cascading elemental beam", SPELL_CASCADING_ELEMENTAL_BEAM, 
                PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
                spell_cascading_elemental_beam);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_ELEMENTALIST);
  SPELL_CREATE("elemental fury", SPELL_ELEM_FURY, PULSE_SPELLCAST * 4,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO | TAR_ELEMENTAL,
                spell_elemental_fury);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 12, SPEC_ELEMENTALIST);
  
  SPELL_CREATE("ice missile", SPELL_ICE_MISSILE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO | TAR_ELEMENTAL,
                spell_ice_missile);
  SPELL_ADD(CLASS_SHAMAN, 1);
  SPELL_ADD(CLASS_ETHERMANCER, 1);

  SPELL_CREATE("flameburst", SPELL_FLAMEBURST, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
                spell_flameburst);
  SPELL_ADD(CLASS_SHAMAN, 2);
  SPEC_SPELL_ADD(CLASS_REAVER, 4, SPEC_FLAME_REAVER);

  SPELL_CREATE("scalding blast", SPELL_SCALDING_BLAST, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
                spell_scalding_blast);
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE_MSG("cold ward", SPELL_COLD_WARD, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT | TAR_ELEMENTAL,
                spell_cold_ward, "&+cYou no longer feel protected from the cold.");
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE_MSG("fire ward", SPELL_FIRE_WARD, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT | TAR_ELEMENTAL,
                spell_fire_ward, "&+rYou no longer feel protected from fire.");
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE("scorching touch", SPELL_SCORCHING_TOUCH, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
                spell_scorching_touch);
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("molten spray", SPELL_MOLTEN_SPRAY, PULSE_SPELLCAST * (3 / 2),
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_AGGRO | TAR_ELEMENTAL,
                spell_molten_spray);
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE_MSG("earthen grasp", SPELL_EARTHEN_GRASP, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
                spell_earthen_grasp, "&+yThe earthen fist finally releases its grip on you.");
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE_MSG("greater earthen grasp", SPELL_GREATER_EARTHEN_GRASP, PULSE_SPELLCAST * 2,
                TAR_AREA | TAR_AGGRO | TAR_ELEMENTAL,
                spell_greater_earthen_grasp, "&+yThe earthen fist finally releases its grip.");
  SPELL_ADD(CLASS_SHAMAN, 11);

  SPELL_CREATE("arieks shattering iceball", SPELL_ARIEKS_SHATTERING_ICEBALL,
               PULSE_SPELLCAST * 3 / 2,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
               spell_arieks_shattering_iceball);
  SPELL_ADD(CLASS_SHAMAN, 9);

  SPELL_CREATE("scathing wind", SPELL_SCATHING_WIND, PULSE_SPELLCAST * 3,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO | TAR_ELEMENTAL,
                spell_scathing_wind);
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE("earthen rain", SPELL_EARTHEN_RAIN, PULSE_SPELLCAST * 3,
                TAR_AREA | TAR_OFFAREA | TAR_AGGRO | TAR_ELEMENTAL,
                spell_earthen_rain);
  SPELL_ADD(CLASS_SHAMAN, 10);


  /* spirit spells */
  
  SPELL_CREATE("tormenting spirits", SPELL_TORMENT_SPIRITS, 
                PULSE_SPELLCAST * 5 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_SPIRIT,
                spell_torment_spirits);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 8, SPEC_SPIRITUALIST);

  SPELL_CREATE_MSG("indomitability", SPELL_INDOMITABILITY, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_SPIRIT,
                spell_indomitability, "&+BYour spiritual senses return to the land of the living, bringing your fears and vulnerabilities back with them.&N");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 7, SPEC_SPIRITUALIST);

  SPELL_CREATE_MSG("spirit armor", SPELL_SPIRIT_ARMOR, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_SPIRIT,
                spell_spirit_armor, "&+mYou feel less protected.");
  SPELL_ADD(CLASS_SHAMAN, 1);

  SPELL_CREATE("transfer wellness", SPELL_TRANSFER_WELLNESS, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_SPIRIT,
                spell_transfer_wellness);
  SPELL_ADD(CLASS_SHAMAN, 1);

  SPELL_CREATE("sustenance", SPELL_SUSTENANCE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_sustenance);
  //SPELL_ADD(CLASS_SHAMAN, 2);

  SPELL_CREATE("reveal spirit essence", SPELL_REVEAL_SPIRIT_ESSENCE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_SELF_NONO | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_reveal_spirit_essence);
  SPELL_ADD(CLASS_SHAMAN, 2);

  SPELL_CREATE("purify spirit", SPELL_PURIFY_SPIRIT, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_SPIRIT, spell_purify_spirit);
  SPELL_ADD(CLASS_SHAMAN, 3);
  SPELL_ADD(CLASS_ETHERMANCER, 2);

  SPELL_CREATE("lesser mending", SPELL_LESSER_MENDING, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_SPIRIT, spell_lesser_mending);
  SPELL_ADD(CLASS_SHAMAN, 2);

  SPELL_CREATE("mending", SPELL_MENDING, (3 * PULSE_SPELLCAST) / 2,
                TAR_CHAR_ROOM | TAR_SPIRIT, spell_mending);
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("soul disturbance", SPELL_SOUL_DISTURBANCE, PULSE_SPELLCAST * 5 / 4,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_SPIRIT,
                spell_soul_disturbance);
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE_MSG("spirit sight", SPELL_SPIRIT_SIGHT, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_spirit_sight, "&+LYour spirit sight fades.");
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE_MSG("sense spirit", SPELL_SENSE_SPIRIT, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_sense_spirit, "&+LYour sense of spirits disappears.");
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE_MSG("malison", SPELL_MALISON, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_SPIRIT,
                spell_malison, "You feel less cursed.");
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE("wellness", SPELL_WELLNESS, PULSE_SPELLCAST,
                TAR_IGNORE | TAR_SPIRIT, spell_wellness);
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("greater mending", SPELL_GREATER_MENDING, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_SPIRIT,
                spell_greater_mending);
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("spirit anguish", SPELL_SPIRIT_ANGUISH, 2 * PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_SPIRIT,
                spell_spirit_anguish);
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("greater spirit anguish", SPELL_GREATER_SPIRIT_ANGUISH, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_SPIRIT,
                spell_greater_spirit_anguish);
  SPELL_ADD(CLASS_SHAMAN, 12);

  SPELL_CREATE("greater soul disturbance", SPELL_GREATER_SOUL_DISTURB, 4/3 * PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_SPIRIT,
                spell_greater_soul_disturbance);
  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE_MSG("spirit ward", SPELL_SPIRIT_WARD, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_SPIRIT,
                spell_spirit_ward, "The dim aura around you fades.");
  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE("greater sustenance", SPELL_GREATER_SUST, PULSE_SPELLCAST,
                TAR_IGNORE | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_greater_sustenance);
//  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE("reveal true form", SPELL_REVEAL_TRUE_FORM, PULSE_SPELLCAST,
                TAR_IGNORE | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_reveal_true_form);
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE("spirit jump", SPELL_SPIRIT_JUMP, PULSE_SPELLCAST * 3,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_spirit_jump);
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE_MSG("greater spirit sight", SPELL_GREATER_SPIRIT_SIGHT, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_greater_spirit_sight, "&+LYour spirit sight fades.");
  SPELL_ADD(CLASS_SHAMAN, 11);
  SPELL_CREATE_MSG("greater spirit ward", SPELL_GREATER_SPIRIT_WARD, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_SPIRIT,
                spell_greater_spirit_ward, "The dim aura around you fades.");
  SPELL_ADD(CLASS_SHAMAN, 9);

  SPELL_CREATE("etherportal", SPELL_ETHERPORTAL, PULSE_SPELLCAST * 7,
                TAR_CHAR_WORLD | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_etherportal);
  SPELL_ADD(CLASS_SHAMAN, 10);

  SPELL_CREATE("spirit walk", SPELL_SPIRIT_WALK, PULSE_SPELLCAST * 7,
                TAR_CHAR_ROOM | TAR_SELF_NONO | TAR_NOCOMBAT | TAR_SPIRIT, spell_spirit_walk);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 10, SPEC_SPIRITUALIST);

  SPELL_CREATE_MSG("essence of the wolf", SPELL_ESSENCE_OF_WOLF, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_SELF_NONO | TAR_NOCOMBAT | TAR_ANIMAL,
                spell_essence_of_the_wolf, "&+yThe essence of the wolf leaves your being.&n");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_ANIMALIST);

// Spells not yet implemented
// Guess again! Zion 9/07


  SPELL_CREATE_MSG("corrosive blast", SPELL_CORROSIVE_BLAST, (int) (PULSE_SPELLCAST * 3/2),
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
                spell_corrosive_blast, "&+GThe corrosive acid evaporates from your armor.&n");
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE("nivards wicked firebrand", SPELL_FIREBRAND, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO | TAR_ELEMENTAL,
                spell_firebrand);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_ELEMENTALIST);

  SPELL_CREATE("restoration", SPELL_RESTORATION, PULSE_SPELLCAST * 5/2,
                TAR_CHAR_ROOM | TAR_SPIRIT,
                spell_restoration);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 10, SPEC_SPIRITUALIST);

  SPELL_CREATE("summon spirit", SPELL_SUMMON_SPIRIT, PULSE_SPELLCAST,
                TAR_IGNORE | TAR_NOCOMBAT | TAR_SPIRIT,
                spell_summon_spirit);
  /* end of shaman spells */

  /* Reaver spells */
  SPELL_CREATE_MSG("baladors protection", SPELL_BALADORS_PROTECTION, PULSE_SPELLCAST,
                TAR_SELF_ONLY,
                spell_baladors_protection, "You feel Balador's protection leave you.");
  SPELL_ADD(CLASS_REAVER, 5);

  SPELL_CREATE_MSG("ferrix precision", SPELL_FERRIX_PRECISION, PULSE_SPELLCAST,
                TAR_SELF_ONLY,
                spell_ferrix_precision, "&+BYou feel less precise&n");
  SPELL_ADD(CLASS_REAVER, 7);

  SPELL_CREATE_MSG("eshabalas vitality", SPELL_ESHABALAS_VITALITY, PULSE_SPELLCAST,
                TAR_SELF_ONLY,
                spell_eshabalas_vitality, "&+mYou feel a bit worn out now that Eshabala has left you.&n");
  //SPELL_ADD(CLASS_REAVER, 9);

  SPELL_CREATE_MSG("cegilunes searing blade", SPELL_CEGILUNE_BLADE, PULSE_SPELLCAST * 4 / 3,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_cegilunes_searing_blade, "&+rYour weapon ceases to glow as Ceguline's essence departs.&n");
  SPEC_SPELL_ADD(CLASS_REAVER, 12, SPEC_FLAME_REAVER);

  SPELL_CREATE_MSG("kanchelsis fury", SPELL_KANCHELSIS_FURY, (PULSE_SPELLCAST * 3) / 2,
                TAR_SELF_ONLY,
                spell_kanchelsis_fury, "&+LYou feel the world speeding up around you.&n");
  SPELL_ADD(CLASS_REAVER, 8);

  SPELL_CREATE("blood alliance", SPELL_BLOOD_ALLIANCE, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_SELF_NONO, spell_blood_alliance);

  SPELL_CREATE_MSG("ilienzes flaming sword", SPELL_ILIENZES_FLAME_SWORD, PULSE_SPELLCAST * 4 / 3,
                   TAR_SELF_ONLY,
                   spell_ilienzes_flame_sword, "&+rYour fl&+yam&+re slowly burns out.&n");
  SPEC_SPELL_ADD(CLASS_REAVER, 11, SPEC_FLAME_REAVER);

  SPELL_CREATE_MSG("thryms icerazor", SPELL_THRYMS_ICERAZOR, PULSE_SPELLCAST * 4 / 3,
                   TAR_SELF_ONLY,
                   spell_thryms_icerazor, "&+CYou feel weaker as &+LThrym's &+Cessence leaves you.&n");
  SPEC_SPELL_ADD(CLASS_REAVER, 11, SPEC_ICE_REAVER);

  SPELL_CREATE_MSG("kostchtchies chilling implosion", SPELL_CHILLING_IMPLOSION, PULSE_SPELLCAST * 4 / 3,
                   TAR_SELF_ONLY | TAR_NOCOMBAT,
                   spell_kostchtchies_implosion, "&+LThe essence of &+CKostchtchie &+Lleaves your weapon.&n");
  SPEC_SPELL_ADD(CLASS_REAVER, 12, SPEC_ICE_REAVER);

  SPELL_CREATE_MSG("lliendils stormshock", SPELL_LLIENDILS_STORMSHOCK, PULSE_SPELLCAST * 4 / 3,
                   TAR_SELF_ONLY,
                   spell_lliendils_stormshock, "&+BThe power of &+Lstorms &+Bleaves your weapon.&n");
  SPEC_SPELL_ADD(CLASS_REAVER, 11, SPEC_SHOCK_REAVER);

  SPELL_CREATE_MSG("stormcallers fury", SPELL_STORMCALLERS_FURY, PULSE_SPELLCAST * 4 / 3,
                   TAR_SELF_ONLY | TAR_NOCOMBAT,
                   spell_stormcallers_fury, "&+BThe call of the &+Lstorm&+B leaves your weapon.&n");
  SPEC_SPELL_ADD(CLASS_REAVER, 12, SPEC_SHOCK_REAVER);

  SPELL_CREATE_MSG("lightning shield", SPELL_LIGHTNINGSHIELD, PULSE_SPELLCAST * 3 / 2,
                   TAR_SELF_ONLY,
                   spell_lightning_shield, "&+YThe maelstrom of energy surrounding your body fades.");
  SPEC_SPELL_ADD(CLASS_REAVER, 6, SPEC_SHOCK_REAVER);
  SPELL_ADD(CLASS_ETHERMANCER, 6);

/* Illusionists */
/* Mossi Modification: 13 Nov 2002 */

  SPELL_CREATE_MSG("phantom armor", SPELL_PHANTOM_ARMOR, PULSE_SPELLCAST,
                TAR_SELF_ONLY,
                spell_phantom_armor, "&+LYou become less hazy.&N");
  SPELL_ADD(CLASS_ILLUSIONIST, 1);

  SPELL_CREATE("shadow monster", SPELL_SHADOW_MONSTER, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_shadow_monster);
  SPELL_ADD(CLASS_ILLUSIONIST, 3);

  SPELL_CREATE("insects", SPELL_INSECTS, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_insects);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);

  SPELL_CREATE("illusionary wall", SPELL_ILLUSIONARY_WALL, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                spell_illusionary_wall);
  SPELL_ADD(CLASS_ILLUSIONIST, 5);

  SPELL_CREATE("boulder", SPELL_BOULDER, PULSE_SPELLCAST * 5 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_boulder);
  SPELL_ADD(CLASS_ILLUSIONIST, 5);

  SPELL_CREATE("shadow travel", SPELL_SHADOW_TRAVEL, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT, spell_shadow_travel);
  SPELL_ADD(CLASS_ILLUSIONIST, 5);

  SPELL_CREATE("stunning visions", SPELL_STUNNING_VISIONS, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_stunning_visions);
  SPELL_ADD(CLASS_ILLUSIONIST, 6);

  SPELL_CREATE("reflection", SPELL_REFLECTION, PULSE_SPELLCAST,
                TAR_CHAR_ROOM, spell_reflection);
  SPELL_ADD(CLASS_ILLUSIONIST, 6);

  SPELL_CREATE("mask", SPELL_MASK, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT, spell_mask);
  SPELL_ADD(CLASS_ILLUSIONIST, 8);

  SPELL_CREATE("watching wall", SPELL_WATCHING_WALL, PULSE_SPELLCAST * 5,
                TAR_IGNORE | TAR_NOCOMBAT, spell_watching_wall);
  SPEC_SPELL_ADD(CLASS_ILLUSIONIST, 6, SPEC_DECEIVER);

  SPELL_CREATE_MSG("nightmare", SPELL_NIGHTMARE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_nightmare, "You feel less frightend.");
  SPELL_ADD(CLASS_ILLUSIONIST, 6);

  SPELL_CREATE_MSG("shadow shield", SPELL_SHADOW_SHIELD, PULSE_SPELLCAST * 4 / 3,
                TAR_SELF_ONLY,
                spell_shadow_shield, "&+yYou feel the &+Ls&+Ww&+Li&+Wr&+Ll&+Wi&+Ln&+Wg &n&+yshadows around your body dissipate.&n");
  SPELL_ADD(CLASS_ILLUSIONIST, 8);
  SPELL_ADD(CLASS_SORCERER, 8);

  SPELL_CREATE_MSG("vanish", SPELL_VANISH, PULSE_SPELLCAST,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_vanish, "&+WYou snap back into visibility.&n");
  SPELL_ADD(CLASS_ILLUSIONIST, 8);

  SPELL_CREATE("hammer", SPELL_HAMMER, PULSE_SPELLCAST * 5 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_hammer);
  SPELL_ADD(CLASS_ILLUSIONIST, 7);

  SPELL_CREATE_MSG("detect illusion", SPELL_DETECT_ILLUSION, PULSE_SPELLCAST * 4 / 3,
                TAR_SELF_ONLY,
                spell_detect_illusion, "Unseen things and illusion vanish from your sight.");
  SPELL_ADD(CLASS_ILLUSIONIST, 9);

  SPELL_CREATE("dream travel", SPELL_DREAM_TRAVEL, PULSE_SPELLCAST * 4,
                TAR_CHAR_WORLD | TAR_NOCOMBAT, spell_dream_travel);
  SPELL_ADD(CLASS_ILLUSIONIST, 9);

  SPELL_CREATE("clone form", SPELL_CLONE_FORM, PULSE_SPELLCAST * 4,
                TAR_CHAR_ROOM | TAR_NOCOMBAT, spell_clone_form);
  SPELL_ADD(CLASS_ILLUSIONIST, 10);

  SPELL_CREATE("imprisonment", SPELL_IMPRISON, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOCOMBAT | TAR_AGGRO, spell_imprison);
  SPELL_ADD(CLASS_ILLUSIONIST, 10);

  SPELL_CREATE_MSG("nonexistence", SPELL_NONEXISTENCE, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT,
                spell_nonexistence, "You slowly fade back into visibility.");
  SPELL_ADD(CLASS_ILLUSIONIST, 11);

  SPELL_CREATE("dragon", SPELL_DRAGON, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_dragon);
  SPELL_ADD(CLASS_ILLUSIONIST, 12);

  SPELL_CREATE("titan", SPELL_TITAN, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_titan);
  SPELL_ADD(CLASS_ILLUSIONIST, 11);

  SPELL_CREATE_MSG("delirium", SPELL_DELIRIUM, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_delirium, "&+WYou feel less &+Gconfused&n");
  SPELL_ADD(CLASS_ILLUSIONIST, 10);

  SPELL_CREATE("flicker", SPELL_FLICKER, PULSE_SPELLCAST * 5,
                TAR_IGNORE | TAR_NOCOMBAT, spell_flicker);
  SPELL_ADD(CLASS_ILLUSIONIST, 11);

  SPELL_CREATE("greater flicker", SPELL_GREATER_FLICKER, PULSE_SPELLCAST * 7,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_greater_flicker);
  //SPELL_ADD(CLASS_ILLUSIONIST, 12);

// Ethermancer

  SPELL_CREATE_MSG("vapor armor", SPELL_VAPOR_ARMOR, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_vapor_armor, "&+CYou feel the protection of the winds dissapate.");
  SPELL_ADD(CLASS_ETHERMANCER, 1);

  SPELL_CREATE_MSG("faerie sight", SPELL_FAERIE_SIGHT, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_faerie_sight, "You feel the &+mtwinkle&n in your eyes fade.");

  SPELL_ADD(CLASS_ETHERMANCER, 4);

  SPELL_CREATE("frost beacon", SPELL_FROST_BEACON, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT, spell_frost_beacon);
  SPELL_ADD(CLASS_ETHERMANCER, 6);

  SPELL_CREATE("cold snap", SPELL_COLD_SNAP, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_cold_snap);
  SPELL_ADD(CLASS_ETHERMANCER, 7);
  SPEC_SPELL_ADD(CLASS_REAVER, 8, SPEC_ICE_REAVER);

  SPELL_CREATE_MSG("vapor strike", SPELL_VAPOR_STRIKE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_vapor_strike, "&+CThe vapors subside leaving you weak.&n");
  SPELL_ADD(CLASS_ETHERMANCER, 5);

  SPELL_CREATE("wind blade", SPELL_WIND_BLADE, PULSE_SPELLCAST,
                TAR_IGNORE, spell_wind_blade);
  SPELL_ADD(CLASS_ETHERMANCER, 3);

  SPELL_CREATE("windwalk", SPELL_WINDWALK, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT, spell_windwalk);
  SPELL_ADD(CLASS_ETHERMANCER, 7);

  SPELL_CREATE_MSG("frost bolt", SPELL_FROST_BOLT, PULSE_SPELLCAST * 3 / 4,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_frost_bolt, "The &+Cice&n wracking your body melts.");
  SPELL_ADD(CLASS_ETHERMANCER, 3);
  SPEC_SPELL_ADD(CLASS_REAVER, 4, SPEC_ICE_REAVER);

  SPELL_CREATE("arctic blast", SPELL_ARCTIC_BLAST, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_arctic_blast );
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_FROSTMAGUS);

  SPELL_CREATE("antimatter collision", SPELL_AMATTER_COLLISION, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_antimatter_collision );
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_STARMAGUS);

  SPELL_CREATE_MSG("ethereal form", SPELL_ETHEREAL_FORM, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_ethereal_form, "&+LYou feel your body begin to regain its former substance...");
  SPELL_ADD(CLASS_ETHERMANCER, 8);

  SPELL_CREATE("ethereal recharge", SPELL_ETHEREAL_RECHARGE, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM,
                spell_ethereal_recharge);
  SPELL_ADD(CLASS_ETHERMANCER, 8);
  SPEC_SPELL_ADD(CLASS_CONJURER, 8, SPEC_AIR);

  SPELL_CREATE("arcane whirlwind", SPELL_ARCANE_WHIRLWIND, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_AGGRO,
                spell_arcane_whirlwind);
  SPELL_ADD(CLASS_ETHERMANCER, 9);

  SPELL_CREATE("forked lightning", SPELL_FORKED_LIGHTNING,
               PULSE_SPELLCAST * 2, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
               spell_forked_lightning);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_TEMPESTMAGUS);

  SPELL_CREATE("ice spikes", SPELL_ICE_SPIKES,
               PULSE_SPELLCAST * 2, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
               spell_ice_spikes);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_FROSTMAGUS);

  SPELL_CREATE("wall of air", SPELL_WALL_OF_AIR, PULSE_SPELLCAST * 4,
                TAR_IGNORE | TAR_NOCOMBAT, spell_wall_of_air);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 8, SPEC_TEMPESTMAGUS);
  SPEC_SPELL_ADD(CLASS_CONJURER, 7, SPEC_AIR);

  SPELL_CREATE("purge", SPELL_PURGE, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT, spell_purge);
  SPELL_ADD(CLASS_ETHERMANCER, 10);

  SPELL_CREATE("conjure air", SPELL_CONJURE_AIR, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT, spell_conjure_air);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 5, SPEC_TEMPESTMAGUS);

  SPELL_CREATE("conjure void", SPELL_CONJURE_VOID, PULSE_SPELLCAST * 3,
	            TAR_IGNORE | TAR_NOCOMBAT, spell_conjure_void_elemental);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_STARMAGUS);

  SPELL_CREATE("conjure ice", SPELL_CONJURE_ICE, PULSE_SPELLCAST * 3,
	            TAR_IGNORE | TAR_NOCOMBAT, spell_conjure_ice_elemental);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_FROSTMAGUS);

  SPELL_CREATE_MSG("iceflow armor", SPELL_ICE_ARMOR, PULSE_SPELLCAST * 4 / 3,
                    TAR_SELF_ONLY,
                    spell_iceflow_armor, "&+CThe &+Bicy&n shell encasing you melts away.&n");
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 8, SPEC_FROSTMAGUS);

  SPELL_CREATE_MSG("negative feedback barrier", SPELL_NEG_ARMOR, PULSE_SPELLCAST * 4 / 3,
                    TAR_SELF_ONLY,
                    spell_negative_feedback_barrier, "&+LThe negative field encasing you fades away...&n");
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 8, SPEC_STARMAGUS);

  SPELL_CREATE("etheric gust", SPELL_ETHERIC_GUST, PULSE_SPELLCAST,
                TAR_SELF_ONLY, spell_etheric_gust);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 8, SPEC_TEMPESTMAGUS);
	            
  SPELL_CREATE_MSG("induce tupor", SPELL_INDUCE_TUPOR, PULSE_SPELLCAST * 4,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_induce_tupor, "You awake from your magical slumber.");
  SPELL_ADD(CLASS_ETHERMANCER, 10);

  SPELL_CREATE2("tempest terrain", SPELL_TEMPEST_TERRAIN, PULSE_SPELLCAST * 6,
                TAR_IGNORE | TAR_NOCOMBAT,
                spell_tempest_terrain, "The terrain becomes solid once again.",
                "The terrain becomes solid once again.");
  SPELL_ADD(CLASS_ETHERMANCER, 11);

  SPELL_CREATE_MSG("storm empathy", SPELL_STORM_EMPATHY, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_storm_empathy, "You do not feel as safe from &+Bstorms&n.");
  SPELL_ADD(CLASS_ETHERMANCER, 5);

  SPELL_CREATE("mass fly", SPELL_MASS_FLY, PULSE_SPELLCAST * 3,
                TAR_IGNORE, spell_mass_fly);
  SPELL_ADD(CLASS_ETHERMANCER, 11);

  SPELL_CREATE_MSG("path of frost", SPELL_PATH_OF_FROST, PULSE_SPELLCAST * 5,
                TAR_SELF_ONLY,
                spell_path_of_frost, "Your tracks return to normal.");
  //SPELL_ADD(CLASS_ETHERMANCER, 12);
  //SPEC_SPELL_ADD(CLASS_ETHERMANCER, 10, SPEC_FROSTMAGUS);
  //SPEC_SPELL_ADD(CLASS_ETHERMANCER, 11, SPEC_TEMPESTMAGUS);
  
  
  SPELL_CREATE("static discharge", SPELL_STATIC_DISCHARGE, PULSE_SPELLCAST,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_static_discharge);
  SPELL_ADD(CLASS_ETHERMANCER, 5);

  // Cosmomancer spec, Ethermancer
  SPELL_CREATE("supernova", SPELL_SUPERNOVA, PULSE_SPELLCAST * 3,
                TAR_AREA + TAR_OFFAREA | TAR_AGGRO, spell_supernova);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 11, SPEC_STARMAGUS);

  SPELL_CREATE("cosmic vacuum", SPELL_COSMIC_VACUUM, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_cosmic_vacuum);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_STARMAGUS);

  SPELL_CREATE("comet", SPELL_COMET, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_comet);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_STARMAGUS);

  SPELL_CREATE("ethereal discharge", SPELL_ETHEREAL_DISCHARGE, PULSE_SPELLCAST * 3,
                TAR_AREA + TAR_OFFAREA | TAR_AGGRO,
                spell_ethereal_discharge);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 8, SPEC_STARMAGUS);

  SPELL_CREATE_MSG("planetary alignment", SPELL_PLANETARY_ALIGNMENT, PULSE_SPELLCAST * 5,
                TAR_SELF_ONLY,
                spell_planetary_alignment, "Your feel weaker.");
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_STARMAGUS);

  SPELL_CREATE("cosmic rift", SPELL_COSMIC_RIFT, PULSE_SPELLCAST * 6,
	            TAR_AREA + TAR_OFFAREA | TAR_AGGRO, spell_cosmic_rift);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 12, SPEC_STARMAGUS);

  // Frost Magus Spec
  SPELL_CREATE("squall", SPELL_TEMPEST, PULSE_SPELLCAST * 4,
                TAR_OFFAREA | TAR_AGGRO, spell_tempest);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 11, SPEC_FROSTMAGUS);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 10, SPEC_STORMBRINGER);

  SPELL_CREATE_MSG("windrage", SPELL_WIND_RAGE, PULSE_SPELLCAST * 4,
                TAR_SELF_ONLY,
                spell_wind_rage, "&+cYour mind begins to slow as the winds subside.");
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 10, SPEC_FROSTMAGUS);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 10, SPEC_TEMPESTMAGUS);
  
  SPELL_CREATE("polar vortex", SPELL_POLAR_VORTEX, PULSE_SPELLCAST * 4,
	            TAR_OFFAREA | TAR_AGGRO, spell_polar_vortex);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 12, SPEC_FROSTMAGUS);

  // Windtalker Spec
  SPELL_CREATE_MSG("ethereal alliance", SPELL_ETHEREAL_ALLIANCE, PULSE_SPELLCAST * 5,
                TAR_CHAR_ROOM,
                spell_ethereal_alliance, "&+cYour alliance has been severed.");
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 11, SPEC_TEMPESTMAGUS);

  SPELL_CREATE("greater ethereal recharge", SPELL_GREATER_ETHEREAL,
                PULSE_SPELLCAST * 2, TAR_CHAR_ROOM,
                spell_greater_ethereal_recharge);

  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 10, SPEC_TEMPESTMAGUS);

  SPELL_CREATE("razor wind", SPELL_RAZOR_WIND, PULSE_SPELLCAST * 2,
	          TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_razor_wind);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_TEMPESTMAGUS);

  SPELL_CREATE("way of the wind", SPELL_WAY_OF_THE_WIND, PULSE_SPELLCAST * 5,
                TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT, spell_relocate);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_TEMPESTMAGUS);

  SPELL_CREATE("ethereal travel", SPELL_ETHEREAL_TRAVEL, PULSE_SPELLCAST * 11,
	            TAR_CHAR_WORLD | TAR_SELF_NONO | TAR_NOCOMBAT, spell_ethereal_travel);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 12, SPEC_TEMPESTMAGUS);

  SPELL_CREATE("doom blade", SPELL_DOOM_BLADE, PULSE_SPELLCAST * 8,
                TAR_IGNORE | TAR_NOCOMBAT, spell_doom_blade);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 8, SPEC_REAPER);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 6, SPEC_RUINER);

  SPELL_CREATE("holy blade", SPELL_HOLY_BLADE, PULSE_SPELLCAST * 8,
                TAR_IGNORE | TAR_NOCOMBAT, spell_doom_blade);
  SPEC_SPELL_ADD(CLASS_THEURGIST, 8, SPEC_THAUMATURGE);

  SPELL_CREATE_MSG("holy sword", SPELL_HOLY_SWORD, PULSE_SPELLCAST * 4 / 3,
                   TAR_SELF_ONLY,
                   spell_holy_sword, "&+wYour weapon ceases to glow with holy power.&n");
//  SPELL_ADD(CLASS_PALADIN, 10); NO OP spells.
  SPELL_ADD(CLASS_AVENGER, 11);

  SPELL_CREATE_MSG("divine power", SPELL_DIVINE_POWER, PULSE_SPELLCAST * 4,
                 TAR_SELF_ONLY,
           spell_divine_power, "&+wThe divine energies leave your body.&n");
  SPELL_ADD(CLASS_AVENGER, 9);

  SPELL_CREATE_MSG("atonement", SPELL_ATONEMENT, PULSE_SPELLCAST * 3,
                 TAR_SELF_ONLY,
           spell_atonement, "&+WYou sense that your past sins once again need forgiving.&n");
  SPELL_ADD(CLASS_AVENGER, 7);

  SPELL_CREATE_MSG("celestial aura", SPELL_CELESTIAL_AURA, PULSE_SPELLCAST * 5,
           TAR_SELF_ONLY,
           spell_celestial_aura, "&+WYou feel your prayers will go unanswered once again.&n");
  SPELL_ADD(CLASS_AVENGER, 12);

  SPELL_CREATE_MSG("chaos shield", SPELL_CHAOS_SHIELD, PULSE_SPELLCAST * 5,
                TAR_SELF_ONLY | TAR_NOCOMBAT, spell_chaos_shield,
    "&+LThe chaotic aura about you fades and dissapates.&n");
  SPEC_SPELL_ADD(CLASS_SORCERER, 11, SPEC_WILDMAGE);

#if 1
  SKILL_CREATE("aerial combat", SKILL_AERIAL_COMBAT, TAR_PHYS);

  SKILL_CREATE("aerial casting", SKILL_AERIAL_CASTING, TAR_MENTAL);
#endif

  SKILL_CREATE("dirt toss", SKILL_DIRTTOSS, TAR_PHYS);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_THIEF);

//  Sneaky strike is now an epic skill.
//  SKILL_CREATE("sneaky strike", SKILL_SNEAKY_STRIKE, TAR_PHYS);
//  SKILL_ADD(CLASS_BARD, 30, 90);
//  SKILL_ADD(CLASS_ROGUE, 30, 100);
//  SKILL_ADD(CLASS_MERCENARY, 30, 85);

  SKILL_CREATE("climb", SKILL_CLIMB, TAR_PHYS);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 75);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 75);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_PSIONICIST, 1, 40);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 40);
  SKILL_ADD(CLASS_CLERIC, 1, 55);
  SKILL_ADD(CLASS_CONJURER, 1, 40);
  SKILL_ADD(CLASS_SUMMONER, 1, 40);
  SKILL_ADD(CLASS_SORCERER, 1, 40);
  SKILL_ADD(CLASS_WARLOCK, 1, 40);
  SKILL_ADD(CLASS_NECROMANCER, 1, 40);
//  SKILL_ADD(CLASS_CABALIST, 1, 40);
  SKILL_ADD(CLASS_THEURGIST, 1, 40);
  SKILL_ADD(CLASS_SHAMAN, 1, 45);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 45);
  SKILL_ADD(CLASS_DRUID, 1, 45);
  SKILL_ADD(CLASS_MERCENARY, 1, 80);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 40);
  SKILL_ADD(CLASS_BERSERKER, 1, 75);
  SKILL_ADD(CLASS_REAVER, 1, 75);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 40);
  SKILL_ADD(CLASS_DREADLORD, 1, 80);
  SKILL_ADD(CLASS_AVENGER, 1, 80);

  SKILL_CREATE("subterfuge", SKILL_SUBTERFUGE, TAR_MENTAL);
  SKILL_ADD(CLASS_THIEF, 20, 100);
  SKILL_ADD(CLASS_ROGUE, 20, 60);

  SKILL_CREATE("riff", SKILL_RIFF, 0);
  SPEC_SKILL_ADD(CLASS_BARD, 30, 100, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_BARD, 30, 100, SPEC_MINSTREL);
  SPEC_SKILL_ADD(CLASS_BARD, 30, 100, SPEC_DISHARMONIST);

  SKILL_CREATE("disguise", SKILL_DISGUISE, 0);
  SKILL_ADD(CLASS_ROGUE, 46, 60);
  SPEC_SKILL_ADD(CLASS_ROGUE, 30, 95, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_ROGUE, 30, 85, SPEC_ASSASSIN);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 55, SPEC_SHARPSHOOTER);
  SPEC_SKILL_ADD(CLASS_BARD, 51, 55, SPEC_SCOUNDREL);

  SKILL_CREATE("trip", SKILL_TRIP, TAR_PHYS);
  SKILL_ADD(CLASS_THIEF, 16, 100);
  SPEC_SKILL_ADD(CLASS_BARD, 51, 95, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_ROGUE, 16, 95, SPEC_THIEF);

  SKILL_CREATE_WITH_MESSAGES("feign death", SKILL_FEIGN_DEATH, TAR_MENTAL,
                             "&+yYou feel up to faking your death again...&n",
                             "");
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
  SKILL_ADD(CLASS_THEURGIST, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);


  SKILL_CREATE("salvage", SKILL_SALVAGE, TAR_PHYS);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 100);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);



  SKILL_CREATE("quivering palm", SKILL_QUIVERING_PALM, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 10, 100);

  SKILL_CREATE("combination attack", SKILL_COMBINATION, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 10, 100);

  SKILL_CREATE("blade barrage", SKILL_BLADE_BARRAGE, TAR_PHYS);
  SKILL_ADD(CLASS_RANGER, 16, 100);
//  SKILL_ADD(CLASS_REAVER, 41, 100);

  SKILL_CREATE("regenerate", SKILL_REGENERATE, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 20, 100);

  SKILL_CREATE("chi purge", SKILL_CHI_PURGE, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 21, 100);


/*
  SKILL_CREATE("first aid", SKILL_FIRST_AID, TAR_MENTAL);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 100);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
*/
  SKILL_CREATE("retreat", SKILL_RETREAT, TAR_PHYS);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 85);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 95);
  SKILL_ADD(CLASS_PSIONICIST, 1, 70);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 60);
  SKILL_ADD(CLASS_CLERIC, 1, 65);
  SKILL_ADD(CLASS_CONJURER, 1, 70);
  SKILL_ADD(CLASS_SUMMONER, 1, 70);
  SKILL_ADD(CLASS_DRUID, 1, 65);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_PALADIN, 1, 85);
  SKILL_ADD(CLASS_RANGER, 1, 85);
  SKILL_ADD(CLASS_SORCERER, 1, 70);
  SKILL_ADD(CLASS_WARLOCK, 1, 70);
  SKILL_ADD(CLASS_NECROMANCER, 1, 70);
//  SKILL_ADD(CLASS_CABALIST, 1, 70);
  SKILL_ADD(CLASS_THEURGIST, 1, 70);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 60);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 60);
  SKILL_ADD(CLASS_MONK, 1, 75);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 60);
  SKILL_ADD(CLASS_BERSERKER, 1, 25);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 70);
  SKILL_ADD(CLASS_REAVER, 1, 85);
  SKILL_ADD(CLASS_DREADLORD, 1, 85);
  SKILL_ADD(CLASS_AVENGER, 1, 85);
  SKILL_ADD(CLASS_WARRIOR, 1, 80);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 1, 100, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);

  // Deceiver Skills

  SKILL_CREATE("expedited retreat", SKILL_EXPEDITED_RETREAT, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ILLUSIONIST, 41, 80, SPEC_DECEIVER);

  /* Alchemist skills become epic skills for wipe 2008 -zion
  SKILL_CREATE("craft", SKILL_CRAFT, TAR_PHYS);
  SKILL_ADD(CLASS_ALCHEMIST, 21, 100);
  SKILL_CREATE("encrust", SKILL_ENCRUST, TAR_PHYS);
  SKILL_ADD(CLASS_ALCHEMIST, 51, 100);


  SKILL_CREATE("spellbind", SKILL_SPELLBIND, TAR_PHYS);
  SKILL_ADD(CLASS_ALCHEMIST, 31, 100);

  SKILL_CREATE("enchant", SKILL_ENCHANT, TAR_PHYS);
  SKILL_ADD(CLASS_ALCHEMIST, 56, 100);

  SKILL_CREATE("fix", SKILL_FIX, TAR_PHYS);
  SKILL_ADD(CLASS_ALCHEMIST, 11, 100);
  */
  SKILL_CREATE("flank", SKILL_FLANK, TAR_PHYS);
  SKILL_ADD(CLASS_DREADLORD, 11, 100);
  SKILL_ADD(CLASS_AVENGER, 11, 100);

  SKILL_CREATE_WITH_MESSAGES("gaze", SKILL_GAZE, TAR_PHYS,
                             "Feeling begins to return to your limbs.",
                             "Color seems to return to $n's face.");
  SKILL_ADD(CLASS_DREADLORD, 21, 100);
  SKILL_ADD(CLASS_AVENGER, 21, 100);

  SKILL_CREATE("tainted blade", SKILL_TAINTED_BLADE, TAR_PHYS);
  SKILL_ADD(CLASS_DREADLORD, 26, 100);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 30, 100, SPEC_VIOLATOR);

  SKILL_CREATE("holy blade", SKILL_HOLY_BLADE, TAR_PHYS);
  SKILL_ADD(CLASS_AVENGER, 26, 100);

  SKILL_CREATE("holy smite", SKILL_HOLY_SMITE, TAR_PHYS);
  SKILL_ADD(CLASS_AVENGER, 11, 100);

  SKILL_CREATE("battle senses", SKILL_BATTLE_SENSES, TAR_PHYS);
  SKILL_ADD(CLASS_DREADLORD, 11, 100);
  //SKILL_ADD(CLASS_AVENGER, 11, 100);

  SKILL_CREATE_WITH_MESSAGES("blood scent", SKILL_BLOOD_SCENT, TAR_PHYS,
                             "The scent of blood leaves what remains of your nostrils.",
                             "$n seems to sniff the air in futility.");
  SPEC_SKILL_ADD(CLASS_DREADLORD, 36, 100, SPEC_DEATHLORD);

  SKILL_CREATE("battle orders", SKILL_BATTLE_ORDERS, TAR_PHYS);
  SKILL_ADD(CLASS_DREADLORD, 51, 70);
  SKILL_ADD(CLASS_AVENGER, 51, 70);

  SKILL_CREATE("call of the grave", SKILL_CALL_GRAVE, TAR_PHYS);
  SKILL_ADD(CLASS_DREADLORD, 21, 100);

  SKILL_CREATE("dread wrath", SKILL_DREAD_WRATH, TAR_PHYS);
  SKILL_ADD(CLASS_DREADLORD, 56, 100);

  SKILL_CREATE("mix", SKILL_MIX, TAR_PHYS);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);

  /*
  SKILL_CREATE("smelt", SKILL_SMELT, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 1, 100, SPEC_BLACKSMITH);
  */
  SKILL_CREATE("throwpotion", SKILL_THROW_POTIONS, TAR_PHYS);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);

  SKILL_CREATE("remix", SKILL_REMIX, TAR_PHYS);
  SKILL_ADD(CLASS_ALCHEMIST, 51, 100);


  SKILL_CREATE("safe fall", SKILL_SAFE_FALL, TAR_PHYS);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);

  SKILL_CREATE("switch opponents", SKILL_SWITCH_OPPONENTS,
               TAR_PHYS);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 95);
  SKILL_ADD(CLASS_PALADIN, 51, 95);
  SKILL_ADD(CLASS_MERCENARY, 51, 50);
  SKILL_ADD(CLASS_ROGUE, 1, 50);
  SKILL_ADD(CLASS_THIEF, 1, 50);
  SKILL_ADD(CLASS_ASSASSIN, 1, 50);
  SKILL_ADD(CLASS_MONK, 1, 75);
  SKILL_ADD(CLASS_RANGER, 1, 85);
  SKILL_ADD(CLASS_REAVER, 1, 85);
  SKILL_ADD(CLASS_DREADLORD, 26, 90);
  SKILL_ADD(CLASS_AVENGER, 26, 90);
  SPEC_SKILL_ADD(CLASS_CLERIC, 30, 85, SPEC_ZEALOT);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 30, 100, SPEC_BATTLE_FORGER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 30, 100, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 50, SPEC_ASSASSIN);

  SKILL_CREATE("springleap", SKILL_SPRINGLEAP, TAR_PHYS);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_REAVER, 26, 80);

  SKILL_CREATE("martial arts", SKILL_MARTIAL_ARTS, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 1, 100);

  SKILL_CREATE("unarmed damage", SKILL_UNARMED_DAMAGE, TAR_PHYS);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 80);
  SKILL_ADD(CLASS_AVENGER, 1, 80);
  SKILL_ADD(CLASS_PSIONICIST, 36, 40);
  SKILL_ADD(CLASS_BARD, 1, 60);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  
  SKILL_CREATE("buddha palm", SKILL_BUDDHA_PALM, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 15, 100);

  SKILL_CREATE_WITH_MESSAGES("heroism", SKILL_HEROISM,
                             TAR_MENTAL,
                             "You no longer feel like a hero!", NULL);
  SKILL_ADD(CLASS_MONK, 1, 100);
  
  SKILL_CREATE_WITH_MESSAGES("diamond soul", SKILL_DIAMOND_SOUL,
                             TAR_MENTAL,
                             "You no longer feel resistant to magic!", NULL);
  SKILL_ADD(CLASS_MONK, 1, 100);

  SKILL_CREATE("chant", SKILL_CHANT, TAR_MENTAL);
  SKILL_ADD(CLASS_MONK, 1, 100);

  SKILL_CREATE("dragon punch", SKILL_DRAGON_PUNCH, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 1, 100);

  SKILL_CREATE("calm", SKILL_CALM, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 1, 100);

  // SKILL_CREATE("song of snatching", SONG_SNATCHING, TAR_MENTAL);
  // SKILL_ADD(CLASS_BARD, 51, 100);

  SKILL_CREATE("song of charming", SONG_CHARMING, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 36, 100);

  SKILL_CREATE("song of discord", SONG_DISCORD, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 16, 100);

  SKILL_CREATE("song of harmony", SONG_HARMONY, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 16, 100);

  SKILL_CREATE("song of storms", SONG_STORMS, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 41, 100);

  SKILL_CREATE("song of chaos", SONG_CHAOS, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 46, 100);

  SKILL_CREATE("song of dragons", SONG_DRAGONS, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 36, 100);

  SKILL_CREATE("song of sleep", SONG_SLEEP, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 6, 100);

  SKILL_CREATE("song of calming", SONG_CALMING, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 16, 100);

  SKILL_CREATE("song of healing", SONG_HEALING, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 31, 100);

  SKILL_CREATE("song of revelation", SONG_REVELATION, TAR_MENTAL);
  //SKILL_CREATE_WITH_MESSAGES("song of revelation", SONG_REVELATION,
  //                           TAR_MENTAL,
  //                           "Your senses are no longer enhanced.", NULL);
  SKILL_ADD(CLASS_BARD, 46, 100);

  SKILL_CREATE("song of harming", SONG_HARMING, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 36, 100);

  SKILL_CREATE("song of flight", SONG_FLIGHT, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 31, 100);

  SKILL_CREATE("song of protection", SONG_PROTECTION, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 21, 100);

  SKILL_CREATE_WITH_MESSAGES("song of heroism", SONG_HEROISM,
                             TAR_MENTAL,
                             "You no longer feel like a hero.", NULL);
  SKILL_ADD(CLASS_BARD, 41, 100);

  SKILL_CREATE("song of cowardice", SONG_COWARDICE, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 21, 100);

  SKILL_CREATE("song of forgetfulness", SONG_FORGETFULNESS,
               TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 21, 100);

  SKILL_CREATE("song of peace", SONG_PEACE, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 26, 100);
  
  SKILL_CREATE("song of dissonance", SONG_DISSONANCE, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 26, 100);

  SKILL_CREATE("song of drifting", SONG_DRIFTING, TAR_MENTAL);
  SPEC_SKILL_ADD(CLASS_BARD, 46, 100, SPEC_MINSTREL);

  SKILL_CREATE("flute", INSTRUMENT_FLUTE, TAR_PHYS);
  SKILL_ADD(CLASS_BARD, 5, 100);

  SKILL_CREATE("lyre", INSTRUMENT_LYRE, TAR_PHYS);
  SKILL_ADD(CLASS_BARD, 5, 100);

  SKILL_CREATE("mandolin", INSTRUMENT_MANDOLIN, TAR_PHYS);
  SKILL_ADD(CLASS_BARD, 5, 100);

  SKILL_CREATE("harp", INSTRUMENT_HARP, TAR_PHYS);
  SKILL_ADD(CLASS_BARD, 5, 100);

  SKILL_CREATE("drums", INSTRUMENT_DRUMS, TAR_PHYS);
  SKILL_ADD(CLASS_BARD, 5, 100);

  SKILL_CREATE("horn", INSTRUMENT_HORN, TAR_PHYS);
  SKILL_ADD(CLASS_BARD, 5, 100);

  SKILL_CREATE("shriek", SKILL_SHRIEK, 0);
  SPEC_SKILL_ADD(CLASS_BARD, 36, 100, SPEC_DISHARMONIST);

  SKILL_CREATE("legend lore", SKILL_LEGEND_LORE, TAR_MENTAL);
  SPEC_SKILL_ADD(CLASS_BARD, 36, 100, SPEC_MINSTREL);

  SKILL_CREATE("circle", SKILL_CIRCLE, TAR_PHYS);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SPEC_SKILL_ADD(CLASS_BARD, 31, 80, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);
// SPEC_SKILL_ADD(CLASS_ROGUE, 1, 10, SPEC_SWASHBUCKLER);

  SPELL_CREATE("airy water", SPELL_AIRY_WATER, PULSE_SPELLCAST * 4,
                TAR_IGNORE, spell_airy_water);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_SUMMONER, 7);

  SPELL_CREATE_MSG("globe of invulnerability", SPELL_GLOBE, PULSE_SPELLCAST * 3,
                TAR_CHAR_ROOM,
                spell_globe, "Your globe shimmers and fades into thin air.");
  SPELL_ADD(CLASS_CONJURER, 8);
  SPELL_ADD(CLASS_SUMMONER, 8);
  SPELL_ADD(CLASS_SORCERER, 12);

  SPELL_CREATE_MSG("group globe of invulnerability", SPELL_GROUP_GLOBE, PULSE_SPELLCAST * 7,
                TAR_IGNORE,
                spell_group_globe, "Your globe shimmers and fades into thin air.");
  SPELL_ADD(CLASS_CONJURER, 11);
  SPELL_ADD(CLASS_SUMMONER, 11);

  SKILL_CREATE("scribe", SKILL_SCRIBE, TAR_MENTAL);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SKILL_ADD(CLASS_THEURGIST, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 50);
  SKILL_ADD(CLASS_REAVER, 1, 50);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);


  SKILL_CREATE("quick chant", SKILL_QUICK_CHANT, TAR_MENTAL);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SKILL_ADD(CLASS_THEURGIST, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
//  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
//  SKILL_ADD(CLASS_DRUID, 1, 100); -- Druid's get auto-quick chant.  This will do nothing nor notch ever.
  SKILL_ADD(CLASS_RANGER, 1, 70);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_REAVER, 11, 90);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);


  SKILL_CREATE("clerical spell knowledge", SKILL_SPELL_KNOWLEDGE_CLERICAL,
               TAR_MENTAL);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 10, 80);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 80);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 100);
  SKILL_ADD(CLASS_RANGER, 10, 80);
  SKILL_ADD(CLASS_WARLOCK, 10, 80);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 80);
  SKILL_ADD(CLASS_AVENGER, 1, 75);

  SKILL_CREATE("sorcerous spell knowledge", SKILL_SPELL_KNOWLEDGE_MAGICAL,
               TAR_MENTAL);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 80);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SKILL_ADD(CLASS_THEURGIST, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
  SKILL_ADD(CLASS_RANGER, 10, 80);
  SKILL_ADD(CLASS_BARD, 10, 50);
  SKILL_ADD(CLASS_REAVER, 10, 80);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);

  SKILL_CREATE("shaman spell knowledge", SKILL_SPELL_KNOWLEDGE_SHAMAN,
               TAR_MENTAL);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);

/* This is an innate not a skill. wth?
  SKILL_CREATE("summon mount", SKILL_SUMMON_MOUNT, TAR_MENTAL);
  SKILL_ADD(CLASS_PALADIN, 8, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 8, 100);
*/

  SKILL_CREATE("1h bludgeon", SKILL_1H_BLUDGEON, TAR_PHYS);
  SKILL_ADD(CLASS_PSIONICIST, 1, 70);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 70);
  SKILL_ADD(CLASS_CLERIC, 1, 70);
  SKILL_ADD(CLASS_CONJURER, 1, 60);
  SKILL_ADD(CLASS_SUMMONER, 1, 60);
  SKILL_ADD(CLASS_DRUID, 1, 70);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_SORCERER, 1, 60);
  SKILL_ADD(CLASS_WARLOCK, 1, 70);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
 // SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 85);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 80); //Changed from 100 9/25/2008 Zion
  SKILL_ADD(CLASS_ALCHEMIST, 1, 90);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 70);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_CLERIC, 30, 100, SPEC_ZEALOT);
  SPEC_SKILL_ADD(CLASS_NECROMANCER, 30, 100, SPEC_NECROLYTE);
  SPEC_SKILL_ADD(CLASS_THEURGIST, 30, 100, SPEC_TEMPLAR);
  SPEC_SKILL_ADD(CLASS_REAVER, 30, 100, SPEC_ICE_REAVER);
  //SPEC_SKILL_ADD(CLASS_ROGUE, 30, 100, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_RANGER, 30, 100, SPEC_MARSHALL);
  
  SKILL_CREATE("1h slashing", SKILL_1H_SLASHING, TAR_PHYS);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 85);
  SKILL_ADD(CLASS_DRUID, 1, 80);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 70);
  SKILL_ADD(CLASS_MERCENARY, 1, 95);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  //SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);
  //SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_PSIONICIST, 21, 70);
  SKILL_ADD(CLASS_BARD, 1, 90);
  // Thieves get 1h slash skill for shortswords only.  Hardcoded in fight.c
  SPEC_SKILL_ADD(CLASS_ROGUE, 30, 70, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_NECROMANCER, 30, 90, SPEC_REAPER);
  SPEC_SKILL_ADD(CLASS_THEURGIST, 31,100, SPEC_THAUMATURGE);
  //SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);
  SPEC_SKILL_ADD(CLASS_REAVER, 30, 100, SPEC_SHOCK_REAVER);
  SPEC_SKILL_ADD(CLASS_REAVER, 30, 95, SPEC_FLAME_REAVER);

  SKILL_CREATE("1h piercing", SKILL_1H_PIERCING, TAR_PHYS);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_PSIONICIST, 1, 80);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 80);
  SKILL_ADD(CLASS_SUMMONER, 1, 80);
  SKILL_ADD(CLASS_MERCENARY, 1, 95);
  SKILL_ADD(CLASS_NECROMANCER, 1, 80);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 95);
  SKILL_ADD(CLASS_SORCERER, 1, 80);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 90); //Changed from 100 9/25/2008 Zion
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 90);
  SPEC_SKILL_ADD(CLASS_NECROMANCER, 30, 95, SPEC_REAPER);
  SPEC_SKILL_ADD(CLASS_THEURGIST, 31, 100, SPEC_THAUMATURGE);
  SPEC_SKILL_ADD(CLASS_REAVER, 30, 100, SPEC_SHOCK_REAVER);
  SPEC_SKILL_ADD(CLASS_BARD, 31, 100, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);

  SKILL_CREATE("1h flaying", SKILL_1H_FLAYING, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 95);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_CLERIC, 30, 95, SPEC_ZEALOT);
  SPEC_SKILL_ADD(CLASS_REAVER, 30, 100, SPEC_FLAME_REAVER);

  SKILL_CREATE("2h bludgeon", SKILL_2H_BLUDGEON, TAR_PHYS);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 95); 
  SKILL_ADD(CLASS_PALADIN, 1, 95); 
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 80);
  SKILL_ADD(CLASS_SORCERER, 1, 60);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 60);
  SKILL_ADD(CLASS_SUMMONER, 1, 60);
  SKILL_ADD(CLASS_PSIONICIST, 1, 60);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 90);
  SKILL_ADD(CLASS_BERSERKER, 1, 95);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 60);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_CLERIC, 30, 100, SPEC_ZEALOT);
  SPEC_SKILL_ADD(CLASS_NECROMANCER, 30, 90, SPEC_NECROLYTE);
  SPEC_SKILL_ADD(CLASS_THEURGIST, 30, 100, SPEC_TEMPLAR);
  SPEC_SKILL_ADD(CLASS_REAVER, 53, 95, SPEC_ICE_REAVER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_GUARDIAN);

  SKILL_CREATE("2h slashing", SKILL_2H_SLASHING, TAR_PHYS);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 95);
  SKILL_ADD(CLASS_PALADIN, 1, 95);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  //SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_GUARDIAN);
//  SPEC_SKILL_ADD(CLASS_REAVER, 51, 100, SPEC_FLAME_REAVER);

  SKILL_CREATE("2h flaying", SKILL_2H_FLAYING, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 1, 95, SPEC_RAGELORD);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_REAVER, 53, 100, SPEC_FLAME_REAVER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_GUARDIAN);

  SKILL_CREATE("ranged weapons", SKILL_ARCHERY, TAR_PHYS);  /* TASFALEN */
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 75);
  SKILL_ADD(CLASS_ROGUE, 1, 75);
  SKILL_ADD(CLASS_ASSASSIN, 1, 75);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 75, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 75, SPEC_ASSASSIN);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_SHARPSHOOTER);

  SKILL_CREATE("reach weapons", SKILL_REACH_WEAPONS, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 30, 90);
  SKILL_ADD(CLASS_WARRIOR, 25, 80);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);

  SKILL_CREATE("blindfighting", SKILL_BLINDFIGHTING, TAR_PHYS);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
  SKILL_ADD(CLASS_PALADIN, 1, 90);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 85);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);
  SKILL_ADD(CLASS_MONK, 1, 80);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_BERSERKER, 6, 40);
  SKILL_ADD(CLASS_REAVER, 11, 85);

  SPELL_CREATE("embalm", SPELL_EMBALM, PULSE_SPELLCAST * 3,
               TAR_OBJ_ROOM | TAR_NOCOMBAT, spell_embalm);
  SPELL_ADD(CLASS_NECROMANCER, 3);
  SPELL_ADD(CLASS_THEURGIST, 3);

  SPELL_CREATE("mass embalm", SPELL_MASS_EMBALM, PULSE_SPELLCAST * 3,
                TAR_IGNORE | TAR_NOCOMBAT, spell_mass_embalm);
  SPELL_ADD(CLASS_NECROMANCER, 10);
  SPELL_ADD(CLASS_THEURGIST, 10);

  SPELL_CREATE_MSG("angelic countenance", SPELL_ANGELIC_COUNTENANCE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_vampire, "The holy shine about you dimishes.");
  SPELL_ADD(CLASS_THEURGIST, 10);

  SPELL_CREATE_MSG("vampiric trance", SPELL_VAMPIRE, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_vampire, "Color fades slowly into your face.");
  SPELL_ADD(CLASS_NECROMANCER, 10);

  SPELL_CREATE_MSG("divine blessing", SPELL_DIVINE_BLESSING, PULSE_SPELLCAST * 3,
                TAR_SELF_ONLY,
                spell_divine_blessing, "&+WYou feel detached from your weapon.");
 SPEC_SPELL_ADD(CLASS_PALADIN, 9, SPEC_CRUSADER);

  SPELL_CREATE_MSG("elemental form", SPELL_ELEMENTAL_FORM, PULSE_SPELLCAST * 4,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_elemental_form, "&+BYou return to your normal form.&N");

  SPELL_CREATE_MSG("elemental aura", SPELL_ELEMENTAL_AURA, PULSE_SPELLCAST * 4,
                TAR_SELF_ONLY | TAR_NOCOMBAT,
                spell_elemental_aura, "&+BYour aura returns to normal.&N");
  SPELL_ADD(CLASS_DRUID, 11);
  SPELL_ADD(CLASS_BLIGHTER, 11);
  SPEC_SPELL_ADD(CLASS_SUMMONER, 10, SPEC_MENTALIST);
 //SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_STORM);

  SPELL_CREATE("wind tunnel", SPELL_WIND_TUNNEL,
               PULSE_SPELLCAST * 1, TAR_IGNORE | TAR_NOCOMBAT | TAR_ELEMENTAL,
               spell_wind_tunnel);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_TEMPESTMAGUS);     
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_STORM);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 9, SPEC_ELEMENTALIST);

  SPELL_CREATE("binding wind", SPELL_BINDING_WIND, PULSE_SPELLCAST * 1,
                TAR_IGNORE | TAR_NOCOMBAT | TAR_ELEMENTAL,
                spell_binding_wind);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_TEMPESTMAGUS);
  SPEC_SPELL_ADD(CLASS_DRUID, 10, SPEC_STORM);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_ELEMENTALIST);

  SPELL_CREATE_MSG("vampiric touch", SPELL_VAMPIRIC_TOUCH, PULSE_SPELLCAST * 2,
                TAR_SELF_ONLY,
                spell_vampiric_touch, "Your hands cease to glow &+Rred.&n");
  SPELL_ADD(CLASS_NECROMANCER, 2);
  SPELL_ADD(CLASS_THEURGIST, 2);
  SPELL_ADD(CLASS_BLIGHTER, 4);

  SPELL_CREATE("protect undead", SPELL_PROT_UNDEAD, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_prot_undead);
  SPELL_ADD(CLASS_NECROMANCER, 7);

  SPELL_CREATE("protect soul", SPELL_PROTECT_SOUL, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM, spell_prot_undead);
  SPELL_ADD(CLASS_THEURGIST, 7);
  
  SPELL_CREATE_MSG("protection from undead", SPELL_PROT_FROM_UNDEAD, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM | TAR_NOCOMBAT,
                spell_prot_from_undead, "Your field of &+Yliving energy&n dissipates&n.");
  SPELL_ADD(CLASS_NECROMANCER, 1);
  SPELL_ADD(CLASS_THEURGIST, 1);
  SPEC_SPELL_ADD(CLASS_CLERIC, 7, SPEC_ZEALOT);
  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_HOLYMAN);

  SPELL_CREATE("command horde", SPELL_COMMAND_HORDE, 0,
                TAR_IGNORE, spell_command_horde);
  // SPELL_ADD(CLASS_NECROMANCER, 9);

  SPELL_CREATE("heal undead", SPELL_HEAL_UNDEAD, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM, spell_heal_undead);
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE("mend soul", SPELL_MEND_SOUL, PULSE_SPELLCAST * 3 / 2,
                TAR_CHAR_ROOM, spell_mend_soul);
  SPELL_ADD(CLASS_THEURGIST, 5);

  SPELL_CREATE("greater heal undead", SPELL_GREATER_HEAL_UNDEAD, PULSE_SPELLCAST * 2,
                TAR_CHAR_ROOM,
                spell_greater_heal_undead);
  SPELL_ADD(CLASS_WARLOCK, 7);

  SPELL_CREATE_MSG("entangle", SPELL_ENTANGLE, PULSE_SPELLCAST * 1,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_entangle, "You are able to move normally again.");
//  SPELL_ADD(CLASS_DRUID, 8);
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_WOODLAND);
  SPEC_SPELL_ADD(CLASS_BLIGHTER, 8, SPEC_SCOURGE);


  SPELL_CREATE("create spring", SPELL_CREATE_SPRING, PULSE_SPELLCAST * 2,
                TAR_IGNORE, spell_create_spring);
  SPELL_ADD(CLASS_DRUID, 4);

  SPELL_CREATE("create pond", SPELL_CREATE_POND, PULSE_SPELLCAST * 2,
                TAR_IGNORE, spell_create_pond);
  SPELL_ADD(CLASS_BLIGHTER, 4);

  SPELL_CREATE_MSG("barkskin", SPELL_BARKSKIN, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_barkskin, "Your skin loses its &+ybarklike &ntexture.");
  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_RANGER, 5);

  SPELL_CREATE_MSG("thornskin", SPELL_THORNSKIN, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_thornskin, "Your skin loses its &+ythorny&n texture.");
  SPELL_ADD(CLASS_BLIGHTER, 1);

  SPELL_CREATE("flame sphere", SPELL_FLAME_SPHERE, PULSE_SPELLCAST * 2/3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2 | TAR_AGGRO, spell_flame_sphere);
  SPELL_ADD(CLASS_BLIGHTER, 4);

  SPELL_CREATE("blight", SPELL_BLIGHT, PULSE_SPELLCAST * 2/3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_blight);
  SPELL_ADD(CLASS_BLIGHTER, 5);

  SPELL_CREATE_MSG("mass barkskin", SPELL_MASS_BARKSKIN, PULSE_SPELLCAST * 3,
                TAR_IGNORE,
                spell_mass_barkskin, "Your skin loses its &+ybarklike &ntexture.");
//  SPELL_ADD(CLASS_DRUID, 10);
//  SPELL_ADD(CLASS_RANGER, 11);

  SPELL_CREATE_MSG( "curse of yzar", SPELL_CURSE_OF_YZAR, PULSE_SPELLCAST,
                TAR_CHAR_ROOM,
                spell_curse_of_yzar, "You &+Wscream&n in &+Rpain&n as the &+Yflesh&n falls from your bones.");

  SPELL_CREATE( "rested", SPELL_REST, PULSE_SPELLCAST,
                TAR_CHAR_ROOM, spell_rest );
  // Immortal only spell

  SPELL_CREATE("moonwell", SPELL_MOONWELL, PULSE_SPELLCAST * 7,
                TAR_CHAR_WORLD | TAR_NOCOMBAT, spell_moonwell);
  SPELL_ADD(CLASS_DRUID, 10);

  SPELL_CREATE("shadow gate", SPELL_SHADOW_GATE, PULSE_SPELLCAST * 7,
                TAR_CHAR_WORLD | TAR_NOCOMBAT, spell_shadow_gate);
  SPELL_ADD(CLASS_WARLOCK, 10);
  SPELL_ADD(CLASS_BLIGHTER, 10);

  SPELL_CREATE("moonstone", SPELL_MOONSTONE, PULSE_SPELLCAST * 6,
                TAR_IGNORE | TAR_NOCOMBAT, spell_moonstone);
  SPELL_ADD(CLASS_DRUID, 12);

  SPELL_CREATE("bloodstone", SPELL_BLOODSTONE, PULSE_SPELLCAST * 6,
                TAR_IGNORE | TAR_NOCOMBAT, spell_bloodstone);
  SPELL_ADD(CLASS_BLIGHTER, 12);

  SPELL_CREATE("create dracolich", SPELL_CREATE_DRACOLICH, PULSE_SPELLCAST * 9,
                TAR_OBJ_ROOM | TAR_NOCOMBAT,
                spell_create_dracolich);
  SPELL_ADD(CLASS_NECROMANCER, 11);

  SPELL_CREATE("call titan", SPELL_CALL_TITAN, PULSE_SPELLCAST * 9,
                TAR_OBJ_ROOM | TAR_NOCOMBAT,
		spell_call_titan);
  SPELL_ADD(CLASS_THEURGIST, 11);

  SPELL_CREATE("napalm", SPELL_NAPALM, PULSE_SPELLCAST * 4 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_napalm);

  SPELL_CREATE("strong acid", SPELL_STRONG_ACID, PULSE_SPELLCAST * 3/2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_strong_acid);

  SPELL_CREATE("glass bomb", SPELL_GLASS_BOMB, PULSE_SPELLCAST * 3/2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_glass_bomb);

  SPELL_CREATE("grease", SPELL_GREASE, PULSE_SPELLCAST * 3/2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_grease);

  SPELL_CREATE("nitrogen", SPELL_NITROGEN, PULSE_SPELLCAST * 3/2,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_nitrogen);

  SKILL_CREATE("sneak", SKILL_SNEAK, TAR_PHYS);
  SKILL_ADD(CLASS_ROGUE, 1, 70);
  SKILL_ADD(CLASS_ASSASSIN, 1, 70);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 70, SPEC_ASSASSIN);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 41, 60, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 35, 60, SPEC_BRIGAND);

  SKILL_CREATE("hide", SKILL_HIDE, 0);
  SKILL_ADD(CLASS_ROGUE, 1, 85);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_ASSASSIN);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_THIEF);
  SKILL_ADD(CLASS_MERCENARY, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SPEC_SKILL_ADD(CLASS_RANGER, 41, 80, SPEC_HUNTSMAN);
  SPEC_SKILL_ADD(CLASS_BARD, 1, 90, SPEC_SCOUNDREL);

  SKILL_CREATE("steal", SKILL_STEAL, TAR_PHYS);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_THIEF);


  SKILL_CREATE("evade", SKILL_EVADE, TAR_PHYS);
  SKILL_ADD(CLASS_THIEF, 46, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 46, 100, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 41, 25, SPEC_SWASHBUCKLER);

  SKILL_CREATE("backstab", SKILL_BACKSTAB, TAR_PHYS);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_MERCENARY, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SPEC_SKILL_ADD(CLASS_BARD, 31, 95, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_THIEF);

  SKILL_CREATE("garrote", SKILL_GARROTE, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 30, 100, SPEC_ASSASSIN);
  SKILL_ADD(CLASS_ASSASSIN, 20, 100);

  SKILL_CREATE("critical stab", SKILL_CRITICAL_STAB, TAR_PHYS);
  SKILL_ADD(CLASS_ASSASSIN, 51, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 100, SPEC_ASSASSIN);

  SKILL_CREATE("shadowstep", SKILL_SHADOWSTEP, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 41, 100, SPEC_THIEF);

  SKILL_CREATE("pick lock", SKILL_PICK_LOCK, TAR_PHYS);
  SKILL_ADD(CLASS_ROGUE, 1, 60);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_THIEF);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 60);

  SKILL_CREATE("kick", SKILL_KICK, TAR_PHYS);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 90);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_PALADIN, 1, 90);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 60);
  SKILL_ADD(CLASS_DREADLORD, 1, 80);
  SKILL_ADD(CLASS_AVENGER, 1, 80);
  SKILL_ADD(CLASS_SORCERER, 1, 40);
  SKILL_ADD(CLASS_CONJURER, 1, 40);
  SKILL_ADD(CLASS_SUMMONER, 1, 40);
  SKILL_ADD(CLASS_BARD, 1, 40);
  SKILL_ADD(CLASS_SHAMAN, 1, 40);
  SKILL_ADD(CLASS_CLERIC, 1, 40);
  SKILL_ADD(CLASS_PSIONICIST, 1, 40);
  SKILL_ADD(CLASS_ROGUE, 1, 40);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 40);
  SKILL_ADD(CLASS_NECROMANCER, 1, 40);
//  SKILL_ADD(CLASS_CABALIST, 1, 40);
  SKILL_ADD(CLASS_THEURGIST, 1, 40);
  SKILL_ADD(CLASS_DRUID, 1, 40);
  SKILL_ADD(CLASS_BLIGHTER, 1, 40);
  SKILL_ADD(CLASS_REAVER, 1, 40);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 40);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 40);

  SKILL_CREATE("roundkick", SKILL_ROUNDKICK, TAR_PHYS);
  SKILL_ADD(CLASS_MONK, 1, 100);

  SKILL_CREATE_WITH_MESSAGES("throat crush", SKILL_THROAT_CRUSH, TAR_PHYS,
   "Your throat ceases to hurt; you feel your beautiful(?) voice returning!&n", NULL);
  //SKILL_CREATE("throat crush", SKILL_THROAT_CRUSH, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 51, 100);

// Creating this bogus skill so only the person being throat crushed receives the wear off message.
//   This needs a good name for 'stat c George' where George is on cooldown.
  SKILL_CREATE("throat crush cooldown.", SKILL_THROAT_CRUSHER, TAR_PHYS);
  // Added this so we can see what the affect is (same as above).
  SKILL_CREATE("charge cooldown.", SKILL_CHARGE, TAR_PHYS);

  SKILL_CREATE("guard", SKILL_GUARD, TAR_PHYS);
  //SKILL_ADD(CLASS_ANTIPALADIN, 20, 90);
  //SKILL_ADD(CLASS_PALADIN, 20, 90);
  SKILL_ADD(CLASS_WARRIOR, 41, 70);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 31, 100, SPEC_GUARDIAN);
  SKILL_ADD(CLASS_ANTIPALADIN, 20, 90);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 0, 0, SPEC_VIOLATOR);
  SKILL_ADD(CLASS_PALADIN, 20, 90);

  SKILL_CREATE("cleave", SKILL_CLEAVE, TAR_PHYS);
  SKILL_ADD(CLASS_PALADIN, 36, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 36, 100);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 0, 0, SPEC_VIOLATOR);

  SKILL_CREATE("bash", SKILL_BASH, TAR_PHYS);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 1, 70, SPEC_SWORDSMAN);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 0, 0, SPEC_VIOLATOR);

  SKILL_CREATE( "groundfighting", SKILL_GROUNDFIGHTING, TAR_PHYS );

  SKILL_CREATE("mounted combat", SKILL_MOUNTED_COMBAT, TAR_PHYS);
  SKILL_ADD(CLASS_PALADIN, 10, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 100);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 0, 0, SPEC_VIOLATOR);

  SKILL_CREATE("rescue", SKILL_RESCUE, TAR_PHYS);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 31, 100);
   SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 0, 0, SPEC_VIOLATOR);
  SKILL_ADD(CLASS_RANGER, 10, 80);
  SKILL_ADD(CLASS_MERCENARY, 10, 80);


  SKILL_CREATE("trap", SKILL_TRAP, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_RANGER, 31, 100, SPEC_HUNTSMAN);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 51, 100, SPEC_BOUNTY);
  SKILL_ADD(CLASS_THIEF, 51, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 100, SPEC_THIEF);

  SKILL_CREATE("track", SKILL_TRACK, TAR_MENTAL);
  SKILL_ADD(CLASS_RANGER, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 95);
  SKILL_ADD(CLASS_ASSASSIN, 1, 95);
  SKILL_ADD(CLASS_ROGUE, 1, 75);
  SKILL_ADD(CLASS_REAVER, 1, 55);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 1, 100, SPEC_BOUNTY);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);
  SPEC_SKILL_ADD(CLASS_RANGER, 1, 100, SPEC_HUNTSMAN);

  SKILL_CREATE("listen", SKILL_LISTEN, TAR_MENTAL);
  SKILL_ADD(CLASS_THIEF, 15, 100);
  SKILL_ADD(CLASS_BARD, 15, 80);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 30, 80, SPEC_BOUNTY);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 40, 90, SPEC_BRIGAND);
  SPEC_SKILL_ADD(CLASS_ROGUE, 30, 100, SPEC_THIEF);

  SKILL_CREATE("disarm", SKILL_DISARM, TAR_PHYS);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);


  SKILL_CREATE("double attack", SKILL_DOUBLE_ATTACK, 0);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 90);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 90);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 95);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_THIEF, 1, 85);
  SKILL_ADD(CLASS_ASSASSIN, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 80);
  SKILL_ADD(CLASS_DRUID, 1, 75);
  SKILL_ADD(CLASS_ALCHEMIST, 11, 75);
  SKILL_ADD(CLASS_BERSERKER, 11, 100);
  SKILL_ADD(CLASS_REAVER, 1, 95);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 60);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 1, 100, SPEC_VIOLATOR);
  SPEC_SKILL_ADD(CLASS_CLERIC, 51, 90, SPEC_ZEALOT);
  SPEC_SKILL_ADD(CLASS_BARD, 31, 95, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_ASSASSIN);

  SKILL_CREATE("triple attack", SKILL_TRIPLE_ATTACK, 0);
  SKILL_ADD(CLASS_RANGER, 51, 95);
  SKILL_ADD(CLASS_REAVER, 51, 80);
  SKILL_ADD(CLASS_MERCENARY, 53, 80);
  SKILL_ADD(CLASS_BERSERKER, 51, 80);
  SKILL_ADD(CLASS_PALADIN, 56, 90);
  SKILL_ADD(CLASS_ANTIPALADIN, 56, 90);
  SKILL_ADD(CLASS_DREADLORD, 46, 100);
  SKILL_ADD(CLASS_AVENGER, 46, 90);
  SKILL_ADD(CLASS_WARRIOR, 56, 90);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 51, 90, SPEC_VIOLATOR);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 51, 70, SPEC_BATTLE_FORGER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 41, 100, SPEC_SWORDSMAN);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 46, 100, SPEC_GUARDIAN);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 46, 70, SPEC_SWASHBUCKLER);

  SKILL_CREATE("dual wield", SKILL_DUAL_WIELD, TAR_PHYS);
// Bards need an instrucment, so they wont ever dual, removing this - gellz
//  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 80);
  SKILL_ADD(CLASS_ROGUE, 1, 75);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 11, 90);
  SKILL_ADD(CLASS_REAVER, 1, 80);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 30, 100, SPEC_RAGELORD);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 31, 100, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_GUARDIAN);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_ASSASSIN);

  SKILL_CREATE("hitall", SKILL_HITALL, TAR_PHYS);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 11, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 60);
  SKILL_ADD(CLASS_AVENGER, 1, 60);

  SKILL_CREATE("tackle", SKILL_TACKLE, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 20, 100);

  SKILL_CREATE("legsweep", SKILL_LEGSWEEP, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 30, 100, SPEC_SWASHBUCKLER);

  SKILL_CREATE_WITH_MESSAGES("berserk", SKILL_BERSERK, TAR_PHYS,
                             "Your blood cools, and you no longer see targets everywhere.",
                             "$n seems to have overcome $s battle madness.");
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 0, 0, SPEC_SWASHBUCKLER);

  SKILL_CREATE_WITH_MESSAGES("rage", SKILL_RAGE, TAR_PHYS,
                             "You feel yourself return to normal as your rage abates.",
                             "$n returns to normal as $s rage abates.");
  SKILL_ADD(CLASS_BERSERKER, 6, 100);

  SKILL_CREATE_WITH_MESSAGES("infuriate", SKILL_INFURIATE, TAR_PHYS,
    "&+RAs your R&+ra&+RG&+re&+R fades away, you return to your normal size.&n",
    NULL);
  SKILL_ADD(CLASS_BERSERKER, 41, 100);

  SKILL_CREATE("maul", SKILL_MAUL, TAR_PHYS);
  SKILL_ADD(CLASS_BERSERKER, 31, 100);

  SKILL_CREATE("vicious strike", SKILL_VICIOUS_STRIKE, TAR_PHYS);
  SKILL_ADD(CLASS_BERSERKER, 51, 100);

  SKILL_CREATE("rush", SKILL_RUSH, TAR_PHYS);
  SKILL_ADD(CLASS_BERSERKER, 51, 100);

  SKILL_CREATE("rampage", SKILL_RAMPAGE, TAR_PHYS);
  SKILL_ADD(CLASS_BERSERKER, 36, 100);

  SKILL_CREATE("shield block", SKILL_SHIELD_BLOCK, TAR_PHYS);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 60);
  SKILL_ADD(CLASS_CLERIC, 1, 60);
  SKILL_ADD(CLASS_BARD, 1, 60);
//  SKILL_ADD(CLASS_REAVER, 1, 60);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 70);
  SPEC_SKILL_ADD(CLASS_CLERIC, 30, 90, SPEC_HOLYMAN);
  SPEC_SKILL_ADD(CLASS_ROGUE, 25, 80, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_DRUID, 31, 60, SPEC_WOODLAND);
  SPEC_SKILL_ADD(CLASS_DRUID, 25, 80, SPEC_STORM);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 1, 0, SPEC_SWASHBUCKLER);

  SKILL_CREATE("parry", SKILL_PARRY, TAR_PHYS);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 80);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 95);
  SKILL_ADD(CLASS_ROGUE, 20, 80);
  SKILL_ADD(CLASS_THIEF, 20, 70);
  SKILL_ADD(CLASS_ASSASSIN, 36, 60);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 70);
  SKILL_ADD(CLASS_BERSERKER, 1, 60);
  SKILL_ADD(CLASS_REAVER, 18, 80);
  SKILL_ADD(CLASS_DREADLORD, 1, 70);
  SKILL_ADD(CLASS_AVENGER, 1, 80);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 1, 100, SPEC_SWASHBUCKLER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 60, SPEC_ASSASSIN);

  SKILL_CREATE("riposte", SKILL_RIPOSTE, TAR_PHYS);
  SKILL_ADD(CLASS_PALADIN, 1, 75);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 75);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 60);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 6, 50);
  SKILL_ADD(CLASS_REAVER, 26, 70);
  SKILL_ADD(CLASS_DREADLORD, 1, 60);
  SKILL_ADD(CLASS_AVENGER, 1, 60);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 36, 70, SPEC_BATTLE_FORGER);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 1, 100, SPEC_SWASHBUCKLER);

  SKILL_CREATE("surprise", SKILL_SURPRISE, 0);
  SKILL_ADD(CLASS_RANGER, 1, 100);

  SKILL_CREATE("headbutt", SKILL_HEADBUTT, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);

  SKILL_CREATE("double headbutt", SKILL_DOUBLE_HEADBUTT, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 56, 100);

  SKILL_CREATE("meditate", SKILL_MEDITATE, TAR_MENTAL);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 80);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 75);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
//  SKILL_ADD(CLASS_DRUID, 1, 95);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
//  SKILL_ADD(CLASS_CABALIST, 1, 100);
  SKILL_ADD(CLASS_THEURGIST, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 95);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 21, 80);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 85);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);

  SKILL_CREATE("apply poison", SKILL_APPLY_POISON, 0);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 40, 100, SPEC_ASSASSIN);

  SKILL_CREATE("concentration", SKILL_CONCENTRATION, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 1, 100);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 100);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 100);

  SKILL_CREATE("dodge", SKILL_DODGE, TAR_PHYS);
//  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  /*
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_PSIONICIST, 1, 70);
  SKILL_ADD(CLASS_CLERIC, 1, 60);
  SKILL_ADD(CLASS_CONJURER, 1, 50);
  SKILL_ADD(CLASS_SUMMONER, 1, 50);
  SKILL_ADD(CLASS_DRUID, 1, 85);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 50);
  SKILL_ADD(CLASS_THEURGIST, 1, 50);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_RANGER, 1, 85);
  SKILL_ADD(CLASS_SHAMAN, 1, 50);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 50);
  SKILL_ADD(CLASS_SORCERER, 1, 50);
  SKILL_ADD(CLASS_WARLOCK, 1, 70);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_THIEF, 1, 95);
  SKILL_ADD(CLASS_WARRIOR, 1, 75);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 75);
  SKILL_ADD(CLASS_BERSERKER, 1, 60);
  SKILL_ADD(CLASS_REAVER, 1, 65);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 50);
  SKILL_ADD(CLASS_DREADLORD, 1, 85);
  SKILL_ADD(CLASS_AVENGER, 1, 75);
  SPEC_SKILL_ADD(CLASS_BARD, 1, 80, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_CLERIC, 1, 80,SPEC_ZEALOT);
  SKILL_ADD(CLASS_ASSASSIN, 1, 80);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 80, SPEC_ASSASSIN);
  */

  SKILL_CREATE("mount", SKILL_MOUNT, TAR_PHYS);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_RANGER, 1, 90);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
  SKILL_ADD(CLASS_PSIONICIST, 1, 90);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 90);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_WARLOCK, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_SUMMONER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_MONK, 1, 90);
  SKILL_ADD(CLASS_THEURGIST, 1, 90);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 70);
  SKILL_ADD(CLASS_REAVER, 1, 90);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 90);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_BERSERKER, 1, 90);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_ASSASSIN);

  SKILL_CREATE("bandage", SKILL_BANDAGE, TAR_MENTAL);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 70);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
  SKILL_ADD(CLASS_SORCERER, 1, 60);
  SKILL_ADD(CLASS_WARLOCK, 1, 60);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 60);
  SKILL_ADD(CLASS_SUMMONER, 1, 60);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_THIEF, 1, 80);
  SKILL_ADD(CLASS_SHAMAN, 1, 95);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 75);
  SKILL_ADD(CLASS_PSIONICIST, 1, 60);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 60);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 80);
  SKILL_ADD(CLASS_BERSERKER, 1, 50);
  SKILL_ADD(CLASS_REAVER, 1, 60);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 60);
  SKILL_ADD(CLASS_AVENGER, 1, 90);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 80, SPEC_ASSASSIN);

/*
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
  SKILL_ADD(CLASS_THEURGIST, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 100);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);
*/


  SKILL_CREATE("fishing", SKILL_FISHING, TAR_MENTAL);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_SUMMONER, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
  SKILL_ADD(CLASS_THEURGIST, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 100);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);


  SKILL_CREATE("carve", SKILL_CARVE, TAR_PHYS);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 40);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 90);
  SKILL_ADD(CLASS_WARRIOR, 1, 60);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 60);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 60);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 60);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 60);
  SKILL_ADD(CLASS_PSIONICIST, 1, 60);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 60);
  SKILL_ADD(CLASS_CONJURER, 1, 60);
  SKILL_ADD(CLASS_SUMMONER, 1, 60);
  SKILL_ADD(CLASS_NECROMANCER, 1, 60);
  SKILL_ADD(CLASS_THEURGIST, 1, 60);
  SKILL_ADD(CLASS_SORCERER, 1, 60);
  SKILL_ADD(CLASS_WARLOCK, 1, 60);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 85);
  SKILL_ADD(CLASS_BERSERKER, 1, 60);
  SKILL_ADD(CLASS_REAVER, 1, 60);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 60);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 60);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);

  SKILL_CREATE("swim", SKILL_SWIM, TAR_PHYS);
  SKILL_ADD(CLASS_BLIGHTER, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_RANGER, 1, 90);
  SKILL_ADD(CLASS_PALADIN, 1, 90);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 90);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_WARLOCK, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_SUMMONER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_THEURGIST, 1, 90);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 90);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_PSIONICIST, 1, 90);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 90);
  SKILL_ADD(CLASS_MONK, 1, 90);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 90);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 90);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_ASSASSIN);
  
  SKILL_CREATE("awareness", SKILL_AWARENESS, TAR_MENTAL);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_THIEF, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_ASSASSIN, 1, 70);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 70, SPEC_ASSASSIN);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 1, 80, SPEC_SWASHBUCKLER);

 // SKILL_CREATE("capture", SKILL_CAPTURE, TAR_PHYS);  /* TASFALEN */
 // SKILL_ADD(CLASS_MERCENARY, 1, 100);    /* TASFALEN */

  SKILL_CREATE("appraise", SKILL_APPRAISE, TAR_MENTAL);
  SKILL_ADD(CLASS_ROGUE, 10, 90);
  SKILL_ADD(CLASS_THIEF, 10, 90);
  SKILL_ADD(CLASS_MERCENARY, 20, 80);
  SKILL_ADD(CLASS_ASSASSIN, 20, 900);
  SKILL_ADD(CLASS_BARD, 10, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_ASSASSIN);

  SKILL_CREATE("lore", SKILL_LORE, TAR_MENTAL);
  SKILL_ADD(CLASS_BARD, 10, 75);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);

  SKILL_CREATE("age corpse", SKILL_AGE_CORPSE, TAR_MENTAL);
  SKILL_ADD(CLASS_NECROMANCER, 20, 100);
  SKILL_ADD(CLASS_THEURGIST, 20, 100);
  SKILL_ADD(CLASS_CLERIC, 40, 75);

  SKILL_CREATE("slip", SKILL_SLIP, TAR_PHYS);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_THIEF);

  SKILL_CREATE("bearhug", SKILL_BEARHUG, TAR_PHYS);
  //SKILL_ADD(CLASS_MERCENARY, 25, 100);

/*  SKILL_CREATE("headlock", SKILL_HEADLOCK, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 35, 100);

  SKILL_CREATE("armlock", SKILL_ARMLOCK, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 50, 100);

  SKILL_CREATE("groundslam", SKILL_GROUNDSLAM, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 40, 100);

  SKILL_CREATE("leglock", SKILL_LEGLOCK, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 45, 100);

  SKILL_CREATE("grappler combat", SKILL_GRAPPLER_COMBAT, TAR_PHYS);
  SKILL_ADD(CLASS_MERCENARY, 25, 100);*/

  SPELL_CREATE("summon ghasts", SPELL_SUMMON_GHASTS, PULSE_SPELLCAST * 2,
              TAR_AREA | TAR_AGGRO, spell_summon_ghasts);
  SPELL_ADD(CLASS_NECROMANCER, 11);

  SPELL_CREATE("aid of the heavens", SPELL_AID_OF_THE_HEAVENS, PULSE_SPELLCAST * 2,
              TAR_AREA | TAR_AGGRO, spell_aid_of_the_heavens);
  SPELL_ADD(CLASS_THEURGIST, 11);

  SPELL_CREATE("ghastly touch", SPELL_GHASTLY_TOUCH, PULSE_SPELLCAST * 2 / 3,
                TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO,
                spell_ghastly_touch);
#ifndef _DE_
  create_epic_skills();
#endif
  create_poisons();
  create_tags();
}

void create_poisons()
{
  POISON_CREATE("lifeleak", POISON_LIFELEAK, poison_lifeleak);
  POISON_CREATE("weakness", POISON_WEAKNESS, poison_weakness);
  POISON_CREATE("neurotoxin", POISON_NEUROTOXIN, poison_neurotoxin);
  POISON_CREATE("heart toxin", POISON_HEART_TOXIN, poison_heart_toxin);
}

void create_tags()
{
  TAG_CREATE( "stoned", HERB_SMOKED );
  TAG_CREATE( "ocularius", HERB_OCULARIUS );
  TAG_CREATE( "blue haze", HERB_BLUE_HAZE );
  TAG_CREATE( "medicus", HERB_MEDICUS );
  TAG_CREATE( "black kush", HERB_BLACK_KUSH );
  TAG_CREATE( "gootwiet", HERB_GOOTWIET );

  TAG_CREATE( "nchat spammer", TAG_NCHATSPAMMER );

  TAG_CREATE("decay", TAG_OBJ_DECAY);
  TAG_CREATE("alt_extra2", TAG_ALTERED_EXTRA2);
  TAG_CREATE("no misfire", TAG_NOMISFIRE);
  TAG_CREATE_WITH_MESSAGES("witch spell", TAG_WITCHSPELL,
                           "&+GYou feel somehow weaker.&n",
                           "");
  TAG_CREATE("racial skills (deprecated)", TAG_RACIAL_SKILLS);
  TAG_CREATE("soulbind", TAG_SOULBIND);

/*  ACHIEVEMENT TAGS - FORMAT: ACH_XXXX (completed), AIP_XXXX (Achievement In Progress) */
  //PVP
  TAG_CREATE("ach - unstoppable", ACH_UNSTOPPABLE);  //static check, 40 frags
  TAG_CREATE("ach - lets get dirty", ACH_LETSGETDIRTY); //static, 1.0 frags
  TAG_CREATE("ach - serial killer", ACH_SERIALKILLER); //static, 10.00 frags

  //PVE
  TAG_CREATE("aip - level achievement", AIP_LEVELACHIEVEMENT); //static, gain 1.0 levels
  TAG_CREATE("aip - free sloop", AIP_FREESLOOP);
  TAG_CREATE("aip - cargo count", AIP_CARGOCOUNT);
  TAG_CREATE("aip - arachnophobia", AIP_ARACHNOPHOBIA);
  TAG_CREATE("ach - arachnophobia", ACH_ARACHNOPHOBIA);
  TAG_CREATE("aip - Trollin", AIP_TROLLIN);
  TAG_CREATE("ach - trollin", ACH_TROLLIN);
  TAG_CREATE("aip - moo juice", AIP_MOOJUICE);
  TAG_CREATE("ach - moo juice", ACH_MOOJUICE);
  TAG_CREATE("aip - dragonslayer", AIP_DRAGONSLAYER);
  TAG_CREATE("ach - dragonslayer", ACH_DRAGONSLAYER);
  TAG_CREATE("aip - mayihealsyou", AIP_MAYIHEALSYOU);
  TAG_CREATE("ach - mayihealsyou", ACH_MAYIHEALSYOU);
  TAG_CREATE("aip - strahd me at hello", AIP_YOUSTRAHDME);
  TAG_CREATE("aip - strahd me at hello pt 2", AIP_YOUSTRAHDME2);
  TAG_CREATE("ach - strahd me at hello", ACH_YOUSTRAHDME);
  TAG_CREATE("aip - decepticon", AIP_DECEPTICON);
  TAG_CREATE("ach - decepticon", ACH_DECEPTICON);
  TAG_CREATE("ach - deaths door 100 stats", ACH_DEATHSDOOR);
  TAG_CREATE("deaths door cooldown", TAG_DEATHSDOOR);
  TAG_CREATE("aip - demonslayer", AIP_DEMONSLAYER);
  TAG_CREATE("ach - demonslayer", ACH_DEMONSLAYER);

  TAG_CREATE("epic control", TAG_EPICS);

  TAG_CREATE("fragged victim", TAG_RECENTLY_FRAGGED);
  TAG_CREATE("intercept", TAG_INTERCEPT);
  TAG_CREATE("Addicted to Blood", TAG_ADDICTED_BLOOD);

  TAG_CREATE_WITH_MESSAGES("LAY HANDS", TAG_LAYONHANDS,
                           "&+WYour &+yhands&+W surge with &+Yholy &+Wstrength once again.&n",
                           "&+W$n's &+yhands&+W surge with &+Yholy &+Wstrength once again.&n");

  TAG_CREATE_WITH_MESSAGES("BLOODLUST", TAG_BLOODLUST,
                           "&+rThe taste of &+Rblood &+rslowly fades from your body.&n",
                           "&+r$n's pupils dialate and return to normal.&n");

  TAG_CREATE("minotaur rage", TAG_MINOTAUR_RAGE);
  TAG_CREATE("conjured pet", TAG_CONJURED_PET);
  TAG_CREATE_WITH_MESSAGES("potion timer", TAG_POTION_TIMER,
                           "&+cYou feel like a potion &+Ccould&+c do you good once again.&n",
                           "&+c$n's body appears to recover from the affects of the &+Wpotion&+c.&n");

  //Prestige Races
  TAG_CREATE("pr - rock gnome", PR_ROCKGNOME);
  TAG_CREATE("pr - forest gnome", PR_FORESTGNOME);
  TAG_CREATE("pr - deep gnome", PR_DEEPGNOME);

  //TRAPS
  TAG_CREATE_WITH_MESSAGES("crippled", TAG_CRIPPLED,
                           "&+yYou feel &+Ystrength &+yreturn to your legs.&n",
                           "&+W$n's &+yleg looks &+Ystronger &+yas the wounds covering them dissipate.&n");

  

  TAG_CREATE("eaten food", TAG_EATEN);
  TAG_CREATE("recent frag obj", TAG_OBJ_RECENT_FRAG);
  TAG_CREATE_WITH_MESSAGES("phantasmal form", TAG_PHANTASMAL_FORM,
                           "&+WYou feel yourself return to normal as you leave your phantasmal form.&n",
                           "&+W$n looks more solid as $s phantasmal form abates.&n");

  TAG_CREATE_WITH_MESSAGES("ethereal form", TAG_ETHEREAL_FORM,
                           "&+LYou feel your body begin to regain its former substance...&n",
                           "&+L$n's body begins to regain its former substance&n");
  TAG_CREATE_WITH_MESSAGES("divine power", TAG_PALADIN_VIT,
                             "Your divine power fades.", "");
  TAG_CREATE_WITH_MESSAGES("vampire bite", TAG_VAMPIRE_BITE,
                             "Nauseous feeling from the bitten neck fades.", "");

  TAG_CREATE("embrace death", TAG_EMBRACE_DEATH);
  TAG_CREATE("innate timer", TAG_INNATE_TIMER);
  TAG_CREATE("master set", TAG_SET_MASTER);
  TAG_CREATE("ilienze sword charge", TAG_ILIENZE_SWORD_CHARGE);
  TAG_CREATE("monk critical hits", TAG_PRESSURE_POINTS);

  TAG_CREATE_WITH_MESSAGES("decrepify", TAG_DECREPIFY,
  			   "The red aura around you flashes blue and is gone.","");
  TAG_CREATE_WITH_MESSAGES("hatred", TAG_HATRED,
  			   "You feel the hatred in your heart subside.","");
  TAG_CREATE("shapechange creature", TAG_KNOWN_SHAPE);
  TAG_CREATE("epic stone touched", TAG_EPIC_MONOLITH);
  TAG_CREATE("epic errand", TAG_EPIC_ERRAND);
  TAG_CREATE("epic completed", TAG_EPIC_COMPLETED);
  TAG_CREATE("epic points", TAG_EPIC_POINTS);
  TAG_CREATE("skill timer", TAG_SKILL_TIMER);
  TAG_CREATE("wet", TAG_WET);
  TAG_CREATE("stat pool", TAG_POOL);
  TAG_CREATE("pvp action", TAG_PVPDELAY);
  TAG_CREATE("mental notched", TAG_MENTAL_SKILL_NOTCH);
  TAG_CREATE("physical notched", TAG_PHYS_SKILL_NOTCH);
  TAG_CREATE("aura of protection", AURA_PROTECTION);
  TAG_CREATE("aura of precision", AURA_PRECISION);
  TAG_CREATE("aura of battlelust", AURA_BATTLELUST);
  TAG_CREATE("aura of improved healing", AURA_HEALING);
  TAG_CREATE("aura of endurance", AURA_ENDURANCE);
  TAG_CREATE("aura of vigor", AURA_VIGOR);
  TAG_CREATE("aura of spell protection", AURA_SPELL_PROTECTION);
  TAG_CREATE("epic level", TAG_EPIC_LEVEL);
  TAG_CREATE("guarding nexus stone", TAG_GUARD_NEXUS_STONE);
  TAG_CREATE("hit counter", TAG_HIT_COUNTER);
  TAG_CREATE("nuked counter", TAG_NUKED_COUNTER);
  TAG_CREATE("stormcallers fury target", TAG_STORMCALLERS_FURY_TARGET);
  TAG_CREATE("kostchtchies implosion target", TAG_CHILLING_IMPLOSION_TARGET);
  TAG_CREATE("racechange", TAG_RACE_CHANGE);
  TAG_CREATE("building", TAG_BUILDING);
  TAG_CREATE("reduced exp", TAG_REDUCED_EXP);
  TAG_CREATE_WITH_MESSAGES("dread wrath", TAG_DREAD_WRATH,
  			   "The dreadful aura surrounding you fades away.","");
  TAG_CREATE("firing arrows", TAG_FIRING);
  TAG_CREATE("do not proc", TAG_STOP_PROC);
  TAG_CREATE("bare feet", TAG_BAREFEET);
  TAG_CREATE("troll burn", TAG_TROLL_BURN);
  TAG_CREATE("guildhall tag", TAG_GUILDHALL);
  TAG_CREATE("direction tag", TAG_DIRECTION);
  TAG_CREATE("holy offense", TAG_HOLY_OFFENSE);
  TAG_CREATE("holy defense", TAG_HOLY_DEFENSE);
  TAG_CREATE("boon power", TAG_BOON);
  TAG_CREATE("flag carrier", TAG_CTF);
  TAG_CREATE("ctf flag bonus", TAG_CTF_BONUS);
  TAG_CREATE("salvation cooldown", TAG_SALVATION);
  TAG_CREATE_WITH_MESSAGES("dreadnaught", TAG_DREADNAUGHT,
				"&+yYou feel ready to &+Ycharge &+yinto battle once again.", "");

  TAG_CREATE_WITH_MESSAGES("recently fragged", TAG_PLR_RECENT_FRAG, 
                           "&+rThe thrill of the &+Lrecent &+Rkill &+rleaves your veins.", "");

  TAG_CREATE_WITH_MESSAGES("alliance proposal", TAG_ALLIANCE, "&+bYou revoke your proposed alliance.", "");
  TAG_CREATE_WITH_MESSAGES("enhance healing", TAG_ENHANCE_HEALING, "&+WYou no longer feel looked over by a greater being.", "");
  TAG_CREATE_WITH_MESSAGES("item set bonus", TAG_SETPROC, "You no longer feel any spirit's support.", "");
  
  TAG_CREATE_WITH_MESSAGES("divine force affect", TAG_DIVINE_FORCE_AFFECT, "&+YThe divine force leaves you!", "$n &+Llooks momentarily drained.");  
  TAG_CREATE_WITH_MESSAGES("divine force timer", TAG_DIVINE_FORCE_TIMER, "&+WYou are ready to embrace the divine.", "$n's features &+wsoften considerably.");
  TAG_CREATE_WITH_MESSAGES("broken arm", TAG_ARMLOCK, "&+WYour arm has healed.&n", "$n's arm has healed.&n");
  TAG_CREATE_WITH_MESSAGES("broken leg", TAG_LEGLOCK, "&+WYour leg has healed.&n", "$n's leg has healed.&n");
  TAG_CREATE_WITH_MESSAGES("arrow bleed", TAG_ARROW_BLEED, "&+WYour bleeding wound has healed.&n", "$n's bleeding wound has healed.&n");
  TAG_CREATE("summon spawn/ally", TAG_SPAWN);

  TAG_CREATE_WITH_MESSAGES("rested bonus", TAG_RESTED, "You sigh as you start to feel a bit tired.",
    "$n sighs and stares off into the sky." );
  TAG_CREATE_WITH_MESSAGES("well-rested bonus", TAG_WELLRESTED, "You sigh as you start to feel a bit tired.",
    "$n sighs and stares off into the sky." );

  TAG_CREATE("total epics gained", TAG_EPICS_GAINED);

  TAG_CREATE("regaining composure", TAG_BARDSONG_FAILURE );

  TAG_CREATE("invisibility from object", TAG_PERMINVIS );
  TAG_CREATE("establish camp", TAG_CAMP );

  TAG_CREATE("old newbie zone tag", TAG_LIFESTREAMNEWBIE );

  TAG_CREATE("times recently suicided", TAG_SUICIDE_COUNT );
  TAG_CREATE_WITH_MESSAGES("squidrage timer", TAG_SQUIDRAGE, "You feel ready to &+rr&+Rag&+re&n again!", "" );
}

#ifdef SKILLPOINTS
void initialize_skills_new()
{
  // Base Trees
  SKILL_CREATE( "Warrior", SKILL_BASEWARR,   TAR_IGNORE );
  SKILL_CREATE( "Rogue",   SKILL_BASEROGUE,  TAR_IGNORE );
  SKILL_CREATE( "Priest",  SKILL_BASEPRIEST, TAR_IGNORE );
  SKILL_CREATE( "Mage",    SKILL_BASEMAGE,   TAR_IGNORE );

  // Warrior Tree
  SKILL_CREATE( "1h Combat", SKILL_1HCOMBAT, TAR_IGNORE );
  SKILL_DEPEND( SKILL_1HCOMBAT, SKILL_BASEWARR, 100 );
  skills[SKILL_1HCOMBAT].maxtrainwarr = 100;
  SKILL_CREATE( "1h Bludgeon", SKILL_1H_BLUDGEON, TAR_PHYS );
  SKILL_DEPEND( SKILL_1H_BLUDGEON, SKILL_1HCOMBAT, 100 );
  skills[SKILL_1H_BLUDGEON].maxtrainwarr = 100;
  SKILL_CREATE( "1h Flaying", SKILL_1H_FLAYING, TAR_PHYS );
  SKILL_DEPEND( SKILL_1H_FLAYING, SKILL_1HCOMBAT, 100 );
  skills[SKILL_1H_FLAYING].maxtrainwarr = 100;
  SKILL_CREATE( "1h Slash", SKILL_1H_SLASHING, TAR_PHYS );
  SKILL_DEPEND( SKILL_1H_SLASHING, SKILL_1HCOMBAT, 100 );
  skills[SKILL_1H_SLASHING].maxtrainwarr = 100;
  SKILL_CREATE( "Rage Tactics", SKILL_RAGETACTICS, TAR_IGNORE );
  SKILL_DEPEND( SKILL_RAGETACTICS, SKILL_1H_BLUDGEON, 100 );
  SKILL_DEPEND( SKILL_RAGETACTICS, SKILL_1H_FLAYING, 100 );
  SKILL_DEPEND( SKILL_RAGETACTICS, SKILL_1H_SLASHING, 100 );
  SKILL_DEPEND( SKILL_RAGETACTICS, SKILL_2H_BLUDGEON, 100 );
  SKILL_DEPEND( SKILL_RAGETACTICS, SKILL_2H_FLAYING, 100 );
  skills[SKILL_1H_BLUDGEON].maxtrainwarr = 100;
  SKILL_CREATE_WITH_MESSAGES( "Berserk", SKILL_BERSERK, TAR_PHYS,
    "Your blood cools, and you no longer see targets everywhere.",
    "$n seems to have overcome $s battle madness." );
  SKILL_DEPEND( SKILL_BERSERK, SKILL_RAGETACTICS, 100 );
  skills[SKILL_BERSERK].maxtrainwarr = 100;
  SKILL_CREATE_WITH_MESSAGES( "Infuriate", SKILL_INFURIATE, TAR_PHYS,
    "&+RAs your R&+ra&+RG&+re&+R fades away, you return to your normal size.&n",
    NULL );
  SKILL_DEPEND( SKILL_INFURIATE, SKILL_BERSERK, 100 );
  skills[SKILL_INFURIATE].maxtrainwarr = 100;
  skills[SKILL_INFURIATE].specskill = TRUE;
  SKILL_CREATE( "Rampage", SKILL_RAMPAGE, TAR_PHYS);
  SKILL_DEPEND( SKILL_RAMPAGE, SKILL_INFURIATE, 100 );
  skills[SKILL_RAMPAGE].maxtrainwarr = 100;
  skills[SKILL_RAMPAGE].specskill = TRUE;
  SKILL_CREATE_WITH_MESSAGES( "Rage", SKILL_RAGE, TAR_PHYS,
    "You feel yourself return to normal as your rage abates.",
    "$n returns to normal as $s rage abates." );
  SKILL_DEPEND( SKILL_RAGE, SKILL_RAMPAGE, 100 );
  skills[SKILL_RAGE].maxtrainwarr = 100;
  skills[SKILL_RAGE].specskill = TRUE;

  SKILL_CREATE( "2h Combat", SKILL_2HCOMBAT, TAR_IGNORE );
  SKILL_DEPEND( SKILL_2HCOMBAT, SKILL_BASEWARR, 100 );
  skills[SKILL_2HCOMBAT].maxtrainwarr = 100;
  SKILL_CREATE( "2h Bludgeon", SKILL_2H_BLUDGEON, TAR_PHYS );
  SKILL_DEPEND( SKILL_2H_BLUDGEON, SKILL_2HCOMBAT, 100 );
  skills[SKILL_2H_BLUDGEON].maxtrainwarr = 100;
  SKILL_CREATE( "2h Flaying", SKILL_2H_FLAYING, TAR_PHYS );
  SKILL_DEPEND( SKILL_2H_FLAYING, SKILL_2HCOMBAT, 100 );
  skills[SKILL_2H_FLAYING].maxtrainwarr = 100;
  SKILL_CREATE( "2h Slash", SKILL_2H_SLASHING, TAR_PHYS );
  SKILL_DEPEND( SKILL_2H_SLASHING, SKILL_2HCOMBAT, 100 );
  skills[SKILL_2H_SLASHING].maxtrainwarr = 100;

  SKILL_CREATE( "Enhanced Melee", SKILL_ENHANCEDMELEE, TAR_IGNORE );
  SKILL_DEPEND( SKILL_ENHANCEDMELEE, SKILL_1H_BLUDGEON, 100 );
  SKILL_DEPEND( SKILL_ENHANCEDMELEE, SKILL_1H_FLAYING, 100 );
  SKILL_DEPEND( SKILL_ENHANCEDMELEE, SKILL_1H_SLASHING, 100 );
  SKILL_DEPEND( SKILL_ENHANCEDMELEE, SKILL_2H_BLUDGEON, 100 );
  SKILL_DEPEND( SKILL_ENHANCEDMELEE, SKILL_2H_SLASHING, 100 );
  SKILL_DEPEND( SKILL_ENHANCEDMELEE, SKILL_2H_FLAYING, 100 );
  SKILL_DEPEND( SKILL_ENHANCEDMELEE, SKILL_BAREHANDED_FIGHTING, 100 );
  skills[SKILL_ENHANCEDMELEE].maxtrainwarr = 100;

  SKILL_CREATE( "Double Attack", SKILL_DOUBLE_ATTACK, TAR_PHYS );
  SKILL_DEPEND( SKILL_DOUBLE_ATTACK, SKILL_ENHANCEDMELEE, 100 );
  skills[SKILL_DOUBLE_ATTACK].maxtrainwarr = 100;
  SKILL_CREATE( "Triple Attack", SKILL_TRIPLE_ATTACK, TAR_PHYS );
  SKILL_DEPEND( SKILL_TRIPLE_ATTACK, SKILL_DOUBLE_ATTACK, 100 );
  skills[SKILL_TRIPLE_ATTACK].maxtrainwarr = 100;
  SKILL_CREATE( "Quadruple Attack", SKILL_QUADRUPLE_ATTACK, TAR_PHYS);
  SKILL_DEPEND( SKILL_QUADRUPLE_ATTACK, SKILL_TRIPLE_ATTACK, 100 );
  skills[SKILL_QUADRUPLE_ATTACK].maxtrainwarr = 100;
  skills[SKILL_QUADRUPLE_ATTACK].specskill = TRUE;
  SKILL_CREATE( "Melee Mastery", SKILL_MELEE_MASTERY, TAR_PHYS );
  SKILL_DEPEND( SKILL_MELEE_MASTERY, SKILL_DOUBLE_ATTACK, 100 );
  skills[SKILL_MELEE_MASTERY].maxtrainwarr = 100;
  skills[SKILL_MELEE_MASTERY].specskill = TRUE;
  SKILL_CREATE( "Dual Wield", SKILL_DUAL_WIELD, TAR_PHYS );
  SKILL_DEPEND( SKILL_DUAL_WIELD, SKILL_ENHANCEDMELEE, 100 );
  skills[SKILL_DUAL_WIELD].maxtrainwarr = 100;
  SKILL_CREATE( "Surprise", SKILL_SURPRISE, TAR_PHYS );
  SKILL_DEPEND( SKILL_SURPRISE, SKILL_ENHANCEDMELEE, 100 );
  skills[SKILL_SURPRISE].maxtrainwarr = 100;
  SKILL_CREATE( "Critical Strike", SKILL_CRITICAL_ATTACK, TAR_PHYS );
  SKILL_DEPEND( SKILL_CRITICAL_ATTACK, SKILL_SURPRISE, 100 );
  skills[SKILL_CRITICAL_ATTACK].maxtrainwarr = 100;
  SKILL_CREATE_WITH_MESSAGES( "Whirlwind", SKILL_WHIRLWIND, TAR_PHYS, 
    "You slow down exhausted.",
    "$n slows down exhausted." );
  SKILL_DEPEND( SKILL_WHIRLWIND, SKILL_CRITICAL_ATTACK, 100 );
  skills[SKILL_WHIRLWIND].maxtrainwarr = 100;
  skills[SKILL_WHIRLWIND].specskill = TRUE;

  SKILL_CREATE( "Barehanded Combat", SKILL_BAREHANDEDCOMBAT, TAR_IGNORE );
  SKILL_DEPEND( SKILL_BAREHANDEDCOMBAT, SKILL_BASEWARR, 100 );
  skills[SKILL_BAREHANDEDCOMBAT].maxtrainwarr = 100;
  SKILL_CREATE( "Barehanded Fighting", SKILL_BAREHANDED_FIGHTING, TAR_PHYS );
  SKILL_DEPEND( SKILL_BAREHANDED_FIGHTING, SKILL_BAREHANDEDCOMBAT, 100 );
  skills[SKILL_BAREHANDED_FIGHTING].maxtrainwarr = 100;
  SKILL_CREATE( "Martial Arts", SKILL_MARTIAL_ARTS, TAR_PHYS );
  SKILL_DEPEND( SKILL_MARTIAL_ARTS, SKILL_BAREHANDED_FIGHTING, 100 );
  skills[SKILL_MARTIAL_ARTS].maxtrainwarr = 100;
  SKILL_CREATE( "Combination Attack", SKILL_COMBINATION, TAR_PHYS );
  SKILL_DEPEND( SKILL_COMBINATION, SKILL_MARTIAL_ARTS, 100 );
  skills[SKILL_COMBINATION].maxtrainwarr = 100;
  SKILL_CREATE( "Dragon Punch", SKILL_DRAGON_PUNCH, TAR_PHYS );
  SKILL_DEPEND( SKILL_DRAGON_PUNCH, SKILL_MARTIAL_ARTS, 100 );
  skills[SKILL_DRAGON_PUNCH].maxtrainwarr = 100;
  SKILL_CREATE( "Ki Strike", SKILL_KI_STRIKE, TAR_PHYS | TAR_EPIC );
  SKILL_DEPEND( SKILL_KI_STRIKE, SKILL_DRAGON_PUNCH, 100 );
  skills[SKILL_KI_STRIKE].maxtrainwarr = 100;
  skills[SKILL_KI_STRIKE].specskill = TRUE;
  SKILL_CREATE( "Kick", SKILL_KICK, TAR_PHYS );
  SKILL_DEPEND( SKILL_KICK, SKILL_BAREHANDEDCOMBAT, 100 );
  skills[SKILL_KICK].maxtrainwarr = 100;
  SKILL_CREATE( "Roundkick", SKILL_ROUNDKICK, TAR_PHYS );
  SKILL_DEPEND( SKILL_ROUNDKICK, SKILL_KICK, 100 );
  skills[SKILL_ROUNDKICK].maxtrainwarr = 100;
  skills[SKILL_ROUNDKICK].specskill = TRUE;

  SKILL_CREATE( "Takedowns", SKILL_TAKEDOWNS, TAR_IGNORE );
  SKILL_DEPEND( SKILL_TAKEDOWNS, SKILL_BASEWARR, 100 );
  skills[SKILL_TAKEDOWNS].maxtrainwarr = 100;
  SKILL_CREATE( "Bash", SKILL_BASH, TAR_PHYS );
  SKILL_DEPEND( SKILL_BASH, SKILL_TAKEDOWNS, 100 );
  skills[SKILL_BASH].maxtrainwarr = 100;
  SKILL_CREATE( "Shieldless Bash", SKILL_SHIELDLESS_BASH, TAR_PHYS );
  SKILL_DEPEND( SKILL_SHIELDLESS_BASH, SKILL_BASH, 100 );
  skills[SKILL_SHIELDLESS_BASH].maxtrainwarr = 100;
  SKILL_CREATE( "Skewer", SKILL_SKEWER, TAR_PHYS );
  SKILL_DEPEND( SKILL_SKEWER, SKILL_BASH, 100 );
  skills[SKILL_SKEWER].maxtrainwarr = 100;
  SKILL_CREATE( "Springleap", SKILL_SPRINGLEAP, TAR_PHYS );
  SKILL_DEPEND( SKILL_SPRINGLEAP, SKILL_TAKEDOWNS, 100 );
  skills[SKILL_SPRINGLEAP].maxtrainwarr = 100;
  SKILL_CREATE( "Maul", SKILL_MAUL, TAR_PHYS );
  SKILL_DEPEND( SKILL_MAUL, SKILL_TAKEDOWNS, 100 );
  skills[SKILL_MAUL].maxtrainwarr = 100;
  SKILL_CREATE( "Trample", SKILL_TRAMPLE, TAR_PHYS );
  SKILL_DEPEND( SKILL_TRAMPLE, SKILL_TAKEDOWNS, 100 );
  skills[SKILL_TRAMPLE].maxtrainwarr = 100;

  SKILL_CREATE( "Defensive Training", SKILL_DEFENSIVETRAINING, TAR_IGNORE );
  SKILL_DEPEND( SKILL_DEFENSIVETRAINING, SKILL_BASEWARR, 100 );
  skills[SKILL_DEFENSIVETRAINING].maxtrainwarr = 100;
  SKILL_CREATE( "Parry", SKILL_PARRY, TAR_PHYS );
  SKILL_DEPEND( SKILL_PARRY, SKILL_BASEWARR, 100 );
  skills[SKILL_PARRY].maxtrainwarr = 100;
  SKILL_CREATE( "Enhanced Parry", SKILL_EXPERT_PARRY, TAR_MENTAL | TAR_EPIC );
  SKILL_DEPEND( SKILL_EXPERT_PARRY, SKILL_PARRY, 100 );
  skills[SKILL_EXPERT_PARRY].maxtrainwarr = 100;
  skills[SKILL_EXPERT_PARRY].specskill = TRUE;
  SKILL_CREATE( "Riposte", SKILL_RIPOSTE, TAR_PHYS );
  SKILL_DEPEND( SKILL_RIPOSTE, SKILL_PARRY, 100 );
  skills[SKILL_EXPERT_RIPOSTE].maxtrainwarr = 100;
  SKILL_CREATE( "Follow-up Riposte", SKILL_FOLLOWUP_RIPOSTE, TAR_PHYS );
  SKILL_DEPEND( SKILL_FOLLOWUP_RIPOSTE, SKILL_RIPOSTE, 100 );
  skills[SKILL_FOLLOWUP_RIPOSTE].maxtrainwarr = 100;
  skills[SKILL_FOLLOWUP_RIPOSTE].specskill = TRUE;
  SKILL_CREATE( "Dodge", SKILL_DODGE, TAR_PHYS );
  SKILL_DEPEND( SKILL_DODGE, SKILL_BASEWARR, 100 );
  skills[SKILL_DODGE].maxtrainwarr = 100;
  SKILL_CREATE( "Groundfighting", SKILL_GROUNDFIGHTING, TAR_PHYS );
  SKILL_DEPEND( SKILL_GROUNDFIGHTING, SKILL_DODGE, 100 );
  skills[SKILL_GROUNDFIGHTING].maxtrainwarr = 100;
  SKILL_CREATE( "Rescue", SKILL_RESCUE, TAR_PHYS );
  SKILL_DEPEND( SKILL_RESCUE, SKILL_BASEWARR, 100 );
  skills[SKILL_RESCUE].maxtrainwarr = 100;
  SKILL_CREATE( "Guard", SKILL_GUARD, TAR_PHYS );
  SKILL_DEPEND( SKILL_GUARD, SKILL_RESCUE, 100 );
  skills[SKILL_GUARD].maxtrainwarr = 100;
  SKILL_CREATE( "Rescue All", SKILL_RESCUEALL, TAR_PHYS );
  SKILL_DEPEND( SKILL_RESCUEALL, SKILL_GUARD, 100 );
  skills[SKILL_RESCUEALL].maxtrainwarr = 100;
  skills[SKILL_RESCUEALL].specskill = TRUE;
  SKILL_CREATE( "Shield Block", SKILL_SHIELD_BLOCK, TAR_PHYS );
  SKILL_DEPEND( SKILL_SHIELD_BLOCK, SKILL_BASEWARR, 100 );
  skills[SKILL_SHIELD_BLOCK].maxtrainwarr = 100;
  SKILL_CREATE( "Shield Punch", SKILL_SHIELDPUNCH, TAR_PHYS );
  SKILL_DEPEND( SKILL_SHIELDPUNCH, SKILL_SHIELD_BLOCK, 100 );
  skills[SKILL_SHIELDPUNCH].maxtrainwarr = 100;
  SKILL_CREATE( "Enhanced Bash", SKILL_ENHANCEDBASH, TAR_PHYS );
  SKILL_DEPEND( SKILL_ENHANCEDBASH, SKILL_SHIELDPUNCH, 100 );
  skills[SKILL_ENHANCEDBASH].maxtrainwarr = 100;
  skills[SKILL_ENHANCEDBASH].specskill = TRUE;

  SKILL_CREATE( "Chants", SKILL_CHANTS, TAR_IGNORE );
  SKILL_DEPEND( SKILL_CHANTS, SKILL_BASEWARR, 100 );
  skills[SKILL_CHANTS].maxtrainwarr = 100;

  SKILL_CREATE( "Control of Hands", SKILL_HANDCONTROL, TAR_IGNORE );
  SKILL_DEPEND( SKILL_HANDCONTROL, SKILL_CHANTS, 100 );
  skills[SKILL_HANDCONTROL].maxtrainwarr = 100;
  SKILL_CREATE( "Quivering Palm", SKILL_QUIVERING_PALM, TAR_PHYS );
  SKILL_DEPEND( SKILL_QUIVERING_PALM, SKILL_HANDCONTROL, WHITE_SKILL );
  skills[SKILL_QUIVERING_PALM].maxtrainwarr = 100;
  SKILL_CREATE( "Buddha Palm", SKILL_QUIVERING_PALM, TAR_PHYS );
  SKILL_DEPEND( SKILL_BUDDHA_PALM, SKILL_HANDCONTROL, WHITE_SKILL );
  skills[SKILL_BUDDHA_PALM].maxtrainwarr = 100;

  SKILL_CREATE( "Control of Body", SKILL_BODYCONTROL, TAR_IGNORE );
  SKILL_DEPEND( SKILL_BODYCONTROL, SKILL_CHANTS, 100 );
  skills[SKILL_BODYCONTROL].maxtrainwarr = 100;
  SKILL_CREATE( "Chi Purge", SKILL_CHI_PURGE, TAR_PHYS );
  SKILL_DEPEND( SKILL_CHI_PURGE, SKILL_BODYCONTROL, WHITE_SKILL );
  skills[SKILL_CHI_PURGE].maxtrainwarr = 100;
  SKILL_CREATE( "Calm", SKILL_CALM, TAR_PHYS );
  SKILL_DEPEND( SKILL_CALM, SKILL_BODYCONTROL, WHITE_SKILL );
  skills[SKILL_CALM].maxtrainwarr = 100;

  SKILL_CREATE( "One with Mind", SKILL_ONEWITHMIND, TAR_IGNORE );
  SKILL_DEPEND( SKILL_ONEWITHMIND, SKILL_CHANTS, 100 );
  skills[SKILL_ONEWITHMIND].maxtrainwarr = 100;
  SKILL_CREATE( "Regenerate", SKILL_REGENERATE, TAR_PHYS );
  SKILL_DEPEND( SKILL_REGENERATE, SKILL_ONEWITHMIND, WHITE_SKILL );
  skills[SKILL_REGENERATE].maxtrainwarr = 100;
  SKILL_CREATE( "Tiger Palm", SKILL_TIGER_PALM, TAR_MENTAL );
  SKILL_DEPEND( SKILL_TIGER_PALM, SKILL_ONEWITHMIND, WHITE_SKILL );
  skills[SKILL_TIGER_PALM].maxtrainwarr = 100;

  SKILL_CREATE( "One with Body", SKILL_ONEWITHBODY, TAR_IGNORE );
  SKILL_DEPEND( SKILL_ONEWITHBODY, SKILL_CHANTS, 100 );
  skills[SKILL_ONEWITHBODY].maxtrainwarr = 100;
  SKILL_CREATE( "Diamond Soul", SKILL_DIAMOND_SOUL, TAR_MENTAL );
  SKILL_DEPEND( SKILL_DIAMOND_SOUL, SKILL_ONEWITHBODY, WHITE_SKILL );
  skills[SKILL_DIAMOND_SOUL].maxtrainwarr = 100;
  SKILL_CREATE( "Heroism", SKILL_HEROISM, TAR_MENTAL );
  SKILL_DEPEND( SKILL_HEROISM, SKILL_ONEWITHBODY, WHITE_SKILL );
  skills[SKILL_HEROISM].maxtrainwarr = 100;
  SKILL_CREATE( "Cell Adjustment", SKILL_CELLADJUSTMENT, TAR_MENTAL );
  SKILL_DEPEND( SKILL_CELLADJUSTMENT, SKILL_ONEWITHBODY, WHITE_SKILL );
  skills[SKILL_CELLADJUSTMENT].maxtrainwarr = 100;

  SKILL_CREATE( "Natural Magic", SKILL_NATURALMAGIC, TAR_IGNORE );
  SKILL_DEPEND( SKILL_NATURALMAGIC, SKILL_BASEWARR, 100 );
  skills[SKILL_NATURALMAGIC].maxtrainwarr = 100;

  SKILL_CREATE( "Natures Protections", SKILL_NATURESPROTECTIONS, TAR_IGNORE );
  SKILL_DEPEND( SKILL_NATURESPROTECTIONS, SKILL_NATURALMAGIC, 100 );
  skills[SKILL_NATURESPROTECTIONS].maxtrainwarr = 100;
  SPELL_CREATE_MSG("barkskin", SPELL_BARKSKIN, PULSE_SPELLCAST, TAR_CHAR_ROOM,
    spell_barkskin, "Your skin loses its &+ybarklike &ntexture.");
  SKILL_DEPEND( SPELL_BARKSKIN, SKILL_NATURESPROTECTIONS, WHITE_SKILL );
  skills[SPELL_BARKSKIN].maxtrainwarr = 100;
  SPELL_CREATE_MSG("protection from animals", SPELL_PROTECT_FROM_ANIMAL, PULSE_SPELLCAST,
    TAR_CHAR_ROOM | TAR_SPIRIT,
    spell_protection_from_animals, "You no longer feel safe from &+yanimals&n.");
  SKILL_DEPEND( SPELL_PROTECT_FROM_ANIMAL, SKILL_NATURESPROTECTIONS, WHITE_SKILL );
  skills[SPELL_PROTECT_FROM_ANIMAL].maxtrainwarr = 100;
  SPELL_CREATE_MSG("protection from evil", SPELL_PROTECT_FROM_EVIL, PULSE_SPELLCAST,
    TAR_CHAR_ROOM,
    spell_protection_from_evil, "You do not feel as safe from &+revil&n.");
  SKILL_DEPEND( SKILL_PROTECT_FROM_EVIL, SKILL_NATURESPROTECTIONS, WHITE_SKILL );
  skills[SKILL_PROTECT_FROM_EVIL].maxtrainwarr = 100;

  SKILL_CREATE( "Natures Senses", SKILL_NATURESSENSES, TAR_IGNORE );
  SKILL_DEPEND( SKILL_NATURESSENSES, SKILL_NATURESPROTECTIONS, 100 );
  skills[SKILL_NATURESSENSES].maxtrainwarr = 100;


  SKILL_CREATE( "Occult Magic", SKILL_OCCULTMAGIC, TAR_IGNORE );
  SKILL_DEPEND( SKILL_OCCULTMAGIC, SKILL_BASEWARR, 100 );
  skills[SKILL_OCCULTMAGIC].maxtrainwarr = 100;
  SKILL_CREATE( "Holy Magic", SKILL_HOLYMAGIC, TAR_IGNORE );
  SKILL_DEPEND( SKILL_HOLYMAGIC, SKILL_BASEWARR, 100 );
  skills[SKILL_HOLYMAGIC].maxtrainwarr = 100;

}
#endif


#ifndef _DE_
void assign_racial_skills(P_char ch)
{
  struct affected_type af;
  //if(!IS_SET(PLR3_FLAGS(ch), PLR3_RACIAL_SKILLS)) 

  // if ((af = get_spell_from_char(ch, TAG_RACIAL_SKILLS)) == NULL)
  if(!affected_by_spell(ch, TAG_RACIAL_SKILLS))
  {
	  do_epic_reset(ch, 0, 1); //player has never had their new racial skills set clear out ALL EPIC SKILLS

    memset(&af, 0, sizeof(struct affected_type));
    af.type = TAG_RACIAL_SKILLS;
    af.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM | AFFTYPE_NODISPEL;
    af.duration = -1;
    affect_to_char(ch, &af);
  }
    update_racial_skills(ch);
}

void assign_racial_skills_norefund(P_char ch)
{
  struct affected_type af;
   if(!affected_by_spell(ch, TAG_RACIAL_SKILLS))
     {
	 do_epic_reset_norefund(ch, 0, 1); //player doesnt need a refund
  
        memset(&af, 0, sizeof(struct affected_type));
        af.type = TAG_RACIAL_SKILLS;
        af.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM | AFFTYPE_NODISPEL;
        af.duration = -1;
        affect_to_char(ch, &af);
     }
	 update_racial_skills(ch);
 
}

void update_racial_skills(P_char ch)
{
	int currrace;
  currrace = GET_RACE(ch);
/*
  if(GET_SPEC(ch, CLASS_SORCERER, SPEC_WIZARD))
  {
		ch->only.pc->skills[SKILL_SPELL_PENETRATION].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
		ch->only.pc->skills[SKILL_SPELL_PENETRATION].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
		do_save_silent(ch, 1); // racial skills require a save.
  }
*/
	  switch (currrace)
		 {
			case RACE_GNOME:
			//assign gnome racial epic skills
			ch->only.pc->skills[SKILL_FIX].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_FIX].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_HALFLING:
			ch->only.pc->skills[SKILL_EXPERT_PARRY].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_EXPERT_PARRY].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_GOBLIN:
			ch->only.pc->skills[SKILL_EXPERT_PARRY].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_EXPERT_PARRY].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_FIX].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_FIX].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);

			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_GITHYANKI:
			case RACE_GITHZERAI:
			ch->only.pc->skills[SKILL_ADVANCED_MEDITATION].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_ADVANCED_MEDITATION].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_HUMAN:
			ch->only.pc->skills[SKILL_SHIELD_COMBAT].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_SHIELD_COMBAT].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_SCRIBE_MASTERY].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_SCRIBE_MASTERY].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_DEVOTION].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_DEVOTION].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_ORC:
			ch->only.pc->skills[SKILL_SHIELD_COMBAT].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_SHIELD_COMBAT].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_SCRIBE_MASTERY].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_SCRIBE_MASTERY].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_DEVOTION].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_DEVOTION].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_CENTAUR:
			ch->only.pc->skills[SKILL_EXPERT_RIPOSTE].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_EXPERT_RIPOSTE].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_TWOWEAPON].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_TWOWEAPON].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_TWOWEAPON].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_TWOWEAPON].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_BARBARIAN:
			ch->only.pc->skills[SKILL_ANATOMY].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_ANATOMY].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_TROLL:
			ch->only.pc->skills[SKILL_ANATOMY].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_ANATOMY].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_TOTEMIC_MASTERY].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_TOTEMIC_MASTERY].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_OGRE:
			ch->only.pc->skills[SKILL_DEVASTATING_CRITICAL].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_DEVASTATING_CRITICAL].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_FIRBOLG:
			ch->only.pc->skills[SKILL_NATURES_SANCTITY].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_NATURES_SANCTITY].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			case RACE_THRIKREEN:
			//assign thri-kreen racial epic skills
			//ch->only.pc->skills[SKILL_SHIELDLESS_BASH].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			//ch->only.pc->skills[SKILL_SHIELDLESS_BASH].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = BOUNDED(10, GET_LEVEL(ch) *2, 100);
			do_save_silent(ch, 1); // racial skills require a save.
			break;
			default:
			//do nothing - not a race that has skills
			return;
			break;
		}
}

void reset_racial_skills(P_char ch)
{
/*
  ch->only.pc->skills[SKILL_SPELL_PENETRATION].taught = 0;
  ch->only.pc->skills[SKILL_SPELL_PENETRATION].learned = 0;
*/
  ch->only.pc->skills[SKILL_FIX].taught = 0;
  ch->only.pc->skills[SKILL_FIX].learned = 0;
  ch->only.pc->skills[SKILL_EXPERT_PARRY].taught = 0;
  ch->only.pc->skills[SKILL_EXPERT_PARRY].learned = 0;
  ch->only.pc->skills[SKILL_EXPERT_PARRY].taught = 0;
  ch->only.pc->skills[SKILL_EXPERT_PARRY].learned = 0;
  ch->only.pc->skills[SKILL_FIX].taught = 0;
  ch->only.pc->skills[SKILL_FIX].learned = 0;
  ch->only.pc->skills[SKILL_ADVANCED_MEDITATION].taught = 0;
  ch->only.pc->skills[SKILL_ADVANCED_MEDITATION].learned = 0;
  ch->only.pc->skills[SKILL_SHIELD_COMBAT].taught = 0;
  ch->only.pc->skills[SKILL_SHIELD_COMBAT].learned = 0;
  ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].taught = 0;
  ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].learned = 0;
  ch->only.pc->skills[SKILL_SCRIBE_MASTERY].taught = 0;
  ch->only.pc->skills[SKILL_SCRIBE_MASTERY].learned = 0;
  ch->only.pc->skills[SKILL_DEVOTION].taught = 0;
  ch->only.pc->skills[SKILL_DEVOTION].learned = 0;
  ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = 0;
  ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = 0;
  ch->only.pc->skills[SKILL_SHIELD_COMBAT].taught = 0;
  ch->only.pc->skills[SKILL_SHIELD_COMBAT].learned = 0;
  ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].taught = 0;
  ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].learned = 0;
  ch->only.pc->skills[SKILL_SCRIBE_MASTERY].taught = 0;
  ch->only.pc->skills[SKILL_SCRIBE_MASTERY].learned = 0;
  ch->only.pc->skills[SKILL_DEVOTION].taught = 0;
  ch->only.pc->skills[SKILL_DEVOTION].learned = 0;
  ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = 0;
  ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = 0;
  ch->only.pc->skills[SKILL_EXPERT_RIPOSTE].taught = 0;
  ch->only.pc->skills[SKILL_EXPERT_RIPOSTE].learned = 0;
  ch->only.pc->skills[SKILL_TWOWEAPON].taught = 0;
  ch->only.pc->skills[SKILL_TWOWEAPON].learned = 0;
  ch->only.pc->skills[SKILL_IMPROVED_TWOWEAPON].taught = 0;
  ch->only.pc->skills[SKILL_IMPROVED_TWOWEAPON].learned = 0;
  ch->only.pc->skills[SKILL_ANATOMY].taught = 0;
  ch->only.pc->skills[SKILL_ANATOMY].learned = 0;
  ch->only.pc->skills[SKILL_ANATOMY].taught = 0;
  ch->only.pc->skills[SKILL_ANATOMY].learned = 0;
  ch->only.pc->skills[SKILL_TOTEMIC_MASTERY].taught = 0;
  ch->only.pc->skills[SKILL_TOTEMIC_MASTERY].learned = 0;
  ch->only.pc->skills[SKILL_DEVASTATING_CRITICAL].taught = 0;
  ch->only.pc->skills[SKILL_DEVASTATING_CRITICAL].learned = 0;
  ch->only.pc->skills[SKILL_NATURES_SANCTITY].taught = 0;
  ch->only.pc->skills[SKILL_NATURES_SANCTITY].learned = 0;
  ch->only.pc->skills[SKILL_SHIELDLESS_BASH].taught = 0;
  ch->only.pc->skills[SKILL_SHIELDLESS_BASH].learned = 0;
  ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = 0;
  ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = 0;

  update_racial_skills( ch );
}

#else
// Defined in utility.c on mud.
int flag2idx(int flag)
{
  int      i = 0;

  while (flag > 0)
  {
    i++;
    flag >>= 1;
  }

  return i;
}
#endif
