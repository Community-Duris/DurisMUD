// Boon system for DurisMUD.
// Redmine Ticket #577 by Seif
// Created April 2011 - Venthix

// TODO:
// in addition to the debug's setup some real logging incase someone completes a boon
//   and gets an error message to contact an imm because the db wouldn't update or
//   create.
// make automatic random boon engine
// finish boon command random controller
// the boon shop
// Add live boon listings to the website.

// To add new boon types or options:
// Add the define to boon.h
// Update boon_types or boon_options with the appropriate messages
// Update boon_data to include the new type or option
// Update parse_boon_args if special circumstances require, especially
//   if the bonus or criteria arguments require words instead of numbers.
// Update validate_boon_data with appropriate checks
// Update check_boon_completion
// Update boon_notify
// If you want it to be included in the random generator instead of only
//   being manually added, update the random_std array to include the new
//   boon_data array number.

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <string>
using namespace std;

#include "boon.h"
#include "interp.h"
#include "structs.h"
#include "sql.h"
#include "utils.h"
#include "utility.h"
#include "prototypes.h"
#include "spells.h"
#include "guildhall.h"
#include "assocs.h"
#include "nexus_stones.h"
#include "buildings.h"
#include "epic.h"
#include "ctf.h"

extern P_desc descriptor_list;
extern P_room world;
extern Skill skills[];
extern struct race_names race_names_table[];
extern P_index mob_index;
extern P_index obj_index;
extern const struct attr_names_struct attr_names[];
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern long new_exp_table[];  // Arih: Fixed type mismatch bug - was int, should be long
extern struct ctfData ctfdata[];

// Max_btype + 1 since we have a null ender.
struct boon_types_struct boon_types[MAX_BTYPE+1] = {
  {"none",  "No bonus exists"},
  {"expm",  "Gain %d%% bonus to exp"},
  {"exp",   "Gain exp"},
  {"epic",  "Gain %d epics"},
  {"cash",  "Receive %s"},
  {"level", "Gain a level"},
  {"power", "Gain the power of '%s'"},
  {"spell", "Gain the spell '%s'"},
  {"stat",  "Notch the %s attribute"},
  {"stats", "Notch %d attributes of your choice"},
  {"point", "Receive %d boon points"},
  {"item",  "Receive '%s'"},
  {"\0"}
};

struct boon_options_struct boon_options[] = {
// RACE		DESC					PROGRESS
  {"none",	"from the zone %s&n.",			0},
  {"zone",	"when you complete the zone %s&n.",	0},
  {"level",	"when you obtain level %d.",		0},
  {"mob",	"when you kill %d %s&n(s).",		1},
  {"race",	"when you kill %d %s&n(s).",		1},
  {"frag",	"when you receive a %.2f frag or better.",	0},
  {"frags",	"when you obtain %.2f frags.",		1},
  {"guildhall",	"when you sack the %s&n guildhall.",	0},
  {"outpost",	"when you capture the %s&n outpost.",	0},
  {"nexus",	"when you capture the %s&n nexus.",	0},
  {"cargo",	"when you sell %d cargo.",		0},
  {"auction",	"when you auction %d equipment.",	0},
  {"ctf",	"when you capture the CTF flag # %d.",	0},
  {"ctfb",	"when you capture the CTF flag # %d.",	0},
  "\0"
};

// level 0 means it can be randomly set, otherwise manually by the listed level.
// Can make more randomly set once the data_validation is updated so we don't end
// up with crazy bonuses for easy accomplishments.
struct boon_data_struct boon_data[] = {
  // Type         Option        Level Requirement
  {BTYPE_EXPM,    BOPT_NONE,    0},           // 0
  {BTYPE_EXPM,    BOPT_RACE,    0},
  {BTYPE_EXPM,    BOPT_MOB,     0},
  {BTYPE_EXP,     BOPT_ZONE,    0},
  {BTYPE_EXP,     BOPT_MOB,     0},
  {BTYPE_EXP,     BOPT_FRAG,    0},
  {BTYPE_EXP,     BOPT_FRAGS,   0},           // 6
  {BTYPE_EXP,     BOPT_LEVEL,   0},
  {BTYPE_EXP,     BOPT_OP,      0},
  {BTYPE_EXP,     BOPT_NEXUS,   0},
  {BTYPE_EXP,     BOPT_CTF,     0},
  {BTYPE_EXP,     BOPT_CTFB,    0},
  {BTYPE_EPIC,    BOPT_ZONE,    0},
  {BTYPE_EPIC,    BOPT_MOB,     0},           // 11
  {BTYPE_EPIC,    BOPT_RACE,    GREATER_G},
  {BTYPE_EPIC,    BOPT_FRAG,    0},
  {BTYPE_EPIC,    BOPT_FRAGS,   0},
  {BTYPE_EPIC,    BOPT_LEVEL,   GREATER_G},
  {BTYPE_EPIC,    BOPT_OP,      0},           // 16
  {BTYPE_EPIC,    BOPT_NEXUS,   0},
  {BTYPE_EPIC,    BOPT_CTF,     0},
  {BTYPE_EPIC,    BOPT_CTFB,    0},
  {BTYPE_CASH,    BOPT_ZONE,    0},
  {BTYPE_CASH,    BOPT_MOB,     0},
  {BTYPE_CASH,    BOPT_RACE,    GREATER_G},
  {BTYPE_CASH,    BOPT_FRAG,    0},           // 21
  {BTYPE_CASH,    BOPT_FRAGS,   0},
  {BTYPE_CASH,    BOPT_LEVEL,   GREATER_G},
  {BTYPE_CASH,    BOPT_OP,      0},
  {BTYPE_CASH,    BOPT_NEXUS,   0},
  {BTYPE_CASH,    BOPT_CTF,     0},
  {BTYPE_CASH,    BOPT_CTFB,    0},
  {BTYPE_LEVEL,   BOPT_ZONE,    GREATER_G},   // 26
  {BTYPE_LEVEL,   BOPT_MOB,     GREATER_G},
  {BTYPE_LEVEL,   BOPT_RACE,    GREATER_G},
  {BTYPE_LEVEL,   BOPT_FRAG,    FORGER},
  {BTYPE_LEVEL,   BOPT_FRAGS,   FORGER},
  {BTYPE_LEVEL,   BOPT_OP,      FORGER},      // 31
  {BTYPE_LEVEL,   BOPT_NEXUS,   FORGER},
  {BTYPE_LEVEL,   BOPT_CTF,     FORGER},
  {BTYPE_LEVEL,   BOPT_CTFB,    FORGER},
  {BTYPE_POWER,   BOPT_ZONE,    GREATER_G},
  {BTYPE_POWER,   BOPT_MOB,     GREATER_G},
  {BTYPE_POWER,   BOPT_FRAG,    GREATER_G},
  {BTYPE_POWER,   BOPT_FRAGS,   GREATER_G},   // 36
  {BTYPE_POWER,   BOPT_OP,      GREATER_G},
  {BTYPE_POWER,   BOPT_NEXUS,   GREATER_G},
  {BTYPE_POWER,   BOPT_CTF,     GREATER_G},
  {BTYPE_POWER,   BOPT_CTFB,    GREATER_G},
  {BTYPE_SPELL,   BOPT_ZONE,    GREATER_G},
  {BTYPE_SPELL,   BOPT_MOB,     GREATER_G},
  {BTYPE_SPELL,   BOPT_FRAG,    GREATER_G},   // 41
  {BTYPE_SPELL,   BOPT_FRAGS,   GREATER_G},
  {BTYPE_SPELL,   BOPT_OP,      GREATER_G},
  {BTYPE_SPELL,   BOPT_NEXUS,   GREATER_G},
  {BTYPE_SPELL,   BOPT_CTF,     GREATER_G},
  {BTYPE_SPELL,   BOPT_CTFB,    GREATER_G},
  {BTYPE_STAT,    BOPT_ZONE,    FORGER},
  {BTYPE_STAT,    BOPT_MOB,     FORGER},      // 46
  {BTYPE_STAT,    BOPT_FRAG,    FORGER},
  {BTYPE_STAT,    BOPT_FRAGS,   FORGER},
  {BTYPE_STAT,    BOPT_OP,      FORGER},
  {BTYPE_STAT,    BOPT_NEXUS,   FORGER},
  {BTYPE_STAT,    BOPT_CTF,     FORGER},
  {BTYPE_STAT,    BOPT_CTFB,    FORGER},
  {BTYPE_STATS,   BOPT_ZONE,    FORGER},      // 51
  {BTYPE_STATS,   BOPT_MOB,     FORGER},
  {BTYPE_STATS,   BOPT_FRAG,    FORGER},
  {BTYPE_STATS,   BOPT_FRAGS,   FORGER},
  {BTYPE_STATS,   BOPT_OP,      FORGER},
  {BTYPE_STATS,   BOPT_NEXUS,   FORGER},      // 56
  {BTYPE_STATS,   BOPT_CTF,     FORGER},
  {BTYPE_STATS,   BOPT_CTFB,    FORGER},
  {BTYPE_POINT,   BOPT_ZONE,    GREATER_G},
  {BTYPE_POINT,   BOPT_MOB,     GREATER_G},
  {BTYPE_POINT,   BOPT_RACE,    GREATER_G},
  {BTYPE_POINT,   BOPT_FRAG,    GREATER_G},
  {BTYPE_POINT,   BOPT_FRAGS,   GREATER_G},   // 61
  {BTYPE_POINT,   BOPT_LEVEL,   GREATER_G},
  {BTYPE_POINT,   BOPT_OP,      GREATER_G},
  {BTYPE_POINT,   BOPT_NEXUS,   GREATER_G},
  {BTYPE_POINT,   BOPT_CTF,     GREATER_G},
  {BTYPE_POINT,   BOPT_CTFB,    GREATER_G},
  {BTYPE_ITEM,    BOPT_ZONE,    GREATER_G},
  {BTYPE_ITEM,    BOPT_MOB,     GREATER_G},
  {BTYPE_ITEM,    BOPT_RACE,    GREATER_G},
  {BTYPE_ITEM,    BOPT_FRAG,    GREATER_G},
  {BTYPE_ITEM,    BOPT_FRAGS,   GREATER_G},
  {BTYPE_ITEM,    BOPT_CTF,     GREATER_G},
  {BTYPE_ITEM,    BOPT_CTFB,    GREATER_G},
  {0}
};

struct BoonRandomStandards random_std[] = {
// ID	Racewar Side 	low	high	boon_data
  {0,	0,		0,	0,	0},
  {1,	RACEWAR_GOOD,	1,	20,	0},
  {2,	RACEWAR_EVIL,	1,	20,	0},
  {0}
};

bool check_boon_combo(int type, int option, int random)
{
  for( int i = 0; boon_data[i].type; i++ )
  {
    if( boon_data[i].type == type && boon_data[i].option == option )
    {
      if( random )
      {
        if( boon_data[i].level )
        {
          return FALSE;
        }
        else
        {
          return TRUE;
        }
      }
      else
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

int get_boon_level(int type, int option)
{
  for (int i = 0; boon_data[i].type; i++)
  {
    if (boon_data[i].type == type &&
	boon_data[i].option == option)
    {
      return boon_data[i].level;
    }
  }
  return 0;
}

int get_valid_boon_type(char *arg)
{
  for (int i = 1; i < MAX_BTYPE; i++)
  {
    if (!strcmp(boon_types[i].type, arg))
      return i;
    else
      continue;
  }
  return -1;
}

int get_valid_boon_option(char *arg)
{
  for (int i = 0; i < MAX_BOPT; i++)
  {
    if (!strcmp(boon_options[i].option, arg))
      return i;
    else
      continue;
  }
  return -1;
}

int is_boon_valid(int id)
{
  if (!qry("SELECT id FROM boons WHERE id = '%d'", id))
    return FALSE;
  else
  {
    MYSQL_RES *res = mysql_store_result(DB);
    if (mysql_num_rows(res) < 1)
    {
      mysql_free_result(res);
      return FALSE;
    }
    else
    {
      mysql_free_result(res);
      return TRUE;
    }
  }  
  return FALSE;
}

int count_boons(int active, int random)
{
  char dbqry[MAX_STRING_LENGTH];
  int count = 0;

  snprintf(dbqry, MAX_STRING_LENGTH, "SELECT id FROM boons%s%s%s%s",
      (active || random ? " WHERE " : ""),
      (active ? "(active = 1) " : ""),
      (active && random ? "AND " : ""),
      (random ? "(random = 1) " : ""));

  if (!qry(dbqry))
    return 0;
  else
  {
    MYSQL_RES *res = mysql_store_result(DB);
    count = mysql_num_rows(res);
    mysql_free_result(res);
    return count;
  }
  return 0;
}

void zero_boon_data(BoonData *bdata)
{
  if (!bdata)
    return;

  bdata->id = 0;
  bdata->time = 0;
  bdata->duration = 0;
  bdata->racewar = 0;
  bdata->type = 0;
  bdata->option = 0;
  bdata->criteria = 0;
  bdata->criteria2 = 0;
  bdata->bonus = 0;
  bdata->bonus2 = 0;
  bdata->random = 0;
  bdata->author = '\0';
  bdata->active = 0;
  bdata->pid = 0;
  bdata->repeat = 0;
  
  return;
}

bool get_boon_data(int id, BoonData *bdata)
{
  if (!bdata)
    return FALSE;

  if (!qry("SELECT id, time, duration, racewar, type, opt, criteria, criteria2, bonus, bonus2, random, author, active, pid, rpt FROM boons WHERE id = '%d'", id))
  {
    debug("get_boon_data(): cant read from db");
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    //debug("get_boon_data(): no results");
    mysql_free_result(res);
    return FALSE;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  bdata->id = atoi(row[0]); // or ID
  bdata->time = atoi(row[1]);
  bdata->duration = atoi(row[2]);
  bdata->racewar = atoi(row[3]);
  bdata->type = atoi(row[4]);
  bdata->option = atoi(row[5]);
  bdata->criteria = atof(row[6]);
  bdata->criteria2 = atof(row[7]);
  bdata->bonus = atof(row[8]);
  bdata->bonus2 = atof(row[9]);
  bdata->random = atoi(row[10]);
  bdata->author = row[11];
  bdata->active = atoi(row[12]);
  bdata->pid = atoi(row[13]);
  bdata->repeat = atoi(row[14]);

  mysql_free_result(res);

  return TRUE;
}

bool get_boon_progress_data(int id, int pid, BoonProgress *bpg)
{
  if (!bpg)
    return FALSE;

  if (!qry("SELECT id, boonid, pid, counter FROM boons_progress WHERE boonid = '%d' AND pid = '%d'", id, pid))
  {
    debug("get_boon_progress_data(): cant read from db");
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    //debug("get_boon_progress_data(): no results");
    mysql_free_result(res);
    return FALSE;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);

  bpg->id = atoi(row[0]);
  bpg->boonid = atoi(row[1]);
  bpg->pid = atoi(row[2]);
  bpg->counter = atof(row[3]);

  mysql_free_result(res);
  
  return TRUE;
}

bool get_boon_shop_data(int pid, BoonShop *bshop)
{
  if (!bshop)
    return FALSE;

  if (!qry("SELECT id, pid, points, stats from boons_shop WHERE pid = '%d'", pid))
  {
    debug("get_boon_shop_data(): cant read from db");
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    //debug("get_boon_progress_data(): no results");
    mysql_free_result(res);
    return FALSE;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);

  bshop->id = atoi(row[0]);
  bshop->pid = atoi(row[1]);
  bshop->points = atoi(row[2]);
  bshop->stats = atoi(row[3]);

  mysql_free_result(res);
  
  return TRUE;
}

// Data validation for data in BoonData struct
// Flag = which variable in the struct you want to validate
// Can also select an all option
// Returns 0 if data validates OK
// returns number for error msg to display in parse_boon_args
// Special return for BARG_ALL that fails:
//   retval / 100 = which BARG flag we failed
//   retval % 100 = true retval for diagnosing problem
int validate_boon_data(BoonData *bdata, int flag)
{
  char buff[MAX_STRING_LENGTH];
  int i, j, z, retval = 0, iCriteria;
  double dCriteria;
  Guildhall *gh;
  NexusStoneInfo nexus;

  if( !bdata )
  {
    debug("validate_boon_data: NULL BoonData sent to function");
    return FALSE;
  }

  switch( flag )
  {
    case BARG_RACEWAR:
      if( bdata->racewar < 0 || bdata->racewar > RACEWAR_NEUTRAL )
      {
        return 1;
      }
      break;
    case BARG_TYPE:
      if( bdata->type <= BTYPE_NONE || bdata->type >= MAX_BTYPE )
      {
        return 1;
      }
      break;
    case BARG_OPTION:
      if( bdata->option < 0 || bdata->option >= MAX_BOPT )
      {
        return 1;
      }
      if( bdata->type && !check_boon_combo(bdata->type, bdata->option, FALSE) )
      {
        return 2;
      }
      break;
    case BARG_CRITERIA:
      dCriteria = bdata->criteria;
      iCriteria = (int) dCriteria;
      switch( bdata->option )
      {
        case BOPT_NONE:
        case BOPT_ZONE:
          for(i = 0;i <= top_of_zone_table;i++ )
          {
            if( zone_table[i].number == iCriteria )
            {
              break;
            }
          }
          if( i > top_of_zone_table )
          {
            return 1;
          }
          if( bdata->option == BOPT_ZONE )
          {
            vector<epic_zone_data> epic_zones = get_epic_zones();

            // Is epic zone already complete?
            if( epic_zone_done_now(zone_table[i].number) )
            {
              return 2;
            }
            // Is it even an epic zone?
            j = 0;
            while( j <= epic_zones.size() && epic_zones[j].number != iCriteria )
              j++;
            for( j = 0; j <= epic_zones.size(); j++ )
            {
              if( epic_zones[j].number == iCriteria )
              {
                break;
              }
            }
            if( j > epic_zones.size() )
            {
              return 3;
            }
          }
          break;

        case BOPT_MOB:
          // Vnum must be > 0.
          if( bdata->criteria2 <= 0 )
          {
            return 1;
          }

          // Get the R_num from the vnum. .. Do we really car what the R_num is? no.
          //   This is to verify that there is an actual mob type with said vnum.
          if( real_mobile((int)bdata->criteria2) < 0 )
          {
            return 2;
          }

          /* Need to remove this as it puts a copy of the mob in room 0?!?
           *   It's not needed as we can trust that the mob is loadable if it has an R_num.
           *   Since we don't need this, we don't need to record the R_num in the previous if.
          if( !read_mobile(r_num, REAL) )
          {
            return 3;
          }
           */

          // Exp multiplier always has a 1 mob criteria.
          if( bdata->type == BTYPE_EXPM )
          {
            bdata->criteria = 1;
            // If it wasn't set to 1 already, return 3.
            if( iCriteria != bdata->criteria )
            {
              return 3;
            }
          }

          // criteria is the number you must kill in order to get the boon.
          if( iCriteria < 1 )
          {
            return 4;
          }
          break;

        case BOPT_RACE:
          // Number of chars that have to be killed to complete the boon.
          if( iCriteria < 1 )
          {
            return 1;
          }
          // criteria2 is the number corresponding to what race to kill.
          if( bdata->criteria2 < RACE_NONE || bdata->criteria2 > LAST_RACE )
          {
            return 2;
          }

          // Exp multiplier always has a 1 mob criteria.
          if( bdata->type == BTYPE_EXPM )
          {
            bdata->criteria = 1;
            // If it wasn't set to 1 already, return 3.
            if( iCriteria != bdata->criteria )
            {
              return 3;
            }
          }
	        break;

        case BOPT_GH:
          gh = Guildhall::find_by_id(iCriteria);
          if( !gh )
          {
            return 1;
          }
          else if( bdata->racewar != 0 && gh->racewar == bdata->racewar )
          {
            return 2;
          }
          break;

        case BOPT_NEXUS:
          // If we can't find the nexus stone
          if( !nexus_stone_info(iCriteria, &nexus) )
          {
            return 1;
          }
          if( (nexus.align == 3 && bdata->racewar == RACEWAR_GOOD)
            || (nexus.align == -3 && bdata->racewar == RACEWAR_EVIL) )
          {
            return 2;
          }
          break;

        case BOPT_OP:
          // If we can't find the building.
          if( get_building_from_id(iCriteria) == NULL )
          {
            return 1;
          }
          break;

        case BOPT_LEVEL:
          // criteria is the level gained.  You start at lvl 1 so min level gained is 2.
          //   And the max level for mortals is 56.
  	      if( iCriteria < 2 || iCriteria > 56 )
          {
            return 1;
          }
          break;

        case BOPT_FRAG:
          // Criteria is the amount of frags gained at one time.  Can't gain 0 or less.
          //   And you can't gain more than the multiplier for killing above your level.
          if( dCriteria <= 0 || dCriteria > get_property("frag.leveldiff.modifier.high", 1.200) )
          {
            return 1;
          }
          break;

        case BOPT_FRAGS:
          // criteria is the cumulative number of frags gained within the time limit.
          //   Makes no sense for this to be <= 0, but can be as high as you want.
          if( dCriteria <= 0 )
          {
            return 1;
          }
          break;

        case BOPT_CTF:
          if( iCriteria <= 0 )
          {
            return 1;
          }
          for( z = 1; ctfdata[z].id; z++ )
          {
            if( ctfdata[z].id == iCriteria )
            {
              break;
            }
          }
          if( !ctfdata[z].id || !ctfdata[z].room )
          {
            return 2;
          }
          bdata->criteria2 = ctfdata[z].room;
          break;

        case BOPT_CTFB:
          if( bdata->racewar != RACEWAR_NONE )
          {
            bdata->racewar = RACEWAR_NONE;
          }

          if( !real_room0(bdata->criteria2) )
          {
            return 1;
          }

          // Walk through ctf data and look for a ctf boon type that's not in a room?
          for( z = 1; ctfdata[z].id; z++ )
          {
            // Skip non boon type ctf data.
            if( ctfdata[z].type != CTF_BOON )
            {
              continue;
            }
            // Stop at data that doesn't have a room?
            if( !ctfdata[z].room )
            {
              break;
            }
          }

          // No boon ctf's left to pick
          if( !ctfdata[z].id )
          {
            return 2;
          }
          break;

        default:
          break;
      }
      // End of BARG_CRITERIA
      break;

    case BARG_BONUS:
      switch( bdata->type )
      {
        case BTYPE_EXP:
        case BTYPE_EXPM:
        case BTYPE_EPIC:
        case BTYPE_CASH:
          if( bdata->bonus <= 0 )
          {
            return 1;
          }
          break;

        case BTYPE_LEVEL:
          // Makes no sense to grant less than level 2.  Also, don't want to grant Immortality.
          if( bdata->bonus < 2 || bdata->bonus > 56 )
          {
            return 1;
          }
          break;

        case BTYPE_SPELL:
          // Faster to do it this way, than to make a zillion IS_SET checks.
          if( IS_SET(skills[(int)bdata->bonus].targets, TAR_FIGHT_VICT | TAR_OBJ_INV | TAR_OBJ_ROOM
            | TAR_OBJ_WORLD | TAR_OBJ_EQUIP | TAR_OFFAREA | TAR_AGGRO | TAR_WALL) )
          {
            return 1;
          }
          break;

        case BTYPE_ITEM:
          // Must be a positive vnum for a real item.
          if( bdata->bonus <= 0 || real_object( bdata->bonus ) == -1 )
          {
            return 1;
          }
          break;

        default:
          break;
      }
      break;

    case BARG_REPEAT:
      // Setting default repeats...
      if( bdata->type == BTYPE_EXPM && bdata->option == BOPT_NONE )
      {
        bdata->repeat = TRUE;
        break;
      }
      break;

    case BARG_ALL:
      for( i = 1; i < MAX_BARG; i++ )
      {
        if ((retval = validate_boon_data(bdata, i)))
        {
          return (i%100) * 100 + retval % 100;
          /* Above does the same thing, but faster.
          snprintf(buff, MAX_STRING_LENGTH, "%02d%02d", i, retval);
            return (atoi(buff));
           */
        }
      }
      break;

    default:
      return 1;
      break;
  }

  // Data validates OK
  return 0;
}

// sort data from argument into BoonData struct
int parse_boon_args(P_char ch, BoonData *bdata, char *argument)
{
  char arg[MAX_STRING_LENGTH];
  int i, retval;

  // Handle racewar argument
  argument = one_argument(argument, arg);
  if( !strcmp(arg, "all") )
  {
    bdata->racewar = 0;
  }
  else if( !strcmp(arg, "good") )
  {
    bdata->racewar = RACEWAR_GOOD;
  }
  else if( !strcmp(arg, "evil") )
  {
    bdata->racewar = RACEWAR_EVIL;
  }
  else if( !strcmp(arg, "undead") )
  {
    bdata->racewar = RACEWAR_UNDEAD;
  }
  else if( !strcmp(arg, "neutral") )
  {
    bdata->racewar = RACEWAR_NEUTRAL;
  }
  else
  {
    send_to_char_f(ch, "&+W'%s' is not a valid racewar.&n\r\n", arg);
    send_to_char("&+cAvailable Racewars:&n\r\nall\r\ngood\r\nevil\r\nundead\r\nneutral\r\n", ch);
    return FALSE;
  }

  // Handle type (2nd) argument
  argument = one_argument(argument, arg);
  for( i = 1; i < MAX_BTYPE; i++ )
  {
    if (!strcmp(boon_types[i].type, arg))
    {
      break;
    }
  }
  bdata->type = i;

  if( validate_boon_data(bdata, BARG_TYPE) )
  {
    send_to_char_f(ch, "&+W'%s' is not a valid boon type.&n\r\n", arg);
    send_to_char("&+cAvailable Boon Types:&n\r\n", ch);
    for( i = 1; i < MAX_BTYPE; i++ )
    {
      send_to_char_f(ch, "%s\r\n", boon_types[i].type);
    }
    return FALSE;
  }

  // Handle bonus argument
  argument = setbit_parseArgument(argument, arg);

  // First we handle char type arguments
  if (bdata->type == BTYPE_POWER)
  {
    if (!*arg)
    {
      send_to_char("&+WPlease enter an affect.&n\r\n", ch);
      return FALSE;
    }
    int aff = 0, bit = 0;
    for (i = 0; affected1_bits[i].flagLong; i++)
    {
      if( !strcasecmp(arg, affected1_bits[i].flagLong)
        || !strcasecmp(arg, affected1_bits[i].flagShort) )
      {
        aff = 1;
        bit = i;
        break;
      }
      else if( !strcasecmp(arg, affected2_bits[i].flagLong)
        || !strcasecmp(arg, affected2_bits[i].flagShort) )
      {
        aff = 2;
        bit = i;
        break;
      }
      else if( !strcasecmp(arg, affected3_bits[i].flagLong)
        || !strcasecmp(arg, affected3_bits[i].flagShort) )
      {
        aff = 3;
        bit = i;
        break;
      }
      else if( !strcasecmp(arg, affected4_bits[i].flagLong)
        || !strcasecmp(arg, affected4_bits[i].flagShort) )
      {
        aff = 4;
        bit = i;
        break;
      }
      else if( affected5_bits[i].flagLong
        && (!strcasecmp(arg, affected5_bits[i].flagLong)
        || !strcasecmp(arg, affected5_bits[i].flagShort)) )
      {
        aff = 5;
        bit = i;
        break;
      }
    }
    if (!bit || !aff)
    {
      char flagbuff[MAX_STRING_LENGTH];

      send_to_char_f(ch, "&+W'%s' is not a valid affect.  Valid options are:&n\r\n", arg);
      *flagbuff = '\0';
      concat_which_flagsde("Aff1", affected1_bits, flagbuff);
      concat_which_flagsde("Aff2", affected2_bits, flagbuff);
      concat_which_flagsde("Aff3", affected3_bits, flagbuff);
      concat_which_flagsde("aff4", affected4_bits, flagbuff);
      concat_which_flagsde("aff5", affected5_bits, flagbuff);
      page_string(ch->desc, flagbuff, 1);
      return FALSE;
    }
    bdata->bonus = aff;
    bdata->bonus2 = bit;
  }
  else if( bdata->type == BTYPE_SPELL )
  {
    if( !*arg )
    {
      send_to_char("&+WPlease enter a spell name.&n\r\n", ch);
      return FALSE;
    }
    if( isdigit(*arg) )
    {
      send_to_char("&+WThat's not a valid spell name.  Use single quotes (') if necessesary.&n\r\n", ch);
      return FALSE;
    }
    for( i = 1; i <= LAST_SPELL; i++ )
    {
      if( is_abbrev(skills[i].name, arg) )
      {
        break;
      }
    }
    if( i > LAST_SPELL )
    {
      send_to_char_f(ch, "&+W'%s' is not a valid spell name.  Try using single quotes (') if needed.&n\r\n", arg);
      return FALSE;
    }
    bdata->bonus = i;
  }
  else if( bdata->type == BTYPE_STAT )
  {
    if( !*arg )
    {
      send_to_char("&+WPlease enter an attribute.&n\r\n", ch);
      return FALSE;
    }
    if( isdigit(*arg) )
    {
      send_to_char("&+WThat bonus is not a valid stat, please choose from: str, dex, agi, con, pow, int, wis, cha, karma, and luck.&n\r\n", ch);
      return FALSE;
    }

    for( i = 1; i < MAX_ATTRIBUTES; i++ )
    {
      if( is_abbrev(arg, attr_names[i].abrv) || is_abbrev(arg, attr_names[i].name) )
      {
        bdata->bonus = i;
        break;
      }
    }
    if( !bdata->bonus )
    {
      send_to_char("The bonus is not a valid stat, please choose from: str, dex, agi, con, pow, int, wis, cha, karma, and luck.\r\n", ch);
      return FALSE;
    }
  }
  else if( bdata->type == BTYPE_ITEM )
  {
    i = atoi(arg);
    if( i <= 0 || real_object(i) < 0 )
    {
      send_to_char_f(ch, "&+W'%s' is not a valid item vnum.  Please enter a number.&n\r\n", arg);
      return FALSE;
    }
    bdata->bonus = i;
  }

  // Then we handle the normal number type stats
  if( !bdata->bonus && (!isdigit(*arg)) )
  {
    send_to_char_f(ch, "&+W'%s' is not a valid bonus.  Please enter a number.&n\r\n", arg);
    return FALSE;
  }
  else if( !bdata->bonus )
  {
    bdata->bonus = atof(arg);
  }

  if( bdata->type == BTYPE_LEVEL )
  {
    argument = setbit_parseArgument(argument, arg);
    if( *arg && !strcmp(arg, "yes") )
    {
      bdata->bonus2 = 1;
    }
    else if( *arg && !strcmp(arg, "no") )
    {
      bdata->bonus2 = 0;
    }
    else if( *arg && atoi(arg) == 0 )
    {
      bdata->bonus2 = 0;
    }
    else if( *arg && atoi(arg) == 1 )
    {
      bdata->bonus2 = 1;
    }
    else
    {
      send_to_char("Invalid secondary bonus, please indicate whether or not to bypass epics (1 or yes, 0 or no).\r\n", ch);
      return FALSE;
    }
  }

  if( (retval = validate_boon_data(bdata, BARG_BONUS)) )
  {
    switch( bdata->type )
    {
      case BTYPE_LEVEL:
        if( retval == 1 )
        {
          send_to_char("That level cap is out of range, please choose a cap on what level you can achieve (56 means, you can gain a level up to level 56).", ch);
        }
        break;
      case BTYPE_SPELL:
        if( retval == 1 )
        {
          send_to_char("&+WThat spell is not an appropriate bonus, choose a non aggressive spell that doesn't interact with objects or other targets in the room (hint choose a spellup).&n\r\n", ch);
        }
        break;
      case BTYPE_EXPM:
      case BTYPE_EXP:
      case BTYPE_EPIC:
      case BTYPE_CASH:
        if( retval == 1 )
        {
          send_to_char("&+WNegative bonus? What's the point?&n\r\n", ch);
        }
        break;
      case BTYPE_ITEM:
        if( retval == 1 )
        {
          send_to_char("&+WCould not find an item with that vnum.&n\r\n", ch);
        }
else
send_to_char_f(ch, "&+WUnknown error with boon item bonus '%d'.&n\r\n", retval);
        break;
      default:
          send_to_char_f(ch, "&+WUnknown error with boon bonus '%d'.&n\r\n", retval);
        break;
    }
    return FALSE;
  }

  // Handle option argument
  argument = one_argument(argument, arg);
	// Keeping this here instead of the validation function because this is specific to manually
  //   created boons.  Randomly created boons cannot generate anything requiring a level.
  for( i = 0; i < MAX_BOPT; i++ )
  {
    if( !strcmp(boon_options[i].option, arg) )
    {
      if( get_boon_level(bdata->type, i) > GET_LEVEL(ch) )
      {
        send_to_char_f(ch, "That combination requires level %d to create.\r\n", get_boon_level(bdata->type, i));
        return FALSE;
      }
      // Found option, level requirement met: Everything's good, let's continue...
      break;
    }
  }

  bdata->option = i;

    if( (retval = validate_boon_data(bdata, BARG_OPTION)) )
    {
      if( retval == 3 )
      {
        send_to_char("CTF boon's are not available when CTF_MUD is not enabled.\r\n", ch);
      }
      else if( retval == 2 )
      {
        send_to_char("That is not a valid type and option combination.\r\n", ch);
      }
      else if( retval == 1 )
      {
        send_to_char_f(ch, "&+W'%s' is not a valid boon option.&n\r\n", arg);
      }
      else
      {
        send_to_char_f(ch, "&+WUnknown error with boon option '%s'.&n\r\n", arg);
      }
      send_to_char("&+cAvailable Boon Options:&n\r\n", ch);
      for (i = 0; i < MAX_BOPT; i++)
      {
	if (check_boon_combo(bdata->type, i, FALSE) &&
	    get_boon_level(bdata->type, i) <= GET_LEVEL(ch))
	  send_to_char_f(ch, "%s\r\n", boon_options[i].option);
	else
	  continue;
      }
      return FALSE;
    }

    // Handle criteria argument
    argument = setbit_parseArgument(argument, arg);

/*
These have a return 3 option:
This is random code for retval 3 handling:
  BARG_CRITERIA
        case BOPT_NONE:
        case BOPT_ZONE:
          if( retval == 3 )
          {
            send_to_char("Could not find the zone.", ch);
          }
          break;

  BOPT_MOB
  BOPT_RACE
        case BTYPE_EXPM:
          if( retval == 3 )
          {
            send_to_char( "Set number of mobs to kill for boon completion to 1.\n\r", ch );
          }
          break;
*/
    if (bdata->option == BOPT_NONE ||
	bdata->option == BOPT_ZONE)
    {
      if (*arg && !isdigit(*arg))
      {
	for (i = 0; i <= top_of_zone_table; i++)
	{
	  if (is_abbrev(strip_ansi(zone_table[i].name).c_str(), arg) ||
	      !strcmp(zone_table[i].filename, arg))
	  {
	    //debug("strip: %s, zt: %s, arg: %s", strip_ansi(zone_table[i].name).c_str(), zone_table[i].filename, arg);
	    break;
	  }
	}
	if (i > top_of_zone_table)
	{
	  send_to_char_f(ch, "&+W'%s' is not a valid zone name or filename.  Try using single quotes (') if you're using the zone name.\r\n", arg);
	  return FALSE;
	}
	bdata->criteria = zone_table[i].number;
	//debug("bdata->criteria: %d, i: %d, num: %d", (int)bdata->criteria, i, zone_table[i].number);
      }
    }

    if (bdata->option == BOPT_CTF)
    {
      if (*arg)
      {
	if (!isdigit(*arg))
	{
	  if (!strcmp(arg, "good"))
	    bdata->criteria = CTF_FLAG_GOOD;
	  else if (!strcmp(arg, "evil"))
	    bdata->criteria = CTF_FLAG_EVIL;
	  else
	  {
	    send_to_char("Please enter good, evil, or the ctf flag ID #.\r\n", ch);
	    return FALSE;
	  }
	}
	else
	{
	  bdata->criteria = atof(arg);
	}
      }
      else
      {
	send_to_char("Please enter good, evil, or the ctf flag ID #.\r\n", ch);
	return FALSE;
      }
    }

    if (bdata->option == BOPT_CTFB)
    {
      if (*arg && !isdigit(*arg))
      {
	send_to_char("Please enter the vnum of the room you wish this ctf flag to load.", ch);
	return FALSE;
      }
      else
      {
        bdata->criteria2 = atof(arg);
      }
      if (bdata->racewar != RACEWAR_NONE)
      {
        send_to_char("CTF Flag Boons must be for all racewars, setting racewar to all.\r\n", ch);
        bdata->racewar = RACEWAR_NONE;
      }
    }

    if (!bdata->criteria && (!*arg || !isdigit(*arg)))
    {
      send_to_char_f(ch, "&+W'%s' is not a valid criteria.  Please enter a number.&n\r\n", arg);
      return FALSE;
    }

    if (!bdata->criteria)
    {
      bdata->criteria = atof(arg);
    }

    if( (bdata->option == BOPT_MOB || bdata->option == BOPT_RACE)
      && bdata->type == BTYPE_EXPM && bdata->bonus > 1 && bdata->criteria != 1 )
    {
      send_to_char("Exp modification is designed to work per mob, so defaulting your kills per completion criteria to 1.\r\n", ch);
      bdata->criteria = 1;
    }

    // secondary arguments
    if (bdata->option == BOPT_MOB)
    {
      argument = one_argument(argument, arg);
      if (!*arg || !atof(arg))
      {
        send_to_char_f(ch, "&+W'%s' is not a valid secondary criteria.  Please enter a number.&n\r\n", arg);
        return FALSE;
      }
      bdata->criteria2 = atof(arg);
    }

    if (bdata->option == BOPT_RACE)
    {
      argument = setbit_parseArgument(argument, arg);
      if( !*arg )
      {
        send_to_char_f(ch, "&+W'%s' is not a valid race.  Please enter a race name or corresponding number.&n\r\n", arg);
        return FALSE;
      }
      if( atoi(arg) > 0 && atoi(arg) <= LAST_RACE )
      {
        bdata->criteria2 = atoi(arg);
      }
      else
      {
        bdata->criteria2 = 0;
        // check for exact match first, skip RACE_NONE
        for( i = 1; i <= LAST_RACE; i++ )
        {
          // isname doesn't work here 'cause some races have 2-word name (ie Aquatic Animal comes before Animal).
          if( !strcmp(arg, race_names_table[i].normal) )
          {
            bdata->criteria2 = i;
            break;
          }
        }
        // otherwise check for abbreviation
        if( bdata->criteria2 == 0 )
        {
          for( i = 0; i <= LAST_RACE; i++ )
          {
            if( is_abbrev(arg, race_names_table[i].normal) )
            {
              bdata->criteria2 = i;
              break;
            }
          }
        }
      }
      if( bdata->criteria2 == 0 )
      {
        send_to_char_f(ch, "&+W'%s' is not a valid race.  Please enter a race name or corresponding number.&n\r\n", arg);
        return FALSE;
      }
    }

    if ((retval = validate_boon_data(bdata, BARG_CRITERIA)))
    {
      switch (bdata->option)
      {
	case BOPT_NONE:
	case BOPT_ZONE:
	  {
	    if (retval == 1)
	      send_to_char_f(ch, "&+W'%d' is an invalid criteria.  Zone does not exist.&n\r\n", (int)bdata->criteria);
	    if (retval == 2)
	      send_to_char("&+WThat zone is already complete.&n\r\n", ch);
	    if (retval == 3)
	      send_to_char("&+WThat is not an epic zone.&n\r\n", ch);
	    break;
	  }
	case BOPT_MOB:
	  {
	    if (retval == 1)
	      send_to_char("&+WA negative vnum?&n\r\n", ch);
	    if (retval == 2)
	      send_to_char_f(ch, "&+WThere is no monster with vnum '%d'.&n\r\n", (int)bdata->criteria2);
	    if (retval == 3)
	      send_to_char("&+WMonster failed to load.&n\r\n", ch);
	    if (retval == 4)
	      send_to_char("&+WA neverending kill quest?&n\r\n", ch);
	    break;
	  }
	case BOPT_RACE:
	  {
	    if (retval == 1)
	      send_to_char("&+WA neverending kill quest?&n\r\n", ch);
	    if (retval == 2)
	      send_to_char("&+WYou shouldn't see this, race # is out of range.\r\n", ch);
	    break;
	  }
	case BOPT_GH:
	  {
	    if (retval == 1)
	      send_to_char_f(ch, "&+WGuildhall # %d does not exist.&n\r\n", (int)bdata->criteria);
	    if (retval == 2)
	      send_to_char("&+WThat guildhall is already owned by that racewar side&n.\r\n", ch);
	    break;
	  }
	case BOPT_NEXUS:
	  {
	    if (retval == 1)
	      send_to_char_f(ch, "&+W'%d' is not a valid nexus stone ID.&n\r\n", (int)bdata->criteria);
	    if (retval == 2)
	      send_to_char("&+WThat nexus is already owned by that racewar side.&n\r\n", ch);
	    break;
	  }
	case BOPT_OP:
	  {
	    if (retval == 1)
	      send_to_char_f(ch, "&+W'%d' is not a valid outpost ID.&n\r\n", (int)bdata->criteria);
	    break;
	  }
	case BOPT_LEVEL:
	  {
	    if (retval == 1)
	      send_to_char("&+WNice try.  Stick to a valid level range.  Use 0 for any level to qualify.\r\n", ch);
	    break;
	  }
	case BOPT_FRAG:
	case BOPT_FRAGS:
	  {
	    if (retval == 1)
	      send_to_char("&+WWhat's the point?&n\r\n", ch);
	    break;
	  }
	case BOPT_CTF:
	  {
	    if (retval == 1)
	      send_to_char("&+WPlease enter a valid CTF flag ID.\r\n", ch);
	    if (retval == 2)
	      send_to_char("&+WThat is not a valid CTF flag ID.\r\n", ch);
	    break;
	  }
	case BOPT_CTFB:
	  {
	    if (retval == 1)
	      send_to_char("&+WThat room vnum does not exist.\r\n", ch);
	    if (retval == 2)
	      send_to_char("&+WThere are no ctf boon flags available for use.\r\n", ch);
	    break;
	  }
	default:
	  {
	    send_to_char("&+RA case was not handled by parse_boon_args().&n\r\n",ch);
	    break;
	  }
      }
      return FALSE;
    }

    // Handle duration argument
    argument = one_argument(argument, arg);
    if (!*arg || !atoi(arg))
    {
      send_to_char_f(ch, "&+W'%s' is not a valid duration.  Please enter a number or -1 for no duration.&n\r\n", arg);
      return FALSE;
    }
    bdata->duration = atoi(arg);

    // Other flags
    while (*argument)
    {
      argument = one_argument(argument, arg);
      // Do we want to bind this to a single player?
      if (!strcmp(arg, "repeat"))
      {
	bdata->repeat = TRUE;
      }
      if (!strcmp(arg, "-p"))
      {
	argument = one_argument(argument, arg);
	
	if (!(bdata->pid = get_player_pid_from_name(arg)))
	{
	  send_to_char("That player does not exist or is invalid.\r\n", ch);
	  return FALSE;
	}
      }
    }

    if ((retval = validate_boon_data(bdata, BARG_REPEAT)))
    {
      if (retval == 1)
	send_to_char("Experience gain modification for zones set to repeat automatically.\r\n", ch);
    }

    // This is being created manually, so lets set the author.
    bdata->author = GET_NAME(ch);

  return TRUE;
}

void do_boon(P_char ch, char *argument, int cmd)
{
  char arg[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH], buffline[MAX_STRING_LENGTH];
  int duration = 0, id = 0;
  int i, dresult;

  argument = one_argument(argument, arg);

  // The mortal arguments
  // Handle list (or no arguments)
  if (*arg == '\0' || !strcmp(arg, "list"))
  {
    dresult = boon_display(ch, argument);
    if (dresult == -1)
    {
      send_to_char("Something's wrong, can't read from the DB", ch);
      return;
    }
    else if (!dresult)
    {
      send_to_char("No results.\r\n", ch);
      return;
    }
    return;
  }
  // Handle Shops
  else if (!strcmp(arg, "shop"))
  {
    //handle shop stuff
    //send_to_char("Boon Shops not implemented yet.\r\n", ch);
    //return;

    boon_shop(ch, argument);
    return;
  }

  // No more mortal arguments past this point
  if (!IS_TRUSTED(ch))
  {
    send_to_char("Invalid argument.  Valid arguments: list, shop.\r\n", ch);
    return;
  }

  // Immortal arguments
  // Adding boons (manually)
  if (!strcmp(arg, "add"))
  {
    BoonData bdata;

    zero_boon_data(&bdata);

    // Let's check the arguments and sort them into BoonData
    if (!parse_boon_args(ch, &bdata, argument))
      return;

    if (bdata.option == BOPT_CTFB)
      if (!ctf_use_boon(&bdata))
      {
	send_to_char("Failed to create a CTF Boon Flag.\r\n", ch);
	return;
      }

    // Ok, we should have everything we need, time to create the boon
    if (create_boon(&bdata))
    {
      send_to_char("Boon successfully created.\r\n", ch);
      return;
    }
    else
    {
      send_to_char("Something went wrong.  Boon failed to create.\r\n", ch);
      return;
    }
    send_to_char("Something's wrong, how'd we get here?\r\n", ch);
    return;
  }
  // Removing boons (essentially making them inactive)
  else if (!strcmp(arg, "remove"))
  {
    argument = one_argument(argument, arg);
    if (!*arg || !isdigit(*arg))
    {
      send_to_char_f(ch, "&+W'%s' is not a valid boon ID.&n\r\n", arg);
      return;
    }
    id = atoi(arg);
    if (!is_boon_valid(id))
    {
      send_to_char_f(ch, "&+WBoon # %d does not exist&n.\r\n", id);
      return;
    }
    if (remove_boon(id))
    {
      boon_notify(id, NULL, BN_VOID);
      send_to_char_f(ch, "&+WSuccessfully removed boon # %d.&n\r\n", id);
      return;
    }
    else
    {
      send_to_char("&+LRemoval was unsuccessful.&n\r\n", ch);
      return;
    }
    send_to_char("Something's wrong.\r\n", ch);
    return;
  }
  // Extend duration on existing boon
  else if (!strcmp(arg, "extend"))
  {
    argument = one_argument(argument, arg);
    if (!*arg || !isdigit(*arg))
    {
      send_to_char_f(ch, "&+w'%s' is not a valid boon ID.  Please enter the boon ID you wish to extend.&n\r\n", arg);
      return;
    }
    id = atoi(arg);
    if (!is_boon_valid(id))
    {
      send_to_char_f(ch, "&+WBoon # %d does not exist.&n\r\n", id);
      return;
    }
    argument = one_argument(argument, arg);
    if (!*arg || !isdigit(*arg))
    {
      send_to_char_f(ch, "&+W'%s' is not a valid duration.  Please enter the amount of time you wish to extend the boon duration in minutes.&n\r\n", arg);
      return;
    }
    duration = atoi(arg);
    //debug("passing id %d duration %d", id, duration);
    if (extend_boon(id, duration, GET_NAME(ch)))
      send_to_char_f(ch, "Boon # %d has been extended for %d minutes.\r\n", id, duration);
    else
      send_to_char("Extension failed.\r\n", ch);
    return;
  }
  // Randonized boon controller
  else if (!strcmp(arg, "random"))
  {
    boon_randomize(ch, argument);
    return;
  }
  else if (!strcmp(arg, "help") || !strcmp(arg, "?"))
  {
    snprintf(buff, MAX_STRING_LENGTH, "&+WBoon Command Help&n\r\n");
    strcat(buff, "&+CNo Argument&n   display current boons(Default flags: hmr)\r\n");
    strcat(buff, "&+CList&n          display current boons\r\n");
    strcat(buff, "       &+Lsyntax&n boon list im u venthix\r\n");
    strcat(buff, "            &+ch&n show active boons\r\n");
    strcat(buff, "            &+ci&n show inactive boons\r\n");
    strcat(buff, "            &+cm&n list manually created boons\r\n");
    strcat(buff, "            &+cr&n list randomly created boons\r\n");
    strcat(buff, "     &+cu [name]&n show only boons created by name\r\n");
    strcat(buff, "                   You can use the '%%' symbol as a wildcard\r\n");
    strcat(buff, "     &+ct [type]&n show only boons of a specified type\r\n");
    strcat(buff, "   &+co [option]&n show only boons of a specified option\r\n");
    strcat(buff, "     &+cp [name]&n show only boons specified for name\r\n");
    strcat(buff, "&+CAdd&n           add a new boon\r\n");
    strcat(buff, "       &+Lsyntax&n boon add racewar type bonus [bonus2] option criteria [criteria2] duration [repeat] -p playername\r\n");
    strcat(buff, "      &+cracewar&n [all|good|evil|undead|neutral]\r\n");
    strcat(buff, "         &+ctype&n [expm|exp|epic|cash|level|power|spell|stat|stats|point|item]\r\n");
    strcat(buff, "        &+cbonus&n the amount of the boon type bonus (200 epics, 2000 copper, etc)\r\n");
    strcat(buff, "       &+coption&n [none|zone|level|mob|race|frag|frags|guildhall|outpost|nexus|cargo|auction|ctf|ctfb]\r\n");
    strcat(buff, "     &+ccriteria&n zone number, level, frag requirement, mob vnum, outpost ID, etc\r\n");
    strcat(buff, "     &+cduration&n time limit till boon expires in minutes (-1 for no expiration)\r\n");
    strcat(buff, "       &+crepeat&n designates the completing the boon is repeatable\r\n");
    strcat(buff, "           &+c-p&n create boon for specified person only (searches by name).\r\n");
    strcat(buff, "&+CRemove&n        remove an existing boon\r\n");
    strcat(buff, "       &+Lsyntax&n boon remove boon_id\r\n");
    strcat(buff, "&+CExtend&n        Extend an existing boon's duration, An * will show next to author name\r\n");
    strcat(buff, "                   (extending an inactive boon will reactivate the boon\r\n");
    strcat(buff, "&+CRandom&n        replace existing random boons with new ones.\r\n");
    strcat(buff, "       &+Lsyntax&n boon random [optional boon_id]\r\n");
    strcat(buff, "      &+cboon_id&n You can select a specific random boon to replace instead\r\n");
    strcat(buff, "              of replacing them all.\r\n\r\n");
    send_to_char(buff, ch);
    send_to_char("&+WValid boon type and option combinations:&n\r\n", ch);
    send_to_char("'&+mM&n' designates manual set by a specific level.\r\n\r\n", ch);
    snprintf(buff, MAX_STRING_LENGTH, "          ");
    for (i = 1; i < MAX_BTYPE; i++)
    {
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "|&+C%-5s&n", boon_types[i].type);
    }
    strcat(buff, "|&n\r\n");
    send_to_char(buff, ch);
    snprintf(buffline, MAX_STRING_LENGTH, "-");
    for (i = 0; i < ((6*(MAX_BTYPE-1))+10); i++)
    {
      strcat(buffline, "-");
    }
    strcat(buffline, "\r\n");
    send_to_char(buffline, ch);
    for (i = 0; i < MAX_BOPT; i++)
    {
      snprintf(buff, MAX_STRING_LENGTH, "&+C%-10s&n", boon_options[i].option);
      for (int k = 1; k < MAX_BTYPE; k++)
      {
	if (check_boon_combo(k, i, FALSE))
	  if (!check_boon_combo(k, i, TRUE))
	    snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "|&+mM&n(&+c%2d&n)", get_boon_level(k, i));
	  else
            strcat(buff, "|&+W  X  &n");
	else
	  strcat(buff, "|     ");
      }
      strcat(buff, "|\r\n");
      send_to_char(buff, ch);
      send_to_char(buffline, ch);
    }

    return;
  }
  
  // How'd we get here?  wrong arguments...
  send_to_char("Invalid control argument.  Valid arguments: list, add, remove, extend, random, help.\r\n", ch);
  return;
}

void boon_shop(P_char ch, char *argument)
{
  char arg[MAX_STRING_LENGTH];
  int stat = 0;
  int i;

  argument = one_argument(argument, arg);

  BoonShop bshop;
  if (!get_boon_shop_data(GET_PID(ch), &bshop))
  {
    bshop.id = 0;
    bshop.pid = GET_PID(ch);
    bshop.points = 0;
    bshop.stats = 0;
  }
  
  // handle arg's.. buy, etc...
  if (!strcmp(arg, "stat") ||
      !strcmp(arg, "stats"))
  {
    if (!bshop.stats)
    {
      send_to_char("You don't have any stat points available.\r\n", ch);
      return;
    }

    argument = one_argument(argument, arg);

    if (!*arg ||
	isdigit(*arg))
    {
      send_to_char("Please choose a stat you wish to apply your stat point towards.\r\n", ch);
      return;
    }
    for (i = 1; i < MAX_ATTRIBUTES; i++)
    {
      if (is_abbrev(arg, attr_names[i].abrv) || is_abbrev(arg, attr_names[i].name))
      {
	stat = i;
	break;
      }
    }
    if (!stat)
    {
      send_to_char("That's not a valid stat, please choose from the following: str, dex, agi, con, pow, int, wis, con.\r\n", ch);
      return;
    }
    else
    {
      bshop.stats--;
      switch (stat)
      {
	case STR:
	  {
	    if (ch->base_stats.Str >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Str = BOUNDED(0, ch->base_stats.Str+1, 100);
	    send_to_char("You feel stronger!\r\n", ch);
	    break;
	  }
	case DEX:
	  {
	    if (ch->base_stats.Dex >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Dex = BOUNDED(0, ch->base_stats.Dex+1, 100);
	    send_to_char("You feel more dextrous!\r\n", ch);
	    break;
	  }
	case AGI:
	  {
	    if (ch->base_stats.Agi >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Agi = BOUNDED(0, ch->base_stats.Agi+1, 100);
	    send_to_char("You feel more agile!\r\n", ch);
	    break;
	  }
	case CON:
	  {
	    if (ch->base_stats.Con >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Con = BOUNDED(0, ch->base_stats.Con+1, 100);
	    send_to_char("You feel ten years younger!\r\n", ch);
	    break;
	  }
	case POW:
	  {
	    if (ch->base_stats.Pow >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Pow = BOUNDED(0, ch->base_stats.Pow+1, 100);
	    send_to_char("Your mind suddenly feels ten times as powerful!\r\n", ch);
	    break;
	  }
	case INT:
	  {
	    if (ch->base_stats.Int >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Int = BOUNDED(0, ch->base_stats.Int+1, 100);
	    send_to_char("You feel smarter! Man, you were a real dumbass before.\r\n", ch);
	    break;
	  }
	case WIS:
	  {
	    if (ch->base_stats.Wis >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Wis = BOUNDED(0, ch->base_stats.Wis+1, 100);
	    send_to_char("You feel wiser!\r\n", ch);
	    break;
	  }
	case CHA:
	  {
	    if (ch->base_stats.Cha >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Cha = BOUNDED(0, ch->base_stats.Cha+1, 100);
	    send_to_char("Suddenly one of the pimples on your face dissapears!\r\n", ch);
	    break;
	  }
	case LUCK:
	  {
	    if (ch->base_stats.Luk >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Luk = BOUNDED(0, ch->base_stats.Luk+1, 100);
	    send_to_char("You feel as if you could roll Triple Tiamat's at the slots...\r\n", ch);
	    break;
	  }
	case KARMA:
	  {
	    if (ch->base_stats.Kar >= 100)
	    {
	      send_to_char("You already have 100 points in that stat.\r\n", ch);
	      bshop.stats++;
	      break;
	    }
	    ch->base_stats.Kar = BOUNDED(0, ch->base_stats.Kar+1, 100);
	    send_to_char("You feel strange.\r\n", ch);
	    break;
	  }
	default:
	  {
	    //well that's not suppose to happen... add the stat back.
	    bshop.stats++;
	    break;
	  }
      }
      if (!qry("UPDATE boons_shop SET stats = '%d' WHERE pid = '%d'", bshop.stats, GET_PID(ch)))
      {
	debug("boon_shop(): failed to update shop DB entry");
	return;
      }
    }
  }
  
  // no arguments
  if (!*arg)
  {
    send_to_char("&+WBoon Shop&n\r\n", ch);
    send_to_char_f(ch, "&+CShop points available: %d\r\n", bshop.points);
    send_to_char_f(ch, "&+CStat points available: %d\r\n", bshop.stats);
    send_to_char("&+CItems available:\r\n", ch);
    //send data
    // but for now...
    send_to_char("No items available.\r\n", ch);
    // reclaiming stats
    return;
  }
}

int boon_display(P_char ch, char *argument)
{
  char arg[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH], dbqry[MAX_STRING_LENGTH];
  char bufftype[MAX_STRING_LENGTH], buffoption[MAX_STRING_LENGTH];
  char cdtime[MAX_STRING_LENGTH], rw[MAX_STRING_LENGTH];
  struct time_info_data timer;
  int ct, i, pid = 0, count = 0;
  int active = 0, inactive = 0, random = 0, manual = 0;
  char name[MAX_STRING_LENGTH], type[MAX_STRING_LENGTH], option[MAX_STRING_LENGTH];
  char player[MAX_STRING_LENGTH], pname[MAX_STRING_LENGTH];

  *name = *type = *option = *pname = *player = '\0';

  // Handle flags
  while (*argument)
  {
    argument = one_argument(argument, arg);
    switch(LOWER(*arg))
    {
      case 'p':
	{
	  argument = one_argument(argument, arg);
	  pid = get_player_pid_from_name(arg);
          if (!pid)
	  {
	    send_to_char_f(ch, "&+W'%s' player does not exist or is not valid.&n\r\n", arg);
	    return -2;
	  }
	  if (*player)
	    snprintf(player + strlen(player), MAX_STRING_LENGTH - strlen(player), "OR pid = '%d' ", pid);
	  else
	    snprintf(player, MAX_STRING_LENGTH, "pid = '%d' ", pid);
	  break;
	}
      case 'u':
	{
	  argument = one_argument(argument, arg);
	  if (*name)
	    snprintf(name + strlen(name), MAX_STRING_LENGTH - strlen(name), "OR author LIKE '%s' ", arg);
	  else
	    snprintf(name, MAX_STRING_LENGTH, "author LIKE '%s' ", arg);
	  break;
	}
      case 't':
	{
	  argument = one_argument(argument, arg);
	  if (get_valid_boon_type(arg) == -1)
	  {
            send_to_char_f(ch, "&+W'%s' is not a valid boon type.&n\r\n", arg);
            send_to_char("&+cAvailable Boon Types:&n\r\n", ch);
            for (i = 1; i < MAX_BTYPE; i++)
	      send_to_char_f(ch, "%s\r\n", boon_types[i].type);
            return -2;
	  }
	  else
	  {
	    if (*type)
	      snprintf(type + strlen(type), MAX_STRING_LENGTH - strlen(type), "OR type = '%d' ", get_valid_boon_type(arg));
	    else
	      snprintf(type, MAX_STRING_LENGTH, "type = '%d' ", get_valid_boon_type(arg));
	  }
	  break;
	}
      case 'o':
	{
	  argument = one_argument(argument, arg);
	  if (get_valid_boon_option(arg) == -1)
	  {
            send_to_char_f(ch, "&+W'%s' is not a valid boon option.&n\r\n", arg);
            send_to_char("&+cAvailable Boon Options:&n\r\n", ch);
            for (i = 0; i < MAX_BOPT; i++)
	      send_to_char_f(ch, "%s\r\n", boon_options[i].option);
	    return -2;
	  }
	  else
	  {
	    if (*option)
	      snprintf(option + strlen(option), MAX_STRING_LENGTH - strlen(option), "OR opt = '%d' ", get_valid_boon_option(arg));
	    else
	      snprintf(option, MAX_STRING_LENGTH, "opt = '%d' ", get_valid_boon_option(arg));
	  }
	  break;
	}
      default:
	{
	  i = 0;
	  while (arg[i] != '\0')
	  {
	    switch(LOWER(arg[i]))
	    {
	      case 'h':
		{
		  active = TRUE;
		  break;
		}
	      case 'i':
		{
		  inactive = TRUE;
		  break;
		}
	      case 'm':
		{
		  manual = TRUE;
		  break;
		}
	      case 'r':
		{
		  random = TRUE;
		  break;
		}
	      default:
		break;
	    }
	    i++;
	  }
	  break;
	}
    }
  }
  if (!active && !inactive && !random && !manual && !*name && !*type && !*option && !*player)
  {
    // No arguments given, set defaults to 'hmr'
    active = 1;
    random = 1;
    manual = 1;
  }

  //debug("active: %d, inactive: %d, random: %d, manual: %d", active, inactive, random, manual);
  //debug("name: %s, type: %s, option: %s", name, type, option);

  send_to_char("&+WThe Gods of Duris have given you and your allies the following boons:&n\r\n", ch);
  // zone_table[zone_count].number = zone number
  // pad_ansi(zone_table[zone_count].name, 45].c_str() = zone name
  // zone_table[zone_count].avg_mob_level = way to find out range of zone
  
  // Please do not touch, thanks. 
  snprintf(dbqry, MAX_STRING_LENGTH, "SELECT id, time, duration, racewar, type, opt, criteria, " \
      "criteria2, bonus, bonus2, random, author, active, pid, rpt FROM boons " \
      "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s" \
      "ORDER BY id ASC",
	(active || inactive || manual || random || *name || *type || *option ? "WHERE " : ""),
	(active || inactive ? "( " : ""),
	(active ? "active = '1' " : ""),
	(active && inactive ? "OR " : ""),
	(inactive ? "active = '0' " : ""),
	(active || inactive ? ") " : ""),
	(active && manual || inactive && manual ? "AND " : ""),
	(manual ? "( " : ""),
	(manual ? "random = '0' " : ""),
	(active && random && !manual || inactive && random && !manual ? "AND " : ""),
	(random && !manual ? "( " : ""),
	(random && manual ? "OR " : ""),
	(random ? "random = '1' " : ""),
	(manual || random ? ") " : ""),
	(active && *name || inactive && *name || manual && *name || random && *name ? "AND " : ""),
	(*name ? "( " : ""),
	(*name ? name : ""),
	(*name ? ") " : ""),
	(active && *type || inactive && *type || manual && *type || random && *type || *name && *type ? "AND " : ""),
	(*type ? "( " : ""),
	(*type ? type : ""),
	(*type ? ") " : ""),
	(active && *option || inactive && *option || manual && *option || random && *option || *name && *option || *type && *option ? "AND " : ""),
	(*option ? "( " : ""),
	(*option ? option : ""),
	(*option ? ") " : ""),
	(active && *player || inactive && *player || manual && *player || random && *player || *name && *player || *type && *player || *option && *player ? "AND " : ""),
	(*player ? "( " : ""),
	(*player ? player : ""),
	(*player ? ") " : ""));
  //debug(dbqry);
  if( !qry(dbqry) )
  {
    debug("boon_display() can't read from db");
    return -1;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return FALSE;
  }

  if (IS_TRUSTED(ch))
    send_to_char_f(ch, "&+C%-6s   %-10s %-8s %-7s %-6s %-9s %9s %9s %10s %7s %-10s&n\r\n",
      "ID", "Random", "Duration", "Racewar", "Type", "Option", "Criteria", "Criteria2", "Bonus", "Bonus2", "Assigned");
  else
    send_to_char_f(ch, "&+C%-6s   %-8s %-7s %s&n\r\n",
	"ID", "Duration", "Racewar", "Description");

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res)))
  {
    int id = atoi(row[0]);
    int timethen = atoi(row[1]);
    int duration = atoi(row[2]);
    int racewar = atoi(row[3]);
    int type = atoi(row[4]);
    int option = atoi(row[5]);
    double criteria = atof(row[6]);
    double criteria2 = atof(row[7]);
    double bonus = atof(row[8]);
    double bonus2 = atof(row[9]);
    int random = atoi(row[10]);
    char *author = row[11];
    int active = atoi(row[12]);
    pid = atoi(row[13]);
    int repeat = atoi(row[14]);

    // Should we display this line to ch?
    if (!IS_TRUSTED(ch) &&
	((racewar != 0 && GET_RACEWAR(ch) != racewar) ||
	 (pid != 0 && GET_PID(ch) != pid)))
      continue;

    count++;

    // interpret and display results
    snprintf(buff, MAX_STRING_LENGTH, "%-6d %s ", id, (repeat ? "R" : " "));
    
    if (IS_TRUSTED(ch))
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%-10s ", random ? "Yes" : author);
   
    if (duration != -1)
    {
      ct = time(0);
      timer = real_time_countdown(ct, timethen, duration*60);
      snprintf(cdtime, MAX_STRING_LENGTH, "%2d:%02d:%02d", timer.day * 24 + timer.hour, timer.minute, timer.second);
    }
    else
      snprintf(cdtime, MAX_STRING_LENGTH, "%-8s", "Forever");
    snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%-8s ", cdtime);
    
    if (racewar == 0)
      strcpy(rw, "All");
    else if (racewar == RACEWAR_GOOD)
      strcpy(rw, "Good");
    else if (racewar == RACEWAR_EVIL)
      strcpy(rw, "Evil");
    else if (racewar == RACEWAR_UNDEAD)
      strcpy(rw, "Undead");
    else if (racewar == RACEWAR_NEUTRAL)
      strcpy(rw, "Neutral");
    snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%-7s ", rw);

    *pname = '\0';

    if (IS_TRUSTED(ch))
    {
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%-6s ", boon_types[type].type);
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%-9s ", boon_options[option].option);
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%9.2f ", criteria);
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%9.2f ", criteria2);
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%10.2f ", bonus);
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%7.2f ", bonus2);
      if (pid)
        snprintf(pname, MAX_STRING_LENGTH, "%s", get_player_name_from_pid(pid));
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%-10s ", pname);
      snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "\r\n &+CDescription&n: ");
    }
    
    // Description of boon for mortal view
    switch(type)
    {
      case BTYPE_LEVEL:
	{
	  snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, (int)bonus);
	  if ((int)bonus != -1)
	    snprintf(bufftype + strlen(bufftype), MAX_STRING_LENGTH - strlen(bufftype), " (up to %d)", (int)bonus);
	  if (bonus2)
	    snprintf(bufftype + strlen(bufftype), MAX_STRING_LENGTH - strlen(bufftype), " and bypass epics");
	  break;
	}
      case BTYPE_EXP:
	{
	  snprintf(bufftype, MAX_STRING_LENGTH, "%s", boon_types[type].desc);
	  break;
	}
      case BTYPE_EXPM:
	{
	  snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, (int)(bonus*100));
	  break;
	}
      case BTYPE_EPIC:
      case BTYPE_STATS:
      case BTYPE_POINT:
	{
	  snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, (int)bonus);
	  break;
	}
      case BTYPE_CASH:
	{
	  snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, coin_stringv(bonus));
	  break;
	}
      case BTYPE_POWER:
	{
	  int aff = 0, bit = 0;
	  aff = (int)bonus;
	  bit = (int)bonus2;
	  if (aff == 1)
	    snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, affected1_bits[bit].flagLong);
	  else if (aff == 2)
	    snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, affected2_bits[bit].flagLong);
	  else if (aff == 3)
	    snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, affected3_bits[bit].flagLong);
	  else if (aff == 4)
	    snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, affected4_bits[bit].flagLong);
	  else if (aff == 5)
	    snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, affected5_bits[bit].flagLong);
	  else
	    snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, "Invalid Affect");
	  break;
	}
      case BTYPE_SPELL:
	{
	  if (!skills[(int)bonus].name)
	    snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, "Invalid Spell");
	  else
	    snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, skills[(int)bonus].name);
	  break;
	}
      case BTYPE_STAT:
	{
	  snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, attr_names[(int)bonus].name);
	  break;
	}
      case BTYPE_ITEM:
  {
    if( real_object((int)bonus) >= 0 )
    {
      snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, obj_index[real_object((int)bonus)].desc2 );
    }
    else
    {
      snprintf(bufftype, MAX_STRING_LENGTH, boon_types[type].desc, "&+RBUGGY ITEM VNUM&n" );
    }
    break;
  }
      default:
	{
	  if (type >= MAX_BTYPE)
	  {
	    snprintf(bufftype, MAX_STRING_LENGTH, "Error, type is invalid.");
	    break;
	  }
	  snprintf(bufftype, MAX_STRING_LENGTH, "%s", boon_types[type].desc);
	  break;
	}
    }
    
    switch (option)
    {
      case BOPT_FRAG:
      case BOPT_FRAGS:
	{
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, criteria);
	  break;
	}
      case BOPT_LEVEL:
	{
	  if (criteria == 0)
	    snprintf(buffoption, MAX_STRING_LENGTH, " when you raise a level.");
	  else
	    snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, (int)criteria);
	  break;
	}
      case BOPT_CARGO:
      case BOPT_AUCTION:
	{
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, (int)criteria);
	  break;
	}
      case BOPT_NONE:
      case BOPT_ZONE:
	{
	  i = 0;
	  while (i <= top_of_zone_table)
	  {
	    if (zone_table[i].number == (int)criteria)
	      break;
	    else
	      i++;
	  }
	  if (i > top_of_zone_table)
	  {
	    snprintf(buffoption, MAX_STRING_LENGTH, "Error, invalid zone number.");
	    break;
	  }
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, zone_table[i].name);
	  break;
	}
      case BOPT_MOB:
	{
	  int r_num = 0;
	  P_char mob;
	  if ((int)criteria2 < 0 ||
	      (r_num = real_mobile((int)criteria2)) < 0 ||
	      !(mob = read_mobile(r_num, REAL)))
	  {
	    snprintf(buffoption, MAX_STRING_LENGTH, "Error, can't read mobile.");
	    break;
	  }
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, (int)criteria, J_NAME(mob));
	  extract_char(mob);
	  break;
	}
      case BOPT_RACE:
	{
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, (int)criteria, race_names_table[(int)criteria2].ansi);
	  break;
	}
      case BOPT_GH:
	{
	  Guildhall *gh;
          if ((gh = Guildhall::find_by_id((int)criteria)) == NULL)
	  {
	    snprintf(buffoption, MAX_STRING_LENGTH, "&+W'%d' is not a valid guildhall ID.&n", (int)criteria);
	    break;
	  }
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, gh->get_assoc()->get_name().c_str() );
	  break;
	}
      case BOPT_NEXUS:
	{
	  //debug("type: %d, option: %d, criteria: %.2f, bonus: %.2f", type, option, criteria, bonus);
	  NexusStoneInfo nexus;
	  if (!nexus_stone_info(criteria, &nexus))
	  {
	    snprintf(buffoption, MAX_STRING_LENGTH, "&+W'%d' is not a valid nexus stone ID.&n", (int)criteria);
	    break;
	  }
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, nexus.name.c_str());
	  break;
	}
      case BOPT_OP:
	{
	  Building *building;
	  if ((building = get_building_from_id((int)criteria)) == NULL)
	  {
	    snprintf(buffoption, MAX_STRING_LENGTH, "&+W'%d' is not a valid outpost ID.&n", (int)criteria);
	    break;
	  }
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc,
	      continent_name(world[building->location()].continent));
	  break;
	}
      case BOPT_CTF:
      case BOPT_CTFB:
	{
	  snprintf(buffoption, MAX_STRING_LENGTH, boon_options[option].desc, (int)criteria);
	  break;
	}
      default:
	{
	  if (option >= MAX_BOPT)
	  {
	    snprintf(buffoption, MAX_STRING_LENGTH, "Error, option is invalid.");
	    break;
          }
	  snprintf(buffoption, MAX_STRING_LENGTH, "%s", boon_options[option].desc);
	  break;
	}
    }

    snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%s %s", bufftype, buffoption);
    snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "&n\r\n");
    send_to_char(buff, ch);
  }
 
  send_to_char_f(ch, "Displaying %d result(s).\r\n", count);
  
  mysql_free_result(res);
  
  return TRUE;
}

int create_boon(BoonData *bdata)
{
  int id;

  if (!bdata)
  {
    debug("create_boon(): NULL bdata passed to function");
    return FALSE;
  }

  if (count_boons(TRUE, FALSE) >= MAX_BOONS)
  {
    debug("Maximum number of boons has been reached.  Aborting create_boon().");
    return FALSE;
  }

  if (qry("INSERT INTO boons (time, duration, racewar, type, opt, criteria, criteria2, bonus, bonus2, random, author, active, pid, rpt) VALUES " \
      "(%d, %d, %d, %d, %d, %f, %f, %f, %f, %d, '%s', 1, '%d', '%d')",
      time(0), bdata->duration, bdata->racewar, bdata->type, bdata->option, bdata->criteria, bdata->criteria2, bdata->bonus, bdata->bonus2, bdata->random, bdata->author.c_str(), bdata->pid, bdata->repeat))
  {

    // Get the new ID
    if (qry("SELECT MAX(id) FROM boons"))
    {
      MYSQL_RES *res = mysql_store_result(DB);
    
      if (mysql_num_rows(res) < 1)
      {
        mysql_free_result(res);
        return FALSE;
      }

      MYSQL_ROW row = mysql_fetch_row(res);
      bdata->id = atoi(row[0]);
      mysql_free_result(res);
    }

    boon_notify(bdata->id, NULL, BN_CREATE);

    return TRUE;
  }
  
  return FALSE;
}

int create_boon_progress(BoonProgress *bpg)
{
  if (!bpg)
  {
    debug("create_boon_progress(): NULL bpg passed to function");
    return FALSE;
  }

  if (qry("INSERT into boons_progress (boonid, pid, counter) VALUES (%d, %d, %f)",
	bpg->boonid, bpg->pid, bpg->counter))
  {
    if (qry("SELECT MAX(id) FROM boons_progress"))
    {
      MYSQL_RES *res = mysql_store_result(DB);
      if (mysql_num_rows(res) < 1)
      {
	mysql_free_result(res);
	return FALSE;
      }

      MYSQL_ROW row = mysql_fetch_row(res);
      bpg->id = atoi(row[0]);
      mysql_free_result(res);
    }

    return TRUE;
  }

  return FALSE;
}

int create_boon_shop_entry(BoonShop *bshop)
{
  if (!bshop)
  {
    debug("create_boon_shop_entry(): NULL bshop passed to function");
    return FALSE;
  }

  if (!qry("INSERT into boons_shop (pid, points, stats) VALUES (%d, %d, %d)",
	bshop->pid, bshop->points, bshop->stats))
  {
    return FALSE;
  }
  else
    return TRUE;
}

int remove_boon(int id)
{
  //if (!qry("DELETE FROM boons WHERE id = %d", id))
  // Gona leave boons on the DB for history lookup purposes
  if (!qry("UPDATE boons SET active='0', duration='0' WHERE id='%d'", id))
  {
    return 0;
  }
  return 1;
}

int extend_boon(int id, int extend, const char *name)
{
  if (!qry("SELECT time, duration, active FROM boons WHERE id = %d", id))
  {
    debug("extend_boon() can't read from db");
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return FALSE;
  }

  MYSQL_ROW row = mysql_fetch_row(res);
  
  int timethen = atoi(row[0]);
  int duration = atoi(row[1]);
  int active = atoi(row[2]);

  mysql_free_result(res);

  // -1 duration = !expiration, no need to extend
  if (duration == -1)
  {
    debug("%s is trying to extend a boon that has no expiration.\r\n", name);
    return FALSE;
  }

  int calculate = MAX(0, timethen+(duration*60)-time(0));
  int ct = calculate + time(0);
  duration = extend;

  if(!qry("UPDATE boons SET time = '%d', duration = '%d', active = '1', author = '*%s' WHERE id = '%d'", ct, duration, name, id))
  {
    debug("extend_boon(): failed to update boon");
    return FALSE;
  }
 
  if (active)
    boon_notify(id, NULL, BN_EXTEND);
  else
    boon_notify(id, NULL, BN_REACTIVATE);

  return TRUE;
}

void boon_notify(int id, P_char ch, int action)
{
  char buff[MAX_STRING_LENGTH];
  P_desc d;
  int pid = 0;

  if (!action)
  {
    debug("boon_notify() called with invalid action");
    return;
  }

  BoonData bdata;
  if (!get_boon_data(id, &bdata))
  {
    debug("boon_nofity() called with invalid id");
    return;
  }

  for (d = descriptor_list; d; d = d->next)
  {
    if( !d->connected && d->character && IS_SET(d->character->specials.act2, PLR2_BOON)
      && (IS_TRUSTED(d->character) || (!bdata.racewar || (GET_RACEWAR(d->character) == bdata.racewar))
      && (!bdata.pid || (bdata.pid == GET_PID(d->character)))) )
    {
      if (ch && ch != d->character)
      {
        continue;
      }
      switch (action)
      {
      case BN_CREATE: // Might become annoying
        snprintf(buff, MAX_STRING_LENGTH, "&+CYou qualify for a new boon (#%d) that has been created.&n\r\n", bdata.id);
        break;
      case BN_REACTIVATE:
  	    snprintf(buff, MAX_STRING_LENGTH, "&+CYou qualify for a boon (#%d) that has been reactivated.&n\r\n", bdata.id);
	      break;
      case BN_EXTEND:
        snprintf(buff, MAX_STRING_LENGTH, "&+CThe duration for Boon # %d has been extended.&n\r\n", bdata.id);
        break;
      case BN_NOTCH: // Progress notification
        BoonProgress bpg;
        if (!get_boon_progress_data(bdata.id, GET_PID(d->character), &bpg))
        {
          return;
        }
        if( bdata.option == BOPT_RACE )
        {
          char tmp[MAX_STRING_LENGTH];
          if( (int)bdata.criteria2 < 0 || (int)bdata.criteria2 > LAST_RACE )
          {
            snprintf(tmp, MAX_STRING_LENGTH, "Invalid Race");
          }
          else
          {
            snprintf(tmp, MAX_STRING_LENGTH, "%s", race_names_table[(int)bdata.criteria2].ansi);
          }
          snprintf(buff, MAX_STRING_LENGTH, "&+CYou have killed %d of %d %s&+C(s) for boon # %d.&n\r\n", (int)bpg.counter, (int)bdata.criteria, race_names_table[(int)bdata.criteria2].ansi, bdata.id);
        }
        else if( bdata.option == BOPT_MOB )
        {
          int r_num = 0;
          char tmp[MAX_STRING_LENGTH];
          P_char mob;

          if( (int)bdata.criteria2 > 0 && (r_num = real_mobile((int)bdata.criteria2)) > 0
            && (mob = read_mobile(r_num, REAL)) )
	        {
            snprintf(tmp, MAX_STRING_LENGTH, "%s", J_NAME(mob));
            extract_char(mob);
  	      }
          else
          {
            snprintf(tmp, MAX_STRING_LENGTH, "Invalid Mob");
          }
          snprintf(buff, MAX_STRING_LENGTH, "&+CYou have killed %d of %d %s&+C(s) for boon # %d.&n\r\n", (int)bpg.counter, (int)bdata.criteria, tmp, bdata.id);
        }
        else if( bdata.option == BOPT_FRAGS )
        {
          snprintf(buff, MAX_STRING_LENGTH, "&+CYou have obtained %.2f out of %.2f frags for boon # %d.&n\r\n", bpg.counter, bdata.criteria, bdata.id);
        }
        else if( bdata.option == BOPT_NONE ) // neverending progression
        {
          snprintf(buff, MAX_STRING_LENGTH, "&+CYou gain some bonus experience.&n\r\n");
        }
        break;
      case BN_COMPLETE: // Completion notification
        snprintf(buff, MAX_STRING_LENGTH, "&+CYou have completed boon # %d.&n\r\n", bdata.id);
        break;
      case BN_VOID: // boon_remove()
        snprintf(buff, MAX_STRING_LENGTH, "&+CBoon # %d is no longer available.&n\r\n", bdata.id);
        break;
      case BN_EXPIRE: // Expired notification
        // TODO: Might just make this only if you have a progress entry for it.
        // or if you're currently in the zone, or if you're currently in the nexus, etc...
        snprintf(buff, MAX_STRING_LENGTH, "&+CBoon # %d has expired.&n\r\n", bdata.id);
        break;
      default:
        break;
      }
      send_to_char(buff, d->character);
    }
  }
  return;
}

void boon_randomize(P_char ch, char *argument)
{
  send_to_char_f(ch, "Randomizing boon list with argument: %s.\r\n", argument);
  send_to_char_f(ch, "Under construction.\r\n" );
  return;
}

// Called from game loop
void boon_maintenance()
{
  BoonData bdata;
  int i, expire;
  int id[MAX_BOONS];

  for (i = 0; i < MAX_BOONS; i++)
    id[i] = 0;

  if (!qry("SELECT id FROM boons WHERE active = '1'"))
  {
    debug("boon_maintenance(): can't read from db");
    return;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return;
  }

  MYSQL_ROW row;
  
  i = 0;
  while ((row = mysql_fetch_row(res)))
  {
    id[i++] = atoi(row[0]);
  }
  
  mysql_free_result(res);

  for (i = 0; id[i]; i++)
  {
    zero_boon_data(&bdata);
    get_boon_data(id[i], &bdata);

    // check durations and expire if necessesary
    expire = FALSE;
    if ((bdata.duration != -1) &&
        (bdata.time + (bdata.duration*60)) < time(0))
    {
      boon_notify(bdata.id, NULL, BN_EXPIRE);
      remove_boon(bdata.id);
      expire = 2;
    }

    // Check status of current boon criteria, ie is an epic zone complete, or goodie nexus already goodie, etc..
    //boon_notify(bdata.id, NULL, BN_VOID);
    // Check boon completability
    switch (bdata.option)
    {
      case BOPT_ZONE:
	{
	  // is epic zone already complete?
	  if (epic_zone_done_now(bdata.criteria))
	    expire = TRUE;
	  break;
	}
      case BOPT_NEXUS:
	{
	  NexusStoneInfo nexus;
	  if (!nexus_stone_info(bdata.criteria, &nexus))
	    break;
	  if ((nexus.align == 3 && bdata.racewar == RACEWAR_GOOD) ||
	      (nexus.align == -3 && bdata.racewar == RACEWAR_EVIL))
	    expire = TRUE;
	  break;
	}
      case BOPT_CTFB:
	{
	  // If this is based on a boon type flag, and that flag has expired
	  if (ctfdata[(int)bdata.criteria].type == CTF_BOON &&
	      ctfdata[(int)bdata.criteria].room == 0)
	  {
	    expire = TRUE;
	  }
	  if (expire)
	  {
	    ctf_delete_flag((int)bdata.criteria);
	    ctfdata[(int)bdata.criteria].room = 0;
	  }
	  break;
	}
      case BOPT_CTF:
      case BOPT_LEVEL:
      case BOPT_CARGO:
      case BOPT_AUCTION:
      case BOPT_OP: // others of same racewar side can capture outpost
      case BOPT_GH:
      case BOPT_FRAGS:
      case BOPT_FRAG:
      case BOPT_RACE:
      case BOPT_MOB:
      case BOPT_NONE:
      default:
	break;
    }
    if (expire == TRUE)
    {
      boon_notify(bdata.id, NULL, BN_VOID);
      remove_boon(bdata.id);
    }
  }


  // Check state of random boons
  boon_random_maintenance();

  return;
}

void boon_random_maintenance()
{
  return;
  BoonData bdata;
  int i, j;
  int id[MAX_BOONS];
  int r[MAX_BOONS];
  
  // assure appropriate levels of random boons in game
  for (i = 0; i < MAX_BOONS; i++)
  {
    id[i] = 0;
    r[i] = 0;
  }


  if (!qry("SELECT id, random FROM boons WHERE active = '1' & random > '0'"))
  {
    debug("boon_maintenance(): can't read from db");
    return;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return;
  }

  MYSQL_ROW row;
  
  i = 0;
  while ((row = mysql_fetch_row(res)))
  {
    id[i] = atoi(row[0]);
    r[i] = atoi(row[1]);
    i++;
  }
  
  mysql_free_result(res);

  // loop through random standards list and load missing boons
  for (i = 1; random_std[i].id; i++)
  {
    for (j = 0; r[j]; j++)
      if (r[j] == random_std[i].id)
	break;
    if (r[j])
      continue;
    // if we didn't find it, load one up
    zero_boon_data(&bdata);
   
    bdata.duration = 120;
    bdata.racewar = random_std[i].racewar;
    bdata.type = boon_data[random_std[i].boon_data].type;
    bdata.option = boon_data[random_std[i].boon_data].option;
    bdata.random = i;
    bdata.active = 1;
    bdata.repeat = 1;

    // refer to boon_data struct for which we're adding in 
    switch (random_std[i].boon_data)
    {
      case 0: // BTYPE_EXPM, BOPT_NONE
	{
	  bdata.criteria = boon_get_random_zone(j);
	  bdata.criteria2 = 0;
	  bdata.bonus = number(20, 50);
	  bdata.bonus2 = 0;
	  break;
	}
      default:
	continue;
    }
    // set boon
    // create_boon(&bdata);
  }
}

int boon_get_random_zone(int std)
{
  if (!random_std[std].id)
    return 0;
  return random_std[std].id;
}

// The function placed throughout the code to check for completion of boons
// and checks to see if a player qualifies for an active boon.  If so, it will
// call boon_notify() to update progress, or notify of completion and apply
// the appropriate bonus.
// This function doesn't update the boon itself to inactive incase others
// in the group have completed the boon as well.  That will be handled at the
// bottom of boon_maintenance().
// Note: BOPT_RACE and BOPT_MOB both are called after victim is set to STAT_DEAD.
void check_boon_completion(P_char ch, P_char victim, double data, int option)
{
  BoonData bdata;
  BoonShop bshop;
  char buff[MAX_STRING_LENGTH];
  char dbqry[MAX_STRING_LENGTH];
  int i;
  int id[MAX_BOONS];

  // We don't use IS_ALIVE just in case "Die 5 times" boon or something.
  if( !ch || IS_NPC(ch) || option < 0 || option >= MAX_BOPT )
  {
    return;
  }

  if( !IS_SET(ch->specials.act2, PLR2_BOON) )
  {
    return;
  }

  for( i = 0; i < MAX_BOONS; i++ )
  {
    id[i] = 0;
  }

  *buff = '\0';
  // Modify the SQL search based on the option
  if( option == BOPT_NONE )
  {
    snprintf(buff, MAX_STRING_LENGTH, " AND (criteria = '%d')", ROOM_ZONE_NUMBER(ch->in_room));
  }
  else if( option == BOPT_ZONE )
  {
    snprintf(buff, MAX_STRING_LENGTH, " AND (criteria = '%d')", (int)data);
  }
  else if( option == BOPT_LEVEL )
  {
    snprintf(buff, MAX_STRING_LENGTH, " AND (criteria = '%d' OR criteria = '0')", GET_LEVEL(ch));
  }
  else if( option == BOPT_MOB )
  {
    if( IS_NPC(victim) && !IS_PC_PET(victim) )
    {
      snprintf(buff, MAX_STRING_LENGTH, " AND (criteria2 = '%d')", GET_VNUM(victim));
    }
    else
    {
      return;
    }
  }
  else if( option == BOPT_RACE )
  {
    // Kill 1 dragon -> What if ch is goodie and victim = evil's illus dragon
    // racewar( ch, victim ) will always be FALSE since victim is dead.
    // So, TRUE && !TRUE || FALSE = FALSE .. ?
    // However, for a NPC pet.. TRUE && !FALSE || FALSE = TRUE .. ?
//    if( IS_NPC(victim) && !IS_PC_PET(victim) || racewar(ch, victim) )
    // What we really want is NPC that isn't a pet at all, or a PC on a different racewar side.
    //   We don't want a current pet, or a NPC with the conjured pet flag that's no longer charmed.
    //   We don't want to give a good a bonus for killing another good (or evil for evil).
    if( (IS_NPC(victim) && !get_linked_char(victim, LNK_PET) && !affected_by_spell(victim, TAG_CONJURED_PET))
      || (IS_PC(victim) && GET_RACEWAR(ch) != GET_RACEWAR(victim)) )
    {
      snprintf(buff, MAX_STRING_LENGTH, " AND (criteria2 = '%d')", GET_RACE(victim));
    }
    else
    {
      return;
    }
  }
  else if( option == BOPT_FRAG )
  {
    snprintf(buff, MAX_STRING_LENGTH, " AND (criteria <= '%f')", data);
  }
  //else if (option == BOPT_FRAGS) // No need for this, we check below in progress
  //else if (option == BOPT_GH) // not imped
  else if( option == BOPT_OP || option == BOPT_NEXUS || option == BOPT_CTF || option == BOPT_CTFB )
  {
    snprintf(buff, MAX_STRING_LENGTH, " AND (criteria = '%d')", (int)data);
  }

  // Perform the search
  snprintf(dbqry, MAX_STRING_LENGTH, "SELECT id FROM boons WHERE opt = '%d' AND active = '1' AND (racewar = '0' OR racewar = '%d') AND (pid = '0' OR pid = '%d')%s", option, GET_RACEWAR(ch), GET_PID(ch), buff);
  if( !qry(dbqry) )
  {
    debug("check_boon_completion(): can't read from db");
    debug(dbqry);
    return;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  // Nothing found, oh well
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return;
  }

  MYSQL_ROW row;

  i = 0;
  while ((row = mysql_fetch_row(res)))
  {
    id[i++] = atoi(row[0]);
  }

  mysql_free_result(res);

  // Run through results and apply bonuses!
  for( i = 0; id[i]; i++ )
  {
    zero_boon_data(&bdata);
    get_boon_data(id[i], &bdata);

    // This should never happen, but..
    if( bdata.option != option )
    {
      debug("check_boon_completion: bdata.option != option");
      continue;
    }

    // First we deal with the progress table
    BoonProgress bpg;
    int counter = 0;

    // try and get the entry for the player for this boon id
    if( !get_boon_progress_data(bdata.id, GET_PID(ch), &bpg) )
    {
      // it doesn't exist, we create one, for anyone...
      // this is for tracking purposes for if the option is repeatable or not.
      if( bdata.option == BOPT_FRAGS )
      {
        counter = data;
      }
      else if( bdata.option == BOPT_RACE || bdata.option == BOPT_MOB )
      {
        counter = 1;
      }
      // else counter = 0 to pass it's first completion.
      bpg.id = 0;
      bpg.boonid = bdata.id;
      bpg.pid = GET_PID(ch);
      bpg.counter = counter;
      // and create...
      if( !create_boon_progress(&bpg) )
      {
        debug("check_boon_completion(): failed to create bpg DB entry");
        logit(LOG_DEBUG, "check_boon_completion(): failed to create bpg DB entry: id: %d|%d, pid: %d|%d, counter: %d|%d.",
          bpg.id, 0, bpg.boonid, bdata.id, bpg.pid, GET_PID(ch), bpg.counter, counter );
        send_to_char("Failed to create a boon progress entry, please contact an Immortal.\r\n", ch);
        continue;
      }
    }
    // otherwise, we've found the boon, if we're dealing with progression
    else if( boon_options[option].progress && bpg.counter != -1 )
    {
      // let's notch up their counter appropriately
      if( !qry("UPDATE boons_progress SET counter = '%f' WHERE id = '%d'", (bpg.counter+(bdata.option == BOPT_FRAGS ? data : 1.0)), bpg.id) )
      {
        debug("check_boon_completion(): failed to update bpg DB entry: counter: %f, id: %d.",
          bpg.counter+(bdata.option == BOPT_FRAGS ? data : 1.0), bpg.id );
        logit(LOG_DEBUG, "check_boon_completion(): failed to update bpg DB entry: counter: %f, id: %d.",
          bpg.counter+(bdata.option == BOPT_FRAGS ? data : 1.0), bpg.id );
        send_to_char("Failed to update your progress entry for this boon, please contact an Immortal.\r\n", ch);
        continue;
      }
      bpg.counter += (bdata.option == BOPT_FRAGS ? data : 1.0);
    }
    // or if we're dealing with non progression and it's not repeatable (counter = -1)
    else
    {
      // They've completed this non repeatable boon before, so no bonuses for them!
      if( bpg.counter == -1 )
      {
        continue;
      }
    }
    // and notify if its a progression boon option
    if (boon_options[option].progress)
    {
      boon_notify(bdata.id, ch, BN_NOTCH);
      if (bpg.counter < bdata.criteria)
      {
        continue;
      }
      else
      {
        qry("UPDATE boons_progress SET counter = '%d' WHERE id = '%d'", (bdata.repeat ? 0 : -1), bpg.id);
      }
    }
    else
      qry("UPDATE boons_progress SET counter = '%d' WHERE id = '%d'", (bdata.repeat ? 0 : -1), bpg.id);

    // OK, if we've made it here, they successfully completed a boon
    // Apply bonuses
    switch( bdata.type )
    {
      case BTYPE_EXPM:
        //boon_notify(id, BN_NOTCH);
        // This should only be called on kills now (since it's limited to type mob or race).
        gain_exp(ch, victim, (int)(data * bdata.bonus), EXP_BOON);
        break;
      case BTYPE_EXP:
        boon_notify(bdata.id, ch, BN_COMPLETE);
        gain_exp(ch, victim, (int)bdata.bonus, EXP_BOON);
        break;
      case BTYPE_EPIC:
        boon_notify(bdata.id, ch, BN_COMPLETE);
        gain_epic(ch, EPIC_BOON, GET_PID(ch), bdata.bonus);
        break;
      case BTYPE_CASH:
        boon_notify(bdata.id, ch, BN_COMPLETE);
        send_to_char_f(ch, "Your bank receives a deposit of %s&n.\r\n", coin_stringv(bdata.bonus));
        GET_BALANCE_PLATINUM(ch) += (bdata.bonus / 1000);
        GET_BALANCE_GOLD(ch) += (((int)bdata.bonus % 1000) / 100);
        GET_BALANCE_SILVER(ch) += ((((int)bdata.bonus % 1000) % 100) / 10);
        GET_BALANCE_COPPER(ch) += ((((int)bdata.bonus % 1000) % 100) % 10);
        break;
      case BTYPE_LEVEL:
        boon_notify(bdata.id, ch, BN_COMPLETE);
        if ((GET_LEVEL(ch)+1) > (int)bdata.bonus)
        {
          send_to_char("&+WWell done, unfortionately you've already surpassed the max level this boon will grant.&n\r\n", ch);
          continue;
        }
        if( (int)bdata.bonus2 )
        {
          //bypass epics
          GET_EXP(ch) -= new_exp_table[GET_LEVEL(ch) + 1];
          advance_level(ch);
        }
        else
        {
          // We'll give them a free level, so long as they have the epics for it.
          epic_free_level(ch);
        }
        break;
      case BTYPE_POWER:
        struct affected_type af;
        bzero(&af, sizeof(af));
        af.type = TAG_BOON;
        af.duration = 60;
        *buff = '\0';
        if( (int)bdata.bonus == 1 )
        {
          snprintf(buff, MAX_STRING_LENGTH, "%s", affected1_bits[(int)bdata.bonus2].flagLong);
          af.bitvector = 1 << (int)bdata.bonus2;
        }
        if( (int)bdata.bonus == 2 )
        {
          snprintf(buff, MAX_STRING_LENGTH, "%s", affected2_bits[(int)bdata.bonus2].flagLong);
          af.bitvector2 = 1 << (int)bdata.bonus2;
        }
        if( (int)bdata.bonus == 3 )
        {
          snprintf(buff, MAX_STRING_LENGTH, "%s", affected3_bits[(int)bdata.bonus2].flagLong);
          af.bitvector3 = 1 << (int)bdata.bonus2;
        }
        if( (int)bdata.bonus == 4 )
        {
          snprintf(buff, MAX_STRING_LENGTH, "%s", affected4_bits[(int)bdata.bonus2].flagLong);
          af.bitvector4 = 1 << (int)bdata.bonus2;
        }
        if( (int)bdata.bonus == 5 )
        {
          snprintf(buff, MAX_STRING_LENGTH, "%s", affected5_bits[(int)bdata.bonus2].flagLong);
          af.bitvector5 = 1 << (int)bdata.bonus2;
        }
        if (!*buff)
        {
          snprintf(buff, MAX_STRING_LENGTH, "Undefined");
        }
        affect_to_char_with_messages(ch, &af, "&+CYour bonus power fa&+cdes away...&n\r\n", NULL);
        boon_notify(bdata.id, ch, BN_COMPLETE);
        send_to_char_f(ch, "You have been granted the power of %s for a while.\r\n", buff);
        break;
      case BTYPE_SPELL:
        boon_notify(bdata.id, ch, BN_COMPLETE);
        ((*skills[(int)bdata.bonus].spell_pointer) (56, ch, NULL, SPELL_TYPE_SPELL, ch, NULL));
        break;
      case BTYPE_STAT:
        boon_notify(bdata.id, ch, BN_COMPLETE);
        if( bdata.bonus == STR )
        {
          ch->base_stats.Str = BOUNDED(0, ch->base_stats.Str+1, 100);
          send_to_char("You feel stronger!\r\n", ch);
        }
        else if( bdata.bonus == DEX )
        {
          ch->base_stats.Dex = BOUNDED(0, ch->base_stats.Dex+1, 100);
          send_to_char("You feel more dextrous!\r\n", ch);
        }
        else if( bdata.bonus == AGI )
        {
          ch->base_stats.Agi = BOUNDED(0, ch->base_stats.Agi+1, 100);
          send_to_char("You feel more agile!\r\n", ch);
        }
        else if( bdata.bonus == CON )
        {
          ch->base_stats.Con = BOUNDED(0, ch->base_stats.Con+1, 100);
          send_to_char("You feel ten years younger!\r\n", ch);
        }
        else if( bdata.bonus == POW )
        {
          ch->base_stats.Pow = BOUNDED(0, ch->base_stats.Pow+1, 100);
          send_to_char("Your mind suddenly feels ten times as powerful!\r\n", ch);
        }
        else if( bdata.bonus == INT )
        {
          ch->base_stats.Int = BOUNDED(0, ch->base_stats.Int+1, 100);
          send_to_char("You feel smarter! Man, you were a real dumbass before.\r\n", ch);
        }
        else if( bdata.bonus == WIS )
        {
          ch->base_stats.Wis = BOUNDED(0, ch->base_stats.Wis+1, 100);
          send_to_char("You feel wiser!\r\n", ch);
        }
        else if( bdata.bonus == CHA )
        {
          ch->base_stats.Cha = BOUNDED(0, ch->base_stats.Cha+1, 100);
          send_to_char("Suddenly one of the pimples on your face dissapears!\r\n", ch);
        }
        else if( bdata.bonus == LUCK )
        {
          ch->base_stats.Luk = BOUNDED(0, ch->base_stats.Luk+1, 100);
          send_to_char("You feel as if you could roll Triple Tiamat's at the slots...\r\n", ch);
        }
        else if( bdata.bonus == KARMA )
        {
          ch->base_stats.Kar = BOUNDED(0, ch->base_stats.Kar+1, 100);
          send_to_char("You feel strange.\r\n", ch);
        }
        else
        {
          debug("check_boon_completion(): Bad bonus value %d.", bdata.bonus );
          logit(LOG_DEBUG, "check_boon_completion(): Bad bonus value %d.", bdata.bonus );
        }
        affect_total(ch, TRUE);
        break;
      case BTYPE_STATS:
        boon_notify(bdata.id, ch, BN_COMPLETE);
        if( !get_boon_shop_data(GET_PID(ch), &bshop) )
        {
          bshop.pid = GET_PID(ch);
          bshop.points = 0;
          bshop.stats = (int)bdata.bonus;
          if( !create_boon_shop_entry(&bshop) )
          {
          	debug("check_boon_completion(): failed to create shop DB entry");
          	send_to_char("Failed to create your shop data, please contact an Immortal.\r\n", ch);
          	continue;
          }
        }
        else
        {
          if( !qry("UPDATE boons_shop SET stats = '%d' WHERE pid = '%d'", (bshop.stats + (int)bdata.bonus), GET_PID(ch)) )
          {
            debug("check_boon_completion(): Failed to update shop DB entry: stats %d, pid %d.",(bshop.stats + (int)bdata.bonus), GET_PID(ch) );
            logit(LOG_DEBUG, "check_boon_completion(): Failed to update shop DB entry: stats %d, pid %d.",(bshop.stats + (int)bdata.bonus), GET_PID(ch) );
            send_to_char("Failed to update your shop data for this boon, please contact an Immortal.\r\n", ch);
            continue;
          }
        }
        break;
      case BTYPE_POINT:
        boon_notify(bdata.id, ch, BN_COMPLETE);
        if( !get_boon_shop_data(GET_PID(ch), &bshop) )
        {
          bshop.pid = GET_PID(ch);
          bshop.points = (int)bdata.bonus;
          bshop.stats = 0;
          if (!create_boon_shop_entry(&bshop))
          {
            debug("check_boon_completion(): Failed to create shop DB entry: pid %d.", GET_PID(ch) );
            logit(LOG_DEBUG, "check_boon_completion(): Failed to create DB entry: pid %d.", GET_PID(ch) );
            send_to_char("Failed to create your shop data, please contact an Immortal.\r\n", ch);
            continue;
          }
        }
        else
        {
          if (!qry("UPDATE boons_shop SET points = '%d' WHERE pid = '%d'", (bshop.points + (int)bdata.bonus), GET_PID(ch)))
          {
            debug("check_boon_completion(): Failed to update shop DB entry: points %d, pid %d.", (bshop.points + (int)bdata.bonus), GET_PID(ch) );
            logit(LOG_DEBUG, "check_boon_completion(): Failed to update shop DB entry: points %d, pid %d.", (bshop.points + (int)bdata.bonus), GET_PID(ch) );
            send_to_char("Failed to update your shop data for this boon, please contact an Immortal.\r\n", ch);
            continue;
          }
        }
        break;
      case BTYPE_ITEM:
        P_obj reward_item;
        reward_item = read_object( (int)bdata.bonus, VIRTUAL );
        if( reward_item == NULL )
        {
          debug("check_boon_completion: Failed loading object vnum %d for %s.", (int)bdata.bonus, J_NAME(ch) );
          send_to_char_f(ch, "&+RCould not load item vnum %d! &+B:&+R(&n\r\n", (int)bdata.bonus);
        }
        else
        {
          obj_to_char( reward_item, ch );
          send_to_char_f(ch, "&+WYou receive %s&+W! &+B:&+W)&n\r\n", reward_item->short_description );
        }
        break;
      default:
          debug("check_boon_completion: Boon #%d: Unknown bonus type (%d) for %s.", bdata.id, bdata.type, J_NAME(ch) );
          send_to_char_f(ch, "&+RCould not find a reward for boon #%d! &+B:&+R(&n\r\n", bdata.id);
        break;
    }
    if( bdata.type != BTYPE_EXPM )
    {
      debug("%s has completed boon # %d.", GET_NAME(ch), bdata.id);
    }
    // Check and expire boon's if they are player targeted and not repeatable..
    // all though boon_progress wont let them continue if its not repeatable,
    // we don't need to leave it on the boon list if it's done.
    if( bdata.pid == GET_PID(ch) && !bdata.repeat )
    {
      remove_boon(bdata.id);
      boon_notify(bdata.id, NULL, BN_VOID);
    }
#if defined(CTF_MUD) && (CTF_MUD == 1)
    // if its a boon ctf flag and not repeatable boon remove flag
    if (bdata.option == BOPT_CTFB)
    {
      if (bdata.repeat)
      {
        obj_to_room(ctfdata[(int)bdata.criteria].obj, real_room0(ctfdata[(int)bdata.criteria].room));
        send_to_room_f(real_room0(ctfdata[(int)bdata.criteria].room), "%s &n appears.\r\n", (ctfdata[(int)bdata.criteria].obj)->short_description);
      }
      else
      {
        ctf_delete_flag((int)bdata.criteria);
        ctfdata[(int)bdata.criteria].room = 0;
      }
    }
#endif
  }
  return;
}
