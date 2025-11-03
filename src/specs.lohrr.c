#include "structs.h"
#include "achievements.h"
#include "comm.h"
#include "damage.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "justice.h"
#include "objmisc.h"
#include "prototypes.h"
#include "ships.h"
#include "spells.h"
#include "utils.h"
#include <string.h>

extern P_room world;
extern P_desc descriptor_list;
extern bool has_skin_spell(P_char);

int adjacent_room_nesw(P_char ch, int num_rooms );
P_ship leviathan_find_ship( P_char leviathan, int room, int num_rooms );

// This is an old proc for Lohrr's eq..
int proc_lohrr( P_obj obj, P_char ch, int cmd, char *argument )
{
  int  locwearing;
  char buf[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;

  // First, verify that it was called properly
  if( !obj )
    return FALSE;

  // Verify object is worn by cy
  if( !OBJ_WORN( obj ) || !obj->loc.wearing )
    return FALSE;

  if( !ch )
    ch = obj->loc.wearing;

  if( ch != obj->loc.wearing )
    return FALSE;

  for( locwearing = 0;locwearing < MAX_WEAR; locwearing++ )
  {
    if( ch->equipment[locwearing] == obj )
      break;
  }
  // obj is not worn !  This must be a bug if true.
  if( locwearing == MAX_WEAR )
    return FALSE;

  switch( locwearing )
  {
    // For his quiver first
    case WEAR_QUIVER:
      // Heal if down more than 10 hps
      if( (GET_HIT(ch) < GET_MAX_HIT(ch) - 10) && (cmd == CMD_PERIODIC) )
      {
        spell_full_heal( 60, ch, 0, 0, ch, 0);
        return TRUE;
      }
    break;
    case WIELD:
    case WIELD2:
    case WIELD3:
    case WIELD4:
      // RAWR!  On a 4, 5 or 6 proc bigbys hand!
      if( (cmd == CMD_MELEE_HIT) && GET_OPPONENT(ch) && (dice(1, 6) > 3) )
      {
        spell_bigbys_crushing_hand(60, ch, NULL, SPELL_TYPE_SPELL, GET_OPPONENT(ch), 0);
        return TRUE;
      }
    break;
    case WEAR_WAIST:
      if( (cmd == CMD_PERIODIC) && !has_skin_spell(ch) && (dice(1, 6) > 4) )
        spell_biofeedback(60, ch, 0, 0, ch, 0);
    break;
  }

  return FALSE;
}

/*
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
   if( !IS_FIGHTING(ch) || !GET_OPPONENT(ch) )
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
         if( !GET_OPPONENT(ch) )
            break;
         hit(ch, GET_OPPONENT(ch), obj );
         i++;
      }
   }
}
*/

int sphinx_prefect_crown( P_obj obj, P_char ch, int cmd, char *arg )
{
  int curr_time;
  char first_arg[256];

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN_POS( obj, WEAR_HEAD ) )
  {
    return FALSE;
  }

  curr_time = time(NULL);

  if( arg && (cmd == CMD_SAY) && isname(arg, "sphinx") )
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
      obj->timer[1] = 0;
      return TRUE;
    }
  }

  // Send a message (once) to the char when the crown becomes usable again.
  if(cmd == CMD_PERIODIC && obj->timer[0] + 300 <= curr_time && obj->timer[1] == 0 )
  {
    act("&+LYour legendary &+ycrown &+Lof the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Gp&+gr&+Ge&+gf&+Ge&+gc&+Gt&+gs &+Cglows &+Lwith an unearthly &+Wlight &+Land starts to &+mpulse &+Lgently in time to your &+rheartbeat&+L.&n",
      TRUE, obj->loc.wearing, obj, 0, TO_CHAR);
    obj->timer[1] = 1;
    return FALSE;
  }
  return 0;
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

// This is a proc for the Leviathan mob.
int leviathan( P_char ch, P_char pl, int cmd, char *arg )
{
   P_char tch;
   int to_room;
   P_ship ship;
   int ram_damage, heading;

   if( cmd == CMD_SET_PERIODIC )
      return TRUE;

   if( !ch )
      return TRUE;

   // Attack nearby ships (within 3 rooms ) if not fighting
   if( cmd == CMD_PERIODIC && !number( 0, 1 ) && !IS_FIGHTING(ch) )
      if( ship = leviathan_find_ship( ch, ch->in_room, 3 ) )
      {
          act_to_all_in_ship( ship,"$N speeds into your ship!", ch );
          act( "$N speeds into a ship!", FALSE, ch, NULL, ch, TO_ROOM );
          ram_damage = number( 10, 15 );
          heading = number( 0, 3 );
          ch_damage_hull( ch, ship, ram_damage, heading, number( 0, 99 ) );
          update_ship_status( ship );
          return TRUE;
      }

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
         tch = GET_OPPONENT(ch);
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
            if( IS_DESTROYING( tch ) )
              stop_destroying( tch );
            // Stun for 3-5 sec
            CharWait( tch, number( 3, 5 ) );
         }
      default:
      break;
      }
   }

   if( cmd == CMD_PERIODIC && ch->in_room == GET_BIRTHPLACE(ch) )
   {
      do_flee( ch, "", CMD_FLEE );
   }

   return FALSE;
}

// Returns a ship if it's near Leviathan
P_ship leviathan_find_ship( P_char leviathan, int room, int num_rooms )
{
   int i;
   P_obj obj;
   P_ship ship;
   room_direction_data *exit;
   char msg[100];

   if( !leviathan )
      return NULL;

   // Look through contents
   for(obj = world[room].contents;obj;obj = obj->next_content )
      // If found a ship && percent >= 50
      if( obj && (GET_ITEM_TYPE(obj) == ITEM_SHIP) && (obj->value[6] == 1) && number( 0, 1) )
         return shipObjHash.find(obj);

   // This is a bit repetative, but that's ok, it's for a small number.
   if( num_rooms > 0 )
   {
      // We only check exits N, S, E and W.
      for(i = 0; i < 4; i++ )
      {
         exit = world[room].dir_option[i];
         if( exit && exit->to_room )
         {
            ship = leviathan_find_ship( leviathan, exit->to_room, num_rooms - 1 );
            if( ship )
               return ship;
         }
      }
   }
   return NULL;
}

// This is a proc for Soldon's hat which he won via winning competitive wipe: ship frags.
int proc_soldon_hat( P_obj obj, P_char ch, int cmd, char *argument )
{
  P_char mob;
  int    count;
  static time_t timer = 0;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // Make sure he's rubbing it, he's alive, it's worn by him, and it's his hat.
  if( cmd != CMD_RUB || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || OBJ_VNUM(obj) != 435 )
  {
    return FALSE;
  }

  // 5 min timer.
  if( time(NULL) < timer + 300 )
  {
    send_to_char( "&+yYou rub the brim of your hat, but nothing happens..&n\n\r", ch );
    return TRUE;
  }
  // Reset timer.
  timer = time(NULL);

  count = 4;
  // Load count deck hands.
  while( count-- )
  {
    // Use vnum to load them here and set charm.
    // Note: These pets have a proc that purges them if not on a ship!
    mob = read_mobile(40248, VIRTUAL);
    if( !mob )
    {
      logit(LOG_DEBUG, "proc_soldon_hat: mob 40248 not loadable");
      send_to_char("Bug in Soldon's hat proc.  Tell a god!\n", ch);
      return TRUE;
    }
    char_to_room( mob, ch->in_room, 0 );
    // If the pet will stop being charmed after a bit, also make it suicide 10 minutes later
    if( setup_pet(mob, ch, 11, PET_NOCASH | PET_NOAGGRO) >= 0 )
    {
      add_event(event_pet_death, 10 * 60 * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    }
    add_follower(mob, ch);
    apply_achievement(mob, TAG_CONJURED_PET);
  }

  return 0;
}

// This is a proc to punish people who crash the mud.
int very_angry_npc( P_char ch, P_char pl, int cmd, char *arg )
{
  char buf[MAX_STRING_LENGTH];
  P_desc d;
  P_obj headgear, head, corpse;
  bool naked;
  int i;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // No making yer charmies crash the mud either...
  if( IS_ALIVE(pl) && IS_PC_PET(pl) && IS_ALIVE(GET_MASTER(pl)) )
  {
    pl = GET_MASTER(pl);
    send_to_char( "&+BYou suddenly feel responsible for your pet's actions...\n\r", pl );
  }

  if( !IS_ALIVE(ch) || !IS_ALIVE(pl) || !IS_PC(pl) )
  {
    return FALSE;
  }

  if( cmd == CMD_SHOUT && GET_PID(pl) == 32620 )
  {
    snprintf(buf, MAX_STRING_LENGTH, "Lohrr, come stop %s from cheating.  The bastard is trying to crash the MUD!", J_NAME(pl) );
    do_shout( ch, buf, CMD_SHOUT );
    snprintf(buf, MAX_STRING_LENGTH, "&+CYou hear a rumbling like thunder... &+g'Ok, %s&+g, I shall deal with %s.\n\r"
      "&+gYour punishment shall be a &+rbeheading&+g, then you will watch as your headless corpse is &+Rdevoured&+g.'&n\n\r",
      J_NAME(ch), J_NAME(pl) );
    for( d = descriptor_list; d; d = d->next )
    {
      if( d->connected == CON_PLAYING )
      {
        send_to_char(buf, d->character);
        write_to_pc_log(d->character, buf, LOG_PRIVATE);
      }
    }
    snprintf(buf, MAX_STRING_LENGTH, "&+wYou barely have time to flinch as a &+WBIG&+w hand karate chops your head into the air."
      "  You see the &+Csky&+w, the &+yground&+w, the &+Csky&+w again, and then feel someone grab your hair."
      "  Your eyes begin to focus on your corpse, still standing.  Suddenly, a large &+Rmouth&+w appears and"
      " begins chewing you up and swallowing.  Soon there is nothing left, the grip on your hair is released,"
      " and things begin to &+Lfade out...\n\r&+LYou hear a thump.&n\n\r" );
    send_to_char(buf, pl);
    write_to_pc_log(pl, buf, LOG_PRIVATE);
    // Remove / Drop / Hide headgear
    if( pl->equipment[WEAR_HEAD] && (headgear = unequip_char(pl, WEAR_HEAD)) )
    {
      obj_to_room( headgear, pl->in_room );
      // And hide the headgear.. the only item that will remain, fallen from the decapitated head.
      SET_BIT( headgear->extra_flags, ITEM_SECRET );
    }
    // Create / Drop head in room.
    head = read_object(8, VIRTUAL);
    head->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3);
    snprintf(buf, MAX_STRING_LENGTH, "head bloody %s", J_NAME(pl) );
    head->name = str_dup( buf );
    snprintf(buf, MAX_STRING_LENGTH, "&+YThe &+rbloody&+Y head of &+R%s&+Y lies here.&n", J_NAME(pl) );
    head->description = str_dup( buf );
    snprintf(buf, MAX_STRING_LENGTH, "&+Ythe &+rbloody&+Y head of &+R%s&n", J_NAME(pl) );
    head->short_description = str_dup( buf );
    snprintf(buf, MAX_STRING_LENGTH, "&+Ythe &+rbloody&+Y head of cheater &+R%s&n", J_NAME(pl) );
    head->action_description = str_dup( buf );
    obj_to_room( head, pl->in_room );

    // Check to see if they have any eq
    naked = TRUE;
    if( IS_CARRYING_N(pl) > 0 )
    {
      naked = FALSE;
    }
    else
    {
      for( i = 0; i < MAX_WEAR; i++ )
      {
        if( pl->equipment[i] )
        {
          naked = FALSE;
          break;
        }
      }
    }
    // Yes, naked cheaters lose a level instead of gear...
    if( naked && GET_LEVEL(pl) > 1 )
    {
      lose_level( pl );
    }

    // Slay the bastard
    snprintf(buf, MAX_STRING_LENGTH, "%s", J_NAME(pl) );
    die( pl, pl );
    // Destroy the corpse including gear... oh no.. my arti.. :P
    corpse = world[head->loc.room].contents;
    while( corpse )
    {
      if( corpse->type == ITEM_CORPSE && isname( buf, corpse->name ) )
      {
        debug( "very_angry_npc: Extracting corpse and eq: %s", corpse->name );
        extract_obj( corpse, TRUE ); // Yes, do handle arti code.
        return TRUE;
      }
      corpse = corpse->next_content;
    }
    debug( "very_angry_npc: Couldn't find corpse (%s)! :(", buf );
    return FALSE;
  }

  if( cmd == CMD_TOROOM )
  {
    if( GET_PID(pl) == 5 )
    {
      snprintf(buf, MAX_STRING_LENGTH, "I remember you, %s.", J_NAME(pl) );
      do_say( ch, buf, CMD_SAY );
      // We can return FALSE _only_ because we know they're both still alive.
      return TRUE;
    }
    if( GET_PID(pl) == 32620 )
    {
      snprintf(buf, MAX_STRING_LENGTH, "I remember you, %s.", J_NAME(pl) );
      do_say( ch, buf, CMD_SAY );
      // We can return FALSE _only_ because we know they're both still alive.
      return FALSE;
    }
    else
    {
      snprintf(buf, MAX_STRING_LENGTH, "You, %s, have number %d and are not on my list.", J_NAME(pl), GET_PID(pl) );
      do_say( ch, buf, CMD_SAY );
      // We can return FALSE _only_ because we know they're both still alive.
      return FALSE;
    }
  }

  return FALSE;
}
