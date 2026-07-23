#include "config.h"
#include "gmcp.h"
#include "structs.h"
#include "telnet.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

P_desc descriptor_list = nullptr;
long sentbytes = 0;

static int gmcp_negotiations = 0;
static int ttype_negotiations = 0;

void * __malloc(size_t size, char *, char *, int) { return std::calloc(1, size); }
void __free(void *ptr, char *, int) { std::free(ptr); }
void panic_corruption_int(const char *, const char *, ...) { std::abort(); }
void panic_corruption(const char *, const char *, ...) { std::abort(); }
void logit(const char *, const char *, ...) {}

void gmcp_handle_negotiation(P_desc, int) { ++gmcp_negotiations; }
void gmcp_handle_input(P_desc, const char *, size_t) {}
void ttype_handle_negotiation(P_desc, int) { ++ttype_negotiations; }
void ttype_handle_subnegotiation(P_desc, const unsigned char *, int) {}
int websocket_send_text(P_desc, const char *) { return 0; }
char *json_escape_ansi_string(const char *) { return nullptr; }

int parse_telnet_options(P_desc player, char *buf, int buflen);

static void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

int main()
{
	descriptor_data descriptor{};

	char one_byte[] = {static_cast<char>(IAC), 0};
	require(parse_telnet_options(&descriptor, one_byte, 1) == 0,
	        "single IAC byte must remain pending");

	char two_byte_do[] = {static_cast<char>(IAC), static_cast<char>(DO), 0};
	require(parse_telnet_options(&descriptor, two_byte_do, 2) == 0,
	        "IAC DO without option byte must remain pending");
	require(gmcp_negotiations == 0, "partial negotiation must have no side effects");

	char complete_gmcp[] = {static_cast<char>(IAC), static_cast<char>(DO),
	                        static_cast<char>(TELOPT_GMCP), 0};
	require(parse_telnet_options(&descriptor, complete_gmcp, 3) == 3,
	        "complete GMCP negotiation must consume three bytes");
	require(gmcp_negotiations == 1, "complete GMCP negotiation must dispatch once");

	char two_iac[] = {static_cast<char>(IAC), static_cast<char>(IAC), 0};
	require(parse_telnet_options(&descriptor, two_iac, 2) == 2,
	        "escaped IAC must consume both bytes");

	char partial_subnegotiation[] = {static_cast<char>(IAC), static_cast<char>(SB),
	                                 static_cast<char>(TELOPT_TTYPE), 0};
	require(parse_telnet_options(&descriptor, partial_subnegotiation, 3) == 0,
	        "unterminated subnegotiation must remain pending");
	require(ttype_negotiations == 0, "partial subnegotiation must have no side effects");

	std::puts("Telnet input runtime harness passed");
	return 0;
}
