#include "stdio.h"
#include "string.h"
#include "time.h"

#include "rogues.h"
#include "db.h"
#include "interp.h"
#include "spells.h"
#include "utils.h"
#include "comm.h"
#include "events.h"
#include "objmisc.h"
#include "prototypes.h"
#include "structs.h"
#include "weather.h"
#include "justice.h"
#include "damage.h"
#include "guard.h"
#include "sql.h"

extern struct str_app_type str_app[];
extern P_room world;

void do_slip(P_char ch, char *argument, int cmd)
{
  char obj_name[MAX_INPUT_LENGTH], vict_name[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  P_char vict, rider;
  P_obj obj, container;
  P_char t_ch = NULL;
  int success, percent, check, bits, bits2;
  bool putsuc = FALSE;

  argument = one_argument(argument, obj_name);

  argument = one_argument(argument, vict_name);

  /* do they have the skill? */
  if (!(GET_CHAR_SKILL(ch, SKILL_SLIP)))
  {
    send_to_char("You don't know how.\r\n", ch);
    return;
  }

  // Are we targeting a person or object?
  bits = generic_find(vict_name, FIND_OBJ_INV, ch, &t_ch, &container);
  bits2 = generic_find(obj_name, FIND_OBJ_INV, ch, &t_ch, &obj);
  if (container = get_obj_in_list_vis(ch, vict_name, ch->carrying))
  {
    if (bits && bits2 && (container->type == ITEM_CONTAINER))
    {
      // Will it fit?
      if (((GET_OBJ_WEIGHT(obj) + GET_OBJ_WEIGHT(container)) <= container->value[0]) ||
         ((container->value[0] == -1)))
      {
        putsuc = put(ch, obj, container, FALSE);
        act("You slip $p into $P...", TRUE, ch, obj, container, TO_CHAR);
        if ((GET_CHAR_SKILL(ch, SKILL_SLIP) - GET_OBJ_WEIGHT(obj)) < number(1, 100))
        {
          send_to_char("But others notice.\r\n", ch);
          act("You notice $n trying to sneak $p into $s $P.", TRUE, ch, obj, container, TO_NOTVICT);
        }
        else
        {
          send_to_char("Successfully, too.\r\n", ch);
        }
      }
      else
      {
        send_to_char("It wont fit.\r\n", ch);
      }
    }
    // Most likely wont see this message.
    else if (!bits && bits2)
    {
      send_to_char("Slip it into what?.\r\n", ch);
    }
    else if (bits && !bits2)
    {
      send_to_char("You don't seem to have anything like that.", ch);
    }
  } 
  else
  {

  /* Check if character can see obj/victim, or if obj is cursed, etc... */
  if (!*obj_name || !*vict_name)
  {
    send_to_char("Slip what to who?\r\n", ch);
    return;
  }
  if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
  {
    send_to_char("You do not seem to have anything like that.\r\n", ch);
    return;
  }
  if (IS_SET(obj->extra_flags, ITEM_NODROP) && !IS_TRUSTED(ch))
  {
    send_to_char("You can't let go of it! Yeech!!\r\n", ch);
    return;
  }
  if (!(vict = get_char_room_vis(ch, vict_name)))
  {
    send_to_char("No one by that name around here.\r\n", ch);
    return;
  }
  if ((IS_NPC(vict) && (GET_RNUM(vict) == real_mobile(20))) || IS_AFFECTED(vict, AFF_WRAITHFORM))
  {
    send_to_char("They couldn't carry that if they tried.\r\n", ch);
    return;
  }
  
  /* will it immobilize the victim? */
  if ((IS_CARRYING_W(vict, rider) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(vict))
  {
    send_to_char("They cannot possibly carry anymore!\r\n", ch);
    return;
  }
  else 
  /* skill check & success check */
  {
    percent = (GET_CHAR_SKILL(ch, SKILL_SLIP) - GET_OBJ_WEIGHT(obj));
    check = number(1, 100);

    if (IS_FIGHTING(ch))
    {
      percent -= (int)get_property("skill.slip.fightingPenalty", 20);
    }

    if (check <= percent)
    {
      success = 1;
    }
    else
    {
      success = 0;
    }
  }
  
  if (success)
  {
    /* notch skill */
    notch_skill(ch, SKILL_SLIP, (int)get_property("skill.notch.slip", 5));

    /* transfer the obj */
    obj_from_char(obj);
    obj_to_char(obj, vict);
    // If this is not suppost to be a secretive hand off, uncomment below.
    //act("$n slips $p to $N.", 1, ch, obj, vict, TO_NOTVICT);
    act("You successfuly slip $p into $N's pockets!", 0, ch, obj, vict, TO_CHAR);

    /* for now repor it anytime */
    if (IS_TRUSTED(ch) && GET_LEVEL(ch) < OVERLORD)
    {
      statuslog(GET_LEVEL(ch), "%s slips %s to %s.", J_NAME(ch), obj->short_description, J_NAME(vict));
      logit(LOG_WIZ, "%s slips %s to %s.", J_NAME(ch), obj->short_description, J_NAME(vict));
      sql_log(ch, WIZLOG, "Slips %s to %s", obj->short_description, J_NAME(vict));
    }
    if (ch != vict)
    {
      writeCharacter(ch, 1, ch->in_room);
      writeCharacter(vict, 1, vict->in_room);
    }

    /* something about a light bug in do_give */
    char_light(ch);
    room_light(ch->in_room, REAL);
    nq_action_check(ch, vict, NULL);
  }
  else
  {
    //act("The weight of %p proves too much for your slyness.", 0, ch, obj, 0, TO_CHAR);
    /* an idea: if you want a fail to place the object in the room (a penalty for poor skill) uncomment below */
    act("You fumble $p as its weight proves too much for your slyness.", 0, ch, obj, 0, TO_CHAR);
    act("$p falls to the ground as $n slyly looks the other way.", 0, ch, obj, 0, TO_NOTVICT);
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);

    writeCharacter(ch, 1, ch->in_room);
  }
  }
}
