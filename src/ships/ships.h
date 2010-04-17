/*******************************************************
ships.h
any probs email me at stil@citylinq.com or talk to me as
Foo on the mud
Ship structure and definitions are here :P
Updated with warships. Nov08 -Lucrot
*******************************************************/
#ifndef _SHIPS_H_
#define _SHIPS_H_

#define PANEL_OBJ 50000
#define SHIP_ZONE_START 50001
#define SHIP_ZONE_END 50999
#define MAX_SHIPS 400
#define BUILDTIME 1

#include "defines.h"
#include "structs.h"

/*#ifndef M_PI
#define M_PI   3.14159265358979323846
#endif*/

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

#define MINCONTRAALIGN     1000
#define MINCONTRAFRAGS      100

// defines for adjust_ship_market
#define SOLD_CARGO 1
#define BOUGHT_CARGO 2
#define SOLD_CONTRA 3
#define BOUGHT_CONTRA 4

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

//Inactivity time in seconds
#define NEWSHIP_INACTIVITY 1814400

/*#define MING                0
#define MAXG                1
#define MINS                2
#define MAXS                3
#define RSPEED              4
#define MINFRAG             5*/

#define SAIL_CREW           0
#define GUN_CREW            1
#define REPAIR_CREW         2
#define ROWING_CREW         3

#define FORE                0
#define PORT                1
#define REAR                2
#define STARBOARD           3
                            
#define SHRTRANGE           0
#define MEDRANGE            1
#define LNGRANGE            2
#define MINRANGE            0
#define MAXRANGE            1

#define SH_SLOOP               0
#define SH_YACHT               1
#define SH_CLIPPER             2
#define SH_KETCH               3
#define SH_CARAVEL             4
#define SH_CARRACK             5
#define SH_GALLEON             6
#define SH_CORVETTE            7
#define SH_DESTROYER           8
#define SH_CRUISER             9
#define SH_FRIGATE            10
#define SH_DREADNOUGHT        11

#define MAXSHIPCLASS          12
#define MAXSHIPCLASSMERCHANT   7

// Ship Flags
#define LOADED              BIT_2
#define AIR                 BIT_3
#define SINKING             BIT_4
#define IMMOBILE            BIT_5
#define DOCKED              BIT_6
#define RAMMING             BIT_7
#define MAINTENANCE         BIT_8
#define ANCHOR              BIT_9
#define NEWSHIP_DELETE      BIT_11
#define SQUID_SHIP          BIT_12
#define MOB_SHIP            BIT_13
#define SUMMONED            BIT_14

#define EVILSHIP          0
#define GOODIESHIP        1
#define UNDEADSHIP        2
#define NPCSHIP           3

// Weapon Flags
#define FORE_ALLOWED     BIT_1
#define PORT_ALLOWED     BIT_2
#define REAR_ALLOWED     BIT_3
#define STAR_ALLOWED     BIT_4
#define SHOTGUN          BIT_5
#define CAPITOL          BIT_6
#define MINDBLAST        BIT_7
#define SAILSHOT         BIT_8
#define RANGEDAM         BIT_9


#define W_SMALL_BALLISTA    0
#define W_MEDIUM_BALLISTA   1
#define W_LARGE_BALLISTA    2
#define W_SMALL_CATAPULT    3
#define W_MEDIUM_CATAPULT   4
#define W_LARGE_CATAPULT    5  
#define W_HEAVY_BALLISTA    6
#define W_QUARTZ            7
#define W_DARKSTONE         8
#define W_MINDBLAST         9
#define W_FRAG_CANON       10
#define W_LONGTOM          11


// Slot Types
#define SLOT_EMPTY      0
#define SLOT_WEAPON     1
#define SLOT_CARGO      2
#define SLOT_CONTRABAND 3
#define SLOT_AMMO       4

// Slot Position
#define SLOT_FORE       0
#define SLOT_PORT       1
#define SLOT_REAR       2
#define SLOT_STAR       3
#define SLOT_HOLD       4

#define MAXCARGOPERSLOT 10
#define MAXCONTRAPERSLOT 10

#define WEIGHT_CARGO 2
#define WEIGHT_CONTRABAND 2

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

struct ShipArcProperties
{
    int max_weapon_slots[4];
    int max_weapon_weight[4];
    int armor[4];
    int internal[4];
};
extern const ShipArcProperties ship_arc_properties[MAXSHIPCLASS];
extern const ulong slotmap[4];
extern const char *arc_name[4];

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

struct VolleyData
{
    P_ship target;
    P_ship attacker;
    int weapon_index;
    int hit_chance;
};


#define MAXCREWS            13
#define SAIL_AUTOMATONS      3
#define GUN_AUTOMATONS       8

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
                  //same as Docked
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
    struct shipai_data *shipai;
    struct ShipCombatAI *combat_ai;
    int time;
    int race;
    int pilotlevel;
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

struct CargoData
{
    int base_cost_cargo;
    int base_cost_contra;
    int required_frags;
};

extern const PortData ports[NUM_PORTS];

struct WeaponData
{
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


extern const char *armor_condition_prefix[3];                                                                       
extern const char *internal_condition_prefix[3];
extern struct ShipMap tactical_map[101][101];
extern struct ContactData contacts[MAXSHIPS];
extern struct ShipFragData shipfrags[10];

/* ship variable access macros */



//Armor defines
#define SHIPMAXFARMOR(shipdata) (shipdata)->maxarmor[FORE]
#define SHIPMAXRARMOR(shipdata) (shipdata)->maxarmor[REAR]
#define SHIPMAXPARMOR(shipdata) (shipdata)->maxarmor[PORT]
#define SHIPMAXSARMOR(shipdata) (shipdata)->maxarmor[STARBOARD]
#define SHIPARMOR(shipdata, arc) (shipdata)->armor[(arc)]
#define SHIPFARMOR(shipdata) (shipdata)->armor[FORE]
#define SHIPRARMOR(shipdata) (shipdata)->armor[REAR]
#define SHIPPARMOR(shipdata) (shipdata)->armor[PORT]
#define SHIPSARMOR(shipdata) (shipdata)->armor[STARBOARD]

//Internal defines
#define SHIPMAXRINTERNAL(shipdata) (shipdata)->maxinternal[REAR]
#define SHIPMAXFINTERNAL(shipdata) (shipdata)->maxinternal[FORE]
#define SHIPMAXPINTERNAL(shipdata) (shipdata)->maxinternal[PORT]
#define SHIPMAXSINTERNAL(shipdata) (shipdata)->maxinternal[STARBOARD]
#define SHIPINTERNAL(shipdata,arc) (shipdata)->internal[(arc)]
#define SHIPFINTERNAL(shipdata) (shipdata)->internal[FORE]
#define SHIPRINTERNAL(shipdata) (shipdata)->internal[REAR]
#define SHIPPINTERNAL(shipdata) (shipdata)->internal[PORT]
#define SHIPSINTERNAL(shipdata) (shipdata)->internal[STARBOARD]
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
#define SHIPNUMFROMOBJ(obj) obj->value[0]
#define SHIPOBJ(shipdata) (shipdata)->shipobj
#define SHIPISLOADED(shipdata) IS_SET((shipdata)->flags, LOADED)
//#define SHIPFRAGS(shipdata) (shipdata)->frags
#define CURSHIPLOADED SHIPISLOADED(obj->value[0])
#define SHIPROOMNUM(shipdata, num) (shipdata)->room[(num)].roomnum
#define SHIPROOMEXIT(shipdata, num, ext) (shipdata)->room[(num)].exit[(ext)]
#define SHIPKEYWORDS(shipdata) (shipdata)->keywords
//#define SHIPCAPACITY(shipdata) SHIPTYPEPEOPLE((shipdata)->m_class)
//#define SHIPPEOPLE(shipdata) (shipdata)->people
//#define SHIPTARGET(shipdata) (shipdata)->target
#define SHIPCLASSNAME(shipdata) SHIPTYPENAME((shipdata)->m_class)
#define SHIPARMORCOND(maxhp, curhp) armor_condition_prefix[armorcondition((maxhp),(curhp))]
#define SHIPINTERNALCOND(maxhp,curhp) internal_condition_prefix[armorcondition((maxhp),(curhp))]
#define SHIPSINKING(shipdata) IS_SET((shipdata)->flags, SINKING)
#define SHIPIMMOBILE(shipdata) (shipdata->get_maxspeed() == 0)
#define SHIPISDOCKED(shipdata) IS_SET((shipdata)->flags, DOCKED)
//#define GUNCREWSTAT(shipdata, stat) guncrewstats[(shipdata)->guncrew.type][(stat)]
//#define SAILCREWSTAT(shipdata, stat) sailcrewstats[(shipdata)->sailcrew.type][(stat)]
#define SHIPANCHORED(shipdata) IS_SET((shipdata)->flags, ANCHOR)

//movement related stuff
//#define SHIPX(shipdata) (shipdata)->x
//#define SHIPY(shipdata) (shipdata)->y
//#define SHIPZ(shipdata) (shipdata)->z
//#define SHIPDX(shipdata) (shipdata)->dx
//#define SHIPDY(shipdata) (shipdata)->dy
//#define SHIPDZ(shipdata) (shipdata)->dz
//#define SHIPHEADING(shipdata) (shipdata)->heading
//#define SHIPSETHEADING(shipdata) (shipdata)->setheading
//#define SHIPSPEED(shipdata) ((shipdata)->speed)
//#define SHIPSETSPEED(shipdata) (shipdata)->setspeed
//#define SHIPMAXSPEED(shipdata) (shipdata)->maxspeed
//#define SHIPMAXTURNRATE(shipdata) (shipdata)->maxturnrate
//#define SHIPTURNRATE(shipdata) (shipdata)->turnrate
#define SHIPDOCK(shipdata) (shipdata)->dock
#define SHIPROOM(shipdata) (shipdata)->shiproom
#define SHIPLOCATION(shipdata) (shipdata)->location

//Cargo related stuff
#define SHIPMAXCARGO(shipdata) SHIPTYPECARGO(SHIPCLASS(shipdata))
#define SHIPMAXCONTRA(shipdata) SHIPTYPECONTRA(SHIPCLASS(shipdata))
#define SHIPCARGO(shipdata) ((shipdata)->slot_weight(SLOT_CARGO) / 2)
#define SHIPCONTRA(shipdata) ((shipdata)->slot_weight(SLOT_CONTRABAND) / 2)
#define SHIPAVAILCARGOLOAD(shipdata) MIN( (SHIPMAXCARGO(shipdata) - (SHIPCARGO(shipdata) + SHIPCONTRA(shipdata))), (SHIPAVAILWEIGHT(shipdata) / 2) )
#define SHIPCARGOLOAD(shipdata) (SHIPCARGO(shipdata) + SHIPCONTRA(shipdata))
#define SHIPMAXCARGOLOAD(shipdata) (SHIPCARGOLOAD(shipdata) + SHIPAVAILCARGOLOAD(shipdata))

// Prototyping for ships
void newship_activity();
void cargo_activity();
void volley_hit_event(P_char ch, P_char victim, P_obj obj, void *data);
void initialize_newships();
void shutdown_newships();
void update_crew(P_ship ship);
void reset_crew_stamina(P_ship ship);
bool ship_loose_frags(P_ship target, int frags);
bool ship_gain_frags(P_ship ship, int frags);
void setcrew(P_ship ship, int crew_index, int skill);
void update_ship_status(P_ship ship, P_ship attacker = 0);
int bearing(float x1, float y1, float x2, float y2);

P_ship get_ship(char *ownername);
P_ship getshipfromchar(P_char ch);
bool rename_ship(P_char ch, char *owner_name, char *new_name);
bool rename_ship_owner(char *old_name, char *new_name);

int read_newship();
int write_newship(P_ship ship);

void nameship(const char *name, P_ship ship);
int loadship(P_ship shipdata, int to_room);

struct ShipData *newship(int m_class, bool npc = false);
void delete_ship(P_ship ship);

// shops
int newship_shop(int room, P_char ch, int cmd, char *arg);
int crew_shop(int room, P_char ch, int cmd, char *arg);
int erzul(P_char ch, P_char pl, int cmd, char *arg);

// proc
int newshiproom_proc(int room, P_char ch, int cmd, char *arg);
int fire_arc(P_ship ship, P_char ch, int arc);
int fire_weapon(P_ship ship, P_char ch, int w_num);
void force_anchor(P_ship ship);
int newship_proc(P_obj obj, P_char ch, int cmd, char *arg);
int shiploader_proc(P_obj obj, P_char ch, int cmd, char *arg);
int shipobj_proc(P_obj obj, P_char ch, int cmd, char *arg);

int  bearing(float x1, float y1, float x2, float y2);
int anchor_room(int room);
void newshipfrags();
void crash_land(P_ship ship);

void setarmor(P_ship ship, bool equal);
void setcrew(P_ship ship, int crew_index, int exp);
void set_weapon(P_ship ship, int slot, int w_num, int arc);
void clear_ship_layout(P_ship ship);
void set_ship_layout(P_ship ship, int m_class);
void reset_ship_physical_layout(P_ship ship);
void dock_ship(P_ship ship, int to_room);
void check_contraband(P_ship ship, int to_room);
void update_maxspeed(P_ship ship);


extern void shipai_activity(P_ship ship);
extern void act_to_all_in_ship(P_ship ship, const char *msg);
extern void act_to_outside_ships(P_ship ship, const char *msg, P_ship notarget);
extern void act_to_outside(P_ship ship, const char *msg);
extern void everyone_get_out_newship(P_ship ship);
extern void everyone_look_out_newship(P_ship ship);
extern int  armorcondition(int maxhp, int curhp);
extern void assignid(P_ship ship, char *id, bool npc = false);
extern int  assign_shipai(P_ship ship);
extern int damage_sail(P_ship ship, P_ship target, int dam);
extern int  damage_hull(P_ship ship, P_ship target, int dam, int arc, int armor_pierce);
//extern void dispcontact(int i);
//extern int  getarc(P_ship ship1, int x, int y);
extern int  getarc(int heading, int bearing);
extern int  ybearing(int bearing, int range);
extern int  xbearing(int bearing, int range);
extern int getcontacts(P_ship ship);
extern int getmap(P_ship ship);
extern P_ship getshipfromchar(P_char ch);
extern int num_people_in_ship(P_ship ship);
extern int try_ram_ship(P_ship ship, P_ship target, int contact_j);
extern int pilotroll(P_ship ship);
extern float range(float x1, float y1, float z1, float x2, float y2, float z2);
extern void scantarget(P_ship target, P_char ch);
extern void stun_all_in_ship(P_ship ship, int timer);
extern void summon_ship_event(P_char ch, P_char victim, P_obj obj, void *data);
extern int weaprange(int w_index, char range);
extern int weaponsight(P_ship ship, P_ship target, int weapon, float mod);
extern void calc_crew_adjustments(P_ship ship);
extern bool try_load_pirate_ship(P_ship target, P_char ch);


int sell_cargo(P_char ch, P_ship ship, int slot);
int sell_cargo_slot(P_char ch, P_ship ship, int slot, int rroom);
int sell_contra(P_char ch, P_ship ship, int slot);
int sell_contra_slot(P_char ch, P_ship ship, int slot, int rroom);

void initialize_ship_cargo();
int read_cargo();
int write_cargo();
void reset_cargo();
void update_cargo();
void update_cargo(bool force);
void update_delayed_cargo_prices();
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

// Externals
extern P_index obj_index;
extern P_index mob_index;
extern P_obj object_list;
extern P_room world;
extern P_event current_event;
extern P_event event_list;
extern struct time_info_data time_info;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern const char *dirs[];
extern const int rev_dir[];


#define SHIP_OBJ_TABLE_SIZE 509

class ShipObjHash
{
public:
    ShipObjHash();

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
};
typedef ShipObjHash::visitor ShipVisitor;
extern ShipObjHash shipObjHash;


#endif
