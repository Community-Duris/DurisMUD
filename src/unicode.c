#include <stdlib.h>
#include <string.h>
#include "unicode.h"

unimap::unimap()
{
  resize(1);
}

ushort unimap::operator[](int c) const
{
  if (c<0 || c>=(1<<21))
    return 0;
  ushort s = at(0)[c >> 14];
  if (!s)
    return 0;
  ushort p = at(s)[c >> 7];
  if (!p)
    return 0;
  return at(p)[c];
}

void unimap::set(int c, ushort v)
{
  if (c<0 || c>=(1<<21))
    abort();

  // radix with fanout of 128 (7 bits)
  // levels are: Global/Segment/Page/char

  um128 &G = at(0);
  ushort s = G[c >> 14];
  if (!s)
  {
    s = G[c >> 14] = size();
    emplace_back();
  }
  um128 &S = at(s);

  ushort p = S[c >> 7];
  if (!p)
  {
    p = S[c >> 7] = size();
    emplace_back();
  }
  um128 &P = at(p);

  P[c] = v;
}

unimap::unimap(const char16_t conv[256])
{
  resize(1);

  for (int c=0; c<256; c++)
    set(conv[c], c);
}

#define CP437_PRINTABLE \
" !\"#$%&'()*+,-./0123456789:;<=>?" \
"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" \
"`abcdefghijklmnopqrstuvwxyz{|}~⌂"  \
"ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒ"  \
"áíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"  \
"└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"  \
"αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ "

// superfluous entry because dumb ISO thinks all char arrays are strings
const char16_t cp437_u[257] = u" ☺☻♥♦♣♠•◘○◙♂♀♪♫☼▶◀↕‼¶§▬↨↑↓→←∟↔▲▼" CP437_PRINTABLE;

// Control characters map to controls or ' '.
unimap u_cp437(u"       \a\b\t\n  \r             \e    " CP437_PRINTABLE);

int get_utf8(const char *&str)
{
  if (!*str)
    return 0;
  int c = (unsigned char)*str++;
  if (c < 128)
    return c;
  if (c < 0xc0)
  {
eat_junk:
    // junk, eat it all but return only one error
    while ((*str & 0xc0) == 0x80)
      str++;
    return UNI_BAD;
  }

  int len, min;
  if ((c&0xe0)==0xc0)
    len=1, c&=0x1f, min=0x80;
  else if ((c&0xf0)==0xe0)
    len=2, c&=0x0f, min=0x800;
  else if ((c&0xf8)==0xf0)
    len=3, c&=0x07, min=0x10000;
#if 0
  else if ((c&0xfc)==0xf8)
    len=4, c&=0x03, min=0x200000;
  else if ((c&0xfe)==0xfc)
    len=5, c&=0x01, min=0x4000000;
#endif
  else
    goto eat_junk;
  while ((*str & 0xc0) == 0x80)
    c = (c<<6) + (*str++&0x3f), len--;
  if (len)
    return UNI_BAD; // tail too long or too short
  if (c < min)
    return UNI_BAD; // overlong encoding
  return c;
}

void put_utf8(char *&d, int v)
{
  unsigned int uv = v;

  if (uv < 0x80)
  {
    *d++ = uv;
  }
  else if (uv < 0x800)
  {
    *d++ = ( uv >>  6)     | 0xc0;
    *d++ = ( uv    & 0x3f) | 0x80;
  }
  else if (uv < 0x10000)
  {
    *d++ = ( uv >> 12)     | 0xe0;
    *d++ = ((uv >>  6) & 0x3f) | 0x80;
    *d++ = ( uv    & 0x3f) | 0x80;
  }
  else if (uv < 0x110000)
  {
    *d++ = ( uv >> 18)     | 0xf0;
    *d++ = ((uv >> 12) & 0x3f) | 0x80;
    *d++ = ((uv >>  6) & 0x3f) | 0x80;
    *d++ = ( uv    & 0x3f) | 0x80;
  }
  else
  {
    // U+FFFD, replacement character
    *d++ = 0xef;
    *d++ = 0xbf;
    *d++ = 0xbd;
  }
}

void downgrade_string(char *out, const char *in, const unimap &conv)
{
  while (*in)
  {
    // let TELNET through unmolested
    if (*in == (char)255)
    {
      // Hack!  The only SB/SE we use is COMPRESS.
      int len = in[1] == (char)250 ? 5 : 3;
      for (int i=0; i<len; i++)
        if (*in) // end in a string inside a TELNET command?  Can't happen but...
          *out++ = *in++;
      continue;
    }

    int c = get_utf8(in);
    int r = conv[c];
    if (!r) // TODO: downgrade by dropping accents, etc
      r = '?';
    *out++ = r;
  }
  *out = 0;
}
