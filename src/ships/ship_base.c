/************************************************************
read_ships.c

New Ship system blah blah blah bug foo about it :P
Updated with warships. Nov08 -Lucrot 
*************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
    
#include "ships.h"
#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "ship_auto.h"
#include "ship_npc.h"
#include "ship_npc_ai.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include "limits.h"
#include "ctf.h"

extern char buf[MAX_STRING_LENGTH];

struct ContactData contacts[MAXSHIPS];
struct ShipMap tactical_map[101][101];
//char     status[20];
//char     position[20];
//char     contact[256];

char arg1[MAX_STRING_LENGTH];
char arg2[MAX_STRING_LENGTH];
char arg3[MAX_STRING_LENGTH];
char tmp_str[MAX_STRING_LENGTH];
int    shiperror;
struct ShipFragData shipfrags[20];

//--------------------------------------------------------------------
// load all ships from file into the world
//--------------------------------------------------------------------
void initialize_ships()
{
  obj_index[real_object0(60000)].func.obj = ship_panel_proc;
  obj_index[real_object0(60001)].func.obj = ship_obj_proc;
  obj_index[real_object0(1223)].func.obj = ship_cargo_info_stick;
  
  if (!read_ships())
  {
      logit(LOG_FILE, "Error reading ships from file!\r\n");
  }

  ShipVisitor svs;
  for (bool fn = shipObjHash.get_first(svs); fn; )
  {
      P_ship ship = svs;
      if (IS_SET(ship->flags, TO_DELETE)) 
      {
          fn = shipObjHash.erase(svs);
          delete_ship(ship);
      } 
      else
          fn = shipObjHash.get_next(svs);
  }

  initialize_ship_cargo();
  load_cyrics_revenge();
  //load_zone_ship();
  if (!load_moonstone_fragments())
  {
    logit(LOG_FILE, "Error initializing automatons quest!\r\n");
  }
}

//--------------------------------------------------------------------
// pre shutdown operations: dock all ships and put sailors to land 
//--------------------------------------------------------------------
void shutdown_ships()
{
    int      i;
    P_char   ch, ch_next;
    P_obj    obj, obj_next;

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        P_ship ship = svs;
        for (i = 0; i < ship->room_count; i++)
        {
            for (ch = world[real_room(SHIP_ROOM_NUM(ship, i))].people; ch; ch = ch_next)
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
        write_ship(ship);
    }
}

//--------------------------------------------------------------------
// create new ship of given class
//--------------------------------------------------------------------
struct ShipData *new_ship(int m_class, bool npc)
{
   if (shipObjHash.size() >= MAXSHIPS) 
   {
       shiperror = 6;   
       return NULL;   // AT MAX FOR GAME
   }

   // Make a new ship
   P_ship ship = NULL;
   CREATE(ship, ShipData, 1, MEM_TAG_SHIPDAT);
   
   ship->shipobj = read_object(60001, VIRTUAL);
   if (ship->shipobj == NULL) {
      shiperror = 16;
      return NULL;
   }
   shipObjHash.add(ship);

   // Set up the new ship
   ship->autopilot = 0;
   ship->npc_ai = 0;
   ship->panel = read_object(60000, VIRTUAL);
   if (ship->panel == NULL) 
   {
      shiperror = 17;
      return NULL;
   }
   ship->m_class = m_class;
   set_ship_armor(ship, true);
   ship->frags = 0;
   ship->name = NULL;
   ship->x = 50;
   ship->y = 50;
   ship->z = 0;
   ship->flags = 0;
   ship->money = 0;
   ship->target = 0;
   ship->mainsail = SHIPTYPE_MAX_SAIL(m_class);
   ship->repair = SHIP_HULL_WEIGHT(ship);
   ship->setheading = ship->heading = 0;
   ship->setspeed = ship->speed = 0;

   ship->maxspeed_bonus = 0;
   ship->capacity_bonus = 0;

   ship->time = time(NULL);

   for (int j = 0; j < MAXSLOTS; j++) 
   {
      ship->slot[j].clear();
   }

   set_crew(ship, DEFAULT_CREW);
   ship->crew.sail_chief = NO_CHIEF;
   ship->crew.guns_chief = NO_CHIEF;
   ship->crew.rpar_chief = NO_CHIEF;

   assignid(ship, "**");
   ship->keywords = str_dup("ship");

   init_ship_layout(ship);
   set_ship_layout(ship, m_class);
   return ship;
}


//--------------------------------------------------------------------
// set ship object names with ship name in them
//--------------------------------------------------------------------
void name_ship(const char *name, P_ship ship)
{
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
   sprintf(buf, "&+yThe %s&N %s&n &+yfloats here.", SHIP_CLASS_NAME(ship), name);
   ship->shipobj->description = str_dup(buf);
   sprintf(buf, "&+yThe %s&N %s&n", SHIP_CLASS_NAME(ship), name);
   ship->shipobj->short_description = str_dup(buf);
   sprintf(buf, "ship %s %s", SHIP_CLASS_NAME(ship), ship->ownername);
   ship->shipobj->name = str_dup(buf);
   ship->keywords = str_dup(buf);

}

void name_ship_rooms(P_ship ship)
{
   for (int i = 0; i < ship->room_count; i++)
   {
     if (SHIP_ROOM_NUM(ship, i) == -1)
       continue;

     int rroom = real_room0(SHIP_ROOM_NUM(ship, i));
     if (!rroom)
     {
        shiperror = 4;
        return;
     }

     if (i == 0) 
     {
        sprintf(buf, "&+ROn the &+WBridge&N&+R of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
     } 
     else 
     {
        sprintf(buf, "&+yAboard the %s&N %s", SHIP_CLASS_NAME(ship), ship->name);
     }
     if (ship->m_class == SH_CORVETTE) 
     {
        if (i == 1 || i == 3 || i == 4) 
        {
           sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
        } 
     }
     if (ship->m_class == SH_DESTROYER) 
     {
        if (i == 1 || i == 2 || i == 3) 
        {
           sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
        } 
     }
     if (ship->m_class == SH_FRIGATE) 
     {
        if (i == 1 || i == 2 || i == 3) 
        {
           sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
        } 
     }
     if (ship->m_class == SH_CRUISER) 
     {
        if (i == 9) 
        {
           sprintf(buf, "&+YDocking Bay&+y of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
        } 
        if (i == 7 || i == 10 || i == 11) 
        {
           sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
        } 
     }
     else if (ship->m_class == SH_DREADNOUGHT)
     {

        if (i == 14) 
        {
           sprintf(buf, "&+YDocking Bay&+y of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
        } 
        if (i == 7) 
        {
           sprintf(buf, "&+YSpacious Hold&+y of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
        } 
        if (i == 3 || i == 8 || i == 9 || i == 10) 
        {
           sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s", SHIP_CLASS_NAME(ship), ship->name);
        } 
     }

     if (world[rroom].name) {
        world[rroom].name = NULL;
     }

     world[rroom].name = str_dup(buf);
   }
}


bool rename_ship(P_char ch, char *owner_name, char *new_name)
{
   P_ship temp;
   
   temp = get_ship_from_owner(owner_name);
   if( !temp )
   {
      if( isname(GET_NAME(ch), owner_name) )
      {
         send_to_char("You do not own a ship yet, buy one first!\n", ch);
      }
      else
      {
         send_to_char_f(ch, "%s does not own a ship!\n", owner_name);
      }

      return FALSE;
   }

   if( IS_TRUSTED(ch) )
   {
      if( !is_valid_ansi(new_name, FALSE) )
      {
         send_to_char("Invalid ANSI characters in name.\n", ch);
         return FALSE;
      }
   }
   else
   {
      if ((int) strlen(strip_ansi(new_name).c_str()) > 20)
      {
         send_to_char("Name must be less than 20 chars (not including ansi))\r\n", ch);
         return FALSE;
      }

      if( !is_valid_ansi_with_msg(ch, new_name, FALSE) )
      {
         return FALSE;
      }
   }

   name_ship(new_name, temp);
   name_ship_rooms(temp);
   write_ship(temp);

   return TRUE;
}

bool rename_ship_owner(char *old_name, char *new_name)
{
   P_ship ship;
   
   ship = get_ship_from_owner(old_name);
   if( !ship )
      return TRUE;

   str_free(ship->ownername);
   ship->ownername = str_dup(new_name);
   name_ship(SHIP_NAME(ship), ship);
   write_ship(ship);
   write_ships_index(); // reset index file

   sprintf(buf, "Ships/%s", old_name);
   unlink(buf);

   return TRUE;
}


//--------------------------------------------------------------------
// load ship into the world
//--------------------------------------------------------------------
int load_ship(P_ship ship, int to_room)
{
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


    if (!set_ship_physical_layout(ship))
    {
        shiperror = 4;
        return FALSE;
    }


    /*for (int i = 0; i < MAX_SHIP_ROOM; i++) 
    {
      if (SHIP_ROOM_NUM(ship, i) != -1) 
      {
         int rroom = real_room0(SHIP_ROOM_NUM(ship, i));
         if (!rroom) 
         {
            shiperror = 4;
            return FALSE;
         }

         world[rroom].funct = ship_room_proc;
         for (int dir = 0; dir < NUM_EXITS; dir++) 
         {
            if (SHIP_ROOM_EXIT(ship, i, dir) != -1) 
            {
               if (!world[rroom].dir_option[dir])
                  CREATE(world[rroom].dir_option[dir], room_direction_data, 1, MEM_TAG_DIRDATA);

               world[rroom].dir_option[dir]->to_room = real_room0(SHIP_ROOM_EXIT(ship, i, dir));
               world[rroom].dir_option[dir]->exit_info = 0;
            } 
            else 
            {
               if (world[rroom].dir_option[dir]) 
                  world[rroom].dir_option[dir]->to_room = -1;
            }
         }
      }
   }*/

   if(IS_SET(ship->flags, SINKING)) 
     REMOVE_BIT(ship->flags, SINKING); 
   if(IS_SET(ship->flags, FLYING)) 
     REMOVE_BIT(ship->flags, FLYING); 
   if(IS_SET(ship->flags, SUNKBYNPC)) 
     REMOVE_BIT(ship->flags, SUNKBYNPC); 
   if(IS_SET(ship->flags, ATTACKBYNPC)) 
     REMOVE_BIT(ship->flags, ATTACKBYNPC); 
   if(IS_SET(ship->flags, RAMMING)) 
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
   if (SHIP_OBJ(ship)) 
   {
      ship->z = 0;
      SHIP_OBJ(ship)->value[6] = 1;
      obj_to_room(SHIP_OBJ(ship), to_room);
   } 
   else 
   {
      shiperror = 5;
      return FALSE;
   }
   ship->anchor = world[to_room].number;
   ship->location = to_room;

   update_crew(ship);
   reset_crew_stamina(ship);
   update_ship_status(ship);
   return TRUE;
}


//--------------------------------------------------------------------
// delete ship completely
//--------------------------------------------------------------------
void delete_ship(P_ship ship, bool npc)
{
    char fname[256];

    clear_ship_layout(ship);

    clear_references_to_ship(ship);

    if (!npc)
    {
        write_ships_index();
        sprintf(fname, "Ships/%s", ship->ownername);
        unlink(fname);
    }

    obj_from_room(ship->panel);
    obj_from_room(ship->shipobj);
    extract_obj(ship->panel, TRUE);
    extract_obj(ship->shipobj, TRUE);
    ship->panel = NULL;
    ship->shipobj = NULL;
    if (ship->npc_ai)
        delete ship->npc_ai;
    if (ship == cyrics_revenge)
        cyrics_revenge = 0;
    //if( ship == zone_ship )
    //    zone_ship = 0;
        

    logit(LOG_STATUS, "Ship \"%s\" (%s) deleted", strip_ansi(ship->name).c_str(), ship->ownername);
    
    FREE(ship);
}

void clear_references_to_ship(P_ship ship)
{
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (svs->target == ship)
        {
            svs->target = NULL;
            act_to_all_in_ship(svs, "&+RTarget lost.\r\n");
        }
    }
}

//--------------------------------------------------------------------
// set ship rooms layout
//--------------------------------------------------------------------
void set_ship_layout(P_ship ship, int m_class)
{
//    int to_room = 0;

    ship->bridge = 0;
    switch (m_class) {
    case SH_SLOOP:
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 1;
        SHIP_ROOM_EXIT(ship, 1, NORTH) = 0;
        ship->entrance = 1;
        ship->room_count = 2;
        break;

    case SH_YACHT:
        SHIP_ROOM_EXIT(ship, 0, NORTH) = 1;
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 2;
        SHIP_ROOM_EXIT(ship, 1, SOUTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, NORTH) = 0;
        ship->entrance = 2;
        ship->room_count = 3;
        break;

    case SH_CLIPPER:
        SHIP_ROOM_EXIT(ship, 0, NORTH) = 1;
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 2;
        SHIP_ROOM_EXIT(ship, 1, SOUTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, SOUTH) = 3;
        SHIP_ROOM_EXIT(ship, 3, NORTH) = 2;
        ship->entrance = 3;
        ship->room_count = 4;
        break;

    case SH_KETCH:
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 2;
        SHIP_ROOM_EXIT(ship, 1, EAST)  = 2;
        SHIP_ROOM_EXIT(ship, 2, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, EAST)  = 3;
        SHIP_ROOM_EXIT(ship, 2, WEST)  = 1;
        SHIP_ROOM_EXIT(ship, 3, WEST)  = 2;
        ship->entrance = 2;
        ship->room_count = 4;
        break;

    case SH_CARAVEL:
        SHIP_ROOM_EXIT(ship, 0, NORTH) = 1;
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 5;
        SHIP_ROOM_EXIT(ship, 0, EAST)  = 3;
        SHIP_ROOM_EXIT(ship, 0, WEST)  = 2;
        SHIP_ROOM_EXIT(ship, 1, SOUTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, SOUTH) = 4;
        SHIP_ROOM_EXIT(ship, 2, EAST)  = 0;
        SHIP_ROOM_EXIT(ship, 3, SOUTH) = 6;
        SHIP_ROOM_EXIT(ship, 3, WEST)  = 0;
        SHIP_ROOM_EXIT(ship, 4, NORTH) = 2;
        SHIP_ROOM_EXIT(ship, 4, EAST)  = 5;
        SHIP_ROOM_EXIT(ship, 5, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 5, EAST)  = 6;
        SHIP_ROOM_EXIT(ship, 5, WEST)  = 4;
        SHIP_ROOM_EXIT(ship, 6, NORTH) = 3;
        SHIP_ROOM_EXIT(ship, 6, WEST)  = 5;
        ship->entrance = 5;
        ship->room_count = 7;
        break;

    case SH_CARRACK:
        SHIP_ROOM_EXIT(ship, 0, NORTH) = 1;
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 5;
        SHIP_ROOM_EXIT(ship, 0, EAST)  = 3;
        SHIP_ROOM_EXIT(ship, 0, WEST)  = 2;
        SHIP_ROOM_EXIT(ship, 1, SOUTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, SOUTH) = 4;
        SHIP_ROOM_EXIT(ship, 2, EAST)  = 0;
        SHIP_ROOM_EXIT(ship, 3, SOUTH) = 6;
        SHIP_ROOM_EXIT(ship, 3, WEST)  = 0;
        SHIP_ROOM_EXIT(ship, 4, NORTH) = 2;
        SHIP_ROOM_EXIT(ship, 4, EAST)  = 5;
        SHIP_ROOM_EXIT(ship, 5, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 5, SOUTH) = 7;
        SHIP_ROOM_EXIT(ship, 5, EAST)  = 6;
        SHIP_ROOM_EXIT(ship, 5, WEST)  = 4;
        SHIP_ROOM_EXIT(ship, 6, NORTH) = 3;
        SHIP_ROOM_EXIT(ship, 6, WEST)  = 5;
        SHIP_ROOM_EXIT(ship, 7, NORTH) = 5;
        ship->entrance = 7;
        ship->room_count = 8;
        break;

    case SH_GALLEON:
        SHIP_ROOM_EXIT(ship, 0, NORTH) = 1;
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 5;
        SHIP_ROOM_EXIT(ship, 0, EAST)  = 3;
        SHIP_ROOM_EXIT(ship, 0, WEST)  = 2;
        SHIP_ROOM_EXIT(ship, 1, SOUTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, SOUTH) = 4;
        SHIP_ROOM_EXIT(ship, 2, EAST)  = 0;
        SHIP_ROOM_EXIT(ship, 3, SOUTH) = 6;
        SHIP_ROOM_EXIT(ship, 3, WEST)  = 0;
        SHIP_ROOM_EXIT(ship, 4, NORTH) = 2;
        SHIP_ROOM_EXIT(ship, 4, SOUTH) = 7;
        SHIP_ROOM_EXIT(ship, 4, EAST)  = 5;
        SHIP_ROOM_EXIT(ship, 5, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 5, SOUTH) = 8;
        SHIP_ROOM_EXIT(ship, 5, EAST)  = 6;
        SHIP_ROOM_EXIT(ship, 5, WEST)  = 4;
        SHIP_ROOM_EXIT(ship, 6, NORTH) = 3;
        SHIP_ROOM_EXIT(ship, 6, SOUTH) = 9;
        SHIP_ROOM_EXIT(ship, 6, WEST)  = 5;
        SHIP_ROOM_EXIT(ship, 7, NORTH) = 4;
        SHIP_ROOM_EXIT(ship, 7, EAST)  = 8;
        SHIP_ROOM_EXIT(ship, 8, NORTH) = 5;
        SHIP_ROOM_EXIT(ship, 8, EAST)  = 9;
        SHIP_ROOM_EXIT(ship, 8, WEST)  = 7;
        SHIP_ROOM_EXIT(ship, 9, NORTH) = 6;
        SHIP_ROOM_EXIT(ship, 9, WEST)  = 8;
        ship->entrance = 8;
        ship->room_count = 10;
        break;

    case SH_CORVETTE:
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 2;
        SHIP_ROOM_EXIT(ship, 0, WEST)  = 1;
        SHIP_ROOM_EXIT(ship, 0, NORTH) = 4;
        SHIP_ROOM_EXIT(ship, 0, EAST)  = 3;
        SHIP_ROOM_EXIT(ship, 1, EAST)  = 0;
        SHIP_ROOM_EXIT(ship, 2, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 3, WEST)  = 0;
        SHIP_ROOM_EXIT(ship, 4, SOUTH) = 0;
        ship->entrance = 2;
        ship->room_count = 5;
        break;

    case SH_DESTROYER:
        SHIP_ROOM_EXIT(ship, 0, NORTH) = 1;
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 7;
        SHIP_ROOM_EXIT(ship, 0, EAST)  = 3;
        SHIP_ROOM_EXIT(ship, 0, WEST)  = 2;
        SHIP_ROOM_EXIT(ship, 1, SOUTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, EAST)  = 0;
        SHIP_ROOM_EXIT(ship, 3, WEST)  = 0;
        SHIP_ROOM_EXIT(ship, 4, EAST)  = 5;
        SHIP_ROOM_EXIT(ship, 5, NORTH) = 7;
        SHIP_ROOM_EXIT(ship, 5, EAST)  = 6;
        SHIP_ROOM_EXIT(ship, 5, WEST)  = 4;
        SHIP_ROOM_EXIT(ship, 6, WEST)  = 5;
        SHIP_ROOM_EXIT(ship, 7, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 7, SOUTH) = 5;
        ship->entrance = 5;
        ship->room_count = 8;
        break;

    case SH_FRIGATE:
        SHIP_ROOM_EXIT(ship, 0, NORTH) = 1;
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 8;
        SHIP_ROOM_EXIT(ship, 0, EAST)  = 3;
        SHIP_ROOM_EXIT(ship, 0, WEST)  = 2;
        SHIP_ROOM_EXIT(ship, 1, SOUTH) = 0;
        SHIP_ROOM_EXIT(ship, 2, SOUTH) = 4;
        SHIP_ROOM_EXIT(ship, 2, EAST)  = 0;
        SHIP_ROOM_EXIT(ship, 3, SOUTH) = 6;
        SHIP_ROOM_EXIT(ship, 3, WEST)  = 0;
        SHIP_ROOM_EXIT(ship, 4, NORTH) = 2;
        SHIP_ROOM_EXIT(ship, 4, EAST)  = 5;
        SHIP_ROOM_EXIT(ship, 5, NORTH) = 8;
        SHIP_ROOM_EXIT(ship, 5, SOUTH) = 7;
        SHIP_ROOM_EXIT(ship, 5, EAST)  = 6;
        SHIP_ROOM_EXIT(ship, 5, WEST)  = 4;
        SHIP_ROOM_EXIT(ship, 6, NORTH) = 3;
        SHIP_ROOM_EXIT(ship, 6, WEST)  = 5;
        SHIP_ROOM_EXIT(ship, 7, NORTH) = 5;
        SHIP_ROOM_EXIT(ship, 8, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 8, SOUTH) = 5;
        ship->entrance = 7;
        ship->room_count = 9;
        break;

    case SH_CRUISER:
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 1;
        SHIP_ROOM_EXIT(ship, 1, NORTH) = 0;
        SHIP_ROOM_EXIT(ship, 1, DOWN)  = 3;
        SHIP_ROOM_EXIT(ship, 2, EAST)  = 3;
        SHIP_ROOM_EXIT(ship, 3, WEST)  = 2;
        SHIP_ROOM_EXIT(ship, 3, EAST)  = 4;
        SHIP_ROOM_EXIT(ship, 3, NORTH) = 5;
        SHIP_ROOM_EXIT(ship, 3, UP)    = 1;
        SHIP_ROOM_EXIT(ship, 4, WEST)  = 3;
        SHIP_ROOM_EXIT(ship, 5, NORTH) = 6;
        SHIP_ROOM_EXIT(ship, 5, SOUTH) = 3;
        SHIP_ROOM_EXIT(ship, 5, DOWN)  = 8;
        SHIP_ROOM_EXIT(ship, 6, NORTH) = 7;
        SHIP_ROOM_EXIT(ship, 6, SOUTH) = 5;
        SHIP_ROOM_EXIT(ship, 7, SOUTH) = 6;
        SHIP_ROOM_EXIT(ship, 8, NORTH) = 9;
        SHIP_ROOM_EXIT(ship, 8, UP)    = 5;
        SHIP_ROOM_EXIT(ship, 9, SOUTH) = 8;
        SHIP_ROOM_EXIT(ship, 6, EAST)  = 10; // laungh
        SHIP_ROOM_EXIT(ship,10, WEST)  = 6;
        SHIP_ROOM_EXIT(ship, 6, WEST)  = 11; // laungh
        SHIP_ROOM_EXIT(ship,11, EAST)  = 6;
        ship->entrance = 9;
        ship->room_count = 12;
        break;

    case SH_DREADNOUGHT:
        SHIP_ROOM_EXIT(ship, 0, DOWN)  = 6;
        SHIP_ROOM_EXIT(ship, 1, SOUTH) = 2;
        SHIP_ROOM_EXIT(ship, 1, NORTH) = 3; // launch
        SHIP_ROOM_EXIT(ship, 1, WEST)  = 4;
        SHIP_ROOM_EXIT(ship, 1, EAST)  = 5;
        SHIP_ROOM_EXIT(ship, 1, DOWN)  = 11;
        SHIP_ROOM_EXIT(ship, 2, NORTH) = 1;
        SHIP_ROOM_EXIT(ship, 2, SOUTH) = 6;
        SHIP_ROOM_EXIT(ship, 2, DOWN)  = 7; // hold
        SHIP_ROOM_EXIT(ship, 2, WEST)  = 8; // launch
        SHIP_ROOM_EXIT(ship, 2, EAST)  = 9; // launch
        SHIP_ROOM_EXIT(ship, 6, UP)    = 0;
        SHIP_ROOM_EXIT(ship, 6, NORTH) = 2;
        SHIP_ROOM_EXIT(ship, 6, SOUTH) = 10;
        SHIP_ROOM_EXIT(ship, 6, WEST)  = 12;
        SHIP_ROOM_EXIT(ship, 6, EAST)  = 13;
        SHIP_ROOM_EXIT(ship,11, UP)    = 1;
        SHIP_ROOM_EXIT(ship,11, NORTH) = 14;
        SHIP_ROOM_EXIT(ship, 3, SOUTH) = 1;
        SHIP_ROOM_EXIT(ship, 4, EAST)  = 1;
        SHIP_ROOM_EXIT(ship, 4, SOUTH) = 8;
        SHIP_ROOM_EXIT(ship, 5, WEST)  = 1;
        SHIP_ROOM_EXIT(ship, 5, SOUTH) = 9;
        SHIP_ROOM_EXIT(ship, 7, UP)    = 2;
        SHIP_ROOM_EXIT(ship, 8, EAST)  = 2;
        SHIP_ROOM_EXIT(ship, 8, SOUTH) = 12;
        SHIP_ROOM_EXIT(ship, 8, NORTH) = 4;
        SHIP_ROOM_EXIT(ship, 9, WEST)  = 2;
        SHIP_ROOM_EXIT(ship, 9, SOUTH) = 13;
        SHIP_ROOM_EXIT(ship, 9, NORTH) = 5;
        SHIP_ROOM_EXIT(ship,10, NORTH) = 6;
        SHIP_ROOM_EXIT(ship,12, EAST)  = 6;
        SHIP_ROOM_EXIT(ship,12, NORTH) = 8;
        SHIP_ROOM_EXIT(ship,13, WEST)  = 6;
        SHIP_ROOM_EXIT(ship,13, NORTH) = 9;
        SHIP_ROOM_EXIT(ship,14, SOUTH) = 11;

        ship->entrance = 14;
        ship->room_count = 15;
        break;
    case SH_ZONE_SHIP:
        SHIP_ROOM_EXIT(ship, 0, SOUTH) = 1;
        SHIP_ROOM_EXIT(ship, 1, NORTH) = 1;

        ship->entrance = 1;
        ship->room_count = 2;
        break;
    default:
        break;
    }
}

void init_ship_layout(P_ship ship)
{
  for (int j = 0; j < MAX_SHIP_ROOM; j++) 
  {
    for (int dir = 0; dir < NUM_EXITS; dir++) 
      SHIP_ROOM_EXIT(ship, j, dir) = -1;
    SHIP_ROOM_NUM(ship, j) = -1;
  }
}

void clear_ship_layout(P_ship ship)
{
  for (int j = 0; j < MAX_SHIP_ROOM; j++) 
  {
    if (SHIP_ROOM_NUM(ship, j) != -1)
    {
      int rroom = real_room0(SHIP_ROOM_NUM(ship, j));
      if (!rroom) continue;
      for (int dir = 0; dir < NUM_EXITS; dir++) 
      {
        if (world[rroom].dir_option[dir])
          FREE(world[rroom].dir_option[dir]);
        world[rroom].dir_option[dir] = 0;
        SHIP_ROOM_EXIT(ship, j, dir) = -1;
      }
      world[rroom].funct = NULL;
    }
    SHIP_ROOM_NUM(ship, j) = -1;
  }
  ship->bridge = -1;
  ship->entrance = -1;
  ship->room_count = 0;
}

int find_free_ship_room()
{
   int room = (SHIPZONE * 100), rroom;
   while ((rroom = real_room0(room))) 
   {
      if (world[rroom].funct != ship_room_proc)
        return room;
      room ++;
   }
   return 0;
}

bool set_ship_physical_layout(P_ship ship)
{
    for (int j = 0; j < ship->room_count; j++) 
    {
        int vroom = find_free_ship_room();
        if (!vroom) return false;
        int rroom = real_room0(vroom);
        if (!rroom)
            return FALSE;
        SHIP_ROOM_NUM(ship, j) = vroom;
        world[rroom].funct = ship_room_proc;
    }
    for (int j = 0; j < ship->room_count; j++) 
    {
        int rroom = real_room0(SHIP_ROOM_NUM(ship, j));
        if (!rroom)
            return FALSE;

        for (int dir = 0; dir < NUM_EXITS; dir++) 
        {
            if (SHIP_ROOM_EXIT(ship, j, dir) != -1) 
            {
                int to_room = real_room0(SHIP_ROOM_NUM(ship, SHIP_ROOM_EXIT(ship, j, dir)));
                if (!to_room)
                    return FALSE;

                if (!world[rroom].dir_option[dir]) 
                    CREATE(world[rroom].dir_option[dir], room_direction_data, 1, MEM_TAG_DIRDATA);

                world[rroom].dir_option[dir]->to_room = to_room;
                world[rroom].dir_option[dir]->exit_info = 0;
            } 
            else 
            {
                if (world[rroom].dir_option[dir])
                {
                    FREE(world[rroom].dir_option[dir]);
                    world[rroom].dir_option[dir] = 0;
                }
            }
        }
    }
    ship->bridge = SHIP_ROOM_NUM(ship, 0);
    ship->entrance = SHIP_ROOM_NUM(ship, ship->entrance);
    name_ship_rooms(ship);

    // Set entrance to/exit from zone to ship to zone here.
    /*
    if( ship == zone_ship )
    {
      int to_room = real_room( ZONE_SHIP_ZONE_ENTRANCE );
      if( !to_room )
        fprintf(stderr, "Failed to link zone ship to zone.\r\n");
      else
      {
        world[real_room(ship->room[1].roomnum)].dir_option[NORTH]->to_room = to_room;
        if (!world[to_room].dir_option[SOUTH]) 
          CREATE(world[to_room].dir_option[SOUTH], room_direction_data, 1, MEM_TAG_DIRDATA);
        world[to_room].dir_option[SOUTH]->to_room = real_room(ship->room[1].roomnum);
        world[to_room].dir_option[SOUTH]->exit_info = 0;
      }
    }
    */
    return TRUE;
}


//--------------------------------------------------------------------
// (re)set ships armor
//--------------------------------------------------------------------
void set_ship_armor(P_ship ship, bool equal)
{
    ship->maxarmor[SIDE_FORE] = ship_arc_properties[ship->m_class].armor[SIDE_FORE];
    ship->maxarmor[SIDE_PORT] = ship_arc_properties[ship->m_class].armor[SIDE_PORT];
    ship->maxarmor[SIDE_STAR] = ship_arc_properties[ship->m_class].armor[SIDE_STAR];
    ship->maxarmor[SIDE_REAR] = ship_arc_properties[ship->m_class].armor[SIDE_REAR];

    ship->maxinternal[SIDE_FORE] = ship_arc_properties[ship->m_class].internal[SIDE_FORE];
    ship->maxinternal[SIDE_PORT] = ship_arc_properties[ship->m_class].internal[SIDE_PORT];
    ship->maxinternal[SIDE_STAR] = ship_arc_properties[ship->m_class].internal[SIDE_STAR];
    ship->maxinternal[SIDE_REAR] = ship_arc_properties[ship->m_class].internal[SIDE_REAR];

    if (equal)
    {
        ship->armor[SIDE_FORE] = ship->maxarmor[SIDE_FORE];
        ship->armor[SIDE_PORT] = ship->maxarmor[SIDE_PORT];
        ship->armor[SIDE_STAR] = ship->maxarmor[SIDE_STAR];
        ship->armor[SIDE_REAR] = ship->maxarmor[SIDE_REAR];

        ship->internal[SIDE_FORE] = ship->maxinternal[SIDE_FORE];
        ship->internal[SIDE_PORT] = ship->maxinternal[SIDE_PORT];
        ship->internal[SIDE_STAR] = ship->maxinternal[SIDE_STAR];
        ship->internal[SIDE_REAR] = ship->maxinternal[SIDE_REAR];
    }
    else
    {
        ship->armor[SIDE_FORE] = MIN(ship->maxarmor[SIDE_FORE], ship->armor[SIDE_FORE]);
        ship->armor[SIDE_PORT] = MIN(ship->maxarmor[SIDE_PORT], ship->armor[SIDE_PORT]);
        ship->armor[SIDE_STAR] = MIN(ship->maxarmor[SIDE_STAR], ship->armor[SIDE_STAR]);
        ship->armor[SIDE_REAR] = MIN(ship->maxarmor[SIDE_REAR], ship->armor[SIDE_REAR]);

        ship->internal[SIDE_FORE] = MIN(ship->maxinternal[SIDE_FORE], ship->internal[SIDE_FORE]);
        ship->internal[SIDE_PORT] = MIN(ship->maxinternal[SIDE_PORT], ship->internal[SIDE_PORT]);
        ship->internal[SIDE_STAR] = MIN(ship->maxinternal[SIDE_STAR], ship->internal[SIDE_STAR]);
        ship->internal[SIDE_REAR] = MIN(ship->maxinternal[SIDE_REAR], ship->internal[SIDE_REAR]);
    }
}


//--------------------------------------------------------------------
// reset ship state to its class default
//--------------------------------------------------------------------
void reset_ship(P_ship ship, bool clear_slots)
{
    set_ship_armor(ship, true);
    ship->mainsail = SHIPTYPE_MAX_SAIL(ship->m_class);
    ship->repair = SHIPTYPE_HULL_WEIGHT(ship->m_class);

    obj_from_room(ship->panel);
    name_ship(ship->name, ship);
    clear_ship_layout(ship);
    set_ship_layout(ship, ship->m_class);
    set_ship_physical_layout(ship);
    obj_to_room(ship->panel, real_room0(ship->bridge));

    ship->timer[T_UNDOCK] = 0; 
    ship->timer[T_MANEUVER] = 0; 
    ship->timer[T_SINKING] = 0; 
    ship->timer[T_BSTATION] = 0; 
    ship->timer[T_RAM] = 0; 
    ship->timer[T_RAM_WEAPONS] = 0; 
    ship->timer[T_MAINTENANCE] = 0; 
    ship->timer[T_MINDBLAST] = 0; 
                 
    if(IS_SET(ship->flags, SINKING)) 
        REMOVE_BIT(ship->flags, SINKING); 
    if(IS_SET(ship->flags, FLYING)) 
        REMOVE_BIT(ship->flags, FLYING); 
    if(IS_SET(ship->flags, SUNKBYNPC)) 
        REMOVE_BIT(ship->flags, SUNKBYNPC); 
    if(IS_SET(ship->flags, ATTACKBYNPC)) 
        REMOVE_BIT(ship->flags, ATTACKBYNPC); 
    if(IS_SET(ship->flags, RAMMING)) 
        REMOVE_BIT(ship->flags, RAMMING); 
    if(IS_SET(ship->flags, ANCHOR)) 
        REMOVE_BIT(ship->flags, ANCHOR); 
    if(IS_SET(ship->flags, SUMMONED)) 
        REMOVE_BIT(ship->flags, SUMMONED); 

    if (clear_slots)
    {
        for (int j = 0; j < MAXSLOTS; j++)
            ship->slot[j].clear();
    }
}


//--------------------------------------------------------------------
// This proc is added to rooms inside ship
//--------------------------------------------------------------------

int ship_room_proc(int room, P_char ch, int cmd, char *arg)
{
   int i, j, k;
   P_ship ship;
   int virt;
   
   if(!ch)
     return false;
        
   if ((cmd != CMD_LOOK) && (cmd != CMD_DISEMBARK))
     return(FALSE);
 
   ship = get_ship_from_char(ch);

   if (cmd == CMD_LOOK)
   {
     if (!arg || !*arg || str_cmp(skip_spaces(arg), "out"))
        return(FALSE);
     look_out_ship(ship, ch);
     return(TRUE);
   }
   if (cmd == CMD_DISEMBARK)
   {
      if(IS_IMMOBILE(ch))
      {
        send_to_char("\r\nYou cannot disembark in your present condition.\r\n", ch);
        return false;
      }

      if(SHIP_FLYING(ship) && !IS_TRUSTED(ch))
      {
        send_to_char("\r\nYou cannot disembark in air!\r\n", ch);
        return false;
      }

      if(IS_BLIND(ch) && number(0, 5))
      {
        send_to_char("&+WIt is hard to disembark when you cannot see anything... but you keep trying!\r\n", ch);
        return false;
      }

      if (ship->m_class == SH_CRUISER || ship->m_class == SH_DREADNOUGHT)
       {
    //      if (SHIP_DOCKED(ship) || SHIP_ANCHORED(ship))
          {
             if (world[ch->in_room].number == ship->entrance || IS_TRUSTED(ch))
             {
                if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL) || IS_FIGHTING(ch)) 
                {
                   send_to_char("You're in no position to disembark!\r\n", ch);
                   return(TRUE);
                }
                act("You step off the docking bay of this ship.", FALSE, ch, 0, 0, TO_CHAR);
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
       if (SHIP_ROOM_EXIT(ship, j, NORTH) == -1 ||
          SHIP_ROOM_EXIT(ship, j, SOUTH) == -1 ||
          SHIP_ROOM_EXIT(ship, j, EAST) == -1 ||
          SHIP_ROOM_EXIT(ship, j, WEST) == -1) {
          k = 1;
       }

	if (world[ch->in_room].number != ship->entrance && !IS_TRUSTED(ch))
       {
	  if (!k && !IS_TRUSTED(ch)) 
	    {	
            send_to_char ("You are not close enough to the edge of the ship to jump out!\r\n", ch);
            return TRUE;
           }
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
// This proc is added to all ship objects.
//--------------------------------------------------------------------
int ship_obj_proc(P_obj obj, P_char ch, int cmd, char *arg)
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

    if (SHIP_FLYING(ship) && !IS_TRUSTED(ch))
    {
       send_to_char("&+RThat ship flies too high to board!&n\r\n", ch);
       return TRUE;
    }
    if (IS_WARSHIP(ship) && ship->speed > 0 && !SHIP_SINKING(ship) && !IS_TRUSTED(ch))
    {
       send_to_char("&+RThat ship is moving too fast to board!&n\r\n", ch);
       return TRUE;
    }
    else if (ship->speed > BOARDING_SPEED && !SHIP_SINKING(ship) && !IS_TRUSTED(ch))
    {
       send_to_char("&+RThat ship is moving too fast to board!&n\r\n", ch);
       return TRUE;
    }
    else if (ship->m_class == SH_CRUISER || ship->m_class == SH_DREADNOUGHT)
    {
//            if (SHIP_DOCKED(ship) || SHIP_ANCHORED(ship))
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
//                send_to_char("This ship's hatches are tightly shut and the armor is too tough to break through!\r\n", ch);
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


bool is_npc_ship_name (const char*);
bool check_ship_name(P_ship ship, P_char ch, char* name)
{
    if (ship && IS_NPC_SHIP(ship))
        return true;

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (svs == ship || IS_NPC_SHIP(svs))
            continue;
        if (!strcmp(strip_ansi(name).c_str(), strip_ansi(svs->name).c_str()))
        {
            if (!ship || ship->frags <= svs->frags)
            {
                send_to_char("Another player already has a ship with such name, choose another name for your ship!\r\n", ch);
                return false;
            }
        }
    }
    if (is_npc_ship_name(name))
    {
        send_to_char("This name is reserved, choose another name for your ship!\r\n", ch);
        return false;
    }
    return true;    
}    


bool check_undocking_conditions(P_ship ship, int m_class, P_char ch)
{
    int arc_weapons[4], arc_weapon_weight[4];

    if (!check_ship_name(ship, ch, ship->name))
        return false;

    for (int a = 0; a < 4; a++)
    {
        arc_weapons[a] = 0;
        arc_weapon_weight[a] = 0;
    }

    for (int sl = 0; sl < MAXSLOTS; sl++) 
    {
        if (ship->slot[sl].type == SLOT_WEAPON)
        {
            arc_weapons[ship->slot[sl].position]++;
            arc_weapon_weight[ship->slot[sl].position] += weapon_data[ship->slot[sl].index].weight;
            if (!ship_allowed_weapons[m_class][ship->slot[sl].index])
            {
                send_to_char_f(ch, "Remove weapon [%d], it is not allowed with this hull!\r\n", sl);
                return FALSE;
            }
        }
    }
    for (int a = 0; a < 4; a++)
    {
      if (arc_weapons[a] > ship_arc_properties[m_class].max_weapon_slots[a])
      {
          send_to_char_f(ch, 
              "Your have too many weapons at one side!\r\nMaximum allowed weapons for this ship is:\r\nFore: %d  Starboard: %d  Port: %d  Rear: %d\r\n",
              ship_arc_properties[m_class].max_weapon_slots[SIDE_FORE], 
              ship_arc_properties[m_class].max_weapon_slots[SIDE_STAR], 
              ship_arc_properties[m_class].max_weapon_slots[SIDE_PORT], 
              ship_arc_properties[m_class].max_weapon_slots[SIDE_REAR]);
          return FALSE;
      }
      if (arc_weapon_weight[a] > ship_arc_properties[m_class].max_weapon_weight[a])
      {
          send_to_char_f(ch, 
              "Your have overloaded one side with weapons!\r\nMaximum allowed weapon weight for this ship is:\r\nFore: %d  Starboard: %d  Port: %d  Rear: %d\r\n",
              ship_arc_properties[m_class].max_weapon_weight[SIDE_FORE], 
              ship_arc_properties[m_class].max_weapon_weight[SIDE_STAR], 
              ship_arc_properties[m_class].max_weapon_weight[SIDE_PORT], 
              ship_arc_properties[m_class].max_weapon_weight[SIDE_REAR]);
          return FALSE;
      }
    }

    if (SHIPTYPE_MIN_LEVEL(m_class) > GET_LEVEL(ch)) 
    {
        send_to_char ("You are too low for such a big ship! Get more experience or downgrade the hull!'\r\n", ch);
        return FALSE;
    }
    return TRUE;
}



//--------------------------------------------------------------------
// Per-clock ship activity
//--------------------------------------------------------------------
void ship_activity()
{
    int        j, k, loc;
    float        rad;

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        P_ship ship = svs;

        // Check timers
        if (!SHIP_LOADED(ship)) 
            continue;



        if (ship->timer[T_RAM_WEAPONS] == 1)
        {
            act_to_all_in_ship(ship, "Your gun crew has recovered from ram impact.&N");
        }
        if (ship->timer[T_RAM] == 1)
        {
            act_to_all_in_ship(ship, "Your crew has returned to their battle stations.");
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
            if (ship->crew.stamina < ship->crew.max_stamina)
            {
                float stamina_inc = 3;
                if (IS_NPC_SHIP(ship))
                    stamina_inc = 4;
                if (ship == cyrics_revenge)
                    stamina_inc = 15;
                //if (ship == zone_ship)
                //    stamina_inc = 15;
                if (SHIP_DOCKED(ship) || SHIP_ANCHORED(ship))
                    stamina_inc *= 4;

                ship->crew.stamina += stamina_inc;
                if (ship->crew.stamina > ship->crew.max_stamina)
                    ship->crew.stamina = ship->crew.max_stamina;
            }
            if (ship->crew.stamina < 0)
            {
                if (number(1, 30) == 1)
                {
                    if (ship->crew.stamina > -ship->crew.max_stamina)
                        act_to_all_in_ship(ship, "&+rYour crew looks exhausted.&N\r\n");
                    else
                        act_to_all_in_ship(ship, "&+rYour crew looks &+Rcompletely &+rexhausted.&N\r\n");
                }
            }


            // Battle Stations!
            if (ship->target != NULL)
                ship->timer[T_BSTATION] = BSTATION;
            if (ship->timer[T_BSTATION] == 1)
                act_to_all_in_ship(ship, "Your crew stands down from battlestations.\r\n");

            // Repairing
            if ((ship->repair > 0) && ship->timer[T_MINDBLAST] == 0) 
            {
                float chance = 0.0;
                if (ship->mainsail < (int)((float)SHIP_MAX_SAIL(ship) * (ship->crew.rpar_mod_applied + 0.4)) && 
                        ship->mainsail < SHIP_MAX_SAIL(ship) * 0.9)
                {
                    if (SHIP_ANCHORED(ship))
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
                        else                    chance = 0.0;
                    }
                    chance *= (1.0 + ship->crew.rpar_mod_applied) * ship->crew.get_stamina_mod();
                    if (number (0, 1000) < (int)chance)
                    {
                        ship->mainsail += MIN(ship->crew.get_sail_repair_mod(), (SHIP_MAX_SAIL(ship) - ship->mainsail));
                        ship->repair--;
                        if (ship->repair == 0)
                            act_to_all_in_ship(ship, "&+RThe ship is out of repair materials!.&N");
                        ship->crew.reduce_stamina(3, ship);
                        ship->crew.rpar_skill_raise((ship->timer[T_BSTATION] > 0 && HAS_VALID_TARGET(ship)) ? 0.1 : 0.01);
                        update_ship_status(ship);
                    }
                } 
                for (j = 0; j < MAXSLOTS; j++) 
                {
                    if (ship->repair < 1)
                        break;
                    if (ship->slot[j].type == SLOT_WEAPON)
                    {
                        if (SHIP_WEAPON_DAMAGED(ship, j) && !SHIP_WEAPON_DESTROYED(ship, j))
                        {
                            chance = 200.0;
                            chance *= (1.0 + ship->crew.rpar_mod_applied) * ship->crew.get_stamina_mod();
                            if (number (0, 999) < (int)chance)
                            {
                                ship->slot[j].val2 -= MIN(ship->crew.get_weapon_repair_mod(), ship->slot[j].val2);
                                if (ship->slot[j].val2 < 0) ship->slot[j].val2 = 0;
                                if (!SHIP_WEAPON_DAMAGED(ship, j))
                                {
                                    act_to_all_in_ship_f(ship, "&+W%s &+Ghas been repaired!&N", weapon_data[ship->slot[j].index].name);
                                    if (ship->slot[j].val1 > 0)
                                        ship->slot[j].timer = (int)((float)weapon_data[j].reload_time * (1.0 - ship->crew.guns_mod_applied * 0.15));
                                }
                                if (!number(0, 4))
                                    ship->repair--;
                                if (ship->repair == 0)
                                    act_to_all_in_ship(ship, "&+RThe ship is out of repair materials!.&N");
                                ship->crew.reduce_stamina(1, ship);
                                ship->crew.rpar_skill_raise((ship->timer[T_BSTATION] > 0 && HAS_VALID_TARGET(ship)) ? 0.1 : 0.01);
                            }
                        }
                    }
                }
                bool can_repair_internal = false;
                for (j = 0; j < 4; j++) 
                {
                    if (ship->repair < 1)
                        break;
                    if (ship->internal[j] < (int)((float)ship->maxinternal[j] * (ship->crew.rpar_mod_applied + 0.1)) &&
                            ship->internal[j] < ship->maxinternal[j] * 0.9) 
                    {
                        can_repair_internal = true;
                        if (SHIP_ANCHORED(ship))
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
                        chance *= (1.0 + ship->crew.rpar_mod_applied) * ship->crew.get_stamina_mod();
                        if (number (0, 1000) < (int)chance)
                        {
                            ship->internal[j] += MIN(ship->crew.get_hull_repair_mod(), (ship->maxinternal[j] - ship->internal[j]));
                            ship->repair--;
                            if (ship->repair == 0)
                                act_to_all_in_ship(ship, "&+RThe ship is out of repair materials!.&N");
                            ship->crew.reduce_stamina(2, ship);
                            ship->crew.rpar_skill_raise((ship->timer[T_BSTATION] > 0 && HAS_VALID_TARGET(ship)) ? 0.1 : 0.01);
                            update_ship_status(ship);
                        }
                    }
                }
                if (!can_repair_internal && ship->timer[T_BSTATION] == 0 && ship->crew.rpar_mod_applied > 0.5)
                { // highly skilled crew can repair some armor when not in combat
                    for (j = 0; j < 4; j++) 
                    {
                        if (ship->repair < 1)
                            break;
                        if (ship->armor[j] < (int)((float)ship->maxarmor[j] * (ship->crew.rpar_mod_applied - 0.5)) &&
                                ship->armor[j] < ship->maxarmor[j] * 0.9) 
                        {
                            if (SHIP_ANCHORED(ship))
                            {
                                chance = 150.0;
                            }
                            else
                            {
                                chance = 20.0;
                            }
                            chance *= (1.0 + ship->crew.rpar_mod_applied) * ship->crew.get_stamina_mod();
                            if (number (0, 1000) < (int)chance)
                            {
                                ship->armor[j] += MIN(ship->crew.get_hull_repair_mod(), (ship->maxarmor[j] - ship->armor[j]));
                                ship->repair--;
                                if (ship->armor == 0)
                                    act_to_all_in_ship(ship, "&+RThe ship is out of repair materials!.&N");
                                ship->crew.reduce_stamina(4, ship);
                                ship->crew.rpar_skill_raise((ship->timer[T_BSTATION] > 0 && HAS_VALID_TARGET(ship)) ? 0.1 : 0.01);
                                update_ship_status(ship);
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
                if (SHIP_ANCHORED(ship) && !SHIP_DOCKED(ship))
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
        if (ship->timer[T_MAINTENANCE] == 1) 
        {
            
        }

        //Undocked and non sinking actions go below here
        if (!SHIP_SINKING(ship) &&
            !SHIP_DOCKED(ship) &&
            !SHIP_ANCHORED(ship)) 
        {
            if (IS_WATER_ROOM(ship->location) || IS_SET(world [ship->location].room_flags, DOCKABLE) || SHIP_FLYING(ship))
            {
                // Setspeed to Speed DRANNAK
               // if (ship->setspeed > ship->get_maxspeed()) 
    P_char ch = captain_is_aboard(ship);
    int realspeed = ship->get_maxspeed();
    
    if(ch){
    if(has_innate(ch, INNATE_SEADOG))
    realspeed += 2;
    }

		 if (ship->setspeed > realspeed)
                {
                    ship->setspeed = realspeed;
                }
                if (ship->setspeed != ship->speed && ship->timer[T_MINDBLAST] == 0) 
                {
                    int sp_change = get_next_speed_change(ship);
                    ship->speed += sp_change;

                    // affect crew stamina
                    float sp_rel_change = ((float)ABS(sp_change) / (float)SHIP_ACCEL(ship)) / (1.0 + ship->crew.sail_mod_applied);
                    ship->crew.reduce_stamina(sp_rel_change * (2.0 + SHIP_HULL_MOD(ship) / 10.0), ship);
                }

                // SetHeading to Heading
                if(ship->setheading != ship->heading && ship->timer[T_MINDBLAST] == 0) 
                {
                    float hd_change = get_next_heading_change(ship);
                    ship->heading += hd_change;
                    normalize_direction(ship->heading);

                    // affect crew stamina
                    float hd_rel_change = (ABS(hd_change) / (float)SHIP_HDDC(ship)) / (1.0 + ship->crew.sail_mod_applied);
                    if (SHIP_IMMOBILE(ship)) hd_rel_change *= 5;
                    ship->crew.reduce_stamina(hd_rel_change * (3.0 + SHIP_HULL_MOD(ship) / 10.0), ship);
                }
            }

            // Movement Goes here
            if (ship->speed != 0) 
            {
                rad = ship->heading * M_PI / 180.000;
                ship->x += (float) ((float) ship->speed * sin(rad)) / 150.000;
                ship->y += (float) ((float) ship->speed * cos(rad)) / 150.000;

                if ((ship->y >= 51.000) || (ship->x >= 51.000) || (ship->y < 50.000) || (ship->x < 50.000)) 
                {
                    if (getmap(ship))
                    {
                        if (SHIP_CLASS(ship) != SH_SLOOP && SHIP_CLASS(ship) != SH_YACHT)
                            ship->crew.sail_skill_raise(0.003);
                        loc = tactical_map[(int) ship->x][100 - (int) ship->y].rroom;
                        if (is_valid_sailing_location(ship, loc))
                        {
                            if (ship->x > 50.999) 
                            {
                                ship->x -= 1.000;
                                if (SHIP_FLYING(ship))
                                {
                                    send_to_room_f(ship->location, "%s&N floats east above you.\r\n", ship->name);
                                    send_to_room_f(loc, "%s&N floats in from the west above you.\r\n", ship->name);
                                }
                                else
                                {
                                    send_to_room_f(ship->location, "%s&N sails east.\r\n", ship->name);
                                    send_to_room_f(loc, "%s&N sails in from the west.\r\n", ship->name);
                                }
                            } 
                            else if (ship->x < 50.000) 
                            {
                                ship->x += 1.000;
                                if (SHIP_FLYING(ship))
                                {
                                    send_to_room_f(ship->location, "%s&N floats west above you.\r\n", ship->name);
                                    send_to_room_f(loc, "%s&N floats in from the east above you.\r\n", ship->name);
                                }
                                else
                                {
                                    send_to_room_f(ship->location, "%s&N sails west.\r\n", ship->name);
                                    send_to_room_f(loc, "%s&N sails in from the east.\r\n", ship->name);
                                }
                            }
        
                            if (ship->y > 50.999) 
                            {
                                ship->y -= 1.000;
                                if (SHIP_FLYING(ship))
                                {
                                    send_to_room_f(ship->location, "%s&N floats north above you.\r\n", ship->name);
                                    send_to_room_f(loc, "%s&N floats in from the south above you.\r\n", ship->name);
                                }
                                else
                                {
                                    send_to_room_f(ship->location, "%s&N sails north.\r\n", ship->name);
                                    send_to_room_f(loc, "%s&N sails in from the south.\r\n", ship->name);
                                }
                            } 
                            else if (ship->y < 50.000) 
                            {
                                ship->y += 1.000;
                                if (SHIP_FLYING(ship))
                                {
                                    send_to_room_f(ship->location, "%s&N floats south above you.\r\n", ship->name);
                                    send_to_room_f(loc, "%s&N floats in from the north above you.\r\n", ship->name);
                                }
                                else
                                {
                                    send_to_room_f(ship->location, "%s&N sails south.\r\n", ship->name);
                                    send_to_room_f(loc, "%s&N sails in from the north.\r\n", ship->name);
                                }
                            }
                            if (SHIP_OBJ(ship) && (loc != ship->location)) 
                            {
                                ship->location = loc;
                                obj_from_room(SHIP_OBJ(ship));
                                obj_to_room(SHIP_OBJ(ship), loc);
                                everyone_look_out_ship(ship);
                            }
                        } 
                        else
                        {
                            ship->setspeed = 0;
                            ship->speed = 0;
                            ship->x = 50.500;
                            ship->y = 50.500;
        
                            if (ship->autopilot) 
                                stop_autopilot(ship);
        
                            int crash_chance = (ship->timer[T_BSTATION] == 0) ? 0 :
                                (int)((float)(ship->speed + 50) / ((1.0 + ship->crew.sail_mod_applied * 2.0) * ship->crew.get_stamina_mod()) );
        
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
                                try_ram_ship(ship, ship->target, contacts[j].bearing);
                            }
                        }
                    }
                }
            }


            // Slot timers
            if (!IS_SET(ship->flags, RAMMING) && ship->timer[T_RAM_WEAPONS] == 0 && ship->timer[T_MINDBLAST] == 0)
            {
                for (j = 0; j < MAXSLOTS; j++) 
                {
                    if (ship->slot[j].type == SLOT_WEAPON) 
                    {
                        if (ship->slot[j].timer > 0) 
                        {
                            if (number(0, 99) >= int (ship->crew.get_stamina_mod() * 100))
                                continue; 
                            ship->slot[j].timer--;
                            if (ship->slot[j].timer == 0) 
                            {
                                act_to_all_in_ship_f(ship, "Weapon &+W[%d]&N: [%s] has finished reloading.", j, ship->slot[j].get_description());
                            }
                            // affect crew stamina
                            ship->crew.reduce_stamina((float)weapon_data[ship->slot[j].index].weight / SHIP_HULL_MOD(ship), ship);
                            if (HAS_VALID_TARGET(ship))
                                ship->crew.guns_skill_raise(0.003);
                        }
                    }
                    if (ship->slot[j].type == SLOT_EQUIPMENT) 
                    {
                        if (ship->slot[j].timer > 0) 
                        {
                            ship->slot[j].timer--;
                            if (ship->slot[j].timer == 0)
                            {
                                if (ship->slot[j].index == E_LEVISTONE && !IS_SET(ship->flags, AIR))
                                {
                                    if (SHIP_FLYING(ship))
                                        land_ship(ship);
                                    else
                                        act_to_all_in_ship_f(ship, "%s is fully recharged.", ship->slot[j].get_description());
                                }
                            }
                        }
                    }
                }
            }

            if (ship->autopilot)
                autopilot_activity(ship);
            if (ship->npc_ai)
                ship->npc_ai->activity();
	     
            if (has_eq_diplomat(ship))
		{
		 if (ship->target == 0 && ship->speed > 0 && number(0, get_property("ships.pirate.diplomat.load.chance", 30000)) == 0)
                try_load_pirate_ship(ship);
		}
	     else
		{
		  if (ship->target == 0 && ship->speed > 0 && number(0, get_property("ships.pirate.load.chance", 7200)) == 0)
                try_load_pirate_ship(ship);
		}

        }
    }
}

void dock_ship(P_ship ship, int to_room)
{
    // Add in Docking event
    act_to_all_in_ship(ship, "Your ship begins docking procedures.");
    if ((real_room0(world[to_room].number) == to_room) && (to_room != 0)) 
    {
        ship->anchor = world[to_room].number;
        write_ship(ship);
    }
    if (to_room == 0) 
    {
        act_to_all_in_ship(ship, "ERROR: Room is void, moving back to anchor point.");
        ship->location = real_room0(ship->anchor);
        obj_from_room(ship->shipobj);
        obj_to_room(ship->shipobj, ship->location);
    }
    if (ship->target != NULL)
        ship->target = NULL;

    clear_references_to_ship(ship);

    ship->location = to_room;
    ship->repair = SHIPTYPE_HULL_WEIGHT(ship->m_class);
    assignid(ship, "**");
    act_to_all_in_ship(ship, "Your ship has completed docking procedures.");
    SET_BIT(ship->flags, DOCKED);
    if(IS_SET(ship->flags, ATTACKBYNPC)) 
      REMOVE_BIT(ship->flags, ATTACKBYNPC); 
    reset_crew_stamina(ship);
    update_ship_status(ship);
}


void crash_land(P_ship ship)
{
    act_to_all_in_ship(ship, "&+yCRUNCH!! Your ship crashes into land!&N");
    act_to_outside_ships(ship, NULL, DEFAULT_RANGE, "&+W[%s]&N:%s&N crashes into land!", SHIP_ID(ship), ship->name);
    int hits = (SHIP_HULL_WEIGHT(ship) / 25) + 1;
    for (int k = 0; k < hits; k++) 
    {
        if (k == 0 || number(1, 2) == 2)
        {
            int arc = (k == 0) ? SIDE_FORE : number(0, 5);
            int dam = number(1, 9);
            if (arc < 4)
            {
                damage_hull(NULL, ship, dam, arc, 0);
            }
            else  // sails
            {
                damage_sail(NULL, ship, dam);
            }
        }
    }
    update_ship_status(ship);
}

void finish_sinking(P_ship ship)
{
    // The zone ship does not sink completely.
    //if( ship == zone_ship )
    //  return;

    if (IS_NPC_SHIP(ship) && pc_is_aboard(ship))
    {
        ship->timer[T_SINKING] = 30;
        return;
    }
    if (IS_WATER_ROOM(ship->location))
    {
        act_to_all_in_ship(ship, "&+yYour ship sinks and you swim out in time!\r\n");
        act_to_outside(ship, 10, "%s &+yhas sunk to the depths of the ocean!\r\n", SHIP_NAME(ship));
        act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]:&N %s&N&+y sinks under the ocean.\r\n", SHIP_ID(ship), SHIP_NAME(ship));
    }
    else
    {
        act_to_all_in_ship(ship, "&+yYour ship falls apart and you jump out in time!\r\n");
        act_to_outside(ship, 10, "%s &+yhas fallen to pieces!\r\n", SHIP_NAME(ship));
        act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]:&N %s&N&+y falls to pieces.\r\n", SHIP_ID(ship), SHIP_NAME(ship));
    }
    clear_ship_content(ship);
    jettison_all(ship);

    if (!IS_NPC_SHIP(ship))
    {
        int insurance = 0;
        if (ship->m_class != SH_SLOOP) // no insurance for sloops
        {
            if (SHIP_SUNK_BY_NPC(ship))
            {
                insurance = (int)(SHIPTYPE_COST(ship->m_class) * 0.90); // if sunk by NPC, you loose same amount as for switching hulls
            }
            else if (IS_MERCHANT(ship))
            {
                insurance = (int)(SHIPTYPE_COST(ship->m_class) * 0.75);
            }
            else if (IS_WARSHIP(ship))
                insurance = (int)(SHIPTYPE_COST(ship->m_class) * 0.50);  // only partial insurance for warships
        }


        if (P_char owner = get_char2(str_dup(SHIP_OWNER(ship))))
        {
            GET_BALANCE_PLATINUM(owner) += insurance / 1000;
            wizlog(56, "Ship insurance to account: %d", insurance / 1000);
            logit(LOG_SHIP, "%s's insurance deposit to account: %d", ship->ownername, insurance / 1000);
        }
        else
        {
            ship->money = insurance; // if owner is not online, money go into ships coffer
            wizlog(56, "Ship insurance to ship's coffer: %d", insurance / 1000);
            logit(LOG_SHIP, "%s's insurance to ship's coffer: %d", ship->ownername, insurance / 1000);
        }

        int old_class = ship->m_class;
        ship->m_class = SH_SLOOP; // all ships become sloops after sinking
        reset_ship(ship);

        if (old_class == SH_SLOOP)
           ship->mainsail = 0;  // have to pay at least something...

        ship->speed = 0;
        ship->setspeed = 0;
    
        // Holding room in Sea Kingdom

        obj_from_room(ship->shipobj);
        obj_to_room(ship->shipobj, DAVY_JONES_LOCKER);
    
        ship->location = DAVY_JONES_LOCKER;
        dock_ship(ship, DAVY_JONES_LOCKER);

        reset_crew_stamina(ship);
        update_ship_status(ship);
        write_ship(ship);
    }
    else
    {
        shipObjHash.erase(ship);
        delete_ship(ship);
    }
}

void summon_ship_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      to_room;
  if (sscanf((const char *) data, "%s %d", buf, &to_room) == 2)
  {
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
      P_ship ship = svs;
      if (isname(buf, ship->ownername) && ship->timer[T_BSTATION] == 0 && !SHIP_SINKING(ship))
      {
        ship->location = to_room;
        obj_to_room(ship->shipobj, to_room);
        send_to_room_f(to_room, "&+y%s arrives at port.\r\n&N", ship->name);
        dock_ship(ship, to_room);
        check_contraband(ship, ship->location);
        REMOVE_BIT(ship->flags, SUMMONED);
        if(IS_SET(ship->flags, ATTACKBYNPC)) 
          REMOVE_BIT(ship->flags, ATTACKBYNPC); 
        ship->speed = 0;
        ship->setspeed = 0;
        write_ship(ship);
        return;
      }
    }
  }
}

void fly_ship(P_ship ship)
{
    if (!IS_SET(ship->flags, FLYING))
        SET_BIT(ship->flags, FLYING);
    if (!IS_SET(ship->flags, AIR))
    {
        int levi_slot = eq_levistone_slot(ship);
        ship->slot[levi_slot].timer = LEVISTONE_TIME;
        act_to_all_in_ship_f(ship, "&+W%s &+Ghums and glows with soft &+Cblue light.\r\n", ship->slot[levi_slot].get_description());
    }
    act_to_all_in_ship(ship, "&+WYour ship slowly ascends and floats in air!&N\r\n");
    act_to_outside(ship, 10, "%s &+Wslowly ascends and floats in air!&N", SHIP_NAME(ship));
    act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]:&N %s&N &+Wslowly ascends and floats in air!&N\r\n", SHIP_ID(ship), SHIP_NAME(ship));

    ship->shipobj->z_cord = 4;
    update_ship_status(ship);
}

void land_ship(P_ship ship)
{
    if (IS_SET(ship->flags, FLYING))
        REMOVE_BIT(ship->flags, FLYING);
    if (!IS_SET(ship->flags, AIR))
    {
        int levi_slot = eq_levistone_slot(ship);
        ship->slot[levi_slot].timer = LEVISTONE_RECHARGE;
        act_to_all_in_ship_f(ship, "&+W%s &+Ldims and becomes silent.\r\n", ship->slot[levi_slot].get_description());
    }
    if (IS_WATER_ROOM(ship->location))
    {
        act_to_all_in_ship(ship, "&+WYour ship slowly descends and lands with a loud &+Bsplash&+W!&N\r\n");
        act_to_outside(ship, 10, "%s &+Wslowly descends and lands with a loud &+Bsplash&+W!&N", SHIP_NAME(ship));
        act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]:&N %s&N &+Wslowly descends and lands with a loud &+Bsplash&+W!&N\r\n", SHIP_ID(ship), SHIP_NAME(ship));
    }
    else
    {
        ship->setspeed = 0;
        ship->speed = 0;

        act_to_all_in_ship(ship, "&+WYour ship slowly descends and lands with &+Ycreaking &+Wsounds!&N\r\n");
        act_to_outside(ship, 10, "%s &+Wslowly descends and lands with a &+Ycreaking &+Wsounds!&N", SHIP_NAME(ship));
        act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]:&N %s&N &+Wslowly descends and lands with a &+Ycreaking &+Wsounds!&N\r\n", SHIP_ID(ship), SHIP_NAME(ship));
    }

    ship->shipobj->z_cord = 0;
    update_ship_status(ship);
}

//--------------------------------------------------------------------
// Writing ships to disk
//--------------------------------------------------------------------
int write_ships_index()
{
    FILE* f = fopen("Ships/ship_index", "w");
    if (!f)
    {
        logit(LOG_FILE, "Ship index file open error.");
        return FALSE;
    }
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (SHIP_LOADED(svs) && !IS_NPC_SHIP(svs))
            fprintf(f, "%s~\n", svs->ownername);
    }
    fprintf(f, "$~");
    fclose(f);
    return TRUE;
}

int write_ship(P_ship ship)
{
    char     buf[MAX_STRING_LENGTH];
    int      i;
    FILE    *f = NULL;

    if (IS_NPC_SHIP(ship)) {
        return FALSE;
    }
    if (!SHIP_LOADED(ship)) {
        return FALSE;
    }
    sprintf(buf, "Ships/%s", ship->ownername);
    f = fopen(buf, "w");
    if (!f)
    {
        logit(LOG_FILE, "&+RError writing ship File!&N\n");
        return FALSE;
    }


    fprintf(f, "version:3\n");
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

    fprintf(f, "%d\n", ship->crew.index);
    fprintf(f, "%d %d %d 0 0 0\n", (int)(ship->crew.sail_skill * 1000), (int)(ship->crew.guns_skill * 1000), (int)(ship->crew.rpar_skill * 1000));
    fprintf(f, "%d %d %d 0 0 0 0 0 0 0\n", ship->crew.sail_chief, ship->crew.guns_chief, ship->crew.rpar_chief);

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
        if (SHIP_LOADED(svs))
            fprintf(f, "%s~\n", svs->ownername);
    }
    fprintf(f, "$~");
    fclose(f);
    return TRUE;
}

//--------------------------------------------------------------------
// Reading ships from disk
//--------------------------------------------------------------------
int read_ships()
{
    P_ship ship;
    char    *ret = NULL;
    int      k, ver;//, intime;
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
        sprintf(buf, "Ships/%s", ret);
        ret = fread_string(f);
        f2 = fopen(buf, "r");
        if (!f2) 
        {
            logit(LOG_FILE, "error reading ship file (couldn't open file %s)!\r\n", buf);
            continue;
        }
        if ((k = fscanf(f2, "version:%d\n", &ver)) != 1)
            ver = 0;

        fscanf(f2, "%d\n", &k);
        ship = new_ship(k);
    
        if( !ship )
        {
            fclose(f2);
            continue;
        }

        
        if (ver == 3)
        {
            fgets(buf, MAX_STRING_LENGTH, f2);
            for (int i = 0; buf[i] != '\0'; i++) 
                if (buf[i] == '\n') { buf[i] = '\0'; break; }

            ship->ownername = str_dup(buf);

            fgets(buf, MAX_STRING_LENGTH, f2);
            for (int i = 0; buf[i] != '\0'; i++) 
                if (buf[i] == '\n') { buf[i] = '\0'; break; }

            ship->name = str_dup(buf);

            fscanf(f2, "%d\n", &(ship->frags));
            fscanf(f2, "%d %d\n", &(ship->anchor), &(ship->time));

            for (int i = 0; i < 4; i++) 
            {
                fscanf(f2, "%d %d\n", &(ship->armor[i]), &(ship->internal[i]));
            }
            fscanf(f2, "%d\n", &(ship->mainsail));
            ship->mainsail = BOUNDED(0, ship->mainsail, SHIP_MAX_SAIL(ship));
                
            if (ver == 3)
            {
                int dummy, ss, gs, rs;
                fscanf(f2, "%d\n", &(ship->crew.index));
                fscanf(f2, "%d %d %d %d %d %d\n", &(ss), &(gs), &(rs), &dummy, &dummy, &dummy);
                ship->crew.sail_skill = (float)ss / 1000;
                ship->crew.guns_skill = (float)gs / 1000;
                ship->crew.rpar_skill = (float)rs / 1000;
                fscanf(f2, "%d %d %d %d %d %d %d %d %d %d\n", &(ship->crew.sail_chief), &(ship->crew.guns_chief), &(ship->crew.rpar_chief), &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
            }
            update_crew(ship);
            reset_crew_stamina(ship);

            for (int i = 0; i < MAXSLOTS; i++)  
                ship->slot[i].clear();

            for (int i = 0; i < MAXSLOTS; i++)  
            {
                if (fscanf(f2, "%d %d\n",
                    &(ship->slot[i].type),
                    &(ship->slot[i].index)) != 2)
                {
                    break;
                }
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
        //intime = time(NULL);
        name_ship(ship->name, ship);

        if (!load_ship(ship, real_room0(ship->anchor)))
        {
            logit(LOG_FILE, "Error loading ship: %d.\r\n", shiperror);
            fclose(f2);
            return FALSE;
        }
        /*if ((intime - ship->time) > NEWSHIP_INACTIVITY && SHIP_CLASS(ship) == 0)
        {
            SET_BIT(ship->flags, TO_DELETE);
        }*/
    
        fclose(f2);

        set_ship_armor(ship, false);
        update_crew(ship);
        reset_crew_stamina(ship);
        update_ship_status(ship);
    }

    fclose(f);
    return TRUE;
}



//--------------------------------------------------------------------
// Top-frags table update
//--------------------------------------------------------------------
void update_shipfrags()
{
    for (int i = 0; i < 20; i++)
    {
        int max = 0;
        shipfrags[i].ship = 0;
        ShipVisitor svs;
        for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
        {
            if (IS_NPC_SHIP(svs))
                continue;
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

void display_shipfrags(P_char ch)
{
  send_to_char("&+L10 most dangerous ships\r\n", ch);
  send_to_char("&+L-======================================================-&N\r\n\r\n", ch);
  for (int i = 0; i < 10; i++)
  {
    if (shipfrags[i].ship == NULL)
    {
      break;
    }
    int found = 0;
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
      if (svs == shipfrags[i].ship)
      {
        found = 1;
      }
    }
    if (!found)
    {
      break;
    }
    if (shipfrags[i].ship->frags == 0)
    {
      break;
    }
    if (i != 0)
    {
      if (shipfrags[i].ship == shipfrags[i - 1].ship)
      {
        break;
      }
    }
    send_to_char_f(ch,
            "&+W%d:&N %s\r\n&+LCaptain: &+W%-20s &+LClass: &+y%-15s&+R Tonnage Sunk: &+W%d&N\r\n\r\n",
            i + 1, shipfrags[i].ship->name, shipfrags[i].ship->ownername,
            SHIPTYPE_NAME(SHIP_CLASS(shipfrags[i].ship)),
            shipfrags[i].ship->frags);
  }
}
