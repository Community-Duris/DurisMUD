#include <string.h>

#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "prototypes.h"
#include "sql.h"
#include "multiplay_whitelist.h"

extern P_desc descriptor_list;

bool whitelisted_host(const char *host)
{
  vector<whitelist_data> whitelist = get_whitelist();

  for( vector<whitelist_data>::iterator it = whitelist.begin(); it != whitelist.end(); it++ )
  {
    if( match_pattern(it->pattern.c_str(), host) )
      return true;    
  }
  
  return false;
}

vector<whitelist_data> get_whitelist()
{
  vector<whitelist_data> whitelist;

#ifdef __NO_MYSQL__
  return whitelist;
#else  
  if( !qry("SELECT id, created_on, pattern, player, admin, description FROM %s", MULTIPLAY_WHITELIST_TABLE_NAME) )
  {
    logit(LOG_DEBUG, "get_whitelist(): qry failed");
    return whitelist;
  }
    
  MYSQL_RES *res = mysql_store_result(DB);
  while( MYSQL_ROW row = mysql_fetch_row(res) )
  {
    whitelist.push_back(whitelist_data(atoi(row[0]), string(row[1]), string(row[2]), string(row[3]), string(row[4]), string(row[5])));    
  }
  
  mysql_free_result(res);
  
  return whitelist;
#endif
}

bool add_to_whitelist(P_char ch, const char *player, const char *pattern, const char *description)
{
#ifdef __NO_MYSQL__
  return false;
#else  
  char descbuff[MAX_STRING_LENGTH];
  
  mysql_real_escape_string(DB, descbuff, description, strlen(description));
  
  if( !qry("INSERT INTO %s (created_on, admin, player, pattern, description) VALUES (now(), trim('%s'), trim('%s'), trim('%s'), trim('%s'))", MULTIPLAY_WHITELIST_TABLE_NAME, GET_NAME(ch), player, pattern, descbuff) )
  {
    logit(LOG_DEBUG, "add_to_whitelist(): qry failed");
    return false;
  }

  sql_log(ch, WIZLOG, "Added '%s' (%s: %s) to multiplay whitelist", pattern, player, description);  
  return true;  
#endif  
}

bool remove_from_whitelist(P_char ch, const char *pattern)
{
#ifdef __NO_MYSQL__
  return false;
#else  
  if( !qry("DELETE FROM %s WHERE pattern = trim('%s')", MULTIPLAY_WHITELIST_TABLE_NAME, pattern) )
  {
    logit(LOG_DEBUG, "remove_from_whitelist(): qry failed");
    return false;
  }
  
  sql_log(ch, WIZLOG, "Removed '%s' from multiplay whitelist", pattern);  
  return true;  
#endif  
}

void do_whitelist_help(P_char ch)
{
  send_to_char("usage: whitelist\r\n"
               "       whitelist add <player> <*.*.*.* ip address pattern> <description>\r\n"
               "       whitelist remove <ip address pattern>\r\n\r\n", ch);
  
}

bool is_connected(const char *pattern)
{
  for (P_desc k = descriptor_list; k; k = k->next)
  {
    if( match_pattern(pattern, k->host) )
    {
      return true;
    }
  }
  return false;
}

void do_whitelist(P_char ch, char *argument, int cmd)
{
  P_char   victim;
  char     argbuf[MAX_STRING_LENGTH], linebuf[MAX_STRING_LENGTH];
  
  
  if(IS_NPC(ch) || !IS_TRUSTED(ch))
    return;
    
  argument = one_argument(argument, argbuf);
  
  if (!*argbuf)
  {
    // no arguments: list existing whitelist
    vector<whitelist_data> whitelist = get_whitelist();
    
    send_to_char(" Host pattern    | Added by     | On         | Player       | Description\r\n"
                 "-----------------------------------------------------------------------------\r\n", ch);
//               " 127.345.234.113 | Torgal       | 2009-12-22 | Zion         | Contacted by email, brothers who play from same net"

    for (vector<whitelist_data>::iterator it = whitelist.begin(); it != whitelist.end(); it++ )
    {
      sprintf(linebuf, " %s%s | %s | %s | %s | %s&n\r\n",
              (is_connected(it->pattern.c_str()) ? string("&+R").c_str() : ""),
              pad_ansi(it->pattern.c_str(), 15).c_str(),
              pad_ansi(it->admin.c_str(), 12).c_str(),
              pad_ansi(it->created_on.c_str(), 10).c_str(),
              pad_ansi(it->player.c_str(), 12).c_str(),
              it->description.c_str());
      
      send_to_char(linebuf, ch); 
    }
    
    return;
  }
  else if ( !strcmp(argbuf, "add") )
  {
    // argument should be: <player> <ip pattern> <description>
    char player[MAX_STRING_LENGTH];
    char pattern[MAX_STRING_LENGTH];
    
    argument = one_argument(argument, player);
    
    if (!(*player))
    {
      send_to_char("Missing player name\r\n", ch);
      do_whitelist_help(ch);
      return;
    }
    
    argument = one_argument(argument, pattern);
    
    if (!(*pattern) || !match_pattern("*.*.*.*", pattern))
    {
      send_to_char("Invalid host pattern.\r\n", ch);
      do_whitelist_help(ch);
      return;
    }
    
    if (!(*argument))
    {
      send_to_char("Missing description.\r\n", ch);
      do_whitelist_help(ch);
      return;
    }
    
    if( add_to_whitelist(ch, player, pattern, argument) )
    {
      send_to_char("Host pattern added to multiplay whitelist.\r\n", ch);
    }
    else
    {
      send_to_char("ERROR: host pattern could not be added to multiplay whitelist.\r\n", ch);
    }
    
    return;    
  }
  else if ( !strcmp(argbuf, "remove") )
  {
    // argument should be: <*.*.*.* pattern>

    if (!match_pattern("*.*.*.*", argument))
    {
      send_to_char("Invalid host pattern.\r\n", ch);
      do_whitelist_help(ch);
      return;
    }
    
    if( remove_from_whitelist(ch, argument) )
    {
      send_to_char("Host pattern removed from multiplay whitelist.\r\n", ch);
    }
    else
    {
      send_to_char("Host pattern not found or could not be removed from multiplay whitelist.\r\n", ch);
    }
    
    return;    
  }
  else
  {
    do_whitelist_help(ch);
    return;
  }
}
