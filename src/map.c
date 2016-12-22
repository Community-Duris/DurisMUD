/*
   ***************************************************************************
   *  File: map.c                                             Part of Duris  *
   *  Copyright  1995 - Duris Systems Ltd.                                   *
   ***************************************************************************
 */
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#include <list>
using namespace std;

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "stdio.h"
#include "config.h"
#include "db.h"
#include "justice.h"
#include "graph.h"
#include "random.zone.h"
#include "weather.h"
#include "spells.h"
#include "map.h"
#include "tradeskill.h"
#include "ships/ships.h"
#include "guildhall.h"
#include "ctf.h"
#include "vnum.mob.h"
#include "vnum.obj.h"

struct continent
{
  int id;
  const char name[64];
  int seed_room;
} continents[] = {
  {CONT_GC, "&+WGood Continent", 546926},
  {CONT_EC, "&+LEvil Continent", 607066},
  {CONT_IC, "&+WIce &+CCrag", 521504},
  {CONT_KK, "&+gKhomani-Khan", 608451},
  {CONT_UC, "&+rUndead Continent", 567408},
  {CONT_JADE, "&+GJade &+gEmpire", 644892},
  {CONT_DRAGONS, "&+cIsland &+yof &+cDragons", 643968},
  {CONT_CEOTHIA, "&+bCeothia", 628685},
  {CONT_BOYARD, "&+rFort &+RBoyard", 562323},
  {CONT_VENAN, "&+YVenan'Trut", 545016},
  {CONT_SHADOW, "&+LShadow Island", 513382},
  {CONT_SCORCHED, "&+rSc&+Ro&+Yrc&+Rh&+red &+rIsland", 576166},
  {CONT_TEZCAT, "&+rT&+Rez&+rca&+Rtl&+rip&+Ro&+rca", 575445},
  {CONT_MOONSHAE, "&+YMoonshae &+GIsland", 556818},
  {0, 0, 0}
};

/*
   external variables
 */

extern P_char char_in_room(int);
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const char *dirs[];
extern const int rev_dir[];
extern const int top_of_world;
extern int top_of_zone_table;

extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern int LOADED_RANDOM_ZONES;
extern struct time_info_data time_info;
extern const racewar_struct racewar_color[MAX_RACEWAR+2];
extern struct continent_misfire_data continent_misfire;

int      whats_in_maproom(P_char, int, int);
// These are set in weather.c
int      map_normal_modifier;
int      map_ultra_modifier;
int      map_dayblind_modifier;

void     add_quest_data(char *map);

/* map specific defines */
// Category 1: Stuff that overrides everything:
#define CONTAINS_NOTHING      0
#define CONTAINS_CH           1
#define CONTAINS_CTF_FLAG     2
// Category 2: Stuff that's really big:
#define CONTAINS_GOOD_SHIP    3
#define CONTAINS_EVIL_SHIP    4
#define CONTAINS_UNDEAD_SHIP  5
#define CONTAINS_NEUTRAL_SHIP 6
#define CONTAINS_UNKNOWN_SHIP 7
#define CONTAINS_SHIP         8
#define CONTAINS_FERRY        9
// Category 3: Light/Dark:
#define CONTAINS_MAGIC_DARK  10
#define CONTAINS_MAGIC_LIGHT 11
// Category 4: Lifeforms:
#define CONTAINS_WITCH       12
#define CONTAINS_DRAGON      13
#define CONTAINS_BUILDING    14
#define CONTAINS_GOOD_PC     15
#define CONTAINS_EVIL_PC     16
#define CONTAINS_GROUP       17
#define CONTAINS_PC          18
#define CONTAINS_MOB         19
// Category 5: Objects:
#define CONTAINS_PORTAL      20
#define CONTAINS_GUILDHALL   21
#define CONTAINS_CORPSE      22
#define CONTAINS_TRACK       23
#define CONTAINS_BLOOD       24
#define CONTAINS_OLD_BLOOD   25
#define CONTAINS_MINE        26
#define CONTAINS_GEMMINE     27
#define CONTAINS_CARGO       28
#define CONTAINS_MAX         28

#define HIDDEN_BY_FOREST(from_room,to_room) ( world[to_room].sector_type == SECT_FOREST && world[from_room].sector_type != SECT_FOREST )

const char *sector_symbol[NUM_SECT_TYPES] = {
  "^",                          /* * larger towns */
  "+",                          /* * roads */
  ".",                          /* * plains/fields */
  "*",                          /* * forest */
  "^",                          /* * hills */
  "M",                          /* * mountains */
  "r",                          /* * water shallow */
  " ",                          /* * water boat */
  " ",                           /* * noground */
  " ",                           /* * underwater */
  " ",                           /* * underwater ground */
  " ",                          /* * fire plane */
  " ",                          /* * water ship */
  ".",                          /* * UD wild */
  "*",                          /* * UD city */
  ".",                          /* * UD inside */
  " ",                          /* * UD water */
  " ",                          /* * UD noswim */
  " ",                           /* * UD noground */
  " ",                           /* * air plane */
  " ",                           /* * water plane */
  " ",                          /* * earth plane */
  " ",                           /* * etheral plane */
  "R",                          /* * astral plane */
  ".",                          /* desert */
  ".",                          /* arctic tundra */
  "*",                          /* swamp */
  "M",                          /* UD mountains */
  "*",                          /* UD slime */
  ",",                          /* UD low ceilings */
  " ",                          /* UD liquid mithril */
  "o",                          /* UD mushroom forest */
  "#",                          /* Castle Wall */
  "^",                          /* Castle Porticulus */
  "O",                           /* Castle Itself */
  " ",                           // Negative Plane
  " ",                           // Plane of Avernus
  "+",                           // Patrolled Road
  "*",                           // Snowy Forest
  " "                            // Lava
};

// whee..  'typedef char bool'?  BAH!

mapSymbolInfo color_symbol[NUM_SECT_TYPES] = {
  {"=wl", true},
  {"+L" , false},
  {"+g" , false},
  {"+g" , false},
  {"+y" , false},
  {"+y" , false},
  {"=cw", true},
  {"=bB", true},
  {"+w" , false},
  {"+w" , false},
  {"+w" , false},
  {"=rR", true},
  {"=bB", true},
  {"=mL", true},
  {"=wl", true},
  {"+m" , false},
  {"=bB", true},
  {"=bB", true},
  {"+w" , false},
  {"+w" , false},
  {"+L" , false},
  {"+w" , false},
  {"+w" , false},
  {"=Lr", true},
  {"=yY", true},
  {"+W" , false},
  {"=mL", true},
  {"+L" , false},
  {"=rL", true},
  {"+M" , false},
  {"=wW", true},
  {"+M" , false},
  {"=wL", true},
  {"=wB", true},
  {"+r" , false},
  {"=lw", true},
  {"=lw", true},
  {"+L" , false},
  {"=lg", true},
  {"=rR", true}
};

unsigned int calculate_relative_room(unsigned int rroom, int x, int y)
{
  int      local_y, local_x;
  int vroom = world[rroom].number, local_map;

  struct zone_data *zone = &zone_table[world[rroom].zone];
  
  if( !IS_SET(zone->flags, ZONE_MAP))
    return 0;
  
  int zone_start_vnum = world[zone->real_bottom].number;
  
  // how far are we from the northern local map edge
  local_y = ( ( vroom - zone_start_vnum) / zone->mapx ) % zone->mapy;

  // how far are we from the western local map edge
  local_x = ( vroom - zone_start_vnum) % zone->mapy;
  
  if( local_x + x < 0 )
    local_x += zone->mapx;
  else if( local_x + x >= zone->mapx )
    local_x -= zone->mapx;

  if( local_y + y < 0 )
    local_y += zone->mapy;
  else if( local_y + y >= zone->mapy )
    local_y -= zone->mapy;
  
  return real_room0(zone_start_vnum + local_x + x + ( (local_y + y ) * zone->mapx));
}

/*
 * return square of the distance between 2 given real rooms
 */
int calculate_map_distance(int room1, int room2)
{
  int v1, v2, map1, map2, x1, dx, y1, dy, x2, y2;

  if( &zone_table[world[room1].zone] != &zone_table[world[room2].zone] )
    return -1;
  
  struct zone_data *zone = &zone_table[world[room1].zone];
  
  if( !IS_SET(zone->flags, ZONE_MAP))
    return -1;
  
  unsigned int zone_start_vnum = world[zone->real_bottom].number;
  
  
  v1 = world[room1].number - zone_start_vnum;
  v2 = world[room2].number - zone_start_vnum;

  x1 = v1 % zone->mapx;
  y1 = (v1 / zone->mapx ) % zone->mapy;

  x2 = v2 % zone->mapx;
  y2 = (v2 / zone->mapx ) % zone->mapy;
  
  dx = x1 - x2;
  dy = y1 - y2;
  
  return dx * dx + dy * dy;
}

/* takes rnum */

int whats_in_maproom(P_char ch, int room, int distance, int show_regardless)
{
  P_obj    obj; // For looping through objs in a room.
  P_char   who; // For looping through chars in a room.
  int z, zw, portal = FALSE, val = CONTAINS_MAX+1;
  int from_room = ch->in_room;

  if( !IS_ALIVE(ch) || room <= 0 )
  {
    return CONTAINS_NOTHING;
  }

  // Category 1: Stuff that overrides everything:
  if( room == from_room )
  {
    return CONTAINS_CH;
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
  // If nobody in room, the for loop won't iterate: no reason to check to see if there's anyone in there.
  for( who = world[room].people; who; who = who->next_in_room )
  {
    // Since room != ch->in_room (see CONTAINS_CH above), we don't need to check ch == who.
    if( !IS_ALIVE(who) || IS_TRUSTED(who) )
      continue;
    if( ctf_carrying_flag(who) == CTF_PRIMARY )
      return CONTAINS_CTF_FLAG;
  }
#endif

  // Underwater / sight blocked / 999 ??
  if( (ch->specials.z_cord < 0) || IS_ROOM(room, ROOM_BLOCKS_SIGHT) || (show_regardless == 999) )
  {
    return CONTAINS_NOTHING;
  }

  if( (world[room].sector_type == SECT_CASTLE || world[room].sector_type == SECT_CASTLE_WALL
    || world[room].sector_type == SECT_CASTLE_GATE)
    && (world[from_room].sector_type != SECT_CASTLE && world[from_room].sector_type != SECT_CASTLE_WALL) )
  {
    return CONTAINS_NOTHING;
  }

  // Look through objects...
  if( world[room].contents )
  {
    for( obj = world[room].contents; obj; obj = obj->next_content )
    {
      if( obj->type == ITEM_SHIP )
      {
        if( isname("__ferry__", obj->name) )
        {
          val = MIN(val, CONTAINS_FERRY);
        }
        else
        {
          P_ship temp = shipObjHash.find(obj);
          if( temp )
          {
            if( SHIP_DOCKED(temp) || temp->race == NPCSHIP )
            {
              val = MIN(val, CONTAINS_SHIP);
            }
            else if( temp->race == GOODIESHIP )
            {
              val = MIN(val, CONTAINS_GOOD_SHIP);
            }
            else if( temp->race == EVILSHIP )
            {
              val = MIN(val, CONTAINS_EVIL_SHIP);
            }
            else if( temp->race == UNDEADSHIP )
            {
              val = MIN(val, CONTAINS_UNDEAD_SHIP);
            }
            else if( temp->race == SQUIDSHIP )
            {
              val = MIN(val, CONTAINS_NEUTRAL_SHIP);
            }
            else if( temp->race == UNKNOWNSHIP )
            {
              val = MIN(val, CONTAINS_UNKNOWN_SHIP);
            }
            else
            {
              val = MIN(val, CONTAINS_SHIP);
            }
          }
        }
      }
      else if( obj->type == ITEM_CORPSE )
      {
        if( !str_cmp(ch->player.name, obj->action_description)
          || IS_TRUSTED(ch) || has_innate(ch, INNATE_VISION_OF_THE_DEAD) )
        {
          val = MIN(val, CONTAINS_CORPSE);
        }
      }
      else if(OBJ_VNUM(obj) == VOBJ_GEMMINE && (( (has_innate(ch, INNATE_MINER) ||
        IS_AFFECTED5(ch, AFF5_MINE)) && distance < 2 ) || IS_TRUSTED(ch) ) )
      {
        val = MIN(val, CONTAINS_GEMMINE);
      }
      else if(OBJ_VNUM(obj) == VOBJ_MINE && (( (has_innate(ch, INNATE_MINER) ||
        IS_AFFECTED5(ch, AFF5_MINE)) && distance < 3 ) || IS_TRUSTED(ch) ) )
      {
        val = MIN(val, CONTAINS_MINE);
      }
      // Using track scan ?
      else if( (OBJ_VNUM(obj) == VNUM_TRACKS) && (( distance <= (GET_CHAR_SKILL(ch, SKILL_IMPROVED_TRACK) / 20) )
        || IS_TRUSTED(ch)) )
      {
        val = MIN(val, CONTAINS_TRACK);
      }
      else if( (OBJ_VNUM(obj) == VOBJ_BLOOD) && (( distance <= (GET_CHAR_SKILL(ch, SKILL_IMPROVED_TRACK) / 20) )
        || IS_TRUSTED(ch)) )
      {
        if( (obj->value[1] == BLOOD_FRESH) && (GET_CHAR_SKILL(ch, SKILL_IMPROVED_TRACK) > number(50, 80)) )
        {
          val = MIN(val, CONTAINS_BLOOD);
        }
        else if( GET_CHAR_SKILL(ch, SKILL_IMPROVED_TRACK) >= number(80, 100) )
        {
          val = MIN(val, CONTAINS_OLD_BLOOD);
        }
      }
      else if( obj->type == ITEM_TELEPORT && OBJ_VNUM(obj) >= 99800 && OBJ_VNUM(obj) <= 99899 )
      {
        val = MIN(val, CONTAINS_PORTAL);
      }
      else if( OBJ_VNUM(obj) == GH_DOOR_VNUM )
      {
        val = MIN(val, CONTAINS_GUILDHALL);
      }
      else if( OBJ_VNUM(obj) == VOBJ_CARGO_CRATE && (world[room].sector_type == SECT_WATER_NOSWIM
        || world[room].sector_type == SECT_OCEAN || world[room].sector_type == SECT_UNDRWLD_NOSWIM) )
      {
        val = MIN(val, CONTAINS_CARGO);
      }
    }
  }

  // Category 2: Stuff that's really big - If we found something really big in the above search.
  if( val < CONTAINS_MAGIC_DARK )
  {
    return val;
  }

  // Category 3: Light/Dark:
  if( IS_ROOM(room, ROOM_MAGIC_DARK) && !IS_ROOM(room, ROOM_MAGIC_LIGHT) )
  {
    return CONTAINS_MAGIC_DARK;
  }

  if( IS_ROOM(room, ROOM_MAGIC_LIGHT) && !IS_ROOM(room, ROOM_MAGIC_DARK) )
  {
    return CONTAINS_MAGIC_LIGHT;
  }
  // Forest covers up Lifeforms and Objects.
  if( HIDDEN_BY_FOREST(from_room, room) )
  {
    return CONTAINS_NOTHING;
  }

  // Category 4: Lifeforms:
  if( world[room].people )
  {
    for( who = world[room].people; who; who = who->next_in_room )
    {
      if( who == ch )
        continue;

      if( IS_TRUSTED(who) )
        continue;

      if( IS_PC_PET(ch) )
        continue;

      if( !CAN_SEE_Z_CORD(ch, who) )
        continue;

      if( affected_by_spell(who, TAG_BUILDING) )
      {
        val = CONTAINS_BUILDING;
        continue;
      }

      zw = who->specials.z_cord;
      z = ch->specials.z_cord;

      if( !IS_TRUSTED(ch) )
      {
        if( IS_AFFECTED3(who, AFF3_PASS_WITHOUT_TRACE) && SECTOR_TYPE(who->in_room) == SECT_FOREST )
          continue;

        if( affected_by_spell(who, SKILL_EXPEDITIOUS_RETREAT) )
          continue;

        if( has_innate(who, INNATE_SWAMP_SNEAK) && SWAMP_SNEAK_TERRAIN(who) )
          continue;

        if( GET_SPEC(who, CLASS_ROGUE, SPEC_THIEF) && IS_AFFECTED(who, AFF_SNEAK) )
        {
          if( (( GET_CHAR_SKILL(who, SKILL_SNEAK) * .46 ) + (GET_LEVEL(who) >= 30) ? (GET_LEVEL(who) * 2 - 58) : 0 )
            >= number(1, 100) )
          {
            continue;
          }
        }
      }

      if( IS_NPC(who) && (GET_VNUM(who) == VMOB_WITCH) )
      {
        val = MIN( val, CONTAINS_WITCH);
        continue;
      }

      if( IS_DRAGON(who) || IS_DEMON(who) || IS_DEVIL(who) )
      {
        val = MIN( val, CONTAINS_DRAGON);
        continue;
      }

      if( IS_DISGUISE_NPC(who) )
      {
        val = MIN( val, CONTAINS_MOB);
        continue;
      }

      if( IS_PC(who) )
      {
        if( distance < 8 && grouped(ch, who) )
        {
          val = MIN( val, CONTAINS_GROUP);
          continue;
        }

        if( distance <= 4 )
        {
          // Priority of good/evil: You see enemy P before friendly P color.
          if( GET_RACEWAR(ch) != GET_RACEWAR(who) )
          {
            if( IS_GOOD(who) && IS_AFFECTED2(ch, AFF2_DETECT_GOOD) )
            {
              val = MIN( val, CONTAINS_GOOD_PC);
            }
            // Here is tricky.. CONTAINS_GOOD_PC < CONTAINS_EVIL_PC, but we still want to show Evil over Good.
            else if( IS_EVIL(who) && IS_AFFECTED2(ch, AFF2_DETECT_EVIL) )
            {
              val = (val == CONTAINS_GOOD_PC) ? CONTAINS_EVIL_PC : MIN(val, CONTAINS_EVIL_PC);
            }
            else
            {
              val = MIN(val, CONTAINS_PC);
            }
          }
          // For friendlies, we don't change the color of the P (if it is colored) to preserve the above.
          else
          {
            if( val == CONTAINS_EVIL_PC || val == CONTAINS_GOOD_PC )
            {
              continue;
            }

            if( IS_EVIL(who) && IS_AFFECTED2(ch, AFF2_DETECT_EVIL) )
            {
              val = MIN( val, CONTAINS_EVIL_PC);
            }
            else if( IS_GOOD(who) && IS_AFFECTED2(ch, AFF2_DETECT_GOOD) )
            {
              val = MIN( val, CONTAINS_GOOD_PC);
            }
            else
            {
              val = MIN(val, CONTAINS_PC);
            }
          }
        }
        else
        {
          val = MIN(val, CONTAINS_PC);
        }
        continue;
      }
      if( IS_NPC(who) && GET_SIZE(who) >= SIZE_SMALL )
      {
          val = MIN(val, CONTAINS_MOB);
      }
    }
  }

  // Category 5: Objects - handled during Category 2.
  return val;
}

// Show ch the map with a distance of n as if the ch is in room from_room
//   and show_map_regardless
void display_map_room(P_char ch, int from_room, int n, int show_map_regardless)
{
  int      x, y, where, what, from_what, prev = -1, temp;
  int      where_rnum, whats_in, distance;
  bool     hadbg = false, map_tile;
  char     buf[MAX_STRING_LENGTH], minibuf[10];
  float    horizontal_factor, vertical_factor;
  P_ship   ship;

  // If ch doesn't exist/is dead/doesn't have a descriptor to send info to.
  if( !IS_ALIVE(ch) || !ch->desc )
  {
    return;
  }

  if(n > 500)
  {
    send_to_char("Lines too wide, can't show map safely!\n\r", ch);
    return;
  }

  from_what = SECTOR_TYPE(from_room);

  if( ch->desc->term_type == TERM_MSP )
  {
    if( show_map_regardless != MAP_AUTOMAP )
    {
      send_to_char("\n<map>\n", ch, LOG_NONE);
    }
    else
    {
      send_to_char("<automap>\n", ch, LOG_NONE);
    }
  }
  else
  {
    send_to_char("&n \n", ch);
  }

  horizontal_factor = get_property("map.horizontalFactor", 0.6);
  vertical_factor = get_property("map.verticalFactor", 1.2);

  for (y = (int) (-0.6 * n); y <= (int) (0.6 * n); y++)
  {
    buf[0] = '\0';

    /* send a space, each line */
    if( ch->desc->term_type != TERM_MSP )
    {
      send_to_char("           ", ch, LOG_NONE);
    }

    for (x = -n; x <= n; x++)
    {
      where_rnum = calculate_relative_room(from_room, x, y);
      distance = (int)sqrt(x * x + y * y);

      if(!where_rnum && IS_UD_MAP(from_room))
      {
        what = SECT_EARTH_PLANE;
      }
      else if(!where_rnum )
      {
        what = SECT_OCEAN;
      }
      else
      {
        what = SECTOR_TYPE(where_rnum);
      }

      what = BOUNDED(0, (int) what, (NUM_SECT_TYPES-1));
      map_tile = false;

      if (hadbg)
      {
        strcat(buf, "&n");
      }

      whats_in = whats_in_maproom(ch, where_rnum, distance, show_map_regardless);

      if (horizontal_factor * y * y + vertical_factor * x * x > n * n * 0.6)
      {
        strcat(buf, " ");
      }
      // We want an @ iff we're on land, and a [< | ^ | > | v] if on an undocked ship.
      // All we need to check here is on undocked ship since @ is done via CONTAINS_CH below.
      else if( x == 0 && y == 0 && (ship = get_ship_from_char(ch)) && !SHIP_DOCKED(ship)
        && ship->location == from_room )
      {
        float heading = ship->heading;
        // Use an arrow in the direction of the ship.
        if( heading > 315 || heading <= 45 )
        {
          strcat(buf, "&+W^&n");
        }
        else if( heading > 45 && heading <= 135 )
        {
          strcat(buf, "&+W>&n");
        }
        else if( heading > 135 && heading <= 225 )
        {
          strcat(buf, "&+Wv&n");
        }
        else
        {
          strcat(buf, "&+W<&n");
        }
      }
      else if (whats_in == CONTAINS_CH)
      {
        strcat(buf, "&+W@&n");
      }
      else if (whats_in == CONTAINS_MAGIC_DARK)
      {
        strcat(buf, "&+LD&n");
      }
      else if (whats_in == CONTAINS_MAGIC_LIGHT)
      {
        strcat(buf, "&+WL&n");
      }
      else if (whats_in == CONTAINS_GOOD_SHIP)
      {
        sprintf(minibuf, "&+%cS&n", racewar_color[RACEWAR_GOOD].color );
        strcat(buf, minibuf);
      }
      else if (whats_in == CONTAINS_EVIL_SHIP)
      {
        sprintf(minibuf, "&+%cS&n", racewar_color[RACEWAR_EVIL].color );
        strcat(buf, minibuf);
      }
      else if (whats_in == CONTAINS_UNDEAD_SHIP)
      {
        sprintf(minibuf, "&+%cS&n", racewar_color[RACEWAR_UNDEAD].color );
        strcat(buf, minibuf);
      }
      else if (whats_in == CONTAINS_NEUTRAL_SHIP)
      {
        sprintf(minibuf, "&+%cS&n", racewar_color[RACEWAR_NEUTRAL].color );
        strcat(buf, minibuf);
      }
      else if (whats_in == CONTAINS_UNKNOWN_SHIP)
      {
        sprintf(minibuf, "&+%cS&n", racewar_color[MAX_RACEWAR+1].color );
        strcat(buf, minibuf);
      }
      else if (whats_in == CONTAINS_SHIP)
      {
        sprintf(minibuf, "&+%cS&n", racewar_color[RACEWAR_NONE].color );
        strcat(buf, minibuf);
      }
      else if (whats_in == CONTAINS_FERRY)
      {
        strcat(buf, "&+WF&n");
      }
      else if (whats_in == CONTAINS_DRAGON)
      {
        strcat(buf, "&=LRD&n");
      }
      else if (whats_in == CONTAINS_WITCH)
      {
        strcat(buf, "&=LWW&n");
      }
      else if (whats_in == CONTAINS_BUILDING)
      {
        strcat(buf, "&+C#&n");
      }
      else if(whats_in == CONTAINS_GUILDHALL)
      {
        strcat(buf, "&+CG&n");
      }
      else if(whats_in == CONTAINS_CARGO)
      {
        strcat(buf, "&=bB&+Lo&n");
      }
      else if(whats_in == CONTAINS_MOB)
      {
        strcat(buf, "&=LBM&n");
      }
      else if (whats_in == CONTAINS_TRACK)
      {
        strcat(buf, "&+Y.");
      }
      else if (whats_in == CONTAINS_OLD_BLOOD)
      {
        strcat(buf, "&+r.");
      }
      else if (whats_in == CONTAINS_BLOOD)
      {
        strcat(buf, "&+R.");
      }
      else if ((whats_in == CONTAINS_PORTAL))
      {
        strcat(buf, "&+MO");
      }
      else if(whats_in == CONTAINS_GROUP)
      {
        strcat(buf, "&=LGP&n");
      }
      else if(whats_in == CONTAINS_GOOD_PC)
      {
        strcat(buf, "&=LYP&n");
      }
      else if(whats_in == CONTAINS_EVIL_PC)
      {
        strcat(buf, "&=LRP&n");
      }
      else if(whats_in == CONTAINS_PC)
      {
        strcat(buf, "&=LWP&n");
      }
      else if (whats_in == CONTAINS_CORPSE)
      {
        strcat(buf, "&+rC");
      }
      else if (whats_in == CONTAINS_MINE)
      {
        strcat(buf, "&+Ym");
      }
      else if (whats_in == CONTAINS_GEMMINE)
      {
        strcat(buf, "&+ym");
      }
      else if (ch->specials.z_cord < 0 && what != SECT_OCEAN && what != SECT_WATER_NOSWIM && what != SECT_NO_GROUND)
      {                         /* underwater */
        strcat(buf, "&+L ");
      }
#if defined(CTF_MUD) && (CTF_MUD == 1)
      else if (whats_in == CONTAINS_CTF_FLAG)
      {
	strcat(buf, "&=LYF");
      }
#endif
      else if( (prev != what || what == SECT_FOREST) || (x == -n) )
      {
        int shift = 0;

        if (hadbg && color_symbol[what].hasBg)
        {
          shift = -2;
        }

        sprintf( buf + strlen(buf) + shift, "&%s%s",
          (what == SECT_FOREST && !number( 0,2 )) ? "+G" : color_symbol[what].colorStrn, sector_symbol[what]);
        hadbg = color_symbol[what].hasBg;
        prev = what;
        map_tile = true;
      }
      else
      {
        int shift = 0;
        if(hadbg)
        {
          shift = -2;
        }
        sprintf(buf + strlen(buf) + shift, "%s", sector_symbol[what]);
        map_tile = true;
      }

      if(!map_tile)
      {
        prev = -1;
        hadbg = false;
      }
    }

    strcat(buf, "&n \n");       // removed '&n'
    send_to_char(buf, ch);
  }

  if( ch->desc->term_type == TERM_MSP )
  {
    if( show_map_regardless != MAP_AUTOMAP )
    {
      send_to_char("\n</map>\n", ch, LOG_NONE);
    }
    else
    {
      send_to_char("\n</automap>\n", ch, LOG_NONE);
    }
  }
}

void display_map(P_char ch, int n, int show_map_regardless)
{
  display_map_room(ch, ch->in_room, n, show_map_regardless);
}

int map_view_distance(P_char ch, int room)
{
  int n;

  if( !IS_ALIVE(ch) )
  {
    return 0;
  }

  // Imms see 12 + altitude.
  if( IS_TRUSTED(ch) )
  {
    return 12 + ((ch->specials.z_cord > 0) ? ch->specials.z_cord : 0);
  }
  // eyeless - this means that ANY other factors mean nothing.
  else if( has_innate(ch, INNATE_EYELESS) )
  {
    return 8;
  }
  // if on the surface
  else if( IS_SURFACE_MAP(room) )
  {
    // The map_*_modifiers vary over time of day (done in weather.c).
    if( IS_DAYBLIND(ch) && IS_SUNLIT(ch->in_room) )
    {
      if( IS_AFFECTED(ch, AFF_INFRAVISION) )
      {
        n = BOUNDED(0, (map_dayblind_modifier + 2), 8);
      }
      else
      {
        n = BOUNDED(0, (map_dayblind_modifier), 8);
      }
    }
    else
    {
      if( IS_AFFECTED2(ch, AFF2_ULTRAVISION) )
      {
        n = BOUNDED(0, (map_ultra_modifier), 8);
      }
      else
      {
        n = BOUNDED(0, (map_normal_modifier), 8);
      }
      // Infra raises view slightly on surface maps.
      if( IS_AFFECTED(ch, AFF_INFRAVISION) )
      {
        n = MIN( 8, n + 1 );
      }
    }
    if( IS_AFFECTED(ch, AFF_FLY) )
    {
      n++;
    }
  }
  // UD maps have a constant ultravision glow, but not much in the way of natural light.
  else if( IS_UD_MAP(room) )
  {
    if( IS_DAYBLIND(ch) )
    {
      n = 8;
    }
    else if( IS_AFFECTED2(ch, AFF2_ULTRAVISION) )
    {
      n = 6;
    }
    else
    {
      n = 4;
    }
    if( IS_AFFECTED(ch, AFF_UD_VISION) )
    {
      n += 2;
    }
    // Infra raises view slightly on UD maps too.
    else if( IS_AFFECTED(ch, AFF_INFRAVISION) )
    {
      n++;
    }
  }
  // In a zone.. ?!
  else
  {
    n = 5;

    if(IS_AFFECTED4(ch, AFF4_HAWKVISION))
    {
      n += 1;
    }
  }

  if( IS_FOREST_ROOM(room) && !has_innate(ch, INNATE_FOREST_SIGHT) && n > 3 )
  {
    n = 3;
  }

  if( IS_FOREST_ROOM(room) && IS_AFFECTED5(ch, AFF5_FOREST_SIGHT) )
  {
    n = 7;
  }

  if( has_innate(ch, INNATE_PERCEPTION) )
  {
    n += 1;
  }

  if( IS_OCEAN_ROOM(room) )
  {
    n = 10;
  }
  else
  {
    n = BOUNDED(0, n, 10);
  }

  if( ch->specials.z_cord > 0 )
  {
    n += ch->specials.z_cord;
  }

  return n;
}

// ch = person to show the map to, room = the center of the map, show_map_regardless = ignore the PLR_MAP toggle.
void map_look_room(P_char ch, int room, int show_map_regardless)
{
  char tot_buf[MAX_STRING_LENGTH];
  int  n;

  // If don't have a living char with a desc to send the map to.
  if( !IS_ALIVE(ch) || !ch->desc )
  {
    return;
  }

  /* quickly repair them, before they see the wrong thing */
  if( ch->specials.z_cord < 0 && !IS_WATER_ROOM(room) )
  {
    send_to_char("&+BYou rise out of the water!&n\n\r", ch );
    ch->specials.z_cord = 0;
  }

  // If we're not ignoring the toggle and we have a PC (includes switched Imms), and they have
  //   the toggle off, don't show the map.
  if( show_map_regardless != MAP_IGNORE_TOGGLE
    && IS_PC(GET_TRUE_CHAR(ch)) && !IS_SET(GET_TRUE_CHAR(ch)->specials.act, PLR_MAP) )
  {
    return;
  }

  if( (n = map_view_distance(ch, room)) > 1 )
  {
    display_map_room(ch, room, n, show_map_regardless);
  }
}

void map_look(P_char ch, int show_map_regardless)
{
  map_look_room(ch, ch->in_room, show_map_regardless);
}

bool is_in_line_of_sight_dir(P_char ch, P_char target, int current_room)
{
  int      start, end, x, x2, temp, count, where, which_dir;

  if( !IS_ALIVE(ch) )
  {
    return false;
  }

  start = world[ch->in_room].number;

  return FALSE;

  end = world[target->in_room].number;
  x = 0;
  x2 = 0;
  where = 0;
  which_dir = line_of_sight_dir(ch->in_room, target->in_room);


  if (which_dir == 0)
    return FALSE;

  for (count = 0; count < 5; count++)
  {
    if (which_dir == DIR_NORTH || which_dir == DIR_NORTHEAST ||
        which_dir == DIR_NORTHWEST)
      x -= 100;
    else if (which_dir == DIR_SOUTH || which_dir == DIR_SOUTHEAST ||
             which_dir == DIR_SOUTHWEST)
      x += 100;
    if (which_dir == DIR_EAST || which_dir == DIR_NORTHEAST || which_dir == DIR_SOUTHEAST)
      x2 -= 1;
    else if (which_dir == DIR_WEST || which_dir == DIR_NORTHWEST ||
             which_dir == DIR_SOUTHWEST)
      x2 += 1;

    /* Did the last bump toss us onto a new map? Adjust our figures then... */
    if (((start / 10000) * 10000) < (((start + x) / 10000) * 10000))
    {
      x += 20000;
      if ((x + start) > 199999)
      {
        x -= 90000;
      }
    }
    else if (((start / 10000) * 10000) > (((start + x) / 10000) * 10000))
    {
      x -= 20000;
      if ((x + start) < 110000)
        x += 90000;
    }
    temp = (start + x) - (((start + x) / 100) * 100);
    if (temp + x2 < 0)
    {
      where = (start + x - 9901 + x2);
      if ((where / 10000) == 13 || (where / 10000) == 16 ||
          (where / 10000) == 19)
        where += 30000;
    }
    else if (temp + x2 > 99)
    {
      where = (start + x + 9901 + x2);
      if ((where / 10000) == 14 || (where / 10000) == 17 ||
          (where / 10000) == 20)
        where -= 30000;
    }
    else
      where = (start + x + x2);

    start = where;

    if (SECTOR_TYPE(real_room0(start)) == SECT_HILLS ||
        SECTOR_TYPE(real_room0(start)) == SECT_FOREST ||
        SECTOR_TYPE(real_room0(start)) == SECT_MOUNTAIN)
      return FALSE;

    if (start == current_room)
    {
      if (CAN_SEE_Z_CORD(ch, target))
        return TRUE;
      /* Should we also check large objects, such as ships blocking view? */

    }
  }
  return FALSE;
}

void assign_continents()
{
  fprintf(stderr, "Assigning continents...\r\n");

  for( int i = 0; continents[i].id; i++ )
  {
    fprintf(stderr, " - %s ", strip_ansi(continents[i].name).c_str());
    set_continent(real_room0(continents[i].seed_room), continents[i].id);
  }
  for( int i = 0;i < NUM_CONTINENTS; i++ )
  {
    for( int j = 0; j <= MAX_RACEWAR; j++ )
    {
      continent_misfire.players[i][j] = 0;
      continent_misfire.misfiring[i][j] = FALSE;
    }
  }
}

bool set_continent(int start_room, int continent) 
{       
  if( !start_room )
    return FALSE;
  
  list<int> rooms_left;
  
  rooms_left.push_back(start_room);
  
  int room = -1;
  int count = 0;
  while( !rooms_left.empty() ) 
  {
    room = rooms_left.front();
    rooms_left.pop_front();
    
    world[room].continent = continent;
    count++;
    
    for( int dir = 0; dir < NUM_EXITS; dir++ ) 
    {
      if( !world[room].dir_option[dir] ) continue;
      if( !TOROOM(room, dir) ) continue;      
      if( world[TOROOM(room, dir)].continent ) continue;
      if( TOROOM(room, dir) == NOWHERE ) continue;
      if( !world[TOROOM(room,dir)].dir_option[rev_dir[dir]] ) continue;
      if( TOROOM(TOROOM(room, dir), rev_dir[dir]) != room ) continue;      
      if( TO_OCEAN(room, dir) ) continue;
      if( !SAME_ZONE(room, dir) ) continue;
      
      rooms_left.push_front( TOROOM(room, dir) );
    }
    
  }
  
  fprintf(stderr, "(%d rooms)\n", count);
  
  return TRUE;
}

const char *continent_name(int continent_id)
{
  for( int i = 0; continents[i].id; i++ )
  {
    if( continents[i].id == continent_id )
      return continents[i].name;
  }
  
  return NULL;
}

void calculate_map_coordinates()
{
    sh_int cur_section = 0;

    int* room_stack = (int*) malloc(sizeof(int) * top_of_world);

    unsigned rs_pointer = 0;
    if (!room_stack)
    {
      logit(LOG_BOARD, " Error - malloc failed in map coordinate calculation");
      exit(1);
    }
    for (int room = 0; room <= top_of_world; room++)
    {
      world[room].x_coord = 0;
      world[room].y_coord = 0;
      world[room].z_coord = 0;
      world[room].map_section = 0;
    }
    bfs_clear_marks();
    for (int room = 0; room <= top_of_world; room++)
    {
      if (IS_MARKED(room) || !IS_MAP_ROOM(room) || IS_OCEAN_ROOM(room))
         continue;

      cur_section++;
      unsigned rs_start = rs_pointer;
      BFSMARK(room);
      world[room].x_coord = 0;
      world[room].y_coord = 0;
      world[room].z_coord = 0;
      world[room].map_section = cur_section;
       
      room_stack[rs_pointer++] = room;
      for (unsigned rs_pointer1 = rs_start; rs_pointer1 != rs_pointer; rs_pointer1++)
      {
        int curr_room = room_stack[rs_pointer1];
        for (int curr_dir = 0; curr_dir < NUM_EXITS; curr_dir++)
        {
          if(world[curr_room].dir_option[curr_dir])
          {
            int next_room = world[curr_room].dir_option[curr_dir]->to_room;

            if (!world[next_room].dir_option[rev_dir[curr_dir]] ||
               (world[next_room].dir_option[rev_dir[curr_dir]]->to_room != curr_room))
            {
               continue;  // skipping teleport exits
            }

            if (IS_MARKED(next_room) || !IS_MAP_ROOM(next_room) || IS_OCEAN_ROOM(next_room))
              continue;

            BFSMARK(next_room);                                           
            switch(curr_dir)
            {                                                             
            case DIR_NORTH:                                                   
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord;        
              world[next_room].y_coord = world[curr_room].y_coord + 1;    
              world[next_room].z_coord = world[curr_room].z_coord;        
            } break;                                                      
            case DIR_EAST:                                                    
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord + 1;    
              world[next_room].y_coord = world[curr_room].y_coord;        
              world[next_room].z_coord = world[curr_room].z_coord;        
            } break;                                                      
            case DIR_SOUTH:                                                   
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord;        
              world[next_room].y_coord = world[curr_room].y_coord - 1;    
              world[next_room].z_coord = world[curr_room].z_coord;        
            } break;                                                      
            case DIR_WEST:                                                    
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord - 1;    
              world[next_room].y_coord = world[curr_room].y_coord;        
              world[next_room].z_coord = world[curr_room].z_coord;        
            } break;                                                      
            case DIR_UP:                                                      
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord;        
              world[next_room].y_coord = world[curr_room].y_coord;        
              world[next_room].z_coord = world[curr_room].z_coord + 1;    
            } break;                                                      
            case DIR_DOWN:                                                    
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord;        
              world[next_room].y_coord = world[curr_room].y_coord;        
              world[next_room].z_coord = world[curr_room].z_coord - 1;    
            } break;                                                      
            case DIR_NORTHWEST:                                               
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord - 1;    
              world[next_room].y_coord = world[curr_room].y_coord + 1;    
              world[next_room].z_coord = world[curr_room].z_coord;        
            } break;                                                      
            case DIR_SOUTHWEST:                                               
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord - 1;    
              world[next_room].y_coord = world[curr_room].y_coord - 1;    
              world[next_room].z_coord = world[curr_room].z_coord;        
            } break;                                                      
            case DIR_NORTHEAST:                                               
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord + 1;    
              world[next_room].y_coord = world[curr_room].y_coord + 1;    
              world[next_room].z_coord = world[curr_room].z_coord;        
            } break;                                                      
            case DIR_SOUTHEAST:                                               
            {                                                             
              world[next_room].x_coord = world[curr_room].x_coord + 1;    
              world[next_room].y_coord = world[curr_room].y_coord - 1;    
              world[next_room].z_coord = world[curr_room].z_coord;        
            } break;                                                      
            default:                                                      
              break;                                                      
            };                                                            
            world[next_room].map_section = cur_section;                   
            room_stack[rs_pointer++] = next_room;                         
          }
        }
      }
      sh_int min_x = (sh_int)SHRT_MAX, min_y = (sh_int)SHRT_MAX, min_z = (sh_int)SHRT_MAX;
      for (unsigned rs_pointer1 = rs_start; rs_pointer1 < rs_pointer; rs_pointer1++)
      {
        int curr_room = room_stack[rs_pointer1];
        if (world[curr_room].x_coord < min_x) min_x = world[curr_room].x_coord;
        if (world[curr_room].y_coord < min_y) min_y = world[curr_room].y_coord;
        if (world[curr_room].z_coord < min_z) min_z = world[curr_room].z_coord;
      }
      for (unsigned rs_pointer1 = rs_start; rs_pointer1 < rs_pointer; rs_pointer1++)
      {
        int curr_room = room_stack[rs_pointer1];
        world[curr_room].x_coord -= min_x;
        world[curr_room].y_coord -= min_y;
        world[curr_room].z_coord -= min_z;
      }
    }
    free(room_stack);
}

const char* get_map_direction(int from, int to)
{
    if (world[from].map_section == 0 || world[from].map_section != world[to].map_section)
      return "somewhere";
    double delta_x = (double)(world[to].x_coord - world[from].x_coord);
    double delta_y = (double)(world[to].y_coord - world[from].y_coord);

    if (delta_x == 0)
    {
       if (delta_y > 0)
         return "north";
       if (delta_y < 0)
         return "south";
       return "somewhere";
    }
    double delta_tan = delta_y / delta_x;
    double angle = atan (delta_tan) * 180 / 3.14159265358979323846;
    if (delta_x < 0)
      angle = 180 + angle;
    else if (delta_x > 0 && angle < 0)
      angle = 360 + angle;

    if (angle < 20 || angle >= 340)
      return "east";
    else if (angle >= 20 && angle < 70)
      return "northeast";
    else if (angle >= 70 && angle < 110)
      return "north";
    else if (angle >= 110 && angle < 160)
      return "northwest";
    else if (angle >= 160 && angle < 200)
      return "west";
    else if (angle >= 200 && angle < 250)
      return "southwest";
    else if (angle >= 250 && angle < 290)
      return "south";
    else if (angle >= 290 && angle < 340)
      return "southeast";

    return "somewhere";
}
