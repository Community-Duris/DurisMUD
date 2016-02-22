#include <signal.h>
#include <string.h>
#include "defines.h"
#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "events.h"
#include "db.h"
#include "utils.h"
#include "prototypes.h"
#include "justice.h"
#include "necromancy.h"
#include "damage.h"
#include "graph.h"

extern P_room world;
extern P_index mob_index;
extern P_index obj_index;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern const struct race_names race_names_table[];

bool isCarved(P_obj corpse);

int CheckFor_remember(P_char ch, P_char victim);
P_obj get_globe( P_char ch );

struct undead_description
{
  char    *name;
  char    *short_desc;
  int      corpse_level;
  int      act;
  int      aff1;
  int      aff2;
  float    hps;
  int      max_level;
  int      cost;
  uint     pet_class;
  int      race;
};

extern const struct golem_description golem_data[];
const struct golem_description golem_data[4] = {
  {
   "flesh golem", 34, 41, 15.0, 50, 6},
  {
   "blood golem", 35, 46, 20.0, 50, 6},
  {
   "bone golem", 33, 51, 25.0, 51, 6},
  {
   "valorhammer valor hammer golem", 77, 41, 25, 51, 6}
};

extern const struct undead_description undead_data[NECROPET_LAST + 1];
const struct undead_description undead_data[NECROPET_LAST + 1] = {
  {
   "skeleton", "", 2, ACT_NICE_THIEF, AFF_HASTE, 0, 1.2, 21, 10, CLASS_WARRIOR,
   RACE_SKELETON},
  {
   "zombie", "", 4, ACT_NICE_THIEF + ACT_MEMORY, AFF_PROTECT_GOOD, AFF2_SLOW, 1.8, 26, 10, CLASS_WARRIOR,
   RACE_ZOMBIE},
  {
   "spectre", "", 20, ACT_MEMORY,
   AFF_FLY + AFF_INVISIBLE + AFF_DETECT_INVISIBLE + AFF_PROTECT_GOOD +
   AFF_SNEAK,
   AFF2_PROT_COLD, 2, 31, 10, CLASS_ASSASSIN, RACE_SPECTRE},
  {
   "wraith", "", 30, ACT_MEMORY, AFF_FLY + AFF_DETECT_INVISIBLE +
   AFF_INVISIBLE + AFF_PROTECT_GOOD,
   AFF2_PROT_COLD, 3.2, 36, 22, CLASS_SORCERER, RACE_WRAITH},
  {
   "vampire", "", 40, ACT_MEMORY,
   AFF_FLY + AFF_DETECT_INVISIBLE + AFF_INVISIBLE + AFF_PROTECT_GOOD
   + AFF_HASTE, AFF2_PROT_COLD + AFF2_VAMPIRIC_TOUCH,
   12, 50, 28, CLASS_WARRIOR,
   RACE_VAMPIRE},
  {
   "lich", "", 45, ACT_MEMORY,
   AFF_DETECT_INVISIBLE + AFF_SENSE_LIFE + AFF_FLY + AFF_INVISIBLE +
   AFF_PROTECT_GOOD + AFF_HASTE,
   AFF2_PROT_COLD + AFF2_VAMPIRIC_TOUCH, 5, 46, 28, CLASS_SORCERER,
   RACE_PLICH},
  {
   "shadow", "", 40, ACT_MEMORY,
   AFF_HASTE + AFF_DETECT_INVISIBLE +
   AFF_SENSE_LIFE + AFF_FLY + AFF_INVISIBLE + AFF_PROTECT_GOOD,
   AFF2_PROT_COLD + AFF4_PHANTASMAL_FORM,
   5, 50, 65, CLASS_PSIONICIST,
   RACE_SHADOW},
  {
   "hound archon brawler", "&+yhou&+Yn&+yd &+Rarch&+Wo&+Rn &+rbr&+Raw&+rler&n", 2, ACT_NICE_THIEF, AFF_HASTE, 0, 1.2, 21, 10, CLASS_WARRIOR,
   RACE_ARCHON},
  {
   "lantern archon soldier", "&+Ylantern &+Rarch&+Wo&+Rn &+yso&+rldi&+yer&n", 4, ACT_NICE_THIEF + ACT_MEMORY, AFF_PROTECT_EVIL, AFF2_SLOW, 1.8, 26, 10, CLASS_WARRIOR,
   RACE_ARCHON},
  {
   "asura avenger", "&+Rasu&+Yr&+Ra &+rave&+Lng&+rer&n", 20, ACT_MEMORY,
   AFF_FLY + AFF_INVISIBLE + AFF_DETECT_INVISIBLE + AFF_PROTECT_EVIL +
   AFF_SNEAK,
   AFF2_PROT_COLD, 2, 31, 10, CLASS_ASSASSIN, RACE_ASURA},
  {
   "bralani battlemage", "&+Cbral&+Wa&+Cni &+Rbattle&+Mmage&n", 30, ACT_MEMORY, AFF_FLY + AFF_DETECT_INVISIBLE +
   AFF_INVISIBLE + AFF_PROTECT_EVIL,
   AFF2_PROT_COLD, 3.2, 36, 22, CLASS_SORCERER, RACE_BRALANI},
  {
   "knight-errant knight errant ghaele", "&+cgha&+Ce&+cle &+wkn&+Wi&+wght&+W-e&+wrr&+Wa&+wnt&n", 40, ACT_MEMORY,
   AFF_FLY + AFF_DETECT_INVISIBLE + AFF_INVISIBLE + AFF_PROTECT_EVIL
   + AFF_HASTE, AFF2_PROT_COLD + AFF2_VAMPIRIC_TOUCH,
   12, 50, 28, CLASS_WARRIOR,
   RACE_GHAELE},
  {
   "liberator holy eladrin", "&+ce&+Cl&+Wadr&+Ci&+cn &+Wholy &+Cliber&+Wator&n", 45, ACT_MEMORY,
   AFF_DETECT_INVISIBLE + AFF_SENSE_LIFE + AFF_FLY + AFF_INVISIBLE +
   AFF_PROTECT_EVIL + AFF_HASTE,
   AFF2_PROT_COLD + AFF2_VAMPIRIC_TOUCH, 5, 46, 28, CLASS_SORCERER,
   RACE_ELADRIN},
  {
   "deva astral", "&+Lastral &+wd&+Wev&+wa&n", 40, ACT_MEMORY,
   AFF_HASTE + AFF_DETECT_INVISIBLE +
   AFF_SENSE_LIFE + AFF_FLY + AFF_INVISIBLE + AFF_PROTECT_EVIL,
   AFF2_PROT_COLD + AFF4_PHANTASMAL_FORM,
   5, 50, 65, CLASS_PSIONICIST,
   RACE_DEVA}
};

#define NECROPLASM_VNUM 67243
#define GLOBE_SHADOWS_VNUM 16262

// Returns TRUE if ch is alive and wearing plasm and tranced.
int is_wearing_necroplasm(P_char ch)
{
  // loop thru ch's equip'd slots, and return 'true' if
  // any of them is the necroplasm object
  int i;
  int plasmID;

  if( !IS_ALIVE(ch) || !affected_by_spell(ch, SPELL_VAMPIRE) )
  {
    return FALSE;
  }

  plasmID = real_object0(NECROPLASM_VNUM);
  if( plasmID )
  {
    for( int i = 0; i < MAX_WEAR; i++ )
    {
      if( ch->equipment[i] && ch->equipment[i]->R_num == plasmID )
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

void charm_broken(struct char_link_data *cld)
{
  int necropets[] = {3, 4, 5, 6, 7, 8, 9, 10, 78, 79, 80, 81, 82, 83, 84, 85};
  
  /* called when a CHARM affect is broken */
  /* verify that the pet is still following anyone, as some mobs self-poof */
  if (cld->linking && cld->linking->following)
  {
    stop_follower(cld->linking);
    if (IS_NPC(cld->linking) && (cld->linking->in_room == cld->linked->in_room) &&
        CheckFor_remember(cld->linking, cld->linked))
    {
      MobStartFight(cld->linking, cld->linked);
    }
  }

  for(int i = 0; i < 16; i++)
  {
    if (IS_NPC(cld->linking) &&
	((mob_index[GET_RNUM(cld->linking)].virtual_number == necropets[i]) ||
         (mob_index[GET_RNUM(cld->linking)].virtual_number == NECROPET)))
    {
      add_event(event_pet_death, 1 * 60 * 4, cld->linking, NULL, NULL, 0, NULL, 0);
      break;
    }
  }
}

void event_pet_death(P_char ch, P_char victim, P_obj obj, void *data)
{
  die(ch, ch);
}

int setup_pet(P_char mob, P_char ch, int duration, int flag)
{
  struct affected_type af;
  P_obj globe;
  string name;

  memset(&af, 0, sizeof(af));
  if( !IS_SET(flag, PET_NOORDER) )
  {
    af.bitvector = AFF_CHARM;
  }
  af.flags = AFFTYPE_NODISPEL;
  af.type = SPELL_CHARM_PERSON;
  af.duration = IS_PC(ch) ? duration : -1;

  globe = get_globe( ch );

  if( has_innate(ch, INNATE_UNHOLY_ALLIANCE) || globe )
  {
    af.duration = -1;
  }

  duration = af.duration;
  /* the higher the level of the mob, the more likely it'll be aggro to the caster after charm*/
  if( IS_NPC(mob) && !IS_SET(flag, PET_NOAGGRO) && (GET_LEVEL(mob) > number(30, 62))
    && !has_innate(ch, INNATE_UNHOLY_ALLIANCE) )
  {
    SET_BIT(mob->specials.act, ACT_MEMORY);
    clearMemory(mob);
    remember(mob, ch);
  }

  linked_affect_to_char(mob, &af, ch, LNK_PET);

  if( flag & PET_NOCASH )
  {
    GET_PLATINUM(mob) = 0;
    GET_GOLD(mob) = 0;
    GET_SILVER(mob) = 0;
    GET_COPPER(mob) = 0;
  }

  if( IS_NPC(mob) )
  {
    GET_EXP(mob) = 0;
  }
  remove_plushit_bits(mob);

  if( IS_PC(ch) )
  {
    name = string(mob->player.name) + " _" + string(ch->player.name) + "_";
    mob->player.name = str_dup(name.c_str());
  }

  return duration;
}

int count_pets(P_char ch)
{
  struct char_link_data *cld;
  int      count;

  for (count = 0, cld = ch->linked; cld; cld = cld->next_linked)
    if (cld->type == LNK_PET)
      count++;

  return count;
}

int count_undead(P_char ch)
{
  struct char_link_data *cld;
  P_char follower;
  float sum;
  int i;

  sum = 0;

  for (cld = ch->linked; cld; cld = cld->next_linked)
  {
    follower = cld->linking;

    if(cld->type != LNK_PET)
    {
      continue;
    }
    else if(IS_GREATER_DRACO(follower) || IS_GREATER_AVATAR(follower))
    {
      sum += 75;
      continue;
    }
    else if (IS_DRACOLICH(follower) || IS_TITAN(follower))
    {
      sum += 37;
      continue;
    }
    else
    {
      for (i = 0; i <= NECROPET_LAST; i++)
      {
        if(mob_index[GET_RNUM(follower)].virtual_number == NECROPET &&
          strstr(follower->player.name, undead_data[i].name))
        {
          sum += undead_data[i].cost;
          break;
        }
      }
    }

    for(i = 0; i <= NECROGOLEM_LAST; i++)
    {
      if (strstr(follower->player.name, golem_data[i].name))
      {
        sum += golem_data[i].cost;
        break;
      }
    }
  }
  return (int) sum;
}

int can_raise_draco(P_char ch, int level, bool bGreater)
{
  int numb = count_undead(ch);

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( IS_TRUSTED(ch) )
  {
    return TRUE;
  }

  // If they aren't trying to raise a greater,
  // and have a single greater, count it as a single draco.
  //if(!bGreater &&
  //  (numb == 10))
  //{
  //  numb = 1;
  //}

  // max allowed to exist already...
  //int maxAllowed = ((level <= 53) || (bGreater)) ? 1 : 2;

  //return numb <= maxAllowed;

  int necro_power = level * 2;
  int cost = 0;
  if( GET_SPEC(ch, CLASS_NECROMANCER, SPEC_NECROLYTE) ||
     GET_SPEC(ch, CLASS_THEURGIST, SPEC_TEMPLAR) )
     necro_power += 10;

  // 56 * 2 == 112 == 75 + 37 => at 56, can raise 1 greater and 1 regular dracolich.
  if( bGreater )
    cost = 75;
  else
    cost = 37;

  //debug("numb %d + cost %d > np %d", numb, cost, necro_power);
  if( (numb + cost > necro_power) && !IS_TRUSTED(ch) )
  {
    if( bGreater )
    {
      act("You are too weak to animate another greater dracolich!", FALSE, ch, 0, 0, TO_CHAR);
    }
    else
    {
      act("You are too weak to animate another dracolich!", FALSE, ch, 0, 0, TO_CHAR);
    }
    return FALSE;
  }

  return TRUE;
}

void raise_undead(int level, P_char ch, P_char victim, P_obj obj, int which_type)
{
  P_char   undead;
  P_obj    obj_in_corpse, next_obj;
  P_obj    globe;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int      num, i, j, l, clevel, tmp, typ, sum, cap;
  struct affected_type af;
  bool     corpselog = FALSE;
  int      life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);

  if( !(obj && ch) )
  {
    logit(LOG_EXIT, "raise_undead: bogus parms - missing ch or obj");
    raise(SIGSEGV);
  }

  if( GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER) || GET_SPEC(ch, CLASS_THEURGIST, SPEC_THAUMATURGE) )
  {
    life += (int) (life / 3);
  }

  if( CHAR_IN_SAFE_ZONE(ch) )
  {
    send_to_char("You do not feel right raising undead here.\r\n", ch);
    return;
  }

  if( (GET_ALIGNMENT(ch) > 0) && GET_CLASS(ch, CLASS_NECROMANCER) )
  {
    send_to_char("You don't even _consider_ such a evil act, meddling with undead!\n", ch);
    return;
  }

  if( (GET_ALIGNMENT(ch) < 0) && GET_CLASS(ch, CLASS_THEURGIST) )
  {
    send_to_char("You don't even _consider_ such a good act, meddling with angels!\n", ch);
    return;
  }

  if( IS_NPC(ch) && IS_PC_PET(ch) )
  {
    send_to_char( "Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET) );
    return;
  }

  if( (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) && !is_wearing_necroplasm(ch) && !IS_NPC(ch))
    || affected_by_spell(ch, SPELL_CORPSEFORM) )
  {
    send_to_char("You cannot control undead in that form!\n\r", ch);
    return;
  }

  // If corpse not in room with ch ?!
  if( !OBJ_IN_ROOM(obj, ch->in_room) )
  {
    act("Where'd $p go?? It's not in the room with you anymore!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if( obj->type != ITEM_CORPSE )
  {
    act("You can't animate $p!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if( strstr(obj->name, "acorpse") )
  {
    send_to_char("Cheater, eh? Go 'way.\r\n", ch);
    return;
  }

  clevel = obj->value[CORPSE_LEVEL];

  if( IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE) && (clevel < 0) )
  {
    clevel = -clevel;
  }

  if( clevel > (level + 4) && !IS_TRUSTED(ch) && (level < 49) )
  {
    act("You are not powerful enough to animate that corpse!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  sum = count_undead(ch);

  if( sum < 0 )
  {
    send_to_char("You cannot have dracoliches, titans, or avatars under your control when animating undead!\r\n", ch);
    return;
  }

  if( clevel < 4 )
  {
    send_to_char("The corpse is too weak to rise as undead!\r\n", ch);
    return;
  }

  if( (which_type >= 0) && (clevel < undead_data[which_type].corpse_level) )
  {
    send_to_char("That corpse is too weak to raise in that manner.\r\n", ch);
    return;
  }

  globe = get_globe( ch );

  if( globe )
  {
    act("&+LYour $q &+Lpulses with evil energy, infusing part of its malevolence into your undead abomination!&N",
          FALSE, ch, globe, 0, TO_CHAR);
    act("&+L$n's $q &+Lthrobs with evil energy, combining its unholy power with $s abomination!&N",
          FALSE, ch, globe, 0, TO_ROOM);
    life = (life + number(50, 100));
  }

  if( which_type < 0 )
  {
    do
    {
      if (GET_CLASS(ch, CLASS_THEURGIST))
        typ = (int) (number(0, MIN(level + 5, 45)) / 10) + ((NECROPET_LAST + 1) / 2);
      // Blighters only raise skeletons/zombies.
      else if (GET_CLASS(ch, CLASS_BLIGHTER))
        typ = number( 0, 1 );
      else
        typ = (int) (number(0, MIN(level + 5, 45)) / 10);
    }
    while( clevel < undead_data[typ].corpse_level );
  }
  else
  {
    typ = which_type;
  }

  int necro_power = level * 2;
  if( GET_SPEC(ch, CLASS_NECROMANCER, SPEC_NECROLYTE)
    || GET_SPEC(ch, CLASS_THEURGIST, SPEC_TEMPLAR) )
  {
    necro_power += 10;
  }
  // Lvl 15 : 1 pet, Lvl 30 : 2 pets, Lvl 45 : 3 pets, lvl 56 : 3 pets.
  if( IS_PC(ch) && GET_CLASS(ch, CLASS_BLIGHTER) )
  {
    necro_power = (necro_power) / 3;
  }

  if( (sum + undead_data[typ].cost > necro_power)
    && !IS_TRUSTED(ch) && IS_PC(ch) )
  {
    act("You are too weak to animate more corpses!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  undead = read_mobile(1201, VIRTUAL);

  if (!undead)
  {
    logit(LOG_DEBUG, "spell_raise_undead(): mob 1201 (undead) not loadable");
    send_to_char("Something is screwed up with the code. Can't read mob.\r\n", ch);
    return;
  }

  /* okie, a few changes so zombies aren't god-like. -JAB */
  GET_SIZE(undead) = SIZE_MEDIUM;
  undead->specials.affected_by = undead_data[typ].aff1;
  undead->specials.affected_by2 = undead_data[typ].aff2;

  undead->specials.act = undead_data[typ].act;
  SET_BIT(undead->specials.act, ACT_SENTINEL);
  SET_BIT(undead->only.npc->aggro_flags, AGGR_ALL);
  SET_BIT(undead->specials.affected_by2, AFF2_ULTRAVISION);
  SET_BIT(undead->specials.act, ACT_ISNPC);
  if (!IS_SET(undead->specials.act, ACT_MEMORY))
  {
    clearMemory(undead);
  }

  remove_plushit_bits(undead);

  GET_RACE(undead) = undead_data[typ].race;
  GET_SEX(undead) = SEX_NEUTRAL;

  if( (typ >= NECROPET_START) && (typ <= NECROPET_END) )
  {
    undead->specials.alignment = -1000;    /* evil aligned */
  }
  else if( (typ >= THEURPET_START) && (typ <= THEURPET_END) )
  {
    undead->specials.alignment = 1000;
  }

  num = (int) (clevel + (level - clevel) / 1.5);

  if( level < 50 )
  {
    cap = (int) ((((float) level) / 4 + 37.5) * undead_data[typ].max_level / 50);
  }
  else
  {
    cap = undead_data[typ].max_level;
  }

  undead->player.level = BOUNDED(1, num, cap);

  for (j = 1; j <= MAX_CIRCLE; j++)
  {
    undead->specials.undead_spell_slots[j] = spl_table[GET_LEVEL(undead)][j - 1];
  }

  undead->points.base_mana = GET_LEVEL(undead) * typ * 2;
  undead->points.mana = undead->points.base_mana;
  undead->player.m_class = undead_data[typ].pet_class;

  while( undead->affected )
  {
    affect_remove(undead, undead->affected);
  }

  undead->base_stats.Str = BOUNDED(74, undead->base_stats.Str, 100);
  undead->base_stats.Agi = BOUNDED(74, undead->base_stats.Agi, 100);
  undead->base_stats.Con = BOUNDED(74, undead->base_stats.Con, 100);
  undead->base_stats.Dex = BOUNDED(74, undead->base_stats.Dex, 100);
  undead->base_stats.Int = BOUNDED(84, undead->base_stats.Int, 100);
  undead->base_stats.Wis = BOUNDED(84, undead->base_stats.Int, 100);
  // Spectres get good stats for backstab (necromancers get str spell, so 90 str).
  if( which_type == NECROPET_SPECTRE )
  {
    undead->base_stats.Str = 90;
    undead->base_stats.Agi = 100;
    undead->base_stats.Dex = 100;
  }

  /* max hp: 800 - really lucky lich.  */
  GET_HIT(undead) = GET_MAX_HIT(undead) = undead->points.base_hit =
    (int) (dice(GET_LEVEL(undead), 12) + GET_LEVEL(ch) * undead_data[typ].hps + (life * 3));

  undead->points.vitality = undead->points.base_vitality = undead->points.max_vitality =
    MAX(50, undead->base_stats.Agi) + (undead->base_stats.Str + undead->base_stats.Con) / 2;

  undead->points.base_armor = 100 - GET_LEVEL(undead) - (typ * 15);

  MonkSetSpecialDie(undead);
  undead->points.base_hitroll = undead->points.hitroll = (int) (0.7 * GET_LEVEL(undead));
  // 24 - 32 at 50.
  undead->points.base_damroll = undead->points.damroll = (int) (0.55 * GET_LEVEL(undead)) + number(-6, 2);
  undead->points.damnodice = (undead->points.damnodice / 2 + 2);
  // 5 at 56.
  undead->points.damsizedice = (level - 1) / 11;

  StartRegen(undead, EVENT_MANA_REGEN);
  balance_affects(undead);
  undead->only.npc->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);

  if( (typ >= THEURPET_START) && (typ <= THEURPET_END) )
  {
    sprintf(Gbuf2, "%s", undead_data[typ].name);
    undead->player.name = str_dup(Gbuf2);
    if( typ == THEURPET_SPECTRE || typ == THEURPET_LICH || typ == THEURPET_SHADOW )
    {
      sprintf(Gbuf1, "an %s", undead_data[typ].short_desc);
      undead->player.short_descr = str_dup(Gbuf1);
      sprintf(Gbuf1, "An %s stands here.\r\n", undead_data[typ].short_desc);
      undead->player.long_descr = str_dup(Gbuf1);

    }
    else
    {
      sprintf(Gbuf1, "a %s", undead_data[typ].short_desc);
      undead->player.short_descr = str_dup(Gbuf1);
      sprintf(Gbuf1, "A %s stands here.\r\n", undead_data[typ].short_desc);
      undead->player.long_descr = str_dup(Gbuf1);
    }
  }
  else
  {
    sprintf(Gbuf2, "undead %s _%s_", undead_data[typ].name, undead_data[typ].name);
    undead->player.name = str_dup(Gbuf2);
    sprintf(Gbuf1, "the %s of %s", undead_data[typ].name, obj->action_description);
    undead->player.short_descr = str_dup(Gbuf1);
    sprintf(Gbuf1, "The %s of %s stands here.\r\n", undead_data[typ].name, obj->action_description);
    undead->player.long_descr = str_dup(Gbuf1);
  }

  if( IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE) )
  {
    logit(LOG_CORPSE, "%s got raised while equipped ( by %s in room %d ).",
      obj->short_description, (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr), world[ch->in_room].number);
    wizlog(57, "%s got raised while equipped ( by %s in room %d ).",
      obj->short_description, (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr), world[ch->in_room].number);
    corpselog = TRUE;
  }

  /* move objects in corpse to undead's inventory */
  if (obj->contains)
  {
    create_saved_corpse(obj, undead);
    for (obj_in_corpse = obj->contains; obj_in_corpse; obj_in_corpse = next_obj)
    {
      if( corpselog )
      {
        logit(LOG_CORPSE, "%s raised with eq: [%d] %s", obj->short_description,
          obj_index[obj_in_corpse->R_num].virtual_number, obj_in_corpse->name);
      }
      next_obj = obj_in_corpse->next_content;
      obj_from_obj(obj_in_corpse);
      obj_to_char(obj_in_corpse, undead);
    }
  }

  if( (typ >= THEURPET_START) && (typ <= THEURPET_END) )
  {
    act("After a short &+yr&+Yi&+Wtu&+Ya&+yl&n, the &+Wsoul&n of $N&n is called to inhabit the $p.", FALSE, ch, obj, undead, TO_CHAR);
    act("The $p &+btr&+Ban&+Csf&+Bor&+bms&n into $N&n and awaits instructions from &n&n.", FALSE, ch, obj, undead, TO_ROOM);
  }
  else
  {
    act("You breathe life into $p with the awesome power of your art.",
      FALSE, ch, obj, 0, TO_CHAR);
    act("You see $p take a deep breath, and suddenly come to life again.",
      FALSE, ch, obj, 0, TO_NOTVICT);
  }

  int timeToDecay = 0;
  struct obj_affect *afDecay = get_obj_affect(obj, TAG_OBJ_DECAY);
  if( NULL !=  afDecay )
  {
    timeToDecay = obj_affect_time(obj, afDecay) / (60 * 4); /* 4 pulses in a sec, 60 secs in a minute */
  }

  extract_obj(obj);
  char_to_room(undead, ch->in_room, 0);

  int duration = setup_pet(undead, ch, MAX(4, timeToDecay), PET_NOCASH);
  add_follower(undead, ch);

  /* if the undead will stop being charmed after a bit, also make it suicide 1-10 minutes later */
  if (duration >= 0)
  {
    duration += number(1,10);
    add_event(event_pet_death, (duration+1) * 60 * 4, undead, NULL, NULL, 0, NULL, 0);
  }
  if(GET_RACE(undead) == RACE_PLICH)
  {
    undead->player.m_class += CLASS_CONJURER;
    undead->player.secondary_class += CLASS_SORCERER;
    //send_to_char("is lich\r\n", ch);
  }

}

#undef UNDEAD_TYPES

// Necro Spells
void spell_raise_spectre(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, NECROPET_SPECTRE);
}

void spell_raise_wraith(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, NECROPET_WRAITH);
}

void spell_raise_shadow(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, NECROPET_SHADOW);
}

void spell_raise_vampire(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, NECROPET_VAMPIRE);
}

void spell_raise_lich(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, NECROPET_LICH);
}

void spell_animate_dead(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, -1);
}


// Theurgist Spells
void spell_call_asura(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, THEURPET_SPECTRE);
}

void spell_call_bralani(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, THEURPET_WRAITH);
}

void spell_call_deva(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, THEURPET_SHADOW);
}

void spell_call_knight(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, THEURPET_VAMPIRE);
}

void spell_call_liberator(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, THEURPET_LICH);
}

void spell_call_archon(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  raise_undead(level, ch, victim, obj, -1);
}


void mummify(P_char ch, P_char vict, P_obj corpse)
{
  act
    ("Your hand glows and with a twist of your wrist, you mummify the remains of $N.",
     FALSE, ch, NULL, vict, TO_CHAR);
  act("$n gestures $s hand causing $N's body to glow with a deep red aura.",
      FALSE, ch, NULL, vict, TO_ROOM);

  spell_embalm(MAXLVL, ch, 0, SPELL_TYPE_SPELL, NULL, corpse);

}

void spawn_raise_undead(P_char ch, P_char vict, P_obj corpse)
{
  P_char   follower;
  struct follow_type *ft;
  int      sum, type = 0;
  int      roll = (corpse->value[CORPSE_LEVEL]) / 5 + number(1, 20);

  if( !IS_ALIVE(ch) )
  {
    logit(LOG_DEBUG, "spawn_raise_undead: Missing or dead char.");
    debug( "spawn_raise_undead: Missing or dead char '%s'.", ch ? J_NAME(ch) : "Null" );
    return;
  }
  if( IS_NPC(ch) && IS_PC_PET(ch) )
  {
    send_to_char( "Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET) );
    return;
  }

  sum = 0;
  for (ft = ch->followers; ft; ft = ft->next)
  {
    follower = ft->follower;
    if (IS_PC(follower))
      continue;
    if (strstr(GET_NAME(follower), "skeleton") ||
	strstr(GET_NAME(follower), "hound archon brawler"))
    {
      sum += 2;
    }
    else if (strstr(GET_NAME(follower), "zombie") ||
	     strstr(GET_NAME(follower), "lantern archon soldier"))
    {
      sum += 3;
    }
    else if (strstr(GET_NAME(follower), "spectre") ||
	     strstr(GET_NAME(follower), "asura avenger"))
    {
      sum += 3;
    }
    else if (strstr(GET_NAME(follower), "wraith") ||
	     strstr(GET_NAME(follower), "bralani battlemage"))
    {
      sum += 4;
    }
    else if (strstr(GET_NAME(follower), "vampire") ||
	     strstr(GET_NAME(follower), "knight-errant knight errant ghaele"))
    {
      sum += 4;
    }
    else if (strstr(GET_NAME(follower), "dracolich") ||
	     strstr(GET_NAME(follower), "titan") ||
	     strstr(GET_NAME(follower), "avatar"))
    {
      sum += 9;
    }
    else if (strstr(GET_NAME(follower), "lich") ||
	     strstr(GET_NAME(follower), "liberator holy ghaele"))
    {
      sum += 5;
    }
  }

  if (roll > 27)
  {
    if (sum == 0)
    {
      
      if (GET_CLASS(ch, CLASS_THEURGIST))
      {
        act("You place your hand on $N upon $S last dying breath.", FALSE, ch, 0, vict, TO_CHAR);
        act("After a short &+yr&+Yi&+Wtu&+Ya&+yl&n, the &+Wsoul&n of $N&n is called to inhabit the $p.", FALSE, ch, 0, vict, TO_CHAR);
	act("$n&n places $s hand on $N upon $S last dying breath.", FALSE, ch, 0, vict, TO_ROOM);
        act("The $p &+btr&+Ban&+Csf&+Bor&+bms&n into $N&n and awaits instructions from &n&n.", FALSE, ch, 0, vict, TO_ROOM);
      }
      else
      {
        act("You plunge your hand into $N's chest grasping $S dying heart.",
          FALSE, ch, NULL, vict, TO_CHAR);
        act
          ("Uttering an unholy curse you will $S soul back from the clutches of death.",
          FALSE, ch, NULL, vict, TO_CHAR);
        act("$n plunges $s hand into $N's chest grasping $S dying heart.",
          FALSE, ch, NULL, vict, TO_ROOM);
        act
          ("Uttering an unholy curse $n wills the soul back from the clutches of death",
          FALSE, ch, NULL, vict, TO_ROOM);
       }

      if (GET_CLASS(ch, CLASS_THEURGIST))
        spell_call_titan(56, ch, NULL, SPELL_TYPE_SPELL, NULL, corpse);
      else
        spell_create_dracolich(56, ch, NULL, SPELL_TYPE_SPELL, NULL, corpse);
    }
  }
  else if (roll > 22)
  {
    if (sum == 0)
      if (GET_CLASS(ch, CLASS_THEURGIST))
        type = THEURPET_LICH;
      else
        type = NECROPET_LICH;
       
  }
  else if (roll > 20)
  {
    if (sum < 6)
      if (GET_CLASS(ch, CLASS_THEURGIST))
        type = THEURPET_VAMPIRE;
      else
        type = NECROPET_VAMPIRE;
  }
  else if (roll > 10)
  {
    if (sum < 6)
    if (GET_CLASS(ch, CLASS_THEURGIST))
      type = THEURPET_WRAITH;
    else
      type = NECROPET_WRAITH;
  }
  else if (roll > 6)
  {
    if (sum < 7)
      if (GET_CLASS(ch, CLASS_THEURGIST))
        type = THEURPET_SPECTRE;
      else
        type = NECROPET_SPECTRE;
  }
  else if (sum < 8)
  {
    type = -1;
  }

  if (type != 0)
  {
    act("You plunge your hand into $N's chest grasping $S dying heart.",
        FALSE, ch, NULL, vict, TO_CHAR);
    act
      ("Uttering an unholy curse you will $S soul back from the clutches of death.",
       FALSE, ch, NULL, vict, TO_CHAR);
    act("$n plunges $s hand into $N's chest grasping $S dying heart.", FALSE,
        ch, NULL, vict, TO_ROOM);
    act
      ("Uttering an unholy curse $n wills the soul back from the clutches of death",
       FALSE, ch, NULL, vict, TO_ROOM);
//    spell_embalm(50, ch, 0, SPELL_TYPE_SPELL, NULL, corpse);
    raise_undead(GET_LEVEL(ch), ch, NULL, corpse, type);
  }
}

void spell_spawn(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if (affected_by_spell(ch, SPELL_SPAWN))
  {
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_SPAWN;
  af.duration = 5;
  af.bitvector = 0;

  affect_to_char(ch, &af);
  act("An aura of &+Ldeath&N surrounds you.", FALSE, ch, 0, ch, TO_CHAR);
  act("The smell of &+Ldeath&N and &+Lsuffering&N emanates from $n.", FALSE,
      ch, 0, ch, TO_ROOM);
}

void spell_call_titan(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  P_obj    obj_in_corpse = NULL, next_obj = NULL;
  P_obj    globe;
  struct follow_type *k;
  int      i, sum, num_undead = 0;
  int      life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
  bool     corpselog;
  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {
    78, "&+rThe corpse summons the spirit of &+ra &+Rcrimson &+rtitan&n!"},
    {
    79, "&+bThe corpse summons the spirit of &+Ba &+bazure &+Btitan&n!"},
    {
    80, "&+gThe corpse summons the spirit of &+ga &+Gjasper &+gtitan&n!"},
    {
    81, "&+LThe corpse summons the spirit of &+wa &+Lbasalt &+wtitan&n!"}
  };

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    send_to_char("But you can barely move!\r\n", ch);
    return;
  }

  if (CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your spell!\r\n", ch);
    return;
  }

  if (obj->value[CORPSE_LEVEL] < 46)
  {
    send_to_char("This spell requires the corpse of a more powerful being!\r\n", ch);
    return;
  }
  
  if (obj->type != ITEM_CORPSE)
  {
    act("You can't animate $p!", FALSE, ch, obj, 0, TO_CHAR);
    return;

  }
  /*
  if (count_undead(ch) > 0)
  {
    send_to_char
      ("You cannot have undead under your control when summoning titans!\r\n",
       ch);
    return;
  }
  */
  if (!can_raise_draco(ch, level, false))
  {
    send_to_char("You cannot control any more titans!\r\n", ch);
    return;
  }
  
  if((IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) &&
    !is_wearing_necroplasm(ch) &&
    !IS_NPC(ch)) ||
    affected_by_spell(ch, SPELL_CORPSEFORM))
  {
    send_to_char("You cannot control undead in that form!\n\r", ch);
    return;
  }

  if (obj->loc.room != ch->in_room)
  {
    send_to_char("You cannot raise a corpse in another room.\r\n", ch);
    return;
  }
   
  globe = get_globe( ch );

  if( globe )
  {
     act("&+LYour $q &+Lpulses with evil energy, infusing part of its malevolence into your titan!&N",
          FALSE, ch, globe, 0, TO_CHAR);
    act("&+L$n's $q &+Lthrobs with evil energy, combining its unholy power with $s titan!&N",
          FALSE, ch, globe, 0, TO_ROOM);
    life += GET_LEVEL(ch) * 2;
  }

  sum = number(0, 3);
  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);
  
  if (!mob)
  {
    logit(LOG_DEBUG, "spell_call_titan(): mob %d not loadable",
          summons[sum].mob_number);
    send_to_char("Bug in call titan.  Tell a god!\r\n", ch);
    return;
  }
  
  GET_SIZE(mob) = SIZE_GIANT;
  GET_RACE(mob) = RACE_TITAN;
  SET_BIT(mob->specials.act, ACT_MOUNT);
  GET_HIT(mob) = GET_MAX_HIT(mob) = mob->points.base_hit = dice(125, 15) + (life * 3);
  mob->specials.act |= ACT_SPEC_DIE;
  //REMOVE_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
  SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
  mob->specials.alignment = 1000;
  
  if(IS_NPC(ch) &&
    !IS_PC_PET(ch))
  {
    if(!IS_SET(mob->only.npc->aggro_flags, AGGR_ALL))
    {
      SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
    }
    if(IS_SET(mob->specials.act, ACT_SENTINEL))
    {
      REMOVE_BIT(mob->specials.act, ACT_SENTINEL);
    }
    if(!SET_BIT(mob->specials.act, ACT_HUNTER))
    {
      SET_BIT(mob->specials.act, ACT_HUNTER);
    }
    if(SET_BIT(mob->specials.act, ACT_GUILD_GOLEM))
    {
      REMOVE_BIT(mob->specials.act, ACT_GUILD_GOLEM);
    }
    if(GET_LEVEL(ch) >= 54)
    {
      mob->points.damnodice = 10;
      mob->points.base_hitroll = mob->points.hitroll = (GET_LEVEL(ch));
    }
  }

  char_to_room(mob, ch->in_room, 0);

  if(IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE))
  {
    logit(LOG_CORPSE,
          "%s got raised as titan ( by %s in room %d ).",
          obj->short_description,
          (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr),
          world[ch->in_room].number);
    wizlog(57,
           "%s got raised as titan ( by %s in room %d ).",
           obj->short_description,
           (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr),
           world[ch->in_room].number);
    corpselog = TRUE;
  }

  if (obj->contains)
  {
    create_saved_corpse(obj, mob);
    for (obj_in_corpse = obj->contains; obj_in_corpse;
         obj_in_corpse = next_obj)
    {
      if (corpselog)
        logit(LOG_CORPSE, "%s raised with eq: [%d] %s", obj->short_description, 
              obj_index[obj_in_corpse->R_num].virtual_number,
              obj_in_corpse->name);
      next_obj = obj_in_corpse->next_content;
      obj_from_obj(obj_in_corpse);
      obj_to_char(obj_in_corpse, mob);
    }
  }
  
  int timeToDecay = 0;
  struct obj_affect *afDecay = get_obj_affect(obj, TAG_OBJ_DECAY);
  
  if (NULL !=  afDecay)
  {
    timeToDecay = obj_affect_time(obj, afDecay) / (60 * 4); /* 4 pulses in a sec, 60 secs in a minute */
  }

  extract_obj(obj);
  remove_plushit_bits(mob);
  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);
  balance_affects(mob);


  /* all dracoliches are around level 52, and PCs must be at least level 51 to cast
     this spell, so I'm just gonna make it a straight random chance */

  if (IS_PC(ch) &&
     (!number(0, 12)) /*(GET_LEVEL(mob) > number((level - i * 2) * 2, level *
                                           3)) */ )
  {
    act("$N is NOT pleased at being returned to life!", TRUE, ch, 0, mob,
        TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
  }
  else
  {                             /* Under control */
    act("&+W$N roars to the sky 'I LIVE!!!'", TRUE, ch, 0, mob, TO_ROOM);
    act("&+W$N roars to the sky 'I LIVE!!!'", TRUE, ch, 0, mob, TO_CHAR);

    radiate_message_from_room(ch->in_room, "&+cYou hear a loud roar in the distance.\r\n", 3, 
                              (RMFR_FLAGS) (RMFR_RADIATE_ALL_DIRS | RMFR_PASS_WALL | RMFR_PASS_DOOR | RMFR_CROSS_ZONE_BARRIER), 0);	

    GET_AC(mob) -= 50;
    int duration = setup_pet(mob, ch, timeToDecay/2 + (6000 / STAT_INDEX(GET_C_INT(mob))), PET_NOCASH);
    add_follower(mob, ch);
    
    
  /* if the undead will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if (duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

struct SavedCorpseData
{
  P_obj corpse;
};

void check_saved_corpse(P_char ch)
{
  P_nevent e = NULL;
  P_obj corpse = NULL;

  if (!ch)
    return;

  if (e = get_scheduled(ch, event_saved_corpse))
  {
    SavedCorpseData *data = (SavedCorpseData*) e->data;
    corpse = data->corpse;
  }

  if (corpse && corpse->loc.room == real_room(CORPSE_STORAGE))
    extract_obj(corpse);
}

void event_saved_corpse(P_char ch, P_char vict, P_obj obj, void *data)
{
  if (!ch || !data)
    return;

  SavedCorpseData *savedCorpseData = (SavedCorpseData*) data;

  add_event(event_saved_corpse, 1000, ch, 0, 0, 0, savedCorpseData, sizeof(SavedCorpseData));
}

void create_saved_corpse(P_obj obj, P_char mob)
{
  char buff[MAX_STRING_LENGTH];
  if (!obj || !mob)
    return;

  if(!IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE))
    return;

  P_obj savecorpse = clone_obj(obj);
  clone_container_obj(savecorpse, obj);
  sprintf(buff, "%s %s", savecorpse->name, "_savecorpse_");
  savecorpse->name = str_dup(buff);
  savecorpse->value[CORPSE_SAVEID] = time(NULL);
  obj_to_room(savecorpse, real_room(40));
  affect_from_obj(savecorpse, TAG_OBJ_DECAY);
  SavedCorpseData savedCorpseData;
  savedCorpseData.corpse = savecorpse;
  add_event(event_saved_corpse, 0, mob, NULL, NULL, 0, &savedCorpseData, sizeof(SavedCorpseData));
}

void spell_create_dracolich(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  P_obj    obj_in_corpse = NULL, next_obj = NULL;
  P_obj    globe;
  struct follow_type *k;
  int      i, sum, num_undead = 0;
  int      life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
  bool     corpselog;
  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {
    3, "&+rThe corpse summons the spirit of a red dragon!"},
    {
    4, "&+bThe corpse summons the spirit of a blue dragon!"},
    {
    5, "&+gThe corpse summons the spirit of a green dragon!"},
    {
    6, "&+LThe corpse summons the spirit of a black dragon!"}
  };

  if( !IS_ALIVE(ch) )
  {
    logit(LOG_DEBUG, "spell_create_dracolich: Missing or dead char.");
    debug( "spell_create_dracolich: Missing or dead char '%s'.", ch ? J_NAME(ch) : "Null" );
    return;
  }

  if( IS_IMMOBILE(ch) )
  {
    send_to_char("But you can barely move!\r\n", ch);
    return;
  }

  if( IS_NPC(ch) && affected_by_spell(ch, TAG_CONJURED_PET) )
  {
    return;
  }

  if( CHAR_IN_SAFE_ZONE(ch) )
  {
    send_to_char("A mysterious force blocks your spell!\r\n", ch);
    return;
  }

  if( obj->value[CORPSE_LEVEL] < 46 )
  {
    send_to_char("This spell requires the corpse of a more powerful being!\r\n", ch);
    return;
  }

  if (obj->type != ITEM_CORPSE)
  {
    act("You can't animate $p!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  /*
  if (count_undead(ch) > 0)
  {
    send_to_char
      ("You cannot have undead under your control when summoning dracoliches!\r\n",
       ch);
    return;
  }
  */
  if (!can_raise_draco(ch, level, false))
  {
    send_to_char("You cannot control any more dracoliches!\r\n", ch);
    return;
  }

  if((IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) &&
    !is_wearing_necroplasm(ch) &&
    !IS_NPC(ch)) ||
    affected_by_spell(ch, SPELL_CORPSEFORM))
  {
    send_to_char("You cannot control undead in that form!\n\r", ch);
    return;
  }

  if (obj->loc.room != ch->in_room)
  {
    send_to_char("You cannot raise a corpse in another room.\r\n", ch);
    return;
  }

  globe = get_globe( ch );

  if( globe )
  {
     act("&+LYour $q &+Lpulses with evil energy, infusing part of its malevolence into your abomination!&N",
          FALSE, ch, globe, 0, TO_CHAR);
    act("&+L$n's $q &+Lthrobs with evil energy, combining its unholy power with $s abomination!&N",
          FALSE, ch, globe, 0, TO_ROOM);
    life += GET_LEVEL(ch) * 2;
  }

  /*P_obj dragonscale = NULL;
  for( P_obj t_obj = ch->carrying; t_obj; t_obj->next_content )
  {
    if( obj_index[t_obj->R_num].virtual_number == DRAGONSCALE_VNUM )
    {
      dragonscale = t_obj;
      break;
    }
  }

  if( !dragonscale )
  {
     send_to_char("Raising a dracolich requires &+La dragon scale&n to bind the dragon spirit to the corpse.\r\n", 			ch );
           return;
  }

  extract_obj(dragonscale);
  */

  sum = number(0, 3);
  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);

  if( !mob )
  {
    logit(LOG_DEBUG, "spell_create_dracolich(): mob %d not loadable",
          summons[sum].mob_number);
    send_to_char("Bug in create dracolich.  Tell a god!\r\n", ch);
    return;
  }

  GET_SIZE(mob) = SIZE_GIANT;
  GET_RACE(mob) = RACE_DRACOLICH;
  SET_BIT(mob->specials.act, ACT_MOUNT);
  GET_HIT(mob) = GET_MAX_HIT(mob) = mob->points.base_hit = dice(125, 15) + (life * 5);
  mob->specials.act |= ACT_SPEC_DIE;
  //REMOVE_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
  SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);

  if(IS_NPC(ch) && !IS_PC_PET(ch))
  {
    if(!IS_SET(mob->only.npc->aggro_flags, AGGR_ALL))
    {
      SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
    }
    if(IS_SET(mob->specials.act, ACT_SENTINEL))
    {
      REMOVE_BIT(mob->specials.act, ACT_SENTINEL);
    }
    if(!SET_BIT(mob->specials.act, ACT_HUNTER))
    {
      SET_BIT(mob->specials.act, ACT_HUNTER);
    }
    if(SET_BIT(mob->specials.act, ACT_GUILD_GOLEM))
    {
      REMOVE_BIT(mob->specials.act, ACT_GUILD_GOLEM);
    }
    if(GET_LEVEL(ch) >= 50)
    {
      mob->points.damnodice = 15;
      mob->points.base_hitroll = mob->points.hitroll = (GET_LEVEL(ch));
    }
  }
  else
  {
    // 5d6 at level 56.
    mob->points.damnodice = (level - 6) / 10;
    mob->points.damsizedice = (level + 10) / 11;
    // 60 - 80 (actual 15 - 20) at level 56.
    mob->points.base_damroll = mob->points.damroll = level + number( 4, 24 );
  }

  char_to_room(mob, ch->in_room, 0);

  if(IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE))
  {
    logit(LOG_CORPSE, "%s got raised as dracolich ( by %s in room %d ).", obj->short_description,
      (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr), world[ch->in_room].number);
    wizlog(57, "%s got raised as dracolich ( by %s in room %d ).", obj->short_description,
      (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr), world[ch->in_room].number);
    corpselog = TRUE;
  }

  if( obj->contains )
  {
    create_saved_corpse(obj, mob);
    for (obj_in_corpse = obj->contains; obj_in_corpse;
         obj_in_corpse = next_obj)
    {
      if( corpselog )
      {
        logit(LOG_CORPSE, "%s raised with eq: [%d] %s", obj->short_description, 
         obj_index[obj_in_corpse->R_num].virtual_number, obj_in_corpse->name);
      }
      next_obj = obj_in_corpse->next_content;
      obj_from_obj(obj_in_corpse);
      obj_to_char(obj_in_corpse, mob);
    }
  }

  int timeToDecay = 0;
  struct obj_affect *afDecay = get_obj_affect(obj, TAG_OBJ_DECAY);

  if( NULL != afDecay )
  {
    // WAIT_SEC pulses in a sec, 60 secs in a minute.
    timeToDecay = obj_affect_time(obj, afDecay) / (60 * WAIT_SEC);
  }

  extract_obj(obj);
  remove_plushit_bits(mob);
  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);
  balance_affects(mob);


  /* all dracoliches are around level 52, and PCs must be at least level 51 to cast
     this spell, so I'm just gonna make it a straight random chance */
  // (GET_LEVEL(mob) > number((level - i * 2) * 2, level * 3))
  if( IS_PC(ch) && !number(0, 12) && !globe )
  {
    act("$N is NOT pleased at being returned to life!", TRUE, ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
  }
  else
  {
    act("&+W$N roars to the sky 'I LIVE!!!'", TRUE, ch, 0, mob, TO_ROOM);
    act("&+W$N roars to the sky 'I LIVE!!!'", TRUE, ch, 0, mob, TO_CHAR);

    radiate_message_from_room(ch->in_room, "&+cYou hear a loud roar in the distance.\r\n", 3, 
                              (RMFR_FLAGS) (RMFR_RADIATE_ALL_DIRS | RMFR_PASS_WALL | RMFR_PASS_DOOR | RMFR_CROSS_ZONE_BARRIER), 0);	

    GET_AC(mob) -= (GET_LEVEL(ch) * 7);
    int duration = setup_pet(mob, ch, timeToDecay/2 + (6000 / STAT_INDEX(GET_C_INT(mob))), PET_NOCASH);
    add_follower(mob, ch);

    // if the undead will stop being charmed after a bit, also make it suicide 1 minute later.
    if( duration >= 0 )
    {
      add_event(event_pet_death, (duration+1) * 60 * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

void spell_create_golem(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  int      pet_type;
  int      clevel;

  if( !IS_ALIVE(ch) )
  {
    logit(LOG_DEBUG, "spell_create_golem: Missing or dead char.");
    debug( "spell_create_golem: Missing or dead char '%s'.", ch ? J_NAME(ch) : "Null" );
    return;
  }
  if( IS_NPC(ch) && IS_PC_PET(ch) )
  {
    send_to_char( "Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET) );
    return;
  }
  if( !obj )
  {
    logit(LOG_DEBUG, "spell_create_golem: Missing obj.");
    debug( "spell_create_golem: Missing obj." );
    return;
  }

  clevel = obj->value[CORPSE_LEVEL];

  if (level >= 41)
    pet_type = NECROGOLEM_FLESH;

  if (level >= 46 && clevel >= 46)
    pet_type = NECROGOLEM_BLOOD;

  if (level >= 51 && clevel >= 51)
    pet_type = NECROGOLEM_BONE;

  if (GET_CLASS(ch, CLASS_THEURGIST))
    pet_type = THEURGOLEM_VALOR;

  create_golem(level, ch, victim, obj, pet_type);
}

void create_golem(int level, P_char ch, P_char victim, P_obj obj,
                  int which_type)
{
  P_char   mob;
  bool     corpselog;
  P_obj    obj_in_corpse = NULL, next_obj = NULL;
  P_obj    globe;
  struct follow_type *k;
  int      num, clevel, i, cap, sum, num_undead = 0;
  int      life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
  int      percent;

  if( !IS_ALIVE(ch) )
  {
    logit(LOG_DEBUG, "create_golem: Missing or dead char.");
    debug( "create_golem: Missing or dead char '%s'.", ch ? J_NAME(ch) : "Null" );
    return;
  }
  if( IS_NPC(ch) && IS_PC_PET(ch) )
  {
    send_to_char( "Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET) );
    return;
  }
  if( !obj )
  {
    logit(LOG_DEBUG, "create_golem: Missing obj.");
    debug( "create_golem: Missing obj." );
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    send_to_char("But you can barely move!\r\n", ch);
    return;
  }

  if (CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your spell!\r\n", ch);
    return;
  }
  if (obj->value[CORPSE_LEVEL] < golem_data[which_type].corpse_lvl)
  {
    send_to_char("This spell requires the corpse of a more powerful being!\r\n", ch);
    return;
  }

  if (obj->type != ITEM_CORPSE)
  {
    act("You can't animate $p!", FALSE, ch, obj, 0, TO_CHAR);
    return;

  }

  if((IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) &&
    !is_wearing_necroplasm(ch) &&
    !IS_NPC(ch)) ||
    affected_by_spell(ch, SPELL_CORPSEFORM))
  {
    send_to_char("You cannot control undead in that form!\n\r", ch);
    return;
  }

  if (obj->loc.room != ch->in_room)
  {
    send_to_char("You cannot raise a corpse in another room\r\n", ch);
    return;
  }

  sum = count_undead(ch);
  if (sum < 0)
  {
    send_to_char
      ("You cannot have dracoliches under your control when animating golems!\r\n",
       ch);
    return;
  }

  if ((sum + golem_data[which_type].cost > GET_LEVEL(ch) / 3) &&
      !IS_TRUSTED(ch) && IS_PC(ch))
  {
    act("You are too weak to animate more corpses!", FALSE, ch, 0, 0,
        TO_CHAR);
    return;
  }

  mob = read_mobile(real_mobile(golem_data[which_type].vnum), REAL);

  if (!mob)
  {
    logit(LOG_DEBUG, "create_golem(): mob %d not loadable",
          golem_data[which_type].vnum);
    send_to_char("Bug in create_golem.  Tell a god!\r\n", ch);
    return;
  }

  clevel = obj->value[CORPSE_LEVEL];
  if (IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE) && (clevel < 0))
    clevel = -clevel;

  if (clevel > (level + 4) && !IS_TRUSTED(ch) && (level < 49))
  {
    act("You are not powerful enough to animate that corpse!", FALSE, ch, 0,
        0, TO_CHAR);
    return;
  }

  globe = get_globe( ch );
  if( globe )
  {
    act("&+LYour $q &+Lpulses with evil energy, infusing part of it's malevolence into your undead abomination!&N",
          FALSE, ch, globe, 0, TO_CHAR);
    act("&+L$n's $q &+Lthrobs with evil energy, combining its unholy power with $s abomination!&N",
          FALSE, ch, globe, 0, TO_ROOM);
    life = GET_LEVEL(ch) * 2;
  }

  num = (int) (clevel + (level - clevel) / 1.5);

  if (level < 50)
    cap = (int) ((((float) level) / 4 + 37.5) * golem_data[which_type].max_level / 50);
  else
    cap = golem_data[which_type].max_level;

  mob->player.level = BOUNDED(1, num, cap);

  mob->points.vitality = mob->points.base_vitality =
  mob->points.max_vitality = MAX(50, mob->base_stats.Agi) + (mob->base_stats.Str + mob->base_stats.Con) / 2;

  mob->points.base_armor = 0 - GET_LEVEL(mob);
  mob->specials.affected_by = AFF_FLY + AFF_DETECT_INVISIBLE;

  percent = number(0, 100);

  if (percent > 65)
  {
    // All golems are already golems now.
    //GET_RACE(mob) = RACE_GOLEM;
    SET_BIT(mob->specials.affected_by4, AFF4_VAMPIRE_FORM);
  }

  GET_SIZE(mob) = SIZE_HUGE;
  GET_HIT(mob) = GET_MAX_HIT(mob) = mob->points.base_hit =
    (int) (dice(GET_LEVEL(mob), 11) +
           GET_LEVEL(ch) * golem_data[which_type].hps + (life * 3));

  MonkSetSpecialDie(mob);
  mob->points.base_hitroll = mob->points.hitroll = (int) (0.90 * GET_LEVEL(mob));
  mob->points.base_damroll = mob->points.damroll = (int) (0.70 * GET_LEVEL(mob));
  mob->points.damnodice = (mob->points.damnodice / 2 + 2);

  char_to_room(mob, ch->in_room, 0);

  if (obj->contains)
  {
    create_saved_corpse(obj, mob);
    for (obj_in_corpse = obj->contains; obj_in_corpse; obj_in_corpse = next_obj)
    {
      if (corpselog)
        logit(LOG_CORPSE, "%s raised with eq: [%d] %s",
              obj->short_description,
              obj_index[obj_in_corpse->R_num].virtual_number,
              obj_in_corpse->name);
      next_obj = obj_in_corpse->next_content;
      obj_from_obj(obj_in_corpse);
      obj_to_char(obj_in_corpse, mob);
    }

  }
  int timeToDecay = 0;
  struct obj_affect *afDecay = get_obj_affect(obj, TAG_OBJ_DECAY);
  if (NULL !=  afDecay)
  {
    timeToDecay = obj_affect_time(obj, afDecay) / (60 * 4); /* 4 pulses in a sec, 60 secs in a minute */
  }

  extract_obj(obj);
  remove_plushit_bits(mob);
  //REMOVE_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
  SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);

  balance_affects(mob);

  int duration = setup_pet(mob, ch, (timeToDecay/2) + (6000 / STAT_INDEX(GET_C_INT(mob))), PET_NOCASH);
  
  act("&+LDark shadows engulf the &+bcorpse &+Las you weave a spell of &+Wreanimation&+L,\r\n"
      "&+Ltransforming the corpse into an &+wundead &+rminion&+L.", FALSE, ch, 0, 0, TO_CHAR);
  act("&+LDark shadows fill the room as $n &+Lchants and descend upon the\r\n"
      "&+wrecently &+Wdeceased &+Ltransforming the corpse into an &+wundead &+rminion&+L.",
       FALSE, ch, 0, 0, TO_ROOM);
  add_follower(mob, ch);
  /* if the undead will stop being charmed after a bit, also make it suicide 1-10 minutes later */
  if (duration >= 0)
  {
    duration += number(1,10);
    add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
  }

}

void spell_call_avatar(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  bool     corpselog;
  P_obj    obj_in_corpse = NULL, next_obj = NULL;
  P_obj    globe;
  struct follow_type *k;
  int      i, sum, num_undead = 0;
  int      life;
  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {
    82, "The corpse summons &+wan &+cav&+Ca&+Wt&+Ca&+cr &+wof the &+Warch&+wangel of &+rWar&+w!&n"},
    {
    83, "The corpse summons &+wan &+cav&+Ca&+Wt&+Ca&+cr &+wof the &+Warch&+wangel of &+RJudgement&+w!&n"},
    {
    84, "The corpse summons &+wan &+cav&+Ca&+Wt&+Ca&+cr &+wof the &+Warch&+wangel of &+LVengeance&+w!&n"},
    {
    85, "The corpse summons &+wan &+cav&+Ca&+Wt&+Ca&+cr &+wof the &+Warch&+wangel of &+CJustice&+w!&n"}
  };

  if( !IS_ALIVE(ch) )
  {
    logit(LOG_DEBUG, "spell_call_avatar: Missing or dead char.");
    debug( "spell_call_avatar: Missing or dead char '%s'.", ch ? J_NAME(ch) : "Null" );
    return;
  }
  if( IS_NPC(ch) && IS_PC_PET(ch) )
  {
    send_to_char( "Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET) );
    return;
  }
  if( !obj )
  {
    logit(LOG_DEBUG, "spell_call_avatar: Missing obj.");
    debug( "spell_call_avatar: Missing obj." );
    return;
  }
  if(IS_IMMOBILE(ch))
  {
    send_to_char("But you can barely move!\r\n", ch);
    return;
  }
  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your spell!\r\n", ch);
    return;
  }
  if(obj->value[CORPSE_LEVEL] < 51)
  {
    send_to_char("This spell requires the corpse of a more powerful being!\r\n", ch);
    return;
  }
  if(obj->type != ITEM_CORPSE)
  {
    act("You can't animate $p!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  /*
  if(count_undead(ch) > 0)
  {
    send_to_char
      ("You cannot have undead under your control when summoning avatars!\r\n",
       ch);
    return;
  }
  */
  if((IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) &&
    !is_wearing_necroplasm(ch) &&
    !IS_NPC(ch)) ||
    affected_by_spell(ch, SPELL_CORPSEFORM))
  {
    send_to_char("You cannot control undead in that form!\n\r", ch);
    return;
  }

  if(!can_raise_draco(ch, level, true))
  {
    send_to_char("You cannot control any more avatars!\r\n", ch);
    return;
  }
  
  if(obj->loc.room != ch->in_room)
  {
    send_to_char("You cannot raise a corpse in another room\r\n", ch);
    return;
  }

  life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
  globe = get_globe( ch );
  
  if( globe )
  {
    act("&+LYour $q &+Lpulses with evil energy, infusing part of it's malevolence into your undead abomination!&N",
          FALSE, ch, globe, 0, TO_CHAR);
    act("&+L$n's $q &+Lthrobs with evil energy, combining its unholy power with $s abomination!&N",
          FALSE, ch, globe, 0, TO_ROOM);
    life += GET_LEVEL(ch) * 2;
  }

  sum = number(0, 3);
  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);
  
  if(!mob)
  {
    logit(LOG_DEBUG, "spell_call_avatar(): mob %d not loadable",
          summons[sum].mob_number);
    send_to_char("Bug in call avatar. Tell a god!\r\n", ch);
    return;
  }
  
  GET_SIZE(mob) = SIZE_GIANT;
  GET_RACE(mob) = RACE_AVATAR;
  SET_BIT(mob->specials.act, ACT_MOUNT);
  mob->specials.act |= ACT_SPEC_DIE;
  GET_HIT(mob) = GET_MAX_HIT(mob) = mob->points.base_hit = 2500 + number(-10, 50) + (life * 5);
  //REMOVE_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
  SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
  mob->specials.alignment = 1000;

  if(IS_NPC(ch) &&
    !IS_PC_PET(ch))
  {
    if(!IS_SET(mob->only.npc->aggro_flags, AGGR_ALL))
    {
      SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
    }
/*    if(IS_SET(mob->specials.act, ACT_SENTINEL))
    {
      REMOVE_BIT(mob->specials.act, ACT_SENTINEL);
    } */
    if(!SET_BIT(mob->specials.act, ACT_HUNTER))
    {
      SET_BIT(mob->specials.act, ACT_HUNTER);
    }
    if(SET_BIT(mob->specials.act, ACT_GUILD_GOLEM))
    {
      REMOVE_BIT(mob->specials.act, ACT_GUILD_GOLEM);
    }
    if(GET_LEVEL(ch) >= 54)
    {
      if(mob->points.damnodice < 15)
      {
        mob->points.damnodice = 15;
      }
      mob->points.base_hitroll = mob->points.hitroll = (GET_LEVEL(ch));
    }
  }

  if(GET_C_STR(mob) < 95 ||
     GET_C_DEX(mob) < 95)
  {
    mob->base_stats.Str = 100;
    mob->base_stats.Dex = 100;
  }
  
  char_to_room(mob, ch->in_room, 0);

  if(IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE))
  {
    logit(LOG_CORPSE,
          "%s got raised as avatar while equipped ( by %s in room %d ).",
          obj->short_description,
          (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr),
          world[ch->in_room].number);
    wizlog(57,
           "%s got raised as avatar while equipped ( by %s in room %d ).",
           obj->short_description,
           (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr),
           world[ch->in_room].number);
    corpselog = TRUE;
  }

  if (obj->contains)
  {
    create_saved_corpse(obj, mob);
    for (obj_in_corpse = obj->contains; obj_in_corpse;
         obj_in_corpse = next_obj)
    {
      if (corpselog)
        logit(LOG_CORPSE, "%s raised with eq: [%d] %s",
              obj->short_description,
              obj_index[obj_in_corpse->R_num].virtual_number,
              obj_in_corpse->name);
      next_obj = obj_in_corpse->next_content;
      obj_from_obj(obj_in_corpse);
      obj_to_char(obj_in_corpse, mob);
    }

  }
  int timeToDecay = 0;
  struct obj_affect *afDecay = get_obj_affect(obj, TAG_OBJ_DECAY);
  if (NULL !=  afDecay)
  {
    timeToDecay = obj_affect_time(obj, afDecay) / (60 * 4); /* 4 pulses in a sec, 60 secs in a minute */
  }
  extract_obj(obj);

  remove_plushit_bits(mob);

  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);


  balance_affects(mob);

  /* all dracoliches are around level 52, and PCs must be at least level 51 to cast
     this spell, so I'm just gonna make it a straight random chance */

  if (IS_PC(ch) && (!number(0, 12)) /*(GET_LEVEL(mob) > number((level - i * 2) * 2, level *
                                           3)) */ )
  {
    act("$N is NOT pleased at being returned to life!", TRUE, ch, 0, mob,
        TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
  }
  else
  {                             /* Under control */
    act("&+W$N roars to the sky 'I LIVE!!!'", TRUE, ch, 0, mob, TO_ROOM);
    act("&+W$N roars to the sky 'I LIVE!!!'", TRUE, ch, 0, mob, TO_CHAR);

    int duration = setup_pet(mob, ch, timeToDecay/2 + (6000 / STAT_INDEX(GET_C_INT(mob))), PET_NOCASH);
    add_follower(mob, ch);
  /* if the undead will stop being charmed after a bit, also make it suicide 1-10 minutes later */
    if (duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

void spell_create_greater_dracolich(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_char   mob;
  bool     corpselog;
  P_obj    obj_in_corpse = NULL, next_obj = NULL;
  P_obj    globe;
  struct follow_type *k;
  int      i, sum, num_undead = 0;
  int      life;
  static struct
  {
    const int mob_number;
    const char *message;
  } summons[] =
  {
    {
    7, "The corpse summons a greater &+ybronze&n dracolich!"},
    {
    8, "The corpse summons a greater &+csilver&n dracolich!"},
    {
    9, "The corpse summons a greater &+Ygold&n dracolich!"},
    {
    10, "The corpse summons a greater &+Lshadow&n dracolich!"}
  };

  if( !IS_ALIVE(ch) )
  {
    logit(LOG_DEBUG, "spell_create_greater_dracolich: Missing or dead char.");
    debug( "spell_create_greater_dracolich: Missing or dead char '%s'.", ch ? J_NAME(ch) : "Null" );
    return;
  }
  if( IS_NPC(ch) && IS_PC_PET(ch) )
  {
    send_to_char( "Your pet can not summon pets.\n\r", get_linked_char(ch, LNK_PET) );
    return;
  }
  if( !obj )
  {
    logit(LOG_DEBUG, "spell_create_greater_dracolich: Missing obj.");
    debug( "spell_create_greater_dracolich: Missing obj." );
    return;
  }
  if( IS_IMMOBILE(ch) )
  {
    send_to_char("But you can barely move!\r\n", ch);
    return;
  }

  if( IS_NPC(ch) && affected_by_spell(ch, TAG_CONJURED_PET) )
  {
    return;
  }

  // For testing.
  if( IS_PC(ch) && level > 56 )
  {
    level = 56;
  }

  if( CHAR_IN_SAFE_ZONE(ch) )
  {
    send_to_char("A mysterious force blocks your spell!\r\n", ch);
    return;
  }
  if( obj->type != ITEM_CORPSE )
  {
    act("You can't animate $p!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if( obj->value[CORPSE_LEVEL] < 51 )
  {
    send_to_char("This spell requires the corpse of a more powerful being!\r\n", ch);
    return;
  }
  /*
  if(count_undead(ch) > 0)
  {
    send_to_char
      ("You cannot have undead under your control when summoning dracoliches!\r\n",
       ch);
    return;
  }
  */
  if( (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) && !is_wearing_necroplasm(ch) &&
    !IS_NPC(ch)) || affected_by_spell(ch, SPELL_CORPSEFORM))
  {
    send_to_char("You cannot control undead in that form!\n\r", ch);
    return;
  }

  if( !can_raise_draco(ch, level, true) )
  {
    send_to_char("You cannot control any more greater dracoliches!\r\n", ch);
    return;
  }

  if( obj->loc.room != ch->in_room )
  {
    send_to_char("You cannot raise a corpse in another room\r\n", ch);
    return;
  }

  life = GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE);
  globe = get_globe( ch );

  if( globe )
  {
    act("&+LYour $q &+Lpulses with evil energy, infusing part of it's malevolence into your undead abomination!&N",
          FALSE, ch, globe, 0, TO_CHAR);
    act("&+L$n's $q &+Lthrobs with evil energy, combining its unholy power with $s abomination!&N",
          FALSE, ch, globe, 0, TO_ROOM);
    life += GET_LEVEL(ch) * 2;
  }

  sum = number(0, 3);
  mob = read_mobile(real_mobile(summons[sum].mob_number), REAL);

  if( !mob )
  {
    logit(LOG_DEBUG, "spell_create_greater_dracolich(): mob %d not loadable", summons[sum].mob_number);
    send_to_char("Bug in create greater  dracolich. Tell a god!\r\n", ch);
    return;
  }

  GET_SIZE(mob) = SIZE_GIANT;
  GET_RACE(mob) = RACE_DRACOLICH;
  SET_BIT(mob->specials.act, ACT_MOUNT);
  mob->specials.act |= ACT_SPEC_DIE;
  GET_HIT(mob) = GET_MAX_HIT(mob) = mob->points.base_hit = 2500 + number(-10, 50) + (life * 5);
  //REMOVE_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
  SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);

  if( IS_NPC(ch) && !IS_PC_PET(ch) )
  {
    if( !IS_SET(mob->only.npc->aggro_flags, AGGR_ALL) )
    {
      SET_BIT(mob->only.npc->aggro_flags, AGGR_ALL);
    }
/*    if( IS_SET(mob->specials.act, ACT_SENTINEL) )
    {
      REMOVE_BIT(mob->specials.act, ACT_SENTINEL);
    } */
    if( !SET_BIT(mob->specials.act, ACT_HUNTER) )
    {
      SET_BIT(mob->specials.act, ACT_HUNTER);
    }
    if( SET_BIT(mob->specials.act, ACT_GUILD_GOLEM) )
    {
      REMOVE_BIT(mob->specials.act, ACT_GUILD_GOLEM);
    }
    if( GET_LEVEL(ch) >= 54 )
    {
      if(mob->points.damnodice < 10)
      {
        mob->points.damnodice = 10;
      }
      mob->points.base_hitroll = mob->points.hitroll = (GET_LEVEL(ch));
    }
  }
  else
  {
    // 6d7 at level 56.
    mob->points.damnodice = (level + 9)/10;
    mob->points.damsizedice = (level + 4)/10 + 1;
    // 80 - 100 (actual 20 - 25) at level 56.
    mob->points.base_damroll = mob->points.damroll = level + number( 24, 44 );
  }

  if( GET_C_STR(mob) < 95 || GET_C_DEX(mob) < 95 )
  {
    mob->base_stats.Str = 100;
    mob->base_stats.Dex = 100;
  }

  char_to_room(mob, ch->in_room, 0);

  if( IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE) )
  {
    logit(LOG_CORPSE, "%s got raised as greater dracolich while equipped ( by %s in room %d ).",
      obj->short_description, (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr), world[ch->in_room].number);
    wizlog(57, "%s got raised as greater dracolich while equipped ( by %s in room %d ).",
      obj->short_description, (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr), world[ch->in_room].number);
    corpselog = TRUE;
  }

  if( obj->contains )
  {
    create_saved_corpse(obj, mob);
    for( obj_in_corpse = obj->contains; obj_in_corpse; obj_in_corpse = next_obj )
    {
      if( corpselog )
      {
        logit(LOG_CORPSE, "%s raised with eq: [%d] %s", obj->short_description,
          obj_index[obj_in_corpse->R_num].virtual_number, obj_in_corpse->name);
      }
      next_obj = obj_in_corpse->next_content;
      obj_from_obj(obj_in_corpse);
      obj_to_char(obj_in_corpse, mob);
    }
  }
  int timeToDecay = 0;
  struct obj_affect *afDecay = get_obj_affect(obj, TAG_OBJ_DECAY);
  if (NULL !=  afDecay)
  {
     // WAIT_SEC pulses in a sec, 60 secs in a minute
    timeToDecay = obj_affect_time(obj, afDecay) / (60 * WAIT_SEC);
  }
  extract_obj(obj);

  remove_plushit_bits(mob);

  act(summons[sum].message, TRUE, mob, 0, 0, TO_ROOM);

  balance_affects(mob);

  /* all dracoliches are around level 52, and PCs must be at least level 51 to cast
     this spell, so I'm just gonna make it a straight random chance */
  // (GET_LEVEL(mob) > number((level - i * 2) * 2, level * 3))
  if( IS_PC(ch) && !number(0, 9) && !globe )
  {
    act("$N is NOT pleased at being returned to life!", TRUE, ch, 0, mob, TO_ROOM);
    act("$N is NOT pleased with you at all!", TRUE, ch, 0, mob, TO_CHAR);
    // Poof in 5-10 sec.
    add_event(event_pet_death, (4 + number(1,6)) * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    MobStartFight(mob, ch);
  }
  else
  {
    act("&+W$N roars to the sky 'I LIVE!!!'", TRUE, ch, 0, mob, TO_ROOM);
    act("&+W$N roars to the sky 'I LIVE!!!'", TRUE, ch, 0, mob, TO_CHAR);

    GET_AC(mob) -= (level * 4);
    int duration = setup_pet(mob, ch, timeToDecay/2 + (6000 / STAT_INDEX(GET_C_INT(mob))), PET_NOCASH);
    add_follower(mob, ch);
    // if the undead will stop being charmed after a bit, also make it suicide 1 minute later
    if( duration >= 0 )
    {
      add_event(event_pet_death, (duration+1) * 60 * WAIT_SEC, mob, NULL, NULL, 0, NULL, 0);
    }
  }
}

void do_exhume(P_char ch, char *argument, int cmd)
{
  int      now;
  P_obj    obj1 = NULL, obj2 = NULL;
  bool     found_something = FALSE, have_one = FALSE;

  if ((GET_CHAR_SKILL(ch, SKILL_EXHUME) == 0) && !IS_TRUSTED(ch))
  {
    send_to_char("You don't know how to exhume.\n", ch);
    return;
  }

  obj1 = ch->equipment[HOLD];
  obj2 = ch->equipment[WIELD];

  if (!IS_TRUSTED(ch))
  {
    if (obj1 && ((isname("shovel", obj1->name) || isname("hoe", obj1->name) || isname("pick", obj1->name))))
        have_one = TRUE;
    else if (obj2 && ((isname("shovel", obj2->name) || isname("hoe", obj2->name) || isname("pick", obj2->name))))
        have_one = TRUE;

    if (!have_one)
    {
      send_to_char("Using what? Your fingers?\r\n", ch);
      return;
    }

    if( world[ch->in_room].sector_type != SECT_FIELD &&
        world[ch->in_room].sector_type != SECT_FOREST &&
        world[ch->in_room].sector_type != SECT_HILLS &&
        world[ch->in_room].sector_type != SECT_MOUNTAIN && 
        world[ch->in_room].sector_type != SECT_UNDRWLD_WILD &&
        world[ch->in_room].sector_type != SECT_UNDRWLD_MOUNTAIN &&
        world[ch->in_room].sector_type != SECT_SNOWY_FOREST)
    {
      send_to_char("This doesn't appear to be the best place for digging.\r\n", ch);
      return;
    }
    
    if (ch->specials.z_cord < 0 || ch->specials.z_cord > 0)
    {
      send_to_char("This doesn't appears to be the best place for digging.\r\n", ch);
      return;
    }

    if (!affect_timer(ch, get_property("timer.mins.exhume", 10) * WAIT_MIN, SKILL_EXHUME))
    {
      send_to_char("Your weak arms need more rest.\r\n", ch);
      return;
    }
  }
  // on success

  send_to_char
    ("Your shovel breaks through the &+yearth&n, revealing a &+Lrotten corpse&n ready for use.\r\n",
     ch);

  /* charWait for exhume is skill dependent:  between 14 and 24 (with base skill of 40, its actually between 14-20) */
  CharWait(ch, 24 - (GET_CHAR_SKILL(ch, SKILL_EXHUME) /10));

  obj1 = read_object(2, VIRTUAL);
  obj1->value[CORPSE_LEVEL] = MIN(50, GET_LEVEL(ch)+number(0,8)-4);

  obj1->action_description = str_dup("&+La cadaver&n");
  obj1->str_mask = STRUNG_DESC3;
  obj_to_room(obj1, ch->in_room);
  set_obj_affected(obj1, 0, TAG_OBJ_DECAY, 0);
  spell_embalm(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, NULL, obj1);
}

void do_spawn(P_char ch, char *argument, int cmd)
{
  struct affected_type af;

  if (!ch)
    return;

  if (!has_innate(ch, INNATE_SPAWN) && !has_innate(ch, INNATE_ALLY))
  {
    send_to_char("You don't know how!\n", ch);
    return;
  }

  if (affected_by_spell(ch, TAG_SPAWN))
  {
    send_to_char("You no longer wish to summon pets from death blows.\n", ch);
    affect_from_char(ch, TAG_SPAWN);
    return;
  }
  
  send_to_char("You will try to summon pets from death blows.\n", ch);
  memset(&af, 0, sizeof(af));
  af.type = TAG_SPAWN;
  af.duration = -1;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
  affect_to_char(ch, &af);
}

void do_summon_host(P_char ch, char *argument, int cmd)
{
  int      now;
  P_obj    obj1 = NULL, obj2 = NULL;
  bool     found_something = FALSE, have_one = FALSE;

  if (!has_innate(ch, INNATE_SUMMON_HOST) && !IS_TRUSTED(ch))
  {
    send_to_char("You don't know how to summon a host body.\n", ch);
    return;
  }

  // removing this until we figure out a better thematic object...
  /*
  obj1 = ch->equipment[HOLD];
  obj2 = ch->equipment[WIELD];
  */
  if (!IS_TRUSTED(ch))
  {
    /*
    if (obj1 && ((isname("shovel", obj1->name) || isname("hoe", obj1->name) || isname("pick", obj1->name))))
        have_one = TRUE;
    else if (obj2 && ((isname("shovel", obj2->name) || isname("hoe", obj2->name) || isname("pick", obj2->name))))
        have_one = TRUE;

    if (!have_one)
    {
      send_to_char("Using what? Your fingers?\r\n", ch);
      return;
    }
    */

    /*
    if( world[ch->in_room].sector_type != SECT_FIELD &&
        world[ch->in_room].sector_type != SECT_FOREST &&
        world[ch->in_room].sector_type != SECT_HILLS &&
        world[ch->in_room].sector_type != SECT_MOUNTAIN && 
        world[ch->in_room].sector_type != SECT_UNDRWLD_WILD &&
        world[ch->in_room].sector_type != SECT_UNDRWLD_MOUNTAIN &&
        world[ch->in_room].sector_type != SECT_SNOWY_FOREST)
    {
      send_to_char("This doesn't appear to be the best place for digging.\r\n", ch);
      return;
    }
    
    if (ch->specials.z_cord < 0 || ch->specials.z_cord > 0)
    {
      send_to_char("This doesn't appears to be the best place for digging.\r\n", ch);
      return;
    }
    */
    if (!affect_timer(ch, get_property("timer.mins.exhume", 10) * WAIT_MIN, SKILL_EXHUME))
    {
      send_to_char("Your spirit is too weak to summon a new host at this time.\r\n", ch);
      return;
    }
  }
  // on success

  send_to_char
    ("A &+Wholy be&+Ra&+Wm&n of &+Ylight&n appears before you.  As the &+Ylight &+Wdis&+wsip&+Lates&n, a &+Wsoul&+wless &+Lcorpse&n remains.\r\n",
     ch);

  /* charWait for exhume is skill dependent:  between 14 and 24 (with base skill of 40, its actually between 14-20) */
  CharWait(ch, 24 - (GET_CHAR_SKILL(ch, SKILL_EXHUME) /10));

  obj1 = read_object(2, VIRTUAL);
  obj1->value[CORPSE_LEVEL] = MIN(50, GET_LEVEL(ch)+number(0,8)-4);

  obj1->action_description = str_dup("&+La cadaver&n");
  obj1->str_mask = STRUNG_DESC3;
  obj_to_room(obj1, ch->in_room);
  set_obj_affected(obj1, 0, TAG_OBJ_DECAY, 0);
  spell_embalm(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, NULL, obj1);
}

void do_remort(P_char ch, char *arg, int cmd) 
{

  if( !IS_PC(ch) )
    return;

  if( !affected_by_spell(ch, SPELL_VAMPIRE) &&
      !affected_by_spell(ch, SPELL_ANGELIC_COUNTENANCE)) {
    send_to_char("But you're still alive!\r\n", ch);
    return;
  }

  if( !has_innate(ch, INNATE_REMORT) )
  {
    send_to_char("You don't know how to return to life so quickly!\r\n", ch);
    return;
  }

  send_to_char("Color fades slowly into your face.\r\n", ch);
  affect_from_char(ch, SPELL_VAMPIRE);
  affect_from_char(ch, SPELL_ANGELIC_COUNTENANCE);
  CharWait(ch, PULSE_VIOLENCE * 3);
}

void spell_corpseform(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool     loss_flag = FALSE;
  int      chance, l, found, clevel, ss_save, ss_roll, the_size;
  long     resu_exp;
  P_obj    obj_in_corpse, next_obj, t_obj, money;
  struct affected_type *af, *afcf, *afrc, *next_af;
  P_char   t_ch, folpet, target = NULL;
  char     tbuf[MAX_STRING_LENGTH];
  struct follow_type *foll, *next_foll;
  
  if (affected_by_spell(ch, SPELL_CORPSEFORM))
  {
    send_to_char("You're already surrounded by a necromatic essence.\r\n", ch);
    return;
  }
  
  if( !ch || !obj )
    return;
  
  if (obj->type != ITEM_CORPSE)
  {
    send_to_char("That isn't very corpse-like!\n", ch);
    return;
  }
  
  int corpse_race = obj->value[CORPSE_RACE];
  
  if (corpse_race <= RACE_NONE || corpse_race > LAST_RACE )
  {
    send_to_char("That corpse is not usable.\n", ch);
    return;    
  }

  /* only perform the checks for non gods */
  
  if (!IS_TRUSTED(ch))
  {
    if ( IS_SET(obj->value[CORPSE_FLAGS], NPC_CORPSE) )
    {
      send_to_char("You can't summon the power to assume that shape.\n", ch);
      return;    
    }  
    
    if (isCarved(obj))
    {
      send_to_char("This corpse is too mutilated.\n", ch);
      return;
    }
    
    if (corpse_race == RACE_ILLITHID ||
        corpse_race == RACE_CENTAUR ||
        corpse_race == RACE_THRIKREEN ||
        corpse_race == RACE_MINOTAUR) 
    {
      send_to_char("That corpse is too weird for you to assume its form!\n", ch);
      return;
    }
    
    if ( GET_RACE(ch) == corpse_race )
    {
      send_to_char("Huh? That corpse is the same race as you!\r\n", ch);
      return;      
    }
    
    if ( IS_PC(ch) && obj->value[CORPSE_PID] == GET_PID(ch) )
    {
      send_to_char("You cannot use your own corpse!\n", ch);
      return;
    }
    
    if (IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE) && obj->value[CORPSE_LEVEL] > 56 )
    {
      send_to_char("Divine corpses are sacrosanct!\n", ch);
      return;
    }
    
    if ( GET_ALT_SIZE(ch) < (race_size(corpse_race) - 1) )
    {
      send_to_char("You're just too small!\r\n", ch);
      return;      
    }

    if ( GET_ALT_SIZE(ch) > (race_size(corpse_race) + 1) )
    {
      send_to_char("You're just too big!\r\n", ch);
      return;      
    }
    
    if ( obj->value[CORPSE_LEVEL] < ( GET_LEVEL(ch) - 10) )
    {
      send_to_char("That corpse is not powerful enough for you to assume its essence.\r\n", ch);
      return;      
    }
    
  }
  
  struct affected_type new_affect;
  afrc = &new_affect;
  afcf = &new_affect;
  memset(afrc, 0, sizeof(new_affect));
  memset(afcf, 0, sizeof(new_affect));

  act("&+LThe necromatic essence of $o swirls up and forms around $n.&n",
      TRUE, ch, obj, NULL, TO_ROOM);
  act("&+LThe necromatic essence of $o swirls up and forms around you.&n",
      TRUE, ch, obj, NULL, TO_CHAR);
  
  if (number(1, 100) < (int) get_property("spell.corpseform.improvedChance", 10))
  {
    // Superior corpseform, get innates of new race
    afcf->modifier = CORPSEFORM_INNATE;
    //IS_DISGUISE_SHAPE(ch) = FALSE;
    act("&+LThe dark mist &+wPULSES&+L for a moment before reforming around $n.", TRUE, ch, obj, NULL, TO_ROOM);
    act("&+LThe dark mist &+wPULSES&+L for a moment before reforming around you.", TRUE, ch, obj, NULL, TO_CHAR);
    
  }
  else
  {
    // Normal corpseform, keep old innates
    //IS_DISGUISE_SHAPE(ch) = TRUE;
    afcf->modifier = CORPSEFORM_REG;
  }
  
  afcf->type = SPELL_CORPSEFORM;
  afcf->flags = AFFTYPE_NODISPEL;
  afcf->duration = (int) get_property("spell.corpseform.duration.mins", 20);
  affect_to_char(ch, afcf);
  
  afrc->type = TAG_RACE_CHANGE;
  afrc->flags = AFFTYPE_NODISPEL | AFFTYPE_NOSAVE;
  afrc->modifier = GET_RACE(ch);
  afrc->duration = -1;
  affect_to_char(ch, afrc);
  
  add_event(event_corpseform_wearoff, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
  
  act("&+LThe corpse of $o crumbles to dust as its essence is completely drained.&n", TRUE, ch, obj, NULL, TO_ROOM);
  act("&+LThe corpse of $o crumbles to dust as its essence is completely drained by you.&n", TRUE, ch, obj, NULL, TO_CHAR);
  
  sprintf(tbuf, "You assume the form of %s!", race_names_table[corpse_race].normal);
  act(tbuf, FALSE, ch, NULL, NULL, TO_CHAR);

  sprintf(tbuf, "$n assumes the form of %s!", race_names_table[corpse_race].normal);
  act(tbuf, FALSE, ch, NULL, NULL, TO_ROOM);

  GET_RACE(ch) = corpse_race;

  // Drop pets if he has any
  for (foll = ch->followers; foll; foll = next_foll)
  {
    next_foll = foll->next;
    folpet = foll->follower;

    if (IS_PC_PET(folpet))
    {
      stop_follower(folpet);
      setup_pet(folpet, ch, 2, PET_NOORDER);
    }
  }

  if (obj->contains)
  {
    for (obj_in_corpse = obj->contains; obj_in_corpse;
         obj_in_corpse = next_obj)
    {
      next_obj = obj_in_corpse->next_content;
      obj_from_obj(obj_in_corpse);
      obj_to_room(obj_in_corpse, ch->in_room);
    }
  }

  extract_obj(obj);
}

void event_corpseform_wearoff(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *afrc;

  if ((afrc = get_spell_from_char(ch, TAG_RACE_CHANGE)) != NULL)
  {
    if (!affected_by_spell(ch, SPELL_CORPSEFORM))
    {
      GET_RACE(ch) = afrc->modifier;
      affect_remove(ch, afrc);
      send_to_char("&+LThe necromatic aura dissipates and your true form is revealed.&n\r\n", ch);
      act("&+LThe necromatic aura dissipates and reveals the true form of $n&n", TRUE, ch, 0, NULL, TO_ROOM);
      return;
    }
    else
    {
      add_event(event_corpseform_wearoff, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
      return;
    }
  }
}

void spell_slashing_darkness(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "&+LThe shadowy hand slashes $N &+Lviciously.",
    "&+LYou stagger as the shadowy hand conjured by $n &+Lslashes you with its sharp claws.",
    "&+LThe shadowy hand slashes at $N&+L, who staggers under the blow.",
    "&+LThe shadowy hand tears away the remaining life of $N.",
    "&+LA clawed darkness rushing at you is the last thing you before everything goes dark...",
    "&+LThe shadowy hand tears away the remaining life of $N&+L, causing $m to stagger and collapse."
  };

  if (resists_spell(ch, victim))
    return;

  int dam;
  int num_missiles = BOUNDED(1, (level / 3), 5);

  dam = (dice(1, 4) * 4 + number(1, level))*num_missiles;

  act("&+LYou utter a word of dark speech and a shadowy hand coalesces near $N!&n", FALSE, ch, 0, victim, TO_CHAR);
  act("&+L$n utters a word of dark speech, and a shadowy hand coalesces near $N!&n", FALSE, ch, 0, victim, TO_NOTVICT);
  act("&+L$n utters a word of dark speech, and a shadowy hand coalesces near YOU!&n", FALSE, ch, 0, victim, TO_VICT);

  spell_damage(ch, victim, (dam/2), SPLDAM_GENERIC, SPLDAM_ALLGLOBES | RAWDAM_NOKILL | SPLDAM_NOSHRUG, 0);

  if (IS_ALIVE(ch) && IS_ALIVE(victim))
    spell_damage(ch, victim, (dam/2), SPLDAM_NEGATIVE, SPLDAM_ALLGLOBES | SPLDAM_NOSHRUG, &messages);

}

void spell_undead_to_death(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{

  int save, dam, hits;
  struct affected_type af;

  struct damage_messages messages = {
    "&+GA strange magical aura forms around $N.",
    "&+GA strange magical aura forms around you causing immense &+Rpain!!!",
    "&+GA strange magical aura forms around $N.",
    "$N's skin gains a little color.. then crumbles to &+Ldust!!!&n",
    "&+WYou feel a slight &+ywarmth &+Wfor a brief moment, &+Lbefore embracing oblivion!!!&n",
    "$N's skin gains a little color.. then crumbles to &+Ldust!!!&n",
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if( !IS_UNDEADRACE(victim) )
  {
    send_to_char("They aren't even undead!", ch);
    return;
  }

  if( IS_GREATER_RACE(ch) || IS_ELITE(ch) || IS_PC_PET(victim) )
  {
    save = 5;
  }
  else
  {
    save = BOUNDED(-10, GET_LEVEL(ch) - GET_LEVEL(victim), 10);
  }

  if( level > 56 )
  {
    level = 56;
  }
  // Note: For destroy undead (5th circle) dam = 15*lvl +/- 40.
  // (16 * 56 = 896) / 4 => 224 +/- 10.
  dam = (16 * level) + number(-40, 40);

  if( IS_AFFECTED2(victim, AFF2_SOULSHIELD) )
  {
    dam = (int) (dam * 0.50);
    save -= 2;
  }

  if( !NewSaves(victim, SAVING_SPELL, save) )
  {
    spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_NOSHRUG, &messages);
  }
  else if( !number(0, 3) )
  {
    spell_damage(ch, victim, (int) (dam * 0.75), SPLDAM_HOLY, SPLDAM_NOSHRUG, &messages);
  }
  else if( GET_HIT(victim) > GET_MAX_HIT(victim) )
  {
    act("&+LYour undeath reserves were neutralized!&n", FALSE, ch, 0, victim, TO_VICT);
    act("&+GThe green mist surrounding&n $n &+Gexplodes!&n", TRUE, victim, 0, 0, TO_ROOM);

    hits = GET_HIT(victim) - GET_MAX_HIT(victim);
    GET_HIT(victim) -= hits;
  }
  else if( !NewSaves(victim, SAVING_PARA, save)
    && !affected_by_spell(victim, SPELL_UNDEAD_TO_DEATH) )
  {
    act("&+LSomething is very, very wrong. Your undead bindings were greatly weakened!&n",
      FALSE, ch, 0, victim, TO_VICT);
    act("$n wavers and almost &+ycollapses!!!&n", TRUE, victim, 0, 0, TO_ROOM);

    bzero(&af, sizeof(af));
    af.type = SPELL_UNDEAD_TO_DEATH;

    af.duration = number(1, 2);

    af.location = APPLY_STR;
    af.modifier = (int) (-1 * dice(3, 10));
    affect_to_char(victim, &af);

    af.location = APPLY_CON;
    af.modifier = (int) (-1 * dice(3, 20));
    affect_to_char(victim, &af);

    af.location = APPLY_AGI;
    af.modifier = (int) (-1 * dice(3, 10));
    affect_to_char(victim, &af);

    af.location = APPLY_DEX;
    af.modifier = (int) (-1 * dice(3, 10));
    affect_to_char(victim, &af);
  }
  else
  {
    spell_damage(ch, victim, (int) (dam * .67), SPLDAM_HOLY, SPLDAM_NOSHRUG, &messages);
  }
}

void spell_taint(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      dam;
  struct damage_messages messages = {
    "Your &+rpowerful voice&n causes $N's &+Cs&nou&+Cl&n to distort and shimmer, writhing in agony!",
    "Your divine spirit writhes in agony at the power of $n's &+rtainted&n word!",
    "$n utters a &+rwicked&n word, causing $N to writhe in agony, echoes of $s &+Cs&nou&+Cl&n shimmering around $s body!",
    "Your eyes sparkle with an insane glee as the &+rabsolute power&n of your word destroys that which was $N!",
    "With a &+Gw&+gr&+Letch&+ge&+Gd &+mincantation&n $n opens $s mouth wider than $s skull and swallos your &+Cs&nou&+Cl&n.",
    "With a &+Gw&+gr&+Letch&+ge&+Gd &+mincantation&n, $n opens $s mouth wider than $s skull, swallowing the &+Cs&nou&+Cl&n of $N!", 0
  };

  if(!ch)
  {
    logit(LOG_EXIT, "spell_taint called in magic.c with no ch");
    raise(SIGSEGV);
    return;
  }

  if(ch &&
    victim &&
    !IS_ANGEL(victim))
  {
    send_to_char("Your victim cannot be tainted!\n", ch);
    return;
  }

  dam = 9 * MIN(level, 56) + number(-40, 40);
  
  if(IS_AFFECTED2(victim, AFF2_SOULSHIELD))
  {
    dam *= (int) .5;
  }

  if(saves_spell(victim, SAVING_SPELL))
  {
    dam >>= 1;
  }

  spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);
}

/*
void spell_eyes_death(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if( !IS_ALIVE(ch) )
  {
    return;
  }
  
  if(victim != ch)
  {
    victim = ch;
  }
  
  if(affected_by_spell(ch, SPELL_EYES_DEATH))
  {
    send_to_char(ch, "Your eyes are already attuned to the lands of death and decay and the realms beyond mortality.\r\n");
    return;
  }

  if(affected_by_spell(ch, SPELL_EYES_DEATH))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_EYES_DEATH)
      {
        af1->duration = MAX(level / 2, 10);
      }
    return;
  }
  
  bzero(&af, sizeof(af));
  af.type = SPELL_EYES_DEATH;
  af.duration = MAX(level / 2, 5);
  af.bitvector = AFF_SENSE_LIFE;
  af.bitvector = AFF_DETECT_INVISIBLE;
  af.bitvector4 = AFF4_DETECT_ILLUSION;
  af.bitvector2 = AFF2_DETECT_MAGIC;
  af.bitvector = AFF_AWARE;
  
  affect_to_char(victim, &af);
}
*/

// Checks both hands for artifact globe of shadows.  If found, returns 
//   a pointer to globe.  If not, returns NULL.
P_obj get_globe( P_char ch )
{
  P_obj globe;

  // Check left hand.
  globe = ch->equipment[HOLD];
  if( !globe || !(obj_index[globe->R_num].virtual_number == GLOBE_SHADOWS_VNUM) )
  {
    // If not left, check right hand.
    globe = ch->equipment[WIELD];
    if( !globe || !(obj_index[globe->R_num].virtual_number == GLOBE_SHADOWS_VNUM) )
    {
      globe = NULL;
    }
  }
  return globe;
}
