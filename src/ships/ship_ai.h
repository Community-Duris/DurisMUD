/***************************************
* ship_ai.h
* 
* Header file for ship AI
***************************************/

// Timers
#define AIT_WAIT	0
#define MAXAITIMER	1

// AI Types
#define AI_LINE 	0
#define AI_STOP		1
#define AI_PATH		2

// AI Modes
#define AIM_COMBAT	0
#define AIM_SEEK	1
#define AIM_FOLLOW	2
#define AIM_WAIT	3
#define AIM_AUTOPILOT	4
#define AIM_RAMMING	5

// AI Bits
#define AIB_ENABLED	BIT_1
#define AIB_AUTOPILOT	BIT_2
#define AIB_BATTLER	BIT_3
#define AIB_HUNTER	BIT_4
#define AIB_MOB		BIT_5
#define AIB_RAMMER	BIT_6
#define AIB_DRONE	BIT_7

struct shipai_data
{
  P_ship ship, target;
  int flags, type, timer[MAXAITIMER], t_room, mode;
  struct shipgroup_data *group;
  struct shipai_data *next;
};

struct shipgroup_data
{
  struct shipai_data *leader;
  struct shipai_data *ai;
  struct shipgroup_data *next;
};

struct ShipCombatAI
{
    P_ship ship;
    P_char debug_char;
    int t_bearing;
    int t_arc;
    int your_arc;
    float t_range;
    int t_x, t_y;

    bool out_of_ammo;

    int active_arc[4]; 
    int too_close;
    bool too_far;

    int new_heading;
    int new_safe_speed;
    int cur_safe_speed;

    
    
    ShipCombatAI(P_ship s, P_char ch = 0);

    void activity();
    void send_message_to_debug_char(const char *msg);
    void send_message_to_debug_char(const char *msg, int arg1);
    void send_message_to_debug_char(const char *msg, int arg1, int arg2);
    void send_message_to_debug_char(const char *msg, int arg1, int arg2, int arg3);
    void send_message_to_debug_char(const char *msg, const char* arg1);
    void send_message_to_debug_char(const char *msg, float arg1);
    void send_message_to_debug_char(const char *msg, float arg1, float arg2);
    void send_message_to_debug_char(const char *msg, float arg1, float arg2, float arg3);

    bool find_target();
    void run_away();
    void try_attack();


    bool try_circle_around(int arc);
    bool try_turn_active_weapon();
    bool try_turn_reloading_weapon();
    bool try_make_distance(int distance);
    bool try_chase();
    bool go_around_land();

    bool weapon_ok(int w_num);
    bool check_ammo();
    void check_weapons();
    int check_dir_for_land(int heading, float range);
    int check_dir_for_land_from_target(int heading, float range);
    int check_dir_for_land_from(int heading, float range, float x, float y);
    void set_new_dir();
    int calc_intercept_heading(int h1, int h2);
    int get_safe_speed(int dir);
    int get_arc_main_bearing(int arc);
    int get_room_in_direction_from_target(int dir, float range);
    int get_room_in_direction_from_target(float &x, float &y, int dir, float range);
    int get_room_in_direction_from_ship(int dir, float range);
    int get_room_in_direction_from_ship(float &x, float &y, int dir, float range);
    void set_arc_priority(int current_bearing, int current_arc, int* arc_priority);
    static void normalize_direction(int &dir);
    static bool inside_map(float x, float y);
};
