#!/usr/bin/env python3
"""Regression contracts for the WebSocket parser and output hardening slices.

The runtime socket-pair harness exercises the authoritative handshake, frame,
and nonblocking output paths. These source contracts keep the RFC 6455 safety
rules and event-loop integration reviewable alongside it.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "src/websocket.h").read_text()
SOURCE = (ROOT / "src/websocket.c").read_text()
WS_HANDLERS = (ROOT / "src/ws_handlers.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
GMCP = (ROOT / "src/gmcp.c").read_text()
MCCP = (ROOT / "src/mccp.c").read_text()
GMCP_H = (ROOT / "src/gmcp.h").read_text()
HARNESS = (ROOT / "tests/async/websocket_runtime_harness.cpp").read_text()
AUTH = (ROOT / "src/ws_auth.h").read_text()


def function_body(source: str, signature: str, next_signature: str) -> str:
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


parse_frame = function_body(
    SOURCE,
    "int websocket_parse_frame(",
    "/* close websocket connection properly */",
)
process_input = function_body(
    SOURCE,
    "int websocket_process_input(",
    "/* check if descriptor has pending websocket data */",
)
parse_handshake = function_body(
    SOURCE,
    "int websocket_parse_handshake(",
    "/* send http upgrade response */",
)


def test_client_frames_require_masking_before_payload_processing():
    assert "if (!mask_bit)" in parse_frame
    assert "must always be masked" in parse_frame


def test_reserved_bits_and_opcodes_are_rejected():
    assert "RSV2" in parse_frame or "0x30" in parse_frame
    assert "!d->ws_compress" in parse_frame
    assert "WS_OPCODE_CONTINUATION" in parse_frame
    assert "WS_OPCODE_TEXT" in parse_frame
    assert "WS_OPCODE_BINARY" in parse_frame
    assert "WS_OPCODE_CLOSE" in parse_frame
    assert "WS_OPCODE_PING" in parse_frame
    assert "WS_OPCODE_PONG" in parse_frame
    assert "reserved" in parse_frame.lower() or "invalid opcode" in parse_frame.lower()


def test_control_frames_are_not_fragmented_or_oversized():
    assert "!fin" in parse_frame
    assert "125" in parse_frame
    assert "control" in parse_frame.lower()


def test_fragmented_input_has_an_aggregate_cap():
    assert "WS_MAX_BUFFERED_BYTES" in HEADER
    assert "WS_MAX_MESSAGE_SIZE" in HEADER
    assert "ws_message_len + payload_len" in process_input
    assert "ws_fragment_len + bytes_read" in process_input

def test_decompression_does_not_deliver_truncated_output():
    assert "avail_out > 0" in parse_frame
    assert "WS_MAX_MESSAGE_SIZE" in parse_frame
    assert "inflate" in parse_frame


def test_runtime_harness_covers_compression_reset_and_rejection():
    assert "compressed-one" in HARNESS
    assert "compressed-two" in HARNESS
    assert "inflater state was not reset between compressed frames" in HARNESS
    assert "fragmented-compressed" in HARNESS
    assert "compressed continuation was not decompressed" in HARNESS
    assert "invalid compressed payload" in HARNESS


def test_handshake_preserves_coalesced_websocket_bytes():
    assert "header_bytes" in process_input
    assert "ws_fragment_buffer" in process_input
    assert "ws_handshake_buffer + header_bytes" in process_input
    assert "bytes_read = 0" in process_input
    assert "if (d->ws_fragment_len == 0)" in process_input or "if (!d->ws_fragment_len)" in process_input


def test_handshake_has_a_real_deadline_outside_connected_states():
    assert "WS_HANDSHAKE_TIMEOUT" in HEADER
    assert "!point->ws_handshake_done" in COMM
    assert "ws_handshake_started" in COMM
    assert "WS_HANDSHAKE_TIMEOUT" in COMM


def test_websocket_output_backpressure_is_queued_and_retried():
    assert "WS_MAX_OUTPUT_BYTES" in HEADER
    assert "WS_CONTROL_OUTPUT_RESERVE" in HEADER
    assert "ws_control_output_buffer" in SOURCE
    assert "websocket_queue_output" in SOURCE
    assert "opcode >= WS_OPCODE_CLOSE" in SOURCE
    assert "EAGAIN" in SOURCE and "EWOULDBLOCK" in SOURCE
    assert "websocket_flush_output(point)" in COMM
    assert "WS_STATE_CLOSING" in COMM
    assert "ws_control_output_len == 0" in COMM
    assert "websocket_valid_utf8" in SOURCE
    assert "websocket_valid_close_code" in SOURCE
    assert "data_len == 1" in SOURCE
    assert "op >= WS_OPCODE_CLOSE" in SOURCE
    assert "d->write_failed = 1" in SOURCE


def test_legacy_binary_output_cannot_bypass_websocket_framing():
    assert 'if (player->websocket)' in MCCP
    assert 'WebSocket output must always use framed send APIs.' in MCCP
    assert 'if (!escaped)' in MCCP
    assert 'if (!json_msg)' in MCCP
    assert 'result = websocket_send_text(player, json_msg)' in MCCP
    assert 'return result;' in MCCP
    assert '#define WS_OUTPUT_QUEUE_FULL (-2)' in HEADER
    assert 'output_result == WS_OUTPUT_QUEUE_FULL' in COMM
    assert '!(t->websocket && output_result == WS_OUTPUT_QUEUE_FULL)' in COMM
    assert 'if (d->websocket)' in GMCP
    assert 'websocket_send_json(d, "gmcp", package, json)' in GMCP


def test_proxy_metadata_requires_trusted_peer_and_validated_values():
    assert "DURIS_TRUSTED_PROXY_IP" in COMM
    assert "proxy_peer_is_trusted(desc)" in COMM
    assert "websocket_peer_is_trusted_proxy(d)" in SOURCE
    assert "DURIS_TRUSTED_PROXY_IP" in SOURCE
    assert "untrusted X-Forwarded-For changed descriptor host" in HARNESS
    assert "inet_pton(AF_INET" in COMM
    assert "inet_pton(AF_INET6" in COMM
    assert "src_port < 1 || src_port > 65535" in COMM
    assert 'strcmp(proto, "TCP4")' in COMM
    assert 'strcmp(proto, "TCP6")' in COMM


def test_authentication_fails_closed_without_configured_secret():
    assert 'DURISWEB_SECRET_DEFAULT' not in WS_HANDLERS
    assert 'DURISWEB_SECRET_DEFAULT' not in GMCP
    assert 'if (!secret || !*secret || !sig || strlen(sig) != 64)' in AUTH
    assert '#include "ws_auth.h"' in WS_HANDLERS
    assert '#include "ws_auth.h"' in GMCP
    assert 'strlen(sig) != 64' in AUTH
    assert 'CRYPTO_memcmp' in AUTH
    assert 'offset = -1' in AUTH and 'offset <= 1' in AUTH


def test_authorization_state_fails_closed_for_service_and_player_commands():
    assert 'd->durisweb_verified = 0;' in WS_HANDLERS
    assert 'd->durisweb_backend  = 0;' in WS_HANDLERS
    assert 'd->account || d->character || d->connected == CON_PLAYING' in WS_HANDLERS
    assert 'void ws_cmd_poll_vote' in WS_HANDLERS
    assert 'if (!d || !d->account || !d->account->acct_name)' in WS_HANDLERS
    assert 'ws_verify_durisweb_signature(sig->valuestring)' in WS_HANDLERS
    assert 'd->connected != CON_PLAYING || !d->character' in WS_HANDLERS
    assert 'Service connection cannot log in as a player' in WS_HANDLERS
    assert 'Already authenticated' in WS_HANDLERS
    assert 'if (d->durisweb_verified || d->durisweb_backend)' in WS_HANDLERS
    assert 'if (!d || d->account || d->durisweb_verified || d->durisweb_backend)' in WS_HANDLERS
    assert 'websocket_close(point, close_code, reason)' in COMM
    assert 'point->ws_state == WS_STATE_OPEN' in COMM
    assert 'point->ws_error_code ? point->ws_error_code : WS_CLOSE_PROTOCOL_ERROR' in COMM
    assert 'websocket_input_error(d, WS_CLOSE_MESSAGE_TOO_BIG)' in SOURCE
    assert 'websocket_input_error(d, WS_CLOSE_INVALID_DATA)' in SOURCE
    assert 'websocket_input_error(d, WS_CLOSE_INTERNAL_ERROR)' in SOURCE
    assert 'WS_CLOSE_INTERNAL_ERROR ? "Internal error"' in COMM
    assert SOURCE.count('deflateInit2') == 1
    assert SOURCE.index('deflateInit2') < SOURCE.index('websocket_send_all(d->descriptor')
    assert 'ws_compressed_message' in SOURCE
    assert 'if (rsv1 || d->ws_compressed_message)' in SOURCE
    assert 'out_size = WS_MAX_MESSAGE_SIZE - d->ws_message_len' in SOURCE
    assert 'compressed fragments failed through aggregate message processing' in HARNESS
    assert 'd->ws_compressed_message = 0;' in SOURCE
    assert 'd->ws_error_code = 0;' in SOURCE
    assert 'deflateReset(strm) != Z_OK' in SOURCE
    assert 'd->ws_deflate_stream = NULL' in SOURCE
    assert 'int deflate_ok = 0' in SOURCE
    assert 'len > (size_t)-1 - 64' in SOURCE
    assert 'payload_len > (size_t)-1 - 10' in SOURCE
    assert 'frame_len > WS_MAX_OUTPUT_BYTES' in SOURCE
    assert 'frame_len > (opcode >= WS_OPCODE_CLOSE ? WS_CONTROL_OUTPUT_RESERVE : WS_MAX_OUTPUT_BYTES - WS_CONTROL_OUTPUT_RESERVE)' in SOURCE
    assert '(*offset > *queued_len || (*offset > 0 && !*buffer))' in SOURCE
    assert 'uint64_t wire_len' in SOURCE
    assert '(len > 0 && !data)' in SOURCE
    assert 'd->ws_bytes_in += len' in SOURCE
    assert SOURCE.count('inflateReset(strm);') >= 3
    assert 'server_no_context_takeover' in SOURCE
    assert 'static void websocket_abort_handshake' in SOURCE
    assert 'websocket_abort_handshake(d);' in SOURCE
    assert 'if (!d || d->descriptor < 0 || !key)' in SOURCE
    assert 'if (len < 0 || (size_t)len >= sizeof(response))' in SOURCE
    assert 'failed handshake retained negotiated state' in HARNESS
    assert 'websocket_free left stale output or heartbeat state' in HARNESS
    assert 'point->ws_state == WS_STATE_CLOSING' in COMM
    assert 'if (websocket_send_ping(point) == 0)' in COMM
    assert 'point->ws_ping_outstanding = 1' in COMM
    assert '!point->ws_ping_queued && !point->ws_ping_outstanding' in COMM
    assert 'point->ws_ping_queued && point->ws_control_output_len == 0' in COMM
    assert 'point->ws_last_ping = time(0)' in COMM
    assert 'if (d->ws_ping_outstanding)' in SOURCE
    assert 'd->ws_ping_outstanding = 0' in SOURCE
    assert 'd->ws_last_ping = 0' in SOURCE
    assert 'failed_handshake.ws_deflate_stream' in HARNESS
    assert 'close frame was blocked by saturated application queue' in HARNESS
    assert 'saturated_close_desc.ws_output_len' in HARNESS
    assert 'websocket_valid_close_code' in SOURCE
    assert 'WS_LEN_7BIT_MAX - 2' in SOURCE
    assert 'websocket_valid_utf8((const unsigned char *)reason' in SOURCE
    assert 'invalid generated close code changed state' in HARNESS
    assert 'generated close exceeded control payload limit' in HARNESS
    assert 'opcode >= WS_OPCODE_CLOSE && len > WS_LEN_7BIT_MAX' in SOURCE
    assert 'invalid generated pong changed state' in HARNESS
    assert 'registered close code 1012 was rejected' in HARNESS
    assert 'reserved or unassigned close code was accepted' in HARNESS
    assert 'client_no_context_takeover' in SOURCE
    assert 'outbound_frame_matches' in HARNESS
    assert 'compressed outbound text send' in HARNESS
    assert 'incompressible outbound data was incorrectly marked compressed' in HARNESS
    assert 'websocket_free left stale compression state' in HARNESS
    assert 'websocket_input_error(d, WS_CLOSE_PROTOCOL_ERROR)' in SOURCE
    assert 'Clear service authorization before descriptor reuse.' in COMM
    assert '#define WS_MAX_FLUSH_BYTES 65536' in HEADER
    assert 'size_t flushed = 0' in SOURCE
    assert 'size_t budget = WS_MAX_FLUSH_BYTES - flushed' in SOURCE
    assert 'websocket_flush_queue(d, d->ws_control_output_buffer' in SOURCE
    assert 'd->ws_deflate_requested = 0;' in SOURCE
    assert 'd->ws_compressed_message = 0;' in SOURCE
    assert 'if (!d || !buf || len > WS_MAX_HANDSHAKE_SIZE)' in SOURCE
    assert 'memcmp(buf + len - 4, "\\r\\n\\r\\n", 4)' in SOURCE
    assert "buf[i] == '\\0'" in SOURCE
    assert 'lone-LF handshake was accepted' in HARNESS
    assert 'embedded-NUL handshake was accepted' in HARNESS
    assert 'canonical_key' in SOURCE
    assert 'EVP_EncodeBlock(canonical_key, decoded_key, WS_KEY_DECODED_SIZE)' in SOURCE
    assert 'noncanonical websocket key was accepted' in HARNESS
    assert 'websocket_valid_header_line' in SOURCE
    assert 'c < 0x20' in SOURCE
    assert 'noncanonical websocket version was accepted' in HARNESS
    assert 'control character in header value was accepted' in HARNESS
    assert 'header without colon was accepted' in HARNESS
    assert 'leading-whitespace header was accepted' in HARNESS
    assert 'websocket_has_header_token' in SOURCE
    assert 'valid upgrade token list was rejected' in HARNESS
    assert 'connection lookalike token was accepted' in HARNESS
    assert 'duplicate_invalid' in SOURCE
    assert 'substring extension token was incorrectly negotiated' in HARNESS
    assert 'duplicate websocket key was accepted' in HARNESS
    assert 'invalid handshake retained compression negotiation state' in HARNESS
    assert 'oversized direct handshake was not rejected' in HARNESS
    assert 'd->ws_state = WS_STATE_CLOSING;' in SOURCE
    assert 'server close did not remain in deferred closing state' in HARNESS
    assert 'queued close frame did not survive state transition' in HARNESS
    assert '#define WS_MAX_FRAMES_PER_READ 64' in HEADER
    assert 'frames_processed < WS_MAX_FRAMES_PER_READ' in SOURCE
    assert '#define WS_AUTH_MAX_FAILURES 5' in HEADER
    assert '#define WS_AUTH_FAILURE_WINDOW 60' in HEADER
    assert 'ws_durisweb_auth_limited' in WS_HANDLERS
    assert 'ws_durisweb_auth_failure(d);' in WS_HANDLERS
    assert 'd->durisweb_auth_failures = 0;' in WS_HANDLERS
    assert 'd->durisweb_auth_failures = 0;' in COMM
    assert 'gmcp_durisweb_auth_limited' in GMCP
    assert 'gmcp_durisweb_auth_failure(d);' in GMCP
    assert 'd->durisweb_backend  = 1;' in GMCP
    assert 'd->account || d->character || d->connected == CON_PLAYING' in GMCP
    assert '#define GMCP_MAX_INPUT_SIZE (1024 * 1024)' in GMCP_H
    assert GMCP.count('cJSON_ParseWithLength') == 2
    assert 'cJSON_Parse(' not in GMCP
    assert "strncmp(data, \"Core.Hello\", 10) == 0 && data[10] == ' '" in GMCP
    assert "strncmp(data, \"Client.Info\", 11) == 0 && data[11] == ' '" in GMCP
    assert '#define GMCP_MAX_PACKAGE_SIZE 128' in GMCP_H
    assert 'package_len <= GMCP_MAX_PACKAGE_SIZE' in SOURCE
    assert 'data_len <= GMCP_MAX_INPUT_SIZE - package_len - 2' in SOURCE


def test_handshake_validates_request_line_version_and_key_shape():
    assert "request_line_ok" in parse_handshake
    assert "first_line" in parse_handshake
    assert "HTTP/1.1" in parse_handshake
    assert "EVP_DecodeBlock" in parse_handshake
    assert "WS_KEY_DECODED_SIZE" in parse_handshake
    assert "decoded_len == WS_KEY_DECODED_SIZE + 2" in parse_handshake
    assert "!request_line_ok" in parse_handshake


if __name__ == "__main__":
    tests = [
        test_client_frames_require_masking_before_payload_processing,
        test_reserved_bits_and_opcodes_are_rejected,
        test_control_frames_are_not_fragmented_or_oversized,
        test_fragmented_input_has_an_aggregate_cap,
        test_decompression_does_not_deliver_truncated_output,
        test_handshake_preserves_coalesced_websocket_bytes,
        test_handshake_has_a_real_deadline_outside_connected_states,
        test_websocket_output_backpressure_is_queued_and_retried,
        test_proxy_metadata_requires_trusted_peer_and_validated_values,
        test_authentication_fails_closed_without_configured_secret,
        test_authorization_state_fails_closed_for_service_and_player_commands,
        test_handshake_validates_request_line_version_and_key_shape,
    ]
    for test in tests:
        test()
    print("WebSocket protocol hardening contracts passed")
