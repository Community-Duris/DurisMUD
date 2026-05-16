#include <vector>
#include <functional>

// Unicode replacement char
#define UNI_BAD 0xFFFD
#define IS_UTF8_TAIL(x) (((x) & 0xc0) == 0x80)

typedef unsigned short ushort;

class um128
{
private:
	ushort v[128];

public:
	um128() : v{} {};
	ushort  operator[](int c) const { return v[c & 127]; }
	ushort &operator[](int c) { return v[c & 127]; }
};

class unimap : private std::vector<um128>
{
public:
	unimap();
	unimap(const char16_t[256]);
	unimap(const char *);
	unimap(ushort r, const char *);
	ushort operator[](int c) const;
	void   set(int c, ushort v);
	void   foreach(std::function<void(int, ushort)> func) const;
	void operator+=(const unimap& other);
	void operator-=(const unimap& other);
};

int  get_utf8(const char *&s);
void put_utf8(char *&d, int v);
void downgrade_string(char *out, const char *in, const unimap &conv);

extern unimap u_cp437;
extern unimap u_ascii;

void upgrade_cp437_and_dollars(char *out, const char *in);
bool validate_utf8_and_dollars(char *out, const char *in);
