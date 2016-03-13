#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "interp.h"
#include "specs.prototypes.h"

int frost_elb_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      incombat;
  int      is_charged;
  char     can_use, *arg2;
  P_char   victim;

  incombat = (cmd / 1000);
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  if (!obj || !ch)
    return FALSE;
  if (!OBJ_WORN(obj))
    return FALSE;
  can_use = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
             (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
             SECONDARY_WEAPON : 0);
  if (!can_use)
    return FALSE;
  is_charged = IS_OBJ_STAT(obj, ITEM_LIT);
  if (arg && (cmd == CMD_SAY) && is_charged)
  {
    arg = one_argument(arg, arg2);
    if (!strcmp(arg, "frost"))
    {
      act("$n says something to $p&n.", TRUE, ch, obj, 0, TO_ROOM);
      act("You say 'frost' to $p&n.", TRUE, ch, obj, 0, TO_CHAR);
      act("$n's $q &+Lhums loudly, and becomes &+Bbitterly &+Ccold&+W!&n",
          TRUE, ch, obj, 0, TO_ROOM);
      act("Your $q &+Lhums loudly, and becomes &+Bbitterly &+Ccold&+W!&n",
          TRUE, ch, obj, 0, TO_CHAR);
      spell_coldshield(50, ch, NULL, 0, ch, obj);
      REMOVE_BIT(obj->extra_flags, ITEM_LIT);
      return TRUE;
    }
    else
    {
      if (!strcmp(arg, "eld"))
      {
        act("$n says something to $p&n.", TRUE, ch, obj, 0, TO_ROOM);
        act("You say 'eld' to $p&n.", TRUE, ch, obj, 0, TO_CHAR);
        act("$n's $q &+Lhums loudly, and becomes &+Rscorching &+Yhot&+W!&n",
            TRUE, ch, obj, 0, TO_ROOM);
        act("Your $q &+Lhums loudly, and becomes &+Rscorching &+Yhot&+W!&n",
            TRUE, ch, obj, 0, TO_CHAR);
        spell_fireshield(50, ch, NULL, 0, ch, obj);
        REMOVE_BIT(obj->extra_flags, ITEM_LIT);
        return TRUE;
      }
    }
  }
  if (!incombat)                /* past here, we have to be in combat */
    return FALSE;
  victim = ch->specials.fighting;
  if (!victim)
    return FALSE;
  if (!is_charged)
  {
    if (number(1, 100) == 1)
    {
      act("$p &+Lhums softly as it strikes &+W$N&n", TRUE, 0, obj, victim,
          TO_ROOM);
      act("$p &+Lhums softly as it strikes &+W_YOU_&n", TRUE, victim, obj, 0,
          TO_CHAR);
      SET_BIT(obj->extra_flags, ITEM_LIT);
      return TRUE;
    }
  }
  return FALSE;
}

int demon_chick(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;
  if ((cmd == CMD_GET) && pl && (pl != ch) && (isname("statue", arg) ||
                                               isname("golden", arg) ||
                                               isname("totem", arg) ||
                                               isname("all", arg)))
  {
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch,
           "I'm free! And now you puny mortals will pay with your lives!");
    act("&+LGouts of &+Wwhite hot&+L gases pour out from &n$n&n", TRUE, ch, 0,
        0, TO_ROOM);
    spell_incendiary_cloud(31, ch, NULL, 0, ch, 0);
    return TRUE;
  }
  return FALSE;
}

int dagger_submission(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;
  P_obj    junk_obj;
  char     can_use;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  if (!ch || !obj)
    return FALSE;
  if (!OBJ_WORN(obj))
    return FALSE;
  can_use = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
             (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
             SECONDARY_WEAPON : 0);
  if (!can_use)
    return FALSE;
  if (cmd == CMD_BACKSTAB)
  {
    generic_find(arg, FIND_CHAR_ROOM, ch, &victim, &junk_obj);
    if (!victim)
      return FALSE;
    if (backstab(ch, victim))
    {
      spell_feeblemind(40, ch, NULL, 0, victim, 0);
      return TRUE;
    }
  }
  return FALSE;
}

int wristthrow_and_gore(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  if (!ch || !pl)
    return FALSE;
  switch (number(1, 2))
  {
  case 1:
    {
      if (!number(0, 20))
      {
        act("<to the victim>", TRUE, pl, 0, ch, TO_CHAR);
        act("<to the thrower>", TRUE, ch, 0, pl, TO_CHAR);
        act("<to the room>", TRUE, ch, 0, pl, TO_ROOM);
        damage(ch, pl, dice(3, 10), 0);
        CharWait(ch, PULSE_VIOLENCE * 3);
        CharWait(pl, PULSE_VIOLENCE * 2);
        SET_POS(pl, GET_STAT(pl) + POS_PRONE);
        return TRUE;
      }
    }
    break;
  case 2:
    {
      if (!number(0, 20))
      {
        act("to vict", TRUE, pl, 0, ch, TO_CHAR);
        act("to char", TRUE, ch, 0, pl, TO_CHAR);
        act("to room", TRUE, ch, 0, pl, TO_ROOM);
        CharWait(ch, PULSE_VIOLENCE * 2);
        damage(ch, pl, dice(5, 20), 0);
        return TRUE;
      }
    }
  }
  return FALSE;
}
