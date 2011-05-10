#ifndef __BOON_H__
#define __BOON_H__

#include "config.h"
#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

// Boon DB Tables
// 
// table name: boons - where we store all the boon's
// id, time, duration, racewar, type, opt, criteria, criteria2, bonus, random, active, author, pid
// 
// table name: boons_progress - progress DB, one pid to boonid
// id, boonid, pid, counter
//
// table name: boons_shop - data for the boon shop, points, redeem data
// id, UNIQUE(pid), points, stats

#define MAX_BOONS		99

struct BoonData {
  int id;
  int time;
  int duration;
  int racewar;
  int type;
  int option;
  double criteria;
  double criteria2;
  double bonus;
  double bonus2;
  int random;
  int active;
  string author;
  int pid;
  int repeat;
};

struct BoonProgress {
  int id;
  int boonid;
  int pid;
  double counter;
};

struct BoonShop {
  int id;
  int pid;
  int points;
  int stats;
  int cash;
};

// This will help the random generation keep a standard minimum boons
// for each level range.
struct BoonRandomStandards {
  int racewar;	// racewar side
  int low;	// low level range
  int high;	// high level range
  int boon_data;// which boon_data we are refering to
};

struct boon_data_struct {
  int type;
  int option;
  int level;
};

struct boon_types_struct {
  const char *type;
  const char *desc;
};

struct boon_options_struct {
  const char *option;
  const char *desc;
  int progress;
};

// Boon types			   BONUS
#define BTYPE_NONE      0	// N/A
#define BTYPE_EXPM	1	// Exp multiplier
#define BTYPE_EXP	2	// Receive exp upon completion
#define BTYPE_EPIC	3	// Receive a number of epics
#define BTYPE_CASH	4	// Cash bonus in copper
#define BTYPE_LEVEL	5	// Receive free level
#define BTYPE_POWER	6	// Receive an affect for duration
#define BTYPE_SPELL	7	// Receive a spell for a duration
#define BTYPE_STAT	8	// Receive a stat point powerup in a specified attribute
#define BTYPE_STATS     9       // Receive a # of stat powerups to apply to your choice
#define BTYPE_POINT	10	// Receive boon points (for the boon shop)
#define MAX_BTYPE	11	// Last + 1

// Boon type options		   CRITERIA
#define BOPT_NONE	0	// Zone # (for bonus exp)
#define BOPT_ZONE	1	// Zone # (complete the zone)
#define BOPT_LEVEL	2	// Level # (gain a level to complete)
#define BOPT_MOB	3	// Mob vnum (mob to kill)
#define BOPT_RACE	4	// Mob race
#define BOPT_FRAG	5	// Frag Threshhold (gain a frag above threshhold)
#define BOPT_FRAGS	6	// Frag Limit (gain so many frags in a set time period)
#define BOPT_GH		7	// Guildhall ID (sack GH)
#define BOPT_OP		8	// Outpost ID (sack OP)
#define BOPT_NEXUS	9	// Nexus ID (sack Nexus)
#define BOPT_CARGO	10	// Cargo Goal (Sell so much cargo in a set time period)
#define BOPT_AUCTION    11	// Auction Goal (Auction so much eq in a set time period)
#define BOPT_CTF	12	// CTF flag ID (found in ctf.c ctfdata struct)
#define BOPT_CTFB	13	// Create a temp ctf flag
#define MAX_BOPT	14	// Last + 1

// Arguments to BoonData struct for validate_boon_data()
// Some of these might not be needed, can adjust after code is done
#define BARG_ALL	0	//  All data
#define BARG_ID 	1	//  bdata->id
#define BARG_TIME	2	//  bdata->time
#define BARG_DURATION	3	//  bdata->duration
#define BARG_RACEWAR	4	//  bdata->racewar
#define BARG_TYPE	5	//  bdata->type
#define BARG_OPTION	6	//  bdata->option
#define BARG_CRITERIA	7	//  bdata->criteria
#define BARG_CRITERIA2  8	//  bdata->criteria2 (optional)
#define BARG_BONUS	9	//  bdata->bonus
#define BARG_BONUS2	10	//  bdata->bonus2
#define BARG_RANDOM	10	//  bdata->random
#define BARG_AUTHOR	11	//  bdata->author
#define BARG_ACTIVE	12	//  bdata->active
#define BARG_PID	13	//  bdata->pid
#define BARG_REPEAT	14	//  bdata->repeat
#define MAX_BARG	15	// Last + 1

// Notify types for boon_notify()
#define BN_NONE		0
#define BN_CREATE	1
#define BN_REACTIVATE	2
#define BN_EXTEND	3
#define BN_NOTCH	4
#define BN_COMPLETE	5
#define BN_VOID		6
#define BN_EXPIRE	7
#define MAX_BN		8

bool check_boon_combo(int, int, int);
int get_boon_level(int, int);
int get_valid_boon_type(char*);
int get_valid_boon_option(char*);
int is_boon_valid(int);
int count_boons(int, int);
void zero_boon_data(BoonData*);
bool get_boon_data(int, BoonData*);
bool get_boon_progress_data(int, int, BoonProgress*);
bool get_boon_shop_data(int, BoonShop*);
int validate_boon_data(BoonData*, int);
int parse_boon_args(P_char, BoonData*, char*);
void do_boon(P_char, char*, int);
int boon_display(P_char, char*);
int create_boon(BoonData*);
int create_boon_progress(BoonProgress*);
int create_boon_shop_entry(BoonShop*);
int remove_boon(int);
int extend_boon(int, int, const char*);
void boon_notify(int, P_char, int);
void boon_randomize(P_char, char*);
void boon_maintenance();
void boon_random_maintenance();
void check_boon_completion(P_char, P_char, double, int);

#endif // __BOON_H__
