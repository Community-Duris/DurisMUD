/*
 * ***************************************************************************
 * *  File: files.c                                            Part of Duris *
 * *  Usage: save/restore player and ecorpse information to/from disk
 * * *  Written by: Andrew Choi,  Modified by: John Bashaw
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <netinet/in.h>
#include "spells.h"
#include "files.h"
#include "justice.h"
#include "structs.h"
#include "config.h"
#include "mm.h"
#include "utils.h"
#include "prototypes.h"
#include "ships.h"
#include "random.zone.h"
#include "necromancy.h"
#include "sql.h"
#include "trophy.h"
#include "vnum.obj.h"

#include <iostream>
using namespace std;

extern P_char character_list;
extern P_event event_list;
extern int mini_mode;
extern P_index mob_index;
extern P_desc descriptor_list;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
static P_event save_event;
char *ibuf;
static int corpse_room;
static int int_size = sizeof(int);
static int long_size = sizeof(long);
static int save_count;
static int short_size = sizeof(short);
static int stat_vers, obj_vers, aff_vers, skill_vers, witness_vers;
extern struct shop_data *shop_index;
extern struct hold_data TmpAffs;
extern struct mm_ds *dead_mob_pool;
//extern struct mm_ds *dead_construction_pool;
extern struct mm_ds *dead_trophy_pool;
extern struct mm_ds *dead_witness_pool;
extern struct mm_ds *dead_crime_pool;
extern struct mm_ds *dead_house_pool;
//extern P_house first_house;
extern int LOADED_RANDOM_ZONES;
extern struct random_zone random_zone_data[101];

extern P_index obj_index;
extern P_room world;
extern const char *class_types[];
extern const int min_stats_for_class[][8];
extern const struct racial_data_type racial_data[];
extern int pulse;
extern int innate_abilities[];
extern int class_innates[][5];
extern int innate2_abilities[];
extern int class_innates2[][5];
extern const int top_of_world;

#ifndef _PFILE_
extern Skill skills[];
#endif
#ifdef _PFILE_
char     buff[SAV_MAXSIZE];
char    *buf = buff;
int      skill_off, affect_off, item_off;
#endif

/* local globals */
static P_obj save_equip[MAX_WEAR];


int      anchor_room(int room);
int      calculate_hitpoints(P_char ch);
int      calculate_mana(P_char ch);
P_nevent get_scheduled(P_obj obj, event_func func);
void     proclib_obj_event(P_char, P_char, P_obj obj, void*);
void     event_poison(P_char, P_char, P_obj obj, void*);
void     event_short_affect(P_char, P_char , P_obj , void *);

struct ship_reg_node *ship_reg_db = NULL;

/*
 * current save file method, all character data is written to a string,
 * which is then written in one operation.  Pluses:  Only one disk access
 * per save so it's damn fast.  Minuses: if string gets too long, boom!
 * Max size is found in SAV_MAXSIZE define.  Data format for save files is
 * found below after the #if 0. -JAB
 */

#if 0
/*
 * save file structure, if you change it, change this as well, so we know
 * what the hell it's doing.
 *
 * key: b - byte, c - string, i - int, l - long, s - short.
 *
 * comments preceded by **.
 *
 * b SAV_SAVEVERS b sizeof(short) b sizeof(int) b sizeof(long) b save type
 *
 * i skill_offset i affect_offset i item_offset i size_offset
 *
 * i save room l save time
 *
 * ** unequip everything and remove all affects before saving char_data
 *
 * b SAV_STATVERS (stat_vers) c GET_NAME b only.pc->screen_length c
 * only.pc->pwd c player.short_descr c player.long_descr c
 * player.description c GET_TITLE b GET_CLASS b GET_RACE b GET_LEVEL b
 * GET_SEX s GET_WEIGHT s GET_HEIGHT (b GET_SIZE, if stat_vers 8+) i GET_HOME i GET_BIRTHPLACE
 * (stat_vers 6+)
 *
 * l player.time.birth i playing time (secs) l save time (again??) s
 * perm_aging (stat_vers 15+)
 *
 * b TROPHYCOUNT,  b * (i vnum, i kills)
 *
 * s MAX_TONGUE b*MAX_TONGUE only.pc->talks[]
 *
 * b GET_STR  (stat_ver < 13) b GET_ADD  (stat_ver < 13) b GET_INT
 * (stat_ver < 13) b GET_WIS  (stat_ver < 13) b GET_DEX  (stat_ver < 13) b
 * GET_CON  (stat_ver < 13)
 *
 * b STR    (stat_ver > 12) b DEX    (stat_ver > 12) b AGI    (stat_ver >
 * 12) b CON    (stat_ver > 12) b POW    (stat_ver > 12) b INT
 * (stat_ver > 12) b WIS    (stat_ver > 12) b CHA    (stat_ver > 12) b
 * KARMA  (stat_ver > 12) b LUCK   (stat_ver > 12)
 *
 * s GET_MANA s points.max_mana s GET_HIT s points.max_hit s GET_VITALITY s
 * points.max_vitality
 *
 * s 100  AC       (stat_vers < 11) b 0    hitroll  (stat_vers < 11) b 0
 *  damroll  (stat_vers < 11)
 *
 * i GET_COPPER i GET_SILVER i GET_GOLD i GET_PLATINUM i GET_EXP i max_exp
 * (stat_vers 15+)
 *
 * b ch->specials.position  (stat_vers < 11) b default position
 * (stat_vers < 11)
 *
 * i specials.act i specials.act2 i only.pc->spells_to_learn
 * i specials.alignment i prestige
 *
 * b 5  # of conditions     (stat_vers < 11) b*3 specials.conditions[]
 *
 * c only.pc->poofIn c only.pc->poofOut
 *
 * b 5  # of saving throws  (stat_vers < 11) s*5
 * specials.apply_saving_throw[]  (stat_vers < 11)
 *
 * b only.pc->echo_toggle s only.pc->prompt l only.pc->wiz_invis l
 * only.pc->law_flags  (pflags in stat_vers < 11) s only.pc->wimpy s
 * only.pc->aggressive
 *
 * i GET_BALANCE_COPPER i GET_BALANCE_SILVER i GET_BALANCE_GOLD i
 * GET_BALANCE_PLATINUM
 *
 *
 * b SAV_SKILLVERS (skill_vers)
 *
 * i number of skills { b skills[].learned b skills[].memorized }*number
 * of skills
 *
 * s MAX_SKILL_USAGE { l char_skill_usage[].time_of_first_use b
 * char_skill_usage[].times_used }*MAX_SKILL_USAGE
 *
 * ** save witness record   (TASFALEN)
 *
 * b SAVE_WTNSVERS (witness_vers)
 *
 * i number of record { s attacker s victim i time i crime i room }
 *
 * b SAV_AFFVERS (aff_vers)
 *
 * s number of affects
 *
 * { b type s duration s modifier b location l bitvector l bitvector2
 * }*number of affects
 *
 * ** add back affects and reequip char if we are not extracting them
 *
 * b SAV_ITEMVERS (obj_vers)
 *
 * i number of items being saved
 *
 * ** equip first, then inven
 *
 * ** New method(s) of saving objects: ** note that order is NOT
 * preserved, as it's arbitrary in the first place.
 *
 * { ** all objects have these first two values:
 *
 * b storage_type_flag i Vnum
 *
 * ** utility bytes, how many and meaning based on storage_type_flag
 *
 * ** if was worn: b location
 *
 * ** if has 'count': s count
 *
 * ** if item is non-stock: s unique_flags
 *
 * ** only the values that varied from 'stock' are saved, as follows: { c
 * name c short_description c description c action_description i*4
 * value[0-3] b type i wear_flags i extra_flags i weight i cost l
 * bitvector l bitvector2 { b obj->affected[0].location b
 * obj->affected[0].modifier b obj->affected[1].location b
 * obj->affected[1].modifier } }
 *
 * ** if type == ITEM_SPELLBOOK & has some spells in it.. b array length
 * if 0, no stuff. just in case. byte_array [MAX_SKILLS/8+1] spells
 *
 * ** if an object contained other objects, the contained objects will
 * follow the container, and the list will be terminated with a flag byte.
 * Nested containers are not a problem, as the routines are recursive.
 *
 * ** greatest savings are found with large numbers of the same 'stock'
 * object (like iron rations), old system required 56 bytes for each, new
 * system only needs 7 bytes for the lot of them.
 *
 * ** restore reads 'flag' and Vnum, loads one if possible, then alters
 * just loaded object (if needed) and puts in it container or adds it to
 * the equip list (equipping items is done last, so that worn containers
 * have correct weight, etc).  If an item can no longer be worn (it
 * changed flags since it was saved, etc), it stays in inventory.  If a
 * container has changed it's holding capacity, or some of it's contents
 * have changed weight, they also load in inventory, rather than
 * overloading the container.
 *
 * ** FUTURE: If an item can't be loaded (it's been removed for game, it's
 * zone isn't loaded, etc) a new object 'missing item token' is created and
 * the saved data is written into it's long_description, this item itself
 * can be saved, and used to restore the item later (if the zone is
 * missing, an error was made, etc), the token weighs 0, is transient, and
 * may not be altered in any way (except to be destroyed), is hidden, and
 * may not be found, and does not count against number of items in inven.
 * Basically, the player won't know it's there, but an immortal will see it
 * if they look at the player (they load in inven)
 *
 * ** under old system, strung objects reverted and there was a 292 object
 * limit.  under new system, limit is more like 2500+, but having lots of
 * 'strung' items will shrink that number alot.
 *
 * ** For most players, the save files will be much smaller (not to mention
 * that containers now restore with contents intact.)
 *
 */
#endif

// bv6 would be in here, but there's no unique flag for it yet

#define ObjectsMatch(obj, control) \
(((obj)->str_mask == (control)->str_mask) && \
 ((obj)->affected[0].location == (control)->affected[0].location) && \
 ((obj)->affected[0].modifier == (control)->affected[0].modifier) && \
 ((obj)->affected[1].location == (control)->affected[1].location) && \
 ((obj)->affected[1].modifier == (control)->affected[1].modifier) && \
 ((obj)->extra_flags == (control)->extra_flags) && \
 ((obj)->extra2_flags == (control)->extra2_flags) && \
 ((obj)->anti_flags == (control)->anti_flags) && \
 ((obj)->anti2_flags == (control)->anti2_flags) && \
 ((obj)->bitvector == (control)->bitvector) && \
 ((obj)->bitvector2 == (control)->bitvector2) && \
 ((obj)->bitvector3 == (control)->bitvector3) && \
 ((obj)->bitvector4 == (control)->bitvector4) && \
 ((obj)->value[0] == (control)->value[0]) && \
 ((obj)->value[1] == (control)->value[1]) && \
 ((obj)->value[2] == (control)->value[2]) && \
 ((obj)->value[3] == (control)->value[3]) && \
 ((obj)->value[4] == (control)->value[4]) && \
 ((obj)->value[5] == (control)->value[5]) && \
 ((obj)->value[6] == (control)->value[6]) && \
 ((obj)->value[7] == (control)->value[7]) && \
 ((obj)->timer[0] == (control)->timer[0]) && \
 ((obj)->timer[1] == (control)->timer[1]) && \
 ((obj)->timer[2] == (control)->timer[2]) && \
 ((obj)->timer[3] == (control)->timer[3]) && \
 ((obj)->wear_flags == (control)->wear_flags) && \
 ((obj)->weight == (control)->weight) && \
 ((obj)->material == (control)->material) && \
 ((obj)->cost == (control)->cost) && \
 ((obj)->type == (control)->type))

/*
 * following are functions that write data to disk (or prepare data for
 * writing).  JAB
 */

int writeStatus(char *buf, P_char ch, bool updateTime )
{
  char    *start = buf;
  int      tmp, i;
  long     tmpl;
  struct affected_type *af, *next_af;
/*  sh_int dummy_short = 0; */

  ADD_BYTE(buf, (char) SAV_STATVERS);
  ADD_STRING(buf, GET_NAME(ch));
  ADD_INT(buf, GET_PID(ch));
  ADD_BYTE(buf, ch->only.pc->screen_length);
  ADD_STRING(buf, ch->only.pc->pwd);
  ADD_STRING(buf, ch->player.short_descr);
  ADD_STRING(buf, ch->player.long_descr);
  ADD_STRING(buf, ch->player.description);
  ADD_STRING(buf, GET_TITLE(ch));
  ADD_INT(buf, ch->player.m_class);
  ADD_INT(buf, ch->player.secondary_class);
  ADD_BYTE(buf, ch->player.spec);

  //NEVER SAVE THE "WRONG" RACE.. Kvark
  if ((af = get_spell_from_char(ch, TAG_RACE_CHANGE)) != NULL)
  {
    ADD_BYTE(buf, af->modifier);
  }
  else
  {
    ADD_BYTE(buf, GET_RACE(ch));
  }
  
  ADD_BYTE(buf, GET_RACEWAR(ch));
  ADD_BYTE(buf, GET_LEVEL(ch));
  ADD_BYTE(buf, GET_SEX(ch));
  ADD_SHORT(buf, ch->player.weight);
  ADD_SHORT(buf, ch->player.height);
  ADD_BYTE(buf, GET_SIZE(ch));
  ADD_INT(buf, GET_HOME(ch));
  ADD_INT(buf, GET_BIRTHPLACE(ch));
  ADD_INT(buf, GET_ORIG_BIRTHPLACE(ch));

  ADD_LONG(buf, ch->player.time.birth);
  if( updateTime )
  {
    tmpl = time(0);
    tmp = ch->player.time.played + (int) (tmpl - ch->player.time.logon);
    ADD_INT(buf, tmp);            /* player age in secs */
    ADD_LONG(buf, tmpl);          /* last save time */
  }
  else
  {
    ADD_INT(buf, ch->player.time.played);
    ADD_LONG(buf, ch->player.time.saved);
  }
  ADD_SHORT(buf, ch->player.time.perm_aging);

  for (i = 0; i < MAX_CIRCLE + 1; i++)
    ADD_BYTE(buf, ch->specials.undead_spell_slots[i]);

  ADD_INT(buf, 0);              //!!! last_level

  ch->player.time.saved = tmpl;

  for (i = 0; i < NUMB_PC_TIMERS; i++)
    ADD_LONG(buf, ch->only.pc->pc_timer[i]);

  //XXX
//  if (0 && ch->only.pc->trophy)
//  {
//    struct trophy_data *tr;
//
//    tmp = 0;
//    for (tr = ch->only.pc->trophy; tr; tr = tr->next)
//      tmp++;
//    ADD_BYTE(buf, tmp);
//    for (tr = ch->only.pc->trophy; tr; tr = tr->next)
//    {
//      ADD_INT(buf, tr->vnum);
//      ADD_INT(buf, tr->kills);
//    }
//  }
//  else
//  {
//    ADD_BYTE(buf, 0);
//  }

  ADD_SHORT(buf, (short) MAX_TONGUE);
  for (tmp = 0; tmp < MAX_TONGUE; tmp++)
    ADD_BYTE(buf, GET_LANGUAGE(ch, tmp));
  ADD_SHORT(buf, (short) MAX_INTRO);
  for (tmp = 0; tmp < MAX_INTRO; tmp++)
  {
    ADD_INT(buf, ch->only.pc->introd_list[tmp]);
    ADD_LONG(buf, ch->only.pc->introd_times[tmp]);
  }
   for (tmp = 0; tmp < MAX_FORGE_ITEMS; tmp++)
       {      
         ADD_INT(buf, ch->only.pc->learned_forged_list[tmp]);
       }
   
  ADD_BYTE(buf, ch->base_stats.Str);
  ADD_BYTE(buf, ch->base_stats.Dex);
  ADD_BYTE(buf, ch->base_stats.Agi);
  ADD_BYTE(buf, ch->base_stats.Con);
  ADD_BYTE(buf, ch->base_stats.Pow);
  ADD_BYTE(buf, ch->base_stats.Int);
  ADD_BYTE(buf, ch->base_stats.Wis);
  ADD_BYTE(buf, ch->base_stats.Cha);
  ADD_BYTE(buf, ch->base_stats.Kar);
  ADD_BYTE(buf, ch->base_stats.Luk);

  ADD_SHORT(buf, GET_MANA(ch));
  ADD_SHORT(buf, ch->points.base_mana);
  // save difference instead cur HP (Lom)
//  ADD_SHORT(buf, MAX(1, GET_HIT(ch)));
  ADD_SHORT(buf, MAX(0, GET_MAX_HIT(ch)-GET_HIT(ch)));
  ADD_BYTE(buf, ch->only.pc->spells_memmed[MAX_CIRCLE]);
  ADD_SHORT(buf, ch->points.base_hit);
  ADD_SHORT(buf, GET_VITALITY(ch));
  ADD_SHORT(buf, ch->points.base_vitality);

  ADD_INT(buf, GET_COPPER(ch));
  ADD_INT(buf, GET_SILVER(ch));
  ADD_INT(buf, GET_GOLD(ch));
  ADD_INT(buf, GET_PLATINUM(ch));
  ADD_INT(buf, GET_EXP(ch));
  ADD_INT(buf, 0);
  ADD_INT(buf, ch->only.pc->epics);
  ADD_INT(buf, ch->only.pc->epic_skill_points);
  ADD_INT(buf, ch->only.pc->skillpoints);
  ADD_INT(buf, ch->only.pc->spell_bind_used);

  ADD_INT(buf, ch->specials.act);
  ADD_INT(buf, ch->specials.act2);
  ADD_INT(buf, ch->only.pc->vote);
  ADD_INT(buf, ch->specials.alignment);
  ADD_INT(buf, 0);
  ADD_SHORT(buf, ch->only.pc->prestige);
  ADD_SHORT(buf, ch->specials.guild);
  ADD_INT(buf, ch->specials.guild_status);
  ADD_LONG(buf, ch->only.pc->time_left_guild);
  ADD_BYTE(buf, ch->only.pc->nb_left_guild);
  ADD_LONG(buf, ch->only.pc->time_unspecced);

  ADD_LONG(buf, ch->only.pc->frags);
  ADD_LONG(buf, ch->only.pc->oldfrags);

  /* granted data */

  ADD_INT(buf, ch->only.pc->numb_gcmd);
  for (tmp = 0; tmp < ch->only.pc->numb_gcmd; tmp++)
    ADD_INT(buf, ch->only.pc->gcmd_arr[tmp]);

  for (tmp = 0; tmp < MAX_COND; tmp++)
    ADD_BYTE(buf, ch->specials.conditions[tmp]);
  ADD_STRING(buf, ch->only.pc->poofIn);
  ADD_STRING(buf, ch->only.pc->poofOut);
  ADD_STRING(buf, ch->only.pc->poofInSound);
  ADD_STRING(buf, ch->only.pc->poofOutSound);
  ADD_BYTE(buf, ch->only.pc->echo_toggle);
  ADD_SHORT(buf, ch->only.pc->prompt);
  ADD_LONG(buf, ch->only.pc->wiz_invis);
  ADD_LONG(buf, ch->only.pc->law_flags);
  ADD_SHORT(buf, ch->only.pc->wimpy);
  ADD_SHORT(buf, ch->only.pc->aggressive);
  ADD_BYTE(buf, ch->only.pc->highest_level);
  ADD_INT(buf, GET_BALANCE_COPPER(ch)); /* bank account */
  ADD_INT(buf, GET_BALANCE_SILVER(ch)); /* bank account */
  ADD_INT(buf, GET_BALANCE_GOLD(ch));   /* bank account */
  ADD_INT(buf, GET_BALANCE_PLATINUM(ch));       /* bank account */
  ADD_LONG(buf, ch->only.pc->numb_deaths);


   ADD_INT(buf, ch->only.pc->quest_active);
  ADD_INT(buf, ch->only.pc->quest_mob_vnum);
  ADD_INT(buf, ch->only.pc->quest_type);
  ADD_INT(buf, ch->only.pc->quest_accomplished );
  ADD_INT(buf, ch->only.pc->quest_started );
  ADD_INT(buf, ch->only.pc->quest_zone_number );
  ADD_INT(buf, ch->only.pc->quest_giver);
  ADD_INT(buf, ch->only.pc->quest_level );
  ADD_INT(buf, ch->only.pc->quest_receiver );
  ADD_INT(buf, ch->only.pc->quest_shares_left );
  ADD_INT(buf, ch->only.pc->quest_kill_how_many);
  ADD_INT(buf, ch->only.pc->quest_kill_original);
  ADD_INT(buf, ch->only.pc->quest_map_room );
  ADD_INT(buf, ch->only.pc->quest_map_bought );


  return (int) (buf - start);
}

// This function updates ch's short affect durations for saving purposes.
//   When called, it walks through ch's affects and finds the corresponding
//   event for each short affect.  It then updates the duration of the affect
//   with the time left on the event.
void updateShortAffects( P_char ch )
{
  affected_type *paf = ch->affected;
  P_nevent pnev;

  // Look through each of ch's affects.
  while( paf != NULL )
  {
    // If it's a short affect,
    if( IS_SET( paf->flags, AFFTYPE_SHORT ) )
    {
      // Find the corresponding event.
      LOOP_EVENTS_CH( pnev, ch->nevents )
      {
        // If the event is a short affect wear-off event and it corresponds to paf.
        if( pnev->func == event_short_affect && pnev->data != NULL
          && ((struct event_short_affect_data*)pnev->data)->af == paf )
        {
          break;
        }
      }
      if( pnev )
      {
        // Update the duration (in pulses).
        paf->duration = ne_event_time( pnev );
      }
      else
      {
        debug( "updateShortAffects: Couldn't find event for short affect '%s', timer %d, on '%s'.",
          (paf->type > 0) ? skills[paf->type].name : "Unknown", paf->duration, J_NAME(ch) );
      }
    }
    paf = paf->next;
  }
}

// Writes the list of affects starting with af to the string buf.
// af->duration updated with updateShortAffects(ch); please make sure this is called,
//   or you will have timers being reset each time a player rents out / re enters game.
int writeAffects( char *buf, struct affected_type *af )
{
  struct affected_type *first = af;
  P_event  tmp;
  char    *start = buf;
  signed short count = 0;

  ADD_BYTE(buf, (char) SAV_AFFVERS);

  while( af )
  {
    if( !IS_SET(af->flags, AFFTYPE_NOSAVE) )
    {
      count++;
    }
    af = af->next;
  }
  ADD_SHORT(buf, count);

  for( af = first; af; af = af->next )
  {
    byte     custom_messages = 0;       /* 0 - none, 1 - to_char, 2 - to_room, 3 - both */

    if( IS_SET(af->flags, AFFTYPE_NOSAVE) )
    {
      continue;
    }

#ifndef _PFILE_
    if (af->wear_off_message_index != 0)
    {
      if (skills[af->type].wear_off_char[af->wear_off_message_index]) ;
      custom_messages = 1;
      if (skills[af->type].wear_off_room[af->wear_off_message_index]) ;
      custom_messages |= 2;
    }
#endif

    ADD_BYTE(buf, custom_messages);

#ifndef _PFILE_
    if (custom_messages & 1)
      ADD_STRING(buf, skills[af->type].wear_off_char[af->wear_off_message_index]);
    if (custom_messages & 2)
      ADD_STRING(buf, skills[af->type].wear_off_room[af->wear_off_message_index]);
#endif

    ADD_SHORT(buf, af->type);
    if( IS_SET(af->flags, AFFTYPE_SHORT) )
    {

#ifndef _PFILE_
      for (tmp = event_list; tmp; tmp = tmp->next_event)
        if ((struct affected_type *) tmp->target.t_arg == af)
          break;

      if (tmp != NULL)
      {
        ADD_INT(buf, event_time(tmp, T_PULSES));
      }
      else
#endif
        // af->duration updated with updateShortAffects(ch), but we want secs to save not pulses.
        ADD_INT(buf, af->duration/WAIT_SEC);
    }
    else
    {
      ADD_INT(buf, af->duration);
    }

    ADD_SHORT(buf, af->flags);
    ADD_INT(buf, af->modifier);
    ADD_BYTE(buf, af->location);
    ADD_LONG(buf, af->bitvector);
    ADD_LONG(buf, af->bitvector2);
    ADD_LONG(buf, af->bitvector3);
    ADD_LONG(buf, af->bitvector4);
    ADD_LONG(buf, af->bitvector5);
    ADD_LONG(buf, 0);           //af->bitvector6);
  }

  return (int) (buf - start);
}

int writeSkills(char *buf, P_char ch, int num)
{
  char    *start = buf;
  int      i;
  struct memorize_data *tmp;

  ADD_BYTE(buf, (char) SAV_SKILLVERS);

  /* Save the spell memorized, and skill usages info -DCL */

  ADD_INT(buf, (int) num);
  for (i = 0; i < num; i++)
  {
    ADD_BYTE(buf, ch->only.pc->skills[i].learned);
    ADD_BYTE(buf, ch->only.pc->skills[i].taught);
    ADD_BYTE(buf, 0);
  }
  ADD_INT(buf, 0);

  ADD_SHORT(buf, (short) 0);

  return (int) (buf - start);
}

/* recursive count of all objects in inventory */
#ifndef _PFILE_

int countInven(P_obj obj)
{
  int      count = 0;

  while (obj)
  {
    if (!IS_SET(obj->extra_flags, ITEM_NORENT))
      count++;
    if (obj->contains)
      count += countInven(obj->contains);
    obj = obj->next_content;
  }

  return count;
}

int countEquip(P_char ch)
{
  int      i, count = 0;

  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
    {
      if (!IS_SET(ch->equipment[i]->extra_flags, ITEM_NORENT))
        count++;
      if (ch->equipment[i]->contains)
        count += countInven(ch->equipment[i]->contains);
    }
    else if (save_equip[i])
    {
      if (!IS_SET(save_equip[i]->extra_flags, ITEM_NORENT))
        count++;
      if (save_equip[i]->contains)
        count += countInven(save_equip[i]->contains);
    }
  return count;
}

/*
 * compares obj to control and sets flag bits for differences, returns 32
 * bit flag value.  JAB
 */

ulong ObjUniqueFlags(P_obj obj, P_obj control)
{
  // mask the flag to ONLY see the O_U_ flags which corrospond directly to STRUNG_* bits
  ulong    flag = (ulong) (obj->str_mask & (O_U_KEYS | O_U_DESC1 | O_U_DESC2 | O_U_DESC3));
  int      i;

  if (obj->str_mask & STRUNG_EDESC)
    flag |= O_U_EDESC;

  for (i = 0; i < 4; i++)
    if (obj->value[i] != control->value[i])
      flag |= 1 << (i + 4);

  if (obj->value[4] != control->value[4])
    flag |= O_U_VAL4;

  if (obj->value[5] != control->value[5])
    flag |= O_U_VAL5;

  if (obj->value[6] != control->value[6])
    flag |= O_U_VAL6;

  if (obj->value[7] != control->value[7])
    flag |= O_U_VAL7;

  for (i = 0; i < 4; i++)
    if (obj->timer[i] != control->timer[i])
      flag |= O_U_TIMER;

  if (obj->type != control->type)
    flag |= O_U_TYPE;

  if (obj->trap_charge != control->trap_charge)
    flag |= O_U_TRAP;

  if (obj->wear_flags != control->wear_flags)
    flag |= O_U_WEAR;

  if (obj->extra_flags != control->extra_flags)
    flag |= O_U_EXTRA;

  if (obj->anti_flags != control->anti_flags)
    flag |= O_U_ANTI;

  if (obj->anti2_flags != control->anti2_flags)
    flag |= O_U_ANTI2;

  if (obj->extra2_flags != control->extra2_flags)
    flag |= O_U_EXTRA2;

  if (obj->weight != control->weight)
    flag |= O_U_WEIGHT;

  if (obj->material != control->material)
    flag |= O_U_MATERIAL;

//  if (obj->space != control->space)
//    flag |= O_U_SPACE;

  if (obj->cost != control->cost)
    flag |= O_U_COST;

  if (obj->bitvector != control->bitvector)
    flag |= O_U_BV1;

  if (obj->bitvector2 != control->bitvector2)
    flag |= O_U_BV2;

  if (obj->bitvector3 != control->bitvector3)
    flag |= O_U_BV3;

  if (obj->bitvector4 != control->bitvector4)
    flag |= O_U_BV4;

  if (obj->bitvector5 != control->bitvector5)
    flag |= O_U_BV5;

  if ((obj->affected[0].location != control->affected[0].location) ||
      (obj->affected[0].modifier != control->affected[0].modifier) ||
      (obj->affected[1].location != control->affected[1].location) ||
      (obj->affected[1].modifier != control->affected[1].modifier) ||
      (obj->affected[1].location != control->affected[2].location) ||
      (obj->affected[1].modifier != control->affected[2].modifier) ||
      (obj->affected[1].location != control->affected[3].location) ||
      (obj->affected[1].modifier != control->affected[3].modifier))
    flag |= O_U_AFFS;

  return flag;
}

/* write a list of objects (and recursively it's contents...)  JAB */

bool writeObjectlist(P_obj obj, int loc)
{
  int      i, done[4000], done_num = 0, cont_wgt, count;
  P_obj    t_obj = NULL, obj2 = NULL, obj_c = NULL, t_obj2 = NULL, w_obj;
  byte     o_f_flag;
  ulong    o_u_flag;
  bool     skip;

  for (w_obj = obj; w_obj; w_obj = obj2)
  {
    obj2 = w_obj->next_content;
    obj_c = w_obj->contains;
    o_f_flag = 0;
    o_u_flag = 0;
    cont_wgt = 0;
    count = 1;

    if (t_obj && (t_obj->R_num != w_obj->R_num))
    {
      extract_obj(t_obj);
      t_obj = NULL;
    }
    if (!t_obj)
      if (!(t_obj = read_object(w_obj->R_num, REAL)))
      {
        logit(LOG_DEBUG, "writeObjectlist(): obj %d [%d] not loadable",
              w_obj->R_num, obj_index[w_obj->R_num].virtual_number);
        continue;
      }
    if (OBJ_WORN(w_obj) || (loc && (save_equip[loc - 1] == w_obj)))
      o_f_flag |= O_F_WORN;

    if (obj_c)
    {
      o_f_flag |= O_F_CONTAINS;
      for (t_obj2 = obj_c; t_obj2; t_obj2 = t_obj2->next_content)
        cont_wgt += GET_OBJ_WEIGHT(t_obj2);
      w_obj->weight -= cont_wgt;
    }
    if (w_obj->type == ITEM_SPELLBOOK && find_spell_description(w_obj))
      o_f_flag |= O_F_SPELLBOOK;

    if (w_obj->affects)
      o_f_flag |= O_F_AFFECTS;

    if ((o_u_flag = ObjUniqueFlags(w_obj, t_obj)))
      o_f_flag |= O_F_UNIQUE;

// temoprary for fixing messed items
    if (t_obj->extra_flags & ITEM_ALLOWED_CLASSES)
      w_obj->extra_flags |= ITEM_ALLOWED_CLASSES;

    if (t_obj->extra_flags & ITEM_ALLOWED_RACES)
      w_obj->extra_flags |= ITEM_ALLOWED_RACES;

    /*
     * ok, to make this work properly, we need to check for stock
     * (only) items at each level (main carried list, and each
     * container count as a seperate 'level' of storage) and store
     * them all together, thus common things like rations, don't take
     * up inordinate space.  But, to avoid huge cpu overhead, we have
     * to keep track, since this is a recursive routine, we store
     * Rnums of already counted objects, to avoid duplication.
     */

    if (o_f_flag)
    {

      /*
       * by definition, if this flag is set, COUNT cannot apply
       * without going to absurd lengths, so we just write it out,
       * call to write it's contents (if any), and move on.
       */

		if (!IS_SET(w_obj->extra_flags, ITEM_NORENT))
		   ibuf += writeObject(w_obj, o_f_flag, o_u_flag, (ush_int) 1, loc, ibuf);

      if (obj_c)
        w_obj->weight += cont_wgt;

      if (obj_c)
        if (!writeObjectlist(obj_c, (byte) 0))
          return FALSE;

      continue;

    }
    else
    {

      /*
       * ok, to keep things sane cpu-wise, but allow us to reduce
       * duplicate items to minimal storage requirements, we do
       * this:
       *
       * the done[] array holds R_nums of non-unique items already
       * counted and stored, if the current object is on this list,
       * it skips it and doesn't store it again.  If current object
       * is NOT already in the array, it adds it to the list, then
       * scans the rest of the list for duplicates, counts them, and
       * if they are contiguous, advances the object pointer so we
       * don't have to look at it ever again.  This should
       * (theoretically anyway) be lots faster than writing every
       * object without checking. Unfortunately, the need to check
       * each item for uniqueness as well, can slow things down
       * quite a bit.
       *
       * Note: count only applies to current 'level' of objects, if
       * a player has 10 rations in inven and 12 more in a bag,
       * there will be two entries for rations, one for 10 and
       * another for 12.  Have to do it this way to avoid having to
       * store pointers, etc.
       */

      for (i = done_num - 1; i >= 0; i--)
        if (done[i] == w_obj->R_num)
          break;

      /* found it, it's already been counted, so skip it */

      if ((i >= 0) && (done[i] == w_obj->R_num))
        continue;

      /* not in done[], so add to done[], then count copies after it.  */

      done[done_num++] = w_obj->R_num;
      t_obj2 = obj2;
      skip = TRUE;

      while (t_obj2)
      {
        if ((t_obj2->R_num == w_obj->R_num) && !t_obj2->contains &&
            ObjectsMatch(t_obj2, t_obj) && (t_obj2->type != ITEM_SPELLBOOK ||
                                            !find_spell_description(t_obj2)))
        {
          count++;
          if (count > 32000)
          {
            statuslog(AVATAR,
                      "Some feeb has > 32,000 %s, find him and kill him, he is too clueless to live.",
                      w_obj->short_description);
            return FALSE;
          }
        }
        else
          skip = FALSE;

        t_obj2 = t_obj2->next_content;

        if (skip)
          obj2 = t_obj2;
      }

      if (count > 1)
        o_f_flag |= O_F_COUNT;
    }

    /* okie, we have all data, let's write it out */

  	if (!IS_SET(w_obj->extra_flags, ITEM_NORENT))
      ibuf += writeObject(w_obj, o_f_flag, o_u_flag, (ush_int) count, loc, ibuf);

    if (obj_c)
      w_obj->weight += cont_wgt;
  }

  /*
   * end of a list, need the marker byte
   */

  if (!loc)
    ADD_BYTE(ibuf, O_F_EOL);

  if (t_obj)
    extract_obj(t_obj);

  return TRUE;
}

/*
 * actually add the data for an object(s) to the buffer
 */

int writeObject(P_obj obj, int o_f_flag, ulong o_u_flag, int count, int loc, char *dest_buff)
{
  char *start = dest_buff;
  char *ibuf = dest_buff;
  int      i;
  struct extra_descr_data *tmp;
  struct obj_affect *af;

  if( !obj )
    return FALSE;
  
  save_count += count;

  ADD_BYTE(ibuf, o_f_flag);
  ADD_INT(ibuf, obj_index[obj->R_num].virtual_number);
  ADD_SHORT(ibuf, obj->craftsmanship);
  ADD_SHORT(ibuf, obj->condition);

  if (o_f_flag & O_F_WORN)
    ADD_BYTE(ibuf, loc);

  if (o_f_flag & O_F_COUNT)
    ADD_SHORT(ibuf, (ush_int) count);

  if (o_f_flag & O_F_AFFECTS)
  {
    for (i = 0, af = obj->affects; af; af = af->next)
      i++;
    ADD_BYTE(ibuf, i);
    for (af = obj->affects; af; af = af->next)
    {
      ADD_INT(ibuf, obj_affect_time(obj, af));
      ADD_SHORT(ibuf, af->type);
      ADD_SHORT(ibuf, af->data);
      ADD_INT(ibuf, af->extra2);
    }
  }
  if (o_u_flag)
  {
    ADD_INT(ibuf, o_u_flag);

    if (o_u_flag & O_U_KEYS)
      ADD_STRING(ibuf, obj->name);

    if (o_u_flag & O_U_DESC1)
      ADD_STRING(ibuf, obj->description);

    if (o_u_flag & O_U_DESC2)
      ADD_STRING(ibuf, obj->short_description);

    if (o_u_flag & O_U_DESC3)
      ADD_STRING(ibuf, obj->action_description);

    if (o_u_flag & O_U_EDESC)
    {
      int nDescs = 0;
      struct extra_descr_data *ed = obj->ex_description;
      while (ed)
      {
        if (ed->keyword)
          nDescs++;
        ed = ed->next;
      }
      // count.  foreach: keyword + desc
      ADD_SHORT(ibuf, nDescs);
      ed = obj->ex_description;
      while (ed)
      {
        if (ed->keyword)
        {
          ADD_STRING(ibuf, ed->keyword);
          if (ed->description)
            ADD_STRING(ibuf, ed->description)
          else
            ADD_STRING(ibuf, "");
        }
        ed = ed->next;
      }
    }

    for (i = 0; i < 4; i++)
      if (o_u_flag & (1 << (i + 4)))
        ADD_INT(ibuf, obj->value[i]);

    if (o_u_flag & O_U_VAL4)
      ADD_INT(ibuf, obj->value[4]);

    if (o_u_flag & O_U_VAL5)
      ADD_INT(ibuf, obj->value[5]);

    if (o_u_flag & O_U_VAL6)
      ADD_INT(ibuf, obj->value[6]);

    if (o_u_flag & O_U_VAL7)
      ADD_INT(ibuf, obj->value[7]);

    if (o_u_flag & O_U_TIMER)
      for (i = 0; i < 4; i++)
        ADD_INT(ibuf, obj->timer[i]);

    if (o_u_flag & O_U_TRAP)
    {
      ADD_SHORT(ibuf, obj->trap_eff);
      ADD_SHORT(ibuf, obj->trap_dam);
      ADD_SHORT(ibuf, obj->trap_charge);
      ADD_SHORT(ibuf, obj->trap_level);
    }
    if (o_u_flag & O_U_TYPE)
      ADD_BYTE(ibuf, obj->type);

    if (o_u_flag & O_U_WEAR)
      ADD_INT(ibuf, obj->wear_flags);

    if (o_u_flag & O_U_EXTRA)
      ADD_INT(ibuf, obj->extra_flags);

    if (o_u_flag & O_U_ANTI)
      ADD_INT(ibuf, obj->anti_flags);

    if (o_u_flag & O_U_ANTI2)
      ADD_INT(ibuf, obj->anti2_flags);

    if (o_u_flag & O_U_EXTRA2)
      ADD_INT(ibuf, obj->extra2_flags);

    if (o_u_flag & O_U_WEIGHT)
      ADD_INT(ibuf, obj->weight);

    if (o_u_flag & O_U_MATERIAL)
      ADD_BYTE(ibuf, obj->material);

/*    if (o_u_flag & O_U_SPACE)
      ADD_BYTE(ibuf, obj->space);
*/
    if (o_u_flag & O_U_COST)
      ADD_INT(ibuf, obj->cost);

    if (o_u_flag & O_U_BV1)
      ADD_LONG(ibuf, obj->bitvector);

    if (o_u_flag & O_U_BV2)
      ADD_LONG(ibuf, obj->bitvector2);

    if (o_u_flag & O_U_BV3)
      ADD_LONG(ibuf, obj->bitvector3);

    if (o_u_flag & O_U_BV4)
      ADD_LONG(ibuf, obj->bitvector4);

    if (o_u_flag & O_U_BV5)
      ADD_LONG(ibuf, obj->bitvector5);

    if (o_u_flag & O_U_AFFS)
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
      {
        ADD_BYTE(ibuf, obj->affected[i].location);
        ADD_BYTE(ibuf, obj->affected[i].modifier);
      }
  }
  if (o_f_flag & O_F_SPELLBOOK)
    if (!(tmp = find_spell_description(obj)))
    {                           /*
                                 * this _SHOULD_
                                 * not happen..
                                 * but will it? we
                                 * shall see.
                                 */
      ADD_INT(ibuf, 0);
    }
    else
    {
      ADD_INT(ibuf, (MAX_SKILLS / 8) + 1);
      for (i = 0; i < (MAX_SKILLS / 8) + 1; i++)
      {
        ADD_BYTE(ibuf, tmp->description[i]);
      }
    }

	return (int) (ibuf-start);
}

// This function writes one object to a char buffer, and returns the total number of bytes written.
// This will *not* write the contents of a container object.
int write_one_object(P_obj obj, char* dest_buff)
{
	char *start = dest_buff;
  char *buff = dest_buff;

	byte o_f_flag = 0;
  ulong o_u_flag = 0;

	if( !obj )
  {
		logit(LOG_DEBUG, "write_one_object(): invalid object\n");
		return 0;
	}

	if( !buff )
  {
		logit(LOG_DEBUG, "write_one_object(): invalid buffer\n");
		return 0;
	}

	P_obj t_obj = read_object(obj->R_num, REAL);

	if( !t_obj )
  {
		logit(LOG_DEBUG, "write_one_object(): obj %d [%d] not loadable\n",
				obj->R_num, obj_index[obj->R_num].virtual_number);
    extract_obj(t_obj);
		return 0;
	}

  ADD_BYTE(buff, (char) SAV_ITEMVERS); // object version
	ADD_INT(buff, 1); // just one item

  if (obj->type == ITEM_SPELLBOOK && find_spell_description(obj))
    o_f_flag |= O_F_SPELLBOOK;

  if (obj->affects)
    o_f_flag |= O_F_AFFECTS;

  if ((o_u_flag = ObjUniqueFlags(obj, t_obj)))
    o_f_flag |= O_F_UNIQUE;

	int len = writeObject(obj, o_f_flag, o_u_flag, (ush_int) 1, 0, buff);

	buff += len;

  ADD_BYTE(buff, O_F_EOL); // end of items

  extract_obj(t_obj);
	return (buff - start);
}


/*
 * this is a combo of writeCharacter and writeItems, stores the corpse,
 * plus its contents in a seperate dir to allow it to be reloaded in the
 * event of a crash.
 */

void writeCorpse(P_obj corpse)
{
  FILE    *f;
  P_obj    hold_content = NULL;
  bool     bak, del_only = FALSE;       /*
                                         * return after unlinking existing
                                         */
  char    *buf, *size_off;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];
  int      i_count = 0;
  static char buff[SAV_MAXSIZE * 2];
  struct stat statbuf;
  int      room;

  if (!corpse || (corpse->type != ITEM_CORPSE) ||
      !IS_SET(corpse->value[1], PC_CORPSE)) {
		logit(LOG_DEBUG, "item wasn't a corpse in writeCorpse!");
		return;
  }
	    
  /*
   * unless corpse is on the ground, it doesn't get saved, and to
   * prevent duplication, any current file gets nuked. JAB
   */

  if (OBJ_CARRIED(corpse) && corpse->loc.carrying != NULL)
    room = corpse->loc.carrying->in_room;
  else if (!OBJ_ROOM(corpse))
    del_only = TRUE;
  else if ((corpse->loc.room <= NOWHERE) || (corpse->loc.room > top_of_world))
    return;
  else {
    room = corpse->loc.room;
    int virtual_room = world[room].number;
    if ( virtual_room >= RANDOM_VNUM_BEGIN && virtual_room < RANDOM_VNUM_END) {
      int i;
      for (i = 0; i < LOADED_RANDOM_ZONES; i++)
        if (virtual_room >= random_zone_data[i].first_room &&
            virtual_room <= random_zone_data[i].last_room)
          room = real_room0(random_zone_data[i].map_room);
    }
  }

  sprintf(Gbuf1, "%s/Corpses/", SAVE_DIR);

  if (corpse->action_description)
    strcpy(Gbuf2, corpse->action_description);

  for (buf = Gbuf2; *buf; buf++)
    *buf = LOWER(*buf);

  /*
   * to make certain we save ALL player corpses, even geeks that die
   * alot, we have to give them a serial number of sorts.  Since
   * value[6] was unused for player corpses, it makes a handy counter.
   * This will be slow if they have lots and lots of corpses (we have to
   * access disk for each one we check).  JAB
   */

  if (corpse->value[CORPSE_SAVEID] == 0)
  {
    corpse->value[CORPSE_SAVEID] = time(NULL);
  }
  sprintf(Gbuf3, "%s/%s%d", Gbuf1, Gbuf2, corpse->value[CORPSE_SAVEID]);
  strcpy(Gbuf1, Gbuf3);
  strcat(Gbuf1, ".bak");

  if (stat(Gbuf3, &statbuf) == 0)
  {
    if (rename(Gbuf3, Gbuf1) == -1)
    {
      logit(LOG_FILE, "Problem with player Corpses directory!\n");
      return;
    }
    bak = 1;
  }
  else
  {
    if (errno != ENOENT)
    {
      logit(LOG_FILE, "Problem with player Corpses directory!\n");
      return;
    }
    bak = 0;
  }

  if (del_only)
  {
    if (bak)
      if (unlink(Gbuf1) == -1)
        logit(LOG_FILE, "Couldn't delete backup of Corpse file.\n");
    return;
  }
  f = fopen(Gbuf3, "w");
  if (!f)
  {
    logit(LOG_FILE, "Couldn't create Corpse save file!\n");
    return;
  }
  buf = buff;
  ADD_BYTE(buf, (char) (short_size));
  ADD_BYTE(buf, (char) (int_size));
  ADD_BYTE(buf, (char) (long_size));

  if ((world[room].number >= (SHIPZONE * 100)) &&
      (world[room].number <= ((SHIPZONE * 100) + (MAXSHIPS * 10))))
  {
    if( anchor_room(world[room].number) )
    {
      ADD_INT(buf, anchor_room(world[room].number));
    }
    else
    {
      ADD_INT(buf, CORPSE_STORAGE); //store corpses in godroom if something goes wrong with buildings
    }
  }
//  else if( IS_BUILDING_ROOM(room) )
//  {
//    Building *building = get_building_from_room(room);
//    
//    if( building )
//    {
//      ADD_INT(buf, building->room_vnum);
//    }
//    else
//    {
//      ADD_INT(buf, CORPSE_STORAGE); //store corpses in godroom if something goes wrong with buildings
//    }
//    
//  }
  else if( IS_RANDOM_ROOM(room) )
  {
    if( random_entrance_vnum(room) )
    {
      ADD_INT(buf, random_entrance_vnum(room));
    }
    else
    {
      ADD_INT(buf, CORPSE_STORAGE); //store corpses in godroom if something goes wrong with buildings
    }    
    
  }
  else
  {
    ADD_INT(buf, world[room].number);   /*
                                         * reload room
                                         * (VIRTUAL)
                                         */
  }

  size_off = buf;               /*
                                 * needed to make sure it's not corrupt
                                 */
  ADD_INT(buf, (int) 0);

  /*
   * have to hold the 'next_content' of corpse, as this is stuff in the
   * room and not to be saved.  Replaced after it's saved.  JAB
   */

  hold_content = corpse->next_content;
  corpse->next_content = NULL;

  i_count = countInven(corpse);

  ADD_BYTE(buf, (char) SAV_ITEMVERS);
  ADD_INT(buf, i_count);

  ibuf = buf;
  save_count = 0;

  writeObjectlist(corpse, (byte) 0);

  corpse->next_content = hold_content;

  if (save_count != i_count)
  {
    logit(LOG_DEBUG, "save_count != count in writeCorpse!");
    return;
  }

  ADD_INT(size_off, (int) (ibuf - buff));

  if (fwrite(buff, 1, (unsigned) (ibuf - buff), f) != (ibuf - buff))
  {
    logit(LOG_FILE, "Couldn't write to Corpse save file!\n");
    fclose(f);
    return;
  }
  fclose(f);

  if (bak)
  {
    if (unlink(Gbuf1) == -1)
    {
      logit(LOG_FILE, "Couldn't delete backup of player file.\n");
    }
  }
}

int writeItems(char *buf, P_char ch)
{
  char    *start = buf;
  int      count, i;
  int      a, b;                /*
                                 * Added for easier debugging via GDB
                                 * -Torm
                                 */

  ibuf = buf;

  ADD_BYTE(ibuf, (char) SAV_ITEMVERS);
  a = countEquip(ch);           /*
                                 * including contents of worn containers
                                 */
  b = countInven(ch->carrying);
  count = a + b;
  save_count = 0;

  ADD_INT(ibuf, count);         /* total number of items being saved */

  /*
   * writeObjectlist() writes the entire list, with recursive calls to
   * handle containers in the list.  We call it for each piece of equip
   * as a list of 1, because it's neater that way.
   */

  for (i = 0; i < MAX_WEAR; i++)
    if (save_equip[i])
      if (!writeObjectlist(save_equip[i], (byte) (i + 1)))
        return 0;

  if (!writeObjectlist(ch->carrying, (byte) 0))
    return 0;

  if (!(save_count == count))
  {
    logit(LOG_DEBUG, "save counts don't match in writeItems!");
		return 0;    
  }
  return (int) (ibuf - start);
}

/* write witness record (TASFALEN) */

int writeWitness(char *buf, wtns_rec * rec)
{
  wtns_rec *first = rec;
  char    *start = buf;
  int      count = 0;

  while (rec)
  {
    count++;
    rec = rec->next;
  }
  ADD_BYTE(buf, (char) SAV_WTNSVERS);

  rec = first;
  ADD_INT(buf, count);

  while (rec)
  {
    ADD_STRING(buf, rec->attacker);
    ADD_STRING(buf, rec->victim);
    ADD_LONG(buf, rec->time);
    ADD_INT(buf, rec->crime);
    ADD_INT(buf, rec->room);

    rec = rec->next;
  }

  return (int) (buf - start);
}

void delete_knownShapes(P_char ch)
{
  struct char_shapechange_data *curShape = ch->only.pc->knownShapes;
  struct char_shapechange_data *pShape;

  while (NULL != (pShape = curShape))
  {
    curShape = pShape->next;
    FREE(pShape);
  }
  ch->only.pc->knownShapes = NULL;
}


void writeShapechangeData(P_char ch)
{
  struct char_shapechange_data *curShape;

  if (IS_PC(ch) && has_innate(ch, INNATE_SHAPECHANGE) && (NULL != ch->only.pc->knownShapes))
  {
    FILE    *f;
    char     buf[MAX_STRING_LENGTH];

    sprintf(buf, "Players/Shapechange/%c/%s", tolower(GET_NAME(ch)[0]),
            GET_NAME(ch));
    f = fopen(buf, "w");
    if (f == NULL)
    {
      sprintf(buf, "Unable to create/truncate '%s' in writeShapechangeData\n", buf);
      logit(LOG_DEBUG, buf);
      return;
    }

    curShape = ch->only.pc->knownShapes;

    while (curShape != NULL)
    {
      fprintf(f, "vnum = %d, timesR = %d, lastR = %d, lastC = %d\n",
              curShape->mobVnum,
              curShape->timesResearched,
              curShape->lastResearched,
              curShape->lastShapechanged);
      curShape = curShape->next;
    }

    fclose(f);
  }
}

void readShapechangeData(P_char ch)
{
  if (IS_PC(ch) && has_innate(ch, INNATE_SHAPECHANGE))
  {
    if (ch->only.pc->knownShapes)
    {
      delete_knownShapes(ch);
    }

    FILE    *f;
    char     s[MAX_STRING_LENGTH];
    struct char_shapechange_data *curShape;

    sprintf(s, "Players/Shapechange/%c/%s", tolower(GET_NAME(ch)[0]),
            GET_NAME(ch));
    f = fopen(s, "r");

    if (f == NULL)
      return;

    // read them in order written...  this might make the read code a bit
    // ulgier, but removes the need to reverse the linked list later.

    struct char_shapechange_data **ppShape = &(ch->only.pc->knownShapes);

    int      vNum, timesR, lastR, lastC;
    while (4 == fscanf(f, "vnum = %d, timesR = %d, lastR = %d, lastC = %d\n",
                       &vNum, &timesR, &lastR, &lastC))
    {
      // ensure that the vnum actually exists.  If not, skip it!
      if (!real_mobile(vNum))
        continue;

      // create a shapechange_data structure
      CREATE(curShape, char_shapechange_data, 1, MEM_TAG_SHPCHNG);
      curShape->mobVnum = vNum;
      curShape->timesResearched = timesR;
      curShape->lastResearched = lastR;
      curShape->lastShapechanged = lastC;
      curShape->next = NULL;

      // attach the structure to the end of ch's list
      (*ppShape) = curShape;
      // and move ppShape to the end of the list
      ppShape = &(curShape->next);
    }
    fclose(f);
  }
}

int calculate_save_room(P_char ch, int type, int room)
{

  /* type 7 save char in room, so if crash it will reload there */
  if ((type == RENT_CRASH2) && (room != NOWHERE))
    ch->specials.was_in_room = world[room].number;

  /*
   * ok, room given to this func, is real room number, to prevent mass
   * confusion, xlate to virtual number before writing it to file.  JAB
   *   the ugliest switch statement ever. Tharkun
   */

  if (room == ch->in_room)
  {
  }
  switch ((room == ch->in_room) ? 1 : (room ==
                                       real_room(ch->specials.
                                                 was_in_room)) ? 2 : 0)
  {
  case 0:
    /*
     * mystery room, make it NOWHERE
     */
    room = NOWHERE;
    break;

  case 1:
    if ((ch->in_room > 1) && (ch->in_room <= top_of_world))
      break;
    room = real_room(ch->specials.was_in_room);
  case 2:
    if ((room > 1) && (room <= top_of_world))
      break;
    else if ((ch->in_room > 1) && (ch->in_room <= top_of_world))
    {
      room = ch->in_room;
      break;
    }
    break;
  }

  if (room == NOWHERE)
    room = real_room(GET_BIRTHPLACE(ch) ? GET_BIRTHPLACE(ch) : GET_HOME(ch));
  /*
   * GET_HOME(ch) is now last rent spot
   */
  /*
   * special case, 4 is death, no items, and they come back in
   * birthplace
   */
  if (type == RENT_DEATH)
  {
    room = real_room(GET_BIRTHPLACE(ch));
    GET_HOME(ch) = GET_BIRTHPLACE(ch);
  }
  /*
   * ok, unless I REALLY screwed up, at this point we have a valid
   * 'real' room number to save in, or room == NOWHERE.  If it's real,
   * we have to convert it to virtual number to save it properly.  JAB
   */

  if (room != NOWHERE)
    room = world[room].number;

  return room;
}


int writeCharacter(P_char ch, int type, int room)
{
  FILE    *f;
  P_obj    obj, obj2;
  char    *buf, *skill_off, *affect_off, *item_off, *size_off, *witness_off, *tmp;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int      i, bak;
  struct affected_type *af;
  static char buff[SAV_MAXSIZE * 2];

  struct stat statbuf;

  if( !ch || !GET_NAME(ch) )
    return 0;

  if( IS_MORPH(ch) )
  {
    ch = MORPH_ORIG(ch);
    if( !ch || !GET_NAME(ch) )
    {
      return 0;
    }
  }

  if (IS_NPC(ch))
  {
/*    if (ch->following && IS_PC(ch->following))
   writePet(ch); */
    return 0;
  }

  /* hook needed for lockers - call a room proc when saving a character in the room */
  if( IS_SET(world[ch->in_room].room_flags, LOCKER) && (world[ch->in_room].funct) )
  {
    room = (*world[ch->in_room].funct) (ch->in_room, ch, (-80), NULL);
  }

  writeShapechangeData(ch);
  room = calculate_save_room(ch, type, room);

  /*
   * this check assumes that the macros have not been fixed up for ** a
   * different architecture type.
   */

  if ((sizeof(char) != 1) || (int_size != long_size))
  {
    logit(LOG_DEBUG,
          "sizeof(char) must be 1 and int_size must == long_size for player saves!\n");
    return 0;
  }
  /*
   * in case char reenters game immediately; handle rent/etc correctly
   */

  sql_update_money(ch);
  if( (type != RENT_POOFARTI) && (type != RENT_SWAPARTI) && (type != RENT_FIGHTARTI) )
  {
    sql_update_playtime(ch);
  }
	sql_update_epics(ch);

  save_zone_trophy(ch);
  
  if (ch->desc)
    ch->desc->rtype = type;

  buf = buff;
  ADD_BYTE(buf, (char) SAV_SAVEVERS);
  ADD_BYTE(buf, (char) (short_size));
  ADD_BYTE(buf, (char) (int_size));
  ADD_BYTE(buf, (char) (long_size));

  ADD_BYTE(buf, (char) type);

  skill_off = buf;
  ADD_INT(buf, (int) 0);
  witness_off = buf;
  ADD_INT(buf, (int) 0);
  affect_off = buf;
  ADD_INT(buf, (int) 0);
  item_off = buf;
  ADD_INT(buf, (int) 0);
  size_off = buf;
  ADD_INT(buf, (int) 0);
  // Surname
  ADD_INT( buf, (ch->specials.act3) );
  /*
   * starting room (VIRTUAL)
   */
  ADD_INT(buf, room);

  ADD_LONG(buf, time(0));       /*
                                 * save time
                                 */

  /*
   * unequip everything and remove affects before saving
   */

  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      save_equip[i] = unequip_char(ch, i);
    else
      save_equip[i] = NULL;

  all_affects(ch, FALSE);

  buf += writeStatus(buf, ch,
    ((type != RENT_POOFARTI) && (type != RENT_SWAPARTI) && (type != RENT_FIGHTARTI)) ? TRUE : FALSE);

  ADD_INT(skill_off, (int) (buf - buff));

  buf += writeSkills(buf, ch, MAX_SKILLS);

  ADD_INT(witness_off, (int) (buf - buff));

  buf += writeWitness(buf, ch->specials.witnessed);

  ADD_INT(affect_off, (int) (buf - buff));

  updateShortAffects(ch);
  buf += writeAffects(buf, ch->affected);

  ADD_INT(item_off, (int) (buf - buff));

  buf += writeItems(buf, ch);

  ADD_INT(size_off, (int) (buf - buff));

  /*
   * if they are staying in game, re-equip them
   */
  if( (type != RENT_INN) && (type != RENT_LINKDEAD) && (type != RENT_CAMPED) && (type != RENT_DEATH)
    && (type != RENT_POOFARTI) && (type != RENT_SWAPARTI) && (type != RENT_FIGHTARTI) )
  {
    for (i = 0; i < MAX_WEAR; i++)
      if (save_equip[i])
        equip_char(ch, save_equip[i], i, 9);
  }
  else
  {
    struct affected_type *af;

    if ((af = get_spell_from_char(ch, TAG_RACE_CHANGE)) != NULL)
    {
      ch->player.race = af->modifier;

      ch->player.time.birth = time(NULL) - (racial_data[GET_RACE(ch)].base_age) * 2;
      // Set birthdate + base_age + 5 years.
      ch->player.time.birth = time(NULL);
      // Add base_age to birthdate + base_age + 5 years.
      ch->player.time.birth -= (racial_data[GET_RACE(ch)].base_age) * SECS_PER_MUD_YEAR;

      affect_remove(ch, af);
    }
    /*
     * if not, nuke the equip and inven (it has already been saved)
     */

    for (i = 0; i < MAX_WEAR; i++)
      if (save_equip[i])
      {
        extract_obj(save_equip[i]);
        save_equip[i] = NULL;
      }
    for (obj = ch->carrying; obj; obj = obj2)
    {
      obj2 = obj->next_content;
      extract_obj(obj);
      obj = NULL;
    }
  }

  // Reapply affects (including equip)
  all_affects(ch, TRUE);

  if ((int) (buf - buff) > SAV_MAXSIZE)
  {
    logit(LOG_PLAYER, "Could not save %s, file too large (%d bytes)",
      GET_NAME(ch), (int) (buf - buff));
    return 0;
  }
  sprintf(Gbuf1, "%s/%c/", SAVE_DIR, LOWER(*ch->player.name));
  tmp = Gbuf1 + strlen(Gbuf1);
  strcat(Gbuf1, GET_NAME(ch));
  for (; *tmp; tmp++)
    *tmp = LOWER(*tmp);
  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");

  if (stat(Gbuf1, &statbuf) == 0)
  {
    if (rename(Gbuf1, Gbuf2) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Problem with player save files directory!\n");
      logit(LOG_FILE, "   rename failed, errno = %d\n", tmp_errno);
      wizlog(AVATAR, "&+R&-LPANIC!&N  Error backing up pfile for %s!",
             GET_NAME(ch));
      return 0;
    }
    bak = 1;
  }
  else
  {
    if (errno != ENOENT)
    {
      int      tmp_errno;

      tmp_errno = errno;

      /*
       * NOTE: with the stat() function, only two errors are
       * possible: EBADF   filedes is bad. ENOENT  File does not
       * exist. Now, EBADF should only occur using fstat().
       * Therefore, if I fall into here, I have some SERIOUS
       * problems!
       */

      logit(LOG_FILE, "Problem with player save files directory!\n");
      logit(LOG_FILE, "   stat failed, errno = %d\n", tmp_errno);
      wizlog(AVATAR, "&+R&-LPANIC!&N  Error finding pfile for %s!",
             GET_NAME(ch));
      return 0;
    }
    /*
     * in this case, no original pfile existed.  Probably a new
     * char... so don't panic.
     */
    bak = 0;
  }

  f = fopen(Gbuf1, "w");

  /*
   * NOTE:  From this point on, if the save is not successful, then the
   * pfile will be corrupted.  While its nice to return an error, THERE
   * IS STILL A FUCKED UP PLAYER FILE!  Making the backup is a complete
   * fucking waste if you don't DO anything with it!  It will just get
   * overwritten the next time the character tries to save! Therefore,
   * I'm adding code that will rename the backup to the original if the
   * save wasn't successful.  At the same time, I'd like to officially
   * request that whoever had the wonderful idea of making a backup, but
   * not using it, be sac'ed repeatedly. (neb)
   */

  if (!f)
  {
    int      tmp_errno;

    tmp_errno = errno;
    logit(LOG_FILE, "Couldn't create player save file!\n");
    logit(LOG_FILE, "   fopen failed, errno = %d\n", tmp_errno);
    wizlog(AVATAR, "&+R&-LPANIC!&N  Error creating pfile for %s!",
           GET_NAME(ch));
    bak -= 2;
  }
  else
  {
    if (fwrite(buff, 1, (unsigned) (buf - buff), f) != (buf - buff))
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Couldn't write to player save file!\n");
      logit(LOG_FILE, "   fwrite failed, errno = %d\n", tmp_errno);
      wizlog(AVATAR, "&+R&-LPANIC!&N  Error writing pfile for %s!",
             GET_NAME(ch));
      fclose(f);
      bak -= 2;
    }
    else
      fclose(f);
  }

  switch (bak)
  {
  case 1:                      /*
                                 * save worked, just get rid of the backup
                                 */
    if (unlink(Gbuf2) == -1)    /*
                                 * not a critical error
                                 */
      logit(LOG_FILE, "Couldn't delete backup of player file.\n");

  case 0:                      /*
                                 * save worked, no backup was made to
                                 * begin with
                                 */
    break;

  case -1:                     /*
                                 * save FAILED, but we have a backup
                                 */
    if (rename(Gbuf2, Gbuf1) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, " Unable to restore backup!  Argh!");
      logit(LOG_FILE, "    rename failed, errno = %d\n", tmp_errno);
      wizlog(AVATAR, "&+R&-LPANIC!&N  Error restoring backup pfile for %s!",
             GET_NAME(ch));
      logit(LOG_EXIT, "unable to restore backup pfile for %s", GET_NAME(ch));
			raise(SIGSEGV);
    }
    else
      wizlog(AVATAR, "        Backup restored.");
    /*
     * restored or not, the save still failed, so return 0
     */
    return 0;

  case -2:                     /*
                                 * save FAILED, and we have NO backup!
                                 */
    logit(LOG_FILE, " No restore file was made!");
    wizlog(AVATAR, "        No backup file available");
    return 0;
  }

  /* hook needed for lockers - call a room proc when saving a character in the room */
  /* -81 means that the save is complete */
  if (IS_SET(world[ch->in_room].room_flags, LOCKER) &&
      (world[ch->in_room].funct))
    (*world[ch->in_room].funct) (ch->in_room, ch, (-81), NULL);

  return 1;
}

#endif

int deleteCharacter(P_char ch, bool bDeleteLocker)
{
  char    *tmp;
  char     name[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  P_obj    obj;
  FILE    *f;

  strcpy(name, GET_NAME(ch));
  for( tmp = name; *tmp; tmp++ )
  {
    *tmp = LOWER(*tmp);
  }

  // Remove all artis from char.
  remove_all_artifacts_sql( ch );

#ifdef USE_ACCOUNT
  remove_char_from_list(ch->desc->account, ch->player.name);
#endif

  sprintf( Gbuf1, "%s/%c/%s", SAVE_DIR, *name, name );
  strcpy( Gbuf2, Gbuf1 );
  sprintf( Gbuf2, "mv -f %s %s.bak", Gbuf1, Gbuf1 );
  if( f = fopen( Gbuf1, "r" ) )
  {
    fclose( f );
    system( Gbuf2 );
  }

  if( bDeleteLocker )
  {
    // delete the locker as well
    sprintf( Gbuf1, "%s/%c/%s.locker", SAVE_DIR, LOWER(*ch->player.name), name );
    sprintf( Gbuf2, "mv -f %s %s.bak", Gbuf1, Gbuf1 );
    if( f = fopen( Gbuf1, "r" ) )
    {
      fclose( f );
      system( Gbuf2 );
    }
  }

  // Delete file containing conjurable mobs.
  sprintf( Gbuf1, "%s/%c/%s.spellbook", SAVE_DIR, LOWER(*ch->player.name), name);
  if( f = fopen( Gbuf1, "r" ) )
  {
    fclose( f );
    sprintf( Gbuf2, "mv -f %s %s.bak", Gbuf1, Gbuf1 );
    system( Gbuf2 );
  }
  // Delete file containing crafting/forging recipe list.
  sprintf(Gbuf1, "%s/Tradeskills/%c/%s.crafting", SAVE_DIR, LOWER(*ch->player.name), name);
  if( f = fopen( Gbuf1, "r" ) )
  {
    fclose( f );
    sprintf( Gbuf2, "mv -f %s %s.bak", Gbuf1, Gbuf1 );
    system( Gbuf2 );
  }

  return TRUE;
}

void PurgeCorpseFile(P_obj corpse)
{
  char    *tmp;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (!corpse || (corpse->type != ITEM_CORPSE) ||
      !IS_SET(corpse->value[1], PC_CORPSE)) {
		logit(LOG_DEBUG, "item not a corpse in PurgeCorpseFile");
    return;
	}

  sprintf(Gbuf2, "%s%d", corpse->action_description, corpse->value[CORPSE_SAVEID]);
  for (tmp = Gbuf2; *tmp; tmp++)
    *tmp = LOWER(*tmp);

  sprintf(Gbuf1, "%s/Corpses/%s", SAVE_DIR, Gbuf2);
  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");

  unlink(Gbuf1);
  unlink(Gbuf2);

  return;
}

/*
 * following are functions that read data from disk (or process data read
 * from disk).  JAB
 */

ush_int getShort(char **buf)
{
  ush_int  s;

  bcopy(*buf, &s, short_size);

  s = ntohs((ush_int) s);
  *buf += short_size;

  return s;
}

uint getInt(char **buf)
{
  uint     i;

  bcopy(*buf, &i, int_size);

  i = ntohl(i);
  *buf += int_size;

  return i;
}

unsigned long getLong(char **buf)
{
  ulong    l;

  bcopy(*buf, &l, long_size);

  l = ntohl(l);
  *buf += long_size;

  return l;
}
char    *getString(char **buf)
{
  int      len;
  char    *s;

  len = (int) GET_SHORT(*buf);
  if (len == 0)
    return 0;
  else
  {
    CREATE(s, char, (unsigned) (len + 1), MEM_TAG_STRING);

    strncpy(s, *buf, (unsigned) len);
    s[len] = 0;
    *buf += len;
  } return s;
}

int restoreStatus(char *buf, P_char ch)
{
  byte     dummy_byte;
  char    *start = buf, *str;
  long     dummy_long;
  int      tmp, tmp2, tmp3, dummy_int, i;
  unsigned short s;             /*, dummy_short; */
  struct trophy_data *tr, *tr2;
  char     buffer[2056];

  stat_vers = GET_BYTE(buf);

  if (stat_vers > (char) SAV_STATVERS)
  {
    logit(LOG_FILE, "Save file for %s status restore failed.", GET_NAME(ch));
    send_to_char
      ("Your character file is in a format which the game doesn't know how\r\n"
       "to load. Please log on with another character and talk to a God.\r\n",
       ch);
    return 0;
  }
  GET_NAME(ch) = GET_STRING(buf);

  ch->only.pc->pid = GET_INTE(buf);

  ch->only.pc->screen_length = (ubyte) GET_BYTE(buf);
  str = GET_STRING(buf);
#ifndef _PFILE_
  if (!str)
  {
    send_to_char
      ("How did you manage to nullify your password!??\r\nPlease contact a GOD!\r\n",
       ch);
    fprintf(stderr,
            "%s somehow managed to clear out his/her password field!\n",
            GET_NAME(ch));
    logit(LOG_FILE, "%s somehow managed to clear out his/her password field!",
          GET_NAME(ch));
    return 0;
  }
#endif
  strcpy(ch->only.pc->pwd, str);
  FREE(str);
  ch->player.short_descr = GET_STRING(buf);
  ch->player.long_descr = GET_STRING(buf);
  ch->player.description = GET_STRING(buf);
  GET_TITLE(ch) = GET_STRING(buf);
  if (stat_vers < 33)
  {
    ch->player.m_class = 1 << (GET_INTE(buf) - 1);
    GET_INTE(buf);
   
    
  }
  else
    ch->player.m_class = GET_INTE(buf);
  if(stat_vers > 36)  
  ch->player.secondary_class = GET_INTE(buf);
  if(stat_vers > 38)  
  ch->player.spec = GET_BYTE(buf);

   if (IS_MULTICLASS_PC(ch))
   {
     ch->player.spec  = 0;
   }

  
  GET_RACE(ch) = GET_BYTE(buf);
  GET_RACEWAR(ch) = GET_BYTE(buf);
  ch->player.level = GET_BYTE(buf);
  if (ch->player.level > MAXLVL)
    ch->player.level = 1;

  if (stat_vers < 33)
  {
    GET_BYTE(buf);
  }

  GET_SEX(ch) = GET_BYTE(buf);
  ch->player.weight = GET_SHORT(buf);
  ch->player.height = GET_SHORT(buf);
  GET_SIZE(ch) = GET_BYTE(buf);

  GET_HOME(ch) = GET_INTE(buf);
  GET_BIRTHPLACE(ch) = 0;
  GET_BIRTHPLACE(ch) = GET_INTE(buf);
  GET_ORIG_BIRTHPLACE(ch) = GET_INTE(buf);

  ch->player.time.birth = GET_LONG(buf);
  ch->player.time.played = GET_INTE(buf);
  ch->player.time.saved = GET_LONG(buf);        /* last save time */
  ch->player.time.logon = time(0);      /* set it */
  ch->player.time.perm_aging = GET_SHORT(buf);
  for (i = 0; i < MAX_CIRCLE + 1; i++)
    ch->specials.undead_spell_slots[i] = GET_BYTE(buf);
  GET_INTE(buf);                //!!! last_level

  for (i = 0; i < NUMB_PC_TIMERS; i++)
    ch->only.pc->pc_timer[i] = GET_INTE(buf);

  /* trophy stuff */
//  ch->only.pc->trophy = NULL;
//  if (!dead_trophy_pool)
//    dead_trophy_pool = mm_create("TROPHY",
//                                 sizeof(struct trophy_data),
//                                 offsetof(struct trophy_data, next), 3);
  ZONE_TROPHY(ch) = NULL;
  load_zone_trophy(ch);

  if( stat_vers < 45 )
  {
    // old trophy data
    tmp = GET_BYTE(buf);
    for (tmp2 = 0; tmp2 < tmp; tmp2++)
    {
      GET_INTE(buf);
      GET_INTE(buf);
      //    tr = (struct trophy_data *) mm_get(dead_trophy_pool);
      //    tr->vnum = GET_INTE(buf);
      //    tr->kills = GET_INTE(buf);
      //    tr->next = NULL;
      //    /* add it to the END */
      //    if (!ch->only.pc->trophy)
      //    {
      //      ch->only.pc->trophy = tr;
      //    }
      //    else
      //    {
      //      tr2 = ch->only.pc->trophy;
      //      while (tr2->next)
      //        tr2 = tr2->next;
      //      tr2->next = tr;
      //    
    }      
  }

  s = GET_SHORT(buf);           /* number of tongues */
  for (tmp = 0; tmp < MAX(s, MAX_TONGUE); tmp++)
    if ((tmp < s) && (tmp < MAX_TONGUE))
    {
      /* valid language number was stored */
      GET_LANGUAGE(ch, tmp) = GET_BYTE(buf);
    }
    else
    {
      if (tmp < s)
      {
        /*
         * MAX_TONGUE is smaller than saved version, so read and
         * discard the extra bytes
         */
        dummy_byte = GET_BYTE(buf);
      }
      else
      {
        /* number of languages has grown, make sure new ones are 0% */
        GET_LANGUAGE(ch, tmp) = 0;
      }
    }
  /* intro stuff */
  s = GET_SHORT(buf);
  for (tmp = 0; tmp < MAX(s, MAX_INTRO); tmp++)
  {
    if ((tmp < s) && (tmp < MAX_INTRO))
    {
      ch->only.pc->introd_list[tmp] = GET_INTE(buf);
      ch->only.pc->introd_times[tmp] = GET_LONG(buf);
    }
    else
    {
      if (tmp < s)
      {
        dummy_int = GET_INTE(buf);
        dummy_long = GET_LONG(buf);
      }
      else
      {
        ch->only.pc->introd_list[tmp] = 0;
        ch->only.pc->introd_times[tmp] = 0;
      }
    }
  }

   if(stat_vers > 37 && stat_vers < 40)
  for (tmp = 0; tmp < 100; tmp++)
  {
   ch->only.pc->learned_forged_list[tmp] = GET_INTE(buf);

  }
  if(stat_vers > 39)
     for (tmp = 0; tmp < MAX_FORGE_ITEMS; tmp++)
        {
            ch->only.pc->learned_forged_list[tmp] = GET_INTE(buf);

             }
  


  
 
  ch->base_stats.Str = (ubyte) GET_BYTE(buf);
  ch->base_stats.Dex = (ubyte) GET_BYTE(buf);
  ch->base_stats.Agi = (ubyte) GET_BYTE(buf);
  ch->base_stats.Con = (ubyte) GET_BYTE(buf);
  ch->base_stats.Pow = (ubyte) GET_BYTE(buf);
  ch->base_stats.Int = (ubyte) GET_BYTE(buf);
  ch->base_stats.Wis = (ubyte) GET_BYTE(buf);
  ch->base_stats.Cha = (ubyte) GET_BYTE(buf);
  ch->base_stats.Kar = (ubyte) GET_BYTE(buf);
  ch->base_stats.Luk = (ubyte) GET_BYTE(buf);
  ch->curr_stats = ch->base_stats;

  GET_MANA(ch) = GET_SHORT(buf);
  ch->points.base_mana = GET_SHORT(buf);
  GET_HIT(ch) = GET_SHORT(buf);
  if (GET_HIT(ch) < 0)
    GET_HIT(ch) = 0;
  ch->only.pc->spells_memmed[MAX_CIRCLE] = GET_BYTE(buf);
  ch->points.base_hit = GET_SHORT(buf);
  if (ch->points.base_hit < 1)
    ch->points.base_hit = 1;
  GET_VITALITY(ch) = GET_SHORT(buf);
  ch->points.base_vitality = GET_SHORT(buf);
  ch->points.hit_reg = 0;
  ch->points.move_reg = 0;
  ch->points.mana_reg = 0;

  GET_COPPER(ch) = GET_INTE(buf);
  GET_SILVER(ch) = GET_INTE(buf);
  GET_GOLD(ch) = GET_INTE(buf);
  GET_PLATINUM(ch) = GET_INTE(buf);

  GET_EXP(ch) = GET_INTE(buf);
//  ch->points.max_exp =
  GET_INTE(buf);
  ch->only.pc->epics = GET_INTE(buf);    // Used for lvl withouth potion
  
  if(stat_vers >= 44)
  {
    ch->only.pc->epic_skill_points = GET_INTE(buf);
  }
  else
  {
    ch->only.pc->epic_skill_points = 0;
  }

  if(stat_vers > 46)
    ch->only.pc->skillpoints = GET_INTE(buf);

  if(stat_vers > 40)
    ch->only.pc->spell_bind_used = GET_INTE(buf);
    
  // quaffed_level
  if(stat_vers < 43)
    GET_INTE(buf);

  SET_POS(ch, POS_STANDING + STAT_NORMAL);
  ch->specials.act = GET_INTE(buf);
  ch->specials.act2 = GET_INTE(buf);
  REMOVE_BIT(ch->specials.act2, PLR2_WAIT);
  if (stat_vers < 35)
  {
    GET_INTE(buf);
    GET_INTE(buf);
  }
  ch->only.pc->vote = GET_INTE(buf);
  ch->specials.alignment = GET_INTE(buf);

  GET_INTE(buf);  // orig_align field, not used anymore

  ch->only.pc->prestige = GET_SHORT(buf);
  ch->specials.guild = GET_SHORT(buf);
  ch->specials.guild_status = GET_INTE(buf);
  GET_TIME_LEFT_GUILD(ch) = GET_LONG(buf);
  GET_NB_LEFT_GUILD(ch) = GET_BYTE(buf);
  if (stat_vers > 31)
    ch->only.pc->time_unspecced = GET_LONG(buf);
  else
    ch->only.pc->time_unspecced = 0;

  if (stat_vers <= 35)
    for (tmp = 0; tmp < 5; tmp++)
    {
      GET_BYTE(buf);
      GET_BYTE(buf);
    }

  if (stat_vers < 46)
  {
    ch->only.pc->frags = 0; GET_LONG(buf);
    ch->only.pc->oldfrags = 0; GET_LONG(buf);
  }
  else
  {
    ch->only.pc->frags = GET_LONG(buf);
    ch->only.pc->oldfrags = GET_LONG(buf);
  }

  if (stat_vers < 35)
  {
    GET_SHORT(buf);
    GET_SHORT(buf);
  }
  if (stat_vers < 34)
    GET_INTE(buf);
  if (stat_vers < 35)
    GET_INTE(buf);

  ch->only.pc->numb_gcmd = GET_INTE(buf);
  if (ch->only.pc->numb_gcmd)
  {
    CREATE(ch->only.pc->gcmd_arr, int, ch->only.pc->numb_gcmd, MEM_TAG_ARRAY);

    for (tmp = 0; tmp < ch->only.pc->numb_gcmd; tmp++)
      ch->only.pc->gcmd_arr[tmp] = GET_INTE(buf);
  }

  for (tmp = 0; tmp < MAX_COND; tmp++)
  {
    ch->specials.conditions[tmp] = GET_BYTE(buf);
    if ((ch->specials.conditions[tmp] < 0) && !IS_TRUSTED(ch))
      ch->specials.conditions[tmp] = 0;
  }

// -Foo Remove hunger/thirst
#if 1
  GET_COND(ch, FULL) = -1;
  GET_COND(ch, THIRST) = -1;
#endif
  if (stat_vers < 35)
    for (tmp = 0; tmp < MAX_PETS; tmp++)
      GET_INTE(buf);
  ch->only.pc->poofIn = GET_STRING(buf);
  ch->only.pc->poofOut = GET_STRING(buf);
  if (stat_vers > 10)
  {
    ch->only.pc->poofInSound = GET_STRING(buf);
    ch->only.pc->poofOutSound = GET_STRING(buf);
  }
  ch->only.pc->echo_toggle = GET_BYTE(buf);
  ch->only.pc->prompt = GET_SHORT(buf);
  ch->only.pc->wiz_invis = GET_LONG(buf);
  ch->only.pc->law_flags = (ulong) GET_LONG(buf);
  ch->only.pc->wimpy = GET_SHORT(buf);
  ch->only.pc->aggressive = GET_SHORT(buf);

  ch->only.pc->highest_level = GET_BYTE(buf);

  GET_BALANCE_COPPER(ch) = GET_INTE(buf);
  GET_BALANCE_SILVER(ch) = GET_INTE(buf);
  GET_BALANCE_GOLD(ch) = GET_INTE(buf);
  GET_BALANCE_PLATINUM(ch) = GET_INTE(buf);

  ch->only.pc->numb_deaths = GET_LONG(buf);

  ch->specials.carry_weight = 0;
  ch->specials.carry_items = 0;

  ch->points.max_hit = 0; //ch->points.base_hit + calculate_hitpoints(ch);
  ch->points.max_mana = ch->points.base_mana + calculate_mana(ch);
  ch->points.max_vitality = vitality_limit(ch);


  if (stat_vers > 41)
  {

  ch->only.pc->quest_active = GET_INTE(buf);
  ch->only.pc->quest_mob_vnum = GET_INTE(buf);
  ch->only.pc->quest_type = GET_INTE(buf);
  ch->only.pc->quest_accomplished = GET_INTE(buf);
  ch->only.pc->quest_started =GET_INTE(buf);
  ch->only.pc->quest_zone_number = GET_INTE(buf);
  ch->only.pc->quest_giver = GET_INTE(buf);
  ch->only.pc->quest_level = GET_INTE(buf);
  ch->only.pc->quest_receiver = GET_INTE(buf);
  ch->only.pc->quest_shares_left = GET_INTE(buf);
  ch->only.pc->quest_kill_how_many = GET_INTE(buf);
  ch->only.pc->quest_kill_original = GET_INTE(buf);
  ch->only.pc->quest_map_room = GET_INTE(buf);
  ch->only.pc->quest_map_bought = GET_INTE(buf);


  }
   
  return (int) (buf - start);
}

#ifndef _PFILE_
int restoreAffects(char *buf, P_char ch)
{
  struct affected_type af;
  char    *start = buf;
  short    count;
  long     short_duration;
  byte     custom_messages = 0;
  char    *wear_off_char = NULL;
  char    *wear_off_room = NULL;

  if ((aff_vers = GET_BYTE(buf)) > (char) SAV_AFFVERS)
  {
    logit(LOG_FILE, "Save file for %s affects restore failed.", GET_NAME(ch));
    send_to_char
      ("Your character file is in a format which the game doesn't know how\r\n"
       "to load. Please log on with another character and talk to a God.\r\n",
       ch);
    return 0;
  }
  for( count = GET_SHORT(buf); count > 0; count-- )
  {
    if (aff_vers > 4)
    {
      if (aff_vers > 5)
      {
        custom_messages = GET_BYTE(buf);
        if (custom_messages & 1)
          wear_off_char = GET_STRING(buf);
        if (custom_messages & 2)
          wear_off_room = GET_STRING(buf);
        af.type = GET_SHORT(buf);
      }
      else
      {
        af.type = GET_INTE(buf);
      }
      af.duration = GET_INTE(buf);
      af.flags = GET_SHORT(buf);
      af.modifier = GET_INTE(buf);
      af.location = GET_BYTE(buf);
      af.bitvector = GET_LONG(buf);
      af.bitvector2 = GET_LONG(buf);
      af.bitvector3 = GET_LONG(buf);
      af.bitvector4 = GET_LONG(buf);
      af.bitvector5 = GET_LONG(buf);
      /*af.bitvector6 = */ GET_LONG(buf);
      af.wear_off_message_index = 0;

      // Duration saved as seconds for short affects, but we want to store duration as pulses in game.
      if( aff_vers > 6 && IS_SET(af.flags, AFFTYPE_SHORT) )
      {
        af.duration *= WAIT_SEC;
      }
    }
    else
    {
      af.type = GET_INTE(buf);
      af.duration = GET_SHORT(buf);
      af.modifier = GET_INTE(buf);
      af.location = GET_BYTE(buf);
      GET_INTE(buf);            // loc2
      af.bitvector = GET_LONG(buf);
      af.bitvector2 = GET_LONG(buf);
      af.bitvector3 = GET_LONG(buf);
      af.bitvector4 = GET_LONG(buf);
      af.bitvector5 = GET_LONG(buf);
      af.wear_off_message_index = 0;
      af.flags = 0;
      if (aff_vers == 4)
      {
        /*af.bitvector6 = */ GET_LONG(buf);
        short_duration = GET_LONG(buf);
        if (short_duration > 0)
        {
          af.flags = AFFTYPE_SHORT;
          af.duration = short_duration;
        }
      }
    }
    if (custom_messages == 0)
      affect_to_char(ch, &af);
    else
    {
      affect_to_char_with_messages(ch, &af, wear_off_char, wear_off_room);
      if (wear_off_char)
        str_free(wear_off_char);
      if (wear_off_room)
        str_free(wear_off_room);
    }
    if (IS_POISON(af.type)) {
      add_event(event_poison, PULSE_VIOLENCE * number(1,5), ch, 0, 0, 0, &af.type, sizeof(af.type));
    }
  }
  affect_total(ch, FALSE);

  /*
   * ok, some innate powers just set bits, so we need to reset those
   */
  if (has_innate(ch, INNATE_SNEAK))
    SET_BIT(ch->specials.affected_by, AFF_SNEAK);
  if (has_innate(ch, INNATE_FARSEE))
    SET_BIT(ch->specials.affected_by, AFF_FARSEE);
  if (has_innate(ch, INNATE_PROT_LIGHTNING))
    SET_BIT(ch->specials.affected_by2, AFF2_PROT_LIGHTNING);
  if (has_innate(ch, INNATE_PROT_FIRE))
    SET_BIT(ch->specials.affected_by, AFF_PROT_FIRE);
  if (has_innate(ch, INNATE_WATERBREATH))
    SET_BIT(ch->specials.affected_by, AFF_WATERBREATH);
  if (has_innate(ch, INNATE_INFRAVISION))
    SET_BIT(ch->specials.affected_by, AFF_INFRAVISION);
  if (has_innate(ch, INNATE_FLY))
    SET_BIT(ch->specials.affected_by, AFF_FLY);
  if (has_innate(ch, INNATE_HASTE))
    SET_BIT(ch->specials.affected_by, AFF_HASTE);
  if (has_innate(ch, INNATE_ULTRAVISION))
    SET_BIT(ch->specials.affected_by2, AFF2_ULTRAVISION);
  if (has_innate(ch, INNATE_ANTI_GOOD))
  {
    SET_BIT(ch->specials.affected_by, AFF_PROTECT_GOOD);
    SET_BIT(ch->specials.affected_by2, AFF2_DETECT_GOOD);
  }
  if (has_innate(ch, INNATE_ANTI_EVIL))
  {
    SET_BIT(ch->specials.affected_by, AFF_PROTECT_EVIL);
    SET_BIT(ch->specials.affected_by2, AFF2_DETECT_EVIL);
    /* paladin innate soulshield at 51st */
#   if 0
    if ((GET_CLASS(ch) == CLASS_PALADIN) && (GET_ALIGNMENT(ch) > 950) &&
        (GET_LEVEL(ch) > 50))
      SET_BIT(ch->specials.affected_by2, AFF2_SOULSHIELD);
#   endif
  }
  if (has_innate(ch, INNATE_VAMPIRIC_TOUCH))
    SET_BIT(ch->specials.affected_by2, AFF2_VAMPIRIC_TOUCH);
  if (has_innate(ch, INNATE_DAUNTLESS))
    SET_BIT(ch->specials.affected_by4, AFF4_NOFEAR);
  if (has_innate(ch, INNATE_BLUR))
    SET_BIT(ch->specials.affected_by3, AFF3_BLUR);
  
  return (int) (buf - start);
}
#endif

int restoreSkills(char *buf, P_char ch, int maxnum)
{
  char    *start = buf;
  int      i, n;

  skill_vers = GET_BYTE(buf);
  if (skill_vers > (char) SAV_SKILLVERS)
  {
    logit(LOG_FILE, "Save file for %s skills restore failed.", GET_NAME(ch));
    send_to_char
      ("Your character file is in a format which the game doesn't know how\r\n"
       "to load. Please log on with another character and talk to a God.\r\n",
       ch);
    return 0;
  }
  n = (int) GET_INTE(buf);
  if (n > maxnum)
  {
    logit(LOG_FILE, "Not all %s skills could be loaded.", GET_NAME(ch));
    send_to_char
      ("Not all your skills could be loaded. Please report this to a God.\r\n",
       ch);
  }
  /*
   * Allow memorized spells and skill usages to be saved. -DCL
   */

  for (i = 0; i < maxnum; i++)
  {
    if (i < n)
    {
      ch->only.pc->skills[i].learned = GET_BYTE(buf);
      ch->only.pc->skills[i].taught = GET_BYTE(buf);
      GET_BYTE(buf);
    }
    else
    {
      ch->only.pc->skills[i].learned = 0;
      ch->only.pc->skills[i].taught = 0;
    }
  }

  do
  {
    n = GET_INTE(buf);
  }
  while (n != 0);

  n = (int) GET_SHORT(buf);
  if (n > MAX_SKILL_USAGE)
  {
    logit(LOG_FILE, "Not all %s skill usages could be loaded.", GET_NAME(ch));
    send_to_char
      ("Not all your skill usages could be loaded. Please report a God.\r\n",
       ch);
  }
  for (i = 0; i < MAX_SKILL_USAGE; i++)
  {
    if (i < n)
    {
      GET_LONG(buf);
      GET_BYTE(buf);
    }
  }
  return (int) (buf - start);
}


/* Restore witness record  */
#ifndef _PFILE_
int restoreWitness(char *buf, P_char ch)
{
  wtns_rec *rec;
  char    *start = buf;
  int      count;

  if ((witness_vers = GET_BYTE(buf)) > (char) SAV_WTNSVERS)
  {
    logit(LOG_FILE, "Save file for %s witness restore failed.", GET_NAME(ch));
    send_to_char
      ("Your witness record is munged. Please log on with another character and talk to a God.\r\n",
       ch);
    return 0;
  }
  count = GET_INTE(buf);
  for (; count > 0; count--)
  {
    if (!dead_witness_pool)
      dead_witness_pool =
        (struct mm_ds *) mm_create("WITNESS", sizeof(wtns_rec),
                                   offsetof(wtns_rec, next), 1);

    rec = (wtns_rec *) mm_get(dead_witness_pool);
    rec->attacker = GET_STRING(buf);
    rec->victim = GET_STRING(buf);
    rec->time = GET_INTE(buf);
    rec->crime = GET_INTE(buf);
    rec->room = GET_INTE(buf);

    /* Ok we remove all the record that are 1 month old and more TASFALEN3 */
    if (((rec->time + SECS_PER_MUD_MONTH) < time(NULL)) ||
        (rec->crime <= CRIME_LAST_FAKE))
    {
      str_free(rec->attacker);
      str_free(rec->victim);
      mm_release(dead_witness_pool, rec);

    }
    else
    {
      rec->next = ch->specials.witnessed;
      ch->specials.witnessed = rec;
    }
  }

  return (int) (buf - start);
}


/*
 * -1 = char doesn't exist -2 = problem reading file other = save type
 */

int restorePasswdOnly(P_char ch, char *name)
{
  FILE    *f;

  struct stat statbuf;
  char     buff[SAV_MAXSIZE], *str;
  char    *buf = buff;
  int      size, csize, type, room;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     b_savevers;          /* TASFALEN */
  char     buffer[2056];

  if (!name || !ch)
    return 0;

  strcpy(buff, name);
  for (; *buf; buf++)
    *buf = LOWER(*buf);
  buf = buff;
  sprintf(Gbuf1, "%s/%c/%s", SAVE_DIR, *buff, buff);
  if (stat(Gbuf1, &statbuf) != 0)
  {
    sprintf(Gbuf1, "%s/%c/%s", SAVE_DIR, *buff, name);
    if (stat(Gbuf1, &statbuf) != 0)
      return -1;
  }
  f = fopen(Gbuf1, "r");
  if (!f)
    return -2;
  size = fread(buf, 1, SAV_MAXSIZE, f);
  fclose(f);
  if (size < 4)
  {
    logit(LOG_FILE, "Warning: Save file less than 4 bytes.");
    fprintf(stderr, "Problem restoring save file of: %s\n", name);
    logit(LOG_FILE, "Problem restoring save file of %s.", name);
    send_to_char
      ("There is something wrong with your save file!  Please talk to a God.\r\n",
       ch);
    return -2;
  }
  b_savevers = GET_BYTE(buf);

/*  if (GET_BYTE(buf) != (char) SAV_SAVEVERS) {
   logit(LOG_FILE, "Save file of %s is in an older format.", name);
   send_to_char("Your character file is in an old format which the game doesn't know how\r\n"
   "to load.  Please log on with another character and talk to a God.\r\n", ch);
   return -2;
   }
 */
  if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
      (GET_BYTE(buf) != long_size))
  {
    logit(LOG_FILE, "Save file of %s is in an old format.", name);
    send_to_char
      ("Your character file was created on a machine of a different architecture\r\n"
       "type than the current one; loading such a file is not yet supported.\r\n"
       "Please talk to a God.\r\n", ch);
    return -2;
  }
  if (size < 5 * int_size + 5 * sizeof(char) + long_size)
  {
    logit(LOG_FILE, "Warning: Save file is only %d bytes.", size);
    fprintf(stderr, "Problem restoring save file of: %s\n", name);
    logit(LOG_FILE, "Problem restoring save file of %s.", name);
    send_to_char
      ("There is something wrong with your save file!  Please talk to a God.\r\n",
       ch);
    return -2;
  }
  type = (int) GET_BYTE(buf);
  GET_INTE(buf);                /*
                                 * skill offset
                                 */
  if (b_savevers >= (char) SAV_WTNSVERS)        /* no witness record save in file (TASFALEN) */
    GET_INTE(buf);
  /*
   * witness offset
   */

  GET_INTE(buf);                /*
                                 * affect offset
                                 */
  GET_INTE(buf);                /*
                                 * item offset
                                 */
  csize = GET_INTE(buf);
  if (size != csize)
  {
    logit(LOG_FILE, "Warning: file size %d doesn't match csize %d.", size,
          csize);
    fprintf(stderr, "Problem restoring save file of: %s\n", name);
    logit(LOG_FILE, "Problem restoring save file of %s.", name);
    send_to_char
      ("There is something wrong with your save file!  Please talk to a God.\r\n",
       ch);
    return -2;
  }
  // Surname
  if( b_savevers > 4 )
    GET_INTE(buf);
  room = GET_INTE(buf);         /*
                                 * virtual room they saved/rented in
                                 */

  GET_LONG(buf);
  stat_vers = GET_BYTE(buf);
  if (stat_vers > (char) SAV_STATVERS)
  {
    logit(LOG_FILE, "Save file for %s status restore failed.", GET_NAME(ch));
    send_to_char
      ("Your character file is in a format which the game doesn't know how\r\n"
       "to load. Please log on with another character and talk to a God.\r\n",
       ch);
    return -2;
  }
  GET_NAME(ch) = GET_STRING(buf);
  if (stat_vers > 16)
    GET_INTE(buf);              /* PC id numb */
  ch->only.pc->screen_length = (ubyte) GET_BYTE(buf);
  str = GET_STRING(buf);
  if (!str)
  {
    send_to_char
      ("How did you manage to nullify your password!??\r\nPlease contact a God!\r\n",
       ch);
    fprintf(stderr,
            "%s somehow managed to clear out his/her password field!\n",
            GET_NAME(ch));
    logit(LOG_FILE, "%s somehow managed to clear out his/her password field!",
          GET_NAME(ch));
    return -2;
  }
  strcpy(ch->only.pc->pwd, str);
  FREE(str);
  return type;
}

/*
 * -1 = char doesn't exist -2 = problem reading file other = save type
 */
#endif
int restoreCharOnly(P_char ch, char *name)
{
  FILE    *f;

  struct stat statbuf;

#ifndef _PFILE_
  char     buff[SAV_MAXSIZE];
  char    *buf;
  int      skill_off, affect_off, item_off, surname;
#endif
  int      start, size, csize, type, room;
  int      witness_off;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     b_savevers;

  if( !name || !ch )
  {
    return -1;
  }

  strcpy(buff, name);
  for( buf = buff; *buf; buf++ )
  {
    *buf = LOWER(*buf);
  }
  buf = buff;
  sprintf(Gbuf1, "%s/%c/%s", SAVE_DIR, *buff, buff);
//  logit(LOG_FILE, "%s is the pfile string!", Gbuf1);
#ifndef _PFILE_
  if (stat(Gbuf1, &statbuf) != 0)
  {
    sprintf(Gbuf1, "%s/%c/%s", SAVE_DIR, *buff, name);
    if (stat(Gbuf1, &statbuf) != 0)
      return -1;
  }
#endif
  f = fopen(Gbuf1, "r");
  if (!f)
    return -2;
  size = fread(buf, 1, SAV_MAXSIZE, f);
  fclose(f);
  if (size < 4)
  {
    logit(LOG_FILE, "Warning: Save file less than 4 bytes.");
    fprintf(stderr, "Problem restoring save file of: %s\n", name);
    logit(LOG_FILE, "Problem restoring save file of %s.", name);
    send_to_char
      ("There is something wrong with your save file!  Please talk to a God.\r\n",
       ch);
    return -2;
  }
/* TASFALEN */

  b_savevers = GET_BYTE(buf);

/*  if (GET_BYTE(buf) != (char) SAV_SAVEVERS) {
   logit(LOG_FILE, "Save file of %s is in an older format.", name);
   send_to_char(
   "Your character file is in an old format which the game doesn't know how\r\n"
   "to load.  Please log on with another character and talk to a God.\r\n",
   ch);
   return -2;
   }
 */

/* end TASFALEN */

  if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
      (GET_BYTE(buf) != long_size))
  {
    logit(LOG_FILE, "Save file of %s is in an old format.", name);
    send_to_char
      ("Your character file was created on a machine of a different architecture\r\n"
       "type than the current one; loading such a file is not yet supported.\r\n"
       "Please talk to a God.\r\n", ch);
    return -2;
  }
  if (size < 5 * int_size + 5 * sizeof(char) + long_size)
  {
    logit(LOG_FILE, "Warning: Save file is only %d bytes.", size);
    fprintf(stderr, "Problem restoring save file of: %s\n", name);
    logit(LOG_FILE, "Problem restoring save file of %s.", name);
    send_to_char
      ("There is something wrong with your save file!  Please talk to a God.\r\n",
       ch);
    return -2;
  }
  type = (int) GET_BYTE(buf);
  skill_off = GET_INTE(buf);

  if (b_savevers >= (char) SAV_WTNSVERS)        /* no witness record save in file */
    witness_off = GET_INTE(buf);        /* TASFALEN */

  affect_off = GET_INTE(buf);
  item_off = GET_INTE(buf);
  csize = GET_INTE(buf);
  if (size != csize)
  {
    logit(LOG_FILE, "Warning: file size %d doesn't match csize %d.",
          size, csize);
    fprintf(stderr, "Problem restoring save file of: %s\n", name);
    logit(LOG_FILE, "Problem restoring save file of %s.", name);
    send_to_char
      ("There is something wrong with your save file!  Please talk to a God.\r\n",
       ch);
    return -2;
  }
  if( b_savevers > 4 )
    surname = GET_INTE(buf);
  ch->specials.act3 = surname;

  room = GET_INTE(buf);         /*
                                 * virtual room they saved/rented in
                                 */
#ifndef _PFILE_
  ch->in_room = real_room(room);
#else
  ch->in_room = room;
#endif

  GET_LONG(buf);
  start = (int) (buf - buff);
  if ((restoreStatus(buf, ch) + start) != skill_off)
  {
    logit(LOG_FILE, "Warning: restoreStatus() not match offset.");
    fprintf(stderr, "Problem restoring save file of: %s\n", name);
    logit(LOG_FILE, "Problem restoring save file of %s.", name);
    send_to_char
      ("There is something wrong with your save file!  Please talk to a God.\r\n",
       ch);
    return -2;
  }
  if (type == 4)                /*
                                 * return from death, flaked out. JAB
                                 */
    SET_POS(ch, POS_PRONE + STAT_SLEEPING);

/* TASFALEN */

  if (b_savevers < (char) SAV_WTNSVERS)
  {                             /* no witness record save in file */

    if ((restoreSkills(buff + skill_off, ch, MAX_SKILLS) + skill_off) !=
        affect_off)
    {
      logit(LOG_FILE, "Warning: restoreSkills() not match offset.");
      fprintf(stderr, "Problem restoring save file of: %s\n", name);
      logit(LOG_FILE, "Problem restoring save file of %s.", name);
      send_to_char
        ("There is something wrong with your save file!  Please talk to a God.\r\n",
         ch);
      return -2;
    }
  }
  else
  {

    if ((restoreSkills(buff + skill_off, ch, MAX_SKILLS) + skill_off) !=
        witness_off)
    {
      logit(LOG_FILE, "Warning: restoreSkills() not match offset.");
      fprintf(stderr, "Problem restoring save file of: %s\n", name);
      logit(LOG_FILE, "Problem restoring save file of %s.", name);
      send_to_char
        ("There is something wrong with your save file!  Please talk to a God.\r\n",
         ch);
      return -2;
    }
#ifndef _PFILE_
    if ((restoreWitness(buff + witness_off, ch) + witness_off) != affect_off)
    {
      logit(LOG_FILE, "Warning: restoreWitness() not match offset.");
      fprintf(stderr, "Problem restoring save file of: %s\n", name);
      logit(LOG_FILE, "Problem restoring save file of %s.", name);
      send_to_char
        ("There is something wrong with your save file!  Please talk to a God.\r\n",
         ch);
      return -2;
    }
#endif
  }
  /* end TASFALEN */

  return type;
}

P_obj restoreObjects(char *buf, P_char ch, int not_room)
{
  P_obj    obj, c_obj = NULL;
  bool     dummy_obj;
  byte     dummy_byte, o_f_flag;
  int      tmp, count, i, loc, obj_count = 0, V_num, i_count, ignore = 0, k;
  struct extra_descr_data *t_desc;
  struct obj_data d_obj;
  ulong    o_u_flag;
  int      new_vnum = 0;
  P_obj    root_obj = NULL;
  int      purge_randoms = FALSE;

  obj_vers = (int) GET_BYTE(buf);
  if (obj_vers > SAV_ITEMVERS)
  {
    if (ch)
    {
      logit(LOG_FILE, "Item save versions don't match (%d, %d) for %s.",
            obj_vers, SAV_ITEMVERS, GET_NAME(ch));
      send_to_char
        ("Your objects are in a format which the game doesn't know how\r\n"
         "to load. Please log on with another character and talk to a God.\r\n",
         ch);
    }
    else
    {
      logit(LOG_FILE, "Item save versions don't match (%d, %d) for pcorpse.",
            obj_vers, SAV_ITEMVERS);
    }
    return 0;
  }

  // Randoms purge 12/14/08 - Torgal
  if( obj_vers < 35 )
  {
    purge_randoms = TRUE;
  }

  count = GET_INTE(buf);

  /* hack to show its a saved item. Log for debugging */
  if (!ch && not_room == 2)
    logit(LOG_OBJ, "Count is %d.", count);

  /*
   * due to vast changes, much easier to start fresh with version 5
   */

  for (;;)
  {
    dummy_obj = FALSE;
    o_u_flag = 0;
    loc = 0;
    i_count = 1;
    o_f_flag = GET_BYTE(buf);

    if (o_f_flag & O_F_EOL)
    {
      if (!c_obj && !ignore)
      {                         /*
                                 * end of the whole list
                                 */
        if (obj_count != count)
          return NULL;
        else if (ch)
        {
          /*
           * ok, simplest hack to make sure carried weight
           * is correct, have to do this because obj_to_obj
           * does not correctly updated carried weight (and
           * would be ugly to fix).  JAB
           */
          GET_CARRYING_W(ch) = 0;
          for (obj = ch->carrying; obj; obj = obj->next_content)
            GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
        }
        return root_obj ? root_obj : (P_obj) 1;
      }
      if (!ignore)
      {
        if (OBJ_INSIDE(c_obj))
          c_obj = c_obj->loc.inside;
        else
          c_obj = NULL;
      }
      else
        ignore--;

      continue;
    }

    V_num = GET_INTE(buf);

    if( purge_randoms && (V_num == VOBJ_RANDOM_ARMOR || V_num == VOBJ_RANDOM_WEAPON) )
    {
      logit(LOG_OBJ, "Purging random #%d", V_num);
      obj = NULL;
    }
    else
    {
      obj = read_object(V_num, VIRTUAL);
    }
   
		if (!obj)
    {
      logit(LOG_OBJ, "Could not load object #%d for %s.", V_num,
            (ch) ? GET_NAME(ch) : "pcorpse");
      obj = &d_obj;
      bzero(obj, sizeof(struct obj_data));
      dummy_obj = TRUE;

      /*
       * have to fudge, if we can't load a container, we have to
       * keep track or the list will end prematurely.
       */
      if (o_f_flag & O_F_CONTAINS)
        ignore++;

		}

  	obj->g_key = 1;

   	if (!root_obj)
     	root_obj = obj;
 		
    if (obj_vers < 32)
    {
      GET_SHORT(buf);
      GET_SHORT(buf);
      GET_SHORT(buf);
    }
    obj->craftsmanship = GET_SHORT(buf);
    if (obj_vers < 32)
      GET_SHORT(buf);
    obj->condition = GET_SHORT(buf);
    if (o_f_flag & O_F_WORN)
    {
      loc = GET_BYTE(buf);
      if (!dummy_obj && (loc > 0))
        save_equip[loc - 1] = obj;
    }

    if (o_f_flag & O_F_COUNT)
      i_count = GET_SHORT(buf);

    if (o_f_flag & O_F_AFFECTS)
    {
      if (obj_vers < 32)
      {
        tmp = (int) GET_SHORT(buf);
      }
      else
      {
        tmp = GET_BYTE(buf);
        while (tmp--)
        {
          int      time = GET_INTE(buf);
          sh_int   type = GET_SHORT(buf);
          sh_int   data = GET_SHORT(buf);
          ulong    extra2 = 0;

          if (obj_vers >= 33)
            extra2 = (ulong) GET_INTE(buf);
          if (type == TAG_ALTERED_EXTRA2)
            continue;
#ifndef _PFILE_
          if (extra2)
            set_obj_affected_extra(obj, time, type, data, extra2);
          else
            set_obj_affected(obj, time, type, data);
#endif
        }
      }
    }

    if (o_f_flag & O_F_UNIQUE)
    {
      o_u_flag = GET_INTE(buf);
      obj->str_mask = (o_u_flag & (O_U_KEYS | O_U_DESC1 | O_U_DESC2 | O_U_DESC3));

      if (obj->str_mask)
      {
        if (o_u_flag & O_U_KEYS)
          obj->name = GET_STRING(buf);
        if (o_u_flag & O_U_DESC1)
          obj->description = GET_STRING(buf);
        if (o_u_flag & O_U_DESC2)
          obj->short_description = GET_STRING(buf);
        if (o_u_flag & O_U_DESC3)
          obj->action_description = GET_STRING(buf);
      }
      if (o_u_flag & O_U_EDESC)
      {
        // note:  this mask is ONLY used for writing and restoring the object.
        //   "normal" objects allocate seperate memory for extra descs already
        obj->str_mask |= STRUNG_EDESC;

        struct extra_descr_data *ed, *next_one;
        for (ed = obj->ex_description; ed; ed = next_one)
        {
          next_one = ed->next;
          if (ed->keyword)
            str_free(ed->keyword);
          if (ed->description)
            str_free(ed->description);
          FREE(ed);
        }
        obj->ex_description = NULL;
        int nDescs = GET_SHORT(buf);
        struct extra_descr_data **lastOne = &(obj->ex_description);
        while (*lastOne)
          lastOne = &((*lastOne)->next);
        while (nDescs--)
        {
          CREATE(ed, extra_descr_data, 1, MEM_TAG_EXDESCD);
          ed->next = NULL;
          ed->keyword = GET_STRING(buf);
          ed->description = GET_STRING(buf);
          *lastOne = ed;
          lastOne = &(ed->next);
        }
      }
      if (o_u_flag & O_U_VAL0)
        obj->value[0] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL1)
        obj->value[1] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL2)
        obj->value[2] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL3)
        obj->value[3] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL4)
        obj->value[4] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL5)
        obj->value[5] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL6)
        obj->value[6] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL7)
        obj->value[7] = GET_INTE(buf);

      if (o_u_flag & O_U_TIMER)
      {
        for (i = 0; i < 4; i++)
        {
          obj->timer[i] = GET_INTE(buf);
        }
      }

      if (o_u_flag & O_U_TRAP)
      {
        obj->trap_eff = GET_SHORT(buf);
        obj->trap_dam = GET_SHORT(buf);
        obj->trap_charge = GET_SHORT(buf);
        obj->trap_level = GET_SHORT(buf);
      }
      if (o_u_flag & O_U_TYPE)
      {
        obj->type = GET_BYTE(buf);
      }
      if (o_u_flag & O_U_WEAR)
      {
        obj->wear_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_EXTRA)
      {
        obj->extra_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_ANTI)
      {
        obj->anti_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_ANTI2)
      {
        obj->anti2_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_EXTRA2)
      {
        obj->extra2_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_WEIGHT)
      {
        obj->weight = GET_INTE(buf);
      }
      if (o_u_flag & O_U_MATERIAL)
      {
        obj->material = GET_BYTE(buf);
      }
      if (o_u_flag & O_U_SPACE)
      {
        if (obj_vers < 32)
          GET_BYTE(buf);
      }
      if (o_u_flag & O_U_COST)
      {
        obj->cost = GET_INTE(buf);
      }
      if (o_u_flag & O_U_BV1)
      {
        obj->bitvector = GET_LONG(buf);
      }
      if (o_u_flag & O_U_BV2)
      {
        obj->bitvector2 = GET_LONG(buf);
      }
      if (o_u_flag & O_U_BV3)
      {
        obj->bitvector3 = GET_LONG(buf);
      }
      if (o_u_flag & O_U_BV4)
      {
        obj->bitvector4 = GET_LONG(buf);
      }
      if (o_u_flag & O_U_BV5)
      {
        obj->bitvector5 = GET_LONG(buf);
      }
      if (o_u_flag & O_U_AFFS)
      {
        for (i = 0; i < MAX_OBJ_AFFECT; i++)
        {
          obj->affected[i].location = GET_BYTE(buf);
          obj->affected[i].modifier = GET_BYTE(buf);
        }
        if (obj_vers < 32)
        {
          GET_BYTE(buf);
          GET_BYTE(buf);
          GET_BYTE(buf);
          GET_BYTE(buf);
        }
      }
      if (o_f_flag & O_F_SPELLBOOK)
      {
        if (obj->type == ITEM_SPELLBOOK)
        {
          if( obj_vers < 34 )
          {
            GET_BYTE(buf);
            tmp = 251;
          }
          else
          {
            tmp = GET_INTE(buf);
          }

          if (tmp)
          {                     /*
                                 * create fake spell description
                                 * thing
                                 */
            CREATE(t_desc, extra_descr_data, 1, MEM_TAG_EXDESCD);

            t_desc->next = obj->ex_description;
            obj->ex_description = t_desc;
            t_desc->keyword = str_dup("\03\01\03");
            CREATE(t_desc->description, char, ((MAX_SKILLS / 8) + 1), MEM_TAG_STRING);

            for (i = 0; i < tmp; i++)
              t_desc->description[i] = GET_BYTE(buf);
            for (i = tmp; i < (MAX_SKILLS / 8 + 1); i++)
              t_desc->description[i] = 0;
          }
        }
        else
        {                       /*
                                 * was causing some silly things before..
                                 * but now fixed: if item _was_ spellbook
                                 * and had desc, will have no longer.
                                 */
          tmp = GET_BYTE(buf);
          for (i = 0; i < tmp; i++)
          {
            dummy_byte = GET_BYTE(buf);
          }
        }
      }
    }

    obj_count += i_count;

    if (!dummy_obj)
    {
      do
      {
#ifdef _PFILE_
        if (c_obj)
        {
          obj->loc_p = LOC_INSIDE;
          obj->loc.inside = c_obj;
        }

        obj_to_char(obj, ch);
#else
        if( IS_SET(obj->extra_flags, ITEM_PROCLIB) && (NULL == get_scheduled(obj, proclib_obj_event)) )
        {
          add_event(proclib_obj_event, PULSE_MOBILE + number(-4, 4), NULL, NULL, obj, 0, NULL, 0);
        }

        if (c_obj && (c_obj->type == ITEM_QUIVER || !ch))
          obj_to_obj(obj, c_obj);
        else if (c_obj && ((GET_OBJ_WEIGHT(obj) + GET_OBJ_WEIGHT(c_obj) <= c_obj->value[0]) || !ch))
        {
          obj_to_obj(obj, c_obj);
        }
        else
        {
          if (ch)
            obj_to_char(obj, ch);
          else
          {
            obj_to_room(obj, corpse_room);
            if (not_room)
              if (obj->type != ITEM_CORPSE)
                writeSavedItem(obj);
          }
        }
#endif
        if (--i_count)
          obj = read_object(V_num, VIRTUAL);
      }
      while (i_count);

      if (o_f_flag & O_F_CONTAINS)
        c_obj = obj;
    }
  }

  if (ch)
  {
    /*
     * ok, simplest hack to make sure carried weight is correct, have
     * to do this because obj_to_obj does not correctly updated
     * carried weight (and would be ugly to fix).  JAB
     */

    GET_CARRYING_W(ch) = 0;
    for (obj = ch->carrying; obj; obj = obj->next_content)
      GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
  }

  return root_obj ? root_obj : (P_obj) 1;
}

/* this function loads *one* object from a char buffer. 
  if it is a container, any possible contents will *not* be loaded */
P_obj read_one_object(char *read_buf)
{
  char *buf = read_buf;
  P_obj    obj;
  byte     dummy_byte, o_f_flag;
  int      tmp, V_num, count, i_count;
  struct extra_descr_data *t_desc;
  struct obj_data d_obj;
  ulong    o_u_flag;

  obj_vers = (int) GET_BYTE(buf);
  if (obj_vers > SAV_ITEMVERS)
  {
		logit(LOG_DEBUG, "read_one_object(): invalid item save version! (%d, %d)",
            obj_vers, SAV_ITEMVERS);
		return NULL;
  }
  
  count = GET_INTE(buf);

    o_f_flag = GET_BYTE(buf);
    o_u_flag = 0;
    i_count = 1;

    if (o_f_flag & O_F_EOL)
    {
		logit(LOG_DEBUG, "read_one_object(): premature end of object string.");
 		return NULL;
   	}

    V_num = GET_INTE(buf);
    obj = read_object(V_num, VIRTUAL);
 
    if (!obj)
    {
      logit(LOG_DEBUG, "read_one_object(): could not load object %d\n", V_num);
      return NULL;
    }

    obj->g_key = 1;
    obj->craftsmanship = GET_SHORT(buf);
    obj->condition = GET_SHORT(buf);

    if (o_f_flag & O_F_COUNT)
      i_count = GET_SHORT(buf);

	if( i_count != 1 ) {
		logit(LOG_DEBUG, "read_one_object(): object i_count != 1\n");
		return NULL;
	}

    if (o_f_flag & O_F_AFFECTS)
    {
        tmp = GET_BYTE(buf);
        while (tmp--)
        {
          int      time = GET_INTE(buf);
          sh_int   type = GET_SHORT(buf);
          sh_int   data = GET_SHORT(buf);
          ulong    extra2 = 0;

          if (obj_vers >= 33)
            extra2 = (ulong) GET_INTE(buf);

          if (type == TAG_ALTERED_EXTRA2)
            continue;

#ifndef _PFILE_
          if (extra2)
            set_obj_affected_extra(obj, time, type, data, extra2);
          else
            set_obj_affected(obj, time, type, data);
#endif            
        }
      
    }

    if (o_f_flag & O_F_UNIQUE)
    {
      o_u_flag = GET_INTE(buf);
      obj->str_mask = (o_u_flag & (O_U_KEYS | O_U_DESC1 | O_U_DESC2 | O_U_DESC3));

      if (obj->str_mask)
      {
        if (o_u_flag & O_U_KEYS)
          obj->name = GET_STRING(buf);
        if (o_u_flag & O_U_DESC1)
          obj->description = GET_STRING(buf);
        if (o_u_flag & O_U_DESC2)
          obj->short_description = GET_STRING(buf);
        if (o_u_flag & O_U_DESC3)
          obj->action_description = GET_STRING(buf);
      }
      if (o_u_flag & O_U_EDESC)
      {
        // note:  this mask is ONLY used for writing and restoring the object.
        //   "normal" objects allocate seperate memory for extra descs already
        obj->str_mask |= STRUNG_EDESC;

        struct extra_descr_data *ed, *next_one;
        for (ed = obj->ex_description; ed; ed = next_one)
        {
          next_one = ed->next;
          if (ed->keyword)
            str_free(ed->keyword);
          if (ed->description)
            str_free(ed->description);
          FREE(ed);
        }
        obj->ex_description = NULL;
        int nDescs = GET_SHORT(buf);
        struct extra_descr_data **lastOne = &(obj->ex_description);
        while (*lastOne)
          lastOne = &((*lastOne)->next);
        while (nDescs--)
        {
          CREATE(ed, extra_descr_data, 1, MEM_TAG_EXDESCD);
          ed->next = NULL;
          ed->keyword = GET_STRING(buf);
          ed->description = GET_STRING(buf);
          *lastOne = ed;
          lastOne = &(ed->next);
        }
      }
      if (o_u_flag & O_U_VAL0)
        obj->value[0] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL1)
        obj->value[1] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL2)
        obj->value[2] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL3)
        obj->value[3] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL4)
        obj->value[4] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL5)
        obj->value[5] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL6)
        obj->value[6] = GET_INTE(buf);
      if (o_u_flag & O_U_VAL7)
        obj->value[7] = GET_INTE(buf);

      if (o_u_flag & O_U_TIMER)
      {
        for (int i = 0; i < 4; i++)
        {
          obj->timer[i] = GET_INTE(buf);
        }
      }

      if (o_u_flag & O_U_TRAP)
      {
        obj->trap_eff = GET_SHORT(buf);
        obj->trap_dam = GET_SHORT(buf);
        obj->trap_charge = GET_SHORT(buf);
        obj->trap_level = GET_SHORT(buf);
      }
      if (o_u_flag & O_U_TYPE)
      {
        obj->type = GET_BYTE(buf);
      }
      if (o_u_flag & O_U_WEAR)
      {
        obj->wear_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_EXTRA)
      {
        obj->extra_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_ANTI)
      {
        obj->anti_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_ANTI2)
      {
        obj->anti2_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_EXTRA2)
      {
        obj->extra2_flags = GET_INTE(buf);
      }
      if (o_u_flag & O_U_WEIGHT)
      {
        obj->weight = GET_INTE(buf);
      }
      if (o_u_flag & O_U_MATERIAL)
      {
        obj->material = GET_BYTE(buf);
      }
      if (o_u_flag & O_U_SPACE)
      {
        if (obj_vers < 32)
          GET_BYTE(buf);
      }
      if (o_u_flag & O_U_COST)
      {
        obj->cost = GET_INTE(buf);
      }
      if (o_u_flag & O_U_BV1)
      {
        obj->bitvector = GET_LONG(buf);
      }
      if (o_u_flag & O_U_BV2)
      {
        obj->bitvector2 = GET_LONG(buf);
      }
      if (o_u_flag & O_U_BV3)
      {
        obj->bitvector3 = GET_LONG(buf);
      }
      if (o_u_flag & O_U_BV4)
      {
        obj->bitvector4 = GET_LONG(buf);
      }
      if (o_u_flag & O_U_BV5)
      {
        obj->bitvector5 = GET_LONG(buf);
      }
      if (o_u_flag & O_U_AFFS)
      {
        for (int i = 0; i < MAX_OBJ_AFFECT; i++)
        {
          obj->affected[i].location = GET_BYTE(buf);
          obj->affected[i].modifier = GET_BYTE(buf);
        }
        if (obj_vers < 32)
        {
          GET_BYTE(buf);
          GET_BYTE(buf);
          GET_BYTE(buf);
          GET_BYTE(buf);
        }
      }
      if (o_f_flag & O_F_SPELLBOOK)
      {
        if (obj->type == ITEM_SPELLBOOK)
        {
          if( obj_vers < 34 )
          {
            GET_BYTE(buf);
            tmp = 251;
          }
          else
          {
            tmp = GET_INTE(buf);
          }
          
          if (tmp)
          {                     /*
                                 * create fake spell description
                                 * thing
                                 */
            CREATE(t_desc, extra_descr_data, 1, MEM_TAG_EXDESCD);

            t_desc->next = obj->ex_description;
            obj->ex_description = t_desc;
            t_desc->keyword = str_dup("\03\01\03");
            CREATE(t_desc->description, char, ((MAX_SKILLS / 8) + 1), MEM_TAG_STRING);

            for (int i = 0; i < tmp; i++)
              t_desc->description[i] = GET_BYTE(buf);
            for (int i = tmp; i < (MAX_SKILLS / 8 + 1); i++)
              t_desc->description[i] = 0;
          }
        }
        else
        {                       /*
                                 * was causing some silly things before..
                                 * but now fixed: if item _was_ spellbook
                                 * and had desc, will have no longer.
                                 */
          tmp = GET_BYTE(buf);
          for (int i = 0; i < tmp; i++)
          {
            dummy_byte = GET_BYTE(buf);
          }
        }
      }
    }

#ifndef _PFILE_
    if( IS_SET(obj->extra_flags, ITEM_PROCLIB) && (NULL == get_scheduled(obj, proclib_obj_event)) )
    {
      add_event(proclib_obj_event, PULSE_MOBILE + number(-4, 4), NULL, NULL, obj, 0, NULL, 0);
    }
#endif

	return obj;
}


#ifndef _PFILE_

int confiscate_item(P_char ch, int debt)
{
  int      value = 2, i;
  P_obj    cobj = 0, obj, obj2;

  /*
   * find most expensive item first
   */
  obj = ch->carrying;
  while (obj)
  {
    if (obj->cost > value && !obj->contains)
    {
      cobj = obj;
      value = cobj->cost;
    }
    if (OBJ_INSIDE(obj))
      if (obj->next_content)
        obj = obj->next_content;
      else
        obj = obj->loc.inside->next_content;
    else
      obj = obj->next_content;
  }

  if (!value || !cobj)
    return 0;

  /*
   * if most expensive item won't cover it, grab it
   */

  if (((value * 3) / 4) < debt)
  {
    act("Confiscating your $o", FALSE, ch, cobj, 0, TO_CHAR);
    for (i = 0; i < MAX_WEAR; i++)
      if (save_equip[i] == cobj)
        save_equip[i] = NULL;
    if (cobj->contains)
    {
      obj = cobj->contains;
      while (obj)
      {
        obj2 = obj->next_content;
        obj_from_obj(obj);
        obj_to_char(obj, ch);
        obj = obj2;
      }
    }
    extract_obj(cobj, TRUE); // If arti is confiscated.. ouch.
    return ((value * 3) / 4);
  }
  /*
   * if we get here most expensive item is worth more than the debt, so
   * find the LEAST expensive item that will cover the debt
   */

  obj = ch->carrying;
  cobj = NULL;
  while (obj)
  {
    if ((obj->cost < value) &&
        (((obj->cost * 3) / 4) >= debt) && !cobj->contains)
    {
      cobj = obj;
      value = cobj->cost;
    }
    if (OBJ_INSIDE(obj))
      if (obj->next_content)
        obj = obj->next_content;
      else
        obj = obj->loc.inside->next_content;
    else
      obj = obj->next_content;
  }
  if (!cobj)
    return 0;
  act("Confiscating your $o", FALSE, ch, cobj, 0, TO_CHAR);
  for (i = 0; i < MAX_WEAR; i++)
    if (save_equip[i] == cobj)
      save_equip[i] = NULL;
#   if 0
  if (cobj->contains)
  {
    obj = cobj->contains;
    for (obj = cobj->contains; obj; obj = obj2)
    {
      obj2 = obj->next_content;
      obj_from_obj(obj);
      obj_to_char(obj, ch);
    }
  }
#   endif
  extract_obj(cobj, TRUE); // If arti is confiscated.. ouch.
  return ((value * 3) / 4);
}

void confiscate_all(P_char ch)
{
  int      i;
  P_obj    obj, obj2;

  for (i = 0; i < MAX_WEAR; i++)
  {
    save_equip[i] = NULL;
    if (ch->equipment[i])
    {
      extract_obj(ch->equipment[i], TRUE); // If arti is confiscated.. ouch!
      ch->equipment[i] = NULL;
    }
  }
  obj = ch->carrying;
  while (obj)
  {
    obj2 = obj->next_content;
    extract_obj(obj, TRUE); // If arti is confiscated.. ouch!
    obj = obj2;
  }
}

/*
 * this table relates equipment position to the 'wear()' keyword
 */

#endif

int      restore_wear[MAX_WEAR] = {
  -2,                           /* WEAR_LIGHT */
  1,                            /* WEAR_FINGER_R */
  1,                            /* WEAR_FINGER_L */
  2,                            /* WEAR_NECK_1 */
  2,                            /* WEAR_NECK_2 */
  3,                            /* WEAR_BODY */
  4,                            /* WEAR_HEAD */
  5,                            /* WEAR_LEGS */
  6,                            /* WEAR_FEET */
  7,                            /* WEAR_HANDS */
  8,                            /* WEAR_ARMS */
  14,                           /* WEAR_SHIELD */
  9,                            /* WEAR_ABOUT */
  10,                           /* WEAR_WAIST */
  11,                           /* WEAR_WRIST_R */
  11,                           /* WEAR_WRIST_L */
  12,                           /* PRIMARY_WEAPON */
  12,                           /* SECONDARY_WEAPON */
  13,                           /* HOLD */
  15,                           /* WEAR_EYES */
  16,                           /* WEAR_FACE */
  17,                           /* WEAR_EARRING_R */
  17,                           /* WEAR_EARRING_L */
  18,                           /* WEAR_QUIVER */
  19,                           /* GUILD_INSIGNIA */
  12,                           /* THIRD_WEAPON */
  12,                           /* FOURTH_WEAPON */
  20,                           /* WEAR_BACK */
  21,                           /* WEAR_ATTACH_BELT_1 */
  21,                           /* WEAR_ATTACH_BELT_2 */
  21,                           /* WEAR_ATTACH_BELT_3 */
  8,                            /* WEAR_ARMS2 */
  7,                            /* WEAR_HANDS2 */
  11,                           /* WEAR_WRIST_LR */
  11,                           /* WEAR_WRIST_LL */
  22,                           /* WEAR_HORSE_BODY */
  5,                            /* WEAR_LEGS_REAR */
  23,                           /* WEAR_TAIL */
  6,                            /* WEAR_FEET_REAR */
  24,                           /* WEAR_NOSE */
  25,                           /* WEAR_HORN */
  26,                           /* WEAR_IOUN */
  27                            /* WEAR_SPIDER_BODY */
};

/*
 * -1 = ran out of rent money -2 = couldn't read file (tried to rename to
 * name.bad) >= 0 = rent amount charged
 */

int restoreItemsOnly(P_char ch, int flatrate)
{
  int      wearSuccess;
#ifndef _PFILE_
  FILE    *f;
  char     buff[SAV_MAXSIZE];
  char    *buf = buff;
  int      skill_off, item_off, affect_off;
#endif
  int      size, csize, tmp, witness_off;
  byte     dummy_byte;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     b_savevers;
  char     buf1[256];

  if (!ch)
    return -2;
#ifndef _PFILE_

  strcpy(buff, GET_NAME(ch));
  for (; *buf; buf++)
    *buf = LOWER(*buf);
  buf = buff;
  sprintf(Gbuf1, "%s/%c/%s", SAVE_DIR, *buff, buff);

  f = fopen(Gbuf1, "r");
  if (!f)
    return -2;

  buf = buff;
  size = fread(buf, 1, SAV_MAXSIZE, f);
  fclose(f);
  if (size < 4)
  {
    fprintf(stderr, "Problem restoring inventory of: %s\n", GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring inventory of %s.", GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }

  b_savevers = GET_BYTE(buf);

  if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
      (GET_BYTE(buf) != long_size))
  {
    logit(LOG_FILE, "Save file in different machine format.");
    fprintf(stderr, "Problem restoring inventory of: %s\n", GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring inventory of %s.", GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }
  if (size < (5 * int_size + 5 * sizeof(char) + long_size))
  {
    logit(LOG_FILE, "Save file is too small %d.", size);
    fprintf(stderr, "Problem restoring inventory of: %s\n", GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring inventory of %s.", GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }
  dummy_byte = GET_BYTE(buf);
  skill_off = GET_INTE(buf);

  if (b_savevers >= (char) SAV_WTNSVERS)        /* no witness record save in file */
    witness_off = GET_INTE(buf);        /* TASFALEN */

  affect_off = GET_INTE(buf);
  item_off = GET_INTE(buf);
  csize = GET_INTE(buf);
  if (size != csize)
  {
    logit(LOG_FILE, "Save file size %d not match size read %d.", size, csize);
    fprintf(stderr, "Problem restoring inventory of: %s\n", GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring inventory of %s.", GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }

  if ((restoreAffects(buff + affect_off, ch) + affect_off) != item_off)
  {
    logit(LOG_FILE, "Warning: restoreAffects() not match offset.");
    fprintf(stderr,
            "Problem restoring save file of: %s was %d, should be %d\n",
            GET_NAME(ch), affect_off, item_off);
    logit(LOG_FILE, "Problem restoring save file of %s.", GET_NAME(ch));
    send_to_char("There is something wrong with your save file!  Please talk to a God.\r\n", ch);
    return -2;
  }

  readShapechangeData(ch);

  for (tmp = 0; tmp < MAX_WEAR; tmp++)
    save_equip[tmp] = NULL;

#endif

  if( !restoreObjects(buff + item_off, ch, 1) )
  {
    fprintf(stderr, "Problem restoring inventory of: %s\n", GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring inventory of %s.", GET_NAME(ch));
#ifndef _PFILE_
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
#endif
    return -2;
  }
  for (tmp = 0; tmp < MAX_WEAR; tmp++)
  {
    if (save_equip[tmp] != NULL)
    {
      wearSuccess = wear(ch, save_equip[tmp], restore_wear[tmp], FALSE);
      if (!wearSuccess && restore_wear[tmp] == 12)
        wearSuccess = wear(ch, save_equip[tmp], 13, FALSE);
    }
  }
//  wear(ch, save_equip[tmp], restore_wear[tmp], 0);

  return 0;
}

/*
 * routine called at boottime, goes through Corpses dir and loads all
 * player corpses back into the game.
 */

#ifndef _PFILE_

void restoreCorpses(void)
{
  FILE    *flist, *f;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH], buff[SAV_MAXSIZE], *buf;
  char     mybuf[MAX_STRING_LENGTH];
  int      size, csize, tmp, start, map, end;
  struct stat statbuf;

  sprintf(Gbuf1, "%s/Corpses", SAVE_DIR);
  if (stat(Gbuf1, &statbuf) == -1)
  {
    perror("Corpses dir");
    return;
  }
  sprintf(Gbuf2, "%s/corpse_list", SAVE_DIR);
  if (stat(Gbuf2, &statbuf) == 0)
  {
    unlink(Gbuf2);
  }
  else if (errno != ENOENT)
  {
    perror("corpse_list");
    return;
  }
  sprintf(Gbuf3, "/bin/ls -1 %s > %s", Gbuf1, Gbuf2);
  system(Gbuf3);                /* ls a list of Corpses dir into corpse_list */
  flist = fopen(Gbuf2, "r");
  if (!flist)
    return;

  while (fscanf(flist, " %s \n", Gbuf2) != EOF)
  {
    sprintf(Gbuf3, "%s/%s", Gbuf1, Gbuf2);
    f = fopen(Gbuf3, "r");
    if (!f)
    {
      logit(LOG_CORPSE, "Could not restore Corpse file %s", Gbuf2);
      continue;
    }
    buf = buff;
    size = fread(buf, 1, SAV_MAXSIZE, f);
    fclose(f);

    if (size < 4)
    {
      fprintf(stderr, "Problem restoring corpse: %s\n", Gbuf2);
      logit(LOG_FILE, "Problem restoring corpse: %s.", Gbuf2);
      continue;
    }

    if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
        (GET_BYTE(buf) != long_size))
    {
      logit(LOG_FILE, "Save file %s in different machine format.", Gbuf2);
      fprintf(stderr, "Problem restoring corpse: %s\n", Gbuf2);
      logit(LOG_FILE, "Problem restoring corpse: %s.", Gbuf2);
      continue;
    }
    if (size < (5 * int_size + 5 * sizeof(char) + long_size))
    {
      logit(LOG_FILE, "Corpse file %s is too small (%d).", Gbuf2, size);
      fprintf(stderr, "Problem restoring corpse: %s\n", Gbuf2);
      logit(LOG_FILE, "Problem restoring corpse: %s.", Gbuf2);
      continue;
    }
    /*
     * WHACK!  The sound of a hack in progress.  Ok, without doing
     * this, restoreObjects will put an empty corpse in the target
     * room, which is then saved (empty).  Result, unless something
     * saves the corpse again it will be empty after one crash/reboot,
     * very annoying.  So we save a backup, and copy the backup into
     * place, after the restoreObjects call. Two extra system calls
     * (rename) but that's cheaper than most other solutions.  JAB
     */

    sprintf(Gbuf2, "%s.bak", Gbuf3);
    if (rename(Gbuf3, Gbuf2) == -1)
    {
      logit(LOG_FILE, "Problem with player Corpses directory!\n");
      return;
    }
    tmp = GET_INTE(buf);
    //Check if tmp is a rooom within a random zone if soo move it to entrance
    f = fopen("Players/Corpses/RandomZoneList", "r+");
    if (f)
    {

      while (!(feof(f)))
      {
        if (fgets(mybuf, sizeof(mybuf) - 1, f))
        {
          if (sscanf(mybuf, "%d %d %d", &start, &end, &map) == 3)
            if ((tmp >= start) && (tmp <= end))
            {
              tmp = map;
            }

        }
      }

      fclose(f);
    }
    //End check
    if ((corpse_room = real_room(tmp)) == NOWHERE)
    {
      logit(LOG_FILE, "No room %d to load %s, loading into room 0",
            tmp, Gbuf2);
      corpse_room = 0;
    }
    csize = GET_INTE(buf);

    if (size != csize)
    {
      logit(LOG_FILE, "Corpse file %s size %d not match size read %d.",
            Gbuf2, size, csize);
      fprintf(stderr, "Problem restoring corpse: %s\n", Gbuf2);
      logit(LOG_FILE, "Problem restoring corpse: %s.", Gbuf2);
      continue;
    }
    if (restoreObjects(buf, 0, 1))
    {
      /*
       * Hack part Deux, put the loaded pcorpse back
       */
      unlink(Gbuf3);
      rename(Gbuf2, Gbuf3);
      continue;
    }
  }

  fclose(flist);
}

/** Pet only functions below. Calls some from above. **/

int writePetStatus(char *buf, P_char ch)
{
  char    *start = buf;

  ADD_STRING(buf, GET_NAME(ch));
  ADD_STRING(buf, ch->player.short_descr);
  ADD_STRING(buf, ch->player.long_descr);
  ADD_STRING(buf, ch->player.description);
//  if (!ch->only.npc->owner)
//    ch->only.npc->owner = str_dup("shop");
//  ADD_STRING(buf, ch->only.npc->owner);
  ADD_BYTE(buf, ch->player.m_class);    // MUST BE UPGRADED TO INT SOMETIME
  //GET_CLASS(ch));
  ADD_BYTE(buf, GET_RACE(ch));
  ADD_BYTE(buf, GET_LEVEL(ch));
  ADD_BYTE(buf, GET_SEX(ch));
  ADD_SHORT(buf, ch->player.weight);
  ADD_SHORT(buf, ch->player.height);
  ADD_BYTE(buf, GET_SIZE(ch));
  ADD_BYTE(buf, GET_HOME(ch));
  ADD_INT(buf, mob_index[GET_RNUM(ch)].virtual_number);
  ADD_INT(buf, world[ch->in_room].number);

  ADD_BYTE(buf, ch->base_stats.Str);
  ADD_BYTE(buf, ch->base_stats.Dex);
  ADD_BYTE(buf, ch->base_stats.Agi);
  ADD_BYTE(buf, ch->base_stats.Con);
  ADD_BYTE(buf, ch->base_stats.Pow);
  ADD_BYTE(buf, ch->base_stats.Int);
  ADD_BYTE(buf, ch->base_stats.Wis);
  ADD_BYTE(buf, ch->base_stats.Cha);
  ADD_BYTE(buf, ch->base_stats.Kar);
  ADD_BYTE(buf, ch->base_stats.Luk);

  ADD_SHORT(buf, GET_MANA(ch));
  ADD_SHORT(buf, ch->points.base_mana);
  ADD_SHORT(buf, GET_HIT(ch));
  ADD_SHORT(buf, ch->points.base_hit);
  ADD_SHORT(buf, GET_VITALITY(ch));
  ADD_SHORT(buf, ch->points.base_vitality);

  ADD_INT(buf, GET_COPPER(ch));
  ADD_INT(buf, GET_SILVER(ch));
  ADD_INT(buf, GET_GOLD(ch));
  ADD_INT(buf, GET_PLATINUM(ch));
  ADD_INT(buf, GET_EXP(ch));

  ADD_INT(buf, ch->specials.act);
  ADD_INT(buf, ch->specials.act2);
  ADD_INT(buf, ch->specials.alignment);

  return (int) (buf - start);
}

int writePet(P_char ch)
{
  FILE    *f;
  char    *buf, *affect_off, *item_off, *size_off;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int      i, bak;
  static char buff[SAV_MAXSIZE * 2];
  struct affected_type *af;
  struct stat statbuf;

  if (!ch || !GET_NAME(ch))
  {
    return 0;
  }

  if (IS_NPC(ch) && (!ch->following))
  {
    return 0;
  }
  if ((sizeof(char) != 1) || (int_size != long_size))
  {
    logit(LOG_DEBUG,
          "sizeof(char) must be 1 and int_size must == long_size for player saves!\n");
    return 0;
  }
  buf = buff;
  ADD_BYTE(buf, (char) (short_size));
  ADD_BYTE(buf, (char) (int_size));
  ADD_BYTE(buf, (char) (long_size));

  affect_off = buf;
  ADD_INT(buf, (int) 0);
  item_off = buf;
  ADD_INT(buf, (int) 0);
  size_off = buf;
  ADD_INT(buf, (int) 0);
  ADD_INT(buf, mob_index[GET_RNUM(ch)].virtual_number);

  ADD_LONG(buf, time(0));       /* save time */

  /* unequip everything and remove affects before saving */

  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      save_equip[i] = unequip_char(ch, i);
    else
      save_equip[i] = NULL;

  af = ch->affected;
  all_affects(ch, FALSE);       /* reset to unaffected state */
  buf += writePetStatus(buf, ch);
  ADD_INT(affect_off, (int) (buf - buff));
  updateShortAffects(ch);
  buf += writeAffects(buf, ch->affected);
  ADD_INT(item_off, (int) (buf - buff));
  buf += writeItems(buf, ch);
  ADD_INT(size_off, (int) (buf - buff));
#   if 0
  for (i = 0; i < MAX_WEAR; i++)
    if (save_equip[i])
    {
      extract_obj(save_equip[i]);
      save_equip[i] = NULL;
    }
  for (obj = ch->carrying; obj; obj = obj2)
  {
    obj2 = obj->next_content;
    extract_obj(obj);
    obj = NULL;
  }
#   endif
  /* might need above if I ever add a type call to this func */
  for (i = 0; i < MAX_WEAR; i++)
    if (save_equip[i])
      equip_char(ch, save_equip[i], i, 1);


  all_affects(ch, TRUE);        /* reapply affects (including equip) */

  if ((int) (buf - buff) > SAV_MAXSIZE)
  {
    logit(LOG_PLAYER, "Could not save %s, file too large (%d bytes)",
          GET_NAME(ch), (int) (buf - buff));
    return 0;
  }
//  if (!ch->only.npc->owner)
//    ch->only.npc->owner = str_dup("shop");
//  sprintf(Gbuf1, "%s/Pets/%s%d", SAVE_DIR, ch->only.npc->owner, GET_IDNUM(ch));

  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");

  if (stat(Gbuf1, &statbuf) == 0)
  {
    if (rename(Gbuf1, Gbuf2) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Problem with pet save files directory!\n");
      logit(LOG_FILE, "   rename failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error backing up petfile for %s!",
             GET_NAME(ch));
      return 0;
    }
    bak = 1;
  }
  else
  {
    if (errno != ENOENT)
    {
      int      tmp_errno;

      tmp_errno = errno;

      /*
       * NOTE: with the stat() function, only two errors are
       * possible: EBADF   filedes is bad. ENOENT  File does not
       * exist. Now, EBADF should only occur using fstat().
       * Therefore, if I fall into here, I have some SERIOUS
       * problems!
       */

      logit(LOG_FILE, "Problem with pet save files directory!\n");
      logit(LOG_FILE, "   stat failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error finding petfile for %s!",
             GET_NAME(ch));
      return 0;
    }
    /*
     * in this case, no original pfile existed.  Probably a new
     * char... so don't panic.
     */
    bak = 0;
  }

  f = fopen(Gbuf1, "w");

  if (!f)
  {
    int      tmp_errno;

    tmp_errno = errno;
    logit(LOG_FILE, "Couldn't create pet save file!\n");
    logit(LOG_FILE, "   fopen failed, errno = %d\n", tmp_errno);
    wizlog(OVERLORD, "&+R&-LPANIC!&N  Error creating petfile for %s!",
           GET_NAME(ch));
    bak -= 2;
  }
  else
  {
    if (fwrite(buff, 1, (unsigned) (buf - buff), f) != (buf - buff))
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Couldn't write to pet save file!\n");
      logit(LOG_FILE, "   fwrite failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error writing petfile for %s!",
             GET_NAME(ch));
      fclose(f);
      bak -= 2;
    }
    else
      fclose(f);
  }

  switch (bak)
  {
  case 1:                      /* save worked, just get rid of the backup */
    if (unlink(Gbuf2) == -1)    /* not a critical error */
      logit(LOG_FILE, "Couldn't delete backup of pet file.\n");
  case 0:                      /* save worked, no backup was made to begin with */
    break;
  case -1:                     /* save FAILED, but we have a backup */
    if (rename(Gbuf2, Gbuf1) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, " Unable to restore backup!  Argh!");
      logit(LOG_FILE, "    rename failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error restoring backup petfile for %s!",
             GET_NAME(ch));
      logit(LOG_EXIT, "unable to restore backup petfile for %s", GET_NAME(ch));
			raise(SIGSEGV);
    }
    else
      wizlog(OVERLORD, "        Backup restored.");
    /* restored or not, the save still failed, so return 0 */
    return 0;
  case -2:                     /* save FAILED, and we have NO backup! */
    logit(LOG_FILE, " No restore file was made!");
    wizlog(OVERLORD, "        No backup file available");
    return 0;
  }

  return 1;
}

int deletePet(char *id)
{
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (!id)
    return FALSE;

  sprintf(Gbuf1, "%s/Pets/%s", SAVE_DIR, id);
  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");
  unlink(Gbuf1);
  unlink(Gbuf2);

  return TRUE;
}
int restorePetStatus(char *buf, P_char ch)
{
  char    *start = buf;
  int      j;

  clearMemory(ch);

  if (IS_SET(ch->specials.act, ACT_SPEC))       /* No bogus procs!  */
    REMOVE_BIT(ch->specials.act, ACT_SPEC);
  all_affects(ch, FALSE);       /* Clean the slate first */
  GET_NAME(ch) = GET_STRING(buf);
  ch->player.short_descr = GET_STRING(buf);
  ch->player.long_descr = GET_STRING(buf);
  ch->player.description = GET_STRING(buf);
//  ch->only.npc->owner = GET_STRING(buf);

//  GET_CLASS(ch) = GET_BYTE(buf);
  ch->player.m_class = GET_BYTE(buf);   // should be updated, must be 16 bits or mroe
  GET_RACE(ch) = GET_BYTE(buf);

  setCharPhysTypeInfo(ch);      /* probably necessary..  or maybe not.  shrug */

//  GET_LEVEL(ch) = GET_BYTE(buf);
  ch->player.level = GET_BYTE(buf);
  GET_SEX(ch) = GET_BYTE(buf);
  ch->player.weight = GET_SHORT(buf);
  ch->player.height = GET_SHORT(buf);
  GET_HOME(ch) = GET_BYTE(buf);
  mob_index[GET_RNUM(ch)].virtual_number = GET_INTE(buf);
  GET_BIRTHPLACE(ch) = GET_INTE(buf);

  ch->player.time.birth = time(0);
  ch->base_stats.Str = (ubyte) GET_BYTE(buf);
  ch->base_stats.Dex = (ubyte) GET_BYTE(buf);
  ch->base_stats.Agi = (ubyte) GET_BYTE(buf);
  ch->base_stats.Con = (ubyte) GET_BYTE(buf);
  ch->base_stats.Pow = (ubyte) GET_BYTE(buf);
  ch->base_stats.Int = (ubyte) GET_BYTE(buf);
  ch->base_stats.Wis = (ubyte) GET_BYTE(buf);
  ch->base_stats.Cha = (ubyte) GET_BYTE(buf);
  ch->base_stats.Kar = (ubyte) GET_BYTE(buf);
  ch->base_stats.Luk = (ubyte) GET_BYTE(buf);
  ch->curr_stats = ch->base_stats;
  GET_MANA(ch) = GET_SHORT(buf);
  ch->points.base_mana = GET_SHORT(buf);
  GET_HIT(ch) = GET_SHORT(buf);
  if (GET_HIT(ch) < 0)
    GET_HIT(ch) = 0;
  ch->points.base_hit = GET_SHORT(buf);
  if (ch->points.base_hit < 1)
    ch->points.base_hit = 1;
  GET_VITALITY(ch) = GET_SHORT(buf);
  ch->points.base_vitality = GET_SHORT(buf);

  GET_COPPER(ch) = GET_INTE(buf);
  GET_SILVER(ch) = GET_INTE(buf);
  GET_GOLD(ch) = GET_INTE(buf);
  GET_PLATINUM(ch) = GET_INTE(buf);

  GET_COPPER(ch) = 0;
  GET_SILVER(ch) = 0;
  GET_GOLD(ch) = 0;
  GET_PLATINUM(ch) = 0;

  GET_EXP(ch) = GET_INTE(buf);
  ch->specials.act = GET_INTE(buf);
  ch->specials.act2 = GET_INTE(buf);
  ch->specials.alignment = GET_INTE(buf);
  ch->specials.fighting = NULL;
  ch->points.max_hit = ch->points.base_hit;
  ch->points.max_mana = ch->points.base_mana;
  ch->points.max_vitality = ch->points.base_vitality;

  SET_BIT(ch->specials.act, ACT_SENTINEL);
  SET_BIT(ch->specials.act, ACT_ISNPC);
  ch->specials.undead_spell_slots[0] = 0;
  for (j = 1; j <= MAX_CIRCLE; j++)
    ch->specials.undead_spell_slots[j] = spl_table[GET_LEVEL(ch)][j - 1];

  return (int) (buf - start);
}

P_char restorePet(char *id)
{
  FILE    *f;
  P_char   ch;
  char     buff[SAV_MAXSIZE];
  char    *buf = buff;
  int      start, size, csize, affect_off, item_off, tmp, virt;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (!id)
  {
    wizlog(OVERLORD, "Who? Pet's IDnum not given!");
    return 0;
  }
  sprintf(Gbuf1, "%s/Pets/%s", SAVE_DIR, id);

  f = fopen(Gbuf1, "r");
  if (!f)
  {
    logit(LOG_DEBUG, "Pet %s savefile does not exist!", id);
    wizlog(OVERLORD, "Pet %s savefile does not exist!", id);
    return 0;
  }
  size = fread(buf, 1, SAV_MAXSIZE, f);
  fclose(f);
  if (size < 4)
  {
    logit(LOG_FILE, "Warning: Save file less than 4 bytes for %s.", id);
  }
  if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
      (GET_BYTE(buf) != long_size))
  {
    wizlog(OVERLORD, "Ouch. Bad file sizing for %d", id);
    return 0;
  }
  if (size < 5 * int_size + 5 * sizeof(char) + long_size)
  {
    logit(LOG_FILE, "Warning: Petsave file is only %d bytes for %s.", size,
          id);
  }
  affect_off = GET_INTE(buf);
  item_off = GET_INTE(buf);
  csize = GET_INTE(buf);
  virt = GET_INTE(buf);
  if (size != csize)
  {
    wizlog(OVERLORD, "Warning: pet file size %d doesn't match csize %d for %s.",
           size, csize, id);
  }
  ch = read_mobile(virt, VIRTUAL);
  if (!ch)
  {
    wizlog(AVATAR, "Can not load pet %s, due to missing prototype %d!", id,
           virt);
    return 0;
  }
  GET_LONG(buf);
  start = (int) (buf - buff);
  restorePetStatus(buf, ch);
  restoreAffects(buff + affect_off, ch);

  for (tmp = 0; tmp < MAX_WEAR; tmp++)
    save_equip[tmp] = NULL;
/*
  restoreObjects(buff + item_off, ch, 1);
*/
  for (tmp = 0; tmp < MAX_WEAR; tmp++)
    if (save_equip[tmp] != NULL)
      wear(ch, save_equip[tmp], restore_wear[tmp], 0);

  convertMob(ch);
  return ch;
}

/* Support for reloading buried items */

void writeSavedItem(P_obj item)
{
  FILE    *f;
  P_obj    hold_content = NULL;
  bool     del_only = FALSE;    /* return after unlinking existing */
  char    *buf, *size_off;
  char     obj_dir_name[MAX_STRING_LENGTH], obj_file_name[MAX_STRING_LENGTH];
  char     obj_path[MAX_STRING_LENGTH];
  int      i_count = 0;
  int      restore_backup = FALSE;
  static char buffer[SAV_MAXSIZE * 2];

  if (!item)
  {
    logit(LOG_DEBUG, "writeSavedItem called with null item");
		return;  
	}
  if (item->cost < 100)         /* not worth saving */
    return;

  if (!OBJ_ROOM(item))
    del_only = TRUE;
  else if ((item->loc.room <= NOWHERE) || (item->loc.room > top_of_world))
  {
    logit(LOG_DEBUG, "writeSaveditem called with item someplace impossible");
  	return;
	}
  sprintf(obj_dir_name, "%s/SavedItems/", SAVE_DIR);

  sprintf(obj_file_name, "item.%s.%d", FirstWord(item->name), (int) item);

  for (buf = obj_file_name; *buf; buf++)
    *buf = LOWER(*buf);

  sprintf(obj_path, "%s/%s", obj_dir_name, obj_file_name);

  buf = buffer;

  ADD_INT(buf, world[item->loc.room].number);   /* reload room (VIRTUAL) */

  size_off = buf;               /* needed to make sure it's not corrupt */
  ADD_INT(buf, (int) 0);

  /*
   * have to hold the 'next_content' of item, as this is stuff in the
   * room and not to be saved.  Replaced after it's saved.  JAB
   */

  hold_content = item->next_content;

  item->next_content = NULL;

  i_count = countInven(item);

  ADD_BYTE(buf, (char) SAV_ITEMVERS);
  ADD_INT(buf, i_count);

  ibuf = buf;
  save_count = 0;

  writeObjectlist(item, (byte) 0);

  item->next_content = hold_content;

  if (save_count != i_count) {
		logit(LOG_DEBUG, "save count mismatch in writeSavedItem!");
		return;
	}

  ADD_INT(size_off, (int) (ibuf - buffer));

  f = fopen(obj_path, "w");
  if (!f)
  {
    logit(LOG_SAVED_OBJ, "Couldn't create SavedItem save file!\n");
    wizlog(57, "&+WCouldn't create SavedItem save file!\n");
    return;
  }

  if (fwrite(buffer, 1, (unsigned) (ibuf - buffer), f) != (ibuf - buffer))
  {
    logit(LOG_SAVED_OBJ, "Couldn't write to SavedItem save file!\n");
    wizlog(57, "&+WCouldn't write to SavedItem save file!\n");
  }
  fclose(f);
}

void restoreSavedItems(void)
{
  FILE    *obj_file;
  DIR     *obj_dir;
  struct dirent *obj_entry;
  char     obj_dir_name[MAX_STRING_LENGTH], obj_path[MAX_STRING_LENGTH];
  char     buffer[SAV_MAXSIZE], *buf;
  int      size, csize, virtual_room;
  P_obj    loaded[4096];
  int      count = 0;

  sprintf(obj_dir_name, "%s/SavedItems", SAVE_DIR);
  obj_dir = opendir(obj_dir_name);
  if (!obj_dir)
  {
    perror("SavedItems dir");
    return;
  }

  while ((obj_entry = readdir(obj_dir)) && count < 4096)
  {
    if (strstr(obj_entry->d_name, "item.") != obj_entry->d_name)
      continue;
    sprintf(obj_path, "%s/%s", obj_dir_name, obj_entry->d_name);
    obj_file = fopen(obj_path, "r");
    if (!obj_file)
    {
      logit(LOG_SAVED_OBJ, "Could not restore SavedItem file %s", obj_path);
      continue;
    }
    buf = buffer;
    size = fread(buffer, 1, SAV_MAXSIZE, obj_file);
    fclose(obj_file);

    if (size < 4)
    {
      fprintf(stderr, "Problem restoring SavedItem: %s\n", obj_path);
      logit(LOG_SAVED_OBJ, "Problem restoring SavedItem: %s.", obj_path);
      continue;
    }

    virtual_room = GET_INTE(buf);
    if ((corpse_room = real_room(virtual_room)) == NOWHERE)
    {
      logit(LOG_SAVED_OBJ, "No room %d to load %s, loading into room 0",
            virtual_room, obj_path);
      corpse_room = 0;
    }
    csize = GET_INTE(buf);

    if (size != csize)
    {
      logit(LOG_SAVED_OBJ,
            "SavedItem file %s size %d not match size read %d.", obj_path,
            size, csize);
      fprintf(stderr, "Problem restoring SavedItem: %s\n", obj_path);
      logit(LOG_SAVED_OBJ, "Problem restoring SavedItem: %s.", obj_path);
      continue;
    }
    unlink(obj_path);
    loaded[count++] = restoreObjects(buf, 0, 0);
  }

  closedir(obj_dir);

  while (count--)
    if (loaded[count])
      writeSavedItem(loaded[count]);
}

void PurgeSavedItemFile(P_obj item)
{
  char    *tmp;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (!item)
		return;

  sprintf(Gbuf2, "item.%s.%d", FirstWord(item->name), (int) item);
  for (tmp = Gbuf2; *tmp; tmp++)
    *tmp = LOWER(*tmp);

  sprintf(Gbuf1, "%s/SavedItems/%s", SAVE_DIR, Gbuf2);
  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");

  logit(LOG_FILE, "SAVEDITEM: deleting file %s.", Gbuf1);

  unlink(Gbuf1);
  unlink(Gbuf2);

  return;
}

/*** Shopkeeper saving ***/

int writeShopKeeper(P_char ch)
{
  FILE    *f;
  char    *buf, *affect_off, *item_off, *size_off;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int      i, bak, shop_nr;
  static char buff[SAV_MAXSIZE * 2];
  struct affected_type *af;
  struct stat statbuf;

  if (!ch || !GET_NAME(ch) || IS_PC(GET_PLYR(ch)))
    return 0;

  if (IS_NPC(ch) && !IS_SHOPKEEPER(ch))
    return 0;

  for (shop_nr = 0; shop_index[shop_nr].keeper != GET_RNUM(ch); shop_nr++) ;

  if ((sizeof(char) != 1) || (int_size != long_size))
  {
    logit(LOG_DEBUG,
          "sizeof(char) must be 1 and int_size must == long_size for player saves!\n");
    return 0;
  }
  buf = buff;
  ADD_BYTE(buf, (char) (short_size));
  ADD_BYTE(buf, (char) (int_size));
  ADD_BYTE(buf, (char) (long_size));

  affect_off = buf;
  ADD_INT(buf, (int) 0);
  item_off = buf;
  ADD_INT(buf, (int) 0);
  size_off = buf;
  ADD_INT(buf, (int) 0);
  ADD_INT(buf, mob_index[GET_RNUM(ch)].virtual_number);

  ADD_LONG(buf, time(0));       /* save time */
  ADD_INT(buf, world[ch->in_room].number);

  /* unequip everything and remove affects before saving */

  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      save_equip[i] = unequip_char(ch, i);
    else
      save_equip[i] = NULL;

  af = ch->affected;
  all_affects(ch, FALSE);       /* reset to unaffected state */
  buf += writePetStatus(buf, ch);
  ADD_INT(affect_off, (int) (buf - buff));
  updateShortAffects(ch);
  buf += writeAffects(buf, ch->affected);
  ADD_INT(item_off, (int) (buf - buff));
  buf += writeItems(buf, ch);
  ADD_INT(size_off, (int) (buf - buff));
  for (i = 0; i < MAX_WEAR; i++)
    if (save_equip[i])
      equip_char(ch, save_equip[i], i, 1);


  all_affects(ch, TRUE);        /* reapply affects (including equip) */

  if ((int) (buf - buff) > SAV_MAXSIZE)
  {
    logit(LOG_PLAYER, "Could not save %s, file too large (%d bytes)",
          GET_NAME(ch), (int) (buf - buff));
    return 0;
  }
  sprintf(Gbuf1, "%s/ShopKeepers/%d", SAVE_DIR, shop_nr);

  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");

  if (stat(Gbuf1, &statbuf) == 0)
  {
    if (rename(Gbuf1, Gbuf2) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Problem with shopkeeper save files directory!\n");
      logit(LOG_FILE, "   rename failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error backing up shop for %s!",
             GET_NAME(ch));
      return 0;
    }
    bak = 1;
  }
  else
  {
    if (errno != ENOENT)
    {
      int      tmp_errno;

      tmp_errno = errno;

      /*
       * NOTE: with the stat() function, only two errors are
       * possible: EBADF   filedes is bad. ENOENT  File does not
       * exist. Now, EBADF should only occur using fstat().
       * Therefore, if I fall into here, I have some SERIOUS
       * problems!
       */

      logit(LOG_FILE, "Problem with shop save files directory!\n");
      logit(LOG_FILE, "   stat failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error finding shopfile for %s!",
             GET_NAME(ch));
      return 0;
    }
    /*
     * in this case, no original pfile existed.  Probably a new
     * char... so don't panic.
     */
    bak = 0;
  }

  f = fopen(Gbuf1, "w");

  if (!f)
  {
    int      tmp_errno;

    tmp_errno = errno;
    logit(LOG_FILE, "Couldn't create shop save file!\n");
    logit(LOG_FILE, "   fopen failed, errno = %d\n", tmp_errno);
    wizlog(OVERLORD, "&+R&-LPANIC!&N  Error creating shopfile for %s!",
           GET_NAME(ch));
    bak -= 2;
  }
  else
  {
    if (fwrite(buff, 1, (unsigned) (buf - buff), f) != (buf - buff))
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Couldn't write to shop save file!\n");
      logit(LOG_FILE, "   fwrite failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error writing shopfile for %s!",
             GET_NAME(ch));
      fclose(f);
      bak -= 2;
    }
    else
      fclose(f);
  }

  switch (bak)
  {
  case 1:                      /* save worked, just get rid of the backup */
    if (unlink(Gbuf2) == -1)    /* not a critical error  */
      logit(LOG_FILE, "Couldn't delete backup of shopkeeper file.\n");

  case 0:                      /* save worked, no backup was made to begin with */
    break;

  case -1:                     /* save FAILED, but we have a backup */
    if (rename(Gbuf2, Gbuf1) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, " Unable to restore shop backup!  Argh!");
      logit(LOG_FILE, "    rename failed, errno = %d\n", tmp_errno);
      logit(LOG_EXIT, "  unable to restore shop backup");
			raise(SIGSEGV);
    }
    else
      wizlog(OVERLORD, "        Backup restored.");
    /*
     * restored or not, the save still failed, so return 0
     */
    return 0;

  case -2:                     /* save FAILED, and we have NO backup! */
    logit(LOG_FILE, " No shop restore file was made!");
    wizlog(OVERLORD, "        No backup file available");
    return 0;
  }

  return 1;
}

int deleteShopKeeper(int id)
{
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (id < 0)
  {
    logit(LOG_EXIT, "invalid shop id %d in deleteShopKeeper", id);
    raise(SIGSEGV);
  }
  sprintf(Gbuf1, "%s/ShopKeepers/%d", SAVE_DIR, id);
  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");
  unlink(Gbuf1);
  unlink(Gbuf2);

  return TRUE;
}

P_char restoreShopKeeper(int id)
{
  FILE    *f;
  P_char   ch;
  char     buff[SAV_MAXSIZE];
  char    *buf = buff;
  int      start, size, csize, affect_off, item_off, tmp, virt;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (!id)
  {
    return 0;
  }
  sprintf(Gbuf1, "%s/ShopKeepers/%d", SAVE_DIR, id);

  f = fopen(Gbuf1, "r");
  if (!f)
  {
    logit(LOG_DEBUG, "ShopKeeper %d savefile does not exist!", id);
    return 0;
  }
  size = fread(buf, 1, SAV_MAXSIZE, f);
  fclose(f);
  if (size < 4)
  {
    logit(LOG_FILE, "Warning: Save file less than 4 bytes.");
  }
  if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
      (GET_BYTE(buf) != long_size))
  {
    wizlog(OVERLORD, "Ouch. Bad file sizing for %d", id);
    return 0;
  }
  if (size < 5 * int_size + 5 * sizeof(char) + long_size)
  {
    logit(LOG_FILE, "Warning: Save file is only %d bytes.", size);
  }
  affect_off = GET_INTE(buf);
  item_off = GET_INTE(buf);
  csize = GET_INTE(buf);
  virt = GET_INTE(buf);
  if (size != csize)
  {
    wizlog(OVERLORD, "Warning: file size %d doesn't match csize %d.",
           size, csize);
  }
  ch = read_mobile(virt, VIRTUAL);
  if (!ch)
  {
    return 0;
  }
  GET_LONG(buf);
  GET_BIRTHPLACE(ch) = GET_INTE(buf);
  start = (int) (buf - buff);
  restorePetStatus(buf, ch);
  restoreAffects(buff + affect_off, ch);

  for (tmp = 0; tmp < MAX_WEAR; tmp++)
    save_equip[tmp] = NULL;
  restoreObjects(buff + item_off, ch, 1);
  for (tmp = 0; tmp < MAX_WEAR; tmp++)
    if (save_equip[tmp] != NULL)
      wear(ch, save_equip[tmp], restore_wear[tmp], 0);

  return ch;
}

void restore_shopkeepers(void)
{
  FILE    *flist;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];
  int      load_room;
  P_char   mob, keeper2;
  struct stat statbuf;

  sprintf(Gbuf1, "%s/ShopKeepers", SAVE_DIR);
  if (stat(Gbuf1, &statbuf) == -1)
  {
    perror("ShopKeepers dir");
    return;
  }
  sprintf(Gbuf2, "%s/shop_list", SAVE_DIR);
  if (stat(Gbuf2, &statbuf) == 0)
  {
    unlink(Gbuf2);
  }
  else if (errno != ENOENT)
  {
    perror("shop_list");
    return;
  }
  sprintf(Gbuf3, "/bin/ls -1 %s > %s", Gbuf1, Gbuf2);
  system(Gbuf3);
  flist = fopen(Gbuf2, "r");
  if (!flist)
    return;

  while (fscanf(flist, " %s \n", Gbuf2) != EOF)
  {
    if ((mob = restoreShopKeeper(atoi(Gbuf2))))
    {
      load_room = real_room(GET_BIRTHPLACE(mob));
      if (load_room != NOWHERE)
      {
        for (keeper2 = world[load_room].people; keeper2;
             keeper2 = keeper2->next_in_room)
          if (mob_index[GET_RNUM(keeper2)].virtual_number ==
              mob_index[GET_RNUM(mob)].virtual_number)
          {
            extract_char(keeper2);
          }
        char_to_room(mob, load_room, 0);
      }
      else
      {
        logit(LOG_DEBUG,
              "Could not load ShopKeeper #%s due to bad load_room!", Gbuf2);
      }
      deleteShopKeeper(atoi(Gbuf2));
    }
    else
    {
      logit(LOG_DEBUG, "Could not load ShopKeeper #%s!", Gbuf2);
      return;
    }
  }
}

void restore_allpets(void)
{
  FILE    *flist;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];
  int      load_room;
  P_char   mob;
  struct stat statbuf;

  sprintf(Gbuf1, "%s/Pets", SAVE_DIR);
  if (stat(Gbuf1, &statbuf) == -1)
  {
    perror("Pets dir");
    return;
  }
  sprintf(Gbuf2, "%s/pet_list", SAVE_DIR);
  if (stat(Gbuf2, &statbuf) == 0)
  {
    unlink(Gbuf2);
  }
  else if (errno != ENOENT)
  {
    perror("pet_list");
    return;
  }
  sprintf(Gbuf3, "/bin/ls -1 %s > %s", Gbuf1, Gbuf2);
  system(Gbuf3);
  flist = fopen(Gbuf2, "r");
  if (!flist)
    return;

  while (fscanf(flist, "%s\n", Gbuf2) != EOF)
  {
    if ((mob = restorePet(Gbuf2)))
    {

      load_room = real_room(GET_BIRTHPLACE(mob));
      if (load_room != NOWHERE)
      {
        char_to_room(mob, load_room, 0);
      }
      else
      {
        logit(LOG_DEBUG, "Could not load Pet #%s due to bad load_room!",
              Gbuf2);
      }
      deletePet(Gbuf2);
    }
  }
  return;
}


/* fonction to save and restore toen justice record (TASFALEN) */
int writeTownJustice(int town_nr)
{
  FILE    *f;
  char    *buf;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int      bak;
  static char buff[SAV_MAXSIZE * 2];
  struct stat statbuf;
  crm_rec *crec;
  int      count = 0;

  if ((sizeof(char) != 1) || (int_size != long_size))
  {
    logit(LOG_DEBUG,
          "sizeof(char) must be 1 and int_size must == long_size for player saves!\n");
    return 0;
  }
  buf = buff;

  crec = hometowns[town_nr - 1].crime_list;

  while (crec)
  {
    count++;
    crec = crec->next;
  }

  ADD_BYTE(buf, (char) SAV_WTNSVERS);
  ADD_BYTE(buf, (char) (short_size));
  ADD_BYTE(buf, (char) (int_size));
  ADD_BYTE(buf, (char) (long_size));

  crec = hometowns[town_nr - 1].crime_list;

  ADD_INT(buf, count);

  while (crec)
  {
    ADD_STRING(buf, crec->attacker);
    ADD_STRING(buf, crec->victim);
    ADD_INT(buf, crec->time);
    ADD_INT(buf, crec->crime);
    ADD_INT(buf, crec->room);
    ADD_INT(buf, crec->status);
    ADD_INT(buf, crec->money);
    crec = crec->next;
  }

  if ((int) (buf - buff) > SAV_MAXSIZE)
  {
    logit(LOG_PLAYER, "Could not save %d, file too large (%d bytes)",
          town_nr, (int) (buf - buff));
    return 0;
  }
  sprintf(Gbuf1, "%s/Justice/%d", SAVE_DIR, town_nr);

  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");

  if (stat(Gbuf1, &statbuf) == 0)
  {
    if (rename(Gbuf1, Gbuf2) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Problem with justice save files directory!\n");
      logit(LOG_FILE, "   rename failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error backing up justice for %d!",
             town_nr);
      return 0;
    }
    bak = 1;
  }
  else
  {
    if (errno != ENOENT)
    {
      int      tmp_errno;

      tmp_errno = errno;

      /*
       * NOTE: with the stat() function, only two errors are
       * possible: EBADF   filedes is bad. ENOENT  File does not
       * exist. Now, EBADF should only occur using fstat().
       * Therefore, if I fall into here, I have some SERIOUS
       * problems!
       */

      logit(LOG_FILE, "Problem with justice save files directory!\n");
      logit(LOG_FILE, "   stat failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error finding justice for %d!",
             town_nr);
      return 0;
    }
    /*
     * in this case, no original pfile existed.  Probably a new
     * char... so don't panic.
     */
    bak = 0;
  }

  f = fopen(Gbuf1, "w");

  if (!f)
  {
    int      tmp_errno;

    tmp_errno = errno;
    logit(LOG_FILE, "Couldn't create justice save file!\n");
    logit(LOG_FILE, "   fopen failed, errno = %d\n", tmp_errno);
    wizlog(OVERLORD, "&+R&-LPANIC!&N  Error creating justice for %d!", town_nr);
    bak -= 2;
  }
  else
  {
    if (fwrite(buff, 1, (unsigned) (buf - buff), f) != (buf - buff))
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Couldn't write to justice save file!\n");
      logit(LOG_FILE, "   fwrite failed, errno = %d\n", tmp_errno);
      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error writing justice for %d!",
             town_nr);
      fclose(f);
      bak -= 2;
    }
    else
      fclose(f);
  }

  switch (bak)
  {
  case 1:                      /* save worked, just get rid of the backup */
    if (unlink(Gbuf2) == -1)    /* not a critical error  */
      logit(LOG_FILE, "Couldn't delete backup of justice file.\n");

  case 0:                      /* save worked, no backup was made to begin with */
    break;

  case -1:                     /* save FAILED, but we have a backup */
    if (rename(Gbuf2, Gbuf1) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, " Unable to restore backup!  Argh!");
      logit(LOG_FILE, "    rename failed, errno = %d\n", tmp_errno);
      logit(LOG_EXIT, "unable to restore backup!");
			raise(SIGSEGV);
    }
    else
      wizlog(OVERLORD, "        Backup restored.");
    /*
     * restored or not, the save still failed, so return 0
     */
    return 0;

  case -2:                     /* save FAILED, and we have NO backup! */
    logit(LOG_FILE, " No restore file was made!");
    wizlog(OVERLORD, "        No backup file available");
    return 0;
  }

  return 1;
}

int deleteTownJustice(int id)
{
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (id < 0)
  {
    logit(LOG_EXIT, "invalid justice id %d in deleteTownJustice", id);
    raise(SIGSEGV);
  }
  sprintf(Gbuf1, "%s/Justice/%d", SAVE_DIR, id);
  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");
  unlink(Gbuf1);
  unlink(Gbuf2);

  return TRUE;
}


int restoreTownJustice(int town_nr)
{
  FILE    *f;
  char     buff[SAV_MAXSIZE];
  char    *buf = buff;
  int      start, size, count;
  char     Gbuf1[MAX_STRING_LENGTH];
  crm_rec *crec;
  char     temp;

  if (!town_nr)
  {
    return 0;
  }
  sprintf(Gbuf1, "%s/Justice/%d", SAVE_DIR, town_nr);

  f = fopen(Gbuf1, "r");
  if (!f)
  {
    logit(LOG_DEBUG, "Justice %d savefile does not exist!", town_nr);
    return 0;
  }
  size = fread(buf, 1, SAV_MAXSIZE, f);
  fclose(f);
  if (size < 4)
  {
    logit(LOG_FILE, "Warning: Save file less than 4 bytes.");
  }
  temp = GET_BYTE(buf);

  if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
      (GET_BYTE(buf) != long_size))
  {
    wizlog(OVERLORD, "Ouch. Bad file sizing for %d", town_nr);
    return 0;
  }
  if (size < 5 * int_size + 5 * sizeof(char) + long_size)
  {
    logit(LOG_FILE, "Warning: Save file is only %d bytes.", size);
  }
  count = GET_INTE(buf);
  for (; count > 0; count--)
  {
    if (!dead_crime_pool)
      dead_crime_pool = mm_create("CRIME",
                                  sizeof(crm_rec),
                                  offsetof(crm_rec, next), 1);

    crec = (crm_rec *) mm_get(dead_crime_pool);
    crec->attacker = GET_STRING(buf);
    crec->victim = GET_STRING(buf);
    crec->time = GET_LONG(buf);
    crec->crime = GET_INTE(buf);
    crec->room = GET_INTE(buf);
    crec->status = GET_INTE(buf);
    crec->money = GET_INTE(buf);
    crec->next = hometowns[town_nr - 1].crime_list;
    hometowns[town_nr - 1].crime_list = crec;
  }

  return (int) (buf - start);
}

void restore_town_justice(void)
{
  FILE    *flist;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];
  struct stat statbuf;

  sprintf(Gbuf1, "%s/Justice", SAVE_DIR);
  if (stat(Gbuf1, &statbuf) == -1)
  {
    logit(LOG_FILE, "restore_town_justice: dir '%s' missing.", Gbuf1);
    return;
  }
  sprintf(Gbuf2, "%s/justice_list", SAVE_DIR);
  if (stat(Gbuf2, &statbuf) == 0)
  {
    unlink(Gbuf2);
  }
  else if (errno != ENOENT)
  {
    logit(LOG_FILE, "restore_town_justice: File '%s' missing.", Gbuf2);
    return;
  }
  sprintf(Gbuf3, "/bin/ls -1 %s > %s", Gbuf1, Gbuf2);
  system(Gbuf3);
  flist = fopen(Gbuf2, "r");
  if (!flist)
  {
    logit(LOG_FILE, "restore_town_justice: Troubles opening justic file '%s'.", Gbuf2);
    return;
  }
  while (fscanf(flist, " %s \n", Gbuf2) != EOF)
  {
    restoreTownJustice(atoi(Gbuf2));
  }
  clean_town_justice();
}

/* end TASFALEN */

/* save item if a player goes to jail TASFALEN3 */

int writeJailItems(P_char ch)
{
  FILE    *f;
  P_obj    obj, obj2;
  char    *buf, *item_off, *size_off, *tmp;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int      i, bak;
  static char buff[SAV_MAXSIZE * 2];
  struct stat statbuf;

  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);

  if (!ch || !GET_NAME(ch))
    return 0;

  /*
   * this check assumes that the macros have not been fixed up for ** a
   * different architecture type.
   */

  if ((sizeof(char) != 1) || (int_size != long_size))
  {
    logit(LOG_DEBUG,
          "sizeof(char) must be 1 and int_size must == long_size for player saves!\n");
    return 0;
  }
  /*
   * in case char reenters game immediately; handle rent/etc correctly
   */

  buf = buff;
  ADD_BYTE(buf, (char) SAV_SAVEVERS);
  ADD_BYTE(buf, (char) (short_size));
  ADD_BYTE(buf, (char) (int_size));
  ADD_BYTE(buf, (char) (long_size));

  item_off = buf;
  ADD_INT(buf, (int) 0);
  size_off = buf;
  ADD_INT(buf, (int) 0);

  /*
   * unequip everything and remove affects before saving
   */

  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      save_equip[i] = unequip_char(ch, i);
    else
      save_equip[i] = NULL;

  all_affects(ch, FALSE);       /*
                                 * reset to unaffected state
                                 */

  ADD_INT(item_off, (int) (buf - buff));

  buf += writeItems(buf, ch);

  ADD_INT(size_off, (int) (buf - buff));

  /*
   * Nuke the equip and inven (it has already been saved)
   */

  for (i = 0; i < MAX_WEAR; i++)
    if (save_equip[i])
    {
      extract_obj(save_equip[i]);
      save_equip[i] = NULL;
    }
  for (obj = ch->carrying; obj; obj = obj2)
  {
    obj2 = obj->next_content;
    extract_obj(obj);
    obj = NULL;
  }

  all_affects(ch, TRUE);        /*
                                 * reapply affects (including equip)
                                 */

  if ((int) (buf - buff) > SAV_MAXSIZE)
  {
    logit(LOG_PLAYER, "Could not save %s, jail file too large (%d bytes)",
          GET_NAME(ch), (int) (buf - buff));
    return 0;
  }
  sprintf(Gbuf1, "%s/Justice/", SAVE_DIR);
  tmp = Gbuf1 + strlen(Gbuf1);
  strcat(Gbuf1, GET_NAME(ch));
  for (; *tmp; tmp++)
    *tmp = LOWER(*tmp);
  strcpy(Gbuf2, Gbuf1);
  strcat(Gbuf2, ".bak");

  if (stat(Gbuf1, &statbuf) == 0)
  {
    if (rename(Gbuf1, Gbuf2) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Problem with jail player save files directory!\n");
      logit(LOG_FILE, "   rename failed, errno = %d\n", tmp_errno);
      wizlog(AVATAR, "&+R&-LPANIC!&N  Error backing up jail file for %s!",
             GET_NAME(ch));
      return 0;
    }
    bak = 1;
  }
  else
  {
    if (errno != ENOENT)
    {
      int      tmp_errno;

      tmp_errno = errno;

      /*
       * NOTE: with the stat() function, only two errors are
       * possible: EBADF   filedes is bad. ENOENT  File does not
       * exist. Now, EBADF should only occur using fstat().
       * Therefore, if I fall into here, I have some SERIOUS
       * problems!
       */

      logit(LOG_FILE, "Problem with jail player save files directory!\n");
      logit(LOG_FILE, "   stat failed, errno = %d\n", tmp_errno);
      wizlog(AVATAR, "&+R&-LPANIC!&N  Error finding jail file for %s!",
             GET_NAME(ch));
      return 0;
    }
    /*
     * in this case, no original pfile existed.  Probably a new
     * char... so don't panic.
     */
    bak = 0;
  }

  f = fopen(Gbuf1, "w");

  /*
   * NOTE:  From this point on, if the save is not successful, then the
   * jail file will be corrupted.  While its nice to return an error, THERE
   * IS STILL A FUCKED UP JAIL PLAYER FILE!  Making the backup is a complete
   * fucking waste if you don't DO anything with it!  It will just get
   * overwritten the next time the character tries to save! Therefore,
   * I'm adding code that will rename the backup to the original if the
   * save wasn't successful.  At the same time, I'd like to officially
   * request that whoever had the wonderful idea of making a backup, but
   * not using it, be sac'ed repeatedly. (neb)
   */

  if (!f)
  {
    int      tmp_errno;

    tmp_errno = errno;
    logit(LOG_FILE, "Couldn't create jail player save file!\n");
    logit(LOG_FILE, "   fopen failed, errno = %d\n", tmp_errno);
    wizlog(AVATAR, "&+R&-LPANIC!&N  Error creating jail file for %s!",
           GET_NAME(ch));
    bak -= 2;
  }
  else
  {
    if (fwrite(buff, 1, (unsigned) (buf - buff), f) != (buf - buff))
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, "Couldn't write to jail player save file!\n");
      logit(LOG_FILE, "   fwrite failed, errno = %d\n", tmp_errno);
      wizlog(AVATAR, "&+R&-LPANIC!&N  Error writing jail file for %s!",
             GET_NAME(ch));
      fclose(f);
      bak -= 2;
    }
    else
      fclose(f);
  }

  switch (bak)
  {
  case 1:                      /*
                                 * save worked, just get rid of the backup
                                 */
    if (unlink(Gbuf2) == -1)    /*
                                 * not a critical error
                                 */
      logit(LOG_FILE, "Couldn't delete backup of jail player file.\n");

  case 0:                      /*
                                 * save worked, no backup was made to
                                 * begin with
                                 */
    break;

  case -1:                     /*
                                 * save FAILED, but we have a backup
                                 */
    if (rename(Gbuf2, Gbuf1) == -1)
    {
      int      tmp_errno;

      tmp_errno = errno;
      logit(LOG_FILE, " Unable to restore backup!  Argh!");
      logit(LOG_FILE, "    rename failed, errno = %d\n", tmp_errno);
      wizlog(AVATAR,
             "&+R&-LPANIC!&N  Error restoring backup jail file for %s!",
             GET_NAME(ch));
      logit(LOG_EXIT, "unable to restore backup");
			raise(SIGSEGV);
    }
    else
      wizlog(AVATAR, "        Backup restored.");
    /*
     * restored or not, the save still failed, so return 0
     */
    return 0;

  case -2:                     /*
                                 * save FAILED, and we have NO backup!
                                 */
    logit(LOG_FILE, " No restore file was made!");
    wizlog(AVATAR, "        No backup file available");
    return 0;
  }

  return 1;
}

int deleteJailItems(P_char ch)
{
  FILE    *f;
  char     buff[SAV_MAXSIZE];
  char    *buf = buff;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if (!ch)
    return -2;

  strcpy(buff, GET_NAME(ch));
  for (; *buf; buf++)
    *buf = LOWER(*buf);
  buf = buff;
  sprintf(Gbuf1, "%s/Justice/%s", SAVE_DIR, buff);

  f = fopen(Gbuf1, "r");
  if (!f)
    return -2;

  fclose(f);

  if (unlink(Gbuf1) == -1)
    logit(LOG_FILE, "Couldn't delete jail player file of %s after release.\n",
          GET_NAME(ch));

  return TRUE;

}

int restoreJailItems(P_char ch)
{
  FILE    *f;
  char     buff[SAV_MAXSIZE];
  char    *buf = buff;
  int      size, csize, item_off, tmp;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     b_savevers;

  if (!ch)
    return -2;

  strcpy(buff, GET_NAME(ch));
  for (; *buf; buf++)
    *buf = LOWER(*buf);
  buf = buff;
  sprintf(Gbuf1, "%s/Justice/%s", SAVE_DIR, buff);

  f = fopen(Gbuf1, "r");
  if (!f)
    return -2;

  buf = buff;
  size = fread(buf, 1, SAV_MAXSIZE, f);
  fclose(f);
  if (size < 4)
  {
    fprintf(stderr, "Problem restoring jail player file of: %s\n",
            GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring jail player file of %s.",
          GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }
  b_savevers = GET_BYTE(buf);

  if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
      (GET_BYTE(buf) != long_size))
  {
    logit(LOG_FILE, "Save file in different machine format.");
    fprintf(stderr, "Problem restoring jail player file of: %s\n",
            GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring jail player file of %s.",
          GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }
  if (size < (5 * int_size + 5 * sizeof(char) + long_size))
  {
    logit(LOG_FILE, "Save file is too small %d.", size);
    fprintf(stderr, "Problem restoring jail player file of: %s\n",
            GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring jail player file of %s.",
          GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }
  item_off = GET_INTE(buf);
  csize = GET_INTE(buf);
  if (size != csize)
  {
    logit(LOG_FILE, "Save file size %d not match size read %d.", size, csize);
    fprintf(stderr, "Problem restoring jail player file of: %s\n",
            GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring jail player file of %s.",
          GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }
  /*
   * This shit axed, no need for rent on a real mud. --MIAX
   */

  for (tmp = 0; tmp < MAX_WEAR; tmp++)
    save_equip[tmp] = NULL;

  if (!restoreObjects(buff + item_off, ch, 1))
  {
    fprintf(stderr, "Problem restoring jail player file of: %s\n",
            GET_NAME(ch));
    logit(LOG_FILE, "Problem restoring jail player file of %s.",
          GET_NAME(ch));
    strcpy(Gbuf2, Gbuf1);
    strcat(Gbuf2, ".bad");
    rename(Gbuf1, Gbuf2);
    return -2;
  }

  for (tmp = 0; tmp < MAX_WEAR; tmp++)
    if (save_equip[tmp] != NULL)
      wear(ch, save_equip[tmp], restore_wear[tmp], 0);

  if (unlink(Gbuf1) == -1)
    logit(LOG_FILE, "Couldn't delete jail player file of %s after release.\n",
          GET_NAME(ch));

  return TRUE;
}

// old guildhalls (deprecated) - Torgal 1/2010
///* house construction Q */
//int writeConstructionQ()
//{
//  FILE    *f;
//  char    *buf;
//  static char buff[SAV_MAXSIZE * 2];
//  P_house_upgrade current_job;
//  char     fname[MAX_STRING_LENGTH];
//  int      count = 0;
//
//  buf = buff;
//  current_job = house_upgrade_list;
//
//  while (current_job)
//  {
//    count++;
//    current_job = current_job->next;
//  }
//  current_job = house_upgrade_list;
//
//  ADD_INT(buf, count);
//  while (current_job)
//  {
//    ADD_INT(buf, current_job->vnum);
//    ADD_LONG(buf, current_job->time);
//    ADD_INT(buf, current_job->type);
//    ADD_INT(buf, current_job->location);
//    ADD_INT(buf, current_job->guild);
//    ADD_INT(buf, current_job->exit_dir);
//    ADD_INT(buf, current_job->door);
//    ADD_STRING(buf, current_job->door_keyword);
//    current_job = current_job->next;
//  }
//  if ((int) (buf - buff) > SAV_MAXSIZE)
//  {
//    logit(LOG_HOUSE, "Could not save contruction Q");
//    return 0;
//  }
//  sprintf(fname, "%s/House/HouseConstructionQ", SAVE_DIR);
//  if (!(f = fopen(fname, "w")))
//    return 0;
//  fwrite(buff, 1, (unsigned) (buf - buff), f);
//  fclose(f);
//  return 1;
//}
//
//int loadConstructionQ()
//{
//  FILE    *f;
//  char     buff[SAV_MAXSIZE * 2];
//  char    *buf = buff;
//  struct house_upgrade_rec *current_job;
//  char     fname[MAX_STRING_LENGTH];
//  int      size, count;
//
//  house_upgrade_list = NULL;
//  current_job = 0;
//  sprintf(fname, "%s/House/HouseConstructionQ", SAVE_DIR);
//  if (!(f = fopen(fname, "r")))
//    return 0;
//  size = fread(buf, 1, SAV_MAXSIZE, f);
//  fclose(f);
//  count = GET_INTE(buf);
//  for (size = 0; size < count; size++)
//  {
//    if (!dead_construction_pool)
//      dead_construction_pool =
//        mm_create("CONSTRUCTION", sizeof(struct house_upgrade_rec),
//                  offsetof(struct house_upgrade_rec, next), 1);
//    current_job = (struct house_upgrade_rec *) mm_get(dead_construction_pool);
//    current_job->vnum = GET_INTE(buf);
//    current_job->time = GET_LONG(buf);
//    current_job->type = GET_INTE(buf);
//    current_job->location = GET_INTE(buf);
//    current_job->guild = GET_INTE(buf);
//    current_job->exit_dir = GET_INTE(buf);
//    current_job->door = GET_INTE(buf);
//    current_job->door_keyword = GET_STRING(buf);
//    if (current_job->type != HCONTROL_DESC_ROOM)
//    {
//      current_job->next = house_upgrade_list;
//      house_upgrade_list = current_job;
//    }
//  };
//  return 1;
//}
//
//int moveHouse(P_house house, int new_vnum)
//{
//  char Gbuf1[256], Gbuf2[256];
//  struct stat statbuf;
//  int      tmp_errno;
//
//  sprintf(Gbuf1, "%s/House/HouseRoom/house.%d", SAVE_DIR, house->vnum);
//  sprintf(Gbuf2, "%s/House/HouseRoom/house.%d", SAVE_DIR, new_vnum);
//    
//  if (stat(Gbuf1, &statbuf) == 0)
//  {
//    if (rename(Gbuf1, Gbuf2) == -1)
//    {
//      
//      tmp_errno = errno;
//      logit(LOG_HOUSE, "Couldn't rename house!\n");
//      logit(LOG_HOUSE, "   rename failed, errno = %d\n", tmp_errno);
//      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error renaming house file %d!",
//             house->vnum);
//      return 0;
//    }
//  }
//  else
//  {
//    tmp_errno = errno;
//    logit(LOG_HOUSE, "Problem with house save files directory, couldn't find house in rename!\n");
//    logit(LOG_HOUSE, "   stat failed, errno = %d\n", tmp_errno);
//    wizlog(OVERLORD, "&+R&-LPANIC!&N  Error renaming house for %d!",
//           house->vnum);
//    return 0;
//  }
//
//  logit(LOG_HOUSE, "House %d moved to %d", house->vnum, new_vnum);  
//  return 1;
//}
//
///*************************************************************/
///* fonction to save and restore house */
//int writeHouse(P_house house)
//{
//  FILE    *f;
//  char    *buf, housefile[MAX_STRING_LENGTH];
//  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
//  int      bak;
//  static char buff[SAV_MAXSIZE * 2];
//  struct stat statbuf;
//  int      count = 0, tmp, count2 = 0;
//
//  if ((sizeof(char) != 1) || (int_size != long_size))
//  {
//    logit(LOG_HOUSE,
//          "sizeof(char) must be 1 and int_size must == long_size for player saves!\n");
//    return 0;
//  }
//  buf = buff;
//
//  ADD_BYTE(buf, (char) SAV_HOUSEVERS);
//  ADD_BYTE(buf, (char) (short_size));
//  ADD_BYTE(buf, (char) (int_size));
//  ADD_BYTE(buf, (char) (long_size));
//
//  ADD_INT(buf, house->vnum);
//  ADD_INT(buf, house->built_on);
//  ADD_SHORT(buf, house->mode);
//  ADD_SHORT(buf, house->type);
//  ADD_BYTE(buf, house->construction);
//  ADD_STRING(buf, house->owner);
//
//  ADD_INT(buf, house->num_of_guests);
//
//  for (count = 0; count < house->num_of_guests; count++)
//    ADD_STRING(buf, house->guests[count]);
//
//  ADD_INT(buf, MAX_HOUSE_ROOMS);
//  for (count = 0; count < MAX_HOUSE_ROOMS; count++)
//  {
//    ADD_INT(buf, house->room_vnums[count]);
//    if (house->room_vnums[count] != -1)
//      write_guild_room(house->room_vnums[count], house->owner_guild);
//  }
//  ADD_INT(buf, house->owner_guild);
//  ADD_INT(buf, house->last_payment);
//  ADD_SHORT(buf, house->size);
//  ADD_LONG(buf, house->upgrades);
//  ADD_INT(buf, house->exit_num);
//  ADD_STRING(buf, house->entrance_keyword);
//  ADD_INT(buf, house->mouth_vnum);
//  ADD_INT(buf, house->teleporter1_room);
//  ADD_INT(buf, house->teleporter1_dest);
//  ADD_INT(buf, house->teleporter2_room);
//  ADD_INT(buf, house->teleporter2_dest);
//  ADD_INT(buf, house->inn_vnum);
//  ADD_INT(buf, house->fountain_vnum);
//  ADD_INT(buf, house->heal_vnum);
//  ADD_INT(buf, house->board_vnum);
//  ADD_INT(buf, house->wizard_golems);
//  ADD_INT(buf, house->warrior_golems);
//  ADD_INT(buf, house->cleric_golems);
//  ADD_INT(buf, house->guard_golems);
//  ADD_INT(buf, house->shop_vnum);
//  ADD_INT(buf, house->holy_fount_vnum);
//  ADD_INT(buf, house->unholy_fount_vnum);
//  ADD_INT(buf, house->secret_entrance);
//  if (house->type == HCONTROL_CASTLE)
//  {
//    ADD_SHORT(buf, (short) MAX_CONTROLLED_LAND);
//    for (tmp = 0; tmp < MAX_CONTROLLED_LAND; tmp++)
//      ADD_INT(buf, house->controlled_land[tmp]);
//  }
//  if ((int) (buf - buff) > SAV_MAXSIZE)
//  {
//    logit(LOG_HOUSE, "Could not save %d, file too large (%d bytes)",
//          house->vnum, (int) (buf - buff));
//    return 0;
//  }
//  sprintf(housefile, "house.%d", house->vnum);
//  sprintf(Gbuf1, "%s/House/HouseRoom/%s", SAVE_DIR, housefile);
//
//  strcpy(Gbuf2, Gbuf1);
//  strcat(Gbuf2, ".bak");
//
//  if (stat(Gbuf1, &statbuf) == 0)
//  {
//    if (rename(Gbuf1, Gbuf2) == -1)
//    {
//      int      tmp_errno;
//
//      tmp_errno = errno;
//      logit(LOG_HOUSE, "Problem with house save files directory!\n");
//      logit(LOG_HOUSE, "   rename failed, errno = %d\n", tmp_errno);
//      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error backing up house for %d!",
//             house->vnum);
//      return 0;
//    }
//    bak = 1;
//  }
//  else
//  {
//    if (errno != ENOENT)
//    {
//      int      tmp_errno;
//
//      tmp_errno = errno;
//      logit(LOG_HOUSE, "Problem with house save files directory!\n");
//      logit(LOG_HOUSE, "   stat failed, errno = %d\n", tmp_errno);
//      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error finding house for %d!",
//             house->vnum);
//      return 0;
//    }
//    bak = 0;
//  }
//
//  f = fopen(Gbuf1, "w");
//
//  if (!f)
//  {
//    int      tmp_errno;
//
//    tmp_errno = errno;
//    logit(LOG_HOUSE, "Couldn't create house save file!\n");
//    logit(LOG_HOUSE, "   fopen failed, errno = %d\n", tmp_errno);
//    wizlog(OVERLORD, "&+R&-LPANIC!&N  Error creating house for %d!",
//           house->vnum);
//    bak -= 2;
//  }
//  else
//  {
//    if (fwrite(buff, 1, (unsigned) (buf - buff), f) != (buf - buff))
//    {
//      int      tmp_errno;
//
//      tmp_errno = errno;
//      logit(LOG_HOUSE, "Couldn't write to house save file!\n");
//      logit(LOG_HOUSE, "   fwrite failed, errno = %d\n", tmp_errno);
//      wizlog(OVERLORD, "&+R&-LPANIC!&N  Error writing house for %d!",
//             house->vnum);
//      fclose(f);
//      bak -= 2;
//    }
//    else
//      fclose(f);
//  }
//
//  switch (bak)
//  {
//  case 1:                      /* save worked, just get rid of the backup */
//    if (unlink(Gbuf2) == -1)    /* not a critical error  */
//      logit(LOG_HOUSE, "Couldn't delete backup of house file.\n");
//
//  case 0:                      /* save worked, no backup was made to begin with */
//    break;
//
//  case -1:                     /* save FAILED, but we have a backup */
//    if (rename(Gbuf2, Gbuf1) == -1)
//    {
//      int      tmp_errno;
//
//      tmp_errno = errno;
//      logit(LOG_HOUSE, " Unable to restore backup!  Argh!");
//      logit(LOG_HOUSE, "    rename failed, errno = %d\n", tmp_errno);
//      logit(LOG_EXIT, "unable to restore backup");
//			raise(SIGSEGV);
//    }
//    else
//      wizlog(OVERLORD, "        Backup restored.");
//    /*
//     * restored or not, the save still failed, so return 0
//     */
//    return 0;
//
//  case -2:                     /* save FAILED, and we have NO backup! */
//    logit(LOG_HOUSE, " No restore file was made!");
//    wizlog(OVERLORD, "        No backup file available");
//    return 0;
//  }
//  return 1;
//}
//
//int restoreHouse(char *file_name)
//{
//  FILE    *f;
//  char     buff[SAV_MAXSIZE];
//  char    *buf = buff;
//  int      start, size, count, tmp, s, dummy_int, version;
//  char     Gbuf1[MAX_STRING_LENGTH];
//  P_house  house;
//  P_obj    obj = NULL;
//
//  if (!file_name)
//  {
//    return 0;
//  }
//  sprintf(Gbuf1, "%s/House/HouseRoom/%s", SAVE_DIR, file_name);
//
//  f = fopen(Gbuf1, "r");
//  if (!f)
//  {
//    logit(LOG_HOUSE, "House %s savefile does not exist!", file_name);
//    return 0;
//  }
//  size = fread(buf, 1, SAV_MAXSIZE, f);
//  fclose(f);
//  if (size < 4)
//  {
//    logit(LOG_HOUSE, "Warning: Save file less than 4 bytes.");
//  }
//  version = GET_BYTE(buf);
//
//  if ((GET_BYTE(buf) != short_size) || (GET_BYTE(buf) != int_size) ||
//      (GET_BYTE(buf) != long_size))
//  {
//    wizlog(OVERLORD, "Ouch. Bad file sizing for %s", file_name);
//    return 0;
//  }
//  if (size < 5 * int_size + 5 * sizeof(char) + long_size)
//  {
//    logit(LOG_HOUSE, "Warning: Save file is only %d bytes.", size);
//  }
//  if (!dead_house_pool)
//    dead_house_pool = mm_create("HOUSE", sizeof(struct house_control_rec),
//                                offsetof(struct house_control_rec, next), 1);
//  house = (struct house_control_rec *) mm_get(dead_house_pool);
//  house->vnum = GET_INTE(buf);
//  house->built_on = GET_INTE(buf);
//  house->mode = GET_SHORT(buf);
//  house->type = GET_SHORT(buf);
//  if (version > 4)
//    house->construction = GET_BYTE(buf);
//  else
//    house->construction = 0;
//  house->owner = GET_STRING(buf);
//  house->num_of_guests = GET_INTE(buf);
//
//  for (count = 0; count < house->num_of_guests; count++)
//    house->guests[count] = GET_STRING(buf);
//
//  house->num_of_rooms = GET_INTE(buf);
//  for (count = 0; count < house->num_of_rooms; count++)
//  {
//    house->room_vnums[count] = GET_INTE(buf);
//    if (house->room_vnums[count] == -1)
//      continue;
//    if (house->room_vnums[count] < START_HOUSE_VNUM ||
//        house->room_vnums[count] > END_HOUSE_VNUM)
//    {
//      logit(LOG_HOUSE, "Troubles with room number for house %d.",
//            house->vnum);
//      logit(LOG_HOUSE, "TEST, %d.", house->num_of_rooms);
//      logit(LOG_HOUSE, "TEST, %d.", house->room_vnums[count]);
//      house->room_vnums[count] = -1;
//    }
//  }
//  house->owner_guild = GET_INTE(buf);
//  house->last_payment = GET_INTE(buf);
///*  house->last_payment = 0;                 temp to clear them all */
//  house->size = GET_SHORT(buf);
//  house->upgrades = GET_LONG(buf);
//  house->exit_num = GET_INTE(buf);
//  house->entrance_keyword = GET_STRING(buf);
//  house->mouth_vnum = GET_INTE(buf);
//  house->teleporter1_room = GET_INTE(buf);
//  house->teleporter1_dest = GET_INTE(buf);
//  house->teleporter2_room = GET_INTE(buf);
//  house->teleporter2_dest = GET_INTE(buf);
//  house->inn_vnum = GET_INTE(buf);
//  house->fountain_vnum = GET_INTE(buf);
//  house->heal_vnum = GET_INTE(buf);
//  house->board_vnum = GET_INTE(buf);
//  house->wizard_golems = GET_INTE(buf);
//  house->warrior_golems = GET_INTE(buf);
//  house->cleric_golems = GET_INTE(buf);
//  house->guard_golems = GET_INTE(buf);
//  house->shop_vnum = GET_INTE(buf);
//  house->holy_fount_vnum = GET_INTE(buf);
//  house->unholy_fount_vnum = GET_INTE(buf);
//  if (version > 4)
//    house->secret_entrance = GET_INTE(buf);
//  else
//    house->secret_entrance = 0;
//  if (house->type == HCONTROL_CASTLE)
//  {
//    s = GET_SHORT(buf);         /* number of controlled lands */
//    for (tmp = 0; tmp < MAX(s, MAX_CONTROLLED_LAND); tmp++)
//    {
//      if ((tmp < s) && (tmp < MAX_CONTROLLED_LAND))
//      {
//        house->controlled_land[tmp] = GET_INTE(buf);
//        if ((obj = read_object(HOUSE_FLAG, VIRTUAL))) ;
//        obj_to_room(obj, real_room0(house->controlled_land[tmp]));
//      }
//      else
//      {
//        if (tmp < s)
//        {
//          /* MAX_CONTROLLED_LAND is smaller than saved version, so read and
//           * discard the extra bytes */
//          dummy_int = GET_INTE(buf);
//        }
//        else
//        {
//          /* number has grown, make sure new ones are nulled */
//          house->controlled_land[tmp] = -1;
//        }
//      }
//    }
//  }
//  house->sack_list = 0;
//  house->next = first_house;
//  first_house = house;
//
//  /* fix incomplete houses */
//  if (house->upgrades == -1)
//    house->upgrades = 0;
//  if (house->owner_guild == -1)
//    house->owner_guild = 0;
//
//  /* sets the flags, builds the walls, etc */
//  construct_castle(house);
//
//  return (int) (buf - start);
//}
//
//void restore_houses(void)
//{
//  DIR     *house_dir;
//  struct dirent *house_entry;
//  char     house_dir_name[MAX_STRING_LENGTH];
//
//  sprintf(house_dir_name, "%s/House/HouseRoom", SAVE_DIR);
//  house_dir = opendir(house_dir_name);
//  if (!house_dir)
//  {
//    logit(LOG_HOUSE, "House dir");
//    return;
//  }
//
//  while (house_entry = readdir(house_dir))
//  {
//    if (strstr(house_entry->d_name, "house."))
//      restoreHouse(house_entry->d_name);
//  }
//
//  closedir(house_dir);
//}

/* Ship registeration Support Funcs */

int ship_registered(int vnum)
{
  struct ship_reg_node *x;

  x = ship_reg_db;
  while (x && (x->vnum != vnum))
    x = x->next;
  if (x)
    return TRUE;
  return FALSE;
};

int register_ship(int vnum)
{
  struct ship_reg_node *x;

  CREATE(x, ship_reg_node, 1, MEM_TAG_SHIPREG);

  x->vnum = vnum;
  x->next = ship_reg_db;
  ship_reg_db = x;
  return TRUE;
};
#endif //_PFILE_

void moveToBackup( char *name )
{
  char lowername[20];
  char filename[512];
  char command[1024];

  strcpy( lowername, name );
  lowername[0] = LOWER(name[0]);

  sprintf( filename, "%s/%c/%s", SAVE_DIR, lowername[0], lowername );
  sprintf( command, "mv -f %s %s.bak", filename, filename );
  system( command );
}
