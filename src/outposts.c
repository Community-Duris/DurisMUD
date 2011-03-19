//
// Outposts and buildings are not attached to the mud, but is here in case someone wants to clean up and finish the code,
// which was started by Torgal in 2008 and then continued by Venthix in 2009.
// - Torgal 1/29/2010
//

/* outposts.c
   - Property of Duris
     Mar 09

*/

#include <stdlib.h>
#include <cstring>
#include <vector>
using namespace std;

#include "prototypes.h"
#include "defines.h"
#include "utils.h"
#include "structs.h"
#include "nexus_stones.h"
#include "racewar_stat_mods.h"
#include "sql.h"
#include "interp.h"
#include "comm.h"
#include "spells.h"
#include "db.h"
#include "damage.h"
#include "epic.h"
#include "buildings.h"
#include "outposts.h"
#include "assocs.h"
#include "specs.prototypes.h"
#include "utility.h"
#include "guildhall.h"
#include "alliances.h"

extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern P_desc descriptor_list;
extern P_char character_list;
extern P_obj object_list;
extern const char *apply_names[];
extern BuildingType building_types[];
extern const char *dirs[];
extern const int rev_dir[NUM_EXITS];
extern bool create_walls(int room, int exit, P_char ch, int level, int type,
                         int power, int decay, char *short_desc, char *desc,
                         ulong flags);
extern vector<Building*> buildings;
extern const long boot_time;

int upkeep_time;


#define CAN_CONSTRUCT_CMD(ch) ( GET_A_NUM(ch) && (IS_LEADER(GET_M_BITS(GET_A_BITS(ch), A_RK_MASK)) || GT_LEADER(GET_M_BITS(GET_A_BITS(ch), A_RK_MASK))) )

#ifdef __NO_MYSQL__
int init_outposts()
{
    // load nothing
}

void do_outpost(P_char ch, char *arg, int cmd) 
{
    // do nothing
}

int get_current_outpost_hitpoints(Building *building)
{
  return 0;
}

void outpost_update_resources(P_char ch, int wood, int stone)
{
  
}

int outpost_rubble(P_obj obj, P_char ch, int cmd, char *arg)
{
  return 0;
}
void set_current_outpost_hitpoints(Building *building)
{
  
}
void outpost_death(P_char outpost, P_char killer)
{
  
}
#else

extern MYSQL* DB;

// Location, entrance dir
int outpost_locations[][2] = {
        // Outpost ID - 1
  {588184, EAST},     // 0 - UC
  {628856, WEST},     // 1 - KK
  {531100, SOUTH},    // 2 - IC
  {0, 0}
};

int init_outposts()
{
  fprintf(stderr, "-- Booting outposts\r\n");
  
  // Outpost specs, (can be found in building.c)
  //mob_index[real_mobile(0)].func.mob = ;
  //obj_index[real_object(0)].func.obj = ;

  load_outposts();
  //init_outpost_resources();
}

int load_outposts()
{
  // load outposts from DB
  if( !qry("SELECT id, owner_id, level, walls, portal_room, golems FROM outposts") )
  {
    debug("load_outposts() can't read from db");
    return FALSE;
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
 
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return FALSE;
  }
 
  Building* building;

  MYSQL_ROW row;
  while( row = mysql_fetch_row(res) )
  {
    int id = atoi(row[0]);
    int guild = atoi(row[1]);
    int level = atoi(row[2]);
    int walls = atoi(row[3]);
    int portal = atoi(row[4]);
    int golems = atoi(row[5]);

    if (building = load_building(guild, BUILDING_OUTPOST, outpost_locations[id][0], level))
    {
      building->golem_dir = outpost_locations[id][1];
      outpost_generate_walls(building, walls, golems);
      if (portal) outpost_generate_portals(building);
    }
  }
  
  mysql_free_result(res);

  return TRUE;
}

void show_outposts(P_char ch)
{
  char buff[MAX_STRING_LENGTH], Gbuf1[MAX_STRING_LENGTH];
  char title[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int i;
  FILE *f;
  Building *building;

  send_to_char("&+WList of outposts:&n\r\n", ch);
  for (i = 0; i <= buildings.size(); i++)
  {
    building = get_building_from_id(i+1);
    if (!qry("SELECT id, owner_id, archers, portal_room, golems, hitpoints FROM outposts WHERE id = %d", i))
    {
      debug("show_outposts() cant read from db");
      return;
    }

    MYSQL_RES *res = mysql_store_result(DB);

    if (mysql_num_rows(res) < 1)
    {
      mysql_free_result(res);
      return;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    int owner = atoi(row[1]);
    int archers = atoi(row[2]);
    int portal = atoi(row[3]);
    int golems = atoi(row[4]);
    int hitp = atoi(row[5]);

    mysql_free_result(res);

    sprintf(Gbuf1, "%sasc.%u", ASC_DIR, (ush_int)owner);
    f = fopen(Gbuf1, "r");
    if (!f)
    {
      sprintf(title, "Unknown");
    }
    else
    {
      fgets(Gbuf2, MAX_STR_NORMAL, f);
      Gbuf2[strlen(Gbuf2)-1] = 0;
      Gbuf2[MAX_STR_ASC-1] = 0;
      strcpy(title, Gbuf2);
      fclose(f);
    }

    sprintf(buff, "&+W*ID: &+c%2d &+WContinent: &+c%-18s&n &+WOwner: &+c%-15s&n\r\n", i+1, pad_ansi(continent_name(world[building->location()].continent), 18).c_str(), title);
    send_to_char(buff, ch);
    if (IS_TRUSTED(ch) || ((owner != 0) && (owner == GET_A_NUM(ch))))
    {
      sprintf(buff, "       &+LGateguards: &+c%d &+LPortal: &+c%-4s &+LArchers: &+c%-4s&n\r\n", golems, (portal ? "Yes" : "No"), (archers ? "Yes" : "No"));
      send_to_char(buff, ch);
      int basehit = building_types[BUILDING_OUTPOST-1].hitpoints;
      sprintf(buff, "       &+LOutpost Condition: %s%-6d&+L/&+c%d&n\r\n",
	  ((hitp <= basehit*10/100) ? "&+R" :
	   (hitp <= basehit*30/100) ? "&+r" :
	   (hitp <= basehit*50/100) ? "&+m" :
	   (hitp <= basehit*70/100) ? "&+M" :
	   (hitp <= basehit*90/100) ? "&+y" :
	   "&+c"), hitp, basehit);
      send_to_char(buff, ch);
      if (i+1 < buildings.size())
	send_to_char("\r\n", ch);
    }
  }
}

int get_current_outpost_hitpoints(Building *building)
{
  if (!qry("SELECT id, hitpoints FROM outposts WHERE id = %d", building->id-1))
  {
    debug("get_current_outpost_hitpoints() cant read from db");
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return FALSE;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  int hitpoints = atoi(row[1]);

  mysql_free_result(res);

  return hitpoints;
}

int get_outpost_owner(Building *building)
{
  if (!qry("SELECT id, owner_id FROM outposts WHERE id = %d", building->id-1))
  {
    debug("get_outpost_owner() cant read from db");
    return FALSE;
  }
  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return FALSE;
  }
  MYSQL_ROW row = mysql_fetch_row(res);

  int owner = atoi(row[1]);
  
  mysql_free_result(res);

  return owner;
}

int get_outpost_resources(Building *building, int type)
{
  if (!qry("SELECT id, owner_id FROM outposts WHERE id = %d", building->id-1))
  {
    debug("get_outpost_resources() cant read from db");
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return FALSE;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  int id = atoi(row[1]);

  mysql_free_result(res);

  int resources = get_guild_resources(id, type);

  return resources;
}

int get_outpost_golems(Building *building)
{
  if (!qry("SELECT id, golems FROM outposts WHERE id = %d", building->id-1))
  {
    debug("get_outpost_resources() cant read from db");
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return FALSE;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  int golems = atoi(row[1]);

  mysql_free_result(res);

  return golems;
}

int get_guild_resources(int id, int type)
{
  if ((type != WOOD) && (type != STONE))
  {
    debug("get_guild_resources() passed invalid type %d", type);
    return FALSE;
  }
  if (!id)
  {
    debug("get_guild_resources() passed invalid guild id %d", id);
    return FALSE;
  }
  
  if (!qry("SELECT id, wood, stone FROM associations WHERE id = %d", id));
  {
    // WHY IS THIS FAILING?
    debug("get_guild_resources() cant read from db");
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return FALSE;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);

  int resources = atoi(row[type]);

  mysql_free_result(res);
  
  return resources;
}

void set_current_outpost_hitpoints(Building *building)
{
  db_query("UPDATE outposts SET hitpoints='%d' WHERE id='%d'", ((GET_HIT(building->mob) < 0) ? 0 : GET_HIT(building->mob)), building->id-1);
}
  
void do_outpost(P_char ch, char *arg, int cmd)
{
  char buff[MAX_STRING_LENGTH];
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];
  P_char op = NULL;
  P_obj rubble;
  Building *building;

  if( !ch || IS_NPC(ch) )
    return;

  argument_interpreter(arg, buff2, buff3);
  
  if( IS_TRUSTED(ch) && !str_cmp("reset", buff2) )
  {
    if (GET_LEVEL(ch) != OVERLORD)
    {
      send_to_char("Ask an overlord, this command will reset all active outposts.\n", ch);
      return;
    }
    
    // Option here to reset an outpost based on target in room.

    send_to_char("Resetting all outposts.\n", ch);
    reset_outposts(ch);

    return;
  }
  
  if( IS_TRUSTED(ch) && !str_cmp("reload", buff2) )
  {
    if( strlen(buff3) < 1 )
    {
      send_to_char("syntax: outpost reload <outpost id>\r\n", ch);
      return;
    }

    if( !isdigit(*buff3) )
    {  
      send_to_char("outpost id needs to be a number.\r\n", ch);
      return;
    }
    
    debug("outpost reload option not implemented yet.");
    //reload_outpost(ch, atoi(buff3));
    
    return;
  }
  
  if (!str_cmp("drop", buff2))
  {
    if (strlen(buff3) < 1)
    {
      send_to_char("syntax: outpost drop <outpost id>\r\n", ch);
      return;
    }

    if (!isdigit(*buff3))
    {
      send_to_char("The id must be a number.\r\n", ch);
      return;
    }

    if (!CAN_CONSTRUCT_CMD(ch) && !IS_TRUSTED(ch))
    {
      send_to_char("You need to be the leader of the guild to use this command.\r\n", ch);
      return;
    }
    int id = atoi(buff3);
    building = get_building_from_id(id);
    if (!building)
    {
      send_to_char("Invalid outpost ID.\r\n", ch);
      return;
    }

    if (get_outpost_owner(building) != GET_A_NUM(ch))
    {
      send_to_char("You don't own that outpost.\r\n", ch);
      return;
    }
    update_outpost_owner(0, building);
    reset_one_outpost(building);
    send_to_char("You relinquish control of that outpost.\r\n", ch);
    return;
  }

  // BEGIN REPAIR
  if (!str_cmp("repair", buff2))
  {
    int cost = (int)get_property("outpost.cost.repair", 0);
    if (!CAN_CONSTRUCT_CMD(ch) && !IS_TRUSTED(ch))
    {
      send_to_char("You need to be the leader of the guild to use this command.\r\n", ch);
      return;
    }
     
    // Find the rubble object in room
    for (rubble = world[ch->in_room].contents; rubble; rubble = rubble->next)
    {
      if (GET_OBJ_VNUM(rubble) == BUILDING_RUBBLE)
	break;
    }
    // Ok found rubble
    if (rubble && GET_OBJ_VNUM(rubble) == BUILDING_RUBBLE)
    {
      building = get_building_from_rubble(rubble);
      if(!building)
      {
         wizlog(56, "Failed to get building id from rubble.");
         raise(SIGSEGV);
      }
      op = building->mob;
      if (GET_A_NUM(ch))
      {
        send_to_char("You begin the arduous task of rebuilding the destroyed tower, claiming it for your guild!\r\n", ch);
	int ownerid = GET_A_NUM(ch);
	update_outpost_owner(ownerid, building);
      }
      if (!GET_A_NUM(ch))
      {
        send_to_char("You must belong to an association to claim an outpost.\r\n", ch);
        return;
      }
      if (get_scheduled(op, event_outpost_repair))
      {	
        send_to_char("The outpost is already being repaired.\r\n", ch);
        return;
      }
      // Ok begin repairs.
      SET_POS(ch, POS_STANDING + STAT_NORMAL);
      char_to_room(op, ch->in_room, -2);
      extract_obj(rubble, TRUE);
      act("$n begins repairs on the outpost.", TRUE, ch, 0, 0, TO_ROOM);
      add_event(event_outpost_repair, 1, op, NULL, NULL, 0, NULL, 0);
      return;
    }
    else
    {
      P_char tch;
      // No rubble around, is there an outpost already around to repair?
      for (tch = world[ch->in_room].people; tch != NULL; tch = tch->next_in_room)
      {
        if (affected_by_spell(tch, TAG_BUILDING))
	{
	  op = tch;
	  break;
	}
      }
      if(op)
      {
	building = get_building_from_char(op);

	if (!building)
	{
	  send_to_char("Can't find building associated with outpost, tell a god.\r\n", ch);
	  return;
	}
	if (GET_A_NUM(ch) != building->guild_id)
	{
	  send_to_char("You don't own this outpost!\r\n", ch);
	  return;
	}
	if (GET_HIT(op) >= building_types[BUILDING_OUTPOST-1].hitpoints)
	{
	  send_to_char("This outpost doesn't need any repairs.\r\n", ch);
	  return;
	}
        for (tch = world[op->in_room].people; tch != NULL; tch = tch->next_in_room)
        {
          if (GET_OPPONENT(tch) == op ||
	      IS_FIGHTING(op))
	  {
	    send_to_char("You cannot repair an outpost being attacked!\r\n", ch);
            return;
	  }
	}
        if (get_scheduled(op, event_outpost_repair))
        {
	  send_to_char("The outpost is already being repaired.\r\n", ch);
	  return;
        }

	send_to_char("You order repairs to begin upon the outpost.\r\n", ch);
        act("$n orders repairs begin on the outpost.", TRUE, ch, 0, 0, TO_ROOM);
	add_event(event_outpost_repair, PULSES_IN_TICK, op, NULL, NULL, 0, NULL, 0);
        return;
      }
      else
      {
        send_to_char("There is no rubble or outpost to repair here, go sac an outpost first!\r\n", ch);
        return;
      }
    }
  }
  // END REPAIR

  // BEGIN PORTAL
  if (!str_cmp("portal", buff2))
  {
    int cost = (int)get_property("outpost.cost.portal", 0);
    
    if (!CAN_CONSTRUCT_CMD(ch) && !IS_TRUSTED(ch))
    {
      send_to_char("You need to be the leader of the guild to use this command.\r\n", ch);
      return;
    }

    if (!(building = get_building_from_room(ch->in_room)))
    {
      send_to_char("You need to be inside your outpost to use this command.\r\n", ch);
      return;
    }
   
    if (!IS_TRUSTED(ch) && !sub_money_asc(building->guild_id, cost/1000, 0, 0, 0))
    {
      send_to_guild(building->guild_id, "The Guild Banker", "There are not enough guild funds to purchase a portal.");
      return;
    }

    if (outpost_generate_portals(building))
    {
      db_query("UPDATE outposts SET portal_room = '1' WHERE id = '%d'", building->id-1);
      send_to_char("Your outpost now contains portals.\r\n", ch);
    }
    else
      send_to_char("There was an issue building portals.\r\n", ch);
    return;
  }
  // END PORTAL
  
  // BEGIN GOLEM
  if (!str_cmp("golem", buff2))
  {
    int cost = (int)get_property("outpost.cost.golem", 0);
    
    if (!CAN_CONSTRUCT_CMD(ch) && !IS_TRUSTED(ch))
    {
      send_to_char("You need to be the leader of the guild to use this command.\r\n", ch);
      return;
    }
     
    if (!(building = get_building_from_room(ch->in_room)))
    {
      send_to_char("You need to be inside your outpost to use this command.\r\n", ch);
      return;
    }
    if (get_outpost_golems(building) >= MAX_OUTPOST_GATEGUARDS)
    {
      send_to_char("You can't have anymore outpost golems.\r\n", ch);
      return;
    }
    
    if (!IS_TRUSTED(ch) && !sub_money_asc(building->guild_id, cost/1000, 0, 0, 0))
    {
      send_to_guild(building->guild_id, "The Guild Banker", "There are not enough guild funds to purchase an outpost golem.");
      return;
    }

    outpost_load_gateguard(building->golem_room, OUTPOST_GATEGUARD_WAR, building, (get_outpost_golems(building)));
    db_query("UPDATE outposts SET golems = '%d' WHERE id = '%d'", (get_outpost_golems(building) + 1), building->id-1);
    send_to_char("You hire a new outpost gateguard.\r\n", ch);
    return;
  }
  // END GOLEM

  if (!str_cmp("?", buff2) || !str_cmp("help", buff2))
  {
    send_to_char("options available: repair, portal, golem, drop\r\n", ch);
    return;
  }

  if( IS_TRUSTED(ch) )
    //show_outposts_wiz(ch);
    show_outposts(ch);
  else
    show_outposts(ch);
    
}

void event_outpost_repair(P_char op, P_char vict, P_obj obj, void *data)
{
  Building *building = get_building_from_char(op);
  int cost = (int)get_property("outpost.cost.repair", 0);
  int div = (int)get_property("outpost.repair.div", 10);

  for (P_char tch = world[op->in_room].people; tch != NULL; tch = tch->next_in_room)
  {
    if (GET_OPPONENT(tch) == op ||
	IS_FIGHTING(op))
      return;
  }

  if (!sub_money_asc(building->guild_id, cost/div/1000, 0, 0, 0))
  {
    send_to_guild(building->guild_id, "The Guild Banker", "There are not enough guild funds to complete outpost repairs.");
    return;
  }

  //  Add 10% of the buildings max hitpoints, up to max.
  GET_HIT(building->mob) = BOUNDED(GET_HIT(building->mob), GET_HIT(building->mob) + (building_types[BUILDING_OUTPOST-1].hitpoints/10), building_types[BUILDING_OUTPOST-1].hitpoints);
  act("The repair process on $n continues.", TRUE, op, 0, 0, TO_ROOM);

  // If outpost is complete, no more events
  if (GET_HIT(building->mob) >= building_types[BUILDING_OUTPOST-1].hitpoints)
  {
    act("The repair process on $n is complete.", TRUE, op, 0, 0, TO_ROOM);
    return;
  }

  // a minute per repair, 10 minutes total.
  add_event(event_outpost_repair, PULSES_IN_TICK, op, NULL, NULL, 0, NULL, 0);
}

void outpost_death(P_char outpost, P_char killer)
{
  Building *building = get_building_from_char(outpost);
  int ownerid = 0;

  /* Not using outpost resources
  //handle materials from old outpost
  int wood = building_types[BUILDING_OUTPOST-1].req_wood * building->level;
  int stone = building_types[BUILDING_OUTPOST-1].req_stone * building->level;
  wood = (int)((float)wood * get_property("outpost.materials.death.convert.wood", 0.500));
  stone = (int)((float)stone * get_property("outpost.materials.death.convert.stone", 0.500));
  rubble->value[0] = wood;
  rubble->value[1] = stone;
  */
  P_obj rubble = read_object(97800, VIRTUAL);
  rubble->value[3] = building->id;
  obj_to_room(rubble, outpost->in_room);
  reset_one_outpost(building);
  GET_HIT(building->mob) = 0;
  set_current_outpost_hitpoints(building);
  update_outpost_owner(0, building);
  // Remove players from inside the outpost.
  for (int i = 0; i < building->rooms.size(); i++)
  {
    P_char next_tch;
    for (P_char tch = world[building->gate_room()].people; tch != NULL; tch = next_tch)
    {
      next_tch = tch->next_in_room;
      if (IS_PC(tch))
        send_to_char("The outpost has been destroyed, you fall to the ground outside!\r\n", tch);
      char_from_room(tch);
      char_to_room(tch, building->location(), -1);
    }
  }
  for (int i = 0; i < building->rooms.size(); i++)
  {
    P_obj next_obj;
    for(P_obj tobj = world[building->gate_room()].contents; tobj != NULL; tobj = next_obj)
    {
      next_obj = tobj->next_content;
      obj_from_room(tobj);
      obj_to_room(tobj, building->location());
    }
  }
  // Remove the outpost from the room.
  char_from_room(outpost);
}

int get_killing_association(P_char ch)
{
  struct group_list *gl;
  int guild[MAX_ASC];
  int killer = 0, curmax = 0;
    
  for (int i = 0; i < MAX_ASC; i++)
  {
    guild[i] = 0;
  }
  
  // are we grouped?
  if (!ch->group)
  {
    if (!GET_A_NUM(ch))
      return 0;
    else
    {
      killer = GET_A_NUM(ch);
      return killer;
    }
  }
  else
  {
    //grouped, spool the number of guildies in groups for each association.
    for (gl = ch->group; gl; gl = gl->next)
    {
      if(gl->ch && GET_A_NUM(gl->ch))
	guild[GET_A_NUM(gl->ch)]++;
    }
  }
  
  // Lets find out who has the most guild members in group
  curmax = guild[0];
  killer = 0;
  for (int i = 0; i < MAX_ASC; i++)
  {
    // If there is a tie, check prestige.
    if (guild[i] == 0)
    {
      continue;
    }
    else if (guild[i] == curmax)
    {
      if (get_assoc_prestige(i) > get_assoc_prestige(killer))
      {
	killer = i;
      }
    }
    else if(guild[i] > curmax)
    {
      killer = i;
      curmax = guild[i];
    }
  }
  
  return killer;
}

// Add resources to a player's guild's current resource pool
void outpost_update_resources(P_char ch, int wood, int stone)
{
  if (!qry("SELECT id, wood, stone FROM associations WHERE id = %d", GET_A_NUM(ch)))
  {
    debug("outpost_update_resources() cant read from db");
    return;
  }
 
  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);

  int cur_wood = atoi(row[1]);
  int cur_stone = atoi(row[2]);

  mysql_free_result(res);
  
  db_query("UPDATE associations SET wood='%d', stone='%d' WHERE id='%d'", (int)(wood+cur_wood), (int)(stone+cur_stone), GET_A_NUM(ch));
}

void update_outpost_owner(int owner, Building *building)
{
  P_char op;
  int id;
  struct affected_type *afp;

  if (!building->id)
  {
    debug("error calling update_outpost_owner, no building ID available");
    return;
  }

  // Update the DB	
  id = building->id-1;
  db_query("UPDATE outposts SET owner_id = '%d' WHERE id = '%d'", owner, id);
  building->guild_id = owner;
  return;
}

void update_outpost_golems(Building *building, int amount)
{
  if (!building->id)
  {
    debug("error calling update_outpost_golems, no building ID available");
    return;
  }

  db_query("UPDATE outposts SET golems = '%d' WHERE id = '%d'", BOUNDED(0, (get_outpost_golems(building) + amount), MAX_OUTPOST_GATEGUARDS), building->id-1);
  return;
}

void reset_one_outpost(Building *building)
{
  P_char op;
  int id;

  if (!building->id)
  {
    debug("error calling reset_one_outpost, no building ID available");
    return;
  }
  id = building->id-1;

  db_query("UPDATE outposts SET owner_id = '0', level = '8', walls = '1', archers = '0', hitpoints = '%d', portal_room = '0' WHERE id = '%d'", building_types[BUILDING_OUTPOST-1].hitpoints, id);

  GET_MAX_HIT(building->mob) = building->mob->points.base_hit = GET_HIT(building->mob) = building_types[BUILDING_OUTPOST-1].hitpoints;

  SET_POS(building->mob, POS_STANDING + STAT_NORMAL);

  //remove portals
  obj_from_room(building->portal_op);
  building->portal_op = NULL;
  obj_from_room(building->portal_gh);
  building->portal_gh = NULL;
}

void reset_outposts(P_char ch)
{
  P_char op;
  Building *building = NULL;

  for (op = character_list; op; op = op->next)
  {
    
    if (!affected_by_spell(op, TAG_BUILDING))
      continue;

    building = get_building_from_char(op);

    debug("resetting outpost #: %d", building->id-1);
    reset_one_outpost(building);
  }
}

int outpost_rubble(P_obj obj, P_char ch, int cmd, char *arg)
{
  char buff[MAX_STRING_LENGTH], buff2[MAX_STRING_LENGTH];
  P_obj t_obj;
  int wood = 0;
  int stone = 0;

  if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
    return FALSE;

  if (!obj)
    return FALSE;

  /* removing resources content from outposts
  if (cmd == CMD_GET || cmd == CMD_TAKE)
  {
    arg = one_argument(arg, buff);
    if (!*buff)
      return FALSE;
    
    t_obj = get_obj_in_list_vis(ch, buff, world[ch->in_room].contents);
    
    if (t_obj = obj)
    {
      if (!GET_A_NUM(ch))
      {
        send_to_char("You need to be guilded to make use of outpost resources!\r\n", ch);
        return TRUE;
      }

      wood = obj->value[0];
      stone = obj->value[1];
      outpost_update_resources(ch, wood, stone);

      sprintf(buff2, "You receive %d wood and %d stone in outpost resources.\r\n", wood, stone);
      send_to_char(buff2, ch);
      
      extract_obj(obj, TRUE);

      return TRUE;
    }
  }
  */

  return FALSE;
}

void outpost_create_wall(int location, int direction, int type)
{
  // type here more determines if we have stronger or weaker walls.. ie setting
  // the power of the wall.  hit wall does 1 damage to WALL_OUTPOST.
  int value;

  // set this later based upon type
  value = 300;

  if (create_walls(location, direction, NULL, 64, WALL_OUTPOST, value, -1,
       "&+Wan outpost wall&n",
       "&+WAn outpost wall is here to the %s.&n", 0))
  {
    SET_BIT(world[location].dir_option[direction]->exit_info, EX_BREAKABLE);
    SET_BIT(VIRTUAL_EXIT
        ((world[location].dir_option[direction])->to_room,
	  rev_dir[direction])->exit_info, EX_BREAKABLE);
  }
}

  /* 
   * currently two options are available for outposts walls.
   * Setting a room to SECT_OUTPOST_WALL will create unbreakable walls
   * Or we can load walls of type outpost wall which we can incorporate
   * to let them break them down like wall of stone, but harder.
   */
int outpost_generate_walls(Building* building, int type, int numgolem)
{
  int location = 0;
  int walllocal[10];
  int x;
  int gate = building->golem_dir;

  if (building->mob->in_room)
    location = building->mob->in_room;

  if (!location)
  {
    debug("outpost_generate_walls() can't find location");
    return FALSE;
  }

  // assign rooms
  walllocal[NORTH] = world[location].dir_option[NORTH]->to_room;
  walllocal[EAST] = world[location].dir_option[EAST]->to_room;
  walllocal[SOUTH] = world[location].dir_option[SOUTH]->to_room;
  walllocal[WEST] = world[location].dir_option[WEST]->to_room;
  walllocal[NORTHWEST] = world[walllocal[NORTH]].dir_option[WEST]->to_room;
  walllocal[NORTHEAST] = world[walllocal[NORTH]].dir_option[EAST]->to_room;
  walllocal[SOUTHEAST] = world[walllocal[SOUTH]].dir_option[WEST]->to_room;
  walllocal[SOUTHWEST] = world[walllocal[SOUTH]].dir_option[EAST]->to_room;
  walllocal[UP] = 0;
  walllocal[DOWN] = 0;

  for (x = 0; x < NUM_EXITS; x++)
  {
    if (x == gate)
    {
      //outpost_setup_gateguards(world[walllocal[x]].dir_option[gate]->to_room, OUTPOST_GATEGUARD_WAR, 2, building->guild_id);
      outpost_setup_gateguards(walllocal[x], OUTPOST_GATEGUARD_WAR, numgolem, building);
      world[walllocal[x]].sector_type = SECT_CASTLE_GATE;
      building->golem_room = walllocal[x];
      continue;
    }
    
    switch(x)
    {
    case NORTH:
    case EAST:
    case SOUTH:
    case WEST:
      world[walllocal[x]].sector_type = SECT_CASTLE_WALL;
      //outpost_create_wall(walllocal[x], x, type);
      break;
    case NORTHWEST:
      world[walllocal[x]].sector_type = SECT_CASTLE_WALL;
      //outpost_create_wall(walllocal[x], WEST, type);
      //outpost_create_wall(walllocal[x], NORTH, type);
      break;
    case NORTHEAST:
      world[walllocal[x]].sector_type = SECT_CASTLE_WALL;
      //outpost_create_wall(walllocal[x], NORTH, type);
      //outpost_create_wall(walllocal[x], EAST, type);
      break;
    case SOUTHEAST:
      world[walllocal[x]].sector_type = SECT_CASTLE_WALL;
      //outpost_create_wall(walllocal[x], WEST, type);
      //outpost_create_wall(walllocal[x], SOUTH, type);
      break;
    case SOUTHWEST:
      world[walllocal[x]].sector_type = SECT_CASTLE_WALL;
      //outpost_create_wall(walllocal[x], SOUTH, type);
      //outpost_create_wall(walllocal[x], EAST, type);
      break;
    default:
      break;
    }
  
    world[building->mob->in_room].sector_type = SECT_CASTLE;
  }
}

int outpost_load_gateguard(int location, int type, Building *building, int golemnum)
{
  building->golems[golemnum] = read_mobile(type, VIRTUAL);

  mob_index[real_mobile0(type)].func.mob = outpost_gateguard_proc;

  if (!building->golems[golemnum])
  {
    debug("outpost_load_gateguards() error reading mobile");
    return FALSE;
  }

  add_tag_to_char(building->golems[golemnum], TAG_DIRECTION, rev_dir[building->golem_dir], AFFTYPE_PERM);
  add_tag_to_char(building->golems[golemnum], TAG_GUILDHALL, building->id, AFFTYPE_PERM);
  GET_A_NUM(building->golems[golemnum]) = building->guild_id;
  building->golems[golemnum]->player.birthplace = building->golems[golemnum]->player.orig_birthplace = building->golems[golemnum]->player.hometown = world[location].number;
  char_to_room(building->golems[golemnum], location, -1);

  return TRUE;
}

void outpost_setup_gateguards(int location, int type, int amnt, Building *building)
{
  int i;
  int guild = building->guild_id;
  for (i = 0; i < amnt; i++)
  {
    outpost_load_gateguard(location, type, building, i);
  }
}

int outpost_gateguard_proc(P_char ch, P_char pl, int cmd, char *arg)
{
  char buff[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  SET_BIT(ch->specials.act, ACT_SPEC_DIE);
  
  // make sure golem stays in its home room
  if( ch->player.birthplace && ch->in_room != real_room(ch->player.birthplace) )
  {
    act("$n pops out of existence.&n", FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(ch->player.birthplace), -1);
    act("$n pops into existence.&n", FALSE, ch, 0, 0, TO_ROOM);
  }
  
  // Make sure the gate guards update with the new owner
  Building *building = get_building_from_gateguard(ch);
  if (!building)
    return FALSE;
  if (get_outpost_owner(building) != GET_A_NUM(ch))
  {
    GET_A_NUM(ch) = get_outpost_owner(building);
    act("$n blinks momentarily then accepts their new owners.", TRUE, ch, 0, 0, TO_ROOM);
  }
  
  SET_MEMBER(GET_A_BITS(ch));
  SET_NORMAL(GET_A_BITS(ch));
  
  int blocked_dir = direction_tag(ch);
  
  if( blocked_dir < NORTH || blocked_dir >= NUM_EXITS )
  {
    logit(LOG_GUILDHALLS, "guildhall_golem() assigned to %s in %d has an invalid blocking direction (%d)!", GET_NAME(ch), world[ch->in_room].number, blocked_dir);
    REMOVE_BIT(ch->specials.act, ACT_SPEC);
    return FALSE;
  }
  
  if( cmd == CMD_PERIODIC )
  {
    // TODO: add buffs based on online guild member count, etc
    return FALSE;
  }
  
  //
  // cmds
  //
  
  if( cmd == CMD_DEATH )
  {
    update_outpost_golems(building, -1);
    return FALSE;
  }
  
  if( pl && cmd == cmd_from_dir(blocked_dir) )
  {
    // char tried to go in the blocked direction

    P_char   t_ch = pl;
    
    if (IS_PC_PET(pl))
      t_ch = pl->following;
    
    if(!t_ch)
      return FALSE;
    
    bool allowed = FALSE;
    struct alliance_data *alliance = get_alliance(GET_A_NUM(ch));
    
    if ((GET_A_NUM(pl) == GET_A_NUM(ch)) ||
	(!GET_A_NUM(ch)))
      allowed = TRUE;
    else if ((alliance = get_alliance(GET_A_NUM(ch))) &&
	      ((GET_A_NUM(pl) == alliance->forging_assoc_id) ||
	       (GET_A_NUM(pl) == alliance->joining_assoc_id)))
      allowed = TRUE;
    if (!allowed && pl->group)
    {
      struct group_list *gl;
      gl = pl->group;
      while (gl)
      {
	if (GET_A_NUM(gl->ch) == GET_A_NUM(ch))
	  allowed = TRUE;
        gl = gl->next;
      }
    }
    
    if( IS_TRUSTED(pl) )
    {
      // don't show anything when immortals enter the GH
      return FALSE;
    }
    else if(allowed)
    {
      if(!number(0, 2))
      {
         act("$N stands impassively as you pass by.", FALSE, pl, 0, ch, TO_CHAR);
         act("$N stands impassively as $n passes by.", FALSE, pl, 0, ch, TO_NOTVICTROOM);
      }
      return FALSE;
    }
    else
    {
      act("$N glares at you and refuses to let you pass.", FALSE, pl, 0, ch, TO_CHAR);
      act("$N glares at $n and refuses to let them pass.", FALSE, pl, 0, ch, TO_NOTVICTROOM);      
      return TRUE;
    }

    return FALSE;
  }
  
  if (pl && (cmd == CMD_GOTHIT && !number(0, 15)) ||
      (cmd == CMD_HIT || cmd == CMD_KILL))
  {
    //can add check here to see if guild has magic mouth upgrade from db?
    sprintf(buff,
	"&+cA magic mouth tells your guild 'Alert! $N&n&+c has trespassed into %s&n&+c!'&n",
	world[ch->in_room].name);
    for (P_desc i = descriptor_list; i; i = i->next)
      if (!i->connected &&
	  !is_silent(i->character, TRUE) &&
	  IS_SET(i->character->specials.act, PLR_GCC) &&
	  IS_MEMBER(GET_A_BITS(i->character)) &&
	  (GET_A_NUM(i->character) == GET_A_NUM(ch)) &&
	  !IS_TRUSTED(i->character))
	act(buff, FALSE, i->character, 0, pl, TO_CHAR);
    return FALSE;
  }
  
  return FALSE;
  
  return FALSE;
}

int outpost_generate_portals(Building *building)
{
  char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH], ghname[MAX_STRING_LENGTH];
  FILE *f;
  int guildhall_room = 0;

  if (!building)
  {
    debug("error in outpost_generate_portals(): called without valid building.");
    return 0;
  }

  if (building->portal_op || building->portal_gh)
  {
    debug("portal already exists");
    return 0;
  }

  if (building->guild_id <= 0)
  {
    debug("building isn't owned by a guild.");
    return 0;
  }
  else
  {
    debug("building->guild_id: %d", building->guild_id);
    if (Guildhall *gh = Guildhall::find_by_id(building->guild_id))
    {
      if (gh->heartstone_room)
        guildhall_room = gh->heartstone_room->vnum;
      else
      {
        debug("can't find guildhall hearthstone_room");
        return 0;
      }
    }
    else
    {
      debug("You don't own a guildhall.");
      return 0;
    }
    
    sprintf(Gbuf1, "%sasc.%u", ASC_DIR, (ush_int)building->guild_id);
    f = fopen(Gbuf1, "r");
    if (!f)
    {
      sprintf(ghname, "Unknown");
    }
    else
    {
      fgets(Gbuf2, MAX_STR_NORMAL, f);
      Gbuf2[strlen(Gbuf2)-1] = 0;
      Gbuf2[MAX_STR_ASC-1] = 0;
      strcpy(ghname, Gbuf2);
      fclose(f);
    }
  }
 
  if (building->portal_op = read_object(BUILDING_PORTAL, VIRTUAL))
  {
    sprintf(buff, building->portal_op->description, ghname);
    building->portal_op->value[0] = guildhall_room;
    building->portal_op->description = str_dup(buff);
    building->portal_op->str_mask = STRUNG_DESC1;
    obj_to_room(building->portal_op, building->gate_room());
  }
  
  if (building->portal_gh = read_object(BUILDING_PORTAL, VIRTUAL))
  {
    sprintf(buff, building->portal_gh->description, continent_name(world[building->location()].continent));
    building->portal_gh->value[0] = building->rooms[0]->number;
    building->portal_gh->description = str_dup(buff);
    building->portal_gh->str_mask = STRUNG_DESC1;
    obj_to_room(building->portal_gh, real_room0(guildhall_room));
  }

  return 1;
}

int check_castle_walls(int from, int to)
{
  if ((world[to].sector_type == SECT_CASTLE_WALL &&
       world[from].sector_type != SECT_CASTLE_WALL &&
       world[from].sector_type != SECT_CASTLE) ||
      (world[from].sector_type == SECT_CASTLE_WALL && 
       (world[to].sector_type != SECT_CASTLE &&
	world[to].sector_type != SECT_CASTLE_WALL)))
    return TRUE;
  else
    return FALSE;
}

void outposts_upkeep()
{
  char buff[MAX_STRING_LENGTH];
  int i, j, k;
  Building *building;
  int cost = (int)get_property("outpost.cost.upkeep", 1000000); // per day
  int deduct = 0;
  int owners[MAX_ASC];

  if (time(0) - upkeep_time < (60*60))
    return;
  else
    upkeep_time = time(0);

  for (j = 0; j < MAX_ASC; j++)
    owners[j] = 0;

  for (i = 0; i <= buildings.size(); i++)
  {
    building = get_building_from_id(i+1);
    if (!building)
      continue;
    owners[building->guild_id]++;
  }
  
  for (j = 1; j < MAX_ASC; j++)
  {
    if (owners[j])
    {
      deduct = cost;
      if (owners[j] > 1)
        for (k = 1; k < owners[j]; k++)
          deduct *= get_property("outpost.cost.upkeep.multi.modifier", 5);
      deduct /= 24; // per hour cost
      debug("owned: %d, owner: %d, deduct: %s", owners[j], j, coin_stringv(deduct));
      int p = deduct / 1000;
      deduct = deduct % 1000;
      int g = deduct / 100;
      deduct = deduct % 100;
      int s = deduct / 10;
      deduct = deduct % 10;
      int c = deduct;
      debug("p: %d, g: %d, s: %d, c: %d", p, g, s, c);
      if (!sub_money_asc(j, p, g, s, c))
      {
	send_to_guild(j, "The Guild Banker", "There are not enough funds for the outpost upkeep.");
        // drop outposts.
        for (i = 0; i <= buildings.size(); i++)
	{
	  building = get_building_from_id(i+1);
	  if (!building)
	    continue;
	  if (building->guild_id == j)
	  {
	    // give them an hour after boot to get things in order before dropping
	    if (real_time_passed(time(0), boot_time).minute < 60)
	      continue;
	    sprintf(buff, "Dropping %s&+C outpost.", continent_name(world[building->location()].continent));
	    send_to_guild(j, "The Guild Banker", buff);
	    update_outpost_owner(0, building);
	    reset_one_outpost(building);
	  }
	}
      }
    }
  }
}

#endif

