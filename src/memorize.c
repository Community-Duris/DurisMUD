/*
   ***************************************************************************
   *  File: memorize.c                                         Part of Duris *
   *  Usage: spell memorization                                                *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "comm.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "profile.h"
#include "guildhall.h"

/*
   external variables
 */

extern P_event current_event;
extern P_room world;
extern char *command[];
extern const struct stat_data stat_factor[];
extern const struct racial_data_type racial_data[];
extern int pulse;
extern const float druid_memtime_terrain_mod[];
extern const float blighter_memtime_terrain_mod[];
extern Skill skills[];
extern char *spells[];
P_nevent  get_scheduled(P_char, event_func);
extern void event_wait(P_char, P_char, P_obj, void*);
extern int is_wearing_necroplasm(P_char);
void     event_memorize(P_char, P_char, P_obj, void *);
void     event_scribe(P_char, P_char, P_obj, void *);
void     affect_to_end(P_char ch, struct affected_type *af);
void     prac_all_spells(P_char ch);

char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH],
  Gbuf3[MAX_STRING_LENGTH];

/*
   Spell memorization system code mainly by Markus Stenberg
   (/ Torm of Duris ) 11/94 - only original memory list structure remained,
   everything else changed.

   This is really adnd-lookalike memorization/scribing system, which I
   personally hate, but which other gods insisted on - shrug.
 */

/*
   data for how many spells/lvl people get. This has kinda bug -
   paladins/rangers rock relatively hard as they get more spells/lvl
   than in adnd, but .. karma.
 */
int spl_table[TOTALLVLS + 1][MAX_CIRCLE];

/*
   coolest thing since sliced bread, instead of linear or anything table,
   we use one with function for level affect:

   f(x) = (root(-25,0.1))^(25-level)

   precalculated to save _BUNCH_ of cpu when adding a lot of spells
 */

float    fake_sqrt_table[] = {
  .3311, .3467, .3631, .3802, .3981, .4169, .4365, .4571,
  .4786, .5011, .5248, .5495, .5754, .6026, .6310, .6609,
  .6918, .7244, .7586, .7943, .8318, .8710, .9120, .9550,
  1, 1.0471, 1.0965, 1.1485, 1.2023, 1.2590, 1.3183, 1.3804,
  1.4454, 1.514, 1.5849, 1.6596, 1.7378, 1.8197, 1.9054, 1.9953,
  2.0893, 2.1877, 2.291, 2.3988, 2.5119, 2.6302, 2.7542, 2.8840,
  3.02, 3.1623, 3.3113, 3.4674, 3.6308, 3.8019, 3.9811, 4.1687,
  4.3652, 4.5709, 4.7863, 5.0118, 5.2481, 5.4954, 5.7544, 6.0256, 6.3096
};

double   lfactor[MAX_CIRCLE] = {
  400.0000, 565.6854, 692.8203, 800.0000, 894.4272,
  979.7959, 1058.3005, 1131.3708, 1200.0000, 1264.9110,
  1324.0000, 1370.1234
};

static int get_circle_memtime(P_char ch, int circle, bool bStatOnly = false);
static int calculate_harpy_time(P_char ch, int circle, bool bStatOnly = false);
static int calculate_undead_time(P_char ch, int circle, bool bStatOnly = false);

bool book_class(P_char ch)
{
  unsigned int first = ch->player.m_class;
  unsigned int second = ch->player.secondary_class;

  if (IS_MULTICLASS_PC(ch)) {
    if (IS_CASTER_CLASS(first))
      return (IS_BOOK_CLASS(first));
    else
      return IS_BOOK_CLASS(second);
  } else {
    return IS_BOOK_CLASS(first);
  }
}

bool praying_class(P_char ch)
{
  return IS_PRAYING_CLASS(ch->player.m_class) ||
    (!IS_CASTER_CLASS(ch->player.m_class) &&
     IS_PRAYING_CLASS(ch->player.secondary_class));
}

bool meming_class(P_char ch)
{
  return IS_MEMING_CLASS(ch->player.m_class) ||
    (!IS_CASTER_CLASS(ch->player.m_class) && !USES_MANA(ch) &&
     IS_MEMING_CLASS(ch->player.secondary_class));
}

int IS_SEMI_CASTER(P_char ch)
{
  unsigned int first = ch->player.m_class;
  unsigned int second = ch->player.secondary_class;

  if (IS_NPC(ch))
  {
    return IS_SEMI_CASTER_CLASS(first) && !IS_PURE_CASTER_CLASS(first);
  }

  if (IS_MULTICLASS_PC(ch))
  {
     return !(IS_PURE_CASTER_CLASS(first) && IS_PURE_CASTER_CLASS(second));
  }
  else
  {
     return IS_SEMI_CASTER_CLASS(first);
  }
}

int IS_PARTIAL_CASTER(P_char ch) 
{
  unsigned int first = ch->player.m_class;
  unsigned int second = ch->player.secondary_class;

  if (IS_MULTICLASS_PC(ch))
  {
    if (IS_PARTIAL_CASTER_CLASS(first))
    {
      return IS_PARTIAL_CASTER_CLASS(first);
    } 
    else
    {
      return IS_PARTIAL_CASTER_CLASS(second);
    }
  } 
  else
  {
    return IS_PARTIAL_CASTER_CLASS(first);
  }
}

int IS_CASTER(P_char ch)
{
  return IS_CASTER_CLASS(ch->player.m_class) ||
    IS_CASTER_CLASS(ch->player.secondary_class);
}

int get_max_circle(P_char ch)
{
  return (BOUNDED(1, ((int) ((GET_LEVEL(ch) - 1) / 5) + 1), MAX_CIRCLE));
}

inline int max_spells_in_circle(P_char ch, int circ, int max_circ)
{
  int      j;
  int      m_class;
  if (circ > max_circ)
    return 0;

  j = MAX(1, spl_table[MIN(GET_LEVEL(ch), MAXLVL)][circ - 1]);

  //j = (int) (j * 1.4);

  if (IS_PARTIAL_CASTER(ch)) {
    return (int) ( (j+1) / 1.5);
  } else {
    if (IS_SEMI_CASTER(ch))
      return (int) ((j + 1) / 2);
  }

  return MAX(1, j);
}

int max_spells_in_circle(P_char ch, int circ)
{
  return max_spells_in_circle(ch, circ, get_max_circle(ch));
}

// This works for PCs and NPCs?  Does now.
int get_spell_circle(P_char ch, int spl)
{
  int i, ch_circle = (GET_LEVEL(ch) - 1) / 5 + 1;
  int lowest, class_idx, spec;
  lowest = MAX_CIRCLE + 1;
  spec = ch->player.spec;

  // We check all classes for NPCs.
  if( IS_NPC(ch) )
  {
    // We skip CLASS_NONE = 0.
    for( i = 0; i < CLASS_COUNT; i++ )
    {
      // If the NPC has the class, check the circle.
      if( ch->player.m_class & (1 << i) )
      {
        if( (skills[spl].m_class[i].rlevel[0] > 0) && (skills[spl].m_class[i].rlevel[0] < lowest) )
        {
          lowest = skills[spl].m_class[i].rlevel[0];
        }
      }
    }
    if( IS_SPECIALIZED(ch) )
    {
      // Set i to the class.
      i = flag2idx(ch->player.m_class) - 1;
      if( (skills[spl].m_class[i].rlevel[spec] > 0) && (skills[spl].m_class[i].rlevel[spec] < lowest) )
      {
        lowest = skills[spl].m_class[i].rlevel[spec];
      }
    }
  }
  else
  {
    // If the player has the spell as a spec spell (or reg if not spec'd).
    if( (SKILL_DATA(ch, spl).rlevel[spec] > 0) )
    {
      lowest = SKILL_DATA(ch, spl).rlevel[spec];
    }

    // If the player has it as a secondary class spell.
    if( ch->player.secondary_class )
    {
      // If the secondary class has the spell (not including MAX_CIRCLE) and the primary doesn't.
      if( (SKILL_DATA2(ch, spl).rlevel[0] > 0) && lowest == MAX_CIRCLE + 1
        && SKILL_DATA2(ch, spl).rlevel[0] + 1 < lowest )
      {
        return SKILL_DATA2(ch, spl).rlevel[0] + 1;
      }
    }
  }

  return lowest;
}

int SpellInThisSpellBook_p(struct extra_descr_data *tmp, int spl)
{
  return (IS_SET(tmp->description[spl / 8], 1 << (spl % 8)));
}

int SpellInThisSpellBook(struct extra_descr_data *tmp, int spl)
{
  return SpellInThisSpellBook_p(tmp, spl);
}

P_obj SpellInSpellBook(P_char ch, int spl, int mode)
{
  return FindSpellBookWithSpell(ch, spl, mode);
}

int GetSpellPages(P_char ch, int spl)
{
  int      pages;

  pages =
    (!SKILL_DATA_ALL(ch, spl).rlevel[ch->player.spec] ? MAX_CIRCLE +
     1 : SKILL_DATA_ALL(ch, spl).rlevel[ch->player.spec]);


  if(IS_MULTICLASS_PC(ch))
    pages = (int) (pages / 2);
    
  if( GET_CHAR_SKILL(ch, SKILL_SCRIBE_MASTERY) )
  {
    int skill = GET_CHAR_SKILL(ch, SKILL_SCRIBE_MASTERY);
    int mod = MAX(1, (int) (skill / 5));
    pages -= mod;
  }
  
  if(pages < 1)
    pages = 1;
  
  return pages;
}

/*
   Only five restrictions:
   1. Highest available circle must have fewest spells.
   2. All circles must have <= previous circle.
   3. progression must be smooth, limit in any given circle must be
   >= number in that circle for previous level.
   4. each level must have at least 1 more spell than previous level.
   (which sets the lower limit for pf)
   5. too keep things from becoming too warped, not circle can add one, when
   that addition will make it 3 greater than the next circle (can always
   add one to reduce the gap though, usually to the max circle).

   To implement this, there is only one settable variable, the power factor (pf).
   A pf of 100 yields (max circle) new circles of spells per level, a pf of 200
   gives twice that, etc.

   JAB
 */

void SetSpellCircles(void)
{
  bool     added, flag;
  int      i, lvl, max, pf, p_c = 0, t_c = 0, tot_circles = 0, cur_circle;

  pf = 125;                     /*
                                   a pf of 125 gives 74 total spells at level
                                   50
                                 */

  for (i = 0; i < MAX_CIRCLE; i++)
    spl_table[0][i] = 0;

  for( lvl = 1; lvl < TOTALLVLS; lvl++ )
  {

    added = FALSE;
    for (i = 0; i < MAX_CIRCLE; i++)
      spl_table[lvl][i] = spl_table[lvl - 1][i];

    max = BOUNDED(1, ((lvl - 1) / 5) + 1, MAX_CIRCLE) - 1;

    t_c += (max + 1);           /* this is base value, eliminates rounding
                                   errors without using floating point.  */

    p_c = MAX(1, (pf * t_c) / 100 - tot_circles);
    cur_circle = max;

    do
    {
      if (p_c > cur_circle)
      {
        spl_table[lvl][cur_circle]++;
        p_c -= (cur_circle + 1);
      }
      else
      {
        cur_circle--;
        continue;
      }

      /* ok, we just added one to cur_circle, now make sure other rules are
         followed, and if not, deduct it again, and decrement cur_circle */

      flag = FALSE;
      if (cur_circle == max)
      {
        if ((max == 0) || (spl_table[lvl][max] < spl_table[lvl][max - 1]))
          flag = TRUE;
      }
      else if (cur_circle == (max - 1))
      {
        if ((cur_circle != 0) &&
            (spl_table[lvl][cur_circle] <= spl_table[lvl][cur_circle - 1]))
          flag = TRUE;
      }
      else
      {
        /* < max - 1 */
        if ((spl_table[lvl][cur_circle] - spl_table[lvl][cur_circle + 1]) < 2)
          flag = TRUE;
        if ((cur_circle != 0) &&
            (spl_table[lvl][cur_circle] > spl_table[lvl][cur_circle - 1]))
          flag = FALSE;
      }

      if (!flag)
      {
        spl_table[lvl][cur_circle]--;
        p_c += (cur_circle + 1);
        cur_circle--;
        continue;
      }
      /* ok, we added a spell, flag it so we know last condition is met.  */

      added = TRUE;
      tot_circles += (cur_circle + 1);
    }
    while ((cur_circle >= 0) && (p_c > 0));

    /* this is final step, if pf is very low, we won't be able to meet all the
       rules, and still be certain of always adding at least 1 spell per level,
       so we force it.  */
    if (!added)
    {
      for (i = 0; i < max; i++)
        if (spl_table[lvl][i] > spl_table[lvl][i + 1])
        {
          spl_table[lvl][i + 1]++;
          tot_circles += (i + 2);
          break;
        }
    }
  }

#if 0
  /*
     use this for debugging, #if 1 will print the entire table to log/debug for
     checking over. JAB
   */
  {
    FILE    *df;

    if (!(df = fopen(LOG_DEBUG, "a")))
      return;

    for (lvl = 1; lvl < TOTALLVLS; lvl++)
    {
      fprintf(df, "L%3d ", lvl);
      for (i = 0; i < MAX_CIRCLE; i++)
        fprintf(df, "%3d", spl_table[lvl][i]);
      fprintf(df, "\n");
    }
    fclose(df);
  }
#endif
}

#if 0                           /* Torm's generic circle setter */
void SetSpellCircles(void)
{
  int      a, b[MAX_CIRCLE], c[MAX_CIRCLE], d, e;

  for (a = 0; a < MAX_CIRCLE; a++)
  {
    b[a] = 0;
    c[a] = 0;
  }
  b[0] = 1;
  c[0] = 0;
  d = 1;
  for (a = 0; a < (MAXLVL * 3); a++)
  {
    if (GetMaxCircle_level(a + 1) > d)
    {
      d++;
      c[d - 1] = (a - 2);
    }
    for (e = 1; e <= d; e++)
      if ((a - c[e - 1]) > (d - e + 1))
      {
        b[e - 1]++;
        c[e - 1] = a;
      }
    for (e = 0; e < MAX_CIRCLE; e++)
      spl_table[a][e] = b[e];
  }

  /* Heh heh, ok, stuff done.  */
  /* Result, if my math holds true (which it doesn't do usually anyway */
  /*
     Level 1:  1 circle 1 spell. Level 50: (with 7 as max_circle: 12, 11, 10,
     9, 8, 6, 4)

     (with max_circle of 9) about 9 all lvl spells, except few less of highest
     lvl models the power of the spellcasters increases exponentially with exp
     level, but so do memorization times and exp costs to next level, and
     rarity of spells, and _SCRIBING_SPEED_ of spells!  (50th lvler doesn't
     scribe MAX_CIRCLE lvl spells fast at all! cackle).
   */
}
#endif

void show_stop_memorizing(P_char ch)
{
  if(IS_PUNDEAD(ch) ||
     GET_CLASS(ch, CLASS_WARLOCK) ||
     IS_UNDEADRACE(ch) ||
     is_wearing_necroplasm(ch))
  {
    send_to_char("Your mind reels in confusion as your link with &+Lnegative powers&N is disturbed!\n", ch);
    act("$n moans loudly in confusion as $s contact with &+Ldark powers&n is disturbed.", FALSE, ch, 0, 0, TO_ROOM);
    CharWait(ch, WAIT_SEC * 2);
  }
  else if( USES_TUPOR(ch) )
  {
    send_to_char("Your tupor has been disturbed!\n"
       "&+CThe interr&n&+cupted lin&+Wk with the st&+Corm spirits leav&n&+ces you in sho&+Wck.\r\n", ch);
    act("$n &+Wshrieks&n in shock from the severed tupor link.", FALSE, ch, 0, 0, TO_ROOM);
    CharWait(ch, WAIT_SEC * 2);
  }
  else if (book_class(ch))
  {
    send_to_char("You abandon your studies.\n", ch);
    act("$n, looking very frustrated, packs up $s book.", FALSE, ch, 0, 0, TO_ROOM);
  }
  else if (GET_CLASS(ch, CLASS_SHAMAN))
  {
    send_to_char("You abandon your meditative trance.\n", ch);
    act("$n snaps out of $s trance, looking frustrated.", FALSE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    send_to_char("You abandon your praying.\n", ch);
    act("$n, looking very frustrated, packs up $s holy symbol.", FALSE, ch, 0, 0, TO_ROOM);
  }
  REMOVE_BIT(ch->specials.affected_by2, AFF2_MEMORIZING);
  if (IS_AFFECTED(ch, AFF_MEDITATE))
    stop_meditation(ch);
}

void stop_memorizing(P_char ch)
{
  if (IS_PC(ch) && IS_AFFECTED2(ch, AFF2_MEMORIZING))
  {
    show_stop_memorizing(ch);
    disarm_char_nevents(ch, event_memorize);
  }
}

int calculate_undead_time(P_char ch, int circle, bool bStatOnly)
{
  float    remem_time, time_mult, tick_factor;

  if( !IS_ALIVE(ch) )
  {
    return 9999; // In effect, creates a huge mem time due to this strange situation.
  }

  if(IS_SEMI_CASTER(ch))
  {
    time_mult = 1.7;
  }
  else
  {
    time_mult = 0.85;
  }

  // Around 14-20
  tick_factor = MAX(1, STAT_INDEX(GET_C_POW(ch)));

  // For every point above the 'racial 100', subtract .7 points from tick_factor.
  if(GET_C_INT(ch) > stat_factor[(int) GET_RACE(ch)].Int)
  {
    tick_factor += 0.7 * (GET_C_INT(ch) - (stat_factor[(int) GET_RACE(ch)].Int));
  }

  if(GET_C_WIS(ch) > stat_factor[(int) GET_RACE(ch)].Wis)
  {
    tick_factor += 0.7 * (GET_C_WIS(ch) - (stat_factor[(int) GET_RACE(ch)].Wis)) / 2;
  }

  remem_time = (int) ((time_mult * lfactor[circle - 1]) / (fake_sqrt_table[GET_LEVEL(ch) - 1] * tick_factor));

  if( !bStatOnly && IS_AFFECTED(ch, AFF_MEDITATE) && GET_CHAR_SKILL(ch, SKILL_MEDITATE) > number(0, 100) )
  {
    remem_time *= 0.5;
  }

  return (int) remem_time;
}

int calculate_harpy_time(P_char ch, int circle, bool bStatOnly)
{
  float remem_time, time_mult, tick_factor;

  if( IS_SEMI_CASTER(ch) )
  {
    time_mult = 1.7;
  }
  else
  {
    time_mult = 0.85;
  }

  if( GET_CLASS(ch, CLASS_ETHERMANCER) || GET_CLASS(ch, CLASS_SHAMAN) )
  {
    tick_factor = MAX(1, STAT_INDEX(GET_C_WIS(ch)));
    if( GET_C_WIS(ch) > stat_factor[(int) GET_RACE(ch)].Wis )
    {
       tick_factor += GET_C_WIS(ch) - (stat_factor[(int) GET_RACE(ch)].Wis);
    }
  }
  else
  {
    tick_factor = MAX(1, STAT_INDEX(GET_C_INT(ch)));
    if (GET_C_INT(ch) > stat_factor[(int) GET_RACE(ch)].Int)
    {
       tick_factor += GET_C_INT(ch) - (stat_factor[(int) GET_RACE(ch)].Int);
    }
  }

  tick_factor = tick_factor * get_property("memorize.factor.tupor", 1.0);

  // Move the following lines from above to calc remem_time after all mods
  // are made to tick_factor - Dalith 2/05
  remem_time = (int) ((time_mult * lfactor[circle - 1]) / (fake_sqrt_table[GET_LEVEL(ch) - 1] * tick_factor));

  /* harpies who tupor while awake or not reclining get a slight disadvantage */
  if( !IS_AWAKE(ch) && GET_POS(ch) == POS_PRONE )
  {
    remem_time *= .75;
  }

  if( !bStatOnly && IS_AFFECTED(ch, AFF_MEDITATE)
    && (notch_skill(ch, SKILL_MEDITATE, (int) get_property("skill.notch.meditate", 3))
    || GET_CHAR_SKILL(ch, SKILL_MEDITATE) < number(0, 100)) )
  {
    remem_time /= 2;
  }
/* Don't know what to do with this.  Ethermancers get meditate.
  // simulate meditate
  if( !bStatOnly && (GET_LEVEL(ch) > number(0, 65)) )
  {
    remem_time *= 0.5;
  }
*/
  return (int) remem_time;
}

int get_circle_memtime(P_char ch, int circle, bool bStatOnly)
{
  float    time_mult, time;
  P_nevent e;
  int      tick_factor = 0, align, level, clevel;
  //int      lowlvlcap = (int)get_property("memorize.lowlvl.cap", 30);
  //int      lowlvlbottom = (int)get_property("memorize.lowlvl.bottom", 40);

  if( IS_PUNDEAD(ch) || GET_CLASS(ch, CLASS_WARLOCK) || IS_UNDEADRACE(ch) || IS_ANGEL(ch)
    || is_wearing_necroplasm(ch))
  {
    return calculate_undead_time(ch, circle, bStatOnly);
  }
  else if( USES_TUPOR(ch) )
  {
    return calculate_harpy_time(ch, circle, bStatOnly);
  }

  level = circle - 1;
  clevel = GET_LEVEL(ch) - 1;

  if( IS_MULTICLASS_PC(ch) )
  {
    if( ch->player.m_class  && (GET_PRIME_CLASS(ch, CLASS_DRUID) || GET_PRIME_CLASS(ch, CLASS_RANGER)) )
    {
      time_mult = get_property("memorize.factor.multi.commune", 1.75);
    }
    else if( ch->player.m_class && GET_PRIME_CLASS(ch, CLASS_BLIGHTER) )
    {
      time_mult = get_property("memorize.factor.multi.deforest", 1.75);
    }
    else
    {
      time_mult = get_property("memorize.factor.multiclass", 1.75);
    }
  }
  else if( IS_SEMI_CASTER(ch) )
  {
    level = MAX(1, level - 2);
    time_mult = 2.25;
  }
  else
  {
    // Why we have a 1.25 base instead of 1.00 I have no idea..
    //drannak - figuring this out
    time_mult = 1.25;
  }

  if(book_class(ch))
  {
    tick_factor = MAX(1, STAT_INDEX(GET_C_INT(ch)));

    if(GET_C_INT(ch) > stat_factor[(int) GET_RACE(ch)].Int)
    {
      tick_factor += GET_C_INT(ch) - (stat_factor[(int) GET_RACE(ch)].Int);
    }
  }
 /* //reverting psionicist to use mana
 else if(ch->player.m_class & CLASS_PSIONICIST)
  {
   tick_factor = MAX(1, STAT_INDEX(GET_C_POW(ch)));

    if(GET_C_POW(ch) > stat_factor[(int) GET_RACE(ch)].Pow)
    {
      tick_factor += GET_C_POW(ch) - (stat_factor[(int) GET_RACE(ch)].Pow);
    }
  }
  */
  else
  {
    tick_factor = MAX(1, STAT_INDEX(GET_C_WIS(ch)));

    // If Wisdom w/eq is > 100.
    if( GET_C_WIS(ch) > stat_factor[(int) GET_RACE(ch)].Wis )
    {
      if( (GET_PRIME_CLASS(ch, CLASS_DRUID) || GET_PRIME_CLASS(ch, CLASS_BLIGHTER)) && IS_MULTICLASS_PC(ch) )
      {
         // Increase tick factor by amount over 100 / 1.3.
         tick_factor += (int)((GET_C_WIS(ch) - (stat_factor[(int) GET_RACE(ch)].Wis)) / 1.3);
      }
      else
      {
         tick_factor += GET_C_WIS(ch) - (stat_factor[(int) GET_RACE(ch)].Wis);
      }
    }
  }

  float clevel_mod = fake_sqrt_table[clevel];

  int lowlvlcap = get_property("memorize.lowlvl.cap", 25);
  if( IS_PC(ch) && (GET_LEVEL(ch) < lowlvlcap) )
  {
      clevel_mod += (fake_sqrt_table[lowlvlcap] - clevel_mod) / 2.0;
  }

  time = (time_mult * lfactor[level]) / (clevel_mod * tick_factor);

  if( IS_NPC(ch) )
  {
    time = time * get_property("memorize.factor.npc", 1.0);
  }
  else if( ch->player.m_class  && (GET_PRIME_CLASS(ch, CLASS_DRUID) || GET_PRIME_CLASS(ch, CLASS_RANGER)) )
  {
    // modify based on property tweak
    time = time * get_property("memorize.factor.commune", 1.0);

    // send_to_char("you're a druid or ranger\r\n", ch);
    // modify for terrain
    float modifier = druid_memtime_terrain_mod[world[ch->in_room].sector_type];

    time = time * modifier;

    // reduction in time if you have the epic skill
    if( OUTSIDE(ch) && GET_CHAR_SKILL(ch, SKILL_NATURES_SANCTITY) > number(1, 100) )
    {
      time = time * .65;
    }

    // randomly reduce the time by 33% based on level of caster
    if( !bStatOnly && !GET_OPPONENT(ch) && GET_LEVEL(ch) > number(0, 65) )
    {
      time = time / 1.5;
    }

    align = GET_ALIGNMENT(ch);

    // alignment modifiers...
    if( (align >= -10) && (align <= 10) && !IS_MULTICLASS_PC(ch) )
    {
        // only true druids...
        time = time / 1.5;  // 0 align reduces time by 1/3
    }
    else if( (align <= 350) && (align >= -350) )
    {
       time = time * 1.0; // -350 to 350 pretty much doesn't do anything
    }
    else if( (align > 350 && align < 900)  ||
           ((align < -350) && (align > -900)) )
    {
       time = time * 1.6;  // -900 to 900 gives a 60% penalty
    }
    else
    {
       time = time * 2.0;  // everyone else has time doubled
    }

    // cant commune while in command lag, this makes sure they will
    // not regain spells while spamming cast
    if(!bStatOnly && (e = get_scheduled(ch, event_wait)))
    {
      time += ne_event_time(e);  // adding the time ensures that the event will not complete
                                 // instantly after a fight or spell is cast
    }
  }
  else if( ch->player.m_class && GET_PRIME_CLASS(ch, CLASS_BLIGHTER) )
  {
    // modify for terrain
    float modifier = blighter_memtime_terrain_mod[world[ch->in_room].sector_type];

    time = time * modifier;

    // randomly reduce the time by 1/3 based on level of caster
    if(!bStatOnly && !GET_OPPONENT(ch) && GET_LEVEL(ch) > number(0, 65))
    {
      time = time / 1.5;
    }

    // reduction in time if you have the epic skill
    if( OUTSIDE(ch) && GET_CHAR_SKILL(ch, SKILL_NATURES_RUIN) > number(1, 100) )
    {
      time = time * .65;
    }

    // cant commune while in command lag, this makes sure they will
    // not regain spells while spamming cast
    if(!bStatOnly && (e = get_scheduled(ch, event_wait)))
    {
      time += ne_event_time(e);  // adding the time ensures that the event will not complete
                                 // instantly after a fight or spell is cast
    }
  }
/* Reverting back to use mana
  else if(ch->player.m_class & CLASS_PSIONICIST)
  {
    time = time * get_property("memorize.factor.focus", 1.0);
    if(GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION) > 0)
    {
      time = (int) (time / (1.0 + 0.5 * (float)GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION) / 100.0));
    }
    if(!bStatOnly && IS_AFFECTED(ch, AFF_MEDITATE))
    {
       int med_roll = GET_CHAR_SKILL(ch, SKILL_MEDITATE) - number(0, 100);
       if (med_roll > 95)
           time *= 0.3;
       else if (med_roll > 80)
           time *= 0.4;
       else if (med_roll > 50)
           time *= 0.5;
       else if (med_roll > 0)
           time *= 0.65;
    }

    // cant focus while in command lag, this makes sure they will
    // not regain spells while spamming cast
    if(!bStatOnly && (e = get_scheduled(ch, event_wait)))
    {
      time += ne_event_time(e);  // adding the time ensures that the event will not complete instantly after a fight or spell is cast
    }
}
*/
  return (int)time;
}

void balance_align(P_char ch) 
{
  int c_al;
  int base = 5;
  int mult = 1;

  // only true druids revert to nuetral
  // Correction: will allow multiclass druids to balance, but 
  // very, very slowly
  /*
  if (!GET_CLASS(ch, CLASS_DRUID) || IS_MULTICLASS_PC(ch)) {
      return;
  }
  */
  if (!(GET_CLASS(ch, CLASS_DRUID) || (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))) {
      return;
  }

  switch(world[ch->in_room].sector_type) {
      case SECT_FOREST:
          break;
      default:
          return;
          break;
  }

  c_al = GET_ALIGNMENT(ch);

  if ((c_al >= -10 && c_al <=10) || (GET_RACEWAR(ch) == RACEWAR_EVIL))
	 {
      GET_ALIGNMENT(ch) = 0;
      return;
  }

  if (IS_MULTICLASS_PC(ch)) { 
     base = mult;
  }

  if (c_al > 5) {
      c_al -= base;
  } else if (c_al < 5) {
      c_al += base;
  }

  GET_ALIGNMENT(ch) = BOUNDED(-1000, c_al, 1000);
  return;
}

void handle_undead_mem(P_char ch)
{
  int      max = 0, i = 1, j = 1, time;
  int      memorized = FALSE, highest_empty = 0;
  char     gbuf[MAX_STRING_LENGTH];

  if(!USES_COMMUNE(ch) && !USES_FOCUS(ch) && !GET_CLASS(ch, CLASS_WARLOCK)
    && !USES_DEFOREST(ch) && !((IS_UNDEADRACE(ch) || is_wearing_necroplasm(ch)
    || IS_PUNDEAD(ch) || USES_TUPOR(ch)
    || IS_ANGEL(ch)) && IS_AFFECTED2(ch, AFF2_MEMORIZING)))
  {
    return;
  }

  for( i = get_max_circle(ch); i >= 1; i-- )
  {
    if( ch->specials.undead_spell_slots[i] >= max_spells_in_circle(ch, i) )
    {
      continue;
    }
    else if( memorized )
    {
      highest_empty = i;
      continue;
    }

    if( (USES_COMMUNE(ch) || USES_FOCUS(ch) || USES_DEFOREST(ch)) && (IS_CASTING(ch) || GET_OPPONENT(ch)) )
    {
      highest_empty = i;
      break;
    }

    ch->specials.undead_spell_slots[i]++;

    if( IS_PUNDEAD(ch) || is_wearing_necroplasm(ch)
      || GET_CLASS(ch, CLASS_WARLOCK) || IS_UNDEADRACE(ch) )
    {
      sprintf(gbuf, "&+LYou feel infused by %d%s circle DARK powers!\n",
        i, i == 1 ? "st" : (i == 2 ? "nd" : (i == 3 ? "rd" : "th")));
    }
    else if( USES_TUPOR(ch) )
    {
      sprintf(gbuf, "&+WEthereal e&+Cssences flo&+cw into you, res&+Ctoring your %d%s&+c &+Wcircle powers!\n",
        i, i == 1 ? "st" : (i == 2 ? "nd" : (i == 3 ? "rd" : "th")));
    }
    /* reverting to use mana
	else if(GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER) || GET_RACE(ch) == RACE_PILLITHID)
    {
      sprintf(gbuf, "&+mYou feel a rush of energy as a&+L %d%s circle &+mspell coalesces in your mind...\n",
              i, i== 1 ? "st" : (i == 2 ? "nd" : (i == 3 ? "rd" : "th")));
    }
	*/
    else if(IS_ANGEL(ch))
    {
      sprintf(gbuf, "&+WYou feel illuminated with a %d%s circle spell.\n",
	      i, i == 1 ? "st" : (i == 2 ? "nd" : (i == 3 ? "rd" : "th")));
    }
    else if( USES_DEFOREST(ch) )
    {
      sprintf(gbuf, "&+yYou draw a %d%s circle of energy from your surroundings.\n",
        i, i == 1 ? "st" : (i == 2 ? "nd" : (i == 3 ? "rd" : "th")));
    }
    else
    {
      sprintf(gbuf, "&+GYou feel nature's energy flow into your %d%s circle of knowledge.\n",
        i, i == 1 ? "st" : (i == 2 ? "nd" : (i == 3 ? "rd" : "th")));
    }

    send_to_char(gbuf, ch);

		/* Druids memming in forets will ever so slowly regain balance 
		 * with nature.
		 */
    balance_align(ch);//drannak

    if (ch->specials.undead_spell_slots[i] < max_spells_in_circle(ch, i))
    {
      highest_empty = i;
      break;
    }
    else
      memorized = TRUE;
  }

  if( highest_empty )
  {
    time = get_circle_memtime(ch, highest_empty);
    add_event(event_memorize, time, ch, 0, 0, 0, &time, sizeof(time));
  }
  else if( is_wearing_necroplasm(ch)
    || !(USES_COMMUNE(ch) || USES_FOCUS(ch) || USES_DEFOREST(ch)) )
  {
    send_to_char("&+rYou feel fully infused...\n", ch);

    CharWait(ch, WAIT_SEC);
    if( IS_PUNDEAD(ch) || is_wearing_necroplasm(ch)
      || IS_UNDEADRACE(ch) || GET_CLASS(ch, CLASS_WARLOCK) )
    {
      send_to_char("The assimilation process leaves you momentarily shocked.\n", ch);
    }
    else if( USES_TUPOR(ch) )
    {
      send_to_char("&+CThe spir&n&+cits reced&+We, leaving y&+Cou in a m&n&+comentary stat&+We of lethargy.&n\n", ch);
    }
    else if (IS_ANGEL(ch))
    {
      send_to_char("&+WYour gift from above is now complete.\n", ch);
    }
    REMOVE_BIT(ch->specials.affected_by2, AFF2_MEMORIZING);
    stop_meditation(ch);
  }
  /* //reverting to use mana
  else if (GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER) || (GET_RACE(ch) == RACE_PILLITHID))
  {
    send_to_char("&+mThe energies in your mind converge into full cohesion...\n", ch);
  }
  */
  else if( USES_DEFOREST(ch) )
  {
    send_to_char( "&+yYour consumption of nature's energy is complete.&n\n", ch);
  }
  else
    send_to_char("&+GYour communion with nature is now complete...\n", ch);
}

int memorized_in_circle(P_char ch, int circle)
{
  struct affected_type *af;
  int      memorized = 0;

  for( af = ch->affected; af; af = af->next )
  {
    if( af->type == TAG_MEMORIZE && get_spell_circle(ch, af->modifier) == circle )
    {
      memorized++;
    }
  }

  return memorized;
}

void handle_memorize(P_char ch)
{
  struct affected_type *af;
  bool     memorized = FALSE;
  int      time;

  if( !IS_AFFECTED2(ch, AFF2_MEMORIZING) )
    return;

  if( GET_STAT(ch) != STAT_RESTING
    || (GET_POS(ch) != POS_SITTING && GET_POS(ch) != POS_KNEELING) )
  {
    show_stop_memorizing(ch);
    return;
  }

  for( af = ch->affected; af; af = af->next )
  {
    if( af->type == TAG_MEMORIZE && (af->flags & MEMTYPE_FULL) == 0 )
    {
      if( memorized )
      {
        time = get_circle_memtime(ch, get_spell_circle(ch, af->modifier));
        add_event(event_memorize, time / 2, ch, 0, 0, 0, &time, sizeof(time));
        return;
      }
      else
      {
#if !defined(CHAOS_MUD) || (CHAOS_MUD != 1)
        if( book_class(ch) && !(SpellInSpellBook(ch, af->modifier, SBOOK_MODE_IN_INV | SBOOK_MODE_AT_HAND
          | SBOOK_MODE_ON_BELT | SBOOK_MODE_ON_GROUND)) )
        {
          send_to_char("You have managed to misplace your spellbook!\n", ch);
          show_stop_memorizing(ch);
          return;
        }
#endif
        if (meming_class(ch))
        {
          sprintf(Gbuf1, "You have finished memorizing %s.\n", skills[af->modifier].name);
        }
        else
        {
          sprintf(Gbuf1, "You have finished praying for %s.\n", skills[af->modifier].name);
        }
        send_to_char(Gbuf1, ch);
        af->flags |= MEMTYPE_FULL;
        memorized = TRUE;
      }
    }
  }

  if (GET_CLASS(ch, CLASS_SHAMAN))
  {
    send_to_char("You snap out of your meditative trance, memorization complete.\n", ch);
    sprintf(Gbuf1, "$n snaps out of $s meditative trance, %s.",
           IS_EVIL(ch) ? "grinning widely" : "smiling placidly");
  }
  else if (!book_class(ch))
  {
    send_to_char("Your prayers are complete.\n", ch);
    sprintf(Gbuf1, "$n raises $s %sholy symbol and smiles %s.",
            IS_EVIL(ch) ? "un" : "",
            IS_EVIL(ch) ? "malevolently" : "broadly");
  }
  else
  {
    send_to_char("Your studies are complete.\n", ch);
    sprintf(Gbuf1, "$n closes $s book and %s.",
            IS_EVIL(ch) ? "grins malevolently" : "smiles broadly");
  }
  act(Gbuf1, TRUE, ch, 0, 0, TO_ROOM);
  REMOVE_BIT(ch->specials.affected_by2, AFF2_MEMORIZING);
  stop_meditation(ch);
}

int memorize_last_spell(P_char ch)
{
  struct affected_type *af, *last_spell = NULL;

  for (af = ch->affected; af; af = af->next)
  {
    if (af->type == TAG_MEMORIZE && (af->flags & MEMTYPE_FULL) == 0)
    {
      last_spell = af;
    }
  }

  if( last_spell )
  {
    last_spell->flags |= MEMTYPE_FULL;
    return last_spell->modifier;
  }

  return FALSE;
}

void event_memorize(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      time = 0;

  if( data )
    time = *((int *) data);

  if( time && (!IS_AFFECTED(ch, AFF_MEDITATE)
    || (!notch_skill(ch, SKILL_MEDITATE, (int) get_property("skill.notch.meditate", 3))
    && GET_CHAR_SKILL(ch, SKILL_MEDITATE) < number(0, 100))))
  {
    add_event(event_memorize, time / 2, ch, 0, 0, 0, 0, 0);
    return;
  }

  if( GET_CLASS(ch, CLASS_WARLOCK) || IS_PUNDEAD(ch) || USES_COMMUNE(ch)
    || USES_DEFOREST(ch) || USES_TUPOR(ch)
    || IS_UNDEADRACE(ch) || is_wearing_necroplasm(ch) || USES_FOCUS(ch) || IS_ANGEL(ch) )
  {
    handle_undead_mem(ch);
  }
  else
  {
    handle_memorize(ch);
  }
}

void do_assimilate(P_char ch, char *argument, int cmd)
{
  int circle, available, need_mem = 0;

  if( cmd == CMD_ASSIMILATE && !(IS_PUNDEAD(ch) || IS_NPC(ch) || IS_ANGEL(ch)
    || IS_UNDEADRACE(ch) || is_wearing_necroplasm(ch) || GET_CLASS(ch, CLASS_WARLOCK)) )
  {
    send_to_char("&+LThe powers of the dark ignore you...\n", ch);
    return;
  }
  else if( cmd == CMD_COMMUNE && !USES_COMMUNE(ch) )
  {
    send_to_char("&+GThe forces of nature ignore you...\n", ch);
    return;
  }
  else if( cmd == CMD_DEFOREST && !USES_DEFOREST(ch) )
  {
    send_to_char("&+yYou wave your hands trying to absorb the forces of nature.\n", ch);
    return;
  }
  else if( cmd == CMD_TUPOR && !USES_TUPOR(ch) )
  {
    send_to_char("You have no idea how to even begin.\n", ch);
    return;
  }
  else if (cmd == CMD_FOCUS && !USES_FOCUS(ch))
  {
    send_to_char("You don't know how to do that!\n", ch);
    return;
  }

  sprintf(Gbuf1, "You currently have the following spell slots available:\n");
  for (circle = 1; circle <= get_max_circle(ch); circle++)
  {
    available = ch->specials.undead_spell_slots[circle];
    sprintf(Gbuf1 + strlen(Gbuf1), " %2d%s circle: %2d of %2d",
            circle, circle == 1 ? "st" : circle == 2 ?
            "nd" : circle == 3 ? "rd" : "th",
            available, max_spells_in_circle(ch, circle));
    if (available != max_spells_in_circle(ch, circle))
    {
      need_mem = circle;
      sprintf(Gbuf1 + strlen(Gbuf1), "   (%4d seconds)", 
           (get_circle_memtime(ch,circle,true) * (max_spells_in_circle(ch,circle) - available))/WAIT_SEC);
    }
    strcat(Gbuf1, "\n");
  }
  send_to_char(Gbuf1, ch);

  if( (USES_COMMUNE(ch) || USES_FOCUS(ch) || USES_DEFOREST(ch)) && need_mem && !get_scheduled(ch, event_memorize) )
    add_event(event_memorize, get_circle_memtime(ch, need_mem), ch, 0, 0, 0, 0, 0);

  if( !need_mem || USES_COMMUNE(ch) || USES_FOCUS(ch) || USES_DEFOREST(ch) )
    return;

  if( cmd == CMD_TUPOR )
  {
    if (GET_STAT(ch) != STAT_RESTING ||
        (GET_POS(ch) != POS_SITTING && GET_POS(ch) != POS_KNEELING))
    {
      /* harpies also get to tupor while sleeping... */
      if (IS_AWAKE(ch))
      {
        return;
      }
    }

    send_to_char("\n&+cYour mind dr&+Cifts int&+Wo a deep meditat&+cion and "
                 "th&+Ce spirits of sto&+Wrms and i&+cce spe&+Cak to yo&+cu.&n\n", ch);

  }
  else if (cmd == CMD_ASSIMILATE)
  {
    if (GET_STAT(ch) != STAT_RESTING ||
        (GET_POS(ch) != POS_SITTING && GET_POS(ch) != POS_KNEELING))
    {
      return;
    }
    if (IS_ANGEL(ch))
      send_to_char("\n&+WYou call to the heavens above for the gift of magic.\n", ch);
    else
      send_to_char("&+LYou begin to invoke evil and assimilate the powers of darkness.\n", ch);
    //if (!IS_AFFECTED2(ch, AFF2_MEMORIZING))
    //  CharWait(ch, 1 * PULSE_VIOLENCE);
  }

  if (!IS_AFFECTED2(ch, AFF2_MEMORIZING))
  {
    add_event(event_memorize, get_circle_memtime(ch, get_max_circle(ch)) / 2, ch,
              0, 0, 0, 0, 0);
  }

  SET_BIT(ch->specials.affected_by2, AFF2_MEMORIZING);
}


/*
  Added this as more effective version of do_assimilate(ch, 0, CMD_COMMUNE)
  to be called from event_mob_mundane  - Odorf
*/
void do_npc_commune(P_char ch)
{
  int circle, available, need_mem = 0;

  int max_circle = get_max_circle(ch);
  for (circle = max_circle; circle > 0; circle--)
  {
    available = ch->specials.undead_spell_slots[circle];
    if (available != max_spells_in_circle(ch, circle, max_circle))
    {
      need_mem = circle;
      break;
    }
  }

  if (need_mem && !get_scheduled(ch, event_memorize))
  {
    add_event(event_memorize, get_circle_memtime(ch, need_mem), ch, 0, 0, 0, 0, 0);
  }
}

//  recount_spells_in_use(ch);

#define MEM_LIST       BIT_1
#define MEM_SPELL      BIT_2
#define MEM_SILENT     BIT_3
#define MEM_FULL       BIT_4

void do_memorize(P_char ch, char *argument, int cmd)
{
  int      circle, i, time, shown_one, first_to_mem, spl;
  unsigned short mode;
  struct affected_type *af, new_mem;
  char     memorized[MAX_CIRCLE + 1];
  char     per_spell[LAST_SPELL + 1];
  P_obj    sbook = NULL;
  P_nevent  e1;

  if( cmd == CMD_ASSIMILATE || cmd == CMD_COMMUNE || cmd == CMD_TUPOR
    || cmd == CMD_FOCUS || cmd == CMD_DEFOREST )
  {
    do_assimilate(ch, argument, cmd);
    return;
  }

  if( cmd == CMD_PRAY && !praying_class(ch) )
  {
    send_to_char("The gods ignore you.\n", ch);
    return;
  }
  else if( cmd == CMD_MEMORIZE && (IS_PUNDEAD(ch) || USES_TUPOR(ch) || !meming_class(ch)
    || IS_UNDEADRACE(ch) || is_wearing_necroplasm(ch)) || (IS_ANGEL(ch)) )
  {
    if( IS_ANGEL(ch) )
    {
      send_to_char("Try using Assimilate in this form.\n", ch);
    }
    else
    {
      send_to_char("You aren't trained in magic.\n", ch);
    }
    return;
  }

  if( argument )
  {
    argument = skip_spaces(argument);
  }

  if( !strcmp(argument, "nl") || !strcmp(argument, "silent") )
  {
    mode = MEM_LIST | MEM_SILENT;
  }
  else if (!argument || !*argument)
  {
    mode = MEM_LIST;
  }
  else
  {
    mode = MEM_SPELL;
  }
  // Remove any invalid crap spells. Happens with multiclassing/deleveling/etc.
  bad_spell_check(ch);

  memset(&memorized, 0, sizeof(memorized));
  memset(&per_spell, 0, sizeof(per_spell));
  first_to_mem = 0;
  e1 = get_scheduled(ch, (event_func) event_memorize);

  for (af = ch->affected; af; af = af->next)
  {
    if (af->type == TAG_MEMORIZE)
    {
      if (af->flags & MEMTYPE_FULL)
      {
        per_spell[af->modifier]++;
      }
      else if (!first_to_mem)
      {
        first_to_mem = af->modifier;
      }
      circle = get_spell_circle(ch, af->modifier);
      if (circle <= MAX_CIRCLE)
      {
        memorized[circle]++;
      }
    }
  }
  if( mode & MEM_LIST )
  {
    for (*Gbuf1 = '\0', circle = get_max_circle(ch); circle > 0; circle--)
    {
      shown_one = FALSE;
      for (spl = FIRST_SPELL; spl <= LAST_SPELL; spl++)
      {
        if (per_spell[spl] && get_spell_circle(ch, spl) == circle)
        {
          if (!shown_one)
          {
            shown_one = TRUE;
            sprintf(Gbuf1 + strlen(Gbuf1), "(%2d%s circle) %2d - %s\n", circle, circle == 1 ? "st"
              : circle == 2 ? "nd" : circle == 3 ? "rd" : "th", per_spell[spl], skills[spl].name);
          }
          else
          {
            sprintf(Gbuf1 + strlen(Gbuf1), "              %2d - %s\n",
              per_spell[spl], skills[spl].name);
          }
        }
      }
    }
    if( *Gbuf1 && !(mode & MEM_SILENT) )
    {
      send_to_char("You have memorized the following spells:\n", ch);
    }

    if( first_to_mem )
    {
      if( *Gbuf1 )
      {
        strcat(Gbuf1, "\nAnd you ");
      }
      else
      {
        strcat(Gbuf1, "You ");
      }

      if( meming_class(ch) )
      {
        strcat(Gbuf1, "are currently memorizing the following spells:\n");
      }
      else
      {
        strcat(Gbuf1, "are currently praying for the following spells:\n");
      }

      if( e1 )
      {
        time = ne_event_time(e1);
        if (e1->data)
        {
          time += *((int *) e1->data) / 2;
        }
        shown_one = TRUE;
      }
      else
      {
        time = 0;
        shown_one = FALSE;
      }

      for (af = ch->affected; af; af = af->next)
      {
        if (af->type == TAG_MEMORIZE && (af->flags & MEMTYPE_FULL) == 0)
        {
          circle = get_spell_circle(ch, af->modifier);
          if (shown_one)
          {
            shown_one = FALSE;
          }
          else
          {
            time += get_circle_memtime(ch, circle, true);
          }
          sprintf(Gbuf1 + strlen(Gbuf1), "%5d seconds:  (%2d%s) %s%s\n", time / 4, circle, (circle == 1)
            ? "st" : (circle == 2) ? "nd" : (circle == 3) ? "rd" : "th", skills[af->modifier].name,
            book_class(ch) ? (SpellInSpellBook(ch, af->modifier, (SBOOK_MODE_IN_INV | SBOOK_MODE_AT_HAND
            | SBOOK_MODE_ON_BELT | SBOOK_MODE_ON_GROUND)) ? "" : "  [not in spell book]") : "");
        }
      }
    }

    if (meming_class(ch))
    {
      strcat(Gbuf1, "\nYou can memorize ");
    }
    else
    {
      strcat(Gbuf1, "\nYou can pray ");
    }
    i = 0;
    for (circle = 1; circle <= get_max_circle(ch); circle++)
    {
      if (max_spells_in_circle(ch, circle) > memorized[circle])
      {
        if (i)
        {
          if (i > 1)
          {
            strcat(Gbuf1, ", ");
          }
          strcat(Gbuf1, Gbuf2);
        }
        i++;
        sprintf(Gbuf2, "%d %d%s", max_spells_in_circle(ch, circle) - memorized[circle],
          circle, (circle == 1) ? "st" : (circle == 2) ? "nd" : (circle == 3) ? "rd" : "th");
      }
    }

    if( !i )
    {
      strcat(Gbuf1, "no more spells.\n");
    }
    else
    {
      if( i > 1 )
      {
        strcat(Gbuf1, " and ");
      }
      strcat(Gbuf1, Gbuf2);
      strcat(Gbuf1, " circle spell(s).\n");
    }
    if( !(mode & MEM_SILENT) )
    {
      send_to_char(Gbuf1, ch);
    }
    if( first_to_mem && !e1 && GET_STAT(ch) == STAT_RESTING &&
      (GET_POS(ch) == POS_SITTING || GET_POS(ch) == POS_KNEELING) )
    {
      if( GET_CLASS(ch, CLASS_SHAMAN) )
      {
        send_to_char("You continue your study.\n", ch);
        strcpy(Gbuf1, "$n stares off into space, entering a meditative trance.");
      }
      else if (book_class(ch))
      {
        sbook = SpellInSpellBook(ch, first_to_mem, SBOOK_MODE_IN_INV | SBOOK_MODE_AT_HAND |
          SBOOK_MODE_ON_BELT | SBOOK_MODE_ON_GROUND);
        send_to_char("You continue your study.\n", ch);
        if( sbook )
          strcpy(Gbuf1, "$n opens $p and begins studying it intently.");
        else
          strcpy(Gbuf1, "$n looks down at $s lap and begins studying it intently.");
      }
      else
      {
        send_to_char("You continue your praying.\n", ch);
        sprintf(Gbuf1, "$n takes out $s %sholy symbol and begins praying intently.",
          IS_EVIL(ch) ? "un" : "");
      }
      act(Gbuf1, TRUE, ch, sbook, 0, TO_ROOM);
      time = get_circle_memtime(ch, get_spell_circle(ch, first_to_mem));
      add_event(event_memorize, time / 2, ch, 0, 0, 0, &time, sizeof(time));
      SET_BIT(ch->specials.affected_by2, AFF2_MEMORIZING);
    }
    return;
  }

  if( GET_STAT(ch) != STAT_RESTING ||
      (GET_POS(ch) != POS_SITTING && GET_POS(ch) != POS_KNEELING) )
  {
    send_to_char("You are not comfortable enough to study. (try sitting/resting)\n", ch);
    return;
  }

  spl = search_block(argument, (const char **) spells, FALSE);

  if( spl <= -1 || !skills[spl].spell_pointer )
  {
    if( book_class(ch) )
    {
      send_to_char("Um.. what spell are you trying to memorize?\n", ch);
    }
    else
    {
      send_to_char("Um.. what spell are you trying to pray for?\n", ch);
    }
    return;
  }

  if( book_class(ch) )
  {
    sbook = SpellInSpellBook(ch, spl, SBOOK_MODE_IN_INV | SBOOK_MODE_AT_HAND
      | SBOOK_MODE_ON_BELT | SBOOK_MODE_ON_GROUND);
  }

  circle = get_spell_circle(ch, spl);

#if !defined(CHAOS_MUD) || (CHAOS_MUD != 1)
  if( circle > get_max_circle(ch) && !quested_spell(ch, spl) )
  {
    if( (!book_class(ch) || sbook) && !quested_spell(ch, spl) )
    {
      send_to_char("That is too powerful an enchantment for you to master.. yet, anyway.\n", ch);
      return;
    }
    else if( !sbook )
    {
      send_to_char("Yes, you _HAVE_ heard of rumors about that spell.. but you think it beyond your powers anyway.\n", ch);
      return;
    }
  }
  else if( !sbook && book_class(ch) )
  {
    send_to_char("Sorry, but you haven't got that spell in any available spellbooks!\n", ch);
    return;
  }
  else if( !SKILL_DATA_ALL(ch, spl).maxlearn[0] && !SKILL_DATA_ALL(ch, spl).maxlearn[ch->player.spec] && !quested_spell(ch, spl) )
#else
  if( !SKILL_DATA_ALL(ch, spl).maxlearn[0] && !SKILL_DATA_ALL(ch, spl).maxlearn[ch->player.spec] && !quested_spell(ch, spl) )
#endif
  {
    send_to_char("That spell is beyond your comprehension.\n", ch);
    return;
  }
  else if( memorized_in_circle(ch, circle) >= max_spells_in_circle(ch, circle) )
  {
    send_to_char("You can't hold any more spells in your thought.\n", ch);
    return;
  }

  memset(&new_mem, 0, sizeof(new_mem));
  new_mem.type = TAG_MEMORIZE;
  new_mem.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY | AFFTYPE_PERM;
  new_mem.duration = -1;
  new_mem.modifier = spl;
  affect_to_end(ch, &new_mem);
  time = get_circle_memtime(ch, get_spell_circle(ch, spl));

  if( !IS_AFFECTED2(ch, AFF2_MEMORIZING) )
  {
    if( GET_CLASS(ch, CLASS_SHAMAN) )
    {
      strcpy(Gbuf1, "$n stares off into space, entering a meditative trance.");
    }
    else if( !book_class(ch) )
    {
      sprintf(Gbuf1, "$n takes out $s %sholy symbol and begins praying intently.",
        IS_EVIL(ch) ? "un" : "");
    }
    else
    {
      strcpy(Gbuf1, "$n opens $p and begins studying it intently.");
    }
    act(Gbuf1, TRUE, ch, sbook, 0, TO_ROOM);
    add_event(event_memorize, time / 2, ch, 0, 0, 0, &time, sizeof(time));
  }
  SET_BIT(ch->specials.affected_by2, AFF2_MEMORIZING);
  if (GET_CLASS(ch, CLASS_SHAMAN))
  {
    sprintf(Gbuf1, "You are memorizing %s, which will take about %d seconds.\n",
      skills[spl].name, time / 4);
  }
  else if( !book_class(ch) )
  {
    sprintf(Gbuf1, "You are praying for %s, which will take about %d seconds.\n",
      skills[spl].name, time / 4);
  }
  else
  {
    sprintf(Gbuf1, "You are memorizing %s, which will take about %d seconds.\n",
      skills[spl].name, time / 4);
  }
  send_to_char(Gbuf1, ch);
}

#undef MEM_LIST
#undef MEM_SPELL
#undef MEM_SILENT
#undef MEM_FULL

/*
 * forget spells, depending on argument spell:
 * -1 : forget all
 *  0 : forget only non-ready slots
 *  forget one spell of given number
 */
int forget_spells(P_char ch, int spell)
{
  struct affected_type *af, *next_af, *full_matching = 0;
  bool     was_first = TRUE;

  for (af = ch->affected; af; af = next_af)
  {
    next_af = af->next;
    if (af->type == TAG_MEMORIZE)
    {
      if (spell == -1)
        affect_remove(ch, af);
      else if (spell == 0)
      {
        if (!(af->flags & MEMTYPE_FULL))
          affect_remove(ch, af);
      }
      else if (af->modifier == spell)
      {
        if (af->flags & MEMTYPE_FULL)
        {
          full_matching = af;
          continue;
        }
        affect_remove(ch, af);
        if (was_first && IS_AFFECTED2(ch, AFF2_MEMORIZING))
        {
          send_to_char("Damn!  Lost your train of thought!\n", ch);
          stop_memorizing(ch);
        }
        return 1;
      }
      else if (!(af->flags & MEMTYPE_FULL))
        was_first = FALSE;
    }
  }

  if (full_matching)
  {
    affect_remove(ch, full_matching);
    return 1;
  }

  if (spell <= 0)
    stop_memorizing(ch);

  return 0;
}

void do_forget(P_char ch, char *argument, int cmd)
{
  int      spl, i;
  struct memorize_data *mem_ptr, *ptr;

  if (!ch || IS_NPC(ch))
    return;

  argument = skip_spaces(argument);

  if (!(*argument))
  {
    send_to_char("You forget all the spells you are trying to "
                 "memorize.\n", ch);
    forget_spells(ch, 0);
    return;
  }
  else if (!str_cmp(argument, "all"))
  {
    send_to_char("You forget all the memorized spells.\n", ch);
    forget_spells(ch, -1);
    return;
  }

  spl = search_block(argument, (const char **) spells, FALSE);
  if (spl == -1 || !skills[spl].spell_pointer)
  {
    send_to_char("Um.. what spell are you trying to forget?\n", ch);
    return;
  }

  if (forget_spells(ch, spl))
  {
    sprintf(Gbuf1, "You purge '%s' from your thoughts.\n", skills[spl].name);
    send_to_char(Gbuf1, ch);
  }
  else
    send_to_char("You don't have that spell in your memory!\n", ch);
}

int knows_spell(P_char ch, int spell)
{
  int      circle;

  if( IS_TRUSTED(ch) )
    return TRUE;

  if( !IS_SPELL(spell) || !ch->player.m_class )
    return FALSE;

  if( IS_PC(ch) && !USES_SPELL_SLOTS(ch) && has_memorized(ch, spell) )
    return -1;

  // Check for specs (mostly only concerns druids, as this check is done for others at mem/pray time)
  if( IS_PC(ch) )
  {
    if( !SKILL_DATA_ALL(ch, spell).maxlearn[0] && !SKILL_DATA_ALL(ch, spell).maxlearn[ch->player.spec] )
    {
      return FALSE;
    }
  }

  circle = get_spell_circle(ch, spell);
  return (circle > get_max_circle(ch)) ? 0 : circle;
}

void *has_memorized(P_char ch, int spell)
{
  struct affected_type *af;

  for (af = ch->affected; af; af = af->next)
    if (af->type == TAG_MEMORIZE && (af->flags & MEMTYPE_FULL) &&
        af->modifier == spell)
      return af;

  return NULL;
}

void use_spell(P_char ch, int spell)
{
  struct affected_type *af;
  char buf[128];
  int spatial = GET_CHAR_SKILL(ch, SKILL_SPATIAL_FOCUS);

  if( !IS_ALIVE(ch) )
    return;

  if(IS_TRUSTED(ch))
  {
    // Immortals don't use up spells.
  }
  else if(USES_MANA(ch))
  {
    if((int) (spatial / 2) > number(1, 100))
    {
      send_to_char("&+MFocusing your mind, you bend reality a bit more easily...&n\n", ch);
      GET_MANA(ch) -= (int) (get_spell_circle(ch, spell) * 35 / 10);
      StartRegen(ch, EVENT_MANA_REGEN);
    }
    else
    {
      GET_MANA(ch) -= get_spell_circle(ch, spell) * MANA_PER_CIRCLE;
      StartRegen(ch, EVENT_MANA_REGEN);
    }
  }
  else if(USES_SPELL_SLOTS(ch))
  {
    // Elite mobs should not lose spell slots as quickly. Jun09 -Lucrot
    if( IS_NPC(ch) && IS_ELITE(ch) && number(0, 1) )
    {
      return;
    }

    if( GET_CHAR_SKILL(ch, SKILL_DEVOTION) )
    {
      if( MIN(GET_CHAR_SKILL(ch, SKILL_DEVOTION)/10, 5) + 4 > number(0,100) )
      {
        sprintf(buf, "%s's grace flows down on you refreshing your spell power!&n\n", get_god_name(ch));
        send_to_char( buf, ch );
        return;
      }
    }
    if(ch->specials.undead_spell_slots[get_spell_circle(ch, spell)] > 0)
    {
      ch->specials.undead_spell_slots[get_spell_circle(ch, spell)] -= 1;
    }

    if( (USES_COMMUNE(ch) || USES_FOCUS(ch) || USES_DEFOREST(ch)) && !get_scheduled(ch, event_memorize) )
    {
      add_event(event_memorize, get_circle_memtime(ch, get_spell_circle(ch, spell)), ch, 0, 0, 0, 0, 0);
    }
  }
  else
  {
    // Elite mobs should not lose spell slots as quickly. Jun09 -Lucrot
    if( IS_NPC(ch) && IS_ELITE(ch) && number(0, 1))
    {
      return;
    }

    if( GET_CHAR_SKILL(ch, SKILL_DEVOTION) )
    {
      if( MIN(GET_CHAR_SKILL(ch, SKILL_DEVOTION)/10, 5) + 4 > number(0,100) )
      {
        sprintf(buf, "%s's grace flows down on you restoring the power of &+W%s.&n\n",
          get_god_name(ch), skills[spell].name);
        send_to_char(buf, ch);
        return;
      }
    }
    for (af = ch->affected; af; af = af->next)
    {
      if( af->type == TAG_MEMORIZE && (af->flags & MEMTYPE_FULL) && af->modifier == spell )
      {
        if( SKILL_DATA_ALL(ch, spell).maxlearn[0] || SKILL_DATA_ALL(ch, spell).maxlearn[ch->player.spec] )
        {
          affect_to_end(ch, af);
          af->flags ^= MEMTYPE_FULL;
        }
        else
        {
          affect_remove(ch, af);
        }
        return;
      }
    }
  }
}

struct extra_descr_data *find_spell_description(P_obj obj)
{
  struct extra_descr_data *tmp;

  for (tmp = obj->ex_description; tmp; tmp = tmp->next)
    if (tmp->keyword && tmp->description)
      if ((tmp->keyword[0] == 3) && (tmp->keyword[1] == 1) &&
          (tmp->keyword[2] == 3))
        return tmp;

  return NULL;
}

P_obj Find_process_entry(P_char ch, P_obj foo, int spl)
{
  struct extra_descr_data *foo2;
  int      i, b, c;

  if (foo->type == ITEM_SPELLBOOK)
  {
    if ((foo2 = find_spell_description(foo)))
    {
      b = foo->value[0];
      c = foo->value[1];
      if ((c == 0) || (c > (1 << CLASS_COUNT)))
        return NULL;
      if ((b <= TONGUE_USING) || (b > TONGUE_LASTHEARD))
        return NULL;
/*      if (IS_PC (ch) && (GET_LANGUAGE (ch, b) <= 40))
   return NULL;
 */
      if (SpellInThisSpellBook(foo2, spl))
        return foo;
    }
  }
  else if (foo->type == ITEM_SCROLL)
    for (i = 1; i < 4; i++)
      if ((foo->value[i] >= 0) && (foo->value[i] == spl))
        return foo;
  return NULL;
}

P_obj FindSpellBookWithSpell(P_char ch, int spl, int mode)
{
  P_obj    foo, foo2;

  if( (foo = find_gh_library_book_obj(ch)) && IS_SET(mode, SBOOK_MODE_ON_GROUND))
  {
    return foo;
  }

  /*
     current format: value[0] = language spell is in value[1] = class the
     spellbook is for value[2] = number of pages in spellbook value[3] = number

     of pages _used_

     extra desc with keyword '\03\01\03\00' contains all the neccessary spells.

   */
  char wizBookName[512];
  sprintf(wizBookName, "bookof%s", ch->player.name);


  if (IS_SET(mode, SBOOK_MODE_AT_HAND))
    if (!IS_SET(mode, SBOOK_MODE_NO_BOOK))
      if ((foo2 = SpellBookAtHand(ch)))
        if ((foo = Find_process_entry(ch, foo2, spl)))
          return foo;
  if (IS_SET(mode, SBOOK_MODE_IN_INV))
    for (foo = ch->carrying; foo; foo = foo->next_content)
      if ((foo->type == ITEM_SPELLBOOK) && CAN_SEE_OBJ(ch, foo))
      {
        if (foo->R_num == real_object(31) && isname(wizBookName, foo->name))
          return foo;
        if (!IS_SET(mode, SBOOK_MODE_NO_BOOK))
        {
          if ((foo2 = Find_process_entry(ch, foo, spl)))
            return foo2;
        }
      }
      else if ((foo->type == ITEM_SCROLL) && CAN_SEE_OBJ(ch, foo) &&
               !IS_SET(mode, SBOOK_MODE_NO_SCROLL))
        if ((foo2 = Find_process_entry(ch, foo, spl)))
          return foo2;

  if (IS_SET(mode, SBOOK_MODE_ON_BELT))
  {
    int      beltSlot;

    for (beltSlot = WEAR_ATTACH_BELT_1; beltSlot <= WEAR_ATTACH_BELT_3;
         beltSlot++)
    {
      foo = ch->equipment[beltSlot];
      if (!foo)
        continue;
      if (foo->R_num == real_object(31) && isname(wizBookName, foo->name))
        return foo;
      if ((foo->type == ITEM_SPELLBOOK)
          && CAN_SEE_OBJ(ch, foo)
          && !IS_SET(mode, SBOOK_MODE_NO_BOOK)
          && (foo2 = Find_process_entry(ch, foo, spl)))
        return foo2;
    }
  }
  return NULL;
}

int AddSpellToSpellBook(P_char ch, P_obj obj, int spl)
{
  struct extra_descr_data *tmp;

  if (!(tmp = find_spell_description(obj)))
  {
    /*
       creation blues
     */

    CREATE(tmp, extra_descr_data, 1, MEM_TAG_EXDESCD);

    tmp->next = obj->ex_description;
    obj->ex_description = tmp;
    sprintf(Gbuf1, "%c%c%c", (char) 3, (char) 1, (char) 3);
    tmp->keyword = str_dup(Gbuf1);
    CREATE(tmp->description, char, (MAX_SKILLS + 1) / 8 + 1, MEM_TAG_BUFFER);
  }
  if (SpellInThisSpellBook(tmp, spl))
    return FALSE;
  if (!obj->value[1])
    obj->value[1] = ch->player.m_class;
  if (!obj->value[0])
    obj->value[0] = TONGUE_MAGIC;

  /* not there, we've gotta add it */
  SET_BIT(tmp->description[spl / 8], 1 << (spl % 8));

  /*
     ok, we done, we happy campers.
   */
  return TRUE;
}

struct scribing_data_type *scribing_base, *scribing_free_base = NULL;

void AddScribingAffect(P_char ch)
{
  int wait_rounds = 2;
  
  if( GET_CHAR_SKILL(ch, SKILL_SCRIBE_MASTERY) )
    wait_rounds--;

  CharWait(ch, MAX(1, wait_rounds * PULSE_VIOLENCE) );
  
  SET_BIT(ch->specials.affected_by2, AFF2_SCRIBING);
}

void add_scribe_data(int spl, P_char ch, P_obj book, int flag, P_obj obj,
                     P_char teacher, void (*done_func) (P_char))
{
  struct scribing_data_type tmp;

  memset(&tmp, 0, sizeof(tmp));

  tmp.ch = ch;
  tmp.book = book;
  /*
     added check -- DTS 7/11/95
   */
  tmp.flag = (flag == 1 ?
              (((tmp.source.obj) && (tmp.source.obj->type == ITEM_SCROLL))
               ? 2 : 1) : 0);
  if (flag)
    tmp.source.obj = obj;
  else
    tmp.source.teacher = teacher;
  tmp.spell = spl;

  tmp.done_func = done_func;
  /*
     ok, now doing some heavy calculation pagetime's unit is 1/4 second
   */
/*
   tmp->pagetime =
   MAX(2, ((38 - STAT_INDEX(GET_C_DEX(ch))) / 3)) *
   MAX(1, ((38 - STAT_INDEX(GET_C_INT(ch))) / 2)) *
   get_spell_circle(ch, spl) * 4 / MAX(5, GET_LEVEL(ch)) *
   (200 - GET_CHAR_SKILL(ch, SKILL_SCRIBE)) / 100;
 */
  AddScribingAffect(ch);
  if (!flag)
    if (teacher)
      AddScribingAffect(teacher);
  add_event(event_scribe, 4 - GET_CHAR_SKILL(ch, SKILL_SCRIBE) / 33, ch, 0,
            0, 0, &tmp, sizeof(tmp));
}

/*
   flag has diff modes: 0 = teach, 1 = scribe
 */
/*
   obj empty if teach mode, contains spellbook/scroll otherwise
 */
/*
   teacher = fellow who's teaching char something
 */

void add_scribing(P_char ch, int spl, P_obj book, int flag, P_obj obj,
                  P_char teacher)
{
  if (flag || !teacher)
    send_to_char("You start to copy down the spell..\n", ch);
  else
  {
    act("You start to teach $N..", TRUE, teacher, 0, ch, TO_CHAR);
    sprintf(Gbuf1, "$n starts to teach you %s..", skills[spl].name);
    act(Gbuf1, TRUE, teacher, 0, ch, TO_VICT );
  }
  add_scribe_data(spl, ch, book, flag, obj, teacher);

  int wait_rounds = 3;
  wait_rounds -= (int) ( GET_CHAR_SKILL(ch, SKILL_SCRIBE) / 45 );
  wait_rounds -= (int) ( GET_CHAR_SKILL(ch, SKILL_SCRIBE_MASTERY) / 20 );
//  debug("wait_rounds: %d", wait_rounds);
  CharWait(ch, MAX(1, wait_rounds * PULSE_VIOLENCE) );
  
  if( !flag )
  {
    wait_rounds = 3;
    wait_rounds -= (int) ( GET_CHAR_SKILL(ch, SKILL_SCRIBE_MASTERY) / 20 );
//    debug("wait_rounds: %d", wait_rounds);
    CharWait(ch, MAX(1, wait_rounds * PULSE_VIOLENCE) );
  }
}

int ScriberSillyChecks(P_char ch, int spl)
{
  P_obj    o1, o2, o3;
  struct extra_descr_data *d;

  if (IS_AFFECTED2(ch, AFF2_MEMORIZING))
  {
    send_to_char("You're busy enough as it is!\n", ch);
    return FALSE;
  }
  if (IS_AFFECTED2(ch, AFF2_SCRIBING))
  {
    send_to_char("Sorry, you cannot speed up your scribing by doing multiple jobs at same time, \n"
      "nor teach/scribe at same time.\nYou are not UNIX compatible.\n*bonk*\n\n", ch);
    return FALSE;
  }
  if( (GET_STAT(ch) != STAT_RESTING) || ((GET_POS(ch) != POS_SITTING) && (GET_POS(ch) != POS_KNEELING)) )
  {
    send_to_char("You don't feel comfortable enough to start to scribe.\n", ch);
    return FALSE;
  }

  // Modified by DTS 7/9/95 - yet another possible null ptr. dereference
  if (!(o1 = ch->equipment[WIELD]) || !(o2 = ch->equipment[HOLD]) )
  {
    send_to_char("You lack one of the necessary implements for scribing!\n", ch);
    return FALSE;
  }

  // If o1 is a PEN, swap o1 and o2, so o2 is the PEN, and o1 is the SPELLBOOK, hopefully.
  if( o1->type == ITEM_PEN )
  {
    o3 = o1;
    o1 = o2;
    o2 = o3;
  }

  if( o2->type != ITEM_PEN )
  {
    send_to_char("You need a quill with which to scribe to spellbook! [held]\n", ch);
    return FALSE;
  }
  if( o1->type != ITEM_SPELLBOOK )
  {
    send_to_char("You need a spellbook to write to at hand! [held]\n", ch);
    return FALSE;
  }
/*
  if( o1->value[0] && (GET_LANGUAGE(ch, o1->value[0]) < 70) )
  {
    send_to_char ("You need to be more fluent in the language before adding anything to\nthe book at hand.\n", ch);
    return FALSE;
  }
  if( o1->value[1] && !GET_CLASS(ch, o1->value[1]) )
  {
    send_to_char("The spellbook you are holding is of the wrong class so you can't add to it..\n", ch);
    return FALSE;
  }*/

#if 0 // nah, I gladly let fellow waste his spellbook-space by copying unusable spells
  o3 = FindSpellBookWithSpell(ch, spl, SBOOK_MODE_IN_INV | SBOOK_MODE_ON_BELT | SBOOK_MODE_ON_GROUND );
  if( o3 && (o3->type == ITEM_SPELLBOOK) && !((o3->value[1] == 0) || (o3->value[1] == o1->value[1])) )
  {
    send_to_char("You cannot copy spells from one classes spellbook to another!\n", ch);
    return;
  }
#endif

  d = find_spell_description(o1);
  if( d && SpellInThisSpellBook(d, spl) )
  {
    send_to_char("You have the spell already in your spellbook, boggle?\n", ch);
    return FALSE;
  }
  if( (GetSpellPages(ch, spl) + o1->value[3]) > o1->value[2] )
  {
    send_to_char("Sorry, the spellbook at hand has no room for more spells.\n", ch);
    return FALSE;
  }

  // Added to prevent scribing/teaching quest spells
  // Added what??

  return TRUE;
}

P_obj SpellBookAtHand(P_char ch)
{
  P_obj    o1, o2;

  o1 = ch->equipment[WIELD];
  o2 = ch->equipment[HOLD];
  if (o1 && (o1->type == ITEM_SPELLBOOK) && CAN_SEE_OBJ(ch, o1))
    return o1;
  if (o2 && (o2->type == ITEM_SPELLBOOK) && CAN_SEE_OBJ(ch, o2))
    return o2;
  return NULL;
}

void do_teach(P_char ch, char *arg, int cmd)
{
  P_char   target;
  P_obj    o3;

  int      spl, tmp;

  if( !IS_ALIVE(ch) || !ch->in_room )
  {
    if( !ch )
    {
      debug( "do_teach: ch: NULL, arg: '%s', cmd: %d!", arg, cmd );
    }
    else if( !IS_ALIVE(ch) )
    {
      debug( "do_teach: DEAD ch: '%s', arg: '%s', cmd: %d!", J_NAME(ch), arg, cmd );
    }
    else
    {
      debug( "do_teach: ch not in room!: ch: '%s', arg: '%s', cmd: %d!", arg, cmd );
    }

    return;
  }

  if( !arg || !*arg )
  {
    if( IS_AFFECTED2(ch, AFF2_SCRIBING) )
    {
      do_scribe(ch, arg, CMD_SCRIBE);
    }
    else
    {
      send_to_char("usage: teach char spellname\n", ch);
    }
    return;
  }
  if( !book_class(ch) && !IS_TRUSTED(ch) )
  {
    send_to_char("Teaching others spells just isn't in your field of work!\n", ch);
    return;
  }
  /*
     hurm. the next things that need to be done: - spell exists - that teacher
     _has_ skill to teach to target, if teacher exists - target is ready to be
     teached - some misc checks - addition of the scribing_array with the usual

     params
   */
  arg = one_argument(arg, Gbuf1);
  if( !(target = get_char_room(Gbuf1, ch->in_room)) )
  {
    send_to_char("Teach who?\n", ch);
    return;
  }
  if( !CAN_SEE(ch, target) )
  {
    send_to_char("You cannot see anyone with such name here!\n", ch);
    return;
  }
  if( ch == target )
  {
    send_to_char("You believe you know, or don't know, that spell already!\n", ch);
    return;
  }
  arg = skip_spaces(arg);
  spl = old_search_block(arg, 0, strlen(arg), (const char **) spells, 0);
  if (spl != -1)
  {
    spl--;
  }
  if( spl == -1 || get_spell_circle(target, (tmp = spl)) > MAX_CIRCLE )
  {
    send_to_char("Teach _WHAT_ spell?? That your target cannot learn, at least.\n", ch);
    return;
  }
  if( !(o3 = FindSpellBookWithSpell(ch, tmp, SBOOK_MODE_IN_INV | SBOOK_MODE_ON_BELT))
    && !IS_TRUSTED(ch) && !IS_NPC(ch) )
  {
    send_to_char("You don't have such spell in your _own_ spellbooks!\n", ch);
    return;
  }
  /* ok, we've gotta valid spell _and_ target */

  if( !ScriberSillyChecks(target, tmp) )
  {
    send_to_char("Your student isn't ready to start yet!\n", ch);
    return;
  }
  add_scribing(target, tmp, SpellBookAtHand(target), 0, 0, ch);
}

void do_scribe(P_char ch, char *arg, int cmd)
{
  int    spl = 0;
  P_obj  o1, o2, o3;
  struct scribing_data_type *tmp_s;
  P_char teacher;

  // No scribing services, at least of now. we'll see later tho.
  if( IS_NPC(ch) )
  {
    return;
  }

  if( !GET_CHAR_SKILL(ch, SKILL_SCRIBE) )
  {
    send_to_char("You don't know how to scribe!\n", ch);
    return;
  }
  if( !arg || !*arg )
  {
    if (!IS_AFFECTED2(ch, AFF2_SCRIBING))
    {
      send_to_char("Scribe what?\n", ch);
    }
    return;
  }
  arg = skip_spaces(arg);

  // Added the scribe all option
  if( !str_cmp(arg, "all") )
  {
    send_to_char( "Scribing all spells:\n", ch );
    // This requires a check for the book in the room of knowledge.
    prac_all_spells(ch);
    return;
  }

  spl = old_search_block(arg, 0, strlen(arg), (const char **) spells, 0);
  if( spl != -1 )
  {
    spl--;
  }
  if( spl == -1 )
  {
    send_to_char("Scribe _WHAT_ spell?\n", ch);
    return;
  }

  if( !ScriberSillyChecks(ch, spl) )
  {
    return;
  }

  o1 = ch->equipment[WIELD];
  o2 = ch->equipment[HOLD];

  // Shortened this and made it make sense.  Book in other hand.
  if( o1->type != ITEM_SPELLBOOK )
  {
    // ScriberSillyChecks -> o1 and o2 are spellbook and pen (or pen and spellbook).
    o1 = o2;
  }

  // If ch doesn't have a book with spl in it already,
  if( !(o3 = FindSpellBookWithSpell(ch, spl, SBOOK_MODE_IN_INV | SBOOK_MODE_ON_BELT | SBOOK_MODE_ON_GROUND)) )
  {
    // Only fail if there's no teacher to teach it.
    teacher = FindTeacher(ch);
    if( !knows_spell(ch, spl) )
    {
      send_to_char("That is too powerful an enchantment for you to master.. right now.\n", ch);
      return;
    }
    else if( !teacher )
    {
      send_to_char("You don't have that spell in any of your books or scrolls in learnable form!\n", ch);
      return;
    }
    else if( !knows_spell(teacher, spl) )
    {
      mobsay( teacher, "Yes, I have heard of rumors about that spell.. but I do not know it." );
      return;
    }
    else
    {
      add_scribing(ch, spl, o1, FALSE, NULL, teacher );
      return;
    }
  }
  // Otherwise, learn from the book.
  add_scribing(ch, spl, o1, TRUE, o3, NULL);
}

void event_scribe(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct scribing_data_type *s_data = (struct scribing_data_type *) data;
  int      i, j;

  /*
     what this proc does? simple: - checks if in proper state (resting,
     spellbook in one, quill in other hand). - if not so, it does some nasty
     things (aborts the memorization prematurely and informs the pc) - else, it

     just adds one more page.. and queries a new event.
   */
  if ((GET_STAT(ch) != STAT_RESTING) ||
      ((GET_POS(ch) != POS_SITTING) && (GET_POS(ch) != POS_KNEELING)) ||
      (!ch->equipment[WIELD] || !ch->equipment[HOLD]) ||
      (ch->equipment[WIELD]->type != ITEM_SPELLBOOK &&
       ch->equipment[HOLD]->type != ITEM_SPELLBOOK) ||
      (ch->equipment[WIELD]->type != ITEM_PEN &&
       ch->equipment[HOLD]->type != ITEM_PEN) || (!s_data || !(s_data->book)) ||
      ((s_data->book != ch->equipment[HOLD]) && (s_data->book != ch->equipment[WIELD])))
  {
    disarm_char_nevents(ch, event_scribe);
    send_to_char("So much for that scribing effort!\n", ch);
    REMOVE_BIT(ch->specials.affected_by2, AFF2_SCRIBING);
    return;
  }

  s_data->page++;
  s_data->book->value[3]++;

  if (s_data->page >= GetSpellPages(ch, s_data->spell))
  {
    REMOVE_BIT(ch->specials.affected_by2, AFF2_SCRIBING);
    sprintf(Gbuf1, "You finish scribing the spell '%s' into $p.",
            skills[s_data->spell].name);
    act(Gbuf1, TRUE, ch, s_data->book, 0, TO_CHAR);
    if (!s_data->flag && s_data->source.teacher)
    {
      sprintf(Gbuf1, "You finish teaching the spell '%s' to %s.\n",
              skills[s_data->spell].name, GET_NAME(ch));
      send_to_char(Gbuf1, s_data->source.teacher);

      REMOVE_BIT(s_data->source.teacher->specials.affected_by2, AFF2_SCRIBING);
    }
    if (s_data->flag == 2)
    {
      j = 0;
      for (i = 1; i < 4; i++)
        if (s_data->source.obj->value[i] >= 1)
          j++;
      if (j == 1)
      {
        send_to_char("The ancient parchment comes apart in your hands..\n", ch);
        extract_obj(s_data->source.obj, TRUE); // Artifact scroll??
      }
      else if (j == 0)
      {
        send_to_char("Error detected - please report this to a coder imm. [0 spells in scroll].\n", ch);
        return;
      }
      else
      {
        j = 0;
        for (i = 1; ((i < 4) && !j); i++)
          if (s_data->source.obj->value[i] == s_data->spell)
          {
            j = 1;
            s_data->source.obj->value[i] = 0;
            send_to_char("You mess up the scroll so badly that part of it becomes unreadable!\n", ch);
          }
      }
    }
    /*
       ok, those few funny things done.. let's do something else:
     */
    if (!AddSpellToSpellBook(ch, s_data->book, s_data->spell))
    {
      send_to_char("Hmm, you have that spell in your spellbook already???\n", ch);
      return;
    }
    if (s_data->book->value[2] - s_data->book->value[3])
    {
      sprintf(Gbuf1,
              "The spell uses %d pages in $p, leaving %d more pages free.",
              GetSpellPages(ch, s_data->spell),
              s_data->book->value[2] - s_data->book->value[3]);
      act(Gbuf1, TRUE, ch, s_data->book, 0, TO_CHAR);
    }
    else
    {
      send_to_char
        ("The spell used the rest of the empty pages in the book.\n", ch);
    }
    notch_skill(ch, SKILL_SCRIBE, 17);
  }
  else
  {
    send_to_char
      ("You complete another page of the spell into your spellbook..\n", ch);
    /*
       ok, let's schedule next event:
     */
    add_event(event_scribe, MAX(1, (int) 4 - (GET_CHAR_SKILL(ch, SKILL_SCRIBE) / 33) - (GET_CHAR_SKILL(ch, SKILL_SCRIBE_MASTERY) / 20)), ch, 0,
              0, 0, s_data, sizeof(*s_data));
             return;
  }
  if (s_data->done_func) s_data->done_func(ch);
}


void spell_mordenkainens_lucubration(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if( !ch )
    return;

  char buff[MAX_STRING_LENGTH];

  act("&+C$n releases a &+Ymassive&+C cloud of energy which $e immediately consumes, leaving $m with a dark grin...", TRUE, ch, 0, 0, TO_ROOM);
  
  sprintf(buff, "&+CYour spell releases a &+Ymassive&+C cloud of energy which you instantly focus into your &+mmind&+C...\n");
  send_to_char(buff, ch);

  if(!USES_COMMUNE(ch) && 
     !GET_CLASS(ch, CLASS_WARLOCK) &&
     !(IS_PUNDEAD(ch) ||
       USES_TUPOR(ch) ||
       IS_UNDEADRACE(ch) |
       is_wearing_necroplasm(ch)) ||
     !USES_FOCUS(ch))
  {
    // normal casters
    
    struct affected_type *af;
    for (af = ch->affected; af; af = af->next)
    {
      if (af->type == TAG_MEMORIZE && 
          !IS_SET(af->flags, MEMTYPE_FULL) &&
          af->modifier != SPELL_MORDENKAINENS_LUCUBRATION &&
          number(0,100) < (int) get_property("spell.mordenkainensLucubration.spellMemChance", 80) )
      {
        af->flags |= MEMTYPE_FULL;
        sprintf(buff, "&+WYou regain the power of %s!\n", spells[af->modifier]);
        send_to_char(buff, ch);
      }
    }
  }
  else
  {
    // assimilating casters
    
    for( int i = 1; i < get_max_circle(ch); i++ )
    {
      if (ch->specials.undead_spell_slots[i] >= max_spells_in_circle(ch, i))
        continue;
      
      for( int j = ch->specials.undead_spell_slots[i]; j < max_spells_in_circle(ch, i); j++ )
      {
        if( number(0,100) < (int) get_property("spell.mordenkainensLucubration.spellMemChance", 80) )
        {
          ch->specials.undead_spell_slots[i]++;
          sprintf(buff, "&+WYou regain power in the %d%s circle of magic!\n",
                  i, i == 1 ? "st" : (i == 2 ? "nd" : (i == 3 ? "rd" : "th")));
          send_to_char(buff, ch);
        }
      }
      
    }
    
  }
}

// Returns the circle of the spell
// If spec is 0, returns circle for base class.
// If spec > 0 returns circle for spec spell or MAX_CIRCLE+1 if not specifically a spec spell.
int get_spell_circle( int cls, int spec, int spl )
{
  // If we're not looking at a spec..
  if( spec == 0 )
  {
    // Just check the base for a rlevel.
    if( skills[spl].m_class[cls-1].rlevel[0] )
    {
      return skills[spl].m_class[cls-1].rlevel[0];
    }
    else
    {
      return MAX_CIRCLE+1;
    }
  }
  else
  {
    // If it's a pure spec spell..
    // Note: Lost spells will return a value of 0 or less or MAX_CIRCLE+1.
    if( !skills[spl].m_class[cls-1].rlevel[0]
      && skills[spl].m_class[cls-1].rlevel[spec] )
    {
      return skills[spl].m_class[cls-1].rlevel[spec];
    }
    else
    {
      return MAX_CIRCLE+1;
    }
  }
}

// Returns the level of the skill
// If spec is 0, returns level for base class.
// If spec > 0 returns level for spec skill or MAXLVLMORTAL+1 if not specifically a spec skill.
int get_skill_level( int cls, int spec, int skl )
{
  // If we're not looking at a spec..
  if( spec == 0 )
  {
    // Just check the base for a rlevel.
    if( skills[skl].m_class[cls-1].rlevel[0] )
    {
      return skills[skl].m_class[cls-1].rlevel[0];
    }
    else
    {
      return MAXLVLMORTAL+1;
    }
  }
  else
  {
    // If it's a pure spec skill..
    // Note: Lost skills will return a value of 0 or less.
    if( skills[skl].m_class[cls-1].rlevel[0] != skills[skl].m_class[cls-1].rlevel[spec] )
    {
      return skills[skl].m_class[cls-1].rlevel[spec];
    }
    else
    {
      return MAXLVLMORTAL+1;
    }
  }
}

// Returns the level of the song
// If spec is 0, returns level for base class.
// If spec > 0 returns level for spec skill or MAXLVLMORTAL+1 if not specifically a spec skill.
int get_song_level( int cls, int spec, int sng )
{
  // If we're not looking at a spec..
  if( spec == 0 )
  {
    // Just check the base for a rlevel.
    if( skills[sng].m_class[cls-1].rlevel[0] )
    {
      return skills[sng].m_class[cls-1].rlevel[0];
    }
    else
    {
      return MAXLVLMORTAL+1;
    }
  }
  else
  {
    // If it's a pure spec song..
    // Note: Lost songs will return a value of 0 or less.
    if( skills[sng].m_class[cls-1].rlevel[0] != skills[sng].m_class[cls-1].rlevel[spec] )
    {
      return skills[sng].m_class[cls-1].rlevel[spec];
    }
    else
    {
      return MAXLVLMORTAL+1;
    }
  }
}

// This function goes through and removes any spells that the char has in
//   their mem list that they don't have (via spec or multi or whatever).
// Note: This does not check number of spells in circle in list vs max.
void bad_spell_check( P_char ch )
{
  struct affected_type *paf, *paf_next;
  int circle;

  for( paf = ch->affected; paf; paf = paf_next )
  {
    paf_next = paf->next;

    if( paf->type == TAG_MEMORIZE )
    {
      circle = get_spell_circle(ch, paf->modifier);
      // If the spell is not valid for the char anymore (for whatever reason).
      if( circle > MAX_CIRCLE )
      {
        debug( "Found bad spell '%s' %d in mem list of char '%s'.", skills[paf->modifier].name, paf->modifier, J_NAME(ch) );
        affect_remove(ch, paf);
      }
    }
  }
}
