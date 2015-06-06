/*
   ***************************************************************************
   *  File: mount.c                                            Part of Duris *
   *  Usage: handle critters for riding.                                       *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "sound.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "graph.h"
#include "ctf.h"

/*
   external variables
 */

extern P_room world;
extern const char *command[];
extern P_char character_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern struct time_info_data time_info;
extern const int rev_dir[];
extern const char *dirs[];

void do_mount(P_char ch, char *argument, int cmd)
{
  char     name[MAX_STRING_LENGTH];
  P_char   mount, rider;
  int      movescost;

  one_argument(argument, name);

  if (*name)
  {
    if (!(mount = get_char_room_vis(ch, name)))
    {
      send_to_char("I see no one by that name here!\r\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("Whom do you wish to ride?\r\n", ch);
    return;
  }

  if (mount == ch)
  {
    send_to_char("Ride on your own back?  How?\r\n", ch);
    return;
  }
  if (IS_MORPH(ch))
  {
    send_to_char("Sadly, you're just not in that kind of shape.\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You try, but fail.\r\n", ch);
    return;
  }
  if (IS_RIDING(ch))
  {
    send_to_char("You are already riding on something.\r\n", ch);
    return;
  }
  if (world[ch->in_room].room_flags & SINGLE_FILE)
  {
    send_to_char("This room is too narrow to ride.\r\n", ch);
    return;
  }
  
// world[ch->in_room].sector_type >= SECT_WATER_SWIM && Feb09 -Lucrot
  if (world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("Are you crazy? You might slip and drown!\r\n", ch);
    return;
  }

  if( IS_THRIKREEN(ch) || IS_DRIDER(ch) )
  {
    send_to_char("You cannot ride.\r\n", ch);
    return;
  }
  if (IS_PC(ch) && IS_DISGUISE_SHAPE(ch))
  {
    send_to_char("You can't ride anything in that form!\r\n", ch);
    return;
  }
  if (IS_PC(mount) && IS_DISGUISE_SHAPE(ch))
  {
    act("It's too difficult to ride on $N.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }
  if (GET_SIZE(ch) > (GET_SIZE(mount) + 1))
  {
    act("$N is too small for you to ride, find something bigger.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  if (IS_PC(mount) && IS_CENTAUR(mount))
  {
    if (IS_CENTAUR(ch))
    {
      send_to_char
        ("Most centaurs do that in the privacy of their own home..\r\n", ch);
      return;
    }

    if (!is_linked_to(ch, mount, LNK_CONSENT) || IS_THRIKREEN(ch))
    {
      send_to_char("They don't seem to want you on their back..\r\n", ch);
      return;
    }
    /* Saddle Check */
    if (!mount->equipment[WEAR_HORSE_BODY])
    {                           /* nothing worn there */
      send_to_char("Ride without a saddle?  I think not...\r\n", ch);
      return;
    }
  }
  else if(IS_CENTAUR(ch) || has_innate(ch, INNATE_HORSE_BODY)
    || has_innate(ch, INNATE_SPIDER_BODY) )
  {
    send_to_char("It's a tad hard for you to mount much of anything.\r\n",
                 ch);
    return;
  }

  else if (!IS_SET(mount->specials.act, ACT_MOUNT))
  {
    if((GET_RACE(mount) == RACE_ANIMAL) ||
	(GET_RACE(mount) == RACE_AQUATIC_ANIMAL) || 
	(GET_RACE(mount) == RACE_QUADRUPED) ||
	(GET_RACE(mount) == RACE_PRIMATE)  ||
	(GET_RACE(mount) == RACE_HERBIVORE)  ||
       (GET_RACE(mount) == RACE_CARNIVORE))
	{
	 int mounttry = (GET_CHAR_SKILL(ch, SKILL_MOUNT) + GET_LEVEL(ch));
        int mountdef = (GET_LEVEL(mount) * 2);
	 if (mountdef > mounttry)
	  {
          act("You attempt to mount $N, but find you are not yet skilled enough to ride that creature.", FALSE, ch, 0, mount, TO_CHAR);
	   return;
	  }
	}
    	else
    	{
    	  act("It's too difficult to ride on $N.", FALSE, ch, 0, mount, TO_CHAR);
    	  return;
   	}
     }
  if (GET_MASTER(mount) &&
     GET_MASTER(mount) != ch &&
     !is_linked_to(ch, GET_MASTER(mount), LNK_CONSENT))
  {
    act("$N does not recognize you and refuses to let you ride $M.", FALSE,
        ch, 0, mount, TO_CHAR);
    return;
  }
  
  rider = get_linking_char(mount, LNK_RIDING);
  
  if (rider && rider != ch)
  {
    act("Someone else is riding on $N.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }
  
  movescost = MAX(1, ((100 - (GET_CHAR_SKILL(ch, SKILL_MOUNT))) / 10));
  
  if (GET_VITALITY(ch) - movescost < 0)
  {
    send_to_char("You're too tired to ride at this point.\r\n", ch);
    return;
  }

  if (IS_FIGHTING(ch) &&
      GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT) / 1.5 < number(0, 100))
  {
    act("Fending off the attackers you try to mount $N, but alas you fail.",
      FALSE, ch, 0, mount, TO_CHAR);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  if (ch->lobj && ch->lobj->Visible_Type())
  {
    send_to_char("You cant ride and deal with what you are moving.\n", ch);
    return;
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  GET_VITALITY(ch) -= movescost;

  if (GET_VITALITY(ch) > GET_MAX_VITALITY(ch))
    GET_VITALITY(ch) = GET_MAX_VITALITY(ch);

  StartRegen(ch, EVENT_MOVE_REGEN);

  link_char(ch, mount, LNK_RIDING);
  act("You climb up and ride $N.", FALSE, ch, 0, mount, TO_CHAR);
  act("$n climbs on and rides $N.", TRUE, ch, 0, mount, TO_NOTVICT);
  act("$n climbs on and rides you.", FALSE, ch, 0, mount, TO_VICT);
  notch_skill(ch, SKILL_MOUNT, 20);
}

/*
   returns true if char is in same room.  Don't want to check ch
   directly because it may be an invalid pointer already.
   i.e.  Someone summons ride away and kills it. -DCL
 */

static int valid_ride(int room, P_char ch)
{
  P_char   i;

  if (room == NOWHERE)
    return FALSE;
  for (i = world[room].people; i; i = i->next_in_room)
  {
    if (IS_NPC(i) && (i == ch))
      return TRUE;
  }
  return FALSE;
}

void do_dismount(P_char ch, char *argument, int cmd)
{
  P_char   mount = get_linked_char(ch, LNK_RIDING);

  if (mount)
  {
    if (valid_ride(ch->in_room, mount))
    {
      if (world[ch->in_room].sector_type >= SECT_WATER_SWIM &&
          world[ch->in_room].sector_type <= SECT_OCEAN &&
          GET_LEVEL(ch) < AVATAR && !IS_AFFECTED(ch, AFF_FLY) &&
          !IS_AFFECTED(ch, AFF_LEVITATE))
        act("Here? That's not too wise.", FALSE, ch, 0, 0, TO_CHAR);
      else
      {
        act("You dismount $N.", FALSE, ch, 0, mount, TO_CHAR);
        act("$n dismounts $N.", FALSE, ch, 0, mount, TO_ROOM);
        SET_POS(ch, POS_STANDING + STAT_NORMAL);
        unlink_char(ch, mount, LNK_RIDING);
      }
    }
    else                        /*
                                   mount not in same room??? How did this
                                   happen?
                                 */
      stop_riding(ch);
  }
  else
    send_to_char("You are not riding anything!\r\n", ch);
}

/*
   update riding info when mount/rider no longer in same room -DCL
 */

void stop_riding(P_char ch)
{
  P_char   mount;

  if (!ch)
    return;

  mount = get_linked_char(ch, LNK_RIDING);
  if (mount)
  {
    act("You realize that you no longer have a rider!",
        FALSE, mount, 0, 0, TO_CHAR);
    act("You stop riding $N.", FALSE, ch, 0, mount, TO_CHAR);
    unlink_char(ch, mount, LNK_RIDING);
  }
}

/* check to make sure mount and rider both exist and are in same room */
bool check_valid_ride(P_char ch)
{
  P_char   mount = get_linked_char(ch, LNK_RIDING);

  if (!ch)
    return FALSE;

  if (valid_ride(ch->in_room, mount))
    if (ch->specials.z_cord == mount->specials.z_cord)
    {
      if (!number(0, 10))
      {
        if (!number(0, 1))
          play_sound(SOUND_HORSE1, NULL, ch->in_room, TO_ROOM);
        else
          play_sound(SOUND_HORSE2, NULL, ch->in_room, TO_ROOM);
      }
      return TRUE;
    }

  stop_riding(ch);

  return FALSE;
}

/******* Following deals with carts/carriages ***********/
/*                                                      */
/* A vehicle is one object, which can be entered, and   */
/* consists of one room. It is powered by one mob.      */
/*                                                      */
/* ITEM_VEHICLE                                         */
/* value[0] Number of occupants                         */
/* value[1] Type: 1) land/cart 2) water/ferry 3) air/?  */


void do_hitch_vehicle(P_char ch, char *arg, int cmd)
{
  char     horse[MAX_STRING_LENGTH], cart[MAX_STRING_LENGTH];
  P_obj    obj;
  P_char   pl;

  send_to_char( "Under construction.\n\r", ch );
	return;

  argument_interpreter(arg, cart, horse);

  if (*cart)
  {
    if (!(obj = get_obj_in_list_vis(ch, cart, world[ch->in_room].contents)))
    {
      send_to_char("I see nothing like that here!\r\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("What do you want to hitch?\r\n", ch);
    return;
  }

  if (*horse)
  {
    if (!(pl = get_char_room_vis(ch, horse)))
    {
      send_to_char("Hitch who?\r\n", ch);
      return;
    } else {
			if((IS_NPC(pl) && GET_MASTER(pl) != ch) || (!IS_NPC(pl) && !is_linked_to(pl,ch,LNK_CONSENT))) {
	      send_to_char("They aren't interested in you hitching anything to them.\n", ch);
  	    return;
			}
		}
  }
  else
  {
    pl=ch;
  }

  if (pl->lobj && pl->lobj->Visible_Type())
  {
    send_to_char("Unhitch from the current burden first.  You can't push or pull more than one thing.\r\n", ch);
    return;
  }
  if (obj->hitched_to)
  {
    send_to_char("That is already someone else's burden.\r\n", ch);
    return;
  }
  if (!HITCHABLE(obj))
  {
    act("You can't pull push or drag $p&n!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  /* What to do about disc? Let's wait till its even written */

  /* Check for weight here */

	if(pl == ch) {
	  act("$n takes up the burden of $N.", TRUE, ch, obj, pl, TO_ROOM);
	  act("You hitch to $N.", FALSE, ch, obj, pl, TO_CHAR);
	} else {
	  act("$n attaches $p to $N.", TRUE, ch, obj, pl, TO_ROOM);
	  act("You hitch $p to $N.", FALSE, ch, obj, pl, TO_CHAR);
	  act("$n attaches $p to you.", FALSE, ch, obj, pl, TO_VICT);
	}
	
  add_linked_object(pl,obj,get_object_link_type(obj));
  obj->hitched_to = pl;
  return;
}

void do_unhitch_vehicle(P_char ch, char *arg, int cmd)
{

  char     horse[MAX_STRING_LENGTH], cart[MAX_STRING_LENGTH];
  P_obj    obj;
  P_char   pl;

  if (!SanityCheck(ch, "unhitch_vehicle"))
    return;

  argument_interpreter(arg, cart, horse);

  if (*cart)
  {
    if (!(obj = get_obj_in_list_vis(ch, cart, world[ch->in_room].contents)))
    {
      send_to_char("I see nothing like that here!\r\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("What do you want to unhitch?\r\n", ch);
    return;
  }
	
  if (*horse)
  {
    if (!(pl = get_char_room_vis(ch, horse)))
    {
      send_to_char("I see noone like that here!\r\n", ch);
      return;
    } else {
			if((IS_NPC(pl) && GET_MASTER(pl) != ch) || (!IS_NPC(pl) && !is_linked_to(pl,ch,LNK_CONSENT))) {
	      send_to_char("They aren't interested in you touching them.\n", ch);
  	    return;
			}
		}
  }
  else
  {
    pl = ch;
  }
  if (!has_linked_object(pl,obj))
  {
    send_to_char("They aren't hitched to that.\r\n", ch);
    return;
  }

  /* something of a kludge, but will work for now   -- NOPE IT WON'T  Dalreth
  if (GET_LEVEL(ch) <= GET_LEVEL(pl))
  {
    act("$N turns towards $n and snarls, for attempting to unhitch $p", FALSE,
        ch, obj, pl, TO_ROOM);
    act("$N turns towards you and snarls, for attempting to unhitch $p",
        FALSE, ch, obj, pl, TO_CHAR);
    act("You turn towards $n and snarl, because $e attempted to unhitch $p",
        FALSE, ch, obj, pl, TO_VICT);
    return;
  }
*/

  /* clear weight here */
	if(pl == ch) {
	  act("$n steps away from $N.", TRUE, ch, obj, pl, TO_ROOM);
	  act("You unhitch from $N.", FALSE, ch, obj, pl, TO_CHAR);
	} else {
	  act("$n detaches $p from $N.", TRUE, ch, obj, pl, TO_ROOM);
	  act("You unhitch $p from $N.", FALSE, ch, obj, pl, TO_CHAR);
	  act("$n detaches $p from you.", FALSE, ch, obj, pl, TO_VICT);
	}

  remove_linked_object(obj);
  return;
}


  /* count chars in vehicle */
int num_char_in_vehicle(P_obj obj)
{
  int      num = 0, room;
  P_char   i;

  room = obj->R_num;

  if (room == NOWHERE)
    return 0;

  for (i = world[room].people; i; i = i->next_in_room)
    num++;

  return num;
}


static struct vehicle_data navi_info[] = {      /* mob start dest1 dest2(opt) dest3(opt) 0 time freq */
  {47000, 157026, 152728, 150777, 154478, 0, 6, 6},
  /* tharnadia->ashrumite->ugta->woodseer->tharnadia */
  {47001, 150777, 154478, 157026, 152728, 0, 6, 6},
  /* ugta->woodseer->tharnadia->ashrumite->ugta */
  {0, 0, 0, 0, 0, 0, 0, 0}
  /* obligatory null bullshit */
};

  /* All wagons/rooms they contain will be in one zone. Loop through
     valid ones at boot up, and assign the procs
   */

void init_wagons(void)
{
  int      Vnum, i;
  P_char   horse;
  P_obj    Wagon;

  return; /* off till needed again */

  /* All these rooms, objects, mobiles are 470xx */
  for (Vnum = 47000; Vnum < 47099; Vnum++)
  {
    if (real_object0(Vnum) > 0)
      obj_index[real_object0(Vnum)].func.obj = wagon;
    if (real_room0(Vnum) > 0)
      world[real_room0(Vnum)].funct = wagon_exit_room;
    if (real_mobile0(Vnum) > 0)
      mob_index[real_mobile0(Vnum)].func.mob = NULL;
  }
  for (i = 0; navi_info[i].mob != 0; i++)
  {
    /* initialize navigational data */
    navi_info[i].mob = navi_info[i].mob;
    navi_info[i].start1 = real_room(navi_info[i].start1);
    navi_info[i].destination1 = real_room(navi_info[i].destination1);
    navi_info[i].destination2 = real_room(navi_info[i].destination2);
    navi_info[i].destination3 = real_room(navi_info[i].destination3);
    /* load the horse and wagon on the fly */
    if ((horse = read_mobile(navi_info[i].mob, VIRTUAL)))
    {
      char_to_room(horse, navi_info[i].start1, 0);
      GET_VITALITY(horse) = 5000;
      if ((Wagon = read_object(navi_info[i].mob, VIRTUAL)))
      {
        obj_to_room(Wagon, horse->in_room);
        /* connect the two... */
				add_linked_object(horse,Wagon,get_object_link_type(Wagon));
      }
    }
  }
}

void check_for_wagon(P_char ch)
{
  int      i;

  for (i = 0; navi_info[i].mob != 0; i++)
    if (navi_info[i].mob == mob_index[GET_RNUM(ch)].virtual_number)
      wagon_pull(ch, i);
  return;
}

int wagon_pull(P_char ch, int mob)
{
  char     Gbuf3[MAX_STRING_LENGTH] = "\0";
  byte     next_step = -1;
  int      dum;

  if ((time_info.hour - navi_info[mob].move_time) % navi_info[mob].freq == 0)
  {
    if (ch->in_room == navi_info[mob].start1)
    {
      navi_info[mob].destination = navi_info[mob].destination1;
    }
    else if ((ch->in_room == navi_info[mob].destination1))
    {
      if (navi_info[mob].destination2)
        navi_info[mob].destination = navi_info[mob].destination2;
      else
        navi_info[mob].destination = navi_info[mob].start1;
    }
    else if ((ch->in_room == navi_info[mob].destination2))
    {
      if (navi_info[mob].destination3)
        navi_info[mob].destination = navi_info[mob].destination3;
      else
        navi_info[mob].destination = navi_info[mob].start1;
    }
    else if ((ch->in_room == navi_info[mob].destination3))
      navi_info[mob].destination = navi_info[mob].start1;
  }

  next_step =
    find_first_step(ch->in_room, navi_info[mob].destination, 0, 0, WAGON_TYPE_WAGON, &dum);

  if ((next_step >= 0) && (next_step < NUM_EXITS))
  {
    strcpy(Gbuf3, dirs[next_step]);
  }
  else
    switch (next_step)
    {
    case BFS_ALREADY_THERE:
      break;
    case BFS_NO_PATH:
      logit(LOG_DEBUG, "BFS_NO_PATH in wagon_pull() mount.c with %s.", GET_NAME(ch));
      break;
    case BFS_ERROR:
      logit(LOG_DEBUG, "BFS_ERROR in wagon_pull() mount.c with %s.", GET_NAME(ch));
      break;
#if 0
    case 0:
      sprintf(Gbuf3, "north");
      break;
    case 1:
      sprintf(Gbuf3, "east");
      break;
    case 2:
      sprintf(Gbuf3, "south");
      break;
    case 3:
      sprintf(Gbuf3, "west");
      break;
    case 4:
      sprintf(Gbuf3, "down");
      break;
    case 5:
      sprintf(Gbuf3, "up");
      break;
#endif
    default:
      fprintf(stderr, "Bug: this line should never be executed.\n");
    }


  command_interpreter(ch, Gbuf3);
  return TRUE;
}

/* proc assigned to the wagon object itself */
int wagon(P_obj obj, P_char ch, int cmd, char *arg)
{
  char     name[MAX_INPUT_LENGTH];
  P_obj    obj_entered;
  int      interior;

  /* check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_ENTER)
    return (FALSE);

  if (GET_LEVEL(ch) > 25)
  {
    act("The wagon is for lowbies my friend", FALSE, ch, obj, 0, TO_CHAR);
    return FALSE;
  }


  one_argument(arg, name);
  obj_entered = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
  if (obj_entered != obj)
    return (FALSE);

  if (!obj || (obj->type != ITEM_VEHICLE))
  {
    return (FALSE);
  }
  else if (obj->value[0] <= num_char_in_vehicle(obj))
  {
    send_to_char("Feeling like a sardine today? There is no more room!\r\n",
                 ch);
  }
  else
  {
    interior = real_room0(obj_index[obj->R_num].virtual_number);
    act("$n climbs on $p.", TRUE, ch, obj, 0, TO_ROOM);
    char_from_room(ch);
    act("You climb on $p.", FALSE, ch, obj, 0, TO_CHAR);
    char_to_room(ch, interior, 0);
    act("$n climbs aboard.", TRUE, ch, 0, 0, TO_ROOM);
  }
  return (TRUE);
}

/* proc attached to the room within the wagon */
int wagon_exit_room(int room, P_char ch, int cmd, char *arg)
{
  P_obj    obj;
  int      rroom;

  if ((cmd != CMD_LOOK) && (cmd != CMD_DISEMBARK) && (cmd != CMD_EXITS))
    return (FALSE);

  for (obj = object_list; obj; obj = obj->next)
    if (obj_index[obj->R_num].virtual_number == world[room].number)
    {
      break;
    }

  if (!obj)
  {
    if (ch)
      send_to_char("Couldn't find obj to leave.  tell a god.\r\n", ch);
    return FALSE;
  }

  if (cmd == CMD_LOOK)
  {
    if (!arg || !*arg || str_cmp(arg, " out"))
      return (FALSE);

    // This should be fine with just the CMD_LOOKAFAR, but I didn't test it to be sure.
//    rroom = ch->in_room;
//    char_from_room(ch);
//    char_to_room(ch, obj->loc.room, -2);
    new_look(ch, NULL, CMD_LOOKAFAR, obj->loc.room);
//    char_from_room(ch);
//    char_to_room(ch, rroom, -2);

    return (TRUE);
  }
  else if (cmd == CMD_DISEMBARK || cmd == CMD_EXITS)
  {
    if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      send_to_char("You're in no position to climb out!\r\n", ch);
      return (TRUE);
    }
    act("You climb out of $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n climbs out.", TRUE, ch, obj, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, obj->loc.room, 0);
    act("$n climbs out of $p.", TRUE, ch, obj, 0, TO_ROOM);
    return (TRUE);
  }
  return FALSE;
}

bool is_natural_mount(P_char ch, P_char mount)
{
    if (IS_GOBLIN(ch) && isname("warg", GET_NAME(mount)))
        return true;
    return false; // TODO
}
