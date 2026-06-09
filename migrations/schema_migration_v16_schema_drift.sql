-- schema migration v16: close drift between runtime code and legacy DBs
-- Safe to re-run for CREATE TABLE statements.
-- Column/index adds for existing tables are handled at boot by sql_ensure_runtime_schema() in src/sql.c
-- (MySQL 5.7-compatible; no ADD COLUMN IF NOT EXISTS).

-- Corpse persistence (sql_player.c)
CREATE TABLE IF NOT EXISTS corpses (
  id INT AUTO_INCREMENT PRIMARY KEY,
  player_name VARCHAR(50) NOT NULL,
  save_id BIGINT NOT NULL,
  room_vnum INT DEFAULT 0,
  short_descr VARCHAR(512) DEFAULT NULL,
  description TEXT DEFAULT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uk_player_saveid (player_name, save_id),
  INDEX idx_player_name (player_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS corpse_items (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  corpse_id INT NOT NULL,
  vnum INT NOT NULL,
  item_type INT NOT NULL DEFAULT 0,
  container_id INT UNSIGNED DEFAULT NULL,
  quantity SMALLINT UNSIGNED DEFAULT 1,
  weight INT DEFAULT 0,
  cost INT DEFAULT 0,
  timer INT DEFAULT -1,
  extra_flags BIGINT UNSIGNED DEFAULT 0,
  value0 INT DEFAULT 0,
  value1 INT DEFAULT 0,
  value2 INT DEFAULT 0,
  value3 INT DEFAULT 0,
  value4 INT DEFAULT 0,
  value5 INT DEFAULT 0,
  value6 INT DEFAULT 0,
  value7 INT DEFAULT 0,
  name VARCHAR(512) DEFAULT NULL,
  short_descr VARCHAR(512) DEFAULT NULL,
  description TEXT DEFAULT NULL,
  action_descr TEXT DEFAULT NULL,
  obj_uid BIGINT UNSIGNED DEFAULT NULL,
  item_condition SMALLINT DEFAULT 100,
  INDEX idx_corpse_id (corpse_id),
  INDEX idx_vnum (vnum),
  INDEX idx_obj_uid (obj_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS corpse_item_affects (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  item_id INT UNSIGNED NOT NULL,
  location TINYINT UNSIGNED DEFAULT 0,
  modifier INT DEFAULT 0,
  INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS corpse_item_extra_descr (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Persistence ownership audit (sql.c async worker)
CREATE TABLE IF NOT EXISTS persistence_item_event_audit (
  event_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  event_time BIGINT UNSIGNED NOT NULL,
  event_type VARCHAR(64) NOT NULL,
  item_uid BIGINT UNSIGNED NOT NULL,
  owner_type VARCHAR(32) NOT NULL,
  owner_ref VARCHAR(64) NOT NULL,
  actor_id INT NOT NULL DEFAULT -1,
  vnum INT NOT NULL DEFAULT -1,
  item_name VARCHAR(255) NOT NULL DEFAULT '',
  raw_event TEXT NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (event_id),
  KEY item_time (item_uid, event_time),
  KEY owner_lookup (owner_type, owner_ref)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS persistence_items_current (
  item_uid BIGINT UNSIGNED NOT NULL,
  owner_type VARCHAR(32) NOT NULL,
  owner_ref VARCHAR(64) NOT NULL,
  event_time BIGINT UNSIGNED NOT NULL,
  event_type VARCHAR(64) NOT NULL,
  actor_id INT NOT NULL DEFAULT -1,
  vnum INT NOT NULL DEFAULT -1,
  item_name VARCHAR(255) NOT NULL DEFAULT '',
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (item_uid),
  KEY owner_lookup (owner_type, owner_ref)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS persistence_item_conflicts (
  conflict_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_uid BIGINT UNSIGNED NOT NULL,
  existing_owner_type VARCHAR(32) NOT NULL,
  existing_owner_ref VARCHAR(64) NOT NULL,
  existing_event_time BIGINT UNSIGNED NOT NULL,
  incoming_owner_type VARCHAR(32) NOT NULL,
  incoming_owner_ref VARCHAR(64) NOT NULL,
  incoming_event_time BIGINT UNSIGNED NOT NULL,
  incoming_event_type VARCHAR(64) NOT NULL,
  resolution VARCHAR(64) NOT NULL,
  raw_event TEXT NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (conflict_id),
  KEY item_created (item_uid, created_at),
  KEY resolution_lookup (resolution)
) ENGINE=InnoDB;

DELIMITER //

DROP PROCEDURE IF EXISTS ensure_runtime_schema_drift//
CREATE PROCEDURE ensure_runtime_schema_drift()
BEGIN
  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'corpses') THEN
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'corpses'
                   AND column_name = 'short_descr') THEN
      ALTER TABLE corpses ADD COLUMN short_descr VARCHAR(512) DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'corpses'
                   AND column_name = 'description') THEN
      ALTER TABLE corpses ADD COLUMN description TEXT DEFAULT NULL;
    END IF;
  END IF;

  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'corpse_items') THEN
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'corpse_items'
                   AND column_name = 'item_type') THEN
      ALTER TABLE corpse_items ADD COLUMN item_type INT NOT NULL DEFAULT 0 AFTER vnum;
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE() AND table_name = 'corpse_items'
               AND column_name = 'unique_id') THEN
      ALTER TABLE corpse_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE() AND table_name = 'corpse_items'
                       AND column_name = 'obj_uid') THEN
      ALTER TABLE corpse_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'corpse_items'
                   AND column_name = 'item_condition') THEN
      ALTER TABLE corpse_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE() AND table_name = 'corpse_items'
                   AND index_name = 'idx_obj_uid') THEN
      ALTER TABLE corpse_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;
  END IF;

  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'player_items') THEN
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE() AND table_name = 'player_items'
               AND column_name = 'unique_id') THEN
      ALTER TABLE player_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE() AND table_name = 'player_items'
                       AND column_name = 'obj_uid') THEN
      ALTER TABLE player_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND column_name = 'item_condition') THEN
      ALTER TABLE player_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND index_name = 'idx_obj_uid') THEN
      ALTER TABLE player_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND column_name = 'wear_flags') THEN
      ALTER TABLE player_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND column_name = 'item_type') THEN
      ALTER TABLE player_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND column_name = 'bitvector1') THEN
      ALTER TABLE player_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER action_descr;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND column_name = 'bitvector2') THEN
      ALTER TABLE player_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND column_name = 'bitvector3') THEN
      ALTER TABLE player_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND column_name = 'bitvector4') THEN
      ALTER TABLE player_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'player_items'
                   AND column_name = 'bitvector5') THEN
      ALTER TABLE player_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4;
    END IF;
  END IF;

  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'locker_items') THEN
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'chest_id') THEN
      ALTER TABLE locker_items ADD COLUMN chest_id INT UNSIGNED DEFAULT NULL AFTER locker_id;
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE() AND table_name = 'locker_items'
               AND column_name = 'unique_id') THEN
      ALTER TABLE locker_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                       AND column_name = 'obj_uid') THEN
      ALTER TABLE locker_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'item_condition') THEN
      ALTER TABLE locker_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND index_name = 'idx_obj_uid') THEN
      ALTER TABLE locker_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'wear_flags') THEN
      ALTER TABLE locker_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'item_type') THEN
      ALTER TABLE locker_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'bitvector1') THEN
      ALTER TABLE locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER action_descr;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'bitvector2') THEN
      ALTER TABLE locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'bitvector3') THEN
      ALTER TABLE locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'bitvector4') THEN
      ALTER TABLE locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND column_name = 'bitvector5') THEN
      ALTER TABLE locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items'
                   AND index_name = 'idx_locker_chest') THEN
      ALTER TABLE locker_items ADD INDEX idx_locker_chest (locker_id, chest_id);
    END IF;
  END IF;

  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'lockers') THEN
    CREATE TABLE IF NOT EXISTS private_chests (
      id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
      locker_id INT UNSIGNED NOT NULL,
      chest_name VARCHAR(32) NOT NULL,
      password_hash VARCHAR(64) DEFAULT NULL,
      is_public TINYINT(1) DEFAULT 0,
      sort_config TEXT DEFAULT NULL,
      created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
      FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
      UNIQUE KEY uk_locker_chest (locker_id, chest_name),
      INDEX idx_locker_id (locker_id)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'private_chests'
                   AND column_name = 'sort_config') THEN
      ALTER TABLE private_chests ADD COLUMN sort_config TEXT DEFAULT NULL AFTER is_public;
    END IF;

    CREATE TABLE IF NOT EXISTS private_chest_log (
      id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
      locker_id INT UNSIGNED NOT NULL,
      chest_id INT UNSIGNED DEFAULT NULL,
      char_name VARCHAR(64) NOT NULL,
      action_type ENUM('open','close','put','get','fail') NOT NULL,
      item_short VARCHAR(256) DEFAULT NULL,
      logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
      FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
      INDEX idx_locker_id (locker_id),
      INDEX idx_logged_at (logged_at)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

    INSERT IGNORE INTO private_chests (locker_id, chest_name, is_public)
    SELECT id, 'public', 1
    FROM lockers
    WHERE locker_name LIKE 'account.%';
  END IF;

  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'auctions')
     AND NOT EXISTS (SELECT 1 FROM information_schema.columns
                     WHERE table_schema = DATABASE() AND table_name = 'auctions'
                     AND column_name = 'obj_info_text') THEN
    ALTER TABLE auctions ADD COLUMN obj_info_text TEXT DEFAULT NULL;
  END IF;

  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'epic_gain') THEN
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'epic_gain'
                   AND column_name = 'event_key') THEN
      ALTER TABLE epic_gain ADD COLUMN event_key VARCHAR(128) DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE() AND table_name = 'epic_gain'
                   AND index_name = 'reward_event_key') THEN
      ALTER TABLE epic_gain ADD UNIQUE KEY reward_event_key (event_key);
    END IF;
  END IF;

  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'world_quest_accomplished') THEN
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'world_quest_accomplished'
                   AND column_name = 'event_key') THEN
      ALTER TABLE world_quest_accomplished ADD COLUMN event_key VARCHAR(128) DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE() AND table_name = 'world_quest_accomplished'
                   AND index_name = 'reward_event_key') THEN
      ALTER TABLE world_quest_accomplished ADD UNIQUE KEY reward_event_key (event_key);
    END IF;
  END IF;

  IF EXISTS (SELECT 1 FROM information_schema.tables
             WHERE table_schema = DATABASE() AND table_name = 'zone_touches') THEN
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'zone_touches'
                   AND column_name = 'event_key') THEN
      ALTER TABLE zone_touches ADD COLUMN event_key VARCHAR(128) DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE() AND table_name = 'zone_touches'
                   AND index_name = 'reward_event_key') THEN
      ALTER TABLE zone_touches ADD UNIQUE KEY reward_event_key (event_key);
    END IF;
  END IF;
END//

DELIMITER ;

CALL ensure_runtime_schema_drift();
DROP PROCEDURE IF EXISTS ensure_runtime_schema_drift;
