/*
 * ***************************************************************************
 * *  File: actwiz.c                                           Part of Duris *
 * *  Usage: wizcommands
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "assocs.h"
#include "shop.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "prototypes.h"
#include "interp.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "weather.h"
#include "mm.h"
#include "specs.prototypes.h"
#include "objmisc.h"
#include "damage.h"
#include "sql.h"
#include "vnum.obj.h"
#include "ships.h"
#include "listen.h"
#include "map.h"
#include "epic.h"
#include "trophy.h"
#include "ships.h"

/*
 * external variables
 */

extern Skill skills[];
extern char *spells[];
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_town towns;
extern P_room world;
extern byte create_locked;
extern byte locked;
extern int top_of_helpt;
extern FILE *help_fl;
extern const char *weapons[];
extern const char *justice_flags[];
extern const char *justice_status[];
extern const char *crime_list[];
extern const flagDef action_bits[];
extern const flagDef action2_bits[];
extern const flagDef aggro_bits[];
extern const flagDef aggro2_bits[];
extern const flagDef aggro3_bits[];
extern const flagDef anti_bits[];
extern const flagDef anti2_bits[];
extern const char *apply_types[];
extern const struct class_names class_names_table[];
extern const char *command[];
extern const char *connected_types[];
extern const char *dirs[];
extern const char *drinks[];
extern const char *equipment_types[];
extern const char *event_names[];
extern const char *exit_bits[];
extern const flagDef extra_bits[];
extern const flagDef extra2_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern const char *item_types[];
extern const char *language_names[];
extern const char *missileweapons[];
extern const char *shot_types[];
extern const char *player_bits[];
extern const char *player2_bits[];
extern const char *player3_bits[];
extern const char *player_prompt[];
extern const char *position_types[];
extern struct race_names race_names_table[];
extern const flagDef room_bits[];
extern const char *sector_types[];
extern const flagDef wear_bits[];
extern const char *zone_bits[];
extern const char *justice_obj_status[];
extern const char *shutdown_message;
extern const char *item_material[];
extern const char *kingdom_type_list[];
extern const char *resource_list[];
extern const int shot_damage[];
extern const struct stat_data stat_factor[];
extern const char *size_types[];
extern const char *item_size_types[];
extern int bounce_null_sites;
extern int number_of_shops;
extern int invitemode;
extern int pulse;
extern int shutdownflag, _reboot, _autoboot, _copyover, _pwipe;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_world;
extern int top_of_zone_table;
extern int used_descs;
extern struct agi_app_type agi_app[];
extern struct ban_t *ban_list;
extern struct wizban_t *wizconnect;
extern struct bonus_stat bonus_stats[];
extern struct bonus_stat general_bonus_stats;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct max_stat max_stats[];
extern struct shop_data *shop_index;
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern struct time_info_data time_info;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern int min_stats_for_class[][8];
extern char *specdata[][MAX_SPEC];
extern struct link_description link_types[];
void     sprintbitde(ulong, const flagDef[], char *);
extern flagDef weapon_types[];
extern flagDef missile_types[];
extern float combat_by_class[][2];
extern float combat_by_race[][2];
extern const int new_exp_table[];
extern const char *get_event_name(P_event);
extern const char *get_function_name(void *);
extern const char *spldam_types[];
extern const char *craftsmanship_names[];
extern int number_of_quests;
extern struct quest_data quest_index[];
extern const struct hold_data hold_index[];
extern float spell_pulse_data[LAST_RACE + 1];

void apply_zone_modifier(P_char ch);
static P_char load_locker_char(P_char ch, char *locker_name, int bValidateAccess);
void shopping_stat( P_char ch, P_char keeper, char *arg, int cmd );
bool is_quested_item( P_obj obj );
void do_setship( P_char ch, char *arg );

/*
 * Macros
 */

#define ARRAY_SIZE(A)           (sizeof(A)/sizeof(*(A)))

#define GET_VICTIM_ROOM(v, c, a)  (v) = get_char_room_vis((c), (a))

/*
 * call with an object, recursively calls itself to build a string in the
 * format "in <container> in <container>... carried by <name>.  Only limit
 * to nesting depth, is MAX_STRING_LENGTH for all output.  Currently only
 * used by the new stat routine, probably add it to 'where' later, and the
 * god's version of locate spell.  JAB
 */

char    *where_obj(P_obj w_obj, int flag)
{
  if(!flag)
    GS_buf1[0] = 0;

  if(!w_obj)
  {
    strcat(GS_buf1, "&+RLost in the bit bucket!&n");
    return (GS_buf1);
  }
  if(OBJ_ROOM(w_obj))
  {
    sprintf(GS_buf1 + strlen(GS_buf1), "in [%4d:%6d] &n%s",
            ROOM_ZONE_NUMBER(w_obj->loc.room), world[w_obj->loc.room].number, world[w_obj->loc.room].name);
    return (GS_buf1);
  }
  if(OBJ_CARRIED(w_obj))
  {
    sprintf(GS_buf1 + strlen(GS_buf1),
            "in [%4d:%6d] &+Ycarried by &n%s&n",
            ((w_obj->loc.carrying->in_room != NOWHERE) ? ROOM_ZONE_NUMBER(w_obj->loc.carrying->in_room) : -1),            
            ((w_obj->loc.carrying->in_room != NOWHERE) ? world[w_obj->loc.carrying->in_room].number : -1),
            GET_NAME(w_obj->loc.carrying));
    return (GS_buf1);
  }
  if(OBJ_WORN(w_obj))
  {
    sprintf(GS_buf1 + strlen(GS_buf1),
            "in [%4d:%6d] &+Yequipped by &n%s&n",
            ((w_obj->loc.carrying->in_room != NOWHERE) ? ROOM_ZONE_NUMBER(w_obj->loc.carrying->in_room) : -1),            
            ((w_obj->loc.carrying->in_room != NOWHERE) ? world[w_obj->loc.carrying->in_room].number : -1),
            GET_NAME(w_obj->loc.wearing));    
    return (GS_buf1);
  }
  if(OBJ_INSIDE(w_obj))
  {
    sprintf(GS_buf1 + strlen(GS_buf1), "&+Yinside &n%s&+Y, ",
            w_obj->loc.inside->short_description);
    where_obj(w_obj->loc.inside, TRUE);
    return GS_buf1;
  }
  if(GS_buf1[0] == 0)
    strcat(GS_buf1, "&+RLost in the bit bucket! #2&N");

  return (GS_buf1);
}

/*
 * make those huge numbers a little more readable, formats a long
 * 000000000 as 000, 000, 000 and returns the string.  Only used in
 * do_stat() now.  JAB
 */

char    *comma_string(long num)
{
  static char buf1[50] = { 0 }, buf2[50] =
  {
  0};
  int      bp1, bp2, len, j;

  sprintf(buf1, "%ld", num);

  len = strlen(buf1);
  bp1 = 0;
  bp2 = 0;

  if(buf1[0] == '-')
  {
    *(buf2 + bp2++) = *(buf1 + bp1++);
    len--;
  }
  if(len < 4)
    return (buf1);              /*
                                 * doesn't need commas
                                 */

  if(len % 3)
  {
    for (j = len % 3; j > 0; j--)
    {
      *(buf2 + bp2++) = *(buf1 + bp1++);
      len--;
    }
    *(buf2 + bp2++) = ',';
  }
  while (len)
  {
    for (j = 0; j < 3; j++)
    {
      *(buf2 + bp2++) = *(buf1 + bp1++);
      len--;
    }
    if(len)
      *(buf2 + bp2++) = ',';
  }

  *(buf2 + bp2) = '\0';

  return (buf2);
}

void sa_byteCopy(P_char ch, unsigned long offset, int value)
{
  byte     new_value = (byte) value;

  bcopy((char *) &new_value, (char *) ch + offset, sizeof(byte));
}

/*
 ** Used by do_setattr to perform short int copying
 */
void sa_shortCopy(P_char ch, unsigned long offset, int value)
{
  sh_int   new_value = (sh_int) value;

  bcopy((char *) &new_value, (char *) ch + offset, sizeof(sh_int));
}

/*
 ** Used by do_setattr to do integer copying
 */
void sa_intCopy(P_char ch, unsigned long offset, int value)
{
  bcopy((char *) &value, (char *) ch + offset, sizeof(int));
}

/*
 ** Used by do_setattr to do set the year of player to "value"
 */
void sa_ageCopy(P_char ch, unsigned long offset, int value)
{
  long     secs;
  time_t   curr_time = time(NULL);

  secs = (value - 17) * SECS_PER_MUD_YEAR;
  ch->player.time.birth = curr_time - secs;
}

void do_reload_help(P_char ch, char *arg, int cmd)
{
  send_to_char("This does not work.\n", ch);
  send_to_char("It is not really neccessary either, but\n", ch);
  send_to_char("if you want to try to make it work, see\n", ch);
  send_to_char("actwiz.c, the function is do_reload_help.\n", ch);
  send_to_char("I tried for several hours, and now I am done trying.\n\n", ch);
/*
  To make this work, SOMEHOW, you have to reference helpfile.c
  and make it compile.  Anytime I try, it gives me stupid errors.
  I can't make it work.
  Below is the code you need, if you can ever get it referenced.

  send_to_char("Clearing old help data..\n", ch);

  help_index.clear();

  send_to_char("Loading new help data..\n", ch);

  if(help_index.reload()){
    send_to_char("Help data rebuilt.\n", ch);
  }else{
    logit(LOG_FILE, "   Could not open help file.");
    fprintf(stderr, "ERROR! Could not open help file! Exiting.");
    raise(SIGSEGV);
  }
*/
}


void do_reboot_restore(P_char ch, P_char victim)
{

  if(!IS_TRUSTED(ch))
    return;

  poison_common_remove(victim);

  if(affected_by_spell(victim, SPELL_CURSE))
    affect_from_char(victim, SPELL_CURSE);

  if(affected_by_spell(victim, SPELL_MALISON))
    affect_from_char(victim, SPELL_MALISON);

  if(affected_by_spell(victim, SPELL_WITHER))
    affect_from_char(victim, SPELL_WITHER);

  if(affected_by_spell(victim, SPELL_BLOODTOSTONE))
    affect_from_char(victim, SPELL_BLOODTOSTONE);
  
  if(affected_by_spell(victim, SPELL_SHREWTAMENESS))
    affect_from_char(victim, SPELL_SHREWTAMENESS);

  if(affected_by_spell(victim, SPELL_MOUSESTRENGTH))
    affect_from_char(victim, SPELL_MOUSESTRENGTH);

  if(affected_by_spell(victim, SPELL_MOLEVISION))
    affect_from_char(victim, SPELL_MOLEVISION);

  if(affected_by_spell(victim, SPELL_SNAILSPEED))
    affect_from_char(victim, SPELL_SNAILSPEED);

  if(affected_by_spell(victim, SPELL_FEEBLEMIND))
    affect_from_char(victim, SPELL_FEEBLEMIND);
  
  if(affected_by_spell(victim, SPELL_SLOW))
    affect_from_char(victim, SPELL_SLOW);

  if(IS_AFFECTED(victim, AFF_BLIND))
  {
    affect_from_char(victim, SPELL_BLINDNESS);
    REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
  }

  if(IS_AFFECTED4(victim, AFF4_CARRY_PLAGUE))
    REMOVE_BIT(victim->specials.affected_by4, AFF4_CARRY_PLAGUE);

  if(affected_by_spell(victim, SPELL_DISEASE) ||
     affected_by_spell(victim, SPELL_PLAGUE) ||
     affected_by_spell(victim, SPELL_BMANTLE))
  {
    affect_from_char(victim, SPELL_DISEASE);
    affect_from_char(victim, SPELL_PLAGUE);
    affect_from_char(victim, SPELL_BMANTLE);
  }
   
  if(affected_by_spell(victim, TAG_ARMLOCK))
    affect_from_char(victim, TAG_ARMLOCK);
        
  if(affected_by_spell(victim, TAG_LEGLOCK))
    affect_from_char(victim, TAG_LEGLOCK);
      
  if(affected_by_spell(victim, SPELL_ENERGY_DRAIN))
      affect_from_char(victim, SPELL_ENERGY_DRAIN);
      
  if(affected_by_spell(victim, SPELL_SLEEP))
    affect_from_char(victim, SPELL_SLEEP);
    
  if(affected_by_spell(victim, SONG_SLEEP))
    affect_from_char(victim, SONG_SLEEP);
    
  if(affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT))
    affect_from_char(victim, SPELL_RAY_OF_ENFEEBLEMENT);

  send_to_char("&+WAll your maladies are washed away...\r\n", victim);
}


void test_load_all_chars(P_char ch)
{
#ifdef TEST_MUD
  FILE *flist;
  int i;
  P_char locker;
  char filename[MAX_STRING_LENGTH];
  char name[MAX_STRING_LENGTH];
  const char *alphabet[] =
  {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};
  
  for (i = 0; i < 26; i++)
  {
    sprintf(filename, "/bin/ls Players/%s > %s", alphabet[i], "temp_letterfile");
    system(filename);
    flist = fopen("temp_letterfile", "r");
    if(!flist)
      return;

    while (fscanf(flist, " %s \n", name) != EOF)
    {
      if(!isname(name, GET_NAME(ch)))// &&
	  //!strstr(name, ".locker"))
	do_read_player(ch, name, 0);
      //else if(strstr(name, ".locker"))
      //{
	//locker = load_locker_char(ch, name, 0);
      //}	
    }
    fclose(flist);
  }
#else
  send_to_char("This command is not to be used in live mud enviornment.", ch);
#endif
}

/* Load a player manually from save files */
void do_read_player(P_char ch, char *arg, int cmd)
{
  P_char   vict = NULL;
  int      tmp;

  if(!*arg)
  {
    send_to_char("Syntax: load char <name> or load <m|c|o|i> <vnum>\n", ch);
    return;
  }
#if 0
  send_to_char("Temp disabled, so I can test a mem leak potential.\n", ch);
  return;
#endif

  vict = (P_char) mm_get(dead_mob_pool);
  clear_char(vict);
#if 0
  dead_pconly_pool = mm_create("PC_ONLY", sizeof(struct pc_only_data),
                               offsetof(struct pc_only_data, switched),
                               mm_find_best_chunk(sizeof(struct pc_only_data),
                                                  10, 25));
#endif
  vict->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
  vict->only.pc->aggressive = -1;
  vict->desc = NULL;
#if 0
  if((tmp = restorePasswdOnly(vict, arg)) < 0)
  {
    send_to_char
      ("&=RlDange�Will Robinson!0Bad pfile!!&n (Or maybe you just fucked up typing the name.)\n",
       ch);
    return;
  }
#endif
  if((tmp = restoreCharOnly(vict, arg)) < 0)
  {
    send_to_char("&=RlDanger Will Robinson! Bad pfile!!&n\n", ch);
    return;
  }
  tmp = restoreItemsOnly(vict, 100);

  /* insert in list */
  vict->next = character_list;
  character_list = vict;

  setCharPhysTypeInfo(vict);

  /* saving info for teleport return command */
  vict->specials.was_in_room = vict->in_room;

  char_to_room(vict, ch->in_room, -2);

  act("$N &+Bappears before you, greatly humbled by your power.", FALSE, ch,
      0, vict, TO_CHAR);
  act("$N &+Bappears before $n, greatly humbled by $s power.", TRUE, ch, 0,
      vict, TO_NOTVICT);

  logit(LOG_WIZ, "%s loaded %s's char into the game [%d]",
        GET_NAME(ch), GET_NAME(vict), world[ch->in_room].number);
  sql_log(ch, WIZLOG, "Loaded char %s", GET_NAME(vict));
}

void do_release(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  P_desc   d;
  int      sdesc;

  if(!IS_TRUSTED(ch))
    return;

  one_argument(argument, arg);
  sdesc = atoi(arg);
  if((sdesc == 0) && arg)
  {
    P_char t_ch = NULL;

    for (d = descriptor_list; d; d = d->next)
    {
      if(d->character)
        t_ch = d->character;
      else
        continue;
      // hide higher level people
      if(GET_LEVEL(t_ch) > GET_LEVEL(ch))
        continue;
      // if this isn't the proper user, keep looking
      if((!t_ch->player.name || !isname(t_ch->player.name, arg)))
        continue;
      sdesc = d->descriptor;
    }
  }
  if(!sdesc)
  {
    send_to_char("Illegal descriptor number or name not found.\n", ch);
    send_to_char("Usage: release {<#> | <name>}\n", ch);
    return;
  }
  for (d = descriptor_list; d; d = d->next)
  {
    if(d->descriptor == sdesc)
    {
      if(!d->character || CAN_SEE(ch, d->character))
      {
        close_socket(d);
        sprintf(buf, "Closing socket to descriptor #%d\n", sdesc);
        send_to_char(buf, ch);
        if(GET_LEVEL(ch) < 62)
        {
          wizlog(GET_LEVEL(ch), "%s just released socket %d.", GET_NAME(ch),
                 sdesc);
          logit(LOG_WIZ, "%s just released socket %d.", GET_NAME(ch), sdesc);
          sql_log(ch, WIZLOG, "Released socket %d.", sdesc);
        }
        return;
      }
    }
  }
  send_to_char("Descriptor not found!\n", ch);
}

void do_emote(P_char ch, char *argument, int cmd)
{
  int      i;
  P_char   k;
  static char buf[MAX_STRING_LENGTH];

/*
   if(IS_SET(ch->specials.act, PLR_NOEMOTE) && !IS_NPC(ch)) {
   send_to_char("You have NoEmote on.\n", ch);
   return;
   }
 */

  if(!(ch))
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_MORPH(ch))
  {
    send_to_char("So much for that idea!\n", ch);
    return;
  }
  for (i = 0; *(argument + i) == ' '; i++) ;

  if(IS_SET(world[ch->in_room].room_flags, UNDERWATER) &&
      !IS_TRUSTED(ch) && !IS_NPC(GET_PLYR(ch)))
  {
    send_to_char("You cannot emote while swimming around in water...", ch);
    return;
  }
  if(IS_AFFECTED2(ch, AFF2_SILENCED))
    send_to_char("You seem unable to make your emotions known.\n", ch);
  else if(IS_AFFECTED(ch, AFF_WRAITHFORM))
    send_to_char("You cannot speak in this form.\n", ch);
  else if(is_silent(ch, FALSE))
    send_to_char("For some reason, that doesn't seem possible here.\n", ch);
  else if(!*(argument + i))
    send_to_char("Yes... But what?\n", ch);
  else
  {
    /*
     * sprintf(buf, "$n %s", argument + i);
     */
    for (k = world[ch->in_room].people; k; k = k->next_in_room)
      if(AWAKE(k))
      {
        sprintf(buf, "$n %s", language_CRYPT(ch, k, argument + i));
        act(buf, FALSE, ch, 0, k, TO_VICT | ACT_SILENCEABLE);
      }
    listen_broadcast(ch, (const char*)buf, LISTEN_EMOTE);

    if(IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(GET_PLYR(ch)))
    {
      sprintf(buf, "$N %s", argument + i);
      act(buf, FALSE, ch, 0, ch, TO_CHAR);
    }
    else
      send_to_char("Ok.\n", ch);
    
    if(get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s emotes '%s'", GET_NAME(ch), argument + i);
  }
}

void do_echo(P_char ch, char *argument, int cmd)
{
  P_desc d;
  int      i;
  static char buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  for (i = 0; *(argument + i) == ' '; i++) ;

  if(!*(argument + i))
    send_to_char("That must be a mistake...\n", ch);
  else
  {
    sprintf(buf, "%s\n", argument + i);
    send_to_room(buf, ch->in_room);
    
    for (d = descriptor_list; d; d = d->next)
    {
      if(d->connected == CON_PLYNG && 
	  ch->in_room == d->character->in_room)
      {
        write_to_pc_log(d->character, buf, LOG_PRIVATE);
      }
    }
    if(get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s echo's '%s'", GET_NAME(ch), argument + i);
  }
}

// This function toggles a player's newbie status
void do_newbie(P_char ch, char *argument, int cmd)
{
  P_char victim;
  char buf[MAX_INPUT_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf) {
    send_to_char("Who's newbie status do you wish to toggle?\n", ch);
    return;
  }

  if(!(victim = get_char_vis(ch, buf))) {
    send_to_char("No-one by that name around.\n", ch);
    return;
  }

  if(IS_TRUSTED(victim)) {
    send_to_char("Haha. Very funny.\n", ch);
    return;
  }

  if(GET_LEVEL(victim) > 49) {
    send_to_char("Aren't they a little high level to be considered a newbie?\n", ch);
    return;
  }

  PLR2_TOG_CHK(victim, PLR2_NEWBIE);

  if(IS_SET(PLR2_FLAGS(victim), PLR2_NEWBIE)) {
    send_to_char("You turned on their newbie status.\n", ch);
      SET_BIT(victim->specials.act2, PLR2_NCHAT);

  } else {
    send_to_char("You turned off their newbie status.\n", ch);
      REMOVE_BIT(victim->specials.act2, PLR2_NCHAT);
  }

  do_save_silent(victim, 1);

  logit(LOG_WIZ, "%s toggled %s's newbie status.", ch->player.name, victim->player.name);

}

// This function toggles a player's newbie helper status
void do_make_guide(P_char ch, char *argument, int cmd)
{
  P_char victim;
  char buf[MAX_INPUT_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf) {
    send_to_char("Who's newbie helper status do you wish to toggle?\n", ch);
    return;
  }

  if(!(victim = get_char_vis(ch, buf))) {
    send_to_char("No-one by that name around.\n", ch);
    return;
  }

  PLR2_TOG_CHK(victim, PLR2_NEWBIE_GUIDE);

  if(IS_SET(PLR2_FLAGS(victim), PLR2_NEWBIE_GUIDE)) {
    send_to_char("You made them a newbie helper.\n", ch);
    send_to_char("You are now an official &+GGuide&N!\n", victim);
      SET_BIT(victim->specials.act2, PLR2_NCHAT);

  } else {
    send_to_char("You turned off their newbie helper status.\n", ch);
    send_to_char("You are no longer an official &+GGuide&N.\n", victim);
      REMOVE_BIT(victim->specials.act2, PLR2_NCHAT);

  }

  do_save_silent(victim, 1);

  logit(LOG_WIZ, "%s toggled %s's newbie helper status.", ch->player.name, victim->player.name);

}

/*
 ** This function now allows a player to transfer anyone
 ** at or below his level
 */
void do_trans(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  P_char   victim;
  char     buf[MAX_INPUT_LENGTH];
  int      target, old_room;
  int      level;

  if(IS_NPC(ch))
    return;


  one_argument(argument, buf);

  if(!*buf)
  {
    send_to_char("Who do you wish to transfer?\n", ch);
    return;
  }
  if(str_cmp("all", buf))
  {

    if(!(victim = get_char_vis(ch, buf)))
    {
      send_to_char("No-one by that name around.\n", ch);
      return;
    }
    level = GET_LEVEL(ch);
    if((GET_LEVEL(victim) > level) && IS_TRUSTED(victim))
    {
      send_to_char("You cannot transfer someone higher level than you\n", ch);
      return;
    }
    if(!can_enter_room(victim, ch->in_room, FALSE) && (GET_LEVEL(ch) < 59))
    {
      send_to_char("That person can't come here.\n", ch);
      return;
    }
    act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
    target = ch->in_room;
    old_room = victim->in_room;
    act("$n demands your presence NOW!", FALSE, ch, 0, victim, TO_VICT);
    char_from_room(victim);
    room_light(old_room, REAL);
    char_to_room(victim, target, -1);
    char_light(victim);
    room_light(victim->in_room, REAL);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("Ok.\n", ch);

    logit(LOG_WIZ, "%s transferred %s from %d to %d", ch->player.name,
          victim->player.name, world[old_room].number,
          world[victim->in_room].number);
    sql_log(ch, WIZLOG, "Transferred %s from %d to %d", victim->player.name, world[old_room].number, world[victim->in_room].number);
  }
  else
  {                             /*
                                 * Trans All
                                 */

    if(level)
    {
      send_to_char("Sorry, 'trans all' is a level 58 command.\n", ch);
      return;
    }
    for (i = descriptor_list; i; i = i->next)
    {

      if(i->character != ch && !i->connected &&
          (GET_LEVEL(i->character) <= level))
      {

        victim = i->character;
        act("$n disappears in a mushroom cloud.",
            FALSE, victim, 0, 0, TO_ROOM);
        target = ch->in_room;
        old_room = victim->in_room;
        char_from_room(victim);
        room_light(old_room, REAL);
        act("$n demands your presence NOW!", FALSE, ch, 0, victim, TO_VICT);
        char_to_room(victim, target, -1);
        char_light(victim);
        room_light(victim->in_room, REAL);
        act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      }
    }

    send_to_char("Ok.\n", ch);

    logit(LOG_WIZ, "%s transferred all to %d", ch->player.name,
          world[ch->in_room].number);
    sql_log(ch, WIZLOG, "Transferred all to", world[ch->in_room].number);
  }
}

void do_at(P_char ch, char *argument, int cmd)
{
  char     loc_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int      loc_nr, location, original_loc, zc, original_zc;
  P_char   target_mob, next;
  P_obj    target_obj;

  if(IS_NPC(ch))
    return;

  half_chop(argument, loc_str, buf);
  if(!*loc_str)
  {
    send_to_char("You must supply a room number or a name.\n", ch);
    return;
  }
  if(isdigit(*loc_str))
  {
    loc_nr = atoi(loc_str);
    for (location = 0; location <= top_of_world; location++)
      if(world[location].number == loc_nr)
      {
        zc = 0;
        break;
      }
      else if(location == top_of_world)
      {
        send_to_char("No room exists with that number.\n", ch);
        return;
      }
  }
  else if((target_mob = get_char_vis(ch, loc_str)))
  {
    if(target_mob->in_room != NOWHERE)
    {
      location = target_mob->in_room;
      zc = target_mob->specials.z_cord;
    }
    else
    {
      logit(LOG_DEBUG, "%s in NOWHERE!!",
            (IS_PC(target_mob) ?
             GET_NAME(target_mob) : target_mob->player.short_descr));
      send_to_char("That person is in NOWHERE!!", ch);
      return;
    }
  }
  else if((target_obj = get_obj_vis(ch, loc_str)))
  {
    if(OBJ_ROOM(target_obj))
    {
      location = target_obj->loc.room;
      zc = target_obj->z_cord;
    }
    else
    {
      send_to_char("The object is not available.\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("No such creature or object around.\n", ch);
    return;
  }

  if(IS_SET(world[location].room_flags, PRIVATE) && (GET_LEVEL(ch) < OVERLORD))
  {
    send_to_char("It is far too private there.\n", ch);
    return;
  }

  original_zc = ch->specials.z_cord;
  ch->specials.z_cord = zc;

  if(!can_enter_room(ch, location, TRUE))
  {
    ch->specials.z_cord = original_zc;
    return;
  }

  /*
   * a location has been found.
   */

  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location, -2);       /*
                                         * avoid triggering aggros
                                         */
  command_interpreter(ch, buf);

  /*
   * check if the guy's still there
   */
  for (target_mob = world[location].people; target_mob; target_mob = next)
  {
    next = target_mob->next_in_room;

    if(ch == target_mob)
    {
      char_from_room(ch);
      ch->specials.z_cord = original_zc;
      char_to_room(ch, original_loc, -2);
      return;
    }
  }
}

void do_goto(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH], output[MAX_STRING_LENGTH];
  int      location = NOWHERE, old_room, i, bits;
  P_char   target_mob = NULL, pers;
  P_obj    target_obj = NULL;

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);
  if(!*buf)
  {
    send_to_char("You must supply a room number or a name.\n", ch);
    return;
  }
  if(is_number(buf))
  {
    location = real_room(atoi(buf));
    if((location == NOWHERE) || (location > top_of_world))
    {
      send_to_char("No room exists with that number.\n", ch);
      return;
    }
  }
  else
  {
    bits = generic_find(buf, (FIND_CHAR_WORLD | FIND_OBJ_WORLD), ch,
                        &target_mob, &target_obj);
    if(!bits)
    {
      send_to_char("Nothing by that name.\n", ch);
      return;
    }
    if(target_mob)
    {
      location = target_mob->in_room;
    }
    else
    {
      while (location == NOWHERE)
      {
        if(OBJ_ROOM(target_obj))
          location = target_obj->loc.room;
        else if(OBJ_CARRIED(target_obj))
          location = target_obj->loc.carrying->in_room;
        else if(OBJ_WORN(target_obj))
          location = target_obj->loc.wearing->in_room;
        else if(OBJ_INSIDE(target_obj))
          target_obj = target_obj->loc.inside;
        else
        {
          send_to_char
            ("that object is BUGGED! can't find a location for it.\n", ch);
          return;
        }
      }
    }
  }

  if((location == NOWHERE) || (location > top_of_world))
  {
    send_to_char("No such creature or object around.\n", ch);
    return;
  }
  /* a location has been found.  */
  if(IS_SET(world[location].room_flags, PRIVATE) && (GET_LEVEL(ch) < MAXLVL))
  {
    if(GET_LEVEL(ch) < MAXLVL)
    {
      send_to_char("That room is private.\n", ch);
      return;
    }
  }
  if(!can_enter_room(ch, location, TRUE))
    return;

  if(ch->only.pc->poofOutSound)
  {
    sprintf(output, "!!SOUND(%s F=1 P=10)\n", ch->only.pc->poofOutSound);

    for (pers = world[ch->in_room].people; pers; pers = pers->next_in_room)
    {
      if((pers == ch) || CAN_SEE(pers, ch))
        sound_to_char(output, pers);
    }
  }
  if(ch->only.pc->poofOut == NULL)
  {
    strcpy(output, "$n disappears in a puff of smoke.");
  }
  else
  {
    if(!strstr(ch->only.pc->poofOut, "%n"))
    {
      strcpy(output, "$n ");
      strcat(output, ch->only.pc->poofOut);
    }
    else
    {
      strcpy(output, ch->only.pc->poofOut);

      for (i = 0; i < strlen(output); i++)
        if((*(output + i) == '%') && (*(output + i + 1) == 'n'))
          *(output + i) = '$';
    }
  }

  act(output, TRUE, ch, 0, 0, TO_ROOM);

  old_room = ch->in_room;
  char_from_room(ch);
  room_light(old_room, REAL);
  if(target_mob)
    ch->specials.z_cord = target_mob->specials.z_cord;
  char_to_room(ch, location, -1);

  if(ch->only.pc->poofInSound)
  {
    sprintf(output, "!!SOUND(%s F=1 P=30)\n", ch->only.pc->poofInSound);

    for (pers = world[ch->in_room].people; pers; pers = pers->next_in_room)
    {
      if((pers == ch) || CAN_SEE(pers, ch))
        sound_to_char(output, pers);
    }
  }
  if(ch->only.pc->poofIn == NULL)
  {
    strcpy(output, "$n appears with an ear-splitting bang.");
  }
  else
  {
    if(!strstr(ch->only.pc->poofIn, "%n"))
    {
      strcpy(output, "$n ");
      strcat(output, ch->only.pc->poofIn);
    }
    else
    {
      strcpy(output, ch->only.pc->poofIn);

      for (i = 0; i < strlen(output); i++)
        if((*(output + i) == '%') && (*(output + i + 1) == 'n'))
          *(output + i) = '$';
    }
  }

  act(output, TRUE, ch, 0, 0, TO_ROOM);
}

void do_affectpurge(P_char ch, char *argument, int cmd)
{
  P_char   victim;
  char     arg1[MAX_STRING_LENGTH], rest[MAX_STRING_LENGTH];
  int      spell = 0, qend;
  struct affected_type *af, *next_af;

  if(!ch || !IS_TRUSTED(ch))
  {
    return;
  }

  half_chop(argument, arg1, rest);
  if(!*arg1)
  {
    send_to_char("Who do you wish to unaffect?\n", ch);
    return;
  }

  victim = get_char(arg1);

  if(!victim)
  {
    send_to_char("Usage: affectpurge <name> <all | '<spell>'>\n", ch);
    return;
  }

  if(GET_LEVEL(victim) > GET_LEVEL(ch))
  {
    send_to_char
      ("You may not unaffect entities more superior than yourself.\n", ch);
    return;
  }

  if(!*rest)
  {
    send_to_char("Usage: affectpurge <name> <all | '<spell>'>\n", ch);
    return;
  }

  if(!strcmp(rest, "all"))
  {
    spell = -1;
    logit(LOG_WIZ, "%s purged affect ALL from %s", GET_NAME(ch),
          GET_NAME(victim));
    sql_log(ch, WIZLOG, "Purged all affects from %s", GET_NAME(victim));
  }
  else
  {
    if(*rest != '\'')
    {
      send_to_char("You must use single quotes around the affect\n", ch);
      return;
    }
    for (qend = 1; *(rest + qend) && (*(rest + qend) != '\''); qend++)
      *(rest + qend) = LOWER(*(rest + qend));

    if(*(rest + qend) != '\'')
    {
      send_to_char("You must use single quotes around the affect\n", ch);
      return;
    }
    spell =
      old_search_block(rest, 1, ((uint) (MAX(0, qend - 1))),
                       (const char **) spells, 0);
    if(spell != -1)
      spell--;
    if(spell == -1)
    {
      send_to_char("There is no such affect.\n", ch);
      return;
    }
    logit(LOG_WIZ, "%s purged affect %s from %s", GET_NAME(ch), rest,
          GET_NAME(victim));
    sql_log(ch, WIZLOG, "Purged affect %s from %s", rest, GET_NAME(victim));
  }
  if(victim && spell != 0)
  {
    act("&+W$n&+W waves $s mighty hand over your body...", FALSE, ch, 0,
        victim, TO_VICT);
    act("&+WYou waves your mighty hand over $N&+W's body...", FALSE, ch, 0,
        victim, TO_CHAR);
    act("&+W$n&+W waves $s mighty hand over $N&+W's body...", FALSE, ch, 0,
        victim, TO_NOTVICT);
    for (af = victim->affected; af; af = next_af)
    {
      next_af = af->next;
      if(spell == -1 || spell == af->type)
      {
        affect_remove(victim, af);
        send_to_char("Affect removed.\n", ch);
      }
    }
    update_pos(victim);
  }
}

// No args: Lists the deathobjects vnum along with short desc.
// With arg add: Adds to list of deathobjects and sets object's proc to kill any mortal.
// With arg remove: Removes argument from list of deathobjects and sets proc to NULL.
// Question: Are these procs read/set at boot and after regular procs set?
//   Otherwise, they won't persist through boots and would make this lame.
void do_deathobj(P_char ch, char *argument, int cmd)
{
  char     vnum[15], out[MAX_STRING_LENGTH], Gbuf[MAX_STRING_LENGTH];
  char     arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  FILE    *f;
  int      count = 0, inserted = FALSE, removed = FALSE, newvnum, rn;
  P_obj    obj;

  // Disabled for unknown reasons. - Lohrr 8/8/14
  send_to_char( "do_deathobj: Command disabled.\n\r", ch );
  return;

  *out = '\0';
  *arg1 = '\0';
  *arg2 = '\0';

  f = fopen("Players/deathobjs", "r");
  if(!f)
  {
    send_to_char("Death object file is missing, no can do.\n", ch);
    return;
  }

  if(!*argument)
  {
    while (fgets(vnum, 15, f))
    {
      count++;
      vnum[strlen(vnum) - 1] = '\0';
      if((obj = read_object(atoi(vnum), VIRTUAL)))
      {
        sprintf(Gbuf, "%d. [%s] %s\n", count, vnum, obj->short_description);
        extract_obj(obj, TRUE);
      }
      else
      {
        sprintf(Gbuf, "%d. [%s] No object found.\n", count, vnum);
      }
      strcat(out, Gbuf);
    }
    fclose(f);
    send_to_char(out, ch);
    return;
  }
  else
  {
    argument_interpreter(argument, arg1, arg2);
    if(!*arg1 || !*arg2)
    {
      send_to_char("Usage: deathobj [add | remove] <vnum>\n", ch);
      fclose(f);
      return;
    }
    if(is_abbrev(arg1, "add"))
    {
      if(!(newvnum = atoi(arg2)))
      {
        send_to_char("Must be a valid vnum.\n", ch);
        fclose(f);
        return;
      }
      if(!(rn = real_object0(newvnum)))
      {
        send_to_char("No such object.\n", ch);
        fclose(f);
        return;
      }
      while (fgets(vnum, 15, f))
      {
        if(atoi(vnum) < newvnum || inserted)
          strcat(out, vnum);
        else if(atoi(vnum) == newvnum)
        {
          send_to_char("Vnum already on list.\n", ch);
          fclose(f);
          return;
        }
        else
        {
          sprintf(Gbuf, "%d\n", newvnum);
          strcat(out, Gbuf);
          strcat(out, vnum);
          inserted = TRUE;
        }
      }
      if(!inserted)
      {
        sprintf(Gbuf, "%d\n", newvnum);
        strcat(out, Gbuf);
      }
      obj_index[rn].func.obj = death_proc;
      fclose(f);
      if((f = fopen("Players/deathobjs", "w")))
      {
        fprintf(f, "%s", out);
        fclose(f);
      }
      send_to_char("New vnum inserted.\n", ch);
      return;
    }
    else if(is_abbrev(arg1, "remove"))
    {
      if(!(newvnum = atoi(arg2)))
      {
        send_to_char("Must be a valid vnum.\n", ch);
        fclose(f);
        return;
      }
      while (fgets(vnum, 15, f))
      {
        if(atoi(vnum) == newvnum)
          removed = TRUE;
        else
          strcat(out, vnum);
      }
      fclose(f);
      if(removed)
      {
        if((f = fopen("Players/deathobjs", "w")))
        {
          fprintf(f, "%s", out);
          fclose(f);
          obj_index[real_object0(newvnum)].func.obj = NULL;
          send_to_char("Object removed from list.\n", ch);
          return;
        }
        send_to_char("Error writing to file!\n", ch);
        return;
      }
      else
      {
        send_to_char("Vnum not found in list.\n", ch);
        return;
      }
    }
    else
    {
      send_to_char("Usage: deathobj [add | remove] <vnum>\n", ch);
      fclose(f);
      return;
    }
  }
}

void stat_dam(P_char ch, char *arg)
{
  char    *race_name, tmplate[512], buf[512], prop_name[512];
  float    pulse, multiplier, mult_mod, melee_factor;
  int      race;

  send_to_char("Race          &+WPulse&n  Mult  (&+WProp&n)\n", ch);
  send_to_char("-----------------------------------------\n", ch);
  melee_factor = get_property("damage.meleeFactor", 1.);
  for (race = 1; race <= LAST_RACE; race++)
  {
    race_name = race_names_table[race].ansi;
    pulse = combat_by_race[race][0];
    pulse += (int)(get_property("damage.pulse.class.all", 2));
    sprintf(prop_name, "damage.totalOutput.racial.%s",
            race_names_table[race].no_spaces);
    mult_mod = get_property(prop_name, 1.);
    multiplier = combat_by_race[race][1];    
    sprintf(buf, "%%-%ds &+W%%2d&n  %%.3f  (&+W%%.3f&n)\n",
            strlen(race_name) - ansi_strlen(race_name) + 15);
    sprintf(tmplate, buf, race_name, (int) pulse, multiplier, mult_mod);
    send_to_char(tmplate, ch);
  }
}

void stat_spldam(P_char ch, char *arg)
{
  char     line[1024], buf[512];

  send_to_char("Spell Type Mods (offensive / defensive)\n", ch);
  send_to_char("Race            Genrc Fire  Cold  Light Gas   Acid  Neg   Holy  Psi   Spirt Sound \n", ch);
  send_to_char("----------------------------------------------------------------------------------\n", ch);
  for (int race = 1; race <= LAST_RACE; race++)
  {
    strcpy(line, pad_ansi(race_names_table[race].ansi, 16).c_str());

    for (int type = 1; type <= LAST_SPLDAM_TYPE; type++)
    {
      sprintf(buf, "damage.spellTypeMod.offensive.racial.%s.%s",
              race_names_table[race].no_spaces,
              spldam_types[type]);

      float val = get_property(buf, 1.00);

      sprintf(buf, "%1.3f ", val);
      strcat(line, buf);
    }

    strcat(line, "\n");
    send_to_char(line, ch);

    sprintf(line, "                ");

    for (int type = 1; type <= LAST_SPLDAM_TYPE; type++)
    {
      sprintf(buf, "damage.spellTypeMod.defensive.racial.%s.%s",
              race_names_table[race].no_spaces,
              spldam_types[type]);

      float val = get_property(buf, 1.00);

      sprintf(buf, "%1.3f ", val);
      strcat(line, buf);
    }

    strcat(line, "\n\n");
    send_to_char(line, ch);
  }

}

void stat_game(P_char ch)
{
  P_desc   d;
  P_char   t_ch = NULL;
  char     buf[MAX_STRING_LENGTH];
  float    race[LAST_RACE + 1];
  float    m_class[CLASS_COUNT + 1];
  sh_int   i, n, evils = 0, goods = 0, pundeads = 0;
  float    x;

  buf[0] = '\0';
  x = used_descs;

  /* clear out counters */
  for (i = 0; i <= LAST_RACE + 1; i++)
    race[i] = 0.0;
  for (i = 0; i <= CLASS_COUNT; i++)
    m_class[i] = 0.0;
  /* begin counting */
  for (d = descriptor_list; d; d = d->next)
  {
    if(d->character)
      t_ch = d->character;
    else
      t_ch = NULL;
    if(d->connected != CON_PLYNG)
      continue;
    if(t_ch)
    {
      race[GET_RACE(t_ch)]++;
      m_class[flag2idx(t_ch->player.m_class)]++;

      if(EVIL_RACE(t_ch))
        evils++;
      if(GOOD_RACE(t_ch))
        goods++;
      if(PUNDEAD_RACE(t_ch))
        pundeads++;
    }
  }
/*  strcat(buf, "\n&+L(Values in approximate percentages currently online)&n\n");*/
  strcat(buf + strlen(buf),
         "\n&+B         RACES                          CLASSES\n\n");
  n = MAX(LAST_RACE, CLASS_COUNT);
  for (i = 0; i < n; i++)
  {
    if(i < LAST_RACE && race[i + 1])
    {
      sprintf(buf + strlen(buf), "%2d%%/%3d  %s",
              (int) ((race[i + 1] / x) * 100 + .5), (int) race[i + 1],
              pad_ansi(race_names_table[i + 1].ansi, 15).c_str());
    }
    else if(i < (LAST_RACE - 1))
    {
      sprintf(buf + strlen(buf), " 0%%/  0  %s",
              pad_ansi(race_names_table[i + 1].ansi, 15).c_str());
    }
    else
      strcat(buf + strlen(buf), "               ");

    if(i < CLASS_COUNT && m_class[i + 1])
    {
      sprintf(buf + strlen(buf), "      %10d%%/%3d  %s\n",
              (int) ((m_class[i + 1] / x) * 100 + .5), (int) m_class[i + 1],
              class_names_table[i + 1].ansi);
    }
    else if(i < (CLASS_COUNT - 1))
    {
      sprintf(buf + strlen(buf), "      %10d%%/%3d  %s \n",
              0, 0, class_names_table[i + 1].ansi);
    }
    else
      strcat(buf + strlen(buf), "\n");
  }
  sprintf(buf + strlen(buf), "\nGood/Evil/Undead -raced players: %3d/%3d/%3d",
          goods, evils, pundeads);
  sprintf(buf + strlen(buf), "\nTotal playing          : %3d\n", used_descs);
  send_to_char(buf, ch);
}

#define STAT_SYNTAX "Syntax:\n   stat game\n   stat room <room #>\n   stat zone <zone #>\n   stat obj|item  #|'name'\n   stat char|mob #|'name'\n   stat trap 'name'\n   stat shop #|'name'\n   stat damage\n   stat quest 'name'"


//CMD = 555 is used for storing stat o string in db.
void do_stat(P_char ch, char *argument, int cmd)
{
  P_char   k = 0, t_mob = 0, shopkeeper, mob;
  P_event  e1 = NULL;
  P_obj    j = 0, t_obj = 0;
  P_room   rm = 0;
  char     arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH], o_buf[MAX_STRING_LENGTH];
  char    *timestr;
  char     time_left[128], showed_vals = FALSE;
  int      i = 0, i2, i3, i4, m_virtual, num_tr, num_pr, count, x;
  Memory  *mem;
  struct affected_type *aff;
  struct extra_descr_data *desc;
  struct follow_type *fol;
  struct time_info_data playing_time;
  struct zone_data *zone = 0;
  float    fragnum = 0;
  static int class_mod[17] =
    { 0, 12, 12, 12, 12, 12, 8, 10, 8, 8, 4, 4, 4, 6, 6, 9, 6 };

  if(IS_NPC(ch))
    return;

  /* for mortals, reroute command to do_att, in case they are used to
     muds using stat */

  if(cmd != 555)
  {
    if(!IS_TRUSTED(ch))
    {
       for( shopkeeper = world[ch->in_room].people; shopkeeper; shopkeeper = shopkeeper->next_in_room )
       if( IS_SHOPKEEPER( shopkeeper ) )
       {
          shopping_stat(ch, shopkeeper, argument, cmd);
          return;
       }
       do_attributes(ch, argument, cmd);
       return;
    }
  }
  
  argument_interpreter(argument, arg1, arg2);

  if(!*arg1)
  {
    send_to_char(STAT_SYNTAX, ch);
    return;
  }
  o_buf[0] = '\0';              /* output string, so we can page it */
  buf[0] = '\0';
  buf1[0] = '\0';
  buf2[0] = '\0';

  /* stats on game */
  if((*arg1 == 'g') || (*arg1 == 'G'))
  {
    stat_game(ch);
    return;
  }
  /* stats on room  */

  if((*arg1 == 'r') || (*arg1 == 'R'))
  {
    // TODO: add guildhall room stats?
    if(!*arg2)
      i = ch->in_room;
    else
    {
      /* accept a room number as second arg  */
      if(!is_number(arg2) || ((i = real_room(atoi(arg2))) < 0) ||
          (i > top_of_world))
      {
        send_to_char("Room not in world.\n", ch);
        return;
      }
    }

    rm = &world[i];
    
    sprinttype(rm->sector_type, sector_types, buf2);
    sprintf(o_buf,
            "&+YRoom: [&N%d&+Y](&N%d&+Y)  Zone: &N%d&+Y  Sector type: &N%s\n",
            rm->number, i, 
            zone_table[rm->zone].number, 
            buf2);

    sprintf(o_buf + strlen(o_buf), "&+YName: &N%s\n", rm->name);

    sprintbitde(rm->room_flags, room_bits, buf2);
    sprintf(o_buf + strlen(o_buf), "&+YRoom flags:&N %s\n", buf2);

    sprintf(o_buf + strlen(o_buf), "&+YWeather sector: &N%d\n",
            in_weather_sector(real_room0(rm->number)));

    if(rm->continent)
    {
      sprintf(o_buf + strlen(o_buf), "&+YContinent: &n%s\n", continent_name(rm->continent));
    }    
    
    sprintf(o_buf + strlen(o_buf), "&+YJustice Patrol:&N %s \n",
            town_name_list[(int) rm->justice_area]);
    
//    sprintf(o_buf + strlen(o_buf), "&+YKingdom Type:&N %s ", kingdom_type_list[(int) rm->kingdom_type]);
//    sprintf(o_buf + strlen(o_buf), "&+YKingdom Number:&N %d\n", rm->kingdom_num);
//    sprintbit(rm->resources, resource_list, buf);
//    sprintf(o_buf + strlen(o_buf), "&+YResources:&N (%ld) %s\n", rm->resources, buf);

    sprintf(o_buf + strlen(o_buf),
            "&+YSpecial procedure:&N %s\n",
            (rm->funct) ? get_function_name((void*)rm->funct) : "None");
    
    sprintf(o_buf + strlen(o_buf),
            "&+YCurrent: (&N%d&+Y)-(&N%d)&+Y  Chance of falling:&N %d&+Y%%  Light sources:&N %d &+YSunShine:&N %s\n&+YDescription:&N\n",
            rm->current_speed, rm->current_direction, rm->chance_fall,
            rm->light, (IS_SUNLIT(i) ? "Yes" : "No"));

    sprintf(o_buf + strlen(o_buf),
            "&+YSection: &N%d  &+YX = &N%d  &+YY = &N%d  &+YZ = &N%d&N\n",
            rm->map_section, rm->x_coord, rm->y_coord, rm->z_coord);

    if(rm->description)
      strcat(o_buf, rm->description);

    if(rm->ex_description)
    {
      strcpy(buf, "\n&+YExtra description keywords(s):\n");
      for (desc = rm->ex_description; desc; desc = desc->next)
      {
        strcat(buf, desc->keyword);
        strcat(buf, "\n");
      }
      strcat(buf, "\n");
      strcat(o_buf, buf);
    }
    strcpy(buf, "&+Y------- Chars present -------\n");
    for (k = rm->people; k; k = k->next_in_room)
    {
      if(IS_PC(k) && !CAN_SEE(ch, k))
        continue;
      strcat(buf, IS_PC(k) ? " &+Y(PC)&N " : "&+R(NPC)&N ");
      strcat(buf, GET_NAME(k));
      strcat(buf, "\n");
    }
    strcat(buf, "\n");
    strcat(o_buf, buf);

    if(rm->contents)
    {
      strcpy(buf, "&+Y--------- Contents ---------\n");
      for (j = rm->contents; j; j = j->next_content)
      {
        strcat(buf, j->short_description);
        strcat(buf, "\n");
      }
      strcat(buf, "\n");
      strcat(o_buf, buf);
    }
    strcat(o_buf, "&+Y------- Exits defined -------\n");
    for (i = 0; i <= (NUM_EXITS - 1); i++)
    {
      if(rm->dir_option[i])
      {
        sprintbit((ulong) rm->dir_option[i]->exit_info, exit_bits, buf2);
        sprintf(buf,
                "&+YDirection &+R%5s  &+YKeyword: &+G%s  &+YKey:&N %d  &+YExit flag: &N%s\n&+YTo room: [&N%d&+Y](&N%d&+Y)&N  %s\n\n",
                dirs[i], rm->dir_option[i]->keyword, rm->dir_option[i]->key,
                buf2,
                (rm->dir_option[i]->to_room !=
                 NOWHERE) ? world[rm->dir_option[i]->to_room].number : -1,
                rm->dir_option[i]->to_room,
                (rm->dir_option[i]->general_description) ? rm->dir_option[i]->
                general_description : "UNDEFINED");
        strcat(o_buf, buf);
      }
    }
    page_string(ch->desc, o_buf, 1);
    return;

  }
  else if((*arg1 == 'z') || (*arg1 == 'Z'))
  {
    int zone_id, zone_number;

    if(!*arg2)
    {
      zone_id = world[ch->in_room].zone;
      zone = &zone_table[zone_id];
    }
    else if(is_number(arg2))
    {
      /* accept a zone number as second arg   */
      if(((zone_number = atoi(arg2)) > -1) && 
          ((zone_id = real_zone(zone_number)) >= 0))
      {
        zone = &zone_table[zone_id];
      }
    }
    else if( !strcmp( arg2, "portable" ) )
    {
      if( IS_MAP_ROOM(ch->in_room) )
      {
        send_to_char("&+rThis command is not available in a map zone, there are too many rooms.\n", ch);
        return;
      }

      send_to_char("&+YPortable rooms in current zone:\n", ch);

      zone_id = world[ch->in_room].zone;
      zone = &zone_table[zone_id];
      for( int i = zone->real_bottom; i < zone->real_top; i++ )
      {
        // If the room is teleportable, display it w/room vnum.
        if( !IS_SET(world[i].room_flags, NO_TELEPORT) )
        {
          sprintf( buf, "[&+C%d&n] %s\n", world[i].number, world[i].name );
          send_to_char( buf, ch );
        }
      }
      return;
    }

    if(!zone)
    {
      send_to_char("Invalid zone number. Type 'world zones' to see list.\n", ch);
      return;
    }

    sprintf(o_buf, "&+YZone: [&N%d&+Y](&N%d&+Y)  Name:&N %s&n  &+YFilename:&n %s\n",
      zone->number, zone_id, zone->name, zone->filename);

    int maproom = maproom_of_zone(zone_id);
    if(maproom > 0)
    {
      sprintf(o_buf + strlen(o_buf), "&+YConnects to map room: [&N%d&+Y]\n", maproom);
    }

    sprintf(o_buf + strlen(o_buf), "&+YRooms: &N%d  &+YRange: [&N%d&+Y](&N%d&+Y) to [&N%d&+Y](&N%d&+Y)  Top: &N%d\n",
      zone->real_top - zone->real_bottom + 1, world[zone->real_bottom].number, zone->real_bottom,
      world[zone->real_top].number, zone->real_top, zone->top);

    sprintf(o_buf + strlen(o_buf), "&+YDifficulty: &N%d ", zone->difficulty);

    sprintf(o_buf + strlen(o_buf), "&+YAvg mob level: &N%d ", zone->avg_mob_level);

    sprintf(o_buf + strlen(o_buf), "&+YLifespan: &N%d  &+YAge: &N%d  &+R", zone->lifespan, zone->age);

    switch (zone->reset_mode)
    {
    case 0:
      strcat(o_buf, "Zone never resets.\n");
      break;
    case 1:
      strcat(o_buf, "Zone resets when empty.\n");
      break;
    case 2:
      strcat(o_buf, "Zone resets regardless.\n");
      break;
    default:
      strcat(o_buf, "Invalid reset mode!\n");
      break;
    }

    sprintf(o_buf + strlen(o_buf), "&+YFull reset lifespan: &n%d  &+YFull reset age: &n%d\n",
      zone->fullreset_lifespan, zone->fullreset_age);

    if( IS_SET(zone->flags, ZONE_MAP) )
    {
      sprintf(o_buf + strlen(o_buf), "&+YMap size:&n %d&+Yx&n%d\n", zone->mapx, zone->mapy);
    }

    sprintf(o_buf + strlen(o_buf), "&+YControlling town:&N %s\n",
      town_name_list[zone->hometown]);
    sprintbit(zone->hometown ? hometowns[zone->hometown - 1].flags : 0,
      justice_flags, buf2);
    sprintf(o_buf + strlen(o_buf), "&+YJustice:&N %s\n", buf2);
    sprintbit(zone->flags, zone_bits, buf);
    sprintf(o_buf + strlen(o_buf), "&+YZone flags:&N %s\n", buf);

    struct zone_info zinfo;
    if(get_zone_info(zone->number, &zinfo))
    {
      string buff;

      sprintf(o_buf + strlen(o_buf), "\n&+GZone Info\n");

      sprintf(o_buf + strlen(o_buf), "&+gTask zone: &+G%s  &+gQuest zone:  &+G%s  &+gTrophy zone:  &+G%s\n",
        (zinfo.task_zone ? "YES" : "NO"), (zinfo.quest_zone ? "YES" : "NO"), (zinfo.trophy_zone ? "YES" : "NO"));

      if(zinfo.epic_type)
      {
        if(zinfo.epic_level)
        {
          sprintf(o_buf + strlen(o_buf), "&+gGrants epic level: &+G%d\n", zinfo.epic_level);
        }

        sprintf(o_buf + strlen(o_buf), "&+gRarity: &+G%1.3f  ", zinfo.frequency_mod);

        sprintf(o_buf + strlen(o_buf), "&+gZone frequency multiplier: &+G%1.3f\n", zinfo.zone_freq_mod);

        sprintf(o_buf + strlen(o_buf), "&+gEpic value: &+G%d  &+gSuggested group size: &+G%d\n",
          zinfo.epic_payout, zinfo.suggested_group_size);

        sprintf(o_buf + strlen(o_buf), "&+gEpic stone(s):\n");

        for( P_obj tobj = object_list; tobj; tobj = tobj->next )
        {
          if( obj_zone_id(tobj) != zone_id )
          {
            continue;
          }

          int obj_vnum = obj_index[tobj->R_num].virtual_number;

          if( obj_vnum != EPIC_SMALL_STONE && obj_vnum != EPIC_LARGE_STONE && obj_vnum != EPIC_MONOLITH )
          {
            continue;
          }

          int obj_room_vnum = world[obj_room_id(tobj)].number;
          if( obj_room_vnum < 0 )
          {
            continue;
          }
          sprintf(o_buf + strlen(o_buf), " %s &nin &+W[&n%d&+W]\n", tobj->short_description, obj_room_vnum);
        }
      }
    }
    sprintf(o_buf + strlen(o_buf), "\n&+YExits from this zone:\n", buf);

    int exits_shown = 0;
    for( i3 = 0, i = zone->real_bottom; (i != NOWHERE && i <= zone->real_top && exits_shown < 1000); i++)
    {
      for( i2 = 0; i2 < NUM_EXITS; i2++ )
      {
        if( world[i].dir_option[i2] )
        {
          if( (world[i].dir_option[i2]->to_room == NOWHERE)
            || (world[world[i].dir_option[i2]->to_room].zone != world[i].zone) )
          {
            if( !i3 )
            {
              i3 = 1;
            }
            if( world[i].dir_option[i2]->to_room == NOWHERE )
            {
              sprintf(o_buf + strlen(o_buf), " &+Y[&n%5d&+Y]&n &+R%-5s&n to &+WNOWHERE\n", world[i].number, dirs[i2]);
              exits_shown++;
            }
            else
            {
              sprintf(o_buf + strlen(o_buf), " &+Y[&n%5d&+Y]&n &+R%-5s&n to &+Y[&+R%3d&n:&+C%5d&+Y]&n %s\n",
                world[i].number, dirs[i2], zone_table[world[world[i].dir_option[i2]->to_room].zone].number,
                world[world[i].dir_option[i2]->to_room].number, world[world[i].dir_option[i2]->to_room].name);
              exits_shown++;
            }
          }
        }
      }
    }

    if(!i3)
    {
      strcat(o_buf, "&+RNONE!&n\n");
    }
    if(exits_shown >= 1000)
    {
      strcat(o_buf, " (and many more)\n");
    }
    page_string(ch->desc, o_buf, 1);
    return;
    /* stat all zones, for exits */
  }
  else if((*arg1 == 'w') || (*arg1 == 'W'))
  {
    send_to_char("broken, leave me alone.\n", ch);
    return;

    for (x = 0; x <= top_of_zone_table; x++)
    {
      zone = &zone_table[x];
      sprintf(o_buf, "&+YZone: (&N%d&+Y)  Name:&N %s\n",
              world[zone->real_bottom].zone, zone->name);

      for (i3 = 0, i = zone->real_bottom;
           (i != NOWHERE) && (i <= zone->real_top); i++)
        for (i2 = 0; i2 < NUM_EXITS; i2++)
          if(world[i].dir_option[i2])
          {
            if((world[i].dir_option[i2]->to_room == NOWHERE) ||
                (world[world[i].dir_option[i2]->to_room].zone !=
                 world[i].zone))
            {
              if(!i3)
                i3 = 1;
              if(world[i].dir_option[i2]->to_room == NOWHERE)
                sprintf(o_buf + strlen(o_buf),
                        " &+Y[&n%5d&+Y](&n%5d&+Y)&n &+R%-5s&n to &+WNOWHERE\n",
                        world[i].number, i, dirs[i2]);
              else
                sprintf(o_buf + strlen(o_buf),
                        " &+Y[&n%5d&+Y]&n &+R%-5s&n to &+Y[&+R%3d&n:&+Y%5d&+Y]&n %s\n",
                        world[i].number, dirs[i2],
                        zone_table[world[world[i].dir_option[i2]->to_room].zone].number,
                        world[world[i].dir_option[i2]->to_room].number,
                        world[world[i].dir_option[i2]->to_room].name);
            }
          }
      send_to_char(o_buf, ch);
    }
    return;

    /* stat on object  */
  }
  else if((*arg1 == 'o') || (*arg1 == 'O') || (*arg1 == 'i') ||
           (*arg1 == 'I'))
  {

    if(!*arg2)
    {
      send_to_char(STAT_SYNTAX, ch);
      return;
    }

    if(is_number(arg2))
    {
      if((i = real_object(atoi(arg2))) < 0)
      {
        send_to_char("Illegal object number.\n", ch);
        return;
      }
      /* load one to stat, extract after statting  */
      t_obj = read_object(i, REAL);
      if(!t_obj)
      {
        logit(LOG_DEBUG, "do_stat(): obj %d [%d] not loadable", i,
              obj_index[i].virtual_number);
        return;
      }                         /*else
                                   obj_to_room(t_obj, 0); */
      if(t_obj->name)
        strcpy(arg2, t_obj->name);
    }
/*
    if(cmd == 555) //special code for web eq stats
    {
        if(j = get_obj_vis(ch, arg2))
        {
          if(!strcmp(j->name, arg2))
            break;
          else
          j = NULL;
        }

        if(j == NULL)
         return;

    }
    else
*/ if(!(j = get_obj_vis(ch, arg2)))
    {
        send_to_char("No such object.\n", ch);
      if(t_obj)
      {
        extract_obj(t_obj, FALSE);
        t_obj = NULL;
      }
      return;
    }
    m_virtual = (j->R_num >= 0) ? obj_index[j->R_num].virtual_number : 0;

    sprinttype(GET_ITEM_TYPE(j), item_types, buf2);
    sprintf(o_buf,
            "&+YObject:\n&+YNumber: [&N%d&+Y](&N%d&+Y)  Type: &N%s  &+YName: &N%s\n",
            m_virtual, j->R_num, buf2,
            ((j->short_description) ? j->short_description : "None"));

    sprintf(o_buf + strlen(o_buf),
            "&+YKeywords: &N%s\n&+YLong description:\n%s\n",
            ((j->name) ? j->name : "None"),
            ((j->description) ? j->description : "None"));

    if(j->ex_description)
    {
      strcpy(buf, "&+YExtra description keyword(s):\n&+Y----------\n");
      for (desc = j->ex_description; desc; desc = desc->next)
      {
        strcat(buf, desc->keyword);
        strcat(buf, "\n");
      }
      strcat(buf, "&+Y----------\n");
      strcat(o_buf, buf);
    }
    sprintf(o_buf + strlen(o_buf), "&+YNumber in game : &N%d\n",
            (obj_index[j->R_num].number - ((t_obj != NULL) ? 1 : 0)));

    sprintbitde(j->wear_flags, wear_bits, buf2);
    sprintf(o_buf + strlen(o_buf), "&+YCan be worn on : &N%s\n", buf2);

    if(j->bitvector)
    {
      sprintbitde(j->bitvector, affected1_bits, buf2);
      sprintf(o_buf + strlen(o_buf), "&+YSet char bits 1: &N%s\n", buf2);
    }

    if(j->bitvector2)
    {
      sprintbitde(j->bitvector2, affected2_bits, buf2);
      sprintf(o_buf + strlen(o_buf), "&+YSet char bits 2: &N%s\n", buf2);
    }

    if(j->bitvector3)
    {
      sprintbitde(j->bitvector3, affected3_bits, buf2);
      sprintf(o_buf + strlen(o_buf), "&+YSet char bits 3: &N%s\n", buf2);
    }

    if(j->bitvector4)
    {
      sprintbitde(j->bitvector4, affected4_bits, buf2);
      sprintf(o_buf + strlen(o_buf), "&+YSet char bits 4: &N%s\n", buf2);
    }

    if(j->bitvector5)
    {
      sprintbitde(j->bitvector5, affected5_bits, buf2);
      sprintf(o_buf + strlen(o_buf), "&+YSet char bits 5: &N%s\n", buf2);
    }

    if(j->extra_flags)
    {
      sprintbitde(j->extra_flags, extra_bits, buf2);
      sprintf(o_buf + strlen(o_buf), "&+YExtra flags    : &N%s (%d)\n", buf2, j->extra_flags);
    }

    if(j->extra2_flags)
    {
      sprintbitde(j->extra2_flags, extra2_bits, buf2);
      sprintf(o_buf + strlen(o_buf), "&+YExtra2 flags   : &N%s\n", buf2);
    }

    if(j->anti_flags)
    {
      *buf2 = '\0';
      for (x = 0; x < CLASS_COUNT; x++)
        if(j->anti_flags & (((unsigned long) 1) << x))
          sprintf(buf2 + strlen(buf2), "%s ",
                  class_names_table[x + 1].normal);
      sprintf(o_buf + strlen(o_buf), "&+Y%s : &N%s\n",
              IS_SET(j->extra_flags,
                     ITEM_ALLOWED_CLASSES) ? "Allowed classes" :
              "Denied classes", buf2);
    }

    if(j->anti2_flags)
    {
      *buf2 = '\0';
      for (x = 0; x < RACE_PLAYER_MAX; x++)
        if(j->anti2_flags & (((unsigned long) 1) << x))
          sprintf(buf2 + strlen(buf2), "%s ",
                  race_names_table[x + 1].no_spaces);
      sprintf(o_buf + strlen(o_buf), "&+Y%s  : &N%s\n",
              IS_SET(j->extra_flags,
                     ITEM_ALLOWED_RACES) ? "Allowed races" : "Denied races",
              buf2);
    }

    sprintf(o_buf + strlen(o_buf),
            "&+YWeight: &N%d&+Y lbs   Value: &N%s   &+YCondition: &N%d   &+YItem Value: &N%d\n",//%d(%d%%)\n",
            j->weight, comma_string((long) (j->cost)), j->condition, itemvalue(ch, j));
//, j->max_condition, (int) (((float) j->condition / j->max_condition) * 100)); wipe2011

    sprintf(o_buf + strlen(o_buf),
            "&+YT0: &n%d&+Y  T1: &n%d&+Y  T2: &n%d&+Y  T3: &n%d&+Y  T4: &n%d&+Y  T5: &n%d\n",
            (int) j->timer[0], (int) j->timer[1], (int) j->timer[2],
            (int) j->timer[3], (int) j->timer[4], (int) j->timer[5]);

    sprintf(o_buf + strlen(o_buf), "&+YCraftsmanship: &n%s\n", craftsmanship_names[j->craftsmanship]);

    sprinttype(j->material, item_material, buf2);
    sprintf(o_buf + strlen(o_buf), "&+YMaterial: &n%s\n", buf2);

    if(!t_obj)
    {
      strcat(o_buf, "&+YLocation: ");
      strcat(o_buf, where_obj(j, FALSE));
      strcat(o_buf, "\n");
    }
    switch (j->type)
    {
    case ITEM_LIGHT:
      sprintf(buf, "&+YColor: [&N%d&+Y]  Type: [&N%d&+Y]  Hours: [&N%d&+Y]",
              j->value[0], j->value[1], j->value[2]);
      break;
    case ITEM_POTION:
    case ITEM_SCROLL:
      sprintf(buf, "&+Y Level: &N%d&+Y  Spells:&N ", j->value[0]);

      for (i = 1; (i < 4) && (j->value[i] > 0); i++)
      {
        sprinttype(j->value[i], (const char **) spells, buf2);
        sprintf(buf, "%s%d) &+C%s [%d]&+Y, ", buf, j->value[i], buf2,
                GetCircle(j->value[i]));
      }

      i = strlen(buf);
      if(buf[i - 2] != ',')
        strcat(buf, "&+RBUGGED!&N\n");
      else
      {
        buf[i - 2] = ',';
        buf[i - 1] = '\0';
      }

      break;
    case ITEM_STAFF:
    case ITEM_WAND:
      if(j->value[3] > 0)
        sprinttype(j->value[3], (const char **) spells, buf2);
      else
        strcpy(buf2, "&+RBUGGED!&");
      sprintf(buf, "%d(%d)&+Y charges, Level &N%d&+Y spell: %d) &+C%s [%d]&N",
              j->value[1], j->value[2], j->value[0], j->value[3], buf2,
              GetCircle(j->value[3]));
      break;
    case ITEM_FIREWEAPON:
      if((j->value[3] < 1) || (j->value[3] > 6))
        strcpy(buf2, "&+RBUGGED!&N");
      else
        sprinttype(j->value[3] - 1, shot_types, buf2);
      sprintf(buf,
              "&+YRange: &N%d  &+YRate of fire: &N%d  &+YMissile type: &N%d",
              j->value[1], j->value[0], j->value[3]);
      break;
    case ITEM_WEAPON:
      {
        int      spells[3];
        char     spell_list[512];

        spells[0] = j->value[5] % 1000;
        spells[1] = j->value[5] % 1000000 / 1000;
        spells[2] = j->value[5] % 1000000000 / 1000000;

        if((j->value[0] < 1) || (j->value[0] > WEAPON_NUMCHUCKS))
          strcpy(buf2, "&+RBUGGED!&N");
        else
          strcpy(buf2, weapon_types[j->value[0] - 1].flagLong);

        if(obj_index[j->R_num].func.obj == NULL && j->value[5])
        {
          if(skills[spells[0]].name)
            strcpy(spell_list, skills[spells[0]].name);
          if(spells[1] && skills[spells[1]].name)
            sprintf(spell_list + strlen(spell_list), "&n, &+W%s",
                    skills[spells[1]].name);
          if(spells[2] && skills[spells[2]].name)
            sprintf(spell_list + strlen(spell_list), "&n, &+W%s",
                    skills[spells[2]].name);

          if(j->value[5] / 1000000000)
            sprintf(buf1,
                    "&+YProcs one of &+W%d&+Y level &+W%s &+Yat &+W1/%d&+Y chance\n",
                    j->value[6], spell_list, j->value[7]);
          else
            sprintf(buf1,
                    "&+YProcs all of &+W%d&+Y level &+W%s &+Yat &+W1/%d&+Y chance\n",
                    j->value[6], spell_list, j->value[7]);
        }
        else
          *buf1 = 0;
        sprintf(buf, "%s&+YType: &n%s &+Ydice: &N%dD%d&N %s", buf1, buf2,
                j->value[1], j->value[2],
                j->value[4] ? "&+g(Poisoned)&n" : "");
        break;
      }
    case ITEM_QUIVER:
      sprintf(buf,
              "&+YMax Capacity: &N%d  &+YCurrent No. Arrows. &N%d  &+YContainer Flags: &N%d  &+YMissile type: &N%d",
              j->value[0], j->value[3], j->value[1], j->value[2]);
      break;
    case ITEM_MISSILE:
      if((j->value[3] < 1) || (j->value[3] > 6))
        strcpy(buf2, "&+RBUGGED!&N");
      else
        sprinttype(j->value[3] - 1, shot_types, buf2);
      sprintf(buf, "&+YDamage: &N%dd%d&N &+YMissile Type: &n%s",
              j->value[1], j->value[2],
              missile_types[j->value[3] - 1].flagLong);
      break;
    case ITEM_ARMOR:
      sprintf(buf, "&+YAC-apply: &N%d  &+rWarmth: &N%d  &+YPrestige: &N%d",
              j->value[0], j->value[1], j->value[2]);

/*      sprintf(buf, "&+YDefl: &n%.2f  &+YAbs: &n%.2f",
              getArmorDeflection(j, NULL), getArmorAbsorbtion(j, NULL));*/
      break;
    case ITEM_SHIELD:
      sprintf(buf, "&+YAC-apply: &N%d", j->value[3]);
      break;
    case ITEM_CONTAINER:
    case ITEM_STORAGE:
      sprintf(buf,
              "&+YHolds: &N%d  &+YLocktype: &N%d  &+YKey: &N%d  &+YSize hold: &N%d",
              j->value[0], j->value[1], j->value[2], j->value[3]);
      break;
    case ITEM_CORPSE:
      if(IS_SET(j->value[1], PC_CORPSE))
        sprintf(buf, "&+mPlayer Corpse&n &+YHolding:&n %d &+Ylbs&N",
                j->value[0]);
      else
        sprintf(buf, "&+bNPC Corpse&n&+Y (&n%d&+Y) Holding:&n %d &+Ylbs&n",
                j->value[3], j->value[0]);
      break;
    case ITEM_DRINKCON:
      sprinttype(j->value[2], drinks, buf2);
      sprintf(buf,
              "&+YHolds: &N%d  &+YContains:&N %d  &+YPoisoned:&N %d  &+YLiquid:&N %s",
              j->value[0], j->value[1], j->value[3], buf2);
      break;
    case ITEM_NOTE:
      sprintf(buf, "&+YTongue:&N %d", j->value[0]);
      break;
    case ITEM_KEY:
      sprintf(buf, "&+YKeytype:&N %d", j->value[0]);
      break;
    case ITEM_FOOD:
      sprintf(buf, "&+YMakes full:&N %d  &+YPoisoned:&N %d",
              j->value[0], j->value[3]);
      break;
    case ITEM_MONEY:
      sprintf(buf,
              "&+YCopper:&N %d  &+YSilver:&N %d  &+YGold:&N %d  &+YPlatinum:&N %d",
              j->value[0], j->value[1], j->value[2], j->value[3]);
      break;
    case ITEM_WORN:
      sprintf(buf, "&+rWarmth:&N %d  &+YPrestige:&N %d  &+YMaterial:&n %d",
              j->value[1], j->value[2], j->value[3]);
      break;
    case ITEM_TELEPORT:
      i = real_room(j->value[0]);
      sprintf(buf, "&+YTo room: [&N%d&+Y]&N %s\n"
              "&+YCommand #: [&N%d&+Y]  Charges: [&N%d&+Y]  Zone-to: [&N%d&+Y]",
              j->value[0],
              ((i > 1) && (i <= top_of_world)) ?
              world[real_room(j->value[0])].name : "",
              j->value[1], j->value[2], j->value[3]);
      break;
    case ITEM_BANDAGE:
      sprintf(buf, "&+YHeals : &n%d&n", j->value[0]);
      break;
    default:
      sprintf(buf,
              "&+YValues 0-7: [&N%d&+Y] [&N%d&+Y] [&N%d&+Y] [&N%d&+Y] [&n%d&+Y] [&n%d&+Y] [&n%d&+Y] [&n%d&+Y]",
              j->value[0], j->value[1], j->value[2], j->value[3], j->value[4],
              j->value[5], j->value[6], j->value[7]);
      showed_vals = TRUE;

      break;
    }
    strcat(o_buf, buf);

    if(!showed_vals)
      sprintf(buf,
              "\n&+YValues 0-7: [&N%d&+Y] [&N%d&+Y] [&N%d&+Y] [&N%d&+Y] [&n%d&+Y] [&n%d&+Y] [&n%d&+Y] [&n%d&+Y]",
              j->value[0], j->value[1], j->value[2], j->value[3], j->value[4],
              j->value[5], j->value[6], j->value[7]);
    else
      buf[0] = '\0';

    strcat(o_buf, buf);

    sprintf(buf, "\n&+YSpecial procedure:&N ");
    if(j->R_num >= 0)
      strcat(buf, (obj_index[j->R_num].func.obj ?
            get_function_name((void*)obj_index[j->R_num].func.obj) : "No"));
    else
      strcat(buf, "No");
    strcat(buf, "\n");

    strcat(o_buf, buf);
    /*
    strcat(buf, "\n&+YGod procedure:&N ");
    if(j->R_num >= 0)
      strcat(buf, (obj_index[j->R_num].god_func ? "exists\n" : "No\n"));
    else
      strcat(buf, "No\n");

    */

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if(j->affected[i].location != APPLY_NONE)
      {
        sprinttype(j->affected[i].location, apply_types, buf2);
        sprintf(o_buf + strlen(o_buf), "   &+YAffects: &+c%s&+y By &N%d\n",
                buf2, j->affected[i].modifier);
      }
    if(j->affects)
    {
      struct obj_affect *o_af;

      strcat(o_buf, "&+YAffected by: \n");
      for (o_af = j->affects; o_af; o_af = o_af->next)
      {
        if(o_af->extra2)
        {
          sprintf(o_buf + strlen(o_buf),
                  "   &n%s &+Yfor&n %d &+Ygranting:&n ",
                  skills[o_af->type].name, (int) o_af->data);
          sprintbitde(o_af->extra2, extra2_bits, o_buf + strlen(o_buf));
        }
        else
          sprintf(o_buf + strlen(o_buf), "   &n%s &+Yfor&n %d&n",
                  skills[o_af->type].name, (int) o_af->data);
        strcat(o_buf, "\n");
      }
    }
    if(j->events || j->nevents)
    {
      P_nevent ne;
      strcat(o_buf, "&+YEvents:\n&+Y-------\n");

      for (e1 = j->events; e1; e1 = e1->next)
        sprintf(o_buf + strlen(o_buf),
                "%6d&+Y seconds,&n %s%s&+Y.\n",
                event_time(e1, T_SECS),
                event_names[(int) e1->type],
                (e1->one_shot) ? "" : "&+Y(&N&+RR&+Y)");
      for (ne = j->nevents; ne; ne = ne->next)
      {
        sprintf(o_buf + strlen(o_buf),
                "%6d&+Y seconds,&n %s&+Y.\n",
                ne_event_time(ne)/WAIT_SEC, get_function_name((void*)ne->func));
      }
      strcat(o_buf, "\n");
    }
    
    /* Since quality of an item can have some meaning now, let's add it to stat command -Alver */
    { 
        int craft = j->craftsmanship;
        
        sprintf(buf, "\n&+YQuality:&N ");
        
      if(craft < OBJCRAFT_LOWEST ||
          craft > OBJCRAFT_HIGHEST)
      {
        strcat(buf, "BUGGY!\n");
      }
      else
      {
        strcat(buf, craftsmanship_names[craft]);
        strcat(buf, "\n");
      }
      if( IS_ARTIFACT( j ) )
      {
         long ct = time(0);
         strcat( buf, "&+YIn game since: &n" );
         strcat( buf, asctime(localtime(& (j->timer[5]) )) );
         strcat( buf, "\n");
      }
      strcat(o_buf, buf);
    }

    /*
       if(j->justice_status > 0) {
       strcat(o_buf, "\n&+RJustice:\n");
       sprinttype(j->justice_status, justice_obj_status, buf2);
       sprintf(o_buf + strlen(o_buf), " &+RStatus: %s Belongs to: %s&N\n",
       buf2, j->justice_name);
       } */

   //Insert item into db
    if(cmd == 555){
      sql_insert_item(ch, j, o_buf);
    }
    else {
       if(j->contains)
        strcat(o_buf, "\n&+YContains:\n");

        page_string(ch->desc, o_buf, 1);

        if(j->contains)
           list_obj_to_char(j->contains, ch, 2, TRUE);
    }

    if(t_obj)
    {
      extract_obj(t_obj, FALSE);
      t_obj = NULL;
    }
    return;

  }
  else if((*arg1 == 'c') || (*arg1 == 'C') || (*arg1 == 'm') ||
           (*arg1 == 'M'))
  {

    /* mobile in world  */

    if(!*arg2)
    {
      send_to_char(STAT_SYNTAX, ch);
      return;
    }
    if(is_number(arg2))
    {
      if((i = real_mobile0(atoi(arg2))) == 0)
      {
        send_to_char("Illegal mob number.\n", ch);
        return;
      }
      /* load one to stat, extract after statting  */
      t_mob = read_mobile(i, REAL);
      if(!t_mob)
      {
        logit(LOG_DEBUG, "do_stat(): mob %d [%d] not loadable", i,
              mob_index[i].virtual_number);
        send_to_char("error loading mob to stat.\n", ch);
        return;
      }
      else
      {
        char_to_room(t_mob, 0, -2);
      }
      if(t_mob->player.name)
        strcpy(arg2, t_mob->player.name);
    }
    if(!(k = get_char_vis(ch, arg2)))
    {
      send_to_char("No such character.\n", ch);
      if(t_mob)
      {
        extract_char(t_mob);
        t_mob = NULL;
      }
      return;
    }
    switch (k->player.sex)
    {
    case SEX_NEUTRAL:
      strcpy(buf, "Neuter");
      break;
    case SEX_MALE:
      strcpy(buf, "&+BMale&N");
      break;
    case SEX_FEMALE:
      strcpy(buf, "&+RFemale&N");
      break;
    default:
      strcpy(buf, "&+MILLEGAL-SEX!!&N");
      break;
    }
	
    sprintf(buf1, "  &+YIn room: [&N%d&+Y] Zone: [&n%d&+Y](&n%d&+Y) %s", world[k->in_room].number,
                  zone_table[world[k->in_room].zone].number, world[k->in_room].zone, zone_table[world[k->in_room].zone].name);
    sprintf(buf2, "%s %s%s  ", buf,
            (IS_PC(k) ? "&+YPC" : (IS_PC(k) ? "&+RNPC" : "&+GMOB")),
            (t_mob) ? "" : buf1);
    if(IS_NPC(k))
      sprintf(buf,
              "Numbers: &N%d&+Y-V &N%d&+Y-R &N%d&+Y-I   # in game: &n%d\n",
              mob_index[GET_RNUM(k)].virtual_number, GET_RNUM(k), GET_IDNUM(k),
              (mob_index[GET_RNUM(k)].number));
    else
      sprintf(buf, " Name: &N%s  &+YID numb: &n%d\n", GET_NAME(k), GET_PID(k));
    strcat(buf2, buf);
    strcpy(o_buf, buf2);

    if(IS_NPC(k))
      sprintf(buf, "&+YName: &N%s\n&+YKeywords: &N%s\n&+YDescription:\n%s\n",
              k->player.short_descr, GET_NAME(k), k->player.long_descr);
    else
      sprintf(buf,
              "&+YTitle: &N%s\n&+YShort Description:&n%s\n&+YLong Description:\n%s\n",
/*              k->only.pc->title ? k->only.pc->title : "&+rNone", */
              k->player.title ? k->player.title : "&+rNone",
              k->player.short_descr ? k->player.short_descr : "&+rNone",
              k->player.description ? k->player.description : "&+rNone");
    strcat(o_buf, buf);

    if(IS_NPC(k))
    {
      //sprintf(buf2, "&+Y+(&N%s&+Y)", comma_string((long) (GET_LEVEL(k) * GET_HIT(k) * .4)));
      sprintf(buf2, "");
    }
    else
    {
      if((k->player.m_class == 0) ||
          (k->player.m_class > (1 << CLASS_COUNT - 1)) || (GET_LEVEL(k) < 1)
          || IS_TRUSTED(k))
        strcpy(buf1, "Unknown");
      else
        strcpy(buf1,
               comma_string((long)
                            (new_exp_table[GET_LEVEL(k) + 1] - GET_EXP(k))));
      sprintf(buf2, "&+Y Exp to Level: &N%s",
              IS_TRUSTED(k) ? "Unknown" : buf1);
    }

    sprintf(buf,
            "&+YLevel: &N%d&+Y(&n%d&+Y)&n  &+YExperience: &N%s %s  &+YAlignment [&N%d&+Y] Assoc:&n %d\n",
            k->player.level,
            IS_PC(k) ? k->only.pc->highest_level : GET_LEVEL(k),
            comma_string((int) GET_EXP(k)), buf2, GET_ALIGNMENT(k), GET_A_NUM(k));
    strcat(o_buf, buf);


    sprintf(buf, "&+YRace: &N%s  &+YClass: &N",
            race_names_table[k->player.race].ansi);
    get_class_string(k, buf2);
    strcat(buf, buf2);
    sprintf(buf + strlen(buf), " &+YRacewar: ");
    if(IS_NPC(k))
      sprintf(buf + strlen(buf), "&+wNPC&n");
    else if(GET_RACEWAR(k) == RACEWAR_GOOD)
      sprintf(buf + strlen(buf), "&+WGood&n");
    else if(GET_RACEWAR(k) == RACEWAR_EVIL)
      sprintf(buf + strlen(buf), "&+rEvil&n");
    else if(GET_RACEWAR(k) == RACEWAR_UNDEAD)
      sprintf(buf + strlen(buf), "&+LUndead&n");
    else if(GET_RACEWAR(k) == RACEWAR_NEUTRAL)
      sprintf(buf + strlen(buf), "&+yNeutral&n");
    else
      sprintf(buf + strlen(buf), "&+RINVALID&n");

    sprintf(buf2,
            "\n&+YHometown: &N%d  &+YBirthplace: &N%d  &+YOrig BP: &n%d &+YSpell Pulse: &n%+.2f\n",
            GET_HOME(k), GET_BIRTHPLACE(k), GET_ORIG_BIRTHPLACE(k), spell_pulse_data[GET_RACE(k)] * SPELL_PULSE(k) );
    strcat(buf, buf2);
    strcat(o_buf, buf);
    if(IS_PC(k))
      fragnum = (float) k->only.pc->frags;
    else
      fragnum = 0;
    fragnum /= 100;

    sprintf(buf, "&+YPulse: &N%4d&+Y  Current Pulse: &N%4d&+Y  Dam Multiplier: &N%1.2f  &+YFrags:&n %+.02f\n",
      k->specials.base_combat_round, k->specials.combat_tics,
      k->specials.damage_mod, fragnum );
    strcat(o_buf, buf);

    strcat(o_buf, "\n");
    
    if(IS_PC(k))
    {
      struct affected_type *paf = get_spell_from_char(ch, TAG_EPICS_GAINED);

      sprintf(buf, "&+YEpic points: &n%ld&+Y  Total epics gained: &n%d\n", k->only.pc->epics, paf ? paf->modifier : 0);
      strcat(o_buf, buf);

      sprintf(buf,
              "&+YAge: &N%4d &+Yyears  &N%2d &+Ymonths  &N%2d &+Ydays  &N%2d &+YHours\n",
              age(k).year, age(k).month, age(k).day, age(k).hour);
      strcat(o_buf, buf);

      playing_time =
        real_time_passed((long) (k->player.time.played +
                                 (time(0) - k->player.time.logon)), 0);
      sprintf(buf,
              "&+YPlayed:  &N%3d &+Ydays  &N%2d &+Yhours  &N%2d &+Yminutes\n",
              playing_time.day, playing_time.hour, playing_time.minute);
      strcat(o_buf, buf);

      playing_time = real_time_passed(time(0), k->player.time.logon);
      sprintf(buf,
              "&+YSession: &N%3d &+Ydays  &N%2d &+Yhours  &N%2d &+Yminutes\n",
              playing_time.day, playing_time.hour, playing_time.minute);
      strcat(o_buf, buf);
    }
    else
    {
      playing_time = real_time_passed(time(0), k->player.time.birth);
      sprintf(buf,
              "&+YLived: &N%2d &+Ydays  &N%2d &+Yhours  &N%2d &+Yminutes\n",
              playing_time.day, playing_time.hour, playing_time.minute);
      strcat(o_buf, buf);
    }

    strcat(o_buf, "      &+gCur (Bas)      Cur (Bas)\n");

    for (i = 0, i3 = 0; i < MAX_WEAR; i++)
      if(k->equipment[i])
        i3++;
    i2 = GET_HEIGHT(k);
    i =  i2 / 12;
    i2 -= i * 12;

    sprintf(buf,
            "&+YStr: &n%3d&+Y (&n%3d&+Y)    Pow: &n%3d&+Y (&n%3d&+Y)    Height: &n%3d&+Y\' &n%2d&+Y\" (&n%d&+Yin)\n",
            GET_C_STR(k), k->base_stats.Str, GET_C_POW(k), k->base_stats.Pow, i, i2, GET_HEIGHT(k));
    strcat(o_buf, buf);

    sprintf(buf,
            "&+YDex: &n%3d&+Y (&n%3d&+Y)    Int: &n%3d&+Y (&n%3d&+Y)    Weight: &n%3d&+Y lbs\n",
            GET_C_DEX(k), k->base_stats.Dex, GET_C_INT(k), k->base_stats.Int,
            GET_WEIGHT(k));
    strcat(o_buf, buf);

    sprinttype(GET_ALT_SIZE(k), size_types, buf2);
    sprintf(buf,
            "&+YAgi: &n%3d&+Y (&n%3d&+Y)    Wis: &n%3d&+Y (&n%3d&+Y)    Size: &n%s&+Y\n",
            GET_C_AGI(k), k->base_stats.Agi, GET_C_WIS(k), k->base_stats.Wis,
            buf2);
    strcat(o_buf, buf);

    sprintf(buf,
            "&+YCon: &n%3d&+Y (&n%3d&+Y)    Cha: &n%3d&+Y (&n%3d&+Y)   Equipped Items: &n%3d&+Y     Carried weight:&n%5d\n",
            GET_C_CON(k), k->base_stats.Con, GET_C_CHA(k), k->base_stats.Cha,
            i3, IS_CARRYING_W(k));
    strcat(o_buf, buf);

    sprintf(buf,
            "&+YKar: &n%3d&+Y (&n%3d&+Y)    Luc: &n%3d&+Y (&n%3d&+Y)    Carried Items: &n%3d&+Y   Max Carry Weight:&n%5d\n",
            GET_C_KAR(k), k->base_stats.Karma, GET_C_LUK(k),
            k->base_stats.Luck, IS_CARRYING_N(k), CAN_CARRY_W(k));
    strcat(o_buf, buf);

    i = GET_C_STR(k) + GET_C_DEX(k) + GET_C_AGI(k) + GET_C_CON(k) +
      GET_C_POW(k) + GET_C_INT(k) + GET_C_WIS(k) + GET_C_CHA(k);

    i2 =
      k->base_stats.Str + k->base_stats.Dex + k->base_stats.Agi +
      k->base_stats.Con + k->base_stats.Pow + k->base_stats.Int +
      k->base_stats.Wis + k->base_stats.Cha;

    sprintf(buf,
            "&+YAvg: &n%3d&+Y (&n%3d&+Y)  Total mod: (&n%3d&+Y)              Load modifer: &n%3d\n\n",
            (int) (i / 8), (int) (i2 / 8), (i - i2), load_modifier(k));
    strcat(o_buf, buf);

/*
 * Print out NPC spell slot information: # spells left in each circle and # of
 * spells left to regain overall. - SKB 7 Apr 1995
 */

    if(IS_NPC(k) || IS_PUNDEAD(k) || GET_CLASS(k, CLASS_DRUID))
    {
      sprintf(buf, "&+mSpells left in circles:  (%d to regain)\n&+m",
              k->specials.undead_spell_slots[0]);
      strcat(o_buf, buf);

      for (i4 = 1; i4 < MAX_CIRCLE + 1; i4++)
      {
        sprintf(buf, "%d:%d/%d", i4, k->specials.undead_spell_slots[i4],
                spl_table[GET_LEVEL(k)][i4 - 1]);
        sprintf(o_buf + strlen(o_buf), "%-8s", buf);
      }
      strcat(o_buf, "\n\n");
    }
    sprintf(buf,
            "&+YHits: [&N%5d&+Y/&N%5d&+Y/&N%5d&+Y+&N%3d&+Y]&+W   Pcoins: &N%5d",
            GET_HIT(k), GET_MAX_HIT(k), k->points.base_hit, hit_regen(k),
            GET_PLATINUM(k));
    if(IS_PC(k))
      sprintf(buf, "%s  &+WPbank: &N%5d\n", buf, GET_BALANCE_PLATINUM(k));
    else
      sprintf(buf, "%-52s  &+YTimer: &N%d\n", buf, k->specials.timer);
    strcat(o_buf, buf);

    sprintf(buf,
            "&+YMana: [&N%5d&+Y/&N%5d&+Y/&N%5d&+Y+&N%3d&+Y]   Gcoins: &N%5d",
            GET_MANA(k), GET_MAX_MANA(k), k->points.base_mana, mana_regen(k),
            GET_GOLD(k));

    if(IS_PC(k))
      sprintf(buf, "%s  &+YGbank: &N%5d\n", buf, GET_BALANCE_GOLD(k));
    else
      sprintf(buf, "%-50s  &+YSpecial: &N%s\n", buf,
              (mob_index[GET_RNUM(k)].func.mob ?
               get_function_name((void*)mob_index[GET_RNUM(k)].func.mob) :
               "None"));
    strcat(o_buf, buf);

    sprintf(buf,
            "&+YMove: [&N%5d&+Y/&N%5d&+Y/&N%5d&+Y+&N%3d&+Y]&n   Scoins: %5d",
            GET_VITALITY(k), GET_MAX_VITALITY(k), vitality_limit(k),
            move_regen(k), GET_SILVER(k));
    if(IS_PC(k))
      sprintf(buf, "%s  &nSbank: %5d\n", buf, GET_BALANCE_SILVER(k));
    else
      sprintf(buf, "%-52s  &+YQuest: &N%s\n", buf,
              (mob_index[GET_RNUM(k)].qst_func ? "&+RExists" : "None"));
    strcat(o_buf, buf);

    sprintf(buf, "                                &+yCcoins: &N%5d",
            GET_COPPER(k));
    if(IS_PC(k))
      sprintf(buf, "%s  &+yCbank: &N%5d\n", buf, GET_BALANCE_COPPER(k));
    else
      strcat(buf, "\n");
    strcat(o_buf, buf);

//    i = calculate_ac(k, FALSE);
//    sprintf(buf, "&+cAgility Armor Class: &+Y%d&n  ", i);
//    strcat(o_buf, buf);

    i = calculate_ac(k);//, TRUE);  wipe 2011

    if(i > 0)
        sprintf(buf, "&+cTotal Armor Class: &+Y%d&n,  Increases melee damage by &+W%+.2f&n percent.\n", i, (double)(i * 0.10));
      else
        sprintf(buf, "&+cTotal Armor Class: &+Y%d&n,  Reduces melee damage by &+W%+.2f&n.\n", i, (double)(i * 0.10));

    strcat(o_buf, buf);

    i2 = calculate_thac_zero(k, 100); // Assumes 100 weapon skill.

    sprintf(buf, "&+Y thAC0: &N%d &+Y  +Hit: &N%d", i2,
            GET_HITROLL(k) + str_app[STAT_INDEX(GET_C_STR(k))].tohit);
    if(IS_NPC(k) ||
        (GET_CLASS(k, CLASS_MONK) &&
        !k->equipment[WIELD] &&
        !k->equipment[WEAR_SHIELD] &&
        !k->equipment[HOLD] &&
        !k->equipment[SECONDARY_WEAPON]))
    {
      sprintf(buf, "%s   &+YUnarmed damage: &N%d&+Yd&N%d  &+Y+Dam: &N%d\n",
              buf, k->points.damnodice, k->points.damsizedice,
              GET_DAMROLL(k) + str_app[STAT_INDEX(GET_C_STR(k))].todam);
    }
    else
    {
      sprintf(buf, "%s  &+Y+Dam: &N%d\n", buf,
              GET_DAMROLL(k) + str_app[STAT_INDEX(GET_C_STR(k))].todam);
    }
    strcat(o_buf, buf);

    strcat(o_buf, "&+YSaves:    Para   Wands  Fear   Breath Spell\n");
    sprintf(buf,
            "&+Y (actual) [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]\n",
            BOUNDED(1, (find_save(k, SAVING_PARA ) + k->specials.apply_saving_throw[0] * 5), 100),
            BOUNDED(1, (find_save(k, SAVING_ROD  ) + k->specials.apply_saving_throw[1] * 5), 100),
            BOUNDED(1, (find_save(k, SAVING_FEAR ) + k->specials.apply_saving_throw[2] * 5), 100),
            BOUNDED(1, (find_save(k, SAVING_BREATH) + k->specials.apply_saving_throw[3] * 5), 100),
            BOUNDED(1, (find_save(k, SAVING_SPELL) + k->specials.apply_saving_throw[4] * 5), 100));
    strcat(o_buf, buf);
    sprintf(buf,
            "&+Y (mods)   [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]\n",
            k->specials.apply_saving_throw[0],
            k->specials.apply_saving_throw[1],
            k->specials.apply_saving_throw[2],
            k->specials.apply_saving_throw[3],
            k->specials.apply_saving_throw[4]);
    strcat(o_buf, buf);

    strcat(o_buf, "\n");

    if(IS_PC(k))
    {
      if(k->desc)
        sprinttype(k->desc->connected, connected_types, buf2);
      else
        strcpy(buf2, "");
      sprintf(buf,
              "&+YHunger: &N%2d  &+YThirst: &N%2d  &+YDrunk: &N%2d &+Y%s%s\n",
              k->specials.conditions[FULL], k->specials.conditions[THIRST],
              k->specials.conditions[DRUNK],// k->only.pc->justice_level, /* &+YJustice Level: &N%d */  wipe2011
              (k->desc) ? "Connected: " : "Linkdead", buf2);
      strcat(o_buf, buf);
    }
    else
    {
      strcat(o_buf, "&+YValues: ");
      
      for(i4 = 0; i4 < NUMB_CHAR_VALS; i4++)
      {
        sprintf(buf, "&+Y[&n%d&+Y] ", k->only.npc->value[i4]);
        strcat(o_buf, buf);
      }      
      
      strcat(o_buf, "\n\n");
    }

    sprinttype(GET_POS(k), position_types, buf1);
    strcat(buf1, " ");
    sprintbit((GET_STAT(k) * 4), position_types, buf1 + strlen(buf1));
    if(IS_NPC(k))
    {
      sprinttype((k->only.npc->default_pos & 3), position_types, buf2);
      strcat(buf2, " ");
      sprintbit(((k->only.npc->default_pos & STAT_MASK) * 4),
                position_types, buf2 + strlen(buf2));
      sprintf(buf, "&+YPosition/Default: &N%s&+Y/&N%s", buf1, buf2);
    }
    else
      sprintf(buf, "&+YPosition: &N%s", buf1);
    sprintf(buf1, "%s  &+YFighting:&n %s", buf,
            ((k->specials.fighting) ? GET_NAME(k->specials.
                                               fighting) : "---"));
    if(IS_NPC(k))
    {
      strcat(buf1, "\n");
      strcpy(buf, buf1);
    }
    else
      sprintf(buf, "%-61s  &+YTimer: &N%d\n", buf1, k->specials.timer);
    strcat(o_buf, buf);

    if(IS_NPC(k))
    {
      sprintf(buf, "&+YJustice hooks: &N%d\n", k->only.npc->spec[2]);
      strcat(o_buf, buf);
      sprintbitde(k->specials.act, action_bits, buf2);
      sprintf(buf + strlen(buf), "&+YACT flags: &N%s\n", buf2);
      sprintbitde(k->specials.act2, action2_bits, buf2);
      sprintf(buf + strlen(buf),  "&+YACT2 flags: &N%s\n", buf2);
      sprintbitde(k->only.npc->aggro_flags, aggro_bits, buf2);
      sprintf(buf + strlen(buf), "&+YAggro    : &n%s\n", buf2);
      sprintbitde(k->only.npc->aggro2_flags, aggro2_bits, buf2);
      sprintf(buf + strlen(buf), "&+YAggro2   : &n%s\n", buf2);
      sprintbitde(k->only.npc->aggro3_flags, aggro3_bits, buf2);
      sprintf(buf + strlen(buf), "&+YAggro3   : &n%s\n", buf2);
      strcat(o_buf, buf);
    }
    else
    {
      sprintbit(k->only.pc->prompt, player_prompt, buf2);
      sprintf(buf, "&+YFlags (Prompt)      : &N%s\n", buf2);
      strcat(o_buf, buf);
      sprintbit(k->specials.act, player_bits, buf2);
      sprintf(buf, "&+YFlags (Specials Act): &N%s ", buf2);
      sprintbit(k->specials.act2, player2_bits, buf2);
      sprintbit(k->specials.act3, player3_bits, buf2);
      strcat(buf, buf2);
      strcat(buf, "\n");
      strcat(o_buf, buf);
      /* sprintbit(k->only.pc->law_flags, player_law_flags, buf2); */
      /* sprintf(buf, "&+YFlags (Player Flags): &N%s\n", buf2); */
      /* strcat(o_buf, buf); */
    }
    if(k->specials.affected_by)
    {
      sprintbitde(k->specials.affected_by, affected1_bits, buf2);
      sprintf(buf, "&+YAffected by (1): %u\n%s\n", k->specials.affected_by, buf2);
      strcat(o_buf, buf);
    }

    if(k->specials.affected_by2)
    {
      sprintbitde(k->specials.affected_by2, affected2_bits, buf2);
      sprintf(buf, "&+YAffected by (2): %u\n%s\n", k->specials.affected_by2, buf2);
      strcat(o_buf, buf);
    }

    if(k->specials.affected_by3)
    {
      sprintbitde(k->specials.affected_by3, affected3_bits, buf2);
      sprintf(buf, "&+YAffected by (3): %u\n%s\n", k->specials.affected_by3, buf2);
      strcat(o_buf, buf);
    }

    if(k->specials.affected_by4)
    {
      sprintbitde(k->specials.affected_by4, affected4_bits, buf2);
      sprintf(buf, "&+YAffected by (4): %u\n%s\n", k->specials.affected_by4, buf2);
      strcat(o_buf, buf);
    }

    if(k->specials.affected_by5)
    {
      sprintbitde(k->specials.affected_by5, affected5_bits, buf2);
      sprintf(buf, "&+YAffected by (5): %u\n%s\n", k->specials.affected_by5, buf2);
      strcat(o_buf, buf);
    }

    sprintf(buf,
            "&+YFollowers:           &+YMaster is: &N%s   &+YRank: &n%s\n",
            ((k->following) ? GET_NAME(k->following) : "---"),
            (k->group ? (IS_BACKRANKED(k) ? "Back" : "Front") : "---"));
    strcat(o_buf, buf);
    for (fol = k->followers; fol; fol = fol->next)
    {
      sprintf(buf, "  %s\n", IS_NPC(fol->follower) ?
              fol->follower->player.short_descr : GET_NAME(fol->follower));
      strcat(o_buf, buf);
    }

    if(IS_PC(k))
    {
      sprintf(buf, "&+YLanguages known:&n\n");
      for (i = 1; i <= TONGUE_GOD; i++)
      {
        sprintf(buf + strlen(buf), "&+Y%-12s&n%3d%% ",
                language_names[i - 1], GET_LANGUAGE(k, i));
        if(!(i % 4))
          strcat(buf, "\n");
      }
      if(TONGUE_GOD % 4)
        strcat(buf, "\n");
      strcat(o_buf, buf);
    }
    /* Show player on mobs piss list */
    if(IS_NPC(k) && IS_SET(k->specials.act, ACT_MEMORY))
    {
      sprintf(buf, "&+RPissed List&n:\n&+Y--------------&n\n");

      // rebuild this for new memory system
      mem = k->only.npc->memory;
      while (mem)
      {
        sprintf(buf2, "  %u\n", mem->pcID);
        strcat(buf, buf2);

        mem = mem->next;
      }

      strcat(buf, "\n");
      strcat(o_buf, buf);
    }
    if(k->affected)
    {
      strcat(o_buf, "&+YAffecting Spells:\n&+Y-----------------\n");
      for (aff = k->affected; aff; aff = aff->next)
      {
        if(aff->type == TAG_MEMORIZE)
        {
          sprintf(buf, "    MEMORIZED &+Yspell&n %s%s&n\n",
                  (aff->flags & AFFTYPE_CUSTOM1) ? "&+W" : "&+L",
                  skills[aff->modifier].name);
          strcat(o_buf, buf);
          continue;
        }

        sprintf(buf, "%13s &+Yby &N%4d &+Yfor &N%3d &+Yfrom &N'%s'\n",
                IS_SET(aff->flags, AFFTYPE_NOAPPLY) ? "NONE" :
                apply_types[(int) aff->location],
                aff->modifier,
                aff->duration,
                (skills[aff->type].name) ? skills[aff->type].name :
                (aff->type == SKILL_PERMINVIS ? "Obj Invis" : ""));
        *buf2 = '\0';

        if(aff->bitvector)
          sprintbitde(aff->bitvector, affected1_bits, buf2);

        if(aff->bitvector2)
        {
          sprintbitde(aff->bitvector2, affected2_bits, buf1);
          strcat(buf2, buf1);
        }

        if(aff->bitvector3)
        {
          sprintbitde(aff->bitvector3, affected3_bits, buf1);
          strcat(buf2, buf1);
        }

        if(aff->bitvector4)
        {
          sprintbitde(aff->bitvector4, affected4_bits, buf1);
          strcat(buf2, buf1);
        }

        if(aff->bitvector5)
        {
          sprintbitde(aff->bitvector5, affected5_bits, buf1);
          strcat(buf2, buf1);
        }

        if(*buf2 != '\0' && !IS_SET(aff->flags, AFFTYPE_NOAPPLY))
        {
          sprintf(buf, "%-61s &+YSets: &N%s\n", buf, buf2);
        }
        else
          strcat(buf, "\n");
        strcat(o_buf, buf);
      }
    }
    if(k->events || k->nevents)
    {
      P_nevent ne;
      strcat(o_buf, "&+YEvents:\n&+Y-------\n");

      for (e1 = k->events; e1; e1 = e1->next)
      {
        sprintf(o_buf + strlen(o_buf),
                "%6d&+Y seconds,&n %s&+Y.\n",
                event_time(e1, T_SECS), get_event_name(e1));
      }
      for (ne = k->nevents; ne; ne = ne->next)
      {
        sprintf(o_buf + strlen(o_buf),
                "%6d&+Y seconds,&n %s&+Y.\n",
                ne_event_time(ne)/WAIT_SEC, get_function_name((void*)ne->func));
      }
      strcat(o_buf, "\n");
    }

    if(k->linking || k->linked)
    {
      struct char_link_data *link;

      strcat(o_buf, "&+YLinks:\n&+Y-------\n");
      for (link = k->linking; link; link = link->next_linking)
      {
        sprintf(o_buf + strlen(o_buf),
                "%s (%s): &+Ylinked to&n %s.\n",
                link_types[link->type].name,
                "master", link->linked->player.name);
      }
      for (link = k->linked; link; link = link->next_linked)
      {
        sprintf(o_buf + strlen(o_buf),
                "%s (%s): &+Ylinked to&n %s.\n",
                link_types[link->type].name,
                "slave", link->linking->player.name);
      }
      strcat(o_buf, "\n");
    }
    if(IS_PC(k))
    {
      strcat(o_buf, "&+YGuild:\n&+Y-------\n");

      if(GET_TIME_LEFT_GUILD(k) > 0)
      {
        timestr = asctime(localtime(&(GET_TIME_LEFT_GUILD(k))));
        *(timestr + 10) = 0;
        strcpy(time_left, timestr);
      }
      else
      {
        strcpy(time_left, "None");
      }
      sprintf(buf, "&+YDate left last guild : &N%s\n", time_left);
      strcat(o_buf, buf);
      sprintf(buf, "&+YNumber of guild left : &N%d\n", GET_NB_LEFT_GUILD(k));
      strcat(o_buf, buf);
      strcat(o_buf, "\n");
    }
    if(IS_PC(k) && IS_DISGUISE(k))
    {
      strcat(o_buf, "&+YDisguise:\n&+Y-------\n");
      sprintf(buf, "&+mDisguise as : &N%s\n", k->disguise.name);
      strcat(o_buf, buf);
    }
    page_string(ch->desc, o_buf, 1);
    if(t_mob)
    {
      extract_char(t_mob);
      t_mob = NULL;
    }
    /* Trap data. Rather than clog do_stat anymore, we'll just pass info on */
  }
  else if((*arg1 == 't') || (*arg1 == 'T') &&
    (arg1[1] == 'r') || (arg1[1] == 'R'))
  {
    do_trapstat(ch, arg2, 0);
  }
  // old guildhalls (deprecated)
//  else if((*arg1 == 'h') || (*arg1 == 'H'))
//  {
//    do_stathouse(ch, arg2, 0);
//  }
  else if((*arg1 == 's') || (*arg1 == 'S'))
  {

    /* shop data on a mobile in world, similar to statting a mobile,
       but gives info on the shop proc, rather than the mob.  Due to
       the way things are setup, the mob must have been loaded by a
       zone command at bootup (or the shop is not setup properly).
       Even if mob has been killed/purged, you can stat-by-number. */

    if(!*arg2)
    {
      send_to_char(STAT_SYNTAX, ch);
      return;
    }
    if(is_number(arg2))
    {
      if((i = real_mobile0(atoi(arg2))) == 0)
      {
        send_to_char("Illegal mob number.\n", ch);
        return;
      }
      /* load one to stat, extract after statting  */
      t_mob = read_mobile(i, REAL);

      if(!t_mob)
      {
        logit(LOG_DEBUG, "do_stat(): mob %d [%d] not loadable", i,
              mob_index[i].virtual_number);
        send_to_char("error loading mob to stat.\n", ch);
        return;
      }
      else
      {
        char_to_room(t_mob, 0, -2);
      }

      if(t_mob->player.name)
        strcpy(arg2, t_mob->player.name);
    }
    i2 = FALSE;
    if(!(k = get_char_vis(ch, arg2)) || !IS_NPC(k))
    {
      send_to_char("No such character.\n", ch);
      i2 = TRUE;
    }
    else
    {
      for( i = 0; (i < number_of_shops) && (shop_index[i].keeper != GET_RNUM(k)); i++ )
      {
        ;
      }
      if(((mob_index[GET_RNUM(k)].func.mob != shop_keeper) &&
           (mob_index[GET_RNUM(k)].qst_func != shop_keeper)) ||
          (i >= number_of_shops))
      {
        send_to_char("No shop data for this mob.\n", ch);
        i2 = TRUE;
      }
    }

    if(i2)
    {
      if(t_mob)
      {
        extract_char(t_mob);
        t_mob = NULL;
      }
      return;
    }
    if(shop_index[i].shop_new_options)
    {
      num_tr = shop_index[i].number_types_traded;
      num_pr = shop_index[i].number_items_produced;
    }
    else
    {
      for (i2 = 0, num_tr = 0, num_pr = 0; i2 < 5; i2++)
      {
        if(SHOP_BUYTYPE(i, i2) != -1)
          num_tr++;
        if(shop_index[i].producing[i2] != -1)
          num_pr++;
      }
    }
    sprintf(o_buf, "&+Y%s %sShop, Number: &N%d&+Y  for [&N%d&+Y](&n%d&+Y)&N %s\n\n",
      shop_index[i].shop_new_options ? "New" : "Old",
      shop_index[i].shop_is_roaming ? "Roaming " : "", i,
      mob_index[GET_RNUM(k)].virtual_number, GET_RNUM(k),
      k->player.short_descr ? k->player.short_descr : "&+rNone");
    sprintf(o_buf + strlen(o_buf), "&+YHours: &N%d&+Y-&N%d&+Y,&N %d&+Y-&N%d  &+YAttackable?: %c  Allow Casting?: %c\n",
      shop_index[i].open1, shop_index[i].close1, shop_index[i].open2,
      shop_index[i].close2, shop_index[i].shop_killable ? 'Y' : 'N',
      shop_index[i].magic_allowed ? 'Y' : 'N');
    sprintf(o_buf + strlen(o_buf), "&+YBuys for: &N%d%%&+Y, Sells for: &N%d%%&+Y, Produces &N%d &+YItems, Trades in &N%d &+YTypes\n",
      (int) (shop_index[i].buy_percent * 100),
      (int) (shop_index[i].sell_percent * 100), num_pr, num_tr);

    /* various messages that shop has stored. */

    sprintf(o_buf + strlen(o_buf), "&+YRacist       :&N %s\n",
      shop_index[i].racist_message ? shop_index[i].racist_message : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YOpening      :&N %s\n",
      shop_index[i].open_message ? shop_index[i].open_message : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YClosing      :&N %s\n",
      shop_index[i].close_message ? shop_index[i].close_message : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YDon't have   :&N %s\n",
      shop_index[i].no_such_item1 ? shop_index[i].no_such_item1 : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YCh don't have:&N %s\n",
      shop_index[i].no_such_item2 ? shop_index[i].no_such_item2 : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YToo poor     :&N %s\n",
      shop_index[i].missing_cash1 ? shop_index[i].missing_cash1 : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YCh too poor  :&N %s\n",
      shop_index[i].missing_cash2 ? shop_index[i].missing_cash2 : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YWrong Type   :&N %s\n",
      shop_index[i].do_not_buy ? shop_index[i].do_not_buy : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YSOLD!        :&N %s\n",
      shop_index[i].message_buy ? shop_index[i].message_buy : "&+R<NONE>");
    sprintf(o_buf + strlen(o_buf), "&+YBought       :&N %s\n",
      shop_index[i].message_sell ? shop_index[i].message_sell : "&+R<NONE>");

    strcat(o_buf, "\n&+YItems traded: &N");
    for (i2 = 0;
         (i2 < shop_index[i].number_types_traded) && SHOP_BUYTYPE(i, i2);
         i2++)
    {
      strcat(o_buf, item_types[(int) SHOP_BUYTYPE(i, i2)]);
      strcat(o_buf, " ");
    }
/**** Out for a bit ***
    for (i2 = 0; SHOP_BUYTYPE(i, i2) != NOTHING; i2++) {
      if(i2)
        strcat(buf, ", ");
      sprintf(buf1, "%s (#%d) ", item_types[SHOP_BUYTYPE(i, i2)],
              SHOP_BUYTYPE(i, i2));
      if(SHOP_BUYWORD(i, i2))
        sprintf(END_OF(buf1), "[%s]", SHOP_BUYWORD(i, i2));
      else
        strcat(buf1, "[all]");
      strcat(o_buf
             }
****/

    strcat(o_buf, "\n\n&+YItems produced:\n");

    for (i2 = 0; i2 < shop_index[i].number_items_produced; i2++)
    {
      if(shop_index[i].producing[i2] != -1)
      {
        if((t_obj = read_object(shop_index[i].producing[i2], REAL)))
        {
          m_virtual =
            (t_obj->R_num >= 0) ? obj_index[t_obj->R_num].virtual_number : 0;

          sprintf(o_buf + strlen(o_buf),
                  "&+Y[&N%5d&+Y] (&N%5d&+Y)&N %12s %s\n", m_virtual,
                  t_obj->R_num, item_types[(int) t_obj->type],
                  ((t_obj->short_description) ? t_obj->
                   short_description : "None"));
          extract_obj(t_obj, FALSE);
        }
        else
        {
          logit(LOG_DEBUG, "do_stat(): obj %d [%d] not loadable (shop stat)",
                shop_index[i].producing[i2],
                obj_index[shop_index[i].producing[i2]].virtual_number);
          sprintf(o_buf + strlen(o_buf), "&+RNon-existant object: &N%d\n",
                  shop_index[i].producing[i2]);
        }
      }
    }

    page_string(ch->desc, o_buf, 1);
    if(t_mob)
    {
      extract_char(t_mob);
      t_mob = NULL;
    }
  }
  else if((*arg1 == 'd') || (*arg1 == 'D'))
  {
    stat_dam(ch, arg2);
    send_to_char("\n", ch);
    stat_spldam(ch, arg2);
  }
  else if((*arg1 == 'q') || (*arg1 == 'Q'))
  {
    if( !(mob = get_char_vis(ch, arg2)) || !IS_NPC(mob) )
    {
      sprintf( buf, "'%s' not found or is not a NPC.\n", arg2 );
      send_to_char( buf, ch );
      return;
    }
    int qi = find_quester_id(GET_RNUM(mob));
    struct quest_complete_data *qdata = quest_index[qi].quest_complete;
    struct goal_data *goals;

    if( !qdata )
    {
      sprintf( buf, "'%s' is not a quest complete mob.\n", mob->player.short_descr );
      send_to_char( buf, ch );
    }
    else
    {

      sprintf( buf, "'%s' has quest:\n", mob->player.short_descr );
      send_to_char( buf, ch );

      while( qdata )
      {
        sprintf( buf, "'%s'\n", qdata->message );
        send_to_char( buf, ch );
        if( qdata->receive )
        {
          for( goals = qdata->receive; goals; goals = goals->next )
          {
            sprintf( buf, "Receive: '%c' %d\n", goals->goal_type, goals->number );
            send_to_char( buf, ch );
          }
        }
        if( qdata->give )
        {
          for( goals = qdata->give; goals; goals = goals->next )
          {
            sprintf( buf, "Give: '%c' %d\n", goals->goal_type, goals->number );
            send_to_char( buf, ch );
          }
        }

        qdata = qdata->next;
      }
    }
  }
  else
    send_to_char(STAT_SYNTAX, ch);
}

#undef STAT_SYNTAX

void do_echoa(P_char ch, char *argument, int cmd)
{
  P_desc   d;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      level;

  if(IS_NPC(ch))
    return;

  while (*argument == ' ' && *argument != '\0')
    argument++;

  if(!*argument)
    send_to_char("Yes, fine, we must echoa something, but what!?\n", ch);
  else
  {
    level = GET_LEVEL(ch);
    
    if(get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s echoa's '%s'", GET_NAME(ch), argument);
    
    strcat(argument, "\n");
    
    for (d = descriptor_list; d; d = d->next)
    {
      if(d->connected == CON_PLYNG)
      {
        if(GET_LEVEL(d->character) >= level)
        {
          sprintf(Gbuf1, "A[%s]", GET_NAME(ch));
          send_to_char(Gbuf1, d->character);
        }
	send_to_char(argument, d->character);
        
        write_to_pc_log(d->character, argument, LOG_PRIVATE);
      }
    }
  }
}


void do_echoz(P_char ch, char *arg, int cmd)
{
  P_desc   d;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      level;

  if(IS_NPC(ch))
    return;

  while (*arg == ' ' && *arg != '\0')
    arg++;

  if(!*arg)
  {
    send_to_char("Yes, fine, we must echoz something, but what?!\n", ch);
    return;
  }
  else
  {
    level = GET_LEVEL(ch);
    
    if(get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s echoz's '%s'", GET_NAME(ch), arg);
    
    strcat(arg, "\n");
    
    for (d = descriptor_list; d; d = d->next)
    {
      if(d->connected == CON_PLYNG)
      {
        if(world[ch->in_room].zone == world[d->character->in_room].zone)
        {
          if(GET_LEVEL(d->character) >= level)
          {
            sprintf(Gbuf1, "Z[%s]", GET_NAME(ch));
            send_to_char(Gbuf1, d->character);
          }
          send_to_char(arg, d->character);
          write_to_pc_log(d->character, arg, LOG_PRIVATE);
        }
      }
    }
  }
  return;
}

void do_echog(P_char ch, char *arg, int cmd)
{
  P_desc   d;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      level;

  if(IS_NPC(ch))
    return;

  while (*arg == ' ' && *arg != '\0')
    arg++;

  if(!*arg)
  {
    send_to_char
      ("Yes, fine, we must inform the goods of something, but what?!\n", ch);
    return;
  }
  else
  {
    level = GET_LEVEL(ch);
    
    if(get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s echog's '%s'", GET_NAME(ch), arg);
    
    strcat(arg, "\n");
    
    for (d = descriptor_list; d; d = d->next)
    {
      if(d->connected == CON_PLYNG)
      {
        if(RACE_GOOD(d->character) || IS_TRUSTED(d->character))
        {
          if(GET_LEVEL(d->character) >= level)
          {
            sprintf(Gbuf1, "G[%s]", GET_NAME(ch));
            send_to_char(Gbuf1, d->character);
          }
          send_to_char(arg, d->character);
          write_to_pc_log(d->character, arg, LOG_PRIVATE);
        }
      }
    }
  }
  return;
}

void do_echoe(P_char ch, char *arg, int cmd)
{
  P_desc   d;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      level;

  if(IS_NPC(ch))
    return;

  while (*arg == ' ' && *arg != '\0')
    arg++;

  if(!*arg)
  {
    send_to_char
      ("Yes, fine, we must inform the evils of something, but what?!\n", ch);
    return;
  }
  else
  {
    level = GET_LEVEL(ch);
    
    if(get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s echoe's '%s'", GET_NAME(ch), arg);
    
    strcat(arg, "\n");
    
    for (d = descriptor_list; d; d = d->next)
    {
      if(d->connected == CON_PLYNG)
      {
        if((EVIL_RACE(d->character) || IS_TRUSTED(d->character)) &&
            (!(RACE_PUNDEAD(d->character))))
        {
          if(GET_LEVEL(d->character) >= level)
          {
            sprintf(Gbuf1, "E[%s]", GET_NAME(ch));
            send_to_char(Gbuf1, d->character);
          }
          send_to_char(arg, d->character);
          write_to_pc_log(d->character, arg, LOG_PRIVATE);
        }
      }
    }
  }
  return;
}

void do_echou(P_char ch, char *arg, int cmd)
{
  P_desc   d;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      level;

  if(IS_NPC(ch))
    return;

  while (*arg == ' ' && *arg != '\0')
    arg++;

  if(!*arg)
  {
    send_to_char
      ("Yes, fine, we must inform the undead of something, but what?!\n", ch);
    return;
  }
  else
  {
    level = GET_LEVEL(ch);
    
    if(get_property("logs.chat.status", 0.000) && IS_PC(ch))
      logit(LOG_CHAT, "%s echou's '%s'", GET_NAME(ch), arg);
    
    strcat(arg, "\n");
    
    for (d = descriptor_list; d; d = d->next)
    {
      if(d->connected == CON_PLYNG)
      {
        if(RACE_PUNDEAD(d->character) || IS_TRUSTED(d->character))
        {
          if(GET_LEVEL(d->character) >= level)
          {
            sprintf(Gbuf1, "U[%s]", GET_NAME(ch));
            send_to_char(Gbuf1, d->character);
          }
          send_to_char(arg, d->character);
          write_to_pc_log(d->character, arg, LOG_PRIVATE);
        }
      }
    }
  }
  return;
}

void do_shutdow(P_char ch, char *argument, int cmd)
{
  send_to_char("If you want to shut something down - say so!\n", ch);
}

void do_wizlock(P_char ch, char *arg, int cmd)
{
  P_desc   d;
  char     buf[MAX_STRING_LENGTH];
  char     buf1[MAX_STRING_LENGTH];

  if(!*arg)
  {
    send_to_char("Usage: wizlock <create | connections | maxplayers>\n", ch);
    return;
  }
  one_argument(arg, buf1);

  if(is_abbrev(buf1, "create"))
  {
    if(IS_SET(game_locked, LOCK_CREATE))
    {
      REMOVE_BIT(game_locked, LOCK_CREATE);
      sprintf(buf,
              "&+GDuris DikuMUD ->&n Restrictions on new character creations lifted.\n");
    }
    else
    {
      SET_BIT(game_locked, LOCK_CREATE);
      sprintf(buf,
              "&+RDuris DikuMUD ->&n Game is being locked;  no more character creation.\n");
    }
  }
  else if(is_abbrev(buf1, "connections"))
  {
    if(IS_SET(game_locked, LOCK_CONNECTIONS))
    {
      REMOVE_BIT(game_locked, LOCK_CONNECTIONS);
      sprintf(buf,
              "&+GDuris DikuMUD ->&n Restrictions on new connections lifted.\n");
    }
    else
    {
      SET_BIT(game_locked, LOCK_CONNECTIONS);
      sprintf(buf,
              "&+RDuris DikuMUD ->&n Game is being locked; no more connections.\n");
    }
  }
  else if(is_abbrev(buf1, "maxplayers"))
  {
    if(IS_SET(game_locked, LOCK_MAX_PLAYERS))
    {
      REMOVE_BIT(game_locked, LOCK_MAX_PLAYERS);
      sprintf(buf,
              "&+GDuris DikuMUD ->&n Restrictions on max players lifted.\n");
    }
    else
    {
      SET_BIT(game_locked, LOCK_MAX_PLAYERS);
      sprintf(buf,
              "&+RDuris DikuMUD ->&n Game is limited to a MAX of %d players\n",
              MAX_PLAYERS_BEFORE_LOCK);
    }
  }
  else
  {
    send_to_char("Usage: wizlock <create | connections | maxplayers>.\n", ch);
    return;
  }

  for (d = descriptor_list; d; d = d->next)
  {
    if(!d->connected)
    {
      send_to_char(buf, d->character);
    }
  }
}

void do_nchat(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  int      goodie = 0;
  int      evil = 0;
  int      undead = 0;
  int      all = 0;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( !IS_SET(ch->specials.act2, PLR2_NCHAT) )
  {
    send_to_char("Newbie chat is turned off, type \"tog nchat\" to turn it on.\n", ch);
    return;
  }

  if( IS_ILLITHID(ch) && !IS_TRUSTED(ch) )
  {
    send_to_char("If you need this channel, you shouldn't be playing this character.\n", ch);
    return;
  }

  if( IS_AFFECTED2(ch, AFF2_SILENCED) )
  {
    send_to_char("You move your lips, but no sound comes forth!\n", ch);
    return;
  }

  if( is_silent(ch, TRUE) )
  {
    return;
  }
/*
  if((GET_LEVEL(ch) > 31) &&
     (GET_LEVEL(ch) < 57) &&
     !IS_SET(PLR2_FLAGS(ch), PLR2_NEWBIE_GUIDE))
  {
   send_to_char("Your level no longer qualifies you for newbie-chat, sorry.\r\n", ch);
   return;
  }
*/
  if( IS_DISGUISE_PC(ch) || IS_DISGUISE_ILLUSION(ch) || IS_DISGUISE_SHAPE(ch) )
  {
    send_to_char("&+WYou are not in your true shape!\r\n", ch);
    return;
  }

  while (*argument == ' ' && *argument != '\0')
  {
    argument++;
  }

  if( !*argument )
  {
    send_to_char("Thats right nchat and then add something else, for example: \"nchat how do I kill things?\"\n", ch);
    return;
  }
  else if( ch->desc )
  {

    if(IS_TRUSTED(ch))
    {
      if(((*argument == 'g') || (*argument == 'G')) &&
          (*(argument + 1) == ' '))
        goodie = 1;
      else if(((*argument == 'e') || (*argument == 'E')) &&
               (*(argument + 1) == ' '))
        evil = 1;
      else if(((*argument == 'u') || (*argument == 'U')) &&
               (*(argument + 1) == ' '))
        undead = 1;
      else if(((*argument == 'a') || (*argument == 'A')) &&
               (*(argument + 1) == ' '))
      {
        undead = 1;
        evil = 1;
        goodie = 1;
        all = 1;
      }
      else
      {
        send_to_char("&+YMake up your mind first which side do you want to help. Use nchat 'e', 'u', 'g' or 'a'. &n\n", ch);
        return;
      }
      argument += 2;

      if( all == 1 )
        sprintf(Gbuf2, "&+C*all* &n");
      else if(goodie == 1)
        sprintf(Gbuf2, "&+Wgoodie &n");
      else if(evil == 1)
        sprintf(Gbuf2, "&+Revil &n");
      else if(undead == 1)
        sprintf(Gbuf2, "&+Lundead &n");
      else
        sprintf(Gbuf2, "undefined ");


      sprintf(Gbuf1, "&+mYou racewar chat to %s'&+w%s&n&+w'\n", Gbuf2, argument);
      send_to_char(Gbuf1, ch, LOG_PRIVATE);
    }
    else if(IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch))
    {
      sprintf(Gbuf1, "&+mYou tell your racewar '&+W%s&n&+w'\n", argument);
      send_to_char( Gbuf1, ch, LOG_PRIVATE );
    }
    else
      send_to_char("Ok.\n", ch);
  }

  if(!IS_TRUSTED(ch))
  {
    if( RACE_GOOD(ch) )
    {
      sprintf(Gbuf2, "&+Wgood&n");
    }
    else if(RACE_EVIL(ch))
    {
      sprintf(Gbuf2, "&+Revil&n");
    }
    else if(RACE_PUNDEAD(ch))
    {
      sprintf(Gbuf2, "&+Lundead&n");
    }
    else
    {
      sprintf(Gbuf2, "&-Rundefined&n");
    }
  }

  for( i = descriptor_list; i; i = i->next )
  {
    if( i->connected )
    {
      continue;
    }
    if( ((racewar(ch, i->character) && !IS_TRUSTED(ch)) || (!all && evil && !RACE_EVIL(i->character))
      || (!all && undead && !RACE_PUNDEAD(i->character)) || (!all && goodie && !RACE_GOOD(i->character)))
      || i->character == ch )
    {
      continue;
    }
    if( !IS_SET(i->character->specials.act2, PLR2_NCHAT) )
    {
      continue;
    }
    if( i->character->only.pc->ignored == ch )
    {
      continue;
    }
    if( IS_DISGUISE(i->character) || IS_DISGUISE_ILLUSION(i->character) || IS_DISGUISE_SHAPE(i->character) )
    {
      continue;
    }
    if( IS_TRUSTED(i->character) )
    {
      sprintf(Gbuf1, "&+W%s&n&+m racewar-chats &+w(%s&+w): '&+Y%s&n&+w'\n",
        PERS(ch, i->character, FALSE), Gbuf2, language_CRYPT(ch, i->character, argument));
    }
    else
    {
      sprintf(Gbuf1, "&+W%s&n&+m tells your racewar: &+w'&+Y%s&n&+w'\n",
        PERS(ch, i->character, FALSE), language_CRYPT(ch, i->character, argument));
    }
    send_to_char(Gbuf1, i->character, LOG_PRIVATE);
  }
  if( get_property("logs.chat.status", 0.000) && IS_PC(ch) )
  {
    logit(LOG_CHAT, "%s newb chat's (%s) '%s'", GET_NAME(ch), Gbuf2, argument);
  }
}

void do_wizmsg(P_char ch, char *arg, int cmd)
{
  P_desc   d;
  char     Gbuf1[MAX_STRING_LENGTH], color[4];
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
  int      min_level = 0, flag = 0;

  if(!ch || IS_NPC(ch) || !IS_TRUSTED(ch))
    return;

  if(IS_SET(ch->specials.act, PLR_WIZMUFFED))
  {
    send_to_char("You can't hear wiz messages.\n", ch);
    return;
  }
  if(!*arg)
  {
    send_to_char("Yes, yes, but WHAT do you want to tell them all?\n", ch);
    return;
  }
  half_chop(arg, Gbuf1, Gbuf2);

  if(is_number(Gbuf1))
  {
    flag = 1;
    min_level = atoi(Gbuf1);
    if((min_level < AVATAR) || (min_level > 62))
    {
      min_level = AVATAR;
      flag = 0;
    }
  }
  if(!flag)
  {
    strcat(Gbuf1, " ");
    strcat(Gbuf1, Gbuf2);
  }
  else
  {
    if(Gbuf2)
      strcpy(Gbuf1, Gbuf2);
  }

  min_level = BOUNDED(AVATAR, min_level, 62);
  if(min_level > AVATAR)
  {
     sprintf(color, "&+R");
  }
  else
  {
     sprintf(color, "&+r");
  }
  /* FOR visible  */

  sprintf(Gbuf2, "%s : (%s%d&n) [ ", GET_NAME(ch), color, min_level);
  strcat(Gbuf2, Gbuf1);
  strcat(Gbuf2, " &n]\n");

  /* For invisible  */

  sprintf(Gbuf3, "Someone : (%s%d&n) [ ", color, min_level);
  strcat(Gbuf3, Gbuf1);
  strcat(Gbuf3, " &n]\n");

  for (d = descriptor_list; d; d = d->next)
  {
    if((d->connected == CON_PLYNG) &&
        (GET_LEVEL(d->character) >= min_level) &&
        !IS_SET(d->character->specials.act, PLR_WIZMUFFED))
    {

      if(CAN_SEE(d->character, ch))
        send_to_char(Gbuf2, d->character, LOG_PRIVATE);
      else
        send_to_char(Gbuf3, d->character, LOG_PRIVATE);
    }
  }
  if(get_property("logs.chat.status", 0.000) && IS_PC(ch))
    logit(LOG_CHAT, "%s wizmsg's '%s'", GET_NAME(ch), Gbuf1);
}

TimedShutdownData shutdownData = {0, -1, TimedShutdownData::NONE};


void timedShutdown(P_char ch, P_char, P_obj, void *data)
{
  // timed shutdown event.  ch is the god who initiated the shutdown.
  //  data refers to the shutdown timer and shutdown type

  if(shutdownData.eShutdownType == TimedShutdownData::NONE)
  {  // silently return (without setting a new event)
    return;
  }

  char buf[500];
  // round a bit due to floating point errors...
  if(shutdownData.reboot_time == 0)
  {
    // perform the shutdown...
    switch (shutdownData.eShutdownType)
    {
      case TimedShutdownData::OK:
        sprintf(buf, "\r\n%s grabs Duris by the balls and rips them off.\r\n", shutdownData.IssuedBy);
        send_to_all(buf);
        logit(LOG_STATUS, buf);
        sql_log(ch, WIZLOG, buf);
        shutdownflag = 1;
        break;
      case TimedShutdownData::REBOOT:
        sprintf(buf, "\r\n%s shreds the world around you.\r\n", shutdownData.IssuedBy);
        send_to_all(buf);
        logit(LOG_STATUS, buf);
        sql_log(ch, WIZLOG, buf);
        shutdownflag = _reboot = 1;
        break;

      case TimedShutdownData::COPYOVER:
        sprintf(buf, "\r\n%s destroys the world as you know it.\r\n", shutdownData.IssuedBy);
        send_to_all(buf);
        logit(LOG_STATUS, buf);
        sql_log(ch, WIZLOG, buf);
        shutdownflag = _copyover = 1;
        break;

      case TimedShutdownData::AUTOREBOOT:
        sprintf(buf, "\r\nDuris fades into nothing, as the world begins its reconstruction...\r\n", shutdownData.IssuedBy);
        send_to_all(buf);
        logit(LOG_STATUS, buf);
        sql_log(ch, WIZLOG, buf);
        shutdownflag = _autoboot = 1;
        break;

      case TimedShutdownData::PWIPE:
        // Dunno why, but send_to_all isn't color coding here, maybe term type is erased in database or such? :(
        sprintf(buf, "\r\n\033[1;5;44mDuris begins to fade into nothing.. So do you.. \033[0m\r\n\033[1;5;44mThis is really the end!!!\033[0m\r\n\033[1;5;44m............\033[0m\r\n\033[1;5;44m........\033[0m\r\n\033[1;5;44m......\033[0m\r\n\033[1;5;44m...\033[0m\r\n\033[1;5;44m.\033[0m\n\r", shutdownData.IssuedBy);
        send_to_all(buf);
        logit(LOG_STATUS, "Shutdown pwipe called.");
        if( !sql_pwipe( 1723699 ) )
        {
          send_to_all( "&=GlSQL database not wiped clean.. Aborting shutdown wipe.&n\n\r&+WYou're still alive!  Yay!&n\n\r" );
          return;
        }
        else
        {
          logit(LOG_STATUS, "Successful wipe of SQL stuff." );
        }
        shutdownflag = _pwipe = 1;
        break;

      default:
        wizlog(60, "WARNING:  Unknown shutdown type ABORTED!!");
        return;
    }
    shutdown_message = str_dup(buf);
  }
  else
  {
    P_char ch = NULL;
    // find the god doing the shutdown.  If he/she loses link or logs off, the reboot will
    // be cancelled.
    for (P_desc d = descriptor_list; d; d = d->next)
    {
      if(d->character && isname(GET_NAME(d->character), shutdownData.IssuedBy))
      {
        ch = d->character;
        break;
      }
    }
 /*   if(!ch)
    {
      // CANCEL the reboot.
      shutdownData.eShutdownType = TimedShutdownData::NONE;
      send_to_all("&+R*** Scheduled Reboot Cancelled ***&n\n");
      sprintf(buf, "Scheduled reboot cancelled: unable to locate %s", shutdownData.IssuedBy);
      wizlog(60, buf);
      return;
    }
*/
    // how much longer until a reboot?
    time_t secs = shutdownData.reboot_time - time(0);
    // special case:  going to reboot in less then ~1 second - force a restore all.
    if((secs <= 1) && (ch != NULL))
    {
      // do restoreall here...
      int old_level = GET_LEVEL(ch);
      ch->player.level = MAXLVL;
      do_restore(ch, " all", CMD_RESTORE);
      ch->player.level = old_level;

      // setting the reboot_time to 0 forces the reboot to occur next event
      shutdownData.reboot_time = 0;
      secs *= WAIT_SEC;
      if(!secs) secs = 1;
      add_event(timedShutdown, secs, NULL, NULL, NULL, 0, NULL, 0);
      return;
    }
    else if(secs <= 1)
    {
      // no restore since no ch.
      // setting the reboot_time to 0 forces the reboot to occur next event
      shutdownData.reboot_time = 0;
      secs *= WAIT_SEC;
      if(!secs) secs = 1;
      add_event(timedShutdown, secs, NULL, NULL, NULL, 0, NULL, 0);
      return;
    }
    // if the next warning timer isn't set, set it now
    if(-1 == shutdownData.next_warning)
      shutdownData.next_warning = secs;

    // okay, see if a warning should be displayed...
    if(secs <= shutdownData.next_warning)
    {
      const char *type = "REBOOT";
      if( shutdownData.eShutdownType == TimedShutdownData::OK
        || shutdownData.eShutdownType == TimedShutdownData::PWIPE )
      {
        type = "SHUTDOWN";
      }
      if(secs > 60)
//        sprintf(buf, "&+R*** Scheduled %s in %d minutes ***&n\n", type, secs/60, shutdownData.IssuedBy);
        sprintf(buf, "&+R*** Scheduled %s in %ld minutes ***&n\n", type, secs/60);
      else
        sprintf(buf, "&+R*** Scheduled &-L%s&n&+R in %ld seconds ***&n\n", type, secs);
      send_to_all(buf);
      // and set when the next warning should occur..

      if(secs <= 15)
      {
        shutdownData.next_warning -= 5;
      }
      else if(secs <= 60)
      {  // <= 1 minute - warn every 15 seconds
        shutdownData.next_warning -= 15;
      }
      else if(secs <= (5 * 60))
      {  // <= 5 minutes - warn every 1 minute
        shutdownData.next_warning -= 60;
      }
      else if(secs <= (15 * 60))
      {  // <= 15 minutes - warn every 2 minutes
        shutdownData.next_warning -= 120;
      }
      else
      {  // > 15 minutes, warn every 5 minutes
        shutdownData.next_warning -= (5 * 60);
      }
    }
    add_event(timedShutdown, WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
  }
}

void displayShutdownMsg(P_char ch)
{
  // send_to_char() ch any pending reboot/shutdown message...
  if(shutdownData.eShutdownType == TimedShutdownData::NONE)
    return;

  char buf[200];
  time_t secs = shutdownData.reboot_time - time(0);
  const char *type = "REBOOT";
  if( shutdownData.eShutdownType == TimedShutdownData::OK
    || shutdownData.eShutdownType == TimedShutdownData::PWIPE )
  {
    type = "SHUTDOWN";
  }

  if(secs > 60)
    sprintf(buf, "&+R*** Scheduled %s in %ld minute%s***&n\n", type, secs/60, (secs >= 120) ? "s " : " ");
  else
    sprintf(buf, "&+R*** Scheduled &-L%s&n&+R in %ld seconds ***&n\n", type, secs);
  send_to_char(buf, ch);
}

void do_shutdown(P_char ch, char *argument, int cmd)
{
  char     buf[100], arg[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  argument = one_argument(argument, arg);

  int mins_to_reboot = 0;
  const char *type = "reboot";
  if( shutdownData.eShutdownType == TimedShutdownData::OK
    || shutdownData.eShutdownType == TimedShutdownData::PWIPE )
  {
    type = "shutdown";
  }
  if(argument)
  {
    while (isspace(*argument))
      argument++;
    if(argument[0])
    {
      mins_to_reboot = atol(argument);
    }
  }

  if( shutdownData.eShutdownType == TimedShutdownData::AUTOREBOOT )
  {
    return; //AutoREboots cannot be cycled.
  }

  // if there is a pending shutdown, cancel it now...
  if((shutdownData.eShutdownType != TimedShutdownData::NONE))
  {
    sprintf(buf, "&+R*** Scheduled %s cancelled ***&n\n", type, GET_NAME(ch));
    send_to_all(buf);
    sprintf(buf, "Scheduled %s cancelled by %s", type, GET_NAME(ch));
    shutdownData.eShutdownType = TimedShutdownData::NONE;
    wizlog(60, buf);
  }

  if( !*arg )
  {
    send_to_char("Syntax: shutdown <ok | reboot | copyover>  [minutes to reboot].\r\n"
                 "  Scheduled shutdowns are cancelled when the scheduler leaves the game or\r\n"
                 "  uses the shutdown command again (even only to get this help)\r\n", ch);
    return;
  }

  if(!str_cmp(arg, "ok"))
  {
    shutdownData.eShutdownType = TimedShutdownData::OK;
  }
  else if(!str_cmp(arg, "reboot"))
  {
    shutdownData.eShutdownType = TimedShutdownData::REBOOT;
  }
  else if(!str_cmp(arg, "autoreboot"))
  {
    shutdownData.eShutdownType = TimedShutdownData::AUTOREBOOT;
  }
  else if(!str_cmp(arg, "copyover"))
  {
    shutdownData.eShutdownType = TimedShutdownData::COPYOVER;
  }
  else if(!str_cmp(arg, "pwipe"))
  {
    argument = one_argument(argument, arg);
    if(!str_cmp(arg, "confirm"))
    {
      argument = one_argument(argument, arg);
      if(!str_cmp(arg, "yes"))
      {
        send_to_char( "&-RYou've done it now.. the world is really going away!!!&n\n\r", ch );
        if( GET_LEVEL(ch) < OVERLORD )
        {
          send_to_char( "&=RLThe world resists your attempt to destroy it!  You must attain a higher level to do this.&n\n\r", ch );
          return;
        }
        send_to_char( "&=glYou have _five_ minutes to reconsider and cancel the shutdown.&n\n\r", ch );
        shutdownData.eShutdownType = TimedShutdownData::PWIPE;
        mins_to_reboot = 5;
      }
      else
      {
        send_to_char( "&=RWYou must &n&=RL'shutdown pwipe confirm yes'&=RW to do this, as it's going to DELETE everything!!!&n\n\r", ch );
        return;
      }
    }
    else
    {
      send_to_char( "&-rYou must &+W'shutdown pwipe confirm'&n&-r to do this, as it's going to DELETE everything!!!&n\n\r", ch );
      send_to_char( "&-rNot to be used for a partial wipe, and will wipe files you didn't know you had!!!&n\n\r", ch );
      return;
    }
  }
  else if(!str_cmp(arg, "segfault"))
  {
    raise(SIGSEGV);
  }
  else
  {
    send_to_char("Go shut down someone your own size.\n", ch);
    return;
  }
  if( shutdownData.eShutdownType == TimedShutdownData::OK
    || shutdownData.eShutdownType == TimedShutdownData::PWIPE )
  {
    type = "shutdown";
  }
  strcpy(shutdownData.IssuedBy, GET_NAME(ch));
  shutdownData.next_warning = -1;
  shutdownData.reboot_time = (time(0) + (mins_to_reboot * 60));
  sprintf(buf, "Scheduled %s initiated by %s in %d minutes.", type, GET_NAME(ch), mins_to_reboot);
  wizlog(60, buf);
  // calling the event will start the event
  timedShutdown(NULL, NULL, NULL, NULL);
}

void do_snoop(P_char ch, char *argument, int cmd)
{
  static char arg[MAX_STRING_LENGTH];
  P_char   victim;
  int      level;
  snoop_by_data *snoop_by_ptr;

  if(!ch->desc)
    return;
  one_argument(argument, arg);

  if(!*arg)
  {
    send_to_char("Snoop who ?\n", ch);
    return;
  }
  if(!(victim = get_char_vis(ch, arg)))
  {
    send_to_char("No such person around.\n", ch);
    return;
  }
  if(!victim->desc)
  {
    send_to_char("There's no link.. nothing to snoop.\n", ch);
    return;
  }
  if(victim == ch->desc->snoop.snooping)
  {
    send_to_char("Duh!  You already ARE snooping that person!\n", ch);
    return;
  }
  level = MIN(62, GET_LEVEL(ch));
  if(victim == ch)
  {
    send_to_char("Ok, you just snoop yourself.\n", ch);
    if(ch->desc->snoop.snooping)
    {
      if(level < 59)
        send_to_char("&+CYou are no longer being snooped.&N\n",
                     ch->desc->snoop.snooping);
      if(GET_LEVEL(ch) < FORGER)
      {
        sql_log(ch, WIZLOG, "Stopped snooping %s", GET_NAME(ch->desc->snoop.snooping));
      }
      rem_char_from_snoopby_list(&ch->desc->snoop.snooping->desc->snoop.snoop_by_list, ch);
      ch->desc->snoop.snooping = 0;
    }
    return;
  }

  if((GET_LEVEL(victim) >= level))
  {
    send_to_char("You failed.\n", ch);
    return;
  }
  send_to_char("Ok. \n", ch);

  if(ch->desc->snoop.snooping)
  {
    if(level < 58)
      send_to_char("&+CYou are no longer being snooped.&N\n",
                   ch->desc->snoop.snooping);
/*
    ch->desc->snoop.snooping->desc->snoop.snoop_by = 0;
*/
    rem_char_from_snoopby_list(&ch->desc->snoop.snooping->desc->snoop.
                               snoop_by_list, ch);

    sql_log(ch, WIZLOG, "Stopped snooping %s", GET_NAME(ch->desc->snoop.snooping));
    logit(LOG_WIZ, "(%s) stopped snooping (%s)", GET_NAME(ch), GET_NAME(ch->desc->snoop.snooping));
  }
  ch->desc->snoop.snooping = victim;
/*
  victim->desc->snoop.snoop_by = ch;
*/
  CREATE(snoop_by_ptr, snoop_by_data, 1, MEM_TAG_SNOOP);
  bzero(snoop_by_ptr, sizeof(snoop_by_data));

  snoop_by_ptr->next = victim->desc->snoop.snoop_by_list;
  snoop_by_ptr->snoop_by = ch;

  victim->desc->snoop.snoop_by_list = snoop_by_ptr;

  if(level < 58)
    send_to_char("&+CSomeone starts snooping you.&N\n", victim);

  if(GET_LEVEL(ch) < FORGER)
  {
    sql_log(ch, WIZLOG, "Started snooping %s", GET_NAME(victim));
  }

}

void do_switch(P_char ch, char *argument, int cmd)
{
  static char arg[MAX_STRING_LENGTH];
  snoop_by_data *snoop_by_ptr, *next;
  P_char   victim;

  if(IS_NPC(ch))
  {
    send_to_char("Sorry, no mobs allowed.\n", ch);
    return;
  }
  one_argument(argument, arg);

  if(!*arg)
  {
    send_to_char("Switch with who?\n", ch);
  }
  else
  {
    if(!(victim = get_char_vis(ch, arg)))
      send_to_char("They aren't here.\n", ch);
    else
    {
      if(ch == victim)
      {
        send_to_char("He he he... We are jolly funny today, eh?\n", ch);
        return;
      }
      if(!ch->desc || ch->desc->snoop.snooping)
      {
        send_to_char("Mixing snoop & switch is bad for your health.\n", ch);
        return;
      }
      if(victim->desc ||
          (IS_PC(victim) && (GET_LEVEL(ch) < GET_LEVEL(victim))))
      {
        send_to_char("You can't do that, the body is already in use!\n", ch);
        return;
      }
      else if((GET_LEVEL(ch) < OVERLORD))
      {
        wizlog(GET_LEVEL(ch), "%s has switched into '%s'",
               GET_NAME(ch), GET_NAME(victim));
        logit(LOG_WIZ, "%s has switched into '%s'", GET_NAME(ch),
              GET_NAME(victim));
      }
      send_to_char("Ok.\n", ch);

      if(ch->desc->snoop.snoop_by_list)
      {
        snoop_by_ptr = ch->desc->snoop.snoop_by_list;
        while (snoop_by_ptr)
        {
          send_to_char
            ("Your victim has switched into something, killing your snoop.\n",
             snoop_by_ptr->snoop_by);
          snoop_by_ptr->snoop_by->desc->snoop.snooping = 0;

          next = snoop_by_ptr->next;
          FREE(snoop_by_ptr);

          snoop_by_ptr = next;
        }

        ch->desc->snoop.snoop_by_list = 0;
      }

      if(IS_TRUSTED(ch) && !IS_FIGHTING(ch) && !IS_DESTROYING(ch))
        if(GET_WIZINVIS(ch) < GET_LEVEL(ch))
          GET_WIZINVIS(ch) = GET_LEVEL(ch);

      ch->desc->character = victim;
      ch->desc->original = ch;
      ch->only.pc->switched = victim;

      victim->desc = ch->desc;
      ch->desc = 0;
    }
  }
}

void do_return(P_char ch, char *argument, int cmd)
{
/*  if(CHAR_POLYMORPH_OBJ(ch))
  {
    return_from_poly_obj(ch);
    return;
  }*/

  if(IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    BackToUsualForm(ch);
    return;
  }
  if(!ch->desc)
    return;

  if(!ch->desc->original || IS_NPC(ch->desc->original))
  {
    send_to_char("Pardon?\n", ch);
    return;
  }
  if(IS_MORPH(ch))
  {
    send_to_char("Use \"shape me\" to return to your normal form.\n", ch);
    return;
  }
  if(ch->desc->original->only.pc->switched)
  {
    send_to_char("You return to your original body.\n", ch);

    ch->desc->character = ch->desc->original;
    ch->desc->original = 0;
    ch->desc->character->only.pc->switched = 0;

    ch->desc->character->desc = ch->desc;
    ch->desc = 0;
  }
  else                          /* switched body due to shape change  */
    send_to_char("No effect.\n", ch);
}

int      forced_command = 0;

void do_force(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  P_char   vict;
  int      level;
  char     name[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH],
    buf[MAX_INPUT_LENGTH + 60];

  if(IS_NPC(ch))
    return;

  level = MIN(62, GET_LEVEL(ch));

  half_chop(argument, name, to_force);

  if(!*name || !*to_force)
    send_to_char("Who do you wish to force to do what?\n", ch);
  else if(str_cmp("all", name))
  {
    if(!(vict = get_char_vis(ch, name)))
    {
      send_to_char("No-one by that name here..\n", ch);
      return;
    }
    else if(!str_cmp("quit", to_force))
    {
      send_to_char("Cannot force that player to quit.\n", ch);
      return;
    }
    else if(!strn_cmp("jun", to_force, 3))
    {
      send_to_char("Cannot force that player to junk.\n", ch);
      return;
    }
    else if(!str_cmp("fafhrd", name))
    {
      send_to_room
        ("&+yThe ground begins to shake...&n &=LBLightning&n&+B streaks towards your head...&n\n",
         ch->in_room);
      act("And Fafhrd turns to $n with a wicked grin... 'I don't think so'",
          FALSE, ch, 0, 0, TO_ROOM);
      send_to_char
        ("And Fafhrd turns to you with a wicked grin... 'I don't think so'\n",
         ch);
      return;
    }
    else
    {
      if((level <= GET_LEVEL(vict)) && IS_PC(vict))
        send_to_char("Ok.\n", ch);
      else
      {
        sprintf(buf, "$n has forced you to '%s'.", to_force);
        act(buf, FALSE, ch, 0, vict, TO_VICT);
        if(level < 62)
          wizlog(GET_LEVEL(ch), "%s has forced %s to '%s' [%d/%d]",
                 GET_NAME(ch), GET_NAME(vict), to_force,
                 world[ch->in_room].number, world[vict->in_room].number);
        logit(LOG_FORCE, "%s has forced %s to '%s' [%d/%d]", GET_NAME(ch),
              GET_NAME(vict), to_force, world[ch->in_room].number,
              world[vict->in_room].number);
        sql_log(ch, WIZLOG, "Forced %s to '%s'", GET_NAME(vict), to_force);
        send_to_char("Ok.\n", ch);
        forced_command = 1;
        command_interpreter(vict, to_force);
        forced_command = 0;
      }
    }
  }
  else
  {                             /* force all  */
    wizlog(level, "%s has forced all to '%s'", GET_NAME(ch), to_force);
    logit(LOG_FORCE, "%s has forced all to '%s'", GET_NAME(ch), to_force);
        sql_log(ch, WIZLOG, "Forced all to '%s'", to_force);
    for (i = descriptor_list; i; i = i->next)
      if(i->character != ch && !i->connected)
      {
        vict = i->character;
        if((level > GET_LEVEL(vict)))
        {
          sprintf(buf, "$n has forced you to '%s'.", to_force);
          act(buf, FALSE, ch, 0, vict, TO_VICT);
          forced_command = 1;
          command_interpreter(vict, to_force);
          forced_command = 0;
        }
      }
    send_to_char("Ok.\n", ch);
  }
}

void do_load(P_char ch, char *argument, int cmd)
{
  P_char   mob;
  P_obj    obj;
  char     type[MAX_INPUT_LENGTH], num[MAX_INPUT_LENGTH];
  int      l_num, r_num;

  if(IS_NPC(ch))
    return;

  argument_interpreter(argument, type, num);

  if(*type && !isdigit(*num) &&
      (GET_LEVEL(ch) >= GREATER_G || god_check(GET_NAME(ch))))
    if(is_abbrev(type, "char"))
    {
      do_read_player(ch, num, 0);
      return;
    }
  if(!*type || !*num || !isdigit(*num))
  {
    send_to_char("Syntax:\nload <'char' | 'obj'> <number>.\n", ch);
    return;
  }
  if((l_num = atoi(num)) < 0)
  {
    send_to_char("A NEGATIVE number??\n", ch);
    return;
  }
  if(is_abbrev(type, "char") || is_abbrev(type, "mobile"))
  {
    if((r_num = real_mobile(l_num)) < 0)
    {
      send_to_char("There is no monster with that number.\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    if(!mob)
    {
      logit(LOG_DEBUG, "do_load(): mob %d [%d] not loadable", r_num,
            mob_index[r_num].virtual_number);
      return;
    }
    GET_BIRTHPLACE(mob) = world[ch->in_room].number;
    apply_zone_modifier(mob);
    char_to_room(mob, ch->in_room, 0);

    if(GET_HIT(mob) > GET_MAX_HIT(mob))
      GET_HIT(mob) = GET_MAX_HIT(mob);

    if(GET_LEVEL(ch) < OVERLORD)
      wizlog(GET_LEVEL(ch), "%s loaded mob %s '%s' in [%d]",
             GET_NAME(ch), num, GET_NAME(mob), world[ch->in_room].number);
    logit(LOG_WIZLOAD, "%s loaded mob %s '%s' in [%d]",
          GET_NAME(ch), num, GET_NAME(mob), world[ch->in_room].number);
    sql_log(ch, WIZLOG, "Loaded mob %s &n[%s]", J_NAME(mob), num);
    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
        0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You have created $N!", FALSE, ch, 0, mob, TO_CHAR);
  }
  else if(is_abbrev(type, "object") || is_abbrev(type, "item"))
  {
    if((r_num = real_object(l_num)) < 0)
    {
      send_to_char("There is no object with that number.\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    if(!obj)
    {
      logit(LOG_DEBUG, "do_load(): obj %d [%d] not loadable", r_num,
            obj_index[r_num].virtual_number);
      return;
    }
    if(GET_LEVEL(ch) < OVERLORD)
      wizlog(GET_LEVEL(ch), "%s loaded obj %s '%s' in [%d]",
             GET_NAME(ch), num, obj->short_description,
             world[ch->in_room].number);
    logit(LOG_WIZLOAD, "%s loaded obj %s '%s' [%d]", GET_NAME(ch), num,
          obj->short_description, world[ch->in_room].number);
    sql_log(ch, WIZLOG, "Loaded obj %s &n[%s]", obj->short_description, num);
    obj->z_cord = ch->specials.z_cord;
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", TRUE, ch, obj, 0, TO_ROOM);
    act("You have created $p!", FALSE, ch, obj, 0, TO_CHAR);
    if(IS_SET(obj->wear_flags, ITEM_TAKE))
      obj_to_char(obj, ch);
    else
      obj_to_room(obj, ch->in_room);
  }
  else
    send_to_char("That'll have to be either 'char' or 'obj'.\n", ch);
}

/* clean a room of all mobiles and objects */
void do_purge(P_char ch, char *argument, int cmd)
{
  P_char   vict, next_v;
  P_obj    obj, next_o;
  int      level;
  char     name[MAX_INPUT_LENGTH];
  char     buf[512], buf2[512];
  time_t   laston, timegone;
  FILE    *flist, *f;
  P_ship temp;

  if(IS_NPC(ch))
    return;


  level = MIN(62, GET_LEVEL(ch));
  one_argument(argument, name);

  if(*name)
  {                             /* argument supplied. destroy single
                                   object or char */
    if((vict = get_char_room_vis(ch, name)))
    {
      if((GET_LEVEL(vict) >= level) && IS_PC(vict))
      {
        send_to_char("Oh no you don't!\n", ch);
        return;
      }
      if((IS_PC(vict) || IS_MORPH(vict)) && (level < GREATER_G))
      {
        send_to_char("You are too lame to be able to purge chars!\n", ch);
        return;
      }
      act("$n disintegrates $N, who is reduced to a small pile of ashes.",
          FALSE, ch, 0, vict, TO_NOTVICT);

      wizlog(GET_LEVEL(ch), "%s has purged %s [%d]", GET_NAME(ch),
               GET_NAME(vict), world[ch->in_room].number);
      logit(LOG_WIZ, "%s has purged %s [%d]", GET_NAME(ch),
            GET_NAME(vict), world[ch->in_room].number);
      
      sql_log(ch, WIZLOG, "Purged %s", GET_NAME(vict)); 
      
      if(IS_NPC(vict))
      {
        extract_char(vict);
        vict = NULL;
      }
      else
      {
        /* player will lose all objects! */
        extract_char(vict);
        writeCharacter(vict, 2, NOWHERE);
        if(vict->desc)
          close_socket(vict->desc);
        else
          free_char(vict);
        vict->desc = 0;
        vict = NULL;
      }
    }
    else
      if((obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))
    {
      if(obj->R_num == real_object(VOBJ_WALLS))
      {
        send_to_char("Use 'dispel magic' to get rid of walls.\n", ch);
        return;
      }
      // Use storage command to delete these items.
      if(GET_ITEM_TYPE(obj) == ITEM_STORAGE)
      {
        send_to_char("Use the storage command if you want to get rid of this item.\n", ch);
        return;
      }
      act("$n destroys $p.", TRUE, ch, obj, 0, TO_ROOM);

      wizlog(GET_LEVEL(ch), "%s has purged %s [%d]", GET_NAME(ch),
          obj->short_description, world[ch->in_room].number);
      logit(LOG_WIZ, "%s has purged %s [%d]", GET_NAME(ch),
          obj->short_description, world[ch->in_room].number);
      sql_log(ch, WIZLOG, "Purged %s", obj->short_description); 

      if(obj_index[obj->R_num].virtual_number == 60001) 
      {
        temp = shipObjHash.find(obj);
        if(!temp) return;
        shipObjHash.erase(temp);
        delete_ship(temp);
      } else {
        extract_obj(obj, TRUE);
        obj = NULL;
      }
    }
    else if(!str_cmp("pfiles", name))
    {
      flist = fopen("Players/pfiles", "r");
      if(!flist)
      {
        return;
      }
      while (fscanf(flist, " %s \n", buf) != EOF)
      {
        sprintf(buf2, "Players/%c/%s", LOWER(*buf), buf);
        f = fopen(buf2, "r");
        vict = (struct char_data *) mm_get(dead_mob_pool);
        vict->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
        if(restoreCharOnly(vict, skip_spaces(buf)) < 0 || !vict)
        {
          if(vict)
            free_char(vict);
          continue;
        }
        laston = vict->player.time.saved;

        /* can't be too careful */

        if((time(0) - laston) >= 0)
          timegone = (time(0) - laston) / 60;
        else
          timegone = 0;

        if(((timegone > 1440) && ((timegone / 1440) > 60)) &&
            (GET_LEVEL(vict) <= 56))
        {
          deleteCharacter(vict);
        }
        if(vict)
          free_char(vict);
        fclose(f);
      }
      fclose(flist);
    }
    else
    {
      send_to_char("I don't know anyone or anything by that name.\n", ch);
      return;
    }

    send_to_char("Ok.\n", ch);
  }
  else
  {                             /* no argument. clean out the room */
    if(IS_NPC(ch))
    {
      send_to_char("Don't... You would only kill yourself...\n", ch);
      return;
    }
    wizlog(GET_LEVEL(ch), "%s has purged the room %d",
           GET_NAME(ch), world[ch->in_room].number);
    logit(LOG_WIZ, "%s has purged the room %d", GET_NAME(ch),
          world[ch->in_room].number);
    sql_log(ch, WIZLOG, "Purged room");
    act("$n gestures... you are surrounded by godly power!",
        FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("The world seems a little cleaner.\n", ch->in_room);

    for (vict = world[ch->in_room].people; vict; vict = next_v)
    {
      next_v = vict->next_in_room;
      if(IS_NPC(vict) && !IS_MORPH(vict))
      {
        extract_char(vict);
        vict = NULL;
      }
    }

    for (obj = world[ch->in_room].contents; obj; obj = next_o)
    {
      next_o = obj->next_content;

      if(obj->R_num == real_object(VOBJ_WALLS))
        continue;

      if((obj->wear_flags & ITEM_TAKE) || (obj->type == ITEM_CORPSE &&
                                            !obj->contains))
      {
        extract_obj(obj, TRUE);
        obj = NULL;
      }
    }
  }
}


/*
   Krov: major modifications to roll_basic_abilities are
   - fixed "stat points" plus a bit of luck for roll fanatics
   - class restrictions are obeyed, since new nanny fixes class before
   rolling the stats
   - kept the range system, modified for total stats: flag=-1 means
   punishment, 0 is standard, 1..3 better and better and >=4
   means at random (don't use last one...)
   - added karma and luck to total points, players can't see them at
   roll time, so they can't control their stats completely
 */

// This function now assigns a static low level base value to all
// "normal" stats(not luck or karma). - Jexni

void roll_basic_abilities(P_char ch, int flag)
{

/* screw 'bell curves' and stat totalling, let's keep it simple */

#if 0
  int i, rolls[10], statp, temp, total, sides;
  /* this gives the total points distributed on _10_ stats,
     according to flag+1 */
  static int totlim[5] =
  {
    250, 550, 600, 650, 700
  };

  /* set minimum of points for class, 25 min in all stats,
     except for luck&karma (invis to player) */
  temp = 0;
  for (i = 0; i < 8; i++) {
    /* class requirements? */
    rolls[i] = min_stats_for_class[GET_CLASS(ch)][i];
    /* min stat */
    if (rolls[i] < 25)
      rolls[i] = 25;
    temp += rolls[i];
  }

  /* determine how many points will be distributed */
  if (flag >= 4)
    total = number(temp, 800);
  else {
    total = totlim[flag + 1];
    total += (dice(3, 21) + 17);
  }

  if (temp > total)
    /* probably punished, no additional points */
    total = 0;
  else
    total -= temp;

  /* distribute the rest of the points, choose random
     stat and add with a pseudo-bell made by 3 dices */
  while (total) {
    temp = number(0, 7);
    sides = (100 - rolls[temp]) / 3;
    if (sides)
      statp = dice(3, sides);
    else
      statp = 100 - rolls[temp];
    if (statp < total) {
      rolls[temp] += statp;
      total -= statp;
    } else {
      rolls[temp] += total;
      total = 0;
    }
  }

  ch->base_stats.Str = ch->curr_stats.Str = rolls[0];
  ch->base_stats.Dex = ch->curr_stats.Dex = rolls[1];
  ch->base_stats.Agi = ch->curr_stats.Agi = rolls[2];
  ch->base_stats.Con = ch->curr_stats.Con = rolls[3];
  ch->base_stats.Pow = ch->curr_stats.Pow = rolls[4];
  ch->base_stats.Int = ch->curr_stats.Int = rolls[5];
  ch->base_stats.Wis = ch->curr_stats.Wis = rolls[6];
  ch->base_stats.Cha = ch->curr_stats.Cha = rolls[7];
  ch->base_stats.Karma = ch->curr_stats.Karma = number(1, 100);
  ch->base_stats.Luck = ch->curr_stats.Luck = number(1, 100);
#endif
  ch->base_stats.Str = ch->curr_stats.Str = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Dex = ch->curr_stats.Dex = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Agi = ch->curr_stats.Agi = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Con = ch->curr_stats.Con = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Pow = ch->curr_stats.Pow = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Int = ch->curr_stats.Int = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Wis = ch->curr_stats.Wis = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Cha = ch->curr_stats.Cha = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Karma = ch->curr_stats.Karma = MIN(dice(5, 8) + 53, 95);
  ch->base_stats.Luck = ch->curr_stats.Luck = MIN(dice(5, 8) + 53, 95);
}
void NewbySkillSet(P_char ch)
{
  int      i;

  for (i = FIRST_SKILL; i <= LAST_SKILL; i++)
  {

//#ifdef SKILLPOINTS
//    if(SKILL_DATA_ALL(ch, i).rlevel[0] &&
//        SKILL_DATA_ALL(ch, i).rlevel[0] <= GET_LEVEL(ch) )
//    {
//      ch->only.pc->skills[i].learned = 10;
//      ch->only.pc->skills[i].taught = 10;
//#else
    if(SKILL_DATA_ALL(ch, i).rlevel[0] &&
        SKILL_DATA_ALL(ch, i).rlevel[0] <= GET_LEVEL(ch) && !IS_SPELL(i))
    {
      ch->only.pc->skills[i].learned = number(5, 20);
      ch->only.pc->skills[i].taught = SKILL_DATA_ALL(ch, i).maxlearn[0] - 10;
//#endif
    }
    else if(SKILL_DATA_ALL(ch, i).rlevel[0] && IS_SPELL(i) &&
             SKILL_DATA_ALL(ch, i).rlevel[0] <= GET_LEVEL(ch) && praying_class(ch))
    {
      ch->only.pc->skills[i].learned = 100;
    }
    else
    {
      ch->only.pc->skills[i].learned = 0;
    }
        //MULTICLASS
  }
}

// If nomsg == 0, send a message and assume a new char.
// If nomsg == CMD_MULTICLASS, this is a new multiclassed char.
void do_start(P_char ch, int nomsg)
{
  int      i;
  P_obj    tempobj;

  if( IS_NPC(ch) )
  {
    return;
  }

  ch->player.level = 1;
  ch->points.base_hit = 1;
  ch->points.base_mana = 1;

  if( !nomsg )
  {
    send_to_char("Welcome. This is now your character on Duris.\n"
      "May your journey here never end.....\n\n"
      " NOTE:  Type TOGGLE and HELP for useful information!\n", ch);
  }

  set_title(ch);

  if( !(GET_CLASS(ch, CLASS_AVENGER | CLASS_DREADLORD))
    && !((GET_RACE(ch) == RACE_DUERGAR || GET_RACE(ch) == RACE_MOUNTAIN)
    && GET_CLASS(ch, CLASS_BERSERKER)) )
  {
    load_obj_to_newbies(ch);
  }

  init_defaultlanguages(ch);

  /* problem, need to clear the skills array */
  for( i = 0; i < MAX_SKILLS; i++ )
  {
    ch->only.pc->skills[i].learned = 0;
  }

  ZONE_TROPHY(ch) = NULL;

  ch->only.pc->prestige = 0;
  if( nomsg == CMD_MULTICLASS )
  {
    // Set ch to parole if guilded.
    level_check(ch);
  }
  else
  {
    ch->specials.guild = 0;
    ch->specials.guild_status = 0;
  }

  NewbySkillSet(ch);

  setCharPhysTypeInfo(ch);

  GET_EXP(ch) = 1;

  if(isname("Duris", GET_NAME(ch)))
  {
    ch->player.level = OVERLORD;
  }

  GET_MAX_HIT(ch) = GET_HIT(ch) = ch->points.base_hit;

  GET_MAX_MANA(ch) = GET_MANA(ch) = ch->points.base_mana;

  GET_MAX_VITALITY(ch) = GET_VITALITY(ch) = vitality_limit(ch);

  if(GET_RACE(ch) != RACE_ILLITHID)
  {
    GET_COND(ch, THIRST) = 96;
    GET_COND(ch, DRUNK) = 0;
  }
  else
  {
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  }

  GET_COND(ch, FULL) = 96;

  if( nomsg != CMD_MULTICLASS )
  {
    /* set some defaults. */
    ch->specials.act = (PLR_PETITION | PLR_ECHO | PLR_SNOTIFY | PLR_PAGING_ON | PLR_MAP);

    /* preserve hardcore and newbie bits */
    ch->specials.act2 &= (PLR2_HARDCORE_CHAR | PLR2_NEWBIE);

    SET_BIT(ch->specials.act2, PLR2_NCHAT);
    SET_BIT(ch->specials.act2, PLR2_QUICKCHANT);
    SET_BIT(ch->specials.act2, PLR2_SPEC);
    SET_BIT(ch->specials.act2, PLR2_HINT_CHANNEL);
    SET_BIT(ch->specials.act2, PLR2_SHOW_QUEST);
    SET_BIT(ch->specials.act2, PLR2_BOON);
    SET_BIT(ch->specials.act2, PLR2_SHIPMAP);
    if( !GET_CLASS(ch, CLASS_PALADIN) )
    {
      SET_BIT(ch->specials.act, PLR_VICIOUS);
    }
    ch->only.pc->wimpy = 10;
    ch->only.pc->aggressive = -1;

    ch->only.pc->prompt =
      (PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MOVE | PROMPT_MAX_MOVE |
      PROMPT_TANK_NAME | PROMPT_TANK_COND | PROMPT_ENEMY |
      PROMPT_ENEMY_COND | PROMPT_TWOLINE | PROMPT_STATUS);
  }
  // New multi'd Paladins don't get vicious on.
  else if( GET_CLASS(ch, CLASS_PALADIN) )
  {
      REMOVE_BIT(ch->specials.act, PLR_VICIOUS);
  }

  if( USES_MANA(ch) )
  {
    ch->only.pc->prompt |= (PROMPT_MANA | PROMPT_MAX_MANA);
  }

/*  if(!GET_CLASS(ch, CLASS_PSIONICIST) && !GET_CLASS(ch, CLASS_MINDFLAYER))
    ch->only.pc->prompt =
      PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MOVE | PROMPT_MAX_MOVE;
  else
    ch->only.pc->prompt = PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MANA |
      PROMPT_MAX_MANA | PROMPT_MOVE | PROMPT_MAX_MOVE; */

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
}

void do_advance(P_char ch, char *argument, int cmd)
{
  P_char   victim;
  char     name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
  int      newlevel = 0, oldlevel, i;

  if(IS_NPC(ch))
    return;

  argument_interpreter(argument, name, level);

  if(*name)
  {
    if(!(victim = get_char_vis(ch, name)))
    {
      send_to_char("That player is not here.\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("Advance who?\n", ch);
    return;
  }

  if(IS_NPC(victim))
  {
    send_to_char("NO! Not on NPC's.\n", ch);
    return;
  }
  oldlevel = GET_LEVEL(victim);

  if(oldlevel == 0)
    ;
  else if(!*level)
  {
    send_to_char("You must supply a level number.\n", ch);
    return;
  }
  else
  {
    if(!isdigit(*level))
    {
      send_to_char("Second argument must be a positive integer.\n", ch);
      return;
    }
    newlevel = atoi(level);

    if(newlevel <= oldlevel)
    {
      send_to_char("Can't dimnish a players status.\n", ch);
      return;
    }
  }

  if(newlevel > GET_LEVEL(ch))
  {
    send_to_char
      ("So sorry Charlie!  You may not advance someone past your level.\n",
       ch);
    return;
  }
  if(newlevel > OVERLORD)
  {
    send_to_char("62 is the highest possible level.\n", ch);
    return;
  }
  
  if(newlevel >= MINLVLIMMORTAL && GET_LEVEL(ch) < OVERLORD)
  {
    send_to_char("You aren't allowed to create new gods.\n", ch);
    return;
  }
  
  if(newlevel != 1)
    send_to_char("You feel generous.\n", ch);

  act("$n makes some strange gestures.\n"
      "A strange feeling comes upon you.   Like a giant hand, light comes down\n"
      "from above, grabbing your body, which begins to pulse with colored lights\n"
      "from inside.  Your head seems to be filled with demons from another plane\n"
      "as your body dissolves to the elements of time and space itself.\n"
      "Suddenly a silent explosion of light snaps you back to reality.  You feel\n"
      "improved!", FALSE, ch, 0, victim, TO_VICT);

  if(oldlevel == 0)
  {
    do_start(victim, 0);
    return;
  }

  for (i = oldlevel; i < newlevel; i++)
    advance_level(victim);//, TRUE); wipe2011

  GET_EXP(victim) = 1;

  if(newlevel >= MINLVLIMMORTAL && oldlevel < MINLVLIMMORTAL)
  {
    send_to_char("A sense of timelessness overwhelms you.  "
                 "You feel the myriad aches and pains\n"
                 "of mortal flesh drop away.  Power courses through your body "
                 "and you seem to\nactually be glowing!\n\n"
                 "Welcome to immortality!\n\n", victim);
    victim->specials.act |=
      (PLR_PETITION | PLR_AGGIMMUNE | PLR_WIZLOG |
       PLR_STATUS | PLR_VNUM | PLR_NAMES);
    REMOVE_BIT(victim->specials.act, PLR_ANONYMOUS);
    victim->only.pc->prompt |= (PROMPT_VIS);
  }

  wizlog(GET_LEVEL(ch), " %s has advanced %s to level %d",
         GET_NAME(ch), GET_NAME(victim), GET_LEVEL(victim));
  logit(LOG_WIZ, "%s advanced %s to level %d",
        GET_NAME(ch), GET_NAME(victim), GET_LEVEL(victim));
  sql_log(ch, WIZLOG, "Advanced %s to level %d", GET_NAME(victim), GET_LEVEL(victim));
  do_restore(ch, name, -4);
}

#define REROLL_SYNTAX \
"Syntax:  reroll <name> [<modifier>]\n\
  Modifier is a number:\n\
    0 - miserable\n\
    1 - normal (default)\n\
    2 - good, still in 'normal' range\n\
    3 - great, can be above normal\n\
    4 - exceptional, well above average\n"

void do_reroll(P_char ch, char *argument, int cmd)
{
  P_char   victim;
  int      flag;
  char     name[MAX_INPUT_LENGTH], modifier[MAX_INPUT_LENGTH],
    buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  argument_interpreter(argument, name, modifier);
  if(!*name)
  {
    send_to_char(REROLL_SYNTAX, ch);
    return;
  }
  else if(!(victim = get_char_vis(ch, name)))
  {
    send_to_char("No-one by that name in the world.\n", ch);
    return;
  }
  if(!*modifier || (*modifier == '1'))
    flag = 0;
  else if(*modifier == '0')
    flag = -1;
  else if(*modifier == '2')
    flag = 1;
  else if(*modifier == '3')
    flag = 2;
  else if(*modifier == '4')
    flag = 3;
  else
  {
    send_to_char(REROLL_SYNTAX, ch);
    return;
  }

  roll_basic_abilities(victim, flag);
  if(!IS_TRUSTED(ch))
    send_to_char("Rerolled...\n", ch);
  else
  {
    sprintf(buf,
            "%s Rerolled:     Avg:(%d)\n  S:%3d  D:%3d  A:%3d  C:%3d\n  P:%3d  I:%3d  W:%3d Ch:%3d\n",
            GET_NAME(victim),
            (GET_C_STR(victim) + GET_C_DEX(victim) + GET_C_AGI(victim) +
             GET_C_CON(victim) + GET_C_POW(victim) + GET_C_INT(victim) +
             GET_C_WIS(victim) + GET_C_CHA(victim)) / 8, GET_C_STR(victim),
            GET_C_DEX(victim), GET_C_AGI(victim), GET_C_CON(victim),
            GET_C_POW(victim), GET_C_INT(victim), GET_C_WIS(victim),
            GET_C_CHA(victim));
    send_to_char(buf, ch);

    if(affect_total(victim, TRUE))
      return;                   /* whoops */
  }
  if(IS_TRUSTED(ch))
  {
    wizlog(GET_LEVEL(ch), "%s has rerolled %s%s", GET_NAME(ch),
           GET_NAME(victim),
           (flag == -1) ? " miserably" : (flag == 0) ? "" : (flag ==
                                                             1) ? " good"
           : (flag == 2) ? " great" : " godlike");
    logit(LOG_WIZ, "%s has rerolled %s%s", GET_NAME(ch), GET_NAME(victim),
          (flag == -1) ? " miserably" : (flag == 0) ? "" : (flag ==
                                                            1) ? " good"
          : (flag == 2) ? " great" : " godlike");
  }
}

#undef REROLL_SYNTAX

void do_restore(P_char ch, char *argument, int cmd)
{
  P_char   victim;
  P_desc   d;
  P_obj    obj;
  int      i = 0, j;
  char     buf[MAX_STRING_LENGTH];
  char     bomb[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf)
    send_to_char("Who do you wish to restore?\n", ch);
  else if(!str_cmp("all", buf))
  {
    if(GET_LEVEL(ch) < FORGER && !god_check(ch->player.name))
    {
      send_to_char("Sorry, thou canst.\n", ch);

      return;
    }

    for (d = descriptor_list; d; d = d->next)
      if(!d->connected)
      {
        victim = d->character;
        if(affected_by_spell(victim, TAG_BUILDING))
          continue;
        balance_affects(victim);
        if(GET_HIT(victim) < GET_MAX_HIT(victim))
          GET_HIT(victim) = GET_MAX_HIT(victim);
        GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
        if(GET_CLASS(victim, CLASS_PSIONICIST) || RACE_PUNDEAD(victim) ||
            GET_CLASS(ch, CLASS_MINDFLAYER))
          GET_MANA(victim) = GET_MAX_MANA(victim);
        GET_COND(victim, FULL) = IS_TRUSTED(victim) ? -1 : 24;
        GET_COND(victim, THIRST) = IS_TRUSTED(victim) ? -1 : 24;
        GET_COND(victim, DRUNK) = 0;
        if(GET_STAT(victim) < STAT_SLEEPING)
          SET_POS(victim, GET_POS(victim) + STAT_NORMAL);

#ifdef NEW_COMBAT
        j = getNumbBodyLocsbyPhysType(GET_PHYS_TYPE(victim));
        for (i = 0; i < j; i++)
          victim->points.location_hit[i] = 0;
#endif

        send_to_char("&+BA haze of magical energies fall from the heavens, engulfing all that you see.&LAs they subside, you feel refreshed...&n\n", victim);
        if(ch != victim)
            do_reboot_restore(ch, victim);
        
        act("You have been fully restored by $N!",
          FALSE, victim, 0, ch, TO_CHAR);

        if(isname("Kvark", ch->player.name))
          send_to_char(file_to_string("lib/creation/boom"), victim);
        if(isname("Zion", ch->player.name))
          send_to_char(file_to_string("lib/creation/hypnotoad"), victim);
        if(isname("Venthix", ch->player.name))
          send_to_char(file_to_string("lib/creation/skullsword"), victim);
        if(isname("Jexni", ch->player.name))
          send_to_char(file_to_string("lib/creation/hypnotoad"), victim);

      }
    send_to_char("Restoration of all players completed.\n", ch);
  }
  else if(!str_cmp("items", buf))
  {
    if(GET_LEVEL(ch) < FORGER)
    {
      send_to_char("Whoops, you can't!\n", ch);
      return;
    }

    for (d = descriptor_list; d; d = d->next)
      if(!d->connected)
      {
        victim = d->character;
        if(affected_by_spell(victim, TAG_BUILDING))
          continue;
        for (i = 0; i < MAX_WEAR; i++)
          if(victim->equipment[i])
            victim->equipment[i]->condition = 100;//victim->equipment[i]->max_condition; wipe2011
        for (obj = victim->carrying; obj; obj = obj->next_content)
          obj->condition = 100;//obj->max_condition; wipe2011
        send_to_char
          ("&+gFrom out of nowhere, little gremlin-like creatures about 6 inches tall pop up.&LThey grab all of your equipment, and fiddle with it before returning to you.&LThey then vanish as quickly as they came.\n",
           victim);
      }
  }
  else if(!(victim = get_char_vis(ch, buf)))
    send_to_char("No-one by that name in the world.\n", ch);
  else
  {
    if(affected_by_spell(victim, TAG_BUILDING))
    {
      send_to_char("Not allowed to restore an outpost!\n", ch);
      return;
    }
       
    balance_affects(victim);
    
    GET_MANA(victim) = GET_MAX_MANA(victim);
    GET_HIT(victim) = GET_MAX_HIT(victim);
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);

/*
 * Restore the NPCs complement of spells available for casting. - SKB
 */
 
    if(IS_NPC(victim))
    {
      victim->specials.undead_spell_slots[0] = 0;

      for (i = 1; i <= MAX_CIRCLE; i++)
        victim->specials.undead_spell_slots[i] =
          spl_table[GET_LEVEL(victim)][i - 1];
    }
    if(GET_STAT(victim) < STAT_SLEEPING)
      SET_POS(victim, GET_POS(victim) + STAT_NORMAL);

    if(IS_PC(ch))
    {
      GET_COND(victim, FULL) = IS_TRUSTED(victim) ? -1 : 24;
      GET_COND(victim, THIRST) = IS_TRUSTED(victim) ? -1 : 24;
      GET_COND(victim, DRUNK) = 0;
    }

/*
    j = getNumbBodyLocsbyPhysType(GET_PHYS_TYPE(victim));
    for (i = 0; i < j; i++)
      victim->points.location_hit[i] = 0;
*/

    if((cmd != -4) && !IS_TRUSTED(victim))
    {
      wizlog(GET_LEVEL(ch), "%s has restored %s", GET_NAME(ch),
             GET_NAME(victim));
      logit(LOG_WIZ, "%s has restored %s", GET_NAME(ch), GET_NAME(victim));
      sql_log(ch, WIZLOG, "Restored %s", GET_NAME(victim));
    }
    if(IS_TRUSTED(victim))
    {
      for (i = 0; i < MAX_SKILLS; i++)
      {
        victim->only.pc->skills[i].learned = 100;
      }
      for (i = 0; i < MAX_TONGUE; i++)
        GET_LANGUAGE(victim, i) = 100;
    }

    update_pos(victim);
    if(ch != victim)
    {
      send_to_char("Done.\n", ch);
      act("You have been fully restored by $N!",
          FALSE, victim, 0, ch, TO_CHAR);
    }
    else
      send_to_char("Restored.\n", ch);
  }
}

void do_freeze(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  P_obj    dummy;
  char     buf[MAX_STRING_LENGTH];
  int      level;

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);
  level = MIN(62, GET_LEVEL(ch));

  if(!*buf)
    send_to_char("Usage: freeze <player>\n", ch);
  else if(!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
    send_to_char("Couldn't find any such creature.\n", ch);
  else if(IS_NPC(vict))
    send_to_char("You can't freeze a mobile.\n", ch);
  else if(GET_LEVEL(vict) >= level)
    act("$E is too hot for you to freeze..", 0, ch, 0, vict, TO_CHAR);
  else if(IS_SET(vict->specials.act, PLR_FROZEN))
  {
    send_to_char("You have thawed out and can move again freely.\n", vict);
    send_to_char("Player has thawed out.\n", ch);
    REMOVE_BIT(vict->specials.act, PLR_FROZEN);
    if(GET_LEVEL(ch) > 50)
    {
      wizlog(GET_LEVEL(ch), "%s was just unfrozen by %s.",
             GET_NAME(vict), GET_NAME(ch));
      logit(LOG_WIZ, "%s was just unfrozen by %s.",
            GET_NAME(vict), GET_NAME(ch));
      sql_log(ch, WIZLOG, "Unfroze %s", GET_NAME(vict));
    }
  }
  else
  {
    send_to_char("You suddenly feel very frozen and can't move.\n", vict);
    send_to_char("FROZEN set.\n", ch);
    SET_BIT(vict->specials.act, PLR_FROZEN);
    if(GET_LEVEL(ch) > 50)
    {
      wizlog(GET_LEVEL(ch), "%s was just frozen by %s.",
             GET_NAME(vict), GET_NAME(ch));
      logit(LOG_WIZ, "%s was just frozen by %s.",
            GET_NAME(vict), GET_NAME(ch));
      sql_log(ch, WIZLOG, "Froze %s", GET_NAME(vict));
    }
  }
}

void do_zreset(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  struct zone_data *zone_struct;
  int      zone_number = 0;

  if(!ch)
  {
    logit(LOG_DEBUG, "Screw-up in do_zreset(), NULL ch.");
    return;
  }
  if(IS_NPC(ch))
  {
    return;
  }

  zone_number = world[ch->in_room].zone;
  zone_struct = &zone_table[zone_number];

  one_argument(argument, arg);

  if(!*arg)
  {
    send_to_char("Syntax: zreset ok or full(purge and reset)\n", ch);
  }
  else if(!str_cmp(arg, "ok"))
  {
    sprintf(buf, "Zone: %s has been reset.\n", zone_struct->name);
    send_to_char(buf, ch);
    reset_zone(zone_number, 0);
    // If char resetting zone is a mortal.. hrm..
    wizlog((GET_LEVEL(ch) > MAXLVLMORTAL) ? GET_LEVEL(ch) : MINLVLIMMORTAL,
      "%s just reset the zone %s.", GET_NAME(ch), zone_struct->name);
    logit(LOG_WIZ, "%s just reset the zone %s.", GET_NAME(ch), zone_struct->name);
    return;
  }
  else if(!str_cmp(arg, "full"))
  {
    sprintf(buf, "Zone: %s has been reset.\n", zone_struct->name);
    send_to_char(buf, ch);
    zone_purge(zone_number);
    reset_zone(zone_number, 1);
    wizlog((GET_LEVEL(ch) > MAXLVLMORTAL) ? GET_LEVEL(ch) : MINLVLIMMORTAL,
      "%s just reset the zone %s.", GET_NAME(ch), zone_struct->name);
    logit(LOG_WIZ, "%s just reset the zone %s.", GET_NAME(ch), zone_struct->name);
    return;
  }
  // We get here if someone enters a bad command.
  send_to_char("Syntax: zreset ok or full(purge and reset)\n", ch);
  return;
}

void do_reinitphys(P_char ch, char *arg, int cmd)
{
  P_char   vict;
  P_desc   p;

/**** temp ****/
  char     buf[MAX_STRING_LENGTH];

  one_argument(arg, buf);

  if(!*buf)
    send_to_char("Who do you wish to reinitialize skills for?\n", ch);
  else if(!str_cmp("all", buf))
  {
    for (p = descriptor_list; p; p = p->next)
      if(!p->connected)
      {
        vict = p->character;
        if(!IS_TRUSTED(vict))
          NewbySkillSet(vict);
        send_to_char
          ("&+LA haze of powdery dust falls from the heavens, clogging your breathing, choking your lungs, blurring your vision. As it begins to subside, you realize all your wordly knowledge is somehow.... different.\n",
           vict);
      }
    send_to_char("Resetting of all player skills completed.\n", ch);
  }
  else if(!(vict = get_char_vis(ch, buf)))
    send_to_char("No-one by that name in the world.\n", ch);
  else if(!IS_TRUSTED(vict))
  {
    NewbySkillSet(vict);
    sprintf(buf, "Resetting of $N's skills completed.");
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
  }
  else
    send_to_char("No gods please.\n", ch);
  return;
/****/

  if( GET_LEVEL(ch) <= MAXLVLMORTAL )
  {                             /*
                                 * mortals can only set
                                 * their own attr
                                 */
    send_to_char("Your height and weight have been re-randomed!\n", ch);
    init_height_weight(ch);
    return;
  }
  if(!*arg || !arg)
  {
    send_to_char("usage:\n   reinitphys <targetname>\n", ch);
    return;
  }
  if(!strcmp(arg, "all"))
  {
    for (p = descriptor_list; p; p = p->next)
      if(!p->connected)
        if(CAN_SEE(ch, p->character))
          if(strcmp(GET_NAME(ch), GET_NAME(p->character)))
            do_reinitphys(ch, GET_NAME(ch), CMD_REINITPHYS);
    send_to_char
      ("Ok, you re-random everyone's physical stats (except yourself's.\n",
       ch);
    return;
  }
  vict = get_char_vis(ch, arg);
  if(!vict)
  {
    send_to_char("You see no char with name like that in the game!\n", ch);
    return;
  }
  act("Ok, $N's weight and height rerolled.", TRUE, ch, 0, vict, TO_CHAR);
  act("Your height and weight have been randomly rolled again by $N!", TRUE,
      vict, 0, ch, TO_CHAR);
  set_char_height_weight(vict);
}

void do_silence(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  P_obj    dummy;
  char     buf[MAX_STRING_LENGTH];
  int      level;

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);
  level = MIN(62, GET_LEVEL(ch));
  if(!*buf)
    send_to_char("Usage: silence <player>\n", ch);
  else if(!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
    send_to_char("Couldn't find any such creature.\n", ch);
  else if(IS_NPC(vict))
    send_to_char("Can't do that to a mobile.\n", ch);
  else if(GET_LEVEL(vict) > level)
    act("$E might object to that.. better not.", 0, ch, 0, vict, TO_CHAR);
  else if(IS_SET(vict->specials.act, PLR_SILENCE))
  {
    send_to_char
      ("You can use communications channels again...but don't piss off the gods again!\n",
       vict);
    send_to_char("SILENCE removed.\n", ch);
    REMOVE_BIT(vict->specials.act, PLR_SILENCE);
    if(GET_LEVEL(ch) > 56)
    {
      wizlog(GET_LEVEL(ch), "%s just removed the silence on %s.",
             GET_NAME(ch), GET_NAME(vict));
      logit(LOG_WIZ, "%s just removed the silence on %s.",
            GET_NAME(ch), GET_NAME(vict));
      sql_log(ch, WIZLOG, "Unsilenced %s", GET_NAME(vict));
    }
  }
  else
  {
    send_to_char
      ("&+WThe gods take away your ability to use ANY communications channel!\n",
       vict);
    send_to_char("SILENCE set.\n", ch);
    SET_BIT(vict->specials.act, PLR_SILENCE);
    if(GET_LEVEL(ch) > 56)
    {
      wizlog(GET_LEVEL(ch), "%s was just silenced by %s.",
             GET_NAME(vict), GET_NAME(ch));
      logit(LOG_WIZ, "%s was just silenced by %s.",
            GET_NAME(vict), GET_NAME(ch));
      sql_log(ch, WIZLOG, "Silenced %s", GET_NAME(vict));
    }
  }
}

/* Make oneself visible only to players above certain levels. */
void do_vis(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf)
  {                             /* Inquire visibility */
    if(IS_TRUSTED(ch))
      sprintf(buf,
              "You are currently visible only to players with level > %d.\n",
              ch->only.pc->wiz_invis);
    if(IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
      if(strlen(buf) > 0)
        strcat(buf, "You are also affected by invisibility.\n");
      else
        strcpy(buf, "You are affected by invisibility.\n");
    else if(strlen(buf) == 0)
      strcpy(buf, "You are not invisible.\n");
    send_to_char(buf, ch);
  }
  else
  {
/** Set new visibility  */
    int      min_level;

    if IS_AFFECTED
      (ch, AFF_WRAITHFORM)
    {
      BackToUsualForm(ch);
      return;
    }
    if(!IS_TRUSTED(ch))
    {
      appear(ch);
      return;
    }
    min_level = MAX(0, atoi(buf));
    if(min_level >= GET_LEVEL(ch))
    {
      send_to_char
        ("Sorry... but you cannot be invis to your peers or superiors.\n",
         ch);
      return;
    }
    sprintf(buf, "You are now visible only to PCs with level > %d.\n",
            min_level);

    ch->only.pc->wiz_invis = (ubyte) min_level;
    send_to_char(buf, ch);
  }
}

void do_law_flags(P_char ch, char *argument, int cmd)
{
  /* intended usage:  lflags player city flag player - any PC city   -
   * any string (or partial string) in town_name_list flag   - any
   * string in justice_flag_names */

  char     c_flag[MAX_STRING_LENGTH], person[MAX_STRING_LENGTH],
    c_city[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  char    *args;
  P_char   vict;
  int      i, city;
  int      flag;
  unsigned l;

  if(IS_NPC(ch))
    return;

  if((args = one_argument(argument, person)) == NULL)
    return;

  if(*person == '\0')
  {
    send_to_char("Eh?  try \"lflags player\" (to display a player's flags)\n",
                 ch);
    send_to_char
      ("      or \"lflags player city flag\" (to set player's flag for city)\n\n",
       ch);
    send_to_char
      ("City names (may be partial):            Flag names (may be partial):\n\n",
       ch);
    for (i = 1; i <= LAST_HOME; i++)
    {
      sprintf(buf, "%-40s%-20s\n", town_name_list[i],
              ((i - 1) > JUSTICE_IS_LAST_FLAG) ? "" :
              justice_flag_names[i - 1]);
      send_to_char(buf, ch);
    }
    send_to_char("\n", ch);
    return;
  }
  /* find the player...  */
  if(!(vict = get_char_vis(ch, person)) || IS_NPC(vict))
  {
    send_to_char("No one by that name here...\n", ch);
    return;
  }
  if((args = one_argument(args, c_city)) == NULL)
    return;
  if(*c_city == '\0')
  {
    /* okay.. just show them all the law flags for this player  */
    sprintf(buf, "Current justice flags for %s:\n\n", vict->player.name);
    send_to_char(buf, ch);
    for (i = 1; i <= LAST_HOME; i++)
    {
      if(IS_TOWN_INVADER(vict, i))
        continue;
      sprintf(buf, "%-15s    %s\n", town_name_list[i],
              justice_flag_names[PC_TOWN_JUSTICE_FLAGS(vict, i)]);
      send_to_char(buf, ch);
    }
    send_to_char("\n", ch);
    return;
  }
  /* validate the city...  */
  l = strlen(c_city);
  for (city = 1; city <= LAST_HOME; city++)
    if(!strn_cmp(c_city, town_name_list[city], l))
      break;

  if(city > LAST_HOME)
  {
    send_to_char("Invalid city name.  Use \"lflags\" with no", ch);
    send_to_char(" parameters for help.\n", ch);
    return;
  }
  if((args = one_argument(args, c_flag)) == NULL)
    return;
  if(*c_flag == '\0')
  {
    send_to_char("And which flag do you want to set?\n", ch);
    return;
  }
  /* validate the flag */
  l = strlen(c_flag);
  for (flag = 0; flag <= JUSTICE_IS_LAST_FLAG; flag++)
    if(!strn_cmp(c_flag, justice_flag_names[flag], l))
      break;

  if(flag > JUSTICE_IS_LAST_FLAG)
  {
    send_to_char("Invalid flag name.  Use \"lflags\" with no", ch);
    send_to_char(" parameters for help.\n", ch);
    return;
  }
  if(IS_TOWN_INVADER(vict, city) && (flag != JUSTICE_IS_OUTCAST))
  {
    send_to_char("They can only be an outcast from there!\n", ch);
    return;
  }
  PC_SET_TOWN_JUSTICE_FLAGS(vict, flag, city);

  /* fix their birth/home rooms  */
  if(flag == JUSTICE_IS_OUTCAST)
  {
    int      r_room;

    r_room = real_room(GET_BIRTHPLACE(vict));
    if((r_room != NOWHERE) &&
        (city == zone_table[world[r_room].zone].hometown))
    {

      /* okay.. they are outcast in their birthplace.. FIX IT!  */
      /* whee!  kludge time! */
      if(PC_TOWN_JUSTICE_FLAGS(vict, HOME_BLOODSTONE) != JUSTICE_IS_OUTCAST)
        GET_BIRTHPLACE(vict) = 7255;
      else
        GET_BIRTHPLACE(vict) = EVIL_RACE(vict) ? 4093 : 1757;
    }
    /* now check their home room  */

    r_room = real_room(GET_HOME(vict));
    if((r_room != NOWHERE) &&
        (city == zone_table[world[r_room].zone].hometown))
      GET_HOME(vict) = GET_BIRTHPLACE(vict);
  }
  sprintf(buf, "%s is now a %s in %s.\n",
          GET_NAME(vict), justice_flag_names[flag], town_name_list[city]);
  send_to_char(buf, ch);

  sprintf(buf, "%s just set your status in %s to \"%s\".\n",
          CAN_SEE(vict, ch) ? GET_NAME(ch) : "Someone",
          town_name_list[city], justice_flag_names[flag]);
  send_to_char(buf, vict);

  /* log the hell out of it. */

  sprintf(buf, "%s made %s in %s by %s",
          GET_NAME(vict), justice_flag_names[flag],
          town_name_list[city], GET_NAME(ch));
  wizlog(GET_LEVEL(ch), buf);
  logit(LOG_FLAG, buf);
  logit(LOG_WIZ, buf);
}

/* Used to demote player to level 1 (and level 1 only) */

void do_demote(P_char ch, char *argument, int cmd)
{
  char     person[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  P_char   victim;
  int      i;
  uint     old_time;

  if(IS_NPC(ch))
    return;


  one_argument(argument, person);

  if(!*person)
  {
    send_to_char("Syntax: demote person\n", ch);
    return;
  }
  if(!(victim = get_char_vis(ch, person)))
  {
    send_to_char("No one by that name here...\n", ch);
    return;
  }
  if(IS_NPC(victim))
  {
    send_to_char("Monsters cannot be demoted.\n", ch);
    return;
  }
  if(GET_LEVEL(victim) >= MIN(62, GET_LEVEL(ch)))
  {
    send_to_char("Oh no you don't!\n", ch);
    return;
  }

  victim->only.pc->epics = 0;
//  gain_exp_regardless(victim, (int) -(GET_EXP(victim)));

  /* Unlearn all skill */

  for (i = 0; i < MAX_SKILLS; i++)
  {
    victim->only.pc->skills[i].learned = 0;
  }
  NewbySkillSet(victim);

  /* Restore other attributes */

  victim->points.max_mana = 0;
  victim->points.max_vitality = 0;

  /* Set hunger and thirst to 24 (instead of -1)  */

  victim->specials.conditions[0] = 24;
  victim->specials.conditions[1] = 24;
  victim->specials.conditions[2] = 0;

  old_time = ch->player.time.played;
  do_start(victim, 0);
  ch->player.time.played = old_time;

  /* LOG */

  wizlog(GET_LEVEL(ch),
         "%s has just demoted %s to level 1...Bummer\n",
         GET_NAME(ch), GET_NAME(victim));
  logit(LOG_WIZ, "%s has just demoted %s to level 1\n",
        GET_NAME(ch), GET_NAME(victim));
  sql_log(ch, WIZLOG, "Demoted %s to level 1", GET_NAME(victim));
  
  /* Tell caster */

  sprintf(buf,
          "You have just demoted %s to level 1!  Ain't they hating..\n",
          GET_NAME(victim));
  send_to_char(buf, ch);

  /* Tell victim  */

  send_to_char("You have just been demoted to level one... oh well.\n",
               victim);
}

/*
 * ** This command gives a wizard the power to modify a player's **
 * attributes.  See below for a list of modifiable player's ** attributes.
 * ** ** Usage: **     setattr player attribute value **
 */

void do_setattr(P_char ch, char *arg, int cmd)
{

  /* Internal Macros */

#define OFFSET(Field)   \
  OFFSET_OF(P_char, Field)

  /* Note that following macro will only work on non-Cray   */
  /* architectures. */

#define OFFSET_OF(Type, Field)  \
    ((int) (((char *) (&(((Type) NULL)->Field))) - ((char *) NULL)))

  /* Declaration */

  char     name[MAX_STRING_LENGTH];
  char     attribute[MAX_STRING_LENGTH];
  char     value_str[MAX_STRING_LENGTH];
  int      value, i;
  P_char   victim;

  /*
   * ** The following structure is used to make adding new fields **
   * that are modifiable by this wiz command as easy as possible. ** **
   * Note that "str" is a special case and is handled ** separately
   * below.
   */

  static struct
  {

    const char *attr_name;      /* Name of attribute  */
    unsigned long attr_offset;  /* Offset from beginning */
    /* Function used to copy  */
    void     (*attr_func) (P_char, unsigned long, int);

  } attr_tuples[] =
  {
    {
    "age", OFFSET(player.time.birth), sa_ageCopy}
    ,
    {
    "sex", OFFSET(player.sex), sa_byteCopy}
    ,
    {
    "str", OFFSET(base_stats.Str), sa_byteCopy}
    ,
    {
    "dex", OFFSET(base_stats.Dex), sa_byteCopy}
    ,
    {
    "agi", OFFSET(base_stats.Agi), sa_byteCopy}
    ,
    {
    "con", OFFSET(base_stats.Con), sa_byteCopy}
    ,
    {
    "pow", OFFSET(base_stats.Pow), sa_byteCopy}
    ,
    {
    "int", OFFSET(base_stats.Int), sa_byteCopy}
    ,
    {
    "wis", OFFSET(base_stats.Wis), sa_byteCopy}
    ,
    {
    "cha", OFFSET(base_stats.Cha), sa_byteCopy}
    ,
    {
    "luck", OFFSET(base_stats.Luck), sa_byteCopy}
    ,
    {
    "Karma", OFFSET(base_stats.Karma), sa_byteCopy}
    ,
    {
    "hit", OFFSET(points.max_hit), sa_shortCopy}
    ,
    {
    "hitroll", OFFSET(points.hitroll), sa_byteCopy}
    ,
    {
    "damroll", OFFSET(points.damroll), sa_byteCopy}
    ,
    {
    "drunk", OFFSET(specials.conditions[0]), sa_byteCopy}
    ,
    {
    "hunger", OFFSET(specials.conditions[1]), sa_byteCopy}
    ,
    {
    "thirst", OFFSET(specials.conditions[2]), sa_byteCopy}
    ,
    {
    "alignment", OFFSET(specials.alignment), sa_intCopy}
    ,
    {
    "affected", OFFSET(specials.affected_by), sa_intCopy}
    ,
    {
    "act", OFFSET(specials.act), sa_intCopy}
    ,
    {
    "npcflag", OFFSET(specials.act), sa_intCopy}
    ,
    {
    "pcflag", OFFSET(specials.affected_by), sa_intCopy}
  ,};

#undef OFFSET
#undef OFFSET_OF

  /* Executable section */

  if(IS_NPC(ch))
  {
    return;
  }
  wizlog(GET_LEVEL(ch), "%s: setattr: %s", GET_NAME(ch), arg);
  logit(LOG_WIZ, "%s: setattr: %s", GET_NAME(ch), arg);
  sql_log(ch, WIZLOG, "setattr: %s", arg);

  /* Get name of player  */

  arg = one_argument(arg, name);

  if(!*name)
  {
    send_to_char("Current settable attributes are:\n\n", ch);

    for (i = 0; i < ARRAY_SIZE(attr_tuples); i++)
    {
      send_to_char(attr_tuples[i].attr_name, ch);
      send_to_char("\n", ch);
    }
    return;
  }

  if( isname(name, "ship") )
  {
    do_setship( ch, arg );
    return;
  }

  if(!(victim = get_char_vis(ch, name)))
  {
    send_to_char("No one by that name here...\n", ch);
    return;
  }
  /* Get name of attribute */

  arg = one_argument(arg, attribute);

  if(!*attribute)
  {
    send_to_char("Current settable attributes are:\n\n", ch);

    for (i = 0; i < ARRAY_SIZE(attr_tuples); i++)
    {
      send_to_char(attr_tuples[i].attr_name, ch);
      send_to_char("\n", ch);
    }
    send_to_char("ship\n", ch);
    return;
  }

  /* Get value */

  arg = one_argument(arg, value_str);

  if(!*value_str)
  {
    send_to_char("Current settable attributes are:\n\n", ch);

    for (i = 0; i < ARRAY_SIZE(attr_tuples); i++)
    {
      send_to_char(attr_tuples[i].attr_name, ch);
      send_to_char("\n", ch);
    }
    send_to_char("ship\n", ch);
    return;
  }

  value = atoi(value_str);

  /* Handle the special cases ...  */

  /* Find out which attributes to set  */

  for (i = 0; i < ARRAY_SIZE(attr_tuples); i++)
  {
    if(!str_cmp(attr_tuples[i].attr_name, attribute))
      break;
  }

  if(i == ARRAY_SIZE(attr_tuples))
  {
    send_to_char("Current settable attributes are:\n\n", ch);

    for (i = 0; i < ARRAY_SIZE(attr_tuples); i++)
    {
      send_to_char(attr_tuples[i].attr_name, ch);
      send_to_char("\n", ch);
    }
    send_to_char("ship\n", ch);
    return;
  }
  /* Now set attributes  */

  (*(attr_tuples[i].attr_func)) (victim, attr_tuples[i].attr_offset, value);
  send_to_char("OK.\n", ch);
  balance_affects(victim);

  return;
}

void do_poofIn(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int      i;

  if(!IS_TRUSTED(ch))
    return;

  one_argument(argument, arg);

  if(!*arg)
  {
    if(ch->only.pc->poofIn != NULL)
      str_free(ch->only.pc->poofIn);
    ch->only.pc->poofIn = NULL;
  }
  else if(!str_cmp("?", arg))
  {
    if(ch->only.pc->poofIn == NULL)
    {
      strcpy(buf, "$n appears with an ear-splitting bang.");
    }
    else
    {
      if(!strstr(ch->only.pc->poofIn, "%n"))
      {
        strcpy(buf, "$n ");
        strcat(buf, ch->only.pc->poofIn);
      }
      else
      {
        strcpy(buf, ch->only.pc->poofIn);

        /* bleah, code doubles $ to prevent entering 'act' strings */
        for (i = 0; i < strlen(buf); i++)
          if((*(buf + i) == '%') && (*(buf + i + 1) == 'n'))
            *(buf + i) = '$';
      }
    }
    act(buf, TRUE, ch, 0, 0, TO_CHAR);
  }
  else
  {
    if(ch->only.pc->poofIn != NULL)
      str_free(ch->only.pc->poofIn);

    if(*argument == ' ')
      argument++;
    ch->only.pc->poofIn = str_dup(argument);
  }
}

void do_poofOut(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int      i;

  if(!IS_TRUSTED(ch))
    return;

  one_argument(argument, arg);

  if(!*arg)
  {
    if(ch->only.pc->poofOut != NULL)
      str_free(ch->only.pc->poofOut);

    ch->only.pc->poofOut = NULL;
  }
  else if(!str_cmp("?", arg))
  {
    if(ch->only.pc->poofOut == NULL)
    {
      strcpy(buf, "$n disappears in a puff of smoke.");
    }
    else
    {
      if(!strstr(ch->only.pc->poofOut, "%n"))
      {
        strcpy(buf, "$n ");
        strcat(buf, ch->only.pc->poofOut);
      }
      else
      {
        strcpy(buf, ch->only.pc->poofOut);
        /* bleah, code doubles $ to prevent entering 'act' strings */
        for (i = 0; i < strlen(buf); i++)
          if((*(buf + i) == '%') && (*(buf + i + 1) == 'n'))
            *(buf + i) = '$';
      }
    }
    act(buf, TRUE, ch, 0, 0, TO_CHAR);
  }
  else
  {
    if(ch->only.pc->poofOut != NULL)
      str_free(ch->only.pc->poofOut);

    if(*argument == ' ')
      argument++;
    ch->only.pc->poofOut = str_dup(argument);
  }
}

void do_poofInSound(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_INPUT_LENGTH];

  if(!IS_TRUSTED(ch))
    return;

  one_argument(argument, arg);

  if(!*arg)
  {
    if(ch->only.pc->poofInSound != NULL)
      str_free(ch->only.pc->poofInSound);
    ch->only.pc->poofInSound = NULL;
  }
  else if(strstr(arg, "!!SOUND") || strstr(arg, "!!sound"))
  {
    send_to_char
      ("To set your poofin sound, just specify the filename (with extension).\n",
       ch);
    return;
  }
  else
  {
    if(ch->only.pc->poofInSound != NULL)
      str_free(ch->only.pc->poofInSound);

    if(*argument == ' ')
      argument++;
    if(strlen(argument) > 39)
      *(argument + 38) = '\0';
    ch->only.pc->poofInSound = str_dup(argument);
  }
}

void do_poofOutSound(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_INPUT_LENGTH];

  if(!IS_TRUSTED(ch))
    return;

  one_argument(argument, arg);

  if(!*arg)
  {
    if(ch->only.pc->poofOutSound != NULL)
      str_free(ch->only.pc->poofOutSound);
    ch->only.pc->poofOutSound = NULL;
  }
  else if(strstr(arg, "!!SOUND") || strstr(arg, "!!sound"))
  {
    send_to_char
      ("To set your poofout sound, just specify the filename (with extension).\n",
       ch);
    return;
  }
  else
  {
    if(ch->only.pc->poofOutSound != NULL)
      str_free(ch->only.pc->poofOutSound);

    if(*argument == ' ')
      argument++;
    if(strlen(argument) > 39)
      *(argument + 38) = '\0';
    ch->only.pc->poofOutSound = str_dup(argument);
  }
}

void do_teleport(P_char ch, char *argument, int cmd)
{
  P_char   victim, target_mob;
  char     person[MAX_INPUT_LENGTH], room[MAX_INPUT_LENGTH];

  int      target, old_room;
  int      loop;

  if(!IS_TRUSTED(ch))
    return;

  half_chop(argument, person, room);

  if(!*person)
  {
    send_to_char("Who do you wish to teleport?\n", ch);
    return;
  }
  if(!*room)
  {
    send_to_char("Where do you wish to send this person?\n", ch);
    return;
  }
  if(!(victim = get_char_vis(ch, person)))
  {
    send_to_char("No-one by that name around.\n", ch);
    return;
  }
  if((GET_LEVEL(victim) > MIN(62, GET_LEVEL(ch))) &&
      (!IS_NPC(victim) || (GET_LEVEL(ch) < 60)))
  {
    send_to_char("You can't do that!\n", ch);
    return;
  }
  if(isdigit(*room))
  {
    target = atoi(&room[0]);
    for (loop = 0; loop <= top_of_world; loop++)
    {
      if(world[loop].number == target)
      {
        target = loop;
        break;
      }
      else if(loop == top_of_world)
      {
        send_to_char("No room exists with that number.\n", ch);
        return;
      }
    }
  }
  else if(!strcmp(room, "home"))
  {
    target = real_room(GET_HOME(victim));
  }
  else if(!strcmp(room, "return"))
  {
    target = real_room(victim->specials.was_in_room);
    if(target == NOWHERE)
    {
      send_to_char("Return them to where?  The main menu?\n", ch);
      return;
    }
  }
  else if((target_mob = get_char_vis(ch, room)))
  {
    target = target_mob->in_room;
  }
  else
  {
    send_to_char("No such target (person) can be found.\n", ch);
    return;
  }
  if(IS_SET(world[target].room_flags, PRIVATE) && (GET_LEVEL(ch) < MAXLVL))
  {
    send_to_char("That room is private.\n", ch);
    return;
  }
  if(!can_enter_room(victim, target, FALSE) && GET_LEVEL(ch) < 59)
  {
    send_to_char("That person can't go there.\n", ch);
    return;
  }
  act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
  old_room = victim->in_room;
  act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
  char_from_room(victim);
  room_light(old_room, REAL);
  char_to_room(victim, target, -1);
  act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
  send_to_char("Teleport completed.\n", ch);

  logit(LOG_WIZ, "%s teleported %s from %d to %d", ch->player.name,
        victim->player.name, world[old_room].number,
        world[victim->in_room].number);
  sql_log(ch, WIZLOG, "Teleported %s from %d to %d",
        victim->player.name, world[old_room].number,
        world[victim->in_room].number);  
}

void do_wizhost(P_char ch, char *argument, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  char     ip[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  struct wizban_t *tmp;
  int      count;

  if(!IS_TRUSTED(ch))
    return;

  argument_interpreter(argument, name, ip);

  if(!*name)
  {
    /* list the sites  */
    send_to_char
      ("Who:              Allowed Host\n--------------------------------------\n",
       ch);
    for (count = 0, tmp = wizconnect; tmp; count++, tmp = tmp->next)
    {
      sprintf(buf, "%16s %s\n", tmp->name, tmp->ban_str);
      send_to_char(buf, ch);
    }
    return;
  }
  logit(LOG_WIZ, "(%s) allows wizconnect: %s", ch->player.name, argument);

  for (tmp = wizconnect; tmp; tmp = tmp->next)
  {
    if((!str_cmp(name, tmp->name)) && (!str_cmp(ip, tmp->ban_str)))
    {
      send_to_char("That site is already allowed!\n", ch);
      return;
    }
  }
  CREATE(tmp, struct wizban_t, 1, MEM_TAG_WIZBAN);
  CREATE(tmp->name, char, strlen(name) + 1, MEM_TAG_STRING);
  CREATE(tmp->ban_str, char, strlen(ip) + 1, MEM_TAG_STRING);

  strcpy(tmp->name, name);
  strcpy(tmp->ban_str, ip);

  tmp->next = wizconnect;
  wizconnect = tmp;
  save_wizconnect_file();
}

void do_ban(P_char ch, char *argument, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  struct ban_t *tmp;
  int      count;

  if(!IS_TRUSTED(ch))
    return;

  one_argument(argument, name);

  if(!*name)
  {
    /* list the sites  */
    send_to_char
      ("Lvl By:              Banned substrings\n--------------------------------------\n",
       ch);
    if(bounce_null_sites)
      send_to_char("                    [Null sites being bounced]\n", ch);
    if(ban_list == (struct ban_t *) NULL)
    {
      send_to_char("Empty list!\n", ch);
      return;
    }
    for (count = 0, tmp = ban_list; tmp; count++, tmp = tmp->next)
    {
      sprintf(buf, "%3d %16s %s\n", tmp->lvl, tmp->name, tmp->ban_str);
      send_to_char(buf, ch);
    }
    if(count == 1)
    {
      sprintf(buf, "\nThere is 1 banned site string.\n");
    }
    else
    {
      sprintf(buf, "\nThere are %d banned site strings.\n", count);
    }
    send_to_char(buf, ch);
    return;
  }
  logit(LOG_WIZ, "(%s) ban %s", ch->player.name, argument);
  if(!str_cmp(name, "null"))
  {
    bounce_null_sites = 1;
    send_to_char("Null sites will now be bounced.\n", ch);
    return;
  }
  for (tmp = ban_list; tmp; tmp = tmp->next)
  {
    if(!str_cmp(name, tmp->name))
    {
      send_to_char("That site is already banned!\n", ch);
      return;
    }
  }
  if(!strn_cmp("localhost", name, strlen(name)))
  {
    send_to_char("'localhost' may not be banned.\n", ch);
    return;
  }

  CREATE(tmp, struct ban_t, 1, MEM_TAG_BAN);
  CREATE(tmp->name, char, strlen(GET_NAME(ch)) + 1, MEM_TAG_STRING);
  CREATE(tmp->ban_str, char, strlen(name) + 1, MEM_TAG_STRING);

  strcpy(tmp->name, GET_NAME(ch));
  strcpy(tmp->ban_str, name);
  tmp->lvl = MIN(62, GET_LEVEL(ch));

  tmp->next = ban_list;
  ban_list = tmp;
  save_ban_file();
}

void do_allow(P_char ch, char *argument, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  struct ban_t *curr, *prev;

  if(!IS_TRUSTED(ch))
    return;

  one_argument(argument, name);

  if(!*name)
  {
    send_to_char("Remove which string from the ban list?\n", ch);
    return;
  }
  if(!str_cmp(name, "null"))
  {
    bounce_null_sites = 0;
    send_to_char("Null sites will now not be bounced.\n", ch);
    return;
  }
  if(ban_list == NULL)
  {
    send_to_char("No sites are banned currently.\n", ch);
    return;
  }
  curr = prev = ban_list;
  if(!str_cmp(curr->ban_str, name))
  {
    if(curr->lvl > GET_LEVEL(ch))
    {
      send_to_char
        ("Sorry, you are not high enough level to remove that ban.\n", ch);
      return;
    }
    ban_list = ban_list->next;
    FREE(curr->ban_str);
    FREE(curr->name);
    FREE(curr);
    curr = NULL;
    send_to_char("Ok.\n", ch);
    logit(LOG_WIZ, "(%s) allow %s", ch->player.name, argument);
    save_ban_file();
    return;
  }
  curr = curr->next;
  while (curr)
  {
    if(!str_cmp(curr->ban_str, name))
    {
      if(curr->lvl > GET_LEVEL(ch))
      {
        send_to_char
          ("Sorry, you are not high enough level to remove that ban.\n", ch);
        return;
      }
      if(curr->next)
      {
        prev->next = curr->next;
        FREE(curr->name);
        FREE(curr->ban_str);
        FREE(curr);
        curr = NULL;
        send_to_char("Ok.\n", ch);
        save_ban_file();
        return;
      }
      prev->next = (struct ban_t *) NULL;
      FREE(curr->name);
      FREE(curr->ban_str);
      FREE(curr);
      curr = NULL;
      send_to_char("Ok.\n", ch);
      sprintf(buf, "WIZ: (%s) allow %s", ch->player.name, argument);
      logit(LOG_WIZ, buf);
      save_ban_file();
      return;
    }
    curr = curr->next;
    prev = prev->next;
  }
  send_to_char("String not found in list!\n", ch);
}

void do_secret(P_char ch, char *argument, int cmd)
{
  P_obj    obj = NULL;
  P_char   dummy;
  char     buf[MAX_STRING_LENGTH];

  one_argument(argument, buf);
  if(generic_find(buf, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &dummy, &obj))
  {
    if(IS_SET(obj->extra_flags, ITEM_SECRET))
    {
      REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
      send_to_char("SECRET bit removed.\n", ch);
    }
    else
    {
      SET_BIT(obj->extra_flags, ITEM_SECRET);
      send_to_char("SECRET bit set.\n", ch);
    }
  }
  else
    send_to_char("Secretize what?\n", ch);
}

/*
 * Look up an object based on a keyword
 *
 * Syntax : "lookup <room | mob | obj | random> <search_string>"
 */
void do_lookup(P_char ch, char *argument, int cmd)
{
  FILE    *fp;
  char    *irc;
  char     arg[MAX_STRING_LENGTH], pattern[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH], file[MAX_INPUT_LENGTH];
  char     o_buf[MAX_STRING_LENGTH];
  int      found, length;

  if(IS_NPC(ch) || !ch->desc)
    return;

  half_chop(argument, arg, buf);
//  one_argument(buf, pattern);
  o_buf[0] = 0;
  strcpy(pattern, buf);


  if(!*arg || !*pattern)
  {
    send_to_char
      ("Syntax : lookup <pfile | room | zone | mob | obj | random> <pattern>\n",
       ch);
    return;
  }
  /*
   * Search the files created via the gawk script (during boot), * for
   * the desired pattern.  Output goes to a temp file, which * is then
   * read in.
   */

  if(strncasecmp(arg, "random", 1) == 0)
  {

    if(strncasecmp(pattern, "zone", 1) == 0 && GET_LEVEL(ch) >= FORGER)
    {
      display_random_zones(ch);
      return;
    }
  }
  if(strncasecmp(arg, "mob", 1) == 0 || strncasecmp(arg, "char", 1) == 0)
  {
    strcpy(file, MOB_LOOKUP);
  }
  else if(strncasecmp(arg, "obj", 1) == 0)
  {
    strcpy(file, OBJ_LOOKUP);
  }
  else if(strncasecmp(arg, "room", 1) == 0)
  {
    strcpy(file, WLD_LOOKUP);
  }
  else if(strncasecmp(arg, "zone", 1) == 0)
  {
    strcpy(file, ZON_LOOKUP);
  }
  else if(strncasecmp(arg, "pfile", 1) == 0)
  {
    char     m_class[MAX_STRING_LENGTH];
    char     race[MAX_STRING_LENGTH];
    char     level[MAX_STRING_LENGTH];
    char     start_letter[MAX_STRING_LENGTH];

    half_chop(pattern, start_letter, buf);
    if(isname("*", start_letter))
    {
      send_to_char
        ("start letter must be a or b etc can't use * (Couse of lag)\n", ch);
      return;
    }

    half_chop(buf, level, pattern);
    half_chop(pattern, m_class, buf);
    half_chop(buf, race, pattern);
    sprintf(buf,
            "&+RQuery:\n&+WFind all ch with first letter:&+R%s&+W Level:&+R%s&+W Class:&+R%s&+W Race:&+R%s&+W\n",
            start_letter, level, m_class, race);
    send_to_char(buf, ch);
    if(!*start_letter || !*level || !*race || !*m_class)
    {
      send_to_char("Syntax : 'lookup pfile a 50 Human Warrior'\n", ch);
      send_to_char("Syntax : 'lookup pfile d * Ogre *'\n", ch);
      return;
    }

    FILE    *flist, *f;
    char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
    char     Gbuf3[MAX_STRING_LENGTH];
    char     buffer[MAX_STRING_LENGTH];
    char     tbuf[MAX_STRING_LENGTH];
    char     tbuf2[MAX_STRING_LENGTH];

    int      how_many = 0;
    P_char   owner;


    sprintf(buf, "&+W%-12s %s %-10s\t %-10s&n\n", "Name", "Lev", "Class",
            "Race");
    send_to_char(buf, ch);

    sprintf(Gbuf3, "/bin/ls -1 Players/%s > %s", start_letter,
            "temp_letterfile");
    system(Gbuf3);              /* ls a list of Players into the temp_file */
    flist = fopen("temp_letterfile", "r");
    if(!flist)
      return;

    while (fscanf(flist, " %s \n", Gbuf2) != EOF)
    {
      owner = (struct char_data *) mm_get(dead_mob_pool);
      owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

      if(restoreCharOnly(owner, skip_spaces(Gbuf2)) >= 0)
      {
        stripansi_2(race_to_string(owner), tbuf);
        half_chop(tbuf, tbuf, buf);
        stripansi_2(get_class_string(owner, buffer), tbuf2);

        tbuf2[strlen(tbuf2) - 1] = '\0';
        if(GET_LEVEL(owner) == (atoi(level)) || isname("*", level))
        {                       // LEVEL
          if(isname(tbuf2, m_class) || isname("*", m_class))
          {                     //CLASS
            if(isname(tbuf, race) || isname("*", race))
            {                   // RACE
              how_many++;
              sprintf(buf, "%-12s %d\t %-10s\t %-10s\n", GET_NAME(owner),
                      GET_LEVEL(owner), get_class_string(owner, buffer),
                      race_to_string(owner));
              send_to_char(buf, ch);
              if(how_many > 20)
              {
                send_to_char("To many results narrow down your search", ch);
                fclose(flist);
                return;
              }                 // End spam check


            }                   //end race
          }                     //end class


        }                       // end Level

      }                         //end restore
    }                           //End while
    fclose(flist);
    return;
  }
  else
  {
    send_to_char
      ("Syntax : lookup <pfile room | zone | mob | obj> <pattern>\n", ch);
    return;
  }

  if((fp = fopen(file, "r")) == NULL)
  {
    sprintf(buf, "Error opening %s", file);
    logit(LOG_FILE, buf);
    sprintf(buf, "Error opening %s...tell an implementor.\n", file);
    send_to_char(buf, ch);
    return;
  }
  else
  {
    /* Read in each line of the file.  See if the pattern
       is in the line.  If so, pass it to the user. */
    found = 0;
    length = 0;
    strToLower(pattern);        /* lower case all values for comparison */
    do
    {
      irc = fgets(buf, MAX_STRING_LENGTH - 1, fp);
      strcpy(arg, buf);
      strcpy(buf, strip_ansi(buf).c_str());
      strToLower(buf);
      if(strstr(buf, pattern) != NULL)
      {
        found = 1;
        if((length + strlen(arg) + 40) > MAX_STRING_LENGTH)
        {
          strcat(o_buf, "...and the list goes on...\n");
          irc = 0;
        }
        else
        {
          length += strlen(arg) + 1;
          strcat(o_buf, arg);
          strcat(o_buf, "");
        }
      }
    }
    while (irc > 0);

    fclose(fp);

    if(!found)
    {
      sprintf(buf, "No matches found for pattern '%s'\n", pattern);
      send_to_char(buf, ch);
    }
    else
    {
      page_string(ch->desc, o_buf, 1);
    }
  }
}

/* wiz command to show total amount of money in the game.
   EXTREMELY SLOW!! Do not even think about running it on the running mud! */
void do_money_supply(P_char ch, char *argument, int cmd)
{
    char buff[MAX_STRING_LENGTH];
    char guild_name[MAX_STRING_LENGTH];
    P_char tch;
    FILE *flist, *f;

    if(!ch || IS_NPC(ch) || !IS_TRUSTED(ch))
       return;

    long  total_p = 0;
    long  total_g = 0;
    long  total_s = 0;
    long  total_c = 0;
    long p, g, s, c;

    send_to_char("This will take awhile, please be patient...\r\n", ch);

    // find association cash
    send_to_char("Associations:\r\n", ch);

    sprintf(buff, "/bin/ls -1 Players/Assocs/*.? > %s", "temp_assocsfile");
    system(buff);

    flist = fopen("temp_assocsfile", "r");

    if(!flist) 
    {
       send_to_char("error reading association files.\r\n", ch);
       fclose(flist);
       system("rm -f temp_assocsfile");
    }
    else 
    {
       while(fscanf(flist, " %s \n", buff) != EOF)
       {
          f = fopen(buff, "r");

          if(!f)
            continue;

          fgets(guild_name, MAX_STR_NORMAL, f);
          for(int i = 0; i < 9; i++)
          {
             fgets(buff, MAX_STR_NORMAL, f);
          }

          fscanf(f, "%ld %ld %ld %ld\n", &p, &g, &s, &c);
          sprintf(buff, "%s: &+W%ld p &+Y%ld g &+w%ld s &+y%ld c\r\n", guild_name, p, g, s, c);
          send_to_char(buff, ch);
          total_p += p;
          total_g += g;
          total_s += s;
          total_c += c;
          fclose(f);
       }

       fclose(flist);
       system("rm -f temp_assocsfile");
    }

    // find player cash
    send_to_char("Players with more than 10k plat:\r\n", ch);
    for(int l = 97; l < 123; l++) 
    {
       sprintf(buff, "/bin/ls -1 Players/%c > %s", l, "temp_letterfile");
       system(buff);
 
       flist = fopen("temp_letterfile", "r");
       if(!flist) 
       {
          sprintf(buff, "error reading from letter file %c\r\n", l);
          send_to_char(buff, ch);
          fclose(flist);
          continue;
       }
       while(fscanf(flist, " %s \n", buff) != EOF) 
       {
          tch = (struct char_data *) mm_get(dead_mob_pool);
          tch->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

          if(restoreCharOnly(tch, skip_spaces(buff)) >= 0) 
          {
             if(IS_TRUSTED(tch))
                 continue;

             p = GET_PLATINUM(tch) + GET_BALANCE_PLATINUM(tch);
             g = GET_GOLD(tch) + GET_BALANCE_GOLD(tch);
             s = GET_SILVER(tch) + GET_BALANCE_SILVER(tch);
             c = GET_COPPER(tch) + GET_BALANCE_COPPER(tch);
             if(((1000*p) + (100*g) + (10*s) + (c)) >= 10000000)
             {
                sprintf(buff, "%s: &+W%ld p &+Y%ld g &+w%ld s &+y%ld c\r\n", GET_NAME(tch), p, g, s, c);
                send_to_char(buff, ch);
             }

             total_p += p;
             total_g += g;
             total_s += s;
             total_c += c;
          }
       }
       fclose(flist);
    }
    system("rm -f temp_letterfile");
    sprintf(buff, "\r\nTotal money in game: &+W%ld platinum, &+Y%ld gold, &+w%ld silver, &+y%ld copper\r\n", total_p, total_g, total_s, total_c);
    send_to_char(buff, ch);
}

void read_ban_file(void)
{
  FILE    *f;
  char     buf[MAX_STRING_LENGTH];
  int      tmp;
  struct ban_t *ban;

  f = fopen(BAN_FILE, "r");
  if(!f)
  {
    logit(LOG_FILE, "Could not open %s to read ban info.\n", BAN_FILE);
    return;
  }
  while (fscanf(f, "%s\n", buf) != EOF)
  {
    CREATE(ban, struct ban_t, 1, MEM_TAG_BAN);
    CREATE(ban->name, char, sizeof(buf) + 1, MEM_TAG_STRING);

    strcpy(ban->name, buf);
    fscanf(f, "%d\n", &tmp);
    ban->lvl = tmp;
    fscanf(f, "%s\n", buf);
    CREATE(ban->ban_str, char, sizeof(buf) + 1, MEM_TAG_STRING);

    strcpy(ban->ban_str, buf);
    ban->next = ban_list;
    ban_list = ban;
  }
  fclose(f);
}

void save_ban_file(void)
{
  struct ban_t *i;
  FILE    *f;

  f = fopen(BAN_FILE, "w");
  if(!f)
  {
    logit(LOG_FILE, "Could not open %s to save ban info.\n", BAN_FILE);
    return;
  }
  for (i = ban_list; i; i = i->next)
  {
    fprintf(f, "%s\n", i->name);
    fprintf(f, "%d\n", i->lvl);
    fprintf(f, "%s\n", i->ban_str);
  }
  fclose(f);
}


void read_wizconnect_file(void)
{
  FILE    *f;
  char     buf[MAX_STRING_LENGTH];
  int      tmp;
  struct wizban_t *ban;

  f = fopen(WIZCONNECT_FILE, "r");
  if(!f)
  {
    logit(LOG_FILE, "Could not open %s to read wizconnect info.\n",
          WIZCONNECT_FILE);
    return;
  }
  while (fscanf(f, "%s\n", buf) != EOF)
  {
    CREATE(ban, struct wizban_t, 1, MEM_TAG_WIZBAN);
    CREATE(ban->name, char, sizeof(buf) + 1, MEM_TAG_STRING);

    strcpy(ban->name, buf);
    fscanf(f, "%s\n", buf);
    CREATE(ban->ban_str, char, sizeof(buf) + 1, MEM_TAG_STRING);

    strcpy(ban->ban_str, buf);
    ban->next = wizconnect;
    wizconnect = ban;
  }
  fclose(f);
}



void save_wizconnect_file(void)
{
  struct wizban_t *i;
  FILE    *f;

  f = fopen(WIZCONNECT_FILE, "w");
  if(!f)
  {
    logit(LOG_FILE, "Could not open %s to save wizconnect info.\n",
          WIZCONNECT_FILE);
    return;
  }
  for (i = wizconnect; i; i = i->next)
  {
    fprintf(f, "%s\n", i->name);
    fprintf(f, "%s\n", i->ban_str);
  }
  fclose(f);
}


void do_ptell(P_char ch, char *arg, int cmd)
{

  struct descriptor_data *d;
  P_char   vict;

//  P_desc d;

  char     name[MAX_INPUT_LENGTH], msg[MAX_STRING_LENGTH],
    Gbuf1[MAX_STRING_LENGTH];

  if(!(SanityCheck(ch, "do_ptell")))
  {
    logit(LOG_DEBUG, "do_ptell failed SanityCheck");
    return;
  }
  if(IS_NPC(ch))
  {
    send_to_char("You try. . . . but you fail miserably.\n", ch);
    return;
  }
  half_chop(arg, name, msg);

  if(!*name || !*msg)
  {
    send_to_char
      ("To whom are you responding? And what are you telling them?\n", ch);
    return;
  }
/*
  if(!(vict = get_char(name))) {
    send_to_char("Nobody by that name seems available.\n", ch);
    return;
  }
*/
  for (d = descriptor_list; d; d = d->next)
  {
    if(!d->character || d->connected || !d->character->player.name)
      continue;
    if(!isname(d->character->player.name, name))
      continue;
    if(!CAN_SEE_Z_CORD(ch, d->character))
      continue;
    vict = d->character;
    break;
  }

  if(!d || !vict)
  {
    send_to_char("Nobody by that name seems available.\n", ch);
    return;
  }
  if(IS_TRUSTED(vict) && (GET_WIZINVIS(vict) >= MIN(62, GET_LEVEL(ch))))
  {
    send_to_char("Nobody by that name seems available.\n", ch);
    return;
  }
  if(ch == vict)
  {
    send_to_char
      ("You try to tell yourself something. Everybody cares. Really.\n", ch);
    return;
  }
  if(IS_NPC(vict))
  {
    send_to_char("What's the point?\n", ch);
    return;
  }
  if(!vict->desc)
  {
    send_to_char("That person can't hear you.\n", ch);
    return;
  }
  if(ch->desc)
  {
    if(IS_SET(ch->specials.act, PLR_ECHO))
    {
      sprintf(Gbuf1, "&+rYou ptell %s '&+R%s&n&+r'\n", GET_NAME(vict), msg);
      send_to_char(Gbuf1, ch, LOG_PRIVATE);
    }
    else
      send_to_char("Ok.\n", ch);
  }
  sprintf(Gbuf1, "&+r%s responds to your petition with '&+R%s&n&+r'\n",
          (CAN_SEE(vict, ch) && IS_TRUSTED(ch)) ? ch->player.name :
          IS_TRUSTED(vict) ? ch->player.name : "Someone", msg);
  send_to_char(Gbuf1, vict, LOG_PRIVATE);
  logit(LOG_PETITION, "(%s) ptells (%s): (%s).", GET_NAME(ch), GET_NAME(vict), msg);
  
  if(get_property("logs.chat.status", 0.000) && IS_PC(ch) && IS_PC(vict))
    logit(LOG_CHAT, "%s ptells %s '%s'", GET_NAME(ch), GET_NAME(vict), msg);

  for (d = descriptor_list; d; d = d->next)
  {
    if((STATE(d) == CON_PLYNG) && IS_TRUSTED(d->character) &&
        IS_SET(d->character->specials.act, PLR_PETITION) &&
        (d->character != vict) && (d->character != ch))
    {
      sprintf(Gbuf1, "&+rPetition: %s responds to %s with '&+R%s&n&+r'.\n",
              CAN_SEE(d->character, ch) ? GET_NAME(ch) : "Someone",
              GET_NAME(vict), msg);
      send_to_char(Gbuf1, d->character, LOG_PRIVATE);
    }
  }

  return;
}


void GetMIA(char *arg, char *returned)
{
  unsigned long laston, timegone;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[512];
  P_char   finger_foo;

  if(!*arg || !arg)
  {
    sprintf(returned, "NoArgs");
    return;
  }
  finger_foo = (struct char_data *) mm_get(dead_mob_pool);
  finger_foo->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

  if(restoreCharOnly(finger_foo, skip_spaces(arg)) < 0 || !finger_foo)
  {
    if(finger_foo)
      free_char(finger_foo);
    sprintf(returned, "NoPfile");
    return;
  }
  timegone = (time(0) - laston) / 60;

  if(timegone > 10)
  {
    strcpy(returned, "  &n&+c(MIA:&n ");
    if(timegone > 1440)
      sprintf(returned + strlen(returned), "%d day%s, ",
              (int) (timegone / 1440), ((timegone / 1440) > 1) ? "s" : "");
    if((timegone % 1440) > 60)
      sprintf(returned + strlen(returned), "%d hour%s, ",
              (int) (timegone % 1440) / 60,
              (((timegone % 1440) / 60) > 1) ? "s" : "");
    if(timegone % 60)
      sprintf(returned + strlen(returned), "%d minute%s, ",
              (int) (timegone % 60), ((timegone % 60) > 1) ? "s" : "");
    returned[strlen(returned) - 2] = 0;
    strcat(returned, "&n)");
  }
  else
    *returned = '\0';
  return;
}

void do_finger(P_char ch, char *arg, int cmd)
{
  unsigned long timegone;
  time_t   laston;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[512];
  P_char   finger_foo;

  if(!*arg || !arg)
  {
    send_to_char("Usage:\n  finger playername.\n", ch);
    return;
  }
  finger_foo = (struct char_data *) mm_get(dead_mob_pool);
  finger_foo->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

  if(restoreCharOnly(finger_foo, skip_spaces(arg)) < 0 || !finger_foo)
  {
    if(finger_foo)
      free_char(finger_foo);
    send_to_char("Pfile does not exist or is invalid.\n", ch);
    return;
  }
  if(GET_LEVEL(finger_foo) > GET_LEVEL(ch))
  {
    send_to_char("Sorry, you cannot finger those higher level than you.\n",
                 ch);
    if(finger_foo)
      free_char(finger_foo);
    return;
  }
  sprintf(Gbuf1,
          "&+cName:&n %s %s &n&+cLevel:&n %d &+cClass:&n %s &n&+cRace:&n %s\n",
          GET_NAME(finger_foo), GET_TITLE(finger_foo), GET_LEVEL(finger_foo),
          get_class_string(finger_foo, Gbuf2),
          race_names_table[(int) GET_RACE(finger_foo)].ansi);
  laston = finger_foo->player.time.saved;
  timegone = (time(0) - laston) / 60;
  send_to_char(Gbuf1, ch);
  sprintf(Gbuf1, "&+cLast saved:&n %s", asctime(localtime(&laston)));
  Gbuf1[strlen(Gbuf1) - 1] = 0;
  send_to_char(Gbuf1, ch);
  send_to_char("\n", ch);
  time_t lastConnect = 0, lastDisconnect = 0;
  strcpy(Gbuf1, "unrecorded IP");
  sql_select_IP_info(finger_foo, Gbuf1, sizeof(Gbuf1), &lastConnect, &lastDisconnect);
  if(lastConnect >= lastDisconnect)
  {
    send_to_char("&+cLast played from: &n", ch);
  }
  else
  {
    send_to_char("&+cPlaying from: &n", ch);
  }
  send_to_char(Gbuf1, ch);

  Gbuf1[0] = '\0';
  if(lastConnect >= lastDisconnect)
  {
    if((lastConnect == 0) && (lastDisconnect == 0))
      timegone = (time(0) - laston);
    else
      timegone = lastDisconnect;

    int days = (timegone / (3600 * 24)),
        hours = (timegone  % (3600 * 24)) / 3600,
        minutes = (timegone % 3600) / 60,
        seconds = (timegone % 60);

    send_to_char("  &+c(MIA:&n ", ch);
    if(days)
      sprintf(Gbuf1 + strlen(Gbuf1), "%d day%s, ",
              (int) days,
              (days > 1) ? "s" : "");
    if(hours)
      sprintf(Gbuf1 + strlen(Gbuf1), "%d hour%s, ",
              (int) hours,
              (hours > 1) ? "s" : "");
    if(minutes)
      sprintf(Gbuf1 + strlen(Gbuf1), "%d minute%s, ",
              (int) minutes, (minutes > 1) ? "s" : "");
    // display seconds only if there are no hours/minutes
    if(timegone < 60)
      sprintf(Gbuf1 + strlen(Gbuf1), "%d second%s, ",
              (int) seconds, (seconds > 1) ? "s" : "");
  }
  else
  {
    timegone = lastConnect;
    int hours = (timegone  / 3600),
        minutes = (timegone % 3600) / 60,
        seconds = (timegone % 60);

    send_to_char("  &+c(Playing:&n ", ch);
    if(hours)
      sprintf(Gbuf1 + strlen(Gbuf1), "%d hour%s, ",
              (int) hours,
              (hours > 1) ? "s" : "");
    if(minutes)
      sprintf(Gbuf1 + strlen(Gbuf1), "%d minute%s, ",
              (int) minutes, (minutes > 1) ? "s" : "");
    // display seconds only if there are no hours/minutes
    if(timegone < 60)
      sprintf(Gbuf1 + strlen(Gbuf1), "%d second%s, ",
              (int) seconds, (seconds > 1) ? "s" : "");
  }
  Gbuf1[strlen(Gbuf1) - 2] = 0;
  strcat(Gbuf1, "&+c)&n\n");
  send_to_char(Gbuf1, ch);

  sprintf(Gbuf1,
          "\n&+cLast Rented: &n%s&n &+W[&+C%d&+W]\n&n&+cBirthplace: &n%s&n &+W[&+C%d&+W]&n\n",
          world[real_room0(GET_HOME(finger_foo))].name, GET_HOME(finger_foo),
          world[real_room0(GET_BIRTHPLACE(finger_foo))].name,
          GET_BIRTHPLACE(finger_foo));
  send_to_char(Gbuf1, ch);
  if(finger_foo)
    free_char(finger_foo);
}

void do_decline(P_char ch, char *arg, int cmd)
{
  char     Gbuf2[MAX_STRING_LENGTH], f_a[MAX_STRING_LENGTH],
    Gbuf1[MAX_STRING_LENGTH];
  P_desc   d, i;

  arg = skip_spaces(arg);
  if(!*arg || !arg)
  {
    send_to_char("Usage: decline <charname> [reason]\n", ch);
    return;
  }
  arg = one_argument(arg, f_a);
  for (d = descriptor_list; d; d = d->next)
    if(STATE(d) == CON_ACCEPTWAIT && d->character &&
        !str_cmp(GET_NAME(d->character), f_a))
    {
      if(!arg || !*arg)
      {
        SEND_TO_Q
          ("\n\nYour new character application has been declined. It is highly probable\n",
           d);
        SEND_TO_Q
          ("that either your name does not suit fantasy theme, or something else is\n"
           "inappropriate to the theme Duris strives to maintain.\n\n", d);
        SEND_TO_Q("Please enter another, more suitable fantasy name:", d);
      }
      else
      {
        SEND_TO_Q
          ("\n\nYour new character application has been declined.\nReason supplied was:",
           d);
        SEND_TO_Q(arg, d);
        SEND_TO_Q("\n\nPlease enter another, more suitable fantasy name:", d);
      }
      d->character->only.pc->prestige = 0;
      logit(LOG_NEWCHAR, "%s declined new char %s (%s): %s.",
            GET_NAME(ch), GET_NAME(d->character),
            (d->host ? d->host : "UNKNOWN"), (arg ? arg : "NO REASON GIVEN"));
      sprintf(Gbuf1, "&+c*** STATUS: %s declined new player %s. (%s)\n",
              GET_NAME(ch), GET_NAME(d->character), arg);
      sprintf(Gbuf2, "&+c*** STATUS: Someone declined new player %s. (%s)\n",
              GET_NAME(d->character), arg);
      for (i = descriptor_list; i; i = i->next)
        if(!i->connected && i->character &&
            IS_SET(i->character->specials.act, PLR_PETITION) &&
            IS_TRUSTED(i->character))
        {
          if(!CAN_SEE(i->character, ch))
            send_to_char(Gbuf2, i->character);
          else
            send_to_char(Gbuf1, i->character);
//    deny_name(GET_NAME(d->character));
          STATE(d) = CON_NEW_NAME;
        }

      deny_name(GET_NAME(d->character));

      return;
    }
  send_to_char("Decline what new player's application?\n", ch);
}

extern int accept_mode;

void do_accept(P_char ch, char *arg, int cmd)
{
  int      i;
  P_desc   d, j;
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf1[MAX_STRING_LENGTH];
  const char *accept_modes[] = {
    "off",
    "on",
    "\n"
  };

  if(!*arg || !arg)
  {
    /* list characters needing approval:  */
    i = 0;
    sprintf(Gbuf1, "&+cAC: Post-creation approval system is now %s.\n",
            accept_modes[accept_mode]);
    send_to_char(Gbuf1, ch);
    if(accept_mode)
    {
      send_to_char("List of characters needing approval:\n", ch);
      for (d = descriptor_list; d; d = d->next)
        if(STATE(d) == CON_ACCEPTWAIT)
        {
          sprintf(Gbuf1, "%s (%s %s %s)\n",
                  GET_NAME(d->character), get_class_string(d->character,
                                                           Gbuf2),
                  race_names_table[(int) GET_RACE(d->character)].ansi,
                  d->host ? d->host : "UNKNOWN");
          send_to_char(Gbuf1, ch);
          i++;
        }
      if(!i)
        send_to_char("None.\n\n", ch);
    }
    send_to_char("Usage: accept <charname|on|off>\n", ch);
    return;
  }
  arg = skip_spaces(arg);
  switch (search_block(arg, accept_modes, FALSE))
  {
  case 0:

    if(GET_LEVEL(ch) < 62)
    {
      send_to_char("Sorry, that option is overlord only.\n", ch);
      return;
    }
    accept_mode = 0;

    sprintf(Gbuf1, "&+cAC: %s set newchar application system %s.\n",
            GET_NAME(ch), accept_modes[accept_mode]);
    sprintf(Gbuf2, "&+CAC: Someone set newchar application system %s.\n",
            accept_modes[accept_mode]);
    logit(LOG_WIZ, Gbuf1);
    for (j = descriptor_list; j; j = j->next)
      if(!j->connected && j->character &&
          IS_SET(j->character->specials.act, PLR_PETITION) &&
          IS_TRUSTED(j->character))
        if(!CAN_SEE(j->character, ch))
          send_to_char(Gbuf2, j->character);
        else
          send_to_char(Gbuf1, j->character);
    break;
  case 1:

    if(GET_LEVEL(ch) < 62)
    {
      send_to_char("Sorry, that option is overlord only.\n", ch);
      return;
    }
    accept_mode = 1;

    sprintf(Gbuf1, "&+cAC: %s set newchar application system %s.\n",
            GET_NAME(ch), accept_modes[accept_mode]);
    logit(LOG_WIZ, Gbuf1);
    sprintf(Gbuf2, "&+cAC: Someone set newchar application system %s.\n",
            accept_modes[accept_mode]);
    for (j = descriptor_list; j; j = j->next)
      if(!j->connected && j->character &&
          IS_SET(j->character->specials.act, PLR_PETITION) &&
          IS_TRUSTED(j->character))
        if(!CAN_SEE(j->character, ch))
          send_to_char(Gbuf2, j->character);
        else
          send_to_char(Gbuf1, j->character);
    break;
  case -1:
    for (d = descriptor_list; d; d = d->next)
      if(STATE(d) == CON_ACCEPTWAIT && d->character &&
          !str_cmp(GET_NAME(d->character), arg))
      {
        logit(LOG_NEWCHAR, "%s accepted new char %s from %s.",
              GET_NAME(ch), GET_NAME(d->character),
              (d->host ? d->host : "UNKNOWN"));
        sprintf(Gbuf1, "&+c*** STATUS: %s accepted new player %s from %s.\n",
                GET_NAME(ch), GET_NAME(d->character),
                d->host ? d->host : "&+WUNKNOWN&n");
        sprintf(Gbuf2,
                "&+c*** STATUS: Someone accepted new player %s from %s.\n",
                GET_NAME(d->character), d->host ? d->host : "&+WUNKNOWN&n");

        for (j = descriptor_list; j; j = j->next)
          if(!j->connected && j->character &&
              IS_SET(j->character->specials.act, PLR_PETITION) &&
              IS_TRUSTED(j->character))
            if(!CAN_SEE(j->character, ch))
              send_to_char(Gbuf2, j->character);
            else
              send_to_char(Gbuf1, j->character);
        SEND_TO_Q
          ("\nYour application for character has been accepted. Welcome into ranks of\nthe players of Duris!\n\n",
           d);
        SEND_TO_Q("\n*** PRESS RETURN:\n", d);
        STATE(d) = CON_WELCOME;

        accept_name(GET_NAME(d->character));

        return;
      }
    send_to_char("No such player in the newplayer-queue! \n", ch);
    break;
  }
}

void do_invite(P_char ch, char *arg, int cmd)
{
  char     f_a[MAX_STRING_LENGTH];

  arg = skip_spaces(arg);
  if(!*arg || !arg)
  {
    send_to_char("Usage: invite <charname> or invite <on|off>\n", ch);
    return;
  }

  if(*arg && (isname(arg, "on") || isname(arg, "off")))
  {
    if(isname(arg, "on"))
    {
      invitemode = 1;
      wizlog(57, "%s has turned on Evil Invite", GET_NAME(ch));
      return;
    }
    else
    {
      invitemode = 0;
      wizlog(57, "%s has turned off Evil Invite", GET_NAME(ch));
      return;
    }
  }

  arg = one_argument(arg, f_a);

  create_denied_file("Players/Invited", f_a);

  send_to_char("Invited.\n", ch);

  wizlog(GET_LEVEL(ch), "%s has invited %s", GET_NAME(ch), f_a);
}

void do_uninvite(P_char ch, char *arg, int cmd)
{
  char     f_a[MAX_STRING_LENGTH], path[2048];

  arg = skip_spaces(arg);
  if(!*arg || !arg)
  {
    send_to_char("Usage: uninvite <charname>\n", ch);
    return;
  }

  arg = one_argument(arg, f_a);

  sprintf(path, "Players/Invited/%c/%s", f_a[0], f_a);

  // -1 is failure, 0 is success.
  if( unlink(path) == -1)
  {
    send_to_char("Failed.\n\r", ch);
    debug( "Couldn't delete file '%s'.", path );
    return;
  }

  send_to_char("Uninvited.\n", ch);

  wizlog(GET_LEVEL(ch), "%s has uninvited %s", GET_NAME(ch), f_a);
}

/* clone stuff - Valkur */

struct obj_data *clone_obj(P_obj obj)
{
  P_obj    ocopy;
  int      i;

  ocopy = read_object(obj->R_num, REAL);
  /* copy  */
  if(obj->name)
  {
    if(IS_SET(obj->str_mask, STRUNG_KEYS))
    {
      ocopy->name = str_dup(obj->name);
      SET_BIT(ocopy->str_mask, STRUNG_KEYS);
    }
  }
  if(obj->short_description)
  {
    if(IS_SET(obj->str_mask, STRUNG_DESC2))
    {
      ocopy->short_description = str_dup(obj->short_description);
      SET_BIT(ocopy->str_mask, STRUNG_DESC2);
    }
  }
  if(obj->description)
  {
    if(IS_SET(obj->str_mask, STRUNG_DESC1))
    {
      ocopy->description = str_dup(obj->description);
      SET_BIT(ocopy->str_mask, STRUNG_DESC1);
    }
  }
  if(obj->action_description)
  {
    if(IS_SET(obj->str_mask, STRUNG_DESC3))
    {
      ocopy->action_description = str_dup(obj->action_description);
      SET_BIT(ocopy->str_mask, STRUNG_DESC3);
    }
  }

  for (i = 0; i <= NUMB_OBJ_VALS; i++)
    ocopy->value[i] = obj->value[i];

  for (i = 0; i < 4; i++)
    ocopy->timer[i] = obj->timer[i];

  ocopy->wear_flags = obj->wear_flags;
  ocopy->extra_flags = obj->extra_flags;
  ocopy->anti_flags = obj->anti_flags;
  ocopy->anti2_flags = obj->anti2_flags;
  ocopy->extra2_flags = obj->extra2_flags;
  ocopy->weight = obj->weight;
  ocopy->cost = obj->cost;
  ocopy->trap_eff = obj->trap_eff;
  ocopy->trap_dam = obj->trap_dam;
  ocopy->trap_charge = obj->trap_charge;
  ocopy->trap_level = obj->trap_level;
  ocopy->condition = obj->condition;
  //ocopy->max_condition = obj->max_condition; wipe2011
  ocopy->material = obj->material;
  ocopy->craftsmanship = obj->craftsmanship;
  ocopy->bitvector = obj->bitvector;
  ocopy->bitvector2 = obj->bitvector2;
  ocopy->bitvector3 = obj->bitvector3;
  ocopy->bitvector4 = obj->bitvector4;
  ocopy->bitvector5 = obj->bitvector5;

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    ocopy->affected[i].location = obj->affected[i].location;
    ocopy->affected[i].modifier = obj->affected[i].modifier;
  }

  return ocopy;
}

void clone_container_obj(P_obj to, P_obj obj)
{
  P_obj    tmp, ocopy;

  for (tmp = obj->contains; tmp; tmp = tmp->next_content)
  {
    ocopy = clone_obj(tmp);
    if(tmp->contains)
      clone_container_obj(ocopy, tmp);
    obj_to_obj(ocopy, to);
  }
}

void do_clone(P_char ch, char *argument, int cmd)
{
  P_char   mob, mcopy;
  P_obj    obj, ocopy;
  char     type[MAX_STRING_LENGTH];
  char     name[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  int      i, j, count, where;

  if(IS_NPC(ch))
  {
    send_to_char("Uh, no you can't clone something as a mob.\n", ch);
    return;
  }
  argument = one_argument(argument, type);
  if(!*type)
  {
    send_to_char("Usage: clone <mob|obj> <name> <count>\n", ch);
    return;
  }
  argument = one_argument(argument, name);
  if(!*name)
  {
    send_to_char("Usage: clone <mob|obj> <name> <count>\n", ch);
    return;
  }
  argument = one_argument(argument, buf);
  if(!*buf)
    count = 1;
  else
    count = atoi(buf);
  if(!count)
  {
    send_to_char("No count specified!  Assuming 1.\n", ch);
    count = 1;
  }
  /* keep them from doing too many on accident ;)  */
  if(count > 100)
  {
    send_to_char("Max count set to 100 so Pook can't do it 2000 times. ;)\n",
                 ch);
    return;
  }
  if(is_abbrev(type, "mobile") || is_abbrev(type, "character"))
  {
    if((mob = get_char_room_vis(ch, name)) == 0)
    {
      send_to_char("Can't find any such mobile!\n", ch);
      return;
    }
    if(IS_PC(mob))
    {
      send_to_char("Cloning a PC?!?!? Buahahahaha!!!!\n", ch);
      send_to_char(GET_NAME(ch), mob);
      send_to_char(" just tried to clone YOU...*laugh*\n", mob);
      return;
    }
    if(GET_RNUM(mob) < 0)
    {
      send_to_char("You can't clone that mob!!\n", ch);
      return;
    }
    for (i = 0; i < count; i++)
    {
      if(!(mcopy = read_mobile(GET_RNUM(mob), REAL)))
        break;

      /* copy    */
      if(mob->player.name)
      {
        if(IS_SET(mob->only.npc->str_mask, STRUNG_KEYS))
        {
          mcopy->player.name = str_dup(mob->player.name);
          SET_BIT(mcopy->only.npc->str_mask, STRUNG_KEYS);
        }
      }
      if(mob->player.short_descr)
      {
        if(IS_SET(mob->only.npc->str_mask, STRUNG_DESC2))
        {
          mcopy->player.short_descr = str_dup(mob->player.short_descr);
          SET_BIT(mcopy->only.npc->str_mask, STRUNG_DESC2);
        }
      }
      if(mob->player.long_descr)
      {
        if(IS_SET(mob->only.npc->str_mask, STRUNG_DESC1))
        {
          mcopy->player.long_descr = str_dup(mob->player.long_descr);
          SET_BIT(mcopy->only.npc->str_mask, STRUNG_DESC1);
        }
      }
      if(mob->player.description)
      {
        if(IS_SET(mob->only.npc->str_mask, STRUNG_DESC3))
        {
          mcopy->player.description = str_dup(mob->player.description);
          SET_BIT(mcopy->only.npc->str_mask, STRUNG_DESC3);
        }
      }
      /* clone EQ equiped  */
      if(mob->equipment)
        for (j = 0; j < MAX_WEAR; j++)
        {
          if(mob->equipment[j])
          {
            /* clone mob->equipment[j]  */
            ocopy = clone_obj(mob->equipment[j]);
            if(mob->equipment[j]->contains)
            {
              clone_container_obj(ocopy, mob->equipment[j]);
            }
            equip_char(mcopy, ocopy, j, 0);
          }
        }
      /* clone EQ carried  */
      if(mob->carrying)
        for (obj = mob->carrying; obj; obj = obj->next_content)
        {
          ocopy = clone_obj(obj);
          if(obj->contains)
            clone_container_obj(ocopy, obj);
          /* move obj to cloned mobs carrying  */
          obj_to_char(ocopy, mcopy);
        }
      char_to_room(mcopy, ch->in_room, -1);
      act("$n has just made a clone of $N!", FALSE, ch, 0, mob, TO_ROOM);
      act("You make a clone of $N.", FALSE, ch, 0, mob, TO_CHAR);
    }
    if( IS_TRUSTED(ch) && GET_LEVEL(ch) < OVERLORD)
    {
      wizlog(GET_LEVEL(ch), "%s just cloned %s %d times [&+C%d&N]",
             GET_NAME(ch), mob->player.short_descr, count,
             world[ch->in_room].number);
      logit(LOG_WIZ, "%s cloned %s %d times [&+C%d&N]", GET_NAME(ch),
            mob->player.short_descr, count, world[ch->in_room].number);
      sql_log(ch, WIZLOG, "Cloned %s &n%d times",
            mob->player.short_descr, count);
    }
  }
  else if(is_abbrev(type, "object"))
  {
    if((obj = get_obj_in_list_vis(ch, name, ch->carrying)))
      where = 1;
    else
      if((obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))
      where = 2;
    else
    {
      send_to_char("Can't find any such object!!\n", ch);
      return;
    }
    if(obj->R_num < 0)
    {
      send_to_char("You can't clone that object!!\n", ch);
      return;
    }
    for (i = 0; i < count; i++)
    {
      ocopy = clone_obj(obj);
      ocopy->type = obj->type;

      if(obj->contains)
        clone_container_obj(ocopy, obj);
      act("$n has just made a clone of $p!", FALSE, ch, obj, 0, TO_ROOM);
      act("You make a clone of $p.", FALSE, ch, obj, 0, TO_CHAR);
      if(where == 1)
        obj_to_char(ocopy, ch);
      else
        obj_to_room(ocopy, ch->in_room);
    }
    if( IS_TRUSTED(ch) && GET_LEVEL(ch) < OVERLORD)
    {
      wizlog(GET_LEVEL(ch), "%s just cloned %s %d times [&+C%d&N]",
             GET_NAME(ch), obj->short_description, count,
             world[ch->in_room].number);
      logit(LOG_WIZ, "%s cloned %s %d times [&+C%d&N]", GET_NAME(ch),
            obj->short_description, count, world[ch->in_room].number);
      sql_log(ch, WIZLOG, "Cloned %s &n%d times",
              obj->short_description, count);
    }
  }
  else
  {
    send_to_char("Usage: clone <mob|obj> <name> <count>\n", ch);
    return;
  }
  return;
}

void do_knock(P_char ch, char *arg, int cmd)
{

  P_char   victim;

  if(!ch || !arg || IS_NPC(ch) || !IS_TRUSTED(ch))
    return;

  if(!*arg)
  {
    send_to_char("At whose door do you wish to knock?\n", ch);
    return;
  }
  if(!(victim = get_char_vis(ch, arg)) || IS_NPC(victim))
  {
    send_to_char("Sorry, no one around that fits that description.\n", ch);
    return;
  }
  if(ch == victim)
  {
    send_to_char("You don't have to ask yourself to come in, silly!\n", ch);
    return;
  }
  if(!IS_TRUSTED(victim))
  {
    act("$N's not a god, why knock?", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  act("You knock at $N's door...", FALSE, ch, 0, victim, TO_CHAR);
  act("&+R*KNOCK KNOCK*&n  $n is knocking at your door.  Can $e come in?",
      FALSE, ch, 0, victim, TO_VICT);

  return;
}

void do_ingame(P_char ch, char *args, int cmd)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  P_desc   i;
  char    *p;

  strcpy(buf1, args);           /* don't wanna change args directly */

  if(!(p = strstr(buf1, " %p")))
  {
    send_to_char("Bleah. Try ingame <string>, and include a %p somewhere.\n",
                 ch);
    return;
  }
  p++;
  p++;
  *p = 's';
  for (i = descriptor_list; i; i = i->next)
    if((i->character != ch) && !i->connected && CAN_SEE(ch, i->character) &&
        (GET_LEVEL(i->character) < GET_LEVEL(ch)))
    {

      sprintf(buf2, buf1, FirstWord(GET_NAME(i->character)));
      command_interpreter(ch, buf2);
    }
}

void do_inroom(P_char ch, char *args, int cmd)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char    *p;
  P_char   v, v_next;
  int      target;

  strcpy(buf1, args);           /* don't wanna change args directly */

  if((p = strstr(buf1, " %p")))
    target = 1;
  else if((p = strstr(buf1, " %m")))
    target = 2;
  else if((p = strstr(buf1, " %a")))
    target = 3;
  else
  {
    /* error.. no token provided  */
    send_to_char("Eh?  try: \"inroom <string>\"\n", ch);
    send_to_char
      ("  <string> should contain 1 occurance of ONE of the following:\n",
       ch);
    send_to_char("    %p - all players in the room\n", ch);
    send_to_char("    %m - all mobs in the room\n", ch);
    send_to_char("    %a - both mobs and players in the room\n", ch);
    send_to_char
      ("  <string> will then be executed once for each pc and/or npc\n", ch);
    send_to_char("  in the room, replacing the \% token with the char name\n",
                 ch);
    return;
  }

  /* okay... now we have p pointing to the space before the % token.
     move it forward two places, and replace the letter with an 's' for
     use in sprintf  */

  p++;
  p++;
  *p = 's';

  /* okay.. now buf1 is setup as an arguement for sprintf...   */

  for (v = world[ch->in_room].people; v; v = v_next)
  {
    v_next = v->next_in_room;
    if((!CAN_SEE(ch, v)) ||
        (IS_PC(v) && (target == 2)) ||
        (IS_NPC(v) && (target == 1)) || (ch == v))
      continue;

    /*
     * Serious flaw in this (and MANY other functions): if the users
     * does "inroom grin %pc %s" that extra %s is going to cause
     * problems... For now, I'm going to just "hope" that people
     * aren't that stupid (considering thats what the code does
     * everywhere else)
     */

    sprintf(buf2, buf1, FirstWord(GET_NAME(v)));

    /* okay.. now just dump buf2 to the command interpretter  */

    command_interpreter(ch, buf2);
  }
}

#define WHICH_SYNTAX "Syntax:\n   which room <zone flag>\n   which zone <zone flag>\n   which char|mob <mobact flag>\n   which obj|item <wear, extra(2), anti(2) or aff(2-6) flag>\n"

/*
 * this is 'where' based on flags, so we can find all the 'peace' rooms,
 * or 'no-ground' rooms, etc. It borrows code from both do_stat, and the
 * setbit support functions.


void concat_which_flags(const char *flagType, const char **flagNames, char *buf)
{
  int j;

  strcat(buf, flagType);
  strcat(buf, ":\n\n");

  for (j = 0; flagNames[j][0] != '\n'; j++) {
    if(j && !(j % 3))
      strcat(buf, "\n");

    sprintf(buf + strlen(buf), "%-20s", flagNames[j]);
  }

  strcat(buf, "\n\n");
}
*/

void concat_which_flagsde(const char *flagType, const flagDef flagNames[],
                          char *buf)
{
  int      j;

  strcat(buf, flagType);
  strcat(buf, ":\n\n");

  for (j = 0; flagNames[j].flagShort != NULL; j++)
  {
    if(j && !(j % 3))
      strcat(buf, "\n");

    sprintf(buf + strlen(buf), "%-20s", flagNames[j].flagShort);
  }

  strcat(buf, "\n\n");
}

bool check_flags(int *value, const char **flagNames, const char *flagName)
{
  int      i;

  for (i = 0; str_cmp(flagNames[i], flagName) && (flagNames[i][0] != '\n');
       i++) ;

  if(flagNames[i][0] == '\n')
    return FALSE;

  *value = i;

  return TRUE;
}

bool check_flagsde(int *value, const flagDef flagNames[],
                   const char *flagName)
{
  int      i;

  for (i = 0;
       (flagNames[i].flagShort != NULL) &&
       str_cmp(flagNames[i].flagShort, flagName); i++) ;

  if(flagNames[i].flagShort == NULL)
    return FALSE;

  *value = i;

  return TRUE;
}

bool check_apply(int *value, const char *flagName)
{
  int i;

  for (i = 0; apply_types[i] != NULL &&
      str_cmp(apply_types[i], flagName); i++) ;

  if(apply_types[i] == NULL)
    return FALSE;

  *value = i;

  return TRUE;
}

void do_which(P_char ch, char *args, int cmd)
{
  typedef enum _whichObjFlagsEnum
  { wear = 0, extra, extra2, anti, anti2, aff, aff2, aff3, aff4,
    aff5, apply
  } whichObjFlagsEnum;

  P_char   t_ch = NULL;
  P_obj    t_obj = NULL;
  char     arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char     arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH];
  int      sc_min, sc_max = 0;
  char     o_buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
  int      i, j, t, room_nr, zone_nr, o_len, which, found;
  bool     stat_check = FALSE;
  whichObjFlagsEnum whichObjFlags;
  uint     w_bit = 0;

  args = setbit_parseArgument(args, arg1);      /* this is one_argument(),
                                                   but allows 'two word'
                                                   args  */
  args = setbit_parseArgument(args, arg2);
  args = one_argument(args, arg3);
  one_argument(args, arg4);
  sc_min = atoi(arg3);
  sc_max = atoi(arg4);

  if(sc_min)
    stat_check = TRUE;

  if(!*arg1 || !*arg2)
  {
    send_to_char(WHICH_SYNTAX, ch);
    return;
  }
  o_buf[0] = '\0';              /* output string, so we can page it */
  o_len = 50;                   /* saves us a bunch of strlen() calls */
  buf1[0] = '\0';

  if((*arg1 == 'r') || (*arg1 == 'R'))
  {
    /* room and zone flags are the easiest, just run down the list  */
    for (i = 0;
         room_bits[i].flagShort && str_cmp(room_bits[i].flagShort, arg2);
         i++) ;
    if(!room_bits[i].flagShort)
    {
      send_to_char("Unknown flag, valid options are:\n", ch);
      for (j = 0; room_bits[j].flagShort; j++)
      {

        if(j && !(j % 3))
          strcat(buf1, "\n");

        sprintf(buf1 + strlen(buf1), "%-20s", room_bits[j].flagShort);
      }
      strcat(buf1, "\n");
      send_to_char(buf1, ch);
      return;
    }
    w_bit = 1 << i;
    for (room_nr = 0; room_nr < top_of_world; room_nr++)
      if(world[room_nr].room_flags & w_bit)
      {
        sprintf(buf1, "&+Y[&n%5d&+Y](&n%5d&+Y)&n %s\n", world[room_nr].number,
                room_nr, world[room_nr].name);
        o_len += strlen(buf1);
        if(o_len > MAX_STRING_LENGTH)
        {
          strcat(o_buf, "And so on, and so forth...\n");
          break;
        }
        else
        {
          strcat(o_buf, buf1);
        }
      }
  }
  else if((*arg1 == 'z') || (*arg1 == 'Z'))
  {

    for (i = 0; str_cmp(zone_bits[i], arg2) && (zone_bits[i][0] != '\n');
         i++) ;
    if(zone_bits[i][0] == '\n')
    {
      send_to_char("Unknown flag, valid options are:\n", ch);
      for (j = 0; zone_bits[j][0] != '\n'; j++)
      {

        if(j && !(j % 3))
          strcat(buf1, "\n");

        sprintf(buf1 + strlen(buf1), "%-20s", zone_bits[j]);
      }
      strcat(buf1, "\n");
      send_to_char(buf1, ch);
      return;
    }
    w_bit = 1 << i;
    for (zone_nr = 0; zone_nr <= top_of_zone_table; zone_nr++)
      if(zone_table[zone_nr].flags & w_bit)
      {
        sprintf(buf1, "&+Y[&n%3d&+Y]&n %s\n", zone_nr,
                zone_table[zone_nr].name);
        o_len += strlen(buf1);
        if(o_len > MAX_STRING_LENGTH)
        {
          strcat(o_buf, "And so on, and so forth...\n");
          break;
        }
        else
        {
          strcat(o_buf, buf1);
        }
      }
  }
  else if((*arg1 == 'o') || (*arg1 == 'O') || (*arg1 == 'i') ||
           (*arg1 == 'I'))
  {

    /*
     * obj flags are slightly different, because we have to check for
     * many flags
     */

    if(check_flagsde(&i, wear_bits, arg2))
      whichObjFlags = wear;
    else if(check_flagsde(&i, extra_bits, arg2))
      whichObjFlags = extra;
    else if(check_flagsde(&i, extra2_bits, arg2))
      whichObjFlags = extra2;
    /*
       else if(check_flagsde(&i, anti_bits, arg2))
       whichObjFlags = anti;
       else if(check_flagsde(&i, anti2_bits, arg2))
       whichObjFlags = anti2; */
    else if(check_flagsde(&i, affected1_bits, arg2))
      whichObjFlags = aff;
    else if(check_flagsde(&i, affected2_bits, arg2))
      whichObjFlags = aff2;
    else if(check_flagsde(&i, affected3_bits, arg2))
      whichObjFlags = aff3;
    else if(check_flagsde(&i, affected4_bits, arg2))
      whichObjFlags = aff4;
    else if(check_flagsde(&i, affected5_bits, arg2))
      whichObjFlags = aff5;
    else if(check_apply(&i, arg2))
      whichObjFlags = apply;
    else
    {
      // assume we won't go over max_string_length?  let's see what happens :)

      strcpy(buf1, "Unknown flag, valid options are:\n\n");

      concat_which_flagsde("Wear", wear_bits, buf1);
      concat_which_flagsde("Extra", extra_bits, buf1);
      concat_which_flagsde("Extra2", extra2_bits, buf1);
//      concat_which_flagsde("Anti", anti_bits, buf1);
//      concat_which_flagsde("Anti2", anti2_bits, buf1);
      concat_which_flagsde("Aff1", affected1_bits, buf1);
      concat_which_flagsde("Aff2", affected2_bits, buf1);
      concat_which_flagsde("Aff3", affected3_bits, buf1);
      concat_which_flagsde("Aff4", affected4_bits, buf1);
      concat_which_flagsde("Aff5", affected5_bits, buf1);
      //add applies to objects here..

      page_string(ch->desc, buf1, 1);
      return;
    }
    w_bit = 1 << i;
    for (t_obj = object_list; t_obj; t_obj = t_obj->next)
    {
      found = 0;

      if(IS_NOSHOW(t_obj))
        continue;

      switch (whichObjFlags)
      {
      case wear:
        found = (t_obj->wear_flags & w_bit);
        break;

      case extra:
        found = (t_obj->extra_flags & w_bit);
        break;

      case extra2:
        found = (t_obj->extra2_flags & w_bit);
        break;

      case anti:
        found = (t_obj->anti_flags & w_bit);
        break;

      case anti2:
        found = (t_obj->anti2_flags & w_bit);
        break;

      case aff:
        found = (t_obj->bitvector & w_bit);
        break;

      case aff2:
        found = (t_obj->bitvector2 & w_bit);
        break;

      case aff3:
        found = (t_obj->bitvector3 & w_bit);
        break;

      case aff4:
        found = (t_obj->bitvector4 & w_bit);
        break;

      case aff5:
        found = (t_obj->bitvector5 & w_bit);
        break;

      case apply:
	for (j = 0; j < 3; j++)
	{
	  if(found = (t_obj->affected[j].location == i))
	    break;
	}
	break;
	
      default:
        found = FALSE;          // shrug
      }

      if(found && !stat_check)
      {
        sprintf(buf1, "[%5d] %-30s - %s\n",
                obj_index[t_obj->R_num].virtual_number,
                t_obj->short_description, where_obj(t_obj, FALSE));

        o_len += strlen(buf1);
        if(o_len > MAX_STRING_LENGTH)
        {
          strcat(o_buf, "And so on, and so forth...\n");
          break;
        }
        else
        {
          strcat(o_buf, buf1);
        }
      }
      else if(found && stat_check)
      {
        if(sc_min <= t_obj->affected[j].modifier && sc_max >= t_obj->affected[j].modifier)
        {
          char temp[MAX_STRING_LENGTH];
          char temp2[MAX_STRING_LENGTH];
          for(t = 0; t < MAX_OBJ_AFFECT; t++)
          {            
            if(t_obj->affected[t].location != APPLY_NONE)
            {
               sprinttype(t_obj->affected[t].location, apply_types, temp2);
               sprintf(temp + strlen(temp), "   &+YAffects: &+c%s&+Y By &+c%d\n",
                  temp2, t_obj->affected[t].modifier);
            }
          }
          sprintf(buf1, "[%5d] %-30s - %s\n%s\n",
                  obj_index[t_obj->R_num].virtual_number,
                  t_obj->short_description, where_obj(t_obj, FALSE), temp);

          sprintf(temp, "");
          o_len += strlen(buf1);
          if(o_len > MAX_STRING_LENGTH)
          {
            strcat(o_buf, "And so on, and so forth...\n");
            break;
          }
          else
          {
            strcat(o_buf, buf1);
          }
        }
      }
    }
  }
  else if((*arg1 == 'c') || (*arg1 == 'C') || (*arg1 == 'm') ||
           (*arg1 == 'M'))
  {

    /*
     * char flags are slightly different, because we have to check for
     * pcact, or npcact
     */

    which = 0;
    for (i = 0;
         action_bits[i].flagShort && str_cmp(action_bits[i].flagShort, arg2);
         i++) ;
    if(!action_bits[i].flagShort)
    {
      which = 1;
      for (i = 0;
           str_cmp(player_bits[i], arg2) && (player_bits[i][0] != '\n');
           i++) ;
      if(player_bits[i][0] == '\n')
      {
        send_to_char("Unknown flag, valid options are:\n", ch);
        for (j = 0; action_bits[j].flagShort; j++)
        {

          if(j && !(j % 3))
            strcat(buf1, "\n");

          sprintf(buf1 + strlen(buf1), "%-20s", action_bits[j].flagShort);
        }
        strcat(buf1, "\n");

        for (j = 0; player_bits[j][0] != '\n'; j++)
        {

          if(j && !(j % 3))
            strcat(buf1, "\n");

          sprintf(buf1 + strlen(buf1), "%-20s", player_bits[j]);
        }
        strcat(buf1, "\n");

        send_to_char(buf1, ch);
        return;
      }
    }
    w_bit = 1 << i;
    for (t_ch = character_list; t_ch; t_ch = t_ch->next)
    {
      if(!CAN_SEE(ch, t_ch))
        continue;
      found = 0;
      if(which)
      {
        if(IS_PC(t_ch) && (t_ch->specials.act & w_bit))
          found = TRUE;
      }
      else
      {
        if(IS_NPC(t_ch) && (t_ch->specials.act & w_bit))
          found = TRUE;
      }

      if(found)
      {
        if(IS_NPC(t_ch))
          sprintf(buf1, "%-30s- &+Y[&n%5d&+Y]&n %s\n",
                  t_ch->player.short_descr, world[t_ch->in_room].number,
                  world[t_ch->in_room].name);
        else
          sprintf(buf1, "%-30s- &+Y[&n%5d&+Y]&n %s\n",
                  t_ch->player.name, world[t_ch->in_room].number,
                  world[t_ch->in_room].name);
        o_len += strlen(buf1);
        if(o_len > MAX_STRING_LENGTH)
        {
          strcat(o_buf, "And so on, and so forth...\n");
          break;
        }
        else
        {
          strcat(o_buf, buf1);
        }
      }
    }
  }
  if(!*o_buf)
    send_to_char("No matches.\n", ch);
  else
    page_string(ch->desc, o_buf, 1);
}

#undef WHICH_SYNTAX


/*
 * do_grant : grant some command
 */

void do_grant(P_char ch, char *args, int cmd)
{
  char     victname[MAX_INPUT_LENGTH], cmdname[MAX_INPUT_LENGTH],
    buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int     *new_arr, i, s;
  P_char   vict;


  args = one_argument(args, victname);
  one_argument(args, cmdname);

  if(!ch || (GET_LEVEL(ch) < FORGER))
  {
    send_to_char("Granted, you want to grant, but not here!\r\n", ch);
    return;
  }

  if(!victname[0])
  {
    send_to_char("Syntax: grant <char> <command name>\n", ch);
    return;
  }

  /* get target, check all sorts of shit */

  vict = get_char(victname);
  if(!vict)
  {
    send_to_char("Target not found.\n", ch);
    return;
  }

  if((ch == vict) && cmdname[0])
  {
    send_to_char("Grant to yourself?  That's odd.\n", ch);
    return;
  }

  if(!IS_PC(vict))
  {
    send_to_char("Need a PC, my friend.\n", ch);
    return;
  }

  if(!IS_TRUSTED(vict))
  {
    send_to_char("That person does not appear to be godly enough.\n", ch);
    return;
  }

  /* if no command arg, list currently granted commands */

  if(!cmdname[0])
  {
    sprintf(buf, "&+WCommands currently granted to %s:\n\n", GET_NAME(vict));

    if(!vict->only.pc->gcmd_arr)
    {
      strcat(buf, "None!\n");
    }
    else
    {
      s = vict->only.pc->numb_gcmd;

      for (i = 0; i < s; i++)
      {
        sprintf(buf2, "[&+Y%d&n] &+c%-20s&n",
                cmd_info[vict->only.pc->gcmd_arr[i]].minimum_level,
                command[vict->only.pc->gcmd_arr[i] - 1]);

        strcat(buf, buf2);

        if(!((i + 1) % 3))
          strcat(buf, "\n");
      }

      strcat(buf, "\n");
    }

    page_string(ch->desc, buf, 1);

    return;
  }

  /* let's get the command */

  cmd = old_search_block(cmdname, 0, strlen(cmdname), command, 2);

  if(cmd <= 0)
  {
    send_to_char("Sorry, that command does not seem to exist.\n", ch);
    return;
  }

  if(!cmd_info[cmd].grantable)
  {
    send_to_char("Sorry, but that command is not grantable.\n", ch);
    return;
  }

  if(cmd_info[cmd].minimum_level > GET_LEVEL(ch))
  {
    send_to_char("You are not worthy of that command yourself!\n", ch);
    return;
  }

  /* okay let's go zany.  ZANY! */

  vict->only.pc->numb_gcmd++;
  s = vict->only.pc->numb_gcmd;

  CREATE(new_arr, int, s, MEM_TAG_ARRAY);

  for (i = 0; i < (s - 1); i++)
    new_arr[i] = vict->only.pc->gcmd_arr[i];

  new_arr[i] = cmd;

  if(vict->only.pc->gcmd_arr)
    FREE(vict->only.pc->gcmd_arr);

  vict->only.pc->gcmd_arr = new_arr;

  sprintf(buf, "You have granted the command '%s' (level %d) to %s.\n",
          command[cmd - 1], cmd_info[cmd].minimum_level, GET_NAME(vict));

  send_to_char(buf, ch);

  sprintf(buf, "%s has granted you the use of the '%s' command.\n",
          GET_NAME(ch), command[cmd - 1]);

  send_to_char(buf, vict);

  logit(LOG_WIZ, "%s granted %s the '%s' (%d) command.",
        GET_NAME(ch), GET_NAME(vict), command[cmd - 1],
        cmd_info[cmd].minimum_level);
  wizlog(GET_LEVEL(ch), "%s granted %s the '%s' (%d) command.", GET_NAME(ch),
         GET_NAME(vict), command[cmd - 1], cmd_info[cmd].minimum_level);
  sql_log(ch, WIZLOG, "Granted %s the '%s' (%d) command.",
         GET_NAME(vict), command[cmd - 1], cmd_info[cmd].minimum_level);
}

void do_revoke(P_char ch, char *args, int cmd)
{
  char     victname[MAX_INPUT_LENGTH], cmdname[MAX_INPUT_LENGTH],
    buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int     *new_arr, i, j, s;
  P_char   vict;


  args = one_argument(args, victname);
  one_argument(args, cmdname);

  if(!victname[0])
  {
    send_to_char("Syntax: revoke <char> <command name>\n", ch);
    return;
  }

  /* get target, check all sorts of shit */

  vict = get_char(victname);
  if(!vict)
  {
    send_to_char("Target not found.\n", ch);
    return;
  }

  if((ch == vict) && cmdname[0])
    send_to_char("Revoke from yourself?  Well, if you say so..\n", ch);

  if(!IS_PC(vict))
  {
    send_to_char("Need a PC, my friend.\n", ch);
    return;
  }

  if(!IS_TRUSTED(vict))
  {
    send_to_char("That person does not appear to be godly enough.\n", ch);
    return;
  }

  if(!cmdname[0])
  {
    send_to_char
      ("Use 'grant' to see what commands a char currently has granted.\n",
       ch);
    return;
  }

  if(!vict->only.pc->gcmd_arr)
  {
    send_to_char("They have no commands granted to them.\n", ch);
    return;
  }

  /* let's get the command */

  cmd = old_search_block(cmdname, 0, strlen(cmdname), command, 2);

  if(cmd <= 0)
  {
    send_to_char("Sorry, that command does not seem to exist.\n", ch);
    return;
  }

  /* okay let's go zany.  ZANY! */

  s = vict->only.pc->numb_gcmd;

  for (i = 0; i < s; i++)
  {
    if(vict->only.pc->gcmd_arr[i] == cmd)
    {
      if(s > 1)
      {
        CREATE(new_arr, int, s - 1, MEM_TAG_ARRAY);

        /* copy up to here .. */

        for (j = 0; j < i; j++)
        {
          new_arr[j] = vict->only.pc->gcmd_arr[j];
        }

        /* copy past revoked command */

        for (j = i + 1; j < s; j++)
        {
          new_arr[j - 1] = vict->only.pc->gcmd_arr[j];
        }

        FREE(vict->only.pc->gcmd_arr);

        vict->only.pc->gcmd_arr = new_arr;
      }
      else                      /* only had one command, g'bye array */
      {
        FREE(vict->only.pc->gcmd_arr);

        vict->only.pc->gcmd_arr = NULL;
      }

      vict->only.pc->numb_gcmd--;

      sprintf(buf, "You have revoked the command '%s' (level %d) from %s.\n",
              command[cmd - 1], cmd_info[cmd].minimum_level, GET_NAME(vict));

      send_to_char(buf, ch);

      sprintf(buf, "%s has revoked your ability to use the '%s' command.\n",
              GET_NAME(ch), command[cmd - 1]);

      send_to_char(buf, vict);

      logit(LOG_WIZ, "%s revoked '%s' (%d) command from %s.",
            GET_NAME(ch), command[cmd - 1], cmd_info[cmd].minimum_level,
            GET_NAME(vict));
      wizlog(GET_LEVEL(ch), "%s revoked '%s' (%d) command from %s.",
             GET_NAME(ch), command[cmd - 1], cmd_info[cmd].minimum_level,
             GET_NAME(vict));
      sql_log(ch, WIZLOG, "Revoked '%s' (%d) command from %s.",
              command[cmd - 1], cmd_info[cmd].minimum_level,
              GET_NAME(vict));
      
      return;
    }
  }

  send_to_char("That character does not have that command granted.\n", ch);
}

#if 0
#   define REVOKETITLE_SYNTAX "Syntax:\n revoketitle <char name>\n"

/*
 * Guild-God Command.
 *
 * This command allows guild god(s) to revoke from any player the ability
 * to 'title.'  It is made a separate command to restrict gods from
 * revoking just any command--since revoke is reserved for gods level 59
 * and up..
 *
 * Some of this code was borrowed from do_revoke() [naturally..]
 */

void do_revoketitle(P_char ch, char *args, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  int      loopvar = 0, bitpos = -1;
  P_char   victim;
  char     buf[MAX_STRING_LENGTH];

  buf[0] = 0;

  if(!args || !*args)
  {
    send_to_char(REVOKETITLE_SYNTAX, ch);
    return;
  }
  (void) one_argument(args, name);
  victim = get_char_vis(ch, name);

  if(!victim)
  {
    send_to_char("Who?", ch);
    return;
  }
  if(GET_LEVEL(ch) < GET_LEVEL(victim))
  {
    send_to_char("Not so fast wise-guy.\n", ch);
    return;
  }
  if(ch == victim)
    send_to_char("Revoking your own powers?  Well, ok, I guess!\n", ch);

  /*
   * just in case the title command moves, or is taken out (?!?)
   */

  for (loopvar = 0; *grantable_bits[loopvar] != '\n'; loopvar++)
    if(is_abbrev("title", grantable_bits[loopvar]))
    {
      bitpos = loopvar;
      break;
    }
  if(bitpos < 0)
  {
    send_to_char("This 'title' command does not exist anymore!\n", ch);
    return;
  }
  if(ch != victim)
    strcpy(buf, "They don't have the 'title' command.\n.");
  else
    strcpy(buf, "You don't have the 'title' command.\n");
  send_to_char(buf, ch);
  return;

  if(ch != victim)
  {
    act("You revoke $S 'title' command.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n revokes your 'title' command!", FALSE, ch, 0, victim, TO_VICT);
  }
  else
    act("You revoke your 'title' command.", FALSE, ch, 0, 0, TO_CHAR);

  do_save_silent(victim, 1);

  logit(LOG_WIZ, "<REVOKE>: %s revokes %s's 'title' command.",
        GET_NAME(ch), GET_NAME(victim));
  wizlog(GET_LEVEL(ch), "%s revokes %s's 'title' command.",
         GET_NAME(ch), GET_NAME(victim));
}

#   undef SYNTAX_REVOKETITLE
#endif

/*
 * Guild-God command.
 *
 * This command acts as a filter, calling do_setbit() after it rebuilds
 * the command to set the hometown.
 */

#define SYNTAX_SETHOME "Syntax:\n   sethome <char> <home flags> <room>\n"

void do_sethome(P_char ch, char *args, int cmd)
{
  char     name[MAX_INPUT_LENGTH], hflg[MAX_INPUT_LENGTH];
  char     val[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  static char homeflags[3][16] = {
    "hometown", "birthplace", "\n"
  };
  static char setbit_flags[2][16] = {
    "home", "orighome"
  };
  char    *p;
  int      i, hflg_index = -1;

  args = one_argument(args, name);
  args = one_argument(args, hflg);
  (void) one_argument(args, val);

  if(!name[0])
  {
    send_to_char(SYNTAX_SETHOME, ch);
    return;
  }
  if(hflg[0])
  {
    for (p = hflg; *p; p++)
      *p = tolower(*p);

    for (i = 0; homeflags[i][0] != '\n'; i++)
    {
      if(is_abbrev(hflg, homeflags[i]))
      {
        hflg_index = i;
        break;
      }
    }
  }
  if(!hflg[0] || hflg_index == -1)
  {
    send_to_char(SYNTAX_SETHOME, ch);
    send_to_char("\nValid home flags are:\n", ch);
    for (i = 0; homeflags[i][0] != '\n'; i++)
    {
      sprintf(buf, "\t%s\n", homeflags[i]);
      send_to_char(buf, ch);
    }
    return;
  }
  /*
   * here we have at least 2 arguments, the 2nd being a valid 'home
   * flag'
   */

  sprintf(buf, "char %s %s %s", name, setbit_flags[hflg_index], val);
  do_setbit(ch, buf, CMD_SETHOME);
  // a hack :)
  if(1 == hflg_index)
  {
    // set the origbp too
    sprintf(buf, "char %s origbp %s", name, val);
    do_setbit(ch, buf, CMD_SETHOME);
  }
}

void revert_sethome(P_char member)
{
  char buf[MAX_STRING_LENGTH];

  // Revert their home to their original birthplace.
  sprintf(buf, "char %s home %d", J_NAME(member), GET_ORIG_BIRTHPLACE(member));
  do_setbit(member, buf, CMD_SETHOME);
  sprintf(buf, "char %s orighome %d", J_NAME(member), GET_ORIG_BIRTHPLACE(member));
  do_setbit(member, buf, CMD_SETHOME);
}

#if 0                           /* don't do anything with this yet (neb)  */
void do_proc(P_char ch, char *args, int cmd)
{
  /*
   * interface for listing, loading, and unloading libraries of special
   * procedures
   *
   * proc [list | [load | unload <libname>]]
   *
   * with no parameter, generate a syntax message
   *
   * list - lists all libs the mud knows about, and if they are loaded
   * or not
   *
   * load - (only usable by 59+) loads <libname> and assigns function
   * pointers.  (if <libname> is already loaded, returns error)
   *
   * unload - (only usable by 59+) unloads <libname> and assigns all
   * used function pointers to stub functions. (if <libname> isn't
   * loaded, returns an error).
   *
   */
#   ifndef SHLIB
  send_to_char("The mud wasn't compiled with dynamic proc loading enabled\n",
               ch);
#   else

  char     func[MAX_STRING_LENGTH];
  char     libname[MAX_STRING_LENGTH];
  int      i;

  if(!ch || !args || IS_NPC(ch) || !IS_TRUSTED(ch))
    return;

  if(!*args)
  {

    send_to_char("Usage: proc \n", ch);
    send_to_char
      ("            list          - list proc libraries the mud knows about\n",
       ch);
    if(GET_LEVEL(ch) < 59)
      return;
    send_to_char
      ("            load <name>   - load the proc library called <name>\n",
       ch);
    send_to_char
      ("            unload <name> - unload the proc library called <name>\n",
       ch);
    return;
  }
  args = one_argument(args, func);
  if(!str_cmp(func, "list"))
  {
    /*
     * list libs, their status, and exit
     */

    send_to_char("\nLib name                Status"
                 "\n--------                ------\n", ch);

    for (i = 0; dynam_proc_list[i].name; i++)
    {
      sprintf(libname, "%-23s %-8s\n", dynam_proc_list[i].name,
              dynam_proc_list[i].handle ? "Loaded" : "Unloaded");
      send_to_char(libname, ch);
    }
    return;
  }
  one_argument(args, libname);
  if(GET_LEVEL(ch) >= 59)
    if(!str_cmp(func, "load"))
    {
      for (i = 0; dynam_proc_list[i].name; i++)
        if(!str_cmp(dynam_proc_list[i].name, libname))
        {
          if(dynam_proc_list[i].handle)
          {
            send_to_char("That lib is already loaded!\n", ch);
          }
          else
          {
            if(load_proc_lib(libname))
            {
              send_to_char("Library loaded successfully!\n", ch);
              /*
               * LOG ME!!
               */
            }
            else
            {
              send_to_char("Unable to load lib!\n", ch);
              /*
               * LOG ME!!
               */
            }
          }
          return;
        }
      send_to_char("No such library exists.\n", ch);
      return;
    }
    else if(!str_cmp(func, "unload"))
    {
      for (i = 0; dynam_proc_list[i].name; i++)
        if(!str_cmp(dynam_proc_list[i].name, libname))
        {
          if(!dynam_proc_list[i].handle)
          {
            send_to_char("That lib isn't loaded!\n", ch);
          }
          else
          {
            if(unload_proc_lib(libname))
            {
              send_to_char("Library unloaded successfully!\n", ch);
              /*
               * LOG ME!!
               */
            }
            else
            {
              send_to_char("Unable to unload lib!\n", ch);
              /*
               * LOG ME!!
               */
            }
          }
          return;
        }
      send_to_char("No such library exists.\n", ch);
      return;
    }
  send_to_char("Bad command.  Try \"proc\" with no paramters for usage\n",
               ch);

#   endif
  /* SHLIB  */

  return;
}

#endif

void do_terminate(P_char ch, char *argument, int cmd)
{
  P_char   victim = NULL;
  char     buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf)
  {
    send_to_char
      ("But oh Mighty One, whom do you wish to utterly annihilate?\n", ch);
    return;
  }
  if(!(victim = get_char_room_vis(ch, buf)))
  {
    send_to_char("You see no person by that name here.\n", ch);
    return;
  }
  if(IS_NPC(victim))
  {
    send_to_char("Who cares about NPC's...\n", ch);
    return;
  }
  if((GET_LEVEL(victim) >= GET_LEVEL(ch)) || (GET_LEVEL(victim) >= 62))
  {
    send_to_char("Go away jackass!\n", ch);
    return;
  }
  send_to_char
    ("&+rYou raise your hands and feel the power of life and death\n", ch);
  send_to_char("&+rflowing through your soul!  You unleash the power in a \n",
               ch);
  act("&+Lblack beam&N &+rof death upon $N!!!", FALSE, ch, 0, victim,
      TO_CHAR);
  act("&+r$N is completely obliterated into a million bits!", FALSE, ch, 0,
      victim, TO_CHAR);
  act("&+rYou smile as $N will never again challenge your greatness!", FALSE,
      ch, 0, victim, TO_CHAR);

  act("&+r$n is surrounded by a red aura, and $s &+Weyes&n &+rglow.", FALSE,
      ch, 0, victim, TO_NOTVICT);
  act("&+r$n raises $s hands and points them at $N and a&n &+Lblack ", FALSE,
      ch, 0, victim, TO_NOTVICT);
  act("&+Lbeam&n &+r shoots out to strike at $N!", FALSE, ch, 0, victim,
      TO_NOTVICT);
  act("&+r$N cries out and $E shatters into a million bits!", FALSE, ch, 0,
      victim, TO_NOTVICT);

  act("&+r$n is surrounded by a red aura, and $s &+Weyes&n &+rglow.", FALSE,
      ch, 0, victim, TO_VICT);
  act("&+r$n raises $s hands and points them at YOU! and a&n &+Lblack ",
      FALSE, ch, 0, victim, TO_VICT);
  act("&+Lbeam&n &+r shoots out to strike at YOU!", FALSE, ch, 0, victim,
      TO_VICT);
  act("&+rYou feel your soul being invaded by $n as $e begins to", FALSE, ch,
      0, victim, TO_VICT);
  act("&+runravel the mysteries of your existance! You feel every part",
      FALSE, ch, 0, victim, TO_VICT);
  act("&+rof your body being torn to shreds, and you suddenly cry out in",
      FALSE, ch, 0, victim, TO_VICT);
  act("&+rutter terror as $n stands over the remains of you in righteous",
      FALSE, ch, 0, victim, TO_VICT);
  act("&+rjustice!  You fall away into complete&n &+Lblackness.......", FALSE,
      ch, 0, victim, TO_VICT);

  statuslog(ch->player.level,
            "%s's existence on Duris was just terminated by %s!",
            GET_NAME(victim), GET_NAME(ch));

  logit(LOG_WIZ, "%s's existence on Duris was just terminated by %s!",
        GET_NAME(victim), GET_NAME(ch));

  sql_log(ch, WIZLOG, "Terminated %s's existence on Duris.", GET_NAME(victim));

  if(victim->desc)
  {
    close_socket(victim->desc);
  }
/*
    victim->desc->connected = CON_DELETE;
    extract_char(victim);
    return;
  } else {
*/
  extract_char(victim);
  deleteCharacter(victim);
  free_char(victim);
  victim = NULL;
  return;
/*  }*/
  logit(LOG_DEBUG, "Somehow reached the end of do_terminate()");
  return;
}

void do_sacrifice(P_char ch, char *argument, int cmd)
{

  P_char   victim = NULL;
  char     buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf)
  {
    send_to_char("Who do you feel like sacrificing today?\n", ch);
    return;
  }
  if(!(victim = get_char_room_vis(ch, buf)))
  {
    send_to_char("You see no person by that name here.\n", ch);
    return;
  }
  if(IS_NPC(victim))
  {
    send_to_char("Who cares about NPC's...\n", ch);
    return;
  }
  if(victim == ch)
  {
    send_to_char("How do you propose to sacrifice yourself?", ch);
    return;
  }
  if(GET_LEVEL(victim) >= MIN(62, GET_LEVEL(ch)))
  {
    send_to_char("Sorry, you can't sacrifice your peers or superiors!\n", ch);
    return;
  }
  act("$n whips out a huge dagger out of nowhere and stabs you in the heart!",
      FALSE, ch, 0, victim, TO_VICT);
  act("You conjure a dagger out of nowhere and stab $N in the heart!", FALSE,
      ch, 0, victim, TO_CHAR);
  act("$n whips out a huge dagger out of nowhere and stabs $N in the heart!",
      FALSE, ch, 0, victim, TO_NOTVICT);
  statuslog(ch->player.level, "%s was just sacrificed by %s.",
            GET_NAME(victim), GET_NAME(ch));
  logit(LOG_WIZ, "%s was sacrificed by %s.", GET_NAME(victim), GET_NAME(ch));
  die(victim, ch);
  return;
}


ACMD(do_depiss)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    buf3[MAX_STRING_LENGTH];
  P_char   vic = NULL, mob = NULL;

  half_chop(argument, buf1, buf2);

  if(!*buf1 || !*buf2)
  {
    send_to_char("Usage: depiss <player> <mobile>\n", ch);
    return;
  }
  if(((vic = get_char_vis(ch, buf1))) && (!IS_NPC(vic)))
  {
    if(((mob = get_char_vis(ch, buf2))) && (IS_NPC(mob)))
    {
      if(IS_SET(mob->specials.act, ACT_MEMORY))
        forget(mob, vic);
      else
      {
        send_to_char("Mobile does not have the memory flag set!\n", ch);
        return;
      }
    }
    else
    {
      send_to_char("Sorry, Player Not Found!\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("Sorry, Mobile Not Found!\n", ch);
    return;
  }
  sprintf(buf3, "%s has been removed from %s's pissed list.\n",
          J_NAME(vic), J_NAME(mob));
  send_to_char(buf3, ch);
}

ACMD(do_repiss)
{
  char     buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
    buf3[MAX_STRING_LENGTH];
  P_char   vic = NULL, mob = NULL;

  half_chop(argument, buf1, buf2);

  if(!*buf1 || !*buf2)
  {
    send_to_char("Usage: repiss <player> <mobile>\n", ch);
    return;
  }
  if(((vic = get_char_vis(ch, buf1))) && (!IS_NPC(vic)))
  {
    if(((mob = get_char_vis(ch, buf2))) && (IS_NPC(mob)))
    {
      if(IS_SET(mob->specials.act, ACT_MEMORY))
        remember(mob, vic);
      else
      {
        send_to_char("Mobile does not have the memory flag set!\n", ch);
        return;
      }
    }
    else
    {
      send_to_char("Sorry, Player Not Found!\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("Sorry, Mobile Not Found!\n", ch);
    return;
  }
  sprintf(buf3, "%s has been added to %s's pissed list.\n",
          J_NAME(vic), J_NAME(mob));
  send_to_char(buf3, ch);
}

#if 0
ACMD(do_tedit)
{
  int      l, i;
  char     field[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];

  struct editor_struct
  {
    const char *cmd;
    char     level;
    char    *buffer;
    int      size;
    const char *filename;
  } fields[] =
  {
    {
    "credits", AVATAR, credits, 65536, CREDITS_FILE},
    {
    "news", AVATAR, news, 65536, NEWS_FILE},
    {
    "motd", AVATAR, motd, 16384, MOTD_FILE},
    {
    "help", AVATAR, help, 16384, HELP_PAGE_FILE},
    {
    "bugs", AVATAR, bugfile, 16384, BUG_FILE},
    {
    "\n", 0, NULL, 0, NULL}
  };

  if(!IS_TRUSTED(ch))
  {
    send_to_char("You do not have text editor permissions.\n", ch);
    return;
  }
  half_chop(argument, field, buf);

  if(!*field)
  {
    strcpy(buf, "Files available to be edited:\n");
    i = 1;
    for (l = 0; *fields[l].cmd != '\n'; l++)
    {
      if(GET_LEVEL(ch) >= fields[l].level)
      {
        sprintf(buf, "%s%-11.11s", buf, fields[l].cmd);
        if(!(i % 7))
          strcat(buf, "\n");
        i++;
      }
    }
    if(--i % 7)
      strcat(buf, "\n");
    if(i == 0)
      strcat(buf, "None.\n");
    send_to_char(buf, ch);
    return;
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if(!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if(*fields[l].cmd == '\n')
  {
    send_to_char("Invalid text editor option.\n", ch);
    return;
  }
  if(GET_LEVEL(ch) < fields[l].level)
  {
    send_to_char("You are not godly enough for that!\n", ch);
    return;
  }
  switch (l)
  {
  case 0:
    ch->desc->str = &credits;
    break;
  case 1:
    ch->desc->str = &news;
    break;
  case 2:
    ch->desc->str = &motd;
    break;
  case 3:
    ch->desc->str = &help;
    break;
  case 4:
    ch->desc->str = &bugfile;
    break;
  default:
    send_to_char("Invalid text editor option.\n", ch);
    return;
  }

  /* set up editor stats */
  send_to_char("\x1B[H\x1B[J", ch);
  send_to_char("Edit file below: (/s saves /h for help)\n", ch);
  ch->desc->backstr = NULL;
  if(fields[l].buffer)
  {
    send_to_char(fields[l].buffer, ch);
    ch->desc->backstr = str_dup(fields[l].buffer);
  }
  ch->desc->max_str = fields[l].size;
  ch->desc->storage = str_dup(fields[l].filename);
  act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT(ch->specials.act, PLR_WRITE);
  STATE(ch->desc) = CON_TEXTED;
}
#endif


/* Function to list the witness record of player and mob
   or the town crime record (replace function TASFALEN) */

#define LWITNESS_SYNTAX "Syntax:\n   lwitness char|mob <name>\n   lwitness town <town # or name> \n   lwitness town <town # or name> <name> (remove records for <name>)\n"

ACMD(do_list_witness)
{
  wtns_rec *rec = NULL;
  crm_rec *crec = NULL;
  char     buf[MAX_STRING_LENGTH], arg1[MAX_STRING_LENGTH];
  char     arg2[MAX_STRING_LENGTH], arg3[MAX_STRING_LENGTH];
  char     arg4[MAX_STRING_LENGTH];
  P_char   target = NULL;
  int      town = 0;
  unsigned l;

  half_chop(argument, arg1, arg2);
  half_chop(arg2, arg3, arg4);

  if(!*arg1)
  {
    send_to_char(LWITNESS_SYNTAX, ch);
    return;
  }
  if((*arg1 == 't') || (*arg1 == 'T'))
  {
    if(is_number(arg3))
    {
      town = atoi(arg3);
      if((town < 1) || (town > LAST_HOME))
      {
        send_to_char("Invalid town number!\n", ch);
        return;
      }
    }
    else
    {
      /* validate the city...  */
      l = strlen(arg3);
      for (town = 1; town <= LAST_HOME; town++)
        if(!strn_cmp(arg3, town_name_list[town], l))
          break;
      if(town > LAST_HOME)
      {
        send_to_char("Invalid town name!\n", ch);
        return;
      }
    }
    if(!*arg4)
    {
      if(town > 0)
      {
        if(!hometowns[town - 1].crime_list)
        {
          sprintf(buf, "Crime record for %s is empty.\n",
                  town_name_list[town]);
          send_to_char(buf, ch);
          return;
        }
        sprintf(buf, "Crimes report for &+M%s&N:\n", town_name_list[town]);

        while ((crec = crime_find(hometowns[town - 1].crime_list,
                                  NULL, NULL, 0, NOWHERE, J_STATUS_NONE,
                                  crec)))
        {

          switch (crec->status)
          {
          case J_STATUS_WANTED:
            sprintf(buf, "%s&+R%s&N wanted for %s; reward %d platinums.\n",
                    buf, crec->attacker, crime_list[crec->crime],
                    crec->money);
            break;
          case J_STATUS_DEBT:
            sprintf(buf, "%s&+R%s&N owe %d plat. to the town.\n",
                    buf, crec->attacker, crec->money);
            break;
          case J_STATUS_PARDON:
            sprintf(buf, "%s&+R%s&N pardon %s for %s.\n",
                    buf, crec->victim, crec->attacker,
                    crime_list[crec->crime]);
            break;
          case J_STATUS_CRIME:
            sprintf(buf, "%s&+R%s&N commited %s against %s.\n",
                    buf, crec->attacker, crime_list[crec->crime],
                    crec->victim);
            break;
          case J_STATUS_IN_JAIL:
            sprintf(buf, "%s&+R%s&N is waiting in jail.\n",
                    buf, crec->attacker);
            break;
          case J_STATUS_JAIL_TIME:
            sprintf(buf, "%s&+R%s&N is in jail for %d days.\n",
                    buf, crec->attacker, crec->money);
            break;
          case J_STATUS_DELETED:
            break;
          case J_STATUS_NONE:
            sprintf(buf,
                    "%s&+R%s&N commited %s against %s, status &+c%s&n (%d)(%d).\n",
                    buf, crec->attacker, crime_list[crec->crime],
                    crec->victim, justice_status[crec->status], crec->money,
                    (int) crec->time);
            break;
          }
        }
        page_string(ch->desc, buf, 1);

        return;
      }
      else
      {
        send_to_char("Invalid town!\n", ch);
        return;
      }
    }
    else if(arg4[0] == '@')
    {
      sprintf(buf, "Special crimes report for &+M%s&N:\n",
              town_name_list[town]);
      while ((crec =
              crime_find(hometowns[town - 1].crime_list, NULL, NULL, 0,
                         NOWHERE, J_STATUS_NONE, crec)))
      {
        sprintf(buf + strlen(buf),
                "&+R%s&N committed %s against %s, status &+c%s&n (%d)(%d).\n",
                crec->attacker, crime_list[crec->crime], crec->victim,
                justice_status[crec->status], crec->money, (int) crec->time);
      }
      page_string(ch->desc, buf, 1);
      return;

    }
    else
    {
      while ((crec = crime_find(hometowns[town - 1].crime_list,
                                arg4, NULL, 0, NOWHERE, J_STATUS_NONE, NULL)))
      {
        crime_remove(town, crec);
      }
      sprintf(buf, "Records removed for &+M%s&N.\n", arg4);
      send_to_char(buf, ch);
    }
  }
  else if((*arg1 == 'c') || (*arg1 == 'C') || (*arg1 == 'm') ||
           (*arg1 == 'M'))
  {
    if(!(target = get_char_vis(ch, arg3)))
    {
      send_to_char("No-one by that name around.\n", ch);
      return;
    }
    sprintf(buf, "Witness record of &+M%s&N:\n", J_NAME(target));

    if(!target->specials.witnessed)
    {
      sprintf(buf, "%sWitness record is empty.\n", buf);
    }
    else
    {
      while ((rec = witness_find(target->specials.witnessed,
                                 NULL, NULL, 0, NOWHERE, rec)))
      {
        sprintf(buf + strlen(buf), "&+R%s&N committed %s against %s at %s.\n",
                rec->attacker, crime_list[rec->crime], rec->victim,
                world[rec->room].name);
      }
    }
    page_string(ch->desc, buf, 1);
    return;
  }
  else
  {
    send_to_char(LWITNESS_SYNTAX, ch);
    return;
  }
}

void do_echot(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  P_desc   d;
  char     name[MAX_INPUT_LENGTH], message[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];

  half_chop(argument, name, message);

  if(!*name || !*message)
  {
    send_to_char("Who do you wish to echot to??\n", ch);
    return;
  }

  vict = NULL;
  /*
   * switching to descriptor list, rather than get_char_vis, since it
   * was lagging hell out of things. JAB
   */
  for (d = descriptor_list; d; d = d->next)
  {
    if(!d->character || d->connected || !d->character->player.name)
      continue;
    if(!isname(d->character->player.name, name))
      continue;
    vict = d->character;
    break;
  }
  if(!vict)
  {
    send_to_char("No-one by that name here...\n", ch);
    return;
  }
  else if(ch == vict)
  {
    send_to_char("You try to echot yourself something.\n", ch);
    return;
  }
  else if(!vict->desc)
  {
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if(ch->desc)
  {
    if(IS_SET(ch->specials.act, PLR_ECHO))
    {
      sprintf(Gbuf1, "&+WYou echot %s '%s'\n", GET_NAME(vict), message);
      send_to_char(Gbuf1, ch);
    }
    else
    {
      send_to_char("Ok.\n", ch);
    }
  }
  if(get_property("logs.chat.status", 0.000) && IS_PC(ch) && IS_PC(vict))
    logit(LOG_CHAT, "%s echot's to %s '%s'", GET_NAME(ch), GET_NAME(vict), message);
  strcat(message, "\n");
  send_to_char(message, vict);
  write_to_pc_log(vict, message, LOG_PRIVATE);
}

int vnum_mobile(char *searchname, struct char_data *ch)
{
  int      i, found = 0, count = 0, length;
  char     pattern[MAX_INPUT_LENGTH], mobile_name[MAX_INPUT_LENGTH];
  char     buff[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  P_char   t_mob;

  strcpy(pattern, searchname);
  strToLower(pattern);

  strcpy(buff, "&+C__________Mobiles__________&n\n");
  for (i = 0; i <= top_of_mobt; i++)
    if((t_mob = read_mobile(mob_index[i].virtual_number, VIRTUAL)))
    {
      if(IS_SET(t_mob->specials.act, ACT_SPEC))
        REMOVE_BIT(t_mob->specials.act, ACT_SPEC);
      char_to_room(t_mob, ch->in_room, -2);
      strcpy(mobile_name, /*strip_color */ (t_mob->player.short_descr));
      strToLower(mobile_name);
      sprintf(buf, "%6d  %5d  %-s\n",
              mob_index[i].virtual_number, mob_index[i].number - 1,
              (t_mob->player.short_descr) ?
              t_mob->player.short_descr : "None");
      count++;
      if(t_mob)
      {
        extract_char(t_mob);
        t_mob = NULL;
      }
      else
      {
        logit(LOG_EXIT, "GLITCH 1");
        raise(SIGSEGV);
      }
      if((strlen(buf) + length + 40) < MAX_STRING_LENGTH)
      {
        strcat(buff, buf);
        length += strlen(buf);
      }
      else
      {
        strcat(buff, "Too many mobiles to list...\n");
        break;
      }
    }
    else
      logit(LOG_DEBUG, "vnum_mobile(): mob %d not loadable",
            mob_index[i].virtual_number);
  strcat(buff, "\n");
  sprintf(buff + strlen(buff),
          "&+LTotal mobs of this name in database:&n %d\n", count);
  page_string(ch->desc, buff, 1);

  return (found);
}


#if 0
int vnum_object(char *searchname, struct char_data *ch)
{
  int      nr, found = 0;
  char     pattern[MAX_INPUT_LENGTH], object_name[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH];

  strcpy(pattern, searchname);
  strToLower(pattern);

  strcpy(buf, "&+C__________Objects__________&n\n");
  for (nr = 0; nr <= top_of_objt; nr++)
  {

    strcpy(object_name, strip_color(obj_index[nr].short_description));
    strToLower(object_name);

    if(strstr(object_name, pattern) != NULL)
    {
      if(strlen(buf) > MAX_STRING_LENGTH - 50)
      {
        sprintf(buf, "%s\n...and the list goes on...\n", buf);
        break;
      }
      else
      {
        sprintf(buf, "%s%3d. &+B[%5d] &n%s\n", buf, ++found,
                obj_index[nr].virtual_number,
                obj_index[nr].short_description);
      }
    }
  }
  page_string(ch->desc, buf, 1);

  return (found);
}
#endif
int vnum_room(char *searchname, struct char_data *ch)
{
  int      nr, found = 0;
  char     pattern[MAX_INPUT_LENGTH], room_name[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH];

  strcpy(pattern, searchname);
  strToLower(pattern);

  strcpy(buf, "&+C__________Rooms__________&n\n");

  for (nr = 0; nr <= top_of_world; nr++)
  {
    strcpy(room_name, strip_color(world[nr].name));
    strToLower(room_name);

    if(strstr(room_name, pattern) != NULL)
    {
      if(strlen(buf) > MAX_STRING_LENGTH - 50)
      {
        sprintf(buf, "%s\n...and the list goes on...\n", buf);
        break;
      }
      else
      {
        sprintf(buf, "%s%3d. &+B[%5d]&n %s\n", buf, ++found,
                world[nr].number, world[nr].name);
      }
    }
  }
  page_string(ch->desc, buf, 1);

  return (found);
}

void do_unspec(P_char ch, char *argument, int cmd)
{
  int      i;
  P_char   victim = NULL;
  char     buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf)
  {
    send_to_char("Who do you want to unspec?\n", ch);
    return;
  }
  if(!(victim = get_char_room_vis(ch, buf)))
  {
    send_to_char("Who do you want to unspec?\n", ch);
    return;
  }
  if(IS_NPC(victim))
    return;

  update_skills(ch);

  if(!IS_SPECIALIZED(victim))
  {
    victim->only.pc->time_unspecced = 0;
    send_to_char("They are not specialized.\n", ch);
    return;
  }
  victim->player.spec = 0;
  send_to_char("You are no longer specialized.\n", victim);
  send_to_char("They are no longer specialized.\n", ch);
}

void do_RemoveSpecTimer(P_char ch, char *argument, int cmd)
{
  P_char   victim = NULL;
  char     buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if(!*buf)
  {
    send_to_char("Who's timer do you want to remove?\n", ch);
    return;
  }
  if(!(victim = get_char_room_vis(ch, buf)))
  {
    send_to_char("Who's timer do you want to remove?\n", ch);
    return;
  }
  if(IS_NPC(victim))
    return;
  if(!IS_SET(victim->specials.act2, PLR2_SPEC_TIMER))
  {
    send_to_char("They're timer isn't set.\n", ch);
    return;
  }
  REMOVE_BIT(ch->specials.act2, PLR2_SPEC_TIMER);
  send_to_char("Thier timer has been removed.\n", ch);
}

void tranquilize(P_char ch, P_char victim)
{
  if(!victim)
    return;
    
  if(!IS_TRUSTED(ch))
  {
    if(GET_LEVEL(victim) >= GET_LEVEL(ch))
      return;
  }
  
  if(victim->specials.fighting)
  {
    for (P_char tch = world[victim->in_room].people; tch; tch = tch->next_in_room)
    {
      if(tch->specials.fighting && (tch->specials.fighting == victim))
      {
        stop_fighting(tch);
        clearMemory(tch);
      }
    }

    stop_fighting(victim);
    clearMemory(victim);
  }

  act("$n begins to speak in a monotone voice. You nod off immediately out of boredom.", FALSE, ch, 0, victim, TO_VICT);
  if(!IS_TRUSTED(victim))
  {
    do_sleep(victim, 0, 0);
  }
}

void do_tranquilize(P_char ch, char *argument, int cmd)
{
  if(!ch || !IS_PC(ch) || !IS_TRUSTED(ch))
    return;

  char buf[100];

  one_argument(argument, buf);

  if(*buf)
  {
    // one target
    P_char victim = get_char_room_vis(ch, buf);

    if(!victim)
    {
      send_to_char("Nobody with that name here.\n", ch);
      return;
    }

    act("You begin to speak in a monotone voice. $N nods off immediately out of boredom.", FALSE, ch, 0, victim, TO_CHAR);
    tranquilize(ch, victim);
  }
  else
  {
    act("You begin to speak in a monotone voice. The entire room nods off immediately out of boredom.", FALSE, ch, 0, 0, TO_CHAR);
    // all
    for(P_char tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if(tch == ch || IS_TRUSTED(tch))
        continue;

      tranquilize(ch, tch);
    }

  }

}

/* Storage command:
 * Item needs to be a type of ITEM_CONTAINER or ITEM_STORAGE
 * storage new <item vnum> - loads a new object setup to store through boots in room.
 * storage delete <item name> - deletes a storage object and all contents within.
 * storage remove <item name> - deletes a storage object and drops all contents to room.
 */

void do_storage(P_char ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH], subcmd[MAX_STRING_LENGTH], objarg[MAX_STRING_LENGTH];
  P_obj s_obj, tmpobj, next_obj;

  arg = one_argument(arg, subcmd);
  arg = one_argument(arg, objarg);

  if( !strcmp(subcmd, "new") )
  {
    if(!(s_obj = read_object(atoi(objarg), VIRTUAL)))
    {
      send_to_char("&+RError&n: Please choose a valid object vnum.\n", ch);
      return;
    }
    if( (s_obj->type != ITEM_STORAGE) && (s_obj->type != ITEM_CONTAINER) )
    {
      send_to_char("&+RError&n: The item on file must be of type STORAGE or CONTAINER\n", ch);
      extract_obj(s_obj, TRUE);
      return;
    }
    if(s_obj->type == ITEM_CONTAINER)
    {
      s_obj->type = ITEM_STORAGE;
    }
    if(s_obj->type == ITEM_STORAGE)
    {
      if(IS_SET(s_obj->wear_flags, ITEM_TAKE))
      {
        REMOVE_BIT(s_obj->wear_flags, ITEM_TAKE);
      }
      obj_to_room(s_obj, ch->in_room);
      writeSavedItem(s_obj);
      send_to_char("This object now loads here without being in a .zon file.  Please remove it from the .zon file if necessessary to prevent double loading and confusion.\n", ch);
      wizlog(GET_LEVEL(ch), "%s loads %s as a storage unit in room %d.",
        J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
      logit(LOG_WIZ, "%s loads %s as a storage unit in room %d.",
        J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
      return;
    }
    else
    {
      extract_obj(s_obj, TRUE);
    }
    send_to_char("Syntax: storage new <item vnum> - load a new object for storage\n" \
                 "        storage delete <item name> - delete a storage item and all contents within\n" \
                 "        storage remove <item name> - delete a storage item and drop contents to room\n", ch);
    return;
  }
  else if( !strcmp(subcmd, "delete") )
  {
    if( (s_obj = get_obj_in_list(objarg, world[ch->in_room].contents))
      && (s_obj->type == ITEM_STORAGE) )
    {
      extract_obj(s_obj, TRUE);
      PurgeSavedItemFile(s_obj);
      wizlog( GET_LEVEL(ch), "%s deletes storage item %s from room %d.",
        J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
      logit(LOG_WIZ, "%s deletes storage item %s from room %d.",
        J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
      return;
    }
    else
    {
      send_to_char("&+RError&n: You don't see a storage item with that name in this room.\n", ch);
      return;
    }
  }
  else if( !strcmp(subcmd, "remove") )
  {
    if( (s_obj = get_obj_in_list(objarg, world[ch->in_room].contents))
      && (s_obj->type == ITEM_STORAGE) )
    {
      for (tmpobj = s_obj->contains; tmpobj; tmpobj = next_obj)
      {
        next_obj = tmpobj->next_content;
        obj_from_obj(tmpobj);
        obj_to_room(tmpobj, ch->in_room);
      }
      extract_obj(s_obj, TRUE);
      PurgeSavedItemFile(s_obj);
      wizlog(GET_LEVEL(ch), "%s removes storage item %s from room %d.",
        J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
      logit(LOG_WIZ, "%s remove storage item %s from room %d.",
        J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
      return;
    }
    else
    {
      send_to_char("&+RError&n: You don't see a storage item with that name in this room.\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("Syntax: storage new <item vnum> - load a new object for storage\n" \
                 "        storage delete <item name> - delete a storage item and all contents within\n" \
                 "        storage remove <item name> - delete a storage item and drop contents to room\n", ch);
    return;
  }
}

void do_newb_spellup_all(P_char ch, char *arg, int cmd)
{
  P_desc d;

  for( d = descriptor_list; d; d = d->next )
  {
    if( d->connected == CON_PLYNG && ch != d->character )
    {
      if (GET_LEVEL(d->character) <= 36)
      {
        do_newb_spellup(ch, GET_NAME(d->character), CMD_NEWBSU);
      }
    }
  }
}

void do_newb_spellup(P_char ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  P_char victim;

  one_argument(arg, buf);

  if(*arg)
  {
    if( !(victim = get_char_vis(ch, buf)) )
    {
      send_to_char("Nobody with that name.\n", ch);
      return;
    }

    if(IS_NPC(victim))
    {
      send_to_char("Only works on players.\n\r", ch);
      return;
    }

    wizlog(58, "(%s) newb buffed: (%s).", GET_NAME(ch), GET_NAME(victim));
    logit(LOG_WIZ, "(%s) newb buffed: (%s).", GET_NAME(ch), GET_NAME(victim));

    //Insert cool ascii shit here
    if(isname("Jexni", ch->player.name))
    {
      send_to_char(file_to_string("lib/creation/hypnotoad"), victim);
    }

    //And lets spellup the noob!  A little good will goes a long way.
    spell_armor(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_bless(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_fly(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_stone_skin(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_haste(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_strength(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_agility(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_dexterity(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_barkskin(61, victim, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_accel_healing(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
    spell_spirit_armor(61, victim, 0, SPELL_TYPE_SPELL, victim, 0);

    send_to_char("Done.", ch);
    send_to_char("\nEnjoy your blessings.\n", victim);
  }
}

void do_givepet(P_char ch, char *arg, int cmd)
{
  char msg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH], pet[MAX_STRING_LENGTH];
  P_char mob = NULL;

  arg = one_argument(arg, buf);
  arg = one_argument(arg, pet);

  if(*buf && *pet)
  {
    P_char victim = get_char_room_vis(ch, buf);

    if(!victim)
    {
      send_to_char("Nobody with that name here.\n", ch);
      return;
    }
    // debug("found victim");
    if(isname("dog", pet))
    {
      mob = read_mobile(28124, VIRTUAL);
      sprintf(msg, "$N runs up to you and barks playfully.");
    }
    else if(isname("cat", pet))
    {
      mob = read_mobile(74132, VIRTUAL);
      sprintf(msg, "$N walks up to you and rubs up against your leg.");
    }
    else if(isdigit(*pet))
    {
      msg[0] = '\0';
      mob = read_mobile(atoi(pet), VIRTUAL);
      if( !mob )
      {
         send_to_char( "Let's try something real!\n", ch );
         return;
      }
      wizlog(56, "%s has loaded pet %s(Level: %d) for %s.", GET_NAME(ch), mob->player.short_descr, GET_LEVEL(mob), GET_NAME(victim));
      logit(LOG_WIZ, "(%s) has loaded pet (%s)(Level: %d) for (%s).",
        GET_NAME(ch), mob->player.short_descr, GET_LEVEL(mob), GET_NAME(victim));
    }
    else
    {
       send_to_char( "Valid pets are 'dog', 'cat', or vnum of mob!\n", ch );
       return;
    }

    if(mob)
    {
      SET_BIT(mob->specials.act, ACT_SENTINEL);
      mob->only.npc->aggro_flags = 0;
      char_to_room(mob, victim->in_room, 0);
      setup_pet(mob, victim, -1, PET_NOCASH | PET_NOAGGRO);
      if(msg)
        act(msg, TRUE, victim, 0, mob, TO_CHAR);
      add_follower(mob, victim);
    }
    else
    {
      debug("givepet(): Error loading mob.");
    }
  }
  else
  {
    send_to_char("Syntax: givepet playername [dog|cat|mob_vnum]", ch);
  }

  return;
}

void do_petition_block(P_char ch, char *argument, int cmd)
{
  P_char   vict;
  P_obj    dummy;
  char     buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch) ||
     !IS_TRUSTED(ch))
        return;

  if(GET_LEVEL(ch) < FORGER)
  {
    send_to_char("You are not yet able to block player petitions.\r\n", ch);
    return;
  }
  
  one_argument(argument, buf);
  
  if(!*buf)
    send_to_char("Usage: petition_block <player>\r\n", ch);
  else if(!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
    send_to_char("Cannot find target.\r\n", ch);
  else if(IS_NPC(vict))
    send_to_char("Can't do that to a mobile.\n", ch);
  else if(GET_LEVEL(vict) >= GET_LEVEL(ch))
    act("Not a chance.", 0, ch, 0, vict, TO_CHAR);
  else if(IS_SET(vict->specials.act2, PLR2_B_PETITION))
  {
    send_to_char("The petition block was removed.\r\n", vict);
    send_to_char("Petition block removed.\n", ch);
    
    REMOVE_BIT(vict->specials.act2, PLR2_B_PETITION);
    
    wizlog(GET_LEVEL(ch), "%s just removed the petition block on %s.",
      GET_NAME(ch), GET_NAME(vict));
    logit(LOG_WIZ, "%s just removed the petition block on %s.",
      GET_NAME(ch), GET_NAME(vict));
    //sql_log(ch, WIZLOG, "Removed petition block on %s", GET_NAME(vict));
  }
  else
  {
    send_to_char("&+WThe gods take away your ability to use the &+Rpetition &+Wchannel.\r\n", vict);
    send_to_char("Petition block set.\r\n", ch);
    
    SET_BIT(vict->specials.act2, PLR2_B_PETITION);

    wizlog(GET_LEVEL(ch), "%s was just PETITION BLOCKED by %s.",
      GET_NAME(vict), GET_NAME(ch));
    logit(LOG_WIZ, "%s was just PETITION BLOCKED by %s.",
      GET_NAME(vict), GET_NAME(ch));
    //sql_log(ch, WIZLOG, "Petition blocked %s", GET_NAME(vict));
  }
}

void do_questwhere(P_char ch, char *arg, int cmd)
{
   P_obj obj;
   char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
   int count = 0, length = 0;
   bool stop = FALSE;

   if( !ch )
      return;

   while( *arg == ' ' )
      arg++;
   if( !(*arg) )
   {
      send_to_char( "The questwhere command requires an argument.\n", ch );
      return;
   }

   buf[0] = '\0';

   for( obj = object_list; obj && !stop; obj = obj->next )
   {
      if( isname( arg, obj->name) && is_quested_item( obj ) )
      {
         sprintf( buf2, "%3d. [%6d] %s - ", ++count, 
            obj_index[obj->R_num].virtual_number,
            pad_ansi( obj->short_description, 40).c_str() );
            strcat( buf2, where_obj( obj, FALSE ));
            strcat( buf2, "\n" );
         if( strlen(buf2) + length + 35 > MAX_STRING_LENGTH)
         {
            strcpy( buf2, " ... The list is too long.\n" );
            stop = TRUE;
         }
         strcat( buf, buf2 );
         length += strlen(buf2);
      }
   }
   if( !count )
      send_to_char( "No items found.\n", ch );
   else
      page_string( ch->desc, buf, 1);
}

bool is_quested_item( P_obj obj )
{
   struct quest_complete_data *qcp;
   struct goal_data *gp;
   int vnum = obj_index[obj->R_num].virtual_number;
   int count = 0;
   static int count2 = 0;

   // For each quest on the mud.
   for( qcp = quest_index[0].quest_complete; count < number_of_quests;
      qcp = quest_index[++count].quest_complete )
   {
      // If there is quest completion data, look through it
      if( qcp )
      for( gp = qcp->receive; gp; gp = gp->next )
      {
         // If the item received from the quest has the right vnum
         if( gp->goal_type == QUEST_GOAL_ITEM )
         {
            if( gp->number == vnum )
               return TRUE;
         }
      }
   }
   return FALSE;
}

void do_setship( P_char ch, char *arg )
{
  char   name[MAX_STRING_LENGTH];
  char   Gbuf1[MAX_STRING_LENGTH];
  P_ship ship;

  arg = one_argument(arg, name);
  if( (ship = get_ship_from_owner(name)) != NULL )
  {
// Debugging:
//    sprintf( Gbuf1, "Found ship '%s', Owner: '%s'.\n\r", ship->name, ship->ownername );
//    send_to_char( Gbuf1, ch );

    arg = one_argument(arg, name);
    if( isname( name, "frags") )
    {
      arg = one_argument(arg, name);
      if( !*name || atoi(name) < 0 )
      {
        send_to_char( "You must enter a postive amount to set ship frags to.\n\r", ch );
        return;
      }
      sprintf( Gbuf1, "Name: '%s&n' Owner: '%s&n'\n\rOld Ship Frags: %d, ", ship->name, ship->ownername, ship->frags );
      send_to_char( Gbuf1, ch );
      ship->frags = atoi(name);
      sprintf( Gbuf1, "New Ship frags: %d.\n\r", ship->frags );
      send_to_char( Gbuf1, ch );
    }
    else if( isname( name, "guns") )
    {
      arg = one_argument(arg, name);
      if( !*name )
      {
        send_to_char( "You must enter an amount to raise the guns crew exp.\n\r", ch );
        return;
      }
      ship->guns_skill_raise( ch, atof(name) );
    }
    else if( isname( name, "repair") )
    {
      arg = one_argument(arg, name);
      if( !*name )
      {
        send_to_char( "You must enter an amount to raise the repair crew exp.\n\r", ch );
        return;
      }
      ship->rpar_skill_raise( ch, atof(name) );
    }
    else if( isname( name, "sail") )
    {
      arg = one_argument(arg, name);
      if( !*name )
      {
        send_to_char( "You must enter an amount to raise the sail crew exp.\n\r", ch );
        return;
      }
      ship->sail_skill_raise( ch, atof(name) );
    }
    else
    {
      send_to_char( "Valid Ship settings: 'frags', 'guns', 'repair', or 'sail'.\n\r", ch );
    }
  }
  else
  {
    sprintf( Gbuf1, "Couldn't find ship '%s'.\n\r", name );
    send_to_char( Gbuf1, ch );
  }
}
