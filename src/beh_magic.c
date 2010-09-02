/* beholder spells */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "damage.h"

extern const int rev_dir[];
extern const char *dirs[];
extern const char *dirs2[];
extern P_room world;

 /*
  * mostly based on AD&D beholders - get sleep, telekinesis, flesh to
  * stone (major para), disintegrate, fear, slow, and cause serious (more
  * like full harm on Duris)
  */

void spell_beholder_sleep(int level, P_char ch, P_char victim, P_obj obj)
{
  struct affected_type af;
  int save = 2;

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if(!IS_ALIVE(ch))
  {
    return;
  }
  
  appear(ch);

  if(resists_spell(ch, victim) ||
    IS_TRUSTED(victim))
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
    return;
  }

  if(IS_NPC(ch) &&
     GET_RACE(ch) == RACE_BEHOLDER)
  {
    save = (int) (GET_LEVEL(ch) / 10);
  }
  
  if(!NewSaves(victim, SAVING_SPELL, save) &&
    !IS_GREATER_RACE(victim) &&
    !IS_ELITE(victim) &&
    !IS_UNDEAD(victim) &&
    !IS_ANGEL(ch) &&
    !IS_ELEMENTAL(victim) &&
    !IS_AFFECTED(victim, AFF_SLEEP) &&
    GET_STAT(victim) != STAT_SLEEPING)
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_SLEEP;
    af.duration = 4 + (level < 0 ? -level : level);
    af.duration /= 10;
    af.duration += 2;
    af.bitvector = AFF_SLEEP;

    send_to_char
      ("&+LYou suddenly begin to get very drowsy!  Keeping the eyes open is becoming\r\n"
       "&+Lall but impossible! The world becomes a faint memory as you slip into\r\n"
       "&+Lla-la land.\r\n", victim);
       
    if(victim->specials.fighting)
    {
      stop_fighting(victim);
    }
    
    if(GET_STAT(victim) > STAT_SLEEPING)
    {
      act("&+W$n &+Wgoes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      SET_POS(victim, GET_POS(victim) + STAT_SLEEPING);
    }
    
    affect_join(victim, &af, FALSE, FALSE);
    /*
     * stop all non-vicious/agg attackers
     */
    StopMercifulAttackers(victim);
    return;
  }
  else
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
    return;
  }

  if(IS_NPC(victim) &&
    CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    
    if(!IS_FIGHTING(victim))
    {
      MobStartFight(victim, ch);
    }
  }
}

void spell_beholder_telekinesis(int level, P_char ch, P_char victim,
                                P_obj obj)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  struct room_direction_data *back;
  int      door, other_room;

  if (!ch)
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if(!IS_ALIVE(ch))
  {
    return;
  }
  
  appear(ch);
  
  /* check for doors to close */

  for(door = 0; door < NUM_EXITS; door++)
  {
    if(EXIT(ch, door))
    {
      /*
       * perhaps it is a door
       */

      if(!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      {
        continue;               /* nope */
      }
      
      /* woohoo, an open door that is closeable.  let's close this badboy */

      if(!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      {
        if(IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) ||     /* doh */
           IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED))
        {
          continue;
        }
        
        SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);

        if ((door != DOWN) && (door != UP))     /* up to up/down */
        {
          if (EXIT(ch, door)->keyword)
          {
            sprintf(Gbuf1, "The $F to %s closes suddenly!", dirs2[door]);
            act(Gbuf1, FALSE, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
          }
          else
          {
            sprintf(Gbuf1, "The door to %s closes suddenly!", dirs2[door]);
            act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
          }
        }
        else                    /* either 4 [up] or 5 [down] */
        {
          if (EXIT(ch, door)->keyword)
          {
            sprintf(Gbuf1, "The $F %s closes suddenly!", dirs2[door]);
            act(Gbuf1, FALSE, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
          }
          else
          {
            sprintf(Gbuf1, "The door %s closes suddenly!", dirs2[door]);
            act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
          }
        }

        /* close door on other side, if possible */

        if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        {
          if ((back = world[other_room].dir_option[rev_dir[door]]))
          {
            SET_BIT(back->exit_info, EX_CLOSED);

            if (back->to_room == ch->in_room)
            {
              if (back->keyword)
              {
                sprintf(Gbuf1,
                        "The %s is closed from the other side.\r\n",
                        FirstWord(back->keyword));
                send_to_room(Gbuf1, EXIT(ch, door)->to_room);
              }
              else
                send_to_room("The door is closed from the other side.\r\n",
                             EXIT(ch, door)->to_room);
            }
          }
        }
      }
    }
  }
}

void spell_beholder_paralyze(int level, P_char ch, P_char victim, P_obj obj)
{
  struct affected_type af;
  int lev = level, save = 2;

  if(!ch)
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if(!IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }

  if(resists_spell(ch, victim) ||
     IS_TRUSTED(victim) ||
     (IS_NPC(victim) &&
     IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA)))
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
    return;
  }

  /*
   * beholder paralyze is 'flesh to stone', so we go for save vs para
   */
   
  if(IS_NPC(ch) &&
     GET_RACE(ch) == RACE_BEHOLDER)
  {
    save = (int) (GET_LEVEL(ch) / 10);
  }

  if((lev < 0) ||
    !NewSaves(victim, SAVING_PARA, save))
  {
    if (lev < 0)
    {
      lev = -lev;
    }

    if(IS_AFFECTED2(victim, AFF2_MAJOR_PARALYSIS))
    {
      return;
    }
    
    bzero(&af, sizeof(af));
    af.flags = AFFTYPE_SHORT;
    af.type = SPELL_MAJOR_PARALYSIS;
    af.duration = WAIT_SEC * 30;
    af.bitvector2 = AFF2_MAJOR_PARALYSIS;

    affect_to_char(victim, &af);

    act("&+L$n&+L screams out in surprise as $s flesh turns into stone!",
        FALSE, victim, 0, 0, TO_ROOM);
    send_to_char
      ("&+LA short gasp escapes your lips as your flesh turns into stone.\r\n",
       victim);
    if(IS_FIGHTING(victim))
    {
      stop_fighting(victim);
    }
    
    /*
     * stop all non-vicious/agg attackers
     */
    StopMercifulAttackers(victim);

  }
  else if(IS_NPC(victim) &&
          (CAN_SEE(victim, ch)))
  {
    remember(victim, ch);
    if(!IS_FIGHTING(victim))
    {
      MobStartFight(victim, ch);
    }
  }
  else
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
    return;
  }
}

void spell_beholder_disintegrate(int level, P_char ch, P_char victim,
                                 P_obj obj)
{
  int i, dam, gotone = FALSE, save = 2;
  P_obj x;
  struct damage_messages messages = {
    "$N&n reels in pain as $S flesh is disintegrated into ions by your beam.",
    "You reel in pain as a large chunk of your flesh is disintegrated.",
    "$N&n reels in pain as $S flesh is disintegrated into mere ions.",
    "The force of the blast destroys $N&n, causing an early death.",
    "The force of the blast destroys you, causing an early death.",
    "The force of the blast destroys $N&n, leading to $S early death."
  };

  if(!ch)
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  
  if(!IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  if(IS_NPC(ch) &&
     GET_RACE(ch) == RACE_BEHOLDER)
  {
    save = (int) (GET_LEVEL(ch) / 10);
  }
  
  dam = dice(level, 12);

  if(!NewSaves(victim, SAVING_SPELL, save) &&
    !IS_TRUSTED(victim))
  {
    /* only destroy one piece of eq at a time, and even then only if the PC
       gets unlucky */

    if(number(0, 5) == 3)
    {
      for (i = 0; i < MAX_WEAR; i++)    /* victim->equipment nuttiness */
      {
        if(victim->equipment[i])
        {
          gotone = TRUE;
        }
      }

      /* only bother trying if they actually have eq (otherwise the mud would
         tend to lock up in an unterminable loop) */

      if(gotone)
      {
        gotone = FALSE;

        do
        {
          i = number(1, MAX_WEAR - 1);

          if(victim->equipment[i])
          {
            gotone = TRUE;

            obj = victim->equipment[i];

            if(!IS_ARTIFACT(obj))
            {
            
              act("$N&n's $q&n turns red hot, disappearing in a puff of smoke!",
                TRUE, ch, obj, victim, TO_VICT);

              if(OBJ_CARRIED(obj))
              {                   /* remove the obj */
                obj_from_char(obj, TRUE);
              }
              else if(OBJ_WORN(obj))
              {
                unequip_char_dale(obj);
              }
              else if(OBJ_INSIDE(obj))
              {
                obj_from_obj(obj);
                obj_to_room(obj, ch->in_room);
              }
              if(obj->contains)
              {
                while (obj->contains)
                {
                  x = obj->contains;
                  obj_from_obj(x);
                  obj_to_room(x, ch->in_room);
                }
              }
              if(obj)
              {
                extract_obj(obj, TRUE);
                obj = NULL;
              }
            }
            else
            {
              act("$N&n's $q&n turns red hot, only to cool down a second later.",
                  TRUE, ch, obj, victim, TO_VICT);
            }
          }
        }
        while (!gotone);
      }                         /* if gotone */
    }                           /* if (!number(...)) */
  }
  else
  {
    dam >>= 1;
  }

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}


void spell_beholder_fear(int level, P_char ch, P_char victim, P_obj obj)
{
  struct affected_type af;
  int save = 2;

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  
  if(!IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }

  appear(ch);
  
  if(resists_spell(ch, victim) ||
    affected_by_spell(victim, SPELL_FEAR))
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
    return;
  }

  if(IS_GREATER_RACE(victim) ||
    IS_ELITE(victim) ||
    IS_UNDEAD(victim) ||
    IS_ANGEL(victim) ||
    IS_TRUSTED(victim))
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
    return;
  }
  
  if(IS_NPC(ch) &&
     GET_RACE(ch) == RACE_BEHOLDER)
  {
    save = (int) (GET_LEVEL(ch) / 10);
  }

  if(!NewSaves(ch, SAVING_FEAR, save))
  {
    bzero(&af, sizeof(af));
    
    if(affected_by_spell(victim, SKILL_BERSERK))
    {
      act("The beam slams into $n!", FALSE, victim, 0, 0, TO_ROOM);
      act("You feel a strange sensation overcome you...",
          TRUE, ch, obj, victim, TO_VICT);
      CharWait(victim, PULSE_VIOLENCE);
      affect_from_char(victim, SKILL_BERSERK);
      send_to_char
        ("Your blood cools, and you no longer see targets everywhere.\r\n",
         victim);
      act("$n seems to have overcome $s battle madness.", TRUE, victim, 0, 0,
          TO_ROOM);
      return;
    }    

    if(fear_check(victim))
    {
      act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
          TO_ROOM);
      return;
    }

    af.type = SPELL_FEAR;
    af.location = APPLY_DAMROLL;
    af.modifier = -2 - (level / 10);
    af.duration = 3 + (level / 10);
    affect_to_char(victim, &af);

    af.location = APPLY_HITROLL;
    af.modifier = -2 - (level / 10);
    affect_to_char(victim, &af);

    send_to_char("&+LA wave of utter terror overcomes you!\r\n", victim);
    do_flee(victim, 0, 2);
  }
  else
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
  }

  if(ch->in_room == victim->in_room)
  {
    /*
     * they didn't flee, know what happens when you corner a scared
     * rat?
     */
    if(IS_NPC(victim) &&
      CAN_SEE(victim, ch))
    {
      remember(victim, ch);
      if (!IS_FIGHTING(victim))
      {
        MobStartFight(victim, ch);
      }
    }
  }
}


void spell_beholder_slowness(int level, P_char ch, P_char victim, P_obj obj)
{
  struct affected_type af;
  int save = 2;

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  
  if(!IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }

  appear(ch);

  if(resists_spell(ch, victim) ||
    IS_TRUSTED(victim))
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
    return;
  }

  if(IS_NPC(ch) &&
     GET_RACE(ch) == RACE_BEHOLDER)
  {
    save = (int) (GET_LEVEL(ch) / 10);
  }
  
  if (!NewSaves(victim, SAVING_SPELL, save))
  {
    bzero(&af, sizeof(af));
    af.type = SPELL_SLOW;
    af.duration = (level / 10) + 2;
    af.modifier = 2;
    af.bitvector2 = AFF2_SLOW;

    affect_to_char(victim, &af);

    act("&+M$n &+Mbegins to sllooowwww down.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&+MYou feel yourself slowing down.\r\n", victim);
  }
  else
  {
    act("The beam seems to have no effect on $n&n.", FALSE, victim, 0, 0,
        TO_ROOM);
    send_to_char("You feel no ill effects.\r\n", victim);
  }

  if(IS_NPC(victim) &&
    CAN_SEE(victim, ch))
  {
    remember(victim, ch);
    if (!IS_FIGHTING(victim))
    {
      MobStartFight(victim, ch);
    }
  }
}


void spell_beholder_damage(int level, P_char ch, P_char victim, P_obj obj)
{
  int  dam;

  struct damage_messages messages = {
    "$N&n nearly stumbles after being slammed by your beam.",
    "You nearly stumble as you're hit by the intense force of $n&n's beam.",
    "$N&n's body is knocked back by the intense force of $n&n's beam.",
    "The brute force of the blast reduces $N&n to a mere shadow of $S former self.",
    "The brute force of the blast reduces you into a mere shadow of your formerself.",
    "The brute force of the blast reduces $N&n into a mere shadow of $S former self."
  };

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  
  if(!IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }

  dam = level * 6 + number(-40, 40);
  
  if(dam < 1)
  {
    dam = 1;
  }

  spell_damage(ch, victim, dam, SPLDAM_GENERIC,
               SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
}

void spell_beholder_dispelmagic(int level, P_char ch, P_char victim, P_obj obj)
{
  struct damage_messages messages = {
    "$N&n &+Wshimmers &+wafter being slammed by your beam.",
    "&+wYou &+Wshimmer &+was you're hit by&n $n's&n &+wbeam.",
    "$N's &+wentire body &+Wshimmers &+wupon&n $n's&n &+wbeam's collision!",
    "",
    "",
    ""
  };

  if (!(victim && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  
  if(!IS_ALIVE(ch) ||
     !IS_ALIVE(victim))
  {
    return;
  }
  
  spell_dispel_magic((GET_LEVEL(ch) + 10), ch, 0, SPELL_TYPE_SPELL, victim, NULL);
}

