// Functions for an addressable char+attr string format that hides that both
// Durian ANSI and UTF-8 are multibyte encodings.  You get attr as a number,
// the character as a wchar_t.

// Duris has 17 different colors not 16: &n is a distinct color from &+w;
// some clients and terminals render it differently.  Thus, we need 5 bits.
// Assignment:
//  0        - &n
//  1..15    - unused
//  16+0..7  - &+k..&+w
//  16+8..15 - &+K..&+W

// The background brightness bit (eg &-C) is currently switchable between
// blink and underline (tog underline).  We might add actual bright background
// as a third setting; some old terminals did that and it looked quite good.

// This uses 21 (Unicode) + 5 (fg) + 5 (bg) = 31 bits.
// If we ever add 256-color or 24-bit, we can instantiate basic_string<uint64_t>
// to get more space; extra memory use hardly matters as we store strings at
// rest in &+M form.

#include <string>
#include <vector>
#include "config.h"

#define ATTR_FG(x) ((x) << 21)
#define ATTR_BG(x) ((x) << 26)
#define GET_CHAR(x) ((x) & 0x1fffff)
#define GET_ATTR(x) ((x) &~ 0x1fffff)
#define GET_FG(x) (((x) >> 21) & 31)
#define GET_BG(x) (((x) >> 26) & 31)
#define SET_FG(c, x) (c) = (((c) & 0x7c1fffff) | ATTR_FG(x))
#define SET_BG(c, x) (c) = (((c) & 0x03ffffff) | ATTR_BG(x))

#define MAX_STRING_LENGTH 65536

enum term_lastbit_t
{
	TL_BLINK,
	TL_UNDERLINE,
	TL_BRIGHT_BG,
};

class AnsiString : public std::wstring
{
public:
	void set(const char *txt);
	AnsiString() {}
	AnsiString(const char *txt) { set(txt); }
	void ansi(char *out) const;
	void plain(char *out) const;
	void term(char *out, int lastbit) const;
	void colorize(int attr);

	wchar_t ch(int i) const { return (i<0 || (size_t)i>=size()) ? 0 : GET_CHAR((*this)[i]); }
	int attr(int i) const { return (i<0 || (size_t)i>=size()) ? 0 : GET_ATTR((*this)[i]); }
};
