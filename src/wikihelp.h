#ifndef __WIKIHELP_H__
#define __WIKIHELP_H__

#define WIKIHELP_RESULTS_LIMIT 100

string wiki_help(string str);
string wiki_racial_stats(string str);
string wiki_classes(string str);
string wiki_specs(string str);
string wiki_innates(string str);
string wiki_races(string str);
string wiki_help_single(string str);

struct cmd_attrib_data {
  char *name;
  char *attributes;
};

void load_cmd_attributes();
char *attrib_help( char * );

#define ATT_STR	0
#define ATT_DEX	1
#define ATT_AGI	2
#define ATT_CON	3
#define ATT_POW	4
#define ATT_INT	5
#define ATT_WIS	6
#define ATT_CHA	7
#define ATT_KAR	8
#define ATT_LUK	9
#define ATT_MAX 10

#endif // __WIKIHELP_H__

