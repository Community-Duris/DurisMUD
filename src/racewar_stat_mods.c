#include <vector>
using namespace std;

#include "racewar_stat_mods.h"

#include "defines.h"
#include "utils.h"
#include "structs.h"

#define REG_STAT 0
#define MAX_STAT 1

// global to hold current racewar stat mods
vector<struct stat_data> racewar_stat_mods(2);

// add racewar stat mods to char affects. called from apply_affs()
int add_racewar_stat_mods(P_char ch, struct hold_data *affs) {
  if( !affs || !ch ) 
    raise(SIGSEGV);
  
  if( !IS_PC(ch) || ( GET_RACEWAR(ch) != RACEWAR_GOOD && GET_RACEWAR(ch) != RACEWAR_EVIL ) )
    return FALSE;
  
  int i;
  
  if( GET_RACEWAR(ch) == RACEWAR_GOOD )
    i = 0;
  else if( GET_RACEWAR(ch) == RACEWAR_EVIL )
    i = 1;
  
  affs->c_Str += racewar_stat_mods[i].Str;
  affs->m_Str += racewar_stat_mods[i].Str;
  affs->c_Dex += racewar_stat_mods[i].Dex;
  affs->m_Dex += racewar_stat_mods[i].Dex;
  affs->c_Agi += racewar_stat_mods[i].Agi;
  affs->m_Agi += racewar_stat_mods[i].Agi;
  affs->c_Con += racewar_stat_mods[i].Con;
  affs->m_Con += racewar_stat_mods[i].Con;
  affs->c_Pow += racewar_stat_mods[i].Pow;
  affs->m_Pow += racewar_stat_mods[i].Pow;
  affs->c_Int += racewar_stat_mods[i].Int;
  affs->m_Int += racewar_stat_mods[i].Int;
  affs->c_Wis += racewar_stat_mods[i].Wis;
  affs->m_Wis += racewar_stat_mods[i].Wis;
  affs->c_Cha += racewar_stat_mods[i].Cha;
  affs->m_Cha += racewar_stat_mods[i].Cha;
  affs->c_Luc += racewar_stat_mods[i].Luk;
  affs->m_Luc += racewar_stat_mods[i].Luk;
  
  return TRUE;
}

int set_racewar_stat_mod(int racewar, int stat_affect, int regular_stat, int max_stat)
{
  if( racewar != RACEWAR_GOOD && racewar != RACEWAR_EVIL ) 
    return FALSE;

  int i;

  if( racewar == RACEWAR_GOOD )
    i = 0;
  else if( racewar == RACEWAR_EVIL )
    i = 1;
  
  switch( stat_affect )
  {
    case APPLY_STR:
      racewar_stat_mods[i].Str = regular_stat;
      break;
      
    case APPLY_DEX:
      racewar_stat_mods[i].Dex = regular_stat;
      break;
      
    case APPLY_INT:
      racewar_stat_mods[i].Int = regular_stat;
      break;
      
    case APPLY_WIS:
      racewar_stat_mods[i].Wis = regular_stat;
      break;
      
    case APPLY_AGI:
      racewar_stat_mods[i].Agi = regular_stat;
      break;
      
    case APPLY_CON:
      racewar_stat_mods[i].Con = regular_stat;
      break;
      
    case APPLY_POW:
      racewar_stat_mods[i].Pow = regular_stat;
      break;
      
    case APPLY_CHA:
      racewar_stat_mods[i].Cha = regular_stat;
      break;
      
    case APPLY_LUCK:
      racewar_stat_mods[i].Luk = regular_stat;
      break;
  }
  
  return TRUE;
}

void reset_racewar_stat_mods()
{
  racewar_stat_mods[0].Str = 0;
  racewar_stat_mods[0].Dex = 0;
  racewar_stat_mods[0].Int = 0;
  racewar_stat_mods[0].Wis = 0;
  racewar_stat_mods[0].Agi = 0;
  racewar_stat_mods[0].Con = 0;
  racewar_stat_mods[0].Pow = 0;
  racewar_stat_mods[0].Cha = 0;
  racewar_stat_mods[0].Luk = 0;

  racewar_stat_mods[1].Str = 0;
  racewar_stat_mods[1].Dex = 0;
  racewar_stat_mods[1].Int = 0;
  racewar_stat_mods[1].Wis = 0;
  racewar_stat_mods[1].Agi = 0;
  racewar_stat_mods[1].Con = 0;
  racewar_stat_mods[1].Pow = 0;
  racewar_stat_mods[1].Cha = 0;
  racewar_stat_mods[1].Luk = 0;
}
