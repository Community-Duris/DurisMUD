#ifndef __MAP_H__
#define __MAP_H__

#include "db.h"

// White map backgrounds
#define SURFACE_MAP_START 500000
#define SURFACE_MAP_END 659999

#define IS_SURFACE_MAP(r) (world[r].number >= SURFACE_MAP_START && world[r].number <= SURFACE_MAP_END)

// Black background maps
#define UD_MAP_START 700000
#define UD_MAP_END 859999

#define UD_ALATORIN_START 120000
#define UD_ALATORIN_END 123833

#define IS_UD_MAP(r) ( (world[r].number >= UD_MAP_START && world[r].number <= UD_MAP_END) \
   || (world[r].number >= UD_ALATORIN_START && world[r].number <= UD_ALATORIN_END) )

#define IS_MAP_ZONE(r) (IS_SET(zone_table[r].flags, ZONE_MAP))
#define IS_MAP_ROOM(r) (IS_SURFACE_MAP(r) || IS_UD_MAP(r) || IS_MAP_ZONE(world[r].zone))

#define CONT_GC          1
#define CONT_EC          2
#define CONT_UC          3
#define CONT_IC          4
#define CONT_KK          5
#define CONT_JADE        6
#define CONT_DRAGONS     7
#define CONT_CEOTHIA     8
#define CONT_BOYARD      9
#define CONT_VENAN      10
#define CONT_SHADOW     11
#define CONT_SCORCHED   12
#define CONT_TEZCAT     13
#define CONT_MOONSHAE   14


#define CONTINENT(r) ( world[r].continent )
#define IS_CONTINENT(room, cont) ( CONTINENT(room) == cont )

//  adding defines for the planes for rewrite of plane_shift -  Jexni 1/26/08

#define ASTRALSTART 19701
#define ASTRALEND 19825
#define IS_ASTRAL(r) (world[r].number >= ASTRALSTART && world[r].number <= ASTRALEND )
#define EARTHSTART 23801
#define EARTHEND 23925
#define IS_EARTH(r) (world[r].number >= EARTHSTART && world[r].number <= EARTHEND )
#define AIRSTART 24401
#define AIREND 24525
#define IS_AIR(r) (world[r].number >= AIRSTART && world[r].number <= AIREND )
#define FIRESTART 25401
#define FIREEND 25525
#define IS_FIRE(r) (world[r].number >= FIRESTART && world[r].number <= FIREEND )
#define WATERSTART 23201
#define WATEREND 23325
#define IS_WATER(r) (world[r].number >= WATERSTART && world[r].number <= WATEREND )
#define ETHSTART 12401
#define ETHEND 12550
#define IS_ETH(r) (world[r].number >= ETHSTART && world[r].number <= ETHEND )
#define NEGSTART 26600
#define NEGEND 26681
#define IS_NEG(r) (world[r].number >= NEGSTART && world[r].number <= NEGEND )

#define BLOOD_FRESH 1
#define BLOOD_REG   2
#define BLOOD_DRY   3

#define MAP_IGNORE_TOGGLE 0
#define MAP_USE_TOGGLE    1
#define MAP_AUTOMAP       2

typedef struct _mapSymbolInfo
{
  const char *colorStrn;        // ansi code string, sans &
  bool     hasBg;               // non-black background color?
} mapSymbolInfo;

void assign_continents();
bool set_continent(int start_room, int continent);
const char *continent_name(int continent_id);
void calculate_map_coordinates();
const char* get_map_direction(int from, int to);

#endif // __MAP_H__

