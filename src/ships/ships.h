#ifndef _SHIPS_H_
#define _SHIPS_H_

#include "defines.h"
#include "structs.h"


#define PANEL_OBJ 50000
#define SHIP_ZONE_START 50001
#define SHIP_ZONE_END 50999
#define MAX_SHIPS 400
#define BUILDTIME 1

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
#define MAXTIMERS         9

#define MAXSHIPS         500
#define MAX_SHIP_ROOM     10
#define SHIPZONE         600

#define COSTPERSAIL        0
#define DEFAULTREPAIR    200
#define BSTATION         180

#define MAXSLOTS          14
#define MAXWEAPON         12
#define NUM_PORTS          9
#define MINCAPFRAG      2000
#define MAXSAIL          250
#define BOARDING_SPEED     9
#define SCAN_RANGE        20

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
#define MAXSHIPCLASS          12

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

#define EVILSHIP          0
#define GOODIESHIP        1
#define UNDEADSHIP        2
#define NPCSHIP           3

#define FORE_BIT          BIT_1
#define PORT_BIT          BIT_2
#define REAR_BIT          BIT_3
#define STAR_BIT          BIT_4

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

// Weapon Flags
#define FORE_ALLOWED     FORE_BIT
#define PORT_ALLOWED     PORT_BIT
#define REAR_ALLOWED     REAR_BIT
#define STAR_ALLOWED     STAR_BIT
#define SHOTGUN          BIT_5
#define CAPITOL          BIT_6
#define MINDBLAST        BIT_7
#define SAILSHOT         BIT_8
#define RANGEDAM         BIT_9
#define BALLISTIC        BIT_10

// Slot Types
#define SLOT_EMPTY      0
#define SLOT_WEAPON     1
#define SLOT_CARGO      2
#define SLOT_CONTRABAND 3
#define SLOT_AMMO       4

// Slot Position
#define SLOT_FORE       SIDE_FORE
#define SLOT_PORT       SIDE_PORT
#define SLOT_REAR       SIDE_REAR
#define SLOT_STAR       SIDE_STAR
#define SLOT_HOLD       SIDE_STAR + 1

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
#define SAIL_CREW           0
#define GUN_CREW            1
#define REPAIR_CREW         2
#define ROWING_CREW         3

#define MAXCREWS            13
#define SAIL_AUTOMATONS      3
#define GUN_AUTOMATONS       8


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
    int _freeweapon;
    int _freecargo;
    int _kind;
};
extern const ShipTypeData ship_type_data[MAXSHIPCLASS];
extern const int ship_allowed_weapons[MAXSHIPCLASS][MAXWEAPON];

// SHIP TYPE DATA MACROS
#define SHIPTYPEID(index) ship_type_data[(index)]._classid
#define SHIPTYPENAME(index) ship_type_data[(index)]._classname
#define SHIPTYPECOST(index) ship_type_data[(index)]._cost
#define SHIPTYPEEPICCOST(index) ship_type_data[(index)]._epiccost
#define SHIPTYPEHULLWEIGHT(index) ship_type_data[(index)]._hull
#define SHIPTYPEMAXWEIGHT(index) ship_type_data[(index)]._mxweight
#define SHIPTYPEMAXSAIL(index) ship_type_data[(index)]._mxsail
#define SHIPTYPESLOTS(index) ship_type_data[(index)]._slots
#define SHIPTYPECARGO(index) ship_type_data[(index)]._mxcargo
#define SHIPTYPECONTRA(index) ship_type_data[(index)]._mxcontra
#define SHIPTYPEPEOPLE(index) ship_type_data[(index)]._mxpeople
#define SHIPTYPESPEED(index) ship_type_data[(index)]._mxspeed
#define SHIPTYPEHDDC(index) ship_type_data[(index)]._headingchangedecrement
#define SHIPTYPESPGAIN(index) ship_type_data[(index)]._speedgain
#define SHIPTYPEMINLEVEL(index) ship_type_data[(index)]._minlevel
#define SHIPTYPEFREEWEAPON(index) ship_type_data[(index)]._freeweapon
#define SHIPTYPEFREECARGO(index) ship_type_data[(index)]._freecargo
#define SHIPTYPEKIND(index) ship_type_data[(index)]._kind

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
    int get_weight() const;
    void show(P_char ch) const;
    char* get_description();
    char* get_status_str();
    const char* get_position_str();
    void clone(const ShipSlot& other);

    char desc[100];
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


struct ShipCrew
{
    void update();
    void reset_stamina();
    void replace_members(float percent);

    int index;
    int skill;
    int stamina;
    int max_stamina;
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
extern const int rowing_crew_list[MAXCREWS];


struct ShipRoom
{
  int roomnum, exit[NUM_EXITS];
};

struct ShipData
{
    void show(P_char ch) const;
    int slot_weight(int type) const;
    int get_maxspeed() const { return maxspeed + maxspeed_bonus; }
    int get_capacity() const { return SHIPTYPEPEOPLE(m_class) + capacity_bonus; }

    int maxarmor[4], armor[4];
    int maxinternal[4], internal[4];
    int mainsail;
    int frags;
    float x, y, z, dx, dy, dz; // coords and their deltas
    struct ShipCrew sailcrew;
    struct ShipCrew guncrew;
    struct ShipCrew repaircrew;
    struct ShipCrew rowingcrew;
    struct ShipSlot slot[MAXSLOTS];  //number of slots
    char *ownername;  //Owner of ship, probably wrong
    char *name, //name of Ship
    *id;  //designation of ship AA-ZZ
    int num; //Ship Number
    int location;  //Current room ship is in might remove and make
    int hashcode;
    int heading;  //current heading
    int setheading; //set heading
    int maxspeed;
    int speed;
    int setspeed; // speeds
    P_obj shipobj, panel;
    int timer[MAXTIMERS];
    int people;
    int bridge, entrance, m_class, anchor, repair;
    struct ShipRoom room[MAX_SHIP_ROOM];
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
    int x,y,z, bearing;
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
    int reload_stamina;
    int volley_time; // pulses for the max range shot
    ulong flags;
};
extern const WeaponData weapon_data[MAXWEAPON];


struct VolleyData
{
    P_ship target;
    P_ship attacker;
    int weapon_index;
    int hit_chance;
};


extern struct ShipMap tactical_map[101][101];
extern struct ContactData contacts[MAXSHIPS];
extern struct ShipFragData shipfrags[10];
extern const char *ship_symbol[NUM_SECT_TYPES];

/* ship variable access macros */

//Armor defines
#define SHIPMAXFARMOR(shipdata) (shipdata)->maxarmor[SIDE_FORE]
#define SHIPMAXRARMOR(shipdata) (shipdata)->maxarmor[SIDE_REAR]
#define SHIPMAXPARMOR(shipdata) (shipdata)->maxarmor[SIDE_PORT]
#define SHIPMAXSARMOR(shipdata) (shipdata)->maxarmor[SIDE_STAR]
#define SHIPARMOR(shipdata, arc) (shipdata)->armor[(arc)]
#define SHIPFARMOR(shipdata) (shipdata)->armor[SIDE_FORE]
#define SHIPRARMOR(shipdata) (shipdata)->armor[SIDE_REAR]
#define SHIPPARMOR(shipdata) (shipdata)->armor[SIDE_PORT]
#define SHIPSARMOR(shipdata) (shipdata)->armor[SIDE_STAR]

//Internal defines
#define SHIPMAXRINTERNAL(shipdata) (shipdata)->maxinternal[SIDE_REAR]
#define SHIPMAXFINTERNAL(shipdata) (shipdata)->maxinternal[SIDE_FORE]
#define SHIPMAXPINTERNAL(shipdata) (shipdata)->maxinternal[SIDE_PORT]
#define SHIPMAXSINTERNAL(shipdata) (shipdata)->maxinternal[SIDE_STAR]
#define SHIPINTERNAL(shipdata,arc) (shipdata)->internal[(arc)]
#define SHIPFINTERNAL(shipdata) (shipdata)->internal[SIDE_FORE]
#define SHIPRINTERNAL(shipdata) (shipdata)->internal[SIDE_REAR]
#define SHIPPINTERNAL(shipdata) (shipdata)->internal[SIDE_PORT]
#define SHIPSINTERNAL(shipdata) (shipdata)->internal[SIDE_STAR]
#define SHIPHULLWEIGHT(shipdata) SHIPTYPEHULLWEIGHT((shipdata)->m_class)
#define SHIPSLOTWEIGHT(shipdata) (shipdata)->slot_weight(-1)
#define SHIPMAXWEIGHT(shipdata)  SHIPTYPEMAXWEIGHT((shipdata)->m_class)
#define SHIPMAXSAIL(shipdata)  SHIPTYPEMAXSAIL((shipdata)->m_class)
#define SHIPAVAILWEIGHT(shipdata) ( SHIPMAXWEIGHT(shipdata) - SHIPSLOTWEIGHT(shipdata) )
#define SHIPFREEWEAPON(shipdata) SHIPTYPEFREEWEAPON((shipdata)->m_class)
#define SHIPFREECARGO(shipdata) SHIPTYPEFREECARGO((shipdata)->m_class)
#define SHIPSAIL(shipdata) (shipdata)->mainsail
#define SHIPWEAPONDAMAGED(ship, i) (ship->slot[i].val2 > 0)
#define SHIPWEAPONDESTROYED(ship, i) (ship->slot[i].val2 >= 100)

//ID stuff
#define SHIPID(shipdata) (shipdata)->id
#define SHIPCLASS(shipdata) (shipdata)->m_class
#define SHIPOWNER(shipdata) (shipdata)->ownername
#define SHIPNAME(shipdata) (shipdata)->name
#define SHIPOBJ(shipdata) (shipdata)->shipobj
#define SHIPROOMNUM(shipdata, num) (shipdata)->room[(num)].roomnum
#define SHIPROOMEXIT(shipdata, num, ext) (shipdata)->room[(num)].exit[(ext)]
#define SHIPCLASSNAME(shipdata) SHIPTYPENAME((shipdata)->m_class)
#define SHIPARMORCOND(maxhp, curhp) condition_prefix((maxhp), (curhp), true)
#define SHIPINTERNALCOND(maxhp, curhp) condition_prefix((maxhp), (curhp), false)
#define SHIPISLOADED(shipdata) IS_SET((shipdata)->flags, LOADED)
#define SHIPSINKING(shipdata) IS_SET((shipdata)->flags, SINKING)
#define SHIPSUNKBYNPC(shipdata) IS_SET((shipdata)->flags, SUNKBYNPC)
#define SHIPIMMOBILE(shipdata) (shipdata->get_maxspeed() == 0)
#define SHIPISDOCKED(shipdata) IS_SET((shipdata)->flags, DOCKED)
#define SHIPANCHORED(shipdata) IS_SET((shipdata)->flags, ANCHOR)
#define ISNPCSHIP(shipdata) ((shipdata)->race == NPCSHIP)
#define ISMERCHANT(shipdata) (SHIPTYPEKIND((shipdata)->m_class) == SHK_MERCHANT)
#define ISWARSHIP(shipdata) (SHIPTYPEKIND((shipdata)->m_class) == SHK_WARSHIP)

//Cargo related stuff
#define SHIPMAXCARGO(shipdata) SHIPTYPECARGO(SHIPCLASS(shipdata))
#define SHIPMAXCONTRA(shipdata) SHIPTYPECONTRA(SHIPCLASS(shipdata))
#define SHIPCARGO(shipdata) ((shipdata)->slot_weight(SLOT_CARGO) / 2)
#define SHIPCONTRA(shipdata) ((shipdata)->slot_weight(SLOT_CONTRABAND) / 2)
#define SHIPAVAILCARGOLOAD(shipdata) MIN( (SHIPMAXCARGO(shipdata) - (SHIPCARGO(shipdata) + SHIPCONTRA(shipdata))), (SHIPAVAILWEIGHT(shipdata) / 2) )
#define SHIPCARGOLOAD(shipdata) (SHIPCARGO(shipdata) + SHIPCONTRA(shipdata))
#define SHIPMAXCARGOLOAD(shipdata) (SHIPCARGOLOAD(shipdata) + SHIPAVAILCARGOLOAD(shipdata))





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
void clear_references_to_ship(P_ship ship);

void set_ship_layout(P_ship ship, int m_class);
void clear_ship_layout(P_ship ship);
void set_ship_physical_layout(P_ship ship);

void set_ship_armor(P_ship ship, bool equal);

void reset_ship(P_ship ship);

int ship_obj_proc(P_obj obj, P_char ch, int cmd, char *arg);
int ship_room_proc(int room, P_char ch, int cmd, char *arg);

void ship_activity();
void dock_ship(P_ship ship, int to_room);
void crash_land(P_ship ship);
void finish_sinking(P_ship ship);
void summon_ship_event(P_char ch, P_char victim, P_obj obj, void *data);

bool check_ship_name(P_ship ship, P_char ch, char* name);
bool check_undocking_conditions(P_ship ship, P_char ch);

void update_shipfrags();
void display_shipfrags(P_char ch);

// shops
int ship_shop_proc(int room, P_char ch, int cmd, char *arg);
int crew_shop_proc(int room, P_char ch, int cmd, char *arg);
int erzul_proc(P_char ch, P_char pl, int cmd, char *arg);

// control
int ship_panel_proc(P_obj obj, P_char ch, int cmd, char *arg);

// autopilot
int  engage_autopilot(P_char ch, P_ship ship, char* arg1, char* arg2);
void stop_autopilot(P_ship ship);
void clear_autopilot(P_ship ship);
void autopilot_activity(P_ship ship);

// combat
int try_ram_ship(P_ship ship, P_ship target, int contact_j);
int weaponsight(P_ship ship, int slot, int t_contact, P_char ch);
int fire_weapon(P_ship ship, int w_num, int t_contact, P_char ch);
void volley_hit_event(P_char ch, P_char victim, P_obj obj, void *data);
void stun_all_in_ship(P_ship ship, int timer);
int damage_sail(P_ship ship, P_ship target, int dam);
int damage_hull(P_ship ship, P_ship target, int dam, int arc, int armor_pierce);
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
int cargo_sell_price(int location);
int cargo_sell_price(int location, bool delayed);
int cargo_buy_price(int location, int type);
int cargo_buy_price(int location, int type, bool delayed);
int contra_sell_price(int location);
int contra_sell_price(int location, int type);
int contra_buy_price(int location, int type);
void adjust_ship_market(int transaction, int location, int type, int volume);
int required_ship_frags_for_contraband(int type);
const char *cargo_type_name(int type);
const char *contra_type_name(int type);
void show_cargo_prices(P_char ch);
void show_contra_prices(P_char ch);
int ship_cargo_info_stick(P_obj obj, P_char ch, int cmd, char *arg);
void do_world_cargo(P_char ch, char *arg);



// utilities
int getmap(P_ship ship);
int getcontacts(P_ship ship, bool limit_range = true);

void setcrew(P_ship ship, int crew_index, int skill);
void update_crew(P_ship ship);
void reset_crew_stamina(P_ship ship);

void set_weapon(P_ship ship, int slot, int w_num, int arc);
void update_maxspeed(P_ship ship);

void assignid(P_ship ship, char *id, bool npc = false);

int bearing(float x1, float y1, float x2, float y2);
float range(float x1, float y1, float z1, float x2, float y2, float z2);
void normalize_direction(int &dir);
int  get_arc(int heading, int bearing);
const char* get_arc_indicator(int arc);
const char* get_arc_name(int arc);
const char* condition_prefix(int maxhp, int curhp, bool light);

P_ship get_ship_from_owner(char *ownername);
P_ship get_ship_from_char(P_char ch);
int anchor_room(int room);
int num_people_in_ship(P_ship ship);
P_char captain_is_aboard(P_ship ship);
int get_turning_speed(P_ship ship);

void act_to_all_in_ship(P_ship ship, const char *msg, ... );
void act_to_outside_ships(P_ship ship, P_ship notarget, const char *msg, ... );
void act_to_outside(P_ship ship, const char *msg, ... );
void everyone_get_out_newship(P_ship ship);
void everyone_look_out_newship(P_ship ship);
bool is_valid_sailing_location(P_ship ship, int room);


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
#define BEG_HASH 23346
#define END_HASH 23402
#define isverge(ship) (ship->hashcode == BEG_HASH || ship->hashcode == END_HASH)

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


#endif // _SHIPS_H_
