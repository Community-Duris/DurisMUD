  /****************************************************************************
 *  File: nanny.c                                            Part of Duris   *
 *  Usage: handle non-playing sockets (new character creation too)           *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                               *
 *************************************************************************** */


#include <arpa/telnet.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "mm.h"
#include "sql.h"
#include "sound.h"
#include "assocs.h"
#include "account.h"
#include "files.h"
#include "sql.h"
#include "paladins.h"
#include "ships.h"
#include "specializations.h"
#include "multiplay_whitelist.h"
#include "guildhall.h"
#include "epic.h"
#include "utility.h"
#include "vnum.room.h"

/* external variables */

extern const int top_of_world;
extern P_ereg email_reg_table;
extern int mini_mode;
// extern FILE *help_fl;		// commented by weebler
extern FILE *info_fl;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern char *stat_names[];
extern char valid_term_list[];
extern const struct class_names class_names_table[];
extern const char *command[];
extern const char *fill_words[];
extern const char *rude_ass[];
extern const char *town_name_list[];
extern Skill skills[];
extern const struct race_names race_names_table[];
extern const struct bonus_stat bonus_stats[];
extern const struct con_app_type con_app[];
extern const struct stat_data stat_factor[];
extern const struct racial_data_type racial_data[];
extern const racewar_struct racewar_color[MAX_RACEWAR+2];
extern int avail_descs;
extern int avail_hometowns[][LAST_RACE + 1];
extern int class_table[LAST_RACE + 1][CLASS_COUNT + 1];
extern int guild_locations[][CLASS_COUNT + 1];
extern int innate_abilities[];
extern int innate2_abilities[];
int      invitemode;
extern int racial_values[LAST_RACE + 1][2];
extern int top_of_helpt;
extern int top_of_infot;
extern int used_descs;
// extern struct help_index_element *help_index;	// Commented by weebler
extern struct info_index_element *info_index;
extern struct zone_data *zone_table;
extern struct mm_ds *dead_mob_pool;
extern char *greetings;
extern char *greetinga1;
extern char *greetinga2;
extern char *greetinga3;
extern char *greetinga4;
extern int top_of_mobt;
extern P_index mob_index;
extern void assign_racial_skills(P_char ch);
extern void reset_racial_skills(P_char ch);
extern void GetMIA(char *playerName, char *returned);
extern void GetMIA2(char *playerName, char *returned);
extern int pulse;
extern P_nevent ne_schedule[PULSES_IN_TICK];
extern P_nevent ne_schedule_tail[PULSES_IN_TICK];
extern const struct time_info_data time_info;

#define TOGGLE_BIT(var, bit) ((var) = (var) ^ (bit))
#define PLR_FLAGS(ch)          ((ch)->specials.act)
#define PLR_FLAGGED(ch, flag)  (IS_SET(PLR_FLAGS(ch), flag))
#define PLR_TOG_CHK(ch, flag)  ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))

void     email_player_info(char *, char *, struct descriptor_data *);
extern int email_in_use(char *, char *);
extern void dump_email_reg_db(void);
extern void ereglog(int, const char *, ...);
int      get_name(char return_name[256]);
void displayShutdownMsg(P_char ch);
void event_hatred_check(P_char, P_char, P_obj, void*);
void event_halfling_check(P_char, P_char, P_obj, void*);
void event_smite_evil(P_char, P_char, P_obj, void*);
long unsigned int ip2ul(const char *ip);

unsigned int game_locked = 0;       /* 0x00000001;  no creation */
struct mm_ds *dead_pconly_pool = NULL;
long int highestPCidNumb;

void swapstat( P_desc d, char *arg);
void select_swapstat( P_desc d, char *arg);
void swapstats(P_char ch, int stat1, int stat2);

int getNewPCidNumb(void)
{
  FILE    *file;

  file = fopen(SAVE_DIR "/pc_idnumb", "wt");
  if (!file)
  {
    logit(LOG_FILE, "could not open pc_idnumb file for writing");
    return -1;
  }
  else
  {
    highestPCidNumb++;
    fprintf(file, "%ld\n", highestPCidNumb);
    fclose(file);
  }

  return highestPCidNumb;
}

void setNewPCidNumbfromFile(void)
{
  FILE    *file;

  file = fopen(SAVE_DIR "/pc_idnumb", "rt");
  if (!file)
  {
    file = fopen(SAVE_DIR "/pc_idnumb", "wt");
    if (!file)
    {
      logit(LOG_FILE, "could not open pc_idnumb file for writing");
      highestPCidNumb = 1;
      return;
    }
    fprintf(file, "1");
    fclose(file);
    highestPCidNumb = 1;
  }
  else
  {
    fscanf(file, "%ld\n", &highestPCidNumb);
    fclose(file);
  }

  logit(LOG_STATUS, "highest PC number is %ld", highestPCidNumb);
}

void init_height_weight(P_char ch)
{
  float    f, i;

  if (IS_NPC(ch))
    return;

  i = number(racial_values[GET_RACE(ch) - 1][0],
             racial_values[GET_RACE(ch) - 1][1]);
  /* ok.. now RM comes to play (wonderful book. :)) */

  /* females a tad bit shorter */

  if (ch->player.sex == SEX_FEMALE)
    ch->player.height *= number(90, 100) / 100;

#define tuuma 2.54
  f = (float) i *i * i * ((float) 0.0000107 * 47) / tuuma / tuuma / tuuma;

#undef tuuma
  switch (GET_RACE(ch))
  {
  case RACE_MOUNTAIN:
  case RACE_DUERGAR:
    f = (float) f *1.77;

    break;
  case RACE_HALFLING:
    f = (float) f *1.5;

    break;
  case RACE_GNOME:
  case RACE_GOBLIN:
    f = (float) f *1.3;

    break;
  case RACE_GITHYANKI:
  case RACE_GITHZERAI:
    f = (float) f *0.85;

    break;
  case RACE_GREY:
  case RACE_DROW:
  case RACE_HARPY:
  case RACE_GARGOYLE:
  case RACE_ILLITHID:
  case RACE_PILLITHID:
    f = (float) f *0.75;

    break;
  }
  /* As a rule, females are more slender than males, thus
     (slightly) lower height, and lesser weight. */
  if (ch->player.sex == SEX_FEMALE)
    f = (float) f *0.8;

  /* char's build.. from 70% to 120% of relative height-to-weight
     factor. (malnutrioned / fat) */

  /* fake-gauss, making most of chars about-average of their race,
     but not letting _exactly_ same height vs weight happen oft
     as these weigh look a bit heavy-ish to me, I added slightly smaller
     max add, and added max lower. anyway, these weighs _ARE_ physically
     realistic: think, halfling of 90 cm is certainly more than 1/3 times
     270 cm troll's weight! (actually just 1/27, which this program
     shows, although halflings have some bonus)
   */

  f =
    (float) f *((float) (100 + number(number(-30, -11), number(-9, 20))) /
                100);

  ch->player.weight = (sh_int) f;
  ch->player.height = (sh_int) i;

  /* set size */

  GET_SIZE(ch) = race_size(GET_RACE(ch));
}

static void LoadNewbyShit(P_char ch, int *items)
{
  int      i;
  P_obj    obj;

  for (i = 0; items[i] != -1; i++)
  {
    if ((obj = read_object(items[i], VIRTUAL)) == NULL)
    {
      logit(LOG_DEBUG, "Cannot load init item with virtual number: %d for %s",
            items[i], GET_NAME(ch));
    }
    else
    {
      obj->cost = 1;
      if (obj->type != ITEM_FOOD && obj->type != ITEM_WEAPON && obj->type !=
          ITEM_SPELLBOOK && obj->type != ITEM_LIGHT && obj->type !=
          ITEM_TOTEM && IS_PC(ch))
        SET_BIT(obj->extra_flags, ITEM_TRANSIENT);
      if (obj->type == ITEM_SPELLBOOK)
      {
          for (int j = FIRST_SPELL; j <= LAST_SPELL; j++)
          {
            if (get_spell_circle(ch, j) == 1)
            {
                AddSpellToSpellBook(ch, obj, j);
                obj->value[3]++;
            }
          }
      }
      obj_to_char(obj, ch);
      if (!IS_PC(ch))
        CheckEqWorthUsing(ch, obj);
    }
  }
}

#define CREATE_KIT(race, cls, kit) (newbie_kits[race][flag2idx(cls)] = (kit))

void load_obj_to_newbies(P_char ch)
{
  int     *random;
  int     *newbie_kits[LAST_RACE][CLASS_COUNT + 1];

  static int torch[] = { 1134, -1 };

/*Thrikreen Basics*/
  static int thrikreen_good_eq[] = { 
						  677, 283, 285, 1112, 286, 288, 290,
						  613, 398, 398, 1176, 1167, -1 };

  static int thrikreen_evil_eq[] = { 
						  677, 283, 285, 1112, 286, 288, 290,
						  613, 1170, 1173, -1 };

/*Minotaur Basics*/
  static int minotaur_good_eq[] = { 
						  677, 283, 285, 1112, 286, 288, 290,
						398, 398, 1176, 1167, 1182, 603, 105, 106, 107, -1 };

  static int minotaur_evil_eq[] = { 
						  677, 283, 285, 1112, 286, 288, 290,
						1170, 1173, 1182, 603, 105, 106, 107, -1 };


  memset(newbie_kits, 0, sizeof(newbie_kits));

  CREATE_KIT(RACE_BARBARIAN, 0, ((int[])
                                 {
						  677, 283, 285, 1112, 286, 288, 290,
                                 560, 603, 398, 398, 1154, 1155, -1}));
    CREATE_KIT(RACE_BARBARIAN, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_BARBARIAN, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*Githzerai Basics*/
  CREATE_KIT(RACE_GITHZERAI, 0, ((int[])
                             {
				 677, 283, 285, 1112, 286, 288, 290,
                             566, 390, 398, 398, 1152, 1153, -1}));

/*Githzerai Classes*/
    CREATE_KIT(RACE_GITHZERAI, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_GITHZERAI, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));


/*Human Basics*/
  CREATE_KIT(RACE_HUMAN, 0, ((int[])
                             {
						  677, 283, 285, 1112, 286, 288, 290,
                             566, 390, 398, 398, 1152, 1153, -1}));

/*Human Classes*/
  CREATE_KIT(RACE_HUMAN, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_HUMAN, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));/*END Human Classes*/


/*Drow Elf Basics*/
  CREATE_KIT(RACE_DROW, 0, ((int[])
                            {
						  677, 283, 285, 1112, 286, 288, 290,
                            561, 604, 36016, 1156, 1157, -1}));

/*Drow Elf Classes*/
   CREATE_KIT(RACE_DROW, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_DROW, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_DROW, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_DROW, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_DROW, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_DROW, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_DROW, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_DROW, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_DROW, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_DROW, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_DROW, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_DROW, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_DROW, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_DROW, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_DROW, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_DROW, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_DROW, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_DROW, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_DROW, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*END Drow Elf Classes*/

/*Duergar Basics*/
  CREATE_KIT(RACE_DUERGAR, 0, ((int[])
                               {
						  677, 283, 285, 1112, 286, 288, 290,
                               562, 605, 1164, 1165, -1}));

/*Duergar Classes*/
   CREATE_KIT(RACE_DUERGAR, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_DUERGAR, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));
/*END Duergar Classes*/

/*Gnome Basics*/
  CREATE_KIT(RACE_GNOME, 0, ((int[])
                             {
						  677, 283, 285, 1112, 286, 288, 290,
                             563, 605, 398, 398, 1166, 1167, -1}));

/*Gnome Classes*/
   CREATE_KIT(RACE_GNOME, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_GNOME, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_GNOME, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_GNOME, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));
/*END Gnome Classes*/


/*Half Elf Basics*/
  CREATE_KIT(RACE_HALFELF, 0, ((int[])
                               {
						  677, 283, 285, 1112, 286, 288, 290,
                               564, 607, 398, 398, 1160, 1161, -1}));

/*Half Elf Class EQ*/
  CREATE_KIT(RACE_HALFELF, CLASS_WARRIOR, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1105, 1105,
                                           1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_CONJURER, ((int[])
                                            {
                                            1114, 1115, 1131, 706, 735, 731,
                                            731, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_SUMMONER, ((int[])
                                            {
                                            1114, 1115, 1131, 706, 735, 731,
                                            731, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_SORCERER, ((int[])
                                            {
                                            1114, 1115, 1131, 706, 735, 731,
                                            731, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_NECROMANCER, ((int[])
                                            {
                                            1114, 1115, 1143, 706, 735, 731,
                                            731, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_RANGER, ((int[])
                                          {
                                          1101, 1102, 1103, 1104, 1105, 1105,
                                          1106, 1107, 1113, 1113, 1114, 1115,
                                          1116, 1117, 1117, 1117, 1117, 1117,
                                          1117, 1117, 1117, 1117, 1117, 1118,
                                          -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_DRUID, ((int[])
                                         {
                                         1135, 1136, 1137, 1138, 1139, 1140,
                                         -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_BARD, ((int[])
                                        {
                                        1112, 1128, 1129, 1130, 1131, 1134,
                                        -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_ASSASSIN, ((int[])
                                            {
                                            1112, 1112, 1128, 1129, 1130,
                                            1131, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_THIEF, ((int[])
                                         {
                                         1112, 1128, 1129, 1130, 1131, 1132,
                                         412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_ROGUE, ((int[])
                                         {
                                         1112, 1128, 1129, 1130, 1131, 1132,
                                         412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_PALADIN, ((int[])
	                                         {
			                 1101, 1102, 1103, 1107, 1110, -1}));

  CREATE_KIT(RACE_HALFELF, CLASS_THEURGIST, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, -1}));
/*END Half Elf Classes*/

/*Halfling Basics*/
  CREATE_KIT(RACE_HALFLING, 0, ((int[])
                                {
						  677, 283, 285, 1112, 286, 288, 290,
                                565, 608, 398, 398, 1168, 1169, -1}));

/*Halfling Classes*/
   CREATE_KIT(RACE_HALFLING, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_HALFLING, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*END Halfling Classes*/


/*Thrikreen Classes*/
  CREATE_KIT(RACE_THRIKREEN, CLASS_WARRIOR, ((int[])
                                             {
                                             1101, 1101, 1102, 1104, 1104,
                                             1105, 1105, 1105, 1105, 1106,
                                             580, 580, 580, 580, 581, 581,
                                             -1}));
/*END Thrikreen Classes*/


/*Centaur Basics*/
  CREATE_KIT(RACE_CENTAUR, 0, ((int[])
                               {
						  677, 283, 285, 1112, 286, 288, 290,
                               398, 398, 591, 590, 1178, 1179, -1}));

/*Centaur Classes*/
   CREATE_KIT(RACE_CENTAUR, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_CENTAUR, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*END Centaur Classes*/

  CREATE_KIT(RACE_HARPY, CLASS_SORCERER, ((int[])
                                          {
                                          706, 735, 731, 731, -1}));
  CREATE_KIT(RACE_HARPY, CLASS_CONJURER, ((int[])
                                          {
                                          706, 735, 731, 731, -1}));
  CREATE_KIT(RACE_HARPY, CLASS_SUMMONER, ((int[])
                                          {
                                          706, 735, 731, 731, -1}));
  CREATE_KIT(RACE_HARPY, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));
  CREATE_KIT(RACE_HARPY, CLASS_BARD, ((int[])
                                      {
                                      1128, 1130, 1131, 1134, -1}));

  CREATE_KIT(RACE_HARPY, CLASS_SHAMAN, ((int[])
                                      {
                                      731, 731, 706, 735, 107, 106, 105,
679, 388, -1}));


/*Planetbound Illithid Basic*/
  CREATE_KIT(RACE_ILLITHID, 0, ((int[])
                                {
						  677, 283, 285, 1112, 286, 288, 290,
                                567, 609, 733, 1174, 1175, -1}));

/*Planetbound Illithid Classes*/
  CREATE_KIT(RACE_PILLITHID, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));
  
  CREATE_KIT(RACE_PILLITHID, CLASS_PSIONICIST, ((int[])
                                               {
                                               1112, 706, 1131, -1}));

  CREATE_KIT(RACE_PILLITHID, CLASS_SORCERER, ((int[])
                                              {
                                              1114, 1115, 1131, 706, 735, 731,
                                              731, -1}));
  
  CREATE_KIT(RACE_PILLITHID, CLASS_CONJURER, ((int[])
                                              {
                                              1114, 1115, 1131, 706, 735, 731,
                                              731, -1}));

  CREATE_KIT(RACE_PILLITHID, CLASS_SUMMONER, ((int[])
                                              {
                                              1114, 1115, 1131, 706, 735, 731,
                                              731, -1}));

  CREATE_KIT(RACE_PILLITHID, CLASS_ILLUSIONIST, ((int[])
                                           {
                                           1114, 1115, 1131, 706, 735, 731,
                                           731, -1}));
  
  CREATE_KIT(RACE_ORC, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

/*END Planetbound Illithid Classes*/
					       
/*Illithid Basic*/
  CREATE_KIT(RACE_ILLITHID, 0, ((int[])
                                {
                                567, 609, 733, 1174, 1175, -1}));

/*Illithid Classes*/
  CREATE_KIT(RACE_ILLITHID, CLASS_PSIONICIST, ((int[])
                                               {
                                               1112, 706, 1131, -1}));
/*END Illithid Classes*/


/*Githyanki Basic*/
  CREATE_KIT(RACE_GITHYANKI, 0, ((int[])
                                 {
						  677, 283, 285, 1112, 286, 288, 290,
                                 1180, 1181, -1}));
                                 
  CREATE_KIT(RACE_GITHYANKI, CLASS_CLERIC, ((int[])
                                      {
                                      1119, 1120, 1121, 1123, 1125, 1126,
                                      1127, -1}));

/*Githyanki Classes*/
   CREATE_KIT(RACE_GITHYANKI, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_GITHYANKI, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));
/*End Githyanki Classes*/

/*Phantom Basic*/
  CREATE_KIT(RACE_PHANTOM, 0, ((int[])
                               {
						  677, 283, 285, 1112, 286, 288, 290,
                               1180, 1181, -1}));

/*Phantom Classes*/
  CREATE_KIT(RACE_PHANTOM, CLASS_PSIONICIST, ((int[])
                                              {
                                              624, -1}));

  CREATE_KIT(RACE_PHANTOM, CLASS_REAVER, ((int[])
                                          {
                                          1109, 1108, 1107, 1106, 1105, 1105,
                                          1104, 1103, 1102, 110, 604, 1157,
                                          256, -1}));


/* End Phantom Classes */

/*Minotaur Classes*/
   CREATE_KIT(RACE_MINOTAUR, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_MINOTAUR, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*END Minotaur Classes*/


/*Grey Elf Basics*/
  CREATE_KIT(RACE_GREY, 0, ((int[])
                            {
						  677, 283, 285, 1112, 286, 288, 290,
                            568, 610, 398, 398, 1158, 1159, -1}));

/*Grey Elf Classes*/
  CREATE_KIT(RACE_GREY, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_GREY, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_GREY, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GREY, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GREY, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_GREY, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_GREY, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_GREY, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GREY, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_GREY, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GREY, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GREY, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_GREY, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_GREY, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_GREY, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_GREY, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_GREY, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_GREY, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_GREY, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*END Grey Elf Classes*/


/*Dwarf Basic*/
  CREATE_KIT(RACE_MOUNTAIN, 0, ((int[])
                                {
						  677, 283, 285, 1112, 286, 288, 290,
                                569, 611, 398, 398, 1162, 1163, -1}));

/*Dwarf Classes*/
  CREATE_KIT(RACE_MOUNTAIN, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_MOUNTAIN, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*END Dwarf Classes*/


/*Ogre Basic*/
  CREATE_KIT(RACE_OGRE, 0, ((int[])
                            {
						  677, 283, 285, 1112, 286, 288, 290,
                            570, 612, 1170, 1183, 1183,
                            1184, -1}));

/*Ogre Classes*/
  CREATE_KIT(RACE_OGRE, CLASS_MERCENARY, ((int[])
                                          {
                                          1105, 1105, 1185, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_SHAMAN, ((int[])
                                       {
                                       105, 106, 107, 1127, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_WARRIOR, ((int[])
                                       {
                                       1103, 1104, 1105, 1105,
                                       1106, 1107, 1185, 1109, -1}));
/*END Ogre Classes*/

/*Orog Basics*/
  CREATE_KIT(RACE_OROG, 0, ((int[])
                           {
						  677, 283, 285, 1112, 286, 288, 290,
                           1172, 1173, 612, -1}));
                           
/*Orog Classes*/
   CREATE_KIT(RACE_OGRE, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_OGRE, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_OGRE, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_OGRE, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*Orc Classes*/
   CREATE_KIT(RACE_ORC, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_ORC, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_ORC, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_ORC, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_ORC, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_ORC, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_ORC, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_ORC, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_ORC, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_ORC, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_ORC, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_ORC, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_ORC, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_ORC, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_ORC, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_ORC, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_ORC, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_ORC, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_ORC, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));


/*END Orc Classes*/

/*Lich Basics*/
  CREATE_KIT(RACE_LICH, 0, ((int[])
                             {
						  677, 283, 285, 1112, 286, 288, 290,
                             1172, 1173, 612, -1}));

  /*Lich Classes */
  CREATE_KIT(RACE_LICH, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));


  CREATE_KIT(RACE_LICH, CLASS_DREADLORD, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1105, 1105,
                                           1106, 1107, 286, -1}));

  CREATE_KIT(RACE_LICH, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, -1}));


  /*END Lich Classes */


/*Death Knight Basics*/
  CREATE_KIT(RACE_PDKNIGHT, 0, ((int[])
                                {
						  677, 283, 285, 1112, 286, 288, 290,
                                1106, 1107, -1}));

  /*Death Knight Classes */
  CREATE_KIT(RACE_PDKNIGHT, CLASS_WARRIOR, ((int[])
                                            {
                                            1101, 1102, 1103, 1104, 1105,
                                            1105, 1108, 1109, -1}));

  CREATE_KIT(RACE_PDKNIGHT, CLASS_ANTIPALADIN, ((int[])
                                                {
                                                1101, 1102, 1103, 1111, -1}));


  /*END Death Knight Classes */



/*Vampire Basics*/

  CREATE_KIT(RACE_VAMPIRE, 0, ((int[])
                               {
						  677, 283, 285, 1112, 286, 288, 290,
                               1172, 1173, 612, -1}));

  /*Vampire Classes */


  CREATE_KIT(RACE_VAMPIRE, CLASS_ANTIPALADIN, ((int[])
                                               {
                                               1101, 1102, 1103, 1107, 1111,
                                               -1}));

  CREATE_KIT(RACE_VAMPIRE, CLASS_THIEF, ((int[])
                                         {
                                         1112, 1128, 1129, 1130, 1131, 1132,
                                         412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_VAMPIRE, CLASS_ASSASSIN, ((int[])
                                            {
                                            1112, 1112, 1128, 1129, 1130,
                                            1131, -1}));

  CREATE_KIT(RACE_VAMPIRE, CLASS_WARRIOR, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1105, 1105,
                                           1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_VAMPIRE, CLASS_MERCENARY, ((int[])
                                             {
                                             1101, 1102, 1103, 1104, 1105,
                                             1105, 1106, 1107, 1108, 1112,
                                             -1}));

  CREATE_KIT(RACE_VAMPIRE, CLASS_DREADLORD, ((int[])
                                             {
                                             1101, 1102, 1103, 1104, 1105,
                                             1105, 1106, 1107, 286, -1}));
  /*END Vampire Classes */

/*Wight Basics*/
  CREATE_KIT(RACE_WIGHT, 0, ((int[])
                             {
						  677, 283, 285, 1112, 286, 288, 290,
                             1172, 1173, 612, -1}));
/*Wight Classes*/
  CREATE_KIT(RACE_WIGHT, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, 286, -1}));
  CREATE_KIT(RACE_WIGHT, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, 286, -1}));
/*END Wight Classes*/

/*Storm Giant Basics*/
  CREATE_KIT(RACE_SGIANT, 0, ((int[])
                              {
						  677, 283, 285, 1112, 286, 288, 290,
                              1172, 1173, 612, -1}));
/*Storm Giant Classes*/
  CREATE_KIT(RACE_SGIANT, CLASS_WARRIOR, ((int[])
                                          {
                                          1103, 1104, 1105, 1105, 1106, 1107,
                                          1108, 1109, 286, 274, -1}));
  CREATE_KIT(RACE_SGIANT, CLASS_MERCENARY, ((int[])
                                            {
                                            1103, 1104, 1106, 1107, 1108,
                                            1112, 286, 274, -1}));
/*END Storm Giant Classes*/


/*Shade Basics*/
  CREATE_KIT(RACE_SHADE, 0, ((int[])
                             {
						  677, 283, 285, 1112, 286, 288, 290,
                             1172, 1173, 612, -1}));
/*Shade Classes*/
  CREATE_KIT(RACE_SHADE, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));
  CREATE_KIT(RACE_SHADE, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));
  CREATE_KIT(RACE_SHADE, CLASS_REAVER, ((int[])
                                        {
                                        1109, 1108, 1107, 1106, 1105, 1105,
                                        1104, 1103, 1102, 110, 604, 1157, 256,
                                        -1}));
  CREATE_KIT(RACE_SHADE, CLASS_DREADLORD, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1105, 1105,
                                           1106, 1107, 286, -1}));
  CREATE_KIT(RACE_LICH, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, -1}));



/*END Shade Classes*/

/*Revenant Basics*/
  CREATE_KIT(RACE_REVENANT, 0, ((int[])
                                {
						  677, 283, 285, 1112, 286, 288, 290,
                                1172, 1173, 612, -1}));
/*Revenant Classes*/
  CREATE_KIT(RACE_REVENANT, CLASS_WARRIOR, ((int[])
                                            {
                                            1101, 1102, 1103, 1104, 1105,
                                            1105, 1106, 1107, 1108, 1109,
                                            -1}));
  CREATE_KIT(RACE_REVENANT, CLASS_MERCENARY, ((int[])
                                              {
                                              1101, 1102, 1103, 1104, 1106,
                                              1107, 1108, 1112, -1}));
/*END Revenant Classes*/


/*Shadow beast Basics*/

  CREATE_KIT(RACE_PSBEAST, 0, ((int[])
                               {
						  677, 283, 285, 1112, 286, 288, 290,
                               1172, 1173, 612, -1}));

  /*Shadow Beast Classes */


  CREATE_KIT(RACE_PSBEAST, CLASS_ASSASSIN, ((int[])
                                            {
                                            1112, 1112, 1128, 1129, 1130,
                                            1131, -1}));

  CREATE_KIT(RACE_PSBEAST, CLASS_THIEF, ((int[])
                                         {
                                         1112, 1128, 1129, 1130, 1131, 1132,
                                         412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_PSBEAST, CLASS_WARRIOR, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1105, 1105,
                                           1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_PSBEAST, CLASS_MERCENARY, ((int[])
                                             {
                                             1101, 1102, 1103, 1104, 1106,
                                             1107, 1108, 1112, -1}));
/* End shadow beast */


/*Troll Basic*/
  CREATE_KIT(RACE_TROLL, 0, ((int[])
                             {
						  677, 283, 285, 1112, 286, 288, 290,
                             1172, 1155, 571, 613, -1}));

/*Troll Classes*/
   CREATE_KIT(RACE_TROLL, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_TROLL, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_TROLL, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_TROLL, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*END Troll Classes*/


/*Goblin Basics*/
  CREATE_KIT(RACE_GOBLIN, 0, ((int[])
                              {
						  677, 283, 285, 1112, 286, 288, 290,
                              1172, 1173, 612, -1}));

/*Goblin Classes*/
   CREATE_KIT(RACE_GOBLIN, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_GOBLIN, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));


/*END Goblin Classes*/

/*Drider Basics*/
  CREATE_KIT(RACE_DRIDER, 0, ((int[])
                            {
						  677, 283, 285, 1112, 286, 288, 290,
                            561, 604, 36016, 1156, 1157, -1}));

/*Drider Classes*/
  CREATE_KIT(RACE_DRIDER, CLASS_WARRIOR, ((int[])
                                        {
                                        1101, 1102, 1103, 1104, 1105, 1105,
                                        1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_DRIDER, CLASS_CLERIC, ((int[])
                                       {
                                       1119, 1120, 1121, 1123, 1125, 1126,
                                       1127, -1}));

  CREATE_KIT(RACE_DRIDER, CLASS_SORCERER, ((int[])
                                         {
                                         1114, 1115, 1131, 706, 735, 731, 731,
                                         -1}));

  CREATE_KIT(RACE_DRIDER, CLASS_CONJURER, ((int[])
                                         {
                                         1114, 1115, 1131, 706, 735, 731, 731,
                                         -1}));

  CREATE_KIT(RACE_DRIDER, CLASS_SUMMONER, ((int[])
                                         {
                                         1114, 1115, 1131, 706, 735, 731, 731,
                                         -1}));

  CREATE_KIT(RACE_DRIDER, CLASS_NECROMANCER, ((int[])
                                            {
                                            1112, 1114, 1115, 1141, 1142,
                                            1143, -1}));

  CREATE_KIT(RACE_DRIDER, CLASS_REAVER, ((int[])
                                         {
                                         1109, 1108, 1107, 1106, 1105, 1104,
                                         1103, 1102, 1101, 604, 1157, 1156,
                                         1115, 1114, -1}));

/* END Drider Classes */

  
/*Kobold Basics*/
  CREATE_KIT(RACE_KOBOLD, 0, ((int[])
                              {
						  677, 283, 285, 1112, 286, 288, 290,
                              1172, 1173, 612, -1}));

/*Goblin Classes*/
   CREATE_KIT(RACE_KOBOLD, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_KOBOLD, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));

/*END Kobold Classes*/
  
  
/*Kuo Toa Basic*/
  CREATE_KIT(RACE_KUOTOA, 0, ((int[])
                             {
						  677, 283, 285, 1112, 286, 288, 290,
                             1172, 1155, 571, 613, -1}));

/*Troll Classes*/
  CREATE_KIT(RACE_KUOTOA, CLASS_CLERIC, ((int[])
                                         {706, 735, 731, 731, -1}));
  
  CREATE_KIT(RACE_KUOTOA, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_KUOTOA, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_KUOTOA, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_KUOTOA, CLASS_ROGUE, ((int[])
                                           {
                                           1112, 1128, 1129, 1130, 1131,
                                           412, 412, 412, -1}));

/*END Troll Classes*/

/* Firbolg Basic */
  CREATE_KIT(RACE_FIRBOLG, 0, ((int[])
                                 {
						  677, 283, 285, 1112, 286, 288, 290,
                                 560, 603, 398, 398, 1154, 1155, -1}));

/* Firbolg Classes */  
   CREATE_KIT(RACE_FIRBOLG, CLASS_WARRIOR, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_RANGER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_PALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_ANTIPALADIN, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1110, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_CLERIC, ((int[])
                                        {
                                        1119, 1120, 1121, 1122, 1124, 1126,
                                        1127, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_DRUID, ((int[])
                                       {
                                       1135, 1136, 1137, 1138, 1139, 1140,
                                       -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_SHAMAN, ((int[])
                                        {
                                        105, 106, 107, 1144, 1145, 1146, 1127,
                                        -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_SORCERER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_NECROMANCER, ((int[])
                                             {
                                             1112, 1114, 1115, 1141, 1142,
                                             1143, 203, 204, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_CONJURER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_SUMMONER, ((int[])
                                          {
                                          1114, 1115, 1131, 706, 735, 731,
                                          731, 203, 204, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_MONK, ((int[])
                                      {
                                      1147, 1148, 1149, 1150, 1151, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_MERCENARY, ((int[])
                                           {
                                           1101, 1102, 1103, 1104, 1106, 1107,
                                           1108, 1112, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_ROGUE, ((int[])
                                       {
                                       1112, 1112, 1128, 1129, 1130, 1131, 1132,
                                       412, 412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_BARD, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1134, 1241,
                                      203, 204, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_ALCHEMIST, ((int[])
                                           {
                                           377, 676, 52, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_ILLUSIONIST, ((int[])
                                             {
                                             1114, 1115, 1131, 706, 735, 731,
                                             731, 203, 204, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_ETHERMANCER, ((int[])
                                             {
                                             706, 735, 731, 731, -1}));

  CREATE_KIT(RACE_FIRBOLG, CLASS_REAVER, ((int[])
                                         {
                                         1101, 1102, 1103, 1104, 1105, 1105,
                                         1106, 1107, 1108, 1108, 203, 204, -1}));
/* END Firbolg */

/*Wood Elf Basics*/
  CREATE_KIT(RACE_WOODELF, 0, ((int[])
                            {
						  677, 283, 285, 1112, 286, 288, 290,
                            568, 610, 398, 398, 1158, 1159, -1}));

/*Wood Elf Classes*/
  CREATE_KIT(RACE_WOODELF, CLASS_WARRIOR, ((int[])
                                        {
                                        1101, 1102, 1103, 1104, 1105, 1105,
                                        1106, 1107, 1108, 1109, -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_RANGER, ((int[])
                                       {
                                       1101, 1102, 1103, 1104, 1105, 1105,
                                       1106, 1107, 1113, 1113, 1114, 1115,
                                       1116, 1117, 1117, 1117, 1117, 1117,
                                       1117, 1117, 1117, 1117, 1117, 1118,
                                       -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_CLERIC, ((int[])
                                       {
                                       1119, 1120, 1121, 1122, 1124, 1126,
                                       1127, -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_CONJURER, ((int[])
                                         {
                                         1114, 1115, 1131, 706, 735, 731, 731,
                                         -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_SUMMONER, ((int[])
                                         {
                                         1114, 1115, 1131, 706, 735, 731, 731,
                                         -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_DRUID, ((int[])
                                      {
                                      1135, 1136, 1137, 1138, 1139, 1140,
                                      -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_SORCERER, ((int[])
                                         {
                                         1114, 1115, 1131, 706, 735, 731, 731,
                                         -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_THIEF, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1132, 412,
                                      412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_ROGUE, ((int[])
                                      {
                                      1112, 1128, 1129, 1130, 1131, 1132, 412,
                                      412, 412, 412, 412, -1}));

  CREATE_KIT(RACE_WOODELF, CLASS_BARD, ((int[])
                                     {
                                     1112, 1128, 1129, 1130, 1131, 1134,
                                     -1}));

/*END Wood Elf Classes*/

  if (ch->carrying && IS_PC(ch))        /* we are _NOT_ here to give people free eq many times */
    return;

  if (newbie_kits[GET_RACE(ch)][0])
    LoadNewbyShit(ch, newbie_kits[GET_RACE(ch)][0]);

  if (GET_RACE(ch) == RACE_THRIKREEN)
    if (GET_ALIGNMENT(ch) >= 0)
      LoadNewbyShit(ch, thrikreen_good_eq);
    else
      LoadNewbyShit(ch, thrikreen_evil_eq);

  if (GET_RACE(ch) == RACE_MINOTAUR)
    if (GET_ALIGNMENT(ch) >= 0)
      LoadNewbyShit(ch, minotaur_good_eq);
    else
      LoadNewbyShit(ch, minotaur_evil_eq);

  if (newbie_kits[GET_RACE(ch)][flag2idx(ch->player.m_class)])
  {
    LoadNewbyShit(ch, newbie_kits[GET_RACE(ch)][flag2idx(ch->player.m_class)]);
  }
  else if( GET_CLASS( ch, CLASS_BLIGHTER ) )
  {
                      // shield, weapons*5, armor*8.
    LoadNewbyShit(ch, (int[]){1109, 1108, 1113, 1112, 1140, 677, 239, 618, 679, 680, 729, 729, 452, 437, -1});
  }

  if (world[ch->in_room].number == 29201)
  {
    P_obj note = read_object(29319, VIRTUAL);

    obj_to_char(note, ch);
  }

 P_obj bandage = read_object(393, VIRTUAL);
 obj_to_char(bandage, ch);

 bandage = read_object(393, VIRTUAL);
  obj_to_char(bandage, ch);

 bandage = read_object(393, VIRTUAL);
  obj_to_char(bandage, ch);

 bandage = read_object(393, VIRTUAL);
  obj_to_char(bandage, ch);

 bandage = read_object(393, VIRTUAL);
  obj_to_char(bandage, ch);

 bandage = read_object(458, VIRTUAL);
  obj_to_char(bandage, ch);

}

#undef CREATE_KIT

/* check for a legal player name, since it's only called when a new character
   is created, we can make it pretty much as detailed as we want, thus:
   must be all alphabetic
   must be from 2-10 characters long
   must not be a command word (*command[])
   or a fill word (*fill[]) (though it can contain these strings)
   or some other silly ass things ('The' 'Other' 'Someone' etc, bleah)
   note that it does not exclude dumbass, profane, offensive etc names, like
   SuckMeSchlong, GodsAreDicks, EatMe, etc, the gods will just have to deal
   with those on an individual basis.
   also note, I could have made it exclude all mob and obj keywords, but
   that would get really restrictive, if someone else wants to do it, feel
   free.
   -JAB */

bool _parse_name(char *arg, char *name)
{
  int      i;
  const char *smart_ass[] = {
    "someone",
    "somebody",
    "me",
    "self",
    "all",
    "group",
    "local",
    "them",
    "they",
    "nobody",
    "any",
    "something",
    "other",
    "no",
    "yes",
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "shape",                    /* infra.. */
    "shadow",                   /* summon */
    "northeast",
    "southeast",
    "northwest",
    "southwest",
    "nw",
    "ne",
    "sw",
    "se",
    "guide",
    "he",
    "she",
    "it",
    "him",
    "his",
    "her",
    "boy",
    "girl",
    "man",
    "woman",
    "it",
    "mail",
    "male",
    "female",
    "duris", // blocking duris auto-62 for now.
    "\n"
  };

  if( strlen(arg) > MAX_NAME_LENGTH )        /* max name size */
  {
    return TRUE;
  }

  if (strlen(arg) < 2)          /* min name size */
    return TRUE;

  for (i = 0; i < strlen(arg); i++)
  {
    name[i] = LOWER(arg[i]);
    /* check for high bit chars, non-alphas, and if any letter other
       then the first is CAPS */
    if ((arg[i] < 0) || !isalpha(arg[i]) || (i && (name[i] != arg[i])))
      return TRUE;
  }
  name[strlen(arg)] = '\0';

  /* if any player or mob already has this name, we can't use it */

  for (i = 0; i <= top_of_mobt; i++)
    if (isname(name, mob_index[i].keys))
      return TRUE;

  if( search_block(name, command, TRUE) >= 0 )
    return TRUE;
  if( search_block(name, fill_words, TRUE) >= 0 )
    return TRUE;
  if( search_block(name, smart_ass, TRUE) >= 0 )
    return TRUE;
  if( sub_string_set(name, rude_ass) )
    return TRUE;

  return FALSE;
}

/* simple anti-cracking measure, require at least a minimally secure password */

bool valid_password(P_desc d, char *arg)
{
  char    *p, name[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];
  int      i, ucase, lcase, other;

  if (strlen(arg) < 5)
  {
    SEND_TO_Q("Passwords must be at least 5 characters long.\r\n", d);
    return FALSE;
  }
  /* sure as I'm writing this code, some feeb will use one of my examples as a password. JAB */

  if (!strncmp("HjuoB", arg, 5) || !strncmp("4ys-&c9", arg, 7) ||
      !strncmp("$s34567", arg, 7))
  {
    SEND_TO_Q
      ("Did I, or did I not, say '(note don't use these EXACT passwords!)'.\r\n"
       "(Answer: Yes, I damn well did, try again)\r\n", d);
    return FALSE;
  }
  i = -1;
  do
  {
    i++;
    password[i] = LOWER(*(arg + i));
  }
  while (*(arg + i));

  i = -1;
  do
  {
    i++;
#ifndef USE_ACCOUNT
    name[i] = LOWER(*(d->character->player.name + i));
  }
  while (*(d->character->player.name + i));
#else
    name[i] = LOWER(*(d->account->acct_name + i));
  }
  while (*(d->account->acct_name + i));
#endif

  if (strstr(name, password) || strstr(password, name))
  {
    SEND_TO_Q
      ("Don't even THINK about using your character's name as a password.\r\n",
       d);
    return FALSE;
  }
  /* stole this from linux passwd.c */

  other = ucase = lcase = 0;
  for (p = arg; *p; p++)
  {
    ucase = ucase || isupper(*p);
    lcase = lcase || islower(*p);
    other = other || !isalpha(*p);
  }

  if ((!ucase || !lcase) && !other)
  {
    SEND_TO_Q
      ("Valid passwords contain a mixture of upper and lowercase letters, or a mixture\r\n"
       "of letter and numbers and symbols, or all 4 elements.  Examples:\r\n"
       "HjuoB, 4ys-&c9, $s34567 (note don't use these EXACT passwords!)\r\n",
       d);
    return FALSE;
  }
  return TRUE;
}


/*
 * Turn on echoing (sepcific to telnet client)
 * Turn on echoing after echo has been turned off by "echo_off".  This
 * function only works if the player is using a telnet client since
 * it sends it TELNET protocol sequence to turn echo on.  "sock" is
 * presumed to be a connected socket to the client player.
 */

void echo_on(P_desc d)
{
  char     on_string[] = {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) TELOPT_NAOFFD,
    (char) TELOPT_NAOCRD,
    (char) 0
  };

  SEND_TO_Q(on_string, d);
}

/*
 * Turn off echoing (specific to telnet client)
 */

void echo_off(P_desc d)
{
  char     off_string[] = {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  SEND_TO_Q(off_string, d);
}

int number_of_players(void)
{
  P_desc   d;
  int      count = 0;

  for (d = descriptor_list; d != NULL; d = d->next)
  {
    count++;
  }

  return (count);
}

void perform_eq_wipe(P_char ch)
{
  static long longestptime = 0;
  struct time_info_data playing_time;
  int i;
  P_obj obj, obj2;
  P_ship ship;
  char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  send_to_char("&+ROh shit, it seems we have misplaced your items...\n", ch );
//  send_to_char("&+RUpon further examination it appears your bank account is empty as well.\n\r", ch );
  send_to_char("&+RYour boat isnt where you left it.\n" ,ch);
//  send_to_char("&+RYou don't have any frags.\n", ch);
//  send_to_char("&+RYou don't have any epicss.\n", ch);
  send_to_char("&+RThis can mean only one of two things.  Either you've just been\n"
               "&+R robbed or this i the eq-wipe.  Have a nice day.\r\n", ch);

  // actually remove their eq!
  for (i = 0; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
    {
      extract_obj(unequip_char(ch, i), TRUE);
    }
  }
  for (obj = ch->carrying; obj; obj = obj2)
  {
    obj2 = obj->next_content;
    extract_obj(obj, TRUE);
    obj = NULL;
  }

  // Delete the locker as well
  sprintf( Gbuf2, "%c%s", LOWER(*ch->player.name), ch->player.name + 1 );
  sprintf( Gbuf1, "%s/%c/%s.locker", SAVE_DIR, *Gbuf2, Gbuf2 );
  sprintf( Gbuf2, "rm -f %s %s.bak", Gbuf1, Gbuf1 );
  system( Gbuf2 );

  // Delete the ship too
  if( ship = get_ship_from_owner(ch->player.name) )
  {
    shipObjHash.erase(ship);
    delete_ship(ship);
  }
  else
    debug( "%s had no ship.", ch->player.name );

  if( longestptime < ch->player.time.played )
  {
    longestptime = ch->player.time.played;
    playing_time = real_time_passed((long) ((time(0) - ch->player.time.logon) + ch->player.time.played), 0);
    sprintf( Gbuf1, "New Longest Ptime: '%s' %d with %d %dD%dH%dM%dS",
      J_NAME(ch), ch->only.pc->pid, ch->player.time.played, playing_time.day, playing_time.hour, playing_time.minute, playing_time.second );
    debug( Gbuf1 );
    logit( LOG_STATUS, Gbuf1 );
  }
/*
  ch->only.pc->frags = 0;
  ch->only.pc->epics = 0;

  GET_BALANCE_COPPER(ch) = 1;
  GET_BALANCE_SILVER(ch) = 0;
  GET_BALANCE_GOLD(ch) = 0;
  GET_BALANCE_PLATINUM(ch) = 0;

  GET_PLATINUM(ch) = 0;
  GET_GOLD(ch) = 0;
  GET_SILVER(ch) = 0;
  GET_COPPER(ch) = 0;
*/
}

//#define MAX_HT_ESCAPE -1
int alt_hometown_check(P_char ch, int room, int count)
{
  //struct zone_data *current_zone;
  //int new_room, new_count;
  //int good_rooms[] = {95553,6074, 66001, 39310};
  //int evil_rooms[] = {11901, 15264, 97628, 36539, 17021};

  //if (count > MAX_HT_ESCAPE) {
  //  return room;
  //}
  //new_count = count + 1;

  //current_zone = &zone_table[world[room].zone];

  //if (current_zone->status > ZONE_NORMAL) {
  //  new_room = number(0,4);
  //  if (EVIL_RACE(ch)) {
  //    send_to_char("&+RThe town is currently under attack, &+Wyou are rushed to safety!\n", ch);
  //    return alt_hometown_check(ch, real_room(evil_rooms[new_room]), new_count);
  //  }

  //  if (GOOD_RACE(ch)) {
  //    send_to_char("&+RThe town is currently under attack, &+Wyou are rushed to safety!\n", ch);
  //    return alt_hometown_check(ch, real_room(good_rooms[new_room]), new_count);
  //  }
  //}

  return room;
}

void schedule_pc_events(P_char ch)
{
  // Not sure if this is happening...
  if( !IS_ALIVE(ch) )
  {
    debug( "schedule_pc_events: DEAD/NONEXISTANT char %s '%s'.", (ch==NULL) ? "!" : IS_NPC(ch) ? "NPC" : "PC",
      (ch==NULL) ? "NULL" : J_NAME(ch) );
    logit( LOG_DEBUG, "schedule_pc_events: DEAD/NONEXISTANT char %s '%s'.",
      (ch==NULL) ? "!" : IS_NPC(ch) ? "NPC" : "PC", (ch==NULL) ? "NULL" : J_NAME(ch) );
    return;
  }

  add_event(event_autosave, 1200, ch, 0, 0, 0, 0, 0);
  if( has_innate(ch, INNATE_HATRED) )
    add_event(event_hatred_check, get_property("innate.timer.hatred", WAIT_SEC), ch, 0, 0, 0, 0, 0);
  if (GET_CHAR_SKILL(ch, SKILL_SMITE_EVIL))
    add_event(event_smite_evil, get_property("skill.timer.secs.smiteEvil", 5) * WAIT_SEC, ch, 0, 0, 0, 0, 0);
  if (GET_RACE(ch) == RACE_HALFLING)
    add_event(event_halfling_check, 1, ch, 0, 0, 0, 0, 0);

  if ( affected_by_spell(ch, SPELL_RIGHTEOUS_AURA) )
    add_event(event_righteous_aura_check, WAIT_SEC, ch, 0, 0, 0, 0, 0);

  if ( affected_by_spell(ch, SPELL_BLEAK_FOEMAN) )
    add_event(event_bleak_foeman_check, WAIT_SEC, ch, 0, 0, 0, 0, 0);
}
/*
 *    existing or new character entering game
 */
void enter_game(P_desc d)
{
  struct  zone_data *zone;
  struct affected_type af1, *afp1, *afp2;
  crm_rec *crec = NULL;
  int      cost;
  int r_room = NOWHERE;
  long     time_gone = 0, hit_g, move_g, heal_time, rest;
  time_t   ct = time(NULL);
  int      mana_g;
  char     Gbuf1[MAX_STRING_LENGTH], timestr[MAX_STRING_LENGTH];
  bool     nobonus = FALSE;
  P_char   ch = d->character;
  P_desc   i;
  P_nevent evp;
  P_Guild  guild;

  // Bring them to life!
  SET_POS(ch, POS_STANDING + STAT_NORMAL);

  // Then put them in a room.
  if ((d->rtype == RENT_QUIT && GET_LEVEL(ch) < 2) || d->rtype == RENT_DEATH)
  {
    /* defaults to birthplace on quit/death */
    r_room = real_room(GET_BIRTHPLACE(ch));
  }
  else if (d->rtype == RENT_CRASH)
  {
    r_room = real_room(ch->specials.was_in_room);
  }
  else
  {
    r_room = real_room(ch->specials.was_in_room);

    if(r_room == NOWHERE)
      r_room = ch->in_room;
  }

  if (zone_table[world[r_room].zone].flags & ZONE_CLOSED)
    r_room = real_room(GET_BIRTHPLACE(ch));

  if (ch->only.pc->pc_timer[PC_TIMER_HEAVEN] > ct)
  {
    if( IS_RACEWAR_GOOD(ch) )
      r_room = real_room(GOOD_HEAVEN_ROOM);
    else if( IS_RACEWAR_EVIL(ch) )
      r_room = real_room(EVIL_HEAVEN_ROOM);
    else if( IS_RACEWAR_UNDEAD(ch) )
      r_room = real_room(UNDEAD_HEAVEN_ROOM);
    else if( IS_ILLITHID(ch) )
      r_room = real_room(NEUTRAL_HEAVEN_ROOM);
    else if( IS_RACEWAR_NEUTRAL(ch) )
      r_room = real_room(NEUTRAL_HEAVEN_ROOM);
    // Cage people on undefined racewar sides.  That'll get a fix quick
    else
      r_room = real_room(VROOM_CAGE);
  }

  if (r_room == NOWHERE)
  {
    if (GET_HOME(ch))
      r_room = real_room(GET_HOME(ch));
    else
      r_room = real_room(GET_BIRTHPLACE(ch));

    if (r_room == NOWHERE)
      if (IS_TRUSTED(ch))
        r_room = real_room0(1200);
      else
        r_room = GET_ORIG_BIRTHPLACE(ch);

    if (r_room == NOWHERE)
      r_room = real_room0(11);
  }
  // old guildhalls (deprecated)
//  else if (world[r_room].number >= 48000 &&
//           world[r_room].number <= 48999 &&
//           find_house(world[r_room].number) == NULL)
//  {
//    GET_HOME(ch) = GET_BIRTHPLACE(ch) = GET_ORIG_BIRTHPLACE(ch);
//    r_room = real_room(GET_HOME(ch));
//  }
  else if (world[r_room].number >= SHIP_ZONE_START &&
           world[r_room].number <= SHIP_ZONE_END)
  {
    r_room = real_room(GET_BIRTHPLACE(ch));
  }

  // Stick them in the cage of smoke!
  if( r_room > top_of_world )
    r_room = real_room(11);
  // Stick them in An Empty Dimension
  if( r_room < 0 )
    r_room = real_room(1197);

  // check home/birthplace/spawn room to see if it's in a GH and if ch is allowed
  r_room = check_gh_home(ch, r_room);

  ch->in_room = NOWHERE;
  char_to_room(ch, r_room, -2);

  ch->specials.x_cord = 0;
  ch->specials.y_cord = 0;
  ch->specials.z_cord = 0;

  if( GET_LEVEL(ch) )
  {
    ch->desc = d;

    reset_char(ch);

    cost = 0;

    if ((d->rtype == RENT_CRASH) || (d->rtype == RENT_CRASH2))
    {
      send_to_char("\r\nRestoring items and pets from crash save info...\r\n",
                   ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if (d->rtype == RENT_CAMPED)
    {
      send_to_char("\r\nYou break camp and get ready to move on...\r\n", ch);
      cost = restoreItemsOnly(ch, 0);
    }
    else if (d->rtype == RENT_INN)
    {
      send_to_char("\r\nRetrieving rented items from storage...\r\n", ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if (d->rtype == RENT_LINKDEAD)
    {
      send_to_char("\r\nRetrieving items from linkdead storage...\r\n", ch);
      cost = restoreItemsOnly(ch, 200);
    }
    else if(d->rtype == RENT_POOFARTI)
    {
      send_to_char("\r\nThe gods have taken your artifact...\r\n", ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if(d->rtype == RENT_FIGHTARTI)
    {
      nobonus = TRUE;
      send_to_char("\r\nYour artifacts argued all night...\r\n", ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if(d->rtype == RENT_SWAPARTI)
    {
      send_to_char("\r\nThe gods have taken your artifact... and replaced it with another!\r\n", ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if (d->rtype == RENT_DEATH)
    {
      if (ch->only.pc->pc_timer[PC_TIMER_HEAVEN] > ct)
        send_to_char("\r\nYour soul finds its way to the afterlife...\r\n", ch);
      else
        send_to_char("\r\nYou rejoin the land of the living...\r\n", ch);
      restoreItemsOnly(ch, 0);
    }
    else
    {
      send_to_char("\r\nCouldn't find any items in storage for you...\r\n", ch);
    }

  if (cost == -2)
    {
      send_to_char
        ("\r\nSomething is wrong with your saved items information - "
         "please talk to an Implementor.\r\n", ch);
    }
    /* to avoid problems if game is shutdown/crashed while they are in 'camp'
       mode, kill the affect if it's active here. */

    if (IS_AFFECTED(ch, AFF_CAMPING))
      affect_from_char(ch, TAG_CAMP);

    ch->specials.affected_by = 0;
    ch->specials.affected_by2 = 0;
    ch->specials.affected_by3 = 0;
    ch->specials.affected_by4 = 0;
    ch->specials.affected_by5 = 0;

    if(affected_by_spell(ch, AIP_YOUSTRAHDME))
      affect_from_char(ch, AIP_YOUSTRAHDME);

    if(affected_by_spell(ch, AIP_YOUSTRAHDME2))
      affect_from_char(ch, AIP_YOUSTRAHDME2);

    /* remove any morph flag that might be leftover */
    REMOVE_BIT(ch->specials.act, PLR_MORPH | PLR_WRITE | PLR_MAIL);

    /* remove any sacking events */
    // old guildhalls (deprecated)
    //    clear_sacks(ch);

    /* this may fix the disguise not showing on who bug */
    if( PLR_FLAGGED(ch, PLR_NOWHO) )
      PLR_TOG_CHK(ch, PLR_NOWHO);

    /* check mail
       if (mail_ok && has_mail(GET_NAME(ch)))
       send_to_char("&=LWMail awaits you at your local postoffice.&n\r\n", ch);
     */

    // time_gone is how many ticks (currently real minutes) they have been out of the game.
    time_gone = (ct - ch->player.time.saved) / SECS_PER_MUD_HOUR;
    // rest is how many seconds they have been out of the game.
    rest = ct - ch->player.time.saved;

    ch->player.time.birth -= time_gone;

    SET_POS(ch, POS_STANDING + STAT_NORMAL);
    heal_time = MAX(0, (time_gone - 120));

    if (d->rtype != RENT_DEATH)
    {
      hit_g = BOUNDED(0, hit_regen(ch) * heal_time, 3000);
      mana_g = BOUNDED(0, mana_regen(ch) * heal_time, 3000);
      move_g = BOUNDED(0, move_regen(ch) * heal_time, 3000);
    }
    else
    {
      hit_g = mana_g = move_g = 0;
    }

    GET_HIT(ch)      = BOUNDED(1, GET_HIT(ch) + hit_g, GET_MAX_HIT(ch));
    GET_MANA(ch)     = BOUNDED(1, GET_MANA(ch) + mana_g, GET_MAX_MANA(ch));
    GET_VITALITY(ch) = BOUNDED(1, GET_VITALITY(ch) + move_g, GET_MAX_VITALITY(ch));

    if (GET_HIT(ch) != GET_MAX_HIT(ch))
      StartRegen(ch, EVENT_HIT_REGEN);
    if (GET_MANA(ch) != GET_MAX_MANA(ch))
      StartRegen(ch, EVENT_MANA_REGEN);
    if (GET_VITALITY(ch) != GET_MAX_VITALITY(ch))
      StartRegen(ch, EVENT_MOVE_REGEN);

    set_char_size(ch);

    update_skills(ch);
// Once racial skills are removed, this will be unnecessary.
//  Furthermore, it will wipe any formerly-racial now-epic skills learned. - Lohrr
//    reset_racial_skills( ch );

    set_surname(ch, 0);
  }
  /* don't do any of above for new chars */

  send_to_char(WELC_MESSG, ch);
  ch->desc = d;
  ch->next = character_list;
  character_list = ch;

  // Need to walk through ch->affects, and drop AFFTYPE_OFFLINE timers.
  for( afp1 = ch->affected; afp1; afp1 = afp2 )
  {
    afp2 = afp1->next;

    if( IS_SET(afp1->flags, AFFTYPE_OFFLINE) )
    {
      /* Debugging:
      sprintf( Gbuf1, "enter_game: afp '%s' has AFFTYPE_OFFLINE\n\r", skills[afp1->type].name );
      SEND_TO_Q( Gbuf1, d);
       */
      if( IS_SET(afp1->flags, AFFTYPE_SHORT) )
      {
        LOOP_EVENTS_CH( evp, ch->nevents )
        {
          if( evp->func == event_short_affect && evp->data != NULL
            && ((struct event_short_affect_data*)(evp->data))->af == afp1 )
          {
            /* The following is my thinking through how this works (You can ignore this if you already know):
             * So, the math goes: multiply event->timer * PULSES_IN_TICK so we have the number of 'rounds'
             *   we make through the ne_schedule[] array until the event actually fires.  I had to modify this
             *   calculation to take into account whether the current pulse has passed the current event or not.
             * Then, we have to add event->element - pulse.  This correlates to which row of ne_schedule[]
             *   we're currently executing (global variable pulse) vs which row the event is in.
             * Once we do this math, we have a positive value of pulses that corresponds to how long it will be
             *   before the event fires (in it's old position).
             * Now we need to subtract the number of seconds of offline time * the number of pulses in a second.
             *   This is the local variable rest (number of secs) * WAIT_SEC (pulses in a sec = 4 on 6/16/2015).
             * Now, if our new number of pulses left before the event expires is negative or 0, it's time to
             *   poof *afp1.  We do this via wear_off_messages (so they know it poofed), and affect_remove.
             * If our new number of pulses left is positive, we need to move the event from it's old row to a new
             *   one. So, we pull the event from it's row in ne_schedule[], updating the head/tail if necessary.
             *   Then we update evp->timer (pulses left / number of rows + 1).  The + 1 to make it range from 1..
             *   instead of 0.., which is important because the first thing ne_events() does is decrement evp->timer.
             *   Now we update evp->element ((pulses left + pulse) % number of rows).  The + pulse is because that's
             *   where the mud is currently executing.
             *   Finally, we add evp to the tail of ne_schedule[evp->element] (since that's easy) and handles properly
             *   for when evp->element == pulse.
             * I probably should've just written pull_event_from_schedule, and add_event_to_schedule functions, but
             *   hey, this is where I wrote it and I'm lazy. (You can break the code into functions if you want).  Perhaps
             *   an event_warp_time_left( P_nevent event, int pulses_lost ) or such would work. *shrug*
             */
            long total_pulses = (evp->timer - ((evp->element < pulse) ? 0 : 1)) * PULSES_IN_TICK + evp->element - pulse;
            /* Debugging:
            sprintf( Gbuf1, "enter_game: short afp '%s': timer: %d, element: %d, pulse: %d, rest(pulses): &+Y%ld&n.\n\r"
              "enter_game: timer(pulses): %d, element - pulse: %d, total pulses: &+Y%ld&n, total pulses - rest: &+Y%ld&n\n\r"
              "enter_game: old time left on event(pulses/sec): %d/%d, old timer: &+B%d&n, old element: &+B%d&n.\n\r",
              skills[afp1->type].name, evp->timer, evp->element, pulse, rest * WAIT_SEC,
              (evp->timer - ((evp->element < pulse) ? 0 : 1)) * PULSES_IN_TICK, evp->element - pulse, total_pulses,
              total_pulses - rest * WAIT_SEC,
              ne_event_time(evp), ne_event_time(evp) / WAIT_SEC, evp->timer, evp->element );
            SEND_TO_Q( Gbuf1, d);
            */
            // Calculate the new number of pulses we want the event to last.
            if( (total_pulses = total_pulses - rest * WAIT_SEC) < 1 )
            {
              // If the event would've fired while they were logged off, fire it now.
              wear_off_message( ch, afp1 );
              affect_remove( ch, afp1 );
              break;
            }

            // Pull evp from ne_schedule[] list.
            // If we're not at the end.
            if( evp->next_sched )
            {
              evp->next_sched->prev_sched = evp->prev_sched;
            }
            else if( evp == ne_schedule_tail[evp->element] )
            {
              ne_schedule_tail[evp->element] = evp->prev_sched;
            }
            else
            {
              raise(SIGSEGV);
            }

            // If we're not at the beginning.
            if( evp->prev_sched )
            {
              evp->prev_sched->next_sched = evp->next_sched;
            }
            else if( evp == ne_schedule[evp->element] )
            {
              ne_schedule[evp->element] = evp->next_sched;
            }
            else
            {
              raise(SIGSEGV);
            }

            // Update the timer.  The +1 is because we want the range from 1..MAX not 0..MAX,
            //   since the first thing ne_events() does is decrement the timer.
            evp->timer = (total_pulses) / PULSES_IN_TICK + 1;

            // total_pulses + pulse because our starting point is ne_schedule[pulse].
            evp->element = (total_pulses + pulse) % PULSES_IN_TICK;

            // Add evp to the end of the new row.
            evp->next_sched = NULL;
            evp->prev_sched = ne_schedule_tail[evp->element];
            // If there was a previous element.
            if( evp->prev_sched != NULL )
            {
              evp->prev_sched->next_sched = evp;
            }
            // Otherwise, this is the only element in the list, so add it to the head.
            else
            {
              ne_schedule[evp->element] = evp;
            }
            ne_schedule_tail[evp->element] = evp;
            /* Debugging:
            sprintf( Gbuf1, "enter_game: new time left on event(pulses/sec): %d/%d, new timer: &+B%d&n, new element: &+B%d&n.\n\r",
              ne_event_time(evp), ne_event_time(evp) / WAIT_SEC, evp->timer, evp->element );
            SEND_TO_Q( Gbuf1, d);
            */
            break;
          }
        }
      }
      else if( time_gone > 0 )
      {
        /* Debugging:
        sprintf( Gbuf1, "enter_game: not-short afp '%s' old duration: %d, new duration: %ld.\n\r",
          skills[afp1->type].name, afp1->duration, afp1->duration - time_gone );
        GetMIA2(ch->player.name, Gbuf1 + strlen(Gbuf1) );
        strcat( Gbuf1, "\n\r" );
        SEND_TO_Q( Gbuf1, d);
        */
        afp1->duration -= time_gone;
        if( afp1->duration < 0 )
        {
          affect_remove( ch, afp1 );
        }
      }
    }
  }

  affect_total(ch, FALSE);

  if( (guild = GET_ASSOC( ch )) != NULL )
  {
    guild->update_member( ch );
    if( IS_MEMBER(GET_A_BITS( ch )) )
    {
      do_gmotd( ch, "", CMD_GMOTD );
    }
  }

  /* check the fraglist .. */

  checkFragList(ch);
  if (!ch->player.short_descr)
    generate_desc(ch);

  if (!ch->player.name)
  {
    wizlog(57, "&+WSomething fucked up with character name. Tell a coder immediately!&n");
    SEND_TO_Q("Serious screw-up with your player file. Log on another char and talk to gods.", d);
    STATE(d) = CON_FLUSH;
  }

  if (!d->host)
  {
    wizlog(57, "%s had null host.", GET_NAME(ch));
    sprintf(d->host, "UNKNOWN \0");
  }

  ch->only.pc->last_ip = ip2ul(d->host);

  if (!d->login)
  {
    wizlog(57, "%s had null login.", GET_NAME(ch));
    sprintf(d->login, "UNKNOWN \0");
  }

  if (IS_TRUSTED(ch))
  {
/*
   ch->only.pc->wiz_invis = MIN(59,GET_LEVEL(ch) - 1);
 */
    ch->only.pc->wiz_invis = 56;
    do_vis(ch, 0, -4);          /* remind them of vis level */
  }
  if (d->rtype == RENT_DEATH)
  {
    act("$n has returned from the dead.", TRUE, ch, 0, 0, TO_ROOM);
    GET_COND(ch, FULL) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = 0;
  }
  else
    act("$n has entered the game.", TRUE, ch, 0, 0, TO_ROOM);

  // inform gods that a newbie has entered the game
  if( IS_NEWBIE(ch))
  {
    statuslog(ch->player.level, "&+GNEWBIE %s HAS ENTERED THE GAME! Help him out :) ", GET_NAME(ch));
    // Message to guides.
    sprintf( Gbuf1, "&+GNEWBIE %s HAS ENTERED THE GAME! Help him out :)\n", 
      GET_NAME(ch));
    for (i = descriptor_list; i; i = i->next)
    {
      if(i->connected)
        continue;

      if( opposite_racewar( ch, i->character ) )
        continue;
      if(!IS_SET(i->character->specials.act2, PLR2_NCHAT))
        continue;
      if(!IS_SET(PLR2_FLAGS(i->character), PLR2_NEWBIE_GUIDE))
        continue;
      if(IS_DISGUISE_PC(i->character) ||
        IS_DISGUISE_ILLUSION(i->character) ||
        IS_DISGUISE_SHAPE(i->character))
        continue;

      send_to_char(Gbuf1, i->character, LOG_PRIVATE);
    }
  }

  if (!GET_LEVEL(ch))
  {
    do_start(ch, 0);
    load_obj_to_newbies(ch);
    set_town_flag_justice(ch, TRUE);
  }
  else if (IS_SET(ch->specials.act2, PLR2_NEWBIEEQ) && !ch->carrying)
    load_obj_to_newbies(ch);

  // hack to handle improperly set highest_level
  if( ch->only.pc->highest_level > MAXLVL )
  {
    ch->only.pc->highest_level = GET_LEVEL(ch);
  }

  // Add well-rested or rested bonus, if applicable.
  if( nobonus )
  {
  }
  // 20 hrs (almost a day) -> 2.5h well-rested bonus.
  else if( rest / 3600 >= 20 )
  {
    affect_from_char(ch, TAG_WELLRESTED);
    affect_from_char(ch, TAG_RESTED);

    memset(&af1, 0, sizeof(struct affected_type));
    af1.type = TAG_WELLRESTED;
    af1.modifier = 0;
    af1.duration = 150;
    af1.location = 0;
    af1.flags = AFFTYPE_PERM | AFFTYPE_NODISPEL | AFFTYPE_OFFLINE;
    affect_to_char(ch, &af1);

    debug( "'%s' getting well-rested bonus!", J_NAME(ch) );
  }
  // 9 hrs (sleep + eat) -> 2.5h rested bonus.
  else if( rest / 3600 >= 9 )
  {
    affect_from_char(ch, TAG_WELLRESTED);
    affect_from_char(ch, TAG_RESTED);

    memset(&af1, 0, sizeof(struct affected_type));
    af1.type = TAG_RESTED;
    af1.modifier = 0;
    af1.duration = 150;
    af1.location = 0;
    af1.flags = AFFTYPE_PERM | AFFTYPE_NODISPEL | AFFTYPE_OFFLINE;
    affect_to_char(ch, &af1);

    debug( "'%s' getting rested bonus!", J_NAME(ch) );
  }

  GetMIA(ch->player.name, Gbuf1 );
  // Convert to EST.
  ct -= 4*60*60;
  sprintf(timestr, "%s", asctime( localtime(&ct) ));
  *(timestr + strlen(timestr) - 1) = '\0';
  strcat( timestr, " EST" );
  ct += 4*60*60;

  loginlog(GET_LEVEL(ch), "%s [%s] enters game @ %s.%s [%d]",
           GET_NAME(ch), d->host, timestr, Gbuf1, world[ch->in_room].number);
  sql_log(ch, CONNECTLOG, "Entered Game");

  if(GET_LEVEL(ch) >= MINLVLIMMORTAL)
    loginlog(GET_LEVEL(ch), "&+GIMMORTAL&n: (%s) [%s] has logged on.%s",
             GET_NAME(ch), d->host, Gbuf1);

//  /* multiplay check */
//  for (P_desc k = descriptor_list; k; k = k->next)
//  {
//    if( d == k || !k->character )
//      continue;
//    
//    if (k->connected == CON_PLYNG && d->host && k->host && !str_cmp(d->host, k->host) )
//    {
//      logit(LOG_STATUS, "%s and %s are logged in from the same IP address",
//            d->character->player.name, k->character->player.name);
//      sql_log(d->character, PLAYERLOG, "%s and %s logged in from same IP address", d->character->player.name, k->character->player.name);
//
//      if( d->character->in_room != k->character->in_room )
//      {
//        wizlog(AVATAR, "%s and %s are logged in from the same IP address but not in the same room",
//               d->character->player.name, k->character->player.name);
//      }
//    }
//  }

  set_town_flag_justice(ch, FALSE);

  /* clean up justice goofs */
  /* ignore linkdead, camp, quit from fixing, to avoid clearing
     cheaters. We dont care about rent, since there is no inn in jail */
  if (d->rtype != RENT_QUIT && d->rtype != RENT_LINKDEAD &&
      d->rtype != RENT_CAMPED && (CHAR_IN_TOWN(ch)))
  {
    while ((crec = crime_find(hometowns[CHAR_IN_TOWN(ch) - 1].crime_list,
                              NULL, NULL, 0, NOWHERE, J_STATUS_JAIL_TIME,
                              crec)))
    {
      crec->crime = CRIME_NONE;
      crec->status = J_STATUS_DELETED;
      if (ch->in_room == real_room(hometowns[CHAR_IN_TOWN(ch) - 1].jail_room))
      {
        char_from_room(ch);
        char_to_room(ch, GET_BIRTHPLACE(ch), -1);
      }
    }
  }

  // CTF - level them up, and setbit hardcore off them!
#if defined(CTF_MUD) && (CTF_MUD == 1)
  // setbit hardcore  off
  REMOVE_BIT(ch->specials.act2, PLR2_HARDCORE_CHAR);
  // if not trusted, make sure they are level 55
  if (GET_LEVEL(ch) == 53)
  {
    ch->player.level = 52;  // so they are raised one level, which will fix skills
  }
  //while ((GET_LEVEL(ch) < 56 && !IS_MULTICLASS_PC(ch)) || GET_LEVEL(ch) < 51)
  // changing this to conform with Kitsero's version of chaos
  while (GET_LEVEL(ch) < 53)
  {
    advance_level(ch);
  }
#endif

  // chaos - level them up, and setbit hardcore off them!
#if defined(CHAOS_MUD) && (CHAOS_MUD == 1)
  // setbit hardcore  off
  REMOVE_BIT(ch->specials.act2, PLR2_HARDCORE_CHAR);
  // if not trusted, make sure they are level 55
  if (GET_LEVEL(ch) == 56)
  {
    ch->player.level = 54;  // so they are raised one level, which will fix skills
  }
  //while ((GET_LEVEL(ch) < 56 && !IS_MULTICLASS_PC(ch)) || GET_LEVEL(ch) < 51)
  // changing this to conform with Kitsero's version of chaos
  while (GET_LEVEL(ch) < 56)
  {
    advance_level(ch);
  }
#endif

  writeCharacter(ch, 1, NOWHERE);
  sql_save_player_core(ch);
  sql_connectIP(ch);
  displayShutdownMsg(ch);


  /* initialize infobar */
  // Disabling

  if(IS_SET(ch->specials.act, PLR_SMARTPROMPT))
     REMOVE_BIT(ch->specials.act, PLR_SMARTPROMPT);
  //if (IS_SET(ch->specials.act, PLR_SMARTPROMPT) && IS_ANSI_TERM(d))
  //  InitScreen(ch);

  schedule_pc_events(ch);

//  play_sound(SOUND_START_GAME, ch, 0, TO_CHAR);

  if (EVIL_RACE(ch) && PLR_FLAGGED(ch, PLR_NOWHO))
  {
    PLR_TOG_CHK(ch, PLR_NOWHO);
  }

  struct affected_type *af;

  if(IS_AFFECTED5(ch, AFF5_HOLY_DHARMA))
  {
    affect_from_char(ch, SPELL_HOLY_DHARMA);
    send_to_char("&+cThe &+Cdivine &+cinspiration withdraws from your soul.&n\r\n", ch);
  }

  if(IS_SET(ch->specials.act, PLR_ANONYMOUS))
  {
    REMOVE_BIT(ch->specials.act, PLR_ANONYMOUS);
  }

  affect_from_char( ch, TAG_SKILL_TIMER );

  memset(&af1, 0, sizeof(af1));
  af1.type = TAG_SKILL_TIMER;
  af1.flags = AFFTYPE_STORE | AFFTYPE_SHORT;
  af1.duration = 5 * WAIT_MIN;

  af1.modifier = TAG_MENTAL_SKILL_NOTCH;
  affect_to_char(ch, &af1);

  af1.modifier = TAG_PHYS_SKILL_NOTCH;
  affect_to_char(ch, &af1);

  initialize_logs(ch, true);

  send_offline_messages(ch);
//make sure existing chars have base stats 80 - Drannak
if(d->character->base_stats.Str < 80)
  {
  d->character->base_stats.Str = 80;
  }
if(d->character->base_stats.Agi < 80)
  {
  d->character->base_stats.Agi = 80;
  }
if(d->character->base_stats.Dex < 80)
  {
  d->character->base_stats.Dex = 80;
  }
if(d->character->base_stats.Con < 80)
  {
  d->character->base_stats.Con = 80;
  }
if(d->character->base_stats.Luk < 80)
  {
  d->character->base_stats.Luk = 80;
  }
if(d->character->base_stats.Pow < 80)
  {
  d->character->base_stats.Pow = 80;
  }
if(d->character->base_stats.Int < 80)
  {
  d->character->base_stats.Int = 80;
  }
if(d->character->base_stats.Wis < 80)
  {
  d->character->base_stats.Wis = 80;
  }
 

 if(d->character->base_stats.Cha < 80)
  {
  d->character->base_stats.Cha = 80;
  }

  //goodie AP fix
  if(GET_CLASS(ch, CLASS_ANTIPALADIN) && GET_ALIGNMENT(ch) > -10)
  GET_ALIGNMENT(ch) = -1000;

  //goodie mino necro fix
  if(GET_CLASS(ch, CLASS_NECROMANCER) && GET_ALIGNMENT(ch) > -10)
  GET_ALIGNMENT(ch) = -1000;

#ifdef EQ_WIPE
  if(d->character->player.time.played < EQ_WIPE)
  {
    if( !IS_TRUSTED(d->character) )
    {
      perform_eq_wipe(ch);
    }
    ch->player.time.played += EQ_WIPE;
  }
#endif

  if( affected_by_spell(ch, SPELL_CURSE_OF_YZAR) )
  {
    if( !affected_by_spell(ch, TAG_RACE_CHANGE) )
    {
      // First race change in 5 sec.
      add_event(event_change_yzar_race, 5 * WAIT_SEC, ch, ch, NULL, 0, NULL, sizeof(NULL));
    }
    else
    {
      // Amount of mud-hours until 3am.
      int time_to_witching_hour = (time_info.hour >= 3) ? (27 - time_info.hour) : (3 - time_info.hour);
      // Convert to seconds.
      time_to_witching_hour = time_to_witching_hour * PULSES_IN_TICK;
      // Subtract time passed in current mud hour.
      time_to_witching_hour -= (300 - ne_event_time(get_scheduled( event_another_hour )));

      add_event(event_change_yzar_race, time_to_witching_hour, ch, ch, NULL, 0, NULL, sizeof(NULL));
    }
  }
  // This is to remove the racial epic skills set with TAG_RACIAL_SKILLS
  // after the current wipe (as of 4/25/14) this should be removed - Torgal
//  clear_racial_skills(ch); - And removed. - Lohrr
  do_look(ch, 0, -4);
}

void select_terminal(P_desc d, char *arg)
{
  int      term;
  int      temp = 1;
  char     temp_buf[200];

  if ((term = (int) strtol(arg, NULL, 0)) == 0)
  {
    if (*arg == '?')
      term = TERM_HELP;
    else if (!*arg)             /* carriage return */
      term = TERM_ANSI;
    else
      term = TERM_UNDEFINED;
  }
  switch (term)
  {
  case TERM_GENERIC:
    d->term_type = TERM_GENERIC;
    SEND_TO_Q(greetings, d);
    break;
  case TERM_MSP:
    d->term_type = TERM_MSP;
    arg = one_argument(arg, temp_buf);
    strcpy(d->client_str, arg);
    SEND_TO_Q(greetinga, d);
    break;
  case TERM_ANSI:
    d->term_type = TERM_ANSI;
    arg = one_argument(arg, temp_buf);
    strcpy(d->client_str, arg);

    temp = number(1, NUM_ANSI_LOGINS);
    switch (temp)
    {
    case 1:
      SEND_TO_Q(greetinga, d);
      break;
    case 2:
      SEND_TO_Q(greetinga1, d);
      break;
    case 3:
      SEND_TO_Q(greetinga2, d);
      break;
    case 4:
      SEND_TO_Q(greetinga3, d);
      break;
    case 5:
      SEND_TO_Q(greetinga4, d);
      break;
    default:
      SEND_TO_Q(greetinga, d);
      break;
    }
    break;
  case TERM_SKIP_ANSI:
    d->term_type = TERM_ANSI;
    break;
  case TERM_HELP:
    SEND_TO_Q(valid_term_list, d);
    SEND_TO_Q
      ("Please enter term type (<CR> for ANSI, '1' for Generic, '9' for Quick): ",
       d);
    return;
  default:
    SEND_TO_Q("Unknown terminal type!\r\n", d);
    SEND_TO_Q
      ("Please re-enter term type (<CR> for ANSI, '1' for Generic, '9' for Quick): ",
       d);
    return;
  }

  /* if it gets here, we have a valid term type, carry on... */
#ifndef USE_ACCOUNT
  STATE(d) = CON_NME;
  SEND_TO_Q
    ("By what name do you wish to be known? Type 'generate' to generate names.",
     d);
#else
  //  account stuff instead of name
  STATE(d) = CON_GET_ACCT_NAME;
  SEND_TO_Q("Please enter your account name: ", d);
#endif
}

bool pfile_exists(const char *dir, char *name)
{
  char     buf[256], *buff;
  struct stat statbuf;
  char     Gbuf1[MAX_STRING_LENGTH];

  strcpy(buf, name);
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  sprintf(Gbuf1, "%s/%c/%s", dir, buf[0], buf);
  if (stat(Gbuf1, &statbuf) != 0)
  {
    sprintf(Gbuf1, "%s/%c/%s", dir, buf[0], name);
    if (stat(Gbuf1, &statbuf) != 0)
      return FALSE;
  }
  return TRUE;
}

int is_invited(char *name)
{
  return (pfile_exists("Players/Invited", name));
}

void create_denied_file(const char *dir, char *name)
{
  char     buf[256], *buff;
  char     Gbuf1[MAX_STRING_LENGTH];
  FILE    *f;

  strcpy(buf, name);
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  sprintf(Gbuf1, "%s/%c/%s", dir, buf[0], buf);
  if( f = fopen(Gbuf1, "w") )
  {
    fclose(f);
  }
  else
  {
    debug("create_denied_file: Couldn't open file '%s' for writing.", Gbuf1 );
  }
}

void accept_name(char *name)
{
  create_denied_file("Players/Accepted", name);
}

void deny_name(char *name)
{
  create_denied_file(BADNAME_DIR, name);
}

void select_name(P_desc d, char *arg, int flag)
{
  char     tmp_name[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  P_desc   t_d = NULL;
  int      i = 1;

  for (; isspace(*arg); arg++) ;
  if (!*arg)
  {
	SEND_TO_Q("Bad name, please try another.\r\n", d);
	SEND_TO_Q("Name: ", d);

  //  close_socket(d);
    return;
  }
  if (_parse_name(arg, tmp_name))
  {
    SEND_TO_Q("Illegal name, please try another.\r\n", d);
    SEND_TO_Q("Name: ", d);
    return;
  }
  else
  {
    for (t_d = descriptor_list; t_d; t_d = t_d->next)
      if ((t_d != d) && t_d->character && t_d->connected &&
          !str_cmp(tmp_name, GET_NAME(t_d->character)))
      {
        close_socket(t_d);
        break;
        /*
        SEND_TO_Q
          ("Your char is stuck at the menu. Try another name, and ask a god for help, or wait a few minutes for it to clear.",
           d);
        SEND_TO_Q("Name: ", d);
        return;
        */
      }
  }

  /* capitalize the first letter of name */
  *tmp_name = toupper(*tmp_name);

  /* first time through here?  If so, let's latch on a character struct */
  if (!d->character)
  {
    d->character = (struct char_data *) mm_get(dead_mob_pool);
    clear_char(d->character);
    if (!dead_pconly_pool)
      dead_pconly_pool = mm_create("PC_ONLY",
                                   sizeof(struct pc_only_data),
                                   offsetof(struct pc_only_data, switched),
                                   mm_find_best_chunk(sizeof
                                                      (struct pc_only_data),
                                                      10, 25));
    d->character->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

    d->character->only.pc->aggressive = -1;
    d->character->desc = d;

    setCharPhysTypeInfo(d->character);
  }
  /* get passwd */

  if (isname("generate", tmp_name))
  {
    SEND_TO_Q("\nI'd suggest one of the following names for you:\n\n", d);
    while (i < 13)
    {
      get_name(tmp_name);
      SEND_TO_Q("&+W", d);
      SEND_TO_Q(tmp_name, d);
      if (i < 12)
        SEND_TO_Q("&n, ", d);
      else
        SEND_TO_Q(".", d);
      if (i == 4 || i == 8)
        SEND_TO_Q("\n", d);
      i++;
    }
    SEND_TO_Q("\n\r\n\r", d);
    SEND_TO_Q("Enter a name or type 'generate' to generate more names.\n", d);
    STATE(d) = CON_NME;
    return;

  }

  //WIPE2013 - Drannak
 /* if (!pfile_exists("Players", tmp_name))
  {
    SEND_TO_Q
      ("Duris is currently undergoing a player wipe. We will re-open our doors on Monday, Sept. 9th at 9:00AM MST.\r\n Please see the DurisMUD forums for information at www.durismud.com.\r\nName:",
       d);
    return;
  }*/


  if (!pfile_exists(SAVE_DIR, tmp_name) &&
      pfile_exists(BADNAME_DIR, tmp_name))
  {
    SEND_TO_Q
      ("That name has been declined before, and would be now too!\r\nName:",
       d);
    return;
  }

  if (flag)
  {
    if ((d->rtype = restorePasswdOnly(d->character, tmp_name)) >= 0)
    {

      /* legal name for existing character */
      SEND_TO_Q("Password: ", d);
      STATE(d) = CON_PWDNRM;
      echo_off(d);
      return;
    }
    else if (d->rtype == -2)
    {
      /* player file exists, but there is a problem reading it */
      SEND_TO_Q
        ("Seems to be a problem reading that player file.  Please choose another\r\n"
         "name and report this problem to an Immortal.\r\n\r\n", d);
      if (d->character)
      {
        free_char(d->character);
        d->character = NULL;
      }
      STATE(d) = CON_NME;
      return;
    }
  }
  else if (pfile_exists(SAVE_DIR, tmp_name))
  {
    SEND_TO_Q("Name is in use already. Please enter new name.\r\nName:", d);
    return;
  }
  else if (pfile_exists(BADNAME_DIR, tmp_name))
  {
    SEND_TO_Q
      ("That name has been declined before, and would be now too!\r\nName:",
       d);
    return;
  }
  /* new player */
  if (IS_SET(game_locked, LOCK_CREATE) || !strcmp(get_mud_info("lock").c_str(), "create"))
  {
    if (!flag && d->character)
    {
      free_char(d->character);
      d->character = NULL;
    }
    SEND_TO_Q(motd.c_str(), d);
    STATE(d) = CON_NME;
    return;
  }
  else if (bannedsite(d->host, 1))
  {
    SEND_TO_Q
      ("New characters have been banned from your site. If you want the ban lifted\r\n"
       "mail duris@duris.org with a _LENGTHY_ explanation about\r\n"
       "why, or who could have forced us to ban the site in the first place.\r\n"
       "          - The Management \r\n\r\n"
       "By what name do you wish to be known? ", d);
    banlog(AVATAR, "&+yNew Character reject from %s, banned.", d->host);
    STATE(d) = CON_NME;
    return;
  }
  else if ((IS_SET(game_locked, LOCK_CONNECTIONS)) ||
           ((IS_SET(game_locked, LOCK_MAX_PLAYERS)) &&
            (number_of_players() >= MAX_PLAYERS_BEFORE_LOCK)))
  {
    SEND_TO_Q("Game is temporarily full.  Please try again later.\r\n", d);
    STATE(d) = CON_FLUSH;
    return;
  }
  else
  {

    if (flag)
    {
      d->character->player.name = str_dup(tmp_name);
      sprintf(Gbuf1, "You wish to be known as %s (Y/N)? ", tmp_name);
      SEND_TO_Q(Gbuf1, d);
      STATE(d) = CON_NMECNF;
      return;
    }
    else
    {
      FREE(d->character->player.name);
      d->character->player.name = str_dup(tmp_name);
      STATE(d) = CON_ACCEPTWAIT;
      SEND_TO_Q
        ("Now you just have to wait for re-acceptance or declination of your char.\r\n",
         d);
      return;
    }
  }
  /* should never get here!!! */
  logit(LOG_EXIT, "create_name: should never get here!!");
  raise(SIGSEGV);
}

P_char find_ch_from_same_host(P_desc d)
{
  // first, run through descriptor list to see if they are connected
  for (P_desc k = descriptor_list; k; k = k->next)
  {
    if( d == k || !k->character )
      continue;
    
    if (k->connected == CON_PLYNG && 
        d->character != k->character && 
        !IS_TRUSTED(k->character) && 
        d->host && k->host && 
        !str_cmp(d->host, k->host) )
    {
      // ch connected from same host
      return k->character;
    }
  }  
  
  // next, run through character list to make sure they didn't just drop link  
  for (P_char tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
  {
    if (!tmp_ch->desc && 
        IS_PC(tmp_ch) && 
        str_cmp(GET_NAME(tmp_ch), GET_NAME(d->character)) &&
        !IS_TRUSTED(tmp_ch) && 
        tmp_ch->only.pc->last_ip == ip2ul(d->host) )
    {
      return tmp_ch;
    }
  }
  
  return NULL;
}

// Checks to see if they're violating the one hour rule.
bool violating_one_hour_rule( P_desc d )
{
  int racewar_side;
  int timer;

  // Immortals don't violate the one hour rule, ever.
  if( GET_LEVEL(d->character) >= MINLVLIMMORTAL )
  {
    return FALSE;
  }

  timer = sql_find_racewar_for_ip( d->host, &racewar_side );

  if( racewar_side < RACEWAR_NONE || racewar_side > MAX_RACEWAR )
  {
    return FALSE;
  }

  // If they haven't been on in an hour (or never before).
  if( racewar_side == RACEWAR_NONE )
    return FALSE;
  // If they're on the same racewar side.
  if( racewar_side == GET_RACEWAR(d->character) )
    return FALSE;
  if( timer <= 0 )
    return FALSE;

  wizlog( AVATAR, "%s tried to break the one-hour rule.", GET_NAME(d->character) );
  sql_log( d->character, PLAYERLOG, "Tried to break the one-hour rule.", GET_NAME(d->character) );

  send_to_char_f( d->character, "\n\rYou need to wait longer before logging a character on a different"
    " racewar side.\n\rCurrent side: &+%c%s&n, Time to clear: %d:%02d\n\rPress return to disconnect.\n\r",
    racewar_color[racewar_side].color, racewar_color[racewar_side].name, timer / 60, timer % 60 );
  return TRUE;
}

bool is_multiplaying(P_desc d)
{
  if( IS_TRUSTED(d->character) )
  {
    return false;
  }
  
  if (P_char t_ch = find_ch_from_same_host(d))
  {
    if (whitelisted_host(d->host))
    {
      wizlog(AVATAR, "%s on multiplay whitelist, entering game.", GET_NAME(d->character));
      sql_log(d->character, PLAYERLOG, "On multiplay whitelist, entering game.");
      
      SEND_TO_Q("\r\nYou are on the approved list for multiple players from the same network.\r\n"
                "&+RIf you abuse this privilege, you will be dealt with harshly when we catch you!&n\r\n"
                "Otherwise, enjoy!\r\n", d);
      return false;
    }
    else
    {
      wizlog(AVATAR, "%s tried to enter the game while already logged on as %s", GET_NAME(d->character), GET_NAME(t_ch));
      sql_log(d->character, PLAYERLOG, "Tried to enter game while already logged on as %s", GET_NAME(t_ch));
      
      char buf[MAX_STRING_LENGTH];
      
      sprintf(buf, "\r\nYou are already in the game as %s, and you need to rent or camp them before you can\r\n"
              "enter the game with a new character.\r\n\r\n"
              "If you are multiple people playing from the same location, please petition or send an email to multiplay@durismud.com\r\n"
              "and if approved we can allow multiple connections from your location.\r\n\r\n", GET_NAME(t_ch));
      
      SEND_TO_Q(buf, d);
      return true;
    }
  }
  
  return false;
}

void reconnect(P_desc d, P_char tmp_ch)
{
  echo_on(d);
  SEND_TO_Q("Reconnecting.\r\n", d);
  free_char(d->character);
  d->character = NULL;
  tmp_ch->desc = d;
  d->character = tmp_ch;
  sql_connectIP(tmp_ch);
  tmp_ch->only.pc->last_ip = ip2ul(d->host);
  tmp_ch->specials.timer = 0;
  STATE(d) = CON_PLYNG;
  act("$n has reconnected.", TRUE, tmp_ch, 0, 0, TO_ROOM);
  logit(LOG_COMM, "%s [%s@%s] has reconnected.",
        GET_NAME(d->character), d->login, d->host);
  loginlog(d->character->player.level, "%s [%s@%s] has reconnected.",
           GET_NAME(d->character), d->login, d->host);
  sql_log(d->character, CONNECTLOG, "Reconnected");
  /* if they were morph'ed when they lost link, put them
   back... */
  if (IS_SET(tmp_ch->specials.act, PLR_MORPH))
  {
    if (!tmp_ch->only.pc->switched ||
        !IS_MORPH(tmp_ch->only.pc->switched) ||
    /*              (tmp_ch != ((P_char)
     tmp_ch->only.pc->switched->only.npc->memory))) */
        (tmp_ch != tmp_ch->only.pc->switched->only.npc->orig_char))
    {
      logit(LOG_EXIT,
            "Something fucked while trying to reconnect linkless morph");
      raise(SIGSEGV);
    }
    d->original = tmp_ch;
    d->character = tmp_ch->only.pc->switched;
    d->character->desc = d;
    tmp_ch->desc = NULL;
  }
  send_offline_messages(d->character);
}

void select_pwd(P_desc d, char *arg)
{
  P_char   tmp_ch;
  P_desc   k;
  char     Gbuf1[MAX_STRING_LENGTH];

  switch (STATE(d))
  {

    /* password for existing player */
  case CON_PWDNRM:
    if (!*arg)
    {
      close_socket(d);
    }
    else
    {
      if( (d->character->only.pc->pwd[0] != '$' && strn_cmp(CRYPT(arg, d->character->only.pc->pwd), d->character->only.pc->pwd, 10))
        || (d->character->only.pc->pwd[0] == '$' && strcmp( CRYPT2(arg, d->character->only.pc->pwd), d->character->only.pc->pwd)) )
      {
        SEND_TO_Q("Invalid password.\r\n", d);
        SEND_TO_Q("Invalid password ... disconnecting.\r\n", d);
        if (!IS_TRUSTED(d->character))
        {
          logit(LOG_PLAYER, "Invalid password for %s from %s@%s.",
                GET_NAME(d->character), d->login, d->host);
          sql_log(d->character, CONNECTLOG, "Invalid Password");
        }
        STATE(d) = CON_FLUSH;
        return;
      }

      /* Check if already playing */
      for (k = descriptor_list; k; k = k->next)
      {
        if ((k->character != d->character) && k->character)
        {
          if (k->original)
          {
            if (GET_NAME(k->original) &&
                (!str_cmp(GET_NAME(k->original), GET_NAME(d->character))))
            {
              SEND_TO_Q("Overriding old connection...\r\n", d);
              close_socket(k);
            }
          }
          else
          {                     /* No switch has been made */
            if (GET_NAME(k->character) &&
                (!str_cmp(GET_NAME(k->character), GET_NAME(d->character))))
            {
              SEND_TO_Q("Overriding old connection...\r\n", d);
              close_socket(k);
            }
          }
        }
      }

      for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
      {
        if (!tmp_ch->desc && IS_PC(tmp_ch) &&
            !str_cmp(GET_NAME(d->character), GET_NAME(tmp_ch)))
        {
          reconnect(d, tmp_ch);
          return;
        }
      }

      if ((d->rtype = restoreCharOnly(d->character, GET_NAME(d->character))) >= 0)
      {

        /* by reserving the last available socket for an immort, gods should
           almost always be able to connect.  JAB */
        if ((used_descs >= avail_descs) && (GET_LEVEL(d->character) < AVATAR))
        {
          SEND_TO_Q("Sorry, the game is almost full and the last slot is reserved...\r\n", d);
          STATE(d) = CON_FLUSH;
          return;
        }
      }
      else if (d->rtype == -2)
      {
        /* player file exists, but there is a problem reading it */
        SEND_TO_Q
          ("Seems to be a problem reading that player file.  Please choose another\r\n"
           "name and report this problem to an Immortal.\r\n\r\n", d);
        if (d->character)
        {
          free_char(d->character);
          d->character = NULL;
        }
        STATE(d) = CON_NME;
        return;
      }

      if( IS_SET(game_locked, LOCK_CONNECTIONS) && !IS_TRUSTED(d->character) )
      {
        SEND_TO_Q("\r\nGame is temporarily closed to additional players.\r\n",
                  d);
        SEND_TO_Q("Please try again later.  -The Mgmt\r\n", d);
        STATE(d) = CON_FLUSH;
        return;
      }

      if( (IS_SET(game_locked, LOCK_MAX_PLAYERS)) && !IS_TRUSTED(d->character)
        && (number_of_players() >= MAX_PLAYERS_BEFORE_LOCK) )
      {
        SEND_TO_Q("\r\nGame is currently full.  Please try again later.\r\n",
                  d);
        SEND_TO_Q
          ("Note 5pm - 8am EST there are no limits on connections.\r\n", d);
        STATE(d) = CON_FLUSH;
        return;
      }

      // multiplay check: if the user already has another character in game, don't let them connect a new character
      if( is_multiplaying(d) )
      {
        STATE(d) = CON_FLUSH;
        return;
      }
      // One hour rule check: if the user has had a char on a different racewar side w/in an hour.
      if( violating_one_hour_rule(d) )
      {
        STATE(d) = CON_FLUSH;
        return;
      }

      logit(LOG_COMM, "%s [%s@%s] has connected.", GET_NAME(d->character),
            d->login, d->host);
      sql_log(d->character, CONNECTLOG, "Connected");

      if( IS_TRUSTED(d->character) )
      {
        if( !wizconnectsite(d->host, GET_NAME(d->character), 0) )
        {
          wizlog(AVATAR, "WARNING: %s connected from an invalid site: %s",
                 GET_NAME(d->character), d->host);
          SEND_TO_Q("Sorry, that host is not allowed to connect to this character.\r\n", d);
          STATE(d) = CON_FLUSH;
          return;
        }
        SEND_TO_Q(wizmotd.c_str(), d);
      }
      else
      {
        SEND_TO_Q(motd.c_str(), d);
      }

      // Use better passwords now.
      if( d->character->only.pc->pwd[0] != '$' )
      {
        SEND_TO_Q("\n\r\n\r&=LRUpgrading password - All characters now in use!&n\n\r\n\r", d);
        strcpy( d->character->only.pc->pwd, CRYPT2(arg, GET_NAME(d->character)) );
      }

      SEND_TO_Q("\r\n*** PRESS RETURN: ", d);
      STATE(d) = CON_RMOTD;
      echo_on(d);
    }
    break;

    /* password for a new player */
  case CON_PWDGET:
    echo_on(d);
    if( !valid_password(d, arg) )
    {
      sprintf(Gbuf1, "Please enter a password for %s: ", GET_NAME(d->character));
      SEND_TO_Q(Gbuf1, d);
      echo_off(d);
      return;
    }
    strcpy( d->character->only.pc->pwd, CRYPT2(arg, d->character->player.name) );
    echo_on(d);
    SEND_TO_Q("\r\nPlease retype password: ", d);
    echo_off(d);

    STATE(d) = CON_PWDCNF;
    break;

    /* confirmation of new password */
  case CON_PWDCNF:
    if( strcmp(CRYPT2(arg, d->character->only.pc->pwd), d->character->only.pc->pwd) )
    {
      echo_on(d);
      sprintf(Gbuf1,"Passwords don't match.\r\nPlease enter a password for %s: ", GET_NAME(d->character));
      SEND_TO_Q(Gbuf1, d);
      echo_off(d);
      STATE(d) = CON_PWDGET;
      return;
    }
    echo_on(d);

	// send to "are you a newbie on duris?" question
	SEND_TO_Q("\r\nAnswer the following question honestly, as you will either get help, or not.", d);
	SEND_TO_Q("\r\nAre you NEW to the World of Duris? (y/n) ", d);
	STATE(d) = CON_NEWBIE;
/*    SEND_TO_Q(racetable, d);
    STATE(d) = CON_QRACE;*/
    break;

    /* new password for an existing player */
  case CON_PWDNEW:
    if( strcmp(CRYPT2(arg, d->character->only.pc->pwd), d->character->only.pc->pwd) )
    {
      echo_on(d);
      SEND_TO_Q("\r\nInvalid password, password change aborted.\r\n", d);
      STATE(d) = CON_SLCT;
      SEND_TO_Q(MENU, d);
      return;
    }
    echo_on(d);
    SEND_TO_Q("\r\nEnter your new password: ", d);
    echo_off(d);
    STATE(d) = CON_PWDNGET;
    break;

    /* Retype new pw when changing */
  case CON_PWDNGET:
    echo_on(d);
    if (!valid_password(d, arg))
    {
      SEND_TO_Q("\r\nPassword: ", d);
      echo_off(d);
      return;
    }
    strcpy(d->character->only.pc->pwd, CRYPT2(arg, d->character->player.name) );
    echo_on(d);
    SEND_TO_Q("\r\nPlease retype your new password: ", d);
    echo_off(d);
    STATE(d) = CON_PWDNCNF;
    break;

    /* Confirm pw for changing pw */
  case CON_PWDNCNF:
    echo_on(d);
    if( strcmp(CRYPT2(arg, d->character->only.pc->pwd), d->character->only.pc->pwd) )
    {
      SEND_TO_Q("\r\nPasswords don't match.\r\nPassword change aborted\r\n",
                d);
      /* restore old pwd */
      strcpy(d->character->only.pc->pwd, d->old_pwd);
      STATE(d) = CON_SLCT;
      SEND_TO_Q(MENU, d);
      return;
    }
    SEND_TO_Q("Password changed, you must enter game and save and/or rent for the change\r\n"
      "to be made permanent.\r\n", d);

    STATE(d) = CON_SLCT;
    SEND_TO_Q(MENU, d);
    if (d->rtype > 20)
      d->rtype -= 20;           /* let them off the hook (for an expired password).  JAB */
    break;

    /* Confirm pw for deleting character */
  case CON_PWDDCNF:
    if( strcmp(CRYPT2(arg, d->character->only.pc->pwd), d->character->only.pc->pwd) )
    {
      echo_on(d);
      SEND_TO_Q("\r\nInvalid password, character delete aborted.\r\n", d);
      STATE(d) = CON_SLCT;
      SEND_TO_Q(MENU, d);
      return;
    }
    SEND_TO_Q("\r\nDeleting character...\r\n\r\n", d);
    statuslog(d->character->player.level, "%s deleted %sself (%s@%s).",
              GET_NAME(d->character),
              GET_SEX(d->character) ==
              SEX_MALE ? "him" : GET_SEX(d->character) ==
              SEX_FEMALE ? "her" : "it", d->login, d->host);
    logit(LOG_PLAYER, "%s deleted %sself (%s@%s).", GET_NAME(d->character),
          GET_SEX(d->character) == SEX_MALE ? "him" : "her", d->login,
          d->host);
    sql_log(d->character, PLAYERLOG, "Deleted self");
    deleteCharacter(d->character);
    STATE(d) = CON_FLUSH;
    break;
  }
}

void select_main_menu(P_desc d, char *arg)
{
  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  /* a little chicanery to force them to enter a valid password.  If they are in in CON_SLCT with a d->rtype
     greater than 20 (6 is normal max), they have to do the 'change password' thing.  JAB */

  if (d->rtype > 20)
  {
    SEND_TO_Q
      ("Your password has been expired.  Please enter your current password:",
       d);
    echo_off(d);
    strcpy(d->old_pwd, d->character->only.pc->pwd);
    STATE(d) = CON_PWDNEW;
    return;
  }
  switch (*arg)
  {
  case '0':                    /* logoff */
    close_socket(d);
    break;
  case '1':                    /* enter game */
    if( is_multiplaying(d) )
    {
      break;
    }
    enter_game(d);
    STATE(d) = CON_PLYNG;
    d->prompt_mode = TRUE;
    break;
  case '2':                    /* read background story */
    SEND_TO_Q(BACKGR_STORY, d);
    STATE(d) = CON_RMOTD;
    break;
  case '3':                    /* change password */
    SEND_TO_Q("Enter current password.", d);
    echo_off(d);
    strcpy(d->old_pwd, d->character->only.pc->pwd);
    STATE(d) = CON_PWDNEW;
    break;
  case '4':                    /* change long description */
    /* same deal here as with password, rather than adding complicated code
       to solve a minor problem, they must enter the game to save changes to
       their description.  Note that there is no 'case' for CON_EXDSCR, it
       is checked for, and STATE changed in string_add() in modify.c */
    SEND_TO_Q("\r\nEnter your new description.\r\n\r\n", d);
    SEND_TO_Q("(/s saves /h for help)\r\n", d);
    if (d->character->player.description)
    {
      SEND_TO_Q("Current description:\r\n", d);
      SEND_TO_Q(d->character->player.description, d);

      /* don't free this now... so that the old description gets loaded */
      /* as the current buffer in the editor */

      /* DO free it now, screw the abort buffer */

      FREE(d->character->player.description);
      d->character->player.description = NULL;
      /* BUT, do setup the ABORT buffer here */
/*      d->backstr = str_dup(d->character->player.description);*/
/*      FREE(d->character->player.description);
      d->character->player.description = NULL;*/
    }
    d->str = &d->character->player.description;
    d->max_str = 1024;
    STATE(d) = CON_EXDSCR;
    break;
  case '5':                    /* delete char */
    if (GET_LEVEL(d->character) > 40)
    {
      SEND_TO_Q("Nope, i'm 2 tired to restore you, soo you're not..\r\n", d);
      SEND_TO_Q(MENU, d);
      break;
    }
    SEND_TO_Q("Confirm deletion with your password.\r\n", d);
    STATE(d) = CON_PWDDCNF;
    break;
  default:
    SEND_TO_Q("Wrong option.\r\n", d);
    SEND_TO_Q(MENU, d);
    break;
  }
}

/*=========================================================================*/
/*
 *      Modifications, additions by SAM 7-94, to allow for rerolling,
 *      min char stats, bonus stats, and a few other thingies.
 *      Also separated each function out from nanny since they are relatively
 *      large and make nanny hard to read :)
 */
/*=========================================================================*/

/*
	Characters with the "newbie" flag will help imms see who's new and who needs special
	attention.
*/
void select_newbie(P_desc d, char *arg)
{
  for(; isspace(*arg); arg++) ;

  switch(*arg) {
    case 'Y':
    case 'y':
      SET_BIT(d->character->specials.act2, PLR2_NEWBIE);
      SEND_TO_Q("\r\nWelcome to Duris!\r\n", d);
      SEND_TO_Q(racewars, d);
      STATE(d) = CON_RACEWAR;
      break;

    case 'N':
    case 'n':
      SEND_TO_Q(racetable, d);
      STATE(d) = CON_QRACE;
      break;

    default:
      SEND_TO_Q("\r\nThat's not a valid option.\r\n", d);
      SEND_TO_Q("Are you new to the World of Duris, or a veteran player? (y/n) ", d);
      return;
   }

}

void select_hardcore(P_desc d, char *arg)
{
  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  switch (*arg)
  {
  case 'H':
  case 'h':
    SET_BIT(d->character->specials.act2, PLR2_HARDCORE_CHAR);
    SEND_TO_Q("HARDCORE!\r\n", d);
    break;
  case 'N':
  case 'n':
    break;
  case 'z':
  case 'Z':
    SEND_TO_Q(racetable, d);
    STATE(d) = CON_QRACE;
    return;

  default:
    SEND_TO_Q("That's not a valid option...\r\n", d);
    SEND_TO_Q("Please select either H (for Hardcore), N (for Normal) ", d);
    return;
  }
  display_classtable(d);
  STATE(d) = CON_QCLASS;

}
void select_sex(P_desc d, char *arg)
{
  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  switch (*arg)
  {
  case 'm':
  case 'M':
    d->character->player.sex = SEX_MALE;
    break;
  case 'f':
  case 'F':
    d->character->player.sex = SEX_FEMALE;
    break;
  case 'z':
  case 'Z':
    SEND_TO_Q(racetable, d);
    STATE(d) = CON_QRACE;
    return;

  default:
    SEND_TO_Q("That's not a valid option...\r\n", d);
    SEND_TO_Q
      ("Please select either F (for Female), M (for Male), or Z (for Race Selection): ",
       d);
    return;
  }


  if( !IS_NEWBIE(d->character) )
	{
	  SEND_TO_Q
	    ("\r\n\r\nDo you want to play hardcore? Hardcore char can only die 5 times, then it's gone for ever.\r\n",
	     d);
	  SEND_TO_Q
	    ("Only recommended for &+Yvery&n experience player who are looking for a new challange.\r\n",
	     d);
	  SEND_TO_Q("Please select either H (for Hardcore), N (for Normal)", d);
	  STATE(d) = CON_HARDCORE;
	}
  else {
	  display_classtable(d);
	  STATE(d) = CON_QCLASS;
	}
//re-enabling hardcore - drannak 
/*   display_classtable(d);
   STATE(d) = CON_QCLASS;*/
}



/* Krov: menu choice of race, the letters used for the menu are
   now connected to the race, to allow for easy addition/deletion */

void select_race(P_desc d, char *arg)
{
  char     Gbuf[MAX_INPUT_LENGTH];

  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  /*
   ** Since we have turned off echoing for telnet client,
   ** if a telnet client is indeed used, we need to skip the
   ** initial 5 bytes ( -1, -4, 1, 13, 0 ) if they are sent back by
   ** client program.
   */
  if (*arg == -1)
  {
    if ((arg[1] != '0') && (arg[2] != '0') && (arg[3] != '0') &&
        (arg[4] != '0'))
    {
      if (arg[5] == '0')
      {
        STATE(d) = CON_QRACE;
        return;
      }
      else
      {
        arg = arg + 5;
      }
    }
    else
    {
      close_socket(d);
    }
  }

  GET_RACE(d->character) = RACE_NONE;
  Gbuf[0] = 0;

  switch (*arg)
  {
  case 'h':
    GET_RACE(d->character) = RACE_HUMAN;
    break;
  case 'H':
    strcpy(Gbuf, "HUMAN");
    break;
  case 'b':
    GET_RACE(d->character) = RACE_BARBARIAN;
    break;
  case 'B':
    strcpy(Gbuf, "BARBARIAN");
    break;
  case 'd':
    GET_RACE(d->character) = RACE_DROW;
    break;
  case 'D':
    strcpy(Gbuf, "DROW ELF");
    break;
  case 'e':
    GET_RACE(d->character) = RACE_GREY;
    break;
  case 'E':
    strcpy(Gbuf, "GREY ELF");
    break;
  case 'm':
    GET_RACE(d->character) = RACE_MOUNTAIN;
    break;
  case 'M':
    strcpy(Gbuf, "MOUNTAIN DWARF");
    break;
  case 'u':
    GET_RACE(d->character) = RACE_DUERGAR;
    break;
  case 'U':
    strcpy(Gbuf, "DUERGAR DWARF");
    break;
  case 'l':
    GET_RACE(d->character) = RACE_HALFLING;
    break;
  case 'L':
    strcpy(Gbuf, "HALFLING");
    break;
  case 'g':
    GET_RACE(d->character) = RACE_GNOME;
    break;
  case 'G':
    strcpy(Gbuf, "GNOME");
    break;
  case 'r':
    GET_RACE(d->character) = RACE_OGRE;
    break;
  case 'R':
    strcpy(Gbuf, "OGRE");
    break;
  case 't':
    GET_RACE(d->character) = RACE_TROLL;
    break;
  case 'T':
    strcpy(Gbuf, "TROLL");
    break;
  /*
  case 'f':
    GET_RACE(d->character) = RACE_HALFELF;
    break;
  case 'F':
    strcpy(Gbuf, "HALF ELF");
    break;
    */
/*  
  case 'i':
    GET_RACE(d->character) = RACE_PILLITHID;
    break;
  case 'I':
    strcpy(Gbuf, "ILLITHID");
    break;
    */
    /*   case 'i':
       GET_RACE(d->character) = RACE_ILLITHID;
       break;
       case 'I':                                     REMOVED ILLITHIDS
       strcpy(Gbuf, "ILLITHID");
       break;
     */
  case 'j':
    GET_RACE(d->character) = RACE_GITHYANKI;
    break;
  case 'J':
    strcpy(Gbuf, "GITHYANKI");
    break;
  case 'o':
    GET_RACE(d->character) = RACE_ORC;
    break;
  case 'O':
    strcpy(Gbuf, "ORC");
    break;
  case 'k':
    GET_RACE(d->character) = RACE_THRIKREEN;
    break;
  case 'K':
    strcpy(Gbuf, "THRI-KREEN");
    break;

  case 'n':
    GET_RACE(d->character) = RACE_GITHZERAI;
    break;
  case 'N':
    strcpy(Gbuf, "GITHZERAI");
    break;

  case 'v':
    GET_RACE(d->character) = RACE_GOBLIN;
    break;
  case 'V':
    strcpy(Gbuf, "GOBLIN");
    break;
  case 'c':
    GET_RACE(d->character) = RACE_CENTAUR;
    break;
  case 'C':
    strcpy(Gbuf, "CENTAUR");
    break;
  case 's':
    GET_RACE(d->character) = RACE_MINOTAUR;
    break;
  case 'S':
    strcpy(Gbuf, "MINOTAUR");
    break;
  case 'p':
    GET_RACE(d->character) = RACE_FIRBOLG;
    break;
  case 'P':
    strcpy(Gbuf, "FIRBOLG");
    break;
/*
  case 'w':
    GET_RACE(d->character) = RACE_WOODELF;
    break;
  case 'W':
    strcpy(Gbuf, "WOOD ELF");
    break;
*/
  case '1':
    GET_RACE(d->character) = RACE_KOBOLD;
    break;
  case '!':
    strcpy(Gbuf, "KOBOLD");
    break;
  case '2':
    GET_RACE(d->character) = RACE_DRIDER;
    break;
  case '@':
    strcpy(Gbuf, "DRIDER");
    break;
/*  case '3':
    GET_RACE(d->character) = RACE_KUOTOA;
    break;
  case '#':
    strcpy(Gbuf, "KUO TOA");
    break;
    */
    /*
       case '1':
       GET_RACE(d->character) = RACE_LICH;
       break;
       case '(':
       strcpy(Gbuf, "LICH");
       break;
       case '2':
       GET_RACE(d->character) = RACE_PVAMPIRE;
       break;
       case '@':
       strcpy(Gbuf, "VAMPIRE");
       break;
       case '3':
       GET_RACE(d->character) = RACE_PDKNIGHT;
       break;
       case ')':
       strcpy(Gbuf, "DEATH KNIGHT");
       break;
       case '4':
       GET_RACE(d->character) = RACE_PSBEAST;
       break;
       case '$':
       strcpy(Gbuf, "SHADOW BEAST");
       break;

       case '5':
       GET_RACE(d->character) = RACE_WIGHT;
       break;
       case '%':
       strcpy(Gbuf, "WIGHT");
       break;
       case '6':
       GET_RACE(d->character) = RACE_PHANTOM;
       break;
       case '^':
       strcpy(Gbuf, "PHANTOM");
       break;
       case '7':
       GET_RACE(d->character) = RACE_SHADE;
       break;
       case '&':
       strcpy(Gbuf, "SHADE");
       break;
       case '8':
       GET_RACE(d->character) = RACE_REVENANT;
       break;
       case '*':
       strcpy(Gbuf, "REVENANT");
       break; */
  case 'x':
  case 'X':
    SEND_TO_Q(generaltable, d);
    STATE(d) = CON_INFO;
    return;
  case 'y':
  case 'Y':
    SEND_TO_Q(racewars, d);
    STATE(d) = CON_RACEWAR;
    return;
/*
  case 'z':
  case 'Z':
    SEND_TO_Q("\r\nIs your character Male or Female? (M/F) ", d);
    STATE(d) = CON_QSEX;
    return;
*/
  default:
    SEND_TO_Q(racetable, d);
    STATE(d) = CON_QRACE;
    return;
  }

  if (*Gbuf)
  {
    do_help(d->character, Gbuf, -4);
    return;
  }
  else if (GET_RACE(d->character) == RACE_NONE)
  {
    SEND_TO_Q("\r\n[Press Return or Enter to return to the Race Menu]", d);
    return;
  }
  /* Krov: select class is next */

  // not anymore, it's sex/class baby

  if (invitemode && EVIL_RACE(d->character) &&
      !is_invited(GET_NAME(d->character)))
  {
    SEND_TO_Q("\r\nSorry, but only those players that have been invited can roll evil characters.\r\n\r\n", d);

    // since STATE is not changed, player should be forced back into race selection

    GET_RACE(d->character) = RACE_NONE;
  }
  else if ((GET_RACE(d->character) != RACE_ILLITHID) &&
           (GET_RACE(d->character) != RACE_PILLITHID))
  {
    SEND_TO_Q("\r\nIs your character Male or Female (Z for race)? (M/F/Z) ", d);
    STATE(d) = CON_QSEX;
  }
  else
  {
    d->character->player.sex = SEX_NEUTRAL;

    display_classtable(d);
    STATE(d) = CON_QCLASS;
  }
}



/* Krov: select_class_info eaten up by select_class */


void select_reroll(P_desc d, char *arg)
{
  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  switch (*arg)
  {
  case 'N':
  case 'n':
    SEND_TO_Q("\r\n\r\nAccepting these stats.\r\n\r\n", d);
    STATE(d) = CON_BONUS1;
    //STATE(d) = CON_REROLL;
    break;
  default:
    SEND_TO_Q("\r\n\r\nRerolling this character.\r\n\r\n", d);
    roll_basic_attributes(d->character, ROLL_NORMAL);
    display_stats(d);
    SEND_TO_Q(reroll, d);
    SEND_TO_Q("Do you want to reroll this char (y/n) [y]:  ", d);
    STATE(d) = CON_REROLL;
    break;
  }

  if (STATE(d) == CON_BONUS1)
  {
    display_stats(d);
    SEND_TO_Q(bonus, d);
    SEND_TO_Q("\r\n\r\nEnter stat for first bonus:  ", d);
  }
}

/* Krov: BONUS3 now connects to KEEPCHAR */
void select_bonus(P_desc d, char *arg)
{
  int      i = 0;

  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  switch (LOWER(*arg))
  {
  case 's':
  case 'S':
    i = 1;
    break;
  case 'd':
  case 'D':
    i = 2;
    break;
  case 'a':
  case 'A':
    i = 3;
    break;
  case 'c':
  case 'C':
    i = 4;
    break;
  case 'p':
  case 'P':
    i = 5;
    break;
  case 'i':
  case 'I':
    i = 6;
    break;
  case 'w':
  case 'W':
    i = 7;
    break;
  case 'h':
  case 'H':
    i = 8;
    break;
  case 'l':
  case 'L':
    i = 9;
    break;
  case '?':
  default:
    display_stats(d);
    SEND_TO_Q(bonus, d);
    switch (STATE(d))
    {
    case CON_BONUS1:
      SEND_TO_Q("\r\nEnter stat for first bonus:  ", d);
      break;
    case CON_BONUS2:
      SEND_TO_Q("\r\nEnter stat for second bonus:  ", d);
      break;
    case CON_BONUS3:
      SEND_TO_Q("\r\nEnter stat for third bonus:  ", d);
      break;
    case CON_BONUS4:
      SEND_TO_Q("\r\nEnter stat for fourth bonus:  ", d);
      break;
    case CON_BONUS5:
      SEND_TO_Q("\r\nEnter stat for fifth bonus:  ", d);
      break;
    }
    return;
    break;
  }

  if (!i)
  {
    SEND_TO_Q("\r\nIllegal input.\r\n", d);
    SEND_TO_Q("Enter desired bonus stat, or '?' to see explanation again:  ",
              d);
    return;
  }
  /* Krov: this now adds randomly 5/10/15 points */
  add_stat_bonus(d->character, i, 5);

  if (STATE(d) == CON_BONUS5)
  {
    display_characteristics(d);
    display_stats(d);
//    SEND_TO_Q(keepchar, d);
    SEND_TO_Q("Do you want to swap stats (Y/N): ", d);
    STATE(d) = CON_SWAPSTATYN;
    return;
  }
  display_stats(d);
  SEND_TO_Q(bonus, d);
  switch (STATE(d))
  {
  case CON_BONUS1:
    SEND_TO_Q("\r\nEnter stat category for second bonus:  ", d);
    STATE(d) = CON_BONUS2;
    break;
  case CON_BONUS2:
    SEND_TO_Q("\r\nEnter stat category for third bonus:  ", d);
    STATE(d) = CON_BONUS3;
    break;
  case CON_BONUS3:
    SEND_TO_Q("\r\nEnter stat category for fourth bonus:  ", d);
    STATE(d) = CON_BONUS5;
    break;
  case CON_BONUS4:
    SEND_TO_Q("\r\nEnter stat category for fifth bonus:  ", d);
    STATE(d) = CON_BONUS5;
    break;
  }
}




/* Krov: show_avail_class, has_avail_class, and display_avail_class
   are gone for good */

/* Krov: select_class is now a simple menu choice.
   Letter to press for class now depends on name of class, making
   it easy to add/delete classes without disturbing alphabetic order.
   Help is now added by Big letters. */
void select_class(P_desc d, char *arg)
{
  int      home, cls;

  char     Gbuf[MAX_INPUT_LENGTH];

  Gbuf[0] = 0;

  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  d->character->player.m_class = CLASS_NONE;

  for( cls = 1; cls <= CLASS_COUNT; cls++ )
  {
    if( *arg == class_names_table[cls].letter )
      d->character->player.m_class = 1 << (cls - 1);
    else if( tolower(*arg) == class_names_table[cls].letter )
      strcpy(Gbuf, class_names_table[cls].normal);
    // Had to hardcode the # for Summoners (option 3).
    else if( *arg == '#' )
    {
      strcpy(Gbuf, class_names_table[29].normal);
    }
    else if( tolower(*arg) == 'z' )
    {
      SEND_TO_Q("\r\nIs your character Male or Female (Z for race)? (M/F/Z) ", d);
      STATE(d) = CON_QSEX;
    }
    else
      continue;
    break;
  }

  if( cls > CLASS_COUNT )
  {
    display_classtable(d);
    STATE(d) = CON_QCLASS;
    return;
  }

  /* Krov: help */
  if( *Gbuf )
  {
    do_help(d->character, Gbuf, -4);
    return;
  }
  else if (d->character->player.m_class == CLASS_NONE)
  {
    SEND_TO_Q("\r\n[Press Return or Enter to return to the Class Menu]", d);
    return;
  }
  if (class_table[GET_RACE(d->character)]
      [flag2idx(d->character->player.m_class)] == 5)
  {
    SEND_TO_Q("\r\nThis is not an allowed class for your race!", d);
    return;
  }
  /* Krov: We do alignment/hometown choice after class now, _then_
     we roll stats depending on the choices made. */

  /* alignment, table gives one of these:
   *     -1 = evil (-1000)
   *      0 = neutral (0)
   *      1 = good (+1000)
   *      2 = choice (any)
   *      3 = choice (good/neutral)
   *      4 = choice (neutral/evil)
   */

  switch (find_starting_alignment
          (GET_RACE(d->character), d->character->player.m_class))
  {
  case -1:
    GET_ALIGNMENT(d->character) = -1000;
    break;
  case 0:
    GET_ALIGNMENT(d->character) = 0;
    break;
  case 1:
    GET_ALIGNMENT(d->character) = 1000;
    break;
  default:
    STATE(d) = CON_ALIGN;
    SEND_TO_Q("\r\n\r\n", d);
    SEND_TO_Q(alignment_table, d);
    if (class_table[(int) GET_RACE(d->character)]
        [flag2idx(d->character->player.m_class)] != 4)
      SEND_TO_Q("G)ood\r\n", d);
    SEND_TO_Q("N)eutral\r\n", d);
/*    if (!invitemode && (class_table[(int) GET_RACE(d->character)][flag2idx(d->character->player.m_class)] != 3) &&
        (!RACE_NEUTRAL(d->character) || is_invited(GET_NAME(d->character))))*/
    if (class_table[(int) GET_RACE(d->character)]
        [flag2idx(d->character->player.m_class)] != 3)
      SEND_TO_Q("E)vil\r\n", d);
    SEND_TO_Q("Alignment only affects your character's alignment and not the chosen racewar side.\n", d);
    SEND_TO_Q("\r\nYour selection: ", d);
    return;
    break;
  }

  if (OLD_RACE_GOOD(GET_RACE(d->character), GET_ALIGNMENT(d->character)))
    GET_RACEWAR(d->character) = RACEWAR_GOOD;
  else if (OLD_RACE_EVIL(GET_RACE(d->character), GET_ALIGNMENT(d->character)))
    GET_RACEWAR(d->character) = RACEWAR_EVIL;
  else if (OLD_RACE_PUNDEAD(GET_RACE(d->character)))
    GET_RACEWAR(d->character) = RACEWAR_UNDEAD;
  else if (IS_HARPY(d->character))
    GET_RACEWAR(d->character) = RACEWAR_NEUTRAL;

  /* pass through here, they don't get an alignment d->characterchoice. */

  home = find_hometown(GET_RACE(d->character), false);

  if (home == HOME_CHOICE)
  {
    STATE(d) = CON_HOMETOWN;
    SEND_TO_Q("\r\n\r\n", d);
    SEND_TO_Q(hometown_table, d);
    show_avail_hometowns(d);
    SEND_TO_Q("\r\nYour selection: ", d);
    return;
  }
  GET_HOME(d->character) = home;
  GET_BIRTHPLACE(d->character) = home;
  GET_ORIG_BIRTHPLACE(d->character) = home;

  /* Krov: didn't get hometown choice either, roll the stats */
  //STATE(d) = CON_BONUS1;
  STATE(d) = CON_REROLL;
  roll_basic_attributes(d->character, ROLL_NORMAL);
  display_characteristics(d);

    display_stats(d);//
  SEND_TO_Q(reroll, d);//
  SEND_TO_Q("\r\nPress return to continue with adding stat bonuses.\r\n", d);
}


/* Krov: this procedure displays the classtable according to race */

void display_classtable(P_desc d)
{
  char     template_buf[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int      cls;

  SEND_TO_Q("\r\nClass Selection", d);
  SEND_TO_Q("\r\n---------------", d);

  buf[0] = 0;
  for (cls = 1; cls <= CLASS_COUNT; cls++)
    if (class_table[GET_RACE(d->character)][cls] != 5)
    {
      sprintf(template_buf, "\r\n%%c) %%-%ds(%%c for help)",
              strlen(class_names_table[cls].ansi) -
              ansi_strlen(class_names_table[cls].ansi) + 20);
      sprintf(buf + strlen(buf), template_buf, class_names_table[cls].letter,
              class_names_table[cls].ansi,
              (class_names_table[cls].letter == '3') ? '#' : toupper(class_names_table[cls].letter));
    }

  strcat(buf, "\r\n");
  SEND_TO_Q(buf, d);

  if (GET_RACE(d->character) == RACE_ILLITHID)
    SEND_TO_Q("\r\nz) Return to previous menu (selecting your race).", d);
  else
    SEND_TO_Q("\r\nz) Return to previous prompt (selecting your sex).", d);

  SEND_TO_Q("\r\n" "\r\nYour selection: ", d);
}




/* Krov: ALIGN connects now to HOME or REROLL */

void select_alignment(P_desc d, char *arg)
{
  int      align = 0, home, err = 0;

  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  switch (*arg)
  {
  case 'G':
  case 'g':
    if (class_table[(int) GET_RACE(d->character)]
        [flag2idx(d->character->player.m_class)] == 4)
      err = 1;
    else
      align = 1000;             /* good */
    break;
  case 'N':
  case 'n':
    align = 0;                  /* neutral */
    break;
  case 'E':
  case 'e':
    if (class_table[(int) GET_RACE(d->character)]
        [flag2idx(d->character->player.m_class)] == 3)
      err = 1;
    else
      align = -1000;
    break;
  default:
    err = 1;
    break;
  }

  if (err)
  {
    SEND_TO_Q
      ("\r\nThat is not a valid alignment\r\nPlease choose an alignment: ",
       d);
    STATE(d) = CON_ALIGN;
    return;
  }
  /* record it */
  GET_ALIGNMENT(d->character) = align;

  if (OLD_RACE_GOOD(GET_RACE(d->character), GET_ALIGNMENT(d->character)))
    GET_RACEWAR(d->character) = RACEWAR_GOOD;
  else if (OLD_RACE_EVIL(GET_RACE(d->character), GET_ALIGNMENT(d->character)))
    GET_RACEWAR(d->character) = RACEWAR_EVIL;
  else if (OLD_RACE_PUNDEAD(GET_RACE(d->character)))
    GET_RACEWAR(d->character) = RACEWAR_UNDEAD;
  else if (IS_HARPY(d->character))
    GET_RACEWAR(d->character) = RACEWAR_NEUTRAL;

  /* does this race get to choose a hometown ? */
  home = find_hometown(GET_RACE(d->character), false);
  if (home == HOME_CHOICE)
  {
    STATE(d) = CON_HOMETOWN;
    SEND_TO_Q("\r\n\r\n", d);
    SEND_TO_Q(hometown_table, d);
    show_avail_hometowns(d);
    SEND_TO_Q("\r\nYour selection: ", d);
    return;
  }
  GET_HOME(d->character) = home;
  GET_BIRTHPLACE(d->character) = home;
  GET_ORIG_BIRTHPLACE(d->character) = home;

    STATE(d) = CON_REROLL;
  roll_basic_attributes(d->character, ROLL_NORMAL);
  display_characteristics(d);
  display_stats(d);
  SEND_TO_Q(reroll, d);
  SEND_TO_Q("Do you want to reroll this char (y/n) [y]:  ", d);
}


/* Krov: HOMETOWN connects now to REROLL */

void select_hometown(P_desc d, char *arg)
{
  int      home;

  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  home = -1;
  for (int i = 0; i <= LAST_HOME; i++)
  {
    char town_letter = LOWER(town_name_list[i][0]);
    // if (i == HOME_SHADY)
      // town_letter = 'a';
    // else if (i == HOME_GOBLIN)
      // town_letter = 'g';
    // else if (i == HOME_SYLVANDAWN)
      // town_letter = 's';

    if ((avail_hometowns[i][GET_RACE(d->character)] == 1) &&
        (LOWER(*arg) == LOWER(town_name_list[i][0])))
    {
      home = i;
      break;
    }
  }
  if (-1 == home)
  {
    SEND_TO_Q
      ("\r\nThat is not a valid hometown\r\nPlease choose a real hometown: ",
       d);
    STATE(d) = CON_HOMETOWN;
    return;
  }

  /* did they select one that is allowed for their race */
  if (avail_hometowns[home][(int) GET_RACE(d->character)] != 1)
  {
    SEND_TO_Q("\r\nThat is not a hometown for your race.\r\n ", d);
    SEND_TO_Q("Please select again.\r\n\r\nHometown: ", d);
    STATE(d) = CON_HOMETOWN;
    return;
  }
  /* record it, move onto the next step */
  GET_HOME(d->character) = home;
  GET_BIRTHPLACE(d->character) = home;
  GET_ORIG_BIRTHPLACE(d->character) = home;

  STATE(d) = CON_REROLL;
  roll_basic_attributes(d->character, ROLL_NORMAL);
  display_characteristics(d);
  display_stats(d);
  SEND_TO_Q(reroll, d);
  SEND_TO_Q("Do you want to reroll this char (y/n) [y]:  ", d);
}

void select_keepchar(P_desc d, char *arg)
{
  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;
  switch (LOWER(*arg))
  {
  case 'n':
    SEND_TO_Q("\r\n\r\nDeleting this character.r\n", d);
    STATE(d) = CON_NME;
    if (d->term_type == TERM_GENERIC)
      SEND_TO_Q(GREETINGS, d);
    else
      SEND_TO_Q(greetinga, d);
    SEND_TO_Q("\r\nBy what name do you wish to be known? ", d);
    break;
  case 'q':
    SEND_TO_Q("\r\n\r\nCome back again real soon.\r\n", d);
    close_socket(d);
    break;
  case 'y':
  default:
    SEND_TO_Q("\r\n\r\nWelcome to Duris, Land of Bloodlust!\r\n\r\n", d);
    STATE(d) = CON_RMOTD;
    break;
  }
}

void display_stats(P_desc d)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  strcpy(Gbuf1, "\r\nYour basic stats:\r\n");

  sprintf(Gbuf1 + strlen(Gbuf1),
          "Strength:     %15s      Power:        %s\r\n",
          stat_to_string2((int) d->character->base_stats.Str),
          stat_to_string2((int) d->character->base_stats.Pow));

  sprintf(Gbuf1 + strlen(Gbuf1),
          "Dexterity:    %15s      Intelligence: %s\r\n",
          stat_to_string2((int) d->character->base_stats.Dex),
          stat_to_string2((int) d->character->base_stats.Int));

  sprintf(Gbuf1 + strlen(Gbuf1),
          "Agility:      %15s      Wisdom:       %s\r\n",
          stat_to_string2((int) d->character->base_stats.Agi),
          stat_to_string2((int) d->character->base_stats.Wis));

  sprintf(Gbuf1 + strlen(Gbuf1),
          "Constitution: %15s      Charisma:     %s\r\n\r\n",
          stat_to_string2((int) d->character->base_stats.Con),
          stat_to_string2((int) d->character->base_stats.Cha));

  sprintf(Gbuf1 + strlen(Gbuf1), "Luck: %15s      Unused:     %s\r\n\r\n",
          stat_to_string2((int) d->character->base_stats.Luk),
          stat_to_string2((int) d->character->base_stats.Kar));

  SEND_TO_Q(Gbuf1, d);
}


void display_characteristics(P_desc d)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  char     buffer[MAX_STRING_LENGTH];

  sprintf(Gbuf1,
          "\r\n\r\n---------------------------------------\r\nNAME:   %s\r\n",
          GET_NAME(d->character));

  if (d->character->player.sex == SEX_MALE)
    strcat(Gbuf1, "SEX:      Male\r\n");
  else
    strcat(Gbuf1, "SEX:      Female\r\n");

  /*
  sprintf(Gbuf1 + strlen(Gbuf1), "Your short description is...%s\r\n",
          d->character->player.short_descr);
  */
  
  sprintf(Gbuf1 + strlen(Gbuf1), "RACE:     %s\r\n",
          race_to_string(d->character));
  sprintf(Gbuf1 + strlen(Gbuf1), "CLASS:    %s\r\n",
          get_class_string(d->character, buffer));

  if (GET_ALIGNMENT(d->character) == 1000)
    strcat(Gbuf1, "ALIGN:    Good\r\n");
  else if (GET_ALIGNMENT(d->character) == -1000)
    strcat(Gbuf1, "ALIGN:    Evil\r\n");
  else
  {
    if (GET_ALIGNMENT(d->character) != 0)
    {
      logit(LOG_STATUS, "display_characteristics: unknown alignment, %d\n",
            GET_ALIGNMENT(d->character));
      GET_ALIGNMENT(d->character) = 0;
    }
    strcat(Gbuf1, "ALIGNMENT:    Neutral\r\n");
  }

  if (GET_HOME(d->character) > 0)
  {
    sprintf(Gbuf1 + strlen(Gbuf1), "HOMETOWN: %s\r\n",
            town_name_list[GET_HOME(d->character)]);    
  }
  else
  {
    logit(LOG_STATUS, "display_characteristics: unknown hometown, %d\n",
          GET_HOME(d->character));
    GET_HOME(d->character) = HOME_THARN;
    sprintf(Gbuf1 + strlen(Gbuf1), "HOMETOWN: %s\r\n",
            town_name_list[GET_HOME(d->character)]);
  }
  
  sprintf(Gbuf1 + strlen(Gbuf1), "\nPossible specializations:\n");
  
  if( !append_valid_specs(Gbuf1, d->character) )
  {
    sprintf(Gbuf1 + strlen(Gbuf1), "None\n");    
  }
    
  /*
  sprintf(Gbuf1 + strlen(Gbuf1), "HARDCORE: ");
  if (IS_HARDCORE(d->character))
    sprintf(Gbuf1 + strlen(Gbuf1), "YES\r\n");
  else
    sprintf(Gbuf1 + strlen(Gbuf1), "NO\r\n");

*/
  SEND_TO_Q(Gbuf1, d);

}



/* Krov: this adds now 5/10/15 depending on what 1..3 to stat which */

void add_stat_bonus(P_char ch, int which, int what)
{
  int      tmp;

  tmp = what;

  switch (which)
  {
  case 1:
    ch->base_stats.Str = BOUNDED(1, ch->base_stats.Str + tmp, 100);
    break;
  case 2:
    ch->base_stats.Dex = BOUNDED(1, ch->base_stats.Dex + tmp, 100);
    break;
  case 3:
    ch->base_stats.Agi = BOUNDED(1, ch->base_stats.Agi + tmp, 100);
    break;
  case 4:
    ch->base_stats.Con = BOUNDED(1, ch->base_stats.Con + tmp, 100);
    break;
  case 5:
    ch->base_stats.Pow = BOUNDED(1, ch->base_stats.Pow + tmp, 100);
    break;
  case 6:
    ch->base_stats.Int = BOUNDED(1, ch->base_stats.Int + tmp, 100);
    break;
  case 7:
    ch->base_stats.Wis = BOUNDED(1, ch->base_stats.Wis + tmp, 100);
    break;
  case 8:
    ch->base_stats.Cha = BOUNDED(1, ch->base_stats.Cha + tmp, 100);
    break;
  case 9:
    ch->base_stats.Luk = BOUNDED(1, ch->base_stats.Luk + tmp, 100);
    break;
  }
  ch->curr_stats = ch->base_stats;
}


/* Krov: char_quals_for_class - gone for good */


void show_avail_hometowns(P_desc d)
{
  int      i, race;
  char     Gbuf1[MAX_STRING_LENGTH];

  race = GET_RACE(d->character);

  for (i = 0; i <= LAST_HOME; i++)
  {
    if (avail_hometowns[i][race] == 1)
    {
      // if (i == HOME_SHADY)
      // {
        // strcpy(Gbuf1, "S) Shady\r\n");
        // SEND_TO_Q(Gbuf1, d);
      // }
      // else if (i == HOME_GOBLIN)
      // {
        // strcpy(Gbuf1, "G) Moregeeth\r\n");
        // SEND_TO_Q(Gbuf1, d);
      // }
      // else
      // {
        sprintf(Gbuf1, "%c)%s\r\n", town_name_list[i][0],
                &town_name_list[i][1]);
        SEND_TO_Q(Gbuf1, d);
      // }
    }
  }
}

/* this function returns the NUMBER OF THE HOMETOWN, not the virtual number of
   the room they start at [that's done later] */

int find_hometown(int race, bool force)
{
  int      i, count = 0, home = 0;
  char     Gbuf1[MAX_STRING_LENGTH];

  if ((race < 1) || (race > LAST_RACE))
  {
    sprintf(Gbuf1, "find_hometown: illegal race, %d\n", race);
    logit(LOG_STATUS, Gbuf1);
    return (HOME_THARN);    /* default */
  }
  for (i = 0; i <= LAST_HOME; i++)
  {
    if (avail_hometowns[i][race] == 1)
    {
      if (home == 0)
        home = i;
      count++;
    }
  }
  if (count == 0)
  {                             /* none found, avail_hometowns matrix fucked */
    sprintf(Gbuf1, "find_hometown: race %d has no avail hometowns\n", race);
    logit(LOG_STATUS, Gbuf1);
    return (HOME_THARN);    /* default */
  }
  else if (count == 1 || force)          /* what we expect, 1 town, return it */
    return (home);

  else                          /* multiple hometows avail, let player choose */
    return (HOME_CHOICE);
}

void find_starting_location(P_char ch, int hometown)
{
  int      guild_num;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (hometown == 0)
  {
    hometown = find_hometown(GET_RACE(ch), true);
    if (hometown == HOME_CHOICE)
      hometown = 0;
  }
  if ((hometown < 1) || (hometown > LAST_HOME))
  {
    sprintf(Gbuf1, "find_starting_location: illegal hometown %d for %s",
            hometown, GET_NAME(ch));
    logit(LOG_DEBUG, Gbuf1);
    GET_HOME(ch) = guild_locations[HOME_THARN][0];  /* default */
    return;
  }
  if ((ch->player.m_class < 1) ||
      (ch->player.m_class > (1 << (CLASS_COUNT - 1))))
  {
    sprintf(Gbuf1, "find_starting_location: illegal class %d for %s",
            ch->player.m_class, GET_NAME(ch));
    logit(LOG_DEBUG, Gbuf1);
    GET_HOME(ch) = guild_locations[HOME_THARN][0];  /* default */
    return;
  }
  guild_num = guild_locations[hometown][flag2idx(ch->player.m_class)];

  if (guild_num == -1)
  {
    sprintf(Gbuf1,
            "find_starting_location: hometown %d, no guild for class %d (%s)",
            hometown, ch->player.m_class, GET_NAME(ch));
    logit(LOG_DEBUG, Gbuf1);
    GET_HOME(ch) = guild_locations[hometown][0];
    return;
  }
  GET_HOME(ch) = guild_num;
}

int find_starting_alignment(int race, int m_class)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  if ((race < 1) || (race > LAST_RACE))
  {
    sprintf(Gbuf1, "find_starting_alignment: illegal race, %d\n", race);
    logit(LOG_STATUS, Gbuf1);
    return (0);                 /* default */
  }
  if ((m_class < 1) || (m_class > (1 << (CLASS_COUNT - 1))))
  {
    sprintf(Gbuf1, "find_starting_alignment: illegal class, %d\n", m_class);
    logit(LOG_STATUS, Gbuf1);
    return (0);                 /* default */
  }
  return (class_table[race][flag2idx(m_class)]);
}

/* sets initial values for player law_flags, based on race and class */

ulong init_law_flags(P_char ch)
{
  ulong    flags = 0;

  return flags;
}

/* set char's height and weight, based mainly on race and sex, but high/low
   CON is a factor, and all variables are bell-curved, so things will tend
   towards the average range.  Not perfect, but beats the snot out of plain
   random range, and all races the same size. JAB */

void set_char_height_weight(P_char ch)
{
  int      mean_h = 0, mean_w = 0, range_h = 0, max_under_w = 0, max_over_w =
  0, female = 100;
  int      h_roll, w_roll, tmp;

  switch (GET_RACE(ch))
  {
    case RACE_SGIANT:
    case RACE_OGRE:
    case RACE_WIGHT:
      mean_h = 90;
      range_h = 10;
      mean_w = 400;
      max_under_w = 80;
      max_over_w = 150;
      break;
    case RACE_TROLL:
      mean_h = 95;
      range_h = 10;
      mean_w = 220;
      max_under_w = 90;
      max_over_w = 115;
      break;
    case RACE_MOUNTAIN:
    case RACE_DUERGAR:
      mean_h = 48;
      range_h = 8;
      mean_w = 175;
      max_under_w = 80;
      max_over_w = 125;
      if (GET_SEX(ch) == SEX_FEMALE)
        female = 95;
        break;
    case RACE_BARBARIAN:
    case RACE_FIRBOLG:
    case RACE_MINOTAUR:
    case RACE_PDKNIGHT:
    case RACE_PSBEAST:
    case RACE_THRIKREEN:
    case RACE_REVENANT:
    case RACE_OROG:
      mean_h = 76;
      range_h = 18;
      mean_w = 210;
      max_under_w = 70;
      max_over_w = 150;
      if (GET_SEX(ch) == SEX_FEMALE)
        female = 85;
        break;
    case RACE_PVAMPIRE:
    case RACE_HUMAN:
    case RACE_GITHYANKI:
    case RACE_ORC:
    case RACE_PHANTOM:
    case RACE_GITHZERAI:
    case RACE_KUOTOA:
      mean_h = 68;
      mean_w = 150;
      range_h = 24;
      max_under_w = 65;
      max_over_w = 180;
      if (GET_SEX(ch) == SEX_FEMALE)
        female = 85;
        break;
    case RACE_HALFLING:
      mean_h = 38;
      range_h = 6;
      mean_w = 55;
      max_under_w = 85;
      max_over_w = 150;
      if (GET_SEX(ch) == SEX_FEMALE)
        female = 95;
        break;
    case RACE_GNOME:
    case RACE_GOBLIN:
    case RACE_SHADE:
    case RACE_KOBOLD:
      mean_h = 42;
      range_h = 6;
      mean_w = 55;
      max_under_w = 75;
      max_over_w = 120;
      if (GET_SEX(ch) == SEX_FEMALE)
        female = 95;
        break;
    case RACE_LICH:
    case RACE_DROW:
    case RACE_GREY:
    case RACE_WOODELF:
    case RACE_HARPY:
    case RACE_GARGOYLE:
    case RACE_ILLITHID:
    case RACE_PILLITHID:
    case RACE_DRIDER:
      mean_h = 70;
      range_h = 8;
      mean_w = 125;
      max_under_w = 90;
      max_over_w = 115;
      if (GET_SEX(ch) == SEX_FEMALE)
        female = 95;
        break;
    case RACE_HALFELF:
      mean_h = 69;
      range_h = 16;
      mean_w = 145;
      max_under_w = 80;
      max_over_w = 145;
      if (GET_SEX(ch) == SEX_FEMALE)
        female = 90;
        break;
    case RACE_CENTAUR:
      mean_h = 68;
      mean_w = 650;
      range_h = 24;
      max_under_w = 65;
      max_over_w = 100;
      if (GET_SEX(ch) == SEX_FEMALE)
        female = 90;
        break;
  }

  /* ok, averages, normal ranges and relative size of females has been set,
    now we do the math. */

  h_roll = dice(4, 51) - 104;   /* bell curved -100 to 100 */

  /* we find height first, since that is more or less fixed in adults,
    high or low con will alter the normal limits (slightly).  Lots of math
    but this is only done when a character is created. */

  h_roll += number(5, 13);

  /* h_roll ranges (by race)
    elf, duegar                                   -110 - 105
    barbarian, ogre, troll, thrikreen, minotaur   -110 - 110
    dwarf, orc                                    -110 - 115
    human, halfling, halfelf, githyanki           -110 - 120
    drow, gnome, goblin                           -120 - 100
    */

  ch->player.height =
    (int) (((mean_h * female) + ((range_h * female * h_roll) / 200)) / 100);

  /* tmp will be in the range 676 - 1191, calc spread out to preserve max
    variance with int math truncation, max sub-value is only 12 million, so
    no worries about overflows. */

  tmp = (int) (((ch->player.height * 1000 * female) /
                (mean_h * female / 10)) / 10);

  /* now weight, this roll is relative to actual height and represents
    over/underweight. */

  /* bell curved over/underweight % */
  w_roll =
    max_under_w +
    (((dice(3, 34) - 3) * (max_over_w - max_under_w + 1)) / 100);

  ch->player.weight = (int) ((mean_w * tmp * w_roll) / 100000);
}

void set_char_size(P_char ch)
{
  switch (GET_RACE(ch))
  {
  case RACE_NONE:
  case RACE_HUMAN:
  case RACE_DROW:
  case RACE_GREY:
  case RACE_GARGOYLE:
  case RACE_HALFELF:
  case RACE_ILLITHID:
  case RACE_PILLITHID:
  case RACE_THRIKREEN:
  case RACE_ORC:
  case RACE_MOUNTAIN:
  case RACE_GITHYANKI:
  case RACE_LICH:
  case RACE_PVAMPIRE:
  case RACE_PHANTOM:
  case RACE_PSBEAST:
  case RACE_DUERGAR:
  case RACE_GITHZERAI:
  case RACE_OROG:
  case RACE_WOODELF:
  case RACE_KUOTOA:
    GET_SIZE(ch) = SIZE_MEDIUM;
    break;
  case RACE_HARPY:
  case RACE_HALFLING:
  case RACE_GOBLIN:
  case RACE_SHADE:
  case RACE_GNOME:
  case RACE_KOBOLD:
    GET_SIZE(ch) = SIZE_SMALL;
    break;
  case RACE_FAERIE:
    GET_SIZE(ch) = SIZE_TINY;
    break;
  case RACE_TROLL:
  case RACE_PDKNIGHT:
  case RACE_CENTAUR:
  case RACE_REVENANT:
  case RACE_BARBARIAN:
  case RACE_DRIDER:
    GET_SIZE(ch) = SIZE_LARGE;
    break;
  case RACE_SGIANT:
  case RACE_WIGHT:
  case RACE_OGRE:
  case RACE_MINOTAUR:
  case RACE_FIRBOLG:
    GET_SIZE(ch) = SIZE_HUGE;
  }
}

/*
 *    moved from db.c: initialize new character, assume
 *      race, class, hometown, and align are SET
 */
void init_char(P_char ch)
{
  int      i;

  clear_title(ch);

  ch->only.pc->pid = getNewPCidNumb();
  ch->only.pc->screen_length = 24;      /* default */
  ch->only.pc->wiz_invis = 0;
  ch->only.pc->law_flags = 0;
  ch->only.pc->highest_level = 1;
  ch->player.short_descr = 0;
  ch->player.long_descr = 0;
  ch->player.description = 0;
  ch->player.time.birth = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
  ch->only.pc->prestige = 0;    /* clear this early, so we can use as newby timer */
  ch->only.pc->nb_left_guild = 0;
  ch->only.pc->time_left_guild = 0;
  ch->player.secondary_level = 0;
  for (i = 0; i < MAX_TONGUE; i++)
    GET_LANGUAGE(ch, i) = 0;

  /* set location within hometown SAM 7-94 */
  find_starting_location(ch, GET_HOME(ch));
  GET_BIRTHPLACE(ch) = GET_HOME(ch);
  GET_ORIG_BIRTHPLACE(ch) = GET_HOME(ch);

  ch->points.mana = GET_MAX_MANA(ch);
  ch->points.hit = GET_MAX_HIT(ch);
  ch->points.vitality = GET_MAX_VITALITY(ch);
  ch->points.base_armor = 0;
  for (i = 0; i < MAX_SKILLS; i++)
  {
    if (GET_LEVEL(ch) < MINLVLIMMORTAL)
    {
      ch->only.pc->skills[i].learned = 0;
    }
    else
    {
      ch->only.pc->skills[i].learned = 100;
    }
  }
  NewbySkillSet(ch, TRUE);

  set_char_height_weight(ch); /* height and weight */
  set_char_size(ch);

  ch->only.pc->law_flags = init_law_flags(ch);  /* starting law flags */
  ch->specials.affected_by = 0;
  ch->specials.affected_by2 = 0;
  ch->specials.affected_by3 = 0;
  ch->specials.affected_by4 = 0;
  ch->specials.affected_by5 = 0;
  /* ok, some innate powers just set bits, so we need to reset those */

  if (has_innate(ch, INNATE_WATERBREATH))
    SET_BIT(ch->specials.affected_by, AFF_WATERBREATH);
  if (has_innate(ch, INNATE_INFRAVISION))
    SET_BIT(ch->specials.affected_by, AFF_INFRAVISION);
  if (has_innate(ch, INNATE_FLY))
    SET_BIT(ch->specials.affected_by, AFF_FLY);
  if (has_innate(ch, INNATE_HASTE))
    SET_BIT(ch->specials.affected_by, AFF_HASTE);
  if (has_innate(ch, INNATE_FARSEE))
    SET_BIT(ch->specials.affected_by, AFF_FARSEE);
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
  }
  if (has_innate(ch, INNATE_PROT_FIRE))
    SET_BIT(ch->specials.affected_by, AFF_PROT_FIRE);
  if (has_innate(ch, INNATE_VAMPIRIC_TOUCH))
    SET_BIT(ch->specials.affected_by2, AFF2_VAMPIRIC_TOUCH);
  if (has_innate(ch, INNATE_DAUNTLESS))
    SET_BIT(ch->specials.affected_by4, AFF4_NOFEAR);


#if 0
  ch->specials.shadow.shadowing = NULL;
  ch->specials.shadow.who = NULL;
  ch->specials.shadow.valid_last_move = FALSE;
  ch->specials.shadow.shadow_move = FALSE;
#endif
  for (i = 0; i < 5; i++)
    ch->specials.apply_saving_throw[i] = 0;

  for (i = 0; i < MAX_COND; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == MAXLVL ? -1 : MAXLVL);

  ch->only.pc->poofIn = 0;
  ch->only.pc->poofOut = 0;
  ch->only.pc->skillpoints = 0;
}

int      accept_mode = 0;       /* whether to have need to accept new players or not */

void newby_announce(P_desc d)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  P_desc   i;

  sprintf(Gbuf1,
          "&+C*** New char: &n&+c'&+C%s&n&+c' (&n%s %s %s&n&+c %s@&+W%s&n&+c) awaiting acceptance.\r\n",
          GET_NAME(d->character),
          (GET_SEX(d->character) ==
           SEX_MALE) ? "Male" : (GET_SEX(d->character) ==
                                 SEX_FEMALE) ? "Female" : "Neuter",
          race_names_table[(int) GET_RACE(d->character)].ansi,
          class_names_table[flag2idx(d->character->player.m_class)].ansi,
          d->login ? d->login : "unknown", d->host ? d->host : "UNKNOWN");
  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character &&
        IS_SET(i->character->specials.act, PLR_NAMES) &&
        IS_TRUSTED(i->character))
      send_to_char(Gbuf1, i->character);
  /* timer, so they don't sit here forever */
  if (d->character->only.pc->prestige > 3)
  {
    SEND_TO_Q
      ("\r\nAppears that no one is free or cares to review you. Enjoy the game.\r\n",
       d);
    writeCharacter(d->character, 2, NOWHERE);
    STATE(d) = CON_RMOTD;
  }
  else
    d->character->only.pc->prestige++;
}

void wimps_in_accept_queue(void)
{
  P_desc   d;

  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_ACCEPTWAIT)
      newby_announce(d);
}

/*=========================================================================*/
/*
 *    Main routine, nanny
 *      deal with newcomers and other non-playing sockets
 *      Added in Gond's changes from 5-94, cleaned it up alot so I could
 *      understand what the fuck is going on.. (SAM 7-94)
 */
/*=========================================================================*/

void nanny(P_desc d, char *arg)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_STRING_LENGTH];

  switch (STATE(d))
  {

    /* Character deleted by a Forger */
  case CON_DELETE:
    SEND_TO_Q("\r\n\r\nCharacter is deleted!\r\n", d);
    statuslog(d->character->player.level,
              "%s forced to delete character.", GET_NAME(d->character));
    logit(LOG_PLAYER, "%s deleted by a forger.", GET_NAME(d->character));
    deleteCharacter(d->character);
    STATE(d) = CON_FLUSH;
    break;

    /* Terminal type */
  case CON_TERM:
    select_terminal(d, arg);
    break;

  case CON_EXIT:
    close_socket(d);
    return;

#ifdef USE_ACCOUNT
    // Select Account Name
  case CON_GET_ACCT_NAME:
    select_accountname(d, arg);
    break;

  case CON_GET_ACCT_PASSWD:
    get_account_password(d, arg);
    break;
/*
  case CON_IS_ACCT_CONFIRMED:
        echo_on(d);
        if(is_account_confirmed(d)) {
                display_account_menu(d, NULL);
                STATE(d) = CON_DISPLAY_ACCT_MENU;
        } else {
                confirm_account(d, NULL);
                STATE(d) = CON_CONFIRM_ACCT;
        }
        break;
        */

  case CON_CONFIRM_ACCT:
    confirm_account(d, arg);
    break;

  case CON_VERIFY_NEW_ACCT_NAME:
    verify_account_name(d, arg);
    break;

  case CON_GET_NEW_ACCT_EMAIL:
    get_new_account_email(d, arg);
    break;

  case CON_VERIFY_NEW_ACCT_EMAIL:
    verify_new_account_email(d, arg);
    break;

  case CON_GET_NEW_ACCT_PASSWD:
    get_new_account_password(d, arg);
    break;

  case CON_VERIFY_NEW_ACCT_PASSWD:
    verify_new_account_password(d, arg);
    break;

  case CON_VERIFY_NEW_ACCT_INFO:
    verify_new_account_information(d, arg);
    break;

  case CON_ACCT_SELECT_CHAR:
    account_select_char(d, arg);
    break;

  case CON_ACCT_NEW_CHAR:
    account_new_char(d, arg);
    break;

  case CON_ACCT_DELETE_CHAR:
    account_delete_char(d, arg);
    break;

  case CON_ACCT_DISPLAY_INFO:
    account_display_info(d, arg);
    break;

  case CON_ACCT_CHANGE_EMAIL:
    get_new_account_email(d, arg);
    break;

  case CON_ACCT_CHANGE_PASSWD:
    get_new_account_password(d, arg);
    break;

  case CON_ACCT_DELETE_ACCT:
    delete_account(d, arg);
    break;

  case CON_ACCT_VERIFY_DELETE_ACCT:
    verify_delete_account(d, arg);
    break;

  case CON_ACCT_NEW_CHAR_NAME:
    account_new_char_name(d, arg);
    break;






#else
    /* Name of player */
  case CON_NME:
    select_name(d, arg, 1);
    break;

  case CON_NEW_NAME:
    select_name(d, arg, 0);
    break;
#endif

    /* Name confirm for new player */
  case CON_NMECNF:
    /* skip whitespaces */
    for (; isspace(*arg); arg++) ;
    if (*arg == 'y' || *arg == 'Y')
    {
      SEND_TO_Q("\r\nEntering new character generation mode.\r\n", d);
      SEND_TO_Q(namechart, d);
      STATE(d) = CON_APROPOS;
    }
    else
    {
      if (*arg == 'n' || *arg == 'N')
      {
        SEND_TO_Q("\r\nOk, what IS it, then? Type 'generate' for name generator.",
                  d);
        FREE(d->character->player.name);
        d->character->player.name = 0;
#ifndef USE_ACCOUNT
        STATE(d) = CON_NME;
#else
        STATE(d) = CON_ACCT_NEW_CHAR;
#endif
      }
      else
      {
        SEND_TO_Q("\r\nPlease type Yes or No? ", d);
      }
    }
    break;
#ifndef USE_ACCOUNT

    /* Player enteres in login */
  case CON_ENTER_LOGIN:
    sprintf(d->registered_login, "%s", arg);
    SEND_TO_Q("\n\rNow, the hostname part of your email address: ", d);
    STATE(d) = CON_ENTER_HOST;
    break;
  case CON_ENTER_HOST:
    sprintf(d->registered_host, "%s", arg);
    if (email_in_use(d->registered_login, d->registered_host))
    {
      SEND_TO_Q("That email is in use already.\n\r", d);
      STATE(d) = CON_EXIT;
      SEND_TO_Q("\n\rPRESS RETURN.", d);
      return;
    }
    sprintf(Gbuf1, "Your email is registered as %s@%s, is this correct? ",
            d->registered_login, d->registered_host);
    SEND_TO_Q(Gbuf1, d);
    STATE(d) = CON_CONFIRM_EMAIL;
    break;
  case CON_CONFIRM_EMAIL:
    for (; isspace(*arg); arg++) ;
    if (*arg == 'y' || *arg == 'Y')
    {                           /* continue */
//      sprintf(Gbuf1, "Please enter your sex? (M/F) ");
//      SEND_TO_Q(Gbuf1, d);

      SEND_TO_Q(racewars, d);
      STATE(d) = CON_RACEWAR;
/*      SEND_TO_Q(racetable, d);
      STATE(d) = CON_QRACE;*/
    }
    else
    {                           /* wrong email */
      SEND_TO_Q("Okay, resetting...\n\r", d);
      SEND_TO_Q("What is your login or userid portion of your email? ", d);
      STATE(d) = CON_ENTER_LOGIN;
    }
    break;
#endif

    /* Appropriate name for new player */
  case CON_APROPOS:
    /* skip whitespaces */
    for (; isspace(*arg); arg++) ;
    if (*arg == 'y' || *arg == 'Y')
    {
/*
   if (mini_mode) {
   SEND_TO_Q("We now need an email address for authorization.\n\rPlease type in your login or userid: ",d);
   STATE(d) = CON_ENTER_LOGIN;
   } else {
 */
#ifndef USE_ACCOUNT
      sprintf(Gbuf1, "\r\nPlease enter a password for %s: ",
              GET_NAME(d->character));
      SEND_TO_Q(Gbuf1, d);
      STATE(d) = CON_PWDGET;
      echo_off(d);
#else
      echo_on(d);
      SEND_TO_Q(racetable, d);
      STATE(d) = CON_QRACE;
#endif
/*     } */
    }
    else
    {
      if (*arg == 'n' || *arg == 'N')
      {
        SEND_TO_Q
          ("Resetting...\r\n\r\nBy what name do you wish to be known? Type 'generate' to get to name generator.",
           d);
        FREE(d->character->player.name);
        d->character->player.name = 0;
        STATE(d) = CON_NME;
      }
      else
      {
        SEND_TO_Q("\r\nPlease type Yes or No!", d);
      }
    }
    break;
#ifndef USE_ACCOUNT
    /* PASSWORD handling */
  case CON_PWDGET:
  case CON_PWDCNF:
  case CON_PWDNEW:
  case CON_PWDNGET:
  case CON_PWDNCNF:
  case CON_PWDDCNF:
  case CON_PWDNRM:
    /* skip whitespaces */
    for (; isspace(*arg); arg++) ;

    if (STATE(d) == CON_PWDNEW ||
        STATE(d) == CON_PWDGET || STATE(d) == CON_PWDNRM)
    {
      /*
       ** Since we have turned off echoing for telnet client,
       ** if a telnet client is indeed used, we need to skip the
       ** initial 3 bytes ( -1, -3, 1 ) if they are sent back by
       ** client program.
       */

      if (*arg == -1)
      {
        if (arg[1] != '0' && arg[2] != '0')
        {
          if (arg[3] == '0')
          {                     /* Password on next read  */
            return;
          }
          else
          {                     /* Password available */
            arg = arg + 3;
          }
        }
        else
          close_socket(d);
      }
    }
    select_pwd(d, arg);
    break;
#endif

    /* Choose sex for new player */
  case CON_QSEX:
    select_sex(d, arg);
    break;

  case CON_NEWBIE:
	select_newbie(d, arg);
	break;

  case CON_HARDCORE:
    select_hardcore(d, arg);
    break;

    /* Select class for new player */
  case CON_QCLASS:
    select_class(d, arg);
    break;

    /* Choose race for new player */
  case CON_QRACE:
    select_race(d, arg);
    break;

    /* Krov: now triggers for general info table */
  case CON_INFO:
    /* Race war info for new player */
  case CON_RACEWAR:
    for (; isspace(*arg); arg++) ;
    SEND_TO_Q(racetable, d);
    STATE(d) = CON_QRACE;
    break;

    /* Reroll stats for new player */
  case CON_REROLL:
    select_reroll(d, arg);
    break;

    /* Stat bonus 1 for new player */
  case CON_BONUS1:
    /* record how many bonuses the char gets, 1d3 */
    select_bonus(d, arg);
    break;

    /* Stat bonus 2 for new player */
  case CON_BONUS2:
    select_bonus(d, arg);
    break;

    /* Stat bonus 3 for new player */
  case CON_BONUS3:
    select_bonus(d, arg);
    break;

    /* Stat bonus 4 for new player */
  case CON_BONUS4:
    select_bonus(d, arg);
    break;

    /* Stat bonus 5 for new player */
  case CON_BONUS5:
    select_bonus(d, arg);
    break;

  case CON_SWAPSTATYN:
    select_swapstat( d, arg );
    break;

  case CON_SWAPSTAT:
    swapstat( d, arg );
    break;

    /* Select alignment for new player, when appropriate */
  case CON_ALIGN:
    select_alignment(d, arg);
    break;

    /* Select hometown for new player, when appropriate */
  case CON_HOMETOWN:
    select_hometown(d, arg);
    break;

    /* Keep the chosen character */
  case CON_KEEPCHAR:
    select_keepchar(d, arg);
    if (STATE(d) == CON_RMOTD)
    {
      logit(LOG_NEW, "%s [%s] new player.", GET_NAME(d->character),
            d->host);
      statuslog(d->character->player.level, "%s [%s] new player.",
                GET_NAME(d->character), d->host);
      init_char(d->character);
#ifdef USE_ACCOUNT
      add_char_to_account(d);
#endif
      SEND_TO_Q(motd.c_str(), d);
      SEND_TO_Q("\r\n\r\n*** PRESS RETURN to read Duris rules. ", d);
      SEND_TO_Q
        ("\r\n*** (Note: You MUST read and agree to all rules to play this mud!)\r\n",
         d);
      STATE(d) = CON_QRETURN;
    }
    break;

    /* Prepare for the mighty disclaimer */
  case CON_QRETURN:

    do_help(d->character, "rules", -4);
    SEND_TO_Q("\r\n", d);

    SEND_TO_Q
      ("\r\n\r\n*** Do you agree to abide by everything written in the set of rules?",
       d);
    SEND_TO_Q("\r\n\r\n(By entering No (or N), you will exit the Mud.)", d);
    SEND_TO_Q("\r\n\r\nYour official and legal response:", d);
    STATE(d) = CON_DISCLMR;

/*   STATE(d) = CON_DSCLMR; */
    break;

/*
   case CON_DISCLMR:
   SEND_TO_Q("\r\n", d);
   SEND_TO_Q(
   "\r\n\r\n*** Do you agree to abide by everything written in the disclaimer?", d);
   SEND_TO_Q("\r\n\r\n(By entering No (or N), you will exit the Mud.)", d);
   SEND_TO_Q("\r\n\r\nYour official and legal responce:", d);
   STATE(d) = CON_DISCLMR;
   break;
 */

    /* response to disclaimer */
  case CON_DISCLMR:
    for (; isspace(*arg); arg++) ;
    switch (*arg)
    {
    case 'N':
    case 'n':
      SEND_TO_Q("\r\n\r\nThank you for considering our mud.\r\n", d);
      close_socket(d);
      return;
      break;
    case 'Y':
    case 'y':
      SEND_TO_Q
        ("\r\n\r\nYou have selected Yes, and hereby agree to all conditions in the set of rules.\r\n",
         d);
      SEND_TO_Q
        ("Now you have to wait for your character to be approved by a god.\r\nProcess should not take long.\r\n",
         d);
      break;
    default:
      SEND_TO_Q("\r\nThat is not a correct response. Try again.\r\n", d);
      return;
      break;
    }
    if (pfile_exists("Players/Accepted", GET_NAME(d->character)))
    {
      SEND_TO_Q
        ("This name has been accepted before, and it is accepted once more.\r\n\r\n"
         "*** PRESS RETURN:\r\n", d);
      STATE(d) = CON_RMOTD;
      statuslog(d->character->player.level,
                "%s auto-accepted due to having been accepted before.",
                GET_NAME(d->character));
    }
    else if (!IS_TRUSTED(d->character) && accept_mode)
    {
      SEND_TO_Q
        ("Now you have to wait for your character to be approved by a god.\r\nProcess should not take long.\r\nIf no god is on to approve you, you will be auto-approved in 5 mins.\r\n",
         d);
      STATE(d) = CON_ACCEPTWAIT;
      newby_announce(d);
    }
    else
    {
      SEND_TO_Q
        ("Now you have to wait for your character to be approved by a god.\r\nProcess should not take long.\r\nIf no god is on to approve you, you will be auto-approved in 5 mins.\r\n",
         d);
      SEND_TO_Q("\r\n*** PRESS RETURN:\r\n", d);
      writeCharacter(d->character, 2, NOWHERE);
      STATE(d) = CON_RMOTD;
    }
/*    for (; isspace(*arg); arg++);
    switch (*arg) {
    case 'Y':
    case 'y':
      SEND_TO_Q(
                 "\r\n\r\nYou have selected Yes, and hereby agree to all conditions in the disclaimer.\r\n", d);
      if (!IS_TRUSTED(d->character) && accept_mode) {
                  writeCharacter(d->character, 2, NOWHERE);
        SEND_TO_Q("Now you have to wait for your character to be approved by a god.\r\nProcess should not take long.\r\n", d);
        STATE(d) = CON_ACCEPTWAIT;
        newby_announce(d);
      } else {
        SEND_TO_Q("\r\n*** PRESS RETURN:\r\n", d);
        writeCharacter(d->character, 2, NOWHERE);
        STATE(d) = CON_RMOTD;
      }
      break;
    case 'N':
    case 'n':
      SEND_TO_Q("\r\n\r\nThank you for considering our mud.\r\n", d);
      close_socket(d);
      return;
      break;
    default:
      SEND_TO_Q("\r\nThat is not a correct response. Try again.\r\n", d);
      return;
      break;
    }*/
    break;

  case CON_ACCEPTWAIT:
    SEND_TO_Q
      ("You cannot do anything during this period. If the time you have waited is\r\ntoo long (by your point of view), you can come back later on.\r\n",
       d);
    break;

  case CON_WELCOME:
#if 0
    if (mini_mode)
    {
      struct registration_node *x;
      CREATE(x, struct registration_node, 1);

      sprintf(x->host, "%s", d->registered_host);
      sprintf(x->login, "%s", d->registered_login);
      sprintf(x->name, "%s", GET_NAME(d->character));
      x->name[0] = tolower(x->name[0]);
      x->next = email_reg_table[(int) x->name[0] - (int) 'a'].next;
      email_reg_table[(int) x->name[0] - (int) 'a'].next = x;
      dump_email_reg_db();
      email_player_info(x->login, x->host, d);
      SEND_TO_Q
        ("Your character information will be emailed to you shortly.\n\r", d);
      STATE(d) = CON_EXIT;
      return;
    }
#endif
    writeCharacter(d->character, 2, NOWHERE);
#ifdef USE_ACCOUNT
    display_account_menu(d, arg);
#else
    SEND_TO_Q(MENU, d);
#endif
    STATE(d) = CON_SLCT;
    break;

  case CON_RMOTD:
#ifdef USE_ACCOUNT
    display_account_menu(d, arg);
#else
    SEND_TO_Q(MENU, d);
#endif
    STATE(d) = CON_SLCT;
    break;

    /* Main menu */
  case CON_SLCT:
  case CON_DISPLAY_ACCT_MENU:
#ifdef USE_ACCOUNT
    display_account_menu(d, arg);
#else
    select_main_menu(d, arg);
#endif
    break;

  case CON_HOST_LOOKUP:
    SEND_TO_Q
      ("Please enter term type (<CR> for ANSI, '1' for Generic, '9' for Quick): ",
       d);
    STATE(d) = CON_TERM;
    break;

    /* Flush output messages, then kill the descriptor */
  case CON_FLUSH:
  default:
    if (STATE(d) != CON_FLUSH)
      logit(LOG_EXIT, "Nanny: illegal state of con'ness #1 (%d)", STATE(d));
    if (d->character && d->character->events)
      ClearCharEvents(d->character);
    if (d->output.head == 0)
      close_socket(d);
    return;
#if 0
    /* better not get here or something is hosed */
  default:
    logit(LOG_EXIT, "Nanny: illegal state of con'ness (%d)", STATE(d));
    raise(SIGSEGV);
    break;

#endif
  }
}

/* EMAIL: takes a char, generate a random password for them, then emails
 * the password data to them.
 */


void email_player_info(char *login, char *host, struct descriptor_data *d)
{

  char     buf[MAX_STRING_LENGTH];
  char     password[9];
  int      counter;
  FILE    *fp;


  for (counter = 0; counter < 6; counter++)
    password[counter] = (random() % 26) + 97;
  for (counter = 6; counter < 8; counter++)
    password[counter] = (random() % 10) + 48;
  password[8] = '\0';
  sprintf(buf, "/tmp/%s.REG", GET_NAME(d->character));
  if (!(fp = fopen(buf, "w")))
  {
    ereglog(AVATAR, "Could not open emailreg temp file! (%s)",
            GET_NAME(d->character));
    return;
  };
  fprintf(fp, "  *** Duris Character Registration *** \n\n\n");
  fprintf(fp, " INSERT POLICY BULLSHIT HERE\n\n\n");
  fprintf(fp, "Your character name is: %s\n", GET_NAME(d->character));
  fprintf(fp, "Your password is %s\n", password);
  fprintf(fp, ".\n");
  fclose(fp);
  if (!(fp = fopen("EmailReg.Q", "at")))
  {
    ereglog(AVATAR, "Could not open Q file! (%s)", GET_NAME(d->character));
    return;
  };
  fprintf(fp, "mail -s \"%s\" %s@%s < /tmp/%s.REG\n", "Duris Character",
          login, host, GET_NAME(d->character));
  ereglog(AVATAR, "Executing Command %s", buf);
  fclose(fp);
}




char    *hint_array[1000];
int      iLOADED = 0;

void loadHints()
{
  FILE    *f;
  char     buf2[MAX_STR_NORMAL * 10];
  int      i = 0;

  f = fopen("lib/information/hints.txt", "r");

  if (!f)
    return;

  while (!feof(f))
  {

    if (fgets(buf2, MAX_STR_NORMAL * 10 - 1, f))
    {
      hint_array[i] = str_dup(buf2);
      i++;
    }
  }

  iLOADED = i;
  fclose(f);
}

int tossHint(P_char ch)
{
  char     buf2[MAX_STR_NORMAL * 10];

  if (iLOADED < 1)
    return 0;
  sprintf(buf2, "&+MHint: &+m%s", hint_array[number(0, iLOADED - 1)]);
  send_to_char(buf2, ch);
}


void Decrypt(char *text, int sizeOfText, const char *key, int sizeOfKey)
{
  int      offSet = 0;

  int      i = 0;

  for (; i < sizeOfText; ++i, ++offSet)
  {
    if (offSet >= sizeOfKey)
      offSet = 0;

    int      value = text[i];
    int      keyValue = key[offSet];

    value -= keyValue;

    char     decryptedChar = value;

    text[i] = decryptedChar;
  }
}

void show_swapstat( P_desc d )
{
  SEND_TO_Q("\r\nThe following letters correspond to the stats:\r\n", d);
  SEND_TO_Q("(S)trength            (P)ower\n\r"
    "(D)exterity           (I)ntelligence\n\r"
    "(A)gility             (W)isdom\n\r"
    "(C)onstitution        C(h)arisma\n\r"
    "(L)uck\n\r", d);
  SEND_TO_Q("\r\nEnter two letters separated by a space to swap: \r\n", d);
}

void select_swapstat( P_desc d, char *arg )
{
  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  switch (*arg)
  {
  case 'N':
  case 'n':
    SEND_TO_Q("\r\n\r\nAccepting these stats.\r\n\r\n", d);
    display_characteristics(d);
    display_stats(d);
    SEND_TO_Q(keepchar, d);
    STATE(d) = CON_KEEPCHAR;
    break;
  case 'Y':
  case 'y':
    show_swapstat( d );
    STATE(d) = CON_SWAPSTAT;
    break;
  default:
    SEND_TO_Q("\r\nUnrecognized response.\r\n", d);
    display_stats(d);
    SEND_TO_Q("Do you want to swap stats (Y/N): ", d);
    STATE(d) = CON_SWAPSTATYN;
    break;
  }
}

void swapstat( P_desc d, char *arg )
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int stat1, stat2;

  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);
  if( !*arg1 || !*arg2 )
  {
    SEND_TO_Q("\r\nUnrecognized response.\r\n", d);
    display_stats(d);
    SEND_TO_Q("Do you want to swap stats (Y/N): ", d);
    STATE(d) = CON_SWAPSTATYN;
    return;
  }
  switch( LOWER(*arg1) )
  {
    case 's':
      stat1 = 1;
      break;
    case 'd':
      stat1 = 2;
      break;
    case 'a':
      stat1 = 3;
      break;
    case 'c':
      stat1 = 4;
      break;
    case 'p':
      stat1 = 5;
      break;
    case 'i':
      stat1 = 6;
      break;
    case 'w':
      stat1 = 7;
      break;
    case 'h':
      stat1 = 8;
      break;
    case 'l':
      stat1 = 9;
      break;
    default:
      stat1 = -1;
  }
  switch( LOWER(*arg2) )
  {
    case 's':
      stat2 = 1;
      break;
    case 'd':
      stat2 = 2;
      break;
    case 'a':
      stat2 = 3;
      break;
    case 'c':
      stat2 = 4;
      break;
    case 'p':
      stat2 = 5;
      break;
    case 'i':
      stat2 = 6;
      break;
    case 'w':
      stat2 = 7;
      break;
    case 'h':
      stat2 = 8;
      break;
    case 'l':
      stat2 = 9;
      break;
    default:
      stat2 = -1;
  }
  if( stat1 == -1 || stat2 == -1 )
  {
    SEND_TO_Q("\r\nUnrecognized response.\r\n", d);
    display_stats(d);
    SEND_TO_Q("Do you want to swap stats (Y/N): ", d);
    STATE(d) = CON_SWAPSTATYN;
    return;
  }

  swapstats( d->character, stat1, stat2 );

    display_stats(d);
    SEND_TO_Q("Do you want to swap more stats (Y/N): ", d);
    STATE(d) = CON_SWAPSTATYN;
}

void swapstats(P_char ch, int stat1, int stat2)
{
  int  tmp;
  sh_int *pstat1;

  // Record stat1 value and location.
  switch( stat1 )
  {
    case 1:
      tmp = ch->base_stats.Str;
      pstat1 = &(ch->base_stats.Str);
      break;
    case 2:
      tmp = ch->base_stats.Dex;
      pstat1 = &(ch->base_stats.Dex);
      break;
    case 3:
      tmp = ch->base_stats.Agi;
      pstat1 = &(ch->base_stats.Agi);
      break;
    case 4:
      tmp = ch->base_stats.Con;
      pstat1 = &(ch->base_stats.Con);
      break;
    case 5:
      tmp = ch->base_stats.Pow;
      pstat1 = &(ch->base_stats.Pow);
      break;
    case 6:
      tmp = ch->base_stats.Int;
      pstat1 = &(ch->base_stats.Int);
      break;
    case 7:
      tmp = ch->base_stats.Wis;
      pstat1 = &(ch->base_stats.Wis);
      break;
    case 8:
      tmp = ch->base_stats.Cha;
      pstat1 = &(ch->base_stats.Cha);
      break;
    case 9:
      tmp = ch->base_stats.Luk;
      pstat1 = &(ch->base_stats.Luk);
      break;
    default:
      send_to_char( "Error in swapstats Part 1!  Tell a God.\n\r", ch );
      return;
      break;
  }
  // Swap: put stat2 value into stat1 and tmp into stat2 value.
  switch( stat2 )
  {
    case 1:
      *pstat1 = ch->base_stats.Str;
      ch->base_stats.Str = tmp;
      break;
    case 2:
      *pstat1 = ch->base_stats.Dex;
      ch->base_stats.Dex = tmp;
      break;
    case 3:
      *pstat1 = ch->base_stats.Agi;
      ch->base_stats.Agi = tmp;
      break;
    case 4:
      *pstat1 = ch->base_stats.Con;
      ch->base_stats.Con = tmp;
      break;
    case 5:
      *pstat1 = ch->base_stats.Pow;
      ch->base_stats.Pow = tmp;
      break;
    case 6:
      *pstat1 = ch->base_stats.Int;
      ch->base_stats.Int = tmp;
      break;
    case 7:
      *pstat1 = ch->base_stats.Wis;
      ch->base_stats.Wis = tmp;
      break;
    case 8:
      *pstat1 = ch->base_stats.Cha;
      ch->base_stats.Cha = tmp;
      break;
    case 9:
      *pstat1 = ch->base_stats.Luk;
      ch->base_stats.Luk = tmp;
      break;
    default:
      send_to_char( "Error in swapstats Part 2!  Tell a God.\n\r", ch );
      return;
      break;
  }

  ch->curr_stats = ch->base_stats;
}
