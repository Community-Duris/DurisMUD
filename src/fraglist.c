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
#define MAX_FRAG_SIZE    10     /* max size of high/low lists */
extern const struct class_names class_names_table[];
extern const struct race_names race_names_table[];
extern P_char misfire_check(P_char ch, P_char spell_target, int flag);

extern P_room world;

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

  if ((ch->only.pc->frags > 2000) && (GET_LEVEL(victim) < 40))
    return FALSE;


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


/*
 * displayFragList
 */

void displayFragList(P_char ch, char *arg, int cmd)
{
  FILE    *fragList;
  char     name[MAX_STRING_LENGTH], buf[65536], buf2[2048];
  int      frags;
  char     i;
  float    fragnum = 0;
  char     filename[1024];

  if (!ch)
    return;

//  sprintf(filename, "Fraglists/fraglist.normal");

  if (arg)
  {
    if (strstr("normal", arg))
    {
      sprintf(filename, "Fraglists/fraglist.normal");
    }
    else if (strstr("goodie", arg))
    {
      sprintf(filename, "Fraglists/fraglist.goodie");
    }
    else if (strstr("evil", arg))
    {
      sprintf(filename, "Fraglists/fraglist.evil");
    }
    else if (strstr("undead", arg))
    {
      sprintf(filename, "Fraglists/fraglist.undead");
    }
    // else if (strstr("death knight", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.death_knight");
    // }
    else if (strstr("lich", arg))
    {
      sprintf(filename, "Fraglists/fraglist.lich");
    }
    else if (strstr("vampire", arg))
    {
      sprintf(filename, "Fraglists/fraglist.vampire");
    }
    // else if (strstr("shadow beast", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.shadow_beast");
    // }
    // else if (strstr("wight", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.wight");
    // }
    // else if (strstr("phantom", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.phantom");
    // }
    // else if (strstr("shade", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.shade");
    // }
    else if (strstr("revenant", arg))
    {
      sprintf(filename, "Fraglists/fraglist.revenant");
    }
    else if (strstr("illithid", arg))
    {
      sprintf(filename, "Fraglists/fraglist.squid");
    }
    else if (strstr("human", arg))
    {
      sprintf(filename, "Fraglists/fraglist.human");
    }
    else if (strstr("barbarian", arg))
    {
      sprintf(filename, "Fraglists/fraglist.barbarian");
    }
    else if (strstr("drow elf", arg))
    {
      sprintf(filename, "Fraglists/fraglist.drow_elf");
    }
    else if (strstr("grey elf", arg))
    {
      sprintf(filename, "Fraglists/fraglist.grey_elf");
    }
    else if (strstr("mountain dwarf", arg))
    {
      sprintf(filename, "Fraglists/fraglist.mountain_dwarf");
    }
    else if (strstr("duergar dwarf", arg))
    {
      sprintf(filename, "Fraglists/fraglist.duergar_dwarf");
    }
    else if (strstr("halfling", arg))
    {
      sprintf(filename, "Fraglists/fraglist.halfling");
    }
    else if (strstr("gnome", arg))
    {
      sprintf(filename, "Fraglists/fraglist.gnome");
    }
    else if (strstr("storm giant", arg))
    {
      sprintf(filename, "Fraglists/fraglist.storm_giant");
    }
    else if (strstr("ogre", arg))
    {
      sprintf(filename, "Fraglists/fraglist.ogre");
    }
    else if (strstr("troll", arg))
    {
      sprintf(filename, "Fraglists/fraglist.troll");
    }
    else if (strstr("drider", arg))
    {
      sprintf(filename, "Fraglists/fraglist.drider");
    }
    else if (strstr("half elf", arg))
    {
      sprintf(filename, "Fraglists/fraglist.half-elf");
    }
    else if (strstr("half-elf", arg))
    {
      sprintf(filename, "Fraglists/fraglist.half-elf");
    }
    else if (strstr("orc", arg))
    {
      sprintf(filename, "Fraglists/fraglist.orc");
    }
    else if (strstr("thrikreen", arg) || strstr("thri-kreen", arg))
    {
      sprintf(filename, "Fraglists/fraglist.thri-kreen");
    }
    else if (strstr("centaur", arg))
    {
      sprintf(filename, "Fraglists/fraglist.centaur");
    }
    else if (strstr("githyanki", arg))
    {
      sprintf(filename, "Fraglists/fraglist.githyanki");
    }
    else if (strstr("minotaur", arg))
    {
      sprintf(filename, "Fraglists/fraglist.minotaur");
    }
    else if (strstr("goblin", arg))
    {
      sprintf(filename, "Fraglists/fraglist.goblin");
    }
    // else if (strstr("harpy", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.harpy");
    // }
    // else if (strstr("gargoyle", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.gargoyle");
    // }
    else if (strstr("orog", arg))
    {
      sprintf(filename, "Fraglists/fraglist.orog");
    }
    else if (strstr("githzerai", arg))
    {
      sprintf(filename, "Fraglists/fraglist.githzerai");
    }
    else if (strstr("agathinon", arg))
    {
      sprintf(filename, "Fraglists/fraglist.agathinon");
    }
    else if (strstr("eladrin", arg))
    {
      sprintf(filename, "Fraglists/fraglist.eladrin");
    }
    else if (strstr("pillithid", arg))
    {
      sprintf(filename, "Fraglists/fraglist.planetbound_illithid");
    }
    else if (strstr("wood elf", arg))
    {
      sprintf(filename, "Fraglists/fraglist.wood_elf");
    }
    else if (strstr("kobold", arg))
    {
      sprintf(filename, "Fraglists/fraglist.kobold");
    }
    else if (strstr("kuo toa", arg))
    {
      sprintf(filename, "Fraglists/fraglist.kuo_toa");
    }
    else if (strstr("firbolg", arg))
    {
      sprintf(filename, "Fraglists/fraglist.firbolg");
    }
    else if (strstr("warrior", arg))
    {
      sprintf(filename, "Fraglists/fraglist.warrior");
    }
    else if (strstr("ranger", arg))
    {
      sprintf(filename, "Fraglists/fraglist.ranger");
    }
    else if (strstr("paladin", arg))
    {
      sprintf(filename, "Fraglists/fraglist.paladin");
    }
    else if (strstr("psionicist", arg))
    {
      sprintf(filename, "Fraglists/fraglist.psionicist");
    }
    else if (strstr("anti-paladin", arg))
    {
      sprintf(filename, "Fraglists/fraglist.anti-paladin");
    }
    else if (strstr("cleric", arg))
    {
      sprintf(filename, "Fraglists/fraglist.cleric");
    }
    else if (strstr("monk", arg))
    {
      sprintf(filename, "Fraglists/fraglist.monk");
    }
    else if (strstr("unholy-piper", arg))
    {
      sprintf(filename, "Fraglists/fraglist.unholy-piper");
    }
    else if (strstr("shaman", arg))
    {
      sprintf(filename, "Fraglists/fraglist.shaman");
    }
    else if (strstr("sorcerer", arg))
    {
      sprintf(filename, "Fraglists/fraglist.sorcerer");
    }
    else if (strstr("necromancer", arg))
    {
      sprintf(filename, "Fraglists/fraglist.necromancer");
    }
    else if (strstr("conjurer", arg))
    {
      sprintf(filename, "Fraglists/fraglist.conjurer");
    }
    else if (strstr("summoner", arg))
    {
      sprintf(filename, "Fraglists/fraglist.summoner");
    }
    else if (strstr("rogue", arg))
    {
      sprintf(filename, "Fraglists/fraglist.rogue");
    }
    else if (strstr("assassin", arg))
    {
      sprintf(filename, "Fraglists/fraglist.assassin");
    }
    else if (strstr("mercenary", arg))
    {
      sprintf(filename, "Fraglists/fraglist.mercenary");
    }
    else if (strstr("bard", arg))
    {
      sprintf(filename, "Fraglists/fraglist.bard");
    }
    else if (strstr("thief", arg))
    {
      sprintf(filename, "Fraglists/fraglist.thief");
    }
    // else if (strstr("warlock", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.warlock");
    // }
    else if (strstr("druid", arg))
    {
      sprintf(filename, "Fraglists/fraglist.druid");
    }
    else if (strstr("blighter", arg))
    {
      sprintf(filename, "Fraglists/fraglist.blighter");
    }
    else if (strstr("reaver", arg))
    {
      sprintf(filename, "Fraglists/fraglist.reaver");
    }
    else if (strstr("illusionist", arg))
    {
      sprintf(filename, "Fraglists/fraglist.illusionist");
    }
    // else if (strstr("alchemist", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.alchemist");
    // }
    else if (strstr("berserker", arg))
    {
      sprintf(filename, "Fraglists/fraglist.berserker");
    }
    // else if (strstr("mindflayer", arg))
    // {
      // sprintf(filename, "Fraglists/fraglist.mindflayer");
    // }
    else if (strstr("dreadlord", arg))
    {
      sprintf(filename, "Fraglists/fraglist.dreadlord");
    }
    else if (strstr("ethermancer", arg))
    {
      sprintf(filename, "Fraglists/fraglist.ethermancer");
    }
    else if (strstr("avenger", arg))
    {
      sprintf(filename, "Fraglists/fraglist.avenger");
    }
    else if (strstr("theurgist", arg))
    {
      sprintf(filename, "Fraglists/fraglist.theurgist");
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
      send_to_char
        ("Valid fraglists exist by race, class, undead/evil/good, and overall (no argument).\r\n",
         ch);
      return;
    }
  }
  else
    sprintf(filename, "Fraglists/fraglist.normal");


  if (!(fragList = fopen(filename, "rt")))
  {
    sprintf(name, "Couldn't open fraglist: %s\r\n", filename);
    logit(LOG_DEBUG, name);
    return;
  }

  strcpy(buf, "\r\n&+WTop Fraggers\r\n\r\n");

  for (i = 0; i < MAX_FRAG_SIZE; i++)
  {
    fscanf(fragList, "%s %d\n", name, &frags);
    name[0] = toupper(name[0]);
    fragnum = frags;
    fragnum /= 100.0;

    //sprintf(buf2, "   &+Y%-30s             &+R%6d.%02d\r\n",
    //        name, frags/100, frags %100);
    sprintf(buf2, "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
    strcat(buf, buf2);
  }

  strcat(buf, "\r\n\r\n&+LLowest Fraggers\r\n\r\n");

  for (i = 0; i < MAX_FRAG_SIZE; i++)
  {
    fscanf(fragList, "%s %d\n", name, &frags);
    name[0] = toupper(name[0]);
    fragnum = frags;
    fragnum /= 100.0;

    //sprintf(buf2, "   &+Y%-30s             &+R%6d.%02d\r\n",
    sprintf(buf2, "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
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
    sprintf(fraglist_file, "Fraglists/fraglist.normal");
  else if (type == FRAGLIST_RACEWAR)
  {
    if (IS_ILLITHID(ch))
      sprintf(fraglist_file, "Fraglists/fraglist.squid");
    else if (GOOD_RACE(ch))
      sprintf(fraglist_file, "Fraglists/fraglist.goodie");
    else if (IS_RACEWAR_UNDEAD(ch))
      sprintf(fraglist_file, "Fraglists/fraglist.undead");
    else if (EVIL_RACE(ch))
      sprintf(fraglist_file, "Fraglists/fraglist.evil");


  }
  else if (type == FRAGLIST_RACE)
  {
    sprintf(fraglist_file, "Fraglists/fraglist.%s",
            race_names_table[(int) GET_RACE(ch)].normal);
  }
  else if (type == FRAGLIST_CLASS)
  {
    sprintf(fraglist_file, "Fraglists/fraglist.%s",
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
    sprintf(buffer, "cp lib/etc/fraglist.empty %s", fraglist_file);
    system(buffer);
    sprintf(buffer, "Fraglist didn't exist, so created empty one: %s\r\n", fraglist_file);
    logit(LOG_DEBUG, buffer);
    fraglist = fopen(fraglist_file, "rt");
  }

  if (!fraglist)
  {
    sprintf(buffer, "Couldn't open fraglist: %s\r\n", fraglist_file);
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
