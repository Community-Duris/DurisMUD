/*
 * copyover.h - true copyover (hotboot) support
 * preserves player connections and world state across process replacement
 */

#ifndef _COPYOVER_H_
#define _COPYOVER_H_

#include "structs.h" // for P_char, P_desc, etc

#define COPYOVER_FILE    "copyover.dat"
#define COPYOVER_MAGIC   "COPY"
#define COPYOVER_VERSION 9 // bumped for obj_uid support

// copyover file header
struct copyover_header
{
	char   magic[4];
	int    version;
	time_t timestamp;
	int    num_descriptors;
	int    num_mobs;
	int    num_objects;
	int    num_rooms;
	int    num_combat;
	int    num_zones; // zone age data for crash recovery
};

// zone age for crash recovery (preserves zone timers across restarts)
struct zone_age_entry
{
	int zone_rnum;
	int age;
	int lifespan;
	int fullreset_age;
	int fullreset_lifespan;
};

struct copyover_desc
{
	int    fd;
	char   player_name[50];
	char   host[50];
	char   host2[254];
	::byte term_type;
	int    gmcp_enabled;
	int    out_compress;
	int    room;
	int    mtts_flags;
	int    charset_detected;
	char   ttype_client[64];
	char   ttype_terminal[32];
	int    fighting_type; // 0=none, 1=mob, 2=player
	int    fighting_id;
	char   fighting_name[50];
	int    num_pets;
	int    pet_vnums[10];   // up to 10 pets
	int    pet_hit[10];     // current hp
	int    pet_max_hit[10]; // max hp
};

// mob state for copyover
struct copyover_mob
{
	int  vnum;
	int  idnum; // unique instance id
	int  room;
	int  hit;
	int  max_hit;
	int  mana;
	int  max_mana;
	int  vitality;
	int  max_vitality;
	int  position;
	int  fighting_type;       // 0=none, 1=player, 2=mob
	int  fighting_id;         // player pid or mob idnum
	char fighting_name[50];   // player name if fighting player
	int  num_affects;         // affects saved separately after mob
	int  equipment_vnums[43]; // worn items (max_wear slots)
	int  num_carrying;        // carried items saved after affects
	int  gold;                // mob's gold
};

// affect data for copyover - matches affected_type fields
struct copyover_affect
{
	sh_int         type;
	::byte         wear_off_message_index;
	int            duration;
	uint           flags;
	int            modifier;
	ubyte          location;
	ubyte          loc2;
	unsigned short level;
	unsigned long  bitvector;
	unsigned long  bitvector2;
	unsigned long  bitvector3;
	unsigned long  bitvector4;
	unsigned long  bitvector5;
};

// room state (doors)
struct copyover_room
{
	int vnum;
	int dir;   // which direction (0-5)
	int state; // door flags (open/closed/locked)
};

// combat state
struct copyover_combat
{
	int  attacker_type; // 0=player, 1=mob
	int  attacker_id;
	char attacker_name[50];
	int  target_type;
	int  target_id;
	char target_name[50];
};

// object on ground
struct copyover_obj
{
	unsigned long obj_uid;
	int           vnum;
	int           room;
	int           type;     // item_corpse, etc
	int           value[8]; // matches numb_obj_vals
	time_t        timer[6]; // matches obj_data timer array
	char          name[80]; // corpses have custom names
	char          short_desc[80];
	char          description[160]; // long desc shown in room
	int           num_contents;     // items inside container/corpse
};

// item inside container/corpse or carried by mob
struct copyover_obj_content
{
	unsigned long obj_uid;
	int           vnum;
};

// alias for clarity
typedef struct copyover_obj_content copyover_carried_item;

// copyover state - set during copyover boot
extern int copyover_boot;

// main copyover functions
void copyover_save(int mother_desc, int mother_desc_ssl, int ws_desc);
int  copyover_recover(int *mother_desc, int *mother_desc_ssl, int *ws_desc);
void copyover_restore_combat(void);
int  is_copyover_boot(void);
void copyover_clear_boot(void);

// helper to clear fd_cloexec on accepted sockets
void copyover_prepare_socket(int fd);

// exposed helpers for crash recovery (redis world state saves)
void copyover_count_items(int *num_mobs, int *num_objs, int *num_rooms);
int  copyover_write_mob_to_buffer(P_char mob, char *buf, size_t max_len);
int  copyover_write_obj_to_buffer(P_obj obj, char *buf, size_t max_len);
int  copyover_write_door_to_buffer(int room_rnum, int dir, char *buf, size_t max_len);
int  copyover_write_zone_age_to_buffer(int zone_rnum, char *buf, size_t max_len);

// buffer-based restore helpers
P_char copyover_restore_mob_from_buffer(const char *buf, size_t len, size_t *bytes_read);
P_obj  copyover_restore_obj_from_buffer(const char *buf, size_t len, size_t *bytes_read);
int    copyover_restore_door_from_buffer(const char *buf, size_t len, size_t *bytes_read);
int    copyover_restore_zone_age_from_buffer(const char *buf, size_t len, size_t *bytes_read);

#endif /* _COPYOVER_H_ */
