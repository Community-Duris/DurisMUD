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
#include "sound.h"
#include "specs.prototypes.h"
#include "map.h"
#include "disguise.h"
#include "graph.h"
#include "ctf.h"
#include "vnum.obj.h"

extern P_index obj_index;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_char combat_list;
extern P_event current_event;
extern P_event event_type_list[];
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern const char *apply_types[];
extern const flagDef extra_bits[];
extern const flagDef anti_bits[];
extern const char *item_types[];
extern const struct stat_data stat_factor[];
extern const char *dirs[], *dirs2[];
extern const int rev_dir[];
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int hometown[];
extern struct str_app_type str_app[];
extern struct con_app_type con_app[];
extern struct time_info_data time_info;
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern const char *undead_type[];
extern const int numCarvables;
extern const char *carve_part_name[];
extern const struct race_names race_names_table[];
extern const int carve_part_flag[];
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern Skill skills[];

extern const char *get_function_name( void * );
extern bool has_skin_spell( P_char );
extern bool exit_wallable( int room, int dir, P_char ch );
extern bool create_walls( int room, int exit, P_char ch, int level, int type, int power,
  int decay, char *short_desc, char *desc, ulong flags );

#define WIND_BLADE 98

int      cast_as_damage_area(P_char,
                             void (*func) (int, P_char, char *, int, P_char,
                                           P_obj), int, P_char, float, float);
int      cast_as_damage_area(P_char,
                             void (*func) (int, P_char, char *, int, P_char,
                                           P_obj), int, P_char, float, float,
                             bool (*s_func) (P_char, P_char));


void spell_vapor_armor(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  struct affected_type af;
  bool shown;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( !IS_AFFECTED(victim, AFF_ARMOR) )
  {
    memset(&af, 0, sizeof(af));

    af.type = SPELL_VAPOR_ARMOR;
    af.duration =  MAX(10, (int)(level / 2));
    af.modifier = (int)(-1 * level * 1.25);
    af.bitvector = AFF_ARMOR;
    af.location = APPLY_AC;

    affect_to_char(victim, &af);

    send_to_char("&+CYou feel the protection of the winds and storms flow through you.&n\r\n", victim);
  }
  else
  {
    struct affected_type *af1;
    shown = FALSE;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_VAPOR_ARMOR)
      {
        af1->duration = MAX(10, (int)(level / 2));
        if( !shown )
        {
          send_to_char("&+cYour vaporous armor is renewed!\r\n", victim);
          shown = TRUE;
        }
      }
    }
    if( !shown )
    {
      send_to_char( "&+WYou're already affected by an armor-type spell.\n", victim );
    }
  }
}

void spell_faerie_sight(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  struct affected_type af;
  int count;
  P_obj    t_obj, next_obj;
  P_obj    used_obj[32];
  bool refreshed;
  struct affected_type *pAff;

  if( !(victim && ch) )
  {
    logit(LOG_EXIT, "spell_faerie_sight: bogus params.");
    raise(SIGSEGV);
  }

  if( affected_by_spell(victim, SPELL_FAERIE_SIGHT) )
  {
    refreshed = FALSE;

    for( pAff = victim->affected; pAff; pAff = pAff->next )
    {
      if( (pAff->type == SPELL_FAERIE_SIGHT) && (pAff->duration < ( level / 2 )) )
      {
        // Takes a faerie dust to refresh the detect magic/good/evil.
        if( !IS_SET(pAff->bitvector2, AFF2_DETECT_MAGIC) || (vnum_in_inv(ch, VOBJ_FORAGE_FAERIE_DUST) > 0) )
        {
          if( IS_SET(pAff->bitvector2, AFF2_DETECT_MAGIC) )
          {
            vnum_from_inv( ch, VOBJ_FORAGE_FAERIE_DUST, 1 );
          }
          pAff->duration = level / 2;
          refreshed = TRUE;
        }
      }
    }

    if( !IS_AFFECTED2(victim, AFF2_DETECT_MAGIC) && (vnum_in_inv(ch, VOBJ_FORAGE_FAERIE_DUST) > 0) )
    {
      vnum_from_inv( ch, VOBJ_FORAGE_FAERIE_DUST, 1 );
      memset(&af, 0, sizeof(af));
      af.type = SPELL_FAERIE_SIGHT;
      af.duration = level / 2;
      af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
      affect_to_char(victim, &af);
      send_to_char("&+mYour eyes begin to twinkle even &+Mmore&+m.&n\r\n", victim);
      refreshed = TRUE;
    }
    if( !refreshed )
    {
      send_to_char("Nothing happens.\n", victim);
    }
    else
    {
      send_to_char( "Your eyes feel &+mwonderful&n.\n", victim );
    }
    return;
  }

  memset(&af, 0, sizeof(af));
  af.type = SPELL_FAERIE_SIGHT;
  af.duration = level / 2;
  af.bitvector = AFF_FARSEE;

  affect_to_char(victim, &af);

  if( GET_LEVEL(ch) >= 36 )
  {
    af.bitvector = af.bitvector | AFF_DETECT_INVISIBLE;
    af.bitvector2 = 0;
  }

/* Changed this to the below, can change it back if 2 dusts do anything.
  for (count = 0, t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;

    if (obj_index[t_obj->R_num].virtual_number == VOBJ_FORAGE_FAERIE_DUST)
    {
      used_obj[count] = t_obj;
      count++;
    }
  }

  if( GET_LEVEL(ch) >= 50 || count > 0 )
  {
    af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;

    if( count > 0 )
       extract_obj(used_obj[0]);
  }
*/
  for( t_obj = ch->carrying; t_obj; t_obj = t_obj->next_content )
  {
    if( OBJ_VNUM(t_obj) == VOBJ_FORAGE_FAERIE_DUST )
    {
      af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
      extract_obj(t_obj);
      break;
    }
  }

  affect_to_char(victim, &af);
  send_to_char("&+mYour eyes begin to twinkle.&n\r\n", ch);
}


void spell_cold_snap(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int      num;
  struct affected_type af, af2;

  memset(&af, 0, sizeof(af));
  memset(&af2, 0, sizeof(af2));

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus params");
    raise(SIGSEGV);
  }

  if (affected_by_spell(victim, SPELL_COLD_SNAP))
  {
    send_to_char("They are already affected!\r\n", ch);
    return;
  }

  num = dice(10, 5);

  af.type = SPELL_COLD_SNAP;
  af.duration = 10;
  af.modifier = num;
  af.location = APPLY_AC;

  af2.type = SPELL_COLD_SNAP;
  af2.duration = 10;
  af2.modifier = num * -1;
  af2.location = APPLY_AC;

  affect_to_char(victim, &af);

  if (!affected_by_spell(ch, SPELL_COLD_SNAP))
    affect_to_char(ch, &af2);

  act
    ("&+CYou unleash a blast of &+Wfreezing wind&n&+C at $N&+C, encasing $M in a layer of frost.",
     TRUE, ch, 0, victim, TO_CHAR);

  act
    ("&+CYou suddenly feel incredibly stiff from the &+Wblast&n&+C of cold unleashed by $n.&n",
     TRUE, ch, 0, victim, TO_VICT);

  act
    ("&+C$n &+Cunleashes a blast of &+Wfreezing wind&n&+C at $N&+C, encasing $M in a layer of frost.",
     TRUE, ch, 0, victim, TO_NOTVICT);

  if (number(0, 100) < 10)
    spell_slow(level, ch, 0, 0, victim, tar_obj);

}


void spell_path_of_frost(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj tar_obj)
{
  struct affected_type af;

  if (affected_by_spell(ch, SPELL_PATH_OF_FROST))
  {
    send_to_char("You are frosty enough as it is..\r\n", ch);
    return;
  }

  send_to_char("Your body emanates with a &+Cfrosty&n glow.\r\n", ch);

  memset(&af, 0, sizeof(af));
  af.type = SPELL_PATH_OF_FROST;
  af.flags = AFFTYPE_SHORT;
  af.duration = 980;
  affect_to_char(ch, &af);
}

void spell_mass_fly(int level, P_char ch, char *arg, int type, P_char victim,
                    P_obj tar_obj)
{
  struct group_list *gl;

  if (ch && ch->group)
  {
    gl = ch->group;

    /* leader first */
    if (gl->ch->in_room == ch->in_room)
      spell_fly(level, ch, 0, SPELL_TYPE_SPELL, gl->ch, 0);

    /* followers */
    for (gl = gl->next; gl; gl = gl->next)
    {
      if (gl->ch->in_room == ch->in_room)
        spell_fly(level, ch, 0, SPELL_TYPE_SPELL, gl->ch, 0);
    }
  }
}

void wind_blade_attack_routine(P_char ch, P_char victim)
{
        int attacks = BOUNDED(2, number(1, GET_LEVEL(ch)/10), 4);
        
        P_obj obj = ch->equipment[PRIMARY_WEAPON];
        
        if (!obj)
        {
                wizlog(MINLVLIMMORTAL, "Screw up in wind blade attack routine for %s", GET_NAME(ch));
                return;
        }

  act("&+CWinds gather in the area to guide your weapon against $N!", TRUE, ch, 0, victim, TO_CHAR);
  act("&+cWinds gather around $n &n&+cto guide $s &n&+cweapon in combat!", TRUE, ch, 0, victim, TO_ROOM);

  struct affected_type af;
  bzero(&af, sizeof(af));

  af.type = SPELL_WIND_BLADE;
  af.flags = AFFTYPE_SHORT;
  af.duration = 1;
  af.modifier = number(5, 15);
  af.location = APPLY_HITROLL;

  affect_to_char(ch, &af);

  affect_total(ch, FALSE);

        for (;attacks;attacks--)
        {
                if (IS_ALIVE(victim) && IS_ALIVE(ch))
                {
                  hit(ch, victim, obj);
                }
  }

  if (IS_ALIVE(ch) && affected_by_spell(ch, SPELL_WIND_BLADE))
    affect_from_char(ch, SPELL_WIND_BLADE);
}

bool has_wind_blade_wielded(P_char ch)
{
        P_obj obj = ch->equipment[PRIMARY_WEAPON];
        
        int blade_vnum = WIND_BLADE;
        
        if (!obj)
          return FALSE;
        
        if (obj->R_num == real_object(blade_vnum))
          return TRUE;
        else
          return FALSE;
}

bool has_wind_blade(P_char ch)
{
  P_obj obj, next_obj;
  
  int blade_vnum = WIND_BLADE;

  if (has_wind_blade_wielded(ch))
    return TRUE;  

  for (obj = ch->carrying; obj; obj = next_obj)
  {
    next_obj = obj->next_content;

    if (obj_index[obj->R_num].virtual_number == blade_vnum)
    {
      return TRUE;
    }
  }
  return FALSE;
}

void grant_wind_blade(P_char ch)
{
  P_obj    blade;

  int blade_vnum = WIND_BLADE;

  blade = read_object(real_object(blade_vnum), REAL);

  if (!blade)
  {
    logit(LOG_DEBUG, "spell_wind_blade(): obj 98 not loadable");
    return;
  }
  blade->extra_flags |= ITEM_NORENT;
  blade->bitvector = 0;
  /* how about some gay de procs for wind blade? Yeah baby! */
  if (GET_LEVEL(ch) >= 21)
      {
          blade->value[5] = 191;
          blade->value[6] = GET_LEVEL(ch);
          blade->value[7] = 40; //procs ice missile
      }
  if (GET_LEVEL(ch) >= 26)
      {
      SET_BIT(blade->bitvector2, AFF2_PROT_COLD);
      }
  if (GET_LEVEL(ch) >= 31)
      {
      SET_BIT(blade->bitvector3, AFF3_COLDSHIELD);
      }
  if (GET_LEVEL(ch) >= 36)
      {
          blade->value[5] = 10;
          blade->value[6] = GET_LEVEL(ch);
          blade->value[7] = 40; //procs cone of cold
      }
  if (GET_LEVEL(ch) >= 41)
      {
      blade->value[5] = 325;
          blade->value[6] = GET_LEVEL(ch);
          blade->value[7] = 40; //procs frostbite
      }
  if (GET_LEVEL(ch) >= 51)
      {
          blade->value[5] = 254;
          blade->value[6] = GET_LEVEL(ch);
          blade->value[7] = 40; //procs iceball
      }
  if (GET_LEVEL(ch) >= 56)
      {
      SET_BIT(blade->bitvector2, AFF2_AIR_AURA);
      }
  act("&+cSwirling air solidifies into a slender sword.&n.", TRUE, ch, blade,
      0, TO_ROOM);
  act("&+cSwirling air solidifies into a slender sword.&n", TRUE, ch, blade,
      0, TO_CHAR);
  blade->timer[0] = 180;

  obj_to_char(blade, ch);
  
  if (!ch->equipment[PRIMARY_WEAPON])
    wear(ch, blade, 12, TRUE);
}

void spell_wind_blade(int level, P_char ch, char *arg, int type, P_char
                      victim, P_obj tar_obj)
{
  P_obj obj, next_obj;
  
  if (!has_wind_blade(ch))
    grant_wind_blade(ch);
  else
  {
        if (!IS_FIGHTING(ch))
          send_to_char("&+cThe winds are already aiding you!\r\n", ch);
        if (!has_wind_blade_wielded(ch) && !ch->equipment[PRIMARY_WEAPON])
        {
          for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (obj_index[obj->R_num].virtual_number == WIND_BLADE)
        {
         wear(ch, obj, 12, TRUE);
        }
      }
        }
  }

  if (IS_FIGHTING(ch))
    if (has_wind_blade_wielded(ch))
    {
        if (P_char vict = GET_OPPONENT(ch))
        wind_blade_attack_routine(ch, vict);
    }
    else
    {
        send_to_char("You need to wield the gift of the air if you want to receive even greater aid!\r\n", ch);
    }
    
}

void spell_windwalk(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      location;
  char     buf[256] = { 0 };
  P_char   tmp = NULL;
  int      distance;

  if (!ch)
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if (IS_PC(ch))
    CharWait(ch, 36);
  else
    CharWait(ch, 5);

  if (!victim)
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }

  if (IS_AFFECTED3(victim, AFF3_NON_DETECTION))
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }
  if (IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) ||
      world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }
  if (IS_PC(victim) && IS_SET(victim->specials.act2, PLR2_NOLOCATE)
      && !is_introd(victim, ch))
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }
  P_char rider = get_linking_char(victim, LNK_RIDING);
  if(IS_NPC(victim) && rider)
  {
    send_to_char("&+CYou failed.\n", ch);
    return;
  }

  location = victim->in_room;

  if (IS_ROOM(location, ROOM_NO_TELEPORT) ||
      world[location].sector_type == SECT_CASTLE ||
      world[location].sector_type == SECT_CASTLE_WALL ||
      world[location].sector_type == SECT_CASTLE_GATE ||
      racewar(ch, victim) || (GET_MASTER(ch) && IS_PC(victim)))
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }
  
  if(ch &&
     !is_Raidable(ch, 0, 0))
  {
    send_to_char("&+WYou are not raidable. The spell fails!\r\n", ch);
    return;
  }
  
  if(victim &&
     IS_PC(ch) &&
     IS_PC(victim) &&
     !is_Raidable(victim, 0, 0))
  {
    send_to_char("&+WYour target is not raidable. The spell fails!\r\n", ch);
    return;
  }
  
  distance = (int)(level * 1.35);
  if (GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_TEMPESTMAGUS))
    distance += 15;

  if( !IS_TRUSTED(ch) && (( how_close(ch->in_room, victim->in_room, distance) < 0 )) )
//    || (how_close(victim->in_room, ch->in_room, distance) < 0)) )
  {
    send_to_char("&+yYou failed.\r\n", ch);
    return;
  }
  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if ((IS_AFFECTED(tmp, AFF_BLIND) ||
         (tmp->specials.z_cord != ch->specials.z_cord) || (tmp == ch) ||
         !number(0, 5)) && (IS_PC(ch) && !IS_TRUSTED(ch)))
      continue;
    if (CAN_SEE(tmp, ch))
      act("&+C$n rides the wind and starts to move away.", FALSE, ch, 0, tmp,
          TO_VICT);
    else
      act
        ("&+CAn ethereal cloud forms and a gust of wind starts to move it away.",
         FALSE, ch, 0, tmp, TO_VICT);
    send_to_char(buf, tmp);
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  char_from_room(ch);
  char_to_room(ch, location, -1);

  for (tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if ((IS_AFFECTED(tmp, AFF_BLIND) || (tmp == ch) || !number(0, 5)) &&
        (IS_PC(ch) && !IS_TRUSTED(ch)))
      continue;
    if (CAN_SEE(tmp, ch))
      act("&+C$n appears in a gust of swirling wind.", FALSE, ch, 0, tmp,
          TO_VICT);
    else
      act("&+CAn ethereal cloud forms in a gust of swirling wind.", FALSE, ch,
          0, tmp, TO_VICT);
    send_to_char(buf, tmp);
  }
}

void spell_frost_beacon(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  P_obj    beacon;

  beacon = read_object(real_object(99), REAL);

  if (!beacon)
  {
    logit(LOG_DEBUG, "spell_frost_beacon(): obj x not loadable");
    return;
  }

  act("&+CYou conjure a block of ice and mentally link it to yourself.",
      FALSE, ch, beacon, 0, TO_CHAR);
  act("&+C$n conjures up a block of arcane ice and leaves it to melt.", FALSE,
      ch, beacon, 0, TO_ROOM);

  // 8 to 16 minutes at lvl 56.  Get spell 6th circle, so level 26 / 7 = 3 to 6 minutes when you get it.
  set_obj_affected(beacon, dice(level/7, 2) * WAIT_MIN, TAG_OBJ_DECAY, 0);

  beacon->value[0] = GET_PID(ch);

  obj_to_room(beacon, ch->in_room);
  SET_BIT(beacon->extra_flags, ITEM_SECRET);
}

void spell_vapor_strike(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  struct affected_type af;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
    return;

  bzero(&af, sizeof(af));

  if(!affected_by_spell(victim, SPELL_VAPOR_STRIKE))
  {
    send_to_char("&+cThe vapors have increased your precision.\r\n", victim);

    af.type = SPELL_VAPOR_STRIKE;
    af.duration = MAX(5, level / 2);
    af.modifier = dice(3, 3) + number(0, 2);
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);
  }
  else
  {
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_VAPOR_STRIKE)
      {
        af1->duration = MAX(5, (int)(level / 2));
        send_to_char("&+cYour &+Cvapor strike &+c ability was refreshed!\r\n", victim);
      }
    }
  }
}

void spell_forked_lightning(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int      dam;
  int num_missiles = 3;
  struct damage_messages messages = {
    "You unleash a powerful blast of &-L&+Bforked lightning&n directed at $N.",
    "Giant bolts of &-L&+Bforked lightning&n hit you square in the chest, sending you reeling.",
    "$n stretches out $s hand, unleashing a blast of deadly &-L&+Bforked lightning&n toward $N.",
    "Your deadly blast of &-L&+Bforked lightning&n ends $N's life abruptly.",
    "Your body goes numb as a bolt of &+Blightning&n ends your life.",
    "$n hits $N with such a blast of &-L&+Blightning&n that nothing is left but a pair of smoking boots.",
      0
  };

  if( GET_LEVEL(ch) >= 53 )
  {
    num_missiles++;
  }

  // dam should total: 9 * level +/- 25 .. but 3 forks -> 3 * level +/- 8..
  dam = 2 * level + number(30, 64);

  if( NewSaves(victim, SAVING_SPELL, 0) )
  {
    dam = (dam * 2) / 3;
  }

  while( num_missiles-- && spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, 0, &messages) == DAM_NONEDEAD)
  {
    ;
  }
}

struct frost_data
{
  int      level;
  int      round;
};

void event_frost_bolt(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct frost_data *fdata;

  int      dam;
  struct damage_messages messages = {
    "&+WYou send a bolt of &+Bchilling frost&n &+Wstreaking towards $N, which shatters on impact.",
    "&+BThe chill in your bones makes your body ache.",
    "&+b$N is hit by an intense blast of &+Bfrost&n&+W, sending shards of ice flying through the room.",
    "&+WYour &+Bfrost bolt&+W is simply too much for $N who collapses in a frozen lump.",
    "&+BThe intense cold is simply too much as it completely overwhelms your body.",
    "&+W$n's &+Bfrost bolt&+W is simply too much for $N who collapses in a frozen lump.",
      0
  };

  fdata = (struct frost_data *) data;

  // Actual damage @lvl 11: 9-12 | 6-8 saved, @lvl 30: 16-19 | 10-12 saved.
  dam = 4 * ( number(6, 9) + (fdata->level / 3) );
  if( NewSaves(victim, SAVING_SPELL, 0) )
    dam = (2 * dam ) / 3;

  if( spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_ALLGLOBES, &messages) == DAM_NONEDEAD )
  {
    // We will have an initil round 0 at cast, then round 1 on first pulse, and round 2 on second pulse, but no 3rd.
    if( ++(fdata->round) < 2 )
      add_event(event_frost_bolt, (3 * PULSE_SPELLCAST) / 2, ch, victim, NULL, 0, fdata, sizeof(struct frost_data));
  }
}

void spell_frost_bolt(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  P_obj    obj;
  struct frost_data fdata;
  struct affected_type af;
  int      dam;
  struct damage_messages messages = {
    "&+WYou send a bolt of &+Bchilling frost&n &+Wstreaking towards $N, which shatters on impact.",
    "&+BYou feel an intense cold that turns your body numb.",
    "&+b$N is hit by an intense blast of &+Bfrost&n&+W, sending shards of ice flying through the room.",
    "&+WYour &+Bfrost bolt&+W is simply too much for $N who collapses in a frozen lump.",
    "&+BThe intense cold is simply too much as it completely overwhelms your body.",
    "&+W$n's &+Bfrost bolt&+W is simply too much for $N who collapses in a frozen lump.",
      0
  };

  obj = read_object(50, VIRTUAL);
  if (!obj || ch->in_room == NOWHERE)
    return;

  set_obj_affected(obj, 800, TAG_OBJ_DECAY, 0);
  obj_to_room(obj, ch->in_room);

  if( level > 30 )
    level = 30;

  // Actual damage @lvl 11: 9-12 | 6-8 saved, @lvl 30: 16-19 | 10-12 saved.
  dam = 4 * ( number(6, 9) + (level / 3) );
  if( NewSaves(victim, SAVING_SPELL, 0) )
    dam = (2 * dam ) / 3;

  fdata.round = 0;
  fdata.level = level;

  if( !spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_ALLGLOBES, &messages) )
  {
    if (!NewSaves(victim, SAVING_SPELL, 2))
    {
      memset(&af, 0, sizeof(af));
      af.type = SPELL_CHILL_TOUCH;
      af.duration = 2;
      af.modifier = -1;
      af.location = APPLY_STR;
      affect_join(victim, &af, TRUE, FALSE);
    }
    add_event(event_frost_bolt, PULSE_VIOLENCE, ch, victim, NULL, 0, &fdata, sizeof(fdata));
  }
}

void spell_ethereal_form(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  struct affected_type af;
  int duration = (int)(8 + GET_LEVEL(ch) / 10 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 10);

  if (affected_by_spell(ch, SPELL_ETHEREAL_FORM))
  {
    struct affected_type *af1;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_ETHEREAL_FORM)
      {
        af1->duration = duration;
      }
    }
    send_to_char("&+LYour ethereal energy form is renewed\r\n", ch);
    return;
  }

  send_to_char("&+LYour physical substance seems to fade away...&n\r\n", ch);
  act("&+L$n physical substance seems to fade away...&n",
    FALSE, ch, 0, 0, TO_ROOM);
      
  memset(&af, 0, sizeof(af));
  af.type = SPELL_ETHEREAL_FORM;
  af.duration = duration;
  af.bitvector = AFF_INVISIBLE;
  af.bitvector2 = AFF2_PASSDOOR;
  af.bitvector4 = AFF4_PHANTASMAL_FORM;
  affect_to_char(ch, &af);
}

void spell_conjure_air(int level, P_char ch, char *arg, int type,
                       P_char victim, P_obj tar_obj)
{
  P_char   mob;
  int      sum, mlvl, lvl;
  int      i, j;
  int      charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
  struct follow_type *k;

  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {
    1102, "&+CA gust of wind solidifies into&n $n."},
    {
    43, "&+CA HUGE gust of wind solidifies into&n $n."}
  };

  if (CHAR_IN_SAFE_ROOM(ch))
  {
    send_to_char("A mysterious force blocks your conjuring!\r\n", ch);
    return;
  }

  for (k = ch->followers, i = 0, j = 0; k; k = k->next)
  {
    victim = k->follower;
    if (IS_ELEMENTAL(victim))
      if (GET_LEVEL(victim) < 50)
        i++;
      else
        j++;
  }

  if (i >= 1 || j >= 1)
  {
    send_to_char("You cannot control any more elementals!\r\n", ch);
    return;
  }

  if (number(0, 100) < 15 || GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_TEMPESTMAGUS))
    sum = 1;
  else
    sum = 0;

  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);

  if (!mob)
  {
    logit(LOG_DEBUG, "spell_conjure_elemental(): mob %d not loadable",
          summons[sum].mob_number);
    send_to_char("Bug in conjure elemental.  Tell a god!\r\n", ch);
    return;
  }

  GET_SIZE(mob) = SIZE_MEDIUM;
  mob->player.m_class = CLASS_WARRIOR;
  char_to_room(mob, ch->in_room, 0);

  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);

  mlvl = (level / 5) * 2;
  lvl = number(mlvl, mlvl * 3);

  if (GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_TEMPESTMAGUS))
          mob->player.level = BOUNDED(10, level, 51);
  else
      mob->player.level = BOUNDED(10, lvl, 45);

  if (GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_TEMPESTMAGUS))
  {
     GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = 
     ch->points.base_hit + (GET_LEVEL(ch) * 2) + charisma;
    /*   
     GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob) / 2, 45) + GET_LEVEL(mob) + charisma;
    */
  }
  else
  {
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob) / 2, 10) + GET_LEVEL(mob) + charisma;
  }

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);


  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 3;
  MonkSetSpecialDie(mob);       /* 2d6 to 4d5 */

  if (IS_PC(ch) &&              /*(GET_LEVEL(mob) > number((level - i * 4), level * 3 / 2)) */
      (GET_LEVEL(mob) > GET_LEVEL(ch)) && charisma < number(10, 100))
  {
    act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE,
        ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);

  }
  else
  {                             /* Under control */
    act("$N sulkily says 'Your wish is my command, $n!'", TRUE, ch, 0, mob,
        TO_ROOM);
    act("$N sulkily says 'Your wish is my command, master!'", TRUE, ch, 0,
        mob, TO_CHAR);

    int duration = setup_pet(mob, ch, 400 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if (duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }

  }
}

void spell_storm_empathy(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj tar_obj)
{
  struct affected_type af;

  bzero(&af, sizeof(af));

  af.type = SPELL_STORM_EMPATHY;
  af.duration = level;
  af.bitvector2 = AFF2_PROT_COLD | AFF2_PROT_LIGHTNING;

  affect_to_char(victim, &af);

  send_to_char("You feel protected from the storms!\r\n", victim);
}

void spell_greater_ethereal_recharge(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int healpoints;

  if( IS_PC_PET(victim) && GET_RACE(victim) != RACE_A_ELEMENTAL
    && GET_RACE(victim) != RACE_V_ELEMENTAL )
  {
    send_to_char("They are not composed of ethereal matter...\r\n", ch);
    return;
  }

  if (!IS_PC_PET(victim) && !IS_AFFECTED4(victim, AFF4_PHANTASMAL_FORM))
  {
    send_to_char("They are not composed of ethereal matter...\r\n", ch);
    return;
  }

  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, tar_obj);

  healpoints = number(150, (GET_LEVEL(ch) * 5));
  heal(victim, ch, healpoints, GET_MAX_HIT(victim));

  healCondition(victim, healpoints);

  if (victim != ch)
  {
    act("$n reaches out at $N, touching $M. ", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You reach out your hand and touch $N, greatly healing $s ethereal body.", TRUE, ch, 0, victim, TO_CHAR);
  }
  
  send_to_char("&+WYou feel the powers of greater ethereal matter recharge you..!\r\n", victim);
  act("$n's ethereal form shimmers and seems to glow with renewed strength.", FALSE, victim, 0, 0, TO_ROOM);

  update_pos(victim);
}


void spell_ethereal_recharge(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int healpoints;

  if( IS_PC_PET(victim) && GET_RACE(victim) != RACE_A_ELEMENTAL
    && GET_RACE(victim) != RACE_V_ELEMENTAL )
  {
    send_to_char("They are not composed of ethereal matter...\r\n", ch);
    return;
  }

  if (!IS_PC_PET(victim) && !IS_AFFECTED4(victim, AFF4_PHANTASMAL_FORM))
  {
    send_to_char("They are not composed of ethereal matter...\r\n", ch);
    return;
  }

  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, tar_obj);

  healpoints = MIN(150, dice(5, level));
  heal(victim, ch, healpoints, GET_MAX_HIT(victim));

  healCondition(victim, healpoints);
  
  if (victim != ch )
  {
    act("$n reaches out at $N, touching $M.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You reach out at $N and touch $M.", TRUE, ch, 0, victim, TO_CHAR);
  }
  
  send_to_char("&+WYou feel ethereal power recharge you!\r\n", victim);
  act("$n appears to gain strength from a cloud of ethereal matter.", FALSE, victim, 0, 0, TO_ROOM);

  update_pos(victim);
}

void spell_arcane_whirlwind(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj tar_obj)
{
  int      success = 0;
  struct affected_type *aff;

  if (IS_TRUSTED(victim))
  {
        send_to_char("&+ROh no you don't!\r\n", ch);
        return;
  }

  if (victim->affected)
  {
    struct affected_type *next;

    for (aff = victim->affected; aff; aff = next)
    {
      next = aff->next;

      if (number(0, 100) < 40)
      {

        if (IS_SPELL(aff->type) && skills[aff->type].spell_pointer &&
            !(skills[aff->type].targets & TAR_SELF_ONLY) &&
            !(skills[aff->type].targets & TAR_AGGRO) &&
            !(aff->flags & AFFTYPE_STORE))
        {
          success++;
          skills[aff->type].spell_pointer(GET_LEVEL(victim), ch, 0,
                                          SPELL_TYPE_SPELL, ch, 0);
          wear_off_message(victim, aff);
          affect_remove(victim, aff);
        }
      }
    }
  }

  if (success)
  {
    act
      ("&+WHuge &+Cgusts&n&+W of arcane wind sweep across the area, encircling $N then quickly returning back to $n.&n",
       FALSE, ch, 0, victim, TO_NOTVICT);
    act
      ("&+CArcane&+W wind from $n envelopes you, then returns, leaving you feeling drained.",
       FALSE, ch, 0, victim, TO_VICT);
    act
      ("&+WAs the &+Carcane &+Wwind hits $N and returns back you, you feel infused with stolen magic.",
       FALSE, ch, 0, victim, TO_CHAR);
  }
}

void spell_purge(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int      dam;
  struct damage_messages messages = {
    "&+CYou chant a harmonic spell, sending forth a blast of magical wind, purging $N.",
    "&+CYou feel your enchantments waver and dissipate from the onslaught of $n's purge.",
    "$n spell sends $N reeling by the powerful dispelling magic.",
    "You purge $N instantly, banishing it from this world and sending it back where it belongs.",
    "$N is simply overwhelmed by the incredible power of $n's purge, and vanishes.",
    ""
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || ch == victim )
    return;

  if (!GET_MASTER(victim) || GET_EXP(victim))
  {
    act("$N belongs to this world, $E cannot be affected.", FALSE, ch, 0,
        victim, TO_CHAR);
    return;
  }

  dam = 10 * (3 * level - GET_LEVEL(victim));

  spell_damage(ch, victim, dam, SPLDAM_GENERIC,
               RAWDAM_TRANCEVAMP | RAWDAM_BTXVAMP | RAWDAM_SHRTVMP, &messages); 
  // Changed the damage type here from SPLDAM_NEGATIVE to SPLDAM_GENERIC, since the negative type would heal undead pets. Zion 9/28/2008
}

void event_tupor_wake(P_char ch, P_char victim, P_obj obj, void *data)
{

  if(!victim ||
    !IS_ALIVE(victim))
  {
    return;
  }
  
  if(IS_AFFECTED4(victim, AFF4_TUPOR))
  {
    REMOVE_BIT(victim->specials.affected_by4, AFF4_TUPOR);
  }
  
  if(IS_AFFECTED(victim, AFF_SLEEP) )
  {
    REMOVE_BIT(victim->specials.affected_by, AFF_SLEEP);
  }

  do_wake(victim, NULL, CMD_WAKE);
  do_alert(victim, NULL, CMD_ALERT);
}

void spell_induce_tupor(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(ch == victim)
  {
    send_to_char("No can do!\r\n", ch);
    return;
  }

  act("&+WYou whisper a musical incantation attempting to induce a deep sleep upon $N.",
    TRUE, ch, 0, victim, TO_CHAR);

  if(IS_AFFECTED4(victim, AFF4_TUPOR))
  {
    send_to_char("Your victim is already in a tupor!\r\n", ch);
    return;
  }

  if(IS_AFFECTED(victim, AFF_SLEEP) ||
    IS_AFFECTED(victim, AFF_KNOCKED_OUT) ||
    (GET_STAT(victim) == STAT_SLEEPING) ||
    affected_by_spell(victim, SPELL_SLEEP) ||
    get_scheduled(victim, event_tupor_wake))
  {
    send_to_char("Your victim is already dreaming of a better life.\r\n", ch);
    return;
  }

  if(resists_spell(ch, victim))
  {
    return;
  }

// 10% of the time there is no save.
  if(number(0, 9))
  {
    if(NewSaves(victim, SAVING_SPELL, 5))
    {
      return;
    }
  }
  
  if(IS_ELITE(victim) ||
     IS_GREATER_RACE(victim))
  {
    send_to_char("You sense your victim cannot be forced into a tupor.\r\n",ch);
    return;
  }
  
  struct affected_type af;
  bzero(&af, sizeof(af));
  
  af.type = SPELL_SLEEP;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
  af.duration = (int) (5 * PULSE_VIOLENCE);
  af.bitvector = AFF_SLEEP;

  if(GET_OPPONENT(victim))
    stop_fighting(victim);
  if( IS_DESTROYING(victim) )
    stop_destroying(victim);
  affect_to_char(victim, &af);

  if (GET_STAT(victim) > STAT_SLEEPING)
  {
    act("&+WYou suddenly feel very sleepy...",
      TRUE, victim, 0, 0, TO_CHAR);
    act("&+W$n blinks dreamily, closes $s eyes and enters a deep sleep.",
      TRUE, victim, 0, 0, TO_ROOM);
    SET_POS(victim, GET_POS(victim) + STAT_SLEEPING);
  }

  if(number(0, 2) &&
    IS_NPC(victim))
  {
    act("&+W$N sleepily stares about blankly, forgetting why $n is here.",
      TRUE, ch, 0, victim, TO_ROOM);
    act("&+W$N sleepily stares about blankly, forgetting why you are here.",
      TRUE, ch, 0, victim, TO_CHAR);
    clearMemory(victim);
  }

  StopMercifulAttackers(victim);

  if(!get_scheduled(victim, event_tupor_wake))
  {
    add_event(event_tupor_wake, (PULSE_VIOLENCE * 5), ch, victim, NULL, 0, NULL, 0);
  }

}

struct tempest_data
{
  int      room;
  int      old_sect;
};

void spell_tempest_terrain(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj tar_obj)
{

  if (CHAR_IN_TOWN(ch))
  {
    send_to_char("This spell is too powerful to use around town.\n", ch);
    return;
  }
  if (world[ch->in_room].sector_type == SECT_AIR_PLANE)
  {
    send_to_char("This terrain is tempest enough!\n", ch);
    return;
  }
  if (world[ch->in_room].sector_type == SECT_ROAD)
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }
  if (world[ch->in_room].sector_type == SECT_INSIDE)
  {
    send_to_char("You need to see the sky to invoke the tempest powers.\n", ch);
    return;
  }

  send_to_char
    ("&+CYou raise your hands to the heavens and whisper arcane words, bending the landscape to your will.\r\n",
     ch);

  act
    ("&+C$n raises $s hands while chanting, and dense clouds start gathering around.",
     FALSE, ch, 0, 0, TO_NOTVICT);


  add_event(event_tempest_terrain, 2*PULSE_VIOLENCE, 0, NULL, NULL, 0, &ch->in_room, sizeof(ch->in_room));

}


void event_tempest_terrain(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct tempest_data tdata;
  
  tdata.room = *((int *) data);
  tdata.old_sect = world[tdata.room].sector_type;

  send_to_room("&+cThe landscape suddenly turns into a billowing cloudscape!\n", tdata.room);

  world[tdata.room].sector_type = SECT_AIR_PLANE;
  
  add_event(event_tempest_terrain_bye, 20*PULSE_VIOLENCE, 0, NULL, NULL, 0, &tdata, sizeof(tdata));
}

void event_tempest_terrain_bye(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct tempest_data *tdata = (struct tempest_data *) data;
  
  world[tdata->room].sector_type = tdata->old_sect;

  send_to_room
    ("&+cThe landscape shimmers, the clouds disperse, and the terrain shifts back to normal...&n\r\n",
     tdata->room);
}

// Frost Magus spec

void spell_single_tempest(int level, P_char ch, char *args, int type,
                         P_char victim, P_obj obj)
{
double  takedown_chance, dam;
int door, target_room;

  struct damage_messages messages = {
  "$N&+W is savagely tossed about by the &+cfreezing &+Wsquall!",
  "&+WYou are savagely tossed around by the &+cfreezing &+Wsquall!",
  "$N&+W is savagely tossed around by the sudden and intense squall.",
  "$N&+W is shredded by &+Cicicles &+Wand violent wind!",
  "&+WYou feel so tired, &+wso very tired. &+LYou close your eyes...",
  "$N&+W is shredded and buffeted to death!", 0
  };

  if(!IS_ALIVE(victim) ||
    !IS_ALIVE(ch))
      return;

  dam = level * 5 + number(1, 20);
  
  /* Level 56 takedown_chance base % = 14% */
  takedown_chance = (int) (level / 4);

  /* Flying around in a squall is a bad idea. */
  if(IS_AFFECTED(victim, AFF_FLY))
  {
    act("&+wFlying right now is a &+rbad idea...&n", FALSE, ch, 0, victim, TO_VICT);
    dam *= 1.2;
    takedown_chance *= 1.5; //Level 56 % = 21%
  }

  /* Increase damage and takedown probability if wind tunnel is present. */
  if(get_spell_from_room(&world[ch->in_room], SPELL_WIND_TUNNEL))
  {
    dam *= 1.2;
    takedown_chance *= 1.3; //Level 56 % = 27%
  }

  /*  Reduced damage and takedown probability if binding wind is present. */
  if(get_spell_from_room(&world[ch->in_room], SPELL_BINDING_WIND))
  {
    dam /= 1.2;
    takedown_chance /= takedown_chance * 1.3;
  }

  /* Indoors and non precipitation rooms are poor terrains. */
  if(world[ch->in_room].sector_type == SECT_INSIDE ||
    IS_ROOM(ch->in_room, ROOM_INDOORS))
  {
    dam /= 2;
    takedown_chance /= 1.1;
  }
  
  /* Victim might get lucky */
  if((GET_C_LUK(victim) / 10) < number(1, 100))
    takedown_chance /= 2;

  dam = dam * get_property("spell.area.damage.factor.squallTempest", 1.000);
  
  if(spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_NODEFLECT, &messages) ==
      DAM_NONEDEAD);
  {
  /* Room kick code from actoff.c
  * Maximum probability is 27% for a room kick.*/
    if(takedown_chance > number(0, 100))
    {
      door = number(0, 9);
      if ((door == DIR_UP) || (door == DIR_DOWN))
        door = number(0, 3);
        if ((CAN_GO(victim, door)) && (!check_wall(victim->in_room, door)))
        {
          act("&+LA mighty &+Wwind&+L sends&n $N &+Lflying out of the room!&n",
            FALSE, ch, 0, victim, TO_CHAR);
          act("&+LA mighty &+Wwind&+L sends you flying out of the room!&n",
            FALSE, ch, 0, victim, TO_VICT);
          act("&+LThe mighty &+Wwind&+L sends&n $N &+Lflying out of the room!&n",
            FALSE, ch, 0, victim, TO_NOTVICT);
          target_room = world[victim->in_room].dir_option[door]->to_room;
          char_from_room(victim);
          if( char_to_room(victim, target_room, -1))
          {
            act("$n flies in looking slightly dazed!", TRUE, victim, 0, 0, TO_ROOM);
            CharWait(victim, PULSE_VIOLENCE * 1);
          }
        }
    }
  }
}

void spell_tempest(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int room = ch->in_room;

  if( !IS_ALIVE(ch) || room == NOWHERE )
    return;

  act("&+yYou raise your hands to the sky calling on the winds of the north to send forth a &+cfreezing&+y squall!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n &+Wraises &+y$s hands to the sky calling forth a &+cfreezing &+ysquall.", FALSE, ch, 0, 0, TO_NOTVICT);

  cast_as_damage_area(ch, spell_single_tempest, level, victim,
  get_property("spell.area.minChance.squallTempest", 90),
  get_property("spell.area.chanceStep.squallTempest", 10));

  if( world[room].sector_type == SECT_INSIDE || IS_ROOM(room, ROOM_INDOORS) )
    act("&+wYou have difficulty summoning the squall.&n",
      FALSE, ch, 0, victim, TO_CHAR);

  zone_spellmessage(ch->in_room, FALSE,"&+wYou feel a bitterly &+Ccold&+w wisp of air.\r\n",
                                "&+wYou feel a bitterly &+Ccold&+w wisp of air from the %s.\r\n");

  CharWait(ch, (int) (PULSE_SPELLCAST * 1));
}

void spell_wind_rage(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  if (affected_by_spell(ch, SPELL_WIND_RAGE))
  {
    send_to_char("&+cYou are already infused with the fury of the wind!\n", ch);
    return;
  }

  struct affected_type af;

  bzero(&af, sizeof(af));
  af.type = SPELL_WIND_RAGE;
  af.duration = level / 3;
  af.location = APPLY_WIS_MAX;
  af.modifier = level / 4;
  affect_to_char(victim, &af);
  if (level >= 51)
  {
    af.location = APPLY_SPELL_PULSE;
    af.modifier = (level >= 56) ? -2 : -1;
    affect_to_char(victim, &af);
    send_to_char("&+cYour mystic incantation bestows you with the &+Cspeed &+cand fury of the winds!\r\n", victim);
  }
  else
    send_to_char("&+cYour mystic incantation bestows you with the fury of the winds!\r\n", victim);
}

// Windtalker spec

void spell_ethereal_alliance(int level, P_char ch, char *arg, int type,
                             P_char victim, P_obj tar_obj)
{
  P_char   tch, target;
  struct affected_type af1, af2;

  bzero(&af1, sizeof(af1));
  bzero(&af2, sizeof(af2));

  af1.type = SPELL_ETHEREAL_ALLIANCE;

  if (!ch || !victim || ch == victim) {
     return;
  }

  if (IS_NPC(victim))
  {
    send_to_char("I don't think so.\n", ch);
    return;
  }

  if (!is_linked_to(ch, victim, LNK_CONSENT))
  {
    send_to_char("They won't appreciate it.\n", ch);
    return;
  }

  if (get_linked_char(ch, LNK_ETHEREAL) || 
      get_linking_char(victim, LNK_ETHEREAL))
  {
    send_to_char("You already have an ethereal alliance.\n", ch);
    return;
  }


  if (!affected_by_spell(ch, SPELL_ETHEREAL_FORM))
  {
    send_to_char("You must be in ethereal form.\n", ch);
    return;
  }

  af1.type = SPELL_ETHEREAL_ALLIANCE;
  af1.duration = 32;
  af1.bitvector5 = AFF5_ETHEREAL_ALLIANCE;

  // Removing phantasmal form and passdoor from target,
  // that's way too powerful for a basher/shreader.
  af2.type = SPELL_ETHEREAL_ALLIANCE;
  af2.duration = 32;
  af2.bitvector = AFF_INVISIBLE;
  //af2.bitvector2 = AFF2_PASSDOOR;
  //af2.bitvector4 = AFF4_PHANTASMAL_FORM;
  af2.bitvector5 = AFF5_ETHEREAL_ALLIANCE;

  affect_to_char(victim, &af2);
  affect_to_char(ch, &af1);


  act("&+CYou form an ethereal connection with $N.", FALSE, ch, 0, victim,
      TO_CHAR);
  act("&+CYou form an ethereal connection with $n.", FALSE, ch, 0, victim,
      TO_VICT);


  link_char(ch, victim, LNK_ETHEREAL);
  return; 
}

// Cosmomancer spec

void spell_comet(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int    temp, dam;
  struct damage_messages messages = {
    "&+LYou open a rift in the &+Yco&+Wsm&+Yos&+L and call forth a small &+Yco&+yme&+Yt&+L that &+Rsm&+ras&+Rhes &+Linto $N&+L!&n" ,
    "&+LA rift opens in front of you, and in it you see &+Yst&+War&+Ys &+Land &+Cgalaxies&+L.  Suddenly, a &+Yco&+yme&+Yt &+Lflies out of it &+Rsm&+rash&+Ring &+Linto YOU!&n" ,
    "&+LA large rift opens in front of $N&+L, you see &+Yst&+War&+Ys &+Lthrough it, and then a &+Yco&+yme&+Yt &+Lflies forth &+Rsmashing &+Linto $M!&n" ,
    "&+LYou open a rift into space and call forth a comet, and an exceptionally &+RLARGE &+Yco&+yme&+Yt &+Lsails through the rift and through the &+rrem&+Rnan&+rts &+Lof $N&+L.&n",
    "&+LA black rift opens before you, showing you &+Yst&+yar&+Ys&+L.  Suddenly a &+Rmassive &+Yco&+yme&+Yt &+Lflies through it and you...ending your &+Rl&+rif&+Re&+L...&n",
    "&+LA black rift opens before $N &+Land out comes a &+RMASSIVE &+Yco&+yme&+Yt &+Lutterly &+Rpul&+rver&+Rizi&+rng &+L$M!&n",
      0
  };

  if( !GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_STARMAGUS) )
  {
    temp = MIN(50, (level + 1));
    // Fireball damage.
    dam = dice(((int)(level/3) + 5), 6) * 4;
  }
  else
  {
    // A little more than fireball damage (d8 instead of d6 and + 10 dam).
    dam = dice(((int)(level/3) + 5), 8) * 4 + 40;
  }

  if( !NewSaves(victim, SAVING_SPELL, level/7) )
  {
    dam = (int)(dam *  1.2);
  }

  spell_damage(ch, victim, (int) dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_cosmic_vacuum(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int success_chance;
  struct affected_type af;

  if( affected_by_spell(victim, SPELL_COSMIC_VACUUM) )
  {
    send_to_char("&+cThey are already affected by a cosmic vacuum!\r\n", ch);
    return;
  }
  else if( resists_spell(ch, victim) )
  {
    return;
  }

  success_chance = BOUNDED(50, 70 + GET_LEVEL(ch) - GET_LEVEL(victim), 80);
  if( success_chance <= number(1, 100) )
  {
    return;
  }

  memset(&af, 0, sizeof(af));

  if( NewSaves(victim, SAVING_SPELL, 0) )
  {
    af.flags = AFFTYPE_SHORT;
    // Saved -> 1/3 level seconds
    af.duration = level / 3 * WAIT_SEC;
  }
  else
  {
    // Failed save -> level / 10 minutes.
    af.duration = level / 10;
  }

  af.type = SPELL_COSMIC_VACUUM;
  // Set AC to 100 if AC + level goes over 100.
  af.modifier = (GET_AC(victim) + level) > 100 ? 100 - GET_AC(victim) : level;
  af.location = APPLY_AC;
  affect_to_char(victim, &af);

  // At level 55, it's 11-2=9 points vs curse at 10 points max for PC.
  // At level 60, it's the same as curse.  Requires item/NPC.
  af.modifier = level / 5 - 2;
  af.location = APPLY_CURSE;
  affect_to_char(victim, &af);

  act("&+LYou invoke &+Yco&+Wsm&+Yic&+L forces, opening a rift in space and creating a vacuum next to&n $N.",
    TRUE, ch, 0, victim, TO_CHAR);
  act("&+wThe &+Yco&+Wsm&+Yic &+Lvacuum's &+wpowerful force leaves you temporarily defenseless.",
    TRUE, ch, 0, victim, TO_VICT);
  act("$n &+wopens a &+Yco&+Wsm&+Yic&+L vacuum &+wby&n $N&+w, draining $M&+w of $S&+w defenses.",
    TRUE, ch, 0, victim, TO_NOTVICT);

  DamageStuff(victim, SPLDAM_NEGATIVE);
}

void spell_single_supernova(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int dam;
  struct affected_type af;
  struct damage_messages messages = {
    "&+LYou blast&n $N &+Lwith &+Yco&+Wsm&+Yic &+Lenergies!",
    "$n &+Lblasts you with &+Yco&+Wsm&+Yic &+Lenergies, stunning you!",
    "$n &+Rblasts&n $N&+w with &+Yco&+Wsm&+Yic&+L energies!",
    "$N is &+Rblasted&n by your focused energies!",
    "&+LYou are sent reeling by&n $n's &+Lcosmic energies!",
    "$n &+Gvaporizes&n $N to nothing with $s &+Yco&+Wsm&+Yic &+Lenergies!", 0
  };

  dam = 120 + level * 6 + number(1, 40);
  dam = dam * get_property("spell.area.damage.factor.superNova", 1.000);
  if (spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages))
    return;

  if (affected_by_spell(victim, SPELL_SUPERNOVA))
  {
    return;
  }

  int success_chance = BOUNDED(50, 70 + GET_LEVEL(ch) - GET_LEVEL(victim), 80);
  if (success_chance <= number(1, 100))
    return;

  memset(&af, 0, sizeof(af));

  if (NewSaves(victim, SAVING_SPELL, 0))
  {
    af.flags = AFFTYPE_SHORT;
    af.duration = level / 3 * WAIT_SEC;
  }
  else 
  {     
    af.duration = level / 10;
  }

  af.type = SPELL_SUPERNOVA;
  af.modifier = level / -10;
  af.location = APPLY_CON;
  affect_to_char(victim, &af);

  act("&+LYou are &+Yburnt &+Lby the &+Yco&+Wsm&+Yic &+Lenergies!", TRUE, ch, 0, victim, TO_VICT);
 
}


void spell_supernova(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int room;
  
  room = ch->in_room;

  if (!OUTSIDE(ch))
  {
    send_to_char("You must be outside to cast this spell!\n", ch);
    return;
  }

  act("&+LThrough your &+Yco&+Wsm&+Yic &+Lmanipulations you channel the powers of a distant &+Ynova &+Lright onto the battlefield!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n &+Lchannels the powers of a distant &+Ynova &+Lright onto the battlefield!", FALSE, ch, 0, 0, TO_ROOM);
  
  zone_spellmessage(ch->in_room, FALSE,
    "&n&+cThe He&+Wav&n&+cens themselves &-L&+Yflash&n&+c as a &+YSupernova&n&+c is unleashed nearby!&n\r\n",
    "&n&+cThe He&+Wav&n&+cens to the %s &-L&+Yflash&n&+c as a &+YSupernova&n&+c is unleashed nearby!&n\r\n" );
  cast_as_damage_area(ch, spell_single_supernova, level, victim, get_property("spell.area.minChance.superNova", 50), get_property("spell.area.chanceStep.superNova", 10));
  
  if (!is_char_in_room(ch, room)) 
    return;

  act("&+LExhausted, $n passes out!", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char("&+LExhausted, you pass out!\r\n", ch);
  KnockOut(ch, (1/10) * WAIT_SEC); 

}

void spell_ethereal_discharge(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int      dam, mod;
  P_char   tch, next_tch;
  struct affected_type *afp, *next_afp;
  struct damage_messages messages = {
    "$N&+L is sent reeling by the shockwave of your expelled e&+ct&+Ch&+ce&+Lreal energy!&n",
    "&n&+LThe e&+ct&+Ch&+ce&+Lreal shockwave from&n $n&+L slams into you with violent force!&n",
    "$n&+L sends&n $N&+L reeling with the e&+ct&+Ch&+ce&+Lreal shockwave!&n",
    "$N&+L is blasted by your shockwave!&n",
    "&n&+LA fierce shockwave of e&+ct&+Ch&+ce&+Lreal magic slams into you!&n",
    "$N&+L is overwhelmed by the e&+ct&+Ch&+ce&+Lreal shockwave, and collapses, d&+Ce&+Lad.&n", 0
  };

  if(!ch ||
     !IS_ALIVE(ch) ||
     !victim ||
     !IS_ALIVE(victim))
  {
    return;
  }

  if (!(afp = get_spell_from_char(ch, SPELL_ETHEREAL_FORM)))
  {
    send_to_char("&+LYou must be in et&+ch&+Ce&+cr&+Leal form.\n", ch);
    return;
  }

  act("&+LYou summon your internal e&+ct&+Ch&+ce&+Lreal energies and force them outwards &+Lv&+ci&+Co&+cl&+Lently.&n",
    FALSE, ch, 0, 0, TO_CHAR);
  act("$n &+Lreleases an e&+ct&+Ch&+ce&+Lreal shockwave!&n",
    FALSE, ch, 0, 0, TO_ROOM);

  dam = (int)((level + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5) / 2);

  wear_off_message(ch, afp);
  affect_remove(ch, afp);
  
  mod = (MAX(0, GET_LEVEL(ch) - GET_LEVEL(victim)) +
         (int)(GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 25));

  for (tch = world[ch->in_room].people; tch; tch = next_tch)
  {  
    next_tch = tch->next_in_room;
    
    if (grouped(tch, ch) || ch == tch)
      continue;
    
    for (afp = tch->affected; afp; afp = next_afp)
    {
      next_afp = afp->next;
      
      if(!(afp->flags & AFFTYPE_NODISPEL) &&
          (afp->type > 0))
      {
        if(!NewSaves(tch, SAVING_SPELL, mod) ||
           !number(0, 9))
        {
          wear_off_message(tch, afp);
          affect_remove(tch, afp);
          
          if(spell_damage(ch, tch, dam, SPLDAM_GENERIC,
              RAWDAM_NOKILL |SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD)
            break;
        }
      }
    }
  }
}


void spell_planetary_alignment(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  struct affected_type af;

  if (affected_by_spell(ch, SPELL_PLANETARY_ALIGNMENT))
  {
    send_to_char("&+LNothing seems to ha&+Cp&+Lpen.&n\n", ch);
    return;
  }

  if( !affect_timer(ch, get_property("timer.mins.planetaryAlignment", 3) * WAIT_MIN, SPELL_PLANETARY_ALIGNMENT) )
  {
    send_to_char("&+LYou are too tired to align celestial bodies.&n\n", ch);
    return;
  }

  act("&+LYou communicate to the &+Yco&+Wsm&+Yos&n&+L, reaching out mentally to the planets and moons...&n",
    FALSE, ch, 0, 0, TO_CHAR);
  act("$n &+Lgazes upwards towards the &+Yco&+Wsm&+Yos&n&+L and strains for control.&n",
    FALSE, ch, 0, 0, TO_ROOM);

  memset(&af, 0, sizeof(af));

  if (number(0,100) < 11)
  {
    af.type = SPELL_PLANETARY_ALIGNMENT;
    af.duration = level / 4;

    af.location = APPLY_STR_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_INT_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_WIS_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_POW_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_CHA_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    af.location = APPLY_LUCK_MAX;
    af.modifier = 15;
    affect_to_char(victim, &af);

    act("&+Land succeed in pulling them into alignment...&n",
      FALSE, ch, 0, 0, TO_CHAR);
    act("&+Lso perfectly that energies strengthen you immensely...&n",
      FALSE, ch, 0, 0, TO_CHAR);
    act("&+LYou are bathed in &+Yco&+Wsm&+Yic&n&+L power, which strengthening every part of you.&n",
      FALSE, ch, 0, 0, TO_CHAR);
  }
  else if (number(0,100) < 31)
  {
    af.type = SPELL_PLANETARY_ALIGNMENT;
    af.duration = level / 4;

    af.location = APPLY_STR_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    af.location = APPLY_CON_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    af.location = APPLY_INT_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    af.location = APPLY_WIS_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    af.location = APPLY_POW_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    af.location = APPLY_CHA_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    af.location = APPLY_LUCK_MAX;
    af.modifier = 10;
    affect_to_char(victim, &af);

    act("&+Land succeed in pulling them into alignment...&n",
      FALSE, ch, 0, 0, TO_CHAR);
    act("&+LYou are bathed in &+Yco&+Wsm&+Yic&n&+L power, strengthening every part of you.&n",
      FALSE, ch, 0, 0, TO_CHAR);
  }
  else if (number(0,100) < 51)
  {
    af.type = SPELL_PLANETARY_ALIGNMENT;
    af.duration = level / 5;
    af.modifier = 10;

    af.location = APPLY_STR;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI;
    affect_to_char(victim, &af);

    af.location = APPLY_CON;
    affect_to_char(victim, &af);

    af.location = APPLY_INT;
    affect_to_char(victim, &af);

    af.location = APPLY_WIS;
    affect_to_char(victim, &af);

    af.location = APPLY_POW;
    affect_to_char(victim, &af);

    af.location = APPLY_CHA;
    affect_to_char(victim, &af);

    af.location = APPLY_LUCK;
    affect_to_char(victim, &af);

    act("&+Land partially pulling them into alignment...&n",
      FALSE, ch, 0, 0, TO_CHAR);
    act("&+LYou are warmed by the &+Yco&+Wsm&+Yic&n&+L power.&n",
      FALSE, ch, 0, 0, TO_CHAR);
  }
  else if (number(0,100) < 80)
  {
    af.type = SPELL_PLANETARY_ALIGNMENT;
    af.duration = level / 5;

    af.location = APPLY_STR;
    af.modifier = 5;
    affect_to_char(victim, &af);

    af.location = APPLY_DEX;
    affect_to_char(victim, &af);

    af.location = APPLY_AGI;
    affect_to_char(victim, &af);

    af.location = APPLY_CON;
    affect_to_char(victim, &af);

    af.location = APPLY_INT;
    affect_to_char(victim, &af);

    af.location = APPLY_WIS;
    affect_to_char(victim, &af);

    af.location = APPLY_POW;
    affect_to_char(victim, &af);

    af.location = APPLY_CHA;
    affect_to_char(victim, &af);

    af.location = APPLY_LUCK;
    affect_to_char(victim, &af);

    act("&+Land nudge into alignment...&n",
      FALSE, ch, 0, 0, TO_CHAR);
    act("&+LThe &+Yco&+Wsm&+Yic&n&+L power brushes you.&n",
      FALSE, ch, 0, 0, TO_CHAR);
  }
  else
  {
    act("&n&+LYet you fail to bring them under your control.&n",
      FALSE, ch, 0, 0, TO_CHAR);
  }
}

void spell_single_polar_vortex(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "$N&+C is horrendously battered by the intense vortex!&n",
    "$n's &+Cchilling vortex encases you in a sheet of &+Bice&+C!&n",
    "$n&+C's chilling vortex encases&n $N&+C in a sheet of &+Bice&+C, causing $M to scream in pain!&n",
    "&+CYour polar vortex is too much for&n $N&+C, and $E gives up the fight for good.&n",
    "&+CYou are encased in a solid block of ice, and then the world fades to black...&n",
    "$N&+C is frozen solid! Where's Boba Fett?&n", 0
  };

  int dam = dice(level, 6) * 3;
  if (IS_PC(ch) && IS_PC(victim))
    dam = dam * get_property("spell.area.damage.to.pc", 0.5);
  dam = dam * get_property("spell.area.damage.factor.polarVortex", 1.000);

  if((!IS_AFFECTED3(victim, AFF3_COLDSHIELD) &&
      !NewSaves(victim, SAVING_PARA, 2)) ||
      (GET_RACE(victim) == RACE_THRIKREEN && !NewSaves(victim, SAVING_PARA, 0)))
  {
    struct affected_type af;

    send_to_char("&+CThe intense cold causes your entire body to freeze!\n", victim);
    act("&+CThe intense cold causes $n&+C's entire body to freeze!", FALSE, victim, 0, 0, TO_ROOM);
    act("$n &+Mceases to move.. still and lifeless.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char ("&+LYour body becomes like stone as the paralyzing takes effect.\n", victim);

    bzero(&af, sizeof(af));
    af.type = SPELL_MAJOR_PARALYSIS;
    af.flags = AFFTYPE_SHORT;
    af.duration = WAIT_SEC * 3;
    af.bitvector2 = AFF2_MAJOR_PARALYSIS;

    affect_to_char(victim, &af);

    if (IS_FIGHTING(victim))
      stop_fighting(victim);
    if (IS_DESTROYING(victim))
      stop_destroying(victim);
  }

  send_to_char("&+CYou find no shelter from the savage vortex!&n\n", victim);
  spell_damage(ch, victim, dam, SPLDAM_COLD, 0, &messages);
}

void spell_polar_vortex(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  if (victim == ch)
  {
    send_to_char("You suddenly decide against that, oddly enough.\n", ch);
    return;
  }

  send_to_char("&+WYou call upon the chilling winds of the north, summoning a mighty storm!&n\n", ch);
  act("&+WThe wind in the room starts to pick up, as&n $n&+W completes $s incantation.&n", FALSE, ch, 0, 0, TO_ROOM);
 
  cast_as_damage_area(ch, spell_single_polar_vortex, level, victim,
                      get_property("spell.area.minChance.polarVortex", 50),
                      get_property("spell.area.chanceStep.polarVortex", 20));

  zone_spellmessage(ch->in_room, FALSE,
    "&+CA blast of &+cfreezing &+Cair swirls violently through the area.\r\n",
    "&+CA blast of &+cfreezing &+Cair from the %s swirls violently through the area.\r\n");
}

void spell_ethereal_travel(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      from_room;
  int      to_room;
  P_char   targetChar;
  int      tries = 0;
  int      location;
  struct group_list *gl = 0;

 location = victim->in_room;

 if (IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) ||
      IS_HOMETOWN(ch->in_room))
  {
    send_to_char("The magic in this room prevents you from leaving.\n", ch);
    return;
  }
 
  if (world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("Yeah, break out the bathing suit and...idiot.\n", ch);
    return;
  }
  
  if (IS_NPC(victim) || (victim == ch))
  {
    send_to_char("Can only travel to another player.\n", ch);
    return;
  }
  if (IS_PC_PET(ch) && IS_PC(victim))
  {
    send_to_char("won't work.\n", ch);
    return;
  }
  if (IS_ROOM(location, ROOM_NO_TELEPORT) ||
      IS_HOMETOWN(location))
  {
    send_to_char("The magic in that room prevents you from entering.\n", ch);
    return;
  }
  
  if((ch && !is_Raidable(ch, 0, 0)) ||
     (victim && !is_Raidable(victim, 0, 0)))
  {
    send_to_char("&+WYou or your target is not raidable. The spell fails!\r\n", ch);
    return;
  }

  // get the room the teleport is taking place so we don't have to move the teleporter last
  from_room = ch->in_room;

  // if the teleporter is grouped
  if (ch->group)
  {

    // get the character's group list
    gl = ch->group;

    // teleport the group members in the character's room
    for (gl; gl; gl = gl->next)
    {
      if (gl->ch->in_room == from_room)
      {

        // show the room they're fading
        act("$n&+W slowly fades into the ethereal mists.&n", FALSE, gl->ch, 0, 0,
            TO_ROOM);

        // if they're fighting, break it up
        if (IS_FIGHTING(gl->ch))
          stop_fighting(gl->ch);
        if (IS_DESTROYING(gl->ch))
          stop_destroying(gl->ch);

        // move the char
        char_from_room(gl->ch);
        char_to_room(gl->ch, location, -1);

        // show the new room they've arrived
        act("&+WA HUGE ethereal portal opens up, and&n $n&+W emerges from the mists!&n", FALSE, gl->ch, 0, 0, TO_ROOM);
	CharWait(gl->ch, 2 * PULSE_VIOLENCE); 
	
      }
    CharWait(ch, 2.5 * PULSE_VIOLENCE);
    }
  }
  else
  {
    // show the room they're fading
    act("$n&+W slowly fades into the ethereal mists.&n", FALSE, ch, 0, 0, TO_ROOM);

    // if they're fighting, break it up
    if (IS_FIGHTING(ch))
      stop_fighting(ch);
    if( IS_DESTROYING(ch) )
      stop_destroying(ch);

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

    // move the char
    char_from_room(ch);
    char_to_room(ch, location, -1);

    // show the new room they've arrived
    act("&+WA HUGE ethereal portal opens up, and&n $n&+W emerges from the mists!&n", FALSE, ch, 0, 0, TO_ROOM);
  }
}

void spell_single_cosmic_rift(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "&+LThe cosmic rift bends the fabric space and time around&n $N&+L, causing $M extreme pain!&n",
    "&+LTime seems to come to a stop, and as it resumes, you a filled with sheer &+RPAIN&+L!&n",
    "$N&+L is nearly consumed by the black hole!&n",
    "$N&+L tries to resist the power of your cosmic rift, but alas, $E succumbs to it's might.&n",
    "&+LThe cosmic rift is too much, and you feel yourself being swallowed whole by it...&n",
    "$N&+L is consumed completely by the cosmic rift!&n", 0
  };
  int dam = dice(number(25, 75), 4) * 4;
  if (IS_PC(ch) && IS_PC(victim))
    dam = dam * get_property("spell.area.damage.to.pc", 0.5); 
  dam = dam * get_property("spell.area.damage.factor.cosmicRift", 1.000);

  if (GET_HIT(victim) < dam / 4)
  {
    int id = -1;

    if( IS_PC(victim) )
      id = GET_PID(victim);
    else
      id = GET_VNUM(victim);
          
    act("$N &+Ltries to resist the power of your cosmic rift, but alas, $E succumbs to it's might.&n",
      FALSE, ch, 0, victim, TO_CHAR);
    act("&+LThe cosmic rift is too much, and you feel yourself being swallowed whole by it...&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("$N&+L is consumed completely by the cosmic rift!&n",
      FALSE, ch, 0, victim, TO_NOTVICT);

    die(victim, ch);
    victim = NULL;

    for( P_obj obj = world[ch->in_room].contents; obj; obj = obj->next_content )
    {
      if( obj->value[3] == id )
      {
        obj_from_room(obj);
        obj_to_room(obj, real_room(number(ASTRAL_VNUM_BEGIN,ASTRAL_VNUM_END)));
        if (obj->type == ITEM_CORPSE && IS_SET(obj->value[1], PC_CORPSE))
          writeCorpse(obj);
      }
    }
  } 
  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_cosmic_rift(int level, P_char ch, char *arg, int type, P_char victim,
                     P_obj obj)
{
  cast_as_damage_area(ch, spell_single_cosmic_rift, level, victim,
                      get_property("spell.area.minChance.cosmicRift", 0),
                      get_property("spell.area.chanceStep.cosmicRift", 20));
  zone_spellmessage(ch->in_room, FALSE,
                     "&+LThe air visibly ripples and distorts.\r\n",
                     "&+LThe air to the %s visibly ripples and distorts.\r\n");
}

struct static_discharge_data
{
  int      level;
  int      intensity;
  int      round;
  bool     discharge_now;
  P_char   caster;
};

void static_discharge(P_char ch, P_char victim, int level, int intensity)
{
  int dam;

  struct damage_messages messages = {
    "&+LThe &+Cel&+cec&+Ctr&+cic&+Cit&+cy&N &+Lsurrounding&n $N &+ydischarges &+Lin a huge burst of &+cli&+Cgh&+ctn&+Cin&+cg!",
    "&+LThe &+Cel&+cec&+Ctr&+cic&+Cit&+cy&N &+Lsurrounding you suddenly &+ydischarges &+Lin a burst of &+cli&+Cgh&+ctn&+Cin&+cg!",
    "&+LThe &+Cel&+cec&+Ctr&+cic&+Cit&+cy&N &+Lsurrounding&n $N &+ydischarges &+Lin a huge burst of &+cli&+Cgh&+ctn&+Cin&+cg.",
    "$N &+Lsuddenly &+Rex&+Ypl&+Rod&+Yes &+Lamongst a cloud of &+cli&+Cgh&+ctn&+Cin&+cg, &+Lleaving behind $S &+rcha&+Rrre&+rd re&+Rmai&+rns.",
    "&+LThe &+Cel&+cec&+Ctr&+cic&+Cit&+cy&N &+Lsurrounding you &+ydischarges &+Lin a huge burst of &+cli&+Cgh&+ctn&+Cin&+cg! R.I.P.",
    "$N &+Lis overwhelmed by a burst of &+cli&+Cgh&+ctn&+Cin&+cg, &+Land leaves behind $S &+rcha&+Rrre&+rd re&+Rmai&+rns.",
      0
  };
  
  level = MIN(46, level);
  dam = (dice(((level / 5) + 5) * intensity, 5) * 3);

  for (int i = intensity;i;i--)
  {
    if(!NewSaves(victim, SAVING_SPELL, intensity-1))
      dam += (int) (dam*0.10/intensity);
  }
    
  if (IS_AFFECTED5(victim, AFF5_WET))
    dam = (int) (dam*1.5);

  play_sound(SOUND_SHOCKWAVE, NULL, victim->in_room, TO_ROOM);

  uint flags = SPLDAM_NODEFLECT | SPLDAM_GLOBE | SPLDAM_GRSPIRIT;

  if (intensity >= 5)
    flags |= SPLDAM_NOSHRUG;
  
  if (intensity >= 3)
    flags &= SPLDAM_GLOBE;
  
  if (intensity >= 2)
    flags &= SPLDAM_GRSPIRIT;

  spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, flags, &messages);
  
}

void event_static_discharge(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct static_discharge_data *sdata;
  struct affected_type afp;

  sdata = (struct static_discharge_data *) data;

  if( sdata->discharge_now || !number(0, 4-sdata->intensity) || (++( sdata->round ) > 5) )
    static_discharge(victim, ch, sdata->level, sdata->intensity);
  else
  {
    act("&+WThe &+cli&+Cgh&+ctn&+Cin&+cg &+Wsurrounding&n $n &+Yglo&+yws b&+Yrie&+yfly.",
      TRUE, ch, 0, victim, TO_ROOM);
    act("&+WThe &+cli&+Cgh&+ctn&+Cin&+cg &+Wsurrounding you &+Yglo&+yws b&+Yrie&+yfly.",
      TRUE, ch, 0, 0, TO_CHAR);
    add_event(event_static_discharge, PULSE_VIOLENCE, ch, victim, NULL, 0, sdata, sizeof(struct static_discharge_data));
  }
}

void spell_static_discharge(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct static_discharge_data sdata;
  struct static_discharge_data *tmpsdata;
  struct affected_type af, afp;
  P_nevent e1 = 0;
  int delay = 0;
  int charges_active = 0;

  sdata.level = GET_LEVEL(ch);
  sdata.intensity = 1;
  sdata.round = 0;
  sdata.discharge_now = FALSE;
  sdata.caster = ch;

  bool found = FALSE;
  LOOP_EVENTS_CH( e1, victim->nevents )
  {
      //wizlog(56, "victim: found event: %s", get_function_name((void*)e1->func));
    if( e1->func == event_static_discharge )
    {
      //wizlog(56, "found it");
      found = TRUE;
      break;
    }
  }

  if( found )
  {
    tmpsdata = (struct static_discharge_data *) e1->data;
    sdata.level += tmpsdata->level;
    sdata.intensity += tmpsdata->intensity;
    disarm_char_nevents(victim, event_static_discharge);
  }

  if( IS_AFFECTED5(victim, AFF5_WET) || IS_WATER_ROOM(victim->in_room) )
  {
    act("&+BThe moisture around&n $n &+Bcauses the &+cli&+Cgh&+ctn&+Cin&+cgs &+Bto discharge immediately!",
      TRUE, victim, 0, 0, TO_ROOM);
    act("&+BThe moisture around you causes the &+cli&+Cgh&+ctn&+Cin&+cgs &+Bto discharge immediately!",
      TRUE, victim, 0, 0, TO_CHAR);
    sdata.discharge_now = TRUE;
  }

  delay = PULSE_VIOLENCE;

  if (found)
  {
    if (!number(0, 4-sdata.intensity))
    {
      delay = 0;
    }
    else
    {
      act("&+WA web of &+cli&+Cgh&+ctn&+Cin&+cg &+Waround $n &+Bintensifies!", TRUE, victim, 0, 0, TO_ROOM);
      act("&+WA web of &+cli&+Cgh&+ctn&+Cin&+cg &+Waround you &+Bintensifies!", TRUE, victim, 0, 0, TO_CHAR);
    }
  }
  else
  {
    act("&+WA web of &+Ycr&+yac&+Lki&+cng &+clig&+Chtn&+cing &+Wappears around $n!", TRUE, victim, 0, 0, TO_ROOM);
    act("&+WA web of &+Ycr&+yac&+Lki&+cng &+clig&+Chtn&+cing &+Wappears around you!", TRUE, victim, 0, 0, TO_CHAR);
  }

  add_event(event_static_discharge, delay, victim, ch, NULL, 0, &sdata, sizeof(sdata));

}

// victim and ch have to be backwards for the event search to work right.. *sigh*
void event_razor_wind(P_char victim, P_char ch, P_obj obj, void *data)
{
  int temp, dam, duration;

  struct damage_messages messages = {
    "&+CThe &+Wwi&+wnd&+Wst&+wor&+Wm &+csends &+Ldebris &+cflying into $N&+c at high speed causing $M to &+Rb&+rle&+Red&+c.&n",
    "&+CThe &+Wwi&+wnd&+Wst&+wor&+Wm &+csends &+Ldebris &+cflying into you at high speed causing you to &+Rb&+rle&+Red&+c.&n",
    "&+CThe &+Wwi&+wnd&+Wst&+wor&+Wm &+csends &+Ldebris &+cflying into $N&+c at high speed causing $M to &+Rb&+rle&+Red&+c.&n",
    "&+cThe &+Rvi&+rol&+Ren&+rt &+Ww&+wi&+Wn&+wd&+Ws&+wt&+Wo&+wr&+Wm &+cproves to be too much for $N as the winds tear $M to &+Rpi&+rec&+Res&+c.&n",
    "&+RB&+rle&+Red&+rin&+Rg &+cprofusely, you succumb to the awesome power of the &+Ww&+wi&+Wn&+wd&+C.&n",
    "&+cThe &+Rvi&+rol&+Ren&+rt &+Ww&+wi&+Wn&+wd&+Ws&+wt&+Wo&+wr&+Wm &+cproves to be too much for $N as the winds tear $M to &+Rpi&+rec&+Res&+c.&n",
      0
  };

  duration = *((int *) data);
  // 35-55 damage.
  dam = 180 + number( -40, 40);

  if( !NewSaves(victim, SAVING_SPELL, 3) )
  {
    dam = (int)(dam *  1.2);
  }

  if( spell_damage(ch, victim, (int) dam, SPLDAM_COLD, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD
    && --duration > 0 )
  {
    // This has to be backwards for the event search to work right.. *sigh*
    add_event(event_razor_wind, PULSE_SPELLCAST, victim, ch, NULL, 0, &duration, sizeof(int));
  }
  else if( IS_ALIVE(victim) && duration <= 0 )
  {
    send_to_char( "&+CThe &+Wwi&+wnd&+Wst&+wor&+Wm &+ccalms, the &+Ldebris &+cfalls, and the room becomes quiet.&n", victim );
  }
}

void spell_razor_wind(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int duration = 3;
  P_nevent e1 = NULL;
  bool found = FALSE;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  // Look for an event already going on.
  LOOP_EVENTS_CH( e1, victim->nevents )
  {
    if( e1->func == event_razor_wind )
    {
      found = TRUE;
      break;
    }
  }

  // If there's an event already going on, suck it's data and kill it!
  if( found )
  {
    duration = *((int *) e1->data) + 2;
    disarm_char_nevents(victim, event_razor_wind);
    act("&+cThe &+Ww&+wi&+Wn&+wd&+Ws&+wt&+Wo&+wr&+Wm &+csurrounding $N &+Cintensifies&+c!", TRUE, ch, 0, victim, TO_CHAR);
    act("&+cThe &+Ww&+wi&+Wn&+wd&+Ws&+wt&+Wo&+wr&+Wm &+csurrounding you &+Cintensifies&+c!", TRUE, ch, 0, victim, TO_VICT);
    act("&+cThe &+Ww&+wi&+Wn&+wd&+Ws&+wt&+Wo&+wr&+Wm &+csurrounding $N &+Cintensifies&+c!", TRUE, ch, 0, victim, TO_NOTVICT);
  }
  else
  {
    act("&+cYou summon forth &+Rv&+ri&+Ro&+rl&+Re&+rn&+Rt &+Ww&+wi&+Wn&+wd&+Ws &+cto batter $N&+c!", TRUE, ch, 0, victim, TO_CHAR);
    act("&+c$n summons forth &+Rv&+ri&+Ro&+rl&+Re&+rn&+Rt &+Ww&+wi&+Wn&+wd&+Ws &+cto batter you!&n", TRUE, ch, 0, victim, TO_VICT);
    act("&+cGales of &+Wvi&+wol&+Wen&+wt &+Ww&+wi&+Wn&+wd &+csurrounds $N&+c causing flying &+Ldebris &+cto shred and errode $S skin!", TRUE, ch, 0, victim, TO_NOTVICT);
  }

  // This has to be backwards for the event search to work right.. *sigh*
  add_event(event_razor_wind, PULSE_SPELLCAST, victim, ch, NULL, 0, &duration, sizeof(duration));
}

void spell_conjure_void_elemental(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  P_char   mob;
  int      sum, mlvl, lvl;
  int      i, j;
  int      charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
  struct follow_type *k;

  if (CHAR_IN_SAFE_ROOM(ch))
  {
    send_to_char("A mysterious force blocks your conjuring!\r\n", ch);
    return;
  }

  for (k = ch->followers, i = 0, j = 0; k; k = k->next)
  {
    victim = k->follower;
    if (IS_ELEMENTAL(victim))
      if (GET_LEVEL(victim) < 50)
        i++;
      else
        j++;
  }

  if (i >= 1 || j >= 1)
  {
    send_to_char("You cannot control any more elementals!\r\n", ch);
    return;
  }

  if (number(0, 100) < 15 || GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_STARMAGUS))
    sum = 1;
  else
    sum = 0;

  mob = read_mobile(real_mobile(76), REAL);

  if (!mob)
  {
    send_to_char("Bug in conjure elemental.  Tell a god!\r\n", ch);
    return;
  }

  GET_SIZE(mob) = SIZE_MEDIUM;
  mob->player.m_class = CLASS_WARRIOR;
  char_to_room(mob, ch->in_room, 0);

  act("&+LNegative energies coalesce and form a Voi&+wd Elemen&+Ltal!&n", TRUE, mob, 0, 0, TO_ROOM);

  mlvl = (level / 5) * 2;
  lvl = number(mlvl, mlvl * 3);

  if (GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_STARMAGUS))
          mob->player.level = BOUNDED(10, level, 51);
  else
      mob->player.level = BOUNDED(10, lvl, 45);

  if (GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_STARMAGUS))
  {
        GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob) / 2, 40) + GET_LEVEL(mob) + charisma;
  }
  else
  {
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob) / 2, 10) + GET_LEVEL(mob) + charisma;
  }

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);


  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 3;
  MonkSetSpecialDie(mob);       /* 2d6 to 4d5 */

  if (IS_PC(ch) &&              /*(GET_LEVEL(mob) > number((level - i * 4), level * 3 / 2)) */
      (GET_LEVEL(mob) > GET_LEVEL(ch)) && charisma < number(10, 100))
  {
    act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE,
        ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);

  }
  else
  {                             /* Under control */
    act("$N sulkily says 'Your wish is my command, $n!'", TRUE, ch, 0, mob,
        TO_ROOM);
    act("$N sulkily says 'Your wish is my command, master!'", TRUE, ch, 0,
        mob, TO_CHAR);

    int duration = setup_pet(mob, ch, 400 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if (duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

void spell_conjure_ice_elemental(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  P_char   mob;
  int      sum, mlvl, lvl;
  int      i, j;
  int      charisma = GET_C_CHA(ch) + (GET_LEVEL(ch) / 5);
  struct follow_type *k;

  if (CHAR_IN_SAFE_ROOM(ch))
  {
    send_to_char("A mysterious force blocks your conjuring!\r\n", ch);
    return;
  }

  for (k = ch->followers, i = 0, j = 0; k; k = k->next)
  {
    victim = k->follower;
    if (IS_ELEMENTAL(victim))
      if (GET_LEVEL(victim) < 50)
        i++;
      else
        j++;
  }

  if (i >= 1 || j >= 1)
  {
    send_to_char("You cannot control any more elementals!\r\n", ch);
    return;
  }

  if (number(0, 100) < 15 || GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_FROSTMAGUS))
    sum = 1;
  else
    sum = 0;

  mob = read_mobile(real_mobile(1157), REAL);

  if (!mob)
  {
    send_to_char("Bug in conjure elemental.  Tell a god!\r\n", ch);
    return;
  }

  GET_SIZE(mob) = SIZE_MEDIUM;
  mob->player.m_class = CLASS_WARRIOR;
  char_to_room(mob, ch->in_room, 0);

  act("&+WAn Ice Elemental forms from a cold gust of air.&n", TRUE, mob, 0, 0, TO_ROOM);

  mlvl = (level / 5) * 2;
  lvl = number(mlvl, mlvl * 3);

  if (GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_FROSTMAGUS))
          mob->player.level = BOUNDED(10, level, 51);
  else
      mob->player.level = BOUNDED(10, lvl, 45);

  if (GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_FROSTMAGUS))
  {
        GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob) / 2, 50) + GET_LEVEL(mob) + charisma;
  }
  else
  {
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit =
    dice(GET_LEVEL(mob) / 2, 10) + GET_LEVEL(mob) + charisma;
  }

  SET_BIT(mob->specials.affected_by, AFF_INFRAVISION);


  mob->points.base_hitroll = mob->points.hitroll = GET_LEVEL(mob) / 3;
  mob->points.base_damroll = mob->points.damroll = GET_LEVEL(mob) / 3;
  MonkSetSpecialDie(mob);       /* 2d6 to 4d5 */

  if (IS_PC(ch) &&              /*(GET_LEVEL(mob) > number((level - i * 4), level * 3 / 2)) */
      (GET_LEVEL(mob) > GET_LEVEL(ch)) && charisma < number(10, 100))
  {
    act("$N is NOT pleased at being suddenly summoned against $S will!", TRUE,
        ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);

  }
  else
  {                             /* Under control */
    act("$N sulkily says 'Your wish is my command, $n!'", TRUE, ch, 0, mob,
        TO_ROOM);
    act("$N sulkily says 'Your wish is my command, master!'", TRUE, ch, 0,
        mob, TO_CHAR);

    int duration = setup_pet(mob, ch, 400 / STAT_INDEX(GET_C_INT(mob)), PET_NOCASH);
    add_follower(mob, ch);
    /* if the pet will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if (duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

void spell_iceflow_armor(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  int      absorb = (level / 4) + number(1, 4);


  if(!IS_AFFECTED3(ch, AFF3_COLDSHIELD))
  {
    send_to_char("You lack the &+Cicy&n aura of &+Bcoldshield&n!\n", ch);
    return;
  }
  
  if(!has_skin_spell(ch))
  {
    absorb = (int) (absorb * 2);
    act("$n&+C is surrounded by a &+Bfrigid&+W aura&+C!&n",
       TRUE, victim, 0, 0, TO_ROOM);
    act("&+CFlowing &+Wi&+Cc&+we &+cencases you in a &+Bsolid &+cprotective barrier!&n",
       TRUE, victim, 0, 0, TO_CHAR);
  }
  else
  {
	send_to_char("You are already affected by a magical barrier!\n", ch);
    return;
  }


  bzero(&af, sizeof(af));
  af.type = SPELL_ICE_ARMOR;
  af.duration = 8;
  af.modifier = absorb;
  affect_to_char(victim, &af);
}

void spell_negative_feedback_barrier(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  int      absorb = (level / 4) + number(1, 4);


  if(!IS_AFFECTED4(ch, AFF4_NEG_SHIELD))
  {
    send_to_char("&+LYou lack the &+rnegative&+L energy to conjure a barrier!&n!\n", ch);
    return;
  }
  
  if(!has_skin_spell(ch))
  {
    absorb = (int) (absorb * 2);
    act("$n &+Lis encased in a &+rdark &+Lenergy field!&n",
       TRUE, victim, 0, 0, TO_ROOM);
    act("&+LGravitational &+wenergy &+Lbegins to &+ws&+Lwi&+wrl &+Laround you in a protective barrier.&n",
       TRUE, victim, 0, 0, TO_CHAR);
  }
  else
  {
	send_to_char("You are already affected by a magical barrier!\n", ch);
    return;
  }


  bzero(&af, sizeof(af));
  af.type = SPELL_NEG_ARMOR;
  af.duration = 8;
  af.modifier = absorb;
  affect_to_char(victim, &af);
}

void spell_etheric_gust(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char  t_ch;
  int tries = 0, to_room, dir;
  int range = get_property("spell.ethericgust.range", 5);

  if( !IS_ALIVE(ch) || ch->in_room == NOWHERE )
    return;

  if((IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) ||
      IS_HOMETOWN(ch->in_room) ||
      world[ch->in_room].sector_type == SECT_OCEAN) &&
      level < 60)
  {
    send_to_char("The magic in this room prevents you from leaving.\n", ch);
    return;
  }
  
  if(!victim)
    victim = ch;
    
  if((ch && !is_Raidable(ch, 0, 0)) ||
     (victim && !is_Raidable(victim, 0, 0)))
  {
    send_to_char("&+WYou are not raidable. The spell fails!\r\n", victim);
    return;
  }

  if(IS_MAP_ROOM(victim->in_room))
  {
    to_room = victim->in_room;

    for( int i = 0; i < range; i++ )
    {
      tries = 0;
      do
        dir = number(0,3);

      while(tries++ < 10 &&
             !VALID_TELEPORT_EDGE(to_room, dir, victim->in_room));

      if(tries < 10)
        to_room = TOROOM(to_room, dir);
    }
  }
  else
  {
    do
    {
      to_room = number(zone_table[world[victim->in_room].zone].real_bottom,
          zone_table[world[victim->in_room].zone].real_top);
      tries++;
    }
    while( (IS_ROOM(to_room, ROOM_PRIVATE) || PRIVATE_ZONE(to_room)
      || IS_ROOM(to_room, ROOM_NO_TELEPORT) || IS_HOMETOWN(to_room)
      || world[to_room].sector_type == SECT_OCEAN) && tries < 1000);
  }

  if(tries >= 1000)
    to_room = victim->in_room;

  if(LIMITED_TELEPORT_ZONE(victim->in_room))
  {
    if(how_close(victim->in_room, to_room, 5))
      send_to_char
        ("The magic gathers, but somehow fades away before taking effect.\n", victim);
    return;
  }
  
  act("&+cA strange &+Wwind &+csuddenly fills the area, and carries you away...&n",
    FALSE, victim, 0, 0, TO_CHAR);
  act("&+cA strange &+Wwind &+cbegins to blow and carries $n&+c away into a &+Cvortex!&n",
    FALSE, victim, 0, 0, TO_ROOM);
  
  if(IS_FIGHTING(victim))
    stop_fighting(victim);
  if(IS_DESTROYING(victim))
    stop_destroying(victim);
  
  if(victim->in_room != NOWHERE)
    for (t_ch = world[victim->in_room].people; t_ch; t_ch = t_ch->next)
      if(IS_FIGHTING(t_ch) &&
         GET_OPPONENT(t_ch) == victim)
            stop_fighting(t_ch);
  
  if(victim->in_room != to_room)
  {
    char_from_room(victim);
    char_to_room(victim, to_room, -1);
  }
  
  act("&+cand softly deposits you elsewhere!&n",
    FALSE, victim, 0, 0, TO_CHAR);  
  act("&+cA strange &+Wvortex &+cappears, and deposits $n&+c in the room!&n",
    FALSE, victim, 0, 0, TO_ROOM);
    
  CharWait(victim, (3 * WAIT_SEC));
}

// Event for spec nuke
// victim and ch have to be backwards for the event search to work right.. *sigh*
void event_antimatter_collision(P_char victim, P_char ch, P_obj obj, void *data)
{
  int temp, dam, duration;

  struct damage_messages messages = {
    "&+LThe black dots swarm and COLLIDE with $N&+L absorbing his &+Mfl&+mes&+Mh&+L!&n",
    "&+LTiny orbs of anti-matter &=LYCO&+WL&+YLI&+WD&+YE&n &+Linto you absorbing your very flesh!",
    "&+LThe black dots swarm and COLLIDE with $N&+L absorbing his &+Mfl&+mes&+Mh&+L!&n",
    "&+LYou &+Ws&+wh&+Wi&+wv&+We&+wr &+Las you watch the tiny black orbs absorb the majority of $N who drops dead and lifeless.",
    "&+LThe black orbs crash into you and invade your very center causing your &+Mbr&+mai&+Mn &+Land internal &+Ror&+rga&+Rns &+Lto shut down.&n",
    "&+LYou &+Ws&+wh&+Wi&+wv&+We&+wr &+Las you watch the tiny black orbs absorb the majority of $N who drops dead and lifeless.",
      0
  };

  duration = *((int *) data);
  // 40-60 damage.
  dam = 200 + number( -40, 40);

  if( !NewSaves(victim, SAVING_SPELL, 3) )
  {
    dam = (int)(dam *  1.2);
  }

  if( spell_damage(ch, victim, (int) dam, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD
    && --duration > 0 )
  {
    // This has to be backwards for the event search to work right.. *sigh*
    add_event(event_antimatter_collision, PULSE_SPELLCAST * 2, victim, ch, NULL, 0, &duration, sizeof(int));
  }
}

// Spec nuke.
void spell_antimatter_collision(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int duration = 3;
  P_nevent e1 = NULL;
  bool found = FALSE;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  // Look for an event already going on.
  LOOP_EVENTS_CH( e1, victim->nevents )
  {
    if( e1->func == event_antimatter_collision )
    {
      found = TRUE;
      break;
    }
  }

  // If there's an event already going on, suck it's data and kill it!
  if( found )
  {
    duration = *((int *) e1->data) + 2;
    disarm_char_nevents(victim, event_antimatter_collision);

    act("&+LMore small black orbs rush out of the rift crashing into $N!", TRUE, ch, 0, victim, TO_CHAR);
    act("&+LMore small black orbs come rushing out of the rift swarming you!", TRUE, ch, 0, victim, TO_VICT);
    act("&+LMore small black orbs rush out of the rift crashing into $N!", TRUE, ch, 0, victim, TO_NOTVICT);
  }
  else
  {
    act("&+wYou open up a &+Ymighty &+WCo&+Ysm&+Wi&+Yc &+Lrift &+win &+Lspace &+wand tiny &+Lblack dots &+wcome pouring out &+Yco&+Ll&+Yl&+Lid&+Ying &+wwith $N&+L!", TRUE, ch, 0, victim, TO_CHAR);
    act("&+LA black rift opens in which you see &+Ws&+Yta&+Wr&+Ys &+Land &+Yco&+yme&+Yts&+L when suddenly small black orbs come pouring out crashing into you!", TRUE, ch, 0, victim, TO_VICT);
    act("&+LA HUGE &+YCo&+Wsm&+Yic &+LRift opens and tiny black orbs come flying out and slam into $N&n!", TRUE, ch, 0, victim, TO_NOTVICT);
  }

  // This has to be backwards for the event search to work right.. *sigh*
  add_event(event_antimatter_collision, PULSE_SPELLCAST*2, victim, ch, NULL, 0, &duration, sizeof(int));
}

// Event for spec nuke
// victim and ch have to be backwards for the event search to work right.. *sigh*
void event_arctic_blast(P_char victim, P_char ch, P_obj obj, void *data)
{
  int temp, dam, duration;

  struct damage_messages messages = {
    "&+C$N&+C &+Wsh&+wud&+Wde&+wrs &+Cviolently as the &+Wi&+Cc&+we &+Bcold &+Wwinds &+Cslice through $M.&n",
    "&+CYou &+Wsh&+wud&+Wde&+wr &+Cviolently as the &+Wi&+Cc&+we &+Bcold &+Wwinds &+Cslice through you.&n",
    "&+C$N&+C &+Wsh&+wud&+Wde&+wrs &+Cviolently as the &+Wi&+Cc&+we &+Bcold &+Wwinds &+Cslice through $M.&n",
    "&+CThe &+Wi&+Cc&+wy &+Wwinds &+Cclaim the life of $N&+C...&n",
    "&+CThe &+Wfrozen gale &+Cproves to be too much, sapping the last bit of &+rheat &+Cfrom your body leaving you in a &+Bfrost &+Ccovered heap...&n",
    "&+CThe &+Wi&+Cc&+wy &+Wwinds &+Cclaim the life of $N&+C...&n",
      0
  };

  duration = *((int *) data);
  // 35-55 damage.
  dam = 180 + number( -40, 40);

  if( !NewSaves(victim, SAVING_SPELL, 3) )
  {
    dam = (int)(dam *  1.2);
  }

  if( spell_damage(ch, victim, (int) dam, SPLDAM_COLD, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD
    && --duration > 0 )
  {
    // This has to be backwards for the event search to work right.. *sigh*
    add_event(event_arctic_blast, PULSE_SPELLCAST * 2, victim, ch, NULL, 0, &duration, sizeof(int));
  }
  else if( IS_ALIVE(victim) && duration <= 0 )
  {
    send_to_char( "&+CThe &+Bcold &+Wwinds &+Csurrounding you subside.&n\n\r", victim );
  }
}

void spell_arctic_blast(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int duration = 3;
  P_nevent e1 = NULL;
  bool found = FALSE;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  // Look for an event already going on.
  LOOP_EVENTS_CH( e1, victim->nevents )
  {
    if( e1->func == event_arctic_blast )
    {
      found = TRUE;
      break;
    }
  }

  // If there's an event already going on, suck it's data and kill it!
  if( found )
  {
    duration = *((int *) e1->data) + 2;
    disarm_char_nevents(victim, event_arctic_blast);

    act("&+CThe &+Wfr&+Ce&+cez&+Win&+Cg &+Wwinds &+Caround $N &+Wintensify!", TRUE, ch, 0, victim, TO_CHAR);
    act("&+CThe &+Wfr&+Ce&+cez&+Win&+Cg &+Wwinds &+Caround you &+Wintensify!", TRUE, ch, 0, victim, TO_VICT);
    act("&+CThe &+Wfr&+Ce&+cez&+Win&+Cg &+Wwinds &+Caround $N &+Wintensify!", TRUE, ch, 0, victim, TO_NOTVICT);
  }
  else
  {
    act( "&+CYou open an &+Wi&+Cc&+wy &+cvortex &+Cin the ether and &+Wbl&+Ca&+Cs&+wt $N &+Cwith the &+BF&+bu&+BR&+by &+Cof &+WAu&+Cri&+wl&+C!", TRUE, ch, 0, victim, TO_CHAR);
    act( "&+cAn &+Carctic &+Wgale &+cbegins to blow unleashing the &+Bcold &+Cf&+Wu&+Cr&+Wy &+con YOU!", TRUE, ch, 0, victim, TO_VICT);
    act( "&+BCold &+cand &+Wi&+Cc&+wy &+Wwinds &+cbegin to &+wblow &+cand engulf $N&+c!&n", TRUE, ch, 0, victim, TO_NOTVICT);
  }

  // This has to be backwards for the event search to work right.. *sigh*
  add_event(event_arctic_blast, PULSE_SPELLCAST*2, victim, ch, NULL, 0, &duration, sizeof(int));

}

void spell_ice_spikes(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  int dam;
  int num_missiles = 3;
  struct damage_messages messages = {
    "&+CYou hurl a spike of &+Rr&+raz&+Ror &+Csharp &+Wice &+Cat $N&+C.",
    "&+CGiant Spikes of &+Wice&+C hit you square in the chest, impaling you!",
    "$n &+Cstretches out $s hand, unleashing a torret of &+Rr&+raz&+Ror &+Csharp &+Wice &+Ctoward $N&+C!&n",
    "&+CYour deadly &+Wspike &+Cof &+Wice &+Rimpales &+C$N&+C ending $S life abruptly.&n",
    "&+CYou clutch at the &+Wice spike &+rimpaling &+Cyour chest as the world fades &+cto &+Lblack...&n",
    "$n &+Chits $N&+C with one final &+rbarrage &+Cof &+Wice spikes &+Cand $E falls over clutching $S chest...&n",
    0
  };

  if( GET_LEVEL(ch) >= 53 )
  {
    num_missiles++;
  }

  // dam should total about: 9 * level +/- 25 .. but 3 spikes -> 3 * level +/- 8..
  dam = 2 * level + number(30, 64);

  if( NewSaves(victim, SAVING_SPELL, 0) )
  {
    dam = (dam * 2) / 3;
  }

  while( num_missiles-- && spell_damage(ch, victim, dam, SPLDAM_COLD, 0, &messages) == DAM_NONEDEAD )
  {
    ;
  }
}

void spell_wall_of_air(int level, P_char ch, char *arg, int type, P_char tar_ch, P_obj tar_obj)
{
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], dir_string[MAX_INPUT_LENGTH];
  int  var = 0;

  one_argument(arg, dir_string);
  var = dir_from_keyword(dir_string);

  if( !exit_wallable(ch->in_room, var, ch) )
  {
    send_to_char( "It's not possible to wall in that direction.\n", ch );
    return;
  }

  if( create_walls(ch->in_room, var, ch, level, WALL_OF_AIR, level, 1800,
    "&+wa wall of air&n",
    "&+WThe &+Cwinds &+Wappear to be especially strong coming from the %s&+W!&n", ITEM_NONE) )
  {

    snprintf(buf1, MAX_STRING_LENGTH, "&+WThe winds pick up &+Rviolently &+Wto the %s!&n\r\n", dirs[var]);
    snprintf(buf2, MAX_STRING_LENGTH, "&+WThe winds pick up &+Rviolently &+Wto the %s!&n\r\n", dirs[rev_dir[var]]);

    send_to_room(buf1, ch->in_room);
    send_to_room(buf2, (world[ch->in_room].dir_option[var])->to_room);
  }
}
