#ifndef _GRAPH_H_
#define _GRAPH_H_

#define BFS_ERROR         -1
#define BFS_ALREADY_THERE -2
#define BFS_NO_PATH       -3

#define BFS_MAX_ROOMS     12000U /* limits how many rooms are searched for a target */
#define BFS_MAX_ROOMS_MAP 160000

// bitflags passed to find_first_step() to determine exits, etc used
#define BFS_CAN_FLY     BIT_1
#define BFS_CAN_DISPEL  BIT_2
#define BFS_BREAK_WALLS BIT_3
#define BFS_STAY_ZONE   BIT_4
#define BFS_ROADRANGER  BIT_5 // special type that ignores 'target' search for a room "IS_ROADRANGER_TARGET()"
#define BFS_AVOID_NOMOB BIT_6

#define WAGON_TYPE_WAGON  1
#define WAGON_TYPE_FLYING 2

/*
 * Utility macros
 */

void       bfs_reset_marks();
void       bfs_clear_marks();
extern int bfs_cur_marker;
#define BFSMARK(room)   (world[room].bfs_mark = bfs_cur_marker)
#define BFSUNMARK(room) (world[room].bfs_mark = 0)
#define IS_MARKED(room) (world[room].bfs_mark == bfs_cur_marker)
#define TOROOM(x, y)    (world[(x)].dir_option[(y)]->to_room)

#define IS_ROADRANGER_TARGET(x) (((x) != NOWHERE) && (world[x].sector_type == SECT_ROAD))

#define IS_CLOSED(x, y)    (IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_CLOSED))
#define IS_SECRET(x, y)    (IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_SECRET))
#define IS_WALLED(x, y)    (IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_WALLED))
#define IS_BREAKABLE(x, y) (IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_BREAKABLE))
#define IS_LOCKED(x, y)    (IS_CLOSED(x, y) && IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_LOCKED))
#define IS_HIDDEN(x, y)    (IS_SECRET(x, y) || IS_BLOCKED(x, y))
#define IS_BLOCKED(x, y)   (IS_CLOSED(x, y) && IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_BLOCKED))
#define SAME_ZONE(x, y)    (world[x].zone == world[TOROOM(x, y)].zone)
#define TO_OCEAN(x, y)     (world[TOROOM(x, y)].sector_type == SECT_OCEAN)
#define TO_MOUNTAIN(x, y)  (((world[TOROOM(x, y)].sector_type == SECT_MOUNTAIN) || (world[TOROOM(x, y)].sector_type == SECT_UNDRWLD_MOUNTAIN)) && IS_MAP_ROOM(TOROOM(x, y)))
#define TO_ROAD(x, y)      (world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && (world[TOROOM(x, y)].sector_type == SECT_CITY))
#define ON_ROAD(x)         (world[x].sector_type == SECT_CITY)
#define TO_DOCKABLE(x, y)  (IS_ROOM(TOROOM(x, y), ROOM_DOCKABLE))
#define TO_GUILD(x, y)     (IS_ROOM(TOROOM(x, y), ROOM_GUILD))
#define TO_NOSWIM(x, y)    (world[TOROOM(x, y)].sector_type == SECT_WATER_NOSWIM)
#define NEEDS_FLY(x, y)    (TO_OCEAN(x, y) || TO_NOSWIM(x, y))
#define TO_NOGROUND(x, y)  (world[TOROOM(x, y)].sector_type == SECT_NO_GROUND)

#define VALID_EDGE(x, y)                                                                                                                                                                               \
	(world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && /* (!IS_SECRET(x, y)) && */ /* (!IS_WALLED(x, y)) &&  */                                                                               \
	 (!IS_LOCKED(x, y)) && (!IS_BLOCKED(x, y)) && (!TO_OCEAN(x, y)) && (!TO_MOUNTAIN(x, y)) && (!TO_GUILD(x, y)) &&                                                                                    \
	 (zone_table[world[x].zone].hometown == zone_table[world[TOROOM(x, y)].zone].hometown) && (!IS_MARKED(TOROOM(x, y))))

// Similar to VALID_EDGE, missing the hometown check, but faster due to less dereferencing.
#define VALID_EDGE2(exit)                                                                                                                                                                              \
	((exit != NULL) && (exit->to_room != NOWHERE) && !IS_MARKED(exit->to_room) && !IS_SET(exit->exit_info, EX_LOCKED | EX_BLOCKED) && !IS_OCEAN_ROOM(exit->to_room) &&                                 \
	 (((world[exit->to_room].sector_type != SECT_MOUNTAIN) && (world[exit->to_room].sector_type != SECT_UNDRWLD_MOUNTAIN)) || !IS_MAP_ROOM(exit->to_room)) && !IS_ROOM(exit->to_room, ROOM_GUILD))

// Handles the missing hometown check difference between VALID_EDGE() and VALID_EDGE2()
#define HOMETOWN_CHECK(src, dest) (zone_table[world[src].zone].hometown == zone_table[world[dest].zone].hometown)

#define VALID_RADIAL_EDGE(x, y)                                                                                                                                                                        \
	(world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && (world[x].zone == world[TOROOM(x, y)].zone) && /* (!IS_SECRET(x, y)) && */                                                             \
	 (!IS_WALLED(x, y)) && (!IS_LOCKED(x, y)) && (!IS_BLOCKED(x, y)) && (!TO_GUILD(x, y)) && (zone_table[world[x].zone].hometown == zone_table[world[TOROOM(x, y)].zone].hometown) &&                  \
	 (!IS_MARKED(TOROOM(x, y))))

#define VALID_TELEPORT_EDGE(room, dir, orig_room)                                                                                                                                                      \
	(world[room].dir_option[dir] && TOROOM(room, dir) != NOWHERE && TOROOM(room, dir) != orig_room && world[room].zone == world[TOROOM(room, dir)].zone &&                                             \
	 !IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_PRIVATE) && !IS_ROOM(TOROOM(room, dir), ROOM_NO_TELEPORT) && !IS_HOMETOWN(TOROOM(room, dir)) &&                                           \
	 world[TOROOM(room, dir)].sector_type != SECT_MOUNTAIN && world[TOROOM(room, dir)].sector_type != SECT_UNDRWLD_MOUNTAIN && world[TOROOM(room, dir)].sector_type != SECT_OCEAN &&                   \
	 world[TOROOM(room, dir)].sector_type != SECT_CASTLE && world[TOROOM(room, dir)].sector_type != SECT_CASTLE_GATE && world[TOROOM(room, dir)].sector_type != SECT_CASTLE_WALL &&                    \
	 world[room].continent == world[TOROOM(room, dir)].continent)

#define VALID_WAGON_EDGE(x, y)                                                                                                                                                                         \
	(world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && (TO_ROAD(x, y)) && (!IS_SECRET(x, y)) && (!IS_WALLED(x, y)) && (!IS_LOCKED(x, y)) && (!IS_BLOCKED(x, y)) && (!TO_OCEAN(x, y)) &&       \
	 (!IS_MARKED(TOROOM(x, y))))

#define VALID_FLYING_EDGE(x, y)                                                                                                                                                                        \
	(world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && (world[TOROOM(x, y)].dir_option[rev_dir[y]]) && (TOROOM(TOROOM(x, y), rev_dir[y]) == x) &&                                             \
	 (world[(x)].zone == world[TOROOM(x, y)].zone) && (!IS_BLOCKED(x, y)) && (!IS_MARKED(TOROOM(x, y))))

#define VALID_SHIP_EDGE(x, y)                                                                                                                                                                          \
	(world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && (!IS_SECRET(x, y)) && (!IS_LOCKED(x, y)) && (!IS_BLOCKED(x, y)) && (TO_OCEAN(x, y) || TO_DOCKABLE(x, y) || TO_NOSWIM(x, y)) &&         \
	 (!IS_MARKED(TOROOM(x, y))))

#define VALID_DIST_EDGE(x, y) (world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && (!TO_OCEAN(x, y)) && (!IS_MARKED(TOROOM(x, y))))

#define VALID_TRACK_EDGE(x, y)                                                                                                                                                                         \
	(world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && (!IS_SECRET(x, y)) && (!IS_LOCKED(x, y)) && (!IS_BLOCKED(x, y)) && (!TO_OCEAN(x, y)) && (!TO_MOUNTAIN(x, y)) &&                        \
	 (!IS_MARKED(TOROOM(x, y))))

#define VALID_MOB_HUNT_EDGE(x, y)                                                                                                                                                                      \
	(world[(x)].dir_option[(y)] && (TOROOM(x, y) != NOWHERE) && (!IS_SECRET(x, y)) && (!IS_LOCKED(x, y)) && (!IS_BLOCKED(x, y)) && (!TO_OCEAN(x, y)) && (!TO_MOUNTAIN(x, y)) &&                        \
	 (!IS_MARKED(TOROOM(x, y))) && !IS_ROOM(TOROOM(x, y), ROOM_NO_MOB))

bool valid_track_edge(int from_room, int dir);

#define HITCHABLE(o) (IS_OBJ_STAT2((o), ITEM2_LINKABLE))

#include <vector>
using namespace std;

// cpu-hungry BFS path finder
bool find_ship_path(int from_room, int to_room, vector<int> &path);

typedef bool valid_edge_func(int from_room, int dir);
bool         valid_ship_edge(int from_room, int dir);

bool dijkstra(int from_room, int to_room, valid_edge_func *valid_edge, vector<int> &path);

#define MAX_AXIS_INDEX                20
#define RMFR_MAX_RADIUS               33
#define RMFR_DEFAULT_PERCEPTION_CHECK 75

enum RMFR_FLAGS
{
	RMFR_NONE                           = 0,
	RMFR_OUTDOORS_ONLY                  = BIT_1,
	RMFR_INDOORS_ONLY                   = BIT_2,
	RMFR_REQUIRE_PERCEPTION_CHECK       = BIT_3,
	RMFR_CROSS_ZONE_BARRIER             = BIT_4,
	RMFR_INCREASE_PC_DIFF_OVER_DISTANCE = BIT_5,
	RMFR_PASS_DOOR                      = BIT_6,
	RMFR_RADIATE_NORTH                  = BIT_7,
	RMFR_RADIATE_SOUTH                  = BIT_8,
	RMFR_RADIATE_EAST                   = BIT_9,
	RMFR_RADIATE_WEST                   = BIT_10,
	RMFR_RADIATE_NE                     = BIT_11,
	RMFR_RADIATE_SE                     = BIT_12,
	RMFR_RADIATE_SW                     = BIT_13,
	RMFR_RADIATE_NW                     = BIT_14,
	RMFR_RADIATE_UP                     = BIT_15,
	RMFR_RADIATE_DOWN                   = BIT_16,
	RMFR_PASS_WALL                      = BIT_17,
	RMFR_PASS_BLOCKED                   = BIT_18,
	RMFR_RADIATE_ALL_DIRS =
		RMFR_RADIATE_NORTH | RMFR_RADIATE_SOUTH | RMFR_RADIATE_EAST | RMFR_RADIATE_WEST | RMFR_RADIATE_NE | RMFR_RADIATE_SE | RMFR_RADIATE_SW | RMFR_RADIATE_NW | RMFR_RADIATE_UP | RMFR_RADIATE_DOWN
};

enum RMFR_Q_TYPE
{
	RMFR_Q_FIRST = 0,
	RMFR_Q_OTHER
};

extern const int distance_array[MAX_AXIS_INDEX][MAX_AXIS_INDEX][MAX_AXIS_INDEX];

#define IS_OUTSIDE(room) (!IS_ROOM(room, ROOM_INDOORS))
#define IS_INSIDE(room)  (IS_ROOM(room, ROOM_INDOORS))

void radiate_message_from_room(int room, char *message, int radius, RMFR_FLAGS flags, int pcbase);

#endif
