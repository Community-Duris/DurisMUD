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


def test_input_queue_is_bounded_and_accounted() -> None:
    structs = (ROOT / "src" / "structs.h").read_text()
    config = (ROOT / "src" / "config.h").read_text()
    require("size_t            bytes;", structs, "text queues must account queued bytes")
    require("size_t            blocks;", structs, "text queues must account queued entries")
    require("MAX_INPUT_QUEUE_BYTES", config, "input queues need an explicit byte ceiling")
    require("MAX_INPUT_QUEUE_COMMANDS", config, "input queues need an explicit command ceiling")
    require("MAX_INPUT_LINES_PER_READ", config, "socket reads need a per-event line-work ceiling")
    require("int write_to_input_q(P_desc d, const char *txt)", COMM, "descriptor input must use the bounded input queue API")
    require("queue->bytes += txtlen;", COMM, "queue writes must update byte accounting")
    require("queue->blocks++;", COMM, "queue writes must update entry accounting")
    require("queue->bytes -= text_len;", COMM, "queue reads must release byte accounting")
    require("queue->blocks--;", COMM, "queue reads must release entry accounting")
    require("if (++lines_processed > MAX_INPUT_LINES_PER_READ)", COMM, "socket input must cap lines handled per read")
    require("write_to_input_q(t, out)", COMM, "Telnet lines must use the bounded descriptor input queue")
    for relative in ("src/websocket.c", "src/ws_handlers.c"):
        source = (ROOT / relative).read_text()
        if re.search(r"write_to_q\([^\n]*&d->input", source):
            raise AssertionError(f"{relative} bypasses the bounded descriptor input queue")


def test_pre_auth_terminal_selection_uses_full_input_capacity() -> None:
    nanny = (ROOT / "src" / "nanny.c").read_text()
    require("char temp_buf[MAX_INPUT_LENGTH];", nanny, "terminal parsing token buffer must match accepted input capacity")
    if "strcpy(d->client_str, arg);" in nanny:
        raise AssertionError("pre-auth terminal parsing must bound copies into client_str")
    require("strlcpy(d->client_str, arg, sizeof(d->client_str));", nanny, "client identifier copies must be destination-sized")


def test_account_confirmation_does_not_echo_stored_state() -> None:
    account = (ROOT / "src" / "account.c").read_text()
    if "DEBUG: Stored=" in account or "SEND_TO_Q(debug_buf, d);" in account:
        raise AssertionError("account confirmation must not echo stored authentication state")


if __name__ == "__main__":
    test_telnet_parser_waits_for_complete_command()
    test_process_input_preserves_partial_telnet_sequence()
    test_queue_dequeue_is_destination_sized()
    test_input_queue_is_bounded_and_accounted()
    test_pre_auth_terminal_selection_uses_full_input_capacity()
    test_account_confirmation_does_not_echo_stored_state()
    print("Telnet input hardening contracts passed")
