#!/usr/bin/env python3
"""
Run server-side latency probes against the large DurisMUD load-test database.

The timings are measured inside MySQL with NOW(6)/TIMESTAMPDIFF so Docker exec
startup overhead is not included in the reported query microseconds.
"""

from __future__ import annotations

import argparse
import math
import statistics
import subprocess
import sys
from typing import Sequence


PROBES = [
    (
        "player_pk_lookup",
        "SELECT name,level,money FROM players_core WHERE pid=100123",
    ),
    (
        "locker_access_lookup",
        "SELECT COUNT(*) FROM locker_access WHERE owner='LoadP0124.locker'",
    ),
    (
        "recent_player_log",
        "SELECT COUNT(*) FROM log_entries WHERE pid=100123 AND kind='player'",
    ),
    (
        "zone_trophy_lookup",
        "SELECT zone_number,exp FROM zone_trophy WHERE pid=100123 ORDER BY exp DESC LIMIT 10",
    ),
    (
        "item_owner_lookup",
        "SELECT item_uid,vnum,item_name FROM persistence_items WHERE owner_type='PLAYER' AND owner_id='100123' LIMIT 50",
    ),
    (
        "item_event_history",
        "SELECT event_id,event_type,event_time FROM persistence_item_events WHERE item_uid=900000001234 ORDER BY event_time DESC LIMIT 25",
    ),
    (
        "pending_save_queue",
        "SELECT queue_id,domain,owner_id FROM persistence_save_queue WHERE status='PENDING' ORDER BY priority DESC, queued_at ASC LIMIT 50",
    ),
    (
        "rollback_insert_minimal_wal",
        "START TRANSACTION; "
        "INSERT INTO persistence_event_wal (queued_at,event_type,payload) "
        "VALUES (NOW(6),'probe','rollback minimal wal event payload'); "
        "ROLLBACK",
    ),
    (
        "rollback_insert_item_event",
        "START TRANSACTION; "
        "INSERT INTO persistence_item_events "
        "(event_id,event_time,event_type,item_uid,from_owner_type,from_owner_id,to_owner_type,to_owner_id,actor_pid,money_delta,notes) "
        "VALUES (999999999999,NOW(6),'probe',900000001234,'PLAYER','100123','LOCKER','LoadP0123.locker',100123,0,'rollback probe'); "
        "ROLLBACK",
    ),
    (
        "rollback_update_item_owner",
        "START TRANSACTION; "
        "UPDATE persistence_items SET owner_type='LOCKER', owner_id='LoadP0123.locker', updated_at=NOW(6) "
        "WHERE item_uid=900000001234; "
        "ROLLBACK",
    ),
    (
        "rollback_insert_log",
        "START TRANSACTION; "
        "INSERT INTO log_entries (date,kind,player_name,pid,ip_address,room_vnum,zone_number,message) "
        "VALUES (NOW(),'player','LoadP0123',100123,'10.1.1.1',12345,100,'rollback latency probe'); "
        "ROLLBACK",
    ),
]


def mysql_timing_sql(statement: str) -> str:
    return (
        "SET @probe_start=NOW(6); "
        f"{statement}; "
        "SELECT TIMESTAMPDIFF(MICROSECOND,@probe_start,NOW(6)) AS query_usec"
    )


def run_probe(args, statement: str) -> int:
    command = [
        "docker", "exec", args.mysql_container,
        "mysql",
        f"-u{args.mysql_user}",
        f"-p{args.mysql_password}",
        "--batch",
        "--skip-column-names",
        args.database,
        "-e",
        mysql_timing_sql(statement),
    ]
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())

    lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    for line in reversed(lines):
        try:
            return int(line.split("\t")[-1])
        except ValueError:
            continue
    raise RuntimeError(f"Could not parse timing from mysql output: {result.stdout!r}")


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", default="duris_test")
    parser.add_argument("--mysql-container", default="durismud-mysql-1")
    parser.add_argument("--mysql-user", default="duris")
    parser.add_argument("--mysql-password", default="duris")
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--warn-usec", type=int, default=5000)
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    failed = 0

    print(f"Database latency probes for {args.database} ({args.repeats} repeats)")
    print(f"Warning budget: {args.warn_usec} usec")

    for name, statement in PROBES:
        samples = [run_probe(args, statement) for _ in range(args.repeats)]
        samples.sort()
        avg = statistics.mean(samples)
        p95 = samples[max(0, min(len(samples) - 1, math.ceil(len(samples) * 0.95) - 1))]
        status = "OK" if p95 <= args.warn_usec else "WARN"
        if status == "WARN":
            failed += 1
        print(
            f"{status:4s} {name:28s} min={samples[0]:7d}us "
            f"avg={avg:9.1f}us p95={p95:7d}us max={samples[-1]:7d}us"
        )

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
