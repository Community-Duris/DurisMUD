

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

char  arc[3];
extern char  contact[256];
extern char  weapon[100];

int   write_newship(P_ship temp);
P_ship getshipfromchar(P_char ch);
int   getarc(int heading, int bearing);
void  setcontact(int i, P_ship obj, P_ship ship, int x, int y);
int   bearing(float x1, float y1, float x2, float y2);
float range(float x1, float y1, float z1, float x2, float y2, float z2);
int   getmap(P_ship ship);
void  act_to_all_in_ship(P_ship ship, const char *msg);
void  act_to_outside(P_ship ship, const char *msg);
void  act_to_outside_ships(P_ship ship, const char *msg, P_ship target);
//void  calc_crew_adjustments(P_ship ship);
void  update_maxspeed(P_ship ship);
int   damage_hull(P_ship ship, P_ship target, int dam, int arc, int armor_pierce);
int   damage_sail(P_ship attacker, P_ship target, int dam);
void  damage_weapon(P_ship ship, P_ship target, int arc, int dam);
//void  destroy_weapon(P_ship ship, P_ship target, int slot);
P_char captain_is_aboard(P_ship ship);

static char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];


int epic_ship_damage_control(P_char ch, int dam)
{
    if(!(ch) || dam < 2)
        return dam;

    int skill = GET_CHAR_SKILL(ch, SKILL_SHIP_DAMAGE_CONTROL);
    if (skill < 1)
        return dam;

    float reduction_mod = dam * (4.0 + (float)skill / 5.0) / 100.0;
    while (reduction_mod >= 1.0 && dam > 1)
    {
        dam--;
        reduction_mod -= 1.0;
    }
    if (reduction_mod == 0.0 || dam < 2)
        return dam;

    if (number(1, (int)(1.0 / reduction_mod)) == 1)
        dam--;

    return dam;
}

int getcontacts(P_ship ship)
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
              if (range(ship->x, ship->y, ship->z, j, 100 - i, temp->z) <= 35)
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

void setcontact(int i, P_ship target, P_ship ship, int x, int y)
{
  contacts[i].bearing = bearing(ship->x, ship->y, (float) x + (target->x - 50.0), (float) y + (target->y - 50.0));

  contacts[i].range = range(ship->x, ship->y, ship->z, (float) x + (target->x - 50.0), (float) y + (target->y - 50.0), target->z);

  contacts[i].x = x;
  contacts[i].y = y;

  contacts[i].z = (int) target->z;
  contacts[i].ship = target;
  
  char your_arc[3];
  getarc(ship->heading, contacts[i].bearing);
  sprintf(your_arc, "%s", arc);
  
  getarc(target->heading, (contacts[i].bearing >= 180) ? (contacts[i].bearing - 180) : (contacts[i].bearing + 180));
  sprintf(contacts[i].arc, "%s%s", your_arc, arc);
}

int armorcondition(int maxhp, int curhp)
{
  if (curhp < (maxhp / 3))
  {
    return 0;
  }
  else if (curhp < ((maxhp * 2) / 3))
  {
    return 1;
  }
  else
  {
    return 2;
  }
}



void stun_all_in_ship(P_ship ship, int timer)
{
  int      i;
  P_char   ch;

  for (i = 0; i < MAX_SHIP_ROOM; i++)
  {
    if ((SHIPROOMNUM(ship, i) != -1) &&
        world[real_room(SHIPROOMNUM(ship, i))].people)
    {
      for (ch = world[real_room(SHIPROOMNUM(ship, i))].people; ch;
           ch = ch->next_in_room)
      {
        if (ch &&
            !IS_TRUSTED(ch))
        {
          SET_POS(ch, POS_SITTING + GET_STAT(ch));
          CharWait(ch, timer);
        }
      }
    }
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

void scantarget(P_ship target, P_char ch)
{
  int      i, j, k;

  k = getcontacts(getshipfromchar(ch));
  for (i = 0; i < k; i++)
  {
    if (target == contacts[i].ship)
    {
      if (contacts[i].range <= 20.0)
      {
        send_to_char("Your lookout scans the target ship:\r\n", ch);
        send_to_char ("&+L=======================================================&N\r\n", ch);

        sprintf(buf, "&+LID:[&+W%s&+L]&N %-20s                Class:&+y%s&N\r\n", SHIPID(target), SHIPNAME(target), SHIPCLASSNAME(target));
        send_to_char(buf, ch);

        send_to_char("Position    Armor   Internal\r\n", ch);

        sprintf(buf,
                "Forward    %s%3d&N/&+G%-3d&N  %s%2d&N/&+g%-2d&N     X:&+W%2d&N Y:&+W%2d&N Z:&+W%2d&N\r\n",
                SHIPARMORCOND(SHIPMAXFARMOR(target), SHIPFARMOR(target)),
                SHIPFARMOR(target), SHIPMAXFARMOR(target),
                SHIPINTERNALCOND(SHIPMAXFINTERNAL(target), SHIPFINTERNAL(target)),
                SHIPFINTERNAL(target), 
                SHIPMAXFINTERNAL(target),
                contacts[i].x, 
                contacts[i].y, 
                contacts[i].z);
        send_to_char(buf, ch);

        sprintf(buf,
                "Starboard  %s%3d&N/&+G%-3d&N  %s%2d&N/&+g%-2d&N     Range  :&+W%-5.1f&N\r\n",
                SHIPARMORCOND(SHIPMAXSARMOR(target), SHIPSARMOR(target)),
                SHIPSARMOR(target), SHIPMAXSARMOR(target),
                SHIPINTERNALCOND(SHIPMAXSINTERNAL(target), SHIPSINTERNAL(target)),
                SHIPSINTERNAL(target), 
                SHIPMAXSINTERNAL(target),
                contacts[i].range);
        send_to_char(buf, ch);

        sprintf(buf,
                "Rear       %s%3d&N/&+G%-3d&N  %s%2d&N/&+g%-2d&N     Heading:&+W%d&N\r\n",
                SHIPARMORCOND(SHIPMAXRARMOR(target), SHIPRARMOR(target)),
                SHIPRARMOR(target), SHIPMAXRARMOR(target),
                SHIPINTERNALCOND(SHIPMAXRINTERNAL(target), SHIPRINTERNAL(target)),
                SHIPRINTERNAL(target), 
                SHIPMAXRINTERNAL(target),
                target->heading);
        send_to_char(buf, ch);

        sprintf(buf,
                "Port       %s%3d&N/&+G%-3d&N  %s%2d&N/&+g%-2d&N     Bearing:&+W%-3d&N Arc:&+W%s&N\r\n",
                SHIPARMORCOND(SHIPMAXPARMOR(target), SHIPPARMOR(target)),
                SHIPPARMOR(target), SHIPMAXPARMOR(target),
                SHIPINTERNALCOND(SHIPMAXPINTERNAL(target), SHIPPINTERNAL(target)),
                SHIPPINTERNAL(target), 
                SHIPMAXPINTERNAL(target),
                contacts[i].bearing,
                arc_name[getarc (getshipfromchar(ch)->heading, contacts[i].bearing)]);
        send_to_char(buf, ch);

        sprintf(buf, "%s %s\r\n",
                SHIPSINKING(target) ?  "&+RSINKING&N" : 
                SHIPIMMOBILE(target) ? "&+RIMMOBILE&N" : 
                SHIPISDOCKED(target) ?  "&+yDOCKED&N" : "",
                contacts[i].range < SCAN_RANGE ? 
                  (!SHIPISDOCKED(target) ? 
                    ((target->race == EVILSHIP) ? "&+RThis ship has an Evil flag&N" : 
                      (target->race == UNDEADSHIP) ? "&+LThis ship has a tattered black flag&N" :
                        (target->race == GOODIESHIP) ? "&+WThis ship has a goodie flag&N" :
                          "This ship does not have a flag raised") : "") : "");
        send_to_char(buf, ch);

        sprintf(buf, "Weapons:\r\n=======================\r\n");
        send_to_char(buf, ch);

        for (j = 0; j < MAXSLOTS; j++)
        {
          if (target->slot[j].type == SLOT_WEAPON)
          {
            sprintf(buf, "&+W[%d]&N &+W%-20s&N &+W%-9s\r\n", j,
                    SHIPWEAPONDESTROYED(target, j) ? "&+LDestroyed&N" : target->slot[j].desc,
                    target->slot[j].get_position_str());
            send_to_char(buf, ch);
          }
        }
      }
      else
      {
        send_to_char("Out of range.\r\n", ch);
        return;
      }
    }
  }
}

int calc_salvage(P_ship target)
{
    int k = 0, max = 0, j, salvage = 0;
    for (j = 0; j < 4; j++)
    {
      k += (target->armor[j] + target->internal[j]);
      max += (target->maxarmor[j] + target->maxinternal[j]);
    }
    k = abs(k);
    max = abs(max);
    salvage = (int) (SHIPTYPECOST(SHIPCLASS(target)) * (float) ((float) k / (float) max));
    for (j = 0; j < MAXSLOTS; j++)
    {
      if (target->slot[j].type == SLOT_WEAPON)
      {
        if (SHIPWEAPONDAMAGED(target, j))
        {
          salvage += (int) (weapon_data[target->slot[j].index].cost * 0.1);
        }
        else
        {    
          salvage += (int) (weapon_data[target->slot[j].index].cost / 2);
        }
      }
    }
    if (target->m_class == 0)
    {
      salvage = 0;
    }
    else
    {
      salvage = abs((int)(salvage/get_property("ship.sinking.rewardDivider", 7.)));
    }
    return salvage;
}

int calc_bounty(P_ship target)
{
    return (int) ( target->frags * 10000 / get_property("ship.sinking.rewardDivider", 7.0) );
}

bool ship_gain_frags(P_ship ship, int frags)
{
  if (frags > 0)
  {
    ship->frags += frags;
    sprintf(buf, "Your ship has gained %d frags!\r\n", frags);
    act_to_all_in_ship(ship, buf);

      //act_to_all_in_ship(ship, "&+GYour Sail Crew has gained some experience!&N\r\n");

    ship->sailcrew.skill   += frags * ship_crew_data[ship->sailcrew.index].skill_gain;
    ship->guncrew.skill    += frags * ship_crew_data[ship->guncrew.index].skill_gain;
    ship->repaircrew.skill += frags * ship_crew_data[ship->repaircrew.index].skill_gain;
    ship->rowingcrew.skill += frags * ship_crew_data[ship->rowingcrew.index].skill_gain;

    update_crew(ship);
    update_ship_status(ship);
  }
  
  return true;
}

bool ship_loose_frags(P_ship target, int frags)
{
  if (frags > 0)
  {
    target->frags = MAX(0, target->frags - frags);

    float members_loss = 15.0 + (float)frags / 30.0;

    target->sailcrew.replace_members(members_loss);
    target->guncrew.replace_members(members_loss);
    target->repaircrew.replace_members(members_loss);

    if (target->frags < ship_crew_data[target->sailcrew.index].min_frags * 0.8)
    {
      act_to_all_in_ship(target, "&+RYour sail crew abandons you, due to your reputation!");
      setcrew(target, sail_crew_list[0], -1);
    }
    if (target->frags < ship_crew_data[target->guncrew.index].min_frags * 0.8)
    {
      act_to_all_in_ship(target, "&+RYour gun crew abandons you, due to your reputation!");
      setcrew(target, gun_crew_list[0], -1);
    }
    if (target->frags < ship_crew_data[target->repaircrew.index].min_frags * 0.8)
    {
      act_to_all_in_ship(target, "&+RYour repair crew abandons you, due to your reputation!");
      setcrew(target, repair_crew_list[0], -1);
    }
    if (target->frags < ship_crew_data[target->rowingcrew.index].min_frags * 0.8)
    {
      act_to_all_in_ship(target, "&+RYour rowing crew abandons you, due to your reputation!");
      setcrew(target, rowing_crew_list[0], -1);
    }
    update_crew(target);
  }
  return true;
}


bool ship_gain_money(P_ship ship, P_ship target, int salvage, int bounty)
{
  if (salvage > 0)
  {
    sprintf(buf,
      "You recieve %s for the salvage of %s!\r\nIt is currently in your ship's coffers.  To look, use 'look cargo', to get, use 'get money'\r\n",
      coin_stringv(salvage), target->name);
    act_to_all_in_ship(ship, buf);
  }
  if (bounty > 0)
  {
    if (target->frags > 1000)
    {
      sprintf(buf, "In addition to salvage, you recieve a bounty of %s for sinking THIS ship.\r\n", coin_stringv(bounty));
    }
    else
    {
      sprintf(buf, "In addition to salvage, you recieve a bounty of %s for sinking this ship.\r\n", coin_stringv(bounty));
    }
    act_to_all_in_ship(ship, buf);
  }
  ship->money += (salvage + bounty);

  return true;
}

bool sink_ship(P_ship ship, P_ship attacker)
{
    int  i;
    if(IS_SET(ship->flags, ANCHOR))
        REMOVE_BIT(ship->flags, ANCHOR);
    if(IS_SET(ship->flags, RAMMING))
        REMOVE_BIT(ship->flags, RAMMING);
    if (ship->shipai)
    {
      if(IS_SET(ship->shipai->flags, AIB_ENABLED))
        REMOVE_BIT(ship->shipai->flags, AIB_ENABLED);
      if(IS_SET(ship->shipai->flags, AIB_AUTOPILOT))
        REMOVE_BIT(ship->shipai->flags, AIB_AUTOPILOT);
    }
    if(!IS_SET(ship->flags, SINKING))
        SET_BIT(ship->flags, SINKING);

    if (attacker)
    {
        logit(LOG_SHIP, "%s's ship sunk by %s", ship->ownername, attacker->ownername);
        statuslog(AVATAR, "%s's ship sunk by %s", ship->ownername, attacker->ownername);
    }
    else
    {
        logit(LOG_SHIP, "%s's ship has sunk", ship->ownername);
        statuslog(AVATAR, "%s's ship has sunk", ship->ownername);
    }

    sprintf(buf, "%s &+Wstarts to sink!&N", SHIPNAME(ship));
    act_to_outside(ship, buf);
    sprintf(buf, "&+W[%s]&N:%s &+Wstarts to sink!&N", SHIPID(ship), SHIPNAME(ship));
    act_to_outside_ships(ship, buf, ship);
    act_to_all_in_ship(ship, "&=LRYOUR SHIP STARTS TO SINK!!&N");

    ship->timer[T_SINKING] = 75;
    ship->maxspeed = 0;
    ship->setspeed = 0;
    ship->speed = 0;

    if (attacker)
    {
        int frag_gain = 0;
        if (attacker->race != ship->race)
        {
            frag_gain = SHIPHULLWEIGHT(ship);
        }

        int salvage = calc_salvage(ship);
        wizlog(56, "Salvage %d", salvage);

        int bounty = 0;
        if ((ship->frags > 100) && attacker->race != ship->race)
        {
            bounty = calc_bounty(ship);
            wizlog(56, "Bounty %d", bounty);
        }

        int fleet_size_grouped = 1;
        int fleet_size_total = 1;
        int k = getcontacts(ship);
        for (i = 0; i < k; i++)
        {
            if (contacts[i].ship == attacker)
                continue;
            if (SHIPISDOCKED(contacts[i].ship))
                continue;
            if (contacts[i].ship->m_class == 0)
                continue;
            if (attacker->race == contacts[i].ship->race)
               fleet_size_total++;

            P_char ch1 = get_char2(str_dup(SHIPOWNER(contacts[i].ship)));
            P_char ch2 = get_char2(str_dup(SHIPOWNER(attacker)));
            
            if (ch1 && ch2 && ch1->group && ch1->group == ch2->group)
            {
                fleet_size_grouped++;
            }
        }

        frag_gain = frag_gain / fleet_size_total;
        salvage = salvage / fleet_size_grouped;
        bounty = bounty / fleet_size_grouped;

        for (i = 0; i < k; i++)
        {
            if (SHIPISDOCKED(contacts[i].ship))
                continue;
            if (contacts[i].ship->m_class == 0)
                continue;

            P_char ch1 = get_char2(str_dup(SHIPOWNER(contacts[i].ship)));
            P_char ch2 = get_char2(str_dup(SHIPOWNER(attacker)));
            
            if (contacts[i].ship == attacker  || ( ch1 && ch2 && ch1->group && ch1->group == ch2->group) )
            {
                ship_gain_frags(contacts[i].ship, frag_gain);
                ship_gain_money(contacts[i].ship, ship, salvage, bounty);
                write_newship(contacts[i].ship);
            }
        }
        ship_loose_frags(ship, frag_gain);
        write_newship(ship);

        if (attacker->target == ship)
            attacker->target = NULL;
    }
    return true;
}



void volley_hit_event(P_char ch, P_char victim, P_obj obj, void *data)
{
    VolleyData* vd = (VolleyData*)data;
    P_ship ship = vd->attacker;
    P_ship target = vd->target;
    int w_num = vd->weapon_index;
    int hit_chance = vd->hit_chance;

    if (SHIPISDOCKED(target))
        return;
    if ((world[SHIPLOCATION(target)].number < 110000) && !IS_SET(target->flags, AIR)) 
        return;

    // forcing target into battle state
    if (target->timer[T_BSTATION] == 0) 
        act_to_all_in_ship(target, "&=LRYour crew scrambles to their battlestions&N!\r\n");
    target->timer[T_BSTATION] = BSTATION;

    if (dice(2, 50) >= 100 - hit_chance) 
    { // we have a hit!
        // calculating your bearing relative to target
        int your_bearing = 0;
        float range = 35.0;
        int k = getcontacts(target);
        for (int j = 0; j < k; j++) 
        {
            if (ship == contacts[j].ship)
            {
                your_bearing = contacts[j].bearing;
                range = contacts[j].range;
            }
        } // yes, if target leaves sight while volley flies, it will be hit from zero bearing and range 35

        int w_index = ship->slot[w_num].index;
        float range_mod = ((float)weapon_data[w_index].max_range - range) / ((float)weapon_data[w_index].max_range - (float)weapon_data[w_index].min_range);
        if (range_mod < 0.0) range_mod = 0.0;
        for (int f_no = 0; f_no < weapon_data[w_index].fragments; f_no++)
        {
            if (weapon_data[w_index].max_damage > 0)
            {
                int damage = 0;
                if (IS_SET(weapon_data[w_index].flags, RANGEDAM))
                {
                    damage = weapon_data[w_index].min_damage + (int)((float)(weapon_data[w_index].max_damage - weapon_data[w_index].min_damage) * range_mod);
                }
                else
                {
                    damage = number(weapon_data[w_index].min_damage, weapon_data[w_index].max_damage);
                }

                if ((SHIPSAIL(target) > 0) && (number (0, 99) < weapon_data[w_index].sail_hit))
                {// hitting sails
                    damage = (int)( (float)damage * (float)weapon_data[w_index].sail_dam / 100.0 );
                    damage_sail(ship, target, damage);
                }
                else
                {// hitting hull
                    int hit_dir = your_bearing + number(-(weapon_data[w_index].hit_arc / 2), (weapon_data[w_index].hit_arc / 2));
                    if (hit_dir >= 360)    hit_dir -= 360;
                    else if (hit_dir < 0)  hit_dir = 360 + hit_dir;
                    int hit_arc = getarc(target->heading, hit_dir);

                    damage = (int)( (float)damage * (float)weapon_data[w_index].hull_dam / 100.0 );
                    damage_hull(ship, target, damage, hit_arc, weapon_data[w_index].armor_pierce);
                }
            }
            //
            // per-fragment effects should go here:
            //
        }
        //
        // per-shot effects should go here:
        //
        if (IS_SET (weapon_data[w_index].flags, MINDBLAST)) 
        {
            act_to_all_in_ship(target, "&+MA powerful mental wave hits your ship!&N");
            act_to_all_in_ship(target, "&+MYour crew is completely disoriented by the blast!&N");
            if (range <= (weapon_data[w_index].max_range + weapon_data[w_index].min_range) / 2) 
            {
                stun_all_in_ship(target, PULSE_VIOLENCE * 2);
                act_to_all_in_ship(target, "&+MYour mind reels from the blast, you huddle in pain!&N");
            }
            sprintf(buf, "You hit &+W[%s]&N:%s&N with a powerful &+Mmental blast&N!", SHIPID(target), SHIPNAME(target));
            act_to_all_in_ship(ship, buf);
            sprintf(buf, "&+W[%s]&N:%s&N has been hit with a powerful &+Mmental blast&N!", SHIPID(target), SHIPNAME(target));
            act_to_outside_ships(target, buf, ship);
            int stuntime = (int)(5.0 + 15.0 * range_mod);
            target->timer[T_MINDBLAST] = stuntime;
        }
        update_ship_status(target, ship);
    }
    else 
    {
        sprintf(buf, "You miss &+W[%s]&N:%s&N.", SHIPID(target), SHIPNAME(target));
        act_to_all_in_ship(ship, buf);
        sprintf(buf, "&+W[%s]&N:%s&N misses your ship.", SHIPID(ship), SHIPNAME(ship));
        act_to_all_in_ship(target, buf);
        sprintf(buf, "&+W[%s]&N:%s&N misses &+W[%s]&N:%s&N.", SHIPID(ship), SHIPNAME(ship), SHIPID(target), SHIPNAME(target));
        act_to_outside_ships(target, buf, ship);
    }
    return;
}
               
/*int weapon_hit(P_ship ship, int w_num)
{
    P_ship target = ship->slot[w_num].target;
    int hit_chance = ship->slot[w_num].hit_chance;

    ship->slot[w_num].target = 0;
    ship->slot[w_num].hit_chance = 0;
    ship->slot[w_num].timer2 = 0;

    if (SHIPISDOCKED(target))
        return FALSE;
    if ((world[SHIPLOCATION(target)].number < 110000) && !IS_SET(target->flags, AIR)) 
        return FALSE;

    // forcing target into battle state
    if (target->timer[T_BSTATION] == 0) 
        act_to_all_in_ship(target, "&=LRYour crew scrambles to their battlestions&N!\r\n");
    target->timer[T_BSTATION] = BSTATION;

    if (dice(2, 50) >= 100 - hit_chance) 
    { // we have a hit!
        // calculating your bearing relative to target
        int your_bearing = 0;
        float range = 35.0;
        int k = getcontacts(target);
        for (int j = 0; j < k; j++) 
        {
            if (ship == contacts[j].ship)
            {
                your_bearing = contacts[j].bearing;
                range = contacts[j].range;
            }
        } // yes, if target leaves sight while volley flies, it will be hit from zero bearing and range 35

        int w_index = ship->slot[w_num].index;
        float range_mod = ((float)weapon_data[w_index].max_range - range) / ((float)weapon_data[w_index].max_range - (float)weapon_data[w_index].min_range);
        for (int f_no = 0; f_no < weapon_data[w_index].fragments; f_no++)
        {
            if (weapon_data[w_index].max_damage > 0)
            {
                int damage = 0;
                if (IS_SET(weapon_data[w_index].flags, RANGEDAM))
                {
                    damage = weapon_data[w_index].min_damage + (int)((float)(weapon_data[w_index].max_damage - weapon_data[w_index].min_damage) * range_mod);
                }
                else
                {
                    damage = number(weapon_data[w_index].min_damage, weapon_data[w_index].max_damage);
                }

                if ((SHIPSAIL(target) > 0) && (number (0, 99) < weapon_data[w_index].sail_hit))
                {// hitting sails
                    damage = (int)( (float)damage * (float)weapon_data[w_index].sail_dam / 100.0 );
                    damage_sail(ship, target, damage);
                }
                else
                {// hitting hull
                    int hit_dir = your_bearing + number(-(weapon_data[w_index].hit_arc / 2), (weapon_data[w_index].hit_arc / 2));
                    if (hit_dir >= 360)    hit_dir -= 360;
                    else if (hit_dir < 0)  hit_dir = 360 + hit_dir;
                    int hit_arc = getarc(target->heading, hit_dir);

                    damage = (int)( (float)damage * (float)weapon_data[w_index].hull_dam / 100.0 );
                    damage_hull(ship, target, damage, hit_arc, weapon_data[w_index].armor_pierce);
                }
            }
            //
            // per-fragment effects should go here:
            //
        }
        //
        // per-shot effects should go here:
        //
        if (IS_SET (weapon_data[w_index].flags, MINDBLAST)) 
        {
            act_to_all_in_ship(target, "&+MA powerful mental wave hits your ship!&N");
            act_to_all_in_ship(target, "&+MYour crew is completely disoriented by the blast!&N");
            if (range <= (weapon_data[w_index].max_range + weapon_data[w_index].min_range) / 2) 
            {
                stun_all_in_ship(target, PULSE_VIOLENCE * 2);
                act_to_all_in_ship(target, "&+MYour mind reels from the blast, you huddle in pain!&N");
            }
            sprintf(buf, "You hit &+W[%s]&N:%s&N with a powerful &+Mmental blast&N!", SHIPID(target), SHIPNAME(target));
            act_to_all_in_ship(ship, buf);
            sprintf(buf, "&+W[%s]&N:%s&N hits &+W[%s]&N:%s&N with a powerful &+Mmental blast&N!", SHIPID(ship), SHIPNAME(ship), SHIPID(target), SHIPNAME(target));
            act_to_outside_ships(target, buf, ship);
            float stuntime = 5.0 + 15.0 * range_mod;
            target->timer[T_MINDBLAST] = (int)stuntime;
        }
        update_ship_status(target, ship);
        return TRUE;
    }
    else 
    {
        sprintf(buf, "You miss &+W[%s]&N:%s&N.", SHIPID(target), SHIPNAME(target));
        act_to_all_in_ship(ship, buf);
        sprintf(buf, "&+W[%s]&N:%s&N misses your ship.", SHIPID(ship), SHIPNAME(ship));
        act_to_all_in_ship(target, buf);
        sprintf(buf, "&+W[%s]&N:%s&N misses &+W[%s]&N:%s&N.", SHIPID(ship), SHIPNAME(ship), SHIPID(target), SHIPNAME(target));
        act_to_outside_ships(target, buf, ship);
    }
    return FALSE;
}*/

int damage_sail(P_ship attacker, P_ship target, int dam)
{
    P_char captain = captain_is_aboard(target);
    
    // debug("Sail damage is: %d.", dam);
    
    if (captain)
    {
        dam = epic_ship_damage_control(captain, dam);
    }

    // debug("Sail damage after epic skill is: %d.", dam);
    
    if(target->m_class >= MAXSHIPCLASSMERCHANT)
    {
        dam = (int) (dam * get_property("warship.sails.damage.reduction", 0.85));
    }
    
    if (dam < 1)
    {
        dam = 1;
    }
    
    if (attacker)
    {
        sprintf(buf, "You hit &+W[%s]&N:%s&N for &+G%d&N %s of damage on the &+Wsails&N!", SHIPID(target), SHIPNAME(target), dam, (dam == 1) ? "point" : "points");
        act_to_all_in_ship(attacker, buf);
    }
    sprintf(buf, "&+WYour ship&N has been hit for &+R%d&N %s of damage on the &+Wsails&N!", dam, (dam == 1) ? "point" : "points");
    act_to_all_in_ship(target, buf);
    sprintf(buf, "&+W[%s]&N:%s&N has been hit for %d %s of damage on the &+Wsails&N!", SHIPID(target), SHIPNAME(target), dam, (dam == 1) ? "point" : "points" );
    act_to_outside_ships(target, buf, attacker);

    target->mainsail -= dam;
    return TRUE;
}

int damage_hull(P_ship attacker, P_ship target, int dam, int arc, int armor_pierce)
{
    P_char captain = captain_is_aboard(target);
    
    if (captain)
    {
        dam = epic_ship_damage_control(captain, dam);
    }
    
    if (dam < 1)
    {
        dam = 1;
    }
    
    if (attacker)
    {
        sprintf(buf, "You hit &+W[%s]&N:%s&N for &+G%d&N %s of damage on the %s side!", SHIPID(target), SHIPNAME(target), dam, (dam == 1) ? "point" : "points", arc_name[arc]);
        act_to_all_in_ship(attacker, buf);
    }
    sprintf(buf, "&+WYour ship&N has been hit for &+R%d&N %s of damage on the %s side!", dam, (dam == 1) ? "point" : "points", arc_name[arc]);
    act_to_all_in_ship(target, buf);
    sprintf(buf, "&+W[%s]&N:%s&N has been hit for %d %s of damage on the %s side!", SHIPID(target), SHIPNAME(target), dam, (dam == 1) ? "point" : "points", arc_name[arc]);
    act_to_outside_ships(target, buf, attacker);

    
    int weapon_hit_chance = 15;

    if (SHIPARMOR(target, arc) != 0)
    {
        SHIPARMOR(target, arc) -= dam;
        if (SHIPARMOR(target, arc) < 0)
        {
            dam = -SHIPARMOR(target, arc);
            SHIPARMOR(target, arc) = 0;
        }
        else
        {
            if (number(0, 99) < armor_pierce)
            { // critical hit, hitting internal
                act_to_all_in_ship(target, " &+WCRITICAL HIT!&N");
                if (attacker)
                    act_to_all_in_ship(attacker, " &+WCRITICAL HIT!&N");
                dam = dam / 2;
                weapon_hit_chance = 50;
            }
            else
            {
                return TRUE;
            }
        }
    }

    if (SHIPINTERNAL(target, arc) < 1)
    {// chance to bypass a rubble and hit random another arc from inside
        int other_arc = number (1, 9);
        if (other_arc < 4)
        {
            other_arc = (arc + other_arc) % 4;
            if (SHIPINTERNAL(target, other_arc) > 0)
                arc = other_arc;
        }
    }
    if (SHIPINTERNAL(target, arc) < 1)
        weapon_hit_chance = 100;

    SHIPINTERNAL(target, arc) -= dam;

    if (number(0, 99) < weapon_hit_chance)
    {
        damage_weapon(attacker, target, arc, dam * 5);
    }
    if (number(1, 9) == 9)
    {
        act_to_all_in_ship(target, " &+YThe blast knocks you off your feet!&N");
        stun_all_in_ship(target, PULSE_VIOLENCE * 2);
    }

    return TRUE;
}

void damage_weapon(P_ship attacker, P_ship target, int arc, int dam)
{
    int arc_weapons[MAXSLOTS];
    int j, i = 0;
    for (j = 0; j < MAXSLOTS; j++)
    {
        if ((target->slot[j].type == SLOT_WEAPON) && (target->slot[j].position == arc) && !SHIPWEAPONDESTROYED(target, j))
            arc_weapons[i++] = j;
    }
    if (i > 0)
    {
        int w_index = arc_weapons[number(0, i - 1)];
        target->slot[w_index].val2 += dam;
        if (SHIPWEAPONDESTROYED(target, w_index))
        {
            sprintf(buf, " &+RYour &+W%s &+Rhas been destroyed!&N", target->slot[w_index].get_description());
            act_to_all_in_ship(target, buf);
            if (attacker)
            {
                sprintf(buf, " &+GYou destroy a &+W%s&+G!&N", target->slot[w_index].get_description());
                act_to_all_in_ship(attacker, buf);
            }
        }
        else
        {
            sprintf(buf, " &+WYour %s has been damaged!&N", target->slot[w_index].get_description());
            act_to_all_in_ship(target, buf);
            if (attacker)
            {
                sprintf(buf, " &+GYou damage a &+W%s&+G!&N", target->slot[w_index].get_description());
                act_to_all_in_ship(attacker, buf);
            }
        }
    }
}

void force_anchor(P_ship ship)
{
    act_to_all_in_ship(ship, "&=LRYour ship functions have ceased due to crew fatigue!&N\r\n");
    sprintf(buf2, "&+W[%s]&N: %s&N suddenly slows rapidly!\r\n", ship->id, ship->name);
    act_to_outside_ships(ship, buf2, ship);
    SET_BIT(ship->flags, ANCHOR);
    ship->setspeed = 0;
}

void update_ship_status(P_ship ship, P_ship attacker)
{
    int breach_count = 0;
    for (int arc = 0; arc < 4; arc++)
    {
        if (SHIPARMOR(ship, arc) < 1)
            SHIPARMOR(ship, arc) = 0;
        if (SHIPINTERNAL(ship, arc) < 1)
            SHIPINTERNAL(ship, arc) = 0;
        if (SHIPARMOR(ship, arc) == 0 && SHIPINTERNAL(ship, arc) == 0)
            breach_count++;
    }
    if (ship->mainsail < 1)
        ship->mainsail = 0;

    if (SHIPSINKING(ship))
        return;

    if (!SHIPISDOCKED(ship))
    {
        if (breach_count > 1)
        {
            sink_ship(ship, attacker);
            return;
        }

        bool immobilized = false;
        if (breach_count == 1 && !SHIPIMMOBILE(ship))
        { // becomes immobile
            immobilized = true;
            act_to_all_in_ship(ship, "&+RYOUR SHIP IS TOO DAMAGED TO MOVE!&N");
        }
        if (ship->mainsail == 0 && !SHIPIMMOBILE(ship))
        { // becomes immobile
            immobilized = true;
            act_to_all_in_ship(ship, "&+RYOUR SAILS ARE TOO DAMAGED TO MOVE!&N");
        }
        if (immobilized)
        {
            ship->maxspeed = 0;
            ship->setspeed = 0;
            ship->speed = 0;
            sprintf(buf2, "%s is immobilized!", SHIPNAME(ship));
            act_to_outside(ship, buf2);
            sprintf(buf2, "&+W[%s]&N:%s &+Ris immobilized!", SHIPID(ship), SHIPNAME(ship));
            act_to_outside_ships(ship, buf2, ship);
        }
    }

    if (breach_count == 0 && ship->mainsail > 0)
    {
        bool currently_immobile = SHIPIMMOBILE(ship) && !SHIPISDOCKED(ship);
        update_maxspeed(ship);
        if (currently_immobile && !SHIPIMMOBILE(ship))
            act_to_all_in_ship(ship, "&+YYOUR SHIP CAN MOVE AGAIN!&N");
    }
    
    if (SHIPISDOCKED(ship) && ship->target)
    {
        ship->target = NULL;
    }   
}

void update_maxspeed(P_ship ship)
{
    int weapon_weight = ship->slot_weight(SLOT_WEAPON);
    int weapon_weight_mod = MIN(SHIPFREEWEAPON(ship), weapon_weight);
    int cargo_weight = ship->slot_weight(SLOT_CARGO) + ship->slot_weight(SLOT_CONTRABAND);
    int cargo_weight_mod = MIN(SHIPFREECARGO(ship), cargo_weight);

    float weight_mod = 1.0 - ( (float) (SHIPSLOTWEIGHT(ship) - weapon_weight_mod - cargo_weight_mod) / (float) SHIPMAXWEIGHT(ship) );

    ship->maxspeed = SHIPTYPESPEED(ship->m_class); // Set to Ship Type Max
    ship->maxspeed = (int)((float)ship->maxspeed * (1.0 + ship->sailcrew.skill_mod));
    ship->maxspeed = (int) ((float)ship->maxspeed * weight_mod);
    ship->maxspeed = BOUNDED(1, ship->maxspeed, SHIPTYPESPEED(ship->m_class));
    ship->maxspeed = (int) ((float)ship->maxspeed * (float)ship->mainsail / (float)SHIPMAXSAIL(ship)); // Adjust for sail condition
}


int check_ram_arc(int heading, int bearing, int size)
{
    size /= 2;

    if (heading >= 360 - size && bearing <= size)
        heading -= 360;
    if (bearing >= 360 - size && heading <= size)
        bearing -= 360;

    if (abs(heading - bearing) <= size)
        return TRUE;

    return FALSE;
}

int try_ram_ship(P_ship ship, P_ship target, int j)
{
    int tbearing = contacts[j].bearing, sbearing = 0;
    int theading = target->heading, sheading = ship->heading;

    if (!check_ram_arc(sheading, tbearing, 120))
    {
        act_to_all_in_ship(ship, "&+WTarget is not in front of you to ram it!&N");
        REMOVE_BIT(ship->flags, RAMMING);
        return FALSE;
    }

    if (ship->speed <= BOARDING_SPEED)
    {
        act_to_all_in_ship(ship, "&+WYour ship is too slow to ram!\r\n");
        REMOVE_BIT(ship->flags, RAMMING);
        return FALSE;
    }


    // Calculating relative speed 
    int heading_angle = abs(sheading - theading);
    if (heading_angle > 180) heading_angle = 360 - heading_angle;
    float rad = ((float) heading_angle * M_PI / 180);

    int ram_speed = ship->speed;
    int counter_speed = (-1) * (int) ((float)target->speed * cos(rad));
    if (counter_speed < 0)
    {
        ram_speed += counter_speed;
        counter_speed = 0;
        if (ram_speed <= 0)
        {
            act_to_all_in_ship(ship, "&+WTarget speeds away from your attempt to ram it!&N");
            REMOVE_BIT(ship->flags, RAMMING);
            return FALSE;
        }
    }


    // Calculating chance for success
    float speed_mod = (((float)ship->speed / (float)SHIPTYPESPEED(ship->m_class)) + ((float)target->speed / (float)SHIPTYPESPEED(target->m_class)) * 3) / 4.0;
    float hull_mod = (float)SHIPHULLWEIGHT(ship) / (float)SHIPHULLWEIGHT(target);
    if (target->speed == 0) 
        hull_mod /= 10;

    float chance = 0;
    if (hull_mod >= 1)
        chance = 75.0 / (hull_mod * speed_mod);
    else
        chance = 100.0 - (25.0 * hull_mod * speed_mod);
    chance = 100.0 - (100.0 - chance) / (1.0 + ship->sailcrew.skill_mod * 2 * (chance / 100.0));

    int hit_chance = BOUNDED(0, (int)chance, 100);


    sprintf(buf, "You attempt to ram &+W[%s]&N: %s! Chance to hit: &+W%d%%&N", SHIPID(target), SHIPNAME(target), hit_chance);
    act_to_all_in_ship(ship, buf);
    if (target->timer[T_BSTATION] == 0)
        act_to_all_in_ship(target, "&=LRYour crew scrambles madly to battlestations!&N\r\n");
    target->timer[T_BSTATION] = BSTATION;
    sprintf(buf, "&+W[%s]&N: %s attempts to ram you!", SHIPID(ship), SHIPNAME(ship));
    act_to_all_in_ship(target, buf);
    sprintf(buf, "&+W[%s]&N:%s&N attempts to ram &+W[%s]&N:%s&N!", SHIPID(ship), SHIPNAME(ship), SHIPID(target), SHIPNAME(target));
    act_to_outside_ships(ship, buf, target);

    if (number(0, 99) < hit_chance)
    { // Successful ram
        if (ram_speed < ship->speed / 2)
        {
            sprintf(buf, "&+yYour ship &+Llurches &+yas you &+rcrunch &+yinto the stern of &+W[%s]&N:%s&+y!&N", SHIPID(target), SHIPNAME(target));
            act_to_all_in_ship(ship, buf);
            sprintf(buf, "&+W[%s]&N:%s &+rcrunches &+yinto the stern of your ship!&N", SHIPID(ship), SHIPNAME(ship));
            act_to_all_in_ship(target, buf);
            sprintf(buf, "&+W[%s]&N:%s &+rcrunches &+yinto the stern of &+W[%s]&N:%s&+y!&N", SHIPID(ship), SHIPNAME(ship), SHIPID(target), SHIPNAME(target));
            act_to_outside_ships(target, buf, ship);
        }
        else if (counter_speed > ram_speed / 2)
        {
            sprintf(buf, "&+yTimbers &+rcrunch &+yand &+Ycrack &+yas you &=LRcollide &+yhead on with &+W[%s]&N:%s&+y!&N", SHIPID(target), SHIPNAME(target));
            act_to_all_in_ship(ship, buf);
            sprintf(buf, "&+yTimbers &+rcrunch &+yand &+Ycrack &+yas &+W[%s]&N:%s &=LRcollides &+yhead on with your ship!&N", SHIPID(ship), SHIPNAME(ship));
            act_to_all_in_ship(target, buf);
            sprintf(buf, "&+W[%s]&N:%s &+Rcollides &+y head on with &+W[%s]&N:%s&+y!&N", SHIPID(ship), SHIPNAME(ship), SHIPID(target), SHIPNAME(target));
            act_to_outside_ships(target, buf, ship);
        }
        else
        {
            sprintf(buf, "&+yTimbers &+rcrunch &+yand &+Ycrack &+yas you &=LRcrash &+yinto &+W[%s]&N:%s&+y!&N", SHIPID(target), SHIPNAME(target));
            act_to_all_in_ship(ship, buf);
            sprintf(buf, "&+yTimbers &+rcrunch &+yand &+Ycrack &+yas &+W[%s]&N:%s &=LRcrashes &+yinto your ship!&N", SHIPID(ship), SHIPNAME(ship));
            act_to_all_in_ship(target, buf);
            sprintf(buf, "&+W[%s]&N:%s &+Rcrashes &+yinto &+W[%s]&N:%s&+y!&N", SHIPID(ship), SHIPNAME(ship), SHIPID(target), SHIPNAME(target));
            act_to_outside_ships(target, buf, ship);
        }


        // Calculating ram point on target
        int k = getcontacts(target);
        for (j = 0; j < k; j++) 
        {
            if (contacts[j].ship == ship)
            {
                sbearing = contacts[j].bearing;
                break;
            }
        }

        // Calculating damage
        int ram_damage = (SHIPHULLWEIGHT(ship) + 100) / 10;
        ram_damage = (int)((float)ram_damage * (0.4 * (float)ship->speed / (float)SHIPTYPESPEED(ship->m_class) + 0.6 * (float)ram_speed / (float)SHIPTYPESPEED(ship->m_class)));

        int counter_damage = (SHIPHULLWEIGHT(target) + 100) / 10;
        counter_damage = (int)((float)counter_damage * (0.1 + 0.3 * (float)ship->speed / (float)SHIPTYPESPEED(ship->m_class) + 0.4 * (float)counter_speed / (float)SHIPTYPESPEED(target->m_class)));


        //sprintf(buf, "SBearing = %d, TBearing = %d, RSpeed = %d, CSpeed = %d, RDam = %d, CDam = %d\r\n", sbearing, tbearing, ram_speed, counter_speed, ram_damage, counter_damage);
        //act_to_all_in_ship(ship, buf);


        // Applying damage
        int split = (int)(100.0 * (float)ram_damage / (float)(ram_damage + counter_damage));
        while (ram_damage != 0 || counter_damage != 0)
        {
            bool hit_target = (number(0, 99) < split);
            if (ram_damage == 0) hit_target = false;
            else if (counter_damage == 0) hit_target = true;

            int dam = number(2, 6);
            if (hit_target)
            {
                if (dam > ram_damage) dam = ram_damage;
                ram_damage -= dam;
                if (SHIPHULLWEIGHT(ship) >= SHIPHULLWEIGHT(target) && number (0, 99) < 10)
                {
                    damage_sail(ship, target, dam);
                }
                else
                {
                    int hit_dir = sbearing + number(-90, 90);
                    if (hit_dir >= 360)    hit_dir -= 360;
                    else if (hit_dir < 0)  hit_dir = 360 + hit_dir;
                    damage_hull(ship, target, dam, getarc(target->heading, hit_dir), 2);
                }
            }
            else
            {
                if (dam > counter_damage) dam = counter_damage;
                counter_damage -= dam;
                if (SHIPHULLWEIGHT(target) >= SHIPHULLWEIGHT(ship) && number (0, 99) < 10)
                {
                    damage_sail(NULL, ship, dam);
                }
                else
                {
                    int hit_dir = ship->heading + number(-90, 90);
                    if (hit_dir >= 360)    hit_dir -= 360;
                    else if (hit_dir < 0)  hit_dir = 360 + hit_dir;
                    damage_hull(NULL, ship, dam, getarc(ship->heading, hit_dir), 0);
                }
            }
        }


        // Changes in ships movement
        if (SHIPHULLWEIGHT(ship) >= SHIPHULLWEIGHT(target))
        {
            target->heading = number(0, 359);
            target->setheading = number(0, 359);
            target->speed = MAX(target->speed, BOARDING_SPEED + 1);
            ship->speed  = MAX(ship->speed, BOARDING_SPEED + 1);
        }
        if (SHIPHULLWEIGHT(ship) < SHIPHULLWEIGHT(target))
        {
            ship->setspeed = 0;
        }

        act_to_all_in_ship(target, " &+YThe impact knocks you off your feet!&N");
        stun_all_in_ship(target, PULSE_VIOLENCE);
        act_to_all_in_ship(ship, " &+YThe impact knocks you off your feet!&N");
        stun_all_in_ship(ship, PULSE_VIOLENCE);
    }

    // Setting ram timers
    float whole_crew_mod = (ship->sailcrew.skill_mod + ship->guncrew.skill_mod + ship->repaircrew.skill_mod) / 3.0;
    ship->timer[T_RAM]         = MAX(1, (int)(50.0 * (1.0 - whole_crew_mod * 0.15)));
    ship->timer[T_RAM_WEAPONS] = MAX(1, (int)(25.0 * (1.0 - ship->guncrew.skill_mod * 0.15)));

    REMOVE_BIT(ship->flags, RAMMING);
    update_ship_status(target, ship);
    if (SHIPSINKING(target))
        update_ship_status(ship);
    else
        update_ship_status(ship, target);

    return TRUE;
}


int weaprange(int w_index, char range)
{
  switch (range)
  {
  case SHRTRANGE:
    return (int) ((float) (weapon_data[w_index].max_range - weapon_data[w_index].min_range) / 3 + weapon_data[w_index].min_range);
    break;
  case MEDRANGE:
    return (int) ((float) (weapon_data[w_index].max_range - weapon_data[w_index].min_range) / 3) * 2 + weapon_data[w_index].min_range;
    break;
  case LNGRANGE:
    return weapon_data[w_index].max_range;
    break;
  default:
    return 0;
    break;
  }
  return 0;
}

int weaponsight(P_ship ship, P_ship target, int slot, float range)
{
  int percent = 50;

  int max = weapon_data[ship->slot[slot].index].max_range;
  int min = weapon_data[ship->slot[slot].index].min_range;
  if ((range <= (float) max) && (range >= (float) min))
  {
    percent -= (ship->speed / 5);
    percent -= (target->speed / 3);

    float range_mod = (max - (float)range) / ((float)max - (float)min);
    percent += (int)((float)75 * range_mod);
    percent += (int)((float)50 * ship->guncrew.skill_mod);
    if (percent > 100)
      percent = 100;
    if (percent < 0)
      percent = 0;
    return percent;
  }
  return 0;
}

int getarc(int shipheading, int bearing)
{
  if (shipheading < bearing)
    shipheading += 360;
  sprintf(arc, "*");
  if ((bearing >= (shipheading - 140)) && (bearing <= (shipheading - 40)))
  {
    sprintf(arc, "P");
    return PORT;
  }
  if ((bearing >= (shipheading - 220)) && (bearing <= (shipheading - 40)))
  {
    sprintf(arc, "R");
    return REAR;
  }
  if ((bearing > (shipheading - 320)) && (bearing <= (shipheading - 40)))
  {
    sprintf(arc, "S");
    return STARBOARD;
  }
  if (((bearing >= (shipheading - 40)) && (bearing <= (shipheading + 40))) ||
      (bearing <= (shipheading - 320)) || (bearing == shipheading))
  {
    sprintf(arc, "F");
    return FORE;
  }
  return FORE;
}
