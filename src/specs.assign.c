/*
   ***************************************************************************
   *  File: spec.assign.c                                      Part of Duris *
   *  Usage: assign function pointers for special procs.                     *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.    *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <stdio.h>
#include <dlfcn.h>

#include "db.h"
#include "events.h"
#include "prototypes.h"
#include "structs.h"
#include "specs.prototypes.h"
#include "utils.h"
#include "proc-libs.h"
#include "vnum.obj.h"
#include "specs.jubilex.h"
#include "specs.winterhaven.h"
#include "specs.zion.h"
#include "specs.barovia.h"
#include "specs.eth2.h"
#include "specs.snogres.h"
#include "specs.ravenloft.h"
#include "specs.keleks.h"
#include "specs.caertannad.h"
#include "specs.firep.h"
#include "outposts.h"
#include "buildings.h"

extern int top_of_world;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;

//Added gellz Procs
int gellz_test_obj_procs(P_obj obj, P_char ch, int cmd, char *argument);
int magic_deck(P_obj obj, P_char ch, int cmd, char *argument);

int unmulti_altar(P_obj obj, P_char ch, int cmd, char *arg);
int harpy_gate(P_obj obj, P_char ch, int cmd, char *arg);
int block_dir(P_char ch, P_char pl, int cmd, char *arg);

int ship_shop_proc(int room, P_char ch, int cmd, char *arg);
void assign_ship_crew_funcs();

#ifdef SHLIB

/*
   first entry is the name of the lib on the disk (excluding the
   trailing .so).  The second entry SHOULD ALWAYS BE NULL.  Last entry
   should have a NULL name (so I can find the end of the list)
 */

dynamic_procs dynam_proc_list[] = {
  {"specs.myranth", NULL},
  {"specs.bloodstone", NULL},
  {NULL, NULL}
};

/*
   I'm gonna be ultra-paranoid here and do redundant sanity checks on
   the lib name.... fucking this up will cause some MAJOR havoc..
 */

int load_proc_lib(char *name)
{
  int      i;
  char     buf[180];
  void    *handle;

  for (i = 0; dynam_proc_list[i].name; i++)
    if (!str_cmp(name, dynam_proc_list[i].name))
    {
      if (dynam_proc_list[i].handle)
        return 0;
      sprintf(buf, "%s.so", dynam_proc_list[i].name);
      handle = dlopen(buf, RTLD_NOW);
      if (!handle)
      {
        logit(LOG_EXIT, "lib: %s: %s", buf, dlerror());
        raise(SIGSEGV);;
      }
      dynam_proc_list[i].handle = handle;
      return 1;
    }
  return 0;
}

int unload_proc_lib(char *name)
{
  int      i;
  char     buf[180];
  const char *error;

  for (i = 0; dynam_proc_list[i].name; i++)
    if (!str_cmp(name, dynam_proc_list[i].name))
    {
      if (!dynam_proc_list[i].handle)
        return 0;
      dlclose(dynam_proc_list[i].handle);
      error = dlerror();
      if (error)
      {
        logit(LOG_EXIT, "lib: %s: %s", buf, error);
        raise(SIGSEGV);;
      }
      dynam_proc_list[i].handle = NULL;
      return 1;
    }
  return 0;
}

void load_all_proc_libs(void)
{
  int      i;
  char     name[180];
  void    *handle;

  for (i = 0; dynam_proc_list[i].name; i++)
  {
    sprintf(name, "%s.so", dynam_proc_list[i].name);
    handle = dlopen(name, RTLD_NOW);
    if (!handle)
    {
      logit(LOG_EXIT, "lib: %s: %s", name, dlerror());
      raise(SIGSEGV);;
    }
    dynam_proc_list[i].handle = handle;
  }
}

int mob_proc_stub(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  else
    return FALSE;
}

int obj_proc_stub(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  else
    return FALSE;
}

int room_proc_stub(int room, P_char ch, int cmd, char *arg)
{
  return FALSE;
}
#endif /*
          SHLIB
        */

/*
   ********************************************************************
   *  Assignments                                                        *
   ********************************************************************
 */

/*
   assign special procedures to mobiles
 */

void assign_mobiles(void)
{
  int      i;

  /* speed wipe entry to Tharn Rifts */
  mob_index[real_mobile0(500129)].func.mob = Baltazo;
  obj_index[real_object0(500055)].func.obj = tharnrifts_portal;
  /* scorched valley */
  mob_index[real_mobile0(71223)].func.mob = block_up;
  mob_index[real_mobile0(71259)].func.mob = yeenoghu;
  
  /* useless god toys */
  mob_index[real_mobile0(444)].func.mob = io_assistant;

  /* rename char mobiles */
  mob_index[real_mobile0(76013)].func.mob = mob_do_rename_hook;

  /* mob clear epic task */
  mob_index[real_mobile0(22428)].func.mob = clear_epic_task_spec;
  
  /* monk quest mob */
  //mob_index[real_mobile0(55178)].func.mob = monk_remort;

  /* player castles */

  /* Shipyards */
  mob_index[real_mobile0(43101)].func.mob = money_changer;
  mob_index[real_mobile0(43110)].func.mob = money_changer; 

  /* Ailvio */
 // mob_index[real_mobile0(29303)].func.mob = bandage_mob;
 // mob_index[real_mobile0(29304)].func.mob = bandage_reward_mob;
  mob_index[real_mobile0(29236)].func.mob = newbie_spellup_mob;
  mob_index[real_mobile0(29238)].func.mob = newbie_spellup_mob;
  mob_index[real_mobile0(29232)].func.mob = newbie_spellup_mob;
  mob_index[real_mobile0(29265)].func.mob = newbie_spellup_mob;
   
  /* Ako */
  mob_index[real_mobile0(3715)].func.mob = ako_hypersquirrel;
  mob_index[real_mobile0(3701)].func.mob = ako_songbird;
  mob_index[real_mobile0(3716)].func.mob = ako_vulture;
  mob_index[real_mobile0(3721)].func.mob = ako_wildmare;
  mob_index[real_mobile0(3720)].func.mob = ako_cow;

 /* Orogs */
  mob_index[real_mobile0(45818)].func.mob = rentacleric;
  mob_index[real_mobile0(45833)].func.mob = money_changer;
  
 /* Undead HT */ 
  mob_index[real_mobile0(90387)].func.mob = money_changer;
  
 /* WinterHaven */

  world[real_room0(55201)].funct = pet_shops; /* WH Mercenaries */
  world[real_room0(55203)].funct = pet_shops; /* WH Mounts */
  world[real_room0(55126)].funct = welfare_well;
  mob_index[real_mobile0(55128)].func.mob = world_quest; /* WH Questmaster */
  //mob_index[real_mobile0(55021)].func.mob = winterhaven_shout_one;
  //mob_index[real_mobile0(55022)].func.mob = winterhaven_shout_two; 
  mob_index[real_mobile0(55008)].func.mob = wh_janitor; 
  mob_index[real_mobile0(55009)].func.mob = wh_janitor; 
  mob_index[real_mobile0(55010)].func.mob = wh_janitor; 
  mob_index[real_mobile0(55020)].func.mob = wh_guard;
  mob_index[real_mobile0(55021)].func.mob = wh_guard;
  mob_index[real_mobile0(55022)].func.mob = wh_guard;
  mob_index[real_mobile0(55022)].func.mob = wh_guard;
  mob_index[real_mobile0(55003)].func.mob = wh_guard;
//  mob_index[real_mobile0(55184)].func.mob = wh_guard;
  mob_index[real_mobile0(55057)].func.mob = wh_guard;
  mob_index[real_mobile0(55126)].func.mob = rentacleric;
  mob_index[real_mobile0(55500)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55246)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55501)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55502)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55503)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55504)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55505)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55506)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55507)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55508)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55509)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55510)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55511)].func.mob = wh_corpse_to_object;
  mob_index[real_mobile0(55512)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55513)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55514)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55515)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55516)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55517)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55518)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55519)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55520)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55521)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55550)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55551)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55552)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55553)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(55554)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(500102)].func.mob = wh_corpse_to_object; 
  mob_index[real_mobile0(500103)].func.mob = wh_corpse_to_object; 
  obj_index[real_object0(55007)].func.obj = no_kill_priest_obj;
  mob_index[real_mobile0(55184)].func.mob = newbie_spellup_mob; // Priest of WH
  mob_index[real_mobile0(55124)].func.mob = money_changer; 
//  mob_index[real_mobile0(55026)].func.mob = volo_teleport;
//  mob_index[real_mobile0()].func.mob = mob_death_proc;
  mob_index[real_mobile0(19617)].func.mob = tiamat_human_to_rareloads;
  mob_index[real_mobile0(6816)].func.mob = dragonnia_heart;
  mob_index[real_mobile0(29025)].func.mob = lanella_heart;
  mob_index[real_mobile0(22024)].func.mob = cerberus_load;
  mob_index[real_mobile0(55264)].func.mob = leviathan;

  /* Aravne */
  mob_index[real_mobile0(21673)].func.mob = wh_corpse_to_object;

  /* Myrabolus */
  mob_index[real_mobile0(82521)].func.mob = money_changer;

  /* NAX */
  mob_index[real_mobile0(37764)].func.mob = world_quest; // Nax

  /* arcium */
  world[real_room0(22439)].funct = inn;
  world[real_room0(29605)].funct = inn;

  /* halfcut hills */
  mob_index[real_mobile0(27009)].func.mob = crossbow_ambusher;

  /* highway */
  mob_index[real_mobile0(41900)].func.mob = mir_spider;

  mob_index[real_mobile0(42172)].func.mob = red_wyrm_shout;
  mob_index[real_mobile0(42173)].func.mob = white_wyrm_shout;
  mob_index[real_mobile0(42174)].func.mob = blue_wyrm_shout;

  mob_index[real_mobile0(42166)].func.mob = amphisbean;
  mob_index[real_mobile0(42167)].func.mob = amphisbean;

  /* claw cavern */
  for (i = 80706; i <= 80734; i++)
  {
    if ((i == 80711) || (i == 80712) || (i == 80714) ||
        ((i > 80724) && (i < 80728)))
      continue;

    mob_index[real_mobile0(i)].func.mob = clwcvrn_crys_die;
  }

  mob_index[real_mobile0(80735)].func.mob = clwcvrn_golem_shatter;
  mob_index[real_mobile0(80739)].func.mob = clwcvrn_protect;
  mob_index[real_mobile0(80726)].func.mob = claw_cavern_drow_mage;

  /* darkfall_forest */
  mob_index[real_mobile0(75013)].func.mob = archer;

  /* Undead Map (map10) */
  mob_index[real_mobile0(210003)].func.mob = charon;

  /* breale */
  mob_index[real_mobile0(2613)].func.mob = breale_townsfolk;
  mob_index[real_mobile0(2614)].func.mob = breale_townsfolk;
  mob_index[real_mobile0(2619)].func.mob = breale_townsfolk;
  mob_index[real_mobile0(2620)].func.mob = breale_townsfolk;
  mob_index[real_mobile0(2621)].func.mob = breale_townsfolk;
  mob_index[real_mobile0(2622)].func.mob = breale_townsfolk;
  mob_index[real_mobile0(2623)].func.mob = breale_townsfolk;
  mob_index[real_mobile0(2624)].func.mob = breale_townsfolk;
  mob_index[real_mobile0(2625)].func.mob = breale_townsfolk;

  /* Braddistock Mansion */
  mob_index[real_mobile0(1314)].func.mob = braddistock;
  obj_index[real_object0(1372)].func.obj = jet_black_maul;

  /* Orc town? */
  mob_index[real_mobile0(97545)].func.mob = troll_slave;
  mob_index[real_mobile0(97509)].func.mob = stray_dog;
  mob_index[real_mobile0(97534)].func.mob = hardworking_fisherman;
  mob_index[real_mobile0(97554)].func.mob = orcish_jailkeeper;
  mob_index[real_mobile0(97539)].func.mob = orcish_woman;

  /* Vella's Bordello */
  mob_index[real_mobile0(93600)].func.mob = Vella_slut;
  mob_index[real_mobile0(93602)].func.mob = sex_crazed_prostitute;
  mob_index[real_mobile0(93603)].func.mob = well_built_prostitute;
  mob_index[real_mobile0(93605)].func.mob = Padh_bouncer;
  mob_index[real_mobile0(93607)].func.mob = Vem_rouge;
  mob_index[real_mobile0(93611)].func.mob = sleezy_prostitute;
  mob_index[real_mobile0(93613)].func.mob = tired_young_man;

#if 0
  /* Northern Verzanan */
/*   mob_index[real_mobile0 (3080)].func.mob = jailkeeper; */
  mob_index[real_mobile0(3081)].func.mob = cityguard;
  mob_index[real_mobile0(3082)].func.mob = verzanan_guard_three;
  mob_index[real_mobile0(3083)].func.mob = cityguard;
#endif
  mob_index[real_mobile0(1250)].func.mob = archer;
  mob_index[real_mobile0(1251)].func.mob = archer;
  mob_index[real_mobile0(1252)].func.mob = archer;
  mob_index[real_mobile0(1253)].func.mob = archer;
  mob_index[real_mobile0(1254)].func.mob = archer;

  /* Sylvandawn */
  mob_index[real_mobile0(8028)].func.mob = cityguard;
  mob_index[real_mobile0(8034)].func.mob = cityguard;
  mob_index[real_mobile0(8047)].func.mob = cityguard;

  /* transparent tower */
  mob_index[real_mobile0(16205)].func.mob = transp_tow_acerlade;

  /* necro dracoliches */
  mob_index[real_mobile0(3)].func.mob = necro_dracolich;
  mob_index[real_mobile0(4)].func.mob = necro_dracolich;
  mob_index[real_mobile0(5)].func.mob = necro_dracolich;
  mob_index[real_mobile0(6)].func.mob = necro_dracolich;
  mob_index[real_mobile0(7)].func.mob = necro_dracolich;
  mob_index[real_mobile0(8)].func.mob = necro_dracolich;
  mob_index[real_mobile0(9)].func.mob = necro_dracolich;
  mob_index[real_mobile0(10)].func.mob = necro_dracolich;
  mob_index[real_mobile0(1201)].func.mob = necro_dracolich;

  /* Illusionist */

  mob_index[real_mobile0(1106)].func.mob = shadow_monster;      //shadow monster proc
  mob_index[real_mobile0(1108)].func.mob = illus_dragon;        //dragon proc
  mob_index[real_mobile0(1107)].func.mob = insects;     //spell_insect so they poison
  mob_index[real_mobile0(650)].func.mob = illus_titan;  //titan disappears when not fighting

  /* Lower God Rooms */
  mob_index[real_mobile0(200)].func.mob = shadow_demon_of_torm;

  /* Main God Rooms */
  mob_index[real_mobile0(1205)].func.mob = sales_spec;
  mob_index[real_mobile0(1228)].func.mob = beavis;
  mob_index[real_mobile0(1229)].func.mob = butthead;
  mob_index[real_mobile0(1230)].func.mob = billthecat;

  /* Kobold Settlement */
  mob_index[real_mobile0(1407)].func.mob = chicken;
  mob_index[real_mobile0(1433)].func.mob = stone_crumble;
  mob_index[real_mobile0(1436)].func.mob = tako_demon;
  mob_index[real_mobile0(1437)].func.mob = kobold_priest;
  mob_index[real_mobile0(1438)].func.mob = stone_golem;

  /* Troll Hills */
  mob_index[real_mobile0(1919)].func.mob = bridge_troll;

/*  mob_index[real_mobile0(6050)].func.mob = citizenship;*/
  mob_index[real_mobile0(36203)].func.mob =
    mob_index[real_mobile0(132599)].func.mob = rentacleric;
  mob_index[real_mobile0(8336)].func.mob =
    mob_index[real_mobile0(95551)].func.mob =
    mob_index[real_mobile0(16512)].func.mob =
    mob_index[real_mobile0(66046)].func.mob =
    mob_index[real_mobile0(6109)].func.mob = justice_clerk;

  /* Evil Spec teachers */
  mob_index[real_mobile0(1520)].func.mob = 0;    // sorc
  mob_index[real_mobile0(22467)].func.mob = 0;   // zerker
  mob_index[real_mobile0(36440)].func.mob = 0;   // assassin
  mob_index[real_mobile0(4401)].func.mob = 0;    // cleric
  mob_index[real_mobile0(1518)].func.mob = 0;    // conj
  mob_index[real_mobile0(1519)].func.mob = 0;    // necro
  mob_index[real_mobile0(97585)].func.mob = 0;   // illus
  mob_index[real_mobile0(36441)].func.mob = 0;   // reaver
  mob_index[real_mobile0(36442)].func.mob = 0;   // bard
  mob_index[real_mobile0(22434)].func.mob = 0;   // warrior  

  /* Undead Spec teachers */
  mob_index[real_mobile0(66238)].func.mob = 0;   // reaver
  mob_index[real_mobile0(200011)].func.mob = 0;  // sorc
  mob_index[real_mobile0(210006)].func.mob = 0;  // warrior
  mob_index[real_mobile0(22415)].func.mob = 0;   // AP
  mob_index[real_mobile0(200012)].func.mob = 0;  // Conj
  mob_index[real_mobile0(22435)].func.mob = 0;   // Mercenary
  mob_index[real_mobile0(90306)].func.mob = 0;   // piper
  mob_index[real_mobile0(90328)].func.mob = 0;   // dreadlord

  /* Goodie Spec teachers */
  mob_index[real_mobile0(1510)].func.mob = 0;    // shaman
  mob_index[real_mobile0(1500)].func.mob = 0;  // sorc
  mob_index[real_mobile0(1512)].func.mob = 0;    // conj
  mob_index[real_mobile0(82507)].func.mob = 0;   // conj
  mob_index[real_mobile0(1501)].func.mob = 0;    // warrior
  mob_index[real_mobile0(1503)].func.mob = 0;    // paladin
  mob_index[real_mobile0(1514)].func.mob = 0;    // assassin
  mob_index[real_mobile0(1509)].func.mob = 0;    // druid
  mob_index[real_mobile0(1507)].func.mob = 0;    // cleric
  mob_index[real_mobile0(1515)].func.mob = 0;    // mercenary
  mob_index[real_mobile0(1502)].func.mob = 0;    // ranger
  mob_index[real_mobile0(82500)].func.mob = 0;   // ranger
  mob_index[real_mobile0(1511)].func.mob = 0;  // necro
  mob_index[real_mobile0(1516)].func.mob = 0;    // bard
  mob_index[real_mobile0(1513)].func.mob = 0;    // thief

  /* All side spec teachers */
  mob_index[real_mobile0(66736)].func.mob = 0;   // shaman
  mob_index[real_mobile0(22420)].func.mob = 0;   // shaman
  mob_index[real_mobile0(1510)].func.mob = 0;    // shaman
  mob_index[real_mobile0(22441)].func.mob = 0;   // thief
  mob_index[real_mobile0(9420)].func.mob = 0;    // thief
  mob_index[real_mobile0(22440)].func.mob = 0;   // assassin




  mob_index[real_mobile0(66732)].func.mob = 0;   // AP
  mob_index[real_mobile0(200321)].func.mob = 0;  // Ethermancer
  mob_index[real_mobile0(9432)].func.mob = 0;    // Warrior
  mob_index[real_mobile0(66731)].func.mob = 0;   // Mercenary
  mob_index[real_mobile0(66631)].func.mob = 0;   // Bard
  mob_index[real_mobile0(9454)].func.mob = 0;    // Alch

  /* Bahamut */
  mob_index[real_mobile0(25700)].func.mob = bahamut;
  /* fooquest */
  mob_index[real_mobile0(65012)].func.mob = fooquest_mob;
  mob_index[real_mobile0(65013)].func.mob = fooquest_boss;

  /* Goodie Highwayman */
  mob_index[real_mobile0(500058)].func.mob = goodie_guardian;

  /* images */
  mob_index[real_mobile0(250)].func.mob = imageproc;

#if 0
  mob_index[real_mobile0(2811)].func.mob = rogue_one;
  mob_index[real_mobile0(2812)].func.mob = mercenary_two;
  mob_index[real_mobile0(2813)].func.mob = youth_one;
  mob_index[real_mobile0(2814)].func.mob = thief;
  mob_index[real_mobile0(2815)].func.mob = homeless_two;
  mob_index[real_mobile0(2816)].func.mob = homeless_one;
  mob_index[real_mobile0(2818)].func.mob = prostitute_one;
  mob_index[real_mobile0(2820)].func.mob = dog_two;
  mob_index[real_mobile0(2824)].func.mob = guild_guard_thirteen;
  mob_index[real_mobile0(2825)].func.mob = assassin_one;
  mob_index[real_mobile0(2827)].func.mob = mercenary_three;
  mob_index[real_mobile0(2829)].func.mob = youth_two;
  mob_index[real_mobile0(2830)].func.mob = brigand_one;
  mob_index[real_mobile0(2831)].func.mob = thief;
  mob_index[real_mobile0(2832)].func.mob = commoner_four;
  mob_index[real_mobile0(2833)].func.mob = commoner_five;
  mob_index[real_mobile0(2834)].func.mob = commoner_six;
  mob_index[real_mobile0(2835)].func.mob = mercenary_three;
  mob_index[real_mobile0(2836)].func.mob = drunk_two;
  /*
     Northern Verzanan
   */
  mob_index[real_mobile0(3006)].func.mob = drunk_two;
  mob_index[real_mobile0(3007)].func.mob = homeless_one;
  mob_index[real_mobile0(3008)].func.mob = crier_one;
  mob_index[real_mobile0(3009)].func.mob = merchant_one;
  mob_index[real_mobile0(3010)].func.mob = farmer_one;
  mob_index[real_mobile0(3011)].func.mob = baker_one;
  mob_index[real_mobile0(3012)].func.mob = baker_two;
  mob_index[real_mobile0(3014)].func.mob = mage_one;
  mob_index[real_mobile0(3018)].func.mob = warrior_one;
  mob_index[real_mobile0(3024)].func.mob = guild_guard_nine;
  mob_index[real_mobile0(3025)].func.mob = guild_guard_ten;
  mob_index[real_mobile0(3026)].func.mob = guild_guard_twelve;
  mob_index[real_mobile0(3027)].func.mob = guild_guard_eleven;
  mob_index[real_mobile0(3030)].func.mob = cleric_one;
  mob_index[real_mobile0(3035)].func.mob = verzanan_guard_two;
  mob_index[real_mobile0(3038)].func.mob = commoner_one;
  mob_index[real_mobile0(3039)].func.mob = commoner_two;
  mob_index[real_mobile0(3042)].func.mob = money_changer;
  mob_index[real_mobile0(3062)].func.mob = dog_one;
  mob_index[real_mobile0(3064)].func.mob = drunk_one;
  mob_index[real_mobile0(3065)].func.mob = homeless_one;
  mob_index[real_mobile0(3066)].func.mob = cat_one;
  mob_index[real_mobile0(3068)].func.mob = blob;
  mob_index[real_mobile0(3069)].func.mob = jester;
  mob_index[real_mobile0(3070)].func.mob = verzanan_guard_one;
  mob_index[real_mobile0(3090)].func.mob = cat_one;

  /*
     Central Verzanan
   */
  mob_index[real_mobile0(3201)].func.mob = mercenary_one;
  mob_index[real_mobile0(3203)].func.mob = drunk_three;
  mob_index[real_mobile0(3204)].func.mob = casino_one;
  mob_index[real_mobile0(3205)].func.mob = casino_two;
  mob_index[real_mobile0(3206)].func.mob = casino_four;
  mob_index[real_mobile0(3207)].func.mob = casino_three;
  mob_index[real_mobile0(3210)].func.mob = mercenary_one;
  mob_index[real_mobile0(3211)].func.mob = piergeiron;
  mob_index[real_mobile0(3212)].func.mob = guard_two;
  mob_index[real_mobile0(3215)].func.mob = park_one;
  mob_index[real_mobile0(3216)].func.mob = park_two;
  mob_index[real_mobile0(3217)].func.mob = park_three;
  mob_index[real_mobile0(3219)].func.mob = park_four;
  mob_index[real_mobile0(3220)].func.mob = park_five;
  mob_index[real_mobile0(3221)].func.mob = park_six;
  mob_index[real_mobile0(3229)].func.mob = guard_one;
  mob_index[real_mobile0(3231)].func.mob = dog_two;
  mob_index[real_mobile0(3232)].func.mob = youth_one;
  mob_index[real_mobile0(3234)].func.mob = tailor_one;
  mob_index[real_mobile0(3235)].func.mob = shopper_one;
  mob_index[real_mobile0(3236)].func.mob = drunk_three;
  mob_index[real_mobile0(3240)].func.mob = shopper_two;
  mob_index[real_mobile0(3242)].func.mob = mercenary_two;
  mob_index[real_mobile0(3243)].func.mob = mercenary_three;
  mob_index[real_mobile0(3244)].func.mob = piergeiron_guard;
#endif
  /*
     Mt. Skelenak (New Moria)
   */
  mob_index[real_mobile0(4070)].func.mob = piercer;
  mob_index[real_mobile0(4120)].func.mob = guild_guard;


  mob_index[real_mobile0(15)].func.mob = witch_doctor;
  mob_index[real_mobile0(21549)].func.mob = llyren;

  /*
     The Underworld
   */
  mob_index[real_mobile0(4480)].func.mob = purple_worm;
  mob_index[real_mobile0(700004)].func.mob = purple_worm;
  mob_index[real_mobile0(4530)].func.mob = piercer;
  mob_index[real_mobile0(4520)].func.mob = underdark_track;
  mob_index[real_mobile0(4510)].func.mob = underdark_track;
  mob_index[real_mobile0(550)].func.mob = underdark_track;
  mob_index[real_mobile0(551)].func.mob = underdark_track;


  mob_index[real_mobile0(210004)].func.mob = undeadcont_track;
  mob_index[real_mobile0(210005)].func.mob = undeadcont_track;
#if 0
  mob_index[real_mobile0(210000)].func.mob = undeadcont_track;
  mob_index[real_mobile0(210001)].func.mob = undeadcont_track;
  mob_index[real_mobile0(210002)].func.mob = undeadcont_track;
#endif

// Newbie guards! - Kvark
  mob_index[real_mobile0(910)].func.mob = newbie_guard_north;
  mob_index[real_mobile0(912)].func.mob = newbie_guard_east;
  mob_index[real_mobile0(911)].func.mob = newbie_guard_south;
  mob_index[real_mobile0(4800)].func.mob = newbie_guard_east;
  mob_index[real_mobile0(913)].func.mob = newbie_guard_west;

  // living earth proc!
  mob_index[real_mobile0(1104)].func.mob = living_stone;
  mob_index[real_mobile0(1105)].func.mob = greater_living_stone;

  // elemental swarm hulks
  /*
  mob_index[real_mobile0(69)].func.mob = elemental_swarm_earth;
  mob_index[real_mobile0(70)].func.mob = elemental_swarm_fire;
  mob_index[real_mobile0(71)].func.mob = elemental_swarm_air;
  mob_index[real_mobile0(72)].func.mob = elemental_swarm_water;
  */


  /*Astral_Tiamat
   */

  mob_index[real_mobile0(19705)].func.mob = guild_guard;

  /*
     Alterian Region
   */
  mob_index[real_mobile0(4812)].func.mob = poison;
  mob_index[real_mobile0(4830)].func.mob = wanderer;

#if 0
  /* Verzanan Harbor */
  mob_index[real_mobile0(5300)].func.mob = fisherman_one;
  mob_index[real_mobile0(5302)].func.mob = fisherman_two;
  mob_index[real_mobile0(5303)].func.mob = sailor_one;
  mob_index[real_mobile0(5305)].func.mob = seaman_one;
  mob_index[real_mobile0(5307)].func.mob = naval_one;
  mob_index[real_mobile0(5308)].func.mob = naval_two;
  mob_index[real_mobile0(5310)].func.mob = merchant_two;
  mob_index[real_mobile0(5311)].func.mob = naval_three;
  mob_index[real_mobile0(5313)].func.mob = lighthouse_one;
  mob_index[real_mobile0(5315)].func.mob = lighthouse_two;
  mob_index[real_mobile0(1210)].func.mob = commoner_three;
  mob_index[real_mobile0(5317)].func.mob = seabird_one;
  mob_index[real_mobile0(5318)].func.mob = seabird_two;
  mob_index[real_mobile0(5320)].func.mob = naval_four;
  mob_index[real_mobile0(5321)].func.mob = artillery_one;

  /* The Guilds of Verzanan */
  mob_index[real_mobile0(5500)].func.mob = guild_guard_one;
  mob_index[real_mobile0(5504)].func.mob = young_paladin_one;
  mob_index[real_mobile0(5505)].func.mob = guild_guard_two;
  mob_index[real_mobile0(5507)].func.mob = wrestler_one;
  mob_index[real_mobile0(5508)].func.mob = young_mercenary_one;
  mob_index[real_mobile0(5511)].func.mob = guild_guard_three;
  mob_index[real_mobile0(5514)].func.mob = young_monk_one;
  mob_index[real_mobile0(5516)].func.mob = selune_one;
  mob_index[real_mobile0(5517)].func.mob = selune_two;
  mob_index[real_mobile0(5518)].func.mob = selune_three;
  mob_index[real_mobile0(5519)].func.mob = selune_four;
  mob_index[real_mobile0(5520)].func.mob = selune_five;
  mob_index[real_mobile0(5521)].func.mob = selune_six;
  mob_index[real_mobile0(5523)].func.mob = bouncer_four;
  mob_index[real_mobile0(5524)].func.mob = guild_guard_four;
  mob_index[real_mobile0(5527)].func.mob = prostitute_one;
  mob_index[real_mobile0(5528)].func.mob = guild_guard_five;
  mob_index[real_mobile0(5531)].func.mob = guild_guard_six;
  mob_index[real_mobile0(5533)].func.mob = young_druid_one;
  mob_index[real_mobile0(5535)].func.mob = guild_guard_seven;
  mob_index[real_mobile0(5537)].func.mob = guild_guard_eight;
  mob_index[real_mobile0(5538)].func.mob = young_necro_one;
  mob_index[real_mobile0(5541)].func.mob = bouncer_two;
  mob_index[real_mobile0(5542)].func.mob = bouncer_three;
  mob_index[real_mobile0(5543)].func.mob = bouncer_one;
#endif
  /*
     dlsc (snik's zone) - temporary for testing
   */
#if 0
  mob_index[real_mobile0(5400)].func.mob = wristthrow_and_gore;
#endif

  /*
     Western Realms
   */
  mob_index[real_mobile0(5701)].func.mob = dryad;
  mob_index[real_mobile0(5702)].func.mob = dryad;
  mob_index[real_mobile0(5739)].func.mob = navagator;

  /* Human HT - Tharnadia */

  world[real_room0(132507)].funct = welfare_well;
  mob_index[real_mobile0(150115)].func.mob = outpost_captain;
  mob_index[real_mobile0(150116)].func.mob = outpost_captain;
  mob_index[real_mobile0(150117)].func.mob = outpost_captain;
  mob_index[real_mobile0(150118)].func.mob = outpost_captain;
  mob_index[real_mobile0(150119)].func.mob = outpost_captain;
  mob_index[real_mobile0(150120)].func.mob = outpost_captain;
  mob_index[real_mobile0(150121)].func.mob = outpost_captain;
  mob_index[real_mobile0(150122)].func.mob = outpost_captain;
  mob_index[real_mobile0(150123)].func.mob = outpost_captain;
  mob_index[real_mobile0(150124)].func.mob = outpost_captain;
  mob_index[real_mobile0(150125)].func.mob = outpost_captain;
  mob_index[real_mobile0(150126)].func.mob = outpost_captain;
  mob_index[real_mobile0(150127)].func.mob = outpost_captain;
  mob_index[real_mobile0(150128)].func.mob = outpost_captain;
  mob_index[real_mobile0(150129)].func.mob = outpost_captain;
  mob_index[real_mobile0(150130)].func.mob = outpost_captain;
  mob_index[real_mobile0(150131)].func.mob = outpost_captain;
  mob_index[real_mobile0(150132)].func.mob = outpost_captain;
  mob_index[real_mobile0(150133)].func.mob = outpost_captain;
  mob_index[real_mobile0(150134)].func.mob = outpost_captain;
  mob_index[real_mobile0(150135)].func.mob = outpost_captain;
  mob_index[real_mobile0(150136)].func.mob = outpost_captain;
  mob_index[real_mobile0(150137)].func.mob = outpost_captain;
  mob_index[real_mobile0(150138)].func.mob = outpost_captain;
  mob_index[real_mobile0(150139)].func.mob = outpost_captain;
  mob_index[real_mobile0(150140)].func.mob = outpost_captain;

  mob_index[real_mobile0(132593)].func.mob = janitor;
  mob_index[real_mobile0(132519)].func.mob = money_changer;
  mob_index[real_mobile0(98718)].func.mob = money_changer;
  mob_index[real_mobile0(82224)].func.mob = money_changer;
  mob_index[real_mobile0(132501)].func.mob = tharn_tall_merchant;
  mob_index[real_mobile0(132509)].func.mob = tharn_beach_guard;
  mob_index[real_mobile0(132510)].func.mob = tharn_male_commoner;
  mob_index[real_mobile0(132511)].func.mob = tharn_female_commoner;
  mob_index[real_mobile0(132537)].func.mob = tharn_female_commoner;
  mob_index[real_mobile0(132539)].func.mob = tharn_female_commoner;
  mob_index[real_mobile0(132532)].func.mob = tharn_human_merchant;
  mob_index[real_mobile0(132514)].func.mob = tharn_lighthouse_attendent;
  mob_index[real_mobile0(132531)].func.mob = tharn_crier_one;
 
 mob_index[real_mobile0(36420)].func.mob = world_quest; // Drow HT
 mob_index[real_mobile0(97540)].func.mob = world_quest; // Ixie
 mob_index[real_mobile0(132555)].func.mob = world_quest;// Tharn
 mob_index[real_mobile0(16553)].func.mob = world_quest; // Woodseer
 
 mob_index[real_mobile0(53670)].func.mob = world_quest; // Sunwell


  /* Hall of the Ancients */
  mob_index[real_mobile0(77714)].func.mob = morkoth_mother;
  mob_index[real_mobile0(77747)].func.mob = akckx;
  mob_index[real_mobile0(77750)].func.mob = human_girl;
  mob_index[real_mobile0(77751)].func.mob = hoa_death;
  mob_index[real_mobile0(77752)].func.mob = hoa_sin;

  /* Vecna */
  mob_index[real_mobile0(130002)].func.mob = vecna_mob_rebirth;
  mob_index[real_mobile0(130003)].func.mob = vecna_mob_rebirth;
  mob_index[real_mobile0(130004)].func.mob = vecna_mob_rebirth;
  mob_index[real_mobile0(130005)].func.mob = vecna_mob_rebirth;
  mob_index[real_mobile0(130030)].func.mob = block_dir;
  mob_index[real_mobile0(130031)].func.mob = block_dir;
  mob_index[real_mobile0(130032)].func.mob = block_dir;
  mob_index[real_mobile0(130033)].func.mob = block_dir;
  mob_index[real_mobile0(130034)].func.mob = block_dir;
  mob_index[real_mobile0(130035)].func.mob = vecnas_fight_proc;
  mob_index[real_mobile0(130016)].func.mob = chressan_shout;
  mob_index[real_mobile0(130028)].func.mob = vecna_black_mass;
  

 mob_index[real_mobile0(67324)].func.mob = world_quest;
 mob_index[real_mobile0(97540)].func.mob = world_quest;
 mob_index[real_mobile0(95517)].func.mob = world_quest;
 mob_index[real_mobile0(85751)].func.mob = world_quest;
 mob_index[real_mobile0(82511)].func.mob = world_quest;
 mob_index[real_mobile0(80194)].func.mob = world_quest;
 mob_index[real_mobile0(80123)].func.mob = world_quest;
 mob_index[real_mobile0(70029)].func.mob = world_quest;
 mob_index[real_mobile0(66041)].func.mob = world_quest;
 mob_index[real_mobile0(45055)].func.mob = world_quest;
 mob_index[real_mobile0(40466)].func.mob = world_quest;
 mob_index[real_mobile0(40410)].func.mob = world_quest;
 mob_index[real_mobile0(30831)].func.mob = world_quest;
 mob_index[real_mobile0(18707)].func.mob = world_quest;
 mob_index[real_mobile0(17022)].func.mob = world_quest;
 mob_index[real_mobile0(11601)].func.mob = world_quest;
 mob_index[real_mobile0(8003)].func.mob = world_quest;
 mob_index[real_mobile0(5755)].func.mob = world_quest;
 mob_index[real_mobile0(1603)].func.mob = world_quest;

 mob_index[real_mobile0(36420)].func.mob = world_quest;
 mob_index[real_mobile0(82229)].func.mob = world_quest;
 mob_index[real_mobile0(82208)].func.mob = world_quest;
 mob_index[real_mobile0(8309)].func.mob = world_quest;

 mob_index[real_mobile0(16553)].func.mob = world_quest;
 mob_index[real_mobile0(16553)].func.mob = world_quest;
 mob_index[real_mobile0(16553)].func.mob = world_quest;
 mob_index[real_mobile0(16553)].func.mob = world_quest;

 mob_index[real_mobile0(5347)].func.mob = world_quest;
 mob_index[real_mobile0(37718)].func.mob = world_quest;
 mob_index[real_mobile0(74060)].func.mob = world_quest;

 mob_index[real_mobile0(132551)].func.mob = world_quest;
 mob_index[real_mobile0(38309)].func.mob = world_quest;
 mob_index[real_mobile0(49064)].func.mob = world_quest;

   mob_index[real_mobile0(132542)].func.mob = tharn_shady_mercenary;
  mob_index[real_mobile0(132543)].func.mob = tharn_shady_youth;
  mob_index[real_mobile0(132544)].func.mob = tharn_jailor;
  mob_index[real_mobile0(132540)].func.mob = tharn_old_man;

  /* Harpy hometown */
  mob_index[real_mobile0(31103)].func.mob = money_changer;

  /* Sarmiz'Duul */

  mob_index[real_mobile0(9445)].func.mob = money_changer;
  mob_index[real_mobile0(9453)].func.mob = erzul_proc;

  /* Undead hometown */

  mob_index[real_mobile0(66208)].func.mob = money_changer;

  /* Aracdrathos */
  mob_index[real_mobile0(36413)].func.mob = money_changer;

  /* New Hope */
 mob_index[real_mobile0(89181)].func.mob = tentacler_death;

  /* Dragonnia */

  mob_index[real_mobile0(6801)].func.mob = demodragon;
  mob_index[real_mobile0(6814)].func.mob = dragon_guard;
  mob_index[real_mobile0(6828)].func.mob = baby_dragon;
  mob_index[real_mobile0(6813)].func.mob = dragon_guard;

  /* Harpy Hometown */
  mob_index[real_mobile0(31108)].func.mob = gargoyle_master;
  mob_index[real_mobile0(31109)].func.mob = harpy_good;
  mob_index[real_mobile0(31124)].func.mob = harpy_evil;

  /* Jademini - Jade Empire */
  mob_index[real_mobile0(77216)].func.mob = archer;

  /* Kimordril - Dwarf HT */
  mob_index[real_mobile0(95506)].func.mob = archer;
  mob_index[real_mobile0(95503)].func.mob = money_changer;
  mob_index[real_mobile0(95535)].func.mob = kimordril_shout;

  /* Ixarkon */
  mob_index[real_mobile0(96449)].func.mob = money_changer;

  /* Bloodstone */
  mob_index[real_mobile0(74073)].func.mob = bs_boss;
  mob_index[real_mobile0(74000)].func.mob = bs_citizen; 
  mob_index[real_mobile0(74001)].func.mob = bs_comwoman;
  mob_index[real_mobile0(74088)].func.mob = bs_brat;
  mob_index[real_mobile0(74003)].func.mob = bs_holyman;
  mob_index[real_mobile0(74004)].func.mob = bs_merchant;
  mob_index[real_mobile0(74005)].func.mob = bs_wino;
  mob_index[real_mobile0(74092)].func.mob = bs_wino;
  mob_index[real_mobile0(74232)].func.mob = bs_watcher;
  mob_index[real_mobile0(74244)].func.mob = bs_guard; 
  mob_index[real_mobile0(74245)].func.mob = bs_squire;
  mob_index[real_mobile0(74245)].func.mob = bs_squire;
  mob_index[real_mobile0(74006)].func.mob = bs_peddler;
  mob_index[real_mobile0(74242)].func.mob = bs_critter;
  mob_index[real_mobile0(74024)].func.mob = bs_critter;
  mob_index[real_mobile0(74113)].func.mob = bs_critter;
  mob_index[real_mobile0(74243)].func.mob = bs_critter;
  mob_index[real_mobile0(74007)].func.mob = bs_timid;
  mob_index[real_mobile0(74008)].func.mob = bs_shady;
  mob_index[real_mobile0(74009)].func.mob = bs_sinister;
  mob_index[real_mobile0(74009)].func.mob = bs_menacing;
  mob_index[real_mobile0(74009)].func.mob = bs_menacing;
  mob_index[real_mobile0(74011)].func.mob = bs_executioner;
  mob_index[real_mobile0(74012)].func.mob = bs_baron;
  mob_index[real_mobile0(74014)].func.mob = bs_sparrow;
  mob_index[real_mobile0(74015)].func.mob = bs_squirrel;
  mob_index[real_mobile0(74016)].func.mob = bs_squirrel;
  mob_index[real_mobile0(74017)].func.mob = bs_crow;
  mob_index[real_mobile0(74018)].func.mob = bs_mountainman;
  mob_index[real_mobile0(74019)].func.mob = bs_salesman;
  mob_index[real_mobile0(74020)].func.mob = bs_nomad;
  mob_index[real_mobile0(74241)].func.mob = bs_insane;
  mob_index[real_mobile0(74021)].func.mob = bs_homeless;
  mob_index[real_mobile0(74022)].func.mob = bs_servant;
  mob_index[real_mobile0(74023)].func.mob = bs_wolf;
  mob_index[real_mobile0(74026)].func.mob = bs_gnoll;
  mob_index[real_mobile0(74246)].func.mob = bs_griffon;
  mob_index[real_mobile0(74029)].func.mob = bs_boar;
  mob_index[real_mobile0(74030)].func.mob = bs_cub;
  mob_index[real_mobile0(74031)].func.mob = bs_fierce;
  mob_index[real_mobile0(74032)].func.mob = bs_fierce;
  mob_index[real_mobile0(74034)].func.mob = bs_stirge;
  mob_index[real_mobile0(74178)].func.mob = devour;
  mob_index[real_mobile0(74134)].func.mob = poison;
  mob_index[real_mobile0(74091)].func.mob = poison;
  mob_index[real_mobile0(74138)].func.mob = poison;
  mob_index[real_mobile0(74185)].func.mob = bs_barons_mistress;

  /* Obsidian Citadel */
//mob_index[real_mobile0(75615)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75631)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75632)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75633)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75634)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75635)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75636)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75637)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75638)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75639)].func.mob = obsid_cit_death_knight;
  mob_index[real_mobile0(75640)].func.mob = obsid_cit_satar_ghulan;


  /* Sylvandawn */
  mob_index[real_mobile0(8004)].func.mob = money_changer;
  mob_index[real_mobile0(8019)].func.mob = guild_guard;
  mob_index[real_mobile0(8029)].func.mob = guild_guard;
  mob_index[real_mobile0(8037)].func.mob = guild_guard;
  mob_index[real_mobile0(8039)].func.mob = guild_guard;
  mob_index[real_mobile0(8040)].func.mob = guild_guard;
  mob_index[real_mobile0(8041)].func.mob = guild_guard;
  mob_index[real_mobile0(8042)].func.mob = guild_guard;
  mob_index[real_mobile0(8044)].func.mob = janitor;
  mob_index[real_mobile0(8050)].func.mob = guild_guard;
  mob_index[real_mobile0(8311)].func.mob = guild_guard;
  mob_index[real_mobile0(8312)].func.mob = guild_guard;
  mob_index[real_mobile0(8313)].func.mob = guild_guard;

  mob_index[real_mobile0(45049)].func.mob = guild_guard;
  /*
     Players Guild
   */
  mob_index[real_mobile0(16501)].func.mob = guild_guard;
  obj_index[real_object0(35102)].func.obj = magic_pool;
  obj_index[real_object0(35103)].func.obj = magic_pool;

  /*
     Split Shield
   */
  mob_index[real_mobile0(10301)].func.mob = gate_guard;
  mob_index[real_mobile0(10302)].func.mob = shady_man;

  /*
     Realms Master
   */
  /*
     Moonshae
   */
  mob_index[real_mobile0(26218)].func.mob = sister_knight;
  mob_index[real_mobile0(26219)].func.mob = sister_knight;
  mob_index[real_mobile0(26220)].func.mob = sister_knight;
  mob_index[real_mobile0(26221)].func.mob = sister_knight;
  mob_index[real_mobile0(26222)].func.mob = sister_knight;

  /* Troll HT - Ghore */
  mob_index[real_mobile0(11542)].func.mob = ghore_paradise;
  mob_index[real_mobile0(11559)].func.mob = money_changer;
  mob_index[real_mobile0(11640)].func.mob = devour;
  mob_index[real_mobile0(11518)].func.mob = poison;

  /*
     Lava Tubes One
   */
  mob_index[real_mobile0(12000)].func.mob = snowbeast;
  mob_index[real_mobile0(12001)].func.mob = snowvulture;
  mob_index[real_mobile0(12002)].func.mob = spiny;
  mob_index[real_mobile0(12003)].func.mob = spiny;
  mob_index[real_mobile0(12005)].func.mob = phalanx;
  mob_index[real_mobile0(12006)].func.mob = skeleton;
  mob_index[real_mobile0(12022)].func.mob = spore_ball;
  mob_index[real_mobile0(12023)].func.mob = spore_ball;
  mob_index[real_mobile0(12024)].func.mob = skeleton;
  mob_index[real_mobile0(12025)].func.mob = xexos;
  mob_index[real_mobile0(12026)].func.mob = agthrodos;
  mob_index[real_mobile0(12027)].func.mob = automaton_unblock;
  obj_index[real_object0(12028)].func.obj = moonstone_fragment;

  /*
     Twin Towers - Forest
   */
  mob_index[real_mobile0(13505)].func.mob =
    mob_index[real_mobile0(13508)].func.mob =
    mob_index[real_mobile0(13511)].func.mob =
    mob_index[real_mobile0(13513)].func.mob =
    mob_index[real_mobile0(13515)].func.mob =
    mob_index[real_mobile0(13516)].func.mob =
    mob_index[real_mobile0(13517)].func.mob =
    mob_index[real_mobile0(13518)].func.mob =
    mob_index[real_mobile0(13519)].func.mob = forest_animals;

  /*
     Faerie Realm
   */
  mob_index[real_mobile0(14015)].func.mob = finn;
  mob_index[real_mobile0(14026)].func.mob = tree_spirit;
  mob_index[real_mobile0(14029)].func.mob = faerie;
  mob_index[real_mobile0(14048)].func.mob = cricket;

  /*
     Wilderness Near Verzanan
   */
  mob_index[real_mobile0(14202)].func.mob = bridge_troll;       /*
                                                                   north bridge
                                                                 */
/* Temple Zone */
  mob_index[real_mobile0(18302)].func.mob = temple_illyn;

  /*
     WD bypass road
   */
  mob_index[real_mobile0(14601)].func.mob = plant_attacks_poison;
  mob_index[real_mobile0(14602)].func.mob = plant_attacks_paralysis;
  mob_index[real_mobile0(14603)].func.mob = plant_attacks_blindness;
  mob_index[real_mobile0(14605)].func.mob = barbarian_spiritist;

  /* Duergar HT - Khildarak */
  mob_index[real_mobile0(17012)].func.mob = archer;
  mob_index[real_mobile0(17132)].func.mob =
    mob_index[real_mobile0(17134)].func.mob =
    mob_index[real_mobile0(17092)].func.mob =
    mob_index[real_mobile0(17049)].func.mob =
    mob_index[real_mobile0(17199)].func.mob =
/*      mob_index[real_mobile0(17247)].func.mob =*/
    mob_index[real_mobile0(17202)].func.mob = devour;
  mob_index[real_mobile0(17247)].func.mob = guild_guard;
  mob_index[real_mobile0(17193)].func.mob = devour;
  mob_index[real_mobile0(17194)].func.mob = devour;
  mob_index[real_mobile0(17195)].func.mob = devour;
  mob_index[real_mobile0(17261)].func.mob = poison;

  /* Ogre HT - Faang */
  mob_index[real_mobile0(15200)].func.mob = boulder_pusher;
  mob_index[real_mobile0(15213)].func.mob = poison;
  mob_index[real_mobile0(15215)].func.mob = poison;

  /*
     New Cavecity
   */
  mob_index[real_mobile0(15113)].func.mob = dranum_jurtrem;
  mob_index[real_mobile0(15125)].func.mob = dranum_jurtrem;


  mob_index[real_mobile0(18)].func.mob = ai_mob_proc;
  mob_index[real_mobile0(1255)].func.mob = random_quest_mob_proc;
  mob_index[real_mobile0(1256)].func.mob = random_mob_proc;
  /*
     Astral Plane - Tiamat
   */
  mob_index[real_mobile0(19700)].func.mob = tiamat;
  mob_index[real_mobile0(19710)].func.mob = guild_guard;
  mob_index[real_mobile0(19720)].func.mob = guild_guard;
  mob_index[real_mobile0(19730)].func.mob = guild_guard;
  mob_index[real_mobile0(19740)].func.mob = guild_guard;
  mob_index[real_mobile0(19880)].func.mob = astral_succubus;
  mob_index[real_mobile0(19600)].func.mob = block_dir;

  /* Shabo shouts */

  mob_index[real_mobile0(32828)].func.mob = strychnesch_shout;
  mob_index[real_mobile0(32830)].func.mob = morgoor_shout;
  mob_index[real_mobile0(32829)].func.mob = jabulanth_shout;
  mob_index[real_mobile0(32831)].func.mob = redpal_shout;
  mob_index[real_mobile0(32832)].func.mob = cyvrand_shout;
  mob_index[real_mobile0(32802)].func.mob = overseer_shout;
  mob_index[real_mobile0(32838)].func.mob = shabo_caran;


  /*
     Plane of Fire One
   */
  mob_index[real_mobile0(25000)].func.mob = guild_guard;
  mob_index[real_mobile0(25400)].func.mob = guild_guard;
  mob_index[real_mobile0(25440)].func.mob = imix_shout;
  mob_index[real_mobile0(25101)].func.mob = guild_guard;
  mob_index[real_mobile0(25104)].func.mob = guild_guard;

/*
     Prison
*/
  mob_index[real_mobile0(7333)].func.mob = warden_shout;


/*
     Nizari
   */
  mob_index[real_mobile0(40053)].func.mob = thief;
  mob_index[real_mobile0(40054)].func.mob = thief;
  mob_index[real_mobile0(40055)].func.mob = thief;

/*
   Ashrumite Village
 */
  mob_index[real_mobile0(66037)].func.mob = drunk_one;
  mob_index[real_mobile0(66001)].func.mob = cityguard;
  mob_index[real_mobile0(66002)].func.mob = cityguard;
  mob_index[real_mobile0(66003)].func.mob = cityguard;
  mob_index[real_mobile0(66031)].func.mob = guild_guard;
  mob_index[real_mobile0(66024)].func.mob = guild_guard;
  mob_index[real_mobile0(66023)].func.mob = guild_guard;
  mob_index[real_mobile0(66025)].func.mob = guild_guard;
  mob_index[real_mobile0(66022)].func.mob = guild_guard;
  mob_index[real_mobile0(66036)].func.mob = janitor;
  mob_index[real_mobile0(66038)].func.mob = money_changer;


  /*
     Menden-on-the-Deep
   */
  mob_index[real_mobile0(88805)].func.mob = menden_fisherman;
  mob_index[real_mobile0(88806)].func.mob = menden_magus;
  mob_index[real_mobile0(88812)].func.mob = menden_inv_serv_die;
  mob_index[real_mobile0(88813)].func.mob = menden_figurine_die;
  mob_index[real_mobile0(88814)].func.mob = crystal_golem_die;
  mob_index[real_mobile0(88815)].func.mob = hippogriff_die;

  /*
     Mirar (Chionthar) Ferry
   */
/*
  mob_index[real_mobile0(90590)].func.mob = navagator;
*/

  /*
     icecrag keep
   */
  mob_index[real_mobile0(97000)].func.mob = ice_snooty_wife;
  mob_index[real_mobile0(97001)].func.mob = ice_cleaning_crew;
  mob_index[real_mobile0(97002)].func.mob = ice_artist;
  mob_index[real_mobile0(97005)].func.mob = ice_privates;
  mob_index[real_mobile0(97006)].func.mob = ice_masha;
  mob_index[real_mobile0(97007)].func.mob = ice_tubby_merchant;
  mob_index[real_mobile0(97008)].func.mob = ice_priest;
  mob_index[real_mobile0(97011)].func.mob = ice_garden_attendant;
  mob_index[real_mobile0(97014)].func.mob = ice_raucous_guest;
  mob_index[real_mobile0(97016)].func.mob = ice_tar;
  mob_index[real_mobile0(97021)].func.mob = ice_commander;
  mob_index[real_mobile0(97023)].func.mob = ice_viscount;
  mob_index[real_mobile0(97028)].func.mob = ice_masonary_crew;
  mob_index[real_mobile0(97033)].func.mob = ice_impatient_guest;
/*
  mob_index[real_mobile0(97018)].func.mob = ice_privates2;
  mob_index[real_mobile0(97019)].func.mob = ice_privates2;
*/
  mob_index[real_mobile0(97040)].func.mob = ice_bodyguards;
  mob_index[real_mobile0(97041)].func.mob = ice_bodyguards;
  mob_index[real_mobile0(97042)].func.mob = ice_bodyguards;
  mob_index[real_mobile0(97030)].func.mob = ice_wolf;
  mob_index[real_mobile0(97003)].func.mob = ice_malice;

  /*
     jotunhiem
   */
  mob_index[real_mobile0(96030)].func.mob = jotun_balor;
  mob_index[real_mobile0(96040)].func.mob = jotun_utgard_loki;
  mob_index[real_mobile0(96013)].func.mob = jotun_mimer;
  mob_index[real_mobile0(96027)].func.mob = jotun_thrym;
  obj_index[real_object0(96073)].func.obj = faith;
  obj_index[real_object0(96012)].func.obj = mistweave;
  obj_index[real_object0(96027)].func.obj = leather_vest;
  obj_index[real_object0(96042)].func.obj = deva_cloak;
  obj_index[real_object0(96059)].func.obj = icicle_cloak;
  obj_index[real_object0(8406)].func.obj = ogrebane;
  obj_index[real_object0(96066)].func.obj = giantbane;
  obj_index[real_object0(20000)].func.obj = mindbreaker;


  /* Heaven */
  
  obj_index[real_object0(1270)].func.obj = treasure_chest;

  /*
     torg
   */
  mob_index[real_mobile0(28961)].func.mob = timoro_die;

/*
drst
*/

  obj_index[real_object0(34226)].func.obj = dwarfslayer;

/*
bogentok
*/
//  obj_index[real_object0(18744)].func.obj = betrayal;

  /*
     negative material plan
   */
  mob_index[real_mobile0(26603)].func.mob = neg_pocket;


  /*
     neverwinter
   */
  mob_index[real_mobile0(99001)].func.mob = nw_woodelf;
  mob_index[real_mobile0(99002)].func.mob = nw_elfhealer;
  mob_index[real_mobile0(99003)].func.mob = nw_ammaster;
  mob_index[real_mobile0(99004)].func.mob = nw_sapmaster;
  mob_index[real_mobile0(99005)].func.mob = nw_diamaster;
  mob_index[real_mobile0(99006)].func.mob = nw_rubmaster;
  mob_index[real_mobile0(99007)].func.mob = nw_emmaster;
  mob_index[real_mobile0(99009)].func.mob = nw_human;
  mob_index[real_mobile0(99010)].func.mob = nw_hafbreed;
  mob_index[real_mobile0(99011)].func.mob = nw_owl;
  mob_index[real_mobile0(99015)].func.mob = nw_golem;
  mob_index[real_mobile0(99016)].func.mob = nw_mirroid;
  mob_index[real_mobile0(99017)].func.mob = nw_agatha;
  mob_index[real_mobile0(99018)].func.mob = nw_farmer;
  mob_index[real_mobile0(99019)].func.mob = nw_chicken;
  mob_index[real_mobile0(99021)].func.mob = nw_pig;
  mob_index[real_mobile0(99022)].func.mob = nw_cow;
  mob_index[real_mobile0(99023)].func.mob = nw_chief;
  mob_index[real_mobile0(99028)].func.mob = nw_malchor;
  mob_index[real_mobile0(99029)].func.mob = nw_builder;
  mob_index[real_mobile0(99030)].func.mob = nw_carpen;
  mob_index[real_mobile0(99031)].func.mob = nw_logger;
  mob_index[real_mobile0(99032)].func.mob = nw_cutter;
  mob_index[real_mobile0(99033)].func.mob = nw_foreman;
  mob_index[real_mobile0(99034)].func.mob = nw_ansal;
  mob_index[real_mobile0(99035)].func.mob = nw_brock;
  mob_index[real_mobile0(99036)].func.mob = nw_merthol;
  mob_index[real_mobile0(99037)].func.mob = nw_vitnor;

  /*
     undermountain
   */

#if 0
  mob_index[real_mobile0()].func.mob = um_durnan;
  mob_index[real_mobile0()].func.mob = um_mhaere;
  mob_index[real_mobile0()].func.mob = um_regular;
  mob_index[real_mobile0()].func.mob = um_gambler;
  mob_index[real_mobile0()].func.mob = um_tamsil;
  mob_index[real_mobile0(92020)].func.mob = um_kevlar;
  mob_index[real_mobile0(92021)].func.mob = um_thorn;
  mob_index[real_mobile0(92022)].func.mob = um_korelar;
  mob_index[real_mobile0()].func.mob = um_essra;
  mob_index[real_mobile0()].func.mob = um_mezzoloth;
  mob_index[real_mobile0()].func.mob = um_goblin_leader;
  mob_index[real_mobile0()].func.mob = animated_sword;
  mob_index[real_mobile0()].func.mob = malodine_one;
  mob_index[real_mobile0()].func.mob = malodine_two;
  mob_index[real_mobile0()].func.mob = malodine_three;
  mob_index[real_mobile0()].func.mob = black_pudding;
  mob_index[real_mobile0(92047)].func.mob = flying_dagger;
  mob_index[real_mobile0(92058)].func.mob = ochre_jelly;
  mob_index[real_mobile0(92062)].func.mob = helmed_horror;
#endif
  mob_index[real_mobile0(150100)].func.mob = patrol_leader;
  mob_index[real_mobile0(150101)].func.mob = patrol_leader_road;

/* elemental plane bosses shout */
  mob_index[real_mobile0(12400)].func.mob = menzellon_shout;
  mob_index[real_mobile0(23806)].func.mob = ogremoch_shout;
  mob_index[real_mobile0(23808)].func.mob = ogremoch_shout;
  mob_index[real_mobile0(23240)].func.mob = olhydra_shout;
  mob_index[real_mobile0(24440)].func.mob = yancbin_shout;
  mob_index[real_mobile0(23807)].func.mob = earth_treant;

/* jind */
  mob_index[real_mobile0(82000)].func.mob = jindo_ticket_master;
/* newbie zone this mob gives a special item to the newbie */
  mob_index[real_mobile0(22801)].func.mob = newbie_paladin;
// lowbie quest proc dont do this if your below lvl 35 thing
  mob_index[real_mobile0(65015)].func.mob = newbie_quest;
  mob_index[real_mobile0(65016)].func.mob = newbie_quest;
  mob_index[real_mobile0(65018)].func.mob = newbie_quest;
  mob_index[real_mobile0(65019)].func.mob = newbie_quest;
  mob_index[real_mobile0(65022)].func.mob = newbie_quest;
  mob_index[real_mobile0(65024)].func.mob = newbie_quest;
  mob_index[real_mobile0(65026)].func.mob = newbie_quest;
  mob_index[real_mobile0(65028)].func.mob = newbie_quest;
  mob_index[real_mobile0(65029)].func.mob = newbie_quest;
  mob_index[real_mobile0(65030)].func.mob = newbie_quest;
  mob_index[real_mobile0(65032)].func.mob = newbie_quest;
  mob_index[real_mobile0(65033)].func.mob = newbie_quest;
  mob_index[real_mobile0(65034)].func.mob = newbie_quest;

/*CELESTIA PROCS*/
  mob_index[real_mobile0(45565)].func.mob = Malevolence;
  mob_index[real_mobile0(45571)].func.mob = Malevolence_vapor;
  mob_index[real_mobile0(45510)].func.mob = celestia_pulsar;

/* Nyneth */
  mob_index[real_mobile0(22956)].func.mob = construct;
  mob_index[real_mobile0(38737)].func.mob = nyneth;

  logit(LOG_STATUS, "   Booting the shops.");
  fprintf(stderr, "--    Booting the shops.\r\n");
  boot_the_shops();
  logit(LOG_STATUS, "   Assigning the shopkeepers.");
  fprintf(stderr, "--    Booting the shopkeepers.\r\n");
  assign_the_shopkeepers();

  logit(LOG_STATUS, "   Booting quests.");
  fprintf(stderr, "--    Booting the quests.\r\n");
  boot_the_quests();
  logit(LOG_STATUS, "   Assigning questers.");
  fprintf(stderr, "--    Assigning the questors.\r\n");
  assign_the_questers();
}

/*
   assign special procedures to objects
 */

void assign_objects(void)
{
  obj_index[real_object0(354)].func.obj = artifact_monolith;

  obj_index[real_object0(83457)].func.obj = miners_helmet;

  /* outposts */
  //obj_index[real_object0(97800)].func.obj = outpost_rubble; 
  mob_index[real_mobile0(97800)].func.mob = building_mob_proc;
  
  /* ailvio */
  obj_index[real_object0(29328)].func.obj = burbul_map_obj;
  obj_index[real_object0(29329)].func.obj = chyron_search_obj;

  /* banks - add proc for storage lockers */
  obj_index[real_object0(132581)].func.obj = storage_locker_obj_hook;
  obj_index[real_object0(3097)].func.obj = storage_locker_obj_hook;

  /* Mossi Mods:  General Items  */
  obj_index[real_object0(4)].func.obj = blood_stains;
  obj_index[real_object0(50)].func.obj = ice_shattered_bits;
  obj_index[real_object0(1276)].func.obj = tracks;
  obj_index[real_object0(99)].func.obj = frost_beacon;
  obj_index[real_object0(110)].func.obj = ice_block;

  /* somewhere */

  obj_index[real_object0(3833)].func.obj = glades_dagger;
  obj_index[real_object0(44499)].func.obj = lucky_weapon;
  obj_index[real_object0(15907)].func.obj = mace_dragondeath;
  obj_index[real_object0(17021)].func.obj = khildarak_warhammer;
  obj_index[real_object0(80830)].func.obj = ogre_warlords_sword;
  obj_index[real_object0(7365)].func.obj = flaming_axe_of_azer;

  /* bahamut */
  obj_index[real_object0(25711)].func.obj = mrinlor_whip;

  /* new Tiamat */
  obj_index[real_object0(135218)].func.obj = sphinx_prefect_crown;


//  obj_index[real_object0(22032)].func.obj = warmace_puredark;

/* Forgot what zone it is from  */

  obj_index[real_object0(38772)].func.obj = platemail_of_defense;
  obj_index[real_object0(414)].func.obj = platemail_of_defense;

  
  obj_index[real_object0(22063)].func.obj = master_set;
  obj_index[real_object0(22237)].func.obj = master_set;
  obj_index[real_object0(22621)].func.obj = master_set;
  obj_index[real_object0(45530)].func.obj = master_set;
  obj_index[real_object0(45531)].func.obj = master_set;
  obj_index[real_object0(82545)].func.obj = master_set;
  obj_index[real_object0(75857)].func.obj = master_set; 
     
  /* artifact funcs */

  obj_index[real_object0(919)].func.obj = ioun_sustenance;
  obj_index[real_object0(922)].func.obj = deflect_ioun;
  obj_index[real_object0(923)].func.obj = ioun_testicle;
  obj_index[real_object0(924)].func.obj = ioun_testicle;
  obj_index[real_object0(925)].func.obj = ioun_testicle;
  obj_index[real_object0(926)].func.obj = ioun_testicle;
  obj_index[real_object0(920)].func.obj = ioun_warp;

  obj_index[real_object0(1210)].func.obj = tendrils;
  obj_index[real_object0(26665)].func.obj = elvenkind_cloak;

  /* Undead zone */
  obj_index[real_object0(210001)].func.obj = charon_ship;

  /* Olympus */
#if 0
  obj_index[real_object0(99801)].func.obj = olympus_portal;
  obj_index[real_object0(99803)].func.obj = olympus_portal;
  obj_index[real_object0(99805)].func.obj = olympus_portal;
  obj_index[real_object0(99807)].func.obj = olympus_portal;
#endif

  obj_index[real_object0(67280)].func.obj =
  obj_index[real_object0(16263)].func.obj =
  obj_index[real_object0(16268)].func.obj =
  obj_index[real_object0(26653)].func.obj =
  obj_index[real_object0(25719)].func.obj =
  obj_index[real_object0(911)].func.obj =
  obj_index[real_object0(23808)].func.obj = artifact_stone;
  obj_index[real_object0(67244)].func.obj = artifact_stone;
  obj_index[real_object0(17)].func.obj = artifact_stone;
  obj_index[real_object0(23806)].func.obj = earthquake_gauntlet;

  obj_index[real_object0(910)].func.obj = artifact_biofeedback;
  obj_index[real_object0(67215)].func.obj = artifact_biofeedback;
  obj_index[real_object0(23807)].func.obj = blind_boots;

  obj_index[real_object0(400237)].func.obj = thanksgiving_wings;
  obj_index[real_object0(55320)].func.obj = artifact_stone;

  obj_index[real_object0(419)].func.obj = moonstone;
  obj_index[real_object0(433)].func.obj = moonstone;

  obj_index[real_object0(12410)].func.obj =
    obj_index[real_object0(19916)].func.obj =
    obj_index[real_object0(23809)].func.obj =
    obj_index[real_object0(17)].func.obj = artifact_hide;
  obj_index[real_object0(97118)].func.obj = artifact_hide;
  obj_index[real_object0(44170)].func.obj = artifact_hide;

  // old guildhalls (deprecated)
//  obj_index[real_object0(115)].func.obj = guild_chest;
  // obj_index[real_object0(96443)].func.obj = illithid_sack;

  obj_index[real_object0(70963)].func.obj = pathfinder;
  obj_index[real_object0(16904)].func.obj =
    obj_index[real_object0(23202)].func.obj =
    obj_index[real_object0(23203)].func.obj =
    obj_index[real_object0(32861)].func.obj =
    obj_index[real_object0(32428)].func.obj =
    obj_index[real_object0(71231)].func.obj =
    obj_index[real_object0(41204)].func.obj = artifact_invisible;
  /* Prison */
  obj_index[real_object0(7371)].func.obj = nexus;

  /* Negative Plane */
  obj_index[real_object0(26662)].func.obj = orb_of_destruction;
  obj_index[real_object0(26621)].func.obj = sanguine;
  obj_index[real_object0(26662)].func.obj = neg_orb;

/* RaxCode (tm) */

/* Necro Pet Specs */
  mob_index[real_mobile0(33)].func.mob = necro_specpet_bone;
  mob_index[real_mobile0(34)].func.mob = necro_specpet_flesh;
  mob_index[real_mobile0(35)].func.mob = necro_specpet_flesh;

/* Conj Pet Specs */
  mob_index[real_mobile0(1121)].func.mob = conj_specpet_golem;
  mob_index[real_mobile0(1122)].func.mob = conj_specpet_xorn;
  mob_index[real_mobile0(1131)].func.mob = conj_specpet_slyph;
  mob_index[real_mobile0(1132)].func.mob = conj_specpet_djinni;
  mob_index[real_mobile0(1141)].func.mob = conj_specpet_undine;
  mob_index[real_mobile0(1142)].func.mob = conj_specpet_triton;
// mob_index[real_mobile0(1111)].func.mob = conj_specpet_salamander;
  mob_index[real_mobile0(1112)].func.mob = conj_specpet_serpent;



/* misc */
  obj_index[real_object0(1230)].func.obj = totem_of_mastery;
//  obj_index[real_object0(41208)].func.obj = mithril_dagger;
  obj_index[real_object0(48)].func.obj = god_bp;
  obj_index[real_object0(49)].func.obj = out_of_god_bp;
  obj_index[real_object0(38763)].func.obj = ring_of_regeneration;
  obj_index[real_object0(31549)].func.obj = glowing_necklace;
  obj_index[real_object0(32008)].func.obj = staff_shadow_summoning;
  obj_index[real_object0(67245)].func.obj = rod_of_magic;
  obj_index[real_object0(76032)].func.obj = proc_whirlwinds;
  obj_index[real_object0(1220)].func.obj = lyrical_instrument_of_time;

/* avernus procs */
  obj_index[real_object0(32001)].func.obj = sinister_tactics_staff;
  obj_index[real_object0(32507)].func.obj = shard_frozen_styx_water;

/* shabo procs */
  obj_index[real_object0(32831)].func.obj = pesky_imp_chest;
  obj_index[real_object0(32832)].func.obj = pesky_imp_chest;
  obj_index[real_object0(32833)].func.obj = pesky_imp_chest;
  obj_index[real_object0(32822)].func.obj = holy_weapon;
  obj_index[real_object0(32836)].func.obj = mox_totem;
// obj_index[real_object0(32807)].func.obj = monitor_trident;
  obj_index[real_object0(32816)].func.obj = flayed_mind_mask;
  obj_index[real_object0(67281)].func.obj = stalker_cloak;
  obj_index[real_object0(32837)].func.obj = finslayer_air;
  obj_index[real_object0(32862)].func.obj = aboleth_pendant;
  obj_index[real_object0(32864)].func.obj = artifact_stone;

  mob_index[real_mobile0(32840)].func.mob = shabo_butler;
  mob_index[real_mobile0(32843)].func.mob = shabo_petre;
  mob_index[real_mobile0(32842)].func.mob = shabo_palle;
  mob_index[real_mobile0(32803)].func.mob = shabo_derro_savant;

  obj_index[real_object0(32850)].func.obj = tower_summoning;
  obj_index[real_object0(32852)].func.obj = shabo_trap_north_two;
  obj_index[real_object0(32851)].func.obj = shabo_trap_south;
  obj_index[real_object0(32848)].func.obj = shabo_trap_south_two;
  obj_index[real_object0(32834)].func.obj = shabo_trap_down;
  obj_index[real_object0(32849)].func.obj = shabo_trap_up;
  obj_index[real_object0(32826)].func.obj = shabo_trap_up_two;

  mob_index[real_mobile0(1000)].func.mob = annoying_mob;
  mob_index[real_mobile0(320)].func.mob = cow_talk;
  obj_index[real_object0(46)].func.obj = lightning_armor;
  obj_index[real_object0(47)].func.obj = imprison_armor;

// re-enabled old procs
  obj_index[real_object0(45)].func.obj = slot_machine;
  mob_index[real_mobile0(11)].func.mob = raoul;
  mob_index[real_mobile0(12)].func.mob = christine;
  mob_index[real_mobile0(13)].func.mob = cookie_monster;

  /* objs */
  obj_index[real_object0(1191)].func.obj = fun_dagger;

  obj_index[real_object0(2203)].func.obj = unspec_altar;
  obj_index[real_object0(12016)].func.obj = unspec_altar;
  obj_index[real_object0(1190)].func.obj = rax_red_dagger;
  obj_index[real_object0(2204)].func.obj = cutting_dagger;
  obj_index[real_object0(70549)].func.obj = circlet_of_light;
  obj_index[real_object0(70554)].func.obj = ljs_sword;
  obj_index[real_object0(70556)].func.obj = wuss_sword;
  obj_index[real_object0(70558)].func.obj = head_guard_sword;
  obj_index[real_object0(70559)].func.obj = priest_rudder;
  obj_index[real_object0(70565)].func.obj = alch_bag;
  obj_index[real_object0(70568)].func.obj = alch_rod;
  obj_index[real_object0(70571)].func.obj = ljs_armor;
  obj_index[real_object0(70572)].func.obj = dragon_skull_helm;
  obj_index[real_object0(99447)].func.obj = nightcrawler_dagger;
  obj_index[real_object0(66419)].func.obj = righteous_blade;


  /* mobs */
  mob_index[real_mobile0(70535)].func.mob = long_john_silver_shout;
  mob_index[real_mobile0(70542)].func.mob = undead_parrot;
  mob_index[real_mobile0(70546)].func.mob = undead_dragon_east;

  /* general jabbering */
  mob_index[real_mobile0(70552)].func.mob = pirate_cabinboy_talk;
  mob_index[real_mobile0(70554)].func.mob = pirate_female_talk;
  mob_index[real_mobile0(70502)].func.mob = pirate_talk;
  mob_index[real_mobile0(70503)].func.mob = pirate_talk;
  mob_index[real_mobile0(70539)].func.mob = pirate_talk;
  mob_index[real_mobile0(70540)].func.mob = pirate_talk;
  mob_index[real_mobile0(70541)].func.mob = pirate_talk;
  mob_index[real_mobile0(70549)].func.mob = pirate_talk;
  mob_index[real_mobile0(70551)].func.mob = pirate_talk;
  mob_index[real_mobile0(70561)].func.mob = pirate_talk;

  /* tower of high sorcery */
  mob_index[real_mobile0(9342)].func.mob = bulette;
  mob_index[real_mobile0(9344)].func.mob = bulette;
  mob_index[real_mobile0(9345)].func.mob = bulette;
  mob_index[real_mobile0(9346)].func.mob = bulette;
  mob_index[real_mobile0(9347)].func.mob = bulette;
  mob_index[real_mobile0(9348)].func.mob = bulette;
  mob_index[real_mobile0(9349)].func.mob = bulette;
  mob_index[real_mobile0(9350)].func.mob = bulette;

/* End RaxCode */

  obj_index[real_object0(666)].func.obj = xmas_cap;
  obj_index[real_object0(400)].func.obj = khaziddea_blade;

  obj_index[real_object0(22070)].func.obj = revenant_helm;
  /* Bahamut */
  obj_index[real_object0(25723)].func.obj = dragonlord_plate;
  obj_index[real_object0(25745)].func.obj = sunblade;
  obj_index[real_object0(25710)].func.obj = bloodfeast;

  /*Newbie quest */
  obj_index[real_object0(65050)].func.obj = dragonslayer;
  obj_index[real_object0(34545)].func.obj = mankiller;

  /* MadMan */
  obj_index[real_object0(44179)].func.obj = madman_mangler;
  obj_index[real_object0(44172)].func.obj = madman_shield;
  obj_index[real_object0(44188)].func.obj = mentality_mace;

  obj_index[real_object0(25)].func.obj = artifact_biofeedback;
  obj_index[real_object0(67203)].func.obj = vapor;

  /* 56 Zone - Kvark */
  obj_index[real_object0(45528)].func.obj = serpent_of_miracles;
  obj_index[real_object0(45514)].func.obj = transparent_blade;
  obj_index[real_object0(45525)].func.obj = Einjar;

  /* Special items for thanks giving. */

  obj_index[real_object0(35)].func.obj = tripboots;
  obj_index[real_object0(38)].func.obj = blindbadge;
  obj_index[real_object0(36)].func.obj = fumblegaunts;
  obj_index[real_object0(37)].func.obj = confusionsword;

  obj_index[real_object0(41)].func.obj = guild_badge;

/* random eq proc  */
  obj_index[real_object0(1253)].func.obj = thrusted_eq_proc;
  obj_index[real_object0(1252)].func.obj = set_proc;
  obj_index[real_object0(1251)].func.obj = encrusted_eq_proc;
  obj_index[real_object0(251)].func.obj = parchment_forge;
  obj_index[real_object0(400210)].func.obj = learn_recipe;

  obj_index[real_object0(58)].func.obj = relic_proc;
  obj_index[real_object0(59)].func.obj = relic_proc;
  obj_index[real_object0(68)].func.obj = relic_proc;

  obj_index[real_object0(69)].func.obj = cold_hammer;

  /* 4 horsemen */

  obj_index[real_object0(34559)].func.obj = brainripper;

  /* Nyneth */
  obj_index[real_object0(22962)].func.obj = hammer_titans;
  obj_index[real_object0(38725)].func.obj = stormbringer;


  /* Tokens */
  obj_index[real_object0(67243)].func.obj = living_necroplasm;
  obj_index[real_object0(67200)].func.obj = vigor_mask;
  obj_index[real_object0(67239)].func.obj = church_door;
  obj_index[real_object0(67260)].func.obj = splinter;
  obj_index[real_object0(67206)].func.obj = demo_scimitar;
  obj_index[real_object0(67272)].func.obj = dranum_mask;
  obj_index[real_object0(67220)].func.obj = golem_chunk;
  obj_index[real_object0(21)].func.obj = good_evil_sword;
  obj_index[real_object0(22)].func.obj = good_evil_sword;

  obj_index[real_object0(1234)].func.obj = banana;
  obj_index[real_object0(358)].func.obj = epic_stone;
  obj_index[real_object0(359)].func.obj = epic_stone;
  obj_index[real_object0(360)].func.obj = epic_stone;

  /* stat pools */

  obj_index[real_object0(60)].func.obj = stat_pool_str;
  obj_index[real_object0(61)].func.obj = stat_pool_dex;
  obj_index[real_object0(62)].func.obj = stat_pool_agi;
  obj_index[real_object0(63)].func.obj = stat_pool_con;
  obj_index[real_object0(64)].func.obj = stat_pool_pow;
  obj_index[real_object0(65)].func.obj = stat_pool_int;
  obj_index[real_object0(66)].func.obj = stat_pool_wis;
  obj_index[real_object0(67)].func.obj = stat_pool_cha;
  obj_index[real_object0(930)].func.obj = stat_pool_luc;

  /* random spell pool */
  obj_index[real_object0(72)].func.obj = spell_pool;

  obj_index[real_object0(26666)].func.obj = transp_tow_misty_gloves;

  /* player castles */

  obj_index[real_object0(11002)].func.obj =
    obj_index[real_object0(11051)].func.obj =
    obj_index[real_object0(11052)].func.obj =
    obj_index[real_object0(11053)].func.obj =
    obj_index[real_object0(11054)].func.obj =
    obj_index[real_object0(11055)].func.obj =
    obj_index[real_object0(11056)].func.obj =
    obj_index[real_object0(11057)].func.obj =
    obj_index[real_object0(11058)].func.obj =
    obj_index[real_object0(11059)].func.obj =
    obj_index[real_object0(11060)].func.obj =
    obj_index[real_object0(11061)].func.obj =
    obj_index[real_object0(11062)].func.obj =
    obj_index[real_object0(11063)].func.obj =
    obj_index[real_object0(11064)].func.obj =
    obj_index[real_object0(11065)].func.obj = magic_mouth;

  /* heavens */
  obj_index[real_object0(750)].func.obj = druid_spring;
  obj_index[real_object0(749)].func.obj = blighter_pond;
//  obj_index[real_object0(366)].func.obj = druid_sabre;
  obj_index[real_object0(1218)].func.obj = flying_citadel;
  obj_index[real_object0(1222)].func.obj = gfstone;

  obj_index[real_object0(192)].func.obj = disarm_pick_gloves;

  obj_index[real_object0(67279)].func.obj = roulette_pistol;
  obj_index[real_object0(67282)].func.obj = orb_of_deception;
  obj_index[real_object0(427)].func.obj = super_cannon;
  obj_index[real_object0(428)].func.obj = zombies_game;

  obj_index[real_object0(435)].func.obj = proc_soldon_hat;
  obj_index[real_object0(499)].func.obj = proc_lohrr;

/* Hall of the Ancients */
  obj_index[real_object0(77706)].func.obj = trap_razor_hooks;
  obj_index[real_object0(77721)].func.obj = trap_tower1_para;
  obj_index[real_object0(77731)].func.obj = trap_tower2_sleep;
  obj_index[real_object0(77738)].func.obj = illesarus;
  obj_index[real_object0(77734)].func.obj = artifact_stone;
  obj_index[real_object0(77752)].func.obj = hoa_plat;
  obj_index[real_object0(77749)].func.obj = artifact_stone;

/* Vecna */
  obj_index[real_object0(130006)].func.obj = vecna_deathportal;
  obj_index[real_object0(130003)].func.obj = vecna_deathaltar;
  obj_index[real_object0(130040)].func.obj = vecna_stonemist;
  obj_index[real_object0(130041)].func.obj = vecna_ghosthands;
  obj_index[real_object0(130041)].func.obj = vecna_torturerroom;
  obj_index[real_object0(130000)].func.obj = vecna_gorge;
  obj_index[real_object0(130013)].func.obj = vecna_pestilence;
  obj_index[real_object0(130009)].func.obj = vecna_minifist;
  obj_index[real_object0(130007)].func.obj = vecna_dispel;
  obj_index[real_object0(130008)].func.obj = vecna_boneaxe;
  obj_index[real_object0(130027)].func.obj = vecna_staffoaken;
  obj_index[real_object0(130028)].func.obj = vecna_krindor_main;
  obj_index[real_object0(130018)].func.obj = vecna_death_mask;
  obj_index[real_object0(130038)].func.obj = mob_vecna_procs;
  
  /* Duris Tournament */
  obj_index[real_object0(67900)].func.obj = arenaobj_proc;

  /*specs.object.c */
  obj_index[real_object0(23056)].func.obj = lifereaver;
  obj_index[real_object0(366)].func.obj = flame_blade;
  /* sea kingdom */

  obj_index[real_object0(31514)].func.obj = SeaKingdom_Tsunami;

  /* arachdrathos */

  obj_index[real_object0(36744)].func.obj = shimmering_longsword;
  obj_index[real_object0(36760)].func.obj = rod_of_zarbon;

  /* dlsc - in for testing currently */

#if 0
  obj_index[real_object0(5484)].func.obj = frost_elb_dagger;
  obj_index[real_object0(5436)].func.obj = dagger_submission;
  mob_index[real_mobile0(5411)].func.mob = demon_chick;
#endif

  /* transparent tower */
  obj_index[real_object0(16262)].func.obj = trans_tower_shadow_globe;
  //obj_index[real_object0(16242)].func.obj = trans_tower_sword;
/*  obj_index[real_object0(80563)].func.obj = yuan_ti_stone;*/

  /* claw cavern */
  obj_index[real_object0(80747)].func.obj = burn_touch_obj;

  /* Yuan-Ti Zone */
  obj_index[real_object0(80556)].func.obj = drowcrusher;
  obj_index[real_object0(80579)].func.obj = squelcher;
  obj_index[real_object0(80569)].func.obj = dragonarmor;

  /* highway */
 // obj_index[real_object0(70924)].func.obj = lobos_jacket; 
  obj_index[real_object0(41304)].func.obj = kearonor_hide;
  obj_index[real_object0(41349)].func.obj = hewards_mystical_organ;
  obj_index[real_object0(42235)].func.obj = amethyst_orb;
  obj_index[real_object0(41350)].func.obj = wand_of_wonder;
  obj_index[real_object0(41918)].func.obj = blade_of_paladins;
  obj_index[real_object0(41913)].func.obj = fade_drusus;
  obj_index[real_object0(41915)].func.obj = lightning_sword;
  obj_index[real_object0(41912)].func.obj = elfdawn_sword;
  obj_index[real_object0(41917)].func.obj = flame_of_north_sword;
  obj_index[real_object0(41914)].func.obj = magebane_falchion;
  obj_index[real_object0(41916)].func.obj = woundhealer_scimitar;
  obj_index[real_object0(41911)].func.obj = martelo_mstar;
  obj_index[real_object0(41907)].func.obj = mir_fire;

  /* Lower God Rooms */
  obj_index[real_object0(18)].func.obj = githpc_special_weap;
  obj_index[real_object0(19)].func.obj = githpc_special_weap;
  obj_index[real_object0(418)].func.obj = githpc_special_weap;
  obj_index[real_object0(89)].func.obj = board;
  obj_index[real_object0(92)].func.obj = board;
  obj_index[real_object0(90)].func.obj = board;
  obj_index[real_object0(76)].func.obj = board;
  obj_index[real_object0(84)].func.obj = board;
  obj_index[real_object0(85)].func.obj = board;
  obj_index[real_object0(86)].func.obj = board;
  obj_index[real_object0(87)].func.obj = board;
  obj_index[real_object0(88)].func.obj = board;
  obj_index[real_object0(75)].func.obj = board;
  obj_index[real_object0(78)].func.obj = board;
  obj_index[real_object0(79)].func.obj = board;
  obj_index[real_object0(80)].func.obj = board;
  obj_index[real_object0(81)].func.obj = board;
  obj_index[real_object0(29)].func.obj = board;
  obj_index[real_object0(42)].func.obj = board;

  /* random zone stuff here please ! */
  obj_index[real_object0(19507)].func.obj = random_tomb;
  obj_index[real_object0(19511)].func.obj = random_glass;
  obj_index[real_object0(19515)].func.obj = random_slab;

  /*            Fountains               */
  obj_index[real_object0(70)].func.obj = refreshing_fountain;
  obj_index[real_object0(71)].func.obj = magical_fountain;

  /* Middle God Rooms */
  obj_index[real_object0(751)].func.obj = portal_door;
  obj_index[real_object0(291)].func.obj = magic_pool;
  obj_index[real_object0(770)].func.obj = portal_wormhole;
  obj_index[real_object0(780)].func.obj = portal_etherportal;
  obj_index[real_object0(782)].func.obj = portal_door;
  obj_index[real_object0(55)].func.obj = trustee_artifact;

  /* Main God Rooms */
  obj_index[real_object0(1227)].func.obj = orcus_wand;
  obj_index[real_object0(30)].func.obj = varon;
  obj_index[real_object0(752)].func.obj = portal_door;
  obj_index[real_object0(781)].func.obj = portal_door;
//  obj_index[real_object0(363)].func.obj = thought_beacon;
  obj_index[real_object0(14)].func.obj = banana;
  obj_index[real_object0(15)].func.obj = banana;
  obj_index[real_object0(1278)].func.obj = changelog;
  obj_index[real_object0(1300)].func.obj = obj_imprison;

  /* Wall procedures */
  obj_index[real_object0(VOBJ_WALLS)].func.obj = wall_generic; /* Wall of stone */

  /* Traps */
  obj_index[real_object0(54)].func.obj = huntsman_ward;
  obj_index[real_object0(73)].func.obj = huntsman_ward;
  obj_index[real_object0(77)].func.obj = huntsman_ward;
  obj_index[real_object0(400229)].func.obj = huntsman_ward;

  /* Kobold Settlement */
  obj_index[real_object0(1421)].func.obj = item_switch;
  obj_index[real_object0(1425)].func.obj = item_switch;
  obj_index[real_object0(1427)].func.obj = item_switch;

#if 0
  /* Southern Verzanan */
  obj_index[real_object0(2998)].func.obj = clock_tower; /* WD clock */
#endif

  obj_index[real_object0(132571)].func.obj = jailtally;
  obj_index[real_object0(8100)].func.obj = jailtally;
  obj_index[real_object0(3068)].func.obj = jailtally;

  /* The Underworld */
  obj_index[real_object0(4505)].func.obj = hammer;
  obj_index[real_object0(4403)].func.obj = magic_pool;
  obj_index[real_object0(4404)].func.obj = magic_pool;
  obj_index[real_object0(17)].func.obj = barb;

  /* Alterian Wilderness */
  obj_index[real_object0(4801)].func.obj = magic_pool;
  obj_index[real_object0(4802)].func.obj = magic_pool;

#if 0
  /* Verzanan Harbor */
  obj_index[real_object0(5316)].func.obj = gesen;

  /* The Guilds of Verzanan */
  obj_index[real_object0(5515)].func.obj = verzanan_portal;
  obj_index[real_object0(5516)].func.obj = verzanan_portal;
#endif

  /* Western Realms */
//  obj_index[real_object0(5731)].func.obj = ship;
//  obj_index[real_object0(5732)].func.obj = control_panel;

  /* Tharnadia */
  obj_index[real_object0(132570)].func.obj = die_roller;
    obj_index[real_object0(132609)].func.obj = die_roller;
    obj_index[real_object0(132610)].func.obj = die_roller;

  
  /* Sylvandawn */
  obj_index[real_object0(425)].func.obj = holy_weapon;
/* These objects don't exist??
  obj_index[real_object0(8110)].func.obj = labelas;
  obj_index[real_object0(8112)].func.obj = elfgate;
  obj_index[real_object0(8113)].func.obj = elfgate;
*/
  /* Dragonnia */

  obj_index[real_object0(6826)].func.obj = dragonkind;
  obj_index[real_object0(6818)].func.obj = resurrect_totem;

  /* Players Guild */
  obj_index[real_object0(11008)].func.obj = guildwindow;
  obj_index[real_object0(11013)].func.obj = guildhome;
/*
      obj_index[real_object0(34300)].func.obj =
      obj_index[real_object0(35953)].func.obj =
      obj_index[real_object0(35350)].func.obj =
      obj_index[real_object0(35952)].func.obj =
      obj_index[real_object0(35950)].func.obj =
      obj_index[real_object0(35951)].func.obj =
      obj_index[real_object0(35900)].func.obj =
      obj_index[real_object0(35050)].func.obj =
      obj_index[real_object0(35253)].func.obj =
      obj_index[real_object0(34200)].func.obj =
      obj_index[real_object0(34404)].func.obj =
      obj_index[real_object0(35970)].func.obj = board;
*/


  obj_index[real_object0(12000)].func.obj = crystal_spike;
/*  obj_index[real_object0(12025)].func.obj = skeleton_key; */
  obj_index[real_object0(12027)].func.obj = automaton_lever;
  obj_index[real_object0(96402)].func.obj = illithid_teleport_veil;

  /* Twin Towers - Forest */
  obj_index[real_object0(13505)].func.obj =
    obj_index[real_object0(13508)].func.obj =
    obj_index[real_object0(13511)].func.obj =
    obj_index[real_object0(13513)].func.obj =
    obj_index[real_object0(13515)].func.obj =
    obj_index[real_object0(13516)].func.obj =
    obj_index[real_object0(13517)].func.obj =
    obj_index[real_object0(13518)].func.obj =
    obj_index[real_object0(13519)].func.obj = forest_corpse;

  /*
     New Cavecity
   */
  obj_index[real_object0(15116)].func.obj = torment;

  /*
     Astral Plane - Tiamat
   */
  obj_index[real_object0(19710)].func.obj = magic_pool;
  obj_index[real_object0(19711)].func.obj = magic_pool;
  obj_index[real_object0(19715)].func.obj = magic_pool;
  obj_index[real_object0(19730)].func.obj = avernus;

  /*
     Astral Plane - Main Grid
   */
  obj_index[real_object0(19860)].func.obj = magic_pool;
  obj_index[real_object0(19861)].func.obj = magic_pool;
  obj_index[real_object0(19862)].func.obj = magic_pool;
  obj_index[real_object0(19863)].func.obj = magic_pool;
  obj_index[real_object0(19864)].func.obj = magic_pool;
  obj_index[real_object0(19900)].func.obj = githyanki;
  mob_index[real_mobile0(19750)].func.mob = guild_guard;
  mob_index[real_mobile0(19704)].func.mob = demogorgon;
  mob_index[real_mobile0(19920)].func.mob = demogorgon_shout;
  /*
     Ethereal Plane - Main Grid
   */

  obj_index[real_object0(12460)].func.obj = magic_pool;
  obj_index[real_object0(12470)].func.obj = teleporting_pool;
  obj_index[real_object0(12472)].func.obj = teleporting_pool;
  obj_index[real_object0(12474)].func.obj = teleporting_pool;
  obj_index[real_object0(12476)].func.obj = teleporting_pool;
  obj_index[real_object0(45536)].func.obj = teleporting_pool;
#if 0
  obj_index[real_object0(12462)].func.obj = teleporting_pool;
  obj_index[real_object0(12464)].func.obj = teleporting_pool;
  obj_index[real_object0(12466)].func.obj = teleporting_pool;
  obj_index[real_object0(12478)].func.obj = teleporting_pool;
  obj_index[real_object0(12480)].func.obj = teleporting_pool;
  obj_index[real_object0(12482)].func.obj = teleporting_pool;
  obj_index[real_object0(12484)].func.obj = teleporting_pool;
#endif
   
  /* illusionist dream plane */

   obj_index[real_object0(30402)].func.obj = teleporting_map_pool;
   obj_index[real_object0(30403)].func.obj = teleporting_pool;
   obj_index[real_object0(30404)].func.obj = teleporting_pool;
   obj_index[real_object0(30405)].func.obj = teleporting_pool;
   obj_index[real_object0(30406)].func.obj = teleporting_pool;
   obj_index[real_object0(30407)].func.obj = teleporting_pool;
   obj_index[real_object0(30408)].func.obj = teleporting_pool;
   obj_index[real_object0(30409)].func.obj = teleporting_pool;
   obj_index[real_object0(30410)].func.obj = teleporting_pool;
   obj_index[real_object0(30411)].func.obj = teleporting_pool;
   obj_index[real_object0(30412)].func.obj = teleporting_pool;
   obj_index[real_object0(30413)].func.obj = teleporting_pool;

  /*
     The Domain of Lost Souls
   */
  obj_index[real_object0(36884)].func.obj = kvasir_dagger;

  /*
     Plane of Fire One
   */
  obj_index[real_object0(25018)].func.obj = doombringer;
  //obj_index[real_object0(25030)].func.obj = flamberge;
  obj_index[real_object0(430)].func.obj = flamberge;
  obj_index[real_object0(25080)].func.obj = ring_elemental_control;

  /*
     Plane of Fire Two
   */
  obj_index[real_object0(25105)].func.obj = holy_mace;
  obj_index[real_object0(25103)].func.obj = staff_of_blue_flames;
  obj_index[real_object0(30)].func.obj = staff_of_power;
  obj_index[real_object0(40409)].func.obj = reliance_pegasus;

  /*
     Plane of Fire Three
   */
  obj_index[real_object0(25404)].func.obj = magic_pool;

  /*
     Plane of Air
   */
  obj_index[real_object0(24404)].func.obj = magic_pool;
  obj_index[real_object0(24406)].func.obj = lightning;
  /*
    Plane of Air Two
  */
  obj_index[real_object0(131616)].func.obj = dagger_of_wind;

  /*
     Plane of Water
   */
  obj_index[real_object0(23204)].func.obj = magic_pool;
  obj_index[real_object0(23206)].func.obj = orb_of_the_sea;

  /*
     Plane of Earth
   */
  obj_index[real_object0(23804)].func.obj = magic_pool;
  obj_index[real_object0(23805)].func.obj = zion_mace_of_earth;

  /*
     Etherial Plane
   */
  obj_index[real_object0(41203)].func.obj = unholy_avenger_bloodlust;

  /*
     Plane of Hell One
   */
  obj_index[real_object0(51005)].func.obj = holy_weapon;
  obj_index[real_object0(51006)].func.obj = tiamat_stinger;


  /*
     Plane of Hell Two
   */
  obj_index[real_object0(51401)].func.obj = dispator;


  /*
     Swamp Part Two
   */
  obj_index[real_object0(26014)].func.obj = nightbringer;

  /*
     Teka 2
   */
/*  obj_index[real_object0(75561)].func.obj = flaming_mace_ruzdo;*/

  /*
     Moonshae Island I
   */
//  obj_index[real_object0(26233)].func.obj = cymric_hugh;

  /*
     Temple Zone
   */
//  obj_index[real_object0(18302)].func.obj = illyns_sword;

  /*
     Myrloch Vale
   */
  obj_index[real_object0(26413)].func.obj = item_switch;
  obj_index[real_object0(26414)].func.obj = item_switch;
  obj_index[real_object0(26415)].func.obj = item_switch;
  obj_index[real_object0(26416)].func.obj = item_switch;


  /*
     undermountain
   */

  obj_index[real_object0(92090)].func.obj = undead_trident;
  obj_index[real_object0(92080)].func.obj = generic_drow_eq;
  obj_index[real_object0(92081)].func.obj = generic_drow_eq;
  obj_index[real_object0(92082)].func.obj = generic_drow_eq;
  obj_index[real_object0(92086)].func.obj = generic_drow_eq;
  obj_index[real_object0(92065)].func.obj = iron_flindbar;
  obj_index[real_object0(92020)].func.obj = generic_parry_proc;
  obj_index[real_object0(82500)].func.obj = generic_parry_proc;
  obj_index[real_object0(18000)].func.obj = generic_riposte_proc;
//  obj_index[real_object0(92121)].func.obj = flame_of_north;

  /*
     Menden-on-the-Deep
   */
  obj_index[real_object0(88825)].func.obj = menden_figurine;
  obj_index[real_object0(88830)].func.obj = llyms_altar;
  obj_index[real_object0(88821)].func.obj = magic_pool;
  obj_index[real_object0(88827)].func.obj = magic_pool;
  /*
     The Fog-Enshrouded Woods
   */
  obj_index[real_object0(90004)].func.obj = fw_ruby_monocle;
  obj_index[real_object0(90010)].func.obj = item_switch;

  /*
     Sevenoaks Stuf
   */
  obj_index[real_object0(38649)].func.obj = sevenoaks_longsword;
//  obj_index[real_object0(38647)].func.obj = sevenoaks_mace;


  obj_index[real_object0(67262)].func.obj = mace_of_sea;
//  obj_index[real_object0(67258)].func.obj = icicle_saber;
  obj_index[real_object0(67237)].func.obj = serpent_blade;
  obj_index[real_object0(67242)].func.obj = lich_spine;

  /*
     newbiezone
   */
  obj_index[real_object0(22803)].func.obj = newbie_portal;
  obj_index[real_object0(22800)].func.obj = newbie_sign1;
  obj_index[real_object0(22801)].func.obj = newbie_sign2;

  // god toys
  obj_index[real_object0(174)].func.obj = vareena_statue;
  obj_index[real_object0(352)].func.obj = doom_blade_Proc;
  obj_index[real_object0(426)].func.obj = doom_blade_Proc;
  
 /* Winterhaven */

  obj_index[real_object0(55008)].func.obj = storage_locker_obj_hook;
  obj_index[real_object0(55026)].func.obj = board;
  obj_index[real_object0(55197)].func.obj = board;
  obj_index[real_object0(55210)].func.obj = dagger_ra;
  obj_index[real_object0(55205)].func.obj = illithid_axe; 
  obj_index[real_object0(55211)].func.obj = deathseeker_mace; 
  obj_index[real_object0(55301)].func.obj = demon_slayer;
  obj_index[real_object0(55277)].func.obj = snowogre_warhammer; 
  obj_index[real_object0(55306)].func.obj = volo_longsword; 
  obj_index[real_object0(55304)].func.obj = blur_shortsword;
  obj_index[real_object0(55322)].func.obj = buckler_saints;
  obj_index[real_object0(55309)].func.obj = helmet_vampires;
  obj_index[real_object0(55315)].func.obj = storm_legplates;
  obj_index[real_object0(55280)].func.obj = gauntlets_legend;
  obj_index[real_object0(55377)].func.obj = gladius_backstabber; 
//  obj_index[real_object0(55311)].func.obj = boots_abyss;
//  obj_index[real_object0(55307)].func.obj = platemail_fame;
//  obj_index[real_object0(55338)].func.obj = poseidon_trident;
//  obj_index[real_object0(55341)].func.obj = sword_random;
//  obj_index[real_object0(55340)].func.obj = rapier_penetration;
  obj_index[real_object0(55362)].func.obj = attribute_scroll;
  obj_index[real_object0(97923)].func.obj = elemental_wand;
  obj_index[real_object0(55361)].func.obj = earring_powers;
  obj_index[real_object0(55363)].func.obj = lorekeeper_scroll;
  obj_index[real_object0(55500)].func.obj = 
  obj_index[real_object0(55501)].func.obj = 
  obj_index[real_object0(55502)].func.obj = 
  obj_index[real_object0(55503)].func.obj = 
  obj_index[real_object0(55504)].func.obj = 
  obj_index[real_object0(55505)].func.obj = 
  obj_index[real_object0(55506)].func.obj = 
  obj_index[real_object0(55507)].func.obj = 
  obj_index[real_object0(55508)].func.obj = 
  obj_index[real_object0(55509)].func.obj =
  obj_index[real_object0(55510)].func.obj = 
  obj_index[real_object0(55511)].func.obj = 
  obj_index[real_object0(55512)].func.obj = 
  obj_index[real_object0(55513)].func.obj = 
  obj_index[real_object0(55514)].func.obj = 
  obj_index[real_object0(55515)].func.obj = 
  obj_index[real_object0(55516)].func.obj = 
  obj_index[real_object0(55517)].func.obj = 
  obj_index[real_object0(55518)].func.obj = 
  obj_index[real_object0(55519)].func.obj = 
  obj_index[real_object0(55520)].func.obj = wh_corpse_decay; 
  obj_index[real_object0(53661)].func.obj = damnation_staff; 
  obj_index[real_object0(53662)].func.obj = nuke_damnation; 
  obj_index[real_object0(53663)].func.obj = nuke_damnation; 
  obj_index[real_object0(97932)].func.obj = collar_frost; 
  obj_index[real_object0(97931)].func.obj = collar_flames; 
  obj_index[real_object0(55080)].func.obj = dragon_heart_decay; 
  obj_index[real_object0(55081)].func.obj = dragon_heart_decay; 
  obj_index[real_object0(55082)].func.obj = dragon_heart_decay; 
  obj_index[real_object0(55420)].func.obj = lancer_gift; 
  obj_index[real_object0(55077)].func.obj = splinter;
  
/* Same problem as with the func.mob, no dice! */

  /* Harpy */
 // obj_index[real_object0(31100)].func.obj = harpy_gate;

  /* Zion Procs */
  obj_index[real_object0(19638)].func.obj = zion_shield_absorb_proc;
  obj_index[real_object0(38761)].func.obj = generic_shield_block_proc;
  obj_index[real_object0(25030)].func.obj = zion_fnf;
  obj_index[real_object0(16242)].func.obj = zion_light_dark;
  
  /* Barovia Procs */
  obj_index[real_object0(58825)].func.obj = barovia_undead_necklace;
  obj_index[real_object0(58424)].func.obj = artifact_shadow_shield;

  /* Ravenloft Procs */
  obj_index[real_object0(58413)].func.obj = ravenloft_bell;
  mob_index[real_mobile0(58387)].func.mob = ravenloft_vistani_shout;
  //mob_index[real_mobile0(58383)].func.mob = strahd_charm;
  obj_index[real_object0(58400)].func.obj = shimmer_shortsword;

  /* Basin Wastes Procs */
  mob_index[real_mobile0(34014)].func.mob = block_dir;

  /* Temple of Earth Procs */
  mob_index[real_mobile0(43576)].func.mob = eligoth_rift_spawn;
  obj_index[real_object0(43584)].func.obj = toe_chamber_switch;
  
  /* Snogres Procs */
  mob_index[real_mobile0(87704)].func.mob = block_dir;
  mob_index[real_mobile0(87741)].func.mob = block_dir;
  mob_index[real_mobile0(87743)].func.mob = block_dir;
  mob_index[real_mobile0(87734)].func.mob = block_dir;
  mob_index[real_mobile0(87733)].func.mob = snogres_lich_shout;
  mob_index[real_mobile0(87734)].func.mob = snogres_flesh_golem;
  mob_index[real_mobile0(87700)].func.mob = berserker_toss;
  mob_index[real_mobile0(87724)].func.mob = remo_burn;
  obj_index[real_object0(87712)].func.obj = hellfire_axe;
  obj_index[real_object0(87724)].func.obj = illithid_whip;
  obj_index[real_object0(87701)].func.obj = skull_leggings;
  obj_index[real_object0(87737)].func.obj = flesh_golem_repop;

  /* Morgs Proc */
 //mob_index[real_mobile0(87880)].func.mob = morgs_protect;
  mob_index[real_mobile0(87891)].func.mob = world_quest;

  /* Keleks Proc */
  obj_index[real_object0(87950)].func.obj = deliverer_hammer;
 
  /* New Caer Tannad */
  mob_index[real_mobile0(78476)].func.mob = caertannad_summon;

  /* Ruins of Tharn */
  mob_index[real_mobile0(5503)].func.mob = undead_howl;

  /* Dungeon */
  obj_index[real_object0(93007)].func.obj = blue_sword_armor;

  /* Shadow Forest */
  mob_index[real_mobile0(6307)].func.mob = mist_protect;

  /* Quietus Quay */
  mob_index[real_mobile0(1709)].func.mob = world_quest;
  world[real_room0(1719)].funct = ship_shop_proc;
 
  /* Charcoal Palace */
  mob_index[real_mobile0(88316)].func.mob = kossuth;
  mob_index[real_mobile0(88319)].func.mob = fruaack_shout;
  mob_index[real_mobile0(88301)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88302)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88303)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88304)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88305)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88306)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88308)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88310)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88323)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88324)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88325)].func.mob = charcoal_guard;
  mob_index[real_mobile0(88329)].func.mob = block_dir;

}

/*
   assign special procedures to rooms
 */


void assign_rooms(void)
{

  int      x;

  for (x = 32907; x <= 32909; x++)
  {
    world[real_room0(x)].funct = shaboath_alternation_tower;
  }

  for (x = 32910; x <= 32911; x++)
  {
    world[real_room0(x)].funct = shaboath_necromancy_tower;
  }

  for (x = 32901; x <= 32902; x++)
  {
    world[real_room0(x)].funct = shaboath_enchantment_tower;
  }

  world[real_room0(1201)].funct = player_council_room;


  // testing for multiclass
  //world[real_room0(1200)].funct = multiclass_proc;
  //world[real_room0(6801)].funct = multiclass_proc;
  world[real_room0(52904)].funct = multiclass_proc;
  
// Dwarf and duergar berserkers
// Gilaxi's Hidden Chamber in Torgs
//world[real_room0(29181)].funct = berserker_proc_room;
  
  /* squid arena */
  for (x = 11200; x <= 11217; x++)
  {
    world[real_room0(x)].funct = squid_arena;
  }

  world[real_room0(19890)].funct = GithyankiCave;
 // world[real_room0(19617)].funct = TiamatThrone;
  
  /* inns */
  world[real_room0(18729)].funct = inn;
  world[real_room0(35264)].funct = inn;
  world[real_room0(37715)].funct = inn;
  world[real_room0(37313)].funct = inn;
  world[real_room0(40454)].funct = inn;
  world[real_room0(37434)].funct = inn;
  world[real_room0(21611)].funct = inn;
  world[real_room0(67493)].funct = inn;
  world[real_room0(18729)].funct = inn;
  world[real_room0(67503)].funct = inn;
  world[real_room0(81070)].funct = inn;
  world[real_room0(81028)].funct = inn; 
  world[real_room0(81019)].funct = inn;
  world[real_room0(81078)].funct = inn;
  world[real_room0(81003)].funct = inn;
  world[real_room0(77442)].funct = inn;
  world[real_room0(82217)].funct = inn;
  world[real_room0(82574)].funct = inn;
  world[real_room0(99715)].funct = inn;
  world[real_room0(10857)].funct = inn;
  world[real_room0(3398)].funct = inn;
  world[real_room0(1736)].funct = inn;
  world[real_room0(139078)].funct = inn;


  /* Aracdrathos */
  world[real_room0(36564)].funct = inn;
  world[real_room0(36567)].funct = pet_shops;


  world[real_room0(74916)].funct = stat_shops;
  world[real_room0(28281)].funct = stat_shops;


  /* minizones */
  world[real_room0(5783)].funct = pet_shops;
  obj_index[real_object0(5805)].func.obj = sword_named_magik;
/*
  mob_index[real_mobile0(5808)].func.mob = plant_attacks_poison;
  mob_index[real_mobile0(5809)].func.mob = plant_attacks_paralysis;
*/
  world[real_room0(37716)].funct = inn;
  world[real_room0(66355)].funct = undead_inn;
  world[real_room0(14362)].funct = inn;
 
  world[real_room0(43341)].funct = patrol_shops;
  world[real_room0(45006)].funct = inn;
  world[real_room0(45036)].funct = pet_shops;
  world[real_room0(29280)].funct = pet_shops;
  world[real_room0(29282)].funct = pet_shops;
  obj_index[real_object0(29236)].func.obj = newbie_portal;
  /* centaur fort */
  world[real_room0(5300)].funct = inn;

  /* Dragonnia */
  world[real_room0(6841)].funct = room_of_sanctum;

  /* Vecna */
  world[real_room0(130079)].funct = vecna_bubble_room;
  
  /* Ugta */
  world[real_room0(39313)].funct = inn;

  /* Shady */
  world[real_room0(97757)].funct = pet_shops;
  world[real_room0(76241)].funct = pet_shops;

  world[real_room0(97663)].funct = inn;

  /* thri town */
  world[real_room0(67710)].funct = inn;

  /* Khildarak */
  /*world[real_room0(17075)].funct = duergar_guild;
  world[real_room0(17223)].funct = duergar_guild;
  world[real_room0(17554)].funct = duergar_guild;
  world[real_room0(17720)].funct = duergar_guild;
  world[real_room0(17724)].funct = duergar_guild;*/
  world[real_room0(17302)].funct = pet_shops;
  world[real_room0(17591)].funct = pet_shops;
  world[real_room0(17087)].funct = inn;

  /* tharn */
  world[real_room0(132575)].funct = inn;
  world[real_room0(132614)].funct = pet_shops;
  world[real_room0(132868)].funct = pet_shops;
  world[real_room0(132743)].funct = pet_shops;
  //world[real_room0(6683)].funct = ship_shop_proc;
  world[real_room0(123113)].funct = ship_shop_proc;
  world[real_room0(88846)].funct = ship_shop_proc;
  world[real_room0(43198)].funct = ship_shop_proc;
  world[real_room0(43158)].funct = ship_shop_proc;
  world[real_room0(140854)].funct = ship_shop_proc;
  world[real_room0(258421)].funct = ship_shop_proc;
  world[real_room0(22441)].funct = ship_shop_proc;
  world[real_room0(70501)].funct = ship_shop_proc;        /* rax's quest zone */
  world[real_room0(258421)].funct = ship_shop_proc;
  world[real_room0(43118)].funct = ship_shop_proc;
  world[real_room0(635260)].funct = ship_shop_proc;
  world[real_room0(584171)].funct = ship_shop_proc;

  world[real_room0(77)].funct = crew_shop_proc;     // whole-list room
  world[real_room0(1734)].funct = crew_shop_proc;   // Quietus
  world[real_room0(9704)].funct = crew_shop_proc;   // Sarmiz
  world[real_room0(43220)].funct = crew_shop_proc;  // WH Harbor
  world[real_room0(43221)].funct = crew_shop_proc;  // Thur'Gurdax
  world[real_room0(43222)].funct = crew_shop_proc;  // Fenaline
  world[real_room0(28197)].funct = crew_shop_proc;  // Vesprin
  world[real_room0(22481)].funct = crew_shop_proc;  // Stormport
  world[real_room0(66735)].funct = crew_shop_proc;  // Torrhan
  world[real_room0(82641)].funct = crew_shop_proc;  // Myrabolus
  world[real_room0(54240)].funct = crew_shop_proc;  // tent at Fiord
  world[real_room0(38107)].funct = crew_shop_proc;  // Boyard
  world[real_room0(49051)].funct = crew_shop_proc;  // Venan'Trut (market inn)
  world[real_room0(81021)].funct = crew_shop_proc;  // Ceothia
  world[real_room0(76859)].funct = crew_shop_proc;  // Jade shipguild
  world[real_room0(132766)].funct = crew_shop_proc;  // Tharnadia
  world[real_room0(55418)].funct = crew_shop_proc;  // Winterhaven
  //world[real_room0()].funct = crew_shop_proc;  // TODO: Stronghold
  world[real_room0(22648)].funct = crew_shop_proc;  // Stronghold -temporary room

  // Fiord on west side of GC.
  world[real_room0(559633)].funct = ship_shop_proc;

  /* Myrabolis */
  world[real_room0(82670)].funct = ship_shop_proc;

  /* Venan */
  world[real_room0(49090)].funct = ship_shop_proc;

  /* Bloodstone */
  world[real_room0(74271)].funct = inn;

  /* Minotaur hometown NAX */
  world[real_room0(37716)].funct = inn;


  /* Sylvandawn */
  world[real_room0(8010)].funct = pet_shops;
  world[real_room0(8211)].funct = dump;
  world[real_room0(8323)].funct = pet_shops;
  world[real_room0(8003)].funct = inn;
  world[real_room0(8287)].funct = ship_shop_proc;

  /* Sarmiz'Duul */
  world[real_room0(9738)].funct = inn;
  world[real_room0(9967)].funct = ship_shop_proc;
   
  /* Torrhan */
  world[real_room0(66689)].funct = ship_shop_proc;

  /* Jade? */
  world[real_room0(76659)].funct = ship_shop_proc;

  /* Players Guild */
  world[real_room0(30504)].funct = inn;
  world[real_room0(29701)].funct = inn;
  world[real_room0(29305)].funct = inn;
  world[real_room0(29103)].funct = inn;
  world[real_room0(29502)].funct = inn;
  world[real_room0(30511)].funct = inn;
  world[real_room0(30104)].funct = inn;
  world[real_room0(29903)].funct = inn;
  world[real_room0(29202)].funct = inn;
  world[real_room0(29403)].funct = inn;
  world[real_room0(30303)].funct = inn;

  /* clav's tundra zone */

  world[real_room0(13714)].funct = inn;

#if 0
  /* Realms Master */
  world[real_room0(11100)].funct = ship_exit_room;
  world[real_room0(11101)].funct = ship_look_out_room;
  world[real_room0(11102)].funct = ship_look_out_room;
  world[real_room0(11103)].funct = ship_look_out_room;
  world[real_room0(11104)].funct = ship_look_out_room;
  world[real_room0(11105)].funct = ship_look_out_room;
  world[real_room0(11108)].funct = ship_look_out_room;
  world[real_room0(11117)].funct = ship_look_out_room;
  world[real_room0(11118)].funct = ship_look_out_room;
  world[real_room0(11119)].funct = ship_look_out_room;
  world[real_room0(11120)].funct = ship_look_out_room;
  world[real_room0(11121)].funct = ship_look_out_room;
  world[real_room0(11123)].funct = ship_look_out_room;

  /* Silver Lady */
  world[real_room0(11300)].funct = ship_exit_room;
  world[real_room0(11301)].funct = ship_look_out_room;
  world[real_room0(11302)].funct = ship_look_out_room;
  world[real_room0(11303)].funct = ship_look_out_room;
  world[real_room0(11304)].funct = ship_look_out_room;
  world[real_room0(11305)].funct = ship_look_out_room;
  world[real_room0(11308)].funct = ship_look_out_room;
  world[real_room0(11317)].funct = ship_look_out_room;
  world[real_room0(11318)].funct = ship_look_out_room;
  world[real_room0(11319)].funct = ship_look_out_room;
  world[real_room0(11320)].funct = ship_look_out_room;
  world[real_room0(11321)].funct = ship_look_out_room;
  world[real_room0(11323)].funct = ship_look_out_room;

  /*
     Sylvan Wind
   */
  world[real_room0(11000)].funct = ship_exit_room;
  world[real_room0(11009)].funct = ship_look_out_room;
#endif

  /*
     Morning Star
   */
  /*
     Ghore
   */
  world[real_room0(11923)].funct = dump;
  world[real_room0(11901)].funct = inn;
  /*
     misc
   */
  world[real_room0(36334)].funct = inn;
  world[real_room0(6535)].funct = inn;
  world[real_room0(1444)].funct = inn;
  world[real_room0(26566)].funct = inn;
  world[real_room0(15264)].funct = inn;
  world[real_room0(95569)].funct = inn;
  world[real_room0(96537)].funct = inn;

  /*
     Lava Tubes One
   */
  world[real_room0(12158)].funct = automaton_trapdoor;

  /*
     Twin Towers - Forest
   */
  world[real_room0(13553)].funct =
    world[real_room0(13555)].funct =
    world[real_room0(13556)].funct =
    world[real_room0(13557)].funct =
    world[real_room0(13558)].funct =
    world[real_room0(13564)].funct =
    world[real_room0(13565)].funct =
    world[real_room0(13567)].funct =
    world[real_room0(13568)].funct =
    world[real_room0(13569)].funct =
    world[real_room0(13570)].funct =
    world[real_room0(13571)].funct = gardener_block;

  /* Halfling HT - Woodseer */
  world[real_room0(16558)].funct = inn;
  world[real_room0(16886)].funct = pet_shops;

  /*
     Dwarven Mines
   */
//  world[real_room0(45118)].funct = GlyphOfWarding;  // not anymore, now it's charing

  /*
     Ashrumite
   */
  world[real_room0(66054)].funct = inn;
  world[real_room0(66080)].funct = dump;
  world[real_room0(66116)].funct = pet_shops;


  /* Fog Enshrouded Woods */
  world[real_room0(90107)].funct = fw_warning_room;
  world[real_room0(90112)].funct = fw_warning_room;
  world[real_room0(90114)].funct = fw_warning_room;

  /* goblin hometown */
  world[real_room0(70175)].funct = inn;

  /* ADD THIS FOR HEAVEN */
  world[real_room0(ILLITHID_HEAVEN_ROOM)].funct = mortal_heaven;
  world[real_room0(EVIL_HEAVEN_ROOM)].funct = mortal_heaven;
  world[real_room0(GOOD_HEAVEN_ROOM)].funct = mortal_heaven;
  world[real_room0(UNDEAD_HEAVEN_ROOM)].funct = mortal_heaven;
  mob_index[real_mobile0(46)].func.mob = epic_teacher;
  mob_index[real_mobile0(54)].func.mob = epic_familiar;
  mob_index[real_mobile0(55)].func.mob = epic_familiar;
  mob_index[real_mobile0(56)].func.mob = epic_familiar;
  mob_index[real_mobile0(57)].func.mob = epic_familiar;
  mob_index[real_mobile0(58)].func.mob = epic_familiar;
  mob_index[real_mobile0(59)].func.mob = epic_familiar;
  
  /* Tradeskills */
  mob_index[real_mobile0(132674)].func.mob = learn_tradeskill;
  mob_index[real_mobile0(17264)].func.mob = learn_tradeskill;
  mob_index[real_mobile0(400000)].func.mob = epic_store;
  mob_index[real_mobile0(400001)].func.mob = pvp_store;
  mob_index[real_mobile0(132675)].func.mob = assoc_founder;
  mob_index[real_mobile0(17265)].func.mob = assoc_founder;

  /* Bronze Citadel */ 
  obj_index[real_object0(32486)].func.obj = bel_sword;

  /* Varathorns keep */
  obj_index[real_object0(99432)].func.obj = zarthos_vampire_slayer;  

  /* Domain proc */
  obj_index[real_object0(36894)].func.obj = critical_attack_proc;

  /* Lost Temple of Tikitzopl */
  obj_index[real_object0(44165)].func.obj = unmulti_altar;

  /* Trakkia */
  world[real_room0(57068)].funct = pet_shops;
  
  /* Jubilex */
  mob_index[real_mobile0(87542)].func.mob = slime_lake;
  mob_index[real_mobile0(87543)].func.mob = jubilex_one;

  obj_index[real_object0(87546)].func.obj = mask_of_wildmagic; 
  //obj_index[real_object0(87600)].func.obj = ebb_vambraces; 
  obj_index[real_object0(87601)].func.obj = flow_amulet; 
  obj_index[real_object0(87611)].func.obj = jubilex_grid_mob_generator; 
  obj_index[real_object0(67274)].func.obj = zion_netheril;

  /* Fortress of Dreams */
  mob_index[real_mobile0(32604)].func.mob = block_dir;
  mob_index[real_mobile0(32613)].func.mob = block_dir;
  mob_index[real_mobile0(32624)].func.mob = eth2_forest_animal;
  mob_index[real_mobile0(32625)].func.mob = eth2_forest_animal;
  mob_index[real_mobile0(32626)].func.mob = eth2_forest_animal;
  mob_index[real_mobile0(32627)].func.mob = eth2_little_girl;
  mob_index[real_mobile0(32631)].func.mob = eth2_demon_princess;
  mob_index[real_mobile0(32628)].func.mob = block_dir;
  mob_index[real_mobile0(32629)].func.mob = block_dir;
  mob_index[real_mobile0(32637)].func.mob = eth2_aramus;

  obj_index[real_object0(32615)].func.obj = eth2_tree_obj; 
  obj_index[real_object0(32614)].func.obj = eth2_tree_obj;
  obj_index[real_object0(32619)].func.obj = eth2_godsfury;
  obj_index[real_object0(32621)].func.obj = eth2_aramus_crown;
  
  // Lucrot procs 
  obj_index[real_object0(424)].func.obj = lucrot_mindstone;

  // Alatorin
  mob_index[real_mobile0(83414)].func.mob = rentacleric;
  obj_index[real_object0(120051)].func.obj = wh_corpse_decay;
  mob_index[real_mobile0(120051)].func.mob = wh_corpse_to_object;

  // Gellz Special Procs added 2015 (gellz)
  obj_index[real_object0(55433)].func.obj = magic_deck;
  obj_index[real_object0(1203)].func.obj = gellz_test_obj_procs;
  
  // NPC Ship Crews
  assign_ship_crew_funcs();
}

/*
   ********************************************************************
   *  Handling funcs                                                     *
   ********************************************************************
 */

void room_procs(void)
{
  int      i;

  for (i = 0; i < top_of_world; i++)
    if (world[i].funct)
      (*world[i].funct) (i, NULL, 0, NULL);
}

void item_procs(void)
{
  P_obj    i;

  for (i = object_list; i; i = i->next)
    if (obj_index[i->R_num].func.obj)
      (*obj_index[i->R_num].func.obj) (i, NULL, 0, NULL);
}
