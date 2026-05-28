#ifndef _SHIP_NPC_AI_H_
#define _SHIP_NPC_AI_H_

#include "ships.h"

// must be a value that survives float conversion exactly
#define INF_RANGE 65536

enum NPC_AI_Mode
{
	NPC_AI_IDLING,
	NPC_AI_ENGAGING,
	NPC_AI_CRUISING,
	NPC_AI_LEAVING,
	NPC_AI_RUNNING,
	NPC_AI_LOOTING,
};

enum NPC_AI_Type
{
	NPC_AI_PIRATE,
	NPC_AI_HUNTER,
	NPC_AI_ESCORT,
};

enum NPC_AI_Turning
{
	NPC_AI_NOT_TURNING,
	NPC_AI_TURNING_LEFT,
	NPC_AI_TURNING_RIGHT,
};

struct NPCShipAI
{
	P_ship                  ship;
	P_ship                  escort;
	P_char                  debug_char;
	NPC_AI_Type             type;
	struct NPCShipCrewData *crew_data;
	bool                    permanent;
	int                     advanced;

	NPC_AI_Mode    mode;
	NPC_AI_Turning turning;

	// Non-combat actions
	NPCShipAI(P_ship s, P_char ch = 0);
	void activity();
	bool check_for_captain_on_bridge();
	void cruise();
	void reload_and_repair();
	void attacked_by(P_ship attacker);
	void escort_attacked_by(P_ship attacker);
	bool do_escort();
	bool go_around_land(P_ship dest);
	bool try_unload();

	// General combat
	int    t_contact;
	float  t_bearing;
	int    t_arc;
	float  s_bearing;
	int    s_arc;
	float  t_range;
	int    t_x, t_y;
	int    ldp;
	int    contacts_count;
	P_ship did_board;
	bool   is_heavy_ship;
	bool   is_multi_target;
	bool   out_of_ammo;
	float  new_heading;
	int    speed_restriction;

	bool  find_current_target();
	bool  find_new_target();
	void  update_target(int i);
	bool  is_valid_target(P_ship t);
	void  run_away();
	bool  chase();
	float calc_intercept_heading(float h1, float h2);
	bool  check_ammo();
	void  check_for_jettison();
	void  set_new_dir();
	bool  worth_ramming();
	bool  check_ram();
	void  ram_target();
	bool  charge_target(bool for_boarding);
	bool  check_boarding_conditions();
	void  board_target();
	void  steal_target_cargo();
	void  immobile_maneuver();

	// Basic combat
	int  active_arc[4];
	int  too_close;
	bool too_far;

	void basic_combat_maneuver();
	void b_attack();
	bool b_circle_around_arc(int arc);
	bool b_turn_active_weapon();
	bool b_turn_reloading_weapon();
	bool b_make_distance(float distance);
	void b_check_weapons();
	void b_set_arc_priority(float current_bearing, int current_arc, int *arc_priority);

	// Advanced combat
	int   since_last_fired_right;
	float prev_hd;
	float curr_x, curr_y;
	float curr_angle[4];

	float proj_x, proj_y;
	float proj_angle[4];
	float proj_range;
	float proj_sb;
	float proj_tb;

	float hd_change;

	void predict_target(int steps);

	struct SideProperties
	{
		int   ready_timer;
		float damage_ready;
		float max_range;
		float good_range;
		float min_range;
	} side_props[4];
	float min_range_total;

	struct TargetSideProperties
	{
		int   ready_timer;
		float damage_ready;
		float max_range;
		float min_range;
		float land_dist;
	} tside_props[4];
	float t_min_range;
	float t_max_range;

	int   target_side;
	bool  within_target_side;
	float cw_cw, cw_ccw, ccw_cw, ccw_ccw;

	int chosen_side;
	int chosen_rot;

	void advanced_combat_maneuver();
	void a_attack();
	void a_update_side_props();
	void a_update_target_side_props();
	void a_choose_target_side();
	void a_calc_rotations();
	void a_choose_rotation();
	bool a_immediate_turn();
	void a_choose_dest_point();
	void a_apply_chosen_destination();
	void a_predict_target(int steps);

	// Utils
	int   check_dir_for_land_from(float x, float y, float heading, float range);
	int   get_room_in_direction_from(float x, float y, float dir, float range);
	int   get_room_at(float x, float y);
	bool  get_coord_in_direction_from(float x, float y, float dir, float range, float &rx, float &ry);
	float calc_land_dist(float x, float y, float dir, float max_range);
	bool  inside_map(float x, float y);
	void  send_message_to_debug_char(const char *fmt, ...);
};

#endif // _SHIP_NPC_AI_H_
