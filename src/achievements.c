#include <string.h>

#include "db.h"
#include "prototypes.h"
#include "structs.h"
#include "achievements.h"
#include "epic.h"
#include "utils.h"
#include "spells.h"
#include "ships/ships.h"
#include "vnum.mob.h"

extern P_index mob_index;
extern int pulse;

int get_frags(P_char ch)
{
  int frags;
  frags = ch->only.pc->frags;
  return frags;
}

void do_achievements(P_char ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
  struct affected_type *paf = get_spell_from_char(ch, AIP_LEVELACHIEVEMENT);
  int lvlachi = paf ? paf->modifier : 0;
  paf = get_spell_from_char(ch, AIP_CARGOCOUNT);
  int cargo = paf ? paf->modifier : 0;

  sprintf(buf, "\r\n&+L=-=-=-=-=-=-=-=-=-=--= &+rDuris Mud &+yAch&+Yieveme&+ynts &+Lfor &+r%s &+L=-=-=-=-=-=-=-=-=-=-=-&n\r\n\r\n", GET_NAME(ch));

  /* PVP ACHIEVEMENTS */

  sprintf(buf2, "   &+W%-23s           &+W%s\r\n",
      " ", "&+L(&+rP&+Rv&+rP&+L)&n");
  strcat(buf, buf2);
  sprintf(buf2, "  &+W%-23s&+W%-42s&+W%s\r\n",
      "Achievement", "Requirement", "Affect/Reward");
  strcat(buf, buf2);
  sprintf(buf2, "  &+W%-23s&+W%-42s&+W%s\r\n",
      "-----------", "-------------", "-------------");
  strcat(buf, buf2);

  //-----Achievement: Soul Reaper
  if(get_frags(ch) >= 2000)
    sprintf(buf2, "  &+L%-50s&+L%-45s&+L%s\r\n",
        "&+B&+LS&+wo&+Wu&+Ll R&+we&+Wap&+we&+Lr", "&+BObtain 20 Frags", "&+BAccess to the soulbind ability");
  else
    sprintf(buf2, "  &+L%-50s&+L%-45s&+L%s\r\n",
        "&+B&+LS&+wo&+Wu&+Ll R&+we&+Wap&+we&+Lr", "&+wObtain 20 Frags", "&+wAccess to the soulbind ability");
  strcat(buf, buf2);
  //-----End Soul Reaper

  //-----Achievement: Serial Killer
  if(affected_by_spell(ch, ACH_SERIALKILLER))
    sprintf(buf2, "  &+L%-41s&+L%-45s&+L%s\r\n",
        "&+LSe&+wr&+Wi&+wa&+Ll &+rKiller", "&+BObtain 10.00 Frags", "&+BGain 2 points in every attribute");
  else
    sprintf(buf2, "  &+L%-41s&+L%-45s&+L%s\r\n",
        "&+LSe&+wr&+Wi&+wa&+Ll &+rKiller", "&+wObtain 10.00 Frags", "&+wGain 2 points in every attribute");
  strcat(buf, buf2);
  //-----End Serial Killer

  //-----Achievement: Let's Get Dirty
  if(affected_by_spell(ch, ACH_LETSGETDIRTY))
    sprintf(buf2, "  &+L%-44s&+L%-45s&+L%s\r\n",
        "&+LLet's Get &+rD&+Ri&+rr&+Rt&+ry&+R!", "&+BObtain 1.00 Frags", "&+BGain 2 CON points");
  else
    sprintf(buf2, "  &+L%-44s&+L%-45s&+L%s\r\n",
        "&+LLet's Get &+rD&+Ri&+rr&+Rt&+ry&+R!", "&+wObtain 1.00 Frags", "&+wGain 2 CON points");
  strcat(buf, buf2);
  //-----End Let's Get Dirty

  /*  PVE ACHIEVEMENTS */
  sprintf(buf2, "\r\n");
  strcat(buf, buf2);
  sprintf(buf3, "   &+W%-23s           &+W%s\r\n",
      " ", "&+L(&+gP&+Gv&+gE&+L)&n");
  strcat(buf, buf3);

  //-----Achievement: The Journey Begins
  if( lvlachi >= 5 )
    sprintf(buf3, "  &+L%-34s&+L%-45s&+L%s\r\n",
        "&+gThe Jou&+Grney Beg&+gins&n", "&+BGain level 5", "&+B&+ya rugged a&+Yd&+yv&+Ye&+yn&+Yt&+yu&+Yr&+ye&+Yr&+ys &+Lsatchel");
  else
    sprintf(buf3, "  &+L%-34s&+L%-45s&+L%s\r\n",
        "&+gThe Jou&+Grney Beg&+gins&n", "&+wGain level 5", "&+wan Unknown Item");
  strcat(buf, buf3);
  //-----The Journey Begins

  //-----Achievement: The Sailor's Tattoo
  if( lvlachi >= 30 )
    sprintf(buf3, "  &+L%-34s&+L%-45s&+L%s\r\n",
        "&+bThe Sai&+Blor's Tat&+btoo&n", "&+BGain level 30", "&+ya small &+bS&+Ba&+bi&+Bl&+bo&+Br&+b'&+Bs&n &+yTattoo&n");
  else
    sprintf(buf3, "  &+L%-34s&+L%-45s&+L%s\r\n",
        "&+bThe Sai&+Blor's Tat&+btoo&n", "&+wGain level 30", "&+wan Unknown Item");
  strcat(buf, buf3);
  //-----The Sailor's Tattoo

  //-----Achievement: Dragonslayer
  if(affected_by_spell(ch, ACH_DRAGONSLAYER))
    sprintf(buf3, "  &+L%-43s&+L%-45s&+L%s\r\n",
        "&+gDr&+Gag&+Lon &+gS&+Glaye&+gr&n", "&+BKill 1000 Dragons", "&+B10% damage increase vs Dragons");
  else
    sprintf(buf3, "  &+L%-43s&+L%-45s&+L%s &+W%d%%\r\n",
        "&+gDr&+Gag&+Lon &+gS&+Glaye&+gr&n", "&+wKill 1000 Dragons", "&+w10% damage increase vs Dragons", get_progress(ch, AIP_DRAGONSLAYER, 1000));
  strcat(buf, buf3);
  //-----Dragonslayer

  //-----Achievement: Demonslayer
  if(affected_by_spell(ch, ACH_DEMONSLAYER))
    sprintf(buf3, "  &+L%-43s&+L%-45s&+L%s\r\n",
        "&+rD&+Rem&+Lon &+rS&+Rlaye&+rr&n", "&+BKill 1000 Demons", "&+B10% damage increase vs Demons");
  else
    sprintf(buf3, "  &+L%-43s&+L%-45s&+L%s &+W%d%%\r\n",
        "&+rD&+Rem&+Lon &+rS&+Rlaye&+rr&n", "&+wKill 1000 Demons", "&+w10% damage increase vs Demons", get_progress(ch, AIP_DEMONSLAYER, 1000));
  strcat(buf, buf3);
  //-----Demonslayer

  //-----Achievement: Trader
  if( cargo >= 10000 )
    sprintf(buf3, "  &+L%-43s&+L%-45s  &+L%s\r\n",
        "&+yT&+Yr&+ya&+Yd&+ye&+Yr&n", "&+BSell 10000 crates of cargo&n", "&+YShip construction halved&n");
  else
    sprintf(buf3, "  &+L%-43s&+L%-45s  &+L%s &+W%d%%\r\n",
        "&+yT&+Yr&+ya&+Yd&+ye&+Yr&n", "&+ySell 10000 crates of cargo&n", "&+YShip construction halved&n", cargo/100);
  strcat(buf, buf3);
  //-----Trader

  //-----Achievement: You Strahd Me
  if(affected_by_spell(ch, ACH_YOUSTRAHDME))
    sprintf(buf3, "  &+L%-34s&+L%-50s&+L%s\r\n",
        "&+LYou &+rStrahd &+LMe At Hello&n", "&+BSee &+chelp strahd&n", "&+Bsee &+chelp you strahd me&n");
  else
    sprintf(buf3, "  &+L%-34s&+L%-50s&+L%s\r\n",
        "&+LYou &+rStrahd &+LMe At Hello&n", "&+wSee &+chelp strahd&n", "&+wan unknown reward&n");
  strcat(buf, buf3);
  //-----You Strahd Me


  //-----Achievement: May I Heals You
  if(affected_by_spell(ch, ACH_MAYIHEALSYOU))
    sprintf(buf3, "  &+L%-40s&+L%-45s&+L%s\r\n", 
        "&+WMay I &+WHe&+Ya&+Wls &+WYou?&n", "&+BHeal 1,000,000 points of player damage", "&+BAccess to the salvation command");
  else
    sprintf(buf3, "  &+L%-40s&+L%-45s&+L%s &+W%d%%&n\r\n", 
        "&+WMay I &+WHe&+Ya&+Wls &+WYou?&n", "&+wHeal 1,000,000 points of player damage", "&+wAccess to the salvation command", get_progress(ch, AIP_MAYIHEALSYOU, 1000000));
  strcat(buf, buf3);
  //-----May I Heals You

  //-----Achievement: Master of Deception
  if(affected_by_spell(ch, ACH_DECEPTICON))
    sprintf(buf3, "  &+L%-40s&+L%-45s&+L%s\r\n",
        "&+LMa&+rst&+Rer of De&+rcep&+Ltion&n", "&+BSuccessfully use 500 disguise kits", "&+BDisguise doesnt consume a kit&n");
  else
    sprintf(buf3, "  &+L%-40s&+L%-45s&+L%s  &+W%d%%\r\n",
        "&+LMa&+rst&+Rer of De&+rcep&+Ltion&n", "&+wSuccessfully use 500 disguise kits", "&+wDisguise doesnt consume a kit&n", get_progress(ch, AIP_DECEPTICON, 500));
  strcat(buf, buf3);
  //-----Master of Deception

  //-----Achievement: Full Base Stats 100 - gellz
  if(affected_by_spell(ch, ACH_DEATHSDOOR))
    sprintf(buf2, "  &+L%-55s&+L%-45s&+L%s\r\n",
        "&+MTo&+mug&+Mhe&+mn &+MU&+mp &+MPr&+min&+Mce&+mss&n", "&+BGet 100 in all stats(help toughen up)", "&+BAbility to use &+LDea&+wths&+L Do&+wor&n");
  else
    sprintf(buf2, "  &+L%-55s&+L%-45s&+L%s\r\n",
        "&+MTo&+mug&+Mhe&+mn &+MU&+mp &+MPr&+min&+Mce&+mss&n", "&+wGet 100 in all stats(help toughen up)", "&+wAbility to use &+LDea&+wths&+L Do&+wor&n");
  strcat(buf, buf2);
  //-----END Achievement: Full Base Stats 100

  if(GET_CLASS(ch, CLASS_NECROMANCER))
  {
    //-----Achievement: Descendent
    if(GET_RACE(ch) == RACE_LICH)
      sprintf(buf3, "  &+L%-50s&+L%-45s&+L%s\r\n",
          "&+LDe&+msc&+Len&+mde&+Lnt&n", "&+BSuccessfully &+cdescend&+L into darkness", "&+BBecome a Lich&n");
    else
      sprintf(buf3, "  &+L%-40s&+L%-51s&+L%s\r\n",
          "&+LDe&+msc&+Len&+mde&+Lnt&n", "&+wSuccessfully &+cdescend&+w into darkness", "&+wSee &+chelp descend&n");
    strcat(buf, buf3);
    //-----Descendent
  }


  sprintf(buf3, "\r\n&+L=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-&n\r\n\r\n", GET_NAME(ch));
  strcat(buf, buf3);
  /* sprintf(buf2, "   &+w%-15s          &+Y% 6.2f\t\r\n",
     name, pts);*/
  page_string(ch->desc, buf, 1);


}
//
void update_achievements(P_char ch, P_char victim, int cmd, int ach)
{
  char     argument[MAX_STRING_LENGTH];
  struct affected_type af;
  struct affected_type *paf, *afp;
  int required = 1;
  P_obj gift;

  /* Achievement int ach list:
     1 - may i heals you
     2 - kill type quest where victim is passed
     3 - disguise
     */

  if (IS_NPC(ch))
    return;

  if( !IS_ALIVE(ch) )
  {
    // some achievements depend on data objects that are freed up when a PC is killed, so if they are no longer
    // alive when update_achievements() is called, just skip it
    return;
  }

  //assign accumulation affects if missing and ach is of type.
  if ((ach == 1) && !affected_by_spell(ch, AIP_MAYIHEALSYOU) && !affected_by_spell(ch, ACH_MAYIHEALSYOU))
    apply_achievement(ch, AIP_MAYIHEALSYOU);

  if ((ach == 2) && !affected_by_spell(ch, AIP_DRAGONSLAYER) && !affected_by_spell(ch, ACH_DRAGONSLAYER))
    apply_achievement(ch, AIP_DRAGONSLAYER);

  if ((ach == 2) && !affected_by_spell(ch, AIP_DEMONSLAYER) && !affected_by_spell(ch, ACH_DEMONSLAYER))
    apply_achievement(ch, AIP_DEMONSLAYER);

  if ((ach == 3) && !affected_by_spell(ch, AIP_DECEPTICON) && !affected_by_spell(ch, ACH_DECEPTICON))
    apply_achievement(ch, AIP_DECEPTICON);

  if( ach == 2 && IS_NPC(victim) && (GET_VNUM(victim) == VMOB_STRAHD_ACH_DORU) )
  {
    if( !affected_by_spell(ch, AIP_YOUSTRAHDME) && !affected_by_spell(ch, AIP_YOUSTRAHDME2)
      && !affected_by_spell(ch, ACH_YOUSTRAHDME) )
    {
      apply_achievement(ch, AIP_YOUSTRAHDME);
    }
  }

  //PvP Achievements
  int frags;
  frags = get_frags(ch);

  /* LETS GET DIRTY */
  if((frags >= 100) && !affected_by_spell(ch, ACH_LETSGETDIRTY)) 
  {
    send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RLet's Get Dirty&+r achievement!&n\r\n", ch);
    ch->base_stats.Con += 2;
    if(ch->base_stats.Con > 100)
      ch->base_stats.Con = 100;

    apply_achievement(ch, ACH_LETSGETDIRTY);
  }

  /*  FULL STATS  - gellz  */ 
  if((!affected_by_spell(ch, ACH_DEATHSDOOR)) && 
    (ch->base_stats.Str == 100) &&
    (ch->base_stats.Agi == 100) &&
    (ch->base_stats.Dex == 100) &&
    (ch->base_stats.Con == 100) &&
    (ch->base_stats.Pow == 100) &&
    (ch->base_stats.Wis == 100) &&
    (ch->base_stats.Int == 100) &&
    (ch->base_stats.Cha == 100))
  {
    send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+MTo&+mug&+Mhe&+mn &+MU&+mp &+MPr&+min&+Mce&+mss&+r achievement!&n\r\n", ch);
    apply_achievement(ch, ACH_DEATHSDOOR);
  }

  /*  SERIAL KILLER */
  if((frags >= 1000) && !affected_by_spell(ch, ACH_SERIALKILLER)) 
  {
    send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RSerial Killer&+r achievement!&n\r\n", ch);
    ch->base_stats.Str = BOUNDED(1, (ch->base_stats.Str+ 2), 100);
    ch->base_stats.Agi = BOUNDED(1, (ch->base_stats.Agi+ 2), 100);
    ch->base_stats.Dex = BOUNDED(1, (ch->base_stats.Dex+ 2), 100);
    ch->base_stats.Con = BOUNDED(1, (ch->base_stats.Con+ 2), 100);
    ch->base_stats.Pow = BOUNDED(1, (ch->base_stats.Pow+ 2), 100);
    ch->base_stats.Wis = BOUNDED(1, (ch->base_stats.Wis+ 2), 100);
    ch->base_stats.Int = BOUNDED(1, (ch->base_stats.Int+ 2), 100);
    ch->base_stats.Cha = BOUNDED(1, (ch->base_stats.Cha+ 2), 100);
    apply_achievement(ch, ACH_SERIALKILLER);
  }

  /* Level-Based Achievements */
  if( !(paf = get_spell_from_char(ch, AIP_LEVELACHIEVEMENT)) || paf->modifier < GET_LEVEL(ch) )
  {
    // The Journey Begins
    if( GET_LEVEL(ch) >= 5 && (!paf || paf->modifier < 5) )
    {
      send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RThe Journey Begins&+r achievement!&n\r\n", ch);
      send_to_char("&+yThis &+Yachievement&+y rewards an &+Yitem&+y! Check your &+Winventory &+yby typing &+Wi&+y!&n\r\n", ch);
      gift = read_object(400222, VIRTUAL);
      if( IS_RACEWAR_EVIL(ch) && gift )
      {
        REMOVE_BIT(gift->extra_flags, ITEM_LIT);
      }
      obj_to_char( gift, ch);
      if( !paf )
      {
        paf = apply_achievement(ch, AIP_LEVELACHIEVEMENT);
      }
      paf->modifier = 5;
    }
    // The Sailor's Tattoo
    if( GET_LEVEL(ch) >= 30 && (!paf || paf->modifier < 30) )
    {
      send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RThe Sailor's Tattoo&+r achievement!&n\r\n", ch);
      send_to_char("&+yThis &+Yachievement&+y rewards a small &+bS&+Ba&+bi&+Bl&+bo&+Br&+b'&+Bs&n &+yTattoo&+y on your forearm.&n\r\n", ch);
      if( get_ship_from_owner( GET_NAME(ch) ) )
      {
        send_to_char( "&+ySeeing as you already own a ship, here's &n100 &+WPlatinum&+y!&n\n\r", ch);
        ADD_MONEY( ch, 100000 );
      }
      else
      {
        send_to_char( "&+yCheck the &+Wdocks &+yfor your reward: &+WA free sloop&+y!&n\n\r", ch);
        send_to_char( "&+y(Type &+g'list hull'&+y at any docks where you can buy a ship.)&n\n\r", ch);
        // Tag them for a free sloop.
        apply_achievement(ch, AIP_FREESLOOP);
      }
      if( !paf )
      {
        paf = apply_achievement(ch, AIP_LEVELACHIEVEMENT);
      }
      paf->modifier = 30;
    }
  }

  if(cmd == 0 && !victim) //no exp or other value means nothing else to do.
    return;

  //begin cumulative achievements
  struct affected_type *findaf, *next_af;  //initialize affects

  for(findaf = ch->affected; findaf; findaf = next_af)
  {
    next_af = findaf->next;

    /* May I Heals You */
    if((findaf && findaf->type == AIP_MAYIHEALSYOU) && !affected_by_spell(ch, ACH_MAYIHEALSYOU) && ach == 1)
    {
      //check to see if we've hit 1000000 healing
      int result = findaf->modifier;
      if(result >= 1000000)
      {
        affect_remove(ch, findaf);
        apply_achievement(ch, ACH_MAYIHEALSYOU);
        send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RMay I Heals You&+r achievement!&n\r\n", ch);
        send_to_char("&+yYou may now access the &+Wsalvation&+y command!&n\r\n", ch);
      }
      if((ch != victim) && !IS_NPC(victim))
      {
        findaf->modifier += cmd;
      }
    }
    /* end may i heals you */

    /* Decepticon */
    if((findaf && findaf->type == AIP_DECEPTICON) && !affected_by_spell(ch, ACH_DECEPTICON) && ach == 3)
    {
      //check to see if we've hit 500 disguise kits
      int result = findaf->modifier;
      if(result >= 500)
      {
        affect_remove(ch, findaf);
        apply_achievement(ch, ACH_DECEPTICON);
        send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RMaster of Deception&+r achievement!&n\r\n", ch);
        send_to_char("&+yYou may now use the &+Ydisguise&+y skill without it consuming a &+Ldisguise kit&+y but you must still have one in inventory!&n\r\n", ch);
      }
      findaf->modifier += cmd;

    }
    /* Decepticon */

    /* Dragonslayer */
    if((findaf && findaf->type == AIP_DRAGONSLAYER) && (ach == 2) && (GET_RACE(victim) == RACE_DRAGON) )
    {
      int result;

      // Skip illusionist dragons.
      if(GET_VNUM(victim) != 1108)
      {
        findaf->modifier++;
      }

      result = findaf->modifier;
      // If they already have the DragonSlayer achievement, kill the achievement in progress.
      if( affected_by_spell(ch, ACH_DRAGONSLAYER) )
      {
        affect_remove(ch, findaf);
      }
      // Check to see if we've hit 1000 kills
      else if(result >= 1000)
      {
        affect_remove(ch, findaf);
        apply_achievement(ch, ACH_DRAGONSLAYER);
        send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RDragonslayer&+r achievement!&n\r\n", ch);
        send_to_char("&+yYou will now do 10 percent more damage to dragon races!&n\r\n", ch);
      }
    }
    /* end Dragonslayer */

    /* Demonslayer */
    if((findaf && findaf->type == AIP_DEMONSLAYER) && (ach == 2) && (GET_RACE(victim) == RACE_DEMON) )
    {
      int result;

      // Skip demons in hometowns (ie zarbons).
      if( !IS_HOMETOWN(ch->in_room) )
      {
        findaf->modifier++;
      }

      result = findaf->modifier;
      // If they already have the DemonSlayer achievement, kill the achievement in progress.
      if( affected_by_spell(ch, ACH_DEMONSLAYER) )
      {
        affect_remove(ch, findaf);
      }
      // Check to see if we've hit 1000 kills
      else if(result >= 1000)
      {
        affect_remove(ch, findaf);
        apply_achievement(ch, ACH_DEMONSLAYER);
        send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RDemonslayer&+r achievement!&n\r\n", ch);
        send_to_char("&+yYou will now do 10 percent more damage to demon races!&n\r\n", ch);
      }
    }
    /* end Demonslayer */

    // You Strahd Me2
    if( (findaf && findaf->type == AIP_YOUSTRAHDME) && (ach == 2)
      && IS_NPC(victim) && (GET_VNUM(victim) == VMOB_STRAHD_ACH_CHERNOVOG) )
    {
      findaf->type = AIP_YOUSTRAHDME2;
    }

    // You Strahd Me - Finish
    if( (findaf && findaf->type == AIP_YOUSTRAHDME2) && (ach == 2)
      && IS_NPC(victim) && (GET_VNUM(victim) == VMOB_STRAHD_ACH_STRAHD) )
    {
      findaf->type = ACH_YOUSTRAHDME;
      send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RYou Strahd Me At Hello&+r achievement!&n\r\n", ch);
      send_to_char("&+yPlease see &+chelp you strahd me &+yfor reward details!&n\r\n", ch);
      gain_epic(ch, EPIC_STRAHDME, 0, 1000 );
    }
  }
}

affected_type *apply_achievement(P_char ch, int ach)
{
  struct affected_type af;

  if( !ach )
  {
    return NULL;
  }
  memset(&af, 0, sizeof(struct affected_type));
  af.type = ach;
  af.modifier = 0;
  af.duration = -1;
  af.location = 0;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM | AFFTYPE_NODISPEL;

  return affect_to_char(ch, &af);

}

// Addicted to Blood - Display
void do_addicted_blood(P_char ch, char *arg, int cmd)
{
  char   buf[MAX_STRING_LENGTH];
  int    time, bonus;
  struct affected_type *af;
  int    secs;

  if( !ch )
  {
    return;
  }

  // Calculate 'seconds' until next tick.  Note: somtimes there will be 60 sec left.
  secs = 60 *(PULSES_IN_TICK - pulse) / PULSES_IN_TICK;

/*
  sprintf( buf, "%s - %s\n%s\nYou are &+W%d%%&n complete.\n",
      "&+rAddicted to Blood&n", "&+wEXP and Plat Bonus&n", "&+wKill &+W30 &+wmobs within 30 minutes", get_progress(ch, TAG_ADDICTED_BLOOD, 30));
  send_to_char( buf, ch );
*/

  af = ch->affected;
  while( af && !(af->type == TAG_ADDICTED_BLOOD) )
    af = af->next;
  if( af )
  {
    time = af->duration * 60 + secs;
    bonus = (af->modifier > 11) ? ( (af->modifier - 10) / 2 ) : 0;
  }
  else
  {
    bonus = 0;
    time = 0;
  }
  sprintf( buf, "You are currently receiving %d%% bonus experience on kills.\n", bonus );
  send_to_char( buf, ch );
  sprintf( buf, "You have %d:%s%d left before this bonus runs out.\n\n", time / 60, (time % 60) < 10 ? "0" : "", time % 60 );
  send_to_char( buf, ch );

  af = ch->affected;
  while( af && !(af->type == TAG_BLOODLUST) )
    af = af->next;
  sprintf( buf, "&+rBloodlust&n - Damage Bonus(&+yvs mob only&n)\nCurrent lust: &+W%d%%&n\n", af ? af->modifier * 10 : 0 );
  send_to_char( buf, ch );
  time = af ? af->duration*60 + secs : 0;
  sprintf( buf, "You have %d:%s%d left.\n", time / 60, (time % 60) < 10 ? "0" : "", time % 60 );
  send_to_char( buf, ch );
}

void update_addicted_to_blood(P_char ch, P_char victim)
{
  int allies, bonus;
  P_char tch;

  if( IS_NPC(victim) && GET_LEVEL(victim) >= GET_LEVEL(ch) - 5
    && !IS_PC_PET(victim) && !affected_by_spell(victim, TAG_CONJURED_PET) )
  {
    // Add addicted to blood if it isn't already there
    if( !affected_by_spell(ch, TAG_ADDICTED_BLOOD) )
    {
      struct affected_type aaf;
      memset(&aaf, 0, sizeof(struct affected_type));
      aaf.type = TAG_ADDICTED_BLOOD;
      aaf.modifier = 0;
      aaf.duration = 5;
      aaf.location = 0;
      aaf.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM | AFFTYPE_NODISPEL;
      affect_to_char(ch, &aaf);
    }
    // Now increment the af and check if 30 kills.
    struct affected_type *af = ch->affected;
    while( af && !(af->type == TAG_ADDICTED_BLOOD) )
      af = af->next;
    // This should always be true, but just in case...
    if( af )
    {
      af->duration = 5;
      if( (bonus = (af->modifier - 10) / 2) > 0 )
      {

        allies = 1;
        for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
        {
          if( tch != ch && IS_PC(tch) && !opposite_racewar(ch, tch) && !IS_TRUSTED(tch) )
          {
            allies++;
          }
        }
        gain_exp( ch, victim, GET_EXP(victim) * bonus / (allies * 100), EXP_KILL );
      }
      // Cap at 50 kills and 20% bonus exp.
      if( af->modifier < 50 )
      {
        af->modifier += 1;
      }

/* Old addicted to blood chunked on 31st kill.
      // Check to see if we've hit 30 kills
      if( af->modifier >= 30)
      {
        affect_remove( ch, af );

        allies = 1;
        for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
        {
          if( tch != ch && IS_PC(tch) && !opposite_racewar(ch, tch) && !IS_TRUSTED(tch) )
          {
            allies++;
          }
        }

        send_to_char("&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed &+rAddicted to Blood&+r!&n\r\n", ch);
        send_to_char("&+yEnjoy an &+Yexp bonus&+y and &+W5 platinum coins&+y!&n\r\n", ch);
        gain_exp(ch, victim, GET_EXP(victim) * 5 / allies, EXP_KILL);
        ADD_MONEY(ch, 5000);
      }
      // Otherwise, add a kill.
      else
        af->modifier += 1;
*/
    }
  }
}

