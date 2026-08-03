#include "prototypes.h"
#include <errno.h>
#include <gnutls/gnutls.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "config.h"

static gnutls_certificate_credentials_t x509_cred      = 0;
static gnutls_priority_t                priority_cache = 0;

#define YELL(msg, ...)                                                                                                                                                                                 \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
		printf(msg, __VA_ARGS__);                                                                                                                                                                      \
		logit(LOG_SYS, msg, __VA_ARGS__);                                                                                                                                                              \
	} while (0)

// read the cert, skipping if file date is the same.
// Errors are ok if we already have one (assume the cert is being updated),
// fatal on startup.
void ssl_read_cert(void)
{
	static timespec                  cert_time = {0, 0};
	struct stat                      st;
	gnutls_certificate_credentials_t cred    = 0;
	const char                      *errfunc = 0;
	int                              err;
	const char                      *certfile = CERTFILE;
	const char                      *keyfile  = KEYFILE;

	if (!priority_cache)
		if ((err = gnutls_priority_init(&priority_cache, NULL, NULL)) < 0)
		{
			YELL("Can't allocate SSL priority cache: %s\n", gnutls_strerror(err));
			exit(1);
		}

	if ((err = stat(certfile, &st)))
	{
		struct stat fallback_cert;
		struct stat fallback_key;
		if (!stat("localhost.crt", &fallback_cert) && !stat("localhost.key", &fallback_key))
		{
			certfile = "localhost.crt";
			keyfile  = "localhost.key";
			st       = fallback_cert;
		}
		else
		{
			YELL("Can't stat cert file %s: %s\n", certfile, strerror(errno));
			if (x509_cred)
				return;
			return;
		}
	}

	if (st.st_mtim.tv_sec == cert_time.tv_sec && st.st_mtim.tv_nsec == cert_time.tv_nsec)
		return;

	printf("[%ld] reading ssl cert: %s key: %s (old_mtime=%ld.%ld new_mtime=%ld.%ld)\n", get_time(), certfile, keyfile, cert_time.tv_sec, cert_time.tv_nsec, st.st_mtim.tv_sec, st.st_mtim.tv_nsec);
	cert_time = st.st_mtim;
	if ((err = gnutls_certificate_allocate_credentials(&cred)) < 0 || (err = gnutls_certificate_set_x509_key_file(cred, certfile, keyfile, GNUTLS_X509_FMT_PEM)) < 0 ||
	    (err = gnutls_certificate_set_known_dh_params(cred, GNUTLS_SEC_PARAM_MEDIUM)) < 0)
	{
		if (cred)
			gnutls_certificate_free_credentials(cred);
		YELL("Can't read cert file %s key file %s: %s\n", CERTFILE, KEYFILE, gnutls_strerror(err));
		if (x509_cred)
			return;
		exit(1);
	}

	// Can't deallocate old cert while some session is still using it.
	// We could refcount, but just memory leak for now...
	x509_cred = cred;
}

gnutls_session_t ssl_new(int s)
{
	int              err;
	gnutls_session_t ses     = 0;
	const char      *errfunc = 0;

	ssl_read_cert();

	if (!x509_cred)
		return NULL;

	if ((err = gnutls_init(&ses, GNUTLS_SERVER | GNUTLS_NONBLOCK | GNUTLS_NO_SIGNAL)) < 0)
		errfunc = "gnutls_init";
	else if ((err = gnutls_priority_set(ses, priority_cache)) < 0)
		errfunc = "gnutls_priority_set";
	else if ((err = gnutls_credentials_set(ses, GNUTLS_CRD_CERTIFICATE, x509_cred)) < 0)
		errfunc = "gnutls_credentials_set";

	if (errfunc)
	{
		logit(LOG_SYS, "%s failed: %s", errfunc, gnutls_strerror(err));
		if (ses)
			gnutls_deinit(ses);
		return NULL;
	}

	gnutls_certificate_server_set_request(ses, GNUTLS_CERT_IGNORE);
	gnutls_handshake_set_timeout(ses, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
	gnutls_transport_set_int(ses, s);

	return ses;
}

int ssl_negotiate(gnutls_session_t ses)
{
	int err = gnutls_handshake(ses);

	if (!err)
		return 0;
	if (err == GNUTLS_E_AGAIN || err == GNUTLS_E_INTERRUPTED)
		return 1;
	logit(LOG_COMM, "gnutls_handshake failed: %s", gnutls_strerror(err));
	return 2;
}

void ssl_close(gnutls_session_t ses)
{
	gnutls_bye(ses, GNUTLS_SHUT_RDWR);
	gnutls_deinit(ses);
}
