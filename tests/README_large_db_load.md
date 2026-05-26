# Large Database Load Seed

`seed_large_game_db.py` creates a deterministic MySQL dataset shaped like a
DurisMUD database with 500 regular players after roughly 90 days of production.
It seeds current Duris tables plus synthetic persistence tables for the database
model being implemented.

The default profile creates:

- 500 players and IP records
- 360,000 player log rows
- 180,000 progress rows
- 810,000 item transfer/persistence events
- 80,000 current item ownership rows
- 24,000 corpse item rows
- auctions, auction bids, locker access, shop sales, eq drops, statistics, and
  pending save queue rows

Generate SQL only:

```sh
python tests/seed_large_game_db.py --sql-out tests/generated_large_game_load.sql
```

Generate and apply to the local MySQL Docker container:

```sh
python tests/seed_large_game_db.py --apply --database duris_test
```

The script applies through:

```sh
docker exec -i durismud-mysql-1 mysql -uduris -pduris
```

Useful scale knobs:

```sh
python tests/seed_large_game_db.py \
  --players 500 \
  --days 90 \
  --item-events-per-player-day 18 \
  --items-per-player 160 \
  --pending-saves 25000 \
  --apply
```

For quick smoke tests:

```sh
python tests/seed_large_game_db.py --players 20 --days 3 --sql-out tests/generated_smoke_load.sql
```

Generated SQL files are ignored by Git as `tests/generated_*.sql`.

After applying a load profile, run the latency probes:

```sh
python tests/probe_large_db_latency.py --database duris_test --repeats 5
```

The probe reports MySQL server-side microsecond timings for representative
read/write paths: player lookup, locker access, recent logs, item ownership,
item transfer history, pending save queue, and rollback-wrapped writes.
