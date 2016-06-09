
/*
 *************************************************************************
 * File: actset.c                                           Part of Duris
 * Usage: low level data fiddling routines
 * Copyright  1991 - Andrew Choi (I think, it has statics from hell. JAB)
 * Copyright 1994 - 2008 - Duris Systems Ltd.
 *************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "defines.h"
#include "sql.h"
#include "ships/ships.h"

/* external variables */

extern P_desc descriptor_list;
extern P_room world;
extern const char *language_names[];
extern const flagDef action_bits[];
extern const flagDef action2_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern const flagDef aggro_bits[];
extern const flagDef aggro2_bits[];
extern const flagDef aggro3_bits[];
extern const char *apply_types[];
extern const char *connected_types[];
extern const char *dirs[];
extern const char *exit_bits[];
extern const flagDef extra_bits[];
extern const flagDef extra2_bits[];
extern const flagDef anti_bits[];
extern const flagDef anti2_bits[];
extern const char *item_material[];
extern const char *item_types[];
extern const char *player_bits[];
extern const char *player2_bits[];
extern const char *player_law_flags[];
extern const char *position_types[];
extern const flagDef room_bits[];
extern const char *sector_types[];
extern const Skill skills[];
extern const char *spells[];
extern const flagDef wear_bits[];
extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];
extern const char *size_types[];
extern const int top_of_world;
extern const int rev_dir[];
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern void reset_racial_skills(P_char ch);

char bad_on_off[MAX_INPUT_LENGTH];

/* Macros */
#define SETBIT_ROOM             0
#define SETBIT_CHAR             1
#define SETBIT_OBJ              2
#define SETBIT_DIR              3
#define SETBIT_AFF              4
#define SETBIT_ZONE             5
#define SETBIT_SHIP             6
#define SETBIT_MAX              6

#define ac_shintCopy            ac_sh_intCopy
#define ARRAY_SIZE(A)           (sizeof(A)/sizeof(*(A)))
#define SAME_STRING(A, B)       ac_strcasecmp((A), (B))
#define LOWER_CASE(C)           (isupper(C) ? tolower(C) : (C))
#define OFFSET_OF(Type, Field)  ((int) (((char *) (&(((Type) NULL)->Field))) - ((char *) NULL)))

/* Types */
struct setBitTable
{
  const char  *sb_flag;             // Name of "flag"
  int          sb_offset;           // Offset from beginning of struct
  const char **sb_subtable;         // Subtable for options

  void       (*sb_func) (void *, int, char *, int, int);
  int          entry_size;
  int          entry_offset;
};

typedef struct setBitTable SetBitTable;

/* Private Interface */

static int setbit_parse(char *arg, int *, char *, char *, char *, int *);

/* Parse command into arguments */
/* Basically same as "one_argument", except this one allows quoting with
 * single quote. */

char    *setbit_parseArgument(char *arg, char *val);

/* Function for branching into "room", "char", or "object" */
static void setbit_room(P_char, char *, char *, char *, int);
static void setbit_char(P_char, char *, char *, char *, int);
static void setbit_obj(P_char, char *, char *, char *, int);
static void setbit_dir(P_char, char *, char *, char *, int);
static void setbit_aff(P_char, char *, char *, char *, int);
static void setbit_zone(P_char ch, char *name, char *flag, char *val, int);
static void setbit_ship(P_char, char *, char *, char *, int);

/* Parse table routine */
static void setbit_parseTable(P_char, void *, SetBitTable *, int, char *,
    char *, int, int);
static void setbit_syntax(P_char ch, int type);

/* Syntax error */
static void setbit_printOutTable(P_char ch, SetBitTable *, int size);
static void setbit_printOutSubTable(P_char ch, const char **subtable, int entry_size);
static int ac_strcasecmp(const char *s1, const char *s2);

/* String insensitive case comparison */
/* Copy functions */

/*
 * ** Copy functions are of the following formats: ** ** func(void *where,
 * -- Where to copy into **   int  offset,    -- Offset from beginning of
 * "where" in bytes **   char *value,    -- Value specified by command **
 *  int  bit,       -- Bit number or atoi(value) **   int  on_off)    --
 * Turn bit on/off
 */

static void ac_ageCopy(void *, int, char *, int, int);
static void ac_affModify(void *, int, char *, int, int);
static void ac_bitCopy(void *, int, char *, int, int);
static void ac_byteCopy(void *, int, char *, int, int);
static void ac_hitmanaCopy(void *, int, char *, int, int);
static void ac_intCopy(void *, int, char *, int, int);
static void ac_longCopy(void *, int, char *, int, int);
static void ac_objaffCopy(void *, int, char *, int, int);
static void ac_positionCopy(void *, int, char *, int, int);
static void ac_savthrCopy(void *, int, char *, int, int);
static void ac_sbyteCopy(void *, int, char *, int, int);
static void ac_sh_intCopy(void *, int, char *, int, int);
static void ac_shortCopy(void *, int, char *, int, int);
static void ac_skillCopy(void *, int, char *, int, int);
static void ac_tongueCopy(void *, int, char *, int, int);
static void ac_ubyteCopy(void *, int, char *, int, int);
static void ac_idx2flagCopy(void *, int, char *, int, int);
static void ac_timerCopy( void *, int, char *, int, int);
/*
 * Wizard's command:
 * setbit "room" room-number flag-type value [ "on | off" ]
 * setbit "char" name flag-type value [ "on | off"]
 * setbit "obj"  name flag-type value [ "on | off"]
 * setbit "dir"  room-number flag-type value [ "on | off" ]
 * setbit "aff"  name "AFFECT NUMBER" flag-type value [ "on | off" ]
 */

void do_setbit(P_char ch, char *arg, int cmd)
{
  int      type, on_off;
  char     name[MAX_INPUT_LENGTH];
  char     flag[MAX_INPUT_LENGTH];
  char     value[MAX_INPUT_LENGTH];
  P_char   target;

  if (IS_NPC(ch))
  {
    return;
  }
  if( setbit_parse(arg, &type, name, flag, value, &on_off) == -1 )
  {
    setbit_syntax(ch, -1);
    return;
  }

  if ( cmd != CMD_SETHOME || GET_LEVEL(ch) >= MINLVLIMMORTAL )
  {
    wizlog(GET_LEVEL(ch), "%s: setbit %s", GET_NAME(ch), arg);
    logit(LOG_WIZ, "%s: setbit %s", GET_NAME(ch), arg);
    sql_log(ch, WIZLOG, "setbit %s", arg);
  }

  switch (type)
  {

  case SETBIT_ROOM:
    setbit_room(ch, name, flag, value, on_off);
    return;

  case SETBIT_CHAR:
    if (!god_check(GET_NAME(ch)) && god_check(name))
    {
      act("One hella pissed god says 'Hey buddy, that's not very polite, trying to setbit my ass.'",
        FALSE, ch, 0, 0, TO_ROOM);
      act("One hella pissed god says 'Hey buddy, that's not very polite, trying to setbit my ass.'",
        FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    setbit_char(ch, name, flag, value, on_off);
    // Different classes/specs get different skills.
    if( !strcmp(flag, "spec") || !strcmp( flag, "class" ) )
    {
      target = get_char_vis(ch, name);
      if (target == NULL)
      {
        send_to_char("No one by that name here.\r\n", ch);
        return;
      }
      update_skills(target);
    }
    break;

  case SETBIT_OBJ:
    setbit_obj(ch, name, flag, value, on_off);
    break;

  case SETBIT_DIR:
    setbit_dir(ch, name, flag, value, on_off);
    break;

  case SETBIT_AFF:
    setbit_aff(ch, name, flag, value, on_off);
    break;

  case SETBIT_ZONE:
    setbit_zone(ch, name, flag, value, on_off);
    break;

  case SETBIT_SHIP:
    setbit_ship(ch, name, flag, value, on_off);
    break;

  default:
    logit(LOG_DEBUG, "SETBIT:  Unknown type: %d (%s %d)", type, __FILE__,
      __LINE__ );
    break;
  }
}

/*
 * Private Interface
 */

/*
 * ** Parses command into separate fields.
 */
static int setbit_parse(char *arg, int *type, char *name, char *flag, char *val, int *on_off)
{
  char     type_str[MAX_INPUT_LENGTH];
  char     on_off_str[MAX_INPUT_LENGTH];

  /* Separate into fields */

  if ((arg = setbit_parseArgument(arg, type_str)) == NULL)
    return -1;
  if ((arg = setbit_parseArgument(arg, name)) == NULL)
    return -1;
  if ((arg = setbit_parseArgument(arg, flag)) == NULL)
    return -1;
  if ((arg = setbit_parseArgument(arg, val)) == NULL)
    return -1;

  setbit_parseArgument(arg, on_off_str);

  /* String conversion to integer */

  switch (*type_str)
  {

    case 'r':
    case 'R':
      *type = SETBIT_ROOM;
      break;

    case 'c':
    case 'C':
    case 'm':
    case 'M':
      *type = SETBIT_CHAR;
      break;

    case 'o':
    case 'O':
    case 'i':
    case 'I':
      *type = SETBIT_OBJ;
      break;

    case 'd':
    case 'D':
      *type = SETBIT_DIR;
      break;

    case 'a':
    case 'A':
      *type = SETBIT_AFF;
      break;

    case 'z':
    case 'Z':
      *type = SETBIT_ZONE;
      break;

    case 's':
    case 'S':
      *type = SETBIT_SHIP;
      break;

    default:
      return -1;
  }

  /*
   * On/Off
   */
  if( is_number(on_off_str) )
  {
    *on_off = atoi(on_off_str);
    bad_on_off[0] = '\0';
  }
  else
  {
    if( *on_off_str )
    {
      sprintf( bad_on_off, "%s", on_off_str );
    }
    else
    {
      sprintf( bad_on_off, " " );
    }
  }

  // Bad on_off_str .. may not matter
  return 0;
}

/*
 * ** For struct room_data: ** ** We allow the following fields to be
 * modified: ** ** 1) zone ** 2) sector type ** 3) room flags ** 4) trap
 * level
 */
static void setbit_room(P_char ch, char *name, char *flag, char *val, int on_off)
{

  /* Internal Macros */
#undef  OFFSET
#define OFFSET(Field)   OFFSET_OF(P_room, Field)

  /* Table */
  SetBitTable table[] = {
    {"zone",              OFFSET(zone),                      NULL,         ac_shintCopy},
    {"sect",              OFFSET(sector_type),               sector_types, ac_byteCopy, sizeof(char *)},
    {"flag",              OFFSET(room_flags), (const char**) room_bits,    ac_bitCopy,  sizeof(flagDef)},
    {"light",             OFFSET(light),                     NULL,         ac_shortCopy},
    {"fall",              OFFSET(chance_fall),               NULL,         ac_shortCopy},
    {"speed_current",     OFFSET(current_speed),             NULL,         ac_shortCopy},
    {"direction_current", OFFSET(current_direction),         dirs,         ac_sbyteCopy, sizeof(char *)}
  };

  /* Local Variables */
  int      room_number;

  /* Executable Section */
  if( !strcmp(name, "here") )
  {
    room_number = ch->in_room;
  }
  else
  {
    room_number = real_room(atoi(name));
  }

  if( room_number < 0 || room_number > top_of_world )
  {
    send_to_char( "Invalid room number.\n\r", ch );
    return;
  }
  setbit_parseTable(ch, (void *) (world + room_number), table, ARRAY_SIZE(table), flag, val, on_off, SETBIT_ROOM);

  // Since we can set room lights (highly temporary as they get reset quickly),
  //   we don't update the light here. - Lohrr
//  room_light(room_number, REAL);
}

/*
 * ** For struct room_data: ** ** We allow the following fields to be
 * modified: ** ** 1) zone ** 2) sector type ** 3) room flags ** 4) trap
 * level
 */
static void setbit_zone(P_char ch, char *name, char *flag, char *val, int on_off)
{
  if( GET_LEVEL(ch) < FORGER )
  {
    return;
  }

  /* Internal Macros */
#undef  OFFSET
#define OFFSET(Field)   OFFSET_OF(struct zone_data*, Field)

  /* Table */
  SetBitTable table[] = {
    {"difficulty", OFFSET(difficulty), NULL, ac_shintCopy},
    {"age", OFFSET(age), NULL, ac_intCopy}
  };

  /* Local Variables */
  int      zone_id;

  /* Executable Section */
  int zone_number = atoi(name);
  zone_id = real_zone(zone_number);

  if( zone_id < 0 )
  {
    send_to_char("Invalid zone ID.\n", ch);
    return;
  }

  if( !strcmp(flag, "difficulty") )
  {
    if( atoi(val) < 1 || atoi(val) > 13 )
    {
      send_to_char("Difficulty must be from 1 to 13\n", ch);
      return;
    }
  }
  else if( !strcmp(flag, "age") )
  {
    if( atoi(val) < 0 || atoi(val) > 1000 )
    {
      send_to_char("Age must be from 0 to 1000.\n", ch);
      return;
    }
  }

  setbit_parseTable(ch, (void *) (zone_table + zone_id), table,
      ARRAY_SIZE(table), flag, val, on_off, SETBIT_ZONE);
}

static void setbit_char(P_char ch, char *name, char *flag, char *val, int on_off)
{
  /* Internal Macros */
#undef  OFFSET
#define OFFSET(Field)   OFFSET_OF(P_char, Field)
#define PLOFFSET(Field) OFFSET(player . Field)
#define ABOFFSET(Field) OFFSET(base_stats . Field)
#define TAOFFSET(Field) OFFSET(curr_stats . Field)
#define POOFFSET(Field) OFFSET(points . Field)
#define SPOFFSET(Field) OFFSET(specials . Field)
#define PCOFFSET(Field) OFFSET_OF(struct pc_only_data *, Field)
#define NPOFFSET(Field) OFFSET_OF(struct npc_only_data *, Field)

  SetBitTable table[] = {
    /* char_player_data */
    {"sex", PLOFFSET(sex), NULL, ac_ubyteCopy},
    {"race", PLOFFSET(race), (const char **)race_names_table, ac_ubyteCopy, sizeof(struct race_names)},
    {"racewar", PLOFFSET(racewar), NULL, ac_ubyteCopy},
    {"level", PLOFFSET(level), NULL, ac_ubyteCopy},
    {"spec", PLOFFSET(spec), NULL, ac_ubyteCopy},
    {"home", PLOFFSET(hometown), NULL, ac_intCopy},
    {"orighome", PLOFFSET(birthplace), NULL, ac_intCopy},
    {"origbp", PLOFFSET(orig_birthplace), NULL, ac_intCopy},
    {"age", 0, NULL, ac_ageCopy},
    {"weight", PLOFFSET(weight), NULL, ac_shortCopy},
    {"height", PLOFFSET(height), NULL, ac_shortCopy},
    {"size", PLOFFSET(size), size_types, ac_ubyteCopy, sizeof(char *)},
    /* stat_data */
    {"str", ABOFFSET(Str), NULL, ac_shortCopy},
    {"dex", ABOFFSET(Dex), NULL, ac_shortCopy},
    {"agi", ABOFFSET(Agi), NULL, ac_shortCopy},
    {"con", ABOFFSET(Con), NULL, ac_shortCopy},
    {"pow", ABOFFSET(Pow), NULL, ac_shortCopy},
    {"int", ABOFFSET(Int), NULL, ac_shortCopy},
    {"wis", ABOFFSET(Wis), NULL, ac_shortCopy},
    {"cha", ABOFFSET(Cha), NULL, ac_shortCopy},
    {"karma", ABOFFSET(Kar), NULL, ac_shortCopy},
    {"luck", ABOFFSET(Luk), NULL, ac_shortCopy},
    /* stat_data (temporary) */
    {"tstr", TAOFFSET(Str), NULL, ac_shortCopy},
    {"tdex", TAOFFSET(Dex), NULL, ac_shortCopy},
    {"tagi", TAOFFSET(Agi), NULL, ac_shortCopy},
    {"tcon", TAOFFSET(Con), NULL, ac_shortCopy},
    {"tpow", TAOFFSET(Pow), NULL, ac_shortCopy},
    {"tint", TAOFFSET(Int), NULL, ac_shortCopy},
    {"twis", TAOFFSET(Wis), NULL, ac_shortCopy},
    {"tcha", TAOFFSET(Cha), NULL, ac_shortCopy},
    {"tkarma", TAOFFSET(Kar), NULL, ac_shortCopy},
    {"tluck", TAOFFSET(Luk), NULL, ac_shortCopy},
    /* char_point_data */
    {"mana", POOFFSET(mana), NULL, ac_shintCopy},
    {"mxmana", POOFFSET(base_mana), NULL, ac_hitmanaCopy},
    {"hit", POOFFSET(hit), NULL, ac_intCopy},
    {"basehit", POOFFSET(base_hit), NULL, ac_intCopy},
    {"vit", POOFFSET(vitality), NULL, ac_shintCopy},
    {"mxvit", POOFFSET(base_vitality), NULL, ac_shintCopy},
    {"ac", POOFFSET(base_armor), NULL, ac_shintCopy},
    {"copper", POOFFSET(cash[0]), NULL, ac_intCopy},
    {"silver", POOFFSET(cash[1]), NULL, ac_intCopy},
    {"gold", POOFFSET(cash[2]), NULL, ac_intCopy},
    {"platinum", POOFFSET(cash[3]), NULL, ac_intCopy},
    {"exp", POOFFSET(curr_exp), NULL, ac_longCopy},
    {"hitrol", POOFFSET(base_hitroll), NULL, ac_sbyteCopy},
    {"damrol", POOFFSET(base_damroll), NULL, ac_sbyteCopy},
    {"diceno", POOFFSET(damnodice), NULL, ac_sbyteCopy},
    {"dicesz", POOFFSET(damsizedice), NULL, ac_sbyteCopy},
    /* char_special_data */
    {"pos", SPOFFSET(position), position_types, ac_positionCopy, sizeof(char *)},
    {"pcact", SPOFFSET(act), player_bits, ac_bitCopy, sizeof(char *)},
    {"pcact2", SPOFFSET(act2), player2_bits, ac_bitCopy, sizeof(char *)},
    {"carryw", SPOFFSET(carry_weight), NULL, ac_intCopy},
    {"carryn", SPOFFSET(carry_items), NULL, ac_shortCopy},
    {"timer", SPOFFSET(timer), NULL, ac_shortCopy},
    {"wasin", SPOFFSET(was_in_room), NULL, ac_intCopy},
    {"savthr", 0, NULL, ac_savthrCopy},
    {"drunk", SPOFFSET(conditions[0]), NULL, ac_sbyteCopy},
    {"hunger", SPOFFSET(conditions[1]), NULL, ac_sbyteCopy},
    {"thirst", SPOFFSET(conditions[2]), NULL, ac_sbyteCopy},
    {"zcord", SPOFFSET(z_cord), NULL, ac_sbyteCopy},
    {"align", SPOFFSET(alignment), NULL, ac_shintCopy},
    {"ascnum", SPOFFSET(guild), NULL, ac_shintCopy},
    {"aff", SPOFFSET(affected_by), (const char **) affected1_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff2", SPOFFSET(affected_by2), (const char **) affected2_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff3", SPOFFSET(affected_by3), (const char **) affected3_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff4", SPOFFSET(affected_by4), (const char **) affected4_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff5", SPOFFSET(affected_by5), (const char **) affected5_bits, ac_bitCopy, sizeof(flagDef)},
    {"class", PLOFFSET(m_class), (const char **) class_names_table, ac_idx2flagCopy, sizeof(struct class_names)},
    {"secondary", PLOFFSET(secondary_class), (const char **) class_names_table, ac_idx2flagCopy, sizeof(struct class_names)},
    {"multiclass", PLOFFSET(m_class), (const char **) &(class_names_table[1]), ac_bitCopy, sizeof(struct class_names)},
    {"npcact", SPOFFSET(act), (const char **) action_bits, ac_bitCopy, sizeof(flagDef)},
    {"npcact2", SPOFFSET(act2), (const char **) action2_bits, ac_bitCopy, sizeof(flagDef)},
    {"aggro", NPOFFSET(aggro_flags), (const char **) aggro_bits, ac_bitCopy, sizeof(flagDef)},
    {"aggro2", NPOFFSET(aggro2_flags), (const char **) aggro2_bits, ac_bitCopy, sizeof(flagDef)},
    {"aggro3", NPOFFSET(aggro3_flags), (const char **) aggro3_bits, ac_bitCopy, sizeof(flagDef)},
    /* char_skill_data */
    {"skill", OFFSET_OF(struct char_skill_data *, learned), (const char **) spells, ac_skillCopy, sizeof(char *)},
    {"taught", OFFSET_OF(struct char_skill_data *, taught), (const char **) spells, ac_skillCopy, sizeof(char *)},
    /* only.npc */
    {"ldir", NPOFFSET(last_direction), dirs, ac_sbyteCopy, sizeof(char *)},
    {"attack", NPOFFSET(attack_type), NULL, ac_sbyteCopy},
    {"val0", NPOFFSET(value[0]), NULL, ac_intCopy},
    {"val1", NPOFFSET(value[1]), NULL, ac_intCopy},
    {"val2", NPOFFSET(value[2]), NULL, ac_intCopy},
    {"val3", NPOFFSET(value[3]), NULL, ac_intCopy},
    {"val4", NPOFFSET(value[4]), NULL, ac_intCopy},
    {"val5", NPOFFSET(value[5]), NULL, ac_intCopy},
    {"val6", NPOFFSET(value[6]), NULL, ac_intCopy},
    {"val7", NPOFFSET(value[7]), NULL, ac_intCopy},
    // only.pc
    {"frags", PCOFFSET(frags), NULL, ac_longCopy},
    {"epics", PCOFFSET(epics), NULL, ac_longCopy},
    {"epic_skill_points", PCOFFSET(epic_skill_points), NULL, ac_longCopy},
    {"prestige", PCOFFSET(prestige), NULL, ac_shintCopy},
    {"time_left_guild", PCOFFSET(time_left_guild), NULL, ac_longCopy},
    {"nb_left_guild", PCOFFSET(nb_left_guild), NULL, ac_sbyteCopy},
    {"language", 0, language_names, ac_tongueCopy, sizeof(char *)},
    {"echo", PCOFFSET(echo_toggle), NULL, ac_ubyteCopy},
    {"prompt", PCOFFSET(prompt), NULL, ac_shortCopy},
    {"screensize", PCOFFSET(screen_length), NULL, ac_ubyteCopy},
    {"winvis", PCOFFSET(wiz_invis), NULL, ac_sbyteCopy},
    {"law_flags", PCOFFSET(law_flags), player_law_flags, ac_longCopy, sizeof(char *)},
    {"wimpy", PCOFFSET(wimpy), NULL, ac_shortCopy},
    {"aggr", PCOFFSET(aggressive), NULL, ac_shortCopy},
    {"balc", PCOFFSET(spare1), NULL, ac_intCopy},
    {"bals", PCOFFSET(spare2), NULL, ac_intCopy},
    {"balg", PCOFFSET(spare3), NULL, ac_intCopy},
    {"balp", PCOFFSET(spare4), NULL, ac_intCopy},
    {"deaths", PCOFFSET(numb_deaths), NULL, ac_longCopy},
    {"heaven", PCOFFSET(pc_timer[PC_TIMER_HEAVEN]), NULL, ac_timerCopy}
  };

  P_char ppl;

  if( (ppl = get_char_vis(ch, name)) == NULL )
  {
    send_to_char("No one by that name here.\r\n", ch);
    return;
  }

  // only level forger+ can setbit players (outside of themselves)
  if( IS_PC(ppl) && ppl != ch
    && ((GET_LEVEL(ch) < FORGER) || (GET_LEVEL(ch) <= GET_LEVEL(ppl))) )
  {
    // Allowing Ashyel, my admin assistant, to be Greater God and setbit.
    // Note: she still has to be Greater God to use setbit at all.
    if( strcmp(ch->player.name, "Ashyel") || GET_LEVEL(ch) <= GET_LEVEL(ppl) )
    {
      send_to_char("Nope, you are too wimpy to affect them.\r\n", ch);
      return;
    }
    else
    {
      debug( "Ashyel: setbit_char '%s' '%s' '%s' '%d'.", name, flag, val, on_off );
      logit(LOG_WIZ, "Ashyel: setbit_char '%s' '%s' '%s' '%d'.", name, flag, val, on_off );
    }
  }

  /* bleah, have to make exceptions now that mobs/pcs aren't the same */
  if (flag)
  {
    if (SAME_STRING(flag, "winvis") && GET_LEVEL(ch) <= 59)
    {
      send_to_char("You can't do that.\r\n", ch);
      return;
    }
    if (SAME_STRING(flag, "race"))
    {
#ifdef NEW_COMBAT
      FREE((char *) ppl->points.location_hit);
      ppl->points.location_hit = NULL;
#endif

      setbit_parseTable(ch, (void *) ppl, table, ARRAY_SIZE(table), flag, val, on_off, SETBIT_CHAR);

      setCharPhysTypeInfo(ppl);

      do_restore(ch, GET_NAME(ppl), 0);

      set_char_size(ppl);
      balance_affects(ppl);
      do_save_silent(ppl, 1);   /* to make it stick */
    }
    if (ppl)
    {
      if (SAME_STRING(flag, "level") || SAME_STRING(flag, "secondary_level"))
      {
        if( IS_PC(ppl) && GET_LEVEL(ch) < OVERLORD && atoi(val) >= MINLVLIMMORTAL )
        {
          send_to_char("You aren't allowed to set the level that high.\r\n", ch);
          return;
        }
        if( IS_PC(ppl) && (atoi(val) > MAXLVL) )
        {
          send_to_char("You can't setbit someone's level above 62.  Changing value to 62.\r\n", ch);
          sprintf(val, "62");
        }
      }
      if (SAME_STRING(flag, "winvis") &&
          (atoi(val) >= MIN(60, GET_LEVEL(ch))))
      {
        send_to_char("Can't set it that high.\r\n", ch);
        return;
      }
    }
    if (SAME_STRING(flag, "played"))
    {
#ifndef EQ_WIPE
      ppl->player.time.played = 3600 * (atoi(val));
#else
      ppl->player.time.played = 3600 * (atoi(val)) + EQ_WIPE;
#endif
      return;
    }
    if (SAME_STRING(flag, "ldir") || SAME_STRING(flag, "defpos") ||
        SAME_STRING(flag, "attack") || SAME_STRING(flag, "memory") ||
        SAME_STRING(flag, "aggro") || SAME_STRING(flag, "aggro2") ||
        SAME_STRING(flag, "aggro3") ||
        SAME_STRING(flag, "val0") || SAME_STRING(flag, "val1") ||
        SAME_STRING(flag, "val2") || SAME_STRING(flag, "val3") ||
        SAME_STRING(flag, "val4") || SAME_STRING(flag, "val5") ||
        SAME_STRING(flag, "val6") || SAME_STRING(flag, "val7") )
    {
      if (IS_PC(ppl))
      {
        send_to_char("PCs do not have this field.\r\n", ch);
        return;
      }
      else
      {
        setbit_parseTable(ch, (void *) ppl->only.npc, table, ARRAY_SIZE(table), flag, val, on_off, SETBIT_CHAR);
        return;
      }
    }
    else if (SAME_STRING(flag, "echo") || SAME_STRING(flag, "screensize") || SAME_STRING(flag, "prompt")
      || SAME_STRING(flag, "law_flags") || SAME_STRING(flag, "winvis") || SAME_STRING(flag, "wimpy")
      || SAME_STRING(flag, "aggr") || SAME_STRING(flag, "balp") || SAME_STRING(flag, "balg")
      || SAME_STRING(flag, "bals") || SAME_STRING(flag, "balc") || SAME_STRING(flag, "lesson")
      || SAME_STRING(flag, "frags") || SAME_STRING(flag, "epics") || SAME_STRING(flag, "epic_skill_points")
      || SAME_STRING(flag, "prestige") || SAME_STRING(flag, "time_left_guild") || SAME_STRING(flag, "nb_left_guild")
      || SAME_STRING(flag, "deaths") || SAME_STRING(flag, "heaven") )
    {
      if (IS_NPC(ppl))
      {
        send_to_char("NPCs do not have this field.\r\n", ch);
        return;
      }
      else
      {
        if( SAME_STRING(flag, "heaven") && !(ppl->in_room == real_room0( GOOD_HEAVEN_ROOM )
          || ppl->in_room == real_room0( EVIL_HEAVEN_ROOM ) || ppl->in_room == real_room0( UNDEAD_HEAVEN_ROOM )
          || ppl->in_room == real_room0( NEUTRAL_HEAVEN_ROOM )) )
        {
          char_from_room( ppl );
          if( IS_RACEWAR_GOOD(ppl) )
            char_to_room( ppl, real_room0(GOOD_HEAVEN_ROOM), -2 );
          else if( IS_RACEWAR_EVIL(ppl) )
            char_to_room( ppl, real_room0(EVIL_HEAVEN_ROOM), -2 );
          else if( IS_RACEWAR_UNDEAD(ppl) )
            char_to_room( ppl, real_room0(UNDEAD_HEAVEN_ROOM), -2 );
          else if( IS_RACEWAR_NEUTRAL(ppl) )
            char_to_room( ppl, real_room0(NEUTRAL_HEAVEN_ROOM), -2 );
          else
          {
            char_to_room( ppl, real_room0(ppl->specials.was_in_room), -2 );
            act("Could not find heaven room for $N.. aborting", TRUE, ch, 0, ppl, TO_CHAR);
            return;
          }
          send_to_char( "You feel your body pulled through reality by your soul...\n", ppl );
          new_look( ppl, "", CMD_LOOK, ppl->in_room );
        }
        setbit_parseTable(ch, (void *) ppl->only.pc, table, ARRAY_SIZE(table), flag, val, on_off, SETBIT_CHAR);
        return;
      }
    }
  }
  setbit_parseTable(ch, (void *) ppl, table, ARRAY_SIZE(table), flag, val, on_off, SETBIT_CHAR);

  if (IS_NPC(ppl))
    set_npc_multi(ppl);

  if (SAME_STRING(flag, "mxmana"))
  {
    if (GET_MANA(ppl) < GET_MAX_MANA(ppl))
      GET_MANA(ppl) = GET_MAX_MANA(ppl);
  }
  balance_affects(ppl);

  if( IS_PC(ppl) )
    update_skills(ppl);

  do_save_silent(ppl, 1);       /* to make it stick */
}

static void setbit_ship(P_char ch, char *name, char *flag, char *val, int on_off)
{
  /* Internal Macros */

#undef  OFFSET
#define OFFSET(Field)   OFFSET_OF(P_ship, Field)

#define AO(N, Field)    OFFSET(affected[N]. Field)

  /* Table */

  SetBitTable table[] = {
    /*    {"item", OFFSET(R_num), NULL, ac_shintCopy},*/

    {"mxarmor0", OFFSET(maxarmor[0]), NULL, ac_intCopy},
    {"mxarmor1", OFFSET(maxarmor[1]), NULL, ac_intCopy},
    {"mxarmor2", OFFSET(maxarmor[2]), NULL, ac_intCopy},
    {"mxarmor3", OFFSET(maxarmor[3]), NULL, ac_intCopy},
    {"armor0", OFFSET(armor[0]), NULL, ac_intCopy},
    {"armor1", OFFSET(armor[1]), NULL, ac_intCopy},
    {"armor2", OFFSET(armor[2]), NULL, ac_intCopy},
    {"armor3", OFFSET(armor[3]), NULL, ac_intCopy},
    {"mxintern0", OFFSET(maxinternal[0]), NULL, ac_intCopy},
    {"mxintern1", OFFSET(maxinternal[1]), NULL, ac_intCopy},
    {"mxintern2", OFFSET(maxinternal[2]), NULL, ac_intCopy},
    {"mxintern3", OFFSET(maxinternal[3]), NULL, ac_intCopy},
    {"intern0", OFFSET(internal[0]), NULL, ac_intCopy},
    {"intern1", OFFSET(internal[1]), NULL, ac_intCopy},
    {"intern2", OFFSET(internal[2]), NULL, ac_intCopy},
    {"intern3", OFFSET(internal[3]), NULL, ac_intCopy},
    {"sail", OFFSET(mainsail), NULL, ac_intCopy},
    {"money", OFFSET(money), NULL, ac_intCopy},
    {"frags", OFFSET(frags), NULL, ac_intCopy},
    // entries below are just for help, they arent used in parsetable
    {"maxspeed", 0, NULL, ac_intCopy},
    {"capacity", 0, NULL, ac_intCopy},
    {"air", 0, NULL, ac_intCopy},
    {"crew", 0, NULL, ac_intCopy},
    {"chief", 0, NULL, ac_intCopy},
    {"clearchiefs", 0, NULL, ac_intCopy},
    {"sailskill", 0, NULL, ac_intCopy},
    {"gunskill", 0, NULL, ac_intCopy},
    {"repairskill", 0, NULL, ac_intCopy},
    {"stamina", 0, NULL, ac_intCopy}
  };


  /* Local Variable */

  P_ship    ship;

  /* Executable Section */

  if ((ship = get_ship_from_owner(name)) == NULL)
  {
    send_to_char("No ship by that name here.\r\n", ch);
    return;
  }
  if (SAME_STRING(flag, "air"))
  {
    if (IS_SET(ship->flags, AIR))
      REMOVE_BIT(ship->flags, AIR);
    else
      SET_BIT(ship->flags, AIR);
    update_ship_status(ship);
    return;
  }
  if (SAME_STRING(flag, "crew"))
  {
    set_crew(ship, atoi(val), true);
    update_ship_status(ship);
    return;
  }
  if (SAME_STRING(flag, "chief"))
  {
    set_chief(ship, atoi(val));
    update_ship_status(ship);
    return;
  }
  if (SAME_STRING(flag, "clearchiefs"))
  {
    ship->crew.sail_chief = NO_CHIEF;
    ship->crew.guns_chief = NO_CHIEF;
    ship->crew.rpar_chief = NO_CHIEF;
    update_ship_status(ship);
    return;
  }
  if (SAME_STRING(flag, "sailskill"))
  {
    ship->crew.sail_skill = atoi(val);
    update_ship_status(ship);
    return;
  }
  if (SAME_STRING(flag, "gunskill"))
  {
    ship->crew.guns_skill = atoi(val);
    update_ship_status(ship);
    return;
  }
  if (SAME_STRING(flag, "repairskill"))
  {
    ship->crew.rpar_skill = atoi(val);
    update_ship_status(ship);
    return;
  }
  if (SAME_STRING(flag, "capacity"))
  {
    ship->capacity_bonus += (atoi(val) - ship->get_capacity());
    return;
  }
  if (SAME_STRING(flag, "maxspeed"))
  {
    ship->maxspeed_bonus += (atoi(val) - ship->get_maxspeed());
    return;
  }
  if (SAME_STRING(flag, "stamina"))
  {
    ship->crew.stamina = atoi(val);
    return;
  }

  setbit_parseTable(ch, (void *) ship, table, ARRAY_SIZE(table), flag, val, on_off, SETBIT_SHIP);
}

static void setbit_obj(P_char ch, char *name, char *flag, char *val, int on_off)
{
  /* Internal Macros */

#undef  OFFSET
#define OFFSET(Field)   OFFSET_OF(P_obj, Field)

#define AO(N, Field)    OFFSET(affected[N]. Field)

  /* Table */

  SetBitTable table[] = {
    /*    {"item", OFFSET(R_num), NULL, ac_shintCopy},*/
    {"wear", OFFSET(wear_flags), (const char **) wear_bits, ac_bitCopy, sizeof(flagDef)},
    {"extra", OFFSET(extra_flags), (const char **) extra_bits, ac_bitCopy, sizeof(flagDef)},
    {"class", OFFSET(anti_flags), (const char **) &(class_names_table[1]), ac_bitCopy, sizeof(struct class_names),
      OFFSET_OF(struct class_names *, normal)},
    {"race", OFFSET(anti2_flags), (const char **) &(race_names_table[1]), ac_bitCopy, sizeof(struct race_names),
      OFFSET_OF(struct race_names *, no_spaces)},
    {"extra2", OFFSET(extra2_flags), (const char **) extra2_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff", OFFSET(bitvector), (const char **) affected1_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff2", OFFSET(bitvector2), (const char **) affected2_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff3", OFFSET(bitvector3), (const char **) affected3_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff4", OFFSET(bitvector4), (const char **) affected4_bits, ac_bitCopy, sizeof(flagDef)},
    {"aff5", OFFSET(bitvector5), (const char **) affected5_bits, ac_bitCopy, sizeof(flagDef)},
    {"val0", OFFSET(value[0]), NULL, ac_intCopy},
    {"val1", OFFSET(value[1]), NULL, ac_intCopy},
    {"val2", OFFSET(value[2]), NULL, ac_intCopy},
    {"val3", OFFSET(value[3]), NULL, ac_intCopy},
    {"val4", OFFSET(value[4]), NULL, ac_intCopy},
    {"val5", OFFSET(value[5]), NULL, ac_intCopy},
    {"val6", OFFSET(value[6]), NULL, ac_intCopy},
    {"val7", OFFSET(value[7]), NULL, ac_intCopy},
    {"type", OFFSET(type), (const char **) item_types, ac_byteCopy, sizeof(char *)},
    {"material", OFFSET(material), (const char **) item_material, ac_shintCopy, sizeof(char *)},
    {"weight", OFFSET(weight), NULL, ac_intCopy},
    {"price", OFFSET(cost), NULL, ac_intCopy},
    {"condition", OFFSET(condition), NULL, ac_shintCopy},
    {"a0mod", AO(0, modifier), NULL, ac_sbyteCopy},
    {"a1mod", AO(1, modifier), NULL, ac_sbyteCopy},
    {"a2mod", AO(2, modifier), NULL, ac_sbyteCopy},
    {"a3mod", AO(3, modifier), NULL, ac_sbyteCopy},
    {"a0loc", AO(0, location), (const char **) apply_types, ac_objaffCopy, sizeof(char *)},
    {"a1loc", AO(1, location), (const char **) apply_types, ac_objaffCopy, sizeof(char *)},
    {"a2loc", AO(2, location), (const char **) apply_types, ac_objaffCopy, sizeof(char *)},
    {"a3loc", AO(3, location), (const char **) apply_types, ac_objaffCopy, sizeof(char *)}
  };

  /* Local Variable */

  P_obj    obj;

  /* Executable Section */

  if ((obj = get_obj_in_list_vis(ch, name, ch->carrying)) == NULL)
  {
    if ((obj =
          get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) == NULL)
    {
      send_to_char("No object by that name here.\r\n", ch);
      return;
    }
  }
#if 0
  if ((obj = get_obj_vis(ch, name)) == NULL)
  {
    send_to_char("No object by that name here\r\n", ch);
    return;
  }
#endif

  setbit_parseTable(ch, (void *) obj, table, ARRAY_SIZE(table), flag, val, on_off, SETBIT_OBJ);

  if (OBJ_WORN(obj))
  {
    char_light(obj->loc.wearing);
    room_light(obj->loc.wearing->in_room, REAL);
  }
  if (OBJ_CARRIED(obj))
  {
    char_light(obj->loc.carrying);
    room_light(obj->loc.carrying->in_room, REAL);
  }
  if (OBJ_ROOM(obj))
    room_light(obj->loc.room, REAL);
}

/*
 * Note that this cannot really be combined together with **
 * setbit_room because this requires a pointer to a room_direction **
 * structure.
 */
static void setbit_dir(P_char ch, char *name, char *flag, char *value, int on_off)
{

  /* Internal Macros */

#undef  OFFSET
#define OFFSET(Field)   OFFSET_OF(struct room_direction_data *, Field)

#undef  DIR
#define DIR(D, Field)   OFFSET(Field)

  /* Extern Variables */

  /* Table */

  SetBitTable table[] = {
    {"ninfo", DIR(NORTH, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"einfo", DIR(EAST, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"sinfo", DIR(SOUTH, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"winfo", DIR(WEST, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"uinfo", DIR(UP, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"dinfo", DIR(DOWN, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"nwinfo", DIR(NORTHWEST, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"neinfo", DIR(NORTHEAST, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"swinfo", DIR(SOUTHWEST, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"seinfo", DIR(SOUTHEAST, exit_info), exit_bits, ac_bitCopy, sizeof(char *)},
    {"nkey", DIR(NORTH, key), NULL, ac_shintCopy},
    {"ekey", DIR(EAST, key), NULL, ac_shintCopy},
    {"skey", DIR(SOUTH, key), NULL, ac_shintCopy},
    {"wkey", DIR(WEST, key), NULL, ac_shintCopy},
    {"ukey", DIR(UP, key), NULL, ac_shintCopy},
    {"dkey", DIR(DOWN, key), NULL, ac_shintCopy},
    {"nwkey", DIR(NORTHWEST, key), NULL, ac_shintCopy},
    {"nekey", DIR(NORTHEAST, key), NULL, ac_shintCopy},
    {"swkey", DIR(SOUTHWEST, key), NULL, ac_shintCopy},
    {"sekey", DIR(SOUTHEAST, key), NULL, ac_shintCopy},
    {"nroom", DIR(NORTH, to_room), NULL, ac_intCopy},
    {"eroom", DIR(EAST, to_room), NULL, ac_intCopy},
    {"sroom", DIR(SOUTH, to_room), NULL, ac_intCopy},
    {"wroom", DIR(WEST, to_room), NULL, ac_intCopy},
    {"uroom", DIR(UP, to_room), NULL, ac_intCopy},
    {"droom", DIR(DOWN, to_room), NULL, ac_intCopy},
    {"nwroom", DIR(NORTHWEST, to_room), NULL, ac_intCopy},
    {"neroom", DIR(NORTHEAST, to_room), NULL, ac_intCopy},
    {"swroom", DIR(SOUTHWEST, to_room), NULL, ac_intCopy},
    {"seroom", DIR(SOUTHEAST, to_room), NULL, ac_intCopy}
  };

  /*
   * Local Variable
   */

  P_room   room;
  int      room_number;
  void    *where = NULL;

  /*
   * Executable section
   */

  if( !strcmp(name, "here") )
  {
    room_number = ch->in_room;
  }
  else
  {
    if( is_number(name) )
    {
      room_number = real_room(atoi(name));
    }
    else
    {
      room_number = -1;
    }
  }

  if( room_number < 0 || room_number > top_of_world )
  {
    setbit_syntax(ch, SETBIT_DIR);
    send_to_char("Invalid room number.  Please enter a virtual number or 'here' for the room you are in.\n\r", ch );
    return;
  }
  room = &world[room_number];

  /*
   * Using first letter to differentiate between
   */
  /*
   * which room_direction pointer to use
   */

  bool bIsSetRoom = false;
  switch( *flag )
  {

    case 'n':
    case 'N':
      switch (toupper(flag[1]))
      {
        case 'W':
          where = room->dir_option[NORTHWEST];
          if (toupper(flag[2]) == 'R') bIsSetRoom = true;
          break;
        case 'E':
          where = room->dir_option[NORTHEAST];
          if (toupper(flag[2]) == 'R') bIsSetRoom = true;
          break;
        default:
          if (toupper(flag[1]) == 'R') bIsSetRoom = true;
          where = room->dir_option[NORTH];
          break;
      }
      break;

    case 'e':
    case 'E':
      where = room->dir_option[EAST];
      if( toupper(flag[1]) == 'R' )
      {
        bIsSetRoom = true;
      }
      break;

    case 'w':
    case 'W':
      where = room->dir_option[WEST];
      if (toupper(flag[1]) == 'R')
      {
        bIsSetRoom = true;
      }
      break;

    case 's':
    case 'S':
      switch (toupper(flag[1]))
      {
        case 'W':
          where = room->dir_option[SOUTHWEST];
          if (toupper(flag[2]) == 'R')
          {
            bIsSetRoom = true;
          }
          break;
        case 'E':
          where = room->dir_option[SOUTHEAST];
          if (toupper(flag[2]) == 'R')
          {
            bIsSetRoom = true;
          }
          break;
        default:
          if (toupper(flag[1]) == 'R')
          {
            bIsSetRoom = true;
          }
          where = room->dir_option[SOUTH];
          break;
      }
      break;

    case 'u':
    case 'U':
      where = room->dir_option[UP];
      if (toupper(flag[1]) == 'R') bIsSetRoom = true;
      break;

    case 'd':
    case 'D':
      where = room->dir_option[DOWN];
      if (toupper(flag[1]) == 'R') bIsSetRoom = true;
      break;

    default:
      *flag = 'g';                /*
      * To generate syntax error
      */
      break;
  }

  if( !where )
  {
    setbit_syntax(ch, SETBIT_DIR);
    send_to_char( "Invalid direction.  The prefix to the flag must be a valid dir (n/s/e/w/nw/ne/se/sw/u/d).\n\r", ch );
    send_to_char( "The rest of the flag can be one of: info, key, room.\n\r", ch );
    setbit_printOutTable(ch, table, ARRAY_SIZE(table));
    return;
  }

  // convert a numerical from a vroom num to a real room num
  if( bIsSetRoom )
  {
    if (-1 == atoi(value))
      room_number = -1;
    else
    {
      room_number = real_room(atoi(value));
      if (room_number < 0 || room_number > top_of_world)
      {
        // invalid room number.. ignore it
        return;
      }
    }
    sprintf(value, "%d", room_number);
  }

  setbit_parseTable(ch, (void *) where, table, ARRAY_SIZE(table), flag, value, on_off, SETBIT_DIR);
}

/*
 * ** Changes affect structure.
 */
static void setbit_aff(P_char ch, char *name, char *flag, char *value, int on_off)
{
  /*
   * Internal Macros
   */

#undef OFFSET
#define OFFSET(Field)   OFFSET_OF(struct affected_type *, Field)

  /*
   * Extern Variables
   */

  /*
   * Table
   */
  // Note: uint flags isn't represeted here.  You'd have to make a table for lookup.
  //   using AFFTYPE_* in structs.h.
  SetBitTable table[] = {
    {"type", OFFSET(type), spells, ac_affModify, sizeof(char *)},
    {"dur", OFFSET(duration), NULL, ac_intCopy},
    {"mod", OFFSET(modifier), NULL, ac_intCopy},
    {"loc", OFFSET(location), apply_types, ac_objaffCopy, sizeof(char *)},
    {"bits", OFFSET(bitvector), (const char **) affected1_bits, ac_bitCopy, sizeof(flagDef)},
    {"bits2", OFFSET(bitvector2), (const char **) affected2_bits, ac_bitCopy, sizeof(flagDef)},
    {"bits3", OFFSET(bitvector3), (const char **) affected3_bits, ac_bitCopy, sizeof(flagDef)},
    {"bits4", OFFSET(bitvector4), (const char **) affected4_bits, ac_bitCopy, sizeof(flagDef)},
    {"bits5", OFFSET(bitvector5), (const char **) affected5_bits, ac_bitCopy, sizeof(flagDef)}
  };

  /*
   * Local Variable
   */

  P_char   ppl;
  struct affected_type *af;
  char    *af_num_str;
  int      af_num, i;

  /*
   * Executable section
   */

  /*
   * Since name and affect number are together, we need to
   */
  /*
   * separate them.
   */

  /*
   * First get affect number
   */

  for (af_num_str = name;
      !isdigit(*af_num_str) && *af_num_str != '\0'; af_num_str++) ;

  if (*af_num_str == '\0')
  {
    send_to_char("Affect number should follow name immediately (no space)\r\n", ch);
    return;
  }
  af_num = atoi(af_num_str);

  /*
   * Now, get player
   */

  *af_num_str = '\0';

  if ((ppl = get_char_vis(ch, name)) == NULL)
  {
    send_to_char("No one by that name here.\r\n", ch);
    *af_num_str = '0';
    return;
  }
  else
  {
    *af_num_str = '0';
  }

  /*
   * Now, find the affect structure
   */

  for( i = 0, af = ppl->affected; i != af_num && af != NULL; i++, af = af->next ) ;

  if (af == NULL)
  {
    send_to_char("Affect number specified is too large.\r\n", ch);
    return;
  }
  setbit_parseTable(ch, (void *) af, table, ARRAY_SIZE(table), flag, value, on_off, SETBIT_AFF);
}

/*
 * This function uses a "parse table" and tries to match "flag" with
 * an entry in "parse table".  If a match is found, parameters "ptr",
 * "value", and "on_off" are passed to function specified in table.  If
 * not found, prints out valid string parsable by table and return.
 */
static void setbit_parseTable(P_char ch, void *ptr, SetBitTable * table,
    int size, char *flag, char *value, int on_off, int type)
{
  int      i, bit;
  SetBitTable *entry;
  char    *string;

  for (i = 0; i < size && !SAME_STRING(flag, table[i].sb_flag); i++) ;

  if (i == size)
  {
    setbit_printOutTable(ch, table, size);
    setbit_syntax(ch, type);
    return;
  }
  /* OK .. found entry -- If sb_subtable is not NULL, then, we */
  /* convert "value" to bit number */

  entry = table + i;

  if( entry->sb_subtable )
  {
    for( bit = 0; (string = *(char **) (((char *) entry->sb_subtable)
      + entry->entry_size * bit + entry->entry_offset)) != NULL && string[0] != '\n'; bit++)
    {
      if( SAME_STRING(string, value) )
      {
        break;
      }
    }
    if( string == NULL || string[0] == '\n' )
    {
      setbit_printOutSubTable(ch, entry->sb_subtable, entry->entry_size);
      return;
    }
  }
  else
  {
    /* Otherwise, convert value to actual integer. */
    if( is_number( value ) )
    {
      bit = atoi(value);
    }
    else
    {
      char buf[128];

      setbit_syntax(ch, type);
      sprintf( buf, "'%s' is not a valid value.  Please enter a number instead.\n\r", value );
      send_to_char( buf, ch );
      return;
    }
  }

  if( bad_on_off[0] != '\0' )
  {
    if( entry->sb_func == ac_bitCopy || entry->sb_func == ac_tongueCopy )
    {
      char buf[128];

      sprintf( buf, "'%s' is not a valid value.  Please enter a number instead.\n\r", bad_on_off );
      send_to_char( buf, ch );
      return;
    }
  }
  if( (entry->sb_func == ac_tongueCopy || entry->sb_func == ac_skillCopy)
    && (on_off < 0 || on_off > 100) )
  {
    char buf[128];

    sprintf( buf, "'%d' is not a valid value.  Please enter a number between 0 and 100.\n\r", on_off );
    send_to_char( buf, ch );
    return;
  }

  /* Now call copy function */
  (*(entry->sb_func)) (ptr, entry->sb_offset, value, bit, on_off);
}

/* This function should be called when there is a high-level error. */
static void setbit_syntax(P_char ch, int type)
{
  if( type < -1 || type > SETBIT_MAX )
  {
    send_to_char( "Invalid type!\n\r", ch);
    type = -1;
  }
  send_to_char("Usage:\r\n", ch);
  if( type == SETBIT_ROOM || type == -1 )
  {
    send_to_char("setbit room room-number flag-type value [on|off]\r\n", ch);
  }
  if( type == SETBIT_CHAR || type == -1 )
  {
    send_to_char("setbit <char|mob> name flag-type value [on|off]\r\n", ch);
  }
  if( type == SETBIT_OBJ || type == -1 )
  {
    send_to_char("setbit obj name flag-type value [on|off]\r\n", ch);
  }
  if( type == SETBIT_DIR || type == -1 )
  {
    send_to_char("setbit dir room-number flag-type value\r\n", ch);
  }
  if( type == SETBIT_AFF || type == -1 )
  {
    send_to_char("setbit aff name[aff-number] flag value\r\n", ch);
  }
  if( type == SETBIT_ZONE || type == -1 )
  {
    send_to_char("setbit zone zone# flag value\r\n", ch);
  }
  if( type == SETBIT_SHIP || type == -1 )
  {
    send_to_char("setbit ship name flag value\r\n", ch);
  }
}

/* This function should be called when "flag" specified is unknown. */
static void setbit_printOutTable(P_char ch, SetBitTable * table, int size)
{
  int      i;
  char     buff[128];

  send_to_char("Valid flags are:\r\n", ch);

  for (i = 0; i < size; i++)
  {

    if (i && !(i % 3))
      send_to_char("\r\n", ch);

    sprintf(buff, "%-20s", table[i].sb_flag);
    page_string(ch->desc, buff, 1);
  }

  send_to_char("\r\n", ch);
}

/* If specified is not in subtable, this function should be called. */
static void setbit_printOutSubTable(P_char ch, const char **subtable, int entry_size)
{
  int      i;
  char     buff[128];
  char    *string;

  send_to_char("Valid sub-options are:\r\n", ch);

  for( i = 0; (string = *(char **) (((char *) subtable) + entry_size * i)) != NULL && string[0] != '\n'; i++ )
  {

    if (!(i % 3))
      send_to_char("\r\n", ch);

    sprintf(buff, "%-20s", string);
    page_string(ch->desc, buff, 1);
  }
  send_to_char("\r\n", ch);
}

/* Basically same as "one_argument" except handles quote */
char    *setbit_parseArgument(char *arg, char *val)
{
  char    *start, *end;

  /* Skip initial space */

  while (*arg == ' ' && *arg != '\0')
    arg++;

  /* Check to see if single quote .. if not, call one_argument */
  /* to do right thing */

  if (*arg != '\'')
  {
    return one_argument(arg, val);
  }
  /* Otherwise .. have single quote.  We modify the terminating */
  /* single quote to '\0', copy arg into val, and change terminating */
  /* '\0' back to quote */

  end = start = arg + 1;

  while (*end != '\'' && *end != '\0')
    end++;

  if (*end == '\0')
  {                             /* Daggling single quote */
    *val = '\0';
    return NULL;
  }
  *end = '\0';

  strcpy(val, start);
  *end = '\'';

  return end + 1;
}

/* Case insensitive comparison */
static int ac_strcasecmp(const char *str1, const char *str2)
{
  char     low1, low2;
  int      i;

  for (i = 0;; i++)
  {

    low1 = LOWER_CASE(*(str1 + i));
    low2 = LOWER_CASE(*(str2 + i));

    if ((low1 != low2) || (low1 == '\0'))
      break;
  }

  return ((low1 == '\0') && (low2 == '\0'));
}

/* Copy functions */

/* Macro to facilitate making of general copy functions */
#define MAKE_COPY_FUNCTION(Type)                                        \
static void ac_ ## Type ## Copy(void *where, int offset, char *value, int bit, int on_off) \
{                                                                       \
  Type val = (Type) bit;                                                \
  bcopy((char *) &val, (char *) where + offset, sizeof(val));           \
}

/* General copy functions */
  MAKE_COPY_FUNCTION(byte)
  MAKE_COPY_FUNCTION(int)
  MAKE_COPY_FUNCTION(long)
  MAKE_COPY_FUNCTION(short)
  MAKE_COPY_FUNCTION(sbyte)
  MAKE_COPY_FUNCTION(sh_int)
  MAKE_COPY_FUNCTION(ubyte)
  /* Specifical copy functions */
  /* For age, we set only the year using by finding the difference
     between current MUD time and intended age.  */
static void ac_ageCopy(void *where, int offset, char *value, int bit, int on_off)
{
  long     secs;
  time_t   curr_time = time(NULL);
  P_char   ch = (P_char) where;

  secs = (bit /* - 17 */ ) * SECS_PER_MUD_YEAR;
  ch->player.time.birth = curr_time - secs;
}

/* Use on_off to determine whether to set/clr which bit */
static void ac_bitCopy(void *where, int offset, char *value, int bit, int on_off)
{
  long     orig_bits;

  while (offset > 31)
  {
    offset -= 32;
    where = (char *) where + 32;
  }
  bcopy((char *) where + offset, (char *) &orig_bits, sizeof(orig_bits));

  if (on_off)
    orig_bits |= 1 << bit;
  else
    orig_bits &= ~(1 << bit);

  bcopy((char *) &orig_bits, (char *) where + offset, sizeof(orig_bits));
}

static void ac_idx2flagCopy(void *where, int offset, char *value, int bit, int on_off)
{
  long     flag;

  flag = 1 << (bit - 1);
  bcopy((char *) &flag, (char *) where + offset, sizeof(long));
}

static void ac_positionCopy(void *where, int offset, char *value, int bit, int on_off)
{
  P_char   ch = (P_char) where;

  if (bit < 4)
    SET_POS(ch, (bit - 1) + GET_STAT(ch));
  else
    SET_POS(ch, GET_POS(ch) + (1 << (bit - 2)));;
}

static void ac_tongueCopy(void *where, int offset, char *value, int bit, int on_off)
{
  P_char   ch = (P_char) where;

  if (IS_PC(ch))
    GET_LANGUAGE(ch, bit + 1) = (byte) on_off;
}

/*
 * ** Saving throw is done by assuming value is in the form ** "A B C D E"
 */
static void ac_savthrCopy(void *where, int offset, char *value, int bit, int on_off)
{
  sh_int   sav_thr[5];
  P_char   ch = (P_char) where;

  sscanf(value, "%hd %hd %hd %hd %hd", sav_thr, sav_thr + 1, sav_thr + 2,
      sav_thr + 3, sav_thr + 4);

  bcopy((char *) &sav_thr, (char *) ch->specials.apply_saving_throw,
      sizeof(sav_thr));
}

/*
 * ** For skill, "bit" is the actual skill number to practice - 1. ** And
 * "on_off" indicates how learn a skill is.
 */
static void ac_skillCopy(void *where, int offset, char *value, int bit, int on_off)
{
  P_char   ch = (P_char) where;
  int      skl;
  byte     val = on_off;

  value = skip_spaces(value);
  skl = search_block(value, spells, FALSE);

  if (IS_PC(ch))
  {
    bcopy(&val, ((char *) &ch->only.pc->skills[skl]) + offset, sizeof(byte));
  }
}

/*
 * ** For skill, "bit" is the actual skill number to practice - 1. ** And
 * "on_off" indicates how learn a skill is.
 */
static void ac_affModify(void *where, int offset, char *value, int bit, int on_off)
{
  int      skl;
  struct affected_type *af = (affected_type *)where;

  value = skip_spaces(value);
  skl = search_block(value, spells, FALSE);

  af->type = skl;
}

/*
 * ** "Value" contains the string.
 */
static void ac_stringCopy(void *where, int offset, char *value, int bit, int on_off)
{
  strcpy((char *) where + offset, value);
}

/*
 * ** "bit" is not really a bit to set, but rather, the value of **
 * location.
 */
static void ac_objaffCopy(void *where, int offset, char *value, int bit, int on_off)
{
  byte     location = (byte) bit;

  bcopy((char *) &location, (char *) where + offset, sizeof(location));
}

/*
 * ** For max hit and max mana, the value is modified by graf(...), see **
 * "limits.c", therefore, we want to subtract that from the value **
 * specified, so that when graf(...) value is added back automatically, **
 * it will be ok.
 */

static void ac_hitmanaCopy(void *where, int offset, char *value, int bit, int on_off)
{
  P_char   ch = (P_char) where;
  sh_int   val;

  if (IS_PC(ch))
    val = (sh_int) bit - graf(ch, age(ch).year, 2, 4, 17, 14, 8, 4, 3);
  else
    val = (sh_int) bit;

  bcopy((char *) &val, (char *) where + offset, sizeof(val));
}

// Sets a time_t value.
static void ac_timerCopy( void *where, int offset, char *value, int time_to_zero, int on_off )
{
  // So, we need to add the timer to time(NULL) to get the time when it should end.
  // And we need to adjust offset to handle the size difference between the time_t and char.
  *( (time_t *)(where) + (sizeof( char ) * offset / sizeof( time_t )) ) = time(NULL) + time_to_zero;
}
