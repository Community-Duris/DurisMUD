#include "structs.h"
#include "websocket.h"
#include <cJSON.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <zlib.h>

extern "C"
{
	descriptor_data *descriptor_list = nullptr;
}

void statuslog(int, const char *, ...) {}
void close_socket(descriptor_data *) {}
int is_desc_valid(descriptor_data *) { return 1; }
void write_to_q(const char *, txt_q *, int) {}
void ws_send_system(descriptor_data *, const char *, const char *) {}
void gmcp_handle_input(descriptor_data *, const char *, size_t) {}
void ws_handle_command(descriptor_data *, const char *, cJSON *) {}
char *json_build_gmcp_message(const char *, const char *) { return nullptr; }

static int fail(const char *message)
{
	fprintf(stderr, "FAIL: %s\n", message);
	return 1;
}

static int make_compressed_text_frame(const char *text, unsigned char *frame, size_t frame_capacity, size_t *frame_len)
{
	unsigned char compressed[256];
	z_stream stream{};
	const size_t text_len = strlen(text);
	if (text_len == 0 || text_len > 200 || frame_capacity < 16 ||
	    deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		return 0;
	stream.next_in = (Bytef *)text;
	stream.avail_in = (uInt)text_len;
	stream.next_out = compressed;
	stream.avail_out = sizeof(compressed);
	if (deflate(&stream, Z_SYNC_FLUSH) != Z_OK || stream.total_out < 4)
	{
		deflateEnd(&stream);
		return 0;
	}
	size_t compressed_len = stream.total_out - 4;
	deflateEnd(&stream);
	if (compressed_len > 125 || 6 + compressed_len > frame_capacity)
		return 0;
	frame[0] = 0xC1;
	frame[1] = (unsigned char)(0x80 | compressed_len);
	const unsigned char mask[4] = {0x11, 0x22, 0x33, 0x44};
	memcpy(frame + 2, mask, sizeof(mask));
	for (size_t i = 0; i < compressed_len; i++)
		frame[6 + i] = compressed[i] ^ mask[i % 4];
	*frame_len = 6 + compressed_len;
	return 1;
}
static int make_compressed_fragment_frames(const char *text, unsigned char *first, size_t first_capacity, size_t *first_len, unsigned char *second, size_t second_capacity, size_t *second_len)
{
	unsigned char whole[256];
	size_t whole_len = 0;
	if (!make_compressed_text_frame(text, whole, sizeof(whole), &whole_len))
		return 0;
	size_t compressed_len = whole[1] & 0x7F;
	unsigned char compressed[256];
	for (size_t i = 0; i < compressed_len; i++)
		compressed[i] = whole[6 + i] ^ whole[2 + (i % 4)];
	size_t first_payload_len = compressed_len / 2;
	size_t second_payload_len = compressed_len - first_payload_len;
	if (first_payload_len == 0 || first_payload_len > 125 || second_payload_len > 125 ||
	    first_capacity < first_payload_len + 6 || second_capacity < second_payload_len + 6)
		return 0;
	const unsigned char first_mask[4] = {0x51, 0x62, 0x73, 0x84};
	const unsigned char second_mask[4] = {0x15, 0x26, 0x37, 0x48};
	first[0] = 0x41;
	first[1] = 0x80 | first_payload_len;
	memcpy(first + 2, first_mask, 4);
	for (size_t i = 0; i < first_payload_len; i++)
		first[6 + i] = compressed[i] ^ first_mask[i % 4];
	second[0] = 0x80;
	second[1] = 0x80 | second_payload_len;
	memcpy(second + 2, second_mask, 4);
	for (size_t i = 0; i < second_payload_len; i++)
		second[6 + i] = compressed[first_payload_len + i] ^ second_mask[i % 4];
	*first_len = first_payload_len + 6;
	*second_len = second_payload_len + 6;
	return 1;
}
static int outbound_frame_matches(const unsigned char *frame, size_t frame_len, const char *expected)
{
	if (frame_len < 2 || !(frame[0] & 0x40) || (frame[1] & 0x80) || (frame[1] & 0x7F) >= 126)
		return 0;
	size_t payload_len = frame[1] & 0x7F;
	if (frame_len != payload_len + 2)
		return 0;
	unsigned char input[256];
	if (payload_len + 4 > sizeof(input))
		return 0;
	memcpy(input, frame + 2, payload_len);
	input[payload_len] = 0x00;
	input[payload_len + 1] = 0x00;
	input[payload_len + 2] = 0xff;
	input[payload_len + 3] = 0xff;
	char output[256] = {0};
	z_stream stream{};
	if (inflateInit2(&stream, -15) != Z_OK)
		return 0;
	stream.next_in = input;
	stream.avail_in = payload_len + 4;
	stream.next_out = (Bytef *)output;
	stream.avail_out = sizeof(output) - 1;
	int result = inflate(&stream, Z_SYNC_FLUSH);
	inflateEnd(&stream);
	return (result == Z_OK || result == Z_BUF_ERROR) && strcmp(output, expected) == 0;
}
static int expect_frame_error(descriptor_data *d, const unsigned char *frame, size_t len, const char *label)
{
	char  *payload = nullptr;
	size_t payload_len = 0;
	int    opcode = -1;
	int    fin = 0;
	if (websocket_parse_frame(d, (const char *)frame, len, &payload, &payload_len, &opcode, &fin) >= 0)
	{
		free(payload);
		fprintf(stderr, "FAIL: accepted malformed frame: %s\n", label);
		return 0;
	}
	return 1;
}

int main()
{
	const char *handshake =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Upgrade: websocket\r\n"
		"Connection: keep-alive, Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
		"Sec-WebSocket-Version: 13\r\n\r\n";
	const unsigned char ping[] = {0x89, 0x82, 0x01, 0x02, 0x03, 0x04, 0x69, 0x6b};
	char input[1024];
	int pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0)
		return fail("socketpair");

	descriptor_data d{};
	d.descriptor = pair[0];
	d.ws_state = WS_STATE_CONNECTING;
	d.ws_handshake_done = 0;
	int handshake_len = (int)strlen(handshake);
	memcpy(input, handshake, handshake_len);
	memcpy(input + handshake_len, ping, sizeof(ping));

	if (write(pair[1], input, handshake_len + sizeof(ping)) != handshake_len + (ssize_t)sizeof(ping))
		return fail("write coalesced handshake and ping");

	if (websocket_process_input(&d) != 0)
		return fail("coalesced handshake/ping processing");
	if (!d.ws_handshake_done || d.ws_state != WS_STATE_OPEN)
		return fail("handshake did not open descriptor");

	char output[2048];
	ssize_t output_len = read(pair[1], output, sizeof(output));
	if (output_len <= 0)
		return fail("read handshake/pong response");
	if (!memmem(output, output_len, "101 Switching Protocols", 23))
		return fail("missing HTTP 101 response");
	const unsigned char pong[] = {0x8a, 0x02, 'h', 'i'};
	if (!memmem(output, output_len, pong, sizeof(pong)))
		return fail("missing pong for coalesced masked ping");

	unsetenv("DURIS_TRUSTED_PROXY_IP");
	int untrusted_xff_pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, untrusted_xff_pair) != 0)
		return fail("untrusted X-Forwarded-For socketpair");
	descriptor_data untrusted_xff_desc{};
	untrusted_xff_desc.descriptor = untrusted_xff_pair[0];
	untrusted_xff_desc.ws_state = WS_STATE_CONNECTING;
	strcpy(untrusted_xff_desc.host, "10.0.0.2");
	const char *untrusted_xff_handshake =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"X-Forwarded-For: 203.0.113.7\r\n\r\n";
	if (write(untrusted_xff_pair[1], untrusted_xff_handshake, strlen(untrusted_xff_handshake)) !=
	    (ssize_t)strlen(untrusted_xff_handshake) || websocket_process_input(&untrusted_xff_desc) != 0)
		return fail("untrusted X-Forwarded-For handshake");
	char untrusted_xff_response[512];
	if (read(untrusted_xff_pair[1], untrusted_xff_response, sizeof(untrusted_xff_response)) <= 0 ||
	    strcmp(untrusted_xff_desc.host, "10.0.0.2") != 0)
		return fail("untrusted X-Forwarded-For changed descriptor host");
	websocket_free(&untrusted_xff_desc);
	close(untrusted_xff_pair[0]);
	close(untrusted_xff_pair[1]);

	z_stream inbound_inflater{};
	if (inflateInit2(&inbound_inflater, -15) != Z_OK)
		return fail("inflate initialization");
	d.ws_compress = 1;
	d.ws_inflate_stream = &inbound_inflater;
	unsigned char compressed_frame[256];
	size_t compressed_frame_len = 0;
	if (!make_compressed_text_frame("compressed-one", compressed_frame, sizeof(compressed_frame), &compressed_frame_len))
		return fail("build first compressed frame");
	char *compressed_payload = nullptr;
	size_t compressed_payload_len = 0;
	int compressed_opcode = -1;
	int compressed_fin = 0;
	if (websocket_parse_frame(&d, (const char *)compressed_frame, compressed_frame_len, &compressed_payload, &compressed_payload_len, &compressed_opcode, &compressed_fin) < 0 ||
	    compressed_opcode != WS_OPCODE_TEXT || !compressed_fin || compressed_payload_len != strlen("compressed-one") ||
	    memcmp(compressed_payload, "compressed-one", compressed_payload_len) != 0)
	{
		free(compressed_payload);
		inflateEnd(&inbound_inflater);
		return fail("first compressed frame was not decompressed");
	}
	free(compressed_payload);
	compressed_payload = nullptr;
	if (!make_compressed_text_frame("compressed-two", compressed_frame, sizeof(compressed_frame), &compressed_frame_len))
		return fail("build second compressed frame");
	if (websocket_parse_frame(&d, (const char *)compressed_frame, compressed_frame_len, &compressed_payload, &compressed_payload_len, &compressed_opcode, &compressed_fin) < 0 ||
	    compressed_payload_len != strlen("compressed-two") || memcmp(compressed_payload, "compressed-two", compressed_payload_len) != 0)
	{
		free(compressed_payload);
		inflateEnd(&inbound_inflater);
		return fail("inflater state was not reset between compressed frames");
	}
	free(compressed_payload);
	unsigned char compressed_first[256], compressed_second[256];
	size_t compressed_first_len = 0, compressed_second_len = 0;
	if (!make_compressed_fragment_frames("fragmented-compressed", compressed_first, sizeof(compressed_first), &compressed_first_len, compressed_second, sizeof(compressed_second), &compressed_second_len))
		return fail("build fragmented compressed frames");
	char *fragment_payload = nullptr;
	size_t fragment_payload_len = 0;
	if (websocket_parse_frame(&d, (const char *)compressed_first, compressed_first_len, &fragment_payload, &fragment_payload_len, &compressed_opcode, &compressed_fin) < 0 || compressed_fin)
	{
		free(fragment_payload);
		inflateEnd(&inbound_inflater);
		return fail("compressed first fragment was not retained");
	}
	char fragmented_text[64];
	size_t fragmented_text_len = fragment_payload_len;
	memcpy(fragmented_text, fragment_payload, fragment_payload_len);
	free(fragment_payload);
	fragment_payload = nullptr;
	if (websocket_parse_frame(&d, (const char *)compressed_second, compressed_second_len, &fragment_payload, &fragment_payload_len, &compressed_opcode, &compressed_fin) < 0 || !compressed_fin ||
	    fragmented_text_len + fragment_payload_len >= sizeof(fragmented_text))
	{
		free(fragment_payload);
		inflateEnd(&inbound_inflater);
		return fail("compressed continuation was not decompressed");
	}
	memcpy(fragmented_text + fragmented_text_len, fragment_payload, fragment_payload_len);
	fragmented_text_len += fragment_payload_len;
	fragmented_text[fragmented_text_len] = '\0';
	free(fragment_payload);
	if (strcmp(fragmented_text, "fragmented-compressed") != 0 || d.ws_compressed_message)
	{
		inflateEnd(&inbound_inflater);
		return fail("compressed fragmented message was corrupted or not reset");
	}
	if (write(pair[1], compressed_first, compressed_first_len) != (ssize_t)compressed_first_len ||
	    write(pair[1], compressed_second, compressed_second_len) != (ssize_t)compressed_second_len ||
	    websocket_process_input(&d) != 0 || d.ws_message_buffer || d.ws_message_len != 0)
	{
		inflateEnd(&inbound_inflater);
		return fail("compressed fragments failed through aggregate message processing");
	}
	compressed_frame[compressed_frame_len - 1] ^= 0x7F;
	if (!expect_frame_error(&d, compressed_frame, compressed_frame_len, "invalid compressed payload"))
	{
		inflateEnd(&inbound_inflater);
		return 1;
	}
	inflateEnd(&inbound_inflater);
	d.ws_inflate_stream = nullptr;
	d.ws_compress = 0;

	int outbound_pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, outbound_pair) != 0)
		return fail("outbound compression socketpair");
	descriptor_data outbound_desc{};
	outbound_desc.descriptor = outbound_pair[0];
	outbound_desc.websocket = 1;
	outbound_desc.ws_state = WS_STATE_OPEN;
	z_stream outbound_deflater{};
	if (deflateInit2(&outbound_deflater, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		return fail("outbound deflate initialization");
	outbound_desc.ws_compress = 1;
	outbound_desc.ws_deflate_stream = &outbound_deflater;
	char repetitive[129];
	memset(repetitive, 'A', sizeof(repetitive) - 1);
	repetitive[sizeof(repetitive) - 1] = '\0';
	if (websocket_send_text(&outbound_desc, repetitive) < 0)
		return fail("compressed outbound text send");
	unsigned char outbound_frame[256];
	ssize_t outbound_frame_len = read(outbound_pair[1], outbound_frame, sizeof(outbound_frame));
	if (outbound_frame_len < 6 || !(outbound_frame[0] & 0x40) || (outbound_frame[1] & 0x80) ||
	    !outbound_frame_matches(outbound_frame, outbound_frame_len, repetitive))
		return fail("outbound compressed frame flags");
	if (websocket_send_text(&outbound_desc, repetitive) < 0)
		return fail("second compressed outbound text send");
	outbound_frame_len = read(outbound_pair[1], outbound_frame, sizeof(outbound_frame));
	if (outbound_frame_len < 6 || !(outbound_frame[0] & 0x40) || (outbound_frame[1] & 0x80) ||
	    !outbound_frame_matches(outbound_frame, outbound_frame_len, repetitive))
		return fail("second outbound compressed frame flags");
	unsigned char random_payload[200];
	unsigned int random_state = 0x12345678;
	for (size_t i = 0; i < sizeof(random_payload); i++)
	{
		random_state = random_state * 1664525u + 1013904223u;
		random_payload[i] = (unsigned char)(random_state >> 24);
	}
	if (websocket_send_binary(&outbound_desc, random_payload, sizeof(random_payload)) < 0)
		return fail("incompressible outbound binary send");
	outbound_frame_len = read(outbound_pair[1], outbound_frame, sizeof(outbound_frame));
	if (outbound_frame_len < 2 || (outbound_frame[0] & 0x40))
		return fail("incompressible outbound data was incorrectly marked compressed");
	deflateEnd(&outbound_deflater);
	outbound_desc.ws_deflate_stream = nullptr;
	websocket_free(&outbound_desc);
	close(outbound_pair[0]);
	close(outbound_pair[1]);

	int generated_close_pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, generated_close_pair) != 0)
		return fail("generated close socketpair");
	descriptor_data generated_close_desc{};
	generated_close_desc.descriptor = generated_close_pair[0];
	generated_close_desc.websocket = 1;
	generated_close_desc.ws_state = WS_STATE_OPEN;
	char long_close_reason[300];
	memset(long_close_reason, 'a', sizeof(long_close_reason) - 1);
	long_close_reason[sizeof(long_close_reason) - 1] = '\0';
	if (websocket_send_close(&generated_close_desc, 1000, long_close_reason) != 0)
		return fail("valid long close reason was rejected");
	unsigned char generated_close_frame[256];
	ssize_t generated_close_len = read(generated_close_pair[1], generated_close_frame, sizeof(generated_close_frame));
	if (generated_close_desc.ws_state != WS_STATE_CLOSING || generated_close_len < 2 || generated_close_frame[0] != 0x88 || generated_close_frame[1] > 125)
		return fail("generated close exceeded control payload limit");
	generated_close_desc.ws_state = WS_STATE_OPEN;
	unsigned char oversized_pong[WS_LEN_7BIT_MAX + 1] = {0};
	if (websocket_send_pong(&generated_close_desc, (const char *)oversized_pong, sizeof(oversized_pong)) >= 0 ||
	    websocket_send_pong(&generated_close_desc, nullptr, 1) >= 0 || generated_close_desc.ws_state != WS_STATE_OPEN)
		return fail("invalid generated pong changed state");
	if (websocket_send_close(&generated_close_desc, 1005, "bad") >= 0 || generated_close_desc.ws_state != WS_STATE_OPEN)
		return fail("invalid generated close code changed state");
	if (websocket_send_close(&generated_close_desc, 1012, "restart") != 0)
		return fail("registered close code 1012 was rejected");
	generated_close_len = read(generated_close_pair[1], generated_close_frame, sizeof(generated_close_frame));
	if (generated_close_len < 4 || generated_close_frame[0] != 0x88 || generated_close_frame[2] != 0x03 || generated_close_frame[3] != 0xF4)
		return fail("registered close code 1012 was not emitted");
	generated_close_desc.ws_state = WS_STATE_OPEN;
	if (websocket_send_close(&generated_close_desc, 1015, "reserved") >= 0 ||
	    websocket_send_close(&generated_close_desc, 1016, "unassigned") >= 0 ||
	    generated_close_desc.ws_state != WS_STATE_OPEN)
		return fail("reserved or unassigned close code was accepted");
	websocket_free(&generated_close_desc);
	close(generated_close_pair[0]);
	close(generated_close_pair[1]);

	int server_close_pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, server_close_pair) != 0)
		return fail("server close socketpair");
	descriptor_data server_close_desc{};
	server_close_desc.descriptor = server_close_pair[0];
	server_close_desc.websocket = 1;
	server_close_desc.ws_state = WS_STATE_OPEN;
	websocket_close(&server_close_desc, WS_CLOSE_GOING_AWAY, "shutdown");
	unsigned char server_close_frame[32];
	ssize_t server_close_len = read(server_close_pair[1], server_close_frame, sizeof(server_close_frame));
	if (server_close_desc.ws_state != WS_STATE_CLOSING || server_close_len < 2 || server_close_frame[0] != 0x88)
		return fail("server close did not remain in deferred closing state");
	websocket_free(&server_close_desc);
	close(server_close_pair[0]);
	close(server_close_pair[1]);

	int saturated_close_pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, saturated_close_pair) != 0)
		return fail("saturated close socketpair");
	descriptor_data saturated_close_desc{};
	saturated_close_desc.descriptor = saturated_close_pair[0];
	saturated_close_desc.websocket = 1;
	saturated_close_desc.ws_state = WS_STATE_OPEN;
	int saturated_flags = fcntl(saturated_close_pair[0], F_GETFL, 0);
	if (saturated_flags < 0 || fcntl(saturated_close_pair[0], F_SETFL, saturated_flags | O_NONBLOCK) < 0)
		return fail("set saturated close nonblocking");
	saturated_close_desc.ws_output_buffer = (unsigned char *)malloc(WS_MAX_OUTPUT_BYTES - WS_CONTROL_OUTPUT_RESERVE);
	if (!saturated_close_desc.ws_output_buffer)
		return fail("allocate saturated application queue");
	saturated_close_desc.ws_output_len = WS_MAX_OUTPUT_BYTES - WS_CONTROL_OUTPUT_RESERVE;
	websocket_close(&saturated_close_desc, WS_CLOSE_GOING_AWAY, "saturated");
	unsigned char saturated_close_frame[32];
	ssize_t saturated_close_len = read(saturated_close_pair[1], saturated_close_frame, sizeof(saturated_close_frame));
	if (saturated_close_desc.ws_state != WS_STATE_CLOSING || saturated_close_len < 2 || saturated_close_frame[0] != 0x88)
	{
		websocket_free(&saturated_close_desc);
		close(saturated_close_pair[0]);
		close(saturated_close_pair[1]);
		return fail("close frame was blocked by saturated application queue");
	}
	websocket_free(&saturated_close_desc);
	close(saturated_close_pair[0]);
	close(saturated_close_pair[1]);

	int close_pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, close_pair) != 0)
		return fail("close handshake socketpair");
	descriptor_data close_desc{};
	close_desc.descriptor = close_pair[0];
	close_desc.websocket = 1;
	close_desc.ws_state = WS_STATE_OPEN;
	close_desc.ws_handshake_done = 1;
	const unsigned char inbound_close[] = {0x88, 0x85, 0x01, 0x02, 0x03, 0x04, 0x02, 0xEA, 0x61, 0x7D, 0x64};
	if (write(close_pair[1], inbound_close, sizeof(inbound_close)) != (ssize_t)sizeof(inbound_close))
		return fail("write inbound close frame");
	int close_result = websocket_process_input(&close_desc);
	if (close_result != 0 || close_desc.ws_state != WS_STATE_CLOSING)
		return fail("inbound close did not enter deferred closing state");
	unsigned char close_reply[8];
	ssize_t close_reply_len = read(close_pair[1], close_reply, sizeof(close_reply));
	unsigned char expected_close_reply[] = {0x88, 0x05, 0x03, 0xE8, 'b', 'y', 'e'};
	if (close_reply_len != (ssize_t)sizeof(expected_close_reply) || memcmp(close_reply, expected_close_reply, sizeof(expected_close_reply)) != 0)
		return fail("inbound close reply was not delivered");
	int close_peer_flags = fcntl(close_pair[1], F_GETFL, 0);
	if (close_peer_flags < 0 || fcntl(close_pair[1], F_SETFL, close_peer_flags | O_NONBLOCK) < 0)
		return fail("set close peer nonblocking");
	if (write(close_pair[1], inbound_close, sizeof(inbound_close)) != (ssize_t)sizeof(inbound_close))
		return fail("write simultaneous close frame");
	if (websocket_process_input(&close_desc) != 0 || close_desc.ws_state != WS_STATE_CLOSING)
		return fail("simultaneous close changed closing state");
	unsigned char duplicate_reply[8];
	if (read(close_pair[1], duplicate_reply, sizeof(duplicate_reply)) >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
		return fail("simultaneous close generated duplicate reply");
	if (shutdown(close_pair[1], SHUT_WR) != 0 || websocket_process_input(&close_desc) != -1)
		return fail("peer half-close was not surfaced as EOF");
	if (close_desc.ws_output_len != 0 || close_desc.ws_control_output_len != 0)
		return fail("EOF left stale WebSocket output queued");
	websocket_free(&close_desc);
	close(close_pair[0]);
	close(close_pair[1]);

	const unsigned char first_fragment[] = {0x02, 0x81, 0x10, 0x20, 0x30, 0x40, 0x71};
	const unsigned char continuation_fragment[] = {0x80, 0x81, 0x11, 0x22, 0x33, 0x44, 0x73};
	if (write(pair[1], first_fragment, sizeof(first_fragment)) != (ssize_t)sizeof(first_fragment))
		return fail("write first fragmented data frame");
	if (websocket_process_input(&d) != 0 || d.ws_message_len != 1 || !d.ws_message_buffer)
		return fail("did not retain first fragmented data frame");

	if (write(pair[1], ping, sizeof(ping)) != (ssize_t)sizeof(ping) ||
	    write(pair[1], continuation_fragment, sizeof(continuation_fragment)) != (ssize_t)sizeof(continuation_fragment))
		return fail("write interleaved ping and continuation");
	if (websocket_process_input(&d) != 0 || d.ws_message_buffer || d.ws_message_len != 0)
		return fail("did not complete interleaved fragmented message");
	output_len = read(pair[1], output, sizeof(output));
	if (output_len <= 0 || !memmem(output, output_len, pong, sizeof(pong)))
		return fail("missing pong during fragmented message");

	const unsigned char split_frame[] = {0x82, 0x81, 0x51, 0x52, 0x53, 0x54, 0x31};
	if (write(pair[1], split_frame, 4) != 4)
		return fail("write partial frame");
	if (websocket_process_input(&d) != 0 || d.ws_fragment_len != 4)
		return fail("did not retain incomplete frame");
	if (write(pair[1], split_frame + 4, sizeof(split_frame) - 4) != (ssize_t)(sizeof(split_frame) - 4))
		return fail("complete partial frame");
	if (websocket_process_input(&d) != 0 || d.ws_fragment_len != 0)
		return fail("did not consume completed split frame");

	const unsigned char unmasked_ping[] = {0x89, 0x00};
	const unsigned char reserved_bits[] = {0xA1, 0x80, 0x00, 0x00, 0x00, 0x00};
	const unsigned char fragmented_ping[] = {0x09, 0x80, 0x00, 0x00, 0x00, 0x00};
	const unsigned char oversized_ping[] = {0x89, 0xFE, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00};
	if (!expect_frame_error(&d, unmasked_ping, sizeof(unmasked_ping), "unmasked client frame") ||
	    !expect_frame_error(&d, reserved_bits, sizeof(reserved_bits), "reserved bits") ||
	    !expect_frame_error(&d, fragmented_ping, sizeof(fragmented_ping), "fragmented control frame") ||
	    !expect_frame_error(&d, oversized_ping, sizeof(oversized_ping), "oversized control frame"))
		return 1;

	const unsigned char invalid_close_length[] = {0x88, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00};
	const unsigned char invalid_close_code[] = {0x88, 0x82, 0x00, 0x00, 0x00, 0x00, 0x03, 0xED};
	const unsigned char invalid_close_utf8[] = {0x88, 0x83, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE8, 0xFF};
	const unsigned char compressed_close[] = {0xC8, 0x80, 0x00, 0x00, 0x00, 0x00};
	if (!expect_frame_error(&d, invalid_close_length, sizeof(invalid_close_length), "one-byte close payload") ||
	    !expect_frame_error(&d, invalid_close_code, sizeof(invalid_close_code), "reserved close code") ||
	    !expect_frame_error(&d, invalid_close_utf8, sizeof(invalid_close_utf8), "invalid close reason UTF-8") ||
	    !expect_frame_error(&d, compressed_close, sizeof(compressed_close), "compressed control frame"))
		return 1;

	const unsigned char invalid_continuation[] = {0x80, 0x80, 0x00, 0x00, 0x00, 0x00};
	if (write(pair[1], invalid_continuation, sizeof(invalid_continuation)) != (ssize_t)sizeof(invalid_continuation))
		return fail("write invalid continuation");
	if (websocket_process_input(&d) == 0)
		return fail("accepted continuation without initial fragmented message");

	const unsigned char aggregate_continuation[] = {0x80, 0x81, 0x61, 0x62, 0x63, 0x64, 0x65};
	d.ws_message_buffer = (char *)calloc(1, 1);
	d.ws_message_len = WS_MAX_MESSAGE_SIZE;
	if (!d.ws_message_buffer)
		return fail("allocate aggregate-limit sentinel");
	if (write(pair[1], aggregate_continuation, sizeof(aggregate_continuation)) != (ssize_t)sizeof(aggregate_continuation))
		return fail("write aggregate-limit continuation");
	if (websocket_process_input(&d) == 0)
		return fail("accepted message beyond aggregate limit");
	free(d.ws_message_buffer);
	d.ws_message_buffer = NULL;
	d.ws_message_len = 0;

	const char *bad_key =
		"GET / HTTP/1.1\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Key: invalid\r\n"
		"Sec-WebSocket-Version: 13\r\n\r\n";
	descriptor_data invalid{};
	if (websocket_parse_handshake(&invalid, bad_key, strlen(bad_key)) != 0)
		return fail("malformed key was accepted");

	int pressure_pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pressure_pair) != 0)
		return fail("backpressure socketpair");
	int pressure_flags = fcntl(pressure_pair[0], F_GETFL, 0);
	int peer_flags = fcntl(pressure_pair[1], F_GETFL, 0);
	if (pressure_flags < 0 || peer_flags < 0 ||
	    fcntl(pressure_pair[0], F_SETFL, pressure_flags | O_NONBLOCK) < 0 ||
	    fcntl(pressure_pair[1], F_SETFL, peer_flags | O_NONBLOCK) < 0)
		return fail("set nonblocking backpressure socket");
	int small_send_buffer = 1024;
	setsockopt(pressure_pair[0], SOL_SOCKET, SO_SNDBUF, &small_send_buffer, sizeof(small_send_buffer));
	unsigned char fill[4096] = {0};
	while (write(pressure_pair[0], fill, sizeof(fill)) > 0)
		{}
	if (errno != EAGAIN && errno != EWOULDBLOCK)
		return fail("did not reach socket backpressure");
	descriptor_data pressure{};
	pressure.descriptor = pressure_pair[0];
	pressure.websocket = 1;
	pressure.ws_state = WS_STATE_OPEN;
	if (websocket_send_text(&pressure, "one") < 0 || websocket_send_text(&pressure, "two") < 0 || pressure.write_failed || pressure.ws_output_len == 0)
		return fail("backpressure was treated as a permanent WebSocket send failure");
	unsigned char drained[8192];
	while (read(pressure_pair[1], drained, sizeof(drained)) > 0)
		{}
	if (websocket_flush_output(&pressure) < 0 || pressure.ws_output_len != 0)
		return fail("queued WebSocket output did not flush after backpressure cleared");
	unsigned char ordered_frames[] = {0x81, 0x03, 'o', 'n', 'e', 0x81, 0x03, 't', 'w', 'o'};
	ssize_t ordered_len = read(pressure_pair[1], drained, sizeof(drained));
	if (ordered_len != (ssize_t)sizeof(ordered_frames) || memcmp(drained, ordered_frames, sizeof(ordered_frames)) != 0)
		return fail("queued WebSocket frames were reordered or corrupted");

	while (write(pressure_pair[0], fill, sizeof(fill)) > 0)
		{}
	if (errno != EAGAIN && errno != EWOULDBLOCK)
		return fail("did not re-establish backpressure before close");
	pressure.ws_state = WS_STATE_OPEN;
	if (websocket_close(&pressure, 1000, "bye"), pressure.ws_state != WS_STATE_CLOSING || pressure.write_failed || pressure.ws_control_output_len == 0)
		return fail("queued close frame did not survive state transition");
	while (read(pressure_pair[1], drained, sizeof(drained)) > 0)
		{}
	if (websocket_flush_output(&pressure) < 0 || pressure.ws_output_len != 0)
		return fail("queued close frame did not flush");
	unsigned char close_frame[] = {0x88, 0x05, 0x03, 0xE8, 'b', 'y', 'e'};
	ssize_t close_len = read(pressure_pair[1], drained, sizeof(drained));
	if (close_len != (ssize_t)sizeof(close_frame) || memcmp(drained, close_frame, sizeof(close_frame)) != 0)
		return fail("queued close frame was corrupted or reordered");

	while (write(pressure_pair[0], fill, sizeof(fill)) > 0)
		{}
	pressure.ws_state = WS_STATE_OPEN;
	unsigned char large_payload[65535] = {0};
	int queue_full_seen = 0;
	for (int i = 0; i < 100; i++)
	{
		if (websocket_send_binary(&pressure, large_payload, sizeof(large_payload)) < 0)
		{
			queue_full_seen = 1;
			break;
		}
	}
	if (!queue_full_seen || pressure.write_failed)
		return fail("application queue saturation became a connection failure");
	if (websocket_send_pong(&pressure, "p", 1) < 0 || pressure.write_failed || pressure.ws_control_output_len == 0)
		return fail("control frame was blocked by application queue saturation");
	while (read(pressure_pair[1], drained, sizeof(drained)) > 0)
		{}
	if (websocket_flush_output(&pressure) < 0)
		return fail("saturated output queues did not flush");
	unsigned char priority_pong[] = {0x8A, 0x01, 'p'};
	ssize_t priority_len = read(pressure_pair[1], drained, sizeof(drained));
	if (priority_len < (ssize_t)sizeof(priority_pong) || memcmp(drained, priority_pong, sizeof(priority_pong)) != 0)
		return fail("control frame was not prioritized ahead of application data");

	pressure.ws_last_ping = time(NULL);
	pressure.ws_pong_received = 1;
	pressure.ws_ping_queued = 1;
	pressure.ws_ping_outstanding = 1;
	websocket_free(&pressure);
	if (pressure.ws_output_buffer || pressure.ws_output_len != 0 || pressure.ws_output_offset != 0 ||
	    pressure.ws_control_output_buffer || pressure.ws_control_output_len != 0 || pressure.ws_control_output_offset != 0 ||
	    pressure.ws_last_ping != 0 || pressure.ws_pong_received != 0 || pressure.ws_ping_queued != 0 || pressure.ws_ping_outstanding != 0)
		return fail("websocket_free left stale output or heartbeat state");
	close(pressure_pair[0]);
	close(pressure_pair[1]);

	d.ws_compressed_message = 1;
	d.ws_error_code = WS_CLOSE_MESSAGE_TOO_BIG;
	const char oversized_handshake = 'x';
	if (websocket_parse_handshake(&d, &oversized_handshake, WS_MAX_HANDSHAKE_SIZE + 1) != -1)
		return fail("oversized direct handshake was not rejected");
	const char invalid_compression_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Version: 13\r\nSec-WebSocket-Extensions: permessage-deflate\r\n\r\n";
	if (websocket_parse_handshake(&d, invalid_compression_handshake, sizeof(invalid_compression_handshake) - 1) != 0 || d.ws_deflate_requested)
		return fail("invalid handshake retained compression negotiation state");
	const char lone_lf_handshake[] =
		"GET / HTTP/1.1\nUpgrade: websocket\nConnection: Upgrade\n\n";
	if (websocket_parse_handshake(&d, lone_lf_handshake, sizeof(lone_lf_handshake) - 1) != 0)
		return fail("lone-LF handshake was accepted");
	const char embedded_nul_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n\0"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
	if (websocket_parse_handshake(&d, embedded_nul_handshake, sizeof(embedded_nul_handshake) - 1) != 0)
		return fail("embedded-NUL handshake was accepted");
	const char malformed_header_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	if (websocket_parse_handshake(&d, malformed_header_handshake, sizeof(malformed_header_handshake) - 1) != 0)
		return fail("header without colon was accepted");
	const char folded_header_handshake[] =
		"GET / HTTP/1.1\r\n Upgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	if (websocket_parse_handshake(&d, folded_header_handshake, sizeof(folded_header_handshake) - 1) != 0)
		return fail("leading-whitespace header was accepted");
	const char noncanonical_version_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: +13\r\n\r\n";
	if (websocket_parse_handshake(&d, noncanonical_version_handshake, sizeof(noncanonical_version_handshake) - 1) != 0)
		return fail("noncanonical websocket version was accepted");
	const char control_value_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\x0b\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	if (websocket_parse_handshake(&d, control_value_handshake, sizeof(control_value_handshake) - 1) != 0)
		return fail("control character in header value was accepted");
	const char noncanonical_key_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZR==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	if (websocket_parse_handshake(&d, noncanonical_key_handshake, sizeof(noncanonical_key_handshake) - 1) != 0)
		return fail("noncanonical websocket key was accepted");
	const char token_list_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket, h2\r\nConnection: keep-alive, Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	if (websocket_parse_handshake(&d, token_list_handshake, sizeof(token_list_handshake) - 1) != 1)
		return fail("valid upgrade token list was rejected");
	const char lookalike_connection_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: NotUpgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	if (websocket_parse_handshake(&d, lookalike_connection_handshake, sizeof(lookalike_connection_handshake) - 1) != 0)
		return fail("connection lookalike token was accepted");
	const char invalid_extension_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Extensions: x-permessage-deflate\r\n\r\n";
	if (websocket_parse_handshake(&d, invalid_extension_handshake, sizeof(invalid_extension_handshake) - 1) != 1 || d.ws_deflate_requested)
		return fail("substring extension token was incorrectly negotiated");
	const char duplicate_key_handshake[] =
		"GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
		"Sec-WebSocket-Version: 13\r\n\r\n";
	if (websocket_parse_handshake(&d, duplicate_key_handshake, sizeof(duplicate_key_handshake) - 1) != 0)
		return fail("duplicate websocket key was accepted");

	int failed_handshake_pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, failed_handshake_pair) != 0)
		return fail("failed-handshake socketpair");
	descriptor_data failed_handshake{};
	failed_handshake.descriptor = failed_handshake_pair[0];
	failed_handshake.ws_state = WS_STATE_CONNECTING;
	failed_handshake.ws_deflate_requested = 1;
	close(failed_handshake_pair[1]);
	if (websocket_complete_handshake(&failed_handshake, "dGhlIHNhbXBsZSBub25jZQ==") >= 0 ||
	    failed_handshake.ws_deflate_requested || failed_handshake.ws_compress ||
	    failed_handshake.ws_deflate_stream || failed_handshake.ws_inflate_stream ||
	    failed_handshake.ws_handshake_done || failed_handshake.ws_state != WS_STATE_CLOSED)
	{
		close(failed_handshake_pair[0]);
		return fail("failed handshake retained negotiated state");
	}
	close(failed_handshake_pair[0]);

	websocket_free(&d);
	if (d.ws_compressed_message || d.ws_error_code || d.ws_compress || d.ws_inflate_stream || d.ws_deflate_stream)
		return fail("websocket_free left stale compression state");

	free(d.ws_fragment_buffer);
	free(d.ws_message_buffer);
	close(pair[0]);
	close(pair[1]);
	puts("WebSocket runtime harness passed");
	return 0;
}
