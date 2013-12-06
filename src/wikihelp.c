#include "sql.h"
#include "wikihelp.h"
#include "prototypes.h"
#include "string.h"
#include "specializations.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
using namespace std;

extern struct race_names race_names_table[];
extern struct class_names class_names_table[];
extern int class_table[LAST_RACE + 1][CLASS_COUNT + 1];
extern char *specdata[][MAX_SPEC];
extern const char *stat_to_string3(int);

void debug( const char *format, ... );

/* strip characters from front and back of string */
string trim(string const& str, char const* sep_chars)
{
  string::size_type const first = str.find_first_not_of(sep_chars);
  return ( first == string::npos ) ? string() : str.substr(first, str.find_last_not_of(sep_chars)-first+1);
}

/* replace a string with another string in a string */
string str_replace(string haystack_, const char* needle_, const char* replace_)
{
  string haystack(haystack_);
  string needle(needle_);
  string replace(replace_);
  string::size_type pos = haystack.find(needle);
  
  if( pos == string::npos )
    return haystack;
  
  haystack.replace( pos, needle.length(), replace );
  return str_replace(haystack, needle_, replace_);
}

/* clean up some of the wiki formatting */
string dewikify(string str_)
{
  string str(str_);
  str = str_replace(str, "[[", "&+c");
  str = str_replace(str, "]]", "&n");
  str = str_replace(str, "'''", "");
  return str;
}

string tolower(string str_)
{
  string str(str_);
#ifndef _OSX_
  transform( str.begin(), str.end(), str.begin(), (int(*)(int))tolower );
#endif
  return str;
}

#ifdef __NO_MYSQL__

string wiki_help(string str)
{
  return string("The help system is temporarily disabled.");  
}

#else

string wiki_help(string str)
{
  string return_str;
  
  // send the default help message
  if( str.length() < 1 )
  {
    return wiki_help_single(string("help"));    
  }

  // first, find the list of help topics that match the search string
  if( !qry("select title from pages where title like '%%%s%%' order by title asc limit %d", str.c_str(), WIKIHELP_RESULTS_LIMIT+1) )
  {
    return string("&+GSorry, but there was an error with the help system.");
  }

  MYSQL_RES *res = mysql_store_result(DB);
  MYSQL_ROW row;

  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return string("&+GSorry, but there are no help topics that match your search.");    
  }

  // if there is only one that matches, go ahead and display it
  if( mysql_num_rows(res) == 1 )
  {
    row = mysql_fetch_row(res); 
    return_str = row[0];
    mysql_free_result(res);
    return wiki_help_single(return_str);    
  }  
  
  // scan through the results to see if the search string matches one exactly.
  // if so, display that one first and then the rest as "see also" links
  vector<string> matching_topics;

  bool exact_match = false;
  while( row = mysql_fetch_row(res) )
  {
    string match(row[0]);
    if( tolower(match) == tolower(str) )
    {
      exact_match = true;
    }
    else
    {
      matching_topics.push_back(match);
    }
  }
  
  mysql_free_result(res);

  if( exact_match )
  {
    return_str += wiki_help_single(str);    
    return_str += "\n\n&+GThe following help topics also matched your search:\n";
  }
  else
  {
    return_str += "&+GThe following help topics matched your search:\n";
  }
  
  // list the topics
  for( int i = 0; i < matching_topics.size(); i++ )
  {
    return_str += " &+c";
    return_str += matching_topics[i];
    return_str += "\n";
  }
  
  return return_str;  
}

// display racial stats for a race category help file
string wiki_racial_stats(string title)
{
  string return_str, race_str;
  char race[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH] = "";
  int i;

  for (i = 0; i <= RACE_PLAYER_MAX; i++)
  {
    if (!strcmp(tolower(race_names_table[i].normal).c_str(), tolower(title).c_str()))
    { 
      race_str += race_names_table[i].no_spaces;
      break;
    }
  }

  return_str += "==Racial Statistics==\n";
  
  if (i > RACE_PLAYER_MAX)
  {
    return_str += "No data found for ";
    return_str += title;
    return_str += "&n\n";
    return return_str;
  }
  
  return_str += "Strength    : &+c";
  sprintf(race, "stats.str.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Agility     : &+c";
  sprintf(race, "stats.agi.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Dexterity   : &+c";
  sprintf(race, "stats.dex.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Constitution: &+c";
  sprintf(race, "stats.con.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Power       : &+c";
  sprintf(race, "stats.pow.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Intelligence: &+c";
  sprintf(race, "stats.int.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Wisdom      : &+c";
  sprintf(race, "stats.wis.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Charisma    : &+c";
  sprintf(race, "stats.cha.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Luck        : &+c";
  sprintf(race, "stats.luc.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Karma       : &+c";
  sprintf(race, "stats.kar.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  sprintf(buf, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "\n==Racial Traits==\n";
  // Removing damage output because this is more of a fine tuning function
  // for imm's, and will only confuse players.
  //return_str += "Damage Output: &+c";
  //sprintf(race, "damage.totalOutput.racial.%s", race_str.c_str());
  //return_str += stat_to_string_spell_pulse(get_property(race, 1.000));
  //return_str += "&n\n";
  return_str += "Combat Pulse : &+c";
  sprintf(race, "damage.pulse.racial.%s", race_str.c_str());
  return_str += stat_to_string_damage_pulse(get_property(race, 14.000));
  return_str += "&n\n";
  return_str += "Spell Pulse  : &+c";
  sprintf(race, "spellcast.pulse.racial.%s", race_str.c_str());
  return_str += stat_to_string_spell_pulse(get_property(race, 1.000));
  return_str += "&n\n";
  return return_str;
}

// Display Classes and specs based on whats allowed in the code.
string wiki_classes(string title)
{
  string return_str;
  int i, found = 0;

  for (i = 0; i <= RACE_PLAYER_MAX; i++)
  {
    if (!strcmp(tolower(race_names_table[i].normal).c_str(), tolower(title).c_str()))
    { 
      break;
    }
  }

  return_str += "==Class list==\n";

  if (i > RACE_PLAYER_MAX)
  {
    return_str += "No data found for ";
    return_str += title;
    return_str += "&n\n";
    return return_str;
  }
  
  for (int cls = 1; cls <= CLASS_COUNT; cls++)
  {
    if (class_table[i][cls] != 5)
    {
      found = 1;
      return_str += "*";
      return_str += pad_ansi(class_names_table[cls].ansi, 12);
      return_str += "&n: ";
      return_str += single_spec_list(i, cls);
      return_str += "\n";
    }
  }
  if (!found)
    return_str += "No classes available.\n";

  return return_str;
}

string wiki_specs(string title)
{
  string return_str;
  int i, j, found = 0;

  for (i = 0; i <= CLASS_COUNT; i++)
  {
    if (!strcmp(tolower(class_names_table[i].normal).c_str(), tolower(title).c_str()))
    {
      break;
    }
  }

  return_str += "==Specializations==\n";
  
  if (i > CLASS_COUNT)
  {
    return_str += "No data found for ";
    return_str += title;
    return_str += "&n\n";
    return return_str;
  }

  for (j = 0; j < MAX_SPEC; j++)
  {
    if (!strcmp(specdata[i][j], "") ||
	!strcmp(specdata[i][j], "Not Used"))
      continue;
    found = 1;
    return_str += string(specdata[i][j]);
    return_str += "\n";
  }
  
  if (!found)
    return_str += "No specializations found.\n";

  return return_str;
}

string wiki_innates(string title)
{
  string return_str;
  int i, j, innate, found = 0;

  for (i = 0; i <= RACE_PLAYER_MAX; i++)
  {
    if (!strcmp(tolower(race_names_table[i].normal).c_str(), tolower(title).c_str()))
    { 
      found = 1;
      break;
    }
  }

  if (!found)
  {
    for (i = 0; i <= CLASS_COUNT; i++)
    {
      if (!strcmp(tolower(class_names_table[i].normal).c_str(), tolower(title).c_str()))
      {
	found = 2;
	break;
      }
    }
  }

  j = 0;

  if (!found)
  {
    for (i = 0; i <= CLASS_COUNT; i++)
    {
      for (j = 0; j < MAX_SPEC; j++)
      {
        if (!strcmp(tolower(strip_ansi(specdata[i][j])).c_str(), tolower(title).c_str()))
	{
	  found = 2;
	  break;
	}
      }
      if (found == 2)
      {
	j++;
	break;
      }
    }
  }
  
  return_str += "==Innate abilities==\n";
  
  if (!found)
  {
    return_str += "No entries found.\n";
    return return_str;
  }
  
  return_str += list_innates(((found == 1) ? i : 0), ((found == 2) ? i : 0), j);

  return return_str;
}

string wiki_races(string title)
{
  string return_str;
  int cls, race, found = 0;

  for (cls = 0; cls <= CLASS_COUNT; cls++)
  {
    if (!strcmp(tolower(class_names_table[cls].normal).c_str(), tolower(title).c_str()))
    {
      break;
    }
  }

  return_str += "==Allowed races==\n";
  
  if (cls > CLASS_COUNT)
  {
    return_str += "No data found for ";
    return_str += title;
    return_str += "&n\n";
    return return_str;
  }

  for (race = 1; race <= RACE_PLAYER_MAX; race++)
  {
    if (class_table[race][cls] == 5)
      continue;
    
    found = 1;
    return_str += "*";
    return_str += string(race_names_table[race].ansi);
    return_str += "&n\n";
  }

  if (!found)
  {
    return_str += "None.\n";
    return return_str;
  }
  return return_str;
}

// display a single help topic
string wiki_help_single(string str)
{
  string return_str, title;
  int category;

  if( !qry("select title, text, category_id from pages where title = '%s' limit 1", str.c_str()) )
  {
    return string("&+GSorry, but there was an error with the help system.");
  }

  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return string("&+GHelp topic not found.");
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);
  
  return_str += "&+c";
  return_str += row[0];
  return_str += "\n";
  
  return_str += "&+L=========================================\n";
  
  return_str += dewikify(trim(string(row[1]), " \t\n"));
  
  // Race type help files
  if (atoi(row[2]) == 25)
  {
    title += row[0];
    return_str += wiki_classes(title);
    return_str += "\n";
    return_str += wiki_racial_stats(title);
    return_str += "\n";
    return_str += wiki_innates(title);
  }
  // Class type help files
  else if (atoi(row[2]) == 9)
  {
    title += row[0];
    return_str += wiki_races(title);
    return_str += "\n";
    return_str += wiki_innates(title);
    return_str += "\n";
    return_str += wiki_specs(title);
  }
  // Spec type help files
  // Going to leave spec help files alone.
  else if (atoi(row[2]) == 16)
  {
  }

  mysql_free_result(res);
  
  return return_str;
}

#endif

struct cmd_attrib_data cmd_attribs [400];

void load_cmd_attributes()
{
  FILE *cmd_file;
  char  line[ MAX_STRING_LENGTH ];
  char  attributes[ MAX_STRING_LENGTH ];
  int   count = 0, i = 0;
  bool  ch_attributes[ ATT_MAX];
  bool  vi_attributes[ ATT_MAX ];

  for( count = 0; count < 400; count++ )
  {
    cmd_attribs[count].name = cmd_attribs[count].attributes = NULL;
  }

  cmd_file = fopen( "command_attributes.txt", "r" );
  if( !cmd_file )
  {
    logit( LOG_DEBUG, "Could not open command_attributes.txt." );
    return;
  }

  count = 0;
  while( fgets( line, sizeof line, cmd_file ) != NULL )
  {
    // First line is the name.
    cmd_attribs[count].name = strdup( line );
    // Set all attributes to false
    for(i = 0; i < ATT_MAX;i++)
    {
      ch_attributes[i] = FALSE;
      vi_attributes[i] = FALSE;
    }
    // Following lines are attributes until '~' is encountered.
    fgets( line, sizeof line, cmd_file);
    while( line[0] != '~' )
    {
      // If GET_C_STR
      if( line[6] == 'S' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_STR] = TRUE;
        else
          ch_attributes[ATT_STR] = TRUE;
      }
      else if( line[6] == 'D' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_DEX] = TRUE;
        else
          ch_attributes[ATT_DEX] = TRUE;
      }
      else if( line[6] == 'A' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_AGI] = TRUE;
        else
          ch_attributes[ATT_AGI] = TRUE;
      }
      else if( line[6] == 'C' && line[7] == 'O' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_CON] = TRUE;
        else
          ch_attributes[ATT_CON] = TRUE;
      }
      else if( line[6] == 'P' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_POW] = TRUE;
        else
          ch_attributes[ATT_POW] = TRUE;
      }
      else if( line[6] == 'I' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_INT] = TRUE;
        else
          ch_attributes[ATT_INT] = TRUE;
      }
      else if( line[6] == 'W' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_WIS] = TRUE;
        else
          ch_attributes[ATT_WIS] = TRUE;
      }
      else if( line[6] == 'C' && line[7] == 'H' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_CHA] = TRUE;
        else
          ch_attributes[ATT_CHA] = TRUE;
      }
      else if( line[6] == 'K' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_KAR] = TRUE;
        else
          ch_attributes[ATT_KAR] = TRUE;
      }
      else if( line[6] == 'L' )
      {
        if( line[10] == 'v' )
          vi_attributes[ATT_LUK] = TRUE;
        else
          ch_attributes[ATT_LUK] = TRUE;
      }
      else
      {
        logit( LOG_DEBUG, "Bad line in command_attributes.txt: " );
        logit( LOG_DEBUG, line );
      }
      fgets( line, sizeof line, cmd_file );
    }
    // create the list of attributes.
    attributes[0] = '\0';
    if( ch_attributes[ATT_STR] )
       strcat( attributes, "Char's Str.\n" );
    if( vi_attributes[ATT_STR] )
       strcat( attributes, "Victim's Str.\n" );
    if( ch_attributes[ATT_DEX] )
       strcat( attributes, "Char's Dex.\n" );
    if( vi_attributes[ATT_DEX] )
       strcat( attributes, "Victim's Dex.\n" );
    if( ch_attributes[ATT_AGI] )
       strcat( attributes, "Char's Agi.\n" );
    if( vi_attributes[ATT_AGI] )
       strcat( attributes, "Victim's Agi.\n" );
    if( ch_attributes[ATT_CON] )
       strcat( attributes, "Char's Con.\n" );
    if( vi_attributes[ATT_CON] )
       strcat( attributes, "Victim's Con.\n" );
    if( ch_attributes[ATT_POW] )
       strcat( attributes, "Char's Pow.\n" );
    if( vi_attributes[ATT_POW] )
       strcat( attributes, "Victim's Pow.\n" );
    if( ch_attributes[ATT_INT] )
       strcat( attributes, "Char's Int.\n" );
    if( vi_attributes[ATT_INT] )
       strcat( attributes, "Victim's Int.\n" );
    if( ch_attributes[ATT_WIS] )
       strcat( attributes, "Char's Wis.\n" );
    if( vi_attributes[ATT_WIS] )
       strcat( attributes, "Victim's Wis.\n" );
    if( ch_attributes[ATT_CHA] )
       strcat( attributes, "Char's Cha.\n" );
    if( vi_attributes[ATT_CHA] )
       strcat( attributes, "Victim's Cha.\n" );
    if( ch_attributes[ATT_KAR] )
       strcat( attributes, "Char's Kar.\n" );
    if( vi_attributes[ATT_KAR] )
       strcat( attributes, "Victim's Kar.\n" );
    if( ch_attributes[ATT_STR] )
       strcat( attributes, "Char's Luk.\n" );
    if( vi_attributes[ATT_STR] )
       strcat( attributes, "Victim's Luk.\n" );

    // add list to the array.
    cmd_attribs[count++].attributes = strdup( attributes );
  }
}

char *attrib_help( char *arg)
{
  int count = 0;

logit( LOG_DEBUG, "The search string is '%s'.", arg );

  while( cmd_attribs[count].name != NULL )
  {
    if( strstr( cmd_attribs[count].name, arg ) )
      return cmd_attribs[count].attributes;
    count++;
  }

  return NULL;
}
