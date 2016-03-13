#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "interp.h"
#include "prototypes.h"
#include "events.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "damage.h"
#include "specs.eth2.h"

extern P_room world;
extern P_index mob_index;
extern P_index obj_index;
extern P_char character_list;

void do_assist_core(P_char ch, P_char victim);
void transfer_inventory(P_char ch, P_char recipient);
bool in_array(int val, int arr[]);
int array_size(int arr[]);

int eth2_forest_animal(P_char ch, P_char pl, int cmd, char *arg)
{
  int replace_mobs[] = {32636, 32630};
  int replace_mobs_size = 2;

  if( cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC )
    return FALSE;

  if( !ch )
    return FALSE;
  
  SET_BIT(ch->specials.act, ACT_SPEC_DIE);
  
  if( cmd == CMD_DEATH )
  {
    int vnum = replace_mobs[ number(0, replace_mobs_size-1) ];
    
    P_char mob = read_mobile(vnum, VIRTUAL);
    
    if( !mob )
    {
      logit(LOG_DEBUG, "eth2_forest_animal(): could not find replacement mob for mob vnum [%d]!", GET_VNUM(ch) );
      debug("eth2_forest_animal(): could not find replacement mob for mob vnum [%d]!", GET_VNUM(ch) );
      return FALSE;
    }

    act("\n&+LThe echoes from $n&+L's death cry reverberate, coalescing into something from your nightmares!", FALSE, ch, 0, 0, TO_ROOM); 
    
    money_to_inventory(ch);
    unequip_all(ch);
    transfer_inventory(ch, mob);
    
    SET_BIT(mob->specials.act, ACT_BREATHES_SHADOW);
    char_to_room(mob, ch->in_room, 0);
    BreathWeapon(mob, -1);
    return TRUE;
  }
  
  return FALSE;
}

int eth2_little_girl(P_char ch, P_char pl, int cmd, char *arg)
{
  int replace_mob_vnum = 32631;
  
  if( cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC )
    return FALSE;
  
  if( !ch )
    return FALSE;
  
  SET_BIT(ch->specials.act, ACT_SPEC_DIE);
  
  if( cmd == CMD_DEATH )
  {
    P_char mob = read_mobile(replace_mob_vnum, VIRTUAL);
    
    if( !mob )
    {
      logit(LOG_DEBUG, "eth2_little_girl(): could not find replacement mob for mob vnum [%d]!", GET_VNUM(ch) );
      debug("eth2_little_girl(): could not find replacement mob for mob vnum [%d]!", GET_VNUM(ch) );
      return FALSE;
    }
    
    act("\n&+LThe echoes from $n&+L's death cry reverberate, coalescing into something from your nightmares!", FALSE, ch, 0, 0, TO_ROOM); 

    money_to_inventory(ch);
    unequip_all(ch);
    transfer_inventory(ch, mob);
    
    SET_BIT(mob->specials.act, ACT_BREATHES_SHADOW);
    char_to_room(mob, ch->in_room, 0);
    BreathWeapon(mob, -1);
    return TRUE;
  }
  
  return FALSE;
}

int eth2_demon_princess(P_char ch, P_char pl, int cmd, char *arg)
{
  int forest_creatures[] = { 32624, 32625, 32626, -1 };
  
  int helpers[] = { 32636 };
    
  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  
  if( !ch )
    return FALSE;
  
  if( cmd == CMD_PERIODIC )
  {
    if( !number(0, 3) && IS_FIGHTING(ch) )
    {
      /* go through and kill all the forest animals, which should create the demons */
      P_char target, next_char;
      for (target = character_list; target; target = next_char)
      {
        next_char = target->next;
        
        if( IS_NPC(target) && in_array( GET_VNUM(target), forest_creatures) )
        {
          act("&+L$n &+Lstiffens and quivers slightly before being torn to pieces from within.", FALSE, target, 0, 0, TO_ROOM); 
          die(target, ch);
        }        
      }

      return shout_and_hunt(ch, 100,
                            "&+LFaded nightmares, form and punish %s!&n",
                            NULL,
                            helpers, 0, 0);
    }
  }
  
  return FALSE;
}

int eth2_aramus(P_char ch, P_char pl, int cmd, char *arg)
{
  int helpers[] = {32638, 32639, 32640, 32641, -1};
  int helpers_size = 4;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_PERIODIC )
  {
    // 1/5 chance.
    if( !number(0,4) && IS_FIGHTING(ch) )
    {
      /* check to see how many of his helpers are currently here. if less than threshold,
         load a new one */
      int num_followers = 0;
      for( struct follow_type *followers = ch->followers; followers; followers = followers->next )
      {
        if( IS_NPC(followers->follower) && in_array( GET_VNUM(followers->follower), helpers ) )
        {
          num_followers++;
        }
      }

      if( num_followers < 3 )
      {
        // load a helper
        int vnum = helpers[number(0, helpers_size-1)];

        P_char mob = read_mobile(vnum, VIRTUAL);

        if( !mob )
        {
          logit(LOG_DEBUG, "eth2_aramus(): could not load mob vnum [%d]!", vnum );
          debug("eth2_aramus(): could not load mob vnum [%d]!", vnum );
          return FALSE;
        }

        char_to_room(mob, ch->in_room, 0);
        act("$n arrives to assist $s master!", FALSE, mob, 0, 0, TO_ROOM);

        add_follower(mob, ch);
        group_add_member(ch, mob);
        do_assist_core(mob, ch);
        return TRUE;
      }
    }
  }
  return FALSE;
}

int eth2_tree_obj(P_obj obj, P_char ch, int cmd, char *arg)
{
  /* first value is the object, second is the mob to replace it with */
  int replace_objs[][2] = { {32615, 32628}, {32614, 32629}, {-1, -1} };

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !obj || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_NORTH )
  {
    if( IS_NPC(ch) )
    {
      return TRUE;
    }

    if( IS_TRUSTED(ch) )
    {
      act("Above in you in the &+Ldarkness&n, leaves and branches start to creak and groan\n" \
          "but then become quiet again.\n", FALSE, ch, 0, 0, TO_CHAR);
      return FALSE;
    }

    act("Above in you in the &+Ldarkness&n, leaves and branches start to creak and groan\n"
      "as the &+gforest &+Gceiling&n comes alive!\n", FALSE, ch, 0, 0, TO_ROOM);

    act("Above in you in the &+Ldarkness&n, leaves and branches start to creak and groan\n"
      "as the &+gforest &+Gceiling&n comes alive!\n", FALSE, ch, 0, 0, TO_CHAR);

    /* replace tree objects with the tree mobs */
    P_obj o, next_obj;
    for( o = world[ch->in_room].contents; o; o = next_obj )
    {
      next_obj = o->next_content;

      for( int i = 0; replace_objs[i][0] != -1; i++ )
      {
        if( OBJ_VNUM(o) == replace_objs[i][0] )
        {
          P_char mob = read_mobile( replace_objs[i][1], VIRTUAL );
          if( !mob )
          {
            logit(LOG_DEBUG, "eth2_tree_obj(): could not find mob vnum [%d]!", replace_objs[i][1] );
            debug("eth2_tree_obj(): could not find mob vnum [%d]!", replace_objs[i][1] );
            return FALSE;
          }

          char_to_room(mob, ch->in_room, -1);
          act("$n stirs $mself out of $s slumber and roars at you in fury!", FALSE, mob, 0, ch, TO_VICT);
          act("$n stirs $mself out of $s slumber and roars at $N in fury!", FALSE, mob, 0, ch, TO_NOTVICT);

          branch(mob, ch);
          MobStartFight(mob, ch);

          obj_from_room(o);
        }
      }
    }
    return TRUE;
  }
  return FALSE;
}

int eth2_godsfury(P_obj obj, P_char ch, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( OBJ_WORN(obj) || OBJ_ROOM(obj) )
      hummer(obj);

    if( OBJ_WORN(obj) )
    {
      ch = obj->loc.wearing;
    }
    else
    {
      return FALSE;
    }

    int curr_time = time(NULL);

    if( !CHAR_IN_NO_MAGIC_ROOM(ch) )
    {
      if (obj->timer[0] + 20 <= curr_time)
      {
        obj->timer[0] = curr_time;

        if (GET_HIT(ch) < GET_MAX_HIT(ch))
        {
          act("&+L$n&+L gasps slightly as $q &+Linfuses $m with energy.", FALSE, ch, obj, 0, TO_ROOM);
          act("&+LYou gasp as $q &+Linfuses you with energy.", FALSE, ch, obj, 0, TO_CHAR);
          spell_heal(60, ch, 0, 0, ch, 0);
          return TRUE;
        }
      }
    }
    return TRUE;
  }

  // 1/20 chance.
  if( cmd == CMD_MELEE_HIT && !number(0,19) && CheckMultiProcTiming(ch))
  {
    P_char vict = (P_char) arg;
    if( !vict )
    {
      return FALSE;
    }

    act("Your $q calls down the &+rfury&n of the Gods on $N!", FALSE, ch, obj, vict, TO_CHAR);
    act("$n's $q calls down the &+rfury&n of the Gods on you!", FALSE, ch, obj, vict, TO_VICT);
    act("$n's $q calls down the &+rfury&n of the Gods on $N!", FALSE, ch, obj, vict, TO_NOTVICT);

    int save = vict->specials.apply_saving_throw[SAVING_SPELL];
    vict->specials.apply_saving_throw[SAVING_SPELL] += 15;

    switch( number(0,3) )
    {
      case 0:
        spell_blindness(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        vict->specials.apply_saving_throw[SAVING_SPELL] = save;
        break;
      case 1:
        spell_disease(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        vict->specials.apply_saving_throw[SAVING_SPELL] = save;
        break;
      case 2:
        // Curse changes apply...[SAVING_SPELL] if succesful.
        vict->specials.apply_saving_throw[SAVING_SPELL] = save;
        spell_curse(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case 3:
        // Dispel magic may change apply...[SAVING_SPELL] if succesful.
        vict->specials.apply_saving_throw[SAVING_SPELL] = save;
        spell_dispel_magic(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
    }
    return TRUE;
  }
  return FALSE;
}

int eth2_aramus_crown(P_obj obj, P_char ch, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  
  if( !obj )
    return FALSE;
  
  if( cmd == CMD_PERIODIC )
  {
    hummer(obj);    
    
    if (OBJ_WORN(obj))
      ch = obj->loc.wearing;
    else
      return FALSE;
    
    int curr_time = time(NULL);
    
    if (obj->timer[0] + 10 <= curr_time)
    {
      obj->timer[0] = curr_time;
      
      if (!IS_AWAKE(ch))
      {
        if( !affected_by_spell(ch, SPELL_REGENERATION) )
        {
          struct affected_type af;
          bzero(&af, sizeof(af));
          af.type = SPELL_REGENERATION;
          af.duration = 10;
          af.location = APPLY_HIT_REG;
          af.modifier = 3 * GET_LEVEL(ch);
          affect_to_char(ch, &af);            
          
          send_to_char("As you settle into your dreams, your body begins to heal itself quicker.\r\n", ch);
          add_event(event_aramus_crown_sleep_check, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
        }
        
        switch( number(0, 6) )
        {
          case 0:
            if (!IS_AFFECTED(ch, AFF_HASTE))
            {
              send_to_char("Your dreams are filled with visions of ultimate speed.\r\n", ch);
              spell_haste(GET_LEVEL(ch), ch, 0, 0, ch, 0);
            }
            break;
            
          case 1:
            if (!affected_by_spell(ch, SPELL_STONE_SKIN))
            {
              send_to_char("You dream of a lonely hillside, covered with rocky shale.\r\n", ch);
              spell_stone_skin(GET_LEVEL(ch), ch, 0, 0, ch, 0);
            }
            break;
            
          case 2:
            if (!IS_AFFECTED(ch, AFF_FLY))
            {
              send_to_char("You dream of soaring above the fjords.\r\n", ch);
              spell_fly(GET_LEVEL(ch), ch, 0, 0, ch, 0);
            }
            break;
            
          case 3:
            if (!IS_AFFECTED(ch, AFF_BARKSKIN))
            {
              send_to_char("You dream of a dark and mysterious forest.\r\n", ch);
              spell_barkskin(GET_LEVEL(ch), ch, 0, 0, ch, 0);
            }
            break;
            
          case 4:
            if (!affected_by_spell(ch, SPELL_VITALITY))
            {
              send_to_char("In your dream, you feel life essence flowing freely through your veins.\r\n", ch);
              spell_vitality(GET_LEVEL(ch), ch, 0, 0, ch, 0);
            }
            break;
            
          case 5:
            if (!affected_by_spell(ch, SPELL_INERTIAL_BARRIER))
            {
              send_to_char("Your dreams are filled with strange visions of interrupted motion.\r\n", ch);
              spell_inertial_barrier(GET_LEVEL(ch), ch, 0, 0, ch, 0);
            }
            break;
            
          case 6:
            send_to_char("You have a horrible nightmare of appearing in public totally naked!\r\n", ch);
            unequip_all(ch);
            break;
          
        }
        
        return TRUE;
      }
    }
    
    return TRUE;
  }
    
  return FALSE;
}

void event_aramus_crown_sleep_check(P_char ch, P_char vict, P_obj obj, void *data)
{
  if( IS_AWAKE(ch) )
  {
    send_to_char("Your body slows down as you blink the sleep from your eyes.\r\n", ch);
    affect_from_char(ch, SPELL_REGENERATION);
  }
  else
  {
    add_event(event_aramus_crown_sleep_check, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
  }  
}
