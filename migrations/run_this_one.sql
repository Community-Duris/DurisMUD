-- durismud pfile-to-db combined migration
-- safe for production, idempotent, run multiple times safely

-- accounts and players

CREATE TABLE IF NOT EXISTS accounts (
    account_name VARCHAR(50) NOT NULL,
    email VARCHAR(255) DEFAULT NULL,
    password VARCHAR(128) NOT NULL,
    confirmation_code VARCHAR(64) DEFAULT NULL,
    confirmed TINYINT(1) DEFAULT 0,
    confirmation_sent TINYINT(1) DEFAULT 0,
    blocked TINYINT(1) DEFAULT 0,
    last_login BIGINT DEFAULT 0,
    last_good_char BIGINT DEFAULT 0,
    last_evil_char BIGINT DEFAULT 0,
    flags1 BIGINT UNSIGNED DEFAULT 0,
    flags2 BIGINT UNSIGNED DEFAULT 0,
    flags3 BIGINT UNSIGNED DEFAULT 0,
    flags4 BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (account_name),
    INDEX idx_email (email)
);

CREATE TABLE IF NOT EXISTS account_characters (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    char_name VARCHAR(64) NOT NULL,
    pid INT UNSIGNED DEFAULT NULL,
    login_count BIGINT UNSIGNED DEFAULT 0,
    last_login BIGINT DEFAULT 0,
    blocked TINYINT DEFAULT 0,
    racewar TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name),
    INDEX idx_char_name (char_name)
);

CREATE TABLE IF NOT EXISTS player_data (
    pid INT UNSIGNED NOT NULL AUTO_INCREMENT,
    name VARCHAR(64) NOT NULL,
    account_name VARCHAR(50) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    long_descr TEXT DEFAULT NULL,
    description TEXT DEFAULT NULL,
    title VARCHAR(512) DEFAULT NULL,
    m_class INT UNSIGNED DEFAULT 0,
    secondary_class INT UNSIGNED DEFAULT 0,
    spec TINYINT UNSIGNED DEFAULT 0,
    race TINYINT UNSIGNED DEFAULT 0,
    racewar TINYINT UNSIGNED DEFAULT 0,
    level TINYINT UNSIGNED DEFAULT 1,
    sex TINYINT UNSIGNED DEFAULT 0,
    weight SMALLINT UNSIGNED DEFAULT 0,
    height SMALLINT UNSIGNED DEFAULT 0,
    size TINYINT DEFAULT 0,
    hometown INT DEFAULT 0,
    birthplace INT DEFAULT 0,
    orig_birthplace INT DEFAULT 0,
    last_room INT DEFAULT 0,
    birth_time BIGINT DEFAULT 0,
    played_time INT DEFAULT 0,
    last_save BIGINT DEFAULT 0,
    perm_aging SMALLINT DEFAULT 0,
    base_str TINYINT DEFAULT 0,
    base_dex TINYINT DEFAULT 0,
    base_agi TINYINT DEFAULT 0,
    base_con TINYINT DEFAULT 0,
    base_pow TINYINT DEFAULT 0,
    base_int TINYINT DEFAULT 0,
    base_wis TINYINT DEFAULT 0,
    base_cha TINYINT DEFAULT 0,
    base_kar TINYINT DEFAULT 0,
    base_luk TINYINT DEFAULT 0,
    mana INT DEFAULT 0,
    base_mana INT DEFAULT 0,
    hit_diff INT DEFAULT 0,
    base_hit INT DEFAULT 0,
    vitality INT DEFAULT 0,
    base_vitality INT DEFAULT 0,
    spells_memmed_extra TINYINT DEFAULT 0,
    copper BIGINT DEFAULT 0,
    silver BIGINT DEFAULT 0,
    gold BIGINT DEFAULT 0,
    platinum BIGINT DEFAULT 0,
    bank_copper BIGINT DEFAULT 0,
    bank_silver BIGINT DEFAULT 0,
    bank_gold BIGINT DEFAULT 0,
    bank_platinum BIGINT DEFAULT 0,
    exp BIGINT DEFAULT 0,
    epics BIGINT DEFAULT 0,
    epic_skill_points BIGINT DEFAULT 0,
    skillpoints INT DEFAULT 0,
    spell_bind_used BIGINT DEFAULT 0,
    act BIGINT UNSIGNED DEFAULT 0,
    act2 BIGINT UNSIGNED DEFAULT 0,
    act3 BIGINT UNSIGNED DEFAULT 0,
    vote BIGINT UNSIGNED DEFAULT 0,
    alignment INT DEFAULT 0,
    prestige SMALLINT DEFAULT 0,
    assoc_id SMALLINT UNSIGNED DEFAULT 0,
    guild_status INT UNSIGNED DEFAULT 0,
    time_left_guild BIGINT DEFAULT 0,
    nb_left_guild TINYINT DEFAULT 0,
    time_unspecced BIGINT DEFAULT 0,
    frags BIGINT DEFAULT 0,
    oldfrags BIGINT DEFAULT 0,
    numb_deaths BIGINT UNSIGNED DEFAULT 0,
    killed_by VARCHAR(64) DEFAULT NULL,
    condition_0 TINYINT DEFAULT 0,
    condition_1 TINYINT DEFAULT 0,
    condition_2 TINYINT DEFAULT 0,
    condition_3 TINYINT DEFAULT 0,
    condition_4 TINYINT DEFAULT 0,
    poof_in VARCHAR(512) DEFAULT NULL,
    poof_out VARCHAR(512) DEFAULT NULL,
    poof_in_sound VARCHAR(512) DEFAULT NULL,
    poof_out_sound VARCHAR(512) DEFAULT NULL,
    echo_toggle TINYINT UNSIGNED DEFAULT 0,
    prompt SMALLINT UNSIGNED DEFAULT 0,
    wiz_invis BIGINT DEFAULT 0,
    law_flags BIGINT UNSIGNED DEFAULT 0,
    wimpy SMALLINT DEFAULT 0,
    aggressive SMALLINT DEFAULT -1,
    highest_level TINYINT UNSIGNED DEFAULT 0,
    screen_length TINYINT UNSIGNED DEFAULT 24,
    quest_active INT DEFAULT 0,
    quest_mob_vnum INT DEFAULT 0,
    quest_type INT DEFAULT 0,
    quest_accomplished INT DEFAULT 0,
    quest_started INT DEFAULT 0,
    quest_zone_number INT DEFAULT 0,
    quest_giver INT DEFAULT 0,
    quest_level INT DEFAULT 0,
    quest_receiver INT DEFAULT 0,
    quest_shares_left INT DEFAULT 0,
    quest_kill_how_many INT DEFAULT 0,
    quest_kill_original INT DEFAULT 0,
    quest_map_room INT DEFAULT 0,
    quest_map_bought INT DEFAULT 0,
    last_ip BIGINT UNSIGNED DEFAULT 0,
    active TINYINT(1) NOT NULL DEFAULT 1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (pid),
    INDEX idx_name (name),
    INDEX idx_account_name (account_name)
);


-- account related

CREATE TABLE IF NOT EXISTS account_ips (
    id INT AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    hostname VARCHAR(255),
    ip_address VARCHAR(45),
    count BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name),
    INDEX idx_ip_address (ip_address),
    UNIQUE KEY uk_account_ip (account_name, ip_address)
);

CREATE TABLE IF NOT EXISTS towns (
    id INT AUTO_INCREMENT PRIMARY KEY,
    zone_filename VARCHAR(100) NOT NULL,
    resources INT DEFAULT 0,
    defense INT DEFAULT 0,
    offense INT DEFAULT 0,
    deploy_guard TINYINT DEFAULT 0,
    guard_vnum INT DEFAULT 0,
    guard_max INT DEFAULT 0,
    guard_load_room INT DEFAULT 0,
    deploy_cavalry TINYINT DEFAULT 0,
    cavalry_vnum INT DEFAULT 0,
    cavalry_max INT DEFAULT 0,
    cavalry_load_room INT DEFAULT 0,
    deploy_portals TINYINT DEFAULT 0,
    portal_vnum INT DEFAULT 0,
    portal_load_room INT DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_zone_filename (zone_filename)
);

CREATE TABLE IF NOT EXISTS kingdom_land (
    id INT AUTO_INCREMENT PRIMARY KEY,
    kingdom_id INT NOT NULL,  -- no fk, comes from filesystem
    start_vnum INT DEFAULT 0,
    end_vnum INT DEFAULT 0,
    type CHAR(1) DEFAULT 'r',
    INDEX idx_kingdom_id (kingdom_id)
);

CREATE TABLE IF NOT EXISTS player_recipes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    recipe_vnum INT NOT NULL,
    learned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_recipe (pid, recipe_vnum)
);

CREATE TABLE IF NOT EXISTS player_shapechanges (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    mob_vnum INT NOT NULL,
    times_researched INT DEFAULT 0,
    last_researched BIGINT DEFAULT 0,
    last_shapechanged BIGINT DEFAULT 0,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_mob (pid, mob_vnum)
);

CREATE TABLE IF NOT EXISTS corpses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    player_name VARCHAR(50) NOT NULL,
    save_id BIGINT NOT NULL,
    room_vnum INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    INDEX idx_player_name (player_name),
    UNIQUE KEY uk_player_saveid (player_name, save_id)
);

CREATE TABLE IF NOT EXISTS shopkeepers (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shop_id INT NOT NULL UNIQUE,
    mob_vnum INT DEFAULT 0,
    room_vnum INT DEFAULT 0,
    save_time BIGINT DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_shop_id (shop_id)
);

CREATE TABLE IF NOT EXISTS shopkeeper_affects (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shopkeeper_id INT NOT NULL,
    type INT DEFAULT 0,
    duration INT DEFAULT 0,
    modifier INT DEFAULT 0,
    location INT DEFAULT 0,
    bitvector1 BIGINT UNSIGNED DEFAULT 0,
    bitvector2 BIGINT UNSIGNED DEFAULT 0,
    bitvector3 BIGINT UNSIGNED DEFAULT 0,
    bitvector4 BIGINT UNSIGNED DEFAULT 0,
    bitvector5 BIGINT UNSIGNED DEFAULT 0,
    FOREIGN KEY (shopkeeper_id) REFERENCES shopkeepers(id) ON DELETE CASCADE,
    INDEX idx_shopkeeper_id (shopkeeper_id)
);


-- race/class lookups (populated by game server)

CREATE TABLE IF NOT EXISTS races (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    short_name VARCHAR(32),
    ansi_name VARCHAR(128),
    abbrev VARCHAR(4),
    racewar TINYINT DEFAULT 0,
    playable TINYINT DEFAULT 0
);

CREATE TABLE IF NOT EXISTS classes (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    ansi_name VARCHAR(128),
    short_name VARCHAR(8),
    menu_char CHAR(1)
);

CREATE OR REPLACE VIEW players_view AS
SELECT
    pd.pid,
    pd.name,
    pd.level,
    pd.race as race_id,
    r.ansi_name as race,
    pd.m_class as class_id,
    c.ansi_name as classname,
    pd.racewar,
    pd.assoc_id,
    pd.exp,
    pd.epics,
    pd.played_time as playtime,
    (pd.copper + pd.silver*10 + pd.gold*100 + pd.platinum*1000) as money,
    (pd.bank_copper + pd.bank_silver*10 + pd.bank_gold*100 + pd.bank_platinum*1000) as balance
FROM player_data pd
LEFT JOIN races r ON pd.race = r.id
LEFT JOIN classes c ON pd.m_class = c.id;


-- player arrays

CREATE TABLE IF NOT EXISTS player_skills (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    skill_id SMALLINT UNSIGNED NOT NULL,
    learned TINYINT UNSIGNED DEFAULT 0,
    taught TINYINT UNSIGNED DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_skill (pid, skill_id),
    CONSTRAINT fk_player_skills FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_languages (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    tongue_id TINYINT UNSIGNED NOT NULL,
    proficiency TINYINT UNSIGNED DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_tongue (pid, tongue_id),
    CONSTRAINT fk_player_languages FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_intros (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    intro_index TINYINT UNSIGNED NOT NULL,
    intro_pid INT DEFAULT 0,
    intro_time BIGINT UNSIGNED DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_intro (pid, intro_index),
    CONSTRAINT fk_player_intros FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_timers (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    timer_id TINYINT UNSIGNED NOT NULL,
    timer_value BIGINT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_timer (pid, timer_id),
    CONSTRAINT fk_player_timers FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_undead_slots (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    circle TINYINT UNSIGNED NOT NULL,
    slots TINYINT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_circle (pid, circle),
    CONSTRAINT fk_player_undead_slots FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_forged_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    forge_index SMALLINT UNSIGNED NOT NULL,
    item_vnum INT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_forge (pid, forge_index),
    CONSTRAINT fk_player_forged_items FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_granted_cmds (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    cmd_num INT NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_cmd (pid, cmd_num),
    CONSTRAINT fk_player_granted_cmds FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);


-- player affects and items

CREATE TABLE IF NOT EXISTS player_affects (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    type SMALLINT NOT NULL,
    duration INT DEFAULT 0,
    flags SMALLINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    location TINYINT UNSIGNED DEFAULT 0,
    level SMALLINT UNSIGNED DEFAULT 0,
    bitvector1 BIGINT DEFAULT 0,
    bitvector2 BIGINT DEFAULT 0,
    bitvector3 BIGINT DEFAULT 0,
    bitvector4 BIGINT DEFAULT 0,
    bitvector5 BIGINT DEFAULT 0,
    custom_msg_char TEXT DEFAULT NULL,
    custom_msg_room TEXT DEFAULT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    CONSTRAINT fk_player_affects FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    equip_slot TINYINT DEFAULT 0,
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
    bitvector1 BIGINT UNSIGNED DEFAULT NULL,
    bitvector2 BIGINT UNSIGNED DEFAULT NULL,
    bitvector3 BIGINT UNSIGNED DEFAULT NULL,
    bitvector4 BIGINT UNSIGNED DEFAULT NULL,
    bitvector5 BIGINT UNSIGNED DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    INDEX idx_container_id (container_id),
    CONSTRAINT fk_player_items FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE,
    CONSTRAINT fk_player_items_container FOREIGN KEY (container_id) REFERENCES player_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_item_affects (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_item_id (item_id),
    CONSTRAINT fk_player_item_affects FOREIGN KEY (item_id) REFERENCES player_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_witnesses (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    crime TINYINT UNSIGNED DEFAULT 0,
    room_vnum INT DEFAULT 0,
    attacker_name VARCHAR(64) DEFAULT NULL,
    victim_name VARCHAR(64) DEFAULT NULL,
    witness_time BIGINT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    CONSTRAINT fk_player_witnesses FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_spellbooks (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    mob_vnum INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_mob (pid, mob_vnum),
    CONSTRAINT fk_player_spellbooks FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);


-- item storage

CREATE TABLE IF NOT EXISTS corpse_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    corpse_id INT NOT NULL,
    vnum INT NOT NULL,
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
    unique_id INT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (corpse_id) REFERENCES corpses(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES corpse_items(id) ON DELETE CASCADE,
    INDEX idx_corpse_id (corpse_id),
    INDEX idx_vnum (vnum)
);

CREATE TABLE IF NOT EXISTS corpse_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES corpse_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);

CREATE TABLE IF NOT EXISTS shopkeeper_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    shopkeeper_id INT NOT NULL,
    vnum INT NOT NULL,
    equip_slot TINYINT DEFAULT 0,
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
    unique_id INT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (shopkeeper_id) REFERENCES shopkeepers(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES shopkeeper_items(id) ON DELETE CASCADE,
    INDEX idx_shopkeeper_id (shopkeeper_id),
    INDEX idx_vnum (vnum)
);

CREATE TABLE IF NOT EXISTS shopkeeper_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES shopkeeper_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);

CREATE TABLE IF NOT EXISTS saved_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_key VARCHAR(100) NOT NULL UNIQUE,
    room_vnum INT DEFAULT 0,
    vnum INT NOT NULL,
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
    unique_id INT UNSIGNED DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (container_id) REFERENCES saved_items(id) ON DELETE CASCADE,
    INDEX idx_room_vnum (room_vnum),
    INDEX idx_vnum (vnum)
);

CREATE TABLE IF NOT EXISTS saved_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES saved_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);

CREATE TABLE IF NOT EXISTS siege_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    room_vnum INT NOT NULL,
    vnum INT NOT NULL,
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
    unique_id INT UNSIGNED DEFAULT NULL,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (container_id) REFERENCES siege_items(id) ON DELETE CASCADE,
    INDEX idx_room_vnum (room_vnum),
    INDEX idx_vnum (vnum)
);

CREATE TABLE IF NOT EXISTS siege_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES siege_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);


-- lockers

CREATE TABLE IF NOT EXISTS lockers (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_name VARCHAR(100) NOT NULL UNIQUE,
    owner_pid INT DEFAULT NULL,
    owner_assoc_id INT DEFAULT NULL,
    racewar TINYINT DEFAULT 0,
    race TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_owner_pid (owner_pid),
    INDEX idx_owner_assoc_id (owner_assoc_id)
);

CREATE TABLE IF NOT EXISTS locker_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
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
    unique_id INT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_vnum (vnum)
);

CREATE TABLE IF NOT EXISTS locker_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);


-- account_characters extra columns
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'login_count');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN login_count BIGINT UNSIGNED DEFAULT 0',
    'SELECT "login_count already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'last_login');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN last_login BIGINT DEFAULT 0',
    'SELECT "last_login already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'blocked');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN blocked TINYINT DEFAULT 0',
    'SELECT "blocked already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'racewar');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN racewar TINYINT DEFAULT 0',
    'SELECT "racewar already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND index_name = 'idx_account_racewar');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_account_racewar ON account_characters(account_name, racewar)',
    'SELECT "idx_account_racewar already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- ships

CREATE TABLE IF NOT EXISTS ships (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    owner_pid INT UNSIGNED DEFAULT NULL,
    owner_name VARCHAR(64) NOT NULL UNIQUE,
    ship_name VARCHAR(128) DEFAULT NULL,
    ship_class TINYINT UNSIGNED DEFAULT 0,
    frags INT DEFAULT 0,
    anchor_room INT DEFAULT 0,
    time_played INT DEFAULT 0,
    mainsail INT DEFAULT 0,
    race TINYINT DEFAULT 0,
    money INT DEFAULT 0,
    flags BIGINT UNSIGNED DEFAULT 0,
    armor_fore INT DEFAULT 0,
    armor_port INT DEFAULT 0,
    armor_rear INT DEFAULT 0,
    armor_star INT DEFAULT 0,
    internal_fore INT DEFAULT 0,
    internal_port INT DEFAULT 0,
    internal_rear INT DEFAULT 0,
    internal_star INT DEFAULT 0,
    crew_index INT DEFAULT 0,
    crew_sail_skill INT DEFAULT 0,
    crew_guns_skill INT DEFAULT 0,
    crew_rpar_skill INT DEFAULT 0,
    crew_sail_chief INT DEFAULT 0,
    crew_guns_chief INT DEFAULT 0,
    crew_rpar_chief INT DEFAULT 0,
    maxspeed_bonus INT DEFAULT 0,
    capacity_bonus INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'ships' AND index_name = 'idx_ships_owner_pid');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_ships_owner_pid ON ships(owner_pid)',
    'SELECT "idx_ships_owner_pid already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

CREATE TABLE IF NOT EXISTS ship_slots (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    ship_id INT UNSIGNED NOT NULL,
    slot_index TINYINT NOT NULL,
    slot_type INT NOT NULL DEFAULT 0,
    item_index INT NOT NULL DEFAULT 0,
    position INT NOT NULL DEFAULT 0,
    timer INT NOT NULL DEFAULT 0,
    val0 INT NOT NULL DEFAULT 0,
    val1 INT NOT NULL DEFAULT 0,
    val2 INT NOT NULL DEFAULT 0,
    val3 INT NOT NULL DEFAULT 0,
    val4 INT NOT NULL DEFAULT 0,
    CONSTRAINT fk_ship_slots_ship FOREIGN KEY (ship_id) REFERENCES ships(id) ON DELETE CASCADE,
    UNIQUE KEY uk_ship_slots_index (ship_id, slot_index)
);


-- guilds

CREATE TABLE IF NOT EXISTS guilds (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    guild_id INT UNSIGNED NOT NULL UNIQUE,
    name VARCHAR(100) NOT NULL,
    racewar INT UNSIGNED NOT NULL DEFAULT 0,
    bits INT UNSIGNED NOT NULL DEFAULT 0,
    prestige BIGINT UNSIGNED NOT NULL DEFAULT 0,
    construction BIGINT UNSIGNED NOT NULL DEFAULT 0,
    platinum INT UNSIGNED NOT NULL DEFAULT 0,
    gold INT UNSIGNED NOT NULL DEFAULT 0,
    silver INT UNSIGNED NOT NULL DEFAULT 0,
    copper INT UNSIGNED NOT NULL DEFAULT 0,
    total_frags BIGINT NOT NULL DEFAULT 0,
    top_frags BIGINT NOT NULL DEFAULT 0,
    top_fragger VARCHAR(50) NOT NULL DEFAULT '',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS guild_ranks (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    guild_id INT UNSIGNED NOT NULL,
    rank_index TINYINT NOT NULL,
    title VARCHAR(100) NOT NULL DEFAULT '',
    CONSTRAINT fk_guild_ranks_guild FOREIGN KEY (guild_id) REFERENCES guilds(id) ON DELETE CASCADE,
    UNIQUE KEY uk_guild_ranks_index (guild_id, rank_index)
);

CREATE TABLE IF NOT EXISTS guild_members (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    guild_id INT UNSIGNED NOT NULL,
    player_name VARCHAR(64) NOT NULL,
    player_pid INT UNSIGNED DEFAULT NULL,
    bits INT UNSIGNED NOT NULL DEFAULT 0,
    debt INT UNSIGNED NOT NULL DEFAULT 0,
    online_status TINYINT NOT NULL DEFAULT 0,
    CONSTRAINT fk_guild_members_guild FOREIGN KEY (guild_id) REFERENCES guilds(id) ON DELETE CASCADE,
    UNIQUE KEY uk_guild_members_name (guild_id, player_name)
);

-- online_status col if missing
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'guild_members' AND column_name = 'online_status');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE guild_members ADD COLUMN online_status TINYINT NOT NULL DEFAULT 0',
    'SELECT "online_status already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'guild_members' AND index_name = 'idx_guild_members_name');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_guild_members_name ON guild_members(player_name)',
    'SELECT "idx_guild_members_name already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- player_data fixes

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'act3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN act3 BIGINT UNSIGNED DEFAULT 0 AFTER act2',
    'SELECT "act3 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'last_room');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN last_room INT DEFAULT 0 AFTER orig_birthplace',
    'SELECT "last_room already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- unique constraints for char names
-- clean up dupes first
DELETE ac1 FROM account_characters ac1
INNER JOIN account_characters ac2
WHERE ac1.char_name = ac2.char_name
  AND ac1.pid > ac2.pid;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE()
    AND table_name = 'account_characters'
    AND index_name = 'idx_char_name_unique');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE account_characters ADD UNIQUE INDEX idx_char_name_unique (char_name)',
    'SELECT "idx_char_name_unique already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE()
    AND table_name = 'player_data'
    AND index_name = 'idx_player_name_unique');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_data ADD UNIQUE INDEX idx_player_name_unique (name)',
    'SELECT "idx_player_name_unique already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- hardcore hall of fame

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'killed_by');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN killed_by VARCHAR(64) DEFAULT NULL AFTER numb_deaths',
    'SELECT "killed_by already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- unique keys for upsert
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_languages' AND index_name = 'uk_pid_tongue');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_languages ADD UNIQUE KEY uk_pid_tongue (pid, tongue_id)',
    'SELECT "uk_pid_tongue already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_intros' AND index_name = 'uk_pid_intro');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_intros ADD UNIQUE KEY uk_pid_intro (pid, intro_index)',
    'SELECT "uk_pid_intro already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_timers' AND index_name = 'uk_pid_timer');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_timers ADD UNIQUE KEY uk_pid_timer (pid, timer_id)',
    'SELECT "uk_pid_timer already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_undead_slots' AND index_name = 'uk_pid_circle');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_undead_slots ADD UNIQUE KEY uk_pid_circle (pid, circle)',
    'SELECT "uk_pid_circle already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_forged_items' AND index_name = 'uk_pid_forge');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_forged_items ADD UNIQUE KEY uk_pid_forge (pid, forge_index)',
    'SELECT "uk_pid_forge already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_granted_cmds' AND index_name = 'uk_pid_cmd');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_granted_cmds ADD UNIQUE KEY uk_pid_cmd (pid, cmd_num)',
    'SELECT "uk_pid_cmd already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_skills' AND index_name = 'uk_pid_skill');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_skills ADD UNIQUE KEY uk_pid_skill (pid, skill_id)',
    'SELECT "uk_pid_skill already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- pets

CREATE TABLE IF NOT EXISTS `player_pets` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `owner_pid` INT UNSIGNED NOT NULL,
  `mob_vnum` INT NOT NULL,
  `pet_order` TINYINT DEFAULT 0,
  `hit` INT DEFAULT 0,
  `max_hit` INT DEFAULT 0,
  `mana` INT DEFAULT 0,
  `max_mana` INT DEFAULT 0,
  `vitality` INT DEFAULT 0,
  `max_vitality` INT DEFAULT 0,
  `charm_duration` INT DEFAULT -1,
  `room_vnum` INT DEFAULT 0,
  `saved_at` BIGINT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_owner_pid` (`owner_pid`),
  CONSTRAINT `fk_player_pets_owner` FOREIGN KEY (`owner_pid`)
    REFERENCES `player_data` (`pid`) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS `player_pet_items` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pet_id` INT UNSIGNED NOT NULL,
  `vnum` INT NOT NULL,
  `equip_slot` TINYINT DEFAULT 0,
  `container_id` INT UNSIGNED DEFAULT NULL,
  `weight` INT DEFAULT 0,
  `cost` INT DEFAULT 0,
  `timer` INT DEFAULT -1,
  `extra_flags` BIGINT UNSIGNED DEFAULT 0,
  `value0` INT DEFAULT 0,
  `value1` INT DEFAULT 0,
  `value2` INT DEFAULT 0,
  `value3` INT DEFAULT 0,
  `value4` INT DEFAULT 0,
  `value5` INT DEFAULT 0,
  `value6` INT DEFAULT 0,
  `value7` INT DEFAULT 0,
  `name` VARCHAR(512) DEFAULT NULL,
  `short_descr` VARCHAR(512) DEFAULT NULL,
  `description` TEXT DEFAULT NULL,
  `action_descr` TEXT DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_pet_id` (`pet_id`),
  KEY `idx_container_id` (`container_id`),
  CONSTRAINT `fk_pet_items_pet` FOREIGN KEY (`pet_id`)
    REFERENCES `player_pets` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_pet_items_container` FOREIGN KEY (`container_id`)
    REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS `player_pet_item_affects` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `item_id` INT UNSIGNED NOT NULL,
  `location` TINYINT UNSIGNED DEFAULT 0,
  `modifier` INT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_pet_item_affects` FOREIGN KEY (`item_id`)
    REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
);


-- zone payouts (optional, only if zones table exists)
DROP PROCEDURE IF EXISTS update_zone_payouts;

DELIMITER //
CREATE PROCEDURE update_zone_payouts()
BEGIN
    DECLARE tbl_exists INT DEFAULT 0;
    SELECT COUNT(*) INTO tbl_exists FROM information_schema.tables
        WHERE table_schema = DATABASE() AND table_name = 'zones';

    IF tbl_exists > 0 THEN
        -- disable group size penalty for now
        UPDATE zones SET suggested_group_size = 100 WHERE epic_type != '0';

        UPDATE zones SET epic_payout = 0 WHERE number = 1389;
        UPDATE zones SET epic_payout = 80 WHERE number IN (400, 93, 740, 14, 90, 383);
        UPDATE zones SET epic_payout = 90 WHERE number IN (264, 140, 823, 370, 38, 879, 113, 143);
        UPDATE zones SET epic_payout = 100 WHERE number IN (191, 342, 285, 67, 381, 27, 429, 805, 133, 183, 130, 666, 1320, 220, 755);
        UPDATE zones SET epic_payout = 110 WHERE number IN (758, 73, 824, 662, 664);
        UPDATE zones SET epic_payout = 120 WHERE number IN (430, 773, 490, 710);
        UPDATE zones SET epic_payout = 130 WHERE number IN (200, 766);
        UPDATE zones SET epic_payout = 150 WHERE number IN (760, 570, 91);
        UPDATE zones SET epic_payout = 175 WHERE number IN (318, 50);
        UPDATE zones SET epic_payout = 200 WHERE number IN (970, 920, 213);
        UPDATE zones SET epic_payout = 225 WHERE number IN (24, 244, 254, 197);
        UPDATE zones SET epic_payout = 250 WHERE number IN (151, 780, 412);
        UPDATE zones SET epic_payout = 260 WHERE number IN (87, 368);
        UPDATE zones SET epic_payout = 275 WHERE number IN (35, 448, 756, 261);
        UPDATE zones SET epic_payout = 285 WHERE number IN (419, 162);
        UPDATE zones SET epic_payout = 300 WHERE number IN (709, 238, 124);
        UPDATE zones SET epic_payout = 315 WHERE number IN (784, 831);
        UPDATE zones SET epic_payout = 325 WHERE number IN (386, 229, 289, 960);
        UPDATE zones SET epic_payout = 335 WHERE number = 441;
        UPDATE zones SET epic_payout = 345 WHERE number = 215;
        UPDATE zones SET epic_payout = 350 WHERE number IN (989, 315, 367, 1200, 1398, 232);
        UPDATE zones SET epic_payout = 400 WHERE number IN (328, 159, 435, 712, 326);
        UPDATE zones SET epic_payout = 425 WHERE number IN (910, 877, 777);
        UPDATE zones SET epic_payout = 450 WHERE number IN (883, 1316);
        UPDATE zones SET epic_payout = 500 WHERE number IN (814, 230, 1390);
        UPDATE zones SET epic_payout = 550 WHERE number IN (444, 588, 1424);
        UPDATE zones SET epic_payout = 600 WHERE number IN (1300, 68);
        UPDATE zones SET epic_payout = 650 WHERE number = 196;
        UPDATE zones SET epic_payout = 700 WHERE number IN (345, 257);
        UPDATE zones SET epic_payout = 800 WHERE number IN (266, 324);
        UPDATE zones SET epic_payout = 850 WHERE number = 4200;
        UPDATE zones SET epic_payout = 900 WHERE number IN (387, 455);
        UPDATE zones SET epic_payout = 950 WHERE number = 875;
        UPDATE zones SET epic_payout = 1000 WHERE number = 583;
    END IF;
END //
DELIMITER ;

CALL update_zone_payouts();
DROP PROCEDURE IF EXISTS update_zone_payouts;


-- item extra descriptions (spellbooks etc)

CREATE TABLE IF NOT EXISTS player_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_player_item_ed FOREIGN KEY (item_id)
    REFERENCES player_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_pet_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_pet_item_ed FOREIGN KEY (item_id)
    REFERENCES player_pet_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS corpse_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_corpse_item_ed FOREIGN KEY (item_id)
    REFERENCES corpse_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS locker_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_locker_item_ed FOREIGN KEY (item_id)
    REFERENCES locker_items(id) ON DELETE CASCADE
);

-- obj_uid for item duplication prevention
DELIMITER //

CREATE PROCEDURE add_obj_uid_columns()
BEGIN
    -- player_items
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'player_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE player_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'player_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE player_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'item_condition') THEN
        ALTER TABLE player_items ADD COLUMN item_condition SMALLINT DEFAULT 100;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND index_name = 'idx_obj_uid') THEN
        ALTER TABLE player_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE player_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'item_type') THEN
        ALTER TABLE player_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector1') THEN
        ALTER TABLE player_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER action_descr;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector2') THEN
        ALTER TABLE player_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector3') THEN
        ALTER TABLE player_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector4') THEN
        ALTER TABLE player_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector5') THEN
        ALTER TABLE player_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4;
    END IF;

    -- corpse_items
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'corpse_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE corpse_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'corpse_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE corpse_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'corpse_items'
                   AND column_name = 'item_condition') THEN
        ALTER TABLE corpse_items ADD COLUMN item_condition SMALLINT DEFAULT 100;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE()
                   AND table_name = 'corpse_items'
                   AND index_name = 'idx_obj_uid') THEN
        ALTER TABLE corpse_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'corpse_items'
                   AND column_name = 'item_type') THEN
        ALTER TABLE corpse_items ADD COLUMN item_type INT NOT NULL DEFAULT 0 AFTER vnum;
    END IF;

    -- locker_items
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'locker_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE locker_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'locker_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE locker_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'item_condition') THEN
        ALTER TABLE locker_items ADD COLUMN item_condition SMALLINT DEFAULT 100;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND index_name = 'idx_obj_uid') THEN
        ALTER TABLE locker_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE locker_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'item_type') THEN
        ALTER TABLE locker_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'bitvector1') THEN
        ALTER TABLE locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER action_descr;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'bitvector2') THEN
        ALTER TABLE locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'bitvector3') THEN
        ALTER TABLE locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'bitvector4') THEN
        ALTER TABLE locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'bitvector5') THEN
        ALTER TABLE locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4;
    END IF;

    -- player_pet_items
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_pet_items'
                   AND column_name = 'obj_uid') THEN
        ALTER TABLE player_pet_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_pet_items'
                   AND column_name = 'item_condition') THEN
        ALTER TABLE player_pet_items ADD COLUMN item_condition SMALLINT DEFAULT 100;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_pet_items'
                   AND index_name = 'idx_obj_uid') THEN
        ALTER TABLE player_pet_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;
END //

DELIMITER ;

CALL add_obj_uid_columns();
DROP PROCEDURE IF EXISTS add_obj_uid_columns;


-- account lockers

CREATE TABLE IF NOT EXISTS account_lockers (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL UNIQUE,
    racewar TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name)
);

CREATE TABLE IF NOT EXISTS locker_chests (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    keyword VARCHAR(64) NOT NULL,
    keyword_hash VARCHAR(64) DEFAULT NULL,
    is_public TINYINT(1) DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_keyword (locker_id, keyword),
    INDEX idx_locker_id (locker_id)
);

CREATE TABLE IF NOT EXISTS account_locker_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    chest_id INT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
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
    FOREIGN KEY (chest_id) REFERENCES locker_chests(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES account_locker_items(id) ON DELETE CASCADE,
    INDEX idx_chest_id (chest_id),
    INDEX idx_vnum (vnum),
    INDEX idx_obj_uid (obj_uid)
);

CREATE TABLE IF NOT EXISTS account_locker_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES account_locker_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);

CREATE TABLE IF NOT EXISTS account_locker_access (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    visitor_account VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_visitor (locker_id, visitor_account),
    INDEX idx_visitor (visitor_account)
);

CREATE TABLE IF NOT EXISTS locker_activity_log (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    char_name VARCHAR(64) NOT NULL,
    action_type ENUM('enter', 'leave', 'chest_open', 'chest_fail', 'kicked', 'chest_create', 'chest_delete', 'item_put', 'item_get') NOT NULL,
    chest_keyword VARCHAR(64) DEFAULT NULL,
    details VARCHAR(255) DEFAULT NULL,
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_logged_at (logged_at)
);

CREATE TABLE IF NOT EXISTS locker_kickouts (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    fail_count TINYINT UNSIGNED DEFAULT 0,
    kicked_until TIMESTAMP NULL DEFAULT NULL,
    last_fail TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_account (locker_id, account_name)
);

CREATE TABLE IF NOT EXISTS locker_session_state (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    chest_id INT UNSIGNED NOT NULL,
    opened_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (chest_id) REFERENCES locker_chests(id) ON DELETE CASCADE,
    UNIQUE KEY uk_session (locker_id, account_name, chest_id)
);


-- migrate char lockers to account lockers (non-destructive)

-- sync account_name
UPDATE player_data pd
JOIN account_characters ac ON pd.pid = ac.pid
SET pd.account_name = ac.account_name
WHERE pd.account_name IS NULL OR pd.account_name = '';

-- create account lockers
INSERT IGNORE INTO lockers (locker_name, racewar, race)
SELECT DISTINCT CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker'), ac.racewar, 0
FROM account_characters ac
JOIN lockers l ON LOWER(SUBSTRING_INDEX(l.locker_name, '.locker', 1)) = LOWER(ac.char_name)
WHERE ac.account_name IS NOT NULL AND ac.account_name != ''
  AND l.locker_name LIKE '%.locker'
  AND l.locker_name NOT LIKE 'guild.%'
  AND l.locker_name NOT LIKE 'account.%';

-- copy items
INSERT INTO locker_items (locker_id, vnum, container_id, quantity, weight, cost, timer,
    extra_flags, value0, value1, value2, value3, value4, value5, value6, value7,
    name, short_descr, description, action_descr, obj_uid, item_condition)
SELECT
    acct_locker.id,
    src.vnum,
    NULL,
    src.quantity, src.weight, src.cost, src.timer,
    src.extra_flags, src.value0, src.value1, src.value2, src.value3,
    src.value4, src.value5, src.value6, src.value7,
    src.name, src.short_descr, src.description, src.action_descr, src.obj_uid, src.item_condition
FROM locker_items src
JOIN lockers char_locker ON src.locker_id = char_locker.id
JOIN account_characters ac ON LOWER(SUBSTRING_INDEX(char_locker.locker_name, '.locker', 1)) = LOWER(ac.char_name)
JOIN lockers acct_locker ON acct_locker.locker_name = CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker')
WHERE char_locker.locker_name LIKE '%.locker'
  AND char_locker.locker_name NOT LIKE 'guild.%'
  AND char_locker.locker_name NOT LIKE 'account.%'
  AND src.vnum != 173
  AND (src.obj_uid IS NULL OR src.obj_uid NOT IN (
      SELECT obj_uid FROM locker_items WHERE locker_id = acct_locker.id AND obj_uid IS NOT NULL
  ));

-- copy affects
INSERT INTO locker_item_affects (item_id, location, modifier)
SELECT new_item.id, lia.location, lia.modifier
FROM locker_item_affects lia
JOIN locker_items old_item ON lia.item_id = old_item.id
JOIN lockers char_locker ON old_item.locker_id = char_locker.id
JOIN account_characters ac ON LOWER(SUBSTRING_INDEX(char_locker.locker_name, '.locker', 1)) = LOWER(ac.char_name)
JOIN lockers acct_locker ON acct_locker.locker_name = CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker')
JOIN locker_items new_item ON old_item.obj_uid = new_item.obj_uid AND new_item.locker_id = acct_locker.id
WHERE char_locker.locker_name LIKE '%.locker'
  AND char_locker.locker_name NOT LIKE 'guild.%'
  AND char_locker.locker_name NOT LIKE 'account.%'
  AND old_item.obj_uid IS NOT NULL
  AND NOT EXISTS (
      SELECT 1 FROM locker_item_affects WHERE item_id = new_item.id AND location = lia.location
  );


-- account banks

CREATE TABLE IF NOT EXISTS account_banks (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    racewar TINYINT NOT NULL DEFAULT 0,
    bank_copper BIGINT UNSIGNED DEFAULT 0,
    bank_silver BIGINT UNSIGNED DEFAULT 0,
    bank_gold BIGINT UNSIGNED DEFAULT 0,
    bank_platinum BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    UNIQUE KEY uk_account_racewar (account_name, racewar),
    INDEX idx_account_name (account_name)
);

-- migrate player banks to account banks
REPLACE INTO account_banks (account_name, racewar, bank_copper, bank_silver, bank_gold, bank_platinum)
SELECT
    ac.account_name,
    ac.racewar,
    SUM(pd.bank_copper),
    SUM(pd.bank_silver),
    SUM(pd.bank_gold),
    SUM(pd.bank_platinum)
FROM account_characters ac
JOIN player_data pd ON ac.pid = pd.pid
WHERE pd.bank_copper > 0 OR pd.bank_silver > 0 OR pd.bank_gold > 0 OR pd.bank_platinum > 0
GROUP BY ac.account_name, ac.racewar;


-- private chests (links to lockers table)

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
);

-- add chest_id to locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'chest_id');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN chest_id INT UNSIGNED DEFAULT NULL AFTER locker_id',
    'SELECT "chest_id already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- activity log for locker owner
CREATE TABLE IF NOT EXISTS private_chest_log (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    chest_id INT UNSIGNED DEFAULT NULL,
    char_name VARCHAR(64) NOT NULL,
    action_type ENUM('open', 'close', 'put', 'get', 'fail') NOT NULL,
    item_short VARCHAR(256) DEFAULT NULL,
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_logged_at (logged_at)
);

-- create default public chest for existing lockers
INSERT IGNORE INTO private_chests (locker_id, chest_name, is_public)
SELECT id, 'public', 1
FROM lockers
WHERE locker_name LIKE 'account.%';


-- kofi donations
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'total_donated');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN total_donated DECIMAL(10,2) DEFAULT 0.00',
    'SELECT "total_donated already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- polls

CREATE TABLE IF NOT EXISTS polls (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    question VARCHAR(512) NOT NULL,
    created_by VARCHAR(32) NOT NULL,
    created_at INT NOT NULL DEFAULT 0,
    expires_at INT NOT NULL DEFAULT 0,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    multi_select TINYINT(1) NOT NULL DEFAULT 0,
    max_choices INT NOT NULL DEFAULT 1
);

CREATE TABLE IF NOT EXISTS poll_options (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    poll_id INT UNSIGNED NOT NULL,
    option_num INT NOT NULL,
    option_text VARCHAR(256) NOT NULL,
    INDEX idx_poll_id (poll_id)
);

CREATE TABLE IF NOT EXISTS poll_votes (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    poll_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(64) NOT NULL,
    option_id INT UNSIGNED NOT NULL,
    voted_at INT NOT NULL DEFAULT 0,
    char_name VARCHAR(32) NOT NULL,
    UNIQUE KEY uk_poll_account_option (poll_id, account_name, option_id),
    INDEX idx_poll_id (poll_id),
    INDEX idx_account_name (account_name)
);


-- done
