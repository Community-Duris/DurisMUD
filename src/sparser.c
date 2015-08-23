/*
***************************************************************************
*  File: sparser.c              Part of Duris *
*  Usage: core routines for handling spellcasting       *
*  Copyright  1990, 1991 - see 'license.doc' for complete information.    *
*  Copyright 1994 - 2008 - Duris Systems Ltd.         *
***************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "comm.h"
#include "db.h"
#include "damage.h"
#include "events.h"
#include "interp.h"
#include "new_combat.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "mm.h"
#include "weather.h"
#include "specs.prototypes.h"
#include "sound.h"
#include "graph.h"
#include "guard.h"
#include "epic_skills.h"
#include "ships.h"
#include "grapple.h"
#include "sql.h"
#include "profile.h"
#include "guildhall.h"
#include "buildings.h"
#include "ctf.h"

/*
   external variables
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_event current_event;
extern P_event event_type_list[];
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const char *event_names[];
extern const struct stat_data stat_factor[];
extern const struct racial_data_type racial_data[];
float spell_pulse_data[LAST_RACE + 1];
int racial_shrug_data[LAST_RACE + 1];
float racial_exp_mods[LAST_RACE + 1];
float racial_exp_mod_victims[LAST_RACE + 1];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern struct zone_data *zone_table;
extern struct time_info_data time_info;
extern void initialize_skills(void);
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];

extern Skill skills[MAX_AFFECT_TYPES+1];
int      SortedSkills[MAX_AFFECT_TYPES+1];
int      MobSpellIndex[MAX_SKILLS];
const char *spells[MAX_AFFECT_TYPES+1];
extern const struct class_names class_names_table[];
extern const struct race_names race_names_table[];
int top_save, starting_save;
extern struct spell_target_data common_target_data;

extern bool divine_blessing_check(P_char, P_char, int);
extern int devotion_skill_check(P_char);
void event_spellcast(P_char, P_char, P_obj, void *);
void event_abort_spell(P_char, P_char, P_obj, void *);
int chant_mastery_bonus(P_char, int);
bool ground_casting_check(P_char ch, int spl);

typedef struct {
  char *name;
  int starting;
  int top;
} saves_data_type;

saves_data_type saves_data[] = {
  {"para", 0, 0 },
  {"rod", 0, 0 },
  {"petri", 0, 0},
  {"breath", 0, 0},
  {"spell", 0, 0}
};

/*
   new saving throws are given as {max, min} for each class, max applies at
   level 1, min applies at MAXLVLMORTAL, levels in between will
   have a saving throw somewhere in between.
   JAB
 */

struct mm_ds *dead_cast_pool = NULL;

int compare_skills(const void *v1, const void *v2)
{
  int     *skill1 = (int *) v1;
  int     *skill2 = (int *) v2;

  if (!*skill1)
    return 1;
  if (!*skill2)
    return -1;
  return strcmp(skills[*skill1].name, skills[*skill2].name);
}

void perform_chaos_check(P_char ch, P_char tar, struct spellcast_datatype *arg)
{
  bool     controlled;
  P_char   tch, next;
  int      randomness;
  controlled = number(0, 2);
  int      level = GET_LEVEL(ch);

  if (GET_STAT(ch) == STAT_DEAD)
    return;

  if (controlled)
  {
    act
      ("$n's magic rips the fabric of &+yreality&n causing &+RCh&+rAo&+RT&+riC&n energies to pour in.&n",
      FALSE, ch, NULL, ch, TO_ROOM);
      send_to_char
      ("&+WYour wild magic sends &+CS&+chO&+Cck&+YwA&+yvEs &+Wthrough the fabric of reality.&n\n",
      ch); 
  }
  else
  {
    act("&+W$n loses control over the raging magic sending &+RCh&+rAo&+RT&+riC&n &+Wenergy writhing throughout the room&n.",FALSE, ch, NULL, ch, TO_ROOM);
    send_to_char("&+LYour wild magic sends &+CS&+chO&+Cck&+YwA&+yvEs through the fabric of reality, alas the forces are too strong and the magic scatters beyond control!\n",ch);
  }
 
  // Increased the spell array -Lucrot
  if (level < 40)
    randomness = 4;
  else if (level < 53) 
    randomness =  11;
  else 
    randomness = 14;

  switch (number(1, randomness))
  {
  case 1: 
    spell_burning_hands(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    spell_acid_blast(level, ch, NULL, SPELL_TYPE_SPELL, tar, 0);
    break;
  case 2:
    spell_magic_missile(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    spell_magic_missile(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 3:
    spell_shocking_grasp(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    spell_chill_touch(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 4:
    spell_insects(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 5:
    spell_inflict_pain(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 6:
    spell_slow(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 7:
    spell_cone_of_cold(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 8:
    spell_fireball(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 9:
    spell_lightning_bolt(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 10:
    spell_boulder(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 11:
    spell_stunning_visions(GET_LEVEL(ch), ch, NULL, 0, tar, 0);
    break;
  case 12:
    spell_energy_drain( (int) level + number(0, 6), ch, 0, SPELL_TYPE_SPELL, tar, 0);
    break;
  case 13:
    spell_feeblemind( (int) level + number(0, 6), ch, NULL, SPELL_TYPE_SPELL, tar, 0);
    break;
  case 14:
    spell_dispel_magic( (int) level + number(0, 6), ch, NULL, SPELL_TYPE_SPELL, tar, 0);
    break;  
  }

  if (!number(0, 3))
  {
    if (!number(0, 3))
    {
    GET_HIT(ch) -= (int) (GET_LEVEL(ch) / 4 + number(1, 5));
    send_to_char("&+rThe violent magic stream drains your health!&n\n", ch);
    }
    else
    {
    GET_HIT(ch) += (int) (GET_LEVEL(ch) / 4 + number(1, 5));
    send_to_char("&+GYou barely control the magic. Energies flow into you!&n\n", ch);
    }
  }
  
  if (GET_STAT(ch) == STAT_DEAD)
    return;
    
  // fixed bug where ch == tar and the above spell would kill them
  if(ch == tar && GET_STAT(ch) == STAT_DEAD)
    return;

  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;
    if (!number(0, 6) && tar != tch && tch != ch && !IS_TRUSTED(tch)
        && (!controlled || !grouped(ch, tch)))
    {
      // Luck influences chaos - Lucrot
      if (!controlled && grouped(ch, tch) && (int) (number(1, 100) < GET_C_LUK(ch) / 2))
        return;
      else
        ((*skills[arg->spell].spell_pointer) ((int) GET_LEVEL(ch), ch, NULL,
        SPELL_TYPE_SPELL, tch, NULL));
    }
  }
}

int is_racewar_in_room(P_char ch)
{
  P_char   t_char = world[ch->in_room].people;

  while (t_char)
  {
    if (racewar(ch, t_char))
    {
      return TRUE;
    }
    if(IS_GH_GOLEM(t_char) || IS_OP_GOLEM(t_char))
    {
      return TRUE;
    }
    t_char = t_char->next_in_room;
  }

  return FALSE;
}

bool is_ally(P_char ch, P_char other)
{
  if(IS_MORPH(other))
  {
    other = MORPH_ORIG(other);
  }
  
  if(IS_TRUSTED(other) ||
     IS_NPC(other))
  {
    return FALSE;
  }
  
  if(IS_TRUSTED(ch))
  {
    return FALSE;
  }
  
  if (RACE_GOOD(ch))
  {
    return RACE_GOOD(other);
  }
  else if (RACE_PUNDEAD(ch))
  {
    return RACE_PUNDEAD(other);
  }
  else if (RACE_EVIL(ch))
  {
    return RACE_EVIL(other) && !RACE_PUNDEAD(other);
  }
  else if (IS_ILLITHID(ch))
  {
    return IS_ILLITHID(other);
  }

  return FALSE;
}

int get_weight_allies_in_room(P_char ch, int room_index)
{
  P_char   t_char = world[room_index].people;
  char buf[250];
  int      allies = 0;
  int	   weights = 0;
  byte chrace;
  
  int chweight = 0;

  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);
  while (t_char)
  {
    if (is_ally(ch, t_char))
    {
     chrace  = GET_RACE(t_char);
	  switch (chrace)
	  {
	  case RACE_OGRE:
	  chweight += 9;
	  break;
         case RACE_FIRBOLG:
	  chweight += 9;
	  break;
	  case RACE_MINOTAUR:
	  chweight += 8;
	  break;
	  case RACE_TROLL:
	  chweight += 7;
	  break;
	  case RACE_CENTAUR:
	  chweight += 8;
	  break;
	  case RACE_BARBARIAN:
	  chweight += 7;
	  break;
	  case RACE_ORC:
	  chweight += 6;
	  break;
	  case RACE_HUMAN:
	  chweight += 5;
	  break;
	  case RACE_GITHYANKI:
	  chweight += 5;
	  break;
	  case RACE_GITHZERAI:
	  chweight += 5;
	  break;
	  case RACE_MOUNTAIN:
	  chweight += 4;
	  break;
	  case RACE_DUERGAR:
	  chweight += 4;
	  break;
         case RACE_THRIKREEN:
         chweight += 4;
         break;
	  case RACE_GREY:
	  chweight += 3;
	  break;
	  case RACE_DROW:
	  chweight += 3;
	  break;
	  case RACE_HALFLING:
	  chweight += 2;
	  break;
	  case RACE_GOBLIN:
	  chweight += 2;
	  break;
	  case RACE_KOBOLD:
	  chweight += 1;
	  break;
	  case RACE_GNOME:
	  chweight += 1;
         break;
         default:
         act("could not find race.\n", FALSE, ch, 0, 0, TO_CHAR);
         //debug("&+gRace:&n (%s) weight (%d) at (%s).", GET_RACE(t_char), chweight, GET_NAME(ch));
	  break; 
	  }
    }
    t_char = t_char->next_in_room;
  }
  return chweight;
}


int get_number_allies_in_room(P_char ch, int room_index)
{
  P_char   t_char = world[room_index].people;
  int      allies = 0;

  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);

  while (t_char)
  {
    if (is_ally(ch, t_char))
    {
      allies++;
    }
    t_char = t_char->next_in_room;
  }

  return allies;
}

int get_number_allies_within_range(P_char ch)
{
  int allies = get_number_allies_in_room(ch, ch->in_room);
  int i, troom;

  for (i = 0; i < NUM_EXITS; i++)
  {
    if (world[ch->in_room].dir_option[i])
    {
      troom = world[ch->in_room].dir_option[i]->to_room;
      if (troom != -1 && troom != ch->in_room)
      {
        if(IS_PC(ch))
          allies += get_number_allies_in_room(ch, troom);
      }
    }
  }

  return allies;
}

P_char misfire_check(P_char ch, P_char victim, int flag)
{
  P_char new_target, tch;
  int oversize = 0, chance;

// Mobs by themselves do not trigger misfire.
  if(!IS_PC(ch) ||
     !victim)
  {
    return victim;
  }

// Player is already tagged, so let us move along.
  if(affected_by_spell(ch, TAG_NOMISFIRE))
  {
    return victim;
  }

// new_target is used as a comparison below.
  new_target = victim;

// If there are no racewar conflicts, then it is a group zone misfire check.
// Code currently set to 99, so tweak this value in sql.
  if(!is_racewar_in_room(ch))
  {
    for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(IS_NPC(tch) &&
        !IS_PC_PET(tch) &&
        IS_FIGHTING(tch))
      {
        break;
      }
    }
    
    if(!tch)
    {
      return new_target;
    }
    
    oversize = get_number_allies_within_range(ch) -
                 get_property("misfire.zoning.maxGroup", 99);
    
    // Nothing to worry about, so let us move along.
    if(oversize <= 0)
    {
      return new_target;
    }
  } 
  else // Racewar PvP section.
  {
    oversize = (get_number_allies_within_range(ch) - 
      (GOOD_RACE(ch) ? get_property("misfire.pvp.maxAllies.good", 14) :
      get_property("misfire.pvp.maxAllies.evil", 12)));

    if(oversize <= 0 &&
      !affected_by_spell(ch, TAG_NOMISFIRE))
    {
      set_short_affected_by(ch, TAG_NOMISFIRE, 
        WAIT_SEC * get_property("misfire.pvp.recountDelay.sec", 2));
      
      return new_target;
    }
  }  
// Too many players... determine misfire percentage.
  if(oversize > 0)
  {
    chance = get_property("misfire.pvp.minChance", 40) +
      (oversize - 1) * get_property("misfire.pvp.chanceStep", 15);
      
    chance = MIN(chance, get_property("misfire.pvp.maxChance", 60));

    if(chance > number(0,100))
    { // Misfire!
      new_target = get_random_char_in_room(ch->in_room, ch, DISALLOW_SELF); 
    }
  }
// And a message.
  if(victim != new_target)
  {
    act("$N just got in your way!",
      FALSE, ch, 0, new_target, TO_CHAR);
  }
  
  return new_target;
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"            */

bool circle_follow(P_char ch, P_char victim)
{
  P_char   k;

  for (k = victim; k; k = k->following)
  {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!    */

void stop_follower(P_char ch)
{
  struct follow_type *j, *k;
  int      i;

  if (!(ch && ch->following))
  {
    logit(LOG_DEBUG, "assert: bogus parms (stop_follower - ch or ch->following is NULL)");
		return;
  }

  // If ch is a charmie who has charmies?
  // IE Charm person on a necro with pets.
  if( GET_MASTER(ch) )
  {
    clear_links(ch, LNK_PET);
    /* clearing a charm/pet link might have stopped the following already... */
    // In fact, this should always be true.
    if( !ch->following )
    {
      return;
    }

    if( IS_NPC(ch) )
    {
      REMOVE_BIT(ch->specials.act, ACT_STAY_ZONE);
      REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
    }
  }
  else
  {
    act("You stop following $N.", FALSE, ch, 0, ch->following, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->following, TO_NOTVICT);
    act("$n stops following you.", TRUE, ch, 0, ch->following, TO_VICT);
  }
  
  if (ch->following->followers->follower == ch)
  {                             /* Head of follower-list?  */
    k = ch->following->followers;
    ch->following->followers = k->next;
    FREE(k);
  }
  else
  {                             /* locate follower who is not head of list  */
    for (k = ch->following->followers;
         k->next && (k->next->follower != ch); k = k->next) ;

    if (!k->next)
    {
      logit(LOG_EXIT, "can't find follower in follower list");
      raise(SIGSEGV);
    }
    j = k->next;
    k->next = j->next;
    FREE(j);
  }

  /* NPCS have to be following to be grouped */
  if (IS_NPC(ch) && (ch->group))
    group_remove_member(ch);
  ch->following = 0;
}

/* boggle, can't believe this didn't exist before.  JAB */

void stop_all_followers(P_char ch)
{
  struct follow_type *j, *k;

  if (!ch)
  {
    logit(LOG_DEBUG, "NULL ch in stop_all_followers");
    return;
  }
  
  if(ch->followers) 
    for (k = ch->followers; k; k = j)
    {
      j = k->next;
      if (k->follower)
        stop_follower(k->follower);
    }
}

/*
   Called when a character that follows/is followed dies
 */
// We want ch's followers to die.
void die_follower(P_char ch)
{
  struct follow_type *j, *k;

  if( ch->following )
    stop_follower(ch);

  // Kill/stop all their followers!  Rawr!
  j = ch->followers;
  while( j )
  {
    k = j->next;

    // If their follower is a pet... (we don't kill the soldiers following a commander).
    if( IS_ALIVE(j->follower) )
    {
      // If they're a pet, kill them!
      if( GET_MASTER(j->follower) == ch )
      {
        die( j->follower, j->follower);
      }
      // Otherwise, just stop following.
      else
      {
        stop_follower(j->follower);
      }
    }

    j = k;
  }

}

#if 0
void petrestore(P_char ch, char *id)
{
  P_char   mob;

  if ((mob = restorePet(id)))
  {
    SET_BIT(mob->specials.affected_by, AFF_CHARM);
    if (ch->in_room != NOWHERE)
      char_to_room(mob, ch->in_room, 0);
    else
      char_to_room(mob, GET_HOME(ch), 0);

    if (!isname(GET_NAME(ch), mob->only.npc->owner))
    {
      logit(LOG_DEBUG, "Oops! Loaded pet belonging to %s and gave to %s!",
            mob->only.npc->owner, GET_NAME(ch));
      extract_char(mob);
      return;
    }
    deletePet(id);
    return;
  }
  logit(LOG_DEBUG, "Could not load pet #%s for %s!", id, GET_NAME(ch));
  wizlog(OVERLORD, "Could not load pet #%s for %s!", id, GET_NAME(ch));
  return;
}
#endif


/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader */

void add_follower(P_char ch, P_char leader)
{
  int      i;
  struct follow_type *k;

  if (!(ch && leader))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
 
#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You're too busy following someone now to carry that.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  if( !IS_ALIVE(ch) || !IS_ALIVE(leader) )
    return;
  
  if (circle_follow(ch, leader))
    return;

  ch->following = leader;

  CREATE(k, follow_type, 1, MEM_TAG_FOLLOW);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (IS_PC(ch) || (GET_VNUM(ch) != 4010 &&
                    GET_VNUM(ch) != 250))
  {
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
    act("$n now follows $N.", TRUE, ch, 0, leader, TO_NOTVICT);
  }
}

/* shamans used to be considered type cleric, they aren't anymore, so or
   it with 4 ...  may cause other bugs outside this file */
int spell_class(int spl)
{
  int      i, j = 0;

  for (i = 0; i < CLASS_COUNT; i++)
    if ((skills[spl].m_class[i].rlevel[0] > 0) &&
        (skills[spl].m_class[i].rlevel[0]<= MAXLVLMORTAL))
      switch (1 << (i + 1))
      {
      case CLASS_SORCERER:
      case CLASS_NECROMANCER:
      case CLASS_CONJURER:
      case CLASS_SUMMONER:
      case CLASS_RANGER:
      case CLASS_WARLOCK:
      case CLASS_REAVER:
      case CLASS_ILLUSIONIST:
        j |= 1;
        break;
      case CLASS_CLERIC:
      case CLASS_DRUID:
      case CLASS_BLIGHTER:
      case CLASS_PALADIN:
      case CLASS_ETHERMANCER:
      case CLASS_ANTIPALADIN:
        j |= 2;
        break;
      case CLASS_SHAMAN:
        j |= 4;
        break;
      }
  return j;
}

#define IS_MAGESPELL(spl) ((spell_class((spl)) & 1))
#define IS_CLERICSPELL(spl) ((spell_class((spl)) & 2))
#define IS_SHAMANSPELL(spl) ((spell_class((spl)) & 4))

void say_spell(P_char ch, int si)
{
  char     splwd[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int      j, offs;
  bool     space;
  P_char   tch;

  struct syllable
  {
    char     org[10];
    char     nnew[10];
  };

  struct syllable syls[] = {
    {" ", " "},
    {"ar", "abra"},
    {"au", "kada"},
    {"bless", "fido"},
    {"blind", "nose"},
    {"bur", "mosa"},
    {"cu", "judi"},
    {"de", "oculo"},
    {"en", "unso"},
    {"light", "dies"},
    {"lo", "hi"},
    {"mor", "zak"},
    {"move", "sido"},
    {"ness", "lacri"},
    {"ning", "illa"},
    {"per", "duda"},
    {"ra", "gru"},
    {"re", "candus"},
    {"son", "sabru"},
    {"tect", "infra"},
    {"tri", "cula"},
    {"ven", "nofo"},
    {"a", "a"},
    {"b", "b"},
    {"c", "q"},
    {"d", "e"},
    {"e", "z"},
    {"f", "y"},
    {"g", "o"},
    {"h", "p"},
    {"i", "u"},
    {"j", "y"},
    {"k", "t"},
    {"l", "r"},
    {"m", "w"},
    {"n", "i"},
    {"o", "a"},
    {"p", "s"},
    {"q", "d"},
    {"r", "f"},
    {"s", "g"},
    {"t", "h"},
    {"u", "j"},
    {"v", "z"},
    {"w", "x"},
    {"x", "n"},
    {"y", "l"},
    {"z", "k"},
    {"-", "-"},
    {"", ""}
  };

  if(IS_NPC(ch) &&
    !ch->desc &&
    ALONE(ch))
  {
    return;
  }
  
  strcpy(Gbuf1, "");
  strcpy(splwd, skills[si].name);

  offs = 0;

  while (*(splwd + offs))
  {
    for (j = 0; *(syls[j].org); j++)
      if (strn_cmp(syls[j].org, splwd + offs, strlen(syls[j].org)) == 0)
      {
        strcat(Gbuf1, syls[j].nnew);
        if (strlen(syls[j].org))
          offs += strlen(syls[j].org);
        else
          ++offs;
      }
  }

  space = FALSE;

  offs = strlen(Gbuf1);
  for (j = 0; Gbuf1[j]; j++)
    if (Gbuf1[j] == ' ')
      space = TRUE;

  sprintf(Gbuf2, "$n utters the word%s '%s'", space ? "s" : "", Gbuf1);
  sprintf(Gbuf1, "$n utters the word%s '%s'", space ? "s" : "", skills[si].name);

// This for allows players who hear a spell being casted the opportunity to notch.
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if(tch->in_room != ch->in_room) // Not in same room.
    {
      continue;
    }

    if(IS_NPC(tch)) // Shouldn't get this far.
    {
      continue;
    }

    if(IS_IMMOBILE(tch))
    {
      continue;
    }

    if(GET_STAT(tch) != STAT_NORMAL)
    {
      continue;
    }

    if( tch != ch && IS_PC(tch) )
    {
      int rand = number(1, 101);

      if((IS_MAGESPELL(si) &&
         rand >= GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_MAGICAL) &&
         GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_MAGICAL) > 1) ||
        (IS_CLERICSPELL(si) &&
         rand >= GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_CLERICAL) &&
         GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_CLERICAL) > 1) ||
        (IS_SHAMANSPELL(si) &&
         rand >= GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_SHAMAN) &&
         GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_SHAMAN) > 1))
      {
        act(Gbuf1, FALSE, ch, 0, tch, TO_VICT | ACT_SILENCEABLE);

        if( IS_MAGESPELL(si) && IS_MAGE(tch) )
        {
          notch_skill(tch, SKILL_SPELL_KNOWLEDGE_MAGICAL, 1);
        }
        else if( IS_CLERICSPELL(si) && (IS_CLERIC(tch) || IS_HOLY(tch)) )
        {
          notch_skill(tch, SKILL_SPELL_KNOWLEDGE_CLERICAL, 1);
        }
        else if( IS_SHAMANSPELL(si) && GET_CLASS(tch, CLASS_SHAMAN) )
        {
          notch_skill(tch, SKILL_SPELL_KNOWLEDGE_SHAMAN, 1);
        }
      }
      else
      {
        act(Gbuf2, FALSE, ch, 0, tch, TO_VICT | ACT_SILENCEABLE);
      }
    }
  }
}

int SpellCastTime(P_char ch, int spl)
{
  int      dura;

  dura = (int)skills[spl].beats;
// if( IS_PC(ch) ) debug( "SpellCastTime: initial beats: %d, PULSE_SPELLCAST: %d", dura, PULSE_SPELLCAST );
  dura = (dura * spell_pulse_data[GET_RACE(ch)]);
// if( IS_PC(ch) ) debug( "SpellCastTime: beats with racial pulse: %d", dura );
  // Affects all racial modifiers.
  dura = (dura + get_property("spellcast.pulse.racial.All", 1.000));
// if( IS_PC(ch) ) debug( "SpellCastTime: beats with racial.All: %d", dura );
  // SPELL_PULSE is a standard modifier, used here, in do_score, and do_stat char..
  dura = dura * SPELL_PULSE(ch);
// if( IS_PC(ch) ) debug( "SpellCastTime: beats with ch->spell_pulse: %d", dura );

  if (IS_AFFECTED2(ch, AFF2_FLURRY))
  {
    dura = (dura * .3);
  }
  else if (IS_AFFECTED(ch, AFF_HASTE))
  {
    dura = (dura * .8);
  }

//  if( IS_PC(ch) ) debug( "SpellCastTime: beats final: %d", dura );
  return MAX(1, dura);
}

void SpellCastShow(P_char ch, int spl)
{
  P_char   tch;
  int      idok, detharm;
  char     Gbuf1[MAX_STRING_LENGTH];

  if(!(ch))
  {
    return;
  }

  if( GET_CLASS(ch, CLASS_MINDFLAYER) || GET_CLASS(ch, CLASS_PSIONICIST) )
  {
    return;
  }

  for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
  {
    if( tch == ch )
    {
      continue;
    }
    idok = 0;
    detharm = 0;
    int rand = number(1, 101);

    if (IS_SHAMANSPELL(spl) && (rand <= GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_SHAMAN)))
      idok = 3;
    if (IS_CLERICSPELL(spl) && (rand <= GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_CLERICAL)))
      idok = 2;
    if (IS_MAGESPELL(spl) && (rand <= GET_CHAR_SKILL(tch, SKILL_SPELL_KNOWLEDGE_MAGICAL)))
      idok = 1;

    if (idok)
    {
      detharm = 1;
      /*
         has chance of recognizing the spell fellow starts to cast
       */
      /*
         (and 100% chance of recognizing whether it is harmful or not)
       */
      if (number(1, 100) > 60)
        idok = 0;
    }
    if (!detharm && (GET_C_INT(ch) > number(1, 100)))
      detharm = 1;

    if (IS_TRUSTED(tch))
      detharm = idok = 1;

    sprintf(Gbuf1, "$n starts casting %s spell%s%s%s.",
            (detharm && IS_AGG_SPELL(spl)) ? "an offensive" : "a",
            idok ? " called '" : "", idok ? skills[spl].name : "",
            idok ? "'" : "");
    act(Gbuf1, TRUE, ch, 0, tch, TO_VICT);

    if (idok == 1)
      notch_skill(ch, SKILL_SPELL_KNOWLEDGE_MAGICAL, 1);
    else if (idok == 2)
      notch_skill(ch, SKILL_SPELL_KNOWLEDGE_CLERICAL, 1);
    else if (idok == 3)
      notch_skill(ch, SKILL_SPELL_KNOWLEDGE_SHAMAN, 1);
  }
}

void update_saving_throws()
{
  char buf[256];

  for (int i = SAVING_PARA; i <= SAVING_SPELL; i++)
  {
    sprintf(buf, "saves.%s.starting", saves_data[i].name);
    saves_data[i].starting = get_property(buf, 70);
    sprintf(buf, "saves.%s.top", saves_data[i].name);
    saves_data[i].top = get_property(buf, 20);
  }
}

int find_save(P_char ch, int save_type)
{
  int save;
  char buf[256];

  save = saves_data[save_type].starting -
    (GET_LEVEL(ch) * (saves_data[save_type].starting - saves_data[save_type].top))/60;

  sprintf(buf, "saves.%s.racial.%s", saves_data[save_type].name, 
      race_names_table[GET_RACE(ch)].no_spaces);

  save += get_property(buf, 0, false);

  sprintf(buf, "saves.%s.class.%s", saves_data[save_type].name, 
      class_names_table[flag2idx(ch->player.m_class)].normal);

  save += get_property(buf, 0, false);

  if (IS_NPC(ch))
    save -= GET_LEVEL(ch) / 3;

  return save;
}

/*
   ok, NewSaves is a more flexible version of saves_spell, to remain backwards
   compatible, mod is a modification to the SAVE, not the roll, thus a negative
   mod makes the save more likely to return TRUE, from the saving character's
   point of view, less is more.  Also, mobs now save as a PC of their level,
   as a Warrior if no ACT_HAS_<class> bits are set, or as the best.  -JAB
 */

bool NewSaves(P_char ch, int save_type, int mod)
{
  int  save;
  bool i;

  if( !ch || (save_type < SAVING_PARA) || (save_type > SAVING_SPELL) )
  {
    logit(LOG_DEBUG, "Invalid arguments to NewSaves");
    debug( "NewSaves: %s has invalid argument: %d.", ch ? J_NAME(ch) : "Nobody", save_type );
    return FALSE;
  }

  if( IS_TRUSTED(ch) )
  {
    return TRUE;
  }

  // Calculate the base save amount with class/race mods.
  save = find_save(ch, save_type);
  //debug( "NewSaves: find_save = %d", save );

   /*
     save file scale has changed, so need to change meaning of the mods to it.
     For now, we just multiply the mod by 5.
   */
  save += (ch->specials.apply_saving_throw[save_type] + mod) * 5;

  //debug( "NewSaves: apply_sv_throw[%d] = %d", save_type, ch->specials.apply_saving_throw[save_type] );
  //debug( "NewSaves: final save = %d", save );

  /* always 1% chance to fail/save regardless of saving throw  */
  i = (BOUNDED(1, save, 99) < number(1, 100));

  // Quick thinking gives you a second chance to save.
  if( has_innate(ch, INNATE_QUICK_THINKING) && (i == FALSE) )
  {
    return (BOUNDED(1, save, 99) < number(1, 100));
  }
  else
    return i;
}

/* left in for compatibility. JAB */

bool saves_spell(P_char ch, int save_type)
{
  return (NewSaves(ch, save_type, 0));
}

char    *skip_spaces(char *string)
{
  for (; *string && ((*string) == ' '); string++) ;

  return (string);
}

void show_abort_casting(P_char ch)
{
  struct spellcast_datatype *data;
  P_nevent e1;

  if (IS_SET(ch->specials.affected_by2, AFF2_CASTING))
  {
    if (meming_class(ch))
    {
      send_to_char("&+rYou abort your spell before it's done!\n", ch);
      act("$n&n&+r stops invoking abruptly!", TRUE, ch, 0, 0, TO_ROOM);
    }
    else if (GET_CLASS(ch, CLASS_PSIONICIST) ||
             GET_CLASS(ch, CLASS_MINDFLAYER))
    {
      send_to_char
        ("&+rYou abort your mental image before it has become reality!\n",
         ch);
      act("$n&n&+r's face flushes white for a moment.", TRUE, ch, 0, 0,
          TO_ROOM);
    }
    else
    {
      send_to_char("&+rYou abort your prayer before it's done!\n", ch);
      act("$n&n&+r stops chanting abruptly!", TRUE, ch, 0, 0, TO_ROOM);
    }
    LOOP_EVENTS_CH( e1, ch->nevents )
    {
      if ( e1->func == event_spellcast)
      {
        data = (struct spellcast_datatype*)e1->data;
        if (data && data->arg)
        {
          FREE(data->arg);
          data->arg = NULL;
        }
      }
    }
    disarm_char_nevents(ch, event_spellcast);
    REMOVE_BIT(ch->specials.affected_by2, AFF2_CASTING);
  }
}

void StopCasting(P_char ch)
{
  show_abort_casting(ch);

  clear_links(ch, LNK_CAST_ROOM);
  clear_links(ch, LNK_CAST_WORLD);
}

bool ground_casting_check(P_char ch, int spl)
{
  /* check for if they were casting but just bashed, then check their groundcast skill */
     //  notch_skill(ch, SKILL_GROUND_CASTING, get_property("skill.notch.groundCasting", 60) ) )
  if( IS_SET(ch->specials.affected_by2, AFF2_CASTING) && !IS_SET(skills[spl].targets, TAR_NOCOMBAT)
    && (( number(0,100) < (int) (GET_CHAR_SKILL(ch, SKILL_GROUND_CASTING) / 2) ) ||
      ( number(0,120) < (int) (GET_CHAR_SKILL(ch, SKILL_CONCENTRATION) / 2) )) )
  {
    act("$n continues preparing $s spell from the ground...", FALSE, ch, 0, 0, TO_ROOM);
    act("You continue preparing your spell from the ground...", FALSE, ch, 0, 0, TO_CHAR);
    return TRUE;
  }
  return FALSE;
}

/*
   this is simplistic part, which just checks for _most_ obvious stuff
   like char moving around etc. this is called once / second.
 */

bool cast_common_generic(P_char ch, int spl)
{
  if ( is_silent(ch, FALSE) && 
       !GET_CLASS(ch, CLASS_PSIONICIST) && 
       !GET_CLASS(ch, CLASS_MINDFLAYER) &&
       !GET_CHAR_SKILL(ch, SKILL_SILENT_SPELL) )
  {
    send_to_char("You move your lips, but no sound comes forth!\n", ch);
    return FALSE;
  }
  
  if (CHAR_IN_SAFE_ZONE(ch) && IS_AGG_SPELL(spl))
  {
    send_to_char("You may not cast harmful magic here!\n", ch);
    return FALSE;
  }
  /*
     change, all spells were either POSITION_STANDING or POSITION_FIGHTING so I

     just changed min_pos to a bool in_battle flag (like commands). In vast
     majority of cases, these switches will never be used, but it's belt and
     suspenders time.  JAB
   */
  if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (GET_STAT(ch))
    {
    case STAT_DEAD:
      send_to_char("Corpses make really pitiful spellcasters.\n", ch);
      break;
    case STAT_DYING:
    case STAT_INCAP:
      send_to_char("You are too busy bleeding to death at the moment.\n", ch);
      break;
    case STAT_SLEEPING:
      send_to_char("You dream about great magical powers.\n", ch);
      break;
    case STAT_RESTING:
      send_to_char("You can't concentrate enough while resting.\n", ch);
      break;
    }
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      send_to_char("Standing would be a good first step.\n", ch);
      break;
    case POS_KNEELING:
      if( ground_casting_check(ch, spl))
        {
         //send_to_char("&+Wgroundcast success&n\n", ch);
        return TRUE;
        }
      send_to_char("Get off your knees!\n", ch);
      break;
    case POS_SITTING:
      if( ground_casting_check(ch, spl))
        {
         //send_to_char("&+Wgroundcast success&n\n", ch);
        return TRUE;
        }
      send_to_char("You can't do this sitting!\n", ch);
      break;
    }
    return FALSE;
  }
  else if(((IS_FIGHTING(ch) || IS_DESTROYING(ch)) &&
           IS_SET(skills[spl].targets, TAR_NOCOMBAT) && 
           !IS_NPC(ch)) ||
           IS_STUNNED(ch))
  {
    send_to_char("Impossible! You can't concentrate enough!\n", ch);
    return FALSE;
  }
  else if (IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
           IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
  {
    if (!GET_CLASS(ch, CLASS_PSIONICIST))
    {
      send_to_char("Too bad, looks like you've been stopped in your tracks!\n", ch);
      return FALSE;
    }
  }
  return TRUE;
}


bool parse_spell_arguments(P_char ch, struct spell_target_data * data, char *argument)
{
  P_char   vict;
  P_obj    obj;
  char    *tar_arg;
  char     Gbuf1[512];
  char     ranged_arg[512];
  bool     target_ok;
  int      i, range;
  int      spl = data->ttype;

  for (; *argument == ' '; argument++) ;

  tar_arg = data->arg = argument;

  strcpy(ranged_arg, argument);

  /* **************** Locate targets ****************  */

  target_ok = FALSE;
  data->t_char = vict = NULL;
  data->t_obj = obj = NULL;

  if( !IS_SET(skills[spl].targets, TAR_IGNORE) && !IS_SET(skills[spl].targets, TAR_OFFAREA) )
  {
    argument = one_argument(argument, Gbuf1);

    if( *Gbuf1 )
    {
      if( !target_ok && IS_SET(skills[spl].targets, TAR_SELF_ONLY) )
      {
        if( str_cmp(GET_NAME(ch), Gbuf1) == 0 )
        {
          vict = ch;
          target_ok = TRUE;
        }
      }

      if (!target_ok && IS_SET(skills[spl].targets, TAR_CHAR_ROOM))
      {
        if (!*argument)
        {
          if ((vict = get_char_room_vis(ch, Gbuf1)))
          {
            target_ok = TRUE;
          }
        }
      }

      if (!target_ok && IS_SET(skills[spl].targets, TAR_OBJ_INV))
        if ((obj = get_obj_in_list_vis(ch, Gbuf1, ch->carrying)))
          target_ok = TRUE;

      if (!target_ok && IS_SET(skills[spl].targets, TAR_OBJ_EQUIP))
      {
        for (i = 0; i < MAX_WEAR && !target_ok; i++)
          if (ch->equipment[i] && isname(Gbuf1, ch->equipment[i]->name))
          {
            obj = ch->equipment[i];
            target_ok = TRUE;
          }
      }

      if (!target_ok && IS_SET(skills[spl].targets, TAR_OBJ_ROOM))
        if ((obj =
             get_obj_in_list_vis(ch, Gbuf1, world[ch->in_room].contents)))
          target_ok = TRUE;

      if (!target_ok && IS_SET(skills[spl].targets, TAR_CHAR_RANGE))
      {
        range = IS_SET(skills[spl].targets, TAR_RANGE2) ? 2 : 1;
        if ((vict = get_char_ranged_vis(ch, ranged_arg, range)))
        {
          if (world[vict->in_room].room_flags & SINGLE_FILE)
          {
            send_to_char("It's too cramped in there to cast accurately.\n", ch);
            return FALSE;
          }

          if ((world[vict->in_room].room_flags & ARENA) !=
              (world[ch->in_room].room_flags & ARENA))
          {
            send_to_char("You wouldn't want to piss off the arena authorities..\n", ch);
            return FALSE;
          }

          target_ok = TRUE;
        }
      }

      if (!target_ok && IS_SET(skills[spl].targets, TAR_AREA))
      {
        if (!*argument)
          if ((vict = get_char_room_vis(ch, Gbuf1)))
            target_ok = TRUE;
      }

      if (!target_ok && IS_SET(skills[spl].targets, TAR_OBJ_WORLD))
        if ((obj = get_obj_vis(ch, Gbuf1)))
          target_ok = TRUE;

      if (!target_ok && IS_SET(skills[spl].targets, TAR_CHAR_WORLD))
      {
        // ALWAYS true for tar_char_world - let spells handle the situation!
        target_ok = TRUE;
        vict = get_char_vis(ch, Gbuf1);
        if (!vict || (!is_introd(vict, ch)) ||
            (IS_PC(ch) && IS_PC(vict) && racewar(ch, vict)))
        {
          P_char   dummy = 0;

          for (dummy = world[real_room(666)].people; dummy;
               dummy = dummy->next_in_room)
            if (IS_NPC(dummy) && GET_VNUM(dummy) == 46)
              break;

          if (!dummy)
            if ((dummy = read_mobile(46, VIRTUAL)) == FALSE)
            {
              vict = 0;
            }
            else
            {
              char_to_room(dummy, real_room0(666), -1);
            }

          if (dummy)
            vict = dummy;
          else
            vict = 0;
        }
      }

      if (!target_ok && IS_SET(skills[spl].targets, TAR_WALL))
      {
        int var = dir_from_keyword(Gbuf1);

        if (check_visible_wall(ch, var))
        {
          obj = get_wall_dir(ch, var);
        }

        if (obj)
      	  target_ok = TRUE;
      }

//    if ((vict = get_char_vis(ch, Gbuf1))) {
      //      if (!racewar(ch, vict)) target_ok = TRUE;  /* can't be too careful */
      //      if (!is_introd(vict, ch)) target_ok = FALSE;
      //   }

    }
    else
    {                           /* No argument was typed  */
      if (!target_ok && IS_SET(skills[spl].targets, TAR_SELF_ONLY))
      {
        vict = ch;
        target_ok = TRUE;
      }
      if (!target_ok && IS_SET(skills[spl].targets, TAR_AREA))
      {
        target_ok = TRUE;
      }
      if (IS_SET(skills[spl].targets, TAR_FIGHT_SELF))
        if (ch->specials.fighting)
        {
          vict = ch;
          target_ok = TRUE;
        }
      if (!target_ok && IS_SET(skills[spl].targets, TAR_FIGHT_VICT))
        if (ch->specials.fighting &&
            (GET_STAT(ch->specials.fighting) != STAT_DEAD) &&
            (ch->specials.fighting->in_room == ch->in_room))
        {
//          if( GET_SPEC(ch, CLASS_CLERIC, SPEC_HOLYMAN)
//            || ( spl != SPELL_HEAL && spl != SPELL_FULL_HEAL ) )
          {
            /* WARNING, MAKE INTO POINTER  */
            vict = ch->specials.fighting;
            target_ok = TRUE;
          }
        }
    }
  }
  else
  {
    if (IS_SET(skills[spl].targets, TAR_CHAR_RANGE) && *ranged_arg)
    {
      range = IS_SET(skills[spl].targets, TAR_RANGE2) ? 2 : 1;
      if ((vict = get_char_ranged_vis(ch, ranged_arg, range)))
      {
        if (world[vict->in_room].room_flags & SINGLE_FILE)
        {
          send_to_char("It's too cramped in there to cast accurately.\n", ch);
          return FALSE;
        }

        target_ok = TRUE;
      }
    }
    else
      target_ok = TRUE;

    /*  if (IS_SET(skills[spl].targets, TAR_IGNORE) && !target_ok) {
       argument = one_argument(argument, Gbuf1);
       if (*Gbuf1) {
       if ((vict = get_char_room_vis(ch, Gbuf1)))
       target_ok = TRUE;
       } else
       target_ok = TRUE;
       } */
  }

  if (!target_ok)
  {
    if (*Gbuf1)
    {
      if (IS_SET(skills[spl].targets, TAR_CHAR_ROOM))
        send_to_char("&+CYou failed.\n", ch);
      else if (IS_SET(skills[spl].targets, TAR_CHAR_WORLD))
        send_to_char("&+CYou failed.\n", ch);
      else if (IS_SET(skills[spl].targets, TAR_CHAR_RANGE))
        send_to_char("&+CYou failed.\n", ch);
      else if (IS_SET(skills[spl].targets, TAR_OBJ_INV))
        send_to_char("&+CYou are not carrying anything like that.\n", ch);
      else if (IS_SET(skills[spl].targets, TAR_OBJ_ROOM))
        send_to_char("&+CYou failed.\n", ch);
      else if (IS_SET(skills[spl].targets, TAR_OBJ_WORLD))
        send_to_char("&+CYou failed.\n", ch);
      else if (IS_SET(skills[spl].targets, TAR_OBJ_EQUIP))
        send_to_char("&+CYou are not wearing anything like that.\n", ch);
      else if (IS_SET(skills[spl].targets, TAR_OBJ_WORLD))
        send_to_char("&+CYou failed.\n", ch);
      else if (IS_SET(skills[spl].targets, TAR_SELF_ONLY))
        send_to_char("&+rYou can only cast this spell upon yourself.\n", ch);
    }
    else
    {                           /* Nothing was given as argument  */
      if( !IS_SET(skills[spl].targets, TAR_OBJ_INV) )
        send_to_char("&+WWho should the spell be cast upon?  You must specify a target!\n", ch);
      else
        send_to_char("&+WWhat should the spell be cast upon?  You must specify a target!\n", ch);
    }
    return FALSE;
  }

  if ((vict == ch) && IS_SET(skills[spl].targets, TAR_SELF_NONO))
  {
    send_to_char("&+WYou cannot cast this spell upon yourself.\n", ch);
    return FALSE;
  }
  else if ((vict != ch) && IS_SET(skills[spl].targets, TAR_SELF_ONLY))
  {
    send_to_char("&+WYou can only cast this spell upon yourself.\n", ch);
    return FALSE;
  }
  if (IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    // TAR_AREA + TAR_CHAR_ROOM = Can target either the whole room, or a single character
    //   This is NOT the same as targetting a group within the room via TAR_AREA + target name argument.
    //   The only two spells like this are 'creeping doom' and 'acid rain' as of 6/28/2015.
    if( IS_SET(skills[spl].targets, TAR_AREA) && !IS_SET(skills[spl].targets, TAR_CHAR_ROOM) )
    {
      send_to_char("&+WThere's no way you can safely cast such a spell in such a tight area.\n", ch);
      return FALSE;
    }

    // ranged spells arleady checked elsewhere

    if( !AdjacentInRoom(ch, vict) && (vict != ch) )
    {
      send_to_char("&+WYou feel too uncomfortable casting at someone that far away in such tight quarters.\n", ch);
      return FALSE;
    }
  }
  if( GET_MASTER(ch) )
  {
    if ((((                     /*IS_SET(skills[spl].targets, TAR_CHAR_WORLD) || */
            IS_SET(skills[spl].targets, TAR_CHAR_RANGE) ||
            IS_SET(skills[spl].targets, TAR_CHAR_ROOM)) &&
          (GET_MASTER(ch) == vict)) ||
         (IS_SET(skills[spl].targets, TAR_IGNORE) &&
          should_area_hit(ch, GET_MASTER(ch)))) && IS_AGG_SPELL(spl))
    {
      send_to_char("You are afraid that it could harm your master.\n", ch);
      return FALSE;
    }
  }
  data->t_char = vict;
  data->t_obj = obj;
  data->arg = tar_arg;

  return TRUE;
}

bool parse_spell(P_char ch, char *argument, struct spell_target_data* target_data, int cmd)
{
  int      qend;
  int      free_slots;
  int      circle;
  char     Gbuf1[MAX_STRING_LENGTH], ranged_arg[MAX_STRING_LENGTH];
  int      spl = 0;
  P_obj    tar_obj = 0;
  P_char   tar_char = 0;
  char    *tar_arg = 0;

  argument = skip_spaces(argument);

  if( IS_DISGUISE_SHAPE(ch) )
  {
    send_to_char("You cannot use magic in that form!\n", ch);
    return FALSE;
  }

  /*
     If there is no chars in argument
   */
  if (!(*argument))
  {
    if (GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER))
      send_to_char("Will what to happen?\n", ch);
    else
      send_to_char("Cast which what where?\n", ch);
    return FALSE;
  }

  if (*argument != '\'')
  {
    if (GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER))
      send_to_char
        ("To will that into reality, you must think of it enclosed in the mystical 'apostrophes'.\n",
         ch);
    else
      send_to_char
        ("Magic must always be enclosed by the holy symbols known only as 'apostrophes'.\n",
         ch);
    return FALSE;
  }
  /*
     Locate the last quote && lowercase the magic words (if any)
   */

  for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
    *(argument + qend) = LOWER(*(argument + qend));

  if (*(argument + qend) != '\'')
  {
    send_to_char
      ("Spells are always to be enclosed by the holy symbols known only as 'apostrophes'.\n",
       ch);
    return FALSE;
  }
  spl = old_search_block(argument, 1, (uint) (MAX(0, (qend - 1))), spells, 0) - 1;

  if (spl < 0 || (!IS_SPELL(spl) && !(IS_TRUSTED(ch) && IS_POISON(spl))))
  {
    switch (number(1, 5))
    {
    case 1:
      send_to_char("Excuse me?\n", ch);
      break;
    case 2:
      send_to_char("Um . . . you made a typo!\n", ch);
      break;
    case 3:
      send_to_char("You ok??\n", ch);
      break;
    case 4:
      send_to_char("Sorry, typo, try again!\n", ch);
      break;
    default:
      send_to_char("Sorry, you spelled it wrong!\n", ch);
      break;
    }
    return FALSE;
  }

  if (!(circle = knows_spell(ch, spl)) && !quested_spell(ch, spl))
  {
    send_to_char("You don't know that spell!\n", ch);
    return FALSE;
  }
  memset(target_data, 0, sizeof(struct spell_target_data));
  target_data->ttype = spl;

  if( IS_TRUSTED(ch) )
    ;
  else if( USES_MANA(ch))
//else if (GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER))
  {
    if (GET_MANA(ch) < 1 && circle != -1)
    {
      send_to_char("&+mYou don't have the energy left to alter reality!\n",
                   ch);
      return FALSE;
    }
  }
  else if (USES_SPELL_SLOTS(ch))
  {
    if (circle != -1 && !ch->specials.undead_spell_slots[circle])
    {
      if (GET_CLASS(ch, CLASS_DRUID) || GET_CLASS(ch, CLASS_RANGER))
        send_to_char("&+gYou must commune with nature more before "
                     "invoking its power.\n", ch);
/*
      else if (GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_MINDFLAYER))
      {
        send_to_char("&+mYour thoughts have not collected enough to cast THAT spell.&n\n", ch);
      }
*/
      else if (IS_ANGEL(ch))
      {
	      send_to_char("&+WYour illumination is not sufficient enough to cast that spell.&n\n", ch);
      }
      else if( USES_TUPOR(ch) )
      {
	      send_to_char("&+CYour &+Wlucidity &+Cis not sufficient enough &+cto cast that spell.&n\n", ch);
      }
      else
        send_to_char("&+LYour power reserves are not sufficient to cast that spell!\n", ch);
      return FALSE;
    }
  }
  else if ((circle != -1) && !IS_TRUSTED(ch))
  {
    send_to_char("&+RYou don't have that spell memorized.\n", ch);
    return FALSE;
  }

  if (IS_SPELL(spl) && (skills[spl].spell_pointer == 0))
  {
    send_to_char("Sorry, this magic has not yet been implemented :(\n", ch);
    return FALSE;
  }
  if (!cast_common_generic(ch, spl))
    return FALSE;

  /* check for shaman's totem */

  if(ch && GET_CLASS(ch, CLASS_SHAMAN) && !IS_NPC(ch) && !hasTotem(ch, spl))
  {
    if (IS_TRUSTED(ch))
    {
      send_to_char("You don't have the right totem, but since you're a god..\n", ch);
    }
    else if(IS_MULTICLASS_PC(ch))
    {
      ; //try to handle  totems for shaman spells..
    }
    else if(GET_CHAR_SKILL(ch, SKILL_TOTEMIC_MASTERY) > number(1, 100))
    {
      send_to_char("Using your mastery of the spirit realm, you prepare your spell without the aid of a focus...\n", ch);
    }
    else
    {
      send_to_char("You aren't holding the correct totem to cast that spell.\n", ch);
      // CharWait(ch, PULSE_VIOLENCE);
      return FALSE;
    }
  }
  argument += qend + 1;         /* Point to the last '  */

  if (cmd != CMD_SPELLWEAVE &&
      !parse_spell_arguments(ch, target_data, argument))
    return FALSE;

  return true;
}

bool parse_spell(P_char ch, char *argument,  struct spell_target_data* target_data)
{
  return parse_spell(ch, argument, target_data, CMD_CAST);
}

// Returns TRUE if ch is killed, FALSE otherwise.
bool check_mob_retaliate(P_char ch, P_char tar_char, int spl)
{
  P_char tch, tch2;

  // This should never be the case, but just to be careful..
  if( !IS_SET(skills[spl].targets, TAR_AGGRO) || !IS_ALIVE(ch) )
  {
    debug( "check_mob_retaliate: Non-aggro (%s) or dead/missing ch: %s is %s.",
      IS_SET(skills[spl].targets, TAR_AGGRO) ? "NO" : "YES",
      !ch ? "NULL" : J_NAME(ch), IS_ALIVE(ch) ? "ALIVE" : "DEAD" );
    return TRUE;
  }

  // If TAR_IGNORE: Unknown target type(?), but aggro, ie area-only aggro like earthquake/nova/etc.
  //   TAR_OFFAREA: A correctly identified area offensive spell.
  //      TAR_AREA: +AGGRO +no target -> targettable to non-area spell (ie doom/acid rain), but aggro area otherwise.
  if( IS_SET(skills[spl].targets, TAR_IGNORE) || IS_SET(skills[spl].targets, TAR_OFFAREA)
    || (IS_SET(skills[spl].targets, TAR_AREA) && !tar_char) )
  {
    if( IS_AFFECTED(ch, AFF_INVISIBLE) )
    {
      appear(ch);
    }

    /* This is real wicked case, like a fellow casting a 'swarm or some such
     *  other aggro kick-ass spell. all unoccupied mobsters which would get hit
     *  by spl do automagical tackle at fellow! - they better be occupied, OR...
     */
    for( tch = world[ch->in_room].people; tch; tch = tch2 )
    {
      tch2 = tch->next_in_room;

      if( tch == ch )
      {
        continue;
      }
      if( IS_AFFECTED2(tch, AFF2_MINOR_PARALYSIS) || IS_AFFECTED2(tch, AFF2_MAJOR_PARALYSIS) )
      {
        continue;
      }
      if( !CAN_SEE(tch, ch) || IS_FIGHTING(tch) || !CAN_ACT(tch) || GET_POS(tch) < POS_STANDING )
      {
        continue;
      }
      if( !should_area_hit(ch, tch) || (IS_PC(tch) && IS_PC(ch)) )
        continue;

      // If the below conditional is true, then ITS NOT AN AGGRO SPELL!
      if( ((spl == SPELL_HOLY_WORD) && !IS_EVIL(tch))
        || ((spl == SPELL_UNHOLY_WORD) && !IS_GOOD(tch))
        || ((spl == SPELL_VOICE_OF_CREATION)) && !IS_EVIL(tch) )
      {
        continue;
      }

      // Chance not to notice.
      if( number(1, 150) > (GET_LEVEL(tch) + STAT_INDEX(GET_C_INT(tch))) )
      {
        continue;
      }

      /* Justice hook:
       *  Okay.. at this point, the victim knows that he's about to get hit by an aggro
       *  area spell.  If he doesn't know it, he'll know when the spell goes off.
       *  If he's already fighting the caster , there is no reason to call this.
       *  The only problem with this code, is that someone else in the room might
       *  (or might not) notice the aggression.
       *  This code, unfortunatly, limits everyones knowledge to basically the same as
       *  the victims. :(
       */
      justice_witness(ch, tch, CRIME_ATT_MURDER);

      if( IS_NPC(tch) )
      {
        MobStartFight(tch, ch);
      }
      else
      {
#ifndef NEW_COMBAT
        hit(tch, ch, tch->equipment[PRIMARY_WEAPON]);
#else
        hit(tch, ch, tch->equipment[WIELD], TYPE_UNDEFINED, getBodyTarget(tch), TRUE, FALSE);
#endif
      }
      if( !IS_ALIVE(ch) || !char_in_list(ch) )
      {
        return TRUE;
      }
    }
  }
  else if( IS_SET(skills[spl].targets, TAR_CHAR_RANGE) )
  {
    if( tar_char && (tar_char != ch) )
    {
      if( IS_AFFECTED(ch, AFF_INVISIBLE) )
      {
        appear(ch);
      }
      justice_witness(ch, tar_char, CRIME_ATT_MURDER);
    }
  }
  else
  {
    if( tar_char && (tar_char != ch) )
    {
      if( IS_AFFECTED(ch, AFF_INVISIBLE) )
      {
        appear(ch);
      }

      // A quick 'n dirty hostility check
      // If mob, automagically attacks fellow trying to cast offensive spell at them if able.
      if( IS_NPC(tar_char) && !IS_FIGHTING(tar_char)
        && CAN_SEE(tar_char, ch) && (GET_POS(ch) == POS_STANDING) )
      {
        if( number(1, 150) <= (GET_LEVEL(tar_char) + STAT_INDEX(GET_C_INT(tar_char))) )
        {
          // Justice hook: read notes above
          justice_witness(ch, tar_char, CRIME_ATT_MURDER);
          if( IS_DESTROYING(tar_char) )
          {
            stop_destroying(tar_char);
          }
          MobStartFight(tar_char, ch);

          if( !IS_ALIVE(ch) || !char_in_list(ch) )
          {
            return TRUE;
          }
        }
      }
    }
  }

  return FALSE;
}

extern void DelayCommune(P_char ch, int delay);

void do_will(P_char ch, char *argument, int cmd)
{
  int      dura, fail;
  int      splnum;
  P_char   kala, kala2;
  struct spellcast_datatype tmp_spl;
  char    *orig_arg;
  bool     is_tank = FALSE;
  int      spl;
  P_char tar_char;

  memset(&tmp_spl, 0, sizeof(tmp_spl));
  if (!USES_MANA(ch))
  {
    send_to_char("Your character lacks the training to enforce his will.\n", ch);
    return;
  }

  if (affected_by_spell(ch, SKILL_BERSERK) && !IS_TRUSTED(ch))
  {
    send_to_char("You are too filled with &+RRAGE&N to think!\n", ch);
    return;
  }

  orig_arg = argument;

  if( !parse_spell(ch, argument, &common_target_data) )
    return;

  // parse_spell parses argument and sets values on a global struct
  spl = common_target_data.ttype;
  tar_char = common_target_data.t_char;

  send_to_char("&+mYou begin to focus your will...\n", ch);
  dura = (SpellCastTime(ch, spl));

  // Handle for not having quick chant.. they're all fast-thinkers.
  dura >>= 1;

  if ((GET_CHAR_SKILL(ch, SKILL_SPATIAL_FOCUS) > 0) &&
      (5 + GET_CHAR_SKILL(ch, SKILL_SPATIAL_FOCUS) / 10 > number(0,100)))
  {
      int chant_bonus = MAX(0, GET_CHAR_SKILL(ch, SKILL_SPATIAL_FOCUS) / 40 + number(-1,1));
      if (chant_bonus > 1)
        send_to_char("&+MFocusing your mind, you bend reality a lot more easily...&n\n", ch);
      else
        send_to_char("&+MFocusing your mind, you bend reality a bit more easily...&n\n", ch);

      if (chant_bonus == 3) {
        CharWait(ch, 1);
        dura = 1;
      } else if (chant_bonus == 2) {
        CharWait(ch, dura >> 1);
        dura = 1;
      } else if (chant_bonus == 1) {
        CharWait(ch, dura * 0.8);
        dura = dura * 0.6;
      } else {
        CharWait(ch, dura);
        dura = dura * 0.8;
      }
  }
  else
  {
    CharWait(ch, dura);
  }

  splnum = spl;
  SpellCastShow(ch, spl);

  if (IS_SET(world[ch->in_room].room_flags, NO_PSI) && !IS_TRUSTED(ch))
  {
    send_to_char("Your psionic powers gather, then fade away.\n", ch);
    StopCasting(ch);
    return;
  }

  if( get_spell_from_room(&world[ch->in_room], SPELL_STARSHELL)
    && !IS_TRUSTED(ch) && number(0, 1) )
  {
    send_to_char("&+YYou are blinded by the light and lose your concentration.&n\n", ch);
    StopCasting(ch);
    return;
  }

  if( IS_AFFECTED5(ch, AFF5_MEMORY_BLOCK) && number(0, 1) )
  {
    send_to_char("You seem to have forgotten how to cast!\n", ch);
    StopCasting(ch);
    return;
  }

  if( IS_AGG_SPELL(spl) )
  {
    if( check_mob_retaliate(ch, tar_char, spl) )
    {
      return;
    }
  }
  is_tank = FALSE;

  LOOP_THRU_PEOPLE(kala, ch)
  {
    if( kala->specials.fighting == ch )
    {
      is_tank = TRUE;
    }
  }

  if (IS_TRUSTED(ch))
  {
    dura = 1;
  }
  else
  {
    /*if (GET_CLASS(ch, CLASS_PSIONICIST) &&
        (!is_tank || (is_tank && number(0, 1))) &&
        IS_SET(ch->specials.act2, PLR2_QUICKCHANT) &&
        ((IS_NPC(ch) && (number(1, 101) > (20 + 1.5 * GET_LEVEL(ch)))) ||
         (GET_CHAR_SKILL(ch, SKILL_QUICK_CHANT) > number(1, 100))))
    {
      dura >>= 1;
      if (!number(0, 1))
        notch_skill(ch, SKILL_QUICK_CHANT, get_property("skill.notch.quickChant", 2.5));
    }*/
    /*
    if (GET_CLASS(ch, CLASS_MINDFLAYER))
    {
      dura = 1;
    }
    */
  }

  tmp_spl.timeleft = dura;
  tmp_spl.spell = common_target_data.ttype;
  tmp_spl.object = common_target_data.t_obj;
  if (common_target_data.arg)
  {
    tmp_spl.arg = str_dup(common_target_data.arg);
  }

  if( get_spell_circle(ch, tmp_spl.spell) == get_max_circle(ch)
    && number(0,100) > GET_C_AGI(ch)/2 + 50 )
  {
    add_event(event_abort_spell, number(0,10)*dura/10, ch, 0, 0, 0, 0, 0);
  }

  dura = BOUNDED(1, dura, 4);
  tmp_spl.timeleft -= dura;
  DelayCommune(ch, dura);
  SET_BIT(ch->specials.affected_by2, AFF2_CASTING);
  add_event(event_spellcast, BOUNDED(1, dura, 4), ch, common_target_data.t_char,
    0, 0, &tmp_spl, sizeof(struct spellcast_datatype));
  if (common_target_data.t_char)
  {
    if (IS_SET(skills[common_target_data.ttype].targets, TAR_CHAR_WORLD))
    {
      link_char(ch, common_target_data.t_char, LNK_CAST_WORLD);
    }
    else
    {
      link_char(ch, common_target_data.t_char, LNK_CAST_ROOM);
    }
  }
}

bool check_disruptive_blow(P_char ch)
{
  struct damage_messages messages = {
    "You lunge, slamming your fist into $N's larynx.",
    "$n lunges toward you, slamming $s fist into your throat.",
    "$n lunges, slamming $s fist into $N's larynx.",
    "You lunge, slamming your fist into $N's larynx. $E's dead.",
    "$n lunges, slamming $s fist into your throat. That did it.",
    "$n lunges, slamming $s fist into $N's larynx. $E's dead."
  };
  struct damage_messages messages2 = {
    "You hit $N in the face, sending $M reeling.",
    "$n hits you in the face, sending you reeling.",
    "$n hits $N in the face, sending $M reeling.",
    "You lunge, slamming your fist into $N's larynx. $E's dead.",
    "$n lunges, slamming $s fist into your throat. That did it.",
    "$n lunges, slamming $s fist into $N's larynx. $E's dead."
  };
  int skl;
  int success;
  P_char tch;

  if( !IS_ALIVE(ch) || !ch->specials.fighting || IS_IMMOBILE(ch)
    || !AWAKE(ch) || IS_STUNNED(ch) || !IS_HUMANOID(ch))
    return FALSE;

  // tch is the merc in this situation.
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch)
      continue;

    if(IS_IMMATERIAL(tch) || !IS_HUMANOID(tch))
      continue;

    if (GET_POS(tch) != POS_STANDING)
      continue;

    // Merc must be targeting caster.
    if(tch->specials.fighting != ch)
      continue;

    skl = GET_CHAR_SKILL(tch, SKILL_DISRUPTIVE_BLOW);
    success = skl - number(0, 130);

    if (skl && success)
    {
      if( notch_skill(ch, SKILL_DISRUPTIVE_BLOW, 17) )
        success = 100;

      if (success > 75)
      {
        if (melee_damage(tch, ch, 4 * (dice(5, 10)), 0, &messages) == DAM_NONEDEAD)
        {
          StopCasting(ch);
          return TRUE;
        }
      }
      else if (success > 35)
      {
        melee_damage(tch, ch, 4 * (dice(5, 10)), 0, &messages2);
      }
      else
      {
        act("&+wYour lunge at $N&+w's throat comes up a bit short.", FALSE, tch, NULL, ch, TO_CHAR);
      }
    }
  }

  return FALSE;
}

void do_cast(P_char ch, char *argument, int cmd)
{
  int      dura, fail;
  P_char   kala, kala2;
  struct spellcast_datatype tmp_spl;
  char    *orig_arg;
  bool     is_tank = FALSE, weaved = false;
  int      virt, spl;
  int      skl, chant_bonus = 0;
  char buffer[256];
  P_char tar_char;
  struct affected_type *weave_af;

  memset(&tmp_spl, 0, sizeof(tmp_spl));
  if( cmd == CMD_SPELLWEAVE && affected_by_spell(ch, SKILL_SPELLWEAVE) )
  {
    send_to_char("You already have a spell prepared.\n", ch);
    return;
  }

  if( affected_by_spell_flagged(ch, SKILL_THROAT_CRUSH, AFFTYPE_CUSTOM1)
    && !GET_CHAR_SKILL(ch, SKILL_SILENT_SPELL) )
  {
    send_to_char("Your throat hurts too much to cast. \n", ch);
    return;
  }

  if( GET_CLASS(ch, CLASS_DRUID) && IS_DISGUISE_SHAPE(ch) )
  {
    send_to_char("You grunt and chirp a little, then realize you're an animal.\n", ch);
    return;
  }

  if( IS_AFFECTED(ch, AFF_BOUND) )
  {
    send_to_char("Your binds are too tight for that!\n", ch);
    return;
  }
  if( affected_by_spell(ch, SKILL_BERSERK) && !IS_TRUSTED(ch) && GET_RACE(ch) != RACE_MINOTAUR )
  {
    send_to_char("You are too filled with &+RRAGE&N to cast!\n", ch);
    return;
  }
  if( USES_MANA(ch) && !IS_TRUSTED(ch) )
  {
    send_to_char("Psionicists use the command &+Bwill&n to use their abilities.\n", ch);
    return;
  }

/*  if (GET_CLASS(ch, CLASS_RANGER) && !IS_GOOD(ch) && !IS_MULTICLASS_PC(ch))
  {
    send_to_char("Alas, your spellcasting abilities are no good unless YOU are good.\n", ch);
    return;
  }*/

 /* if (get_linking_char(ch, LNK_RIDING) != NULL)
  {
    send_to_char("Sorry, you can't cast with someone on your back!\n", ch);
    return;
  }*/

  if( P_char mount = get_linked_char(ch, LNK_RIDING) )
  {
    if( !GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT) && !is_natural_mount(ch, mount) )
    {
      send_to_char("You're too busy concentrating on staying on your mount to cast!\n", ch);
      return;
    }
  }

  if( affected_by_spell(ch, TAG_BARDSONG_FAILURE) )
  {
    send_to_char("You haven't regained your composure.\n", ch);
    return;
  }

  if( IS_HEADLOCK(ch) )
  {
    send_to_char("You're having a hard enough time breathing, let alone trying to cast anything right now!\n", ch);
    return;
  }

  argument = skip_spaces(argument);
  orig_arg = argument;

  if( IS_AFFECTED2(ch, AFF2_SILENCED) && !USES_MANA(ch)
    && !GET_CHAR_SKILL(ch, SKILL_SILENT_SPELL) && !IS_TRUSTED(ch) )
  {
    send_to_char("You seem unable to say your own name, much less chant a spell!\n", ch);
    return;
  }

  if( !parse_spell(ch, argument, &common_target_data, cmd) )
  {
    return;
  }

  if( (weave_af = get_spell_from_char(ch, SKILL_SPELLWEAVE)) &&
      weave_af->modifier == common_target_data.ttype )
  {
    send_to_char("You call forth your prepared spell...\n", ch);
    weaved = TRUE;
  }
  else
  {
    if(is_silent(ch, FALSE))
    {
      if( silent_spell_check(ch) )
      {
        send_to_char("With the absence of sound, you begin using your hands to channel the weave...\n", ch);
      }
      else
      {
        CharWait(ch, PULSE_VIOLENCE);
        return;
      }
    }
    else
    {
      send_to_char("You start chanting...\n", ch);
    }
  }

  // parse_spell parses argument and sets values on a global struct
  spl = common_target_data.ttype;
  tar_char = common_target_data.t_char;
  tmp_spl.spell = common_target_data.ttype;
  tmp_spl.object = common_target_data.t_obj;

  if( IS_SET(world[ch->in_room].room_flags, NO_MAGIC) && !IS_TRUSTED(ch) )
  {
    send_to_char("The magic gathers, then fades away.\n", ch);
    StopCasting(ch);
    return;
  }

  if( get_spell_from_room(&world[ch->in_room], SPELL_STARSHELL)
    && !IS_TRUSTED(ch) && number(0, 1) )
  {
    send_to_char("You are blinded by the light and lose your concentration.\n", ch);
    StopCasting(ch);
    return;
  }

  if( IS_AFFECTED5(ch, AFF5_MEMORY_BLOCK) && number(0, 1) )
  {
    send_to_char("You seem to have forgotten how to cast!\n", ch);
    StopCasting(ch);
    return;
  }

  if( weaved )
  {
    if( notch_skill(ch, SKILL_SPELLWEAVE, get_property("skill.notch.spellWeave", 50))
      || GET_CHAR_SKILL(ch, SKILL_SPELLWEAVE) > number(0, 100) )
    {
      SET_BIT(ch->specials.affected_by2, AFF2_CASTING);
      event_spellcast(ch, tar_char, 0, &tmp_spl);
    }
    else
    {
      send_to_char("The spell fails, dispersing magical energy into the surroundings.\n", ch);
    }
    affect_remove(ch, weave_af);
    return;
  }

  dura = (SpellCastTime(ch, spl));
  if( GET_CHAR_SKILL(ch, SKILL_CHANT_MASTERY) )
  {
    // This function calls CharWait appropriately
    dura = chant_mastery_bonus(ch, dura);
  }
  else if( GET_CHAR_SKILL(ch, SKILL_TOTEMIC_MASTERY) > number(50, 200) && hasTotem(ch, spl) )
  {
    act("&+yYou call upon the powers of the &+wspirit &+Lrealm&+y, channeling your power...&n", FALSE, ch, 0, 0, TO_CHAR);
    act("&+y$n &+ygrasps his totem tightly, and begins communing with the &+wspirits&+y.&n", TRUE, ch, 0, 0, TO_ROOM);
    dura = (int) (dura * .75);
    CharWait(ch, dura);
  }
  else if( OUTSIDE(ch) && GET_CHAR_SKILL(ch, SKILL_NATURES_SANCTITY) > number(1, 100) )
  {
    send_to_char("&+GThe power of nature flows into you, hastening your incantation.\n", ch);
    dura = (int) (dura * .75);
    CharWait(ch, dura);
  }
  else if( OUTSIDE(ch) && GET_CHAR_SKILL(ch, SKILL_NATURES_RUIN) > number(1, 100) )
  {
    send_to_char("&+yYou drain power from nature, hastening your incantation.\n", ch);
    dura = (int) (dura * .75);
    CharWait(ch, dura);
  }
  else if( GET_CLASS(ch, CLASS_DRUID) && !IS_MULTICLASS_PC(ch) )
  {
    CharWait(ch, (dura >> 1) + 6);
  }
  else if( affected_by_spell(ch, SKILL_BERSERK) )
  {
    dura = (int) (dura * get_property("spell.berserk.casting.starMod", 1.500));
    CharWait(ch, dura);
  }
  else
  {
    CharWait(ch, dura);
  }

  SpellCastShow(ch, spl);

  if(IS_AGG_SPELL(spl))
  {
    appear(ch);

    if( check_mob_retaliate(ch, tar_char, spl) )
    {
      return;
    }
  }

  is_tank = FALSE;

  LOOP_THRU_PEOPLE(kala, ch)
  {
    if (kala->specials.fighting == ch)
      is_tank = TRUE;
  }

  // Why were Psi's set to instacast?? This must've been really old code.
  // It only showed up when caster/psi multi was played, making a psi
  //  secondary trigger on the code in do_cast instead of do_will.
  if( IS_TRUSTED(ch) || IS_SET(skills[spl].targets, TAR_INSTACAST) )
  {
    dura = 1;
  }
  else if( (GET_CLASS(ch, CLASS_DRUID) && !IS_MULTICLASS_PC(ch))
    || (GET_CLASS(ch, CLASS_BLIGHTER) && !IS_MULTICLASS_PC(ch))
    || ((!is_tank || number(0, 1)) && (IS_NPC(ch) || IS_SET(ch->specials.act2, PLR2_QUICKCHANT))
    && (notch_skill(ch, SKILL_QUICK_CHANT, get_property("skill.notch.quickChant", 2.5))
    || (GET_CHAR_SKILL(ch, SKILL_QUICK_CHANT) > number(1, 100)))) )
  {
    dura >>= 1;
  }

  tmp_spl.timeleft = dura;
// if( IS_PC(ch) ) debug( "Final cast time: %d.", tmp_spl.timeleft );
  if( get_spell_circle(ch, tmp_spl.spell) == get_max_circle(ch)
    && number(0,100) > GET_C_AGI(ch)/2 + 50 )
  {
    add_event(event_abort_spell, number(0,9)*dura/10, ch, 0, 0, 0, 0, 0);
  }

  dura = BOUNDED(1, dura, 4);
  tmp_spl.timeleft -= dura;
  DelayCommune(ch, dura);
  if( cmd == CMD_SPELLWEAVE )
  {
    if (GET_CHAR_SKILL(ch, SKILL_SPELLWEAVE))
    {
      tmp_spl.flags = CST_SPELLWEAVE;
    }
    else
    {
      send_to_char("You haven't mastered this sophisticated art.\n", ch);
      return;
    }
  }

  if( common_target_data.arg )
  {
    tmp_spl.arg = str_dup(common_target_data.arg);
  }

  SET_BIT(ch->specials.affected_by2, AFF2_CASTING);
  add_event(event_spellcast, BOUNDED(1, dura, 4), ch, common_target_data.t_char,
    0, 0, &tmp_spl, sizeof(struct spellcast_datatype));

  if( common_target_data.t_char )
  {
    if (IS_SET(skills[common_target_data.ttype].targets, TAR_CHAR_WORLD))
    {
      link_char(ch, common_target_data.t_char, LNK_CAST_WORLD);
    }
    else
    {
      link_char(ch, common_target_data.t_char, LNK_CAST_ROOM);
    }
  }
  if( IS_FIGHTING(ch) && check_disruptive_blow(ch) )
  {
    return;
  }
}

bool is_obj_in_list_vis(P_char ch, P_obj obj, P_obj list)
{
  for (P_obj t_obj = list; t_obj ; t_obj = t_obj->next_content)
  {
    if (t_obj == obj && CAN_SEE_OBJ(ch, t_obj))
    {
      return TRUE;
    }
  }

  return FALSE;
}


void casting_broken(struct char_link_data *cld)
{
  show_abort_casting(cld->linking);
}

void event_abort_spell(P_char ch, P_char victim, P_obj obj, void *data)
{
  send_to_char("You lost your concentration!\n", ch);
  StopCasting(ch);
}

void event_spellcast(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct spellcast_datatype *arg = (struct spellcast_datatype *)data;
  int      skl = 0, i = 0, circle = 0, num = 0, manacost, room = ch->in_room, chance;
  char     buf[1024];
  char args[128];
  P_obj    room_junk = NULL, room_junk_temp = NULL;
  P_char   tmpch, tmpch2, gvict;
  struct affected_type af;
  P_char  tar_char;
  P_obj tar_obj;
  bool weaving = (arg->flags & CST_SPELLWEAVE) != 0;
  struct affected_type *weave_af;

  if (!ch || (ch->in_room == NOWHERE))
    return;

  if (!IS_SET(ch->specials.affected_by2, AFF2_CASTING))
    return;

  if (!arg || !cast_common_generic(ch, arg->spell))
  {
    StopCasting(ch);
    return;
  }

  if (GET_CLASS(ch, CLASS_PSIONICIST))
  {
    if (IS_HEADLOCK(ch) || IS_AFFECTED(ch, AFF_KNOCKED_OUT) || affected_by_spell(ch, SONG_SLEEP) || affected_by_spell(ch, SPELL_SLEEP))
    {
      StopCasting(ch);
      return;
    }
  }
  else if(IS_HEADLOCK(ch) || IS_IMMOBILE(ch))
  {
    StopCasting(ch);
    return;
  }

  tar_obj = arg->object;
  tar_char = victim;

  // We don't have char-obj links, so need to check whether item is still there..
  if( tar_obj )
  {
    bool ok = FALSE;

    if (IS_SET(skills[arg->spell].targets, TAR_OBJ_INV))
    {
      ok = is_obj_in_list_vis(ch, tar_obj, ch->carrying);
    }
    if (!ok && IS_SET(skills[arg->spell].targets, TAR_OBJ_EQUIP))
    {
      for (int i = 0; i < MAX_WEAR; i++)
      {
        if (ch->equipment[i] == tar_obj)
        {
          ok = TRUE;
          break;
        }
      }
    }
    if( !ok && IS_SET(skills[arg->spell].targets, TAR_OBJ_ROOM) )
    {
      ok = is_obj_in_list_vis(ch, tar_obj, world[ch->in_room].contents);
    }
    if( !ok )
    {
      StopCasting(ch);
      return;
    }
  }

  // Room links aren't perfect, so checking if victim really is in the same room
  if( tar_char )
  {
    if( IS_SET(skills[arg->spell].targets, TAR_CHAR_RANGE) )
      ;
    else if( IS_SET(skills[arg->spell].targets, TAR_CHAR_ROOM )
      && !is_char_in_room(tar_char, ch->in_room) )
    {
      StopCasting(ch);
      return;
    }
  }

  if (arg->timeleft > 0)
  {
    if( IS_AGG_SPELL(arg->spell) )
    {
      appear(ch);
    }
    // No point in sending messages to mobs  -Odorf
    // Should probably change this to ch->desc since God switch.
    if( IS_PC(ch) )
    {
      if (GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_SHAMAN))
      {
        skl = SKILL_SPELL_KNOWLEDGE_SHAMAN;
      }
      else if (GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_CLERICAL))
      {
        skl = SKILL_SPELL_KNOWLEDGE_CLERICAL;
      }
      else
      {
        skl = SKILL_SPELL_KNOWLEDGE_MAGICAL;
      }
      //if (GET_CLASS(ch, CLASS_PSIONICIST | CLASS_DRUID | CLASS_ETHERMANCER) ||
      if( GET_CLASS(ch, CLASS_PSIONICIST | CLASS_MINDFLAYER | CLASS_DRUID | CLASS_BLIGHTER) ||
	      number(1, 100) <= GET_CHAR_SKILL(ch, skl) )
      {
        sprintf(buf, "Casting: %s ", skills[arg->spell].name);
        for (i = 0; i < (arg->timeleft / 4); i++)
        {
          strcat(buf, "*");
        }
        strcat(buf, "\n");
        send_to_char(buf, ch);
      }
      else
      {
        notch_skill(ch, skl, 2);
      }
    }
    i = MIN(arg->timeleft, 4);
    arg->timeleft -= i;
    DelayCommune(ch, i);
    add_event(event_spellcast, BOUNDED(1, i, 4), ch, tar_char, 0, 0, arg, sizeof(struct spellcast_datatype));
    return;
  }

  if (arg->arg)
  {
    strcpy(args, arg->arg);
    FREE(arg->arg);
    arg->arg = NULL;
  }
  else
  {
    args[0] = '\0';
  }

  /*
     ok, we are in the home stretch, this event call has arg->timeleft of <= 0
     so now we *FINALLY*, actually cast the spell.  JAB
   */

  if( !((weave_af = get_spell_from_char(ch, SKILL_SPELLWEAVE))
    && weave_af->modifier == common_target_data.ttype) )
  {
    use_spell(ch, arg->spell);
  }

  if( weaving )
  {
    struct affected_type af;

    REMOVE_BIT(ch->specials.affected_by2, AFF2_CASTING);
    memset(&af, 0, sizeof(af));
    af.type = SKILL_SPELLWEAVE;
    af.modifier = arg->spell;
    af.duration = GET_LEVEL(ch)/10; // 3 mins at 30, 5 mins max at lvl 50+
    affect_to_char(ch, &af);
    send_to_char("You finish weaving a spell.\n", ch);
    return;
  }

  /*
   * Make sure target situation appropriate for cast completion.  The nature of
   * the spell.targets bitvector prompts my quite unconventional usage of the
   * IS_SET() macro.  However, the bitwise "and" operation yields the intended
   * result using the bitwise "or" constructed mask.  Traditionally the macro
   * uses a single bit as a mask, but I see no reason not to employ more complex
   * masks when appropriate, as in the case below IMHO.  N.B., however, that the
   * bitwise masking operations proceed into a boolean not. - SKB 12 Apr 1995
   */

  if( !IS_SET(skills[arg->spell].targets, TAR_IGNORE)
    && !IS_SET(skills[arg->spell].targets, TAR_AREA) && !IS_SET(skills[arg->spell].targets, TAR_OFFAREA)
    && !IS_SET(skills[arg->spell].targets, (TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)) )
  {
    if( (!tar_char && !IS_SET(skills[arg->spell].targets, TAR_CHAR_WORLD))
      || (tar_char && !IS_SET(skills[arg->spell].targets, TAR_CHAR_WORLD)
      && !IS_SET(skills[arg->spell].targets, TAR_CHAR_RANGE)
      && (tar_char->in_room != ch->in_room)) )
    {
      send_to_char("You don't have a valid target.\n", ch);
      StopCasting(ch);
      return;
    }
  }
  if( tar_obj && !IS_SET(skills[arg->spell].targets, (TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)) )
  {
    StopCasting(ch);
    return;
  }
  /*
   * If the flow reaches this point, then we have had a successful cast.  In
   * such a case, the (N)PC utters the spell, and invokes its action.  In the
   * NPC case, the appropriate element decremented in
   * mob->specials.undead_spell_slots[], the array tracking the # of spells the
   * mob may still cast in that circle.  Player spell tracking already existed
   * and handled by the time the mob "mem" case added. - SKB 24 Mar 1995
   */
  // We don't want IS_TRUSTED(ch) because that can be turned off with toggle fog.
  if( GET_LEVEL(ch) > MAXLVLMORTAL && IS_PC(ch) )
  {
    sprintf(buf, "%s cast '%s' at %s in room %d", GET_NAME(ch), skills[arg->spell].name,
      tar_char ? GET_NAME(tar_char) : tar_obj ? tar_obj->short_description : "(no target ch|obj)",
      world[ch->in_room].number);

    logit(LOG_WIZ, buf);
    wizlog(GET_LEVEL(ch), buf);
    sql_log(ch, WIZLOG, buf);
  }

#ifdef MISFIRE
  if( IS_PC(ch) && tar_char && IS_SET(skills[arg->spell].targets, TAR_CHAR_ROOM) )
  {
    tar_char = misfire_check(ch, tar_char, 0);
  }
#endif

  if( affected_by_spell(ch, SKILL_BERSERK) )
  {
    if( number(0, 100) < (int)get_property("spell.berserk.casting.mistarget.perc", 80.000) )
    {
      tar_char = get_random_char_in_room(ch->in_room, ch, DISALLOW_SELF);
      // If you're the only one in room when this excites, we crash because we
      // pass NULL to tar_char.
      if( !tar_char )
      {
        tar_char = ch;
      }
      act("&+wIn an explosion of rage, you accidently target $N!&n", FALSE, ch, 0, tar_char, TO_CHAR);
      act("&+WIn an explosion of rage, $n accidently targets YOU!&n", FALSE, ch, 0, tar_char, TO_VICT);
      act("&+WIn an explosion of rage, $n's accidently targets $n!&n", FALSE, ch, 0, tar_char, TO_NOTVICT);
    }
  }

  if (tar_char && IS_GRAPPLED(tar_char))
  {
    gvict = grapple_attack_check(tar_char);
    if (gvict && (gvict != ch))
    {
      chance = grapple_misfire_chance(ch, gvict, 1);
      if (number(1, 100) <= chance)
      {
        tar_char = gvict;
      }
    }
  }

  if( tar_char && (tar_char != ch) && IS_AGG_SPELL(arg->spell) )
  {
    appear(ch);
    tar_char = guard_check(ch, tar_char);
  }

  /*
     if (tar_char && ch->desc && tar_char->desc && !IS_TRUSTED(ch) &&
     !strcmp(ch->desc->host, tar_char->desc->host) && (ch != tar_char))
     wizlog(GET_LEVEL(ch), "&+W%s &+Rcasted &+G'%s'&+R on &+W%s&+R "
     "using same ip&+W(%s)&+R Type &+Wok <name>&+R to accept this "
     "mutliplaying.", GET_NAME(ch), skills[spl].name, 
     GET_NAME(tar_char), ch->desc->host);
   */

  if( !USES_MANA(ch) )
  {
    send_to_char("&+WYou complete your spell...&n\n", ch);
  }
  else
  {
    send_to_char("&+MYour mental manipulations become a reality...&n\n", ch);
  }

  REMOVE_BIT(ch->specials.affected_by2, AFF2_CASTING);
  clear_links(ch, LNK_CAST_ROOM);
  clear_links(ch, LNK_CAST_WORLD);

  if( !USES_MANA(ch) )
  {
    act("$n &+Wcompletes $s spell...&n", FALSE, ch, 0, 0, TO_ROOM);

    if( is_silent(ch, FALSE) )
    {
      say_silent_spell(ch, arg->spell);
    }
    else
    {
      say_spell(ch, arg->spell);
    }
  }
  if( IS_AGG_SPELL(arg->spell) && tar_char && ch != tar_char && IS_AFFECTED(ch, AFF_INVISIBLE) )
  {
    appear(ch);
  }

  if( tar_char )
  {
    if( (has_innate(ch, INNATE_WILDMAGIC) || IS_AFFECTED4(ch, AFF4_WILDMAGIC)
      || (IS_NPC(ch) && IS_SET(ch->specials.act, ACT_WILDMAGIC)))
      && !number(0, 5) && IS_AGG_SPELL(arg->spell)
      && !(IS_SET(skills[arg->spell].targets, TAR_IGNORE)
      || IS_SET(skills[arg->spell].targets, TAR_AREA)) )
    {
      perform_chaos_check(ch, tar_char, arg);
      if( !char_in_list(ch) || !char_in_list(tar_char) )
      {
         return;
      }
    }
  }

  // divine blessing
  if( IS_AGG_SPELL(arg->spell) )
  {
    if( divine_blessing_check(ch, tar_char, arg->spell) )
    {
      return;
    }
  }

  circle = 0;

  /* justice hook  */
  /* note the special checking for holyword spells...  */
  if( IS_AGG_SPELL(arg->spell) )
  {
    if( IS_SET(skills[arg->spell].targets, TAR_IGNORE)
      || IS_SET(skills[arg->spell].targets, TAR_AREA) )
    {
      P_char   t, t_next;

      for( t = world[ch->in_room].people; t; t = t_next )
      {
        t_next = t->next_in_room;
        if( should_area_hit(ch, t) )
        {
          if( ((arg->spell == SPELL_HOLY_WORD) && !IS_EVIL(t))
            || ((arg->spell == SPELL_UNHOLY_WORD) && !IS_GOOD(t))
            || ((arg->spell == SPELL_VOICE_OF_CREATION && !IS_EVIL(t))) )
          {
            continue;
          }
          justice_witness(ch, t, CRIME_ATT_MURDER);
        }
      }
    }
    else if( IS_SET(skills[arg->spell].targets, TAR_CHAR_RANGE) )
    {
      if( tar_char && (tar_char != ch) )
      {
        if( IS_AFFECTED(ch, AFF_INVISIBLE) )
        {
          appear(ch);
        }
        if( ch->in_room != tar_char->in_room && (IS_PC(ch) || IS_PC_PET(ch)) && IS_NPC(tar_char) )
        {
          for (tmpch = world[tar_char->in_room].people; tmpch; tmpch = tmpch2)
          {
            tmpch2 = tmpch->next_in_room;

            if (IS_NPC(tmpch))
              MobRetaliateRange(tmpch, ch);

            if (!char_in_list(ch))
              return;
          }
        }
      }
    }
    else
    {
      justice_witness(ch, tar_char, CRIME_ATT_MURDER);
    }
  }

  if( IS_AGG_SPELL(arg->spell) && (arg->spell != SPELL_SLEEP) )
  {
    if( tar_char && (ch != tar_char) && (GET_STAT(tar_char) != STAT_DEAD) )
    {
      if( affected_by_spell(tar_char, SPELL_SLEEP) )
      {
        affect_from_char(tar_char, SPELL_SLEEP);
      }
      if (GET_STAT(tar_char) == STAT_SLEEPING)
      {
        send_to_char("Your rest is violently disturbed!\n", tar_char);
        act("Your spell disturbs $N's beauty sleep!", FALSE, ch, 0, tar_char,
            TO_CHAR);
        act("$n's spell disturbs $N's beauty sleep!", FALSE, ch, 0, tar_char,
            TO_NOTVICT);
        SET_POS(tar_char, GET_POS(tar_char) + STAT_NORMAL);
      }
    }
  }
  ((*skills[arg->spell].spell_pointer) ((int) GET_LEVEL(ch), ch, args, SPELL_TYPE_SPELL, tar_char, tar_obj));

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_AGG_SPELL(arg->spell) && is_char_in_room(tar_char, room) && is_char_in_room(ch, room) )
  {
    // Devotion double cast check..
    int dev_power;
    if( devotion_spell_check(arg->spell) && (dev_power = devotion_skill_check(ch)) > 0 )
    {
      ((*skills[arg->spell].spell_pointer) (dev_power, ch, args, SPELL_TYPE_SPELL, tar_char, tar_obj));
      if( !IS_ALIVE(ch) )
      {
        return;
      }
    }

    // FALSE -> proc from non-physical.
    if( IS_ALIVE(tar_char) && affected_by_spell(ch, ACH_YOUSTRAHDME) && IS_UNDEADRACE(tar_char)
      && lightbringer_proc(ch, tar_char, FALSE) )
    {
      // If the proc killed someone, check if it was ch.
      if( !IS_ALIVE(ch) )
      {
        return;
      }
    }
  }

  /*
  if ((GET_STAT(ch) != STAT_DEAD) && USES_FOCUS(ch))
  {
    if (IS_PC(ch) && (GET_MANA(ch) < 0))
    {
      act("Exhausted, $n passes out!", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("Reserves exhausted, you pass out!\n", ch);
      KnockOut(ch, -(int) ((float) (GET_MANA(ch)) * 1.5));
    }
  }
  */
  CharWait(ch, PULSE_SPELLCAST / 2);
}

void assign_spell_pointers(void)
{
  int      i, j;

  memset(SortedSkills, 0, sizeof(int) * (MAX_AFFECT_TYPES+1));

  initialize_skills();

  for (i = 0; i < MAX_AFFECT_TYPES; i++)
  {
    spells[i] = skills[i].name;
  }
  
  spells[MAX_AFFECT_TYPES] = "\n";

  /*
     create a lookup table for mob casters.  Since mob caster far outnumber PC
     casters, any time savings is worthwhile.  JAB
   */

  for (j = 0; j <= LAST_SPELL; j++)
  {
    MobSpellIndex[j] = 11;
    for (i = 0; i < CLASS_COUNT; i++)
      if (skills[j].m_class[i].rlevel[0] > 0)
        if (skills[j].m_class[i].rlevel[0] < MobSpellIndex[j])
          MobSpellIndex[j] = skills[j].m_class[i].rlevel[0];
  }

  /* alphabetize spells for ease of reading later */
  qsort(SortedSkills, MAX_AFFECT_TYPES+1, sizeof(int), compare_skills);
}

void update_spellpulse_data()
{
  char     buf[128];
  int      i;

  for (i = 0; i <= LAST_RACE; i++)
  {
    sprintf(buf, "spellcast.pulse.racial.%s", race_names_table[i].no_spaces);
    spell_pulse_data[i] = get_property(buf, 1.0);
  }
}

void update_racial_shrug_data()
{
  char     buf[128];
  int      i;

  for (i = 0; i <= LAST_RACE; i++)
  {
    sprintf(buf, "innate.shrug.%s", race_names_table[i].no_spaces);
    racial_shrug_data[i] = get_property(buf, 0);
  }
}

void update_racial_exp_mods()
{
  char     buf[128];
  int      i;

  for (i = 0; i <= LAST_RACE; i++)
  {
    sprintf(buf, "exp.factor.%s", race_names_table[i].no_spaces);
    racial_exp_mods[i] = get_property(buf, 1.0);
  }
}

void update_racial_exp_mod_victims()
{
  char     buf[128];
  int      i;

  for (i = 0; i <= LAST_RACE; i++)
  {
    sprintf(buf, "gain.exp.mod.victim.race.%s", race_names_table[i].no_spaces);
    racial_exp_mod_victims[i] = get_property(buf, 1.0);
  }
}
