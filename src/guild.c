/*
 * ***************************************************************************
 * *  File: guild.c                                            Part of Duris *
 * *  Usage: player guilds and skill procedures.
 * * *  Written by: Markus Stenberg
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <stdio.h>
#include "comm.h"
#include "db.h"
#include "epic_skills.h"
#include "guild.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "string.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
/*
 * external variables
 */


extern P_char character_list;
extern P_room world;
extern char *coin_names[];
extern const struct stat_data stat_factor[];
extern int top_of_zone_table;
extern struct int_app_type int_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern int SortedSkills[];
extern const char *spell_types[];
extern const char *position_types[];
extern const char *target_types[];
extern const char *class_names[];
extern const char *specdata[][MAX_SPEC];
extern bool racial_innates[][LAST_RACE + 1];
extern Skill skills[];
extern char *spells[];
extern epic_teacher_skill epic_teachers[];

extern void reset_racial_skills(P_char ch);
int GET_LVL_FOR_SKILL(P_char ch, int skill);
P_obj find_gh_library_book_obj(P_char ch);
//void do_practice_new(P_char ch, char *arg, int cmd);

#define MAX_GUILDS    15        /* max size of high/low lists */

bool     avail_prac[MAX_SKILLS];

void update_skills(P_char ch)
{
  if( !ch || !IS_PC(ch) )
    return;

//#ifdef SKILLPOINTS
//  return;
//#endif

  int      s, notched = 0, lastlvl = 0, shouldbe = 0;
	int spec = ch->player.spec;

  for (s = FIRST_SKILL; s <= LAST_SKILL; s++)
  {
    if (IS_EPIC_SKILL(s))
    {
#if defined(CHAOS_MUD) && (CHAOS_MUD == 1)
      ch->only.pc->skills[s].taught = 100;
      ch->only.pc->skills[s].learned = 100;
#endif
    }
    else if ( GET_LVL_FOR_SKILL(ch, s) > 0 && GET_LEVEL(ch) >= GET_LVL_FOR_SKILL(ch, s) )
    {
/* Why is there 3 possibilitie for .taught?  Either you lose a lvl or spec out of the skill and it goes down,
 *   or you gain a level or spec to a higher skill max and it goes up.  There should be no MAX( ... ).
      ch->only.pc->skills[s].taught = MAX(ch->only.pc->skills[s].taught, MAX(SKILL_DATA_ALL(ch, s).maxlearn[0],
        SKILL_DATA_ALL(ch, s).maxlearn[ch->player.spec]));
*/
      ch->only.pc->skills[s].taught = MAX(ch->only.pc->skills[s].taught,
        SKILL_DATA_ALL(ch, s).maxlearn[ch->player.spec]);

      lastlvl = ch->only.pc->skills[s].learned;
      shouldbe = (GET_LEVEL(ch) * 3 / 2);
      notched = lastlvl-shouldbe;

      //debug("should be: %d, learned so far: %d, notched above: %d", shouldbe, lastlvl, notched);
#if defined(CHAOS_MUD) && (CHAOS_MUD == 1)
      ch->only.pc->skills[s].learned = 100;
      ch->only.pc->skills[s].taught = 100;
#else
      ch->only.pc->skills[s].learned =
        MAX( MIN(40, GET_LEVEL(ch) * 3 / 2), ch->only.pc->skills[s].learned );

      //debug("new learned: %d", ch->only.pc->skills[s].learned);
#endif
    } else {
      ch->only.pc->skills[s].taught = ch->only.pc->skills[s].learned = 0;
    }
    if (!IS_EPIC_SKILL(s) &&
        SKILL_DATA_ALL(ch, s).maxlearn[0] < ch->only.pc->skills[s].taught &&
        SKILL_DATA_ALL(ch, s).maxlearn[ch->player.spec] < ch->only.pc->skills[s].taught)
#if defined(CHAOS_MUD) && (CHAOS_MUD == 1)
      ch->only.pc->skills[s].taught = ch->only.pc->skills[s].learned = 100;
#else
      ch->only.pc->skills[s].taught =
        MAX(0,
            MAX(SKILL_DATA_ALL(ch, s).maxlearn[0],
                SKILL_DATA_ALL(ch, s).maxlearn[ch->player.spec]));
#endif  
  }
  
  // assumes that the instruments and songs are back to back in spells.h
  for( s = FIRST_INSTRUMENT; s <= LAST_SONG; s++ )
  {
    if ( GET_LVL_FOR_SKILL(ch, s) > 0 && GET_LEVEL(ch) >= GET_LVL_FOR_SKILL(ch, s) )
    {
#if defined(CHAOS_MUD) && (CHAOS_MUD == 1)
      ch->only.pc->skills[s].taught = ch->only.pc->skills[s].learned = 100;
#else
      ch->only.pc->skills[s].taught =
      MAX(ch->only.pc->skills[s].taught,
          MAX(SKILL_DATA_ALL(ch, s).maxlearn[0],
              SKILL_DATA_ALL(ch, s).maxlearn[ch->player.spec]));
      ch->only.pc->skills[s].learned =
        MAX(MIN(40, GET_LEVEL(ch) * 3 / 2), ch->only.pc->skills[s].learned);
#endif
    } else {
      ch->only.pc->skills[s].taught = ch->only.pc->skills[s].learned = 0;
    }
    
    if (SKILL_DATA_ALL(ch, s).maxlearn[0] < ch->only.pc->skills[s].taught &&
        SKILL_DATA_ALL(ch, s).maxlearn[ch->player.spec] < ch->only.pc->skills[s].taught)
#if defined(CHAOS_MUD) && (CHAOS_MUD == 1)      
      ch->only.pc->skills[s].taught = ch->only.pc->skills[s].learned = 100;
#else
      ch->only.pc->skills[s].taught =
        MAX(0,
            MAX(SKILL_DATA_ALL(ch, s).maxlearn[0],
                SKILL_DATA_ALL(ch, s).maxlearn[ch->player.spec]));
#endif  
  }
}

bool notch_skill(P_char ch, int skill, float chance)
{
  int intel, t, lvl, l, slvl, i;
  char buf[MAX_STRING_LENGTH];

//#ifdef SKILLPOINTS
//  return 0;
//#endif

  if( !IS_ALIVE(ch) )
    return FALSE;

  if( IS_NPC(ch) )
    return FALSE;

  if( IS_ROOM(ch->in_room, ROOM_GUILD | ROOM_SAFE) )
    return FALSE;

  if( IS_FIGHTING(ch) )
  {
    // This prevents players from notching up skills using images and
    //   summoned pets such as elementals. Jan08 -Lucrot
    if( IS_PC_PET(GET_OPPONENT(ch)) || (GET_LEVEL(GET_OPPONENT(ch)) < 2) )
    {
      return FALSE;
    }
  }

  lvl = GET_LEVEL(ch);

  l = ch->only.pc->skills[skill].learned;
  t = ch->only.pc->skills[skill].taught;

  if( l >= t )
  {
    ch->only.pc->skills[skill].learned = t;
    return FALSE;
  }

#if wipe2011
  // The following addition is for wipe 2011, where intelligence will help determine
  //   chance to notch a skill, thus making it a partially important stat for rockhead melee
  //   characters - Jexni 6/5/11
  intel = BOUNDED(0, 100 - GET_C_INT(ch), 50);
  chance = chance + (intel / 2);

  /* skills can be no higher than level * 2.5 + 5 */
  /* level 1, max = 7 (adjusted to 20), level 10, max = 30, level 30, max = 80 */

  slvl = ((lvl * 5) / 2) + 5;

  if(l >= slvl)
  {
    ch->only.pc->skills[skill].learned = MAX(20, slvl);
    return 0;
  }

  if(ch->only.pc->skills[skill].learned > 99)
  {
    return 0;
  }

  //  Wipe2011 - The above code causes several issues.
  //  1. GET_OPPONENT only returns who you are currently tanking
  //     meaning all experience is compared to that mob, not necesssarily
  //     the one the actual skill is being compared to.
  //  2. This checks multiple skills regardless of whether or not they
  //     actually come into play, such as checking dodge and parry when
  //     the mob misses anyhow.
  //  To that end, we'll add a check into fight.c instead. - Jexni 6/5/11

  if( IS_HARDCORE(ch) )
  {
    chance += (int)(get_property("skill.notch.hardcoreBonus", 0.6) * chance);
  }

  // Instead of simply not allowing notches, we just make it harder - Jexni 1/3/12
  if(affected_by_skill(ch, TAG_PHYS_SKILL_NOTCH))
  {
    chance /= 4;
  }
  else if(affected_by_skill(ch, TAG_MENTAL_SKILL_NOTCH))
  {
    chance /= 4;
  }

  // The higher the skill, the tougher to notch and vice versa
  chance -= chance * (float)l / (float)t;

#endif

#if !defined(CHAOS_MUD) || (CHAOS_MUD == 0)
  // Actual check here.
  if( number(1, 10000) > chance * 100 )
  {
    return FALSE;
  }

  // These will fail if ch is already affected by TAG_..._SKILL_NOTCH.
  if( IS_SET(skills[skill].targets, TAR_PHYS) )
  {
    if(!affect_timer(ch, get_property("timer.mins.physicalNotch", 5) * WAIT_MIN, TAG_PHYS_SKILL_NOTCH))
    {
//      debug( "notch_skill: failed affect_timer on '%s' TAG_PHYS_SKILL_NOTCH", J_NAME(ch) );
    }
  }
  else if(!affect_timer(ch, get_property("timer.mins.mentalNotch", 10) * WAIT_MIN, TAG_MENTAL_SKILL_NOTCH))
  {
//    debug( "notch_skill: failed affect_timer on '%s' TAG_MENTAL_SKILL_NOTCH", J_NAME(ch) );
  }
#endif

  sprintf(buf, "&+cYou feel your skill in %s improving.\n", skills[skill].name);
  send_to_char(buf, ch);
  // If skill is maxxed, check it vs. the epic skill list to see if an epic skill has opened up.
  l = ++(ch->only.pc->skills[skill].learned);

  for( i = 0; epic_teachers[i].vnum; i++ )
  {
    // If they've just opened up the new skill.
    if( epic_teachers[i].pre_requisite == skill
      && epic_teachers[i].pre_req_lvl == l )
    {
      sprintf( buf, "&+WYou can now learn the epic skill '%s'.&n\n\r", skills[epic_teachers[i].skill].name );
      send_to_char( buf, ch );
    }
  }

  return TRUE;
}


int SpellCopyCost(P_char ch, int spell)
{
   int circle, cost;
   // new simple cost formula, none of that other BS - Jexni 1/2/12
   circle = get_spell_circle(ch, spell);
   cost = circle * get_property("spell.cost.plat.per.circle", 1000.000);
   // All spells are currently free to scribe. - Lohrr
   return 0;
}

#if 0
int SpellCopyCost(P_char ch, int skill)  // Old spell copy cost
{
  int      cost = 0, c;

  return 0;

  cost = get_spell_circle(ch, skill);

  c = cost * (cost / 2);
  if (c >= 5)
    c *= cost;
  /* okay, now we have thing ranging from 4 to 1000 */
  /* lets make it fake-plats. */
  c *= 129;
  return c;
}
#endif

int SkillRaiseCost(P_char ch, int skill)
{
  float      cost, s_lvl;
  
  if (IS_SPELL(skill))
    return SpellCopyCost(ch, skill);

  s_lvl = MAX(1, ch->only.pc->skills[skill].learned / 10);
  cost = (s_lvl * s_lvl - (2 * s_lvl)) + 2;
  cost = cost * get_property("skill.cost.practice", 1.0);

  /* ok, the result: cost in gp/etc stuff raising of the skill costs. */
  if (cost < 10)
    cost = 10;
  return (int) cost;
}

int GetClassType(P_char ch)
{
  if (IS_WARRIOR(ch))
    return CLASS_TYPE_WARRIOR;
  if (IS_CLERIC(ch))
    return CLASS_TYPE_CLERIC;
  if (IS_THIEF(ch))
    return CLASS_TYPE_THIEF;
  if (IS_MAGE(ch))
    return CLASS_TYPE_MAGE;
  return CLASS_NONE;
}

char    *how_good(int percent, int level)
{
  if (level > 24)
  {
    sprintf(GS_buf1, "%d", percent);
    return (GS_buf1);
  }

  if (percent == 0)
    strcpy(GS_buf1, " (not learned)");
  else if (percent <= 20)
    strcpy(GS_buf1, " (just learning)");
  else if (percent <= 30)
    strcpy(GS_buf1, " (slightly below average)");
  else if (percent <= 40)
    strcpy(GS_buf1, " (average)");
  else if (percent <= 50)
    strcpy(GS_buf1, " (slightly above average)");
  else if (percent <= 60)
    strcpy(GS_buf1, " (good)");
  else if (percent <= 70)
    strcpy(GS_buf1, " (very good)");
  else if (percent <= 80)
    strcpy(GS_buf1, " (excellent)");
  else
    strcpy(GS_buf1, " (master)");

  return (GS_buf1);
}

int FindHomeTown(P_char ch)
{
  return (zone_table[world[(ch)->in_room].zone].hometown);

}

P_char FindTeacher(P_char ch)
{
  P_char   teacher;

  LOOP_THRU_PEOPLE(teacher, world[ch->in_room].people)
  {
    if (!CAN_SEE(ch, teacher))
      continue;
    if (!IS_PC(teacher) && IS_SET(teacher->specials.act, ACT_TEACHER))
      return teacher;
    /* Gods may teach */
    if (IS_TRUSTED(teacher) && is_linked_to(ch, teacher, LNK_CONSENT) )
      return teacher;
  }
  return NULL;
}

// Checks to see if teacher is the proper class & lvl for a spec spell
// Returns TRUE iff teacher's class has a spec that knows spell skl.
bool is_spec_spell( P_char teacher, int skl )
{
  int spec = teacher->player.spec;

  for (int i = 0; i < MAX_SPEC; i++)
  {
    teacher->player.spec = i;
    if( knows_spell( teacher, skl ) )
    {
      teacher->player.spec = spec;
      return TRUE;
    }
  }

  teacher->player.spec = spec;
  return FALSE;
}

int IsTaughtHere(P_char ch, int skl)
{
  P_char   teacher;
  char     Gbuf1[MAX_STRING_LENGTH];

  teacher = FindTeacher(ch);

  if( !teacher )
  {
    // In the room of knowledge.  This is for 'scribe all' option.
    if( find_gh_library_book_obj(ch) != NULL )
    {
      return TRUE;
    }
    send_to_char("And just who did you plan on teaching you?!?\n", ch);
    return FALSE;
  }
  if( IS_SPELL(skl) )
  {
    if( IS_TRUSTED(teacher) || knows_spell(teacher, skl) || is_spec_spell(teacher, skl) )
    {
      return TRUE;
    }
    else
    {
      if( IS_CASTER(teacher) || IS_SEMI_CASTER(teacher) )
        sprintf(Gbuf1, "I'm not familiar with that spell. Perhaps another more experienced could aid you.");
      else
        sprintf(Gbuf1, "Magic?!? If you had wished to learn the arts, you should have signed up with another guild!");
      mobsay(teacher, Gbuf1);
      return FALSE;
    }
  }

  if (GET_LVL_FOR_SKILL(ch, skl) == 0 )
  {
    sprintf(Gbuf1, "It might help if you knew about such a skill.");
    mobsay(teacher, Gbuf1);
    return FALSE;
  }

  if (SKILL_DATA_ALL(teacher, skl).rlevel[teacher->player.spec] == 0)
  {
    sprintf(Gbuf1, "It might help if I knew about such a skill.");
    mobsay(teacher, Gbuf1);
    return FALSE;
  }
#if 0
  sprintf(Gbuf1,
          "I don't teach such things. Get out and learn them yourself.");
  mobsay(teacher, Gbuf1);
  return FALSE;
#else
  return TRUE;
#endif

  logit(LOG_DEBUG, "default false reached in IsTaughtHere()");
  return FALSE;
}
int RobCash(P_char ch, int cost)
{
  char     buf[MAX_STRING_LENGTH];

  if (cost > (GET_MONEY(ch) + GET_BALANCE(ch)))
    return FALSE;
  if (cost > GET_MONEY(ch))
  {
    GET_COPPER(ch) += GET_BALANCE_COPPER(ch);
    GET_SILVER(ch) += GET_BALANCE_SILVER(ch);
    GET_GOLD(ch) += GET_BALANCE_GOLD(ch);
    GET_PLATINUM(ch) += GET_BALANCE_PLATINUM(ch);
    SUB_MONEY(ch, cost, 0);
    sprintf(buf,
            "Withdrawing &+W%d p&n, &+Y%d g&n, %d s and &+y%d c&N from the bank for training.\n",
            GET_BALANCE_PLATINUM(ch) - GET_PLATINUM(ch),
            GET_BALANCE_GOLD(ch) - GET_GOLD(ch),
            GET_BALANCE_SILVER(ch) - GET_SILVER(ch),
            GET_BALANCE_COPPER(ch) - GET_COPPER(ch));
    send_to_char(buf, ch);
    GET_BALANCE_COPPER(ch) = GET_COPPER(ch);
    GET_BALANCE_SILVER(ch) = GET_SILVER(ch);
    GET_BALANCE_GOLD(ch) = GET_GOLD(ch);
    GET_BALANCE_PLATINUM(ch) = GET_PLATINUM(ch);
    GET_COPPER(ch) = GET_SILVER(ch) = GET_GOLD(ch) = GET_PLATINUM(ch) = 0;
  }
  else
    SUB_MONEY(ch, cost, 0);
  return TRUE;
}

#if 0
bool CharHasSpec(P_char ch)
{
  int      i;

  for (i = SKILL_FIRST_SPEC; i <= SKILL_LAST_SPEC; i++)
    if (GET_CHAR_SKILL_P(ch, i))
      return TRUE;
  return FALSE;
}
#endif

struct spl_list
{
  int      circle;
  int      spell;
};

//static int spell_cmp(const struct spl_list *, const struct spl_list *);

int spell_cmp(const void *va, const void *vb)
{
  const struct spl_list *a = (const struct spl_list *) va;
  const struct spl_list *b = (const struct spl_list *) vb;

  if (a->circle < b->circle)
    return -1;

  if (a->circle > b->circle)
    return 1;

  return (str_cmp(skills[a->spell].name, skills[b->spell].name));
}

void do_spells(P_char ch, char *argument, int cmd)
{
  int      spl, circle, i, count = 0, m_class = 0, class2 = 0, god_mode =
    0, memmed = 0, to_mem = 0, lvl, qend;
  char     buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH],
    buf2[MAX_STRING_LENGTH];
  P_char   target = NULL;
  struct spl_list spell_list[LAST_SPELL+1];
  struct memorize_data *ptr;
  
  if (IS_NPC(ch))
  {
    send_to_char("You ain't nothin' but a hound-dog.\n", ch);
    return;
  }
  *buf = '\0';
  *buf1 = '\0';
  *buf2 = '\0';

  if (!*argument && IS_TRUSTED(ch))
  {
    for (spl = 0; spl <= MAX_AFFECT_TYPES; spl++)
    {
      int      spell = SortedSkills[spl];

      if ( !IS_SPELL(spell) || skills[spell].name == NULL)
        continue;

      *buf2 = '\0';
      if ((lvl =
           skills[spell].m_class[flag2idx(CLASS_ANTIPALADIN) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+LAP(&n&+C%d&+L)&n,", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_CLERIC) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf), "&+cCL(&n&+C%d&n&+y)&n,", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_CONJURER) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+YCO(&n&+C%d&+Y)&n,", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_DRUID) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+gDR(&n&+C%d&n&+g)&n,", lvl);
      if ((lvl =
           skills[spell].m_class[flag2idx(CLASS_NECROMANCER) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+mNE(&n&+C%d&n&+m)&n,", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_PALADIN) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+WPA(&n&+C%d&+W)&n,", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_RANGER) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+GRA(&n&+C%d&+G)&n,", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_SHAMAN) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+CSH(&n&+C%d&+C)&n,", lvl);
      if ((lvl =
           skills[spell].m_class[flag2idx(CLASS_SORCERER) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+MSO(&n&+C%d&+M)&n,", lvl);
      if ((lvl =
           skills[spell].m_class[flag2idx(CLASS_PSIONICIST) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+bPS(&n&+C%d&n&+b)&n", lvl);
      if ((lvl =
           skills[spell].m_class[flag2idx(CLASS_MINDFLAYER) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+bMF(&n&+C%d&n&+b)&n", lvl);
      if ((lvl =
           skills[spell].m_class[flag2idx(CLASS_ILLUSIONIST) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+MIL(&n&+C%d&+M)&n,", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_REAVER) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+bRV(&n&+C%d&n&+b)&n", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_THEURGIST) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+cTH(&n&+C%d&n&+b)&n", lvl);
      if ((lvl = skills[spell].m_class[flag2idx(CLASS_SUMMONER) - 1].rlevel[0]))
        sprintf(buf2 + strlen(buf2), "&+RSU(&n&+C%d&+R)&n,", lvl);

      char buf3[MAX_STRING_LENGTH];
      sprintf(buf3,
              "[%3d] %-28s  %s   %s\n",
              spell, skills[spell].name,
              ((int) IS_AGG_SPELL(spell) ? "&+RAGGR&n" : "    "), buf2);
      count++;

      if( strlen(buf1) + strlen(buf3) > MAX_STRING_LENGTH )
      {
        send_to_char(buf1, ch);
        sprintf(buf1, "%s", buf3);
      }
      else
      {
        strcat(buf1, buf3);
      }

    }
  }
  else
  {                             /* * mortal usage, or immortal with argument */

/*    if (IS_TRUSTED(ch) && argument) {
      if (!(class = get_class_number(argument))) {
        if (*argument == '\'') {
          for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
            *(argument + qend) = LOWER(*(argument + qend));
          if (*(argument + qend) != '\'') {
            send_to_char("Spells are always to be enclosed by the holy symbols known only as 'apostrophies'.",ch);
            return;
          }
          spl = old_search_block(argument, 1, (uint) (MAX(0, (qend - 1))), spells, 0);
          if(spl == -1 || !IS_SPELL(spl)) {
            send_to_char("That is not a spell.\n", ch);
            return;
          }
          sprinttype(skills[spl].min_pos, position_types, buf2);
          sprintbit(skills[spl].targets, target_types, buf);
          sprintf(buf1, "&+ySpell:&n            %s\n"
                        "&+yType:&n             %s\n"
                        "&+yAggressive:&n       %s\n"
                        "&+yCasting time:&n     %d\n"
                        "&+yMinimum position:&n %s\n"
                        "&+yTarget bits:&n      %s\n"
                        "&+ySpell attached:&n   %s\n"
                        "&+yWear off message:&n %s\n",
            skills[spl].name,
            spell_types[skills[spl].type],
            YESNO(skills[spl].harmful),
            skills[spl].beats / 4,
            buf2,
            buf,
            YESNO(skills[spl].spell_pointer),
            skills[spl].wear_off_char[0] ? skills[spl].wear_off_char[0] : "NONE");

          send_to_char(buf1, ch);
          send_to_char("&+yClasses:&n\n&+y--------&n\n",ch);
          *buf1 = '\0';
          for(i=0;i < CLASS_COUNT; i++) {
            if(skills[spl].class[i].rlevel)
              sprintf(buf1 + strlen(buf1), "%-15s &n&+yCircle:&n %d\n",
                class_names[i + 1],
                skills[spl].class[i].rlevel);
          }
          send_to_char(buf1, ch);
          return;
        }
        else if (!(target = get_char_vis(ch, argument))) {
          send_to_char("No such player or class.\n", ch);
          return;
        }
        else if(IS_NPC(target)) {
          send_to_char("Don't use that on mobs.\n", ch);
          return;
        }
        god_mode = 1;
      }
    } else*/

    if (IS_TRUSTED(ch) && argument)
    {
      if (!(target = get_char_vis(ch, argument)))
      {
        send_to_char("No such player.\n", ch);
        return;
      }
    }
    else
      target = ch;

    /* first, build a list of all the spells this person can ever
       have. */
    if (!m_class && target)
    {
      m_class = target->player.m_class;
      class2 =  target->player.secondary_class;
    }
    i = 0;
    for (spl = FIRST_SPELL; spl <= LAST_SPELL; spl++)
    {
      circle = get_spell_circle(target, spl);

      if (circle < MAX_CIRCLE + 1)
      {
        spell_list[i].circle = circle;
        spell_list[i++].spell = spl;
      }
    }
    /* then sort the list... */
    qsort(spell_list, i, sizeof(struct spl_list), spell_cmp);


    /* finally, show it */

    for (spl = 0; spl < i; spl++)
    {
      int      spell = spell_list[spl].spell;
      circle = spell_list[spl].circle;

      if (!spl || (circle != spell_list[spl - 1].circle))
      {
        sprintf(buf, "\n&+B%d%s CIRCLE:&N\n", circle,
                circle == 1 ? "st" : circle == 2 ? "nd" : circle == 3
                ? "rd" : "th");
        strcat(buf1, buf);
      }
      strcpy(buf2, " ");

      if( !SKILL_DATA_ALL(target, spell).maxlearn[0] &&
          !SKILL_DATA_ALL(target, spell).maxlearn[target->player.spec] )
        continue;
//#ifdef SKILLPOINTS
//        sprintf(buf, "%3d %s%-25s %s", 
//          (target && IS_PC(target)) ? target->only.pc->skills[spell].taught : 0,
//          (target && (circle > get_max_circle(target))) ? "&+L" : "",
//          skills[spell].name, buf2);
//#else
        sprintf(buf, "%s%-25s %s", 
          (target && (circle > get_max_circle(target))) ? "&+L" : "",
          skills[spell].name, buf2);
//#endif
      if (target)
      {
        if (meming_class(target))
          strcat(buf, (SpellInSpellBook(target, spell, SBOOK_MODE_IN_INV | SBOOK_MODE_AT_HAND | SBOOK_MODE_ON_BELT))
            ? circle > get_max_circle(target) ? " [in spellbook, but too high level]" : " [in spellbook]" : "");
      }
      strcat(buf, "\n");
      strcat(buf1, buf);
    }
    if (!buf1[0])
      strcpy(buf1, "Duh! You aren't of the casting type!\n");

  }
  strcat(buf1, "\n");
  page_string(ch->desc, buf1, 1);
}

void do_skills(P_char ch, char *argument, int cmd)
{
  int      skl, passes, skil;
  char     buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[256];
  P_char   target;

  if (IS_NPC(ch))
  {
    send_to_char("You ain't nothin' but a hound-dog.\n", ch);
    return;
  }

  *buf = '\0';
  *buf1 = '\0';

  if (!*argument && IS_TRUSTED(ch))
  {
    send_to_char
      ("      Name                        Ap As Ps Ba Cl Co Dr Me Mo Ne Pa Ra Sh So Th Wa Al Re Be Wl Il Su\n",
       ch);
    for (skil = 0; skil <= MAX_AFFECT_TYPES; skil++)
    {
      skl = SortedSkills[skil];

      if( !IS_SKILL(skl) && !IS_INSTRUMENT_SKILL(skl) && !IS_BARD_SONG(skl) )
        continue;

      sprintf(buf1,
        "%-30s    %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d\n",
        skills[skl].name,
        skills[skl].m_class[flag2idx(CLASS_ANTIPALADIN) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_ASSASSIN) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_PSIONICIST) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_BARD) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_CLERIC) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_CONJURER) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_DRUID) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_MERCENARY) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_MONK) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_NECROMANCER) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_PALADIN) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_RANGER) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_SHAMAN) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_SORCERER) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_ROGUE) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_WARRIOR) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_ALCHEMIST) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_REAVER) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_BERSERKER) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_WARLOCK) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_ILLUSIONIST) - 1].rlevel[0],
        skills[skl].m_class[flag2idx(CLASS_SUMMONER) - 1].rlevel[0]);

      send_to_char(buf1, ch);
    }
    return;
  }
  else
  {
    if (IS_TRUSTED(ch) && argument)
    {
      argument = one_argument( argument, buf );
      if( is_abbrev( buf, "reset" ) )
      {
        argument = one_argument( argument, buf );
        if (!(target = get_char_vis(ch, buf)))
        {
          send_to_char("No such character.\n", ch);
          return;
        }
        else
        {
          send_to_char("No longer using racial skils, so nothing to reset.\n", ch);
//          reset_racial_skills(target);
          return;
        }
      }
      if (!(target = get_char_vis(ch, buf)))
      {
        send_to_char("No such character.\n", ch);
        return;
      }
    }
    else
      target = ch;

    send_to_char("&+WSkill\n", ch);
    if (affected_by_spell(target, SPELL_MENTAL_ANGUISH))
      send_to_char("&=LYYou feel a great disturbance in your skills.\n",
          ch);

    sprintf(buf1, "");
    for (skil = 0; skil <= MAX_AFFECT_TYPES; skil++)
    {
      sprintf(buf, "");
      skl = SortedSkills[skil];
      if(( (IS_EPIC_SKILL(skl) &&
        target->only.pc->skills[skl].learned) ||
        GET_LVL_FOR_SKILL(target, skl) > 0 &&
        GET_LVL_FOR_SKILL(target, skl) <= MAXLVLMORTAL )
        && ( IS_SKILL(skl) || IS_INSTRUMENT_SKILL(skl) || IS_BARD_SONG(skl) ) )
      {
        if (IS_PC(target))
        {
          int lvl = GET_LVL_FOR_SKILL(target, skl);
          if(lvl > GET_LEVEL(target))
            if( IS_TRUSTED(ch) )
              sprintf(buf, "%-25s (obtained at level %d) [%d/%d]\n", skills[skl].name,
                  lvl, target->only.pc->skills[skl].taught, target->only.pc->skills[skl].learned);
            else
              sprintf(buf, "%-25s (obtained at level %d)\n", skills[skl].name, lvl);

          else
            if( IS_TRUSTED(ch) )
              sprintf(buf, "%-25s %s%6d&n [%d]\n", skills[skl].name,
                GET_CHAR_SKILL(target, skl) < 
                target->only.pc->skills[skl].taught ? "" : "&+W", 
                GET_CHAR_SKILL(target, skl), target->only.pc->skills[skl].taught);
            else
              sprintf(buf, "%-25s %s%6d&n\n", skills[skl].name,
                GET_CHAR_SKILL(target, skl) < 
                target->only.pc->skills[skl].taught ? "" : "&+W", GET_CHAR_SKILL(target, skl) );
            
        }
        else
        {
          sprintf(buf, "%-25s %6d  \n", skills[skl].name,
              GET_CHAR_SKILL_P(target, skl));
        }

      }
      strcat(buf1, buf);
    }
  }
  page_string(ch->desc, buf1, 1);

}


void prac_all_spells(P_char ch)
{
  int spl;
  int nSpellCnt = 0;
  struct spl_list spell_list[LAST_SPELL+1];

  if( !meming_class(ch) )
  {
    send_to_char( "You don't practice spells.\n", ch );
    return;
  }

  int max_circle = get_max_circle(ch);
  // get a list of spells for this char in circle order
  for (spl = FIRST_SPELL; spl <= LAST_SPELL; spl++)
  {
    // if they can't have the spell, then don't look at it!
    if( !SKILL_DATA_ALL(ch, spl).maxlearn[0] && !SKILL_DATA_ALL(ch, spl).maxlearn[ch->player.spec] )
      continue;

    int circle = get_spell_circle(ch, spl);
    if (circle > max_circle)
      continue;

    if (circle < MAX_CIRCLE + 1)
    {
      spell_list[nSpellCnt].circle = circle;
      spell_list[nSpellCnt++].spell = spl;
    }
  }
  qsort(spell_list, nSpellCnt, sizeof(struct spl_list), spell_cmp);

  // Now find the first one in that SORTED list that they don't already have in a spellbook
  for (spl = 0; spl < nSpellCnt; spl++)
  {
    if( !SpellInSpellBook(ch, spell_list[spl].spell, SBOOK_MODE_IN_INV | SBOOK_MODE_AT_HAND | SBOOK_MODE_ON_BELT ) )
    {
      // yes!  found a spell to scribe!
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "Attempting to scribe '%s'...\r\n", skills[spell_list[spl].spell].name);
      send_to_char(buf, ch);
      if( !IsTaughtHere(ch, spell_list[spl].spell) )
      {
        continue;
      }

      if (!ScriberSillyChecks(ch, spell_list[spl].spell))
      {
        return;
      }

      add_scribe_data(spell_list[spl].spell, ch, SpellBookAtHand(ch), 0, NULL, NULL, prac_all_spells);
//      CharWait(ch, (int) ((3.0 - ((double) GET_CHAR_SKILL(ch, SKILL_SCRIBE)) / 45) * PULSE_VIOLENCE));
      return;

    }
  }

  // everything they can scribe is already scribed!
  P_char teacher = FindTeacher(ch);
  if( teacher )
    mobsay(teacher, "You have everything I will teach you scribed!");

}

void do_practice(P_char ch, char *arg, int cmd)
{
  char   buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH],
         obuf[MAX_STRING_LENGTH];
  int    skl, spl, circle, i, meming_cl, cost, ret;
  P_char teacher;

#ifdef SKILLPOINTS
  do_practice_new( ch, arg, cmd );
  return;
#endif

  if( !IS_ALIVE(ch) || !IS_PC(ch) )
    return;

  teacher = FindTeacher(ch);

  *buf = '\0';
  *buf1 = '\0';
  *obuf = '\0';
  meming_cl = meming_class(ch);


  // List skills available to be taught
  if( !*arg && teacher )
  {

    sprintf(obuf, "&+BSkill                    Cost of Teachings\n&n");
    for (skl = FIRST_SKILL; skl <= LAST_SKILL; skl++)
    {
                           /* skills first */
      if (!IS_SPELL(skl) && (GET_CHAR_SKILL_S(ch, skl) /* ||
                             ((GET_LEVEL(FindTeacher(ch)) >= 51) &&
                              (GET_LEVEL(ch) >= 51)) */ ))
      {
        if( GET_LVL_FOR_SKILL(ch, skl) <= GET_LEVEL(ch)
          && GET_LVL_FOR_SKILL(teacher, skl) <= GET_LEVEL(teacher) )
        {
          sprintf(buf, "%-25s %s\n", skills[skl].name, coin_stringv(SkillRaiseCost(ch, skl)));
        }
        else
        {
          sprintf(buf, "%-25s (cannot practice)\n", skills[skl].name);
        }
        strcat(buf1, buf);
      }
    }
    strcat(obuf, buf1);

    if( meming_cl )
    {
      *buf1 = '\0';
      strcat(obuf, "\n&+BSpell                    Cost to Scribe\n&n");
      for (spl = FIRST_SPELL; spl <= LAST_SPELL; spl++)
      {
        if( IS_SPELL(spl) && GET_LVL_FOR_SKILL(ch, spl) <= GET_LEVEL(ch)
          && GET_LVL_FOR_SKILL(teacher, spl) <= GET_LEVEL(teacher) )
        {
          circle = get_spell_circle(ch, spl);
          if( circle <= get_max_circle(ch) && circle < MAX_CIRCLE + 1 && circle > 0 && knows_spell(teacher, spl) )
          {
            sprintf(buf, "%-25s %s\n", skills[spl].name, coin_stringv(SpellCopyCost(ch, spl)));
            strcat(buf1, buf);
          }
        }
      }
      strcat(obuf, buf1);
    }
    page_string(ch->desc, obuf, 1);
  }

  if (!*arg)
  {
    //send_to_char("Practice what?!? With who as the teacher?!?\n", ch);
  }
  else
  {                             /* request teachings of a certain skill */
    *buf1='\0';
    *buf='\0';

    arg = skip_spaces(arg);
    if (!str_cmp(arg, "all"))
    {
      if( meming_cl )
      {
        prac_all_spells(ch);
      }
      else
      {
        send_to_char( "Try practicing a specific skill.\n", ch );
      }
      return;
    }
    // Search for an exact match first... otherwise guard -> guardian spirits instead of guard.
    if( !(skl = search_block(arg, (const char **) spells, TRUE)) )
    {
      skl = search_block(arg, (const char **) spells, FALSE);
    }
    i = skl;


    if( !IsTaughtHere(ch, skl) )
    {
      /* function will give approp. message */
      return;
    }
    if( IS_SPELL(skl) && get_max_circle(ch) < get_spell_circle(ch, skl) )
    {
      sprintf(buf, "Well, sure, I know that one, but my conscience prevents me from teaching it to someone so unskilled as yourself.");
      if( teacher )
      {
        mobsay(teacher, buf);
      }
      else
      {
        send_to_char("That's too high of a circle for you.\n", ch);
      }
      return;
    }

    if( !SKILL_DATA_ALL(ch, skl).rlevel[ch->player.spec] || SKILL_DATA_ALL(ch, skl).rlevel[ch->player.spec] > GET_LEVEL(ch) )
    {
      if( teacher )
      {
        mobsay(teacher, "Hmm, I don't think you'd understand a damn thing if I *did* try to teach you.");
      }
      else
      {
        send_to_char("You wouldn't understand.\n", ch);
      }
      return;
    }

    if (SKILL_DATA_ALL(ch, skl).rlevel[ch->player.spec] >= 51)
    {
      ret = FALSE;

      if (GET_LEVEL(teacher) < 51)
      {
        strcpy(buf, "Yes, I've heard of such a skill, but have never learned it myself..");
        ret = TRUE;
      }
      else if (GET_LEVEL(ch) < 51)
      {
        strcpy(buf, "Sorry, but you're not quite learned enough for that one yet.");
        ret = TRUE;
      }

      if (ret)
      {
        mobsay(teacher, buf);
        return;
      }
    }

    if( IS_SPELL(skl) && !meming_class(ch) && ch->only.pc->skills[i].learned )
    {
      send_to_char("You need not prac spells! Your deity grants it to you, if you are deemed worthy of it.\n", ch);
      return;
    }

    if( (cost = (!IS_SPELL(skl) ? SkillRaiseCost(ch, skl) : 0
                 /*SpellCopyCost(ch, skl) */ )) > GET_MONEY(ch) )
    {
      sprintf(buf, "Sorry, boss, but I'm afraid you cannot afford the training.");
      mobsay(teacher, buf);
      return;
    }
    if( !IS_SPELL(skl) && GET_LEVEL(ch) * 2 < (ch->only.pc->skills[i].learned) )
    {
      sprintf(buf, "You have not fully grasped your previous lessons. Come back when you have practiced more.");
      mobsay(teacher, buf);
      return;
    }

    if( !IS_SPELL(skl) && (ch->only.pc->skills[i].learned >= 2 * GET_LEVEL(ch)
      || ch->only.pc->skills[i].learned >=
      ch->only.pc->skills[i].taught * get_property("skill.practice.relativeCap", 0.75)) )
    {
      sprintf(buf, "You will have to go learn more on your own, I can teach you no more right now.");
      mobsay(teacher, buf);
      return;
    }

    if( !IS_SPELL(skl) && (ch->only.pc->skills[i].learned == ch->only.pc->skills[i].taught) )
    {
      sprintf(buf, "I'm sorry but I can teach you no more.");
      mobsay(teacher, buf);
      return;
    }
    if( !IS_SPELL(skl) && (ch->only.pc->skills[i].learned >= GET_LEVEL(teacher) * 2) )
    {
      switch (number(1, 4))
      {
      case 1:
        sprintf(buf, "You are awesome already! Perhaps you would be so kind as to teach me?");
        break;
      case 2:
        sprintf(buf, "You trying to make a fool of me? I can teach you nothing more!");
        break;
      case 3:
        sprintf(buf, "I fear I am not good enough to teach you more.");
        break;
      case 4:
        sprintf(buf, "Begone from my halls! I do not stand for sarcasm!");
        break;
      }
      mobsay(teacher, buf);
      sprintf(buf, "DEBUG: ch->only.pc->skills[i].learned = %d (%s)\n",
              ch->only.pc->skills[i].learned, J_NAME(ch));
      debug(buf);
      return;
    }

    if( IS_SPELL(skl) )
    {
      if( SpellInSpellBook(ch, skl, SBOOK_MODE_IN_INV | SBOOK_MODE_AT_HAND | SBOOK_MODE_NO_SCROLL | SBOOK_MODE_ON_BELT) )
      {
        send_to_char("You know that spell already!\n", ch);
        return;
      }
      else if (!ScriberSillyChecks(ch, skl))
      {
        return;
      }
    }

/*** Can practice skill now ***/
    if( !meming_cl || !IS_SPELL(skl) )
    {
      SUB_MONEY(ch, SkillRaiseCost(ch, skl), 0);
/*      ch->only.pc->skills[i].taught += 3; */
      ch->only.pc->skills[i].learned += 1;
      if (ch->only.pc->skills[i].learned > 100)
      {
        ch->only.pc->skills[i].learned = 100;
      }
/*      if (ch->only.pc->skills[i].taught > 100)
        ch->only.pc->skills[i].taught = 100;
*/
      sprintf(buf, "You practice '%s' for a while...\n", skills[skl].name);
      send_to_char(buf, ch);
    }
    else
    {
      sprintf(buf, "You start to scribe the spell '%s'..\n", skills[skl].name);
      send_to_char(buf, ch);
      sprintf(buf, " %s %s", GET_NAME(ch), skills[skl].name);
      do_teach(teacher, buf, cmd);
    }
/*
 * end of single practicing
 */
  }
}

int readGuildFile(P_char ch, int zonenum)
{
  FILE    *guildhalllist;
  char     zoneNumber[MAX_GUILDS][MAX_STRING_LENGTH];
  char     buff[65536], temp[65536];
  int      i = 0;
  char     filename[65536], *ptr;

  sprintf(filename, "lib/information/GuldHallOwner.dat");

  ptr = filename;
  for (ptr = filename; *ptr != '\0'; ptr++)
  {
    *ptr = LOWER(*ptr);
    if (*ptr == ' ')
      *ptr = '_';
  }
  sprintf(temp, "%d", zonenum);

  guildhalllist = fopen(filename, "rt");

  if (!guildhalllist)
  {
    sprintf(buff, "Couldn't open guildhalllist: %s\n", filename);
    send_to_char(buff, ch);
    return -1;
  }

  for (i = 0; i < MAX_GUILDS; i++)
  {
    fscanf(guildhalllist, "%s\n", zoneNumber[i]);
    if (strstr(zoneNumber[i], temp))
    {
      fclose(guildhalllist);
      return i;
    }
  }

  fclose(guildhalllist);
  return 0;
}

int sackGuild(int oldguild, int guildnumber, int newguild)
{
  //  sackGuild(1,243,2);
  FILE    *guildhalllist;
  char     zoneNumber[MAX_GUILDS][MAX_STRING_LENGTH];
  char     filename[65536], *ptr;
  char     buff[65536];
  char     temp[65536];
  int      i;
  char    *r_str;


  sprintf(filename, "lib/information/GuldHallOwner.dat");

  ptr = filename;
  for (ptr = filename; *ptr != '\0'; ptr++)
  {
    *ptr = LOWER(*ptr);
    if (*ptr == ' ')
      *ptr = '_';
  }
  guildhalllist = fopen(filename, "rt");

  if (!guildhalllist)
  {
    sprintf(buff, "Couldn't open guildhalllist: %s\n", filename);

    return -1;
  }



//READ IT UP TO MEMORY
  for (i = 0; i < MAX_GUILDS; i++)
  {
    fscanf(guildhalllist, "%s\n", zoneNumber[i]);
  }
  fclose(guildhalllist);

//FIX THE STUFF

  sprintf(temp, "%d", guildnumber);


  r_str = replace_it(zoneNumber[oldguild], temp, "");

  sprintf(zoneNumber[oldguild], "%s", r_str);

  strcat(zoneNumber[newguild], temp);

//WRITE IT OUT TO FILE

// wizlog(56, "4");

  guildhalllist = fopen(filename, "wt");

  for (i = 0; i < MAX_GUILDS; i++)
  {
    fprintf(guildhalllist, "%s\n", zoneNumber[i]);
  }
  fclose(guildhalllist);
  wizlog(56, "5");

  return 10;
}

char    *replace_it(char *g_string, char *replace_from, char *replace_to)
{
  char    *p, *p1, *return_str;
  int      i_diff;

  i_diff = strlen(replace_from) - strlen(replace_to);   //the margin between the replace_from and replace_to;
  CREATE(return_str, char, strlen(g_string) + 1, MEM_TAG_STRING);

  if (return_str == NULL)
    return g_string;
  return_str[0] = 0;

  p = g_string;

  for (;;)
  {
    p1 = p;                     // old position
    p = strstr(p, replace_from);        // next position
    if (p == NULL)
    {
      strcat(return_str, p1);
      break;
    }
    while (p > p1)
    {
      sprintf(return_str, "%s%c", return_str, *p1);
      p1++;
    }
    if (i_diff > 0)
    {
      RECREATE(return_str, char, strlen(g_string) + i_diff + 1);
      if (return_str == NULL)
        return g_string;
    }
    strcat(return_str, replace_to);
    p += strlen(replace_from);  // new point position
  }
  return return_str;
}

int skill_cost( P_char ch, int skl )
{
  if(  (skl < FIRST_SKILL || skl > LAST_SKILL)
    && (skl < FIRST_SPELL || skl > LAST_SPELL) )
    return -1;
  return 1;
}

void do_practice_new( P_char ch, char *arg, int cmd )
{
  char   buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH],
         obuf[MAX_STRING_LENGTH];
  int    skl, spl, circle, i, cost, ret;
  P_char teacher;

  if( !IS_ALIVE(ch) || !IS_PC(ch) )
    return;

  teacher = FindTeacher(ch);

  *buf = '\0';
  *buf1 = '\0';
  *obuf = '\0';

  // List skills available to be taught
  if( !*arg && teacher )
  {
    sprintf( obuf, "&+BSkill                    Cost of Teachings\n&n");
    for( skl = FIRST_SKILL; skl <= LAST_SKILL; skl++ )
    {
                           /* skills first */
      if( !IS_SPELL(skl) && GET_CHAR_SKILL_S(ch, skl) )
      {
        if( GET_LVL_FOR_SKILL(ch, skl) <= GET_LEVEL(ch)
          && GET_LVL_FOR_SKILL(teacher, skl) <= GET_LEVEL(teacher)
          && skill_cost( ch, skl) > 0 )
          sprintf( buf, "%-25s %d\n", skills[skl].name, skill_cost( ch, skl) );
        else
          sprintf( buf, "%-25s (cannot practice)\n", skills[skl].name );

        strcat( buf1, buf );
      }
    }
    strcat( obuf, buf1 );

    *buf1 = '\0';
    strcat(obuf, "\n&+BSpell                    Cost\n&n");
    for (spl = FIRST_SPELL; spl <= LAST_SPELL; spl++)
    {
      if( GET_LVL_FOR_SKILL(ch, spl) <= GET_LEVEL(ch)
        && GET_LVL_FOR_SKILL(teacher, spl) <= GET_LEVEL(teacher)
        && IS_SPELL(spl) && skill_cost( ch, spl) > 0 )
      {
        circle = get_spell_circle(ch, spl);
        sprintf( buf, "%-25s %d\n", skills[spl].name, skill_cost( ch, spl) );
        strcat(buf1, buf);
      }
    }
    strcat(obuf, buf1);
    page_string(ch->desc, obuf, 1);
    return;
  }

  if( *arg )
  {                             /* request teachings of a certain skill */
    *buf1='\0';
    *buf='\0';

    arg = skip_spaces(arg);
    if (!str_cmp(arg, "all"))
    {
      prac_all_spells(ch);
      return;
    }
    skl = search_block(arg, (const char **) spells, FALSE);
    i = skl;

    if (!IsTaughtHere(ch, skl))
    {
      /* function will give approp. message */
      return;
    }
    if (IS_SPELL(skl) && get_max_circle(ch) < get_spell_circle(ch, skl))
    {
      sprintf(buf,
              "Well, sure, I know that one, but my conscience prevents me from teaching it to someone so unskilled as yourself.");
      mobsay(teacher, buf);
      return;
    }

    if( !SKILL_DATA_ALL(ch, skl).rlevel[ch->player.spec] || SKILL_DATA_ALL(ch, skl).rlevel[ch->player.spec] > GET_LEVEL(ch) )
    {
      mobsay(teacher,
             "Hmm, I don't think you'd understand a damn thing if I *did* try to teach you.");
      return;
    }

    if (SKILL_DATA_ALL(ch, skl).rlevel[ch->player.spec] >= 51)
    {
      ret = FALSE;

      if (GET_LEVEL(teacher) < 51)
      {
        strcpy(buf,
               "Yes, I've heard of such a skill, but have never learned it myself..");
        ret = TRUE;
      }
      else if (GET_LEVEL(ch) < 51)
      {
        strcpy(buf,
               "Sorry, but you're not quite learned enough for that one yet.");
        ret = TRUE;
      }

      if (ret)
      {
        mobsay(teacher, buf);
        return;
      }
    }

    if( skill_cost( ch, skl ) > ch->only.pc->skillpoints )
    {
      sprintf(buf,
              "Sorry, boss, but I'm afraid you cannot afford the training.");
      mobsay(teacher, buf);
      return;
    }
    if (!IS_SPELL(skl) && GET_LEVEL(ch) * 2 <
        (ch->only.pc->skills[i].learned))
    {
      sprintf(buf,
              "You have not fully grasped your previous lessons. Come back when you have practiced more.");
      mobsay(teacher, buf);
      return;
    }

    if (!IS_SPELL(skl) &&
        (ch->only.pc->skills[i].learned >= 2 * GET_LEVEL(ch) ||
         ch->only.pc->skills[i].learned >= 
           ch->only.pc->skills[i].taught * get_property("skill.practice.relativeCap", 0.75)))
    {
      sprintf(buf,
              "You will have to go learn more on your own, I can teach you no more right now.");
      mobsay(teacher, buf);
      return;
    }

    if (!IS_SPELL(skl) &&
        (ch->only.pc->skills[i].learned >= GET_LEVEL(teacher) * 2))
    {
      switch (number(1, 4))
      {
      case 1:
        sprintf(buf,
                "You are awesome already! Perhaps you would be so kind as to teach me?");
        break;
      case 2:
        sprintf(buf,
                "You trying to make a fool of me? I can teach you nothing more!");
        break;
      case 3:
        sprintf(buf, "I fear I am not good enough to teach you more.");
        break;
      case 4:
        sprintf(buf, "Begone from my halls! I do not stand for sarcasm!");
        break;
      }
      mobsay(teacher, buf);
      sprintf(buf, "DEBUG: ch->only.pc->skills[i].learned = %d (%s)\n",
              ch->only.pc->skills[i].learned, J_NAME(ch));
      debug(buf);
      return;
    }

    ch->only.pc->skillpoints -= skill_cost( ch, skl );
    ch->only.pc->skills[i].learned += 10;
    ch->only.pc->skills[i].taught += 10;

    if (ch->only.pc->skills[i].learned > 100)
      ch->only.pc->skills[i].learned = 100;
    sprintf(buf, "You practice '%s' for a while...\n", skills[skl].name);
    send_to_char(buf, ch);
  }
}

void advance_skillpoints( P_char ch )
{
  if( !IS_PC(ch) )
    return;
  ch->only.pc->skillpoints += 10;
  send_to_char( "Skill points not implemented yet.\n", ch );
  return;
}

void demote_skillpoints( P_char ch )
{
  if( !IS_PC(ch) )
    return;
  send_to_char( "Skill points not implemented yet.\n", ch );
  return;
}

string list_spells( int cls, int spec )
{
  int      spl, spell, circle, i, oldcircle;
  char     buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH],
    buf2[MAX_STRING_LENGTH];
  char     color;
  struct spl_list spell_list[LAST_SPELL+1];
  bool found;

  *buf = '\0';
  *buf1 = '\0';
  *buf2 = '\0';
  memset( spell_list, 0, sizeof(spl_list) * (LAST_SPELL+1) );

  // first, build a list of all the spells for this class/spec.
  i = 0;
  for( spl = FIRST_SPELL; spl <= LAST_SPELL; spl++ )
  {
    circle = get_spell_circle(cls, spec, spl);

    if (circle < MAX_CIRCLE + 1)
    {
      spell_list[i].circle = circle;
      spell_list[i++].spell = spl;
    }
  }
  /* then sort the list... */
  qsort(spell_list, i, sizeof(struct spl_list), spell_cmp);

  oldcircle = 0;
  found = FALSE;
  /* finally, show it */
  for( spl = 0; spl < i; spl++ )
  {
    spell = spell_list[spl].spell;
    circle = spell_list[spl].circle;

    // If they don't have the spell..
    if( !(skills[spell_list[spl].spell].m_class[cls-1]).maxlearn[spec] )
    {
      continue;
    }
    if( !spl || circle != oldcircle )
    {
      sprintf( buf, "\n&+B%d%s Circle:&N", circle,
        circle == 1 ? "st" : circle == 2 ? "nd" : circle == 3 ? "rd" : "th" );
      strcat( buf1, buf );
      oldcircle = circle;
      found = FALSE;
    }
    strcpy(buf2, " ");

    color = cls != flag2idx(CLASS_SHAMAN) ? 'c' :
      IS_SET(skills[spell].targets, TAR_ANIMAL) ? 'y' :
      IS_SET(skills[spell].targets, TAR_ELEMENTAL) ? 'r' :
      IS_SET(skills[spell].targets, TAR_SPIRIT) ? 'W' : 'L';
    sprintf(buf, "%s&+%c%s&n", found ? ", " : " ", color, skills[spell].name);
    found = TRUE;
    strcat(buf1, buf);
  }

  if( *buf1 == '\0' )
  {
    strcat( buf1, "\nNone." );
  }
  strcat(buf1, "\n");
  return string(buf1);
}

string list_skills( int cls, int spec )
{
  int      skl, skill, skllvl, i, oldskllvl, lvlending;
  char     buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  struct spl_list skill_list[LAST_SKILL - FIRST_SKILL + 1 ];
  bool     found;

  *buf = '\0';
  *buf1 = '\0';
  *buf2 = '\0';
  memset( skill_list, 0, sizeof(spl_list) * (LAST_SKILL - FIRST_SKILL + 1) );

  // first, build a list of all the spells for this class/spec.
  i = 0;
  for( skl = FIRST_SKILL; skl <= LAST_SKILL; skl++ )
  {
    skllvl = get_skill_level(cls, spec, skl);

    if( skllvl < MAXLVLMORTAL + 1 )
    {
      skill_list[i].circle = skllvl;
      skill_list[i++].spell = skl;
    }
  }
  /* then sort the list... (Can use spell_cmp although we're doing skills and levels not spells and circles). */
  qsort(skill_list, i, sizeof(struct spl_list), spell_cmp);

  oldskllvl = 0;
  found = FALSE;
  /* finally, show it */
  // First, hunt for lost skills:
  for( skl = 0; skl < i; skl++ )
  {
    skill = skill_list[skl].spell;
    skllvl = skill_list[skl].circle;
    if( skllvl < 1)
    {
      if( !found )
      {
        sprintf( buf, "\n&+BSkills lost:&N &+c%s&n", skills[skill].name);
        strcat(buf1, buf);
        found = TRUE;
      }
      else
      {
        sprintf(buf, ", &+c%s&n", skills[skill].name);
        strcat(buf1, buf);
      }
    }
    else
    {
      break;
    }
  }
  // Second, show gained skills.
  found = FALSE;
  for( ; skl < i; skl++ )
  {
    skill = skill_list[skl].spell;
    skllvl = skill_list[skl].circle;

    // If they don't have the spell..
    if( !(skills[skill].m_class[cls-1]).maxlearn[spec] )
    {
      continue;
    }
    if( !skl || skllvl != oldskllvl )
    {
      // Lvls 4-20 get "th", rest get the st/nd/rd or th.
      lvlending = (skllvl > 3 && skllvl < 21) ? 4 : skllvl % 10;
      sprintf( buf, "\n&+B%d%s Level:&N", skllvl,
        lvlending == 1 ? "st" : lvlending == 2 ? "nd" : lvlending == 3 ? "rd" : "th" );
      strcat( buf1, buf );
      oldskllvl = skllvl;
      found = FALSE;
    }
    strcpy(buf2, " ");

    sprintf(buf, "%s&+c%s&n", found ? ", " : " ", skills[skill].name);
    found = TRUE;
    strcat(buf1, buf);
  }

  if( *buf1 == '\0' )
  {
    strcat( buf1, "\nNone." );
  }
  strcat(buf1, "\n");
  return string(buf1);
}

string list_songs( int cls, int spec )
{
  int      sng, song, snglvl, i, oldsnglvl, lvlending;
  char     buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  struct spl_list song_list[LAST_SONG - FIRST_SONG + 1 ];
  struct spl_list instrument_list[LAST_INSTRUMENT - FIRST_INSTRUMENT + 1];
  bool     found;

  *buf = '\0';
  *buf1 = '\0';
  *buf2 = '\0';
  memset( song_list, 0, sizeof(spl_list) * (LAST_SONG - FIRST_SONG + 1) );

  // first, build a list of all the spells for this class/spec.
  i = 0;
  for( sng = FIRST_SONG; sng <= LAST_SONG; sng++ )
  {
    snglvl = get_song_level(cls, spec, sng);

    if( snglvl < MAXLVLMORTAL + 1 )
    {
      song_list[i].circle = snglvl;
      song_list[i++].spell = sng;
    }
  }
  /* then sort the list... (Can use spell_cmp although we're doing songs and levels not spells and circles). */
  qsort(song_list, i, sizeof(struct spl_list), spell_cmp);

  oldsnglvl = 0;
  found = FALSE;
  /* finally, show it */
  // First, hunt for lost skills:
  for( sng = 0; sng < i; sng++ )
  {
    song = song_list[sng].spell;
    snglvl = song_list[sng].circle;
    // Don't think this will be the case ever, but...
    if( snglvl < 1)
    {
      if( !found )
      {
        sprintf( buf, "\n&+BSongs lost:&N &+c%s&n", skills[song].name);
        strcat(buf1, buf);
        found = TRUE;
      }
      else
      {
        sprintf(buf, ", &+c%s&n", skills[song].name);
        strcat(buf1, buf);
      }
    }
    else
    {
      break;
    }
  }
  // Second, show gained skills.
  found = FALSE;
  for( ; sng < i; sng++ )
  {
    song = song_list[sng].spell;
    snglvl = song_list[sng].circle;

    // If they don't have the song..
    if( !(skills[song].m_class[cls-1]).maxlearn[spec] )
    {
      continue;
    }
    if( !sng || snglvl != oldsnglvl )
    {
      // Lvls 4-20 get "th", rest get the st/nd/rd or th.
      lvlending = (snglvl > 3 && snglvl < 21) ? 4 : snglvl % 10;
      sprintf( buf, "\n&+B%d%s Level:&N", snglvl,
        lvlending == 1 ? "st" : lvlending == 2 ? "nd" : lvlending == 3 ? "rd" : "th" );
      strcat( buf1, buf );
      oldsnglvl = snglvl;
      found = FALSE;
    }
    strcpy(buf2, " ");

    sprintf(buf, "%s&+c%s&n", found ? ", " : " ", skills[song].name);
    found = TRUE;
    strcat(buf1, buf);
  }

  // Don't think this will ever be the case either.
  if( *buf1 == '\0' )
  {
    strcat( buf1, "\nNone." );
  }
  strcat(buf1, "\n");

  if( spec == 0 )
  {
    strcat(buf1, "\n==Instruments==");
  }
  else
  {
    strcat(buf1, "\n==Additional Instruments==");
  }
  // first, build a list of all the instrument skills for this class/spec.
  memset( instrument_list, 0, sizeof(spl_list) * (LAST_INSTRUMENT - FIRST_INSTRUMENT + 1) );
  i = 0;
  for( sng = FIRST_INSTRUMENT; sng <= LAST_INSTRUMENT; sng++ )
  {
    // Instruments are skills.
    snglvl = get_skill_level(cls, spec, sng);

    if( snglvl < MAXLVLMORTAL + 1 )
    {
      instrument_list[i].circle = snglvl;
      instrument_list[i++].spell = sng;
    }
  }
  /* then sort the list... (Can use spell_cmp although we're doing instruments and levels not spells and circles). */
  qsort(instrument_list, i, sizeof(struct spl_list), spell_cmp);

  oldsnglvl = 0;
  found = FALSE;
  /* finally, show it */
  // First, hunt for lost instruments:
  for( sng = 0; sng < i; sng++ )
  {
    song = instrument_list[sng].spell;
    snglvl = instrument_list[sng].circle;
    // Don't think this will be the case ever, but...
    if( snglvl < 1)
    {
      if( !found )
      {
        sprintf( buf, "\n&+BInstruments lost:&N &+c%s&n", skills[song].name);
        strcat(buf1, buf);
        found = TRUE;
      }
      else
      {
        sprintf(buf, ", &+c%s&n", skills[song].name);
        strcat(buf1, buf);
      }
    }
    else
    {
      break;
    }
  }
  // Second, show gained skills.
  found = FALSE;
  for( ; sng < i; sng++ )
  {
    song = instrument_list[sng].spell;
    snglvl = instrument_list[sng].circle;

    // If they don't have the song..
    if( !(skills[song].m_class[cls-1]).maxlearn[spec] )
    {
      continue;
    }
    if( !sng || snglvl != oldsnglvl )
    {
      // Lvls 4-20 get "th", rest get the st/nd/rd or th.
      lvlending = (snglvl > 3 && snglvl < 21) ? 4 : snglvl % 10;
      sprintf( buf, "\n&+B%d%s Level:&N", snglvl,
        lvlending == 1 ? "st" : lvlending == 2 ? "nd" : lvlending == 3 ? "rd" : "th" );
      strcat( buf1, buf );
      oldsnglvl = snglvl;
      found = FALSE;
    }
    strcpy(buf2, " ");

    sprintf(buf, "%s&+c%s&n", found ? ", " : " ", skills[song].name);
    found = TRUE;
    strcat(buf1, buf);
  }

  // Don't think this will ever be the case either (A bard with no instrument skills?).
  if( !found )
  {
    strcat( buf1, "\nNone." );
  }

  return string(buf1);
}

