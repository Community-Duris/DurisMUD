#include <string.h>
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "structs.h"
#include "utils.h"
#include "dreadlord.h"
#include "interp.h"
#include "damage.h"
#include "spells.h"
#include "prototypes.h"
#include "objmisc.h"

void event_dread_wrath(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *af;
  P_char curr_vict;

  curr_vict = ch->specials.fighting;

  if (curr_vict != victim)
  {
    af = get_spell_from_char(victim, TAG_DREAD_WRATH);

    if (af)
    {
      affect_remove(victim, af);
      wear_off_message(victim, af);
      return;
    }
  }
  else
  {
    if (curr_vict)
    {
      af = get_spell_from_char(curr_vict, TAG_DREAD_WRATH);
      if (af)
      {
        af->duration = 1;
        balance_affects(curr_vict);
        add_event(event_dread_wrath, 2 * WAIT_SEC, ch, curr_vict, 0, 0, 0, 0);
      }
    }
  }

}


void do_dread_wrath(P_char ch, P_char victim)
{
  struct affected_type af, *afp;

  if (GET_OPPONENT(ch) != victim)
    return;

  if(IS_CONSTRUCT(victim) || IS_AFFECTED4(victim, AFF4_NOFEAR))
    return;

  afp = get_spell_from_char(victim, TAG_DREAD_WRATH);
  if (!afp)
  {
  act ("&+LA look of sheer horror and dr&+rea&+Ld cover $N&+L's face as $n&+L locks $M in unholy combat.&n",
         FALSE, ch, 0, victim, TO_NOTVICT);
  act ("&+LYou feel terror strike at your very soul as $n&+L draws $s unholy weapon against you!",
         FALSE, ch, 0, victim, TO_VICT);
  act ("&+LYou can almost taste the terror of $N&+L as you fiercely rush to strike $M down!",
          FALSE, ch, 0, victim, TO_CHAR);

  notch_skill(ch, SKILL_DREAD_WRATH, 2);

  memset(&af, 0, sizeof(af));
  af.type = TAG_DREAD_WRATH;
  af.flags = AFFTYPE_NOSAVE;
  af.location = APPLY_AC;
  af.modifier = (int)(GET_CHAR_SKILL(ch, SKILL_DREAD_WRATH) * 2);
  af.duration = 1;
  affect_to_char(victim, &af);
  add_event(event_dread_wrath, 2 * WAIT_SEC, ch, victim, 0, 0, 0, 0);
  }
  else
  {
    afp->duration = 1;
    balance_affects(victim);
  }



}
