/*
 * ***************************************************************************
 *   File: actinf.c                                           Part of Duris
 *   Usage: Informative commands.
 *   Copyright  1990, 1991 - see 'license.doc' for complete information.
 *   Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fnmatch.h>

#include <iostream>
#include <sstream>
#include <string>
using namespace std;

#include "comm.h"
#include "db.h"
#include "events.h"
#include "objmisc.h"
#include "interp.h"
#include "utils.h"
#include "justice.h"
#include "prototypes.h"
#include "utility.h"
#include "spells.h"
#include "structs.h"
#include "weather.h"
#include "helpfile.h"	
#include "sql.h"
#include "paladins.h"
#include "guard.h"
#include "epic.h"
#include "wikihelp.h"
#include "map.h"
#include "specializations.h"
#include "grapple.h"
#include "nexus_stones.h"
#include "ships.h"
#include "ctf.h"
#include "epic_bonus.h"


/* * external variables */

extern bool debug_event_list;
extern float spell_pulse_data[LAST_RACE + 1];
extern char *target_locs[];
extern char *set_master_text[];
extern int mini_mode;
extern FILE *help_fl;
extern FILE *info_fl;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern struct class_names class_names_table[];
extern const char *color_liquid[];
extern const char *command[];
extern const char *connected_types[];
extern const char *craftsmanship_names[];
extern const char *dirs[];
extern const char *event_names[];
extern const char *fullness[];
extern const char *language_names[];
extern struct material_data materials[];
extern const char *month_name[];
extern const char *player_bits[];
extern const char *player_law_flags[];
extern const char *player_prompt[];
extern flagDef weapon_types[];
extern const char *weekdays[];
extern const char *where[];
extern const int rev_dir[];
extern const long boot_time;
extern const struct stat_data stat_factor[];
extern const char *size_types[];
extern const char *item_size_types[];
extern int avail_descs;
extern int help_array[27][2];
extern int info_array[27][2];
extern int max_descs;
extern int max_users_playing;
extern int number_of_quests;
extern int number_of_shops;
extern int pulse;
extern int str_weightcarried, str_todam, str_tohit, dex_defense;
extern int top_of_helpt;
extern int top_of_infot;
extern int top_of_mobt;
extern int top_of_objt;
extern const int top_of_world;
extern int top_of_zone_table;
extern int used_descs;
extern struct agi_app_type agi_app[];
extern struct command_info cmd_info[];
extern struct dex_app_type dex_app[];
extern struct info_index_element *info_index;
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern uint event_counter[];
extern char *specdata[][MAX_SPEC];
extern long sentbytes;
extern long recivedbytes;
extern const struct race_names race_names_table[];
extern Skill skills[];
extern const int new_exp_table[];
extern const mcname multiclass_names[];
extern void displayShutdownMsg(P_char);
extern void map_look_room(P_char ch, int room, int show_map_regardless);
extern const char *get_function_name(void *);
extern void show_world_events(P_char ch, const char* arg);
extern struct quest_data quest_index[];
extern struct epic_bonus_data ebd[];
extern long ne_event_counter;
extern int map_normal_modifier;
extern int map_ultra_modifier;
extern int map_dayblind_modifier;

void display_map(P_char ch, int n, int show_map_regardless);

extern HelpFilesCPPClass help_index;

extern struct TimedShutdownData shutdownData;

int astral_clock_setMapModifiers(void);
void unmulti(P_char ch, P_obj obj);
void where_stat(P_char ch, char *argument);
void list_ships_to_char( P_char ch, int room_no );

const char *stat_names1[18] = {
  "bad",                        /* 0 */
  "bad",                        /* 1-9 */
  "bad",                        /* * 10-15 */
  "bad",                        /* * 16-21 */
  "bad",                        /* * 22-27 */
  "fair",                       /* * 28-33 */
  "fair",                       /* * 34-39 */
  "fair",                       /* * 40-45 */
  "fair",                       /* * 46-50 */
  "fair",                       /* * 51-55 */
  "fair",                       /* * 56-61 */
  "fair",                       /* * 62-67 */
  "fair",                       /* * 68-73 */
  "good",                       /* * 74-79 */
  "good",                       /* * 80-85 */
  "good",                       /* * 86-91 */
  "good",                       /* * 92-100 */
  "good"                        /* * 100+ */
};

const char *stat_names2[18] = {
  "lame",                       /* * 0 */
  "lame",                       /* * 1-9 */
  "poor",                       /* * 10-15 */
  "poor",                       /* * 16-21 */
  "below average",              /* * 22-27 */
  "below average",              /* * 28-33 */
  "average",                    /* * 34-39 */
  "average",                    /* * 40-45 */
  "average",                    /* * 46-50 */
  "average",                    /* * 51-55 */
  "average",                    /* * 56-61 */
  "average",                    /* * 62-67 */
  "above average",              /* * 68-73 */
  "above average",              /* * 74-79 */
  "good",                       /* * 80-85 */
  "very good",                  /* * 86-91 */
  "excellent",                  /* * 92-100 */
  "quite excellent",            /* * 100+ */
};

const char *stat_names3[13] = {
  "atrociously bad",            /* * 0-55 */
  "very bad",                   /* * 56-65 */
  "bad",                        /* * 66-75 */
  "poor",                       /* * 76-85 */
  "below average",              /* * 86-95 */
  "average",                    /* * 96-105 */
  "above average",              /* * 106-115 */
  "good",                       /* * 116-125 */
  "very good",                  /* * 126-135 */
  "formidable",                 /* * 136-145 */
  "excellent",                  /* * 146-155 */
  "awesome",                    /* * 156-165 */
  "incredible"                  /* * 166-280 */
};

const char *stat_outofrange[2] = {
  "sucks",
  "abnormally good"
};

const char *align_names[9] = {
  "&+Lextremely evil",
  "&+Levil",
  "&+Levil, leaning towards neutrality",
  "neutral, leaning towards evil",
  "neutral",
  "neutral, leaning towards good",
  "&+Wgood, leaning towards neutrality",
  "&+Wgood",
  "&+Wextremely good"
};

const char *ac_names[19] = {
  "At least you aren't naked",  /* * 90 to 99 */
  "You do not feel safe from a dull butter knife",      /* * 80 to 89 */
  "Your armor is pathetic",     /* * 70 to 79 */
  "Your armor is awful",        /* * 60 to 69 */
  "Your armor is bad",          /* * 50 to 59 */
  "Your armor is slightly better than bad",     /* * 40 to 49 */
  "Your armor is far worse than average",       /* * 30 to 39 */
  "Your armor is below average",        /* * 20 to 29 */
  "Your armor is slightly below average",       /* * 10 to 19 */
  "Your armor is average",      /* * -9 to 9 */
  "Your armor is slightly above average",       /* * -19 to -10 */
  "Your armor is above average",        /* * -29 to -20 */
  "Your armor is well above average",   /* * -39 to -30 */
  "Your armor is good",         /* * -49 to -40 */
  "Your armor is really good",  /* * -59 to -50 */
  "You are ready for a serious battle", /* * -69 to -60 */
  "Your armor is awesome",      /* * -79 to -70 */
  "Your armor really rocks",    /* * -89 to -80 */
  "Your armor REALLY rocks"     /* * -99 to -90 */
};

const char *ac_outofrange[2] = {
  "You have no armor class, you are buck naked! *brrrr*",
  "If you had any more armor class, it'd be a bug"
/*
  "Your armor is eqivalent to that of an M1A1 Abrahams - Main battle tank"
*/
};

const char *hitdam_roll_names[9] = {
  "lame",
  "bad",
  "lower than average",
  "average",
  "better than average",
  "fairly good",
  "good",
  "excellent",
  "awesome"
};

const char *hitdam_roll_outofrange[2] = {
  "really lame",
  "quite awesome"
};

const char *save_names[9] = {
  "lame",
  "bad",                        /* * 3 to 7 */
  "lower than average",
  "average",                    /* * 2 to -2 */
  "better than average",
  "fairly good",
  "good",                       /* * -3 to -7 */
  "excellent",
  "awesome"
};

const char *save_outofrange[2] = {
  "really lame",
  "quite awesome"
};

const char *load_names[15] = {
  "What load?",
  "Unburdened",
  "No Sweat",
  "Not a problem",
  "Paltry",
  "Very Light",
  "Light",
  "Moderate",
  "Heavy",
  "Very Heavy",
  "Extremely Heavy",
  "Backbreaking",
  "Crushing",
  "Atlas would be Proud",
  "Immobilizing"
};

string itoa(int num)
{
  stringstream converter;
  converter << num;
  return converter.str();
}

/*
 * some defines for 'who' code
 */

#define ALL_CHAR 0
#define SPEC     1
#define NONSPEC  2

#define MAX_PLAYERS     512

#define NUM_FIELDS 41
int      cmd_nr[NUM_FIELDS];

const char *race_to_string(P_char ch)
{
  int      val;

  if (ch == NULL)
    return (NULL);

  val = ch->player.race;
  if ((val <= 0) || (val > LAST_RACE))
    return (race_names_table[0].ansi);
  else
    return (race_names_table[val].ansi);
}

const char *class_to_string(P_char ch, char *buf)
{
  int      val;
  char     buf2[2048];

  if (ch == NULL)
    return (NULL);

  val = flag2idx(ch->player.m_class);
  if ((val <= 0) || (val > CLASS_COUNT))
    strcpy(buf2, (class_names_table[0].ansi));
  else
    strcpy(buf2, (class_names_table[val].ansi));

  sprintf(buf, "%s&n /  %s        ", buf2,
          IS_SPECIALIZED(ch) ? GET_SPEC_NAME(ch->player.m_class, ch->player.spec-1) : "None");
  return buf;
}

const char *stat_to_string1(int val)
{
  int      si = STAT_INDEX(val);

  if (si < 0)
    return (stat_names1[0]);
  else if (si > 17)
    return (stat_names1[17]);
  else
    return (stat_names1[si]);
}

const char *stat_to_string2(int val)
{
  int      si = STAT_INDEX(val);

  if (si < 0)
    return (stat_names2[0]);
  else if (si > 17)
    return (stat_names2[17]);
  else
    return (stat_names2[si]);
}

const char *stat_to_string3(int val)
{
  int     si = STAT_INDEX2(val);

  if (si < 0)
    return (stat_names3[12]);
  else if (si > 12)
    return (stat_names3[0]);
  else
    return (stat_names3[si]);
}

const char *stat_to_string_damage_pulse(float val)
{
  int    si = STAT_INDEX_DAMAGE_PULSE(val);
  
  if (si < 0)
    return (stat_names3[0]);
  else if (si > 12)
    return (stat_names3[12]);
  else
    return (stat_names3[si]);
}

const char *stat_to_string_spell_pulse(float val)
{
  int    si = STAT_INDEX_SPELL_PULSE(val);
  
  if (si < 0)
    return (stat_names3[0]);
  else if (si > 12)
    return (stat_names3[12]);
  else
    return (stat_names3[si]);
}

const char *align_to_string(int val)
{
  if (val < -783)
    return (align_names[0]);
  else if ((val > -784) && (val < -566))
    return (align_names[1]);
  else if ((val > -567) && (val < -349))
    return (align_names[2]);
  else if ((val > -350) && (val < -117))
    return (align_names[3]);
  else if ((val > -118) && (val < 117))
    return (align_names[4]);
  else if ((val > 116) && (val < 350))
    return (align_names[5]);
  else if ((val > 349) && (val < 567))
    return (align_names[6]);
  else if ((val > 566) && (val < 783))
    return (align_names[7]);
  else if (val > 782)
    return (align_names[8]);
  else
  {
    logit(LOG_STATUS, "align_to_string: ATT BUG illegal value %d", val);
    return ("Att BUG, please report this immediately!");
  }
}

const char *ac_to_string(int val)
{
  int      temp, temp2;

  if (val >= 100)
    return (ac_outofrange[0]);
  else if (val <= -100)
    return (ac_outofrange[1]);

  temp = -val / 10;
  temp2 = temp + 9;

  if ((temp2 < 0) || (temp2 > 18))
  {
    logit(LOG_STATUS, "ATT bug found, ac_to_string, temp = %d", temp);
    return ("BUGGED, report this now!");
  }
  return (ac_names[temp2]);
}

const char *hitdam_roll_to_string(int val)
{
  if (val > 32)
    return (hitdam_roll_outofrange[1]);
  else if (val < -15)
    return (hitdam_roll_outofrange[0]);

  if (val > 27)
    return (hitdam_roll_names[8]);
  else if (val > 20)
    return (hitdam_roll_names[7]);
  else if (val > 12)
    return (hitdam_roll_names[6]);
  else if (val > 8)
    return (hitdam_roll_names[5]);
  else if (val > 3)
    return (hitdam_roll_names[4]);
  else if (val > -2)
    return (hitdam_roll_names[3]);
  else if (val > -6)
    return (hitdam_roll_names[2]);
  else if (val > -10)
    return (hitdam_roll_names[1]);
  else
    return (hitdam_roll_names[0]);
}

string save_to_string(P_char ch, int save_type)
{
  int save = find_save(ch, save_type);

  save += ch->specials.apply_saving_throw[save_type] * 5;

  if( IS_TRUSTED(ch) || GET_LEVEL(ch) >= 25 )
  {
    return itoa(save);
  }
  
  if (save > 100)
    return string("vulnerable");
  if (save > 90)
    return string("pathetic");
  if (save > 85)
    return string("awful");
  if (save > 80)
    return string("bad");
  if (save > 75)
    return string("slightly better than bad");
  if (save > 65)
    return string("far worse than average");
  if (save > 60)
    return string("below average");
  if (save > 55)
    return string("slightly below average");
  if (save > 45)
    return string("average");
  if (save > 40)
    return string("slightly above average");
  if (save > 35)
    return string("above average");
  if (save > 30)
    return string("well above average");
  if (save > 25)
    return string("good");
  if (save > 20)
    return string("really good");
  if (save > 10)
    return string("excellent");
  if (save > 0)
    return string("superior");
  if (save > -10)
    return string("immune");
  if (save > -30)
    return string("arcane");

  return string("ultimate");
}

/*
 * heavy bell curve, middle one has 33%, middle five have 79% of total
 */

const char *load_to_string(P_char ch)
{
  int      percent = CAN_CARRY_W(ch);

  if (percent <= 0)
    percent = 1;

  percent = (int) ((IS_CARRYING_W(ch) * 100) / percent);

  if (percent <= 0)
    return (load_names[0]);
  else if (percent <= 1)
    return (load_names[1]);
  else if (percent <= 3)
    return (load_names[2]);
  else if (percent <= 6)
    return (load_names[3]);
  else if (percent <= 10)
    return (load_names[4]);
  else if (percent <= 15)
    return (load_names[5]);
  else if (percent <= 25)
    return (load_names[6]);
  else if (percent <= 40)
    return (load_names[7]);
  else if (percent <= 55)
    return (load_names[8]);
  else if (percent <= 70)
    return (load_names[9]);
  else if (percent <= 80)
    return (load_names[10]);
  else if (percent <= 89)
    return (load_names[11]);
  else if (percent <= 94)
    return (load_names[12]);
  else if (percent < 100)
    return (load_names[13]);

  return (load_names[14]);

}

/*
 * Procedures related to 'look'
 */

void argument_split_2(char *argument, char *first_arg, char *second_arg)
{
  int      look_at, found, begin;

  found = begin = 0;

  if (strlen(argument) >= MAX_INPUT_LENGTH)
  {
    logit(LOG_SYS, "too long arg in argument_split_2.");
    *(first_arg) = '\0';
    *(second_arg) = '\0';
    return;
  }
  /*
   * Find first non blank
   */
  for (; *(argument + begin) == ' '; begin++) ;

  /*
   * Find length of first word
   */
  for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
    /*
     * Make all letters lower case, AND copy them to first_arg
     */
    *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
  *(first_arg + look_at) = '\0';
  begin += look_at;

  /*
   * Find first non blank
   */
  for (; *(argument + begin) == ' '; begin++) ;

  /*
   * Find length of second word
   */
  for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
    /*
     * Make all letters lower case, AND copy them to second_arg
     */
    *(second_arg + look_at) = LOWER(*(argument + begin + look_at));
  *(second_arg + look_at) = '\0';
  begin += look_at;
}

P_obj get_object_in_equip_vis(P_char ch, char *arg, int *j)
{
  for ((*j) = 0; (*j) < MAX_WEAR; (*j)++)
    if (ch->equipment[(*j)])
      if (CAN_SEE_OBJ(ch, ch->equipment[(*j)]))
        if (isname(arg, ch->equipment[(*j)]->name))
          return (ch->equipment[(*j)]);

  return (0);
}

P_obj get_object_in_equip(P_char ch, char *arg, int *j)
{
  for ((*j) = 0; (*j) < MAX_WEAR; (*j)++)
    if (ch->equipment[(*j)])
      if (isname(arg, ch->equipment[(*j)]->name))
        return (ch->equipment[(*j)]);

  return (0);
}

char    *find_ex_description(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword)
        && !isname("_id_name_", i->keyword)
        && !isname("_id_short_", i->keyword)
        && !isname("_id_desc_", i->keyword))
      return (i->description);

  return (0);
}

int ageCorpse(P_char ch, P_obj obj, char *s)
{
  int      maxErrorPercent, maxError, error;
  int      realAge, guessAge;
  char     buf[MAX_STRING_LENGTH];
  struct obj_affect *af = NULL;

  if (GET_CHAR_SKILL(ch, SKILL_AGE_CORPSE) == 0)
    return FALSE;

  notch_skill( ch, SKILL_AGE_CORPSE, 10 );

  /* Change this to something that finds out how long
   * the corpse has left
   */
  af = get_obj_affect(obj, TAG_OBJ_DECAY);

  if (af == NULL)
  {
    strcat(s, "\nThis corpse seems as though it will never decay.");
    return FALSE;
  }

  realAge = obj_affect_time(obj, af) / PULSES_IN_TICK;

  maxErrorPercent = 100 - GET_CHAR_SKILL(ch, SKILL_AGE_CORPSE);
  maxError = (realAge * maxErrorPercent) / 100;
  error = number(0, maxError);
  if (number(0, 1))
    error = -error;
  guessAge = realAge + error;

  sprintf(buf, "\nThis corpse will remain fresh for a further %d hours.",
          guessAge);
  strcat(s, buf);

  return TRUE;

}

char *show_obj_to_char(P_obj object, P_char ch, int mode, int print)
{
  bool     found;
  static char buf[MAX_STRING_LENGTH];
  P_obj    wpn;

  if (IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_VNUM))
    sprintf(buf, "[&+B%5d&N] ", (object->R_num >= 0 ? obj_index[object->R_num].virtual_number : -1));
  else
    buf[0] = 0;

  if ((mode == 0) && object->description)
  {
    strcat(buf, object->description);
  }
  else if (object->short_description && (mode == BOUNDED(1, mode, 4)))
  {
    strcat(buf, object->short_description);
  }
  else if (mode == 5)
  {
    if (object->type == ITEM_NOTE)
    {
      if (object->action_description)
      {
        strcat(buf, "There is something written upon it:\n\n");
        strcat(buf, object->action_description);
        page_string(ch->desc, buf, 1);
      }
      else
        act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
      return NULL;
    }
    else if (object->type == ITEM_CORPSE)
    {
      strcat(buf, "Ye roll up yer sleeves, and examine the dead...");
      ageCorpse(ch, object, buf);
    }
    else if ((object->type == ITEM_DRINKCON))
    {
      strcat(buf, "It looks like a drink container.");
    }
  }
  if (mode != 3)
  {
    found = FALSE;

    if (IS_OBJ_STAT(object, ITEM_INVISIBLE))
    {
      strcat(buf, " (&+Linvis&n)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_NOSHOW) && IS_TRUSTED(ch))
    {
      strcat(buf, " (&+LNOSHOW&n)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_SECRET))
    {
      strcat(buf, " (&+Lsecret&n)");
      found = TRUE;
    }
    if( (IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION) || has_innate( ch, INNATE_DET_SUBVERSION)) && is_illusion_obj(object))
    {
      strcat(buf, " (&+MIllusion&n)");
      found = TRUE;
    }

    if (IS_OBJ_STAT(object, ITEM_BURIED))
    {
      strcat(buf, " (&+Lburied&n)");
      found = TRUE;
    }
    if (IS_OBJ_STAT2(object, ITEM2_MAGIC) && (IS_TRUSTED(ch) || IS_AFFECTED2(ch, AFF2_DETECT_MAGIC)))
    {
      strcat(buf, " (&+bmagic&n)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_GLOW))
    {
      strcat(buf, " (&+Mglowing&n)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_HUM))
    {
      strcat(buf, " (&+rhumming&n)");
      found = TRUE;
    }
    if (get_obj_affect(object, SKILL_ENCHANT))
    {
      strcat(buf, " (&+mEnchanted&n)");
      found = TRUE;
    }
    if( IS_OBJ_STAT(object, ITEM_LIT) || ((object->type == ITEM_LIGHT) && (object->value[2] == -1)) )
    {
      strcat(buf, " (&+Willuminating&n)");
      found = TRUE;
    }
    /*if (OBJ_WORN(object) &&
        affected_by_spell(object->loc.wearing, SPELL_HEALING_BLADE))
    {
      if (object->type == ITEM_WEAPON)
      {
        if (IS_SWORD(object))
        {
          strcat(buf, " &+W(&+Bblue aura&+W)&N");
        }
      }
    }
    */
    if (OBJ_WORN(object) && affected_by_spell(object->loc.wearing, SPELL_ILIENZES_FLAME_SWORD))
    {
      if (object->type == ITEM_WEAPON && (IS_SWORD(object) || IS_AXE(object)))
      {
        strcat(buf, " &+L(&+rf&+Rl&+Ya&+Wm&+Yi&+Rn&+rg&+L)&N");
      }
    }

    if (OBJ_WORN(object) && affected_by_spell(object->loc.wearing, SPELL_THRYMS_ICERAZOR))
    {
      if (object->type == ITEM_WEAPON && IS_BLUDGEON(object) )
      {
        strcat(buf, " &+L(&+Cch&+Bill&+Cing&+L)&N"); // XXX zion
      }
    }

    if (OBJ_WORN(object) && affected_by_spell(object->loc.wearing, SPELL_LLIENDILS_STORMSHOCK))
    {
      if (object->type == ITEM_WEAPON && !IS_BLUDGEON(object) && !IS_AXE(object) )
      {
        strcat(buf, " &+L(&+Bele&+Wctri&+Bfied&+L)&N"); // XXX zion
      }
    }

    if (OBJ_WORN(object) && affected_by_spell(object->loc.wearing, SPELL_HOLY_SWORD))
    {
      if (object->type == ITEM_WEAPON && IS_SWORD(object) )
      {
        strcat(buf, " &+L(&+Wh&+wol&+Wy&+L)&N");
      }
    }

    // Now Imms can see timers on other's eq.
    if( IS_ARTIFACT(object) && IS_TRUSTED(ch) )
    {
      artifact_timer_sql( GET_OBJ_VNUM(object), buf + strlen(buf) );
    }

    /*
    if (IS_TRUSTED(ch)) {
      sprintf(buf, "%s (&+m%s&n)", buf, item_size_types[GET_OBJ_SIZE(object)]);
      found = TRUE;
    }
    */
  }
  strcat(buf, item_condition(object));
  if (ch->specials.z_cord > object->z_cord)
      strcat(buf, " &N(below you)");
  if (ch->specials.z_cord < object->z_cord)
      strcat(buf, " &N(above you)");

  strcat(buf, "\n");
  if (print)
    page_string(ch->desc, buf, 1);

  return buf;
}

void list_obj_to_char(P_obj list, P_char ch, int mode, int show)
{
  P_obj    i;
  bool     found;
  char     buf[MAX_STRING_LENGTH], *s, buf2[20];
  int      count;


  buf[0] = 0;
  count = 0;
  found = FALSE;
  for (i = list; i; i = i->next_content)
  {
    if (i->hitched_to)
      continue;                 /* strung to the horse */
    /* thieves notice things are 'odd' */
    if (IS_OBJ_STAT(i, ITEM_BURIED) && GET_CLASS(ch, CLASS_THIEF))
      send_to_char("The ground appears recently turned...\n", ch);
    if (CAN_SEE_OBJ(ch, i))
    {
      s = show_obj_to_char(i, ch, mode, 0);
      if (!str_cmp(s, buf))
      {
        count++;
      }
      else
      {
        if (count)
        {
          sprintf(buf2, "[%d] ", count + 1);
          send_to_char(buf2, ch);
          count = 0;
        }
        if (buf[0])
          send_to_char(buf, ch);
        if (s)
          strcpy(buf, s);
        else
          logit(LOG_DEBUG, "Fubar strcpy in list_obj_to_char.");
      }
      found = TRUE;
    }
  }
  if (found)
  {
    if (count)
    {
      sprintf(buf2, "[%d] ", count + 1);
      send_to_char(buf2, ch);
    }
    if (buf[0])
      send_to_char(buf, ch);
  }
  if ((!found) && (show))
    send_to_char("Nothing.\n", ch);
}

#define SVS(string) (act(string, FALSE, ch, 0, tar_char, TO_CHAR))

void show_visual_status(P_char ch, P_char tar_char)
{
  int      percent;
  char     buf[256], buf2[256];

  if (GET_MAX_HIT(tar_char) > 0)
    percent = (100 * GET_HIT(tar_char)) / GET_MAX_HIT(tar_char);
  else
    percent = -1;

  //if (IS_NPC(tar_char) && !IS_TRUSTED(ch)) {
  //  sprintf(buf, "$N %%s");
  //}
  //else
  if (!racewar(ch, tar_char) || IS_ILLITHID(ch)/* || IS_TRUSTED(ch)*/)
  {
    sprintf(buf, "$N appears to be %s and %%s",
            GET_RACE(tar_char) ? race_names_table[(int) GET_RACE1(tar_char)].
            ansi : "&=LRBogus race!&n");
  }
  else
    sprintf(buf, "This %s %%s",
            race_names_table[(int) GET_RACE1(tar_char)].ansi);

  if (percent >= 100)	
    sprintf(buf2, buf, "is in &+Gexcellent&n condition.");
  else if (percent >= 90)
    sprintf(buf2, buf, "has a &+yfew scratches&n.");
  else if (percent >= 75)
  {
    if(IS_CONSTRUCT(tar_char))
      sprintf(buf2, buf, "has &+Ysome gouges and cracks in its surface&n.");
    else
      sprintf(buf2, buf, "has &+Ysome small wounds and bruises&n.");
  }
  else if (percent >= 50)
  {
    if(IS_CONSTRUCT(tar_char))
      sprintf(buf2, buf, "has &+Mdeep gouges and serious cracks in its surface&n.");
    else
    sprintf(buf2, buf, "has &+Mquite a few wounds&n.");
  }
  else if (percent >= 30)
  {
    if(IS_CONSTRUCT(tar_char))
      sprintf(buf2, buf, "has &+mbegun to crumble from from an ongoing assault&n.");
    else
    sprintf(buf2, buf, "has &+msome big nasty wounds and scratches&n.");
  }
  else if (percent >= 15)
  {
    if(IS_CONSTRUCT(tar_char))
      sprintf(buf2, buf, "has &+Rlarge holes and cracks in its surface&n.");
    else
      sprintf(buf2, buf, "looks &+Rpretty hurt&n.");
  }
  else if (percent >= 0)
  {
    if(IS_CONSTRUCT(tar_char))
      sprintf(buf2, buf, "is &+ralmost completely destroyed&n.");
    else
      sprintf(buf2, buf, "is in &+rawful condition&n.");
  }
  else
    sprintf(buf2, buf, "is &+rbleeding awfully from big wounds&n.");
  SVS(buf2);

  sprintf(buf, "&+c$E's %s in size.", size_types[GET_ALT_SIZE(tar_char)]);
  SVS(buf);

  if (IS_CASTING(tar_char))
    SVS("&+m$E appears to be casting.");

  if (IS_AFFECTED(tar_char, AFF_CAMPING))
    SVS("&+y$E appears to be setting up camp.");

  if (tar_char->group)
  {
    if (!IS_BACKRANKED(tar_char))
      SVS("&+R$E is in the front rank.");
    else
      SVS("&+B$E is in the back rank.");
  }

  if( (IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION) || has_innate( ch, INNATE_DET_SUBVERSION)) && is_illusion_char(tar_char))
    SVS("&+m$E seems to waver slightly.");

  if (IS_AFFECTED(tar_char, AFF_BARKSKIN))
    SVS("&+y$S skin has a barklike texture..");
  if (IS_AFFECTED5(tar_char, AFF5_THORNSKIN))
    SVS("&+y$S skin has the texture of bark with thorns.");

#ifdef STANCES_ALLOWED
  if(IS_AFFECTED5(tar_char, AFF5_STANCE_DEFENSIVE))
    SVS("&+y$E is in defensive stance\n");
  else if (IS_AFFECTED5(tar_char, AFF5_STANCE_OFFENSIVE))
    SVS("&+r$E is in offensive stance\n");
  else
    SVS("&+w$E is in no stance at all.");
#endif

  if (IS_AFFECTED(tar_char, AFF_HASTE))
    SVS("&+y$E seems to be moving much faster than normal..");

  if (affected_by_spell(tar_char, SPELL_STONE_SKIN))
    SVS("&+L$S body seems to be made of stone!");

  if (affected_by_spell(tar_char, SPELL_SHADOW_SHIELD))
    SVS("&+y$S body appears to be covered with "
        "&+Ls&+Ww&+Li&+Wr&+Ll&+Wi&+Ln&+Wg &+Lshadows!&n");

  if (affected_by_spell(tar_char, SPELL_BIOFEEDBACK))
    SVS("&+G$S body is surrounded in a a green mist!");

  if (affected_by_spell(tar_char, SPELL_IRONWOOD))
    SVS("&+y$S skin has the texture of &+Liron&+y!");

  if (affected_by_spell(tar_char, SPELL_FAERIE_FIRE))
    SVS("&+r$E is outlined with dancing purplish flames!");

  if (IS_AFFECTED4
      (tar_char, AFF4_STORNOGS_SPHERES | AFF4_STORNOGS_GREATER_SPHERES))
    SVS("&+R$E is surrounded by orbiting spheres!");

  if (IS_AFFECTED2(tar_char, AFF2_VAMPIRIC_TOUCH))
    SVS("&+L$S hands glow &+Rblood red&+L!&n");

  if (IS_AFFECTED5(tar_char, AFF5_VINES))
    SVS("&+G$E is encased in a shield of vines!");

  if (IS_AFFECTED2(tar_char, AFF2_SOULSHIELD))
  {
    if (IS_EVIL(tar_char))
      SVS("&+r$E is glowing with malevolent power!&N");
    else
      SVS("&+W$E is glowing with holiness!&n");
  }
  
  if (IS_AFFECTED4(tar_char, AFF4_NEG_SHIELD))
    SVS("&+L$E is surrounded by a &+rputrid &+Laura!&n");

  if (IS_AFFECTED3(tar_char, AFF3_COLDSHIELD))
    SVS("&+B$E is encased in killing ice!");

  if (IS_AFFECTED3(tar_char, AFF3_LIGHTNINGSHIELD))
    SVS("&+Y$E is surrounded by a maelstrom of energy!");

  if (IS_AFFECTED2(tar_char, AFF2_FIRESHIELD))
    SVS("&+R$E is surrounded by burning flames!");

  if (IS_AFFECTED(tar_char, AFF_MINOR_GLOBE) ||
      IS_AFFECTED2(tar_char, AFF2_GLOBE))
    SVS("&+r$E's encased in a shimmering globe!");

   if (IS_AFFECTED5(tar_char, AFF5_DECAYING_FLESH))
    SVS("&nTheir &+rflesh &nappears to be &+gdec&+Gay&+ging &nrapidly!");

  if (IS_AFFECTED3(tar_char, AFF3_SPIRIT_WARD) ||
      IS_AFFECTED3(tar_char, AFF3_GR_SPIRIT_WARD))
    SVS("&+W$E's surrounded by a diffuse globe of light!");

  if (IS_AFFECTED(tar_char, AFF_WATERBREATH))
    SVS("&+r$E has little gills in $S neck!");

  if ((IS_AFFECTED2(tar_char, AFF2_MAJOR_PARALYSIS) ||
      IS_AFFECTED2(tar_char, AFF2_MINOR_PARALYSIS) ) &&
      GET_RACE(tar_char) != RACE_CONSTRUCT)
    SVS("&+Y$E seems to be paralyzed!");

  if(IS_AFFECTED3(tar_char, AFF3_PALADIN_AURA))
    send_paladin_auras(ch, tar_char);

  if (IS_AFFECTED5(tar_char, AFF5_WET) ||
      IS_AFFECTED5(tar_char, AFF5_WET))
    SVS("&+b$E is all wet!");

  if (affected_by_spell(tar_char, SKILL_DREADNAUGHT))
    SVS("&+y$E is in a &+Ldefensive&+y position!");

  if(IS_AFFECTED4(tar_char, AFF4_CARRY_PLAGUE) ||
    affected_by_spell(tar_char, SPELL_PLAGUE))
    SVS("&+y$E is covered by &+Rin&+rfec&+Rti&+rou&+Rs &+yopen s&+ror&+yes!&n");

  if(IS_AFFECTED4(tar_char, AFF4_DEFLECT))
  {
    SVS("&+c$E is surrounded by a &+wfl&+Wux&+wing &+Bblue&n &+cenergy field.&n");
  }
  
  if(IS_AFFECTED4(tar_char, AFF4_PHANTASMAL_FORM))
  {
    SVS("&+L$E does not &+ywholly&n &+Loccupy this plane of existence.&n");
  }
  
  if (affected_by_spell(tar_char, SPELL_WITHER))
  {
  	SVS("&+w$E &+wlooks pale and &+Lwithered&n&+w.&n");
  }
  
  if (affected_by_spell(tar_char, SPELL_RAY_OF_ENFEEBLEMENT))
  {
  	SVS("&+w$E &+wlooks pale and &+Lweakened&n&+w.&n");
  }
  
  if (affected_by_spell(tar_char, SPELL_CHILL_TOUCH))
  {
  	SVS("&+w$E &+wlooks &+Lweakened&+w from the &+Wchilly aura&n&+w surrounding $M&+w.&n");
  }
  
  if (affected_by_spell(tar_char, SPELL_ICE_ARMOR))
  {
        SVS("&+C$E appears to be encased in a barrier of &+Wflowing &+Bice&+C!&n");
  }

  if (affected_by_spell(tar_char, SPELL_NEG_ARMOR))
  {
	    SVS("&+w$E appears to be encased in a &+Lsw&+wir&+Llin&+wg &+Lbarrier &+wof &+Lgravitational energy!&n");
  }

  if( IS_NPC(tar_char) && (GET_CLASS(ch, CLASS_RANGER)
    || (IS_PC_PET(tar_char) && GET_CLASS(ch, CLASS_SUMMONER) && GET_MASTER(tar_char) == ch)) )
  {
    get_class_string(tar_char, buf2);
    sprintf(buf, "Through your advanced training, you glean they are a level &+Y%d &N%s&n.", GET_LEVEL(tar_char), buf2);
    SVS(buf);
  }

  if (IS_TRUSTED(ch))
  {
    get_class_string(tar_char, buf2);
    sprintf(buf, "&+YLevel: &N%d &+YClass(es): &N%s &+YHitpoints: %d/%d", GET_LEVEL(tar_char), buf2, GET_HIT(tar_char), GET_MAX_HIT(tar_char));
    SVS(buf);
  }
  send_to_char("\n", ch);
}

#undef SVS

void create_in_room_status(P_char ch, P_char i, char buffer[])
{
  if (IS_AFFECTED(i, AFF_HIDE) && IS_TRUSTED(ch))
    strcat(buffer, "(&+Lhiding&n)");

  if (IS_AFFECTED(i, AFF_WRAITHFORM))
    strcat(buffer, "(&+rwraith&n)");

  if (IS_AFFECTED4(ch, AFF4_SENSE_HOLINESS) && IS_HOLY(i))
    strcat(buffer, "(&+Wholy&n)");

  if( IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION) && is_illusion_char(i) )
    strcat(buffer, "(&+mIllusion&n)");

  if( (IS_DISGUISE(i) || is_illusion_char(i)) && has_innate(ch, INNATE_DET_SUBVERSION) )
  {
    if( is_illusion_char(i) )
      strcat(buffer, "(&+mIllusion&n)");
    else if (IS_DISGUISE_SHAPE(i))
      strcat(buffer, "(&+mShapechanged&n)");
    else
      strcat(buffer, "(&+mdisguised&n)");
  }

  if (IS_AFFECTED5(ch, AFF5_BLOOD_SCENT) && GET_HIT(i) < 0.4 * GET_MAX_HIT(i))
      strcat(buffer, " (&+Rbleeding&n) ");

  if ((IS_AFFECTED2(i, AFF2_MINOR_PARALYSIS) ||
      IS_AFFECTED2(i, AFF2_MAJOR_PARALYSIS)) &&
      GET_RACE(i) != RACE_CONSTRUCT)
    strcat(buffer, " (&+Yparalyzed&n) ");

  if (IS_AFFECTED5(i, AFF5_IMPRISON))
    strcat(buffer, " (&+cim&+Cpr&+cis&+Con&+ced&n) ");

  if(IS_PC_PET(i) && (!isname("image", GET_NAME(i))))
   strcat(buffer, " &+w(&+Yminion&+w) ");

  if (IS_CASTING(i))
    strcat(buffer, " (&+mcasting&n) ");

  if (IS_AFFECTED(i, AFF_CAMPING))
    strcat(buffer, " (&+ycamping&n) ");

  if (IS_AFFECTED2(ch, AFF2_DETECT_EVIL) && IS_EVIL(i))
    strcat(buffer, "(&+rRed Aura&n)");

  if (IS_AFFECTED2(ch, AFF2_DETECT_GOOD) && IS_GOOD(i))
    strcat(buffer, "(&+YGold Aura&n)");
}

/* mode 0 - shows char info you see when looking in room
   mode 1 - shows equipment
   mode 2 - shows inventory

   ch is char looking, i is target */
void show_char_to_char(P_char i, P_char ch, int mode)
{
  char     buffer[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int      j, found, percent, lt_lvl;
  P_obj    tmp_obj, wpn;
  int      wear_order[] =
    { 41, 24, 40,  6, 19, 21, 22, 20, 39, 3,  4, 5, 35, 12, 27, 42, 37, 23, 13, 28,
      29, 30, 10, 31, 11, 14, 15, 33, 34, 9, 32, 1,  2, 16, 17, 25, 26, 18,  7,
      36,  8, 38, -1
    };  // Also defined in get_equipment_list
  int      diff, race;
  struct affected_type *af;
  int      quester_id;
  bool     visobj, higher, lower;

  *buffer = '\0';
  *buf2 = '\0';

  // Undetectable mobs - SKB 22 Nov 1995
  if( IS_NPC(i) && !IS_TRUSTED(ch) && !i->player.long_descr )
  {
    return;
  }

  // This shouldnt occur, but it does -Granor
  if( i->only.pc == NULL )
  {
    debug( "show_char_to_char: Looker: '%s' %d, Target: '%s' has no only data!",
      J_NAME(ch), GET_ID(ch), i->player.name );
    return;
  }

  // Show char info shown in list of people in room
  if( mode == 0 )
  {
    // Mounts are strung to riders description, so don't show them.
    if( get_linking_char(i, LNK_RIDING) && !IS_TRUSTED(ch) )
    {
      return;
    }

    diff = i->specials.z_cord - ch->specials.z_cord;
    higher = (diff > 0);
    lower = (diff < 0);

    // Don't show yourself in room list
    if( i == ch )
    {
      return;
    }

    // Check for enhanced hide and return if undetectable (Not currently implemented, but ok).
    if( IS_AFFECTED5(i, AFF5_ENH_HIDE) && !IS_TRUSTED(ch) )
    {
      return;
    }

    if( !CAN_SEE_Z_CORD(ch, i) && !IS_TRUSTED(i) && IS_AFFECTED3(i, AFF3_NON_DETECTION)
      && !affected_by_spell(ch, SPELL_TRUE_SEEING) && !GET_CLASS(ch, CLASS_PSIONICIST | CLASS_DRUID) )
    {
      if( IS_AFFECTED(ch, AFF_SENSE_LIFE) )
      {
        if( higher )
          send_to_char("&+LYou sense a hidden lifeform above you.\n", ch);
        else if( lower )
          send_to_char("&+LYou sense a hidden lifeform below you.\n", ch);
        else
          send_to_char("&+LYou sense a hidden lifeform nearby.\n", ch);
      }
      return;
    }

    if( (IS_AFFECTED(i, AFF_INVISIBLE) || IS_AFFECTED2(i, AFF2_MINOR_INVIS)
      || IS_AFFECTED3(i, AFF3_ECTOPLASMIC_FORM)) && !IS_AFFECTED(ch, AFF_WRAITHFORM) )
    {
      strcat(buffer, "*");
    }

    if( (IS_NPC(i) && IS_MORPH(i)) && affected_by_spell(ch, SPELL_TRUE_SEEING) )
    {
      strcat(buffer, "%");
    }

    if( get_linking_char(i, LNK_RIDING) )
    {
      strcat(buffer, "&+L(R)&n");
    }

    // If it's a quest mob
    if( IS_NPC(i) && (IS_NPC(ch) || PLR2_FLAGGED(ch, PLR2_SHOW_QUEST))
      && (GET_LEVEL(ch) <= get_property("look.showquestgiver.maxlvl", 30)) )
    {
      if( mob_index[GET_RNUM(i)].func.mob
        && !strcmp(get_function_name((void*)mob_index[GET_RNUM(i)].func.mob), "world_quest") )
      {
        strcat(buffer, "&+Y(Q)&n");
      }
      else if( mob_index[GET_RNUM(i)].qst_func )
      {
        if (has_quest(i))
        {
          strcat(buffer, "&+B(Q)&n");
        }
      }
    }

    if( (ch->only.pc->quest_active) && IS_NPC(i) && (IS_NPC(ch) || PLR2_FLAGGED(ch, PLR2_SHOW_QUEST))
      && (GET_LEVEL(ch) <= get_property("look.showquestgiver.maxlvl", 30))
      && (GET_VNUM(i) == ch->only.pc->quest_mob_vnum) )
    {
      strcat(buffer, "&+R(Q)&n");
    }

    if( IS_PC(i) || (i->specials.position != i->only.npc->default_pos) || IS_FIGHTING(i) || IS_RIDING(i)
      || i->lobj->Visible_Type() || (GET_RNUM(i) == real_mobile(IMAGE_REFLECTION_VNUM)) || IS_DESTROYING(ch) )
    {
      /* A player char or a mobile w/o long descr, or not in default pos. */
      if( IS_PC(i) || (GET_RNUM(i) == real_mobile(IMAGE_REFLECTION_VNUM)) )
      {
        if( i->in_room < 0 )
        {
          logit(LOG_DEBUG, "show_char_to_char: %s->in_room < 0.", GET_NAME(i));
          return;
        }

        // if char is more than 2 above or below and char doesn't have hawkvision, they just
        // see 'someone'
        if( !IS_TRUSTED(ch) && !IS_AFFECTED4(ch, AFF4_HAWKVISION) && (abs(diff) > 2) )
        {
          strcat(buffer, "Someone");

          if( GET_TITLE(i) )
          {
            strcat(buffer, " ");
            strcat(buffer, GET_TITLE(i));
          }
        }
        else if( IS_DISGUISE(i) )
        {
          race = GET_DISGUISE_RACE(i);
          if( racewar(ch, i) && !IS_ILLITHID(ch) && !IS_TRUSTED(i) && IS_DISGUISE_PC(i) )
          {
            sprintf(buffer + strlen(buffer), "%s %s (%s)",
              ( race == RACE_ILLITHID || race == RACE_PILLITHID || race == RACE_ORC
              || race == RACE_OROG || race == RACE_OGRE || race == RACE_AGATHINON ) ? "An" : "A",
              race_names_table[race].ansi, size_types[GET_ALT_SIZE(i)] );
          }
          else
          {
            if( IS_DISGUISE_NPC(i) && GET_STAT(i) == STAT_NORMAL && GET_POS(i) == POS_STANDING )
            {
              sprintf(buffer + strlen(buffer), "%s",
                GET_DISGUISE_LONG(i) ? (char *) striplinefeed(GET_DISGUISE_LONG(i)) : "&=LRBogus char!&n");
            }
            else
            {
              sprintf(buffer + strlen(buffer), "%s",
                (IS_DISGUISE_NPC(i) && i->disguise.name) ? i->disguise.name :
                (IS_DISGUISE_PC(i) && GET_DISGUISE_NAME(i)) ? GET_DISGUISE_NAME(i) : "&=LRBogus char!&n");   // Alv
            }
          }
          // If not a disguised NPC, & we're showing titles to ch or diff't racewars (!including squids),
          //   and i's disguise has a title.
          if( !IS_DISGUISE_NPC(i) && ((IS_NPC(ch) || !IS_SET(ch->specials.act2, PLR2_NOTITLE))
            || (racewar(ch, i) && !IS_ILLITHID(ch))) && GET_DISGUISE_TITLE(i) )
          {
            strcat(buffer, " ");
            strcat(buffer, GET_DISGUISE_TITLE(i));
          }
          if( !IS_TRUSTED(i) && (!racewar(ch, i) || IS_ILLITHID(ch)) && IS_DISGUISE_PC(i) )
          {
            sprintf(buffer + strlen(buffer), " (%s)(%s)", race_names_table[race].ansi, size_types[GET_ALT_SIZE(i)]);
          }
        }
        else
        {
          race = GET_RACE(i);
          if( racewar(ch, i) && !IS_TRUSTED(i) && !IS_ILLITHID(ch) )
          {
            sprintf(buffer + strlen(buffer), "%s %s (%s)",
              ( race == RACE_ILLITHID || race == RACE_PILLITHID || race == RACE_ORC || race == RACE_OROG
              || race == RACE_AGATHINON || GET_RACE(i) == RACE_OGRE ) ? "An" : "A",
              race_names_table[(int) GET_RACE(i)].ansi, size_types[GET_ALT_SIZE(i)]);
          }
          else
          {
            if( IS_NPC(i) && (GET_RNUM(i) == real_mobile(IMAGE_REFLECTION_VNUM)) )
              sprintf(buffer + strlen(buffer), "%s", i->player.short_descr);
            else
            {
              if( is_introd(i, ch) )
              {
                sprintf(buffer + strlen(buffer), "%s", GET_NAME(i) ? GET_NAME(i) : "&=LRBogus char!&n");
              }
              else
              {
                sprintf(buffer + strlen(buffer), "%s", i->player.short_descr);
              }
            }
          }
          if( IS_HARDCORE(i) )
            strcat(buffer, " &+L(&+rHard&+RCore&+L)&n");

          if( (!IS_SET(ch->specials.act2, PLR2_NOTITLE) || (racewar(ch, i) && !IS_ILLITHID(ch))) && GET_TITLE(i) )
          {
            strcat(buffer, " ");
            strcat(buffer, GET_TITLE(i));
          }
          if( GET_LEVEL(i) < AVATAR && (!racewar(ch, i) || IS_ILLITHID(ch)) && is_introd(i, ch) )
          {
            sprintf(buffer + strlen(buffer), " (%s)(%s)",
              race_names_table[(int) GET_RACE(i)].ansi, size_types[GET_ALT_SIZE(i)]);
          }
        }
        //Backrank display - Drannak
        /*
				if (i->group && !IS_BACKRANKED(i))
          strcat(buffer, "(&+RF&n)");

        if (i->group && IS_BACKRANKED(i))
          strcat(buffer, "(&+BB&n)");
        */

        if (IS_PC(i) && i->desc && i->desc->olc)
          strcat(buffer, " (&+MOLC&N)");

        if (IS_PC(i) && IS_SET(i->specials.act, PLR_WRITE))
          strcat(buffer, " (&+LEDIT&N)");

        if (IS_PC(i) && IS_SET(i->specials.act, PLR_AFK) && IS_TRUSTED(ch))
          strcat(buffer, " (&+RAFK&N)");

        if (IS_TRUSTED(ch) && (IS_DISGUISE(i) || is_illusion_char(i)) )
        {
          if( is_illusion_char(i) )
            strcat(buffer, "(&+mIllusion&n)");
          else if( IS_DISGUISE_SHAPE(i) )
            strcat(buffer, "(&+mShapechanged&n)");
          else
            strcat(buffer, "(&+mdisguised&n)");
        }
/* Commented this out as it was showing (Illusion) 2x when you typed look.
        if( (IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION) || has_innate( ch, INNATE_DET_SUBVERSION)) && is_illusion_char(i))
          strcat(buffer, "(&+mIllusion&n)");
*/
        if (!i->desc && IS_TRUSTED(ch))
          strcat(buffer, " %%");
      }
      else
      {
        if (i->player.short_descr)
        {
          strcpy(buf2, i->player.short_descr);
          CAP(buf2);
          strcat(buffer, buf2);
          /* troops wont show any more info than this */
          /*
					if (i->group && !IS_BACKRANKED(i))
            strcat(buffer, "(&+RF&n)");

          if (i->group && IS_BACKRANKED(i))
            strcat(buffer, "(&+BB&n)");
        	*/
				}
      }

      if( !(IS_DISGUISE_NPC(i) && GET_STAT(i) == STAT_NORMAL && GET_POS(i) == POS_STANDING) )
      {
        switch( GET_STAT(i) )
        {
        case STAT_DEAD:
          sprintf(buffer + strlen(buffer), " is lying %s, quite dead",
            higher ? "above you" : lower ? "below you" : "here");
          break;
        case STAT_DYING:
          sprintf(buffer + strlen(buffer), " is lying %s, mortally wounded",
            higher ? "above you" : lower ? "below you" : "here");
          break;
        case STAT_INCAP:
          sprintf(buffer + strlen(buffer), " is lying %s, incapacitated",
            higher ? "above you" : lower ? "below you" : "here");
          break;
        case STAT_SLEEPING:
          switch (GET_POS(i))
          {
          case POS_PRONE:
            sprintf(buffer + strlen(buffer), " is stretched out%s, sound asleep",
              higher ? " above you" : lower ? " below you" : "");
            break;
          case POS_SITTING:
            sprintf(buffer + strlen(buffer), " has nodded off%s, sitting",
              higher ? " above you" : lower ? " below you" : "");
            break;
          case POS_KNEELING:
            sprintf(buffer + strlen(buffer), " is asleep%s, kneeling",
              higher ? " above you" : lower ? " below you" : "");
            break;
          case POS_STANDING:
            sprintf(buffer + strlen(buffer), " stands%s, apparently asleep",
              higher ? " above you" : lower ? " below you" : "");
            break;
          }
          if( IS_AFFECTED(i, AFF_BOUND) )
            strcat(buffer, " and restrained");
          break;
        case STAT_RESTING:
          switch (GET_POS(i))
          {
          case POS_PRONE:
            sprintf(buffer + strlen(buffer), " is sprawled out%s, resting",
              higher ? " above you" : lower ? " below you" : "");
            break;
          case POS_SITTING:
            sprintf(buffer + strlen(buffer), " sits resting%s",
              higher ? " above you" : lower ? " below you" : "");
            break;
          case POS_KNEELING:
            sprintf(buffer + strlen(buffer), " squats comfortably%s",
              higher ? " above you" : lower ? " below you" : "");
            break;
          case POS_STANDING:
            sprintf(buffer + strlen(buffer), " stands at ease%s",
              higher ? " above you" : lower ? " below you" : "");
            break;
          }
          if (IS_AFFECTED(i, AFF_KNOCKED_OUT))
            strcat(buffer, " and appears to be unconscious");
          if (IS_AFFECTED(i, AFF_BOUND))
            strcat(buffer, " and restrained");
          if (IS_AFFECTED2(i, AFF2_STUNNED))
            strcat(buffer, " with a stunned look");
          break;
        case STAT_NORMAL:
          if (!IS_RIDING(i)
              /*&& i->specials.z_cord == 0 *//*&& (!IS_AFFECTED(i, AFF_FLY | AFF_LEVITATE) || (higher || lower)) */
            )
            switch (GET_POS(i))
            {
            case POS_PRONE:
              sprintf(buffer + strlen(buffer), " is lying%s",
                higher ? " above you" : lower ? " below you" : "");
              break;
            case POS_SITTING:
              sprintf(buffer + strlen(buffer), " sits%s",
                higher ? " above you" : lower ? " below you" : "");
              break;
            case POS_KNEELING:
              sprintf(buffer + strlen(buffer), " crouches%s",
                higher ? " above you" : lower ? " below you" : "");
              break;
            case POS_STANDING:
              sprintf(buffer + strlen(buffer), " stands%s",
                higher ? " above you" : lower ? " below you" : "");
              break;
            }
          if (IS_AFFECTED(i, AFF_KNOCKED_OUT))
            strcat(buffer, " unconscious");
          if (IS_AFFECTED(i, AFF_BOUND))
            strcat(buffer, ", restrained");
          if (IS_AFFECTED2(i, AFF2_STUNNED))
            strcat(buffer, " with a stunned look");
/*        if (i->specials.z_cord > 0)
          strcat(buffer, " floats in mid-air");*/
          if (IS_AFFECTED(i, AFF_FLY) && !higher && !lower)
            strcat(buffer, " in mid-air");
          else if (IS_AFFECTED(i, AFF_LEVITATE) && !higher && !lower)
            strcat(buffer, ", floating");
          if (!IS_RIDING(i))
            strcat(buffer, " here");
          /*
          if (IS_SET(i->specials.act3, PLR3_FRAGLEAD))
            strcat(buffer, " (&+rF&+Rr&+ra&+Rg &+rL&+Ro&+rr&+Rd&n&N)");

          if (IS_SET(i->specials.act3, PLR3_FRAGLOW))
            strcat(buffer, " (&+yFrag &+YFood&n&N)");
          */
          if(IS_SET(i->specials.act2, PLR2_NEWBIE))
            strcat(buffer, " (&+GNewbie&N)");
          //drannak2
          break;
        }
        if( IS_RIDING(i) )
        {
          strcat(buffer, " sits atop ");
          if (IS_PC(i) && (get_linked_char(i, LNK_RIDING) == ch))
            strcat(buffer, " YOU");
          else
          {
            if (i->in_room == get_linked_char(i, LNK_RIDING)->in_room)
            {
              if (CAN_SEE(ch, get_linked_char(i, LNK_RIDING)))
              {
                if (IS_NPC(get_linked_char(i, LNK_RIDING)))
                  strcat(buffer,
                         get_linked_char(i, LNK_RIDING)->player.short_descr);
                else
                  strcat(buffer, GET_NAME(get_linked_char(i, LNK_RIDING)));
              }
              else
              {
                /* invis/hiding mount */
                strcat(buffer, "a cushion of air");
              }
            }
            else
              strcat(buffer, "someone who has already left");
          }
        }
        if (i->desc && i->desc->str)
          strcat(buffer, ", writing a message");

        if (i->lobj && i->lobj->Visible_Type())
          sprintf(buffer + strlen(buffer), " %s $p",i->lobj->Visible_Message());
        if (!i->specials.fighting)
        {
          if( i->specials.destroying_obj )
          {
            strcat( buffer, " destroying " );
            strcat( buffer, (i->specials.destroying_obj)->short_description );
          }
          strcat(buffer, ".");
        }
        else
        {
          strcat(buffer, ", fighting ");
          if (i->specials.fighting == ch)
            strcat(buffer, "YOU!");
          else
          {
            if (i->in_room == i->specials.fighting->in_room)
            {
              strcat(buffer, "$N.");
            }
            else
              strcat(buffer, "someone who has already left.");
          }
        }
      }

#if defined (CTF_MUD) && (CTF_MUD == 1)
      if ((af = get_spell_from_char(i, TAG_CTF)) != NULL)
      {
        if (af->modifier == CTF_FLAG_GOOD)
          strcat(buffer, " &+W(&+YFlag&+W)&n");
        else if (af->modifier == CTF_FLAG_EVIL)
          strcat(buffer, " &+W(&+rFlag&+W)&n");
        else
          strcat(buffer, " &+W(&+LFlag&+W)&n");
      }
#endif

      create_in_room_status(ch, i, buffer);

      if( IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_VNUM) && IS_NPC(i))
        sprintf(buffer + strlen(buffer), " [&+Y%d&n]", mob_index[GET_RNUM(i)].virtual_number);

      act(buffer, TRUE, ch, i->lobj->Visible_Object(), i->specials.fighting, TO_CHAR);
    }
    else
    {                           /* npc with long */
      if (i->player.long_descr || (IS_DISGUISE_NPC(i) && GET_DISGUISE_LONG(i)))
      {
        found = FALSE;
        if (!IS_DISGUISE_NPC(i) || !GET_DISGUISE_LONG(i))
        {
          strcpy(buf2, i->player.long_descr);
        }
        else
        {
          strcpy(buf2, GET_DISGUISE_LONG(i));
        }
        CAP(buf2);
        strcat(buffer, buf2);
        /* remove spurious characters from end of npc description. */
        j = strlen(buffer) - 1;
        if (!str_cmp((buffer + j - 1), "&n"))
        {
          found = TRUE;
          buffer[j - 1] = 0;
          j -= 2;
        }
        while( (j >= 0) && ((buffer[j] == '\n') || (buffer[j] == '\r') || (buffer[j] == '~') || (buffer[j] == ' ')) )
        {
          buffer[j] = 0;
          j--;
        }
        if (found)
          strcat(buffer, "&n");
      }
      *buf2 = '\0';

      /*
			if (i->group && !IS_BACKRANKED(i))
        strcat(buffer, "(&+RF&n)");

      if (i->group && IS_BACKRANKED(i))
        strcat(buffer, "(&+BB&n)");
			*/

      create_in_room_status(ch, i, buffer);

      if (higher)
        strcat(buf2, "(&+Wabove you&n) ");
      else if (lower)
        strcat(buf2, "(&+Wbelow you&n) ");

      if (IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_VNUM))
        sprintf(buf2 + strlen(buf2), "[&+Y%d&n] ",
                mob_index[GET_RNUM(i)].virtual_number);

      if (strlen(buf2))
        strcat(buffer, buf2);

      strcat(buffer, "\n");
      send_to_char(buffer, ch);
    }

    /* Obscuring mist (illusionist spec) spell */

    if (IS_AFFECTED5(i, AFF5_OBSCURING_MIST))
    {
      strcpy(buffer, "&+cA very thick &+Wcloud of mist&+c swirls all around $N.&n");
    }
    /* now show mage flame info on line after char info (if existant) */

    if( IS_AFFECTED4(i, AFF4_MAGE_FLAME) || IS_AFFECTED4(i, AFF4_GLOBE_OF_DARKNESS) )
    {
      lt_lvl = 0;

      /* first, check if spell has been cast, and base level of light on
         that.  if not, assume artifact or whatnot and use vict level */

      for (af = i->affected; af; af = af->next)
      {
        if ((IS_AFFECTED4(i, AFF4_MAGE_FLAME) ? (af->type == SPELL_MAGE_FLAME)
             : (af->type == SPELL_GLOBE_OF_DARKNESS)) && (af->modifier > 0))
        {
          lt_lvl = af->modifier / 10;
        }
      }

      if (!lt_lvl)
        lt_lvl = GET_LEVEL(i) / 10;

      if (IS_AFFECTED4(i, AFF4_MAGE_FLAME))
      {
        if (GET_CLASS(ch, CLASS_THEURGIST))
        {
          strcpy(buffer, "&+WA ");

          if (lt_lvl <= 1)
            strcat(buffer, "bright white ");
          else if (lt_lvl >= 5)
            strcat(buffer, "blindingly bright white ");

          strcat(buffer, "light floats near $N&+W's head. &n(&+Willuminating&n)");
        }
        else
        {
          strcpy(buffer, "&+WA ");

          if (lt_lvl <= 1)
            strcat(buffer, "feebly glowing ");
          else if (lt_lvl >= 5)
            strcat(buffer, "brightly glowing ");

          strcat(buffer, "torch floats near $N&+W's head. &n(&+Willuminating&n)");
        }
      }
      else
      {
        lt_lvl = -lt_lvl;

        strcpy(buffer, "&+LA");

        if (lt_lvl >= -1)
          strcat(buffer, " barely opaque");
        else if (lt_lvl <= -5)
          strcat(buffer, "n impenetrable");

        strcat(buffer, " globe of darkness floats near $N&+L's head.");
      }

      act(buffer, TRUE, ch, 0, i, TO_CHAR);
    }
  }
  else if (mode == 1)
  {
    *buffer = '\0';

    if (i->player.description)
      send_to_char(i->player.description, ch);
    else
      act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

    /* Show a character to another */
    show_visual_status(ch, i);

    if (IS_AFFECTED(ch, AFF_WRAITHFORM))
      return;

    // New look character wear order - Allenbri
    found = FALSE;
    for( j = 0; wear_order[j] != -1; j++ )
    {
      /* There is no -2 in wear_order ??
      if( wear_order[j] == -2 )
      {
        send_to_char("You are holding:\n", ch);
        found = FALSE;
        continue;
      }
      */
      if( (tmp_obj = i->equipment[wear_order[j]]) != NULL )
      {
        visobj = CAN_SEE_OBJ(ch, tmp_obj);
        if( visobj || (ch == i) )
        {
          if( ((wear_order[j] >= WIELD) && (wear_order[j] <= HOLD))
            && (wield_item_size(i, i->equipment[wear_order[j]]) == 2) )
          {
            send_to_char( (i->equipment[wear_order[j]]->type != ITEM_WEAPON)
              ? "<held in both hands> " : "<wielding twohanded> ", ch);
          }
          else
          {
            send_to_char( (wear_order[j] >= WIELD && wear_order[j] <= HOLD
              && i->equipment[wear_order[j]]->type != ITEM_WEAPON
              && i->equipment[wear_order[j]]->type != ITEM_FIREWEAPON) ? where[HOLD] : where[wear_order[j]], ch);
          }
          found = TRUE;
        }
        if( visobj )
        {
          show_obj_to_char(i->equipment[wear_order[j]], ch, 1, 1);
        }
        else if( ch == i )
        {
          send_to_char("Something.\n", ch);
        }
      }
    }

    if( (ch != i) && (GET_CLASS(ch, CLASS_ROGUE) || GET_CLASS(ch, CLASS_THIEF)) && !IS_TRUSTED(ch) )
    {
      found = FALSE;
      send_to_char("\nYou attempt to peek at the inventory:\n", ch);
      for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content)
      {
        if (CAN_SEE_OBJ(ch, tmp_obj) &&
            (number(1, GET_LEVEL(i) * 4) < GET_LEVEL(ch)))
        {
          show_obj_to_char(tmp_obj, ch, 1, 1);
          found = TRUE;
        }
      }
      if (!found)
        send_to_char("You can't see anything.\n", ch);
    }
  }
  else if ((mode == 2) && IS_TRUSTED(ch))
  {
    act("\n\r$n is carrying:", FALSE, i, 0, ch, TO_VICT);
    list_obj_to_char(i->carrying, ch, 1, TRUE);
  }
}

// mode argument is unused?
void list_char_to_char(P_char list, P_char ch, int mode)
{
  P_char i, j;
  char   buf[MAX_STRING_LENGTH];
  int    higher, lower;
  bool   globe, flame;

  globe = flame = FALSE;
  for( j = world[ch->in_room].people; j; j = j->next_in_room )
  {
    if( IS_AFFECTED4(j, AFF4_MAGE_FLAME) )
    {
      flame = TRUE;
      if( globe )
      {
        break;
      }
    }
    if( IS_AFFECTED4(j, AFF4_GLOBE_OF_DARKNESS) )
    {
      globe = TRUE;
      if( flame )
      {
        break;
      }
    }
  }

  for( i = list; i; i = i->next_in_room )
  {
    if( (i == ch) || WIZ_INVIS(ch, i) )
    {
      continue;
    }

    higher = (i->specials.z_cord > ch->specials.z_cord);
    lower = (i->specials.z_cord < ch->specials.z_cord);

//    if (i->specials.z_cord == ch->specials.z_cord) {
    if( !CAN_SEE_Z_CORD(ch, i) )
    {
      if( IS_AFFECTED(ch, AFF_SENSE_LIFE) && !IS_UNDEAD(i) &&
          !IS_ANGEL(i) && !IS_AFFECTED3(i, AFF3_NON_DETECTION) )
      {
        if( higher )
          send_to_char("&+LYou sense a lifeform above you.\n", ch);
        else if (lower)
          send_to_char("&+LYou sense a lifeform below you.\n", ch);
        else
          send_to_char("&+LYou sense a lifeform nearby.\n", ch);
      }
      else if( IS_AFFECTED(ch, AFF_SENSE_LIFE)
        && (IS_AFFECTED3(i, AFF3_NON_DETECTION) || IS_UNDEAD(i) || IS_ANGEL(i))
        && !number(0, 3) )
      {
        send_to_char("&+rYou barely sense a lifeform nearby.\n", ch);
      }
      continue;
    }
    /* ok, they can see SOMETHING at this point */

    /*
     * in the checks for light and dark... we should check it from
     * the vantage point of i... not of ch... as ch might not be in
     * the same room...
     */
    // CAN_SEE returns TRUE for infravision sight.
    if( CAN_SEE(ch, i) )
    {
      // Infravision: Too dark for day people, has infra, no ultra.
      if( !IS_TRUSTED(ch) && IS_AFFECTED(ch, AFF_INFRAVISION) && !IS_AFFECTED2(ch, AFF2_ULTRAVISION)
        && !CAN_DAYPEOPLE_SEE(i->in_room) && !flame )
      {
        sprintf(buf, "&+rYou see the red shape of a %s living being %shere.\n",
          size_types[GET_ALT_SIZE(i)], higher ? "above you " : lower ? "below you " : "");
        send_to_char(buf, ch);
      }
      else
      {
        show_char_to_char(i, ch, 0);
      }
      continue;
    }
    /*if (IS_AFFECTED2(ch, AFF2_ULTRAVISION) ||
        (IS_LIGHT(i->in_room)) ||
        (IS_TRUSTED(ch)) ||
        (IS_TWILIGHT_ROOM(i->in_room)) || (IS_AFFECTED(ch, AFF_WRAITHFORM)))
    {
      show_char_to_char(i, ch, 0);
      continue;
    }*/
  }
}

void do_do(P_char ch, char *argument, int cmd)
{
  /*
   * This command should replace all social commands ... as in do poke
   * instead of just poke ...
   */
  return;
}

// func assumes only PCs get in here

void ShowCharSpellBookSpells(P_char ch, P_obj obj)
{
  struct extra_descr_data *desc;
  char     buf[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
  int      i, j, k = 0, l, m = 0;

  if( IS_NPC(ch) )
  {
    send_to_char("you're an npc, leave me alone.\n", ch);
    return;
  }

  desc = find_spell_description(obj);
  if( !desc )
  {
    sprintf(buf, "$p appears to be unused and has %d pages left.",
            obj->value[2]);
    act(buf, TRUE, ch, obj, 0, TO_CHAR);
    return;
  }
  i = 0;
  j = 0;
/*
   if (obj->value[0] && GET_LANGUAGE (ch, obj->value[0]) <= (25 + number (1, 10))) {
   act ("$p is written in some language you don't understand!", TRUE, ch, obj, 0, TO_CHAR);
   return;
   }
 */
  if( obj->value[1] && !GET_CLASS(ch, obj->value[1]) )
  {
    sprintf(buf, "The original writer of $p appears to have been a %s.",
            class_names_table[flag2idx(obj->value[1])].ansi);
    act(buf, TRUE, ch, obj, 0, TO_CHAR);
  }
  if( obj->value[1] )
  {
    l = ch->player.m_class;
    ch->player.m_class = obj->value[1];

    /* whee, what a gay hack! */

    switch (GetClassType(ch))
    {
      case CLASS_TYPE_CLERIC:
        k = SKILL_SPELL_KNOWLEDGE_CLERICAL;
        break;
      default:
        k = SKILL_SPELL_KNOWLEDGE_MAGICAL;
        break;
    }
    ch->player.m_class = l;
  }
  else if( GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_MAGICAL) >
           GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_CLERICAL) )
    k = SKILL_SPELL_KNOWLEDGE_MAGICAL;
  else
    k = SKILL_SPELL_KNOWLEDGE_CLERICAL;
  buf[0] = 0;
  m = 0;
  for( j = FIRST_SPELL; j <= LAST_SPELL; j++ )
  {
    if( !SpellInThisSpellBook(desc, j) )
    {
      continue;
    }
    if( !((GET_CLASS(ch, obj->value[1])
      && get_spell_circle(ch, j) <= get_max_circle(ch))
      || (GET_CHAR_SKILL(ch, k) <= number(1, 100)
      && (GET_CHAR_SKILL(ch, k) && number(1, 70) <= GET_LEVEL(ch)))) )
    {
      continue;
    }
    if( m )
    {
      strcat(buf, ", ");
    }
    m++;
    strcat(buf, skills[j].name);
  }
#if 0
  if( m )
    strcat(buf, ".");
#endif
  if( !buf[0] )
  {
    send_to_char("It contains no spells you recognize outright ", ch);
  }
  else if( m == 1 )
  {
    sprintf(buf, "It has just one spell you recognize : %s ", buf);
    send_to_char(buf, ch);
  }
  else
  {
    sprintf(buf3, "It contains the spells %s ", buf);
    send_to_char(buf3, ch);
  }

  if( obj->value[2] <= obj->value[3] )
  {
    send_to_char("and has no free pages.\n", ch);
  }
  else
  {
    sprintf(buf, "and has %d free pages.\n", obj->value[2] - obj->value[3]);
    send_to_char(buf, ch);
  }
  if( k )
    notch_skill( ch, k, 5 );
}

void do_look(P_char ch, char *argument, int cmd)
{
  new_look(ch, argument, cmd, ch->in_room);
}

void display_room_auras(P_char ch, int room_no)
{
  char     buffer[MAX_STRING_LENGTH];

  if (room_no == NOWHERE)
    return;
    
  if (!ch || !IS_ALIVE(ch))
    return;

  if ((IS_AFFECTED2(ch, AFF2_DETECT_GOOD) ||
      affected_by_spell(ch, SPELL_AURA_SIGHT) ||
      affected_by_spell(ch, SPELL_FAERIE_SIGHT)) &&
       (IS_SET(world[room_no].room_flags, HEAL) ||
       get_spell_from_room(&world[room_no], SPELL_CONSECRATE_LAND)))
  {
    buffer[0] = 0;
    sprintf(buffer, "&+WA &+Bsoothing&+W aura fills the area.&n\n");
    send_to_char(buffer, ch);
  }
    
  if ((IS_AFFECTED2(ch, AFF2_DETECT_EVIL) ||
      affected_by_spell(ch, SPELL_AURA_SIGHT) ||
      affected_by_spell(ch, SPELL_FAERIE_SIGHT)) &&
      IS_SET(world[room_no].room_flags, NO_HEAL))
  {
    buffer[0] = 0;  
    sprintf(buffer, "&+LAn &n&+revil&+L aura fills the area.&n\n");
    send_to_char(buffer, ch);
  }
  
  if ((IS_AFFECTED2(ch, AFF2_DETECT_MAGIC) || 
      affected_by_spell(ch, SPELL_AURA_SIGHT) ||
      affected_by_spell(ch, SPELL_FAERIE_SIGHT)) &&
      IS_SET(world[room_no].room_flags, NO_MAGIC))
  {
    buffer[0] = 0;
    sprintf(buffer, "&+bThis area seems to be &+Ldevoid&n&+b of &+Bmagic&n&+b!&n\n");
    send_to_char(buffer, ch);
  }
    
  if((world[room_no].room_flags & SINGLE_FILE) ||
    (world[room_no].room_flags & TUNNEL))
  {
    buffer[0] = 0;
    sprintf(buffer, "&+WThis area seems to be exceptionally &+ynarrow!&n\n");
    send_to_char(buffer, ch);
  }
  
  if (IS_SET(world[room_no].room_flags, ROOM_SILENT))
  {
    buffer[0] = 0;
    strcat(buffer, "&+wAn &+yunnatural&+w silence fills this area.&n\r\n");
    send_to_char(buffer, ch);
  }
  
  if(IS_SET(world[ch->in_room].room_flags, MAGIC_LIGHT))
  {
    buffer[0] = 0;
    strcat(buffer, "&+wA &+Wbright light&n&+w fills this area.&n\r\n");
    send_to_char(buffer, ch);
  }
  
  if(IS_SET(world[ch->in_room].room_flags, MAGIC_DARK))
  {
    buffer[0] = 0;
    strcat(buffer, "&+LAn unnatural darkness fills this area.&n\r\n");
    send_to_char(buffer, ch);
  }
}

// Where's the desc for this AWFUL function?!?
// new_look(ch, 0, CMD_LOOKOUT, ship->location) == 'look out' while on ship.
void new_look(P_char ch, char *argument, int cmd, int room_no)
{
  char     buffer[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  char     arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  char     Gbuf5[MAX_STRING_LENGTH];
  int      keyword_no, j, bits, temp, vis_mode = 0;
  bool     found, brief_mode;
  P_obj    tmp_object, found_object;
  P_char   tmp_char;
  char    *tmp_desc;

  const char *keywords[] = {
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "in",
    "at",
    "",                         /* Look at '' case */
    "room",
    "northwest",
    "southwest",
    "northeast",
    "southeast",
    "nw",
    "sw",
    "ne",
    "se",
    "inside",
    "\n"
  };

  // No descriptor -> nobody to send info to.  valid room number check, and char alert enough to see.
  if( !ch->desc || room_no < 0 || room_no > top_of_world || GET_STAT(ch) < STAT_SLEEPING )
  {
    if( room_no < 0 || room_no > top_of_world )
    {
      debug( "new_look: room_no (&+C%d&n) out of range &+W0..%d&n.", room_no, top_of_world );
      send_to_char( "You're in a &+RVERY buggy&N room, please tell a god.\n\r", ch );
    }
    return;
  }
  if( GET_STAT(ch) == STAT_SLEEPING )
  {
    send_to_char("Try opening your eyes first.\n", ch);
    return;
  }
/* dont need a message, since being ko'd refuses commands already.
 * We simply need to make sure that being drug or some such doesn't
 * attempt to override command_interp
 */
  if( IS_AFFECTED(ch, AFF_KNOCKED_OUT) )
  {
    return;
  }
  if( IS_AFFECTED(ch, AFF_BLIND) )
  {
    send_to_char("You can't see a damn thing, you're blinded!\n", ch);
    return;
  }

  if( argument )
  {
    argument_split_2(argument, arg1, arg2);
  }
  else
  {
    *arg1 = '\0';
    *arg2 = '\0';
  }

  vis_mode = get_vis_mode(ch, room_no);
  if( vis_mode == 5 && cmd != CMD_LOOKOUT )
  {
    if( IS_MAP_ROOM(ch->in_room) && IS_MAP_ROOM(room_no) && *arg1 == '\0' )
    {
      map_look( ch, MAP_USE_TOGGLE );
    }
    send_to_char("&+LIt is pitch black...\n", ch);
    list_ships_to_char( ch, room_no );
    return;
  }
  if( vis_mode == 6 && cmd != CMD_LOOKOUT )
  {
    if( IS_MAP_ROOM(ch->in_room) && IS_MAP_ROOM(room_no) && *arg1 == '\0' )
    {
      map_look( ch, MAP_USE_TOGGLE );
    }
    send_to_char("&+WArgh!!! It's too bright!\n", ch);
    list_ships_to_char( ch, room_no );
    return;
  }

  brief_mode = IS_SET(GET_TRUE_CHAR(ch)->specials.act, PLR_BRIEF);

  /* Partial Match */
  keyword_no = search_block(arg1, keywords, FALSE);

  /* Let arg2 become the target object (arg1) */
  if( (keyword_no == -1) && *arg1 )
  {
    keyword_no = 7;
    strcpy(arg2, arg1);
  }
  found = FALSE;
  tmp_object = NULL;
  tmp_char = NULL;
  tmp_desc = NULL;

  // If we have no args, and CMD_LOOKOUT or CMD_LOOKAFAR
  if( (keyword_no == 8) && (cmd != CMD_LOOK) )
  {
    keyword_no = 20;
  }

  switch (keyword_no)
  {
    /* look <dir> */
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 10:
  case 11:
  case 12:
  case 13:
  case 14:
  case 15:
  case 16:
  case 17:
    /*
     * ok, this gets ugly, but adding vis_mode helps, all the
     * MAGIC_DARK cases are already handled.  vis_mode < 3 works prety
     * much like this routine always has.  vis_mode 3 can't see closed
     * exits.
     */

    /* we get to decrement keyword_no because of northwest etc */

    if (keyword_no >= 14)
      keyword_no -= 8;
    else if (keyword_no >= 10)
      keyword_no -= 4;

    if( !EXIT(ch, keyword_no) || (!IS_TRUSTED(ch) && IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ILLUSION)) )
    {
      /* not a valid exit there */
      send_to_char("You see nothing special...\n", ch);
      return;
    }
    if( !IS_TRUSTED(ch) && check_castle_walls(ch->in_room, EXIT(ch, keyword_no)->to_room) )
    {
      send_to_char("You cannot see over the castle walls.\r\n", ch);
      return;
    }
    /* there IS an exit in that direction */
    if( (vis_mode == 3) &&
        (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) ||
         IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SECRET) ||
         IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ILLUSION) ||
         IS_SET(EXIT(ch, keyword_no)->exit_info, EX_BLOCKED)) )
    {
      /* but infra-only can't detect it */
      send_to_char("&+rYou see nothing special...&n\n", ch);
      return;
    }

    /* vis_mode = 1, 2 or 4, or vis_mode = 3 and not a closed door */
    // WTF does faerie sight got to do with anything??
    if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ISDOOR) &&
        !affected_by_spell(ch, SPELL_FAERIE_SIGHT))
    {
      if (vis_mode == 1)
      {
        sprintf(buffer, "The %s is %s%s%s%s.\n", FirstWord(EXIT(ch, keyword_no)->keyword),
          IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) ? "closed" : "open",
          IS_SET(EXIT(ch, keyword_no)->exit_info, EX_LOCKED) ? " (locked)" : "",
          IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SECRET) ? " (hidden)" : "",
          IS_SET(EXIT(ch, keyword_no)->exit_info, EX_BLOCKED) ? " (blocked)" : "");
        send_to_char(buffer, ch);
      }
      else
      {
        if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SECRET) ||
            IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ILLUSION) ||
            IS_SET(EXIT(ch, keyword_no)->exit_info, EX_BLOCKED))
        {
          send_to_char("You see nothing special...\n", ch);
          return;
        }
        if( vis_mode == 4 )
        {
          if( IS_SET(EXIT(ch, keyword_no)->exit_info, EX_PICKPROOF) || (EXIT(ch, keyword_no)->key == -2) )
          {
            send_to_char("You see nothing special...\n", ch);
            return;
          }
          else if( IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ISDOOR)
            && IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) )
          {
            send_to_char("You see a tenuous barrier.\n", ch);
            return;
          }
          else
          {
            send_to_char("You see an opening.\n", ch);
          }
        }
        else if( vis_mode == 2 )
        {
          sprintf(buffer, "The %s is %s.\n", FirstWord(EXIT(ch, keyword_no)->keyword),
            IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) ? "closed" : "open");
          send_to_char(buffer, ch);
          if( (EXIT(ch, keyword_no)->general_description) )
//            && !IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) )
          {
            send_to_char(EXIT(ch, keyword_no)->general_description, ch);
          }
        }
      }

      if( IS_AFFECTED(ch, AFF_FARSEE) )
      {
        if( !IS_TRUSTED(ch) && IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) )
        {
          sprintf(buffer, "The closed %s blocks your farsight.\n",
            FirstWord(EXIT(ch, keyword_no)->keyword));
          send_to_char(buffer, ch);
          return;
        }
      }
      else
      {
        if (!IS_TRUSTED(ch))
        {
          return;
        }
      }
    }
    /* non-door exit */
    else
    {
      if( EXIT(ch, keyword_no)->general_description )
      {
        send_to_char(EXIT(ch, keyword_no)->general_description, ch);
      }
      else
      {
        send_to_char("Looks like an exit.\n", ch);
      }
      if( !IS_TRUSTED(ch) && !IS_AFFECTED(ch, AFF_FARSEE) )
      {
        return;
      }
    }

    /*
     * get here, and it's a visible open exit, is a mortal with
     * farsee, or a god (with or without farsee).  change vis_mode for
     * farsee. vis_mode 1: god sight (without farsee) 2: gods with
     * farsee or mortals with vis_mode 2 3: farsee (infravision)
     */
    /* real room we are peering into */
    temp = world[room_no].dir_option[keyword_no]->to_room;

    if( IS_AFFECTED(ch, AFF_FARSEE) )
    {
      sprintf(buffer, "You extend your sights %sward.\n", dirs[keyword_no]);
      send_to_char(buffer, ch);
    }
    for( tmp_char = world[temp].people; tmp_char != NULL; tmp_char = tmp_char->next_in_room )
    {
      if( IS_AFFECTED5(tmp_char, AFF5_OBSCURING_MIST) && !IS_TRUSTED(ch) )
      {
        send_to_char("&+cA very thick &+Wcloud of mist&+c blocks your vision.&n\n", ch);
        return;
      }
    }
    if( temp == NOWHERE )
    {
      if( IS_TRUSTED(ch) )
        send_to_char("You look into nothingness (NOWHERE).\n", ch);
      else
        send_to_char("Swirling mists block your sight.\n", ch);
      return;
    }
    if( (vis_mode == 1) && IS_AFFECTED(ch, AFF_FARSEE) )
    {
      vis_mode = 2;
    }

    /*
     * if return direction doesn't exist (one-way) or is not the
     * reverse direction, normal farsee will be blocked.  vis_mode 1
     * sees all.
     */
    if( (!world[temp].dir_option[rev_dir[keyword_no]]
      || (world[temp].dir_option[rev_dir[keyword_no]]->to_room != room_no))
      && !IS_TRUSTED(ch) && !IS_SET(world[ch->in_room].room_flags, GUILD_ROOM) )
    {
      /* sight blocked */
      send_to_char("Something seems to be blocking your line of sight.\n", ch);
      return;
    }

    if( IS_SET(world[ch->in_room].room_flags, BLOCKS_SIGHT) && !IS_TRUSTED(ch) )
    {
      send_to_char("It's too difficult to see what lies in that direction.\n", ch);
      return;
    }

    if( IS_SET(world[temp].room_flags, BLOCKS_SIGHT) && !IS_TRUSTED(ch) )
    {
      send_to_char("It's too difficult to see anything in that direction.\n", ch);
      return;
    }

    if( !IS_TRUSTED(ch) && !has_innate(ch, INNATE_EYELESS) )
    {
      if( has_innate(ch, INNATE_DAYBLIND) && IS_SUNLIT(temp) )
      {
        send_to_char("&+YThe sunlight! &+WIt's too bright to see!&n\n", ch);
        return;
      }

      if( (IS_AFFECTED2(ch, AFF2_ULTRAVISION) && IS_MAGIC_LIGHT(temp)
        && !OLD_RACE_NEUTRAL(GET_RACE(ch))
        && !OLD_RACE_GOOD(GET_RACE(ch), GET_ALIGNMENT(ch))
        && !IS_HARPY(ch) && !IS_REVENANT(ch) && !IS_DRAGONKIN(ch) && !IS_HALFORC(ch))
        || (IS_MAGIC_LIGHT(temp)
        && !OLD_RACE_NEUTRAL(GET_RACE(ch))
        && OLD_RACE_EVIL(GET_RACE(ch), GET_ALIGNMENT(ch))
        && !IS_HARPY(ch) && !IS_REVENANT(ch) && !IS_DRAGONKIN(ch) && !IS_HALFORC(ch)) )
      {
        send_to_char("&+WThe brightness there hurts your head!\n", ch);
        return;
      }
      else if( (IS_MAGIC_DARK(temp) && !OLD_RACE_NEUTRAL(GET_RACE(ch)))
        && OLD_RACE_GOOD(GET_RACE(ch), GET_ALIGNMENT(ch)) && !IS_HARPY(ch)
        && !IS_HALFORC(ch) && !IS_REVENANT(ch) && !IS_DRAGONKIN(ch) )
      {
        send_to_char("&+LIt's much too dark there for you to see!\n", ch);
        return;
      }
    }

    new_look(ch, NULL, CMD_LOOKAFAR, temp);
    break;

  case 6:
    /* look 'in' */
    if( vis_mode == 3 )
    {
      send_to_char("&+rYou can't make out much detail in the dark with just infravision..&n\n", ch);
      break;
    }
    if( vis_mode == 4 )
    {
      send_to_char("You can't seem to spot any objects in this form.\n\r", ch );
      break;
    }
    if( *arg2 )
    {
      /* Item carried or in room */
      if( IS_TRUSTED(ch) )
      {
        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
      }
      else
      {
        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_NO_TRACKS,
          ch, &tmp_char, &tmp_object);
      }
      if( bits != 0 )
      {                         /* Found something */
        if (GET_ITEM_TYPE(tmp_object) == ITEM_DRINKCON)
        {
          if( tmp_object->value[1] == 0 )
          {
            act("It is empty.", FALSE, ch, 0, 0, TO_CHAR);
          }
          else
          {
            if (tmp_object->value[1] < 0)
              temp = 3;
            else
              temp = ((tmp_object->value[1] * 3) / tmp_object->value[0]);

	    temp = BOUNDED(0, temp, 3);
            sprintf(buffer, "It's %sfull of a %s liquid.\n",
                    fullness[temp], color_liquid[tmp_object->value[2]]);
            send_to_char(buffer, ch);
          }
        }
        else if (GET_ITEM_TYPE(tmp_object) == ITEM_CORPSE)
        {
          sprintf(buf, "It appears to be the corpse of %s.\n",
                  tmp_object->action_description);
          send_to_char(buf, ch);
          list_obj_to_char(tmp_object->contains, ch, 2, TRUE);
        }
        else if (GET_ITEM_TYPE(tmp_object) == ITEM_SPELLBOOK)
        {
          ShowCharSpellBookSpells(ch, tmp_object);
        }
        else if ((GET_ITEM_TYPE(tmp_object) == ITEM_CONTAINER) ||
                 (GET_ITEM_TYPE(tmp_object) == ITEM_STORAGE))
        {
          if (IS_SET(tmp_object->value[1], CONT_CLOSED))
            send_to_char("It is closed.\n", ch);
          else
            list_obj_to_char(tmp_object->contains, ch, 2, TRUE);
        }
        else if (GET_ITEM_TYPE(tmp_object) == ITEM_QUIVER)
        {
          list_obj_to_char(tmp_object->contains, ch, 2, TRUE);
        }
        else
          send_to_char("That is not a container.\n", ch);
      }
      else                      /* wrong argument */
        send_to_char("You do not see that item here.\n", ch);
    }
    else                        /* no argument */
      send_to_char("Look in what?!\n", ch);
    break;

  case 7:
    /* look 'at' */
    if( vis_mode == 3 )
    {
      send_to_char("&+rYou can't make out much detail in the dark with just infravision..&n\n", ch);
      break;
    }
    if( *arg2 )
    {
      if( IS_TRUSTED(ch) )
      {
        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM,
          ch, &tmp_char, &found_object);
      }
      else
      {
        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM | FIND_NO_TRACKS,
          ch, &tmp_char, &found_object);
      }
      if( tmp_char )
      {
        show_char_to_char(tmp_char, ch, 1);
        /* Immortals can also see person's inventory */
        if( IS_TRUSTED(ch) )
        {
          show_char_to_char(tmp_char, ch, 2);
        }
        if( ch != tmp_char )
        {
          act("$n looks at you.", TRUE, ch, 0, tmp_char, TO_VICT);
          act("$n looks at $N.", TRUE, ch, 0, tmp_char, TO_NOTVICT);
        }
        return;
      }
      if( vis_mode == 4 )
      {
        found = FALSE;
        found_object = NULL;
      }
      /* Search for Extra Descriptions in room and items */
      /* Extra description in room?? */
      if( !found )
      {
        tmp_desc = find_ex_description(arg2, world[room_no].ex_description);
        if (tmp_desc)
        {
          page_string(ch->desc, tmp_desc, 0);
          return;               /* RETURN SINCE IT WAS A ROOM DESCRIPTION */
        }
      }
      /* Search for extra descriptions in items, if not wraithform*/
      /* Equipment Used */
      if( !found && vis_mode != 4 )
      {
        for( j = 0; j < MAX_WEAR && !found; j++ )
        {
          if( ch->equipment[j] )
          {
            if( CAN_SEE_OBJ(ch, ch->equipment[j]) )
            {
              tmp_desc = find_ex_description(arg2, ch->equipment[j]->ex_description);
              if( tmp_desc )
              {
                page_string(ch->desc, tmp_desc, 1);
                found = TRUE;
              }
            }
          }
        }
      }
      if (vis_mode != 4)
      {
        /* In inventory */
        if( !found)
        {
          for( tmp_object = ch->carrying; tmp_object && !found; tmp_object = tmp_object->next_content )
          {
            if( CAN_SEE_OBJ(ch, tmp_object) )
            {
              tmp_desc = find_ex_description(arg2, tmp_object->ex_description);
              if( tmp_desc )
              {
                page_string(ch->desc, tmp_desc, 1);
                found = TRUE;
              }
            }
          }
        }
        /* Object In room */
        if( !found )
        {
          for( tmp_object = world[room_no].contents; tmp_object && !found; tmp_object = tmp_object->next_content )
          {
            if( CAN_SEE_OBJ(ch, tmp_object) || IS_OBJ_STAT(tmp_object, ITEM_NOSHOW) )
            {
              /* can't look at tracks */
						  if( tmp_object->R_num != real_object(VNUM_TRACKS) )
              {
              	tmp_desc = find_ex_description(arg2, tmp_object->ex_description);
              	if( tmp_desc )
              	{
                	page_string(ch->desc, tmp_desc, 1);
                	found = TRUE;
              	}
							}
            }
          }
        }
      }
      /* wrong argument */
      if( bits && (vis_mode != 4) )
      {                         /* If an object was found */
        if( !found )
        {
          show_obj_to_char(found_object, ch, 5, 1);     /* Show no-description */
        }
        else
        {
          show_obj_to_char(found_object, ch, 6, 1);     /* Find hum, glow etc */
        }
      }
      else if( !found )
      {
        send_to_char("You do not see that here.\n", ch);
      }
    }
    else
    {
      /* no argument */
      send_to_char("Look at what?\n", ch);
    }
    break;

  case 8:                      /* look COMMAND, with NULL args, brief is forced */
  case 9:                      /* look 'room', brief is overridden */
  case 20:                     /* look called with CMD_LOOKOUT or CMD_LOOKAFAR */

  case 18:                     // looking 'inside' something - so we can show a room w/o the map

    switch (keyword_no)
    {
      case 8:                    /* look COMMAND, with NULL args, brief is forced */
        if( IS_MAP_ROOM(ch->in_room) || ch->specials.z_cord < 0 || (IS_MAP_ROOM(room_no) && cmd == CMD_LOOKOUT) )
          map_look_room(ch, room_no, MAP_USE_TOGGLE);
        brief_mode = TRUE;
        break;
      case 9:                    /* look 'room', brief is overridden */
        if( IS_MAP_ROOM(ch->in_room) || ch->specials.z_cord < 0 )
          map_look(ch, MAP_IGNORE_TOGGLE);
        brief_mode = FALSE;
        break;
      case 20:
        // Look out -> look out of a tower, or out of a ship.
        if( IS_MAP_ROOM(room_no) && (cmd == CMD_LOOKOUT) )
        {
          map_look_room(ch, room_no, MAP_IGNORE_TOGGLE);
        }
        // CMD_LOOKAFAR -> look in a room that ch isn't in.
        if( IS_MAP_ROOM(ch->in_room) && IS_MAP_ROOM(room_no) && (cmd == CMD_LOOKAFAR) )
        {
          map_look_room(ch, room_no, MAP_USE_TOGGLE);
        }
        else if( IS_MAP_ROOM(ch->in_room) )
        {
          map_look_room(ch, ch->in_room, MAP_USE_TOGGLE);
        }
        break;
      default:
        break;
    }

    if( vis_mode == 4 )
    {
      /* cackle! JAB */
      if( IS_SET(world[room_no].room_flags, INDOORS) )
      {
        send_to_char("An Enclosed Space\n", ch);
      }
      else
      {
        send_to_char("An Open Space\n", ch);
      }
    }
    else if (world[room_no].sector_type == SECT_CASTLE_GATE)
    {
      send_to_char("&+LA Large Iron Portcullis&n\n", ch);
    }
    else
    {
      if (ch->specials.z_cord > 0 && ch->specials.z_cord < 5)
        send_to_char("Flying above ", ch);
      else if (ch->specials.z_cord > 4)
        send_to_char("Flying high above ", ch);
      else if (ch->specials.z_cord < 0)
        send_to_char("Swimming below ", ch);
      send_to_char(world[room_no].name, ch);
      if( IS_SET(ch->specials.act, PLR_VNUM) && IS_TRUSTED(ch) )
      {
        sprintf(buffer, " [&+R%d&N:&+C%d&N]", zone_table[world[room_no].zone].number, world[room_no].number);
        send_to_char(buffer, ch);
      }
      send_to_char("\n", ch);

      if( !(brief_mode || (keyword_no == 8) || (vis_mode == 3)) )
      {
        if (world[room_no].description)
          send_to_char(world[room_no].description, ch);

        display_room_auras(ch, room_no);
      }
    }

    // cmd == CMD_LOOKOUT -> Mode = -1 (for look out on ship).
    show_exits_to_char(ch, room_no, (cmd == CMD_LOOKOUT) ? -1 : 1);
    if (get_spell_from_room(&world[room_no], SPELL_WANDERING_WOODS))
    {
      sprintf(Gbuf5, "&+GAn air of bewildering enchantment lies heavy here.&n\n");
      send_to_char(Gbuf5, ch);
    }
    if (get_spell_from_room(&world[room_no], SPELL_CONSECRATE_LAND))
    {
      sprintf(Gbuf5, "&+CA series of magical runes are dispersed about this area.&n\n");
      send_to_char(Gbuf5, ch);
    }
    if (get_spell_from_room(&world[room_no], SPELL_DESECRATE_LAND))
    {
      sprintf(Gbuf5, "&+LA series of &+Revil&+L runes are dispersed about this area.&n\n");
      send_to_char(Gbuf5, ch);
    }
    if (get_spell_from_room(&world[room_no], SPELL_FORBIDDANCE))
    {
      sprintf(Gbuf5, "&+LA strange unwelcoming energy flows through the area.&n\n");
      send_to_char(Gbuf5, ch);
    }
		if (get_spell_from_room(&world[room_no], SPELL_BINDING_WIND)) 
		{
			sprintf(Gbuf5, "&+CThe wind has picked up so that it is hard to move!&n\n");
			send_to_char(Gbuf5, ch);
		}
		if (get_spell_from_room(&world[room_no], SPELL_WIND_TUNNEL)) {
			sprintf(Gbuf5, "&+cThe wind has picked up so that is easier to move!&n\n");
			send_to_char(Gbuf5, ch);
		}
/*    if(world[room_no].troop_info)
    {
      sprintf(Gbuf5,"&+RThere are some troops here, bearing the mark of Kingdom #%d&n\n", world[room_no].troop_info->kingdom_num);
      send_to_char(Gbuf5, ch);
    }  */

    // If we can see normally, or we're on a ship looking out.
    if( (vis_mode == 2 || vis_mode == 1) || (( cmd == CMD_LOOKOUT ) && ( vis_mode == 5 || vis_mode == 6 )) )
    {
      list_obj_to_char(world[room_no].contents, ch, 0, FALSE);
    }

    list_char_to_char(world[room_no].people, ch, 0);

    show_tracks(ch, room_no);

    break;

    /* wrong arg */
  default:
    send_to_char("Sorry, I didn't understand that!\n", ch);
    break;
  }
}

/* end of look */


// mode: 2 == exits command, 1 == look command/move to new room/etc.
//      -1 == 'look out' on ship.
void show_exits_to_char(P_char ch, int room_no, int mode)
{
  int      i, vis_mode, count;
  struct room_direction_data *exit;
  bool     brief_mode;
  P_char   tmp_char;
  char     buffer[MAX_STRING_LENGTH];

  const char *short_exits[] = {
    "N",
    "E",
    "S",
    "W",
    "U",
    "D",
    "NW",
    "SW",
    "NE",
    "SE"
  };

  const char *exits[] = {
    "&+cNorth&n",
    "&+cEast&n",
    "&+cSouth&n",
    "&+cWest&n",
    "&+cUp&n",
    "&+cDown&n",
    "&+cNorthwest&n",
    "&+cSouthwest&n",
    "&+cNortheast&n",
    "&+cSoutheast&n"
  };

  *buffer = '\0';

  if( room_no < 0 )
  {
    room_no = ch->in_room;
  }

  vis_mode = get_vis_mode(ch, room_no);
  // mode == 1 -> exits via new_look: if they're blind, no obvious exits.
  // mode == 2 -> actual exits command: check for exits.  Should we allow this?
  if( (vis_mode == 5 || vis_mode == 6) && mode == 1 )
  {
//    send_to_char("&+gObvious exits: &+RNone!\n", ch);
    return;
  }
  if( mode == -1 )
  {
    mode = 1;
  }

  brief_mode = IS_SET(ch->specials.act, PLR_BRIEF);

  strcpy(buffer, "&+gObvious exits:&n");

  if( mode == 1 )
  {
    for (count = 0, i = 0; i < NUM_EXITS; i++)
    {
      exit = world[room_no].dir_option[i];
      if( !exit || (exit->to_room == NOWHERE && ( (vis_mode = get_vis_mode( ch, exit->to_room )) != 1) ) )
      {
        continue;
      }
      switch( vis_mode )
      {
      // God vision.
      case 1:
        count++;
        if( !brief_mode )
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", exits[i]);
        }
        else
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", short_exits[i]);
        }
        if (IS_SET(exit->exit_info, EX_CLOSED))
        {
          strcat(buffer, "&+g#&n");
        }
        if (IS_SET(exit->exit_info, EX_LOCKED))
        {
          strcat(buffer, "&+y@&n");
        }
        if (IS_SET(exit->exit_info, EX_SECRET))
        {
          strcat(buffer, "&+L*&n");
        }
        if (IS_SET(exit->exit_info, EX_BLOCKED))
        {
          strcat(buffer, "&+r%&n");
        }
        if (IS_SET(exit->exit_info, EX_WALLED))
        {
          strcat(buffer, "&+Y!&n");
        }

        if( IS_SET(ch->specials.act, PLR_VNUM) )
        {
          if( exit->to_room == NOWHERE )
          {
            strcat(buffer, "[&+RNOWHERE&N] ");
          }
          else
          {
            sprintf(buffer + strlen(buffer), " [&+R%d&N:&+C%d&N]",
              zone_table[world[exit->to_room].zone].number, world[exit->to_room].number);
          }
        }
        break;
      case 2:
        if( IS_SET(exit->exit_info, EX_SECRET) || IS_SET(exit->exit_info, EX_BLOCKED)
          || IS_SET(exit->exit_info, EX_ILLUSION) )
        {
          break;
        }
        count++;
        if( !brief_mode )
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", exits[i]);
        }
        else
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", short_exits[i]);
        }
        if(IS_SET(exit->exit_info, EX_CLOSED))
        {
          strcat(buffer, "&+g#&n");
        }
        break;
      case 3:
        if( IS_SET(exit->exit_info, EX_SECRET) || IS_SET(exit->exit_info, EX_CLOSED)
          || IS_SET(exit->exit_info, EX_BLOCKED) )
        {
          break;
        }
        count++;
        if( !brief_mode )
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", exits[i]);
        }
        else
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", short_exits[i]);
        }
        break;
      case 4:
        if( IS_SET((world[room_no].dir_option[i])->exit_info, EX_SECRET)
          || (IS_SET((world[room_no].dir_option[i])->exit_info, EX_CLOSED)
          && (IS_SET((world[room_no].dir_option[i])->exit_info, EX_PICKPROOF)
          || ((world[room_no].dir_option[i])->key == -2)))
          || IS_SET((world[room_no].dir_option[i])->exit_info, EX_BLOCKED))
        {
          break;
        }
        count++;
        if( !brief_mode )
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", exits[i]);
        }
        else
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", short_exits[i]);
        }
        if( IS_SET((world[room_no].dir_option[i])->exit_info, EX_CLOSED) )
        {
          strcat(buffer, "&+g#&n");
        }
        break;
      case 5:
      case 6:
        if( IS_SET(exit->exit_info, EX_SECRET) || IS_SET(exit->exit_info, EX_BLOCKED)
          || IS_SET(exit->exit_info, EX_ILLUSION) )
        {
          break;
        }
        count++;
        if( !brief_mode )
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", exits[i]);
        }
        else
        {
          sprintf(buffer + strlen(buffer), " &+c-%s&n", short_exits[i]);
        }
        if( IS_SET(exit->exit_info, EX_CLOSED) )
        {
          strcat(buffer, "&+g#&n");
        }
        else if( vis_mode == 5 )
        {
          strcat(buffer, "&+LD&n");
        }
        else
        {
          strcat(buffer, "&+WB&n");
        }
        break;
      }
    }
  }
  else if( mode == 2 )
  {
    strcat(buffer, "\n");
    for( count = 0, i = 0; i < NUM_EXITS; i++ )
    {
      exit = world[room_no].dir_option[i];
      if( !exit || (exit->to_room == NOWHERE && (vis_mode != 1)) )
      {
        continue;
      }
      // Gods need to see NOWHERE exits and you can dereference NOWHERE.
      if( vis_mode != 1 )
      {
        vis_mode = get_vis_mode(ch, exit->to_room);
      }

      switch( vis_mode )
      {
      case 1:
        count++;
        sprintf(buffer + strlen(buffer), "%14s- %s%s%s%s%s%s&n ", exits[i],
          IS_SET(exit->exit_info, EX_ISDOOR) ? "&+yD" : " ",
          IS_SET(exit->exit_info, EX_CLOSED) ? "&+gC" : " ",
          IS_SET(exit->exit_info, EX_LOCKED) ? "&+RL&n" : " ",
          IS_SET(exit->exit_info, EX_SECRET) ? "&+LS&n" : " ",
          IS_SET(exit->exit_info, EX_BLOCKED) ? "&+bB" : " ",
          IS_SET(exit->exit_info, EX_WALLED) ? "&+YW&n" : " ");

        if( exit->to_room != NOWHERE )
        {
          if (IS_SET(world[exit->to_room].room_flags, MAGIC_DARK))
          {
            strcat(buffer, "(&+LMagic Dark&n) ");
          }
          else if (!IS_LIGHT(exit->to_room))
          {
            strcat(buffer, "(&+LDark&n) ");
          }
          else if (IS_SET(world[exit->to_room].room_flags, MAGIC_LIGHT))
          {
            strcat(buffer, "(&+WMagic Light&n) ");
          }
        }
        if( IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_VNUM) )
        {
          if( exit->to_room == NOWHERE )
          {
            strcat(buffer, "[&+RNOWHERE&N] ");
          }
          else
          {
            sprintf(buffer + strlen(buffer), "[&+R%d&N:&+C%d&N] ",
              zone_table[world[exit->to_room].zone].number, world[exit->to_room].number);
          }
        }
        if( exit->to_room != NOWHERE )
        {
          strcat(buffer, world[exit->to_room].name);
        }
        strcat(buffer, "\n");
        break;
      case 2:
        if( (exit->to_room == NOWHERE) || IS_SET(exit->exit_info, EX_SECRET)
          || IS_SET(exit->exit_info, EX_BLOCKED) )
        {
          break;
        }
        count++;
        sprintf(buffer + strlen(buffer), "%14s- ", exits[i]);
        if( IS_SET(exit->exit_info, EX_ISDOOR) )
        {
          sprintf(buffer + strlen(buffer), "(%s %s) ", IS_SET(exit->exit_info, EX_CLOSED) ? "closed" : "open",
            FirstWord(EXIT(ch, i)->keyword));
        }
        /* can we see that exit is walled off ?*/
  	    if( check_visible_wall(ch, i) )
        {
		      sprintf(buffer + strlen(buffer), "(%s) ", get_wall_dir(ch, i)->short_description);
        }
        if( !IS_SET(exit->exit_info, EX_CLOSED) )
        {
          strcat(buffer, world[exit->to_room].name);
        }
        strcat(buffer, "\n");
        break;
      case 3:
        if( (exit->to_room == NOWHERE) || IS_SET(exit->exit_info, EX_SECRET)
          || IS_SET(exit->exit_info, EX_CLOSED) || IS_SET(exit->exit_info, EX_BLOCKED) )
        {
          break;
        }
        count++;
        sprintf(buffer + strlen(buffer), "%14s- %s\n", exits[i], world[exit->to_room].name);
        break;
      case 4:
        if( (exit->to_room == NOWHERE) || IS_SET(exit->exit_info, EX_SECRET)
          || IS_SET(exit->exit_info, EX_BLOCKED) || IS_SET(exit->exit_info, EX_PICKPROOF)
          || (exit->key == -2) )
        {
          break;
        }
        count++;
        sprintf(buffer + strlen(buffer), "%s- ", exits[i]);
        if( IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED) )
        {
          strcat(buffer, "(veiled)\n");
        }
        else
        {
          strcat(buffer, "open\n");
        }
        break;
      case 5:
        if( (exit->to_room == NOWHERE) || IS_SET(exit->exit_info, EX_SECRET)
          || IS_SET(exit->exit_info, EX_BLOCKED) || (exit->key == -2) )
        {
          break;
        }
        count++;
        sprintf(buffer + strlen(buffer), "%s- ", exits[i]);
        if( IS_SET(exit->exit_info, EX_CLOSED) )
        {
          sprintf(buffer + strlen(buffer), "(closed %s) ", FirstWord(EXIT(ch, i)->keyword));
          break;
        }
        strcat(buffer, "&+LToo dark to tell.&n.");
        break;
      case 6:
        if( (exit->to_room == NOWHERE) || IS_SET(exit->exit_info, EX_SECRET)
          || IS_SET(exit->exit_info, EX_BLOCKED) || (exit->key == -2) )
        {
          break;
        }
        count++;
        sprintf(buffer + strlen(buffer), "%s- ", exits[i]);
        if( IS_SET(exit->exit_info, EX_CLOSED) )
        {
          sprintf(buffer + strlen(buffer), "(closed %s) ", FirstWord(EXIT(ch, i)->keyword));
          break;
        }
        strcat(buffer, "&+WToo bright to tell.&n.");
        break;
      default:
        if( (exit->to_room == NOWHERE) || IS_SET(exit->exit_info, EX_SECRET)
          || IS_SET(exit->exit_info, EX_BLOCKED) || IS_SET(exit->exit_info, EX_PICKPROOF)
          || (exit->key == -2) )
        {
          break;
        }
        count++;
        sprintf(buffer + strlen(buffer), "%s- ", exits[i]);
        strcat(buffer, "Buggy vis mode, plz tell a god.");
        break;
      }
    }
  }
  if( !count )
  {
    strcat(buffer, " &+RNone!\n");
  }
  else if( mode == 1 )
  {
    strcat(buffer, "\n");
  }

  send_to_char(buffer, ch);
}

void do_read(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_INPUT_LENGTH];

  /*
   * This is just for now - To be changed later.!
   */
  sprintf(buf, "at %s", argument);
  do_look(ch, buf, -4);
}

void do_examine(P_char ch, char *argument, int cmd)
{
  char     name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH + 4], buf2[MAX_INPUT_LENGTH];
  int      bits, wtype, craft, mat, percent;
  P_char   tmp_char;
  P_obj    tmp_object;
  float    result_space;

  one_argument(argument, name);

  if (!*name)
  {
    send_to_char("Examine what?\n", ch);
    return;
  }
  sprintf(buf, "at %s", argument);
  do_look(ch, buf, -4);

  // This needs to match the generic find in new_look, switch(keyword_no) case 7,
  //   so that we are dealing with the same char/obj.  This is really bad form, since we should do the
  //   lookup once and have a function to display what we found called in each case (here and new_look).
  if( IS_TRUSTED(ch) )
  {
    bits = generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM,
      ch, &tmp_char, &tmp_object);
  }
  else
  {
    bits = generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM | FIND_NO_TRACKS,
      ch, &tmp_char, &tmp_object);
  }

  // check legend lore
  if( tmp_object && (GET_CHAR_SKILL(ch, SKILL_LEGEND_LORE) > number(0, 110)) &&
		  (GET_ITEM_TYPE(tmp_object) != ITEM_CONTAINER && GET_ITEM_TYPE(tmp_object) != ITEM_CORPSE))
  {
    notch_skill( ch, SKILL_LEGEND_LORE, 5 );
    spell_identify(GET_LEVEL(ch), ch, 0, 0, 0, tmp_object);
    CharWait(ch, (int) (PULSE_VIOLENCE * 1.5));
    return;
  }

  if (tmp_object)
    if (obj_index[tmp_object->R_num].virtual_number == 1252)
    {
      do_lore(ch, buf, 999);
    }
  if (tmp_object)
  {
    if ((tmp_object->craftsmanship < OBJCRAFT_LOWEST) ||
        (tmp_object->craftsmanship > OBJCRAFT_HIGHEST))
    {
      act("$p has a buggy craftsmanship value, tell a god-type fellow.",
          FALSE, ch, tmp_object, 0, TO_CHAR);
      craft = OBJCRAFT_AVERAGE;
    }
    else
      craft = tmp_object->craftsmanship;

    if (GET_C_INT(ch) < number(1, 100))
    {
      craft += (number(0, 1) ? 1 : -1);
      craft = BOUNDED(OBJCRAFT_LOWEST, craft, OBJCRAFT_HIGHEST);
    }

    if ((tmp_object->material < MAT_LOWEST) ||
        (tmp_object->material > MAT_HIGHEST))
    {
      act("$p has a buggy material value, tell a god-type fellow.", FALSE, ch,
          tmp_object, 0, TO_CHAR);
      mat = MAT_STEEL;
    }
    else
      mat = tmp_object->material;

    sprintf(buf, "$p is made of %s and appears to be %s.",
            materials[mat].name, craftsmanship_names[craft]);
    act(buf, FALSE, ch, tmp_object, 0, TO_CHAR);

    sprintf(buf, "$p &nhas an item value of &+W%d&n.",
            itemvalue(ch, tmp_object));
    act(buf, FALSE, ch, tmp_object, 0, TO_CHAR);

    if ((GET_ITEM_TYPE(tmp_object) == ITEM_WAND || GET_ITEM_TYPE(tmp_object) == ITEM_STAFF) && IS_AFFECTED2(ch, AFF2_DETECT_MAGIC))
    {
      int max = tmp_object->value[1];
      int curr = tmp_object->value[2];
      int ratio = (int)(100*curr/max);

      if (curr < 0 | max < 0)
        sprintf(buf, "$p seems to be bugged - please notify a god-type fellow, and report the item via the BUG command!");
      

      if (ratio >= 100)
        sprintf(buf, "$p seems to be unused.");
      else if (ratio > 90)
        sprintf(buf, "$p seems to be slightly used.");
      else if (ratio > 55)
        sprintf(buf, "$p seems to be somehow depleted of its magic.");  
      else if (ratio > 45)
        sprintf(buf, "$p seems to be about halfway full.");
      else if (ratio > 10)
        sprintf(buf, "$p seems to be worn out.");
      else if (ratio > 0)
        sprintf(buf, "$p seems to be almost dried up.");
      else if (ratio == 0)
        sprintf(buf, "$p seems to be completely drained of its magic."); 

      act(buf, FALSE, ch, tmp_object, 0, TO_CHAR);
    }

    if (GET_ITEM_TYPE(tmp_object) == ITEM_WEAPON)
    {
      if ((tmp_object->value[0] < WEAPON_LOWEST) ||
          (tmp_object->value[0] > WEAPON_HIGHEST))
      {
        act("$p has a buggy weapon type, tell a god-type fellow.", FALSE, ch,
            tmp_object, 0, TO_CHAR);
        wtype = WEAPON_CLUB;
      }
      else
        wtype = tmp_object->value[0];

      sprintf(buf, "$p is a %s.", weapon_types[wtype - 1].flagLong);

      act(buf, FALSE, ch, tmp_object, 0, TO_CHAR);
    }
    else
      if ((GET_ITEM_TYPE(tmp_object) == ITEM_DRINKCON) ||
          (GET_ITEM_TYPE(tmp_object) == ITEM_CONTAINER) ||
          (GET_ITEM_TYPE(tmp_object) == ITEM_STORAGE) ||
          (GET_ITEM_TYPE(tmp_object) == ITEM_QUIVER))
    {
/*
      if (GET_ITEM_TYPE(tmp_object) == ITEM_CONTAINER) {
         if (tmp_object->value[3] <= 0) {
           send_to_char("Seem to be a problem with this container! Contact a God.\n",ch);

         } else {
           sprintf(buf, "%s&n is ",
                  tmp_object->short_description);
           result_space = (float)(tmp_object->space) / (float)(tmp_object->value[3]);
           if (result_space < 0.25)
              sprintf(buf, "%s%s.\n", buf, "almost empty");
           else if (result_space < 0.5)
              sprintf(buf, "%s%s.\n", buf, "around one quarter full");
           else if (result_space < 0.75)
              sprintf(buf, "%s%s.\n", buf, "around half full");
           else if (result_space < 1.0)
              sprintf(buf, "%s%s.\n", buf, "around three quarter full");
           else
              sprintf(buf, "%s%s.\n", buf, "almost full");

          send_to_char(buf, ch);
        }
      }
*/
      if (GET_ITEM_TYPE(tmp_object) != ITEM_DRINKCON)
      {
        if (OBJ_WORN(tmp_object) || OBJ_CARRIED(tmp_object))
        {

          if (tmp_object->weight < 0)
	          percent = 0;
	        else
	          percent = (int) (100 * tmp_object->weight / tmp_object->value[0]);
	      
      	  percent = BOUNDED(1, (int) percent, 100);
  	  
    	    if (percent > 99)
	          sprintf(buf2, "&nis full!");
	        else if (percent > 80)
	          sprintf(buf2, "&nis stuffed with items.");
	        else if (percent > 60)
	          sprintf(buf2, "&nis about three-quarters full.");
	        else if (percent > 40)
	          sprintf(buf2, "&nis about halfway full.");
	        else if (percent > 30) 
	          sprintf(buf2, "&nis partially filled.");
	        else if (percent > 10)
	          sprintf(buf2, "&ncan hold a lot more.");
	        else
	          sprintf(buf2, "&nis as good as empty.");

          sprintf(buf,
                  "%s&n can hold around %d pounds, and %s\n",
                  tmp_object->short_description,
                  tmp_object->value[0] + number(-(tmp_object->value[0] >> 1),
                                                tmp_object->value[0] >> 1), buf2);
          send_to_char(buf, ch);
          
          sprintf(buf, "%s &ncurrently contains:\n", tmp_object->short_description);
          send_to_char(buf, ch);
        }
        else
        {
          sprintf(buf, "%s&n currently contains:\n",
                  tmp_object->short_description);
          send_to_char(buf, ch);
        }
      }
      sprintf(buf, "in %s", argument);
      do_look(ch, buf, -4);
            
    }
    else if (GET_ITEM_TYPE(tmp_object) == ITEM_CORPSE)
    {
      sprintf(buf, "in %s", argument);
      do_look(ch, buf, -4);
    }
  }
}

/*
 * almost completely rewritten to handle various 'vis_modes'. JAB
 */

void do_exits(P_char ch, char *argument, int cmd)
{
  if( !ch->desc || (ch->in_room == NOWHERE) )
  {
    return;
  }

  if( GET_STAT(ch) < STAT_SLEEPING )
  {
    return;
  }
  else if (GET_STAT(ch) == STAT_SLEEPING)
  {
    send_to_char("Try opening your eyes first.\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_BLIND))
  {
    send_to_char("You can't see a damn thing, you're blinded!\n", ch);
    return;
  }

  // Handles all visibility modes.
  show_exits_to_char(ch, ch->in_room, 2);
}


void show_vnums(P_char ch)
{
  char buff[MAX_STRING_LENGTH];
  
  int last_top = 0;
  
  int unused_vnums = 0;
  
  struct zone_data *zone;
  
  send_to_char("  #         Name                                           From -   To  | Empty vnums\n", ch);
  send_to_char("-------------------------------------------------------------------------------------\n", ch);
  
  for( int z = 1; z <= top_of_zone_table; z++ )
  {
    struct zone_data *zone = &zone_table[z];
    
    int bottom = world[zone->real_bottom].number;
    int top = world[zone->real_top].number;
    
    int next_bottom;
    
    if( z < top_of_zone_table )
    {
      next_bottom = world[zone_table[z+1].real_bottom].number;
    }
    else
    {
      next_bottom = bottom;
    }
    
    if( top < next_bottom-1 )
    {
      int diff = next_bottom - top - 1;
      unused_vnums += diff;
      
      sprintf(buff, "[%4d](%3d) %s %6d-%6d | &+G%d\n",
              zone_table[z].number,
              z,
              pad_ansi(zone->name, 45).c_str(),
              world[zone->real_bottom].number,
              world[zone->real_top].number,
              diff);
      
    }
    else
    {
      sprintf(buff, "[%4d](%3d) %s %6d-%6d\n",
              zone_table[z].number,
              z,
              pad_ansi(zone->name, 45).c_str(),
              world[zone->real_bottom].number,
              world[zone->real_top].number);
    }
    
    send_to_char(buff, ch);
  }
  
  sprintf(buff, "Unused room vnums: %d\n", unused_vnums);
  send_to_char(buff, ch);
}

#define WORLD_STATS    0
#define WORLD_ZONES    1
#define WORLD_EVENTS   2
#define WORLD_ROOMS    3
#define WORLD_OBJECTS  4
#define WORLD_MOBILES  5
#define WORLD_DEBUG    6
#define WORLD_VNUMS    7
#define WORLD_QUESTS   9
#define WORLD_CARGO   10
#define WORLD_DEBUG_E 11
#define MAX_WORLD     11

const char *world_keywords[MAX_WORLD+2] = {
  "stats",
  "zones",
  "events",
  "rooms",
  "objects",
  "mobiles",
  "debug",
  "vnums",
  "quests",
  "cargo",
  "debug_events",
  "\n"
};

const int world_values[] = {
  WORLD_STATS,
  WORLD_ZONES,
  WORLD_EVENTS,
  WORLD_ROOMS,
  WORLD_OBJECTS,
  WORLD_MOBILES,
  WORLD_DEBUG,
  WORLD_VNUMS,
  WORLD_QUESTS,
  WORLD_CARGO,
  WORLD_DEBUG_E,
  -1
};

extern const char *get_function_name(void *);

void do_world(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH], buff[MAX_STRING_LENGTH], buff2[MAX_STRING_LENGTH];
  char     arg[MAX_INPUT_LENGTH];
  long     ct, diff_time;
  char    *tmstr;
  int      count, i, tmp, choice, zone_count, world_index, room_count, length;
  struct zone_data *z_num = &zone_table[world[ch->in_room].zone];
  P_obj    t_obj;
  P_char   t_mob;


  if( IS_NPC(ch) )
    return;
  if( argument && *argument )
  {
    argument = one_argument(argument, arg);
    world_index = search_block(arg, world_keywords, FALSE);
  }
  else
    world_index = -1;

  if (world_index == -1 || (!IS_TRUSTED(ch) && (choice = world_values[world_index]) > WORLD_ZONES) )
  {
    sprintf(buff, "Syntax: world < ");
    for (i = 0; world_values[i] != (IS_TRUSTED(ch) ? -1 : WORLD_ZONES + 1); i++)
    {
      if (i)
        sprintf(buff + strlen(buff), ", ");
      sprintf(buff + strlen(buff), "%s", world_keywords[i]);
    }
    sprintf(buff + strlen(buff), " >\n");
    send_to_char(buff, ch);
    if (!argument || !*argument)
      send_to_char("This server compiled at " __TIME__ " " __DATE__ ".\n", ch);
    return;
  }
  switch (world_values[world_index])
  {
  case WORLD_STATS:
    ct = time(0);
    tmstr = asctime(localtime(&ct));
    *(tmstr + strlen(tmstr) - 1) = '\0';
    sprintf(buf, "Current time is: %s (GMT)\n", tmstr);
    send_to_char(buf, ch);

    diff_time = ct - boot_time;
    sprintf(buf, "Time elapsed since boot-up: %ldH %ldM %ldS\n\n",
            diff_time / 3600, (diff_time / 60) % 60, diff_time % 60);
    send_to_char(buf, ch);

    sprintf(buf, "Total number of zones in world:        %5d\n",
            top_of_zone_table + 1);
    send_to_char(buf, ch);
    sprintf(buf, "Total number of real rooms in world:     %4d\n",
            top_of_world + 1);
    send_to_char(buf, ch);
    sprintf(buf, "Total number of virtual rooms in world:  10452396\n");
    send_to_char(buf, ch);
    sprintf(buf, "&+WGrand total number of rooms in world:    %4d\n\n",
            top_of_world + 10452397);
    send_to_char(buf, ch);

    if (IS_TRUSTED(ch))
    {
      sprintf(buf, "Total number of different mobiles:       %5d\n",
              top_of_mobt + 1);
      send_to_char(buf, ch);
    }
    for (i = 0, count = 0; i <= top_of_mobt;
         i++, count += mob_index[i].number) ;
    sprintf(buf, "&+WTotal number of living mobiles:          %5d\n", count);
    send_to_char(buf, ch);

    if (IS_TRUSTED(ch))
    {
      sprintf(buf, "Total number of different objects:       %5d\n",
              top_of_objt + 1);
      send_to_char(buf, ch);
    }
    for (i = 0, count = 0; i <= top_of_objt;
         i++, count += obj_index[i].number) ;
    sprintf(buf, "&+WTotal number of existing objects:        %5d\n", count);
    send_to_char(buf, ch);

    sprintf(buf, "Total number of shops:                 %5d\n",
            number_of_shops);
    send_to_char(buf, ch);
    sprintf(buf, "Total number of quests:                %5d\n\n",
            number_of_quests);
    send_to_char(buf, ch);

    if (IS_TRUSTED(ch))
    {
/* Didn't do this.. guess needs to be handled in mmc somehow.
      sprintf(buf, "Total allocated events:                  %5ld\n",
              ne_event_counter );
*/
      send_to_char(buf, ch);
      sprintf(buf, "Total number of pending events:          %5ld\n\n",
              ne_event_counter );
      send_to_char(buf, ch);

      sprintf(buf, "Number of active sockets:              %5d\n",
              used_descs);
      send_to_char(buf, ch);
      sprintf(buf, "Max sockets used this boot:            %5d\n", max_descs);
      send_to_char(buf, ch);
    }
    sprintf(buf, "Maximum allowable sockets:             %5d\n", avail_descs);
    send_to_char(buf, ch);

      sprintf(buf, "Total MB sent since boot:                %5.4f\n",
              (float) sentbytes /  1048576.00);
      send_to_char(buf, ch);

      sprintf(buf, "Total MB received since boot:             %5.4f\n",
              (float) recivedbytes /  1048576.00);
      send_to_char(buf, ch);

    break;

  case WORLD_ZONES:

//    *buff = '\0';
    if (GET_LEVEL(ch) < MINLVLIMMORTAL)
    {
      send_to_char("Name\n&+W-----------------------------------\n", ch);
      for (zone_count = 1; zone_count <= top_of_zone_table; zone_count++)
      {
        sprintf(buff, " %s\n", pad_ansi(zone_table[zone_count].name, 30).c_str());
        send_to_char(buff, ch);
      }
    }
    else
    {
      send_to_char
        ("Zone        Name                                       First vnum  Age  AvgLevel\n",
         ch);
      send_to_char
        ("&+W----------------------------------------------------------------------------------\n", ch);
      for (zone_count = 1; zone_count <= top_of_zone_table; zone_count++)
      {
        sprintf(buff,
                "[%4d](%3d) %-40s %7d %3d/%3d %2d\n",
                zone_table[zone_count].number,
                zone_count, 
                pad_ansi(zone_table[zone_count].name, 45).c_str(),
                world[zone_table[zone_count].real_bottom].number,
                zone_table[zone_count].age,
                zone_table[zone_count].lifespan,
                zone_table[zone_count].avg_mob_level);
        send_to_char(buff, ch);
      }
    }
//    strcat(buff, "\n");
//    page_string(ch->desc, buff, 1);
    break;

  case WORLD_EVENTS:
    one_argument(argument, arg);
    show_world_events(ch, arg);
    break;

  case WORLD_ROOMS:

    *buff = '\0';
    length = 0;
    i = world[ch->in_room].zone;
    send_to_char("R-Num   V-Num   Room-Name\n", ch);
    if( !argument || !*argument )
    {
      for (room_count = 0; room_count <= top_of_world; room_count++)
      {
        if (world[room_count].zone == i)
        {
          sprintf(buf, "%5d  %6d  %-s\n", room_count, world[room_count].number, world[room_count].name);
          if( (strlen(buf) + length + 40) < MAX_STRING_LENGTH )
          {
            strcat(buff, buf);
            length += strlen(buf);
          }
          else
          {
            strcat(buff, "Too many rooms to list...\n");
            break;
          }
        }
      }
    }
    else
    {
      for (room_count = 0; room_count <= top_of_world; room_count++)
      {
        if( world[room_count].zone == i )
        {
          if( !isname( argument, strip_ansi(world[room_count].name).c_str() ) )
            continue;
          sprintf(buf, "%5d  %6d  %-s\n", room_count, world[room_count].number, world[room_count].name);
          if( (strlen(buf) + length + 40) < MAX_STRING_LENGTH )
          {
            strcat(buff, buf);
            length += strlen(buf);
          }
          else
          {
            strcat(buff, "Too many rooms to list...\n");
            break;
          }
        }
      }
    }
    strcat(buff, "\n");
    page_string(ch->desc, buff, 1);
    break;

  case WORLD_OBJECTS:
    send_to_char("V-Num   Count  Limit  Name\n", ch);
    *buff = '\0';
    length = 0;
    for (i = 0; i <= top_of_objt; i++)
      if ((obj_index[i].virtual_number >= world[z_num->real_bottom].number) &&
          (obj_index[i].virtual_number <= world[z_num->real_top].number))
      {
        /*
         * easier to just load one, and free it, than load only
         * ones that aren't already in game.
         */
        if ((t_obj = read_object(obj_index[i].virtual_number, VIRTUAL)))
        {
          sprintf(buf, "%6d  %5d  %5d  %-s\n",
                  obj_index[i].virtual_number, obj_index[i].number - 1,
                  obj_index[i].limit,
                  (t_obj->short_description) ?
                  t_obj->short_description : "None");
          extract_obj(t_obj, FALSE);
          t_obj = NULL;
          if ((strlen(buf) + length + 40) < MAX_STRING_LENGTH)
          {
            strcat(buff, buf);
            length += strlen(buf);
          }
          else
          {
            strcat(buff, "Too many objects to list...\n");
            break;
          }
        }
        else
          logit(LOG_DEBUG, "do_world(): obj %d not loadable",
                obj_index[i].virtual_number);
      }
    strcat(buff, "\n");
    page_string(ch->desc, buff, 1);
    break;

  case WORLD_MOBILES:
    send_to_char("V-Num   Count  Limit  Name\n", ch);
    *buff = '\0';
    length = 0;
    count = 0;
    for (i = 0; i <= top_of_mobt; i++)
      if ((mob_index[i].virtual_number >= world[z_num->real_bottom].number) &&
          (mob_index[i].virtual_number <= world[z_num->real_top].number))
      {
        /*
         * easier to just load one, and free it, than load only
         * ones that aren't already in game. Only gods can do this
         * so a little load doesn't matter. JAB
         */
        if ((t_mob = read_mobile(mob_index[i].virtual_number, VIRTUAL)))
        {
          if (IS_SET(t_mob->specials.act, ACT_SPEC))
            REMOVE_BIT(t_mob->specials.act, ACT_SPEC);
          char_to_room(t_mob, ch->in_room, -2);
          sprintf(buf, "%6d  %5d  %5d  %-s\n",
                  mob_index[i].virtual_number, mob_index[i].number - 1,
                  mob_index[i].limit,
                  (t_mob->player.short_descr) ?
                  t_mob->player.short_descr : "None");
          count++;
          if (t_mob)
          {
            extract_char(t_mob);
            t_mob = NULL;
          }
          else
          {
            logit(LOG_EXIT, "GLITCH 1");
            raise(SIGSEGV);
          }
          if ((strlen(buf) + length + 40) < MAX_STRING_LENGTH)
          {
            strcat(buff, buf);
            length += strlen(buf);
          }
          else
          {
            strcat(buff, "Too many mobiles to list...\n");
            break;
          }
        }
        else
          logit(LOG_DEBUG, "do_world(): mob %d not loadable",
                mob_index[i].virtual_number);
      }
    strcat(buff, "\n");
    sprintf(buf + strlen(buf), "&+LTotal mobs for this zone loaded:&n %d\n",
            count);
    page_string(ch->desc, buff, 1);
    break;

  case WORLD_VNUMS:
    show_vnums(ch);
    break;
  
  case WORLD_CARGO:
    one_argument(argument, arg);
    do_world_cargo(ch, arg);
    break;
      
  case WORLD_QUESTS:
    send_to_char("V-Num   Count  Limit  Name\n", ch);
    *buff = '\0';
    length = 0;
    count = 0;

    for (i = 0; i <= top_of_mobt; i++)
      if ((mob_index[i].virtual_number >= world[z_num->real_bottom].number) &&
          (mob_index[i].virtual_number <= world[z_num->real_top].number) && 
          (mob_index[i].qst_func))
      {
	    
		    bool real_quest = FALSE;
        
        if ((t_mob = read_mobile(mob_index[i].virtual_number, VIRTUAL)))
        {
          if (IS_SET(t_mob->specials.act, ACT_SPEC))
            REMOVE_BIT(t_mob->specials.act, ACT_SPEC);
          char_to_room(t_mob, ch->in_room, -2);
		  
		      for (tmp = 0; quest_index[tmp].quester; tmp++)
          {
		        if (quest_index[tmp].quester == i)
			      {
			        if (quest_index[tmp].quest_complete)
			        real_quest = TRUE;
			        break;
			      }
		      }
		  
		      if (real_quest)
            sprintf(buf, " %6d  %5d  %5d  %-s\n",
		              mob_index[i].virtual_number, mob_index[i].number - 1,
                  mob_index[i].limit,
                  (t_mob->player.short_descr) ?
                  t_mob->player.short_descr : "None");
          else
            sprintf(buf, "!%6d  %5d  %5d  %-s\n",
		              mob_index[i].virtual_number, mob_index[i].number - 1,
                  mob_index[i].limit,
                  (t_mob->player.short_descr) ?
                  t_mob->player.short_descr : "None");
          
          count++;
          if (t_mob)
          {
            extract_char(t_mob);
            t_mob = NULL;
          }
          else
          {
            logit(LOG_EXIT, "GLITCH 1");
            raise(SIGSEGV);
          }
          if ((strlen(buf) + length + 40) < MAX_STRING_LENGTH)
          {
            strcat(buff, buf);
            length += strlen(buf);
          }
          else
          {
            strcat(buff, "Too many quest mobiles to list...\n");
            break;
          }
        }
        else
          logit(LOG_DEBUG, "do_world(): mob %d not loadable",
                mob_index[i].virtual_number);
      }
    strcat(buff, "\n");
    sprintf(buf + strlen(buf), "&+LTotal quest mobs for this zone loaded:&n %d\n",
            count);
    page_string(ch->desc, buff, 1);
    break;
  case WORLD_DEBUG:

/*
 * sprintf(buff + strlen(buff), "Pulses: deficit:  %ld seconds.\n",
 * (long)time_deficit.tv_sec);
 */
  case WORLD_DEBUG_E:
    one_argument(argument, arg);
    if( *arg )
    {
      if( !strcmp(arg, "once") )
      {
        check_nevents();
        return;
      }
      else
      {
        send_to_char( "&+YValid options are 'world debug_events' and 'world debug_events once'.&n\n\r", ch );
        return;
      }
    }
    debug_event_list = !debug_event_list;
    if( debug_event_list )
      send_to_char( "&+YTurned event list debugging on!\n\r", ch );
    else
      send_to_char( "&+YTurned event list debugging off!\n\r", ch );
    break;
  default:
    send_to_char( "&=LRYou should never see this!\n\r", ch );
    break;
  }
}

/*
 *      SAM 5-94, re-written to make a bit nicer, show more info!
 */
void do_attributes(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH];
  char     buffer[MAX_STRING_LENGTH];
  int      t_val, h, w;

  if (ch == NULL)
  {
    logit(LOG_STATUS, "do_attributes passed NULL ch pointer");
    return;
  }
  if (IS_MORPH(ch))
  {
    send_to_char("Wouldn't you like to know?\n", ch);
    return;
  }
  /* header */
  sprintf(buf, "\n\t\t&+gCharacter attributes for &+G%s\n\n&n", GET_NAME(ch));
  send_to_char(buf, ch);

  /* level, race, class */
  sprintf(buf, "&+cLevel: &+Y%d   &n&+cRace:&n %s   &+cClass:&n %s &n\n",
          GET_LEVEL(ch), race_to_string(ch), get_class_string(ch, buffer));
  send_to_char(buf, ch);

  if (IS_PC(ch))
  {
    h = (int) GET_HEIGHT(ch);
    w = (int) GET_WEIGHT(ch);
  }
  else
    h = w = -1;

  /* age, height, weight */
  sprintf(buf, "&+cAge: &+Y%d &n&+yyrs &+L/ &+Y%d &n&+ymths  "
          "&+cHeight: &+Y%d &n&+yinches "
          "&+cWeight: &+Y%d &n&+ylbs  &+cSize: &+Y%s\n\n", 
          GET_AGE(ch), age(ch).month, h, w, size_types[GET_ALT_SIZE(ch)]);
  send_to_char(buf, ch);

  /* Stats */
  if (GET_LEVEL(ch) >= 1)
  {
    if (IS_TRUSTED(ch) || GET_LEVEL(ch) >= 20)
    {
      /* this is ugly, because of new racial stat mods.  JAB */
#if 0
      if (GET_C_STR(ch) > stat_factor[(int) GET_RACE(ch)].Str)
        sprintf(buf, "&+cSTR: &+Y***&n");
      else
        sprintf(buf, "&+cSTR: &+Y%3d&n",
                MAX(1,
                    (int) ((GET_C_STR(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Str) + .55)));

      if (GET_C_AGI(ch) > stat_factor[(int) GET_RACE(ch)].Agi)
        sprintf(buf + strlen(buf), "  &+cAGI: &+Y***&n");
      else
        sprintf(buf + strlen(buf), "  &+cAGI: &+Y%3d&n",
                MAX(1,
                    (int) ((GET_C_AGI(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Agi) + .55)));

      if (GET_C_DEX(ch) > stat_factor[(int) GET_RACE(ch)].Dex)
        sprintf(buf + strlen(buf), "  &+cDEX: &+Y***&n");
      else
        sprintf(buf + strlen(buf), "  &+cDEX: &+Y%3d&n",
                MAX(1,
                    (int) ((GET_C_DEX(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Dex) + .55)));

      if (GET_C_CON(ch) > stat_factor[(int) GET_RACE(ch)].Con)
        sprintf(buf + strlen(buf), "  &+cCON: &+Y***&n");
      else
        sprintf(buf + strlen(buf), "  &+cCON: &+Y%3d&n",
                MAX(1,
                    (int) ((GET_C_CON(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Con) + .55)));

      if (GET_C_LUK(ch) > stat_factor[(int) GET_RACE(ch)].Luck)
        sprintf(buf + strlen(buf), "  &+cLUCK: &+Y***&n\n");
      else
        sprintf(buf + strlen(buf), "  &+cLUCK: &+Y%3d&n\n",
                MAX(1,
                    (int) ((GET_C_LUK(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Luck) + .55)));

      if (GET_C_POW(ch) > stat_factor[(int) GET_RACE(ch)].Pow)
        sprintf(buf + strlen(buf), "&+cPOW: &+Y***&n");
      else
        sprintf(buf + strlen(buf), "&+cPOW: &+Y%3d&n",
                MAX(1,
                    (int) ((GET_C_POW(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Pow) + .55)));

      if (GET_C_INT(ch) > stat_factor[(int) GET_RACE(ch)].Int)
        sprintf(buf + strlen(buf), "  &+cINT: &+Y***&n");
      else
        sprintf(buf + strlen(buf), "  &+cINT: &+Y%3d&n",
                MAX(1,
                    (int) ((GET_C_INT(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Int) + .55)));

      if (GET_C_WIS(ch) > stat_factor[(int) GET_RACE(ch)].Wis)
        sprintf(buf + strlen(buf), "  &+cWIS: &+Y***&n");
      else
        sprintf(buf + strlen(buf), "  &+cWIS: &+Y%3d&n",
                MAX(1,
                    (int) ((GET_C_WIS(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Wis) + .55)));

      if (GET_C_CHA(ch) > stat_factor[(int) GET_RACE(ch)].Cha)
        sprintf(buf + strlen(buf), "  &+cCHA: &+Y***&n\n\n");
      else
        sprintf(buf + strlen(buf), "  &+cCHA: &+Y%3d&n\n\n",
                MAX(1,
                    (int) ((GET_C_CHA(ch) * 100 /
                            stat_factor[(int) GET_RACE(ch)].Cha) + .55)));
#endif
/*

      sprintf(buf, "&+cSTR: &+Y%3d&n",
              MAX(1,
                  (int) ((GET_C_STR(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Str) + .55)));
      sprintf(buf + strlen(buf), "  &+cAGI: &+Y%3d&n",
              MAX(1,
                  (int) ((GET_C_AGI(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Agi) + .55)));
      sprintf(buf + strlen(buf), "  &+cDEX: &+Y%3d&n",
              MAX(1,
                  (int) ((GET_C_DEX(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Dex) + .55)));
      sprintf(buf + strlen(buf), "  &+cCON: &+Y%3d&n",
              MAX(1,
                  (int) ((GET_C_CON(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Con) + .55)));
      sprintf(buf + strlen(buf), "  &+cLUCK: &+Y%3d&n\n",
              MAX(1,
                  (int) ((GET_C_LUK(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Luck) + .55)));
      sprintf(buf + strlen(buf), "&+cPOW: &+Y%3d&n",
              MAX(1,
                  (int) ((GET_C_POW(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Pow) + .55)));
      sprintf(buf + strlen(buf), "  &+cINT: &+Y%3d&n",
              MAX(1,
                  (int) ((GET_C_INT(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Int) + .55)));
      sprintf(buf + strlen(buf), "  &+cWIS: &+Y%3d&n",
              MAX(1,
                  (int) ((GET_C_WIS(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Wis) + .55)));
      sprintf(buf + strlen(buf), "  &+cCHA: &+Y%3d&n\n\n",
              MAX(1,
                  (int) ((GET_C_CHA(ch) * 100 /
                          stat_factor[(int) GET_RACE(ch)].Cha) + .55)));
*/

//drannak new way
  char o_buf[MAX_STRING_LENGTH] = "", buf2[MAX_STRING_LENGTH] = "";
  strcat(o_buf, "  &+GActual &n(&+gBase&n)     &+GActual &n(&+gBase&n)\n");

  int i, i2, i3;
  for (i = i3 = 0; i < MAX_WEAR; i++)
    if(ch->equipment[i])
      i3++;
  i2 = (int) (GET_HEIGHT(ch));
  i = (int) (i2 / 12);
  i2 -= i * 12;

  sprintf(buf, "&+YStr: &n%3d&+Y (&n%3d&+Y)    Pow: &n%3d&+Y (&n%3d&+Y)\n",
    GET_C_STR(ch), MAX(1, (int) ((GET_C_STR(ch) * 100 /
    stat_factor[(int) GET_RACE(ch)].Str) + .55)), GET_C_POW(ch), MAX(1,
    (int) ((GET_C_POW(ch) * 100 / stat_factor[(int) GET_RACE(ch)].Pow) + .55)));
  strcat(o_buf, buf);

  sprintf(buf, "&+YDex: &n%3d&+Y (&n%3d&+Y)    Int: &n%3d&+Y (&n%3d&+Y)   \n",
    GET_C_DEX(ch), MAX(1, (int) ((GET_C_DEX(ch) * 100 /
    stat_factor[(int) GET_RACE(ch)].Dex) + .55)), GET_C_INT(ch), MAX(1,
    (int) ((GET_C_INT(ch) * 100 / stat_factor[(int) GET_RACE(ch)].Int) + .55)));
  strcat(o_buf, buf);

  sprinttype(GET_ALT_SIZE(ch), size_types, buf2);
  sprintf(buf, "&+YAgi: &n%3d&+Y (&n%3d&+Y)    Wis: &n%3d&+Y (&n%3d&+Y)\n",
    GET_C_AGI(ch), MAX(1, (int) ((GET_C_AGI(ch) * 100 /
    stat_factor[(int) GET_RACE(ch)].Agi) + .55)), GET_C_WIS(ch), MAX(1,
    (int) ((GET_C_WIS(ch) * 100 / stat_factor[(int) GET_RACE(ch)].Wis) + .55)));
  strcat(o_buf, buf);

  sprintf(buf, "&+YCon: &n%3d&+Y (&n%3d&+Y)    Cha: &n%3d&+Y (&n%3d&+Y)   Luc: &n%3d&+Y (&n%3d&+Y)\n"
               "&+cEquipped Items: &n%3d&+Y     &+cCarried weight:&n%5d\n\n",
    GET_C_CON(ch), MAX(1, (int) ((GET_C_CON(ch) * 100 /
    stat_factor[(int) GET_RACE(ch)].Con) + .55)), GET_C_CHA(ch), MAX(1,
    (int) ((GET_C_CHA(ch) * 100 / stat_factor[(int) GET_RACE(ch)].Cha) + .55)), 
    GET_C_LUK(ch), MAX(1, (int) ((GET_C_LUK(ch) * 100 /
    stat_factor[(int) GET_RACE(ch)].Luck) + .55)), i3, IS_CARRYING_W(ch));
  strcat(o_buf, buf);

   /* sprintf(buf,
            "&+YKar: &n%3d&+Y (&n%3d&+Y)    Luc: &n%3d&+Y (&n%3d&+Y)    Carried Items: &n%3d&+Y   Max Carry Weight:&n%5d\n",
            GET_C_KARMA(k), k->base_stats.Karma, GET_C_LUK(k),
            k->base_stats.Luck, IS_CARRYING_N(k), CAN_CARRY_W(k));
    strcat(o_buf, buf);*/

    i = GET_C_STR(ch) + GET_C_DEX(ch) + GET_C_AGI(ch) + GET_C_CON(ch) +
      GET_C_POW(ch) + GET_C_INT(ch) + GET_C_WIS(ch) + GET_C_CHA(ch);

    i2 =
      ch->base_stats.Str + ch->base_stats.Dex + ch->base_stats.Agi +
      ch->base_stats.Con + ch->base_stats.Pow + ch->base_stats.Int +
      ch->base_stats.Wis + ch->base_stats.Cha;

 /*   sprintf(buf,
            "&+YAvg: &n%3d&+Y (&n%3d&+Y)  Total mod: (&n%3d&+Y)              Load modifer: &n%3d\n\n",
            (int) (i / 8), (int) (i2 / 8), (i - i2), load_modifier(k));
    strcat(o_buf, buf);*/
//    send_to_char(o_buf, ch);
	sprintf(buf, "%s", o_buf);


    }
    else                        /*if (GET_LEVEL(ch) >= 10) */
    {
      sprintf(buf,
              "&+cSTR: &+Y%-15s&n  &+cAGI: &+Y%-15s&n   &+cDEX: &+Y%s\n"
              "&+cPOW: &+Y%-15s&n  &+cINT: &+Y%-15s&n   &+cWIS: &+Y%s\n"
              "&+cCON: &+Y%-15s&n  &+cCHA: &+Y%-15s&n  &+cLUCK: &+Y%s\n\n",
              stat_to_string2((int)
                              ((GET_C_STR(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Str) + .55)),
              stat_to_string2((int)
                              ((GET_C_AGI(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Agi) + .55)),
              stat_to_string2((int)
                              ((GET_C_DEX(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Dex) + .55)),
              stat_to_string2((int)
                              ((GET_C_POW(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Pow) + .55)),
              stat_to_string2((int)
                              ((GET_C_INT(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Int) + .55)),
              stat_to_string2((int)
                              ((GET_C_WIS(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Wis) + .55)),
              stat_to_string2((int)
                              ((GET_C_CON(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Con) + .55)),
              stat_to_string2((int)
                              ((GET_C_CHA(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Cha) + .55)),
              stat_to_string2((int)
                              ((GET_C_LUK(ch) * 100 /
                                stat_factor[(int) GET_RACE(ch)].Luck) +
                               .55)));
    }
  }
  else
  {
    sprintf(buf,
            "&+cSTR: &+Y%s\t&n&+cAGI: &+Y%s\t&n&+cDEX: &+Y%s\t&n&+cCON: &+Y%s\t&n&+cLUCK: &+Y%s&n\n"
            "&+cPOW: &+Y%s\t&n&+cINT: &+Y%s\t&n&+cWIS: &+Y%s\t&n&+cCHA: &+Y%s&n\n\n",
            stat_to_string1((int)
                            ((GET_C_STR(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Str) + .55)),
            stat_to_string1((int)
                            ((GET_C_AGI(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Agi) + .55)),
            stat_to_string1((int)
                            ((GET_C_DEX(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Dex) + .55)),
            stat_to_string1((int)
                            ((GET_C_CON(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Con) + .55)),
            stat_to_string1((int)
                            ((GET_C_LUK(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Luck) + .55)),
            stat_to_string1((int)
                            ((GET_C_POW(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Pow) + .55)),
            stat_to_string1((int)
                            ((GET_C_INT(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Int) + .55)),
            stat_to_string1((int)
                            ((GET_C_WIS(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Wis) + .55)),
            stat_to_string1((int)
                            ((GET_C_CHA(ch) * 100 /
                              stat_factor[(int) GET_RACE(ch)].Cha) + .55)));
  }
  send_to_char(buf, ch);

  /* Armor Class */

  t_val = calculate_ac(ch);
/*  if (GET_LEVEL(ch) >= 25) { */
//    sprintf(buf, "&+cArmor Class: &+Y%d&n  &+y(100 to -100)\n", t_val);
      if(t_val >= 0)
      sprintf(buf, "&+cArmor Points: &+Y%d&+c  Increases melee damage taken by &+Y%.1f&+y%%&n\n", t_val, (double)(t_val * 0.10) );
      else
      sprintf(buf, "&+cArmor Points: &+Y%d&+c  Reduces melee damage taken by &+Y%.1f&n&+y%%&n \n", t_val, (double)(t_val * -0.10) );
  send_to_char(buf, ch);
/*  statupdate2013 stats - drannak */

  if(IS_PC(ch))
	{

    // At 100 int : 5% crit, at 200 int : 28% crit
    int critroll = CRITRATE(ch);
//(GET_C_INT(ch) - 100)/5 + 8, MAX( critroll, 8);

	  // Calm Chance:
//    int rolmod = ((GET_C_CHA(ch) > 160) ? 4 : (GET_C_CHA(ch) < 80) ? 9 : 7);
    int calmroll = CALMCHANCE(ch);
// (GET_C_CHA(ch) / rolmod);

  	// Magic Res:
    int magicres = MAGICRES(ch);
// BOUNDED( 0, (GET_C_WIS(ch) - 110)/2, 75);

  	//vamp percentage:
    int vamppct = (100 * VAMPPERCENT(ch));

    // Magic Dam:
    // 121 str -> 10%, 141 str -> 20%, 181 str -> 30%.
//    double modifierx = GET_C_STR(ch) - 120;
//    double remod = 100 + (modifierx < 1) ? 0 : (modifierx < 21) ? 10 : (modifierx < 61) ? 20 : 30;
    int remod = MAGICDAMBONUS(ch);

    // Bloodlust:
    int bloodlust = 0;
    if(affected_by_spell(ch, TAG_BLOODLUST))
    {
      struct affected_type *findaf, *next_af;
      for( findaf = ch->affected; findaf; findaf = next_af )
      {
        next_af = findaf->next;
        if( findaf && findaf->type == TAG_BLOODLUST )
        {
          bloodlust = ((findaf->modifier * 10));
          break;
        }
      }
    }

/*
  sprintf(buf, "&+cMelee Critical Percentage(int): &+Y%d%   &n&+cMax Spell Damage Reduction Percent(wis): &+Y%d%\r\n",
    critroll, (int)modifier);
  send_to_char(buf, ch);
*/

    sprintf(buf, "&+cMelee Critical Percentage&n(&+Mint&n): &+Y%d%%\r\n", critroll);
    send_to_char(buf, ch);
    sprintf(buf, "&+cMax Spell Damage Reduction Percent&n(&+cwis&n): &+Y%d%%\r\n", magicres);
    send_to_char(buf, ch);
    sprintf(buf, "&+cCalming Chance&n(&+Ccha&n): &+Y%d%% \r\n", calmroll);
    send_to_char(buf, ch);

/*
  sprintf(buf, "&+cHP Vamp Cap Percentage&n(&+mpow&n): &+Y%d%     &n&+cSpell Damage Modifier(str): &+Y%d%\r\n",
    (int)vamppct, (int)remod);
*/
    sprintf(buf, "&+cHP Vamp Cap Percentage&n(&+mpow&n): &+Y%d%% \r\n", vamppct);
    send_to_char(buf, ch);
    sprintf(buf, "&+cSpell Damage Modifier&n(&+rstr&n): &+Y%d%% \r\n", remod);
    send_to_char(buf, ch);
    sprintf(buf, "&+rBloodlust &+cDamage Bonus&n(&+yvs mob only&n): &+Y%d%% \r\n", bloodlust);
    send_to_char(buf, ch);
	}



//  sprintf(buf, "&+cArmor Class: &+Y%s\n", ac_to_string(t_val));
//  send_to_char(buf, ch);

  if (IS_PC(ch) && GET_CLASS(ch, CLASS_MONK))
    MonkSetSpecialDie(ch);

  /*
   * Hitroll, Damroll
   */

/*  if (GET_LEVEL(ch) >= 25) {*/
  if (IS_TRUSTED(ch) || GET_LEVEL(ch) >= 25)
  {
    sprintf(buf, "&+cHitroll: &+Y%d   &n&+cDamroll: &+Y%d",
            GET_HITROLL(ch) + str_app[STAT_INDEX(GET_C_STR(ch))].tohit,
            GET_DAMROLL(ch) + str_app[STAT_INDEX(GET_C_STR(ch))].todam);
    if (IS_NPC(ch) || GET_CLASS(ch, CLASS_MONK))
      sprintf(buf + strlen(buf), "   Barehand Damage: %dd%d",
              ch->points.damnodice, ch->points.damsizedice);
  }
  else
  {
    sprintf(buf, "&+cHitroll: &+Y%s&n\t&+cDamroll: &+Y%s",
            hitdam_roll_to_string(GET_HITROLL(ch)),
            hitdam_roll_to_string(GET_DAMROLL(ch)));
    if (IS_NPC(ch) || GET_CLASS(ch, CLASS_MONK))
    {
      t_val = ch->points.damnodice * ch->points.damsizedice;
      sprintf(buf + strlen(buf), "   &n&+cApprox Barehand Damage: &+Y%s",
              (t_val < 8) ? "mace" :
              (t_val < 15) ? "axe" :
              (t_val < 20) ? "broadsword" :
              (t_val < 24) ? "battle axe" :
              (t_val < 28) ? "greatsword" :
              (t_val < 30) ? "two-handed" :
              (t_val < 35) ? "titan" : "artifact");
    }
  }

  strcat(buf, "\n");
  send_to_char(buf, ch);

  /*
   * Alignment
   */
/*  if (GET_LEVEL(ch) >= 25) {*/
  if (IS_TRUSTED(ch) || GET_LEVEL(ch) >= 25)
  {
    sprintf(buf, "&+cAlignment: &+Y%d  &n&+y(-1000 to 1000)\n\n",
            GET_ALIGNMENT(ch));
  }
  else
  {
    sprintf(buf, "&+cAlignment: &+Y%s\n\n",
            align_to_string(GET_ALIGNMENT(ch)));
  }
  send_to_char(buf, ch);

  /*
   * Saving throws
   */
  sprintf(buf, "&+cSaving Throws: &n&+gPAR[&+Y%s&n&+g]  FEA[&+Y%s&n&+g]\n"
          "&+g               BRE[&+Y%s&n&+g]  SPE[&+Y%s&n&+g]\n",
          save_to_string(ch, SAVING_PARA).c_str(),
          save_to_string(ch, SAVING_FEAR).c_str(),
          save_to_string(ch, SAVING_BREATH).c_str(),
          save_to_string(ch, SAVING_SPELL).c_str());
  send_to_char(buf, ch);

/*
  if (IS_PC(ch)) {
    if (GET_WIMPY(ch) > 0) {
      sprintf(buf, "&+cWimpy: &+Y%d\n\n", GET_WIMPY(ch));
    } else {
      sprintf(buf, "&+cWimpy: &+Ynot set\n\n");
    }
    send_to_char(buf, ch);
    sprintf(buf, "&+cCombat Target Location: &+Y%s\n", target_locs[ch->player.combat_target_loc]);
    send_to_char(buf, ch);
  }
*/
  /*
   * Equipment Carried
   */
  sprintf(buf, "\n&+cLoad carried: &+Y%s\n", load_to_string(ch));
  send_to_char(buf, ch);
}

void do_score(P_char ch, char *argument, int cmd)
{
  struct time_info_data playing_time;
  static char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  struct follow_type *fol;
  struct affected_type *aff;
  bool     found = FALSE;
  int      i, last, percent, secs;
  float    frags;
  char     buffer[1024];
  float    fragnum, hardcorepts = 0;
  P_nevent ne;
  P_char   tch, tch2;
  P_obj    nexus;

  if (ch == NULL)
  {
    logit(LOG_STATUS, "do_score passed NULL ch pointer");
    return;
  }
  
  /* header */
  sprintf(buf, "\n\t\t&+gScore information for&n &+G%s\n\n&n",
          IS_MORPH(ch) ? ch->player.short_descr : GET_NAME(ch));
  send_to_char(buf, ch);

  switch (GET_SEX(ch))
  {
  case 1:
    strcpy(buf2, "Male");
    break;
  case 2:
    strcpy(buf2, "Female");
    break;
  default:
    strcpy(buf2, "Neuter");
    break;
  }

  /* level, race, class */
  sprintf(buf, "Level: %d   Race: %s   Class: %s &nSex: %s\n&n",
          GET_LEVEL(ch),
          race_to_string(ch), get_class_string(ch, buffer), buf2);
  send_to_char(buf, ch);

  /* description */
#if 0
  if (ch->player.short_descr)
  {
    sprintf(buf, "Quick Description: %s\n", ch->player.short_descr);
    send_to_char(buf, ch);
  }
#endif

  /* hit pts, mana, moves */
  if (GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER))
  {
    sprintf(buf,
            "&+RHit points: %d&+r(%d)   &+MMana: %d&+m(%d)   &+YMoves: %d&+y(%d)&N\n",
            GET_HIT(ch), GET_MAX_HIT(ch),
            GET_MANA(ch), GET_MAX_MANA(ch),
            GET_VITALITY(ch), GET_MAX_VITALITY(ch));
  }
  else
  {
    sprintf(buf,
            "&+RHit points: %d&+r(%d)  &+YMoves: %d&+y(%d)&N\n",
            GET_HIT(ch), GET_MAX_HIT(ch),
            GET_VITALITY(ch), GET_MAX_VITALITY(ch));
  }
  send_to_char(buf, ch);

  /* money */
  sprintf(buf,
          "Coins carried: &+W%4d platinum&N  &+Y%4d gold&N  &n%4d silver&N  &+y%4d copper&N\n",
          GET_PLATINUM(ch), GET_GOLD(ch), GET_SILVER(ch), GET_COPPER(ch));
  send_to_char(buf, ch);
  buf[0] = 0;
  if (IS_PC(ch))
  {
    sprintf(buf,
            "Coins in bank: &+W%4d platinum&N  &+Y%4d gold&N  &n%4d silver&N  &+y%4d copper&N\n",
            GET_BALANCE_PLATINUM(ch),
            GET_BALANCE_GOLD(ch),
            GET_BALANCE_SILVER(ch), GET_BALANCE_COPPER(ch));
    send_to_char(buf, ch);
    /* justice info */
/*    found = FALSE;
    strcpy(buf, "&+YWanted in:    &N");
    for (i = 1; i <= LAST_HOME; i++)
      if ((PC_TOWN_JUSTICE_FLAGS(ch, i) == JUSTICE_IS_WANTED) &&
          !IS_TOWN_INVADER(ch, i)) {
        found = TRUE;
        strcat(buf, town_name_list[i]);
        strcat(buf, ", ");
      }
    if (found) {
      buf[strlen(buf) - 2] = '\0';
      strcat(buf, "\n");
      send_to_char(buf, ch);
    }
*/
    found = FALSE;
    strcpy(buf, "&+ROutcast from: &N");
    for (i = 1; i <= LAST_HOME; i++)
      if ((PC_TOWN_JUSTICE_FLAGS(ch, i) == JUSTICE_IS_OUTCAST) &&
          !IS_TOWN_INVADER(ch, i))
      {
        found = TRUE;
        strcat(buf, town_name_list[i]);
        strcat(buf, ", ");
      }
    if (found)
    {
      buf[strlen(buf) - 2] = '\0';
      strcat(buf, "\n");
      send_to_char(buf, ch);
    }
    /* playing time */
#ifndef EQ_WIPE
    playing_time = real_time_passed((long) ((time(0) - ch->player.time.logon) + ch->player.time.played), 0);
#else
    playing_time = real_time_passed((long) ((time(0) - ch->player.time.logon) + ch->player.time.played - EQ_WIPE), 0);
#endif
    sprintf(buf, "Playing time: %d days / %d hours/ %d minutes\n",
            playing_time.day, playing_time.hour, playing_time.minute);
    send_to_char(buf, ch);

    /* traffic info*/
    sprintf(buf, "&+wReceived data:&+y %5.4f &+wMB this session.\n", (float) (ch->only.pc->send_data / 1048576.0000 ));
    send_to_char(buf, ch);

    sprintf(buf, "&+wSend data:    &+y %5.4f &+wMB this session.\n", (float) (ch->only.pc->recived_data / 1048576.0000 ));
    send_to_char(buf, ch);



    /* compression */
    send_to_char("Compression ratio: ", ch);
    if (ch->desc && ch->desc->z_str)
    {
      sprintf(buf, "%d%%\n", compress_get_ratio(ch->desc));
      send_to_char(buf, ch);
    }
    else
      send_to_char("none\n", ch);

//    /* prestige */
//    sprintf(buf, "Prestige: %s\n", epic_prestige(ch));
//    send_to_char(buf, ch);
//    buf[0] = 0;

    /* title */
    if (GET_TITLE(ch))
      sprintf(buf, "Title: %s\n", GET_TITLE(ch));

    /* poofin/poofout for gods */
    if (IS_TRUSTED(ch))
    {
      if (ch->only.pc->poofIn == NULL)
        sprintf(buf + strlen(buf), "PoofIn:  None\n");
      else
        sprintf(buf + strlen(buf), "PoofIn:  %s\n", ch->only.pc->poofIn);

      if (ch->only.pc->poofOut == NULL)
        sprintf(buf + strlen(buf), "PoofOut: None\n");
      else
        sprintf(buf + strlen(buf), "PoofOut: %s\n", ch->only.pc->poofOut);

      if (ch->only.pc->poofInSound == NULL)
        sprintf(buf + strlen(buf), "PoofInSound:  None\n");
      else
        sprintf(buf + strlen(buf), "PoofInSound:  %s\n",
                ch->only.pc->poofInSound);

      if (ch->only.pc->poofOutSound == NULL)
        sprintf(buf + strlen(buf), "PoofOutSound: None\n");
      else
        sprintf(buf + strlen(buf), "PoofOutSound: %s\n",
                ch->only.pc->poofOutSound);
    }
  }
  /* group leader */
  if (ch->group && (ch->group->ch != ch))
  {
    sprintf(buf + strlen(buf), "Group Leader: %s\n", GET_NAME(ch->group->ch));
  }

  if (GET_MASTER(ch))
    sprintf(buf + strlen(buf), "Your Master: %s\n", GET_NAME(GET_MASTER(ch)));

  if (*buf)
    send_to_char(buf, ch);

  /* followers */
  found = FALSE;
  strcpy(buf, "Followers:\n");
  for (fol = ch->followers; fol; fol = fol->next)
  {
    if (IS_TRUSTED(fol->follower) || !CAN_SEE(ch, fol->follower))
    {
      /* Should not show up on "score" if a god is following. */
      continue;
    }
    found = TRUE;
    sprintf(buf + strlen(buf), "   %s\n", (IS_NPC(fol->follower) ||
                                           IS_MORPH(fol->follower)) ? fol->
            follower->player.short_descr : GET_NAME(fol->follower));
  }
  if (found)
    send_to_char(buf, ch);

  /* status */
  switch (GET_STAT(ch))
  {
  case STAT_DEAD:
    strcpy(buf, "Status:  How odd, you appear to be quite dead.");
    logit(LOG_DEBUG, "Deader in do_score().  %s (%d/%d).", GET_NAME(ch),
          GET_HIT(ch), GET_MAX_HIT(ch));
    statuslog(GET_LEVEL(ch), "%s is dead, but still in game.", GET_NAME(ch));
    break;
  case STAT_DYING:
    if (hit_regen(ch) > 0)
    {
      strcpy(buf, "Status:  Very badly wounded, but healing");
    }
    else
      strcpy(buf, "Status:  Bleeding to death");

    if (IS_FIGHTING(ch))
    {
      logit(LOG_DEBUG, "%s dying, but still fighting %s.", GET_NAME(ch),
            GET_NAME(ch->specials.fighting));
      statuslog(GET_LEVEL(ch), "%s is dying, but still fighting %s.",
                GET_NAME(ch), GET_NAME(ch->specials.fighting));
      strcat(buf, ".");
    }
    else if (NumAttackers(ch) > 0)
    {
      strcat(buf, ", assuming you are allowed the time.");
    }
    else
      strcat(buf, ".");
    break;
  case STAT_INCAP:
    if (hit_regen(ch) > 0)
    {
      strcpy(buf, "Status:  Badly wounded, but healing");
    }
    else
      strcpy(buf, "Status:  Slowly bleeding to death");

    if (IS_FIGHTING(ch))
    {
      logit(LOG_DEBUG, "%s incap, but still fighting %s.", GET_NAME(ch),
            GET_NAME(ch->specials.fighting));
      statuslog(GET_LEVEL(ch), "%s is incap, but still fighting %s.",
                GET_NAME(ch), GET_NAME(ch->specials.fighting));
      strcat(buf, ".");
    }
    else if (NumAttackers(ch) > 0)
    {
      strcat(buf, ", assuming you are allowed the time.");
    }
    else
      strcat(buf, ".");
    break;
  case STAT_SLEEPING:
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      if (IS_RIDING(ch))
      {
        strcpy(buf, "Status:  Stretched out, fast asleep (while riding?)");
        logit(LOG_DEBUG, "%s prone/asleep, but still riding?", GET_NAME(ch));
        statuslog(GET_LEVEL(ch), "%s prone/asleep, but still riding?",
                  GET_NAME(ch));
        send_to_char("It's kind of tough to ride while sleeping.\n", ch);
        stop_riding(ch);
      }
      else
        strcpy(buf, "Status:  Stretched out, fast asleep");
      break;
    case POS_SITTING:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Asleep, in the saddle");
      else
        strcpy(buf, "Status:  Asleep, sitting up");
      break;
    case POS_KNEELING:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Kneeling, fast asleep (while riding?)");
      else
        strcpy(buf, "Status:  Asleep, kneeling (?)");
      break;
    case POS_STANDING:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Asleep, (standing in the saddle?)");
      else
        strcpy(buf, "Status:  Asleep on your feet (literally)");
      break;
    }

    if (IS_FIGHTING(ch))
    {
      logit(LOG_DEBUG, "%s asleep, but still fighting %s.", GET_NAME(ch),
            GET_NAME(ch->specials.fighting));
      statuslog(GET_LEVEL(ch), "%s is asleep, but still fighting %s.",
                GET_NAME(ch), GET_NAME(ch->specials.fighting));
      strcat(buf, ".");
    }
    else if (NumAttackers(ch) > 0)
    {
      strcat(buf, ", but not for long.");
    }
    else
      strcat(buf, ".");
    break;
  case STAT_RESTING:
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Stretched out in the saddle, resting (?)");
      else
        strcpy(buf, "Status:  Laying down, resting");
      break;
    case POS_KNEELING:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Kneeling in the saddle (?), resting (?)");
      else
        strcpy(buf, "Status:  Resting on your hands and knees");
      break;
    case POS_SITTING:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Resting in the saddle");
      else
        strcpy(buf, "Status:  Sitting around, resting");
      break;
    case POS_STANDING:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Standing in the saddle (?), resting (?)");
      else
        strcpy(buf, "Status:  Standing, catching your breath");
      break;
    }
    if (IS_FIGHTING(ch))
    {
      logit(LOG_DEBUG, "%s resting, but still fighting %s.", GET_NAME(ch),
            GET_NAME(ch->specials.fighting));
      statuslog(GET_LEVEL(ch), "%s is resting, but still fighting %s.",
                GET_NAME(ch), GET_NAME(ch->specials.fighting));
      strcat(buf, ".");
    }
    else if (NumAttackers(ch) > 0)
    {
      strcat(buf, ", but not for long.");
    }
    else
      strcat(buf, ".");
    break;
  case STAT_NORMAL:
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Lying down (while riding?)");
      else
        strcpy(buf, "Status:  Lying down");
      break;
    case POS_KNEELING:
      if (IS_RIDING(ch))
        strcpy(buf, "Status:  Kneeling (while riding?)");
      else
        strcpy(buf, "Status:  Kneeling");
      break;
    case POS_SITTING:
      if (IS_RIDING(ch))
        sprintf(buf, "Status:  Sitting in %s's saddle",
                get_linked_char(ch, LNK_RIDING)->player.short_descr);
      else
        strcpy(buf, "Status:  Sitting");
      break;
    case POS_STANDING:
      if (IS_RIDING(ch))
        sprintf(buf, "Status:  Standing in %s's saddle",
                get_linked_char(ch, LNK_RIDING)->player.short_descr);
      else
        strcpy(buf, "Status:  Standing");
      break;
    }
    if (IS_FIGHTING(ch))
    {
      sprintf(buf + strlen(buf), ", fighting %s.",
              PERS(ch->specials.fighting, ch, FALSE));
    }
    else if( IS_DESTROYING(ch) )
    {
      sprintf(buf + strlen(buf), ", destroying %s.", ch->specials.destroying_obj->short_description);
    }
    else if (NumAttackers(ch) > 0)
    {
      strcat(buf, ", and getting beaten on.");
    }
    else
      strcat(buf, ".");
    break;
  default:
    strcpy(buf, "Status:  Beats the hell out of me, how did you DO that?");
    break;
  }

  strcat(buf, "\n");
  send_to_char(buf, ch);

  /* status type things */

  buf[0] = '\0';

  if (IS_AFFECTED4(ch, AFF4_STORNOGS_SPHERES))
    strcat(buf, "&+b Stornogs &+BLesser&n&+b Spheres&n");

  if (IS_AFFECTED4(ch, AFF4_STORNOGS_GREATER_SPHERES))
    strcat(buf, "&+B Stornogs &+WGreater&+B Spheres&n");

  if (IS_AFFECTED5(ch, AFF5_VINES))
    strcat(buf, "&+g Vines&n");

  if (IS_AFFECTED4(ch, AFF4_SANCTUARY))
    strcat(buf, " &+WSanctuary&n");
  if (IS_AFFECTED4(ch, AFF4_HELLFIRE))
    strcat(buf, " &+rHell&+Rfire&n");

  if (IS_AFFECTED4(ch, AFF4_NEG_SHIELD))
    strcat(buf, " &+LNegative Shielded&n");
  else if (IS_AFFECTED2(ch, AFF2_SOULSHIELD))
    if (GET_CLASS(ch, CLASS_THEURGIST))
      strcat(buf, " &+WHoly Aura");
    else
      strcat(buf, " &+WSoulshielded&n");
  
  if( IS_AFFECTED2(ch, AFF2_FIRESHIELD) )
  {
    strcat(buf, " &+RFireshielded&n");
  }
  else if( IS_AFFECTED3(ch, AFF3_COLDSHIELD) )
  {
    strcat(buf, " &+BColdshielded&n");
  }
  else if( IS_AFFECTED3(ch, AFF3_LIGHTNINGSHIELD) )
  {
    strcat(buf, " &+YLightningshielded&n");
  }
  
  if (IS_AFFECTED(ch, AFF_CAMPING))
    strcat(buf, " Camping");
  if (IS_AFFECTED3(ch, AFF3_SINGING))
    strcat(buf, " Singing");
  if (IS_AFFECTED2(ch, AFF2_MEMORIZING))
  {
    if (meming_class(ch) || GET_CLASS(ch, CLASS_SHAMAN))
      strcat(buf, " Memorizing");
    else
      strcat(buf, " Praying");
    if (IS_AFFECTED(ch, AFF_MEDITATE))
      strcat(buf, " Meditating");
  }
  if (IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    strcat(buf, " Unconscious");
  if (IS_AFFECTED(ch, AFF_BOUND))
    strcat(buf, " Restrained");
  if (IS_AFFECTED2(ch, AFF2_STUNNED))
    strcat(buf, " Stunned");

  if ((IS_SET(world[ch->in_room].room_flags, UNDERWATER)) ||
      ch->specials.z_cord < 0)
  {
    if (!IS_AFFECTED2(ch, AFF2_HOLDING_BREATH) &&
        !IS_AFFECTED2(ch, AFF2_IS_DROWNING))
      strcat(buf, " Underwater : Swimming");
    else if (IS_AFFECTED2(ch, AFF2_HOLDING_BREATH))
      strcat(buf, " Underwater : Holding breath");
    else
      strcat(buf, " Underwater : &+BDrowning!&n");
  }
  if (*buf)
  {
    send_to_char("        ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }

  if (IS_PC(ch) && IS_HARDCORE(ch))
  {
    hardcorepts = getHardCorePts(ch);
    hardcorepts = (hardcorepts / 100.0);
    sprintf(buf, "&nHardCore pts:   &+R%+6.2f&n\n", hardcorepts);
    send_to_char(buf, ch);
  }
  /* frags */
#ifdef STANCES_ALLOWED
  if(IS_AFFECTED5(ch, AFF5_STANCE_DEFENSIVE))
          send_to_char("Stance: &+yDefensive stance&n\n", ch);
           
  else if (IS_AFFECTED5(ch, AFF5_STANCE_OFFENSIVE))
    send_to_char("Stance: &+rOffensive stance&n\n", ch);
  else
    send_to_char("Stance: &+WNo stance at all&n\n", ch);
#endif
                             
  if( GET_LEVEL(ch) > 19 )
  {

    if( IS_PC(ch) && ch->only.pc->epics < 0 )
    {
      ch->only.pc->epics = 0;
    }

    if( IS_PC(ch) )
    {
      struct affected_type *afp, *afp2;

      // 0 and above = zone epic task
      // -1 to -9 = nexus stone task
      // -10 = spill blood task
	  /* commenting this out due to skill points being deprecated - Zion 4/8/2014
      if (afp = get_epic_task(ch)) {
        if (afp->modifier == SPILL_BLOOD)
          sprintf(buf, "&n&+YEpic points:&n &+W%ld&n  &+YSkill points:&n &+W%ld&n  Current task: &+rspill enemy blood&n\n",
              ch->only.pc->epics, ch->only.pc->epic_skill_points);
        else if (afp->modifier >= 0)
          sprintf(buf, "&nEpic points: &+W%ld&n  Skill points: &+W%ld&n  Current task: find runestone of %s\n",
              ch->only.pc->epics, ch->only.pc->epic_skill_points, zone_table[real_zone0(afp->modifier)].name);
	else if (afp->modifier <= MAX_NEXUS_STONES && afp->modifier < 0)
	{
	  nexus = get_nexus_stone(-(afp->modifier));
          if (nexus)
	  {
	  sprintf(buf, "&nEpic points: &+W%ld&n  Skill points: &+W%ld&n  Current task: &+Gturn %s.&n\n",
              ch->only.pc->epics, ch->only.pc->epic_skill_points, nexus->short_description);
	  }
	  else
	  {
	  sprintf(buf, "&nEpic points: &+W%ld&n  Skill points: &+W%ld&n  Current task: &+RERROR - can't find nexus, report to imms&n\n",
              ch->only.pc->epics, ch->only.pc->epic_skill_points);
          }
	}
	else
	{
	  sprintf(buf, "&nEpic points: &+W%ld&n  Skill points: &+W%ld&n  Current task: &+RERROR - report to imms&n\n",
              ch->only.pc->epics, ch->only.pc->epic_skill_points);
	}
      } else {
        sprintf(buf, "&nEpic points: &+W%ld&n  Skill points: &+W%ld&n\n",
             ch->only.pc->epics, ch->only.pc->epic_skill_points);
      }
      send_to_char(buf, ch);
    }
  }
  */
      afp2 = get_spell_from_char(ch, TAG_EPICS_GAINED);

      if( afp = get_epic_task(ch) )
      {
        if( afp->modifier == SPILL_BLOOD )
        {
          sprintf(buf, "&n&+YEpic points(total):&n &+W%ld(%d)&n Current task: &+rspill enemy blood&n\n",
              ch->only.pc->epics, afp2 ? afp2->modifier : 0 );
        }
        else if( afp->modifier >= 0 )
        {
          sprintf(buf, "&nEpic points(total): &+W%ld(%d)&n Current task: find runestone of %s\n",
              ch->only.pc->epics, afp2 ? afp2->modifier : 0, zone_table[real_zone0(afp->modifier)].name);
        }
        else if( afp->modifier <= MAX_NEXUS_STONES && afp->modifier < 0 )
        {
          nexus = get_nexus_stone(-(afp->modifier));
          if( nexus )
          {
            sprintf(buf, "&nEpic points(total): &+W%ld(%d)&n Current task: &+Gturn %s.&n\n",
              ch->only.pc->epics, afp2 ? afp2->modifier : 0, nexus->short_description);
          }
          else
          {
            sprintf(buf, "&nEpic points(total): &+W%ld(%d)&n Current task: &+RERROR - can't find nexus, report to imms&n\n",
              ch->only.pc->epics, afp2 ? afp2->modifier : 0);
          }
        }
        else
        {
          sprintf(buf, "&nEpic points(total): &+W%ld(%d)&n Current task: &+RERROR - report to imms&n\n",
              ch->only.pc->epics, afp2 ? afp2->modifier : 0);
        }
      }
      else
      {
        sprintf(buf, "&nEpic points(total): &+W%ld(%d)&n\n",
             ch->only.pc->epics, afp2 ? afp2->modifier : 0);
      }
      send_to_char(buf, ch);
    }
  }
  else
  {
    sprintf(buf, "&nEpic points: &+W%ld&n\n", ch->only.pc->epics);
    send_to_char(buf, ch);
  }

  EpicBonusData ebdata;
  if (!get_epic_bonus_data(ch, &ebdata))
  {
    ebdata.type = EPIC_BONUS_NONE;
  }
  send_to_char_f(ch, "&nEpic Bonus: &+C%s&n (&+C%.2f%%&n)\r\n", ebd[ebdata.type].description, get_epic_bonus(ch, ebdata.type)*100);
//#ifdef SKILLPOINTS
  //send_to_char_f(ch, "&nSkill Points: &+W%d&n\r\n", ch->only.pc->skillpoints);
//#endif
  send_to_char("&+RFrags:&n   ", ch);

  fragnum = (float) ch->only.pc->frags;
  fragnum /= 100;
  sprintf(buf, "%+.2f   &n&+LDeaths:&n   %lu\n", fragnum, ch->only.pc->numb_deaths);
  send_to_char(buf, ch);

  if(ch->linking)
  {
    sprintf(buf, "Consenting: %s\n", ch->linking->linked->player.name);
    send_to_char(buf, ch);
  }

  if (IS_DISGUISE(ch))
  {
    if (IS_DISGUISE_ILLUSION(ch))
      send_to_char("In the illusion of: ", ch);
    else if (IS_DISGUISE_SHAPE(ch))
      send_to_char("Shapechanged as: ", ch);
    else
      send_to_char("Disguised as: ", ch);
    percent =
      (int) ((float) ch->disguise.hit / (float) (GET_LEVEL(ch) * 4) * 100.0);
    sprintf(buf, "%s (%s)\n", ch->disguise.name,
            (percent > 75) ? "&+GExcellent&N" : (percent >
                                                 50) ? "&+YGood&N" : (percent
                                                                      >
                                                                      25) ?
            "&+yfair&N" : "&+rPoor&N");
    send_to_char(buf, ch);
  }

  buf[0] = 0;

  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
    strcat(buf, "&+LWraith&+gform&n");
  if (IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION))
  {
//     if (!IS_AFFECTED(ch, AFF_DETECT_INVISIBLE))
//     strcat(buf, " Invisible Illusions");
//     else
     strcat(buf, " &+MI&+Ll&+ml&+Mu&+Ls&+mi&+Mo&+Ln&+ms&n");
  }
  if (IS_AFFECTED(ch, AFF_DETECT_INVISIBLE))
    strcat(buf, " &+CI&+cn&+Cv&+ci&+Cs&+ci&+Cb&+cl&+Ce&n");
  if (IS_AFFECTED2(ch, AFF2_DETECT_EVIL))
    strcat(buf, " &+RE&+rv&+Ri&+rl&n");
  if (IS_AFFECTED2(ch, AFF2_DETECT_GOOD))
    strcat(buf, " &+YGood&n");
  if (IS_AFFECTED2(ch, AFF2_DETECT_MAGIC))
    strcat(buf, " &+MM&+ma&+Mg&+mi&+Mc&n");
  if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
    strcat(buf, " &+WLife&n");
  if (IS_AFFECTED(ch, AFF_INFRAVISION))
    strcat(buf, " &+rHeat&n");
  if (IS_AFFECTED4(ch, AFF4_SENSE_HOLINESS))
    strcat(buf, " &+WH&+Yo&+Wl&+Yi&+Wn&+Ye&+Ws&+Ys&n");

  if (*buf)
  {
    send_to_char("Detecting:      ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }
  buf[0] = 0;

  if (IS_AFFECTED2(ch, AFF2_DETECT_MAGIC) || IS_TRUSTED(ch) )
  {
    if (IS_AFFECTED(ch, AFF_PROTECT_EVIL))
      strcat(buf, " &+RE&+rv&+Ri&+rl&n");
    if (IS_AFFECTED(ch, AFF_PROTECT_GOOD))
      strcat(buf, " &+YGood&n");
    if (IS_AFFECTED(ch, AFF_PROT_FIRE))
      strcat(buf, " &+rFire&n");
    if (IS_AFFECTED2(ch, AFF2_PROT_COLD))
      strcat(buf, " &+cCold&n");
    if (IS_AFFECTED2(ch, AFF2_PROT_LIGHTNING))
      strcat(buf, " &+BLightning&n");
    if (IS_AFFECTED2(ch, AFF2_PROT_GAS))
      strcat(buf, " &+GGas&n");
    if (IS_AFFECTED2(ch, AFF2_PROT_ACID))
      strcat(buf, " &+GA&+gc&+Gi&+gd&n");
    if (IS_AFFECTED5(ch, AFF5_PROT_UNDEAD))
      strcat(buf, " &+LU&+Wn&+Ld&+We&+La&+Wd&n");
    if (IS_AFFECTED4(ch, AFF4_PROT_LIVING))
      strcat(buf, " &+WLiving&n");
    if (IS_AFFECTED3(ch, AFF3_PROT_ANIMAL))
      strcat(buf, " &+yAnimals&n");
  }
  if (IS_AFFECTED2(ch, AFF2_GLOBE))
    strcat(buf, " &+WAll but High Circle Spells&n");
  else if (IS_AFFECTED(ch, AFF_MINOR_GLOBE))
    strcat(buf, " &+WLow Circle spells&n");
  
  if (*buf)
  {
    send_to_char("Protected from: ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }

  buf[0] = 0;
  if (IS_AFFECTED3(ch, AFF3_GR_SPIRIT_WARD))
    strcat(buf, " &+WGreater Spirit Ward&n");
  else if (IS_AFFECTED3(ch, AFF3_SPIRIT_WARD))
    strcat(buf, " Spirit Ward");
  if (IS_AFFECTED(ch, AFF_SLOW_POISON))
    strcat(buf, " Slow Poison");

  if (*buf)
  {
    send_to_char("Protected by:   ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }
  buf[0] = 0;

  if( IS_AFFECTED3(ch, AFF3_INERTIAL_BARRIER) )
  {
    strcat(buf, " &+WIner&+Ltia&+Wl-Ba&+Lrri&+Wer&n");
  }
  if( IS_AFFECTED3(ch, AFF3_NON_DETECTION) )
  {
    strcat(buf, " &+LNon-Detection&n");
  }
  if( IS_AFFECTED2(ch, AFF2_ULTRAVISION) )
  {
    strcat(buf, " &+MUltravision&n");
  }
  if( IS_AFFECTED(ch, AFF_FARSEE) )
  {
    strcat(buf, " &+YFarsee&n");
  }
  if( IS_AFFECTED(ch, AFF_FLY) )
  {
    strcat(buf, " &+WFly&n");
  }
  if(IS_AFFECTED(ch, AFF_ARMOR) )
  {
    strcat(buf, " &+WArmor&n");
  }
  if( IS_AFFECTED(ch, AFF_AWARE) )
  {
    strcat(buf, " &+BA&+Ww&+Ba&+Wr&+Be&n");
  }
  if(IS_AFFECTED(ch, AFF_HASTE) )
  {
    strcat(buf, " &+RH&+ra&+Rs&+rt&+Re&n");
  }
  if( IS_AFFECTED3(ch, AFF3_BLUR) )
  {
    strcat(buf, " &+CBlur&n");
  }
  if( IS_AFFECTED2(ch, AFF2_MINOR_INVIS) || IS_AFFECTED(ch, AFF_INVISIBLE) )
  {
    strcat(buf, " &+cInv&+Cisi&+cbil&+City&n");
  }
  if( IS_AFFECTED3(ch, AFF3_ECTOPLASMIC_FORM) )
  {
    strcat(buf, " &+LEct&+mopla&+Lsmic f&+morm&n");
  }
  if( IS_AFFECTED(ch, AFF_LEVITATE) )
  {
    strcat(buf, " &+WLevitation&n");
  }
  if( IS_AFFECTED(ch, AFF_WATERBREATH) )
  {
    strcat(buf, " &+bWater&+Bbreathing&n");
  }
  if( IS_AFFECTED3(ch, AFF3_SWIMMING) )
  {
    strcat(buf, " Treading Water");
  }
  if( IS_AFFECTED3(ch, AFF3_ENLARGE) )
  {
    strcat(buf, " &+REnlarged Size&n");
  }
  if( IS_AFFECTED3(ch, AFF3_REDUCE) )
  {
    strcat(buf, " &+yReduced Size&n");
  }
  if( in_command_aura(ch) )
  {
    strcat(buf, " &+WCommand Aura&n");
  }
  if( IS_AFFECTED5(ch, AFF5_LISTEN) )
  {
      strcat(buf, " &+wListen&n");
  }
  if( affected_by_spell(ch, SPELL_CORPSEFORM) )
  {
     strcat(buf, " &+LCorpseform&n");
  }
  if( affected_by_spell(ch, SPELL_MIRAGE) )
  {
     strcat(buf, " &+GM&+gi&+Gr&+ga&+Gg&+ge&n");
  }
  if( IS_AFFECTED5(ch, AFF5_TITAN_FORM) )
  {
     strcat(buf, " &+REno&+yrmou&+Rs Si&+yze&n");
  }
  if( IS_AFFECTED(ch, AFF_SNEAK) )
  {
     strcat(buf, " &+WSneaking&n");
  }
  if( IS_AFFECTED4(ch, AFF4_EPIC_INCREASE) )
  {
    strcat(buf, " &+WBlessing of the Gods&n");
  }
  if( IS_AFFECTED3(ch, AFF3_TOWER_IRON_WILL) )
  {
    strcat(buf, " &+WTow&+Ler o&+Wf Ir&+Lon W&+Will&n");
  }
  if( IS_AFFECTED4(ch, AFF4_NOFEAR) )
  {
      strcat(buf, " &+WFearless&n");
  }
  if( IS_AFFECTED3(ch, AFF3_PASS_WITHOUT_TRACE) )
  {
      strcat(buf, " &+gLight&+yfooted&n");
  }
  if( IS_AFFECTED4(ch, AFF4_DAZZLER) )
  {
      strcat(buf, " &+WS&+Bp&+Ca&+Wr&+Bk&+Cl&+Wi&+cn&+bg&n" );
  }

  if (*buf)
  {
    send_to_char("Enchantments:   ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }

  buf[0] = 0;

  if(affected_by_spell(ch, SONG_DRAGONS))
    strcat(buf, " &+GSong of &+rD&+Lr&+ra&+Lg&+ro&+Ln&+rs&n");

  if(affected_by_spell(ch, SONG_PROTECTION))
    strcat(buf, " &+GSong of &+WProtection&n");

  if(affected_by_spell(ch, SONG_PROTECTION))
    strcat(buf, " &+GSong of &+WProtection&n");

  if(affected_by_spell(ch, SONG_REVELATION))
    strcat(buf, " &+GSong of &+cR&+Ce&+cv&+Ce&+cl&+Ca&+ct&+Ci&+co&+Cn&n");

  if(affected_by_spell(ch, SONG_HEROISM))
    strcat(buf, " &+GSong of &+YHeroism&n");

  if(affected_by_spell(ch, SONG_FLIGHT))
    strcat(buf, " &+GSong of &+CFlight&n");

  if(affected_by_spell(ch, SONG_PEACE))
    strcat(buf, " &+GSong of &+WPeace&n");

  if(affected_by_spell(ch, SONG_SLEEP))
    strcat(buf, " &+GSong of &+bSleep&n");

  if (*buf)
  {
    send_to_char("Songs:   ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }
  
  buf[0] = 0;
  
  if (GET_LEVEL(ch) > 50)
  {
    struct affected_type *af;
    long ct;
    struct time_info_data timer;
	
    if((af = get_spell_from_char(ch, TAG_POOL)) == NULL)
    {
      send_to_char("Stat pool timeout: none\n", ch);
    }
	  else
	  {
	    ct = time(0);
	    timer = real_time_countdown(ct, af->modifier, 60*60*24*2);
	    sprintf(buf, "Stat Pool timeout: %d:%s%d:%s%d\n", 
	            //af->duration * PULSES_IN_TICK / (WAIT_SEC * SECS_PER_REAL_HOUR));
	      timer.day * 24 + timer.hour,
	      (timer.minute > 9) ? "" : "0", timer.minute,
	      (timer.second > 9) ? "" : "0", timer.second);
	    send_to_char(buf, ch);
	  }
  }
  
  buf[0] = 0;
  
  if(IS_PC(ch))
  {
  	int RemainingBartenderQuests = sql_world_quest_can_do_another(ch);
	  sprintf(buf, "&+yBartender Quests Remaining:&n %d\n", RemainingBartenderQuests);
	  send_to_char(buf, ch);
  }

  if(IS_PC(ch))
  {
  
        sprintf(buf, "&+YCombat Pulse: &N%4d&+Y &+MSpell Pulse&n:  %.2f&+Y ",
//            ch->specials.base_combat_round, ch->points.spell_pulse);
         ch->specials.base_combat_round, spell_pulse_data[GET_RACE(ch)] * SPELL_PULSE(ch) );



    strcat(buf, "\n");
    send_to_char(buf, ch);

    sprintf(buf, "&+LLea&+wder&+Wboard Points&n: &n%4ld&n ", (getLeaderBoardPts(ch) / 100));
    strcat(buf, "\n");
    send_to_char(buf, ch);
  }
  
  buf[0] = 0;

  if (IS_AFFECTED2(ch, AFF2_SLOW))
    strcat(buf, " &+cSlowness&n");
  if (IS_AFFECTED(ch, AFF_BLIND))
    strcat(buf, " &+LBlindness&n");
  if (IS_AFFECTED4(ch, AFF4_DEAF))
    strcat(buf, " &+LDeaf&n");
  if (IS_AFFECTED(ch, AFF_FEAR))
    strcat(buf, " &+LFear&n");
  if (IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS) && GET_RACE(ch) != RACE_CONSTRUCT)
    strcat(buf, " &+MTotal Paralysis&n");
  if (IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS))
    strcat(buf, " &+mParalysis&n");
  if (IS_AFFECTED2(ch, AFF2_POISONED))
    strcat(buf, " &+GPoison&n");
  if (IS_AFFECTED5(ch, AFF5_WET))
    strcat(buf, " &+bWet&n");
    
  if (*buf)
  {
    send_to_char("Afflicted with: ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }

  struct affected_type af, *afp;
  buf2[0] = 0;
  buf[0] = 0;
  for (afp = ch->affected; afp; afp = afp->next)
  {
     if (afp->type == TAG_ARMLOCK)
     {
        strcat(buf2, " &+wBroken arm");
     }
     else if(afp->type == TAG_LEGLOCK)
     {
        strcat(buf2, " &+wBroken leg");
     }
  }
  sprintf(buf, "Injuries: ");
  if(*buf2)
  {
    send_to_char(buf, ch);
    send_to_char(buf2, ch);
    send_to_char("\n", ch);
  }

  tch = get_linking_char(ch, LNK_BLOOD_ALLIANCE);
  if( !tch ) 
    tch = get_linked_char(ch, LNK_BLOOD_ALLIANCE);
  
  if( tch )
  {
    buf[0] = 0;
    send_to_char("Blood alliance with: ", ch);
    strcat(buf, GET_NAME(tch));
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }
  
  if( IS_AFFECTED3(ch, AFF3_PALADIN_AURA) )
  {
    buf[0] = 0;

    for( int i = FIRST_AURA; i <= LAST_AURA; i++ )
    {
      if( has_aura(ch, i) )
      {
        strcat(buf, " ");
        strcat(buf, aura_name(i) );
      }
    }

    if (*buf)
    {
      send_to_char("Paladin auras:  ", ch);
      send_to_char(buf, ch);
      send_to_char("\n", ch);
    }
  }

  /* somehow displaying "Heal Enhancement" for score or aura */
  if(IS_AFFECTED3(ch, AFF3_ENHANCE_HEALING))
  {
      buf[0] = 0;
      strcat(buf, " Healing");
      if(*buf)
      {
          send_to_char("Enhancements:   ", ch);
          send_to_char(buf, ch);
          send_to_char("\n", ch);
      }
  }
  
  if (affected_by_spell(ch, TAG_SPAWN))
   if (has_innate(ch, INNATE_SPAWN))
     send_to_char("&+LYou are willing to summon spawns.\n", ch);
   else if (has_innate(ch, INNATE_ALLY))
     send_to_char("&+WYou are willing to summon allies.\n", ch);
  
  tch = guarding(ch);
  if( tch )
  {
    buf[0] = 0;
    strcat(buf, tch->player.name);
    if (CAN_MULTI_GUARD(ch))
      for (int maxg = 2; maxg <= (int)get_property("skill.guard.max.multi", 2); maxg++)
      {
	tch2 = guarding2(ch, maxg);
        if (tch2)
	{
          strcat(buf, ", ");
	  strcat(buf, tch2->player.name);
	}
      }
    send_to_char("Guarding: ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }

  tch = guarded_by(ch);
  if( tch )
  {
    buf[0] = 0;
    strcat(buf, tch->player.name);
    send_to_char("Guarded by: ", ch);
    send_to_char(buf, ch);
    send_to_char("\n", ch);
  }

  buf[0] = 0;

  /*
   * loop through affected list, show them the ones they can see, if
   * affected by detect magic, show remaining durations too. JAB
   */
  if( ch->affected )
  {
    // Initialize last to a nonsense value since we don't have a 'last aff type' before we start.
    //   TYPE_UNDEFINED == skill number -1 as of 6/17/2015.
    last = TYPE_UNDEFINED;

    for( aff = ch->affected; aff; aff = aff->next )
    {
      if( (aff->type > 0) && skills[aff->type].name
        && (aff->type <= LAST_SKILL || aff->type == TAG_CTF || aff->type == TAG_RESTED || aff->type == TAG_WELLRESTED) )
      {
        switch (aff->type)
        {
          // These are never reported
          case SKILL_AWARENESS:
          case SKILL_FIRST_AID:
          case SKILL_HEADBUTT:
          case SKILL_THROAT_CRUSHER:
          case SKILL_SNEAK:
          case SPELL_RECHARGER:
          case SPELL_SUMMON:
          case SKILL_SCRIBE:
          case SKILL_TACKLE:
            continue;
            break;

          // These get reported, only if detect magic active
          case SONG_CHARMING:
          case SPELL_CHARM_PERSON:
          case SPELL_PROTECT_FROM_ACID:
          case SPELL_PROTECT_FROM_COLD:
          case SPELL_PROTECT_FROM_EVIL:
          case SPELL_PROTECT_FROM_FIRE:
          case SPELL_PROTECT_FROM_GAS:
          case SPELL_PROTECT_FROM_GOOD:
          case SPELL_PROTECT_FROM_LIGHTNING:
          case SPELL_PROT_FROM_UNDEAD:
          case SPELL_PROT_UNDEAD:
          case SPELL_SLOW_POISON:
          case SPELL_VAMPIRIC_TOUCH:
          case SPELL_ETHEREAL_FORM:
          case SPELL_HOLY_DHARMA:
          case SPELL_SANCTUARY:
          case SPELL_BLUR:
          case SPELL_FAERIE_SIGHT:
            if(!IS_AFFECTED2(ch, AFF2_DETECT_MAGIC))
            {
              continue;
            }
            break;

          // The rest always get reported.
          default:
            break;
        }

        if( IS_SET(aff->flags, AFFTYPE_NOSHOW) )
        {
          continue;
        }

        // Don't display affects with 2 or more structs twice.
        if (aff->type - 1 != last)
        {
          strcat(buf, skills[aff->type].name);
          if(IS_SET(aff->flags, AFFTYPE_SHORT))
          {
            secs = 0;
            LOOP_EVENTS_CH( ne, ch->nevents )
            {
              if( ne->func == event_short_affect && ne->data != NULL )
              {
                if( aff == ((struct event_short_affect_data *)(ne->data))->af )
                {
                  secs = ne_event_time(ne)/WAIT_SEC;
                  break;
                }
              }
            }
            if( secs > 60 )
            {
	            sprintf(buf1, " (&+W%d &+Yminute%s&n)\n", secs/60, (secs/60 > 1) ? "s" : "" );
              strcat(buf, buf1);
            }
            else
            {
              strcat(buf, " (&+Rless than a minute remaining&n)\n");
            }
          }
          else
          {
        	  if(aff->duration < 0)
            {
              //strcat(buf, "\n");
      	      sprintf(buf1, " (&+Bno expiration timer&n)\n");
              strcat(buf, buf1);
            }
            else if(aff->duration > 1) //(!IS_AFFECTED2(ch, AFF2_DETECT_MAGIC) ||  
            {
              //strcat(buf, "\n");
	            sprintf(buf1, " (&+W%d &+Yminutes&n)\n", aff->duration);
              strcat(buf, buf1);
            }
            else if( aff->duration == 1 )
            {
	            sprintf(buf1, " (&+W1 &+Yminute&n)\n" );
              strcat(buf, buf1);
            }
            else if (aff->duration < 1)
            {
              strcat(buf, " (&+Rless than a minute remaining&n)\n");
            }
            else if (aff->duration == 2)
            {
              strcat(buf, " (fading)\n");
            }
          }
        }
        last = aff->type - 1;
      }
    }
#if defined(CTF_MUD) && (CTF_MUD == 1)
    affected_type *af2;
    if ((af2 = get_spell_from_char(ch, TAG_CTF_BONUS)) != NULL)
      sprintf(buf + strlen(buf), "CTF Bonus Level %d", af2->modifier);
#endif

    if( *buf && !affected_by_spell(ch, SPELL_FEEBLEMIND) )
    {
      send_to_char("\n&+cActive Spells:&n\n--------------\n", ch);
      send_to_char(buf, ch);
    }
  }

  if(affected_by_spell(ch, SKILL_REGENERATE))
  {
    send_to_char("regenerating\n", ch);
  }

  send_to_char("\n", ch);
}

void do_time(P_char ch, char *argument, int cmd)
{
  char    *tmstr;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  const char *suf;
  int      weekday, day, hour;
  long     ct;
  struct tm *lt;
  struct time_info_data uptime;

  argument = one_argument(argument, Gbuf1);

  if( IS_TRUSTED(ch) && GET_LEVEL(ch) >= FORGER && strlen(Gbuf1) )
  {
    hour = time_info.hour = atoi(Gbuf1) % 24;

    wizlog(56, "%s set the time to %d:00 or %d:00%s.", GET_NAME(ch), time_info.hour,
      (hour == 0) ? 12 : (hour > 12) ? hour - 12 : hour, (hour > 11) ? "pm" : "am" );
    send_to_char("You set the time.\n", ch);
    astral_clock_setMapModifiers();
    return;
  }

#if 0
  if (IS_TRUSTED(ch))
    sprintf(Gbuf1, ":%s%d", ((pulse / 5) > 9) ? "" : "0", pulse / 5);
  else
#endif
    Gbuf1[0] = 0;

  sprintf(Gbuf2, "It is %d%s%s, on ",
          (time_info.hour % 12) ? (time_info.hour % 12) : 12, Gbuf1,
          (time_info.hour > 11) ? "pm" : "am");

  /*
   * 35 days in a month
   */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(Gbuf2, weekdays[weekday]);
  strcat(Gbuf2, "\n");
  send_to_char(Gbuf2, ch);

  day = time_info.day + 1;      /*
                                 * day in [1..35]
                                 */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(Gbuf2, "The %d%s Day of the %s, Year %d.\n",
          day, suf, month_name[time_info.month], (time_info.year + 1000));
  send_to_char(Gbuf2, ch);

  ct = time(0);
  uptime = real_time_passed(ct, boot_time);


 

  //Auto Reboot - Drannak
  if( (uptime.day * 24 + uptime.hour) > 65 )
  {
    // If no shutdown in progress, or shutdown is > 60 minutes out.
    if( shutdownData.reboot_time == 0
      || shutdownData.reboot_time - time(NULL) > 60 * 60 )
    {
      do_shutdown(ch, "autoreboot 60", 1);
    }
  }

  sprintf(Gbuf2, "Time elapsed since boot-up: %d:%s%d:%s%d\n",
          uptime.day * 24 + uptime.hour,
          (uptime.minute > 9) ? "" : "0", uptime.minute,
          (uptime.second > 9) ? "" : "0", uptime.second);
  send_to_char(Gbuf2, ch);

  lt = localtime(&ct);
  tmstr = asctime(lt);
  *(tmstr + strlen(tmstr) - 1) = '\0';
  sprintf(Gbuf2, "Current time is: %s (%s)\n",
          tmstr, (lt->tm_isdst <= 0) ? "GMT" : "GMT");
  send_to_char(Gbuf2, ch);

  if (IS_TRUSTED(ch))
  {
    sprintf(Gbuf2, "Kvark time&n: %ld \n", time(NULL));
    send_to_char(Gbuf2, ch);
  }
  displayShutdownMsg(ch);
}

void do_weather(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH];
  struct weather_data *cond;
  struct climate *clime;
  int      seas;

  const char *wind_patterns[7] = {
    "Calm ",
    "Breezy ",
    "Unsettled ",
    "Windy ",
    "Chinook ",
    "Violent ",
    "Hurricane "
  };
  const char *wind_dir[4] = { "North. ", "East. ", "South. ", "West. " };
  const char *precip_patterns[9] = {
    "No precip. ",
    "Arid. ",
    "Dry. ",
    "Low precip. ",
    "Avg precip. ",
    "High precip. ",
    "Stormy. ",
    "Torrent. ",
    "Const precip. "
  };
  const char *temp_patterns[11] = {
    "Frostbite. ",
    "Nippy. ",
    "Freezing. ",
    "Cold. ",
    "Cool. ",
    "Mild. ",
    "Warm. ",
    "Hot. ",
    "Blustery. ",
    "Heatstroke. ",
    "Boiling. "
  };
  const char *season_name[4] = {
    "Winter ",
    "Spring ",
    "Summer ",
    "Fall "
  };
  if (in_weather_sector(ch->in_room) == NOWHERE)
    return;

  clime = &sector_table[in_weather_sector(ch->in_room)].climate;
  cond = &sector_table[in_weather_sector(ch->in_room)].conditions;

  if (!clime || !cond)
  {
    logit(LOG_DEBUG, "do_weather: Major error!");
    send_to_char("Ouch! Error in weather, please petition!\n", ch);
    return;
  }
  if (IS_TRUSTED(ch))
  {
    seas = get_season(in_weather_sector(ch->in_room));
    if (seas < 0 || seas > 3)
    {
      send_to_char("Couldn't find a valid season. How odd.\n", ch);
      return;
    }
    sprintf(buf, "Weather Dump for sector %d:\n",
            in_weather_sector(ch->in_room));
    send_to_char(buf, ch);

    sprintf(buf, "  Current Season: %s", season_name[seas]);
    send_to_char(buf, ch);

    send_to_char("\n&+YVariable data, current for this season:&n", ch);
    send_to_char("\nWind Characteristics: ", ch);
    send_to_char(wind_patterns[clime->season_wind[seas] - 1], ch);
    if (clime->season_wind_variance[seas - 1])
      send_to_char("direction is variable. ", ch);
    else
    {
      send_to_char("from ", ch);
      send_to_char(wind_dir[(int) clime->season_wind_dir[seas]], ch);
    }
    send_to_char("\nPrecipition: ", ch);
    send_to_char(precip_patterns[clime->season_precip[seas] - 1], ch);
    send_to_char("\nTemperature: ", ch);
    send_to_char(temp_patterns[clime->season_temp[seas] - 1], ch);

    send_to_char("\n&+YSector statics, not affected by seasons:&n", ch);
    sprintf(buf, "\nTemp: %dc %df  Humidity: %d  Pressure: %d\n", cond->temp,
            (9 / 5 * (cond->temp) + 32), cond->humidity, cond->pressure);

    send_to_char(buf, ch);

    sprintf(buf,
            "Windspeed: %d  Direction: %d  Precip Rate: %d  Precip Depth: %d\n",
            cond->windspeed, cond->wind_dir, cond->precip_rate,
            cond->precip_depth);

    send_to_char(buf, ch);

    sprintf(buf,
            "Light: %d  Energy: %d  Pressure change: %d  Precip change: %d\n",
            cond->ambient_light, cond->free_energy, cond->pressure_change,
            cond->precip_change);

    send_to_char(buf, ch);
    send_to_char("\nAnd the mortals see it as:\n", ch);
  }
  if (!OUTSIDE(ch))
  {
    send_to_char
      ("How can you know what the weather's like when you are inside?\n\n",
       ch);
    return;
  }
  strcpy(buf, " ");
  if (cond->precip_rate)
  {
    if (cond->temp <= 0)
      strcat(buf, "It's snowing");
    else
      strcat(buf, "It's raining");
    if (cond->precip_rate > 65)
      strcat(buf, " extremely hard");
    else if (cond->precip_rate > 50)
      strcat(buf, " very hard");
    else if (cond->precip_rate > 30)
      strcat(buf, " hard");
    else if (cond->precip_rate < 15)
      strcat(buf, " lightly");
    strcat(buf, ", ");
  }
  else
  {
    if (cond->humidity > 80)
      strcat(buf, "It's very cloudy, ");
    else if (cond->humidity > 55)
      strcat(buf, "It's cloudy, ");
    else if (cond->humidity > 25)
      strcat(buf, "It's partly cloudy, ");
    else if (cond->humidity)
      strcat(buf, "It's mostly clear, ");
    else
      strcat(buf, "It's clear, ");
  }
  if (cond->temp > 100)
    strcat(buf, "boiling, ");
  else if (cond->temp > 80)
    strcat(buf, "blistering, ");
  else if (cond->temp > 50)
    strcat(buf, "incredibly hot, ");
  else if (cond->temp > 40)
    strcat(buf, "very, very hot, ");
  else if (cond->temp > 30)
    strcat(buf, "very hot, ");
  else if (cond->temp > 24)
    strcat(buf, "hot, ");
  else if (cond->temp > 18)
    strcat(buf, "warm, ");
  else if (cond->temp > 9)
    strcat(buf, "mild, ");
  else if (cond->temp > 3)
    strcat(buf, "cool, ");
  else if (cond->temp > -1)
    strcat(buf, "cold, ");
  else if (cond->temp > -10)
    strcat(buf, "freezing, ");
  else if (cond->temp > -25)
    strcat(buf, "well past freezing, ");
  else
    strcat(buf, "numbingly frozen, ");

  strcat(buf, "and ");

  if (cond->windspeed <= 0)
    strcat(buf, "there is absolutely no wind");
  else if (cond->windspeed < 10)
    strcat(buf, "calm");
  else if (cond->windspeed < 20)
    strcat(buf, "breezy");
  else if (cond->windspeed < 35)
    strcat(buf, "windy");
  else if (cond->windspeed < 50)
    strcat(buf, "very windy");
  else if (cond->windspeed < 70)
    strcat(buf, "very, very windy");
  else if (cond->windspeed < 100)
    strcat(buf, "there is a gale blowing");
  else
    strcat(buf, "the wind is unbelievable");
  strcat(buf, ".\n");
  send_to_char(buf, ch);
  if (GET_CLASS(ch, CLASS_CLERIC) ||
      GET_CLASS(ch, CLASS_RANGER) ||
      GET_CLASS(ch, CLASS_MONK) ||
      GET_CLASS(ch, CLASS_DRUID) ||
      GET_CLASS(ch, CLASS_SHAMAN) ||
      GET_CLASS(ch, CLASS_PALADIN) ||
      GET_CLASS(ch, CLASS_ANTIPALADIN) ||
      GET_CLASS(ch, CLASS_NECROMANCER) || GET_CLASS(ch, CLASS_SUMMONER) ||
      GET_CLASS(ch, CLASS_CONJURER) || GET_CLASS(ch, CLASS_SORCERER))
  {
    if (cond->free_energy > 40000)
      send_to_char("Wow! This place is bursting with energy!\n", ch);
    else if (cond->free_energy > 30000)
      send_to_char("The environs tingle your magical senses.\n", ch);
    else if (cond->free_energy > 20000)
      send_to_char("The area is rich with energy.\n", ch);
    else if (cond->free_energy < 4000)
      send_to_char("There is almost no magical energy here.\n", ch);
    else if (cond->free_energy < 5000)
      send_to_char
        ("Your magical senses are dulled by the scarceness of energy here.\n",
         ch);
  }
  send_to_char("\n", ch);
}

void do_help(P_char ch, char *argument, int cmd)
{
  char *attribs;

  send_to_char( wiki_help(string(argument)).c_str(), ch );
  send_to_char( "\n", ch );

  attribs = attrib_help(argument);
  if( attribs )
    send_to_char( attribs, ch );
}

void do_wizhelp(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH];
  int      no, i;
  bool     found;
  P_char   target = NULL;

  if( IS_NPC(ch) )
  {
    return;
  }

  // Argument should be a valid char name.
  if( argument && *argument && (target = get_char_vis(ch, argument)) == NULL )
  {
    send_to_char("No one by that name here..\n", ch);
    return;
  }
  if( target && target != ch && GET_LEVEL(target) >= GET_LEVEL(ch) )
  {
    act("$N is not smaller than you, you can not look at their commands.", FALSE, ch, 0, target, TO_CHAR);
    return;
  }

  sprintf(buf, "The following privileged commands are available to %s:\n\n", target ? J_NAME(target) : "you");
  send_to_char( buf, ch );

  if( !target )
  {
    target = ch;
  }
  *buf = '\0';

  for( no = 0, i = 1; *command[i - 1] != '\n'; i++ )
  {
    found = FALSE;
    if( cmd_info[i].minimum_level > MAXLVLMORTAL )
    {
      if (can_exec_cmd(target, i))
      {
/*(cmd_info[i].minimum_level <= GET_LEVEL(ch)) || is_cmd_granted(ch, i)) {*/
        sprintf(buf + strlen(buf), "[&+y%2d&n] &+c%-14s&n",
                cmd_info[i].minimum_level, command[i - 1]);
        no++;
        found = TRUE;
      }
    }
    if( found && !(no % 4) )
    {
      strcat(buf, "\n");
    }
  }
  if( *buf )
  {
    strcat(buf, "\n");
    page_string(ch->desc, buf, 1);
  }
  else
  {
    send_to_char("None, go away.\n", ch);
  }
}

const char *get_multiclass_name(P_char ch)
{
 int i = 0;

  if (!IS_MULTICLASS_PC(ch))
    return "error #1";

  while (multiclass_names[i].cls1 != -1)
  {
    if ((multiclass_names[i].cls1 == ch->player.m_class) &&
        (multiclass_names[i].cls2 == ch->player.secondary_class))
      return multiclass_names[i].mc_name;

    if ((multiclass_names[i].cls2 == ch->player.m_class) &&
        (multiclass_names[i].cls1 == ch->player.secondary_class))
      return multiclass_names[i].mc_name;
    i++;
  }

  return "error #2";
}



const char *get_class_name(P_char ch, P_char tch)
{
  if (IS_MULTICLASS_PC(ch))
    return get_multiclass_name(ch);
  if (IS_SPECIALIZED(ch) && (IS_SET(tch->specials.act2, PLR2_SPEC)))
  {
    return GET_SPEC_NAME(ch->player.m_class, ch->player.spec-1);
  }
  return class_names_table[flag2idx(ch->player.m_class)].ansi;
}

int compare_char_data(const void *char1, const void *char2)
{
  return ((*(P_char *) char2)->player.level) -
    ((*(P_char *) char1)->player.level);
}

void do_who(P_char ch, char *argument, int cmd)
{
  P_char   who_list[MAX_WHO_PLAYERS], who_gods[MAX_WHO_PLAYERS];
  struct time_info_data playing_time;
  P_char   tch, tchReal;
  P_desc   d;
  char     who_output[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
  char     buf4[MAX_STRING_LENGTH], buf5[MAX_STRING_LENGTH];
  char     pattern[256], arg[256];
  int      i, j, k, nr_args_now = 0, who_list_size = 0, who_gods_size = 0, total_ingame_connections = 0;
  long     timer = 0;
  snoop_by_data *snoop_by_ptr;
  int      align = RACEWAR_NONE, min_level = MAXLVL + 1, max_level = -1;
  bool     sort = FALSE, zone = FALSE, lfg = FALSE, mortalsonly = FALSE;
  struct affected_type *pafepics;

  if( !IS_ALIVE(ch) || IS_NPC(ch) )
  {
    return;
  }
  if( IS_MORPH(ch) )
  {
    send_to_char("Morphs do not possess the extra-sensory organs necessary to detect who all is in the world at the moment.\n", ch);
    return;
  }

  *pattern = 0;

  while (*argument)
  {
    argument = one_argument(argument, arg);
    if (!*arg)
      ;
    else if( is_abbrev(arg, "good") && IS_TRUSTED(ch) )
      align = RACEWAR_GOOD;
    else if( is_abbrev(arg, "evil") && IS_TRUSTED(ch) )
      align = RACEWAR_EVIL;
    else if( is_abbrev(arg, "undead") && IS_TRUSTED(ch) )
      align = RACEWAR_UNDEAD;
    else if( (is_abbrev(arg, "illithid") || is_abbrev(arg, "neutral")) && IS_TRUSTED(ch) )
      align = RACEWAR_NEUTRAL;
    else if( is_abbrev(arg, "zone") && IS_TRUSTED(ch) )
      zone = TRUE;
    else if( is_abbrev(arg, "god") )
      align = -1;
    else if( is_abbrev(arg, "sort") )
      sort = TRUE;
    else if( !strcmp(arg, "short") || is_abbrev(arg, "mortals") )
      mortalsonly = TRUE;
    else if( is_abbrev(arg, "lfg") )
      lfg = TRUE;
    else if( !strcmp(arg, "?") )
    {
      send_to_char("Usage:\n", ch);
      send_to_char("&+Ywho &+W[<level> | <minlevel> <maxlevel>] [<pattern>] [s]&n\n\r", ch);
      send_to_char("where pattern is matched against class, race, name, of visible players.\n\r", ch );
      send_to_char("Other options include hardcore, spec, multi, newbie, guide, short, and god.\n\r", ch );
      return;
    }
    else
    {
      if( atoi(arg) > 0 )
      {
        min_level = MIN(atoi(arg), min_level);
        max_level = MAX(atoi(arg), max_level);
        sort = TRUE;
      }
      else
        strcpy(pattern, arg);
    }
  }
  if( min_level == MAXLVL + 1 )
  {
    min_level = 0;
  }
  if( max_level == -1 )
  {
    max_level = MAXLVL;
  }

  if (GET_RACE(ch) == RACE_ILLITHID )
  {
    if( GET_LEVEL(ch) < 20 )
    {
      send_to_char("You haven't mastered your mind control enough yet!\n", ch);
      return;
    }
    if( !IS_TRUSTED(ch) )
    {
      i = MAX(1, GET_MAX_MANA(ch) - GET_LEVEL(ch));

      if( (GET_MANA(ch) - i) < 0 )
      {
        send_to_char("You do not have enough mana.\n", ch);
        return;
      }
      GET_MANA(ch) -= i;
      if (GET_MANA(ch) < GET_MAX_MANA(ch))
        StartRegen(ch, EVENT_MANA_REGEN);
    }
  }

  for( d = descriptor_list; d; d = d->next )
  {
    tch = d->character;

    if( d->connected != CON_PLYNG )
      continue;

    total_ingame_connections++;

    // Anyone can now see invis people other than gods on who list - Drannak
    if( racewar(ch, tch) || IS_NPC(tch) || (!CAN_SEE(ch, tch) && IS_TRUSTED(tch)) )
      continue;

    // Gods can see all!
    if( !IS_TRUSTED(ch) )
    {
      if( IS_DISGUISE(tch) || IS_SET(tch->specials.act, PLR_NOWHO) )
// Anon removed atm.
//        || (sort && IS_SET(tch->specials.act, PLR_ANONYMOUS)) )
      {
        continue;
      }
    }

    if( GET_LEVEL(tch) < min_level || GET_LEVEL(tch) > max_level )
    {
      continue;
    }

    if( align != RACEWAR_NONE )
    {
      // If you remove the align == -1 from below, gods will show to gods on all who lists (except mortals-only lists).
      if( (IS_TRUSTED(tch) && align == -1) || GET_RACEWAR(tch) == align )
      {
        if (IS_TRUSTED(tch))
          who_gods[who_gods_size++] = tch;
        else
          who_list[who_list_size++] = tch;
      }
      continue;
    }

    if( zone && world[ch->in_room].zone != world[tch->in_room].zone )
    {
      continue;
    }

    if( lfg && !IS_SET(tch->specials.act2, PLR2_LGROUP) )
    {
      continue;
    }

    if( !*pattern || is_abbrev(pattern, tch->player.name)
      || is_abbrev(pattern, race_names_table[GET_RACE(tch)].normal)
      || (is_abbrev(pattern, "hardcore") && IS_HARDCORE(tch))
      || (is_abbrev(pattern, "spec") && IS_SPECIALIZED(tch))
      || (is_abbrev(pattern, "multi") && IS_MULTICLASS_PC(tch))
      || (is_abbrev(pattern, "newbie") && (IS_TRUSTED(ch) || IS_NEWBIE_GUIDE(ch)) && IS_NEWBIE(tch))
      || (is_abbrev(pattern, "guide") && IS_NEWBIE_GUIDE(tch))
      || ( (IS_TRUSTED(ch) || !IS_SET(tch->specials.act, PLR_ANONYMOUS))
      && is_abbrev(pattern, class_names_table[flag2idx(tch->player.m_class)].normal) ) )
    {
      if( IS_TRUSTED(tch) )
        who_gods[who_gods_size++] = tch;
      else
        who_list[who_list_size++] = tch;
    }
  }

  if( sort )
  {
    qsort(who_gods, who_gods_size, sizeof(P_char), compare_char_data);
    qsort(who_list, who_list_size, sizeof(P_char), compare_char_data);
  }

  *buf = '\0';
  if( mortalsonly )
  {
    k = 1;
    sprintf(who_output, "\n Short Listing for Mortals\n-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
    if( who_list_size == 0 )
      strcat(who_output, "<None>\n");
    else
    {
      for( j = 0; j < who_list_size; j++ )
      {
        sprintf(who_output + strlen(who_output), "[&+w%2d&N%-3s&N]", GET_LEVEL(who_list[j]),
          class_names_table[flag2idx(who_list[j]->player.m_class)].code);
        sprintf(who_output + strlen(who_output), " %-15s%s", GET_NAME(who_list[j]), (!(k++ % 3) ? "\n" : ""));
      }
    }
    strcat(who_output, "\n");
    send_to_char(who_output, ch, LOG_NONE);
  }
  else
  {
    strcpy(who_output, " Listing of the Gods\n" "-=-=-=-=-=-=-=-=-=-=-\n");
    for( j = 0; j < who_gods_size; j++ )
    {
      if( GET_LEVEL(who_gods[j]) == OVERLORD )
        strcat(who_output, "[  &+rOverlord&n   ] ");
      else if (GET_LEVEL(who_gods[j]) == FORGER)
        strcat(who_output, "[   &+LForger&n    ] ");
      else if (GET_LEVEL(who_gods[j]) == IMMORTAL)
        strcat(who_output, "[  &+cImmortal&n   ] ");
      else if (GET_LEVEL(who_gods[j]) == LESSER_G)
        strcat(who_output, "[ &+yLesser God&n  ] ");
      else if (GET_LEVEL(who_gods[j]) == GREATER_G)
        strcat(who_output, "[ &+MGreater God&n ] ");
      else if (GET_LEVEL(who_gods[j]) == AVATAR)
        strcat(who_output, "[   &+RAvatar&n    ] ");
      else
        strcat(who_output, "[   &=LCCheater&n   ] ");

      strcat(who_output, GET_NAME(who_gods[j]));
      strcat(who_output, " ");
      if( GET_TITLE(who_gods[j]) )
        strcat(who_output, GET_TITLE(who_gods[j]));
      if( who_gods[j]->desc && who_gods[j]->desc->olc )
        strcat(who_output, " (&+MOLC&N)");
      if( IS_SET((who_gods[j])->specials.act, PLR_AFK) )
        strcat(who_output, " (&+RAFK&N)");


      if( IS_TRUSTED(ch) )
      {
        sprintf(who_output + strlen(who_output), " (%d)",
                who_gods[j]->only.pc->wiz_invis);
        if( IS_AFFECTED(who_gods[j], AFF_INVISIBLE) || IS_AFFECTED2(who_gods[j], AFF2_MINOR_INVIS)
          || IS_AFFECTED3(who_gods[j], AFF3_ECTOPLASMIC_FORM) )
        {
          strcat(who_output, " (inv)");
        }
      }
      strcat(who_output, "\n");
    }
    if( who_gods_size == 0 )
    {
      strcat(who_output, "<None>\n");
    }
    sprintf(who_output + strlen(who_output), "\nThere are %d visible &+rgod(s)&n on.\n", who_gods_size);
    strcpy(buf, "");
    if( who_list_size > 0 )
    {
      strcat(who_output, "\n Listing of the Mortals!\n" "-=-=-=-=-=-=-=-=-=-=-=-=-\n");
      for( j = 0; j < who_list_size; j++ )
      {
        if( !IS_SET((who_list[j])->specials.act, PLR_ANONYMOUS) || IS_TRUSTED(ch) )
        {
          sprintf(who_output + strlen(who_output), "&n[&+w%2d&n%s%s&n]%s", GET_LEVEL(who_list[j]),
            (IS_TRUSTED(ch) && (IS_SET(who_list[j]->specials.act, PLR_ANONYMOUS)) ? "*" : " "),
            pad_ansi( get_class_name(who_list[j], ch), 16, TRUE).c_str(),
            (IS_TRUSTED(ch) &&(IS_SET(who_list[j]->specials.act, PLR_NOWHO)) ? "&+r%&n" : " "));
        }
        else
        {
          strcat(who_output, "[&+L - Anonymous -  &N] ");
        }

        strcat(who_output, GET_NAME(who_list[j]));

        if( GET_TITLE(who_list[j]) )
        {
          strcat(who_output, " ");
          strcat(who_output, GET_TITLE(who_list[j]));
        }

        if( IS_AFFECTED(who_list[j], AFF_INVISIBLE) || IS_AFFECTED2(who_list[j], AFF2_MINOR_INVIS)
          || IS_AFFECTED3(who_list[j], AFF3_ECTOPLASMIC_FORM) )
        {
          strcat(who_output, " (inv)");
        }

        strcat(who_output, " &N(");
        strcat(who_output, race_names_table[(int) GET_RACE(who_list[j])].ansi);
        strcat(who_output, "&N)");

        if( who_list[j]->desc && who_list[j]->desc->olc )
          strcat(who_output, " (&+MOLC&N)");
        if( IS_HARDCORE(who_list[j]) )
          strcat(who_output, "&+L (&+rHard&+RCore&+L)&n");
        if( IS_SET((who_list[j])->specials.act, PLR_AFK) )
          strcat(who_output, " (&+RAFK&N)");
        if( IS_SET((who_list[j])->specials.act2, PLR2_LGROUP) )
          strcat(who_output, " (&+WGroup Needed&N)");
        if( IS_SET((who_list[j])->specials.act2, PLR2_NEWBIE_GUIDE) )
          strcat(who_output, " (&+GGuide&N)");
        if( IS_SET((who_list[j])->specials.act3, PLR3_FRAGLEAD) )
          strcat(who_output, " (&+rF&+Rr&+ra&+Rg &+rL&+Ro&+rr&+Rd&n&N)");

        // Surtitles - Drannak ... You did this so wrong.. should've used a mask and numbers... not on/off bits.
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURSERF))
          strcat(who_output, "&n[&+ySerf&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURCOMMONER))
          strcat(who_output, "&n[&+YCommoner&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURKNIGHT))
          strcat(who_output, "&n[&+LK&+wn&+Wig&+wh&+Lt&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURNOBLE))
          strcat(who_output, "&n[&+mN&+Mobl&+me&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURLORD))
          strcat(who_output, "&n[&+rL&+Ror&+rd&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURKING))
          strcat(who_output, "&n[&+yK&+Yin&+yg&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURLIGHT))
          strcat(who_output, "&n[&+WLight&+wbri&+Lnger&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURDRAGON))
          strcat(who_output, "&n[&+gDr&+Gag&+Lon &+gS&+Glaye&+gr&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURHEALS))
          strcat(who_output, "&n[&+WD&+Ro&+rct&+Ro&+Wr&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURSERIAL))
          strcat(who_output, "&n[&+LSe&+wr&+Wi&+wa&+Ll &+rKiller&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURREAPER))
          strcat(who_output, "&n[&+LGr&+wi&+Wm Rea&+wp&+Ler&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURDECEPTICON))
          strcat(who_output, "&n[&+LDe&+mCePTiC&+LoN&n]");
        if(IS_SET((who_list[j])->specials.act3, PLR3_SURDEATHSDOOR))
          strcat(who_output, "&n[&+MTo&+mug&+Mh G&+muy&n]");

        /* No frag food title? :(
        if( IS_SET((who_list[j])->specials.act3, PLR3_FRAGLOW) )
          strcat(who_output, " (&+yFrag &+YFood&n&N)");
        */

        if( ( IS_TRUSTED(ch) || IS_NEWBIE_GUIDE(ch) ) && IS_NEWBIE(who_list[j]) )
        {
          strcat(who_output, " (&+GNewbie&N)");
        }

        strcat(who_output, "\n");
        if( strlen(who_output) > (MAX_STRING_LENGTH - 175) )
        {
          strcat(who_output, "-= List too long, use 'who <arg> =-'\n");
          break;
        }
      }

      sprintf(who_output + strlen(who_output),
        "\nThere are %d &+gmortal(s)&n on.\n\n&+cTotal visible players: %d.&N\n"
        "&+cTotal in-game connections: %d.&N\n\n&+rRecord number of connections this boot: %d.&n\n",
        who_list_size, who_list_size + who_gods_size, total_ingame_connections, max_descs);
    }
    send_to_char(who_output, ch, LOG_NONE);
    strcpy(who_output, "");
  }

  // This is brief info about a player available to gods only
  if( IS_TRUSTED(ch) && (who_list_size + who_gods_size == 1) )
  {
    tch = who_list_size ? who_list[0] : who_gods[0];

    if( strcasecmp(tch->player.name, pattern) )
      return;

    if( (GET_LEVEL(ch) >= FORGER) && tch->desc && tch->desc->snoop.snoop_by_list )
    {
      bool seen = FALSE;

      for( snoop_by_ptr = tch->desc->snoop.snoop_by_list; snoop_by_ptr; snoop_by_ptr = snoop_by_ptr->next )
      {
        if( WIZ_INVIS( ch, snoop_by_ptr->snoop_by) )
        {
          continue;
        }
        if( !seen )
        {
          sprintf(who_output, " (Snooped By: %s", GET_NAME(snoop_by_ptr->snoop_by));
          seen = TRUE;
        }
        else
        {
          sprintf(who_output + strlen(who_output), ", %s", GET_NAME(snoop_by_ptr->snoop_by));
        }
      }
      if( seen )
      {
        strcat(who_output, ")\n\r");
      }
    }

    timer = tch->desc->character->specials.timer;
    tchReal = (tch->desc->original != NULL) ? tch->desc->original : tch->desc->character;
    sprinttype(tch->desc->connected, connected_types, buf5);

    sprintf(buf4, "Desc: %3d, Idle: %3ld, Lvl:%3d, Switched: %c, Name: %s\n\r"
      "Room: %6d, Vis level: %d, Connection: %-11s %15s\n\r\n\r",
      tch->desc->descriptor, timer, GET_LEVEL( tchReal ), (tch->desc->original) ? 'Y' : 'N', GET_NAME(tchReal),
      (tchReal->in_room > NOWHERE) ? world[tchReal->in_room].number : -1, tchReal->only.pc->wiz_invis, buf5,
      (tch->desc->host) ? tch->desc->host : "UNKNOWN" );

    strcat(who_output, buf4);
    send_to_char(who_output, ch);

    if( USES_MANA(tch) )
    {
      sprintf(buf, "      Hit Points = %d(%d),  Mana = %d(%d),  Movement Points = %d(%d)\n",
        GET_HIT(tch), GET_MAX_HIT(tch), GET_MANA(tch), GET_MAX_MANA(tch), GET_VITALITY(tch), GET_MAX_VITALITY(tch));
    }
    else
    {
      sprintf(buf, "      Hit Points = %d(%d), Movement Points = %d(%d), Alignment = %d\n",
        GET_HIT(tch), GET_MAX_HIT(tch), GET_VITALITY(tch), GET_MAX_VITALITY(tch), GET_ALIGNMENT(tch));
    }

    sprintf(buf3, "      Experience to next level = %d.\n",
            (new_exp_table[GET_LEVEL(tch) + 1] - GET_EXP(tch)));
    strcat(buf, buf3);

    pafepics = get_spell_from_char(tch, TAG_EPICS_GAINED);
    sprintf(buf3, "      Epic points = %ld.   Total epics gained = %d.\n",
      tch->only.pc->epics, pafepics ? pafepics->modifier : 0 );
    strcat(buf, buf3);

    sprintf(buf3, "      Stats: STR = %d, DEX = %d, AGI = %d, CON = %d, LUCK = %d\n"
      "             POW = %d, INT = %d, WIS = %d, CHA = %d\n",
      GET_C_STR(tch), GET_C_DEX(tch), GET_C_AGI(tch), GET_C_CON(tch), GET_C_LUK(tch),
      GET_C_POW(tch), GET_C_INT(tch), GET_C_WIS(tch), GET_C_CHA(tch));
    strcat(buf, buf3);

#ifndef EQ_WIPE
    playing_time = real_time_passed((long)((time(0) - tch->player.time.logon) + tch->player.time.played), 0);
#else
    playing_time = real_time_passed((long)((time(0) - tch->player.time.logon) + tch->player.time.played - EQ_WIPE), 0);
#endif
    sprintf(buf3, "      Playing time = %d days and %d hours, Age = %d years\n",
      playing_time.day, playing_time.hour, GET_AGE(tch));
    strcat(buf, buf3);

    sprintf(buf3, "      Client: %s&n\n", (strlen(tch->desc->client_str) > 2) ? tch->desc->client_str : "Unknown" );
    strcat(buf, buf3);

    send_to_char(buf, ch);
  }
}

int got_dupe_host(P_desc orig)
{
  P_desc   d = descriptor_list;

  while (d)
  {
    if ((d != orig) && d->host && !strcasecmp(orig->host, d->host))
    {
      return TRUE;
    }

    d = d->next;
  }

  return FALSE;
}

void do_users_DEPRECATED(P_char ch, char *argument, int cmd);

void do_users(P_char ch, char *argument, int cmd)
{
  char linebuf[MAX_STRING_LENGTH], connbuf[MAX_STRING_LENGTH], hostbuf[MAX_STRING_LENGTH];
  bool nonplaying = FALSE;
  bool getname = FALSE;

  half_chop(argument, linebuf, connbuf);

  // when we are sure we don't need the old do_users, remove it completely
  if( !strcmp("old", linebuf) )
  {
    do_users_DEPRECATED(ch, connbuf, cmd);
    return;
  }
  else if( linebuf && *linebuf && is_abbrev(linebuf, "nonplaying") )
  {
    nonplaying = TRUE;
  }
  else if( linebuf && *linebuf && is_abbrev(linebuf, "get_name") )
  {
    getname = TRUE;
  }

  send_to_char("\r\n Character   |  #  | State       | Idle | Hostname\r\n-----------------------------------------------------------------------------------\r\n", ch);
  for( P_desc d = descriptor_list; d; d = d->next )
  {
    // don't show admins of higher level who are invisible
    if( d->character && IS_PC(d->character) && WIZ_INVIS(ch, d->character) )
    {
      continue;
    }

    // non-playing chars only.
    if( nonplaying && !(d->connected) )
    {
      continue;
    }
    // Descriptors at get name screen only.
    if( getname && (d->connected != CON_NME) )
    {
      continue;
    }

    sprinttype(d->connected, connected_types, connbuf);

    if( d->host )
    {
      if( !*d->host2 )
      {
        sprintf(hostbuf, "lib/etc/hosts/%d", d->descriptor);
        FILE *f = fopen(hostbuf, "r");

        if( f != NULL )
        {
          if( fscanf(f, "%s\n", hostbuf) == 1 )
          {
            strncpy(d->host2, hostbuf, 128);
          }
          else
          {
            strncpy(d->host2, d->host, 128);
          }

          fclose(f);
        }
      }

      if( got_dupe_host(d) )
      {
        sprintf(hostbuf, "&+R%s (%s)&n", d->host2, d->host);
      }
      else
      {
        sprintf(hostbuf, "%s (%s)", d->host2, d->host);
      }
    }
    else
    {
      sprintf(hostbuf, "&+Yunknown&n");
    }

    sprintf(linebuf, " %s | %3d | %s | %4d | %s\r\n",
      ((d->character && GET_NAME(d->character)) ? pad_ansi(GET_NAME(d->character), 11).c_str() : "-          "),
      d->descriptor, pad_ansi(connbuf, 11).c_str(), (d->wait / 240), hostbuf );

    send_to_char(linebuf, ch);
  }
}


/*
 * changed to show linkdeads as well, checks character_list, rather than
 * descriptor_list, added some features too. JAB
 */
/* deprecated by Torgal 12/21/09 */
void do_users_DEPRECATED(P_char ch, char *argument, int cmd)
{
  char     biglist[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  char     buf2[MAX_INPUT_LENGTH], line[MAX_INPUT_LENGTH],
    name[MAX_INPUT_LENGTH];
  char     temp_buf[MAX_INPUT_LENGTH];
  int      num_link_dead = 0, num_in_game = 0, num_non_play = 0, num_mccp =
    0, b_len = 160;
  int      num_client = 0;
  long     timer = 0;
  P_char   t_ch;
  P_desc   d = NULL;
  snoop_by_data *snoop_by_ptr;
  FILE    *f;
  int      mccp_ratio;

  biglist[0] = '\0';

  half_chop(argument, name, line);

  strcpy(biglist, "&+RNon-playing sockets:\n--------------------\n");

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->character)
      t_ch = d->character;
    else
      t_ch = NULL;

    if (!d->connected)          /*
                                 * * * want non-playing sockets only
                                 */
      continue;

    if (t_ch && (GET_LEVEL(t_ch) > GET_LEVEL(ch)))
      continue;                 /*
                                 * hide status on higher levels
                                 */

    if (*name)
      if ((!t_ch || !t_ch->player.name || !isname(t_ch->player.name, name)) &&
          (!d->host || !strstr(d->host, name)))
        continue;

    num_non_play++;

    sprintf(line, "%3d I:%3d", d->descriptor, d->wait / 240);

    if (d && d->z_str)
    {
      mccp_ratio = compress_get_ratio(d);
      sprintf(line + strlen(line), " C:%s%3d%%&n",
              mccp_ratio > 0 ? "&n" : "&+R", mccp_ratio);
      num_mccp++;
    }
    else
      strcat(line, " C:  - ");

    if (d && strlen(d->client_str) > 2)
    {
      one_argument(d->client_str, temp_buf);
      sprintf(line + strlen(line), " Client:&+C%s&n", temp_buf);
      num_client++;
    }
    else
      strcat(line, " Client:  - ");

    if (t_ch && t_ch->player.name)
      sprintf(line + strlen(line), "     %-15s", GET_NAME(t_ch));
    else
      strcat(line, "     &+mNONE&n           ");

    sprinttype(d->connected, connected_types, buf2);
    sprintf(line + strlen(line), "  %11s", buf2);

    /*
     * Get IP Address
     */
    if (d->host)
    {
      if (!*d->host2)
      {
        sprintf(buf2, "lib/etc/hosts/%d", d->descriptor);
        f = fopen(buf2, "r");

        if (f != NULL)
        {
          if (fscanf(f, "%s\n", buf, buf2) == 1)
          {
            strncpy(d->host2, buf, 128);
          }
          else
            strncpy(d->host2, d->host, 128);
          fclose(f);
        }
      }
      if (d->login)
      {
        sprintf(line + strlen(line), " %8s %s%s&n", d->login,
                got_dupe_host(d) ? "&+R" : "&+Y", d->host2);
      }
      else
      {
        sprintf(line + strlen(line), "         %s%s&n",
                got_dupe_host(d) ? "&+R" : "&+Y", d->host2);
      }
    }
    else
    {
      sprintf(line + strlen(line), " &+CUNKNOWN&n        ");
    }


    strcat(line, "\n");
    b_len += strlen(line);
    if (b_len > MAX_STRING_LENGTH)
    {
      strcat(biglist, "List too long, use 'users <arg>'\n");
      page_string(ch->desc, biglist, 1);
      return;
    }
    strcat(biglist, line);
  }

  strcat(biglist, "\n&+GCharacters:\n-----------\n");

  b_len = strlen(biglist) + 160;

  for (t_ch = character_list; t_ch; t_ch = t_ch->next)
  {
    /*
     * handleswitched and shapechanged using NPC body, since that's
     * where the desc will be.
     */
    if (IS_NPC(t_ch) && !t_ch->desc)
      continue;

    if (IS_PC(t_ch) && t_ch->only.pc->switched) /*
                                                 * * * don't list * *
                                                 * switched chars * *
                                                 * real bodies
                                                 */
      continue;

    if (t_ch->desc)
      d = t_ch->desc;
    else
      d = NULL;

    /*
     * minor prob with shapechange, there is no difference between a
     * link dead char, and one that is shapechanged.  So, all linkdead
     * bodies in room 0 (Void), where shapechangers real bodies are
     * stored, do not show up on this list.  JAB
     */

    if (!d && (t_ch->in_room == 0))
      continue;

    if (IS_PC(t_ch) && t_ch->player.name && WIZ_INVIS(ch, t_ch))
      continue;

    if (*name)
      if ((!t_ch->player.name || !isname(t_ch->player.name, name)) &&
          (!d || !d->host || !strstr(d->host, name)))
        continue;

    if (d && d->original)
      strcpy(line, " S ");
    else
      strcpy(line, "   ");

    if (t_ch->player.name)
      sprintf(line + strlen(line), "%-15s",
              (d &&
               d->original) ? (d->original->player.name) : (t_ch->player.
                                                            name));
    else
      strcpy(line, "&+RUNDEFINED&n          ");

    timer = t_ch->specials.timer;       /*
                                         * idle time
                                         */

    if (t_ch->in_room > NOWHERE)
      sprintf(line + strlen(line), " [%6d] ", world[t_ch->in_room].number);
    else
      strcat(line, "  ??????  ");

    if (d && d->original && IS_PC(d->original) &&
        (d->original->only.pc->wiz_invis != 0))
      sprintf(line + strlen(line), "%2d ", d->original->only.pc->wiz_invis);
    else if (IS_PC(t_ch) && (t_ch->only.pc->wiz_invis != 0))
      sprintf(line + strlen(line), "%2d ", t_ch->only.pc->wiz_invis);
    else
      strcat(line, "   ");

    if (d)
    {
      num_in_game++;

      /*
       * Get IP Address
       */
      if (d->host)
      {
        if (!*d->host2)
        {
          sprintf(buf2, "lib/etc/hosts/%d", d->descriptor);
          f = fopen(buf2, "r");

          if (f != NULL)
          {
            if (fscanf(f, "%s\n", buf, buf2) == 1)
            {
              strncpy(d->host2, buf, 128);
            }
            else
              strncpy(d->host2, d->host, 128);
            fclose(f);
          }
        }
        if (d->login)
        {
          sprintf(line + strlen(line), " %8s %s%s&n", d->login,
                  got_dupe_host(d) ? "&+R" : "&+Y", d->host2);
        }
        else
        {
          sprintf(line + strlen(line), "         %s%s&n",
                  got_dupe_host(d) ? "&+R" : "&+Y", d->host2);
        }
      }
      else
      {
        sprintf(line + strlen(line), " &+CUNKNOWN&n        ");
      }

    }
    else
    {
      num_link_dead++;
      strcat(line, " &+BLINK DEAD&n       ");
    }

    if (d && d->snoop.snoop_by_list && (GET_LEVEL(ch) >= 62))
    {
      snoop_by_ptr = d->snoop.snoop_by_list;

      sprintf(line + strlen(line), " &+g(S: %s",
              GET_NAME(snoop_by_ptr->snoop_by));

      snoop_by_ptr = snoop_by_ptr->next;
      while (snoop_by_ptr)
      {
        sprintf(line + strlen(line), ", %s",
                GET_NAME(snoop_by_ptr->snoop_by));

        snoop_by_ptr = snoop_by_ptr->next;
      }

      strcat(line, ")&n");
    }

    strcat(line, "\n");
    if (d)
      sprintf(buf, "%3d I:%3ld  ", d->descriptor, timer);
    else
      sprintf(buf, "&+B---&n I:%3ld  ", timer);

    if (d && d->z_str)
    {
      mccp_ratio = compress_get_ratio(d);
      sprintf(buf + strlen(buf), " C:%s%3d%%&n",
              mccp_ratio > 0 ? "&n" : "&+R", mccp_ratio);
      num_mccp++;
    }
    else
      strcat(buf, " C:  - ");

    if (d && strlen(d->client_str) > 2)
    {
      one_argument(d->client_str, temp_buf);
      sprintf(buf + strlen(buf), " Client:&+C%s&n", temp_buf);
      num_client++;
    }
    else
      strcat(buf, " Client:  - ");

    strcat(buf, line);
    b_len += strlen(buf);
    if (b_len > MAX_STRING_LENGTH)
    {
      strcat(biglist, "List too long, use 'users <arg>'\n");
      page_string(ch->desc, biglist, 1);
      return;
    }
    strcat(biglist, buf);
  }

  sprintf(line,
          "\nNon-playing: %d  In game: %d  Using Client: %d Linkdeads: %d  Sockets: %d  Using compression: %d\n",
          num_non_play, num_in_game, num_client, num_link_dead,
          (num_non_play + num_in_game), num_mccp);
  strcat(biglist, line);

  page_string(ch->desc, biglist, 1);
}

void do_inventory(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH];
  int      i;

  if( IS_AFFECTED(ch, AFF_WRAITHFORM) )
  {
    send_to_char("You have no place to keep anything!\n", ch);
  }
  else
  {
    sprintf(buf, "You are carrying: (%d/%d)\n", IS_CARRYING_N(ch), CAN_CARRY_N(ch));
    send_to_char(buf, ch);
    list_obj_to_char(ch->carrying, ch, 1, TRUE);
  }
}

// Returns true iff ch can wear something in wear_slot slot.
bool has_eq_slot( P_char ch, int wear_slot )
{

  switch (wear_slot)
  {
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
      if( IS_THRIKREEN(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_NECK_1:
    case WEAR_NECK_2:
      return TRUE;
      break;
    case WEAR_BODY:
//      if( has_innate( ch, INNATE_SPIDER_BODY ) || has_innate( ch, INNATE_HORSE_BODY ) || IS_THRIKREEN(ch) )
      if( IS_THRIKREEN(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_HEAD:
      if( IS_MINOTAUR(ch) || IS_ILLITHID(ch) || IS_PILLITHID(ch))
      {
        return FALSE;
      }
      break;
    case WEAR_LEGS:
      if( IS_DRIDER(ch) || IS_CENTAUR(ch) || IS_HARPY(ch)
        || IS_OGRE(ch) || IS_FIRBOLG(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_FEET:
      if( IS_DRIDER(ch) || IS_THRIKREEN(ch)
        || IS_HARPY(ch) || IS_MINOTAUR(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_HANDS:
      return TRUE;
      break;
    case WEAR_ARMS:
      if( IS_OGRE(ch) || IS_FIRBOLG(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_SHIELD:
    case WEAR_ABOUT:
    case WEAR_WAIST:
    case WEAR_WRIST_L:
    case WEAR_WRIST_R:
    case PRIMARY_WEAPON:
      return TRUE;
      break;
    case SECONDARY_WEAPON:
      if( GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) <= 0 )
      {
        return FALSE;
      }
      break;
    case HOLD:
    case WEAR_EYES:
    case WEAR_FACE:
      return TRUE;
      break;
    case WEAR_EARRING_R:
    case WEAR_EARRING_L:
      if( IS_THRIKREEN(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_QUIVER:
    case GUILD_INSIGNIA:
      return TRUE;
      break;
    case THIRD_WEAPON:
    case FOURTH_WEAPON:
      if( !HAS_FOUR_HANDS( ch ) )
      {
        return FALSE;
      }
      break;
    case WEAR_BACK:
    case WEAR_ATTACH_BELT_1:
    case WEAR_ATTACH_BELT_2:
    case WEAR_ATTACH_BELT_3:
      return TRUE;
      break;
    case WEAR_ARMS_2:
    case WEAR_HANDS_2:
    case WEAR_WRIST_LR:
    case WEAR_WRIST_LL:
      if( !HAS_FOUR_HANDS( ch ) )
      {
        return FALSE;
      }
      break;
    case WEAR_HORSE_BODY:
      if( !has_innate(ch, INNATE_HORSE_BODY ) )
      {
        return FALSE;
      }
      break;
    case WEAR_LEGS_REAR:
      return FALSE;
      break;
    case WEAR_TAIL:
      if( !IS_CENTAUR(ch) && !IS_MINOTAUR(ch) && !IS_PSBEAST(ch)
        && !IS_KOBOLD(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_FEET_REAR:
      return FALSE;
      break;
    case WEAR_NOSE:
      if( !IS_MINOTAUR(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_HORN:
      if( !IS_MINOTAUR(ch) && !IS_HARPY(ch) && !IS_PSBEAST(ch) )
      {
        return FALSE;
      }
      break;
    case WEAR_IOUN:
      return TRUE;
      break;
    case WEAR_SPIDER_BODY:
      if( !has_innate( ch, INNATE_SPIDER_BODY ) )
      {
        return FALSE;
      }
      break;
    default:
      wizlog(56, "has_eq_slot: Bad wear slot %d", wear_slot );
      logit(LOG_WIZ, "has_eq_slot: Bad wear slot %d", wear_slot );
      return FALSE;
      break;
  }
  return TRUE;
}

bool get_equipment_list(P_char ch, char *buf, int list_only)
{
  int      j;
  int      blood = 0;
  bool     found;
  char     tempbuf[MAX_STRING_LENGTH];
  P_obj    t_obj, wpn;
  int      free_hands;
  int      wear_order[] =
    { 41, 24, 40,  6, 19, 21, 22, 20, 39, 3,  4, 5, 35, 12, 27, 42, 37, 23, 13, 28,
      29, 30, 10, 31, 11, 14, 15, 33, 34, 9, 32, 1,  2, 16, 17, 25, 26, 18,  7,
      36,  8, 38, -1
    }; //also defined in show_char_to_char

  buf[0] = '\0';
  found = FALSE;
  free_hands = get_numb_free_hands(ch);
  if (!list_only)
  {
    strcpy(buf, "You are using:\n");
  }
  else if( list_only == 2 )
  {
    strcpy(buf, "Your available eq slots are:\n");
  }

  for (j = 0; wear_order[j] != -1; j++)
  {
    if (ch->equipment[wear_order[j]])
    {
      t_obj = ch->equipment[wear_order[j]];
      found = TRUE;

      if( ((wear_order[j] >= WIELD) && (wear_order[j] <= HOLD))
        && (wield_item_size(ch, ch->equipment[wear_order[j]]) == 2) )
      {
        strcat(buf, ((wear_order[j] >= WIELD) && (wear_order[j] <= HOLD)
          && (t_obj->type != ITEM_WEAPON) &&  t_obj->type != ITEM_FIREWEAPON)
          ? "<held in both hands> " : "<wielding twohanded> ");
      }
      else
      {
        strcat(buf, (wear_order[j] >= WIELD && wear_order[j] <= HOLD
          && t_obj->type != ITEM_WEAPON && t_obj->type != ITEM_FIREWEAPON)
          ? where[HOLD] :  where[wear_order[j]]);
      }

      if (CAN_SEE_OBJ(ch, t_obj) || list_only)
      {
        /*
         * stolen from show_obj_to_char(), so we can buffer it.
         * JAB
         */
        if (IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_VNUM))
          sprintf(buf + strlen(buf), "[&+B%5d&N] ",
            (t_obj->R_num >= 0 ? obj_index[t_obj->R_num].virtual_number : -1));
        if (t_obj->short_description)
          strcat(buf, t_obj->short_description);
        if (IS_OBJ_STAT(t_obj, ITEM_INVISIBLE))
          strcat(buf, " (&+Linvis&n)");
        if (IS_OBJ_STAT(t_obj, ITEM_SECRET))
          strcat(buf, " (&+Lsecret&n)");
        if (IS_OBJ_STAT(t_obj, ITEM_BURIED))
          strcat(buf, " (&+Lburied&n)");
        if (IS_OBJ_STAT2(t_obj, ITEM2_MAGIC) &&
            (IS_TRUSTED(ch) || IS_AFFECTED2(ch, AFF2_DETECT_MAGIC)))
          strcat(buf, " (&+bmagic&n)");
        if (get_obj_affect(t_obj, SKILL_ENCHANT))
          strcat(buf, " (&+mEnchanted&n)");
        if (IS_OBJ_STAT(t_obj, ITEM_GLOW))
          strcat(buf, " (&+Mglowing&n)");
        if (IS_OBJ_STAT(t_obj, ITEM_HUM))
          strcat(buf, " (&+rhumming&N)");
        if (IS_OBJ_STAT(t_obj, ITEM_NOSHOW) && (IS_TRUSTED(ch)))
          strcat(buf, " (&+LNOSHOW&N)");
        if (IS_OBJ_STAT(t_obj, ITEM_LIT) ||
            ((t_obj->type == ITEM_LIGHT) && t_obj->value[2]))
          strcat(buf, " (&+Willuminating&n)");

        if (affected_by_spell(ch, SPELL_HOLY_SWORD))
        {
          if (ch->equipment[wear_order[j]]->type == ITEM_WEAPON)
          {
            if (ch->equipment[wear_order[j]]->value[3] == 3)
            {
              strcat(buf, " &+L(&+Wh&+wol&+Wy&+L)&N");
            }
          }
        }

        if (affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD))
        {
          if (ch->equipment[wear_order[j]]->type == ITEM_WEAPON)
          {
            if (ch->equipment[wear_order[j]]->value[3] == 3)
            {
              strcat(buf, " &+L(&+rf&+Rl&+Ya&+Wm&+Yi&+Rn&+rg&+L)&N");
            }
          }
        }

        if (affected_by_spell(ch, SPELL_THRYMS_ICERAZOR))
        {
          if (ch->equipment[wear_order[j]]->type == ITEM_WEAPON)
          {
            strcat(buf, " &+L(&+Cch&+Bill&+Cing&+L)&N"); // XXX zion
          }
        }

        if (affected_by_spell(ch, SPELL_LLIENDILS_STORMSHOCK))
        {
          if (ch->equipment[wear_order[j]]->type == ITEM_WEAPON)
          {
            strcat(buf, " &+L(&+Bele&+Wctri&+Bfied&+L)&N"); // XXX zion
          }
        }

        strcat(buf, item_condition(t_obj));

        sprintf(tempbuf, "Error");

        if (IS_ARTIFACT(t_obj))
        {
          artifact_timer_sql( GET_OBJ_VNUM(t_obj), tempbuf );
          strcat(buf, tempbuf);
        }

        strcat(buf, "\n");
      }
      else
      {
        strcat(buf, "Something.\n");
      }
    }
    else if( list_only == 2 )
    {
      if( free_hands || !(wear_order[j] == HOLD || wear_order[j] == WIELD || wear_order[j] == WIELD2
        || wear_order[j] == WIELD3 || wear_order[j] == WIELD4 || wear_order[j] == WEAR_SHIELD) )
      {
        if( has_eq_slot( ch, wear_order[j] ) )
        {
          strcat( buf, where[wear_order[j]] );
          strcat( buf, "\n" );
        }
      }
    }
  }

  if (affected_by_spell(ch, TAG_SET_MASTER))
    if(ch->only.pc->master_set)
    strcat(buf, set_master_text[ch->only.pc->master_set]);
  return found;
}

void do_equipment(P_char ch, char *argument, int cmd)
{
  bool     found;
  char     buf[MAX_STRING_LENGTH];

  argument = one_argument(argument, buf);
  if IS_AFFECTED(ch, AFF_WRAITHFORM)
  {
    send_to_char("You have no body to wear anything upon!\n", ch);
    return;
  }
  if( isname( buf, "list" ) || isname( buf, "slots" ) )
  {
    found = get_equipment_list(ch, buf, 2);
  }
  else
  {
    found = get_equipment_list(ch, buf, 0);
  }

  if (!found)
    send_to_char("You aren't wearing anything!\n", ch);
  else
    send_to_char(buf, ch);
}

void do_credits(P_char ch, char *argument, int cmd)
{
  page_string(ch->desc, credits, 0);
}

void do_map(P_char ch, char *arg, int cmd)
{
//  return;
//  if (IS_ANSI_TERM(ch->desc))
//    page_string(ch->desc, worldmapa, 0);
//  else
//    page_string(ch->desc, worldmap, 0);

  if( !IS_TRUSTED(ch) )
  {
//    send_to_char("Please visit http://www.durismud.com/map.php to see the world map.\r\n", ch);
    return;    
  }
  
  if( !IS_MAP_ROOM(ch->in_room) )
  {
    send_to_char("This is not a map room.\r\n", ch);
    return;    
  }

  char buff[MAX_STRING_LENGTH];
  
  one_argument(arg, buff);
  
  int radius = atoi(buff);
  
  if( radius <= 0 )
  {
    send_to_char("Invalid map size.\r\n", ch);
    return;    
  }
  
  display_map(ch, radius, TRUE);
}

void do_cheaters(P_char ch, char *argument, int cmd)
{
  char    *buf;

  buf = file_to_string(CHEATERS_FILE);
  send_to_char(buf, ch);
}
void do_news(P_char ch, char *argument, int cmd)
{

  if (!IS_SET(ch->specials.act, PLR_PAGING_ON))
  {
    send_to_char("&+WNews file is to long, please tog page on, to read it\n",
                 ch);
    return;
  }
  send_to_char("&+WDuris News\n\n", ch);
  send_to_char(news.c_str(), ch, LOG_NONE);
}

void do_projects(P_char ch, char *argument, int cmd)
{
  send_to_char( "Current list of projects:\n\r", ch );
  page_string(ch->desc, projects, 0);
}

void do_faq(P_char ch, char *argument, int cmd)
{
  page_string(ch->desc, faq, 0);
}

void do_wizlist(P_char ch, char *argument, int cmd)
{
  page_string(ch->desc, wizlist, 0);
}

void do_rules(P_char ch, char *argument, int cmd)
{
  do_help(ch, "rules", -4);
//  page_string(ch->desc, rules, 0);
}

void do_motd(P_char ch, char *argument, int cmd)
{
  send_to_char(motd.c_str(), ch, LOG_NONE);
}

// Does a lookup and shows where things are in game.
//   Note: This is an Immortal only command per interp.c -> assign_command_pointers.
void do_where(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char    *args;
  int      length = 0, count = 0, o_count = 0, v_num;
  bool     flag = FALSE;
  register P_char i;
  register P_obj k;
  P_desc   d;
  P_char   t_ch;

  buf[0] = 0;
  length = 0;

  while( *argument == ' ' )
  {
    argument++;
  }

  if( argument && !strcmp( argument, "?" ) )
  {
    send_to_char( "&+YValid arguments: &+Wevils, goods, undeads, neutrals, <vnum>, zone, trap, stat.\n\r", ch );
    return;
  }

  if( !*argument )
  {
    strcpy(buf, "Players:\n--------\n");

    for (d = descriptor_list; d; d = d->next)
    {
      t_ch = d->character;
      if ((!t_ch) && d->original)
        t_ch = d->original;

      if (t_ch && IS_PC(t_ch) && (d->connected == CON_PLYNG) &&
          (d->character->in_room != NOWHERE) && CAN_SEE(ch, t_ch))
      {
        if (d->original)        /* If switched */
          sprintf(buf2, "%-20s - [%4d:%6d] %s &n(In body of %s&n)\n",
                  t_ch->player.name, ROOM_ZONE_NUMBER(d->character->in_room), world[d->character->in_room].number,
                  world[d->character->in_room].name, FirstWord(d->character->player.name));
        else
          sprintf(buf2, "%-20s - [%4d:%6d] %s\n",
                  t_ch->player.name, ROOM_ZONE_NUMBER(d->character->in_room), world[d->character->in_room].number,
                  world[d->character->in_room].name);

        if (strlen(buf2) + length + 35 > MAX_STRING_LENGTH)
        {
          sprintf(buf2, "   ...the list is too long...\n");
          strcat(buf, buf2);
        }
        strcat(buf, buf2);
        length = strlen(buf);
      }
    }
    page_string(ch->desc, buf, 1);
    return;
  }

  if( isname(argument, "evils") || isname(argument,"goods") || isname(argument,"undeads")
    || isname(argument,"neutrals") )
  {
   	int racewar = 0;

		if( isname(argument, "goods") )
		{
		 racewar = RACEWAR_GOOD;
		 strcpy(buf, "Players (goods):\n--------------\n");
		}
		else if( isname(argument, "evils") )
		{
		 racewar = RACEWAR_EVIL;
		 strcpy(buf, "Players (evils):\n--------------\n");
		}
		else if( isname(argument, "undeads") )
		{
		 racewar = RACEWAR_UNDEAD;
		 strcpy(buf, "Players (undeads):\n--------------\n");
		}
		else if( isname(argument, "neutrals") )
		{
		 racewar = RACEWAR_NEUTRAL;
		 strcpy(buf, "Players (neutrals):\n--------------\n");
		}
    else
    {
      racewar = RACEWAR_NONE;
		 strcpy(buf, "Players (&+Rbuggy&n):\n--------------\n");
    }

		for( d = descriptor_list; d; d = d->next )
    {
      t_ch = d->character;
      if( (!t_ch) && d->original )
      {
        t_ch = d->original;
      }

      if( t_ch && IS_PC(t_ch) && (d->connected == CON_PLYNG)
        && (d->character->in_room != NOWHERE) && CAN_SEE(ch, t_ch) && t_ch->player.racewar == racewar )
      {
        if (d->original)        /* If switched */
          sprintf(buf2, "%-20s - [%4d:%6d] %s &n(In body of %s&n)\n",
                  t_ch->player.name, ROOM_ZONE_NUMBER(d->character->in_room), world[d->character->in_room].number,
                  world[d->character->in_room].name, FirstWord(d->character->player.name));
        else
          sprintf(buf2, "%-20s - [%4d:%6d] %s\n",
                  t_ch->player.name, ROOM_ZONE_NUMBER(d->character->in_room), world[d->character->in_room].number,
                  world[d->character->in_room].name);

        if (strlen(buf2) + length + 35 > MAX_STRING_LENGTH)
        {
          sprintf(buf2, "   ...the list is too long...\n");
          strcat(buf, buf2);
        }
        strcat(buf, buf2);
        length = strlen(buf);
      }
    }
    page_string(ch->desc, buf, 1);
    return;
  }
  /*
   * This chunk of code allows "where <v-number>" if the argument is a
   * number.  It will return all mobs/objects with that v-number.
   * Otherwise it defaults to treating the argument as a string.
   */
  if( is_number(argument) )
  {
    v_num = atoi(argument);

    /* mobs */
    for( i = character_list; i && !flag; i = i->next )
    {
      if( IS_NPC(i) && CAN_SEE(ch, i) && (v_num == GET_VNUM(i)) )
      {
        if( (i->in_room != NOWHERE) && (IS_TRUSTED(ch) || (world[i->in_room].zone == world[ch->in_room].zone)) )
        {
          count++;
          sprintf(buf2, "%3d. [%6d] %s - [%4d:%6d] %s\n", count, v_num, pad_ansi(i->player.short_descr, 40).c_str(),
            ROOM_ZONE_NUMBER(i->in_room), world[i->in_room].number, world[i->in_room].name);
          if( (length + strlen(buf2) + 35) > MAX_STRING_LENGTH )
          {
            strcpy(buf2, "   ...the list is too long...\n");
            flag = TRUE;
          }
          strcat(buf, buf2);
          length += strlen(buf2);
        }
      }
    }

    if( count && !flag )
    {
      strcat(buf, "\n\n");      /* extra lines between mobs/objs */
    }

    /* objects */
    for( k = object_list; k && !flag; k = k->next )
    {
      if( v_num == GET_OBJ_VNUM(k) )
      {
        // wizinvis checks
        P_obj tobj = k;
        if( (k->affected[0].location == APPLY_LEVEL || k->affected[1].location == APPLY_LEVEL) && GET_LEVEL(ch) <= 59 )
        {
          continue;
        }
        while( OBJ_INSIDE(tobj) )
        {
          tobj = tobj->loc.inside;
        }
        if( (OBJ_WORN(tobj) && WIZ_INVIS(ch, tobj->loc.wearing))
          || (OBJ_CARRIED(tobj) && WIZ_INVIS(ch, tobj->loc.carrying)) )
        {
          continue;
        }

        o_count++;
        count++;

        sprintf(buf2, "%3d. [%6d] %s - %s\n", o_count, v_num, pad_ansi(k->short_description, 40).c_str(), where_obj(k, FALSE));

        if( (strlen(buf2) + length + 35) > MAX_STRING_LENGTH )
        {
          strcpy(buf2, "   ...the list is too long...\n");
          flag = TRUE;
        }
        strcat(buf, buf2);
        length += strlen(buf2);
      }
    }
    if( !count )
    {
      send_to_char("Nothing found.\n", ch);
    }
    else
    {
      page_string(ch->desc, buf, 1);
    }
    return;
  }
  /*
   * "where zone" -- added by DTS 7/6/95
   */
  if( is_abbrev(argument, "zone") )
  {

    strcpy(buf, "Players in this zone:\n---------------------\n");

    for (d = descriptor_list; d; d = d->next)
    {
      if (d->character && IS_PC(d->character) && (d->connected == CON_PLYNG)
          && (d->character->in_room != NOWHERE) && CAN_SEE(ch, d->character))
      {
        if (world[d->character->in_room].zone == world[ch->in_room].zone)
        {
          if (d->original)
          {
            sprintf(buf2, "%-20s - [%6d] %s (In body of %s&n)\n",
                    d->original->player.name,
                    world[d->character->in_room].number,
                    world[d->character->in_room].name,
                    FirstWord(d->character->player.name));
          }
          else
          {
            sprintf(buf2, "%-20s - [%6d] %s\n",
                    d->character->player.name,
                    world[d->character->in_room].number,
                    world[d->character->in_room].name);
          }
          if (strlen(buf2) + length + 35 > MAX_STRING_LENGTH)
          {
            sprintf(buf2, "   ...the list is too long...\n");
            strcat(buf, buf2);
          }
          strcat(buf, buf2);
          length = strlen(buf);
        }
      }
    }
    page_string(ch->desc, buf, 1);
    return;
  }
  else if (is_abbrev(argument, "trap"))
    do_traplist(ch, argument, 0);

  args = one_argument(argument, buf);
  if( is_abbrev(buf, "stat") )
  {
    where_stat(ch, args);
    return;
  }

  /* mobs */
  buf[0] = '\0';
  for (i = character_list; i && !flag; i = i->next)
  {
    if ((isname(argument, GET_NAME(i)) ||
         (IS_NPC(i) && isname(argument, i->player.short_descr))) &&
        CAN_SEE(ch, i))
    {
      if ((i->in_room != NOWHERE) &&
          (IS_TRUSTED(ch) ||
           (world[i->in_room].zone == world[ch->in_room].zone)))
      {

        count++;
        if (IS_NPC(i))
          sprintf(buf2, "%3d. [%6d] %s - [%4d:%6d] %s ", count, GET_VNUM(i), pad_ansi(i->player.short_descr, 40).c_str(),
                  ROOM_ZONE_NUMBER(i->in_room), world[i->in_room].number, world[i->in_room].name);
        else
          sprintf(buf2, "%3d. %s - [%4d:%6d] %s ", count, pad_ansi(i->player.name, 40).c_str(),
                  ROOM_ZONE_NUMBER(i->in_room), world[i->in_room].number, world[i->in_room].name);

        strcat(buf2, "\n");

        if ((length + strlen(buf2) + 35) > MAX_STRING_LENGTH)
        {
          strcpy(buf2, "   ...the list is too long...\n");
          flag = TRUE;
        }
        strcat(buf, buf2);
        length += strlen(buf2);

        if (!IS_TRUSTED(ch))
          flag = TRUE;
      }
    }
  }

  if (count && !flag)
    strcat(buf, "\n\n");        /*
                                 * extra lines between mobs/objs
                                 */

  for (k = object_list; k && !flag; k = k->next)
  {
    if ((isname(argument, k->name) ||
         isname(argument, k->short_description)) && CAN_SEE_OBJ(ch, k))
    {
      // wizinvis checks
      P_obj tobj = k;
      while (OBJ_INSIDE(tobj))
        tobj = tobj->loc.inside;
      if (OBJ_WORN(tobj) && WIZ_INVIS(ch, tobj->loc.wearing))
        continue;
      if (OBJ_CARRIED(tobj) && WIZ_INVIS(ch, tobj->loc.carrying))
        continue;
      if ((k->affected[0].location == APPLY_LEVEL ||
          k->affected[1].location == APPLY_LEVEL) &&
          GET_LEVEL(ch) <= 60)
        continue;

      o_count++;
      count++;
      sprintf(buf2, "%3d. [%6d] %s - %s\n",
              o_count, GET_OBJ_VNUM(k), pad_ansi(k->short_description, 40).c_str(), where_obj(k, FALSE));
      if ((strlen(buf2) + length + 35) > MAX_STRING_LENGTH)
      {
        strcpy(buf2, "   ...the list is too long...\n");
        flag = TRUE;
      }
      strcat(buf, buf2);
      length += strlen(buf2);
    }
  }

  if (!count)
    send_to_char("Nothing found.\n", ch);
  else
    page_string(ch->desc, buf, 1);
}

// This is pretty outdated since all classes get same exp table now.
//   Even more so, with exp being subtracted upon level.
void do_levels(P_char ch, char *argument, int cmd)
{
  int      i, which;
  char     buf[MAX_STRING_LENGTH], arg[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
  {
    send_to_char("You ain't nothin' but a hound-dog.\n", ch);
    return;
  }
  one_argument(argument, arg);
  if (*arg)
  {
    switch (LOWER(*arg))
    {
    case 'm':
      {
        if (LOWER(*(arg + 1)) == 'a')
          which = CLASS_SORCERER;
        else if (LOWER(*(arg + 1)) == 'e')
          which = CLASS_MERCENARY;
        else if (LOWER(*(arg + 1)) == 'o')
          which = CLASS_MONK;
        else
          which = ch->player.m_class;
        break;
      }
    case 'c':
      {
        if (LOWER(*(arg + 1)) == 'l')
          which = CLASS_CLERIC;
        else if (LOWER(*(arg + 1)) == 'o')
          which = CLASS_CONJURER;
        else
          which = ch->player.m_class;
        break;
      }
    case 'b':
      {
        if (LOWER(*(arg + 1)) == 'a')
          which = CLASS_BARD;
        else if (LOWER(*(arg + 1)) == 'l')
          which = CLASS_BLIGHTER;
        else
          which = ch->player.m_class;
        break;
      }
    case 't':
      {
        which = CLASS_ROGUE;
        break;
      }
    case 'w':
      {
        which = CLASS_WARRIOR;
        break;
      }
    case 'a':
      {
        if (LOWER(*(arg + 1)) == 's')
          which = CLASS_ASSASSIN;
        else if (LOWER(*(arg + 1)) == 'n')
          which = CLASS_ANTIPALADIN;
        else
          which = ch->player.m_class;
        break;
      }
    case 'r':
      {
        which = CLASS_RANGER;
        break;
      }
    case 'p':
      {
        which = CLASS_PALADIN;
        break;
      }
    case 'd':
      {
        which = CLASS_DRUID;
        break;
      }
    case 'n':
      {
        which = CLASS_NECROMANCER;
        break;
      }
    case 's':
      {
        if (LOWER(*(arg + 1)) == 'o')
          which = CLASS_SORCERER;
        else if (LOWER(*(arg + 1)) == 'h')
          which = CLASS_SHAMAN;
        else if (LOWER(*(arg + 1)) == 'u')
          which = CLASS_SUMMONER;
        else
          which = ch->player.m_class;
        break;
      }
    default:
      {
        which = ch->player.m_class;
        break;
      }
    }
  }
  else
  {
    which = ch->player.m_class;
  }

  *buf = '\0';

  for( i = 1; i < MAXLVLMORTAL; i++ )
  {
    sprintf(buf + strlen(buf), "[%2d %2s] %9d-%-9d\n",
      i, class_names_table[flag2idx(which)].ansi,
      0, new_exp_table[i + 1] - 1);
//  Exp is subtracted upon level -> start of each level is 0 exp.
//      new_exp_table[i], new_exp_table[i + 1] - 1);
  }
  // i == MAXLVLMORTAL.  Imms don't use exp, so this is final entry.
  sprintf(buf + strlen(buf), "[%2d %2s] %9d+\n",
    i, class_names_table[flag2idx(which)].ansi, 0);
//    i, class_names_table[flag2idx(which)].ansi, new_exp_table[i]);

  page_string(ch->desc, buf, 1);
}

/*
 * ** Command to ignore certain devious individual.  At most one ** person
 * can be ignored at a time.
 */

void do_ignore(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_STRING_LENGTH];
  P_char   target;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg)
  {
    send_to_char("You feel sociable and stop ignoring anyone.\n", ch);
    ch->only.pc->ignored = NULL;
    return;
  }
  if ((target = get_char_vis(ch, arg)) == NULL)
  {
    send_to_char("No one by that name here..\n", ch);
    return;
  }
  if (target == ch)
  {
    send_to_char("You ignore your own attempt to ignore yourself.\n", ch);
    return;
  }
  if ((GET_LEVEL(target) > 56) || racewar(ch, target))
  {
    send_to_char("No one by that name here..\n", ch);
    return;
  }
  if (IS_NPC(target))
  {
    send_to_char("No one by that name here..\n", ch);
    return;
  }
  if (ch->only.pc->ignored == target)
  {
    send_to_char("You're already ignoring that person.\n", ch);
    return;
  }
  if (ch->only.pc->ignored)
  {
    act("You stop ignoring $N.", FALSE, ch, 0, ch->only.pc->ignored, TO_CHAR);
  }
  act("You now ignore $N.", FALSE, ch, 0, target, TO_CHAR);
  ch->only.pc->ignored = target;

  /* 
  if (!IS_TRUSTED(ch))
    act("$n is now ignoring you.", FALSE, ch, 0, target, TO_VICT);
  */
}

void do_consider(P_char ch, char *argument, int cmd)
{
  P_char   victim;
  char     name[MAX_STRING_LENGTH];
  int      diff;

  one_argument(argument, name);

  if (!(victim = get_char_room_vis(ch, name)))
  {
    send_to_char("Consider killing who?\n", ch);
    return;
  }
  if (victim == ch)
  {
    send_to_char("Easy! Very easy indeed!\n", ch);
    return;
  }

  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char("Now where did that chicken go?\n", ch);
  else if (diff <= -5)
    send_to_char("You could do it with a needle!\n", ch);
  else if (diff <= -2)
    send_to_char("Easy.\n", ch);
  else if (diff <= -1)
    send_to_char("Fairly easy.\n", ch);
  else if (diff == 0)
    send_to_char("The perfect match!\n", ch);
  else if (diff <= 1)
    send_to_char("You would need some luck!\n", ch);
  else if (diff <= 2)
    send_to_char("You would need a lot of luck!\n", ch);
  else if (diff <= 3)
    send_to_char("You would need a lot of luck and great equipment!\n", ch);
  else if (diff <= 5)
    send_to_char("Do you feel lucky, punk?\n", ch);
  else if (diff <= 10)
    send_to_char("Are you mad!?\n", ch);
  else if (diff <= 15)
    send_to_char("You ARE mad!\n", ch);
  else if (diff <= 20)
    send_to_char
      ("Why don't you just lie down and pretend you're dead instead?\n", ch);
  else if (diff <= 25)
    send_to_char("What do you want your epitaph to say?!?\n", ch);
  else if (diff <= 100)
    send_to_char("This thing will kill you so fast, it's not EVEN funny!\n",
                 ch);

  act("$n&n sizes you up with a quick glance.", TRUE, ch, 0, victim, TO_VICT);
  act("$n&n sizes up $N&n with a quick glance.", TRUE, ch, 0, victim,
      TO_NOTVICT);
}

void do_report(P_char ch, char *argument, int cmd)
{
  char     buf[240], report_name[MAX_INPUT_LENGTH];

  if (GET_RACE(ch) == RACE_ILLITHID && !*argument)
  {
    send_to_char("But, you have no mouth!\n", ch);
    return;
  }
  report_name[0] = 0;
  if (GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER))
    sprintf(buf,
            "I have %d (%d) hits, %d (%d) mana, and %d (%d) movement points.",
            GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch),
            GET_VITALITY(ch), GET_MAX_VITALITY(ch));
  else
    sprintf(buf, "I have %d (%d) hits, and %d (%d) movement points.",
            GET_HIT(ch), GET_MAX_HIT(ch),
            GET_VITALITY(ch), GET_MAX_VITALITY(ch));

  if (*argument)
  {
    one_argument(argument, report_name);
    strcat(report_name, " ");
    strcat(report_name, buf);
    do_tell(ch, report_name, -4);
  }
  else
    do_say(ch, buf, -4);
}

void do_display(P_char ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  P_char to_ch = ch;
  bool switched = FALSE;

  // If an Imm is switched, we want to toggle the Imm's char's pcact not the mobs npcact
  if( ch->desc && ch->desc->original )
  {
    ch = ch->desc->original;
    switched = TRUE;
  }

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (*buf)
  {
    if (!str_cmp("twoline", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_TWOLINE))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_TWOLINE);
        send_to_char("Twoline display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_TWOLINE);
        send_to_char("Twoline display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("screen", buf))
    {
      if (IS_SET(ch->specials.act, PLR_SMARTPROMPT))
        InitScreen(ch);
    }
    else if (!str_cmp("hits", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_HIT))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_HIT);
        send_to_char("Hits display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_HIT);
        send_to_char("Hits display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("maxhits", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_MAX_HIT))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_MAX_HIT);
        send_to_char("Maxhits display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_MAX_HIT);
        send_to_char("Maxhits display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("mana", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_MANA))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_MANA);
        send_to_char("Mana display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_MANA);
        send_to_char("Mana display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("maxmana", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_MAX_MANA))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_MAX_MANA);
        send_to_char("Max Mana display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_MAX_MANA);
        send_to_char("Max Mana display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("moves", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_MOVE))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_MOVE);
        send_to_char("Moves display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_MOVE);
        send_to_char("Moves display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("maxmoves", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_MAX_MOVE))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_MAX_MOVE);
        send_to_char("Maxmoves display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_MAX_MOVE);
        send_to_char("Maxmoves display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("tankname", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_TANK_NAME))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_TANK_NAME);
        send_to_char("Tank name display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_TANK_NAME);
        send_to_char("Tank name display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("tankcond", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_TANK_COND))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_TANK_COND);
        send_to_char("Tank condition display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_TANK_COND);
        send_to_char("Tank condition display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("enemy", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_ENEMY))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_ENEMY);
        send_to_char("Enemy display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_ENEMY);
        send_to_char("Enemy display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("status", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_STATUS))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_STATUS);
        send_to_char("Status display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_STATUS);
        send_to_char("Status display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("enemycond", buf))
    {
      if (IS_SET(ch->only.pc->prompt, PROMPT_ENEMY_COND))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_ENEMY_COND);
        send_to_char("Enemy condition display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_ENEMY_COND);
        send_to_char("Enemy condition display turned &+gON&N.\n", to_ch);
      }
    }
    /*
     * SAM 7-94 visibility prompt
     */
    else if( (!str_cmp("vis", buf)) && IS_TRUSTED(ch) )
    {
      if( switched )
      {
        send_to_char("You shall remain invisible while switched.\n\r", to_ch );
        return;
      }

      if (IS_SET(ch->only.pc->prompt, PROMPT_VIS))
      {
        REMOVE_BIT(ch->only.pc->prompt, PROMPT_VIS);
        send_to_char("Visibility display turned &+rOFF&N.\n", to_ch);
      }
      else
      {
        SET_BIT(ch->only.pc->prompt, PROMPT_VIS);
        send_to_char("Visibility display turned &+gON&N.\n", to_ch);
      }
    }
    else if (!str_cmp("all", buf))
    {
      ch->only.pc->prompt =
        (PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MOVE | PROMPT_MAX_MOVE |
         PROMPT_TANK_NAME | PROMPT_TANK_COND | PROMPT_ENEMY |
         PROMPT_ENEMY_COND | PROMPT_TWOLINE | PROMPT_STATUS);
      if (GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER))
        ch->only.pc->prompt |= (PROMPT_MANA | PROMPT_MAX_MANA);
      if( IS_TRUSTED(ch) )
        ch->only.pc->prompt |= PROMPT_VIS;
    }
    else if (!str_cmp("off", buf))
    {
      if (ch->only.pc->prompt)
        send_to_char("Turning off display.\n", to_ch);
      else
        send_to_char("But you aren't displaying anything!\n", to_ch);
      ch->only.pc->prompt = 0;
    }
    else
      send_to_char("Bad argument for display.\n\r", to_ch );
    return;
  }
  else
    send_to_char("Syntax: display <option>\n"
                 "Note: You must type the full name of the option listed below.\n"
                 "Options:all|off|hits|maxhits|slots|maxslots|moves|maxmoves|\n"
                 "        tankname|tankcond|enemy|enemycond|twoline\n", to_ch);
  return;
}

/*
 * Set a players title
 */

void do_ok(P_char ch, char *arg, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_INPUT_LENGTH];
  long     t;


  P_char   t_ch = NULL;
  FILE    *fl;

  arg = one_argument(arg, name);

  if (!*name || !(t_ch = get_char_vis(ch, name)))
  {
    send_to_char("&+WOK command: Puts someone in the OK file.\n\rUsage: 'ok <char online> <message about char>'\n\r", ch );
    send_to_char("&+WWho do you want to treat like a non-potential cheater?\n", ch);
    return;
  }

  if (IS_NPC(t_ch))
  {
    send_to_char("Sorry, mobs don't deserve that.\n", ch);
    return;
  }

  if ((GET_LEVEL(t_ch) > GET_LEVEL(ch)) && (t_ch != ch))
  {
    send_to_char("Sorry, you can't punish or ok superiors!\n", ch);
    return;
  }

  /*
   * eat leading spaces
   */
  while( isspace(*arg) )
    arg++;

  t = time(0);

  sprintf(Gbuf1, "%s accepted - %s : %s", GET_NAME(t_ch), arg, ctime(&t));

  if (!(fl = fopen(OK_FILE, "a")))
  {
    perror("do_ok");
    send_to_char("Could not open the ok-file.\n", ch);
    return;
  }

  fputs(Gbuf1, fl);
  fclose(fl);
  send_to_char("&+WAdded to file..\n", ch);

  if( t_ch->only.pc->poofIn != NULL )
    str_free(t_ch->only.pc->poofIn);
  t_ch->only.pc->poofIn = str_dup("Accepted");
}

void do_punish(P_char ch, char *arg, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  long     t;
  int      i = 0;
  P_char   t_ch = NULL;
  FILE    *fl;

  arg = one_argument(arg, name);
  if( !*name )
  {
    send_to_char("&+WFormat: punish <char name> <number of levels to drop> <reason for punishment>.\n", ch);
    return;
  }
  if( !(t_ch = get_char_vis(ch, name)) )
  {
    send_to_char("&+WPunish who?\n", ch);
    return;
  }

  arg = one_argument(arg, name);
  if( !*name || (i = atoi(name)) < 1 )
  {
    send_to_char("&+WPunish how many levels?\n", ch);
    return;
  }

  if( IS_NPC(t_ch) )
  {
    send_to_char("Sorry, mobs don't deserve that.\n", ch);
    return;
  }

  if( (GET_LEVEL(t_ch) > GET_LEVEL(ch)) && (t_ch != ch) )
  {
    send_to_char("Sorry, you can't punish superiors!\n", ch);
    return;
  }

  // Eat leading spaces
  while (*arg == ' ')
  {
    arg++;
  }

  if( !*arg )
  {
    sprintf(Gbuf1, "Supply a damn reason!\n", GET_NAME(t_ch));
    send_to_char(Gbuf1, ch);
    set_title(t_ch);
    return;
  }
  t = time(0);
  // Note: ctime contains a carriage return, so nothing after it.
  sprintf(Gbuf1, "&+W%s &+Lpunished&+W %d level%s for: %s -  %s", GET_NAME(t_ch), i, (i>1) ? "s" : "", arg, ctime(&t));

  if( !(fl = fopen(CHEATERS_FILE, "a")) )
  {
    perror("do_punish");
    send_to_char("Could not open the cheaters-file.\n", ch);
    return;
  }
  fputs(Gbuf1, fl);
  fclose(fl);
  send_to_char("&+WAdded to file..\n", ch);
  send_to_char(Gbuf1, t_ch);

  while( i-- > 0 )
  {
    lose_level(t_ch);
  }

  GET_EXP(t_ch) = 1;
  send_to_char("&+WPunished...\n", ch);
}

void do_title(P_char ch, char *arg, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  P_char   t_ch = NULL;

  if (IS_TRUSTED(ch))
  {
    arg = one_argument(arg, name);
    if (!*name || !(t_ch = get_char_vis(ch, name)))
    {
      send_to_char("Title who?\n", ch);
      return;
    }
  }
  else
    t_ch = ch;

/*
  if (IS_NPC(t_ch)) {
    send_to_char("Sorry, mobs don't deserve titles.\n", ch);
    return;
  }
*/
  if ((GET_LEVEL(t_ch) > GET_LEVEL(ch)) && (t_ch != ch))
  {
    send_to_char("Sorry, you can't change the title of your superiors!\n",
                 ch);
    return;
  }
  /*
   * eat leading spaces
   */
  while (*arg == ' ')
    arg++;

/*  if ((strlen(arg) + strlen(GET_NAME(t_ch))) > 80) {
    send_to_char("Sorry, keep title + name under 80 characters.\n", ch);
    return;
  }*/
  if (!*arg)
  {
    sprintf(Gbuf1, "%s's title cleared.\n", GET_NAME(t_ch));
    send_to_char(Gbuf1, ch);
    set_title(t_ch);
    return;
  }
  sprintf(Gbuf1, "Title Bestowed:\n%s %s\n", GET_NAME(t_ch), arg);
  send_to_char(Gbuf1, ch);

  sprintf(Gbuf1, "char %s title %s", GET_NAME(t_ch), arg);
  do_string(ch, Gbuf1, -4);
}

void do_artireset(P_char ch, char *arg, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  int      bits, wtype, craft, mat;
  P_char   tmp_char;
  P_obj    tmp_object;
  float    result_space;

  send_to_char("This is no longer supported.  Use the 'artifact timer <artifact name> <timer>' command instead.\n", ch);
  return;

  one_argument(arg, name);

  if( !*name )
  {
    send_to_char("Reset what arti?\n", ch);
    return;
  }

  bits = generic_find(name, FIND_OBJ_INV, ch, &tmp_char, &tmp_object);

  if( tmp_object )
  {
    if (!IS_ARTIFACT(tmp_object))
    {
      act("$p is not an artifact.", FALSE, ch, tmp_object, 0, TO_CHAR);
      return;
    }

//    UpdateArtiBlood(ch, tmp_object, 100);
    tmp_object->timer[3] = time(NULL);
    act("$p is reset.", FALSE, ch, tmp_object, 0, TO_CHAR);
    tmp_object->value[7] = number(4, 7);
    wizlog(GET_LEVEL(ch), "%s reset artifact '%s' in [%d]", GET_NAME(ch),
           tmp_object->short_description, world[ch->in_room].number);
    logit(LOG_WIZ, "%s reset artifact ' %s' in  [%d]", GET_NAME(ch),
          tmp_object->short_description, world[ch->in_room].number);
    sql_log(ch, WIZLOG, "Reset artifact %s", tmp_object->short_description);
    return;
  }

  send_to_char("Cant find that object, make sure you have it in your inventory.\n", ch);
  return;

}


void do_glance(P_char ch, char *argument, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  P_char   tar_char = NULL;
  int      percent, j, vis_mode;
  int      wear_order[] =
    { 24, 40, 6, 19, 21, 22, 20, 39, 3, 4, 5, 35, 37, 12, 27, 23, 13, 28, 29,
    30, 10, 31, 11, 14, 15, 33, 34, 9, 32, 1, 2, 16, 17, 25, 26, 18, 7, 36, 8,
      38, -1
  };

  if( !IS_ALIVE(ch) || !ch->desc )
  {
    return;
  }

  if( IS_AFFECTED(ch, AFF_BLIND) )
  {
    send_to_char("You can't see a thing, you're blinded!\n", ch);
    return;
  }


  /* Uncomment this if you want to make dayblind/nightblind active on glance.

  vis_mode = get_vis_mode(ch, room_no);
  if( vismode == 5 )
  {
    send_to_char("&+LIt is pitch black...&n\n", ch);
    return;
  }
  else if( vis_mode == 6 )
  {
    send_to_char("&+WArgh!!! The sun is too bright!\n", ch);
    return;
  }
  */

  if( *argument )
  {
    one_argument(argument, name);
    if( !(tar_char = get_char_room_vis(ch, name)) )
    {
      send_to_char("You don't see that here.\n", ch);
      return;
    }
  }
  else if( ch->specials.fighting )
  {
    tar_char = ch->specials.fighting;
  }
  if( !tar_char )
  {
    send_to_char("Glance at whom?\n", ch);
    return;
  }

  show_visual_status(ch, tar_char);

  if( GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF) || GET_CLASS(ch, CLASS_THIEF) )
  {
    for( j = 0; wear_order[j] != -1; j++ )
    {
      if( wear_order[j] == -2 )
      {
        send_to_char("You are holding:\n", ch);
        continue;
      }
      if( tar_char->equipment[wear_order[j]] )
      {
        if( CAN_SEE_OBJ(ch, tar_char->equipment[wear_order[j]]) || (ch == tar_char) )
        {
          if( ((wear_order[j] >= WIELD) && (wear_order[j] <= HOLD))
            && (wield_item_size(tar_char, tar_char->equipment[wear_order[j]]) == 2) )
          {
            send_to_char( ((wear_order[j] >= WIELD) && (wear_order[j] <= HOLD)
              && (tar_char->equipment[wear_order[j]]->type != ITEM_WEAPON))
              ? "<held in both hands> " : "<wielding twohanded> ", ch);
          }
          else
          {
            send_to_char((wear_order[j] >= WIELD && wear_order[j] <= HOLD
              && tar_char->equipment[wear_order[j]]->type != ITEM_FIREWEAPON
              && tar_char->equipment[wear_order[j]]->type != ITEM_WEAPON)
              ? where[HOLD] : where[wear_order[j]], ch);
          }
        }
        if( CAN_SEE_OBJ(ch, tar_char->equipment[wear_order[j]]) )
        {
          show_obj_to_char(tar_char->equipment[wear_order[j]], ch, 1, 1);
        }
        else if (ch == tar_char)
        {
          send_to_char("Something.\n", ch);
        }
      }
    }
  }
}

const char *dist_name[] = {
  "just",
  "close by", "not far off", "a brief walk away",
  "rather far off", "in the distance", "almost out of sight"
};

const char *dir_desc[] = {
  "to your north", "to your east", "to your south", "to your west",
  "above you", "below you", "to your northwest", "to your southwest",
  "to your northeast", "to your southeast"
};

void do_scan(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int      dir, distance, visibility /*= 4*/ ;
  bool     found;
  P_char   vict, vict_next;
  int      was_in_room, percent, basemod = 0, dirmod = 0;
  int      obscurmist = 0, vis_mode;
  char     buf3[20];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_AFFECTED(ch, AFF_BLIND) )
  {
    send_to_char("You can't see a damned thing, you're blind!\n", ch);
    return;
  }

  if( IS_DAYBLIND(ch) )
  {
    send_to_char("&+WArgh!!! The sun is too bright.\n", ch);
    return;
  }

  if( ch->specials.z_cord < 0 )
  {
    send_to_char("The salts prevent a clear view that far away.\n", ch);
    return;
  }

  if( IS_SET(world[ch->in_room].room_flags, BLOCKS_SIGHT) )
  {
    send_to_char("There's too much stuff blocking your sight here to scan.\n", ch);
    return;
  }

  if( argument && !str_cmp(argument, "here") )
  {
    bool found = FALSE;
    send_to_char("You quickly scan the closest area.\n", ch);
    for (vict = world[ch->in_room].people; vict != NULL; vict = vict_next)
    {
      vict_next = vict->next_in_room;
      if( CAN_SEE_Z_CORD(ch, vict) &&  ch != vict )
      {
        sprintf(buf, "$N is %s here.", GET_POS(vict) == POS_PRONE ? "reclining" : GET_POS(vict) == POS_KNEELING ? "kneeling" : GET_POS(vict) == POS_SITTING ? "sitting" : "standing");
        act(buf , FALSE, ch, 0, vict, TO_CHAR);
        found = TRUE;
      }
    }
    if( !found )
    {
      send_to_char("Noone here that you can see.\r\n", ch);
    }
    return;
  }


  was_in_room = ch->in_room;
  found = FALSE;

  send_to_char("You quickly scan the area.\n", ch);

  switch (world[ch->in_room].sector_type)
  {
    case SECT_FOREST:
      if( GET_RACE(ch) != RACE_GREY && GET_RACE(ch) != RACE_HALFLING && !GET_CLASS(ch, CLASS_RANGER) )
            basemod += 25;
      break;                  /* Trees and such will */
    case SECT_HILLS:
      if( GET_RACE(ch) != RACE_MOUNTAIN )
        basemod += 50;
      break;                  /* block ones view     */
    case SECT_MOUNTAIN:
      basemod += 100;
      break;
    case SECT_UNDERWATER:
      basemod += 80;
      break;
    case SECT_UNDRWLD_MOUNTAIN:
      basemod += 100;
      break;
    default:
      break;
  }

  basemod -= (int)((GET_C_WIS(ch) - 80) / 2);

  for( dir = 0; dir < NUM_EXITS; dir++ )
  {
    visibility = 2;
    dirmod = 0;

    if( IS_SURFACE_MAP(ch->in_room) )
    {
      visibility += IS_DAYBLIND(ch) ? map_dayblind_modifier : IS_AFFECTED2(ch, AFF2_ULTRAVISION)
        ? map_ultra_modifier : map_normal_modifier;
    }
    else if( IS_UD_MAP(ch->in_room) )
    {
      visibility += IS_AFFECTED2(ch, AFF2_ULTRAVISION) ? 4 : 3;
    }
    else
    {
      // catch any other cases
      visibility += 3;
    }

    for( distance = 1; distance <= visibility; distance++ )
    {
      /* fake the next room stuff on z_cord. Only need up/down, as cardinal */
      /* points at the same z_cord should show fine */
      obscurmist = FALSE;
      if( (dir == 4 && ch->specials.z_cord > -1) ||
          (dir == 5 && ch->specials.z_cord - distance > -1) )
      {
        for (vict = world[was_in_room].people; vict != NULL; vict = vict_next)
        {
          vict_next = vict->next_in_room;

          dirmod++;

          percent = number(1, 100) - basemod - dirmod;

          if( IS_TRUSTED(ch) )
            percent = 100;

          if( CAN_SEE_Z_CORD(ch, vict) && (percent > 5)
            && ((dir == 4 && vict->specials.z_cord == (ch->specials.z_cord + distance))
            || (dir == 5 && vict->specials.z_cord == (ch->specials.z_cord - distance))) )
          {
            if( !((vict->specials.z_cord == 0 && IS_AFFECTED3(vict, AFF3_COVER)) && (ch->specials.z_cord > 0)) )
            {
              found = TRUE;
              /* TASFALEN checking for limits */
              sprintf(buf, "$N who is %s %s.", dist_name[((distance > 6) ? 6 : distance)], dir_desc[dir]);
              act(buf, FALSE, ch, 0, vict, TO_CHAR);
            }
          }
        }
      }
      if( EXIT(ch, dir) && (EXIT(ch, dir)->to_room != NOWHERE) &&
          !IS_SET(EXIT(ch, dir)->exit_info, EX_SECRET) &&
          !IS_SET(EXIT(ch, dir)->exit_info, EX_BLOCKED) )
      {
        if( !CAN_GO(ch, dir) )
          continue;
        if( IS_SET(world[EXIT(ch, dir)->to_room].room_flags, BLOCKS_SIGHT) )
          continue;
        vis_mode = get_vis_mode(ch, EXIT(ch, dir)->to_room);

        // If too dark or too bright.
        if( vis_mode == 5 || vis_mode == 6 )
        {
          visibility -= 1;
        }

        for (vict = world[EXIT(ch, dir)->to_room].people; vict != NULL; vict = vict_next)
        {
          if (IS_AFFECTED5(vict, AFF5_OBSCURING_MIST) && !IS_TRUSTED(ch))
          {
            obscurmist = TRUE;
            break;
          }
          vict_next = vict->next_in_room;
        }
        for (vict = world[EXIT(ch, dir)->to_room].people; vict != NULL; vict = vict_next)
        {
          if( obscurmist )
          {
            send_to_char("&+cA very thick &+Wcloud of mist&+c blocks your vision.&n\n", ch);
            break;
          }

          vict_next = vict->next_in_room;

          if( dirmod > 5 )
            dirmod += 2;         /* gets harder, the more people around */
          else if( dirmod > 10 )
            dirmod += 3;
          else if( dirmod > 15 )
            dirmod += 20;        /* big jump to cut down spam */
          else
            dirmod++;

          percent = number(1, 100) - basemod - dirmod;

          // No ULTRA/INFRA + DARK = NOT ok, DAYBLIND + LIGHT = NOT ok.
          if( (!(IS_AFFECTED2(ch, AFF2_ULTRAVISION) || IS_AFFECTED(ch, AFF_INFRAVISION))
            && IS_MAGIC_DARK(vict->in_room)) || (IS_DAYBLIND(ch) && IS_MAGIC_LIGHT(vict->in_room)) )
          {
            percent = 0;
          }

          if( IS_TRUSTED(ch) )
          {
            percent = 100;
          }

          if( CAN_SEE(ch, vict) && (percent > 5) && ch->specials.z_cord == vict->specials.z_cord )
          {
            if( IS_AFFECTED4(ch, AFF4_DETECT_ILLUSION) && is_illusion_char(vict) )
            {
              sprintf(buf, "&+m(Illusion)&n $N who is %s %s.", dist_name[((distance > 6) ? 6 : distance)], dir_desc[dir]);
            }
            else
              sprintf(buf, "$N who is %s %s.", dist_name[((distance > 6) ? 6 : distance)], dir_desc[dir]);
            found = TRUE;
            act(buf, FALSE, ch, 0, vict, TO_CHAR);
          }
        }
        ch->in_room = world[ch->in_room].dir_option[dir]->to_room;
      }
    }
    ch->in_room = was_in_room;
  }

  if (!found)
    send_to_char("You see nothing.\n", ch);
}

void web_info(void)
{
  char    *tmstr;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];
  const char *suf;
  int      weekday, day;
  long     ct;
  struct tm *lt;
  struct time_info_data uptime;
  FILE    *opf;
  P_char   t_ch;
  int      number_player = 0;
  int      nb_good = 0, nb_evil = 0;

  Gbuf1[0] = 0;
  Gbuf2[0] = 0;
  Gbuf3[0] = 0;

  sprintf(Gbuf2, "It is %d%s%s, on ",
          (time_info.hour % 12) ? (time_info.hour % 12) : 12, Gbuf1,
          (time_info.hour > 11) ? "pm" : "am");

  /*
   * 35 days in a month
   */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(Gbuf2, weekdays[weekday]);
  strcat(Gbuf2, "\n");

  strcat(Gbuf3, Gbuf2);

  day = time_info.day + 1;      /*
                                 * day in [1..35]
                                 */
  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(Gbuf2, "The %d%s Day of the %s, Year %d.\n",
          day, suf, month_name[time_info.month], time_info.year);

  strcat(Gbuf3, Gbuf2);

  ct = time(0);
  uptime = real_time_passed(ct, boot_time);
  sprintf(Gbuf2, "Time elapsed since boot-up: %d:%s%d:%s%d\n",
          uptime.day * 24 + uptime.hour,
          (uptime.minute > 9) ? "" : "0", uptime.minute,
          (uptime.second > 9) ? "" : "0", uptime.second);

  strcat(Gbuf3, Gbuf2);

  lt = localtime(&ct);
  tmstr = asctime(lt);
  *(tmstr + strlen(tmstr) - 1) = '\0';
//  sprintf(Gbuf2, "Current time is: %s (%s)\n",
//          tmstr, (lt->tm_isdst <= 0) ? "EDT" : "EDT");

//  strcat(Gbuf3, Gbuf2);

  for (t_ch = character_list; t_ch; t_ch = t_ch->next)
  {
    if (!IS_NPC(t_ch) && t_ch->desc)
    {
      number_player++;
      if (EVIL_RACE(t_ch))
        nb_evil++;
      else
        nb_good++;
    }
  }

  sprintf(Gbuf2,
          "&+RRecord number of connections this boot: &+W%d&n\n", max_descs);
  
  strcat(Gbuf3, Gbuf2);

  if ((opf = fopen("lib/reports/status", "w")) == 0)
  {
    logit(LOG_DEBUG, "couldn't open lib/reports/status for writin summary file");
    return;
  }

  fprintf(opf, "%s", Gbuf3);
  fclose(opf);
}

void do_recall(P_char ch, char *argument, int cmd)
{
  int index, i;
  char arg[256];
  char buf[2048];
  int size = 10;
  char *pattern = 0;
  P_char victim = NULL;

  argument = skip_spaces(one_argument(argument, arg));
  if( *argument && IS_TRUSTED(ch) )
  {
    victim = get_char_vis(ch, argument);
    if( !victim )
    {
      sprintf( buf, "Could not find char '%s'.\n", argument );
      send_to_char( buf, ch );
      return;
    }
  }
  if (*arg && atoi(arg) > 0)
  {
    size = MIN(atoi(arg), PRIVATE_LOG_SIZE);
  }
  else if (*arg)
  {
    if( strcmp( arg, "all" ) )
      pattern = arg;
    size = PRIVATE_LOG_SIZE;
  }

  if(!IS_PC(ch))
    return;
  if( victim && !IS_PC(victim) )
  {
    sprintf( buf, "'%s' is not a PC.\n", argument );
    send_to_char( buf, ch );
    return;
  }

  if( victim )
  {
    if( !GET_PLAYER_LOG(victim) )
    {
      logit(LOG_DEBUG, "Unintialized player log (%s) in do_recall()", GET_NAME(victim));
      return;    
    }
  }
  else
  {
    if( !GET_PLAYER_LOG(ch) )
    {
      logit(LOG_DEBUG, "Unintialized player log (%s) in do_recall()", GET_NAME(ch));
      return;    
    }
  }

  if( victim )  
    ITERATE_LOG_LIMIT(victim, LOG_PRIVATE, size)
    {
      if( !pattern || isname( pattern, strip_ansi(LOG_MSG()).c_str() ) )
        send_to_char( LOG_MSG(), ch, LOG_NONE );
    }
  else
    ITERATE_LOG_LIMIT(ch, LOG_PRIVATE, size)
    {
      if( !pattern || isname( pattern, strip_ansi(LOG_MSG()).c_str() ) )
        send_to_char( LOG_MSG(), ch, LOG_NONE );
    }

}

void unmulti(P_char ch, P_obj obj)
{
  if( !IS_MULTICLASS_PC(ch) )
    return;

  if( GET_EPIC_POINTS(ch) < 100 )
  {
    send_to_char("&+WYou must have at least 100 epic points to unmulti!\n", ch);
    return;
  }

  act("You kneel in front of $p and pray to the \n"
      "Gods of &+RDuris&n. As you continue your meditation,\n"
      "you begin to revert to your old ways, and forget\n"
      "the knowledge you have recently gained.\n", FALSE, ch, obj, 0, TO_CHAR);

  act("$n kneels before $p and sinks deep into meditation.\n"
      "After a few moments, $e stands up quietly.\n", FALSE, ch, obj, 0, TO_ROOM);

  ch->player.secondary_class = 0;
  ch->player.spec = 0;
  forget_spells(ch, -1);
  update_skills(ch);
  //epic_gain_skillpoints(ch, -1);
  ch->only.pc->epics -= 100;
}

void where_stat(P_char ch, char *argument)
{
  char   arg1[MAX_STRING_LENGTH];
  char   arg2[MAX_STRING_LENGTH];
  char   buf[MAX_STRING_LENGTH];
  int    amount;
  int    flags;
  int    i;
  P_obj  obj;

  argument_interpreter(argument, arg1, arg2);    

  if( arg1[0] == '\0' || arg2[0] == '\0' )
  {
    send_to_char( "Format: where stat <flag> <amount>\n", ch );
    send_to_char( "i.e. 'where stat maxint 5' for all items with 5maxint.\n", ch );
    return;
  }
  if( (amount = atoi(arg2)) <= 0 )
  {
    send_to_char( "The second argument must be a number > 0.\n", ch );
    return;
  }

  // Header for list: shows the flag and amount to ch.
  send_to_char( "Flag: '", ch);
  send_to_char( arg1, ch);
  send_to_char( "', Amount: '", ch);
  send_to_char( arg2, ch);
  send_to_char( "'\n", ch);

  if( is_abbrev( arg1, "strength" ) )
  {
    flags = APPLY_STR;
  }
  else if( is_abbrev( arg1, "dexterity" ) )
  {
    flags = APPLY_DEX;
  }
  else if( is_abbrev( arg1, "intelligence" ) )
  {
    flags = APPLY_INT;
  }
  else if( is_abbrev( arg1, "wisdom" ) )
  {
    flags = APPLY_WIS;
  }
  else if( is_abbrev( arg1, "constitution" ) )
  {
    flags = APPLY_CON;
  }
  else if( is_abbrev( arg1, "sex" ) )
  {
    flags = APPLY_SEX;
  }
  else if( is_abbrev( arg1, "class" ) )
  {
    flags = APPLY_CLASS;
  }
  else if( is_abbrev( arg1, "level" ) )
  {
    flags = APPLY_LEVEL;
  }
  else if( is_abbrev( arg1, "age" ) )
  {
    flags = APPLY_AGE;
  }
  else if( is_abbrev( arg1, "weight" ) )
  {
    flags = APPLY_CHAR_WEIGHT;
  }
  else if( is_abbrev( arg1, "height" ) )
  {
    flags = APPLY_CHAR_HEIGHT;
  }
  else if( is_abbrev( arg1, "mana" ) )
  {
    flags = APPLY_MANA;
  }
  else if( is_abbrev( arg1, "hitpoints" ) )
  {
    flags = APPLY_HIT;
  }
  else if( is_abbrev( arg1, "move" ) )
  {
    flags = APPLY_MOVE;
  }
  else if( is_abbrev( arg1, "gold" ) )
  {
    flags = APPLY_GOLD;
  }
  else if( is_abbrev( arg1, "exp" ) )
  {
    flags = APPLY_EXP;
  }
  else if(  is_abbrev( arg1, "armor" )
         || is_abbrev( arg1, "ac" ) )
  {
    flags = APPLY_AC;
  }
  else if( is_abbrev( arg1, "hitroll" ) )
  {
    flags = APPLY_HITROLL;
  }
  else if( is_abbrev( arg1, "damroll" ) )
  {
    flags = APPLY_DAMROLL;
  }
  else if( is_abbrev( arg1, "para" ) )
  {
    flags = APPLY_SAVING_PARA;
  }
  else if( is_abbrev( arg1, "rod" ) )
  {
    flags = APPLY_SAVING_ROD;
  }
  else if( is_abbrev( arg1, "fear" ) )
  {
    flags = APPLY_SAVING_FEAR;
  }
  else if( is_abbrev( arg1, "breath" ) )
  {
    flags = APPLY_SAVING_BREATH;
  }
  else if( is_abbrev( arg1, "spell" ) )
  {
    flags = APPLY_SAVING_SPELL;
  }
  else if( is_abbrev( arg1, "pff" )
         || is_abbrev( arg1, "fire" ) )
  {
    flags = APPLY_FIRE_PROT;
  }
  else if( is_abbrev( arg1, "agility" ) )
  {
    flags = APPLY_AGI;
  }
  else if( is_abbrev( arg1, "power" ) )
  {
    flags = APPLY_POW;
  }
  else if( is_abbrev( arg1, "charisma" ) )
  {
    flags = APPLY_CHA;
  }
  else if( is_abbrev( arg1, "karma" ) )
  {
    flags = APPLY_KARMA;
  }
  else if( is_abbrev( arg1, "luck" ) )
  {
    flags = APPLY_LUCK;
  }
  else if(  is_abbrev( arg1, "maxstr" )
    || is_abbrev( arg1, "max_str" ) )
  {
    flags = APPLY_STR_MAX;
  }
  else if(  is_abbrev( arg1, "maxdex" )
    || is_abbrev( arg1, "max_dex" ) )
  {
    flags = APPLY_DEX_MAX;
  }
  else if(  is_abbrev( arg1, "maxint" )
    || is_abbrev( arg1, "max_int" ) )
  {
    flags = APPLY_INT_MAX;
  }
  else if(  is_abbrev( arg1, "maxwis" )
    || is_abbrev( arg1, "max_wis" ) )
  {
    flags = APPLY_WIS_MAX;
  }
  else if(  is_abbrev( arg1, "maxcon" )
    || is_abbrev( arg1, "max_con" ) )
  {
    flags = APPLY_CON_MAX;
  }
  else if(  is_abbrev( arg1, "maxagi" )
    || is_abbrev( arg1, "max_agi" ) )
  {
    flags = APPLY_AGI_MAX;
  }
  else if(  is_abbrev( arg1, "maxpow" )
    || is_abbrev( arg1, "max_pow" ) )
  {
    flags = APPLY_POW_MAX;
  }
  else if(  is_abbrev( arg1, "maxcha" )
    || is_abbrev( arg1, "max_cha" ) )
  {
    flags = APPLY_CHA_MAX;
  }
  else if(  is_abbrev( arg1, "maxkarma" )
    || is_abbrev( arg1, "max_karma" ) )
  {
    flags = APPLY_KARMA_MAX;
  }
  else if(  is_abbrev( arg1, "maxluck" )
    || is_abbrev( arg1, "max_luck" ) )
  {
    flags = APPLY_LUCK_MAX;
  }
  else if( is_abbrev( arg1, "racestr" ) )
  {
    flags = APPLY_STR_RACE;
  }
  else if( is_abbrev( arg1, "racedex" ) )
  {
    flags = APPLY_DEX_RACE;
  }
  else if( is_abbrev( arg1, "raceint" ) )
  {
    flags = APPLY_INT_RACE;
  }
  else if( is_abbrev( arg1, "racewis" ) )
  {
    flags = APPLY_WIS_RACE;
  }
  else if( is_abbrev( arg1, "racecon" ) )
  {
    flags = APPLY_CON_RACE;
  }
  else if( is_abbrev( arg1, "raceagi" ) )
  {
    flags = APPLY_AGI_RACE;
  }
  else if( is_abbrev( arg1, "racepow" ) )
  {
    flags = APPLY_POW_RACE;
  }
  else if( is_abbrev( arg1, "racecha" ) )
  {
    flags = APPLY_CHA_RACE;
  }
  else if( is_abbrev( arg1, "racekarma" ) )
  {
    flags = APPLY_KARMA_RACE;
  }
  else if( is_abbrev( arg1, "raceluck" ) )
  {
    flags = APPLY_LUCK_RACE;
  }
  else if( is_abbrev( arg1, "curse" ) )
  {
    flags = APPLY_CURSE;
  }
  else if( is_abbrev( arg1, "skillgrant" ) )
  {
    flags = APPLY_SKILL_GRANT;
  }
  else if( is_abbrev( arg1, "skilladd" ) )
  {
    flags = APPLY_SKILL_ADD;
  }
  else if( is_abbrev( arg1, "hitreg" ) )
  {
    flags = APPLY_HIT_REG;
  }
  else if( is_abbrev( arg1, "movereg" ) )
  {
    flags = APPLY_MOVE_REG;
  }
  else if( is_abbrev( arg1, "manareg" ) )
  {
    flags = APPLY_MANA_REG;
  }
  else if( is_abbrev( arg1, "pulsecombat" ) )
  {
    flags = APPLY_COMBAT_PULSE;
  }
  else if( is_abbrev( arg1, "pulsespell" ) )
  {
    flags = APPLY_SPELL_PULSE;
  }
  else
  {
    send_to_char( "Could not find flag.  :(\n", ch );
    return;
  }

  // For each real object..
  for( int r_num = 0; r_num <= top_of_objt; r_num++ )
  {
    // Load a copy of object.
    obj = read_object(r_num, REAL);

    // Check flags for match.
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      if( obj->affected[i].location == flags
        && obj->affected[i].modifier == amount )
      {
        sprintf( buf, "%d: '%s'\n", obj_index[r_num].virtual_number, 
          obj->short_description );
        send_to_char( buf, ch );
      }
    }

    // Free up the object copy.
    extract_obj( obj, FALSE );
  }
}

// Making ships visible 24-7 to dayblind/nightblind.
void list_ships_to_char( P_char ch, int room_no )
{
  P_obj obj;
  static char buf[MAX_STRING_LENGTH];
  static char buf2[MAX_STRING_LENGTH];

  // Clear / initialize the buffer.  Necessary if no ships found.
  buf[0] = '\0';

  // Walk through items in room...
  for( obj = world[room_no].contents; obj; obj = obj->next_content )
  {
    // If it's a ship
    if( obj_index[obj->R_num].func.obj == ship_obj_proc )
    {
      // Add it's description to the buffer.
      // Show vnum if applicable (this will probably never apply unless PLR_MORTAL is toggled on).
      if( IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_VNUM) )
      {
        sprintf( buf2, "[&+B%5d&N] ", (obj->R_num >= 0 ? obj_index[obj->R_num].virtual_number : -1));
        strcat( buf, buf2 );
      }

      // Show long description (& don't crash if it doesn't have one).
      if( obj->description )
      {
        strcat( buf, obj->description );
        strcat( buf, "\n\r" );
      }
      else
      {
        strcat( buf, "&+RThis ship has no long description. &+B:&+g(&n\n\r" );
      }
    }
  }

  // If there were any ships in the room to show...
  if( buf[0] != '\0' )
  {
    page_string(ch->desc, buf, 1);
  }
}
