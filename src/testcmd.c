#include <cstring>
#include "testcmd.h"
#include "epic.h"
#include "sql.h"
#include "db.h"
#include "map.h"
#include "spells.h"
#include <fstream>
#include <math.h>
using namespace std;

extern struct zone_data *zone_table;
extern struct room_data *world;
extern const char *sector_symbol[];
extern mapSymbolInfo color_symbol[];
extern P_index mob_index;
extern int count_classes( P_char mob );
extern long new_exp_table[];
extern const char *spells[];
extern struct zone_random_data {
  int zone;
  int races[10];
  int proc_spells[3][2];
} zones_random_data[100];

extern int get_mincircle( int spell );

void display_map(P_char ch, int n, int show_map_regardless);

void disproom(P_char ch, int x, int y)
{
  int      local_y, local_x;
  int vroom = world[ch->in_room].number;
  struct zone_data *zone = &zone_table[world[ch->in_room].zone];
  
  int zone_start_vnum = world[zone->real_bottom].number;

  // how far are we from the northern local map edge
  local_y = ( ( vroom - zone_start_vnum) / zone->mapx ) % zone->mapy;
  
  // how far are we from the western local map edge
  local_x = ( vroom - zone_start_vnum) % zone->mapy;
 
  if( local_x + x < 0 )
    local_x += zone->mapx;
  else if( local_x + x >= zone->mapx )
    local_x -= zone->mapx;
  
  if( local_y + y < 0 )
    local_y += zone->mapy;
  else if( local_y + y >= zone->mapy )
    local_y -= zone->mapy;
  
  int newx = local_x + x;
  int newy = local_y + y;
  
  char buff[100];
  sprintf(buff, "(%d,%d) : [%d] <%d,%d>\n", x,y, (zone_start_vnum + newx + ( newy * zone->mapx)), newx, newy);
  send_to_char(buff, ch);
}

void display_exp_table(P_char ch, char *arg, int cmd)
{
  for( int i = 0; i < TOTALLVLS; i++ )
  {
    char buff[128];
    sprintf(buff, "%d: %ld\n", i, new_exp_table[i]);
    send_to_char(buff, ch);
  }
}

void do_test_room(P_char ch, char *arg, int cmd)
{
  int x = 0, y = 0;
  int      local_y, local_x;
  int vroom = world[ch->in_room].number;
  struct zone_data *zone = &zone_table[world[ch->in_room].zone];
  
  if( !IS_SET(zone->flags, ZONE_MAP))
    send_to_char("This room not on a map.", ch);
  
  int zone_start_vnum = world[zone->real_bottom].number;
  
  //  if (IS_SURFACE_MAP(room))
  //    vroom -= MAP_START;
  
  // how far are we from the northern local map edge
  local_y = ( ( vroom - zone_start_vnum) / zone->mapx ) % zone->mapy;
  
  // how far are we from the western local map edge
  local_x = ( vroom - zone_start_vnum) % zone->mapy;
  
  char buff[100];
  sprintf(buff, "Zone (%dx%d) [%d,%d]\n", zone->mapx, zone->mapy, local_x, local_y);
  send_to_char(buff, ch);

  send_to_char("Calculated vnums:\n", ch);

  disproom(ch, 0, 0);
  disproom(ch, 0, -1);
  disproom(ch, 1, 0);
  disproom(ch, 0, 1);
  disproom(ch, -1, 0);    
}

void do_test_writemap(P_char ch, char *arg, int cmd)
{
  char buff[MAX_STRING_LENGTH];
  buff[0] = '\0';
  
  struct zone_data *zone = &zone_table[world[ch->in_room].zone];
  
  if( !IS_SET(zone->flags, ZONE_MAP))
    send_to_char("This room not on a map.", ch);

  string filename = zone->filename;
  filename += ".map";
  
  ofstream output_file(filename.c_str());
  
  int rroom = zone->real_bottom;
  
  int what = 0;
  bool hadbg = false;
  int prev = -1;
  int where_rnum;
  
  int width = zone->mapx;
  int height = zone->mapy;
  
  for( int y = 0; y < height; y++ )
  {
    buff[0] = '\0';
    for( int x = 0; x < width; x++ )
    {
      where_rnum = calculate_relative_room(rroom, x, y);
      
      if( !where_rnum )
      {
        what = 21; // rock
      }
      else
      {
        what = world[where_rnum].sector_type;
      }
      /*
      if (hadbg)
        strcat(buf, "&n");
      if ((prev != what) || (x == 0))
      {
        int shift = 0;
        if (hadbg && color_symbol[what].hasBg)
          shift = -2;
        sprintf(buff + strlen(buff) + shift, "&%s%s",
                color_symbol[what].colorStrn, sector_symbol[what]);
        
        hadbg = color_symbol[what].hasBg;
        prev = what;
      }
      else
      {
        int shift = 0;
        if (hadbg)
          shift = -2;
        sprintf(buff + strlen(buff) + shift, "%s", sector_symbol[what]);
      }*/
      sprintf(buff + strlen(buff), "&%s%s", color_symbol[what].colorStrn, sector_symbol[what]);
//      if( color_symbol[what].hasBg )
//        strcat(buff, "&n");
      
      //strcat(buff, sector_symbol[what]);
    }
       
    strcat(buff, "\n");       // removed '&n'
    output_file << buff;
   // send_to_char(buff, ch);
  }
    
}
/*
void do_test_add_epic_skillpoint(P_char ch, const char *charname)
{
  P_char victim = get_char_room_vis(ch, charname);

  if( !victim )
  {
    send_to_char("No player by that name here.\n", ch);
    return;
  }
  
  epic_gain_skillpoints(victim, 1);
  send_to_char("&+WYou add an epic skill point.\n", ch);
  send_to_char("&+WYou have gained an epic skill point!\n", victim);

}
*/
extern Skill skills[];

void do_test(P_char ch, char *arg, int cmd)
{
  char buff[MAX_STRING_LENGTH];

  if( !IS_TRUSTED(ch) )
  {
    return;
  }

  sprintf(buff, "%s in [%d]: test %s", GET_NAME(ch), world[ch->in_room].number, arg);

  wizlog(56, buff);
  logit(LOG_WIZ, buff);

  arg = one_argument(arg, buff);

  if( isname("room", buff) )
  {
    do_test_room(ch, arg, cmd);
    return;
  }
  else if( isname("exp", buff) )
  {
    display_exp_table(ch, arg, cmd);
    return;
  }
  else if( isname("map", buff) )
  {
    one_argument(arg, buff);
    display_map(ch, atoi(buff), TRUE); 
    return;
  }
  else if( isname("writemap", buff) )
  {
    do_test_writemap(ch, arg, cmd);
    return;
  }
  else if( isname("arti", buff) )
  {
    P_obj arti;
    int   feed_min;

    arg = one_argument( arg, buff );
    if( (arti = get_obj_in_list(buff, ch->carrying)) != NULL )
    {
      one_argument( arg, buff );
      if( (feed_min = atoi(buff)) > 0 )
      {
        artifact_feed_to_min( arti, feed_min );
      }
      else
      {
        send_to_char( "Please enter a valid (positive) minimum feed minutes for arti.\n\r", ch );
      }
    }
    else
    {
      send_to_char( "Couldn't find object to test.\n\r", ch );
    }
    return;
  }
  else if( isname("mem", buff) )
  {
    int spell = memorize_last_spell(ch);

    if( spell )
    {
      char buf[256];
      sprintf( buf, "&+Cpower of %s\n", skills[spell].name );
      send_to_char(buf, ch);
    }
    return;
  }
  else if( isname("skills", buff) )
  {
    for( int i = 0; i < MAX_SKILLS; i++ )
    {
      debug("[%d] %s", GET_CHAR_SKILL(ch, i), skills[i].name);
    }
  }
  else if ( isname("loadallchars", buff) )
  {
    test_load_all_chars(ch);
    return;
  }
  else if ( isname("count", buff) )
  {
    char   buf[MAX_STRING_LENGTH];
    int    classes;
    P_char mob;
    P_obj  obj;

    generic_find( arg, FIND_CHAR_WORLD, ch, &mob, &obj );
    if( mob )
    {
      classes = count_classes( mob );
      sprintf( buf, "%s (%d) has %d classes.\n", J_NAME(mob), IS_PC(mob) ? -1 : GET_VNUM(mob), classes );
      send_to_char( buf, ch );
    }
    else
    {
      sprintf( buf, "Char '%s' not found.\n", skip_spaces(arg) );
      send_to_char( buf, ch );
    }
    return;
  }
  else if ( isname("sql_log", buff) )
  {
    P_char mob;
    // Shadow
    mob = read_mobile(92076, VIRTUAL);
    sql_log( mob, WIZLOG, "SQL: '%s'" , arg );
    extract_char( mob );
  }
  else if ( isname("xlogx", buff) )
  {
    int num = atoi(arg);
    char   buf[MAX_STRING_LENGTH];

    if( num > 0 )
    {
      for( int i = 1;i <= num; i++ )
      {
        sprintf( buf, "%d log %d: %f == %d.\n\r", i, i, i * log(i), (int) (i * log(i)) );
        send_to_char( buf, ch );
      }
    }
    else
    {
      send_to_char( "This is for testing x*log(x) function used in itemvalue.\n\rTakes a number > 0 as an argument.\n\r", ch );
    }
  }
  else if ( isname("xlog2x", buff) )
  {
    int num = atoi(arg);
    char   buf[MAX_STRING_LENGTH];

    if( num > 0 )
    {
      for( int i = 1;i <= num; i++ )
      {
        sprintf( buf, "%d log2 %d: %f == %d.\n\r", i, i, i * log2(i), (int) (i * log2(i)) );
        send_to_char( buf, ch );
      }
    }
    else
    {
      send_to_char( "This is for testing x*log2(x) function used in itemvalue.\n\rTakes a number > 0 as an argument.\n\r", ch );
    }
  }
  else if ( isname("spell", buff) )
  {
    int num = atoi(arg);
    char   buf[MAX_STRING_LENGTH];

    if( num < 1 )
    {
      num = search_block( skip_spaces(arg), (const char **) spells, FALSE );
    }
    if( num < FIRST_SPELL || num > LAST_SPELL )
    {
      sprintf( buf, "Spell must be a number between %d and %d, or a valid spell name.\n\r", FIRST_SPELL, LAST_SPELL );
      send_to_char( buf, ch );
      return;
    }
    sprintf( buf, "The mincircle for spell '%s' (%d), is %d.\n\r", spells[num], num, get_mincircle(num) );
    send_to_char( buf, ch );
  }
  else if ( isname("randomize", buff) )
  {
    P_obj obj;

    arg = one_argument( arg, buff );
    if( (obj = get_obj_in_list(buff, ch->carrying)) != NULL )
    {
      randomizeitem(ch, obj);
    }
    else
    {
      send_to_char( "Could not find object '", ch );
      send_to_char( buff, ch );
      send_to_char( "' in your inventory to randomize.\n\r", ch );
    }
  }
  else if ( isname("randomzones", buff) )
  {
    int i, j;
    char buf[MAX_STRING_LENGTH];
    FILE *pf;

    if( !(pf = fopen( "logs/log/randomzones", "w" )) )
    {
      send_to_char( "Couldn't open file. :(\n\r", ch );
      return;
    }
    fprintf( pf, "Printing list of zones with their spells:\n" );
    debug( "List of zones_random_data:" );
    for( i = 0; i < 100; i++ )
    {
      if( zones_random_data[i].zone == 0 )
      {
        break;
      }
      debug( "%2d) %4d = %4d rb: %7d zone: '%s'.", i, zones_random_data[i].zone, zone_table[real_zone0(zones_random_data[i].zone)].number,
        zone_table[real_zone0(zones_random_data[i].zone)].real_bottom, zone_table[real_zone0(zones_random_data[i].zone)].name );

      fprintf(pf, "%2d) '%s'\n", i+1, strip_ansi(zone_table[real_zone0(zones_random_data[i].zone)].name).c_str() );
      for( j = 0; j < 3; j++ )
      {
        if( zones_random_data[i].proc_spells[j][0] > 0 )
        {
          fprintf(pf, "  With %d pieces: Spell '%s'.\n", zones_random_data[i].proc_spells[j][0],
            zones_random_data[i].proc_spells[j][1] == SPELL_STONE_SKIN ? "stone skin" :
            zones_random_data[i].proc_spells[j][1] == SPELL_BLESS ? "bless" :
            zones_random_data[i].proc_spells[j][1] == SPELL_STRENGTH ? "strength" :
            zones_random_data[i].proc_spells[j][1] == SPELL_BARKSKIN ? "barkskin" :
            zones_random_data[i].proc_spells[j][1] == SPELL_ARMOR ? "armor" : "unknown" );
        }
      }
    }
    return;
  }
  else if ( isname("clearepiczones", buff) )
  {
    if( GET_LEVEL(ch) >= FORGER )
    {
      while( *arg == ' ' )
      {
        arg++;
      }
      if( isname(arg, "confirm") )
      {
        sql_clear_zone_trophy();
      }
      else
      {
        send_to_char("You must '&=Blconfirm&n' this command in order to use it.\n\r", ch );
        send_to_char("This will &=Blpermenantly&n clear the zone alignment list; there is no going back!\n\r", ch );
      }
    }
    else
    {
      send_to_char( "You must be a &+LFORGER&n or higher to execute this command.\n\r", ch );
    }
  }
  else if ( isname("clearoutposts", buff) )
  {
    if( GET_LEVEL(ch) >= FORGER )
    {
      while( *arg == ' ' )
      {
        arg++;
      }
      if( isname(arg, "confirm") )
      {
        send_to_char( "&=GLResetting oupost information... .. .", ch );
        if( qry("UPDATE outposts SET owner_id='0', level='8', walls='1', archers='0', hitpoints='300000', territory='0',"
          " portal_room='0', resources='0', applied_resources='0', golems='0', meurtriere='0', scouts='0'") )
        {
          send_to_char("&=Bl success!&n\n\r", ch);
        }
        else
        {
          send_to_char("&=Rl failure!&n\n\r", ch);
        }
      }
      else
      {
        send_to_char("You must '&=Blconfirm&n' this command in order to use it.\n\r", ch );
        send_to_char("This will &=Blpermenantly&n reset the info on outposts, setting owners to none; there is no going back!\n\r", ch );
      }
    }
    else
    {
      send_to_char( "You must be a &+LFORGER&n or higher to execute this command.\n\r", ch );
    }
  }
  else
  {
    send_to_char("Invalid keyword.\r\n", ch);
    return;
  }
}

