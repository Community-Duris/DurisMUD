#include "unicode.h"
#include <stdlib.h>
#include <string.h>

unimap::unimap() { resize(1); }

ushort unimap::operator[](int c) const
{
	if (c < 0 || c >= (1 << 21))
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
	if (c < 0 || c >= (1 << 21))
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

	for (int c = 0; c < 256; c++)
		set(conv[c], c);
}

#define CP437_PRINTABLE                                                                                                                                                                                \
	" !\"#$%&'()*+,-./0123456789:;<=>?"                                                                                                                                                                \
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"                                                                                                                                                                \
	"`abcdefghijklmnopqrstuvwxyz{|}~⌂"                                                                                                                                                                 \
	"ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒ"                                                                                                                                                                 \
	"áíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"                                                                                                                                                                 \
	"└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"                                                                                                                                                                 \
	"αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ "

// superfluous entry because dumb ISO thinks all char arrays are strings
const char16_t cp437_u[257] = u" ☺☻♥♦♣♠•◘○◙♂♀♪♫☼▶◀↕‼¶§▬↨↑↓→←∟↔▲▼" CP437_PRINTABLE;

// Control characters map to controls or ' '.
unimap u_cp437(u"       \a\b\t\n  \r             \e    " CP437_PRINTABLE);

unimap ascii("  " // U+00A0 = &nbsp;
             "!¡ c¢ L£ $¤ Y¥ |¦ $§ \"¨ <« !¬ -­ -¯ 2² 3³ '´ uµ $¶ .· ,¸ 1¹ >» ?¿ AÀ AÁ AÂ AÃ"
             "AÄ AÅ AÆ CÇ EÈ EÉ EÊ EË IÌ IÍ IÎ IÏ DÐ NÑ OÒ OÓ OÔ OÕ OÖ *× OØ UÙ UÚ UÛ UÜ YÝ"
             "TÞ sß aà aá aâ aã aä aå aæ cç eè eé eê eë iì ií iî iï dð nñ oò oó oô oõ oö /÷"
             "oø uù uú uû uü yý tþ yÿ AĀ"
             "aā AĂ aă AĄ aą CĆ cć CĈ cĉ CĊ cċ CČ cč DĎ dď DĐ dđ EĒ eē EĔ eĕ EĖ eė EĘ eę EĚ"
             "eě GĜ gĝ GĞ gğ GĠ gġ GĢ gģ HĤ hĥ HĦ hħ IĨ iĩ IĪ iī IĬ iĭ IĮ iį Iİ iı JĴ jĵ KĶ"
             "kķ kĸ KĹ lĺ LĻ lļ LĽ lľ LĿ lŀ LŁ lł NŃ nń NŅ nņ NŇ nň nŉ NŊ nŋ OŌ oō OŎ oŏ OŐ"
             "oő OŒ oœ RŔ rŕ RŖ rŗ RŘ rř SŚ sś SŜ sŝ SŞ sş SŠ sš TŢ tţ TŤ tť TŦ tŧ UŨ uũ UŪ"
             "uū UŬ uŭ UŮ uů UŰ uű UŲ uų WŴ wŵ YŶ yŷ YŸ ZŹ zź ZŻ zż ZŽ zž sſ bƀ BƁ BƂ bƃ 6Ƅ"
             "6ƅ OƆ CƇ Cƈ DƉ DƊ DƋ dƌ dƍ EƎ EƏ EƐ FƑ fƒ GƓ GƔ hƕ iƖ IƗ KƘ kƙ lƚ lƛ MƜ NƝ nƞ"
             "OƟ OƠ oơ OƢ oƣ PƤ pƥ kƦ SƧ sƨ EƩ eƪ tƫ TƬ tƭ TƮ UƯ uư UƱ VƲ YƳ yƴ ZƵ zƶ EƷ eƸ"
             "eƹ eƺ 2ƻ 5Ƽ 5ƽ sƾ pƿ AǍ aǎ IǏ iǐ OǑ oǒ UǓ uǔ UǕ uǖ UǗ uǘ UǙ uǚ UǛ uǜ eǝ AǞ aǟ"
             "AǠ aǡ AǢ aǣ GǤ gǥ GǦ gǧ KǨ kǩ OǪ oǫ OǬ oǭ EǮ eǯ jǰ GǴ gǵ HǶ PǷ NǸ nǹ AǺ aǻ AǼ"
             "aǽ OǾ oǿ AȀ"
             "aȁ AȂ aȃ EȄ eȅ EȆ eȇ IȈ iȉ IȊ iȋ OȌ oȍ OȎ oȏ RȐ rȑ RȒ rȓ UȔ uȕ UȖ uȗ SȘ sș TȚ"
             "tț 3Ȝ 3ȝ HȞ hȟ NȠ dȡ ZȤ zȥ AȦ aȧ EȨ eȩ OȪ oȫ OȬ oȭ OȮ oȯ OȰ oȱ YȲ yȳ lȴ nȵ tȶ"
             "jȷ AȺ CȻ cȼ LȽ TȾ sȿ zɀ aɐ aɑ aɒ bɓ oɔ cɕ dɖ dɗ eɘ eə eɚ eɛ eɜ eɝ eɞ jɟ gɠ gɡ"
             "Gɢ gɣ hɥ hɦ hɧ iɨ iɩ Iɪ lɫ lɬ lɭ mɯ mɰ mɱ nɲ nɳ Nɴ oɵ rɹ rɺ rɻ rɼ rɽ rɾ rɿ Rʀ"
             "Rʁ sʂ sʃ sʄ sʅ sʆ tʇ tʈ uʉ uʊ vʋ vʌ wʍ yʎ Yʏ zʐ zʑ 3ʒ 3ʓ ?ʔ Cʗ Bʙ eʚ gʛ Hʜ jʝ"
             "kʞ Lʟ qʠ ?ʡ hʮ hʯ hʰ hʱ jʲ rʳ rʴ rʵ Rʶ wʷ yʸ 'ʹ \"ʺ `ʻ 'ʼ `ʽ 'ʾ `ʿ <˂ >˃ ^˄ v˅"
             "^ˆ vˇ 'ˊ `ˋ ,ˏ +˖ -˗ ~˜ \"˝ `˞ '˟ \"ˮ -‐"
             "-‑ -‒ -– -— -― |‖ _‗ `‘ '’ ,‚ `‛ \"“ \"” \"„ \"‟ +† $‡ o• +‣ .․ :‥ .… -‧ '′ \"″ `‵"
             "\"‶ ^‸ <‹ >› ?‽ _‿ *⁂ -⁃ /⁄ [⁅ ]⁆ <⁌ >⁍ *⁎ ;⁏ *⁕ F₣ L₤ W₩ e€ p₱ hℎ"
             "KK AÅ e℮ <← ^↑ >→ v↓ -−"
             "/∕ \\∖ *∗ |∣ ~∼ ~∽ ~≈ ^⌃"
             "v⌄ [⌊ ]⌋ -─"
             "-━ |│ |┃ -┄ -┅ |┆ |┇ -┈ -┉ |┊ |┋ +┌ +┍ +┎ +┏ +┐ +┑ +┒ +┓ +└ +┕ +┖ +┗ +┘ +┙ +┚"
             "+┛ +├ +┝ +┞ +┟ +┠ +┡ +┢ +┣ +┤ +┥ +┦ +┧ +┨ +┩ +┪ +┫ +┬ +┭ +┮ +┯ +┰ +┱ +┲ +┳ +┴"
             "+┵ +┶ +┷ +┸ +┹ +┺ +┻ +┼ +┽ +┾ +┿ +╀ +╁ +╂ +╃ +╄ +╅ +╆ +╇ +╈ +╉ +╊ +╋ -╌ -╍ |╎"
             "|╏ =═ |║ +╒ +╓ +╔ +╕ +╖ +╗ +╘ +╙ +╚ +╛ +╜ +╝ +╞ +╟ +╠ +╡ +╢ +╣ +╤ +╥ +╦ +╧ +╨"
             "+╩ +╪ +╫ +╬ .╭ .╮ '╯ `╰ /╱ \\╲ X╳ -╴ '╵ -╶ .╷ -╸ '╹ -╺ .╻ -╼ |╽ -╾ |╿ #█ .░ X▒"
             "#▓ *▪ *▫ ^▲ ^△ ^▴ ^▵ >▶ >▷ >▸ >▹ >► >▻ V▼ V▽ v▾ v▿ <◀ <◁ <◂ <◃ <◄ <◅ o○ O◯ ");

unimap::unimap(const char *in)
{
	resize(1);

	while (*in)
	{
		unsigned char i = *in++;
		int           o = get_utf8(in);
		if (!o)
			break;
		set(o, i);

		while (*in == ' ') // spaces for readability
			in++;
	}
}

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
	if ((c & 0xe0) == 0xc0)
		len = 1, c &= 0x1f, min = 0x80;
	else if ((c & 0xf0) == 0xe0)
		len = 2, c &= 0x0f, min = 0x800;
	else if ((c & 0xf8) == 0xf0)
		len = 3, c &= 0x07, min = 0x10000;
#if 0
  else if ((c&0xfc)==0xf8)
    len=4, c&=0x03, min=0x200000;
  else if ((c&0xfe)==0xfc)
    len=5, c&=0x01, min=0x4000000;
#endif
	else
		goto eat_junk;
	while ((*str & 0xc0) == 0x80)
		c = (c << 6) + (*str++ & 0x3f), len--;
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
		*d++ = (uv >> 6) | 0xc0;
		*d++ = (uv & 0x3f) | 0x80;
	}
	else if (uv < 0x10000)
	{
		*d++ = (uv >> 12) | 0xe0;
		*d++ = ((uv >> 6) & 0x3f) | 0x80;
		*d++ = (uv & 0x3f) | 0x80;
	}
	else if (uv < 0x110000)
	{
		*d++ = (uv >> 18) | 0xf0;
		*d++ = ((uv >> 12) & 0x3f) | 0x80;
		*d++ = ((uv >> 6) & 0x3f) | 0x80;
		*d++ = (uv & 0x3f) | 0x80;
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
		int c = get_utf8(in);
		int r = conv[c];
		if (!r)
		{
			r = ascii[c]; // downgrade by dropping accents, etc
			if (!r)
				r = '?';
		}
		*out++ = r;
	}
	*out = 0;
}

void upgrade_cp437_and_dollars(char *out, const char *in)
{
	while (*in)
	{
		int c = (unsigned char)*in++;
		if (c < ' ') // controls are illegal, including \n
			continue;
		if (c == '$')
			*out++ = '$';
		put_utf8(out, cp437_u[c]);
	}
	*out = 0;
}

bool validate_utf8_and_dollars(char *out, const char *in)
{
	bool err = false;

	while (*in)
	{
		int c = get_utf8(in);
		if (c < ' ') // controls are illegal, including \n
			continue;
		if (c == '$')
			*out++ = '$';
		if (c < 127 || ascii[c]) // rule: anything we can downgrade is allowed
			put_utf8(out, c);
		else
			err = true; // complain
	}
	*out = 0;

	return err;
}
