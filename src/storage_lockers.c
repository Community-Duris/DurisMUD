//
// C Implementation: storage_lockers
//
// Description:
//
//
// Author: Gary Dezern <gdezern@comcast.net>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//


#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "ships.h"
#include "mm.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "events.h"
#include "assocs.h"
#include "justice.h"
#include "storage_lockers.h"
#include "sql.h"
#include "specs.winterhaven.h"
#include "ctf.h"
#include "vnum.room.h"

extern P_index obj_index;
extern P_index mob_index;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern P_event current_event;
extern P_event event_list;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern P_char character_list;
extern struct zone_data *zone_table;

extern P_nevent get_scheduled(P_char ch, event_func func);
void event_memorize(P_char, P_char, P_obj, void *);
int is_wearing_necroplasm(P_char);

#define LOCKERS_START           65201
#define LOCKERS_MAX             99
// for mini_mode
//#define LOCKERS_START           1285
//#define LOCKERS_MAX             2

#define LOCKERS_SECT_TYPE       SECT_INSIDE
#define LOCKERS_ROOMFLAGS       (ROOM_NO_MOB | ROOM_NO_TRACK | ROOM_INDOORS | ROOM_SILENT | ROOM_NO_RECALL   \
  | ROOM_NO_MAGIC | ROOM_NO_TELEPORT | ROOM_LOCKER | ROOM_SAFE | ROOM_NO_HEAL | ROOM_NO_SUMMON | ROOM_NO_PSI \
  | ROOM_NO_GATE | ROOM_BLOCKS_SIGHT)

#define LOCKERS_DOORSIGN        "The door has a sign with the current occupant's name on it: "


inline StorageLocker *GetChestList(int real_room)
{
  StorageLocker *pRet = NULL;

  if ((world[real_room].ex_description) &&
      (world[real_room].ex_description->next) &&
      (world[real_room].ex_description->next->keyword))
  {
    if (sscanf(world[real_room].ex_description->next->keyword, "%p", &pRet) != 1)
      pRet = NULL;
  }
  return pRet;
}
const unsigned LockerChest::m_chestVnum = 173;

StorageLocker::StorageLocker(int rroom, P_char chLocker, P_char chUser)
              : m_realRoom(rroom),
                m_chLocker(chLocker),
                m_chUser(chUser),
                m_pChestList(NULL),
                m_itemCount(0),
                m_bIValue(false)
{
  if (!world[rroom].ex_description)
  {
    CREATE(world[rroom].ex_description, extra_descr_data, 1, MEM_TAG_EXDESCD);
    CREATE(world[rroom].ex_description->next, extra_descr_data, 1, MEM_TAG_EXDESCD)
  }
  char     buf[500];

  strcpy(buf, GET_NAME(chLocker));
  world[rroom].ex_description->keyword = str_dup(buf);
  world[rroom].ex_description->description = NULL;
  snprintf(buf, 500, "%p", (void *) this);
  world[rroom].ex_description->next->keyword = str_dup(buf);
  world[rroom].ex_description->next->description = NULL;
  world[rroom].ex_description->next->next = NULL;
};


StorageLocker::~StorageLocker(void)
{
  NukeLockerChests();
  if ((world[m_realRoom].ex_description) &&
      (world[m_realRoom].ex_description->keyword))
  {
    str_free(world[m_realRoom].ex_description->keyword);
    world[m_realRoom].ex_description->keyword = NULL;
  }
  if ((world[m_realRoom].ex_description) &&
      (world[m_realRoom].ex_description->next) &&
      (world[m_realRoom].ex_description->next->keyword))
  {
    str_free(world[m_realRoom].ex_description->next->keyword);
    world[m_realRoom].ex_description->next->keyword = NULL;
  }

//  free_char(m_chLocker);
}

void StorageLocker::NukeLockerChests(void)
{
  LockerChest *p;

  while (m_pChestList)
  {
    p = m_pChestList;
    m_pChestList = p->m_pNextInChain;
    delete p;
  }
}

bool StorageLocker::PutInProperChest(P_obj obj)
{
  // assumes 'obj' is located in nowhere
  LockerChest *p = m_pChestList;

  while (p)
  {
    if (p->ItemFits(obj))
    {
      obj_to_obj(obj, p->GetChestObj());
      return true;
    }
    p = p->m_pNextInChain;
  }
  return false;
};

LockerChest *StorageLocker::AddLockerChest(LockerChest * p)
{
  if (p)
  {
    // update:  need to add it to the END of the list!
    if (!m_pChestList)
      m_pChestList = p;
    else
    {
      LockerChest *next = m_pChestList;

      while (next->m_pNextInChain)
        next = next->m_pNextInChain;
      next->m_pNextInChain = p;
    }
  }
  return p;
}

#define LOCKER_HELP_NONE            0
#define LOCKER_HELP_SHORT       BIT_1
#define LOCKER_HELP_LONG        BIT_2
#define LOCKER_HELP_ALL_SHORT   BIT_3
#define LOCKER_HELP_ALL_LONG    BIT_4
bool StorageLocker::MakeChests(P_char ch, char *args)
{
  bool bRet = true;
  bool bIsCustomChest = false;
  int  helpMode = LOCKER_HELP_NONE;
  char GBuf1[MAX_STRING_LENGTH];
  char GBuf2[MAX_STRING_LENGTH], *tmp;

  GBuf1[0] = GBuf2[0] = '\0';

  // Change held to hold (every instance but first).
  while ((tmp = strstr(args, " held")))
  {
    tmp[2] = 'o';
  }
  if( !strncmp(args, "held ", 5) )
  {
    args[1] = 'o';
  }

  args = one_argument(args, GBuf1);
  while( *args == ' ' )
  {
    args++;
  }
  if( ('\0' == GBuf1[0]) || !str_cmp(GBuf1, "help") || !str_cmp(GBuf1, "?") )
  {
    args = one_argument(args, GBuf1);
    if( !str_cmp(GBuf1, "all") )
    {
      args = one_argument(args, GBuf1);
      if( !str_cmp(GBuf1, "long") )
      {
        helpMode = LOCKER_HELP_ALL_LONG;
      }
      else
      {
        helpMode = LOCKER_HELP_ALL_SHORT;
      }
      GBuf1[0] = '\0';
      args = GBuf1;
    }
    else if( !str_cmp(GBuf1, "long") )
    {
      helpMode = LOCKER_HELP_LONG;
      args = one_argument(args, GBuf1);
      if( GBuf1[0] == '\0' || !str_cmp(GBuf1, "all") )
      {
        helpMode = LOCKER_HELP_ALL_LONG;
        GBuf1[0] = '\0';
        args = GBuf1;
      }
    }
    else
    {
      if( !str_cmp(GBuf1, "short") )
      {
        args = one_argument(args, GBuf1);
        if( GBuf1[0] == '\0' || !str_cmp(GBuf1, "all") )
        {
          helpMode = LOCKER_HELP_ALL_SHORT;
          GBuf1[0] = '\0';
          args = GBuf1;
        }
        else
        {
          helpMode = LOCKER_HELP_SHORT;
        }
      }
      else if( GBuf1[0] == '\0' )
      {
        helpMode = LOCKER_HELP_ALL_SHORT;
      }
      // equip sort ? {type}*
      else
      {
        helpMode = LOCKER_HELP_LONG;
      }
    }
  }
  else
  {
    if (!str_cmp("none", GBuf1))
      strcpy(GBuf1, "unsorted");

    // check for 'custom' chest type next...
    if (!str_cmp("custom", GBuf1))
    {
      // deal with it...
      bIsCustomChest = true;
      args = one_argument(args, GBuf1);
      if( ('\0' == GBuf1[0]) )
        helpMode = LOCKER_HELP_ALL_SHORT;
    }
    // eq sort custom might set help mode... and don't want to nuke existing chests on help mode
    if( helpMode == LOCKER_HELP_NONE )
      NukeLockerChests();
  }
  if( helpMode != LOCKER_HELP_NONE )
  {
    bRet = false;
    strcpy(GBuf2, "Usage:  \r\n"
      "  For help:              equip sort {help|?} [short|long] [all|type1] [type2] [type3] ...\r\n"
      "  To Remove all sorting: equip sort none\r\n"
      "  Multiple chests:       equip sort type1 [type2] [type3] ...\r\n"
      "  A custom sort chest:   equip sort custom type1 type2 [type3] ...\r\n"
      "\r\nValid types are:");
    // Long mode -> newline, short mode = 3 spaces (2 replaced with ".\n" if no valid args in reg short mode).
    strcat(GBuf2, (helpMode == LOCKER_HELP_LONG || helpMode == LOCKER_HELP_ALL_LONG) ? "\r\n" : "   ");
  }

  // hold and attach are special case chests.. being that damn near every item in the
  // game is holdable, and quite a few are attachable, force them to always be created
  // last...  Do this by setting a couple bools for them...
  bool bMakeHoldable = false;
  bool bMakeAttachable = false;

  const char *chestKeyword;
  const char *chestDesc;

#define IF_ISLOCKERTYPE(keyword, desc)                                                \
                            chestKeyword = (keyword);                                 \
                            chestDesc = (desc);                                       \
                            if( helpMode == LOCKER_HELP_ALL_LONG )                    \
                            {                                                         \
                              bFound = true;                                          \
                              snprintf(GBuf2 + strlen(GBuf2), MAX_STRING_LENGTH - strlen(GBuf2), "&+C%15s&n   &+w(items %s)\r\n", keyword, desc); \
                            }                                                         \
                            else if( helpMode == LOCKER_HELP_ALL_SHORT )              \
                            {                                                         \
                              bFound = true;                                          \
                              snprintf(GBuf2 + strlen(GBuf2), MAX_STRING_LENGTH - strlen(GBuf2), "&+C%s&n, ", keyword);   \
                            }                                                         \
                            else if (!str_cmp(keyword, GBuf1))                        \
                              if (isname(keyword, args))                              \
                                bFound = true;                                        \
                              else if( helpMode == LOCKER_HELP_LONG )                 \
                              {                                                       \
                                bFound = true;                                        \
                                snprintf(GBuf2 + strlen(GBuf2), MAX_STRING_LENGTH - strlen(GBuf2), "&+C%15s&n   &+w(items %s)\r\n", keyword, desc); \
                              }                                                       \
                              else if( helpMode == LOCKER_HELP_SHORT )                \
                              {                                                       \
                                bFound = true;                                        \
                                snprintf(GBuf2 + strlen(GBuf2), MAX_STRING_LENGTH - strlen(GBuf2), "&+C%s&n, ", keyword); \
                              }                                                       \
                              else

  if( helpMode != LOCKER_HELP_NONE || ('\0' != GBuf1[0]) )
  {
    bool bFound;
    LockerChest *p;
    do
    {
      bFound = FALSE;
      p = NULL;

      // parse 'GBuf1'
      IF_ISLOCKERTYPE("hold", "that you can hold")
        bFound = bMakeHoldable = true;
      IF_ISLOCKERTYPE("attach", "attached to belt")
        bFound = bMakeAttachable = true;
      IF_ISLOCKERTYPE("ivalue", "in chest sorted by item value")
        bFound = m_bIValue = true;

      IF_ISLOCKERTYPE("horns", "worn on body")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_HORN, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("nose", "worn on nose")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_NOSE, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("tail", "worn on tail")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_TAIL, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("horse", "worn on a horses body")
        p = AddLockerChest(new EqSlotChest(ITEM_HORSE_BODY, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("spider", "worn on a spider body")
        p = AddLockerChest(new EqSlotChest(ITEM_SPIDER_BODY, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("back", "worn on back")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_BACK, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("badge", "worn as a badge")
        p = AddLockerChest(new EqSlotChest(ITEM_GUILD_INSIGNIA, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("quiver", "worn as a quiver")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_QUIVER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("ear", "worn on or in ear")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_EARRING, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("face", "worn on face")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_FACE, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("eyes", "worn on or over eyes")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_EYES, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("wield", "used as a weapon or wielded")
        p = AddLockerChest(new EqSlotChest(ITEM_WIELD, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("wrist", "worn around wrist")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_WRIST, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("waist", "worn about waist")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_WAIST, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("about", "worn about body")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_ABOUT, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("shield", "worn as a shield")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_SHIELD, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("arms", "worn on arms")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_ARMS, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("hands", "worn on hands")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_HANDS, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("feet", "worn on feet")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_FEET, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("legs", "worn on legs")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_LEGS, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("head", "worn on head")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_HEAD, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("body", "worn on body")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_BODY, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("neck", "worn around neck")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_NECK, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("finger", "worn on finger")
        p = AddLockerChest(new EqSlotChest(ITEM_WEAR_FINGER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("warrior", "usable by a warrior")
        p = AddLockerChest(new EqWearChest(CLASS_WARRIOR, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("ranger", "usable by a ranger")
        p = AddLockerChest(new EqWearChest(CLASS_RANGER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("psionicist", "usable by a psionicist")
        p = AddLockerChest(new EqWearChest(CLASS_PSIONICIST, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("paladin", "usable by a paladin")
        p = AddLockerChest(new EqWearChest(CLASS_PALADIN, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("antipaladin", "usable by an antipaladin")
        p = AddLockerChest(new EqWearChest(CLASS_ANTIPALADIN, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("cleric", "usable by a cleric")
        p = AddLockerChest(new EqWearChest(CLASS_CLERIC, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("monk", "usable by a monk")
        p = AddLockerChest(new EqWearChest(CLASS_MONK, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("druid", "usable by a druid")
        p = AddLockerChest(new EqWearChest(CLASS_DRUID, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("shaman", "usable by a shaman")
        p = AddLockerChest(new EqWearChest(CLASS_SHAMAN, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("sorcerer", "usable by a sorcerer")
        p = AddLockerChest(new EqWearChest(CLASS_SORCERER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("necromancer", "usable by a necromancer")
        p = AddLockerChest(new EqWearChest(CLASS_NECROMANCER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("conjurer", "usable by a conjurer")
        p = AddLockerChest(new EqWearChest(CLASS_CONJURER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("rogue", "usable by a rogue")
        p = AddLockerChest(new EqWearChest(CLASS_ROGUE, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("summoner", "usable by a summoner")
        p = AddLockerChest(new EqWearChest(CLASS_SUMMONER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("assassin", "usable by an assassin")
        p = AddLockerChest(new EqWearChest(CLASS_ASSASSIN, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mercenary", "usable by a mercenary")
        p = AddLockerChest(new EqWearChest(CLASS_MERCENARY, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("bard", "usable by a bard")
        p = AddLockerChest(new EqWearChest(CLASS_BARD, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("thief", "usable by a thief")
        p = AddLockerChest(new EqWearChest(CLASS_THIEF, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("alchemist", "usable by an alchemist")
        p = AddLockerChest(new EqWearChest(CLASS_ALCHEMIST, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("berserker", "usable by a berserker")
        p = AddLockerChest(new EqWearChest(CLASS_BERSERKER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("reaver", "usable by a reaver")
        p = AddLockerChest(new EqWearChest(CLASS_REAVER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("illusionist", "usable by an illusionist")
        p = AddLockerChest(new EqWearChest(CLASS_ILLUSIONIST, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("dreadlord", "usable by a dreadlord")
        p = AddLockerChest(new EqWearChest(CLASS_DREADLORD, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("ethermancer", "usable by an ethermancer")
        p = AddLockerChest(new EqWearChest(CLASS_ETHERMANCER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("blighter", "usable by a blighter")
        p = AddLockerChest(new EqWearChest(CLASS_BLIGHTER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("totems", "used as totems")
        p = AddLockerChest(new EqTypeChest(ITEM_TOTEM, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("instruments", "playable as bard instruments")
        p = AddLockerChest(new EqTypeChest(ITEM_INSTRUMENT, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("potions", "quaffable or used as potions")
        p = AddLockerChest(new EqTypeChest(ITEM_POTION, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("spellbooks", "used as spellbooks")
        p = AddLockerChest(new EqTypeChest(ITEM_SPELLBOOK, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("scrolls", "used as scrolls")
        p = AddLockerChest(new EqTypeChest(ITEM_SCROLL, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("containers", "that are also containers")
        p = AddLockerChest(new EqTypeChest(ITEM_CONTAINER, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("hitpoints", "that affect hitpoints")
        p = AddLockerChest(new EqApplyChest(APPLY_HIT, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mana", "that affect mana")
        p = AddLockerChest(new EqApplyChest(APPLY_MANA, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("moves", "that affect moves")
        p = AddLockerChest(new EqApplyChest(APPLY_MOVE, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("hitroll", "that affect hitroll")
        p = AddLockerChest(new EqApplyChest(APPLY_HITROLL, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("damroll", "that affect damroll")
        p = AddLockerChest(new EqApplyChest(APPLY_DAMROLL, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("save_para", "that affect save_para")
        p = AddLockerChest(new EqApplyChest(APPLY_SAVING_PARA, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("save_rod", "that affect save_rod")
        p = AddLockerChest(new EqApplyChest(APPLY_SAVING_ROD, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("save_fear", "that affect save_fear")
        p = AddLockerChest(new EqApplyChest(APPLY_SAVING_FEAR, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("save_breath", "that affect save_breath")
        p = AddLockerChest(new EqApplyChest(APPLY_SAVING_BREATH, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("save_spell", "that affect save_spell")
        p = AddLockerChest(new EqApplyChest(APPLY_SAVING_SPELL, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("str", "that affect strength")
        p = AddLockerChest(new EqApplyChest(APPLY_STR, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("dex", "that affect dexterity")
        p = AddLockerChest(new EqApplyChest(APPLY_DEX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("int", "that affect intelligence")
        p = AddLockerChest(new EqApplyChest(APPLY_INT, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("wis", "that affect wisdom")
        p = AddLockerChest(new EqApplyChest(APPLY_WIS, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("con", "that affect constitution")
        p = AddLockerChest(new EqApplyChest(APPLY_CON, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("agi", "that affect agility")
        p = AddLockerChest(new EqApplyChest(APPLY_AGI, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("pow", "that affect power")
        p = AddLockerChest(new EqApplyChest(APPLY_POW, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("cha", "that affect charisma")
        p = AddLockerChest(new EqApplyChest(APPLY_CHA, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("luck", "that affect luck")
        p = AddLockerChest(new EqApplyChest(APPLY_LUCK, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("karma", "that affect karma")
        p = AddLockerChest(new EqApplyChest(APPLY_KARMA, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("str_max", "that affect maximum strength (str_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_STR_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("dex_max", "that affect maximum dexterity (dex_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_DEX_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("int_max", "that affect maximum intelligence (int_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_INT_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("wis_max", "that affect maximum wisdom (wis_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_WIS_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("con_max", "that affect maximum constitution (con_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_CON_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("agi_max", "that affect maximum agility (agi_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_AGI_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("pow_max", "that affect maximum power (pow_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_POW_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("cha_max", "that affect maximum charisma (cha_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_CHA_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("luck_max", "that affect maximum luck (luck_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_LUCK_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("karma_max", "that affect maximum karma (karma_max)")
        p = AddLockerChest(new EqApplyChest(APPLY_KARMA_MAX, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("ac", "armor class")
        p = AddLockerChest(new EqApplyChest(APPLY_AC, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mana_reg", "that affect mana regeneration")
        p = AddLockerChest(new EqApplyChest(APPLY_MANA_REG, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("move_reg", "that affect movement regeneration")
        p = AddLockerChest(new EqApplyChest(APPLY_MOVE_REG, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("hit_reg", "that affect hitpoint regeneration")
        p = AddLockerChest(new EqApplyChest(APPLY_HIT_REG, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("spell_pulse", "that affect spell_pulse (negative is better)")
        p = AddLockerChest(new EqApplyChest(APPLY_SPELL_PULSE, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("combat_pulse", "that affect combat_pulse (negative is better)")
        p = AddLockerChest(new EqApplyChest(APPLY_COMBAT_PULSE, chestKeyword, chestDesc));

      // Mine related.
      IF_ISLOCKERTYPE("mine", "that are ore from a mine")
        p = AddLockerChest(new OreChest(NULL, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mine_adamantium", "that are adamantium ore from a mine (mine_adamantium)")
        p = AddLockerChest(new OreChest("adamantium", chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mine_mithril", "that are mithril ore from a mine (mine_mithril)")
        p = AddLockerChest(new OreChest("mithril", chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mine_platinum", "that are platinum ore from a mine (mine_platinum)")
        p = AddLockerChest(new OreChest("platinum", chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mine_gold", "that are gold ore from a mine (mine_gold)")
        p = AddLockerChest(new OreChest("gold", chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mine_silver", "that are silver ore from a mine (mine_silver)")
        p = AddLockerChest(new OreChest("silver", chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mine_copper", "that are copper ore from a mine (mine_copper)")
        p = AddLockerChest(new OreChest("copper", chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mine_tin", "that are iron ore from a mine (mine_tin)")
        p = AddLockerChest(new OreChest("tin", chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("mine_iron", "that are iron ore from a mine (mine_iron)")
        p = AddLockerChest(new OreChest("iron", chestKeyword, chestDesc));

      IF_ISLOCKERTYPE("invis", "that provide you with invisibility")
        p = AddLockerChest(new EqAffectChest(AFF_INVISIBLE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("farsee", "that provide you with farsee")
        p = AddLockerChest(new EqAffectChest(AFF_FARSEE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("det_invis", "that provide you with detect invisibile")
        p = AddLockerChest(new EqAffectChest(AFF_DETECT_INVISIBLE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("haste", "that provide you with haste")
        p = AddLockerChest(new EqAffectChest(AFF_HASTE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("sense_life", "that provide you with sense life")
        p = AddLockerChest(new EqAffectChest(AFF_SENSE_LIFE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("minor_globe", "that provide you with minor globe of invulnerability")
        p = AddLockerChest(new EqAffectChest(AFF_MINOR_GLOBE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("stone_skin", "that provide you with stone skin")
        p = AddLockerChest(new EqAffectChest(AFF_STONE_SKIN, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("waterbreath", "that provide you with waterbreath")
        p = AddLockerChest(new EqAffectChest(AFF_WATERBREATH, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_evil", "that provide you with protection from evil")
        p = AddLockerChest(new EqAffectChest(AFF_PROTECT_EVIL, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("slow_poison", "that provide you with slow poison")
        p = AddLockerChest(new EqAffectChest(AFF_SLOW_POISON, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_good", "that provide you with protection from good")
        p = AddLockerChest(new EqAffectChest(AFF_PROTECT_GOOD, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("sneak", "that provide you with sneak")
        p = AddLockerChest(new EqAffectChest(AFF_SNEAK, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("barkskin", "that provide you with barkskin")
        p = AddLockerChest(new EqAffectChest(AFF_BARKSKIN, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("infravision", "that provide you with infravision")
        p = AddLockerChest(new EqAffectChest(AFF_INFRAVISION, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("levitate", "that provide you with levitation")
        p = AddLockerChest(new EqAffectChest(AFF_LEVITATE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("fly", "that provide you with fly")
        p = AddLockerChest(new EqAffectChest(AFF_FLY, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("aware", "that provide you with awareness")
        p = AddLockerChest(new EqAffectChest(AFF_AWARE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_fire", "that provide you with protection from fire")
        p = AddLockerChest(new EqAffectChest(AFF_PROT_FIRE, 1, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("biofeedback", "that provide you with biofeedback")
        p = AddLockerChest(new EqAffectChest(AFF_BIOFEEDBACK, 1, chestKeyword, chestDesc));

      IF_ISLOCKERTYPE("fireshield", "that provide you with fireshield")
        p = AddLockerChest(new EqAffectChest(AFF2_FIRESHIELD, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("ultravision", "that provide you with ultravision")
        p = AddLockerChest(new EqAffectChest(AFF2_ULTRAVISION, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("det_evil", "that provide you with detect evil")
        p = AddLockerChest(new EqAffectChest(AFF2_DETECT_EVIL, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("det_good", "that provide you with detect good")
        p = AddLockerChest(new EqAffectChest(AFF2_DETECT_GOOD, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("det_magic", "that provide you with detect magic")
        p = AddLockerChest(new EqAffectChest(AFF2_DETECT_MAGIC, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_cold", "that provide you with protection from cold")
        p = AddLockerChest(new EqAffectChest(AFF2_PROT_COLD, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_light", "that provide you with protection from lightning")
        p = AddLockerChest(new EqAffectChest(AFF2_PROT_LIGHTNING, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("globe", "that provide you with globe of invulnerability")
        p = AddLockerChest(new EqAffectChest(AFF2_GLOBE, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_gas", "that provide you with protection from gas")
        p = AddLockerChest(new EqAffectChest(AFF2_PROT_GAS, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_acid", "that provide you with protection from acid")
        p = AddLockerChest(new EqAffectChest(AFF2_PROT_ACID, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("soulshield", "that provide you with soulshield")
        p = AddLockerChest(new EqAffectChest(AFF2_SOULSHIELD, 2, chestKeyword, chestDesc));

      IF_ISLOCKERTYPE("ecto_form", "that provide you with ectoplasmic form")
        p = AddLockerChest(new EqAffectChest(AFF3_ECTOPLASMIC_FORM, 3, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_animal", "that provide you with protection from animals")
        p = AddLockerChest(new EqAffectChest(AFF3_PROT_ANIMAL, 3, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("spirit_ward", "that provide you with spirit ward")
        p = AddLockerChest(new EqAffectChest(AFF3_SPIRIT_WARD, 3, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("gsw", "that provide you with greater spirit ward")
        p = AddLockerChest(new EqAffectChest(AFF3_GR_SPIRIT_WARD, 3, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("inert_barrier", "that provide you with inertial barrier")
        p = AddLockerChest(new EqAffectChest(AFF3_INERTIAL_BARRIER, 3, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("light_shield", "that provide you with lightning shield")
        p = AddLockerChest(new EqAffectChest(AFF3_LIGHTNINGSHIELD, 3, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("coldshield", "that provide you with coldshield")
        p = AddLockerChest(new EqAffectChest(AFF3_COLDSHIELD, 3, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("blur", "that provide you with blur")
        p = AddLockerChest(new EqAffectChest(AFF3_BLUR, 3, chestKeyword, chestDesc));

      IF_ISLOCKERTYPE("nofear", "that make you fearless")
        p = AddLockerChest(new EqAffectChest(AFF4_NOFEAR, 4, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("regeneration", "that provide you with regeneration")
        p = AddLockerChest(new EqAffectChest(AFF4_REGENERATION, 4, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("hawkvision", "that provide you with hawkvision")
        p = AddLockerChest(new EqAffectChest(AFF4_HAWKVISION, 4, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("sense_holy", "that provide you with sense holyness")
        p = AddLockerChest(new EqAffectChest(AFF4_SENSE_HOLINESS, 4, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("prot_living", "that provide you with protection from living")
        p = AddLockerChest(new EqAffectChest(AFF4_PROT_LIVING, 4, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("det_illusion", "that provide you with detect illusion")
        p = AddLockerChest(new EqAffectChest(AFF4_DETECT_ILLUSION, 4, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("neg_shield", "that provide you with negative shield")
        p = AddLockerChest(new EqAffectChest(AFF4_NEG_SHIELD, 4, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("wildmagic", "that provide you with wildmagic")
        p = AddLockerChest(new EqAffectChest(AFF4_WILDMAGIC, 4, chestKeyword, chestDesc));

      IF_ISLOCKERTYPE("prot_undead", "that provide you with protection from undead")
        p = AddLockerChest(new EqAffectChest(AFF5_PROT_UNDEAD, 5, chestKeyword, chestDesc));
     // New locker sorts added by Gellz 29/04/15
      IF_ISLOCKERTYPE("vamp", "that provide you with vampiric touch")
        p = AddLockerChest(new EqAffectChest(AFF2_VAMPIRIC_TOUCH, 2, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("toiw", "that provide you with tower of iron will")
        p = AddLockerChest(new EqAffectChest(AFF3_TOWER_IRON_WILL, 3, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("dark", "that provide you with globe of darkness")
        p = AddLockerChest(new EqAffectChest(AFF4_GLOBE_OF_DARKNESS, 4, chestKeyword, chestDesc));
      IF_ISLOCKERTYPE("pwt", "that provide you with pass without trace")
        p = AddLockerChest(new EqAffectChest(AFF3_PASS_WITHOUT_TRACE, 3, chestKeyword, chestDesc));

      IF_ISLOCKERTYPE("dazzle", "that make you a dazzler")
        p = AddLockerChest(new EqAffectChest(AFF4_DAZZLER, 4, chestKeyword, chestDesc));

      // New locker sorts added by Gellz 29/04/15
      // ignore 'unsorted' (or none), but give error if not found, not in
      // help mode, and nothing added
      if( !p && !bFound && str_cmp("unsorted", GBuf1)
        && helpMode != LOCKER_HELP_SHORT ) // != LOCKER_HELP_ALL_SHORT && helpMode != LOCKER_HELP_ALL_LONG )
      {
        snprintf(GBuf2 + strlen(GBuf2), MAX_STRING_LENGTH - strlen(GBuf2), "Invalid sort option: %s \n", GBuf1);
      }
      args = one_argument(args, GBuf1);
    }
    while( '\0' != GBuf1[0] );
  }

  if( bMakeAttachable )
    AddLockerChest(new EqSlotChest(ITEM_ATTACH_BELT, "attach", "attached to belt"));
  if( bMakeHoldable )
    AddLockerChest(new EqSlotChest(ITEM_HOLD, "hold", "that you can hold"));

  if( helpMode == LOCKER_HELP_NONE && ('\0' != GBuf2[0]) )
  {
    // error messages...
    send_to_char(GBuf2, ch);
  }

  // For non-custom, create the chest P_objs, and place them in the room...
  if( helpMode == LOCKER_HELP_NONE )
  {
    if( bIsCustomChest )
    {
      if( NULL != m_pChestList )
      {
        // The combochest gets the chest list we have now...
        ComboChest *pCombo = new ComboChest(m_pChestList);

        // Which is then reset to nothing
        m_pChestList = NULL;
        // Insert the created combo
        AddLockerChest(pCombo);
      }
    }
    // unsorted chest is ALWAYS present, and always the last chest
    AddLockerChest(new UnsortedChest());

    LockerChest *p = m_pChestList;

    while (p)
    {
      P_obj obj = p->CreateChestObject();

      obj_to_room(obj, m_realRoom);
      p = p->m_pNextInChain;
    }
  }
  else
  {
    // If there's a short list, replace the last ", " with ".\n"
    if( ('\0' != GBuf2[0]) && (helpMode == LOCKER_HELP_SHORT || helpMode == LOCKER_HELP_ALL_SHORT) )
    {
      snprintf(GBuf2 + strlen(GBuf2) - 2, MAX_STRING_LENGTH, ".\n" );
    }
    send_to_char(GBuf2, ch);
  }

  return bRet;
}

void LockerChest::BeautifyDesc(const char *srcDesc, char *destDesc)
{
  int len = strlen(m_chestKeyword);
  const char *p1 = srcDesc;
  char    *p2 = destDesc;

  do
  {
    if (!strn_cmp(p1, m_chestKeyword, len))
    {
      *p2++ = '&';
      *p2++ = '+';
      *p2++ = 'C';
      for (int i = 0; i < len; i++)
        *p2++ = *p1++;
      *p2++ = '&';
      *p2++ = '+';
      *p2++ = 'y';
    }
    else
      *p2++ = *p1++;
  }
  while (*p1);
  *p2 = '\0';

}

void LockerChest::FillExtraDescBuf(char *GBuf1)
{
  snprintf(GBuf1, MAX_STRING_LENGTH, "&+yThis chest contains your items %s&n.",
          this->m_chestDescText);
}


P_obj LockerChest::CreateChestObject(void)
{
  if (!m_pChestObject)
  {
    m_pChestObject = read_object(m_chestVnum, VIRTUAL);
    if (m_pChestObject)
    {
      // set sane values (or insane ones)
      m_pChestObject->wear_flags = 0;
      m_pChestObject->cost = 0;
      m_pChestObject->weight = 0;
      m_pChestObject->condition = 100;
      m_pChestObject->value[0] = -1;
      m_pChestObject->value[1] = m_pChestObject->value[2] =
        m_pChestObject->value[3] = 0;
      m_pChestObject->type = ITEM_CONTAINER;
      // string it
      m_pChestObject->str_mask |= (STRUNG_KEYS | STRUNG_DESC1);
      char GBuf1[MAX_STRING_LENGTH];

      strcpy(GBuf1, "chest ");
      strcat(GBuf1, this->m_chestKeyword);
      m_pChestObject->name = str_dup(GBuf1);
      strcpy(GBuf1, "&+yAn ornate chest bearing items ");
      strcat(GBuf1, this->m_chestDescText);
      strcat(GBuf1, "&+y.&n");
      char GBuf2[MAX_STRING_LENGTH];

      BeautifyDesc(GBuf1, GBuf2);
      m_pChestObject->description = str_dup(GBuf2);
      // make the extra keywords for the obj
      // "An ornate chest bearing your items *desc*." obj_data
      CREATE(m_pChestObject->ex_description, extra_descr_data, 1, MEM_TAG_EXDESCD);

      m_pChestObject->ex_description->keyword = str_dup(m_pChestObject->name);
      FillExtraDescBuf(GBuf1);
      BeautifyDesc(GBuf1, GBuf2);
      m_pChestObject->ex_description->description = str_dup(GBuf2);
      m_pChestObject->ex_description->next = NULL;
    }
  }
  return m_pChestObject;
}

LockerChest::~LockerChest(void)
{
  int   room;
  P_obj content, next;

  if( m_pChestObject )
  {
    // Drop the contents - default to limbo just in case.
    if( OBJ_ROOM(m_pChestObject) )
    {
      room = m_pChestObject->loc.room;
      room = (room < 0 || room > top_of_world) ? RROOM_LIMBO : room;
    }
    // This should never happen, but just in case.
    else
    {
      room = RROOM_LIMBO;
    }
    next = m_pChestObject->contains;
    while( (content = next) != NULL )
    {
      next = content->next_content;
      obj_from_obj(content);
      obj_to_room(content, room);
    }
    obj_from_room(m_pChestObject);
    extract_obj(m_pChestObject, TRUE); // Not an arti, but 'in game.'
  }
}

ComboChest::~ComboChest(void)
{
  LockerChest *p;

  while (m_LockerChests)
  {
    p = m_LockerChests;
    m_LockerChests = p->m_pNextInChain;
    delete   p;
  }
}

void ComboChest::FillExtraDescBuf(char *GBuf1)
{
  char GBuf2[MAX_STRING_LENGTH];

  strcpy(GBuf1, "&+yThis &+Ccustom&+y chest bears your items ");
  LockerChest *p = m_LockerChests;

  while (p)
  {
    p->BeautifyDesc(p->m_chestDescText, GBuf2);
    strcat(GBuf1, GBuf2);
    p = p->m_pNextInChain;
    if (p)
    {
      if (!p->m_pNextInChain)
        strcat(GBuf1, " and ");
      else
        strcat(GBuf1, ", ");
    }
  }
  strcat(GBuf1, "&n.");
}

bool ComboChest::ItemFits(P_obj obj)
{
  LockerChest *p = m_LockerChests;

  while (p)
  {
    if (!p->ItemFits(obj))
      return false;
    p = p->m_pNextInChain;
  }
  return true;
}

bool EqApplyChest::ItemFits(P_obj obj)
{
  for (int i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if ((obj->affected[i].location == m_applyType) &&
        (obj->affected[i].modifier != 0))
      return true;
  }
  return false;
}

bool EqAffectChest::ItemFits(P_obj obj)
{

// Removed buggy switch.
  if(m_bitVector == 1 &&
    IS_SET(obj->bitvector, m_bit))
  {
    return true;
  }
  else if(m_bitVector == 2 &&
         IS_SET(obj->bitvector2, m_bit))
  {
    return true;
  }
  else if(m_bitVector == 3 &&
         IS_SET(obj->bitvector3, m_bit))
  {
    return true;
  }
  else if(m_bitVector == 4 &&
          IS_SET(obj->bitvector4, m_bit))
  {
    return true;
  }
  else if(m_bitVector == 5 &&
         IS_SET(obj->bitvector5, m_bit))
  {
    return true;
  }
  else
  {
    return false;
  }
  return false;
}

/* storage locker room proc */
int storage_locker(int room, P_char ch, int cmd, char *arg);

/* create a new locker room for locker 'locker' which 'ch' will go to */
static int create_new_locker(P_char ch, P_char locker);

/* free memory associated with creating a new locker */
static void free_locker(int roomNum);

static P_char load_locker_char(P_char ch, char *locker_name,
                               int bValidateAccess);
static P_char create_locker_char(P_char chOwner, P_char newCh,
                                 char *locker_name);
static int save_locker_char(P_char chInLocker, int bTerminal);

static void check_for_artisInRoom(P_char ch, int rroom);
static int lockerName_is_inuse(char *lockerName);


static int locker_grantcmd(P_char ch, char *arg);
static int locker_equipcmd(P_char ch, char *arg);


/* cmds for access lists... */
static void locker_access_addAccess(P_char locker, char *ch_name);
static void locker_access_transferAccess(P_char locker, P_char ch);
static bool locker_access_canAccess(P_char locker, char *ch_name);
static int locker_access_count(P_char locker);
static void locker_access_show(P_char ch, P_char locker);
static bool locker_access_CanAdd(P_char locker, char *ch_name);
static void locker_access_remAccess(P_char locker, char *ch_name);

int storage_locker_room_hook(int room, P_char ch, int cmd, char *arg);

void display_no_mem( P_char ch )
{
  if( IS_PUNDEAD(ch) || GET_CLASS(ch, CLASS_WARLOCK) || IS_UNDEADRACE(ch)
    || is_wearing_necroplasm(ch) )
  {
    send_to_char("You can not manage your link with your &+Lnegative powers&N!\n", ch);
  }
  else if( USES_COMMUNE(ch) )
  {
    send_to_char( "&+yYou don't seem to see any nature in this closet.\n\r", ch );
  }
  else if( USES_DEFOREST(ch) )
  {
    send_to_char( "&+yYou don't seem to see any &+gnature&+y in this closet.\n\r", ch );
  }
  else if( USES_TUPOR(ch) )
  {
    send_to_char("You seem unable to relax enough to commune with the storm spirits.\r\n", ch );
  }
  else if (book_class(ch))
  {
    send_to_char("It's too quiet to read here.\n", ch);
  }
  else if (GET_CLASS(ch, CLASS_SHAMAN))
  {
    send_to_char("You seem unable to manage your trance.\n", ch);
  }
  else
  {
    send_to_char("Your prayers don't seem to be heard here.\n", ch);
  }
}

/* obj proc - put on bank counter objects instead of in bank rooms */
int storage_locker_obj_hook(P_obj obj, P_char ch, int cmd, char *argument)
{
  return storage_locker_room_hook(obj->loc.room, ch, cmd, argument);
}

/* room proc put in banks, etc to allow a person to enter a locker */
int storage_locker_room_hook(int room, P_char ch, int cmd, char *arg)
{
  P_char chLocker = NULL;
  char enterWhat[MAX_INPUT_LENGTH];
  char enterWho[MAX_INPUT_LENGTH];
  int locker_room;
  struct zone_data *zone;
  int is_guild_locker = 0;

  char lockerName[500];
  int bValidate = 0;


  if (cmd != CMD_ENTER)
    return FALSE;

  if (ch == NULL)
    return (FALSE);

  // not a god and not in a town?  then there's no locker here
  else if( !CHAR_IN_TOWN(ch) && (!IS_TRUSTED(ch) && (GET_RACE(ch) != RACE_LICH) && !IS_RACEWAR_NEUTRAL(ch)) )
  {
    return FALSE;
  }

  if (IS_NPC(ch) || IS_MORPH(ch))
    return (FALSE);

  argument_interpreter(arg, enterWhat, enterWho);

  if (str_cmp(enterWhat, "locker"))
    return FALSE;

  if (IS_TRUSTED(ch) && GET_LEVEL(ch) < OVERLORD)
    return FALSE;

  if( IS_IMMOBILE(ch) || IS_STUNNED(ch) || GET_STAT(ch) == STAT_SLEEPING )
  {
    send_to_char("You're not in much of a condition for that!\r\n", ch);
    return TRUE;
  }
  if( affected_by_spell(ch, TAG_PVPDELAY) )
  {
    send_to_char("There is too much adrenaline pumping through your body right now.\r\n", ch);
    return TRUE;
  }
  if( IS_RIDING(ch) )
  {
    send_to_char("If you really want your mount in your locker, you'll have to kill it first.\r\n", ch);
    return TRUE;
  }
  if (get_linking_char(ch, LNK_RIDING))
  {
    send_to_char("Perhaps your rider should dismount first?\r\n", ch);
    return TRUE;
  }

  if (IS_NOTWELCOME(ch))
  {
    send_to_char("You are NOT welcome here!\r\n", ch);
    return TRUE;
  }

  // zone = &zone_table[world[ch->in_room].zone];
  // if (zone->status > ZONE_NORMAL && (GET_LEVEL(ch) > 25) &&
      // (zone_table[world[ch->in_room].zone].number != WINTERHAVEN))
  // {
    // send_to_char("The safety commission says to you, '&+RThe town is under attack!!&n'\n", ch);
    // send_to_char(" .. '&+WWe must fortify our defenses and keep the invaders out of the locker!'\n", ch);
    // return TRUE;
  // }

  // check for guild locker
  if (!str_cmp(enterWho, "guild"))
  {                             /* guild lockers are named:  guild.x.locker where 'x' is the assoc number */
    if (!GET_ASSOC(ch) || !IS_MEMBER(GET_A_BITS(ch)) ||
        (IS_PC(ch) && !GT_PAROLE(GET_A_BITS(ch))))
    {
      send_to_char("Try becoming part of a guild first!\r\n", ch);
      return TRUE;
    }
    
    if (GET_ASSOC(ch)->get_prestige() < get_property("prestige.locker.required", 0))
    {
      send_to_char("Your association is not yet prestigious enough to have a locker!\r\n", ch);
      return TRUE;
    }
    snprintf(enterWho, MAX_INPUT_LENGTH, "guild.%d", GET_ASSOC(ch)->get_id());
        is_guild_locker = 1;
  }
  else if ('\0' == enterWho[0])
  {
    strcpy(enterWho, GET_NAME(ch));
  }
  else
  {
    bValidate = 1;
  }

  snprintf(lockerName, 500, "%s.locker", enterWho);

  chLocker = load_locker_char(ch, lockerName, bValidate);


  if (!chLocker)
  {
    return TRUE;
  }
  
  locker_room = create_new_locker(ch, chLocker);
  
  if (!locker_room)
  {
    send_to_char
      ("There are no free rooms available right now.  Please try later.\r\n",
       ch);
    return TRUE;
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  send_to_char("A member of the &+YStorage Locker Safety Commission&n escorts you to the locker.\r\n", ch);
  act("A member of the &+YStorage Locker Safety Commission&n escorts $n to a private room.", FALSE, ch, 0, ch, TO_ROOM);

  send_to_char("&+RWARNING: &+WStorage Lockers are not meant to have multiple containers in them. There is a possibility you may &+RLOSE &+Wyour container and all items in it. Store them at your own risk! NO REIMBURSEMENTS!\r\n", ch);

  // PFileToLocker

  GetChestList(locker_room)->PFileToLocker();
//MONEY HACK

  char_from_room(ch);

  char_to_room(ch, locker_room, 0);

  StorageLocker *pLocker = GetChestList(locker_room);

  int temp = 1 + ( 3 * pLocker->m_itemCount);

  if(pLocker->m_itemCount >= 401)
  {
    send_to_char("\r\n&+RYou have a ton of &+WSTUFF&+R, as in more than 400 items - so there's a surcharge!\r\n", ch);
    temp += (pLocker->m_itemCount - 400) * 2000;
  }

  if(is_guild_locker)
  {
    temp =1 + ( 1 * pLocker->m_itemCount);
  }

  char money_string[MAX_INPUT_LENGTH];
  snprintf(money_string, MAX_INPUT_LENGTH, "\r\nThe escort says 'You have &+W%d items&n, this cost you %s'&n\r\n", pLocker->m_itemCount , coin_stringv(temp) );
  send_to_char(money_string, ch);

  if( GET_MONEY(ch) < temp && GET_BALANCE(ch) < temp )
  {
    send_to_char("..but you don't have the money not even in your bank account!, GET OUT!\r\n\r\n", ch);
    room = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, world[room].dir_option[0]->to_room, 0);
    return TRUE;
  }

  if(GET_MONEY(ch) < temp)
  {
    SUB_BALANCE(ch, temp, 0);
  }
  else
  {
    SUB_MONEY(ch, temp, 0);
  }
//End Money hack

  // Stop deforest/commune/etc.
  if( IS_AFFECTED2(ch, AFF2_MEMORIZING) || get_scheduled(ch, event_memorize) )
  {
    REMOVE_BIT( ch->specials.affected_by2, AFF2_MEMORIZING );
    disarm_char_nevents(ch, event_memorize);
    send_to_char( "\n\r", ch );
    display_no_mem( ch );
    CharWait(ch, WAIT_SEC * 3);
  }

  strcpy(lockerName, GET_NAME(chLocker));

  if( strrchr(lockerName, '.') )
  {
    *(strrchr(lockerName, '.')) = '\0';
  }

  // warn them that they can't idle in the locker...
  if( str_cmp(lockerName, GET_NAME(ch)) )
  {
    logit(LOG_WIZ, "LOCKER: (%s) entered (%s's) locker.", GET_NAME(ch), lockerName);
    send_to_char("&+RWARNING:&n This isn't your own locker.  Therefore, you'll be ejected if\r\n"
      "you are idle for more then 2 minutes.\r\n", ch);
  }

  return TRUE;
}

/* room proc put in guildhalls, etc to allow a person to enter their guild locker */
int guild_locker_room_hook(int room, P_char ch, int cmd, char *arg)
{
  P_char chLocker = NULL;
  char enterWhat[MAX_INPUT_LENGTH];
  char enterWho[MAX_INPUT_LENGTH];
  int locker_room;
  struct zone_data *zone;
  int is_guild_locker = 0;
  
  char lockerName[500];
  int bValidate = 0;
  
  
  if (cmd != CMD_ENTER)
    return FALSE;
  
  if (ch == NULL)
    return (FALSE);
  
  if (IS_NPC(ch) || IS_MORPH(ch))
    return (FALSE);
  
  argument_interpreter(arg, enterWhat, enterWho);
  
  if (str_cmp(enterWhat, "locker"))
    return FALSE;
  
  if (IS_TRUSTED(ch) && GET_LEVEL(ch) < OVERLORD)
    return FALSE;
  
  if (IS_IMMOBILE(ch) ||
      IS_STUNNED(ch) ||
      GET_STAT(ch) == STAT_SLEEPING)
  {
    send_to_char("You're not in much of a condition for that!\r\n", ch);
    return TRUE;
  }
  
  if (affected_by_spell(ch, TAG_PVPDELAY))
  {
    send_to_char
    ("There is too much adrenaline pumping through your body right now.\r\n",
     ch);
    return TRUE;
  }
  
  if (IS_RIDING(ch))
  {
    send_to_char
    ("If you really want your mount in your locker, you'll have to kill it first.\r\n",
     ch);
    return TRUE;
  }
  
  if (get_linking_char(ch, LNK_RIDING))
  {
    send_to_char("Perhaps your rider should dismount first?\r\n", ch);
    return TRUE;
  }
  
  /* guild lockers are named:  guild.x.locker where 'x' is the assoc number */
  if (!GET_ASSOC(ch) || !IS_MEMBER(GET_A_BITS(ch)) ||
      (IS_PC(ch) && !GT_PAROLE(GET_A_BITS(ch))))
  {
    send_to_char("Try becoming part of a guild first!\r\n", ch);
    return TRUE;
  }
    
  snprintf(enterWho, MAX_INPUT_LENGTH, "guild.%d", GET_ASSOC(ch)->get_id());
  is_guild_locker = 1;
  
  snprintf(lockerName, 500, "%s.locker", enterWho);

  chLocker = load_locker_char(ch, lockerName, bValidate);
  
  if (!chLocker)
  {
    return TRUE;
  }
  
  locker_room = create_new_locker(ch, chLocker);
  
  if (!locker_room)
  {
    send_to_char
    ("There are no free rooms available right now.  Please try later.\r\n",
     ch);
    return TRUE;
  }
  
#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  send_to_char
  ("A member of the &+YStorage Locker Safety Commission&n escorts you to the locker\r\n",
   ch);
  act
  ("A member of the &+YStorage Locker Safety Commission&n escorts $n to a private room.",
   FALSE, ch, 0, ch, TO_ROOM);
  
  // PFileToLocker
  
  GetChestList(locker_room)->PFileToLocker();
  
  char_from_room(ch);
  
  char_to_room(ch, locker_room, 0);
  
  StorageLocker *pLocker = GetChestList(locker_room);
  
  strcpy(lockerName, GET_NAME(chLocker));
  
  if (strrchr(lockerName, '.'))
    *(strrchr(lockerName, '.')) = '\0';
  
  // warn them that they can't idle in the locker...
  if (str_cmp(lockerName, GET_NAME(ch)))
  {
    logit(LOG_WIZ, "LOCKER: (%s) entered (%s's) locker.", GET_NAME(ch), lockerName);
    send_to_char
    ("&+RWARNING:&n This isn't your own locker.  Therefore, you'll be ejected if\r\n"
     "you are idle for more then 2 minutes.\r\n", ch);
  }
  
  return TRUE;  
}

int storage_locker(int room, P_char ch, int cmd, char *arg)
{
  P_char tmpChar = NULL;
  int troom, cost;
  struct zone_data *zone;
  int      bits, wtype, craft, mat;
  P_char   tmp_char;
  P_obj    tmp_object;
  float    result_space;
  char     name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH + 4];


  if( (cmd == CMD_SHAPECHANGE) || (cmd == CMD_DISGUISE) || (cmd == CMD_SWITCH) || (cmd == CMD_AT)
    || (cmd == CMD_CAMP) || (cmd == CMD_QUIT) || cmd == CMD_USE || cmd == CMD_SUMMON || cmd == CMD_INNATE
    || cmd == CMD_BANDAGE || cmd == CMD_QUEST )
  {
    send_to_char("You can't do that in here!  Leave first.\r\n", ch);
    return TRUE;
  }

  if( cmd == CMD_ASSIMILATE || cmd == CMD_COMMUNE || cmd == CMD_DEFOREST || cmd == CMD_TUPOR
    || cmd == CMD_FOCUS || cmd == CMD_PRAY || cmd == CMD_MEMORIZE )
  {
    display_no_mem(ch);
    return TRUE;
  }

  if ((cmd == CMD_DOORBASH) || (cmd == CMD_DOORKICK))
  {
    send_to_char("Instead, just open the door and walk out, okay?\r\n", ch);
    return TRUE;
  }

  one_argument(arg, name);

  // check legend lore

  if (cmd == CMD_STAT && !IS_TRUSTED(ch) )
  {

 if (!*name)
  {
    send_to_char("The member of the &+YStorage Locker Safety Commission&n says 'Hmm, what item should i tell you more about?'\r\n", ch); 
   return TRUE;
  }

  bits =
    generic_find(name, FIND_OBJ_INV , ch,
                 &tmp_char, &tmp_object);
                 
  
// 1000 = 1 plat, 100 = 1 gold
  cost = get_property("locker.stat.cost", 100);

  // check legend lore
  if (tmp_object)
  {
    CharWait(ch, (int) (PULSE_VIOLENCE * 1.5));
     if (GET_MONEY(ch) < cost && GET_BALANCE(ch) < cost  )
    {
      send_to_char("The member of the &+YStorage Locker Safety Commission&n says 'Bring me 1 &+Ygold&n and ill give you the stats.'\r\n", ch);
      return (TRUE);
    }
    else {
    send_to_char("The member of the &+YStorage Locker Safety Commission&n takes 1 &+Ygold.&n\r\n", ch);
    send_to_char("The member of the &+YStorage Locker Safety Commission&n says 'This is:'\r\n", ch);
        if(GET_MONEY(ch) < cost)
        {
        SUB_BALANCE(ch, cost, 0);
        }
        else
        {
        SUB_MONEY(ch, cost, 0);
        }

    do_lore(ch, arg, 999);
    return TRUE;
    }
  }
  else {
     send_to_char("The member of the &+YStorage Locker Safety Commission&n says 'Hmm, what item should i tell you more about?'\r\n", ch);
   return TRUE;

  }
   
}
  StorageLocker *pLocker = GetChestList(ch->in_room);

  if (cmd == (-75))
  {                             /* they are leaving the locker - say bye-bye! */
    /* someone is leaving the locker room.  If it's the proper occupant, kick
       everyone else out... */
    if (ch == pLocker->GetLockerUser())
    {
      tmpChar = world[ch->in_room].people;
      while (tmpChar)
      {
        if (ch == world[ch->in_room].people)
        {
          tmpChar = ch->next_in_room;
          continue;
        }
        /* this person isn't the proper occupant, so kick them the hell out */
        send_to_char
          ("You feel yourself being (ungracefully) ejected from the locker.\r\n",
           tmpChar);
        char_from_room(tmpChar);
        //char_to_room(tmpChar, world[ch->in_room].dir_option[0]->to_room, 0);
        // functionality of alt_hometown_check disabled
        char_to_room(tmpChar, alt_hometown_check(ch, world[ch->in_room].dir_option[0]->to_room, 0), 0);
        tmpChar = world[ch->in_room].people;
      }                         /* while */

      /* only person that wasn't kicked out was the proper occupant... */
      save_locker_char(ch, TRUE);
      /* throw any corpses out as well */
      P_obj next_obj;

      for (P_obj tmp_obj = world[ch->in_room].contents; tmp_obj;
           tmp_obj = next_obj)
      {
        next_obj = tmp_obj->next_content;
        if (tmp_obj->type == ITEM_CORPSE)
        {
          char buf[MAX_STRING_LENGTH];

          snprintf(buf, MAX_STRING_LENGTH, "%s&n flies out in front of you!\r\n",
                  tmp_obj->short_description);
          send_to_char(buf, ch);
          obj_from_room(tmp_obj);
          obj_to_room(tmp_obj, world[ch->in_room].dir_option[0]->to_room);
        }
      }
      send_to_char
        ("As you leave, you see the &+YSLSC&n member applying magic locks to the door...\r\n",
         ch);
    
      troom = world[ch->in_room].dir_option[0]->to_room;
  
      zone = &zone_table[world[troom].zone];
      
      if (zone->status > ZONE_NORMAL) 
      {
        // functionality of alt_hometown_check disabled
        world[ch->in_room].dir_option[0]->to_room = alt_hometown_check(ch, troom, 0);
      }

      free_locker(room);
    }
  }
  if (cmd == (-80))
  {
    /* save hook - someone in the room is being saved. adjust things so they don't
       really get saved in the locker room */

    /* quick adjustment - if someone dropped an arti, give it back to them before saving 'em */
    check_for_artisInRoom(ch, ch->in_room);
    ch->specials.was_in_room =
      world[world[ch->in_room].dir_option[0]->to_room].number;
    /* this return value will ensure that the pfile is saved where the locker_hook is, and NOT
       in the actual locker room */
    return (world[ch->in_room].dir_option[0]->to_room);
  }

  if (cmd == (-81))
  {
    /* another save hook... this one is called AFTER the player is saved.  If it happens that
       the character being saved is the proper occupant, save the locker character too */
    if (ch == pLocker->GetLockerUser())
    {                           /* if so, save the locker too */
      if (!save_locker_char(ch, FALSE))
      {
        /* wasn't able to save the locker char.  This is bad, but can happen
           if the locker char was purged by a god, or idle rented.  the only way
           to deal with it is to eject the player from the locker */
        room = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[room].dir_option[0]->to_room, 0);
      }
      else
      {
        /* anti-idle code.  only let people idle in their own locker */
        char arg1[MAX_INPUT_LENGTH];

        strcpy(arg1, GET_NAME(pLocker->GetLockerChar()));
        if (strrchr(arg1, '.'))
          *(strrchr(arg1, '.')) = '\0';
        if (str_cmp(arg1, GET_NAME(ch)) && (ch->specials.timer > 3))
        {
          send_to_char
            ("You can only sit idle in your own locker...  GET OUT!\r\n", ch);
          room = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[room].dir_option[0]->to_room, 0);
        }
      }
    }
  }
  if (cmd == CMD_GRANT)
  {
    return locker_grantcmd(ch, arg);
  }
  if (cmd == CMD_EQUIPMENT)
  {
    return locker_equipcmd(ch, arg);
  }
  return FALSE;
}


void StorageLocker::event_resortLocker(P_char chLocker, P_char ch, P_obj obj,
                                       void *data)
{
  /* always will be attached to the 'unsorted' locker...  when called, all the lockers
     will already be in the room.  ch will be the player, and chLocker is the locker..
     if 'data' is null, this was called from the event mgr, meaning a save or initial
     entry.  Otherwise, its being called directly - from an eq sort command */
  StorageLocker *pLocker = GetChestList(ch->in_room);

  int nOldCount = pLocker->m_itemCount;

  // usually, everything will already be on pfile, but just make sure
  pLocker->LockerToPFile();

  // move everything from the pfile to the locker - this also sorts it
  pLocker->PFileToLocker();

  if (NULL != data)
  {
    /* eq sort cmd... */
    if (world[ch->in_room].contents &&
        world[ch->in_room].contents->next_content)
    {
      send_to_char
        ("&+LA small &+ggremlin&+L appears out of no where, quickly arranging your belongings "
         "before disappearing into thin &+Cair&n.\r\n", ch);
    }
    else
    {
      send_to_char
        ("&+LA &+gLARGE gremlin&+L with hulking muscles storms through the room, gathers all of your "
         "belongings and places them back into the &+Cunsorted&+y items chest&n.\r\n",
         ch);
    }
  }
  else
  {
    /* due to the locker being saved (most likely from dropping something ) */
    /* to prevent spammage, however, only display the message if something was added to the locker */
    if (nOldCount < pLocker->m_itemCount)
      send_to_char
        ("&+LA small &+ggremlin&+L appears out of nowhere, and ensures everything is sorted...\r\n",
         ch);
  }
}


static int locker_equipcmd(P_char ch, char *arg)
{
  P_char chLocker = NULL;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];


  StorageLocker *pLocker = GetChestList(ch->in_room);

  if (ch != pLocker->GetLockerUser())
  {
    send_to_char("Sort your own locker!.\r\n", ch);
    return TRUE;
  }
  chLocker = pLocker->GetLockerChar();
  if (!chLocker)
  {
    send_to_char
      ("Error: unable to locate chLocker.  Please report ASAP\r\n", ch);
    return TRUE;
  }
  arg = one_argument(arg, arg1);
  if (str_cmp(arg1, "sort"))
  {
    return FALSE;
  }
  int nChestsLoaded = 0;
  P_obj chestObj = NULL;

  if (pLocker->MakeChests(ch, arg))
  {
    pLocker->LockerToPFile();
    StorageLocker::event_resortLocker(chLocker, ch, NULL, (void *) -1);
    /* resort event will move everything back into the room */
  }

  return TRUE;
}

static int locker_grantcmd(P_char ch, char *arg)
{
  P_char chLocker = NULL;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  StorageLocker *pLocker = GetChestList(ch->in_room);
  bool bPlayerIsGod = ((GET_LEVEL(ch) >= OVERLORD) || god_check(ch->player.name));

  strcpy(arg1, GET_NAME(pLocker->GetLockerChar()));
  if (strrchr(arg1, '.'))
    *(strrchr(arg1, '.')) = '\0';
  if (str_cmp(arg1, GET_NAME(ch)) && !bPlayerIsGod)
  {
    send_to_char("Only the actual owner of a locker can manipulate access lists.\r\n", ch);
    return TRUE;
  }
  chLocker = pLocker->GetLockerChar();
  if( !chLocker )
  {
    send_to_char("Error: unable to locate chLocker.  Please report ASAP\r\n", ch);
    return TRUE;
  }
  if( !bPlayerIsGod )
    GET_RACEWAR(chLocker) = GET_RACEWAR(ch);
  argument_interpreter(arg, arg1, arg2);
  if( *arg1 == '\0' )
    snprintf(arg1, MAX_INPUT_LENGTH, "?" );

  if( is_abbrev(arg1, "list") )
  {
    locker_access_show(ch, chLocker);
    return TRUE;
  }
  else if( is_abbrev(arg1, "add") )
  {                             /* max of 10 people in the list */
    if ('\0' == arg2[0])
    {
      send_to_char("Okay, you want to add someone.  WHO!?\r\n", ch);
    }
    else if (locker_access_count(chLocker) >= 10)
    {
      send_to_char("Too many people would have access!  Remove someone first.\r\n", ch);
    }
    else if (locker_access_canAccess(chLocker, arg2))
    {
      send_to_char("That person already has access!\r\n", ch);
    }
    else
    {
      if (locker_access_CanAdd(chLocker, arg2))
      {
        locker_access_addAccess(chLocker, arg2);
        send_to_char_f( ch, "'%s' given access to your locker.\n", arg2 );
        storage_locker(ch->in_room, ch, (-81), NULL);   // saves the locker
        storage_locker(ch->in_room, ch, CMD_GRANT, "list");
      }
      else
      {
        send_to_char("Unknown character: ", ch);
        send_to_char(arg2, ch);
        send_to_char("\r\n", ch);
      }
    }
    return TRUE;
  }
  else if( is_abbrev(arg1, "remove") )
  {
    if ('\0' == arg2[0])
    {
      send_to_char("Okay, you want to remove someone.  WHO!?\r\n", ch);
    }
    else if (!locker_access_canAccess(chLocker, arg2))
    {
      send_to_char("You can only remove someone who already has access.  Duh!\r\n", ch);
    }
    else
    {
      locker_access_remAccess(chLocker, arg2);
      send_to_char_f( ch, "'%s' lost access to your locker.\n", arg2 );
      storage_locker(ch->in_room, ch, (-81), NULL);     // saves the locker
      storage_locker(ch->in_room, ch, CMD_GRANT, "list");
    }
    return TRUE;
  }
  else if( is_abbrev(arg1, "transfer") )
  {
    locker_access_transferAccess( chLocker, ch );
    return TRUE;
  }
  else
  {
    if( str_cmp(arg1, "?") && !is_abbrev(arg1, "help") )
    {
      send_to_char_f( ch, "'%s' is not a valid locker command.\n", arg1 );
    }
    send_to_char("Locker Grant Commands\r\n", ch);
    send_to_char("----------------------------------------------------------------------------------\n", ch);
    send_to_char("grant list            shows who has access to your locker.\n", ch);
    send_to_char("grant add <name>      adds <name> to those who can access your locker.\n", ch);
    send_to_char("grant remove <name>   removes <name> from those who can access your locker.\n", ch);
    send_to_char("grant transfer        transfers the old list of those with access to the new list.\n", ch);
    send_to_char("grant [? | help]      displays this help.\n\n", ch);
    return TRUE;
  }
  return FALSE;
}

static void locker_access_show(P_char ch, P_char locker)
{
  char buffer[MAX_STR_NORMAL];
  MYSQL_RES *res;
  MYSQL_ROW row;

  if( !qry("select visitor from locker_access where owner = '%s'", GET_NAME( locker )) )
  {
    send_to_char( "Error with database.\n", ch );
    return;
  }

  res = mysql_store_result(DB);
  if( mysql_num_rows(res) < 1)
  {
    snprintf(buffer, MAX_STR_NORMAL, "No one has access to your locker but you.\n" );
  }
  else
  {
    snprintf(buffer, MAX_STR_NORMAL, "Locker Access: " );
    while ((row = mysql_fetch_row(res)))
    {
      strcat( buffer, row[0] );
      strcat( buffer, ", " );
    }
    snprintf(&(buffer[strlen(buffer)-2]), MAX_STRING_LENGTH, ".\n" );
  }

  mysql_free_result(res);
  send_to_char( buffer, ch );
  return;
}

static bool locker_access_CanAdd(P_char locker, char *ch_name)
{
  /* load ch_name, and if loaded, compare RACEWAR() sides.  if they are the
     same, return 1, else return 0.   if the restore of ch_name fails, return 0 */
  P_char vict = NULL;
  bool bCanAdd = FALSE;

  vict = (P_char) mm_get(dead_mob_pool);
  clear_char(vict);
  ensure_pconly_pool();
  vict->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

  if( (restoreCharOnly( vict, ch_name )) >= 0 )
  {
    bCanAdd = GET_RACEWAR(locker) == GET_RACEWAR(vict);
    free_char(vict);
  }

  return bCanAdd;
}

static int locker_access_count(P_char locker)
{
  MYSQL_RES *res;
  int count;

  if( !qry("select owner, visitor from locker_access where owner = '%s'", GET_NAME( locker )) )
  {
    return FALSE;
  }

  res = mysql_store_result(DB);

  count = mysql_num_rows(res);

  mysql_free_result(res);
  return count;
}

static bool locker_access_canAccess(P_char locker, char *ch_name)
{
  MYSQL_RES *res;

  if( !qry("select owner, visitor from locker_access where owner = '%s' and visitor = '%s' limit 1",
    GET_NAME( locker ), ch_name) )
  {
    return FALSE;
  }

  res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return FALSE;
  }

  mysql_free_result(res);
  return TRUE;
}

static void locker_access_remAccess(P_char locker, char *ch_name)
{
  qry( "DELETE FROM locker_access WHERE owner='%s' AND visitor='%s'", GET_NAME(locker), ch_name );
}

static void locker_access_addAccess(P_char locker, char *ch_name)
{
  qry( "INSERT INTO locker_access (owner, visitor) VALUES ('%s', '%s')", GET_NAME(locker), ch_name );
}

static int create_new_locker(P_char ch, P_char locker)
{
  P_obj tmp_object = NULL, next_obj = NULL;
  int roomNum = -1;
  int realNum = -1;
  int dir;
  int exitDir;

  /* find a room to use... */
  roomNum = LOCKERS_START + 1;
  while( roomNum < (LOCKERS_START + LOCKERS_MAX) &&
                 world[realNum = real_room0(roomNum)].funct == storage_locker)
  {
    roomNum++;
  }

  if(roomNum >= (LOCKERS_START + LOCKERS_MAX))
  {
          wizlog(AVATAR, "All locker rooms in use or locker rooms not loaded!");
          return 0;
  }
  
  if (roomNum < (LOCKERS_START + LOCKERS_MAX))
  {
    char roomNameBuf[500];

    /* check for a guild locker... */
    if (!strn_cmp("guild.", GET_NAME(locker), 6))
    {                           /* its a guild locker... room name is based on assoc name */
      FILE    *f = NULL;
      int assoc_num = 0;
      char Gbuf1[MAX_STR_NORMAL];

      assoc_num = atoi(GET_NAME(locker) + 6);
      snprintf(Gbuf1, MAX_STR_NORMAL, "%sasc.%u", ASC_DIR, assoc_num);
      f = fopen(Gbuf1, "r");
      if (f)
      {
        fgets(Gbuf1, MAX_STR_NORMAL, f);
        fclose(f);
      }
      else
      {
        strcpy(Gbuf1, "&+RUNKNOWN ASSOC&n");
      }
      snprintf(roomNameBuf, 500, "The Storage Locker for %s&n", Gbuf1);
    }
    else
    {                           /* normal player locker */
      snprintf(roomNameBuf, 500, "The Storage Locker for %s", GET_NAME(locker));
      if (strrchr(roomNameBuf, '.'))
        *(strrchr(roomNameBuf, '.')) = '\0';
    }
    /* found a room to use!  set the room name, flags, etc */
    world[realNum].sector_type = LOCKERS_SECT_TYPE;
    // commenting this out for now
    // room names/description pointers can be shared, so we effectively have to leak
    // this memory because we don't know if it is being used somewhere else or not
    //if(world[realNum].name)
    //    str_free(world[realNum].name);
    world[realNum].name = str_dup(roomNameBuf);
    world[realNum].funct = storage_locker;
    world[realNum].room_flags = LOCKERS_ROOMFLAGS;

    strcpy(roomNameBuf, LOCKERS_DOORSIGN);
    strcat(roomNameBuf, GET_NAME(ch));
    /* exit 0 will always be the way out... */
    if (!world[realNum].dir_option[0])
      CREATE(world[realNum].dir_option[0], room_direction_data, 1, MEM_TAG_DIRDATA);

    world[realNum].dir_option[0]->to_room = ch->in_room;
    world[realNum].dir_option[0]->exit_info = EX_ISDOOR | EX_CLOSED;
    // world[realNum].dir_option[0]->general_description = NULL;
    // fixed issue in free_world() when trying to free a locker room's keyword
    if(world[realNum].dir_option[0]->keyword)
        str_free(world[realNum].dir_option[0]->keyword);
    world[realNum].dir_option[0]->keyword = str_dup("door");
    if (world[realNum].dir_option[0]->general_description)
      str_free(world[realNum].dir_option[0]->general_description);
    world[realNum].dir_option[0]->general_description = str_dup(roomNameBuf);
    world[realNum].dir_option[0]->key = -1;

    /* setup an extra description for the room which tells me the real locker pfile name */
    StorageLocker *pLocker = new StorageLocker(realNum, locker, ch);

    pLocker->MakeChests(ch, "none");

    for (dir = 1; dir < NUM_EXITS; dir++)
    {
      if (world[realNum].dir_option[dir])
        world[realNum].dir_option[dir]->to_room = -1;
    }
  }
  else
  {
    /* failed to find a valid room to use as a locker */
    realNum = 0;
  }
  return realNum;
}

static void free_locker(int roomNum)
{
  P_obj tmp_object, next_obj;

  /* perform cleanup - basically just frees a couple pointers and marks the room as
     available for reuse */

  if (-1 != roomNum)
  {
    str_free(world[roomNum].name);
    world[roomNum].name = NULL;
    world[roomNum].funct = NULL;
    if ((world[roomNum].dir_option[0]) &&
        (world[roomNum].dir_option[0]->general_description))
    {
      str_free(world[roomNum].dir_option[0]->general_description);
      world[roomNum].dir_option[0]->general_description = NULL;
    }

    StorageLocker *p = GetChestList(roomNum);

    if (p)
      delete p;
  }
}

static P_char load_locker_char(P_char ch, char *locker_name, int bValidateAccess)
{
  P_char vict = NULL;
  int tmp;

  if( !ch )
  {
    wizlog(56, "load_locker_char() in storage_lockers.c without ch : locker %s!", locker_name);
    logit(LOG_WIZ, "load_locker_char() in storage_lockers.c without ch : locker %s!", locker_name);
    sql_log(ch, PLAYERLOG, "load_locker_char() in storage_lockers.c without ch : locker %s!", locker_name);
    logit(LOG_EXIT, "load_locker_char() called in storage_lockers.c without ch.");
    raise(SIGSEGV);
  }

  bool bPlayerIsGod = (GET_LEVEL(ch) >= OVERLORD || god_check(ch->player.name));

  vict = (P_char) mm_get(dead_mob_pool);
  clear_char(vict);
  ensure_pconly_pool();
  vict->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
  vict->only.pc->aggressive = -1;
  vict->desc = NULL;

  if(!(vict))
  {
    return NULL;
  }

  tmp = restoreCharOnly(vict, locker_name);


  if (tmp < (-1))
  {
    send_to_char("ERROR: Unable to load locker.  Please report ASAP.\r\n", ch);
    free_char(vict);
    return NULL;
  }
  // if not primary char for locker, it can't be created - also check access
  if ((bValidateAccess) &&
      ((-1 == tmp) || (!bPlayerIsGod && !locker_access_canAccess(vict, GET_NAME(ch)))))
  {
    // can't access it
    send_to_char("You don't have access to that locker!\r\n", ch);
    free_char(vict);
    return NULL;
  }
  if (-1 != tmp)
  {
    if (lockerName_is_inuse(locker_name) > 0)
    {
      send_to_char
        ("Someone is currently using that locker.  Please try later.\r\n", ch);
      return NULL;
    }
    if (bValidateAccess)
    {
      if (!bPlayerIsGod)
        if (GET_RACEWAR(ch) != GET_RACEWAR(vict))
        { // well, THIS is interesting.  VERY interesting, indeed.  The locker has a different racewar
          // side then the character... hmmmmmmmmm.  One of two things going on here.  Either the player
          // made a mistake in re-creating an existing char, or they are purposely cheating.
          send_to_char("Well, THIS is interesting... You have access to a locker on the other racewar\n"
                      "side.  A special log is being generated which will be followed up on to\n"
                      "determine if this was a mistake, or an attempt at cheating.\n", ch);
          wizlog(56, "&+RPOSSIBLE CHEATING:&n %s is trying to access opposite racewar side locker %s", GET_NAME(ch), locker_name);
          logit(LOG_WIZ, "POSSIBLE CHEATING: %s is trying to access opposite racewar side locker %s", GET_NAME(ch), locker_name);
          sql_log(ch, PLAYERLOG, "&+RPOSSIBLE CHEATING:&n trying to access opposite racewar side locker %s", locker_name);
          free_char(vict);
          return NULL;
        }
    }
    else
    {
      // just in case their racewar side happened to change mysteriously..
      GET_RACEWAR(vict) = GET_RACEWAR(ch);
    }

  }

  if (-1 == tmp)
  {                             /* need to create a new locker pfile */
    create_locker_char(ch, vict, locker_name);
  }
  else
  {
    tmp = restoreItemsOnly(vict, 100);
  }

  /* insert in list */
  vict->next = character_list;
  character_list = vict;

  setCharPhysTypeInfo(vict);

  /* saving info for teleport return command */
  vict->specials.was_in_room = vict->in_room;

  char_to_room(vict, real_room0(LOCKERS_START), -2);

  return vict;
}

static P_char create_locker_char(P_char chOwner, P_char ch, char *locker_name)
{
  /* create a new pfile with 'locker_name' name */

  ch->player.name = str_dup(locker_name);
  GET_RACEWAR(ch) = GET_RACEWAR(chOwner);
  GET_RACE(ch) = GET_RACE(chOwner);
//  init_char(ch);
  strcpy(ch->only.pc->pwd, chOwner->only.pc->pwd);
  writeCharacter(ch, 0, NOWHERE);
  return ch;
}


static int save_locker_char(P_char ch, int bTerminal)
{

  StorageLocker *pLocker = GetChestList(ch->in_room);

  if (NULL != pLocker)
  {
    P_char chLocker = NULL;

    chLocker = pLocker->GetLockerChar();
    if (chLocker)
    {
      pLocker->LockerToPFile();
      if (!writeCharacter(chLocker, bTerminal ? 3 : 0, NOWHERE))
      {
        logit(LOG_OBJ, "%s's locker not saving properly!", GET_NAME(ch));
        debug("%s's locker not saving properly!", GET_NAME(ch));
        send_to_char
          ("&+R&-LWARNING:  The locker is not saving properly.  This may be due to having too much stuff in it, or another error.  Please pick items up until this error goes away.&n\r\n",
           ch);
      }

      /* prevent storage locker chars from idle-renting */
      chLocker->specials.timer = 0;
      if (bTerminal)
      {
        extract_char(chLocker);
      }
      else
      {
        if (!get_scheduled(chLocker, StorageLocker::event_resortLocker))
          add_event(StorageLocker::event_resortLocker, 1, chLocker, ch, NULL, 0, NULL, 0);
      }
    }
    else
    {
      /* player not in world?  This can happen if the locker character idled out... */
      return 0;
    }
  }
  else
  {
    send_to_char("Error: which locker are you in?\r\n", ch);
    return 0;
  }
  return 1;
}

void StorageLocker::LockerToPFile(void)
{
  P_obj tmp_object, next_obj;

  const int lockerChestRNUM = real_object(LockerChest::m_chestVnum);

  for (tmp_object = world[m_realRoom].contents; tmp_object;
       tmp_object = next_obj)
  {
    next_obj = tmp_object->next_content;
    if (tmp_object->type == ITEM_CORPSE)
      continue;
    if (tmp_object->R_num == lockerChestRNUM)
    {
      /* dump the contents of the chest to the room */
      for (P_obj innerObj = tmp_object->contains;
           tmp_object->contains; innerObj = tmp_object->contains)
      {
        obj_from_obj(innerObj);
        obj_to_char(innerObj, m_chLocker);
      }
    }
    else
    {
      obj_from_room(tmp_object);
      obj_to_char(tmp_object, m_chLocker);
    }
  }
}

void StorageLocker::PFileToLocker(void)
{
  /* drop everything chLocker is holding into rroom - sorting into chests if they're present */
  P_obj tmp_object, next_obj;
  int nCount = 0;

  for (tmp_object = m_chLocker->carrying; tmp_object; tmp_object = next_obj)
  {
    next_obj = tmp_object->next_content;
    nCount++;
    obj_from_char(tmp_object);
    if ((tmp_object->type == ITEM_MONEY) || !PutInProperChest(tmp_object))
      obj_to_room(tmp_object, m_realRoom);
  }
  m_itemCount = nCount;

  if( m_bIValue )
    SortIValues();
}


static void check_for_artisInRoom(P_char ch, int rroom)
{
  P_obj tmp_object, next_obj;

  for (tmp_object = world[rroom].contents; tmp_object; tmp_object = next_obj)
  {
    next_obj = tmp_object->next_content;
    
    if (IS_SET(tmp_object->extra_flags, ITEM_ARTIFACT))
    {
      /* okay, they can't store that here... tell them so, and stick it back
         in their inventory */
      obj_from_room(tmp_object);
      obj_to_char(tmp_object, ch);
      act
        ("&+LThe overpowering will of $p &+Ldenies $n&+L's attempt to release it.",
         TRUE, ch, tmp_object, 0, TO_ROOM);
      act
        ("&+LThe overpowering will of $p &+Ldefies you as it flies back into your hands.",
         TRUE, ch, tmp_object, 0, TO_CHAR);
      wizlog( 56, "Artifact %s (%d) returns to %s's hands in room %d.",
        tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number, J_NAME(ch), world[ch->in_room].number );
      logit(LOG_OBJ, "Artifact %s (%d) returns to %s's hands in room %d.",
        tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number, J_NAME(ch), world[ch->in_room].number );
    }
  }
}

static int lockerName_is_inuse(char *lockerName)
{
   P_char chLocker = NULL;
   int nCnt = 0;

   for (chLocker = character_list; chLocker; chLocker = chLocker->next)
   {
      if (isname(lockerName, GET_NAME(chLocker)))
      {
         nCnt++;
      }
   }
   return nCnt;
}

bool rename_locker(P_char ch, char *old_charname, char *new_charname)
{
   char lockerOldName[MAX_STRING_LENGTH], lockerNewName[MAX_STRING_LENGTH];
   P_char chLocker = NULL;
   int tmp;

   snprintf(lockerOldName, MAX_STRING_LENGTH, "%s.locker", old_charname);
   snprintf(lockerNewName, MAX_STRING_LENGTH, "%s.locker", new_charname);

   chLocker = (P_char) mm_get(dead_mob_pool);
   clear_char(chLocker);
   ensure_pconly_pool();
   chLocker->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
   chLocker->only.pc->aggressive = -1;
   chLocker->desc = NULL;

   tmp = restoreCharOnly(chLocker, lockerOldName);

   /* char has no locker yet, nothing to rename then */
   if (-1 == tmp) return TRUE;

   if (tmp != -1)
   {
     if (lockerName_is_inuse(lockerOldName) > 0)
     {
        send_to_char("Someone is currently using that locker.  Please try later.\r\n", ch);
        return FALSE;
     }

     tmp = restoreItemsOnly(chLocker, 100);
   }

   /* remove old char file, ups... no backup? */
   deleteCharacter(chLocker, false);

   /* put new name and save char file */
   CAP(lockerNewName);
   GET_NAME(chLocker) = str_dup(lockerNewName);

   if( !writeCharacter(chLocker, 0, NOWHERE) )
   {
     logit(LOG_OBJ, "Char save failed for %s in rename_locker()!", GET_NAME(ch));
     debug("Char save failed for %s in rename_locker()!", GET_NAME(ch));
   }
   
   free_char(chLocker);

   /* here we pray that write realy writed locker char or we just nuked someones locker */
   return TRUE;
}

void StorageLocker::SortIValues(void)
{
  P_obj object, rest, pObjList;
  char buf[MAX_STRING_LENGTH];
  LockerChest *pChests = m_pChestList;
  int value;

  if( m_bIValue )
  {
    while (pChests)
    {
      rest = pChests->m_pChestObject->contains;
      pChests->m_pChestObject->contains = NULL;

      // While there is more to sort..
      while( rest )
      {
        // Remove one object from the list
        object = rest;
        rest = rest->next_content;
        object->next_content = NULL;
        value = itemvalue( object );

        // Put into right spot in chest.
        // If value is smallest, insert to head of list.
        if( !pChests->m_pChestObject->contains
          || value <= itemvalue( pChests->m_pChestObject->contains ) )
        {
          object->next_content = pChests->m_pChestObject->contains;
          pChests->m_pChestObject->contains = object;
        }
        else
        {
          // Walk through the list to find the correct spot.
          pObjList = pChests->m_pChestObject->contains;
          while( pObjList->next_content
            && value > itemvalue( pObjList->next_content ) )
            pObjList = pObjList->next_content;

          // Insert the object into the list.
          object->next_content = pObjList->next_content;
          pObjList->next_content = object;
        }
      }
      pChests = pChests->m_pNextInChain;
    }

  }
  else
    logit( LOG_DEBUG, "SortIValues called when m_bIValue is not set!" );
  m_bIValue = false;
}

void remove_all_locker_access( P_char ch )
{
  qry( "DELETE FROM locker_access WHERE visitor='%s'", GET_NAME(ch) );
}

static void locker_access_transferAccess(P_char chLocker, P_char ch)
{
  // 8 = ".locker" + string terminator.
  char locker_name[MAX_NAME_LENGTH + 8];
  char ch_name[MAX_NAME_LENGTH+1];
  char names[MAX_STR_NORMAL], *pIndex;

  // Set locker name. (for speed)
  snprintf(locker_name, MAX_STRING_LENGTH, "%s", GET_NAME(chLocker) );
  // Set list of names that have access to locker.
  if( chLocker->player.description != NULL )
    snprintf(names, MAX_STR_NORMAL, "%s", chLocker->player.description );
  else
    names[0] = '\0';

  if( names[0] == '\0' )
  {
    send_to_char( "No old accesses found.\n", ch );
    return;
  }

  pIndex = names;
  do
  {
    // Grab the next name.
    pIndex = one_argument( pIndex, ch_name );
    // If they already have access
    if( locker_access_canAccess(chLocker, ch_name) )
    {
      send_to_char_f( ch, "'%s' already has access to your locker.\n", ch_name );
    }
    else
    {
      // Insert it into the table
      qry( "INSERT INTO locker_access (owner, visitor) VALUES ('%s', '%s')", locker_name, ch_name );
      send_to_char_f( ch, "'%s' given access to your locker.\n", ch_name );
    }
  } while( pIndex[0] != '\0' );

}
