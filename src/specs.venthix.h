#ifndef _SPECS_VENTHIX_H_
#define _SPESC_VENTHIX_H_

#include <vector>
using namespace std;

#include "structs.h"

struct ZombieGame
{
  ZombieGame();
  ZombieGame(P_obj _generator);
  ~ZombieGame();

  int load();
  int unload();

  int id;

  static int next_id;

  P_obj generator;

  vector<P_char> zombies;
  int zombies_to_load;

  int zombies_alive()
  {
    if (zombies.size() < 1)
      return 0;

    return (int)zombies.size();
  }
};

int zgame_load_zombie(P_obj obj);
void zgame_clear_zombies(P_obj obj);
int zg_count_zombies(P_obj obj);
int zgame_mob_proc(P_char ch, P_char pl, int cmd, char *arg);
ZombieGame* get_zgame_from_obj(P_obj obj);


#endif
