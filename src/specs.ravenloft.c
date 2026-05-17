/* Made Jexni, for ravenloft */
#include <ctype.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
;

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include "assocs.h"
#include "damage.h"
#include "graph.h"
#include "justice.h"
#include "reavers.h"
#include "specs.prototypes.h"
#include "specs.ravenloft.h"
#include "spells.h"
#include "vnum.obj.h"
#include "vnum.room.h"
#include "weather.h"

extern P_char      character_list;
extern P_desc      descriptor_list;
extern P_index     mob_index;
extern P_index     obj_index;
extern P_room      world;
extern P_obj       justice_items_list;
extern char       *coin_names[];
extern const char *command[];
extern const char *dirs[];
// extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int                    planes_room_num[];
extern int                    racial_base[];
extern int                    top_of_zone_table;
extern struct command_info    cmd_info[MAX_CMD_LIST];
extern struct str_app_type    str_app[];
extern struct time_info_data  time_info;
extern struct zone_data      *zone;
extern struct zone_data      *zone_table;
extern const char            *crime_list[];
extern const char            *crime_rep[];
extern const char            *specdata[][MAX_SPEC];
extern struct class_names     class_names_table[];
int                           range_scan_track(P_char ch, int distance, int type_scan);
extern P_obj                  object_list;

int ravenloft_vistani_shout(P_char ch, P_char tch, int cmd, char *arg)
{
	int helpers[] = {58382, 0};
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 4))
		return shout_and_hunt(ch, 200, "&+cVISTANI, TO ARMS! %s has attacked me!\n", NULL, helpers, 0, 0);

	return FALSE;
}

int ravenloft_bell(P_obj bell, P_char ch, int cmd, char *arg)
{
	P_obj  weapon    = NULL;
	P_obj  swordcase = NULL;
	P_room room      = NULL;
	char   buf[MAX_STRING_LENGTH];
	int    roomIndex;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!bell || !IS_ALIVE(ch) || !arg)
	{
		return FALSE;
	}

	if (!(weapon = ch->equipment[WIELD]) || OBJ_VNUM(weapon) != VOBJ_RAVENLOFT_MALLET || !OBJ_WORN_POS(weapon, WIELD))
	{
		return FALSE;
	}

	P_obj obj = ch->equipment[WIELD];

	if (cmd == CMD_HIT)
	{
		if (isname(arg, "bell"))
		{
			roomIndex = real_room0(VROOM_RAVENLOFT_SWORDCASE);
			for (swordcase = world[roomIndex].contents; swordcase; swordcase = swordcase->next_content)
			{
				if (OBJ_VNUM(swordcase) == VOBJ_RAVENLOFT_LOCKED_CASE)
				{
					break;
				}
			}
			if (!swordcase)
			{
				return FALSE;
			}
			act("&+wYou raise $p &+wand bring it down upon the surface of the &+Cbell&+w.\n\n"
			    "&+wThe &+rmallet &+Rshatters &+wupon impact as it moves the heavy &+ycopper &+Cbell&+w.\n"
			    "&+wThe &+Cbell &+wsways and the clapper strikes the lip with a beautiful &+Wlucid note&+w.\n"
			    "&+wSomewhere in the distance, an answering note rings out in the castle, as if\n"
			    "&+wthe note awakened a long lost secret hidden deep below.\n\n"
			    "&+wIn the distance, a &+Ygreat beam of light &+wsurges downwards and &+Willuminates\n"
			    "&+wthe &+WHall of &+YHeroes&+w.\n",
			    FALSE,
			    ch,
			    obj,
			    0,
			    TO_CHAR);
			act("$n &+wraises $p &+wand brings it down upon the surface of the &+Cbell&+w.\n"
			    "&+wThe &+rmallet &+Rshatters &+wupon impact as it moves the heavy &+ycopper &+Cbell&+w.\n"
			    "&+wThe &+Cbell &+wsways and the clapper strikes the lip with a beautiful &+Wlucid note&+w.\n"
			    "&+wSomewhere in the distance, an answering note rings out in the castle, as if\n"
			    "&+wthe note awakened a long lost secret hidden deep below.\n\n"
			    "&+wIn the distance, a &+Ygreat beam of light &+wsurges downwards and &+Willuminates\n"
			    "&+wthe &+WHall of &+YHeroes&+w.\n",
			    FALSE,
			    ch,
			    obj,
			    0,
			    TO_ROOM);
			extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			REMOVE_BIT(swordcase->value[1], CONT_LOCKED);
			snprintf(buf, MAX_STRING_LENGTH, "&+WA brilliant &+Ybeam &+Wof light shines upon an &+Lancient &+wsword case &+Where.&n");
			set_long_description(swordcase, buf);
			snprintf(buf, MAX_STRING_LENGTH, "&+Wan &+Lancient &+wswordcase &+Wbathed in &+Ylight&n");
			set_short_description(swordcase, buf);
			return TRUE;
		}
	}
	return FALSE;
}
/*
int strahd_charm(P_char strahd, P_char charmie, int cmd, char *arg)
{
  struct affected_type af;
  P_char   vict, tmp_ch, next_vict_ch, next_tmp_ch, victim;
  int      InRoom, HasCharmies, highstr, currstr;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  Gbuf4[0] = 0;
  if ((charmie) && GET_MASTER(charmie) == strahd)
    switch (cmd)
    {
    case CMD_SCORE:
    case CMD_TELL:
    case CMD_SHOUT:
    case CMD_LOOK:
    case CMD_HELP:
    case CMD_WHO:
    case CMD_WEATHER:
    case CMD_SAVE:
    case CMD_QUIT:
    case CMD_TIME:
    case CMD_TOGGLE:
    case CMD_CHANNEL:
    case CMD_GCC:
    case CMD_COMMANDS:
    case CMD_ATTRIBUTES:
    case CMD_PETITION:
      break;
    default:
      send_to_char
        ("You can hardly take your focus off of your master, Strahd.\r\n",
         charmie);
      return (TRUE);
      break;
    }
  if (charmie)
    return (0);
  else
  {
    if (IS_FIGHTING(strahd))
    {
      for (tmp_ch = world[strahd->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
      {
        next_tmp_ch = tmp_ch->next_in_room;

        if (GET_MASTER(tmp_ch) == strahd && MIN_POS(tmp_ch, POS_STANDING + STAT_SLEEPING))
        {
          for (vict = world[strahd->in_room].people; vict; vict = next_vict_ch)
          {
            next_vict_ch = vict->next_in_room;

            if (IS_FIGHTING(vict) && (vict != strahd) && GET_MASTER(vict) != strahd)
            {
              snprintf(Gbuf4, MAX_STRING_LENGTH, "Strahd harshly orders %s, 'Protect me!'",
                      (IS_NPC(tmp_ch) ? tmp_ch->player.
                       short_descr : GET_NAME(tmp_ch)));
              act(Gbuf4, FALSE, strahd, 0, tmp_ch, TO_NOTVICT);
              snprintf(Gbuf4, MAX_STRING_LENGTH,
                      "Strahd harshly orders you, 'Protect me!'\r\n");
              send_to_char(Gbuf4, tmp_ch);
              snprintf(Gbuf4, MAX_STRING_LENGTH,
                      "%s looks blankly for a moment, before shoving Strahd aside and attacking you!",
                      (IS_NPC(tmp_ch) ? tmp_ch->player.
                       short_descr : GET_NAME(tmp_ch)),
                      (IS_NPC(vict) ? vict->player.
                       short_descr : GET_NAME(vict)));
              act(Gbuf4, FALSE, strahd, 0, 0, TO_NOTVICT);
              snprintf(Gbuf4, MAX_STRING_LENGTH,
                      "Your master's actions compel you to attack %s!\r\n",
                      (IS_NPC(vict) ? vict->player.
                       short_descr : GET_NAME(vict)));
              send_to_char(Gbuf4, tmp_ch);
              stop_fighting(vict);
              hit(tmp_ch, vict, tmp_ch->equipment[PRIMARY_WEAPON]);
              return (TRUE);
            }
          }
        }
      }

      for (tmp_ch = world[strahd->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
      {
         currstr = GET_C_STR(tmp_ch);
         next_tmp_ch = tmp_ch->next_in_room;

         if (IS_TRUSTED(tmp_ch) || IS_NPC(tmp_ch))
           continue;

         if(currstr > highstr)
         {
           victim = tmp_ch;
           highstr = currstr;
         }
      }

        if(!victim)
          return FALSE;

      if (!(NewSaves(victim, SAVING_FEAR, 10)))
      {
        act("&+rStrahd looks upon the muscular, mortal body of %s&+r, and remarks calmly, &+L'Were\r\n",FALSE, victim, 0, 0, TO_VICTROOM);
        act("&+LI a younger, more able body, such as yourself, perhaps I could return the power to\n", FALSE, strahd, 0, 0, TO_VICTROOM);
        act("&+Lmy family's name once more.  Come and fight by my side, and reap the rewards!", FALSE, strahd, 0, 0, TO_VICTROOM);
        act("&+rStrahd looks upon your muscular, mortal body and remarks calmly, &+L'Were\n", FALSE, strahd, 0, 0, TO_CHAR);
        act("&+LI a younger, more able body, such as yourself, perhaps I could return the power to\n", FALSE, strahd, 0, 0, TO_CHAR);
        act("&+Lmy family's name once more.  Come and fight by my side, and reap the rewards!", FALSE, strahd, 0, 0, TO_CHAR);
        stop_fighting(victim);
        if( IS_DESTROYING(victim) )
          stop_destroying(victim);
        stop_fighting(strahd);
        if( IS_DESTROYING(strahd) )
          stop_destroying(strahd);
        if (victim->following)
          stop_follower(victim);
        add_follower(victim, strahd);
        setup_pet(victim, strahd, 24, 0);
      }
      else
      {
        act("&+LStrahd's power nearly overwhelms your senses, but you manage to stave off\r\n" \
            "&+Lhis attack upon your mind.\r\n", FALSE, strahd, 0, victim, TO_CHAR);
        act("&+LStrahd's power nearly overwhelms %s&+L's senses, but $e manages to stave off\r\n" \
            "&+LStrahd's attack upon $s mind.\r\n", FALSE, strahd, 0, vict, TO_ROOM);
        return FALSE;
      }
    }
  else if (MIN_POS(strahd, POS_STANDING + STAT_NORMAL))
  {
    HasCharmies = 0;
    for (tmp_ch = world[strahd->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
    {
      next_tmp_ch = tmp_ch->next_in_room;
      if (GET_MASTER(tmp_ch) == strahd)
      {
        HasCharmies = 1;
        if (number(0, 1) == 0)
        {
          switch (number(1, 3))
          {
          case 1:
           act("&+rStrahd sighs heavily and says &+L'Time has ravaged this land...'", TRUE, 0, 0, 0, TO_ROOM);
          case 2:
            act("&+rStrahd eyes you with an adoring look.", TRUE, tmp_ch, 0, 0, TO_CHAR);
            act("&+rStrahd eyes %s &+rwith an adoring look.", TRUE, tmp_ch, 0, 0, TO_VICTROOM);
           break;
          case 3:
           break;
          default:
           break;
          }
        }
      }
    }
  }
 }
 return (FALSE);
} */

int shimmer_shortsword(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char               vict;
	struct affected_type af;
	int                  curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	if ((cmd == CMD_REMOVE) && arg)
	{
		if (isname(arg, obj->name) || isname(arg, "all"))
		{
			if (affected_by_spell(ch, SPELL_BLUR))
			{
				affect_from_char(ch, SPELL_BLUR);
			}
			if (affected_by_spell(ch, SPELL_CURSE))
			{
				affect_from_char(ch, SPELL_CURSE);
			}
		}
		return FALSE;
	}

	if (!OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	curr_time = time(NULL);

	if (!IS_ROOM(ch->in_room, ROOM_NO_MAGIC))
	{
		// Every 10 sec?!
		if (obj->timer[0] + 10 <= curr_time)
		{
			obj->timer[0] = curr_time;

			if (!affected_by_spell(ch, SPELL_CURSE))
			{
				act("&n$n's $q &+Rflares &+Bwith energy for a moment.&n", TRUE, ch, obj, vict, TO_ROOM);
				act("&nYour $q &+Rflares &+Bwith energy for a moment.&n", TRUE, ch, obj, vict, TO_CHAR);

				bzero(&af, sizeof(af));

				af.type     = SPELL_CURSE;
				af.duration = 100;
				af.modifier = -1;
				af.location = APPLY_HITROLL;
				affect_to_char(ch, &af);
				af.modifier = 10;
				af.location = APPLY_CURSE;
				affect_to_char(ch, &af);

				act("&+r$n briefly reveals a red aura!", FALSE, ch, 0, 0, TO_ROOM);
				act("&+rYou feel very uncomfortable.", FALSE, ch, 0, 0, TO_CHAR);
				return FALSE;
			}
			if (!IS_AFFECTED3(ch, AFF3_BLUR))
			{
				act("&n$n's $q &+Ysparkles &+Bwith energy for a moment.&n", TRUE, ch, obj, vict, TO_ROOM);
				act("&nYour $q &+Ysparkles &+Bwith energy for a moment.&n", TRUE, ch, obj, vict, TO_CHAR);
				spell_blur(70, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				return FALSE;
			}
		}
	}
	return FALSE;
}
