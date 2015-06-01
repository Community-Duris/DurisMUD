#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
    
#include "ships.h"
#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "ship_auto.h"
#include "utils.h"
#include "events.h"
#include "epic.h"
#include "ship_npc.h"
#include "ship_npc_ai.h"

//char  arc[3];
extern char buf[MAX_STRING_LENGTH];

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

void stun_all_in_ship(P_ship ship, int timer)
{
  int      i;
  P_char   ch;

  for (i = 0; i < ship->room_count; i++)
  {
    if ((SHIP_ROOM_NUM(ship, i) != -1) &&
        world[real_room(SHIP_ROOM_NUM(ship, i))].people)
    {
      for (ch = world[real_room(SHIP_ROOM_NUM(ship, i))].people; ch;
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


float range(float x1, float y1, float z1, float x2, float y2, float z2);
void scan_target(P_ship ship, P_ship target, P_char ch)
{
  int      i, j, k;

  k = getcontacts(ship);
  for (i = 0; i < k; i++)
  {
    if (target == contacts[i].ship)
    {
      if (contacts[i].range <= 20.0 + (float)ship->crew.get_contact_range_mod())
      {
        send_to_char("Your lookout scans the target ship:\r\n", ch);
        send_to_char ("&+L=======================================================&N\r\n", ch);

        send_to_char_f(ch, "&+LID:[&+W%s&+L]&N %-20s                Class:&+y%s&N\r\n", SHIP_ID(target), SHIP_NAME(target), SHIP_CLASS_NAME(target));

        send_to_char("Position    Armor   Internal\r\n", ch);

        send_to_char_f(ch,
                "Forward    %s%3d&N/&+G%-3d&N  %s%2d&N/&+g%-2d&N     X:&+W%2d&N Y:&+W%2d&N Z:&+W%2d&N\r\n",
                SHIP_ARMOR_COND(SHIP_MAX_FARMOR(target), SHIP_FARMOR(target)),
                SHIP_FARMOR(target), SHIP_MAX_FARMOR(target),
                SHIP_INTERNAL_COND(SHIP_MAX_FINTERNAL(target), SHIP_FINTERNAL(target)),
                SHIP_FINTERNAL(target), 
                SHIP_MAX_FINTERNAL(target),
                contacts[i].x, 
                contacts[i].y, 
                contacts[i].z);

        send_to_char_f(ch,
                "Starboard  %s%3d&N/&+G%-3d&N  %s%2d&N/&+g%-2d&N     Range  :&+W%-5.1f&N\r\n",
                SHIP_ARMOR_COND(SHIP_MAX_SARMOR(target), SHIP_SARMOR(target)),
                SHIP_SARMOR(target), SHIP_MAX_SARMOR(target),
                SHIP_INTERNAL_COND(SHIP_MAX_SINTERNAL(target), SHIP_SINTERNAL(target)),
                SHIP_SINTERNAL(target), 
                SHIP_MAX_SINTERNAL(target),
                contacts[i].range);

        send_to_char_f(ch,
                "Rear       %s%3d&N/&+G%-3d&N  %s%2d&N/&+g%-2d&N     Heading:&+W%d&N\r\n",
                SHIP_ARMOR_COND(SHIP_MAX_RARMOR(target), SHIP_RARMOR(target)),
                SHIP_RARMOR(target), SHIP_MAX_RARMOR(target),
                SHIP_INTERNAL_COND(SHIP_MAX_RINTERNAL(target), SHIP_RINTERNAL(target)),
                SHIP_RINTERNAL(target), 
                SHIP_MAX_RINTERNAL(target),
                (int)target->heading);

        send_to_char_f(ch,
                "Port       %s%3d&N/&+G%-3d&N  %s%2d&N/&+g%-2d&N     Bearing:&+W%-3d&N Arc:&+W%s&N\r\n",
                SHIP_ARMOR_COND(SHIP_MAX_PARMOR(target), SHIP_PARMOR(target)),
                SHIP_PARMOR(target), SHIP_MAX_PARMOR(target),
                SHIP_INTERNAL_COND(SHIP_MAX_PINTERNAL(target), SHIP_PINTERNAL(target)),
                SHIP_PINTERNAL(target), 
                SHIP_MAX_PINTERNAL(target),
                (int)contacts[i].bearing,
                get_arc_name(get_arc(ship->heading, contacts[i].bearing)));

        send_to_char_f(ch, 
                "%s %s\r\n",
                SHIP_SINKING(target) ?  "&+RSINKING&N" : 
                SHIP_IMMOBILE(target) ? "&+RIMMOBILE&N" : 
                SHIP_DOCKED(target) ?  "&+yDOCKED&N" : "",
                contacts[i].range < SCAN_RANGE ? 
                  (!SHIP_DOCKED(target) ? 
                    ((target->race == EVILSHIP) ? "&+RThis ship has an Evil flag&N" : 
                      (target->race == UNDEADSHIP) ? "&+LThis ship has a tattered black flag&N" :
                        (target->race == GOODIESHIP) ? "&+WThis ship has a goodie flag&N" :
                          "This ship does not have a flag raised") : "") : "");

        send_to_char_f(ch, "Weapons:\r\n=======================\r\n");

        for (j = 0; j < MAXSLOTS; j++)
        {
          if (target->slot[j].type == SLOT_WEAPON)
          {
            send_to_char_f(ch, "&+W[%d]&N &+W%-20s&N &+W%-9s\r\n", j,
                    SHIP_WEAPON_DESTROYED(target, j) ? "&+LDestroyed&N" : target->slot[j].get_description(),
                    target->slot[j].get_position_str());
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
    k = ABS(k);
    max = ABS(max);
    salvage = (int) (SHIPTYPE_COST(SHIP_CLASS(target)) * (float) ((float) k / (float) max));
    for (j = 0; j < MAXSLOTS; j++)
    {
      if (target->slot[j].type == SLOT_WEAPON)
      {
        if (SHIP_WEAPON_DAMAGED(target, j))
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
      salvage = MAX((int)(salvage/get_property("ship.sinking.rewardDivider", 7.)), 0);
    }
    return salvage;
}

int calc_frag_gain(P_ship ship)
{
    return SHIP_HULL_WEIGHT(ship);
}

int calc_bounty(P_ship target)
{
    return (int) ( target->frags * 10000 / get_property("ship.sinking.rewardDivider", 7.0) );
}

bool ship_gain_frags(P_ship ship, P_ship target, int frags)
{
  if (frags > 0)
  {
    int skill_frags = frags;
    if (IS_NPC_SHIP(target))
    {
      if (target == cyrics_revenge)
      {
        frags /= 5;
        skill_frags = frags;
      }
      else
      {
        skill_frags = frags / 10;
        frags = 0;
      }
    }
    if (frags > 0)
    {
        ship->frags += frags;
        act_to_all_in_ship_f(ship, "Your ship has gained %d frags!\r\n", frags);

        P_char captain = captain_is_aboard(ship);
        if (captain && frags >= 20)
        {
            gain_epic(captain, EPIC_SHIP_PVP, 0, frags / 20);
        }
    }
    ship->crew.sail_skill_raise((float)skill_frags);
    ship->crew.guns_skill_raise((float)skill_frags);
    ship->crew.rpar_skill_raise((float)skill_frags);

    update_crew(ship);
    update_ship_status(ship);
  }
  
  return true;
}

bool ship_loss_on_sink(P_ship ship, P_ship attacker, int frags)
{
  float members_loss;
  if (IS_NPC_SHIP(attacker))
  {
    members_loss = 5.0 + (float)SHIP_HULL_WEIGHT(ship) / 100.0;
    frags = 0;
  }
  else
    members_loss = 10.0 + (float)frags / 30.0;

  ship->crew.replace_members(members_loss);

  if (frags > 0)
  {
    ship->frags = MAX(0, ship->frags - frags);

    /*if (ship->frags < ship_crew_data[ship->sailcrew.index].min_frags * 0.8)
    {
      act_to_all_in_ship(ship, "&+RYour sail crew abandons you, due to your reputation!");
      setcrew(ship, sail_crew_list[0], -1);
    }
    if (ship->frags < ship_crew_data[ship->guncrew.index].min_frags * 0.8)
    {
      act_to_all_in_ship(ship, "&+RYour gun crew abandons you, due to your reputation!");
      setcrew(ship, gun_crew_list[0], -1);
    }
    if (ship->frags < ship_crew_data[ship->repaircrew.index].min_frags * 0.8)
    {
      act_to_all_in_ship(ship, "&+RYour repair crew abandons you, due to your reputation!");
      setcrew(ship, repair_crew_list[0], -1);
    }
    if (ship->frags < ship_crew_data[ship->rowingcrew.index].min_frags * 0.8)
    {
      act_to_all_in_ship(ship, "&+RYour rowing crew abandons you, due to your reputation!");
      setcrew(ship, rowing_crew_list[0], -1);
    }*/
  }

  update_crew(ship);
  return true;
}


bool ship_gain_money(P_ship ship, P_ship target, int salvage, int bounty)
{
  if (salvage > 0)
  {
    act_to_all_in_ship_f(ship, "You recieve %s for the salvage of %s!\r\nIt is currently in your ship's coffers.  To look, use 'look cargo', to get, use 'get money'\r\n", coin_stringv(salvage), target->name);
  }
  if (bounty > 0)
  {
    if (target->frags > 1000)
      act_to_all_in_ship_f(ship, "In addition to salvage, you recieve a bounty of %s for sinking THIS ship.\r\n", coin_stringv(bounty));
    else
      act_to_all_in_ship_f(ship, "In addition to salvage, you recieve a bounty of %s for sinking this ship.\r\n", coin_stringv(bounty));
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

    if (ship->autopilot)
        clear_autopilot(ship);

    if (SHIP_FLYING(ship))
        land_ship(ship);

    if(!IS_SET(ship->flags, SINKING))
        SET_BIT(ship->flags, SINKING);

    if (attacker)
    {
        if (IS_NPC_SHIP(attacker))
            SET_BIT(ship->flags, SUNKBYNPC);
        logit(LOG_SHIP, "%s's ship sunk by %s", ship->ownername, attacker->ownername);
        statuslog(AVATAR, "%s's ship sunk by %s", ship->ownername, attacker->ownername);
    }
    else
    {
        logit(LOG_SHIP, "%s's ship has sunk", ship->ownername);
        statuslog(AVATAR, "%s's ship has sunk", ship->ownername);
    }

    if (IS_WATER_ROOM(ship->location))
    {
        act_to_outside(ship, DEFAULT_RANGE, "%s &+Wstarts to sink!&N", SHIP_NAME(ship));
        act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]&N:%s &+Wstarts to sink!&N", SHIP_ID(ship), SHIP_NAME(ship));
        act_to_all_in_ship(ship, "&=LRYOUR SHIP STARTS TO SINK!!&N");
    }
    else
    {
        act_to_outside(ship, DEFAULT_RANGE, "%s &+Wstarts to fall apart!&N", SHIP_NAME(ship));
        act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]&N:%s &+Wstarts to fall apart!&N", SHIP_ID(ship), SHIP_NAME(ship));
        act_to_all_in_ship(ship, "&=LRYOUR SHIP IS DESTROYED!!&N");
    }

    if (ship == cyrics_revenge)
        ship->timer[T_SINKING] = 7500; // to let people clear it/take nexus
    //if (ship == zone_ship)
    //    ship->timer[T_SINKING] = -1; // The zone ship does not sink.
    else if (IS_NPC_SHIP(ship))
        ship->timer[T_SINKING] = number(1000, 1500); // to let people clear it
    else
        ship->timer[T_SINKING] = number(75, 150);
    ship->maxspeed = 0;
    ship->setspeed = 0;
    ship->speed = 0;

    if (attacker)
    {
        int frag_gain = 0;
        int salvage = 0;
        int bounty = 0;

        P_ship gainer = attacker;
        if (IS_NPC_SHIP(attacker))
        {
           if (attacker->npc_ai && attacker->npc_ai->escort && !IS_NPC_SHIP(attacker->npc_ai->escort))
               gainer = attacker->npc_ai->escort;
           else
               gainer = 0;
        }
        int gain_race = ship->race;
        if (IS_NPC_SHIP(ship))
        {
            if (ship->npc_ai && ship->npc_ai->escort)
                gain_race = ship->npc_ai->escort->race;
        }
        if (gainer)
        {

            if (gainer->race != gain_race)
            {
                frag_gain = calc_frag_gain(ship);
            }

            if (gainer->race != gain_race || !IS_NPC_SHIP(ship))
            {
                salvage = calc_salvage(ship);
                wizlog(56, "Salvage %d", salvage);
            }

            if ((ship->frags > 100) && (gainer->race != ship->race))
            {
                bounty = calc_bounty(ship);
                wizlog(56, "Bounty %d", bounty);
            }

            int fleet_size_grouped = 1;
            int fleet_size_total = 1;
            int k = getcontacts(ship);
            P_char ch1 = get_char2(str_dup(SHIP_OWNER(gainer)));
            for (i = 0; i < k; i++)
            {
                if (contacts[i].ship == gainer)
                    continue;
                if (SHIP_DOCKED(contacts[i].ship))
                    continue;
                if (contacts[i].ship->m_class == SH_SLOOP)
                    continue;
                if (contacts[i].ship->race == gainer->race ||
                    (contacts[i].ship->npc_ai && contacts[i].ship->npc_ai->escort && contacts[i].ship->npc_ai->escort->race == gainer->race))
                {
                   fleet_size_total++;
                }

                P_char ch2 = get_char2(str_dup(SHIP_OWNER(contacts[i].ship)));
                if (ch1 && ch2 && ch2->group && ch2->group == ch1->group)
                {
                    fleet_size_grouped++;
                }
            }

            frag_gain = frag_gain / fleet_size_total;
            salvage = salvage / fleet_size_grouped;
            bounty = bounty / fleet_size_grouped;

            for (i = 0; i < k; i++)
            {
                if (SHIP_DOCKED(contacts[i].ship))
                    continue;
                if (contacts[i].ship->m_class == SH_SLOOP)
                    continue;

                P_char ch2 = get_char2(str_dup(SHIP_OWNER(contacts[i].ship)));
                if ((contacts[i].ship == gainer) || 
                    ( ch1 && ch2 && ch2->group && ch2->group == ch1->group))
                {
                    ship_gain_frags(contacts[i].ship, ship, frag_gain);
                    ship_gain_money(contacts[i].ship, ship, salvage, bounty);
                    write_ship(contacts[i].ship);
                }
            }
        }

        ship_loss_on_sink(ship, attacker, frag_gain);
        write_ship(ship);

        if (attacker->target == ship)
        {
            attacker->target = NULL;
            if (attacker == cyrics_revenge && IS_NPC_SHIP(ship)) // dirty hack to counter revenge's appearing in pirate fight
                attacker->timer[T_BSTATION] = 0;
        }
    }
    return true;
}


void attacked_by(P_ship target, P_ship attacker, int contact_counter)
{
    if (target->npc_ai)
        target->npc_ai->attacked_by(attacker);

    for (int i = 0; i < contact_counter; i++)
    {
        if (contacts[i].ship->npc_ai && contacts[i].ship->npc_ai->escort == target)
            contacts[i].ship->npc_ai->escort_attacked_by(attacker);
    }
}

void volley_hit_event(P_char ch, P_char victim, P_obj obj, void *data)
{
    VolleyData* vd = (VolleyData*)data;
    P_ship ship = vd->attacker;
    P_ship target = vd->target;
    int w_num = vd->weapon_index;
    int hit_chance = vd->hit_chance;

    if (SHIP_DOCKED(target))
        return;
    if ((world[target->location].number < 110000)) // hid in zone, lucky bastard
        return;

    // forcing target into battle state
    if (target->timer[T_BSTATION] == 0) 
        act_to_all_in_ship(target, "&=LRYour crew scrambles to their battlestions&N!\r\n");
    target->timer[T_BSTATION] = BSTATION;

    int k = getcontacts(target);
    attacked_by(target, ship, k);

    if (dice(2, 50) >= 100 - hit_chance) 
    { // we have a hit!
        // calculating your bearing relative to target
        float your_bearing = 0;
        float range = 35.0;
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

                if ((SHIP_SAIL(target) > 0) && (number (0, 99) < weapon_data[w_index].sail_hit))
                {// hitting sails
                    damage = (int)( (float)damage * (float)weapon_data[w_index].sail_dam / 100.0 );
                    damage_sail(ship, target, damage);
                }
                else
                {// hitting hull
                    float hit_dir = your_bearing + number(-(weapon_data[w_index].hit_arc / 2), (weapon_data[w_index].hit_arc / 2));
                    if (hit_dir >= 360)    hit_dir -= 360;
                    else if (hit_dir < 0)  hit_dir = 360 + hit_dir;
                    int hit_arc = get_arc(target->heading, hit_dir);

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
            act_to_all_in_ship_f(ship, "You hit &+W[%s]&N:%s&N with a powerful &+Mmental blast&N!", SHIP_ID(target), SHIP_NAME(target));
            act_to_outside_ships(target, ship, DEFAULT_RANGE, "&+W[%s]&N:%s&N has been hit with a powerful &+Mmental blast&N!", SHIP_ID(target), SHIP_NAME(target));
            int stuntime = (int)(5.0 + 15.0 * range_mod);
            if (target == cyrics_revenge) stuntime /= 3; // demons are resistant to mindblast
            target->timer[T_MINDBLAST] = stuntime;
        }
        update_ship_status(target, ship);
    }
    else 
    {
        act_to_all_in_ship_f(ship, "You miss &+W[%s]&N:%s&N.", SHIP_ID(target), SHIP_NAME(target));
        act_to_all_in_ship_f(target, "&+W[%s]&N:%s&N misses your ship.", SHIP_ID(ship), SHIP_NAME(ship));
        act_to_outside_ships(target, ship, DEFAULT_RANGE, "&+W[%s]&N:%s&N misses &+W[%s]&N:%s&N.", SHIP_ID(ship), SHIP_NAME(ship), SHIP_ID(target), SHIP_NAME(target));
    }
    return;
}
    
int damage_sail(P_ship attacker, P_ship target, int dam)
{
    /*P_char captain = captain_is_aboard(target);
    
    // debug("Sail damage is: %d.", dam);
    
    if (captain)
    {
        dam = epic_ship_damage_control(captain, dam);
    }*/
    
    if (dam < 1)
        dam = 1;
    
    if (attacker)
    {
        act_to_all_in_ship_f(attacker, "You hit &+W[%s]&N:%s&N for &+G%d&N %s of damage on the &+Wsails&N!", SHIP_ID(target), SHIP_NAME(target), dam, (dam == 1) ? "point" : "points");
    }
    act_to_all_in_ship_f(target, "&+WYour ship&N has been hit for &+R%d&N %s of damage on the &+Wsails&N!", dam, (dam == 1) ? "point" : "points");
    act_to_outside_ships(target, attacker, DEFAULT_RANGE, "&+W[%s]&N:%s&N has been hit for %d %s of damage on the &+Wsails&N!", SHIP_ID(target), SHIP_NAME(target), dam, (dam == 1) ? "point" : "points");

    target->mainsail -= dam;
    return TRUE;
}

int damage_hull(P_ship attacker, P_ship target, int dam, int arc, int armor_pierce)
{
    /*P_char captain = captain_is_aboard(target);
    
    if (captain)
    {
        dam = epic_ship_damage_control(captain, dam);
    }*/
    
    if (dam < 1)
        dam = 1;
    
    if (attacker)
    {
        act_to_all_in_ship_f(attacker, "You hit &+W[%s]&N:%s&N for &+G%d&N %s of damage on the %s side!", SHIP_ID(target), SHIP_NAME(target), dam, (dam == 1) ? "point" : "points", get_arc_name(arc));
    }
    act_to_all_in_ship_f(target, "&+WYour ship&N has been hit for &+R%d&N %s of damage on the %s side!", dam, (dam == 1) ? "point" : "points", get_arc_name(arc));
    act_to_outside_ships(target, attacker, DEFAULT_RANGE, "&+W[%s]&N:%s&N has been hit for %d %s of damage on the %s side!", SHIP_ID(target), SHIP_NAME(target), dam, (dam == 1) ? "point" : "points", get_arc_name(arc));

    
    int weapon_hit_chance = 15;

    if (SHIP_ARMOR(target, arc) != 0)
    {
        SHIP_ARMOR(target, arc) -= dam;
        if (SHIP_ARMOR(target, arc) < 0)
        {
            dam = -SHIP_ARMOR(target, arc);
            SHIP_ARMOR(target, arc) = 0;
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

    if (SHIP_INTERNAL(target, arc) < 1)
    {// chance to bypass a rubble and hit random another arc from inside
        int other_arc = number (1, 9);
        if (other_arc < 4)
        {
            other_arc = (arc + other_arc) % 4;
            if (SHIP_INTERNAL(target, other_arc) > 0)
                arc = other_arc;
        }
    }
    if (SHIP_INTERNAL(target, arc) < 1)
        weapon_hit_chance = 100;

    SHIP_INTERNAL(target, arc) -= dam;

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

int ch_damage_hull(P_char attacker, P_ship target, int dam, int arc, int armor_pierce)
{
    if (!attacker)
        return FALSE;
    
    if (dam < 1)
        dam = 1;
    
    act("You hit a ship!", FALSE, attacker, NULL, NULL, TO_CHAR );

    act_to_all_in_ship_f(target, "&+WYour ship&N has been hit for &+R%d&N %s of damage on the %s side!", dam, (dam == 1) ? "point" : "points", get_arc_name(arc));
    act_to_outside_ships(target, target, DEFAULT_RANGE, "&+W[%s]&N:%s&N has been hit for %d %s of damage on the %s side!", SHIP_ID(target), SHIP_NAME(target), dam, (dam == 1) ? "point" : "points", get_arc_name(arc));

    
    int weapon_hit_chance = 15;

    if (SHIP_ARMOR(target, arc) != 0)
    {
        SHIP_ARMOR(target, arc) -= dam;
        if (SHIP_ARMOR(target, arc) < 0)
        {
            dam = -SHIP_ARMOR(target, arc);
            SHIP_ARMOR(target, arc) = 0;
        }
        else
        {
            if (number(0, 99) < armor_pierce)
            { // critical hit, hitting internal
                act_to_all_in_ship(target, " &+WCRITICAL HIT!&N");
                act("&+WCRITICAL HIT!&N!", FALSE, attacker, NULL, NULL, TO_CHAR );
                dam = dam / 2;
                weapon_hit_chance = 50;
            }
            else
            {
                return TRUE;
            }
        }
    }

    if (SHIP_INTERNAL(target, arc) < 1)
    {// chance to bypass a rubble and hit random another arc from inside
        int other_arc = number (1, 9);
        if (other_arc < 4)
        {
            other_arc = (arc + other_arc) % 4;
            if (SHIP_INTERNAL(target, other_arc) > 0)
                arc = other_arc;
        }
    }
    if (SHIP_INTERNAL(target, arc) < 1)
        weapon_hit_chance = 100;

    SHIP_INTERNAL(target, arc) -= dam;

    if (number(0, 99) < weapon_hit_chance)
    {
        damage_weapon(target, target, arc, dam * 5);
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
        if ((target->slot[j].type == SLOT_WEAPON) && (target->slot[j].position == arc) && !SHIP_WEAPON_DESTROYED(target, j))
            arc_weapons[i++] = j;
    }
    if (i > 0)
    {
        int w_index = arc_weapons[number(0, i - 1)];
        target->slot[w_index].val2 += dam;
        if (SHIP_WEAPON_DESTROYED(target, w_index))
        {
            act_to_all_in_ship_f(target, " &+RYour &+W%s &+Rhas been destroyed!&N", target->slot[w_index].get_description());
            if (attacker)
            {
                act_to_all_in_ship_f(attacker, " &+GYou destroy a &+W%s&+G!&N", target->slot[w_index].get_description());
            }
        }
        else
        {
            act_to_all_in_ship_f(target, " &+WYour %s has been damaged!&N", target->slot[w_index].get_description());
            if (attacker)
            {
                act_to_all_in_ship_f(attacker, " &+GYou damage a &+W%s&+G!&N", target->slot[w_index].get_description());
            }
        }
    }
}

void force_anchor(P_ship ship)
{
    act_to_all_in_ship(ship, "&=LRYour ship functions have ceased due to crew fatigue!&N\r\n");
    act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]&N:%s&N suddenly slows rapidly!\r\n", SHIP_ID(ship), SHIP_NAME(ship));
    SET_BIT(ship->flags, ANCHOR);
    ship->setspeed = 0;
}

void update_ship_status(P_ship ship, P_ship attacker)
{
    int breach_count = 0;
    for (int arc = 0; arc < 4; arc++)
    {
        if (SHIP_ARMOR(ship, arc) < 1)
            SHIP_ARMOR(ship, arc) = 0;
        if (SHIP_INTERNAL(ship, arc) < 1)
            SHIP_INTERNAL(ship, arc) = 0;
        if (SHIP_ARMOR(ship, arc) == 0 && SHIP_INTERNAL(ship, arc) == 0)
            breach_count++;
    }
    if (ship->mainsail < 1)
        ship->mainsail = 0;

    if (SHIP_SINKING(ship))
        return;

    if (breach_count > 1 && !SHIP_DOCKED(ship))
    {
        sink_ship(ship, attacker);
        return;
    }

    bool currently_immobile = SHIP_IMMOBILE(ship);
    update_maxspeed(ship, breach_count);
    if (!SHIP_DOCKED(ship))
    {
        if (SHIP_IMMOBILE(ship) && !currently_immobile)
        {
            act_to_all_in_ship(ship, "&+RYOUR SHIP IS TOO DAMAGED TO MOVE!&N");
            act_to_outside(ship, DEFAULT_RANGE, "%s is immobilized!", SHIP_NAME(ship));
            act_to_outside_ships(ship, ship, DEFAULT_RANGE, "&+W[%s]&N:%s &+Ris immobilized!", SHIP_ID(ship), SHIP_NAME(ship));
        }
        if (!SHIP_IMMOBILE(ship) && currently_immobile)
        {
            act_to_all_in_ship(ship, "&+YYOUR SHIP CAN MOVE AGAIN!&N");
        }
    }

    if (SHIP_DOCKED(ship) && ship->target)
    {
        ship->target = NULL;
    }   
}


int check_ram_arc(float heading, float bearing, float size)
{
    size /= 2;

    if (heading >= 360 - size && bearing <= size)
        heading -= 360;
    if (bearing >= 360 - size && heading <= size)
        bearing -= 360;

    if (ABS(heading - bearing) <= size)
        return TRUE;

    return FALSE;
}

int try_ram_ship(P_ship ship, P_ship target, float tbearing)
{
    int k = getcontacts(target);
    attacked_by(target, ship, k);
    
    float theading = target->heading, sheading = ship->heading;
    float sbearing = tbearing + 180;
    if (sbearing > 360) sbearing -= 360;

    int tarc = get_arc(target->heading, sbearing);
    int sarc = get_arc(ship->heading, tbearing);

    if (!check_ram_arc(sheading, tbearing, 120))
    {
        act_to_all_in_ship(ship, "&+WTarget is not in front of you to ram it!&N");
        if (IS_SET(ship->flags, RAMMING)) REMOVE_BIT(ship->flags, RAMMING);
        return FALSE;
    }

    if (SHIP_FLYING(ship) != SHIP_FLYING(target))
    {
        if (SHIP_FLYING(ship))
            act_to_all_in_ship(ship, "&+WYou try to ram but pass above your target instead!&N");
        else
            act_to_all_in_ship(ship, "&+WYou try to ram but pass below your target instead!&N");
        if (IS_SET(ship->flags, RAMMING)) REMOVE_BIT(ship->flags, RAMMING);
        return FALSE;
    }

    if (ship->speed <= BOARDING_SPEED)
    {
        act_to_all_in_ship(ship, "&+WYour ship is too slow to ram!\r\n");
        if (IS_SET(ship->flags, RAMMING)) REMOVE_BIT(ship->flags, RAMMING);
        return FALSE;
    }

    if (HAS_VALID_TARGET(ship))
        ship->crew.sail_skill_raise((float)number(1, 3));

    // Calculating relative speed 
    float heading_angle = ABS(sheading - theading);
    if (heading_angle > 180) heading_angle = 360 - heading_angle;
    float rad = heading_angle * M_PI / 180.0;

    int ram_speed = ship->speed;
    int counter_speed = (-1) * (int) ((float)target->speed * cos(rad));
    if (counter_speed < 0)
    {
        ram_speed += counter_speed;
        counter_speed = 0;
        if (ram_speed <= 0)
        {
            act_to_all_in_ship(ship, "&+WTarget speeds away from your attempt to ram it!&N");
            if (IS_SET(ship->flags, RAMMING)) REMOVE_BIT(ship->flags, RAMMING);
            return FALSE;
        }
    }


    // Calculating chance for success
    float speed_mod = (((float)ship->speed / (float)SHIPTYPE_SPEED(ship->m_class)) + ((float)target->speed / (float)SHIPTYPE_SPEED(target->m_class)) * 3) / 4.0;
    float hull_mod = (float)SHIP_HULL_WEIGHT(ship) / (float)SHIP_HULL_WEIGHT(target);
    if (target->speed == 0) 
        hull_mod /= 10;

    float chance = 0;
    if (hull_mod >= 1)
        chance = 75.0 / (hull_mod * speed_mod);
    else
        chance = 100.0 - (25.0 * hull_mod * speed_mod);
    chance = 100.0 - (100.0 - chance) / (1.0 + ship->crew.sail_mod_applied * (chance / 50.0));

    int hit_chance = BOUNDED(0, (int)chance, 100);


    act_to_all_in_ship_f(ship, "You attempt to ram &+W[%s]&N: %s! Chance to hit: &+W%d%%&N", SHIP_ID(target), SHIP_NAME(target), hit_chance);
    if (target->timer[T_BSTATION] == 0)
        act_to_all_in_ship(target, "&=LRYour crew scrambles madly to battlestations!&N\r\n");
    target->timer[T_BSTATION] = BSTATION;
    act_to_all_in_ship_f(target, "&+W[%s]&N: %s attempts to ram you!", SHIP_ID(ship), SHIP_NAME(ship));
    act_to_outside_ships(ship, target, DEFAULT_RANGE, "&+W[%s]&N:%s&N attempts to ram &+W[%s]&N:%s&N!", SHIP_ID(ship), SHIP_NAME(ship), SHIP_ID(target), SHIP_NAME(target));

    if (number(0, 99) < hit_chance)
    { // Successful ram
        if (ram_speed < ship->speed / 2)
        {
            act_to_all_in_ship_f(ship, "&+yYour ship &+Llurches &+yas you &+rcrunch &+yinto the stern of &+W[%s]&N:%s&+y!&N", SHIP_ID(target), SHIP_NAME(target));
            act_to_all_in_ship_f(target, "&+W[%s]&N:%s &+rcrunches &+yinto the stern of your ship!&N", SHIP_ID(ship), SHIP_NAME(ship));
            act_to_outside_ships(target, ship, DEFAULT_RANGE, "&+W[%s]&N:%s &+rcrunches &+yinto the stern of &+W[%s]&N:%s&+y!&N", SHIP_ID(ship), SHIP_NAME(ship), SHIP_ID(target), SHIP_NAME(target));
        }
        else if (counter_speed > ram_speed / 2)
        {
            act_to_all_in_ship_f(ship, "&+yTimbers &+rcrunch &+yand &+Ycrack &+yas you &=LRcollide &+yhead on with &+W[%s]&N:%s&+y!&N", SHIP_ID(target), SHIP_NAME(target));
            act_to_all_in_ship_f(target, "&+yTimbers &+rcrunch &+yand &+Ycrack &+yas &+W[%s]&N:%s &=LRcollides &+yhead on with your ship!&N", SHIP_ID(ship), SHIP_NAME(ship));
            act_to_outside_ships(target, ship, DEFAULT_RANGE, "&+W[%s]&N:%s &+Rcollides &+y head on with &+W[%s]&N:%s&+y!&N", SHIP_ID(ship), SHIP_NAME(ship), SHIP_ID(target), SHIP_NAME(target));
        }
        else
        {
            act_to_all_in_ship_f(ship, "&+yTimbers &+rcrunch &+yand &+Ycrack &+yas you &=LRcrash &+yinto &+W[%s]&N:%s&+y!&N", SHIP_ID(target), SHIP_NAME(target));
            act_to_all_in_ship_f(target, "&+yTimbers &+rcrunch &+yand &+Ycrack &+yas &+W[%s]&N:%s &=LRcrashes &+yinto your ship!&N", SHIP_ID(ship), SHIP_NAME(ship));
            act_to_outside_ships(target, ship, DEFAULT_RANGE, "&+W[%s]&N:%s &+Rcrashes &+yinto &+W[%s]&N:%s&+y!&N", SHIP_ID(ship), SHIP_NAME(ship), SHIP_ID(target), SHIP_NAME(target));
        }


        // Calculating damage from crash
        int ram_damage = (SHIP_HULL_WEIGHT(ship) + 100) / 10;
        ram_damage = (int)((float)ram_damage * (0.1 + 0.4 * (float)ship->speed / (float)SHIPTYPE_SPEED(ship->m_class) + 0.5 * (float)ram_speed / (float)SHIPTYPE_SPEED(ship->m_class)));
        if (sarc == SIDE_FORE)
            ram_damage = (int)((float)ram_damage * 1.2);

        int counter_damage = (SHIP_HULL_WEIGHT(target) + 100) / 10;
        counter_damage = (int)((float)counter_damage * (0.1 + 0.3 * (float)ship->speed / (float)SHIPTYPE_SPEED(ship->m_class) + 0.4 * (float)counter_speed / (float)SHIPTYPE_SPEED(target->m_class)));
        if (tarc == SIDE_FORE)
            counter_damage = (int)((float)counter_damage * 1.2);
        counter_damage += SHIP_HULL_WEIGHT(target) / SHIP_HULL_WEIGHT(ship);


        //act_to_all_in_ship(ship, "SBearing = %f, TBearing = %f, RSpeed = %d, CSpeed = %d, RDam = %d, CDam = %d\r\n", sbearing, tbearing, ram_speed, counter_speed, ram_damage, counter_damage);


        // Applying damage

        // Damage from weapon ram
        bool ship_eram = false, target_eram = false;
        if (has_eq_ram(ship) && sarc == SIDE_FORE)
        {
            int eram_dam = eq_ram_damage(ship);
            eram_dam = number(eram_dam * 0.8, eram_dam * 1.2);
            damage_hull(ship, target, eram_dam, tarc, 30);
            ship_eram = true;
        }
        if (has_eq_ram(target) && tarc == SIDE_FORE)
        {
            int counter_eram_dam = eq_ram_damage(ship);
            counter_eram_dam = number(counter_eram_dam * 0.6, counter_eram_dam * 1.0);
            damage_hull(NULL, ship, counter_eram_dam, sarc, 30);
            target_eram = true;
        }

        // Damage from crash
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
                if (SHIP_HULL_WEIGHT(ship) >= SHIP_HULL_WEIGHT(target) && number (0, 99) < 10)
                {
                    damage_sail(ship, target, dam);
                }
                else
                {
                    float hit_dir = sbearing + number(-90, 90);
                    if (hit_dir >= 360)    hit_dir -= 360;
                    else if (hit_dir < 0)  hit_dir = 360 + hit_dir;
                    int hit_arc = get_arc(target->heading, hit_dir);
                    if (target_eram && hit_arc == SIDE_FORE)  // equiped ram partially protects fore side
                        dam = MAX(1, dam/2);
                    damage_hull(ship, target, dam, hit_arc, 2);
                }
            }
            else
            {
                if (dam > counter_damage) dam = counter_damage;
                counter_damage -= dam;
                if (SHIP_HULL_WEIGHT(target) >= SHIP_HULL_WEIGHT(ship) && number (0, 99) < 10)
                {
                    damage_sail(NULL, ship, dam);
                }
                else
                {
                    float hit_dir = ship->heading + number(-90, 90);
                    if (hit_dir >= 360)    hit_dir -= 360;
                    else if (hit_dir < 0)  hit_dir = 360 + hit_dir;
                    int hit_arc = get_arc(ship->heading, hit_dir);
                    if (ship_eram && hit_arc == SIDE_FORE)   // equiped ram partially protects fore side
                        dam = MAX(1, dam/2);
                    damage_hull(NULL, ship, dam, hit_arc, 0);
                }
            }
        }


        // Changes in ships movement

        ship->x = target->x;
        ship->y = target->y;

        if (SHIP_HULL_WEIGHT(ship) >= SHIP_HULL_WEIGHT(target))
        {
            target->heading = number(0, 359);
            target->setheading = number(0, 359);
            target->speed = MIN(target->speed, BOARDING_SPEED + 1);
            ship->speed  = MIN(ship->speed, BOARDING_SPEED + 1);
        }
        if (SHIP_HULL_WEIGHT(ship) < SHIP_HULL_WEIGHT(target))
        {
            ship->setspeed = 0;
        }

        act_to_all_in_ship(target, " &+YThe impact knocks you off your feet!&N");
        stun_all_in_ship(target, PULSE_VIOLENCE);
        act_to_all_in_ship(ship, " &+YThe impact knocks you off your feet!&N");
        stun_all_in_ship(ship, PULSE_VIOLENCE);


        // Setting ram timers for successful ram
        float whole_crew_mod = (ship->crew.sail_mod_applied + ship->crew.guns_mod_applied + ship->crew.rpar_mod_applied) / 3.0;
        ship->timer[T_RAM]         = MAX(1, (int)(50.0 * (1.0 - whole_crew_mod * 0.15)));
        ship->timer[T_RAM_WEAPONS] = MAX(1, (int)(25.0 * (1.0 - ship->crew.guns_mod_applied * 0.15)));
    }
    else
    {
        // Setting ram timers for failed ram
        float whole_crew_mod = (ship->crew.sail_mod_applied + ship->crew.guns_mod_applied + ship->crew.rpar_mod_applied) / 3.0;
        ship->timer[T_RAM]         = MAX(1, (int)(25.0 * (1.0 - whole_crew_mod * 0.15)));
    }


    if (IS_SET(ship->flags, RAMMING)) REMOVE_BIT(ship->flags, RAMMING);
    update_ship_status(target, ship);
    if (SHIP_SINKING(target))
        update_ship_status(ship);
    else
        update_ship_status(ship, target);

    return TRUE;
}


/*int weaponsight(P_ship ship, int slot, ContactData& t_contact, P_char ch)
{
  int percent = 50;

  int max = weapon_data[ship->slot[slot].index].max_range;
  int min = weapon_data[ship->slot[slot].index].min_range;
  if ((t_contact.range <= (float) max) && (t_contact.range >= (float) min))
  {
    percent -= (ship->speed / 5);
    percent -= (t_contact.target->speed / 3);

    float range_mod = (max - (float)t_contact.range) / ((float)max - (float)min);
    percent += (int)((float)75 * range_mod);
    percent += (int)((float)50 * ship->guncrew.mod_applied);
    if (percent > 100)
      percent = 100;
    if (percent < 0)
      percent = 0;
    return percent;
  }
  return 0;
}*/


int weaponsight(P_ship ship, int slot, int t_contact, P_char ch)
{
  float max = (float)weapon_data[ship->slot[slot].index].max_range;
  float min = (float)weapon_data[ship->slot[slot].index].min_range;
  float t_curr_range = contacts[t_contact].range;                     // target range this turn

  if ((t_curr_range > max) || (t_curr_range < min))
      return 0;

  P_ship target = contacts[t_contact].ship;

  // first, calculating 'normal' ortogonal speed of target
  float tr_heading = target->heading - contacts[t_contact].bearing;
  if (tr_heading < 0) tr_heading += 360;
  float tr_rad = tr_heading * M_PI / 180.000;
  float ortogonal_speed = sin(tr_rad) * (float)target->speed; // targets ortogonal speed in 'ship units'

  // second, calculating projected change in relative position/heading
  float t_curr_bearing = contacts[t_contact].bearing - ship->heading;   // target relative bearing this turn
  if (t_curr_bearing < 0) t_curr_bearing += 360;
 
  // calculating next position of the ship
  float s_next_turn = get_next_heading_change(ship);
  float s_next_heading = ship->heading + s_next_turn;
  normalize_direction(s_next_heading);
  float s_next_speed = ship->speed + get_next_speed_change(ship);
  float rad = s_next_heading * M_PI / 180.000;
  float s_next_x = ship->x + (s_next_speed * sin(rad)) / 150.000;
  float s_next_y = ship->y + (s_next_speed * cos(rad)) / 150.000;

  // calculating next position of the target
  float t_next_turn = get_next_heading_change(target);
  float t_next_heading = target->heading + t_next_turn;
  normalize_direction(t_next_heading);
  float t_next_speed = target->speed + get_next_speed_change(target);
  rad = t_next_heading * M_PI / 180.000;
  float t_next_x = (float) contacts[t_contact].x + (target->x - 50.0) + (t_next_speed * sin(rad)) / 150.000;
  float t_next_y = (float) contacts[t_contact].y + (target->y - 50.0) + (t_next_speed * cos(rad)) / 150.000;

  float t_next_bearing = bearing(s_next_x, s_next_y, t_next_x, t_next_y);
  t_next_bearing -= s_next_heading;                                                       // target relative bearing next turn
  if (t_next_bearing < 0) t_next_bearing += 360;
  float t_next_range = range(s_next_x, s_next_y, ship->z, t_next_x, t_next_y, target->z); // target range next turn

  //send_to_char_f(ch, "snt=%5.2f,snx=%5.2f,sny=%5.2f,snh=%5.2f,tnt=%5.2f,tnx=%5.2f,tny=%5.2f,tnh=%5.2f   tcb=%5.2f,tcr=%5.2f,tnb=%5.2f,tnr=%5.2f", s_next_turn, s_next_x, s_next_y, s_next_heading, t_next_turn, t_next_x, t_next_y, t_next_heading, t_curr_bearing, t_curr_range, t_next_bearing, t_next_range);

  float closing_speed = ABS(t_next_range - t_curr_range) * 150.0; // in 'ship units'
  float angle_speed = ABS(t_next_bearing - t_curr_bearing);       // in degrees
  if (angle_speed > 180) angle_speed = 360 - angle_speed;

  //send_to_char_f(ch, " ort_sp=%f, cl_sp=%f, an_sp=%f", ortogonal_speed, closing_speed, angle_speed);



  float speed;
  if (IS_SET(weapon_data[ship->slot[slot].index].flags, BALLISTIC))
  {
    speed = ABS(ortogonal_speed) / 2 + ABS(angle_speed) * 3 + ABS(closing_speed); // catapults mostly depend on approaching speed
  }
  else
  {
    speed = ABS(ortogonal_speed) + ABS(angle_speed) * 4 + ABS(closing_speed) / 4; // straight weapons almost exclusively depend on ortogonal and angle speed
  }

  //send_to_char_f(ch, " speed=%5.2f", speed);

  float hit_chance = 0.5; // sloop with zero speed modifier at max range

  float speed_mod = 1 + speed / 50.0;
  hit_chance /= speed_mod;

  //send_to_char_f(ch, " hit_1=%5.2f", hit_chance * 100);

  float size_mod = SHIP_HULL_MOD(target) - 3; // to compensate for sloops size (~0-17)
  float size_to_speed_mod = 1 + (size_mod / 100) / hit_chance; // the faster it goes, the more hullsize affects it
  hit_chance = hit_chance * size_to_speed_mod;

  //send_to_char_f(ch, " hit_2=%5.2f", hit_chance * 100);
  
  float miss_chance = 1.0 - hit_chance;
  float max_range_miss = miss_chance;
  miss_chance -= 0.05;
  float min_range_miss = miss_chance * miss_chance * miss_chance * miss_chance;
  float range_mod = (max - t_curr_range) / (max * 0.75); // if range below max/4, there is no-penalty (and yes, too bad for longtom)
  if (range_mod < 0.0) range_mod = 0.0;
  if (range_mod > 1.0) range_mod = 1.0;

  miss_chance = max_range_miss - (max_range_miss - min_range_miss) * range_mod;  // ok here we have our miss chance for the basic crew

  //send_to_char_f(ch, " hit_3=%5.2f", (1.0 - miss_chance) * 100);

  miss_chance /= (1.0 + ship->crew.guns_mod_applied);
  if (SHIP_FLYING(target)) miss_chance = MIN(miss_chance * 1.5, 0.99);
  hit_chance = 1.0 - miss_chance;
  hit_chance *= ship->crew.get_stamina_mod();
  if (hit_chance < 0.01) hit_chance = 0.01;
  if (hit_chance > 1.00) hit_chance = 1.00;

  //send_to_char_f(ch, " hit_4=%5.2f\n", hit_chance * 100);
  
  //if ((t_curr_range > (float) max) || (t_curr_range < (float) min)) // remove it
  //    return 0;

  return (int)(hit_chance * 100);
}

int fire_weapon(P_ship ship, int w_num, int t_contact, P_char ch)
{
    int hit_chance = weaponsight(ship, w_num, t_contact, ch);
    return fire_weapon(ship, w_num, t_contact, hit_chance, ch);
}


int fire_weapon(P_ship ship, int w_num, int t_contact, int hit_chance, P_char ch)
{
    P_ship target = contacts[t_contact].ship;
    float range = contacts[t_contact].range;
    int w_index = ship->slot[w_num].index;

    // displaying firing messages
    act_to_all_in_ship_f(ship, "Your ship fires &+W%s&N at &+W[%s]&N:%s! Chance to hit: &+W%d%%&N", ship->slot[w_num].get_description(), target->id, target->name, hit_chance);
    act_to_all_in_ship_f(target, "&+W[%s]&N:%s&N fires %s at your ship!", SHIP_ID(ship), ship->name, ship->slot[w_num].get_description());
    act_to_outside(ship, DEFAULT_RANGE, "%s&N fires %s at %s!", ship->name, ship->slot[w_num].get_description(), target->name);
    act_to_outside_ships(ship, target, DEFAULT_RANGE, "&+W[%s]&N:%s&N fires %s at &+W[%s]&N:%s!", SHIP_ID(ship), ship->name, ship->slot[w_num].get_description(), target->id, target->name);

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
    if (ship->slot[w_num].val1 == 1 && ch) 
        send_to_char("&+RWarning! This is the last round of ammo!\r\n", ch);
    if (ship->slot[w_num].val1 > 0)
       ship->slot[w_num].val1--;

    if (ship->slot[w_num].val1 > 0)
    {
        float reload_time = (float)weapon_data[w_index].reload_time * (1.0 - ship->crew.guns_mod_applied * 0.15);
        reload_time /= ship->crew.get_stamina_mod();
        ship->slot[w_num].timer = MAX(1, (int)reload_time);
    }

    // reducing crew stamina
    ship->crew.reduce_stamina((float)weapon_data[w_index].weight / (SHIP_HULL_MOD(ship) / 10.0), ship);
    if (HAS_VALID_TARGET(ship))
        ship->crew.guns_skill_raise(0.1);

    return TRUE;
}


