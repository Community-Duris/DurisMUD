
/* Welcome to Norabi's playhouse for Winterhaven mobs and eq procs */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <list>

#include "comm.h"
#include "db.h"
#include "damage.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "range.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "graph.h"
#include "justice.h"
#include "specs.winterhaven.h"
#include "reavers.h"
#include "assocs.h"
#include "specs.prototypes.h"
#include "specs.zion.h"
#include "objmisc.h"
#include "vnum.obj.h"

/*
 * external variables
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern P_obj justice_items_list;
extern char *coin_names[];
extern const char *command[];
extern const char *dirs[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int planes_room_num[];
extern int racial_base[];
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern const char *crime_list[];
extern const char *crime_rep[];
extern const char *specdata[][MAX_SPEC];
extern struct class_names class_names_table[];
extern P_obj    object_list;
extern int pulse;
extern bool has_skin_spell(P_char);

int range_scan_track(P_char ch, int distance, int type_scan);

/*
 * CREATION OF ADDITIONAL SKILL/SPELL OPPORTUNITIES
 */

/*
 * EVENT PROCS
 */

int wh_corpse_to_object(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_DEATH)
  {
    P_obj    obj;

    obj = read_object(GET_VNUM(ch), VIRTUAL);
    if (!(obj))
    {
      logit(LOG_EXIT, "winterhaven_object: death object for mob %d doesn't exist", GET_VNUM(ch));
      raise(SIGSEGV);
    }
    obj_to_room(obj, ch->in_room);

    obj->value[0] = SECS_PER_MUD_DAY / PULSE_MOBILE * WAIT_SEC;

    return FALSE;
  }
  return FALSE;
}

int wh_corpse_decay(P_obj obj, P_char ch, int cmd, char *args)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch || cmd)
    return FALSE;

  // Make it into a real corpse 
  if( obj->value[0]-- <= 0 )
  {
    P_obj    corpse;

    corpse = read_object(VOBJ_WH_ROTTING_CORPSE, VIRTUAL);
    if (!corpse)
    {
      logit(LOG_OBJ, "wh_corpse_decay: unable to load obj #%d", VOBJ_WH_ROTTING_CORPSE);
      return FALSE;
    }
    corpse->weight = obj->weight;
    set_obj_affected(corpse, get_property("timer.decay.corpse.npc", 120), TAG_OBJ_DECAY, 0);

    if (OBJ_CARRIED(obj))
    {
      P_char   carrier;

      carrier = obj->loc.carrying;
      send_to_char("Something smells real bad...\r\n", carrier);
      obj_to_char(corpse, carrier);

    }
    else if (OBJ_ROOM(obj))
    {
      send_to_room("Something smells real bad...\r\n", obj->loc.room);
      obj_to_room(corpse, obj->loc.room);

    }
    else if (OBJ_INSIDE(obj))
    {
      obj_to_obj(corpse, obj->loc.inside);
    }
    else
    {
      extract_obj(corpse);
    }
    extract_obj(obj, TRUE);
    return TRUE;
  }
  return FALSE;
}


/* 
 * MOB PROCS
 */

int winterhaven_shout_one(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 55241, 55255, 55258, 55259, 132520, 132521, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 5,
                          "&+GGuards! %s &+Gis invading our beloved city! To arms!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int winterhaven_shout_two(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 55240, 55256, 55257, 55260, 97560, 97562, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 5,
                          "&+GGuards! %s &+Gis invading our beloved city! To arms!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

/* 
 *  Object Procs 
 */


int storm_legplates(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  int curr_time;
  struct proc_data *data;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( IS_ALIVE(ch->specials.fighting) && ch->in_room == ch->specials.fighting->in_room )
  {
    if( arg && (cmd == CMD_SAY) )
    {
      if( isname(arg, "storm") )
      {
        curr_time = time(NULL);
        vict = ch->specials.fighting;
        // 10 min timer.
        if( obj->timer[0] + 600 <= curr_time )
        {
          if(OUTSIDE(ch))
          {
            act("&+WYou say '&+wI summon the &+Lst&+wo&+Lrm&+w!&+W'&n", TRUE, ch, obj, NULL, TO_CHAR);
            act("&+LA m&+Wassive &+Ls&+Wtatic &+Lc&+wharge &+Lb&+Wuilds &+Li&+Wn $q &+La&+Ws &+Ly&+Wou &+Ls&+Wtomp &+Ly&+wour &+Lf&+weet!&n", TRUE, ch, obj, obj, TO_CHAR);
            act("$n says '&+wI summon the &+Lst&+wo&+Lrm&+w!&+W'&n", TRUE, ch, obj, NULL, TO_NOTVICT);
            act("&+LA m&+Wassive &+Ls&+Wtatic &+Lc&+wharge &+Lb&+Wuilds &+Li&+Wn $q &+La&+Ws $N &+Ls&+Wtomps $s &+Lf&+weet!&n", TRUE, ch, obj, NULL, TO_NOTVICT);
            act("$n says '&+wI summon the &+Lst&+wo&+Lrm&+w!&+W'&n", TRUE, ch, obj, NULL, TO_VICT);
            act("&+LA m&+Wassive &+Ls&+Wtatic &+Lc&+wharge &+Lb&+Wuilds &+Li&+Wn $q &+La&+Ws $N &+Ls&+Wtomps $s &+Lf&+weet!&n", TRUE, ch, obj, NULL, TO_VICT);

            spell_call_lightning(56, ch, vict, 0);
            obj->timer[0] = curr_time;
            return TRUE;
          }
        }
      }
    }

    // 1/30 chance.
    if( cmd == CMD_GOTHIT && !number(0, 29) )
    {
      if( !(data = (struct proc_data *) arg) )
      {
        return FALSE;
      }
      vict = data->victim;
      if( !IS_ALIVE(vict) )
      {
        return FALSE;
      }

      act("$q &+wsizzles with &+Yenergy &+was $N tries to hit you.&n", TRUE, ch, obj, vict, TO_CHAR);
      act("$q &+wsizzles with &+Yenergy &+was you try to hit $n.&n", TRUE, ch, obj, vict, TO_VICT);
      act("$q &+wsizzles with &+Yenergy &+was $N &+wtries to hit $n&+w.&n", TRUE, ch, obj, vict, TO_NOTVICT);

      spell_lightning_bolt(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      spell_lightning_bolt(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);

      return TRUE;
    }
  }

  if( !IS_FIGHTING(ch) )
  {
    if (arg && (cmd == CMD_SAY))
    {
      if (isname(arg, "storm"))
      {
        curr_time = time(NULL);
        // 10 min timer.
        if( OUTSIDE(ch) && obj->timer[0] + 600 <= curr_time )
        {

          act("&+WYou say '&+wI summon the &+Lst&+wo&+Lrm&+w!&+W'&n", TRUE, ch, obj, NULL, TO_CHAR);
          act("&+LA m&+Wassive &+Ls&+Wtatic &+Lc&+wharge &+Lb&+Wuilds &+Li&+Wn $q &+La&+Ws &+Ly&+Wou &+Ls&+Wtomp &+Ly&+wour &+Lf&+weet!&n", TRUE, ch, obj, obj, TO_CHAR);
          act("$n says '&+wI summon the &+Lst&+wo&+Lrm&+w!&+W'&n", TRUE, ch, obj, NULL, TO_NOTVICT);
          act("&+LA m&+Wassive &+Ls&+Wtatic &+Lc&+wharge &+Lb&+Wuilds &+Li&+Wn $q &+La&+Ws $N &+Ls&+Wtomps $s &+Lf&+weet!&n", TRUE, ch, obj, NULL, TO_NOTVICT);
          act("$n says '&+wI summon the &+Lst&+wo&+Lrm&+w!&+W'&n", TRUE, ch, obj, NULL, TO_VICT);
          act("&+LA m&+Wassive &+Ls&+Wtatic &+Lc&+wharge &+Lb&+Wuilds &+Li&+Wn $q &+La&+Ws $N &+Ls&+Wtomps $s &+Lf&+weet!&n", TRUE, ch, obj, NULL, TO_VICT);

          spell_call_lightning(51, ch, NULL, 0);
          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }

    if( arg && (cmd == CMD_STOMP) )
    {
      if( isname(arg, "ground") )
      {
        curr_time = time(NULL);
        if( obj->timer[0] + 600 <= curr_time )
        {
          act("$q &+wsends a burst of &+Yelectricity &+wthrough your body.&n", TRUE, ch, obj, obj, TO_CHAR);
          act("You feel &+Bvitalized &+wand &+Cenergized&n.", TRUE, ch, obj, obj, TO_CHAR);
          act("$q &+wsends a burst of &+Yelectricity &+wthrough $n's body.&n", TRUE, ch, obj, NULL, TO_ROOM);
          act("$n looks &+Bvitalized &+wand &+Cenergized&n.", TRUE, ch, obj, NULL, TO_ROOM);

          GET_HIT(ch) += 150;
          spell_regeneration(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          spell_endurance(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);

          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

int blur_shortsword(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int curr_time, rand;
  struct proc_data *data;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if ( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( IS_FIGHTING(ch) )
  {
    if( arg && (cmd == CMD_SAY) )
    {
      if( isname(arg, "blur") )
      {
        curr_time = time(NULL);
        vict = ch->specials.fighting;
        // 10 min timer.
        if( obj->timer[0] + 600 <= curr_time)
        {
          act("&+LYour $q &+Lslows down time and freezes $n &+Lin place!&n", TRUE, ch, obj, NULL, TO_CHAR);
          act("&+L...you leap at $n &+Land deal a series of &+cvicious &+Lattacks!&n", TRUE, ch, obj, NULL, TO_CHAR);
          act("$n's $q &+Lslows down time and freezes you in place!&n", TRUE, ch, obj, NULL, TO_VICT);
          act("&+L...$n leaps at you and deals a series of &+cvicious &+Lattacks!&n", TRUE, ch, obj, NULL, TO_VICT);
          act("$n's $q &+Lslows down time and freezes&n $n &+Lin place!&n", TRUE, ch, obj, NULL, TO_NOTVICT);
          act("&+L...$n &+Lleaps towards&n $N &+Land deals a series of &+cvicious &+Lattacks!&n", TRUE, ch, obj, NULL, TO_NOTVICT);

          if( IS_ALIVE(ch) && ch->specials.fighting )
          {
            hit(ch, ch->specials.fighting, obj);
          }
          if( IS_ALIVE(ch) && ch->specials.fighting )
          {
            hit(ch, ch->specials.fighting, obj);
          }
          if( IS_ALIVE(ch) && ch->specials.fighting )
          {
            hit(ch, ch->specials.fighting, obj);
          }

          act("$Q &+Cglows &+Las it touches $N's &+Csoul&+L!&n", TRUE, ch, obj, NULL, TO_CHAR);
          act("$Q &+Cglows &+Las it touches your &+Csoul&+L!&n", TRUE, ch, obj, NULL, TO_VICT);
          act("$Q &+Cglows &+Las it touches $N's &+Csoul&+L!&n", TRUE, ch, obj, NULL, TO_NOTVICT);
          switch (number(0,2))
          {
          case 0:
            rand = number(1, 20);

            if (rand <= 15)
            {
              spell_frostbite(35, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            }
            else
            {
                spell_arieks_shattering_iceball(35, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            }
            break;
          case 1:
            spell_pword_stun(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            break;
          case 2:
            spell_pword_blind(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            break;
          }

          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }

    // 1/25 chance.
    if( cmd == CMD_GOTHIT && !number(0, 24) )
    {
      // important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED
      if( !(data = (struct proc_data *) arg) )
      {
        return FALSE;
      }
      vict = data->victim;
      if( !IS_ALIVE(vict) || vict != ch->specials.fighting )
      {
        return FALSE;
      }

      act("&+LAn &+Bicy &+chaze &+Lbillows out from $q&+L, &+Bslowing &+Ldown &n$N&+L's attack.", TRUE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);
      act("&+L...you swiftly dodge &n$N&+L's attack and turn to deliver a &+Cvicious &+Lstrike!", TRUE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);

      act("&+LAn &+Bicy &+chaze &+Lbillows out from $q+L, &+Bslowing &+Ldown your attack.&n", TRUE, ch, obj, vict, TO_VICT | ACT_NOTTERSE);
      act("&+L...&n$n &+Lswiftly dodges your attack, $m turns to deliver a &+Cvicious &+Lstrike!&n", TRUE, ch, obj, vict, TO_VICT | ACT_NOTTERSE);

      act("&+LAn &+Bicy &+chaze &+Lbillows out from $q&+L, &+Bslowing &+Ldown &n$N&+L's attack.", TRUE, ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);
      act("&+L...&n$n &+Lswiftly dodges &n$N&+L's attack and turns to deliver a &+Cvicious &+Lstrike!", TRUE, ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);

      hit(ch, vict, obj);

      act("$q &+Cglows &+Las it touches $n&+L's &+Csoul&+L!&n", TRUE, ch, obj, NULL, TO_CHAR);
      act("$q &+Cglows &+Las it touches your &+Csoul&+L!&n", TRUE, ch, obj, NULL, TO_VICT);
      act("$q &+Cglows &+Las it touches $n&+L's &+Csoul&+L!&n", TRUE, ch, obj, NULL, TO_NOTVICT);

      switch (number(0,2))
      {
      case 0:
        rand = number(1, 20);
        if( rand <= 12 )
        {
          spell_chill_touch(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          spell_chill_touch(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        }
        else if( rand <= 19 )
        {
          spell_frostbite(35, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        }
        else
        {
          spell_arieks_shattering_iceball(35, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        }
        break;
      case 1:
        spell_pword_stun(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 2:
        spell_pword_blind(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      }
      return TRUE;
    }
  }

  if( !IS_FIGHTING(ch) )
  {
    if( arg && (cmd == CMD_RUB) )
    {
      if( isname(arg, "misty") )
      {
        curr_time = time(NULL);
        // 10 min timer.
        if( obj->timer[0] + 600 <= curr_time )
        {
          act("&+LYour $q &+Chums &+Lloudly and surrounds you in a &+Cmisty &+chaze&+L.&n", TRUE, ch, obj, NULL, TO_CHAR);
          act("&n$N's $q &+Chums &+Lloudly and surrounds $m in a &+Cmisty &+chaze&+L.&n", TRUE, ch, obj, NULL, TO_VICT);
          act("&n$N's $q &+Chums &+Lloudly and surrounds $m in a &+Cmisty &+chaze&+L.&n", TRUE, ch, obj, NULL, TO_NOTVICT);

          spell_shadow_shield(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          spell_vanish(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);

          CharWait(ch,PULSE_VIOLENCE * 2);
          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

int volo_longsword(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  int curr_time;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if ( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
  	return FALSE;
  }

  if( IS_FIGHTING(ch) )
  {
    if (arg && (cmd == CMD_SAY))
    {
      if (isname(arg, "volo"))
      {
        curr_time = time(NULL);
        vict = ParseTarget(ch, arg);
        // 10 min timer.
        if( obj->timer[0] + 600 <= curr_time )
        {
          act("&+WYou say '&+cI s&+Lummon &+ct&+Lhe &+cT&+Lraveler!&+W'&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+cY&+Lou &+Lthrust $q &+Linto $N&+L's &+rfl&+Re&+rsh &+Land send a &+ct&+Lor&+cr&+Len&+ct &+Lof dark &+cp&+Llanar &+ce&+Lnergy into $S &+cs&+Lou&+cl&+L!&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+cV&+Lo&+cl&+Lo &+ct&+Lhe &+cT&+Lraveler reaches from beyond the &+cP&+Llanar &+cR&+Lealms and grabs a hold of $N&+L's soul!&n", TRUE, ch, obj, vict, TO_CHAR);

          act("$n says '&+cI s&+Lummon &+ct&+Lhe &+cT&+Lraveler!&+W'&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("$n &+Lthrusts $q &+Linto $N&+L's &+rfl&+Re&+rsh &+Land send a &+ct&+Lor&+cr&+Len&+ct &+Lof dark &+cp&+Llanar &+ce&+Lnergy into $S &+cs&+Lou&+cl&+L!&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("&+cV&+Lo&+cl&+Lo &+ct&+Lhe &+cT&+Lraveler reaches from beyond the &+cP&+Llanar &+cR&+Lealms and grabs a hold of $N&+L's soul!&n", TRUE, ch, obj, vict, TO_NOTVICT);

          act("$n says '&+cI s&+Lummon &+ct&+Lhe &+cT&+Lraveler!&+W'&n", TRUE, ch, obj, vict, TO_VICT);
          act("$n &+Lthrust $q &+Linto your &+rfl&+Re&+rsh &+Land send a &+ct&+Lor&+cr&+Len&+ct &+Lof dark &+cp&+Llanar &+ce&+Lnergy into your &+cs&+Lou&+cl&+L!&n", TRUE, ch, obj, vict, TO_VICT);
          act("&+cV&+Lo&+cl&+Lo &+ct&+Lhe &+cT&+Lraveler reaches from beyond the &+cP&+Llanar &+cR&+Lealms and grabs a hold of YOUR soul!&n", TRUE, ch, obj, vict, TO_VICT);

          spell_teleport(50, ch, 0, 0, vict, 0);

          act("$n slowly fades out of existence.&n", TRUE, ch, obj, vict, TO_ROOM);

          char_from_room(ch);
          char_to_room(ch, vict->in_room, -1);
          act("&+cV&+Lo&+cl&+Lo &+ct&+Lhe &+cT&+Lraveler grins wickedly as he throws you back into combat!&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+cV&+Lo&+cl&+Lo &+ct&+Lhe &+cT&+Lraveler grins wickedly as he throws you back into combat!&n", TRUE, ch, obj, vict, TO_VICT);
          attack(ch, vict);
          CharWait(ch,PULSE_VIOLENCE * 2);
          CharWait(vict,PULSE_VIOLENCE * 2);


          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }

    room = ch->in_room;
    vict = ParseTarget(ch, arg);
    // 1/33 chance.
    if( cmd == CMD_MELEE_HIT && IS_ALIVE(vict) && !number(0, 32) && CheckMultiProcTiming(ch) && !IS_ELITE(vict) && !IS_GREATER_RACE(vict) )
    {
      switch (number(0,2))
      {
      case 0:
        act("&+LYour $q &+Lsinks deep into &n$N&+L's &+rfl&+Re&+rsh&+L!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$n's $q &+Lsinks deep into &n$N&+L's &+rfl&+Re&+rsh&+L!&n", TRUE, ch, obj, vict, TO_NOTVICT);
        act("$n's $q &+Lsinks deep into your &+rfl&+Re&+rsh&+L!&n", TRUE, ch, obj, vict, TO_VICT);

        spell_energy_drain(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 1:
        act("&+LYour $q &+cg&+Llows &+cb&+Lrill&+ci&+Lantly!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+LYou feel the &+cwounds &+Land &+cfatigue &+Lof your body melt away.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$n's $q &+cg&+Llows &+cb&+Lrill&+ci&+Lantly!&n", TRUE, ch, obj, vict, TO_ROOM);
        act("$n's $q &+cg&+Llows &+cb&+Lrill&+ci&+Lantly!&n", TRUE, ch, obj, vict, TO_VICT);

        GET_HIT(ch) += 75;
        spell_invigorate(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      case 2:
        act("&+LYour $q &+cf&+Llares &+cw&+Lith &+cp&+Lower and strikes at the back $N&+L's of legs!&n", FALSE, ch, obj, vict, TO_CHAR);
        act("$N &+Lis stunned and falls to $s knees in agony!&n", FALSE, ch, obj, vict, TO_CHAR);

        act("$n's $q &+cf&+Llares &+cw&+Lith &+cp&+Lower!&n", FALSE, ch, obj, vict, TO_ROOM);
        act("$N &+Lis stunned and falls to $s knees in agony!&n", FALSE, ch, obj, vict, TO_ROOM);

        act("$n's $q &+cf&+Llares &+cw&+Lith &+cp&+Lower!&n", FALSE, ch, obj, vict, TO_VICT);
        act("You are stunned and fall to your knees in agony!&n", FALSE, ch, obj, vict, TO_VICT);

        SET_POS(vict, POS_KNEELING + GET_STAT(vict));
        CharWait(vict, PULSE_VIOLENCE);
      }
      return TRUE;
    }
  }
  return FALSE;
}

int snowogre_warhammer(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict;
  int rand, room, curr_time;
  struct affected_type af;

  if(cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "fury") )
    {
      curr_time = time(NULL);

      // 10 min timer.
      if(obj->timer[0] + 600 <= curr_time)
      {
        act("You say '&+rf&+Ru&+rr&+ry&n'", TRUE, ch, obj, vict, TO_CHAR);
        act("&+WYou whisper to $p &+Wunleashing the &+rF&+Ru&+rr&+Ry &+Wof the Snow &+bOgre &+Wkings!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$n says '&+rf&+Ru&+rr&+Ry&n'", TRUE, ch, obj, vict, TO_ROOM);
        act("$n&+W's $q &+Wunleashes the &+rF&+Ru&+rr&+Ry &+Wof the Snow &+bOgre &+WKings!&n", TRUE, ch, obj, vict, TO_ROOM);

        if(!IS_AFFECTED(ch, AFF_HASTE))
        {
          spell_haste(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        if(!affected_by_spell(ch, SPELL_ENLARGE))
        {
          spell_enlarge(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        spell_vitality(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);

        if( !has_skin_spell(ch) )
        {
          switch (number(0,2))
          {
          case 0:
            spell_shadow_shield(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          case 1:
            spell_biofeedback(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          case 2:
            spell_stone_skin(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          default:
            break;
          }
        }
        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }

  room = ch->in_room;
  vict = ch->specials.fighting;
  // 1/33 chance.
  if( cmd == CMD_MELEE_HIT && CanDoFightMove(ch, ch->specials.fighting) && !IS_IMMOBILE(ch) && !number(0, 32)
    && CheckMultiProcTiming(ch) && !IS_ELITE(vict) )
  {
    if( !IS_ALIVE(vict) || ch->in_room != vict->in_room )
    {
      return FALSE;
    }

    switch( number(0, 4) )
    {
    case 0:
      act("$p &+Wsuddenly unleashes the &+wF&+ru&+Rr&+Ly &+Wof the &+LV&+rol&+Rc&+ran&+Lo&n", TRUE, ch, obj, vict, TO_CHAR);
      act("$p &+Wsuddenly unleashes the &+wF&+ru&+Rr&+Ly &+Wof the &+LV&+rol&+Rc&+ran&+Lo&n", TRUE, ch, obj, vict, TO_NOTVICT);
      act("$p &+Wsuddenly unleashes the &+wF&+ru&+Rr&+Ly &+Wof the &+LV&+rol&+Rc&+ran&+Lo &+Wupon you!&n", TRUE, ch, obj, vict, TO_VICT);

      if( affected_by_spell(ch, SPELL_COLDSHIELD) )
      {
        affect_from_char(ch, SPELL_COLDSHIELD);
      }
      if( !affected_by_spell(ch, SPELL_FIRESHIELD) )
      {
        spell_fireshield(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      switch(number(0, 4))
      {
      case 0:
        spell_immolate(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 1:
        spell_solar_flare(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 2:
        spell_magma_burst(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 3:
        spell_fireball(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 4:
        spell_sunray(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      default:
        break;
      }

      if( affected_by_spell(vict, SPELL_COLDSHIELD) )
      {
        affect_from_char(vict, SPELL_COLDSHIELD);
      }

      if( !affected_by_spell(vict, SPELL_FIRESHIELD) )
      {
        spell_fireshield(50, vict, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      return TRUE;
      break;
    case 1:
      act("$p &+Wsuddenly unleashes the &+wF&+yu&+Yr&+wy &+Wof the &+wD&+ye&+Yse&+yr&+wt&n", TRUE, ch, obj, vict, TO_CHAR);
      act("$p &+Wsuddenly unleashes the &+wF&+yu&+Yr&+wy &+Wof the &+wD&+ye&+Yse&+yr&+wt&n", TRUE, ch, obj, vict, TO_NOTVICT);
      act("$p &+Wsuddenly unleashes the &+wF&+yu&+Yr&+wy &+Wof the &+wD&+ye&+Yse&+yr&+wt &+Wupon you!&n", TRUE, ch, obj, vict, TO_VICT);

      act("A powerful &+Ysand&+ystorm &npummels $N!&n", TRUE, ch, obj, vict, TO_CHAR);
      act("A powerful &+Ysand&+ystorm &npummels $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);
      act("A powerful &+Ysand&+ystorm &npummels you!&n", TRUE, ch, obj, vict, TO_VICT);

      if(affected_by_spell(vict, SPELL_STONE_SKIN))
      {
        act("&+yThe &+Ysand&+ystorm grinds away $N&+y's &+Lstone skin&+y!&n", TRUE, ch, obj, vict, TO_NOTVICT);
        act("&+yThe &+Ysand&+ystorm grinds away your &+Lstone skin&+y!", TRUE, ch, obj, vict, TO_VICT);
        act("&+yThe &+Ysand&+ystorm grinds away $N&+y's &+Lstone skin&+y!", TRUE, ch, obj, vict, TO_CHAR);
        affect_from_char(vict, SPELL_STONE_SKIN);
      }
      if(affected_by_spell(vict, SPELL_SHADOW_SHIELD))
      {
        act("&+yThe &+Ysand&+ystorm grinds away $N&+y's &+Lshadow shield&+y!&n", TRUE, ch, obj, vict, TO_NOTVICT);
        act("&+yThe &+Ysand&+ystorm grinds away your &+Lshadow shield&+y!", TRUE, ch, obj, vict, TO_VICT);
        act("&+yThe &+Ysand&+ystorm grinds away $N&+y's &+Lshadow shield&+y!", TRUE, ch, obj, vict, TO_CHAR);
        affect_from_char(vict, SPELL_SHADOW_SHIELD);
      }

      if(affected_by_spell(vict, SPELL_BIOFEEDBACK))
      {
        act("&+yThe &+Ysand&+ystorm grinds away $N&+y's &+Gbiofeedback&+y!&n", TRUE, ch, obj, vict, TO_NOTVICT);
        act("&+yThe &+Ysand&+ystorm grinds away your &+Gbiofeedback&+y!", TRUE, ch, obj, vict, TO_VICT);
        act("&+yThe &+Ysand&+ystorm grinds away $N&+y's &+Gbiofeedback&+y!", TRUE, ch, obj, vict, TO_CHAR);
        affect_from_char(vict, SPELL_BIOFEEDBACK);
      }

      if(affected_by_spell(vict, SPELL_GLOBE))
      {
        act("&+yThe &+Ysand&+ystorm shatters $N&+y's &+rglobe of invulnerability&+y!&n", TRUE, ch, obj, vict, TO_NOTVICT);
        act("&+yThe &+Ysand&+ystorm shatters your &+rglobe of invulnerability&+y!", TRUE, ch, obj, vict, TO_VICT);
        act("&+yThe &+Ysand&+ystorm shatters $N&+y's &+rglobe of invulnerability&+y!", TRUE, ch, obj, vict, TO_CHAR);
        affect_from_char(vict, SPELL_GLOBE);
      }
      return TRUE;
      break;
    case 2:
      act("$p &+Wsuddenly unleashes the &+wF&+cu&+Cr&+wy &+Wof the &+wgl&+ca&+Cc&+ci&+wer&n", TRUE, ch, obj, vict, TO_CHAR);
      act("$p &+Wsuddenly unleashes the &+wF&+cu&+Cr&+wy &+Wof the &+wgl&+ca&+Cc&+ci&+wer&n", TRUE, ch, obj, vict, TO_NOTVICT);
      act("$p suddenly unleashes the &+wF&+cu&+Cr&+wy &+Wof the &+wgl&+ca&+Cc&+ci&+wer &+Wupon you!&n", TRUE, ch, obj, vict, TO_VICT);

      if( affected_by_spell(ch, SPELL_FIRESHIELD) )
      {
        affect_from_char(ch, SPELL_FIRESHIELD);
      }

      if( !affected_by_spell(ch, SPELL_COLDSHIELD) )
      {
        spell_coldshield(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      switch( number(0, 3) )
      {
      case 0:
        spell_arieks_shattering_iceball(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 1:
        spell_frostbite(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 2:
        spell_cone_of_cold(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 3:
        spell_cold_snap(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      default:
        break;
      }

      if( affected_by_spell(vict, SPELL_FIRESHIELD) )
      {
        affect_from_char(vict, SPELL_FIRESHIELD);
      }

      if( !affected_by_spell(vict, SPELL_COLDSHIELD) )
      {
        spell_coldshield(50, vict, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      return TRUE;
      break;
    case 3:
      act("&+W$p &+Win your hands which suddenly unleashes the &+lF&+Bu&+Cr&+By &+Wof the &+bs&+Bq&+Cua&+Bl&+bl&n", TRUE, ch, obj, vict, TO_CHAR);
      act("$p &+Wsuddenly unleashes the &+lF&+Bu&+Cr&+By &+Wof the &+bs&+Bq&+Cua&+Bl&+bl&n", TRUE, ch, obj, vict, TO_NOTVICT);
      act("$p &+Wsuddenly unleashes the &+lF&+Bu&+Cr&+By &+Wof the &+bs&+Bq&+Cua&+Bl&+bl &+Wupon you!&n", TRUE, ch, obj, vict, TO_VICT);

      switch( number(0, 3) )
      {
      case 0:
        spell_call_lightning(50, ch, vict, 0);
        break;
      case 1:
        spell_lightning_bolt(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 2:
        spell_forked_lightning(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 3:
        spell_cyclone(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      default:
        break;
      }
      return TRUE;
      break;
    case 4:
      act("&+WYour $q &+wsuddenly &+Wglows brightly!&n", TRUE, ch, obj, vict, TO_CHAR);
      act("&+W$n's $q &+wsuddenly &+Wglows brightly!&n", TRUE, ch, obj, vict, TO_ROOM);

      rand = number(0, 9);
      if( rand > 8 )
      {
        spell_purify_spirit(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      else if( rand > 6 )
      {
        spell_greater_mending(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      else
      {
        spell_mending(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
      break;
    default:
        break;
    }
    return TRUE;
  }

  return FALSE;
}

int deathseeker_mace(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  char     Command[MAX_STRING_LENGTH];
  char     viewperson[MAX_STRING_LENGTH];
  int room;
  int curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if ( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
  	return FALSE;
  }

  if( cmd == CMD_PERIODIC && !number(0, 1) )
  {
    act("$n&+L's $q &+rvi&+Rbra&+rtes &+Lsoftly.&n", TRUE, ch, obj, vict, TO_ROOM);
    act("&+LYour $q &+rvi&+Rbra&+rtes &+Lsoftly.&n", TRUE, ch, obj, vict, TO_CHAR);

    switch(number(0,1))
    {
    case 0:
      spell_cure_critic(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
      break;
    case 1:
      spell_invigorate(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
      break;
    default:
      break;
    }
    return TRUE;
  }

  if( !IS_FIGHTING(ch) )
  {
    if( arg && (cmd == CMD_SAY) )
    {
      if( isname(arg, "death") )
      {
        curr_time = time(NULL);

        if (obj->timer[0] + 600 <= curr_time)
        {
          act("You say 'death'", TRUE, ch, obj, vict, TO_CHAR);
          act("&+LYou grip $q &+Lfirmly in your hand and call upon the &+Rp&+ro&+Rw&+re&+Rr&+rs &+Lof &+BD&+Lar&+Bk&+Lness to aid you in your &+Rb&+ra&+Rttl&+re&+Rs&+L!", TRUE, ch, obj, vict, TO_CHAR);

          act("$n says 'death'", TRUE, ch, obj, vict, TO_ROOM);
          act("$n &+Lgrips $q &+Lfirmly in $s hand and call upon the &+Rp&+ro&+Rw&+re&+Rr&+rs &+Lof &+BD&+Lar&+Bk&+Lness to aid in the &+Rb&+ra&+Rttl&+re&+Rs&+L! ", TRUE, ch, obj, vict, TO_ROOM);

          spell_vitality(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          spell_prot_from_undead(45, ch, 0, SPELL_TYPE_SPELL, ch, 0);

          switch( number(0,2) )
          {
          case 0:
            spell_shadow_shield(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          case 1:
            spell_biofeedback(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          case 2:
            spell_stone_skin(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          }

          switch( number(0,2) )
          {
          case 0:
            spell_lifelust(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          case 1:
            spell_cannibalize(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          case 2:
            spell_combat_mind(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            break;
          }

          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }

    half_chop(arg, Command, viewperson);
    if( *Command && (cmd == CMD_SAY) )
    {
      if (!strcmp(Command, "seek") && *viewperson)
      {
        target = get_char_vis(ch, viewperson);
        if( target && target != ch && IS_PC(target) && !IS_TRUSTED(target) )
        {
          curr_time = time(NULL);
          if (obj->timer[0] + 600 <= curr_time)
          {
            switch (number(0,1))
            {
            case 0:
              act("&nYou say 'I &+Rd&+re&+Rm&+ra&+Rn&+rd &nto know the whereabouts of this mortal's &+Rs&+ro&+Ru&+Rl&n..&n'", TRUE, ch, obj, vict, TO_CHAR);
              act("&+LYou grip $q &+Lfirmly in your hand and call upon the &+Rp&+ro&+Rw&+re&+Rr&+rs &+Lof &+BD&+Lar&+Bk&+Lness to aid you in your &+Rs&+re&+Ra&+rr&+Rc&+rh&+L!", TRUE, ch, obj, vict, TO_CHAR);
              act("$n says 'I &+Rd&+re&+Rm&+ra&+Rn&+rd &nto know the whereabouts of this mortal's &+Rs&+ro&+Ru&+Rl&n..&n'", TRUE, ch, obj, vict, TO_ROOM);
              act("$n &+Lgrips $q &+Lfirmly in $s hand and call upon the &+Rp&+ro&+Rw&+re&+Rr&+rs &+Lof &+BD&+Lar&+Bk&+Lness to aid in the &+Rs&+re&+Ra&+rr&+Rc&+rh&+L!", TRUE, ch, obj, vict, TO_ROOM);

              spell_clairvoyance(60, ch, 0, 0, target, 0);
            break;
            case 1:
              act("&nYou say 'I &+Rd&+re&+Rm&+ra&+Rn&+rd &nto know the whereabouts of this mortal's &+Rs&+ro&+Ru&+Rl&n..&n'", TRUE, ch, obj, vict, TO_CHAR);
              act("&+L$q &+rr&+Re&+rf&+Ru&+rs&+Re&+rs &+Lyour command and channels the &+Rp&+ro&+Rw&+re&+Rr&+rs &+Lof &+BD&+Lar&+Bk&+Lness upon you!", TRUE, ch, obj, vict, TO_CHAR);
              act("$n says 'I &+Rd&+re&+Rm&+ra&+Rn&+rd &nto know the whereabouts of this mortal's &+Rs&+ro&+Ru&+Rl&n..&n'", TRUE, ch, obj, vict, TO_ROOM);
              act("&n$q &+rr&+Re&+rf&+Ru&+rs&+Re&+rs &+L$n&+L's command and channels the &+Rp&+ro&+Rw&+re&+Rr&+rs &+Lof &+BD&+Lar&+Bk&+Lness upon them!", TRUE, ch, obj, vict, TO_ROOM);

              damage(ch, ch, 250, SPELL_PSYCHIC_CRUSH);
            }
            obj->timer[0] = curr_time;
            return TRUE;
          }
        }
      }
    }
  }

  room = ch->in_room;
  vict = ParseTarget(ch, arg);

  // 1/33 chance.
  if( cmd == CMD_MELEE_HIT && IS_ALIVE(vict) && !number(0, 32) && CheckMultiProcTiming(ch) )
  {
    switch( number(0,2) )
    {
    case 0:
      act("&+LYour $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+rSc&+RRe&+raM&+L!&n", TRUE, ch, obj, vict, TO_CHAR);
      act("&+W$n's $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+rSc&+RRe&+raM&+L!&n", TRUE, ch, obj, vict, TO_VICT);
      act("&+W$n's $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+rSc&+RRe&+raM&+L!$n", TRUE, ch, obj, vict, TO_NOTVICT);

      spell_full_harm(55, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
      {
        return TRUE;
      }
      switch( number(0,3) )
      {
      case 0:
        spell_wither(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 1:
        spell_curse(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 2:
        spell_fear(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 3:
        spell_dispel_magic(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      }
      break;
    case 1:
      act("&+LYour $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+RSc&+rRe&+RaM&+L!&n", TRUE, ch, obj, vict, TO_CHAR);
      act("&+W$n's $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+RSc&+rRe&+RaM&+L!&n", TRUE, ch, obj, vict, TO_VICT);
      act("&+W$n's $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+RSc&+rRe&+RaM&+L!&n", TRUE, ch, obj, vict, TO_NOTVICT);

      spell_greater_soul_disturbance(55, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
      {
        return TRUE;
      }
      switch (number(0,3))
      {
      case 0:
        spell_wither(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 1:
        spell_curse(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 2:
        spell_fear(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 3:
        spell_dispel_magic(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      }
      break;
    case 2:
      act("&+LYour $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+mSc&+RRe&+ma&+rM&+L!&n", TRUE, ch, obj, vict, TO_CHAR);
      act("&+W$n's $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+mSc&+RRe&+ma&+rM&+L!&n", TRUE, ch, obj, vict, TO_VICT);
      act("&+W$n's $q &+Llets loose a &+rPo&+RWe&+rRF&+ruL &+mSc&+RRe&+ma&+rM&+L!$n", TRUE, ch, obj, vict, TO_NOTVICT);

      spell_energy_drain(55, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
      {
        return TRUE;
      }
      switch (number(0,3))
      {
      case 0:
        spell_wither(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 1:
        spell_curse(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 2:
        spell_fear(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 3:
        spell_dispel_magic(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      }
    }
    return TRUE;
  }
  return FALSE;
}

int illithid_axe(P_obj obj, P_char ch, int cmd, char *arg)
{
  char     Command[MAX_STRING_LENGTH];
  char     Toperson[MAX_STRING_LENGTH];
  P_char   next, target, vict;
  int      dam;
  int      curr_time;
  int      room;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || IS_NPC(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  // 1/2 chance.
  if( cmd == CMD_PERIODIC && !number(0, 1) )
  {
    act("$n&+L's $q &+rvi&+Rbra&+rtes &+Lsoftly.&n", TRUE, ch, obj, vict, TO_ROOM);
    act("&+LYour $q &+rvi&+Rbra&+rtes &+Lsoftly.&n", TRUE, ch, obj, vict, TO_CHAR);

    switch(number(0,1))
    {
      case 0:
        spell_cure_critic(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      case 1:
        spell_invigorate(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      default:
        break;
    }
    return TRUE;
  }

  half_chop(arg, Command, Toperson);

  if( !IS_FIGHTING(ch) )
  {
    if( *Command && (cmd == CMD_SAY) )
    {
      curr_time = time(NULL);
      if( !strcmp(Command, "warp") && *Toperson )
      {
        target = get_char_vis(ch, Toperson);

        if( !target || !can_relocate_to(ch, target) || target == ch )
        {
          act("$q &+Lhas failed to take you to your destination and gives you a powerful jolt.&n", TRUE, ch, obj, ch, TO_CHAR);

          obj->timer[0] = curr_time;
          spell_inflict_pain(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          spell_inflict_pain(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          //spell_damage(ch, ch, number(100, 400), SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
          return FALSE;
        }
        // 10 min timer.
        if (obj->timer[0] + 600 <= curr_time)
        {
          act("&+L$p &+Lshoots out a &+mB&+ME&+mA&+MM &+Lof pure &+WL&+YI&+WG&+YH&+WT&+L that completely engulfs $n!&n\r\n", TRUE, ch, obj, target, TO_ROOM);
          act("&+LYour $q &+Lshoots out a &+mB&+ME&+mA&+MM &+Lof pure &+WL&+YI&+WG&+YH&+WT&+L that completely engulfs you!&n\r\n", TRUE, ch, obj, target, TO_CHAR);

          spell_ether_warp(60, ch, 0, 0, target, 0);

          act("&+LSlowl&+wy you&+Wr par&+wticl&+Les be&+wgin t&+Wo rem&+water&+Lialize.&n", TRUE, ch, obj, target, TO_CHAR);
          act("&+LA thin &+Ww&+wh&+Wi&+wt&+We &+Lline suddenly cuts through the air before you, revealing a hole of pure &+WL&+YI&+WG&+YH&+WT&+L.\r\n"
            "&+LYou feel &+Ws&+Yt&+Wa&+Yt&+Wi&+Yc &+Lbegin to build throughout the room as an &+Yel&+Wec&+Ytr&+Wic&+Yal &+Lcharge seems to descend upon you!&n", TRUE, ch, obj, target, TO_ROOM);

          // Ether warp has lag.
          // CharWait(ch,PULSE_VIOLENCE * number(4,8));
          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }
  }

  room = ch->in_room;
  vict = ParseTarget(ch, arg);

  // Limiting the weapon to one proc a round since the proc is very potent.
  // 1/35 chance.
  if( cmd == CMD_MELEE_HIT && IS_ALIVE(vict) && !number(0, 34) && CheckMultiProcTiming(ch) )
  {
    if( room == vict->in_room )
    {
      switch(number(0,1))
      {
        case 0:
          act("&+LYour $q &+Rvi&+rci&+Rou&+rsl&+Ry &+Lgrabs $N &+Lwith a &+Mm&+me&+Mn&+mt&+Ma&+ml &+Lattack hurling them against the wall!&n\r\n"
            "&n$N &nfalls to $s knees!&n", TRUE, obj->loc.wearing, obj, vict, TO_CHAR);
          act("$n's $q &+Rvi&+rci&+Rou&+rsl&+Ry &+Lgrabs you with a &+Mm&+me&+Mn&+mt&+Ma&+ml &+Lattack sending you flying against the wall!&n\r\n"
            "&nYou fall to your knees!&n", TRUE, obj->loc.wearing, obj, vict, TO_VICT);
          act("$n's $q &+Rvi&+rci&+Rou&+rsl&+Ry &+Lgrabs $N &+Lwith a &+Mm&+me&+Mn&+mt&+Ma&+ml &+Lattack hurling them against the wall!&n\r\n"
            "&n$N &nfalls to $s knees!&n", TRUE, obj->loc.wearing, obj, vict, TO_NOTVICT);

          SET_POS(vict, POS_SITTING + GET_STAT(vict));
          stop_fighting(vict);
          if( IS_DESTROYING(vict) )
          {
            stop_destroying(vict);
          }
          CharWait(vict, PULSE_VIOLENCE);
          break;
        case 1:

          act("&+LYour $q &+rt&+Rea&+rr&+Rs &+Linto $N&+L's &+mth&+Mo&+Cug&+Mh&+mts &+Land unleashes a &+Rvi&+rci&+Rou&+rs &+Lmental &+ra&+Rtt&+ra&+Rck&+L!&n", TRUE, obj->loc.wearing, obj, vict, TO_CHAR);
          act("$n's $q &+rt&+Rea&+rr&+Rs &+Linto your &+mth&+Mo&+Cug&+Mh&+mts &+Land unleashes a &+Rvi&+rci&+Rou&+rs &+Lmental &+ra&+Rtt&+ra&+Rck&+L!&n", TRUE, obj->loc.wearing, obj, vict, TO_VICT);
          act("$n's $q &+rt&+Rea&+rr&+Rs &+Linto $N&+L's &+mth&+Mo&+Cug&+Mh&+mts &+Land unleashes a &+Rvi&+rci&+Rou&+rs &+Lmental &+ra&+Rtt&+ra&+Rck&+L!&n", TRUE, obj->loc.wearing, obj, vict, TO_NOTVICT);

          spell_psychic_crush(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
          break;
        default:
          break;
      }
    }
    return TRUE;
  }
  return FALSE;
}

int dagger_ra(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict;
  struct affected_type af;
  int curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN(obj) || (ch && ch != obj->loc.wearing) )
  {
    return FALSE;
  }


  if( cmd == CMD_PERIODIC )
  {
    curr_time = time(NULL);
    ch = obj->loc.wearing;

    if( !CHAR_IN_NO_MAGIC_ROOM(ch) )
    {
      // 1 min timer.
      if(obj->timer[0] + 60 <= curr_time)
      {
        obj->timer[0] = curr_time;

        if( GET_HIT(ch) < GET_MAX_HIT(ch) )
        {
          act("$n&+L's $q &+rvi&+Rbra&+rtes &+Lsoftly.&n", TRUE, ch, obj, NULL, TO_ROOM);
          act("&+LYour $q &+rvi&+Rbra&+rtes &+Lsoftly.&n", TRUE, ch, obj, NULL, TO_CHAR);
          spell_cure_critic(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          spell_invigorate(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
        }
      }
    }
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "ra") )
    {
      curr_time = time(NULL);
      // 750 sec timer == 12 min 30 sec.
      if( obj->timer[1] + 750 <= curr_time )
      {
        act("You say '&+YRa&+W'&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+LYou &+Yth&+Wr&+Yu&+Wst $q &+Ltowards the sky calling upon the &+Yf&+Wa&+Yb&+Wl&+Ye&+Wd &+Yp&+Wo&+Yw&+We&+Yr&+Ws &+Lof the &+WS&+Yu&+Wn &+YG&+Wo&+Yd&+L!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$n says '&+YRa&+W'&n", TRUE, ch, obj, vict, TO_ROOM);
        act("$n &+Yth&+Wr&+Yu&+Wst&+Ys $q &+Ltowards the sky and calls upon the &+Yf&+Wa&+Yb&+Wl&+Ye&+Wd &+Yp&+Wo&+Yw&+We&+Yr&+Ws &+Lof the &+WS&+Yu&+Wn &+YG&+Wo&+Yd&+L!&n", TRUE, ch, obj, vict, TO_ROOM);

        if( affected_by_spell(ch, SPELL_COLDSHIELD) )
        {
          act("&+LThe &+rflames &+Lmelt away $n&+L's &+Bicy &+Lshield&+L.&n", TRUE, ch, obj, vict, TO_ROOM);
          act("&+LThe &+rflames &+Lmelt away your &+Bicy &+Lshield&+L.&n", TRUE, ch, obj, vict, TO_CHAR);

          affect_from_char(ch, SPELL_COLDSHIELD);
          spell_fireshield(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        else
        {
          spell_fireshield(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }

        spell_globe(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_deflect(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[1] = curr_time;
      }
      return TRUE;
    }
    return FALSE;
  }

  // 1/50 chance.
  if( cmd == CMD_MELEE_HIT && !number(0, 49) && CheckMultiProcTiming(ch))
  {
    vict = (P_char) arg;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }

    act("&+LYour $q &+Lchannels the &+Wb&+Yro&+Wke&+Yn &+Wr&+Ya&+Wy&+Ys &+Lof &+Wm&+Yor&+Wni&+Yng &+rS&+Ru&+rnr&+Ri&+rs&+Re &+Lat&n $N&+L.&n", TRUE, ch, obj, vict, TO_CHAR);
    act("$n's $q &+Lchannels the &+Wb&+Yro&+Wke&+Yn &+Wr&+Ya&+Wy&+Ys &+Lof &+Wm&+Yor&+Wni&+Yng &+rS&+Ru&+rnr&+Ri&+rs&+Re &+Lat&n you&+L.&n", TRUE, ch, obj, vict, TO_VICT);
    act("$n's $q &+Lchannels the &+Wb&+Yro&+Wke&+Yn &+Wr&+Ya&+Wy&+Ys &+Lof &+Wm&+Yor&+Wni&+Yng &+rS&+Ru&+rnr&+Ri&+rs&+Re &+Lat&n $N&+L.&n", TRUE, ch, obj, vict, TO_NOTVICT);

    switch (number(0, 10))
    {
      case 0:
        spell_solar_flare(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 1:
        spell_immolate(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 2:
        spell_magma_burst(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 3:
        spell_sunray(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 4:
        spell_fireball(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 5:
        spell_molten_spray(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
        spell_flamestrike(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      default:
        break;
    }
    return TRUE;
  }
  return FALSE;
}

// Winterhaven high priest is prevented from summoning beasts in mobact.c

int newbie_spellup_mob(P_char ch, P_char victim, int cmd, char *arg)
{
  int *spells = NULL;

// Some spells do not stack such as pantherspeed and wolfspeed, accelerated healing and regeneration.
// So try to avoid these hangups otherwise the spellup mob will simply spam the spell over and over.
  int      ClerBeneSpells[] = {SPELL_ARMOR, SPELL_BLESS, SPELL_DETECT_MAGIC,
    SPELL_PROTECT_FROM_COLD, SPELL_PROTECT_FROM_FIRE,
    SPELL_SLOW_POISON, SPELL_PROTECT_FROM_GAS, SPELL_PROTECT_FROM_EVIL,
    SPELL_PROTECT_FROM_GOOD, SPELL_PROTECT_FROM_ACID,
    SPELL_PROTECT_FROM_LIGHTNING, 0};

  int      ShamBeneSpells[] = {SPELL_SPIRIT_ARMOR, SPELL_PANTHERSPEED, SPELL_HAWKVISION, SPELL_FIRE_WARD,
    SPELL_COLD_WARD, SPELL_GREATER_RAVENFLIGHT,0};

  int      DruidBeneSpells[] = {SPELL_BARKSKIN, SPELL_FORTITUDE, SPELL_AID,
    SPELL_PROTECT_FROM_ANIMAL, SPELL_REGENERATION, 0};

  int      SorcBeneSpells[] = {SPELL_DETECT_MAGIC, SPELL_STRENGTH, SPELL_AGILITY, SPELL_LEVITATE, 0};

  if(cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) || IS_IMMOBILE(ch) || IS_CASTING(ch) || IS_FIGHTING(ch) )
  {
    return FALSE;
  }

  if( cmd != CMD_PERIODIC)
  {
    return FALSE;
  }

  // everything after here is in the periodic event

  /* make sure I'm even able to cast in this room! */
  if( IS_SET(world[ch->in_room].room_flags,  SAFE_ZONE | NO_MAGIC | ROOM_SILENT)
    || affected_by_spell(ch, SPELL_FEEBLEMIND) || IS_AFFECTED2(ch, AFF2_SILENCED) )
  {
    return FALSE;
  }

  // find what class the mob is. a bit of randomness
  // so that multiclass mobs will cast from all of their spells
  if(number(0, 3))
  {
    if( GET_CLASS(ch, CLASS_CLERIC) && !number(0, 1) )
    {
      spells = ClerBeneSpells;
    }
    else if( GET_CLASS(ch, CLASS_SHAMAN) && !number(0, 1) )
    {
      spells = ShamBeneSpells;
    }
    else if( GET_CLASS(ch, CLASS_DRUID) && !number(0, 1) )
    {
      spells = DruidBeneSpells;
    }
    else if( GET_CLASS(ch, CLASS_SORCERER))
    {
      spells = SorcBeneSpells;
    }
  }

  // go through room, find someone who needs a spell
  for(P_char tch = world[ch->in_room].people; tch; tch = tch->next )
  {
    // return if its an npc, too high level, fighting, with a bit of randomness thrown in

    if(!IS_PC(tch) ||
        GET_LEVEL(tch) > 35 ||
        !number(0, 2) ||
        ch->in_room != tch->in_room)
          continue;
          
    if(IS_FIGHTING(tch))
      continue;
    
    if(GET_HIT(tch) < (int)(GET_MAX_HIT(tch) * 0.75))
    {
      if(npc_has_spell_slot(ch, SPELL_FULL_HEAL) && number(0, 1))
      {
        return MobCastSpell(ch, tch, 0, SPELL_FULL_HEAL, GET_LEVEL(ch));
      }
      else if(npc_has_spell_slot(ch, SPELL_GREATER_MENDING) && number(0, 1))
      {
        return MobCastSpell(ch, tch, 0, SPELL_GREATER_MENDING, GET_LEVEL(ch));
      }
      else if(npc_has_spell_slot(ch, SPELL_NATURES_TOUCH) && number(0, 1))
      {
        return MobCastSpell(ch, tch, 0, SPELL_NATURES_TOUCH, GET_LEVEL(ch));
      }
      else
        continue;
    }

    // else step through the spell list, find one to cast
    if(spells)
      for( int i = 0; spells[i]; i++ )
      {
        if(!affected_by_spell(tch, spells[i]) && 
            npc_has_spell_slot(ch, spells[i]) )
        {
          debug("(%s): casting on (%s).", J_NAME(ch), J_NAME(tch));
          return MobCastSpell(ch, tch, 0, spells[i], GET_LEVEL(ch));
        }
      }
  }
  return FALSE; 
}

int welfare_well(int room, P_char ch, int cmd, char *arg)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  P_obj obj, well;
  P_char victim;
  int bits;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return FALSE;

  if (cmd == CMD_PUT)
  {
    half_chop(arg, buf, buf2);
    if ( (!strcmp(buf2, "well")) && !IS_TRUSTED(ch) )
    {
      send_to_char("Please use the donate command.\n", ch);
      return TRUE;
    }
    return FALSE;
  }

  if ((cmd == CMD_GET) || (cmd == CMD_TAKE))
  {
    if ( (!strcmp(arg, " all well")) && !IS_TRUSTED(ch) )
    {
      send_to_char("Aren't we greedy today?  I think not.\n", ch);
      return TRUE;
    }
  
    one_argument(arg, buf);
    if (!buf)
      return FALSE;
    // We're looking for a well, so skip tracks.
    bits = generic_find("well", FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &well);
    if (bits && (well->type == ITEM_STORAGE))
    {
      obj = get_obj_in_list(buf, well->contains);
      if (obj)
        logit(LOG_DONATION, "&n%s: (%d) %s.", GET_NAME(ch), obj_index[obj->R_num].virtual_number, obj->short_description);
    }
  }
  return FALSE;
}

int wh_janitor(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    o, next_obj, o_1, well;
  P_nevent  ev = NULL;
  hunt_data data;
  bool     found_well, dumped;
  bool     loaded = FALSE;


  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

/* Is there anything in the room that we can pick up? Do it! */
  for (o = world[ch->in_room].contents; o; o = o->next_content)
  {
    if(o->type == (ITEM_SWITCH || ITEM_KEY || ITEM_TRASH))
      continue;

  	if (!CAN_GET_OBJ(ch, o))
  	{
  		continue;
  	}

    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);

    obj_from_room(o);
    obj_to_char(o, ch);

    return TRUE;
  }

/* Are we in the well room? Drop the loot into it! */
  int well_room = real_room(WELL_ROOM);

  if (well_room == NOWHERE)
    return FALSE;


  if (ch->in_room == well_room)
  {

    found_well = FALSE;
    for (o_1 = world[ch->in_room].contents; o_1; o_1 = o_1->next_content)
    {
      if (o_1->R_num == real_object(WELL))
      {
        found_well = TRUE;
        well = o_1;
        continue;
      }
    }

    if (!found_well)
    {
      debug("wh_janitor(): can't find donation well [%d] in its room [%d] - loading a new one.", WELL, WELL_ROOM);

      well = read_object(WELL, VIRTUAL);
      obj_to_room(well, real_room(WELL_ROOM));
    }

    if (!well)
    {
	    return FALSE;
    }

    unequip_all(ch);

    dumped = FALSE;

    if (ch->carrying)
    {
      do_donate(ch, "all", CMD_DONATE);
      dumped = TRUE;
    }

    for( o = ch->carrying; o; o = next_obj )
    {
      next_obj = o->next_content;
      if( !IS_ARTIFACT(o) )
        extract_obj(o, TRUE);
      //obj_from_char(o, FALSE);
      //obj_to_obj(o, well);
      //dumped = TRUE;
    }

    if (dumped)
    {
      act("$n empties his loot into the $o.", FALSE, ch, well, 0, TO_ROOM);
      return TRUE;
    }
  }

  loaded = ((weight_notches_above_naked(ch) > 3) ||
   (IS_CARRYING_N(ch) > (int) (0.25*CAN_CARRY_N(ch))) || !number(0,299));

/* Are we loaded past 6% of our capacity? (or sometimes even without it)*/
  switch (loaded)
  {
  case FALSE:

/*  No we're not. Let's look if there's anything in adjacent rooms and move there. */
  if (1/*!number(0, 5)*/)
  {
    int move_to_loot = 0;
    int a;

    for (a = 0; a < NUM_EXITS; a++)
      if (!number(0, 3) && EXIT(ch, a) && CAN_GO(ch, a))
      {
        for (o = world[EXIT(ch, a)->to_room].contents; o; o = o->next_content)
        {
          if (CAN_WEAR(o, ITEM_TAKE) && CAN_CARRY_OBJ(ch, o))
          {
            act("$n notices some garbage nearby.", FALSE, ch, 0, 0, TO_ROOM);
            move_to_loot = exitnumb_to_cmd(a);
            do_move(ch, NULL, move_to_loot);
            return TRUE;
          }
        }
      }
  }
  break;

  case TRUE:

/* Yes we are. Let's get closer to the well */

  if(!number(0, 9) && loaded)
  {

    if (ch->in_room == well_room)
      return FALSE;

    LOOP_EVENTS_CH(ev, ch->nevents)
    {
      if (ev->func == mob_hunt_event)
      {
        return FALSE;
      }
    }
    data.hunt_type = HUNT_ROOM;
    data.targ.room = well_room;
    data.huntFlags = BFS_STAY_ZONE;
    add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
  }
  break;
  }

  return FALSE;
}

int wh_guard(P_char ch, P_char victim, int cmd, char *arg)
{
	int      helpers[] = { 55240, 55241, 55255, 55256, 55257, 55258, 55259, 55260, 55022, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!victim && IS_FIGHTING(ch) && EVIL_RACE(ch->specials.fighting))
    return shout_and_hunt(ch, 20, "&+YCome to my aid! We are being invaded!&n", NULL, helpers, 0, 0);
  return FALSE;
}

int no_kill_priest_obj(P_obj fountain, P_char ch, int cmd, char *arg)
{
   if(cmd == CMD_SET_PERIODIC)
     return true;

   if(arg && (IS_AGG_CMD(cmd)))
   {
     if(isname(arg, "priest") || isname(arg, "winterhaven") || isname(arg, "high"))
     {
       send_to_char("Your conscience stays your wicked thoughts.\n", ch);
       return TRUE;
     }
   }
   return FALSE; 
}


int demon_slayer(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room, rand;
  int curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if ( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
	  return FALSE;
  }

  if( IS_UNDEAD(ch) )
  {
    if( cmd == CMD_PERIODIC && !number(0, 2) )
    {
      act("An aura of &+Ldeath &nseems to drain $n's strength.", TRUE, ch, obj, vict, TO_ROOM);
      act("An aura of &+Ldeath &ndrains you of your strength.", TRUE, ch, obj, vict, TO_CHAR);

      GET_HIT(ch) -= 75;
      update_pos(ch);
      return TRUE;
    }
  }

  // This should work good to prevent cheesing the procs.
  // Not going to reset for situations like when it's been disarmed
  // and they rewield, and I would say that's a nice feature, not
  // a bug, so just keep that in mind if you are looking to edit this.
  if( obj->timer[1] == 0 )
  {
    obj->timer[1] = time(NULL);
  }
  if( arg && (cmd == CMD_REMOVE) )
  {
    int j;
    if (obj == get_object_in_equip(ch, arg, &j) || isname(arg, "all"))
    {
      obj->timer[1] = 0;
    }
  }

  if( IS_FIGHTING(ch) )
  {
    if (arg && (cmd == CMD_SAY))
    {
      if (isname(arg, "bel"))
      {
        curr_time = time(NULL);
        vict = ParseTarget(ch, arg);

        // 800 sec timer == 13 min 20 sec, and 10 min timer.
        if( obj->timer[0] + 800 <= curr_time && obj->timer[1] + 600 <= curr_time )
        {
          act("&+WYou scream '&+rBel&+L! I call upon you to &+rS&+Rl&+rA&+Ry my foe!&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+WYou thrust $q &+Winto the ground!&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+LDarkness quickly swallows up all available light and the room suddenly turns to silence.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+LThe ghostly image of &+rBel &+Lsuddenly appears and reaches it's hand into the ground.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+YBoom! &+LA giant pillar of &+Cenergy &+Lerupts from beneath your feet, engulfing $N &+Lin a &+rc&+Ro&+Yl&+Bo&+Cr&+Yf&+Ru&+rl &+Lhaze!&n", TRUE, ch, obj, vict, TO_CHAR);

          act("$n screams '&+rBel&+L! I call upon you to &+rS&+Rl&+rA&+Ry my foe!'&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("$n &+Lthrusts $q &+Winto the ground!&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("&+LDarkness quickly swallows up all available light and the room suddenly turns to silence.&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("&+LThe ghostly image of &+rBel &+Lsuddenly appears and reaches it's hand into the ground.&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("&+YBoom! &+LA giant pillar of &+Cenergy &+Lerupts from beneath your feet, engulfing $N &+Lin a &+rc&+Ro&+Yl&+Bo&+Cr&+Yf&+Ru&+rl &+Lhaze!&n", TRUE, ch, obj, vict, TO_NOTVICT);

          act("$n screams '&+rBel&+L! I call upon you to &+rS&+Rl&+rA&+Ry my foe!'&n", TRUE, ch, obj, vict, TO_VICT);
          act("$n &+Lthrusts $q &+Winto the ground!&n", TRUE, ch, obj, vict, TO_VICT);
          act("&+LDarkness quickly swallows up all available light and the room suddenly turns to silence.&n", TRUE, ch, obj, vict, TO_VICT);
          act("&+LThe ghostly image of &+rBel &+Lsuddenly appears and reaches it's hand into the ground.&n", TRUE, ch, obj, vict, TO_VICT);
          act("&+YBoom! &+LA giant pillar of &+Cenergy &+Lerupts from the ground, engulfing &+wyou &+Lin a &+rc&+Ro&+Yl&+Bo&+Cr&+Yf&+Ru&+rl &+Lhaze!&n", TRUE, ch, obj, vict, TO_VICT);

          switch( number(1, 6) )
          {
          case 1:
            spell_pword_blind(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            break;
          case 2:
            spell_pword_stun(56, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            break;
          case 3:
            spell_pword_stun(56, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            spell_pword_blind(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            break;
          case 4:
            spell_bigbys_clenched_fist(45, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            break;
          case 5:
            spell_firebrand(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            break;
          case 6:
            spell_chaotic_ripple(56, ch, 0, SPELL_TYPE_SPELL, vict, 0);
            break;
          }
          act("&+LThe ghostly image of &+RBel &+Lfades out of existence and the world slowly returns to normal.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+LThe ghostly image of &+RBel &+Lfades out of existence and the world slowly returns to normal.&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("&+LThe ghostly image of &+RBel &+Lfades out of existence and the world slowly returns to normal.&n", TRUE, ch, obj, vict, TO_VICT);

          CharWait(ch,PULSE_VIOLENCE * 2);
          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "euronymous") )
    {
      curr_time = time(NULL);
      vict = ParseTarget(ch, arg);

      // 450 sec == 7 min 30 sec & 10 min timer.
      if( obj->timer[0] + 450 <= curr_time && obj->timer[1] + 600 <= curr_time )
      {
        act("&+WYou say '&+rEuronymous&+L! Bring me &+mP&+Mo&+mW&+Me&+mr &+Lor bring me &+rD&+Re&+rA&+Rt&+rH&+L!'&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+WYou thrust $q &+Winto the ground!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+LA dark &+bportal &+Lappears before you, and &+rEuronymous, the Prince of Demons &+Lsteps out.&n", TRUE, ch, obj, vict, TO_CHAR);

        act("$n says '&+rEuronymous&+L! Bring me &+mP&+Mo&+mW&+Me&+mr &+Lor bring me &+rD&+Re&+rA&+Rt&+rH&+L!&n'&n", TRUE, ch, obj, vict, TO_ROOM);
        act("$n &+Lthrusts $q &+Winto the ground!&n", TRUE, ch, obj, vict, TO_ROOM);
        act("&+LA dark &+bportal &+Lappears before you, and &+rEuronymous, the Prince of Demons &+Lsteps out.&n", TRUE, ch, obj, vict, TO_ROOM);

        rand = number(0, 96);
        if( rand <= 31 )
        {
            act("&+rThe Prince of Demons &+Llooks at you and says, &+W'&+CProt&+Bec&+Ction&+W'&n", TRUE, ch, obj, vict, TO_CHAR);
            act("&+rThe Prince of Demons &+Llooks at $n and says, &+W'&+CProt&+Bec&+Ction&+W'&n", TRUE, ch, obj, vict, TO_ROOM);

            //spell_displacement(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            spell_stone_skin(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            spell_globe(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        else if(rand <= 63)
        {
            act("&+rThe Prince of Demons &+Llooks at you and says, &+W'&+mSp&+Me&+med&+W'&n", TRUE, ch, obj, vict, TO_CHAR);
            act("&+rThe Prince of Demons &+Llooks at $n and says, &+W'&+mSp&+Me&+med&+W'&n", TRUE, ch, obj, vict, TO_ROOM);

            spell_enhanced_agility(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            spell_enhanced_dexterity(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            spell_blur(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
           // spell_reduce(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        else if(rand <= 94)
        {
            act("&+rThe Prince of Demons &+Llooks at you and says, &+W'&+bPo&+Bw&+ber&+W'&n", TRUE, ch, obj, vict, TO_CHAR);
            act("&+rThe Prince of Demons &+Llooks at $n and says, &+W'&+bPo&+Bw&+ber&+W'&n", TRUE, ch, obj, vict, TO_ROOM);

            spell_enhanced_strength(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            spell_strength(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            spell_lionrage(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          //  spell_enlarge(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        else if( rand > 97 ) //ho ho, hellfire? I think not -Z
        {
            act("&+rThe Prince of Demons &+grins at you and says, &+W'&+rF&+Ru&+rr&+Ry&+W'&n", TRUE, ch, obj, vict, TO_CHAR);
            act("&+rThe Prince of Demons &+grins at $n and says, &+W'&+rF&+Ru&+rr&+Ry&+W'&n", TRUE, ch, obj, vict, TO_ROOM);

            spell_hellfire(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);

            act("&+ROuch&+L! The demonic &+rflames &+RSEAR &+Lyour &+rfl&+Re&+rsh&+L!&n", TRUE, ch, obj, vict, TO_CHAR);

            GET_HIT(ch) = 50;
        }
        // No vamp trance for you!
        else if( FALSE )
        {
            act("&+rThe Prince of Demons &+Llooks at you and laughs, &+W'&+rD&+Re&+ra&+Rt&+rh&+W? &+mU&+Mn&+md&+Me&+ma&+Mt&+mh&+L!'&n", TRUE, ch, obj, vict, TO_CHAR);
            act("&+rEuronymous &+Lleaps forward and bites you in the neck, sinking his fangs deep into your flesh!", TRUE, ch, obj, vict, TO_CHAR);
            act("&+LAs the &+Rblood &+Lflows out of your body, you begin to see the world in a different light.", TRUE, ch, obj, vict, TO_CHAR);

            act("&+rThe Prince of Demons &+Llooks at $n and laughs, &+W'&+rD&+Re&+ra&+Rt&+rh&+W? &+mU&+Mn&+md&+Me&+ma&+Mt&+mh&+L!'&n", TRUE, ch, obj, vict, TO_ROOM);
            act("&+rEuronymous suddenly leaps forward and sinks his fangs into the neck of $n, sending blood flying.&n", TRUE, ch, obj, vict, TO_ROOM);
            act("$n &+Lbegins to look rather &+wpale &+Las the last drops of &+Rblood &+Ldrip to the floor.&n", TRUE, ch, obj, vict, TO_ROOM);

            spell_vampire(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
            act("&nAs life departs from your body, you feel strained.&n", TRUE, ch, obj, vict, TO_CHAR);
            GET_HIT(ch) = 50;
        }

        act("&+rEuronymous &+Lgrins at you, steps into the &+bportal&+L, and vanishes.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+rEuronymous &+Lgrins at $n, steps into the &+bportal&+L, and vanishes.&n", TRUE, ch, obj, vict, TO_ROOM);

        CharWait(ch,PULSE_VIOLENCE * 2);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
    else if( isname(arg, "jubilex") )
    {
      curr_time = time(NULL);
      vict = ParseTarget(ch, arg);
      // 450 sec == 7 min 30 sec & 10 min timers.
      if (obj->timer[0] + 450 <= curr_time && obj->timer[1] + 600 <= curr_time)
      {
        act("&+WYou say '&+rJubilex&+L! Grant me strength!&+W'&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+WYou thrust $q &+Winto the ground!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+LA thin line of &+ggreen &+Glight &+Lcuts into the &+Cair&+L, revealing a &+gportal &+Lto another &+Gdimension&+L.", TRUE, ch, obj, vict, TO_CHAR);
        act("&+LSuddenly, &+rJubilex &+Lleaps into this reality and conjures up a &+rgreen &+Gmist&+L.&n", TRUE, ch, obj, vict, TO_CHAR);

        act("$n says '&+rJubilex&+L! Grant me strength!&+W'&n", TRUE, ch, obj, vict, TO_ROOM);
        act("$n &+Lthrusts $q &+Winto the ground!&n", TRUE, ch, obj, vict, TO_ROOM);
        act("&+LA thin line of &+ggreen &+Glight &+Lcuts into the &+Cair&+L, revealing a &+gportal &+Lto another &+Gdimension&+L.&n", TRUE, ch, obj, vict, TO_ROOM);
        act("&+LSuddenly, &+rJubilex &+Lleaps into this reality and conjures up a &+rgreen &+Gmist&+L.&n", TRUE, ch, obj, vict, TO_ROOM);

        rand = number(0, 100);
        if(rand >= 76)
        {
          act("You are bathed in an extremely powerful healing aura.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("$n is bathed in an extremely powerful healing aura.&n", TRUE, ch, obj, vict, TO_ROOM);
          GET_HIT(ch) += 400;
          spell_invigorate(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        else if(rand >= 51)
        {
          act("You are bathed in a strong healing aura.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("$n is bathed in a strong healing aura.&n", TRUE, ch, obj, vict, TO_ROOM);
          GET_HIT(ch) += 300;
          spell_invigorate(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        else if(rand >= 26)
        {
          act("You are bathed in a healing aura.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("$n is bathed in a healing aura.&n", TRUE, ch, obj, vict, TO_ROOM);
          GET_HIT(ch) += 200;
          spell_invigorate(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        else
        {
          act("You are bathed in a light healing aura.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("$n is bathed in a light healing aura.&n", TRUE, ch, obj, vict, TO_ROOM);
          GET_HIT(ch) += 100;
          spell_invigorate(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }

        act("&+LAs the &+ggreen &+Gmist &+Lsubsides, &+rJubilex &+Lflashes you a wicked grin, steps into the portal, and vanishes.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+LAs the &+ggreen &+Gmist &+Lsubsides, &+rJubilex &+Lflashes $n a wicked grin, steps into the portal, and vanishes.&n", TRUE, ch, obj, vict, TO_ROOM);

        CharWait(ch,PULSE_VIOLENCE * 2);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }

  room = ch->in_room;
  vict = ParseTarget(ch, arg);
  // 1/33 chance.
  if( cmd == CMD_MELEE_HIT && IS_ALIVE(vict) && !number(0, 32) && CheckMultiProcTiming(ch) )
  {
    if (GET_RACE(vict) == RACE_DEMON)
    {
      switch( number(0,2) )
      {
      case 0:
        act("&+LYour $q &+rfl&+Rar&+res &+Las it channels the powers of &+rBel &+Lupon your foe!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+L$n's $q &+rfl&+Rar&+res &+Las it channels the powers of &+rBel &+Lupon you!&n", TRUE, ch, obj, vict, TO_VICT);
        act("&+L$n's $q &+rfl&+Rar&+res &+Las it channels the powers of &+rBel &+Lupon $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);

        spell_sunray(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      /*  spell_sunray(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        spell_sunray(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        spell_sunray(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);*/
        break;
      case 1:
        act("&+LYour $q &+bbl&+Cu&+brs &+Las it channels the powers of &+rEuronymous &+Lupon your foe!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+L$n's $q &+bbl&+Cu&+brs &+Las it channels the powers of &+rEuronymous &+Lupon you!&n", TRUE, ch, obj, vict, TO_VICT);
        act("&+L$n's $q &+bbl&+Cu&+brs &+Las it channels the powers of &+rEuronymous &+Lupon $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);

        spell_bigbys_crushing_hand(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
       /* spell_bigbys_crushing_hand(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);*/
        break;
      case 2:
        act("&+LYour $q &+ggl&+Go&+gws &+Las it channels the powers of &+rJubilex &+Lupon your foe!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+L$n's $q &+ggl&+Go&+gws &+Las it channels the powers of &+rJubilex &+Lupon you!&n", TRUE, ch, obj, vict, TO_VICT);
        act("&+L$n's $q &+ggl&+Go&+gws &+Las it channels the powers of &+rJubilex &+Lupon $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);
        spell_full_heal(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      }
    }
    else
    {
      switch( number(0,2) )
      {
      case 0:
        act("&+LYour $q &+rfl&+Rar&+res &+Las it channels the powers of &+rBel &+Lupon your foe!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+L$n's $q &+rfl&+Rar&+res &+Las it channels the powers of &+rBel &+Lupon you!&n", TRUE, ch, obj, vict, TO_VICT);
        act("&+L$n's $q &+rfl&+Rar&+res &+Las it channels the powers of &+rBel &+Lupon $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);

        spell_immolate(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 1:
        act("&+LYour $q &+bbl&+Cu&+brs &+Las it channels the powers of &+rEuronymous &+Lupon your foe!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+L$n's $q &+bbl&+Cu&+brs &+Las it channels the powers of &+rEuronymous &+Lupon you!&n", TRUE, ch, obj, vict, TO_VICT);
        act("&+L$n's $q &+bbl&+Cu&+brs &+Las it channels the powers of &+rEuronymous &+Lupon $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);

        spell_immolate(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);

        break;
      case 2:
        act("&+LYour $q &+ggl&+Go&+gws &+Las it channels the powers of &+rJubliex &+Lupon your foe!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+L$n's $q &+ggl&+Go&+gws &+Las it channels the powers of &+rJubilex &+Lupon you!&n", TRUE, ch, obj, vict, TO_VICT);
        act("&+L$n's $q &+ggl&+Go&+gws &+Las it channels the powers of &+rJubilex &+Lupon $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);

        spell_heal(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      }
    }
    return TRUE;
  }
  return FALSE;
}

int helmet_vampires(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict = NULL, target, necropet;
  P_obj corpse;
  int room, dam;
  int curr_time;
  int necropets[] = {55027, 55028, 50029, 0};

  if(cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( !IS_FIGHTING(ch) )
  {
    if( arg && (cmd == CMD_WORSHIP) )
    {
      if( isname(arg, "dead") )
      {
        curr_time = time(NULL);
        // 25 min timer.
        if( obj->timer[0] + 1500 <= curr_time )
        {
          act("&+WYou kneel down to the &+Lground &+Wand utter a prayer for the &+rd&+Ra&+rm&+Rn&+re&+Rd&+W.&n", TRUE, ch, obj, NULL, TO_CHAR);
          act("&+WYour $q &+Wbegins to &+rg&+Rlo&+rw &+Wand a strange &+bm&+Bis&+bt &+Wbegins filling the &+Cair&+W.&n", TRUE, ch, obj, NULL, TO_CHAR);
          act("&+WAs the &+bm&+Bis&+bt &+Wthickens, your nostrils are filled with the smell of &+gde&+Gca&+gyi&+Gng &+rf&+Rl&+re&+Rs&+rh&+W.&n", TRUE, ch, obj, NULL, TO_CHAR);
          act("&+W... the &+yground &+Wseems to be coming alive.&n", TRUE, ch, obj, NULL, TO_CHAR);

          act("$n &+Wkneels down to the &+Lground &+Wand utters a prayer for the &+rd&+Ra&+rm&+Rn&+re&+Rd&+W.&n", TRUE, ch, obj, NULL, TO_ROOM);
          act("$n's $q &+Wbegins &+rg&+Rlo&+rwi&+Rng &+Wand a strange &+bm&+Bis&+bt &+Wbegins filling the &+Cair&+W.&n", TRUE, ch, obj, NULL, TO_ROOM);
          act("&+WAs the &+bm&+Bis&+bt &+Wthickens, your nostrils are filled with the smell of &+gde&+Gca&+gyi&+Gng &+rf&+Rl&+re&+Rs&+rh&+W.&n", TRUE, ch, obj, NULL, TO_ROOM);
          act("&+W... the &+yground &+Wseems to be coming alive.&n", TRUE, ch, obj, NULL, TO_ROOM);

          necropet = read_mobile(necropets[1], VIRTUAL);
          char_to_room(necropet, ch->in_room, 0);
          setup_pet(necropet, ch, 1500, PET_NOCASH);
          add_follower(necropet, ch);
/*
          switch( number(0,2) )
          {
          case 0:
            necropet = read_mobile(55027, VIRTUAL);
            char_to_room(necropet, ch->in_room, 0);
            setup_pet(necropet, ch, 1500, PET_NOCASH);
            add_follower(necropet, ch);
            break;
          case 1:
            necropet = read_mobile(55028, VIRTUAL);
            char_to_room(necropet, ch->in_room, 0);
            setup_pet(necropet, ch, 1500, PET_NOCASH);
            add_follower(necropet, ch);
            break;
          case 2:
            necropet = read_mobile(55029, VIRTUAL);
            char_to_room(necropet, ch->in_room, 0);
            setup_pet(necropet, ch, 1500, PET_NOCASH);
            add_follower(necropet, ch);
          }
*/

          spell_prot_undead(56, ch, 0, SPELL_TYPE_SPELL, necropet, 0);

          act("&+WAs you rise to your feet, the &+gf&+Gou&+gl &+bm&+Bis&+bt &+Wlooks &+Ldrained &+Wand begins to fade away.&n", FALSE, ch, 0, NULL, TO_CHAR);
          act("&+WAs &n rises to $s feet, the &+gf&+Gou&+gl &+bm&+Bis&+bt &+Wlooks &+Ldrained &+Wand begins to fade away.&n", FALSE, ch, 0, NULL, TO_ROOM);

          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }
  }

  if( IS_FIGHTING(ch) && !IS_IMMOBILE(ch) && IS_ALIVE(ch->specials.fighting) && ch->in_room == ch->specials.fighting->in_room )
  {
    if( arg && (cmd == CMD_PRAY) )
    {
      if( isname(arg, "koztk") )
      {
        curr_time = time(NULL);
        vict = ParseTarget(ch, arg);
        // 15 min timer.
        if (obj->timer[0] + 900 <= curr_time)
        {
          act("You utter a prayer for '&+rKoztk&n'&n", TRUE, ch, obj, vict, TO_CHAR);
          act("$q &+rhums &+Land turns your eyes a deep &+rred&+L.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+L...an uncontrollable &+ranger &+Lstarts building within you.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+L...&nyou &+Lgasp as your skin turns &+Wpale &+Land a vicious set of &+rfangs &+Lburst out of your upper jaw.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+L...&nyou &+Lleap forward and sink your fangs into $N's neck, sending &+rblood &+Leverywhere.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+L...&nyou &+Lfeel &+Renergy &+Ldraining from $N &+Land flowing into you!&n", TRUE, ch, obj, vict, TO_CHAR);

          act("$n utters a prayer for '&+rKoztk&n'&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("$n's $q &+rhums &+Land turns $s eyes a deep &+rred&+L.&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("&+L...&n$n &+Lgasps as $s skin turns pale and a pair of fangs burst out of $s mouth.&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("&+L...&n$n &+Lleaps forward and sinks $s fangs into $N's neck, sending &+rblood &+Leveryone.&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("&+L...&n$n &+Lseems to drain $N's &+rstrength&+L.&n", TRUE, ch, obj, vict, TO_NOTVICT);

          act("$n utters a prayer for '&+rKoztk&n'&n", TRUE, ch, obj, vict, TO_VICT);
          act("$n's $q &+rhums &+Land turns $s eyes a deep &+rred&+L.&n", TRUE, ch, obj, vict, TO_VICT);
          act("&+L...&n$n &+Lgasps as $s skin turns pale and a pair of fangs burst out of $s mouth.&n", TRUE, ch, obj, vict, TO_VICT);
          act("&+L...&n$n &+Lleaps forward and sinks $s fangs into your neck, sending &+rblood &+Leveryone.&n", TRUE, ch, obj, vict, TO_VICT);
          act("&+L...&nYou &+Lfeel your &+rstrength &+Land &+Blifeforce &+Lflow into $n.&n", TRUE, ch, obj, vict, TO_VICT);

          switch (number(0, 2))
          {
          case 0:
            dam = BOUNDED(0, (GET_HIT(vict) + 9), 50);
            GET_HIT(vict) -= dam;
            if( GET_HIT(vict) < 0 )
            {
                GET_HIT(vict) = 0;
            }
            vamp(ch, dam / 2, (int) (GET_MAX_HIT(ch) * 1.3));
            break;
          case 1:
            dam = BOUNDED(0, (GET_HIT(vict) + 9), 75);
            GET_HIT(vict) -= dam;
            if( GET_HIT(vict) < 0 )
            {
              GET_HIT(vict) = 0;
            }
            vamp(ch, dam / 2, (int) (GET_MAX_HIT(ch) * 1.3));
            break;
          case 2:
            dam = BOUNDED(0, (GET_HIT(vict) + 9), 100);
            GET_HIT(vict) -= dam;
            if( GET_HIT(vict) < 0 )
            {
              GET_HIT(vict) = 0;
            }
            vamp(ch, dam / 2, (int) (GET_MAX_HIT(ch) * 1.3));
            break;
          default:
            break;
          }

          CharWait(ch,PULSE_VIOLENCE * 1);
          CharWait(vict,PULSE_VIOLENCE * 1);

          act("$N is stunned!&n", TRUE, ch, obj, vict, TO_CHAR);
          act("You grin with satisfaction.&n", TRUE, ch, obj, vict, TO_CHAR);

          act("$N is stunned!&n", TRUE, ch, obj, vict, TO_NOTVICT);
          act("$n grins with satisfaction.&n", TRUE, ch, obj, vict, TO_NOTVICT);

          act("You are stunned!&n", TRUE, ch, obj, vict, TO_VICT);
          act("$n grins with satisfaction.&n", TRUE, ch, obj, vict, TO_VICT);
/* Umm.. no.  No vampiric trance on a non-arti and no insta-kills.
          int rand = number(1, 1000);
          if(rand > 975)
          {
            spell_vampire(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          }
          if(rand == 1000)
          {
            act("You drank every last drop of $N's blood!&n", TRUE, ch, obj, vict, TO_CHAR);
            act("$n drank every last drop of $N's blood!&n", TRUE, ch, obj, vict, TO_NOTVICT);
            act("$n drank every last drop of your blood!&n", TRUE, ch, obj, vict, TO_VICT);
            act("&+LDarkness consumes you and you slip into oblivion.&n", TRUE, ch, obj, vict, TO_VICT);
            die(vict, ch);
          }
*/
          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

int buckler_saints(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room, max_hp;
  int curr_time = time(NULL);
  int curr_time2 = time(NULL);
  struct proc_data *data;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if ( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( !OBJ_WORN_BY(obj, ch) )
    {
      return FALSE;
    }

    if( !CHAR_IN_NO_MAGIC_ROOM(ch) )
    {
      // 3 min timer.
      if (obj->timer[1] + 180 <= curr_time2)
      {
        obj->timer[1] = curr_time2;

        if (GET_HIT(ch) < GET_MAX_HIT(ch))
        {
          act("&+LYour $q &+Mhums &nsoftly and fills your soul with a warm feeling.", TRUE, ch, obj, 0, TO_CHAR);
          act("$n&+L's $q &+Mhums &nsoftly.", TRUE, ch, obj, 0, TO_ROOM);

          spell_heal(60, ch, 0, SPELL_TYPE_SPELL, 0, 0);

          return TRUE;
        }
      }
    }
    return TRUE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "gauce") )
    {
      curr_time = time(NULL);
      // 7 min 30 sec timer.
      if( obj->timer[0] + 450 <= curr_time )
      {
        act("&+WYou utter a prayer for &+YSaint &+WGauce.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+WYour $q hums loudly, sending waves of &+csoothing &+Wsensations through the &+Cair&+W.&n", TRUE, ch, obj, vict, TO_CHAR);

        act("$n &+Wutters a prayer for &+YSaint &+WGauce.&n", TRUE, ch, obj, vict, TO_ROOM);
        act("$n's $q hums loudly, sending waves of &+csoothing &+Wsensations through the &+Cair&+W.&n", TRUE, ch, obj, vict, TO_ROOM);

        spell_group_heal(56, ch, 0, SPELL_TYPE_SPELL, 0, 0);
        spell_group_heal(56, ch, 0, SPELL_TYPE_SPELL, 0, 0);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
    else if( isname(arg, "macavor") )
    {
      curr_time = time(NULL);

      if( obj->timer[0] + 450 <= curr_time )
      {
        act("&+WYou utter a prayer for &+YSaint &+WMacavor.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+WYour $q hums loudly, sending waves of &+csoothing &+Wsensations through the &+Cair&+W.&n", TRUE, ch, obj, vict, TO_CHAR);

        act("$n &+Wutters a prayer for &+YSaint &+WMacavor.&n", TRUE, ch, obj, vict, TO_ROOM);
        act("$n's $q hums loudly, sending waves of &+csoothing &+Wsensations through the &+Cair&+W.&n", TRUE, ch, obj, vict, TO_ROOM);

        if( ch->group )
        {
          cast_as_area(ch, SPELL_ACCEL_HEALING, 56, 0);
          cast_as_area(ch, SPELL_ENDURANCE, 56, 0);
          cast_as_area(ch, SPELL_SOULSHIELD, 56, 0);
        }
        else
        {
          spell_accel_healing(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          spell_endurance(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
          spell_soulshield(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
    else if( isname(arg, "verdonnaly") )
    {
      curr_time = time(NULL);

      if( obj->timer[0] + 450 <= curr_time )
      {
        act("&+WYou utter a prayer for &+YSaint &+WVerdonnaly.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&+WYour $q hums loudly, sending waves of &+csoothing &+Wsensations through the &+Cair&+W.&n", TRUE, ch, obj, vict, TO_CHAR);

        act("$n &+Wutters a prayer for &+YSaint &+WVerdonnaly.&n", TRUE, ch, obj, vict, TO_ROOM);
        act("$n's $q hums loudly, sending waves of &+csoothing &+Wsensations through the &+Cair&+W.&n", TRUE, ch, obj, vict, TO_ROOM);

     /*   if (ch->group)
        {
          cast_as_area(ch, SPELL_VITALITY, 50, 0);
        }
        else
        {*/
        spell_vitality(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      //  }

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }

  if( IS_FIGHTING(ch) )
  {
    if( cmd == CMD_GOTHIT && !number(0, 24) )
    {
      // important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED           
      if( !(data = (struct proc_data *) arg) )
      {
        return FALSE;
      }
      vict = data->victim;
      if( !IS_ALIVE(vict) )
      {
        return FALSE;
      }

      act("$q &nsings with &+Yholy &npower as $N attempts to hit you.&n", TRUE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);
      act("$q &nsings with &+Yholy &npower as you attempt to hit $n.&n", TRUE, ch, obj, vict, TO_VICT | ACT_NOTTERSE);
      act("$q &nsings with &+Yholy &npower as $N attempts to hit $n.&n", TRUE, ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);

      if( GET_ALIGNMENT(vict) > 250 )
      {
        spell_dispel_good(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      if( GET_ALIGNMENT(vict) < -250 )
      {
        spell_dispel_evil(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
      return TRUE;
    }
  }
  return FALSE;
}

/*
int mob_death_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(cmd == CMD_PERIODIC && !number(0, 10))
  {
    switch(number(0,1))
    {
      case 0:

        act("The magic binding $n to this existence fades away.", TRUE, ch, obj, 0, TO_ROOM);
        die(ch, ch);

        break;
      case 1:

        act("The magic binding $n to this existence seems to weaken.", TRUE, ch, obj, 0, TO_ROOM);
        break;
      default:
        break;
    }
       
    return TRUE;
  }
  return FALSE;
}
*/

int rapier_penetration(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  struct affected_type *af;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!obj)
    return FALSE;

  if (IS_FIGHTING(ch))
  {
  
    room = ch->in_room;
    vict = ParseTarget(ch, arg);
  
    if (cmd == CMD_MELEE_HIT &&
        ch &&
        vict &&
        !number(0, 32 ) && // 3%
        CheckMultiProcTiming(ch) &&
        !IS_ELITE(vict))
        
    {

      act("Your $q &+mpulsates &+Lwith energy!&n", TRUE, ch, obj, vict, TO_CHAR);

      act("$n&+L's $q &+mpulsates &+Lwith energy!&n", TRUE, ch, obj, vict, TO_NOTVICT);

      act("$n&+L's $q &+mpulsates &+Lwith energy!&n", TRUE, ch, obj, vict, TO_VICT);


      if (number(1, 10) > 5)
      {
      	if (affected_by_spell(vict, SPELL_STONE_SKIN))
        {
          act("$N's &+Lstone skin &ncrumbles to the ground!&n", TRUE, ch, obj, vict, TO_CHAR);

          act("$N's &+Lstone skin &ncrumbles to the ground!&n", TRUE, ch, obj, vict, TO_NOTVICT);

          act("&nYour &+Lstone skin &ncrumbles to the ground!&n", TRUE, ch, obj, vict, TO_VICT);

          af = get_spell_from_char(target, SPELL_STONE_SKIN);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
        }
      }

      if (number(1, 10) > 5)
      {
     	if (affected_by_spell(vict, SPELL_GLOBE))
        {
          act("$N's &+Rglobe of invulnerability &nshatters to pieces!&n", TRUE, ch, obj, vict, TO_CHAR);

          act("$N's &+Rglobe of invulnerability &nshatters to pieces!&n", TRUE, ch, obj, vict, TO_NOTVICT);

          act("&nYour &+Rglobe of invulnerability &nshatters to pieces!&n", TRUE, ch, obj, vict, TO_VICT);

          af = get_spell_from_char(target, SPELL_GLOBE);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
        }
      }

      if (number(1, 10) > 5)
      {
     	if (affected_by_spell(vict, SPELL_SHADOW_SHIELD))
        {
          act("&nWaves of energy destroy $N's &+Lshadow shield&n!", TRUE, ch, obj, vict, TO_CHAR);

          act("&nWaves of energy destroy $N's &+Lshadow shield&n!", TRUE, ch, obj, vict, TO_NOTVICT);

          act("&nWaves of energy destroy your &+Lshadow shield&n!", TRUE, ch, obj, vict, TO_VICT);

          af = get_spell_from_char(target, SPELL_SHADOW_SHIELD);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
        }
      }

      if (number(1, 10) > 5)
      {
     	if (affected_by_spell(vict, SPELL_BIOFEEDBACK))
        {
          act("The thick &+Ggreen &+gmist &nsurrounding $N turns pale and fades away.", TRUE, ch, obj, vict, TO_CHAR);

          act("The thick &+Ggreen &+gmist &nsurrounding $N turns pale and fades away.", TRUE, ch, obj, vict, TO_NOTVICT);

          act("The thick &+Ggreen &+gmist &nsurrounding you turns pale and fades away.", TRUE, ch, obj, vict, TO_VICT);

          af = get_spell_from_char(target, SPELL_BIOFEEDBACK);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
        }
      }

      if (number(1, 10) > 5)
      {
      	if (affected_by_spell(vict, SPELL_FIRESHIELD))
        {
          act("The &+rflames &nsurrounding $N flicker out of existence.&n", TRUE, ch, obj, vict, TO_CHAR);

          act("The &+rflames &nsurrounding $N flicker out of existence.&n", TRUE, ch, obj, vict, TO_NOTVICT);

          act("The &+rflames &nsurrounding you flicker out of existence.&n", TRUE, ch, obj, vict, TO_VICT);

          af = get_spell_from_char(target, SPELL_FIRESHIELD);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
        }
      }

      if (number(1, 10) > 5)
      {
      	if (affected_by_spell(vict, SPELL_COLDSHIELD))
        {
          act("The &+Bice &nsurrounding $N melt away.&n", TRUE, ch, obj, vict, TO_CHAR);

          act("The &+Bice &nsurrounding $N melt away.&n", TRUE, ch, obj, vict, TO_NOTVICT);

          act("The &+Bice &nsurrounding you melt away.&n", TRUE, ch, obj, vict, TO_VICT);

          af = get_spell_from_char(target, SPELL_COLDSHIELD);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
        }
      }

      switch(number(0,3))
      {
        case 0:
          spell_chill_touch(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);

          break;
        case 1:
          spell_burning_hands(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);

          break;
        case 2:
          spell_shocking_grasp(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);

          break;
        case 3:
          spell_acid_blast(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          break;
        default:
          break;

      }

      switch(number(0,3))
      {
        case 0:
          spell_chill_touch(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);

          break;
        case 1:
          spell_burning_hands(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);

          break;
        case 2:
          spell_shocking_grasp(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);

          break;
        case 3:
          spell_acid_blast(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          break;
        default:
          break;

      }

      return TRUE;
    }
  }
  return FALSE;
}

/*

int sword_random(P_obj obj, P_char ch, int cmd, char *arg)
{ 
  P_char vict, target;
  int curr_time, rand;
  struct affected_type *af;
  int *spells = NULL;

#define MAX_RANDOMSPELLS 52

    int      RandomSpells[MAX_RANDOMSPELLS] = {
    SPELL_ARMOR, 
    SPELL_BLESS, 
    SPELL_DETECT_MAGIC,
    SPELL_SLOW_POISON, 
    SPELL_PROTECT_FROM_COLD, 
    SPELL_PROTECT_FROM_FIRE,
    SPELL_PROTECT_FROM_GAS, 
    SPELL_PROTECT_FROM_EVIL,
    SPELL_PROTECT_FROM_GOOD, 
    SPELL_PROTECT_FROM_ACID,
    SPELL_PROTECT_FROM_LIGHTNING, 
    SPELL_ACCEL_HEALING,
    SPELL_ENDURANCE, 
    SPELL_FIRESHIELD,
    SPELL_COLDSHIELD,
    SPELL_HASTE,
    SPELL_STONE_SKIN, 
    SPELL_SPIRIT_WARD,
    SPELL_GREATER_SPIRIT_WARD, 
    SPELL_GLOBE, 
    SPELL_DETECT_INVISIBLE,
    SPELL_DETECT_ILLUSION, 
    SPELL_STRENGTH, 
    SPELL_DEXTERITY,
    SPELL_ENLARGE, 
    SPELL_REDUCE, 
    SPELL_VITALITY, 
    SPELL_SHADOW_SHIELD,
    SPELL_AID, 
    SPELL_BARKSKIN, 
    SPELL_REGENERATION, 
    SPELL_LEVITATE,
    SPELL_FLY, 
    SPELL_PULCHRITUDE, 
    SPELL_SERENDIPITY, 
    SPELL_PANTHERSPEED,
    SPELL_HAWKVISION, 
    SPELL_SPIRIT_ARMOR, 
    SPELL_PHANTOM_ARMOR, 
    SPELL_WOLFSPEED,
    SPELL_FORTITUDE, 
    SPELL_BIOFEEDBACK,
    SPELL_DISPEL_MAGIC, 
    SPELL_ENHANCED_STR, 
    SPELL_ENHANCED_DEX, 
    SPELL_ENHANCED_AGI, 
    SPELL_ENHANCED_CON, 
    SPELL_INVISIBLE,
    SPELL_INFRAVISION, 
    SPELL_FARSEE,
    SPELL_MAGE_FLAME, 
    SPELL_GLOBE_OF_DARKNESS 
    };

#define MAX_BENESPELLS 11

    int      BeneSpells[] = {
    SPELL_VIGORIZE_SERIOUS, 
    SPELL_VIGORIZE_CRITIC, 
    SPELL_CURE_SERIOUS, 
    SPELL_CURE_CRITIC, 
    SPELL_REMOVE_POISON, 
    SPELL_CURE_DISEASE, 
    SPELL_PURIFY_SPIRIT, 
    SPELL_CURE_BLIND,
    SPELL_REMOVE_CURSE, 
    SPELL_CURE_LIGHT, 
    SPELL_VIGORIZE_LIGHT
    };

  spells = RandomSpells;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!obj)
    return FALSE;

  if(cmd == CMD_PERIODIC && !number(0, 2))
  {
    act("$n's $q shimmers with magical energy.", TRUE, ch, obj, vict, TO_ROOM);
    act("Your $q shimmers with magical energy.", TRUE, ch, obj, vict, TO_CHAR);

    switch(number(0,1))
    {
      case 0:
        ((*spells[RandomSpells[number(0, MAX_RANDOMSPELLS-1)]].spell_pointer) (number(25,56), ch, 0, SPELL_TYPE_SPELL, ch, 0));
//        RandomSpells[number(0, MAX_RANDOMSPELLS-1)] (number(25,56), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      case 1:
        BeneSpells[1](number(25,56), ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      default:
        break;

    }
    return TRUE;
  }

  if (IS_FIGHTING(ch))
  {
    if (cmd == CMD_MELEE_HIT && ch && !number(0,20))
    {

      room = ch->in_room;
      vict = ParseTarget(ch, arg);
 
      act("Your $q pulses with energy and unleashes a spectrum of light at $N!&n", TRUE, ch, obj, vict, TO_CHAR);

      act("$n's $q pulses with energy and unleashes a spectrum of light at $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);

      act("$n's $q pulses with energy and unleashes a spectrum of light at $N!&n", TRUE, ch, obj, vict, TO_VICT);

      rand = number(0, 100);
 
      if (rand > 90)
      {
        if (affected_by_spell(vict, SPELL_COLDSHIELD))
        {
	  af = get_spell_from_char(target, SPELL_COLDSHIELD);
   	  if (af)
   	  {
	    affect_remove(vict, af);
	    wear_off_message(vict, af);
	  }
          spell_fireshield(number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
        }
      }

      rand = number(0, 100);
 
      if (rand > 90)
      {
        if (affected_by_spell(vict, SPELL_FIRESHIELD))
        {
          af = get_spell_from_char(target, SPELL_FIRESHIELD);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
          spell_coldshield(number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
        }
      }

      rand = number(0, 100);
 
      if (rand > 90)
      {
        if (affected_by_spell(vict, SPELL_VITALITY))
        {
          af = get_spell_from_char(target, SPELL_VITALITY);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
          spell_vitality(number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
        }
      }

      rand = number(0, 100);
 
      if (rand > 90)
      {
        if (affected_by_spell(vict, SPELL_REDUCE))
        {
          af = get_spell_from_char(target, SPELL_REDUCE);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
          spell_enlarge(number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
        }
      }
 
     rand = number(0, 100);
 
      if (rand > 90)
      {
        if (affected_by_spell(vict, SPELL_ENLARGE))
        {
          af = get_spell_from_char(target, SPELL_ENLARGE);
          if (af)
          {
            affect_remove(vict, af);
            wear_off_message(vict, af);
          }
          spell_reduce(number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
        }
      }

      rand = RandomSpells[1];
 
      if (affected_by_spell(vict, rand))
      {
        af = get_spell_from_char(target, rand);
        if (af)
        {
          affect_remove(vict, af);
          wear_off_message(vict, af);
        }
        RandomSpells[1](number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
      }
  
      rand = RandomSpells[1];
 
      if (affected_by_spell(vict, rand))
      {
        af = get_spell_from_char(target, rand);
        if (af)
        {
          affect_remove(vict, af);
          wear_off_message(vict, af);
        }
        RandomSpells[1](number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
      }

      rand = RandomSpells[1];
 
      if (affected_by_spell(vict, rand))
      {
        af = get_spell_from_char(target, rand);
        if (af)
        {
          affect_remove(vict, af);
          wear_off_message(vict, af);
        }
        RandomSpells[1](number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
      }

      rand = RandomSpells[1];
 
      if (affected_by_spell(vict, rand))
      {
        af = get_spell_from_char(target, rand);
        if (af)
        {
          affect_remove(vict, af);
          wear_off_message(vict, af);
        }
        RandomSpells[1](number(25,56), vict, 0, SPELL_TYPE_SPELL, vict, 0);
      }

      return TRUE;
    }
  }
  return FALSE;
}

*/


int gauntlets_legend(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  int curr_time = time(NULL);
  int curr_time2 = time(NULL);
  struct proc_data *data;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if ( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( !CHAR_IN_NO_MAGIC_ROOM(ch) )
    {
      // 3 min timer.
      if( obj->timer[1] + 180 <= curr_time2 )
      {
        obj->timer[1] = curr_time2;

        if (GET_HIT(ch) < GET_MAX_HIT(ch))
        {
          act("&+LYour $q &+Mhums &nsoftly and fills your soul with a warm feeling.", TRUE, ch, obj, 0, TO_CHAR);
          act("$n&+L's $q &+Mhums &nsoftly.", TRUE, ch, obj, 0, TO_ROOM);

          spell_stone_skin(60, ch, 0, SPELL_TYPE_SPELL, 0, 0);
          return TRUE;
        }
      }
    }
    return TRUE;
  }

  // 1/10 chance.
  if( IS_FIGHTING(ch) && (cmd == CMD_PERIODIC) && !number(0, 9) )
  {

    room = ch->in_room;
    vict = ParseTarget(ch, arg);

    act("$q &nbegin to &+mglow&n.", TRUE, ch, obj, vict, TO_CHAR);
    act("$q &nbegin to &+mglow&n.", TRUE, ch, obj, vict, TO_NOTVICT);
    act("$q &nbegin to &+mglow&n.", TRUE, ch, obj, vict, TO_VICT);

    if(GET_HIT(ch) < (GET_MAX_HIT(ch) / 4))
    {
      act("$q &nsend a &+csoothing &nsensation up your arm.&n.", TRUE, ch, obj, vict, TO_CHAR);
      act("$n looks healthier.", TRUE, ch, obj, vict, TO_NOTVICT);
      act("$n looks healthier.", TRUE, ch, obj, vict, TO_VICT);

      GET_HIT(ch) += (GET_MAX_HIT(ch) / 8);
    }
    else if(GET_HIT(ch) < (GET_MAX_HIT(ch) / 2))
    {
      act("$q &nsend a &+csoothing &nsensation up your arm.&n.", TRUE, ch, obj, vict, TO_CHAR);
      act("$n looks healthier.", TRUE, ch, obj, vict, TO_NOTVICT);
      act("$n looks healthier.", TRUE, ch, obj, vict, TO_VICT);

      GET_HIT(ch) += (GET_MAX_HIT(ch) / 12);
    }
    if(GET_HIT(vict) < (GET_MAX_HIT(vict) / 4) && GET_HIT(vict) > (GET_MAX_HIT(vict) / 20))
    {
      act("A &+rpulse &nof &+Menergy &nshoots from&n $q&+M, striking&n $N &+Min the chest&n.", TRUE, ch, obj, vict, TO_CHAR);
      act("A &+rpulse &nof &+Menergy &nshoots from&n $q&+M, striking&n $N &+Min the chest&n.", TRUE, ch, obj, vict, TO_NOTVICT);
      act("A &+rpulse &nof &+Menergy &nshoots from&n $q, &+Mstriking you in the chest&n.", TRUE, ch, obj, vict, TO_VICT);

      GET_HIT(vict) -= (GET_MAX_HIT(vict) / 25);
    }
    else if(GET_HIT(vict) < (GET_MAX_HIT(vict) / 2) && GET_HIT(vict) > (GET_MAX_HIT(vict) / 10))
    {
      act("A &+rpulse &nof &+Menergy &nshoots from $q, striking $N in the chest&n.", TRUE, ch, obj, vict, TO_CHAR);
      act("A &+rpulse &nof &+Menergy &nshoots from $q, striking $N in the chest&n.", TRUE, ch, obj, vict, TO_NOTVICT);
      act("A &+rpulse &nof &+Menergy &nshoots from $q, striking you in the chest&n.", TRUE, ch, obj, vict, TO_VICT);

      GET_HIT(vict) -= (GET_MAX_HIT(vict) / 15);
    }
    return TRUE;
  }

  if( !IS_FIGHTING(ch) )
  {
    if( arg && (cmd == CMD_CLAP) )
    {
      if( isname(arg, "hands") )
      {
        curr_time = time(NULL);
        // 10 min timer.
        if( obj->timer[0] + 600 <= curr_time )
        {
          act("&nAs you clap your hands, $q begin to tremble with power.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&+LDarkness &ncreeps up and begins to surround you.&n", TRUE, ch, obj, vict, TO_CHAR);

          act("&nAs $n claps $s hands, $q begin to tremble with power.&n", TRUE, ch, obj, vict, TO_ROOM);
          act("&+LDarkness &ncreeps up and begins to surround you.&n", TRUE, ch, obj, vict, TO_ROOM);

          spell_nonexistence(50, ch, 0, SPELL_TYPE_SPELL, 0, 0);
          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

/*

int boots_abyss(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  int curr_time;
  int curr_time2;

  if ( !ch )
	return FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == 0)
    hummer(obj);

  if (!OBJ_WORN(obj))
    return FALSE;

  if(!IS_FIGHTING(ch))
  {
    if (arg && (cmd == CMD_SUMMON))
    {
      if (isname(arg, "poseidon"))
      {
        curr_time = time(NULL);
  
        if (obj->timer[0] + 1200 <= curr_time)
        {
          act("&nAs you click your heels, $q start humming.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&nA dark blue portal materializes before you and Poseidon, Lord of the Ocean, steps out of it, grinning.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&nPoseidon approaches you and then hands you his magical trident.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&nThe Lord of the Ocean steps back into the portal, which promptly vanishes into thin air.&n", TRUE, ch, obj, vict, TO_CHAR);

          act("&nAs $N clicks $s heels, $q start humming.&n", TRUE, ch, obj, vict, TO_ROOM);
          act("&nA dark blue portal materializes before you and Poseidon steps out of it, grinning.&n", TRUE, ch, obj, vict, TO_ROOM);
          act("&nPoseidon approaches $N and then hands $m his magical trident.&n", TRUE, ch, obj, vict, TO_ROOM);
          act("&nThe Lord of the Ocean steps back into the portal, which promptly vanishes into thin air.&n", TRUE, ch, obj, vict, TO_ROOM);
  
          obj_to_char(read_object(55338, VIRTUAL), ch);

          obj->timer[0] = curr_time;
          return TRUE;
        }
      }
    }
  
    if (arg && (cmd == CMD_TAP))
    {
      if (isname(arg, "boots") || isname(arg, "abyss"))
      {
        curr_time2 = time(NULL);
  
        if (obj->timer[1] + 600 <= curr_time2)
        {
          act("&nYou tap your heels.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("&n$q conjures a powerful potion.&n", TRUE, ch, obj, vict, TO_CHAR);

          act("$N taps $s heels..&n", TRUE, ch, obj, vict, TO_ROOM);
          act("&n$q conjures a powerful potion.&n", TRUE, ch, obj, vict, TO_ROOM);
  
          obj_to_char(read_object(55339, VIRTUAL), ch);

          obj->timer[1] = curr_time2;
          return TRUE;
        }
      }
    }
  }

  if (arg && (cmd == CMD_STOMP))
  {
    if (isname(arg, "ground"))
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 600 <= curr_time)
      {
        act("&nYou stomp the ground!&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&nA massive portal opens before you and fills the room with cold water!&n.", TRUE, ch, obj, vict, TO_CHAR);

        act("&nYou stomp the ground!&n", TRUE, ch, obj, vict, TO_ROOM);
        act("&nA massive portal opens before you and fills the room with cold water!&n.", TRUE, ch, obj, vict, TO_ROOM);
  
        cast_as_area(ch, SPELL_DREAD_WAVE, 50, 0);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "water"))
    {
      curr_time2 = time(NULL);

      if (obj->timer[1] + 600 <= curr_time2)
      {
        act("&nYou say, 'water'&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&nA powerful wave of aquatic energy resonates from $q&n.", TRUE, ch, obj, vict, TO_CHAR);

        act("$N says, 'water'&n", TRUE, ch, obj, vict, TO_ROOM);
        act("&nA powerful wave of aquatic energy resonates from $q&n.", TRUE, ch, obj, vict, TO_ROOM);
  
        spell_create_spring(50, ch, 0, SPELL_TYPE_SPELL, 0, 0);
        cast_as_area(ch, SPELL_WATERBREATH, 50, 0);

        obj->timer[1] = curr_time2;
        return TRUE;
      }
    }
  }

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "air"))
    {
      curr_time2 = time(NULL);

      if (obj->timer[1] + 600 <= curr_time2)
      {
        act("&nYou say, 'water'&n", TRUE, ch, obj, vict, TO_CHAR);
        act("&nA powerful wave of aquatic energy resonates from $q&n.", TRUE, ch, obj, vict, TO_CHAR);

        act("$N says, 'water'&n", TRUE, ch, obj, vict, TO_ROOM);
        act("&nA powerful wave of aquatic energy resonates from $q&n.", TRUE, ch, obj, vict, TO_ROOM);
  
        spell_airy_water(50, ch, 0, SPELL_TYPE_SPELL, 0, 0);

        obj->timer[1] = curr_time2;
        return TRUE;
      }
    }
  }
  return FALSE;
}


int poseidon_trident(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time, room, rand;
  P_char vict, carrier, kraken, mental;
  struct affected_type *af;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(cmd == CMD_PERIODIC && !number(0, 2))
  {

    if (OBJ_CARRIED(obj))
    {

      carrier = obj->loc.carrying;

      send_to_char("$q decides to return to it's rightful master!\r\n", carrier);

      send_to_room("$q decides to return to it's rigthful master!\r\n", obj->loc.room);

    }
    else
    if (OBJ_ROOM(obj))
    {

      send_to_room("$q decides to return to it's rigthful master!\r\n", obj->loc.room);

    }
    else 
    if (OBJ_INSIDE(obj))
    {

      send_to_room("$q decides to return to it's rigthful master!\r\n", obj->loc.room);

    }

    extract_obj(obj, TRUE);
    return TRUE;
  }


  if (cmd == CMD_REMOVE && isname(arg, obj->name))
  {
    if (affected_by_spell(ch, SPELL_BLUR))
    {
      af = get_spell_from_char(ch, SPELL_BLUR);
      if (af)
      {
        affect_remove(ch, af);
        wear_off_message(ch, af);
      }
    }

    if (affected_by_spell(ch, SPELL_VITALITY))
    {
      af = get_spell_from_char(ch, SPELL_VITALITY);
      if (af)
      {
        affect_remove(ch, af);
        wear_off_message(ch, af);
      }
    }
    return TRUE;
  }

  if (IS_FIGHTING(ch))
  {
    if (cmd == CMD_MELEE_HIT && ch && !number(0,20))
    {
      switch(number(0,2))
      {
        case 0:      

          room = ch->in_room;
          vict = ParseTarget(ch, arg);
 
          act("Your $q &+mpulsates &+Lwith energy!&n", TRUE, ch, obj, vict, TO_CHAR);

          act("$n&+L's $q &+mpulsates &+Lwith energy!&n", TRUE, ch, obj, vict, TO_NOTVICT);
   
          act("$n&+L's $q &+mpulsates &+Lwith energy!&n", TRUE, ch, obj, vict, TO_VICT);

          spell_dread_wave(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
          return TRUE;

        break;
        case 1:

          mental = read_mobile(1103, VIRTUAL);
          char_to_room(mental, ch->in_room, 0);
          setup_pet(mental, ch, 1500, PET_NOCASH);
          add_follower(mental, ch);  
          attack(mental, vict);

        break;
        case 2:

          mental = read_mobile(1140, VIRTUAL);
          char_to_room(mental, ch->in_room, 0);
          setup_pet(mental, ch, 1500, PET_NOCASH);
          add_follower(mental, ch);  
          attack(mental, vict);
        default:
          break;

      }
    return TRUE;
    }
  }

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "kraken"))
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 600 <= curr_time)
      {
        act("&nYou call upon the dreaded Kraken.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$N calls upon the dreaded Kraken.&n", TRUE, ch, obj, vict, TO_ROOM);

        kraken = read_mobile(55033, VIRTUAL);
        char_to_room(kraken, ch->in_room, 0);
        setup_pet(kraken, ch, 1500, PET_NOCASH);

        if(number(0, 10) < 8)
        {
          add_follower(kraken, ch);  
        }
        else
        {
          act("The Kraken is NOT happy to see you.&n", TRUE, ch, obj, vict, TO_CHAR);
          act("The Kraken is NOT happy to see $n!&n", TRUE, ch, obj, vict, TO_ROOM);
 
          attack(kraken, ch);
        }

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "poseidon"))
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 600 <= curr_time)
      {
        act("&nYou summon the powers of Poseidon.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$N summons the powers of Poseidon.&n", TRUE, ch, obj, vict, TO_ROOM);

        spell_vitality(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_blur(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }
  return FALSE;
}


int platemail_fame(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room, fame = 0;
  int curr_time = time(NULL);
  int curr_time2 = time(NULL);
  struct proc_data *data;
  struct affected_type *af;

  if ( !ch )
	return FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == 0)
    hummer(obj);

  if (!OBJ_WORN(obj))
    return FALSE;

  if(obj->timer[1] + 180 <= curr_time2)
  {
    act("$n&+L's $q &+Mhums &nsoftly.", TRUE, ch, obj, vict, TO_ROOM);
    act("&+LYour $q &+Mhums &nsoftly.", TRUE, ch, obj, vict, TO_CHAR);

    spell_heal(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);

    obj->timer[1] = curr_time2;
    return TRUE;
  }

  if (arg && (cmd == CMD_RUB))
  {
    if (isname(arg, "platemail")  || isname(arg, "fame"))
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 600 <= curr_time)
      {
        act("&nYou cackle insanely, '&+MHohaaaah&+m! &+MLook who it is&+m! &+MMe&+m!!&n'", TRUE, ch, obj, vict, TO_CHAR);

        act("$N cackles insanely, '&+MHohaaaah&+m! &+MLook who it is&+m! &+MMe&+m!!&n'", TRUE, ch, obj, vict, TO_ROOM);

        for(vict = world[obj->loc.room].people; vict; vict = vict->next_in_room)
        {
          if (IS_PC(vict) && GET_LEVEL(vict) > 35 && !ch)
          continue;

          act("$n looks awestruck by your presence&n.'", TRUE, ch, obj, vict, TO_CHAR);

          act("You are simply awestruck by $N's presence.&n", TRUE, ch, obj, vict, TO_VICT);

          act("$n is simply awestruck by $N's presence.&n", TRUE, ch, obj, vict, TO_NOTVICT);

          switch(number(0,2))
          {
            case 0:
              act("$n bows deeply before you.&n", TRUE, ch, obj, vict, TO_CHAR);
              act("You bow deeply before $N.&n", TRUE, ch, obj, vict, TO_VICT);
              act("$n bows deeply before $N.&n", TRUE, ch, obj, vict, TO_NOTVICT);
            break;
            case 1:
              act("$n claps $s hands and cheers wildly for you!&n", TRUE, ch, obj, vict, TO_CHAR);
              act("You clap your hands and cheer wildly for $N.&n", TRUE, ch, obj, vict, TO_VICT);
              act("$n claps $s hands and cheers wildly for $N.&n", TRUE, ch, obj, vict, TO_NOTVICT);
            break;
            case 2:
              act("$n's eyes pop wide open and says, 'Holy cow! You're famous!!&n", TRUE, ch, obj, vict, TO_CHAR);
              act("Your eyes pop wide open and say, 'Holy cow! You're famous!!&n", TRUE, ch, obj, vict, TO_VICT);
              act("$n's eyes pop wide open and says, 'Holy cow! You're famous!!&n", TRUE, ch, obj, vict, TO_NOTVICT);
            default:
              break;
          }

          CharWait(vict,PULSE_VIOLENCE * dice(0,1));
          CharWait(ch,PULSE_VIOLENCE * dice(0,3));
          fame++;

        }	

      if(fame = 0)
      {
        act("$q trembles with anger as nobody witnessed how famous you are.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("You are zapped by a powerful force!&n", TRUE, ch, obj, vict, TO_CHAR);

        spell_detonate(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_detonate(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_detonate(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 0)
      {
        spell_haste(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_improved_invisibility(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_fly(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 3)
      {
        spell_stone_skin(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_globe(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 6)
      {
        spell_regeneration(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_vitality(51, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_pantherspeed(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 9)
      {
        spell_greater_spirit_ward(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_spirit_ward(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_spirit_armor(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 12)
      {
        spell_pass_without_trace(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_detect_illusion(53, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 15)
      {
        spell_blur(53, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_serendipity(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 18)
      {
        spell_stornogs_spheres(52, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 21)
      {
        spell_deflect(56, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      if(fame > 24)
      {
        spell_inertial_barrier(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }

      obj->timer[0] = curr_time;
      return TRUE;
      }
    }
  }
  return FALSE;
}


int tower_shield_indomnibility(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  int curr_time;
  struct proc_data *data;
  struct affected_type *af;

  if ( !ch )
	return FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == 0)
    hummer(obj);

  if (!OBJ_WORN(obj))
    return FALSE;

  return FALSE;
}


int armplates_cosmos(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  int curr_time;
  struct proc_data *data;
  struct affected_type *af;

  if ( !ch )
	return FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == 0)
    hummer(obj);

  if (!OBJ_WORN(obj))
    return FALSE;

  return FALSE;
}

*/

int attribute_scroll(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char    vict = NULL;
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

  if(!IS_FIGHTING(ch))
  {
    if (arg && (cmd == CMD_RECITE))
    {
      if (isname(arg, "scroll"))
      {

        act("&+WYou begin reciting the words on $q, which quickly transforms and reveals its true identity.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$n begins reciting the words on $q, which quickly transforms and reveals its true identity.&n", TRUE, ch, obj, vict, TO_ROOM);

        extract_obj(obj, TRUE);
        obj_to_char(read_object(number(55352,55360), VIRTUAL), ch);
        return TRUE;
      }
    }
  }
  return FALSE;
}

int earring_powers(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time, first, second;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  // The earring prevents all encrusting since encrust can be used to cheese the proc.
  //   If you're feeling like limiting to when encrust target is the earring, fine.  However,
  //   this was easy and I'm lazy. :)
  //   Also, might want to check to see if it stops someone else in room from encrusting...
  if( cmd == CMD_ENCRUST )
  {
    // This should always be true, but just in case..
    if( IS_ALIVE(ch) )
    {
      act("&nYour $q prevents encrust magic from working.&n", TRUE, ch, obj, NULL, TO_CHAR);
    }
    return TRUE;
  }

  if( cmd != CMD_PERIODIC || !OBJ_WORN(obj) )
  {
    return FALSE;
  }
  ch = obj->loc.wearing;

  if( !IS_ALIVE(ch) || IS_NPC(ch) )
  {
    return FALSE;
  }

  curr_time = time(NULL);
  // 10 min timer.
  if( obj->timer[0] + 600 <= curr_time )
  {

    act("&nYour $q hums briefly.&n", TRUE, obj->loc.wearing, obj, NULL, TO_CHAR);
    act("$n's $q hums briefly.&n", TRUE, ch, obj, NULL, TO_ROOM);

/* Too much power for a single item.
    switch( number(0, 3) )
    {
    case 0:
      obj->affected[2].location = APPLY_SAVING_SPELL;
      obj->affected[2].modifier = number(-2,-5);
      break;
    case 1:
      obj->affected[2].location = APPLY_SAVING_FEAR;
      obj->affected[2].modifier = number(-2,-5);
      break;
    case 2:
      obj->affected[2].location = APPLY_SAVING_PARA;
      obj->affected[2].modifier = number(-2,-5);
      break;
    case 3:
      obj->affected[2].location = APPLY_SAVING_BREATH;
      obj->affected[2].modifier = number(-2,-5);
      break;
    default:
      break;
    }
*/
    first = number(0, 8);
    do{ second = number(0, 8); } while( second == first );

    switch( first )
    {
    case 0:
      obj->affected[0].location = APPLY_STR_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    case 1:
      obj->affected[0].location = APPLY_DEX_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    case 2:
      obj->affected[0].location = APPLY_AGI_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    case 3:
      obj->affected[0].location = APPLY_CON_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    case 4:
      obj->affected[0].location = APPLY_INT_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    case 5:
      obj->affected[0].location = APPLY_WIS_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    case 6:
      obj->affected[0].location = APPLY_POW_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    case 7:
      obj->affected[0].location = APPLY_CHA_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    case 8:
      obj->affected[0].location = APPLY_LUCK_MAX;
      obj->affected[0].modifier = number(3, 5);
      break;
    default:
      break;
    }

    switch( second )
    {
    case 0:
      obj->affected[1].location = APPLY_STR_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    case 1:
      obj->affected[1].location = APPLY_DEX_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    case 2:
      obj->affected[1].location = APPLY_AGI_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    case 3:
      obj->affected[1].location = APPLY_CON_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    case 4:
      obj->affected[1].location = APPLY_INT_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    case 5:
      obj->affected[1].location = APPLY_WIS_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    case 6:
      obj->affected[1].location = APPLY_POW_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    case 7:
      obj->affected[1].location = APPLY_CHA_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    case 8:
      obj->affected[1].location = APPLY_LUCK_MAX;
      obj->affected[1].modifier = number(3, 5);
      break;
    default:
      break;
    }

    balance_affects(ch);
    obj->timer[0] = curr_time;
    return TRUE;
  }
  return FALSE;
}

int lorekeeper_scroll(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char    vict = NULL;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !obj || !IS_ALIVE(ch) || IS_FIGHTING(ch) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_RECITE) )
  {
    if( isname(arg, "legend") )
    {
      act("&+WYou begin reciting the words on $q, which quickly transforms and reveals its true identity.&n", TRUE, ch, obj, vict, TO_CHAR);
      act("$n begins reciting the words on $q, which quickly transforms and reveals its true identity.&n", TRUE, ch, obj, vict, TO_ROOM);

      extract_obj(obj, TRUE);
      obj_to_char(read_object(number(55364,55365), VIRTUAL), ch);
      return TRUE;
    }
  }
  return FALSE;
}

int gladius_backstabber(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  P_obj    first_w, second_w;
  int room;
  int curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  first_w = ch->equipment[WIELD];
  second_w = ch->equipment[SECONDARY_WEAPON];

  if( IS_FIGHTING(ch) )
  {

    room = ch->in_room;
    vict = ParseTarget(ch, arg);
    if( !IS_ALIVE(vict) || !CanDoFightMove(ch, vict) || GET_POS(ch) < POS_STANDING )
    {
      return FALSE;
    }

    // 1/30 chance.
    if( cmd == CMD_MELEE_HIT && !number(0, 29) && CheckMultiProcTiming(ch) )
    {
      act("&nYour $q &nbegins to &+mhum&n.", TRUE, ch, obj, vict, TO_CHAR);
      act("&nYou quickly vanish and reappear behind $N!", TRUE, ch, obj, vict, TO_CHAR);
      act("$n's $q &nbegins to &+mhum&n.", TRUE, ch, obj, vict, TO_NOTVICT);
      act("$n quickly vanishes and reappears behind $N!", TRUE, ch, obj, vict, TO_NOTVICT);
      act("$n's $q &nbegins to &+mhum&n.", TRUE, ch, obj, vict, TO_VICT);
      act("$n quickly vanishes and reappears behind you!", TRUE, ch, obj, vict, TO_VICT);

      if( first_w && IS_BACKSTABBER(first_w) )
      {
        single_stab(ch, vict, first_w);
      }
      else if( second_w )
      {
        single_stab(ch, vict, second_w);
      }

      return TRUE;
    }
  }
  return FALSE;
}

int damnation_staff(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  int curr_time;
  struct proc_data *data;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }
  // 1/20 chance.
  if( cmd == CMD_GOTHIT && !number(0, 19) )
  {
    if( !(data = (struct proc_data *) arg) )
    {
      return FALSE;
    }
    vict = data->victim;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }

    act("&+LYour $q &+mflares &+rbril&+Rli&+rantly &+Las it intercepts &n$N's &+Lattack and &+rsav&+Rag&+rely &+Lstrikes back!&n", TRUE, ch, obj, vict, TO_CHAR);
    act("$n's $q &+mflares &+rbril&+Rli&+rantly &+Las it intercepts &nyour &+Lattack and &+rsav&+Rag&+rely &+Lstrikes back!&n", TRUE, ch, obj, vict, TO_VICT);
    act("$n's $q &+mflares &+rbril&+Rli&+rantly &+Las it intercepts &n$N's &+Lattack and &+rsav&+Rag&+rely &+Lstrikes back!&n", TRUE, ch, obj, vict, TO_NOTVICT);

    switch( number(0, 2) )
    {
    case 0:
      spell_curse(55, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      break;
    case 1:
      spell_malison(55, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      break;
    default:
      break;
    }

    return TRUE;
  }

  return FALSE;
}


int elemental_wand(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char    vict = NULL;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( !IS_FIGHTING(ch) )
  {
    if( arg && (cmd == CMD_RUB) )
    {
      if (isname(arg, "wand"))
      {

        act("&+WYou rub $q and it quickly begins to transform in your hands.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$n rubs $q and it begins to transform in $n hands.&n", TRUE, ch, obj, vict, TO_ROOM);

        extract_obj(obj, TRUE);
        obj_to_char(read_object(number(97924, 97929), VIRTUAL), ch);
        return TRUE;
      }
    }
  }
  return FALSE;
}


int nuke_damnation(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict, target;
  int room;
  int curr_time;
  struct proc_data *data;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }
  // 1/20 chance.
  if( cmd == CMD_GOTNUKED && !number(0, 19) )
  {
    if( !(data = (struct proc_data *) arg) )
    {
      return FALSE;
    }
    vict = data->victim;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }

    act("&+LYour $q &+mflares &+rbril&+Rli&+rantly &+Las &n$N's &+Lspell is tranformed and deflected!&n", TRUE, ch, obj, vict, TO_CHAR);
    act("$n's $q &+mflares &+rbril&+Rli&+rantly &+Las your spell is tranformed and deflected back at you!&n", TRUE, ch, obj, vict, TO_VICT);
    act("$n's $q &+mflares &+rbril&+Rli&+rantly &+Las &n$N's &+Lspell is tranformed and deflected!&n", TRUE, ch, obj, vict, TO_NOTVICT);

    switch( number(0, 1) )
    {
    case 0:
      spell_curse(55, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      break;
    case 1:
      spell_malison(55, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      break;
    default:
      break;
    }
    return TRUE;
  }
  return FALSE;
}

int collar_frost(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time;
  P_char   icemental = NULL;
  int      i, j, sum, elesize, chance;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !OBJ_WORN(obj) || !IS_ALIVE(ch) || IS_SET(world[ch->in_room].room_flags, LOCKER) )
  {
    return FALSE;
  }

  int level = GET_LEVEL(ch);

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "frost") )
    {
      if( IS_FIGHTING(ch) )
      {
        send_to_char("You are too focused on fighting to complete the incantations!\n", ch);
        return FALSE;
      }
      if( !can_conjure_lesser_elem(ch, level) )
      {
        send_to_char("You have too many followers already.\n\r", ch);
 	      return FALSE;
  	  }

      curr_time = time(NULL);
      // 10 min timer.
      if (obj->timer[0] + 600 <= curr_time)
      {
        //raise to 100 if you want spec pets to occur
        elesize = number(1, 100);
        if( can_conjure_greater_elem(ch, GET_LEVEL(ch)) )
        {
          if( elesize > 94 )
          {
            act("You whisper '&+bFr&+Co&+bst&n' to your $q...", FALSE, ch, obj, 0, TO_CHAR);
            act("&+cEnormous crystals of &+Cice &+cstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_CHAR);

            act("$n whispers '&+bFr&+Co&+bst&n' to $s $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
            act("&+cEnormous crystals of &+Cice &+cstart forming and condensing into the form of an elemental.", FALSE, ch, obj, obj, TO_ROOM);

            icemental = read_mobile(1160, VIRTUAL);
          }
          else if( elesize > 89 )
          {
            act("You whisper '&+bFr&+Co&+bst&n' to your $q...", FALSE, ch, obj, 0, TO_CHAR);
            act("&+cEnormous crystals of &+Cice &+cstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_CHAR);

            act("$n whispers '&+bFr&+Co&+bst&n' to $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
            act("&+cEnormous crystals of &+Cice &+cstart forming and condensing into the form of an elemental.", FALSE, ch, obj, obj, TO_ROOM);

            icemental = read_mobile(1159, VIRTUAL);
          }
          else if( elesize > 70 )
          {
            act("You whisper '&+bFr&+Co&+bst&n' to your $q...", FALSE, ch, obj, 0, TO_CHAR);
            act("&+cLarge crystals of &+Cice &+cstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_CHAR);

            act("$n whispers '&+bFr&+Co&+bst&n' to $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
            act("&+cLarge crystals of &+Cice &+cstart forming and condensing into the form of an elemental.", FALSE, ch, obj, obj, TO_ROOM);

            icemental = read_mobile(1158, VIRTUAL);
          }
        }
        if (elesize <= 70)
        {
          act("You whisper '&+bFr&+Co&+bst&n' to your $q...", FALSE, ch, obj, 0, TO_CHAR);
          act("&+cSmall crystals of &+Cice &+cstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_CHAR);

          act("$n whispers '&+bFr&+Co&+bst&n' to $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
          act("&+cSmall crystals of &+Cice &+cstart forming and condensing into the form of an elemental.", FALSE, ch, obj, obj, TO_ROOM);

          icemental = read_mobile(1157, VIRTUAL);
        }
        if( !(icemental) || icemental == NULL || ch->in_room == NOWHERE )
        {
          act("&=LBTHERE IS NO ICE ELEMENTAL, TELL A GOD!!&N", FALSE, ch, obj, obj, TO_CHAR);
          debug("collar_frost: missing mental (%s) or room (%d).", icemental ? J_NAME(icemental) : "NULL", ch->in_room );
          return FALSE;
        }
        if( IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) )
        {
          send_to_char("&+cThe elemental failed to arrive. This area is too narrow.\r\n", ch);
          extract_char( icemental );
          return FALSE;
        }
        char_to_room(icemental, ch->in_room, 0);
        CharWait(ch,PULSE_VIOLENCE * 2);

        act("&nYou feel slightly drained as your $q channels magical energy.&n", FALSE, ch, obj, obj, TO_CHAR);

        GET_SIZE(icemental) = SIZE_MEDIUM;
        icemental->player.m_class = CLASS_WARRIOR;
        justice_witness(ch, NULL, CRIME_SUMMON);
        icemental->player.level = 45;
        sum = dice(GET_LEVEL(icemental) * 4, 8) + (GET_LEVEL(icemental) * 3);
        while( icemental->affected )
        {
          affect_remove(icemental, icemental->affected);
        }
        if( !IS_SET(icemental->specials.act, ACT_MEMORY) )
        {
          clearMemory(icemental);
        }
        SET_BIT(icemental->specials.affected_by, AFF_INFRAVISION);
        remove_plushit_bits(icemental);
        GET_MAX_HIT(icemental) = GET_HIT(icemental) = icemental->points.base_hit = sum;
        icemental->points.base_hitroll = icemental->points.hitroll = GET_LEVEL(icemental) / 3;
        icemental->points.base_damroll = icemental->points.damroll = GET_LEVEL(icemental) / 3;
        MonkSetSpecialDie(icemental);
        GET_EXP(icemental) = 0;
        balance_affects(icemental);
        setup_pet(icemental, ch, 1500, PET_NOCASH);
        add_follower(icemental, ch);
        obj->timer[0] = curr_time;
        return TRUE;
      }
      else
      {
        act("&+cOnly a slight &+Cchill &+cfills the air.&n", FALSE, ch, obj, obj, TO_CHAR);
        act("&+cOnly a slight &+Cchill &+cfills the air.&n", FALSE, ch, obj, obj, TO_ROOM);

        act("&nYou feel slightly drained as your $q fails to channel magical energy.&n", FALSE, ch, obj, obj, TO_CHAR);
        CharWait(ch,PULSE_VIOLENCE * 2);
      }
    }
  }
  return FALSE;
}

int collar_flames(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time;
  P_char   firemental = NULL;
  int      i, j, sum, elesize, chance;


  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !OBJ_WORN(obj) || !IS_ALIVE(ch) || IS_NPC(ch) || IS_SET(world[ch->in_room].room_flags, LOCKER) )
  {
    return FALSE;
  }

  int level = GET_LEVEL(ch);

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "flames") )
    {
      if( IS_FIGHTING(ch) )
      {
        send_to_char("You are too focused on fighting to complete the incantations!\r\n", ch);
        return TRUE;
      }
      if(!can_conjure_lesser_elem(ch, level))
  	  {
  	    send_to_char("You have too many followers already.\n\r", ch);
   	    return FALSE;
 	    }
      curr_time = time(NULL);
      // 10 min timer.
      if (obj->timer[0] + 600 <= curr_time)
      {
        // Raise to 100 if you want spec pets to occur
        elesize = number(1, 100);
        if( can_conjure_greater_elem(ch, GET_LEVEL(ch)) )
        {
          if( elesize > 98 )
          {
            act("You whisper '&+rFl&+Ram&+res&n' to your $q...&n", FALSE, ch, obj, 0, TO_CHAR);
            act("&+rEnormous pilars of &+Rflames &+rstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_CHAR);
            act("$n whispers '&+rFl&+Ram&+res&n' to $s $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
            act("&+cEnormous crystals of &+Cice &+cstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_ROOM);

            firemental = read_mobile(1111, VIRTUAL);
          }
          else if( elesize > 94 )
          {
            act("You whisper '&+rFl&+Ram&+res&n' to your $q...&n", FALSE, ch, obj, 0, TO_CHAR);
            act("&+rEnormous pilars of &+Rflames &+rstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_CHAR);
            act("$n whispers '&+rFl&+Ram&+res&n' to $s $q...&n", TRUE, ch, obj, NULL, TO_ROOM);
            act("&+cEnormous crystals of &+Cice &+cstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_ROOM);

            firemental = read_mobile(1112, VIRTUAL);
          }
          else if( elesize > 89 )
          {
            act("You whisper '&+rFl&+Ram&+res&n' to your $q...&n", FALSE, ch, obj, 0, TO_CHAR);
            act("&+rLarge pilars of &+Rflames &+rstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_CHAR);
            act("$n whispers '&+rFl&+Ram&+res&n' to $s $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
            act("&+cLarge crystals of &+Cice &+cstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_ROOM);

            firemental = read_mobile(1110, VIRTUAL);
          }
        }
        if (elesize <= 89)
        {
          act("You whisper '&+rFl&+Ram&+res&n' to your $q...&n", FALSE, ch, obj, 0, TO_CHAR);
          act("&+rSmall pilars of &+Rflames &+rstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_CHAR);
          act("$n whispers '&+rFl&+Ram&+res&n' to $s $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
          act("&+cSmall crystals of &+Cice &+cstart forming and condensing into the form of an elemental.&n", FALSE, ch, obj, obj, TO_ROOM);

          firemental = read_mobile(1100, VIRTUAL);
        }

        if( !firemental || firemental == NULL || ch->in_room == NOWHERE )
        {
          act("&+LTHERE IS NO FIRE ELEMENTAL, TELL A GOD!!&N", FALSE, ch, obj, obj, TO_CHAR);
          debug("collar_frost: missing mental (%s) or room (%d).", firemental ? J_NAME(firemental) : "NULL", ch->in_room );
          return FALSE;
        }

        if(IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
        {
          send_to_char("&+RThe elemental failed to arrive. This area is too narrow.\r\n", ch);
          return FALSE;
        }

        char_to_room(firemental, ch->in_room, 0);
        CharWait(ch, PULSE_VIOLENCE * 2);
        act("&nYou feel slightly drained as your $q channels magical energy.&n", FALSE, ch, obj, obj, TO_CHAR);
        GET_SIZE(firemental) = SIZE_MEDIUM;
        firemental->player.m_class = CLASS_WARRIOR;
        justice_witness(ch, NULL, CRIME_SUMMON);
        firemental->player.level = 45 + number(-5, 0);
        sum = dice(GET_LEVEL(firemental) * 4, 8) + (GET_LEVEL(firemental) * 3);
        while( firemental->affected )
        {
          affect_remove(firemental, firemental->affected);
        }
        if (!IS_SET(firemental->specials.act, ACT_MEMORY))
        {
          clearMemory(firemental);
        }
        SET_BIT(firemental->specials.affected_by, AFF_INFRAVISION);
        remove_plushit_bits(firemental);
        GET_MAX_HIT(firemental) = GET_HIT(firemental) = firemental->points.base_hit = sum;
        firemental->points.base_hitroll = firemental->points.hitroll = GET_LEVEL(firemental) / 3;
        firemental->points.base_damroll = firemental->points.damroll = GET_LEVEL(firemental) / 3;
        MonkSetSpecialDie(firemental);
        GET_EXP(firemental) = 0;
        balance_affects(firemental);
        setup_pet(firemental, ch, 1500, PET_NOCASH);
        add_follower(firemental, ch);
        obj->timer[0] = curr_time;
        return TRUE;
      }
      else
      {
        act("&+rOnly a small gust of &+Rhot &+rair fills the room.&n", FALSE, ch, obj, obj, TO_CHAR);
        act("&+rOnly a small gust of &+Rhot &+rair fills the room.&n", FALSE, ch, obj, obj, TO_ROOM);
        act("&nYou feel slightly drained as your $q fails to channel magical energy.&n", FALSE, ch, obj, obj, TO_CHAR);
        CharWait(ch, PULSE_VIOLENCE * 2);
      }
    }
  }
  return FALSE;
}

/*

int key_mold(P_obj obj, P_char ch, int cmd, char *args)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch || cmd)
    return FALSE;

  if (!obj->value[0]--)
  {
    P_obj    corpse;

    corpse = read_object(VOBJ_WH_ROTTING_CORPSE, VIRTUAL);
    if (!corpse)
    {
      logit(LOG_EXIT, "wh_corpse_decay: unable to load obj #%d.", VOBJ_WH_ROTTING_CORPSE);
      raise(SIGSEGV);
    }
    corpse->weight = obj->weight;
    set_obj_affected(corpse, get_property("timer.decay.corpse.npc", 120), TAG_OBJ_DECAY, 0);

    if (OBJ_CARRIED(obj))
    {
      P_char   carrier;

      carrier = obj->loc.carrying;
      send_to_char("Something smells real bad...\r\n", carrier);
      obj_to_char(corpse, carrier);

    }
    else if (OBJ_ROOM(obj))
    {
      send_to_room("Something smells real bad...\r\n", obj->loc.room);
      obj_to_room(corpse, obj->loc.room);

    }
    else if (OBJ_INSIDE(obj))
    {
      obj_to_obj(corpse, obj->loc.inside);
    }
    else
    {
      extract_obj(corpse, TRUE);
    }
    extract_obj(obj, TRUE);
    return TRUE;
  }
  return FALSE;
}

*/

int tiamat_human_to_rareloads(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_DEATH)
  {
    P_obj    obj;

    switch(number(0,3))
    {
      case 0:
        obj = read_object(19911, VIRTUAL);
        break;
      case 1:
        obj = read_object(19916, VIRTUAL);
        break;
      case 2:
        obj = read_object(19637, VIRTUAL);
        break;
      case 3:
        obj = read_object(19638, VIRTUAL);
        break;
      default:
        break;
    }

    if (!(obj))
    {
      logit(LOG_EXIT, "winterhaven_object: death object for mob %d doesn't exist",
            GET_VNUM(ch));
      raise(SIGSEGV);
    }

    obj_to_room(obj, ch->in_room);

    act("&nAs &+La pitiful slave &nfalls to the ground, his corpse quickly disolves, leaving only a few valuable artifacts...&n", FALSE, ch, obj, obj, TO_ROOM);

    obj->value[0] = SECS_PER_MUD_DAY / PULSE_MOBILE * WAIT_SEC;

    return FALSE;
  }
  return FALSE;
}

int dragonnia_heart(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    obj;

  if( cmd == CMD_DEATH )
  {
    if( !(obj = read_object(VOBJ_WH_DRAGONHEART_DRAGONNIA, VIRTUAL)) )
    {
      logit(LOG_EXIT, "dragonnia_heart: could not load heart vnum %d", VOBJ_WH_DRAGONHEART_DRAGONNIA );
      raise(SIGSEGV);
    }

    obj_to_room(obj, ch->in_room);

    act("&+WWith her very last breath, &+GDragonnia &+Wcloses her eyes.&n", FALSE, ch, obj, obj, TO_ROOM);
    act("&+GDragonnia&+W's body begins to shimmer brilliantly, her flesh becoming more and more translucent!&n", FALSE, ch, obj, obj, TO_ROOM);
    act("&+WAs the light subsides, only a few pieces of her once great body remain.&n", FALSE, ch, obj, obj, TO_ROOM);

    // 3 mud days to complete (object ticks are the same as mob ticks.. *sigh*).
    // (Sec / MudDay) * (Pulse / Sec) * (ObjPulse / Pulse) = ObjPulse / MudWeek
    obj->value[0] = (3 * SECS_PER_MUD_DAY * WAIT_SEC) / PULSE_MOBILE;
    // Time in minutes = ObjPulse * (Pulse / ObjPulse) * (Sec / Pulse) = Secs
    obj->value[1] = ((obj->value[0] * PULSE_MOBILE) / WAIT_SEC);
    // Secs / 3600 = Hrs
    obj->value[2] =  obj->value[1] / 3600;
    // (Secs / 60) % 60 = Remainder of Mins
    obj->value[3] = (obj->value[1] / 60) % 60;
    // Secs % 60 = Remainder of Secs.
    obj->value[4] = (obj->value[1]) % 60;
    debug("&+GDragonnia death: Heart decays in &+C%d&+G obj ticks = &+C%d&+G sec = &+C%d&+G:&+C%02d&+G:&+C%02d&+G.&n",
      obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4]);
    logit(LOG_OBJ, "Dragonnia death: Heart decays in %d obj ticks = %d sec = %d:%02d:%02d.",
      obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4]);
    // Value1 is break chance haha.. need to 0 that out.
    obj->value[1] = 0;

    return FALSE;
  }
  return FALSE;
}


int dragon_heart_decay(P_obj obj, P_char ch, int cmd, char *args)
{
  int    loc;
  P_char carrier;
  P_obj  decayed_heart;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;

  if( cmd == CMD_EXAMINE )
  {
    generic_find(args, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM,
      ch, &carrier, &decayed_heart);

    if( obj == decayed_heart )
    {
      // obj->value[0] == ObjTicks to decay.
      // ObjTick * (MudTick / ObjTick) / ((MudTick / Sec) * (Sec / MudDay)) == MudDays left (floored to int).
      switch( (obj->value[0] * PULSE_MOBILE) / (WAIT_SEC * SECS_PER_MUD_DAY)  )
      {
        case 2:
          send_to_char( "&+WYou still have over &+B2&+W days before this &+rheart&+W goes bad.&n\n", ch );
          break;
        case 1:
          send_to_char( "&+WYou have over &+B1&+W day before this &+rheart&+W goes bad.&n\n", ch );
          break;
        case 0:
          send_to_char( "&+WYou have less than &+B1&+W day before this &+rheart&+W goes bad.  Hurry!&n\n", ch );
          break;
        default:
          send_to_char_f( ch, "&+WIt seems this &+rheart&+W will last for &+B%d&+W days, no worries.&n\n",
            (obj->value[0] * PULSE_MOBILE) / (WAIT_SEC * SECS_PER_MUD_DAY) );
          break;
      }
      return TRUE;
    }
    return FALSE;
  }

  if( cmd != CMD_PERIODIC )
    return FALSE;

  if( !obj->value[0]-- )
  {

    if( !(decayed_heart = read_object(VOBJ_WH_DRAGONHEART_ROTTED, VIRTUAL)) )
    {
      logit(LOG_EXIT, "wh_corpse_decay: unable to load decayed heart #%d.", VOBJ_WH_DRAGONHEART_ROTTED);
      raise(SIGSEGV);
    }
    decayed_heart->weight = obj->weight;
    set_obj_affected(decayed_heart, 15000, TAG_OBJ_DECAY, 0);

    if( OBJ_CARRIED(obj) )
    {
      carrier = obj->loc.carrying;
      send_to_char("Something starts to smell really bad...\r\n", carrier);
      obj_to_char(decayed_heart, carrier);
    }
    else if( OBJ_WORN(obj) )
    {
      carrier = obj->loc.wearing;
      send_to_char("Something starts to smell really bad...\r\n", carrier);
      // This _should_ only be HOLD = 18.
      for( loc = 0; loc < MAX_WEAR; loc++ )
      {
        if( carrier->equipment[loc] == obj )
        {
          break;
        }
      }
      if( loc == MAX_WEAR )
      {
        debug( "Dragon heart '%s' %d supposedly equipped but not!", OBJ_SHORT(obj), OBJ_VNUM(obj) );
        logit( LOG_OBJ, "Dragon heart '%s' %d supposedly equipped but not!", OBJ_SHORT(obj), OBJ_VNUM(obj) );
        obj_to_char(decayed_heart, carrier);
      }
      else
      {
        if( loc != HOLD )
        {
          debug( "Dragon heart '%s' %d equipped in non-hold slot %d!", OBJ_SHORT(obj), OBJ_VNUM(obj), loc );
          logit( LOG_OBJ, "Dragon heart '%s' %d equipped in non-hold slot %d!", OBJ_SHORT(obj), OBJ_VNUM(obj), loc );
        }
        // Remove the heart.
        unequip_char( carrier, loc );
        // And put decayed heart in its place.
        equip_char( carrier, decayed_heart, loc, TRUE);
      }
    }
    else if( OBJ_ROOM(obj) )
    {
      send_to_room("Something starts to smell really bad...\r\n", obj->loc.room);
      obj_to_room(decayed_heart, obj->loc.room);
    }
    else if( OBJ_INSIDE(obj) )
    {
      obj_to_obj(decayed_heart, obj->loc.inside);
    }
    else
    {
      extract_obj(decayed_heart);
    }
    switch( OBJ_VNUM(obj) )
    {
      case VOBJ_WH_DRAGONHEART_TIAMAT:
        debug( "dragon_heart_decay: &+LTiamat's heart decayed.&n" );
        logit( LOG_OBJ, "dragon_heart_decay: Tiamat's heart decayed." );
        break;
      case VOBJ_WH_DRAGONHEART_DRAGONNIA:
        debug( "dragon_heart_decay: &+GDragonnia's heart decayed.&n" );
        logit( LOG_OBJ, "dragon_heart_decay: Dragonnia's heart decayed." );
        break;
      case VOBJ_WH_DRAGONHEART_BAHAMUT:
        debug( "dragon_heart_decay: &+WBahamut's heart decayed.&n" );
        logit( LOG_OBJ, "dragon_heart_decay: Bahamut's heart decayed." );
        break;
      default:
        debug( "dragon_heart_decay: Unknown item decaying - '%s' %d?!?", OBJ_SHORT(obj), OBJ_VNUM(obj) );
        break;
    }

    extract_obj(obj, TRUE); // Not an arti, but 'in game.'
    return TRUE;
  }
  return FALSE;
}

int lanella_heart(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_DEATH)
  {
    P_obj    obj;

    switch(number(0,1))
    {
      case 0:
        obj = read_object(28998, VIRTUAL);
        break;
      case 1:
        obj = read_object(28997, VIRTUAL);
        break;
    }

    if (!(obj))
    {
      logit(LOG_EXIT, "winterhaven_object: death object for mob %d doesn't exist",
            GET_VNUM(ch));
      raise(SIGSEGV);
    }

    obj_to_room(obj, ch->in_room);

    act("&nAs &+MLanella &nfalls to the ground, her body begins to decay.&n", FALSE, ch, obj, obj, TO_ROOM);
    act("&nHer body crumbles to dust and blows away.&n", FALSE, ch, obj, obj, TO_ROOM);

    obj->value[0] = SECS_PER_MUD_DAY / PULSE_MOBILE * WAIT_SEC;

    return FALSE;
  }
  return FALSE;
}

int weapon_vampire(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  char     e_pos;
  int      in_battle;

  vict = (P_char) arg;
  in_battle = cmd / 1000;

  if (cmd == CMD_SET_PERIODIC)               /*
                                   Events have priority 
                                 */
    return FALSE;

  if (!ch || !obj)              /*
                                   If the player ain't here, why are we? 
                                 */
    return FALSE;

  if (!OBJ_WORN(obj))           /*
                                   Most things don't work in a sack... 
                                 */
    return FALSE;

/*
   If it must be wielded, use this 
 */
  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);
  if (!e_pos)
    return FALSE;

  if (!in_battle)               /*
                                   Past here, and you're fighting 
                                 */
    return FALSE;

  in_battle = BOUNDED(0, (GET_HIT(vict) + 9), number(1, 8));

  if ((obj->loc.wearing == ch) && vict)
  {
    // act("&+rBlood is around the corner", FALSE, ch, 0, 0, TO_CHAR); 

    if (GET_MAX_HIT(ch) - GET_HIT(ch) >= 0)
    {
      GET_HIT(ch) += in_battle;
      //GET_HIT(vict) -= in_battle;
    }
  }
  return FALSE;

}

int lancer_gift(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict = NULL;
  P_obj  gift;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( !IS_FIGHTING(ch) )
  {
    if( arg && (cmd == CMD_OPEN) )
    {
      if( isname(arg, "gift") )
      {

        act("&+WYou open &n$q &+Wand the wrappings fall to the floor.&n", TRUE, ch, obj, vict, TO_CHAR);
        act("$n opens &n$q &+Wand the wrappings fall to the floor.&n", TRUE, ch, obj, vict, TO_ROOM);

        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
        gift = read_object(number(55198, 55203), VIRTUAL);
        obj_to_char(gift, ch);
        act("You receive $q.", TRUE, ch, obj, vict, TO_CHAR);
        gift = read_object(number(55198, 55203), VIRTUAL);
        obj_to_char(gift, ch);
        act("You receive $q.", TRUE, ch, obj, vict, TO_CHAR);
        return TRUE;
      }
    }
  }
  return FALSE;
}

int cerberus_load(P_char ch, P_char pl, int cmd, char *arg)
{
  if( cmd == CMD_DEATH )
  {
    P_obj    obj;

    // 50% chance to drop one of below (50%*1/8 == 1/16 chance each).
    switch(number(0,15))
    {
      case 0:
        obj = read_object(22031, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 1:
        obj = read_object(22049, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 2:
        obj = read_object(22036, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 3:
        obj = read_object(22053, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 4:
        obj = read_object(22050, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 5:
        obj = read_object(22056, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 6:
        obj = read_object(22035, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        obj = read_object(22035, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        obj = read_object(22035, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        obj = read_object(22035, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 7:
        obj = read_object(22037, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
    }

    // 100% chance to drop 1 of below (1/8 chance each).
    switch(number(8,15))
    {
      case 8:
        obj = read_object(22063, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 9:
        obj = read_object(22054, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 10:
        obj = read_object(22057, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 11:
        obj = read_object(22066, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 12:
        obj = read_object(22058, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 13:
        obj = read_object(22051, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 14:
        obj = read_object(22032, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
      case 15:
        obj = read_object(22065, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        obj = read_object(22065, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        obj = read_object(22064, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        obj = read_object(22064, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        obj = read_object(22035, VIRTUAL);
        obj_to_room(obj, ch->in_room);
        break;
    }

    act("&+yA HUGE rockworm gasps and begins to turn to dust.&n", FALSE, ch, NULL, NULL, TO_ROOM);
    extract_char(ch);

    return TRUE;
  }
  return FALSE;
}

