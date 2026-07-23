#ifndef DURIS_WS_AUTH_H
#define DURIS_WS_AUTH_H

#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Shared DurisWeb HMAC authentication contract for WebSocket and GMCP. */
static int ws_verify_durisweb_signature(const char *sig)
{
	const char *secret = getenv("DURISWEB_SECRET");
	time_t now;
	long minute;

	if (!secret || !*secret || !sig || strlen(sig) != 64)
		return 0;
	for (size_t i = 0; i < 64; i++)
	{
		if (!((sig[i] >= '0' && sig[i] <= '9') || (sig[i] >= 'a' && sig[i] <= 'f') || (sig[i] >= 'A' && sig[i] <= 'F')))
			return 0;
	}

	now = time(NULL);
	minute = now / 60;
	for (int offset = -1; offset <= 1; offset++)
	{
		char ts[32];
		unsigned char digest[EVP_MAX_MD_SIZE];
		unsigned int digest_len = 0;
		char expected[64];

		snprintf(ts, sizeof(ts), "%ld", minute + offset);
		if (!HMAC(EVP_sha256(), secret, strlen(secret), (unsigned char *)ts, strlen(ts), digest, &digest_len) || digest_len != 32)
			continue;
		for (unsigned int i = 0; i < digest_len; i++)
			snprintf(expected + (i * 2), 3, "%02x", digest[i]);
		if (CRYPTO_memcmp(sig, expected, sizeof(expected)) == 0)
			return 1;
	}
	return 0;
}

#endif
