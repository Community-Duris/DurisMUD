/*
 * ttype.c - terminal type (rfc 1091) and mtts support
 *
 * detects mud clients via ttype negotiation and parses
 * mtts bitvector for terminal capabilities like utf-8.
 *
 * mtts cycling:
 *   round 1: client name (e.g. "MUDLET", "TINTIN++")
 *   round 2: terminal type (e.g. "XTERM-256COLOR")
 *   round 3: "MTTS ###" bitvector with capabilities
 *   round 4: same as round 3 = end of cycling
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "ttype.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "telnet.h"

/* telnet sequences */
static const unsigned char ttype_do[]   = {IAC, DO, TELOPT_TTYPE};
static const unsigned char ttype_send[] = {IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE};

/* internal helpers */
static void ttype_send_request(P_desc d);
static void parse_mtts_bitvector(P_desc d, const char *str);

/*
 * ttype_negotiate - initiate ttype negotiation
 */
void ttype_negotiate(P_desc d)
{
	if (!d || d->websocket)
		return;

	write_to_descriptor_binary(d, ttype_do, 3);
	d->ttype_state = TTYPE_SENT_DO;
}

/*
 * ttype_handle_negotiation - handle WILL/WONT response
 */
void ttype_handle_negotiation(P_desc d, int cmd)
{
	if (!d)
		return;

	if (cmd == WILL)
	{
		d->ttype_state   = TTYPE_CYCLING;
		d->ttype_round   = 0;
		d->ttype_last[0] = '\0';
		ttype_send_request(d);
	}
	else if (cmd == WONT)
	{
		d->ttype_state = TTYPE_COMPLETE;
	}
}

/*
 * ttype_send_request - send TTYPE SEND subnegotiation
 */
static void ttype_send_request(P_desc d)
{
	if (!d || d->ttype_state != TTYPE_CYCLING)
		return;

	if (d->ttype_round >= MTTS_MAX_ROUNDS)
	{
		d->ttype_state = TTYPE_COMPLETE;
		return;
	}

	d->ttype_round++;
	write_to_descriptor_binary(d, ttype_send, 6);
}

/*
 * parse_mtts_bitvector - extract flags from "MTTS ###" string
 */
static void parse_mtts_bitvector(P_desc d, const char *str)
{
	long  flags;
	char *endptr;

	if (!d || !str)
		return;

	/* expect "MTTS ###" format */
	if (strncasecmp(str, "MTTS ", 5) != 0)
		return;

	flags = strtol(str + 5, &endptr, 10);
	if (endptr == str + 5 || flags < 0 || flags > 65535)
		return;

	d->mtts_flags = (int)flags;

	if (flags & MTTS_UTF8)
		d->charset_detected = 1;
	check_cp437(d);
}

/*
 * ttype_handle_subnegotiation - parse TTYPE IS response
 *
 * data points to content after IAC SB TTYPE (starts with IS byte)
 * len is length of content (excluding trailing IAC SE)
 */
void ttype_handle_subnegotiation(P_desc d, const unsigned char *data, int len)
{
	char term_type[128];
	int  i, copy_len, offset;

	if (!d || d->websocket || !data || len < 1)
		return;

	if (d->ttype_state != TTYPE_CYCLING)
		return;

	/* check for IS (0) byte - if present, skip it */
	if (data[0] == TELQUAL_IS)
	{
		offset   = 1;
		copy_len = len - 1;
	}
	else
	{
		offset   = 0;
		copy_len = len;
	}

	if (copy_len > 127)
		copy_len = 127;
	if (copy_len < 1)
		return;

	for (i = 0; i < copy_len; i++)
	{
		term_type[i] = data[i + offset];
	}
	term_type[copy_len] = '\0';

	/* convert to uppercase for comparison */
	for (i = 0; term_type[i]; i++)
	{
		term_type[i] = toupper((unsigned char)term_type[i]);
	}

	/* sanity check - should start with letter or digit (for MTTS) */
	if (!isalnum((unsigned char)term_type[0]))
	{
		d->ttype_state = TTYPE_COMPLETE;
		return;
	}

	/* check if same as last response - signals end of cycling */
	if (d->ttype_last[0] && strcmp(term_type, d->ttype_last) == 0)
	{
		d->ttype_state = TTYPE_COMPLETE;
		return;
	}

	/* store for duplicate detection */
	strncpy(d->ttype_last, term_type, sizeof(d->ttype_last) - 1);
	d->ttype_last[sizeof(d->ttype_last) - 1] = '\0';

	/* process based on round */
	switch (d->ttype_round)
	{
		case 1:
			/* round 1: client name */
			strncpy(d->ttype_client, term_type, sizeof(d->ttype_client) - 1);
			d->ttype_client[sizeof(d->ttype_client) - 1] = '\0';
			/* also set client_name for display */
			strncpy(d->client_name, term_type, sizeof(d->client_name) - 1);
			d->client_name[sizeof(d->client_name) - 1] = '\0';
			break;

		case 2:
			/* round 2: terminal type */
			strncpy(d->ttype_terminal, term_type, sizeof(d->ttype_terminal) - 1);
			d->ttype_terminal[sizeof(d->ttype_terminal) - 1] = '\0';
			break;

		case 3:
		case 4:
			/* round 3/4: mtts bitvector */
			if (strncasecmp(term_type, "MTTS", 4) == 0)
			{
				parse_mtts_bitvector(d, term_type);
			}
			break;
	}

	/* request next round */
	ttype_send_request(d);
}

// recheck UTF8 capability
void check_cp437(P_desc d)
{
	/*
	 * charset logic:
	 * - ssl connections: always utf8
	 * - mtts with utf8 flag: utf8
	 * - mtts without utf8 flag: cp437
	 * - zmud/cmud (no mtts support): cp437
	 * - everyone else: utf8 (modern default)
	 */

	if (d->sslses)
		d->cp437 = 0;
	else if (d->mtts_flags)
		d->cp437 = !(d->mtts_flags & MTTS_UTF8);
	else if (!d->ttype_client[0])
		d->cp437 = 0;
	else if (strncasecmp(d->ttype_client, "ZMUD", 4) == 0
		 || strncasecmp(d->ttype_client, "CMUD", 4) == 0
		 || strncasecmp(d->ttype_client, "VT", 2) == 0
		 || strncasecmp(d->ttype_client, "ANSI", 4) == 0
		 || strncasecmp(d->ttype_client, "DUMB", 4) == 0)
	{
		d->cp437 = 1;
	}
	else
		d->cp437 = 0;
}
