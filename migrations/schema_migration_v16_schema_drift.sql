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

-- After CREATE TABLE above, start the MUD once (or run reward/auction ALTERs manually)
-- so sql_ensure_runtime_schema() can add: event_key*, obj_info_text, corpse item_type, etc.
