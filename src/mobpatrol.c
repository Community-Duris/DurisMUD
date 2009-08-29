#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "graph.h"
#include "mm.h"
#include "range.h"
#include "objmisc.h"
#include "vnum.obj.h"
#include "mobpatrol.h"

extern Skill skills[];
extern P_desc descriptor_list;
extern P_event current_event;
extern P_event event_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *dirs[];
extern const struct stat_data stat_factor[];
extern double lfactor[];
extern float fake_sqrt_table[];
extern int MobSpellIndex[MAX_SKILLS];
extern int equipment_pos_table[CUR_MAX_WEAR][3];
extern int no_specials;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int top_of_world;
extern const int rev_dir[NUMB_EXITS];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern const char *undead_type[];
extern int is_in_own_guild(P_char member);
extern struct potion potion_data[];

struct PatrolData
{
  enum {
    PATROL_SENTINEL = 0,
    PATROL_LEFT,
    PATROL_RIGHT,
  } dirPref;
  
  enum {
    PATROL_NOTHING = 0,
    PATROL_HUNT,
    PATROL_FIGHTING,
  } curActivity;
  // this is how fast the rr moves for normal walking.
  // this is randomly generated at creation, and keeps
  // things interesting as well as ensuring that two
  // rangers will rarely ever walk in step with each
  // other
  int normalSpeed;
  
  
  // hunt targets are in order of priority.  It's possible that
  // both of these get set, in which case the mob will hunt to
  // huntCh, and when done (if nothing else to do), will hunt to
  // huntRoom.
  P_char huntCh;     // if set, the highest priority hunt
  int    huntRoom;   // lower priority..
};


#define VALID_PATROL_ROOM(x) (((x) != NOWHERE) && \
                              (world[x].sector_type == SECT_ROAD))



// for a new patrol/RR, this sets up everything needed...
void mobPatrol_SetupNew(P_char ch)
{
  // mobconvert likes to add MEMORY to mobs.  Remove it
  REMOVE_BIT(ch->specials.act, ACT_MEMORY);
  REMOVE_BIT(ch->specials.act, ACT_HUNTER);
  // all road rangers have fly.
  SET_BIT(ch->specials.affected_by, AFF_FLY);

  // 33% chance that we get detect illusion
  if (!number(0,2))
    SET_BIT(ch->specials.affected_by4, AFF4_DETECT_ILLUSION);
  else
    REMOVE_BIT(ch->specials.affected_by4, AFF4_DETECT_ILLUSION);

  if (GET_MAX_HIT(ch) < 1000)
    GET_MAX_HIT(ch) = GET_HIT(ch) = ch->points.base_hit = 1000;    
  
  // these guys are NOT worth exp or money
  CLEAR_MONEY(ch);
  GET_EXP(ch) = 0;

  // proper patrols aren't followers.  
  if (ch->following)
      stop_follower(ch);
  
  PatrolData newData;
  // this is the inital call.  setup some stuff...
  memset(&newData, 0, sizeof(PatrolData));
  newData.curActivity = PatrolData::PATROL_NOTHING;
  newData.huntCh = NULL;
  newData.huntRoom = NOWHERE;

  if (IS_SET(ch->specials.act, ACT_SENTINEL))
  {
    newData.dirPref = PatrolData::PATROL_SENTINEL;
  }
  else
  {
    if (!number(0,1))
      newData.dirPref = PatrolData::PATROL_LEFT;
    else
      newData.dirPref = PatrolData::PATROL_RIGHT;
  }
  // determine their normal walking speed between 1 and 3 secs
  newData.normalSpeed = number(WAIT_SEC, WAIT_SEC*3);
  // the first event is always longer than any other.  This gives
  // the standard mobAct time to put some spells up
  add_event(event_patrol_move, PULSE_MOBILE * number(1,4) + number(0,10) , ch, NULL, NULL, 0, 
            &newData, sizeof(PatrolData));
  return;
}


// standard "movement" event for PATROL mobs.  *data is a hunt_data
// structure or null.  If null, just do standard movement.  If !null,
// then we're hunting *something* or *somewhere* and that should be
// dealt with
void event_patrol_move(P_char ch, P_char vict, P_obj obj, void *data)
{
  char buf[MAX_STRING_LENGTH];
  
  PatrolData *huntData = (PatrolData *)data;
    
  // standard mob can act checks..
  if(!IS_NPC(ch) || 
     IS_IMMOBILE(ch))
  { // reset the event and return
    add_event(event_patrol_move, PULSE_VIOLENCE , ch, vict, obj, 0, huntData,
              huntData ? sizeof(struct hunt_data) : 0);
    return;
  }
  if (NULL == huntData)
  {
    mobPatrol_SetupNew(ch);
    return;
  }
  if (!ALONE(ch))
  {
    P_char t_ch = PickTarget(ch);
    if (t_ch && is_aggr_to(ch, t_ch))
      MobStartFight(ch, t_ch);
  }

  if (IS_FIGHTING(ch))
  {
    if (huntData->curActivity != PatrolData::PATROL_FIGHTING)
    {
      do_yell(ch, "Guards!  Come destroy this trash which threatens our road!", CMD_SHOUT);
      huntData->curActivity = PatrolData::PATROL_FIGHTING;
    }
  }
  else if (huntData->curActivity == PatrolData::PATROL_FIGHTING)
  {
    huntData->curActivity = PatrolData::PATROL_NOTHING;
  }
  if (huntData->curActivity == PatrolData::PATROL_NOTHING)  // standard movement, spell casting, etc.
  {
    if (huntData->dirPref != PatrolData::PATROL_SENTINEL)
    {
      int dir = -1;
      
      // scan for someone we're aggro to...
      // Disabled Kvark
        //dir = range_scan(ch, NULL, number(0, 3), SCAN_EVILRACE);
      
      if (-1 != dir)
      {
        // do_shout(ch, "Rangers!  Evils threaten our road!", CMD_SHOUT);
      }
      else if (!VALID_PATROL_ROOM(ch->in_room))
      {  // if not in a valid patrolling room - find one!
        int dummy = 0;
        dir = find_first_step(ch->in_room, 0, 
                              BFS_CAN_FLY | BFS_BREAK_WALLS | BFS_ROADRANGER,
                              0,0, &dummy);
        if (dir < 0)
        {
          mobsay(ch, "Well, I'm seriously screwed with no way to get back to the road.");
	  die(ch, ch);
          return;
        }                            
      }
      else
      {
        // find a direction to move.  just loop the possible directions in order
        // starting from the last moved+1, and move to the first one thats
        // SECT_CITY and in the same zome.
        int fromdir = rev_dir[ch->only.npc->last_direction];
        dir = fromdir;
        do
        {
          dir += (huntData->dirPref == PatrolData::PATROL_LEFT ? -1 : 1);
          if (dir >= NUMB_EXITS)
            dir = 0;
          else if (dir < 0)
            dir = NUMB_EXITS -1;
            
          if (world[ch->in_room].dir_option[dir] &&
              VALID_PATROL_ROOM(TOROOM(ch->in_room, dir)))
    			    break; 
        }    
        while (dir != fromdir);
      }
      
      if (IS_CLOSED(ch->in_room, dir))
      {
        sprintf(buf, "%s %s", EXIT(ch, (int) dir)->keyword ?
                FirstWord(EXIT(ch, (int) dir)->keyword) : "door",
                dirs[(int) dir]);
        do_open(ch, buf, 0);
      }
      else if (IS_WALLED(ch->in_room, dir))
      {
        MobDestroyWall(ch, dir, true);
      }
      else
      {
        do_move(ch, NULL, exitnumb_to_cmd(dir));
        ch->only.npc->last_direction = dir;
      }
    }
  }
  add_event(event_patrol_move, huntData->normalSpeed, ch, vict, obj, 0, 
            huntData, sizeof(struct hunt_data));
}

