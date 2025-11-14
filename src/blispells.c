#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "graph.h"
#include "guildhall.h"
#include "interp.h"
#include "new_combat_def.h"
#include "structs.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "utils.h"
#include "weather.h"
#include "sound.h"
#include "assocs.h"
#include "justice.h"
#include "mm.h"
#include "damage.h"
#include "objmisc.h"
#include "vnum.obj.h"
#include "utils.h"
#include "defines.h"
#include "necromancy.h"
#include "disguise.h"
#include "grapple.h"
#include "map.h"
#include "sql.h"
#include "graph.h"
#include "outposts.h"
#include "ctf.h"
#include "achievements.h"

extern P_room world;
extern const int top_of_world;

void spell_thornskin(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af1;
  bool shown;

  if(!ch)
  {
    logit(LOG_EXIT, "spell_thornskin called in magic.c with no ch");
    raise(SIGSEGV);
  }
  if(!IS_ALIVE(ch))
  {
    act("Lay still, you seem to be dead!", TRUE, ch, 0, 0, TO_CHAR);
    return;
  }
  if(!victim || !IS_ALIVE(victim))
  {
    act("$N is not a valid target.", TRUE, ch, 0, victim, TO_CHAR);
  }
  if(racewar(ch, victim))
  {
    if(NewSaves(victim, SAVING_PARA, 0))
    {
      act("$N evades your spell!", TRUE, ch, 0, victim, TO_CHAR);
      return;
    }
  }
  if( !IS_AFFECTED(victim, AFF_ARMOR) )
  {
    bzero(&af1, sizeof(af1));
    af1.type = SPELL_THORNSKIN;
    af1.duration =  25;
    af1.modifier = -1 * level / 4;
    af1.location = APPLY_AC;
    af1.bitvector = AFF_ARMOR;
    af1.bitvector5 = AFF5_THORNSKIN;
    af1.level = (ushort)level;

    affect_to_char(victim, &af1);
    act("&+y$n&+y's skin gains the toughness of dead plant life, &+Lthorns&+y and brambles grow from $s skin!",
      FALSE, victim, 0, 0, TO_ROOM);
    act("&+yYour skin gains the toughness of dead plant life, &+Lthorns&+y and brambles grow from your skin!",
      FALSE, victim, 0, 0, TO_CHAR);
  }
  else if( !IS_AFFECTED5(victim, AFF5_THORNSKIN) && !IS_AFFECTED(victim, AFF_BARKSKIN) )
  {
    bzero(&af1, sizeof(af1));
    af1.type = SPELL_THORNSKIN;
    af1.duration =  25;
    af1.bitvector5 = AFF5_THORNSKIN;
	af1.level = (ushort)level;

    affect_to_char(victim, &af1);
    act("&+LThorns&+y and brambles grow from $n&+y's skin!", FALSE, victim, 0, 0, TO_ROOM);
    act("&+LThorns&+y and brambles grow from your skin!", FALSE, victim, 0, 0, TO_CHAR);
  }
  else
  {
    struct affected_type *af1;

    shown = FALSE;
    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_THORNSKIN)
      {
        if( !shown )
        {
          send_to_char( "&+yThe thorns re-harden around you.&n\n", victim );
          shown = TRUE;
        }
        af1->duration = 25;
		af1->level = (ushort)level;
      }
    }
    if( !shown )
    {
      send_to_char( "&+WYou're already affected by an armor-type spell.\n", victim );
    }
  }
}

// Range Spell!
// A burning globe of fire that burns target 2d6 damage / round.
// Since range, limited to one round with lightning bolt damage.
void spell_flame_sphere(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  struct damage_messages messages = {
    "You grin as your opponent is &+Ren&+Ygu&+rlf&+Red&n in &+rfl&+Ram&+res&n!",
    "You scream in agony as you &+Rb&+Yu&+rr&+Rn&n in a &+Rsph&+rer&+Re of f&+Yi&+rr&+Re&n!",
    "$N screams in agony as $S &+Rb&+Yu&+rr&+Rns&n in a &+Rsph&+rer&+Re of f&+Yi&+rr&+Re&n!",
    "Your &+Rsph&+rer&+Re&n of &+Rf&+Yi&+rr&+Re&n proves to be too much for $N, who turns to &+Lash&n.",
    "You &+Wscream&n as your body turns to &+Las&nhe&+Ls&n.",
    "$N &+Wscreams&n as $S body turns to &+Las&nhe&+Ls&n.",
      0
  };

  int num_dice = (level / 5);
  int dam = (dice(num_dice+5, 6) * 3);

  if(!NewSaves(victim, SAVING_SPELL, 0))
  {
    dam = (int) (dam * 1.33);
  }
  if( GET_SPEC(ch, CLASS_BLIGHTER, SPEC_RUINER) )
  {
    dam = (dam * 112) / 100;
  }

  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);

}

// Room spell.
void spell_desecrate_land(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct room_affect *raf;
  struct room_affect  af;

  if( !ch || get_spell_from_room(&world[ch->in_room], SPELL_DESECRATE_LAND) )
  {
    return;
  }

  if( (raf = get_spell_from_room(&world[ch->in_room], SPELL_CONSECRATE_LAND)) )
  {
    affect_room_remove(ch->in_room, raf);
    send_to_char("&+YYou destroy the &+Crunes&+Y laying about the area.&n\r\n", ch);
    act("&+Y$n&+Y's prayer shatters the &+Crunes&+Y laying around the area.&n",0, ch, 0, 0, TO_ROOM);
    return;
  }

  switch (world[ch->in_room].sector_type)
  {
  case SECT_INSIDE:
  case SECT_UNDRWLD_INSIDE:
    send_to_char("Try again, OUTDOORS this time.\r\n", ch);
    return;
    break;
  case SECT_CITY:
  case SECT_ROAD:
  case SECT_CASTLE_WALL:
  case SECT_CASTLE_GATE:
  case SECT_UNDRWLD_CITY:
  case SECT_CASTLE:
    send_to_char("Nothing happens.  Perhaps you need to be farther outdoors...\r\n", ch);
    return;
    break;
  case SECT_SWAMP:
  case SECT_UNDRWLD_SLIME:
  case SECT_FIELD:
  case SECT_FOREST:
  case SECT_HILLS:
  case SECT_MOUNTAIN:
  case SECT_UNDRWLD_WILD:
  case SECT_UNDRWLD_MUSHROOM:
  case SECT_UNDRWLD_MOUNTAIN:
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_DESERT:
    send_to_char("&+YYou fill the area with &+Lnegative energy&+Y.&n\r\n", ch);
    act("&+L$n&+L's prayer floods the area with dark energy.&n",0, ch, 0, 0, TO_ROOM);
    memset(&af, 0, sizeof(struct room_affect));
    af.type = SPELL_DESECRATE_LAND;
    af.duration = GET_LEVEL(ch) * 4;
    af.ch = ch;
    affect_to_room(ch->in_room, &af);
    break;
  case SECT_PLANE_OF_AVERNUS:
    send_to_char("This place is already desecrated, beyond your powers even.\r\n", ch);
    return;
    break;
  case SECT_NO_GROUND:
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_UNDRWLD_NOSWIM:
  case SECT_UNDRWLD_WATER:
  case SECT_FIREPLANE:
  case SECT_UNDRWLD_LIQMITH:
  case SECT_NEG_PLANE:
  case SECT_UNDERWATER:
  case SECT_UNDRWLD_NOGROUND:
  case SECT_UNDERWATER_GR:
  case SECT_OCEAN:
    send_to_char("Desecrate _LAND_...  There is no land here!\r\n", ch);
    return;
  break;
  default:
    logit(LOG_DEBUG, "Bogus sector_type (%d) in desecrate_land",
          world[ch->in_room].sector_type);
    send_to_char("How strange!  This terrain doesn't seem to exist!\r\n", ch);
    return;
    break;
  }
}

// Target spell
// Similar to disease
void spell_contagion(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  int temp;

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  temp = (level < 20) ? 1 : level / 10;

  if(get_spell_from_room(&world[ch->in_room], SPELL_DESECRATE_LAND))
  {
    temp += 3;
  }

/* Allowing multiple contagions atm.
  if( affected_by_spell(victim, SPELL_CONTAGION) )
  {
    act("$N is sick enough.", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
*/
  // If saves or is undead, do nothing...
  if( NewSaves(victim, SAVING_PARA, temp) || IS_UNDEADRACE(victim) )
  {
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_CONTAGION;

  switch( number(1,7) )
  {
    // Blinding sickness: 1d4 Str
    case 1:
      send_to_char("&+yYou start coughing horribly!&n\n", victim);
      act("&+y$n &+ysuddenly starts coughing a lot.&n", FALSE, victim, 0, 0, TO_ROOM);
      af.duration = 3 * (1 + temp);
      af.modifier = -dice( 5, 4 );
      af.location = APPLY_STR;
      affect_to_char(victim, &af);
      if((level < 0) || !saves_spell(victim, SAVING_SPELL))
      {
        send_to_char("&+LYou suddenly go blind!\n", victim);
        blind(ch, victim, 25 * WAIT_SEC);
      }
    break;
    // Cackle fever: 1d6 Wis
    case 2:
      send_to_char("&+yYou start cackling hysterically!&n\n", victim);
      act("&+y$n &+ysuddenly starts cackling.&n", FALSE, victim, 0, 0, TO_ROOM);
      af.duration = 3 * (1 + temp);
      af.modifier = -dice( 5, 6 );
      af.location = APPLY_WIS;
      affect_to_char(victim, &af);
    break;
    // Filth fever: 1d3 Dex and 1d3 Con
    case 3:
      send_to_char("&+yYou suddenly feel really dirty!&n\n", victim);
      act("&+y$n &+ystarts scratching at his skin.&n", FALSE, victim, 0, 0, TO_ROOM);
      af.duration = 3 * (1 + temp);
      af.modifier = -dice( 5, 3 );
      af.location = APPLY_DEX;
      affect_to_char(victim, &af);
      af.modifier = -dice( 5, 3 );
      af.location = APPLY_CON;
      affect_to_char(victim, &af);
    break;
    // Mindfire: 1d4 Int
    case 4:
      send_to_char("&+yYour mind goes &+Ra&+Yb&+Rl&+Ya&+Rz&+Ye&+y!&n\n", victim);
      act("&+y$n &+ystarts sweating profusely.&n", FALSE, victim, 0, 0, TO_ROOM);
      af.duration = 3 * (1 + temp);
      af.modifier = -dice( 5, 4 );
      af.location = APPLY_INT;
      affect_to_char(victim, &af);
    break;
    // Red ache: 1d6 Str
    case 5:
      send_to_char("&+yYou suddenly get a horrible stomach ache!&n\n", victim);
      act("&+y$n &+ystarts leaning forward grasping $s belly.&n", FALSE, victim, 0, 0, TO_ROOM);
      af.duration = 3 * (1 + temp);
      af.modifier = -dice( 5, 6 );
      af.location = APPLY_STR;
      affect_to_char(victim, &af);
    break;
    // Shakes: 1d8 Dex
    case 6:
      send_to_char("&+yYou start sweating and shaking!&n\n", victim);
      act("&+y$n &+ystarts shaking.&n", FALSE, victim, 0, 0, TO_ROOM);
      af.duration = 3 * (1 + temp);
      af.modifier = -dice( 5, 8 );
      af.location = APPLY_DEX;
      affect_to_char(victim, &af);
    break;
    // Slimy doom: 1d4 Con
    case 7:
      send_to_char("&+yYou suddenly feel really slimy!&n\n", victim);
      act("&+y$n &+ystarts sweating and slipping.&n", FALSE, victim, 0, 0, TO_ROOM);
      af.duration = 3 * (1 + temp);
      af.modifier = -dice( 5, 4 );
      af.location = APPLY_CON;
      affect_to_char(victim, &af);
    break;
  }
}

// Target Plant only spell
// 1d6 damage / caster lvl to 15.
void spell_blight(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  struct damage_messages messages = {
    "You point at $N and $S &+ywilts&n and &+Lwithers&n.",
    "$n points at you and you wither!",
    "$n points at $N and $S &+ywilts&n and &+Lwithers&n.",
    "&+LYou leave just a husk of $N&+L.&n",
    "&+LYou are reduced to a husk.&n",
    "&+L$N &+Lwithers into a husk&n.",
     0
  };

  if( GET_RACE(victim) != RACE_PLANT && GET_RACE(victim) != RACE_SLIME )
  {
    send_to_char( "This spell only works on plants.\n", ch );
    return;
  }

  int dam = (level > 40) ? dice( 40*3, 6 ) : dice( level*3, 6 );
  if( GET_SPEC(ch, CLASS_BLIGHTER, SPEC_RUINER)
    || GET_SPEC(ch, CLASS_BLIGHTER, SPEC_SCOURGE) )
  {
    dam = (dam * 112) / 100;
  }

  if( !NewSaves(victim, SAVING_SPELL, 0) )
  {
    dam *= 2;
  }

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);

}

// Room spell.
// Stops all forms of magical transportation.
void spell_forbiddance(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct room_affect  af;

  if( !ch || !IS_ALIVE(ch) )
  {
    return;
  }

  send_to_char("&+YYou stop the flow of &+Mmagical transport energy&+Y.&n\r\n", ch);
  act("&+L$n&+L sends forth a strange energy into the room.&n",0, ch, 0, 0, TO_ROOM);
  memset(&af, 0, sizeof(struct room_affect));
  af.type = SPELL_FORBIDDANCE;
  af.duration = (GET_LEVEL(ch) * 4);
  af.room_flags = ROOM_NO_RECALL + ROOM_NO_TELEPORT + ROOM_NO_SUMMON + ROOM_NO_GATE;
  af.ch = ch;
  affect_to_room(ch->in_room, &af);

}

void event_waves_fatigue(P_char ch, P_char victim, P_obj obj, void *data)
{
  int moves = *(int *)data;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( GET_VITALITY(victim) < moves )
  {
    send_to_char( "&+YYou feel really tired!&n\n", victim );
    GET_VITALITY(victim) = 0;
  }
  else
  {
    send_to_char( "&+yYou feel tired!&n\n", victim );
    GET_VITALITY(victim) -= moves;
  }
}

extern struct link_description link_types[];
// Target spell.
// Create 3 waves (2 events) that sap moves.
void spell_waves_fatigue(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int      moves;
  P_nevent e;
  char_link_data *link;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  // If victim is already suffering..
  // Look through all links where victim is a slave.
  for( link = victim->linked; link; link = link->next_linked )
  {
    // Look through master's events for the waves_fatigue event.
    LOOP_EVENTS_CH( e, link->linking->nevents )
    {
      // If master has a waves of fatigue event on victim already..
      if( e->func == event_waves_fatigue && e->victim == victim )
      {
        act("&+Y$N&+Y already looks tired.&n", FALSE, ch, 0, victim, TO_CHAR );
        act("&+YYou begin sweating, but don't feel any worse.&n", FALSE, NULL, 0, victim, TO_VICT);
        return;
      }
    }
  }

  // moves: level / 4 -> 10 - 16 @ 56 * 3 waves = 30 - 48.
  moves = GET_LEVEL(ch)/4 + number( -4, 2);
  // level / 6 : 26: 4, 32: 5, 44: 7, 50: 8, 56: 9.
  if( NewSaves(victim, SAVING_PARA, (level-2) / 6) )
  {
    // 2-4 * 3 waves = 6-12 moves.
    moves /= 4;
    act("&+y$n&+y looks a bit tired.&n", 0, victim, 0, 0, TO_ROOM);
  }
  else
  {
    act("&+y$n&+y looks really tired.&n", 0, victim, 0, 0, TO_ROOM);
  }
  event_waves_fatigue( ch, victim, NULL, &moves);
  add_event(event_waves_fatigue, PULSE_VIOLENCE/2, ch, victim, 0, 0, &moves, sizeof(moves));
  add_event(event_waves_fatigue, PULSE_VIOLENCE, ch, victim, 0, 0, &moves, sizeof(moves));
}

void event_acid_rain(P_char ch, P_char victim, P_obj obj, void *data)
{
  int    room = ch->in_room;
  int    dam;
  P_char next;
  struct damage_messages messages = {
    "&+G$N&+G is burned as the rain dissolves $S skin.&n",
    "&+GYou are burned as the rain dissolves your skin.&n",
    "",
    "&+g$N &+gmelts into a pile of &+GGOO&n ... $E is no more!",
    "&+GThe rain &+gconsuming your flesh &+Gdevours you completely!",
    "$N &+gmelts into a pile of &+GGOO&n ... $E is no more!",
      0
  };

  if( !IS_ALIVE(ch) )
  {
    return;
  }

//  dam = 110 + GET_LEVEL(ch) * 3 + number(1, 10);
  dam = 30 + GET_LEVEL(ch) + number(0, 20);

  if( GET_SPEC(ch, CLASS_BLIGHTER, SPEC_STORMBRINGER) )
  {
    dam += 20;
  }

  // Targetted acid rain.
  if( victim )
  {
    if( IS_ALIVE(victim) )
    {
      spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_NODEFLECT, &messages);
    }
    return;
  }

  if( world[room].people )
  {
    act("An awful &+Gburning rain&n continues to fall from the sky.",0, world[room].people, 0, 0, TO_ROOM);
    act("An awful &+Gburning rain&n continues to fall from the sky.",0, world[room].people, 0, 0, TO_CHAR);
  }

  for( victim = world[room].people; victim; victim = next )
  {
    next = victim->next_in_room;
    if( victim == ch || ( ch->group && ch->group == victim->group ) )
      continue;
    if( !NewSaves(victim, SAVING_SPELL, GET_LEVEL(ch)>50 ? GET_LEVEL(ch)-50 : 0) )
    {
      spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_NODEFLECT, &messages);
    }
    else
    {
      spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_NODEFLECT, &messages);
    }
  }
}

bool has_scheduled_area_acid_rain( P_char ch )
{
  P_nevent e;

  LOOP_EVENTS_CH( e, ch->nevents )
  {
    if( e->func == event_acid_rain && !(e->victim) )
    {
      return TRUE;
    }
  }

  return FALSE;
}

// Area spell.
// Create events that do 2d6 damage for lvl rounds.
void spell_acid_rain(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int waves = level / 10;

  if( !ch || !IS_ALIVE(ch) )
  {
    return;
  }

  // If ch already has a acid rain going.. fail.
  if( has_scheduled_area_acid_rain( ch ) && !victim )
  {
    send_to_char( "You've already called some &+grain&n.\n\r", ch );
    return;
  }

/*
  if( !IS_OUTSIDE(ch->in_room) )
  {
    send_to_char( "Try again.. Outdoors next time!\n\r", ch );
    return;
  }
*/

  send_to_char("The clouds above converge and turn &+Lpitch black&n, and suddenly an awful &+Gburning rain&n begins to fall from the sky.\n\r", ch );
  act("The clouds above converge and turn &+Lpitch black&n, and suddenly an awful &+Gburning rain&n begins to fall from the sky.",0, ch, 0, 0, TO_ROOM);

  for( int i = 1; i <= waves; i++ )
  {
    add_event(event_acid_rain, (i * PULSE_VIOLENCE)/2, ch, victim, 0, 0, NULL, 0 );
  }
}

// Sunray equivalent.
// 1d6 damage (1d8 to water mentals/plants) per lvl.
void spell_horrid_wilting(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "$N &+Lbegins to dry and wilt.&n",
    "&+LYou begin to dry out and wilt.&n",
    "$N &+Lbegins to dry and wilt.&n",
    "You suck all the water from $N.",
    "You are completely dried up.",
    "$N dries up completely and crumbles&n.",
      0
  };
  int dam = dice((int)(level * 3), 6) - number(0, 40);

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  int mod = BOUNDED(0, (GET_LEVEL(ch) - GET_LEVEL(victim)), 20);

  if(!NewSaves(victim, SAVING_SPELL, mod))
  {
      dam = (int)(dam * 1.30);
  }

  if(!NewSaves(victim, SAVING_SPELL, (int)(mod / 3)) && !IS_BLIND(victim))
  {
    send_to_char( "&+CYour &+ceyes &+Bdry &+bout&+L!!!&n\n", victim );
    blind(ch, victim, number((int)(level / 3), (int)(level / 2)) * WAIT_SEC);
  }

  if( GET_SPEC(ch, CLASS_BLIGHTER, SPEC_RUINER) )
  {
    dam = (dam * 112) / 100;
  }

  if( GET_RACE(victim) != RACE_PLANT && GET_RACE(victim) != RACE_SLIME && GET_RACE(victim) != RACE_W_ELEMENTAL )
  {
    spell_damage(ch, victim, dam, SPLDAM_ACID, 0, &messages);
  }
  else
  {
    spell_damage(ch, victim, (dam*4)/3, SPLDAM_ACID, 0, &messages);
  }
}

// Summon spell: 1d4+2 shambling mounds.
void spell_shambler(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int count;
  P_char mob;
  struct char_link_data *cld;

  if( !ch || !IS_ALIVE(ch) )
  {
    return;
  }

  for (cld = ch->linked; cld; cld = cld->next_linked)
  {
    if(cld->type == LNK_PET && cld->linking->only.npc->R_num == real_mobile(100) )
    {
      send_to_char( "You already have a mound.\n", ch );
      return;
    }
  }
  // Don't need an undead army + shambler army.
  if( count_pets( ch ) > 1 )
  {
    send_to_char( "You already have enough pets.", ch );
    return;
  }

  switch (world[ch->in_room].sector_type)
  {
    case SECT_CITY:
    case SECT_DESERT:
    case SECT_ROAD:
    case SECT_UNDRWLD_CITY:
      count = dice( 1, 2);
      break;
    case SECT_FIELD:
    case SECT_HILLS:
    case SECT_UNDRWLD_WILD:
    case SECT_UNDRWLD_MOUNTAIN:
    case SECT_MOUNTAIN:
      count = dice( 1, 3 ) + 1;
      break;
    case SECT_UNDRWLD_SLIME:
    case SECT_SWAMP:
    case SECT_UNDRWLD_MUSHROOM:
    case SECT_SNOWY_FOREST:
    case SECT_FOREST:
      count = dice( 1, 4 ) + 2;
      break;
    default:
      send_to_char( "&+yThere's no &+gvegetation &+yaround here.&n\n\r", ch );
      return;
      break;
  }

  // Load count shambling mounds 11hd
  while( count-- )
  {
    // Use vnum to load them here and set charm.
    mob = read_mobile(100, VIRTUAL);
    if(!mob)
    {
      logit(LOG_DEBUG, "spell_shambler(): mob 100 not loadable");
      send_to_char("Bug in spell_shambler.  Tell a god!\n", ch);
      return;
    }
    char_to_room( mob, ch->in_room, 0 );
    /* if the pet will stop being charmed after a bit, also make it suicide 2-12 minutes later */
    if( setup_pet(mob, ch, 15, PET_NOCASH | PET_NOAGGRO) >= 0 )
    {
      add_event(event_pet_death, dice(2,6) * 60 * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    }
    add_follower(mob, ch);
    apply_achievement(mob, TAG_CONJURED_PET);
  }
}

// Target Damage.
void spell_implosion(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  // Slight decrease in damage and decent cecrease in damage variance.
  int dam = 5*level + dice(level * 2, 10);

  struct damage_messages messages = {
    "Your &+We&+wy&+We&+ws&n roll back into your head as you cause $N's &+Gcells&n to &+rimplode&n!",
    "The &+Rpain&n is &+rexcruciating&n as $n summons the power of &+Gnature's &+Ldarker side&n to &+Ycrush&n your body!",
    "$N almost hits his knees in &+Wagony&n as $n &+Ycrushes&n his body, causing parts of him to &+Rimplode&n!",
    "There is one final &+Csu&+crg&+Ce&N of raw &+Mpower&N before you &+Ycrush&n $N's &+Rhe&+rar&+Rt&n.",
    "As $n crushes you, there is one final &+Csu&+crg&+Ce&N of raw &+Rpa&+ri&+Rn&N before your &+Rhe&+rar&+Rt&n is &+Wcrushed&n, then &+Lblackness&n...",
    "$N is utterly &+Ccr&+cush&+Ced&n by $n's &+rdestructive&n &+Gnatural &+mmagic&n!",
      0
  };

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(NewSaves(victim, SAVING_SPELL, level/17))
  {
    dam /= 1.8;
  }
  if( GET_SPEC(ch, CLASS_BLIGHTER, SPEC_RUINER) )
  {
    dam = (dam * 112) / 100;
  }
  spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);

}

void event_sandstorm(P_char ch, P_char victim, P_obj obj, void *data)
{
  int    level = GET_LEVEL(ch);
  int    rroom;
  P_char next;
  struct damage_messages messages = {
    "&+yYou en&+Ygu&+ylf $N&+y in s&+Ya&+ynd which &+Rsh&+rre&+Rds&+y $S &+Yskin&+y and gets in $S &+Weyes&+y!&n",
    "&+yYou are en&+Ygu&+ylfed in s&+Ya&+ynd which &+Rsh&+rre&+Rds&+y your &+Yskin&+y and gets in your &+Weyes&+y!&n",
    "&+y$N&+y is en&+Ygu&+ylfed in s&+Ya&+ynd which &+Rsh&+rre&+Rds&+y $S &+Yskin&+y and gets in $S &+Weyes&+y!&n",
    "&+yYou smile as the &+Lhowling &+ys&+Yan&+yds consume $N&+y completely!&n",
    "&+ySand&+Y, sand&+y, sand and more s&+Yan&+yd &+Rsh&+rre&+Rds&+y you and &+Wfills&+y your lungs, causing a major case of &+Ldeath&+y!&n",
    "&+yYou can barely &+wsee&+y, but you believe that you just witnessed the &+Ysandy &+Ldeath&+y of $N&+y.&n",
      0
  };

  if( !IS_ALIVE(ch) )
  {
    return;
  }
  rroom = ch->in_room;
  if( ROOM_VNUM(rroom) == NOWHERE )
  {
    return;
  }

  if( IS_WATER_ROOM(rroom) )
  {
    act( "&+yYour wall of s&+Ya&+ynd becomes &+Cwet, &+cclumps up, and &+Bcollapses in a &+bsplash!&n", FALSE, ch, 0, victim, TO_CHAR );
    act("&+yThe wall of s&+Ya&+ynd following $n becomes &+Cwet, &+cclumps up, and &+Bcollapses in a &+bsplash!&n", FALSE, ch, 0, 0, TO_ROOM );
    return;
  }

  send_to_room( "&+YA &+RM&+rA&+RSSI&+rV&+RE &+yw&+Ya&+yll of s&+Yan&+yd engulfs the area crashing into everything!!!&n\n", rroom );

  for( victim = world[rroom].people; victim; victim = next )
  {
    next = victim->next_in_room;
    if( victim == ch || ( ch->group && ch->group == victim->group ) )
      continue;
    if( !NewSaves(victim, SAVING_SPELL, level/8) )
    {
      spell_damage(ch, victim, dice(level * 3, 6), SPLDAM_GAS, 0, &messages);
      if( victim && IS_ALIVE(victim) )
      {
        blind(ch, victim, number((int)(level / 3), (int)(level / 2)) * WAIT_SEC);
      }
    }
    else
    {
      spell_damage(ch, victim, dice(level * 3, 6)/2, SPLDAM_GAS, 0, &messages);
    }
  }
}

// Just display growing messages..
void event_sandstorm_message(P_char ch, P_char victim, P_obj obj, void *data)
{
  int rroom, num_rounds = *((int *)data);

  // If ch isn't alive in a room, we can't very well have a room-area damage spell, can we?
  if( !IS_ALIVE(ch) )
  {
    return;
  }
  rroom = ch->in_room;
  if( ROOM_VNUM(rroom) == NOWHERE )
  {
    return;
  }

  if( IS_WATER_ROOM(rroom) )
  {
    act( "&+yYour wall of s&+Ya&+ynd becomes &+Cwet, &+cclumps up, and &+Bcollapses in a &+bsplash!&n", FALSE, ch, 0, victim, TO_CHAR );
    act("&+yThe wall of s&+Ya&+ynd following $n becomes &+Cwet, &+cclumps up, and &+Bcollapses in a &+bsplash!&n", FALSE, ch, 0, 0, TO_ROOM );
    return;
  }

  send_to_room( "&+yA MASS&+YIV&+yE wall of s&+Ya&+ynd engulfs the area, crashing into everything in sight!&n\n", rroom );

  if( --num_rounds > 0 )
  {
    add_event( event_sandstorm_message, PULSE_VIOLENCE*2, ch, victim, 0, 0, &(num_rounds), sizeof(num_rounds) );
  }
  else
  {
    add_event( event_sandstorm, PULSE_VIOLENCE*2, ch, victim, 0, 0, NULL, 0 );
  }
}

// Area Damage on timer.
void spell_sandstorm(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  // number of rounds before sandstorm hits.
  int num_rounds = number( 1, 3 );
  int rroom = ch->in_room;
  P_char roomie;

  if( !IS_ALIVE(ch) || ROOM_VNUM(ch->in_room) == NOWHERE )
  {
    return;
  }

  act( "&+yYour &+Weyes roll&+y back into your head as you begin to &+Ysummon&+y the fury of the &+Ys&+ya&+Yn&+yd&+Ys&+y!&n", FALSE, ch, 0, victim, TO_CHAR );

  if( IS_WATER_ROOM(rroom) )
  {
    act( "&+BA &+cwave&+B comes up and soaks everyone in the room!&n", FALSE, ch, 0, victim, TO_CHAR );
    act("The &+Weyes&n in $n's head roll back, and a &+cwave&n rises from the &+bwater!&n", FALSE, ch, 0, 0, TO_ROOM );
    for( roomie = world[rroom].people; roomie; roomie = roomie->next_in_room )
    {
      act("&+BThe &+cwave&+B soaks you!&N", FALSE, ch, 0, roomie, TO_VICT );
      make_wet(roomie, WAIT_MIN);
    }
    return;
  }

  act( "&+yA MASS&+YIV&+yE wall of s&+Ya&+ynd engulfs the area, crashing into everything in sight!&n",0, ch, 0, 0, TO_ROOM);

  add_event( event_sandstorm_message, PULSE_VIOLENCE*2, ch, victim, 0, 0, &(num_rounds), sizeof(num_rounds) );
}

// Target Damage.
void spell_firelance(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "As you complete your spell, you launch a &+Rmassive fi&+rr&+Yel&+ran&+Rce&n at $N, &+rscorching&n $S skin!",
    "As $n completes $s spell, $e launches a &+Rmassive fi&+rr&+Yel&+ran&+Rce&n at you, &+rscorching&n your skin!",
    "As $n completes $s spell, $e launches a &+Rmassive fi&+rr&+Yel&+ran&+Rce&n at $N, &+rscorching&n $S skin!",
    "$N succumbs to your &+Rmassive fi&+rr&+Yel&+ran&+Rce&n.",
    "You succumb to $n's &+Rmassive fi&+rr&+Yel&+ran&+Rce&n.",
    "$N succumbs to $n's &+Rmassive fi&+rr&+Yel&+ran&+Rce&n.",
      0
  };

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }
  dam = dice( 4 * MIN(17, level / 2 + 1), 7 );

  if( GET_SPEC(ch, CLASS_BLIGHTER, SPEC_RUINER) )
  {
    dam = (dam * 112) / 100;
  }

  if(NewSaves(victim, SAVING_SPELL, 1.5))
  {
    dam /= 1.5;
  }

  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);

}

void event_drain_nature(P_char ch, P_char vict, P_obj obj, void *data)
{
  int healpoints, wavevalue, x;

  healpoints = wavevalue = *((int *) data);

  switch( world[vict->in_room].sector_type )
  {
  case SECT_UNDRWLD_CITY:
  case SECT_CITY:
    healpoints = (healpoints * 2) / 3;
    break;
  case SECT_FIELD:
    healpoints = (healpoints * 4) / 3;
    break;
  case SECT_FOREST:
  case SECT_UNDRWLD_MUSHROOM:
    healpoints = (healpoints * 3) / 2;
    break;
  case SECT_HILLS:
  case SECT_UNDRWLD_WILD:
    healpoints = (healpoints * 5) / 4;
    break;
  case SECT_UNDERWATER_GR:
  case SECT_UNDRWLD_SLIME:
  case SECT_MOUNTAIN:
  case SECT_UNDRWLD_MOUNTAIN:
    healpoints = (healpoints * 6) / 5;
    break;
  case SECT_UNDRWLD_LOWCEIL:
  case SECT_UNDRWLD_LIQMITH:
    healpoints = (healpoints * 7) / 6;
    break;
  default:
    break;
  }

  x = vamp(vict, healpoints, GET_MAX_HIT(vict));
  update_pos(vict);

  if( x > 0 && IS_FIGHTING(vict) )
    gain_exp(ch, vict, x, EXP_HEALING);

  wavevalue /= 2;
  if( wavevalue > 1 )
    add_event(event_drain_nature, 2, ch, vict, 0, 0, &wavevalue, sizeof(wavevalue));
}

// Heals the target/cures blind.
void spell_drain_nature( int level, P_char ch, char *arg, int type, P_char victim, P_obj obj )
{
  struct affected_type af;
  int healpoints;

  if( !IS_ALIVE(victim) || !IS_ALIVE(ch) )
    return;

  // 32 at level 21 -> 63 hps healing, 50 at level 56 -> 97 hps healing, modified by terrain.
  healpoints = (level / 2) + 22;

  if( !GET_CLASS(ch, CLASS_BLIGHTER) )
  {
    healpoints /= 2;
  }

  if( GET_CLASS(ch, CLASS_BLIGHTER) && IS_BLIND(victim) )
    spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);

  grapple_heal(victim);

  if( healpoints < 8 )
  {
    healpoints = 8;
  }

  if( GET_SPEC(ch, CLASS_BLIGHTER, SPEC_SCOURGE) )
  {
    healpoints += 50;
  }

  add_event(event_drain_nature, 1, ch, victim, 0, 0, &healpoints, sizeof(healpoints));

  if( ch == victim )
    act( "&+yYou drain &+Whealth&+y from your surroundings.&n", FALSE, ch, 0, victim, TO_CHAR );
  else
  {
    act("&+y$n&+y sucks the &+glife&+y out of the surroundings, &+Whealing&+y you.&n",
      FALSE, ch, 0, victim, TO_VICT );
    act("&+yYou drain &+Whealth&+y from your surroundings, sending it to $N&+y.&n",
      FALSE, ch, 0, victim, TO_CHAR);
  }
  act("&+y$n&+y sucks the &+glife&+y out of the surroundings, &+Whealing&+y $N&+y.&n",
    FALSE, ch, 0, victim, TO_NOTVICT);
}

void spell_create_pond(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj )
{
  P_obj pond;

  if (world[ch->in_room].sector_type == SECT_NO_GROUND ||
      world[ch->in_room].sector_type == SECT_UNDRWLD_NOGROUND ||
      world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("&+bA pond usually needs more solid ground for support!\n", ch);
    return;
  }

  pond = read_object(749, VIRTUAL);
  if(!pond)
  {
    logit(LOG_DEBUG, "spell_create_pond(): obj 749 (pond) not loadable");
    send_to_char("Tell someone to make a pond object ASAP!\n", ch);
    return;
  }

  pond->value[0] = GET_LEVEL(ch);
  send_to_room("&+bA pond grows out of nowhere!\n", ch->in_room);
  set_obj_affected(pond, 60 * 10, TAG_OBJ_DECAY, 0);
  obj_to_room(pond, ch->in_room);
}

// Target Damage.
void spell_toxic_fog(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "&+GToxic ten&+gdri&+Gls&n of &+Wm&+Li&+ws&+Wt float out and &+csuffocate&n $N &+Wcausing&+w them to &+cchoke&n!",
    "&+GToxic ten&+gdri&+Gls&n of &+Wm&+Li&+ws&+Wt float out to &+csuffocate&n you, &+Wcausing&+w you to &+Cchoke&n!",
    "$N &+Cgasps&n for &+Cair&n as $n's &+Glethal&n &+Wm&+Li&+ws&+Wt&n envelops $M!",
    "You smile as $N's &+Wlungs&n fill with &+Gto&+gxi&+Gc poison&n and they turn &+wgrey&n and &+Bbl&+bu&+Be&N before keeling over &+rdead&n.",
    "You &+cgasp&n for &+Ca&+ci&+Cr as $n's &+Wm&+Li&+ws&+Wt &ncompletely fills your &+Wlungs&n before passing out for lack of &+Coxygen&n...",
    "$N &+cwheezes&n and &+Cchokes&n as $E turns &+Bbl&+bu&+Be&n and collapses into a crumpled heap.",
      0
  };

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(level > 50)
  {
    dam = dice( 4 * MIN(20, (level / 2 + 1)), 8 );
  }
  else
  {
    dam = dice( 4 * MIN(20, (level / 2 + 1)), 7 );
  }

  if(!NewSaves(victim, SAVING_SPELL, 1.5))
  {
    dam /= 1.5;
  }

  spell_damage(ch, victim, dam, SPLDAM_GAS, 0, &messages);

}

void spell_faluzures_vitality(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool message = FALSE;
  int healpoints = 3 * level + level / 2;

  if(!ch)
  {
    logit(LOG_EXIT, "spell_faluzures_vitality: No ch!");
    raise(SIGSEGV);
  }

  if(affected_by_spell(ch, SPELL_ESHABALAS_VITALITY))
  {
    send_to_char("&+LThe blessings of the God &+yFa&+Lluz&+yure&+L are denied!&n\r\n", victim);
    return;
  }

  if(affected_by_spell(ch, SPELL_VITALITY))
  {
    send_to_char("&+LThe God &+yFa&+Lluz&+yure&+L will not further bless your vitality...&n\r\n", victim);
    return;
  }

  if(affected_by_spell(ch, SPELL_MIELIKKI_VITALITY))
  {
    send_to_char("&+LThe God &+yFa&+Lluz&+yure&+L will not further bless your vitality...&n\r\n", victim);
    return;
  }

  if(affected_by_spell(ch, SPELL_FALUZURES_VITALITY))
  {
    struct affected_type *paf;

    for( paf = victim->affected; paf; paf = paf->next )
    {
      if( paf->type == SPELL_FALUZURES_VITALITY )
      {
        paf->duration = 15;
        message = true;
      }
    }
    if(message)
      send_to_char("&+yThe God graces you.\r\n", victim);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_FALUZURES_VITALITY;
  af.duration = 15;
  af.modifier = healpoints;
  af.location = APPLY_HIT;
  affect_to_char(victim, &af);

  af.modifier = level;
  af.location = APPLY_MOVE;
  affect_to_char(victim, &af);

  send_to_char("&+yYou feel the &+Lcold &+ybreath of the God &+yFa&+Lluz&+yure.&n\r\n", ch);
}

// Blighter's version of endurance.
void spell_sap_nature(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj )
{
  struct affected_type af;
  struct affected_type *afp;
  int skl_lvl;

  if( !IS_ALIVE(victim) || !IS_ALIVE(ch) )
  {
    return;
  }

  if( affected_by_spell(victim, SPELL_ENDURANCE) )
  {
    send_to_char("You can't possibly regain movement any faster.\n", victim);
    return;
  }

  if( affected_by_spell(victim, SPELL_MIELIKKI_VITALITY)
    && !GET_CLASS(victim, CLASS_DRUID)
    && !GET_SPEC(victim, CLASS_RANGER, SPEC_HUNTSMAN) )
  {
    send_to_char("&+GThe Goddess Mielikki is aiding your health, and prevents the endurance spell from functioning", ch );
    return;
  }

  if( affected_by_spell(ch, SPELL_FALUZURES_VITALITY)
    && !GET_CLASS(victim, CLASS_BLIGHTER) )
  {
    send_to_char( "&+yFa&+Lluz&+yure refuses to help you further.&n\r\n", ch);
    return;
  }

  // 3 up to lvl 20 (4) ... 56 (13)
  skl_lvl = (int)(MAX(3, ((level / 4) - 1)) * get_property("spell.endurance.modifiers", 1.000));
  if( GET_SPEC(ch, CLASS_BLIGHTER, SPEC_SCOURGE) )
  {
    skl_lvl += 5;
  }

  if( (afp = get_spell_from_char(ch, SPELL_SAP_NATURE)) )
  {
    act( "&+yYour sapping &+Genergy&+y from your surroundings is refreshed.&n", FALSE, ch, 0, victim, TO_CHAR );
    afp->duration = skl_lvl;
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_SAP_NATURE;
  af.location = APPLY_MOVE_REG;
  af.duration = skl_lvl;
  af.modifier = skl_lvl;
  affect_to_char(victim, &af);

  if(ch == victim)
  {
    act( "&+yYou sap &+Genergy&+y from your surroundings.&n", FALSE, ch, 0, victim, TO_CHAR );
  }
  else
  {
    act("&+yYou feel &+Genergy&+y flow into you from your surroundings.&n", FALSE, ch, 0, victim, TO_VICT );
    act("&+yYour incantation saps &+Genergy&+y from your surroundings, sending it to $N&+y.&n",
      FALSE, ch, 0, victim, TO_CHAR);
  }
  act("&+GEnergy&+y from the area begins to drain into &+y$N&+y.&n",
    FALSE, ch, 0, victim, TO_NOTVICT);
}

void spell_bloodstone(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj)
{
  P_obj bloodstone;
  struct affected_type af, *afp;
  int duration = level * 4 * WAIT_MIN;

  if( !IS_ALIVE(ch) || IS_NPC(ch) )
  {
    return;
  }

  if( IS_ROOM(ch->in_room, ROOM_NO_TELEPORT)
    || world[ch->in_room].sector_type == SECT_OCEAN )
  {
    send_to_char("The powers of nature ignore your call for serenity.\n", ch);
    return;
  }

  if( afp = get_spell_from_char(ch, SPELL_BLOODSTONE) )
  {
    bloodstone = get_obj_in_list_num(real_object(433), world[afp->modifier].contents);
    if( bloodstone )
    {
      extract_obj(bloodstone, FALSE);
    }
    afp->modifier = ch->in_room;
  }
  else
  {
    memset(&af, 0, sizeof(af));
    af.type = SPELL_BLOODSTONE;
    af.flags = /*AFFTYPE_NOSHOW |*/ AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
    af.modifier = ch->in_room;
    af.duration = duration / PULSES_IN_TICK;

    affect_to_char(ch, &af);
  }

  bloodstone = read_object(real_object(433), REAL);

  if(!bloodstone)
  {
    logit(LOG_DEBUG, "spell_bloodstone: obj 433 not loadable.");
    return;
  }

  send_to_char("&+WA shimmering stone begins to take shape....\n"
    "&+YThe stone rises into the &+cair&+Y briefly, then shoots downward with amazing speed into the ground.\n"
    "&+yThe stone fades to a &+rblood red color.&n\n", ch);

  act("&+WA shimmering stone begins to take shape....\n"
    "&+YThe stone rises into the &+cair&+Y briefly, then shoots downward with amazing speed into the ground.\n"
    "&+yThe stone fades to a &+rblood red color.&n\n", FALSE, ch, 0, 0, TO_ROOM);

  set_obj_affected( bloodstone, duration, TAG_OBJ_DECAY, 0 );
  bloodstone->value[0] = GET_PID(ch);
  obj_to_room( bloodstone, ch->in_room );
}

