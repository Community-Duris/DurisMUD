USE `duris_test`;

CREATE TABLE IF NOT EXISTS `persistence_event_wal` (
  `wal_id` bigint unsigned NOT NULL auto_increment,
  `queued_at` datetime(6) NOT NULL,
  `event_type` varchar(32) NOT NULL,
  `payload` text NOT NULL,
  PRIMARY KEY (`wal_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

ALTER TABLE `persistence_save_queue`
  ADD INDEX `status_priority_dequeue`
  (`status`,`priority` DESC,`queued_at`,`queue_id`,`domain`,`owner_id`);
