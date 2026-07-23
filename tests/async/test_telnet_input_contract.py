#!/usr/bin/env python3
"""Source contracts for legacy Telnet input boundary hardening."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
COMM = (ROOT / "src" / "comm.c").read_text()
MCCP = (ROOT / "src" / "mccp.c").read_text()
PROTOTYPES = (ROOT / "src" / "prototypes.h").read_text()


def require(text: str, source: str, message: str) -> None:
    if text not in source:
        raise AssertionError(message)


def test_telnet_parser_waits_for_complete_command() -> None:
    require(
        "if (buflen < 2)",
        MCCP,
        "Telnet parser must not read the command byte until IAC plus command are available",
    )
    require(
        "buflen < 3 &&",
        MCCP,
        "Telnet parser must not read an option byte until IAC, command, and option are available",
    )


def test_process_input_preserves_partial_telnet_sequence() -> None:
    require(
        "memmove(bp, buf + i, (size_t)(len - i));",
        COMM,
        "process_input must retain a fragmented Telnet sequence for the next socket read",
    )
    require(
        "bp += len - i;",
        COMM,
        "retained fragmented Telnet bytes must contribute to descriptor buflen",
    )


def test_queue_dequeue_is_destination_sized() -> None:
    require(
        "int get_from_q(struct txt_q *queue, char *dest, size_t dest_size)",
        COMM,
        "queue dequeue must receive the destination capacity",
    )
    if not re.search(r"int\s+get_from_q\(struct txt_q \*, char \*, size_t\);", PROTOTYPES):
        raise AssertionError("public queue dequeue declaration must require destination capacity")
    require(
        "copy_len = std::min(text_len, dest_size - 1);",
        COMM,
        "queue dequeue must bound the copied text by destination capacity",
    )


if __name__ == "__main__":
    test_telnet_parser_waits_for_complete_command()
    test_process_input_preserves_partial_telnet_sequence()
    test_queue_dequeue_is_destination_sized()
    print("Telnet input hardening contracts passed")
