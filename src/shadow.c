// I've put here all shadow related stuff since it was huge, bloated
// and never really worked

struct char_special_data {
  ulong affected_by;            /* Bitvector for spells/skills affected by */
  ulong affected_by2;           /* Bitvector for spells/skills affected by */
  ulong affected_by3;
  ulong affected_by4;
  ulong affected_by5;

  byte x_cord;                  /* Sub-coordinate of large room            */
  byte y_cord;
  byte z_cord;                  /* hieght for flyers                       */
  int death_exp;

  ubyte position;               /* posture and status                      */
  ulong act;                    /* flags for NPC behavior                  */
  ulong act2;                   /* extra toggles - Zod                     */

  int		base_combat_round;
  int		combat_tics;
  float		damage_mod;

  ush_int guild;                /* which guild? 			   */
  int guild_status;             /* rank, how you enter, etc.    	   */

  int carry_weight;             /* Carried weight                          */
  ush_int carry_items;          /* Number of items carried                 */
  int was_in_room;              /* previous room char was in               */
  byte apply_saving_throw[5];   /* Saving throw (Bonuses)                  */
  byte conditions[5];           /* Drunk full etc.                         */
  byte disease_type[5];
  byte disease_dura[5];
  sh_int alignment;             /* +-1000 for alignments                   */
  sh_int orig_align;            /* true neutral races - align at creation  */
  int tracking;   /* room you are tracking to*/
  bool command_delays;          /* input not processed while TRUE 	   */
  P_char consent;
  P_char fighting;              /* Opponent                                */
  P_char was_fighting;          /* time to see who I was fighting, mainly for assassin track */
  /* fighting list's next-char pointer thing is good, but too memhoggy
     if we want more combat values.. so.. */
#ifdef REALTIME_COMBAT
  P_combat combat;
#else
  P_char next_fighting;         /* For fighting list             */
#endif

  struct skill_usage_struct skill_usage[MAX_SKILL_USAGE];

  sh_int timer;
  wtns_rec *witnessed;           /* linked list of witness records */
  P_char arrest_by;

  struct shadow_data shadow;    /* data pertaining to shadow skill --TAM 7-6-94 */

  struct char_paladin_aura *aura;


  struct char_numbered_spells numbered_spells;
  char undead_spell_slots[MAX_CIRCLE+1];
  int undead_ticks;

  P_obj polymorphed_obj;	/* points to obj char is polymorphed into */
};

struct shadow_data {
  P_char shadowing;
  bool shadow_move;
  bool valid_last_move;
  int room_last_in;
  struct shadower_data *who;
};

struct shadower_data {
  P_char shadower;
  byte dir_last_move;
  byte num_of_moves;
  struct shadower_data *next;
};



/*
   Support function for shadow skill.

   * 'shadower' MUST be a pointer to the char who is doing the shadowing.
   * 'shadowed' MUST be a pointer to the char who is being shadowed.
   *
   * Assumption(s): 'shadower' is in 'shadowed' list
   *
   *    --TAM 7-1-94
 */

void FreeShadowedData(P_char shadower, P_char shadowed)
{
  struct shadower_data *sh_ptr, *tmp_ptr;

  shadower->specials.shadow.shadowing = NULL;

  if (shadowed->specials.shadow.who->shadower == shadower) {
    shadowed->specials.shadow.who = shadowed->specials.shadow.who->next;
    FREE(shadowed->specials.shadow.who);
    shadowed->specials.shadow.who = NULL;
  } else {
    for (sh_ptr = shadowed->specials.shadow.who;
         sh_ptr->next;
         sh_ptr = sh_ptr->next) {
      if (sh_ptr->next->shadower == shadower) {
        tmp_ptr = sh_ptr->next;
        sh_ptr->next = sh_ptr->next->next;
        FREE(tmp_ptr);
        tmp_ptr = NULL;
        break;
      }
    }
  }

  if (IS_AFFECTED(shadower, AFF_SHADOW)) {
    REMOVE_BIT(shadower->specials.affected_by, AFF_SHADOW);
  }
}

/*
   Called from extract_char() if char is being shadowed. --TAM 7-5-94
 */

void StopShadowers(P_char ch)
{
  struct shadower_data *sh_ptr, *sh_next;

  for (sh_ptr = ch->specials.shadow.who; sh_ptr; sh_ptr = sh_next) {
    sh_next = sh_ptr->next;
    sh_ptr->shadower->specials.shadow.shadowing = NULL;
    REMOVE_BIT(sh_ptr->shadower->specials.affected_by, AFF_SHADOW);
    act("You lost $N.", FALSE, sh_ptr->shadower, 0, ch, TO_CHAR);
    FREE(sh_ptr);
  }

  ch->specials.shadow.who = NULL;
}

/*
   Support function for shadow skill.

   * This function recursively counts the number of shadowers
   * effectively following 'ch'.  This count is ultimately returned
   * by the function.
   *
   *   --TAM 7-6-94
 */

int CountNumShadowers(P_char ch)
{
  struct shadower_data *sh;
  int count = 0;

  sh = ch->specials.shadow.who;

  if (sh == NULL) {
    return 0;
  } else {
    while (sh) {
      ++count;
      count += CountNumShadowers(sh->shadower);
      sh = sh->next;
    }
  }

  return count;
}

/* Support function for shadow skill.  --TAM 7-1-94 */

void MoveShadower(P_char ch, int to_room)
{
  struct shadower_data *sh_ptr, *sh_next;
  char buf[128];
  int i, j, fence, num_fol, num_shad;
  int noise_lev = 0, noise_lev_chart[] =
  {3, 5, 8, 13, 20, 30};

  /*
     if a char is moved from room by 'trans' or 'teleport spell' or by any
     other magical or godly means, the shadowing stops.  "valid_last_move" is
     set to TRUE in do_simple_move(), when it's definite that the char being
     shadowed will move.
   */

  if (ch->specials.shadow.valid_last_move == FALSE) {
    if (to_room != ch->specials.shadow.room_last_in) {
      StopShadowers(ch);
    }
    return;
  }
  /* count the # of char following/shadowing victim.. # affects */
  /* chance each is heard. */

  num_fol = 0;
  num_shad = 0;

  for (sh_ptr = ch->specials.shadow.who; sh_ptr; sh_ptr = sh_ptr->next) {
    ++num_shad;
    num_fol += CountNumFollowers(sh_ptr->shadower);
    num_shad += CountNumShadowers(sh_ptr->shadower);
  }

  i = num_fol + (num_shad >> 1);
  if (i > 0) {
    if (i > 6) {
      noise_lev = 35;
    } else {
      --i;
      noise_lev = noise_lev_chart[i];
    }
  }
  /*
     Now, there's one of three possibilities which must be handled differently:

     a) the char being shadowed moves into same room as shadower. b) the char
     being shadowed moved into a room adjacent to shadower. c) the char being
     shadowe is already in a room adjacent to the shadower, and moves one room
     further away from shadower.

     Each is handled in the order listed above.
   */

  for (sh_ptr = ch->specials.shadow.who; sh_ptr; sh_ptr = sh_next) {
    sh_next = sh_ptr->next;
    if (to_room == sh_ptr->shadower->in_room) {
      act("You see $N coming back this way.", FALSE, sh_ptr->shadower, 0, ch, TO_CHAR);
      do_hide(sh_ptr->shadower, 0, CMD_SHADOW);
      sh_ptr->dir_last_move = NOWHERE;
    } else if (sh_ptr->dir_last_move && (sh_ptr->dir_last_move == NOWHERE)) {
      for (i = 0; i < NUM_EXITS; i++) {
        /* check for existence pointer added -- DTS 7/9/95 */
        if (world[ch->specials.shadow.room_last_in].dir_option[i] &&
            world[ch->specials.shadow.room_last_in].dir_option[i]->to_room == to_room) {
          sh_ptr->dir_last_move = i;
          break;
        }
      }
    } else {
      sh_ptr->shadower->specials.shadow.shadow_move = TRUE;
      do_move(sh_ptr->shadower, 0, exitnumb_to_cmd(sh_ptr->dir_last_move));
      sh_ptr->shadower->specials.shadow.shadow_move = FALSE;
      /*
         if for some reason shadower couldn't follow victim (ie. closed door,
         need levitate or fly, et cetera.), have em stop shadowing.
       */

      if (sh_ptr->shadower->in_room != ch->specials.shadow.room_last_in) {
        act("You lost $N.", FALSE, sh_ptr->shadower, 0, ch, TO_CHAR);
        FreeShadowedData(sh_ptr->shadower, ch);
        continue;
      }
      j = sh_ptr->dir_last_move;

      for (i = 0; i < NUM_EXITS; i++) {
        if (world[ch->specials.shadow.room_last_in].dir_option[i] &&
        (world[ch->specials.shadow.room_last_in].dir_option[i]->to_room ==
         to_room)) {
          sh_ptr->dir_last_move = i;
          break;
        }
      }

      sprintf(buf, "You shadow $N %s as $E leaves %s.", dirs[j], dirs[i]);
      act(buf, FALSE, sh_ptr->shadower, 0, ch, TO_CHAR);

      sh_ptr->num_of_moves++;

      /* does char being shadowed hear or sense a hidden presence? */
      /* awareness affect increases chances of sensing */

      if (sh_ptr->num_of_moves >= MAX_SHADOW_MOVES) {
        if (IS_PC(sh_ptr->shadower))
          fence = GET_CHAR_SKILL(sh_ptr->shadower, SKILL_SHADOW);
        else
          fence = 0;

        if (IS_AFFECTED(ch, AFF_AWARE)) {
          fence -= SHADOW_AWARE_PENALTY;
        }
        fence -= noise_lev;     /* calculated above */

        if (number(1, 101) > fence) {
          switch (world[sh_ptr->shadower->in_room].sector_type) {
          case SECT_INSIDE:
          case SECT_CITY:
          case SECT_ROAD:
          case SECT_UNDRWLD_LOWCEIL:
            switch (number(1, 2)) {
            case 1:
              act("You hear the echo of footsteps from behind.", FALSE, ch, 0, 0, TO_CHAR);
              break;
            case 2:
              act("You get the feeling you're being followed.", FALSE, ch, 0, 0, TO_CHAR);
              break;
            default:;
              /* shouldn't get here */
              break;
            }
            break;

          case SECT_FOREST:
          case SECT_HILLS:
            switch (number(1, 3)) {
            case 1:
              act("You hear the cracking of twigs from behind.",
                  FALSE, ch, 0, 0, TO_CHAR);
              break;
            case 2:
              act("You suddenly hear loud rustling of branches from behind.",
                  FALSE, ch, 0, 0, TO_CHAR);
              break;
            case 3:
              act("You hear muffled heavy breathing hidden in the thicket from behind.",
                  FALSE, ch, 0, 0, TO_CHAR);
              break;
            default:
              /* should never get here */
              break;
            }
            break;

          case SECT_MOUNTAIN:
          case SECT_UNDRWLD_MOUNTAIN:
            act("Somewhere behind you hear some small rocks tumble down the mountain.",
                FALSE, ch, 0, 0, TO_CHAR);
            break;

          case SECT_WATER_SWIM:
          case SECT_UNDERWATER:
          case SECT_UNDERWATER_GR:
          case SECT_UNDRWLD_SLIME:
            switch (number(1, 3)) {
            case 1:
              act("You suddenly hear a loud splash from behind.",
                  FALSE, ch, 0, 0, TO_CHAR);
              break;
            case 2:
              act("You hear a faint plop from behind.",
                  FALSE, ch, 0, 0, TO_CHAR);
              break;
            case 3:
              act("There are footsteps slightly splashing the water behind you.",
                  FALSE, ch, 0, 0, TO_CHAR);
              break;
            default:
              /* should never reach this pt */
              break;
            }
            break;

          case SECT_OCEAN:
          case SECT_FIREPLANE:
          case SECT_UNDRWLD_MUSHROOM:
          case SECT_UNDRWLD_LIQMITH:
            act("You're suddenly stricken with the eerie chill of being followed.",
                FALSE, ch, 0, 0, TO_CHAR);
            break;

          default:              /* should never reach this case */
            sprintf(buf, "MoveShadower(): (real) room %d with unknown sector type %d.",
                    sh_ptr->shadower->in_room, world[sh_ptr->shadower->in_room].sector_type);
            logit(LOG_DEBUG, buf);
            break;
          }

          if (GET_C_INT(sh_ptr->shadower) > number(1, 200)) {
            act("You think $N has discovered you.", FALSE, sh_ptr->shadower, 0, ch, TO_CHAR);
          }
        }                       /* if heard */
        sh_ptr->num_of_moves = 0;
      }                         /* check for being heard */
    }                           /* else try to move shadower */
  }                             /* for each shadower */

  ch->specials.shadow.room_last_in = to_room;
  ch->specials.shadow.valid_last_move = FALSE;
}

/* Shadow skill.   --TAM 7-5-94 */

void do_shadow(P_char ch, char *argument, int cmd)
{
  P_char victim = NULL, shadowed = NULL;
  struct shadower_data *sdata, *tmp_ptr;
  char vict_name[MAX_INPUT_LENGTH];
  int fence, skl_lvl = 0;
   if(!GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF)){
      act("You don't know how to shadow!", TRUE, ch, 0, 0, TO_CHAR);
      return;
   }
 // if (GET_LEVEL(ch) < 57) {
   // send_to_char("Sorry, but this command is temporarily disabled.\r\n", ch);
   // return;
 // }

  one_argument(argument, vict_name);

  if (IS_PC(ch))
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_SHADOW);

  if (!skl_lvl) {
    act("You don't know how to shadow!", TRUE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!str_cmp(vict_name, "me")) {
    victim = ch;
  } else if (!(victim = get_char_room_vis(ch, vict_name))) {
    send_to_char("Shadow whom?\r\n", ch);
    return;
  }
  if (IS_SHADOWING(ch)) {
    shadowed = GET_CHAR_SHADOWED(ch);

    act("You stop shadowing $N.", FALSE, ch, 0, shadowed, TO_CHAR);
    FreeShadowedData(ch, shadowed);

    if (ch == victim) {
      ch->specials.shadow.shadowing = NULL;
      return;
    }
  } else {
    if (ch == victim) {
      act("Try as you may, silly person.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    /* stop following victim and begin shadowing */
    if (ch->following) {
      stop_follower(ch);
    }
  }

  /* right off the bat, does victim notice being followed? */
  if (IS_PC(ch))
    fence = GET_CHAR_SKILL(ch, SKILL_SHADOW);
  else
    fence = 0;

  if (IS_AFFECTED(victim, AFF_AWARE)) {
    fence -= SHADOW_AWARE_PENALTY;
  }
  if (number(1, 101) > fence) {
    do_follow(ch, vict_name, CMD_SHADOW);
    return;
  }
  /* shadower has good start. */
  /* initialize shadower_data */

  CREATE(sdata, struct shadower_data, 1);
  sdata->shadower = ch;
  sdata->num_of_moves = 0;
  sdata->dir_last_move = NOWHERE;

  if (victim->specials.shadow.who) {
    tmp_ptr = victim->specials.shadow.who;
    victim->specials.shadow.who = sdata;
    victim->specials.shadow.who->next = tmp_ptr;
  } else {
    victim->specials.shadow.who = sdata;
    victim->specials.shadow.who->next = NULL;
  }

  victim->specials.shadow.room_last_in = victim->in_room;
  victim->specials.shadow.valid_last_move = FALSE;

  ch->specials.shadow.shadowing = victim;

  /* now tag em */

  SET_BIT(ch->specials.affected_by, AFF_SHADOW);

  act("You now follow $N, keeping to the shadows as you go.", FALSE, ch, 0, victim, TO_CHAR);
}

/*
 * this spell no longer comforms to standard spell_*() function
 * definition, this was done to fix a bug in locate object. -JAB
 */

void spell_locate_object(int level, P_char ch, char *arg)
{
  P_obj i;
  int j, k, length;
  bool flag = FALSE;
  char Gbuf1[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  if (!(ch)) {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  while (*arg == ' ')
    arg++;

  j = level >> 1;

  if (!*arg) {
    send_to_char("What EXACTLY are you looking for?\r\n", ch);
    return;
  }
  if (strstr(arg, "coin")) {
    send_to_char("You ain't found shit.\r\n", ch);
    return;
  }
  k = j;

  Gbuf3[0] = 0;
  length = 0;

  for (i = object_list; i && (j > 0) && !flag; i = i->next)
    if ((isname(arg, i->name) || isname(arg, i->short_description)) &&
  CAN_SEE_OBJ(ch, i) && (!IS_SET(i->extra_flags, ITEM_NOLOCATE) ||
             IS_TRUSTED(ch))) {
      if ((i->type == ITEM_CORPSE))
  continue;
      if (i->type == ITEM_TELEPORT)
  continue;
      if (IS_ARTIFACT(i))
        continue;

      /* don't allow people to see objects carried/worn by opposite-
         raced players */

      if ((OBJ_CARRIED(i) || OBJ_WORN(i)) && racewar(ch, i->loc.carrying))
  continue;
      if ((OBJ_CARRIED(i) || OBJ_WORN(i)) && IS_AFFECTED3(i->loc.carrying,
                 AFF3_NON_DETECTION))
  continue;
      if (OBJ_CARRIED(i)) {
  if (!i->short_description)
    raise(SIGSEGV);
  sprintf(Gbuf1, "%s carried by %s.\r\n", i->short_description,
    ((level > MAXLVLMORTAL) ||
  (number((level - 7), (level + 7)) > GET_LEVEL(i->loc.carrying)) ?
     PERS(i->loc.carrying, ch, FALSE) : "someone"));
      } else if (OBJ_WORN(i)) {
  if (!i->short_description)
    raise(SIGSEGV);
  sprintf(Gbuf1, "%s equipped by %s.\r\n", i->short_description,
    ((level > MAXLVLMORTAL) ||
     (number((level - 7), (level + 7)) >
      GET_LEVEL(i->loc.wearing)) ?
     PERS(i->loc.wearing, ch, FALSE) : "someone"));
      } else if (OBJ_INSIDE(i)) {
  if (!i->short_description)
    raise(SIGSEGV);
  sprintf(Gbuf1, "%s in %s.\r\n", i->short_description,
    (level > MAXLVLMORTAL) ?
    i->loc.inside->short_description :
    "some sort of container.");
      } else {
  if (OBJ_ROOM(i)) {
    if (!i->short_description)
      raise(SIGSEGV);
    if (level > MAXLVLMORTAL) {
      sprintf(Gbuf1, "%s in %s [%d].\r\n", i->short_description,
        world[i->loc.room].name, world[i->loc.room].number);
    } else {
      sprintf(Gbuf1, "%s in %s.\r\n", i->short_description,
        world[i->loc.room].name);
    }
  } else
    logit(LOG_DEBUG, "yup, locate found something in NOWHERE");
      }
      if ((strlen(Gbuf1) + length + 150) > MAX_STRING_LENGTH) {
  strcpy(Gbuf1, "   ...and the list goes on...\r\n");
  flag = TRUE;
      }
      strcat(Gbuf3, Gbuf1);
      length = strlen(Gbuf3);

      if (!IS_TRUSTED(ch))
  j--;
    }
  if (!*Gbuf3)
    send_to_char("No such object.\r\n", ch);
  else
    page_string(ch->desc, Gbuf3, 1);
}

// Why was all of this code sitting in a global scope?

   //if( IS_AFFECTED5(opponent, AFF5_GRAPPLED) && (!IS_AFFECTED5(ch, AFF5_GRAPPLER ) )){
   //        act("$N is standing in your way.", FALSE, ch, 0, 0, TO_CHAR);
   //                act("$n tries to attack but can't reach!", FALSE, ch, 0, 0, TO_ROOM);
   //                      ch->specials.combat_tics = ch->specials.base_combat_round;

   //                        if (IS_FIGHTING(ch))
   //                                      stop_fighting(ch);
   //                          continue;

   //                             }


   //if(IS_AFFECTED5(ch, AFF5_GRAPPLED)){
   //        act("You try to break free but the grip is too strong!", FALSE, ch, 0, 0, TO_CHAR);
   //                act("$n tries to break free but the grid is too strong!", FALSE, ch, 0, 0, TO_ROOM);
   //                  ch->specials.combat_tics = ch->specials.base_combat_round;
   //                           continue;
   //                               }

   //if(IS_AFFECTED5(ch, AFF5_GRAPPLER) && IS_AFFECTED5(opponent, AFF5_GRAPPLED) ){
   //    if(!number(0,1)){
   //          //act("You continue the arm lock!", FALSE, ch, 0, 0, TO_CHAR);
   //      //        //act("$n continues locking $s opponents arms", FALSE, ch, 0, 0, TO_ROOM);
   //      //          }
   //      //            ch->specials.combat_tics = ch->specials.base_combat_round;
   //      //                       continue;
   //      //                           }
   //    //
   //    //
   //    //

   // if (affected_by_spell(ch, SPELL_WINDSTROM_BLESSING) && ch->equipment[PRIMARY_WEAPON]) {
   //             act("$N's body is covered with &+Bchilling &+CFrost&n as your $q strikes them.",
   //                         FALSE, ch, ch->equipment[PRIMARY_WEAPON], opponent, TO_CHAR);
   //                       act("Your body is covered with &+Bchilling &+CICE&n as $n's $q strikes you hard.",
   //                                   FALSE, ch, ch->equipment[PRIMARY_WEAPON], opponent, TO_VICT);
   //                                 act("&+LDeathly &+CIce&n covers $N as $n's $q strikes them hard.",
   //                                             FALSE, ch, ch->equipment[PRIMARY_WEAPON], opponent, TO_NOTVICT);
   //                                     } 

   // if (affected_by_spell(ch, SPELL_WINDSTROM_BLESSING) &&
   //            ch->equipment[PRIMARY_WEAPON] && num_hits) {
   //                struct generic_event_arguments args;
   //                             char buf[16];
   //                                          sprintf(buf, "%d", num_hits);
   //                                                       args.actor1 = ch;
   //                                                                    args.actor2 = opponent;
   //                                                                                 args.data = buf;
   //                                                                                              AddEvent(EVENT_INTERACTION, 0, TRUE, event_windstrom, &args);
   //                                                                                                      }

void event_grapple(P_char ch, P_char victim, P_obj obj, void *data)
{
     int percent = 100, skill, stages=1, dam=0, move;

          notch_skill(ch, SKILL_GRAPPLE, 17);
               act("...and then, in rapid succession you slam your head twice into\r\n$S face smashing it to a pulp. ",
                   FALSE, ch, 0, victim, TO_CHAR);
                          act("...then the taste of blood fills your mouth as your\r\nteeth are shattered by $s forehead.", F
                              ALSE, ch, 0, victim, TO_VICT);
                               act("$n slams $s head twice into $N face in rapid\r\nsuccession crushing $S face.", FALSE, ch, 0, victim,
                                    TO_NOTVICT);

                                 do_headbutt(ch, "", 0);
                                   affect_from_char(ch, SKILL_HEADBUTT);
                                           do_headbutt(ch, "", 0);

}

void do_grapple(P_char ch, char *arg, int cmd)
{

  P_char vict = NULL;
  P_char kala, kala2;
  int i, skl;
  struct affected_type af;
  int door, target_room;
  int percent_chance;

  if (!IS_FIGHTING(ch))
  {
    vict = ParseTarget(ch, arg);
    if (!vict)
    {
      send_to_char("Who do you plan to grapple?\r\n", ch);
      return;
    }
  }
  else
  {
    vict = ch->specials.fighting;
    if (!vict)
    {
      stop_fighting(ch);
      return;
    }
  }

  if (affected_by_spell(ch, SKILL_GRAPPLE))
  {
    send_to_char("You are not ready yet for another grapple attempt!\r\n", ch);
    return;
  }
  if (affected_by_spell(vict, SKILL_GRAPPLE))
  {
    send_to_char("Get in line, someone is already holding him!\r\n", ch);
    return;
  }

  if ( get_takedown_size(vict)  <  (get_takedown_size(ch) - 1) )
  {
    send_to_char("No way, they are way to small for a maneuver like that!\r\n", ch);
    return;
  }

                          
  if ( get_takedown_size(vict)  >  (get_takedown_size(ch) + 1) )
  {
    send_to_char("Ugh, you might want to pick on someone your own size!\r\n", ch);
    return;
  }
                            
  if (GET_CHAR_SKILL(ch, SKILL_GRAPPLE) == 0)
  {
    send_to_char("Yeah right, you suddenly change into a specialized mercenary?!?!.\r\n", ch);
    return;
  }

                                
  if (GET_POS(vict) != POS_STANDING)
  {
    act("You leap at $N only to realize $E is already down .", FALSE, ch, 0, vict, TO_CHAR);
    act("$n leaps at $N only to realize $E is already down.", FALSE, ch, 0, vict, TO_NOTVICT);
    act("$n leaps at you only to realize you are already down.",  FALSE, ch, 0, vict, TO_VICT);
    CharWait(ch, PULSE_VIOLENCE * 1);
    return;
  }

  skl = GET_CHAR_SKILL(ch, SKILL_GRAPPLE);
  if(skl > number(0,120))
  {

  CharWait(vict, PULSE_VIOLENCE * 3.1);
  CharWait(ch, PULSE_VIOLENCE * 4);

  for (kala = world[ch->in_room].people; kala; kala = kala2)
  {
    kala2 = kala->next_in_room;

    if (kala == ch)
      continue;
    if(kala == vict)
      continue;

    if(kala->specials.fighting == ch)
    {
      stop_fighting(kala);
                                                                                           
      act("You cannot fight for fear of hitting $N. ", FALSE, ch, 0, kala, TO_CHAR);
    }

    if(kala->specials.fighting == vict)
    {
      stop_fighting(kala);
      act("You cannot fight for fear of hitting $N. ", FALSE, vict, 0, kala, TO_CHAR);
    }
  }
  if (!IS_FIGHTING(ch) && !IS_DESTROYING(ch))
    set_fighting(ch, vict);
  if (!IS_FIGHTING(vict) && !IS_DESTROYING(vict))
    set_fighting(vict, ch);

  act("You lock $N's arms with your own holding $M tightly. ", FALSE, ch, 0, vict, TO_CHAR);
  act("$n grabs you holding your arms tightly together.", FALSE, ch, 0, vict, TO_VICT);
  act("$n locks $N's arms with $s own holding $M tightly.", FALSE, ch, 0, vict, TO_NOTVICT);

  //ch
  //   bzero(&af, sizeof(af));
  //      af.type = SKILL_GRAPPLE;
  //         af.bitvector5 = AFF5_GRAPPLER;
  //            af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
  //               af.duration = 7 * PULSE_VIOLENCE;
  //                  affect_to_char_with_messages(ch, &af,
  //                            "",
  //                                "");
  //victim
  //                                     bzero(&af, sizeof(af));
  //                                        af.type = SKILL_GRAPPLE;
  //                                           af.bitvector5 = AFF5_GRAPPLED;
  //                                              af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
  //                                                 af.duration = 3 * PULSE_VIOLENCE;
  //                                                    affect_to_char_with_messages(vict, &af,
  //                                                              "&+rYou manage to break the arm lock!&n",
  //                                                                  "&+r$n manages to get the arm lock!&n");
  //
  //                                                                     add_event(event_grapple,
  //                                                                            GET_LEVEL(ch) < 51 ? 2 * PULSE_VIOLENCE : PULSE_VIOLENCE,
  //                                                                                   ch, vict, 0, 0, 0, 0);
  //                                                                                       }
  //                                                                                            else {
  //                                                                                                 act("You charge $N, but $E nimbly rolls out of the way.", FALSE, ch, 0, vict, TO_CHAR);
  //                                                                                                            act("$n lunges forward, but you roll out of the way", FALSE, ch, 0, vict, TO_VICT);
  //                                                                                                                 act("$n lunges forward, but $N rolls out of the way.", FALSE, ch, 0, vict, TO_NOTVICT);
  //                                                                                                                          CharWait(ch, PULSE_VIOLENCE * 1);
  //                                                                                                                                     SET_POS(ch, POS_SITTING + GET_STAT(ch));
  //                                                                                                                                              }
  //
  //
  //                                                                                                                                              }
  //
  //


#if 0
#define SHAPECHANGE_MOB_LEVEL_DIFF 6

bool shapechange_canResearch(P_char ch, P_char target)
{

  if (IS_PC(target))
  {
    send_to_char
      ("This creature's erratic behavior forces you to abandon your study.\r\n",
       ch);
    return FALSE;
  }

  if (GET_LEVEL(ch) < shapechange_levelNeeded(target->player.race))
  {
    send_to_char
      ("You can not yet comprehend the workings of the class of creature.\r\n",
       ch);
    return FALSE;
  }

  /* Can research mobs more than SHAPECHANGE_MOB_LEVEL_DIFF levels lower
   * Feel free to tweak this number
   */
  if (GET_LEVEL(target) > GET_LEVEL(ch) - SHAPECHANGE_MOB_LEVEL_DIFF)
  {
    send_to_char
      ("You lack the experience necessary to comphrehend this creature.\r\n",
       ch);
    return FALSE;
  }
  // can't study proc'd mobs
  if (mob_index[GET_RNUM(target)].func.mob)
  {
    send_to_char
      ("There's something special about this creature you just cannot place.\r\n",
       ch);
    return FALSE;
  }

  return TRUE;
}

#define SHAPECHANGE_TIME_BETWEEN_RESEARCH SECS_PER_MUD_DAY

void shapechange_studyShape(P_char ch, struct char_shapechange_data *shape,
                            char *shortDesc)
{
  char     s[MAX_STRING_LENGTH];

  if (shape->lastResearched > time(0) - SHAPECHANGE_TIME_BETWEEN_RESEARCH)
  {
    send_to_char
      ("You do not wish to disturb this creature again so soon.\r\n", ch);
    return;
  }

  sprintf(s, "With all of $s attention, $n observes %s.\r\n", shortDesc);
  act(s, TRUE, ch, NULL, NULL, TO_ROOM);

  /*
   * Make this more level based..
   */
  if (GET_LEVEL(ch) < 30)
  {
    CharWait(ch, 2 * PULSE_VIOLENCE);
  }
  else if ((GET_LEVEL(ch) <= 50) && (GET_LEVEL(ch) >= 30))
  {
    CharWait(ch, PULSE_VIOLENCE);
  }
  else if ((GET_LEVEL(ch) > 50) && (GET_LEVEL(ch) < 56))
  {
    CharWait(ch, PULSE_VIOLENCE / 2);
  }
  else if ((GET_LEVEL(ch) == 56))
  {
    CharWait(ch, PULSE_VIOLENCE / 4);
  }

  if (shape->timesResearched == 0)
  {
    sprintf(s, "You gain a basic understanding of %s&n.\r\n", shortDesc);
    send_to_char(s, ch);
  }
  else if (shape->timesResearched >= 15)
  {
    send_to_char("You gain no further understanding of this animal.\r\n", ch);
    return;
  }
  else
  {
    sprintf(s, "You increase your understanding of %s&n.\r\n", shortDesc);
    send_to_char(s, ch);
  }

  shape->timesResearched++;
  shape->lastResearched = time(0);

  writeShapechangeData(ch);
}

void shapechange_adjustKnownShapesLength(P_char ch)
{
  int      curLen = 0;
  struct char_shapechange_data *curShape, *lastShape;

  lastShape = curShape = ch->only.pc->knownShapes;


  while (curShape != NULL)
  {
    curLen++;
    lastShape = curShape;
    curShape = curShape->next;
  }

  /* You get level / 5 shapes in your memory
   * tweak as necessary
   */
  if (curLen > GET_LEVEL(ch) / 2)
    shapechange_removeShape(ch, lastShape);
}

void shapechange_research(P_char ch, char *mobname)
{
  P_char   target;

  target = get_char_room_vis(ch, mobname);

  if (target != NULL)
  {
    if (shapechange_canResearch(ch, target))
    {

      struct char_shapechange_data *curShape;

      curShape = ch->only.pc->knownShapes;

      while ((curShape != NULL)
             && (curShape->mobVnum != GET_VNUM(target)))
        curShape = curShape->next;

      if (curShape != NULL)
        shapechange_studyShape(ch, curShape, target->player.short_descr);
      else
      {
        CREATE(curShape, struct char_shapechange_data, 1);

        curShape->mobVnum = mob_index[GET_VNUM(target)].virtual_number;
        curShape->timesResearched = 0;
        curShape->lastResearched = 0;
        curShape->lastShapechanged = 0;
        curShape->next = ch->only.pc->knownShapes;
        ch->only.pc->knownShapes = curShape;

        shapechange_adjustKnownShapesLength(ch);

        shapechange_studyShape(ch, curShape, target->player.short_descr);

      }
    }
  }
  else
  {                             /* target == NULL */
    send_to_char("Begin your research by finding such a creature!\r\n", ch);
  }
}
#endif // 0

/*
 * if you want to reenable this, port this to add_event api, wont
 * work otherwise

void shapechange_event(void)
{

  char     chName[MAX_STRING_LENGTH];
  int      room, shapeNum, count, i;
  P_char   ch;
  struct char_shapechange_data *shape;

  sscanf(current_event->target.t_arg, "%s %d %d %d",
         chName, &room, &shapeNum, &count);

  ch = get_char_room(chName, room);
  if (ch == NULL)
    ch = get_char(chName);
  if (ch == NULL)
  {
    logit(LOG_DEBUG, "%s vanished while shapechanging", chName);
    return;
  }

  if (count > 0)
  {
    char     buf[MAX_STRING_LENGTH];

    strcpy(buf, "Changing shape: ");
    for (i = 0; i < count; i++)
      strcat(buf, "*");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    count--;
    sprintf(buf, "%s %d %d %d", chName, room, shapeNum, count);
    AddEvent(EVENT_SPECIAL, PULSE_SHAPECHANGE, TRUE, shapechange_event, buf);
    return;
  }

  shape = shapechange_getShape(ch, shapeNum);
  if (shape == NULL)
    return;

  shape->lastShapechanged = time(0);
  writeShapechangeData(ch);

  shapechange_changeTo(ch, shape->mobVnum);
//
//  if (morph(ch, shape->mobVnum, VIRTUAL)) {
//    send_to_char("You finish changing your form.\r\n", ch);
//    act("$N has become $n.", FALSE,
//        ch->only.pc->switched, 0, ch, TO_ROOM);
//  } else {
//    send_to_char("You suddenly stop reforming!\r\n", ch);
//    act("$N aborts his shapeshifting, and suddenly reverts to his natural form.",
//        FALSE, ch, 0, 0, TO_ROOM);
//  }
}
*/

#if 0
int shapechange_timeToChange(P_char ch, int shapeNum)
{
  /*
   * Based on timesResearched, char level and mob level
   */
  struct char_shapechange_data *shape;
  int      i;
  P_char   mob;
  float    levelRatio;
  const int MAX_SHAPECHANGE_TICKS = 20;
  const int MIN_SHAPECHANGE_TICKS = 2;

  shape = shapechange_getShape(ch, shapeNum);
  if (shape == NULL)
    return FALSE;

  mob = read_mobile(real_mobile0(shape->mobVnum), REAL);

  if (mob == NULL)
  {
    logit(LOG_DEBUG, "Unable to read mob in shapechange_timeToChange");
    return FALSE;
  }

  levelRatio = (float) GET_LEVEL(mob) /
    (float) (GET_LEVEL(ch) - SHAPECHANGE_MOB_LEVEL_DIFF);

  extract_char(mob);

  return (int)
    MAX((levelRatio * MAX_SHAPECHANGE_TICKS - shape->timesResearched),
        MIN_SHAPECHANGE_TICKS);

}
#endif // 0
/*
 * port this to add_event if you want to reenable

void shapechange_changeShape(P_char ch, int shapeNum)
{
  char     eventArgs[MAX_STRING_LENGTH];
  int      count;
  struct char_shapechange_data *shape;

  shape = shapechange_getShape(ch, shapeNum);
  if (shape == NULL)
    return;

  if (shape->lastShapechanged + TIME_BETWEEN_SHAPECHANGES > time(0))
  {
    send_to_char
      ("Your mind needs more rest before changing into that form again.\r\n",
       ch);
    return;
  }

  count = shapechange_timeToChange(ch, shapeNum);;

  sprintf(eventArgs, "%s %d %d %d",
          GET_NAME(ch), ch->in_room, shapeNum, count);

  send_to_char("You begin altering your form.\r\n", ch);
  act
    ("$n's body distorts, twisting and writhing, as $e begins to change form.",
     TRUE, ch, NULL, NULL, TO_ROOM);

  if (GET_POS(ch) > POS_PRONE)
  {
    send_to_char
      ("You fall to the ground as your limbs no longer support you!\r\n", ch);
    act("$n falls to the ground as $s limbs no longer support $m.", TRUE, ch,
        NULL, NULL, TO_ROOM);
    SET_POS(ch, GET_STAT(ch) + POS_PRONE);
  }

  CharWait(ch, (count) * PULSE_SHAPECHANGE);
  // was count+1, count is min 2, max 20..
  AddEvent(EVENT_SPECIAL, PULSE_SHAPECHANGE, TRUE, shapechange_event,
           eventArgs);
}

void shapechange_stopChanging(P_char ch)
{
  clear_char_events(ch, EVENT_SPECIAL, (void*)shapechange_event);
}
*/

