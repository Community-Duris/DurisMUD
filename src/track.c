/*
   ***************************************************************************
   *  File: track.c                                            Part of Duris *
   *  Usage: handle one char tracking another.                                 *
   *  Copyright  1991 - Andrew Choi                                            *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   *************************************************************************** 
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "graph.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "map.h"

extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const char *dirs[];
extern const char *short_dirs[];
extern const struct race_names race_names_table[];
extern const int track_limit[];
extern const int top_of_world;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;

struct trackrecordtype *first_track, *last_track, *spare_track_base;

void     set_short_description(P_obj t_obj, const char *newShort);
void     set_long_description(P_obj t_obj, const char *newDescription);

#define MAX_ROOMS 100
#define NOTRACKoff

void event_track_move(P_char ch, P_char vict, P_obj obj, void *data)
{
  int      dir, dist;
  char     buf[MAX_STRING_LENGTH];

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("Something comes up, and you abandon the hunt.\r\n", ch);
    REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING);
    return;
  }

  if( !IS_SET(ch->specials.affected_by3, AFF3_TRACKING) )
  {
    send_to_char("Something breaks your concentration, and you abandon the hunt.\r\n", ch);
    return;
  }

  dir = BFS_ERROR;

//debug( "Victim: %s, specs.tracking: %d", vict ? J_NAME(vict) : "NULL", ch->specials.tracking );
  dir = find_first_step(ch->in_room,(vict ? vict->in_room : ch->specials.tracking),
    (IS_SET(ch->specials.affected_by, AFF_FLY) ? BFS_CAN_FLY : 0) | BFS_CAN_DISPEL | (IS_NPC(ch) ? BFS_AVOID_NOMOB : 0),
    0, 0, &dist);

  if( dir == BFS_ERROR || dir == BFS_NO_PATH )
  {
    send_to_char("You are unable to find any trace.\r\n", ch);
    REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING);

    logit(LOG_DEBUG, "BFS_ERROR or BFS_NO_PATH error in event_track_move() track.c with %s.", GET_NAME(ch));
    return;
  }
  else if( dir == BFS_ALREADY_THERE )
  {
    send_to_char("The trail appears to end here.\r\n", ch);
    REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING);
    return;
  }
  else if( dist > MaxTrackDist(ch) )
  {
    send_to_char("The trail is too old to follow with any certainty.\r\n", ch);
    REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING);
    return;
  }
  else
  {
    /* still following the trail */
    snprintf(buf, MAX_STRING_LENGTH, "You find traces of tracks leading %s.\r\n", dirs[dir]);
    send_to_char(buf, ch);
    if (EXIT(ch, dir) && (EXIT(ch, dir)->to_room != NOWHERE)
      && IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)
      && !IS_SET(EXIT(ch, dir)->exit_info, EX_LOCKED))
    {
      if( (dir == DIR_DOWN) || (dir == DIR_UP) )
      {
        snprintf(buf, MAX_STRING_LENGTH, "You open the %sward %s.\n", dirs[dir],
          EXIT(ch, dir)->keyword ? FirstWord(EXIT(ch, dir)->keyword) : "door" );
      }
      else
      {
        snprintf(buf, MAX_STRING_LENGTH, "You open the %s to the %s.\n",
          EXIT(ch, dir)->keyword ? FirstWord(EXIT(ch, dir)->keyword) : "door", dirs[dir] );
      }
      send_to_char(buf, ch);
      snprintf(buf, MAX_STRING_LENGTH, "%s", dirs[(int) dir]);
      do_open(ch, buf, 0);
    }

    if( IS_AFFECTED( ch, AFF_HIDE ) )
    {
      send_to_char( "You leave the shadows and continue your hunt.\n", ch );
      REMOVE_BIT( ch->specials.affected_by, AFF_HIDE );
    }
    do_move(ch, NULL, exitnumb_to_cmd(dir));

    // Could die from various crap
    if( !char_in_list(ch) )
    {
      return;
    }

    add_event(event_track_move, 10 + number(-2, 10), ch, vict, 0, 0, 0, 0);
    act("$n intently searches for tracks.", TRUE, ch, 0, 0, TO_ROOM);
    notch_skill(ch, SKILL_TRACK, 20);
  }
}

bool valid_track_edge(int from_room, int dir) {
  return VALID_TRACK_EDGE(from_room, dir);
}

void do_track_not_in_use(P_char ch, char *arg, int cmd)
{
  int skill, epic_skill, percent, found_track;
  P_obj obj;
  char descbuf[MAX_STRING_LENGTH];

#ifdef NOTRACK
  send_to_char
    ("Tracking is a passive skill. If you know how, you will see tracks.\r\n",
     ch);
  return;
#endif

  if (IS_NPC(ch))
  {
    return;
  }
  
  if( IS_TRUSTED(ch) )
  {
    int i = 0;
  	vector<int> path;
    
    if(!is_number(arg) || ((i = real_room(atoi(arg))) < 0) || (i > top_of_world))
    {
      send_to_char("Room not in world.\n", ch);
      return;
    }
    
		bool found_path = dijkstra(ch->in_room, i, valid_track_edge, path );
    
    if( found_path )
    {
      snprintf(descbuf, MAX_STRING_LENGTH, "&+BFound path (%ld steps):\r\n", (long)path.size());
      send_to_char(descbuf, ch);
      for( vector<int>::iterator it = path.begin(); it != path.end(); it++ )
      {
        snprintf(descbuf, MAX_STRING_LENGTH, "%s ", short_dirs[*it]);
        send_to_char(descbuf, ch);
      }
      send_to_char("\r\n", ch);
    }
    else
    {
      send_to_char("&+RCouldn't find a valid path.\r\n", ch);
    }
    
    return;
  }

  // Setup the numbers
  skill = GET_CHAR_SKILL(ch, SKILL_TRACK);
  epic_skill = GET_CHAR_SKILL(ch, SKILL_IMPROVED_TRACK);
  percent = number(1, 100);

  if (!skill)
  {
    send_to_char("You don't know how to track!\r\n", ch);
    return;
  }
  
  send_to_char("You pause to study your surroundings.\r\n", ch);
  act("$n pauses to study his surroundings.", TRUE, ch, 0, 0, TO_ROOM);

  // Display the map if they have improved track and are on the map
  if (epic_skill && IS_MAP_ROOM(ch->in_room))
  {
    show_tracking_map(ch);
    send_to_char("\r\n", ch);
  }

  found_track = FALSE;
  
  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    if (obj->R_num == real_object(4))
    {
      if (obj->value[1] == BLOOD_FRESH)
      {
        snprintf(descbuf, MAX_STRING_LENGTH, "%s\n", obj->description);
        send_to_char(descbuf, ch);
        found_track = TRUE;
      }
      else if ((obj->value[1] == BLOOD_REG) &&
               (epic_skill >= number(50, 70)))
      {
        snprintf(descbuf, MAX_STRING_LENGTH, "%s\n", obj->description);
        send_to_char(descbuf, ch);
        found_track = TRUE;
      }
      else if ((obj->value[1] == BLOOD_DRY) &&
               (epic_skill >= number(90, 100)))
      {
        snprintf(descbuf, MAX_STRING_LENGTH, "%s\n", obj->description);
        send_to_char(descbuf, ch);
        found_track = TRUE;
      }
    }
  }
  
  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    if (obj->R_num == real_object(VNUM_TRACKS))
    {
      if (skill >= number(90, 100))
      {
        snprintf(descbuf, MAX_STRING_LENGTH, "%s\n", obj->description);
        send_to_char(descbuf, ch);
      }
      else 
      {
        snprintf(descbuf, MAX_STRING_LENGTH, "There are some tracks");
        if (skill >= 50)
        {
				  strcat(descbuf, " heading ");
          strcat(descbuf, dirs[obj->value[0]]);
          strcat(descbuf, ".\n");
        }
        else
        {
				  strcat(descbuf, ".\n");
				}	
        send_to_char(descbuf, ch);
      }  
      found_track = TRUE;
    }
  }
  
  if (found_track != TRUE)
  {
    send_to_char("&+LYou are unable to find any tracks.&n\r\n", ch);
  }

  notch_skill(ch, SKILL_TRACK, 10);

  CharWait(ch, (int)(PULSE_VIOLENCE * (float)get_property("track.scan.lag", 1)));

  return;
}

void do_track(P_char ch, char *arg, int cmd) //do_track_not_in_use
{
  P_char   victim;
  int      skill_lvl;
  char     name[MAX_INPUT_LENGTH];

/*
  struct trackrecordtype *hmm;
  struct trackmainttype *hmm2;
*/

#ifdef NOTRACK
  send_to_char("Tracking is a passive skill. If you know how, you will see tracks.\r\n", ch);
  return;
#endif

  /* Pets can't track.  */
  if( IS_NPC(ch) )
  {
    return;
  }

  skill_lvl = IS_TRUSTED(ch) ? 100 : GET_CHAR_SKILL(ch, SKILL_TRACK);
  if( !skill_lvl )
  {
    send_to_char("You dont know how to track!\r\n", ch);
    return;
  }

  one_argument(arg, name);

  if (!strcmp(name, "scan"))
  {
    if (IS_MAP_ROOM(ch->in_room))
    {
      show_tracking_map(ch);
      CharWait(ch, (int)(PULSE_VIOLENCE * (float)get_property("track.scan.lag", 1)));
      return;
    }
    else
    {
      // Should we do anything for non map rooms?
    }
  }

  /* wimps */
  if (!strcmp(name, "stop") || !strcmp(name, "off"))
  {
    send_to_char("You abandon the hunt.\r\n", ch);
    ch->specials.tracking = 0;
    REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING);
    CharWait(ch, PULSE_VIOLENCE * 2);
    return;
  }
  if (IS_AFFECTED3(ch, AFF3_TRACKING))
  {
    send_to_char("You are already tracking something!\n\r", ch);
    return;
  }
  /* Are they in the game? */
  /* Slight mod, if they aren't in game, set them tracking room 1200 */
  /* This way its transparent if they're in the game, or not */

/* OLD IF
  if (!(victim = get_char_vis(ch, name)) || racewar(ch, victim) || IS_AFFECTED3(victim, AFF3_PASS_WITHOUT_TRACE)) {
    send_to_char("You are unable to find any trace.\r\n", ch);
    return;
  }
*/
/* New IF */
 // victim = get_char_vis(ch, name);

  if( !name[0] && (GET_SPEC(ch, CLASS_ROGUE, SPEC_ASSASSIN) || GET_CLASS(ch, CLASS_ASSASSIN)
    || GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF)) && ch->specials.was_fighting
    && char_in_list(ch->specials.was_fighting) )
  {
    victim = ch->specials.was_fighting;
  }
  else
  {
    victim = get_char_vis(ch, name);
  }

  // is victim disguised?  if so, do some special checking to make sure
  // tracker is not tracking by real name, thereby bypassing racewars
  // checks due to the operation of racewar()/get_char_vis() functions

  if( victim && IS_DISGUISE(victim) && isname(name, GET_NAME(victim)) )
  {
    victim = NULL;
  }

  if( (!victim || IS_AFFECTED3(victim, AFF3_PASS_WITHOUT_TRACE)) )
  {
    // If it's a God, allow tracking to room vnum.
    if( !IS_TRUSTED(ch) || real_room(atoi(name)) <= 0 )
    {
      send_to_char("You are unable to find any tracks.\n", ch);
      return;
    }
  }

  SET_BIT(ch->specials.affected_by3, AFF3_TRACKING);
  ch->specials.tracking = (victim ? victim->in_room : real_room(atoi(name)));
  ch->specials.was_fighting = victim;
  send_to_char("You attempt your skills at tracking.\n", ch);
  add_event(event_track_move, 5, ch, victim, 0, 0, 0, 0);
  CharWait(ch, PULSE_VIOLENCE);
  notch_skill(ch, SKILL_TRACK, 20);

  return;
}

int MaxTrackDist(P_char ch)
{
  int      dist;

  if (!ch->only.pc->skills[SKILL_TRACK].learned)
    dist = 3;
  else
    dist = ch->only.pc->skills[SKILL_TRACK].learned / 3;

  if (GET_CLASS(ch, CLASS_ROGUE) || GET_CLASS(ch, CLASS_THIEF) ||
      GET_CLASS(ch, CLASS_RANGER))
    dist = (int) (dist * 1.5);

  switch (GET_RACE(ch))
  {
  case RACE_GREY:
    dist = (int) (dist * 1.5);  /* even better */
    break;
  case RACE_DEVIL:
  case RACE_DEMON:
    dist = MAX_ROOMS;           /* as good as can be */
    break;
  default:
    break;
  }

  if (IS_TRUSTED(ch))
    dist = MAX_ROOMS;

  return dist;
}

// 90 seconds, 1.5 mins
#define MAX_TRACK_DURATION 90 * WAIT_SEC

void add_track(P_char ch, int dir)
{
  P_char  mount;
  P_obj   track, obj, next_obj;
  int     counter, found, dura, zon;
  char    buf1[MAX_STRING_LENGTH];
//  char    buf2[MAX_STRING_LENGTH];
  char    buf3[MAX_STRING_LENGTH];
  struct extra_descr_data *ed;

  // Gods don't leave tracks.
  if (IS_TRUSTED(ch))
    return;

  // NPCs don't leave tracks.
  if (!IS_PC(GET_PLYR(ch)))
    return;

  if (IS_AFFECTED3(ch, AFF3_PASS_WITHOUT_TRACE))
    return;

  // These races leave no tracks
  if( GET_RACE(ch) == RACE_INSECT || GET_RACE(ch) == RACE_GHOST
    || GET_RACE(ch) == RACE_FAERIE || GET_RACE(ch) == RACE_PARASITE )
    return;

  // If they're in nowhere.. no tracks.
  if (ch->in_room == NOWHERE)
    return;

  // If they hit a wall.. no tracks.
  if (!EXIT(ch, dir))
    return;

  dura = MAX_TRACK_DURATION;

  switch (world[ch->in_room].sector_type)
  {
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
    case SECT_UNDERWATER:
    case SECT_UNDRWLD_WATER:
    case SECT_UNDRWLD_NOGROUND:
    case SECT_NO_GROUND:
    case SECT_FIREPLANE:
    case SECT_OCEAN:
    case SECT_UNDRWLD_NOSWIM:
    case SECT_ASTRAL:
    case SECT_ETHEREAL:
    case SECT_ROAD:
      return;
    case SECT_UNDRWLD_INSIDE:
    case SECT_INSIDE:
      dura /= 4;
      break;
    case SECT_UNDERWATER_GR:
    case SECT_CITY:
    case SECT_LAVA:
      dura /= 3;
      break;
    case SECT_DESERT:
    case SECT_ARCTIC:
      if (GET_RACE(ch) == RACE_BARBARIAN)
        dura = (2 * dura) / 3;
      else
        dura += 60;
      break;
    case SECT_FOREST:
      if( GET_RACE(ch) == RACE_GREY )
        dura /= 4;
      else if( GET_RACE(ch) == RACE_CENTAUR )
        dura /= 2;
      else if( GET_RACE(ch) == RACE_HALFELF )
        dura /= 3;
      break;
    case SECT_UNDRWLD_MOUNTAIN:
    case SECT_UNDRWLD_SLIME:
    case SECT_UNDRWLD_MUSHROOM:
    case SECT_UNDRWLD_LIQMITH:
    case SECT_UNDRWLD_WILD:
    case SECT_UNDRWLD_CITY:
      if( GET_RACE(ch) == RACE_DROW || GET_RACE(ch) == RACE_DUERGAR )
        dura /= 4;
      else
        dura *= 2;
      break;
    case SECT_HILLS:
      if( GET_RACE(ch) == RACE_HALFLING || GET_RACE(ch) == RACE_GOBLIN )
        dura /= 3;
      break;
    case SECT_MOUNTAIN:
      if (GET_RACE(ch) == RACE_MOUNTAIN || GET_RACE(ch) == RACE_DUERGAR)
        dura /= 3;
      else
        dura += 40;
      break;
    default:
      break;                      /* stuff is as we want it to be.      */
  }

/*  Removing weather conditions until weather is rationalized
  zon = world[ch->in_room].zone;
  if (zon <= 0)
    break;
  if (!sector_table[zon].conditions.precip_rate)
    break;

  dura = dura * (100 - sector_table[zon].conditions.precip_rate) * 
       (100 - sector_table[zon].conditions.windspeed) / 100 / 100;
*/

  // Flying people break branches and push down grasses as they fly around
  if( IS_AFFECTED(ch, AFF_FLY) )
  {
    if (number(0,2))
      dura /= 5;
    else
      return;
  }
  // Sneaking only makes a difference if not flying.
  else if( IS_AFFECTED(ch, AFF_SNEAK) )
    dura /= 2;

  // Dragonkin tracks are gigantic
  if( GET_RACE(ch) == RACE_DRAGONKIN )
    dura *= 2;

  dura += GET_WEIGHT(ch) / 10;

  // Minimum 10 seconds, maximum MAX_TRACK_DURATION.
  dura = BOUNDED( 10 * WAIT_SEC, dura, MAX_TRACK_DURATION );

  // Count the number of tracks in the room.
  counter = 0;
  for (obj = world[ch->in_room].contents; obj; obj = next_obj)
  {
     next_obj = obj->next_content;
     if (obj->R_num == real_object(VNUM_TRACKS))
       counter++;
  }

  // If there are too many tracks for the sector type..
  if (counter >= track_limit[world[ch->in_room].sector_type])
  {
    for (obj = world[ch->in_room].contents; obj; obj = next_obj)
    {
      if (counter >= track_limit[world[ch->in_room].sector_type])
      {
        // Remove one set of tracks.
        if (obj->R_num == real_object(VNUM_TRACKS))
      	{
          extract_obj(obj, TRUE); // Not an arti, but 'in game.'
  	      obj = NULL;
	        break;
        }
      }
      next_obj = obj->next_content;
    }
  }

  track = read_object(VNUM_TRACKS, VIRTUAL);

  strcpy(buf1, "");
//  strcpy(buf2, "");
  strcpy(buf3, "");

  // Show tracks of the mount if mounted.
  if( (mount = GET_MOUNT(ch)) != NULL )
  {
    snprintf(buf1, MAX_STRING_LENGTH, "There are deep %s tracks going %s.", race_names_table[(int) GET_RACE(mount)].ansi, dirs[dir]);
    strcpy(buf3, mount->player.short_descr);
    track->value[0] = dir;
    dura += 15 * WAIT_SEC;
  }
  else if( IS_DISGUISE_SHAPE(ch) )
  {
    snprintf(buf1, MAX_STRING_LENGTH, "There are %s tracks going %s.", race_names_table[(int) GET_DISGUISE_RACE(ch)].ansi, dirs[dir]);
    strcpy(buf3, ch->disguise.name);
    track->value[0] = dir;
  }
  else
  {
    snprintf(buf1, MAX_STRING_LENGTH, "There are %s tracks going %s.", race_names_table[(int) GET_RACE(ch)].ansi, dirs[dir]);
    strcpy(buf3, ch->player.short_descr);
    track->value[0] = dir;
  }

  //snprintf(buf1, MAX_STRING_LENGTH, "There are tracks here going %s.", dirs[dir]);

  set_long_description(track, buf1);  

  /*
  snprintf(buf2, MAX_STRING_LENGTH, "These appear to be the track of %s", buf3);
  set_short_description(track, buf2); 
  ed = track->ex_description;
  ed->description = str_dup(buf2);
  */


/* ch in NOWHERE already checked for with return.
  if (ch->in_room == NOWHERE)
  {
    if (real_room(ch->specials.was_in_room) != NOWHERE)
      obj_to_room(track, real_room(ch->specials.was_in_room));
    else
    {
      extract_obj(track, TRUE); // Not an arti, but 'in game.'
      track = NULL;
    }
  }
  else
*/
  obj_to_room(track, ch->in_room);

  set_obj_affected(track, dura, TAG_OBJ_DECAY, 0);
  return;

#if 0
  {
    tmp->weight = (GET_WEIGHT(ch) / 5);
    tmp->height = (GET_HEIGHT(ch) / 5);
  {

  tmp->race = GET_RACE(ch);
  tmp->name = GET_NAME(ch);
  tmp->dir = dir;
  tmp->next = first_track;
  tmp->maint = tmp2;
  if (tmp->next)
    tmp->next->prev = tmp;
  if (tmp->prev)
    tmp->prev->next = tmp;
  first_track = tmp;
  tmp->next_track = tmp2->first;
  if (tmp->next_track)
    tmp->next_track->prev_track = tmp;
  tmp2->first = tmp;
  if (!last_track)
    last_track = tmp;
  if (!tmp2->last)
    tmp2->last = tmp;

  if(tmp2->tracks_in_room > track_limit[world[ch->in_room].sector_type])
    nuke_track(tmp2->first);

#   if 0
  /* This practically lowers it to from 1/25 to 1/1 of the original      */
  dura = dura * (16 - STAT_INDEX(GET_C_DEX(ch))) / 23;
#   endif

  /* Ok, now weather      */

  tmp2->tracks_in_room++;

  dura = BOUNDED(1, dura, MAX_TRACK_DURATION);
  AddEvent(EVENT_TRACK_DECAY, dura, TRUE, tmp, 0);
#endif
}

void nuke_track(struct trackrecordtype *hmm)
{
  return;
#if 0
  struct trackmainttype *hmm2;

  if ((hmm2 = world[hmm->maint->in].track))
  {
    if (hmm->prev_track)
      hmm->prev_track->next_track = hmm->next_track;
    if (hmm->next_track)
      hmm->next_track->prev_track = hmm->prev_track;
    if (hmm->prev_track == NULL)
      if (hmm2->first == hmm)
        hmm2->first = hmm->next_track;
    if (hmm->next_track == NULL)
      if (hmm2->last == hmm)
        hmm2->last = hmm->prev_track;

    /* Ok, now all traces of track in a room are nuked.      */
    /* next project: nuke track from global database      */

    if (hmm->next)
      hmm->next->prev = hmm->prev;
    else
      last_track = hmm->prev;
    if (hmm->prev)
      hmm->prev->next = hmm->next;
    else
      first_track = hmm->next;
    hmm->next = spare_track_base;
    spare_track_base = hmm;
    return;
  }
  logit(LOG_EXIT, "error in track decay! *PANIC*!");
  raise(SIGSEGV);
#endif
}

char    *sickprocess(const char *arg)
{
  static char hmm[20];

  if (arg == NULL)
    return NULL;
  if (arg[0] == 0)
/*
   return arg;    old line, generates errors. fix later. 
 */
    return NULL;
  snprintf(hmm, 20, "%c%s", tolower(arg[0]), arg + 1);
  return hmm;
}

// Shows tracks / messages when appropriate to ch for tracks in room room.
void show_tracks(P_char ch, int room)
{
  P_obj    obj, next_obj;
  char     Gbuf3[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];
  int      chance, percent, skill, level, race;
  bool     outside, forest_check;

  // Changed this from IS_NPC for the sake of switched Immortals.
  if( !ch->desc )
  {
    return;
  }

  if( room < 0 || room > top_of_world )
  {
    return;
  }

  if( !CAN_SEE(ch, ch) )
  {
    return;
  }

  skill = GET_CHAR_SKILL(ch, SKILL_IMPROVED_TRACK);
  percent = number(1, 100);
  level = GET_LEVEL(ch);
  race = GET_RACE(ch);
  outside = FALSE; // set for shaman/druid presence detection
  forest_check = GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND) && IS_SECT(room, SECT_FOREST);

  // If they don't have the skill, make it impossible to use it.
  if( skill == 0 )
  {
    skill = -1000;
  }

  // If we're using farsee or such
  if( room != ch->in_room )
  {
    skill -= 25;
  }

  if( has_innate(ch, INNATE_PERCEPTION) )
  {
    skill += 30;
  }

  if( IS_AFFECTED(ch, AFF_FLY) )
  {
    percent += 30;
  }

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    return;
  }

  switch( GET_POS(ch) )
  {
	case POS_PRONE:
	  percent -= 20;
    break;
  case POS_KNEELING:
  case POS_SITTING:
    percent -= 10;
    break;
  case POS_STANDING:
    percent += 10;
    break;
  default:
    break;
  }

  if( IS_AFFECTED(ch, AFF_FARSEE) )
  {
    percent -= 10;
  }
  if( IS_AFFECTED4(ch, AFF4_HAWKVISION))
  {
    percent -= 10;
  }

  if( affected_by_spell(ch, SPELL_MOLEVISION) )
  {
    percent += 20;
  }

  switch( world[room].sector_type )
  {
    case SECT_CITY:
      if( GET_CLASS(ch, CLASS_ROGUE | CLASS_ASSASSIN | CLASS_THIEF | CLASS_MERCENARY)
        || GET_SPEC(ch, CLASS_ROGUE, SPEC_ASSASSIN) )
      {
        skill += 15;
      }

      if( GET_CLASS(ch, CLASS_RANGER) )
      {
        skill -= 20;
      }

      if( race != RACE_HUMAN && race != RACE_BARBARIAN && race != RACE_ORC && race != RACE_HALFELF )
      {
        skill -=15;
      }
      break;
    case SECT_FIELD:
    case SECT_ROAD:
      outside = TRUE;
      break;
    case SECT_FOREST:
      if( GET_CLASS(ch, CLASS_RANGER) )
      {
        skill += 15;
      }

      if( race == RACE_GREY || race == RACE_CENTAUR )
        skill += 10;
      else if( race == RACE_HALFELF )
        skill += 5;
      else
        skill -=20;
      outside = TRUE;
      break;
    case SECT_HILLS:
      if( race == RACE_HALFLING )
        skill += 20;
      else if( race == RACE_MOUNTAIN )
        skill += 10;
      else
        skill -= 10;
      outside = TRUE;
      break;
    case SECT_MOUNTAIN:
      if( race == RACE_MOUNTAIN || race == RACE_DUERGAR )
        skill += 30;
      else
        skill -= 20;
      break;
    case SECT_SWAMP:
      if( race == RACE_TROLL )
        skill += 20;
      else
        skill -= 30;
      break;
    case SECT_DESERT:
      percent -= 20;
      break;
    case SECT_ARCTIC:
      if( race == RACE_BARBARIAN )
        skill += 10;
      percent -= 10;
      break;
    case SECT_UNDRWLD_WILD:
    case SECT_UNDRWLD_CITY:
    case SECT_UNDRWLD_MOUNTAIN:
    case SECT_UNDRWLD_SLIME:
    case SECT_UNDRWLD_LOWCEIL:
    case SECT_UNDRWLD_LIQMITH:
    case SECT_UNDRWLD_MUSHROOM:
      if( race == RACE_DROW || race == RACE_DUERGAR )
        skill += 30;
      else
        skill -= 30;
      break;
    case SECT_INSIDE:
    case SECT_UNDRWLD_INSIDE:
      skill -= 40;
      break;
    default:
      break;
  }
  if( IS_INSIDE(room) )
  {
    outside = FALSE;
  }

  /* For special cases only, people with the tracking skill use the skill/percent caculations
   *   Special cases get a a 10% chance up to 46 and a 30% chance at 56 for detecting the track (as per below).
   * Now we have a skill, percent, & chance...
   */
  chance = 10 + (GET_LEVEL(ch) > 46) ? (2 * (GET_LEVEL(ch) - 46)) : 0;

  for( obj = world[room].contents; obj; obj = next_obj )
  {
    next_obj = obj->next_content;

    if( obj->R_num == real_object(VNUM_TRACKS) )
    {
      if( affected_by_spell(ch, SPELL_AURA_SIGHT) )
      {
        if( chance > number(1, 100) )
        {
          send_to_char("&+LThere are residual brain waves in the area.\n", ch);
        }
      }
      if( affected_by_spell(ch, SPELL_BLOODHOUND) )
      {
        if( chance + 50 > number(1, 100) )
        {
          //snprintf(Gbuf3, MAX_STRING_LENGTH, "%s\n", obj->description);
          //send_to_char(Gbuf3, ch);
          send_to_char("&+rYou smell a scent in the area.\n", ch);
        }
      }
      /*
       * Most druids will get the disturbance.
       * Forest druids do not see disturbance in forest rooms; they can see tracks.
       * Tweaked this such that we only skip if we're in a forest room.
       */
      // All druids except Ranger/Druids (They get better).
      if( !GET_PRIME_CLASS(ch, CLASS_RANGER) && GET_CLASS(ch, CLASS_DRUID)
        && !forest_check )
      {
        if( outside )
        {
          if( chance > number(1, 100) )
          {
            send_to_char("&+gThere is a slight disturbance in nature.\n", ch);
          }
        }
      }
      // shamans get the straight chance for all of the outside terrains
      if( GET_CLASS(ch, CLASS_SHAMAN) )
      {
        if( outside )
        {
          if (chance > number(1, 100) )
          {
            send_to_char("&+mThere is a slight disturbance in the spirit realm here.\n", ch);
          }
        }
      }
      if( skill > percent || forest_check )
      {
        if (percent < 0)
        {
          percent = 0;
        }
        // 2/3 chance for ppl with 30 greater skill than necessary & 1/2 for forest druids in forest.
        if( (number(0, 2) && skill - percent > 30) || (number(0,1) && forest_check) )
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "%s\n", obj->description);
          send_to_char(Gbuf3, ch);
        }
        else
        {
          // I guess forest druids don't see race-less tracks.
          if( forest_check )
          {
            continue;
          }
          snprintf(Gbuf3, MAX_STRING_LENGTH, "There are some tracks");

          if( strstr(obj->description, "north") )
            strcat(Gbuf3, " heading north.\n");
          else if( strstr(obj->description, "south") )
            strcat(Gbuf3, " heading south.\n");
          else if( strstr(obj->description, "west") )
            strcat(Gbuf3, " heading west.\n");
          else if( strstr(obj->description, "east") )
            strcat(Gbuf3, " heading east.\n");
          else
            strcat(Gbuf3, ".\n");

          send_to_char(Gbuf3, ch);
        }
      }
    }
  }

  /* I guess skill notching is done elsewhere.
  if (GET_CLASS(ch, CLASS_RANGER) ||
      GET_CLASS(ch, CLASS_DRUID) ||
      GET_CLASS(ch, CLASS_THIEF) ||
      GET_CLASS(ch, CLASS_ROGUE) ||
      GET_CLASS(ch, CLASS_ASSASSIN) ||
      GET_SPEC(ch, CLASS_ROGUE, SPEC_ASSASSIN))
  {
    notch_skill(ch, SKILL_TRACK, 10);
  }
  else
  {
    notch_skill(ch, SKILL_TRACK, 20);
  }
  */

  return;
}

void show_tracking_map(P_char ch)
{
  struct affected_type af;

  memset(&af, 0, sizeof(af));
  af.type = SKILL_TRACK;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOSAVE;
  af.duration = 1;
  affect_to_char(ch, &af);

  map_look(ch, TRUE);

  return;
}
