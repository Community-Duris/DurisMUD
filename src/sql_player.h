// sql_player.h
// player save/load functions for mysql storage
// part of pfile-to-db migration

#ifndef __SQL_PLAYER_H_INCLUDED__
#define __SQL_PLAYER_H_INCLUDED__

#include "structs.h"

#ifndef __NO_MYSQL__
#include <mysql.h>

// ============================================================================
// transaction helpers
// ============================================================================

// start a transaction, returns true on success
bool sql_begin_transaction(void);

// commit current transaction, returns true on success
bool sql_commit(void);

// rollback current transaction, returns true on success
bool sql_rollback(void);

// check if we're currently in a transaction
bool sql_in_transaction(void);

// for forked child - create separate db connection
MYSQL *sql_create_child_connection(void);

// child swaps globals after fork
void sql_reset_for_child(MYSQL *child_conn);

// ============================================================================
// player save functions
// ============================================================================

// master save function - saves entire player to db atomically
// type: save type (RENT_CAMPED, RENT_RENTED, etc from defines.h)
// room: room vnum to save
// returns true on success
bool sql_save_player(P_char ch, int type, int room);

// individual save functions (called by sql_save_player)
bool sql_save_player_status(P_char ch, int type, int room);
bool sql_save_player_skills(P_char ch);
bool sql_save_player_affects(P_char ch);
bool sql_save_player_items(P_char ch);
bool sql_delete_player_items(int pid);
bool sql_save_player_witnesses(P_char ch);
bool sql_save_player_shapechanges(P_char ch);
bool sql_save_player_recipes(P_char ch);
bool sql_add_player_recipe(int pid, int recipe_vnum);
bool sql_delete_player_recipes(int pid);
bool sql_has_player_recipe(int pid, int recipe_vnum);
int *sql_get_player_recipes(int pid, int *count);

// ============================================================================
// player load functions
// ============================================================================

// master load function - loads entire player from db
// name: player name to load
// returns char_data pointer or NULL on failure
P_char sql_load_player(const char *name);

// check if player exists in db
bool sql_player_exists(const char *name);

// character rename
bool sql_player_rename(P_char ch, const char *new_name);

// get player pid by name
int sql_get_player_pid(const char *name);

// individual load functions (called by sql_load_player)
bool sql_load_player_status(P_char ch, int pid);
bool sql_load_player_skills(P_char ch);
bool sql_load_player_affects(P_char ch);
bool sql_load_player_items(P_char ch);
bool sql_load_player_shapechanges(P_char ch);

// pet save/load for crash recovery
bool sql_save_player_pets(P_char ch, int save_type);
bool sql_load_player_pets(P_char ch);

// ============================================================================
// player delete
// ============================================================================

// delete player from db (for pwipe, etc)
bool sql_delete_player(int pid);
bool sql_delete_player_by_name(const char *name);

// ============================================================================
// account functions
// ============================================================================

// save account to db
bool sql_save_account(struct acct_entry *acc);

// load account from db by name
struct acct_entry *sql_load_account(const char *name);

// check if account exists
bool sql_account_exists(const char *name);

// link player to account (updates player_data.account_name)
bool sql_link_player_to_account(const char *account_name, int pid);

// ============================================================================
// locker functions
// ============================================================================

// save locker to db
// for personal locker: owner_pid set, owner_assoc_id = 0
// for guild locker: owner_pid = 0, owner_assoc_id set
bool sql_save_locker(P_char locker_ch, int owner_pid, int owner_assoc_id);

// load locker from db
// pass owner_pid for personal, owner_assoc_id for guild (other should be 0)
P_char sql_load_locker(int owner_pid, int owner_assoc_id);

// load locker by name (e.g. "playername.locker" or "guild.123.locker")
P_char sql_load_locker_by_name(const char *locker_name);

// check if locker exists
bool sql_locker_exists(int owner_pid, int owner_assoc_id);
bool sql_locker_exists_by_name(const char *locker_name);

// delete locker
bool sql_delete_locker(int owner_pid, int owner_assoc_id);
bool sql_delete_locker_by_name(const char *locker_name);

// private chest functions
int  sql_get_locker_id_by_name(const char *locker_name);
int  sql_get_or_create_public_chest(int locker_id);
int  sql_create_private_chest(int locker_id, const char *chest_name, const char *password);
bool sql_delete_private_chest(int chest_id);
int  sql_get_chest_id(int locker_id, const char *chest_name);
bool sql_verify_chest_password(int chest_id, const char *password);
int  sql_count_private_chests(int locker_id);
// private_chest_log action_type values
#define CHEST_ACTION_OPEN  1
#define CHEST_ACTION_CLOSE 2
#define CHEST_ACTION_PUT   3
#define CHEST_ACTION_GET   4
#define CHEST_ACTION_FAIL  5

bool  sql_log_chest_activity(int locker_id, int chest_id, const char *char_name, int action_type, const char *item_short);
bool  sql_save_private_chest_items(int locker_id, int chest_id, P_obj chest_obj);
void  sql_load_private_chest_items(int locker_id, int chest_id, P_obj chest_obj);

// account bank
bool      sql_load_account_bank(const char *account_name, int racewar, P_char ch);
bool      sql_save_account_bank(const char *account_name, int racewar, P_char ch);
long long sql_account_bank_deposit(const char *account_name, int racewar, int coin_type, int amount);
long long sql_account_bank_withdraw(const char *account_name, int racewar, int coin_type, int amount);
bool      sql_ensure_account_bank(const char *account_name, int racewar);

// ============================================================================
// migration helpers
// ============================================================================

// migrate single player from pfile to db
// loads from pfile, saves to db, verifies
bool sql_migrate_player(const char *name);

// verify player data matches between pfile and db
bool sql_verify_player(const char *name);

// migrate all players from pfiles to db
// returns count of successfully migrated players
int sql_migrate_all_players(void);

// ============================================================================
// utility
// ============================================================================

// escape string for sql (wrapper around mysql_real_escape_string)
// caller must free returned string
char *sql_escape_string(const char *str);

// log sql error with context
void sql_player_error(const char *context, const char *query);

// towns
bool sql_save_towns(void);
bool sql_load_towns(void);

// account ips
struct acct_ip;
bool            sql_save_account_ips(const char *account_name, struct acct_ip *ips);
struct acct_ip *sql_load_account_ips(const char *account_name);
bool            sql_delete_account_ips(const char *account_name);

// kingdom land
bool sql_save_kingdom_land(void);

// corpses
bool sql_save_corpse(P_obj corpse);
bool sql_delete_corpse(const char *player_name, int save_id);
bool sql_load_all_corpses(void);

// shopkeepers
bool   sql_save_shopkeeper(P_char ch, int shop_nr);
bool   sql_delete_shopkeeper(int shop_nr);
P_char sql_restore_shopkeeper(int shop_nr);
void   sql_restore_shopkeepers(void);
void   sql_save_dirty_shopkeepers(void);

// saved items
bool sql_save_saved_item(P_obj item, const char *item_key);
bool sql_delete_saved_item(const char *item_key);
void sql_restore_saved_items(void);

// siege items
bool sql_save_siege_item(P_obj obj, int room_vnum);
bool sql_save_siege_list(void);
bool sql_delete_siege_items(int room_vnum);
void sql_load_siege_list(void);

// ships
struct ShipData;
bool             sql_save_ship(struct ShipData *ship);
struct ShipData *sql_load_ship(const char *owner_name);
bool             sql_load_all_ships(void);
bool             sql_delete_ship(const char *owner_name);

// guilds
class Guild;
bool   sql_save_guild(Guild *guild);
Guild *sql_load_guild(unsigned int guild_id);
bool   sql_load_all_guilds(void);
bool   sql_delete_guild(unsigned int guild_id);

// spellbooks (conjurable mobs)
bool sql_add_spellbook_mob(int pid, int mob_vnum);
bool sql_has_spellbook_mob(int pid, int mob_vnum);
int *sql_get_spellbook_mobs(int pid, int *count);
bool sql_delete_spellbook_mobs(int pid);

#endif // __NO_MYSQL__

#endif // __SQL_PLAYER_H_INCLUDED__
