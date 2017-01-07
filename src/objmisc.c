/*
 * ***************************************************************************
 *   File: objmisc.c                                       Part of Duris
 *   Usage: Miscellaneous stuff related to objects
 *   Copyright  1990, 1991 - see 'license.doc' for complete information.
 *   Copyright  1994, 1995, 1997 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "objmisc.h"
#include "new_combat.h"
#include "utils.h"
#include "damage.h"
#include "db.h"
#include <string.h>

extern P_room   world;                 /* dyn alloc'ed array of rooms     */
extern int rev_dir[];
extern struct zone_data *zone_table;
/*
 * getWeaponDamType
 */

int getWeaponDamType(const int weaptype)
{
  switch (weaptype)
  {
  case WEAPON_SICKLE:
  case WEAPON_2HANDSWORD:
  case WEAPON_SHORTSWORD:
  case WEAPON_LONGSWORD:
  case WEAPON_AXE:
    return WEAPONTYPE_SLASH;

  case WEAPON_LANCE:
  case WEAPON_TRIDENT:
  case WEAPON_HORN:
  case WEAPON_SPEAR:
  case WEAPON_POLEARM:
  case WEAPON_DAGGER:
    return WEAPONTYPE_PIERCE;

  case WEAPON_HAMMER:
  case WEAPON_MACE:
  case WEAPON_SPIKED_MACE:
  case WEAPON_CLUB:
  case WEAPON_SPIKED_CLUB:
  case WEAPON_STAFF:
  case WEAPON_NUMCHUCKS:
    return WEAPONTYPE_BLUDGEON;

  case WEAPON_FLAIL:
  case WEAPON_WHIP:
    return WEAPONTYPE_WHIP;
  }

  return WEAPONTYPE_UNDEFINED;
}

int get_weapon_msg(P_obj weapon)
{
  switch (weapon->value[0])
  {
  case WEAPON_AXE:
  case WEAPON_SHORTSWORD:
  case WEAPON_2HANDSWORD:
  case WEAPON_SICKLE:
  case WEAPON_POLEARM:
  case WEAPON_LONGSWORD:
    return MSG_SLASH;
  case WEAPON_DAGGER:
  case WEAPON_SPEAR:
  case WEAPON_TRIDENT:
  case WEAPON_HORN:
    return MSG_PIERCE;
  case WEAPON_HAMMER:
  case WEAPON_FLAIL:
  case WEAPON_CLUB:
  case WEAPON_SPIKED_CLUB:
  case WEAPON_LANCE:
    return MSG_CRUSH;
  case WEAPON_MACE:
  case WEAPON_SPIKED_MACE:
  case WEAPON_STAFF:
  case WEAPON_NUMCHUCKS:
    return MSG_BLUDGEON;
  case WEAPON_WHIP:
    return MSG_WHIP;
  default:
    return MSG_HIT;
  }
}

void event_random_exit(P_char ch, P_char victim, P_obj obj, void *data)
{
  char buf[512];
  char exit_name[32];
  int exit_dir, s_room, d_room;

  if (obj->value[0] > number(0,99) && OBJ_ROOM(obj) &&
      sscanf(obj->name, "%s exit_%s ", buf, exit_name) &&
      (exit_dir = dir_from_keyword(exit_name)) != -1 &&
      (d_room = real_room(obj->value[1])) != -1)
  {
    s_room = obj->loc.room;
    if (!world[s_room].dir_option[exit_dir])
    {
      CREATE(world[s_room].dir_option[exit_dir], room_direction_data, 1, MEM_TAG_DIRDATA);
      memset(world[s_room].dir_option[exit_dir], 0, sizeof(struct room_direction_data));
    }
    else
    { // if an exit exists, we close off the zone the exit leads to
      // if we are using this as a random exit generator instead leading
      // to the same zone, it's ok, because we remove the closed flag of
      // the destination zone below.  Example result: Desolate is closed,
      // and Desolate Under Fire (default closed) becomes opened.  This
      // will help prevent people shifting into the zone when they shouldn't.
      if (!(zone_table[world[(world[s_room].dir_option[exit_dir])->to_room].zone].flags & ZONE_CLOSED))
      { // close it...
	      SET_BIT(zone_table[world[(world[s_room].dir_option[exit_dir])->to_room].zone].flags, ZONE_CLOSED);
      }
    }
    if (!world[d_room].dir_option[rev_dir[exit_dir]])
    {
      CREATE(world[d_room].dir_option[rev_dir[exit_dir]], room_direction_data, 1, MEM_TAG_DIRDATA);
      memset(world[d_room].dir_option[rev_dir[exit_dir]], 0, sizeof(struct room_direction_data));
    }
    world[s_room].dir_option[exit_dir]->to_room = real_room(obj->value[1]);
    world[d_room].dir_option[rev_dir[exit_dir]]->to_room = s_room;
    if (zone_table[world[d_room].zone].flags & ZONE_CLOSED)
      REMOVE_BIT(zone_table[world[d_room].zone].flags, ZONE_CLOSED);
  }

  extract_obj(obj);
}

/*
 * getMaterialDeflection : given an object, returns percentage of damage
 *                         that material takes that is magically spirited
 *                         away and ignored
 */

float getMaterialDeflection(const int mat /*P_obj obj */ , const P_obj weap)
{
  float    def = 0.0;
  int /*mat, */ weaptype, damtype;

/*  if (!obj) return 0.0;

   mat = obj->material;
 */

  if ((mat < MAT_LOWEST) || (mat > MAT_HIGHEST))
    return 0.0;

  if (weap)
    weaptype = weap->value[0];
  else
    weaptype = WEAPON_CLUB;

  damtype = getWeaponDamType(weaptype);

  /* hell, just give them full deflection..  that'll force the guy to
     change his weapon.  heh */

  if (damtype == WEAPONTYPE_UNDEFINED)
    return 1.0;

  switch (mat)
  {
  case MAT_NONSUBSTANTIAL:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.0;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_FLESH:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.02;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.01;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.01;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.01;
      break;
    }

    break;

  case MAT_CLOTH:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.01;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_BARK:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.01;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.02;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.02;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.02;
      break;
    }

    break;

  case MAT_SOFTWOOD:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.06;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.03;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.03;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.04;
      break;
    }

    break;

  case MAT_HARDWOOD:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.09;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.05;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.05;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.06;
      break;
    }

    break;

  case MAT_SILICON:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.0;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.02;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.01;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.02;
      break;
    }

    break;

  case MAT_CRYSTAL:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.06;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.04;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.04;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.04;
      break;
    }

    break;

  case MAT_CERAMIC:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.01;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.02;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.01;
      break;
    }

    break;

  case MAT_BONE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.08;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.07;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.07;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.07;
      break;
    }

    break;

  case MAT_STONE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.10;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.15;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.12;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.15;
      break;
    }

    break;

  case MAT_HIDE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.04;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.03;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.02;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.03;
      break;
    }

    break;

  case MAT_LEATHER:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.05;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.04;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.03;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.04;
      break;
    }

    break;

  case MAT_CURED_LEATHER:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.06;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.05;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.04;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.05;
      break;
    }

    break;

  case MAT_IRON:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.10;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.10;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.10;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.10;
      break;
    }

    break;

  case MAT_STEEL:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.15;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.15;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.15;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.15;
      break;
    }

    break;

  case MAT_BRASS:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.08;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.08;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.08;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.08;
      break;
    }

    break;

  case MAT_MITHRIL:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.20;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.20;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.20;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.20;
      break;
    }

    break;

  case MAT_ADAMANTIUM:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.25;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.25;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.25;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.25;
      break;
    }

    break;

  case MAT_BRONZE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.12;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.12;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.12;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.12;
      break;
    }

    break;

  case MAT_COPPER:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.08;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.08;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.08;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.08;
      break;
    }

    break;

  case MAT_SILVER:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.08;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.08;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.08;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.08;
      break;
    }

    break;

  case MAT_ELECTRUM:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.08;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.08;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.08;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.08;
      break;
    }

    break;

  case MAT_GOLD:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.08;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.08;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.08;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.08;
      break;
    }

    break;

  case MAT_PLATINUM:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.08;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.08;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.08;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.08;
      break;
    }

    break;

  case MAT_GEM:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.06;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.06;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.06;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.06;
      break;
    }

    break;

  case MAT_DIAMOND:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.12;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.12;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.12;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.12;
      break;
    }

    break;

  case MAT_PAPER:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.01;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_PARCHMENT:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.01;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_LEAVES:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.01;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_RUBY:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.07;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.07;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.07;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.07;
      break;
    }

    break;

  case MAT_EMERALD:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.07;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.07;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.07;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.07;
      break;
    }

    break;

  case MAT_SAPPHIRE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.07;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.07;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.07;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.07;
      break;
    }

    break;

  case MAT_IVORY:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.07;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.05;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.05;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.05;
      break;
    }

    break;

  case MAT_DRAGONSCALE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.20;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.30;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.30;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.30;
      break;
    }

    break;

  case MAT_OBSIDIAN:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.03;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.07;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.05;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.07;
      break;
    }

    break;

  case MAT_GRANITE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.10;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.15;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.12;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.15;
      break;
    }

    break;

  case MAT_MARBLE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.10;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.15;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.12;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.15;
      break;
    }

    break;

  case MAT_LIMESTONE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.07;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.09;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.09;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.09;
      break;
    }

    break;

  case MAT_LIQUID:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.0;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_BAMBOO:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.05;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.04;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.03;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.04;
      break;
    }

    break;

  case MAT_REEDS:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.02;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.01;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.01;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.01;
      break;
    }

    break;

  case MAT_HEMP:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.02;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.01;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.01;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.01;
      break;
    }

    break;

  case MAT_GLASSTEEL:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.15;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.15;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.15;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.15;
      break;
    }

    break;

  case MAT_EGGSHELL:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.0;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_CHITINOUS:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.02;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.01;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.01;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.01;
      break;
    }

    break;

  case MAT_REPTILESCALE:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.03;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.02;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.02;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.02;
      break;
    }

    break;

  case MAT_GENERICFOOD:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.0;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_RUBBER:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.03;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.01;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.01;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.01;
      break;
    }

    break;

  case MAT_FEATHER:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.01;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.0;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.0;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.0;
      break;
    }

    break;

  case MAT_WAX:
    switch (damtype)
    {
    case WEAPONTYPE_BLUDGEON:
      def = 0.03;
      break;
    case WEAPONTYPE_SLASH:
      def = 0.01;
      break;
    case WEAPONTYPE_PIERCE:
      def = 0.01;
      break;
    case WEAPONTYPE_WHIP:
      def = 0.01;
      break;
    }

    break;

  default:
    return 0.0;
  }

  return def;
}


/*
 * getArmorDeflection : takes material, craftsmanship, damage resistance
 *                      bonus, and thickness into account
 */

float getArmorDeflection(const P_obj armor, const P_obj weap)
{
  float    mod_def, mod = 0.0;
  int      craft;

  if (!armor || ((GET_ITEM_TYPE(armor) != ITEM_ARMOR) &&
                 (GET_ITEM_TYPE(armor) != ITEM_SHIELD)))
    return getMaterialDeflection(armor->material, weap);

  mod_def = getMaterialDeflection(armor->material, weap);

  /* max of 5% + 7% + 5% for a total of 17% added possible */

  /* first, thickness - doesn't affect deflection much */

  if (GET_ITEM_TYPE(armor) == ITEM_ARMOR)
  {
    switch (armor->value[3])
    {
    case ARMOR_THICKNESS_VERY_THIN:
      mod -= 0.05;
      break;

    case ARMOR_THICKNESS_THIN:
      mod -= 0.02;
      break;

    case ARMOR_THICKNESS_THICK:
      mod += 0.02;
      break;

    case ARMOR_THICKNESS_VERY_THICK:
      mod += 0.05;
      if (mod_def < 0.01)
        mod_def = 0.01;
      break;
    }
  }
  else if (GET_ITEM_TYPE(armor) == ITEM_SHIELD)
  {
    switch (armor->value[4])
    {
    case ARMOR_THICKNESS_VERY_THIN:
      mod -= 0.05;
      break;

    case ARMOR_THICKNESS_THIN:
      mod -= 0.02;
      break;

    case ARMOR_THICKNESS_THICK:
      mod += 0.02;
      break;

    case ARMOR_THICKNESS_VERY_THICK:
      mod += 0.05;
      if (mod_def < 0.01)
        mod_def = 0.01;
      break;
    }
  }
  /* next, craftsmanship (should be 0-15) */

  mod +=
    (BOUNDED(OBJCRAFT_LOWEST, armor->craftsmanship, OBJCRAFT_HIGHEST) -
     OBJCRAFT_AVERAGE) * 0.01;

  /* finally let's apply a little damage resistance bonus (0.0-0.05) */

//  mod += ((float) BOUNDED(0, armor->damres_bonus, 100) / 2000.0);

  mod += mod_def;
  if (mod < 0.0)
    mod = 0.0;
  if (mod > MAX_ARMOR_DEFLECTION)
    mod = MAX_ARMOR_DEFLECTION;

  return (ARMOR_DEFLECTION_MULT * mod);
}


/*
 * getMaterialAbsorbtion : let's make this easy and just use the deflection
 *                         numbers times two..  if necessary it can be
 *                         broken down individually later
 */

float getMaterialAbsorbtion(const int mat, /*P_obj obj, */ const P_obj weap)
{
  return (BOUNDED(0, (int) (getMaterialDeflection(mat, weap) * 2), 1));
}


/*
 * getArmorAbsorbtion : takes material, craftsmanship, damage resistance
 *                      bonus, and thickness into account
 */

float getArmorAbsorbtion(const P_obj armor, const P_obj weap)
{
  float    mod_def, mod = 0.0;
  int      craft;

  if (!armor || ((GET_ITEM_TYPE(armor) != ITEM_ARMOR) &&
                 (GET_ITEM_TYPE(armor) != ITEM_SHIELD)))
    return getMaterialAbsorbtion(armor->material, weap);

  mod_def = getMaterialAbsorbtion(armor->material, weap);

  /* max of 10% + 14% + 10% for a highest multiplier of 34% possible */

  /* first, thickness - affects absorbtion a bit */

  if (GET_ITEM_TYPE(armor) == ITEM_ARMOR)
  {
    switch (armor->value[3])
    {
    case ARMOR_THICKNESS_VERY_THIN:
      mod -= 0.10;
      break;

    case ARMOR_THICKNESS_THIN:
      mod -= 0.05;
      break;

    case ARMOR_THICKNESS_THICK:
      mod += 0.05;
      if (mod_def < 0.01)
        mod_def = 0.01;
      break;

    case ARMOR_THICKNESS_VERY_THICK:
      mod += 0.10;
      if (mod_def < 0.02)
        mod_def = 0.02;
      break;
    }
  }
  else if (GET_ITEM_TYPE(armor) == ITEM_SHIELD)
  {
    switch (armor->value[4])
    {
    case ARMOR_THICKNESS_VERY_THIN:
      mod -= 0.10;
      break;

    case ARMOR_THICKNESS_THIN:
      mod -= 0.05;
      break;

    case ARMOR_THICKNESS_THICK:
      mod += 0.05;
      if (mod_def < 0.01)
        mod_def = 0.01;
      break;

    case ARMOR_THICKNESS_VERY_THICK:
      mod += 0.10;
      if (mod_def < 0.02)
        mod_def = 0.02;
      break;
    }
  }
  /* next, craftsmanship (should be 0-15) */

  mod +=
    (BOUNDED(OBJCRAFT_LOWEST, armor->craftsmanship, OBJCRAFT_HIGHEST) -
     OBJCRAFT_AVERAGE) * 0.02;

  /* finally let's apply a little damage resistance bonus (0.0-0.10) */

//  mod += ((float) BOUNDED(0, armor->damres_bonus, 100) / 1000.0);

  if (mod < 0.0)
    mod = 0.0;

  return (ARMOR_ABSORBTION_MULT *
          BOUNDED(0, (int) (mod_def * (1.0 + mod)),
                  (int) MAX_ARMOR_ABSORBTION));
}


/*
 * getMaterialMaxSP
 */

int getMaterialMaxSP(const int material)
{
  switch (material)
  {
  case MAT_NONSUBSTANTIAL:
    return 100;
    break;
  case MAT_FLESH:
    return 200;
    break;
  case MAT_CLOTH:
    return 75;
    break;
  case MAT_BARK:
    return 300;
    break;
  case MAT_SOFTWOOD:
    return 450;
    break;
  case MAT_HARDWOOD:
    return 600;
    break;
  case MAT_SILICON:
    return 100;
    break;
  case MAT_CRYSTAL:
    return 250;
    break;
  case MAT_CERAMIC:
    return 150;
    break;
  case MAT_BONE:
    return 500;
    break;
  case MAT_STONE:
    return 1000;
    break;
  case MAT_HIDE:
    return 400;
    break;
  case MAT_LEATHER:
    return 400;
    break;
  case MAT_CURED_LEATHER:
    return 500;
    break;
  case MAT_IRON:
    return 750;
    break;
  case MAT_STEEL:
    return 1500;
    break;
  case MAT_BRASS:
    return 650;
    break;
  case MAT_MITHRIL:
    return 2500;
    break;
  case MAT_ADAMANTIUM:
    return 3000;
    break;
  case MAT_BRONZE:
    return 850;
    break;
  case MAT_COPPER:
    return 800;
    break;
  case MAT_SILVER:
    return 800;
    break;
  case MAT_ELECTRUM:
    return 850;
    break;
  case MAT_GOLD:
    return 600;
    break;
  case MAT_PLATINUM:
    return 1000;
    break;
  case MAT_GEM:
    return 1250;
    break;
  case MAT_DIAMOND:
    return 1750;
    break;
  case MAT_PAPER:
    return 30;
    break;
  case MAT_PARCHMENT:
    return 50;
    break;
  case MAT_LEAVES:
    return 40;
    break;
  case MAT_RUBY:
    return 1300;
    break;
  case MAT_EMERALD:
    return 1300;
    break;
  case MAT_SAPPHIRE:
    return 1300;
    break;
  case MAT_IVORY:
    return 450;
    break;
  case MAT_DRAGONSCALE:
    return 2000;
    break;
  case MAT_OBSIDIAN:
    return 500;
    break;
  case MAT_GRANITE:
    return 1100;
    break;
  case MAT_MARBLE:
    return 1100;
    break;
  case MAT_LIMESTONE:
    return 800;
    break;
  case MAT_LIQUID:
    return 1;
    break;
  case MAT_BAMBOO:
    return 350;
    break;
  case MAT_REEDS:
    return 100;
    break;
  case MAT_HEMP:
    return 100;
    break;
  case MAT_GLASSTEEL:
    return 1500;
    break;
  case MAT_EGGSHELL:
    return 10;
    break;
  case MAT_CHITINOUS:
    return 200;
    break;
  case MAT_REPTILESCALE:
    return 300;
    break;
  case MAT_GENERICFOOD:
    return 50;
    break;
  case MAT_RUBBER:
    return 200;
    break;
  case MAT_FEATHER:
    return 50;
    break;
  case MAT_WAX:
    return 125;
    break;

  default:
    return 10;
  }
}

/*
 * getArmorMaxSP
 */

#if 0
int getArmorMaxSP(const P_obj armor)
{
  int      maxSP, matSP;
  float    mod = 0.0;


  if (!armor)
    return 0;

  matSP = getMaterialMaxSP(armor->material);
  maxSP = getItemMaxSP(armor);

  /* max SP can be affected by +/-85% after being increased anywhere from
     0%-100% by the damage resistance bonus */

  /* first, thickness - affects max SP radically */

  switch (armor->value[3])
  {
  case ARMOR_THICKNESS_VERY_THIN:
    mod -= 0.50;
    break;

  case ARMOR_THICKNESS_THIN:
    mod -= 0.25;
    break;

  case ARMOR_THICKNESS_THICK:
    mod += 0.25;
    break;

  case ARMOR_THICKNESS_VERY_THICK:
    mod += 0.50;
    break;
  }

  /* next, craftsmanship (should be 0-15, thus max of -35% to 35%) */

  mod +=
    (BOUNDED(OBJCRAFT_LOWEST, armor->craftsmanship, OBJCRAFT_HIGHEST) -
     OBJCRAFT_AVERAGE) * 0.05;

  /* damage resistance bonus can up to double armor's SP */

  maxSP += (BOUNDED(0, armor->damres_bonus, 100) / 100.0) * matSP;

  maxSP *= BOUNDED(-0.75, mod, 0.75);

  return (maxSP * MAX_SP_ARMOR_MULT);
}
#endif

int obj_zone_id(P_obj o)
{
  P_obj tobj = o;

  while (tobj && OBJ_INSIDE(tobj))
    tobj = tobj->loc.inside;

  int zone_id = -1;

  if( !tobj )
  {
    return -1;
  }
  else if( OBJ_ROOM(tobj) )
  {
    zone_id = world[tobj->loc.room].zone;
  }
  else if( OBJ_CARRIED(tobj) && tobj->loc.carrying->in_room != NOWHERE )
  {
    zone_id = world[tobj->loc.carrying->in_room].zone;
  }
  else if( OBJ_WORN(tobj) && tobj->loc.wearing->in_room != NOWHERE )
  {
    zone_id = world[tobj->loc.wearing->in_room].zone;
  }
  
  return zone_id;
}

int obj_room_id(P_obj o)
{
  P_obj tobj = o;
  
  while (tobj && OBJ_INSIDE(tobj))
    tobj = tobj->loc.inside;
  
  int room_id = -1;
  
  if( !tobj )
  {
    return -1;
  }
  else if( OBJ_ROOM(tobj) )
  {
    room_id = tobj->loc.room;
  }
  else if( OBJ_CARRIED(tobj) && tobj->loc.carrying->in_room != NOWHERE )
  {
    room_id = tobj->loc.carrying->in_room;
  }
  else if( OBJ_WORN(tobj) && tobj->loc.wearing->in_room != NOWHERE )
  {
    room_id = tobj->loc.wearing->in_room;
  }
  
  return room_id;
}

