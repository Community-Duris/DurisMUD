#include <cstring>
#include "testcmd.h"
#include "epic.h"
#include "sql.h"
#include "db.h"
#include "map.h"
#include <fstream>
using namespace std;

extern struct zone_data *zone_table;
extern struct room_data *world;
extern const char *sector_symbol[];
extern mapSymbolInfo color_symbol[];
extern P_index mob_index;
extern int count_classes( P_char mob );

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

extern long      new_exp_table[];

void display_exp_table(P_char ch, char *arg, int cmd)
{
  for( int i = 0; i < 63; i++ )
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
  }/*
  else if( isname("esp", buff) )
  {
    one_argument(arg, buff);
    do_test_add_epic_skillpoint(ch, buff);
    return;    
  }*/
  else if( isname("mem", buff) )
  {
    int spell = memorize_last_spell(ch);

    if( spell )
    {
      char buf[256];
      sprintf( buf, "&+Cpower of %s\n",
               skills[spell].name );
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
  else
  {
    send_to_char("Invalid keyword.\r\n", ch);
    return;
  }
}

