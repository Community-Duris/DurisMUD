/*
 * websocket.h - websocket support for durismud
 *
 * rfc 6455 websocket protocol for browser clients.
 * runs alongside telnet/ssl on port 4050.
 */

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "structs.h"

/* config */
#define WS_PORT           4050
#define WS_MAX_FRAME_SIZE 65536
#define WS_MAGIC_STRING   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_PING_INTERVAL  30
#define WS_PING_TIMEOUT   60

/* buffer sizes */
#define WS_LISTEN_BACKLOG       40
#define WS_INPUT_BUFFER_SIZE    4096
#define WS_ACCEPT_KEY_SIZE      64
#define WS_RESPONSE_BUFFER_SIZE 512
#define WS_KEY_BUFFER_SIZE      128
#define WS_CLOSE_PAYLOAD_SIZE   128
#define WS_CONCAT_BUFFER_SIZE   256
#define WS_MAX_HANDSHAKE_SIZE   8192
#define WS_HANDSHAKE_TIMEOUT    30
#define WS_KEY_DECODED_SIZE     16
#define WS_KEY_ENCODED_SIZE     24
#define WS_MAX_MESSAGE_SIZE     (1024 * 1024)
#define WS_MAX_BUFFERED_BYTES   (1024 * 1024)
#define WS_MAX_OUTPUT_BYTES     (4 * 1024 * 1024)
#define WS_CONTROL_OUTPUT_RESERVE 4096
#define WS_MAX_FRAMES_PER_READ 64
#define WS_MAX_FLUSH_BYTES 65536
#define WS_OUTPUT_QUEUE_FULL (-2)
#define WS_AUTH_MAX_FAILURES 5
#define WS_AUTH_FAILURE_WINDOW 60

/* frame length thresholds (rfc 6455) */
#define WS_LEN_7BIT_MAX     125
#define WS_LEN_16BIT_MAX    65535
#define WS_PROTOCOL_VERSION 13

/* compression (rfc 7692 permessage-deflate) */
#define WS_COMPRESS_THRESHOLD 64

/* opcodes (rfc 6455 section 5.2) */
#define WS_OPCODE_CONTINUATION 0x00
#define WS_OPCODE_TEXT         0x01
#define WS_OPCODE_BINARY       0x02
#define WS_OPCODE_CLOSE        0x08
#define WS_OPCODE_PING         0x09
#define WS_OPCODE_PONG         0x0A

/* close codes (rfc 6455 section 7.4.1) */
#define WS_CLOSE_NORMAL             1000
#define WS_CLOSE_GOING_AWAY         1001
#define WS_CLOSE_PROTOCOL_ERROR     1002
#define WS_CLOSE_UNSUPPORTED        1003
#define WS_CLOSE_NO_STATUS          1005
#define WS_CLOSE_ABNORMAL           1006
#define WS_CLOSE_INVALID_DATA       1007
#define WS_CLOSE_POLICY_VIOLATION   1008
#define WS_CLOSE_MESSAGE_TOO_BIG    1009
#define WS_CLOSE_EXTENSION_REQUIRED 1010
#define WS_CLOSE_INTERNAL_ERROR     1011

/* connection states */
#define WS_STATE_CONNECTING 0 /* http upgrade in progress */
#define WS_STATE_OPEN       1 /* handshake done, ready for data */
#define WS_STATE_CLOSING    2 /* close frame sent, waiting */
#define WS_STATE_CLOSED     3 /* connection closed */

/* websocket data attached to descriptor */
struct websocket_data
{
	int    state;           /* WS_STATE_* */
	int    handshake_done;  /* http upgrade complete */
	char  *fragment_buffer; /* for fragmented messages */
	size_t fragment_len;    /* fragment buffer length */
	int    fragment_opcode; /* opcode of fragmented message */
};

/* init/shutdown */
int  websocket_init(int port);
void websocket_shutdown(void);

/* connection handling */
int websocket_accept(int listen_fd, struct descriptor_data *d);
int websocket_process_input(struct descriptor_data *d);
int websocket_has_pending(struct descriptor_data *d);

/* http upgrade handshake */
int websocket_parse_handshake(struct descriptor_data *d, const char *buf, size_t len);
int websocket_complete_handshake(struct descriptor_data *d, const char *key);

/* frame sending */
int websocket_send_text(struct descriptor_data *d, const char *text);
int websocket_send_binary(struct descriptor_data *d, const void *data, size_t len);
int websocket_send_json(struct descriptor_data *d, const char *type, const char *package, const char *json);
int websocket_send_close(struct descriptor_data *d, int code, const char *reason);
int websocket_send_ping(struct descriptor_data *d);
int websocket_send_pong(struct descriptor_data *d, const char *data, size_t len);
int websocket_flush_output(struct descriptor_data *d);

/* frame parsing - returns bytes consumed, -1 on error, 0 if need more data */
int websocket_parse_frame(struct descriptor_data *d, const char *buf, size_t len, char **payload, size_t *payload_len, int *opcode, int *fin);

/* utility */
void websocket_generate_accept_key(const char *client_key, char *accept_key);
#define IS_WEBSOCKET(d) ((d) && (d)->websocket)
void websocket_close(struct descriptor_data *d, int code, const char *reason);
void websocket_free(struct descriptor_data *d);

#endif /* WEBSOCKET_H */
