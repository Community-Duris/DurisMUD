-- MySQL dump 10.11
--
-- Host: localhost    Database: duris
-- ------------------------------------------------------
-- Server version	5.0.51a-24+lenny5
--
-- Arih: Last update 2025-11-04
-- Merged fix_mysql_schema.sql into this file for MySQL 8.0 compatibility
-- Changes:
--   - Updated ip_info table: replaced '0000-00-00 00:00:00' with NULL defaults
--   - Added racewar_side column to ip_info table
--   - Added artifacts table with column order documentation
--   - Added artifacts_mortal table
--   - Added locker_access table for storage permissions
--   - level_cap table already present with correct structure
--
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `alliances`
--

DROP TABLE IF EXISTS `alliances`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `alliances` (
  `id` int(11) NOT NULL auto_increment,
  `created_at` datetime default NULL,
  `forging_assoc_id` int(11) NOT NULL,
  `joining_assoc_id` int(11) NOT NULL,
  `tribute_owed` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `artifact_bind`
--

DROP TABLE IF EXISTS `artifact_bind`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `artifact_bind` (
  `vnum` int(11) NOT NULL,
  `owner_pid` int(11) default NULL,
  `timer` int(11) default NULL,
  PRIMARY KEY  (`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `associations`
--

DROP TABLE IF EXISTS `associations`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `associations` (
  `id` int(11) NOT NULL,
  `name` varchar(255) NOT NULL default '',
  `prestige` int(11) NOT NULL default '0',
  `active` tinyint(1) NOT NULL default '1',
  `wood` int(11) NOT NULL default '0',
  `stone` int(11) NOT NULL default '0',
  `construction_points` int(11) NOT NULL default '0',
  `over_max` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `auction_bid_history`
--

DROP TABLE IF EXISTS `auction_bid_history`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
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
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `auction_item_pickups`
--

DROP TABLE IF EXISTS `auction_item_pickups`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `auction_item_pickups` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` int(10) unsigned NOT NULL default '0',
  `obj_blob_str` blob NOT NULL,
  `retrieved` tinyint(1) NOT NULL default '0',
  `quantity` int(11) NOT NULL default '1',
  PRIMARY KEY  (`id`),
  KEY `pid` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `auction_money_pickups`
--

DROP TABLE IF EXISTS `auction_money_pickups`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `auction_money_pickups` (
  `pid` int(10) unsigned NOT NULL default '0',
  `money` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `auctions`
-- status values: 1=OPEN, 2=CLOSED, 3=REMOVED (matches AUCTION_STATUS_* defines)
--

DROP TABLE IF EXISTS `auctions`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `auctions` (
  `id` int(11) NOT NULL auto_increment,
  `seller_pid` int(10) unsigned NOT NULL default '0',
  `seller_name` varchar(32) NOT NULL default '',
  `start_time` TIMESTAMP NULL DEFAULT NULL,
  `end_time` TIMESTAMP NULL DEFAULT NULL,
  `status` int(11) NOT NULL default '1',
  `winning_bidder_pid` int(11) NOT NULL default '0',
  `winning_bidder_name` varchar(32) NOT NULL default '',
  `cur_price` int(10) unsigned NOT NULL default '0',
  `buy_price` int(11) NOT NULL default '0',
  `obj_short` varchar(255) NOT NULL default '',
  `obj_vnum` int(11) NOT NULL default '0',
  `obj_blob_str` blob NOT NULL,
  `id_keywords` varchar(1024) NOT NULL default '',
  `quantity` int(11) NOT NULL default '1',
  `obj_info_text` text,
  PRIMARY KEY  (`id`),
  KEY `seller_pid` (`seller_pid`),
  KEY `auction_end` (`end_time`),
  KEY `status` (`status`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `boons`
--

DROP TABLE IF EXISTS `boons`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `boons` (
  `id` int(11) NOT NULL auto_increment,
  `time` int(11) NOT NULL default '0',
  `duration` int(11) NOT NULL default '0',
  `racewar` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `opt` int(11) NOT NULL default '0',
  `criteria` decimal(10,2) NOT NULL default '0.00',
  `criteria2` decimal(10,2) NOT NULL default '0.00',
  `bonus` decimal(10,2) NOT NULL default '0.00',
  `bonus2` decimal(10,2) NOT NULL default '0.00',
  `random` int(11) NOT NULL default '0',
  `author` varchar(20) default NULL,
  `active` int(11) NOT NULL default '0',
  `pid` int(11) NOT NULL default '0',
  `rpt` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `boons_progress`
--

DROP TABLE IF EXISTS `boons_progress`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `boons_progress` (
  `id` int(11) NOT NULL auto_increment,
  `boonid` int(11) NOT NULL default '0',
  `pid` int(11) NOT NULL default '0',
  `counter` decimal(10,2) NOT NULL default '0.00',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `boons_shop`
--

DROP TABLE IF EXISTS `boons_shop`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `boons_shop` (
  `id` int(11) NOT NULL auto_increment,
  `pid` int(11) NOT NULL default '0',
  `points` int(11) NOT NULL default '0',
  `stats` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `pid` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `categories`
--

DROP TABLE IF EXISTS `categories`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `categories` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(255) default NULL,
  `desc` varchar(255) default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `changes`
--

DROP TABLE IF EXISTS `changes`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `changes` (
  `id` int(11) NOT NULL auto_increment,
  `history_id` int(11) default NULL,
  `history_text` text,
  `history_title` varchar(255) default NULL,
  `history_category_id` int(11) default NULL,
  `new_text` text,
  `new_title` varchar(255) default NULL,
  `new_category_id` int(11) default NULL,
  `timestamp` datetime default NULL,
  `action` varchar(255) default NULL,
  `ip_number` varchar(255) default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `ctf_data`
--

DROP TABLE IF EXISTS `ctf_data`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ctf_data` (
  `id` int(11) NOT NULL auto_increment,
  `time` TIMESTAMP NULL DEFAULT NULL,
  `pid` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `flagtype` int(11) NOT NULL default '0',
  `racewar` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `epic_bonus`
--

DROP TABLE IF EXISTS `epic_bonus`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `epic_bonus` (
  `pid` int(10) NOT NULL,
  `type` int(10) NOT NULL default '0',
  `time` datetime default NULL,
  UNIQUE KEY `pid` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `epic_gain`
--

DROP TABLE IF EXISTS `epic_gain`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `epic_gain` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `event_key` varchar(128) default NULL,
  `pid` bigint(20) NOT NULL default '0',
  `time` datetime NOT NULL,
  `type` int(11) NOT NULL default '0',
  `type_id` int(11) NOT NULL default '0',
  `epics` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `reward_event_key` (`event_key`),
  KEY `pid_index` (`pid`),
  KEY `epic_gain_pid_type_time` (`pid`,`type`,`time`),
  KEY `epic_gain_pid_type_id` (`pid`,`type`,`type_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `eq_drop`
--

DROP TABLE IF EXISTS `eq_drop`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `eq_drop` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `date` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `vnum` int(10) unsigned NOT NULL default '0',
  `pid_looter` bigint(20) unsigned NOT NULL default '0',
  `room_id` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `Index_2` USING BTREE (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `guild_transactions`
--

DROP TABLE IF EXISTS `guild_transactions`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `guild_transactions` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `soc_id` int(10) unsigned NOT NULL default '0',
  `date` int(11) NOT NULL default '0',
  `transaction_info` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`id`),
  KEY `soc_id` (`soc_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `guildhall_rooms`
--

DROP TABLE IF EXISTS `guildhall_rooms`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `guildhall_rooms` (
  `id` int(11) NOT NULL auto_increment,
  `guildhall_id` int(11) NOT NULL default '0',
  `vnum` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `value0` int(10) unsigned NOT NULL default '0',
  `value1` int(10) unsigned NOT NULL default '0',
  `value2` int(10) unsigned NOT NULL default '0',
  `value3` int(10) unsigned NOT NULL default '0',
  `value4` int(10) unsigned NOT NULL default '0',
  `value5` int(10) unsigned NOT NULL default '0',
  `value6` int(10) unsigned NOT NULL default '0',
  `value7` int(10) unsigned NOT NULL default '0',
  `exit0` int(11) NOT NULL default '0',
  `exit1` int(11) NOT NULL default '0',
  `exit2` int(11) NOT NULL default '0',
  `exit3` int(11) NOT NULL default '0',
  `exit4` int(11) NOT NULL default '0',
  `exit5` int(11) NOT NULL default '0',
  `exit6` int(11) NOT NULL default '0',
  `exit7` int(11) NOT NULL default '0',
  `exit8` int(11) NOT NULL default '0',
  `exit9` int(11) NOT NULL,
  `name` varchar(255) NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `vnum` (`vnum`),
  KEY `guildhall_id` (`guildhall_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `guildhalls`
--

DROP TABLE IF EXISTS `guildhalls`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `guildhalls` (
  `id` int(11) NOT NULL auto_increment,
  `assoc_id` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `outside_vnum` int(11) NOT NULL default '0',
  `racewar` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `assoc_id` (`assoc_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `ip_info`
--

DROP TABLE IF EXISTS `ip_info`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ip_info` (
  `pid` bigint(20) NOT NULL default '0',
  `last_ip` varchar(50) NOT NULL default 'none',
  `last_connect` datetime NULL DEFAULT NULL,
  `last_disconnect` datetime NULL DEFAULT NULL,
  `racewar_side` int(11) NOT NULL default '0',
  PRIMARY KEY  (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `items`
--

DROP TABLE IF EXISTS `items`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `items` (
  `vnum` int(10) unsigned NOT NULL default '0',
  `short_desc` varchar(100) NOT NULL default '',
  `obj_stat` text NOT NULL,
  `num_sold` int(11) NOT NULL default '0',
  `avg_sell_price` int(11) NOT NULL default '0',
  PRIMARY KEY  (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `level_cap`
-- Arih: Added missing table definition to fix level cap bug where characters
--        couldn't level past 20 due to empty level_cap table in dev database
--

DROP TABLE IF EXISTS `level_cap`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `level_cap` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `most_frags` float NOT NULL DEFAULT '0',
  `racewar_leader` int(11) NOT NULL DEFAULT '0',
  `level` int(11) NOT NULL DEFAULT '25',
  `next_update` datetime DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `level_cap`
-- NOTE: This table should have only ONE row. The racewar_leader column indicates which side is leading.
--

INSERT INTO `level_cap` (id, most_frags, racewar_leader, level, next_update)
VALUES (1, 0, 2, 56, NOW());

--
-- Table structure for table `log_entries`
--

DROP TABLE IF EXISTS `log_entries`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
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
  KEY `zone_id_index` (`zone_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `mud_info`
--

DROP TABLE IF EXISTS `mud_info`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `mud_info` (
  `name` varchar(255) NOT NULL,
  `content` text NOT NULL,
  PRIMARY KEY  (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

INSERT INTO `mud_info` (`name`, `content`) VALUES
  ('motd', ''),
  ('wizmotd', ''),
  ('news', ''),
  ('rules', ''),
  ('credits', ''),
  ('info', ''),
  ('wizlist', ''),
  ('faq', '')
ON DUPLICATE KEY UPDATE `name` = `name`;

--
-- Table structure for table `multiplay_whitelist`
--

DROP TABLE IF EXISTS `multiplay_whitelist`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `multiplay_whitelist` (
  `id` int(11) NOT NULL auto_increment,
  `pattern` varchar(255) NOT NULL,
  `admin` varchar(255) NOT NULL,
  `description` varchar(255) NOT NULL,
  `created_on` date default NULL,
  `player` varchar(255) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `nexus_stones`
--

DROP TABLE IF EXISTS `nexus_stones`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `nexus_stones` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(255) NOT NULL default '',
  `room_vnum` int(11) NOT NULL default '0',
  `align` int(11) NOT NULL default '0',
  `stat_affect` int(11) NOT NULL default '-1',
  `affect_amount` int(11) NOT NULL default '0',
  `last_touched_at` TIMESTAMP NULL DEFAULT NULL,
  `bonus` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `offline_messages`
--

DROP TABLE IF EXISTS `offline_messages`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `offline_messages` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `date` datetime NOT NULL default '0000-00-00 00:00:00',
  `pid` int(11) NOT NULL default '0',
  `message` text NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `outposts`
--

DROP TABLE IF EXISTS `outposts`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `outposts` (
  `id` int(11) NOT NULL,
  `owner_id` int(11) NOT NULL default '0',
  `level` int(11) NOT NULL default '1',
  `walls` int(11) NOT NULL default '0',
  `archers` int(11) NOT NULL default '0',
  `resources` int(11) NOT NULL default '0',
  `applied_resources` int(11) NOT NULL default '100000',
  `hitpoints` int(11) NOT NULL default '0',
  `territory` int(11) NOT NULL default '0',
  `portal_room` int(11) NOT NULL default '0',
  `golems` int(11) NOT NULL default '0',
  `meurtriere` int(2) NOT NULL default '0',
  `scouts` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `pages`
--

DROP TABLE IF EXISTS `pages`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `pages` (
  `id` int(11) NOT NULL auto_increment,
  `title` varchar(255) default NULL,
  `text` text,
  `last_update` datetime default NULL,
  `last_update_by` varchar(255) default NULL,
  `category_id` int(11) default NULL,
  `ip_number` varchar(255) default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `ping`
--

DROP TABLE IF EXISTS `ping`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ping` (
  `ID` bigint(11) NOT NULL auto_increment,
  `TIMESTAMP` datetime NOT NULL default '0000-00-00 00:00:00',
  `URL` varchar(100) NOT NULL default '',
  `IP` varchar(100) NOT NULL default '',
  `SEQ` bigint(20) NOT NULL default '0',
  `TIME` int(11) NOT NULL default '0',
  PRIMARY KEY  (`ID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `pkill_event`
--

DROP TABLE IF EXISTS `pkill_event`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `pkill_event` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `stamp` datetime NOT NULL default '0000-00-00 00:00:00',
  `room_vnum` int(11) NOT NULL default '0',
  `room_name` text NOT NULL,
  `tweeted` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `pkill_info`
--

DROP TABLE IF EXISTS `pkill_info`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
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
  `player_description` varchar(255),
  PRIMARY KEY  (`id`),
  KEY `part_of_name` (`event_id`),
  KEY `unique_index` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `prepstatment_duris_sql`
--

DROP TABLE IF EXISTS `prepstatment_duris_sql`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `prepstatment_duris_sql` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `desc` text NOT NULL,
  `sql` text NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `progress`
-- var_type values: 1=FRAGS, 2=EXP (matches PROGRESS_* defines in sql.c)
--

DROP TABLE IF EXISTS `progress`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `progress` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` bigint(20) NOT NULL default '0',
  `var_type` int(11) NOT NULL default '1',
  `stamp` datetime NOT NULL,
  `delta` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `pid_index` (`pid`),
  KEY `index_var_type` (`var_type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `racewar_stat_mods`
--

DROP TABLE IF EXISTS `racewar_stat_mods`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
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
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `ship_cargo_market_mods`
--

DROP TABLE IF EXISTS `ship_cargo_market_mods`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ship_cargo_market_mods` (
  `type` varchar(255) NOT NULL default '',
  `port_id` int(11) NOT NULL default '-1',
  `cargo_type` int(11) NOT NULL default '-1',
  `modifier` float NOT NULL default '0',
  KEY `type_port_id_cargo_type` (`type`,`port_id`,`cargo_type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `ship_cargo_prices`
--

DROP TABLE IF EXISTS `ship_cargo_prices`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ship_cargo_prices` (
  `type` varchar(255) NOT NULL default '',
  `port_id` int(11) NOT NULL default '-1',
  `cargo_type` int(11) NOT NULL default '-1',
  `price` int(11) NOT NULL default '0',
  KEY `type_port_id_cargo_type` (`type`,`port_id`,`cargo_type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `shop_trophy`
--

DROP TABLE IF EXISTS `shop_trophy`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `shop_trophy` (
  `id` int(11) NOT NULL auto_increment,
  `item` int(11) NOT NULL default '0',
  `value` int(11) NOT NULL default '0',
  `seller` int(11) NOT NULL default '0',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `statistics`
--

DROP TABLE IF EXISTS `statistics`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `statistics` (
  `id` int(11) NOT NULL auto_increment,
  `date` int(11) NOT NULL default '0',

  `goods_count` int(11) NOT NULL default '0',
  `evils_count` int(11) NOT NULL default '0',
  `illithids_count` int(11) NOT NULL default '0',
  `undeads_count` int(11) NOT NULL default '0',
  `gods_count` int(11) NOT NULL default '0',
  `in_guildhall_count` int(11) NOT NULL default '0',
  `sum_goods_levels` int(11) NOT NULL default '0',
  `sum_evils_levels` int(11) NOT NULL default '0',
  `sum_illithids_levels` int(11) NOT NULL default '0',
  `sum_undeads_levels` int(11) NOT NULL default '0',
  `unique_ips_count` int(11) NOT NULL default '0',

  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `timers`
--

DROP TABLE IF EXISTS `timers`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `timers` (
  `name` varchar(255) NOT NULL default '',
  `date` int(11) NOT NULL default '0',
  PRIMARY KEY  (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `world_quest_accomplished`
--

DROP TABLE IF EXISTS `world_quest_accomplished`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `world_quest_accomplished` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `event_key` varchar(128) default NULL,
  `pid` varchar(45) NOT NULL default '',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `quest_giver` int(10) unsigned NOT NULL default '0',
  `player_name` varchar(45) NOT NULL default '',
  `player_level` int(10) unsigned NOT NULL default '0',
  `quest_target` int(10) NOT NULL default '0',
  `reward_vnum` int(10) NOT NULL default '0',
  `reward_desc` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `reward_event_key` (`event_key`),
  KEY `world_quest_pid_level_time` (`pid`,`player_level`,`timestamp`),
  KEY `world_quest_pid_target` (`pid`,`quest_target`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `zone_trophy`
--

DROP TABLE IF EXISTS `zone_trophy`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `zone_trophy` (
  `pid` bigint(20) NOT NULL default '0',
  `zone_number` int(11) NOT NULL default '0',
  `exp` int(11) NOT NULL default '0',
  PRIMARY KEY  (`pid`,`zone_number`),
  KEY `pid_index` (`pid`),
  KEY `zone_number` (`zone_number`),
  KEY `exp_index` (`exp`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `zones`
--

DROP TABLE IF EXISTS `zones`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `zones` (
  `id` int(10) NOT NULL auto_increment,
  `number` int(10) default NULL,
  `name` varchar(100) NOT NULL default '',
  `epic_type` int(11) NOT NULL default '0',
  `frequency_mod` float NOT NULL default '1',
  `zone_freq_mod` float NOT NULL default '1',
  `epic_level` int(11) NOT NULL default '0',
  `task_zone` tinyint(1) NOT NULL default '0',
  `quest_zone` tinyint(1) NOT NULL default '0',
  `trophy_zone` tinyint(1) NOT NULL default '1',
  `suggested_group_size` int(10) NOT NULL default '1',
  `epic_payout` int(10) NOT NULL default '0',
  `difficulty` int(10) NOT NULL default '0',
  `randoms_zone` tinyint(1) NOT NULL default '1',
  `alignment` int(11) NOT NULL default '0',
  `last_touch` TIMESTAMP NULL DEFAULT NULL,
  `reset_perc` int(10) default '0',
  `stonecount` int(10) NOT NULL default '1',
  PRIMARY KEY  (`id`),
  KEY `number_index` (`number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `corpses`
-- Normalized corpse persistence (see migrations/schema_migration_v2.sql)
--

DROP TABLE IF EXISTS `corpse_item_extra_descr`;
DROP TABLE IF EXISTS `corpse_item_affects`;
DROP TABLE IF EXISTS `corpse_items`;
DROP TABLE IF EXISTS `corpses`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `corpses` (
  `id` int(11) NOT NULL auto_increment,
  `player_name` varchar(50) NOT NULL,
  `save_id` bigint(20) NOT NULL,
  `room_vnum` int(11) default '0',
  `short_descr` varchar(512) default NULL,
  `description` text,
  `created_at` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `uk_player_saveid` (`player_name`,`save_id`),
  KEY `idx_player_name` (`player_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
SET character_set_client = @saved_cs_client;

DROP TABLE IF EXISTS `corpse_items`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `corpse_items` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `corpse_id` int(11) NOT NULL,
  `vnum` int(11) NOT NULL,
  `item_type` int(11) NOT NULL default '0',
  `container_id` int(10) unsigned default NULL,
  `quantity` smallint(5) unsigned default '1',
  `weight` int(11) default '0',
  `cost` int(11) default '0',
  `timer` int(11) default '-1',
  `extra_flags` bigint(20) unsigned default '0',
  `value0` int(11) default '0',
  `value1` int(11) default '0',
  `value2` int(11) default '0',
  `value3` int(11) default '0',
  `value4` int(11) default '0',
  `value5` int(11) default '0',
  `value6` int(11) default '0',
  `value7` int(11) default '0',
  `name` varchar(512) default NULL,
  `short_descr` varchar(512) default NULL,
  `description` text,
  `action_descr` text,
  `obj_uid` bigint(20) unsigned default NULL,
  `item_condition` smallint(6) default '100',
  PRIMARY KEY  (`id`),
  KEY `idx_corpse_id` (`corpse_id`),
  KEY `idx_vnum` (`vnum`),
  KEY `idx_obj_uid` (`obj_uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
SET character_set_client = @saved_cs_client;

DROP TABLE IF EXISTS `corpse_item_affects`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `corpse_item_affects` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `item_id` int(10) unsigned NOT NULL,
  `location` tinyint(3) unsigned default '0',
  `modifier` int(11) default '0',
  PRIMARY KEY  (`id`),
  KEY `idx_item_id` (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
SET character_set_client = @saved_cs_client;

DROP TABLE IF EXISTS `corpse_item_extra_descr`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `corpse_item_extra_descr` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `item_id` int(10) unsigned NOT NULL,
  `keyword` varchar(255) NOT NULL,
  `description` text,
  PRIMARY KEY  (`id`),
  KEY `idx_item_id` (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
SET character_set_client = @saved_cs_client;

--
-- Persistence item ownership tables (async worker; also created at boot in sql.c)
--

DROP TABLE IF EXISTS `persistence_item_conflicts`;
DROP TABLE IF EXISTS `persistence_items_current`;
DROP TABLE IF EXISTS `persistence_item_event_audit`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `persistence_item_event_audit` (
  `event_id` bigint(20) unsigned NOT NULL auto_increment,
  `event_time` bigint(20) unsigned NOT NULL,
  `event_type` varchar(64) NOT NULL,
  `item_uid` bigint(20) unsigned NOT NULL,
  `owner_type` varchar(32) NOT NULL,
  `owner_ref` varchar(64) NOT NULL,
  `actor_id` int(11) NOT NULL default '-1',
  `vnum` int(11) NOT NULL default '-1',
  `item_name` varchar(255) NOT NULL default '',
  `raw_event` text NOT NULL,
  `created_at` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`event_id`),
  KEY `item_time` (`item_uid`,`event_time`),
  KEY `owner_lookup` (`owner_type`,`owner_ref`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

DROP TABLE IF EXISTS `persistence_items_current`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `persistence_items_current` (
  `item_uid` bigint(20) unsigned NOT NULL,
  `owner_type` varchar(32) NOT NULL,
  `owner_ref` varchar(64) NOT NULL,
  `event_time` bigint(20) unsigned NOT NULL,
  `event_type` varchar(64) NOT NULL,
  `actor_id` int(11) NOT NULL default '-1',
  `vnum` int(11) NOT NULL default '-1',
  `item_name` varchar(255) NOT NULL default '',
  `updated_at` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`item_uid`),
  KEY `owner_lookup` (`owner_type`,`owner_ref`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

DROP TABLE IF EXISTS `persistence_item_conflicts`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `persistence_item_conflicts` (
  `conflict_id` bigint(20) unsigned NOT NULL auto_increment,
  `item_uid` bigint(20) unsigned NOT NULL,
  `existing_owner_type` varchar(32) NOT NULL,
  `existing_owner_ref` varchar(64) NOT NULL,
  `existing_event_time` bigint(20) unsigned NOT NULL,
  `incoming_owner_type` varchar(32) NOT NULL,
  `incoming_owner_ref` varchar(64) NOT NULL,
  `incoming_event_time` bigint(20) unsigned NOT NULL,
  `incoming_event_type` varchar(64) NOT NULL,
  `resolution` varchar(64) NOT NULL,
  `raw_event` text NOT NULL,
  `created_at` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`conflict_id`),
  KEY `item_created` (`item_uid`,`created_at`),
  KEY `resolution_lookup` (`resolution`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `zone_touches`
--

DROP TABLE IF EXISTS `zone_touches`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `zone_touches` (
  `id` int(10) NOT NULL auto_increment,
  `event_key` varchar(128) default NULL,
  `boot_time` TIMESTAMP NULL DEFAULT NULL,
  `zone_number` int(11) default NULL,
  `touched_at` TIMESTAMP NULL DEFAULT NULL,
  `toucher_pid` int(10) default NULL,
  `group_size` int(10) default NULL,
  `epic_value` int(11) default NULL,
  `alignment_delta` int(10) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `reward_event_key` (`event_key`),
  KEY `zone_number_index` (`zone_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `artifacts`
-- Arih : artifacts table - main artifact tracking
-- IMPORTANT: Column order matters! The MUD code uses INSERT VALUES without column names
-- INSERT INTO artifacts VALUES( vnum, owned, locType, location, timer, type, SYSDATE())
-- locType values: 1=NotInGame, 2=OnNPC, 3=OnPC, 4=OnGround, 5=OnCorpse (matches artifact.c defines)
--

DROP TABLE IF EXISTS `artifacts`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `artifacts` (
  `vnum` int(11) NOT NULL,
  `owned` char(1) NOT NULL,
  `locType` int(11) NOT NULL default '1',
  `location` int(11) NOT NULL,
  `timer` datetime default NULL,
  `type` int(11) NOT NULL,
  `lastUpdate` datetime default NULL,
  PRIMARY KEY  (`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `artifacts_mortal`
-- Arih : artifacts_mortal table structure matches the INSERT/SELECT query in artifact.c:474
--

DROP TABLE IF EXISTS `artifacts_mortal`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `artifacts_mortal` (
  `vnum` int(11) NOT NULL,
  `owned` char(1) NOT NULL,
  `locType` int(11) NOT NULL,
  `location` int(11) NOT NULL,
  `timer` datetime default NULL,
  `type` int(11) NOT NULL,
  PRIMARY KEY  (`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `locker_access`
-- Storage locker access control table
-- Used in storage_lockers.c for managing locker permissions
--

DROP TABLE IF EXISTS `locker_access`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `locker_access` (
  `owner` varchar(255) NOT NULL,
  `visitor` varchar(255) NOT NULL,
  PRIMARY KEY  (`owner`,`visitor`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2012-10-13 12:13:13
