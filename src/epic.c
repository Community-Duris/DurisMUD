#include <stdio.h>
#include <string.h>
#include <math.h>

#include <string>
#include <vector>
#include <list>
using namespace std;

#include "structs.h"
#include "spells.h"
#include "comm.h"
#include "interp.h"
#include "prototypes.h"
#include "utils.h"
#include "utility.h"
#include "damage.h"
#include "db.h"
#include "sql.h"
#include "epic.h"
#include "objmisc.h"
#include "events.h"
#include "random.zone.h"
#include "timers.h"
#include "assocs.h"
#include "nexus_stones.h"
#include "auction_houses.h"
#include "boon.h"
#include "trophy.h"
#include "epic_bonus.h"
#include "epic_skills.h"
#include "achievements.h"

extern long boot_time;
extern P_room world;
extern P_index obj_index;
extern P_index mob_index;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern P_obj object_list;
extern int top_of_objt;
extern P_desc descriptor_list;
extern Skill skills[];
extern long new_exp_table[];  // Arih: Fixed type mismatch bug - was int, should be long
extern void event_reset_zone(P_char, P_char, P_obj, void *);

extern epic_reward epic_rewards[];
extern epic_teacher_skill epic_teachers[];

vector<epic_zone_completion> epic_zone_completions;

const char *prestige_names[EPIC_MAX_PRESTIGE] = {
  "Unknown",
  "Serf",
  "Peasant",
  "Commoner",
  "Citizen",
  "Squire",
  "Noble",
  "Knight",
  "Hero",
  "Lord",
  "Champion",
  "Living Legend"
};

int errand_notch;

int epic_points(P_char ch)
{
  for (int i = 0; epic_teachers[i].vnum; i++) {
    mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;
  }
  return 0;
}

const char *epic_prestige(P_char ch)
{
  return prestige_names[MIN(GET_EPIC_POINTS(ch)/get_property("epic.prestigeNotch", 400), EPIC_MAX_PRESTIGE-1)];
}
/* shouldn't need this now - Zion 4/8/2014
int epic_skillpoints(P_char ch)
{
  if(!ch || !IS_PC(ch))
    return 0;

  return ch->only.pc->epic_skill_points;
}

void epic_gain_skillpoints(P_char ch, int gain)
{
  if(!ch || !IS_PC(ch))
    return;

  ch->only.pc->epic_skill_points = MAX(0, ch->only.pc->epic_skill_points + gain);
} */

bool epic_stored_in(unsigned long *vector, int code)
{
  unsigned long flag = *vector;

  if((flag >> 30) == 0)
    *vector = (unsigned long)code;
  else if((flag >> 30) == 1)
    *vector |= (unsigned long)code << 10;
  else if((flag >> 30) == 2)
    *vector |= (unsigned long)code << 20;
  else
    return false;

  *vector += 0x40000000;

  return true;
}

void epic_complete_errand(P_char ch, int zone)
{
  struct affected_type af, *afp;

  for (afp = ch->affected; afp; afp = afp->next) {
    if(afp->type == TAG_EPIC_COMPLETED &&
        afp->modifier < 12) {
      if(!epic_stored_in(&afp->bitvector, zone))
        if(!epic_stored_in(&afp->bitvector2, zone))
          if(!epic_stored_in(&afp->bitvector3, zone))
            epic_stored_in(&afp->bitvector4, zone);
      afp->modifier++;
      break;
    }
  }

  if(!afp) {
    memset(&af, 0, sizeof(af));
    af.type = TAG_EPIC_COMPLETED;
    af.modifier = 1;
    af.flags = AFFTYPE_STORE | AFFTYPE_PERM;
    af.duration = -1;
    af.bitvector = 0x40000000 | zone;
    affect_to_char(ch, &af);
  }
}

int epic_random_task_zone(P_char ch)
{
  int zone_number = -1;
#ifdef __NO_MYSQL__
  return zone_number;
#else
  if(!qry("select number, name from zones where task_zone = 1 and number not in " \
           "(select type_id from epic_gain where pid = '%d' and type = '%d') " \
           "order by rand() limit 1", GET_PID(ch), EPIC_ZONE))
    return -1;

  MYSQL_RES *res = mysql_store_result(DB);

  if(mysql_num_rows(res) > 0)
  {
    MYSQL_ROW row = mysql_fetch_row(res);
    zone_number = atoi(row[0]);
  }

  mysql_free_result(res);

  return zone_number;

#endif
}

void epic_choose_new_epic_task(P_char ch)
{
  char buffer[512];
  P_obj nexus;
  struct affected_type af, *afp;
  int zone_number = -1;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  memset(&af, 0, sizeof(af));
  af.type = TAG_EPIC_ERRAND;
  af.flags = AFFTYPE_STORE | AFFTYPE_PERM;
  af.duration = -1;

  // Now a 5% chance to get spill blood down from 9%.
  if( number(0, 19) )
  {
    zone_number = epic_random_task_zone(ch);
  }

  // Getting rid of nexus . 7/13 Drannak
  // Adding nexus back. 5/23/2016
  if( zone_number < 0 )
  {
    nexus = get_random_enemy_nexus(ch);
    // 10% chance.
    if( (number(1, 100) <= 10) && (GET_LEVEL(ch) >= 51) && nexus )
    {
      act("The Gods of &+rDuris&n demand that you seek out $p and convert it!", FALSE, ch, nexus, 0, TO_CHAR);
      // Nexus stone IDs run from 1 on up.
      af.modifier = SPILL_BLOOD + STONE_ID(nexus);
    }
    else
    {
      send_to_char("The Gods of &+rDuris&n demand that you &+rspill the &+Rblood&n of the &+Lenemies&n of your race!\n", ch);
      af.modifier = SPILL_BLOOD;
    }
  }
  else
  {
    snprintf(buffer, 512, "The Gods of &+rDuris&n have sent you to seek out the &+Bmagical &+Lstone&n of %s!\n", zone_table[real_zone0(zone_number)].name);
    send_to_char(buffer, ch);
    af.modifier = zone_number;
  }

  affect_to_char(ch, &af);
}

vector<epic_trophy_data> get_epic_zone_trophy(P_char ch)
{
  vector<epic_trophy_data> trophy;

#ifdef __NO_MYSQL__
  debug("get_epic_zone_trophy(): __NO_MYSQL__, returning 0");
  return trophy;
#else
  if(!qry("select type_id from epic_gain where pid = '%d' and type = '%d' order by time asc", GET_PID(ch), EPIC_ZONE))
    return trophy;

  MYSQL_RES *res = mysql_store_result(DB);

  if(!res)
  {
    mysql_free_result(res);
    return trophy;
  }

  list<epic_trophy_data> tq;

  int trophy_size = (int) get_property("epic.zoneTrophy.size", 40);

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res)))
  {
    int zone_number = atoi(row[0]);

    bool in_trophy = false;
    for(list<epic_trophy_data>::iterator it = tq.begin(); it != tq.end(); it++)
    {
      if(it->zone_number == zone_number)
      {
        in_trophy = true;
        break;
      }
    }

    if(!in_trophy)
    {
      tq.push_front(epic_trophy_data(zone_number, 1));
      if(tq.size() > trophy_size) tq.pop_back();
    }
    else
    {
      for(list<epic_trophy_data>::iterator it = tq.begin(); it != tq.end(); it++)
      {
        if(it->zone_number == zone_number)
        {
          it->count++;
          break;
        }
      }
    }

  }

  while(!tq.empty())
  {
    trophy.push_back(tq.front());
    tq.pop_front();
  }

  mysql_free_result(res);

  return trophy;

#endif
}

int modify_by_epic_trophy(P_char ch, int amount, int zone_number)
{
  vector<epic_trophy_data> trophy = get_epic_zone_trophy(ch);

  for(vector<epic_trophy_data>::iterator it = trophy.begin(); it != trophy.end(); it++)
  {
    if(zone_number == it->zone_number  && it->count > 0)
    {
      float factor = pow(get_property("epic.zoneTrophy.mod", 0.8), MIN(it->count, get_property("epic.zoneTrophy.maxMods", 4)));
      amount = (int) (amount * factor);
      amount = MAX(1, amount);

      switch(it->count)
      {
        case 1:
          send_to_char("This seems familiar somehow...\n", ch);
          break;

        case 2:
        case 3:
          send_to_char("&+GHaven't you seen all of this before?\n", ch);
          break;

        case 4:
        default:
          send_to_char("&+RThis is getting extremely boring.\n", ch);
          break;
      }

      return amount;
    }

  }

  return amount;
}

void group_gain_epic(P_char ch, int type, int data, int amount)
{
  gain_epic(ch, type, data, amount);

  if(ch->group)
  {
    for(struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if(gl->ch == ch) continue;
      if(gl->ch->in_room == ch->in_room)
      {
        gain_epic(gl->ch, type, data, amount);
      }
    }
  }
}

void gain_epic(P_char ch, int type, int data, int amount)
{
  char buffer[256];
  struct affected_type af, *afp;
  int notch = get_property("epic.skillPointStep", 100);

  // If invalid ch or bad load of errand_notch (don't care about notch as we don't use skillpoints anymore).
  if( !IS_ALIVE(ch) || errand_notch < 1 )
  {
    debug( "gain_epic: Bad ch '%s' %d, or bad errand_notch %d.", (ch==NULL) ? "NULL" : J_NAME(ch), errand_notch );
    return;
  }

  // If we're not gaining epics, then perhaps we should be using a different function?  NPCs don't gain epics.
  if( amount < 1 || IS_NPC(ch))
  {
    return;
  }

  // Epic bonus from witch potion.
  if( IS_AFFECTED4(ch, AFF4_EPIC_INCREASE) && type != EPIC_BOTTLE )
  {
    send_to_char("You feel the &+cblessing&n of the &+WGods&n wash over you.\n", ch);
    amount = (int) (amount * get_property("epic.witch.multiplier", 1.5));
  }

  // These don't get hacked by being tasked: randommob (only 1 epic to start with), strahdme (super acheivement),
  //   bottle (epic bottles), PvP, ship PvP, or boons.
  if( type != EPIC_RANDOMMOB && type != EPIC_STRAHDME && type != EPIC_BOTTLE && type != EPIC_PVP && type != EPIC_SHIP_PVP
    && type != EPIC_BOON && has_epic_task(ch) )
  {
    send_to_char("You have not completed the task given to you by the Gods, \n" \
                 "so you are not able to progress at usual pace.\n", ch);
    amount = MAX(1, (int) (amount * get_property("epic.errand.penaltyMod", 0.25)));
  }

  // Epic bottles don't get modifiers from nexus.
  if( type != EPIC_BOTTLE )
  {
    // For marduk nexus stone... to change rate use property nexusStones.bonus.epics
    amount = check_nexus_bonus(ch, amount, NEXUS_BONUS_EPICS);
    amount = amount + (int)((float)amount * get_epic_bonus(ch, EPIC_BONUS_EPIC_POINT));

    if(GET_RACEWAR(ch) == RACEWAR_GOOD)
    {
      amount = amount * (float)get_property("epic.gain.modifier.good", 1.000);
    }
    if(GET_RACEWAR(ch) == RACEWAR_EVIL)
    {
      amount = amount * (float)get_property("epic.gain.modifier.evil", 1.000);
    }
  }

  // add guild prestige
  if( GET_ASSOC(ch) )
    GET_ASSOC(ch)->add_points_from_epics(ch, amount, type);

//  int old = ch->only.pc->epics; - Disabled epic skill points.
  snprintf(buffer, 256, "You have gained %d epic point%s.\n", amount, amount == 1 ? "" : "s");
  send_to_char(buffer, ch);
  ch->only.pc->epics += amount;
  log_epic_gain(GET_PID(ch), type, data, amount);
  char type_str[20];

  switch(type)
  {
    case EPIC_ZONE:
      strcpy(type_str, "ZONE");
      break;
    case EPIC_PVP:
      strcpy(type_str, "PVP");
      break;
    case EPIC_SHIP_PVP:
      strcpy(type_str, "PVP_SHIP");
      break;
    case EPIC_ELITE_MOB:
      strcpy(type_str, "ELITE_MOB");
      break;
    case EPIC_QUEST:
      strcpy(type_str, "QUEST");
      break;
    case EPIC_RANDOM_ZONE:
      strcpy(type_str, "RANDOM_ZONE");
      break;
    case EPIC_NEXUS_STONE:
      strcpy(type_str, "NEXUS_STONE");
      break;
    case EPIC_BOON:
      strcpy(type_str, "BOON");
      break;
    case EPIC_BOTTLE:
      strcpy(type_str, "BOTTLE");
      break;
    case EPIC_STRAHDME:
      strcpy(type_str, "STRAHD_ME");
      break;
    case EPIC_RANDOMMOB:
      strcpy(type_str, "RANDOM_MOB");
      break;
    default:
      strcpy(type_str, "UNKNOWN");
      break;
  }

  epiclog( 56, "%s received %d epic points (%s)", ch->player.name, amount, type_str);

  /*
    exp.maxExpLevel means the highest level you can reach with just experience (i.e., without epics)
    epic.maxFreeLevel means the highest level you can reach by touching any stone. higher levels you have
    to touch specific stones to level.
  */

  if( GET_LEVEL(ch) >= get_property("exp.maxExpLevel", 46)
    && GET_LEVEL(ch) < get_property("epic.maxFreeLevel", 50) )
  {
     epic_free_level(ch);
     //advance_level(ch);//, FALSE); handles leveling for wipe2011
  }

  // Feed artifacts, and add to sum of epics gained (epic bottles don't count towards total epics).
  if( type != EPIC_BOTTLE )
  {
    epic_feed_artifacts(ch, amount, type);

    // Handle the total number of epics ch has gained.
    if ((afp = get_spell_from_char(ch, TAG_EPICS_GAINED)))
    {
      afp->modifier += amount;
    }
    else
    {
      afp = apply_achievement(ch, TAG_EPICS_GAINED);
      afp->modifier = amount;
    }
  }
  // Epic bottles no longer task you.
  else
  {
    return;
  }

/* Lets do away with skill points---we don't need them at all with the new system.
  int skill_notches = MAX(0, (int) ((old+amount)/notch) - (old/notch));
  //if(skill_notches)
  if( add_epiccount(ch, amount) )
  {
    send_to_char("&+WYou have gained an epic skill point!&n\n", ch);
    epic_gain_skillpoints(ch, skill_notches);
  }
*/
  if( (afp->modifier - amount) / errand_notch < afp->modifier / errand_notch && !has_epic_task(ch))
  {
    debug( "%s got new task: old epics: %d, new epics: %d, errand_notch: %d, %d < %d.",
      J_NAME(ch), afp->modifier - amount, afp->modifier, errand_notch, (afp->modifier - amount) / errand_notch,
      afp->modifier / errand_notch );
    epiclog( 56, "%s got new task: old epics: %d, new epics: %d, errand_notch: %d, %d < %d.",
      J_NAME(ch), afp->modifier - amount, afp->modifier, errand_notch, (afp->modifier - amount) / errand_notch,
      afp->modifier / errand_notch );
    epic_choose_new_epic_task(ch);
  }

}

struct affected_type *get_epic_task(P_char ch)
{
  struct affected_type *hjp;

  if(!ch)
    return NULL;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if(hjp->type == TAG_EPIC_ERRAND)
      return hjp;

  return NULL;
}

bool has_epic_task(P_char ch)
{
  return (get_epic_task(ch) != NULL);
}

void epic_frag(P_char ch, int victim_pid, int amount)
{
  struct affected_type *afp;

  if ((afp = get_epic_task(ch)))
  {
    if( abs(afp->modifier) == SPILL_BLOOD )
    {
      send_to_char("The &+rGods of Duris&n are very pleased with this &+rblood&n.\n", ch);
      send_to_char("You can now progress further in your quest for epic power!\n", ch);
      // Spill blood task is now a flat 500 epics, not a multiplier since that very easily translates to 3*0=0.
      amount += 500;
      affect_remove(ch, afp);
    }
  }
  gain_epic(ch, EPIC_PVP, victim_pid, amount);
}

void epic_feed_artifacts(P_char ch, int epics, int epic_type)
{
  P_obj obj;

  if( IS_TRUSTED(ch) || !IS_PC(ch) )
  {
    return;
  }

  /* Disabled this code because we're feeding normal now and letting artis fight in artifact_wars().
  int num_artis = 0;
  for (int i = 0; i < MAX_WEAR; i++)
  {
    if(ch->equipment[i] && (IS_ARTIFACT(ch->equipment[i]) || isname("powerunique", ch->equipment[i]->name)))
    {
      num_artis++;
    }
  }
  */

  int feed_seconds = (epics * get_property("artifact.feeding.epic.point.seconds", 3600));

  switch(epic_type)
  {
    case EPIC_ZONE:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.zone", 1.0));
      break;
    case EPIC_PVP:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.pvp", 2.0));
      break;
    case EPIC_SHIP_PVP:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.pvpShip", 2.0));
      break;
    case EPIC_ELITE_MOB:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.eliteMob", 1.0));
      break;
    case EPIC_QUEST:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.quest", 1.0));
      break;
    case EPIC_RANDOM_ZONE:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.randomZone", 1.0));
      break;
    case EPIC_NEXUS_STONE:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.nexusStone", 1.0));
      break;
    // This can add up quickly for repeatable boons, so we use a value less than 1.
    case EPIC_BOON:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.boon", 0.25));
      break;
    case EPIC_STRAHDME:
    case EPIC_RANDOMMOB:
      break;
    default:
      feed_seconds = 0;
      break;
  }

  /* Making feed normal because we have artifact_wars for chars with multiple artis now.
  // Changed slope from x to 2x for num_artis, making it 3x as hard for 2 artis, 5x as hard for 3, 7x for 4, and so on.
  if( num_artis > 0 )
  {
    feed_seconds = (int) (feed_seconds / (2 * num_artis - 1));
  }
  */

  if( affected_by_spell(ch, TAG_PLR_RECENT_FRAG) )
  {
    feed_seconds = (feed_seconds * 3 ) / 2;
  }

  for( int i = 0; i < MAX_WEAR; i++ )
  {
    obj = ch->equipment[i];
    if( obj && IS_ARTIFACT(obj) )
    {
      artifact_feed_sql(ch, obj, feed_seconds, ((epic_type == EPIC_PVP || epic_type == EPIC_SHIP_PVP) ? TRUE : FALSE));
    }
  }
}

/* epic stones absorb smaller stones and level potions into themselves */
void epic_stone_absorb(P_obj obj)
{
  P_obj obj_list = NULL;

  if(OBJ_ROOM(obj))
  {
    obj_list = world[obj->loc.room].contents;
  }
  else if(OBJ_CARRIED(obj))
  {
    obj_list = (obj->loc.carrying)->carrying;
  }

  if(!obj_list)
    return;

  for(P_obj tobj = obj_list; tobj; tobj = tobj->next_content)
  {
    if(tobj == obj)
      continue;

    /* if the other object is smaller epic stone, absorb it */
    if( OBJ_VNUM(tobj) <= OBJ_VNUM(obj) && obj_index[tobj->R_num].func.obj == epic_stone )
    {
      extract_obj(tobj);
    }

    /* if there is a level potion on the ground, absorb it */
    if(tobj->affected[0].location == APPLY_LEVEL)
    {
      extract_obj(tobj);
    }
  }
}

/* calculate the epic point payout based on members in group */
int epic_stone_payout(P_obj obj, P_char ch)
{
  int num_players;
  if(ch->group)
  {
    num_players = 0;
    for(struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if( IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) && (gl->ch->in_room == ch->in_room) )
      {
        num_players++;
      }
    }
  }
  else
  {
    num_players = 1;
  }

  if( (num_players < obj->value[1]) && (obj->value[1] != 0) )
      num_players = obj->value[1];

  // Payout is the base value of the stone * (100% for up to max group size, then max/current % for larger groups).
  int payout = (int) (obj->value[0] * obj->value[1] / num_players);
  // Max_payout is just the base value of the stone * 10.
  int max_payout = (int) (obj->value[0] * (float) get_property("epic.touch.maxPayoutFactor", 10.));

//  DEPRECATED - Torgal 12/21/09
//  float freq_mod = get_epic_zone_frequency_mod(obj->value[2]);
//  int __old_epic_value = epic_value;
//
//  epic_value = MAX(1, (int) (epic_value * freq_mod));
//  debug("epic_stone_payout:freq_mod: old_epic_value: %d, epic_value: %d", __old_epic_value, epic_value);

  float alignment_mod = get_epic_zone_alignment_mod(obj->value[2], GET_RACEWAR(ch));
  int __old_epic_value = payout;

  payout = MAX(1, (int) (payout * alignment_mod));
  debug("epic_stone_payout:alignment_mod: old_epic_value: %d, epic_value: %d", __old_epic_value, payout);

  int epic_value = payout * get_property("epic.touch.PayoutFactor", 1.000);

  epic_value = BOUNDED(1, epic_value, max_payout);

  return epic_value;
}

void epic_stone_feed_artifacts(P_obj obj, P_char ch)
{
  int feed_amount = 0;
  switch(OBJ_VNUM(obj))
  {
    case EPIC_MONOLITH:
      feed_amount = 3600 * get_property("artifact.feeding.epic.hours.monolith", 12);
      break;

    case EPIC_LARGE_STONE:
      feed_amount = 3600 * get_property("artifact.feeding.epic.hours.large", 6);
      break;

    case EPIC_SMALL_STONE:
      feed_amount = 3600 * get_property("artifact.feeding.epic.hours.small", 1);
      break;

    default:
      feed_amount = 0;
  }
}

void epic_stone_set_affect(P_char ch)
{
  struct affected_type af;
  memset(&af, 0, sizeof(af));
  af.type = TAG_EPIC_MONOLITH;
  af.flags = AFFTYPE_STORE | AFFTYPE_PERM;
  af.duration = 1 + get_property("epic.monolith.activeTime", 60)/75;
  affect_to_char(ch, &af);
}

__attribute__ ((deprecated))
void epic_free_level(P_char ch)
{
  char buf[256];
  snprintf(buf, 256, "epic.forLevel.%d", GET_LEVEL(ch)+1);

  int epics_for_level = get_property(buf, 1 << ((GET_LEVEL(ch) + 1) - 43));

  if(GET_EXP(ch) >= new_exp_table[GET_LEVEL(ch)+1] &&
         ch->only.pc->epics >= epics_for_level)
     {
         GET_EXP(ch) -= new_exp_table[GET_LEVEL(ch) + 1];
	ch->only.pc->epics -= epics_for_level;
         advance_level(ch);//, FALSE); wipe2011
		 wizlog(56, "%s has attained epic level &+W%d&n!",
                GET_NAME(ch),
                GET_LEVEL(ch));
	
     }

}

void epic_stone_level_char(P_obj obj, P_char ch)
{
  char buf[256];
  int  epics_for_level, anystone_epics_for_level;
  int  levelcap = sql_level_cap( GET_RACEWAR(ch) );

  if( !IS_ALIVE(ch) || IS_NPC(ch) || !obj )
  {
    debug( "epic_stone_level_char: Bad argument(s)." );
    logit( LOG_DEBUG, "epic_stone_level_char: Bad argument(s): Char '%s' : %s, obj: %s (%d).",
      (ch == NULL) ? "NULL" : J_NAME(ch), IS_ALIVE(ch) ? "ALIVE" : "NOT ALIVE",
      (obj == NULL) ? "NULL" : obj->short_description, (obj == NULL) ? -1 : OBJ_VNUM(obj) );
    epiclog( 56, "epic_stone_level_char: Bad argument(s): Char '%s' : %s, obj: %s (%d).",
      (ch == NULL) ? "NULL" : J_NAME(ch), IS_ALIVE(ch) ? "ALIVE" : "NOT ALIVE",
      (obj == NULL) ? "NULL" : obj->short_description, (obj == NULL) ? -1 : OBJ_VNUM(obj) );
    return;
  }

  // Already attained max level or doesn't want to spend epics to level.
  if( GET_LEVEL(ch) >= MAXLVLMORTAL || PLR3_FLAGGED(ch, PLR3_NOLEVEL)
    || (GET_LEVEL( ch ) >= levelcap) )
  {
    return;
  }

  if( IS_MULTICLASS_PC(ch) && GET_LEVEL(ch) >= get_property("exp.maxMultiLevel", MAXLVLMORTAL) )
  {
    return;
  }

  snprintf(buf, 256, "epic.forLevel.%d", GET_LEVEL(ch)+1);
  // If the property can not be found, set epics_for_level to -1.
  epics_for_level = get_property(buf, -1);

  // This is a serious error.. results in nobody reaching lvl GET_LEVEL(ch)+1.
  if( epics_for_level == -1 )
  {
    send_to_char( "&+RError in epic leveling please tell a God immediately.&n\n", ch );
    debug( "epic_stone_level_char: Couldn't find property '%s' which is vital for leveling.", buf );
    logit(LOG_DEBUG, "epic_stone_level_char: Couldn't find property '%s' which is vital for leveling.", buf );
    epiclog(56, "epic_stone_level_char: Couldn't find property '%s' which is vital for leveling.", buf );
    return;
  }

  // Multiclass now use same amount of epics for leveling.
  // Currently set to 1. - Lohrr 7/26/2014
  if(IS_MULTICLASS_PC(ch) && GET_LEVEL(ch) >= 51)
  {
    epics_for_level *= (int) get_property("exp.multiEpicMultiplier", 2);
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
  epics_for_level = (int)(epics_for_level/3);
#endif

  // Amount of epics to level at any stone.
  anystone_epics_for_level = epics_for_level * 2;

  // If they have the exp, and epics and touch right stone, or double epics..
  if( GET_EXP(ch) >= new_exp_table[GET_LEVEL(ch)+1]
    && ((ch->only.pc->epics >= epics_for_level && GET_LEVEL(ch) == obj->value[3] - 1)
    || ch->only.pc->epics >= anystone_epics_for_level) )
  {
    GET_EXP(ch) -= new_exp_table[GET_LEVEL(ch) + 1];
    ch->only.pc->epics -= epics_for_level;
    advance_level(ch);
	  wizlog(56, "%s has attained epic level &+W%d&n!", GET_NAME(ch), GET_LEVEL(ch));
  }
}

void epic_stone_one_touch(P_obj obj, P_char ch, int epic_value)
{
  int  curr_epics;
  char buf[256];

  if( !obj || !ch || !epic_value || IS_NPC(ch) )
    return;

  curr_epics = ch->only.pc->epics;

  /*if(get_zone_exp(ch, world[ch->in_room].zone) < calc_min_zone_exp(ch))	
  {
    act("The burst of &+Bblue energy&n from $p flows around $n, leaving them unaffected!",
        FALSE, ch, obj, ch, TO_NOTVICT);
    act("The burst of &+Bblue energy&n from $p flows around you, leaving you unaffected!",
        FALSE, ch, obj, 0, TO_CHAR);
    send_to_char("You did not gain enough experience here before trying to gain more power!\n", ch);
    return;
  }*/

  act("The mystic &+Bblue energy&n from $p flows into $n!", FALSE, ch, obj, ch, TO_NOTVICT);
  act("The mystic &+Bblue energy&n from $p flows into you!", FALSE, ch, obj, 0, TO_CHAR);

  act("From deep inside, you realize that you have reached one of the key nodes of the\n"
      "magical energies flowing through the World of &+rDuris&n!\n"
      "Your body and mind align smoothly with the energy, embracing its powers, giving\n"
      "you strength and new knowledge!", FALSE, ch, obj, 0, TO_CHAR);

  epic_stone_set_affect(ch);

  /* if char is completing their epic errand, give them extra epic points! */
  struct affected_type *afp = get_epic_task(ch);
  if( afp && (( afp->modifier == obj->value[2] ) || ( 0 - afp->modifier == obj->value[2] )) )
  {
    send_to_char("The &+rGods of Duris&n are very pleased with your achievement!\n"
                 "You can now continue with your quest for &+Wpower!\n", ch);
    epic_complete_errand(ch, afp->modifier);
    affect_remove(ch, afp);
    gain_epic(ch, EPIC_ZONE, obj->value[2], (int) (epic_value * get_property("epic.errand.completeBonusMod", 1.5)));

  } else {
    /* not on epic errand, just give them the epic points */
    gain_epic(ch, EPIC_ZONE, obj->value[2], epic_value);
  }

  snprintf(buf, 256, "epic.forLevel.%d", GET_LEVEL(ch) + 1 );
  // Characters can now level up to 55 by epics and exp alone - 11/13/12 Drannak
  // Characters can now level up to 56 with double epics/exp. If they get BIT_32*2 epics they can be a imm... not.
  // Characters with exp >= exp needed for lvl need epics to level.
  if( (GET_EXP(ch) >= new_exp_table[GET_LEVEL(ch)+1])
    && ((GET_LEVEL(ch) == (obj->value[3] - 1))
    || (curr_epics / 2 > get_property( buf, (int)BIT_32 ))) )
  {
    epic_stone_level_char(obj, ch);
  }
  check_boon_completion(ch, NULL, obj->value[2], BOPT_ZONE);
}

int epic_stone(P_obj obj, P_char ch, int cmd, char *arg)
{
  int zone_number = -1;
  char arg1[MAX_INPUT_LENGTH];
  P_obj stoneobj = NULL;

  if(cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(obj && cmd == CMD_PERIODIC)
  {
    /* periodic call */
    epic_stone_absorb(obj);

    /* set zone id */
    if(!obj->value[2])
    {
      zone_number = zone_table[obj_zone_id(obj)].number;
      obj->value[2] = zone_number;

      // set epic payout, suggested_group_size and epic_level from db
      struct zone_info zinfo;
      if(get_zone_info(zone_number, &zinfo))
      {
        obj->value[0] = zinfo.epic_payout;
        obj->value[1] = zinfo.suggested_group_size;
        obj->value[3] = zinfo.epic_level;
      }
    }

    if(OBJ_ROOM(obj))
    {
      REMOVE_BIT(obj->wear_flags, ITEM_TAKE);

      if(OBJ_MAGIC(obj) && !number(0,5))
      {
        act("A powerful humming sound can be heard from $p.", FALSE, 0, obj, 0, TO_ROOM);
      }
    }
  }

  if(cmd == CMD_TOUCH && IS_PC(ch))
  {
    one_argument(arg, arg1);
    stoneobj = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if(!stoneobj)
    {
      stoneobj = get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents);
      if(!stoneobj)
        return FALSE;
    }

    if(stoneobj != obj)
      return FALSE;

    zone_number = obj->value[2];

    /* the (magic) flag determines if the stone has been touched or not */
    if(!OBJ_MAGIC(obj) || epic_zone_done_now(zone_number))
    {
      act("$p seems to be powerless.", FALSE, ch, obj, 0, TO_CHAR);
      return TRUE;
    }

    for (P_char tmp_p = world[ch->in_room].people; tmp_p; tmp_p = tmp_p->next_in_room)
    {
      if(IS_FIGHTING(tmp_p))
      {
        send_to_char("It's not peaceful enough here!\r\n", ch);
        return TRUE;
      }
    }

    if(IS_TRUSTED(ch))
    {
      send_to_char("But you're already epic enough!\r\n", ch);
      return TRUE;
    }

    /* stones must be touched in the zone in which they were loaded */
    if(zone_number && world[ch->in_room].zone != real_zone(zone_number))
    {
      act("A sick noise emanates from $p, and a large crack runs down the side! Something was misplaced!",
           FALSE, ch, obj, 0, TO_CHAR);
      act("A sick noise emanates from $p, and a large crack runs down the side! Something was misplaced!",
          FALSE, ch, obj, 0, TO_ROOM);

      REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);
      return TRUE;
    }
    /*
    if(affected_by_spell(ch, TAG_EPIC_MONOLITH))
    {
      send_to_char("You need to wait before touching an epic stone again!\n", ch);
      return TRUE;
    } this mitigation is no longer necessary for wipe2011, as exp must be gained in zone before touch
      can reward player - Jexni */

    /* calculate epic value */
    int epic_value = epic_stone_payout(obj, ch);

    act("$n touches $p.", FALSE, ch, obj, ch, TO_NOTVICT);
    act("You touch $p.", FALSE, ch, obj, 0, TO_CHAR);

    act("$p begins to vibrate madly, shaking the entire room\n"
        "almost knocking you off your feet!\n"
        "Suddenly, a huge storm of &+Bblue energy&n erupts from it!", FALSE, ch, obj, 0, TO_ROOM);

    if(zone_number)
    {
      statuslog(GREATER_G, "%s touched the epic stone in %s", ch->player.name, zone_table[real_zone0(zone_number)].name);
      epiclog( 56, "%s touched the epic stone in %s.", J_NAME(ch), strip_ansi(zone_table[real_zone0(zone_number)].name).c_str());
      if (get_property("thanksgiving", 0.000))
        thanksgiving_proc(ch);
    }

    epic_stone_one_touch(obj, ch, epic_value);

    /* go through all members of group */
    int group_size = 1;

    if(ch->group)
    {
      for(struct group_list *gl = ch->group; gl; gl = gl->next)
      {
        if(gl->ch != ch && IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) && gl->ch->in_room == ch->in_room )
        {
          group_size++;
          epic_stone_one_touch(obj, gl->ch, epic_value);
        }
      }
    }

    if(zone_number > 0 && zone_number != RANDOM_ZONE_ID)
    {
      int delta = GET_RACEWAR(ch);
      delta = (delta == RACEWAR_EVIL) ? -1 : (delta == RACEWAR_GOOD ? 1 : 0);
      if( delta != 0 )
        update_epic_zone_alignment(zone_number, delta);

      // set completed flag
      epic_zone_completions.push_back(epic_zone_completion(zone_number, time(NULL), delta));
      db_query("UPDATE zones SET last_touch = '%d' WHERE number = '%d'", time(NULL), zone_number);
      db_query("INSERT INTO zone_touches (boot_time, touched_at, zone_number, toucher_pid, group_size, epic_value, alignment_delta) VALUES (%d, %d, %d, %d, %d, %d, %d);", boot_time, time(NULL), zone_number, GET_PID(ch), group_size, epic_value, delta);

      //  Allow !reset zones to possibly reset somewhere down the line...  - Jexni 11/7/11
      if(!zone_table[zone_number].reset_mode)
      {
        int x = real_zone(zone_number);
        add_event(event_reset_zone, 1, 0, 0, 0, 0, &x, sizeof(x));
        db_query("UPDATE zones SET reset_perc = 1 WHERE number = '%d'", zone_number);
      }
    }

    act("$p flashes brightly then blurs, and remains still and powerless.",
        FALSE, 0, obj, 0, TO_ROOM);
    REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);

    return TRUE;
  }

  return FALSE;
}


void epic_zone_balance()
{
  int i, alignment, delta, lt;
  vector<epic_zone_data> epic_zones = get_epic_zones();

  for (i = 0; i < epic_zones.size(); i++)
  {
    if(!qry("SELECT alignment, last_touch FROM zones WHERE number = %d", epic_zones[i].number))
      return;

    MYSQL_RES *res = mysql_store_result(DB);

    if(mysql_num_rows(res) < 1)
    {
      mysql_free_result(res);
      return;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if(row)
    {
      alignment = atoi(row[0]);
      lt = atoi(row[1]);
    }

    mysql_free_result(res);
    
    if(lt == 0)
      db_query("UPDATE zones SET last_touch='%d' WHERE number='%d'", time(NULL), epic_zones[i].number);

    if((alignment == 0))
      continue;
    
    //debug("zone %d alignment %d", epic_zones[i].number, alignment);

    if(time(NULL) - lt > ((int)get_property("epic.alignment.reset.hour", 7*24)*60*60))
    {
      if(alignment > 0)
        delta = -1;
      else if(alignment < 0)
        delta = 1;

      //debug("calling update_epic_zone_alignment");
      db_query("UPDATE zones SET last_touch='%d' WHERE number='%d'", time(NULL), epic_zones[i].number);
      update_epic_zone_alignment(epic_zones[i].number, delta);
      continue;
    }
  }
}

void epic_initialization()
{
  for (int i = 0; epic_teachers[i].vnum; i++) {
    mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;
  }
  errand_notch = get_property("epic.errandStep", 200);
}

int stat_shops(int room, P_char ch, int cmd, char *arg)
{
  char     buf[MAX_INPUT_LENGTH];
  int cost = 0;
  int cost_mod = 8;
  int MAX_SHOP_BUY = 95;

  /* check for periodic event calls */
  if(cmd == CMD_SET_PERIODIC)
    return FALSE;

  if(!ch)
    return (FALSE);

  if(cmd == CMD_LIST)
  {
     send_to_char("Available potions:\r\n\r\n", ch);

     cost = ch->base_stats.Str * ch->base_stats.Str * ch->base_stats.Str * cost_mod;
     if(ch->base_stats.Str < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "1. A &+Gmagical&n strength potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "1. A &+Gmagical&n strength potion is not available for you.\r\n");
     send_to_char(buf, ch); 

     cost = ch->base_stats.Agi * ch->base_stats.Agi * ch->base_stats.Agi * cost_mod;
     if(ch->base_stats.Agi < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "2. A &+Gmagical&n agility potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "2. A &+Gmagical&n agility potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Dex * ch->base_stats.Dex * ch->base_stats.Dex * cost_mod;
     if(ch->base_stats.Dex < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "3. A &+Gmagical&n dexterity potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "3. A &+Gmagical&n dexterity potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Con * ch->base_stats.Con * ch->base_stats.Con * cost_mod;
     if(ch->base_stats.Con < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "4. A &+Gmagical&n constitution potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "4. A &+Gmagical&n constitution potion is not available for you.\r\n");
     send_to_char(buf, ch); 

     cost = ch->base_stats.Luk * ch->base_stats.Luk * ch->base_stats.Luk * cost_mod;
     if(ch->base_stats.Luk < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "5. A &+Gmagical&n luck potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "5. A &+Gmagical&n luck stat potion is not available for you.\r\n");
     send_to_char(buf, ch);

      cost = ch->base_stats.Pow   *ch->base_stats.Pow   *ch->base_stats.Pow  * cost_mod;
     if(ch->base_stats.Pow < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "6. A &+Gmagical&n power potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "6. A &+Gmagical&n power stat potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Int * ch->base_stats.Int * ch->base_stats.Int * cost_mod;
     if(ch->base_stats.Int < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "7. A &+Gmagical&n intelligence potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "7. A &+Gmagical&n intelligence stat potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Wis * ch->base_stats.Wis * ch->base_stats.Wis * cost_mod;
     if(ch->base_stats.Wis < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "8. A &+Gmagical&n wisdom potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "8. A &+Gmagical&n wisdom stat potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Cha * ch->base_stats.Cha * ch->base_stats.Cha * cost_mod;
     if(ch->base_stats.Cha < MAX_SHOP_BUY)
       snprintf(buf, MAX_INPUT_LENGTH, "9. A &+Gmagical&n charisma potion for %s\r\n", coin_stringv(cost));
     else
       snprintf(buf, MAX_INPUT_LENGTH, "9. A &+Gmagical&n charisma stat potion  is not available for you.\r\n");
     send_to_char(buf, ch);

     return TRUE;
  }
  else if(cmd == CMD_BUY)
  {      
     arg = one_argument(arg, buf);
     if(!atoi(buf))
     {
        send_to_char("Exactly what are you trying to buy?\r\n", ch);
        return TRUE;
     }
     switch(atoi(buf))
     {
        case 1:
          cost = ch->base_stats.Str * ch->base_stats.Str * ch->base_stats.Str * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          } 
          if(ch->base_stats.Str >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }

          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_str(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 2:
          cost = ch->base_stats.Agi * ch->base_stats.Agi * ch->base_stats.Agi * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Agi >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_agi(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 3:
          cost = ch->base_stats.Dex * ch->base_stats.Dex * ch->base_stats.Dex * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Dex >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_dex(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 4:
          cost = ch->base_stats.Con * ch->base_stats.Con * ch->base_stats.Con * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Con >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_con(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 5:
          cost = ch->base_stats.Luk * ch->base_stats.Luk * ch->base_stats.Luk * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Luk >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_luck(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 6:
          cost = ch->base_stats.Pow * ch->base_stats.Pow * ch->base_stats.Pow * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Pow >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_pow(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 7:
          cost = ch->base_stats.Int * ch->base_stats.Int * ch->base_stats.Int * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Int >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_int(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 8:
          cost = ch->base_stats.Wis * ch->base_stats.Wis * ch->base_stats.Wis * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Wis >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_wis(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 9:
          cost = ch->base_stats.Cha * ch->base_stats.Cha * ch->base_stats.Cha * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Cha >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_cha(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        default:
          send_to_char("Exactly what are you trying to buy?\r\n", ch);
          return TRUE;
     }

  }
  return FALSE;
}


vector<string> get_epic_players(int racewar)
{
  vector<string> names;

#ifdef __NO_MYSQL__
  debug("get_epic_players(): __NO_MYSQL__, returning 0");
  return names;
#else
  if(!qry("SELECT name from players_core WHERE active=1 AND epics > 0 AND racewar = '%d' AND level < 57 ORDER BY epics DESC LIMIT %d", racewar, (int) get_property("epic.list.limit", 10)))
    return names;

  MYSQL_RES *res = mysql_store_result(DB);

  if(!res)
  {
    return names;
  }

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res)))
  {
    names.push_back(string(row[0]));
  }

  mysql_free_result(res);

  return names;
#endif
}

void do_epic(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];

  if(!ch || IS_NPC(ch))
    return;

  argument_interpreter(arg, buff2, buff3);
  if(!str_cmp("reset", buff2))
  {
    do_epic_reset(ch, arg, cmd);
    return;
  }
  
  if(!str_cmp("skills", buff2))
  {
    do_epic_skills(ch, arg, cmd);
    return;
  }
  if(!str_cmp("trophy", buff2))
  {
    do_epic_trophy(ch, arg, cmd);
    return;
  }

  if(!str_cmp("zones", buff2))
  {
    do_epic_zones(ch, arg, cmd);
    return;
  }

  if( !str_cmp("bonus", buff2) )
  {
    do_epic_bonus(ch, arg, cmd);
    return;
  }

  if( !str_cmp("share", buff2) )
  {
    do_epic_share(ch, arg, cmd);
    return;
  }

  // else show list of epic players
  vector<string> top_good_players = get_epic_players(RACEWAR_GOOD);
  vector<string> top_evil_players = get_epic_players(RACEWAR_EVIL);

  // list
  send_to_char("&+GEpic Players\n\n", ch);
  send_to_char(" &+WGoods\n\n", ch);

  for(int i = 0; i < top_good_players.size(); i++)
  {
    send_to_char("   ", ch);
    send_to_char(top_good_players[i].c_str(), ch);
    send_to_char("\n", ch);
  }

  send_to_char("\n\n &+LEvils\n\n", ch);

  for(int i = 0; i < top_evil_players.size(); i++)
  {
    send_to_char("   ", ch);
    send_to_char(top_evil_players[i].c_str(), ch);
    send_to_char("\n", ch);
  }
}

void epic_zone_erase_touch(int zone_number)
{
   for(vector<epic_zone_completion>::iterator it = epic_zone_completions.begin();
             it != epic_zone_completions.end();
             it++)
   {
      if((it->number == zone_number))
      {
         epic_zone_completions.erase(it);
         break;
      }
   }
}

bool epic_zone_done_now(int zone_number)
{
  int count = 1;

  // All this to set count to a value in zones.
  if( qry("SELECT stonecount FROM zones WHERE number = %d", zone_number) )
  {
    MYSQL_RES *res = mysql_store_result(DB);
    if( mysql_num_rows(res) >= 1 )
    {
      MYSQL_ROW row = mysql_fetch_row(res);
      if( row )
      {
        count = atoi(row[0]);
      }
    }
    mysql_free_result(res);
  }

  for( vector<epic_zone_completion>::iterator it = epic_zone_completions.begin();
    it != epic_zone_completions.end(); it++ )
  {
    // Relys on lazy evaluation.
    if( it->number == zone_number && --count <= 0 )
    {
      return TRUE;
    }
  }
  return FALSE;
}

bool epic_zone_done(int zone_number)
{
  for( vector<epic_zone_completion>::iterator it = epic_zone_completions.begin();
    it != epic_zone_completions.end(); it++ )
   {
      if((it->number == zone_number) &&
         (time(NULL) - it->done_at) > (int) get_property("epic.showCompleted.delaySecs", (15*60))) 
      {
         return TRUE;
      }
   }
   return FALSE;
}

int epic_zone_data::displayed_alignment() const 
{
   int delta = 0;

   for(vector<epic_zone_completion>::iterator it = epic_zone_completions.begin(); it != epic_zone_completions.end(); it++)
   {
      if( (it->number == this->number)
        && (time(NULL) - it->done_at) < (int) get_property("epic.showCompleted.delaySecs", (15*60)) )
      {
         return this->alignment - it->delta;
      }
   }
   return this->alignment;
}


//void do_epic_zones(P_char ch, char *arg, int cmd)
//{
//  return;
//  
//  if(!ch || IS_NPC(ch))
//    return;
//
//#ifdef __NO_MYSQL__
//  send_to_char("This feature is disabled.\r\n", ch);
//  return;
//#endif
//
//  char buff[MAX_STRING_LENGTH];
//	char done_str[] = " ";
//
//  vector<epic_zone_data> epic_zones = get_epic_zones();
//
//  send_to_char("&+WEpic Zones &+G-----------------------------------------\n\n", ch);
//
//  const char *freq_strs[] = {
//    "&+W(&+Roverdone&+W)",
//    "&+W(&+yvery common&+W)",
//    "&+W(&ncommon&+W)",
//    "",
//    "&+W(&+cuncommon&+W)",
//    "&+W(&+brare&+W)",
//    "&+W(&+BVERY RARE&+W)"
//  };
//
//  for(int i = 0; i < epic_zones.size(); i++)
//  {
//    float freq = epic_zones[i].freq;
//
//		if(epic_zone_done(epic_zones[i].number)) done_str[0] = '*';
//		else done_str[0] = ' ';
//
//    int freq_str = 3;
//
//    if(freq < 0.50)
//      freq_str = 0;
//    else if(freq < 0.50)
//      freq_str = 1;
//    else if(freq < 0.80)
//      freq_str = 2;
//    else if(freq > 1.90)
//      freq_str = 6;
//    else if(freq > 1.50)
//      freq_str = 5;
//    else if(freq > 1.20)
//      freq_str = 4;
//    else
//      freq_str = 3;
//
//    snprintf(buff, MAX_STRING_LENGTH, "  %s%s %s\r\n", done_str, epic_zones[i].name.c_str(), freq_strs[freq_str]);
//    send_to_char(buff, ch);
//  }
//
//	send_to_char("\n* = already completed this boot.\n", ch);
//
//}
/*
void do_epic_reset(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];
  
  argument_interpreter(arg, buff2, buff3);
  
  if(!ch || !IS_PC(ch))
    return;

  P_char t_ch = ch;

// Disabling for equipment wipe - re-enable at a later time - Drannak 8/9/2012
//  send_to_char("&+YEpic point resetting has been temporarily &+Rdisabled &+Ywhile we equipment wipe.&n\r\n", ch);
//  return;
// end disable
  
  if(IS_TRUSTED(ch) && strlen(buff3))
  {
    if(!(t_ch = get_char_vis(ch, buff3)) || !IS_PC(t_ch))
    {
      send_to_char("They don't appear to be in the game.\n", ch);
      return;
    }
  }
  
  // run through skills
  // for each skill that is epic:
  //    for each skill point:
  //      calculate epic pointcost / plat cost
  //      reimburse points / plat
  
  send_to_char("&+WResetting epic skills:\n", ch);
  
  int point_refund = 0;
  int coins_refund = 0;
  
  for (int skill_id = 0; skill_id <= MAX_AFFECT_TYPES; skill_id++)
  {
    int learned = t_ch->only.pc->skills[skill_id].learned;
    
    if((IS_EPIC_SKILL(skill_id) && learned) && (strcmp(skills[skill_id].name, "forge")) && (strcmp(skills[skill_id].name, "mine")) && (strcmp(skills[skill_id].name, "craft")))
     //  (skills[skill_id].name != ("forge" || "mine" || "craft")))
    {
      // find in epic_rewards
      int s;
      
      bool found = false;
      for(s = 0; epic_rewards[s].type; s++)
      {
        if(epic_rewards[s].value == skill_id)
        {
          found = true;
          break;
        }
      }
      
      if(!found)
      {
        continue;
      }
      
      int points = 0;
      int coins = 0;
      
      for(int skill_level = 0; skill_level < learned; skill_level += (int) get_property("epic.skillGain", 10))
      {
        float cost_f = 1 + skill_level / get_property("epic.progressFactor", 30);
        int points_cost = (int) (cost_f * epic_rewards[s].points_cost);
        int coins_cost = (int) (cost_f * epic_rewards[s].coins);
        
        if(IS_MULTICLASS_PC(t_ch) &&
           !IS_SET(epic_rewards[s].classes, t_ch->player.m_class) &&
           IS_SET(epic_rewards[s].classes, t_ch->player.secondary_class))
        {
          points_cost *= (int) (get_property("epic.multiclass.EpicSkillCost", 2));
          coins_cost *= (int) (get_property("epic.multiclass.EpicPlatCost", 3));
        }
        
        points += points_cost;
        coins += coins_cost;
      }      
      
      snprintf(buff2, MAX_STRING_LENGTH, "&+W%s %d&n: &+W%d&n esp, %s\n", skills[skill_id].name, learned, points, coin_stringv(coins));
      send_to_char(buff2, ch);
      
      point_refund += points;
      coins_refund += coins;
      
      t_ch->only.pc->skills[skill_id].learned = t_ch->only.pc->skills[skill_id].taught = 0;
    }
  }
  
  snprintf(buff2, MAX_STRING_LENGTH, "Total: &+W%d&n esp, %s&n refunded\r\n", point_refund, coin_stringv(coins_refund));
  send_to_char(buff2, ch);
  
  insert_money_pickup(GET_PID(t_ch), coins_refund);
  t_ch->only.pc->epic_skill_points += point_refund;

  snprintf(buff2, MAX_STRING_LENGTH, "\r\n&+GYour epic skills have been reset: your skill points have been refunded, \r\n&+Gand %s&+G has been reimbursed and is waiting for you at the nearest auction house.\r\n\r\n", coin_stringv(coins_refund));
  
  if(!send_to_pid(buff2, GET_PID(t_ch)))
    send_to_pid_offline(buff2, GET_PID(t_ch));
  
  do_save_silent(t_ch, 1);  
}*/

void do_epic_zones(P_char ch, char *arg, int cmd)
{
  char buff[MAX_STRING_LENGTH];
	char done_char;
  int  zone_align;

  if( !ch || IS_NPC(ch) )
    return;

#ifdef __NO_MYSQL__
  send_to_char("This feature is disabled.\r\n", ch);
  return;
#endif

  vector<epic_zone_data> epic_zones = get_epic_zones();

  send_to_char("&+WEpic Zones &+G-----------------------------------------\n\n", ch);

  // this array depends on the alignment max/min being +/-5
  const char *alignment_strs[] = {
    "&n(&+Lpure evil&n)     ",
    "&n(&+Lextremely evil&n)",
    "&n(&+Lvery evil&n)     ",
    "&n(&+Levil&n)          ",
    "&n(&+Lslightly evil&n) ",
    "&N(&+wneutral&N)       ",
    "&n(&+Wslightly good&n) ",
    "&n(&+Wgood&n)          ",
    "&n(&+Wvery good&n)     ",
    "&n(&+Wextremely good&n)",
    "&n(&+Wpure good&n)     "
  };

  for(int i = 0; i < epic_zones.size(); i++)
  {
		if( epic_zone_done(epic_zones[i].number) )
      done_char = '*';
		else
      done_char = ' ';

    zone_align = BOUNDED(0, EPIC_ZONE_ALIGNMENT_MAX + epic_zones[i].displayed_alignment(), 10);

    snprintf(buff, MAX_STRING_LENGTH, "  %c%s %s\r\n", done_char, pad_ansi(epic_zones[i].name.c_str(), 45).c_str(),
      alignment_strs[zone_align] );

    send_to_char(buff, ch);
  }

	send_to_char("\n* = already completed this boot.\n", ch);  
}

void do_epic_share(P_char ch, char *arg, int cmd)
{

  struct affected_type *afp, *tafp;

  if( !has_epic_task(ch) )
  {
    send_to_char("You don't have an epic task to share.\r\n", ch);
    return;
  }
  else
  {
    afp = get_epic_task(ch);
  }
  if( afp->modifier < 0 )
  {
    send_to_char( "Only the &+Woriginal&n task owner can share tasks.\n", ch );
    return;
  }

  if( ch->group )
  {
    for( struct group_list *gl = ch->group; gl; gl = gl->next )
    {
      if( gl->ch == ch )
        continue;
      if( gl->ch->in_room == ch->in_room )
      {
        if( is_linked_to(ch, gl->ch, LNK_CONSENT) && IS_PC(gl->ch) )
        {
          if( has_epic_task(gl->ch) )
          {
            tafp = get_epic_task(gl->ch);
            // Don't let nexus stones or pvp get replaced
            if( abs(tafp->modifier) >= SPILL_BLOOD )
              continue;
            tafp->type = afp->type;
            tafp->flags = afp->flags;
            tafp->duration = afp->duration;
            // The - sign here is intentional; it prevents people from sharing tasks that have been shared,
            //   which, in turn, should stop storing tasks for the most part.
            tafp->modifier = - afp->modifier;
            act("&+C$n has just shared $s epic task with you!&n", TRUE, ch, 0, gl->ch, TO_VICT);
            act("&+CYou have just shared your epic task with $N.&n", TRUE, ch, 0, gl->ch, TO_CHAR);
          }
        }
      }
    }
  }

}

void do_epic_trophy(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];

  argument_interpreter(arg, buff2, buff3);

  if(!ch || IS_NPC(ch))
    return;

  P_char t_ch = ch;

  if(IS_TRUSTED(ch) && strlen(buff3))
  {
    if(!(t_ch = get_char_vis(ch, buff3)))
    {
      send_to_char("They don't appear to be in the game.\n", ch);
      return;
    }
  }

  vector<epic_trophy_data> trophy = get_epic_zone_trophy(t_ch);

  send_to_char("&+WEpic Trophy\n", ch);

  for(int i = 0; i < trophy.size(); i++)
  {
    if(trophy[i].zone_number >= 0 && real_zone0(trophy[i].zone_number))
    {
      snprintf(buff2, MAX_STRING_LENGTH, "[&+W%3d&n] %s\n", trophy[i].count, zone_table[real_zone0(trophy[i].zone_number)].name);
      send_to_char(buff2, ch);
    }

  }

}

void update_epic_zone_alignment(int zone_number, int delta)
{
#ifdef __NO_MYSQL__
  return;
#else
  // add alignment
  qry("UPDATE zones SET alignment = alignment + (%d) WHERE number = %d AND epic_type > 0", delta, zone_number);

  // if alignment delta resulted in 0, add one more so that it doesn't stay on 0
/* This is ruining the epic_zone_balance function causing it to go from good to evil instead of neutral.
  qry("UPDATE zones SET alignment = alignment + (%d) WHERE number = %d AND epic_type > 0 and alignment = 0", delta, zone_number);
 */
  // min/max bounds on alignment
  qry("UPDATE zones SET alignment = %d WHERE alignment > %d", EPIC_ZONE_ALIGNMENT_MAX, EPIC_ZONE_ALIGNMENT_MAX);
  qry("UPDATE zones SET alignment = %d WHERE alignment < %d", EPIC_ZONE_ALIGNMENT_MIN, EPIC_ZONE_ALIGNMENT_MIN);
  
  //debug("update_epic_zone_alignment(zone_number=%d, delta=%d)", zone_number, delta);
#endif  
}

// Should return a number 0.0 or greater. (0.0: no epics, 1.0: full epics, 2.0: double epics, etc).
float get_epic_zone_alignment_mod(int zone_number, ubyte racewar)
{
#ifdef __NO_MYSQL__
  return 1.0;
#else

  float mod = 1.0, minPercentage;
  int alignment = 0;

  if(!qry("SELECT alignment FROM zones WHERE number = %d", zone_number))
    return mod;

  MYSQL_RES *res = mysql_store_result(DB);

  if(mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return 1.0;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  if( row )
  {
    alignment = atoi(row[0]);
  }

  mysql_free_result(res);

  if((alignment < 0 && racewar == RACEWAR_GOOD) || (alignment > 0 && racewar == RACEWAR_EVIL))
  {
    // good alignment, evil racewar or evil alignment, good racewar
    mod += ((float) abs(alignment)) * 0.3 * (float) get_property("epic.zone.alignmentMod", 0.10);
  }
  else if((alignment > 0 && racewar == RACEWAR_GOOD) || (alignment < 0 && racewar == RACEWAR_EVIL))
  {
    // good alignment, good racewar or evil alignment, evil racewar
    mod -= ((float) abs(alignment)) * (float) get_property("epic.zone.alignmentMod", 0.10);
  }
  // Undead / Illithids don't get a modifier atm.
  else
  {
    return 1;
  }

  minPercentage = get_property("epic.alignment.minPercentage", 0.10);
  // If the minimum percentage is more than the current modifier, up the current mod to the minimum.
  if( minPercentage > mod )
    mod = minPercentage;

  debug("get_epic_zone_alignment_mod(zone_number=%d, racewar=%d): %f", zone_number, (int) racewar, mod);

  return mod;

#endif
}

// called from timers.c
void update_epic_zone_mods()
{
#ifdef __NO_MYSQL__
  return;
#else
  int wait_secs = (int) get_property("epic.freqMod.tick.waitSecs", 3600);

  if(!has_elapsed("epic_zone_mod", wait_secs))
    return;

  float add = (float) get_property("epic.freqMod.tick.add", 0.002);
  float mod_max = (float) get_property("epic.freqMod.max", 2.00);
  float mod_min = (float) get_property("epic.freqMod.min", 0.40);

  qry("UPDATE zones SET frequency_mod = frequency_mod + (%f) WHERE epic_type > 0", add);
  qry("UPDATE zones SET frequency_mod = %f WHERE frequency_mod > %f", mod_max, mod_max);
  qry("UPDATE zones SET frequency_mod = %f WHERE frequency_mod < %f", mod_min, mod_min);

  set_timer("epic_zone_mod");
#endif
}

void update_epic_zone_frequency(int zone_number)
{
#ifdef __NO_MYSQL__
  return;
#else
  float sub = (float) get_property("epic.freqMod.touch.sub", 0.10);
  float mod_min = (float) get_property("epic.freqMod.min", 0.40);

  qry("UPDATE zones SET frequency_mod = frequency_mod - (%f * zone_freq_mod) WHERE number = %d AND epic_type > 0", sub, zone_number);
  qry("UPDATE zones SET frequency_mod = %f WHERE frequency_mod < %f", mod_min, mod_min);

  debug("update_epic_zone_frequency(zone_number=%d): -%f", zone_number, sub);
#endif
}

float get_epic_zone_frequency_mod(int zone_number)
{
#ifdef __NO_MYSQL__
  return 1.0;
#else

  float mod = 1.0;

  if(!qry("SELECT frequency_mod FROM zones WHERE number = %d", zone_number))
    return mod;

  MYSQL_RES *res = mysql_store_result(DB);

  if(mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return mod;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  if(row)
    mod = atof(row[0]);

  mysql_free_result(res);

  return mod;
#endif
}

vector<epic_zone_data> get_epic_zones()
{
  vector<epic_zone_data> zones;

#ifdef __NO_MYSQL__
  return zones;
#else

  if(!qry("SELECT number, name, frequency_mod, alignment FROM zones WHERE epic_type > 0 ORDER BY (suggested_group_size*epic_payout), id"))
  {
    return zones;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  MYSQL_ROW row;

  while ((row = mysql_fetch_row(res)))
  {
    zones.push_back(epic_zone_data(atoi(row[0]), string(row[1]), atof(row[2]), atoi(row[3])));
  }

  mysql_free_result(res);

  return zones;
#endif
}

//referenced in actwiz.c for existing chars - Drannak
void do_epic_reset_norefund(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];
  
  argument_interpreter(arg, buff2, buff3);
  
  if(!ch || !IS_PC(ch))
    return;
  
  P_char t_ch = ch;

// Disabling for equipment wipe - re-enable at a later time - Drannak 8/9/2012
//  send_to_char("&+YEpic point resetting has been temporarily &+Rdisabled &+Ywhile we equipment wipe.&n\r\n", ch);
//  return;
// end disable
  
  if(IS_TRUSTED(ch) && strlen(buff3))
  {
    if(!(t_ch = get_char_vis(ch, buff3)) || !IS_PC(t_ch))
    {
      send_to_char("They don't appear to be in the game.\n", ch);
      return;
    }
  }
  
  // run through skills
  // for each skill that is epic:
  //    for each skill point:
  //      calculate epic pointcost / plat cost
  //      reimburse points / plat
  
 /* send_to_char("&+WResetting epic skills:\n", ch); */
  
  int point_refund = 0;
  int coins_refund = 0;
  
  for (int skill_id = 0; skill_id <= MAX_AFFECT_TYPES; skill_id++)
  {
    int learned = t_ch->only.pc->skills[skill_id].learned;
    
    if((IS_EPIC_SKILL(skill_id) && learned) && (strcmp(skills[skill_id].name, "forge")) && (strcmp(skills[skill_id].name, "mine")) && (strcmp(skills[skill_id].name, "craft")))
     //  (skills[skill_id].name != ("forge" || "mine" || "craft")))
    {
      // find in epic_rewards
      int s;
      
      bool found = false;
      for(s = 0; epic_rewards[s].type; s++)
      {
        if(epic_rewards[s].value == skill_id)
        {
          found = true;
          break;
        }
      }
      
      if(!found)
      {
        continue;
      }
      
      int points = 0;
      int coins = 0;
      
      for(int skill_level = 0; skill_level < learned; skill_level += (int) get_property("epic.skillGain", 10))
      {
        float cost_f = 1 + skill_level / get_property("epic.progressFactor", 30);
        int points_cost = (int) (cost_f * epic_rewards[s].points_cost);
        int coins_cost = (int) (cost_f * epic_rewards[s].coins);
        
        if(IS_MULTICLASS_PC(t_ch) &&
           !IS_SET(epic_rewards[s].classes, t_ch->player.m_class) &&
           IS_SET(epic_rewards[s].classes, t_ch->player.secondary_class))
        {
          points_cost *= (int) (get_property("epic.multiclass.EpicSkillCost", 2));
          coins_cost *= (int) (get_property("epic.multiclass.EpicPlatCost", 3));
        }
        
        points += points_cost;
        coins += coins_cost;
      }      
      
    
      point_refund += points;
      coins_refund += coins;
      
      t_ch->only.pc->skills[skill_id].learned = t_ch->only.pc->skills[skill_id].taught = 0;
    }
  }
/*  
  snprintf(buff2, MAX_STRING_LENGTH, "Total: &+W%d&n esp, %s&n refunded\r\n", point_refund, coin_stringv(coins_refund));
  send_to_char(buff2, ch);
  
  insert_money_pickup(GET_PID(t_ch), coins_refund);
  t_ch->only.pc->epic_skill_points += point_refund;

  snprintf(buff2, MAX_STRING_LENGTH, "\r\n&+GYour epic skills have been reset: your skill points have been refunded, \r\n&+Gand %s&+G has been reimbursed and is waiting for you at the nearest auction house.\r\n\r\n", coin_stringv(coins_refund));
*/
  if(!send_to_pid(buff2, GET_PID(t_ch)))
    send_to_pid_offline(buff2, GET_PID(t_ch));
  
  do_save_silent(t_ch, 1);  
}

void do_epic_reset(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];
  
  argument_interpreter(arg, buff2, buff3);
  
  if(!ch || !IS_PC(ch))
    return;

  if( !IS_TRUSTED(ch) )
  {
    send_to_char("&+CEpic resetting is unavailable - choose your epic skills wisely\r\n.", ch);
    return;
  }

  P_char t_ch = ch;

// Disabling for equipment wipe - re-enable at a later time - Drannak 8/9/2012
//  send_to_char("&+YEpic point resetting has been temporarily &+Rdisabled &+Ywhile we equipment wipe.&n\r\n", ch);
//  return;
// end disable
  
  if(IS_TRUSTED(ch) && strlen(buff3))
  {
    if(!(t_ch = get_char_vis(ch, buff3)) || !IS_PC(t_ch))
    {
      send_to_char("They don't appear to be in the game.\n", ch);
      return;
    }
  }
  
  // run through skills
  // for each skill that is epic:
  //    for each skill point:
  //      calculate epic pointcost / plat cost
  //      reimburse points / plat
  
 /* send_to_char("&+WResetting epic skills:\n", ch); */
  
  int point_refund = 0;
  int coins_refund = 0;
  
  for (int skill_id = 0; skill_id <= MAX_AFFECT_TYPES; skill_id++)
  {
    int learned = t_ch->only.pc->skills[skill_id].learned;
    
    if((IS_EPIC_SKILL(skill_id) && learned) && (strcmp(skills[skill_id].name, "forge")) && (strcmp(skills[skill_id].name, "mine")) && (strcmp(skills[skill_id].name, "craft")))
     //  (skills[skill_id].name != ("forge" || "mine" || "craft")))
    {
      // find in epic_rewards
      int s;
      
      bool found = false;
      for(s = 0; epic_rewards[s].type; s++)
      {
        if(epic_rewards[s].value == skill_id)
        {
          found = true;
          break;
        }
      }
      
      if(!found)
      {
        continue;
      }
      
      int points = 0;
      int coins = 0;
      
      for(int skill_level = 0; skill_level < learned; skill_level += (int) get_property("epic.skillGain", 10))
      {
        float cost_f = 1 + skill_level / get_property("epic.progressFactor", 30);
        int points_cost = (int) (cost_f * epic_rewards[s].points_cost);
        int coins_cost = (int) (cost_f * epic_rewards[s].coins);
        
        if(IS_MULTICLASS_PC(t_ch) &&
           !IS_SET(epic_rewards[s].classes, t_ch->player.m_class) &&
           IS_SET(epic_rewards[s].classes, t_ch->player.secondary_class))
        {
          points_cost *= (int) (get_property("epic.multiclass.EpicSkillCost", 2));
          coins_cost *= (int) (get_property("epic.multiclass.EpicPlatCost", 3));
        }
        
        points += points_cost;
        coins += coins_cost;
      }      
      
    
      point_refund += points;
      coins_refund += coins;
      
      t_ch->only.pc->skills[skill_id].learned = t_ch->only.pc->skills[skill_id].taught = 0;
    }
  }
/*  
  snprintf(buff2, MAX_STRING_LENGTH, "Total: &+W%d&n esp, %s&n refunded\r\n", point_refund, coin_stringv(coins_refund));
  send_to_char(buff2, ch);
  
  insert_money_pickup(GET_PID(t_ch), coins_refund);
  t_ch->only.pc->epic_skill_points += point_refund;

  snprintf(buff2, MAX_STRING_LENGTH, "\r\n&+GYour epic skills have been reset: your skill points have been refunded, \r\n&+Gand %s&+G has been reimbursed and is waiting for you at the nearest auction house.\r\n\r\n", coin_stringv(coins_refund));
*/
  if(!send_to_pid(buff2, GET_PID(t_ch)))
    send_to_pid_offline(buff2, GET_PID(t_ch));
  
  do_save_silent(t_ch, 1);  
}

// This is here to clear out the racial skills set along with the tag TAG_RACIAL_SKILLS
// After the next wipe (as of 4/25/14) this is deprecated and should no longer be needed
void clear_racial_skills(P_char ch)
{
  if(!affected_by_spell(ch, TAG_RACIAL_SKILLS))
  {
    return;
  }

  if(GET_SPEC(ch, CLASS_SORCERER, SPEC_WIZARD))
  {
    ch->only.pc->skills[SKILL_SPELL_PENETRATION].taught = ch->only.pc->skills[SKILL_SPELL_PENETRATION].learned = 0;
  }

  switch (GET_RACE(ch))
  {
    case RACE_GNOME:
      ch->only.pc->skills[SKILL_FIX].taught = ch->only.pc->skills[SKILL_FIX].learned = 0;
      break;
    case RACE_HALFLING:
      ch->only.pc->skills[SKILL_EXPERT_PARRY].taught = ch->only.pc->skills[SKILL_EXPERT_PARRY].learned = 0;
      break;
    case RACE_GOBLIN:
      ch->only.pc->skills[SKILL_EXPERT_PARRY].taught = ch->only.pc->skills[SKILL_EXPERT_PARRY].learned = 0;
      ch->only.pc->skills[SKILL_FIX].taught = ch->only.pc->skills[SKILL_FIX].learned = 0;
      break;
    case RACE_GITHYANKI:
      ch->only.pc->skills[SKILL_ADVANCED_MEDITATION].taught = ch->only.pc->skills[SKILL_ADVANCED_MEDITATION].learned = 0;
      break;
    case RACE_HUMAN:
      ch->only.pc->skills[SKILL_SHIELD_COMBAT].taught = ch->only.pc->skills[SKILL_SHIELD_COMBAT].learned = 0;
      ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].taught = ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].learned = 0;
      ch->only.pc->skills[SKILL_SCRIBE_MASTERY].taught = ch->only.pc->skills[SKILL_SCRIBE_MASTERY].learned = 0;
      ch->only.pc->skills[SKILL_DEVOTION].taught = ch->only.pc->skills[SKILL_DEVOTION].learned = 0;
      ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = 0;
      break;
    case RACE_ORC:
      ch->only.pc->skills[SKILL_SHIELD_COMBAT].taught = ch->only.pc->skills[SKILL_SHIELD_COMBAT].learned = 0;
      ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].taught = ch->only.pc->skills[SKILL_IMPROVED_SHIELD_COMBAT].learned = 0;
      ch->only.pc->skills[SKILL_SCRIBE_MASTERY].taught = ch->only.pc->skills[SKILL_SCRIBE_MASTERY].learned = 0;
      ch->only.pc->skills[SKILL_DEVOTION].taught = ch->only.pc->skills[SKILL_DEVOTION].learned = 0;
      ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = 0;
      break;
    case RACE_CENTAUR:
      ch->only.pc->skills[SKILL_EXPERT_RIPOSTE].taught = ch->only.pc->skills[SKILL_EXPERT_RIPOSTE].learned = 0;
      ch->only.pc->skills[SKILL_TWOWEAPON].taught = ch->only.pc->skills[SKILL_TWOWEAPON].learned = 0;
      ch->only.pc->skills[SKILL_IMPROVED_TWOWEAPON].taught = ch->only.pc->skills[SKILL_IMPROVED_TWOWEAPON].learned = 0;
      break;
    case RACE_BARBARIAN:
      ch->only.pc->skills[SKILL_ANATOMY].taught = ch->only.pc->skills[SKILL_ANATOMY].learned = 0;
      break;
    case RACE_TROLL:
      ch->only.pc->skills[SKILL_ANATOMY].taught = ch->only.pc->skills[SKILL_ANATOMY].learned = 0;
      ch->only.pc->skills[SKILL_TOTEMIC_MASTERY].taught = ch->only.pc->skills[SKILL_TOTEMIC_MASTERY].learned = 0;
      break;
    case RACE_OGRE:
      ch->only.pc->skills[SKILL_DEVASTATING_CRITICAL].taught = ch->only.pc->skills[SKILL_DEVASTATING_CRITICAL].learned = 0;
      break;
    case RACE_FIRBOLG:
      ch->only.pc->skills[SKILL_NATURES_SANCTITY].taught = ch->only.pc->skills[SKILL_NATURES_SANCTITY].learned = 0;
      break;
    case RACE_THRIKREEN:
      ch->only.pc->skills[SKILL_SHIELDLESS_BASH].taught = ch->only.pc->skills[SKILL_SHIELDLESS_BASH].learned = 0;
      ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].taught = ch->only.pc->skills[SKILL_IMPROVED_ENDURANCE].learned = 0;
      break;
  }

  affect_from_char(ch, TAG_RACIAL_SKILLS);

  do_save_silent(ch, 1); // racial skills require a save.
}

void refund_epic_skills(P_char ch)
{
  int skl, er_skl, learned, cost_multiplier, multiplier_step;
  int point_refund = 0;
  int coins_refund = 0;

  point_refund = coins_refund = 0;
  multiplier_step = get_property("epic.progressFactor", 30) / 10;

  for( skl = FIRST_SKILL; skl <= LAST_SKILL; skl++ )
  {
    if( !IS_EPIC_SKILL(skl) )
    {
      continue;
    }
    if( isname(skills[skl].name, "forge mine craft") )
    {
      continue;
    }
    if( (learned = (GET_CHAR_SKILL(ch, skl) / 10)) == 0 )
    {
      continue;
    }
    for( er_skl = 0; epic_rewards[er_skl].type; er_skl++ )
    {
      if( epic_rewards[er_skl].value == skl )
      {
        break;
      }
    }
    if( !epic_rewards[er_skl].type )
    {
      continue;
    }
    while( learned-- > 0 )
    {
      cost_multiplier = 1 + learned / multiplier_step;
      point_refund += cost_multiplier * epic_rewards[er_skl].points_cost * 3;
      coins_refund += cost_multiplier * epic_rewards[er_skl].coins * 2;
    }
    ch->only.pc->skills[skl].learned = ch->only.pc->skills[skl].taught = 0;
  }
  insert_money_pickup(GET_PID(ch), coins_refund);
  ch->only.pc->epics += point_refund;
  debug( "%s getting epic-skill refund of %d epics and %s.", GET_NAME(ch), point_refund, coin_stringv(coins_refund) );
  logit( LOG_EPIC, "%s getting epic-skill refund of %d epics and %s coins.", GET_NAME(ch), point_refund,
    comma_string(coins_refund) );
  send_to_char_f( ch, "&+WYour epic skills have been reset.&n\n&+WYou are refunded %d epics.&N\n&+WYou are refunded %s.&n\n",
    point_refund, coins_to_string(coins_refund/1000, (coins_refund/100)%10, (coins_refund/10)%10, coins_refund%10, "&+W") );
}
