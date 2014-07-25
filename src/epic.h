#ifndef __EPIC_H__
#define __EPIC_H__

#include <vector>
#include <string>
using namespace std;

#define EPIC_SMALL_STONE    358
#define EPIC_LARGE_STONE    359
#define EPIC_MONOLITH       360

#define EPIC_ZONE_TYPE_NONE     0
#define EPIC_ZONE_TYPE_SMALL    1
#define EPIC_ZONE_TYPE_LARGE    2
#define EPIC_ZONE_TYPE_MONOLITH 3

#define EPIC_ZONE_ALIGNMENT_MAX 5
#define EPIC_ZONE_ALIGNMENT_MIN -5

#define EPIC_REWARD_ITEM      1
#define EPIC_REWARD_SKILL     2
#define EPIC_REWARD_LEVEL     3
#define EPIC_MAX_PRESTIGE    12

struct epic_trophy_data 
{
  epic_trophy_data(int _z, int _c) : zone_number(_z), count(_c) {}
  int zone_number;
  int count;
};

struct epic_zone_data 
{
  epic_zone_data(int _number, string _name, float _freq, int _alignment) : number(_number), name(_name), freq(_freq), alignment(_alignment) {}
  int number;
  string name;
  float freq;
  int alignment;
  int displayed_alignment() const;
};

struct epic_zone_completion
{
  epic_zone_completion(int _number, int _done_at, int _delta) : number(_number), done_at(_done_at), delta(_delta) {}
	int number;
	int done_at;
  int delta;
};

void create_epic_skills();
void epic_initialization();
void do_epic(P_char ch, char *arg, int cmd);
void do_epic_trophy(P_char ch, char *arg, int cmd);
void do_epic_zones(P_char ch, char *arg, int cmd);
void do_epic_share(P_char ch, char *arg, int cmd);
vector<string> get_epic_players(int racewar);
vector<epic_trophy_data> get_epic_zone_trophy(P_char ch);
int modify_by_epic_trophy(P_char ch, int amount, int zone_number);
void gain_epic(P_char, int type, int data, int amount);
void group_gain_epic(P_char, int type, int data, int amount);
void epic_frag(P_char, int victim_pid, int amount);
const char *epic_prestige(P_char);
void init_guild_frags();
void epic_feed_artifacts(P_char ch, int epics, int epic_type);
void do_epic_reset(P_char ch, char *arg, int cmd);
void do_epic_reset_norefund(P_char ch, char *arg, int cmd);
void do_infuse(P_char ch, char *arg, int cmd);
struct affected_type *get_epic_task(P_char ch);
bool has_epic_task(P_char ch);

int epic_stone(P_obj obj, P_char ch, int cmd, char *arg);
void epic_stone_one_touch(P_obj obj, P_char ch, int epic_value);
void epic_free_level(P_char ch);
void epic_stone_level_char(P_obj obj, P_char ch);
void epic_stone_set_affect(P_char ch);
void epic_stone_feed_artifacts(P_obj obj, P_char ch);
int epic_stone_payout(P_obj obj, P_char ch);
void epic_stone_absorb(P_obj obj);

void update_epic_zone_alignment(int zone_number, int delta);
float get_epic_zone_alignment_mod(int zone_number, ubyte racewar);
void update_epic_zone_mods();
void update_epic_zone_frequency(int zone_number);
vector<epic_zone_data> get_epic_zones();
float get_epic_zone_frequency_mod(int zone_number);
void epic_zone_erase_touch(int);
bool epic_zone_done_now(int zone_number);
bool epic_zone_done(int zone_number);
void epic_zone_balance();
int zone2saveable(int zone_index);
int saveable2zone(int saved_zone);
void epic_choose_new_epic_task(P_char ch);

void clear_racial_skills(P_char ch);

#define GET_EPIC_POINTS(ch) (IS_NPC(ch) ? 0 : ch->only.pc->epics)

#define EPIC_ZONE 0
#define EPIC_PVP 1
#define EPIC_ELITE_MOB 2
#define EPIC_QUEST 3
#define EPIC_RANDOM_ZONE 4
#define EPIC_NEXUS_STONE 5
#define EPIC_SHIP_PVP 6
#define EPIC_BOON 7

#define SPILL_BLOOD -10

#endif
