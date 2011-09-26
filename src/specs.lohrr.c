#include "structs.h"
#include "utils.h"
#include "interp.h"
#include "comm.h"
#include "prototypes.h"
extern P_room world;

int adjacent_room_nesw(P_char ch, int num_rooms );

// This is an old proc for Lohrr's eq..
void proc_lohrr( P_obj obj, P_char ch, int cmd, char *argument )
{
   int locwearing;

   // First, verify that it was called properly
   if( !obj || !ch )
      return;

   // Verify object is worn by cy
   if( !OBJ_WORN( obj ) || obj->loc.wearing != ch)
      return;

   for(locwearing = 0;locwearing < MAX_WEAR; locwearing++ )
   {
      if( ch->equipment[locwearing] == obj )
         break;
   }
   // obj is not worn !  This must be a bug if true.
   if( locwearing = MAX_WEAR )
      return;

   switch( locwearing )
   {
      // For his quiver first
      case WEAR_QUIVER:
	// Heal if down more than 10 hps
	if( GET_HIT(ch) < GET_MAX_HIT(ch) - 10 )
	   spell_full_heal( 60, ch, 0, 0, ch, 0);
      break;
   }
}

// This is a proc for the Leviathan mob.
int leviathan( P_char ch, P_char pl, int cmd, char *arg )
{
   P_char tch;
   int to_room;

// ship_data * ship;

   if( cmd == CMD_SET_PERIODIC )
      return TRUE;

   if( !ch )
      return TRUE;

// random chance to nearby ships:
// if( ship = find_ship( ch ) && number( 0, 3) )
// {
//       roll/whale dive damaging ships like a ram attack
//       return TRUE;
// }

   if( cmd == CMD_PERIODIC && !number( 0, 1 ) )
   {
      switch( number( 1, 2 ) )
      {
      case 1:
         act( "$N lifts up out of the water, then splashes back down, causing a massive wave!", FALSE, ch, NULL, ch, TO_ROOM );

         // To each char in room, chance of knockdown.
         for( tch = world[ch->in_room].people;tch;tch = tch->next_in_room )
         {
            if( tch != ch )
            {
               if( number( 0, 1 ) )
                  SET_POS( tch, POS_SITTING + GET_STAT(tch));
               // if not knocked down, chance to get moved 1 room away.
               else if( number( 0, 1 ) && (to_room = adjacent_room_nesw(ch, 1)) )
               {
                  // Move char 1 room
                  char_from_room(tch);
                  char_to_room(tch, to_room, -1);
               }
            }
         }
      break;
      case 2:
         tch = ch->specials.fighting;
         if( tch )
         {
            act( "$N lashes out with a tentacle, wrapping it around you, lifts and quickly slams you upon the water surface!", FALSE, tch, NULL, ch, TO_CHAR );
            act( "$N lashes out grabbing $n with a tentacle, thrashing $m into the water!", FALSE, tch, NULL, ch, TO_ROOM );
            // Move victim 1-3 rooms away
            if( to_room = adjacent_room_nesw(ch, number( 1, 3 )) )
            {
               char_from_room(tch);
               char_to_room(tch, to_room, -1);
            }
            stop_fighting( tch );
            // Stun for 3-5 sec
            CharWait( tch, number( 3, 5 ) );
         }
      default:
      break;
      }
   }

   return FALSE;
}

// It's a percentage chance to make them attack a few extra times.
// It's size dependdent: < medium = 10, medium/large = 6, > large = 4
void dagger_of_wind( P_obj obj, P_char ch, int cmd, char *argument )
{
   int numhits = 0;
   int i = 0;

   // Verify that obj is dagger of wind and being wielded by ch.
   if( cmd != CMD_MELEE_HIT || !ch || !obj || !OBJ_WORN(obj) || obj->loc.wearing != ch )
      return;
   // Verify that ch is in battle with someone.
   if( !IS_FIGHTING(ch) || !ch->specials.fighting )
      return;

   // 50% chance to proc.
   if( number(1,100) > 50 )
   {
       act("You move with a blur of speed!",
          FALSE, ch, obj, 0, TO_CHAR);
       act("$n moves with a blur of speed!",
          FALSE, ch, obj, 0, TO_ROOM);

      // Calculate number of hits based on size.
      if( GET_SIZE(ch) < SIZE_MEDIUM )
         numhits = 10;
      else if( GET_SIZE(ch) == SIZE_MEDIUM || GET_SIZE(ch) == SIZE_LARGE )
         numhits = 6;
      else
         numhits = 4;

      while( i < numhits )
      {
         // Stop hitting if no one to hit.
         if( !ch->specials.fighting )
            break;
         hit(ch, ch->specials.fighting, obj );
         i++;
      }
   }
}

// Alright, so... I made a first attempt at trying to hack some code together and
//  wanted to see if I did it correct. I'm going to cut/paste the proc I put together.
// Basicly, what I was intending, is for a proc that works on command, with a cooldown
//  (yea, the current cooldown is to fast for this item, but i can tweak)... lemme know
//  if it looks right, or what needs to change...
int sphinx_prefect_crown( P_obj obj, P_char ch, int cmd, char *arg )
{
   int curr_time;
   char first_arg256;

   if( cmd == CMD_SET_PERIODIC )
      return TRUE;
   if( !OBJ_WORN_POS( obj, WEAR_HEAD ) )
      return FALSE;

   curr_time = time(NULL);

   if( arg && (cmd == CMD_SAY) && isname(arg, "sphinx"))
   {

      // Set timer here: 5 min = 60 * 5 sec = 300
      // Note: This will be replaced by get_property("timer.proc.crownXXX", ??? )
      // Note: 300 is a magic number here and below.
      if( curr_time >= obj->timer[0] + 300 )
      {
         act("You say 'sphinx'", FALSE, ch, 0, 0, TO_CHAR);
         act("$n says 'sphinx'", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LYour crown seems to &+Yp&+yu&+Yl&+ys&+Ye &+Lwith a vibrant &+Ymagic&+L!&n", FALSE, ch, obj, obj, TO_CHAR);
         act("&+LA misty haze of &+yun&+Learthly &+Cknowledge &+Lflows from your crown!&n", FALSE, ch, obj, obj, TO_CHAR);
         act("&+mS&+Mw&+mi&+Mr&+ml&+Mi&+mn&+Mg &+Lthoughts and words of &+Bmagic &+Lseem to &+rsear &+Linto your mind as the wisdom of the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Cinvigorates &+Lyou with &+Gpower&+L!&n", FALSE, ch, obj, obj, TO_CHAR); 
         act("$n's &+Lcrown glows with a &+Gv&+gi&+Gb&+gr&+Ga&+gn&+Gt &+Ylight&+L!&n", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LThe image of a wise &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Lseems to &+Cshimmer &+Laround $n's &+Lhead!&n", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LIn a rush of displaced &+Cair&+L, the image of the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Levaporates into a misty essence infused with &+mmagic&+L!&n", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LThe misty essence of the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Lenvelopes $n!", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LA wicked &+rgrin &+Lpasses across $n's &+Lface as they are &+me&+Mm&+mp&+Mo&+mw&+Me&+mr&+Me&+md &+Lwith &+rancient &+gwisdom &+Land &+Cknowledge&+L!&n", TRUE, ch, obj, NULL, TO_ROOM);
         spell_mordenkainens_lucubration(60, ch, 0, 0, ch, NULL);
         obj->timer[0] = curr_time;
         obj->timer[1] = 0;														      return FALSE;
         return TRUE;
      }
   }

   // Send a message (once) to the char when the crown becomes usable again.
   if (cmd == CMD_PERIODIC && obj->timer[0] + 300 <= curr_time && obj->timer[1] == 0 )
   {
      act("&+LYour legendary &+ycrown &+Lof the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Gp&+gr&+Ge&+gf&+Ge&+gc&+Gt&+gs &+Cglows &+Lwith an unearthly &+Wlight &+Land starts to &+mpulse &+Lgently in time to your &+rheartbeat&+L.&n",
         TRUE, obj->loc.wearing, obj, 0, TO_CHAR);
      obj->timer[1] = 1;														      return FALSE;
   }
}

int adjacent_room_nesw(P_char ch, int num_rooms )
{
   int dir = number( 0, 3 );
   int i, j;
   room_direction_data *exit;

   if( !ch )
      return -1;

   for( i = 0; i < 3; i++ )
   {
      exit = EXIT( ch, (dir+i)%4 );
      if( exit && exit->to_room )
         // Move num_rooms away (if possible).
         for ( j = 1; j < num_rooms;j++ )
            if( world[exit->to_room].dir_option[(dir+i)%4] )
               exit = world[exit->to_room].dir_option[(dir+i)%4];
      if( exit && exit->to_room )
         return exit->to_room;
   }

   // If no exit found, return -1;
   return -1;
}
