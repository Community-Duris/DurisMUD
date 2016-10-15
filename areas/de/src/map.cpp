//
//  File: map.cpp       originally part of durisEdit, derived largely from Duris map code
//
//  Usage: functions related to looking at a map
//


#include "room/room.h"

#include "fh.h"

extern room *g_currentRoom;
extern zone g_zoneRec;

const char *sector_symbol[NUMB_SECT_TYPES] =
{
  "^",                          /* * larger towns */
  "+",                          /* * roads */
  ".",                          /* * plains/fields */
  "*",                          /* * forest */
  "^",                          /* * hills */
  "M",                          /* * mountains */
  "r",                          /* * water shallow */
  " ",                          /* * water boat */
  "?",                          /* * noground */
  "?",                          /* * underwater */
  "?",                          /* * underwater ground */
  " ",                          /* * fire plane */
  " ",                          /* * water ship */
  ".",                          /* * UD wild */
  "^",                          /* * UD city */
  ".",                          /* * UD inside */
  " ",                          /* * UD water */
  " ",                          /* * UD noswim */
  "?",                          /* * UD noground */
  "?",                          /* * air plane */
  "?",                          /* * water plane */
  " ",                          /* * earth plane */
  "?",                          /* * ethereal plane */
  "?",                          /* * astral plane */
  ".",                          /* desert */
  ".",                          /* arctic tundra */
  "*",                          /* swamp */
  "M",                          /* UD mountains */
  " ",                          /* UD slime */
  ",",                          /* UD low ceilings */
  " ",                          /* UD liquid mithril */
  "o",                          /* UD mushroom forest */
  " ",                          /* Castle Wall */
  "#",                          /* Castle Gate */
  "0",                          /* Castle Itself */
  "X",                          /* Negative Plane */
  "-",                          /* Plane of Avernus */
  "+",                          /* Patrolled road */
  "*",                          /* Snowy Forest */
};

const char *color_symbol[NUMB_SECT_TYPES] =
{
  "=wl",                        /* * larger towns */
  "+L",                         /* * roads */
  "+g",                         /* * plains/fields */
  "=gl",                        /* * forest */
  "+y",                         /* * hills */
  "+y",                         /* * mountains */
  "=cl",                        /* * water shallow */
  "=bB",                        /* * water boat */
  "+w",                         /* * noground */
  "+w",                         /* * underwater */
  "+w",                         /* * underwater ground */
  "=rR",                        /* * fire plane */
  "=bB",                        /* * water ship */
  "=mL",                        /* * UD wild */
  "=wL",                        /* * UD city */
  "+m",                         /* * UD inside */
  "=bB",                        /* * UD water */
  "=bB",                        /* * UD noswim */
  "+w",                         /* * UD noground */
  "+w",                         /* * air plane */
  "=lL",                        /* * water plane */
  "+w",                         /* * earth plane */
  "+w",                         /* * ethereal plane */
  "+w",                         /* * astral plane */
  "=yY",                        /* desert */
  "+W",                        /* arctic tundra */
  "=mL",                        /* swamp */
  "+L",                         /* UD mountains */
  "=gG",                        /* UD slime */
  "+M",                         /* UD low ceilings */
  "=wW",                        /* UD liquid mithril */
  "+M",                         /* UD mushroom forest */
  "=wW",                        /* Castle Wall */
  "=wL",                        /* Castle Gate */
  "=wL",                        /* Castle Itself */
  "+L",                         /* Negative Plane */
  "+R",                         /* Plane of Avernus */
  "=wl",                        /* Patrolled road */
  "=wl",                        /* Snowy Forest */
};

int calculate_relative_room(room *room, int x, int y)
{
  if( !room )
    return FALSE;
  
  int      local_y, local_x;
  int vroom = room->roomNumber;
  int zone_start_vnum = getLowestRoomNumber();
  
  // how far are we from the northern local map edge
  local_y = ( ( vroom - zone_start_vnum) / g_zoneRec.mapWidth ) % g_zoneRec.mapHeight;
  
  // how far are we from the western local map edge
  local_x = ( vroom - zone_start_vnum) % g_zoneRec.mapWidth;
  
  // handle horizontal wrapping
  if( local_x + x < 0 )
    local_x += g_zoneRec.mapWidth;
  else if( (uint)(local_x + x) >= g_zoneRec.mapWidth )
    local_x -= g_zoneRec.mapWidth;
  
  // handle vertical wrapping
  if( local_y + y < 0 )
    local_y += g_zoneRec.mapHeight;
  else if( (uint)(local_y + y) >= g_zoneRec.mapHeight )
    local_y -= g_zoneRec.mapHeight;
  
  return (zone_start_vnum + local_x + x + ( (local_y + y ) * g_zoneRec.mapWidth));
}

//
// displayMap - mostly copied from map.c on Duris, modified a tad for DE
//

void displayMap(void)
{
//  int where, what;
  char buf[16384];

 /* return, so map isnt on prompt line */

  int window_x = 10;
  int window_y = 7;
  
  _outtext("\n\n");

  // draw a line +------...--+
  sprintf(buf, " +");
  for( int i = 0; i < ( (window_x * 2)+1); i++ )
  {
    strcat(buf, "-");
  }
  strcat(buf, "+\n");
  displayColorString(buf);

  for (int y = -window_y; y <= window_y; y++)
  {
    sprintf(buf, " |");
    for (int x = -window_x; x <= window_x; x++)
    {
      int where = calculate_relative_room(g_currentRoom, x, y);

      room *temp_room = findRoom(where);

      int what = SECT_EARTH_PLANE; // if no map square, show blank

      if(temp_room)
        what = temp_room->sectorType;

      if (what < 0) what = 0;
      if (what > SECT_HIGHEST) what = SECT_HIGHEST;

      if (x == 0 && y == 0) 
      {	
        /* you */
        sprintf(buf + strlen(buf), "&n&+W@");
      } 
      else 
      {
        sprintf(buf + strlen(buf), "&n&%s%s", color_symbol[what], sector_symbol[what]);
      }

    }
    strcat(buf, "&n|\n");
    displayColorString(buf);
  }

  // draw a line +------...--+
  sprintf(buf, " +");
  for( int i = 0; i < ( (window_x * 2)+1); i++ )
  {
    strcat(buf, "-");
  }
  strcat(buf, "+\n");
  displayColorString(buf);
    
  displayColorString("\n");
}
