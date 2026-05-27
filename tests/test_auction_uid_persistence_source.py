#!/usr/bin/env python3
"""Source-level guard for auction item UID persistence.

Auction quantity listings historically store one object blob plus a quantity.
With persistent item UIDs, cloning that blob would duplicate one UID. Single
item auctions can preserve identity through escrow; quantity auctions must not
serialize the UID until the schema can store per-item blobs.
"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="latin1")


def section(text: str, start: str, end: str) -> str:
    start_at = text.index(start)
    end_at = text.index(end, start_at)
    return text[start_at:end_at]


def assert_contains(text: str, needle: str, message: str) -> None:
    if needle not in text:
        raise AssertionError(message)


def assert_order(text: str, before: str, after: str, message: str) -> None:
    before_at = text.index(before)
    after_at = text.index(after)
    if before_at > after_at:
        raise AssertionError(message)


def main() -> int:
    auction = read("src/auction_houses.c")
    files = read("src/files.c")
    handler = read("src/handler.c")
    prototypes = read("src/prototypes.h")

    offer = section(auction, "bool auction_offer", "// syntax: auction list")
    write_one = section(files, "int write_one_object", "return (buff - start);")
    extract = section(handler, "void extract_obj", "Generic Find")

    assert_contains(
        prototypes,
        "int  write_one_object(P_obj, char *, int include_persistent_uid = TRUE);",
        "write_one_object should expose an opt-out for UID serialization",
    )
    assert_contains(
        write_one,
        "if (include_persistent_uid)",
        "object serialization should only write UID when requested",
    )
    assert_contains(
        offer,
        "write_one_object(tmp_obj, obj_buff_ptr,",
        "auction offer should choose whether the serialized blob includes a UID",
    )
    assert_contains(
        offer,
        "auction_quantity == 1",
        "single-item auctions should preserve UID identity",
    )
    assert_contains(
        offer,
        "persistence_record_item_event(\"owner_auction\"",
        "single-item auctions should move ownership to auction escrow",
    )
    assert_contains(
        offer,
        "auction_quantity_serialized_without_uid",
        "quantity auctions should document UID-free serialization",
    )
    assert_contains(
        offer,
        "extract_obj(tmp_obj, auction_quantity > 1)",
        "quantity auction originals should be removed as transformed source items",
    )
    assert_contains(
        extract,
        "if (gone_for_good)",
        "ordinary extract_obj calls should not mark escrow/transient moves destroyed",
    )
    assert_order(
        offer,
        "persistence_record_item_event(\"owner_auction\"",
        "extract_obj(tmp_obj, auction_quantity > 1)",
        "auction ownership should be recorded before removing the in-memory item",
    )

    print("auction UID persistence source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
