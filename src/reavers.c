#include <string.h>
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "structs.h"
#include "reavers.h"
#include "interp.h"
#include "damage.h"
#include "utils.h"
#include "spells.h"
#include "prototypes.h"
#include "objmisc.h"

void spell_baladors_protection(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if (affected_by_spell(ch, SPELL_BALADORS_PROTECTION))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if (af1->type == SPELL_BALADORS_PROTECTION)
      {
        af1->duration = 32;
      }
    }
    return;
  }
  bzero(&af, sizeof(af));

  if (GET_LEVEL(ch) > 25)
  {
    af.type = SPELL_BALADORS_PROTECTION;
    af.duration = 32;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char(victim, &af);

    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char(victim, &af);
  }

  if (GET_LEVEL(ch) > 45)
  {
    af.type = SPELL_BALADORS_PROTECTION;
    af.duration = 32;
    af.location = APPLY_SAVING_SPELL;
    af.modifier = -3;
    affect_to_char(victim, &af);

    af.location = APPLY_SAVING_PARA;
    af.modifier = -3;
    affect_to_char(victim, &af);

    af.location = APPLY_SAVING_ROD;
    af.modifier = -3;
    affect_to_char(victim, &af);

    af.location = APPLY_SAVING_FEAR;
    af.modifier = -3;
    affect_to_char(victim, &af);

    af.location = APPLY_SAVING_BREATH;
    af.modifier = -3;
    affect_to_char(victim, &af);
  }

//  if( GET_LEVEL(ch) > 4 && !IS_AFFECTED(victim, AFF_ARMOR) )
  // Taking the AFF_ARMOR part out of this so it'll give some ac regardless.
  if( GET_LEVEL(ch) > 4 )
  {
    af.type = SPELL_BALADORS_PROTECTION;
    af.duration = 32;
    af.modifier = -20;

//    af.bitvector = AFF_ARMOR;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
  }

  send_to_char("&+WYou feel Balador's protection wash over you.&n\n", victim);
}

void spell_ferrix_precision(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if (affected_by_spell(ch, SPELL_FERRIX_PRECISION))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
    {
      if (af1->type == SPELL_FERRIX_PRECISION)
      {
        af1->duration = 32;
      }
    }
    return;
  }
  bzero(&af, sizeof(af));

  af.type = SPELL_FERRIX_PRECISION;
  af.duration = 32;
  af.modifier = ((int) (level / 7)) + 2;
  af.location = APPLY_DEX_MAX;
  affect_to_char(victim, &af);

  af.duration =  32;
  af.modifier = ((int) (level / 7)) + 2;
  af.location = APPLY_HITROLL;
  affect_to_char(victim, &af);


  send_to_char("&+BYour precision in battle has increased greatly!&n\n", ch);

}

void spell_eshabalas_vitality(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool message = false;
  int      healpoints = (2 * level) + number(40, 90);
  
  if(affected_by_spell(ch, SPELL_MIELIKKI_VITALITY))
  {
    send_to_char("&+WThe vitality spell fails.\r\n", ch);
    return;
  }

  if(affected_by_spell(ch, SPELL_VITALITY))
  {
    send_to_char("&+rThe vitality spell is negated by Eshabala!\r\n", victim);
    return;
  }

  if (affected_by_spell(ch, SPELL_ESHABALAS_VITALITY))
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_ESHABALAS_VITALITY)
      {
        af1->duration = 15;
        message = true;
      }
    
    if(message)
      send_to_char("&+mEshabalas vitality flows through you!&n\n", ch);
      
    return;
  }
  bzero(&af, sizeof(af));
  af.type = SPELL_ESHABALAS_VITALITY;
  af.duration = 15;
  af.modifier = healpoints;
  af.location = APPLY_HIT;
  affect_to_char(victim, &af);

  /*
  af.duration = 15;
  af.modifier = number(20, 45);
  af.location = APPLY_MOVE;
  affect_to_char(victim, &af);
  */

  send_to_char("&+mEshabalas vitality flows through you!&n\n", ch);
}

void spell_kanchelsis_fury(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;

  if( !affected_by_spell(victim, SPELL_KANCHELSIS_FURY) )
  {
    send_to_char("&+mYou feel your heart start to burn as the fury of Kanchelsis takes hold!\n", victim);
    act("$n &+mlooks stronger and starts to move with uncanny speed!", TRUE, victim, 0, 0, TO_ROOM);
    bzero(&af, sizeof(af));

    // Earth reavers are a bit slower.
    if( GET_SPEC(ch, CLASS_REAVER, SPEC_EARTH_REAVER) )
    {
      level -= 3;
    }
    af.type = SPELL_KANCHELSIS_FURY;
    af.duration = 10;
    af.modifier = (level / 7) + 2;
    af.location = APPLY_STR_MAX;
    affect_to_char(victim, &af);
    if( level >= 56 )
    {
      af.location = APPLY_COMBAT_PULSE;
      af.modifier = -3;
      affect_to_char(victim, &af);
    }
    else if( level >= 53 )
    {
      af.location = APPLY_COMBAT_PULSE;
      af.modifier = -2;
      affect_to_char(victim, &af);
    }
    else if( level >= 51 )
    {
      af.location = APPLY_COMBAT_PULSE;
      af.modifier = -1;
      affect_to_char(victim, &af);
    }
  }
  else
  {
    struct affected_type *af1;

    for (af1 = victim->affected; af1; af1 = af1->next)
      if (af1->type == SPELL_KANCHELSIS_FURY)
      {
        af1->duration = 10;
      }
  }
}

void spell_blood_alliance(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  if (get_linking_char(victim, LNK_BLOOD_ALLIANCE)) {
    send_to_char("This person has already granted blood allegiance to someone.\n", ch);
    return;
  }

  if (!is_linked_to(ch, victim, LNK_CONSENT)) {
    send_to_char("You need consent from your friend to seal your alliance.\n",
                 ch);
    return;
  }

  if (ch == victim) {
    send_to_char("Nothing happens.\n", ch);
    return;
  }

  act("Your forearm glows &+rred&n as you align it with $N's forearm.",
      FALSE, ch, 0, victim, TO_CHAR);
  act("Your forearm glows &+rred&n as $n aligns it with $s forearm.",
      FALSE, ch, 0, victim, TO_VICT);
  act("&+rRed glow&n surrounds $n's and $N's forearms as they align them "
      "with each other.", FALSE, ch, 0, victim, TO_NOTVICT);

  link_char(ch, victim, LNK_BLOOD_ALLIANCE);
}

void check_blood_alliance(P_char ch, int dam)
{
  P_char linked;
  struct affected_type *afp, af;

  if (GET_HIT(ch) >= GET_MAX_HIT(ch) * 0.5)
    return;

  linked = get_linking_char(ch, LNK_BLOOD_ALLIANCE);

  if (!linked)
    linked = get_linked_char(ch, LNK_BLOOD_ALLIANCE);

  if (linked) {
    afp = get_spell_from_char(ch, SPELL_BLOOD_ALLIANCE);
    if (afp)
      afp->modifier += dam;
    else {
      memset(&af, 0, sizeof(af));
      af.type = SPELL_BLOOD_ALLIANCE;
      af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
      af.modifier = dam;
      af.duration = 1;
      affect_to_char(ch, &af);
      add_event(event_blood_alliance, 0, ch, linked, 0, 0, 0, 0);
    }
  }
}

void event_blood_alliance(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char linked;
  struct affected_type *af;
  int sdam;

  linked = get_linking_char(ch, LNK_BLOOD_ALLIANCE);

  if (!linked)
    linked = get_linked_char(ch, LNK_BLOOD_ALLIANCE);

  if (linked->in_room != ch->in_room)
    return;

  if (GET_HIT(linked) < GET_MAX_HIT(linked) * 0.7)
    return;

  if (!(af = get_spell_from_char(ch, SPELL_BLOOD_ALLIANCE)))
    return;

  sdam = (int)(get_property("damage.reduction.bloodAlliance", 0.4) *
               af->modifier);
  sdam = MIN(sdam, GET_HIT(linked) - 5);

  GET_HIT(linked) -= sdam;
  vamp(ch, sdam, GET_MAX_HIT(ch));
  if (sdam > number(0,40)) {
    act("You share some of your blood with $N.", FALSE, linked, 0, ch, TO_CHAR);
    if (linked->desc)
      linked->desc->prompt_mode = TRUE;
  }

  affect_remove(ch, af);
}

void spell_ilienzes_flame_sword(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool weapon = false;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD))
  {
    send_to_char("Your weapon is already on &+Rfire!!\n", ch);
    return;
  }

  if(ch->equipment[WIELD] &&
    FLAME_REAVER_WEAPONS(ch->equipment[WIELD]))
  {
    weapon = true;
  }
    
  if(!weapon &&
     ch->equipment[WIELD2] &&
     FLAME_REAVER_WEAPONS(ch->equipment[WIELD2]))
  {
    weapon = true;
  }
  
  if(!weapon)
  {
    send_to_char("You need to wield the correct type of weapon!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_ILIENZES_FLAME_SWORD;
  af.duration = level/2;
  af.location = APPLY_HITROLL;
  af.modifier = number(1, 4);

  affect_to_char(victim, &af);

  send_to_char("&+rIlienze's &+Lpower&+r &+Rflows&+r down inflaming your weapon!!&n\n", ch);
}

bool ilienze_sword(P_char ch, P_char victim, P_obj wpn)
{
  int dam;
  char ch_msg[256], vict_msg[256], room_msg[256];
  struct affected_type *afp, af;

  struct damage_messages messages = {
    "&+LYour&N $q &+Rburns&n $N &+Rwith a godly fire!&n",
    "&+L$n's&N $q &+Rburns you with a godly fire!&n",
    "&+L$n's&N $q &+Rburns&n $N &+Rwith a godly fire!&n",
    "&+LYour&N $q &+Rburned&n $N &+Rto a crisp!&n",
    "&+L$n's&N $q's &+Rgodly fire burned&n $N &+Rto death!&n",
    "&+L$n's&N $q &+Rburned&n $N &+Rto a crisp!&n",
    DAMMSG_TERSE
  };

  struct damage_messages proc_messages = {
    ch_msg, vict_msg, room_msg,
    "&+rUnholy &+Rflames&n &+rflow down your blade and devour $N's &+rbody in a &+Rdancing inferno&n.",
    "&+rUnholy &+Rflames&n &+rflow down $n's &+rblade and devour your body in a &+Rdancing inferno&n.",
    "&+rUnholy &+Rflames&n &+rflow down $n's &+rblade and devour $N's &+rbody in a &+Rdancing inferno&n.",
    DAMMSG_TERSE
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || !(wpn) || !FLAME_REAVER_WEAPONS(wpn) )
  {
    return false;
  }

  messages.obj = wpn;
  
  dam = number(1, (GET_LEVEL(ch) / 4));

  if(IS_AFFECTED2(ch, AFF2_FIRESHIELD))
  {
    dam += number(2, 8);
  }

  if(IS_AFFECTED2(victim, AFF2_FIRESHIELD))
  {
    dam = 1;
  }
  
  if(spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD)
  {
    return TRUE;
  }
  
  if(IS_AFFECTED2(ch, AFF2_FIRESHIELD) &&
    has_innate(ch, INNATE_IMMOLATE))
  {
    afp = get_spell_from_char(ch, TAG_ILIENZE_SWORD_CHARGE);

    if (!afp)
    {
      memset(&af, 0, sizeof(af));
      af.type = TAG_ILIENZE_SWORD_CHARGE;
      af.modifier = 1;
      af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
      af.duration = 2;
      afp = affect_to_char(ch, &af);
      send_to_char("&+rYou start to gather the &+Rf&+Yl&+Ra&+rmes&n &+raround your blade..&n\n", ch);
    }
    else if (afp->modifier > number(4,40))
    {
      if (afp->modifier < 8)
      {
        ilienze_sword_proc_messages(&proc_messages, "&+Rf&+Yi&+Rr&+re&+rball&n");
      }
      else if (afp->modifier < 14)
      {
        ilienze_sword_proc_messages(&proc_messages, "&+Rf&+Yi&+Rr&+Re&+rb&+rall");
      }
      else if (afp->modifier < 21)
      {
        ilienze_sword_proc_messages(&proc_messages, "&+Rf&+Yi&+Rr&+Re&+Rb&+rall&n");
      }
      else
      {
        ilienze_sword_proc_messages(&proc_messages, "&+Rf&+Yi&+Rr&+Re&+Rb&+Ra&+rll&n");
      }

      dam = dice(afp->modifier, 12);
      afp->modifier = 0;

      return( spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &proc_messages) != DAM_NONEDEAD );
    }
    else
    {
      afp->modifier++;
    }
  }

  return FALSE;
}

void ilienze_sword_proc_messages(struct damage_messages *messages, const char *sub)
{
  snprintf(messages->attacker, MAX_STRING_LENGTH,
          "&+rUnholy &+Rflames&n &+rflow down your blade and shoot in a %s &+rtowards $N&+r.&n", sub);
  snprintf(messages->victim, MAX_STRING_LENGTH,
          "&+rUnholy &+Rflames&n &+rflow down $n's &+rblade and shoot in a %s &+rtowards &+Ryou&+r.&n", sub);
  snprintf(messages->room, MAX_STRING_LENGTH,
          "&+rUnholy &+Rflames&n &+rflow down $n's &+rblade and shoot in a %s &+rtowards $N&+r.&n", sub);
}

void cegilunes_broken( struct char_obj_link_data *cold )
{
  wear_off_message( cold->ch, cold->affect );
}

void spell_cegilunes_searing_blade(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool weapon = FALSE;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  // If they don't have a prime or secondary reaver weapon.
  if( !(( (obj = ch->equipment[WIELD]) && FLAME_REAVER_WEAPONS(obj) )
    || ( (obj = ch->equipment[WIELD2]) && FLAME_REAVER_WEAPONS(obj) )
    || ( (obj = ch->equipment[WIELD3]) && FLAME_REAVER_WEAPONS(obj) )
    || ( (obj = ch->equipment[WIELD4]) && FLAME_REAVER_WEAPONS(obj) )) )
  {
    send_to_char("You need to wield the correct type of weapon!\n", ch);
    return;
  }

  if( affected_by_spell(ch, SPELL_CEGILUNE_BLADE) )
  {
    send_to_char("&+LCegilune can infuse your weapon no further.&n\n", ch);
    return;
  }

  if( !affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD) )
  {
    send_to_char("&+LYour weapon glows for a moment, and then grows cool once more.&n\n", ch);
    return;
  }

  struct affected_type *afp = get_spell_from_char(ch, SPELL_ILIENZES_FLAME_SWORD);

  bzero(&af, sizeof(af));

  af.type = SPELL_CEGILUNE_BLADE;
  af.duration = MIN(afp->duration,  level / 2);
  af.location = APPLY_NONE;
  af.modifier = 0;

  linked_affect_to_char_obj( ch, &af, obj, LNK_CEGILUNE );

  act("&+LYour weapon glows with a strange &+Rh&+rea&+Rt&+L as Cegilune infuses it.&n", false, ch, 0, 0, TO_CHAR);
}

// Called by reaver_hit_proc, this function handles the cegilune's proc.
bool cegilune_blade(P_char ch, P_char victim, P_obj wpn)
{
  int dam, wave;
  struct affected_type *afp, *af2p, af;
  struct damage_messages tiers_messages[6] = {
  {"&+LThe &+Rs&+re&+Ra&+rr&+Ri&+rn&+Rg &+Lblade slices into $N, causing $m to scream in pain!&n",
    "&+L$n's $q &+Lglows &+rf&+Rie&+rry &+wh&+Wo&+wt &+Las it painfully brands your flesh.&n",
    "&+L$N screams in &+rt&+Re&+rr&+Rr&+ro&+Rr &+Las $n's $q &+Lsears past $S armor into $S flesh!&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    "&+LAs $n's $q &+Lstrikes you, it opens up a fatal w&+roun&+Ld. The last thing you sense is the burning of your own &+rflesh&+L.&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    DAMMSG_TERSE},
  {"&+LYour $q &+Ltakes on a mind of it's own as it &+rb&+Ru&+rr&+Rn&+rs&+L deep into $N's flesh.&n",
    "&+w$n's $q &+Lglows &+rf&+Rie&+rry &+wh&+Wo&+wt &+was it painfully brands your flesh.&n",
    "&+L$N screams in &+rbl&+Roo&+rd&+wc&+rur&+Rdl&+rin&+Rg &+Lpain as $n's $q &+Lsears deep into $S flesh.&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    "&+LAs $n's $q strikes you, it opens up a fatal w&+roun&+Ld. The last thing you sense is the burning of your own &+rflesh&+L.&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    DAMMSG_TERSE},
  {"&+rSe&+Rar&+wi&+rng &+Lf&+rur&+Ly fills your $q &+Las it rends $N's &+rf&+Rl&+re&+Rs&+rh&+L, leaving a horrid stench.&n",
    "&+L$n thrusts $s $q &+Linto your flesh with &+wbli&+Wndin&+wg &+Lf&+rur&+Ly, causing you great &+rse&+Rar&+wi&+rng &+Lp&+Ra&+ri&+Ln!&n",
    "&+L$n's $q &+L&+Lmoves in a &+wbli&+Wndin&+wg &+Lrush of &+rse&+Rar&+wi&+rng &+Lf&+rur&+Ly as it slices into $N!&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    "&+LAs $n's $q strikes you, it opens up a fatal w&+roun&+Ld. The last thing you sense is the burning of your own &+rflesh&+L.&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    DAMMSG_TERSE},
  {"&+LYou press your attack, and $q &+Lglows with a &+Wwh&+wit&+We &+rh&+Ro&+rt &+rf&+Rl&+ra&+Rm&+re&+L, nearly igniting $N's &+rflesh&+L!&n",
    "&+L$n gazes into your eyes, &+rh&+Rell&+rfi&+Lr&+we &+Ldancing in $s pupils. Suddenly, $q &+Lcomes alive with &+rs&+Rear&+Lin&+rg &+rf&+Rl&+ra&+Rm&+re&+Rs &+Las it slashes into you!",
    "&+L$n fiercely presses $s attack on $N, and $q &+Lglows with &+Wwh&+wit&+We &+rh&+Ro&+rt &+rf&+Rl&+ra&+Rm&+re&+L, cleaving into $S flesh with it's &+rd&+Rem&+Lon&+ri&+Rc &+rh&+Rell&+rfi&+Lr&+we&+L!&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    "&+LAs $n's $q strikes you, it opens up a fatal w&+roun&+Ld. The last thing you sense is the burning of your own &+rflesh&+L.&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    DAMMSG_TERSE},
  {"&+LNearing the apex of it's might, &+ww&+Wi&+ws&+Wp&+ws&+L of &+rse&+Rar&+Lin&+rg &+rf&+Rl&+ra&+Rm&+re&+Rs &+Lbegin to dance along the edges of $q &+Las it strikes $N.&n",
    "&+LYou feel the very &+rw&+Rar&+rmt&+Rh &+Lbeing sucked from your &+cs&+Co&+cu&+Cl&+L, being used to feed the growing &+rhe&+Rllf&+Lir&+re &+Las $n's $q &+Lslams into your body!",
    "&+LWithout warning, $n's $q &+Lbegins sucking all the &+rh&+Rea&+rt &+Lfrom the room as it sla&+Rs&+Lhes into $N!",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    "&+LAs $n's $q strikes you, it opens up a fatal w&+roun&+Ld. The last thing you sense is the burning of your own &+rflesh&+L.&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    DAMMSG_TERSE},
  {"&+LYour $q &+Lbursts into &+rhe&+Rll&+ris&+Lh &+rf&+Rl&+ra&+Rm&+re&+Rs &+Land causes $N to scream in great and &+wa&+Lg&+wo&+Ln&+wi&+Lz&+wi&+Ln&+wg &+Lp&+rai&+Ln as it burns away at $S &+rflesh&+L!",
    "&+rHe&+Rll&+Lis&+rh &+rf&+Rl&+ra&+Rm&+re&+Rs &+Lburst forth from $n's $q&+L, rending your flesh and causing you incredible p&+rai&+Ln!",
    "&+rF&+Rl&+ra&+Rm&+re&+Rs &+Lburst forth from $q&+L, dancing along $N's &+rflesh&+L as $E screams in sheer &+Lp&+rai&+Ln!",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    "&+LAs $n's $q strikes you, it opens up a fatal w&+roun&+Ld. The last thing you sense is the burning of your own &+rflesh&+L.&n",
    "&+LThe &+rs&+Rea&+rr&+Rin&+rg &+Lw&+round&+Ls are too much for $N to handle, and $E falls to the ground, thrashing wildly.&n",
    DAMMSG_TERSE}
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || !(wpn) || !FLAME_REAVER_WEAPONS(wpn) )
  {
    return FALSE;
  }

  for( int i = 0; i < 6; i++ )
    tiers_messages[i].obj = wpn;

  afp = get_spell_from_char(ch, SPELL_CEGILUNE_BLADE);
  if( !afp )
    return FALSE;

  // Chance for upgrading tier of damage.
  if( !number(0, 4) )
  {
    // Modifier holds tier of damage.
    switch( afp->modifier++ )
    {
      case 0:
        if( spell_damage(ch, victim, number(8,16), SPLDAM_FIRE, SPLDAM_NODEFLECT, &tiers_messages[afp->modifier])
          != DAM_NONEDEAD )
        {
          afp->modifier = 0;
          return TRUE;
        }
        break;
      case 1:
        if( spell_damage(ch, victim, number(16,32), SPLDAM_FIRE, SPLDAM_NODEFLECT, &tiers_messages[afp->modifier])
          != DAM_NONEDEAD )
        {
          afp->modifier = 0;
          return TRUE;
        }
        break;
      case 2:
        if( spell_damage(ch, victim, number(32,40), SPLDAM_FIRE, SPLDAM_NODEFLECT, &tiers_messages[afp->modifier])
          != DAM_NONEDEAD )
        {
          afp->modifier = 0;
          return TRUE;
        }
        break;
      case 3:
        if( spell_damage(ch, victim, number(40,56), SPLDAM_FIRE, SPLDAM_NODEFLECT, &tiers_messages[afp->modifier])
          != DAM_NONEDEAD )
        {
          afp->modifier = 0;
          return TRUE;
        }
        break;
      case 4:
        if( spell_damage(ch, victim, number(56,64), SPLDAM_FIRE, SPLDAM_NODEFLECT, &tiers_messages[afp->modifier])
          != DAM_NONEDEAD )
        {
          afp->modifier = 0;
          return TRUE;
        }
        break;
      default:
        if( spell_damage(ch, victim, number(64,80), SPLDAM_FIRE, SPLDAM_NODEFLECT, &tiers_messages[5])
          != DAM_NONEDEAD )
        {
          afp->modifier = 0;
          return TRUE;
        }
        afp->modifier = 0; // lets not go over max int
        break;
    }
  }

  // if we are on 3th tier or higher there is chance for immolate like attack
  if( afp->modifier >= 3 )
  {
    if( !number(0,5) )
    {
      act("&+LYour $q&+L suddenly ig&+rni&+Rte&+rs &+Linto &+rf&+Rl&+ra&+Rm&+re&+Rs &+Las it strikes $N!&n",
          FALSE, ch, wpn, victim, TO_CHAR);
      act("&+L$n's $q&+L ignites into a raging &+ri&+Rn&+rf&+Re&+rr&+Rn&+ro &+Las it bites deep into your &+rflesh&+L.&n",
          FALSE, ch, wpn, victim, TO_VICT);
      act("&+LRaging &+rhe&+Rll&+rfi&+Lre bursts forth from $n's $q&+L as it slices into $N!&n",
          FALSE, ch, wpn, victim, TO_NOTVICT);

      af2p = get_spell_from_char(victim, TAG_CEGILUNE_FIRE);
      if( !af2p )
      {
        memset(&af, 0, sizeof(af));
        af.type = TAG_CEGILUNE_FIRE;
        af.modifier = 5;
        af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
        af.duration = 10;
        af2p = affect_to_char(victim, &af);
        add_event(event_cegilune_searing, PULSE_VIOLENCE, ch, victim, wpn, 0, 0, 0);
      }
      else
      {
        af2p->modifier = 5;
      }
    }
  }
  return FALSE;
}

void event_cegilune_searing(P_char ch, P_char vict, P_obj wpn, void *data)
{
  struct damage_messages messages = {
    "",
    "&+LThe &+rf&+Ri&+rr&+Re&+rs &+Llick at your wound, causing you to scream in pain!&n",
    "",
    "&+L$N &+Lsuddenly begins thrashing around violently. Several chunks of smoldering &+rflesh&+L fall from $S body, and $E topples to the ground, screaming violently--then is suddenly silent!&n",
    "&+LThe &+rf&+Ri&+re&+Rr&+ry &+Lp&+rai&+Ln is too much for you to handle, and you think you hear the incessant cackling of &+md&+Lemoni&+mc &+Llaughter as your vision spins toward oblivion...&n",
    "&+L$N &+Lsuddenly begins thrashing around violently. Several chunks of smoldering &+rflesh&+L fall from$S body, and $E topples to the ground, screaming violently--then is suddenly silent!&n", 0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(vict) || !FLAME_REAVER_WEAPONS(wpn) )
  {
    return;
  }

  struct affected_type *afp;
  afp = get_spell_from_char(vict, TAG_CEGILUNE_FIRE);

  if (!afp)
  {
    return;
  }

  if( --(afp->modifier) <= 0 )
  {
    send_to_char("&+RThe flames engulfing your body subside.\n", vict);
    affect_remove(vict, afp);
    return;
  }

  // This correlates to 3 to 12 damage
  if( spell_damage(ch, vict, dice( 12, 4 ), SPLDAM_FIRE, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD )
  {
    add_event(event_cegilune_searing, PULSE_VIOLENCE, ch, vict, wpn, 0, 0, 0);
  }
}

void spell_girilals_granite_hammer(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  P_obj weapon = NULL;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( affected_by_spell(ch, SPELL_GIRILALS_GRANITE_HAMMER) )
  {
    send_to_char("&+LGirilal&+L refuses to grant you any more of his powers!&n\n", ch);
    return;
  }

  if( ch->equipment[WIELD] && EARTH_REAVER_WEAPONS(ch->equipment[WIELD]) )
  {
    weapon = ch->equipment[WIELD];
  }

  if( !weapon && ch->equipment[WIELD2] && EARTH_REAVER_WEAPONS(ch->equipment[WIELD2]) )
  {
    weapon = ch->equipment[WIELD2];
  }

  if( !weapon )
  {
    send_to_char("&+LThe power of &+yGirilal&+L requires you to have a suitable weapon!&n\n", ch);
    return;
  }

  struct affected_type af;
  bzero(&af, sizeof(af));

  af.type = SPELL_GIRILALS_GRANITE_HAMMER;
  af.duration = level / 2;
  af.location = APPLY_DAMROLL;
  // +1 at 30 and +4 at 56.
  af.modifier = level / 14;

  affect_to_char(victim, &af);

  act( "&+LA dull &+yglow&+L surrounds $p.", FALSE, ch, weapon, NULL, TO_CHAR);
}

bool girilals_granite_hammer(P_char ch, P_char victim, P_obj wpn)
{
  int dam;
  struct affected_type *afp, af;

  struct damage_messages messages = {
    "&+LThe &+ydust&+L surrounding $p &+Lscratches at $N&+L!&n",
    "&+LThe &+ydust&+L surrounding $n&+L's $q &+Lscratches at your flesh!&n",
    "&+LThe &+ydust&+L surrounding $n&+L's $q &+Lscratchs at $N&+L's flesh!&n",
    "&+LThe &+ydust&+L is too much for $N&+L; they fall to the ground in a &+rbloody&+L heap.&n",
    "&+LThe &+ydust&+L overwhelms you causing everything to go black.&n",
    "&+LThe &+ydust&+L is too much for $N&+L; they fall to the ground in a &+rbloody&+L heap.&n",
    DAMMSG_TERSE
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || !(wpn) || !EARTH_REAVER_WEAPONS(wpn) )
  {
    return FALSE;
  }

  messages.obj = wpn;

  // 2 to 5 damage at lvl 50.
  dam = number( 8, (GET_LEVEL(ch) * 2) / 5 );

  dam = MAX(1, dam);

  if( spell_damage(ch, victim, dam, SPLDAM_EARTH, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD )
    return TRUE;

  return FALSE;
}

void ileshs_broken( struct char_obj_link_data *cold )
{
  wear_off_message( cold->ch, cold->affect );
}

void spell_ileshs_smashing_fury(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  P_obj weapon;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  // If they don't have a prime or secondary reaver weapon.
  if( !(( (weapon = ch->equipment[WIELD]) && EARTH_REAVER_WEAPONS(weapon) )
    || ( (weapon = ch->equipment[WIELD2]) && EARTH_REAVER_WEAPONS(weapon) )
    || ( (weapon = ch->equipment[WIELD3]) && EARTH_REAVER_WEAPONS(weapon) )
    || ( (weapon = ch->equipment[WIELD4]) && EARTH_REAVER_WEAPONS(weapon) )) )
  {
    send_to_char("You need to wield the correct type of weapon!\n", ch);
    return;
  }

  if( affected_by_spell(ch, SPELL_ILESHS_SMASHING_FURY) )
  {
    send_to_char("&+yIlesh &+Lwill infuse your weapon no further.&n\n", ch);
    return;
  }

  if( !affected_by_spell(ch, SPELL_GIRILALS_GRANITE_HAMMER) )
  {
    send_to_char("&+LYour weapon glows for a moment, and then grows cool once more.&n\n", ch);
    return;
  }

  struct affected_type *afp = get_spell_from_char(ch, SPELL_GIRILALS_GRANITE_HAMMER);

  bzero(&af, sizeof(af));

  af.type = SPELL_ILESHS_SMASHING_FURY;
  af.duration = MIN(afp->duration,  level / 2);
  af.location = APPLY_NONE;
  af.modifier = 0;

  linked_affect_to_char_obj( ch, &af, weapon, LNK_ILESH );

  act("&+L$p&+L glows with a strange &+yhue&+L as &+yIlesh&+L infuses it.&n", FALSE, ch, weapon, 0, TO_CHAR);
}

// Called by reaver_hit_proc, this function handles the Ilesh's proc.
bool ilesh_fury(P_char ch, P_char victim, P_obj wpn)
{
  struct affected_type *afp;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || !(wpn) || !EARTH_REAVER_WEAPONS(wpn) )
  {
    return FALSE;
  }

  afp = get_spell_from_char(ch, SPELL_ILESHS_SMASHING_FURY);
  if( !afp )
    return FALSE;

  // Chance for upgrading tier of damage.
  if( !number(0, 4) )
  {
    // Modifier holds tier of damage.
    switch( afp->modifier++ )
    {
      case 0:
        spell_blindness( GET_LEVEL(ch), ch, NULL, 0, victim, NULL );
        break;
      case 1:
      case 2:
        spell_acid_blast( GET_LEVEL(ch), ch, NULL, 0, victim, NULL );
        break;
      case 3:
        spell_earthen_grasp( GET_LEVEL(ch), ch, NULL, 0, victim, NULL );
        break;
      case 4:
        if( !number(0, 3) )
          spell_acidimmolate( GET_LEVEL(ch), ch, NULL, 0, victim, NULL );
        else
          spell_acid_stream( GET_LEVEL(ch), ch, NULL, 0, victim, NULL );
        break;
      default:
        act( "&+LThe &+ydust&+L flies into your &+reyes&n.", FALSE, ch, 0, victim, TO_VICT);
        blind(ch, victim, number(5, 20) * WAIT_SEC);
        afp->modifier = 0; // lets not go over max int
        break;
    }
  }
  return FALSE;
}

/*
void event_cegilune_searing(P_char ch, P_char vict, P_obj wpn, void *data)
{
  struct damage_messages messages = {
    "",
    "&+LThe &+rf&+Ri&+rr&+Re&+rs &+Llick at your wound, causing you to scream in pain!&n",
    "",
    "&+L$N &+Lsuddenly begins thrashing around violently. Several chunks of smoldering &+rflesh&+L fall from $S body, and $E topples to the ground, screaming violently--then is suddenly silent!&n",
    "&+LThe &+rf&+Ri&+re&+Rr&+ry &+Lp&+rai&+Ln is too much for you to handle, and you think you hear the incessant cackling of &+md&+Lemoni&+mc &+Llaughter as your vision spins toward oblivion...&n",
    "&+L$N &+Lsuddenly begins thrashing around violently. Several chunks of smoldering &+rflesh&+L fall from$S body, and $E topples to the ground, screaming violently--then is suddenly silent!&n", 0
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(vict) || !FLAME_REAVER_WEAPONS(wpn) )
  {
    return;
  }

  struct affected_type *afp;
  afp = get_spell_from_char(vict, TAG_CEGILUNE_FIRE);

  if (!afp)
  {
    return;
  }

  if( --(afp->modifier) <= 0 )
  {
    send_to_char("&+RThe flames engulfing your body subside.\n", vict);
    affect_remove(vict, afp);
    return;
  }

  // This correlates to 3 to 12 damage
  if( spell_damage(ch, vict, dice( 12, 4 ), SPLDAM_FIRE, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD )
  {
    add_event(event_cegilune_searing, PULSE_VIOLENCE, ch, vict, wpn, 0, 0, 0);
  }
}

*/
void spell_thryms_icerazor(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  bool weapon = false;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(affected_by_spell(ch, SPELL_THRYMS_ICERAZOR))
  {
    send_to_char("&+CThrym&+L refuses to grant you any more of his powers!&n\n", ch);
    return;
  }

  if(ch->equipment[WIELD] &&
    FROST_REAVER_WEAPONS(ch->equipment[WIELD]))
  {
    weapon = true;
  }
    
  if(!weapon &&
     ch->equipment[WIELD2] &&
     FROST_REAVER_WEAPONS(ch->equipment[WIELD2]))
  {
    weapon = true;
  }
  
  if(!weapon)
  {
    send_to_char("&+LThe power of &+CThrym&+L requires you to have a suitable weapon!&n\n", ch);
    return;
  }

  struct affected_type af;
  bzero(&af, sizeof(af));

  af.type = SPELL_THRYMS_ICERAZOR;
  af.duration = level/2;
  af.location = APPLY_HITROLL;
  af.modifier = number(1, 4);

  affect_to_char(victim, &af);

  send_to_char("&+LThe &+CChill &+Lof &+BThrym&+L permeates your weapon.&n\n", ch);
}

bool thryms_icerazor(P_char ch, P_char victim, P_obj wpn)
{
  int      dam;
  char ch_msg[256], vict_msg[256], room_msg[256];
  struct affected_type *afp, af;

  struct damage_messages messages = {
    "&+CThe shards of &+Bice&+C surrounding $q &+Csplinter off as they strike $N&+C!&n",
    "&+CThe shards of &+Bice&+C surrounding $n's $q &+Ctear into your flesh!&n",
    "&+CThe chilling &+Bice &+Csurrounding $n's $q &+Cslices into $N&+C's flesh!&n",
    "&+CThe &+Bchilling &+Ccold is too much for $N&+C, and they fall to the ground, the stench of rending flesh filling the air.&n",
    "&+CThe &+Bchilling &+Cice is too much, and you fall to the ground, before everything goes &+Lblack&+C.&n",
    "&+CThe &+Bchilling &+Ccold is too much for $N&+C, and they fall to the ground, the stench of rending flesh filling the air.&n",
    DAMMSG_TERSE
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || !(wpn) || !FROST_REAVER_WEAPONS(wpn) )
  {
    return false;
  }

  messages.obj = wpn;

  dam = number(1, (GET_LEVEL(ch) / 4));

  if(IS_AFFECTED2(ch, AFF3_COLDSHIELD) )
    dam += number(1,8);

   if(IS_AFFECTED2(ch, AFF2_FIRESHIELD) )
    dam -= number(1,8);

  dam = MAX(1, dam);

  if(IS_AFFECTED3(ch, AFF3_COLDSHIELD) &&
     number(1, 100) <= 5)
  {
    // additional slowness messages
    act("&+CThe &+Bchilling&+C ice surrounding $n's &+Cweapon suddenly begin to vibrate as they strike $N!&n", FALSE, ch, 0, victim, TO_ROOM);
    act("&+CThe &+Bchilling&+C ice surrounding your weapon suddenly begins to vibrate as it strikes $N!&n", FALSE, ch, 0, victim, TO_CHAR);
    act("&+CThe &+Bchilling&+C ice surrounding $n's $q &+Csuddenly begins to vibrate as it strikes you!&n", FALSE, ch, 0, victim, TO_VICT);


  // apply slowness
    struct affected_type af;
    bzero(&af, sizeof(af));
    af.type = SPELL_SLOW;
    af.duration = 2;
    af.modifier = 2;
    af.bitvector2 = AFF2_SLOW;
    affect_to_char(victim, &af);
  }

  if(spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD )
    return TRUE;
    
  return FALSE;
}

void spell_lliendils_stormshock(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool weapon = false;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(affected_by_spell(ch, SPELL_LLIENDILS_STORMSHOCK) )
  {
    send_to_char("&+LLliendil &+Brefuses to grant you any more of his power!&n\n", ch);
    return;
  }

  if(ch->equipment[WIELD] &&
    SHOCK_REAVER_WEAPONS(ch->equipment[WIELD]))
  {
    weapon = true;
  }
    
  if(!weapon &&
     ch->equipment[WIELD2] &&
     SHOCK_REAVER_WEAPONS(ch->equipment[WIELD2]))
  {
    weapon = true;
  }
  
  if(!weapon)
  {
    send_to_char("You need to wield a suitable weapon!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_LLIENDILS_STORMSHOCK;
  af.duration = level / 2;
  af.location = APPLY_COMBAT_PULSE;
  af.modifier = number(-1, -2);

  affect_to_char(victim, &af);

  send_to_char("&+BSparks begin to &+barc&+B and &+Wjump&+B across your weapon&+B, and &+LLliendil&+B infuses your blade with the fury of &+wst&+Lor&+wms&+B!&n\n", ch);
}

bool lliendils_stormshock(P_char ch, P_char victim, P_obj wpn)
{
  int dam;
  struct affected_type *afp, af;

  struct damage_messages messages = {
    "&+BSearing &+Yb&+Colt&+Ys &+Bof &+Lblack &+Blig&+Whtn&+Bing &+Bflow forth from your $q, &+Bstriking $N!&n",
    "&+BSearing &+Yb&+Colt&+Ys &+Bof &+Lblack &+Blig&+Whtn&+Bing &+Bflow forth from $n's $q, &+Bstriking you soundly!&n",
    "&+BSearing &+Yb&+Colt&+Ys &+Bof &+Lblack &+Blig&+Whtn&+Bing &+Bflow forth from $n's $q, &+Bcausing $N to scream in pain!&n",
    "&+BAs the &+Lblack &+Yb&+Colt&+Ys &+Bwrack $N's body, they suddenly fall to the ground, a crackling heap of burnt flesh!&n",
    "&+BThe last thing your senses notice before you leave this mortal coil, is the scent of your own burning flesh.&n",
    "&+BSuddenly, a HUGE &+Lblack &+Yb&+Col&+Yt &+Bleaps forth from $n's $q&+B, and $N&+B falls to the ground, nothing but a twitching mass of burnt flesh!&n",
    DAMMSG_TERSE
  };

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || !(wpn) || !SHOCK_REAVER_WEAPONS(wpn) )
  {
    return false;
  }
  
  messages.obj = wpn;

  dam = number(1, GET_LEVEL(ch) / 4);

  if(IS_AFFECTED3(ch, AFF3_LIGHTNINGSHIELD) &&
    number(1, 100) <= 5 )
  {
    // apply some lightning effect
    act("&+BA HUGE bolt of lightning flares forth from $n's &+Bweapon! ZAAAAP!&n", FALSE, ch, 0, victim, TO_ROOM);
    act("&+BA HUGE bolt of lightning flares forth from your weapon! ZAAAAP!&N", FALSE, ch, 0, victim, TO_CHAR);
  }

  if(spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD )
  {
    return TRUE;
  }
  
  return FALSE;
}

void spell_stormcallers_fury(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool weapon = false;
  
  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(affected_by_spell(ch, SPELL_STORMCALLERS_FURY) )
  {
    send_to_char("You already are infuriated!&n\n", ch);
    return;
  }

  if(!affected_by_spell(ch, SPELL_LLIENDILS_STORMSHOCK) )
  {
    send_to_char("&+LYou must have the power of Lliendil to cast this spell!\n", ch);
    return;
  }

  if(ch->equipment[WIELD] &&
    SHOCK_REAVER_WEAPONS(ch->equipment[WIELD]))
  {
    weapon = true;
  }
    
  if(!weapon &&
     ch->equipment[WIELD2] &&
     SHOCK_REAVER_WEAPONS(ch->equipment[WIELD2]))
  {
    weapon = true;
  }
  
  if(!weapon)
  {
    send_to_char("You need to wield a suitable weapon!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_STORMCALLERS_FURY;
  af.duration = level/2;
  af.modifier = 0;

  affect_to_char(victim, &af);

  send_to_char("&+BThe power of the storm causes your weapon to crackle with energy.\n", ch);

}

bool stormcallers_fury(P_char ch, P_char victim, P_obj wpn)
{
  int pid;
  struct affected_type *afp, af;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || !(wpn) || !SHOCK_REAVER_WEAPONS(wpn) )
  {
    return false;
  }
  
  if(!affected_by_spell(ch, SPELL_LLIENDILS_STORMSHOCK))
  {
    return FALSE;
  }
  
  afp = get_spell_from_char(ch, SPELL_STORMCALLERS_FURY);
  
  if(!afp)
  {
    return FALSE;
  }
  
  /*
   the current level of chargeup is stored in the spell modifier
   the targets get TAG_STORMCALLERS_FURY_TARGET counters with the pid of the attacker,
   so if the reaver attacks a new target without the counter, it will reset the chargeup
   */

  pid = ( IS_PC(ch) ? GET_PID(ch) : GET_RNUM(ch) );

  int stormcaller_id = counter(victim, TAG_STORMCALLERS_FURY_TARGET);

  if( stormcaller_id != pid )
  {
    // starting on a new target, reset spell modifier
    afp->modifier = 0;
    remove_counter(victim, TAG_STORMCALLERS_FURY_TARGET, pid);
    add_counter(victim, TAG_STORMCALLERS_FURY_TARGET, pid, 0);
    return FALSE;
  }
  else
  {
    // buildup
    if( !number(0,1) )
    {
      afp->modifier++;

      if( afp->modifier == 6 )
      {
        act("&+BSparks begin to &+Yarc &+Band &+Ljump &+Bacross your weapon&+B.&n", FALSE, ch, wpn, victim, TO_CHAR);
        act("&+BSparks begin to &+Yarc &+Band &+Ljump &+Bacross $n's weapon&+B.&n", FALSE, ch, wpn, victim, TO_VICT);
        act("&+BSparks begin to &+Yarc &+Band &+Ljump &+Bacross $n's weapon&+B.&n", FALSE, ch, wpn, victim, TO_NOTVICT);
      }
      else if( afp->modifier == 12 )
      {
        act("&+BThe &+Yelectrical&+B aura around your weapon &+Bintensifies, and begins to &+Wcrackle &+Bloudly!&n", FALSE, ch, wpn, victim, TO_CHAR);
        act("&+BThe &+Yelectrical&+B aura around $n's &+Bweapon &+Bintensifies, and begins to &+Wcrackle &+Bloudly!&n", FALSE, ch, wpn, victim, TO_VICT);
        act("&+BThe &+Yelectrical&+B aura around $n's &+Bweapon &+Bintensifies, and begins to &+Wcrackle &+Bloudly!&n", FALSE, ch, wpn, victim, TO_NOTVICT);
      }
      else if( afp->modifier > number(14,16) )
      {
        afp->modifier = 0;

        act("&+BSuddenly, the &+Yenergy&+B surrounding your weapon &+Bleaps into your body, and empowers you to strike down $N&+B!&n", FALSE, ch, wpn, victim, TO_CHAR);
        act("&+BSuddenly, the &+Yenergy&+B surrounding $n's &+Bweapon &+Bleaps into their body, causing them to strike out in a flurry of movement!&n", FALSE, ch, wpn, victim, TO_VICT);
        act("&+BSuddenly, the &+Yenergy&+B surrounding $n's &+Bweapon &+Bleaps into their body, causing them to strike out in a flurry of movement!&n", FALSE, ch, wpn, victim, TO_NOTVICT);

        int num_hits = number(3,6);

        for( int i = 0; i < num_hits && IS_ALIVE(victim); i++ )
          hit(ch, victim, wpn);

        afp->modifier = 0;

        if( !IS_ALIVE(victim) )
          return TRUE;
      }

    }

    remove_counter(victim, TAG_STORMCALLERS_FURY_TARGET, pid);
    add_counter(victim, TAG_STORMCALLERS_FURY_TARGET, pid, 0);
    return FALSE;
  }

  return FALSE;
}

void spell_kostchtchies_implosion(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af;
  bool weapon = false;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return;
  }

  if(affected_by_spell(ch, SPELL_CHILLING_IMPLOSION) )
  {
    send_to_char("&+CKostchtchie &+Lrefuses to grant you any more power!&n\n", ch);
    return;
  }

  if(!affected_by_spell(ch, SPELL_THRYMS_ICERAZOR) )
  {
    send_to_char("&+CYou must have the power of &+LThrym&+C to cast this spell!\n", ch);
    return;
  }
  
  if(ch->equipment[WIELD] &&
    FROST_REAVER_WEAPONS(ch->equipment[WIELD]))
  {
    weapon = true;
  }
    
  if(!weapon &&
     ch->equipment[WIELD2] &&
     FROST_REAVER_WEAPONS(ch->equipment[WIELD2]))
  {
    weapon = true;
  }
  
  if(!weapon)
  {
    send_to_char("You need to wield a suitable weapon!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_CHILLING_IMPLOSION;
  af.duration = level/2;
  af.modifier = 0;

  affect_to_char(victim, &af);

  send_to_char("You become implosive with the power of the flair!\n", ch);
}

bool kostchtchies_implosion(P_char ch, P_char victim, P_obj wpn)
{
  int pid;
  struct affected_type *afp, af;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || !(wpn) || !FROST_REAVER_WEAPONS(wpn) )
  {
    return false;
  }

  if(!affected_by_spell(ch, SPELL_THRYMS_ICERAZOR) )
  {
    return FALSE;
  }
  
  afp = get_spell_from_char(ch, SPELL_CHILLING_IMPLOSION);
  
  if(!afp)
  {
    return FALSE;
  }
  
  /*
   the current level of chargeup is stored in the spell modifier
   the targets get TAG_CHILLING_IMPLOSION_TARGET counters with the pid of the attacker,
   so if the reaver attacks a new target without the counter, it will reset the chargeup
   */

  pid = ( IS_PC(ch) ? GET_PID(ch) : GET_RNUM(ch) );

  int imploder_id = counter(victim, TAG_CHILLING_IMPLOSION_TARGET);

  if( imploder_id != pid )
  {
    // starting on a new target, reset spell modifier
    afp->modifier = 0;
    remove_counter(victim, TAG_CHILLING_IMPLOSION_TARGET, pid);
    add_counter(victim, TAG_CHILLING_IMPLOSION_TARGET, pid, 0);
    return FALSE;
  }
  else
  {
    // buildup
    if( !number(0,1) )
    {
      afp->modifier++;

      if( afp->modifier == 6 )
      {
        act("&+CThe &+Bchilling &+Cice begins to gather upon your weapon&+C.&n", FALSE, ch, wpn, victim, TO_CHAR);
        act("&+CThe &+Bchilling &+Cice begins to gather upon $n's weapon&+C.&n", FALSE, ch, wpn, victim, TO_VICT);
        act("&+CThe &+Bchilling &+Cice begins to gather upon $n's weapon&+C.&n", FALSE, ch, wpn, victim, TO_NOTVICT);
      }
      else if( afp->modifier == 12 )
      {
        act("&+CThe &+Bice &+Cbegins to become heavy on your weapon&+C, and your determination grows!&n", FALSE, ch, wpn, victim, TO_CHAR);
        act("&+CThe &+Bice &+Cbegins to become heavy on $n's weapon&+C, and their determination fiercely intensifies!&n", FALSE, ch, wpn, victim, TO_VICT);
        act("&+CThe &+Bice &+Cbegins to become heavy on $n's weapon&+C, and their determination fiercely intensifies!&n", FALSE, ch, wpn, victim, TO_NOTVICT);
      }
      else if( afp->modifier > number(14,16) )
      {
        afp->modifier = 0;
        struct damage_messages messages = {
          "&+CSuddenly, a the &+Lshards &+Cof &+Bice &+Cfly forth from your weapon&+C, slicing into $N's &+Cflesh!&n",
          "&+CSuddenly, a the &+Lshards &+Cof &+Bice &+Cfly forth from $n's weapon&+C, slicing into your &+Cflesh!&n ",
          "&+CSuddenly, a the &+Lshards &+Cof &+Bice &+Cfly forth from $n's weapon&+C, slicing into $N's &+Cflesh!&n ",
          "&+CThe &+Lshards &+Cof &+Bice &+Ccut deep into $N&+C, leaving only a pile of rending flesh!&n",
          "&+CThe &+Lshards &+Cof &+Bice &+Care too much for you to handle, and you fall to the ground, the pain finally subsiding.",
          "&+CThe &+Lshards &+Cof &+Bice &+Ccut deep into $N&+C, leaving only a pile of rending flesh!&n",
          DAMMSG_TERSE
        };

        int dam = 450 + dice(10,10);

        if (spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages) != DAM_NONEDEAD)
          return TRUE;
      }

    }

    remove_counter(victim, TAG_CHILLING_IMPLOSION_TARGET, pid);
    add_counter(victim, TAG_CHILLING_IMPLOSION_TARGET, pid, 0);
    return FALSE;
  }

  return FALSE;
}

/*
 called in hit() if the ch is a reaver.
 put all the checks for the reaver spells here
 */

bool reaver_hit_proc(P_char ch, P_char victim, P_obj weapon)
{
  if(GET_RACE(victim) == RACE_CONSTRUCT)
     return FALSE;

  if (affected_by_spell(ch, SPELL_STORMCALLERS_FURY) && stormcallers_fury(ch, victim, weapon))
    return TRUE;

  if (affected_by_spell(ch, SPELL_CHILLING_IMPLOSION) && kostchtchies_implosion(ch, victim, weapon))
    return TRUE;

  if( is_linked_to( ch, weapon, LNK_CEGILUNE ) && cegilune_blade(ch, victim, weapon) )
    return TRUE;

  if( is_linked_to( ch, weapon, LNK_ILESH ) && ilesh_fury(ch, victim, weapon) )
    return TRUE;

  if (affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD) && ilienze_sword(ch, victim, weapon))
    return TRUE;

  if (affected_by_spell(ch, SPELL_THRYMS_ICERAZOR) && thryms_icerazor(ch, victim, weapon))
    return TRUE;

  if (affected_by_spell(ch, SPELL_LLIENDILS_STORMSHOCK) && lliendils_stormshock(ch, victim, weapon))
    return TRUE;

  if( affected_by_spell(ch, SPELL_GIRILALS_GRANITE_HAMMER) && girilals_granite_hammer(ch, victim, weapon) )
    return TRUE;

  return FALSE;
}

/*
 called from affect_total() in affects.c
 set all standard mods for reavers based on weapons, skills, etc
 */
void apply_reaver_mods(P_char ch)
{
  if( !ch )
    return;

  P_obj w1 = ch->equipment[WIELD];
  P_obj w2 = ch->equipment[WIELD2];

  if(GET_SPEC(ch, CLASS_REAVER, SPEC_ICE_REAVER))
  {
    if((w1 && required_weapon_skill(w1) == SKILL_1H_BLUDGEON) ||
       (w2 && required_weapon_skill(w2) == SKILL_1H_BLUDGEON))
    {
      ch->specials.base_combat_round += get_property("reavers.ice.penalty.pulse.1hBludgeon", 2.);
    }
    else if((w1 && required_weapon_skill(w1) == SKILL_2H_BLUDGEON) ||
            (w2 && required_weapon_skill(w2) == SKILL_2H_BLUDGEON))
    {
      ch->specials.base_combat_round += get_property("reavers.ice.penalty.pulse.2hBludgeon", 3.);
    }

  }
  else if(GET_SPEC(ch, CLASS_REAVER, SPEC_FLAME_REAVER))
  {
    if(required_weapon_skill(w1) == SKILL_2H_SLASHING ||
       required_weapon_skill(w2) == SKILL_2H_SLASHING ||
       required_weapon_skill(w1) == SKILL_2H_FLAYING ||
       required_weapon_skill(w2) == SKILL_2H_FLAYING )
    {
      ch->specials.base_combat_round += get_property("reavers.flame.penalty.pulse.2hSlashing", 3.);
    }
  }
/* Shock reavers currently have no two handed weapon skills. Apr09 -Lucrot
  else if( GET_SPEC(ch, CLASS_REAVER, SPEC_SHOCK_REAVER) )
  {
    if( required_weapon_skill(w1) == SKILL_2H_SLASHING ||
        required_weapon_skill(w2) == SKILL_2H_SLASHING )
    {
      ch->specials.base_combat_round += get_property("reavers.shock.penalty.pulse.2hSlashing", 3.);
    }
  } */
}

