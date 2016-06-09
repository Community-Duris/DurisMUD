/*
   If a person is already grouped, they cannot be grouped in another group.

   If a person is the group leader, and they leave the group, the
   second person  in the group becomes the new group leader.

   Only the group leader may add members to a group.

   'group' shows a list of all members in a group.

   'group me', 'group self', or 'group <my_name>' will remove me from
   any group I'm in.

   'group <name>' is only usable by the group leader, and adds <name>
   to the group.

   'group all' is only usable by the group leader, and attempts to
   group everyone following the leader.

   'disband' is only usable by the group leader, and removes everyone
   from the group.

   If a group only has 1 member, it is automatically disbanded.


   Other then as a convienence with the "group all" command,
   following, and follow order has NOTHING to do with grouping.
 */

#include <stdio.h>
#include <string.h>

#include "spells.h"
#include "comm.h"
#include "interp.h"
#include "mm.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "assocs.h"
#include "events.h"
#include "ships/ships.h"


extern P_desc descriptor_list;
extern const struct race_names race_names_table[];
extern P_room world;
extern void purge_linked_auras(P_char ch);


struct mm_ds *dead_group_pool = NULL;


/*
 * Calculates free slots in back rank, might be negative
 */
int free_back_slots(P_char ch)
{
  struct group_list *gl;
  int      front = 0, back = 0;

  for (gl = ch->group; gl; gl = gl->next)
  {
    if (IS_PC(gl->ch) && gl->ch->in_room == ch->in_room)
    {
      if (IS_BACKRANKED(gl->ch))
        back++;
      else
        front++;
    }
  }

  return MIN(front - back - 1, 3 - back);
}

/*
 * Push all that dont fit into back rank to the front
 */
void fix_group_ranks(P_char ch)
{
  struct group_list *gl;
  int      how_many;

  if (!ch || !(ch->group))
    return;

  how_many = -1 * free_back_slots(ch);
  for (gl = ch->group; gl && how_many > 0; gl = gl->next)
  {
    if (IS_BACKRANKED(gl->ch) && gl->ch->in_room == ch->in_room)
    {
      REMOVE_BIT(gl->ch->specials.act2, PLR2_BACK_RANK);
      act("You notice that you've moved up towards the battle lines!", TRUE,
          gl->ch, 0, NULL, TO_CHAR);
      act("$n notices that $e has moved up towards the battle lines!", TRUE,
          gl->ch, 0, NULL, TO_NOTVICT);
      how_many--;
    }
  }
}

/*
 * Test if victim is on front line, if flag indicates it is,
 * make sure recalculating group ranks.
 */
int on_front_line(P_char ch)
{
  P_char   tch;

  if (!ch || !(ch->group) || IS_NPC(ch) || !IS_BACKRANKED(ch))
    return TRUE;

  if (free_back_slots(ch) < 0)
  {
    fix_group_ranks(ch);
    if (!IS_BACKRANKED(ch))
      return TRUE;
  }
  if( IS_DESTROYING(ch) )
  {
    REMOVE_BIT(ch->specials.act2, PLR2_BACK_RANK);
    return TRUE;
  }

  if( IS_FIGHTING(ch))
  {
    if (!IS_FIGHTING(ch->specials.fighting))
    {
      tch = ch->specials.fighting;
      if (IS_NPC(tch) && !IS_PC_PET(tch))
      {
        REMOVE_BIT(ch->specials.act2, PLR2_BACK_RANK);
        return TRUE;
      }
    }
  }
  return FALSE;
}

void displayM(P_char ch, char *tbuf)
{
  int      percent = 0;
  char     color[10];

//  GET_VITALITY(ch), GET_MAX_VITALITY(ch), WTF is this for?
  // Stops divide by zero crashes (see spell enervation).
  if( GET_MAX_VITALITY(ch) != 0 )
  {
    percent = 100 * GET_VITALITY(ch) / GET_MAX_VITALITY(ch);
  }
  else
  {
    percent = 0;
  }

//wizlog(56, "percent:%d", percent);

  if (percent == 100)
    sprintf(color, "&+G");
  else if (percent > 95)
    sprintf(color, "&+g");
  else if (percent > 50)
    sprintf(color, "&+Y");
  else if (percent > 0)
    sprintf(color, "&+R");
  else
    sprintf(color, "&+L");

  sprintf(tbuf, "%s <%s%dm&n/%s%dM&n>", tbuf, color, GET_VITALITY(ch), color,
          GET_MAX_VITALITY(ch));
}



void displayH(P_char ch, char *tbuf)
{
  int      percent = 0;
  char     color[10];

  GET_HIT(ch), GET_MAX_HIT(ch), percent = 100 * GET_HIT(ch) / GET_MAX_HIT(ch);

//wizlog(56, "percent:%d", percent);

  if (percent == 100)
    sprintf(color, "&+G");
  else if (percent > 95)
    sprintf(color, "&+g");
  else if (percent > 50)
    sprintf(color, "&+Y");
  else if (percent > 0)
    sprintf(color, "&+R");
  else
    sprintf(color, "&+L");

  sprintf(tbuf, "%s <%s%dh&n/%s%dH&n>", tbuf, color, GET_HIT(ch), color,
          GET_MAX_HIT(ch));
}

float group_exp_modifier(P_char ch)
{

  float group_fact = 1.00;

  if (ch->group)
        {
           struct group_list *gl;

           for (gl = ch->group; gl; gl = gl->next)
             if ((ch->in_room == gl->ch->in_room) && (gl->ch != ch))
               if(IS_PC(gl->ch))
               group_fact = group_fact + 0.02;
        }
 return group_fact;
}

P_char get_char_on_ship_bridge(P_char ch, const char *name)
{
    P_desc   d;
    P_char vict = NULL;
    P_ship ship, ship_ch = NULL, ship_vict = NULL;

    for (d = descriptor_list; d; d = d->next)
    {
        if (!d->character || d->connected || !d->character->player.name)
          continue;
        if (!isname(d->character->player.name, name))
          continue;
        if (!CAN_SEE_Z_CORD(ch, d->character))
          continue;
        vict = d->character;
        break;
    }

    if (vict && !IS_TRUSTED(ch) && !IS_TRUSTED(vict))
        if (racewar(ch, vict) && !IS_DISGUISE(vict) ||
            (IS_DISGUISE(vict) && (EVIL_RACE(ch) != EVIL_RACE(vict))) ||
            (GET_RACE(ch) == RACE_ILLITHID && !IS_ILLITHID(vict)))
    {
        vict = NULL;
    }

    if (!vict)
        return NULL;


    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (IS_SET(svs->flags, LOADED))
        {
            if (world[ch->in_room].number == svs->bridge)
            {
                ship_ch = svs;
            }
            if (world[vict->in_room].number == svs->bridge)
            {
                ship_vict = svs;
            }
        }
    }

    if (!ship_ch || !ship_vict)
        return NULL;

    if (IS_SET(ship_ch->flags, DOCKED) || IS_SET(ship_vict->flags, DOCKED))
        return NULL;


    return vict;
}

void do_group(P_char ch, char *argument, int cmd)
{
  char     name[MAX_INPUT_LENGTH], rank[MAX_INPUT_LENGTH];
  struct follow_type *f;
  bool     found;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH],
    Gbuf3[MAX_STRING_LENGTH];
  struct group_list *gl;
  P_char   victim;
  int maxsize, counter;

/*  one_argument(argument, name); */
  argument_interpreter(argument, name, rank);

  /* display group list */

  if(ch && GET_RACE(ch) == RACE_ILLITHID)
  {
    send_to_char("You feel like being alone right now.\n", ch);
    return;
  }

  if (!*name)
  {
    if (!ch->group)
    {
      if (ch && ch->desc && ch->desc->term_type == TERM_MSP)
      {
        send_to_char("<group>\n</group>\n", ch);
      }
      else
        send_to_char("But you are a member of no group?!\n", ch);

      return;

    }
    else
    {
      if (GET_RACE(ch) == RACE_ILLITHID)
      {
        send_to_char("You feel like being alone right now.\n", ch);
        return;
      }

      if (ch && ch->desc && ch->desc->term_type == TERM_MSP)
      {
        send_to_char("<group>\n", ch, LOG_NONE);

        gl = ch->group;
        if (IS_NPC(gl->ch))
          sprintf(Gbuf3, "%s", gl->ch->player.short_descr);
        else
        {
          if (racewar(ch, gl->ch))
            strcpy(Gbuf3, race_names_table[GET_RACE(gl->ch)].ansi);
          else
            sprintf(Gbuf3, "%s", GET_NAME(gl->ch));
        }
        sprintf(Gbuf1, "( Head) %-s\n",
                (!CAN_SEE_Z_CORD(ch, gl->ch)) ? "Someone" : Gbuf3);

        if (ch->in_room == gl->ch->in_room)
        {
          displayH(gl->ch, Gbuf1);
          displayM(gl->ch, Gbuf1);
        }
        else
        {
          sprintf(Gbuf1, "%s\n", Gbuf1);
        }

        for (gl = gl->next; gl; gl = gl->next)
        {
          if (IS_NPC(gl->ch))
            sprintf(Gbuf3, "%s", gl->ch->player.short_descr);
          else
          {
            if (racewar(ch, gl->ch))
              strcpy(Gbuf3, race_names_table[GET_RACE(gl->ch)].ansi);
            else
              sprintf(Gbuf3, "%s", GET_NAME(gl->ch));
          }

          sprintf(Gbuf2, "\n(%-5s)  %-s\n",
                  (!IS_BACKRANKED(gl->ch) ? "Front" : "Back"),
                  (!CAN_SEE_Z_CORD(ch, gl->ch)) ? "Someone" : Gbuf3);

          if (ch->in_room == gl->ch->in_room)
          {
            displayH(gl->ch, Gbuf2);
            displayM(gl->ch, Gbuf2);
          }
          else
          {
            sprintf(Gbuf2, "%s\n", Gbuf2);
          }
          sprintf(Gbuf1, "%s%s", Gbuf1, Gbuf2);

        }
        send_to_char(Gbuf1, ch, LOG_NONE);
        send_to_char("\n</group>\n", ch, LOG_NONE);
        return;

      }
      for (gl = ch->group, counter = 0; gl; gl = gl->next)
      {
        counter++;
      }
      if (IS_RACEWAR_EVIL(ch))
        maxsize = (int) get_property("groups.size.max.evil", 13);
      if (IS_RACEWAR_GOOD(ch))
        maxsize = (int) get_property("groups.size.max.good", 13);
#if defined(CTF_MUD) && (CTF_MUD == 1)
      maxsize = 5;
#endif

      sprintf(Gbuf1, "Your group consists of (%2d/%d):\n", counter, maxsize);
      send_to_char(Gbuf1, ch);


      gl = ch->group;
/*      if (GET_CLASS(gl->ch, CLASS_PSIONICIST) ||
          GET_CLASS(gl->ch, CLASS_MINDFLAYER))
        sprintf(Gbuf2, "%5d/%-5d hit, %4d/%-4d move, %4d/%-4d mana",
                GET_HIT(gl->ch), GET_MAX_HIT(gl->ch), GET_VITALITY(gl->ch),
                GET_MAX_VITALITY(gl->ch), GET_MANA(gl->ch),
                GET_MAX_MANA(gl->ch));
      else*/
        sprintf(Gbuf2, "%5d/%-5d hit, %4d/%-4d move",
                GET_HIT(gl->ch), GET_MAX_HIT(gl->ch),
                GET_VITALITY(gl->ch), GET_MAX_VITALITY(gl->ch));
      if (IS_NPC(gl->ch))
        strcpy(Gbuf3, gl->ch->player.short_descr);
      else
      {
        if (racewar(ch, gl->ch))
          strcpy(Gbuf3, race_names_table[GET_RACE(gl->ch)].ansi);
        else
          strcpy(Gbuf3, GET_NAME(gl->ch));
      }
      sprintf(Gbuf1, "( Head) %-30s %-s\n",
              (ch->in_room == gl->ch->in_room) ? Gbuf2 : "",
              (!CAN_SEE_Z_CORD(ch, gl->ch)) ? "Someone" : Gbuf3);
/*       IS_NPC(gl->ch) ? gl->ch->player.short_descr : GET_NAME(gl->ch)*/

      send_to_char(Gbuf1, ch);

      for (gl = gl->next; gl; gl = gl->next)
      {
        /*if (GET_CLASS(gl->ch, CLASS_PSIONICIST) ||
            GET_CLASS(gl->ch, CLASS_MINDFLAYER))
          sprintf(Gbuf2, "%5d/%-5d hit, %4d/%-4d move, %4d/%-4d mana",
                  GET_HIT(gl->ch), GET_MAX_HIT(gl->ch), GET_VITALITY(gl->ch),
                  GET_MAX_VITALITY(gl->ch), GET_MANA(gl->ch),
                  GET_MAX_MANA(gl->ch));
        else*/
          sprintf(Gbuf2, "%5d/%-5d hit, %4d/%-4d move",
                  GET_HIT(gl->ch), GET_MAX_HIT(gl->ch),
                  GET_VITALITY(gl->ch), GET_MAX_VITALITY(gl->ch));

        if (IS_NPC(gl->ch))
          strcpy(Gbuf3, gl->ch->player.short_descr);
        else
        {
          if (racewar(ch, gl->ch))
            strcpy(Gbuf3, race_names_table[GET_RACE(gl->ch)].ansi);
          else
            strcpy(Gbuf3, GET_NAME(gl->ch));
        }

        sprintf(Gbuf1, "(%-5s) %-30s %-s\n",
                (!IS_BACKRANKED(gl->ch) ? "Front" : "Back"),
                ((ch->in_room == gl->ch->in_room) && (!racewar(ch, gl->ch) || IS_DISGUISE(gl->ch)) )
                ? Gbuf2 : "",
                (!CAN_SEE_Z_CORD(ch, gl->ch)) ? "Someone" : Gbuf3);
/*              IS_NPC(gl->ch) ? gl->ch->player.short_descr :
                GET_NAME(gl->ch));*/
        send_to_char(Gbuf1, ch);
      }
    }
    return;
  }
  if (ch->group)
  {

    if (!str_cmp(name, "back"))
    {
        return;
      // code here to send self to back rank
      if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
        return;
      SET_BIT(ch->specials.act2, PLR2_BACK_RANK);
      send_to_char("You move back to the back rank!\n", ch);
      on_front_line(ch);
      //Client
      for (gl = ch->group; gl; gl = gl->next)
      {
        if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
        {
          gl->ch->desc->last_group_update = 1;
        }
      }
      return;
    }
    else if (!str_cmp(name, "front"))
    {
        return;
      // code here to send self to front rank
      if( IS_FIGHTING(ch) )
        return;
      REMOVE_BIT(ch->specials.act2, PLR2_BACK_RANK);
      send_to_char("You move up to the front rank!\n", ch);
      //Client
      for (gl = ch->group; gl; gl = gl->next)
      {
        if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
        {
          gl->ch->desc->last_group_update = 1;
        }
      }
      on_front_line(ch);
      return;
    }
    else if (!str_cmp(name, "display"))
    {
      // Code here to display ranks graphically
    }
  }

  /* group all */
  if (!str_cmp(name, "all"))
  {
    found = FALSE;

    if (ch->group && (ch->group->ch != ch))
    {
      send_to_char("This only works for group leaders.\n", ch);
      return;
    }
    for (f = ch->followers, found = FALSE; f; f = f->next)
      if (CAN_SEE(ch, f->follower) && f->follower->group)
        if (ch->group == f->follower->group)
        {
          act("$N already grouped by you.", TRUE, ch, 0, f->follower,
              TO_CHAR);
        }
        else
        {
          act("$N is in another group.", TRUE, ch, 0, f->follower, TO_CHAR);
        }
      else if(ch &&
             (IS_ILLITHID(f->follower) ||
             IS_ILLITHID(ch)))
        {
          return;
        }
      else if((f->follower->in_room == ch->in_room) &&
             CAN_SEE(ch, f->follower) &&
             ((IS_ILLITHID(f->follower) == IS_ILLITHID(ch)) ||
             (IS_ILLITHID(ch) && IS_NPC(f->follower))))
      {
        if (group_add_member(ch, f->follower))
        {
          act("$N is now a member of your group.",
              TRUE, ch, 0, f->follower, TO_CHAR);
          act("$N is now a member of $n's group.",
              TRUE, ch, 0, f->follower, TO_NOTVICT);
          act("You are now a member of $n's group.",
              FALSE, ch, 0, f->follower, TO_VICT);

          found = TRUE;
        }
      }
    if (!found)
      send_to_char("No new group members.\n", ch);

    return;
  }
  /* group <name>  (can be group me) */
  if (!(victim = get_char_room_vis(ch, name)))
  {
    if (!(victim = get_char_on_ship_bridge(ch, name))   )
    {
      send_to_char("No one here by that name.\n", ch);
      return;
    }
  }
  if (victim == ch)
  {
    if (!ch->group)
    {
      send_to_char("You can't leave a group when your not already in one!\n",
                   ch);
    }
    else
    {
      send_to_char("You leave the group.\n", ch);
      for (gl = ch->group; gl; gl = gl->next)
      {
        if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
        {
          gl->ch->desc->last_group_update = 1;
        }
      }
      group_remove_member(ch);
      purge_linked_auras(ch);
      REMOVE_BIT(ch->specials.affected_by3, AFF3_PALADIN_AURA);
      clear_links(ch, LNK_PALADIN_AURA);
    }
    //Client
    for (gl = ch->group; gl; gl = gl->next)
    {
      if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
      {
        gl->ch->desc->last_group_update = 1;
      }
    }
    return;
  }
  /* only the group leader can do anything below this point */
  if (ch->group && (ch->group->ch != ch))
  {
    send_to_char
      ("You can not enroll group members without being head of a group.\n",
       ch);
    return;
  }
  /* okay.. if victim is already in the group, kick 'em out */

  if (ch->group && (victim->group == ch->group))
  {
    if (rank && (isname(rank, "back") || isname(rank, "front")))
    {
        return;
      if (isname(rank, "front"))
      {
        REMOVE_BIT(victim->specials.act2, PLR2_BACK_RANK);
        act("You are sent to the front ranks!", TRUE, ch, 0, victim, TO_VICT);
        act("You send $N to the front ranks!", TRUE, ch, 0, victim, TO_CHAR);
      }
      else if (isname(rank, "back") && free_back_slots(victim) > 0)
      {

        SET_BIT(victim->specials.act2, PLR2_BACK_RANK);
        act("You are sent to the back ranks.", TRUE, ch, 0, victim, TO_VICT);
        act("You send $N to the back ranks.", TRUE, ch, 0, victim, TO_CHAR);
      }
      else
        send_to_char("You need more members in your front rank first!\n", ch);
      return;
    }
    else
    {
      act("You have been kicked out of $n's group.",
          FALSE, ch, 0, victim, TO_VICT);
      act("$N has been kicked out of $n's group.",
          TRUE, ch, 0, victim, TO_NOTVICT);
      group_remove_member(victim);
      //Client
      for (gl = ch->group; gl; gl = gl->next)
      {
        if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
        {
          gl->ch->desc->last_group_update = 1;
        }
      }
      //Client
      for (gl = victim->group; gl; gl = gl->next)
      {
        if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
        {
          gl->ch->desc->last_group_update = 1;
        }
      }
      return;
    }
  }
  /* well.. nothing left to do.. if the victim isn't in another group,
     then group them :) */

  if (victim->group)
  {
    act("$N is in another group.", TRUE, ch, 0, victim, TO_CHAR);
    return;
  }
  /* NPC's only allow themselves to be grouped with people they are
     following */
  /* if(IS_ILLITHID(ch) || // Illithids cannot group with anything. Nov08 -Lucrot
     IS_ILLITHID(victim))
  {
    act("$N doesn't want to be in your group.", TRUE, ch, 0, victim, TO_CHAR);
    return;
  } */
/*
//Paladins and AP cannot group together - Drannak
if(GET_CLASS(victim, CLASS_PALADIN))
{
for (gl = ch->group; gl; gl = gl->next)
 {
  if (GET_CLASS(gl->ch, CLASS_ANTIPALADIN))
   {
    send_to_char("&+WYou do not wish to group with such unholy scum!&n\n", victim);
    send_to_char("&+LAhh, but grouping with someone so &+Wholy &+Lwould disturb the unholy energies of your group.&n\n", ch);
	return;
   }
  
  }
 }

if(GET_CLASS(victim, CLASS_ANTIPALADIN))
{
for (gl = ch->group; gl; gl = gl->next)
 {
  if (GET_CLASS(gl->ch, CLASS_PALADIN))
   {
    send_to_char("&+WYou do not wish to group with such holy scum!&n\n", victim);
    send_to_char("&+WAhh, but grouping with someone so &+Lunholy &+Wwould disturb the holy energies of your group.&n\n", ch);
	return;
   }
  
  }
 }
if(GET_CLASS(victim, CLASS_ANTIPALADIN) && GET_CLASS(ch, CLASS_PALADIN))
{
    send_to_char("&+WYou do not wish to group with such unholy scum!&n\n", ch);
    send_to_char("&+LAhh, but grouping with someone so &+Wholy &+Lwould disturb the unholy energies of your group.&n\n", victim);
	return;
   }
if(GET_CLASS(victim, CLASS_PALADIN) && GET_CLASS(ch, CLASS_ANTIPALADIN))
  {
    send_to_char("&+WYou do not wish to group with such holy scum!&n\n", ch);
    send_to_char("&+WAhh, but grouping with someone so &+Lunholy &+Wwould disturb the holy energies of your group.&n\n", victim);
	return;
   }
*/

  if (IS_NPC(victim) && (victim->following != ch) &&
      !is_linked_to(ch, victim, LNK_CONSENT))
  {
    if (!is_guild_golem(victim, ch))
    {
      act("$N doesn't want to be in your group.", TRUE, ch, 0, victim,
          TO_CHAR);
      return;
    }
  }

  /*
     if(IS_ILLITHID(ch) && !IS_NPC(victim) && IS_ILLITHID(victim)) {
     send_to_char("You feel more like being alone right now...\n", ch);
     return;
     }
   */

        /*
         * Group caps for CHAOS_MUD only
         * group sizes are controlled in config.h
         */

  if (!group_add_member(ch, victim))
    return;

  act("$N is now a member of your group.", TRUE, ch, 0, victim, TO_CHAR);
  act("$N is now a member of $n's group.", TRUE, ch, 0, victim, TO_NOTVICT);
  act("You are now a member of $n's group.", FALSE, ch, 0, victim, TO_VICT);
  //Client
  for (gl = victim->group; gl; gl = gl->next)
  {
    if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
    {
      gl->ch->desc->last_group_update = 1;
    }
  }
}



void do_disband(P_char ch, char *arg, int cmd)
{
  char     buf[MAX_STRING_LENGTH];

  if (!ch)
  {
    logit(LOG_DEBUG, "disband called with NULL ch");
    return;
  }
  if (!ch->group || (ch->group->ch != ch))
  {
    send_to_char("You must be the leader of a group to disband it.\n", ch);
    return;
  }
  send_to_char("You disband the group.\n", ch);

  sprintf(buf, "%s has disbanded the group.\n", GET_NAME(ch));

  while (ch->group && ch->group->next)
  {
    send_to_char(buf, ch->group->next->ch);
    group_remove_member(ch->group->next->ch);
  }
}

P_char in_command_aura(P_char ch)
{
  if (ch->group && ch->in_room == ch->group->ch->in_room &&
      has_innate(ch->group->ch, INNATE_COMMAND_AURA))
    return ch->group->ch;
  else
    return NULL;
}

void add_aura_message(P_char ch, P_char commander)
{
  if (ch == commander)
    if (GET_CLASS(ch, CLASS_AVENGER))
      act
        ("You feel stronger as more believers join your cause!",
         FALSE, ch, 0, commander, TO_CHAR);
    else
      act
        ("You feel like you would taste &+Rblood&n soon as more adventurers join you!",
         FALSE, ch, 0, commander, TO_CHAR);
  else
    if (GET_CLASS(ch, CLASS_AVENGER))
      act
        ("You feel &+Wfaith&n growing in your heart as $N takes over the command!",
         FALSE, ch, 0, commander, TO_CHAR);
    else
      act
        ("You feel like you would taste &+Rblood&n soon as $N takes over the command!",
         FALSE, ch, 0, commander, TO_CHAR);
  balance_affects(ch);
}

void remove_aura_message(P_char ch, P_char commander)
{
  if (ch == commander)
    act("You feel abandoned!", FALSE, ch, 0, commander,
        TO_CHAR);
  else
    act("You no longer feel $N's support!", FALSE, ch, 0, commander, TO_CHAR);
  balance_affects(ch);
}

bool group_remove_member(P_char ch)
{
  struct group_list *gl, *elem, *temp_gl;

  /* remove 'ch' from a group.  Deal with removing the group leader,
     and deal with what happens when less then 2 people left in the
     group. */

  if (!ch->group)
    return TRUE;


  /* okay.. 2 possible special conditions:
     1) group LEADER is removed.
     2) only 2 people in the group now, so remove all members
   */


  gl = ch->group;

  if (ch == gl->ch)
  {                             /* group leader */

    /* move all the group members to point to the new group leader
       (who is the second person in the group list */
    for (elem = gl->next; elem; elem = elem->next)
    {
      if (IS_AFFECTED3(elem->ch, AFF3_PALADIN_AURA))
      purge_linked_auras(elem->ch);
      if (in_command_aura(elem->ch))
        remove_aura_message(elem->ch, ch);
      elem->ch->group = gl->next;
      if (in_command_aura(elem->ch))
        add_aura_message(elem->ch, gl->next->ch);
    }

    elem = gl->next;            /* remember who the new leader is... */
    mm_release(dead_group_pool, gl);    /* free the old struct */
    gl = elem;                  /* and put 'gl' to point to the new
                                   leader */

  }
  else
  {
    /* okay.. its not the group leader... lets figure out WHO, by
       looping  */

    for (elem = gl; elem; elem = elem->next)
      if (elem->next->ch == ch)
        break;

    /* okay.. serious possible problem:  if ch is pointing to a group
       list, but they aren't in that list, the below code will catch */
    if (!elem)
    {
      wizlog(60, "GROUP: %s claims to be a member when he isn't!",
             GET_NAME(ch));
    }
    else
    {
      /* okay.. elem->next is the element to remove.  don't forget to
         shift! */
      if (in_command_aura(ch))
        remove_aura_message(ch, ch->group->ch);
      gl = elem->next;          /* this is the one to be removed.. */
      elem->next = elem->next->next;    /* shift! */
      mm_release(dead_group_pool, gl);  /* remove the old */
      gl = ch->group;           /* and reset gl to the group leader */
    }
  }

  /* group is too small.. dispand it */
  if (!gl->next)
  {                             /* only 1 person in the group */
    /* silently disband it */
    if (in_command_aura(gl->ch))
      remove_aura_message(gl->ch, gl->ch);
    gl->ch->group = NULL;
    send_to_char("Your group has been disbanded.\n", gl->ch);
    mm_release(dead_group_pool, gl);
    gl = NULL;
  }
  /* group leader changed.. tell them about it */
  if (gl && (ch->group != gl))
    send_to_char("You are now the leader of your group!\n", gl->ch);

  /* let the group leader know that someone left... */
  if (gl)
  {
    char     buf[MAX_STRING_LENGTH];

    sprintf(buf, "%s is no longer in your group.\n",
            IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch));
    send_to_char(buf, gl->ch);
  }
  ch->group = NULL;
  REMOVE_BIT(ch->specials.act2, PLR2_BACK_RANK);

  /* check for rank balance, move the first poor sap in the back rank up to front */
  if (gl && free_back_slots(gl->ch) < 0)
    fix_group_ranks(gl->ch);
  for (gl = ch->group; gl; gl = gl->next)
  {
    if (gl->ch && gl->ch->desc && gl->ch->desc->term_type == TERM_MSP)
    {
      gl->ch->desc->last_group_update = 1;
    }
  }
  return TRUE;
}

int num_group_members_in_room(P_char ch)
{
  int numb = 0;

  if( !ch || !ch->group )
    return 0;

  struct group_list *group = ch->group;

  while (group)
  {
    if (ch != group->ch && IS_PC(group->ch) && group->ch->in_room == ch->in_room)
    {
      numb++;
    }
    group = group->next;
  }

  return numb;
}

int get_numb_chars_in_group(struct group_list *group)
{
  int      numb = 0;

  while (group)
  {
    if (IS_PC(group->ch))
    {
      numb++;
    }
    group = group->next;
  }

  return numb;
}

int get_mob_numb_in_group(struct group_list *group)
{
  int      numb = 0;

  while (group)
  {
    if (IS_NPC(group->ch))
    {
      numb++;
    }
    group = group->next;
  }

  return numb;
}


int evil_race_in_group(struct group_list *group)
{
  while (group)
  {
    if (EVIL_RACE(group->ch))
      return TRUE;

    //if (IS_PUNDEAD(group->ch))
    //  return TRUE;

    group = group->next;
  }

  return FALSE;
}

int undead_race_in_group(struct group_list *group)
{
  while (group)
  {
    if (IS_PUNDEAD(group->ch))
      return TRUE;

    group = group->next;
  }

  return FALSE;
}

bool group_add_member(P_char leader, P_char member)
{
  struct group_list *gl;
  char buf[MAX_STRING_LENGTH];

  if( !leader || !member || !IS_ALIVE(leader) || !IS_ALIVE(member) )
    return FALSE;

  if( member->group )
  {
    sprintf(buf, "%s is already in another group!\n", GET_NAME(member));
    send_to_char(buf, leader);
    return FALSE;
  }

  if( racewar(leader, member) && !IS_DISGUISE(member) )
  {
    send_to_char("You don't want to group with scum!\n", leader);
    return FALSE;
  }

#if 1
  if( IS_PC(member) && IS_PC(leader) && member != leader
    && !is_linked_to(leader, member, LNK_CONSENT) )
  {
    send_to_char("But ye haven't their permission to do that!\n", leader);
    return FALSE;
  }
#endif
#if 0
  if( GET_RACE(ch) == RACE_ILLITHID(leader) && IS_PC(member) )
  {
    send_to_char("You can't stand being near them!\n", leader);
    return FALSE;
  }
#endif

  // Check if the leader is a member of a group, but not the leader of it
  if( leader->group && (leader->group->ch != leader) )
  {
    send_to_char("This only works for the group leader!\n", leader);
    return FALSE;
  }

  if( leader->group )
  {
    int group_size = 0;
    for (gl = leader->group; gl; gl = gl->next)
    {
      group_size++;
    }

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (group_size >= 5)
    {
      send_to_char("Your group is too large!\n", leader);
      return FALSE;
    }
#else
    if (IS_RACEWAR_EVIL(leader) && !IS_PC_PET(member))
    {
      if (group_size >= (int) get_property("groups.size.max.evil", 13) )
      {
        send_to_char("Your group is too large!\n", leader);
        return FALSE;
      }
    }
    if (IS_RACEWAR_GOOD(leader) && !IS_PC_PET(member))
    {
      if (group_size >= (int) get_property("groups.size.max.good", 13) )
      {
        send_to_char("Your group is too large!\n", leader);
        return FALSE;
      }
    }
#endif
  }

  if (!leader->group)
  {
    if (!dead_group_pool)
      dead_group_pool = mm_create("GROUPS", sizeof(struct group_list), offsetof(struct group_list, next), 1);

    leader->group = (struct group_list *) mm_get(dead_group_pool);
    leader->group->ch = leader;
    leader->group->next = NULL;
    REMOVE_BIT(leader->specials.act2, PLR2_BACK_RANK);
    if (in_command_aura(leader))
    {
      add_aura_message(leader, leader);
    }
  }
  /* okay.. now put the new member in this group. */

  /* find the end of this leaders group list... */
  for (gl = leader->group; gl->next; gl = gl->next) ;

  gl->next = (struct group_list *) mm_get(dead_group_pool);
  gl->next->ch = member;
  gl->next->next = NULL;
  member->group = leader->group;
  REMOVE_BIT(member->specials.act2, PLR2_BACK_RANK);
  if (in_command_aura(member))
    add_aura_message(member, leader);
  return TRUE;
}


/* fonction to check if ch is guild golem */

int is_guild_golem(P_char ch, P_char pl)
{
  uint     bits;
  char    *tmp;
  int      allowed = FALSE;
  P_Guild  guild;

  if( !GET_ASSOC(ch) )
  {
    tmp = strstr(GET_NAME(ch), "assoc");
    if( !tmp )
    {
      return FALSE;
    }
    guild = get_guild_from_id(atoi( tmp + 5 ));

    if( guild == NULL )
    {
      return FALSE;
    }
    GET_ASSOC(ch) = guild;
  }

  tmp = strstr(GET_NAME(ch), "assoc");

  guild = GET_ASSOC(ch);

  allowed = FALSE;

  bits = GET_A_BITS(pl);

  allowed = ( (( GET_ASSOC(pl) == guild ) && IS_MEMBER( bits ) && GT_PAROLE( bits )) || IS_TRUSTED(pl) );

  return (allowed);
}
