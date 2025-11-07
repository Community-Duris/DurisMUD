#ifndef _SHIPS_H_
#define _SHIPS_H_

#include "defines.h"
#include "structs.h"


#define VOBJ_PANEL          60000
#define VOBJ_ALL_SHIPS      60001
#define VOBJ_CARGO_CRATE    60002

#define VROOM_DAVY_JONES    60000
#define VROOM_SHIP_TRANSIT  60001
// Includes davy jones and transit and ferry (60002).
#define VROOM_SHIPS_START   60000
#define VROOM_SHIPS_END     64999

#define BUILDTIME               1

// Ship status
#define DOCK              0
#define T_UNDOCK          1
#define T_MANEUVER        2
#define T_SINKING         3
#define T_BSTATION        4
#define T_RAM             5
#define T_RAM_WEAPONS     6
#define T_MAINTENANCE     7
#define T_MINDBLAST       8
#define MAXTIMERS        10

#define MAXSHIPS        2000
#define MAX_SHIP_ROOM     15
#define SHIPZONE         600

#define COSTPERSAIL        0
#define DEFAULTREPAIR    200
#define BSTATION         180

#define MAXSLOTS          16
#define NUM_PORTS          9
#define MAXSAIL          250
#define BOARDING_SPEED     9
#define SCAN_RANGE        20
#define DEFAULT_RANGE     35

// Inactivity time in seconds
#define NEWSHIP_INACTIVITY 1814400

#define SIDE_FORE              0
#define SIDE_PORT              1
#define SIDE_REAR              2
#define SIDE_STAR              3

// Ship Types
#define SH_SLOOP               0
#define SH_YACHT               1
#define SH_CLIPPER             2
#define SH_KETCH               3
#define SH_CARAVEL             4
#define SH_CARRACK             5
#define SH_GALLEON             6
#define SH_CORVETTE            7
#define SH_DESTROYER           8
#define SH_FRIGATE             9
#define SH_CRUISER            10
#define SH_DREADNOUGHT        11
#define SH_ZONE_SHIP          12
#define MAXSHIPCLASS          13

#define SHK_MERCHANT          1
#define SHK_WARSHIP           2

// Ship Flags
#define LOADED              BIT_1
#define AIR                 BIT_2
#define SINKING             BIT_3
#define IMMOBILE            BIT_4
#define DOCKED              BIT_5
#define RAMMING             BIT_6
#define ANCHOR              BIT_7
#define TO_DELETE           BIT_8
#define SQUID_SHIP          BIT_9
#define SUMMONED            BIT_10
#define SUNKBYNPC           BIT_11
#define ATTACKBYNPC         BIT_12
#define FLYING              BIT_13

#define UNKNOWNSHIP       0
#define GOODIESHIP        1
#define EVILSHIP          2
#define UNDEADSHIP        3
#define SQUIDSHIP         4
#define NPCSHIP           5

// Weapons
#define W_SMALL_BAL         0
#define W_MEDIUM_BAL        1
#define W_LARGE_BAL         2
#define W_SMALL_CAT         3
#define W_MEDIUM_CAT        4
#define W_LARGE_CAT         5  
#define W_HEAVY_BAL         6
#define W_LIGHT_BEAM        7
#define W_HEAVY_BEAM        8
#define W_MINDBLAST         9
#define W_FRAG_CAN         10
#define W_LONGTOM          11
#define MAXWEAPON          12

#define FORE_BIT          BIT_1
#define PORT_BIT          BIT_2
#define REAR_BIT          BIT_3
#define STAR_BIT          BIT_4

// Weapon Flags
#define FORE_ALLOWED     FORE_BIT
#define PORT_ALLOWED     PORT_BIT
#define REAR_ALLOWED     REAR_BIT
#define STAR_ALLOWED     STAR_BIT
#define CAPITAL          BIT_5
#define MINDBLAST        BIT_6
#define RANGEDAM         BIT_7
#define BALLISTIC        BIT_8
#define DIPLOMAT	       BIT_9

// Equipment
#define E_RAM            0
#define E_LEVISTONE      1
#define E_DIPLOMAT	    2
#define MAXEQUIPMENT     3

#define LEVISTONE_TIME     60
#define LEVISTONE_RECHARGE 600

// Slot Types
#define SLOT_EMPTY      0
#define SLOT_WEAPON     1
#define SLOT_CARGO      2
#define SLOT_CONTRABAND 3
#define SLOT_EQUIPMENT  4
#define SLOT_AMMO       5

// Slot Position
#define SLOT_FORE       SIDE_FORE
#define SLOT_PORT       SIDE_PORT
#define SLOT_REAR       SIDE_REAR
#define SLOT_STAR       SIDE_STAR
#define SLOT_HOLD       SLOT_STAR + 1
#define SLOT_EQUI       SLOT_STAR + 2

// Cargo
#define MAXCARGOPERSLOT 10
#define MAXCONTRAPERSLOT 10

#define WEIGHT_CARGO 2
#define WEIGHT_CONTRABAND 2

#define MINCONTRAALIGN     1000
#define MINCONTRAFRAGS      100

// defines for adjust_ship_market
#define SOLD_CARGO 1
#define BOUGHT_CARGO 2
#define SOLD_CONTRA 3
#define BOUGHT_CONTRA 4


// Crews
#define NO_CHIEF            0
#define SAIL_CHIEF          1
#define GUNS_CHIEF          2
#define RPAR_CHIEF          3

#define MAXCREWS            25
#define MAXCHIEFS           13
#define DEFAULT_CREW         0
#define AUTOMATON_CREW       1


// Crew Flags
#define CF_NONE              0
#define CF_SCOUT_RANGE_1     BIT_1
#define CF_SCOUT_RANGE_2     BIT_2
#define CF_MAXSPEED_1        BIT_3
#define CF_MAXSPEED_2        BIT_4
#define CF_MAXCARGO_10       BIT_5
#define CF_HULL_REPAIR_2     BIT_6
#define CF_HULL_REPAIR_3     BIT_7
#define CF_WEAPONS_REPAIR_2  BIT_8
#define CF_WEAPONS_REPAIR_3  BIT_9
#define CF_SAIL_REPAIR_2     BIT_10
#define CF_SAIL_REPAIR_3     BIT_11

// Crew Chief Flags
#define CCF_NONE             0


typedef struct ShipData *P_ship;

struct ShipTypeData
{
    int _classid;
    const char *_classname;
    int _cost;
    int _epiccost;
    int _hull;
    int _slots;
    int _mxweight;
    int _mxsail;
    int _mxcargo;
    int _mxcontra;
    int _mxpeople;
    int _mxspeed;
    int _headingchangedecrement;
    int _speedgain;
    int _minlevel;
    int _freeequip;
    int _freecargo;
    int _kind;

    float get_hull_mod() const;
};
extern const ShipTypeData ship_type_data[MAXSHIPCLASS];
extern const int ship_allowed_weapons[MAXSHIPCLASS][MAXWEAPON];
extern const int ship_allowed_equipment[MAXSHIPCLASS][MAXEQUIPMENT];

// SHIP TYPE DATA MACROS
#define SHIPTYPE_ID(index) ship_type_data[(index)]._classid
#define SHIPTYPE_NAME(index) ship_type_data[(index)]._classname
#define SHIPTYPE_COST(index) ship_type_data[(index)]._cost
#define SHIPTYPE_EPIC_COST(index) ship_type_data[(index)]._epiccost
#define SHIPTYPE_HULL_WEIGHT(index) ship_type_data[(index)]._hull
#define SHIPTYPE_HULL_MOD(index) ship_type_data[(index)].get_hull_mod()
#define SHIPTYPE_MAX_WEIGHT(index) ship_type_data[(index)]._mxweight
#define SHIPTYPE_MAX_SAIL(index) ship_type_data[(index)]._mxsail
#define SHIPTYPE_SLOTS(index) ship_type_data[(index)]._slots
#define SHIPTYPE_CARGO(index) ship_type_data[(index)]._mxcargo
#define SHIPTYPE_CONTRA(index) ship_type_data[(index)]._mxcontra
#define SHIPTYPE_PEOPLE(index) ship_type_data[(index)]._mxpeople
#define SHIPTYPE_SPEED(index) ship_type_data[(index)]._mxspeed
#define SHIPTYPE_HDDC(index) ship_type_data[(index)]._headingchangedecrement
#define SHIPTYPE_SPEED_GAIN(index) ship_type_data[(index)]._speedgain
#define SHIPTYPE_MIN_LEVEL(index) ship_type_data[(index)]._minlevel
#define SHIPTYPE_FREE_EQUIPMENT(index) ship_type_data[(index)]._freeequip
#define SHIPTYPE_FREE_CARGO(index) ship_type_data[(index)]._freecargo
#define SHIPTYPE_KIND(index) ship_type_data[(index)]._kind

struct ShipArcProperties
{
    int max_weapon_slots[4];
    int max_weapon_weight[4];
    int armor[4];
    int internal[4];
};
extern const ShipArcProperties ship_arc_properties[MAXSHIPCLASS];
extern const ulong arcbitmap[4];

struct ShipSlot
{
    void clear();
    int get_weight(const ShipData* ship) const;
    void show(P_char ch, const ShipData* ship) const;
    char* get_description();
    char* get_status_str();
    const char* get_position_str();
    void clone(const ShipSlot& other);

    char desc[50];
    char status[20];
  
    int type;      // type of slot
    int index;     // weapon index, ammo index, cargo index
    int position;  // Position of slot F P R S Hold

    // weapons only for now
    int timer; // reloading timer

               //   Cargo         Weapon      Ammo
    int val0; // count           ammo type   count
    int val1; // invoice price   ammo count  max count
    int val2; //                 damaged
    int val3; //                 
    int val4; //
};


/*struct ShipCrew
{
    void update();
    void reset_stamina();
    void replace_members(float percent);
    float get_stamina_mod();
    int get_display_stamina();
    const char* get_stamina_prefix();

    int index;
    int skill;
    float stamina;
    float max_stamina;
    float skill_mod;
};

struct ShipCrewData
{
    int type;
    const char *name;  //Crew desc
    int start_skill;
    int min_skill;
    int skill_gain;
    int base_stamina;
    int hire_cost;
    int min_frags;
};
extern const ShipCrewData ship_crew_data[MAXCREWS];
extern const int sail_crew_list[MAXCREWS];
extern const int gun_crew_list[MAXCREWS];
extern const int repair_crew_list[MAXCREWS];
extern const int rowing_crew_list[MAXCREWS];*/

struct ShipCrew
{

    int index;
    float sail_skill;
    float guns_skill;
    float rpar_skill;
    float stamina;
    
    float max_stamina;
    float sail_mod_applied;
    float guns_mod_applied;
    float rpar_mod_applied;

    int sail_chief;
    int guns_chief;
    int rpar_chief;

    void update();
    void reset_stamina();
    void replace_members(float percent);

    float get_stamina_mod();
    int get_display_stamina();
    const char* get_stamina_prefix();

    void sail_skill_raise(float raise);
    void guns_skill_raise(float raise);
    void rpar_skill_raise(float raise);
    void skill_raise(float raise, float& skill, int chief);
    void reduce_stamina(float val, P_ship ship);

    int sail_mod();
    int guns_mod();
    int rpar_mod();

    int get_contact_range_mod() const;
    int get_sail_repair_mod() const;
    int get_weapon_repair_mod() const;
    int get_hull_repair_mod() const;
    int get_maxspeed_mod() const;
    float get_maxcargo_mod() const;
};

struct ShipCrewData
{
    const char *name;  //Crew desc

    int level;

    int base_sail_skill;
    int base_guns_skill;
    int base_rpar_skill;
    int base_stamina;

    int sail_mod;
    int guns_mod;
    int rpar_mod;

    int hire_cost;
    int hire_frags;
    int hire_rooms[5];

    ulong flags;

    bool hire_room(int room) const;
    const char* get_next_bonus(int *cur) const;
    const char* get_bonus_string(ulong flag) const;
};

struct ShipChiefData
{
    int type;
    const char* name;

    int min_skill;
    int skill_gain_bonus;
    int skill_mod;

    int hire_cost;
    int hire_frags;
    int hire_rooms[5];

    ulong flags;

    bool hire_room(int room) const;
    const char* get_spec() const;
};


extern const ShipCrewData ship_crew_data[MAXCREWS];
extern const ShipChiefData ship_chief_data[MAXCHIEFS];


struct ShipRoom
{
  int roomnum, exit[NUM_EXITS];
};

struct ShipData
{
    void show(P_char ch) const;
    void guns_skill_raise(P_char, float);
    void sail_skill_raise(P_char, float);
    void rpar_skill_raise(P_char, float);
    int slot_weight(int type) const;
    int get_maxspeed() const { return maxspeed + maxspeed_bonus; }
    int get_capacity() const { return SHIPTYPE_PEOPLE(m_class) + capacity_bonus; }
    int has_capital(); // returns which type of capital item (SLOT_WEAPON/SLOT_EQUIPMENT) or SLOT_EMPTY if none found.
    bool buy_check_capital( P_char ch ); // Checks for capital item and sends message to ch if it does.

    int maxarmor[4], armor[4];
    int maxinternal[4], internal[4];
    int mainsail;
    int frags;
    float x, y, z, dx, dy, dz; // coords and their deltas
    ShipCrew crew;
    //struct ShipCrew sailcrew;
    //struct ShipCrew guncrew;
    //struct ShipCrew repaircrew;
    //struct ShipCrew rowingcrew;
    struct ShipSlot slot[MAXSLOTS];  //number of slots
    char *ownername;  //Owner of ship, probably wrong
    char *name, //name of Ship
    *id;  //designation of ship AA-ZZ
    int num; //Ship Number
    int location;  //Current room ship is in might remove and make
    float heading;  //current heading
    float setheading; //set heading
    int maxspeed;
    int speed;
    int setspeed; // speeds
    P_obj shipobj, panel;
    int timer[MAXTIMERS];
    int people;
    int bridge, entrance, m_class, anchor, repair;
    struct ShipRoom room[MAX_SHIP_ROOM];
    int room_count;
    char *keywords;
    ulong flags;
    struct ShipData *next, *target;
    struct shipai_data *autopilot;
    struct NPCShipAI *npc_ai;
    int time;
    int race;
    int money;

    int maxspeed_bonus;
    int capacity_bonus;
};

struct ContactData
{
    int x,y,z; 
    float bearing;
    float range;
    struct ShipData *ship;
    char arc[10];
};

struct ShipMap
{
    char map[10];
    int rroom,vroom;
};

struct ShipFragData
{
    struct ShipData *ship;
};

struct PortData
{
  int loc_room;
  const char * loc_name;
  int ocean_map_room;
};
extern const PortData ports[NUM_PORTS];

struct CargoData
{
    int base_cost_cargo;
    int base_cost_contra;
    int required_frags;
};


struct WeaponData
{
    float average_hull_damage() const;
    float average_sail_damage() const;

    const char* name;
    int cost;
    int min_frags;
    int min_crewexp;
    int weight;
    int ammo;
    int min_range;
    int max_range;
    int min_damage;
    int max_damage;
    int fragments;    // fragments per volley
    int hit_arc;      // hit area width in degrees
    int sail_hit;     // chance to hit sails
    int hull_dam;     // damage modifier to hull
    int sail_dam;     // damage modifier to sails
    int armor_pierce; // chances for critical hit
    int reload_time;
    int volley_time; // pulses for the max range shot
    ulong flags;
};
extern const WeaponData weapon_data[MAXWEAPON];

struct EquipmentData
{
    const char* name;
    int cost;
    int min_frags;
    int weight;
    ulong flags;
};
extern const EquipmentData equipment_data[MAXEQUIPMENT];


struct VolleyData
{
    P_ship target;
    P_ship attacker;
    int weapon_index;
    int hit_chance;
};


extern struct ShipMap tactical_map[101][101];
extern struct ContactData contacts[MAXSHIPS];
extern struct ShipFragData shipfrags[20];
extern const char *ship_symbol[NUM_SECT_TYPES];

/* ship variable access macros */

//Armor defines
#define SHIP_MAX_FARMOR(shipdata) (shipdata)->maxarmor[SIDE_FORE]
#define SHIP_MAX_RARMOR(shipdata) (shipdata)->maxarmor[SIDE_REAR]
#define SHIP_MAX_PARMOR(shipdata) (shipdata)->maxarmor[SIDE_PORT]
#define SHIP_MAX_SARMOR(shipdata) (shipdata)->maxarmor[SIDE_STAR]
#define SHIP_ARMOR(shipdata, arc) (shipdata)->armor[(arc)]
#define SHIP_FARMOR(shipdata) (shipdata)->armor[SIDE_FORE]
#define SHIP_RARMOR(shipdata) (shipdata)->armor[SIDE_REAR]
#define SHIP_PARMOR(shipdata) (shipdata)->armor[SIDE_PORT]
#define SHIP_SARMOR(shipdata) (shipdata)->armor[SIDE_STAR]

//Internal defines
#define SHIP_MAX_RINTERNAL(shipdata) (shipdata)->maxinternal[SIDE_REAR]
#define SHIP_MAX_FINTERNAL(shipdata) (shipdata)->maxinternal[SIDE_FORE]
#define SHIP_MAX_PINTERNAL(shipdata) (shipdata)->maxinternal[SIDE_PORT]
#define SHIP_MAX_SINTERNAL(shipdata) (shipdata)->maxinternal[SIDE_STAR]
#define SHIP_INTERNAL(shipdata,arc) (shipdata)->internal[(arc)]
#define SHIP_FINTERNAL(shipdata) (shipdata)->internal[SIDE_FORE]
#define SHIP_RINTERNAL(shipdata) (shipdata)->internal[SIDE_REAR]
#define SHIP_PINTERNAL(shipdata) (shipdata)->internal[SIDE_PORT]
#define SHIP_SINTERNAL(shipdata) (shipdata)->internal[SIDE_STAR]
#define SHIP_HULL_WEIGHT(shipdata) SHIPTYPE_HULL_WEIGHT(SHIP_CLASS(shipdata))
#define SHIP_HULL_MOD(shipdata) SHIPTYPE_HULL_MOD(SHIP_CLASS(shipdata))
#define SHIP_SLOT_WEIGHT(shipdata) (shipdata)->slot_weight(-1)
#define SHIP_MAX_WEIGHT(shipdata)  SHIPTYPE_MAX_WEIGHT(SHIP_CLASS(shipdata))
#define SHIP_MAX_SAIL(shipdata)  SHIPTYPE_MAX_SAIL(SHIP_CLASS(shipdata))
#define SHIP_AVAIL_WEIGHT(shipdata) ( SHIP_MAX_WEIGHT(shipdata) - SHIP_SLOT_WEIGHT(shipdata) )
#define SHIP_FREE_EQUIPMENT(shipdata) SHIPTYPE_FREE_EQUIPMENT(SHIP_CLASS(shipdata))
#define SHIP_FREE_CARGO(shipdata) SHIPTYPE_FREE_CARGO(SHIP_CLASS(shipdata))
#define SHIP_SAIL(shipdata) (shipdata)->mainsail
#define SHIP_WEAPON_DAMAGED(ship, i) (ship->slot[i].val2 > 0)
#define SHIP_WEAPON_DESTROYED(ship, i) (ship->slot[i].val2 >= 100)
#define SHIP_HDDC(shipdata) (SHIPTYPE_HDDC(SHIP_CLASS(shipdata)))
#define SHIP_ACCEL(shipdata) (SHIPTYPE_SPEED_GAIN(SHIP_CLASS(shipdata)))

//ID stuff
#define SHIP_ID(shipdata) (shipdata)->id
#define SHIP_CLASS(shipdata) (shipdata)->m_class
#define SHIP_OWNER(shipdata) (shipdata)->ownername
#define SHIP_NAME(shipdata) (shipdata)->name
#define SHIP_OBJ(shipdata) (shipdata)->shipobj
#define SHIP_ROOM_NUM(shipdata, num) (shipdata)->room[(num)].roomnum
#define SHIP_ROOM_EXIT(shipdata, num, ext) (shipdata)->room[(num)].exit[(ext)]
#define SHIP_CLASS_NAME(shipdata) SHIPTYPE_NAME((shipdata)->m_class)
#define SHIP_ARMOR_COND(maxhp, curhp) condition_prefix((maxhp), (curhp), true)
#define SHIP_INTERNAL_COND(maxhp, curhp) condition_prefix((maxhp), (curhp), false)
#define SHIP_LOADED(shipdata) IS_SET((shipdata)->flags, LOADED)
#define SHIP_SINKING(shipdata) IS_SET((shipdata)->flags, SINKING)
#define SHIP_SUNK_BY_NPC(shipdata) IS_SET((shipdata)->flags, SUNKBYNPC)
#define SHIP_IMMOBILE(shipdata) (shipdata->get_maxspeed() == 0)
#define SHIP_DOCKED(shipdata) IS_SET((shipdata)->flags, DOCKED)
#define SHIP_ANCHORED(shipdata) IS_SET((shipdata)->flags, ANCHOR)
#define SHIP_FLYING(shipdata) IS_SET((shipdata)->flags, FLYING)
#define IS_NPC_SHIP(shipdata) ((shipdata)->race == NPCSHIP)
#define IS_MERCHANT(shipdata) (SHIPTYPE_KIND((shipdata)->m_class) == SHK_MERCHANT)
#define IS_WARSHIP(shipdata) (SHIPTYPE_KIND((shipdata)->m_class) == SHK_WARSHIP)
#define HAS_VALID_TARGET(shipdata) ((shipdata->target != 0) && (!SHIP_SINKING(shipdata->target)) && (shipdata->target->race != shipdata->race))

//Cargo related stuff
#define SHIP_MAX_CARGO(shipdata) ((int)(SHIPTYPE_CARGO(SHIP_CLASS(shipdata)) * shipdata->crew.get_maxcargo_mod()))
#define SHIP_MAX_CONTRA(shipdata) SHIPTYPE_CONTRA(SHIP_CLASS(shipdata))
#define SHIP_CARGO(shipdata) ((shipdata)->slot_weight(SLOT_CARGO) / 2)
#define SHIP_CONTRA(shipdata) ((shipdata)->slot_weight(SLOT_CONTRABAND) / 2)
#define SHIP_CARGO_LOAD(shipdata) (SHIP_CARGO(shipdata) + SHIP_CONTRA(shipdata))
#define SHIP_AVAIL_CARGO_LOAD(shipdata) MIN( (SHIP_MAX_CARGO(shipdata) - SHIP_CARGO_LOAD(shipdata)), (SHIP_AVAIL_WEIGHT(shipdata) / 2) )
#define SHIP_MAX_CARGO_LOAD(shipdata) (SHIP_CARGO_LOAD(shipdata) + SHIP_AVAIL_CARGO_LOAD(shipdata))
#define SHIP_AVAIL_CARGO_SALVAGE(shipdata) \
        MIN( ((IS_MERCHANT(ship) ? SHIP_MAX_CARGO(ship) : (SHIP_MAX_WEIGHT(ship) / 5)) - SHIP_CARGO_LOAD(shipdata)), \
             (SHIP_AVAIL_WEIGHT(shipdata) / 2) )
#define SHIP_MAX_CARGO_SALVAGE(shipdata) (SHIP_CARGO_LOAD(shipdata) + SHIP_AVAIL_CARGO_SALVAGE(shipdata))




//////////////////////
// Prototypes
//////////////////////
void initialize_ships();
void shutdown_ships();

int write_ships_index();
int write_ship(P_ship ship);
int read_ships();

struct ShipData *new_ship(int m_class, bool npc = false);
void name_ship(const char *name, P_ship ship);
bool rename_ship(P_char ch, char *owner_name, char *new_name);
bool rename_ship_owner(char *old_name, char *new_name);
int load_ship(P_ship shipdata, int to_room);

void delete_ship(P_ship ship, bool npc = false);
void delete_ship(char *owner_name);
void clear_references_to_ship(P_ship ship);

void init_ship_layout(P_ship ship);
void clear_ship_layout(P_ship ship);
void set_ship_layout(P_ship ship, int m_class);
bool set_ship_physical_layout(P_ship ship);

void set_ship_armor(P_ship ship, bool equal);

void reset_ship(P_ship ship, bool clear_slots = true);

int ship_obj_proc(P_obj obj, P_char ch, int cmd, char *arg);
int ship_room_proc(int room, P_char ch, int cmd, char *arg);

void ship_activity();
void dock_ship(P_ship ship, int to_room);
void crash_land(P_ship ship);
void finish_sinking(P_ship ship);
void summon_ship_event(P_char ch, P_char victim, P_obj obj, void *data);
void fly_ship(P_ship ship);
void land_ship(P_ship ship);

bool check_ship_name(P_ship ship, P_char ch, char* name);
bool check_undocking_conditions(P_ship ship, int m_class, P_char ch);

void update_shipfrags();
void display_shipfrags(P_char ch);

// shops
int ship_shop_proc(int room, P_char ch, int cmd, char *arg);
int crew_shop_proc(int room, P_char ch, int cmd, char *arg);

// control
int ship_panel_proc(P_obj obj, P_char ch, int cmd, char *arg);

// autopilot
int  engage_autopilot(P_char ch, P_ship ship, char* arg1, char* arg2);
void stop_autopilot(P_ship ship);
void clear_autopilot(P_ship ship);
void autopilot_activity(P_ship ship);

// combat
int try_ram_ship(P_ship ship, P_ship target, float t_bearing);
int weaponsight(P_ship ship, int slot, int t_contact, P_char ch);
int fire_weapon(P_ship ship, int w_num, int t_contact, P_char ch);
int fire_weapon(P_ship ship, int w_num, int t_contact, int hit_chance, P_char ch);
void volley_hit_event(P_char ch, P_char victim, P_obj obj, void *data);
void stun_all_in_ship(P_ship ship, int timer);
int damage_sail(P_ship ship, P_ship target, int dam);
int damage_hull(P_ship ship, P_ship target, int dam, int arc, int armor_pierce);
int ch_damage_hull(P_char ship, P_ship target, int dam, int arc, int armor_pierce);
void damage_weapon(P_ship ship, P_ship target, int arc, int dam);
void force_anchor(P_ship ship);
bool ship_gain_frags(P_ship ship, P_ship target, int frags);
bool ship_loss_on_sink(P_ship target, P_ship attacker, int frags);
void update_ship_status(P_ship ship, P_ship attacker = 0);
void scan_target(P_ship ship, P_ship target, P_char ch);

// cargo
void cargo_activity();
void check_contraband(P_ship ship, int to_room);
void initialize_ship_cargo();
int read_cargo();
int write_cargo();
int cargo_sell_price(int location, bool delayed = false);
int cargo_buy_price(int location, int type, bool delayed = false);
int contra_sell_price(int location);
int contra_sell_price(int location, int type);
int contra_buy_price(int location, int type);
void adjust_ship_market(int transaction, int location, int type, int volume);
bool can_buy_contraband(P_ship ship, int type);
const char *cargo_type_name(int type);
const char *contra_type_name(int type);
void show_cargo_prices(P_char ch);
void show_contra_prices(P_char ch);
int ship_cargo_info_stick(P_obj obj, P_char ch, int cmd, char *arg);
void do_world_cargo(P_char ch, char *arg);
int salvage_cargo(P_char ch, P_ship ship, int crates);
int jettison_cargo(P_char ch, P_ship ship, int crates);
int jettison_contraband(P_char ch, P_ship ship, int crates);
void jettison_all(P_ship ship);



// utilities
bool getmap(P_ship ship, bool limit_range = FALSE);
int getcontacts(P_ship ship, bool limit_range = TRUE);

void change_crew(P_ship ship, int crew_index, bool skill_drop);
void set_crew(P_ship ship, int crew_index, bool reset_skills = true);
void set_chief(P_ship ship, int chief_index);
void update_crew(P_ship ship);
void reset_crew_stamina(P_ship ship);
char *crew_bonuses( const ShipCrewData crew );

void set_weapon(P_ship ship, int slot, int w_num, int arc);
void set_equipment(P_ship ship, int slot, int w_num);
void update_maxspeed(P_ship ship, int breach_count);
int get_maxspeed_without_cargo(P_ship ship);

void assignid(P_ship ship, char *id, bool npc = false);

float bearing(float x1, float y1, float x2, float y2);
float range(float x1, float y1, float z1, float x2, float y2, float z2);
void normalize_direction(float &dir);
int  get_arc(float heading, float bearing);
const char* get_arc_indicator(int arc);
const char* get_arc_name(int arc);
const char* condition_prefix(int maxhp, int curhp, bool light);

P_ship get_ship_from_owner(char *ownername);
P_ship get_ship_from_char(P_char ch);
int anchor_room(int room);
int num_people_in_ship(P_ship ship);
void clear_cargo(P_ship ship);
int calculate_full_cost(P_ship ship);
P_char captain_is_aboard(P_ship ship);
bool pc_is_aboard(P_ship ship);
float get_turning_speed(P_ship ship);
float get_next_heading_change(P_ship ship);
int get_acceleration(P_ship ship);
int get_next_speed_change(P_ship ship);

void act_to_all_in_ship_f(P_ship ship, const char *msg, ... );
void act_to_all_in_ship(P_ship ship, const char *msg, P_char victim);
void act_to_all_in_ship(P_ship ship, const char *msg);
void act_to_outside_ships(P_ship ship, P_ship notarget, int range, const char *msg, ... );
void act_to_outside(P_ship ship, int range, const char *msg, ... );
void kick_everyone_off(P_ship ship);
void clear_ship_content(P_ship ship);
void look_out_ship(P_ship ship, P_char ch);
void everyone_look_out_ship(P_ship ship);
void set_pvp_on_passengers(P_ship ship);
bool is_valid_sailing_location(P_ship ship, int room);

bool has_eq_ram(const ShipData* ship);
int eq_ram_slot(const ShipData* ship);
int eq_ram_damage(const ShipData* ship);
int eq_ram_weight(const ShipData* ship);
int eq_ram_cost(const ShipData* ship);

bool has_eq_levistone(const ShipData* ship);
int eq_levistone_slot(const ShipData* ship);
int eq_levistone_weight(const ShipData* ship);

bool is_diplomat_slot( const ShipData *ship, int slot );
bool has_eq_diplomat(const ShipData *ship);
int eq_diplomat_slot(const ShipData *ship);
int eq_diplomat_weight(const ShipData *ship);

bool ocean_pvp_state();

// Externals
extern P_index obj_index;
extern P_index mob_index;
extern P_room world;
extern struct zone_data *zone_table;
extern const char *dirs[];
extern const int rev_dir[];
extern int shiperror;


// Ship object hash
#define SHIP_OBJ_TABLE_SIZE 509
#define ABS(x) ((x) < 0 ? -(x) : (x)) // to go around mainbox issue with math library

class ShipObjHash
{
  public:
    ShipObjHash();
    int size() { return sz; }

    P_ship find(P_obj key);
    bool add(P_ship ship);

    bool erase(P_ship ship);
    bool erase(P_ship ship, unsigned t_index);
    struct visitor
    {
      public:
        operator P_ship ()  { return curr; }
        P_ship operator -> ()  { return curr; }
        P_ship get_value() { return curr; }

        unsigned t_index;
        P_ship curr;
    };
    bool get_first(visitor& vs);
    bool get_next(visitor& vs);
    bool erase(visitor& vs);

  private:
    P_ship table[SHIP_OBJ_TABLE_SIZE];
    int sz;
};
typedef ShipObjHash::visitor ShipVisitor;
extern ShipObjHash shipObjHash;



#define AUTOMATONS_MOONSTONE          12001
#define AUTOMATONS_MOONSTONE_CORE     12029
#define AUTOMATONS_MOONSTONE_FRAGMENT 12028
//int moonstone_fragment(P_obj obj, P_char ch, int cmd, char *argument);
//int erzul_proc(P_char ch, P_char pl, int cmd, char *arg);
bool load_moonstone_fragments();

#endif // _SHIPS_H_
