#ifndef __EPIC_BONUS_H__
#define __EPIC_BONUS_H__

#define EPIC_BONUS_NONE 	0
#define EPIC_BONUS_CARGO	1
#define EPIC_BONUS_SHOP		2
#define EPIC_BONUS_EXP		3
#define EPIC_BONUS_EPIC_POINT	4
#define EPIC_BONUS_HEALTH	5
#define EPIC_BONUS_MOVES	6

struct EpicBonusData {
  int pid;
  int type;
  const char *time;
};

struct epic_bonus_data
{
  int type;
  const char *name;
  const char *description;
};

void do_epic_bonus(P_char, char*, int);
void epic_bonus_help(P_char);
void epic_bonus_set(P_char, int);
float get_epic_bonus_max(int);
bool get_epic_bonus_data(P_char, EpicBonusData*);
float get_epic_bonus(P_char, int);
#endif
