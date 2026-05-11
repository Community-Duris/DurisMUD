#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "mccp.h"
#include <errno.h>
#include <gnutls/gnutls.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "copyover.h"
#include "gmcp.h"
#include "json_utils.h"
#include "telnet.h"
#include "ttype.h"
#include "unicode.h"
#include "websocket.h"

/* external variables used by this module */
extern P_desc descriptor_list;
extern long   sentbytes;

/* global variables provided by this module */
int mccp_alloc = 0;
int mccp_free  = 0;

const unsigned char compress_on_str[]  = {IAC, WILL, TELOPT_COMPRESS};
const unsigned char compress2_on_str[] = {IAC, WILL, TELOPT_COMPRESS2};
const unsigned char enable_compress[]  = {IAC, SB, TELOPT_COMPRESS, WILL, SE};
const unsigned char enable_compress2[] = {IAC, SB, TELOPT_COMPRESS2, IAC, SE};
const unsigned char sga_will_str[]     = {IAC, WILL, TELOPT_SGA};
const unsigned char ga_str[]           = {IAC, GA};

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
void  zlib_free(void *opaque, void *address);
int   raw_write_to_descriptor(P_desc desc, const char *txt, const int total);

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
	char *p;
	mccp_alloc++;
	CREATE(p, char, items *size, MEM_TAG_ZSTREAM);
	return (void *)p;
}

void zlib_free(void *opaque, void *address)
{
	mccp_free++;
	FREE(address);
}

void advertise_mccp(P_desc desc)
{
	write_to_descriptor_binary(desc, compress2_on_str, sizeof compress2_on_str);
	write_to_descriptor_binary(desc, compress_on_str, sizeof compress_on_str);
}

void sga_negotiate(P_desc desc) { write_to_descriptor_binary(desc, sga_will_str, 3); }

int send_ga(P_desc desc)
{
	if (desc && !desc->sga_disabled && !desc->websocket)
		return write_to_descriptor_binary(desc, ga_str, 2);
	return 0;
}

/* parse telnet options and return amount of characters
 * to "cut" from input stream
 * If you ever need to make it "the right way", look into
 * sources of telnetd demon */
int parse_telnet_options(P_desc player, char *buf, int buflen)
{
	ubyte *p        = (ubyte *)(buf);
	int    mccp_ver = 0;

	if (buflen < 1 || *p != IAC)
		return 0;

	switch (*(p + 1))
	{
		case DO:
			switch (*(p + 2))
			{
				case TELOPT_COMPRESS:
					if (mccp_ver < MCCP_VER2) // prefer version 2 over 1
						mccp_ver = MCCP_VER1;
					break;
				case TELOPT_COMPRESS2:
					mccp_ver = MCCP_VER2;
					break;
				case TELOPT_GMCP:
					gmcp_handle_negotiation(player, DO);
					return 3;
				case TELOPT_SGA:
					player->sga_disabled = 1;
					return 3;
			}
			break;
		case DONT:
			switch (*(p + 2))
			{
				case TELOPT_GMCP:
					gmcp_handle_negotiation(player, DONT);
					return 3;
				case TELOPT_SGA:
					player->sga_disabled = 0;
					return 3;
			}
			/* fall through */
		case WILL:
			if (*(p + 2) == TELOPT_TTYPE)
				ttype_handle_negotiation(player, WILL);
			return (*(p + 2) != '\0') ? 3 : 2;
		case WONT:
			if (*(p + 2) == TELOPT_TTYPE)
				ttype_handle_negotiation(player, WONT);
			return (*(p + 2) != '\0') ? 3 : 2;
		case SB: /* subnegotiation */
		{
			int len = 2;
			while (len < buflen && p[len] != '\0' && !(p[len] == IAC && p[len + 1] == SE))
				len++;

			/* incomplete, wait for more */
			if (len >= buflen || p[len] == '\0')
				return 0;

			if (p[len] == IAC && p[len + 1] == SE)
				len += 2;
			else
				return 0;

			if (p[2] == TELOPT_TTYPE && len > 4)
			{
				if (p[3] == TELQUAL_IS)
				{
					ttype_handle_subnegotiation(player, p + 3, len - 5);
				}
				else
				{
					player->ttype_state = TTYPE_COMPLETE;
					return len;
				}
			}
			/* If GMCP subnegotiation, pass data to handler */
			else if (p[2] == TELOPT_GMCP && len > 5)
			{
				gmcp_handle_input(player, (const char *)(p + 3), len - 5);
			}
			return len;
		}
	}

	if (mccp_ver)
	{
		compress_start(player, mccp_ver);
		return 3;
	}
	return 1; /* lets cut at least IAC from stream */
}

int compress_start(P_desc player, int mccp_version)
{
	z_stream *s;

	if (player->z_str)
		return 0;

	CREATE(s, z_stream, 1, MEM_TAG_ZSTREAM);
	// s = (z_stream *) malloc(sizeof(z_stream));
	CREATE(player->out_compress_buf, char, COMPRESS_BUF_SIZE, MEM_TAG_BUFFER);
	// player->out_compress_buf = (char *) malloc(COMPRESS_BUF_SIZE);

	s->next_in   = NULL;
	s->avail_in  = 0;
	s->next_out  = (Bytef *)player->out_compress_buf;
	s->avail_out = COMPRESS_BUF_SIZE;
	s->zalloc    = zlib_alloc;
	s->zfree     = zlib_free;
	s->opaque    = NULL;

	if (deflateInit(s, COMPRESS_EFFICIENCY) != Z_OK)
	{
		FREE(player->out_compress_buf);
		FREE(s);
		logit(LOG_DEBUG, "MCCP: deflateInit failed");
		return -1;
	}

	if (mccp_version == MCCP_VER1)
	{
		write_to_descriptor_binary(player, enable_compress, sizeof enable_compress);
	}
	else if (mccp_version == MCCP_VER2)
	{
		write_to_descriptor_binary(player, enable_compress2, sizeof enable_compress2);
	}
	else
	{
		logit(LOG_DEBUG, "MCCP: unknown version %d", mccp_version);
	}
	player->out_compress = mccp_version;
	player->z_str        = s;

	return 0;
}

/* ZMUD seems not to handle Z_FINISH event properly, so socket needs
 to be closed immediatly after stopping compression! */
int compress_end(P_desc player, int flush)
{
	unsigned char dummy[1] = {' '};
	int           status, len;

	if (!player->out_compress || !player->z_str)
		return 0;

	player->z_str->avail_in  = 0;
	player->z_str->next_in   = dummy;
	player->z_str->next_out  = (Bytef *)player->out_compress_buf;
	player->z_str->avail_out = COMPRESS_BUF_SIZE;

	/* flush all pending data, Z_OK means there's still more data to process */
	if (flush)
	{
		do
		{
			status = deflate(player->z_str, Z_FINISH);
			if (status != Z_STREAM_END && status != Z_OK)
			{
				break;
			}
			len = (long)player->z_str->next_out - (long)player->out_compress_buf;
			if (raw_write_to_descriptor(player, player->out_compress_buf, len) < 0)
				break;
		} while (status != Z_STREAM_END);
	}

	deflateEnd(player->z_str); /* free memory allocated by zlib */
	FREE(player->out_compress_buf);
	FREE(player->z_str);
	player->out_compress = 0;

	return 0;
}

/* use this function whenever you want to send anything to player,
 do not attempt to call raw_write_to_descriptor, or you may
 screw up compression */
int write_to_descriptor(P_desc player, const char *txt)
{
	int   len, total, status, i, j;
	char  static_conv_buf[MAX_STRING_LENGTH];
	char *conv_buf = static_conv_buf;

	if (player->write_failed)
		return -1;

	/* WebSocket connections need JSON-wrapped text frames */
	if (player->websocket)
	{
		char *escaped = json_escape_ansi_string(txt);
		if (escaped && escaped[0] != '\0')
		{
			/* Skip empty messages */
			char  *json_msg = NULL;
			size_t msg_len  = strlen(escaped) + 64;
			json_msg        = (char *)malloc(msg_len);
			if (json_msg)
			{
				snprintf(json_msg, msg_len, "{\"type\":\"text\",\"category\":\"info\",\"data\":\"%s\"}", escaped);
				websocket_send_text(player, json_msg);
				free(json_msg);
			}
		}
		if (escaped)
			free(escaped);
		return 0;
	}

	if ((len = strlen(txt)) > MAX_STRING_LENGTH * 0.8)
		CREATE(conv_buf, char, (unsigned int)(len * 1.1), MEM_TAG_BUFFER);
	// conv_buf = (char *) malloc((unsigned int) (len * 1.1));

	for (i = 0, j = 0; txt[i]; i++)
	{
		if (txt[i] == '\n')
		{
			conv_buf[j++] = '\r';
			conv_buf[j++] = '\n';
		}
		else if (txt[i] != '\r')
			conv_buf[j++] = txt[i];
	}

	conv_buf[j] = '\0';
	txt         = conv_buf;
	total       = j;
	char down[j + 1];

	if (player->cp437)
	{
		downgrade_string(down, txt, u_cp437);
		txt   = down;
		total = strlen(txt);
	}

	int ret = write_to_descriptor_binary(player, (const unsigned char*)txt, total);
	if (conv_buf != static_conv_buf)
		FREE(conv_buf);
	return ret;
}

/* never ever call this function, unless you are write_to_descriptor */
int raw_write_to_descriptor(P_desc d, const char *txt, const int total)
{
	int sofar, thisround;

	sofar = 0;

	sentbytes += total;

	if (d->character && !IS_NPC(d->character))
		d->character->only.pc->send_data = d->character->only.pc->send_data + total;

	if (d->sslses)
	{
		int ret = gnutls_record_send(d->sslses, txt, total);
		// retry on interrupt, but not on buffer full
		while (ret == GNUTLS_E_INTERRUPTED)
			ret = gnutls_record_send(d->sslses, NULL, 0);
		if (ret == GNUTLS_E_AGAIN)
		{
			// ssl buffer full, skip this write and try next tick
			return 0;
		}
		if (ret < 0)
		{
			logit(LOG_COMM, "Write to SSL socket error: %s (ret=%d)", gnutls_strerror(ret), ret);
			d->write_failed = 1;
			return -1;
		}
	}
	else
		do
		{
			thisround = write(d->descriptor, txt + sofar, (unsigned)(total - sofar));
			if (thisround < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					// socket buffer full, skip this write and try next tick
					return (0);
				}
				logit(LOG_COMM, "Write to socket error: %s (errno=%d)", strerror(errno), errno);
				d->write_failed = 1;
				return (-1);
			}
			if (thisround == 0)
			{
				// wrote nothing - treat like eagain, try again next tick
				return (0);
			}
			sofar += thisround;
		} while (sofar < total);

	return (0);
}

/* Write binary data (like telnet subnegotiations) with compression support
 * but without text conversions (no \n->\r\n, no charset downgrade) */
int write_to_descriptor_binary(P_desc player, const unsigned char *data, size_t len)
{
	int  status;
	long out_len;

	if (!player || !data || len == 0)
		return 0;

	if (player->write_failed)
		return -1;

	if (!player->out_compress)
	{
		if (raw_write_to_descriptor(player, (const char *)data, len) < 0)
			return -1;
	}
	else
	{
		if (player->z_str)
		{
			player->z_str->next_in  = (unsigned char *)data;
			player->z_str->avail_in = len;

			while (player->z_str->avail_in)
			{
				do
				{
					player->z_str->next_out  = (Bytef *)player->out_compress_buf;
					player->z_str->avail_out = COMPRESS_BUF_SIZE;

					status = deflate(player->z_str, Z_SYNC_FLUSH);
					if (status != Z_OK)
					{
						return -1;
					}

					out_len = (long)player->z_str->next_out - (long)player->out_compress_buf;
					if (raw_write_to_descriptor(player, player->out_compress_buf, out_len) < 0)
						return -1;
				} while (player->z_str->avail_out == 0);
			}
		}
	}
	return 0;
}

int compress_get_ratio(P_desc player)
{
	if (!player->z_str)
		return 0;

	if (player->z_str->total_in == 0 || player->z_str->total_out == 0)
		return 0;

	return (player->z_str->total_in - player->z_str->total_out) * 100 / player->z_str->total_in;
}
