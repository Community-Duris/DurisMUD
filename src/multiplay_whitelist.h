#ifndef __MULTIPLAY_WHITELIST_H__
#define __MULTIPLAY_WHITELIST_H__

#include <vector>
#include <string>
using namespace std;

#define MULTIPLAY_WHITELIST_TABLE_NAME "multiplay_whitelist"

struct whitelist_data 
{
  whitelist_data(int _id, string _created_on, string _pattern, string _player, string _admin, string _description) : 
    id(_id), created_on(_created_on), pattern(_pattern), player(_player), admin(_admin), description(_description) {}

  int id;
  string created_on;
  string pattern;
  string player;
  string admin;
  string description;
};

void do_whitelist(P_char, char *, int);
bool whitelisted_host(const char *host);
vector<whitelist_data> get_whitelist();

#endif // __MULTIPLAY_WHITELIST_H__
