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
