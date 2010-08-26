#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h> 

#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include "ships.h"


char buf[MAX_STRING_LENGTH];

float hull_mod[MAXSHIPCLASS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
float ShipTypeData::get_hull_mod() const
{
    if (hull_mod[_classid - 1]) return hull_mod[_classid - 1];
    hull_mod[_classid - 1] = sqrt(_hull);
    return hull_mod[_classid - 1];
}


ShipObjHash shipObjHash;

ShipObjHash::ShipObjHash()
{
    for (int i = 0; i < SHIP_OBJ_TABLE_SIZE; i++)
        table[i] = 0;
    sz = 0;
}
P_ship ShipObjHash::find(P_obj key)
{
    unsigned hash_value = (unsigned)key;
    unsigned t_index = hash_value % SHIP_OBJ_TABLE_SIZE;
    P_ship curr = table[t_index];
    while (curr != 0)
    {
        if (curr->shipobj == key)
            break;
        curr = curr->next;
    }
    return curr;
}
bool ShipObjHash::add(P_ship ship)
{
    unsigned hash_value = (unsigned)ship->shipobj;
    unsigned t_index = hash_value % SHIP_OBJ_TABLE_SIZE;
    P_ship curr = table[t_index];
    while (curr != 0)
    {
        if (curr == ship)
            return false;
        curr = curr->next;
    }
    ship->next = table[t_index];
    table[t_index] = ship;
    sz++;
    return true;
}
bool ShipObjHash::erase(P_ship ship)
{
    unsigned hash_value = (unsigned)ship->shipobj;
    return erase(ship, hash_value % SHIP_OBJ_TABLE_SIZE);
}
bool ShipObjHash::erase(P_ship ship, unsigned t_index)
{
    P_ship curr = table[t_index];
    P_ship prev = 0;
    while (curr != 0)
    {
        if (curr == ship)
        {
            if (prev != 0)
                prev->next = curr->next;
            else
                table[t_index] = curr->next;
            curr->next = 0;
            sz--;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}
bool ShipObjHash::get_first(visitor& vs)
{
    for (vs.t_index = 0; vs.t_index < SHIP_OBJ_TABLE_SIZE; ++vs.t_index)
    {
        if (table[vs.t_index] != 0)
        {
            vs.curr = table[vs.t_index];
            return true;
        }
    }
    return false;
}
bool ShipObjHash::get_next(visitor& vs)
{
    if (vs.curr->next != 0)
    {
        vs.curr = vs.curr->next;
        return true;
    }
    for (++vs.t_index; vs.t_index < SHIP_OBJ_TABLE_SIZE; ++vs.t_index)
    {
        if (table[vs.t_index] != 0)
        {
            vs.curr = table[vs.t_index];
            return true;
        }
    }
    return false;
}
bool ShipObjHash::erase(visitor& vs)
{
    unsigned t_index = vs.t_index;
    P_ship curr = vs.curr;
    bool res = get_next(vs);
    erase(curr, t_index);
    return res;
}


//--------------------------------------------------------------------
P_ship get_ship_from_owner(char *ownername)
{
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if( isname(ownername, svs->ownername) )
            return svs;
    }
    return 0;
}

//--------------------------------------------------------------------


void everyone_look_out_newship(P_ship ship)
{
  P_char   ch, ch_next;
  int      i;

  for (i = 0; i < MAX_SHIP_ROOM; i++)
  {
    for (ch = world[real_room(ship->room[i].roomnum)].people; ch;
         ch = ch_next)
    {
      if (ch)
      {
        ch_next = ch->next_in_room;
        if (IS_SET(ch->specials.act2, PLR2_SHIPMAP))
        {
          new_look(ch, 0, -5, ship->location);
        }
      }
    }
  }
}

void everyone_get_out_newship(P_ship ship)
{
  P_char   ch, ch_next;
  P_obj    obj, obj_next;
  int      i;

  for (i = 0; i < MAX_SHIP_ROOM; i++)
  {
    for (ch = world[real_room(ship->room[i].roomnum)].people; ch;
         ch = ch_next)
    {
      if (ch)
      {
        ch_next = ch->next_in_room;
        char_from_room(ch);
        char_to_room(ch, ship->location, -1);
      }
    }
    for (obj = world[real_room(ship->room[i].roomnum)].contents; obj;
         obj = obj_next)
    {
      if (obj)
      {
        obj_next = obj->next_content;
        if (obj != ship->panel)
        {
          obj_next = obj->next_content;
          obj_from_room(obj);
          obj_to_room(obj, ship->location);
        }
      }
    }
  }
}

static char local_buf[MAX_STRING_LENGTH];
void act_to_all_in_ship_f(P_ship ship, const char *msg, ... )
{
  if (ship == NULL)
    return;

  va_list args;
  va_start(args, msg);
  vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
  va_end(args);

  act_to_all_in_ship(ship, local_buf);
}

void act_to_all_in_ship(P_ship ship, const char *msg)
{
  if (ship == NULL)
    return;

  for (int i = 0; i < MAX_SHIP_ROOM; i++)
  {
    if ((SHIPROOMNUM(ship, i) != -1) && world[real_room(SHIPROOMNUM(ship, i))].people)
    {
      act(msg, FALSE, world[real_room(SHIPROOMNUM(ship, i))].people, 0, 0, TO_ROOM);
      act(msg, FALSE, world[real_room(SHIPROOMNUM(ship, i))].people, 0, 0, TO_CHAR);
    }
  }
}

void act_to_outside(P_ship ship, const char *msg, ... )
{
  va_list args;
  va_start(args, msg);
  va_end(args);
  act_to_outside(ship, 35, msg, args);
}

void act_to_outside(P_ship ship, int rng, const char *msg, ... )
{
  va_list args;
  va_start(args, msg);
  vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
  va_end(args);

  if (!getmap(ship))
    return;
  for (int i = 0; i < 100; i++)
  {
    for (int j = 0; j < 100; j++)
    {
      if ((range(50, 50, ship->z, j, i, 0) < rng) && world[tactical_map[j][i].rroom].people)
      {
        act(local_buf, FALSE, world[tactical_map[j][i].rroom].people, 0, 0, TO_ROOM);
        act(local_buf, FALSE, world[tactical_map[j][i].rroom].people, 0, 0, TO_CHAR);
      }
    }
  }
}

void act_to_outside_ships(P_ship ship, P_ship target, const char *msg, ... )
{
  va_list args;
  va_start(args, msg);
  va_end(args);
  act_to_outside_ships(ship, target, 35, msg, args);
}

void act_to_outside_ships(P_ship ship, P_ship target, int rng, const char *msg, ... )
{
  va_list args;
  va_start(args, msg);
  vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
  va_end(args);

  if (!getmap(ship))
    return;
  for (int i = 0; i < 100; i++)
  {
    for (int j = 0; j < 100; j++)
    {
      if ((range(50, 50, ship->z, j, i, 0) < rng) && world[tactical_map[j][i].rroom].contents)
      {
        for (P_obj obj = world[tactical_map[j][i].rroom].contents; obj; obj = obj->next_content)
        {
          if ((GET_ITEM_TYPE(obj) == ITEM_SHIP) && (obj->value[6] == 1) && (obj != ship->shipobj))
          {
            if (target != NULL && obj == target->shipobj)
              continue;

            act_to_all_in_ship(shipObjHash.find(obj), local_buf);
          }
        }
      }
    }
  }
}

P_ship get_ship_from_char(P_char ch)
{
  int      j;
  
  if(!(ch))
  {
    return NULL;
  }

  ShipVisitor svs;
  for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
  {
    if (!SHIPISLOADED(svs))
      continue;

    for (j = 0; j < MAX_SHIP_ROOM; j++)
    {
      if (world[ch->in_room].number == svs->room[j].roomnum)
      {
        return svs;
      }
    }
  }
  return NULL;
}

int num_people_in_ship(P_ship ship)
{
  int      i, num = 0;
  P_char   ch;

  if (!SHIPISLOADED(ship))
    return 0;

  for (i = 0; i < MAX_SHIP_ROOM; i++)
  {
    for (ch = world[real_room0(ship->room[i].roomnum)].people; ch;
         ch = ch->next_in_room)
    {
      if (IS_TRUSTED(ch))
        continue;
      if (IS_NPC(ch) && 
         ((GET_VNUM(ch) > 40200 && GET_VNUM(ch) < 40300) ||  // pirates
          (GET_VNUM(ch) == 250))) // images
      {
        continue;
      }
      num++;
    }
  }
  return (num);
}

float get_turning_speed(P_ship ship)
{
    if (SHIPIMMOBILE(ship))
        return 1;

    float tspeed = (float)SHIPHDDC(ship);
    tspeed *= 0.75 + 0.25 * (float)(ship->speed - BOARDING_SPEED) / (float)(SHIPTYPESPEED(SHIPCLASS(ship)) - BOARDING_SPEED); // only 3/4 turn at boarding speed, even less if slower
    tspeed *= (1.0 + ship->crew.sail_mod_applied);
    tspeed *= ship->crew.get_stamina_mod();
    return tspeed;
}

float get_next_heading_change(P_ship ship)
{
    if (ship->heading == ship->setheading)
        return 0;

    float hdspeed = get_turning_speed(ship);

    float diff = ship->setheading - ship->heading;
    if (diff > 180)
      diff -= 360;
    if (diff < -180)
      diff += 360;

    float change = 0;
    if (diff >= 0)
      change = MIN(diff, hdspeed);
    else
      change = MAX(diff, -hdspeed);

    return change;
}

int get_acceleration(P_ship ship)
{
    float accel = SHIPACCEL(ship);
    accel *= (1.0 + ship->crew.sail_mod_applied);
    accel *= ship->crew.get_stamina_mod();
    return (int)accel;
}
int get_next_speed_change(P_ship ship)
{
    int accel = get_acceleration(ship);
    if (ship->setspeed > ship->speed) 
    {
        if (ship->speed + accel <= ship->setspeed)
            return accel;
        else 
            return ship->setspeed - ship->speed;
    }
    if (ship->setspeed < ship->speed) 
    {
        if (ship->speed - accel >= ship->setspeed)
            return -accel;
        else 
            return ship->setspeed - ship->speed;
    }
    return 0;
}

void update_maxspeed(P_ship ship, int breach_count)
{
    if ((breach_count >= 1 && !SHIPISFLYING(ship)) || ship->mainsail == 0)
    {
        ship->maxspeed = 0;
        return;
    }

    int weapon_weight = ship->slot_weight(SLOT_WEAPON);
    int weapon_weight_mod = MIN(SHIPFREEWEAPON(ship), weapon_weight);
    int cargo_weight = ship->slot_weight(SLOT_CARGO) + ship->slot_weight(SLOT_CONTRABAND);
    int cargo_weight_mod = MIN(SHIPFREECARGO(ship), cargo_weight);

    float weight_mod = 1.0 - ( (float) (SHIPSLOTWEIGHT(ship) - weapon_weight_mod - cargo_weight_mod) / (float) SHIPMAXWEIGHT(ship) );

    int maxspeed = SHIPTYPESPEED(ship->m_class) + ship->crew.get_maxspeed_mod();
    if (breach_count == 0 && SHIPISFLYING(ship)) maxspeed *= 1.2;
    ship->maxspeed = maxspeed;
    ship->maxspeed = (int)((float)ship->maxspeed * (1.0 + ship->crew.sail_mod_applied));
    ship->maxspeed = (int) ((float)ship->maxspeed * weight_mod);
    ship->maxspeed = (int) ((float)ship->maxspeed * (float)ship->mainsail / (float)SHIPMAXSAIL(ship)); // Adjust for sail condition
    if (breach_count == 1 && SHIPISFLYING(ship)) ship->maxspeed *= 0.5;
    ship->maxspeed = BOUNDED(1, ship->maxspeed, maxspeed);
}



void assignid(P_ship ship, char *id, bool npc)
{
  if(!id)
  {
    // assigning new contact id

    bool found_id = false;
    char newid[] = "--";

    int c = 0; // to make sure its not an infinite loop

    while( !found_id )
    {
      if (npc)
        newid[0] = 'X' + number(0,2);
      else
        newid[0] = 'A' + number(0,22);
      newid[1] = 'A' + number(0,25);


      bool taken = false;
      ShipVisitor svs;
      for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
      {
        if( strcmp(newid, SHIPID(svs)) == 0 )
        {
          taken = true;
          break;
        }
      }

      if( !taken )
        found_id = true;

      if( c++ > 1000 )
      {
        wizlog(56, "error in ships code assignid(): cannot find new contact id");
        return;
      }

    }

    SHIPID(ship) = str_dup(newid);
   
  }
  else
  {
    // assigning specific id
    if( isname(id, "**"))
    {
      SHIPID(ship) = str_dup("**");
      return;
    }


  }

}

int getmap(P_ship ship)
{
  int      x, y, rroom;
  P_obj    obj;

  if (!IS_MAP_ROOM(ship->location))
      return FALSE;

  for (y = 0; y < 100; y++)
  {
    for (x = 0; x < 100; x++)
    {
      strcpy(tactical_map[x][y].map, "  ");
    }
  }
  for (y = 99; y >= 0; y--)
  {
    for (x = 0; x < 100; x++)
    {
      rroom = calculate_relative_room(ship->location, x - 50, y - 50);
      tactical_map[x][y].rroom = rroom;
      if ((world[rroom].sector_type < NUM_SECT_TYPES) && (world[rroom].sector_type > -1))
      {
        sprintf(tactical_map[x][y].map, ship_symbol[(int) world[rroom].sector_type]);
      }
      else
      {
        sprintf(tactical_map[x][y].map, ship_symbol[0]);
      }
      if (world[rroom].contents)
      {
        for (obj = world[rroom].contents; obj; obj = obj->next_content)
        {
          if ((GET_ITEM_TYPE(obj) == ITEM_SHIP) && (obj->value[6] == 1))
          {
            P_ship temp = shipObjHash.find(obj);
            if (temp == NULL)
              continue;
            sprintf(tactical_map[x][y].map, "&+W%s&N", temp->id);
          }
        }
      }
    }
  }
  sprintf(tactical_map[50][50].map, "&+W**&N");

  return TRUE;
}

int get_arc(float heading, float bearing)
{
  bearing -= heading;
  normalize_direction(bearing);
  if (bearing <= 40 || bearing >= 320)
    return SIDE_FORE;
  if (bearing >= 140 && bearing <= 220)
    return SIDE_REAR;
  if (bearing >= 40 && bearing <= 140)
    return SIDE_STAR;
  if (bearing >= 220 && bearing <= 320)
    return SIDE_PORT;
  return SIDE_FORE;
}

const char *get_arc_indicator(int arc_no)
{
  switch(arc_no)
  {
  case SIDE_FORE:
    return "F";
  case SIDE_REAR:
    return "R";
  case SIDE_STAR:
    return "S";
  case SIDE_PORT:
    return "P";
  }
  return "*";
}

void setcontact(int i, P_ship target, P_ship ship, int x, int y)
{
  contacts[i].bearing = bearing(ship->x, ship->y, (float) x + (target->x - 50.0), (float) y + (target->y - 50.0));

  contacts[i].range = range(ship->x, ship->y, ship->z, (float) x + (target->x - 50.0), (float) y + (target->y - 50.0), target->z);

  contacts[i].x = x;
  contacts[i].y = y;

  contacts[i].z = (int) target->z;
  contacts[i].ship = target;
  
  sprintf(contacts[i].arc, "%s%s", 
      get_arc_indicator(get_arc(ship->heading, contacts[i].bearing)), 
      get_arc_indicator(get_arc(target->heading, (contacts[i].bearing >= 180) ? (contacts[i].bearing - 180) : (contacts[i].bearing + 180))));
}

int getcontacts(P_ship ship, bool limit_range)
{
  int      i, j, counter;
  P_obj    obj;
  P_ship temp;
  
  if(!ship)
    return 0;

  if (!getmap(ship))
    return 0;

  counter = 0;
  for (i = 0; i < 100; i++)
  {
    for (j = 0; j < 100; j++)
    {
      if (world[tactical_map[j][i].rroom].contents)
      {
        for (obj = world[tactical_map[j][i].rroom].contents; obj;
             obj = obj->next_content)
        {
          if(!(obj))
          {
            continue;
          }

          if ((GET_ITEM_TYPE(obj) == ITEM_SHIP) && (obj->value[6] == 1))
          {
            if (obj != ship->shipobj)
            {
              temp = shipObjHash.find(obj);
              if (!limit_range || range(ship->x, ship->y, ship->z, j, 100 - i, temp->z) <= (float)(35 + ship->crew.get_contact_range_mod()))
              {
                setcontact(counter, temp, ship, j, 100 - i);
                counter++;
              }
            }
          }
        }
      }
    }
  }
  return counter;
}


float bearing(float x1, float y1, float x2, float y2)
{
    float val;

    if (y1 == y2) {
        if (x1 > x2)
            return 270;
        return 90;
    }
    if (x1 == x2) {
        if (y1 > y2)
            return 180;
        else
            return 0;
    }
    val = atan((x2 - x1) / (y2 - y1)) * 180 / M_PI;
    if (y1 < y2) {
        if (val >= 0)
            return val;
        return(val + 360);
    } else {
        return val + 180;
    }
    return 0;
}

void ShipData::show(P_char ch) const
{  
  send_to_char("Ship Information\r\n-----------------------------------\r\n", ch);
  
  send_to_char_f(ch, "Name: %s\r\n", this->name);

  send_to_char_f(ch, "Owner: %s\r\n", this->ownername);

  send_to_char_f(ch, "ID: %s\r\n", this->id);

  send_to_char_f(ch, "Heading: %d, Set Heading: %d\r\n", (int)this->heading, (int)this->setheading);
  
  send_to_char_f(ch, "Speed: %d, Set Speed: %d, Max Speed: %d, Sailcond: %d\r\n", this->speed, this->setspeed, this->maxspeed, this->mainsail);
  
  send_to_char_f(ch, "Hull weight: %d, Max load: %d, Slot weight: %d, Available weight: %d\r\n", SHIPHULLWEIGHT(this), SHIPMAXWEIGHT(this), slot_weight(-1), SHIPAVAILWEIGHT(this));
  
  send_to_char_f(ch, "Max cargo: %d, Current cargo: %d, Available cargo: %d\r\n", SHIPMAXCARGO(this), SHIPCARGO(this), SHIPAVAILCARGOLOAD(this) );
  
  send_to_char("\r\nSlots:\r\n---------------------------------------------------\r\n", ch);
  
  for( int i = 0; i < MAXSLOTS; i++ )
  {
    send_to_char_f(ch, "%-2d) ", i);
    this->slot[i].show(ch, this);
    send_to_char("\r\n", ch);
  }
}

void ShipSlot::show(P_char ch, const ShipData* ship) const
{
  switch( this->type )
  {
    case SLOT_EMPTY:
      send_to_char("Empty       ", ch);
      break;
      
    case SLOT_WEAPON:
      send_to_char("Weapon      ", ch);
      break;
      
    case SLOT_CARGO:
      send_to_char("Cargo       ", ch);
      break;
      
    case SLOT_CONTRABAND:
      send_to_char("Contraband  ", ch);
      break;
      
    default:
      send_to_char("(unknown)   ", ch);
      break;
  }
  
  switch( this->position )
  {
    case SLOT_FORE:
      send_to_char("F  ", ch);
      break;
      
    case SLOT_PORT:
      send_to_char("P  ", ch);
      break;
      
    case SLOT_REAR:
      send_to_char("R  ", ch);
      break;
      
    case SLOT_STAR:
      send_to_char("S  ", ch);
      break;

    case SLOT_EQUI:
      send_to_char("E  ", ch);
      break;

    case SLOT_HOLD:
      send_to_char("H  ", ch);
      break;
      
    default:
      send_to_char("?  ", ch);
      break;
  }

  send_to_char_f(ch, "%-3d  ", this->get_weight(ship));
  
  send_to_char_f(ch, "%-5d %-7d %-5d %-5d %-5d  ", this->val0, this->val1, this->val2, this->val3, this->val4);
}

int ShipData::slot_weight(int type) const
{
  int weight = 0;
  
  for( int i = 0; i < MAXSLOTS; i++ )
  {
    if(this->slot[i].type != SLOT_EMPTY && (type < 0 || type == this->slot[i].type))
      weight += this->slot[i].get_weight(this);
  }
  
  return weight;
}

char* ShipSlot::get_status_str()
{
    if (type == SLOT_WEAPON || type == SLOT_EQUIPMENT)
    {
        if (val2 > 100)
            sprintf(status, "&+LDestroyed");
        else if (val2 > 20)
            sprintf(status, "&+RBadly Damaged");
        else if (val2 > 0)
            sprintf(status, "&+WDamaged");
        else if (timer > 0)
            sprintf(status, "&+Y%-6d", timer);
        else if (timer == 0)
            strcpy(status, "&NReady");
    }
    else
      strcpy(status, "");

    return status;
}

const char* ShipSlot::get_position_str()
{
  switch (position) 
  {
  case SLOT_FORE:
      return "Forward";
  case SLOT_REAR:
      return "Rear";
  case SLOT_PORT:
      return "Port";
  case SLOT_STAR:
      return "Starboard";
  case SLOT_EQUI:
      return "Equipment";
  case SLOT_HOLD:
      return "Cargo Hold";
  default:
      return "ERROR";
  }
}

char* ShipSlot::get_description()
{
    if (type == SLOT_WEAPON)
    {
        sprintf(desc, "%s", weapon_data[index].name);
    }
    else if (type == SLOT_EQUIPMENT)
    {
        sprintf(desc, "%s", equipment_data[index].name);
    }
    else if (type == SLOT_CARGO)
    {
        sprintf(desc, "%s", cargo_type_name(index));
    }
    else if (type == SLOT_CONTRABAND)
    {
        sprintf(desc, "%s", contra_type_name(index));
    }
    else
    {
        desc [0] = 0;
    }
    return desc;
}


void ShipSlot::clear()
{
    strcpy(desc, "");
    strcpy(status, "");
    type = SLOT_EMPTY;
    index = -1;
    position = -1;
    timer = 0;
    val0 = -1;
    val1 = -1;
    val2 = -1;
    val3 = -1;
    val4 = -1;
}

int ShipSlot::get_weight(const ShipData* ship) const 
{
    int wt = 0;
    if (type == SLOT_WEAPON)
    {
        wt =  weapon_data[index].weight;
    }
    if (type == SLOT_EQUIPMENT)
    {
        wt = equipment_data[index].weight;
        if (index == E_RAM)
            wt = eq_ram_weight(ship);
        if (index == E_LEVISTONE)
        {
            if (SHIPISFLYING(ship))
                wt = 0;
            else
                wt = eq_levistone_weight(ship);
        }
    }
    else if (type == SLOT_CARGO)
    {
        wt = val0 * WEIGHT_CARGO;
    }
    else if (type == SLOT_CONTRABAND)
    {
        wt = val0 * WEIGHT_CONTRABAND;
    }
    return wt;
}

void ShipSlot::clone(const ShipSlot& other)
{
    type = other.type;
    index = other.index;
    position = other.position;
    timer = other.timer;
    val0 = other.val0;
    val1 = other.val1;
    val2 = other.val2;
    val3 = other.val3;
    val4 = other.val4;
}

bool ShipCrewData::hire_room(int room) const
{
    for (int i = 0; i < 5; i++)
        if (hire_rooms[i] == room)
            return true;
    return false;
}

const char* ShipCrewData::get_next_bonus(int& cur) const
{
    for (cur++; cur < 32; cur++)
    {
        ulong flag = 1 << cur;
        if (IS_SET(flags, flag))
        {
            return get_bonus_string(flag);
        }
    }
    cur = -1;
    return NULL;
}

const char* ShipCrewData::get_bonus_string(ulong flag) const
{
    if (flag == CF_SCOUT_RANGE_1)
        return "Scout Range +1";
    if (flag == CF_SCOUT_RANGE_2)
        return "Scout Range +2";
    if (flag == CF_MAXSPEED_1)
        return "Maximum Speed +1";
    if (flag == CF_MAXSPEED_2)
        return "Maximum Speed +2";
    if (flag == CF_MAXCARGO_10)
        return "Cargo Load +10%";
    if (flag == CF_HULL_REPAIR_2)
        return "Hull Repair x2";
    if (flag == CF_HULL_REPAIR_3)
        return "Hull Repair x3";
    if (flag == CF_WEAPONS_REPAIR_2)
        return "Weapons Repair x2";
    if (flag == CF_WEAPONS_REPAIR_3)
        return "Weapons Repair x3";
    if (flag == CF_SAIL_REPAIR_2)
        return "Sail Repair x2";
    if (flag == CF_SAIL_REPAIR_3)
        return "Sail Repair x3";
    
    return "Unknown";
}

bool ShipChiefData::hire_room(int room) const
{
    for (int i = 0; i < 5; i++)
        if (hire_rooms[i] == room)
            return true;
    return false;
}

const char* ShipChiefData::get_spec() const
{
    switch (type)
    {
    case SAIL_CHIEF: return "Deck";
    case GUNS_CHIEF: return "Guns";
    case RPAR_CHIEF: return "Maintenance";
    };
    return "";
}



void ShipCrew::replace_members(float percent)
{
    sail_skill = (float)ship_crew_data[index].base_sail_skill + (sail_skill - (float)ship_crew_data[index].base_sail_skill) * (100.0 - percent) / 100.0;
    guns_skill = (float)ship_crew_data[index].base_guns_skill + (guns_skill - (float)ship_crew_data[index].base_guns_skill) * (100.0 - percent) / 100.0;
    rpar_skill = (float)ship_crew_data[index].base_rpar_skill + (rpar_skill - (float)ship_crew_data[index].base_rpar_skill) * (100.0 - percent) / 100.0;
}

float ShipCrew::get_stamina_mod()
{
    if (stamina > 0) return 1.0;
    return 1.0 / (1.0 + (-stamina) / max_stamina / 3.0);
}
const char* ShipCrew::get_stamina_prefix()
{
    if (stamina > 0) return "&+G";
    if (stamina > - max_stamina) return "&+Y";
    return "&+R";
}
int ShipCrew::get_display_stamina()
{
    if (stamina > 0) return (int)stamina;
    return (int) -sqrt(-stamina);
}

void ShipCrew::sail_skill_raise(float raise)
{
    skill_raise(raise, sail_skill, sail_chief);
}
void ShipCrew::guns_skill_raise(float raise)
{
    skill_raise(raise, guns_skill, guns_chief);
}
void ShipCrew::rpar_skill_raise(float raise)
{
    skill_raise(raise, rpar_skill, rpar_chief);
}

void ShipCrew::skill_raise(float raise, float& skill, int chief)
{
    raise *= 1.0 + (float)ship_chief_data[chief].skill_gain_bonus / 100;
    if (skill < ship_chief_data[chief].min_skill)
    {
        skill += raise * 4;
        if (skill > ship_chief_data[chief].min_skill)
        {
            raise = (skill - ship_chief_data[chief].min_skill) / 4;
            skill = ship_chief_data[chief].min_skill;
        }
        else
            raise = 0;
    }
    skill += raise;
}


void ShipCrew::reduce_stamina(float val, P_ship ship)
{
    stamina -= val;
    //act_to_all_in_ship_f(ship, "stamina: -%f", val);
}

int ShipCrew::sail_mod()
{
    return ship_crew_data[index].level + ship_crew_data[index].sail_mod + ship_chief_data[sail_chief].skill_mod;
}

int ShipCrew::guns_mod()
{
    return ship_crew_data[index].level + ship_crew_data[index].guns_mod + ship_chief_data[guns_chief].skill_mod;
}

int ShipCrew::rpar_mod()
{
    return ship_crew_data[index].level + ship_crew_data[index].rpar_mod + ship_chief_data[rpar_chief].skill_mod;
}

void ShipCrew::update()
{
    sail_skill = MAX((float)ship_crew_data[index].base_sail_skill, sail_skill);
    guns_skill = MAX((float)ship_crew_data[index].base_guns_skill, guns_skill);
    rpar_skill = MAX((float)ship_crew_data[index].base_rpar_skill, rpar_skill);
    max_stamina = ship_crew_data[index].base_stamina;
    //TODO max_stamina = (int)((float)ship_crew_data[index].base_stamina * (1.0 + sqrt((float)(skill - ship_crew_data[index].min_skill) / 1000.0) / 300.0) );
    if (stamina > max_stamina) stamina = max_stamina;
    
    sail_mod_applied = sqrt((float)sail_skill) / 130.0 + (float)sail_mod() * 0.04;
    guns_mod_applied = sqrt((float)guns_skill) / 130.0 + (float)guns_mod() * 0.04;
    rpar_mod_applied = sqrt((float)rpar_skill) / 130.0 + (float)rpar_mod() * 0.04;
}

void ShipCrew::reset_stamina() 
{ 
    stamina = max_stamina; 
}

int ShipCrew::get_contact_range_mod() const
{
    if (IS_SET(ship_crew_data[index].flags, CF_SCOUT_RANGE_2))
        return 2;
    if (IS_SET(ship_crew_data[index].flags, CF_SCOUT_RANGE_1))
        return 1;
    return 0;
}
int ShipCrew::get_sail_repair_mod() const
{
    if (IS_SET(ship_crew_data[index].flags, CF_SAIL_REPAIR_3))
        return 3;
    if (IS_SET(ship_crew_data[index].flags, CF_SAIL_REPAIR_2))
        return 2;
    return 1; // TODO
}
int ShipCrew::get_weapon_repair_mod() const
{
    if (IS_SET(ship_crew_data[index].flags, CF_WEAPONS_REPAIR_3))
        return 3;
    if (IS_SET(ship_crew_data[index].flags, CF_WEAPONS_REPAIR_2))
        return 2;
    return 1; // TODO
}
int ShipCrew::get_hull_repair_mod() const
{
    if (IS_SET(ship_crew_data[index].flags, CF_HULL_REPAIR_3))
        return 3;
    if (IS_SET(ship_crew_data[index].flags, CF_HULL_REPAIR_2))
        return 2;
    return 1; // TODO
}
int ShipCrew::get_maxspeed_mod() const
{
    if (IS_SET(ship_crew_data[index].flags, CF_MAXSPEED_2))
        return 2;
    if (IS_SET(ship_crew_data[index].flags, CF_MAXSPEED_1))
        return 1;
    return 0; // TODO
}
float ShipCrew::get_maxcargo_mod() const
{
    if (IS_SET(ship_crew_data[index].flags, CF_MAXCARGO_10))
        return 1.1;
    return 1.0; // TODO
}


void update_crew(P_ship ship)
{
    ship->crew.update();
}

void reset_crew_stamina(P_ship ship)
{
    ship->crew.reset_stamina();
}

void change_crew(P_ship ship, int crew_index, bool skill_drop)
{
    if (crew_index < 0 || crew_index > MAXCREWS)
        return;

    if (ship == NULL)
        return;

    if (skill_drop)
    {
        ship->crew.sail_skill -= ship->crew.sail_skill * ((float)number (20, 100) / 2000);
        ship->crew.guns_skill -= ship->crew.guns_skill * ((float)number (20, 100) / 2000);
        ship->crew.rpar_skill -= ship->crew.rpar_skill * ((float)number (20, 100) / 2000);
    }

    ship->crew.index = crew_index;
    ship->crew.sail_skill = MAX((float)ship_crew_data[crew_index].base_sail_skill, ship->crew.sail_skill);
    ship->crew.guns_skill = MAX((float)ship_crew_data[crew_index].base_guns_skill, ship->crew.guns_skill);
    ship->crew.rpar_skill = MAX((float)ship_crew_data[crew_index].base_rpar_skill, ship->crew.rpar_skill);
    ship->crew.update();
    ship->crew.reset_stamina();
}

void set_crew(P_ship ship, int crew_index, bool reset_skills)
{
    if (crew_index < 0 || crew_index > MAXCREWS)
        return;

    if (ship == NULL)
        return;

    ship->crew.index = crew_index;
    if (reset_skills)
    {
        ship->crew.sail_skill = ship_crew_data[crew_index].base_sail_skill;
        ship->crew.guns_skill = ship_crew_data[crew_index].base_guns_skill;
        ship->crew.rpar_skill = ship_crew_data[crew_index].base_rpar_skill;
    }
    ship->crew.update();
    ship->crew.reset_stamina();
}

void set_chief(P_ship ship, int chief_index)
{
    if (ship_chief_data[chief_index].type == SAIL_CHIEF)
        ship->crew.sail_chief = chief_index;
    if (ship_chief_data[chief_index].type == GUNS_CHIEF)
        ship->crew.guns_chief = chief_index;
    if (ship_chief_data[chief_index].type == RPAR_CHIEF)
        ship->crew.rpar_chief = chief_index;
    if (ship_chief_data[chief_index].type == NO_CHIEF)
    {
        ship->crew.sail_chief = chief_index;
        ship->crew.guns_chief = chief_index;
        ship->crew.rpar_chief = chief_index;
    }
}





P_char captain_is_aboard(P_ship ship)
{
    P_char ch, ch_next;
    int i;

    if (!(ship))
    {
        return NULL;
    }
  
    for (i = 0; i < MAX_SHIP_ROOM; i++)
    {
        for (ch = world[real_room(ship->room[i].roomnum)].people; ch;
             ch = ch_next)
        {
            if (ch)
            {
                ch_next = ch->next_in_room;
                
                if (IS_NPC(ch))
                {
                    continue;
                }
                
                if (IS_PC(ch) && isname(GET_NAME(ch), SHIPOWNER(ship)))
                {
                    return ch;
                }
            }
        }
    }

    return NULL;
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

void set_weapon(P_ship ship, int slot, int w_num, int arc)
{
    ship->slot[slot].type = SLOT_WEAPON;
    ship->slot[slot].index = w_num;
    ship->slot[slot].position = arc;
    ship->slot[slot].timer = 0;
    ship->slot[slot].val0 = w_num; // ammo type
    ship->slot[slot].val1 = weapon_data[w_num].ammo; // ammo count
    ship->slot[slot].val2 = 0; // damage level
}

void set_equipment(P_ship ship, int slot, int e_num)
{
    ship->slot[slot].type = SLOT_EQUIPMENT;
    ship->slot[slot].index = e_num;
    ship->slot[slot].position = SLOT_EQUI;
    ship->slot[slot].timer = 0;
    ship->slot[slot].val0 = 0;
    ship->slot[slot].val1 = 0;
    ship->slot[slot].val2 = 0;
}

float WeaponData::average_hull_damage() const
{
    return 
    ((float)(min_damage + max_damage) / 2.0) * 
    ((float)fragments) * 
    ((float)hull_dam / 100.0) * 
    ((100.0 - (float)sail_hit) / 100.0);
}

float WeaponData::average_sail_damage() const
{
    return 
    ((float)(min_damage + max_damage) / 2.0) * 
    ((float)fragments) * 
    ((float)sail_dam / 100.0) * 
    ((float)sail_hit / 100.0);
}

void normalize_direction(float &dir)
{
    while (dir >= 360) dir = dir - 360;
    while (dir < 0) dir = dir + 360;
}

const char* get_arc_name(int arc)
{
    switch (arc)
    {
    case SIDE_FORE: return "forward";
    case SIDE_PORT: return "port";
    case SIDE_REAR: return "rear";
    case SIDE_STAR: return "starboard";
    }
    return "";
}

const char* condition_prefix(int maxhp, int curhp, bool light)
{
  if (curhp < (maxhp / 3))
  {
    return light ? "&+R" : "&+r";
  }
  else if (curhp < ((maxhp * 2) / 3))
  {
    return light ? "&+Y" : "&+y";
  }
  else
  {
    return light ? "&+G" : "&+g";
  }
}

float range(float x1, float y1, float z1, float x2, float y2, float z2)
{
  float    dx, dy, dz, range;

  dx = x2 - x1;
  dy = y2 - y1;
  dz = z2 - z1;

  range = sqrt((dx * dx) + (dy * dy) + (dz * dz));
  return range;
}

bool is_valid_sailing_location(P_ship ship, int room)
{
    if (world[room].number < 110000)
        return false;

    if (SHIPISFLYING(ship))
    {
        if (!IS_MAP_ROOM(room) || world[room].sector_type == SECT_MOUNTAIN)
        {
            return false;
        }
    }
    else
    {
        if (!IS_MAP_ROOM(room) || world[room].sector_type != SECT_OCEAN)
        {
            return false;
        }
    }
    return true;
}

bool has_eq_ram(const ShipData* ship)
{
    return eq_ram_slot(ship) != -1;
}
int eq_ram_slot(const ShipData* ship)
{
    for (int slot = 0; slot < MAXSLOTS; slot++) 
        if (ship->slot[slot].type == SLOT_EQUIPMENT && ship->slot[slot].index == E_RAM)
            return slot;
    return -1;
}
int eq_ram_damage(const ShipData* ship)
{
    return eq_ram_weight(ship);
}
int eq_ram_weight(const ShipData* ship)
{
    return (SHIPHULLWEIGHT(ship) + 10) / 24;
}
int eq_ram_cost(const ShipData* ship)
{
    return SHIPHULLWEIGHT(ship) * 1000;
}

bool has_eq_levistone(const ShipData* ship)
{
    return eq_levistone_slot(ship) != -1;
}
int eq_levistone_slot(const ShipData* ship)
{
    for (int slot = 0; slot < MAXSLOTS; slot++) 
        if (ship->slot[slot].type == SLOT_EQUIPMENT && ship->slot[slot].index == E_LEVISTONE)
            return slot;
    return -1;
}
int eq_levistone_weight(const ShipData *ship)
{
    return (SHIPHULLWEIGHT(ship) + 50) / 40;
}
