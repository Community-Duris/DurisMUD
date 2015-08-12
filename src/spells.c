/*
   ***************************************************************************
   *  File: spells.c                                           Part of Duris *
   *  Usage: Preprocessing of spells.                                          *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "damage.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "vnum.obj.h"
#include "specs.prototypes.h"
#include "map.h"
#include "disguise.h"
#include "necromancy.h"
#include "ctf.h"

/*
   external variables
 */

extern int get_multicast_chars(P_char leader, int m_class, int min_level);
int      fight_in_room(P_char ch);
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern struct minor_create_struct minor_create_name_list[];
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern P_index obj_index;
extern const char *dirs[], *dirs2[];
extern const int rev_dir[];
extern P_desc descriptor_list;
extern const struct race_names race_names_table[];
extern bool exit_wallable(int room, int dir, P_char ch);
extern bool create_walls(int room, int exit, P_char ch, int level, int type,
                         int power, int decay, char *short_desc, char *desc,
                         ulong flags);


void cast_call_lightning(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj tar_obj)
{
  P_char   next;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
/*
    if (OUTSIDE(ch) && (sector_table[in_weather_sector(ch->in_room)].conditions.precip_rate > 5)) {
*/ if (OUTSIDE(ch))
    {
      spell_call_lightning(level, ch, victim, 0);
    }
    break;
  case SPELL_TYPE_POTION:
/*
    if (OUTSIDE(ch) && (sector_table[in_weather_sector(ch->in_room)].conditions.precip_rate > 5)) {
*/ if (OUTSIDE(ch))
    {
      spell_call_lightning(level, ch, ch, 0);
    }
    break;
  case SPELL_TYPE_SCROLL:
/*
    if (OUTSIDE(ch) && (sector_table[in_weather_sector(ch->in_room)].conditions.precip_rate > 5)) {
*/ if (OUTSIDE(ch))
    {
      if (victim)
        spell_call_lightning(level, ch, victim, 0);
      else if (!tar_obj)
        spell_call_lightning(level, ch, ch, 0);
    }
    break;
  case SPELL_TYPE_STAFF:
/*
    if (OUTSIDE(ch) && (sector_table[in_weather_sector(ch->in_room)].conditions.precip_rate > 5)) {
*/ if (OUTSIDE(ch))
    {
      for (victim = world[ch->in_room].people; victim; victim = next)
      {
        next = victim->next_in_room;
        if (victim != ch)
          spell_call_lightning(level, ch, victim, 0);
      }
    }
    break;
  default:
    logit(LOG_DEBUG, "Serious screw-up in call lightning!");
    break;
  }
}

void cast_control_weather(int level, P_char ch, char *arg, int type,
                          P_char tar_ch, P_obj tar_obj)
{
  char     Gbuf4[MAX_STRING_LENGTH];
  int      var;
  struct sector_data *zone;
  const char *variables[] = {
    "cold", "warm", "wet", "dry", "windy", "calm", "\n"
  };

  switch (type)
  {
  case SPELL_TYPE_SPELL:

    one_argument(arg, Gbuf4);

    var = old_search_block(Gbuf4, 0, strlen(Gbuf4), variables, 0);
    if (var == -1)
    {
      send_to_char
        ("What kind of weather do you want?\r\n(cold, warm, wet, dry, windy, calm)\r\n",
         ch);
      return;
    }
    if (in_weather_sector(ch->in_room) > -1)
      zone = &sector_table[in_weather_sector(ch->in_room)];
    else
    {
      send_to_char("But there is no weather here!\r\n", ch);
      return;
    }

    if ((zone->climate.flags & NON_CONTROLLABLE) ||
        (zone->conditions.flags & WEATHER_CONTROLLED))
    {
      send_to_char
        ("Someone seems to have control already of the weather here.\r\n",
         ch);
      return;
    }

    switch (var - 1)
    {
    case 0:                    /* cold */
      zone->conditions.temp = MAX(zone->conditions.temp - level / 2, -level);
      send_to_weather_sector(ch->in_room,
                             "The temperature suddenly dips.\r\n");
      break;
    case 1:                    /* warm */
      zone->conditions.temp =
        MIN(zone->conditions.temp + level / 2, 45 + level);
      send_to_weather_sector(ch->in_room,
                             "The temperature suddenly rises.\r\n");
      break;
    case 2:                    /* wet */
      if (zone->climate.
          season_precip[(int) get_season(in_weather_sector(ch->in_room))] ==
          SEASON_NO_PRECIP_EVER)
      {
        send_to_char("There is no moisture in the area!\r\n", ch);
        return;
      }
      if (!zone->conditions.precip_rate)
      {
        zone->conditions.precip_rate++;
        if (zone->conditions.temp > 0)
          send_to_weather_sector(ch->in_room,
                                 "A few drops of rain begin to fall.\r\n");
        else
          send_to_weather_sector(ch->in_room,
                                 "A few flakes of snow begin to fall.\r\n");
      }
      else
      {
        zone->conditions.precip_rate += level;
        if (zone->conditions.temp > 0)
          send_to_weather_sector(ch->in_room, "It rains a bit faster.\r\n");
        else
          send_to_weather_sector(ch->in_room,
                                 "The snow comes down a bit harder.\r\n");
      }
      zone->conditions.humidity = MIN(100, zone->conditions.humidity + level);
      break;
    case 3:                    /* dry */
      if (!zone->conditions.humidity && !zone->conditions.precip_rate)
      {
        send_to_char("There's no more moisture left in the air!\r\n", ch);
        return;
      }
      else
      {
        if (zone->conditions.precip_rate)
        {
          zone->conditions.precip_rate =
            MAX(0, zone->conditions.precip_rate - level);
          if (zone->conditions.precip_rate)
          {
            if (zone->conditions.temp > 0)
              send_to_weather_sector(ch->in_room,
                                     "The rain lets up suddenly.\r\n");
            else
              send_to_weather_sector(ch->in_room,
                                     "The snow suddenly lets up a bit.\r\n");
          }
        }
        else
        {
          zone->conditions.humidity =
            MAX(0, zone->conditions.humidity - level);
          send_to_weather_sector(ch->in_room,
                                 "The air feels a bit drier.\r\n");
        }
      }
      break;
    case 4:                    /* windy */
      zone->conditions.windspeed += level / 2;
      send_to_weather_sector(ch->in_room, "The wind picks up.\r\n");
      break;
    case 5:                    /* calm */
      zone->conditions.windspeed -= level;
      if (zone->conditions.windspeed <= 0)
      {
        zone->conditions.windspeed = 0;
        send_to_weather_sector(ch->in_room,
                               "The air seems to come to a stagnant, dead halt.\r\n");
      }
      else
        send_to_weather_sector(ch->in_room, "The wind calms down a bit.\r\n");
      break;
    default:
      break;
    }
    SET_BIT(zone->conditions.flags, WEATHER_CONTROLLED);
    break;
  default:
    logit(LOG_DEBUG, "Serious screw-up in control weather!");
    break;
  }
}


void cast_minor_creation(int level, P_char ch, char *arg, int type,
                         P_char tar_ch, P_obj tar_obj)
{
  int      i;
  sh_int   obj_num;

  obj_num = 0;
  tar_obj = NULL;               /* in case silly thing provides a tar_obj */

  if (!arg)
  {
    send_to_char
      ("You really should focus on what it is you're trying to create.\r\n",
       ch);
    return;
  }
  for (i = 0; minor_create_name_list[i].keyword[0]; i++)
  {
    if (isname(arg, minor_create_name_list[i].keyword))
    {
      obj_num = minor_create_name_list[i].obj_number;
      break;
    }
  }

  if (obj_num == 0)
  {
    send_to_char("You cannot fathom the power needed to create that.\r\n",
                 ch);
    return;
  }
  else
  {
    tar_obj = read_object(obj_num, VIRTUAL);
    if (tar_obj)
      SET_BIT(tar_obj->extra_flags, ITEM_NOSELL);
    else
      logit(LOG_DEBUG, "cast_minor_create(): obj %d not loadable", obj_num);
  }

  spell_minor_creation(level, ch, 0, tar_obj);
}

void cast_channel(int level, P_char ch, char *arg, int type, P_char tar_ch, P_obj tar_obj)
{
  P_char   t_ch, is_head = get_linked_char(ch, LNK_CONSENT);
  P_obj    t_obj;
  int      num_valid_chars = 0, obj_found = FALSE, obj_num;
  int      curr_time = time(NULL);

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!IS_PC(ch))
      return;
    if (ch->only.pc->pc_timer[3] + 7200 > curr_time)
    {
      send_to_char("You have not built up enough energy to summon another diety.\r\n", ch);
      return;
    }
    if (!is_head)
    {                           // caster is the head
      if ((num_valid_chars = get_multicast_chars(ch, CLASS_CLERIC, 51)) < 3)
      {
        send_to_char
          ("You need more participants to begin the channeling.\r\n", ch);
        return;
      }
      else
        t_ch = ch;
    }
    else
    {                           // caster is a participant, is_head is leader
      if ((num_valid_chars =
           get_multicast_chars(is_head, CLASS_CLERIC, 51)) < 4)
      {
        send_to_char
          ("Your channeler needs more participants to begin the channeling.\r\n",
           ch);
        return;
      }
      else
        t_ch = is_head;
    }

    // Ok we have the participants, now check for the object
    if (IS_EVIL(ch))
      obj_num = EVIL_AVATAR_OBJ;
    else
      obj_num = GOOD_AVATAR_OBJ;

    for (t_obj = world[ch->in_room].contents; t_obj;
         t_obj = t_obj->next_content)
    {
      if (obj_index[t_obj->R_num].virtual_number == obj_num)
      {
        obj_found = TRUE;
        break;
      }
    }
    if (obj_found)
    {
      spell_channel(level, ch, t_ch, t_obj);
      return;
    }
    else if (t_ch == ch)
    {
      if (IS_EVIL(ch))
        t_obj = read_object(EVIL_AVATAR_OBJ, VIRTUAL);
      else
        t_obj = read_object(GOOD_AVATAR_OBJ, VIRTUAL);
      if (!t_obj)
      {
        send_to_char
          ("Avatar summoning object missing, please tell a god.\r\n", ch);
        return;
      }
      t_obj->timer[0] = 0;
      obj_to_room(t_obj, ch->in_room);
      act
        ("$n's eyes roll back in $s head as $e begins the incantation... specs of light begin to form in the room.",
         FALSE, ch, 0, 0, TO_ROOM);
      act
        ("Your eyes roll back in your head as you begin the incantation... specs of light begin to form in the room.",
         FALSE, ch, 0, 0, TO_CHAR);
      set_obj_affected(t_obj, 500, TAG_OBJ_DECAY, 0);
      spell_channel(level, ch, t_ch, t_obj);
      return;
    }
    send_to_char("The channeler must begin the incantation.\r\n", ch);
    break;
  case SPELL_TYPE_POTION:
    break;
  case SPELL_TYPE_SCROLL:
    break;
  case SPELL_TYPE_WAND:
    break;
  case SPELL_TYPE_STAFF:
    break;
  default:
    logit(LOG_DEBUG, "Serious screw-up in channel!");
    break;
  }
}

int planes_room_num[] = {
    23801,
    23201,
    12401,
    24401,
    19701,
    25401,
    SURFACE_MAP_START,
    32385,
    26600,
    0
};

const char *planes_name[] = {
    "earth", 
    "water",
    "ethereal", 
    "air", 
    "astral", 
    "fire", 
    "prime", 
    "hell",
    "negative", 
    "\n" };

void cast_gate(int level, P_char ch, char *arg, int type, P_char tar_ch, P_obj tar_obj)
{
  struct portal_settings set = {
      752, /* portal type  */
      -1,  /* from room */
      -1,  /* to room */
      0,   /* How many can pass before closes */
      0,   /* Timeout before anyone can enter after open */
      0,   /* Timeout before next person can enter */
      0,   /* Lag person gets when steps out portal */
      0    /* Portal decay timer */
  };
  struct portal_create_messages msg = {
    /*ch   */ "The portal opens for a brief second and then closes.\r\n",
    /*ch r */ 0,
    /*vic  */ 0,
    /*vic r*/ 0,
    /*ch   */ "&+cA sudden breeze blows by, creating a &+ycloud of dust&+c that quickly condenses into $p&n&+c.",
    /*ch r */ "&+cA sudden breeze blows by, creating a &+ycloud of dust&+c that quickly condenses into $p&n&+c.",
    /*vic  */ "&+cA sudden breeze blows by, creating a &+ycloud of dust&+c that quickly condenses into $p&n&+c.",
    /*vic r*/ "&+cA sudden breeze blows by, creating a &+ycloud of dust&+c that quickly condenses into $p&n&+c.",
    /*npc  */ 0,
    /*bad  */ "Gate is used for interplanar travel, try walking!\n"
  };

  char     Gbuf4[MAX_STRING_LENGTH];
  int      to_room, plane_id, from_zone, from_room;
  
  if((ch && !is_Raidable(ch, 0, 0)) ||
     (tar_ch && !is_Raidable(tar_ch, 0, 0)))
  {
    send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
    return;
  }
  
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    one_argument(arg, Gbuf4);

    plane_id = search_block(Gbuf4, planes_name, FALSE);
    if ((plane_id < 0) || (plane_id > 8))
    {
      send_to_char
        ("You may only travel to the Prime, Negative, Hell, Ethereal, Astral, Earth, Fire, Water or Air planes!\r\n",
         ch);
      return;
    }
    from_room = ch->in_room;
    from_zone = world[from_room].zone;

    if (plane_id != 6)
    {
      if (from_zone ==
          world[MAX(0, real_room(planes_room_num[plane_id]))].zone)
      {
        /*
           trying to use gate as a teleport, nah nah
         */
        send_to_char(msg.bad_target_caster, ch);
        return;
      }

      if (plane_id == 8)
        to_room = real_room(number(26601, 26681));
      else if (plane_id == 7)
        to_room = real_room(number(32101, 32399));
      else
        do
        {
          to_room = get_room_in_zone(planes_room_num[plane_id], ch);
        }
        while (zone_table[world[to_room].zone].flags & ZONE_CLOSED);
    }
    else
    {
      if ((from_zone == world[MAX(0, real_room(SURFACE_MAP_START))].zone) || 
          (from_zone == world[MAX(0, real_room(UD_MAP_START))].zone))
      {
        /* trying to use gate as a teleport, nah nah  */
        send_to_char(msg.bad_target_caster, ch);
        return;
      }

      do
      {
        to_room = number(real_room(planes_room_num[6]), top_of_world);
      }
      while ((world[MAX(0, real_room(planes_room_num[1]))].zone ==
              world[to_room].zone) ||
             (world[to_room].sector_type == SECT_OCEAN) ||
             (world[to_room].sector_type == SECT_FIREPLANE) ||
             (world[to_room].zone == 83) || (world[to_room].zone == 260) ||
             (world[to_room].sector_type == SECT_AIR_PLANE) ||
             (world[to_room].sector_type == SECT_MOUNTAIN) || /* mountains are no-walk on maps nowdays*/
             (world[to_room].sector_type == SECT_UNDRWLD_MOUNTAIN) || /* underworld mountains double so */
             (zone_table[world[to_room].zone].flags & ZONE_CLOSED) ||
             (!IS_MAP_ROOM(to_room))    //not a map room
          );
    }

    if ((to_room == NOWHERE) || (to_room == from_room) ||
        IS_SET(world[to_room].room_flags, NO_MAGIC) ||
        IS_SET(world[from_room].room_flags, NO_GATE) ||
        IS_SET(world[to_room].room_flags, NO_GATE) ||
        IS_HOMETOWN(to_room) ||
        IS_HOMETOWN(from_room))
    {
      send_to_char(msg.fail_to_caster, ch);
      return;
    }

    if (is_prime_plane(from_room) &&
        is_prime_plane(to_room) &&
	!IS_MAP_ROOM(from_room))
    {
      send_to_char(msg.fail_to_caster, ch);
      return;
    }

  set.to_room = to_room;
  set.throughput         = get_property("portals.gate.maxToPass", -1);
  set.init_timeout       = get_property("portals.gate.initTimeout", 0);
  set.post_enter_timeout = get_property("portals.gate.postEnterTimeout", 0);
  set.post_enter_lag     = get_property("portals.gate.postEnterLag", 0);
  set.decay_timer        = get_property("portals.gate.decayTimeout", 240);

  if( spell_general_portal(level, ch, 0, &set, &msg) )
  {
    sprintf(Gbuf4, "A gateway is opened to the %s plane!\r\n", planes_name[plane_id]);
    send_to_char(Gbuf4, ch);
  }

    break;
  default:
    logit(LOG_DEBUG, "Serious screw-up in gate!");
    break;
  }
}

void cast_nether_gate(int level, P_char ch, char *arg, int type,
                      P_char tar_ch, P_obj tar_obj)
{
  struct portal_settings set = {
      781, /* portal type  */
      -1,  /* from room */
      -1,  /* to room */
      0,   /* How many can pass before closes */
      0,   /* Timeout before anyone can enter after open */
      0,   /* Timeout before next person can enter */
      0,   /* Lag person gets when steps out portal */
      0    /* Portal decay timer */
  };
  struct portal_create_messages msg = {
    /*ch   */ "The portal opens for a brief second and then closes.\r\n",
    /*ch r */ 0,
    /*vic  */ 0,
    /*vic r*/ 0,
    /*ch   */ "&+LA dark &n&+brift&+L peels through reality, accompanied by the &n&+gstench&+L of &+Rhell.",
    /*ch r */ "&+LA dark &n&+brift&+L peels through reality, accompanied by the &n&+gstench&+L of &+Rhell.",
    /*vic  */ "&+LA dark &n&+brift&+L peels through reality, accompanied by the &n&+gstench&+L of &+Rhell.",
    /*vic r*/ "&+LA dark &n&+brift&+L peels through reality, accompanied by the &n&+gstench&+L of &+Rhell.",
    /*npc  */ 0,
    /*bad  */ "Nether gate is used for interplanar travel, try walking!\n"
  };

  char     Gbuf4[MAX_STRING_LENGTH];
  int      to_room, plane_id, from_zone, from_room;

  if((ch && !is_Raidable(ch, 0, 0)) ||
     (tar_ch && !is_Raidable(tar_ch, 0, 0)))
  {
    send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
    return;
  }
  
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    one_argument(arg, Gbuf4);

    plane_id = search_block(Gbuf4, planes_name, FALSE);
    if ((plane_id < 0) || (plane_id > 8))
    {
      send_to_char
        ("You may only travel to the Negative, Hell, Ethereal, Astral, Earth, Fire, Water or Air planes!\r\n",
         ch);
      return;
    }
    from_room = ch->in_room;
    from_zone = world[from_room].zone;

    if (plane_id != 6)
    {
      if (from_zone ==
          world[MAX(0, real_room(planes_room_num[plane_id]))].zone)
      {
        /*
           trying to use gate as a teleport, nah nah
         */
        send_to_char(msg.bad_target_caster, ch);
        return;
      }
      if (plane_id == 8)
        to_room = real_room(number(26601, 26681));
      else
        to_room = get_room_in_zone(planes_room_num[plane_id], ch);
    }
    else
    {
      if ((from_zone == world[MAX(0, real_room(SURFACE_MAP_START))].zone) || 
          (from_zone == world[MAX(0, real_room(UD_MAP_START))].zone))
      {
        /*
           trying to use gate as a teleport, nah nah
         */
        send_to_char(msg.bad_target_caster, ch);
        return;
      }
      do
      {
        to_room = number(real_room(planes_room_num[3]), top_of_world);
      }
      while ((world[MAX(0, real_room(planes_room_num[1]))].zone ==
              world[to_room].zone) ||
             (world[to_room].sector_type == SECT_OCEAN) ||
             (world[to_room].sector_type == SECT_FIREPLANE) ||
             (world[to_room].zone == 83) || (world[to_room].zone == 260) ||
             (world[to_room].sector_type == SECT_AIR_PLANE));
    }

    if ((to_room == NOWHERE) || (to_room == from_room) ||
        IS_SET(world[from_room].room_flags, NO_GATE) ||
        IS_SET(world[to_room].room_flags, NO_GATE) ||
        IS_HOMETOWN(to_room) ||
        IS_HOMETOWN(from_room))
    {
      send_to_char(msg.fail_to_caster, ch);
      return;
    }
    
  set.to_room = to_room;
  set.throughput         = get_property("portals.gate.maxToPass", -1);
  set.init_timeout       = get_property("portals.gate.initTimeout", 0);
  set.post_enter_timeout = get_property("portals.gate.postEnterTimeout", 0);
  set.post_enter_lag     = get_property("portals.gate.postEnterLag", 0);
  set.decay_timer        = get_property("portals.gate.decayTimeout", 240);

  if( spell_general_portal(level, ch, 0, &set, &msg) )
  {
    sprintf(Gbuf4, "A gateway is opened to the %s plane!\r\n", planes_name[plane_id]);
    send_to_char(Gbuf4, ch);
  }

    break;
  default:
    logit(LOG_DEBUG, "Serious screw-up in nether gate!");
    break;
  }
}

int char_is_on_plane(P_char ch)
{
  int      i;
  
  if(!(ch))
  {
    logit(LOG_EXIT, "char_is_on_plane called in spells.c without ch");
    raise(SIGSEGV);
  }
  if(ch)
  {
    if(IS_ALIVE(ch))
    {
      for (i = 0; i < 9; i++)
      {
        if (i == 6)
        {
          continue;
        }
        if(world[ch->in_room].zone ==
            world[MAX(0, real_room(planes_room_num[i]))].zone)
        {
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

void cast_plane_shift(int level, P_char ch, char *arg, int type, P_char tar_ch, P_obj tar_obj)
{
  char     Gbuf4[MAX_STRING_LENGTH];
  int      to_room, plane_id, from_zone, from_room;

  if(!(ch))
  {
    logit(LOG_EXIT, "cast_plane_shift called in spells.c without ch");
    raise(SIGSEGV);
  }

  if(!IS_ALIVE(ch))
  {
    send_to_char("The dead do not shift!\r\n", ch);
    return;
  }

  if( !is_Raidable(ch, 0, 0) || (tar_ch && !is_Raidable(tar_ch, 0, 0)))
  {
    send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
    return;
  }

  switch( type )
  {
    case SPELL_TYPE_SPELL:
      one_argument(arg, Gbuf4);

      if((GET_CLASS(ch, CLASS_DRUID) || (IS_MULTICLASS_PC(ch)
        && GET_SECONDARY_CLASS(ch, CLASS_DRUID))) && GET_LEVEL(ch) < 41)
      {
        send_to_char("You must reach level 41 to use this ability.\r\n", ch);
        return;
      }
/*
*   Allowing mortals to shift, gate, word and well from ocean tiles
*   to encourage naval battles: 22Aug08 Lucrot
*
*   if (world[ch->in_room].sector_type == SECT_OCEAN)
*    {
*      send_to_char("Chant such a complex spell while swimming?\r\n", ch);
*      return;
*    }
*/
// plane_id
// 0    earth,
// 1    water,
// 2    ethereal,
// 3    air,
// 4    astral,
// 5    fire,
// 6    prime,
// 7    hell,
// 8    negative,

      plane_id = search_block(Gbuf4, planes_name, FALSE);

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

      if( plane_id == 6 && char_is_on_plane(ch) && !IS_MULTICLASS_PC(ch)
        && (GET_PRIME_CLASS(ch, CLASS_DRUID) || GET_PRIME_CLASS(ch, CLASS_BLIGHTER)) )
      {
        act("$n slowly fades away...", 0, ch, 0, 0, TO_ROOM);
        char_from_room(ch);
        sprintf(Gbuf4, "You materialize on the %s plane!\r\n", planes_name[plane_id]);
        send_to_char(Gbuf4, ch);
        char_to_room(ch, real_room(GET_BIRTHPLACE(ch)), 0);
        act("$n slowly materializes...", 0, ch, 0, 0, TO_ROOM);
        return;
      }
      if( (plane_id < 0) || (plane_id > 8) )
      {
        send_to_char("Negative, Ethereal, Astral, Air, Water, Fire, Earth and Prime are the only valid targets!\r\n", ch);
        return;
      }
      from_zone = world[ch->in_room].zone;

      if(plane_id != 6)
      {
        if(from_zone == world[MAX(0, real_room(planes_room_num[plane_id]))].zone)
        {
          send_to_char("Plane shift is used for interplanar travel, try walking!\n", ch);
          return;
        }
        if(plane_id == 8)
        {
          to_room = real_room(number(26601, 26681));
        }
        else
        {
          do
          {
            to_room = get_room_in_zone(planes_room_num[plane_id], ch);
          }
          while (zone_table[world[to_room].zone].flags & ZONE_CLOSED);
        }
      }
      else
      {
        // If not on water plane, fire plane, or air plane.. gotta walk.
        if( (from_zone != world[MAX(0, real_room(planes_room_num[1]))].zone)
          && (world[ch->in_room].sector_type != SECT_FIREPLANE)
          && (world[ch->in_room].sector_type != SECT_AIR_PLANE) )
        {
          /*  trying to use plane shift as a teleport, nah nah  */
          send_to_char("&+WPlane shift is used for interplanar travel, try walking!&n\n", ch);
          return;
        }
        do
        {
          to_room = number(real_room(planes_room_num[6]), top_of_world);
        }
        while ((world[MAX(0, real_room(planes_room_num[1]))].zone ==
                world[to_room].zone) ||
               (world[to_room].sector_type == SECT_OCEAN) ||
               (world[to_room].sector_type == SECT_FIREPLANE) ||
               (world[to_room].zone == 83) ||
               (world[to_room].zone == 260) ||
               (world[to_room].sector_type == SECT_AIR_PLANE) ||
               (world[to_room].sector_type == SECT_MOUNTAIN) || /* mountains are no-walk on maps nowdays*/
               (world[to_room].sector_type == SECT_UNDRWLD_MOUNTAIN) || /* underworld mountains double so */
               (zone_table[world[to_room].zone].flags & ZONE_CLOSED) ||
               (!IS_MAP_ROOM(to_room))    //not a map room
            );
      }

      if((to_room == NOWHERE) ||
        (to_room == ch->in_room) ||
        IS_SET(world[to_room].room_flags, NO_MAGIC) ||
        IS_SET(world[ch->in_room].room_flags, NO_GATE) ||
        IS_SET(world[to_room].room_flags, NO_GATE) ||
        IS_HOMETOWN(to_room) ||
        IS_HOMETOWN(ch->in_room))
      {
        send_to_char("Strange... nothing happens.\r\n", ch);
        return;
      }
      act("$n slowly fades away...", 0, ch, 0, 0, TO_ROOM);
      char_from_room(ch);
      sprintf(Gbuf4, "You materialize in the %s plane!\r\n", planes_name[plane_id]);
      send_to_char(Gbuf4, ch);
      char_to_room(ch, to_room, 0);
      act("$n slowly materializes...", 0, ch, 0, 0, TO_ROOM);
      break;
    default:
      logit(LOG_DEBUG, "Serious screw-up in plane shift!");
      break;
  }
}

void cast_area_resurrect(int level, P_char ch, char *arg, int type,
                         P_char tar_ch, P_obj tar_obj)
{

  P_obj    t_obj;
  P_desc   d;
  P_obj    obj;
  int      i = 0;

  if (!ch)
    return;

  if (fight_in_room(ch))
  {
    send_to_char("There is no way to focus enough in this room!!\r\n", ch);
    return;
  }

  send_to_char("&+rYou silently prepare for the incantation, as it will surely draw you close to &+Ldeath&+r.\r\n", ch);

  for (obj = world[ch->in_room].contents; obj; obj = t_obj)
  {
    t_obj = obj->next_content;
    if (obj->type == ITEM_CORPSE && IS_SET(obj->value[1], PC_CORPSE))
    {
      spell_resurrect(level, ch, 0, 0, 0, obj);
      i++;
    }
  }

  if (IS_TRUSTED(ch))
    return;

  GET_HIT(ch) = (20 - i);
  update_pos(ch);

  if (i > 19)
  {
    send_to_char("&+rThe strain on your body is too much, and the &+Ccold &+rembrace of &+Ldeath&+r welcomes you.&n\r\n", ch);
    die(ch, ch);
  }
  else
  {
    send_to_char
      ("&+LThe strain of returning your comrade's souls to their bodies is almost too much...&n\r\n", ch);
    CharWait(ch, i * PULSE_VIOLENCE);
  }
}

void cast_wall_of_flames(int level, P_char ch, char *arg, int type,
                         P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (IS_SET(world[ch->in_room].room_flags, UNDERWATER))
  {
    send_to_char("Fire has a problem staying lit underwater...", ch);
    return;
  }

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, WALL_OF_FLAMES, dice(level / 4, 10), 1800,
    "&+Ra billowing wall of flames&n",
    "&+RA towering wall of flames is here to the %s.&n", 0) )
  {
    sprintf(buf1,
            "&+RYou feel a blast of heat as a huge wall of flames bursts to the %s!&n\r\n",
            dirs[var]);
    sprintf(buf2,
            "&+RYou feel a blast of heat as a huge wall of flames bursts to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void cast_wall_of_ice(int level, P_char ch, char *arg, int type,
                      P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, WALL_OF_ICE, 40, 1800,
    "&+Wa wall of ice&N",
    "&+WA huge block of solid ice is here to the %s.&n", 0) )
  {
    SET_BIT(EXIT(ch, var)->exit_info, EX_BREAKABLE);
    SET_BIT(VIRTUAL_EXIT
            ((world[ch->in_room].dir_option[var])->to_room,
             rev_dir[var])->exit_info, EX_BREAKABLE);

    sprintf(buf1,
            "&+WYou feel a gust of cold as a huge block of ice forms to the %s!&n\r\n",
            dirs[var]);
    sprintf(buf2,
            "&+WYou feel a gust of cold as a huge block of ice forms to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void cast_life_ward(int level, P_char ch, char *arg, int type, P_char tar_ch,
                    P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, LIFE_WARD, 40, 1800,
    "&+La wall of &n&+bnegative energy&n",
    "&+LA strange blackness cloaks the %s exit here.&n", 0) )
  {
    sprintf(buf1, "&+LA wall of blackness begins to spread to the %s!&n\r\n",
            dirs[var]);
    sprintf(buf2, "&+LA wall of blackness begins to spread to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void cast_wall_of_stone(int level, P_char ch, char *arg, int type,
                        P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, WALL_OF_STONE, level, 1800,
    "&+La greyish stone wall&N",
    "&+LA greyish stone wall is here to the %s.&n", 0) )
  {

    SET_BIT(EXIT(ch, var)->exit_info, EX_BREAKABLE);
    SET_BIT(VIRTUAL_EXIT
            ((world[ch->in_room].dir_option[var])->to_room,
             rev_dir[var])->exit_info, EX_BREAKABLE);

    sprintf(buf1, "&+LA block of grey stone forms to the %s!&n\r\n",
            dirs[var]);
    sprintf(buf2, "&+LA block of grey stone forms to the %s!&n\r\n",
            dirs[rev_dir[var]]);
    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void cast_wall_of_iron(int level, P_char ch, char *arg, int type,
                       P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, WALL_OF_IRON, level, 1800,
    "&+ya massive wall of iron&N",
    "&+yA massive wall of iron is here to the %s.&n", 0) )
  {
    SET_BIT(EXIT(ch, var)->exit_info, EX_BREAKABLE);
    SET_BIT(VIRTUAL_EXIT
            ((world[ch->in_room].dir_option[var])->to_room,
             rev_dir[var])->exit_info, EX_BREAKABLE);

    sprintf(buf1, "&+yA massive iron wall forms to the %s!&n\r\n", dirs[var]);
    sprintf(buf2, "&+yA massive iron wall forms to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void cast_wall_of_force(int level, P_char ch, char *arg, int type,
                        P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, WALL_OF_FORCE, level, 1800,
    "&+Wa wall of force&n",
    "&+WThe outline of some wall is here to the %s.&n", ITEM_INVISIBLE) )
  {
    SET_BIT(EXIT(ch, var)->exit_info, EX_BREAKABLE);
    SET_BIT(VIRTUAL_EXIT
            ((world[ch->in_room].dir_option[var])->to_room,
             rev_dir[var])->exit_info, EX_BREAKABLE);

    sprintf(buf1, "&+WThe air swirls and thickens to the %s!&n\r\n",
            dirs[var]);
    sprintf(buf2, "&+WThe air swirls and thickens to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void cast_wall_of_bones(int level, P_char ch, char *arg, int type, P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
           Gbuf4[MAX_STRING_LENGTH], Gbuf5[MAX_STRING_LENGTH];
  int      var = 0;
  int      clevel = 0;
  int      scales = 0;
  P_obj    corpse = NULL;
  P_obj    obj_in_corpse, next_obj;
  

  arg = one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);
  
  if (var != -1)
  {
    if (EXIT(ch, var))
      scales = get_spell_component(ch, DRAGONSCALE_VNUM, 4) * number(1, 2);
    else
    {
      send_to_char("You see no exit in that direction!\r\n", ch);
      return;
    }
      
  } 
  else
  {
    tar_obj = get_obj_in_list_vis(ch, Gbuf4, world[ch->in_room].contents);
    
    if (tar_obj && tar_obj->type == ITEM_CORPSE)
    {
      corpse = tar_obj;
	
      clevel = corpse->value[CORPSE_LEVEL];
     
      if (IS_SET(corpse->value[CORPSE_FLAGS], PC_CORPSE) && (clevel < 0))
        clevel = -clevel;

      if (clevel > (level + 4) &&
        !IS_TRUSTED(ch) &&
        (level < 49))
      {
        act("You are not powerful enough to transform that corpse!", FALSE, ch, 0, 0, TO_CHAR);
        return;
      }
	
	    if (clevel < 46)
      {
  	    act("This spell requires the corpse of a more powerful being!", FALSE, ch, 0, 0, TO_CHAR);
        return;
      }
      
      arg = one_argument(arg, Gbuf5);
      var = dir_from_keyword(Gbuf5);
    }
  }

  
  if(!corpse && !scales)
  {
    send_to_char("You need some flesh and bones to transform!\nAt the very last, even some dragonscale would suffice.\n", ch);
    act("&+L$n's&+L spell fizzles and dies.\n", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  
  if (!exit_wallable(ch->in_room, var, ch))
  {
    send_to_char("You cannot block that direction.\n", ch);
    act("&+L$n's&+L spell fizzles and dies.\n", TRUE, ch, 0, 0, TO_ROOM);
	return;
  }

  
  if( corpse && create_walls(ch->in_room, var, ch, level, WALL_OF_BONES, level, 1800,
    "&+La wall of &+wbones&n",
    "&+LA large wall of &+wbones&+L is here to the %s.&n", 0) )
  {

    SET_BIT(EXIT(ch, var)->exit_info, EX_BREAKABLE);
    SET_BIT(VIRTUAL_EXIT((world[ch->in_room].dir_option[var])->to_room, rev_dir[var])->exit_info, EX_BREAKABLE);

    sprintf(buf1, "&+LInfused by a powerful magic, %s &+Lmagically transforms into a pile of bones, blocking the %s!&n\r\n",
	        corpse->short_description, dirs[var]);
    sprintf(buf2, "&+LA pile of bones magically assembles to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);

    for (obj_in_corpse = corpse->contains; obj_in_corpse; obj_in_corpse = next_obj)
    {
      next_obj = obj_in_corpse->next_content;
      obj_from_obj(obj_in_corpse);
      obj_to_room(obj_in_corpse, ch->in_room);
    }

    extract_obj(corpse, TRUE); // Empty corpse, but 'in game.'
  }
  else if( scales && create_walls(ch->in_room, var, ch, level, WALL_OF_BONES, scales, 1000,
    "&+La thin wall of &+gscales&n",
    "&+LA thin wall of &+gscales&+L is here to the %s.&n", 0) )
  {

    SET_BIT(EXIT(ch, var)->exit_info, EX_BREAKABLE);
    SET_BIT(VIRTUAL_EXIT((world[ch->in_room].dir_option[var])->to_room, rev_dir[var])->exit_info, EX_BREAKABLE);

    sprintf(buf1, "&+LInfused by powerful sorcery, some &+gdragonscales &+Lmagically transform into a delicate yet solid curtain, blocking exit to the %s!&n\r\n",
            dirs[var]);
    sprintf(buf2, "&+LA thin &+gdragonscale&+L curtain magically assembles to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void cast_lightning_curtain(int level, P_char ch, char *arg, int type,
                            P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, LIGHTNING_CURTAIN, dice(level / 4, 10),
    1800, "&+Ya crackling curtain of lightning&n",
    "&+BA rippling curtain of lightning crackles to the %s.&n", 0) )
  {
    sprintf(buf1, "&+BYou see an electrical surge to the %s!&n\r\n",
            dirs[var]);
    sprintf(buf2, "&+BYou see an electrical surge to the %s!&n\r\n",
            dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}

void cast_web(int level, P_char ch, char *arg, int type, P_char tar_ch,
              P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    Gbuf4[MAX_STRING_LENGTH];
  int      var = 0;

  one_argument(arg, Gbuf4);
  var = dir_from_keyword(Gbuf4);

  if (!exit_wallable(ch->in_room, var, ch))
  {
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, WEB, 0, 1800, "&+Wa sticky web&n",
    "A large web appears %s!", 0) )
  {
    sprintf(buf1, "A large web appears %s!\r\n", dirs[var]);
    sprintf(buf2, "A large web appears %s!\r\n", dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}


void cast_prismatic_cube(int level, P_char ch, char *arg, int type,
                         P_char tar_ch, P_obj tar_obj)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int      dir, room, in_room;
  
  if(!(ch) ||
     !IS_ALIVE(ch) ||
     !(room = ch->in_room))
        return;

  for (dir = 0; dir < NUM_EXITS; dir++)
  {
    if (!exit_wallable(room, dir, NULL))
    {
      continue;
    }

    if( create_walls(room, dir, ch, level, PRISMATIC_WALL, level, 1800,
      "a prismatic wall",
      "A &+rw&N&+ca&N&+bl&N&+yl&N of &+gs&N&+Rh&N&+Ci&N&+Bf&N&+Yt&N&+Gi&N&+rn&N&+Cg&N &+bc&N&+yo&N&+Gl&N&+Ro&N&+cr&N is here to the %s.", 0) )
    {
      sprintf(buf1,
              "A &+rw&N&+ca&N&+bl&N&+yl&N of &+gs&N&+Rh&N&+Ci&N&+Bf&N&+Yt&N&+Gi&N&+rn&N&+Cg&N &+bc&N&+yo&N&+gl&N&+ro&N&+cr&N appears to the %s!\r\n",
              dirs[dir]);
      sprintf(buf2,
              "A &+rw&N&+ca&N&+bl&N&+yl&N of &+gs&N&+Rh&N&+Ci&N&+Bf&N&+Yt&N&+Gi&N&+rn&N&+Cg&N &+bc&N&+yo&N&+gl&N&+ro&N&+cr&N appears to the %s!\r\n",
              dirs[rev_dir[dir]]);

      send_to_room(buf1, room);
      send_to_room(buf2, (world[room].dir_option[dir])->to_room);
    }
  }
}

void event_earthen_tomb(P_char ch, P_char victim, P_obj obj, void *data)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int      exit, room;
  int      available_exits, picked, i;

  room = *((int *) data);

  if (!get_spell_from_room(&world[room], SPELL_EARTHEN_TOMB))
  {
    return;
  }

  if (number(1, 10) < 5)
  {
    send_to_room
      ("&+yThe ground &+Lr&+yu&+Lm&+ybl&+ye&+Ls &+yand quivers under your feet.&n\r\n",
       room);
    add_event(event_earthen_tomb, PULSE_VIOLENCE, 0, 0, 0, 0, &room,
              sizeof(room));
    return;
  }

  available_exits = 0;

  for (i = 0; i < NUM_EXITS; i++)
  {
    if (exit_wallable(room, i, NULL) && i != UP && i != DOWN)
    {
      available_exits++;
    }
  }

  if (available_exits == 0)
  {
    send_to_room
      ("&+yThe ground &+Lr&+yu&+Lm&+ybl&+ye&+Ls &+yand quivers under your feet.&n\r\n",
       room);
    add_event(event_earthen_tomb, (int) (PULSE_VIOLENCE * 1.5), 0, 0, 0, 0,
              &room, sizeof(room));
    return;
  }

  picked = number(0, available_exits - 1);

  for (i = 0; i < NUM_EXITS; i++)
  {
    if (exit_wallable(room, i, NULL) && i != UP && i != DOWN)
    {
      if (!picked--)
      {
        exit = i;
        break;
      }
    }
  }

  if( create_walls(room, exit, NULL, 50, WALL_OF_STONE, 50, 1800, "&+yAn earthen wall&n",
    "&+yAn earthen wall blocks the exit to the %s.&n", 0) )
  {
    sprintf(buf1,
            "&+ySuddenly a tall earthen wall rises from the ground blocking the exit to the %s!&n\r\n",
            dirs[exit]);
    sprintf(buf2,
            "&+ySuddenly a tall earthen wall rises from the ground blocking the exit to the %s!&n\r\n",
            dirs[rev_dir[exit]]);

    send_to_room(buf1, room);
    send_to_room(buf2, (world[room].dir_option[exit])->to_room);
  }
  add_event(event_earthen_tomb, PULSE_VIOLENCE, 0, 0, 0, 0, &room,
            sizeof(room));
}

void cast_earthen_tomb(int level, P_char ch, char *arg, int type,
                       P_char tar_ch, P_obj tar_obj)
{
  struct room_affect af;

  if (!HAS_FOOTING(ch))
  {
    send_to_char("There is no ground here to raise!\r\n", ch);
    return;
  }

  if (get_spell_from_room(&world[ch->in_room], SPELL_EARTHEN_TOMB))
  {
    send_to_char("The ground here is all shakin already!\r\n", ch);
    return;
  }

  send_to_room
    ("&+ySuddenly the earth beneath your feet starts &+Lr&+yu&+Lm&+ybl&+yi&+Ln&+yg and moving!&n\r\n",
     ch->in_room);
  memset(&af, 0, sizeof(struct room_affect));
  af.type = SPELL_EARTHEN_TOMB;
  af.duration = number(8, 10) * PULSE_VIOLENCE;
  affect_to_room(ch->in_room, &af);
  add_event(event_earthen_tomb, PULSE_VIOLENCE, 0, 0, 0, 0, &ch->in_room,
            sizeof(ch->in_room));
}

/*
 * Start Druid Specs
 */

struct grow_data
{
  int      room;
  byte     old_sect;
  ulong    flags;
  event_func_type func_bye;
  int      skill;
  int      duration;
};

//-------------------------------------------------------------------------------
void event_transmute_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   if(g_data && g_data->func_bye)
   {
      g_data->func_bye(ch, victim, obj, data);
   }
   else
   {
      world[g_data->room].sector_type = g_data->old_sect;
      world[g_data->room].room_flags = g_data->flags;
   }
}

//-------------------------------------------------------------------------------
bool prepare_room_transmute(P_char ch, int room, int skill, int duration, int delay,
                            event_func_type trans_func, event_func_type func_bye)
{
   // cannot transmute if another transmute in action
   if(get_spell_from_room(&world[room], TAG_TRANSMUTE_ROOM))
   {
      send_to_char("Sparks flow from your hands, but CHAOS in room disperse your magic.\r\n", ch);
      return FALSE;
   }

   struct grow_data g_data;
   g_data.room = ch->in_room;
   g_data.skill = skill;
   g_data.duration = duration;
   g_data.func_bye = func_bye;
   
   if(delay > 0)
   {
      // mark for other transmutes that action in progress
      struct room_affect af;
      memset(&af, 0, sizeof(struct room_affect));
      af.type = TAG_TRANSMUTE_ROOM;
      af.duration = delay + 10;
      af.ch = ch;
      affect_to_room(room, &af);

      // mark for current transmute that action in progress
      struct room_affect af1;
      memset(&af1, 0, sizeof(struct room_affect));
      af1.type = skill;
      af1.duration = delay + 10;
      af1.ch = ch;
      affect_to_room(room, &af1);

      add_event(trans_func, delay, ch, NULL, NULL, 0, &g_data,
                sizeof(g_data));
   }
   else
   {
      trans_func(ch, NULL, NULL, &g_data);
   }
   
   return TRUE;
}

//-------------------------------------------------------------------------------
void finish_room_transmute(P_char ch, struct grow_data *data)
{
   struct grow_data g_data;
   struct room_affect *af;

   af = get_spell_from_room(&world[data->room], TAG_TRANSMUTE_ROOM);
   if(af)
   {
      affect_room_remove(data->room, af);
   }
   af = get_spell_from_room(&world[data->room], data->skill);
   if(af)
   {
      affect_room_remove(data->room, af);
   }

   g_data.room = data->room;
   g_data.old_sect = world[g_data.room].sector_type;
   g_data.flags = world[g_data.room].room_flags;
   g_data.func_bye = data->func_bye;
   
   // find an associated event and disarm it
   // also take original room sector/flags
   P_nevent e;
   for( e = get_scheduled(ch, event_transmute_bye); e; e = get_next_scheduled_char(e, event_transmute_bye) )
   {
     struct grow_data *tmp_data = (struct grow_data *) e->data;
     if( tmp_data->room == data->room)
     {
       g_data.old_sect = tmp_data->old_sect;
       g_data.flags = tmp_data->flags;
       disarm_single_event(e);
       break;
     }
   }

   add_event(event_transmute_bye, data->duration, 0, NULL, NULL, 0, &g_data,
             sizeof(g_data));
}
//-------------------------------------------------------------------------------

//--------- SPELL_ETHEREAL_GROUNDS ------------
void event_ethereal_grounds_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("&+LAs mysterious &+wh&+Waz&+we &+Wf&+wad&+Les the surroundings start to look familiar again.\n",
               g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_ethereal_grounds(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
   send_to_room("&+LThe surroundings begin to &+cb&+Cl&+cu&+Cr &+Land &+ct&+Cw&+ci&+Cs&+ct &+Las mysterious &+wh&+Waz&+we &+Lfills the area.\n",
                g_data->room);

   world[g_data->room].sector_type = SECT_ETHEREAL;
   REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void spell_ethereal_grounds(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  if(!ch)
  {
    logit(LOG_EXIT, "spell_ethereal_grounds called in magic.c with no ch");
    raise(SIGSEGV);
  }
  
  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

   switch (terrain_type)
   {
      case SECT_ETHEREAL:
         send_to_char("This room is ethereal enough!\n", ch);
         return;
      default:
          break;
   }

  if (get_spell_from_room(&world[ch->in_room], SPELL_ETHEREAL_GROUNDS))
  {
    send_to_char("The earth is already starting to transform!\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_ETHEREAL_GROUNDS,
                              4 * 60, 4 * seconds,
                              event_ethereal_grounds, event_ethereal_grounds_bye) )
     return;
  
  send_to_room("&+cThe ground starts to swirl together...&n\n", ch->in_room);
}

//--------- SPELL_TRANS_MUD_ROCK ------------
void event_mud_rock_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The water in the ground rises, becoming &+mswampy&n.\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_trans_mud_rock(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
  send_to_room("&+yThe muddy ground swirls together and becomes &nsolid.\n", g_data->room);

  world[g_data->room].sector_type = SECT_FIELD;
  REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_transmute_mud_rock(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type) {
                case SECT_FIELD:
                case SECT_FOREST:
		case SECT_SWAMP:
		   break;
		default:
			 send_to_char("There's no mud to form earth here!\r\n", ch);
			 return;
			 break;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_TRANS_MUD_ROCK))
  {
    send_to_char("The earth is already starting to transform!\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_TRANS_MUD_ROCK,
                              4 * 60, 4 * seconds,
                              event_trans_mud_rock, event_mud_rock_bye) )
     return;
  
  send_to_room("&+yThe ground starts to swirl together...&n\n", ch->in_room);
}

//--------- SPELL_TRANS_ROCK_MUD ------------
void event_rock_mud_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The swamp dries out and becomes &+ysolid&n.\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_trans_rock_mud(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
  send_to_room("&+yThe ground swirls around and becomes &+mswampy!&n\n", g_data->room);

  world[g_data->room].sector_type = SECT_SWAMP;
  REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_transmute_rock_mud(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type) {
          case SECT_MOUNTAIN:
  	  case SECT_HILLS:
	  case SECT_FIELD:
          case SECT_FOREST:
          case SECT_ARCTIC:
          case SECT_UNDRWLD_WILD:
          case SECT_UNDRWLD_CITY:
          case SECT_UNDRWLD_MOUNTAIN:
          case SECT_UNDRWLD_LOWCEIL:
          case SECT_UNDRWLD_LIQMITH:
          case SECT_UNDRWLD_MUSHROOM:
	  case SECT_EARTH_PLANE:
		   break;
		default:
			 send_to_char("You'll have a tough time making lots of mud here!\r\n", ch);
			 return;
			 break;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_TRANS_ROCK_MUD))
  {
    send_to_char("The earth is already starting to transform!\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_TRANS_ROCK_MUD,
                              4 * 60, 4 * seconds,
                              event_trans_rock_mud, event_rock_mud_bye) )
     return;
  
  send_to_room("&+yThe ground starts to &+Bswirl together...&n\n", ch->in_room);
}

//--------- SPELL_TRANS_MUD_WATER ------------
void event_mud_water_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The water in this room drains and the area becomes more &+mswampy&n.\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_trans_mud_water(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
  send_to_room("&+yThe &+mswamp &+Bwater &+yrises and fills the area!&n\n", g_data->room);

  world[g_data->room].sector_type = SECT_WATER_SWIM;
  REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_transmute_mud_water(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type)
	{
      case SECT_FOREST:
      case SECT_FIELD:
		case SECT_SWAMP:
		   break;
		default:
			 send_to_char("This might as well be a desert!\r\n", ch);
			 return;
			 break;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_TRANS_MUD_WATER))
  {
    send_to_char("&+bThe area is already filling with water!\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_TRANS_MUD_WATER,
                              4 * 60, 4 * seconds,
                              event_trans_mud_water, event_mud_water_bye) )
     return;
  
  send_to_room("&+BThe area starts to flood...&n\n", ch->in_room);
}

//--------- SPELL_TRANS_WATER_MUD ------------
void event_water_mud_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The swamp becomes so filled with water, it returns to &+Bwater&n.\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_trans_water_mud(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
  send_to_room("&+BThe water in this area drains, turning the area into &+ma swamp&n!&n\n",
               g_data->room);

  world[g_data->room].sector_type = SECT_SWAMP;
  REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_transmute_water_mud(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type) {
		
                case SECT_UNDRWLD_WATER:
                case SECT_UNDERWATER:
                case SECT_UNDERWATER_GR:
                case SECT_OCEAN:
		case SECT_WATER_SWIM:
		case SECT_WATER_PLANE:
		case SECT_WATER_NOSWIM:
		   break;
		default:
			 send_to_char("You need a fair amount of water and mud to work with!\r\n", ch);
			 return;
			 break;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_TRANS_WATER_MUD))
  {
    send_to_char("&+yThe land is already &+Brising!\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_TRANS_WATER_MUD,
                              4 * 60, 4 * seconds,
                              event_trans_water_mud, event_water_mud_bye) )
     return;

  send_to_room("&+bThe water clouds &+yand becomes more solid...&n\n",
               ch->in_room);
}

//--------- SPELL_TRANS_WATER_AIR ------------
void event_water_air_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The air once again condenses into &+Bwater&n.\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_trans_water_air(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
   send_to_room("&+BThe water in this area &+Wevaporates!&n\n", g_data->room);

   world[g_data->room].sector_type = SECT_AIR_PLANE;
   REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_transmute_water_air(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type) {
                case SECT_UNDRWLD_WATER:
                case SECT_UNDERWATER:
                case SECT_UNDERWATER_GR:
                case SECT_OCEAN:
		case SECT_WATER_SWIM:
		case SECT_WATER_PLANE:
		case SECT_WATER_NOSWIM:
		   break;
		default:
			 send_to_char("You need a fair amount of water to work with!\r\n", ch);
			 return;
			 break;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_TRANS_WATER_AIR))
  {
    send_to_char("&+yThe land is already &+Brising!\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_TRANS_WATER_AIR,
                              4 * 60, 4 * seconds,
                              event_trans_water_air, event_water_air_bye) )
     return;

  send_to_room("&+BThe water starts to &+Cevaporate...&n\n",
               ch->in_room);
}

//--------- SPELL_TRANS_AIR_WATER ------------
void event_air_water_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The water evaporates and reverts to &+Cair&n.\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_trans_air_water(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
   send_to_room("&+CThe air becomes heavy and finally condenses into &+Bwater!&n\n",
                g_data->room);

   world[g_data->room].sector_type = SECT_WATER_SWIM;
   REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_transmute_air_water(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type) {
		case SECT_AIR_PLANE:
		   break;
		default:
			 send_to_char("You need more air than this!\r\n", ch);
			 return;
			 break;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_TRANS_AIR_WATER))
  {
    send_to_char("&+cThe air is already gathering moisture!\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_TRANS_AIR_WATER,
                              4 * 60, 4 * seconds,
                              event_trans_air_water, event_air_water_bye) )
     return;

  send_to_room("&+cThe air starts becoming &+Bvery moist...&n\n",
               ch->in_room);
}

//--------- SPELL_TRANS_ROCK_LAVA ------------
void event_rock_lava_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The &+rlava&n cools down and forms to rock.\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_trans_rock_lava(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);

  send_to_room("&+RThe ground opens up spewing lava everywhere!&n\n", g_data->room);

  world[g_data->room].sector_type = SECT_FIREPLANE;
  REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_transmute_rock_lava(int level, P_char ch, char *arg, int type, P_char tar_ch, P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  if (CHAR_IN_TOWN(ch))
  {
    send_to_char("Disabled in town due to abuse.\n", ch);
    return;
  }
  
  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type)
	{
	   case SECT_DESERT:
      case SECT_ARCTIC:
      case SECT_CITY:
      case SECT_FOREST:
      case SECT_FIELD:
      case SECT_HILLS:
      case SECT_MOUNTAIN:
      case SECT_UNDRWLD_WILD:
      case SECT_EARTH_PLANE:
		   break;
		default:
			 send_to_char("How about trying this with more rock?\r\n", ch);
			 return;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_TRANS_ROCK_LAVA))
  {
    send_to_char("&+rThe rocks are already starting to melt...\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_TRANS_ROCK_LAVA,
                              4 * 60, 4 * seconds,
                              event_trans_rock_lava, event_rock_lava_bye) )
     return;

  send_to_room("&+LThe rocky ground &+Rstarts to melt before your eyes...&n\n",
               ch->in_room);
}

//--------- SPELL_TRANS_LAVA_ROCK ------------
void event_lava_rock_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The &+rlava&n bursts through the ground and once again surrounds you!\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_trans_lava_rock(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
   send_to_room("&+RThe lava &+Ccools&+r and forms an island of &+Lrock&n!&n\n",
                g_data->room);

   world[g_data->room].sector_type = SECT_HILLS;
   REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_transmute_lava_rock(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type)
	{
		case SECT_FIREPLANE:
		case SECT_LAVA:
		   break;
		default:
			 send_to_char("How about trying this with more lava?\r\n", ch);
			 return;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_TRANS_LAVA_ROCK))
  {
    send_to_char("&+rThe lava is already starting to cool...\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_TRANS_LAVA_ROCK,
                              4 * 60, 4 * seconds,
                              event_trans_lava_rock, event_lava_rock_bye) )
     return;
  
  send_to_room("&+LThe lava starts to &+Ccool &+Lright before your eyes!&n\n",
               ch->in_room);
}

//--------- SPELL_DEPRESSED_EARTH ------------
void event_depressed_earth_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("&+cThe &+Wspiritual &+cpresence in this land returns to normal&n.\n", g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_depressed_earth(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);
  
   send_to_room("&+LA wave of spiritual dread turns this area into &+ma swamp!&n\n",
                g_data->room);

   world[g_data->room].sector_type = SECT_SWAMP;
   REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_depressed_earth(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  terrain_type = world[ch->in_room].sector_type;

	switch (terrain_type) {
		case SECT_FOREST:
		case SECT_HILLS:
		case SECT_FIELD:
		case SECT_UNDRWLD_WILD:
		case SECT_CITY:
                case SECT_ROAD:
		case SECT_UNDRWLD_CITY:
		   break;
		default:
			 send_to_char("The spirits of this area withstand your spell!\r\n", ch);
			 return;
			 break;
	}

  if (get_spell_from_room(&world[ch->in_room], SPELL_DEPRESSED_EARTH))
  {
    send_to_char("&+LThis area is already depressed.\r\n", ch);
    return;
  }

  if( !prepare_room_transmute(ch, ch->in_room, SPELL_DEPRESSED_EARTH,
                              4 * 60, 4 * seconds,
                              event_depressed_earth, event_depressed_earth_bye) )
     return;
  
  send_to_room("&+LA wave of depression sweeps through these &+ylands...&n\n",
               ch->in_room);
}

//--------- SPELL_GROW ------------
void event_grow_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct grow_data *g_data = (struct grow_data *) data;

  send_to_room("The &+Gtrees&n start to wither and die and within "
               "minutes nothing is left of the once proud forest.\n",
               g_data->room);

  world[g_data->room].sector_type = g_data->old_sect;
  world[g_data->room].room_flags = g_data->flags;
}

void event_grow(P_char ch, P_char victim, P_obj obj, void *data)
{
   struct grow_data *g_data = (struct grow_data *) data;
   finish_room_transmute(ch, g_data);

  if (IS_UNDERWORLD(g_data->room))
  {
    send_to_room("The ground buckles and splits apart as large "
                 "&+Mmushrooms&n and flailing\n"
                 "&+Gtrees&n grow from pod to full stature in a "
                 "matter of seconds.\n",
                 g_data->room);
  }
  else
  {
    send_to_room("Small &+Gplants&n sprout up from the ground and "
                 "start to grow at an incredible speed.\n"
                 "Within seconds you are surrounded by a small forest!\n",
                 g_data->room);
  }

  world[g_data->room].sector_type = SECT_FOREST;
  REMOVE_BIT(world[g_data->room].room_flags, INDOORS);
}

void cast_grow(int level, P_char ch, char *arg, int type, P_char tar_ch,
               P_obj tar_obj)
{
  int      terrain_type, seconds, duration;
  struct room_affect af;

  seconds = 5 + dice(2, 5);
  duration = 4 * (60 * 5);
  
  terrain_type = world[ch->in_room].sector_type;

  if (IS_WATER_ROOM(ch->in_room))
  {
    send_to_char("Forests don't grow on water.\n", ch);
    return;
  }
  if (get_spell_from_room(&world[ch->in_room], SPELL_GROW))
  {
    send_to_char("The vegetation is already starting to grow!\r\n", ch);
    return;
  }
  
  switch(terrain_type) {

	case SECT_FOREST:
		send_to_char("&+GThere seems to be enough vegetation here already.&n\n", ch);
	 	return;
	case SECT_FIELD:
   case SECT_SWAMP:
   case SECT_MOUNTAIN:
	case SECT_HILLS:
	case SECT_ARCTIC:
   case SECT_CITY:
	case SECT_EARTH_PLANE:
	case SECT_ASTRAL:
	case SECT_ETHEREAL:
		break;   	
	case SECT_DESERT:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_ROAD:
	case SECT_INSIDE:
   default:
		send_to_char("This terrain is not fit for a forest.\n", ch);
		return;
		break;

  }
  
  if(IS_SET(world[ch->in_room].room_flags, GUILD_ROOM))
  {
    send_to_char("This terrain is not fit for a forest.\n", ch);
    return;
  }
  
  if( !prepare_room_transmute(ch, ch->in_room, SPELL_GROW,
                              4 * 60, 4 * seconds,
                              event_grow, event_grow_bye) )
     return;

  send_to_room("&+GThe ground starts to glow with a soft green light.&n\n",
               ch->in_room);
}


void cast_vines(int level, P_char ch, char *arg, int type, P_char tar_ch, P_obj tar_obj)
{
  P_obj    t_obj, next_obj;
  P_obj    used_obj[4];
  struct affected_type af;
  int      count, i;

  if (IS_AFFECTED5(ch, AFF5_VINES))
    return;

/*
  if (world[ch->in_room].sector_type != SECT_FOREST || world[ch->in_room].sector_type == SECT_FIELD
      || world[ch->in_room].sector_type == SECT_SWAMP) {
     send_to_char("There doesn't appear to be many vines around here.\n", ch);
     return;
  }
*/

  if(IS_PC(ch) || IS_PC_PET(ch))
  {
    for( count = 0, t_obj = ch->carrying; t_obj; t_obj = next_obj )
    {
      next_obj = t_obj->next_content;

      if (obj_index[t_obj->R_num].virtual_number == VOBJ_INGRED_GREEN_HERB )
      {
        used_obj[count] = t_obj;
        if( ++count == 4 )
        {
          break;
        }
      }
    }

    if( count == 0 )
    {
      send_to_char("You must have &+ga green herb&n in your inventory.\r\n", ch);
      return;
    }

    for( i = 0; i < count; i++ )
    {
      extract_obj(used_obj[i], TRUE); // Just herb ingred, but 'in game.'
    }
  }
  else
    count = (int)(level / 10);

  act("&+GGreen&n vines sprout up around you forming a protective shield.", FALSE, ch, 0, 0, TO_CHAR);
  act("&+GVines&n sprout up around $n forming a protective shield.", FALSE, ch, 0, 0, TO_NOTVICT);

  memset(&af, 0, sizeof(af));
  af.type = SPELL_VINES;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
  af.bitvector5 = AFF5_VINES;
  af.duration = level / 2;
  af.modifier = 40 * count;
  affect_to_char(ch, &af);
}

struct spike_growth_data
{
	int	room;
	int iter;
};

struct awaken_forest_data
{
  int      room;
  int      iter;
};

void event_spike_growth(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct spike_growth_data *sgd;
  P_char tch;
  int dam, technique, duration, chance;
  struct room_affect af;

  struct damage_messages message = {
    "From out of nowhere &+yspikes in the ground&n shoot out piercing $N!",
    "From out of nowhere &+yspikes in the ground&n shoot out at you, that really hurt!",
    "From out of nowhere &+yspikes in the ground&n shoot out at piercing $N!",
    "You cringe as you hear the snapping of &+Wbones&n.",
    "The last thing you hear before darkness claims you is the noise of snapping &+Wbones&n...",
    "You cringe as you hear the snapping of &+Wbones.&n", 0
  };

  sgd = (struct spike_growth_data *) data;

  if (sgd->iter == 0)
  {
    send_to_room
      ("The creaking of the &+yearth&n can be heard as it starts to move beneath your feet...\r\n", ch->in_room);
    sgd->iter++;

    duration = WAIT_SEC * (10 + dice(3, 2));

    memset(&af, 0, sizeof(struct room_affect));
    af.type = SPELL_SPIKE_GROWTH;
    af.duration = duration;
    af.ch = ch;
    affect_to_room(ch->in_room, &af);

    goto spike_grow_next;
  }

  if (sgd->iter == 1)
  {
    send_to_room("The ground fissures and &+ytiny spikes&n can be seen in the broken ground.\r\n",
                 sgd->room);

    sgd->iter++;

    goto spike_grow_next;
  }

  tch = world[sgd->room].people;

  chance = (int) get_property("spell.spikeGrowth.affectChance", 10.00);

  for (tch = world[sgd->room].people; tch; tch = tch->next_in_room)
  {
    char     buf[1024];

    memset(buf, 0, sizeof(buf));

    if (tch == ch)
      continue;

    if (ch->in_room == sgd->room)
      if (!should_area_hit(ch, tch))
        continue;

    if (dice(1, 100) < 100-chance)
      continue;

    technique = dice(1, 3);
    switch (technique)
    {
    case 1:
      spell_grow_spike(GET_LEVEL(ch), ch, 0, 0, tch, obj);
      break;
    case 2:
      act
        ("A &+yHUGE SPIKE&n bursts from the &+yground&n and pierces you in the foot!.",
         FALSE, tch, 0, 0, TO_CHAR);
      act
        ("A &+yHUGE SPIKE&n bursts from the &+yground&n piercing&n $n's &+yfoot!&n",
         FALSE, tch, 0, 0, TO_NOTVICT);
      spell_damage(ch, tch, GET_LEVEL(ch) * number(1, 3), SPLDAM_GENERIC, 0, &message);
      SET_POS(tch, POS_SITTING + GET_STAT(tch));
      CharWait(tch, PULSE_VIOLENCE);
      break;
    case 3:
      dam = dice(1, GET_LEVEL(ch)) + 160;
      spell_damage(ch, tch, dam, SPLDAM_GENERIC, 0, &message);
      break;
    }
  }

  if (!get_spell_from_room(&world[ch->in_room], SPELL_SPIKE_GROWTH))
  {
    send_to_room
      ("The spike-filled fissures in the &+yearth&n close up and smooth over.\r\n",
       sgd->room);
    return;
  }

spike_grow_next:
  add_event(event_spike_growth, WAIT_SEC * 2, ch, NULL, NULL, 0, sgd,
            sizeof(struct spike_growth_data));
}


void event_awaken_forest(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct awaken_forest_data *awd;
  P_char   tch;
  int      dam, technique, duration;
  struct room_affect af;
  int nCnt = 0;

  struct damage_messages messages = {
    "From out of nowhere &+ybranches&n wrap themselves around $N crushing $M in their grip.",
    "From out of nowhere branches wrap themselves around you, crushing you in their grip.",
    "From out of nowhere &+ybranches&n wrap themselves around $N crushing $M in their grip.",
    "You cringe as you hear the snapping of &+Wbones&n.",
    "The last thing you hear before darkness claims you is the noise of snapping &+Wbones&n...",
    "You cringe as you hear the snapping of &+Wbones.&n", 0
  };

  awd = (struct awaken_forest_data *) data;

  if (world[awd->room].sector_type != SECT_FOREST)
    goto awaken_next;

  if (awd->iter == 0)
  {
    send_to_room
      ("The creaking of &+ytree trunks&n shifting and a low groaning sound can be heard throughout the &+Gforest&n.\r\n", ch->in_room);
    awd->iter++;

    duration = WAIT_SEC * (5 + dice(3, 2));

    memset(&af, 0, sizeof(struct room_affect));
    af.type = SPELL_AWAKEN_FOREST;
    af.duration = duration;
    af.ch = ch;
    affect_to_room(ch->in_room, &af);

    goto awaken_next;
  }

  if (awd->iter == 1)
  {
    send_to_room("The &+Gtrees&n stretch their old gnarly &+ybranches&n and you feel their roots slowly moving beneath the ground.\r\n",
                 awd->room);

    awd->iter++;

    goto awaken_next;
  }

  for (tch = world[awd->room].people; tch; tch = tch->next_in_room)
  {
    if ((ch->in_room != awd->room) || (should_area_hit(ch, tch)))
      nCnt ++;
  }
  // now use the nCnt to determine the chance of a hit...
  // the more people, the less the chance.
  
  for (tch = world[awd->room].people; tch; tch = tch->next_in_room)
  {
    char     buf[1024];

    memset(buf, 0, sizeof(buf));

    if (tch == ch)
      continue;

    if (ch->in_room == awd->room)
      if (!should_area_hit(ch, tch))
        continue;

    if (number(0, MAX(2,nCnt)) > 2)
      continue;

    technique = dice(1, 3);
    switch (technique)
    {
    case 1:
      spell_entangle(60, ch, 0, 0, tch, obj);
      break;
    case 2:
      
      if (GET_POS(tch) == POS_STANDING &&  (GET_C_AGI(tch) < number(0, 125) ))
          {
        act("Thick &+groots&n burst from the &+yground&n knocking you off balance.", FALSE, tch, 0, 0, TO_CHAR);
        act("Thick &+groots&n burst from the &+yground&n knocking&n $n &+yoff balance.&n", FALSE, tch, 0, 0, TO_NOTVICT);    
        SET_POS(tch, POS_SITTING + GET_STAT(tch));
        if (!NewSaves(tch, SAVING_SPELL, 3)) { 
    act(" .. the &+groots&n hit you really hard!", FALSE, tch, 0, 0, TO_CHAR);
    act(" .. the &+groots&n hit $n really hard!", FALSE, tch, 0, 0, TO_NOTVICT);
          CharWait(tch, (int) (PULSE_VIOLENCE * 1.5));
        }
      }
      else {
      act("Thick &+groots&n burst from the &+yground&n missing you.", FALSE, tch, 0, 0, TO_CHAR);
      act("Thick &+groots&n burst from the &+yground&n missing&n $n.", FALSE, tch, 0, 0, TO_NOTVICT);
      }
      break;
    case 3:
      dam = dice(1, 40) + 160;
      spell_damage(ch, tch, dam, SPLDAM_GENERIC, 0, &messages);

      break;
    }
  }

  if (!get_spell_from_room(&world[ch->in_room], SPELL_AWAKEN_FOREST))
  {
    send_to_room
      ("The flailing &+Gforest&n calms down and returns to its normal state.\r\n",
       awd->room);
    return;
  }

awaken_next:
  add_event(event_awaken_forest, WAIT_SEC * 3 /2, ch, NULL, NULL, 0, awd,
            sizeof(struct awaken_forest_data));
}


void cast_spike_growth(int level, P_char ch, char *arg, int type,
                        P_char tar_ch, P_obj tar_obj)
{
  int      duration = 0;
  struct spike_growth_data sgd;

  memset(&sgd, 0, sizeof(sgd));

  if (get_spell_from_room(&world[ch->in_room], SPELL_SPIKE_GROWTH))
  {
    send_to_char("You're already growing &+yspikes&n!\r\n", ch);
    return;
  }

  switch(world[ch->in_room].sector_type) {
      case SECT_FOREST:
      case SECT_HILLS:
      case SECT_UNDRWLD_MUSHROOM:
      case SECT_UNDRWLD_SLIME:
      case SECT_UNDRWLD_INSIDE:
      case SECT_UNDRWLD_WILD:
      case SECT_UNDRWLD_CITY:
      case SECT_CITY:
      case SECT_INSIDE:
      case SECT_MOUNTAIN:
      case SECT_ARCTIC:
      case SECT_EARTH_PLANE:
      case SECT_ROAD:
          break;
      default:
          send_to_char("The ground is not suitable for forming &+yspikes&n\r\n", ch);
  }


  sgd.room = ch->in_room;
  sgd.iter = 0;

  add_event(event_spike_growth, WAIT_SEC, ch, NULL, NULL, 0, &sgd,
            sizeof(sgd));
}

void cast_awaken_forest(int level, P_char ch, char *arg, int type,
                        P_char tar_ch, P_obj tar_obj)
{
  int      duration = 0;
  struct awaken_forest_data awd;

  memset(&awd, 0, sizeof(awd));

  if (get_spell_from_room(&world[ch->in_room], SPELL_AWAKEN_FOREST))
  {
    send_to_char("The forest is already awake!\r\n", ch);
    return;
  }

  if (!get_spell_from_room(&world[ch->in_room], SPELL_GROW) &&
      world[ch->in_room].sector_type != SECT_FOREST)
  {
    send_to_char("What forest do you expect to wake?\r\n", ch);
    return;
  }

  act("&+gThe &+Gforest&+g begins to move and rumble!&n", FALSE, ch, 0, 0, TO_CHAR);
  act("&+gThe &+Gforest&+g begins to move and rumble!&n", FALSE, ch, 0, 0, TO_NOTVICT);
  awd.room = ch->in_room;
  awd.iter = 0;

  add_event(event_awaken_forest, WAIT_SEC, ch, NULL, NULL, 0, &awd,
            sizeof(awd));
}

/*** STORM DRUID ***/

void cast_hurricane(int level, P_char ch, char *arg, int type, P_char tar_ch,
                    P_obj tar_obj)
{
  int      dam, affchance, chance;
  P_char   tch, next;

  struct damage_messages messages = {
    "&+WHurricane&n winds sweep through the room tearing at $N!.",
    "&+WHurricane&n winds sweep through the room tearing at you.",
    "&+WHurricane&n winds sweep through the room tearing at $N!.",
    "&+WHurricane winds lift $N up into the air only to send $M tumbling to $S death.",
    "Strong &+Wwinds&n lift you skyward only to send you tumbling to your doom!",
    "&+WHurricane winds lift $N up into the air only to send $M tumbling to $S death."
  };

  //dam = 40 << 2;
  // This new damage below is about 1/2 that of swarm
  dam = 40 + level * 6 + number(1, 30);
 
  tch = world[ch->in_room].people;

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if (tch == ch)
      continue;

    if (!should_area_hit(ch, tch))
      continue;

    if (dice(1, 100) < 30)
      continue;

    if (IS_AFFECTED(tch, AFF_FLY))
      chance = 5;
    else
      chance = 1;

    if (GET_LEVEL(ch) >= 51)
      chance *= 2;
    
    affchance = number(1, 100);
    if (OUTSIDE(ch))
    {
      if (affchance < chance)
      {
        act("The gail force of your spell sends $N crashing to the ground!",
          FALSE, ch, 0, tch, TO_CHAR);
        act("The gail force of $n's spell sends you crashing to the ground!",
          FALSE, ch, 0, tch, TO_VICT);
        act("The gail force of $n's spell sends $N crashing to the ground!",
          FALSE, ch, 0, tch, TO_NOTVICT);
        SET_POS(tch, POS_SITTING + GET_STAT(tch));
      }
    }
    spell_damage(ch, tch, dam, SPLDAM_GENERIC, 0, &messages);
  }
}

void cast_storm_shield(int level, P_char ch, char *arg, int type,
                       P_char tar_ch, P_obj tar_obj)
{
  struct affected_type af;

  if (affected_by_spell(ch, SPELL_STORMSHIELD))
    return;

  if (!tar_ch->equipment[WEAR_SHIELD])
  {
    send_to_char("But you do not wear any shield!\r\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_STORMSHIELD;
  af.duration = level / 5;
  affect_to_char(ch, &af);

  act
    ("&+bYour $q &+bstarts to crackle with energy, as small &+Wbolts &+bof &+Wlightning &+brun along its surface.&n",
     FALSE, ch, ch->equipment[WEAR_SHIELD], 0, TO_CHAR);

  act
    ("&+b$n&+b's $q &+bstarts to crackle with energy, as small &+Wbolts &+bof &+Wlightning &+brun along its surface.&n",
     FALSE, ch, ch->equipment[WEAR_SHIELD], 0, TO_ROOM);
}

void cast_bloodstone(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  struct affected_type af;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( resists_spell(ch, victim) )
  {
    return;
  }

  if( affected_by_spell(victim, SPELL_BLOODTOSTONE) )
  {
    send_to_char("Their blood is already made of stone!\n", ch);
    return;
  }


  // Calculate % health missing.
  float duration = GET_HIT(victim) * 1.0 / GET_MAX_HIT(victim);
  debug("blood to stone: duration %f", duration);
  duration = 1 - duration;
  duration *= 100;
  duration = MAX( level, duration );
  // Lasts % health missing seconds, or level seconds, whichever is more.
  int dur = duration;
  if( NewSaves(victim, SAVING_PARA, 5) )
  {
    dur /= 2;
  }
  debug("blood to stone: duration: %f, dur: %d", duration, dur);

  GET_VITALITY(victim) -= 3;
  StartRegen(victim, EVENT_MOVE_REGEN);

  bzero(&af, sizeof(af));
  af.type = SPELL_BLOODTOSTONE;
  af.duration = MAX( WAIT_SEC, dur * WAIT_SEC );
  af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
  affect_to_char(victim, &af);

  act("You feel as if though your blood starts to flow slower in your veins.",
    FALSE, victim, 0, 0, TO_CHAR);
  act("$n grimaces and clutches $s chest.",
    FALSE, victim, 0, 0, TO_NOTVICT);

}

/*
 * room - starting room
 * exit - direction to be walled
 * ch - who created the wall, can go past it or hit down
 * level - wall level
 * type - wall type
 * power - basic damage dealt by fire wall, ice wall etc
 * decay - how long until the wall drops, in 1/4 second pulses
 * short_descr - wall name as seen in room, use %s to include direction this wall blocks
 * desc - wall name as seen when walking into wall ie. bumps into <desc>.
 * flags - extra flags to be set on wall, like ITEM_INVISIBLE, ITEM_SECRET
 *
 * returns true if direction was walled succesfully, will always happen unless zone files are broken etc.
 */
bool create_walls(int room, int exit, P_char ch, int level, int type, int power, int decay, char *short_desc, char *desc, ulong flags)
{
  P_obj    wall_inside;
  P_obj    wall_outside;
  int      dir_room;
  int      reverse_exit;
  char     buf1[1024];
  char     buf2[1024];

  dir_room = (world[room].dir_option[exit])->to_room;
  reverse_exit = rev_dir[exit];

  wall_inside = read_object(VOBJ_WALLS, VIRTUAL);
  wall_outside = read_object(VOBJ_WALLS, VIRTUAL);

  if (!wall_inside || !wall_outside)
  {
    return FALSE;
  }

  SET_BIT(wall_inside->str_mask, STRUNG_DESC1 | STRUNG_DESC2);
  SET_BIT(wall_outside->str_mask, STRUNG_DESC1 | STRUNG_DESC2);

  sprintf(buf1, desc, dirs[exit]);
  sprintf(buf2, desc, dirs[reverse_exit]);

  wall_inside->description = str_dup(buf1);
  wall_outside->description = str_dup(buf2);
  wall_inside->short_description = str_dup(short_desc);
  wall_outside->short_description = str_dup(short_desc);
  wall_inside->extra_flags |= flags;
  wall_outside->extra_flags |= flags;

  wall_inside->value[0] = world[dir_room].number;
  wall_inside->value[1] = exit;
  wall_inside->value[2] = power;
  wall_inside->value[3] = type;
  wall_inside->value[4] = level;
  if (ch != NULL)
    if( IS_PC(ch) )
      wall_inside->value[5] = GET_PID(ch);
    else
      wall_inside->value[5] = GET_RNUM(ch);

  wall_outside->value[0] = world[room].number;
  wall_outside->value[1] = reverse_exit;
  wall_outside->value[2] = power;
  wall_outside->value[3] = type;
  wall_outside->value[4] = level;
  if (ch != NULL)
    if( IS_PC(ch) )
      wall_outside->value[5] = GET_PID(ch);
    else
      wall_outside->value[5] = GET_RNUM(ch);

  SET_BIT(world[room].dir_option[exit]->exit_info, EX_WALLED);
  SET_BIT(world[dir_room].dir_option[reverse_exit]->exit_info, EX_WALLED);

  if (decay != -1)
  {
    set_obj_affected(wall_inside, decay, TAG_OBJ_DECAY, 0);
    set_obj_affected(wall_outside, decay, TAG_OBJ_DECAY, 0);
  }

  obj_to_room(wall_inside, room);
  obj_to_room(wall_outside, dir_room);

  return TRUE;
}

bool exit_wallable(int room, int dir, P_char ch)
{
  struct room_direction_data *exit;
  int      dir_room, reverse_exit;

  if (dir == -1)
  {
    if (ch != NULL)
      send_to_char
        ("You can only cast this spell to the north, east, west, south, northwest,\r\n"
         "southwest, northeast, or southeast!\r\n", ch);
    return FALSE;
  }

  if (check_wall(room, dir))
  {
    if (ch != NULL)
      send_to_char("There's already a magical wall there!\r\n", ch);
    return FALSE;
  }

  exit = world[room].dir_option[dir];

  if (exit == NULL)
  {
    if (ch != NULL)
      send_to_char("You see no exit in that direction!\r\n", ch);
    return FALSE;
  }

  if (IS_SET(exit->exit_info, EX_ISDOOR) &&
      !IS_SET(exit->exit_info, EX_SECRET))
  {
    if (ch != NULL)
      send_to_char("You cannot cast the spell on doors!\r\n", ch);
    return FALSE;
  }

  if (IS_SET(exit->exit_info, EX_BLOCKED) ||
      IS_SET(exit->exit_info, EX_SECRET))
  {
    if (ch != NULL)
      send_to_char("You see no exit in that direction!\r\n", ch);
    return FALSE;
  }

  P_obj obj, next_obj;
  for (obj = world[room].contents; obj; obj = next_obj)
  {
     next_obj = obj->next_content;
     if (obj->R_num == real_object(500054))
     {
       send_to_char("The magic attempts to take hold, but disperses suddenly...", ch);
       return FALSE;
     }
  }

  dir_room = (world[room].dir_option[dir])->to_room;
  reverse_exit = rev_dir[dir];

  if (!VIRTUAL_EXIT(dir_room, reverse_exit) ||
      world[dir_room].dir_option[reverse_exit]->to_room != room)
  {
    if (ch != NULL)
      send_to_char
        ("There is something strange about this exit, you cannot wall it.\r\n",
         ch);
    return FALSE;
  }

  return TRUE;
}

void spell_mirage(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char gm, fm;
  struct group_list *gl;
  struct affected_type af;
  int race;
  char buff[MAX_STRING_LENGTH];

  if (!ch->group)
  {
    send_to_char("You need a group to cast this!\r\n", ch);
    return;
  }
  
  for (fm = world[ch->in_room].people; fm; fm = fm->next_in_room)
  {
    if (fm && (IS_FIGHTING(fm) || IS_DESTROYING(fm)))
    {
      send_to_char("Your mirage swirls briefly before dissapearing from a disturence in the area.\r\n", ch);
      return;
    }
  }

  act("&+LAs $n spreads $s hands out wide, bright &+rp&+Rr&+Yi&+Gs&+ym&+Ca&+ct&+Bi&+bc &+wm&+Wote&+ws&+L of &+ma&+Mr&+Lca&+Mn&+me&L&+Lpower appear before $m in mid air.  The &+wm&+Wote&+ws&+L pul&+wse a&+Wnd be&+wgin t&+Lo abs&+worb al&+Wl&L&+wra&+Wys of li&+wght&+L into i&+wtself g&+Wrowing b&+wright&+Ler and m&+wore num&+Werous m&+woment b&+Ly mome&+wnt.&L&+LWith another &+ma&+Mr&+Lca&+Mn&+me&+L gesture $n scatters the &+rp&+Rr&+Yi&+Gs&+ym&+Ca&+ct&+Bi&+bc &+wm&+Wote&+ws&+L accross&L&+La great wide arc before $s companions.&n\r\n", TRUE, ch, 0, 0, TO_ROOM);
  act("&+LAs a &+rp&+Rr&+Yi&+Gs&+ym&+Ca&+ct&+Bi&+bc &+wm&+Wot&+we&+L settles before you $n speaks an &+ma&+Mr&+Lca&+Mn&+me&+L word of &+Wpower&+L!&n", TRUE, ch, 0, 0, TO_ROOM);

  act("&+LAs you spread your hands out wide, bright &+rp&+Rr&+Yi&+Gs&+ym&+Ca&+ct&+Bi&+bc &+wm&+Wote&+ws&+L of &+ma&+Mr&+Lca&+Mn&+me&L&+Lpower appear before you in mid air.  The &+wm&+Wote&+ws&+L pul&+wse a&+Wnd be&+wgin t&+Lo abs&+worb al&+Wl&L&+wra&+Wys of li&+wght&+L into i&+wtself g&+Wrowing b&+wright&+Ler and m&+wore num&+Werous m&+woment b&+Ly mome&+wnt.&L&+LWith another &+ma&+Mr&+Lca&+Mn&+me&+L gesture you scatter the &+rp&+Rr&+Yi&+Gs&+ym&+Ca&+ct&+Bi&+bc &+wm&+Wote&+ws&+L accross&L&+La great wide arc before your companions.&n\r\n", TRUE, ch, 0, 0, TO_CHAR);
  act("&+LAs a &+rp&+Rr&+Yi&+Gs&+ym&+Ca&+ct&+Bi&+bc &+wm&+Wot&+we&+L settles before you, you speak an &+ma&+Mr&+Lca&+Mn&+me&+L word of &+Wpower&+L!&n", TRUE, ch, 0, 0, TO_CHAR);

  
  memset(&af, 0, sizeof(af));
  af.type = SPELL_MIRAGE;
  af.duration = 3;
  af.modifier = ch->in_room;

  for (gl = ch->group; gl; gl = gl->next)
  {
    if (ch->in_room == gl->ch->in_room)
    {
      gm = gl->ch;
      // Error handling
      if (!gm)
        continue;
      // Not affecting npc's
      if (IS_NPC(gm))
        continue;
      // So we don't mess up an assassin's disguise
      if (is_illusion_char(gm))
        continue;

      if (get_spell_from_char(gm, SPELL_MIRAGE))
        if (!is_illusion_char(gm))
	  affect_from_char(gm, SPELL_MIRAGE);

      if (RACE_GOOD(gm))
      {
        do
	{
	  race = number(1, LAST_RACE);
	}
	while (race != RACE_HUMAN &&
	       race != RACE_GREY &&
	       race != RACE_MOUNTAIN &&
	       race != RACE_BARBARIAN &&
	       race != RACE_GNOME &&
	       race != RACE_HALFLING &&
	       race != RACE_HALFELF &&
	       race != RACE_CENTAUR &&
	       race != RACE_GITHZERAI &&
	       race != RACE_AGATHINON &&
	       race != RACE_THRIKREEN &&
	       race != RACE_MINOTAUR &&
	       race != RACE_FIRBOLG &&
	       race != RACE_WOODELF);
      }
      else if (RACE_EVIL(gm))
      {
        do
	{
	  race = number(0, LAST_RACE);
	}
	while (race != RACE_DROW &&
	       race != RACE_DUERGAR &&
 	       race != RACE_GITHYANKI &&
	       race != RACE_OGRE &&
	       race != RACE_GOBLIN &&
	       race != RACE_ORC &&
	       race != RACE_OROG &&
	       race != RACE_TROLL &&
	       race != RACE_THRIKREEN &&
	       race != RACE_MINOTAUR &&
	       race != RACE_DRIDER &&
	       race != RACE_PILLITHID &&
	       race != RACE_KUOTOA);
      }
      else if (RACE_PUNDEAD(gm))
      {
        do
	{
	  race = number(0, LAST_RACE);
	}
	while (race != RACE_PLICH &&
	       race != RACE_PVAMPIRE &&
	       race != RACE_PDKNIGHT &&
	       race != RACE_SHADE &&
	       race != RACE_REVENANT &&
	       race != RACE_PSBEAST &&
	       race != RACE_WIGHT &&
	       race != RACE_GARGOYLE &&
	       race != RACE_PHANTOM);
       }
       else
       {
         race = number(0, LAST_RACE);
       }

      // Setting the disguise
      IS_DISGUISE_NPC(gm) = FALSE;
      IS_DISGUISE_PC(gm) = TRUE;
      IS_DISGUISE_ILLUSION(gm) = TRUE;
      IS_DISGUISE_SHAPE(gm) = FALSE;
      gm->disguise.name = str_dup(GET_NAME(gm));
      gm->disguise.m_class = gm->player.m_class;
      gm->disguise.race = race; 
      gm->disguise.level = GET_LEVEL(gm);
      gm->disguise.hit = 100;
      gm->disguise.racewar = GET_RACEWAR(gm);
      if (GET_TITLE(gm))
        gm->disguise.title = str_dup(GET_TITLE(gm));
      SET_BIT(gm->specials.act, PLR_NOWHO);
      justice_witness(gm, NULL, CRIME_DISGUISE);
      
      affect_to_char(gm, &af);
      add_event(event_mirage, 0, gm, NULL, NULL, 0, 0, 0);

      sprintf(buff, "&+LYour &+wi&+Wmag&+we &+ms&+Mh&+Lif&+Mt&+ms &+Land &+bb&+Blur&+bs &+Linto %s %s.&n", VOWEL(race_names_table[race].normal[0]) ? "an" : "a", race_names_table[race].ansi);
      act(buff, TRUE, gm, 0, ch, TO_CHAR);
      sprintf(buff, "&+LThe &+wi&+Wmag&+we &+Lof $n &+ms&+Mh&+Lif&+Mt&+ms &+Land &+bb&+Blur&+bs &+Linto %s %s.&n", VOWEL(race_names_table[race].normal[0]) ? "an" : "a", race_names_table[race].ansi);
      act(buff, FALSE, gm, 0, ch, TO_ROOM);
      //sprintf(buff, "$N's arcane magic forms an illusion about $n turning $m into %s %s.&n", VOWEL(race_names_table[race].normal[0]) ? "an" : "a", race_names_table[race].ansi);
      //act(buff, TRUE, gm, 0, ch, TO_NOTVICTROOM);
    }
  }
}

void event_mirage(P_char ch, P_char vict, P_obj obj, void *data)
{
  struct affected_type *afp;

  if (afp = get_spell_from_char(ch, SPELL_MIRAGE))
  {
    if (afp->modifier != ch->in_room)
    {
      remove_disguise(ch, TRUE);
      affect_from_char(ch, SPELL_MIRAGE);
      return;
    }
    if (!IS_DISGUISE_ILLUSION(ch))
    {
      affect_from_char(ch, SPELL_MIRAGE);
      return;
    }
  }
  else
  {
    remove_disguise(ch, TRUE);
    return;
  }

  add_event(event_mirage, 0, ch, NULL, NULL, 0, 0, 0);
}

