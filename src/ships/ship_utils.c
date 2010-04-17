
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
#include <math.h>


extern const char *ship_symbol[35];

extern void    dock_ship(P_ship ship, int to_room);
extern float   range(float x1, float y1, float z1, float x2, float y2, float z2);
extern int     write_newship(P_ship temp);

void  summon_ship_event(P_char ch, P_char victim, P_obj obj, void *data);
int   getmap(P_ship ship);
void  everyone_look_out_newship(P_ship ship);
void  everyone_get_out_newship(P_ship ship);
void  act_to_all_in_ship(P_ship ship, const char *msg);
void  act_to_outside(P_ship ship, const char *msg);
void  act_to_outside_ships(P_ship ship, const char *msg, P_ship target);

P_ship   getshipfromchar(P_char ch);
int      num_people_in_ship(P_ship ship);
void     assignid(P_ship ship, char *id);
int      pilotroll(P_ship ship);
void     dispshipfrags(P_char ch);


//--------------------------------------------------------------------
ShipObjHash shipObjHash;

ShipObjHash::ShipObjHash()
{
    for (int i = 0; i < SHIP_OBJ_TABLE_SIZE; i++)
        table[i] = 0;
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
P_ship get_ship(char *ownername)
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
bool rename_ship_owner(char *old_name, char *new_name)
{
   char tmp_buf[MAX_STRING_LENGTH];
   P_ship ship;
   
   ship = get_ship(old_name);
   if( !ship )
      return TRUE;

   str_free(ship->ownername);
   ship->ownername = str_dup(new_name);
   nameship(SHIPNAME(ship), ship);
   write_newship(ship);
   write_newship(NULL); // reset index file

   sprintf(tmp_buf, "Ships/%s", old_name);
   unlink(tmp_buf);

   return TRUE;
}

/* ------------------------------------------------------------------------------ */
/* pure ship rename function, to be called from rename hooks or command functions */
/* ------------------------------------------------------------------------------ */
bool rename_ship(P_char ch, char *owner_name, char *new_name)
{
   char buf[MAX_STRING_LENGTH];
   P_ship temp;
   
   temp = get_ship(owner_name);
   if( !temp )
   {
      if( isname(GET_NAME(ch), owner_name) )
      {
         send_to_char("You do not own a ship yet, buy one first!\n", ch);
      }
      else
      {
         sprintf(buf, "%s does not own a ship!\n", owner_name);
         send_to_char(buf, ch);
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

   nameship(new_name, temp);
   write_newship(temp);

   return TRUE;
}

void summon_ship_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      i;
  char     tmp_buf[MAX_STRING_LENGTH];
  int      to_room;
  int      is_trusted;

  if (sscanf((const char *) data, "%s %d %d", tmp_buf, &to_room, &is_trusted) == 3)
  {
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
      P_ship temp = svs;
      if (isname(tmp_buf, temp->ownername) && temp->timer[T_BSTATION] == 0 && !SHIPSINKING(temp))
      {
        if( !is_trusted )
        {
          for (i = 0; i < MAXSLOTS; i++)
          {
            if (temp->slot[i].type == SLOT_CARGO || temp->slot[i].type == SLOT_CONTRABAND)
            {
              temp->slot[i].type = SLOT_EMPTY;
            }
          }
        }
        
        everyone_get_out_newship(temp);
        sprintf(tmp_buf, "&+y%s is called away elsewhere.&N\r\n", temp->name);
        send_to_room(tmp_buf, temp->location);
        temp->location = to_room;
        obj_from_room(temp->shipobj);
        obj_to_room(temp->shipobj, to_room);
        sprintf(tmp_buf, "&+y%s arrives at port.\r\n&N", temp->name);
        send_to_room(tmp_buf, to_room);
        dock_ship(temp, to_room);
        check_contraband(temp, SHIPLOCATION(temp));
        REMOVE_BIT(temp->flags, SUMMONED);
        temp->speed = 0;
        temp->setspeed = 0;
        write_newship(temp);
        return;
      }
    }
  }
}

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



void act_to_all_in_ship(P_ship ship, const char *msg)
{
  if (ship == NULL)
  {
    return;
  }
  for (int i = 0; i < MAX_SHIP_ROOM; i++)
  {
    if ((SHIPROOMNUM(ship, i) != -1) &&
        world[real_room(SHIPROOMNUM(ship, i))].people)
    {
      act(msg, FALSE, world[real_room(SHIPROOMNUM(ship, i))].people, 0, 0, TO_ROOM);
      act(msg, FALSE, world[real_room(SHIPROOMNUM(ship, i))].people, 0, 0, TO_CHAR);
    }
  }
}

void act_to_outside(P_ship ship, const char *msg)
{
  int      i, j;

  getmap(ship);
  for (i = 0; i < 100; i++)
  {
    for (j = 0; j < 100; j++)
    {
      if ((range(50, 50, ship->z, j, i, 0) < 35) &&
          world[tactical_map[j][i].rroom].people)
      {
        act(msg, FALSE, world[tactical_map[j][i].rroom].people, 0, 0, TO_ROOM);
        act(msg, FALSE, world[tactical_map[j][i].rroom].people, 0, 0, TO_CHAR);
      }
    }
  }
}

void act_to_outside_ships(P_ship ship, const char *msg, P_ship target)
{
  int      i, j;
  P_obj    obj;

  getmap(ship);
  for (i = 0; i < 100; i++)
  {
    for (j = 0; j < 100; j++)
    {
      if ((range(50, 50, ship->z, j, i, 0) < 35) && world[tactical_map[j][i].rroom].contents)
      {
        for (obj = world[tactical_map[j][i].rroom].contents; obj; obj = obj->next_content)
        {
          if ((GET_ITEM_TYPE(obj) == ITEM_SHIP) && (obj->value[6] == 1) && (obj != ship->shipobj))
          {
            if (target != NULL && obj == target->shipobj)
              continue;

            act_to_all_in_ship(shipObjHash.find(obj), msg);
          }
        }
      }
    }
  }
}

P_ship getshipfromchar(P_char ch)
{
  int      j;
  
  if(!(ch))
  {
    return NULL;
  }

  ShipVisitor svs;
  for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
  {
    P_ship temp = svs;
    if (!IS_SET(temp->flags, LOADED))
      continue;

    for (j = 0; j < MAX_SHIP_ROOM; j++)
    {
      if (world[ch->in_room].number == temp->room[j].roomnum)
      {
        return temp;
      }
    }
  }
  return NULL;
}

int num_people_in_ship(P_ship ship)
{
  int      i, num = 0;
  P_char   ch;

  if (!IS_SET(ship->flags, LOADED))
    return 0;

  for (i = 0; i < MAX_SHIP_ROOM; i++)
  {
    for (ch = world[real_room0(ship->room[i].roomnum)].people; ch;
         ch = ch->next_in_room)
    {
      if (!IS_TRUSTED(ch))
      {
        num++;
      }
    }
  }
  return (num);
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

void dispshipfrags(P_char ch)
{
  int      i, found;
  char     tmp_buf[MAX_STRING_LENGTH];

  send_to_char("&+L10 most dangerous ships\r\n", ch);
  send_to_char
    ("&+L-======================================================-&N\r\n\r\n",
     ch);
  for (i = 0; i < 10; i++)
  {
    if (shipfrags[i].ship == NULL)
    {
      break;
    }
    found = 0;
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
    sprintf(tmp_buf,
            "&+W%d:&N %s\r\n&+LCaptain: &+W%-20s &+LClass: &+y%-15s&+R Tonnage Sunk: &+W%d&N\r\n\r\n",
            i + 1, shipfrags[i].ship->name, shipfrags[i].ship->ownername,
            SHIPTYPENAME(SHIPCLASS(shipfrags[i].ship)),
            shipfrags[i].ship->frags);
    send_to_char(tmp_buf, ch);
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
  char buf[MAX_STRING_LENGTH];

  send_to_char("Ship Information\r\n-----------------------------------\r\n", ch);
  
  sprintf(buf, "Name: %s\r\n", this->name);
  send_to_char(buf, ch);

  sprintf(buf, "Owner: %s\r\n", this->ownername);
  send_to_char(buf, ch);

  sprintf(buf, "ID: %s\r\n", this->id);
  send_to_char(buf, ch);

  sprintf(buf, "Heading: %d, Set Heading: %d\r\n", this->heading, this->setheading);
  send_to_char(buf, ch);
  
  sprintf(buf, "Speed: %d, Set Speed: %d, Max Speed: %d, Sailcond: %d\r\n", this->speed, this->setspeed, this->maxspeed, this->mainsail);
  send_to_char(buf, ch);
  
  sprintf(buf, "Hull weight: %d, Max load: %d, Slot weight: %d, Available weight: %d\r\n", SHIPHULLWEIGHT(this), SHIPMAXWEIGHT(this), slot_weight(-1), SHIPAVAILWEIGHT(this));
  send_to_char(buf, ch);
  
  sprintf(buf, "Max cargo: %d, Current cargo: %d, Available cargo: %d\r\n", SHIPMAXCARGO(this), SHIPCARGO(this), SHIPAVAILCARGOLOAD(this) );
  send_to_char(buf, ch);
  
  send_to_char("\r\nSlots:\r\n---------------------------------------------------\r\n", ch);
  
  for( int i = 0; i < MAXSLOTS; i++ )
  {
    sprintf(buf, "%-2d) ", i);
    send_to_char(buf, ch);
    this->slot[i].show(ch);
    send_to_char("\r\n", ch);
  }
}

void ShipSlot::show(P_char ch) const
{
  char buf[MAX_STRING_LENGTH];

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
    case FORE:
      send_to_char("F  ", ch);
      break;
      
    case PORT:
      send_to_char("P  ", ch);
      break;
      
    case REAR:
      send_to_char("R  ", ch);
      break;
      
    case STARBOARD:
      send_to_char("S  ", ch);
      break;
      
    default:
      send_to_char("?  ", ch);
      break;
  }

  sprintf(buf, "%-3d  ", this->get_weight());
  send_to_char(buf, ch);
  
  sprintf(buf, "%-5d %-7d %-5d %-5d %-5d  ", this->val0, this->val1, this->val2, this->val3, this->val4);
  send_to_char(buf, ch);
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
  case FORE:
      return "Forward";
  case REAR:
      return "Rear";
  case PORT:
      return "Port";
  case STARBOARD:
      return "Starboard";
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
        skill_mod = sq / 100.0 * 1.5;
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