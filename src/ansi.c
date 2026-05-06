#include <string.h>
#include "ansi.h"
#include "unicode.h"

#if 0
static int char2col(char c)
{
	switch (c)
	{
	case 'l': return 16+0;
	case 'b': return 16+1;
	case 'g': return 16+2;
	case 'c': return 16+3;
	case 'r': return 16+4;
	case 'm': return 16+5;
	case 'y': return 16+6;
	case 'w': return 16+7;
	case 'L': return 16+8;
	case 'B': return 16+9;
	case 'G': return 16+10;
	case 'C': return 16+11;
	case 'R': return 16+12;
	case 'M': return 16+13;
	case 'Y': return 16+14;
	case 'W': return 16+15;
	default:  return 0;
	}
}
#else
static const unsigned char letters[64] =
{
	0,  0, 25, 27,  0,  0,  0, 26,  0,  0,  0,  0, 24, 29,  0,  0,
	0,  0, 28,  0,  0,  0,  0, 31,  0, 30,  0,  0,  0,  0,  0,  0,
	0,  0, 17, 19,  0,  0,  0, 18,  0,  0,  0,  0, 16, 21,  0,  0,
	0,  0, 20,  0,  0,  0,  0, 23,  0, 22,  0,  0,  0,  0,  0,  0,
};

static int char2col(char c)
{
	if (c>=64 && c<128)
		return letters[c - 64];
	return 0;
}
#endif

static char col2char[] = "lbgcrmywLBGCRMYW";

static void put_ansi(char *&out, int attr)
{
	int f = GET_FG(attr);
	int b = GET_BG(attr);

	if (!GET_ATTR(attr))
		*out++='&', *out++='n';
	else if (!b)
		*out++='&', *out++='+', *out++=col2char[f-16];
	else if (!f)
		*out++='&', *out++='-', *out++=col2char[b-16];
	else
		*out++='&', *out++='=', *out++=col2char[b-16], *out++=col2char[f-16];
}

/*
Terminal colors: "\e[" then codes separated by ';' then "m"
 * 0: reset all (can be abbreviated to an empty string)
 * 1: bold
 * 2: half-bright
 * 4: underline
 * 5: blink
 * 8: strikethrough
 * 30..37: foreground color (in bgr order)
 * 38: 256/24-bit foreground color (extra codes follow)
 * 39: default foreground color
 * 40..47: background color
 * 48: 256/24-bit foreground color (extra codes follow)
 * 49: default background color
 * 90..97: bright foreground colors (avoiding ambiguous bold)
 * 100..107: bright background colors
Various clients have varying support.
*/

static const char *bgr="04261537";

static void put_term(char *&out, int attr, int lastbit)
{
	int f = GET_FG(attr);
	int b = GET_BG(attr);

	*out++='\e';
	*out++='[';

	if (f)
	{
		if (f & 8)
			*out++=';', *out++='1';
		*out++=';';
		*out++='3';
		*out++ = bgr[f & 7];
	}

	if (b)
	{
		if (b & 8)
		{
			switch (lastbit)
			{
			case TL_BLINK:
				*out++=';', *out++='5', *out++=';', *out++='4';
				break;
			case TL_UNDERLINE:
				*out++=';', *out++='4', *out++=';', *out++='4';
				break;
			case TL_BRIGHT_BG:
				*out++=';', *out++='1', *out++='0';
				break;
			}
		}
		else
			*out++=';', *out++='4';

		*out++ = bgr[b & 7];
	}

	*out++='m';
	*out=0;
}

void AnsiString::set(const char *txt)
{
	clear();

	int attr = 0, a, b;

	while (*txt)
	{
		if (txt[0] == '\r')
			txt++;
		else if (txt[0] == '\n')
		{
			attr = 0;
			push_back('\n');
			txt++;
		}
		else if (txt[0] != '&')
			push_back(get_utf8(txt) | attr);
		else switch (txt[1])
		{
		case 'n':
		case 'N':
			attr = 0;
			txt += 2;
			break;
		case '+':
			if (!(a = char2col(txt[2])))
				goto bad_ansi;
			SET_BG(attr, 0);
			SET_FG(attr, a);
			txt += 3;
			break;
		case '-':
			if (!(a = char2col(txt[2])))
				goto bad_ansi;
			SET_BG(attr, a);
			SET_FG(attr, 0);
			txt += 3;
			break;
		case '=':
			if (!(a = char2col(txt[2])) || !(b = char2col(txt[3])))
				goto bad_ansi;
			SET_BG(attr, a);
			SET_FG(attr, b);
			txt += 4;
			break;
		default:
		bad_ansi:
			push_back('&' | attr);
			txt++;
		}
	}
}

void AnsiString::ansi(char *out) const
{
	char *limit = out + MAX_STRING_LENGTH - 11; // ansi 4 + utf8 4 + final &n 2 + null 1

	int oattr = 0;
	for (wchar_t c : *this)
	{
		if (out >= limit)
			break;

		int attr = GET_ATTR(c);
		if (oattr != attr)
			put_ansi(out, oattr = attr);
		if (GET_CHAR(c) == '&')
			oattr = -1; // force a redundant ansi to defuse the '&'
		put_utf8(out, GET_CHAR(c));
	}

	if (oattr)
		*out++='&', *out++='n';
	*out = 0;
}

void AnsiString::plain(char *out) const
{
	char *limit = out + MAX_STRING_LENGTH - 5; // utf8 4 + null 1

	for (wchar_t c : *this)
	{
		if (out >= limit)
			break;
		put_utf8(out, GET_CHAR(c));
	}
	*out = 0;
}

void AnsiString::term(char *out, int lastbit) const
{
	char *limit = out + MAX_STRING_LENGTH - 64; // safety for large mods

	int oattr = 0;
	for (wchar_t ch : *this)
	{
		if (out >= limit)
			break;

		wchar_t c = GET_CHAR(ch);
		if (c == '\r')
			; // ignore, ensure that there's always only 1 before \n
		else if (c == '\n')
		{
			if (oattr)
			{
				*out++='\e', *out++='[', *out++='m';
				oattr = 0;
			}
			*out++='\r', *out++='\n';
		}
		else
		{
			int attr = GET_ATTR(ch);
			if (oattr != attr)
				put_term(out, oattr = attr, lastbit);
			put_utf8(out, c);
		}
	}

	if (oattr)
		*out++='\e', *out++='[', *out++='m';
	*out = 0;
}

void AnsiString::colorize(int attr)
{
	for (wchar_t &c : *this)
		c = GET_CHAR(c) | attr;
}
