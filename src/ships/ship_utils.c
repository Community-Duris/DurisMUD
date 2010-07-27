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


extern char buf[MAX_STRING_LENGTH];


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
void act_to_all_in_ship(P_ship ship, const char *msg, ... )
{
  if (ship == NULL)
    return;

  va_list args;
  va_start(args, msg);
  vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
  va_end(args);

  for (int i = 0; i < MAX_SHIP_ROOM; i++)
  {
    if ((SHIPROOMNUM(ship, i) != -1) && world[real_room(SHIPROOMNUM(ship, i))].people)
    {
      act(local_buf, FALSE, world[real_room(SHIPROOMNUM(ship, i))].people, 0, 0, TO_ROOM);
      act(local_buf, FALSE, world[real_room(SHIPROOMNUM(ship, i))].people, 0, 0, TO_CHAR);
    }
  }
}

void act_to_outside(P_ship ship, const char *msg, ... )
{
  va_list args;
  va_start(args, msg);
  vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
  va_end(args);

  getmap(ship);
  for (int i = 0; i < 100; i++)
  {
    for (int j = 0; j < 100; j++)
    {
      if ((range(50, 50, ship->z, j, i, 0) < 35) && world[tactical_map[j][i].rroom].people)
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
  vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
  va_end(args);

  getmap(ship);
  for (int i = 0; i < 100; i++)
  {
    for (int j = 0; j < 100; j++)
    {
      if ((range(50, 50, ship->z, j, i, 0) < 35) && world[tactical_map[j][i].rroom].contents)
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
      if (IS_NPC(ch))
      {
        if(!ch->following || !IS_PC(ch->following)) // not counting mobs that arent followers
          continue;
        if (GET_VNUM(ch) == 250) // image
          continue;
      }
      num++;
    }
  }
  return (num);
}

int get_turning_speed(P_ship ship)
{
    if (SHIPIMMOBILE(ship))
    {
        return 2;
    }
    int tspeed = (int) (ship->speed / 10);
    tspeed += (int) ((280 - SHIPHULLWEIGHT(ship)) / 10);

    if (tspeed < 5)
        tspeed = 5;

    float turn_mod = 1.0 + ship->sailcrew.skill_mod;
    tspeed = (int) ((float)tspeed * turn_mod);
    return tspeed;
}

void update_maxspeed(P_ship ship)
{
    int weapon_weight = ship->slot_weight(SLOT_WEAPON);
    int weapon_weight_mod = MIN(SHIPFREEWEAPON(ship), weapon_weight);
    int cargo_weight = ship->slot_weight(SLOT_CARGO) + ship->slot_weight(SLOT_CONTRABAND);
    int cargo_weight_mod = MIN(SHIPFREECARGO(ship), cargo_weight);

    float weight_mod = 1.0 - ( (float) (SHIPSLOTWEIGHT(ship) - weapon_weight_mod - cargo_weight_mod) / (float) SHIPMAXWEIGHT(ship) );

    ship->maxspeed = SHIPTYPESPEED(ship->m_class);
    ship->maxspeed = (int)((float)ship->maxspeed * (1.0 + ship->sailcrew.skill_mod));
    ship->maxspeed = (int) ((float)ship->maxspeed * weight_mod);
    ship->maxspeed = BOUNDED(1, ship->maxspeed, SHIPTYPESPEED(ship->m_class));
    ship->maxspeed = (int) ((float)ship->maxspeed * (float)ship->mainsail / (float)SHIPMAXSAIL(ship)); // Adjust for sail condition
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

int get_arc(int heading, int bearing)
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
  
  if(!(ship))
  {
    return 0;
  }

  counter = 0;
  getmap(ship);
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
              if (!limit_range || range(ship->x, ship->y, ship->z, j, 100 - i, temp->z) <= 35)
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


int bearing(float x1, float y1, float x2, float y2)
{
    int      val;

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
    val = (int) (atan((x2 - x1) / (y2 - y1)) * 180 / 3.14);
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

  send_to_char_f(ch, "Heading: %d, Set Heading: %d\r\n", this->heading, this->setheading);
  
  send_to_char_f(ch, "Speed: %d, Set Speed: %d, Max Speed: %d, Sailcond: %d\r\n", this->speed, this->setspeed, this->maxspeed, this->mainsail);
  
  send_to_char_f(ch, "Hull weight: %d, Max load: %d, Slot weight: %d, Available weight: %d\r\n", SHIPHULLWEIGHT(this), SHIPMAXWEIGHT(this), slot_weight(-1), SHIPAVAILWEIGHT(this));
  
  send_to_char_f(ch, "Max cargo: %d, Current cargo: %d, Available cargo: %d\r\n", SHIPMAXCARGO(this), SHIPCARGO(this), SHIPAVAILCARGOLOAD(this) );
  
  send_to_char("\r\nSlots:\r\n---------------------------------------------------\r\n", ch);
  
  for( int i = 0; i < MAXSLOTS; i++ )
  {
    send_to_char_f(ch, "%-2d) ", i);
    this->slot[i].show(ch);
    send_to_char("\r\n", ch);
  }
}

void ShipSlot::show(P_char ch) const
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
    case SIDE_FORE:
      send_to_char("F  ", ch);
      break;
      
    case SIDE_PORT:
      send_to_char("P  ", ch);
      break;
      
    case SIDE_REAR:
      send_to_char("R  ", ch);
      break;
      
    case SIDE_STAR:
      send_to_char("S  ", ch);
      break;

    case HOLD:
      send_to_char("H  ", ch);
      break;
      
    default:
      send_to_char("?  ", ch);
      break;
  }

  send_to_char_f(ch, "%-3d  ", this->get_weight());
  
  send_to_char_f(ch, "%-5d %-7d %-5d %-5d %-5d  ", this->val0, this->val1, this->val2, this->val3, this->val4);
}

int ShipData::slot_weight(int type) const
{
  int weight = 0;
  
  for( int i = 0; i < MAXSLOTS; i++ )
  {
    if(this->slot[i].type != SLOT_EMPTY && (type < 0 || type == this->slot[i].type))
      weight += this->slot[i].get_weight();
  }
  
  return weight;
}

char* ShipSlot::get_status_str()
{
    if (type == SLOT_WEAPON)
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
  case SIDE_FORE:
      return "Forward";
  case SIDE_REAR:
      return "Rear";
  case SIDE_PORT:
      return "Port";
  case SIDE_STAR:
      return "Starboard";
  case HOLD:
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

int ShipSlot::get_weight() const 
{
    if (type == SLOT_WEAPON)
    {
        return weapon_data[index].weight;
    }
    else if (type == SLOT_CARGO)
    {
        return (int) (val0 * WEIGHT_CARGO);
    }
    else if (type == SLOT_CONTRABAND)
    {
        return (int) (val0 * WEIGHT_CONTRABAND);
    }
    return 0;
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

void ShipCrew::replace_members(float percent)
{
    skill = ship_crew_data[index].min_skill + (int)( (float)(skill - ship_crew_data[index].min_skill) * (100.0 - percent) / 100.0);
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

void ShipCrew::update()
{
    skill = MAX(ship_crew_data[index].min_skill, skill);
    max_stamina = (int)((float)ship_crew_data[index].base_stamina * (1.0 + sqrt((float)(skill - ship_crew_data[index].min_skill) / 1000.0) / 300.0) );
    if (stamina > max_stamina) stamina = max_stamina;
    
    float sq = sqrt((float)skill / 1000.0);
    switch (ship_crew_data[index].type)
    {
    case SAIL_CREW:
    case GUN_CREW:
        skill_mod = sq / 100.0;
        break;
    case REPAIR_CREW:
        skill_mod = sq / 100.0;
        break;
    case ROWING_CREW:
        skill_mod = sq / 100.0;
        break;
    default:
        skill_mod = 0;
    };
}


void ShipCrew::reset_stamina() 
{ 
    stamina = max_stamina; 
}

void update_crew(P_ship ship)
{
    ship->sailcrew.update();
    ship->guncrew.update();
    ship->repaircrew.update();
    ship->rowingcrew.update();
}

void reset_crew_stamina(P_ship ship)
{
    ship->sailcrew.reset_stamina();
    ship->guncrew.reset_stamina();
    ship->repaircrew.reset_stamina();
    ship->rowingcrew.reset_stamina();
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

void normalize_direction(int &dir)
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

    if (IS_SET(ship->flags, AIR))
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
