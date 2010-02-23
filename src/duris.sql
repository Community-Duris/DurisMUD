--
-- Table structure for table `artifact_bind`
--

DROP TABLE IF EXISTS `artifact_bind`;
CREATE TABLE `artifact_bind` (
  `vnum` int(11) NOT NULL,
  `owner_pid` int(11),
  `timer` int(11),
  PRIMARY KEY (`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `auction_bid_history`
--

DROP TABLE IF EXISTS `auction_bid_history`;
CREATE TABLE `auction_bid_history` (
  `id` int(11) NOT NULL auto_increment,
  `date` int(11) NOT NULL default '0',
  `auction_id` int(11) NOT NULL default '0',
  `bidder_pid` int(11) NOT NULL default '0',
  `bidder_name` varchar(32) NOT NULL default '',
  `bid_amount` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `auction_id` (`auction_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `auction_item_pickups`
--

DROP TABLE IF EXISTS `auction_item_pickups`;
CREATE TABLE `auction_item_pickups` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` int(10) unsigned NOT NULL default '0',
  `obj_blob_str` blob NOT NULL,
  `retrieved` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `pid` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `auction_money_pickups`
--

DROP TABLE IF EXISTS `auction_money_pickups`;
CREATE TABLE `auction_money_pickups` (
  `pid` int(10) unsigned NOT NULL default '0',
  `money` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `auctions`
--

DROP TABLE IF EXISTS `auctions`;
CREATE TABLE `auctions` (
  `id` int(11) NOT NULL auto_increment,
  `seller_pid` int(10) unsigned NOT NULL default '0',
  `seller_name` varchar(32) NOT NULL default '',
  `start_time` int(11) NOT NULL default '0',
  `end_time` int(11) NOT NULL default '0',
  `status` enum('OPEN','CLOSED','REMOVED') NOT NULL default 'OPEN',
  `winning_bidder_pid` int(11) NOT NULL default '0',
  `winning_bidder_name` varchar(32) NOT NULL default '',
  `cur_price` int(10) unsigned NOT NULL default '0',
  `buy_price` int(11) NOT NULL default '0',
  `obj_short` varchar(255) NOT NULL default '',
  `obj_vnum` int(11) NOT NULL default '0',
  `obj_blob_str` blob NOT NULL,
  `id_keywords` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`id`),
  KEY `seller_pid` (`seller_pid`),
  KEY `auction_end` (`end_time`),
  KEY `status` (`status`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `categories`
--

DROP TABLE IF EXISTS `categories`;
CREATE TABLE `categories` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(50) default NULL,
  `desc` text,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `uname` (`name`)
) ENGINE=MyISAM AUTO_INCREMENT=33 DEFAULT CHARSET=latin1;

--
-- Table structure for table `changes`
--

DROP TABLE IF EXISTS `changes`;
CREATE TABLE `changes` (
  `id` int(11) NOT NULL auto_increment,
  `history_id` int(11) default '-1',
  `history_text` text,
  `history_title` varchar(255) default NULL,
  `history_category_id` int(11) default NULL,
  `new_text` text,
  `new_title` varchar(255) default NULL,
  `new_category_id` int(11) default NULL,
  `timestamp` datetime default NULL,
  `action` varchar(255) default NULL,
  `ip_number` varchar(255) default '',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=64 DEFAULT CHARSET=latin1;

--
-- Table structure for table `epic_gain`
--

DROP TABLE IF EXISTS `epic_gain`;
CREATE TABLE `epic_gain` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` bigint(20) NOT NULL default '0',
  `time` datetime NOT NULL,
  `type` int(11) NOT NULL default '0',
  `type_id` int(11) NOT NULL default '0',
  `epics` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `pid_index` (`pid`)
) ENGINE=MyISAM AUTO_INCREMENT=68 DEFAULT CHARSET=latin1;

--
-- Table structure for table `eq_drop`
--

DROP TABLE IF EXISTS `eq_drop`;
CREATE TABLE `eq_drop` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `date` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `vnum` int(10) unsigned NOT NULL default '0',
  `pid_looter` bigint(20) unsigned NOT NULL default '0',
  `room_id` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `Index_2` USING BTREE (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Table structure for table `guild_transactions`
--

DROP TABLE IF EXISTS `guild_transactions`;
CREATE TABLE `guild_transactions` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `soc_id` int(10) unsigned NOT NULL default '0',
  `date` int(11) NOT NULL default '0',
  `transaction_info` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`id`),
  KEY `soc_id` (`soc_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `guildhall_rooms`
--

DROP TABLE IF EXISTS `guildhall_rooms`;
CREATE TABLE `guildhall_rooms` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `guildhall_id` int(11) NOT NULL DEFAULT '0',
  `vnum` int(11) NOT NULL DEFAULT '0',
  `type` int(11) NOT NULL DEFAULT '0',
  `value0` int(10) unsigned NOT NULL DEFAULT '0',
  `value1` int(10) unsigned NOT NULL DEFAULT '0',
  `value2` int(10) unsigned NOT NULL DEFAULT '0',
  `value3` int(10) unsigned NOT NULL DEFAULT '0',
  `value4` int(10) unsigned NOT NULL DEFAULT '0',
  `value5` int(10) unsigned NOT NULL DEFAULT '0',
  `value6` int(10) unsigned NOT NULL DEFAULT '0',
  `value7` int(10) unsigned NOT NULL DEFAULT '0',
  `exit0` int(11) NOT NULL DEFAULT '0',
  `exit1` int(11) NOT NULL DEFAULT '0',
  `exit2` int(11) NOT NULL DEFAULT '0',
  `exit3` int(11) NOT NULL DEFAULT '0',
  `exit4` int(11) NOT NULL DEFAULT '0',
  `exit5` int(11) NOT NULL DEFAULT '0',
  `exit6` int(11) NOT NULL DEFAULT '0',
  `exit7` int(11) NOT NULL DEFAULT '0',
  `exit8` int(11) NOT NULL DEFAULT '0',
  `exit9` int(11) NOT NULL,
  `name` varchar(255) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `vnum` (`vnum`),
  KEY `guildhall_id` (`guildhall_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `guildhalls`
--

DROP TABLE IF EXISTS `guildhalls`;
CREATE TABLE `guildhalls` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `assoc_id` int(11) NOT NULL DEFAULT '0',
  `type` int(11) NOT NULL DEFAULT '0',
  `outside_vnum` int(11) NOT NULL DEFAULT '0',
  `racewar` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `assoc_id` (`assoc_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `ip_info`
--

DROP TABLE IF EXISTS `ip_info`;
CREATE TABLE `ip_info` (
  `pid` bigint(20) NOT NULL default '0',
  `last_ip` varchar(50) NOT NULL default 'none',
  `last_connect` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_disconnect` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `items`
--

DROP TABLE IF EXISTS `items`;
CREATE TABLE `items` (
  `vnum` int(10) unsigned NOT NULL default '0',
  `short_desc` varchar(100) NOT NULL default '',
  `obj_stat` text NOT NULL,
  `num_sold` int(11) NOT NULL default '0',
  `avg_sell_price` int(11) NOT NULL default '0',
  PRIMARY KEY  (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Table structure for table `multiplay_whitelist`
--

DROP TABLE IF EXISTS `multiplay_whitelist`;
CREATE TABLE `multiplay_whitelist` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `pattern` varchar(255) NOT NULL,
  `admin` varchar(255) NOT NULL,
  `description` varchar(255) NOT NULL,
  `created_on` date DEFAULT NULL,
  `player` varchar(255) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=latin1;

--
-- Table structure for table `nexus_stones`
--

DROP TABLE IF EXISTS `nexus_stones`;
CREATE TABLE `nexus_stones` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(255) NOT NULL default '',
  `room_vnum` int(11) NOT NULL default '0',
  `align` int(11) NOT NULL default '0',
  `stat_affect` int(11) NOT NULL default '-1',
  `affect_amount` int(11) NOT NULL default '0',
  `last_touched_at` int(11) NOT NULL default '0',
  `bonus` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=latin1;

--
-- Table structure for table `offline_messages`
--

DROP TABLE IF EXISTS `offline_messages`;
CREATE TABLE `offline_messages` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `date` datetime NOT NULL default '0000-00-00 00:00:00',
  `pid` int(11) NOT NULL default '0',
  `message` text NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `pages`
--

DROP TABLE IF EXISTS `pages`;
CREATE TABLE `pages` (
  `id` int(11) NOT NULL auto_increment,
  `title` varchar(255) default NULL,
  `text` text,
  `last_update` datetime default NULL,
  `last_update_by` varchar(255) default NULL,
  `category_id` int(11) default NULL,
  `ip_number` varchar(50) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `titleu` (`title`),
  FULLTEXT KEY `fulltextindex` (`title`,`text`)
) ENGINE=MyISAM AUTO_INCREMENT=10806 DEFAULT CHARSET=latin1;

--
-- Table structure for table `ping`
--

DROP TABLE IF EXISTS `ping`;
CREATE TABLE `ping` (
  `ID` bigint(11) NOT NULL auto_increment,
  `TIMESTAMP` datetime NOT NULL default '0000-00-00 00:00:00',
  `URL` varchar(100) NOT NULL default '',
  `IP` varchar(100) NOT NULL default '',
  `SEQ` bigint(20) NOT NULL default '0',
  `TIME` int(11) NOT NULL default '0',
  PRIMARY KEY  (`ID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `pkill_event`
--

DROP TABLE IF EXISTS `pkill_event`;
CREATE TABLE `pkill_event` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `stamp` datetime NOT NULL default '0000-00-00 00:00:00',
  `room_vnum` int(11) NOT NULL default '0',
  `room_name` text NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=14 DEFAULT CHARSET=latin1;

--
-- Table structure for table `pkill_info`
--

DROP TABLE IF EXISTS `pkill_info`;
CREATE TABLE `pkill_info` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `event_id` int(10) unsigned NOT NULL default '0',
  `pid` bigint(20) NOT NULL default '0',
  `level` int(11) NOT NULL default '0',
  `pk_type` text NOT NULL,
  `equip` text NOT NULL,
  `log` text,
  `inroom` int(11) NOT NULL default '0',
  `leader` int(11) default NULL,
  PRIMARY KEY  (`id`),
  KEY `part_of_name` (`event_id`),
  UNIQUE KEY `unique_index` (`pid`),
  KEY `level` (`level`),
  KEY `racewar` (`racewar`)
) ENGINE=MyISAM AUTO_INCREMENT=22 DEFAULT CHARSET=latin1;

--
-- Table structure for table `players_core`
--

DROP TABLE IF EXISTS `players_core`;
CREATE TABLE `players_core` (
  `pid` bigint(20) NOT NULL default '0',
  `name` varchar(255) NOT NULL,
  `race` varchar(255) NOT NULL,
  `classname` varchar(255) NOT NULL,
  `spec` varchar(255) NOT NULL,
  `guild` varchar(255) NOT NULL,
  `webinfo_toggle` int(1) NOT NULL default '0',
  `racewar` int(11) NOT NULL default '0',
  `level` int(11) NOT NULL default '0',
  `money` int(11) NOT NULL default '0',
  `balance` int(11) NOT NULL default '0',
  `playtime` int(11) NOT NULL default '0',
  `epics` int(11) NOT NULL default '0',
  PRIMARY KEY  (`pid`),
  UNIQUE KEY `unique_index` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `prepstatment_duris_sql`
--

DROP TABLE IF EXISTS `prepstatment_duris_sql`;
CREATE TABLE `prepstatment_duris_sql` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `desc` text NOT NULL,
  `sql` text NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Table structure for table `progress`
--

DROP TABLE IF EXISTS `progress`;
CREATE TABLE `progress` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` bigint(20) NOT NULL default '0',
  `var_type` enum('FRAGS','EXP') NOT NULL default 'FRAGS',
  `stamp` datetime NOT NULL default '0000-00-00 00:00:00',
  `delta` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `pid_index` (`pid`),
  KEY `index_enum` (`var_type`)
) ENGINE=MyISAM AUTO_INCREMENT=23 DEFAULT CHARSET=latin1;

--
-- Table structure for table `racewar_stat_mods`
--

DROP TABLE IF EXISTS `racewar_stat_mods`;
CREATE TABLE `racewar_stat_mods` (
  `racewar` int(11) NOT NULL default '0',
  `Str` int(11) NOT NULL default '0',
  `Dex` int(11) NOT NULL default '0',
  `Agi` int(11) NOT NULL default '0',
  `Con` int(11) NOT NULL default '0',
  `Pow` int(11) NOT NULL default '0',
  `Intl` int(11) NOT NULL default '0',
  `Wis` int(11) NOT NULL default '0',
  `Cha` int(11) NOT NULL default '0',
  `Kar` int(11) NOT NULL default '0',
  `Luc` int(11) NOT NULL default '0'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `world_quest_accomplished`
--

DROP TABLE IF EXISTS `world_quest_accomplished`;
CREATE TABLE `world_quest_accomplished` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` varchar(45) NOT NULL default '',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `quest_giver` int(10) unsigned NOT NULL default '0',
  `player_name` varchar(45) NOT NULL default '',
  `player_level` int(10) unsigned NOT NULL default '0',
	`quest_target` int(10) NOT NULL default '0',
  `reward_vnum` int(10) NOT NULL default '0',
  `reward_desc` VARCHAR(255) NOT NULL default '',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Table structure for table `zones`
--

DROP TABLE IF EXISTS `zones`;
CREATE TABLE `zones` (
  `id` int(10) auto_increment,
  `number` int(10),
  `name` varchar(100) NOT NULL default '',
  `epic_type` int(11) NOT NULL default '0',
  `frequency_mod` float NOT NULL default '1',
  `zone_freq_mod` float NOT NULL default '1',
  `epic_level` int(11) NOT NULL default '0',
  `task_zone` tinyint(1) NOT NULL default '0',
  `quest_zone` tinyint(1) NOT NULL default '0',
  `trophy_zone` tinyint(1) NOT NULL default '1',
  `suggested_group_size` int(10) NOT NULL default '1',
  `epic_payout` int(10) NOT NULL default 0,
  `difficulty` int(10) NOT NULL default 0,
  `randoms_zone` tinyint(1) NOT NULL default '1',
  `alignment` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY  (`id`),
  KEY `number_index` (`number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Table structure for table `log_entries`
--

DROP TABLE IF EXISTS `log_entries`;
CREATE TABLE `log_entries` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `date` datetime NOT NULL,
  `kind` varchar(255) NOT NULL default '',
  `player_name` varchar(255) NOT NULL default '',
  `pid` int(10) NOT NULL default '0',
  `ip_address` varchar(15) NOT NULL default '',
  `room_vnum` int(10) NOT NULL default '0',
  `zone_number` int(11) NOT NULL default '0',
  `message` varchar(255) NOT NULL default '',  
  PRIMARY KEY  (`id`),
  KEY `date_index` (`date`),
  KEY `kind_index` (`kind`),
  KEY `name_index` (`player_name`),
  KEY `pid_index` (`pid`),
  KEY `ip_address_index` (`ip_address`),
  KEY `room_vnum_index` (`room_vnum`),
  KEY `zone_number_index` (`zone_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Table structure for table `timers`
--

DROP TABLE IF EXISTS `timers`;
CREATE TABLE `timers` (
  `name` varchar(255) default '',
  `date` int(11) NOT NULL default '0',
  PRIMARY KEY  (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Table structure for table `associations`
--

DROP TABLE IF EXISTS `associations`;
CREATE TABLE `associations` (
  `id` int(11) NOT NULL,
  `name` varchar(255) NOT NULL default '',
  `prestige` int(11) NOT NULL default '0',
  `active` bool NOT NULL default 1,
  `wood` int(11) NOT NULL default '0',
  `stone` int(11) NOT NULL default '0',
  `construction_points` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `alliances`;
CREATE TABLE `alliances` (
  `id` int(11) NOT NULL auto_increment,
  `created_at` datetime,
  `forging_assoc_id` int(11) NOT NULL,
  `joining_assoc_id` int(11) NOT NULL,
  `tribute_owed` int(11) NOT NULL default 0,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Table structure for table `zone_trophy`
--

DROP TABLE IF EXISTS `zone_trophy`;
CREATE TABLE `zone_trophy` (
  `pid` bigint(20) NOT NULL default '0',
  `zone_number` int(11) NOT NULL default '0',
  `exp` int(11) NOT NULL default '0',
  PRIMARY KEY  (`pid`,`zone_number`),
  KEY `pid_index` (`pid`),
  KEY `zone_number` (`zone_number`),
  KEY `exp_index` (`exp`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `shop_trophy`;
CREATE TABLE `shop_trophy` (
  `id` int not null auto_increment,
  `item` int not null default 0,
  `value` int not null default 0,
  `seller` int not null default 0,
  `timestamp` timestamp not null,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `mud_info`;
CREATE TABLE `mud_info` (
	`name` VARCHAR(255) NOT NULL,
	`content` TEXT NOT NULL,
	PRIMARY KEY (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
INSERT INTO `mud_info` (name, content) VALUES ('news', 'News');
INSERT INTO `mud_info` (name, content) VALUES ('motd', 'Motd');
INSERT INTO `mud_info` (name, content) VALUES ('wizmotd', 'Wizmotd');

--
-- Table structure for table `ship_cargo_market_mods`
--

DROP TABLE IF EXISTS `ship_cargo_market_mods`;
CREATE TABLE `ship_cargo_market_mods` (
  `type` varchar(255) NOT NULL DEFAULT '',
  `port_id` int(11) NOT NULL DEFAULT '-1',
  `cargo_type` int(11) NOT NULL DEFAULT '-1',
  `modifier` float NOT NULL DEFAULT '0',
  KEY `type_port_id_cargo_type` (`type`,`port_id`,`cargo_type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `ship_cargo_market_mods`
--

DROP TABLE IF EXISTS `ship_cargo_prices`;
CREATE TABLE `ship_cargo_prices` (
  `type` varchar(255) NOT NULL DEFAULT '',
  `port_id` int(11) NOT NULL DEFAULT '-1',
  `cargo_type` int(11) NOT NULL DEFAULT '-1',
  `price` int(11) NOT NULL DEFAULT '0',
  KEY `type_port_id_cargo_type` (`type`,`port_id`,`cargo_type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table 'outposts'
--

--DROP TABLE IF EXISTS `outposts`;
--CREATE TABLE `outposts` (
--  `id` int(11) NOT NULL,
--  `owner_id` int(11) NOT NULL default '0',
--  `level` int(11) NOT NULL default '1',
--  `walls` int(11) NOT NULL default '0',
--  `archers` int(11) NOT NULL default '0',
--  `hitpoints` int(11) NOT NULL default '0',
--  `territory` int(11) NOT NULL default '0',
--  `portal_room` int(11) NOT NULL default '0',
--  `portal_dest` int(11) NOT NULL default '0',
--  `resources` int(11) NOT NULL default '0',
--  `applied_resources` int(11) NOT NULL default '100000',
--  PRIMARY KEY (`id`)
--) ENGINE=MyISAM DEFAULT CHARSET=latin1;

