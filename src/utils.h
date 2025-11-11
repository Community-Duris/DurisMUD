/**************************************************************************
 *  file: utils.h, Utility module.                                         *
 *  Usage: Utility macros                                                  *
 ************************************************************************* */

#ifndef _SOJ_UTILS_H_
#define _SOJ_UTILS_H_

#ifdef __CYGWIN_BUILD__
#include <crypt.h>
#endif

#include <signal.h>

#include "config.h"

#define str_cmp(a,b) ((!(a) && !(b)) ? 0 : \
                      !(a) ? -1 : \
                      !(b) ? 1 : \
                      strcasecmp((a), (b)))

#define LOWER(c) (((c) >= 'A' && (c) <= 'Z')? ((c) + ('a' - 'A')): (c))

#define UPPER(c) (((c) >= 'a' && (c) <= 'z')? ((c) + ('A' - 'a')): (c))

/* Functions in utility.c                     */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#if 0
#define BOUNDED(a, b, c) (MIN( MAX (a, b), c ))
#endif

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

#define IF_STR(st) ((st) ? (st) : "\0")

#define GET_TIME_JUDGE(ch) ((ch)->specials.time_judge)

/*
#define CAP(st)  (*(st) = UPPER(*(st)), st)
*/

#define CLS(ch) (IS_ANSI_TERM(ch) ? "\033[H\033[J" : "")

//#ifndef MEMCHK
//#
//#define CREATE(result, type, number, tag)
//  if (!((result) = (type *) calloc ((number), sizeof(type))))
//    { perror("calloc failure"); raise(SIGSEGV); }
//
//#define RECREATE(result, type, number)
//  if (!((result) = (type *) realloc ((char *)(result), sizeof(type) * (number))))
//    { perror("realloc failure"); raise(SIGSEGV); }
//
//#define FREE(i)    free(i)
//
//#else  /* memory debug option.  See memory.c */

#define CREATE(result, type, num, tag)                                                      \
  {                                                                                         \
    if (!((result) = (type *) __malloc(sizeof(type) * (num), (tag), __FILE__, __LINE__)))   \
    {                                                                                       \
      logit(LOG_EXIT, "__malloc returned null pointer");                                    \
      raise(SIGSEGV);                                                                       \
    }                                                                                       \
  }
//" This is here to clean up the coloring in nano.  Something wrong with the backslash and quotes causes bleeding

#define RECREATE(result, type, num)                                                                  \
  {                                                                                                  \
    if (!((result) = (type *) __realloc((void*)(result), sizeof(type) * (num), __FILE__, __LINE__))) \
    {                                                                                                \
      logit(LOG_EXIT, "__realloc returned null pointer");                                            \
      raise(SIGSEGV);                                                                                \
    }                                                                                                \
  }
//" This is here to clean up the coloring in nano.  Something wrong with the backslash and quotes causes bleeding

#define FREE(i)                    \
  {                                \
    __free(i, __FILE__, __LINE__); \
   (i) = NULL;                     \
  }

//#endif

#define IS_SET(flag, bit)  ((flag) & (bit))

#define SWITCH(a, b) { (a) ^= (b); (b) ^= (a); (a) ^= (b); }

/*
#define CAN_CMD_PARALYSIS(cmd) ((cmd) == 8 || ((cmd) >= 15 && (cmd) <= 21) || (cmd) == 41 || \
                                (cmd) == 72 || (cmd) == 76 || (cmd) == 80 || (cmd) == 81 || \
                                (cmd) == 82 || (cmd) == 87 || (cmd) == 210 || (cmd) == 212 || \
                                (cmd) == 213 || (cmd) == 214 || (cmd) == 222 || (cmd) == 237 || \
                                (cmd) == 246 || (cmd) == 241 || (cmd) == 262 || (cmd) == 549)
*/

#define CAN_CMD_PARALYSIS(cmd) (((cmd) == CMD_LOOK) || \
        ((cmd) == CMD_SCORE) || ((cmd) == CMD_QUI) || \
        ((cmd) == CMD_TIME) || ((cmd) == CMD_IDEA) || \
        ((cmd) == CMD_TYPO) || ((cmd) == CMD_BUG) || \
        ((cmd) == CMD_CREDITS) || ((cmd) == CMD_GLANCE) || \
        ((cmd) == CMD_RULES) || ((cmd) == CMD_WILL) || \
        ((cmd) == CMD_PETITION))

#define IS_BEING_SHADOWED(ch) ((ch)->specials.shadow.who != NULL)
#define IS_SHADOWING(ch) ((ch)->specials.shadow.shadowing != NULL)
#define IS_SHADOW_MOVE(ch) ((ch)->specials.shadow.shadow_move == TRUE)
#define GET_CHAR_SHADOWED(ch) ((ch)->specials.shadow.shadowing)

#define IS_GLOBED(target) (IS_AFFECTED2(target, AFF2_GLOBE) || IS_AFFECTED3(target, AFF3_GR_SPIRIT_WARD))
#define IS_MINGLOBED(ch) (IS_AFFECTED3(target, AFF3_SPIRIT_WARD) || IS_AFFECTED(target, AFF_MINOR_GLOBE))

#define IS_ELITE(mob) (IS_SET(mob->specials.act, ACT_ELITE))
#define IS_IMMATERIAL(mob) ((GET_RACE(mob) == RACE_WRAITH) || \
         (GET_RACE(mob) == RACE_GHOST) || \
         (GET_RACE(mob) == RACE_SHADE) || \
         (GET_RACE(mob) == RACE_A_ELEMENTAL) || \
         (GET_RACE(mob) == RACE_F_ELEMENTAL) || \
         (GET_RACE(mob) == RACE_PHANTOM) || \
         (GET_RACE(mob) == RACE_WIGHT) || \
         (GET_RACE(mob) == RACE_SHADOW) || \
         (GET_RACE(mob) == RACE_DEVA) || \
         (GET_RACE(mob) == RACE_SPECTRE) || \
         (GET_RACE(mob) == RACE_V_ELEMENTAL) || \
         (GET_RACE(mob) == RACE_BRALANI) || \
         IS_AFFECTED4(mob, AFF4_PHANTASMAL_FORM))

#define LEGLESS(mob) (IS_IMMATERIAL(mob) || IS_BEHOLDER(mob) || \
        (GET_RACE(mob) == RACE_SLIME) || \
        (GET_RACE(mob) == RACE_CONSTRUCT) || \
        (GET_RACE(mob) == RACE_SNAKE) || \
        (GET_RACE(mob) == RACE_INSECT) || \
        (GET_RACE(mob) == RACE_PARASITE) || \
        (GET_RACE(mob) == RACE_AQUATIC_ANIMAL) || \
        (GET_RACE(mob) == RACE_PLANT) || \
        (GET_RACE(mob) == RACE_EFREET) || \
        (GET_RACE(mob) == RACE_PWORM))

#define IS_PC_PET(mob) (IS_NPC(mob) && get_linked_char(mob, LNK_PET)\
                        && IS_PC(get_linked_char(mob, LNK_PET)))
#define GET_MASTER(ch) (get_linked_char(ch, LNK_PET))
#define GET_RIDER(ch) (get_linking_char(ch, LNK_RIDING))
#define GET_MOUNT(ch) (get_linked_char(ch, LNK_RIDING))

#define OBJ_POLYMORPH(o) ((o)->polymorphed_ch)
#define CHAR_POLYMORPH_OBJ(ch) ((ch)->specials.polymorphed_obj)
#define IS_GREATER_DRACO(ch) (IS_NPC(ch) && (GET_VNUM(ch) == 7 || GET_VNUM(ch) == 8 || GET_VNUM(ch) == 9 || GET_VNUM(ch) == 10))
#define IS_GREATER_AVATAR(ch) (IS_NPC(ch) && (GET_VNUM(ch) == 82 || GET_VNUM(ch) == 83 || GET_VNUM(ch) == 84 || GET_VNUM(ch) == 85))

#define OUTDOOR_SNEAK(ch)  \
(has_innate(ch, INNATE_OUTDOOR_SNEAK) && \
 (world[(ch)->in_room].sector_type >= 2) && (world[(ch)->in_room].sector_type <= 4))
#define UD_SNEAK(ch) (has_innate(ch, INNATE_UD_SNEAK) && IS_UNDERWORLD(ch->in_room))

#define SWAMP_SNEAK_TERRAIN(ch) \
 ( (world[(ch)->in_room].sector_type == SECT_SWAMP) || (world[(ch)->in_room].sector_type == SECT_UNDRWLD_SLIME) )

#define SWAMP_SNEAK(ch) (has_innate(ch, INNATE_SWAMP_SNEAK) && SWAMP_SNEAK_TERRAIN(ch))

#define GET_LANGUAGE(ch,d) ((ch)->only.pc->talks[(d)])
#define GET_PASSWORD(d) ((d)->character->pc_only->pwd)

#define SOUL_TAKING_STILETTO 88314

#define USES_FOCUS(ch)(GET_RACE(ch) == RACE_PSBEAST)

#define GET_SONG(ch) ((ch)->specials.song)

#define BAD_APPLYS(apply) (apply == APPLY_GOLD || apply == APPLY_CHAR_WEIGHT || apply == APPLY_CHAR_HEIGHT || \
                           apply == APPLY_EXP || apply == APPLY_CLASS || apply == APPLY_LEVEL || \
                           apply == APPLY_SEX || apply == APPLY_FIRE_PROT)

#define IS_FULLY_RESTED(ch, skill_type, now)  \
(((now) - (ch)->specials.skill_usage[(skill_type)].time_of_first_use) >= \
SECS_PER_MUD_DAY)

#define SKILL_HOURS_TO_REST(ch, skill_type, now)  \
(24 - (((now) - (ch)->specials.skill_usage[(skill_type)].time_of_first_use) * 24 / SECS_PER_MUD_DAY))

/*#define GET_CHAR_SKILL_P(ch, num) (IS_PC(ch)? (ch)->only.pc->skills[num].learned: 0)*/
#define IS_AFFECTED(ch, skill)  (IS_SET((ch)->specials.affected_by, (skill)))
#define IS_AFFECTED2(ch, skill) (IS_SET((ch)->specials.affected_by2, (skill)))
#define IS_AFFECTED3(ch, skill) (IS_SET((ch)->specials.affected_by3, (skill)))
#define IS_AFFECTED4(ch, skill) (IS_SET((ch)->specials.affected_by4, (skill)))
#define IS_AFFECTED5(ch, skill) (IS_SET((ch)->specials.affected_by5, (skill)))
#define IS_AFFECTED6(ch, skill) (IS_SET((ch)->specials.affected_by6, (skill)))
#define IS_ACT(ch, flags) (IS_SET((ch)->specials.act, (flags)))
#define IS_ACT2(ch, flags) (IS_SET((ch)->specials.act2, (flags)))

#define IS_ALIVE(ch) (ch && (GET_STAT(ch) > STAT_DEAD))

#define IS_AGGROFLAG(ch, flags) (IS_NPC(ch) && IS_SET((ch)->only.npc->aggro_flags, (flags)))
#define IS_AGGRO2FLAG(ch, flags) (IS_NPC(ch) && IS_SET((ch)->only.npc->aggro2_flags, (flags)))
#define IS_AGGRO3FLAG(ch, flags) (IS_NPC(ch) && IS_SET((ch)->only.npc->aggro3_flags, (flags)))

#define GET_CHAR_SKILL_S(ch, num) (GET_CHAR_SKILL_P((ch), num))
#define GET_CHAR_SKILL(ch, num)  (\
                IS_AFFECTED5(ch, AFF5_MENTAL_ANGUISH) ?  \
                  (MIN(GET_CHAR_SKILL_P((ch), (num)), 10)) : \
                  GET_CHAR_SKILL_P(ch, (num)))

#define IS_SPECIALIZED(ch) (ch->player.spec != 0)

#define IS_STUNNED(ch) (IS_SET((ch)->specials.affected_by2, AFF2_STUNNED))

#define IS_BLOODLUST (time_info.day == 4 && time_info.month == 16)

#define IS_WATERFORM(ch) ((GET_RACE(ch) == RACE_W_ELEMENTAL) || IS_AFFECTED2(ch, AFF2_WATER_AURA))

#define LIMITED_TELEPORT_ZONE(r) (world[r].number >= 5700 && world[r].number <= 5999)

#define SECTOR_TYPE(rroom) ( world[rroom].sector_type )

#define IS_HOMETOWN(r) ( zone_table[world[r].zone].hometown != 0 )

#define IS_SHIP_ROOM(r) ( (world[r].number >= VROOM_SHIPS_START) && (world[r].number <= VROOM_SHIPS_END) )

#define IS_OCEAN_ROOM(r) ( world[r].sector_type == SECT_OCEAN )
#define IS_SECT(room, sect) ( world[room].sector_type == sect )
#define IS_FOREST_ROOM(r) ( world[r].sector_type == SECT_FOREST || world[r].sector_type == SECT_SNOWY_FOREST)

#define HAS_VEGETATION(sect) ( sect == SECT_FIELD         || sect == SECT_FOREST           || sect == SECT_HILLS    \
                            || sect == SECT_MOUNTAIN      || sect == SECT_UNDRWLD_WILD     || sect == SECT_DESERT   \
                            || sect == SECT_ARCTIC        || sect == SECT_SWAMP            || sect == SECT_UNDRWLD_MOUNTAIN \
                            || sect == SECT_UNDRWLD_SLIME || sect == SECT_UNDRWLD_MUSHROOM || sect == SECT_SNOWY_FOREST \
                            || sect == SECT_UNDRWLD_WILD  || sect == SECT_UNDRWLD_LOWCEIL  || sect == SECT_CITY     \
                            || sect == SECT_UNDRWLD_CITY )
#define IS_SWAMP_ROOM(r) ( world[r].sector_type == SECT_SWAMP )
#define IS_CASTLE(r) ( world[r].sector_type == SECT_CASTLE_WALL || world[r].sector_type == SECT_CASTLE_GATE \
  || world[r].sector_type == SECT_CASTLE )

#define IS_UNDERWATER(c) ( (world[c->in_room].sector_type == SECT_UNDERWATER) \
  || (world[c->in_room].sector_type == SECT_UNDERWATER_GR)                    \
  || (world[c->in_room].sector_type == SECT_WATER_PLANE)                      \
  || (IS_WATER_ROOM(c->in_room) && c->specials.z_cord < 0)                    \
  || (IS_ROOM( ch->in_room, ROOM_UNDERWATER)) )

#define IS_WATER_ROOM(r) (world[r].sector_type == SECT_UNDRWLD_WATER \
    || world[r].sector_type == SECT_UNDRWLD_NOSWIM \
    || world[r].sector_type == SECT_OCEAN \
    || world[r].sector_type == SECT_UNDERWATER \
    || world[r].sector_type == SECT_UNDERWATER_GR \
    || world[r].sector_type == SECT_WATER_NOSWIM \
    || world[r].sector_type == SECT_WATER_SWIM \
    || world[r].sector_type == SECT_WATER_PLANE)

#define IS_UNDERWORLD(r) (world[r].sector_type == SECT_UNDRWLD_CITY \
    || world[r].sector_type == SECT_UNDRWLD_INSIDE \
    || world[r].sector_type == SECT_UNDRWLD_WILD \
    || world[r].sector_type == SECT_UNDRWLD_WATER \
    || world[r].sector_type == SECT_UNDRWLD_NOSWIM \
    || world[r].sector_type == SECT_UNDRWLD_NOGROUND \
    || world[r].sector_type == SECT_UNDRWLD_MOUNTAIN \
    || world[r].sector_type == SECT_UNDRWLD_SLIME \
    || world[r].sector_type == SECT_UNDRWLD_LOWCEIL \
    || world[r].sector_type == SECT_UNDRWLD_LIQMITH \
    || world[r].sector_type == SECT_UNDRWLD_MUSHROOM)

#define NORMAL_PLANE(r) (world[r].sector_type != SECT_FIREPLANE  \
    && world[r].sector_type != SECT_WATER_PLANE \
    && world[r].sector_type != SECT_EARTH_PLANE \
    && world[r].sector_type != SECT_AIR_PLANE \
    && world[r].sector_type != SECT_ETHEREAL \
    && world[r].sector_type != SECT_ASTRAL \
    && world[r].sector_type != SECT_NEG_PLANE)

#define IS_LIGHT(r) ( (world[r].light > 0 && !IS_ROOM( r, ROOM_MAGIC_DARK)) \
                    || IS_SUNLIT(r) || IS_TWILIGHT_ROOM(r) || IS_ROOM(r, ROOM_MAGIC_LIGHT) )

#define CAN_DAYPEOPLE_SEE(r) ( world[r].light > 0 || IS_SUNLIT(r) || IS_TWILIGHT_ROOM(r) \
                             || IS_ROOM(r, ROOM_MAGIC_LIGHT) )

// This isn't simply !LIGHT, because twilight counts as both, and so does dark + lights in room.
#define CAN_NIGHTPEOPLE_SEE(r) ( IS_TWILIGHT_ROOM(r) || (!IS_SUNLIT(r) && !IS_ROOM(r, ROOM_MAGIC_LIGHT)) )

#define IS_MAGIC_LIGHT(r) ( IS_ROOM( r, ROOM_MAGIC_LIGHT) && !IS_ROOM( r, ROOM_MAGIC_DARK ) )
#define IS_MAGIC_DARK(r) ( IS_ROOM( r, ROOM_MAGIC_DARK) && !IS_ROOM( r, ROOM_MAGIC_LIGHT ) )

// For it to be sunlit, the sun must be out and bright, we must be outdoors,
//   not under cover (ie forest) and not in a DARK or MAGIC_DARK room.
#define IS_SUNLIT(r) ( IS_DAY && !IS_TWILIGHT && IS_OUTDOORS(r) \
  && !IS_ROOM(r, ROOM_DARK | ROOM_MAGIC_DARK ) )

bool IS_TWILIGHT_ROOM(int r);
bool IS_OUTDOORS(int r);

#define IS_ROOM( room, flag ) (IS_SET(world[room].room_flags, (flag)) )

#define SET_BIT(var, bit)  ((var) = (var) | (bit))

#define REMOVE_BIT(var, bit)  ((var) = (var) & ~(bit) )

#define TOGGLE_BIT(var, bit) ((var) = (var) ^ (bit))
#define PLR_FLAGS(ch)          ((ch)->specials.act)
#define PLR2_FLAGS(ch)          ((ch)->specials.act2)
#define PLR3_FLAGS(ch)	    ((ch)->specials.act3)
#define PLR_FLAGGED(ch, flag)  (IS_SET(PLR_FLAGS(ch), flag))
#define PLR2_FLAGGED(ch, flag)  (IS_SET(PLR2_FLAGS(ch), flag))
#define PLR3_FLAGGED(ch, flag)  (IS_SET(PLR3_FLAGS(ch), flag))
#define PLR_TOG_CHK(ch, flag)  ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PLR2_TOG_CHK(ch, flag)  ((TOGGLE_BIT(PLR2_FLAGS(ch), (flag))) & (flag))
#define PLR3_TOG_CHK(ch, flag)  ((TOGGLE_BIT(PLR3_FLAGS(ch), (flag))) & (flag))

/* Can subject see character "obj"? */
#define IS_NOSHOW(obj)    (IS_SET(obj->extra_flags, ITEM_NOSHOW))

#define CAN_SEE(sub, obj)       (ac_can_see((sub), (obj), TRUE))
#define CAN_SEE_Z_CORD(sub, obj)       (ac_can_see((sub), (obj), FALSE))

#define IS_ARTIFACT(obj)  (IS_SET(obj->extra_flags, ITEM_ARTIFACT))
#define IS_IOUN(obj)      (CAN_WEAR(obj, ITEM_WEAR_IOUN))
#define IS_UNIQUE(obj)    (isname("unique", (obj)->name) && !isname("powerunique", (obj)->name))

#define GET_REQ(i) (i<1  ? "Awful" :(i<3  ? "Bad"     :(i<5  ? "Poor"      :\
(i<7 ? "Average" :(i<10 ? "Fair"    :(i<15 ? "Good"    :(i<24 ? "Very good" :\
  "Superb" )))))))

#define HSHR(ch) ((ch)->player.sex ?                                    \
  (((ch)->player.sex == 1) ? "his" : "her") : "its")

#define HSSH(ch) ((ch)->player.sex ?                                    \
  (((ch)->player.sex == 1) ? "he" : "she") : "it")

#define HMHR(ch) ((ch)->player.sex ?                                    \
  (((ch)->player.sex == 1) ? "him" : "her") : "it")

#define ANA(letter) (index("aeiouyAEIOUY", letter) ? "An" : "A")

#define SANA(obj) (index("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")

#define IS_NPC(ch) (IS_SET((ch)->specials.act, ACT_ISNPC))

#define IS_PC(ch) (!IS_NPC((ch)))

#define GET_PID(ch) ( !IS_PC(ch) ? raise(SIGSEGV) : (ch)->only.pc->pid )
#define GET_RNUM(ch) ( !IS_NPC(ch) ? raise(SIGSEGV) : (ch)->only.npc->R_num )
#define GET_VNUM(ch) ( !IS_NPC(ch) ? raise(SIGSEGV) : mob_index[(ch)->only.npc->R_num].virtual_number )
#define NPC_SPEC(ch, index) ( !IS_NPC(ch) ? raise(SIGSEGV) : (ch)->only.npc->spec[index] )
#define GET_ID(ch) (IS_ALIVE(ch) ? (IS_NPC(ch) ? (mob_index[(ch)->only.npc->R_num].virtual_number) : ((ch)->only.pc->pid)) : -2 )

#define GET_IDNUM(ch)   (ch->only.npc->idnum)

#define GET_POS(ch)     ((ch)->specials.position & 3)

#define GET_STAT(ch)     ((ch)->specials.position & STAT_MASK)

#define GET_COND(ch, i) ((ch)->specials.conditions[(i)])

#define GET_NAME1(ch)   (IS_DISGUISE((ch)) ? GET_DISGUISE_NAME((ch)) : GET_NAME((ch)))
#define GET_NAME(ch)    ((ch)->player.name)

#define GET_TRUE_CHAR(ch) ( ((ch)->desc && (ch)->desc->original) ? ((ch)->desc->original) : (ch) )
#define GET_TRUE_CHAR_D(d) ( ((d)->original) ? ((d)->original) : ((d)->character) )
#define GET_TRUE_NAME(ch) ( GET_TRUE_CHAR(ch)->player.name )
#define J_NAME_TRUE(ch) ( J_NAME(GET_TRUE_CHAR(ch)) )

#define GET_DISGUISE_NAME(ch) (IS_DISGUISE_PC(ch) ? (ch)->disguise.name : (ch)->disguise.title)
#define GET_DISGUISE_LONG(ch) ((ch)->disguise.longname)
#define GET_DISGUISE_SHORT(ch) ((ch)->disguise.short_descr)

#define J_NAME(a) (IS_NPC(a) ? a->player.short_descr : a->player.name)

/*
#define GET_TITLE(ch)   ((ch)->only.pc->title)
*/
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_DISGUISE_TITLE(ch) ((ch)->disguise.title)

#define GET_SECONDARY_LEVEL(ch)   ((int)(ch)->player.secondary_level)
#define GET_DISGUISE_LEVEL(ch) ((int)(ch)->disguise.level)

//#define GET_CLASS(ch)   ((ch)->player.m_class)
#define GET_SPEC(ch, cls, spc) (((ch)->player.m_class & cls) && (ch)->player.spec == spc)
#define IS_MULTICLASS(ch, cls1, cls2) (GET_CLASS((ch), (cls1)) && GET_CLASS((ch), (cls2)))
#define GET_DISGUISE_CLASS(ch) ((ch)->disguise.m_class)

#define GET_RACE2(ch)   (IS_DISGUISE_SHAPE((ch)) ? GET_DISGUISE_RACE((ch)) : GET_RACE((ch)))
#define GET_RACE1(ch)   (IS_DISGUISE((ch)) ? GET_DISGUISE_RACE((ch)) : GET_RACE((ch)))
#define GET_RACE(ch)    ((ch)->player.race)
#define GET_DISGUISE_RACE(ch) ((ch)->disguise.race)
#define GET_RACEWAR(ch) ((ch)->player.racewar)

int race_size(int race);

#define GET_PHYS_TYPE(ch) ((ch)->player.phys_type)

#define GET_HOME(ch)    ((ch)->player.hometown)
#define GET_BIRTHPLACE(ch) ((ch)->player.birthplace)
#define GET_ORIG_BIRTHPLACE(ch) ((ch)->player.orig_birthplace)

#define GET_AGE(ch)     (age(ch).year)

#define GET_C_STR(ch)     ((ch)->curr_stats.Str)
#define GET_C_DEX(ch)     ((ch)->curr_stats.Dex)
#define GET_C_AGI(ch)     ((ch)->curr_stats.Agi)
#define GET_C_CON(ch)     ((ch)->curr_stats.Con)
#define GET_C_POW(ch)     ((ch)->curr_stats.Pow)
#define GET_C_INT(ch)     ((ch)->curr_stats.Int)
#define GET_C_WIS(ch)     ((ch)->curr_stats.Wis)
#define GET_C_CHA(ch)     ((ch)->curr_stats.Cha)
#define GET_C_KAR(ch)     ((ch)->curr_stats.Kar)
#define GET_C_LUK(ch)     ((ch)->curr_stats.Luk)

#define GET_WIMPY(ch)   ((ch)->only.pc->wimpy)

#define GET_AC(ch)      ((ch)->points.curr_armor)

#define GET_HIT(ch)     ((ch)->points.hit)
#define GET_MAX_HIT(ch) ((ch)->points.max_hit)
#define GET_LOWEST_HIT(ch)     ((ch)->only.npc->lowest_hit)

#define GET_VITALITY(ch)    ((ch)->points.vitality)
#define GET_MAX_VITALITY(ch) ((ch)->points.max_vitality)

#define GET_MANA(ch)    ((ch)->points.mana)
#define GET_MAX_MANA(ch) ((ch)->points.max_mana)

#define GET_MONEY(ch) ((ch)->points.cash[0] + \
           10 * (ch)->points.cash[1] + \
                       100 * (ch)->points.cash[2] + \
                       1000 * (ch)->points.cash[3])

#define GET_BALANCE(ch) ((ch)->only.pc->spare1 + \
       10 * (ch)->only.pc->spare2 + \
       100 * (ch)->only.pc->spare3 + \
       1000 * (ch)->only.pc->spare4)

#define GET_COPPER(ch)    ((ch)->points.cash[0])

#define GET_SILVER(ch)    ((ch)->points.cash[1])

#define GET_GOLD(ch)    ((ch)->points.cash[2])

#define GET_PLATINUM(ch)    ((ch)->points.cash[3])

#define CLEAR_MONEY(ch) \
  GET_PLATINUM(ch) = 0; \
  GET_GOLD(ch) = 0; \
  GET_SILVER(ch) = 0; \
  GET_COPPER(ch) = 0;

#define GET_BALANCE_COPPER(ch) ((ch)->only.pc->spare1)

#define GET_BALANCE_SILVER(ch) ((ch)->only.pc->spare2)

#define GET_BALANCE_GOLD(ch) ((ch)->only.pc->spare3)

#define GET_BALANCE_PLATINUM(ch) ((ch)->only.pc->spare4)

#define GET_EXP(ch)     ((ch)->points.curr_exp)
#define GET_SECONDARY_EXP(ch)     ((ch)->points.curr_secondary_exp)

#define EXP_NOTCH(ch) (GET_LEVEL(ch) <= MAXLVLMORTAL ? MAX(new_exp_table[GET_LEVEL(ch) + 1] / 10, 1) : 1)

#define GET_HEIGHT(ch)  ((ush_int) ((IS_AFFECTED3(ch, AFF3_ENLARGE) ? ((ch)->player.height * 1.5) : \
                         IS_AFFECTED3(ch, AFF3_REDUCE) ? ((ch)->player.height / 1.5) : \
                         (ch)->player.height)))

#define GET_WEIGHT(ch)  (IS_AFFECTED3(ch, AFF3_ENLARGE) ? ((ch)->player.weight * 2) : \
                         IS_AFFECTED3(ch, AFF3_REDUCE) ? ((ch)->player.weight / 2) : \
                         (ch)->player.weight)

#define GET_SEX(ch)     ((ch)->player.sex)
/*
//Moved to utility.c for more complex code.. -Kvark
#define GET_ALT_SIZE(ch) (BOUNDED(SIZE_MINIMUM, ((ch)->player.size + \
                         ((IS_AFFECTED3((ch), AFF3_ENLARGE)) ? 1 : \
                         ((IS_AFFECTED3((ch), AFF3_REDUCE)) ? -1 : 0))) + \
                         ( ( IS_AFFECTED5( (ch) , AFF5_TITAN_FORM ) ) ? 3 : 0), \
                         SIZE_MAXIMUM))
*/
#define GET_SIZE(ch) ((ch)->player.size)

#define GET_HITROLL(ch) ((ch)->points.hitroll)

#define GET_DAMROLL(ch) ((ch)->points.damroll)
#define TRUE_DAMROLL(ch) ((ch)->points.damroll + str_app[STAT_INDEX(GET_C_STR(ch))].todam)

#define GET_WIZINVIS(ch) ((ch)->only.pc->wiz_invis)

#define GET_LFLAGS(ch)  ((ch)->only.pc->law_flags)

#define GET_PLAYER_LOG(ch) ((ch)->only.pc->log)

#define IS_AWAKE(ch) ((GET_STAT(ch) > STAT_SLEEPING) && !IS_AFFECTED((ch), AFF_KNOCKED_OUT))

#define MIN_POS(ch, v)  \
((GET_POS(ch) >= (ubyte)((v) & 3)) && (GET_STAT(ch) >= (ubyte)((v) & STAT_MASK)))

#define SET_POS(ch, v) ((ch)->specials.position = (ubyte)(v))

#define IS_DISGUISE(ch)  (IS_DISGUISE_PC(ch) || IS_DISGUISE_NPC(ch))
#define IS_DISGUISE_NPC(ch) ((ch)->disguise.active_npc)
#define IS_DISGUISE_PC(ch) ((ch)->disguise.active_pc)
#define IS_DISGUISE_ILLUSION(ch) ((ch)->disguise.active_illusion)
#define IS_DISGUISE_SHAPE(ch) ((ch)->disguise.active_shapechange)

/* metamorph related macros */

/*
#define IS_MORPH(a) (IS_NPC(a) && (a)->only.npc->memory &&      \
                     !IS_SET((a)->specials.act, ACT_MEMORY))
*/

     /* (use this to redirect xp, toggles, etc to the original body) */
/*
#define MORPH_ORIG(a) (IS_MORPH(a) ? (P_char) (a)->only.npc->memory : \
                       NULL)
*/
#define MORPH_ORIG(a) (IS_NPC(a) ? (a)->only.npc->orig_char : NULL)
/*
#define TRUSTED_NPC(a) ((IS_MORPH(a) && IS_TRUSTED(MORPH_ORIG(a))) || \
      (IS_NPC(a) && (a)->desc))
*/
#define TRUSTED_NPC(a) ((IS_NPC(a) && (a)->desc))

     /* generic function to point to either a PC, or the "owning"
        morph (or the original person switched into me)  */

#define GET_PLYR(a) (IS_MORPH(a) ? MORPH_ORIG(a) :               \
                     ((a)->desc && (a)->desc->original) ?        \
                     (a)->desc->original : (a))
#define SWITCHED(a) ( (a)->desc && (a)->desc->original && (a) == (a)->desc->original )

#define IS_SHOPKEEPER(a)  (IS_NPC(a) && ((mob_index[GET_RNUM(a)].qst_func == shop_keeper) || \
    (mob_index[GET_RNUM(a)].func.mob == shop_keeper)))

#define MOB_PROC(npc) (mob_index[GET_RNUM(npc)].func.mob)

     /* Object And Carry related macros */

#define CAN_SEE_OBJ(sub, obj)   (ac_can_see_obj((sub), (obj)))
#define CAN_SEE_OBJZ(sub, obj, z)   (ac_can_see_obj((sub), (obj), z))

#define GET_ITEM_TYPE(obj) ((obj)->type)

#define CAN_WEAR(obj, part) (IS_SET((obj)->wear_flags, part))

#define OBJ_VNUM(obj) ( !obj ? raise(SIGSEGV) : obj_index[obj->R_num].virtual_number )
#define OBJ_SHORT(obj) ((obj)->short_description)
#define GET_OBJ_PROC(obj) ( !obj ? raise(SIGSEGV) : obj_index[obj->R_num].func.obj )

#define OBJ_MAGIC(obj) ( IS_SET(obj->extra2_flags, ITEM2_MAGIC) )

/* this allows 'magic' containers with negative weight, until the negative
   amount is exceeded, they weight 0. JAB */

#define GET_OBJ_WEIGHT(obj)  (((obj)->weight > 0)? (obj)->weight: 0)

#define GET_OBJ_SIZE(obj)  (((obj)->size > 0) ? (obj)->size : 0)
#define GET_OBJ_SPACE(obj)  (((obj)->space > 0) ? (obj)->space : 0)

#define COIN_WEIGHT(c, s, g, p) (0)
//#define COIN_WEIGHT(c, s, g, p) (((c) + (s) + (g) + (p)) / 50)

#define CAN_CARRY_W(ch) (str_app[STAT_INDEX(GET_C_STR(ch))].carry_w + (IS_TRUSTED(ch)? 20000: 0))

#define CAN_CARRY_N(ch) (IS_TRUSTED(ch)? 3000: (STAT_INDEX(GET_C_DEX(ch)) / 3) + (IS_NPC(ch)? 12: 6)) 

#define IS_CARRYING_W(ch, rider) ( \
  (ch)->specials.carry_weight + COIN_WEIGHT(GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch), GET_PLATINUM(ch))     \
  + (( (rider = GET_RIDER( ch )) == NULL ) ? 0 : ( rider->player.weight + rider->specials.carry_weight )) \
  + COIN_WEIGHT(GET_COPPER( rider ), GET_SILVER( rider ), GET_GOLD( rider ), GET_PLATINUM( rider )) )

#define GET_CARRYING_W(ch) ((ch)->specials.carry_weight)

#define IS_CARRYING_N(ch) ((ch)->specials.carry_items)

#define CAN_CARRY_COINS(ch, rider) ((CAN_CARRY_W(ch) - IS_CARRYING_W(ch, rider)) * 25)

#define CAN_CARRY_OBJ(ch, obj, rider) (                                 \
  ((IS_CARRYING_W(ch, rider) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) \
  && ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj, rider) (                                                 \
  CAN_WEAR(obj, ITEM_TAKE) && CAN_CARRY_OBJ(ch, obj, rider) && CAN_SEE_OBJ(ch, obj) )

#define IS_OBJ_STAT(obj, stat)  (IS_SET((obj)->extra_flags, stat))
#define IS_OBJ_STAT2(obj, stat) (IS_SET((obj)->extra2_flags, stat))  /* TASFALEN */

#define CAN_ACT(ch)  (!IS_SET(ch->specials.act2, PLR2_WAIT))

#define OBJS(obj, vict)  (CAN_SEE_OBJ((vict), (obj))? (obj)->short_description: "something")

#define OBJN(obj, vict)  (CAN_SEE_OBJ((vict), (obj))? FirstWord((obj)->name): "something")

#define OUTSIDE(ch)  (!IS_ROOM((ch)->in_room, ROOM_INDOORS) && !IS_UNDERWORLD(ch->in_room))

#define ROOM_VOID_VNUM        0
#define ROOM_LIMBO_VNUM       1
#define ROOM_VNUM(rroom_id) ( (rroom_id >= 0 && rroom_id <= top_of_world) ? world[rroom_id].number : NOWHERE )
#define ROOM_VNUM0(rroom_id) ( (rroom_id >= 0 && rroom_id <= top_of_world) ? world[rroom_id].number : 0 )

#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])
#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])
#define VIRTUAL_EXIT(rnum, door)  (world[rnum].dir_option[door])

#define CAN_GO(ch, door)  \
        (ch && \
        EXIT(ch, door) && \
        (EXIT(ch, door)->to_room != NOWHERE) && \
        (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) || \
        IS_AFFECTED2(ch, AFF2_PASSDOOR)) && \
        !IS_SET(EXIT(ch, door)->exit_info, EX_SECRET | EX_BLOCKED) && \
	!check_castle_walls(ch->in_room, EXIT(ch, door)->to_room))

#define VIRTUAL_CAN_GO(room, door) \
        (VIRTUAL_EXIT(room, door) && \
  (VIRTUAL_EXIT(room, door)->to_room != NOWHERE) && \
        !IS_SET(VIRTUAL_EXIT(room, door)->exit_info, EX_CLOSED | EX_SECRET | EX_BLOCKED))

#define GET_ALIGNMENT(ch) ((ch)->specials.alignment)
#define GET_PRESTIGE(ch) ((ch)->only.pc->prestige)
#define GET_TIME_LEFT_GUILD(ch) ((ch)->only.pc->time_left_guild)
#define GET_NB_LEFT_GUILD(ch) ((ch)->only.pc->nb_left_guild)

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_OPPOSING_ALIGN(ch, victim) ((IS_GOOD(ch) && IS_EVIL(victim)) || \
                                       (IS_EVIL(ch) && IS_GOOD(victim)))


#define IS_BACKRANKED(ch) ((ch)->group && IS_SET((ch)->specials.act2, ACT2_BACK_RANK))

#define NPC_REMEMBERS_GROUP(ch) (IS_ELITE(ch) || \
                                 IS_SET(ch->specials.act2, ACT2_REMEMBERS_GROUP))

/* New Macros */

#define CHAR_IN_SAFE_ROOM(CH)  IS_ROOM( (CH)->in_room, ROOM_SAFE )

#define CHAR_IN_NO_MAGIC_ROOM(CH)  IS_ROOM( (CH)->in_room, ROOM_NO_MAGIC)

#define MOB_IN_NO_MAGIC_ROOM(ch) (IS_ROOM( ch->in_room, (ROOM_NO_MAGIC | ROOM_SILENT) || \
                                 ch->in_room != NOWHERE) || \
                                 IS_ROOM( ch->in_room, ROOM_SAFE))

#define CHAR_IN_PRIV_ZONE(CH) IS_SET(zone_table[world[(CH)->in_room].zone].flags, ZONE_PRIVATE)
#define PRIVATE_ZONE(room) IS_SET(zone_table[world[(room)].zone].flags, ZONE_PRIVATE)

#define CHAR_IN_ARENA(CH) IS_ROOM( (CH)->in_room, ROOM_ARENA )

#define CHAR_IN_HEAL_ROOM(CH)  ((CH)->in_room != NOWHERE &&\
    (IS_ROOM( (CH)->in_room, ROOM_HEAL ) ||\
     get_spell_from_room(&world[ch->in_room], SPELL_CONSECRATE_LAND)))

#define CHAR_IN_NO_HEAL_ROOM(CH)\
  ( ((CH)->in_room != NOWHERE) && IS_ROOM((CH)->in_room, ROOM_NO_HEAL) )

#define NEEDS_HEAL(ch) (ch->points.hit < ch->points.base_hit)

#define HAS_MEMORY(CH)  (IS_NPC(CH) && IS_SET((CH)->specials.act, ACT_MEMORY))

#define GET_MEMORY(CH)  (((P_char) (CH))->only.npc->memory)

#define HAS_RESCUE(CH)  IS_SET((CH)->specials.act, ACT_RESCUE)

#define LOOP_THRU_PEOPLE(IN_ROOM, PLAYER) \
for ((IN_ROOM) = world[(PLAYER)->in_room].people; (IN_ROOM) != NULL; (IN_ROOM) = (IN_ROOM)->next_in_room)

#define RANDOM_VNUM_BEGIN 100000
#define RANDOM_VNUM_END 200000
#define RANDOM_OBJ_VNUM 1250
#define IS_RANDOM_MOB(a) (IS_NPC(a) && GET_VNUM(a) > RANDOM_VNUM_BEGIN && GET_VNUM(a) < RANDOM_VNUM_END)

#define IS_PATROL(CH) (IS_NPC(CH) && IS_SET((CH)->specials.act, ACT_PATROL))

/*
#define IS_AGGRESSIVE(MOB) (IS_NPC(MOB) && \
  ((IS_SET((MOB)->specials.act, ACT_AGGRESSIVE)) || \
  (IS_SET((MOB)->specials.act, ACT_AGGRESSIVE_EVIL)) || \
  (IS_SET((MOB)->specials.act, ACT_AGGRESSIVE_GOOD)) || \
  (IS_SET((MOB)->specials.act, ACT_AGGRESSIVE_NEUTRAL)) || \
  (IS_SET((MOB)->specials.act, ACT_AGG_RACEEVIL)) || \
  (IS_SET((MOB)->specials.act, ACT_AGG_RACEGOOD)) || \
  (IS_SET((MOB)->specials.act, ACT_AGG_OUTCAST))))
*/

#define IS_AGGRESSIVE(m) (IS_NPC(m) && ((m)->only.npc->aggro_flags || (m)->only.npc->aggro2_flags || (m)->only.npc->aggro3_flags))

#define CHAR_IS_FLAGGED(CH)  \
(0)
/* have to redo this macro for new law flags JAB
(IS_NPC(CH)? (IS_SET((CH)->specials.act, NPC_OUTLAW)? 2: 0):  \
 IS_SET((CH)->only.pc->law_flags, PLR_OUTCAST)? 4:  \
 IS_SET((CH)->only.pc->law_flags, PLR_KILLER)?  3:  \
 IS_SET((CH)->only.pc->law_flags, PLR_OUTLAW)?  2:  \
 IS_SET((CH)->only.pc->law_flags, PLR_THIEF)?   1: 0)
*/

#define GET_LEVEL(character) ((character==NULL) ? -1 :(int) character->player.level)

 // Most common fail: NPC, second: mortal, third: fog set.
#define IS_TRUSTED(ch) (IS_PC(ch) && GET_LEVEL(ch) > MAXLVLMORTAL && !IS_SET(ch->specials.act, PLR_MORTAL))
  /* || IS_SET(ch->specials.act, PLR_DEBUG))*/

#define IS_FIGHTING(ch) ( (ch)->specials.fighting != NULL )
#define IS_DESTROYING(ch) ( (ch)->specials.destroying_obj != NULL )
#define GET_OPPONENT(ch) (ch->specials.fighting)

/* Defining this to make life considerably easier - SKB 24 Mar 1995 */
#define IS_CASTING(ch) (IS_SET((ch)->specials.affected_by2, AFF2_CASTING))

#define IS_RIDING(ch) (get_linked_char((ch), LNK_RIDING))

#define OBJ_FALLING(o) ((o) && !IS_SET((o)->extra_flags, ITEM_LEVITATES) && OBJ_ROOM(o) \
  && (   (world[(o)->loc.room].sector_type == SECT_NO_GROUND) \
      || (world[(o)->loc.room].sector_type == SECT_UNDRWLD_NOGROUND) \
      || (world[(o)->loc.room].chance_fall >= number(1,100)) \
      || (o->z_cord > 0))  )

#define IS_OUTLAW(ch) ((IS_NPC(ch) && IS_SET(ch->specials.act, NPC_OUTLAW))|| \
      (IS_PC(ch) && IS_SET(ch->only.pc->law_flags, PLR_OUTLAW)))

#define IS_GUARD(ch)  (IS_NPC(ch) && (IS_SET((ch)->only.npc->aggro_flags, AGGR_OUTCASTS) || isname("guard", GET_NAME(ch))))

#define IS_DRAGON(ch)  ((GET_RACE(ch) == RACE_DRAGON) || IS_DRACOLICH(ch))
#define IS_DRAGONKIN(ch) ((GET_RACE(ch) == RACE_DRAGONKIN))
#define CAN_BREATHE(ch) (IS_NPC(ch) && (IS_DRAGON(ch) || \
                         IS_AVATAR(ch) || IS_TITAN(ch) || \
                         IS_ACT(ch, ACT_BREATHES_FIRE | ACT_BREATHES_LIGHTNING | ACT_BREATHES_FROST | ACT_BREATHES_ACID | \
            ACT_BREATHES_GAS | ACT_BREATHES_SHADOW | ACT_BREATHES_BLIND_GAS)))

#define IS_SLIME(ch) (GET_RACE(ch) == RACE_SLIME)
#define IS_DEMON(ch)  (GET_RACE(ch) == RACE_DEMON)
#define IS_DEVIL(ch) (GET_RACE(ch) == RACE_DEVIL)

#define IS_GIANT(ch)  ((GET_RACE(ch) == RACE_GIANT) || \
                       (GET_RACE(ch) == RACE_OGRE) || \
                       (GET_RACE(ch) == RACE_SGIANT) || \
                       (GET_RACE(ch) == RACE_MINOTAUR) || \
                       (GET_RACE(ch) == RACE_SNOW_OGRE) || \
                       (GET_RACE(ch) == RACE_FIRBOLG) || \
                       (GET_RACE(ch) == RACE_FIREGIANT) || \
                       (GET_RACE(ch) == RACE_FROSTGIANT))

#define IS_INSECT(ch) ((GET_RACE(ch) == RACE_INSECT))

#define IS_PWORM(ch) ((GET_RACE(ch) == RACE_PWORM))

#define IS_BEHOLDER(ch) ((GET_RACE(ch) == RACE_BEHOLDER))
#define IS_BEHOLDERKIN(ch) ((GET_RACE(ch) == RACE_BEHOLDERKIN))

#define IS_DRACOLICH(ch) (GET_RACE(ch) == RACE_DRACOLICH)
#define IS_TITAN(ch) (GET_RACE(ch) == RACE_TITAN)
#define IS_AVATAR(ch) (GET_RACE(ch) == RACE_AVATAR)

#define IS_UNDEADRACE(ch)  ((GET_RACE(ch) == RACE_UNDEAD) || \
           (GET_RACE(ch) == RACE_LICH) || \
           (GET_RACE(ch) == RACE_PVAMPIRE) || \
           (GET_RACE(ch) == RACE_SHADE) || \
           (GET_RACE(ch) == RACE_REVENANT) || \
           (GET_RACE(ch) == RACE_PSBEAST) || \
           (GET_RACE(ch) == RACE_WIGHT) || \
           (GET_RACE(ch) == RACE_PHANTOM) || \
           (IS_DRACOLICH(ch)) || \
           (IS_UNDEAD(ch)))

#define IS_THEURPET_RACE(ch) ((GET_RACE(ch) == RACE_ARCHON) || \
           (GET_RACE(ch) == RACE_ASURA) || \
           (GET_RACE(ch) == RACE_BRALANI) || \
           (GET_RACE(ch) == RACE_GHAELE) || \
           (GET_RACE(ch) == RACE_ELADRIN) || \
           (GET_RACE(ch) == RACE_TITAN) || \
           (GET_RACE(ch) == RACE_AVATAR))

#define IS_UNDEAD(ch) ((GET_RACE(ch) == RACE_UNDEAD) || \
           (GET_RACE(ch) == RACE_GHOST) || \
           (GET_RACE(ch) == RACE_VAMPIRE) || \
           (GET_RACE(ch) == RACE_LICH) || \
           (GET_RACE(ch) == RACE_PDKNIGHT) || \
           (GET_RACE(ch) == RACE_ZOMBIE) || \
           (GET_RACE(ch) == RACE_SPECTRE) || \
           (GET_RACE(ch) == RACE_SKELETON) || \
           (GET_RACE(ch) == RACE_WRAITH) || \
           (GET_RACE(ch) == RACE_SHADOW) || \
           (IS_DRACOLICH(ch)) || \
           (IS_AFFECTED(ch, AFF_WRAITHFORM) ) || \
           (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) && !GET_CLASS(ch, CLASS_THEURGIST) ) || \
           (OLD_RACE_PUNDEAD(GET_RACE(ch))))

#define IS_ANGELIC(ch) (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM) && GET_CLASS(ch, CLASS_THEURGIST))

#define IS_ELEMENTAL(ch) ((GET_RACE(ch) == RACE_F_ELEMENTAL) || \
        (GET_RACE(ch) == RACE_A_ELEMENTAL) || \
        (GET_RACE(ch) == RACE_W_ELEMENTAL) || \
		(GET_RACE(ch) == RACE_V_ELEMENTAL) || \
		(GET_RACE(ch) == RACE_I_ELEMENTAL) || \
        (GET_RACE(ch) == RACE_E_ELEMENTAL))

#define IS_PC_CORPSE(obj) ( (obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], PC_CORPSE) )

#define IS_NOCORPSE(ch) ((GET_RACE(ch) == RACE_UNDEAD) || \
           ( GET_RACE(ch) == RACE_GHOST ) || \
           ( GET_RACE(ch) == RACE_VAMPIRE ) || \
           ( GET_RACE(ch) == RACE_LICH ) || \
           ( GET_RACE(ch) == RACE_ZOMBIE ) || \
           ( GET_RACE(ch) == RACE_SPECTRE ) || \
           ( GET_RACE(ch) == RACE_SKELETON ) || \
           ( GET_RACE(ch) == RACE_WRAITH ) || \
           ( GET_RACE(ch) == RACE_DEVA) || \
           ( GET_RACE(ch) == RACE_SHADOW ) || \
           ( GET_RACE(ch) == RACE_W_ELEMENTAL ) || \
           ( GET_RACE(ch) == RACE_A_ELEMENTAL ) || \
           ( GET_RACE(ch) == RACE_F_ELEMENTAL ) || \
           ( GET_RACE(ch) == RACE_E_ELEMENTAL ) || \
	   ( GET_RACE(ch) == RACE_V_ELEMENTAL ) || \
           (affected_by_spell(ch, TAG_CONJURED_PET)) || \
           ( GET_RACE(ch) == RACE_I_ELEMENTAL ) || \
           ( IS_THEURPET_RACE(ch) ) || \
           ( GET_RACE(ch) == RACE_EFREET ))


#define IS_ANIMAL(ch) ((GET_RACE(ch) == RACE_ANIMAL) || \
           (GET_RACE(ch) == RACE_AQUATIC_ANIMAL) || \
           (GET_RACE(ch) == RACE_FLYING_ANIMAL) || \
           (GET_RACE(ch) == RACE_QUADRUPED) || \
           (GET_RACE(ch) == RACE_PRIMATE) || \
           (GET_RACE(ch) == RACE_HERBIVORE) || \
           (GET_RACE(ch) == RACE_CARNIVORE) || \
           (GET_RACE(ch) == RACE_PARASITE) || \
           (GET_RACE(ch) == RACE_REPTILE) || \
           (GET_RACE(ch) == RACE_DRAGONKIN) || \
           (GET_RACE(ch) == RACE_SNAKE) || \
           (GET_RACE(ch) == RACE_ARACHNID) || \
           (GET_RACE(ch) == RACE_INSECT))

#define IS_PLANT(ch) ((GET_RACE(ch) == RACE_PLANT) || \
           (GET_RACE(ch) == RACE_SLIME))

#define IS_HUMANOID(ch) ((GET_RACE(ch) > RACE_NONE) && \
           (GET_RACE(ch) < 31) || \
           (GET_RACE(ch) == RACE_FIRBOLG)   || \
           (GET_RACE(ch) == RACE_AGATHINON) || \
           (GET_RACE(ch) == RACE_HUMANOID)  || \
           (GET_RACE(ch) == RACE_HALFORC)   || \
           (GET_RACE(ch) == RACE_FAERIE)    || \
           (GET_RACE(ch) == RACE_PVAMPIRE)  || \
           (GET_RACE(ch) == RACE_VAMPIRE)   || \
           (GET_RACE(ch) == RACE_RAKSHASA)  || \
           (GET_RACE(ch) == RACE_ANGEL)     || \
           (GET_RACE(ch) == RACE_DRIDER)    || \
           (GET_RACE(ch) == RACE_SNOW_OGRE) || \
           (GET_RACE(ch) == RACE_LYCANTH)   || \
           (GET_RACE(ch) == RACE_ZOMBIE)    || \
           (GET_RACE(ch) == RACE_PRIMATE)   || \
           (GET_RACE(ch) == RACE_ELADRIN)   || \
           (GET_RACE(ch) == RACE_KOBOLD)    || \
           (GET_RACE(ch) == RACE_PILLITHID) || \
           (GET_RACE(ch) == RACE_KUOTOA)    || \
           (GET_RACE(ch) == RACE_WOODELF)   || \
           (GET_RACE(ch) == RACE_ARCHON)    || \
           (GET_RACE(ch) == RACE_ASURA)     || \
           (GET_RACE(ch) == RACE_GHAELE)    || \
           (GET_RACE(ch) == RACE_BRALANI)   || \
           (GET_RACE(ch) == RACE_INCUBUS)   || \
           (GET_RACE(ch) == RACE_SUCCUBUS))

#define IS_CONTROLLER_RACE(race) ( (race >= RACE_HUMAN && race <= RACE_PLAYER_MAX && race != RACE_ILLITHID) \
  || race == RACE_AGATHINON || race == RACE_HUMANOID || race == RACE_HALFORC ||  race == RACE_FAERIE \
  || race == RACE_VAMPIRE || race == RACE_RAKSHASA || race == RACE_ANGEL || race == RACE_SNOW_OGRE \
  || race == RACE_LYCANTH || race == RACE_ZOMBIE || race == RACE_PRIMATE || race == RACE_ELADRIN )

#define IS_MULTICLASS_NPC(ch) (IS_NPC(ch) && IS_AFFECTED4(ch, AFF4_MULTI_CLASS))

#define IS_HARDCORE(ch) (IS_PC(ch) && IS_SET(ch->specials.act2, PLR2_HARDCORE_CHAR))


#define IS_MULTICLASS_PC(ch) (IS_PC(ch) && ((ch)->player.secondary_class > 0) && ((ch)->player.secondary_class != BIT_32))

#define IS_FULL_MULTICLASS_PC(ch) (IS_PC(ch) && ((ch)->player.secondary_level == 56))

#define IS_NEWBIE(ch) (IS_PC(ch) && IS_SET(ch->specials.act2, PLR2_NEWBIE))
#define IS_NEWBIE_GUIDE(ch) (IS_PC(ch) && IS_SET(ch->specials.act2, PLR2_NEWBIE_GUIDE))

#define FIRESHIELDED(ch) (IS_AFFECTED2(ch, AFF2_FIRESHIELD))

#define COLDSHIELDED(ch) (IS_AFFECTED3(ch, AFF3_COLDSHIELD))

#define LIGHTNINGSHIELDED(ch) (IS_AFFECTED3(ch, AFF3_LIGHTNINGSHIELD))

#define IS_ILLITHID(ch) (GET_RACE(ch) == RACE_ILLITHID)

#define IS_PILLITHID(ch) (GET_RACE(ch) == RACE_PILLITHID)

#define IS_OGRE(ch) (GET_RACE(ch) == RACE_OGRE)

#define IS_HARPY(ch) (GET_RACE(ch) == RACE_HARPY || GET_RACE(ch) == RACE_GARGOYLE)

#define IS_THRIKREEN(ch) (GET_RACE(ch) == RACE_THRIKREEN)

#define IS_CENTAUR(ch) (GET_RACE(ch) == RACE_CENTAUR)

#define IS_GOBLIN(ch) (GET_RACE(ch) == RACE_GOBLIN)

#define IS_GITHYANKI(ch) (GET_RACE(ch) == RACE_GITHYANKI)

#define IS_MINOTAUR(ch) (GET_RACE(ch) == RACE_MINOTAUR)

#define IS_TIEFLING(ch) (GET_RACE(ch) == RACE_TIEFLING)

#define IS_SHADE(ch) (GET_RACE(ch) == RACE_SHADE)

#define IS_REVENANT(ch) (GET_RACE(ch) == RACE_REVENANT)

#define IS_HALFORC(ch) (GET_RACE(ch) == RACE_HALFORC)

#define IS_SGIANT(ch) (GET_RACE(ch) == RACE_SGIANT)

#define IS_WIGHT(ch) (GET_RACE(ch) == RACE_WIGHT)

#define IS_PHANTOM(ch) (GET_RACE(ch) == RACE_PHANTOM)

#define IS_PSBEAST(ch) (GET_RACE(ch) == RACE_PSBEAST)

#define IS_EFREET(ch) (GET_RACE(ch) == RACE_EFREET)

#define IS_SNOWOGRE(ch) (GET_RACE(ch) == RACE_SNOW_OGRE)

#define IS_ZOMBIE(ch) (GET_RACE(ch) == RACE_ZOMBIE)

#define IS_SPECTRE(ch) (GET_RACE(ch) == RACE_SPECTRE)

#define IS_SKELETON(ch) (GET_RACE(ch) == RACE_SKELETON)

#define IS_WRAITH(ch) (GET_RACE(ch) == RACE_WRAITH)

#define IS_SHADOW(ch) (GET_RACE(ch) == RACE_SHADOW)

#define IS_DEVA(ch) (GET_RACE(ch) == RACE_DEVA)

#define IS_DRIDER(ch) (GET_RACE(ch) == RACE_DRIDER)

#define IS_FIRBOLG(ch) (GET_RACE(ch) == RACE_FIRBOLG)

#define IS_ARCHON(ch) (GET_RACE(ch) == RACE_ARCHON)

#define IS_ASURA(ch) (GET_RACE(ch) == RACE_ASURA)

#define IS_BRALANI(ch) (GET_RACE(ch) == RACE_BRALANI)

#define IS_KOBOLD(ch) (GET_RACE(ch) == RACE_KOBOLD)

#define IS_DWARF(ch)  (GET_RACE(ch) == RACE_MOUNTAIN || GET_RACE(ch) == RACE_DUERGAR)

#define HAS_TAIL(ch) (IS_CENTAUR(ch) || IS_MINOTAUR(ch) || IS_PSBEAST(ch) || IS_KOBOLD(ch))

#define HAS_FOUR_HANDS(ch) ((GET_RACE(ch) == RACE_THRIKREEN) || \
                           (IS_AFFECTED3((ch), AFF3_FOUR_ARMS)))

#define IS_INCORPOREAL(ch) (IS_IMMATERIAL(ch))

#define IS_PURE_CASTER_CLASS(cls) ( (cls) &\
  (CLASS_SORCERER | CLASS_CONJURER | CLASS_ILLUSIONIST | CLASS_SUMMONER | \
  CLASS_NECROMANCER | CLASS_CLERIC | CLASS_SHAMAN | CLASS_BARD |\
  CLASS_DRUID | CLASS_ETHERMANCER | CLASS_THEURGIST | CLASS_BLIGHTER))
#define IS_PARTIAL_CASTER_CLASS(cls) ( (cls) &\
   (CLASS_AVENGER))
#define IS_SEMI_CASTER_CLASS(cls) ( (cls) &\
  (CLASS_ANTIPALADIN | CLASS_PALADIN | CLASS_RANGER |\
   CLASS_REAVER | CLASS_AVENGER))
#define IS_CASTER_CLASS(cls) (\
  IS_PURE_CASTER_CLASS(cls) || IS_SEMI_CASTER_CLASS(cls) )
#define IS_BOOK_CLASS(cls) ( (cls) &\
  (CLASS_SORCERER | CLASS_CONJURER | CLASS_NECROMANCER |\
   CLASS_ILLUSIONIST | CLASS_BARD | CLASS_SUMMONER | \
   CLASS_REAVER | CLASS_THEURGIST ))
#define IS_PRAYING_CLASS(cls) ( (cls) &\
  (CLASS_CLERIC | CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_AVENGER))
#define IS_MEMING_CLASS(cls) (IS_BOOK_CLASS(cls) || ((cls) & CLASS_SHAMAN))

#define USES_COMMUNE(ch) ((IS_NPC(ch) || GET_PRIME_CLASS(ch, CLASS_DRUID) || GET_PRIME_CLASS(ch, CLASS_RANGER) \
|| (!(IS_CASTER_CLASS(ch->player.m_class) || USES_MANA(ch)) \
&& ( GET_SECONDARY_CLASS(ch, CLASS_DRUID) || GET_SECONDARY_CLASS( ch, CLASS_RANGER) )) ))

#define USES_DEFOREST(ch)( IS_PC(ch) && (GET_PRIME_CLASS(ch, CLASS_BLIGHTER) \
|| ( !(IS_CASTER_CLASS(ch->player.m_class) || USES_MANA(ch)) && GET_SECONDARY_CLASS( ch, CLASS_BLIGHTER ) ) ) )

#define USES_MANA(ch) ((GET_RACE(ch) == RACE_PILLITHID) || \
						(GET_RACE(ch) == RACE_ILLITHID)  || \
						(GET_PRIME_CLASS(ch, CLASS_PSIONICIST)) || \
						(GET_PRIME_CLASS(ch, CLASS_MINDFLAYER)) || \
            (!IS_CASTER_CLASS(ch->player.m_class) && \
            (GET_SECONDARY_CLASS(ch, CLASS_PSIONICIST) || \
            GET_SECONDARY_CLASS(ch, CLASS_MINDFLAYER))) )

//    (IS_NPC(ch) || IS_SET((ch)->player.m_class, CLASS_PSIONICIST)) ||
//
  //  (IS_SET((ch)->player.m_class, CLASS_MINDFLAYER))
#define USES_TUPOR(ch) (IS_HARPY(ch) || GET_CLASS(ch, CLASS_ETHERMANCER))

#define USES_SPELL_SLOTS(ch) ( \
        USES_COMMUNE(ch) || \
        USES_DEFOREST(ch) || \
        IS_PUNDEAD(ch) || \
        USES_TUPOR(ch) || \
        USES_FOCUS(ch) || \
        IS_ANGEL(ch))

#define IS_SPELL_S(n) (IS_SET(skills[n].targets, TAR_SPELL))
#define IS_SPELL(n) (n>=FIRST_SPELL && n<=LAST_SPELL && IS_SPELL_S(n))
#define IS_SKILL(n) (n>=FIRST_SKILL && n<=LAST_SKILL && !IS_SPELL(n) )
#define IS_POISON(n) (n>=FIRST_POISON && n<=LAST_POISON)
#define IS_EPIC_SKILL(n) (IS_SKILL(n) && IS_SET(skills[n].targets, TAR_EPIC))
#define IS_INSTRUMENT_SKILL(n) (n>=FIRST_INSTRUMENT && n<=LAST_INSTRUMENT)
#define IS_BARD_SONG(n) (n>=FIRST_SONG && n<=LAST_SONG)
#define IS_TRADESKILL(n) (n == SKILL_FORGE || n == SKILL_MINE || n == SKILL_CRAFT)

#define POW_DIFF(att, def) (STAT_INDEX(GET_C_POW(def)) - STAT_INDEX(GET_C_POW(att)))

/* terminal stuff */
#define TERM_GENERIC        1
#define TERM_ANSI           2
#define TERM_MSP            3   /* mud sound protocol */
#define TERM_SKIP_ANSI      9
#define TERM_HELP         254
#define TERM_UNDEFINED    255

#define IS_ANSI_TERM(i) (i && (i->term_type == TERM_ANSI || i->term_type == TERM_MSP))

#define CAN_SPEAK(ch) ( IS_HUMANOID(ch) || IS_DRAGON(ch) || IS_DRAGONKIN(ch) || IS_DEMON(ch) || IS_GIANT(ch) \
  || (( IS_PC_PET(ch) || IS_PC(ch) ) && !IS_ANIMAL(ch)) || IS_UNDEAD(ch) || IS_EFREET(ch) || IS_TITAN(ch) )

/* For Moradin's lag command but useful for other things, so I put it here. */
/* #define GET_VICTIM_ROOM(v, c, a)  (v) = get_char_room((c), (a)) */
/* #define GET_VICTIM_ROOM(v, c, a)  (v) = get_char_room((c)->in_room, (a))  */

/* next two are the same, except one is for skill numbers and the other for
   command numbers.  Rather than changing whole bunches of places when a new
   agg command is added, just change these macros, much easier.  What goes in
   these lists?  Anything that can start one character fighting another, or
   that can cause harm in some form to a character, even if not all forms of
   the command/skill are harmful (cast/use/recite/chant/etc). */

#define IS_AGG_CMD(cmd) ((cmd == CMD_KILL) || (cmd == CMD_HIT) || \
       (cmd == CMD_BACKSTAB) || (cmd == CMD_BASH) || \
       (cmd == CMD_CAST) || (cmd == CMD_RESCUE) || \
       (cmd == CMD_STEAL) || (cmd == CMD_KICK) || \
       (cmd == CMD_USE) || (cmd == CMD_RECITE) || \
       (cmd == CMD_CIRCLE) || (cmd == CMD_HITALL) || \
       (cmd == CMD_MURDER) || (cmd == CMD_DISARM) || \
       (cmd == CMD_ASSIST) || (cmd == CMD_BODYSLAM) || \
       (cmd == CMD_HEADBUTT) || (cmd == CMD_SPRINGLEAP) || \
       (cmd == CMD_CHANT) || (cmd == CMD_DRAGONPUNCH) || \
       (cmd == CMD_TRAP) || (cmd == CMD_WILL) || \
       (cmd == CMD_STAMPEDE) || (cmd == CMD_CHARGE) || \
       (cmd == CMD_FIRE) || (cmd == CMD_BREATH) || \
       (cmd == CMD_OGRE_ROAR) || (cmd == CMD_BEARHUG)|| \
       (cmd == CMD_THROAT_CRUSH))

#define IS_AGG_SKILL(skl) ((skl == SKILL_BACKSTAB) || (skl == SKILL_BASH) || \
     (skl == SKILL_BUDDHA_PALM) || (skl == SKILL_CIRCLE) || (skl == SKILL_DISARM) || \
     (skl == SKILL_DRAGON_PUNCH) || (skl == SKILL_HEADBUTT) || (skl == SKILL_HITALL) || \
     (skl == SKILL_KICK) || (skl == SKILL_QUIVERING_PALM) || (skl == SKILL_RESCUE) || \
     (skl == SKILL_SPRINGLEAP) || (skl == SKILL_STEAL) || (skl == SKILL_TRAP) || \
     (skl == SONG_CHARMING) || (skl == SONG_FORGETFULNESS) || (skl == SONG_HARMING) || \
     (skl == SONG_SLEEP) || (skl == SKILL_THROAT_CRUSH) || (skl == SKILL_SHIELDPUNCH) || \
     (skl == SKILL_ROUNDKICK) || (skl == SKILL_DIRT_TOSS) || (skl == SKILL_SWEEPING_THRUST) || \
     (skl == SKILL_INDIRECT_SHOT) || (skl == SKILL_BEARHUG) || (skl == SKILL_FLANK) || \
     (skl == SKILL_KI_STRIKE) || (skl == SKILL_JIN_TOUCH) || (skl == SKILL_LANCE_CHARGE) || \
     (skl == SKILL_MAUL) || (skl == SKILL_SHIELD_PUNCH) || (skl == SONG_STORMS) || \
     (skl == SKILL_TACKLE) || (skl == SKILL_TRIP) || (skl == SKILL_WHIRLWIND) || \
     (skl == SKILL_GAZE) || (skl == SKILL_TRAMPLE) || (skl == SONG_DISCORD) || \
     (skl == SONG_HARMONY) || (skl == SKILL_RESTRAIN))

#define IS_AGG_SPELL(spl) (IS_SET(skills[spl].targets, TAR_AGGRO))

#define STATE(d) ((d)->connected)

/* new easy-to-use object handling macros */

#define OBJ_WORN(o)    (((o) != NULL) && ((o)->loc_p & LOC_WORN))
#define OBJ_CARRIED(o) (((o) != NULL) && ((o)->loc_p & LOC_CARRIED))
#define OBJ_ROOM(o)    (((o) != NULL) && ((o)->loc_p & LOC_ROOM))
#define OBJ_INSIDE(o)  (((o) != NULL) && ((o)->loc_p & LOC_INSIDE))
#define OBJ_NOWHERE(o) (((o) != NULL) && ((o)->loc_p & LOC_NOWHERE))

#define OBJ_WORN_BY(o, c)     (OBJ_WORN(o) && ((o)->loc.wearing == (c)))
#define WEARER(o)             (OBJ_WORN(o) ? (o)->loc.wearing : NULL)
#define OBJ_WORN_POS(o, p)    (OBJ_WORN(o) && ((o)->loc.wearing->equipment[(p)] == (o)))
#define OBJ_CARRIED_BY(o, c)  (OBJ_CARRIED(o) && ((o)->loc.carrying == (c)))
#define OBJ_IN_ROOM(o, r)     (OBJ_ROOM(o) && ((o)->loc.room == (r)))
#define OBJ_INSIDE_OBJ(o, o2) (OBJ_INSIDE(o) && ((o)->loc.inside == (o2)))

/* This defines the planar vnum ranges, for use in procs/ect */

#define WATERP_VNUM_BEGIN 23201
#define WATERP_VNUM_END 23325
#define FIREP_VNUM_BEGIN 25401
#define FIREP_VNUM_END 25525
#define EARTHP_VNUM_BEGIN 23801
#define EARTHP_VNUM_END 23929
#define AIRP_VNUM_BEGIN 24401
#define AIRP_VNUM_END 24525
#define ASTRAL_VNUM_BEGIN 19701
#define ASTRAL_VNUM_END 19825
#define ETH_VNUM_BEGIN 12401
#define ETH_VNUM_END 12550
#define NEG_VNUM_BEGIN 26600
#define NEG_VNUM_END 26869

/* Race determinators --MIAX */

#define IS_RACEWAR_GOOD(ch) (GET_RACEWAR(ch) == RACEWAR_GOOD)
#define IS_RACEWAR_EVIL(ch) (GET_RACEWAR(ch) == RACEWAR_EVIL)
#define IS_RACEWAR_UNDEAD(ch) (GET_RACEWAR(ch) == RACEWAR_UNDEAD)
#define IS_RACEWAR_NEUTRAL(ch) (GET_RACEWAR(ch) == RACEWAR_NEUTRAL)

#define EVIL_RACE(ch)     IS_RACEWAR_EVIL(ch)
#define GOOD_RACE(ch)     IS_RACEWAR_GOOD(ch)
#define PUNDEAD_RACE(ch)  IS_RACEWAR_UNDEAD(ch)

#define OLD_RACE_NEUTRAL(race) ( (race == RACE_THRIKREEN) \
                              || (race == RACE_MINOTAUR) \
		              || (race == RACE_TIEFLING) )

#define OLD_RACE_GOOD(race, align)( (race == RACE_HUMAN) \
                                 || (race == RACE_GREY) \
                                 || (race == RACE_MOUNTAIN) \
                                 || (race == RACE_BARBARIAN) \
                                 || (race == RACE_GNOME) \
                                 || (race == RACE_HALFLING) \
                                 || (race == RACE_HALFELF) \
                                 || (race == RACE_CENTAUR) \
                                 || (race == RACE_GITHZERAI) \
                                 || (race == RACE_AGATHINON) \
                                 || (race == RACE_WOODELF) \
                                 || (race == RACE_FIRBOLG) \
                                 || (race == RACE_ELADRIN) \
                                 || (OLD_RACE_NEUTRAL(race) && (align >= 0)) )

#define OLD_RACE_EVIL(race, align) ( (race == RACE_DROW) \
                                  || (race == RACE_DUERGAR) \
                                  || (race == RACE_GITHYANKI) \
                                  || (race == RACE_OGRE) \
                                  || (race == RACE_GOBLIN) \
                                  || (race == RACE_ORC) \
                                  || (race == RACE_OROG) \
                                  || (race == RACE_TROLL) \
	                                || (race == RACE_KUOTOA) \
	                                || (race == RACE_KOBOLD) \
	                                || (race == RACE_DRIDER) \
                                  || (OLD_RACE_NEUTRAL(race) && (align < 0)) )

#define OLD_RACE_PUNDEAD(race) ( (race == RACE_LICH) \
                              || (race == RACE_PVAMPIRE) \
                              || (race == RACE_PDKNIGHT) \
                              || (race == RACE_SHADE) \
                              || (race == RACE_REVENANT) \
                              || (race == RACE_PSBEAST) \
                              || (race == RACE_WIGHT) \
                              || (race == RACE_GARGOYLE) \
                              || (race == RACE_PHANTOM) )

#define IS_PUNDEAD(ch) (IS_UNDEAD(ch))

#define IS_NHARPY(ch) (IS_HARPY(ch) && (GET_RACEWAR(ch) == RACEWAR_NEUTRAL))

#define VOWEL(letter) ( index("aeiouAEIOU", letter) )
#define YESNO(boo) ((boo) ? "Yes" : "No")

/* this macro returns TRUE if the character is the only one in room.
   Designed to save tons of processor time when dealing with mobs.  If
   they are alone in room, we can skip bunches of things.  JAB */

#define ALONE(ch)  (!(ch) || ((ch)->in_room == NOWHERE) || \
        (((ch)->next_in_room == NULL) && (world[(ch)->in_room].people == (ch))))

#define IS_ENCRUSTED(obj) (IS_SET(obj->extra_flags, ITEM_ENCRUSTED))

#define WIZ_INVIS(viewer, target) (IS_TRUSTED(target) && (GET_LEVEL(viewer) <= target->only.pc->wiz_invis) \
 && !is_linked_to(viewer, target, LNK_CONSENT))

/* This is for a check to see if the the player/mob has the footing
   to do any actions, i.e. bash, etc, in certain sector types, such
   as SECT_UNDERWATER. -DR */
#define HAS_FOOTING(ch) \
 ((world[(ch)->in_room].sector_type != SECT_WATER_NOSWIM) && \
  (world[(ch)->in_room].sector_type != SECT_WATER_SWIM) && \
  (world[(ch)->in_room].sector_type != SECT_NO_GROUND) && \
  (world[(ch)->in_room].sector_type != SECT_UNDERWATER) && \
  (world[(ch)->in_room].sector_type != SECT_FIREPLANE) && \
  (world[(ch)->in_room].sector_type != SECT_OCEAN) && \
  (world[(ch)->in_room].sector_type != SECT_UNDRWLD_NOSWIM) && \
  (world[(ch)->in_room].sector_type != SECT_UNDRWLD_NOGROUND) && \
  (world[(ch)->in_room].sector_type != SECT_AIR_PLANE) && \
  (world[(ch)->in_room].sector_type != SECT_WATER_PLANE) && \
  (world[(ch)->in_room].sector_type != SECT_ETHEREAL) && \
  (world[(ch)->in_room].sector_type != SECT_ASTRAL) && \
  ((ch)->specials.z_cord == 0))

// Certain mobs are feared more than others. This define simply
// allows for quick use. -Lucrot Oct08

#define IS_GREATER_RACE(ch) (IS_NPC(ch) && \
                            ((GET_RACE(ch) == RACE_DRAGON) || \
                             (GET_RACE(ch) == RACE_DEMON) || \
                             (GET_RACE(ch) == RACE_DEVIL) || \
                             (GET_RACE(ch) == RACE_ANGEL) || \
                             (GET_RACE(ch) == RACE_BEHOLDER) || \
                             (GET_RACE(ch) == RACE_LICH) || \
                             (GET_RACE(ch) == RACE_AVATAR) || \
                             (GET_RACE(ch) == RACE_CONSTRUCT)))

#define SKILL_DATA(ch, skill)   (skills[(skill)].m_class[flag2idx((ch)->player.m_class)-1])
#define SKILL_DATA2(ch, skill)   (skills[(skill)].m_class[flag2idx((ch)->player.secondary_class)-1])

/* vt-100 code macros */

#define VT_CURSPOS    "\033[%d;%dH"         /* direct move */
#define VT_CURSRIG    "\033[%dC"    /* move right */
#define VT_CURSLEF    "\033[%dD"    /* move left */
#define VT_HOMECLR    "\033[2J\033[0;0H"  /* clear, go home */
#define VT_CTEOTCR    "\033[K"      /* clear to end of line */
#define VT_CLENSEQ    "\033[r\033[2J"
#define VT_INDUPSC    "\033M"     /* index? */
#define VT_INDDOSC    "\033D"     /* index? */
#define VT_HTAB       "\033H"     /* sets horizontal tab */
#define VT_SETSCRL    "\033[%d;%dr"   /* sets region of scroll */
#define VT_INVERTT    "\033[0;1;7m"   /* invert text */
#define VT_BOLDTEX    "\033[0;1m"   /* bold text */
#define VT_NORMALT    "\033[0m"
#define VT_MARGSET    "\033[%d;%dr"
#define VT_CURSAVE    "\033[s"      /* save cursor position */
#define VT_CURREST    "\033[u"      /* return to saved */
#define VT_REMAP      "\033[0;%d;%s:13p"  /* used exclusively for f-xx
                             keys here. int = key,
               string = command */
#if 0

         Other function key codes       F1=59,F2=60,F3=61,F4=62,F5=63
                                        F6=64,F7=65,F8=66,F9=67,F10=68
#endif

/* Combat related */

#define GET_ZONE(ch)  (world[ch->in_room].zone)
#define ROOM_ZONE_NUMBER(rnum) (zone_table[world[rnum].zone].number)

#define CRYPT(a,b) ((char *) crypt((a),(b)))

char *CRYPT2( char *passwd, char *name );

#define IS_BLIND(ch)  (IS_AFFECTED(ch, AFF_BLIND)) // || IS_DAYBLIND(ch))

#define IS_DAYBLIND(ch) (!has_innate(ch, INNATE_EYELESS) && \
                         !IS_TRUSTED(ch) && \
                         !IS_TWILIGHT_ROOM(ch->in_room) && \
                         !IS_AFFECTED4(ch, AFF4_GLOBE_OF_DARKNESS) && \
                          has_innate(ch, INNATE_DAYBLIND))

#define DISALLOW_GROUPED 1
#define DISALLOW_SELF    2
#define DISALLOW_BACKRANK 4
#define DISALLOW_UNGROUPED 8

#define PRSVCT_NOENG     BIT_1
#define PRSVCT_ENGFIRST  BIT_2

/* bunch of defines used by pick_target() function */

#define PT_WEAKEST    1
#define PT_CASTER     2
#define PT_STANDING   4
#define PT_FRONT      8
#define PT_SMALLER   16
#define PT_TRIP_SIZE 32
#define PT_BASH_SIZE 64
#define PT_TOLERANT 128
#define PT_SIZE_TOLERANT 256
#define PT_NUKETARGET 512

#define CAN_SING(ch) (!IS_IMMOBILE(ch) && !is_silent(ch, FALSE) && \
        !IS_STUNNED(ch) && GET_STAT(ch) >= STAT_RESTING)

#define IS_IMMOBILE(ch) (IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS) || \
       IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) || \
       IS_AFFECTED(ch, AFF_KNOCKED_OUT) || \
       IS_AFFECTED(ch, AFF_BOUND) || \
       affected_by_spell(ch, SONG_SLEEP) || \
       affected_by_spell(ch, SPELL_SLEEP))

// Meeting the following define grants hitpoints.spellcaster.maxConBonus.
#define MAX_CON_BONUS_CLASSES ( \
  CLASS_DRUID    | CLASS_BLIGHTER   | CLASS_CLERIC      | CLASS_SORCERER    | CLASS_NECROMANCER | \
  CLASS_SHAMAN   | CLASS_PSIONICIST | CLASS_MINDFLAYER  | CLASS_ILLUSIONIST | CLASS_CONJURER    | \
  CLASS_SUMMONER | CLASS_BARD       | CLASS_ETHERMANCER | CLASS_THEURGIST )

#define IS_COLD_VULN(ch) (GET_RACE(ch) == RACE_THRIKREEN || \
                          GET_RACE(ch) == RACE_F_ELEMENTAL || \
                          GET_RACE(ch) == RACE_EFREET || \
                          FIRESHIELDED(ch) || \
                          IS_AFFECTED2(ch, AFF2_FIRE_AURA))

#define ENJOYS_FIRE_DAM(ch) ((GET_RACE(ch) == RACE_F_ELEMENTAL || \
                              GET_RACE(ch) == RACE_EFREET) || \
                              IS_AFFECTED2(ch, AFF2_FIRE_AURA))

#define IS_AFFLICTED_PSI(ch) ((affected_by_spell(ch, SPELL_POISON) || \
            IS_AFFECTED2(ch, AFF2_POISONED) || \
            affected_by_spell(ch, SPELL_CURSE) || \
            (affected_by_spell(ch, SPELL_WITHER) && GET_LEVEL(ch) > 50) || \
            (affected_by_spell(ch, SPELL_DISEASE) && GET_LEVEL(ch) > 55) || \
            IS_AFFECTED(ch, AFF_BLIND)) && \
            GET_CLASS(ch, CLASS_MINDFLAYER | CLASS_PSIONICIST))

#define EYELESS(ch) (has_innate(ch, INNATE_EYELESS))

#define DO_SHAM_RESTORATION(victim) \
        (affected_by_spell(victim, SPELL_CURSE) || \
        affected_by_spell(victim, SPELL_MALISON) || \
        affected_by_spell(victim, SPELL_WITHER) || \
        affected_by_spell(victim, SPELL_BLOODTOSTONE) || \
        affected_by_spell(victim, SPELL_SHREWTAMENESS) || \
        affected_by_spell(victim, SPELL_MOUSESTRENGTH) || \
        affected_by_spell(victim, SPELL_MOLEVISION) || \
        affected_by_spell(victim, SPELL_SNAILSPEED) || \
        affected_by_spell(victim, SPELL_FEEBLEMIND) || \
        affected_by_spell(victim, SPELL_SLOW) || \
        IS_AFFECTED(victim, AFF_BLIND) || \
        IS_AFFECTED4(victim, AFF4_CARRY_PLAGUE) || \
        affected_by_spell(victim, SPELL_DISEASE) || \
        affected_by_spell(victim, SPELL_PLAGUE) || \
        affected_by_spell(victim, TAG_ARMLOCK) || \
        affected_by_spell(victim, TAG_LEGLOCK) || \
        affected_by_spell(victim, SPELL_BMANTLE) || \
        affected_by_spell(victim, SPELL_ENERGY_DRAIN))

// Simply change these defines to focus reaver preferred weapons.
// These defines are used for all the applicable reaver spells.
// Mar09 -Lucrot

#define SHOCK_REAVER_WEAPONS(wpn) \
        ((IS_SWORD(wpn) && !IS_SET(wpn->extra_flags, ITEM_TWOHANDS)) || \
        (IS_AXE(wpn) && !IS_SET(wpn->extra_flags, ITEM_TWOHANDS)) || \
        (IS_DAGGER(wpn) && !IS_SET(wpn->extra_flags, ITEM_TWOHANDS)))

#define EARTH_REAVER_WEAPONS(wpn) \
        ( (getWeaponDamType( wpn->value[0] ) == WEAPONTYPE_BLUDGEON) && \
        !IS_SET(wpn->extra_flags, ITEM_TWOHANDS) )

#define FLAME_REAVER_WEAPONS(wpn) \
        (IS_SWORD(wpn) || \
        (wpn)->value[0] == WEAPON_FLAIL | WEAPON_WHIP)

#define FROST_REAVER_WEAPONS(wpn) \
        (IS_BLUDGEON(wpn))

// Can't seem to find this defined anywhere else.
#define IS_MELEE_CLASS(ch) \
        (GET_CLASS(ch, CLASS_WARRIOR | CLASS_PALADIN | CLASS_ROGUE | CLASS_MERCENARY | CLASS_DREADLORD | CLASS_REAVER | CLASS_AVENGER | CLASS_ANTIPALADIN | CLASS_RANGER | CLASS_MONK))

        
#define SHAMAN_MOVE_SPELL(ch) \
      (affected_by_spell(ch, SPELL_WOLFSPEED) || \
      affected_by_spell(ch, SPELL_SNAILSPEED) || \
      affected_by_spell(ch, SPELL_PANTHERSPEED))
 
#define SHAMAN_STR_SPELL(ch) \
    (affected_by_spell(ch, SPELL_MOUSESTRENGTH) || \
    affected_by_spell(ch, SPELL_BEARSTRENGTH) || \
    affected_by_spell(ch, SPELL_ELEPHANTSTRENGTH))

#define IS_GREATER_ELEMENTAL(mob) \
    (IS_NPC(mob) && \
    (GET_VNUM(mob) >= 1110 && GET_VNUM(mob) <= 1112 || \
    GET_VNUM(mob) >= 1120 && GET_VNUM(mob) <= 1122 || \
    GET_VNUM(mob) >= 1130 && GET_VNUM(mob) <= 1132 || \
    GET_VNUM(mob) >= 1140 && GET_VNUM(mob) <=1142 || \
    GET_VNUM(mob) >= 41 && GET_VNUM(mob) <= 44))
    
#define IS_ANIMALIST(ch) (GET_SPEC(ch, CLASS_SHAMAN, SPEC_ANIMALIST))
#define IS_SPIRITUALIST(ch) (GET_SPEC(ch, CLASS_SHAMAN, SPEC_SPIRITUALIST))
#define IS_ELEMENTALIST(ch) (GET_SPEC(ch, CLASS_SHAMAN, SPEC_ELEMENTALIST))

#define IS_BRAINLESS(victim) ((GET_RACE(victim) == RACE_PLANT) || \
          (GET_RACE(victim) == RACE_GOLEM) || \
          (GET_RACE(victim) == RACE_CONSTRUCT) || \
          (GET_RACE(victim) == RACE_SLIME) || \
          (GET_RACE(victim) == RACE_PWORM) || \
          (GET_RACE(victim) == RACE_SKELETON) ||\
          (GET_RACE(victim) == RACE_ZOMBIE) || \
          (IS_NPC(victim) && GET_VNUM(victim) == 250))

#define IS_SOULLESS(victim) ((GET_RACE(victim) == RACE_PLANT) || \
                        (GET_RACE(victim) == RACE_GOLEM) || \
                        IS_UNDEADRACE(victim) || \
                        (GET_RACE(victim) == RACE_CONSTRUCT) || \
                        (GET_RACE(victim) == RACE_SLIME))

#define IS_CONSTRUCT(ch) ((GET_RACE(ch) == RACE_GOLEM) || \
                          (GET_RACE(ch) == RACE_CONSTRUCT))

#define IS_NECRO_GOLEM(ch) IS_NPC(ch) && (GET_RACE(ch) == RACE_GOLEM) \
                      && ( GET_VNUM(ch) == golem_data[0].vnum || \
                           GET_VNUM(ch) == golem_data[1].vnum || \
                           GET_VNUM(ch) == golem_data[2].vnum || \
                           GET_VNUM(ch) == golem_data[3].vnum )

#define IS_ANGEL(ch) ((GET_RACE(ch) == RACE_ANGEL) || \
                      (GET_RACE(ch) == RACE_ELADRIN) || \
                      (GET_RACE(ch) == RACE_AGATHINON) || \
                      (GET_RACE(ch) == RACE_ARCHON) || \
                      (GET_RACE(ch) == RACE_ASURA) || \
                      (GET_RACE(ch) == RACE_TITAN) || \
                      (GET_RACE(ch) == RACE_AVATAR) || \
                      (GET_RACE(ch) == RACE_GHAELE) || \
                      (GET_RACE(ch) == RACE_BRALANI) || \
                      (GET_RACE(ch) == RACE_DEVA) || \
                       IS_ANGELIC(ch))

#define INFRA_INVIS_RACE(race)   (race == RACE_UNDEAD      || race == RACE_GHOST       || race == RACE_VAMPIRE       \
  || race == RACE_LICH         || race == RACE_PDKNIGHT    || race == RACE_ZOMBIE      || race == RACE_SPECTRE       \
  || race == RACE_SKELETON     || race == RACE_WRAITH      || race == RACE_SHADOW      || race == RACE_DRACOLICH     \
  || race == RACE_PVAMPIRE     || race == RACE_SHADE       || race == RACE_REVENANT    || race == RACE_PSBEAST       \
  || race == RACE_WIGHT        || race == RACE_GARGOYLE    || race == RACE_PHANTOM     || race == RACE_AQUATIC_ANIMAL\
  || race == RACE_PARASITE     || race == RACE_GOLEM       || race == RACE_SNAKE       || race == RACE_ARACHNID      \
  || race == RACE_PLANT        || race == RACE_SLIME       || race == RACE_A_ELEMENTAL || race == RACE_INSECT        \
  || race == RACE_W_ELEMENTAL  || race == RACE_V_ELEMENTAL || race == RACE_I_ELEMENTAL || race == RACE_E_ELEMENTAL   \
  || (IS_AFFECTED4(obj, AFF4_VAMPIRE_FORM) && !GET_CLASS(obj, CLASS_THEURGIST)) || IS_RACEWAR_UNDEAD(obj) )

// This is done in reverse 'cause it's faster that way.
#define HAS_LUNGS(race)     ( race != RACE_SHADE     && race != RACE_REVENANT    && race != RACE_LICH       \
  && race != RACE_PVAMPIRE && race != RACE_PDKNIGHT  && race != RACE_PSBEAST     && race != RACE_WIGHT       \
  && race != RACE_PHANTOM  && race != RACE_GARGOYLE  && race != RACE_F_ELEMENTAL && race != RACE_A_ELEMENTAL \
  && race != RACE_UNDEAD   && race != RACE_VAMPIRE   && race != RACE_W_ELEMENTAL && race != RACE_E_ELEMENTAL \
  && race != RACE_GHOST    && race != RACE_GOLEM     && race != RACE_PLANT       && race != RACE_DRACOLICH   \
  && race != RACE_SLIME    && race != RACE_CONSTRUCT && race != RACE_ZOMBIE      && race != RACE_SPECTRE     \
  && race != RACE_SKELETON && race != RACE_WRAITH    && race != RACE_V_ELEMENTAL && race != RACE_I_ELEMENTAL \
  && race != RACE_SHADOW   && race != RACE_AVATAR )

#define CAN_HEAR(ch) ( !IS_AFFECTED4(ch, AFF4_DEAF) && (GET_STAT(ch) > STAT_SLEEPING) )

// Return values for coin_type(char *)
#define COIN_NONE    -1
#define COIN_COPPER   0
#define COIN_SILVER   1
#define COIN_GOLD     2
#define COIN_PLATINUM 3

#define GROUNDFIGHTING_CHECK(ch) (notch_skill(ch, SKILL_GROUNDFIGHTING, get_property("skill.notch.offensive", 7)) \
    || GET_CHAR_SKILL(ch, SKILL_GROUNDFIGHTING) >= number(1, 100) )

#define GET_FRAGS(ch) ((ch)->only.pc->frags)

// Good spell pulse is negative.
#define SPELL_PULSE(ch) (1.0 + (.03 * ch->points.spell_pulse))
// Derived this via 1.3 for slow, 1.0 for normal and .5 for fast pulsers:
//   Base + Modifier * ( (.08/3) Base^2 - (2.20/3) Base + 5.46 )
#define COMBAT_PULSE(ch) (ch->specials.base_combat_round \
  + ch->points.combat_pulse * ( (.04/3) * ch->specials.base_combat_round * ch->specials.base_combat_round \
  - (1.10/3) * ch->specials.base_combat_round + 2.73 ))

// New effects of attributes:
// Vamp multiplier for ch: value between 1.1 and 2.2.
#define VAMPPERCENT(ch) (BOUNDEDF(1.10, (GET_C_POW(ch) / 90.0), 2.20) \
  + (GET_PRIME_CLASS(ch, CLASS_ANTIPALADIN) ? .15 : 0) )
// Crit rate for ch: value between 8 and 100+ (if you get int to 560+).
#define CRITRATE(ch) ((GET_C_INT(ch) < 105) ? 8 : (GET_C_INT(ch) - 100)/5 + 8)
// Calming chance for ch: value between 1 and 100+ (if you get cha to 400+).
#define CALMCHANCE(ch) (GET_C_CHA(ch) / ((GET_C_CHA(ch) > 160) ? 4 : (GET_C_CHA(ch) < 80) ? 9 : 7))
// Magic Resistance for ch: value between 0 and 75 (75 at wisdom of 260).
#define MAGICRES(ch) (BOUNDED( 0, (GET_C_WIS(ch) - 110)/2, 75))
// Damage bonus to offensive magic for ch: value between 100 and 130% (130 at strength of 181).
#define MAGICDAMBONUS(ch) ((GET_C_STR(ch) < 121) ? 100 : (GET_C_STR(ch) < 141) ? 110 : (GET_C_STR(ch) < 181) ? 120 : 130)

// Quest items and containers (maybe containing quest items) have 100% load.
#define ITEM_LOAD_CHECK(item, ival, zone_percent) ( (IS_OBJ_STAT2( item, ITEM2_QUESTITEM ) \
  || ( item->type == ITEM_CONTAINER )) ? zone_percent : item_load_check(item, ival, zone_percent) )
bool item_load_check( P_obj item, int ival, int zone_percent );

#endif /* _DURIS_UTILS_H_ */
