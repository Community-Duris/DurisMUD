/************************************************************
newship.c

New Ship system blah blah blah bug foo about it :P
Updated with warships. Nov08 -Lucrot 
*************************************************************/

#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <math.h>
    
#include "ships.h"
#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "ship_ai.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include "epic.h"
#include "nexus_stones.h"
#include "limits.h"

int read_newship();
int write_newship(P_ship ship);

void nameship(char *name, P_ship ship);
int loadship(P_ship shipdata, int to_room);

struct ShipData *newship(int m_class);
void delete_ship(P_ship ship);
    
// shops
int newship_shop(int room, P_char ch, int cmd, char *arg);
int crew_shop(int room, P_char ch, int cmd, char *arg);
int erzul(P_char ch, P_char pl, int cmd, char *arg);

// proc
int newshiproom_proc(int room, P_char ch, int cmd, char *arg);
int fire_arc(P_ship ship, P_char ch, int arc);
int fire_weapon(P_ship ship, P_char ch, int w_num);
void force_anchor(P_ship ship);
int newship_proc(P_obj obj, P_char ch, int cmd, char *arg);
int shiploader_proc(P_obj obj, P_char ch, int cmd, char *arg);
int shipobj_proc(P_obj obj, P_char ch, int cmd, char *arg);

int  bearing(float x1, float y1, float x2, float y2);
int anchor_room(int room);
void newshipfrags();
void crash_land(P_ship ship);

void setarmor(P_ship ship, bool equal);
void setcrew(P_ship ship, int crew_index, int exp);
void clear_ship_layout(P_ship ship);
void set_ship_layout(P_ship ship, int m_class);
void reset_ship_physical_layout(P_ship ship);
void dock_ship(P_ship ship, int to_room);
void check_contraband(P_ship ship, int to_room);
void update_maxspeed(P_ship ship);


extern void shipai_activity(P_ship ship);
extern void act_to_all_in_ship(P_ship ship, const char *msg);
extern void act_to_outside_ships(P_ship ship, const char *msg, P_ship notarget);
extern void act_to_outside(P_ship ship, const char *msg);
extern void everyone_get_out_newship(P_ship ship);
extern void everyone_look_out_newship(P_ship ship);
extern int  armorcondition(int maxhp, int curhp);
extern void assignid(P_ship ship, char *id);
extern int  assign_shipai(P_ship ship);
extern int damage_sail(P_ship ship, P_ship target, int dam);
extern int  damage_hull(P_ship ship, P_ship target, int dam, int arc, int armor_pierce);
extern void dispcontact(int i);
//extern int  getarc(P_ship ship1, int x, int y);
extern int  getarc(int heading, int bearing);
extern int  ybearing(int bearing, int range);
extern int  xbearing(int bearing, int range);
extern int getcontacts(P_ship ship);
extern int getmap(P_ship ship);
extern P_ship getshipfromchar(P_char ch);
extern int num_people_in_ship(P_ship ship);
extern int try_ram_ship(P_ship ship, P_ship target, int contact_j);
extern int pilotroll(P_ship ship);
extern float range(float x1, float y1, float z1, float x2, float y2, float z2);
extern void scantarget(P_ship target, P_char ch);
extern void stun_all_in_ship(P_ship ship, int timer);
extern void summon_ship_event(P_char ch, P_char victim, P_obj obj, void *data);
extern int weaprange(int w_index, char range);
extern int weaponsight(P_char ch, P_ship ship, P_ship target, int weapon, float mod);
extern void calc_crew_adjustments(P_ship ship);


int sell_cargo(P_char ch, P_ship ship, int slot);
int sell_cargo_slot(P_char ch, P_ship ship, int slot, int rroom);
int sell_contra(P_char ch, P_ship ship, int slot);
int sell_contra_slot(P_char ch, P_ship ship, int slot, int rroom);


extern int read_cargo();
extern int write_cargo();

static char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

struct ContactData contacts[MAXSHIPS];
struct ShipMap tactical_map[101][101];
char     status[20];
char     position[20];
char     contact[256];

static char arg1[MAX_STRING_LENGTH];
static char arg2[MAX_STRING_LENGTH];
static char arg3[MAX_STRING_LENGTH];
static char tmp_str[MAX_STRING_LENGTH];
int    shiperror;
struct ShipFragData shipfrags[10];
struct CargoStats shipcargo[MAXCARGO];
struct CargoStats shipcontra[MAXCARGO];














//--------------------------------------------------------------------
// load all ships from file into the world
//--------------------------------------------------------------------
void initialize_newships()
{
    int      i, j;

    if (!read_newship())
    {
        logit(LOG_FILE, "Error reading ships from file!\r\n");
    }

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; )
    {
        P_ship ship = svs;
        if (IS_SET(ship->flags, NEWSHIP_DELETE)) 
        {
            fn = shipObjHash.erase(svs);
            delete_ship(ship);
        } 
        else
            fn = shipObjHash.get_next(svs);
    }


    for (i = 0; i < MAXCARGO; i++) 
    {
        for (j = 0; j < MAXCARGO; j++) 
        {
            shipcargo[i].buy[j] =    50;
            shipcargo[i].sell[j] =  100;
            shipcontra[i].buy[j] =   50;
            shipcontra[i].sell[j] = 100;
        }
    }
    if (!read_cargo()) {
        logit(LOG_FILE, "Error reading cargo file!\r\n");
    }
}

//--------------------------------------------------------------------
// pre shutdown operations: dock all ships and put sailors to land 
//--------------------------------------------------------------------
void shutdown_newships()
{
    int      i;
    P_char   ch, ch_next;
    P_obj    obj, obj_next;

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        P_ship ship = svs;
        for (i = 0; i < MAX_SHIP_ROOM; i++)
        {
            for (ch = world[real_room(ship->room[i].roomnum)].people; ch; ch = ch_next)
            {
                if (ch)
                {
                    ch_next = ch->next_in_room;
                    char_from_room(ch);
                    char_to_room(ch, real_room0(ship->anchor), -2);
                }
            }
            for (obj = world[real_room(ship->room[i].roomnum)].contents; obj; obj = obj_next)
            {
                if (obj)
                {
                    obj_next = obj->next_content;
                    obj_from_room(obj);
                    obj_to_room(obj, real_room0(ship->anchor));
                }
            }
        }
        write_newship(ship);
    }
}

//--------------------------------------------------------------------
// set ship object names with ship name in them
//--------------------------------------------------------------------
void nameship(char *name, P_ship ship)
{
   int rroom, i;
   
   if (ship->name != NULL)
      ship->name = NULL;

   if (ship->shipobj->description != NULL)
      ship->shipobj->description = NULL;

   if (ship->shipobj->short_description != NULL)
      ship->shipobj->short_description = NULL;

   if (ship->shipobj->name != NULL)
      ship->shipobj->name = NULL;

   if (ship->keywords != NULL)
      ship->keywords = NULL;

   ship->name = str_dup(name);
   sprintf(buf2, "&+yThe %s&N %s&n &+yfloats here.", SHIPCLASSNAME(ship), name);
   ship->shipobj->description = str_dup(buf2);
   sprintf(buf2, "&+yThe %s&N %s&n", SHIPCLASSNAME(ship), name);
   ship->shipobj->short_description = str_dup(buf2);
   sprintf(buf2, "ship %s %s", SHIPCLASSNAME(ship), ship->ownername);
   ship->shipobj->name = str_dup(buf2);
   ship->keywords = str_dup(buf2);

   // name ship rooms
   for (i = 0; i < MAX_SHIP_ROOM; i++)
   {
      if (SHIPROOMNUM(ship, i) != -1)
      {
         rroom = real_room0(SHIPROOMNUM(ship, i));
         if (!rroom)
         {
            shiperror = 4;
            return;
         }

         if (ship->m_class == SH_FRIGATE || ship->m_class == SH_DREADNOUGHT) 
         {
            if (ship->bridge == world[rroom].number) {
               sprintf(buf, "&+ROn the &+WBridge&N&+R of the &+L%s&N %s",
                     SHIPCLASSNAME(ship), ship->name);
            } else if (world[rroom].number == ship->bridge + 9) {
               sprintf(buf, "&+YDocking Bay&+y of the &+L%s&N %s",
                     SHIPCLASSNAME(ship), ship->name);
            } else if (world[rroom].number == ship->bridge + 7) {
               sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s",
                     SHIPCLASSNAME(ship), ship->name);
            } else {
               sprintf(buf, "&+yAboard the &+L%s&N %s", SHIPCLASSNAME(ship),
                     ship->name);
            }
         } else if (ship->bridge == world[rroom].number) {
            sprintf(buf, "&+yOn the &+WBridge&N&+y of the %s&N %s",
                  SHIPCLASSNAME(ship), ship->name);
         } else {
            sprintf(buf, "&+yAboard the %s&N %s", SHIPCLASSNAME(ship),
                  ship->name);
         }

         if (world[rroom].name) {
//        str_free(world[rroom].name);
            world[rroom].name = NULL;
         }

         world[rroom].name = str_dup(buf);
      }
   }
}

//--------------------------------------------------------------------
// load ship into the world
//--------------------------------------------------------------------
int loadship(P_ship ship, int to_room)
{
   int      i, rroom, dir;

   if (!IS_SET(ship->flags, LOADED)) 
   {
      shiperror = 1;
      return FALSE;
   }

   if (ship->shipobj == NULL) 
   {
      shiperror = 2;
      return FALSE;
   }

   if (ship->panel == NULL) 
   {
      shiperror = 3;
      return FALSE;
   }

   for (i = 0; i < MAX_SHIP_ROOM; i++) 
   {
      if (SHIPROOMNUM(ship, i) != -1) 
      {
         rroom = real_room0(SHIPROOMNUM(ship, i));
         if (!rroom) 
         {
            shiperror = 4;
            return FALSE;
         }

         world[rroom].funct = newshiproom_proc;
         for (dir = 0; dir < NUMB_EXITS; dir++) 
         {
            if (SHIPROOMEXIT(ship, i, dir) != -1) 
            {
               if (!world[rroom].dir_option[dir])
                  CREATE(world[rroom].dir_option[dir], room_direction_data, 1, MEM_TAG_DIRDATA);

               world[rroom].dir_option[dir]->to_room =
               real_room0(SHIPROOMEXIT(ship, i, dir));
               world[rroom].dir_option[dir]->exit_info = 0;
            } 
            else 
            {
               if (world[rroom].dir_option[dir]) 
                  world[rroom].dir_option[dir]->to_room = -1;
            }
         }
      }
   }
   
   ship->target = NULL;
   REMOVE_BIT(ship->flags, SINKING);
   REMOVE_BIT(ship->flags, RAMMING);
   SET_BIT(ship->flags, DOCKED);
   SET_BIT(ship->flags, LOADED);
   
   if (ship->panel != NULL)
      obj_to_room(ship->panel, real_room0(ship->bridge));
   else 
   {
      shiperror = 23;
      return FALSE;
   }
   if (SHIPOBJ(ship)) 
   {
      ship->z = 0;
      SHIPOBJ(ship)->value[6] = 1;
      obj_to_room(SHIPOBJ(ship), to_room);
   } 
   else 
   {
      shiperror = 5;
      return FALSE;
   }
   ship->anchor = world[to_room].number;
   ship->location = to_room;

   ship->repair = SHIPHULLWEIGHT(ship);
   update_crew(ship);
   reset_crew_stamina(ship);
   update_ship_status(ship);
   return TRUE;
}

//--------------------------------------------------------------------
// create new ship of given class
//--------------------------------------------------------------------
struct ShipData *newship(int m_class)
{
   int j = 0;
   ShipVisitor svs;
   for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs)) j++;
   if (j >= MAXSHIPS) 
       return NULL;   // AT MAX FOR GAME?

   // Make a new ship
   P_ship ship = NULL;
   CREATE(ship, ShipData, 1, MEM_TAG_SHIPDAT);
   
   ship->shipobj = read_object(60001, VIRTUAL);
   shipObjHash.add(ship);

   // Set up the new ship
   ship->shipai = 0;
   ship->panel = read_object(60000, VIRTUAL);
   ship->m_class = m_class;
   setarmor(ship, true);
   ship->frags = 0;
   ship->name = NULL;
   ship->x = 50;
   ship->y = 50;
   ship->z = 0;
   ship->flags = 0;
   ship->money = 0;
   ship->mainsail = SHIPTYPEMAXSAIL(m_class);

   ship->maxspeed_bonus = 0;
   ship->capacity_bonus = 0;

   SET_BIT(ship->flags, LOADED);

   ship->time = time(NULL);
   int room = (SHIPZONE * 100);
   while (world[real_room0(room)].funct == newshiproom_proc) 
   {
      room += 10;
   }
   int bridge = room;
   ship->bridge = bridge;
   ship->entrance = bridge;
   if (ship->panel == NULL) 
   {
      shiperror = 17;
      return NULL;
   }
   for (j = 0; j <= NUMB_EXITS; j++) 
   {
      ship->room[0].exit[j] = -1;
   }
   if (ship->panel == NULL) {
      shiperror = 16;
      return NULL;
   }

   for (j = 0; j < MAXSLOTS; j++) 
   {
        ship->slot[j].clear();
   }
   clear_ship_layout(ship);

   ship->room[0].roomnum = bridge;

   setcrew(ship, sail_crew_list[0], 0);
   setcrew(ship, gun_crew_list[0], 0);
   setcrew(ship, repair_crew_list[0], 0);
   setcrew(ship, rowing_crew_list[0], 0);

   assignid(ship, "**");
   SHIPKEYWORDS(ship) = str_dup("ship");

   if (ship->panel == NULL) {
      shiperror = 13;
      return NULL;
   }
   set_ship_layout(ship, m_class);
   return ship;
}

//--------------------------------------------------------------------
void delete_ship(P_ship ship)
{
    char fname[256];

    for (int i = 0; i < MAX_SHIP_ROOM; i++)
        world[real_room0(SHIPROOMNUM(ship, i))].funct = NULL;

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (svs->target == ship)
            svs->target = NULL;
    }

    write_newship(NULL);
    sprintf(fname, "Ships/%s", ship->ownername);
    unlink(fname);

    obj_from_room(ship->panel);
    obj_from_room(ship->shipobj);
    extract_obj(ship->panel, TRUE);
    extract_obj(ship->shipobj, TRUE);
    ship->panel = NULL;
    ship->shipobj = NULL;

    logit(LOG_STATUS, "Ship \"%s\" (%s) deleted", strip_ansi(ship->name).c_str(), ship->ownername);
    
    FREE(ship);
}

void clear_ship_layout(P_ship ship)
{
   for (int j = 0; j < MAX_SHIP_ROOM; j++) 
   {
      for (int k = 0; k < NUMB_EXITS; k++) 
      {
         SHIPROOMEXIT(ship, j, k) = -1;
      }
      SHIPROOMNUM(ship, j) = ship->bridge + j;
   }
}


void set_ship_layout(P_ship ship, int m_class)
{
    int room;

    room = ship->bridge;
    switch (m_class) {
    case SH_SLOOP:
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 1;
        SHIPROOMEXIT(ship, 1, NORTH) = room;
        ship->entrance = room + 1;
        break;

    case SH_YACHT:
        SHIPROOMEXIT(ship, 0, NORTH) = room + 1;
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 2;
        SHIPROOMEXIT(ship, 1, SOUTH) = room;
        SHIPROOMEXIT(ship, 2, NORTH) = room;
        ship->entrance = room + 2;
        break;

    case SH_CLIPPER:
        SHIPROOMEXIT(ship, 0, NORTH) = room + 1;
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 2;
        SHIPROOMEXIT(ship, 1, SOUTH) = room;
        SHIPROOMEXIT(ship, 2, NORTH) = room;
        SHIPROOMEXIT(ship, 2, SOUTH) = room + 3;
        SHIPROOMEXIT(ship, 3, NORTH) = room + 2;
        ship->entrance = room + 3;
        break;

    case SH_KETCH:
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 2;
        SHIPROOMEXIT(ship, 1, EAST) = room + 2;
        SHIPROOMEXIT(ship, 2, NORTH) = room;
        SHIPROOMEXIT(ship, 2, EAST) = room + 3;
        SHIPROOMEXIT(ship, 2, WEST) = room + 1;
        SHIPROOMEXIT(ship, 3, WEST) = room + 2;
        ship->entrance = room + 2;
        break;

    case SH_CARAVEL:
        SHIPROOMEXIT(ship, 0, NORTH) = room + 1;
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 5;
        SHIPROOMEXIT(ship, 0, EAST) = room + 3;
        SHIPROOMEXIT(ship, 0, WEST) = room + 2;
        SHIPROOMEXIT(ship, 1, SOUTH) = room;
        SHIPROOMEXIT(ship, 2, SOUTH) = room + 4;
        SHIPROOMEXIT(ship, 2, EAST) = room;
        SHIPROOMEXIT(ship, 3, SOUTH) = room + 6;
        SHIPROOMEXIT(ship, 3, WEST) = room;
        SHIPROOMEXIT(ship, 4, NORTH) = room + 2;
        SHIPROOMEXIT(ship, 4, EAST) = room + 5;
        SHIPROOMEXIT(ship, 5, NORTH) = room;
        SHIPROOMEXIT(ship, 5, EAST) = room + 6;
        SHIPROOMEXIT(ship, 5, WEST) = room + 4;
        SHIPROOMEXIT(ship, 6, NORTH) = room + 3;
        SHIPROOMEXIT(ship, 6, WEST) = room + 5;
        ship->entrance = room + 5;
        break;

    case SH_CARRACK:
        SHIPROOMEXIT(ship, 0, NORTH) = room + 1;
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 5;
        SHIPROOMEXIT(ship, 0, EAST) = room + 3;
        SHIPROOMEXIT(ship, 0, WEST) = room + 2;
        SHIPROOMEXIT(ship, 1, SOUTH) = room;
        SHIPROOMEXIT(ship, 2, SOUTH) = room + 4;
        SHIPROOMEXIT(ship, 2, EAST) = room;
        SHIPROOMEXIT(ship, 3, SOUTH) = room + 6;
        SHIPROOMEXIT(ship, 3, WEST) = room;
        SHIPROOMEXIT(ship, 4, NORTH) = room + 2;
        SHIPROOMEXIT(ship, 4, EAST) = room + 5;
        SHIPROOMEXIT(ship, 5, NORTH) = room;
        SHIPROOMEXIT(ship, 5, SOUTH) = room + 7;
        SHIPROOMEXIT(ship, 5, EAST) = room + 6;
        SHIPROOMEXIT(ship, 5, WEST) = room + 4;
        SHIPROOMEXIT(ship, 6, NORTH) = room + 3;
        SHIPROOMEXIT(ship, 6, WEST) = room + 5;
        SHIPROOMEXIT(ship, 7, NORTH) = room + 5;
        ship->entrance = room + 7;
        break;

    case SH_GALLEON:
        SHIPROOMEXIT(ship, 0, NORTH) = room + 1;
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 5;
        SHIPROOMEXIT(ship, 0, EAST) = room + 3;
        SHIPROOMEXIT(ship, 0, WEST) = room + 2;
        SHIPROOMEXIT(ship, 1, SOUTH) = room;
        SHIPROOMEXIT(ship, 2, SOUTH) = room + 4;
        SHIPROOMEXIT(ship, 2, EAST) = room;
        SHIPROOMEXIT(ship, 3, SOUTH) = room + 6;
        SHIPROOMEXIT(ship, 3, WEST) = room;
        SHIPROOMEXIT(ship, 4, NORTH) = room + 2;
        SHIPROOMEXIT(ship, 4, SOUTH) = room + 7;
        SHIPROOMEXIT(ship, 4, EAST) = room + 5;
        SHIPROOMEXIT(ship, 5, NORTH) = room;
        SHIPROOMEXIT(ship, 5, SOUTH) = room + 8;
        SHIPROOMEXIT(ship, 5, EAST) = room + 6;
        SHIPROOMEXIT(ship, 5, WEST) = room + 4;
        SHIPROOMEXIT(ship, 6, NORTH) = room + 3;
        SHIPROOMEXIT(ship, 6, SOUTH) = room + 9;
        SHIPROOMEXIT(ship, 6, WEST) = room + 5;
        SHIPROOMEXIT(ship, 7, NORTH) = room + 4;
        SHIPROOMEXIT(ship, 7, EAST) = room + 8;
        SHIPROOMEXIT(ship, 8, NORTH) = room + 5;
        SHIPROOMEXIT(ship, 8, EAST) = room + 9;
        SHIPROOMEXIT(ship, 8, WEST) = room + 7;
        SHIPROOMEXIT(ship, 9, NORTH) = room + 6;
        SHIPROOMEXIT(ship, 9, WEST) = room + 8;
        ship->entrance = room + 8;
        break;

    case SH_CORVETTE:
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 2;
        SHIPROOMEXIT(ship, 1, EAST) = room + 2;
        SHIPROOMEXIT(ship, 2, NORTH) = room;
        SHIPROOMEXIT(ship, 2, EAST) = room + 3;
        SHIPROOMEXIT(ship, 2, WEST) = room + 1;
        SHIPROOMEXIT(ship, 3, WEST) = room + 2;
        ship->entrance = room + 2;
        break;

    case SH_DESTROYER:
        SHIPROOMEXIT(ship, 0, NORTH) = room + 1;
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 5;
        SHIPROOMEXIT(ship, 0, EAST) = room + 3;
        SHIPROOMEXIT(ship, 0, WEST) = room + 2;
        SHIPROOMEXIT(ship, 1, SOUTH) = room;
        SHIPROOMEXIT(ship, 2, SOUTH) = room + 4;
        SHIPROOMEXIT(ship, 2, EAST) = room;
        SHIPROOMEXIT(ship, 3, SOUTH) = room + 6;
        SHIPROOMEXIT(ship, 3, WEST) = room;
        SHIPROOMEXIT(ship, 4, NORTH) = room + 2;
        SHIPROOMEXIT(ship, 4, EAST) = room + 5;
        SHIPROOMEXIT(ship, 5, NORTH) = room;
        SHIPROOMEXIT(ship, 5, EAST) = room + 6;
        SHIPROOMEXIT(ship, 5, WEST) = room + 4;
        SHIPROOMEXIT(ship, 6, NORTH) = room + 3;
        SHIPROOMEXIT(ship, 6, WEST) = room + 5;
        ship->entrance = room + 5;
        break;

    case SH_CRUISER:
        SHIPROOMEXIT(ship, 0, NORTH) = room + 1;
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 5;
        SHIPROOMEXIT(ship, 0, EAST) = room + 3;
        SHIPROOMEXIT(ship, 0, WEST) = room + 2;
        SHIPROOMEXIT(ship, 1, SOUTH) = room;
        SHIPROOMEXIT(ship, 2, SOUTH) = room + 4;
        SHIPROOMEXIT(ship, 2, EAST) = room;
        SHIPROOMEXIT(ship, 3, SOUTH) = room + 6;
        SHIPROOMEXIT(ship, 3, WEST) = room;
        SHIPROOMEXIT(ship, 4, NORTH) = room + 2;
        SHIPROOMEXIT(ship, 4, EAST) = room + 5;
        SHIPROOMEXIT(ship, 5, NORTH) = room;
        SHIPROOMEXIT(ship, 5, SOUTH) = room + 7;
        SHIPROOMEXIT(ship, 5, EAST) = room + 6;
        SHIPROOMEXIT(ship, 5, WEST) = room + 4;
        SHIPROOMEXIT(ship, 6, NORTH) = room + 3;
        SHIPROOMEXIT(ship, 6, WEST) = room + 5;
        SHIPROOMEXIT(ship, 7, NORTH) = room + 5;
        ship->entrance = room + 7;
        break;

    case SH_FRIGATE:
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 1;
        SHIPROOMEXIT(ship, 1, NORTH) = room;
        SHIPROOMEXIT(ship, 1, DOWN) = room + 3;
        SHIPROOMEXIT(ship, 2, EAST) = room + 3;
        SHIPROOMEXIT(ship, 3, WEST) = room + 2;
        SHIPROOMEXIT(ship, 3, EAST) = room + 4;
        SHIPROOMEXIT(ship, 3, NORTH) = room + 5;
        SHIPROOMEXIT(ship, 3, UP) = room + 1;
        SHIPROOMEXIT(ship, 4, WEST) = room + 3;
        SHIPROOMEXIT(ship, 5, NORTH) = room + 6;
        SHIPROOMEXIT(ship, 5, SOUTH) = room + 3;
        SHIPROOMEXIT(ship, 5, DOWN) = room + 8;
        SHIPROOMEXIT(ship, 6, NORTH) = room + 7;
        SHIPROOMEXIT(ship, 6, SOUTH) = room + 5;
        SHIPROOMEXIT(ship, 7, SOUTH) = room + 6;
        SHIPROOMEXIT(ship, 8, NORTH) = room + 9;
        SHIPROOMEXIT(ship, 8, UP) = room + 5;
        SHIPROOMEXIT(ship, 9, SOUTH) = room + 8;
        ship->entrance = room + 9;
        break;

    case SH_DREADNOUGHT:
        SHIPROOMEXIT(ship, 0, SOUTH) = room + 1;
        SHIPROOMEXIT(ship, 1, NORTH) = room;
        SHIPROOMEXIT(ship, 1, DOWN) = room + 3;
        SHIPROOMEXIT(ship, 2, EAST) = room + 3;
        SHIPROOMEXIT(ship, 3, WEST) = room + 2;
        SHIPROOMEXIT(ship, 3, EAST) = room + 4;
        SHIPROOMEXIT(ship, 3, NORTH) = room + 5;
        SHIPROOMEXIT(ship, 3, UP) = room + 1;
        SHIPROOMEXIT(ship, 4, WEST) = room + 3;
        SHIPROOMEXIT(ship, 5, NORTH) = room + 6;
        SHIPROOMEXIT(ship, 5, SOUTH) = room + 3;
        SHIPROOMEXIT(ship, 5, DOWN) = room + 8;
        SHIPROOMEXIT(ship, 6, NORTH) = room + 7;
        SHIPROOMEXIT(ship, 6, SOUTH) = room + 5;
        SHIPROOMEXIT(ship, 7, SOUTH) = room + 6;
        SHIPROOMEXIT(ship, 8, NORTH) = room + 9;
        SHIPROOMEXIT(ship, 8, UP) = room + 5;
        SHIPROOMEXIT(ship, 9, SOUTH) = room + 8;
        ship->entrance = room + 9;
        break;
    default:
        break;
    }
}


void reset_ship_physical_layout(P_ship ship)
{
    for (int j = 0; j < MAX_SHIP_ROOM; j++) 
    {
        if (SHIPROOMNUM(ship, j) != -1) 
        {
            int rroom = real_room0(SHIPROOMNUM(ship, j));
            if (!rroom)
                continue;
            if (real_room0(ship->bridge) == rroom) 
                sprintf(buf, "&+yOn the &+WBridge&N&+y of the %s&N %s", SHIPCLASSNAME(ship), ship->name);
            else 
                sprintf(buf, "&+yAboard the %s&N %s", SHIPCLASSNAME(ship), ship->name);
            if (world[rroom].name) 
                world[rroom].name = NULL;

            world[rroom].name = str_dup(buf);
            world[rroom].funct = newshiproom_proc;
            for (int dir = 0; dir < NUMB_EXITS; dir++) 
            {
                if (SHIPROOMEXIT(ship, j, dir) != -1) 
                {
                    if (!world[rroom].dir_option[dir]) 
                        CREATE(world[rroom].dir_option[dir], room_direction_data, 1, MEM_TAG_DIRDATA);

                    world[rroom].dir_option[dir]->to_room = real_room0(SHIPROOMEXIT(ship, j, dir));
                    world[rroom].dir_option[dir]->exit_info = 0;
                } 
                else 
                {
                    if (world[rroom].dir_option[dir])
                        world[rroom].dir_option[dir]->to_room = -1;
                }
            }
        }
    }
}

void setarmor(P_ship ship, bool equal)
{
    ship->maxarmor[FORE]      = ship_arc_properties[ship->m_class].armor[FORE];
    ship->maxarmor[PORT]      = ship_arc_properties[ship->m_class].armor[PORT];
    ship->maxarmor[STARBOARD] = ship_arc_properties[ship->m_class].armor[STARBOARD];
    ship->maxarmor[REAR]      = ship_arc_properties[ship->m_class].armor[REAR];

    ship->maxinternal[FORE]      = ship_arc_properties[ship->m_class].internal[FORE];
    ship->maxinternal[PORT]      = ship_arc_properties[ship->m_class].internal[PORT];
    ship->maxinternal[STARBOARD] = ship_arc_properties[ship->m_class].internal[STARBOARD];
    ship->maxinternal[REAR]      = ship_arc_properties[ship->m_class].internal[REAR];

    if (equal)
    {
        ship->armor[FORE]      = ship->maxarmor[FORE];
        ship->armor[PORT]      = ship->maxarmor[PORT];
        ship->armor[STARBOARD] = ship->maxarmor[STARBOARD];
        ship->armor[REAR]      = ship->maxarmor[REAR];

        ship->internal[FORE]      = ship->maxinternal[FORE];
        ship->internal[PORT]      = ship->maxinternal[PORT];
        ship->internal[STARBOARD] = ship->maxinternal[STARBOARD];
        ship->internal[REAR]      = ship->maxinternal[REAR];
    }
    else
    {
        ship->armor[FORE]      = MIN(ship->armor[FORE], ship->maxarmor[FORE]);
        ship->armor[PORT]      = MIN(ship->maxarmor[PORT], ship->armor[PORT]);
        ship->armor[STARBOARD] = MIN(ship->maxarmor[STARBOARD], ship->armor[STARBOARD]);
        ship->armor[REAR]      = MIN(ship->maxarmor[REAR], ship->armor[REAR]);

        ship->internal[FORE]      = MIN(ship->internal[FORE], ship->maxinternal[FORE]);
        ship->internal[PORT]      = MIN(ship->maxinternal[PORT], ship->internal[PORT]);
        ship->internal[STARBOARD] = MIN(ship->maxinternal[STARBOARD], ship->internal[STARBOARD]);
        ship->internal[REAR]      = MIN(ship->maxinternal[REAR], ship->internal[REAR]);
    }

}


int newshiproom_proc(int room, P_char ch, int cmd, char *arg)
{
   int      old_room, i, j, k;
   P_ship ship;
   int      virt;
   
  if(!(ch))
  {
    return false;
  }
        
   if ((cmd != CMD_LOOK) && (cmd != CMD_DISEMBARK))
      return(FALSE);
 
   ship = getshipfromchar(ch);

   if (cmd == CMD_LOOK)
   {
      if (!arg || !*arg || str_cmp(arg, " out"))
         return(FALSE);

      old_room = ch->in_room;
      char_from_room(ch);
      if (ship->m_class == SH_FRIGATE || ship->m_class == SH_DREADNOUGHT) {
         ch->specials.z_cord = 2;
      }
      char_to_room(ch, ship->location, -1);
      char_from_room(ch);
      ch->specials.z_cord = 0;
      char_to_room(ch, old_room, -2);
      return(TRUE);
   }
   if (cmd == CMD_DISEMBARK)
   {
      if(IS_IMMOBILE(ch))
      {
        send_to_char("\r\nYou cannot disembark in your present condition.\r\n", ch);
        return false;
      }
      
      if(IS_BLIND(ch) &&
        number(0, 5))
      {
        send_to_char("&+WIt is hard to disembark when you cannot see anything... but you keep trying!\r\n", ch);
        return false;
      }

      if (ship->m_class == SH_FRIGATE || ship->m_class == SH_DREADNOUGHT)
       {
    //      if (SHIPISDOCKED(ship) || SHIPANCHORED(ship))
          {
             if (world[ch->in_room].number == ship->entrance)
             {
                if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL) || IS_FIGHTING(ch)) 
                {
                   send_to_char("You're in no position to disembark!\r\n", ch);
                   return(TRUE);
                }
                act("You step off the docking bay of this ship.", FALSE, ch, 0, 0,
                   TO_CHAR);
                act("$n steps off the ship.", TRUE, ch, 0, 0, TO_ROOM);
                char_from_room(ch);
                char_to_room(ch, ship->shipobj->loc.room, 0);
                act("$n disembarks from the docking bay of $p.", TRUE, ch, ship->shipobj, 0, TO_ROOM);
                return TRUE;
             }
             else
             {
                send_to_char("You must disembark from the lower docking bay!\r\n", ch);
                return TRUE;
             }
          }
    //      else
    //      {
    //         send_to_char("All hatches are tightly closed!  You cannot disembark!\r\n", ch);
    //         return TRUE;
    //      }
       }

       i = world[ch->in_room].number;
       j = i - ((int) (i / 10) * 10);
       k = 0;
       if (SHIPROOMEXIT(ship, j, NORTH) == -1 ||
          SHIPROOMEXIT(ship, j, SOUTH) == -1 ||
          SHIPROOMEXIT(ship, j, EAST) == -1 ||
          SHIPROOMEXIT(ship, j, WEST) == -1) {
          k = 1;
       }
       if (!k) {
          send_to_char
          ("You are not close enough to the edge of the ship to jump out!\r\n",
           ch);
          return TRUE;
       }
       if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL) || IS_FIGHTING(ch)) {
          send_to_char("You're in no position to disembark!\r\n", ch);
          return(TRUE);
       }
       if (IS_NPC(ch)) {
          virt = mob_index[GET_RNUM(ch)].virtual_number;
          if (virt == EVIL_AVATAR_MOB || virt == GOOD_AVATAR_MOB) {
             send_to_char("You can't leave this place!\r\n", ch);
             return(FALSE);
          }
       }

       /* board another ship */
    /*
      if ((to_ship = ships[t_ship].dock_vehicle) != NONE) 
      {
       if (!is_valid_ship(to_ship)) {
         send_to_char("Strange... that is a ghost ship!\r\n", ch);
       } else {
         act("You leave this ship and board $p.",
            FALSE, ch, ships[to_ship].obj, 0, TO_CHAR);
         act("$n leaves this ship and boards $p.",
            TRUE, ch, ships[to_ship].obj, 0, TO_ROOM);
         char_from_room(ch);
         char_to_room(ch, ships[to_ship].entrance_room, 0);
         act("$n leaves $p and boards this ship.",
            TRUE, ch, ships[t_ship].obj, 0, TO_ROOM);
       }
      } 
      else 
    */
       {                      /* go on land */
          act("You disembark this ship.", FALSE, ch, 0, 0, TO_CHAR);
          act("$n disembarks this ship.", TRUE, ch, 0, 0, TO_ROOM);
          char_from_room(ch);
          char_to_room(ch, ship->shipobj->loc.room, 0);
          act("$n disembarks from $p.", TRUE, ch, ship->shipobj, 0, TO_ROOM);
       }
       return(TRUE);
   }
   return(TRUE);
}


//--------------------------------------------------------------------
void dock_ship(P_ship ship, int to_room)
{
    // Add in Docking event
    act_to_all_in_ship(ship, "Your ship begins docking procedures.");
    if ((real_room0(world[to_room].number) == to_room) && (to_room != 0)) 
    {
        ship->anchor = world[to_room].number;
        write_newship(ship);
    }
    if (to_room == 0) 
    {
        act_to_all_in_ship(ship, "ERROR: Room is void, moving back to anchor point.");
        ship->location = real_room0(ship->anchor);
        obj_from_room(ship->shipobj);
        obj_to_room(ship->shipobj, ship->location);
    }

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (svs->target == ship) 
        {
            svs->target = NULL;
            act_to_all_in_ship(svs, "&+RTarget lost.\r\n");
        }
    }
    ship->location = to_room;
    ship->repair = SHIPTYPEHULLWEIGHT(ship->m_class);
    assignid(ship, "**");
    act_to_all_in_ship(ship, "Your ship has completed docking procedures.");
    SET_BIT(ship->flags, DOCKED);
    reset_crew_stamina(ship);
    update_ship_status(ship);
}


bool check_undocking_conditions(P_ship ship, P_char ch)
{
    int arc_weapons[4];

    for (int a = 0; a < 4; a++)
        arc_weapons[a] = 0;

    for (int sl = 0; sl < MAXSLOTS; sl++) 
    {
        if (ship->slot[sl].type == SLOT_WEAPON)
        {
            arc_weapons[ship->slot[sl].position]++;
            if (!ship_allowed_weapons[ship->m_class][ship->slot[sl].index])
            {
                sprintf(buf, "Remove weapon [%d], it is not allowed with this hull!\r\n", sl);
                send_to_char(buf, ch);
                return FALSE;
            }
        }
    }
    for (int a = 0; a < 4; a++)
    {
      if (arc_weapons[a] > ship_arc_properties[ship->m_class].max_weapon_slots[a])
      {
          sprintf(buf, 
              "Your have too many weapons at one side!\r\nMaximum allowed weapons for this ship is:\r\nFore: %d  Starboard: %d  Port: %d  Rear: %d\r\n",
              ship_arc_properties[ship->m_class].max_weapon_slots[FORE], 
              ship_arc_properties[ship->m_class].max_weapon_slots[STARBOARD], 
              ship_arc_properties[ship->m_class].max_weapon_slots[PORT], 
              ship_arc_properties[ship->m_class].max_weapon_slots[REAR]);
          send_to_char(buf, ch);
          return FALSE;
      }
    }

    if (SHIPTYPEMINLEVEL(ship->m_class) > GET_LEVEL(ch)) 
    {
        send_to_char ("You are too low for such a big ship! Get more experience or downgrade the hull!'\r\n", ch);
        return FALSE;
    }
    return TRUE;
}

int order_sail(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    if (SHIPANCHORED(ship)) 
    {
        send_to_char("You are anchored, de-anchor first.\r\n", ch);
        return TRUE;
    }
    if ((world[SHIPLOCATION(ship)].number < 110000) && !IS_SET(ship->flags, AIR)) 
    {
        send_to_char ("You cannot autopilot until you are out in the open seas!\r\n", ch);
        return TRUE;
    }
    int dir = 0;
    if (is_number(arg1)) 
    {
        dir = atoi(arg1);
        if (dir < 0 || dir > 359) 
        {
            send_to_char("0-359 degrees or N E S W!\r\n", ch);
            return TRUE;
        }
    } 
    else 
    {
        if (isname(arg1, "north n"))
            dir = 0;
        else if (isname(arg1, "east e"))
            dir = 90;
        else if (isname(arg1, "south s"))
            dir = 180;
        else if (isname(arg1, "west w"))
            dir = 270;
        else if (isname(arg1, "off"))
        {
            if (ship->shipai) 
            {
                if (!IS_SET(ship->shipai->flags, AIB_ENABLED) && !IS_SET(ship->shipai->flags, AIB_AUTOPILOT)) 
                {
                    send_to_char("There IS no active autopilot atm!\r\n", ch);
                    return TRUE;
                } 
                else 
                {
                    REMOVE_BIT(ship->shipai->flags, AIB_ENABLED);
                    REMOVE_BIT(ship->shipai->flags, AIB_AUTOPILOT);
                    act_to_all_in_ship(ship, "Autopilot disengaged.");
                    return TRUE;
                }
            } 
            else 
            {
                send_to_char("There IS no active autopilot atm!\r\n", ch);
                return TRUE;
            }
        } 
        else 
        {
            send_to_char ("Valid syntax is order sail <N/E/S/W/Heading> <number of rooms>\r\n", ch);
            return TRUE;
        }
    }
    int dist = 0;
    if (is_number(arg2)) 
    {
        dist = atoi(arg2);
        if (dist > 35) 
        {
            send_to_char("Maximum number of rooms is 35!\r\n", ch);
            return TRUE;
        }
    } 
    else 
    {
        send_to_char ("Valid syntax is order sail <N/E/S/W/Heading> <number of rooms>\r\n", ch);
        return TRUE;
    }
    if (ship->shipai)
        REMOVE_BIT(ship->shipai->flags, AIB_ENABLED);

    assign_shipai(ship);
    getmap(ship);
    ship->shipai->t_room = tactical_map [(int) (xbearing(dir, dist + 1) + ship->x)][100 - (int)(ybearing (dir, dist + 1) + ship->y)].rroom;

    SET_BIT(ship->shipai->flags, AIB_AUTOPILOT);
    ship->shipai->mode = AIM_AUTOPILOT;
    sprintf(buf, "Autopilot engaged, heading %d for %d rooms. Target room is %d", dir, dist, world[ship->shipai->t_room].number);
    act_to_all_in_ship(ship, buf);
    return TRUE;
}
int jettison_cargo(P_char ch, P_ship ship, char* arg)
{
    int left = INT_MAX, done = 0;
    if (is_number(arg))
        left = atoi(arg);

    for (int i = 0; i < MAXSLOTS; i++)
    {
        if (ship->slot[i].type == SLOT_CARGO)
        {
            if (ship->slot[i].val0 > left)
            {
                sprintf(buf, "%d units of %s have been jettisoned!\r\n", left, cargo_name[ship->slot[i].index]);
                send_to_char(buf, ch);
                ship->slot[i].val0 -= left;
                done += left;
                break;
            }
            else
            {
                sprintf(buf, "%d units of %s have been jettisoned!\r\n", ship->slot[i].val0, cargo_name[ship->slot[i].index]);
                send_to_char(buf, ch);
                left -= ship->slot[i].val0;
                done += ship->slot[i].val0;
                ship->slot[i].clear();
                if (left == 0)
                    break;
            }
        }
    }
    if (done == 0) 
    {
        send_to_char("You have no cargo to jettison!\r\n", ch);
    }
    else
    {
        update_ship_status(ship);
        write_newship(ship);
    }
    return TRUE;
}
int jettison_contraband(P_char ch, P_ship ship, char* arg)
{
    int left = INT_MAX, done = 0;
    if (is_number(arg))
        left = atoi(arg);

    for (int i = 0; i < MAXSLOTS; i++)
    {
        if (ship->slot[i].type == SLOT_CONTRABAND)
        {
            if (ship->slot[i].val0 > left)
            {
                sprintf(buf, "%d units of %s have been jettisoned!\r\n", left, contra_name[ship->slot[i].index]);
                send_to_char(buf, ch);
                ship->slot[i].val0 -= left;
                done += left;
                break;
            }
            else
            {
                sprintf(buf, "%d units of %s have been jettisoned!\r\n", ship->slot[i].val0, contra_name[ship->slot[i].index]);
                send_to_char(buf, ch);
                left -= ship->slot[i].val0;
                done += ship->slot[i].val0;
                ship->slot[i].clear();
                if (left == 0)
                    break;
            }
        }
    }
    if (done == 0) 
    {
        send_to_char("You have no contraband to jettison!\r\n", ch);
    }
    else
    {
        update_ship_status(ship);
        write_newship(ship);
    }
    return TRUE;
}
int order_undock(P_char ch, P_ship ship)
{        
    if (!SHIPISDOCKED(ship) && !SHIPANCHORED(ship)) 
    {
        send_to_char("The ship is not docked or anchored!\r\n", ch);
        return TRUE;
    }
    
    if (SHIPIMMOBILE(ship))
    {
        send_to_char("&+RYour ship is immobilized. Undocking procedures cancelled.\r\n", ch);
        return TRUE;
    }
    
    if (ship->timer[T_UNDOCK] != 0)
    {
        send_to_char("The crew is already working on it!\r\n", ch);
        return TRUE;
    }
    if (ship->mainsail == 0) 
    {
        send_to_char ("You cannot unfurl your sails, they are destroyed.\r\n", ch);
        return TRUE;
    }
    if (!check_undocking_conditions(ship, ch))
        return TRUE;

    send_to_char("Your crew begins undocking procedures.\r\n", ch);
    ship->pilotlevel = GET_LEVEL(ch);
    if (RACE_PUNDEAD(ch))
        ship->race = UNDEADSHIP;
    else if (RACE_GOOD(ch))
        ship->race = GOODIESHIP;
    else
        ship->race = EVILSHIP;
    ship->timer[T_UNDOCK] = 30;
    ship->time = time(NULL);
    update_ship_status(ship);
    return TRUE;
}


int order_maneuver(P_char ch, P_ship ship, char* arg)
{
    if (SHIPISDOCKED(ship)) 
    {
        send_to_char("Might want to undock first.\r\n", ch);
        return TRUE;
    }
    if (SHIPIMMOBILE(ship)) 
    {
        send_to_char("You're immobile, you can't maneuver!\r\n", ch);
        return TRUE;
    }
    if (ship->speed > 20) 
    {
        send_to_char("You're coming in too fast!!\r\n", ch);
        return TRUE;
    }
    if (ship->target != NULL) 
    {
        send_to_char ("Your crew is on alert status while you have a target locked!\r\n", ch);
        return TRUE;
    }
    if (ship->timer[T_BSTATION] > 0) 
    {
        sprintf(buf, "Your crew is still on alert status and will take %d seconds longer to stand down.\r\n",
                ship->timer[T_BSTATION]);
        send_to_char(buf, ch);
        return TRUE;
    }
    if (ship->timer[T_MANEUVER] > 0) 
    {
        send_to_char("Your ship isn't ready to maneuver again yet.\r\n", ch);
        return TRUE;
    }

    int dir;
    if (isname(arg, "n") || isname(arg, "north"))
        dir = NORTH;
    else if (isname(arg, "e") || isname(arg, "east")) 
        dir = EAST;
    else if (isname(arg, "s") || isname(arg, "south")) 
        dir = SOUTH;
    else if (isname(arg, "w") || isname(arg, "west"))
        dir = WEST;
    else 
    {
        sprintf(buf, "%s is not a valid direction try (north, east, south, west)\r\n", arg);
        send_to_char(buf, ch);
        return TRUE;
    }
    if (world[SHIPLOCATION(ship)].dir_option[dir]) 
    {
        if (world[SHIPLOCATION(ship)].dir_option[dir]->to_room != NOWHERE) 
        {
            if (IS_WATER_ROOM(world[SHIPLOCATION(ship)].dir_option[dir]->to_room) ||
                IS_SET(world [world[SHIPLOCATION(ship)].dir_option[dir]->to_room]. room_flags, DOCKABLE)) 
            {
                if (ship->shipai) 
                {
                    if (IS_SET(ship->shipai->flags, AIB_ENABLED)) 
                    {
                        if (IS_SET(ship->shipai->flags, AIB_AUTOPILOT)) 
                        {
                            REMOVE_BIT(ship->shipai->flags, AIB_AUTOPILOT);
                            REMOVE_BIT(ship->shipai->flags, AIB_ENABLED);
                            act_to_all_in_ship(ship, "Autopilot stopped.");
                        }
                    }
                }
                int dir_room = world[SHIPLOCATION(ship)].dir_option[dir]->to_room;
                if ((world[dir_room].number < 110000) ||
                    (world[SHIPLOCATION(ship)].number < 110000) ||
                    IS_SET(world[dir_room].room_flags, DOCKABLE)) 
                {
                    ship->speed = 0;
                    ship->setspeed = 0;
                    sprintf(buf, "%s maneuvers to the %s.\r\n", ship->name, dirs[dir]);
                    send_to_room(buf, SHIPLOCATION(ship));
                    SHIPLOCATION(ship) =
                    world[SHIPLOCATION(ship)].dir_option[dir]->to_room;
                    obj_from_room(SHIPOBJ(ship));
                    obj_to_room(SHIPOBJ(ship), SHIPLOCATION(ship));
                    sprintf(buf, "%s maneuvers in from the %s.\r\n", ship->name, dirs[rev_dir[dir]]);
                    send_to_room(buf, SHIPLOCATION(ship));
                    sprintf(buf, "Your ship manuevers to the %s.", dirs[dir]);
                    act_to_all_in_ship(ship, buf);
                    ship->timer[T_MANEUVER] = 5;
                    if (ship->target != NULL)
                        ship->target = NULL;
                    everyone_look_out_newship(ship);
                    if (IS_SET(world[SHIPLOCATION(ship)].room_flags, DOCKABLE))
                    {
                        dock_ship(ship, SHIPLOCATION(ship));
                        check_contraband(ship, SHIPLOCATION(ship));
                    }
                    return TRUE;
                } 
                else 
                {
                    send_to_char("You cannot maneuver out in the open seas!\r\n", ch);
                    return TRUE;
                }
            } 
            else 
            {
                send_to_char("We'll Crash into land if we do that!\r\n", ch);
                return TRUE;
            }
        }
    } 
    else 
    {
        send_to_char("Sorry the ship will not go there.\r\n", ch);
        return TRUE;
    }
    return TRUE;
}
int order_anchor(P_char ch, P_ship ship)
{
    if (SHIPSINKING(ship)) 
    {
        send_to_char ("Anchor while sinking?! Your ship IS the anchor now!\r\n", ch);
        return TRUE;
    }

    if (SHIPISDOCKED(ship)) 
    {
        send_to_char("You are docked, undock first!\r\n", ch);
        return TRUE;
    }
    if (ship->speed != 0) 
    {
        send_to_char("You need to stop to anchor!\r\n", ch);
        return TRUE;
    }

    if (ship->shipai) 
    {
        if (IS_SET(ship->shipai->flags, AIB_ENABLED)) 
        {
            if (IS_SET(ship->shipai->flags, AIB_AUTOPILOT)) 
            {
                REMOVE_BIT(ship->shipai->flags, AIB_AUTOPILOT);
                REMOVE_BIT(ship->shipai->flags, AIB_ENABLED);
                act_to_all_in_ship(ship, "Autopilot stopped.");
            }
        }
    }

    act_to_all_in_ship(ship, "&+yYour ship anchors here and your crew begins repairs.&N\r\n");
    SET_BIT(ship->flags, ANCHOR);
    ship->speed = 0;
    ship->setspeed = 0;
    return TRUE;
}
int order_ram(P_char ch, P_ship ship, char* arg)
{
// Ship ramming causes strange behaviour.
    //send_to_char("Ram is currently disabled!\r\n", ch);
    //return TRUE;

    
    if (SHIPSINKING(ship)) 
    {
        send_to_char("Ram while sinking, yeah right!\r\n", ch);
        return TRUE;
    }

    if (SHIPISDOCKED(ship)) 
    {
        send_to_char("Ram while docked, yeah right!\r\n", ch);
        return TRUE;
    }
    if (!*arg) 
    {
        if (ship->timer[T_RAM] != 0) 
        {
            send_to_char("&+WYou aren't ready to ram again!\r\n", ch);
            return TRUE;
        }
        if (ship->target == NULL) 
        {
            send_to_char("No target to ram.\r\n", ch);
            return TRUE;
        }

        if (ship->speed < 20)
        {
            send_to_char("&+WYou are too slow to ram!\r\n", ch);
            return TRUE;
        }

        if (!IS_SET(ship->flags, RAMMING))
        {
            SET_BIT(ship->flags, RAMMING);
            act_to_all_in_ship(ship, "&+WYou crew braces for impact!&N");
        } 
        else 
        {
            send_to_char("Ship is already in ramming mode!\r\n", ch);
        }
        return TRUE;
    }

    if (isname(arg, "off")) 
    {
        if (IS_SET(ship->flags, RAMMING)) 
        {
            act_to_all_in_ship(ship, "&+WStand down from ramming speed!");
            REMOVE_BIT(ship->flags, RAMMING);
            return TRUE;
        } 
        else 
        {
            send_to_char("Ship is currently not ramming anyone.\r\n", ch);
            return TRUE;
        }
    } 
    else 
    {
        send_to_char("Syntax: order ram or order ram off\r\n", ch);
        return TRUE;
    }
}

int order_heading(P_char ch, P_ship ship, char* arg)
{
    int      heading;
    if (!*arg) 
    {
        sprintf(buf, "Current heading: &+W%d&N\r\nSet heading: &+W%d&N\r\n", ship->heading, ship->setheading);
        send_to_char(buf, ch);
        return TRUE;
    } 
    else 
    {
        if (!is_number(arg)) 
        {
            if (isname(arg, "e") || isname(arg, "east"))
                ship->setheading = 90;
            else if (isname(arg, "w") || isname(arg, "west"))
                ship->setheading = 270;
            else if (isname(arg, "s") || isname(arg, "south"))
                ship->setheading = 180;
            else if (isname(arg, "n") || isname(arg, "north"))
                ship->setheading = 0;
            else if (isname(arg, "ne northeast"))
                ship->setheading = 45;
            else if (isname(arg, "nw northwest"))
                ship->setheading = 315;
            else if (isname(arg, "se southeast"))
                ship->setheading = 135;
            else if (isname(arg, "sw southwest"))
                ship->setheading = 225;
            else if (isname(arg, "h heading"))
                ship->setheading = ship->heading;
            else 
            {
                send_to_char("Please enter a heading from 0-360 or N E S W NW NE SE SW.\r\n", ch);
                return TRUE;
            }
        }
    }
    if (is_number(arg)) 
    {
        heading = atoi(arg);
        if ((0 <= heading) && (heading <= 360)) 
            ship->setheading = heading;
        else 
        {
            send_to_char("Please enter a heading from 0-360 or N E S W NW NE SE SW.\r\n", ch);
            return TRUE;
        }
    }
    sprintf(buf, "Heading set to &+W%d&N.", ship->setheading);
    act_to_all_in_ship(ship, buf);
    return TRUE;
}

int order_speed(P_char ch, P_ship ship, char* arg)
{
    int      speed;

    if ((world[SHIPLOCATION(ship)].number < 110000) && !IS_SET(ship->flags, AIR)) 
    {
        send_to_char ("You cannot unfurl the sails till you are out on open sea!\r\n", ch);
        return TRUE;
    }
    if (!*arg) 
    {
        sprintf(buf, "Current speed: &+W%d&N\r\nSet speed: &+W%d&N\r\n",  ship->speed, ship->setspeed);
        send_to_char(buf, ch);
    } 
    else 
    {
        if (is_number(arg)) 
        {
            speed = atoi(arg);
            if ((speed <= ship->get_maxspeed()) && speed >=0) 
            {
                ship->setspeed = speed;
                sprintf(buf, "Speed set to &+W%d&N.", ship->setspeed);
                act_to_all_in_ship(ship, buf);
            } 
            else 
            {
                sprintf(buf, "This ship can only go from &+W%d&N to &+W%d&N.\r\n", 0, ship->get_maxspeed());
                send_to_char(buf, ch);
            }
        } 
        else if (isname(arg, "max maximum")) 
        {
            if (!SHIPIMMOBILE(ship)) 
            {
                ship->setspeed = ship->get_maxspeed();
                sprintf(buf, "Speed set to &+W%d&N.", ship->setspeed);
                act_to_all_in_ship(ship, buf);
            } 
            else 
                send_to_char("&+RThe ship is immobile, it cannot move!\r\n", ch);
        } 
        else if (isname(arg, "med medium")) 
        {
            if (!SHIPIMMOBILE(ship)) 
            {
                ship->setspeed = MAX(1, ship->get_maxspeed() * 2 / 3);
                sprintf(buf, "Speed set to &+W%d&N.", ship->setspeed);
                act_to_all_in_ship(ship, buf);
            } 
            else 
                send_to_char("&+RThe ship is immobile, it cannot move!\r\n", ch);
        } 
        else if (isname(arg, "min minimum slow")) 
        {
            if (!SHIPIMMOBILE(ship)) 
            {
                ship->setspeed = MAX(1, ship->get_maxspeed() / 3);
                sprintf(buf, "Speed set to &+W%d&N.", ship->setspeed);
                act_to_all_in_ship(ship, buf);
            } 
            else
                send_to_char("&+RThe ship is immobile, it cannot move!\r\n", ch);
        } 
        else 
        {
            sprintf(buf, "Please enter a number value between %3d-%-d.\r\n", 0, ship->get_maxspeed());
            send_to_char(buf, ch);
        }
    }
    return TRUE;
}

int order_signal(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    if (!*arg1) 
    {
        send_to_char("Syntax: signal <ship id> <message>\r\n", ch);
        return TRUE;
    }

    if (!*arg2) 
    {
        send_to_char("Send what message?\r\n", ch);
        return TRUE;
    }

    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to send signals!\r\n", ch); 
        return TRUE; 
    } 
    int k = getcontacts(ship);
    for (int i = 0; i < k; i++) 
    {
        P_ship target = contacts[i].ship;
        if (isname(arg1, target->id)) 
        {
            if (SHIPISDOCKED(target)) 
            {
                send_to_char ("You cannot send signals to docked ships!\r\n", ch);
                return TRUE;
            }
            if (contacts[i].range > 20.0)
            {
                send_to_char ("This ship is too far to see your signals!\r\n", ch);
                return TRUE;
            }
            if (target->race != ship->race)
            {
                send_to_char ("They wouldn't understand your signals!\r\n", ch);
                return TRUE;
            }

            sprintf(buf, "&+GYou've sent a &+Ys&+Bi&+Wg&+yn&+Ma&+Cl &+Gmessage to &+W[%s]&N:%s&+G.&N\r\n", SHIPID(target), SHIPNAME(target));
            send_to_char (buf, ch);
            sprintf(buf, "&+GYour ship has recieved a &+Ys&+Bi&+Wg&+yn&+Ma&+Cl &+Gmessage from &+W[%s]&N:%s &+Gdecoded as \'&+Y%s&+G\'.&n", SHIPID(ship), SHIPNAME(ship), arg2);
            act_to_all_in_ship(target, buf);
            return TRUE;
        }
    }
    send_to_char("This ship not in sight!\r\n", ch);
    return TRUE;
}

int do_scan(P_char ch, P_ship ship, char* arg)
{
    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to scan!\r\n", ch); 
        return TRUE; 
    } 
 
    if (!*arg) 
    {
        if (ship->target != NULL) 
        {
            scantarget(ship->target, ch);
            return TRUE;
        }
        send_to_char("No target locked. Syntax: Scan <target ID> or Scan\r\n", ch);
        return TRUE;
    }
    int k = getcontacts(ship);
    for (int i = 0; i < k; i++) 
    {
        if (isname(arg, contacts[i].ship->id)) 
        {
            scantarget(contacts[i].ship, ch);
            return TRUE;
        }
    }
    send_to_char("Target not in sight!\r\n", ch);
    return TRUE;
}

int do_fire (P_char ch, P_ship ship, char* arg)
{
    if (!isname(GET_NAME(ch), SHIPOWNER(ship)) && !IS_TRUSTED(ch) &&
        (ch->group == NULL ? 1 : get_char2(str_dup(SHIPOWNER(ship))) ==
         NULL ? 1 : (get_char2(str_dup(SHIPOWNER(ship)))->group != ch->group))) 
    {
        send_to_char("You are not the captain of this ship, the crew ignores you.\r\n", ch);
        return TRUE;
    }

    if (SHIPSINKING(ship)) 
    {
        send_to_char ("The ship is sinking! Your crew has already abandoned ship!\r\n", ch);
        return TRUE;
    }
    if (SHIPISDOCKED(ship) || SHIPANCHORED(ship)) 
    {
        send_to_char("Your crew isn't ready, undock first.\r\n", ch);
        return TRUE;
    }

    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to fire your weapons!\r\n", ch); 
        return TRUE; 
    } 
    if (!*arg) 
    {
        send_to_char("Valid syntax: 'fire <fore/starboard/port/rear/weapon number>'\r\n", ch);
        return TRUE;
    }
    if (ship->target == NULL) 
    {
        send_to_char("No target locked.\r\n", ch);
        return TRUE;
    }
    if (ship->timer[T_MINDBLAST] > 0) 
    {
        send_to_char("&+RYour crew members crawl around and ignore your orders!&N\r\n", ch);
        return TRUE;
    }
    if (ship->timer[T_RAM_WEAPONS] > 0) 
    {
        send_to_char("Your gun crew has not recovered from the ram impact yet!\r\n", ch);
        return TRUE;
    }
    arg = skip_spaces(arg);
    if (isname(arg, "fore")) 
    {
        return fire_arc(ship, ch, FORE);
    }
    else if (isname(arg, "starboard")) 
    {
        return fire_arc(ship, ch, STARBOARD);
    }
    else if (isname(arg, "port")) 
    {
        return fire_arc(ship, ch, PORT);
    }
    else if (isname(arg, "rear")) 
    {
        return fire_arc(ship, ch, REAR);
    }
    else if (is_number(arg)) 
    {
        return fire_weapon(ship, ch, atoi(arg));
    }
    send_to_char("Valid syntax: 'fire <fore/starboard/port/rear/weapon number>'\r\n", ch);
    return TRUE;
}

int lock_target(P_char ch, P_ship ship, char* arg)
{
    if (!isname(GET_NAME(ch), SHIPOWNER(ship)) && !IS_TRUSTED(ch) &&
        (ch->group == NULL ? 1 : get_char2(str_dup(SHIPOWNER(ship))) ==
         NULL ? 1 : (get_char2(str_dup(SHIPOWNER(ship)))->group != ch->group))) 
    {
        send_to_char ("You are not the captain of this ship, the crew ignores you.\r\n", ch);
        return TRUE;
    }

    if (!*arg) 
    {
        send_to_char("Syntax: Lock <target id>\r\n", ch);
        return TRUE;
    }
    if (isname(arg, "off")) 
    {
        if (ship->target != NULL) {
            ship->target = NULL;
            act_to_all_in_ship(ship, "Target Cleared.\r\n");
            return TRUE;
        } else {
            send_to_char("You currently have no target.\r\n", ch);
            return TRUE;
        }
    }


    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to lock your weapons!\r\n", ch); 
        return TRUE; 
    } 
    int k = getcontacts(ship);
    for (int i = 0; i < k; i++) 
    {
        P_ship temp = contacts[i].ship;
        if (isname(arg, temp->id)) 
        {
            if (SHIPISDOCKED(temp)) 
            {
                send_to_char ("You cannot lock onto docked ships, that's against the sailor's code!\r\n", ch);
                return TRUE;
            }
            if (ship->timer[T_BSTATION] == 0) 
            {
                act_to_all_in_ship(ship, "&+RYour crew scrambles to battle stations!&N\r\n");
                ship->timer[T_BSTATION] = BSTATION;
            }
            ship->target = temp;
            sprintf(buf, "Locked onto &+W[%s]:&N %s&N\r\n", temp->id, temp->name);
            act_to_all_in_ship(ship, buf);
            return TRUE;
        }
    }
    send_to_char("Target not in sight!\r\n", ch);
    return TRUE;
}

int look_cargo(P_char ch, P_ship ship)
{
    if( ship->money > 0 )
    {
        sprintf(buf, "&+WShip's Coffer: %s\r\n\r\n", coin_stringv(ship->money));
        send_to_char(buf, ch);
    } 

    send_to_char("&+cCargo Manifest&N\r\n", ch);
    send_to_char("----------------------------------\r\n", ch);

    for (int slot = 0; slot < MAXSLOTS; slot++) 
    {
        if (ship->slot[slot].type == SLOT_CARGO) 
        {
            sprintf(buf, "%s&n, &+Y%d&n crates, invoiced at %s.\r\n",
              cargo_name[ship->slot[slot].index],
              ship->slot[slot].val0,
              coin_stringv(ship->slot[slot].val1));
            send_to_char(buf, ch);
        }
        else if (ship->slot[slot].type == SLOT_CONTRABAND) 
        {
            sprintf(buf, "&+Y*&n%s&n, &+Y%d&n crates, invoiced at %s.\r\n",
              contra_name[ship->slot[slot].index],
              ship->slot[slot].val0,
              coin_stringv(ship->slot[slot].val1));
            send_to_char(buf, ch);
        }
    }

    sprintf(buf, "\r\nCargo capacity: &+W%d&n/&+W%d\r\n", SHIPAVAILCARGO(ship), SHIPMAXCARGO(ship));
    send_to_char(buf, ch);

    return TRUE;
}

int look_crew (P_char ch, P_ship ship)
{
    sprintf(buf, "&+L            Crew                  Skill  Stamina\r\n");
    send_to_char(buf, ch);

    sprintf(buf, "&+LSails     :&+W %-20s  %5d\r\n",
        ship_crew_data[ship->sailcrew.index].name, 
        ship->sailcrew.skill / 1000);
    send_to_char(buf, ch);

    sprintf(buf, "&+LGuns      :&+W %-20s  %5d  %d/%d\r\n",
        ship_crew_data[ship->guncrew.index].name, 
        ship->guncrew.skill / 1000,
        ship->guncrew.stamina, ship->guncrew.max_stamina);
    send_to_char(buf, ch);

    sprintf(buf, "&+LRepair    :&+W %-20s  %5d\r\n",
        ship_crew_data[ship->repaircrew.index].name, 
        ship->repaircrew.skill / 1000);
    send_to_char(buf, ch);

    // TODO: Oars

    return TRUE;
}
int look_weapon (P_char ch, P_ship ship, char* arg)
{
    if (!*arg) 
    {
        send_to_char("Valid syntax: look <sight/weapon> <weapon number>\r\n", ch);
        return TRUE;
    }
    if(SHIPISDOCKED(ship)) 
    { 
        send_to_char("You must be undocked to sight your weapons!\r\n", ch); 
        return TRUE; 
    } 
                     
    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to sight your weapons!\r\n", ch); 
        return TRUE; 
    } 
                          
    if (ship->target == NULL)
        send_to_char("No target.\r\n", ch);

    if (!is_number(arg)) 
    {
        send_to_char("Invalid number!\r\n", ch);
        return TRUE;
    }
    int slot = atoi(arg);
    if ((slot >= MAXSLOTS) || (slot < 0)) 
    {
        send_to_char("Invalid Weapon\r\n", ch);
        return TRUE;
    }
    if (ship->slot[slot].type != SLOT_WEAPON) 
    {
        send_to_char("Invalid Weapon!\r\n", ch);
        return TRUE;
    }
    if (SHIPWEAPONDESTROYED(ship, slot)) 
    {
        send_to_char("That weapon is destroyed!\r\n", ch);
        return TRUE;
    }
    if (SHIPWEAPONDAMAGED(ship, slot)) 
    {
        send_to_char("That weapon is damaged!\r\n", ch);
        return TRUE;
    }

    int k = getcontacts(ship);
    int j;
    for (j = 0; j < k; j++)
    {
        if (ship->target == contacts[j].ship)
            break;
    }
    if (j == k) 
    {
        send_to_char("Target out of range or out of sight!\r\n", ch);
        return TRUE;
    }
    sprintf(buf, "Chance to hit target: &+W%d%%&N\r\n", weaponsight(ch, ship, ship->target, slot, contacts[j].range));
    send_to_char(buf, ch);
    return TRUE;
}

int look_tactical_map(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    int      x, y;
    float    shiprange;

    if( !IS_MAP_ROOM(ship->location) )
    {
        send_to_char("You have no maps for this region.\r\n", ch);
        return TRUE;
    }

    if(SHIPISDOCKED(ship)) 
    { 
        send_to_char("You must be undocked to look tactical.\r\n", ch); 
        return TRUE; 
    } 
    
    if (!*arg1) 
    {
        x = (int) ship->x;
        y = (int) ship->y;
    } 
    else 
    {
        if (!*arg2) 
        {
            send_to_char("&+WValid syntax: look <tactical/t> [<x> <y>]&N\r\n", ch);
            return TRUE;
        }
        if (is_number(arg1) && is_number(arg2)) 
        {
            shiprange = range(ship->x, ship->y, 0, atoi(arg1), atoi(arg2), 0);
            if ((int) (shiprange + .5) <= 35) 
            {
                x = atoi(arg1);
                y = atoi(arg2);
            } 
            else 
            {
                sprintf(buf, "This coord is out of range.\r\nMust be within 35 units.\r\nCurrent range: %3.1f\r\n", shiprange);
                send_to_char(buf, ch);
                return TRUE;
            }
        } 
        else 
        {
            send_to_char("&+WValid syntax: look <tactical/t> [<x> <y>]&N\r\n", ch);
            return TRUE;
        }
    }
    getmap(ship);
    sprintf(buf,
            "&+W     %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d&N\r\n",
            x - 11, x - 9, x - 7, x - 5, x - 3, x - 1, x + 1, x + 3, x + 5,
            x + 7, x + 9, x + 11);
    send_to_char(buf, ch, LOG_NONE);
    sprintf(buf,
            "     __ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__&N\r\n",
            x - 10, x - 8, x - 6, x - 4, x - 2, x, x + 2, x + 4, x + 6,
            x + 8, x + 10);
    send_to_char(buf, ch, LOG_NONE);
    y = 100 - y;
    for (int i = y - 7; i < y + 8; i++) 
    {
        sprintf(buf,
                "&+W%-3d&N /%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\\r\n",
                100 - i, tactical_map[x - 11][i].map, tactical_map[x - 9][i].map,
                tactical_map[x - 7][i].map, tactical_map[x - 5][i].map,
                tactical_map[x - 3][i].map, tactical_map[x - 1][i].map,
                tactical_map[x + 1][i].map, tactical_map[x + 3][i].map,
                tactical_map[x + 5][i].map, tactical_map[x + 7][i].map,
                tactical_map[x + 9][i].map, tactical_map[x + 11][i].map);
        send_to_char(buf, ch, LOG_NONE);
        sprintf(buf,
                "    \\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/\r\n",
                tactical_map[x - 10][i].map, tactical_map[x - 8][i].map,
                tactical_map[x - 6][i].map, tactical_map[x - 4][i].map,
                tactical_map[x - 2][i].map, tactical_map[x][i].map,
                tactical_map[x + 2][i].map, tactical_map[x + 4][i].map,
                tactical_map[x + 6][i].map, tactical_map[x + 8][i].map,
                tactical_map[x + 10][i].map);
        send_to_char(buf, ch, LOG_NONE);
    }
    send_to_char
    ("       \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/\r\n", ch);
    return TRUE;
}

int look_contacts(P_char ch, P_ship ship)
{

    if( !IS_MAP_ROOM(ship->location) ||
        SHIPISDOCKED(ship) ) 
    {
        send_to_char("You must be on the open sea to display contacts.\r\n", ch);
        return TRUE;
    }

    int k = getcontacts(ship);
    send_to_char ("&+WContact listing\r\n===============================================&N\r\n", ch);
    for (int i = 0; i < k; i++) 
    {
        if (SHIPISDOCKED(contacts[i].ship)) 
        {
            if (contacts[i].range > 5)
                continue;
        }
        dispcontact(i);
        if (contacts[i].ship == ship->target)
            sprintf(buf, "&+G%s", contact);
        else if (contacts[i].ship->target == ship)
            sprintf(buf, "&+R%s", contact);
        else
            sprintf(buf, contact);
        send_to_char(buf, ch);
    }
    return TRUE;
}


int look_weaponspec(P_char ch, P_ship ship)
{
    char rng[20], dam[20];
    send_to_char("&+rWeapon Specifications&N\r\n", ch);
    send_to_char("&+r===========================================================&N\r\n", ch);
    send_to_char("Num  Name                     Range  Damage    Ammo   Status\r\n", ch);
    for (int slot = 0; slot < MAXSLOTS; slot++) 
    {
        if (ship->slot[slot].type == SLOT_WEAPON) 
        {
            int w_index = ship->slot[slot].index;
            sprintf(rng, "%d-%d", weapon_data[w_index].min_range, weapon_data[w_index].max_range);
            if (weapon_data[w_index].fragments > 1)
            {
                if (weapon_data[w_index].min_damage == weapon_data[w_index].max_damage)
                    sprintf(dam, "%d x %d", weapon_data[w_index].fragments, weapon_data[w_index].min_damage);
                else
                    sprintf(dam, "%d x %d-%d", weapon_data[w_index].fragments, weapon_data[w_index].min_damage, weapon_data[w_index].max_damage);
            }
            else
            {
                if (weapon_data[w_index].min_damage == weapon_data[w_index].max_damage)
                    sprintf(dam, "%d", weapon_data[w_index].min_damage);
                else
                    sprintf(dam, "%d-%d", weapon_data[w_index].min_damage, weapon_data[w_index].max_damage);
            }
            if (!SHIPWEAPONDESTROYED(ship, slot))
            {
                sprintf(buf,  "&+W[%2d]  %-20s    %5s  %7s    %2d    %s&N\r\n",
                  slot, weapon_data[w_index].name, rng, dam, weapon_data[w_index].ammo, ship->slot[slot].get_status_str());
            }
            else
            {
                sprintf(buf,  "&+W[%2d]  %-20s    %5s  %7s    &+L**    %s&N\r\n",
                  slot, weapon_data[w_index].name, rng, dam, ship->slot[slot].get_status_str());
            }
            send_to_char(buf, ch);
        }
    }
    return TRUE;
}


char slot_desc[100];
char* generate_slot(P_ship ship, int sl)
{
  if (sl >= MAXSLOTS)
  {
    strcpy(slot_desc, "");
    return slot_desc;
  }
    
  if (ship->slot[sl].type == SLOT_CARGO || ship->slot[sl].type == SLOT_CONTRABAND)
  {
      sprintf(slot_desc, "&+W[%2d] &n%s&n (&+Y%d&n %s)", sl, ship->slot[sl].get_description(), ship->slot[sl].val0, (ship->slot[sl].val0 > 1) ? "crates" : "crate");
  }
  else if (ship->slot[sl].type == SLOT_WEAPON)
  {
      if (!SHIPWEAPONDESTROYED(ship, sl))
      {
          sprintf(slot_desc, "&+W[%2d] %-20s &+W%-9s   %2d    %s", sl, 
              ship->slot[sl].get_description(), ship->slot[sl].get_position_str(), ship->slot[sl].val1, ship->slot[sl].get_status_str());
      }
      else
      {
          sprintf(slot_desc, "&+W[%2d] %-20s &+W%-9s   &+L**    %s", sl,
              ship->slot[sl].get_description(), ship->slot[sl].get_position_str(), ship->slot[sl].get_status_str());
      }
  }
  else
  {
    strcpy(slot_desc, " ");
  }
  return slot_desc;
}

int look_ship(P_char ch, P_ship ship)
{
    char target_str[100];
    P_ship target = ship->target;
    if ((target != NULL) && (IS_SET(target->flags, LOADED)))
        sprintf(target_str, "Target: &+W[%s]&N: %s", target->id, target->name);
    else
        sprintf(target_str, " ");
    
    sprintf(buf, "%s\r\n", ship->name);
    send_to_char(buf, ch);
    send_to_char("&+L-========================================================================-&N\r\n", ch);
    sprintf(buf, "&+LCaptain: &+W%-20s &+rFrags: &+W%-31d&+LID[&+Y%s&+L]\r\n\r\n",
            SHIPOWNER(ship), ship->frags, SHIPID(ship));
    send_to_char(buf, ch);

    sprintf(buf, "        %s%3d&N/&+G%-3d      &+LSpeed Range: &+W0-%-3d&+L            Sails: &+W%-20s&N\r\n",
            SHIPARMORCOND(SHIPMAXFARMOR(ship), SHIPFARMOR(ship)),
            SHIPFARMOR(ship), SHIPMAXFARMOR(ship),
            ship->get_maxspeed(), ship_crew_data[ship->sailcrew.index].name);
    send_to_char(buf, ch);

    sprintf(buf, "                          &+LWeight: &+W%3d,000   &+L        Guns: &+W%-20s&N\r\n",
            SHIPHULLWEIGHT(ship), ship_crew_data[ship->guncrew.index].name);
    send_to_char(buf, ch);

    sprintf(buf, "           &+y||&N               &+LLoad: &+W%3d/&+W%3d&+L         Repair: &+W%-20s&N\r\n",
            SHIPSLOTWEIGHT(ship), SHIPMAXWEIGHT(ship), ship_crew_data[ship->repaircrew.index].name);
    send_to_char(buf, ch);

    sprintf(buf, "          &+y/..\\&N        &+LPassengers: &+W%2d/%2d&N\r\n",
            num_people_in_ship(ship), ship->get_capacity());
    send_to_char(buf, ch);

    sprintf(buf, "         &+y/.%s%2d&+y.\\        &N\r\n",
            SHIPINTERNALCOND(SHIPMAXFINTERNAL(ship), SHIPFINTERNAL(ship)),
            SHIPFINTERNAL(ship));
    send_to_char(buf, ch);

    send_to_char("        &+y/..&N--&+y..\\        &+LNum  Name                 Position   Ammo   Status&N\r\n", ch);

    sprintf(buf, "        &+y|..&+g%2d&+y..|        %s&N\r\n", SHIPMAXFINTERNAL(ship), generate_slot(ship, 0));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|......| &+g%3d    %s&N\r\n", ship->mainsail, generate_slot(ship, 1));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y\\__..__/ &N---    %s&N\r\n", generate_slot(ship, 2));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|..||..|&+L/&N&+g%3d    %s&N\r\n", SHIPMAXSAIL(ship), generate_slot(ship, 3));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|......&+L/        %s&N\r\n", generate_slot(ship, 4));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|.....&+L/&N&+y|        %s&N\r\n", generate_slot(ship, 5));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|....&+L/&N&+y.|        %s&N\r\n", generate_slot(ship, 6));
    send_to_char(buf, ch);

    sprintf(buf, "    %s%3d &N&+y|%s%2d&+y.&+L/&N%s%2d&+y| %s%3d    %s&N\r\n",
            SHIPARMORCOND(SHIPMAXPARMOR(ship), SHIPPARMOR(ship)), SHIPPARMOR(ship),
            SHIPINTERNALCOND(SHIPMAXPINTERNAL(ship), SHIPPINTERNAL(ship)), SHIPPINTERNAL(ship),
            SHIPINTERNALCOND(SHIPMAXSINTERNAL(ship), SHIPSINTERNAL(ship)), SHIPSINTERNAL(ship), 
            SHIPARMORCOND(SHIPMAXSARMOR(ship), SHIPSARMOR(ship)), SHIPSARMOR(ship), 
            generate_slot(ship, 7));
    send_to_char(buf, ch);

    sprintf(buf, "    &N--- &+y|&N--&+Y/\\&N--&+y| &N---    %s&N\r\n", generate_slot(ship, 8));
    send_to_char(buf, ch);

    sprintf(buf, "    &+G%3d &N&+y|&+g%2d&+Y\\/&N&+g%2d&+y| &+G%3d    %s&N\r\n",
            SHIPMAXPARMOR(ship), SHIPMAXPINTERNAL(ship),
            SHIPMAXSINTERNAL(ship), SHIPMAXSARMOR(ship), generate_slot(ship, 9));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|......|        %s&N\r\n", generate_slot(ship, 10));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|__||__|        %s&N\r\n", generate_slot(ship, 11));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y/......\\        %s&N\r\n", generate_slot(ship, 12));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|......|        %s&N\r\n", generate_slot(ship, 13));
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|..%s%2d&+y..|      %s&N\r\n",
            SHIPINTERNALCOND(SHIPMAXRINTERNAL(ship), SHIPRINTERNAL(ship)),
            SHIPRINTERNAL(ship), target_str);
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|..&N--&+y..|&N        %s&N\r\n",
            SHIPSINKING(ship) ? "&=LRSINKING!!&N" :
            SHIPIMMOBILE(ship) ? "&+RIMMOBILE&N" : 
            SHIPISDOCKED(ship) ? "&+yDOCKED&N" : "");
    send_to_char(buf, ch);

    sprintf(buf, "        &+y|..&+g%2d&+y..|   &NCrew Stamina: %s%-3d&N Repair Materials: &+W%d&N\r\n",
            SHIPMAXRINTERNAL(ship), 
            SHIPARMORCOND(ship->guncrew.max_stamina, ship->guncrew.stamina), ship->guncrew.stamina, 
            ship->repair);
    send_to_char(buf, ch);

    sprintf(buf, "        &+y\\______/    &NSet Heading: &+W%-3d   &NSet Speed: &+W%-4d&N\r\n",
            ship->setheading, ship->setspeed);
    send_to_char(buf, ch);

    sprintf(buf, "                        &NHeading: &+W%-3d       &NSpeed: &+W%-4d&N\r\n", 
            ship->heading, ship->speed);
    send_to_char(buf, ch);

    sprintf(buf, "        %s%3d&N/&+G%-3d&N\r\n",
            SHIPARMORCOND(SHIPMAXRARMOR(ship), SHIPRARMOR(ship)),
            SHIPRARMOR(ship), SHIPMAXRARMOR(ship));
    send_to_char(buf, ch);
    return TRUE;
}

int claim_coffer(P_char ch, P_ship ship)
{
    if (ship->money == 0) 
    {
        send_to_char("The ship's coffers are empty!\r\n", ch);
        return TRUE;
    }
    sprintf(buf, "You get %s from the ship coffers.\r\n", coin_stringv(ship->money));
    send_to_char(buf, ch);
    ADD_MONEY(ch, ship->money);
    ship->money = 0;
    return TRUE;
}

int newship_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
    //if(!get_property("ship.toggle.master", 1.000));
    //    return false;

    if (!ch && !cmd)
        return TRUE;

    if (cmd == -10)
        return TRUE;


    if (!ch)
        return(FALSE);

    if (cmd != CMD_GET && cmd != CMD_ORDER && cmd != CMD_SCAN 
        && cmd != CMD_FIRE && cmd != CMD_LOCK && cmd != CMD_LOOK)
        return FALSE;

    P_ship ship = NULL;

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (svs->panel == obj)
        {
            ship = svs;
            break;
        }
    }
    
    
    if (ship == NULL) 
    {
        send_to_char("Not valid ship or error in ship code.\r\n", ch);
        return FALSE;
    }
    if (!IS_SET(ship->flags, LOADED))
    {
        send_to_char("Error: ship is not loaded!\r\n", ch);
        return FALSE;
    }

    if ((cmd == CMD_GET) && arg) 
    {
        if (isname(arg, "coins money")) 
        {
            return claim_coffer(ch, ship);
        }
        return FALSE;
    }
    if ((cmd == CMD_ORDER) && arg) 
    {
        half_chop(arg, arg1, tmp_str);
        half_chop(tmp_str, arg2, arg3);
      
        if (!(isname(arg1, "sail sa") ||
              isname(arg1, "jettison j") ||
              isname(arg1, "undock") ||
              isname(arg1, "maneuver") || isname(arg1, "m") ||
              isname(arg1, "anchor") ||
              isname(arg1, "ram") ||
              isname(arg1, "heading") || isname(arg1, "h") ||
              isname(arg1, "speed") || isname(arg1, "s") ||
              isname(arg1, "signal")))
        {
            do_order(ch, arg, cmd);
            return TRUE;
        }
      
        if (!isname(GET_NAME(ch), SHIPOWNER(ship)) && !IS_TRUSTED(ch) &&
            (ch->group == NULL ? 1 : get_char2(str_dup(SHIPOWNER(ship))) ==
             NULL ? 1 : (get_char2(str_dup(SHIPOWNER(ship)))->group != ch->group))) 
        {
            send_to_char ("You are not the captain of this ship, the crew ignores you.\r\n", ch);
            return TRUE;
        }
        if (IS_SET(ship->flags, MAINTENANCE)) 
        {
            sprintf(buf, "This ship is being worked on for at least another %.1f hours, it can't move.\r\n",
                    (float) ship->timer[T_MAINTENANCE] / 75.0);
            send_to_char(buf, ch);
            return TRUE;
        }
        if (ship->get_capacity() < num_people_in_ship(ship)) 
        {
            send_to_char ("Arrgh! There are too many people on this ship to move!\r\n", ch);
            return TRUE;
        }
        if (SHIPSINKING(ship)) 
        {
            send_to_char("&+RYou cannot control the ship while it's sinking!&N\r\n", ch);
            return TRUE;
        }
        if (ship->timer[T_MINDBLAST] > 0) 
        {
            send_to_char("&+RYour crew members crawl around and ignore your orders!&N\r\n", ch);
            return TRUE;
        }

        if (isname(arg1, "sail sa")) 
        {
            return order_sail(ch, ship, arg2, arg3);
        }
        if (isname(arg1, "jettison j"))
        {
            if (isname(arg2, "cargo")) 
            {
                return jettison_cargo(ch, ship, arg3);
            } 
            else if (isname(arg2, "contraband")) 
            {
                return jettison_contraband(ch, ship, arg3);
            }
            send_to_char("Valid syntax: order jettison <cargo/contraband> [<number of crates>]\r\n", ch);
            return TRUE;
        }
        if (isname(arg1, "undock")) 
        {
            return order_undock(ch, ship);
        }
        if (isname(arg1, "maneuver") || isname(arg1, "m")) 
        {
            return order_maneuver(ch, ship, arg2);
        }
        if (isname(arg1, "anchor")) 
        {
            return order_anchor(ch, ship);
        }
        if (isname(arg1, "ram")) 
        {
            return order_ram(ch, ship, arg2);
        }

        if (isname(arg1, "heading") || isname(arg1, "h")) 
        {
            return order_heading(ch, ship, arg2);
        }

        if (isname(arg1, "speed") || isname(arg1, "s")) 
        {
            return order_speed(ch, ship, arg2);
        }

        if (isname(arg1, "signal")) 
        {
            return order_signal(ch, ship, arg2, arg3);
        }

        return FALSE;
    }

    if (cmd == CMD_SCAN) 
    {
        return do_scan(ch, ship, arg);
    }

    if (cmd == CMD_FIRE) 
    {
        return do_fire(ch, ship, arg);
    }

    if (cmd == CMD_LOCK) 
    {
        return lock_target(ch, ship, arg);
    }

    if ((cmd == CMD_LOOK) && arg) 
    {
        half_chop(arg, arg1, tmp_str);
        half_chop(tmp_str, arg2, arg3);

        if (isname(arg1, "cargo")) 
        {
            return look_cargo(ch, ship);
        }
        if (isname(arg1, "crew")) 
        {
            return look_crew(ch, ship);
        }
        if (isname(arg1, "sight weapon")) 
        {
            return look_weapon(ch, ship, arg2);
        }
        if (isname(arg1, "tactical") || isname(arg1, "t")) 
        {
            return look_tactical_map(ch, ship, arg2, arg3);
        }
        if (isname(arg, "contacts") || isname(arg, "c")) 
        {
            return look_contacts(ch, ship);
        }
        if (isname(arg, "weaponspec")) 
        {
            return look_weaponspec(ch, ship);
        }
        if (isname(arg, "show"))
        {
            update_ship_status(ship);
            ship->show(ch);
            return TRUE;
        }
        if (isname(arg, "status") || isname(arg, "ship")) 
        {
            return look_ship(ch, ship);
        }
        return FALSE;
    }
    return FALSE;
}

int fire_arc(P_ship ship, P_char ch, int arc)
{
    int fired = 0;
    for (int i = 0; i < MAXSLOTS; i++)
    {
        if (ship->slot[i].type == SLOT_WEAPON && ship->slot[i].position == arc)
        {
            if (SHIPANCHORED(ship)) 
                return TRUE;
            fire_weapon(ship, ch, i);
            fired++;
        }
    }
    if (fired == 0)
        send_to_char("No weapons installed on this arc!\r\n", ch);
    return TRUE;
}




int fire_weapon(P_ship ship, P_char ch, int w_num)
{
    if (w_num < 0 || w_num >= MAXSLOTS)
    {
        send_to_char("Invalid weapon!\r\n", ch);
        return TRUE;
    }
    if (ship->slot[w_num].type != SLOT_WEAPON)
    {
        send_to_char("Invalid weapon!\r\n", ch);
        return TRUE;
    }

    int j;
    int k = getcontacts(ship);
    for (j = 0; j < k; j++) 
    {
        if (contacts[j].ship == ship->target) 
            break;
    }
    if (j == k)
    {
        send_to_char("Target out of sight!\r\n", ch);
        return TRUE;
    }

    int w_index = ship->slot[w_num].index;
    float range = contacts[j].range;
    if (range > (float) weapon_data[w_index].max_range)
    {
        send_to_char("Out of Range!\r\n", ch);
        return TRUE;
    }
    if (range < (float) weapon_data[w_index].min_range) 
    {
        send_to_char("You're too close to use this weapon!\r\n", ch);
        return TRUE;
    }
    if (getarc(ship->heading, contacts[j].bearing) != ship->slot[w_num].position) 
    {
        send_to_char("Target is not in weapon's firing arc!\r\n", ch);
        return TRUE;
    }
    if (SHIPWEAPONDESTROYED(ship, w_num)) 
    {
        send_to_char("Weapon is destroyed!\r\n", ch);
        return TRUE;
    }
    if (SHIPWEAPONDAMAGED(ship, w_num)) 
    {
        send_to_char("Weapon is damaged!\r\n", ch);
        return TRUE;
    }
    if (ship->slot[w_num].timer > 0) 
    {
        send_to_char("Weapon is still reloading.\r\n", ch);
        return TRUE;
    }
    if (ship->slot[w_num].val1 == 0) 
    {
        send_to_char("Out of Ammo!\r\n", ch);
        return TRUE;
    }

    P_ship target = ship->target;

    // calculating hit chance/displaying firing messages
    int hit_chance = weaponsight(ch, ship, target, w_num, range);
    sprintf(buf, "Your ship fires &+W%s&N at &+W[%s]&N:%s! Chance to hit: &+W%d%%&N",
            ship->slot[w_num].get_description(), ship->target->id, ship->target->name, hit_chance);
    act_to_all_in_ship(ship, buf);
    sprintf(buf, "&+W[%s]&N:%s&N fires %s at your ship!",
            SHIPID(ship), ship->name, ship->slot[w_num].get_description());
    act_to_all_in_ship(target, buf);
    sprintf(buf, "%s&N fires %s at %s!",
            ship->name, ship->slot[w_num].get_description(), ship->target->name);
    act_to_outside(ship, buf);
    sprintf(buf, "&+W[%s]&N:%s&N fires %s at &+W[%s]&N:%s!",
            SHIPID(ship), ship->name, ship->slot[w_num].get_description(), SHIPID(target), SHIPNAME(target));
    act_to_outside_ships(ship, buf, target);


    // setting volley
    float volley_time = (range / (float)weapon_data[w_index].max_range) * (float)weapon_data[w_index].volley_time;
    volley_time *= 0.9 + 0.01 * (float) number(0, 20);
    if (volley_time < 1.0)
        volley_time = 1.0;

    VolleyData vd;
    vd.attacker = ship;
    vd.target = target;
    vd.weapon_index = w_num;
    vd.hit_chance = hit_chance;
    add_event(volley_hit_event, (int)volley_time, NULL, NULL, NULL, 0, (void*)&vd, sizeof(VolleyData));


    ship->timer[T_BSTATION] = BSTATION;

    // initiating reload
    if (ship->slot[w_num].val1 == 1) 
        send_to_char("&+RWarning! This is the last round of ammo!\r\n", ch);
    if (ship->slot[w_num].val1 > 0)
       ship->slot[w_num].val1--;
    ship->slot[w_num].timer = MAX(1, (int)((float)weapon_data[w_index].reload_time * (1.0 - ship->guncrew.skill_mod * 0.15)));

    // reducing crew stamina
    ship->guncrew.stamina -= weapon_data[ship->slot[w_num].index].reload_stamina;
    if (ship->guncrew.stamina <= 0) 
    {
        force_anchor(ship);
    } 
    else if (ship->guncrew.stamina <= 20) 
    {
        act_to_all_in_ship(ship, "&+RWarning, Crew stamina critical!&N\r\n");
    }
    return TRUE;
}



// Ship Loader Proc is added to 1223 ship loader wand.
int shiploader_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
    return FALSE; // disabling ship loader wand for now  -Odorf


    /*int      i, j, numb;
    P_ship temp;

    if (cmd == -10) {
        return TRUE;
    }

    if (!ch && !cmd) {
        return TRUE;
    }

    if (!ch)
        return(FALSE);

    if (!OBJ_WORN_POS(obj, HOLD))
        return(FALSE);

    if ((cmd != CMD_LIST) && (cmd != CMD_REMOVE) && (cmd != CMD_TAP))
        return FALSE;

    half_chop(arg, arg1, arg2);
    temp = newships;
    numb = 0;
    while (temp != NULL) {
        temp = temp->next;
        numb += 1;
    }
    if (arg && (cmd == CMD_LIST)) 
    {
        if (isname(arg, "economy e")) 
                {
            send_to_char("---===Current Ship Economy Status===---\r\n", ch);
            sprintf(buf, "%-10s", "");
            for (j = 0; j < MAXCARGO; j++) 
            {
                sprintf(buf2, " %-10s", cargo_data[j].loc_name);
                strcat(buf, buf2);
            }
            strcat(buf, " Buy");
            strcat(buf, "\r\n");
            send_to_char(buf, ch);
            for (i = 0; i < MAXCARGO; i++) 
            {
                sprintf(buf, "%-10s", cargo_data[i].loc_name);
                for (j = 0; j < MAXCARGO; j++) 
                {
                    if (i != j) 
                    {
                        sprintf(buf2, " %-10d", (int) shipcargo[i].sell[j]);
                        strcat(buf, buf2);
                    } 
                    else 
                    {
                        strcat(buf, " N/A       ");
                    }
                }
                sprintf(buf2, " %-3d", (int) shipcargo[i].buy[i]);
                strcat(buf, buf2);
                strcat(buf, "\r\n");
                send_to_char(buf, ch);
            }
            return TRUE;
        }
        if (isname(arg, "ships")) 
        {
            temp = newships;
            send_to_char("&+y-============Current ships in game============-\r\n",
                         ch);
            while (temp != NULL) {
                sprintf(buf, "[%s]: %-20s %-10s %d %d\r\n", temp->id,
                        strip_ansi(temp->name).c_str(), temp->ownername,
                        world[temp->location].number, temp->bridge);
                send_to_char(buf, ch);
                temp = temp->next;
            }
            return TRUE; 
        }
        return FALSE;
    }
    if (arg && (cmd == CMD_REMOVE)) 
    {
        if (isname(arg1, "ship")) {
            temp = newships;
            while (temp != NULL) 
            {
                if (IS_SET(temp->flags, LOADED)) 
                {
                    if (isname(arg2, temp->ownername)) 
                    {
                        sprintf(buf, "Ship: [%s]: %s has been deleted.\r\n", SHIPID(temp), SHIPNAME(temp));
                        send_to_char(buf, ch);
                        deleteship(temp);
                        return TRUE;
                    }
                }
                temp = temp->next;
            }
            send_to_char("Ship not found.\r\n", ch);
            return TRUE;
        } else {
            return FALSE;
        }
    }
    if (arg && (cmd == CMD_TIME)) 
    {
        if (isname(arg, "mud")) 
        {
            sprintf(buf, "%d", (int) time(NULL));
            send_to_char(buf, ch);
            return TRUE;
        }
        return FALSE;
    }
    if (arg && (cmd == CMD_TAP)) 
    {
        half_chop(arg, arg1, arg2);
        if (IS_TRUSTED(ch) && isname(arg1, "log10")) {
            if (!is_number(arg2)) {
                send_to_char("Not a number fool!\r\n", ch);
            }
            i = 0;
            i = atoi(arg2);
            sprintf(buf, "Log10 of %d == %f\r\n", i, log10((double) i));
            send_to_char(buf, ch);
            return TRUE;
        }
        if (IS_TRUSTED(ch) && isname(arg1, "random")) {
            sprintf(buf, "Random number: %10.8f\r\n", genrand_real2());
            send_to_char(buf, ch);
            return TRUE;
        }
        if (IS_TRUSTED(ch) && isname(arg1, "frag")) {
            half_chop(arg2, arg1, arg2);
            if (!is_number(arg1)) {
                send_to_char("Invalid syntax.\r\n", ch);
                return TRUE;
            }
            if (!*arg2) {
                send_to_char("Invalid Syntax!\r\n", ch);
                return TRUE;
            }
            i = 0;
            temp = newships;
            while (temp != NULL) {
                if (isname(arg2, temp->ownername)) {
                    i = 1;
                    break;
                }
                temp = temp->next;
            }
            if (i != 1) {
                send_to_char("Ship not found.\r\n", ch);
                return TRUE;
            }
            i = atoi(arg1);
            if (i < 0) {
                i = 0;
            }
            temp->frags = i;
            send_to_char("Done.\r\n", ch);
            if (write_newship(temp)) {
                send_to_char("Saved.\r\n", ch);
            } else {
                send_to_char("Error saving file!\r\n", ch);
            }
            return TRUE;
        }
        if (IS_TRUSTED(ch) && isname(arg1, "crew")) {
            half_chop(arg2, arg1, arg2);
            if (isname(arg1, "sail s")) {
                half_chop(arg2, arg1, arg2);
                if (is_number(arg1)) {
                    i = atoi(arg1);
                    if (i < 0 || i >= MAXSAILCREW) {
                        send_to_char("Invalid crew number, die.\r\n", ch);
                        return TRUE;
                    }
                    if (!*arg2) {
                        send_to_char("So close, yet so far, invalid syntax loser.\r\n",
                                     ch);
                        return TRUE;
                    }

                    temp = newships;
                    if (isname(arg2, "all")) {
                        while (temp != NULL) {
                            setcrew(temp, i, SAIL_CREW);
                            temp = temp->next;
                        }
                    }
                    while (temp != NULL) {
                        if (isname(arg2, temp->ownername)) {
                            break;
                        }
                        temp = temp->next;
                    }
                    setcrew(temp, i, SAIL_CREW);
                    return TRUE;
                } else {
                    send_to_char("Invalid syntax, DIE!\r\n", ch);
                    return TRUE;
                }
            } else if (isname(arg1, "gun g")) {
                half_chop(arg2, arg1, arg2);
                if (is_number(arg1)) {
                    i = atoi(arg1);
                    if (i < 0 || i >= MAXGUNCREW) {
                        send_to_char("Invalid crew number, die.\r\n", ch);
                        return TRUE;
                    }
                    if (!*arg2) {
                        send_to_char("So close, yet so far, invalid syntax loser.\r\n",
                                     ch);
                        return TRUE;
                    }
                    temp = newships;
                    if (isname(arg2, "all")) {
                        while (temp != NULL) {
                            setcrew(temp, i, GUN_CREW);
                            temp = temp->next;
                        }
                    }
                    while (temp != NULL) {
                        if (isname(arg2, temp->ownername)) {
                            break;
                        }
                        temp = temp->next;
                    }
                    setcrew(temp, i, GUN_CREW);
                    return TRUE;
                } else {
                    send_to_char("Invalid syntax, DIE!\r\n", ch);
                    return TRUE;
                }
            }
            send_to_char("INVALID SYNTAX ASSHOLE!\r\n", ch);
            return TRUE;
        }
    }
    return FALSE;*/
}


void finish_sinking(P_ship ship)
{
    act_to_all_in_ship(ship, "&+yYour ship sinks and you swim out in time!\r\n");
    act_to_outside(ship, "&+yA ship has sunk to the depths of the ocean!\r\n");
    sprintf(buf, "&+W[%s]:&N %s&N&+y sinks under the ocean.\r\n", ship->id, ship->name);
    act_to_outside_ships(ship, buf, ship);
    everyone_get_out_newship(ship);

    int insurance = 0;
    if (ship->m_class > 0 && ship->m_class < MAXSHIPCLASSMERCHANT) // no insurance for sloops
        insurance = (int)(SHIPTYPECOST(ship->m_class) * 0.75);
    else if (ship->m_class >= MAXSHIPCLASSMERCHANT)
        insurance = (int)(SHIPTYPECOST(ship->m_class) * 0.50);  // only partial insurance for warships

    if (P_char owner = get_char2(str_dup(SHIPOWNER(ship))))
    {
        GET_BALANCE_PLATINUM(owner) += insurance / 1000;
    }
    else
    {
        ship->money = insurance; // if owner is not online, money go into ships coffer
    }




    ship->m_class = 0; // all ships become sloops after sinking
    setarmor(ship, true);
    ship->mainsail = SHIPTYPEMAXSAIL(ship->m_class);

    clear_ship_layout(ship);
    set_ship_layout(ship, ship->m_class);
    reset_ship_physical_layout(ship);
    nameship(ship->name, ship);

    ship->timer[T_UNDOCK] = 0; 
    ship->timer[T_MANEUVER] = 0; 
    ship->timer[T_SINKING] = 0; 
    ship->timer[T_BSTATION] = 0; 
    ship->timer[T_RAM] = 0; 
    ship->timer[T_RAM_WEAPONS] = 0; 
    ship->timer[T_MAINTENANCE] = 0; 
    ship->timer[T_MINDBLAST] = 0; 
	 	     
    if(IS_SET(ship->flags, MAINTENANCE)) 
        REMOVE_BIT(ship->flags, MAINTENANCE); 
    if(IS_SET(ship->flags, SINKING)) 
        REMOVE_BIT(ship->flags, SINKING); 
    if(IS_SET(ship->flags, RAMMING)) 
        REMOVE_BIT(ship->flags, RAMMING); 
    if(IS_SET(ship->flags, ANCHOR)) 
        REMOVE_BIT(ship->flags, ANCHOR); 
    if(IS_SET(ship->flags, SUMMONED)) 
        REMOVE_BIT(ship->flags, SUMMONED); 

    for (int j = 0; j < MAXSLOTS; j++)
        ship->slot[j].clear();

    ship->location = real_room0(ship->anchor);
    obj_from_room(ship->shipobj);
    obj_to_room(ship->shipobj, real_room0(ship->anchor));
    dock_ship(ship, real_room0(ship->anchor));
    ship->speed = 0;
    ship->setspeed = 0;
    reset_crew_stamina(ship);
    update_ship_status(ship);
    write_newship(ship);
    return;
}

// This proc is added to all ship objects.
int shipobj_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  char   name[MAX_INPUT_LENGTH];
  P_obj  obj_entered;
  P_ship ship;

  /* check for periodic event calls */
  if (cmd == -10)
      return FALSE;

  if (cmd != CMD_ENTER)
      return(FALSE);

  one_argument(arg, name);
  obj_entered = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
  if (obj_entered != obj)
      return(FALSE);

  if (!obj || (obj->type != ITEM_SHIP)) 
  {
      return(FALSE);
  } 
  else 
  {
    ship = shipObjHash.find(obj);
    if (ship == NULL)
        return FALSE;

      
    if (ship->m_class >= MAXSHIPCLASSMERCHANT && ship->speed > 0)
    {
       send_to_char("&+RThat ship is moving too fast to board!&n\r\n", ch);
       return TRUE;
    }
    else if (ship->speed > BOARDING_SPEED)
    {
       send_to_char("&+RThat ship is moving too fast to board!&n\r\n", ch);
       return TRUE;
    }
    else if (ship->m_class == SH_FRIGATE || ship->m_class == SH_DREADNOUGHT)
    {
//            if (SHIPISDOCKED(ship) || SHIPANCHORED(ship))
      {
        act("$n enters through the docking bay of $p.", TRUE, ch, ship->shipobj, 0, TO_ROOM);
        char_from_room(ch);
        act("You step through the docking bay of $p.", FALSE, ch, ship->shipobj, 0, TO_CHAR);
        char_to_room(ch, real_room0(ship->entrance), 0);
        act("$n steps through the docking bay.", TRUE, ch, 0, 0, TO_ROOM);
        return TRUE;
      }
//            else
//            {
//                send_to_char
//                ("This ship's hatches are tightly shut and the armor is too tough to break through!\r\n",
//                 ch);
//                return TRUE;
//            }
    }
    else 
    {
        act("$n boards $p.", TRUE, ch, ship->shipobj, 0, TO_ROOM);
        char_from_room(ch);
        act("You board $p.", FALSE, ch, ship->shipobj, 0, TO_CHAR);
        char_to_room(ch, real_room0(ship->entrance), 0);
        act("$n comes aboard.", TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  return(TRUE);
}

void newship_activity()
{
    int        j, k, loc;
    float        rad;

    //if(!get_property("ship.toggle.master", 1.000));
    //    return;
        
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        P_ship ship = svs;

        // Check timers
        if (!IS_SET(ship->flags, LOADED)) 
            continue;

        if (ship->timer[T_RAM_WEAPONS] == 1)
        {
            act_to_all_in_ship(ship, "Your gun crew has recovered from ram impact.&N");
        }
        if (ship->timer[T_RAM] == 1)
        {
            act_to_all_in_ship(ship, "Your ship has recovered from ram impact.&N");
        }
        if (ship->timer[T_MINDBLAST] == 1) 
        {
            act_to_all_in_ship(ship, "Your crew has recovered from mental shock.&N\r\n");
        }

        for (j = 0; j < MAXTIMERS; j++) 
        {
            if (ship->timer[j] > 0) 
                ship->timer[j] -= 1;
        }

        if (!IS_SET(ship->flags, SINKING)) 
        {
            // STAMINA REGEN
            if (ship->sailcrew.stamina < ship->sailcrew.max_stamina) 
                ship->sailcrew.stamina += 1;
            if (ship->guncrew.stamina < ship->guncrew.max_stamina) 
                ship->guncrew.stamina += 1;
            if (ship->repaircrew.stamina < ship->repaircrew.max_stamina) 
                ship->repaircrew.stamina += 1;
            if (ship->rowingcrew.stamina < ship->rowingcrew.max_stamina) 
                ship->rowingcrew.stamina += 1;

            // Battle Stations!
            if (ship->target != NULL)
                ship->timer[T_BSTATION] = BSTATION;
            if (ship->timer[T_BSTATION] == 1)
                act_to_all_in_ship(ship, "Your crew stands down from battlestations.\r\n");

            // Repairing
            if ((ship->repair > 0) && ship->timer[T_MINDBLAST] == 0) 
            {
                float chance = 0.0;
                if (ship->mainsail < (int)((float)SHIPMAXSAIL(ship) * (ship->repaircrew.skill_mod + 0.1)) && 
                        ship->mainsail < SHIPMAXSAIL(ship) * 0.9)
                {
                    if (SHIPANCHORED(ship))
                    {
                        if (ship->mainsail > 0) 
                                chance = 500.0;
                        else if (ship->timer[T_BSTATION] == 0) 
                                chance = 100.0;
                        else
                                chance = 30.0;
                    }
                    else
                    {
                        if (ship->mainsail > 0) chance = 30.0;
                        else                                        chance = 0.0;
                    }
                    chance *= (1.0 + ship->repaircrew.skill_mod);
                    if (number (0, 999) < (int)chance)
                    {
                        ship->mainsail++;
                        ship->repair--;
                        if (ship->repair == 0)
                            act_to_all_in_ship(ship, "&+RThe ship is out of repair materials!.&N");
                        update_ship_status(ship);
                    }
                } 
                for (j = 0; j < 4; j++) 
                {
                    if (ship->repair < 1)
                        break;
                    if (ship->internal[j] < (int)((float)ship->maxinternal[j] * (ship->repaircrew.skill_mod + 0.1)) &&
                            ship->internal[j] < ship->maxinternal[j] * 0.9) 
                    {
                        if (SHIPANCHORED(ship))
                        {
                            if (ship->timer[T_BSTATION] == 0) 
                            {
                                if (ship->internal[j] > 0) chance = 250.0;
                                else                       chance = 100.0;
                            }
                            else
                            {
                                if (ship->internal[j] > 0) chance = 100.0;
                                else                       chance = 10.0;
                            }
                        }
                        else
                        {
                            if (ship->internal[j] > 0) chance = 30.0;
                            else                       chance = 0.0;
                        }
                        chance *= (1.0 + ship->repaircrew.skill_mod);
                        if (number (0, 999) < (int)chance)
                        {
                            ship->internal[j]++;
                            ship->repair--;
                            if (ship->repair == 0)
                                act_to_all_in_ship(ship, "&+RThe ship is out of repair materials!.&N");
                            update_ship_status(ship);
                        }
                    }
                }
                for (j = 0; j < MAXSLOTS; j++) 
                {
                    if (ship->repair < 1)
                        break;
                    if (ship->slot[j].type == SLOT_WEAPON)
                    {
                        if (SHIPWEAPONDAMAGED(ship, j) && !SHIPWEAPONDESTROYED(ship, j))
                        {
                            chance = 200.0;
                            chance *= (1.0 + ship->repaircrew.skill_mod);
                            if (number (0, 999) < (int)chance)
                            {
                                ship->slot[j].val2--;
                                if (!SHIPWEAPONDAMAGED(ship, j))
                                {
                                    sprintf(buf, "&+W%s &+Ghas been repaired!&N", weapon_data[ship->slot[j].index].name);
                                    act_to_all_in_ship(ship, buf);
                                    ship->slot[j].timer = (int)((float)weapon_data[j].reload_time * (1.0 - ship->guncrew.skill_mod * 0.15));
                                }
                                if (ship->repair == 0)
                                    act_to_all_in_ship(ship, "&+RThe ship is out of repair materials!.&N");
                            }
                        }
                    }
                }
            }
        }

        // SINKING!!!!        
        if (IS_SET(ship->flags, SINKING)) 
        {
            if (ship->timer[T_SINKING] == 0) 
            {
                finish_sinking(ship);
                return;
            }
        }

        // Undocking
        if (ship->timer[T_UNDOCK] > 0) 
        {
            if (ship->timer[T_UNDOCK] == 27)
                act_to_all_in_ship(ship, "&+LThe crew begins raising the anchor.&N");
            else if (ship->timer[T_UNDOCK] == 21)
                act_to_all_in_ship(ship, "&+WThe crew finishes raising the anchor.&N");
            else if (ship->timer[T_UNDOCK] == 17)
            {
                if (SHIPANCHORED(ship) && !SHIPISDOCKED(ship))
                {
                    act_to_all_in_ship(ship, "&+GThe crew scrambles to their stations, the ship is ready to go.&N");
                    REMOVE_BIT(ship->flags, ANCHOR);
                    ship->timer[T_UNDOCK] = 0;
                    update_ship_status(ship);
                }
                else
                    act_to_all_in_ship(ship, "&+yThe crew scrambles to their stations.&N");
            }
            else if (ship->timer[T_UNDOCK] == 12)
                act_to_all_in_ship(ship, "&+WThe crew readies the sails.&N");
            else if (ship->timer[T_UNDOCK] == 9)
                act_to_all_in_ship(ship, "&+yThe first officer begins a checkup of all ship systems.&N");
            else if (ship->timer[T_UNDOCK] == 1) 
            {
                assignid(ship, NULL);
                act_to_all_in_ship(ship, "&+GThe first officer reports everything is in order and the ship is ready to go.&N");
                REMOVE_BIT(ship->flags, DOCKED);
                REMOVE_BIT(ship->flags, ANCHOR);
                update_crew(ship);
                update_ship_status(ship);
            }
        }

        // Maintenance Timer
        if (IS_SET(ship->flags, MAINTENANCE)) 
        {
            if (ship->timer[T_MAINTENANCE] == 0) 
            {
                REMOVE_BIT(ship->flags, MAINTENANCE);
            }
        }

        //Undocked and non sinking actions go below here
        if (!SHIPSINKING(ship) &&
            !SHIPISDOCKED(ship) &&
            !SHIPANCHORED(ship)) 
        {
            float accel_mod = 1.0 + ship->sailcrew.skill_mod;
            float turn_mod = 1.0 + ship->sailcrew.skill_mod;
            // Setspeed to Speed
            if (ship->setspeed > ship->get_maxspeed()) 
            {
                ship->setspeed = ship->get_maxspeed();
            }
            if (ship->setspeed != ship->speed && ship->timer[T_MINDBLAST] == 0) 
            {
                j = SHIPTYPESPGAIN(ship->m_class);
                j = (int) ((float)j * accel_mod);
    
                if (ship->setspeed > ship->speed) 
                {
                    ship->speed += j;
                    if (ship->setspeed < (ship->speed + j)) 
                    {
                        ship->speed = ship->setspeed;
                    }
                    if (ship->speed > ship->get_maxspeed()) 
                    {
                        ship->speed = ship->get_maxspeed();
                    }
                }
                if (ship->setspeed < ship->speed) 
                {
                    ship->speed -= j;
                    if (ship->setspeed > (ship->speed - j)) 
                    {
                        ship->speed = ship->setspeed;
                    }
                    if (ship->speed < 0) 
                    {
                        ship->speed = 0;
                    }
                }
            }

            //    SetHeading to Heading
            if(ship->setheading != ship->heading && ship->timer[T_MINDBLAST] == 0) 
            {
                if (SHIPIMMOBILE(ship))
                {
                    j = 2;
                }
                else
                {
                    j = (int) (ship->speed / 10);
                    j += (int) ((280 - SHIPHULLWEIGHT(ship)) / 10);
    
                    if (j < 5)
                        j = 5;

                    j = (int) ((float)j * turn_mod);
                }

                k = ship->heading;
                if (ship->heading < ship->setheading)
                    k = ship->heading + 360;
                if ((ship->setheading + 180) < k) 
                {
                    ship->heading = (ship->heading + j);
                    if (ship->heading > 359)
                        ship->heading -= 360;
                    if ((k + j) > (ship->setheading + 360))
                        ship->heading = ship->setheading;
                } 
                else 
                {
                    ship->heading = (ship->heading - j);
                    if (ship->heading < 0)
                        ship->heading += 360;
                    if ((k - j) < ship->setheading)
                        ship->heading = ship->setheading;
                }
            }

            // Movement Goes here
            if (ship->speed != 0) 
            {
                rad = (float) ((float) (ship->heading) * M_PI / 180.000);
                ship->x += (float) ((float) ship->speed * sin(rad)) / 150.000;
                ship->y += (float) ((float) ship->speed * cos(rad)) / 150.000;
                if ((ship->y >= 51.000) || (ship->x >= 51.000) || (ship->y < 50.000) || (ship->x < 50.000)) 
                {
                    getmap(ship);
                    loc = ship->location;
                    ship->location = tactical_map[(int) ship->x][100 - (int) ship->y].rroom;
                    if (world[ship->location].sector_type == SECT_OCEAN || IS_SET(ship->flags, AIR)) 
                    {
                        if (ship->x > 50.999) 
                        {
                            ship->x -= 1.000;
                            sprintf(buf, "%s sails east.\r\n", ship->name);
                            send_to_room(buf, tactical_map[50][50].rroom);
                            sprintf(buf, "%s sails in from the west.\r\n", ship->name);
                            send_to_room(buf, ship->location);
                        } 
                        else if (ship->x < 50.000) 
                        {
                            ship->x += 1.000;
                            sprintf(buf, "%s sails west.\r\n", ship->name);
                            send_to_room(buf, tactical_map[50][50].rroom);
                            sprintf(buf, "%s sails in from the east.\r\n", ship->name);
                            send_to_room(buf, ship->location);
                        }

                        if (ship->y > 50.999) 
                        {
                            ship->y -= 1.000;
                            sprintf(buf, "%s sails north.\r\n", ship->name);
                            send_to_room(buf, tactical_map[50][50].rroom);
                            sprintf(buf, "%s sails in from the south.\r\n", ship->name);
                            send_to_room(buf, ship->location);
                        } 
                        else if (ship->y < 50.000) 
                        {
                            ship->y += 1.000;
                            sprintf(buf, "%s sails south.\r\n", ship->name);
                            send_to_room(buf, tactical_map[50][50].rroom);
                            sprintf(buf, "%s sails in from the north.\r\n", ship->name);
                            send_to_room(buf, ship->location);
                        }
                        if (SHIPOBJ(ship) && (loc != ship->location)) 
                        {
                            obj_from_room(SHIPOBJ(ship));
                            obj_to_room(SHIPOBJ(ship), ship->location);
                            everyone_look_out_newship(ship);
                        }
                    } 
                    else if (loc != ship->location) 
                    {
                        ship->setspeed = 0;
                        ship->speed = 0;
                        ship->x = 50.500;
                        ship->y = 50.500;
                        SHIPLOCATION(ship) = ship->shipobj->loc.room;
                        if (ship->shipai) 
                        {
                            if (IS_SET(ship->shipai->flags, AIB_ENABLED)) 
                            {
                                if (IS_SET(ship->shipai->flags, AIB_AUTOPILOT)) 
                                {
                                    REMOVE_BIT(ship->shipai->flags, AIB_AUTOPILOT);
                                    REMOVE_BIT(ship->shipai->flags, AIB_ENABLED);
                                    act_to_all_in_ship(ship, "Your autopilot has stopped due to possible impact!");
                                }
                            }
                        }

                        int crash_chance = (ship->timer[T_BSTATION] == 0) ? 0 :
                            (int)((float)(ship->speed + 50) / (1.0 + ship->sailcrew.skill_mod * 2.0));

                        if (ship->timer[T_MINDBLAST] == 0)
                            act_to_all_in_ship(ship, "Your crew attempts to stop the ship from crashing into land!");
                        else
                            crash_chance = 100;

                        if (dice(2, 50) <= crash_chance)
                            crash_land(ship);
                        else
                            act_to_all_in_ship(ship, "Your crew manages to stop the ship from running ashore.");
                    }
                }
            }

            // Ramming
            if (IS_SET(ship->flags, RAMMING)) 
            {
                if (ship->target == NULL) 
                {
                    act_to_all_in_ship(ship, "&+WStanding down from ramming mode due to no target.&N");
                    REMOVE_BIT(ship->flags, RAMMING);
                }
                else if (ship->speed <= BOARDING_SPEED)
                {
                    act_to_all_in_ship(ship, "&+WStanding down from ramming mode due to low speed.&N");
                    REMOVE_BIT(ship->flags, RAMMING);
                }
                else if (ship->timer[T_MINDBLAST] == 0)
                {
                    k = getcontacts(ship);
                    for (j = 0; j < k; j++) 
                    {
                        if (contacts[j].ship == ship->target) 
                        {
                            if (contacts[j].range < 1.0) 
                            {
                                    try_ram_ship(ship, ship->target, j);
                            }
                        }
                    }
                }
            }


            // Weapon reloading
            if (!IS_SET(ship->flags, RAMMING) && ship->timer[T_RAM_WEAPONS] == 0 && ship->timer[T_MINDBLAST] == 0)
            {
                for (j = 0; j < MAXSLOTS; j++) 
                {
                    if (ship->slot[j].type == SLOT_WEAPON) 
                    {
                        if (ship->slot[j].timer > 0) 
                        {
                            ship->slot[j].timer--;
                            if (ship->slot[j].timer == 0) 
                            {
                                    sprintf(buf, "Weapon &+W[%d]&N: [%s] has finished reloading.", j, ship->slot[j].get_description());
                                    act_to_all_in_ship(ship, buf);
                            }
                        }
                    }
                }
            }

            shipai_activity(ship);
        }
    }
}

void crash_land(P_ship ship)
{
    act_to_all_in_ship(ship, "&+yCRUNCH!! Your ship crashes into land!&N");
    sprintf(buf, "&+W[%s]&N:%s&N crashes into land!", SHIPID(ship), ship->name);
    act_to_outside_ships(ship, buf, NULL);
    int hits = (SHIPHULLWEIGHT(ship) / 25) + 1;
    for (int k = 0; k < hits; k++) 
    {
        if (k == 0 || number(1, 2) == 2)
        {
            int arc = (k == 0) ? FORE : number(0, 5);
            int dam = number(1, 9);
            if (arc < 4)
            {
                /*sprintf(buf, " Your ship is hit for %d points of damage on the %s side!", dam, arc_name[arc]);
                act_to_all_in_ship(ship, buf);
                sprintf(buf, " &+W[%s]&N:%s&N has been hit for %d points of damage on the %s side!", SHIPID(ship), ship->name, dam, arc_name[arc]);
                act_to_outside_ships(ship, buf, NULL);*/
                damage_hull(NULL, ship, dam, arc, 0);
            }
            else  // sails
            {
                /*sprintf(buf, " &+WYour sails are hit for %d points of damage!", dam);
                act_to_all_in_ship(ship, buf);
                sprintf(buf, " &+W[%s]&N:%s&N sails has been hit for %d points of damage!", SHIPID(ship), ship->name, dam);
                act_to_outside_ships(ship, buf, NULL);*/
                damage_sail(NULL, ship, dam);
            }
        }
    }
    update_ship_status(ship);
}


int list_cargo(P_char ch, P_ship ship, int owned)
{
    if (!owned)
    {
        send_to_char("You do not own a ship!\r\n", ch);
        return TRUE;
    }
    int rroom = 0;
    while (rroom < MAXCARGO) 
    {
        if (cargo_data[rroom].loc_room == world[ch->in_room].number)
            break;
        rroom++;
    }
    if (rroom >= MAXCARGO) 
    {
        send_to_char ("There is no cargo for sale here!\r\n", ch);
        return TRUE;
    }
    send_to_char ("&+y---=== For sale ===---&N\r\n", ch);
    int cost = 1000 * (int) (cargo_data[rroom].base_cost_cargo * shipcargo[rroom].buy[rroom] / 100.0);

    //if( GET_LEVEL(ch) < 50 )
    //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);
    
    sprintf(buf, "%s:&N %s &Nper crate.\r\n", cargo_name[rroom], coin_stringv(cost));
    send_to_char(buf, ch);

    send_to_char("\r\n&+y---=== We're buying ===---&N\r\n", ch);
    for (int i = 0; i < MAXCARGO; i++) 
    {
        if (i == rroom)
            continue;
        cost = 1000 * (int) (cargo_data[i].base_cost_cargo * (cargo_mod[rroom][i] / 100.0) * (shipcargo[rroom].sell[i] / 100.0));

        //if( GET_LEVEL(ch) < 50 )
        //  cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);
    
        int ansi_diff = strlen(strip_ansi(str_dup(cargo_name[i])).c_str()) - strlen(cargo_name[i]);
        sprintf(buf2, "%%-%ds&n%%s per crate.\r\n", 30 - ansi_diff);
        sprintf(buf, buf2, cargo_name[i], coin_stringv(cost));
        send_to_char(buf, ch);
    }

    send_to_char("\r\nTo buy cargo, type 'buy cargo <number of crates>'\r\n", ch);
    send_to_char("To sell cargo, type 'sell cargo'\r\n", ch);

    if (ship->frags >= MINCONTRAFRAGS && GET_ALIGNMENT(ch) <= MINCONTRAALIGN) 
    {

        if (ship->frags >= cargo_data[rroom].frag_threshold)
        {
            send_to_char ("\r\n&+L---=== Contraband for sale ===---&N\r\n", ch);
            cost = (1000 * (int) (cargo_data[rroom].base_cost_contra * shipcontra[rroom].buy[rroom] / 100.0));

            //if( GET_LEVEL(ch) < 50 )
            //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);

            sprintf(buf, "%s&n: %s &+Lper crate.&N\r\n", contra_name[rroom], coin_stringv(cost));
            send_to_char(buf, ch);
        }
        send_to_char("\r\n&+L---=== We buy contraband for ===---&N\r\n", ch);

        for (int i = 0; i < MAXCARGO; i++) 
        {
            if (i == rroom)
                continue;
            cost = 1000 * (int) (cargo_data[i].base_cost_contra * (cargo_mod[rroom][i] / 100.0) * (shipcontra[rroom].sell[i] / 100.0));

            //if( GET_LEVEL(ch) < 50 )
            //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);

            int ansi_diff = strlen(strip_ansi(str_dup(contra_name[i])).c_str()) - strlen(contra_name[i]);
            sprintf(buf2, "%%-%ds&n%%s per crate.\r\n", 30 - ansi_diff);
            sprintf(buf, buf2, contra_name[i], coin_stringv(cost));
//            sprintf(buf, "&+L%-30s&n%s per crate.\r\n", strip_ansi(str_dup(contra_name[i])).c_str(), coin_stringv(cost));
            send_to_char(buf, ch);
        }

        send_to_char("\r\nTo buy contraband, type 'buy contraband <number of crates>'\r\n", ch);
        send_to_char("To sell contraband, type 'sell contraband'\r\n", ch);
    }

    send_to_char("\r\n&+cYour cargo manifest\r\n", ch);
    send_to_char("&+W----------------------------------\r\n", ch);

    for (int i = 0; i < MAXSLOTS; i++) 
    {
        if (ship->slot[i].type == SLOT_CARGO) 
        {
            int cargo_type = ship->slot[i].index;
            if (cargo_type == rroom) 
            {
                sprintf(buf, "%s&n, &+Y%d&n crates, invoiced at %s&n.\r\n", 
                    cargo_name[cargo_type], 
                    ship->slot[i].val0,
                    coin_stringv(ship->slot[i].val1));
                send_to_char(buf, ch);
            }
            else
            {
                cost = 1000 * (int) (cargo_data[cargo_type].base_cost_cargo * (cargo_mod[rroom][cargo_type] / 100.0) * (shipcargo[rroom].sell[cargo_type] / 100.0));
                cost *= ship->slot[i].val0;

                //if( GET_LEVEL(ch) < 50 )
                //    cost = (int) (cost * (float) ((float) GET_LEVEL(ch) / 50.0));
            
                int cost2 = (int) ( ((float) cost / (float) ship->slot[i].val1) - 1.00) * 100;

                sprintf(buf, "%s&n, &+Y%d&n crates. Can sell for %s&n, profit: %d%%\r\n",
                    cargo_name[cargo_type], 
                    ship->slot[i].val0,
                    coin_stringv(cost),
                    cost2);
                send_to_char(buf, ch);
            }
        }
        if (ship->slot[i].type == SLOT_CONTRABAND) 
        {
            int contra_type = ship->slot[i].index;
            if (contra_type == rroom) 
            {
                sprintf(buf, "&+L*&n%s&n, &+Y%d&n crates, invoiced at %s&n.\r\n", 
                    contra_name[contra_type], 
                    ship->slot[i].val0,
                    coin_stringv(ship->slot[i].val1));
                send_to_char(buf, ch);
            }
            else
            {
                cost = 1000 * (int) (cargo_data[contra_type].base_cost_contra * (cargo_mod[rroom][contra_type] / 100.0) * (shipcontra[rroom].sell[contra_type] / 100.0));
                cost *= ship->slot[i].val0;

                //if( GET_LEVEL(ch) < 50 )
                //    cost = (int) (cost * (float) ((float) GET_LEVEL(ch) / 50.0));
            
                int cost2 = (int) (((float) ((float) cost / (float) ship->slot[i].val1) - 1.00) * 100);
                sprintf(buf, "&+L*&n%s&n, &+Y%d&n crates. Can sell for %s&n, profit: %d&n%%\r\n", 
                    contra_name[contra_type],
                    ship->slot[i].val0,
                    coin_stringv(cost),
                    cost2);
                send_to_char(buf, ch);
            }
        }
    }

    sprintf(buf, "\r\nCapacity: &+W%d&n/&+W%d\r\n", SHIPAVAILCARGO(ship), SHIPMAXCARGO(ship));
    send_to_char(buf, ch);

    return TRUE;
}

int list_weapons(P_char ch, P_ship ship, int owned)
{
    char rng[20], dam[20];
    if (!owned)
    {
        send_to_char("You do not own a ship!\r\n", ch);
        return TRUE;
    }

    send_to_char("&+gWeapons\r\n", ch);
    send_to_char("&+y==================================================================================================\r\n", ch);
    send_to_char("    Name               Weight  Range   Damage  Ammo Disper  Sail  Hull  Sail Reload  Cost            \r\n", ch);
    send_to_char("                                                      sion   Hit   Dam   Dam   Time                  \r\n", ch);
    send_to_char("&+W--------------------------------------------------------------------------------------------------\r\n", ch);
    for (int i = 0; i < MAXWEAPON; i++) 
    {
        sprintf(rng, "%d-%d", weapon_data[i].min_range, weapon_data[i].max_range);
        if (weapon_data[i].fragments > 1)
        {
            if (weapon_data[i].min_damage == weapon_data[i].max_damage)
                sprintf(dam, "%d x %d", weapon_data[i].fragments, weapon_data[i].min_damage);
            else
                sprintf(dam, "%d x %d-%d", weapon_data[i].fragments, weapon_data[i].min_damage, weapon_data[i].max_damage);
        }
        else
        {
            if (weapon_data[i].min_damage == weapon_data[i].max_damage)
                sprintf(dam, "%d", weapon_data[i].min_damage);
            else
                sprintf(dam, "%d-%d", weapon_data[i].min_damage, weapon_data[i].max_damage);
        }

        sprintf(buf,  "%2d) %s%-20s   &+Y%2d  %5s  %7s    %2d    %3d  %3d%%  %3d%%  %3d%%    %3d  &n%10s&n\r\n",
          i + 1, 
          (ship_allowed_weapons[ship->m_class][i] && ((!IS_SET(weapon_data[i].flags, CAPITOL)) || (ship->frags >= MINCAPFRAG))) ? "&+W" : "&+L", 
          weapon_data[i].name,
          weapon_data[i].weight,
          rng,
          dam,
          weapon_data[i].ammo,
          weapon_data[i].hit_arc,
          weapon_data[i].sail_hit,
          weapon_data[i].hull_dam,
          weapon_data[i].sail_dam,
          weapon_data[i].reload_time,
          coin_stringv(weapon_data[i].cost));
        send_to_char(buf, ch);
    }

    send_to_char("\r\nTo buy a weapon, type 'buy weapon <number> <fore/rear/port/starboard>'\r\n", ch);
    send_to_char("To sell a weapon, type 'sell <slot number>'\r\n", ch);
    send_to_char("To reload a weapon, type 'reload <weapon number>'\r\n", ch);
    send_to_char("\r\n", ch);
    
    return TRUE;
}

int list_hulls (P_char ch, P_ship ship, int owned)
{
    if (owned)
    {
        send_to_char("&+gHull Upgrades\r\n",ch);
        send_to_char("&+y===================================================================================&N\r\n", ch);
        send_to_char("                 Load   Cargo  Passenger   Max   Total  Weapons  Epic     \r\n", ch);
        send_to_char("    Name         Cap.    Cap.     Cap.    Speed  Armor (F/S/R/P) Cost Cost\r\n", ch);
        send_to_char("&+W-----------------------------------------------------------------------------------\r\n", ch);
        for (int i = 0; i < MAXSHIPCLASS; i++)                                
        {
            if (SHIPTYPECOST(i) == 0) continue;
            sprintf(buf, "%-2d) %-11s   &+Y%-3d    &+Y%-3d       &+Y%-2d      &+Y%-3d    &+Y%-3d   &+Y%1d/&+Y%1d/&+Y%1d/&+Y%1d   &+R%c   &n%-14s\r\n", 
                i + 1,
                SHIPTYPENAME(i), 
                SHIPTYPEMAXWEIGHT(i),
                SHIPTYPECARGO(i),
                SHIPTYPEPEOPLE(i),
                SHIPTYPESPEED(i),
                ship_arc_properties[i].armor[FORE] + ship_arc_properties[i].armor[STARBOARD] + ship_arc_properties[i].armor[REAR] + ship_arc_properties[i].armor[PORT],
                ship_arc_properties[i].max_weapon_slots[FORE], ship_arc_properties[i].max_weapon_slots[STARBOARD], ship_arc_properties[i].max_weapon_slots[REAR], ship_arc_properties[i].max_weapon_slots[PORT],
                (SHIPTYPEEPICCOST(i) > 0) ? ('0' + SHIPTYPEEPICCOST(i)) : ' ',
                coin_stringv(SHIPTYPECOST(i)));
            send_to_char(buf, ch);
        }
        send_to_char("\r\nTo upgrade/downgrade your hull, type 'buy <number>'\r\n", ch);
        send_to_char("To sell your ship completely, type 'sell ship'. &+RCAUTION!&n You will loose your frags and crews!\r\n", ch);

        send_to_char("\r\n", ch);


        send_to_char("&+gRepairs\r\n",ch);
        send_to_char("&+y===================================================================&N\r\n", ch);

        if (IS_SET(ship->flags, MAINTENANCE)) 
        {
            sprintf(buf, "Your ship is currently being worked on, please wait another %.1f hours.\r\n",
                (float) ship->timer[T_MAINTENANCE] / 75.0);
            send_to_char(buf, ch);
        }
        else
        {
            send_to_char("To repair, type 'repair <armor/internal/sail/weapon> <fore/port/starboard/rear>'\r\n", ch);
        }

        send_to_char("\r\n", ch);
        send_to_char("&+gRename\r\n",ch);
        send_to_char("&+y===================================================================&N\r\n", ch);
        send_to_char("To rename ship, for a price, type 'buy rename <new ship name>'\r\n", ch);
    }
    else
    {
        send_to_char("&+cShips Available\r\n",ch);
        send_to_char("&+y=======================================================&N\r\n", ch);

        send_to_char("&+y===================================================================================&N\r\n", ch);
        send_to_char("                 Load   Cargo  Passenger   Max   Total  Weapons  Epic     \r\n", ch);
        send_to_char("    Name         Cap.    Cap.     Cap.    Speed  Armor (F/S/R/P) Cost Cost\r\n", ch);
        send_to_char("&+W-----------------------------------------------------------------------------------\r\n", ch);
        for (int i = 0; i < MAXSHIPCLASS; i++)
        {
            if (SHIPTYPECOST(i) == 0) continue;
            sprintf(buf, "%-2d) %-11s   &+Y%-3d    &+Y%-3d       &+Y%-2d      &+Y%-3d    &+Y%-3d   &+Y%1d/&+Y%1d/&+Y%1d/&+Y%1d   &+R%c   &n%-14s\r\n",
                i + 1,
                SHIPTYPENAME(i), 
                SHIPTYPEMAXWEIGHT(i),
                SHIPTYPECARGO(i),
                SHIPTYPEPEOPLE(i),
                SHIPTYPESPEED(i),
                ship_arc_properties[i].armor[FORE] + ship_arc_properties[i].armor[STARBOARD] + ship_arc_properties[i].armor[REAR] + ship_arc_properties[i].armor[PORT],
                ship_arc_properties[i].max_weapon_slots[FORE], ship_arc_properties[i].max_weapon_slots[STARBOARD], ship_arc_properties[i].max_weapon_slots[REAR], ship_arc_properties[i].max_weapon_slots[PORT],
                (SHIPTYPEEPICCOST(i) > 0) ? ('0' + SHIPTYPEEPICCOST(i)) : ' ',
                coin_stringv(SHIPTYPECOST(i)));
            send_to_char(buf, ch);
        }

        send_to_char("\r\n", ch);
        send_to_char("&+YRead HELP WARSHIP before buying one!\r\n", ch);
        send_to_char("To buy a ship, type 'buy <number> <name of ship>'\r\n", ch);
    }
    return TRUE;
}

int summon_ship (P_char ch, P_ship ship)
{

    if (ship->location == ch->in_room) {
        send_to_char("Your ship is already docked here!\r\n", ch);
        return TRUE;;
    }
    if (IS_SET(ship->flags, SINKING)) {
        send_to_char("We can't summon your ship.  Sorry.\r\n", ch);
        return TRUE;
    }
    if (IS_SET(ship->flags, SUMMONED)) {
        send_to_char ("There is already an order out on your ship.\r\n", ch);
        return TRUE;
    }
    if (ship->timer[T_BSTATION] > 0) 
    {
        send_to_char ("Your crew is not responding to our summons!\r\n", ch);
        return TRUE;
    }
    int summon_cost = SHIPTYPEHULLWEIGHT(ship->m_class) * 100;
    if (GET_MONEY(ch) < summon_cost) 
    {
        sprintf(buf, "It will cost %s to summon your ship!\r\n", coin_stringv(summon_cost));
        send_to_char(buf, ch);
        return TRUE;
    }
    SUB_MONEY(ch, summon_cost, 0);
    int buildtime = (int) (140 * 100 / (SHIPIMMOBILE(ship) ? 1 : ship->get_maxspeed()));
    if( IS_TRUSTED(ch) )
        buildtime = 0;
    SET_BIT(ship->flags, SUMMONED);
    //everyone_get_out_newship(ship);
    sprintf(buf, "Thanks for your business, it will take %d hours for your ship to get here.\r\n", buildtime / 280);
    send_to_char(buf, ch);

    sprintf(buf, "%s %d %d", GET_NAME(ch), ch->in_room, IS_TRUSTED(ch));

    add_event(summon_ship_event, buildtime, NULL, NULL, NULL, 0, buf, strlen(buf)+1);
    return TRUE;
}
int sell_slot (P_char ch, P_ship ship, int slot)
{
    if (ship->location != ch->in_room) 
    {
        send_to_char("Your ship is not here!\r\nYou can only sell your entire ship, not specific parts!\r\n", ch);
        return TRUE;
    }
    if (slot < 0 || slot >= MAXSLOTS) 
    {
        send_to_char("Invalid slot number.\r\n", ch);
        return TRUE;
    }
    if (ship->slot[slot].type == SLOT_WEAPON) 
    {
        int cost;
        if (SHIPWEAPONDAMAGED(ship, slot))
            cost = (int) (weapon_data[ship->slot[slot].index].cost * .1);
        else
            cost = (int) (weapon_data[ship->slot[slot].index].cost * .75);

        ADD_MONEY(ch, cost);
        sprintf(buf, "Here's %s for that %s.\r\n", coin_stringv(cost), ship->slot[slot].get_description());
        send_to_char(buf, ch);
        ship->slot[slot].clear();
        update_ship_status(ship);
        write_newship(ship);
        return TRUE;
    } 
    else if (ship->slot[slot].type == SLOT_CARGO)
    {
        return sell_cargo(ch, ship, slot);
    }
    else if (ship->slot[slot].type == SLOT_CONTRABAND)
    {
        return sell_contra(ch, ship, slot);
    }
    else 
    {
        send_to_char ("That slot does not contain anything to sell!\r\n", ch);
        return TRUE;
    }
}

int sell_cargo(P_char ch, P_ship ship, int slot)
{
    if (ship->location != ch->in_room) 
    {
        send_to_char("You ship is not here to unload cargo!\r\n", ch);
        return TRUE;
    }

    int rroom = 0;
    for (; rroom < MAXCARGO; ++rroom) 
        if (cargo_data[rroom].loc_room == world[ch->in_room].number)
            break;
    if (rroom >= MAXCARGO) 
    {
        send_to_char("We don't buy cargo here!\r\n", ch);
        return 0;
    }

    int total_cost = 0;
    if (slot == -1)
    {
        int none_to_sell = true;
        for (int i = 0; i < MAXSLOTS; i++)
            if (ship->slot[i].type == SLOT_CARGO)
            { none_to_sell = false; break; }

        if (none_to_sell)
        {
            send_to_char("You don't have anything we're interested in.\r\n", ch);
        }
        else
        {
            for (int i = 0; i < MAXSLOTS; i++)
            {
                if (ship->slot[i].type == SLOT_CARGO)
                {
                    total_cost += sell_cargo_slot(ch, ship, i, rroom);
                }
            }
        }
    }
    else
    {
        total_cost = sell_cargo_slot(ch, ship, slot, rroom);
    }

    if (total_cost > 0)
    {
        total_cost = check_nexus_bonus(ch, total_cost, NEXUS_BONUS_CARGO);
        sprintf(buf, "You receive %s&n.\r\n", coin_stringv(total_cost));
        send_to_char(buf, ch);
        send_to_char("Thanks for your business!\r\n", ch);        
        ADD_MONEY(ch, total_cost);

        ship->sailcrew.skill += (int)(((float)ship_crew_data[ship->sailcrew.index].skill_gain / 750.0) * (float)total_cost / 1000.0);
        ship->repaircrew.skill += (int)(((float)ship_crew_data[ship->sailcrew.index].skill_gain / 2000.0) * (float)total_cost / 1000.0);

        update_crew(ship);
        update_ship_status(ship);
        write_newship(ship);

        write_cargo();
    }
    return TRUE;
}
    
int sell_cargo_slot(P_char ch, P_ship ship, int slot, int rroom)
{
    int type = ship->slot[slot].index;
    if( type >= MAXCARGO )
        return 0;

    if (type == rroom)
    {
        sprintf(buf, "We don't buy %s&n here.\r\n", cargo_name[type]);
        send_to_char(buf, ch);
        return 0;
    }
    else
    {
        int crates = ship->slot[slot].val0;
        ship->slot[slot].clear();

        int cost = 1000 * (int) ( crates * cargo_data[type].base_cost_cargo * (cargo_mod[rroom][type] / 100.0) * (shipcargo[rroom].sell[type] / 100.0));
        //if( GET_LEVEL(ch) < 50 )
        //    cost = (int) ( cost * GET_LEVEL(ch) / 50.0 );

        wizlog(56,"Cargo: %s sold &+W%d&n crates of %s&n for %s&n.\r\n", GET_NAME(ch), crates, cargo_name[type], coin_stringv(cost));
        sprintf(buf, "You sell &+W%d&n crates of %s&n for %s&n.\r\n", crates, cargo_name[type], coin_stringv(cost));
        send_to_char(buf, ch);

        // economy affect
        shipcargo[rroom].sell[type] = MAX(MINSELLMOD, shipcargo[rroom].sell[type] - (ONSELLADJUST * (float)crates));
        for (int j = 0; j < MAXCARGO; j++) 
        {
            if (j != rroom)
            {
                shipcargo[j].sell[type] = MIN(MAXSELLMOD, shipcargo[j].sell[type] + (ONSELLADJUSTCARGO * (float)crates));
            }
            for (int k = 0; k < MAXCARGO; k++) 
            {
                if (k != type)
                {
                    shipcargo[j].sell[k] = MIN(MAXSELLMOD, shipcargo[j].sell[k] + (ONSELLADJUSTALL * (float)crates));
                }
            }
        }
        return cost;
    }
}

int sell_contra(P_char ch, P_ship ship, int slot)
{
    if (ship->location != ch->in_room) 
    {
        send_to_char ("You ship is not here to unload contraband!\r\n", ch);
        return TRUE;
    }

    int rroom = 0;    
    for (rroom = 0; rroom < MAXCARGO; ++rroom) 
        if (cargo_data[rroom].loc_room == world[ch->in_room].number)
            break;
    if (rroom >= MAXCARGO) 
    {
        send_to_char("We don't buy any contraband here!\r\n", ch);
        return 0;
    }

    int total_cost = 0;
    if (slot == -1)
    {
        int none_to_sell = true;
        for (int i = 0; i < MAXSLOTS; i++)
            if (ship->slot[i].type == SLOT_CONTRABAND)
            { none_to_sell = false; break; }

        if (none_to_sell)
        {
            send_to_char("You don't have anything we're interested in.\r\n", ch);
        }
        else
        {
            for (int i = 0; i < MAXSLOTS; i++)
            {
                if (ship->slot[i].type == SLOT_CONTRABAND)
                {
                    total_cost += sell_contra_slot(ch, ship, i, rroom);
                }
            }
        }
    }
    else
    {
        total_cost = sell_contra_slot(ch, ship, slot, rroom);
    }

    if (total_cost > 0)
    {
        total_cost = check_nexus_bonus(ch, total_cost, NEXUS_BONUS_CARGO);
        sprintf(buf, "You receive %s&n.\r\n", coin_stringv(total_cost));
        send_to_char(buf, ch);
        send_to_char("Thanks for your business!\r\n", ch);        
        ADD_MONEY(ch, total_cost);

        ship->sailcrew.skill += (int)(((float)ship_crew_data[ship->sailcrew.index].skill_gain / 1000.0) * (float)total_cost / 1000.0);
        ship->repaircrew.skill += (int)(((float)ship_crew_data[ship->sailcrew.index].skill_gain / 4000.0) * (float)total_cost / 1000.0);

        update_crew(ship);
        update_ship_status(ship);
        write_newship(ship);

        write_cargo();
    }
    return TRUE;
}

int sell_contra_slot(P_char ch, P_ship ship, int slot, int rroom)
{
    int type = ship->slot[slot].index;
    if( type >= MAXCARGO )
        return 0;

    if (type == rroom)
    {
        sprintf(buf, "We're not interested in your %s&n.\r\n", contra_name[type]);
        send_to_char(buf, ch);
        return 0;
    }
    else
    {
        int crates = ship->slot[slot].val0;
        ship->slot[slot].clear();

        int cost = 1000 * (int) ( crates * cargo_data[type].base_cost_contra * (cargo_mod[rroom][type] / 100.0) * (shipcontra[rroom].sell[type] / 100.0));
        //if( GET_LEVEL(ch) < 50 )
        //    cost = (int) ( cost * GET_LEVEL(ch) / 50.0 );

        wizlog(56,"Cargo: %s sold &+W%d&n crates of %s&n for %s&n.\r\n", GET_NAME(ch), crates, contra_name[type], coin_stringv(cost));
        sprintf(buf, "You sell &+W%d&n crates of %s&n for %s&n.\r\n", crates, contra_name[type], coin_stringv(cost));
        send_to_char(buf, ch);

        // economy affect
        shipcontra[rroom].sell[type] = MAX(MINSELLMOD, shipcontra[rroom].sell[type] - (ONSELLADJUST * (float)crates * 5.0));
        for (int j = 0; j < MAXCARGO; j++) 
        {
            if (j != rroom)
            {
                shipcontra[j].sell[type] = MIN(MAXSELLMOD, shipcontra[j].sell[type] + (ONSELLADJUSTCARGO * (float)crates * 5.0));
            }
            for (int k = 0; k < MAXCARGO; k++) 
            {
                if (k != type)
                {
                    shipcontra[j].sell[k] = MIN(MAXSELLMOD, shipcontra[j].sell[k] + (ONSELLADJUSTALL * (float)crates * 5.0));
                }
            }
        }
        return cost;
    }
}
void check_contraband(P_ship ship, int to_room)
{
    int rroom = 0;    
    for (rroom = 0; rroom < MAXCARGO; ++rroom) 
        if (cargo_data[rroom].loc_room == world[to_room].number)
            break;
    if (rroom >= MAXCARGO) 
        return;

    if (SHIPCONTRA(ship) + SHIPCARGO(ship) == 0)
        return;

    act_to_all_in_ship(ship, "The port authorities boarded the ship in search of contraband");
    float total_load = (float)(SHIPCARGO(ship) + SHIPCONTRA(ship)) / (float)SHIPMAXCARGO(ship);
    bool did_confiscate = false;
    for (int slot = 0; slot < MAXSLOTS; slot++)
    {
        if (ship->slot[slot].type == SLOT_CONTRABAND)
        {
            int type = ship->slot[slot].index;
            int crates = ship->slot[slot].val0;
            if( type < 0 || type >= MAXCARGO ) return;
            if (type == rroom) continue; // port does not confiscate its own contraband

            float conf_chance = 100.0;
            /*if (IS_TRUSTED(ch))
            {
                conf_chance = 0;
            }
            else*/ 
            if (ship->frags >= cargo_data[type].frag_threshold)
            {
                conf_chance = 25.0 + (float)crates / 2; // the more contraband you have, the bigger confiscation chance
                conf_chance -= sqrt(ship->frags) / 5.0;
                conf_chance += (100.0 - conf_chance) * (1.0 - total_load); // the more total cargo onboard, the less confiscation chance
                if (conf_chance > 100) conf_chance = 100;
            }
            debug("SHIP: (%s) confiscation chance (%d).", SHIPOWNER(ship), conf_chance);

            int confiscated = 0;
            for (int i = 0; i < crates; i++)
                if (number(0, 99) < (int)conf_chance) 
                    confiscated++;
            if (confiscated > 0)
            {
                sprintf(buf, "The port authorities found %d crates of %s&n and confiscated it!", confiscated, contra_name[type]);
                act_to_all_in_ship(ship, buf);
                ship->slot[slot].val0 -= confiscated;
                if (ship->slot[slot].val0 <= 0)
                    ship->slot[slot].clear();
                did_confiscate = true;
            }
        }
    }
    if (!did_confiscate)
        act_to_all_in_ship(ship, "... but found nothing suspicious");
}


int sell_ship(P_char ch, P_ship ship, const char* arg)
{
    int i = 0, k = 0, j;

    if (!arg || !(*arg) || !isname(arg, "confirm"))
    {
        send_to_char ("&+RIf you are sure you want to sell your ship, type 'sell ship confirm'.\r\n&+RYou WILL loose your frags and crews!&N\r\n", ch);
        return TRUE;
    }


    for (j = 0; j < 4; j++) 
    {
        i += (ship->armor[j] + ship->internal[j]);
        k += (ship->maxarmor[j] + ship->maxinternal[j]);
    }
    int cost = (int) ((int) (SHIPTYPECOST(ship->m_class) * .75) * (float) ((float) i / (float) k));
    for (j = 0; j < MAXSLOTS; j++) 
    {
        if (ship->slot[j].type == SLOT_WEAPON) 
        {
            if (SHIPWEAPONDAMAGED(ship, j))
                cost += (int) (weapon_data[ship->slot[j].index].cost * .1);
            else
                cost += (int) (weapon_data[ship->slot[j].index].cost * .75);
        }
    }
    if (ship->location != ch->in_room) 
    {
        cost /= 2;
        if (cost <= 0)
            cost = 1;
        send_to_char ("Since your ship is not here, you only get half the price, salvage costs and all!\r\n", ch);
    }
    ADD_MONEY(ch, cost);
    sprintf(buf, "Here's %s for your ship: %s.\r\n", coin_stringv(cost), ship->name);
    send_to_char(buf, ch);

    shipObjHash.erase(ship);
    delete_ship(ship);
    return TRUE;
}

int repair_sail(P_char ch, P_ship ship)
{
    if (ship->mainsail >= SHIPMAXSAIL(ship)) 
    {
        send_to_char("Your sails are fine, they don't need repair!\r\n", ch);
        return TRUE;
    }

    int cost = (SHIPMAXSAIL(ship) - ship->mainsail) * MAX(1, SHIPCLASS(ship)) * 1000;

    if (GET_MONEY(ch) < cost) 
    {
        sprintf(buf, "This sail costs %s to repair!\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
        return TRUE;
    }

    if( cost > 0 )
    {
        sprintf(buf, "You pay %s&n.\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
    }

    SUB_MONEY(ch, cost, 0);

    ship->mainsail = SHIPMAXSAIL(ship);

    int buildtime = 75 * 5;
    if (!IS_TRUSTED(ch) && BUILDTIME) 
    {
        SET_BIT(ship->flags, MAINTENANCE);
    }
    sprintf(buf, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
    send_to_char(buf, ch);
    ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}


int repair_armor(P_char ch, P_ship ship, char* arg)
{
    int cost, buildtime, total_damage, j;
    if (!*arg) 
    {
        send_to_char("Which armor do you want to repair?\r\n", ch);
        return TRUE;
    }
    if (isname(arg, "all")) 
    {
        total_damage = 0;
        for (j = 0; j < 4; j++) 
            total_damage += ship->maxarmor[j] - ship->armor[j];

        if (total_damage == 0)
        {
            sprintf(buf, "Your ship's armor is unscathed!\r\n");
            send_to_char(buf, ch);
            return TRUE;
        }

        cost = total_damage * 1000;
        if (GET_MONEY(ch) < cost) 
        {
            sprintf(buf, "This will cost %s to repair!\r\n", coin_stringv(cost));
            send_to_char(buf, ch);
            return TRUE;
        }
        /* OKay, they have the plat, deduct it and do the repairs */
        SUB_MONEY(ch, cost, 0);
        for (j = 0; j < 4; j++)
            ship->armor[j] = ship->maxarmor[j];

        buildtime = 75;
        if (!IS_TRUSTED(ch) && BUILDTIME)
            SET_BIT(ship->flags, MAINTENANCE);

        sprintf(buf, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
        send_to_char(buf, ch);
        ship->timer[T_MAINTENANCE] += buildtime;
        return TRUE;
    }
    if (isname(arg, "fore") || isname(arg, "f")) {
        j = FORE;
    } else if (isname(arg, "rear") || isname(arg, "r")) {
        j = REAR;
    } else if (isname(arg, "port") || isname(arg, "p")) {
        j = PORT;
    } else if (isname(arg, "starboard") || isname(arg, "s")) {
        j = STARBOARD;
    } else {
        send_to_char ("Invalid location.\r\nValid syntax is 'repair armor <fore/rear/starboard/port>'\r\n", ch);
        return TRUE;
    }
    total_damage = ship->maxarmor[j] - ship->armor[j];
    if (total_damage == 0)
    {
        sprintf(buf, "This side's armor is unscathed!\r\n");
        send_to_char(buf, ch);
        return TRUE;
    }
    cost = total_damage * 1000;
    buildtime = 75;
    if (GET_MONEY(ch) < cost) 
    {
        sprintf(buf, "This will cost %s to repair!\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->armor[j] = ship->maxarmor[j];
    if (!IS_TRUSTED(ch) && BUILDTIME)
        SET_BIT(ship->flags, MAINTENANCE);
    sprintf(buf, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
    send_to_char(buf, ch);
    ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}
int repair_internal(P_char ch, P_ship ship, char* arg)
{
    int cost, buildtime, total_damage, j;

    if (!*arg) 
    {
        send_to_char("Which internal do you want to repair?\r\n", ch);
        return TRUE;
    }
    if (isname(arg, "all")) 
    {
        total_damage = 0;
        for (j = 0; j < 4; j++)
            total_damage += ship->maxinternal[j] - ship->internal[j];

        if (total_damage == 0)
        {
            sprintf(buf, "Your ship's internal structured are fine!\r\n");
            send_to_char(buf, ch);
            return TRUE;
        }

        cost = total_damage * 1000;
        if (GET_MONEY(ch) < cost) 
        {
            sprintf(buf, "This will cost %s to repair!\r\n", coin_stringv(cost));
            send_to_char(buf, ch);
            return TRUE;
        }
    /* OKay, they have the plat, deduct it and do the repairs */
        SUB_MONEY(ch, cost, 0);
        for (j = 0; j < 4; j++)
            ship->internal[j] = ship->maxinternal[j];

        buildtime = 75;
        if (!IS_TRUSTED(ch) && BUILDTIME)
            SET_BIT(ship->flags, MAINTENANCE);

        sprintf(buf, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
        send_to_char(buf, ch);
        ship->timer[T_MAINTENANCE] += buildtime;
        update_ship_status(ship);
        write_newship(ship);
        return TRUE;
    }
    if (isname(arg, "fore") || isname(arg, "f")) {
        j = FORE;
    } else if (isname(arg, "rear") || isname(arg, "r")) {
        j = REAR;
    } else if (isname(arg, "port") || isname(arg, "p")) {
        j = PORT;
    } else if (isname(arg, "starboard") || isname(arg, "s")) {
        j = STARBOARD;
    } else {
        send_to_char ("Invalid location.\r\nValid syntax is 'repair internal <fore/rear/starboard/port>'\r\n", ch);
        return TRUE;
    }
    total_damage = ship->maxinternal[j] - ship->internal[j];
    if (total_damage == 0)
    {
        sprintf(buf, "This side's internal structures are fine!\r\n");
        send_to_char(buf, ch);
        return TRUE;
    }
    cost = total_damage * 1000;
    buildtime = 75;
    if (GET_MONEY(ch) < cost) 
    {
        sprintf(buf, "This will cost %s to repair!\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->internal[j] = ship->maxinternal[j];
    if (!IS_TRUSTED(ch) && BUILDTIME)
        SET_BIT(ship->flags, MAINTENANCE);

    sprintf(buf, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
    send_to_char(buf, ch);
    ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}

int repair_weapon(P_char ch, P_ship ship, char* arg)
{
    int cost, buildtime;

    if (!is_number(arg)) 
    {
        send_to_char("Valid syntax is 'repair weapon <weapon slot>'\r\n", ch);
        return TRUE;
    }
    int slot = atoi(arg);
    if (slot > MAXSLOTS || slot < 0) 
    {
        send_to_char ("Invalid weapon slot'\r\n", ch);
        return TRUE;
    }
    if (ship->slot[slot].type != SLOT_WEAPON) 
    {
        send_to_char ("Invalid weapon slot'\r\n", ch);
        return TRUE;
    }
    if (!SHIPWEAPONDAMAGED(ship, slot)) 
    {
        send_to_char("This weapon isn't broken!\r\n", ch);
        return TRUE;
    }
    cost = weapon_data[ship->slot[slot].index].cost / 2;
    buildtime = 75;
    if (GET_MONEY(ch) < cost) 
    {
        sprintf(buf, "This will cost %s to repair!\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->slot[slot].val2 = 0;
    if (!IS_TRUSTED(ch) && BUILDTIME)
        SET_BIT(ship->flags, MAINTENANCE);

    sprintf(buf, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
    send_to_char(buf, ch);
    ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}

int reload_ammo(P_char ch, P_ship ship, char* arg)
{
    char weapons_to_reload[MAXSLOTS];
    int cost = 0, buildtime = 0;

    if (is_number(arg)) 
    {
        int slot = atoi(arg);

        if (slot >= MAXSLOTS || slot < 0) 
        {
            send_to_char ("Invalid weapon.\r\nValid syntax is 'reload <weapon number>' or 'reload all'\r\n", ch);
            return TRUE;
        }
        if (ship->slot[slot].type != SLOT_WEAPON) 
        {
            send_to_char ("Invalid weapon.\r\nValid syntax is 'reload <weapon number> or 'reload all''\r\n", ch);
            return TRUE;
        }
        if (ship->slot[slot].val1 >= weapon_data[ship->slot[slot].index].ammo) 
        {
            send_to_char("That weapon is already fully loaded!\r\n", ch);
            return TRUE;
        }

        for (int k = 0; k < MAXSLOTS; k++)
            weapons_to_reload[k] = 0;
        weapons_to_reload[slot] = 1;
        cost = (weapon_data[ship->slot[slot].index].ammo - ship->slot[slot].val1) * 1000;
        buildtime = 75;

    } 
    else if (isname(arg, "all"))
    {
        for (int slot = 0; slot < MAXSLOTS; slot++) 
        {
            if ((ship->slot[slot].type == SLOT_WEAPON) && (ship->slot[slot].val1 < weapon_data[ship->slot[slot].index].ammo)) 
            {
                weapons_to_reload[slot] = 1;
                cost += (weapon_data[ship->slot[slot].index].ammo - ship->slot[slot].val1) * 1000;
                buildtime += 75;
            } 
            else
                weapons_to_reload[slot] = 0;
        }
    }

    if (cost == 0 || buildtime == 0) 
    {
        send_to_char("Nothing to reload here!\n\r", ch);
        return TRUE;
    }

    if (GET_MONEY(ch) < cost) 
    {
        sprintf(buf, "This will cost %s to reload!\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);

    for (int slot = 0; slot < MAXSLOTS; slot++)
        if (weapons_to_reload[slot] == 1)
            ship->slot[slot].val1 = weapon_data[ship->slot[slot].index].ammo;

    if (!IS_TRUSTED(ch) && BUILDTIME) 
        SET_BIT(ship->flags, MAINTENANCE);

    sprintf(buf, "Thanks for your business, it will take %d hours to complete this reload.\r\n", buildtime / 75);
    send_to_char(buf, ch);
    ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}

int rename_ship(P_char ch, P_ship ship, char* new_name)
{
    if (!new_name || !*new_name)
    {
        send_to_char("Invalid syntax.\r\n", ch);
        return TRUE;
    }

    /* count money */
    int renamePrice = (int) (SHIPTYPECOST(ship->m_class) / 10);

    if (GET_MONEY(ch) < renamePrice)
    {
        sprintf(buf, "It will cost you %s!\r\n", coin_stringv(renamePrice));
        send_to_char(buf, ch);
        return TRUE;
    }
    /* count money end */

    if( !rename_ship(ch, GET_NAME(ch), new_name) == TRUE)
    {
        return TRUE;
    }
    SUB_MONEY(ch, renamePrice, 0);

    sprintf(buf, "&+WCongratulations! From now on yours ship will be known as&n %s\r\n", arg2);
    send_to_char(buf, ch);

    wizlog(AVATAR, "%s renamed %s ship to %s\r\n", GET_NAME(ch), GET_SEX(ch) == SEX_MALE ? "his" : "her", arg2);
    logit(LOG_PLAYER, "%s renamed %s ship to %s\r\n", GET_NAME(ch), GET_SEX(ch) == SEX_MALE ? "his" : "her", arg2);

    return TRUE;
}

int buy_cargo(P_char ch, P_ship ship, char* arg)
{
    int rroom = 0;
        
    while (rroom < MAXCARGO) 
    {
        if (world[ch->in_room].number == cargo_data[rroom].loc_room)
            break;
        rroom++;
    }
    if (rroom >= MAXCARGO) 
    {
        send_to_char("What cargo? We don't sell any cargo here!\r\n", ch);
        return TRUE;
    }
    if (SHIPMAXCARGO(ship) == 0)
    {
        send_to_char("You ship has no space for cargo.\r\n", ch);
        return TRUE;    
    }
    if (!is_number(arg)) 
    {
        send_to_char("Valid syntax is 'buy cargo <number of crates>'\r\n", ch);
        return TRUE;
    }
    int asked_for = atoi(arg);
    if (asked_for < 1) 
    {
        send_to_char("Please enter a valid number of crates to buy.\r\n", ch);
        return TRUE;
    }

    if (SHIPAVAILCARGO(ship) <= 0)
    {
        send_to_char("You don't have any space left on your ship!\r\n", ch);
        return TRUE;
    }
    else if (asked_for > SHIPAVAILCARGO(ship))
    {
        sprintf(buf, "You only have space for %d more crates!\r\n", SHIPAVAILCARGO(ship));
        send_to_char(buf, ch);
        return TRUE;
    }

    int slot;
    for (slot = 0 ; slot < MAXSLOTS; ++slot) 
    {
        if (ship->slot[slot].type == SLOT_CARGO && ship->slot[slot].index == rroom)
            break;
    }
    if (slot == MAXSLOTS)
    {
        for (slot = 0 ; slot < MAXSLOTS; ++slot) 
        {
            if (ship->slot[slot].type == SLOT_EMPTY) 
                break;
        }
    }
    if (slot == MAXSLOTS) 
    {
        send_to_char("You do not have a free slot to fit this cargo into!\r\n", ch);
        return TRUE;
    }

    int unit_cost = 1000 * (int) (cargo_data[rroom].base_cost_cargo * shipcargo[rroom].buy[rroom] / 100.0);
    int cost = asked_for * unit_cost; 

    //if (GET_LEVEL(ch) < 50)
    //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);

    if (GET_MONEY(ch) < cost) 
    {
        sprintf(buf, "This cargo load costs %s&n!\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
        return TRUE;
    }

    int placed = asked_for;
    if (ship->slot[slot].type == SLOT_CARGO)
    {
        ship->slot[slot].val0 += placed; 
        ship->slot[slot].val1 += cost;
    }
    else
    {
        ship->slot[slot].type = SLOT_CARGO;
        ship->slot[slot].index = rroom;
        ship->slot[slot].position = SLOT_HOLD;
        ship->slot[slot].val0 = placed;
        ship->slot[slot].val1 = cost;
    }

    sprintf(buf, "You buy &+W%d&n crates of %s&n for %s.\r\n", placed, cargo_name[rroom], coin_stringv(cost) );
    send_to_char(buf, ch);

    SUB_MONEY(ch, cost, 0);

    // economy affect
    shipcargo[rroom].buy[rroom] += ONBUYADJUST * placed;
    if (shipcargo[rroom].buy[rroom] > MAXBUYMOD)
        shipcargo[rroom].buy[rroom] = MAXBUYMOD;
    write_cargo();

    send_to_char ("Thank you for your purchase, your cargo is loaded and set to go!\r\n", ch);

    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}

int buy_contra(P_char ch, P_ship ship, char* arg)
{
    int rroom = 0;
    while (rroom < MAXCARGO) 
    {
        if (world[ch->in_room].number == cargo_data[rroom].loc_room) 
            break;
        rroom++;
    }
    if (rroom >= MAXCARGO) 
    {
        send_to_char("What contraband?  We don't sell any contraband!\r\n", ch);
        return TRUE;
    }
    if (!IS_TRUSTED(ch) && GET_ALIGNMENT(ch) > MINCONTRAALIGN) 
    {
        send_to_char ("Goodie goodie two shoes like you shouldn't think of contraband.\r\n", ch);
        return TRUE;
    }
    if (!IS_TRUSTED(ch) && ship->frags < cargo_data[rroom].frag_threshold) 
    {
        send_to_char ("Contraband?! *gasp* how could you even think that I would sell such things!\r\n", ch);
        return TRUE;
    }
    if (SHIPMAXCONTRA(ship) == 0)
    {
        send_to_char("You ship has no places to hide contraband.\r\n", ch);
        return TRUE;
    }
    if (!is_number(arg)) 
    {
        send_to_char("Invalid number of crates!  Syntax is 'buy contraband <number of crates>'\r\n", ch);
        return TRUE;
    }

    int asked_for = atoi(arg);
    if (asked_for < 1) 
    {
        send_to_char("Please enter a valid number of crates to buy.\r\n", ch);
        return TRUE;
    }

    if (SHIPAVAILCARGO(ship) <= 0)
    {
        send_to_char("You don't have any space left on your ship!\r\n", ch);
        return TRUE;
    }
    int contra_avail = SHIPMAXCONTRA(ship) - SHIPCONTRA(ship);
    if (contra_avail == 0)
    {
        send_to_char("You don't have any space left on your ship to hide contraband!\r\n", ch);
        return TRUE;
    }
    int max_avail = MIN(SHIPAVAILCARGO(ship), contra_avail);
    if (asked_for > max_avail)
    {
        sprintf(buf, "You only have space for %d more crates of contraband to hide!\r\n", max_avail);
        send_to_char(buf, ch);
        return TRUE;
    }
    
    int slot;
    for (slot = 0 ; slot < MAXSLOTS; ++slot) 
    {
        if (ship->slot[slot].type == SLOT_CONTRABAND && ship->slot[slot].index == rroom)
            break;
    }
    if (slot == MAXSLOTS)
    {
        for (slot = 0 ; slot < MAXSLOTS; ++slot) 
        {
            if (ship->slot[slot].type == SLOT_EMPTY) 
                break;
        }
    }
    if (slot == MAXSLOTS) 
    {
        send_to_char("You do not have a free slot to fit this contraband into!\r\n", ch);
        return TRUE;
    }

    int unit_cost = 1000 * (int) (cargo_data[rroom].base_cost_contra * shipcontra[rroom].buy[rroom] / 100.0);
    int cost = asked_for * unit_cost;

    //if (GET_LEVEL(ch) < 50) 
    //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);

    if (GET_MONEY(ch) < cost) 
    {
        sprintf(buf, "This contraband costs %s!\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
        return TRUE;
    }

    int placed = asked_for;
    if (ship->slot[slot].type == SLOT_CONTRABAND)
    {
        ship->slot[slot].val0 += placed; 
        ship->slot[slot].val1 += cost;
    }
    else
    {
        ship->slot[slot].type = SLOT_CONTRABAND;
        ship->slot[slot].index = rroom;
        ship->slot[slot].position = SLOT_HOLD;
        ship->slot[slot].val0 = placed;
        ship->slot[slot].val1 = cost;
    }

    sprintf(buf, "You buy &+W%d&n crates of %s&n for %s.\r\n", placed, contra_name[rroom], coin_stringv(cost) );
    send_to_char(buf, ch);

    SUB_MONEY(ch, cost, 0);

    // economy affect
    shipcontra[rroom].buy[rroom] += ONBUYADJUST * asked_for;
    if (shipcontra[rroom].buy[rroom] > MAXBUYMOD)
        shipcontra[rroom].buy[rroom] = MAXBUYMOD;
    write_cargo();

    send_to_char ("Thank you for your 'purchase' *snicker*, your 'cargo' is loaded and set to go!\r\n", ch);

    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}

int buy_weapon(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    if (!is_number(arg1) || !arg2 || !*arg2) 
    {
        send_to_char ("Valid syntax is 'buy weapon <number> <fore/rear/port/starboard>'\r\n", ch);
        return TRUE;
    }
    
    int w = atoi(arg1) - 1;
    if ((w < 0) || (w >= MAXWEAPON)) 
    {
        send_to_char ("Invalid weapon number.  Valid syntax is 'buy weapon <number> <fore/rear/starboard/port>'\r\n", ch);
        return TRUE;
    }

    int arc;
    if (isname(arg2, "fore") || isname(arg2, "f"))
        arc = FORE;
    else if (isname(arg2, "rear") || isname(arg2, "r"))
        arc = REAR;
    else if (isname(arg2, "port") || isname(arg2, "p"))
        arc = PORT;
    else if (isname(arg2, "starboard") || isname(arg2, "s"))
        arc = STARBOARD;
    else
    {
        send_to_char ("Invalid weapon placement. Valid syntax is 'buy weapon <number> <fore/rear/starboard/port>'\r\n", ch);
        return TRUE;
    }

    if (weapon_data[w].weight > SHIPAVAILWEIGHT(ship) ) 
    {
        sprintf(buf, "That weapon weighs %d! Your ship can only support %d!\r\n", weapon_data[w].weight, SHIPMAXWEIGHT(ship));
        send_to_char(buf, ch);
        return TRUE;
    }
    if (!ship_allowed_weapons[ship->m_class][w])
    {
        sprintf(buf, "This weapon can not be installed on this hull!\r\n");
        send_to_char(buf, ch);
        return TRUE;
    }

    int slot = 0;
    while (slot < MAXSLOTS) 
    {
        if (ship->slot[slot].type == SLOT_EMPTY)
            break;
        slot++;
    }
    if (slot >= MAXSLOTS) 
    {
        send_to_char("You do not have a free slot to intall this weapon!\r\n", ch);
        return TRUE;
    }

    int same_arc = 0;
    for (int sl = 0; sl < MAXSLOTS; sl++) 
    {
        if (ship->slot[sl].type == SLOT_WEAPON && ship->slot[sl].position == arc)
            same_arc++;
    }
    if (same_arc >= ship_arc_properties[ship->m_class].max_weapon_slots[arc])
    {
        sprintf(buf, "Your can not put more weapons to this side!\r\n");
        send_to_char(buf, ch);
        return TRUE;
    }

    if (!IS_SET(weapon_data[w].flags, slotmap[arc])) 
    {
        send_to_char ("That weapon cannot be placed in that position, try another one.\r\n", ch);
        return TRUE;
    }

    if (IS_SET(weapon_data[w].flags, CAPITOL)) 
    {
        if (ship->frags < MINCAPFRAG) 
        {
            send_to_char ("I'm sorry, but not just anyone can buy a capitol weapon!  You must earn it!\r\n", ch);
            return TRUE;
        }
        for (int j = 0; j < MAXSLOTS; j++) 
        {
            if (IS_SET(weapon_data[ship->slot[j].index].flags, CAPITOL) && (ship->slot[j].type == SLOT_WEAPON)) 
            {
                  send_to_char ("You already have a capitol weapon! You can only have one.\r\n", ch);
                  return TRUE;
            }
        }
    }

    int cost = weapon_data[w].cost;
    if (GET_MONEY(ch) < cost) 
    {
        sprintf(buf, "This weapon costs %s!\r\n", coin_stringv(cost));
        send_to_char(buf, ch);
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->slot[slot].type = SLOT_WEAPON;
    ship->slot[slot].index = w;
    ship->slot[slot].position = arc;
    ship->slot[slot].timer = 0;
    ship->slot[slot].val0 = w; // ammo type
    ship->slot[slot].val1 = weapon_data[w].ammo; // ammo count
    ship->slot[slot].val2 = 0; // damage level
    if (!IS_TRUSTED(ch) && BUILDTIME)
        SET_BIT(ship->flags, MAINTENANCE);
    int buildtime = weapon_data[w].weight * 75;
    sprintf(buf, "Thank you for your purchase, it will take %d hours to install the part.\r\n", (int) (buildtime / 75));
    send_to_char(buf, ch);
    ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}

int buy_hull(P_char ch, P_ship ship, int owned, char* arg1, char* arg2)
{
    int m_class, oldclass, cost, buildtime;

    if (owned)
    {
        if (!is_number(arg1)) 
        {
            send_to_char("To upgrade or downgrade a hull type the follow: 'buy hull <number>.'\r\n", ch);
            return TRUE;
        }
    }
    else
    {
        if (!is_number(arg1) || !arg2 || !*arg2) 
        {
            send_to_char("To buy a hull type the follow: 'buy hull <number> <name>.'\r\n", ch);
            return TRUE;
        }
    }

    int i = atoi(arg1) - 1;
    if ((i < 0) || (i >= MAXSHIPCLASS)) 
    {
        send_to_char ("Not a valid hull selection.'\r\n", ch);
        return TRUE;
    }

    if (SHIPTYPEMINLEVEL(i) > GET_LEVEL(ch)) 
    {
        send_to_char ("You are too low for such a big ship! Get more experience!\r\n", ch);
        return TRUE;
    }

    if (SHIPTYPECOST(i) == 0) 
    {
        send_to_char ("You can not buy this hull.\r\n", ch);
        return TRUE;
    }

    /* Okay, we have a valid selection for ship hull */
    if (owned) 
    {
        oldclass = ship->m_class;
        if (i == oldclass)
        {
            send_to_char ("You own this hull already!\r\n", ch);
            return TRUE;
        }

        if (i < oldclass)
        {
            for( int k = 0; k < MAXSLOTS; k++ )
            {
                if (ship->slot[k].type != SLOT_EMPTY)
                {
                    send_to_char ("You can not downgrade a hull with non-empty slots!\r\n", ch);
                    return TRUE;
                }
            }
        }
        cost = SHIPTYPECOST(i) - (int) (SHIPTYPECOST(oldclass) * .75);
        if (cost >= 0)
        {
            if (GET_MONEY(ch) < cost || epic_skillpoints(ch) < SHIPTYPEEPICCOST(i)) 
            {
                if (SHIPTYPEEPICCOST(i) > 0)
                    sprintf(buf, "That upgrade costs %s and %d epic points!\r\n", coin_stringv(cost), SHIPTYPEEPICCOST(i));
                else
                    sprintf(buf, "That upgrade costs %s!\r\n", coin_stringv(cost));
                send_to_char(buf, ch);
                return TRUE;
            }
            SUB_MONEY(ch, cost, 0);/* OKay, they have the plat, deduct it and build the ship */
            if (SHIPTYPEEPICCOST(i) > 0)
                epic_gain_skillpoints(ch, -SHIPTYPEEPICCOST(i));
        }
        else
        {
            if (epic_skillpoints(ch) < SHIPTYPEEPICCOST(i))
            {
                sprintf(buf, "That upgrade costs %d epic points!\r\n", SHIPTYPEEPICCOST(i));
                send_to_char(buf, ch);
                return TRUE;
            }
            cost *= -1;
            sprintf(buf, "You receive %s&n for remaining materials.\r\n", coin_stringv(cost));
            send_to_char(buf, ch);
            ADD_MONEY(ch, cost);
            if (SHIPTYPEEPICCOST(i) > 0)
                epic_gain_skillpoints(ch, -SHIPTYPEEPICCOST(i));
        }


        m_class = i;
        ship->m_class = i;
        setarmor(ship, true);
        ship->mainsail = SHIPMAXSAIL(ship);
        ship->repair = SHIPTYPEHULLWEIGHT(ship->m_class);
        /*if(ship->lastsunk > 2)
        {
            ship->lastsunk /= 2;
            send_to_char("Your reputation diminishes.\r\n", ch);
        }*/
    
        clear_ship_layout(ship);
        set_ship_layout(ship, ship->m_class);
        reset_ship_physical_layout(ship);
        nameship(str_dup(ship->name), ship);
        update_maxspeed(ship);
        if (m_class > oldclass)
            buildtime = 75 * (m_class / 2 - oldclass / 3);
        else
            buildtime = 75 * (oldclass / 2 - m_class / 3);

        sprintf(buf, "Thanks for your business, it will take %d hours to complete this upgrade.\r\n", buildtime / 75);
        send_to_char(buf, ch);
    }
    else
    {
        if ((int)strlen(strip_ansi(arg2).c_str()) <= 0)
        {
            send_to_char ("No name selected. Valid syntax is 'buy <number> <name>'\r\n", ch); 
            return TRUE;
        }
        if (!is_valid_ansi_with_msg(ch, arg2, FALSE))
        {
            send_to_char ("Invalid ANSI name'\r\n", ch); 
            return TRUE;
        }
        if ((int) strlen(strip_ansi(arg2).c_str()) > 20) 
        {
            send_to_char("Name must be less than 20 characters (not including ansi))\r\n", ch);
            return TRUE;
        }

        if (GET_MONEY(ch) < SHIPTYPECOST(i) || epic_skillpoints(ch) < SHIPTYPEEPICCOST(i)) 
        {
            if (SHIPTYPEEPICCOST(i) > 0)
                sprintf(buf, "That ship costs %s and %d epic points!\r\n", coin_stringv(SHIPTYPECOST(i)), SHIPTYPEEPICCOST(i));
            else
                sprintf(buf, "That ship costs %s!\r\n", coin_stringv(SHIPTYPECOST(i)));
            send_to_char(buf, ch);
            return TRUE;
        }

        SUB_MONEY(ch, SHIPTYPECOST(i), 0);
        if (SHIPTYPEEPICCOST(i) > 0)
            epic_gain_skillpoints(ch, -SHIPTYPEEPICCOST(i));

        /* Now, create the ship object */
        ship = newship(i);
        if (ship == NULL) 
        {
            send_to_char("World is too full of ships.\r\n", ch);
            return TRUE;
        }
        if (ship->panel == NULL) 
        {
            send_to_char("panel null 1\r\n", ch);
            return TRUE;
        }
        buildtime = 75 * SHIPTYPEID(i) / 4;
        ship->ownername = str_dup(GET_NAME(ch));
        ship->anchor = world[ch->in_room].number;
        set_ship_layout(ship, i);
        nameship(arg2, ship);
        if (!loadship(ship, ch->in_room)) 
        {
            logit(LOG_FILE, "error in loadship(): %d\n", shiperror); 
            send_to_char("Error loading ship, please notify a god.\r\n", ch);
            return TRUE;
        }
        sprintf(buf, "Thanks for your business, this hull will take %d hours to build.\r\n", SHIPTYPEID(i) / 4);
        send_to_char(buf, ch);
    }

    if (!IS_TRUSTED(ch) && BUILDTIME)
        SET_BIT(ship->flags, MAINTENANCE);
    ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_newship(ship);
    return TRUE;
}

int swap_slots(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    if (!*arg1 || !is_number(arg1) || !*arg2 || !is_number(arg2)) 
    {
        send_to_char("Valid Syntax: buy swap <slot1> <slot2>\r\n", ch);
        return TRUE;
    }
    int slot1 = atoi(arg1);
    int slot2 = atoi(arg2);
    if ((slot1 >= MAXSLOTS) || (slot1 < 0) || (slot2 >= MAXSLOTS) || (slot2 < 0)) 
    {
        send_to_char("Invalid slot number!\r\n", ch);
        return TRUE;
    }
    if (slot1 == slot2)
    {
        send_to_char("Switch slot with itself?! What for?\r\n", ch);
        return TRUE;
    }
    ShipSlot temp;
    temp.clone(ship->slot[slot1]);
    ship->slot[slot1].clone(ship->slot[slot2]);
    ship->slot[slot2].clone(temp);
    send_to_char("Done! Thank you for your business!\r\n", ch);
    write_newship(ship);
    return TRUE;
}

int newship_shop(int room, P_char ch, int cmd, char *arg)
{
    //if(!get_property("ship.toggle.master", 1.000));
    //    return false;

    if (!ch)
        return FALSE;

    if ((cmd != CMD_SUMMON) && (cmd != CMD_LIST) && (cmd != CMD_BUY) &&
        (cmd != CMD_RELOAD) && (cmd != CMD_REPAIR) && (cmd != CMD_SELL))
        return FALSE;

    half_chop(arg, arg1, arg2);

    /* hack to allow ferry ticket automats in room with ship store */
    if ( cmd == CMD_BUY && isname(arg1, "ticket") )
        return FALSE;

    if ( cmd == CMD_SUMMON && !isname(arg1, "ship") )
        return FALSE;

    int  owned = 0;
    P_ship ship = NULL;
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (isname(svs->ownername, GET_NAME(ch))) 
        {
            owned = 1;
            ship = svs;
            break;
        }
    }

    if (owned && (ship->location != ch->in_room)) 
    {
            
        if ((cmd != CMD_SUMMON) && (cmd != CMD_LIST) && (cmd != CMD_SELL))
        {
            int summon_cost = SHIPTYPEHULLWEIGHT(ship->m_class) * 100;
            send_to_char("Your ship needs to be here to receive service.\r\n", ch);
            sprintf(buf, "For a small fee of %s, &nI can have my men tell your crew to sail here.\r\n", coin_stringv(summon_cost));
            send_to_char(buf, ch);
            send_to_char("Just type 'summon ship' to have your ship sail here.\r\n", ch);
            return TRUE;
        }
    }

    if (owned && (!SHIPISDOCKED(ship) || ship->timer[T_UNDOCK] != 0)) 
    {
        if ((cmd != CMD_SUMMON) && (cmd != CMD_LIST) && (cmd != CMD_SELL))
        {
            sprintf(buf,"Dock your ship to recieve a maintenance!\r\n");
            send_to_char(buf, ch);
            return TRUE;
        }
    }

    if (owned && (cmd != CMD_LIST) && IS_SET(ship->flags, MAINTENANCE))
    {
        sprintf(buf,"Your ship is currently being worked on, please wait for another %.1f hours.\r\n", 
            (float) ship->timer[T_MAINTENANCE] / 75.0);
        send_to_char(buf, ch);
        return TRUE;
    }

    if (cmd == CMD_LIST) 
    {
        if (*arg1) 
        {
            if (isname(arg1, "cargo")) 
            {
                return list_cargo(ch, ship, owned);
            }
            else if (isname(arg1, "weapon") || isname(arg1, "weapons")) 
            {
                return list_weapons(ch, ship, owned);
            }
            else if (isname(arg1, "hull") || isname(arg1, "hulls")) 
            {
                return list_hulls(ch, ship, owned);
            }
        }
        send_to_char("Valid syntax is 'list <hull/weapon/cargo>'\r\n", ch);
        return TRUE;
    }
    if (cmd == CMD_SUMMON) 
    {
        
        if (!owned) 
        {
            send_to_char("You do not own a ship!\r\n", ch);
            return TRUE;
        }
        return summon_ship(ch, ship);
    }

    if (cmd == CMD_SELL) 
    {
         
        if (!owned) 
        {
            send_to_char("You have no ship to sell something!\r\n", ch);
            return TRUE;
        }
        if (ship->location != ch->in_room && ship->timer[T_BSTATION] != 0) 
        {
            send_to_char("Your ship is currently pre-occupied in combat!\r\n", ch);
            return TRUE;
        }
        if (*arg1) 
        {
            if (isname(arg1, "ship")) 
            {
                return sell_ship(ch, ship, arg2);
            }
            else 
            {
                if (!SHIPISDOCKED(ship) || ship->timer[T_UNDOCK] != 0)
                {
                    sprintf(buf,"Dock your ship first!\r\n");
                    send_to_char(buf, ch);
                    return TRUE;
                }
                if (isname(arg1, "cargo")) 
                {
                    return sell_cargo(ch, ship, -1);
                } 
                else if (isname(arg1, "contraband")) 
                {
                    return sell_contra(ch, ship, -1);
                } 
                else if (is_number(arg1)) 
                {
                    return sell_slot(ch, ship, atoi(arg1));
                }
            }
        }
        send_to_char("Valid syntax is 'sell <ship/cargo/contraband/[slot]>'\r\n", ch);
        return TRUE;
    }

    if (cmd == CMD_REPAIR) 
    {
        if (!owned) 
        {
            send_to_char("You have no ship to repair!\r\n", ch);
            return TRUE;
        }
        if (*arg1) 
        {
            if (isname(arg1, "sail") || isname(arg1, "s")) 
            {
                return repair_sail (ch, ship);
            } 
            else if (isname(arg1, "armor") || isname(arg1, "a")) 
            {
                return repair_armor(ch, ship, arg2);
            } 
            else if (isname(arg1, "internal") || isname(arg1, "i")) 
            {
                return repair_internal(ch, ship, arg2);
            } 
            else if (isname(arg1, "weapon") || isname(arg1, "w")) 
            {
                return repair_weapon(ch, ship, arg2);
            } 
        }
        send_to_char("Valid syntax is 'repair <armor/internal/sail/weapon>'\r\n", ch);
        return TRUE;
    }
    if (cmd == CMD_RELOAD) 
    {
        if (!owned) 
        {
            send_to_char("You have no ship to reload!\r\n", ch);
            return TRUE;
        }
        if (*arg1) 
        {
            if (is_number(arg1) || isname(arg1, "all"))
            {
                return reload_ammo(ch, ship, arg1);
            }
        }
        send_to_char ("Valid syntax is 'reload <[slot]/all>'\r\n", ch);
        return TRUE;

    }

    if (cmd == CMD_BUY) 
    {
        if (isname(arg1, "rename"))
        {
            if (!owned) 
            {
                send_to_char("You have no ship to rename!\r\n", ch);
                return TRUE;
            }
            return rename_ship(ch, ship, arg2);
        }
        else if (isname(arg1, "swap"))
        {
            if (!owned) 
            {
                send_to_char("You have no ship to swap slots!\r\n", ch);
                return TRUE;
            }
            half_chop(arg2, arg1, arg3);
            return swap_slots(ch, ship, arg1, arg3);
        }
        else if (isname(arg1, "cargo")) 
        {
            if (!owned) 
            {
                send_to_char("You have no ship to load!\r\n", ch);
                return TRUE;
            }
            return buy_cargo(ch, ship, arg2);
        } 
        else if (isname(arg1, "contraband")) 
        {
            if (!owned) 
            {
                send_to_char("You have no ship to load!\r\n", ch);
                return TRUE;
            }
            return buy_contra(ch, ship, arg2);
        } 
        else if (isname(arg1, "weapon") || isname(arg1, "w")) 
        {
            if (!owned) 
            {
                send_to_char("You have no ship to equip!\r\n", ch);
                return TRUE;
            }
            half_chop(arg2, arg1, arg3);
            return buy_weapon(ch, ship, arg1, arg3);
        }
        else if (isname(arg1, "hull") || isname(arg1, "h")) 
        {
            half_chop(arg2, arg1, arg3);
            return buy_hull(ch, ship, owned, arg1, arg3);
        } 
        else
        {
            send_to_char ("Valid syntax: 'buy hull/cargo/contraband/weapon/rename/swap>'.\r\n", ch);
            return TRUE;
        }
    }


    send_to_char ("ship_shop deadend reached, report a bug!\r\n", ch);
    return FALSE;
}; /* ship_shop */



int write_newship(P_ship ship)
{
    char     buf[MAX_STRING_LENGTH];
    int      i;
    FILE    *f = NULL;

    if (ship == NULL)
    {
        f = fopen("Ships/ship_index", "w");
        if (!f)
        {
            logit(LOG_FILE, "Ship index file open error.");
            return FALSE;
        }
        ShipVisitor svs;
        for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
        {
            if (IS_SET(svs->flags, LOADED))
                fprintf(f, "%s~\n", svs->ownername);
        }
        fprintf(f, "$~");
        fclose(f);
        return TRUE;
    }
    if (!IS_SET(ship->flags, LOADED)) {
        return FALSE;
    }
    if (IS_SET(ship->flags, MOB_SHIP)) {
        return TRUE;
    }
    sprintf(buf, "Ships/%s", ship->ownername);
    f = fopen(buf, "w");
    if (!f)
    {
        logit(LOG_FILE, "&+RError writing ship File!&N\n");
        return FALSE;
    }


    fprintf(f, "version:2\n");
    fprintf(f, "%d\n", ship->m_class);
    fprintf(f, "%s\n", ship->ownername);
    fprintf(f, "%s\n", ship->name);
    fprintf(f, "%d\n", ship->frags);
    fprintf(f, "%d %d\n", ship->anchor, ship->time);
    for (i = 0; i < 4; i++)
    {
        fprintf(f, "%d %d\n", ship->armor[i], ship->internal[i]);
    }
    fprintf(f, "%d\n", ship->mainsail);

    fprintf(f, "%d %d\n", ship->sailcrew.index, ship->sailcrew.skill);
    fprintf(f, "%d %d\n", ship->guncrew.index, ship->guncrew.skill);
    fprintf(f, "%d %d\n", ship->repaircrew.index, ship->repaircrew.skill);
    fprintf(f, "%d %d\n", ship->rowingcrew.index, ship->rowingcrew.skill);

    for (i = 0; i < MAXSLOTS; i++)
    {
        fprintf(f, "%d %d\n",
                ship->slot[i].type,
                ship->slot[i].index);
        fprintf(f, "%d %d\n",
                ship->slot[i].position,
                ship->slot[i].timer);
        fprintf(f, "%d %d %d %d %d\n",
                ship->slot[i].val0,
                ship->slot[i].val1,
                ship->slot[i].val2, 
                ship->slot[i].val3, 
                ship->slot[i].val4);
    }
    fclose(f);                          
    
    f = fopen("Ships/ship_index", "w");
    if (!f)
    {
        logit(LOG_FILE, "Ship index file open error.");
        return FALSE;
    }
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (IS_SET(svs->flags, LOADED))
            fprintf(f, "%s~\n", svs->ownername);
    }
    fprintf(f, "$~");
    fclose(f);
    return TRUE;
}

int read_newship()
{
    P_ship ship;
    char    *ret = NULL;
    int      i, k, intime, ver;
    FILE    *f = NULL, *f2 = NULL;

    f = fopen("Ships/ship_index", "r");
    if (!f) 
        {
        logit(LOG_FILE, "&+RError reading ship index!&N\n");
        return FALSE;
    }
    ret = fread_string(f);
    while (*ret != '$') 
    {
        sprintf(buf2, "Ships/%s", ret);
        ret = fread_string(f);
        f2 = fopen(buf2, "r");
        if (!f2) 
        {
            logit(LOG_FILE, "error reading ship file (couldn't open file %s)!\r\n", buf2);
            continue;
        }
        if ((k = fscanf(f2, "version:%d\n", &ver)) != 1)
            ver = 0;

        fscanf(f2, "%d\n", &k);
        ship = newship(k);
    
        if( !ship )
        {
            fclose(f2);
            continue;
        }

        
        if (ver == 2)
        {
            fgets(buf2, MAX_STRING_LENGTH, f2);
            i = 0;
            while (buf2[i] != '\0') 
            {
                if (buf2[i] == '\n') 
                {
                    buf2[i] = '\0';
                    break;
                }
                i++;
            }
            ship->ownername = str_dup(buf2);

            fgets(buf2, MAX_STRING_LENGTH, f2);
            i = 0;
            while (buf2[i] != '\0') 
            {
                if (buf2[i] == '\n') 
                {
                    buf2[i] = '\0';
                    break;
                }
                i++;
            }
            ship->name = str_dup(buf2);

            fscanf(f2, "%d\n", &(ship->frags));
            fscanf(f2, "%d %d\n", &(ship->anchor), &(ship->time));

            for (i = 0; i < 4; i++) 
            {
                fscanf(f2, "%d %d\n", &(ship->armor[i]), &(ship->internal[i]));
            }
            fscanf(f2, "%d\n", &(ship->mainsail));
            ship->mainsail = BOUNDED(0, ship->mainsail, SHIPMAXSAIL(ship));
                
            fscanf(f2, "%d %d\n", &(ship->sailcrew.index), &(ship->sailcrew.skill));
            fscanf(f2, "%d %d\n", &(ship->guncrew.index), &(ship->guncrew.skill));
            fscanf(f2, "%d %d\n", &(ship->repaircrew.index), &(ship->repaircrew.skill));
            fscanf(f2, "%d %d\n", &(ship->rowingcrew.index), &(ship->rowingcrew.skill));
            ship->sailcrew.update();
            ship->guncrew.update();
            ship->repaircrew.update();
            ship->rowingcrew.update();
            reset_crew_stamina(ship);

            for (i = 0; i < MAXSLOTS; i++)  
            {
                ship->slot[i].clear();

                fscanf(f2, "%d %d\n",
                    &(ship->slot[i].type),
                    &(ship->slot[i].index));
                fscanf(f2, "%d %d\n",
                    &(ship->slot[i].position),
                    &(ship->slot[i].timer));
                fscanf(f2, "%d %d %d %d %d\n",
                    &(ship->slot[i].val0),
                    &(ship->slot[i].val1),
                    &(ship->slot[i].val2), 
                    &(ship->slot[i].val3), 
                    &(ship->slot[i].val4));

                if (ship->slot[i].type == SLOT_WEAPON)
                {
                    if (ship->slot[i].timer < 0)
                        ship->slot[i].timer = 0;
                    ship->slot[i].val3 = -1;
                    ship->slot[i].val4 = -1;
                }
                else if (ship->slot[i].type == SLOT_CARGO || ship->slot[i].type == SLOT_CONTRABAND)
                {
                    ship->slot[i].val2 = -1;
                    ship->slot[i].val3 = -1;
                    ship->slot[i].val4 = -1;
                }
            }
        }
        else
        {
            logit(LOG_FILE, "Error reading ship: unknown version\r\n");
            fclose(f2);
            return FALSE;
        }
        nameship(ship->name, ship);
        intime = time(NULL);

        if (!loadship(ship, real_room0(ship->anchor)))
        {
            logit(LOG_FILE, "Error loading ship: %d.\r\n", shiperror);
            fclose(f2);
            return FALSE;
        }
        if ((intime - ship->time) > NEWSHIP_INACTIVITY && SHIPCLASS(ship) == 0)
        {
            SET_BIT(ship->flags, NEWSHIP_DELETE);
        }
    
        fclose(f2);

        setarmor(ship, false);
        update_crew(ship);
        reset_crew_stamina(ship);
        update_ship_status(ship);
    }

    fclose(f);
    return TRUE;
}

void newshipfrags()
{
    for (int i = 0; i < 10; i++)
    {
        int max = 0;
        shipfrags[i].ship = 0;
        ShipVisitor svs;
        for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
        {
            if (svs->frags >= max)
            {
                int exists = 0;
                for (int p = i - 1; p >= 0; p--)
                {
                    if (shipfrags[p].ship == svs)
                        exists = 1;
                }
                if (exists != 1)
                {
                    max = svs->frags;
                    shipfrags[i].ship = svs;
                }
            }
        }
    }
}

int anchor_room(int room)
{
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        for (int i = 0; i < MAX_SHIP_ROOM; i++)
        {
            if (room == svs->room[i].roomnum)
                return svs->anchor;
        }
    }
    return room;
}

void setcrew(P_ship ship, int crew_index, int skill)
{
    if (crew_index < 0 || crew_index > MAXCREWS)
        return;

    if (ship == NULL)
        return;

    switch (ship_crew_data[crew_index].type)
    {
    case SAIL_CREW:
      {
        ship->sailcrew.index = crew_index;
        ship->sailcrew.skill = skill;
        ship->sailcrew.update();
        ship->sailcrew.reset_stamina();
      }
      break;

    case GUN_CREW:
      {
        ship->guncrew.index = crew_index;
        ship->guncrew.skill = skill;
        ship->guncrew.update();
        ship->guncrew.reset_stamina();
      }
      break;

    case REPAIR_CREW:
      {
        ship->repaircrew.index = crew_index;
        ship->repaircrew.skill = skill;
        ship->repaircrew.update();
        ship->repaircrew.reset_stamina();
      }

    case ROWING_CREW:
      {
        ship->rowingcrew.index = crew_index;
        ship->rowingcrew.skill = skill;
        ship->rowingcrew.update();
        ship->rowingcrew.reset_stamina();
      }
    };
}

int crew_shop(int room, P_char ch, int cmd, char *arg)
{
    P_ship ship;
    int      owned, i, j;
    char     buf[MAX_STRING_LENGTH];

    if (!ch)
        return FALSE;
 
    if ((cmd != CMD_LIST) && (cmd != CMD_HIRE))
        return FALSE;

    owned = 0;
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (isname(GET_NAME(ch), svs->ownername))
        {
            ship = svs;
            owned = 1;
            break;
        }
    }

    if (!owned)
    {
        send_to_char("You own no ship, no one here wants to talk to you.\r\n", ch);
        return TRUE;
    }

    if (cmd == CMD_LIST)
    {
        send_to_char("&+YYou look around and see these people to hire:&N\r\n", ch);
        send_to_char("\r\n-=======Sailors=======-      Skill    Skill-Gain   Cost\r\n", ch);
        for (i = 1; i < MAXCREWS; i++)
        {
            j = sail_crew_list[i];
            if (j != -1 && j != SAIL_AUTOMATONS)
            {
                sprintf(buf,
                    "&+Y%d)%s %-20s&N       %4d       %1.2f      %s&N\r\n",
                    i, ship->frags < ship_crew_data[j].min_frags ? "&+L" : "&+W",
                    ship_crew_data[j].name, 
                    ship_crew_data[j].min_skill / 1000,
                    (float)ship_crew_data[j].skill_gain / 1000.0,
                    coin_stringv(ship_crew_data[j].hire_cost));
                send_to_char(buf, ch);
            }
        }
        send_to_char("\r\n-===Gunnery Experts===-      Skill    Skill-Gain   Stamina     Cost\r\n", ch);
        for (i = 1; i < MAXCREWS; i++)
        {
            j = gun_crew_list[i];
            if (j != -1 && j != GUN_AUTOMATONS)
            {
                sprintf(buf,
                    "&+Y%d)%s %-20s&N       %4d       %1.2f        %d       %s&N\r\n",
                    i, ship->frags < ship_crew_data[j].min_frags ? "&+L" : "&+W",
                    ship_crew_data[j].name, 
                    ship_crew_data[j].min_skill / 1000,
                    (float)ship_crew_data[j].skill_gain / 1000.0,
                    ship_crew_data[j].base_stamina,
                    coin_stringv(ship_crew_data[j].hire_cost));
                send_to_char(buf, ch);
            }
        }
        send_to_char("\r\n-=====Shipwrights=====-      Skill    Skill-Gain   Cost\r\n", ch);
        for (i = 1; i < MAXCREWS; i++)
        {
            j = repair_crew_list[i];
            if (j != -1)
            {
                sprintf(buf,
                    "&+Y%d)%s %-20s&N       %4d       %1.2f      %s&N\r\n",
                    i, ship->frags < ship_crew_data[j].min_frags ? "&+L" : "&+W",
                    ship_crew_data[j].name, 
                    ship_crew_data[j].min_skill / 1000,
                    (float)ship_crew_data[j].skill_gain / 1000.0,
                    coin_stringv(ship_crew_data[j].hire_cost));
                send_to_char(buf, ch);
            }
        }
        
        sprintf(buf, "\r\nCurrent Crew\r\n&+LSails:     &+W%-20s &+L(skill &+W%d&+L) \r\n&+LGuns:      &+W%-20s &+L(skill &+W%d&+L)\r\n&+LRepair:    &+W%-20s &+L(skill &+W%d&+L)&N\r\n",
          ship_crew_data[ship->sailcrew.index].name, ship->sailcrew.skill / 1000,
          ship_crew_data[ship->guncrew.index].name, ship->guncrew.skill / 1000,
          ship_crew_data[ship->repaircrew.index].name, ship->repaircrew.skill / 1000); 
        send_to_char(buf, ch);
        return TRUE;
    }

    if (cmd == CMD_HIRE)
    {
        half_chop(arg, arg1, arg3);
        half_chop(arg3, arg2, arg);

        if (!is_number(arg2))
        {
            send_to_char("Valid syntax 'hire <sail/gun/repair> <number>'\r\n", ch);
            return TRUE;
        }
        i = atoi(arg2);
        if (i < 1 && i > MAXCREWS)
        {
            send_to_char("Invalid number.\r\n", ch);
            return TRUE;
        }

        if (isname(arg1, "sail s"))
        {
            j = sail_crew_list[i];
        }
        else if (isname(arg1, "gun g"))
        {
            j = gun_crew_list[i];
        }
        else if (isname(arg1, "repair r"))
        {
            j = repair_crew_list[i];
        }
        else
        {
            send_to_char("Valid syntax 'hire <sail/gun/repair> <number>'\r\n", ch);
            return TRUE;
        }
        if (j == -1 || j == SAIL_AUTOMATONS || j == GUN_AUTOMATONS)
        {
            send_to_char("Invalid number.\r\n", ch);
            return TRUE;
        }

        if (ship->frags < ship_crew_data[j].min_frags)
        {
            send_to_char("Your reputation does not impress these guys.\r\n", ch);
            sprintf(buf, "You need at least %d frags to hire this crew.\r\n", ship_crew_data[j].min_frags);
            send_to_char(buf, ch);
            return TRUE;
        }
        int cost = ship_crew_data[j].hire_cost;
        if (GET_MONEY(ch) < cost)
        {
            sprintf(buf, "It will cost %s to hire this crew!\r\n", coin_stringv(cost));
            send_to_char(buf, ch);
            return TRUE;
        }

        int current_skill = 0;
        switch (ship_crew_data[j].type)
        {
        case SAIL_CREW:
            current_skill = ship->sailcrew.skill;
            break;
        case GUN_CREW:
            current_skill = ship->guncrew.skill;
            break;
        case REPAIR_CREW:
            current_skill = ship->repaircrew.skill;
            break;
        case ROWING_CREW:
            current_skill = ship->rowingcrew.skill;
            break;
        };
        if (current_skill > ship_crew_data[j].min_skill)
        {
            if (!arg || !(*arg) || !isname(arg, "confirm"))
            {
                send_to_char ("&+RYour current crew is more skilled!\r\nIf you are sure you want to hire this crew, type 'hire <sail/gun/repair> <number> confirm'.&N\r\n", ch);
                return TRUE;
            }
        }

        SUB_MONEY(ch, cost, 0);
        send_to_char ("Aye aye cap'n!  We'll be on yer ship before you board!\r\n", ch);
        setcrew(ship, j, 0);
        update_ship_status(ship);
        write_newship(ship);
        return TRUE;
    }
    return FALSE;
}

int erzul(P_char ch, P_char pl, int cmd, char *arg)
{
  int      cost;
  P_obj    obj;
  P_ship   ship = 0;
  char     buf[MAX_STRING_LENGTH];

  /*
   check for periodic event calls 
   */
  if (cmd == -10)
    return TRUE;

  if (GET_STAT(ch) <= STAT_SLEEPING)
    return FALSE;

  if (pl == ch)
    return FALSE;

  if (cmd == CMD_SAY)
  {
    if (*arg)
    {
        if (isname(arg, "hello hi quest help"))
        {
          send_to_char ("Erzul says 'That cursed &+WXexos&N!  Now I can't complete my &+Wmasterpiece&N!'\r\n", pl);
          return TRUE;
        }
        if (isname(arg, "xexos"))
        {
          send_to_char ("Erzul says 'Yes, Xexos, he stole my &+Wmoonstone&N heart and used it on his\r\naccursed creation!\r\n", pl);
          return TRUE;
        }
        if (isname(arg, "masterpiece moonstone"))
        {
          send_to_char ("Erzul says 'The moonstone was my life's work.  It's the power source I was\r\ngoing to use to power my &+Wautomoton&N, my masterpiece!'", pl);
          return TRUE;
        }
        if (isname(arg, "automoton"))
        {
          send_to_char ("Yes, if you can retrieve my moonstone, then I can create the ultimate crew for a ship!'\r\n", pl);
          return TRUE;
        }
    }
    return FALSE;
  }

  if (cmd == CMD_LIST)
  {
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
      if (isname(GET_NAME(pl), svs->ownername))
      {
        ship = svs;
        break;
      }
    }

    if (!ship)
    {
      send_to_char ("Erzul says 'You have no ship, I have no business with you!'\r\n", pl);
      return TRUE;
    }

    if (ship->frags < ship_crew_data[SAIL_AUTOMATONS].min_frags || ship->frags < ship_crew_data[GUN_AUTOMATONS].min_frags)
    {
      send_to_char ("Erzul says 'Who are you I've never heard of you, come back when you are\r\nmore well known.'\r\n", pl);
      return TRUE;
    }

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
      if (obj_index[obj->R_num].virtual_number == 12001)
        break;
    }
    if (!obj)
    {
      send_to_char ("Erzul says 'I'm sorry, but I can't make any magical automotons right now, maybe\r\nif you performed a quest for me, then I can sell you one for\r\n20000 &+Wplatinum&N'\r\n", pl);
      return TRUE;
    }

    send_to_char("Erzul says 'Now that I have the proper power source, I can sell you a \r\nmagical automoton for 20000 &+Wplatinum&N.'\r\n", pl);
    send_to_char("To buy, type 'buy <sail/gun crew>'\r\n", pl);
    return TRUE;
  }

  if (cmd == CMD_BUY)
  {
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
      if (isname(GET_NAME(pl), svs->ownername))
      {
        ship = svs;
        break;
      }
    }

    if (!ship)
    {
      send_to_char("Erzul says 'You have no ship, I have no business with you!'\r\n", pl);
      return TRUE;
    }

    if (ship->frags < ship_crew_data[SAIL_AUTOMATONS].min_frags || ship->frags < ship_crew_data[GUN_AUTOMATONS].min_frags )
    {
      send_to_char ("Erzul says 'Who are you I've never heard of you, come back when you are\r\nmore well known.'\r\n", pl);
      return TRUE;
    }

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
      if (obj_index[obj->R_num].virtual_number == 12001)
        break;
    }

    if (!obj)
    {  
      send_to_char ("Erzul says 'I'm sorry, but I can't make any magical automotons right now, maybe\r\nif you performed a quest for me, then I can sell you one for 20000 &+Wplatinum&N'\r\n", pl);
      return TRUE;
    }

    if (!*arg)
    {
      send_to_char ("Erzul says 'Buy what? Sail crew or Gun crew, they are made differently\r\nyou know.'\r\n", pl);
      return TRUE;
    }

    if (isname(arg, "sail s"))
    {
      cost = ship_crew_data[SAIL_AUTOMATONS].hire_cost;
      if (GET_MONEY(pl) < cost)
      {
        sprintf(buf, "Erzul says 'It will cost %s to make this crew!\r\n", coin_stringv(cost));
        send_to_char(buf, pl);
        return TRUE;
      }
      SUB_MONEY(pl, cost, 0);
      obj_from_char(obj, TRUE);
      extract_obj(obj, TRUE);
      obj = NULL;
      setcrew(ship, SAIL_AUTOMATONS, 0);
      update_ship_status(ship);
      write_newship(ship);
      send_to_char ("Erzul says 'They'll be at your ship by the time you get there!\r\n", pl);
      return TRUE;
    }
    else if (isname(arg, "gun g"))
    {
      cost = ship_crew_data[GUN_AUTOMATONS].hire_cost;
      if (GET_MONEY(pl) < cost)
      {
        sprintf(buf, "Erzul says 'It will cost %s to make this crew!\r\n", coin_stringv(cost));
        send_to_char(buf, pl);
        return TRUE;
      }
      SUB_MONEY(pl, cost, 0);
      obj_from_char(obj, TRUE);
      extract_obj(obj, TRUE);
      obj = NULL;
      setcrew(ship, GUN_AUTOMATONS, 0);
      update_ship_status(ship);
      write_newship(ship);
      send_to_char("Erzul says 'They'll be at your ship by the time you get there!\r\n",       pl);
      return TRUE;
    }
    else
    {
      send_to_char ("Erzul says 'Buy what? Sail crew or Gun crew, they are made differently\r\nyou know.'\r\n", pl);
      return TRUE;
    }
    return TRUE;
  }
  return FALSE;
}



              
