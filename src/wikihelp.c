#include "sql.h"
#include "utils.h"
#include "utility.h"
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
extern int allowed_secondary_classes[][5];
extern const mcname multiclass_names[];

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
  // Arih: Security fix - Escape user input to prevent SQL injection.
  // Using escape_str() wraps mysql_real_escape_string() to sanitize special chars like quotes.
  if( !qry("select title from pages where title like '%%%s%%' order by title asc limit %d", escape_str(str.c_str()).c_str(), WIKIHELP_RESULTS_LIMIT+1) )
  {
    return string("&+GSorry, but there was an error with the help system.");
  }

  MYSQL_RES *res = mysql_store_result(DB);
  MYSQL_ROW row;

  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    // Log bad help file requests.
    logit( LOG_HELP, str.c_str() );
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
  while ((row = mysql_fetch_row(res)))
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

  if( i > RACE_PLAYER_MAX )
  {
    return_str += "No data found for race '";
    return_str += title;
    return_str += "&n'\n";
    return return_str;
  }

  return_str += "Strength    : &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.str.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Agility     : &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.agi.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Dexterity   : &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.dex.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Constitution: &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.con.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Power       : &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.pow.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Intelligence: &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.int.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Wisdom      : &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.wis.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Charisma    : &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.cha.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Luck        : &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.luc.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "Karma       : &+c";
  snprintf(race, MAX_STRING_LENGTH, "stats.kar.%s", race_str.c_str());
  //return_str += stat_to_string3((int)get_property(race, 100));
  snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
  return_str += buf;
  return_str += "&n\n";
  return_str += "\n==Racial Traits==\n";
  // Removing damage output because this is more of a fine tuning function
  // for imm's, and will only confuse players.
  //return_str += "Damage Output: &+c";
  //snprintf(race, MAX_STRING_LENGTH, "damage.totalOutput.racial.%s", race_str.c_str());
  //return_str += stat_to_string_spell_pulse(get_property(race, 1.000));
  //return_str += "&n\n";
  return_str += "Combat Pulse : &+c";
  snprintf(race, MAX_STRING_LENGTH, "damage.pulse.racial.%s", race_str.c_str());
  return_str += stat_to_string_damage_pulse(get_property(race, 14.000));
  return_str += "&n\n";
  return_str += "Spell Pulse  : &+c";
  snprintf(race, MAX_STRING_LENGTH, "spellcast.pulse.racial.%s", race_str.c_str());
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
    return_str += "No data found for race '";
    return_str += title;
    return_str += "&n'\n";
    return return_str;
  }

  for (int cls = 1; cls <= CLASS_COUNT; cls++)
  {
    if (class_table[i][cls] != 5)
    {
      found = 1;
      return_str += "* ";
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

  for( i = 0; i <= CLASS_COUNT; i++ )
  {
    if( !strcmp(tolower(class_names_table[i].normal).c_str(), tolower(title).c_str()) )
    {
      break;
    }
  }

  return_str += "==Specializations==\n";

  if( i > CLASS_COUNT )
  {
    return_str += "No data found for class '";
    return_str += title;
    return_str += "'&n\n";
    return return_str;
  }

  for (j = 0; j < MAX_SPEC; j++)
  {
    if( !strcmp(specdata[i][j], "") || !strcmp(specdata[i][j], "Not Used") )
    {
      continue;
    }
    found = TRUE;
    return_str += "* ";
    return_str += string(specdata[i][j]);
    return_str += "\n";
  }

  if (!found)
  {
    return_str += "No specializations found.\n";
  }

  return return_str;
}

string wiki_innates(string title, int type)
{
  string return_str;
  int i, j = 0, innate, found = 0;

  if( type == WIKI_RACE )
  {
    for (i = 0; i <= RACE_PLAYER_MAX; i++)
    {
      if (!strcmp(tolower(race_names_table[i].normal).c_str(), tolower(title).c_str()))
      {
        found = 1;
        break;
      }
    }
  }

  if( type == WIKI_CLASS )
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

  if( type == WIKI_SPEC )
  {
    // Skip "CLASS_NONE"
    for( i = 1; i <= CLASS_COUNT; i++ )
    {
      for( j = 0; j < MAX_SPEC; j++ )
      {
        if (!strcmp(tolower(strip_ansi(specdata[i][j])).c_str(), tolower(title).c_str()))
        {
          found = 2;
          break;
        }
      }
      if( found == 2 )
      {
        // Specs range from 1-4 not 0-3.
        j++;
        break;
      }
    }
  }

  return_str += "==Innate abilities==\n";

  if( !found )
  {
    return_str += "No entries found.\n";
    return return_str;
  }

  return_str += list_innates(((found == 1) ? i : 0), ((found == 2) ? i : 0), j);

  return return_str;
}

string wiki_races(string title, int type )
{
  string return_str;
  int cls, spec, race;
  bool found;

  if( type == WIKI_CLASS )
  {
    spec = 0;
    // Find class to search for.
    for( cls = 0; cls <= CLASS_COUNT; cls++ )
    {
      if (!strcmp(tolower(class_names_table[cls].normal).c_str(), tolower(title).c_str()))
      {
        break;
      }
    }
  }
  else if( type == WIKI_SPEC )
  {
    found = FALSE;
    for( cls = 0; cls <= CLASS_COUNT; cls++ )
    {
      for( spec = 0; spec < MAX_SPEC; spec++ )
      {
        if( !strcmp(tolower(strip_ansi(specdata[cls][spec])).c_str(), tolower(title).c_str()) )
        {
          found = TRUE;
          break;
        }
      }
      if( found == TRUE )
      {
        // Specs range from 1-4 not 0-3.
        spec++;
        break;
      }
    }
  }

  return_str += "==Allowed races==\n";

  if( cls > CLASS_COUNT )
  {
    if( type == WIKI_CLASS )
    {
      return_str += "No data found for class '";
    }
    else if( type == WIKI_SPEC )
    {
      return_str += "No data found for spec '";
    }
    else
    {
      return_str += "Unknown type.  Plz report to a God.\n";
      return return_str;
    }
    return_str += title;
    return_str += "&n'\n";
    return return_str;
  }

  found = FALSE;
  for( race = 1; race <= RACE_PLAYER_MAX; race++ )
  {
    // Class not allowed for race.
    if( class_table[race][cls] == 5 )
    {
      continue;
    }
    // Spec not allowed for race.
    if( type == WIKI_SPEC && !is_allowed_race_spec(race, 1 << (cls - 1), spec) )
    {
      continue;
    }
    if( !found )
    {
      return_str += "&+W*&n ";
      found = TRUE;
    }
    else
    {
      return_str += ", ";
    }
    return_str += string(race_names_table[race].ansi);
    return_str += "&n";
  }

  if( !found )
  {
    return_str += "None.\n";
    return return_str;
  }

  return_str += "\n";
  return return_str;
}

// Display a single help topic
string wiki_help_single(string str)
{
  string return_str, title;
  int category, dashes;

  // Arih: Security fix - Escape user input to prevent SQL injection.
  // Using escape_str() wraps mysql_real_escape_string() to sanitize special chars like quotes.
  if( !qry("select title, text, category_id, last_update, last_update_by from pages where title = '%s' limit 1", escape_str(str.c_str()).c_str()) )
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

  // If category undefined and we have a redirect entry..
  if( atoi(row[2]) == 1 )
  {
    char redirect[MAX_STRING_LENGTH];
    if( sscanf( row[1], "Redirect: %s", redirect) == 1 )
    {
      mysql_free_result(res);
      return wiki_help_single(redirect);
    }
  }

  return_str = "&+c";
  return_str += row[0];
  return_str += "&N - Last Edited: &+w";
  return_str += row[3];
  return_str += "&n by &+w";
  return_str += (row[4] == NULL) ? "Unknown" : row[4];

  dashes = ansi_strlen(return_str.c_str());
  return_str += "&N\n&+L";
  while( dashes-- > 0 )
  {
    return_str += "=";
  }
  return_str += "&N\n";

  return_str += dewikify(trim(string(row[1]), " \t\n"));

  // Race type help files
  if( atoi(row[2]) == 25 )
  {
    title += row[0];
    return_str += "\n";
    return_str += wiki_classes(title);
    return_str += "\n";
    return_str += wiki_racial_stats(title);
    return_str += "\n";
    return_str += wiki_innates(title, WIKI_RACE);
  }
  // Class type help files
  else if( atoi(row[2]) == 9 )
  {
    title += row[0];
    return_str += "\n";
    return_str += wiki_races(title, WIKI_CLASS);
    return_str += "\n";
    return_str += wiki_innates(title, WIKI_CLASS);
    return_str += "\n";
    return_str += wiki_specs(title);
  }
  // Spec type help files
  else if( atoi(row[2]) == 16 )
  {
    title += row[0];
    // Dunno why it requires two carriage returns here, but it does.
    return_str += "\n\n";
    return_str += wiki_races(title, WIKI_SPEC);
    return_str += "\n";
    return_str += wiki_innates(title, WIKI_SPEC);
    return_str += "\n";
    return_str += wiki_skills(title, WIKI_SPEC);
    return_str += "\n";
    return_str += wiki_spells(title, WIKI_SPEC);
  }
  // Class skillsets type help files
  else if( atoi(row[2]) == 10 )
  {
    // Need to pull " Skills" off the title.
    char buf[MAX_INPUT_LENGTH];
    one_argument( row[0], buf );
    title += buf;
    return_str += "\n\n";
    return_str += wiki_innates(title, WIKI_CLASS);
    return_str += "\n";
    return_str += wiki_skills(title, WIKI_CLASS);
    return_str += "\n";
    return_str += wiki_spells(title, WIKI_CLASS);
  }

  // Yeah, yeah.. I know this is a hack..
  if( !strcmp(row[0], "Multiclass") )
  {
    return_str += wiki_multiclass(row[0]);
  }
  else if( !strcmp(row[0], "Races") )
  {
    return_str += wiki_pcraces(row[0]);
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

  cmd_file = fopen( "lib/information/command_attributes.txt", "r" );
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
    strcat( attributes, "The following character attributes are used in execution of this ability (if any):\n" );
    if( ch_attributes[ATT_STR] )
       strcat( attributes, "Char's Strength.\n" );
    if( vi_attributes[ATT_STR] )
       strcat( attributes, "Victim's Strength.\n" );
    if( ch_attributes[ATT_DEX] )
       strcat( attributes, "Char's Dexterity.\n" );
    if( vi_attributes[ATT_DEX] )
       strcat( attributes, "Victim's Dexterity.\n" );
    if( ch_attributes[ATT_AGI] )
       strcat( attributes, "Char's Agility.\n" );
    if( vi_attributes[ATT_AGI] )
       strcat( attributes, "Victim's Agility.\n" );
    if( ch_attributes[ATT_CON] )
       strcat( attributes, "Char's Constitution.\n" );
    if( vi_attributes[ATT_CON] )
       strcat( attributes, "Victim's Constitution.\n" );
    if( ch_attributes[ATT_POW] )
       strcat( attributes, "Char's Power.\n" );
    if( vi_attributes[ATT_POW] )
       strcat( attributes, "Victim's Power.\n" );
    if( ch_attributes[ATT_INT] )
       strcat( attributes, "Char's Intelligence.\n" );
    if( vi_attributes[ATT_INT] )
       strcat( attributes, "Victim's Intelligence.\n" );
    if( ch_attributes[ATT_WIS] )
       strcat( attributes, "Char's Wisdom.\n" );
    if( vi_attributes[ATT_WIS] )
       strcat( attributes, "Victim's Wisdom.\n" );
    if( ch_attributes[ATT_CHA] )
       strcat( attributes, "Char's Charisma.\n" );
    if( vi_attributes[ATT_CHA] )
       strcat( attributes, "Victim's Charisma.\n" );
    if( ch_attributes[ATT_KAR] )
       strcat( attributes, "Char's Karma.\n" );
    if( vi_attributes[ATT_KAR] )
       strcat( attributes, "Victim's Karma.\n" );
    if( ch_attributes[ATT_STR] )
       strcat( attributes, "Char's Luck.\n" );
    if( vi_attributes[ATT_STR] )
       strcat( attributes, "Victim's Luck.\n" );

    // add list to the array.
    cmd_attribs[count++].attributes = strdup( attributes );
  }
}

char *attrib_help( char *arg )
{
  int count = 0;

  while( cmd_attribs[count].name != NULL )
  {
    if( strstr( cmd_attribs[count].name, arg ) )
    {
      return cmd_attribs[count].attributes;
    }
    count++;
  }

  return NULL;
}

string wiki_spells( string title, int type )
{
  string return_str;
  int i, j, innate;
  bool found = FALSE;

  if( type == WIKI_CLASS )
  {
    for( i = 0; i <= CLASS_COUNT; i++ )
    {
      if( !strcmp(tolower(class_names_table[i].normal).c_str(), tolower(title).c_str()) )
      {
	      found = TRUE;
        j = 0;
      	break;
      }
    }
  }

  if( type == WIKI_SPEC )
  {
    for( i = 0; i <= CLASS_COUNT; i++ )
    {
      for( j = 0; j < MAX_SPEC; j++ )
      {
        if( !strcmp(tolower(strip_ansi(specdata[i][j])).c_str(), tolower(title).c_str()) )
        {
          found = TRUE;
          break;
        }
      }
      if( found == TRUE )
      {
        // Specs range from 1-4 not 0-3.
        j++;
        break;
      }
    }
  }

  return_str += "==Spells==";

  if( !found )
  {
    return_str += "\nNo entries found.\n";
    return return_str;
  }

  // List spells( class, spec )
  return_str += list_spells( i, j );

  // If Bard, then show songs after spells.
  if( i == flag2idx(CLASS_BARD) )
  {
    if( j == 0 )
    {
      return_str += "\n==Songs==";
    }
    else
    {
      return_str += "\n==Spec Songs==";
    }
    return_str += list_songs( i, j );
  }
  return return_str;
}

string wiki_skills( string title, int type )
{
  string return_str;
  int i, j, innate;
  bool found = FALSE;

  if( type == WIKI_CLASS )
  {
    for( i = 0; i <= CLASS_COUNT; i++ )
    {
      if( !strcmp(tolower(class_names_table[i].normal).c_str(), tolower(title).c_str()) )
      {
	      found = TRUE;
        j = 0;
    	  break;
      }
    }
  }

  if( type == WIKI_SPEC )
  {
    for( i = 0; i <= CLASS_COUNT; i++ )
    {
      for( j = 0; j < MAX_SPEC; j++ )
      {
        if( !strcmp(tolower(strip_ansi(specdata[i][j])).c_str(), tolower(title).c_str()) )
        {
          found = TRUE;
          break;
        }
      }
      if( found == TRUE )
      {
        // Specs range from 1-4 not 0-3.
        j++;
        break;
      }
    }
  }

  return_str += "==Skills==";

  if( !found )
  {
    return_str += "\nNo entries found.\n";
    return return_str;
  }

  // List spells( class, spec )
  return_str += list_skills( i, j );

  return return_str;
}

string wiki_multiclass( string title )
{
  string return_str;
  int i, j, k;
  bool found, allowed;

  return_str = "\n\r\n\rHere are the options available to each class:";
  for( i = 1; i <= CLASS_COUNT; i++ )
  {
    found = FALSE;
    for( j = 0; j < 5; j++ )
    {
      // allowed_secondary_classes ends with a -1.
      if( allowed_secondary_classes[i][j] == -1 )
        break;
      if( !found )
      {
        return_str += "\n\r* ";
        return_str += pad_ansi(class_names_table[i].ansi, 12);
        return_str += "&n: ";
      }
      else
      {
        return_str += ", ";
      }
      return_str += class_names_table[flag2idx(allowed_secondary_classes[i][j])].ansi;
      return_str += "&n";
      found = TRUE;
    }
  }

  // This works, but we're missing a lotta multiclass names?
  return_str += "\n\r\n\r==Multi-Class Names==";

  // For each class (skipping CLASS_NONE)..
  for( i = 1; i <= CLASS_COUNT; i++ )
  {
    // For each allowed secondary class
    for( j = 0; j < 5; j++ )
    {
      // allowed_secondary_classes ends with a -1.
      if( allowed_secondary_classes[i][j] == -1 )
      {
        break;
      }
// i : 1 (warrior), j : 0, allowed_secondary_classes[i][j] : CLASS_MERCENARY
      // Find the corresponding multi-class name..
      for( k = 0; multiclass_names[k].cls1 != -1; k++ )
      {
        // If cls1 and cls2 match..
        if( ((multiclass_names[k].cls1 == (1 << (i-1)))
          && (multiclass_names[k].cls2 == allowed_secondary_classes[i][j]))
          || ((multiclass_names[k].cls2 == (1 << (i-1))
          && (multiclass_names[k].cls1 == allowed_secondary_classes[i][j]))) )
        {
          return_str += "\n\r* ";
          return_str += multiclass_names[k].mc_name;
          return_str += "&n: ";
          return_str += class_names_table[ i ].ansi;
          return_str += "&n / ";
          return_str += class_names_table[ flag2idx(allowed_secondary_classes[i][j]) ].ansi;
          return_str += "&n";
          break;
        }
      }
      if( multiclass_names[k].cls1 == -1 )
      {
        return_str += "\n\r&+RMulticlass: &n";
        return_str += "&n not found!";
      }
    }
  }
  return_str += "\n\r";

  return return_str;
}

string wiki_pcraces(string title)
{
  string return_str;
  bool found = FALSE;
  int i, j;

  return_str = "\n\n==Good Races==\n";
  for( i = 1; i <= RACE_PLAYER_MAX; i++ )
  {
    // With a negative align, this won't include neutral races.
    if( !OLD_RACE_GOOD(i, -1) )
      continue;
    // Hunt for an available class for the race.
    for( j = 1; j <= CLASS_COUNT; j++ )
    {
      if( class_table[i][j] != 5 )
        break;
    }
    if( j > CLASS_COUNT )
      continue;

    found = TRUE;
    return_str += "* ";
    return_str += string(race_names_table[i].ansi);
    return_str += "&n\n";
  }
  if( !found )
  {
    return_str += "No data found for Good races.\n";
  }

  found = FALSE;
  return_str += "\n==Evil Races==\n";
  for( i = 1; i <= RACE_PLAYER_MAX; i++ )
  {
    // With a positive align, this won't include neutral races.
    if( !OLD_RACE_EVIL(i, 1) )
      continue;
    // Hunt for an available class for the race.
    for( j = 1; j <= CLASS_COUNT; j++ )
    {
      if( class_table[i][j] != 5 )
        break;
    }
    if( j > CLASS_COUNT )
      continue;

    found = TRUE;
    return_str += "* ";
    return_str += string(race_names_table[i].ansi);
    return_str += "&n\n";
  }
  if( !found )
  {
    return_str += "No data found for Evil races.\n";
  }

  found = FALSE;
  return_str += "\n==Neutral Races==\n";
  for( i = 1; i <= RACE_PLAYER_MAX; i++ )
  {
    if( !OLD_RACE_NEUTRAL(i) )
      continue;
    // Hunt for an available class for the race.
    for( j = 1; j <= CLASS_COUNT; j++ )
    {
      if( class_table[i][j] != 5 )
        break;
    }
    if( j > CLASS_COUNT )
      continue;

    found = TRUE;
    return_str += "* ";
    return_str += string(race_names_table[i].ansi);
    return_str += "&n\n";
  }
  if( !found )
  {
    return_str += "No data found for Neutral races.\n";
  }

  return return_str;
}
