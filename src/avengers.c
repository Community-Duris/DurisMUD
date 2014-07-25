#include <string.h>
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "structs.h"
#include "avengers.h"
#include "interp.h"
#include "damage.h"
#include "utils.h"
#include "spells.h"
#include "prototypes.h"
#include "objmisc.h"

void spell_holy_sword(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if (affected_by_spell(ch, SPELL_HOLY_SWORD))
  {
    send_to_char("&+WYou cannot imbue your weapon with any more holy powers.&n\n", ch);
    return;
  }

  if ( !ch->equipment[WIELD] || !IS_SWORD( ch->equipment[WIELD] ))
  {
    send_to_char("&+WThe Gods refuse to bless your anything but your sword.&n.\n", ch);
    return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_HOLY_SWORD;
  af.duration = level/2;
  af.location = APPLY_DAMROLL;
  af.modifier = 5;
  affect_to_char(ch, &af);

  af.location = APPLY_HITROLL;
  af.modifier = 5;
  affect_to_char(ch, &af);

  send_to_char("&+WHoliness flows down from the heavens, and imbues your sword with might!&n\n", ch);
}

void spell_divine_power(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  struct affected_type af;
  int      healpoints = level;

  if (!IS_GOOD(ch))
  {
    send_to_char("Nothing seems to happen.\n", ch);
    return;
  }
  if (!affected_by_spell(ch, SPELL_DIVINE_POWER))
  {
    act
    ("&+WHoly power surrounds $n&+W, and imbues him with it's might!&n",
     FALSE, ch, 0, 0, TO_ROOM);
     send_to_char("&+WA wave of holy energy sweeps over your body.\n", ch);

	 bzero(&af, sizeof(af));

     af.type = SPELL_DIVINE_POWER;
     af.duration = level/2;
     af.location = APPLY_HITROLL;
     af.modifier = number(2, 5);
     affect_to_char(ch, &af);

     af.location = APPLY_STR_MAX;
     af.modifier = number(2, 5);
     affect_to_char(ch, &af);

	if (GET_LEVEL(ch) > 49)
	{
     af.location = APPLY_HIT;
     af.modifier = healpoints;
     affect_to_char(ch, &af);
	}
  }
}

void spell_atonement(int level, P_char ch, char *arg, int type,
                        P_char victim, P_obj obj)
{
  struct affected_type af;
  char     Gbuf1[100];

  if (affected_by_spell(ch, SPELL_ATONEMENT))
  {
    send_to_char("&+wFurther forgiveness cannot be achieved.&n\n", ch);
    return;
  }

  sprintf(Gbuf1, "&+WYou atone for your sins, and feel your holy power begin to heal your body.&n\n");

  bzero(&af, sizeof(af));
  af.type = SPELL_ATONEMENT;
  af.duration = level / 2;
  af.location = APPLY_HIT_REG;
  af.modifier = level * 2;
  send_to_char(Gbuf1, ch);
  affect_to_char(ch, &af);
}

//some skill type stuff, thrown in here for continuty
void do_holy_smite(P_char ch, char *argument, int cmd)
{
  P_char   vict = NULL;
  char     name[100];
  int      skl_lvl = 0;
  struct damage_messages messages = {
    "Calling forth the virtue of the gods, you smite $N with holy power!",
    "$n smirks proudly, as $e burns your soul with $s holy power!",
    "$n's smirks, chanting to $mself, and divine energy ravages $N's body!",
    "$N is utterly dissolved by your mighty torrent of divine energy!",
    "$n smirks a final time, as you feel your very soul being torn apart divine energy!",
    "$N falls to the ground, $s soul purged and cleansed by $n.",
      0
  };

  if ((skl_lvl = GET_CHAR_SKILL(ch, SKILL_HOLY_SMITE)) == 0)
  {
    send_to_char("You don't know how.\r\n", ch);
    return;
  }

  if (!*argument)
  {
    if (!(vict = ch->specials.fighting))
    {
      send_to_char
        ("Smite whom?\r\n",
         ch);
      return;
    }
  }
  if (!vict)
  {
    one_argument(argument, name);

    if (!(vict = get_char_room_vis(ch, name)))
    {
      send_to_char
        ("Smite whom?\r\n",
         ch);
      return;
    }
  }
  if (!CanDoFightMove(ch, vict))
    return;

  if (GET_ALIGNMENT(vict) > -1)
  {
   send_to_char
	 ("Your conscience prevents you from executing such a maneuver on this being.\r\n",
	  ch);
   return;
  }
  if (!notch_skill(ch, SKILL_HOLY_SMITE, get_property("skill.notch.offensive", 7))
    && number(1, 101) > skl_lvl)
  {
    act("You fruitlessly attempt to judge $N's sins.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n's attempt to judge your sins causes you to laugh malevolently.", FALSE, ch,
        0, vict, TO_VICT);
    act("$N laughs derisively at $n, as $e attempts to purify $S soul.", FALSE, ch, 0, vict,
        TO_NOTVICT);
	engage(ch, vict);
	CharWait(ch, PULSE_VIOLENCE);
	return;
  }
  else
  {
    melee_damage(ch, vict, (dice((GET_LEVEL(ch) / 2), (skl_lvl / 10)) / 2), PHSDAM_NOSHIELDS | PHSDAM_NOREDUCE | PHSDAM_NOPOSITION, &messages);
    if ((GET_LEVEL(ch) / 2) > number(1, 150) && IS_ALIVE(vict))
	{
	  act("&+L$N&+L is blinded by the &+Wholy&+L energy!&n", FALSE, ch, 0, vict, TO_CHAR);
      act("&+LYou are blinded by the &+Wholy&+L energy!&n", FALSE, ch,
        0, vict, TO_VICT);
      act("&+L$N &+Lis blinded by the &+Wholy&+L energy!&n", FALSE, ch, 0, vict,
        TO_NOTVICT);
	  blind(ch, vict, 25 * WAIT_SEC);
	}
  }
  CharWait(ch, 2 * PULSE_VIOLENCE);
}

void spell_celestial_aura(int level, P_char ch, char *arg, int type,
                                   P_char victim, P_obj obj)
{
  struct affected_type af;

  //The affects here are temporary, until I have time to whip up something really unique -Zion 9/13/08
  if (!affected_by_spell(ch, SPELL_CELESTIAL_AURA))
  {
    act("&+WA beam of light emerges from the heavens, infusing $n&+W's body with h&+Yol&+Wy energy!&n",
        TRUE, ch, 0, 0, TO_ROOM);
    act("&+WA column of holy light sweeps over your body; your prayers are answered!&n",
        TRUE, ch, 0, 0, TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_CELESTIAL_AURA;
    af.duration =  4;
    af.location = APPLY_AC;
    af.modifier = -number(80, 150);
    affect_to_char(ch, &af);

    af.location = APPLY_HIT;
    af.modifier = number(50, 150);
    affect_to_char(ch, &af);
  }
}
