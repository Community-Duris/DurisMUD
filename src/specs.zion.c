      /* Made by Zion, to house all of his zany procs in one special location of pure love */
      /* Hijacked by Jexni with love */
#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <list>
#include <vector>
using namespace std;

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "assocs.h"
#include "graph.h"
#include "damage.h"
#include "reavers.h"
#include "specs.zion.h"

extern const char *dirs[];
extern P_index obj_index;
extern Skill skills[];
extern char *spells[];
extern bool has_skin_spell(P_char);
extern P_room world;
bool exit_wallable(int room, int dir, P_char ch);

int tharnrifts_portal(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if (!obj || !ch)
    return FALSE;

  if (cmd == CMD_PERIODIC && !number(0, 15))
  {
    act("&+LThe shadowrift &+rpulsates &+Rwildly &+La moment, causing the &+Rmolten &+rrocks &+Lin the area to levitate briefly.&n", FALSE, ch, obj, 0, TO_ROOM);
    return FALSE;
  }

  if (cmd == CMD_ENTER && arg)
  {
    if (isname(arg, "portal") || isname(arg, "shadowrift"))
    {
      if(IS_RACEWAR_GOOD(ch))
        obj->value[0] = 5569;
      else
        obj->value[0] = 5583;
    }
  }
  return FALSE;
}

int Baltazo(P_char ch, P_char victim, int cmd, char *arg)
{
  int j, i, devil2load;

  if(cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(cmd == CMD_PERIODIC && !number(0, 5))
  {
     act("$n&+g's skin bubbles and &+Yemits &+Ggreenish&+g-&+Yyellow &+yooze&+g.&n", FALSE, ch, 0, 0, TO_ROOM);
     return FALSE;
  }

   if(arg && (IS_AGG_CMD(cmd)))
   {
     if(isname(arg, "emaciated") || isname(arg, "decorated") || isname(arg, "general") || isname(arg, "sickly"))
     {
       send_to_char("Something tells you that would end badly for you...\n", ch);
       return TRUE;
     }
   }

 /* if(cmd == CMD_DEATH)
  {
     act("$n &+Lraises $s arms and $s &+reyes &+Rflash &+Lwith an inner &+Rfire&+L.", FALSE, ch, 0, 0, TO_ROOM);
     act("&+LA deep voice seems to echo from within $m, '&+GFool mortal trash.  Your insolence&n", FALSE, ch, 0, 0, TO_ROOM);
     act("&+Gwill be repaid one thousand times over.  &+RHollow Heart &+Gdoes not forgive...'&n", FALSE, ch, 0, 0, TO_ROOM);
     act("&+YThe image of the man is replaced by a gaping &+Lshadowrift&+Y, which pulses rapidly...&n", FALSE, ch, 0, 0, TO_ROOM);
     for(j = 0;j < 11;j++)
     {
        i = 90702;
        if(!number(0, 1))
        {
           i += number(0, 4);
        }
        devil2load = BOUNDED(90702, i, 90706);
        P_char devil = read_mobile(devil2load, VIRTUAL);
        if (!devil)
        {
          logit(LOG_DEBUG, "Baltazo failed to load a mob, specs.zion.c");
          continue;
        }
        SET_BIT(devil->specials.act, ACT_SENTINEL);
        if(affected_by_spell(devil, AFF_INVISIBLE))
          affect_from_char(devil, AFF_INVISIBLE);
        char_to_room(devil, ch->in_room, -1);
        act("$n &+rsteps through the &+Lshadowrift&+r, and does not look pleased...&n", FALSE, devil, 0, 0, TO_ROOM);

        P_char tch, temp;
        for(tch = world[real_room0(devil->in_room)].people; tch; tch = temp)
        {
           temp = tch->next_in_room;
           wizlog(56, "tch is %s", GET_NAME(tch));
           if(IS_TRUSTED(tch))
             continue;
           if(IS_PC(tch))// && number(0, 2))
           {
             MobStartFight(tch, devil);
             break;
           }
        }
     }
     char_to_room(ch, 500, -2);
     return FALSE;
  }*/

  return FALSE;
}  

int zion_shield_absorb_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict = NULL;
  struct proc_data *data = NULL;
  struct affected_type af;
  int percent = number(1, 10);

  if(cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  // 1/10-1/100 chance?
  if( (cmd == CMD_GOTNUKED) && percent > number(1,100) )
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

    act("&+L$N&+L's $q &+Labsorbs $n&+L's spell!", FALSE, vict, obj, ch, TO_NOTVICT);
    act("&+LYour spell is absorbed by $N&+L's $q&+L!", FALSE, vict, obj, ch, TO_CHAR);
    act("&+L$n&+L's spell is absorbed by your $q&+L!", FALSE, vict, obj, ch, TO_VICT);

    return TRUE;
  }

  // 1/20 chance.
  if( (cmd == CMD_GOTNUKED) && !number(0, 19) && !affected_by_spell(ch, SPELL_BLINDNESS) )
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

    act("&+L$N&+L's $q &+Lbegins to crackle around the edges, as it siphons the magic of $n&+L's spell!", FALSE, vict, obj, ch, TO_NOTVICT);
    act("&+LYour $q &+Lbegins to crackle around the edges, as it siphons the magic of $n&+L's spell!", FALSE, vict, obj, ch, TO_VICT);
    act("&+L$N&+L's $q &+Lbegins to crackle around the edges, as it siphons the magic of your&+L spell!", FALSE, vict, obj, ch, TO_CHAR);


    act("&+L$n's $q &+Llashes out with the stored magical energies, blasting $N &+Lwith pure eldritch power!&n", FALSE, ch, obj, vict, TO_NOTVICT);
    act("&+LYour $q &+Llashes out with the stored magical energies, blasting $N &+Lwith pure eldritch power!&n", FALSE, ch, obj, vict, TO_CHAR);
    act("&+L$n's $q &+Llashes out with the stored magical energies, blasting you &+Lwith pure eldritch power!&n", FALSE, ch, obj, vict, TO_VICT);

    blind(vict, ch, 3 * PULSE_VIOLENCE);
    damage(ch, vict, (80 + number(0, 40)), TYPE_UNDEFINED);

    act("&+rThe swift drain of magical energies from $q &+Lcauses $n &+Lto look queasy!&n", FALSE, ch, obj, 0, TO_ROOM);
    act("&+LUgh, maybe that wasn't so great after all...&n", FALSE, ch, obj, 0, TO_CHAR);

    return TRUE;
  }
  return FALSE;
}

int generic_shield_block_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || cmd != CMD_GOTHIT )
  {
    return FALSE;
  }

  // 1/5 chance.
  if( !number(0, 4) )
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

    act("Your $q blocks $N's vicious attack.", FALSE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);
    act("$n's $q blocks your futile attack.", FALSE, ch, obj, vict, TO_VICT | ACT_NOTTERSE);
    act("$n's $q blocks $N's attack.", FALSE, ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);

    return TRUE;
  }

  return FALSE;
}

int zion_fnf(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict;
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

  if ( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( (cmd == CMD_REMOVE) && arg )
  {
    if( isname(arg, obj->name) || isname(arg, "all") )
    {
      if( affected_by_spell(ch, SPELL_FIRE_AURA) )
      {
        affect_from_char(ch, SPELL_FIRE_AURA);
      }
    }
   return FALSE;
  }

  if( !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_SAY && arg )
  {
    if( isname(arg, "fire") )
    {
      curr_time = time(NULL);
      // 1 min timer.
      if( obj->timer[0] + 60 <= curr_time )
      {
        act("You say 'fire'", FALSE, ch, 0, 0, TO_CHAR);
        act("&+rYour $q &+rengulfs the room in &+Wwhite-&+Rhot &+Yfl&+ram&+res!", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'fire'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q &+rengulfs the room in &+Wwhite-&+Rh&+ro&+Rt &+Yfl&+yam&+res!", TRUE, ch, obj, NULL, TO_ROOM);
        cast_transmute_rock_lava(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
    else if( isname(arg, "flame") )
    {
      curr_time = time(NULL);
      // 3 min 20 sec timer.
      if( obj->timer[1] + 200 <= curr_time )
      {
        act("You say 'flame'", FALSE, ch, 0, 0, TO_CHAR);
        act("&+rYour $q &+rinfuses you with the power of &+RFIRE&+r!&n", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'flame'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q &+rinfuses them with the power of &+RFIRE&+r!&n", TRUE, ch, obj, NULL, TO_ROOM);
        spell_fire_aura(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[1] = curr_time;

        return TRUE;
      }
    }
  }
  // 1/25 chance.
  if( (cmd == CMD_MELEE_HIT) && !number(0, 24) && CheckMultiProcTiming(ch) )
  {
    vict = (P_char) arg;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }

    act("Your $q &+rsummons the power of fire and flames at at $N.", FALSE, ch, obj, vict, TO_CHAR);
    act("$n's $q &+rsummons the power of fire and flames at you.", FALSE, ch, obj, vict, TO_VICT);
    act("$n's $q &+rsummons the power of fire and flames at $N.", FALSE, ch, obj, vict, TO_NOTVICT);

    switch( number(0, 2) )
    {
    case 0:
      spell_incendiary_cloud(50, ch, 0, 0, vict, 0);
      break;
    case 1:
      spell_firestorm(50, ch, 0, 0, vict, 0);
      break;
    case 2:
      spell_scathing_wind(50, ch, 0, 0, vict, 0);
      break;
    default:
      break;
    }

    if( !IS_ALIVE(vict) || !IS_ALIVE(ch) )
    {
      return FALSE;
    }

    act("&+RYour $q &+Rcalls upon the pure power of the plane of fire, directing it at $N!&n", FALSE, ch, obj, vict, TO_CHAR);
    act("$n's $q &+Rcalls upon the pure power of the plane of fire, directing it at you!&n", FALSE, ch, obj, vict, TO_VICT);
    act("$n's $q &+Rcalls upon the pure power of the plane of fire, directing it at $N!&n", FALSE, ch, obj, vict, TO_NOTVICT);

    switch( number(0, 6) )
    {
    case 0:
      spell_burning_hands(50, ch, 0, 0, vict, 0);
      break;
    case 1:
      spell_fireball(50, ch, 0, 0, vict, 0);
      break;
    case 2:
      spell_magma_burst(50, ch, 0, 0, vict, 0);
      break;
    case 3:
      spell_solar_flare(50, ch, 0, 0, vict, 0);
      break;
    case 4:
      spell_immolate(50, ch, 0, 0, vict, 0);
      break;
    case 5:
      spell_flamestrike(50, ch, 0, 0, vict, 0);
      break;
    case 6:
      spell_flameburst(50, ch, 0, 0, vict, 0);
      break;
    default:
      break;
    }
    return TRUE;
  }
  return FALSE;
}

int zion_light_dark(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  int      curr_time;
  P_char   vict;
  struct damage_messages messages = {
    "&+wThe power of your &+WGod&+w rains down pain and suffering upon $N&+w!&n",
    "&+wPain unlike you have ever felt before permeates your body.&n",
    "&+w$N &+wscreams in utter terror as he is judged before $n&+w's &+WGod&+w.&n",
    "&+w$N &+wfalls to the ground, their soul a mere shell of what it once was.&n",
    "&+wJudgement is rendered, as you feel your soul being shattered to pieces.&n",
    "&+w$N &+wfalls to the ground, their soul a mere shell of what it once was.&n.",
      0
  };

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if( (cmd == CMD_SAY) && arg )
  {
    if( isname(arg, "light") && GET_RACEWAR(ch) != RACEWAR_EVIL )
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 60 <= curr_time)
      {
        act("You say 'light'", FALSE, ch, 0, 0, TO_CHAR);
        act("&+WYour $q &+Wflares up, emitting a pure white light!&n", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'light'", TRUE, ch, obj, NULL, TO_ROOM);
        act("&+W$n&+W's $q &+Wflares up, emitting a pure white light!&n", TRUE, ch, obj, NULL, TO_ROOM);

        spell_stone_skin(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_bless(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_armor(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_protection_from_evil(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_continual_light(40, ch, 0, SPELL_TYPE_SPELL,ch, 0);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
    else if( isname(arg, "darkness") && GET_RACEWAR(ch) != RACEWAR_GOOD )
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 60 <= curr_time)
      {
        act("You say 'darkness'", FALSE, ch, 0, 0, TO_CHAR);
        act("&+LYour $q &+Lbecomes pitch black!&n", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'darkness'", TRUE, ch, obj, NULL, TO_ROOM);
        act("&+L$n&+L's $q &+Lgrows pitch black!&n", TRUE, ch, obj, NULL, TO_ROOM);
        spell_shadow_shield(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_lifelust(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_phantom_armor(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_protection_from_good(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_darkness(40, ch, 0, SPELL_TYPE_SPELL,ch, 0);

        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }

  // 1/25 chance.
  if( (cmd == CMD_MELEE_HIT) && !number(0,24) && CheckMultiProcTiming(ch) )
  {
    if( ch->equipment[WIELD] != obj || !dam )
    {
      return FALSE;
    }
    vict = (P_char) arg;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }

    if( GET_RACEWAR(ch) == RACEWAR_GOOD )
    {
      act("$n's $q &+Wflares with pure light, unleashing the virtue of the gods at $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);
      act("Your $q &+Wflares with pure light, unleashing the virtue of the gods at $N!&n", TRUE, ch, obj, vict, TO_CHAR);
      act("$n's $q &+Wflares with pure light, unleashing the virtue of the gods at _YOU_!&n", TRUE, ch, obj, vict, TO_VICT);
      spell_damage(ch, vict, 480, SPLDAM_HOLY, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
      if( GET_C_LUK(ch) > number(0, 500) )
      {
       spell_mending(40, ch, 0, 0, ch, 0);
      }
    }
    else if( GET_RACEWAR(ch) == RACEWAR_EVIL )
    {
      act("$n's $q &+Lflares with darkness, unleashing the wrath of the underworld upon $N!&n", TRUE, ch, obj, vict, TO_NOTVICT);
      act("Your $q &+Lflares with darkness, unleashing the wrath of the underworld upon $N!&n", TRUE, ch, obj, vict, TO_CHAR);
      act("$n's $q &+Lflares with darkness, unleashing the wrath of the underworld upon _YOU_!&n", TRUE, ch, obj, vict, TO_VICT);
      spell_damage(ch, vict, 480, SPLDAM_COLD, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
      if(GET_C_LUK(ch) > number(0, 500))
      {
        spell_mending(40, ch, 0, 0, ch, 0);
      }
    }
    else
    {
      act("$n's $q &+Wglows white&n, unleashing a massive ball of ice towards $N!", TRUE, ch, obj, vict, TO_NOTVICT);
      act("Your $q &+Wglows white&n, unleashing a massive ball of ice towards $N!", TRUE, ch, obj, vict, TO_CHAR);
      act("$n's $q &+Wglows white&n, unleashing a massive ball of ice towards _YOU_!", TRUE, ch, obj, vict, TO_VICT);
      spell_harm(50, ch, NULL, 0, vict, obj);
      if( !IS_ALIVE(vict) || !IS_ALIVE(ch) )
      {
        return FALSE;
      }
      spell_arieks_shattering_iceball(30, ch, NULL, SPELL_TYPE_SPELL, vict, obj);
    }
    return TRUE;
  }
  return FALSE;
}

// ROD OF DISPATOR
// updated Oct08 - Lucrot
// defined in fight.c - this #define should really be in a global .h file somewhere,
// but make sure the definition of proccing_slots[] also uses it
#define NUM_PROCCING_SLOTS 31
#define DISPATOR_VNUM 51401
extern int proccing_slots[];

void do_dispator_remove(P_char ch)
{
  if(!ch)
  {
    logit(LOG_EXIT, "do_dispator_remove called in specs.zion.c without ch");
    raise(SIGSEGV);
  }

  act("&+yDis&n &+Lstrips you of your augmented powers!&n", TRUE, ch, 0, ch, TO_CHAR);
  if(affected_by_spell(ch, SPELL_REGENERATION))
    affect_from_char(ch, SPELL_REGENERATION);
  
  if(affected_by_spell(ch, SPELL_LIFELUST))
    affect_from_char(ch, SPELL_LIFELUST);
  
  if(affected_by_spell(ch, SPELL_FLESH_ARMOR))
    affect_from_char(ch, SPELL_FLESH_ARMOR);
  
  if(affected_by_spell(ch, SPELL_GLOBE))
    affect_from_char(ch, SPELL_GLOBE);
  
  if(affected_by_spell(ch, SPELL_ARMOR))
    affect_from_char(ch, SPELL_ARMOR);
  
  if(affected_by_spell(ch, SPELL_VITALITY))
    affect_from_char(ch, SPELL_VITALITY);
}

void event_zion_dispator(P_char ch, P_char victim, P_obj obj, void *data)
{
  // first check to make sure the item is still on the character
  struct affected_type af;
  bool has_item = false;

  if( !IS_ALIVE(ch) )
  {
    return;
  }
/* Shortened this up a good bit.
  for( int i = 0; i < NUM_PROCCING_SLOTS; i++ )
  {
    P_obj item = ch->equipment[proccing_slots[i]];

    if( item && obj_index[item->R_num].func.obj == zion_dispator )
    {
      has_item = true;
      break;
    }
  }
*/
  if( ch->equipment[WIELD] &&  obj_index[ch->equipment[WIELD]->R_num].func.obj == zion_dispator )
  {
    has_item = TRUE;
  }

  if( has_item )
  {
    if( !number(0, 1) && !IS_FIGHTING(ch) )
    {
      switch(number(0, 5))
      {
        case 0:
          if(!IS_AFFECTED4(ch, AFF4_REGENERATION) &&
            !affected_by_spell(ch, SPELL_ACCEL_HEALING) &&
            !affected_by_skill(ch, SKILL_REGENERATE) &&
            !affected_by_spell(ch, SPELL_REGENERATION))
          {
            send_to_char("&+YThe power of the &+LNine Hells&+y flows forth from the rod, imbuing your body with the power to regenerate.\r\n", ch);
            spell_regeneration(GET_LEVEL(ch), ch, 0, 0, ch, 0);
          }
          break;
        case 1:
          if (!affected_by_spell(ch, SPELL_VITALITY))
          {
            send_to_char("&+YThe power of the &+LNine Hells&+y flows forth from the rod, imbuing your body with dark vitality.\r\n", ch);
            spell_vitality(60, ch, 0, 0, ch, 0);
          }
          break;
        case 2:
          if (!affected_by_spell(ch, SPELL_LIFELUST))
          {
            send_to_char("&+YThe power of the &+LNine Hells&+y flows forth from the rod, imbuing your body with unholy strength.\r\n", ch);
            spell_lifelust(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
          }
          break;
        case 3:
          if (!IS_AFFECTED2(ch, AFF2_GLOBE))
          {
            send_to_char("&+YThe power of the &+LNine Hells&+y flows forth from the rod, imbuing you with protection from spells.\r\n", ch);
            spell_globe(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
          }
          break;
        case 4:
          if (!IS_AFFECTED5(ch, AFF5_FLESH_ARMOR))
          {
            send_to_char("&+YThe power of the &+LNine Hells&+y flows forth from the rod, imbuing you with unholy armor.\r\n", ch);
            spell_flesh_armor(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
          }
          break;
        case 5:
          // Borrowing the Theurgist version of prot. undead.
          if( !affected_by_spell(ch, SPELL_PROTECT_SOUL) )
          {
            send_to_char("&+YThe power of the &+LNine Hells&+y flows forth from the rod, imbuing you with vile defense.\r\n", ch);
            bzero(&af, sizeof(af));
            af.type = SPELL_PROTECT_SOUL;
            af.duration =  5;
            af.modifier = -100;
            af.location = APPLY_AC;
            affect_to_char(ch, &af);
            act("&+LBands of dark armor burrow into your &+rflesh&+L.&n", FALSE, ch, 0, 0, TO_CHAR);
            act("&+L$n&+L screams in agony as dark armor burrows into $s &+rflesh&+L, then $e grins malevolently.&n", FALSE, ch, 0, 0, TO_ROOM);
          }
          break;
        default:
          break;
      }
    }
    add_event(event_zion_dispator, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
  }
  else
  {
    if(!has_item)
    {
      do_dispator_remove(ch);
      return;
    }
  }
}

int zion_dispator(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct affected_type af;
  bool has_item_wielded = false;

  // tell duris to always send the object CMD_PERIODIC
  if(cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && !number(0, 1) )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return (FALSE);
  }

  if( ch->equipment[WIELD] == obj )
  {
     has_item_wielded = TRUE;
  }
  else
  {
    return FALSE;
  }

// we have to check for this on every event because CMD_PERIODIC fires too slowly
// character doesn't have the event scheduled, so fire the event which
// handles the affect and will renew itself
  if( !get_scheduled(ch, event_zion_dispator) && has_item_wielded && arg && (cmd == CMD_SAY) && isname(arg, "dis") )
  {
    add_event(event_zion_dispator, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
    act("&+LThe power of &+yDis &+Lawakens!&n", TRUE, ch, obj, 0, TO_CHAR);
    act("&+y$n&+y's $q &+ycomes alive with the &+rf&+Ri&+rr&+Re&+rs &+yof the &+LNine &+rHells!&n",
      FALSE, ch, obj, 0, TO_ROOM);
    return TRUE;
  }

  if( cmd == CMD_GOTHIT && !number(0, 19) && has_item_wielded )
  {
    if(!IS_AFFECTED4(ch, AFF4_HELLFIRE))
    {
      act("&+yThe power of Dis comes alive in the $q&+y, engulfing you in &+rfl&+Rami&+rng &+Rhellfire&+y!&n", TRUE,
        ch, obj, 0, TO_CHAR);
      act("&+y$n&+y's $q &+ycomes alive with the &+rf&+Ri&+rr&+Re&+rs &+yof the &+LNine Hells&+y, and engulfs $m in raging &+Rhellfire&+y!&n", FALSE,
        ch, obj, 0, TO_ROOM);

      bzero(&af, sizeof(af));
      af.type = SPELL_HELLFIRE;
      af.location = APPLY_NONE;
      af.flags = AFFTYPE_SHORT;
      af.duration = 40;
      af.bitvector4 = AFF4_HELLFIRE;
      affect_to_char(ch, &af);
    }

    // pick a random direction to deflect the incoming blow; if the direction
    // is wallable, cast wall of iron
    int random_dir = number(0, NUM_EXITS-1);

    char buff[128];
    sprintf(buff, "&+y$n's $q &+ydeflects the blow, and channels a torrent of magical energy %s!&n", dirs[random_dir]);

    if(exit_wallable(ch->in_room, random_dir, ch))
    {
      act(buff, TRUE, ch, obj, NULL, TO_ROOM);
      sprintf(buff, "%s", dirs[random_dir]);
      // store the direction keyword in a buffer to be passed to spell_wall_of_iron
      spell_wall_of_iron(60, ch, buff, 0, 0, 0);
    }
    return TRUE;
  }
  return FALSE;
}


// event for spell regeneration
void event_zion_netheril(P_char ch, P_char victim, P_obj obj, void *data);

int zion_netheril(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && (OBJ_WORN(obj) || OBJ_ROOM(obj)) )
  {
    /* can either remove the next line or set the second number higher if it hums too often */
    if( !number(0, 2) )
    {
      hummer(obj);
    }
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !arg )
  {
    return FALSE;
  }

  if( !(ch->equipment[PRIMARY_WEAPON] && (ch->equipment[PRIMARY_WEAPON] == obj))
    && !(ch->equipment[HOLD] && (ch->equipment[HOLD]) == obj) )
  {
    return FALSE;
  }

  if( !get_scheduled(ch, event_zion_netheril) )
  {
    // character doesn't have the event scheduled, so fire the event which handles the affect and will renew itself
    add_event(event_zion_netheril, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "blink") )
    {
      curr_time = time(NULL);
      // 15-20 sec timer ?
      if( obj->timer[1] + number(15, 20) <= curr_time )
      {
        act("You say 'blink'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly, and you feel your body begin to vibrate.", FALSE, ch, obj, obj, TO_CHAR);
        act("$n says 'blink'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly, and their body begins to vibrate violently!", TRUE, ch, obj, NULL, TO_ROOM);
        spell_blink(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[1] = curr_time;
        return TRUE;
      }
    }
    else if( isname(arg, "deflect") )
    {
      curr_time = time(NULL);
      // 500 sec timer == 8 min 20 sec.
      if (obj->timer[2] + 500 <= curr_time)
      {
        act("You say 'deflect'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q begins to send out ripples of pure magical energy!", FALSE, ch, obj, obj, TO_CHAR);
        act("$n says 'deflect'", FALSE, ch, obj, obj, TO_ROOM);
        act("$n's $q begins to send out ripples of pure magical energy!", TRUE, ch, obj, 0, TO_ROOM);
        if( ch->group )
        {
          cast_as_area(ch, SPELL_DEFLECT, 50, 0);
        }
        else
        {
          spell_deflect(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        }
        obj->timer[2] = curr_time;
        return TRUE;
      }
    }
  }

  return FALSE;
}


void event_zion_netheril(P_char ch, P_char victim, P_obj obj, void *data)
{
  // first check to make sure the item is still on the character
  bool has_item = false;
  int circle;
  char buf[256];

  // search through all of the possible proc spots of character
  // and check to see if the item is equipped
  for (int i = 0; i < NUM_PROCCING_SLOTS; i++)
  {
    P_obj item = ch->equipment[proccing_slots[i]];

    if( item && obj_index[item->R_num].func.obj == zion_netheril )
      has_item = true;
  }

  if( !has_item )
    return;



  if (!number(0, 10))
   {
    if (IS_PUNDEAD(ch) || (GET_RACE(ch) == RACE_ELADRIN))
    {
      for (circle = get_max_circle(ch); circle >= 1; circle--)
      {
        if (ch->specials.undead_spell_slots[circle] <
            max_spells_in_circle(ch, circle))
         {
          sprintf(buf, "&+LYou feel your %d%s circle power returning to you.\n",
          circle, circle == 1 ? "st" : (circle == 2 ? "nd" : (circle == 3 ? "rd" : "th")));
          send_to_char(buf, ch);
          ch->specials.undead_spell_slots[circle]++;
          break;
         }
       }
     }
  else
  {
  int spell = memorize_last_spell(ch);
  if( spell )
    {
      sprintf( buf, "&+WYou feel your power of %s &+Wreturning to you.&n\n",
        skills[spell].name );
      send_to_char(buf, ch);
    }
  }
  }
  add_event(event_zion_netheril, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
}

int zion_mace_of_earth(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time;
  int      dam;
  P_char   vict;
  struct proc_data *data;
  struct damage_messages messages = {
    "Your $q &+Wslams &+L$N with a &+yhuge boulder&+L!&N",
    "$n's $q &+Wslams &+Lyou with a &+yhuge boulder&+L!&N",
    "$n's $q &+Wslams &+L$N with a &+yhuge boulder&+L!&N",
    "A &+yhuge boulder&+L from your $q &+Wsmashes &n$N to a &+rbloody&n pulp!",
    "A &+yhuge boulder&+L from $n's $q shoots right towards your face!",
    "A &+yhuge boulder&+L from $n's $q &+Wsmashes &n$N to a &+rbloody&n pulp!",
    0, obj
  };

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

/*
  if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
    return (FALSE);
*/
  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "earth"))
    {
      curr_time = time(NULL);
      // 1 min timer.
      if (curr_time >= obj->timer[0] + 60)
      {
        act("You say 'earth'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'earth'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_group_stone_skin(45, ch, 0, 0, ch, NULL);
        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }

  if( (cmd == CMD_GOTHIT) && !number(0, 50) && !has_skin_spell(ch) && IS_ALIVE(ch) )
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

    act("$q &+Lreacts to the attack, and encases you in a shell of stone.&n", FALSE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);
    act("&+L$n's $q &+Lreacts to the attack, and encases $m in a shell of stone.&n", FALSE, ch, obj, vict, TO_VICT | ACT_NOTTERSE);
    act("&+L$n's $q &+Lreacts to $N&+L's vicious attack, and encases $m in a shell of stone.&n", FALSE, ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);
    spell_stone_skin(30, ch, 0, 0, ch, NULL);

    return FALSE;
  }

  vict = (P_char) arg;
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  dam = number(50, 200) * 4;
  // 1/25 chance.
  if( OBJ_WORN_BY(obj, ch) && CheckMultiProcTiming(ch) && !number(0, 24) )
  {
    act("Your $q &+ysummons the power of the earth and rock at $N.", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
    act("$n's $q &+ysummons the power of the earth and rock at you.", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
    act("$n's $q &+ysummons the power of the earth and rock at $N.", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
    if( char_in_list(vict) )
    {
      spell_earthquake(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( IS_ALIVE(ch) && is_char_in_room(vict, ch->in_room) )
    {
      /*spell_damage(ch, vict, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);*/
      spell_earthen_rain(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
    {
      return TRUE;
    }
  }
  return FALSE;
}

