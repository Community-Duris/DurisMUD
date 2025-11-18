#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "assocs.h"
#include "prototypes.h"
#include "structs.h"
#include "config.h"
#include "utils.h"
#include "spells.h"
#include "ships.h"
#include "sql.h"
#define MAX_FRAG_SIZE    10     /* max size of high/low lists */

extern const struct class_names class_names_table[];
extern const struct race_names race_names_table[];
extern P_char misfire_check(P_char ch, P_char spell_target, int flag);

extern P_room world;
extern const racewar_struct racewar_color[MAX_RACEWAR+2];

extern void get_level_cap_info( long *max_frags, int *racewar, int *level, time_t *next_update );
extern void get_level_cap( int *max_level, int *racewar );
extern int sql_level_cap( int racewar_side );

/*
 * fragWorthy - is ch worthy of gaining a frag and victim worthy of losing
 *              one?
 */

int fragWorthy(P_char ch, P_char victim)
{
  int      racew;
  P_char tch;
#if 0
  if (!IS_PC(ch) || !IS_PC(victim))
    return FALSE;
#endif

 
  if (IS_NPC(victim))
    return FALSE;

  if (IS_NPC(ch))
    if (ch->following && IS_PC(ch->following))
      ch = ch->following;
    else
      return FALSE;

  
  if ((GET_LEVEL(ch) > 56) || (GET_LEVEL(victim) > 56))
    return FALSE;

  if (CHAR_IN_ARENA(ch) || CHAR_IN_ARENA(victim))
    return FALSE;

  /* killing people under 20 - no frag.  killing people more than 10
     levels under you - no frag. */

  /* non-floating point floating point system, dig it?  100 frags = 1.00 */
/* Commenting this out for the 2017 wipe.
  if ((ch->only.pc->frags > 2000) && (GET_LEVEL(victim) < 40))
    return FALSE;
*/

  if (GET_LEVEL(victim) < 20)
    return FALSE;

  racew = (opposite_racewar(ch, victim) /* || (IS_ILLITHID(ch) && !IS_ILLITHID(victim)) || 
                                           (IS_DISGUISE(victim) && (EVIL_RACE(victim) != EVIL_RACE(ch))) */
           );

  if (!racew)
    return FALSE;

  /* Kvark adding harder check for frags, connected to missfire. */ 
/*
  misfire_check(ch, victim,
                  DISALLOW_SELF | DISALLOW_BACKRANK);
  
  if(!affected_by_spell(ch, TAG_NOMISFIRE))
  {
      send_to_char("&+WThis kind of frag counts as nothing, go prove your self in a fair fight instead.&n\n", ch);
                return FALSE;
                
  }
*/
  /*
  else
  {
   for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
   {
    if(tch)
     if (tch != ch) 
      if(opposite_racewar(victim, tch) && !IS_TRUSTED(tch))
       if(!affected_by_spell(tch, TAG_NOMISFIRE)){
          send_to_char("This kind of frag counts as nothing, blame your firends.", tch);  
          return FALSE;          
       }
   }
   
  }
  */

  
  if (victim->only.pc->frags > 500)
    return TRUE;

  if (racew && (victim->only.pc->frags > 200))
    return TRUE;

  if ((GET_LEVEL(victim) + 10) < GET_LEVEL(ch))
    return FALSE;

  return racew;
/*  if (racew) return TRUE;

  if (IS_ILLITHID(ch) && !IS_ILLITHID(victim)) return TRUE;

  return FALSE;*/
}


/*
 * deleteFragEntry
 */

void deleteFragEntry(char names[MAX_FRAG_SIZE][MAX_STRING_LENGTH],
                     int frags[MAX_FRAG_SIZE], int pos)
{
  int      i;

  if (pos >= MAX_FRAG_SIZE)
    return;

  for (i = pos; i < MAX_FRAG_SIZE; i++)
  {
    strcpy(names[i], names[i + 1]);
    frags[i] = frags[i + 1];
  }

  strcpy(names[MAX_FRAG_SIZE - 1], "Nobody");
  frags[MAX_FRAG_SIZE - 1] = 0;
}


/*
 * insertFragEntry
 */

void insertFragEntry(char names[MAX_FRAG_SIZE][MAX_STRING_LENGTH],
                     int frags[MAX_FRAG_SIZE], char *name, int newFrags,
                     int pos)
{
  int      i;

  if (pos >= MAX_FRAG_SIZE)
    return;

  if (pos == (MAX_FRAG_SIZE - 1))
  {
    strcpy(names[pos], name);
    frags[pos] = newFrags;
    return;
  }

  for (i = MAX_FRAG_SIZE - 2; i >= pos; i--)
  {
    strcpy(names[i + 1], names[i]);
    frags[i + 1] = frags[i];
  }

  strcpy(names[pos], name);
  frags[pos] = newFrags;
}


// Shows the frag list.
void do_fraglist(P_char ch, char *arg, int cmd)
{
  FILE    *fragList;
  char     name[MAX_STRING_LENGTH], buf[65536], buf2[2048];
  int      frags;
  char     i;
  float    fragnum = 0;
  char     filename[1024];
  int      cap_level, cap_racewar, cap_others;
  long     cap_frags;
  time_t   cap_timer;
  int      days, hours, mins, secs;

  if( !IS_ALIVE(ch) )
    return;

  if( arg )
  {
    if (strstr("normal", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.normal");
    }
    else if (strstr("goodie", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.goodie");
    }
    else if (strstr("evil", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.evil");
    }
    else if (strstr("undead", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.undead");
    }
    // else if (strstr("death knight", arg))
    // {
      // snprintf(filename, 1024, "Fraglists/fraglist.death_knight");
    // }
    else if (strstr("lich", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.lich");
    }
    else if (strstr("vampire", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.vampire");
    }
    // else if (strstr("shadow beast", arg))
    // {
      // snprintf(filename, 1024, "Fraglists/fraglist.shadow_beast");
    // }
    // else if (strstr("wight", arg))
    // {
      // snprintf(filename, 1024, "Fraglists/fraglist.wight");
    // }
    // else if (strstr("phantom", arg))
    // {
      // snprintf(filename, 1024, "Fraglists/fraglist.phantom");
    // }
    // else if (strstr("shade", arg))
    // {
      // snprintf(filename, 1024, "Fraglists/fraglist.shade");
    // }
    else if (strstr("revenant", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.revenant");
    }
    else if (strstr("illithid", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.squid");
    }
    else if (strstr("human", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.human");
    }
    else if (strstr("barbarian", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.barbarian");
    }
    else if (strstr("drow elf", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.drow_elf");
    }
    else if (strstr("grey elf", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.grey_elf");
    }
    else if (strstr("mountain dwarf", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.mountain_dwarf");
    }
    else if (strstr("duergar dwarf", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.duergar_dwarf");
    }
    else if (strstr("halfling", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.halfling");
    }
    else if (strstr("gnome", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.gnome");
    }
    else if (strstr("storm giant", arg))
    {
      snprintf(filename, 1024, "Fraglists/fraglist.storm_giant");
    }
    else if (strstr("ogre", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.ogre");
    }
    else if (strstr("troll", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.troll");
    }
    else if (strstr("drider", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.drider");
    }
    else if (strstr("half elf", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.half-elf");
    }
    else if (strstr("half-elf", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.half-elf");
    }
    else if (strstr("orc", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.orc");
    }
    else if (strstr("thrikreen", arg) || strstr("thri-kreen", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.thri-kreen");
    }
    else if (strstr("centaur", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.centaur");
    }
    else if (strstr("githyanki", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.githyanki");
    }
    else if (strstr("minotaur", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.minotaur");
    }
    else if (strstr("goblin", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.goblin");
    }
    // else if (strstr("harpy", arg))
    // {
      // snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.harpy");
    // }
    // else if (strstr("gargoyle", arg))
    // {
      // snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.gargoyle");
    // }
    else if (strstr("orog", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.orog");
    }
    else if (strstr("githzerai", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.githzerai");
    }
    else if (strstr("agathinon", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.agathinon");
    }
    else if (strstr("eladrin", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.eladrin");
    }
    else if (strstr("pillithid", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.planetbound_illithid");
    }
    else if (strstr("wood elf", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.wood_elf");
    }
    else if (strstr("kobold", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.kobold");
    }
    else if (strstr("kuo toa", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.kuo_toa");
    }
    else if (strstr("firbolg", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.firbolg");
    }
    else if (strstr("tiefling", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.tiefling");
    }
    else if (strstr("warrior", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.warrior");
    }
    else if (strstr("ranger", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.ranger");
    }
    else if (strstr("paladin", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.paladin");
    }
    else if (strstr("psionicist", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.psionicist");
    }
    else if (strstr("anti-paladin", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.anti-paladin");
    }
    else if (strstr("cleric", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.cleric");
    }
    else if (strstr("monk", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.monk");
    }
    else if (strstr("unholy-piper", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.unholy-piper");
    }
    else if (strstr("shaman", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.shaman");
    }
    else if (strstr("sorcerer", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.sorcerer");
    }
    else if (strstr("necromancer", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.necromancer");
    }
    else if (strstr("conjurer", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.conjurer");
    }
    else if (strstr("summoner", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.summoner");
    }
    else if (strstr("rogue", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.rogue");
    }
    else if (strstr("assassin", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.assassin");
    }
    else if (strstr("mercenary", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.mercenary");
    }
    else if (strstr("bard", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.bard");
    }
    else if (strstr("thief", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.thief");
    }
    // else if (strstr("warlock", arg))
    // {
      // snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.warlock");
    // }
    else if (strstr("druid", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.druid");
    }
    else if (strstr("blighter", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.blighter");
    }
    else if (strstr("reaver", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.reaver");
    }
    else if (strstr("illusionist", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.illusionist");
    }
    // else if (strstr("alchemist", arg))
    // {
      // snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.alchemist");
    // }
    else if (strstr("berserker", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.berserker");
    }
    // else if (strstr("mindflayer", arg))
    // {
      // snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.mindflayer");
    // }
    else if (strstr("dreadlord", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.dreadlord");
    }
    else if (strstr("ethermancer", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.ethermancer");
    }
    else if (strstr("avenger", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.avenger");
    }
    else if (strstr("theurgist", arg))
    {
      snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.theurgist");
    }
    else if (strstr("ship", arg))
    {
      update_shipfrags();
      display_shipfrags(ch);
      return;
    }
    else if (strstr("guild", arg))
    {
      show_guild_frags(ch);
      return;
    }
    else
    {
      send_to_char("Valid fraglists exist by race, class, undead/evil/good, and overall (no argument).\r\n", ch);
      return;
    }
  }
  else
    snprintf(filename, MAX_STRING_LENGTH, "Fraglists/fraglist.normal");


  if( !(fragList = fopen(filename, "rt")) )
  {
    snprintf(name, MAX_STRING_LENGTH, "Couldn't open fraglist: %s\r\n", filename);
    send_to_char( "&+RError: Couldn't open fraglist.&n\n", ch );
    logit(LOG_DEBUG, name);
    return;
  }

//  snprintf(buf, MAX_STRING_LENGTH, "\r\n&+WTop Fraggers\r\n\r\n");
  get_level_cap_info( &cap_frags, &cap_racewar, &cap_level, &cap_timer );
  cap_others = sql_level_cap( (cap_racewar == RACEWAR_GOOD) ? RACEWAR_EVIL : RACEWAR_GOOD );
  cap_timer -= time(NULL);

  if( cap_timer <= 0 )
  {
    secs = mins = hours = days = 0;
  }
  else
  {
    secs = cap_timer % 60;
    cap_timer /= 60;
    mins = cap_timer % 60;
    cap_timer /= 60;
    hours = cap_timer % 24;
    cap_timer /= 24;
    days = cap_timer;
  }

  snprintf(buf, MAX_STRING_LENGTH, "&+YFrag Level Cap:&+w %d - &+%c%s&n, &+w%d&N - Others, &+YTop Frag Amount: &+w%d.%02d\n&+YTimer:&+w %02d:%02d:%02d:%02d &+YFrags needed:&+w %.2f&n\n\n&+WTop Fraggers\n\n",
    cap_level, racewar_color[cap_racewar].color, racewar_color[cap_racewar].name, cap_others, (int)(cap_frags / 100),
    (int)(cap_frags % 100), days, hours, mins, secs, LEVEL_TO_FRAGS(cap_level + 1) );

  for (i = 0; i < MAX_FRAG_SIZE; i++)
  {
    fscanf(fragList, "%s %d\n", name, &frags);
    name[0] = toupper(name[0]);
    fragnum = frags;
    fragnum /= 100.0;

    //snprintf(buf2, MAX_STRING_LENGTH, "   &+Y%-30s             &+R%6d.%02d\r\n",
    //        name, frags/100, frags %100);
    snprintf(buf2, MAX_STRING_LENGTH, "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
    strcat(buf, buf2);
  }

  strcat(buf, "\r\n\r\n&+LLowest Fraggers\r\n\r\n");

  for (i = 0; i < MAX_FRAG_SIZE; i++)
  {
    fscanf(fragList, "%s %d\n", name, &frags);
    name[0] = toupper(name[0]);
    fragnum = frags;
    fragnum /= 100.0;

    //snprintf(buf2, MAX_STRING_LENGTH, "   &+Y%-30s             &+R%6d.%02d\r\n",
    snprintf(buf2, MAX_STRING_LENGTH, "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
    //name, frags / 100, (-1 * (frags % 100)));
    strcat(buf, buf2);
  }

  fclose(fragList);

  strcat(buf, "\r\n");

  page_string(ch->desc, buf, 1);
}


/*
 * checkFragList_internal
 */

void checkFragList_internal(P_char ch, char type)
{
  FILE    *fraglist;
  char     highPlayerName[MAX_FRAG_SIZE][MAX_STRING_LENGTH],
    lowPlayerName[MAX_FRAG_SIZE][MAX_STRING_LENGTH], change = FALSE;
  int      highFrags[MAX_FRAG_SIZE], lowFrags[MAX_FRAG_SIZE], pfrags, i;
  char     fraglist_file[1024];
  char     buffer[1024], *ptr;

#if 0
  return;
#endif

  if (!ch)
    return;

  if (type == FRAGLIST_NORMAL)
    snprintf(fraglist_file, 1024, "Fraglists/fraglist.normal");
  else if (type == FRAGLIST_RACEWAR)
  {
    if (IS_ILLITHID(ch))
      snprintf(fraglist_file, 1024, "Fraglists/fraglist.squid");
    else if (GOOD_RACE(ch))
      snprintf(fraglist_file, 1024, "Fraglists/fraglist.goodie");
    else if (IS_RACEWAR_UNDEAD(ch))
      snprintf(fraglist_file, 1024, "Fraglists/fraglist.undead");
    else if (EVIL_RACE(ch))
      snprintf(fraglist_file, 1024, "Fraglists/fraglist.evil");


  }
  else if (type == FRAGLIST_RACE)
  {
    snprintf(fraglist_file, 1024, "Fraglists/fraglist.%s",
            race_names_table[(int) GET_RACE(ch)].normal);
  }
  else if (type == FRAGLIST_CLASS)
  {
    snprintf(fraglist_file, 1024, "Fraglists/fraglist.%s",
            class_names_table[(int) flag2idx(ch->player.m_class)].normal);
  }
  else
  {
    send_to_char("Coding error in fraglist, bug a coder.\r\n", ch);
    return;

  }

  ptr = fraglist_file;
  for (ptr = fraglist_file; *ptr != '\0'; ptr++)
  {
    *ptr = LOWER(*ptr);
    if (*ptr == ' ')
      *ptr = '_';
  }

  fraglist_file[0] = 'F';

  fraglist = fopen(fraglist_file, "rt");
  if (!fraglist)
  {
    snprintf(buffer, 1024, "cp lib/etc/fraglist.empty %s", fraglist_file);
    system(buffer);
    snprintf(buffer, 1024, "Fraglist didn't exist, so created empty one: %s\r\n", fraglist_file);
    logit(LOG_DEBUG, buffer);
    fraglist = fopen(fraglist_file, "rt");
  }

  if (!fraglist)
  {
    snprintf(buffer, 1024, "Couldn't open fraglist: %s\r\n", fraglist_file);
    logit(LOG_DEBUG, buffer);
    return;
  }
  char     tmp_buf[MAX_STRING_LENGTH];
  pfrags = ch->only.pc->frags;

  /* read ten highest */

  for (i = 0; i < MAX_FRAG_SIZE; i++)
  {
    if (feof(fraglist))
    {
      
			logit(LOG_DEBUG,"error: frag list terminated prematurely.");
      fclose(fraglist);
      return;
    }

    fscanf(fraglist, "%s %d\n", highPlayerName[i], &highFrags[i]);
    if (type == FRAGLIST_NORMAL)
    {
    if(isname(highPlayerName[0], GET_NAME(ch)))
       {
        //spell_biofeedback(60, ch, 0, 0, ch, 0);
        SET_BIT(ch->specials.act3, PLR3_FRAGLEAD);
	}
    if(!isname(highPlayerName[0], GET_NAME(ch)))
      {
        REMOVE_BIT(ch->specials.act3, PLR3_FRAGLEAD);
      }

     //debug("&+gKick&n (%s) chance (%d) at (%s).", GET_NAME(ch), highPlayerName[0], highPlayerName[i]);

    //highPlayerName[1]->player.title = "&+WVICTORY!&n";
    }
    
  }

/* read ten lowest */

  for (i = 0; i < MAX_FRAG_SIZE; i++)
  {
    if (feof(fraglist))
    {
      
			logit(LOG_DEBUG, "error: frag list terminated prematurely.");
      fclose(fraglist);
      return;
    }

    fscanf(fraglist, "%s %d\n", lowPlayerName[i], &lowFrags[i]);

     if (type == FRAGLIST_NORMAL)
     {
     if(isname(lowPlayerName[0], GET_NAME(ch)))
       {
        //spell_biofeedback(60, ch, 0, 0, ch, 0);
        SET_BIT(ch->specials.act3, PLR3_FRAGLOW);
	}
    if(!isname(lowPlayerName[0], GET_NAME(ch)))
      {
        REMOVE_BIT(ch->specials.act3, PLR3_FRAGLOW);
      }
     //debug("&+gKick&n (%s) chance (%d) at (%s).", GET_NAME(ch), lowPlayerName[1], lowPlayerName[i]);
     }
  }

  fclose(fraglist);

  /* check if player already has an entry and is higher than somebody else
     (including his previous entry) - if so, delete it.  if they end up at
     the end of the list (higher than nobody after deleted), stick em there
     here */

  for (i = 0; i < MAX_FRAG_SIZE; i++)
  {
    // check for dupe entry - just delete it if it exists.  let's see what
    // happens, shall we?

    if (!str_cmp(ch->player.name, highPlayerName[i]))
    {
      deleteFragEntry(highPlayerName, highFrags, i);

      break;
    }
  }

  /* see if player has beaten anybody currently on the list */

  for (i = 0; (i < MAX_FRAG_SIZE); i++)
  {
    if (pfrags > highFrags[i])
    {
      if (!IS_TRUSTED(ch))
        insertFragEntry(highPlayerName, highFrags, ch->player.name, pfrags,
                        i);

      change = TRUE;
      break;
    }
  }

  /* do same shit we did above for lowest - checkjing for dupe names */

  /* see if player has made it to the lowest ten!  yay */

  /* check if player already has an entry and is lower than somebody else
     (including his previous entry) - if so, delete it.  if they end up at
     the end of the list (lower than nobody after deleted), stick em there
     here */

  for (i = 0; i < MAX_FRAG_SIZE; i++)
  {
    // check for dupe entry

    if (!str_cmp(ch->player.name, lowPlayerName[i]))
    {
      deleteFragEntry(lowPlayerName, lowFrags, i);

      break;
    }
  }

  /* see if player is lower than anybody currently on the list */

  for (i = 0; (i < MAX_FRAG_SIZE); i++)
  {
    if (pfrags < lowFrags[i])
    {
      if (!IS_TRUSTED(ch))
        insertFragEntry(lowPlayerName, lowFrags, ch->player.name, pfrags, i);

      change = TRUE;
      break;
    }
  }


  /* if changes, write em out */

  if (change)
  {
    fraglist = fopen(fraglist_file, "wt");
    if (!fraglist)
    {
			logit(LOG_DEBUG, "error: couldn't open fraglist for writing.");
      return;
    }

    for (i = 0; i < MAX_FRAG_SIZE; i++)
    {
      fprintf(fraglist, "%s %d\n", highPlayerName[i], highFrags[i]);
    }

    for (i = 0; i < MAX_FRAG_SIZE; i++)
    {
      fprintf(fraglist, "%s %d\n", lowPlayerName[i], lowFrags[i]);
    }

    fclose(fraglist);
  }
}

void checkFragList(P_char ch)
{
  if (!ch)
    return;

  checkFragList_internal(ch, FRAGLIST_NORMAL);
  checkFragList_internal(ch, FRAGLIST_CLASS);
  checkFragList_internal(ch, FRAGLIST_RACE);
  checkFragList_internal(ch, FRAGLIST_RACEWAR);
  return;
}
