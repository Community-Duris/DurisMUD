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

void spell_thornskin(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af1;

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
  if(!affected_by_spell(victim, SPELL_THORNSKIN))
  {
    bzero(&af1, sizeof(af1));
    af1.type = SPELL_THORNSKIN;
    af1.duration =  25;
    af1.modifier = -1 * level / 2;
    af1.location = APPLY_AC;
    af1.bitvector5 = AFF5_THORNSKIN;

    affect_to_char(victim, &af1);
    act("&+y$n&+y's skin gains the toughness of dead plant life, &+Lthorns&+y and bramblees grow from $s skin!",
      FALSE, victim, 0, 0, TO_ROOM);
    act("&+yYour skin gains the toughness of dead plant life, &+Lthorns&+y and bramblees grow from your skin!",
      FALSE, victim, 0, 0, TO_CHAR);
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if(af1->type == SPELL_THORNSKIN)
      {
        af1->duration = 25;
      }
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

  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);

}

// Room spell.
void spell_desecrate_land(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct room_affect *raf;
  struct room_affect  af;

  if( !ch )
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
    af.duration = (GET_LEVEL(ch) * 4);
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

  if( affected_by_spell(victim, SPELL_CONTAGION) )
  {
    act("$N is sick enough.", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

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

  if(NewSaves(victim, SAVING_SPELL, 0))
  {
    dam /= 2;
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
  af.room_flags = NO_RECALL + NO_TELEPORT + NO_SUMMON + NO_GATE;
  af.ch = ch;
  affect_to_room(ch->in_room, &af);

}

void event_waves_fatigue(P_char ch, P_char victim, P_obj obj, void *data)
{
  int moves;

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  moves = dice( GET_LEVEL(ch)/2, 4 )/4;
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

// Target spell.
// Create 3 waves (2 events) that sap moves.
void spell_waves_fatigue(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(NewSaves(victim, SAVING_PARA, 0))
  {
    return;
  }

  event_waves_fatigue( ch, victim, NULL, NULL);
  add_event(event_waves_fatigue, PULSE_VIOLENCE/2, ch, victim, 0, 0, 0, 0);
  add_event(event_waves_fatigue, PULSE_VIOLENCE, ch, victim, 0, 0, 0, 0);
}

void event_acid_rain(P_char ch, P_char victim, P_obj obj, void *data)
{
  int    room = *((int *)data);
  P_char next;
  struct damage_messages messages = {
    "",
    "&+GYou are burned as the rain dissolves your skin.&n",
    "",
    "$N &+gmelts into a pile of &+GGOO&n ... $E is no more!",
    "&+GThe rain &+gconsuming your flesh devours you completely!",
    "$N &+gmelts into a pile of &+GGOO&n ... $E is no more!",
      0
  };

  if( !ch || !IS_ALIVE(ch) )
  {
    return;
  }
  if( world[room].people)
  {
    act("An awful &+Gburning rain&n continues to fall from the sky.",0, world[room].people, 0, 0, TO_ROOM);
    act("An awful &+Gburning rain&n continues to fall from the sky.",0, world[room].people, 0, 0, TO_CHAR);
  }

  for( victim = world[room].people; victim; victim = next )
  {
    next = victim->next_in_room;
    if( (victim != ch) && !saves_spell(victim, SAVING_SPELL) )
    {
      spell_damage(ch, victim, dice(GET_LEVEL(ch)/2, 6), SPLDAM_ACID, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);
    }
  }

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

  if( !OUTSIDE(ch) )
  {
    send_to_char( "Try again.. Outdoors next time!", ch );
    return;
  }

  send_to_char("The clouds above converge and turn &+Lpitch black&n, and suddenly an awful &+Gburning rain&n begins to fall from the sky.\n", ch );
  act("The clouds above converge and turn &+Lpitch black&n, and suddenly an awful &+Gburning rain&n begins to fall from the sky.",0, ch, 0, 0, TO_ROOM);

  for( int i = 1; i <= waves; i++ )
  {
    add_event(event_acid_rain, (i * PULSE_VIOLENCE)/2, ch, victim, 0, 0, &(ch->in_room), sizeof(ch->in_room));
  }
}

// Area spell.
// 1d6 damage (1d8 to water mentals/plants) per lvl.
void spell_horrid_wilting(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char next;
  int    dam = dice(level, 6);
  struct damage_messages messages = {
    "$N &+Lbegins to dry and wilt.&n",
    "&+LYou begin to dry out and wilt.&n",
    "$N &+Lbegins to dry and wilt.&n",
    "You suck all the water from $N.",
    "You are completely dried up.",
    "$N dries up completely and crumbles&n.",
      0
  };

  if( !ch || !IS_ALIVE(ch) )
  {
    return;
  }

  for( victim = world[ch->in_room].people; victim; victim = next )
  {
    next = victim->next_in_room;
    if( (victim != ch) )
    {
      if(!NewSaves(victim, SAVING_SPELL, 0))
      {
        if( GET_RACE(victim) != RACE_PLANT && GET_RACE(victim) != RACE_SLIME && GET_RACE(victim) != RACE_W_ELEMENTAL )
        {
          spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);
        }
        else
        {
          spell_damage(ch, victim, (dam*4)/3, SPLDAM_ACID, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);
        }
      }
      else
      {
        if( GET_RACE(victim) != RACE_PLANT && GET_RACE(victim) != RACE_SLIME && GET_RACE(victim) != RACE_W_ELEMENTAL )
        {
          spell_damage(ch, victim, dam/2, SPLDAM_ACID, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);
        }
        else
        {
          spell_damage(ch, victim, (dam*2)/3, SPLDAM_ACID, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);
        }
      }
    }
  }
}

// Summon spell: 1d4+2 shambling mounds.
void spell_shambler(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int count = dice( 1, 4 ) + 2;
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
    setup_pet(mob, ch, 1, PET_NOCASH | PET_NOAGGRO);
    add_follower(mob, ch);
  }
}

// Target Damage.
void spell_implosion(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam = dice(level * 3, 10);

  struct damage_messages messages = {
    "You begin to crush $N.",
    "$n begins to crush you.",
    "$N is crushed by $n.",
    "$N implodes!",
    "You implode!",
    "$N implodes!",
      0
  };

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(!NewSaves(victim, SAVING_SPELL, 0))
  {
    dam /= 2;
  }
  spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);

}

// Target Damage.
void spell_sandstorm(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam = dice(level * 3, 6);
  struct damage_messages messages = {
    "You molest $N with sand.",
    "$n sends sand at you!",
    "$N is beleaguered with sand by $n.",
    "$N chokes to death on the sand!",
    "You choke to death on the sand!",
    "$N chokes to death on the sand!",
      0
  };

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(!NewSaves(victim, SAVING_SPELL, 0))
  {
    dam /= 2;
  }
  spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);

}
// Target Damage.
void spell_firelance(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "As you complete your spell, you launch a &+Rmassive fi&+r&+Yel&+ran&+Rce&n at $N, &+rscorching&n $S skin!",
    "As $n completes $s spell, $e launches a &+Rmassive fi&+r&+Yel&+ran&+Rce&n at you, &+rscorching&n your skin!",
    "As $n completes $s spell, $e launches a &+Rmassive fi&+r&+Yel&+ran&+Rce&n at $N, &+rscorching&n $S skin!",
    "$N succumbs to your &+Rmassive fi&+r&+Yel&+ran&+Rce&n.",
    "You succumb to $n's &+Rmassive fi&+r&+Yel&+ran&+Rce&n.",
    "$N succumbs to $n's &+Rmassive fi&+r&+Yel&+ran&+Rce&n.",
      0
  };

  if( !ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(level > 50)
  {
    dam = dice(6 * MIN(20, (level / 2 + 1)), 9);
  }
  else
  {
    dam = dice(5 * MIN(20, (level / 2 + 1)), 9);
  }

  if(!NewSaves(victim, SAVING_SPELL, 1.5))
  {
    dam /= 2;
  }

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);

}
