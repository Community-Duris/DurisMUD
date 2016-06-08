/*
   ***************************************************************************
   *  File: specs.jot.c                                 Part of Duris        *
   *  Usage: Special Procs for Jot area                                      *
   *  Copyright  1997 - Tim Devlin (Cython)  cython@duris.org                *
   *  Copyright  1994, 1997 - Duris Dikumud                                  *
   ***************************************************************************
 */

#include <stdio.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"

/*
   extern variables
 */

extern P_room world;
extern struct zone_data *zone_table;

/*
 *  Mobile Procs
 *
 */
int halfcut_defenders(P_char ch, P_char player, int cmd, char *arg)
{
  char buf[32];

  if (IS_FIGHTING(ch))
    return FALSE;

  if (!player)
    return FALSE;

  if (IS_NPC(player))
    return FALSE;

  if ((GET_LEVEL(player) < 44) && (!number(0, 20)))
  {
    do_consider(ch, GET_NAME(player), CMD_CONSIDER);
    sprintf( buf, "chuckle" );
    command_interpreter( ch, buf );
    return FALSE;
  }
  /* get here only if not fighting, and pc is 44+ level, attack them */

  do_consider(ch, GET_NAME(player), CMD_CONSIDER);
  do_say(ch, "Hah!  You think you can challenge me for the treasures here?!?",
         0);
  do_say(ch, "&+WHave at thee!&N", 0);
  MobStartFight(ch, player);

  return TRUE;
}

int crossbow_ambusher(P_char ch, P_char player, int cmd, char *arg)
{
  int      targ_rooms[] = { 27139, 27137, 27136, 0 };
  int      x, y;

  if (cmd != CMD_SET_PERIODIC)
  {                             /*only gonan check on periodic calls */
    return FALSE;
  }
  x = 0;
  for (x = 0; real_room0(targ_rooms[x]) != 0; x++)
  {
    for (player = world[real_room0(targ_rooms[x])].people; player;
         player = player->next_in_room)
    {
      if (IS_PC(player))
      {                         /* Wohoo, got a target */
        for (y = 0; y <= 3; y++)
        {                       /* shoot bolts */
          act("A crossbow bolt flies in from the north, striking $N!",
              0, player, 0, player, TO_ROOM);
          act("A crossbow bolt flies in from the north striking you!",
              0, player, 0, player, TO_VICT);
          damage(ch, player, dice(2, 4) + 10, TYPE_UNDEFINED);
        }
      }
    }
  }
  return FALSE;
}
int blowgunner(P_char ch, P_char player, int cmd, char *arg)
{

  int      targ_rooms[] = { 27146, 27144, 27131, 0 };
  int      x, y;

  if (cmd != CMD_SET_PERIODIC)
    return FALSE;
/*  x = 0;*/
  for (x = 0; real_room0(targ_rooms[x]) != 0; x++)
  {
    for (player = world[real_room0(targ_rooms[x])].people; player;
         player = player->next_in_room)
    {
      if (IS_PC(player))
      {
        for (y = 0; y <= 3; y++)
        {
          act("A small dart flies in from the west, striking $N!",
              0, player, 0, player, TO_ROOM);
          act("A small dart flies in from the west striking you!",
              0, player, 0, player, TO_VICT);
          damage(ch, player, dice(1, 5) + 5, TYPE_UNDEFINED);
        }
      }
    }
  }
  return FALSE;
}
