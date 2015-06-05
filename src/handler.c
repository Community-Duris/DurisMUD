/*
 * ***************************************************************************
 *  file: handler.c                                          part of Duris
 *  usage: various routines for moving about objects/players.
 *  copyright  1990, 1991 - see 'license.doc' for complete information.
 *  copyright  1994, 1995 - sojourn systems ltd.
 *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "account.h"
#include "db.h"
#include "events.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "arena.h"
#include "justice.h"
#include "mm.h"
#include "weather.h"
#include "interp.h"
#include "damage.h"
#include "sql.h"
#include "vnum.obj.h"
#include "map.h"
#include "handler.h"
#include "ctf.h"

/*
 *
 * external variables
 */
extern Skill skills[];
extern P_char character_list;
extern P_char combat_list;
extern P_char dead_guys;
extern P_desc descriptor_list;
extern P_event current_event;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const struct race_names race_names_table[];
extern char *coin_names[];
extern char *coin_abbrev[];
extern const char *dirs[];
extern const struct stat_data stat_factor[];
extern int rev_dir[];
extern int top_of_world;
extern struct con_app_type con_app[];
extern struct dex_app_type dex_app[];
extern struct max_stat max_stats[];
extern const struct racial_data_type racial_data[];
extern struct zone_data *zone_table;
extern struct time_info_data time_info;
extern struct arena_data arena;
extern P_event event_list;
extern const int dam_cap_data[];
static char buf[MAX_INPUT_LENGTH];

void     send_to_arena(char *msg, int race);
extern void timedShutdown(P_char ch, P_char, P_obj, void *data);
void disarm_obj_events(P_obj obj, event_func func);
int map_view_distance(P_char ch, int room);

/*
 * called every 20 seconds, just loops through chars doing...stuff
 */
void generic_char_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char   i, i_next;
  int      n, x;
  int      dam;

  for( i = character_list; i; i = i_next )
  {
    i_next = i->next;

    /* A basic mob sanity check */
    if( IS_NPC(i) && !i->only.npc && !IS_MORPH(i) )
    {
      wizlog(AVATAR, "&=LRDanger! Mob without only.npc struct! Attempting to neutralize!");
      logit(LOG_DEBUG, "mob #%u (%s) without only.npc struct", GET_RNUM(i), i->player.long_descr);
      extract_char(i);
      continue;
    }

    if( !IS_BLOODLUST && has_innate(i, INNATE_VULN_SUN) )
    {
      sun_damage_check(i);
    }

    if( GET_CLASS(i, CLASS_DRUID) && (GET_LEVEL(i) > 30) && (IS_AFFECTED2(i, AFF2_POISONED)) )
    {
      if( poison_common_remove(i) )
      {
      	send_to_char("You neutralize the poison in your bloodstream!\r\n", i);
      }
    }

    // that wonderful god spell...
    if( affected_by_spell(i, SPELL_PLEASANTRY) )
    {
      pleasantry(i);
    }

    /* repair munged flyers/swimmers */
    if( i->specials.z_cord > 0 && !OUTSIDE(i) )
    {
      i->specials.z_cord = 0;
    }
    else if( i->specials.z_cord < 0 && !IS_WATER_ROOM(i->in_room) )
    {
      i->specials.z_cord = 0;
    }
    if( IS_SET(i->specials.affected_by3, AFF3_SWIMMING) && !IS_WATER_ROOM(i->in_room) )
    {
      REMOVE_BIT(i->specials.affected_by3, AFF3_SWIMMING);
    }

    /* keep taught/learned proper */
    if( IS_PC(i) && !IS_MORPH(i) )
    {
      for (n = FIRST_SKILL; n <= LAST_SKILL; n++)
      {
        if (i->only.pc->skills[n].taught < i->only.pc->skills[n].learned)
          i->only.pc->skills[n].learned = i->only.pc->skills[n].taught;
      }
    }

    /* light sources, et al */
    update_char_objects(i);

    /* since fights stop healing, lets make sure we restart it */
    if( GET_HIT(i) < GET_MAX_HIT(i) )
    {
      StartRegen(i, EVENT_HIT_REGEN);
    }
  }
  add_event(generic_char_event, 20 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
  //AddEvent(EVENT_SPECIAL, 20 * WAIT_SEC, TRUE, generic_char_event, 0);
}

void event_sundamage(P_char ch, P_char victim, P_obj obj, void *data);

void sun_damage_check(P_char ch)
{
  if( IS_NPC(ch) || IS_TRUSTED(ch) )
  {
    return;
  }

  if( !has_innate(ch, INNATE_VULN_SUN) || IS_AFFECTED4(ch, AFF4_GLOBE_OF_DARKNESS) )
  {
    return;
  }

  if( IS_SWAMP_ROOM(ch->in_room) || IS_FOREST_ROOM(ch->in_room) )
  {
    return;
  }

  if( !IS_SUNLIT(ch->in_room) || IS_TWILIGHT_ROOM(ch->in_room) )
  {
    return;
  }

  if( GET_HIT(ch) < 1 )
  {
    return;
  }

  switch( GET_RACE(ch) )
  {
    case RACE_TROLL:
      send_to_char("&+rArrgg! The &+Ysun&+r! It &+Rburnss&+r!!\r\n", ch); 
      break;
    case RACE_DROW:
      send_to_char("&+rThe cursed &+Ysun&+r of the surface world &+Rburns&+r into your skin!\r\n", ch);
      break;
    case RACE_VAMPIRE:
    case RACE_PVAMPIRE:
      send_to_char("&+yThe cursed &+Ysun &+ymakes your skin &+Lcrack and burn!\r\n", ch);
      break;
    default:
      send_to_char("&+rThe heat from the &+Ysun&+r saps your life away!\r\n", ch);
      break;
  }

  int dam = number(5,20);
  GET_HIT(ch) = MAX(1, GET_HIT(ch) - dam);

//  if( !get_scheduled(ch, event_sundamage) )
//   add_event(event_sundamage, 5, ch, 0, 0, 0, 0, 0);
}

void event_sundamage(P_char ch, P_char victim, P_obj obj, void *data)
{

  if( !IS_ALIVE(ch) || IS_NPC(ch) || IS_TRUSTED(ch) )
    return;

  if( !IS_SUNLIT(ch->in_room) || IS_TWILIGHT_ROOM(ch->in_room) )
    return;

  if( !has_innate(ch, INNATE_VULN_SUN) || IS_AFFECTED4(ch, AFF4_GLOBE_OF_DARKNESS) )
    return;

  if( GET_HIT(ch) < 2 )
    return;

  int dam = number(1, 8);

  switch( GET_RACE(ch) )
  {
    case RACE_TROLL:
      dam = (int) (dam * 1.5);
      break;
    case RACE_DROW:
      dam = (int) (dam * 1.75);
      break;
    case RACE_VAMPIRE:
    case RACE_PVAMPIRE:
      dam = (int) (dam * 2.25);
      break;
    default:
      break;
  }

  GET_HIT(ch) = MAX(1, GET_HIT(ch) - dam);

  if( IS_PC(ch) && ch->desc )
    ch->desc->prompt_mode = TRUE;
  P_char orig = ch->desc ? ch->desc->original : NULL;
  if( orig )
    orig->desc->prompt_mode = TRUE;

  if( GET_HIT(ch) > 1 )
    add_event(event_sundamage, 5, ch, 0, 0, 0, 0, 0);
}

/*
 * ok, currently, rooms are of fixed size, and a single torch is enough to
 * illuminate any room.  This, needless to say, sucks.  However, doing
 * anything about it will be hard as hell, so we live with it for now.
 * Light will work like this: 1- if there is a light source in the room,
 * room is lit. 2- if there is a 'dark' source in the room, room is pitch
 * black. 3- this is partially overruled by darkness not being permanent,
 * ever.
 *
 * You can cast darkness on an object, which will last until the item is
 * rented, or destroyed, or a god casts continual light on it, or it wears
 * off (darkness on an object starts an event).  You can cast it on a
 * char, and it sets an affect that will wear off, or be dispelled like
 * any other magic affect. You can cast it in a room, and it will last til
 * it wears off, or a continual light negates it.  The DARK flag on
 * objects is NEVER saved, which is why it wears off when rented.
 */

/*
 * recalculate (and return) the amount of light ch is emitting
 */

int char_light(P_char ch)
{
  P_obj    t_obj = NULL;
  int      i, amt = 0, dark = 0, mf_l;
  struct affected_type *af;

  if( !ch )
  {
    return -1;
  }

/* These are all handled elsewhere.  And fire elementals no longer light up the room.
  if( GET_RACE(ch) == RACE_F_ELEMENTAL )
  {
    amt += 3;
  }

  if (IS_AFFECTED4(ch, AFF4_MAGE_FLAME) ||
      IS_AFFECTED4(ch, AFF4_GLOBE_OF_DARKNESS))
  {
    mf_l = 0;

    // first, check if spell has been cast, and base level of light on
    //   that.  if not, assume artifact or whatnot and use vict level

    for (af = ch->affected; af; af = af->next)
    {
      if ((IS_AFFECTED4(ch, AFF4_MAGE_FLAME) ? (af->type == SPELL_MAGE_FLAME)
           : (af->type == SPELL_GLOBE_OF_DARKNESS)) && (af->modifier > 0))
      {
        mf_l = af->modifier / 10;
      }
    }

    if (!mf_l)
      mf_l = (GET_LEVEL(ch) / 10) + 3;

    if (IS_AFFECTED4(ch, AFF4_GLOBE_OF_DARKNESS))
      mf_l = -mf_l;

    amt += mf_l;

    amt = BOUNDED(-1, amt, 127);
  }
*/

  for( i = 0; i < MAX_WEAR; i++ )
  {
    if( ch->equipment[i] )
    {
      /* hands have a light that's not burnt out */
      if( ((i >= WIELD) && (i <= HOLD)) && (ch->equipment[i]->type == ITEM_LIGHT)
        && ch->equipment[i]->value[2] )
      {
        amt++;
      }
      else if( IS_SET(ch->equipment[i]->extra_flags, ITEM_LIT) )
      {
        amt++;
      }
    }
  }
  /* yup inven (surface layer anyway) counts */
  /* No more inventory.. too cheesy.
  for (t_obj = ch->carrying; !dark && t_obj; t_obj = t_obj->next_content)
  {
    if (IS_SET(t_obj->extra_flags, ITEM_LIT))
      amt += 3;
  }
  if (dark)
    amt = -1;
  */

  i = ch->light;
  ch->light = BOUNDED(-1, amt, 127);

  // If the ch changed the amount of light on them, change the room they're in too.
  if( ch->light != i )
  {
    room_light(ch->in_room, REAL);
  }

  return ch->light;
}

/*
 * recalculate (and return) the amount of light in a room
 */
// This is lightsources only now!
// Returns -1 on error, otherwise number of lit objects worn in room.
int room_light(int room_nr, int flag)
{
  P_char   t_ch = NULL;
  P_obj    t_obj = NULL;
  int      amt = 0, dark = 0, rroom = -1;

  if( room_nr < 0 )
  {
    return -1;
  }

  if( flag == REAL )
  {
    rroom = room_nr;
  }
  else if( room_nr < top_of_world )
  {
    rroom = real_room(room_nr);
  }
  else
  {
    return -1;
  }

  if( rroom == NOWHERE )
  {
    return -1;
  }

  amt = 0;

/* No.. surface maps are not always lit.. there's no magic torch in every room.
 * In fact, we're only counting light sources from equipment here now.
 * Sunlight/Fireplane/etc is handled elsewhere. - Lohrr
  if( IS_SURFACE_MAP(rroom) )
    amt += 1;

  //if (world[rroom].sector_type == SECT_INSIDE)
  //{
  if (IS_SET(world[rroom].room_flags, DARK))
    amt -= 2;

    if (IS_SET(world[rroom].room_flags, MAGIC_DARK ))
      amt -= 1;
    //else
      //amt++; // give them a little light
  //}
  //else if (IS_SET(world[rroom].room_flags, DARK))
    //amt--;

  //if (IS_SET(world[rroom].room_flags, MAGIC_DARK))
  //{
//    world[rroom].light = -1;
//    return -1;
    //amt = -1;
    //amt--;
  //}
  if (IS_SET(world[rroom].room_flags, MAGIC_LIGHT))
    amt += 4;

  if (world[rroom].sector_type == SECT_FIREPLANE)
    amt += 4;
  if (world[rroom].sector_type == SECT_UNDRWLD_LIQMITH)
    amt += 2;
  int dirty_loop_fix = 0;
*/

  // Add the number of lights on each person.
  for( t_ch = world[rroom].people; t_ch; t_ch = t_ch->next_in_room )
  {
    if( t_ch->light > 0 )
    {
      amt += t_ch->light;
    }
    if( t_ch == t_ch->next_in_room )
    {
      debug( "Buggy char '%s' in room list twice, room %d.", J_NAME(t_ch), rroom );
      break;
    }
  }

  /*
   * lit items in room count
   */
  for( t_obj = world[rroom].contents; t_obj; t_obj = t_obj->next_content )
  {
    if( IS_SET(t_obj->extra_flags, ITEM_LIT) )
    {
if( rroom == 59 ) debug( "t_obj: %s, lit", t_obj->short_description );
        amt++;
    }
    else if( (t_obj->type == ITEM_LIGHT) && (t_obj->value[2] == -1) )
    {
if( rroom == 59 ) debug( "t_obj: %s, light", t_obj->short_description );
        amt++;
    }
  }

  /*
   * have to do something about ambient (sun) light, not sure what yet
   */
/*
  if (dark)
    amt = BOUNDED(-1, amt, 1);
*/

  world[rroom].light = BOUNDED(-1, amt, 127);

#if 0
  if (world[rroom].people && !ALONE(world[rroom].people))
  {
    /*
     * room just lit up, let's give the mobs a chance to jump things,
     * this will mostly negate the gra torch/look/rem torch strategy,
     * as mobs will tend to jump you when you light things up.
     * MuHAHAHAHAHA!  JAB
     */

    LOOP_THRU_PEOPLE(t_ch, world[rroom].people)
    {
      if (IS_NPC(t_ch) && AWAKE(t_ch) && !IS_DESTROYING(t_ch) && !IS_FIGHTING(t_ch) &&
          MIN_POS(t_ch, POS_STANDING + STAT_NORMAL) &&
          (victim = PickTarget(t_ch)) && is_aggr_to(t_ch, victim))
        AddEvent(EVENT_AGG_ATTACK, number(1, 5), TRUE, t_ch, victim);
    }
  }
#endif

  return world[rroom].light;
}

/*
 * common initial setup for poison
 */

 // common function for removing all poisons
int poison_common_remove(P_char ch)
{
	bool tmp = FALSE;
	
  if (ch)
  {
    struct affected_type *af, *next;

    if (IS_SET(ch->specials.affected_by2, AFF2_POISONED))
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_POISONED);
      tmp = TRUE;
    }
    if (affected_by_spell(ch, SPELL_POISON))
    {
      affect_from_char(ch, SPELL_POISON);
      tmp = TRUE;
    }
    for (af = ch->affected; af; af = next)
    {
      next = af->next;
      if (af->bitvector2 & AFF2_POISONED)
      {
        affect_remove(ch, af);
        tmp = TRUE;
      }
    }
  }
	return tmp;
}

struct affected_type *poison_common(P_char victim, short int type)
{
  struct affected_type af;

  if (type < FIRST_POISON || type > LAST_POISON)
    type = FIRST_POISON;

  if (affected_by_spell(victim, (int)type))
    return NULL;

  memset(&af, 0, sizeof(af));
  af.type = type;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
  af.bitvector2 = AFF2_POISONED;
  af.duration = (int) get_property("poison.maxDuration.min", 15);
  return affect_to_char(victim, &af);
}

/*
 * check if poison didn't wear off since last check,
 * if not just call poison function
 */
void event_poison(P_char ch, P_char attacker, P_obj obj, void *data)
{
  short int      type = *((short int *) data);
  struct affected_type *afp;

  for (afp = ch->affected; afp; afp = afp->next)
    if (afp->type == type)
      break;

  if (!afp)
    return;
  if (affected_by_spell(ch, SPELL_AID) && !(number(0,4)))
  {
    affect_remove(ch, afp);
    send_to_char("&+WThe power of nature has managed to eliminate the poison coursing through your veins!\n", ch);
  }
  else
    (skills[type].spell_pointer) (0, attacker, 0, 0, ch, (P_obj) afp);
}

/*
 * lose 10 hitpoints every 20 seconds
 */
void poison_lifeleak(int level, P_char ch, char *arg, int type, P_char victim,
                     struct affected_type *af)
{
  if (!af) {
    if (!(af = poison_common(victim, POISON_LIFELEAK)))
      return;
  }
  else
  {
    send_to_char("Ye feel a burning sensation in yer blood.\n", victim);
    GET_HIT(victim) = MAX(1, GET_HIT(victim) - 10);
  }
  if (number(0, 20))
    add_event(event_poison,
        WAIT_SEC * (IS_AFFECTED(victim, AFF_SLOW_POISON) ? 60 : 20),
        victim, 0, 0, 0, &af->type, sizeof(af->type));
  else
    affect_remove(victim, af);
}

/*
 * take once 40ish damage a while after being poisoned
 * can frag with this poison
 */
void poison_heart_toxin(int level, P_char ch, char *arg, int type,
                        P_char victim, struct affected_type *af)
{
  struct damage_messages messages = {
    "$N suddenly turns &+ggreen &nas your poison reaches $S &+Wvital &norgans.",
    "You suddenly feel &+gsick &nas $n's poison reaches your &+Wvital &norgans.",
    "$N suddenly turns &+ggreen &nas poison reaches $S &+Wvital &norgans.",
    "$N suddenly turns &+Ggreen &nholds $S throat and vomits &+Rblood &nas $S soul leaves the body forever.",
    "A &+gsickening &nwave going up your throat is the last thing you feel..",
    "$N suddenly turns &+Ggreen &nholds $S throat and vomits &+Rblood &nas $S soul leaves the body forever.",
  };

  if (ch)
    level = GET_LEVEL(ch);

  if (!af)
  {
    if (!(af = poison_common(victim, POISON_HEART_TOXIN)))
      return;;
    add_event(event_poison,
              IS_AFFECTED(victim,
                          AFF_SLOW_POISON) ? 3 *
              PULSE_VIOLENCE : PULSE_VIOLENCE * 3 / 2, victim, ch, 0, 0,
              &af->type, sizeof(af->type));
  }
  else
  {
    affect_remove(victim, af);
    if (!ch)
      ch = victim;
    raw_damage(ch, victim, 3 * level + number(0, 40), RAWDAM_DEFAULT, &messages);
  }
}

/*
 * get frozen for 1.5 seconds every half minute
 */
void poison_neurotoxin(int level, P_char ch, char *arg, int type,
                       P_char victim, struct affected_type *af)
{
  if (!af)
  {
    if (!(af = poison_common(victim, POISON_NEUROTOXIN)))
      return;
    af->duration = 1;
  }
  else
  {
    send_to_char
      ("Your muscles contract suddenly as toxin attacks your neural system.\n",
       victim);
    CharWait(victim, PULSE_VIOLENCE / 2);
  }
  if (number(0, 20))
    add_event(event_poison,
              WAIT_SEC * (IS_AFFECTED(victim, AFF_SLOW_POISON) ? 60 : 20),
              victim, 0, 0, 0, &af->type, sizeof(af->type));
  else
    affect_remove(victim, af);
}

/*
 * lose strength, lost value changes randomly from 5 to 20 every 2 minutes
 */
void poison_weakness(int level, P_char ch, char *arg, int type, P_char victim,
                     struct affected_type *af)
{
  if (!af)
  {
    if (!(af = poison_common(victim, POISON_WEAKNESS)))
      return;
    af->location = APPLY_STR;
  }
  af->modifier = -1 * number(5, IS_AFFECTED(victim, AFF_SLOW_POISON) ? 8 : 20);
  balance_affects(victim);

  if (number(0, 100))
  {
    send_to_char
      ("Ye feel a wave of strange weakness running through yer body.\n",
       victim);
    add_event(event_poison, WAIT_SEC * 120, victim, 0, 0, 0, &af->type,
              sizeof(af->type));
  }
  else
  {
    send_to_char("Ye feel at yer full strength again.\n", victim);
    affect_remove(victim, af);
  }
}

char    *FirstWord(char *namelist)
{
  static char holder[30];
  register char *point;

  if (!namelist)
    return (NULL);

  while (*namelist == ' ')
    namelist++;
  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

/*
bool isname( const char *match, const char *namelist )
{
        char match_copy[ MAX_STRING_LENGTH ];
        char word[ MAX_STRING_LENGTH ];
        char exp_namelist[ MAX_STRING_LENGTH ];
        char exp_word[ MAX_STRING_LENGTH ];
        char *p, *str;

        if ( !match || !namelist ) return FALSE;
        strcpy( match_copy, match );
        str = match_copy;
        while ( *str == ' ' ) str++;
        if ( !*str ) return FALSE;
        sprintf(exp_namelist, " %s ", namelist );

        // Ok str now points to begining of match's copy
        for (;;) {
                p = strchr( str, '.' );
                if ( p ) *p++ = '\0';
                strcpy( word, str );
                str = p; // If p == NULL, it still works as side effect

                // Now we check if word exists in namelist
                // Best solution would be splitting namelist and store each
                // word in a hash, however lacking hashes we do an strstr
                sprintf( exp_word, " %s ", word );
                if ( !strstr( exp_namelist, exp_word ) ) return FALSE;
        }
        return TRUE;
}
*/


bool isname(const char *str, const char *namelist)
{
  int      i = 0, j = 0, k = 0, k2 = 0;
  char     tstr[MAX_STRING_LENGTH];

  if (!str || !namelist)
    return FALSE;

  // eat leading spaces in str
  while (*(str + k) == ' ')
    k++;

  if (!*(str + k))
    return FALSE;

  // lowercase the search string now, so we don't have to do it over and

  for (k2 = 0; *(str + k); k++)
    tstr[k2++] = LOWER(*(str + k));
  tstr[k2] = 0;

  for (;;)
  {
    for (i = 0;; i++, j++)
    {
      if (!tstr[i])
      {
        if (!*(namelist + j) || (*(namelist + j) == ' '))
          return TRUE;
        break;
      }
      else
      {
        if (!*(namelist + j))
          return FALSE;

        if (tstr[i] != LOWER(*(namelist + j)))
          break;
      }
    }

    // skip to next name
    for (; *(namelist + j) && (*(namelist + j) != ' '); j++) ;
    if (!*(namelist + j))
      return FALSE;
    j++;                        // first char of new name
  }
}

/*
 * move a player out of a room
 */

void char_from_room(P_char ch)
{
  P_char   i;

  if(!(ch))
  {
    return;
  }

  if (ch->in_room == NOWHERE)
  {
    return;
  }
/*  
  logit(LOG_DEBUG, "call to char_from_room() when already NOWHERE (%s)", GET_NAME(ch));
    
    if(IS_NPC(ch))
    {
      logit(LOG_DEBUG, "continued: call to char_from_room() mob vnum (%d)",
        mob_index[GET_RNUM(ch)].virtual_number);
    }
    if(IS_PC(ch))
    {
      logit(LOG_DEBUG, "continued: call to char_from_room() PC name (%s)",
        GET_NAME(ch));
    }
    
    return;
  }
*/
  DelCharFromZone(ch);

  if (IS_AFFECTED2(ch, AFF2_CASTING))
    StopCasting(ch);

  char_light(ch);
  room_light(ch->in_room, REAL);

  if (GET_HIT(ch) <= (GET_MAX_HIT(ch) / 4))
    make_bloodstain(ch);

  if (ch == world[ch->in_room].people)  /* head of list */
    world[ch->in_room].people = ch->next_in_room;
  else
  {
    /* locate the previous element */
    for (i = world[ch->in_room].people;
         i && (i->next_in_room != ch); i = i->next_in_room) ;

    if (!i)
    {
      logit(LOG_DEBUG, "called char_from_room, char not in room list");
      return;
    }
    i->next_in_room = ch->next_in_room;
  }
	

#if 0
  if (IS_BEING_SHADOWED(ch))
    ch->specials.shadow.room_last_in = ch->in_room;
#endif

  /* GLD - as well as a callback on entering, also make a callback when exiting... */
  /* while this doesn't check the return value, it does allow some rooms (such as
     storage lockers) to close things out properly */
  if (world[ch->in_room].funct)
    (*world[ch->in_room].funct) (ch->in_room, ch, (-75), NULL);


  ch->specials.was_in_room = world[ch->in_room].number;
  ch->in_room = NOWHERE;
  ch->next_in_room = 0;

}

/*
 * place a character in a room.
 */
// Returns TRUE iff char made it into the room.
bool char_to_room(P_char ch, int room, int dir)
{
  P_char   t_ch, k, who;
  P_desc   d;
  char     j, exit1 = -1, exit2 = -1, exit3 = -1;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     temp_buffer[MAX_STRING_LENGTH];
  int      was_in, current, total_coins, x, worked = FALSE, was_in_arena;
  struct zone_data *zone = 0;
  P_room   rm = 0;
  struct group_list *gl;
  int      x_distance = 0;
  int      y_distance = 0;

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if (room < 0)
  {
    if( IS_NPC(ch) )
    {
      logit(LOG_DEBUG, "char_to_room: trying to move %s (%d) to room %d.", J_NAME(ch), GET_VNUM(ch), room );
      extract_char(ch);
      ch = NULL;
      return FALSE;
    }
    // Move them to Limbo!
    room = 0;
    logit(LOG_DEBUG, "char_to_room: trying to move %s to room < 0", GET_NAME(ch));
    wizlog(AVATAR, "char_to_room: trying to move %s to room < 0", GET_NAME(ch));
  }

  /* this is a serious error, but let's just try and live with it */

/*
  if (ch->in_room != NOWHERE)
  {
    logit(LOG_DEBUG,
          "char_to_room: char is not in NOWHERE (%s, room rnum #%d [vnum %d], trying to move to rnum %d [vnum %d])",
          J_NAME(ch), ch->in_room, world[ch->in_room].number, room,
          world[room].number);
  }
*/
  
#if 0
  if (IS_BEING_SHADOWED(ch) && (dir > -1))
  {
    MoveShadower(ch, room);
  }
#endif

  if (!IS_SET(world[room].room_flags, SINGLE_FILE))
  {
    ch->next_in_room = world[room].people;
    world[room].people = ch;
  }
  else
  {
    for (j = 0; j < NUM_EXITS; j++)
      if (world[room].dir_option[j])
        if (exit1 == -1)
          exit1 = j;
        else if (exit2 == -1)
          exit2 = j;
        else
          exit3 = j;

    if ((exit1 == -1) || (exit2 == -1))
    {
      REMOVE_BIT(world[room].room_flags, SINGLE_FILE);
      exit1 = -1;               /* will cause normal behavior */
    }
    if (exit3 != -1)
    {
      REMOVE_BIT(world[room].room_flags, SINGLE_FILE);
      exit1 = -1;               /* will cause normal behavior */
    }
    if ((exit1 == -1) || (exit1 == rev_dir[(dir < 0) ? 0 : dir]))
    {
      ch->next_in_room = world[room].people;
      world[room].people = ch;
    }
    else
    {
      if (!(k = world[room].people))
        world[room].people = ch;
      else
      {
        while (k->next_in_room)
          k = k->next_in_room;
        k->next_in_room = ch;
        ch->next_in_room = 0;
      }
    }
  }

  was_in = real_room0(ch->specials.was_in_room);
  was_in_arena = world[was_in].room_flags & ARENA;

  ch->in_room = room;

  if ((was_in_arena != (world[room].room_flags & ARENA)) && !IS_TRUSTED(ch))
  {
    if (was_in_arena && IS_SET(arena.flags, FLAG_SEENAME))
    {
      sprintf(buf, "%s has left the arena.\r\n", GET_NAME(ch));
      send_to_arena(buf, -1);
//      broadcast_to_arena("%s has left the arena.\r\n", ch, 0, was_in);
    }
    else if (IS_SET(arena.flags, FLAG_SEENAME))
    {
      sprintf(buf, "%s has entered the arena.\r\n", GET_NAME(ch));
      send_to_arena(buf, -1);
//      broadcast_to_arena("%s has entered the arena.\r\n", ch, 0, room);
    }
  }



  for (gl = ch->group; gl; gl = gl->next)
  {
    if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
    {
      if (ch->in_room == gl->ch->in_room)
        gl->ch->desc->last_group_update = 1;
    }
  }

  // if anything has moved on the map, find everyone who can see them
  // and set their flag to send a map update
  if (IS_MAP_ROOM(was_in) || IS_MAP_ROOM(ch->in_room))
  {
    for (d = descriptor_list; d; d = d->next)
    {
      who = d->character;
      if (who && who->desc && who->desc->term_type == TERM_MSP &&
          IS_MAP_ROOM(who->in_room))
      {
        if (!CAN_SEE_Z_CORD(who, ch))
          continue;
        if(!IS_MAP_ROOM(ch->in_room))
          continue;
        if (ch == who)
          continue;
        if (who->desc->last_map_update) //performance saving!
          continue;

        int dist = calculate_map_distance(ch->in_room, who->in_room);
        int view_dist = map_view_distance(who, who->in_room);

        if ( dist >= 0 && dist <= (view_dist*view_dist) ) 
        {
          who->desc->last_map_update = 1;
          continue;
        }
      }
    }
  }

  if (ch && ch->desc && ch->desc->term_type == TERM_MSP)
  {
    if (!(IS_MAP_ROOM(ch->in_room)) && !(IS_SHIP_ROOM(ch->in_room)))
    {
      send_to_char("\n<map>\n", ch);
      rm = &world[ch->in_room];
      zone = &zone_table[world[ch->in_room].zone];
      sprintf(temp_buffer, "&+WZone: %s&n.\n&+WRoom: %s", zone->name, rm->name);
      send_to_char(temp_buffer, ch);
      send_to_char("\n</map>\n", ch);
      ch->desc->last_map_update = 0;
    }
  }

  AddCharToZone(ch);

  if ((t_ch = get_linked_char(ch, LNK_RIDING)) &&
      t_ch->in_room != ch->in_room)
  {
    char_from_room(t_ch);
    char_to_room(t_ch, ch->in_room, dir);
  }

  if ((t_ch = get_linking_char(ch, LNK_RIDING)) &&
      t_ch->in_room != ch->in_room)
  {
    char_from_room(t_ch);
    char_to_room(t_ch, ch->in_room, dir);
  }

  /*
   * ok, since running battles aren't allowed, if they get here and are
   * still fighting, they either got yanked out of combat or this is a
   * do_at() call.  we check for the do_at() call, and stop them from
   * fighting if it's not a do_at() call (farsee spell uses do_at())
   */

  if (ch->specials.fighting && (dir >= 0))
    stop_fighting(ch);
  if( IS_DESTROYING(ch) && (dir >= 0) )
    stop_destroying(ch);

  char_light(ch);
  room_light(ch->in_room, REAL);

  if (dir != -2)
    do_look(ch, 0, -4);

  if (dir < 0)                  /* flag value, skip aggro checks */
  {
    return TRUE;
  }

  /*
   * new, room specials get checked when chars enter, ignores return,
   * but does some checking before it continues. JAB
   */

  if(GET_STAT(ch) == STAT_DEAD)
  {
    if( ch->in_room == room )
    {
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }

  // Room procs.
  if (world[ch->in_room].funct)
  {
    (*world[ch->in_room].funct) (ch->in_room, ch, (-50 + dir), NULL);
    // Room proc killed them.
    if( !char_in_list(ch) || (room != ch->in_room) || (GET_STAT(ch) == STAT_DEAD) )
    {
      return FALSE;
    }
  }
  if( (world[room].sector_type == SECT_FIREPLANE)
    || (world[room].sector_type == SECT_LAVA)
    || (world[room].sector_type == SECT_UNDRWLD_LIQMITH) )
  {
    firesector(ch);
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
  }
  if ((world[room].sector_type == SECT_NEG_PLANE))
  {
    negsector(ch);
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
  }

  /* underwater stuff */
  if( IS_SET(world[ch->in_room].room_flags, UNDERWATER) || ch->specials.z_cord < 0 )
  {
    underwatersector(ch);
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
  }

  if( !IS_SET(world[ch->in_room].room_flags, UNDERWATER) && IS_AFFECTED2(ch, AFF2_HOLDING_BREATH) )
  {
    REMOVE_BIT(ch->specials.affected_by2, AFF2_HOLDING_BREATH);
  }

  if( (world[(ch)->in_room].sector_type == SECT_NO_GROUND) ||
      (world[(ch)->in_room].sector_type == SECT_UNDRWLD_NOGROUND) ||
      (ch->specials.z_cord > 0) )
  {
    if( char_falling(ch) )
    {
      // FALSE -> can't kill char, FALSE -> This is not an event.
      if( falling_char(ch, FALSE, FALSE) )
      {
        if( ch->in_room != room )
        {
        return FALSE;
        }
      }
    }
  }

  /* repair munged flyers/swimmers */
  if (ch->specials.z_cord > 0 && !OUTSIDE(ch))
  {
    send_to_char("You land your flight.\r\n", ch);
    ch->specials.z_cord = 0;
  }
  else if (ch->specials.z_cord < 0 && !IS_WATER_ROOM(ch->in_room))
  {
    send_to_char("You leave the waters.\r\n", ch);
    ch->specials.z_cord = 0;
    if (GET_VITALITY(ch) != GET_MAX_VITALITY(ch))
      StartRegen(ch, EVENT_MOVE_REGEN);
  }
  /* Underwater stuff */
  if (IS_SET(world[room].room_flags, UNDERWATER) || ch->specials.z_cord < 0)
  {
    if (!IS_AFFECTED2(ch, AFF2_IS_DROWNING) &&
        !IS_AFFECTED2(ch, AFF2_HOLDING_BREATH))
      underwatersector(ch);
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
  }
  if (IS_SET(ch->specials.affected_by3, AFF3_SWIMMING) &&
      ch->specials.z_cord < 1 && IS_WATER_ROOM(ch->in_room))
  {
    swimming_char(ch);
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
  }
/*
  if (!IS_SET(ch->specials.affected_by3, AFF3_SWIMMING) && dir > -1 &&
        ch->specials.z_cord < 1 && IS_WATER_ROOM(ch->in_room) && !IS_WATER_ROOM(was_in))
    send_to_char("You jump into the waters...\r\n", ch);
*/

  if (world[ch->in_room].current_speed && !IS_TRUSTED(ch))
    if (IS_WATER_ROOM(ch->in_room))
      if (number(1, 101) < world[ch->in_room].current_speed)
      {
        current = world[ch->in_room].current_direction;
        if (CAN_GO(ch, current))
          if (IS_WATER_ROOM(ch->in_room) && !IS_AFFECTED(ch, AFF_FLY)
              && !IS_AFFECTED(ch, AFF_LEVITATE))
          {
            send_to_char("The current sweeps you away!\r\n", ch);
            do_move(ch, 0, exitnumb_to_cmd(current));
            if( !IS_ALIVE(ch) )
            {
              return FALSE;
            }
          }
      }
  /* too much money in your hands? Didn't have it in a bag? shame...  */
  total_coins = GET_COPPER(ch) + GET_SILVER(ch) + GET_GOLD(ch) + GET_PLATINUM(ch);
  if (total_coins > 200 && !IS_TRUSTED(ch))
  {
    do
    {
      x = number(0, 3);
      if (ch->points.cash[x] >= 40)
      {
        sprintf(Gbuf1, "%d %s", number(5, 40), coin_abbrev[x]);
        do_drop(ch, Gbuf1, 1);
        worked = TRUE;
      }
    }
    while (!worked);
  }
  /*
   * justice hook
   */
  if (IS_INVADER(ch) || IS_OUTCAST(ch))
  {
    justice_action_invader(ch);
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
  }

  // Purge NPCs from safe rooms? ok...
  if( IS_SET(world[room].room_flags, SAFE_ZONE) )
  {
    if( IS_NPC(ch) )
    {
      extract_char(ch);
      ch = NULL;
      return FALSE;
    }
    return TRUE;
  }
  if( IS_MAP_ROOM(ch->in_room) && IS_PC(ch) && !IS_TRUSTED(ch) )
  {
    /*random_encounters(ch);
       check_for_kingdom_trespassing(ch); */
  }

  if( ALONE(ch) )
  {
    return TRUE;
  }

  // This check is important because we want to make sure ch isn't a newly created mob.
  if( was_in > 0 )
  {
    // If you comment out the return, you need to change this loop to handle deaths.
    for( k = world[room].people; k; k = k->next_in_room )
    {
      // Skip PCs and NPCs with no proc
      if( !IS_NPC(k) || mob_index[GET_RNUM(k)].func.mob == NULL )
      {
        continue;
      }
      if( (*mob_index[GET_RNUM(k)].func.mob) (k, ch, CMD_TOROOM, NULL) )
      {
        // Can comment out this return if we want to allow multiple procs upon entering room.
        //   If you do this, make sure IS_ALIVE(ch) and k and such...
        if( IS_ALIVE(ch) && ch->in_room == room )
          return TRUE;
        else
          return FALSE;
      }
    }
  }

  /*
   * check char entering room for an agg (auto) attack on occupants.
   */
  t_ch = PickTarget(ch);

  /*
   * delay is 0 to 7, 0 to 5 for avg. dex, 0 to 4 for 18, 0 to 1 for
   * 23+, that's pulses, so at most, delay < 2 seconds.  delays for
   * people in the room are slightly longer.  instant attacks are now a
   * thing of the past.
   */

      int nocalming = 0;
      int calming = 0;
      if ((RACE_EVIL(ch) && IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_GOODHOME)) ||
          (RACE_GOOD(ch) && IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_EVILHOME)))
      nocalming = 1;

      if(t_ch && !IS_ELITE(t_ch) && !nocalming &&
	 (((GET_LEVEL(t_ch) - GET_LEVEL(ch)) <= 5) || !number(0, 3)) &&
	 has_innate(ch, INNATE_CALMING))
        calming = (int)get_property("innate.calming.delay", 10);

  if( t_ch && is_aggr_to(ch, t_ch) )
  {
    add_event(event_agg_attack, number(0, MAX(0, (11 - dex_app[STAT_INDEX(GET_C_DEX(ch))].reaction) / 2)) + calming,
      ch, t_ch, 0, 0, 0, 0);
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
  }

  for (t_ch = world[ch->in_room].people; t_ch; t_ch = t_ch->next_in_room)
  {
    // Probably won't die from a social, but you never know.
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
    if( !IS_ALIVE(t_ch) )
    {
      break;
    }

    if(t_ch == ch)
      continue;

    if(IS_PC(t_ch))
      continue;

    if(GET_LEVEL(ch) >= 56 &&
      !is_aggr_to(ch, t_ch) &&
      CAN_SEE(t_ch, ch) &&
      !IS_IMMOBILE(t_ch) &&
      !IS_ELITE(t_ch))
    {
      if(GET_RACE(ch) == GET_RACE(t_ch))
      {
        switch (number(0, 500))
        {
        case 0:
          do_action(t_ch, GET_NAME(ch), CMD_BOW);
          break;
        case 1:
        do_action(t_ch, GET_NAME(ch), CMD_KNEEL);
          break;
        case 2:
          do_action(t_ch, GET_NAME(ch), CMD_APPLAUD);
          break;
        case 3:
          do_action(t_ch, GET_NAME(ch), CMD_SMILE);
          break;
        case 4:
          do_action(t_ch, GET_NAME(ch), CMD_WORSHIP);
          break;
        case 5:
          do_action(t_ch, GET_NAME(ch), CMD_POINT);
          break;
        case 6:
          do_action(t_ch, GET_NAME(ch), CMD_GASP);
          break;
        case 7:
          do_action(t_ch, GET_NAME(ch), CMD_CHEER);
          break;
        default:
          break;
        }

        if(GET_SEX(t_ch) != GET_SEX(ch))
          if(!number(0, 250)){
            do_action(t_ch, GET_NAME(ch), CMD_BLUSH);
            P_obj  flow = read_object(6107, VIRTUAL);
            obj_to_char(flow, t_ch);
            char     text[MAX_STRING_LENGTH];
            sprintf(text, "rose %s", GET_NAME(ch) );
            do_give(t_ch, text, CMD_GIVE);
          }
      }
    }
  }
  return TRUE;
}

/*
 * give an object to a char
 */

void obj_to_char(P_obj object, P_char ch)
{
  P_obj    o;
  char Gbuf[MAX_STRING_LENGTH];

  if (!ch)
  {
    logit(LOG_MOB, "call to obj_to_char with no ch");
    return;
  }
  
  if(!object)
  {
    if(IS_NPC(ch))
    {
      logit(LOG_OBJ, "call to obj_to_char with no obj: mob (%d).", GET_VNUM(ch));
    }
    else
    {
      logit(LOG_OBJ, "call to obj_to_char with no obj: player (%s).", GET_NAME(ch));
    }
    return;
  }
  
  if (!OBJ_NOWHERE(object))
  {
    logit(LOG_DEBUG /*DEBUG*/, "wonders never cease, obj not in NOWHERE");
    return;
/*
    act("&+gWith a scurry, bugs appear from nowhere, engulfing $p.", TRUE, ch, object, 0, TO_ROOM);
    extract_obj(object, TRUE);
    return;
*/
  }
  
  if (IS_OBJ_STAT2(object, ITEM2_CRUMBLELOOT) && IS_PC(ch) && !IS_TRUSTED(ch))
  {
    if (ch->in_room)
    {
      sprintf(Gbuf, "&+LThe magic within %s &+Lfades causing it to crumble to dust.\r\n", object->short_description);
      send_to_room(Gbuf, ch->in_room);
    }
    extract_obj(object, TRUE);
    object = NULL;
    return;
  }

  if (ch->carrying && (ch->carrying->R_num == object->R_num))
  {
    object->next_content = ch->carrying;
    ch->carrying = object;
  }
  else
  {
    o = ch->carrying;
    while (o)
    {
      if (o->next_content && (o->next_content->R_num == object->R_num))
      {
        object->next_content = o->next_content;
        o->next_content = object;
        break;
      }
      else
        o = o->next_content;
    }
    if (!o)
    {
      object->next_content = ch->carrying;
      ch->carrying = object;
    }
  }

  if (IS_ARTIFACT(object))
    add_owned_artifact(object, ch, object->timer[3]);

  if (IS_SET(object->extra_flags, ITEM_LIT) ||
      ((object->type == ITEM_LIGHT) && (object->value[2] == -1)))
  {
    char_light(ch);
    room_light(ch->in_room, REAL);
  }
  object->loc_p = LOC_CARRIED;
  object->loc.carrying = ch;
  object->z_cord = 0;
  GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(ch)++;

if(object->g_key == 0 && IS_PC(ch) && GET_LEVEL(ch) < 57 && GET_PID(ch) < 10000000)
  {
   object->g_key = 1;
  }
}

/*
 * take an object from a char
 */

void obj_from_char(P_obj object, int gone_for_good)
{
  P_obj    tmp;
  P_char   ch;

  if (!OBJ_CARRIED(object) || !object->loc.carrying)
  {
    logit(LOG_EXIT, "obj not carried in obj_from_char");
    raise(SIGSEGV);
  }
  if (object->loc.carrying->carrying == object) /* head of list */
    object->loc.carrying->carrying = object->next_content;
  else
  {
    for (tmp = object->loc.carrying->carrying; tmp && (tmp->next_content != object); tmp = tmp->next_content) ; /* locate previous */

    tmp->next_content = object->next_content;
  }

  ch = object->loc.carrying;


  if (IS_ARTIFACT(object) && gone_for_good)
    remove_owned_artifact(object, object->loc.carrying, TRUE);

  if (IS_SET(object->extra_flags, ITEM_LIT) ||
      ((object->type == ITEM_LIGHT) && (object->value[2] == -1)))
  {
    char_light(object->loc.carrying);
    room_light((object->loc.carrying)->in_room, REAL);
  }
  GET_CARRYING_W(object->loc.carrying) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->loc.carrying)--;
  object->z_cord = object->loc.carrying->specials.z_cord;
  object->loc_p = LOC_NOWHERE;
  object->loc.room = 0;
  object->next_content = NULL;
}

void money_to_inventory(P_char ch)
{
  if( GET_MONEY(ch) <= 0 )
    return;
  
  /* make a 'pile of coins' object to hold ch's cash */
  GET_COPPER(ch) = BOUNDED(0, GET_COPPER(ch), 32000);
  GET_SILVER(ch) = BOUNDED(0, GET_SILVER(ch), 32000);
  GET_GOLD(ch) = BOUNDED(0, GET_GOLD(ch), 32000);
  GET_PLATINUM(ch) = BOUNDED(0, GET_PLATINUM(ch), 32000);
  
  P_obj money =
    create_money(GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch),
                 GET_PLATINUM(ch));
  
  SUB_MONEY(ch, GET_MONEY(ch), 0);
  
  obj_to_char(money, ch); 
}

void equip_char(P_char ch, P_obj obj, int pos, int nodrop)
{
  struct affected_type af;
  struct obj_affect *o_af;

  if (!(ch && obj && (pos >= 0) && (pos < MAX_WEAR) && !ch->equipment[pos]))
  {
    logit(LOG_EXIT, "assert: bogus args in equip_char");
    raise(SIGSEGV);
  }
  if (!OBJ_NOWHERE(obj))
  {
    logit(LOG_DEBUG,
          "and now for something completely different, obj not in NOWHERE");
    return;
  }

  if (IS_ARTIFACT(obj))
    add_owned_artifact(obj, ch, obj->timer[3]);

  ch->equipment[pos] = obj;
  obj->loc.wearing = ch;
  obj->loc_p = LOC_WORN;

  if (IS_PC(ch) && GET_ITEM_TYPE(ch->equipment[pos]) == ITEM_ARMOR)
    ch->only.pc->prestige += obj->value[2];
  balance_affects(ch);

  /*
   * light works though
   */
  if (IS_SET(obj->extra_flags, ITEM_LIT) ||
      ((obj->type == ITEM_LIGHT) && obj->value[2]))
  {
    char_light(ch);
    room_light(ch->in_room, REAL);
  }
  GET_CARRYING_W(ch) += (GET_OBJ_WEIGHT(obj) / 2);

  if (nodrop != 9)
    if (obj && (o_af = get_obj_affect(obj, SKILL_ENCHANT)))
    {
      act("&+YA magical aura forms around your body.&n", FALSE, ch, obj, 0,
          TO_CHAR);
      ((*skills[o_af->data].spell_pointer) ((int) GET_LEVEL(ch), ch, 0,
                                            SPELL_TYPE_SPELL, ch, 0));
    }
}

P_obj unequip_char(P_char ch, int pos)
{
  P_obj    obj;
  struct obj_affect *o_af;


  if (!(ch && (pos >= 0) && (pos < MAX_WEAR) && ch->equipment[pos]))
  {
    logit(LOG_EXIT, "assert: unequip_char char called with bad args");
    raise(SIGSEGV);
  }
  obj = ch->equipment[pos];

  if (!OBJ_WORN(obj))
  {
    logit(LOG_EXIT, "equip: obj is not flagged equipped when in equip.");
    raise(SIGSEGV);
  }
  if (IS_PC(ch) && GET_ITEM_TYPE(ch->equipment[pos]) == ITEM_ARMOR)
    ch->only.pc->prestige -= obj->value[2];

  all_affects(ch, FALSE);
  ch->equipment[pos] = NULL;
  obj->loc_p = LOC_NOWHERE;
  all_affects(ch, TRUE);

  balance_affects(ch);

  if (IS_SET(obj->extra_flags, ITEM_LIT) ||
      ((obj->type == ITEM_LIGHT) && obj->value[2]))
  {
    char_light(ch);
    room_light(ch->in_room, REAL);
  }
  GET_CARRYING_W(ch) -= (GET_OBJ_WEIGHT(obj) / 2);

  return (obj);
}

void unequip_all(P_char ch)
{
  for (int i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      obj_to_char(unequip_char(ch, i), ch);
}  

void transfer_inventory(P_char ch, P_char recipient)
{
  P_obj o, next_obj;
  
  for( o = ch->carrying; o; o = next_obj )
  {
    next_obj = o->next_content;
    obj_from_char(o, FALSE);
    obj_to_char(o, recipient);
  }
}  

int get_number(char **name)
{
  int      i;
  char    *ppos;
  char     t_buf1[MAX_STRING_LENGTH];

  t_buf1[0] = 0;

  if ((ppos = index(*name, '.')))
  {
    *ppos++ = '\0';
    strcpy(t_buf1, *name);
    strcpy(*name, ppos);

    for (i = 0; *(t_buf1 + i); i++)
      if (!isdigit(*(t_buf1 + i)))
        return (0);

    return (atoi(t_buf1));
  }
  return (1);
}

/*
 * search a given list for an object, and return a pointer to that object
 */

P_obj get_obj_in_list(char *name, P_obj list)
{
  P_obj    i;
  int      j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (name)
    strcpy(tmpname, name);
  tmp = tmpname;
  if (!(k = get_number(&tmp)))
    return (0);

  for (i = list, j = 1; i && (j <= k); i = i->next_content)
    if (isname(tmp, i->name))
    {
      if (j == k)
        return (i);
      j++;
    }
  return (0);
}

/*
 * search a given list for an object number, and return a ptr to that obj
 */

P_obj get_obj_in_list_num(int num, P_obj list)
{
  P_obj    i;

  for (i = list; i; i = i->next_content)
    if (i->R_num == num)
      return (i);

  return (0);
}

/*
 * search the entire world for an object, and return a pointer
 */

P_obj get_obj(char *name)
{
  P_obj    i;
  int      j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (name)
    strcpy(tmpname, name);
  tmp = tmpname;
  if (!(k = get_number(&tmp)))
    return (0);

  for (i = object_list, j = 1; i && (j <= k); i = i->next)
    if (isname(tmp, i->name))
    {
      if (j == k)
        return (i);
      j++;
    }
  return (0);
}

/*
 * search the entire world for an object number, and return a pointer
 */

P_obj get_obj_num(int nr)
{
  P_obj    i;

  for (i = object_list; i; i = i->next)
    if (i->R_num == nr)
      return (i);

  return (0);
}

/*
 * search a room for a char, and return a pointer if found..
 */

P_char get_char_room(const char *name, int room)
{
  P_char   i;
  int      j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (name)
    strcpy(tmpname, name);
  tmp = tmpname;
  if (!(k = get_number(&tmp)))
    return (0);

  for (i = world[room].people, j = 1; i && (j <= k); i = i->next_in_room)
    if (isname(tmp, IS_NPC(i) ? FirstWord(i->player.name) : GET_NAME(i)))
    {
      if (j == k && !IS_AFFECTED(i, AFF_HIDE))
        return (i);
      j++;
    }
  return (0);
}

P_char get_char_ranged(const char *name, P_char ch, int distance, int dir)
{
  P_char   vict = NULL;
  int      source_room, old_cord;
  int      target_room2, target_room, i;
  char     tmp[MAX_STRING_LENGTH];

  if (name)
    strcpy(tmp, name);

  source_room = target_room = ch->in_room;

  // For each room in range
  for( i = 0; i < distance; i++ )
  {
    // If there's a good exit in that direction
    if( VIRTUAL_EXIT(target_room, dir)
      && !(VIRTUAL_EXIT(target_room, dir)->exit_info & (EX_CLOSED | EX_LOCKED | EX_SECRET | EX_BLOCKED)) )
    {
      // If there's no wall in the way.
      if( !check_wall(target_room, dir) )
      {
        ch->in_room = target_room2 = VIRTUAL_EXIT(target_room, dir)->to_room;
        vict = get_char_room_vis(ch, tmp);
        if( vict )
        {
          if( !IS_TRUSTED(ch) && IS_AFFECTED3(vict, AFF3_COVER) )
            vict = NULL;
          else
            break;
        }
      }
      // Otherwise, if there is a wall and we can shoot over.
      else if( check_wall(target_room, dir) && GET_CHAR_SKILL(ch, SKILL_INDIRECT_SHOT) )
      {
        ch->in_room = target_room2 = world[source_room].dir_option[dir]->to_room;
        vict = get_char_room_vis(ch, tmp);
        if( vict )
        {
          if ((world[source_room].sector_type == SECT_INSIDE) ||
        (world[source_room].sector_type == SECT_UNDRWLD_WILD) ||
        (world[source_room].sector_type == SECT_UNDRWLD_CITY) ||
        (world[source_room].sector_type == SECT_UNDRWLD_INSIDE) ||
        (world[source_room].sector_type == SECT_UNDRWLD_WATER) ||
        (world[source_room].sector_type == SECT_UNDRWLD_NOSWIM) ||
        (world[source_room].sector_type == SECT_UNDRWLD_NOGROUND) ||
        (world[source_room].sector_type == SECT_UNDRWLD_MOUNTAIN) ||
        (world[source_room].sector_type == SECT_UNDRWLD_SLIME) ||
        (world[source_room].sector_type == SECT_UNDRWLD_LOWCEIL) ||
        (world[source_room].sector_type == SECT_UNDRWLD_LIQMITH) ||
        (world[source_room].sector_type == SECT_UNDRWLD_MUSHROOM) ||
        (world[source_room].sector_type == SECT_UNDRWLD_WILD) ||
        (world[target_room].sector_type == SECT_INSIDE) ||
        (world[target_room].sector_type == SECT_UNDRWLD_WILD) ||
        (world[target_room].sector_type == SECT_UNDRWLD_CITY) ||
        (world[target_room].sector_type == SECT_UNDRWLD_INSIDE) ||
        (world[target_room].sector_type == SECT_UNDRWLD_WATER) ||
        (world[target_room].sector_type == SECT_UNDRWLD_NOSWIM) ||
        (world[target_room].sector_type == SECT_UNDRWLD_NOGROUND) ||
        (world[target_room].sector_type == SECT_UNDRWLD_MOUNTAIN) ||
        (world[target_room].sector_type == SECT_UNDRWLD_SLIME) ||
        (world[target_room].sector_type == SECT_UNDRWLD_LOWCEIL) ||
        (world[target_room].sector_type == SECT_UNDRWLD_LIQMITH) ||
        (world[target_room].sector_type == SECT_UNDRWLD_MUSHROOM) ||
        (world[target_room].sector_type == SECT_UNDRWLD_WILD))
          {
            vict = NULL;
          }
          else if( !IS_TRUSTED(ch) && IS_AFFECTED3(vict, AFF3_COVER))
          {
            vict = NULL;
          }
          else
          {
            break;
          }
        }
      }
      // Otherwise, we can't get over the wall.
      else
        break;
    }
    // Otherwise, there no good exit in that direction.
    else
      break;
    target_room = target_room2;
  }

  ch->in_room = source_room;
  old_cord = ch->specials.z_cord;

  /* ok we check in room but diff. z-coor */

  if (dir == 4)
  {
    if (!vict)
    {
      for (i = old_cord + 1; i <= old_cord + distance; i++)
      {
        ch->specials.z_cord = i;
        vict = get_char_room_vis(ch, tmp);
        if (vict)
        {
          target_room = ch->in_room;
          break;
        }
      }
    }
  }
  ch->specials.z_cord = old_cord;

  if (dir == 5 && old_cord > 0)
  {
    if (!vict)
    {
      for (i = old_cord - 1; i >= 0 && i >= old_cord - distance; i--)
      {
        ch->specials.z_cord = i;
        vict = get_char_room_vis(ch, tmp);
        if (vict)
        {
          if (i == 0 && IS_AFFECTED3(vict, AFF3_COVER))
          {
            vict = NULL;
            break;
          }
          else
          {
            target_room = ch->in_room;
            break;
          }
        }
      }
    }
  }
  ch->specials.z_cord = old_cord;

  if (vict)
  {
    if (check_castle_walls(ch->in_room, vict->in_room))
    {
      send_to_char("&+LYou can't breach the castle wall.&n\r\n", ch);
      return NULL;
    }
    if (GET_ZONE(ch) == GET_ZONE(vict))
      return (vict);
    else
    {
      send_to_char("&+LYou can't reach them there. Try getting closer.&n\r\n",
                   ch);
      return NULL;
    }
  }
  else
  {
    send_to_char
      ("&+LUmm. I don't believe there is anything that direction.&n\r\n", ch);
    return NULL;
  }

  return NULL;
}

P_char get_pcchar(P_char ch, char *name, int vis)
{
  P_char   i;
  P_desc   d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->character && !d->connected)
    {
      i = d->character;
      if (isname(name, GET_NAME(i)))
      {
        if (!vis || CAN_SEE(ch, i))
        {
          return (i);
        }
      }
    }
  }
  return (0);
}

/*
 * search all over the world for a char, and return a pointer if found
 */

P_char get_char(char *name)
{
  P_char   i;
  int      j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (name)
    strcpy(tmpname, name);
  tmp = tmpname;
  if (!(k = get_number(&tmp)))
    return (0);

  for (i = character_list, j = 1; i && (j <= k); i = i->next)
    if (isname(tmp, GET_NAME(i)))
    {
      if (j == k)
        return (i);
      j++;
    }
  return (0);
}

/*
 * search all over the world for a PC char, and return a pointer if found
 */

P_char get_char2(char *name)
{
  P_char   i;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (!name)
    return (0);
  else
    strcpy(tmpname, name);
  tmp = tmpname;

  for (i = character_list; i; i = i->next)
    if (isname(tmp, GET_NAME(i)))
    {
      if (IS_PC(i))
        return (i);
    }
  return (0);
}

/*
 * search all over the world for a char num, and return a pointer if found
 */

P_char get_char_num(int nr)
{
  P_char   i;

  for (i = character_list; i; i = i->next)
    if (IS_NPC(i) && GET_RNUM(i) == nr)
      return (i);

  return (0);
}

/* put an object in a room */

void obj_to_room(P_obj object, int room)
{
  P_char   i;
  P_nevent  e1;
  P_obj    o;

  if (!OBJ_NOWHERE(object))
  {
    logit(LOG_DEBUG, "here's a switch, object NOT in NOWHERE");
    return;
  }
  if ((room < 0) || (room > top_of_world))
  {
    wizlog(56, "to_room %d", room);
    logit(LOG_EXIT, "bogus room in obj_to_room");
    return;
  }
  if (IS_ARTIFACT(object))
    remove_owned_artifact(object, NULL, TRUE);

  if (IS_WATER_ROOM(room) && !IS_SET(object->extra_flags, ITEM_FLOAT) &&
      (object->type != ITEM_BOAT) && (object->type != ITEM_SHIP) &&
      (world[room].sector_type != SECT_UNDERWATER_GR) &&
      /*(world[room].sector_type != SECT_WATER_PLANE) && */
      ((VIRTUAL_EXIT(room, DOWN) != NULL) ||
       (world[room].sector_type != SECT_UNDERWATER)))
  {
    for (i = world[room].people; i; i = i->next_in_room)
      if (CAN_SEE_OBJ(i, object) && !object->z_cord)
        act("$p sinks into the water.", TRUE, i, object, 0, TO_CHAR);

    if (/*number(0, 1) ||*/ IS_ARTIFACT(object)) 
    {
      object->z_cord = -(distance_from_shore(room));
    }
    else if ((object->type >= ITEM_SCROLL && object->type <= ITEM_WORN) ||
              (object->type == ITEM_CONTAINER) ||
              (object->type == ITEM_MONEY) ||
              (object->type >= ITEM_QUIVER && object->type <= ITEM_TOTEM) ||
              (object->type = ITEM_SHIELD))
    //else
    {
    for (i = world[room].people; i; i = i->next_in_room)
        if (CAN_SEE_OBJ(i, object) && !object->z_cord)
          act("$p gets swept away in the current!", TRUE, i, object, 0,
              TO_CHAR);
      //extract_obj(object, TRUE);
      // Sunk items goto vault under poseidon
      obj_from_room(object);
      obj_to_room(object, real_room(31724));
      return;
    }
    else
    {
      object->z_cord = -(distance_from_shore(room));
    }
  }
  object->loc_p = LOC_ROOM;
  object->loc.room = room;

  if (IS_SET(object->extra_flags, ITEM_TRANSIENT))
  {
    /* Transient objects needed to be destroyed when dropped */
    // in lockers, this can cause problems with the resort events, so make
    // the DECAY event slightly delayed...
    if (!IS_SET(world[room].room_flags, LOCKER))
      set_obj_affected(object, 0, TAG_OBJ_DECAY, 0);
    else
      set_obj_affected(object, 2, TAG_OBJ_DECAY, 0);
  }
  if (world[room].contents && (world[room].contents->R_num == object->R_num))
  {
    if (obj_index[object->R_num].virtual_number == 3)
    {
      /* generic 'pile of coins' object, merge them */
      add_coins(world[room].contents,
                object->value[0], object->value[1], object->value[2],
                object->value[3]);
      object->loc_p = LOC_NOWHERE;
      extract_obj(object, TRUE);
      return;
    }
    else
    {
      object->next_content = world[room].contents;
      world[room].contents = object;
    }
  }
  else
  {
    o = world[room].contents;
    while (o)
    {
      if (o->next_content && (o->next_content->R_num == object->R_num))
      {
        if (obj_index[object->R_num].virtual_number == 3)
        {
          /* generic 'pile of coins' object, merge them */
          add_coins(o->next_content,
                    object->value[0], object->value[1], object->value[2],
                    object->value[3]);
          object->loc_p = LOC_NOWHERE;
          extract_obj(object, TRUE);
          return;
        }
        object->next_content = o->next_content;
        o->next_content = object;
        break;
      }
      else
        o = o->next_content;
    }
    if (!o)
    {
      object->next_content = world[room].contents;
      world[object->loc.room].contents = object;
    }
  }

  if (IS_SET(object->extra_flags, ITEM_LIT) ||
      ((object->type == ITEM_LIGHT) && (object->value[2] == -1)))
    room_light(room, REAL);

  if (object && (object->type == ITEM_CORPSE) &&
      IS_SET(object->value[1], PC_CORPSE))
    writeCorpse(object);

  if (OBJ_FALLING(object))
  {
    falling_obj(object, 1, false);
  }
}

/*
 * Take an object from a room
 */

void obj_from_room(P_obj object)
{
  P_obj    i;

  if(!(object))
  {
    return;
  }

  if (!OBJ_ROOM(object))
  { // FYI, get_obj_vnum raises SIGSEGV when there is no object. Dec08 -Lucrot
    logit(LOG_DEBUG, "obj (%d) not in room in obj_from_room.",
      GET_OBJ_VNUM(object));
    return;
  }
  /* remove object from room */

  if (object == world[object->loc.room].contents)
    world[object->loc.room].contents = object->next_content;
  else
  {
    for (i = world[object->loc.room].contents;
         i && (i->next_content != object); i = i->next_content) ;

    if (!i)
    {
      logit(LOG_EXIT, "obj_from_room: futzed up room object list");
      return;
    }
    i->next_content = object->next_content;
  }


  if (IS_SET(object->extra_flags, ITEM_LIT) ||
      ((object->type == ITEM_LIGHT) && (object->value[2] == -1)))
    room_light(object->loc.room, REAL);

  object->loc_p = LOC_NOWHERE;
  object->loc.room = NOWHERE;
  object->next_content = NULL;

  /* nuke player corpse file */

  if (object && (object->type == ITEM_CORPSE) &&
      IS_SET(object->value[1], PC_CORPSE))
    PurgeCorpseFile(object);

  if (object && (object->type == ITEM_STORAGE))
    PurgeSavedItemFile(object);

}

/* put an object in an object (quaint) */

void obj_to_obj(P_obj obj, P_obj obj_to)
{
  P_obj    tmp_obj, o;
  int      wgt = 0, t_wgt = 0;
  char     buf[MAX_STRING_LENGTH];

  if (!obj || !obj_to || ((obj_to->type != ITEM_CONTAINER) &&
                          (obj_to->type != ITEM_QUIVER) &&
                          (obj_to->type != ITEM_STORAGE) &&
                          (obj_to->type != ITEM_CORPSE)))
  {

    if (obj && obj_to)
    {
      sprintf(buf, "Object %d: %s to Object %d: %s error\r\n", obj->R_num,
              obj->short_description, obj_to->R_num,
              obj_to->short_description);
      logit(LOG_EXIT, buf);
    }
    else
      logit(LOG_EXIT, "obj_to_obj: obj or obj_to is somehow invalid");

    raise(SIGSEGV);
  }
  obj->loc_p = LOC_INSIDE;
  obj->loc.inside = obj_to;

  if (obj_to->contains && (obj_to->contains->R_num == obj->R_num))
  {
    obj->next_content = obj_to->contains;
    obj_to->contains = obj;
  }
  else
  {
    o = obj_to->contains;
    while (o)
    {
      if (o->next_content && (o->next_content->R_num == obj->R_num))
      {
        obj->next_content = o->next_content;
        o->next_content = obj;
        break;
      }
      else
        o = o->next_content;
    }
    if (!o)
    {
      obj->next_content = obj_to->contains;
      obj_to->contains = obj;
    }
  }

  wgt = GET_OBJ_WEIGHT(obj);
  for (tmp_obj = obj->loc.inside; wgt && tmp_obj;
       tmp_obj = OBJ_INSIDE(tmp_obj) ? tmp_obj->loc.inside : NULL)
  {
    t_wgt = GET_OBJ_WEIGHT(tmp_obj);
    tmp_obj->weight += wgt;
    if (t_wgt == GET_OBJ_WEIGHT(tmp_obj))
      break;
    wgt = GET_OBJ_WEIGHT(tmp_obj) - t_wgt;
  }
}

/*
 * remove an object from an object
 */

void obj_from_obj(P_obj obj)
{
  P_obj    tmp, obj_from;
  int      wgt;

  if OBJ_INSIDE
    (obj)
  {
    obj_from = obj->loc.inside;
    if (obj == obj_from->contains)      /* head of list */
      obj_from->contains = obj->next_content;
    else
    {
      for (tmp = obj_from->contains; tmp && (tmp->next_content != obj); tmp = tmp->next_content) ;      /* locate previous */

      if (!tmp)
      {
        logit(LOG_EXIT, "obj_from_obj(): Fatal error in object structures");
        raise(SIGSEGV);
      }
      tmp->next_content = obj->next_content;
    }

    /* Subtract weight from containers container */
    wgt = GET_OBJ_WEIGHT(obj);
    for (tmp = obj->loc.inside; wgt && tmp;
         tmp = OBJ_INSIDE(tmp) ? tmp->loc.inside : NULL)
    {
      tmp->weight -= GET_OBJ_WEIGHT(obj);
      wgt = GET_OBJ_WEIGHT(tmp);
    }

    obj->loc_p = LOC_NOWHERE;
    obj->loc.inside = NULL;
    obj->next_content = NULL;

    if (GET_ITEM_TYPE(obj_from) == ITEM_STORAGE)
      writeSavedItem(obj_from);

  }
  else
  {
    logit(LOG_EXIT, "obj_from_obj(): call with no object");
    raise(SIGSEGV);
  }

}

/*
 * Set all loc.carrying to point to new owner
 */

void object_list_new_owner(P_obj list, P_char ch)
{
  if (list)
  {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->loc.carrying = ch;
  }
}

/* Extract an object from the world */

void extract_obj(P_obj obj, int gone_for_good)
{
  int      i;
  P_obj    temp1 = NULL;

  if (!obj)
  {
    logit(LOG_EXIT, "NULL obj in call to extract_obj");
    raise(SIGSEGV);
  }
  if (OBJ_ROOM(obj))
    obj_from_room(obj);
  else if (OBJ_CARRIED(obj))
    obj_from_char(obj, gone_for_good);
  else if (OBJ_WORN(obj))
  {
    for (i = 0; i < MAX_WEAR; i++)
      if (obj->loc.wearing->equipment[i] == obj)
      {
        temp1 = obj;
        break;
      }
    if (!temp1)
    {
      logit(LOG_EXIT, "obj loc.wearing, but not in equipment list");
      raise(SIGSEGV);
    }
    unequip_char(obj->loc.wearing, i);
  }
  else if (OBJ_INSIDE(obj))
  {
    obj_from_obj(obj);
  }
  for (; obj->contains; obj->contains = temp1)
  {
    temp1 = obj->contains->next_content;
    extract_obj(obj->contains, gone_for_good);
  }

  obj->contains = NULL;

  /*
   * remove pointer from connected char
   */
  if (obj->hitched_to)
  {
    remove_linked_object(obj);
  }
  /*
   * leaves nothing !
   */
  // clear New events as well as old ones!
  disarm_obj_events(obj, 0);
  ClearObjEvents(obj);

  if (IS_ARTIFACT(obj) && gone_for_good)
    remove_owned_artifact(obj, NULL, TRUE);

  /*
   * yank it from the object_list, very fast now
   */

  if (object_list == obj)
  {                             /*
                                 * head of list
                                 */
    object_list = obj->next;
    if (object_list)
      object_list->prev = NULL;
  }
  else
  {
    if (obj->prev)
      obj->prev->next = obj->next;
    if (obj->next)
      obj->next->prev = obj->prev;
  }

  obj->prev = NULL;
  obj->next = NULL;

  if (obj->R_num >= 0)
    (obj_index[obj->R_num].number)--;

  free_obj(obj);
}

/*
 * replaces major part of point_update, called by Events() to make an
 * object decay and be extracted.  Mainly for use on corpses of course,
 * but other objects may be handled in the same way.
 */

void Decay(P_obj obj)
{
  P_char   carrier = NULL;
  P_obj    t_obj = NULL, t_obj2 = NULL;
  int      pos, dest = 0, old_load;
  bool     corpselog = FALSE;
  bool     genericdecay = TRUE;


  if (!obj)
  {
    logit(LOG_DEBUG, "Decay():  NULL obj");
    return;
  }

  if (OBJ_ROOM(obj))
  {
    dest = 1;

    // Mossimment:     if the proc returns TRUE (it has done its own decay proc)
    //                 so genericdecay = false -- no need to do a default decay
    if (obj_index[obj->R_num].func.obj)
    {
      genericdecay =
        !(*obj_index[obj->R_num].func.obj) (obj, NULL, CMD_DECAY, NULL);
    }
    else if (obj->R_num == real_object(2))
    {
      /*
       * corpses
       */
      if (world[obj->loc.room].people)
      {
        act("The winds of time have reclaimed $p.",
            0, world[obj->loc.room].people, obj, 0, TO_ROOM);
        act("The winds of time have reclaimed $p.",
            0, world[obj->loc.room].people, obj, 0, TO_CHAR);
      }
      if IS_SET
        (obj->value[1], PC_CORPSE)
      {
        logit(LOG_CORPSE, "%s decayed in room %d.", obj->short_description,
              world[obj->loc.room].number);
        corpselog = TRUE;
      }
    }

    if (genericdecay)
    {
      /*
       * everything else
       */
      if (world[obj->loc.room].people)
      {
        act("$p crumbles to dust and blows away.",
            TRUE, world[obj->loc.room].people, obj, 0, TO_ROOM);
        act("$p crumbles to dust and blows away.",
            TRUE, world[obj->loc.room].people, obj, 0, TO_CHAR);
        /*
         * if its a wall, and we blocked the exitbit, remove it
         */
      }
      if (obj->R_num == real_object(VOBJ_WALLS))
      {
        if (!VIRTUAL_EXIT(obj->loc.room, obj->value[1]))
        {
          logit(LOG_DEBUG,
                "Decay(): error - wall is not on valid exit - room rnum #%d, value[1] %d (trying to remove EX_WALLED)\r\n",
                obj->loc.room, obj->value[1]);
        }
        else
        {
          REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info,
                     EX_WALLED);
          REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info,
                     EX_BREAKABLE);
          REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info,
                     EX_ILLUSION);
        }
      }
    }
  }
  else if (OBJ_INSIDE(obj))
  {
    dest = 2;

    /*
     * no messages if they decay while inside something else, except
     * if it is being carried, and changes the load.  So find carrier
     * and load.
     */

    for (t_obj = obj->loc.inside; OBJ_INSIDE(t_obj);
         t_obj = t_obj->loc.inside) ;
    if (OBJ_CARRIED(t_obj))
    {
      carrier = t_obj->loc.carrying;
      old_load = IS_CARRYING_W(carrier);

      if (IS_SET(obj->value[1], PC_CORPSE))
      {
        logit(LOG_CORPSE, "%s decayed in possession of %s in room %d.",
              obj->short_description,
              (IS_PC(carrier) ? GET_NAME(carrier) :
               carrier->player.short_descr), world[carrier->in_room].number);
        corpselog = TRUE;
      }
    }
    else if (OBJ_WORN(t_obj))
    {
      carrier = t_obj->loc.wearing;
      old_load = IS_CARRYING_W(carrier);

      if (IS_SET(obj->value[1], PC_CORPSE))
      {
        logit(LOG_CORPSE, "%s decayed while equipped (!) by %s in room %d.",
              obj->short_description,
              (IS_PC(carrier) ? GET_NAME(carrier) :
               carrier->player.short_descr), world[carrier->in_room].number);
        corpselog = TRUE;
      }
    }
  }
  else
  {
    /*
     * handle the other locations
     */

    if (OBJ_WORN(obj))
    {
      for (pos = 0; pos < MAX_WEAR; pos++)
        if (obj->loc.wearing->equipment[pos] &&
            (obj->loc.wearing->equipment[pos] == obj))
          break;

      if (obj->loc.wearing->equipment[pos] != obj)
      {
        logit(LOG_DEBUG, "Decay():  equipped obj %d (%s) not in equip (%s)",
              obj->R_num, obj->name, GET_NAME(obj->loc.wearing));
        balance_affects(obj->loc.wearing);
        extract_obj(obj, TRUE);
        return;
      }
      obj_to_char(unequip_char(obj->loc.wearing, pos), obj->loc.wearing);
      /*
       * now carried, drop through
       */
    }
    if (OBJ_CARRIED(obj))
    {
      if (obj->R_num == real_object(2))
      {
        /*
         * corpses
         */
        if (obj->contains)
          act("$p decays in your hands, dumping its contents on the ground.",
              FALSE, obj->loc.carrying, obj, 0, TO_CHAR);
        else
          act("$p decays in your hands, leaving no trace.",
              FALSE, obj->loc.carrying, obj, 0, TO_CHAR);
        /*
         * added logging to prevent player bitching -- DTS 2/1/95
         */
        if (IS_SET(obj->value[1], PC_CORPSE))
        {
          logit(LOG_CORPSE, "%s decayed in possession of %s in room %d.",
                obj->short_description,
                (IS_PC(obj->loc.carrying) ? GET_NAME(obj->loc.carrying) :
                 obj->loc.carrying->player.short_descr),
                world[obj->loc.carrying->in_room].number);
          corpselog = TRUE;
        }
      }
      else
      {
        /*
         * everything else
         */
        if (obj->contains)
          act
            ("$p crumbles in your hands, dumping its contents on the ground.",
             FALSE, obj->loc.carrying, obj, 0, TO_CHAR);
        else
          act("$p crumbles in your hands, leaving no trace.",
              FALSE, obj->loc.carrying, obj, 0, TO_CHAR);
      }

      if ((pos = obj->loc.carrying->in_room) != NOWHERE)
      {
        obj_from_char(obj, TRUE);
        obj_to_room(obj, pos);
        if (IS_ARTIFACT(obj))
          remove_owned_artifact(obj, NULL, TRUE);
        dest = 1;
      }
      else
      {
        extract_obj(obj, TRUE);
        return;
      }
    }
    else
    {
      /*
       * ugly
       */
      extract_obj(obj, TRUE);
      return;
    }
  }

  if (obj->contains)
  {
    for (t_obj = obj->contains; t_obj; t_obj = t_obj2)
    {
      t_obj2 = t_obj->next_content;
      obj_from_obj(t_obj);
      if (corpselog && !IS_SET(t_obj->extra_flags, ITEM_TRANSIENT))
        logit(LOG_CORPSE, "%s Decay drop: [%d] %s", obj->short_description,
              obj_index[t_obj->R_num].virtual_number, t_obj->name);
      if (dest == 1)
      {
        obj_to_room(t_obj, obj->loc.room);
        if (IS_ARTIFACT(t_obj))
          remove_owned_artifact(t_obj, NULL, TRUE);
      }
      else
      {
        obj_to_obj(t_obj, obj->loc.inside);
      }
    }
  }
  extract_obj(obj, TRUE);

  if (carrier)
  {
    if (old_load > IS_CARRYING_W(carrier))
      send_to_char("Your load suddenly feels lighter!\r\n", carrier);
    if (old_load < IS_CARRYING_W(carrier))
      send_to_char("Your load suddenly feels heavier!\r\n", carrier);
  }
}

/*
 ** Improved update_char_objects tells holder of light source that
 ** his/her light source has just gone out.
 */

void update_char_objects(P_char ch)
{
  int      i, change;

  for (change = 0, i = PRIMARY_WEAPON; i < WEAR_EYES; i++)
    if (ch->equipment[i] && (ch->equipment[i]->type == ITEM_LIGHT) &&
        (ch->equipment[i]->value[2] > 0))
    {

      (ch->equipment[i]->value[2])--;

      /*
       * Check if light source has just gone out .. if so, inform
       * the HOLDer of light source.
       */

      if (ch->equipment[i]->value[2] <= 0)
      {
        act("Your $q just went out.", FALSE, ch, ch->equipment[i], 0,
            TO_CHAR);
        act("$n's $q just went out.", FALSE, ch, ch->equipment[i], 0,
            TO_ROOM);
        change = 1;
      }
      else if (ch->equipment[i]->value[2] <= 2)
        act("Your $q glows dimly, barely illuminating the room.", FALSE, ch,
            ch->equipment[i], 0, TO_CHAR);
      else if (ch->equipment[i]->value[2] <= 6)
        act("Your $q flickers as it slowly burns down.", FALSE, ch,
            ch->equipment[i], 0, TO_CHAR);
    }
  if (change)
  {
    char_light(ch);
    room_light(ch->in_room, REAL);
  }
}

/*
 * Extract a ch completely from the world
 */

void extract_char(P_char ch)
{
  P_obj    obj;
  P_char   k;
  P_desc   t_desc;
  int      l, i;
  P_event  ev_save;
  P_event  ev;
  char     buf[MAX_STRING_LENGTH];
  snoop_by_data *snoop_by_ptr, *next;
  struct affected_type *af, *nextaf;

  if (!ch)
  {
    logit(LOG_EXIT, "No ch in extract_char");
    raise(SIGSEGV);
  }
  if (!(*ch->player.name))
  {
    logit(LOG_EXIT, "No name in extract_char");
    raise(SIGSEGV);
  }
#if defined (CTF_MUD) && (CTF_MUD == 1)
  while (affected_by_spell(ch, TAG_CTF))
  {
    int stat = GET_STAT(ch);
    SET_POS(ch, GET_POS(ch) + STAT_NORMAL);
    drop_ctf_flag(ch);
    SET_POS(ch, GET_POS(ch) + stat);
  }
#endif
  if (IS_MORPH(ch))
  {                             /*
                                 * eek...
                                 */
    un_morph(ch);
    return;
  }
  if (IS_PC(ch) && !ch->desc)
  {
    for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
      if (t_desc->original == ch)
        do_return(t_desc->character, 0, -4);
  }
  
  if (IS_PC(ch))
  {
    if (IS_AFFECTED2(ch, AFF2_CASTING))
      StopCasting(ch);
  }
  if (ch->followers || ch->following)
    die_follower(ch);

  if (ch->group)
    group_remove_member(ch);

  clear_all_links(ch);
  if (IS_PC(ch))
    clear_logs(ch);

#if 0
  if (IS_SHADOWING(ch))
  {
    act("You stop shadowing $N.",
        TRUE, ch, 0, GET_CHAR_SHADOWED(ch), TO_CHAR);
    FreeShadowedData(ch, GET_CHAR_SHADOWED(ch));
  }
  else
  {
    if (IS_BEING_SHADOWED(ch))
    {
      StopShadowers(ch);
    }
  }
#endif

  /* empty vehicle slot */
  remove_all_linked_objects(ch);

  /* clear auctions they are involved in */
/*
  for (i = 1; i <= LAST_HOME; i++)
  {
    if (auction[i].item != NULL && (ch == auction[i].seller))
    {
      P_char   auctioneer = find_auctioneer(i);

      sprintf(buf,
              "Sale of %s has been cancelled by the seller's early departure!",
              auction[i].item->short_description);
      if (auctioneer)
        mobsay(auctioneer, buf);
      // * return money to the buyer
      if (auction[i].buyer != NULL && auction[i].buyer != ch)
      {
        ADD_MONEY(auction[i].buyer, auction[i].bet);
        send_to_char("Your money has been returned.\r\n", auction[i].buyer);
      }
      auction[i].item = NULL;
      auction[i].seller = NULL;
      auction[i].buyer = NULL;
      auction[i].bet = 0;
      auction[i].going = 0;
      auction[i].pulse = 0;
    }
    if (auction[i].item != NULL && (ch == auction[i].buyer))
    {
      P_char   auctioneer = find_auctioneer(i);

      sprintf(buf,
              "Last bid has been withdrawn. We will now start over.\r\n");
      if (auctioneer)
        mobsay(auctioneer, buf);
      ADD_MONEY(auction[i].buyer, auction[i].bet);
      auction[i].buyer = NULL;
      auction[i].bet = 0;
      auction[i].going = 0;
      auction[i].pulse = 0;
    }
  }
*/
  /*
   * make mobs stop hunting ch.  Doing it "longhand" to make the code a
   * bit more readable..
   */

  justice_guard_remove(ch);
  justice_victim_remove(ch);

  if (ch->desc)
  {
    sql_disconnectIP(ch);

    if (ch->desc->snoop.snooping)
    {
      /*
       * if !d->character, or they aren't playing, I can't get their
       * level.. so I'll assume its better then 58 to be safe
       */
      if (GET_LEVEL(ch) < 58)
        //send_to_char("&+CYou are no longer being snooped.&N\r\n",
        //           ch->desc->snoop.snooping);
        rem_char_from_snoopby_list(&ch->desc->snoop.snooping->desc->snoop.
                                   snoop_by_list, ch);
    }
    snoop_by_ptr = ch->desc->snoop.snoop_by_list;
    while (snoop_by_ptr)
    {
      send_to_char("Your victim is no longer among us.\r\n",
                   snoop_by_ptr->snoop_by);
      snoop_by_ptr->snoop_by->desc->snoop.snooping = 0;

      snoop_by_ptr = snoop_by_ptr->next;
    }

    ch->desc->snoop.snooping = /*ch->desc->snoop.snoop_by = */ 0;

    snoop_by_ptr = ch->desc->snoop.snoop_by_list;
    while (snoop_by_ptr)
    {
      next = snoop_by_ptr->next;
      FREE(snoop_by_ptr);

      snoop_by_ptr = next;
    }

    ch->desc->snoop.snoop_by_list = 0;
  }
  /*
   * Code to stop others from ignoring person quitting
   */
  ac_stopAllFromIgnoring(ch);

  if (ch->specials.fighting)
    stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  /*
   * Code to stop all that are attacking ch
   */
  StopAllAttackers(ch);

  if (current_event && current_event->actor.a_ch == ch)
    current_event = NULL;

  for (af = ch->affected; af; af = nextaf)
  {
    nextaf = af->next;
    affect_remove(ch, af);
  }

  ClearCharEvents(ch);

  /* clear equipment_list */
  for (l = 0; l < MAX_WEAR; l++)
    if (ch->equipment[l])
    {

      obj = unequip_char(ch, l);
      /* Added pet check */
      if( ch->in_room == NOWHERE || IS_SET(obj->extra_flags, ITEM_TRANSIENT) || IS_SHOPKEEPER(ch) || (IS_NPC(ch) && IS_RANDOM_MOB(ch)) )
      {
        extract_obj(obj, TRUE);
        obj = NULL;
      }
      else
        obj_to_room(obj, ch->in_room);
    }
  if(ch && ch->carrying)
  {
    P_obj    next_obj;

    for (obj = ch->carrying; obj != NULL; obj = next_obj)
    {
      next_obj = obj->next_content;

      if( ch->in_room == NOWHERE || IS_SHOPKEEPER(ch) || IS_SET(obj->extra_flags, ITEM_TRANSIENT)
        || (IS_NPC(ch) && mob_index[GET_RNUM(ch)].virtual_number >= 100000 && mob_index[GET_RNUM(ch)].virtual_number < 110000))
      {
        extract_obj(obj, TRUE);
        obj = NULL;
      }
      else
      {
        obj_from_char(obj, TRUE);
        obj_to_room(obj, ch->in_room);
      }
    }
  }
  witness_destroy(ch);

  char_from_room(ch);

  /* pull the char from the list */

  if (ch == character_list)
    character_list = ch->next;
  else
  {
    for (k = character_list; (k) && (k->next != ch); k = k->next) ;
    if (k)
    {
      k->next = ch->next;
    }
    else
    {
      logit(LOG_EXIT, "extract_char(), Char not in character_list. (%s)",
            GET_NAME(ch));
      raise(SIGSEGV);
    }
  }
  
  SET_POS(ch, GET_POS(ch) + STAT_DEAD);
  GET_AC(ch) = 100;

  if (ch->desc)
  {
    if (ch->desc->original)
      do_return(ch, 0, -4);
  }
  if (ch->desc)
  {
    if (ch->desc->connected != CON_DELETE)
    {
#ifndef USE_ACCOUNT
      ch->desc->connected = CON_SLCT;
      SEND_TO_Q(MENU, ch->desc);
      //  SEND_TO_Q("\r\n*** PRESS RETURN: ", ch->desc);
      //  ch->desc->connected = CON_RMOTD;
#else
      ch->desc->connected = CON_DISPLAY_ACCT_MENU;
      ch->desc->character = NULL;
      display_account_menu(ch->desc, NULL);
      free_char(ch);
      ch = NULL;
#endif
    }
  }
  if (ch && IS_NPC(ch))
  {
    if (GET_RNUM(ch) > -1)            /* if mobile */
      mob_index[GET_RNUM(ch)].number--;
    free_char(ch);
    ch = NULL;
  }
}

/*
 * ***********************************************************************
 * Here follows high-level versions of some earlier routines, ie functions
 * which incorporate the actual player-data.
 * ***********************************************************************
 */

P_char get_char_ranged_vis(P_char ch, char *arg, int range)
{
  int      dir;
  char     direction[MAX_INPUT_LENGTH], target[MAX_INPUT_LENGTH];
  P_char   victim;

  if (!arg || !*arg)
  {
    send_to_char("Hmm?\r\n", ch);
    return NULL;
  }
  /*
   * Now, we can extract the info we need
   */
  half_chop(arg, target, direction);

  if (!direction || !*direction)
    return NULL;

  dir = dir_from_keyword(direction);
  if (dir == -1)
    return NULL;
  else if (!(victim = get_char_ranged(target, ch, range, dir)))
    return NULL;

  if (CAN_SEE(ch, victim))
    return victim;

  return NULL;
}

P_char get_char_room_vis(P_char ch, const char *name)
{
  P_char   i;
  int      j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (!name || !*name)
    return NULL;

  strcpy(tmpname, name);
  tmp = tmpname;

  while (*tmp == ' ')
    tmp++;

  if (!*tmp || !(k = get_number(&tmp)))
    return NULL;

  if (!str_cmp(tmp, "me") || !str_cmp(tmp, "self"))
    return (ch);

  for (i = world[ch->in_room].people, j = 1;
       i && (j <= k); i = i->next_in_room)
  {

    if (CAN_SEE(ch, i) && (ch->specials.z_cord == i->specials.z_cord) && (
       (IS_TRUSTED(ch) && isname(tmp, GET_NAME(i))) ||  // Is imm looking at existing name
       (isname(tmp, GET_NAME1(i)) && (!racewar(ch, i) || // Is name of real or disguised? & is there racewar?(no if same faction)
       IS_ILLITHID(ch) || IS_PILLITHID(ch) || IS_TRUSTED(ch))) || 
       //(IS_DISGUISE(i) && IS_DISGUISE_PC(i) && isname(tmp, GET_DISGUISE_NAME(i))) ||
       (IS_DISGUISE(i) && IS_DISGUISE_NPC(i) && isname(tmp, GET_DISGUISE_TITLE(i)) && racewar(ch, i)) ||
       ((isname(tmp, race_names_table[GET_RACE1(i)].normal) &&
       /*  !IS_DISGUISE_NPC(i) && */ (!is_introd(i, ch) ||
       /* racewar(ch, i) || */ (IS_DISGUISE(i) && (i != ch)) ))) ||
       (isname(tmp, GET_NAME(i)) && (IS_NPC(i))) ||
         ((i != ch) && !IS_TRUSTED(ch) && !CAN_DAYPEOPLE_SEE(ch->in_room) &&
          IS_AFFECTED(ch, AFF_INFRAVISION) && (isname(tmp, "shape") ||
                                               isname(tmp, "outline")))))
    {
      if (j == k)
        return (i);
      j++;
    }
  }

  return (0);
}

P_char get_pc_vis(P_char ch, const char *name)
{
  P_char   i;
  P_desc   d;
  int      j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (!name || !*name)
    return (0);
  /*
   * check location
   */
  if ((i = get_char_room_vis(ch, name)))
    return (i);

  strcpy(tmpname, name);
  tmp = tmpname;
  if (!(k = get_number(&tmp)))
    return (0);

  if (!str_cmp(name, "me") || !str_cmp(name, "self"))
    return (ch);

  for (d = descriptor_list, j = 1; d && (j <= k); d = d->next)
    if (d->character && isname(tmp, GET_NAME(d->character)) &&
        !racewar(ch, d->character))
      if (CAN_SEE(ch, d->character))
      {
        if (j == k)
          return (d->character);
        j++;
      }
  return (0);
}

P_char get_char_vis(P_char ch, const char *name)
{
  P_char   i;
  int      j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (!name || !*name)
    return (0);
  /*
   * check location
   */
  if ((i = get_char_room_vis(ch, name)))
    return (i);

  strcpy(tmpname, name);
  tmp = tmpname;
  if (!(k = get_number(&tmp)))
    return (0);

  if (!str_cmp(name, "me") || !str_cmp(name, "self"))
    return (ch);

  for (i = character_list, j = 1; i && (j <= k); i = i->next)
    if (isname(tmp, GET_NAME(i)) && !racewar(ch, i))
      if (CAN_SEE(ch, i))
      {
        if (j == k)
          return (i);
        j++;
      }
  return (0);
}

P_obj get_obj_in_list_vis(P_char ch, char *name, P_obj list)
{
  P_obj    i;
  int      j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if( !name || !*name )
    return (0);

  strcpy(tmpname, name);
  tmp = tmpname;
  if( !(k = get_number(&tmp)) )
  {
    return (0);
  }
  for (i = list, j = 1; i && (j <= k); i = i->next_content)
  {
    if( isname(tmp, i->name)
      || (IS_PC(ch) && IS_TRUSTED(ch) && atoi(name) > 0 && atoi(name) == GET_OBJ_VNUM(i)) )
    {
      if (CAN_SEE_OBJ(ch, i) || IS_NOSHOW(i))
      {
        if (j == k)
          return (i);
        j++;
      }
    }
  }
  return (0);
}

/*
 * search the entire world for an object, and return a pointer
 * zrange is the distance above/below char.
 */
P_obj get_obj_vis(P_char ch, char *name, int zrange )
{
  P_obj    t_obj;
  int      i, j, k;
  char     tmpname[MAX_STRING_LENGTH];
  char    *tmp;

  if (!name || !*name)
    return (0);

  strcpy(tmpname, name);
  tmp = tmpname;
  if (!(k = get_number(&tmp)))
    return (0);

  /*
   * check equipment first, this was ancient glitch
   */
  for (i = 0, j = 0; (i < MAX_WEAR) && (j <= k); i++)
    if ((t_obj = ch->equipment[i]))
      if (isname(tmp, t_obj->name))
        if (CAN_SEE_OBJ(ch, t_obj) || IS_NOSHOW(t_obj))
        {
          if (j == k)
            return (t_obj);
          j++;
        }
  /*
   * scan items carried
   */
  if ((t_obj = get_obj_in_list_vis(ch, tmp, ch->carrying)))
    return (t_obj);

  /*
   * scan room
   */
  if ((t_obj = get_obj_in_list_vis(ch, tmp, world[ch->in_room].contents)))
    return (t_obj);

  /*
   * ok.. no luck yet. scan the entire obj list
   */
  if( zrange <= 0 )
  {
    for( t_obj = object_list, j = 1; t_obj && (j <= k); t_obj = t_obj->next )
    {
      if (isname(tmp, t_obj->name))
      {
        // If you can see it, or it's flagged noshow... then you can see it?
        //   Yes, the NOSHOW flag means you can't see it when you look in room,
        //   but it shows when you try to interact with it (ie push button / touch flowers / l <extra desc> etc).
        if (CAN_SEE_OBJ(ch, t_obj) || IS_NOSHOW(t_obj))
        {
          if (j == k)
            return (t_obj);
          j++;
        }
      }
    }
  }
  else
  {
    for( t_obj = object_list, j = 1; t_obj && (j <= k); t_obj = t_obj->next )
    {
      if (isname(tmp, t_obj->name))
      {
        if( CAN_SEE_OBJZ(ch, t_obj, zrange) || IS_NOSHOW(t_obj) )
        {
          if (j == k)
            return (t_obj);
          j++;
        }
      }
    }
  }

  return (0);
}

void add_coins(P_obj pile, int copper, int silver, int gold, int platinum)
{
  int      num, i, j, p;
  const char *desc, *desc2;
  char     buf[200], buf2[200];
  struct extra_descr_data *nd;

  if (!pile || (pile->type != ITEM_MONEY))
    return;

  if ((copper < 0) || (silver < 0) || (gold < 0) || (platinum < 0))
  {
    logit(LOG_EXIT, "add_coins: trying to add negative coins");
    raise(SIGSEGV);
  }

  pile->value[0] += copper;
  pile->value[1] += silver;
  pile->value[2] += gold;
  pile->value[3] += platinum;

  if ((pile->value[0] < 0) || (pile->value[1] < 0) || (pile->value[2] < 0) ||
      (pile->value[3] < 0))
  {
    logit(LOG_EXIT, "add_coins: pile has negative coins");
    raise(SIGSEGV);
  }

  num = (pile->value[0] + pile->value[1] + pile->value[2] + pile->value[3]);

  if (num < 0)
  {
    logit(LOG_EXIT, "add_coins: total number of coins in pile is negative");
    raise(SIGSEGV);
  }
  else if (num == 0)
    return;

  // Making money weightless per Kitsero due to money inflation causing
  // issues with weight.  - Venthix Nov '10
  //pile->weight = num / 75;
  pile->weight = 0;

  if (num >= 1000000)
  {
    desc = "A mountain of coins is piled here.";
    desc2 = "a mountain of coins";
  }
  else if (num >= 100000)
  {
    desc = "A huge pile of coins lies here.";
    desc2 = "a huge pile of coins";
  }
  else if (num >= 10000)
  {
    desc = "A large pile of coins lies here.";
    desc2 = "a large pile of coins";
  }
  else if (num >= 1000)
  {
    desc = "A pile of coins lies here.";
    desc2 = "a pile of coins";
  }
  else if (num >= 30)
  {
    desc = "A small pile of coins lies here.";
    desc2 = "a small pile of coins";
  }
  else if (num >= 10)
  {
    desc = "A few coins lie scattered here.";
    desc2 = "a few coins";
  }
  else if (num > 5)
  {
    desc = "A handful of coins lie here.";
    desc2 = "a handful of coins";
  }
  else if (num > 1)
  {
    sprintf(buf, "%d coins are scattered about here.", num);
    sprintf(buf2, "%d coins", num);
    desc = buf;
    desc2 = buf2;
  }
  else
  {
    for (i = 0; i < 4; i++)
      if (pile->value[i])
        j = i;
    sprintf(buf, "A %s coin is here.", coin_names[j]);
    sprintf(buf2, "a %s coin", coin_names[j]);
    desc = buf;
    desc2 = buf2;
  }

  if ((pile->str_mask & STRUNG_DESC2) && pile->short_description)
    str_free(pile->short_description);

  pile->str_mask |= STRUNG_DESC2;
  pile->short_description = (char *) str_dup(desc2);
//  pile->short_description[0] = tolower(pile->short_description[0]);
//  pile->short_description[strlen(pile->short_description) - 1] = 0;

  if ((pile->str_mask & STRUNG_DESC1) && pile->description)
    str_free(pile->description);

  pile->str_mask |= STRUNG_DESC1;
  pile->description = (char *) str_dup(desc);

  nd = pile->ex_description;
  if (nd)
  {
    if (nd->description)
    {
      str_free(nd->description);
      nd->description = NULL;
    }
    if (num == 1)
      nd->description = (char *) str_dup(pile->description);
    else
    {
      strcpy(buf, "The pile appears to consist of: ");
      for (i = 0; i < 4; i++)
      {
        p = (pile->value[i] * 100) / num;
        if (p > 99)
          sprintf(buf + strlen(buf), "%s coins, ", coin_names[i]);
        else if (p >= 85)
          sprintf(buf + strlen(buf), "mostly %s coins, ", coin_names[i]);
        else if (p >= 65)
          sprintf(buf + strlen(buf), "3/4 %s coins, ", coin_names[i]);
        else if (p >= 40)
          sprintf(buf + strlen(buf), "half %s coins, ", coin_names[i]);
        else if (p >= 20)
          sprintf(buf + strlen(buf), "1/4 %s coins, ", coin_names[i]);
        else if (p >= 10)
          sprintf(buf + strlen(buf), "some %s coins, ", coin_names[i]);
        else if (p >= 1)
          sprintf(buf + strlen(buf), "a few %s coins, ", coin_names[i]);
      }
      buf[strlen(buf) - 2] = '.';
      buf[strlen(buf) - 1] = 0;
      nd->description = (char *) str_dup(buf);
    }
  }
}

P_obj create_money(int copper, int silver, int gold, int platinum)
{
  P_obj    obj;

  obj = read_object(VOBJ_COINS, VIRTUAL);
  if (!obj)
  {
    logit(LOG_EXIT, "create_money: cannot load coin pile object");
    raise(SIGSEGV);
  }

  add_coins(obj, copper, silver, gold, platinum);

  return (obj);
}

/*
 * Generic Find, designed to find any object/character
 * Calling :
 * *arg     is the string containing the string to be searched for.
 * This string doesn't have to be a single word, the routine
 * extracts the next word itself.
 * bitv..   All those bits that you want to "search through".
 * Bit found will be result of the function
 * *ch      This is the person that is trying to "find"
 * **tar_ch  Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 * The routine returns 0 (if not found) otherwise, returns the bit in
 * which target was found.
 */

int generic_find(char *arg, int bitvector, P_char ch, P_char * tar_ch, P_obj * tar_obj)
{
  int      i;
  char     name[MAX_INPUT_LENGTH];
  bool     found = FALSE;
  static const char *ignore[] = {
    "the",
    "in",
    "on",
    "at",
    "\n"
  };

  bzero(name, MAX_INPUT_LENGTH);
  *tar_ch = 0;
  *tar_obj = 0;

  /*
   * * Eliminate spaces and "ignore" words
   */
  while( *arg && !found )
  {
    while( *arg == ' ' )
    {
      ++arg;
    }

    for(i = 0; (name[i] = *(arg + i)) && (name[i] != ' '); i++)
    {
      ;
    }

    name[i] = 0;
    arg += i;
    if( search_block(name, ignore, TRUE) > -1 )
    {
      found = TRUE;
    }
  }

  if( !name[0] )
  {
    return (0);
  }

/*
   *tar_ch = 0;
   *tar_obj = 0;
 */

  /*
   * * local people
   */

  if (IS_SET(bitvector, FIND_CHAR_ROOM))
  {                             /*
                                 * * Find person in room
                                 */
    if ((*tar_ch = get_char_room_vis(ch, name)))
    {
      return (FIND_CHAR_ROOM);
    }
  }
  /*
   * * local objects
   */

  if (IS_SET(bitvector, FIND_OBJ_INV) && ch->carrying)
  {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)))
    {
      return (FIND_OBJ_INV);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_EQUIP))
  {
    for (found = FALSE, i = 0; i < MAX_WEAR && !found; i++)
      if (ch->equipment[i] && isname(name, ch->equipment[i]->name))
      {
        *tar_obj = ch->equipment[i];
        found = TRUE;
      }
    if (found)
    {
      return (FIND_OBJ_EQUIP);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_ROOM) && world[ch->in_room].contents)
  {
    if( (*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) )
    {
      return (FIND_OBJ_ROOM);
    }
  }
  /*
   * check for both sorts of global searches
   */

  if (IS_SET(bitvector, FIND_CHAR_WORLD))
  {
    if ((*tar_ch = get_char_vis(ch, name)))
    {
      return (FIND_CHAR_WORLD);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_WORLD))
  {
    if ((*tar_obj = get_obj_vis(ch, name, (bitvector & FIND_IGNORE_ZCOORD) ? MAX_ALTITUDE : 0)))
    {
      return (FIND_OBJ_WORLD);
    }
  }
  return (0);
}

/*
 * AC new additions
 */

/*
 * ** This function is called when "ch" is quitting game.  Note that **
 * the relevant fields of "ch" must be intact before calling of this **
 * function. ** ** This function stops other people from ignoring the
 * quitting, thereby ** freeing them to ignore someone else ;)
 */

void ac_stopAllFromIgnoring(P_char ch)
{
  P_desc   c;

  for (c = descriptor_list; c; c = c->next)
  {
    if (c->character /*&& (c->connected == CON_PLYNG) */  &&
        IS_PC(c->character)
        && /*(c->character->only.pc) && */
      (c->character->only.pc->ignored == ch))
    {
      if (c->connected == CON_PLYNG)
        send_to_char
          ("The person you are ignoring has just quit the game.\r\n",
           c->character);
      c->character->only.pc->ignored = NULL;
    }
  }
}

int can_char_use_item(P_char ch, P_obj obj)
{
  if( !ch || !obj )
  {
    return (FALSE);
  }

  // Images can't wear eq.
  if( IS_NPC(ch) && (GET_RNUM(ch) == real_mobile(250)) )
  {
    return FALSE;
  }

  if( IS_ILLITHID(ch) || GET_PRIME_CLASS(ch, CLASS_MINDFLAYER) )
  {
    return TRUE;
  }

  if( !IS_SET(obj->extra_flags, ITEM_ALLOWED_RACES) )
  {
    if( GET_RACE(ch) <= RACE_PLAYER_MAX && IS_SET(obj->anti2_flags, 1 << (GET_RACE(ch) - 1)) )
    {
      return FALSE;
    }
  }
  else
  {
    if( GET_RACE(ch) > RACE_PLAYER_MAX || !IS_SET(obj->anti2_flags, 1 << (GET_RACE(ch) - 1)) )
    {
      return FALSE;
    }
  }

  // Allow Blighters/Blighter Multiclasses to use Druid eq unless specifically denied.
  if( GET_CLASS(ch, CLASS_BLIGHTER) )
  {
    if( IS_SET(obj->extra_flags, ITEM_ALLOWED_CLASSES) )
    {
      if( IS_SET(obj->anti_flags, CLASS_DRUID) )
      {
        return TRUE;
      }
    }
  }
  // Allow Summoners/Summoner Multiclasses to wear Conjurer eq unless specifically denied.
  if( GET_CLASS(ch, CLASS_SUMMONER) )
  {
    if( IS_SET(obj->extra_flags, ITEM_ALLOWED_CLASSES) )
    {
      if( IS_SET(obj->anti_flags, CLASS_CONJURER) )
      {
        return TRUE;
      }
    }
  }

  if( !IS_MULTICLASS_PC(ch) )
  {
    if( !IS_SET(obj->extra_flags, ITEM_ALLOWED_CLASSES) )
    {
      if( IS_SET(obj->anti_flags, ch->player.m_class) )
      {
        return FALSE;
      }
    }
    else if( !IS_SET(obj->anti_flags, ch->player.m_class) )
    {
      return FALSE;
    }
  }
  // Multiclass can use either class of equipment. Nov08 -Lucrot
  else if( IS_MULTICLASS_PC(ch) || IS_MULTICLASS_NPC(ch) )
  {
    if(!IS_SET(obj->extra_flags, ITEM_ALLOWED_CLASSES))
    {
      if(IS_SET(obj->anti_flags, ch->player.m_class) &&
        IS_SET(obj->anti_flags, ch->player.secondary_class))
      {
        return FALSE;
      }
    }
    else if(!IS_SET(obj->anti_flags, ch->player.m_class) &&
            !IS_SET(obj->anti_flags, ch->player.secondary_class))
    {
      return FALSE;
    }
  }
  return TRUE;
}

// Added this function so we can check on certain items if only the prime class is allowed to use it
int can_prime_class_use_item(P_char ch, P_obj obj)
{
	if (!ch || !obj)
    return (FALSE);
	 
 	  if (IS_NPC(ch) && (GET_RNUM(ch) == real_mobile(250)))
 	    return FALSE;
 	
 	  if (IS_ILLITHID(ch))
 	    return TRUE;
 	 
 	  if (!IS_SET(obj->extra_flags, ITEM_ALLOWED_RACES))
 	  {
 	    if (GET_RACE(ch) <= RACE_PLAYER_MAX &&
 	        IS_SET(obj->anti2_flags, 1 << (GET_RACE(ch) - 1)))
 	      return FALSE;
 	  }
 	  else
 	    if (GET_RACE(ch) > RACE_PLAYER_MAX ||
	        !IS_SET(obj->anti2_flags, 1 << (GET_RACE(ch) - 1)))
 	    return FALSE;
 	
 	  if (!IS_SET(obj->extra_flags, ITEM_ALLOWED_CLASSES))
 	  {
 	    if (IS_SET(obj->anti_flags, ch->player.m_class))
 	      return FALSE;
 	  }
 	  else if (!IS_SET(obj->anti_flags, ch->player.m_class))
 	    return FALSE;
 	
 	  return TRUE;
}

int io_agi_defense(P_char ch)
{
  int      i = GET_C_AGI(ch);

#ifdef GOND_KLUDGE
  return agi_app[STAT_INDEX(i)].defensive;
#else

  /*
   * NOTE:  This formula took me _hours_ to come up with.  It produces values
   * that are very close to what the "gond kludge" method does, however, it
   * works much smoother, allowing naked AC to 'notch' 1 point at a time,
   * instead of 8-10 points at a time.   DON'T FUCK WITH THIS UNLESS YOU HAVE A
   * FUCKING PH.D. IN STATISTICS!
   */
  i = MAX(-275, 275 - i);
  return -((18906 - (i * i / 4)) / 151 - 44);
#endif
}
