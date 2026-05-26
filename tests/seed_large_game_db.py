#!/usr/bin/env python3
"""
Generate and optionally apply a DurisMUD-scale MySQL load dataset.

The default profile models a 500-regular-player game roughly three months into
production. It seeds the current Duris SQL tables that are queried by the code
today, plus synthetic persistence tables for item ownership, transfer events,
corpse recovery, and pending save queue pressure.

The generated data is deterministic by default so timing comparisons are
repeatable between branches.
"""

from __future__ import annotations

import argparse
import os
import random
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Iterable, Iterator, Sequence


RACES = [
    "Human", "Elf", "Dwarf", "Orc", "Troll", "Ogre", "Illithid",
    "Drow", "Halfling", "Gnome", "Draconian", "Undead",
]
CLASSES = [
    "Warrior", "Cleric", "Mage", "Rogue", "Paladin", "Anti-Paladin",
    "Ranger", "Druid", "Psionicist", "Shaman", "Bard", "Monk",
]
SPECS = [
    "none", "berserker", "templar", "invoker", "assassin", "necromancer",
    "huntsman", "earth", "telepath", "totemic", "songblade",
]
EVENTS = [
    "player_save", "autosave", "item_transfer", "item_drop", "item_pickup",
    "corpse_loot", "shop_buy", "shop_sell", "locker_deposit",
    "locker_withdraw", "craft_create", "salvage_destroy", "admin_create",
    "admin_destroy", "money_transfer", "auction_sale",
]
ITEM_NAMES = [
    "blade", "helm", "boots", "ring", "amulet", "cloak", "gauntlets",
    "staff", "wand", "shield", "pouch", "chest", "totem", "quiver",
]
ZONES = [100, 101, 115, 220, 317, 404, 512, 777, 900, 1201, 1600, 31724]


def sql_string(value: object) -> str:
    if value is None:
        return "NULL"
    text = str(value)
    return "'" + text.replace("\\", "\\\\").replace("'", "''") + "'"


def row(values: Sequence[object]) -> str:
    return "(" + ",".join(sql_string(v) if isinstance(v, str) or v is None else str(v) for v in values) + ")"


def chunks(items: Iterable[Sequence[object]], size: int) -> Iterator[list[Sequence[object]]]:
    batch: list[Sequence[object]] = []
    for item in items:
        batch.append(item)
        if len(batch) >= size:
            yield batch
            batch = []
    if batch:
        yield batch


def emit_insert(out, table: str, columns: Sequence[str], rows: Iterable[Sequence[object]], batch_size: int) -> int:
    count = 0
    column_sql = ",".join(f"`{c}`" for c in columns)
    for batch in chunks(rows, batch_size):
        out.write(f"INSERT INTO `{table}` ({column_sql}) VALUES\n")
        out.write(",\n".join(row(values) for values in batch))
        out.write(";\n")
        count += len(batch)
    return count


def create_schema(out, database: str, reset: bool, disable_binlog: bool) -> None:
    if disable_binlog:
        out.write("SET SESSION sql_log_bin=0;\n")
    out.write("SET autocommit=0;\n")
    out.write("SET unique_checks=0;\n")
    out.write("SET foreign_key_checks=0;\n")
    out.write(f"CREATE DATABASE IF NOT EXISTS `{database}`;\n")
    out.write(f"USE `{database}`;\n")

    if reset:
        tables = [
            "persistence_event_wal", "persistence_save_queue", "persistence_corpse_items",
            "persistence_corpse_snapshots", "persistence_item_events",
            "persistence_items", "locker_access", "auction_bid_history",
            "auction_item_pickups", "auction_money_pickups", "auctions",
            "offline_messages", "zone_trophy", "epic_gain", "progress",
            "log_entries", "ip_info", "players_core", "statistics",
            "shop_trophy", "eq_drop", "zones",
        ]
        for table in tables:
            out.write(f"DROP TABLE IF EXISTS `{table}`;\n")

    out.write("""
CREATE TABLE IF NOT EXISTS `players_core` (
  `pid` bigint(20) NOT NULL,
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
  `active` tinyint(1) NOT NULL default '0',
  PRIMARY KEY (`pid`),
  KEY `level` (`level`),
  KEY `racewar` (`racewar`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `ip_info` (
  `pid` bigint(20) NOT NULL,
  `last_ip` varchar(50) NOT NULL default 'none',
  `last_connect` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `last_disconnect` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `racewar_side` int(11) NOT NULL default 0,
  PRIMARY KEY (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `log_entries` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `date` datetime NOT NULL,
  `kind` varchar(255) NOT NULL default '',
  `player_name` varchar(255) NOT NULL default '',
  `pid` int(10) NOT NULL default '0',
  `ip_address` varchar(15) NOT NULL default '',
  `room_vnum` int(10) NOT NULL default '0',
  `zone_number` int(11) NOT NULL default '0',
  `message` varchar(255) NOT NULL default '',
  PRIMARY KEY (`id`),
  KEY `date_index` (`date`),
  KEY `kind_index` (`kind`),
  KEY `name_index` (`player_name`),
  KEY `pid_index` (`pid`),
  KEY `ip_address_index` (`ip_address`),
  KEY `room_vnum_index` (`room_vnum`),
  KEY `zone_id_index` (`zone_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `progress` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` bigint(20) NOT NULL default '0',
  `var_type` enum('FRAGS','EXP') NOT NULL default 'FRAGS',
  `stamp` datetime NOT NULL,
  `delta` int(11) NOT NULL default '0',
  PRIMARY KEY (`id`),
  KEY `pid_index` (`pid`),
  KEY `index_enum` (`var_type`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `epic_gain` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` bigint(20) NOT NULL default '0',
  `time` datetime NOT NULL,
  `type` int(11) NOT NULL default '0',
  `type_id` int(11) NOT NULL default '0',
  `epics` int(11) NOT NULL default '0',
  PRIMARY KEY (`id`),
  KEY `pid_index` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `zone_trophy` (
  `pid` bigint(20) NOT NULL default '0',
  `zone_number` int(11) NOT NULL default '0',
  `exp` int(11) NOT NULL default '0',
  PRIMARY KEY (`pid`,`zone_number`),
  KEY `pid_index` (`pid`),
  KEY `zone_number` (`zone_number`),
  KEY `exp_index` (`exp`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `offline_messages` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `date` datetime NOT NULL,
  `pid` int(11) NOT NULL default '0',
  `message` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `pid_date` (`pid`,`date`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `locker_access` (
  `owner` varchar(255) NOT NULL,
  `visitor` varchar(255) NOT NULL,
  PRIMARY KEY (`owner`,`visitor`),
  KEY `visitor_index` (`visitor`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `auctions` (
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
  PRIMARY KEY (`id`),
  KEY `seller_pid` (`seller_pid`),
  KEY `auction_end` (`end_time`),
  KEY `status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `auction_bid_history` (
  `id` int(11) NOT NULL auto_increment,
  `date` int(11) NOT NULL default '0',
  `auction_id` int(11) NOT NULL default '0',
  `bidder_pid` int(11) NOT NULL default '0',
  `bidder_name` varchar(32) NOT NULL default '',
  `bid_amount` int(11) NOT NULL default '0',
  PRIMARY KEY (`id`),
  KEY `auction_id` (`auction_id`),
  KEY `bidder_pid` (`bidder_pid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `auction_item_pickups` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` int(10) unsigned NOT NULL default '0',
  `obj_blob_str` blob NOT NULL,
  `retrieved` tinyint(1) NOT NULL default '0',
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `auction_money_pickups` (
  `pid` int(10) unsigned NOT NULL default '0',
  `money` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `statistics` (
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
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `shop_trophy` (
  `id` int(11) NOT NULL auto_increment,
  `item` int(11) NOT NULL default '0',
  `value` int(11) NOT NULL default '0',
  `seller` int(11) NOT NULL default '0',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `item_time` (`item`,`timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `eq_drop` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `date` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `vnum` int(10) unsigned NOT NULL default '0',
  `pid_looter` bigint(20) unsigned NOT NULL default '0',
  `room_id` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY (`id`),
  KEY `vnum` (`vnum`),
  KEY `pid_looter` (`pid_looter`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `zones` (
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
  `last_touch` int(10) default '0',
  `reset_perc` int(10) default '0',
  PRIMARY KEY (`id`),
  KEY `number_index` (`number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `persistence_items` (
  `item_uid` bigint unsigned NOT NULL,
  `owner_type` enum('PLAYER','LOCKER','CORPSE','ROOM','AUCTION','SHOP','DESTROYED') NOT NULL,
  `owner_id` varchar(64) NOT NULL,
  `container_uid` bigint unsigned default NULL,
  `vnum` int NOT NULL,
  `item_name` varchar(255) NOT NULL,
  `item_value` int NOT NULL default 0,
  `updated_at` datetime(6) NOT NULL,
  PRIMARY KEY (`item_uid`),
  KEY `owner_lookup` (`owner_type`,`owner_id`),
  KEY `container_uid` (`container_uid`),
  KEY `updated_at` (`updated_at`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `persistence_item_events` (
  `event_id` bigint unsigned NOT NULL,
  `event_time` datetime(6) NOT NULL,
  `event_type` varchar(32) NOT NULL,
  `item_uid` bigint unsigned NOT NULL,
  `from_owner_type` varchar(16) NOT NULL,
  `from_owner_id` varchar(64) NOT NULL,
  `to_owner_type` varchar(16) NOT NULL,
  `to_owner_id` varchar(64) NOT NULL,
  `actor_pid` bigint NOT NULL default 0,
  `money_delta` int NOT NULL default 0,
  `notes` varchar(255) NOT NULL default '',
  PRIMARY KEY (`event_id`),
  KEY `item_time` (`item_uid`,`event_time`),
  KEY `actor_time` (`actor_pid`,`event_time`),
  KEY `to_owner_lookup` (`to_owner_type`,`to_owner_id`),
  KEY `event_type_time` (`event_type`,`event_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `persistence_corpse_snapshots` (
  `corpse_id` bigint unsigned NOT NULL,
  `player_pid` bigint NOT NULL,
  `player_name` varchar(255) NOT NULL,
  `room_vnum` int NOT NULL,
  `created_at` datetime(6) NOT NULL,
  `refreshed_until` datetime(6) NOT NULL,
  `status` enum('ACTIVE','LOOTED','EXPIRED') NOT NULL default 'ACTIVE',
  PRIMARY KEY (`corpse_id`),
  KEY `player_status` (`player_pid`,`status`),
  KEY `status_expiry` (`status`,`refreshed_until`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `persistence_corpse_items` (
  `corpse_id` bigint unsigned NOT NULL,
  `item_uid` bigint unsigned NOT NULL,
  `container_uid` bigint unsigned default NULL,
  `vnum` int NOT NULL,
  `item_name` varchar(255) NOT NULL,
  PRIMARY KEY (`corpse_id`,`item_uid`),
  KEY `item_uid` (`item_uid`),
  KEY `container_uid` (`container_uid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `persistence_save_queue` (
  `queue_id` bigint unsigned NOT NULL auto_increment,
  `queued_at` datetime(6) NOT NULL,
  `domain` varchar(32) NOT NULL,
  `owner_id` varchar(64) NOT NULL,
  `priority` tinyint NOT NULL default 1,
  `payload_bytes` int NOT NULL default 0,
  `attempts` int NOT NULL default 0,
  `status` enum('PENDING','PROCESSING','DONE','FAILED') NOT NULL default 'PENDING',
  PRIMARY KEY (`queue_id`),
  KEY `status_priority` (`status`,`priority`,`queued_at`),
  KEY `status_priority_dequeue` (`status`,`priority` DESC,`queued_at`,`queue_id`,`domain`,`owner_id`),
  KEY `owner_lookup` (`domain`,`owner_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
CREATE TABLE IF NOT EXISTS `persistence_event_wal` (
  `wal_id` bigint unsigned NOT NULL auto_increment,
  `queued_at` datetime(6) NOT NULL,
  `event_type` varchar(32) NOT NULL,
  `payload` text NOT NULL,
  PRIMARY KEY (`wal_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
""")


def player_rows(args, rng: random.Random) -> list[dict[str, object]]:
    rows = []
    for i in range(args.players):
        pid = 100000 + i
        level = int(min(56, max(1, rng.gauss(34, 13))))
        racewar = rng.choice([-1, 1, 2, 3])
        rows.append(
            {
                "pid": pid,
                "name": f"LoadP{i + 1:04d}",
                "race": rng.choice(RACES),
                "classname": rng.choice(CLASSES),
                "spec": rng.choice(SPECS),
                "guild": f"Guild{rng.randint(1, 28):02d}",
                "webinfo_toggle": rng.randint(0, 1),
                "racewar": racewar,
                "level": level,
                "money": rng.randint(0, 2500000),
                "balance": rng.randint(0, 6000000),
                "playtime": rng.randint(3 * 3600, args.days * 8 * 3600),
                "epics": max(0, int(rng.gauss(level * 2, 20))),
                "active": 1 if rng.random() < 0.82 else 0,
                "ip": f"10.{rng.randint(1, 240)}.{rng.randint(0, 255)}.{rng.randint(1, 254)}",
            }
        )
    return rows


def dt_from_day(base_epoch: int, day: int, rng: random.Random) -> str:
    second = rng.randint(0, 86399)
    return time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(base_epoch + day * 86400 + second))


def generate_data(out, args) -> dict[str, int]:
    rng = random.Random(args.seed)
    base_epoch = int(time.time()) - args.days * 86400
    players = player_rows(args, rng)
    batch = args.batch_size
    counts: dict[str, int] = {}

    create_schema(out, args.database, args.reset, args.disable_binlog)

    counts["zones"] = emit_insert(
        out,
        "zones",
        ["number", "name", "epic_type", "frequency_mod", "zone_freq_mod", "epic_level",
         "task_zone", "quest_zone", "trophy_zone", "suggested_group_size", "epic_payout",
         "difficulty", "randoms_zone", "alignment", "last_touch", "reset_perc"],
        (
            [z, f"Load Test Zone {z}", rng.randint(0, 3), 1, 1, rng.randint(0, 56),
             rng.randint(0, 1), rng.randint(0, 1), 1, rng.randint(1, 8),
             rng.randint(0, 500), rng.randint(1, 10), 1, rng.randint(-500, 500),
             base_epoch + rng.randint(0, args.days * 86400), rng.randint(0, 100)]
            for z in ZONES
        ),
        batch,
    )

    counts["players_core"] = emit_insert(
        out,
        "players_core",
        ["pid", "name", "race", "classname", "spec", "guild", "webinfo_toggle",
         "racewar", "level", "money", "balance", "playtime", "epics", "active"],
        (
            [p["pid"], p["name"], p["race"], p["classname"], p["spec"], p["guild"],
             p["webinfo_toggle"], p["racewar"], p["level"], p["money"], p["balance"],
             p["playtime"], p["epics"], p["active"]]
            for p in players
        ),
        batch,
    )

    counts["ip_info"] = emit_insert(
        out,
        "ip_info",
        ["pid", "last_ip", "last_connect", "last_disconnect", "racewar_side"],
        (
            [p["pid"], p["ip"], dt_from_day(base_epoch, args.days - 1, rng),
             dt_from_day(base_epoch, args.days - 1, rng), p["racewar"]]
            for p in players
        ),
        batch,
    )

    def per_player_day_rows(multiplier: int, maker):
        for p in players:
            for day in range(args.days):
                for _ in range(multiplier):
                    yield maker(p, day)

    counts["log_entries"] = emit_insert(
        out,
        "log_entries",
        ["date", "kind", "player_name", "pid", "ip_address", "room_vnum", "zone_number", "message"],
        per_player_day_rows(
            args.log_events_per_player_day,
            lambda p, day: [
                dt_from_day(base_epoch, day, rng), rng.choice(["player", "wiz", "quest", "exp", "connect"]),
                p["name"], p["pid"], p["ip"], rng.randint(1000, 98000), rng.choice(ZONES),
                f"{rng.choice(EVENTS)} synthetic load event value={rng.randint(1, 999999)}",
            ],
        ),
        batch,
    )

    counts["progress"] = emit_insert(
        out,
        "progress",
        ["pid", "var_type", "stamp", "delta"],
        per_player_day_rows(
            args.progress_events_per_player_day,
            lambda p, day: [
                p["pid"], rng.choice(["EXP", "EXP", "EXP", "FRAGS"]),
                dt_from_day(base_epoch, day, rng), rng.randint(-35000, 250000),
            ],
        ),
        batch,
    )

    counts["epic_gain"] = emit_insert(
        out,
        "epic_gain",
        ["pid", "time", "type", "type_id", "epics"],
        (
            [p["pid"], dt_from_day(base_epoch, rng.randrange(args.days), rng), rng.randint(0, 4),
             rng.choice(ZONES), rng.randint(1, 8)]
            for p in players
            for _ in range(args.epic_events_per_player)
        ),
        batch,
    )

    counts["zone_trophy"] = emit_insert(
        out,
        "zone_trophy",
        ["pid", "zone_number", "exp"],
        (
            [p["pid"], z, rng.randint(5000, 9000000)]
            for p in players
            for z in rng.sample(ZONES, k=min(len(ZONES), args.zone_trophies_per_player))
        ),
        batch,
    )

    item_uid_start = 900000000000
    item_rows = []
    item_count = args.players * args.items_per_player
    for i in range(item_count):
        p = players[i % len(players)]
        uid = item_uid_start + i
        owner_roll = rng.random()
        if owner_roll < 0.58:
            owner_type, owner_id = "PLAYER", str(p["pid"])
        elif owner_roll < 0.87:
            owner_type, owner_id = "LOCKER", f"{p['name']}.locker"
        elif owner_roll < 0.94:
            owner_type, owner_id = "ROOM", str(rng.randint(1000, 98000))
        elif owner_roll < 0.98:
            owner_type, owner_id = "AUCTION", str(rng.randint(1, max(1, args.auctions)))
        else:
            owner_type, owner_id = "CORPSE", str(700000 + rng.randint(0, max(1, args.corpses) - 1))
        vnum = 10000 + rng.randint(1, 35000)
        item_rows.append([uid, owner_type, owner_id, None, vnum,
                          f"{rng.choice(ITEM_NAMES)} of load testing {vnum}",
                          rng.randint(1, 250), dt_from_day(base_epoch, rng.randrange(args.days), rng)])

    counts["persistence_items"] = emit_insert(
        out,
        "persistence_items",
        ["item_uid", "owner_type", "owner_id", "container_uid", "vnum", "item_name", "item_value", "updated_at"],
        item_rows,
        batch,
    )

    event_total = args.players * args.days * args.item_events_per_player_day
    counts["persistence_item_events"] = emit_insert(
        out,
        "persistence_item_events",
        ["event_id", "event_time", "event_type", "item_uid", "from_owner_type", "from_owner_id",
         "to_owner_type", "to_owner_id", "actor_pid", "money_delta", "notes"],
        (
            [
                500000000000 + i,
                dt_from_day(base_epoch, (i // max(1, args.players * args.item_events_per_player_day)) % args.days, rng),
                rng.choice(EVENTS),
                item_uid_start + rng.randrange(item_count),
                rng.choice(["PLAYER", "LOCKER", "CORPSE", "ROOM", "AUCTION", "SHOP"]),
                str(rng.choice(players)["pid"]),
                rng.choice(["PLAYER", "LOCKER", "CORPSE", "ROOM", "AUCTION", "SHOP", "DESTROYED"]),
                str(rng.choice(players)["pid"]),
                rng.choice(players)["pid"],
                rng.randint(-200000, 200000) if rng.random() < 0.18 else 0,
                "synthetic persistence transfer",
            ]
            for i in range(event_total)
        ),
        batch,
    )

    counts["persistence_corpse_snapshots"] = emit_insert(
        out,
        "persistence_corpse_snapshots",
        ["corpse_id", "player_pid", "player_name", "room_vnum", "created_at", "refreshed_until", "status"],
        (
            [
                700000 + i,
                (p := rng.choice(players))["pid"],
                p["name"],
                rng.randint(1000, 98000),
                dt_from_day(base_epoch, rng.randrange(args.days), rng),
                dt_from_day(base_epoch, args.days - 1, rng),
                rng.choice(["ACTIVE", "ACTIVE", "LOOTED", "EXPIRED"]),
            ]
            for i in range(args.corpses)
        ),
        batch,
    )

    counts["persistence_corpse_items"] = emit_insert(
        out,
        "persistence_corpse_items",
        ["corpse_id", "item_uid", "container_uid", "vnum", "item_name"],
        (
            [
                700000 + (i // max(1, args.items_per_corpse)),
                item_uid_start + (i % item_count),
                None,
                10000 + rng.randint(1, 35000),
                f"{rng.choice(ITEM_NAMES)} from corpse recovery",
            ]
            for i in range(args.corpses * args.items_per_corpse)
        ),
        batch,
    )

    counts["persistence_save_queue"] = emit_insert(
        out,
        "persistence_save_queue",
        ["queued_at", "domain", "owner_id", "priority", "payload_bytes", "attempts", "status"],
        (
            [
                dt_from_day(base_epoch, args.days - 1, rng),
                rng.choice(["player", "corpse", "locker", "item_event"]),
                str(rng.choice(players)["pid"]),
                rng.randint(0, 3),
                rng.randint(250, 65000),
                rng.randint(0, 4),
                rng.choice(["PENDING", "PENDING", "PROCESSING", "FAILED"]),
            ]
            for _ in range(args.pending_saves)
        ),
        batch,
    )

    counts["persistence_event_wal"] = emit_insert(
        out,
        "persistence_event_wal",
        ["queued_at", "event_type", "payload"],
        (
            [
                dt_from_day(base_epoch, args.days - 1, rng),
                rng.choice(EVENTS),
                f"uid={item_uid_start + rng.randrange(item_count)} actor={rng.choice(players)['pid']} transfer={rng.randint(1, 1000000)}",
            ]
            for _ in range(args.wal_events)
        ),
        batch,
    )

    counts["locker_access"] = emit_insert(
        out,
        "locker_access",
        ["owner", "visitor"],
        (
            [f"{owner['name']}.locker", visitor["name"]]
            for owner in players
            for visitor in rng.sample(players, k=min(args.locker_visitors_per_player, len(players)))
            if visitor["pid"] != owner["pid"]
        ),
        batch,
    )

    counts["auctions"] = emit_insert(
        out,
        "auctions",
        ["seller_pid", "seller_name", "start_time", "end_time", "status", "winning_bidder_pid",
         "winning_bidder_name", "cur_price", "buy_price", "obj_short", "obj_vnum", "obj_blob_str", "id_keywords"],
        (
            [
                (seller := rng.choice(players))["pid"],
                seller["name"],
                base_epoch + rng.randint(0, args.days * 86400),
                base_epoch + rng.randint(0, args.days * 86400) + 86400,
                rng.choice(["OPEN", "CLOSED", "CLOSED", "REMOVED"]),
                (bidder := rng.choice(players))["pid"],
                bidder["name"],
                rng.randint(100, 2000000),
                rng.randint(1000, 5000000),
                f"{rng.choice(ITEM_NAMES)} auction item",
                10000 + rng.randint(1, 35000),
                "synthetic-object-blob",
                rng.choice(ITEM_NAMES),
            ]
            for _ in range(args.auctions)
        ),
        batch,
    )

    counts["auction_bid_history"] = emit_insert(
        out,
        "auction_bid_history",
        ["date", "auction_id", "bidder_pid", "bidder_name", "bid_amount"],
        (
            [
                base_epoch + rng.randint(0, args.days * 86400),
                rng.randint(1, max(1, args.auctions)),
                (bidder := rng.choice(players))["pid"],
                bidder["name"],
                rng.randint(100, 3000000),
            ]
            for _ in range(args.auctions * args.bids_per_auction)
        ),
        batch,
    )

    counts["auction_item_pickups"] = emit_insert(
        out,
        "auction_item_pickups",
        ["pid", "obj_blob_str", "retrieved"],
        ([rng.choice(players)["pid"], "synthetic-unretrieved-auction-item", 0] for _ in range(args.auction_pickups)),
        batch,
    )

    counts["auction_money_pickups"] = emit_insert(
        out,
        "auction_money_pickups",
        ["pid", "money"],
        ([p["pid"], rng.randint(1000, 5000000)] for p in rng.sample(players, k=min(args.auction_money_pickups, len(players)))),
        batch,
    )

    counts["offline_messages"] = emit_insert(
        out,
        "offline_messages",
        ["date", "pid", "message"],
        (
            [dt_from_day(base_epoch, rng.randrange(args.days), rng), rng.choice(players)["pid"],
             "synthetic offline message"]
            for _ in range(args.offline_messages)
        ),
        batch,
    )

    counts["shop_trophy"] = emit_insert(
        out,
        "shop_trophy",
        ["item", "value", "seller", "timestamp"],
        (
            [10000 + rng.randint(1, 35000), rng.randint(100, 1000000),
             rng.choice(players)["pid"], dt_from_day(base_epoch, rng.randrange(args.days), rng)]
            for _ in range(args.shop_sales)
        ),
        batch,
    )

    counts["eq_drop"] = emit_insert(
        out,
        "eq_drop",
        ["date", "vnum", "pid_looter", "room_id"],
        (
            [dt_from_day(base_epoch, rng.randrange(args.days), rng), 10000 + rng.randint(1, 35000),
             rng.choice(players)["pid"], rng.randint(1000, 98000)]
            for _ in range(args.eq_drops)
        ),
        batch,
    )

    counts["statistics"] = emit_insert(
        out,
        "statistics",
        ["date", "goods_count", "evils_count", "illithids_count", "undeads_count", "gods_count",
         "in_guildhall_count", "sum_goods_levels", "sum_evils_levels", "sum_illithids_levels",
         "sum_undeads_levels", "unique_ips_count"],
        (
            [
                base_epoch + day * 86400 + hour * 3600,
                rng.randint(40, 180), rng.randint(35, 170), rng.randint(8, 55), rng.randint(8, 55),
                rng.randint(0, 8), rng.randint(0, 45),
                rng.randint(500, 7000), rng.randint(500, 7000),
                rng.randint(150, 2500), rng.randint(150, 2500), rng.randint(30, 190),
            ]
            for day in range(args.days)
            for hour in range(24)
        ),
        batch,
    )

    out.write("COMMIT;\n")
    out.write("SET foreign_key_checks=1;\n")
    out.write("SET unique_checks=1;\n")
    out.write("SET autocommit=1;\n")
    out.write("-- Row count summary\n")
    for table, count in sorted(counts.items()):
        out.write(f"-- {table}: {count}\n")

    return counts


def write_sql(args) -> tuple[Path, dict[str, int]]:
    if args.sql_out:
        path = Path(args.sql_out)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", encoding="utf-8", newline="\n") as out:
            counts = generate_data(out, args)
        return path, counts

    handle = tempfile.NamedTemporaryFile("w", encoding="utf-8", newline="\n", suffix=".sql", delete=False)
    with handle:
        counts = generate_data(handle, args)
    return Path(handle.name), counts


def apply_sql(path: Path, args) -> None:
    command = [
        "docker", "exec", "-i", args.mysql_container,
        "mysql",
        f"-u{args.mysql_user}",
        f"-p{args.mysql_password}",
    ]

    start = time.perf_counter()
    with path.open("rb") as sql_file:
        result = subprocess.run(command, stdin=sql_file)
    elapsed = time.perf_counter() - start

    if result.returncode != 0:
        raise SystemExit(result.returncode)

    print(f"Applied {path} through {args.mysql_container} in {elapsed:.2f}s")


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", default="duris_test")
    parser.add_argument("--players", type=int, default=500)
    parser.add_argument("--days", type=int, default=90)
    parser.add_argument("--seed", type=int, default=777)
    parser.add_argument("--batch-size", type=int, default=500)
    parser.add_argument("--items-per-player", type=int, default=160)
    parser.add_argument("--item-events-per-player-day", type=int, default=18)
    parser.add_argument("--log-events-per-player-day", type=int, default=8)
    parser.add_argument("--progress-events-per-player-day", type=int, default=4)
    parser.add_argument("--epic-events-per-player", type=int, default=12)
    parser.add_argument("--zone-trophies-per-player", type=int, default=8)
    parser.add_argument("--corpses", type=int, default=1500)
    parser.add_argument("--items-per-corpse", type=int, default=16)
    parser.add_argument("--pending-saves", type=int, default=25000)
    parser.add_argument("--wal-events", type=int, default=25000)
    parser.add_argument("--locker-visitors-per-player", type=int, default=4)
    parser.add_argument("--auctions", type=int, default=12000)
    parser.add_argument("--bids-per-auction", type=int, default=5)
    parser.add_argument("--auction-pickups", type=int, default=6000)
    parser.add_argument("--auction-money-pickups", type=int, default=350)
    parser.add_argument("--offline-messages", type=int, default=8000)
    parser.add_argument("--shop-sales", type=int, default=50000)
    parser.add_argument("--eq-drops", type=int, default=50000)
    parser.add_argument("--sql-out", default="")
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--mysql-container", default=os.environ.get("DURIS_MYSQL_CONTAINER", "durismud-mysql-1"))
    parser.add_argument("--mysql-user", default=os.environ.get("DURIS_MYSQL_USER", "duris"))
    parser.add_argument("--mysql-password", default=os.environ.get("DURIS_MYSQL_PASSWORD", "duris"))
    parser.add_argument("--no-reset", dest="reset", action="store_false")
    parser.add_argument(
        "--disable-binlog",
        action="store_true",
        help="emit SET SESSION sql_log_bin=0; requires a privileged MySQL user",
    )
    parser.set_defaults(reset=True)
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    start = time.perf_counter()
    path, counts = write_sql(args)
    elapsed = time.perf_counter() - start
    total = sum(counts.values())

    print(f"Generated {path} in {elapsed:.2f}s")
    print(f"Total inserted rows: {total}")
    for table, count in sorted(counts.items()):
        print(f"  {table}: {count}")

    if args.apply:
        apply_sql(path, args)
        if not args.sql_out:
            path.unlink(missing_ok=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
