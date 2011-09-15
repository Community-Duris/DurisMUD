/*
 *  trophy.c
 *  Duris
 *
 *  Created by Torgal on 1/6/08.
 *
 */

#ifndef _TROPHY_H_
#define _TROPHY_H_

struct zone_trophy_data {
  int zone_number;
  int exp;
};

void update_zone_trophy(P_char ch, int zone_number, int XP);
int get_zone_exp(P_char ch, int zone_number);
long calc_min_zone_exp(P_char);
vector<struct zone_trophy_data> get_zone_trophy(P_char ch);
float modify_exp_by_zone_trophy(P_char ch, int type, float XP);
void save_zone_trophy(P_char ch);
void load_zone_trophy(P_char ch);

#define ZONE_TROPHY(ch) ( ch->only.pc->zone_trophy )

typedef vector<struct zone_trophy_data>::iterator zone_trophy_iterator;

#endif // _TROPHY_H_

