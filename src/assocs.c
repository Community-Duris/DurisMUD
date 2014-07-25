/************************************************************************
* assocs.c - the procedures used for player associations                *
*                                                                       *
* basically a collection of file manipulation routines                  *
* done 1996 by Krov (Ingo Bojak) for Duris DikuMUD                      *
************************************************************************/


/* INCLUDES for assocs.c */

/* first standard libraries that are needed */
#include <errno.h>
#include <sys/stat.h>
#include <cstring>

/* Duris stuff, extensively used... */
#include "mm.h"
#include "db.h"
#include "sql.h"
#include "prototypes.h"
#include "utils.h"
#include "interp.h"
#include "timers.h"
#include "alliances.h"
#include "assocs.h"
#include "nexus_stones.h"
#include "utility.h"
#include "guildhall.h"
#include "epic.h"

/* EXTERNALS for assoc.c */
extern P_char get_pc_vis(P_char, const char *);
string trim(string const& str, char const* sep_chars);

extern char *guild_frags;
extern P_char character_list;
//extern P_house first_house;
extern P_room world;
extern struct zone_data *zone_table;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;

int valid_assocs = 0;
bool guild_frags_initialized = FALSE;
struct guild_frags guild_frags_data[80];

/* god association command interpreter
   general syntax: supervise <s>ubcommand argumentlist
just the first letter of the subcommand triggers.  */
void do_supervise(P_char god, char *argument, int cmd)
{
  char     first[MAX_INPUT_LENGTH];
  char     second[MAX_INPUT_LENGTH];
  char     third[MAX_INPUT_LENGTH];
  char     fourth[MAX_INPUT_LENGTH];
  char     rest[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  ush_int  asc_number;
  char     sign;
  P_char   victim;
  int      test;

  /* why is this proc called at all? */
  if (!god)
    return;

  /* this is a god command only! */
  if (!IS_TRUSTED(god))
  {
    send_to_char("A mere mortal can't do this!\r\n", god);
    return;
  }
  /* chop - 4 arguments max., further divisions for found */
  half_chop(argument, first, rest);
  half_chop(rest, second, rest);

  /* let's be _really_ unsophisticated and use just the first
     letter of the subcommand - this will make programming easy
     and will actually IMHO improve handling as well */
  sign = LOWER(*first);

  /* without arguments the list of associations is displayed */
  if (!sign)
  {
    asc_list(god);
    return;
  }
  asc_number = 0;
  /* all but the found, help and ban command need as second argument
     the association number, chop up further for found */
  if (sign != 'f' && sign != 'b' && sign != '?' && sign != 'h')
  {
    if (!*second)
    {
      send_to_char("Missing association number.\r\n", god);
      return;
    }
    asc_number = (ush_int) atoi(second);
  }
  test = 1;
  switch (sign)
  {
    /* now we found association, we need leader, bits, name */
  case 'f':
    half_chop(rest, third, fourth);
    if (!*second)
    {
      send_to_char("Missing leader name.\r\n", god);
      return;
    }
    victim = get_char_vis(god, second);
    if (victim == NULL)
    {
      send_to_char("That person doesn't seem to be online.\r\n", god);
      return;
    }
    if (!*third)
    {
      send_to_char("Missing association bits string.\r\n", god);
      return;
    }
    if (!*fourth)
    {
      send_to_char("Missing association name.\r\n", god);
      return;
    }
    test = found_asc(god, victim, third, fourth);
    if (test < 1)
      break;

    wizlog(GET_LEVEL(god),
           "%s made %s founding leader of %s&n. Type string is '%s'.",
           GET_NAME(god), GET_NAME(victim), fourth, third);
    logit(LOG_WIZ,
          "%s made %s founding leader of %s&n. Type string is '%s'.",
          GET_NAME(god), GET_NAME(victim), fourth, third);
    sql_log(god, WIZLOG,
          "Made %s founding leader of %s&n. Type string is '%s'.",
            GET_NAME(victim), fourth, third);    
    break;
    /* delete association, we need the number */
  case 'd':
    test = delete_asc(god, asc_number);
    if (test < 1)
      break;

    wizlog(GET_LEVEL(god), "%s deleted association %u.",
           GET_NAME(god), asc_number);
    logit(LOG_WIZ, "%s deleted association %u.", GET_NAME(god), asc_number);
    sql_log(god, WIZLOG, "Deleted association %u.", asc_number);
    break;
    /* rename the association, number and new name needed */
  case 'n':
    if (!*rest)
    {
      send_to_char("Missing association name.\r\n", god);
      return;
    }
    test = name_asc(god, asc_number, rest);
    if (test < 1)
      break;

    wizlog(GET_LEVEL(god), "%s renamed association %u to %s&n.",
           GET_NAME(god), asc_number, rest);
    logit(LOG_WIZ, "%s renamed association %u to %s&n.",
          GET_NAME(god), asc_number, rest);
    sql_log(god, WIZLOG, "Renamed association %u to %s&n.",
            asc_number, rest);
    break;
    /* change association bits */
  case 't':
    if (!*rest)
    {
      send_to_char("Missing association bits string.\r\n", god);
      return;
    }
    test = type_asc(god, asc_number, rest);
    if (test < 1)
      break;

    wizlog(GET_LEVEL(god), "%s changed type of association %u to '%s'.",
           GET_NAME(god), asc_number, rest);
    logit(LOG_WIZ, "%s changed type of association %u to '%s'.",
          GET_NAME(god), asc_number, rest);
    break;
    /* govern association -> access to mortal commands */
  case 'g':
    test = govern_asc(god, asc_number);
    if (test < 1)
      break;

    if (asc_number)
    {
      wizlog(GET_LEVEL(god), "%s now governs association %u.",
             GET_NAME(god), asc_number);
      logit(LOG_WIZ, "%s now governs association %u.",
            GET_NAME(god), asc_number);
    }
    else
    {
      wizlog(GET_LEVEL(god),
             "%s stops doing mortal association administration.",
             GET_NAME(god));
      logit(LOG_WIZ,
            "%s stops doing mortal association administration.",
            GET_NAME(god));
    }
    break;
    /* update an association */
  case 'u':
    update_asc(god, asc_number, 1);

    wizlog(GET_LEVEL(god), "%s forces an update of association %u.",
           GET_NAME(god), asc_number);
    logit(LOG_WIZ, "%s forces an update of association %u.",
          GET_NAME(god), asc_number);
    break;
    /* ban/unban a mortal from all associations */
  case 'b':
    if (!*second)
    {
      send_to_char("Missing mortal name.\r\n", god);
      return;
    }
    victim = get_char_vis(god, second);
    if (victim == NULL)
    {
      send_to_char("That person doesn't seem to be online.\r\n", god);
      return;
    }
    test = ban_mortal(god, victim);
    if (test < 1)
      break;

    if (test == 1)
    {
      wizlog(GET_LEVEL(god), "%s bans %s from all associations.",
             GET_NAME(god), GET_NAME(victim));
      logit(LOG_WIZ, "%s bans %s from all associations.",
            GET_NAME(god), GET_NAME(victim));
    }
    else
    {
      wizlog(GET_LEVEL(god), "%s lifts the association ban on %s.",
             GET_NAME(god), GET_NAME(victim));
      logit(LOG_WIZ, "%s lifts the association ban on %s.",
            GET_NAME(god), GET_NAME(victim));
    }
    break;
    /* this gives a bit of help... */
  case 's': /* setbit */
      // TODO set bit construction_points or prestige
      half_chop(rest, third, fourth);

      if( !*third )
      {
        send_to_char("Please enter prestige or construction_points.\r\n", god);
        return;
      }
      
      if( !*fourth )
      {
        send_to_char("Please enter a valid number.\r\n", god);
        return;
      }
      
      if( is_abbrev(third, "prestige") )
      {
        int old_prestige = get_assoc_prestige(asc_number);
        
        set_assoc_prestige(asc_number, atoi(fourth));
        send_to_char("Association prestige set.\r\n", god);
        
        wizlog(GET_LEVEL(god), "%s set %s&n prestige from %d to %d.", GET_NAME(god), get_assoc_name(asc_number).c_str(), old_prestige, atoi(fourth));
        logit(LOG_WIZ, "%s set %s&n prestige from %d to %d.", GET_NAME(god), get_assoc_name(asc_number).c_str(), old_prestige, atoi(fourth));
        return;
      }
      else if( is_abbrev(third, "construction_points") )
      {
        int old_cps = get_assoc_cps(asc_number);

        set_assoc_cps(asc_number, atoi(fourth));
        send_to_char("Association construction points set.\r\n", god);
        
        wizlog(GET_LEVEL(god), "%s set %s&n construction points from %d to %d.", GET_NAME(god), get_assoc_name(asc_number).c_str(), old_cps, atoi(fourth));
        logit(LOG_WIZ, "%s set %s&n construction points from %d to %d.", GET_NAME(god), get_assoc_name(asc_number).c_str(), old_cps, atoi(fourth));
        return;
      }
      else
      {
        send_to_char("Please enter prestige or construction_points.\r\n", god);
        return;
      }
      
     break;
  case 'h':
  case '?':
  default:
    strcpy(buf, "\r\n");
    strcat(buf, "&+RUsage:&n supervise <&ns>ubcommand <argumentlist>\r\n");
    strcat(buf, "&+MStandard Guilds and Kingdoms&n\r\n");
    strcat(buf, "&+m============================&n\r\n");
    strcat(buf, "<> - displays list of existing associations\r\n");
    strcat(buf, "<&+Mf&n>ound  <leader_name> <bits_string> <asc_name>\r\n");
    strcat(buf, "<&+Md&n>elete <asc_number>\r\n");
    strcat(buf, "<&+Mn&n>ame   <asc_number> <new_asc_name>\r\n");
    strcat(buf, "<&+Mt&n>ype   <asc_number> <bits_string>\r\n");
    strcat(buf, "<&+Mg&n>overn <asc_number>\r\n");
    strcat(buf, "<&+Mu&n>pdate <asc_number>\r\n");
    strcat(buf, "<&+Ms&n>etbit <asc_number> <prestige|construction_points> <value>\r\n");
    strcat(buf, "<&+Mb&n>an    <mortal_name>\r\n");
    strcat(buf, "&+m============================&n\r\n");
    strcat(buf, "   &+y<bits_string>: c = challenge, h = hide title, s = hide subtitle, combine for both, n = none&n\r\n");
    strcat(buf, "   &+y(i.e. sup f johnny ch The Jumpin' Johnnies)&n\r\n");
    strcat(buf, "\r\n");
//    strcat(buf, "&+YKingdoms Only&n\r\n");
//    strcat(buf, "&+y=============&n\r\n");
//    strcat(buf, "<&+Yk&n>ingdom <primary asc #> <2ndary assoc #>\n\r");
//    strcat(buf, "<&+Yl&n>and asc_number\n\r");
//    strcat(buf, "<&+Ys&n>tatus asc_number\n\r\n\r");
    strcat(buf, "<&+yh&n>elp or <&+y?&n> - this list\r\n\r\n");
    send_to_char(buf, god);
    break;
  }

  return;
}

void do_asclist(P_char god, char *argument, int cmd)
{
  if (!IS_TRUSTED(god))
  {
    send_to_char("This is a god only command!\r\n", god);
    return;
  }

  asc_list(god);
}

/* association list: list the number and name of available associations */
void asc_list(P_char god)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     buf[MAX_STRING_LENGTH];
  ush_int  i, j;


  /* this is a god command only! */
  if (!IS_TRUSTED(god))
  {
    send_to_char("WTF? Get lost!\r\n", god);
    return;
  }
  /* title of list */
  strcpy(buf, " Number : Name of Player Associations\r\n"
         " ------------------------------------\r\n");

  /* check through all association numbers, if there is a
     gap of more than 20 it is assumed that there will be no
     more associations. This is a feature not a bug. No need to
     waste endless time on checking non-existent files if
     the creation procedure automatically fills any gaps */
  for (i = 1, j = 0; i < MAX_ASC; i++)
  {
    sprintf(Gbuf1, "%sasc.%u", ASC_DIR, i);
    f = fopen(Gbuf1, "r");
    if (f)
    {
      strcat(buf, "   ");
      sprintf(Gbuf2, "%s %2u : ", (GET_A_NUM(god) == i) ? "&+Y*&n" : " ", i);
      strcat(buf, Gbuf2);
      fgets(Gbuf2, MAX_STR_NORMAL, f);
      strcat(buf, Gbuf2);
      strcat(buf, "&n\r");
      fclose(f);
      j = 0;
    }
    else
      j++;
    if (j > 20)
      break;
  }
  /* now send to char through pager */
  page_string(god->desc, buf, 1);
  return;
}

/* god founds an association, needed arguments are the char who
   is going to be leader of it, the name of the association
   and a string triggering guild bits, returned is 1
   on success, returns 0 if there are
   too many associations or if leader is already member somewhere
   else, -1 if there was a problem with the file */
int found_asc(P_char god, P_char leader, char *bits, char *asc_name)
{

  FILE    *f;
  char     Gbuf[MAX_STR_NORMAL];
  char     title[MAX_STR_TITLE], *new_title;
  ush_int  i;
  uint     temp, asc_bits;
  struct stat statbuf;

  /* this is a god command only! */
  //5/6/13 - Drannak - modifying to allow mob proc to call this for automated guild creation.
 /* if (!IS_TRUSTED(god))
  {
    send_to_char("Arrrgghh! Out, out,...\r\n", god);
    return (0);
  }*/
  /* is this guy involved somewhere else? */
  if ((bits[0] != 'k') &&
      (GET_A_NUM(leader) || !IS_NO_THANKS(GET_A_BITS(leader))))
  {
    send_to_char("That person is unable to found an association...\r\n", god);
    return (0);
  }
  /* no associations under level MIN_LEVEL */
  if (GET_LEVEL(leader) < MIN_LEVEL)
  {
    send_to_char("That person is too low in level!\r\n", god);
    return (0);
  }
  /* find first free association number */
  sprintf(Gbuf, "%sasc.%u", ASC_DIR, i = 1);
  while ((!stat(Gbuf, &statbuf)) && (i < MAX_ASC))
  {
    i++;
    sprintf(Gbuf, "%sasc.%u", ASC_DIR, i);
  }

  /* too many associations */
  if (i >= MAX_ASC)
  {
    send_to_char("There are too many associations!\r\n", god);
    return (0);
  }
  f = fopen(Gbuf, "w");

  /* problems with creating association file */
  if (!f)
  {
    send_to_char("What's up? Couldn't create the association file!\r\n", god);
    return (-1);
  }
  /* name of association */
  if (strlen(asc_name) >= MAX_STR_ASC)
    asc_name[MAX_STR_ASC - 1] = 0;
  fprintf(f, "%s\n", asc_name);
  /* name of ranks: god to enemy standard names */
  fprintf(f, "%s\n", "Enemy of ");
  fprintf(f, "%s\n", "On parole, ");
  fprintf(f, "%s\n", "Member of ");
  fprintf(f, "%s\n", "Senior of ");
  fprintf(f, "%s\n", "Officer of ");
  fprintf(f, "%s\n", "Deputy of ");
  fprintf(f, "%s\n", "Leader of ");
  fprintf(f, "%s\n", "King of ");

  /* bits string interpreter */
  asc_bits = 0;
  while (LOWER(*bits))
  {
    switch (*bits)
    {
      /* challenge allowed */
    case 'c':
      SET_CHALL(asc_bits);
      break;
    case 'h':
      SET_HIDETITLE(asc_bits);
      break;
    case 's':
      SET_HIDESUBTITLE(asc_bits);
      break;
    case 'n':
    default:
      asc_bits = 0;
      break;
    }
    bits++;
  }

  /* now write the guild bits, protect player info */
  SET_M_BITS(asc_bits, A_P_MASK, 0);
  fprintf(f, "%u\n", asc_bits);
  /* cash of guild: platin, gold, silver, copper */
  fprintf(f, "%i %i %i %i\n", 0, 0, 0, 0);

  /* now for the members, association starts off with leader only */
  /* name, bits including rank, debt in platin,gold,silver,copper */
  temp = asc_bits;
  SET_MEMBER(temp);
  SET_LEADER(temp);
  fprintf(f, "%s %u %i %i %i %i\n", GET_NAME(leader), temp, 0, 0, 0, 0);

  fclose(f);

  /* setup the leader */
  GET_A_NUM(leader) = i;
  GET_A_BITS(leader) = temp;

  /* construct and add new title */
 if (!(GET_A_BITS(leader) & A_HIDESUBTITLE))
    strcpy(title, "Leader of ");
  strcat(title, asc_name);
  new_title = title_member(leader, title);
  if (new_title)
  {
    strcpy(Gbuf, "You are ");
    strcat(Gbuf, new_title);
    strcat(Gbuf, "!\r\n");
    send_to_char(Gbuf, leader);
  }
  
#ifndef __NO_MYSQL__
  char sql_buff[MAX_STRING_LENGTH];
  mysql_real_escape_string(DB, sql_buff, asc_name, strlen(asc_name)); 
  qry("REPLACE INTO associations (id, name, active) VALUES (%d, '%s', 1)", i, sql_buff);
#endif
  
  send_to_char("Ok, new association is set up.\r\n", god);
  return (1);
}

/* god deletes association, association file with number asc_number
   is deleted and all online members are notified by updating them
   1 returned on success, 0 on failure */
int delete_asc(P_char god, ush_int asc_number)
{
  char     Gbuf1[MAX_STR_NORMAL];

  /* this is a god command only! */
  if (!IS_TRUSTED(god))
  {
    send_to_char("WTF? How did you even get to do that command?\r\n", god);
    return (0);
  }
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  if (unlink(Gbuf1))
  {
    if (errno == ENOENT)
      send_to_char("That association doesn't seem to exist anymore.\r\n",
                   god);
    else
      send_to_char("Oh geez, you can't access the file?!?\r\n", god);
    return (0);
  }
  // remove alliance if existed
  sever_alliance(asc_number);
  
  send_to_char("You have deleted the association!\r\n", god);
  update_asc(god, asc_number, 1);
  set_assoc_active(asc_number, 0);
  return (1);
}

/* god becomes god of association asc_number,
   that allows access to member commands
   1 returned on success, 0 on failure */
int govern_asc(P_char god, ush_int asc_number)
{
  char     Gbuf1[MAX_STR_NORMAL];
  struct stat statbuf;

  /* this is a god command only! */
  if (!IS_TRUSTED(god))
  {
    send_to_char("You think you are a god, eh?\r\n", god);
    return (0);
  }
  /* asc_number 0 used to reset god */
  if (!asc_number)
  {
    GET_A_NUM(god) = 0;
    SET_NO_THANKS(GET_A_BITS(god));
    SET_GOD(GET_A_BITS(god));
    send_to_char("You are not doing mortal association admin anymore.\r\n",
                 god);
    return (1);
  }
  /* test if association (file) exists */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  if (stat(Gbuf1, &statbuf))
  {
    send_to_char("That association does not exist!\r\n", god);
    return (0);
  }
  /* make the god god */
  GET_A_NUM(god) = asc_number;
  SET_MEMBER(GET_A_BITS(god));
  SET_GOD(GET_A_BITS(god));

  send_to_char("You are now god of that association!\r\n", god);
  return (1);
}

/* god changes association asc_number name, returns 1 if ok, 0 else */
int name_asc(P_char god, ush_int asc_number, char *asc_name)
{
  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];

  /* this is a god command only! */
  if (!IS_TRUSTED(god))
  {
    send_to_char("Heck? Who are you to attempt that?\r\n", god);
    return (0);
  }
  
  if( !is_valid_ansi_with_msg(god, asc_name, FALSE) )
  {
     return 0;
  }
  
  /* find association */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    send_to_char("Couldn't find that association file!\r\n", god);
    return (0);
  }
  /* change the name */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  /* name of association */
  if (strlen(asc_name) >= MAX_STR_ASC)
    asc_name[MAX_STR_ASC - 1] = 0;
  sprintf(buf, "%s\n", asc_name);
  while (fgets(Gbuf2, MAX_STR_NORMAL, f))
    strcat(buf, Gbuf2);
  fclose(f);

  /* write changed buffer */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    send_to_char("Couldn't write the file with changed name!\r\n", god);
    return (0);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  send_to_char("You changed the association name.\r\n", god);
  update_asc(god, asc_number, 1);
  return (1);
}

/* god changes association asc_number bits, returns 1 if ok, 0 else */
int type_asc(P_char god, ush_int asc_number, char *bits)
{
  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1, asc_bits;

  /* this is a god command only! */
  if (!IS_TRUSTED(god))
  {
    send_to_char("Laugh! Sure...\r\n", god);
    return (0);
  }
  /* find association */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    send_to_char("Where is the association file?\r\n", god);
    return (0);
  }
  /* copy up to association bits */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 1; i < 9; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }
 fgets(Gbuf2, MAX_STR_NORMAL, f);

  /* bits string interpreter */
  asc_bits = 0;
  while (LOWER(*bits))
  {
    switch (*bits)
    {
      /* challenge allowed */
    case 'c':
      SET_CHALL(asc_bits);
      break;
    case 'h':
      SET_HIDETITLE(asc_bits);
      break;
    case 's':
      SET_HIDESUBTITLE(asc_bits);
      break;
    case 'n':
    default:
      asc_bits = 0;
      break;
    }
    bits++;
  }

  SET_M_BITS(asc_bits, A_P_MASK, 0);
  sprintf(Gbuf2, "%u\n", asc_bits);
  strcat(buf, Gbuf2);

  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcat(buf, Gbuf2);

  while (fscanf(f, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
                &dummy3, &dummy4, &dummy5) != EOF)
  {
    dummy1 = asc_bits + GET_M_BITS(dummy1, A_P_MASK);
    sprintf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
            dummy3, dummy4, dummy5);
    strcat(buf, Gbuf3);
  }
  fclose(f);

  /* write changed buffer */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    send_to_char("Couldn't write the file with changed bits!\r\n", god);
    return (0);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  send_to_char("You changed the association bits.\r\n", god);
  update_asc(god, asc_number, 0);
  return (1);
}

/* god bans an online mortal from all associations
   the mortals bits are set accordingly and he is deleted from
   his association (if he has one)
   if mortal is banned already this sets him back to no thanks!
   returns 1 upon ban, 2 upon unban, 0 for no success */
int ban_mortal(P_char god, P_char mortal)
{
  int      test;
  ush_int  asc_number, gnum;
  uint     temp, gtemp;
  char     Gbuf[MAX_STR_NORMAL];

  /* this is a god command only! */
  if (!IS_TRUSTED(god))
  {
    send_to_char("Laugh! Sure...\r\n", god);
    return (0);
  }

  /* a call with nobody */
  if (!mortal)
    return (0);

  asc_number = GET_A_NUM(mortal);
  temp = GET_A_BITS(mortal);

  /* tell the mortal what is going on */
  if (!IS_BANNED(temp))
    send_to_char("Guess what? You are banned from all associations now...\r\n", mortal);
  else
    send_to_char("Whew, the association ban on you is lifted!\r\n", mortal);

  /* if the (banned) guy is in an association we set the god up as
     in the govern proc temporarily to be able to use the
     kickout procedure */
  if (IS_MEMBER(temp) && asc_number)
  {
    /* save old bits */
    gnum = GET_A_NUM(god);
    gtemp = GET_A_BITS(god);
    /* make the god god */
    GET_A_NUM(god) = asc_number;
    SET_MEMBER(GET_A_BITS(god));
    SET_GOD(GET_A_BITS(god));
    /* delete the mortal */
    test = kickout_member(god, GET_NAME(mortal));
    if (test > 0)
      send_to_char("The members of your former association spit at you!\r\n", mortal);
    /* god back to old state */
    GET_A_NUM(god) = gnum;
    GET_A_BITS(god) = gtemp;
  }
  /* now set banned/unbanned */
  if (!IS_BANNED(temp))
  {
    temp = 0;
    SET_BANNED(temp);
    strcpy(Gbuf, "&+RAssociation &+YBan&+W!&n");
    title_member(mortal, Gbuf);
    send_to_char("You banned the sucker from all associations!\r\n", god);
    test = 1;
  }
  else
  {
    temp = 0;
    SET_NO_THANKS(temp);
    Gbuf[0] = 0;
    title_member(mortal, Gbuf);
    send_to_char("You have lifted the association ban.\r\n", god);
    test = 2;
  }
  GET_A_BITS(mortal) = temp;
  GET_A_NUM(mortal) = 0;
  return (test);
}

/******************************************************************/
/* used by gods and members */
/******************************************************************/


/* update all members of an association, if caller is god, tell
   him how many have been updated, full decides if title is displayed */
void update_asc(P_char caller, ush_int asc_number, int full)
{
  char     Gbuf[MAX_STR_NORMAL];
  P_char   changed;
  int      i;

  /* update online members of the association */
  for (i = 0, changed = character_list; changed; changed = changed->next)
    if (GET_A_NUM(changed) == asc_number && !IS_TRUSTED(changed))
    {
      update_member(changed, full);
      i++;
    }
  /* Notify a god of what has happened */
  if (IS_TRUSTED(caller))
    if (i)
    {
      sprintf(Gbuf, "%i online member(s) have been updated.\r\n", i);
      send_to_char(Gbuf, caller);
    }
    else
      send_to_char("There were no online members.\r\n", caller);

  return;
}

/* now member commands */

/*
   member association command interpreter
   general syntax: member <s>ubcommand argumentlist
   just the first letter of the subcommand triggers.
*/
void do_society(P_char member, char *argument, int cmd)
{
  char     first[MAX_INPUT_LENGTH];
  char     second[MAX_INPUT_LENGTH];
  char     rest[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  int      test, full, some, pc, gc, sc, cc;
  bool     okay = FALSE;
  char     sign;
  P_char   victim;
  time_t   temp_time;
  byte     temp_nb;
  char    *timestr;
  char     time_left[128];
  // old guildhalls (deprecated)
//  P_house  house = NULL;

  /* why is this proc called at all? */
  if (!member)
    return;

  /* the banned check is done here because it really means _all_ commands */
  if (IS_BANNED(GET_A_BITS(member)))
  {
    temp_time = GET_TIME_LEFT_GUILD(member);
    if (temp_time <= 0)
    {
      send_to_char("You are banned from all associations, so sod off!\r\n",
                   member);
      return;
    }
    if (GET_NB_LEFT_GUILD(member) <= 0)
    {
      send_to_char("You are banned from all associations, so sod off!\r\n",
                   member);
      return;
    }
    temp_nb = GET_NB_LEFT_GUILD(member);
    temp_time += SECS_PER_REAL_DAY * 
      get_property("guild.secede.delay.days", 2);
    if (time(NULL) >= temp_time)
    {
      send_to_char("Look like you can join another guild!\r\n", member);
      SET_BANNED(GET_A_BITS(member));
    }
    else
    {
      timestr = asctime(localtime(&(temp_time)));
      *(timestr + 10) = 0;
      strcpy(time_left, timestr);
      sprintf(buf, "You cannot join another guild until %s\r\n", time_left);
      send_to_char(buf, member);
      return;
    }

  }
  /* test for MIN_LEVEL */
  if (level_check(member) < 1)
  {
    update_member(member, 1);
    return;
  }
  /* chop for first argument */
  half_chop(argument, first, rest);

  /* let's be _really_ unsophisticated and use just the first
     letter of the subcommand - this will make programming easy
     and will actually IMHO improve handling as well */
  sign = LOWER(*first);

  /* without arguments the list of members is displayed */
  if (!sign)
  {
    member_list(member);
    return;
  }
  /* money commands need the money string in rest */
  if (!(sign == 'd' || sign == 'w'))
    /* chop for second argument */
    half_chop(rest, second, rest);

  /* eye to eye commands check */
  if (sign == 'a' || sign == 'e' || sign == 'c' || sign == 't')
  {
    if (!*second)
    {
      send_to_char("Try to find the right person first.\r\n", member);
      return;
    }
    /* "me" check... */
    if (!str_cmp(second, "me"))
      victim = member;
    else
      victim = get_char_room_vis(member, second);
    /* nobody of that name is here... */
    if (!victim)
    {
      send_to_char("Are you hallucinating?\r\n", member);
      return;
    }
  }
  test = full = 0;
  switch (sign)
  {
    /* member applies to victim for membership */
  case 'a':
    apply_asc(member, victim);
    break;
    /* member enrols victim */
  case 'e':
    enrol_asc(member, victim);
    break;
    /* secede from the association */
  case 's':
    if (str_cmp(second, "confirm"))
      send_to_char("Use 'secede confirm' to really leave.\r\n", member);
    else
      secede_asc(member);
    break;
    /* kick victim out */
  case 'k':
    if (!*second)
    {
      send_to_char("You aimlessly kick. Nothing happens.\r\n", member);
      return;
    }
    test = kickout_member(member, second);
    break;
    /* member promotes/demotes victim */
  case 'r':
    if (!*second)
    {
      send_to_char("Wow. You have changed the rank of nobody.\r\n", member);
      return;
    }
    if (!*rest)
    {
      send_to_char("Change the rank by how much?\r\n", member);
      return;
    }
    some = atoi(rest);
    test = rank_member(member, second, some);
    full = 1;
    break;
    /* member changes title of one specific member */
  case 't':
    if (!*second)
    {
      send_to_char("Specify a person's title to change.\r\n", member);
      return;
    }
    if (!*rest)
    {
      send_to_char("Set this person's title to what?\r\n", member);
      return;
    }
    retitle_member(member, victim, rest);
    test = 0;
    break;
    /* member punishes victim */
  case 'p':
    if (!*second)
    {
      send_to_char("Who is going to get it?\r\n", member);
      return;
    }
    test = punish_member(member, second);
    full = 1;
    break;
    /* member changes name of a rank */
  case 'n':
    if (!*second)
    {
      send_to_char("Which rank name do you want to change?\r\n", member);
      return;
    }
    test = name_rank(member, second, rest);
    full = 1;
    break;
    /* member fines somebody */
  case 'f':
    if (!*second)
    {
      send_to_char("Whom do you want to fine?\r\n", member);
      return;
    }
    if (!*rest)
    {
      send_to_char("How much do you want to fine?\r\n", member);
      return;
    }
    str_to_money(rest, &pc, &gc, &sc, &cc);
    test = fine_member(member, second, pc, gc, sc, cc);
    break;
    /* member pays dues */
  case 'd':
    if (!*rest)
    {
      send_to_char("How much do you want to deposit?\r\n", member);
      return;
    }
    str_to_money(rest, &pc, &gc, &sc, &cc);
    deposit_asc(member, pc, gc, sc, cc);
    // dunno why its switching the statitile flag on soc d, but this switches it again back to normal :P
    GET_A_BITS(member) ^= A_STATICTITLE;
    break;
    /* member withdraws money */
  case 'w':
    if (!*rest)
    {
      send_to_char("How much do you want to withdraw?\r\n", member);
      return;
    }
    str_to_money(rest, &pc, &gc, &sc, &cc);
    withdraw_asc(member, pc, gc, sc, cc);
    break;
    /* user toggles his 'allow retitle' flag */
  case 'g':
    GET_A_BITS(member) ^= A_STATICTITLE;

    if (GET_A_BITS(member) & A_STATICTITLE)
    {
      send_to_char
        ("Your title will now not be changed by the leader/automatic rank title updates.\r\n",
         member);
    }
    else
      send_to_char
        ("Your title can now be changed by the leader/automatic rank title updates.\r\n",
         member);

    break;
    /* member challenges victim */
  case 'c':
    challenge(member, victim);
    break;
    /* proscribe an enemy of the association */
  case 'o':
    if (!*second)
    {
      send_to_char("You ostracize nobody. Nobody cares.\r\n", member);
      return;
    }

    send_to_char("No you dont. You actually love him..\r\n", member);
          return;

    ostracize_enemy(member, second);
    break;
    /* set hometown to guild room you are in */
  case 'h':
    {
      if (IS_APPLICANT(GET_A_BITS(member)))
      {
        send_to_char("Try joining a guild first!\r\n", member);
        return;
      }
      if (IS_ILLITHID(member))
      {
        send_to_char("You'd miss the Elder Brain too much...\r\n", member);
        return;
      }

      //
      // Set player's home/orighome to their guildhall's inn room
      //
      Guildhall* gh = Guildhall::find_by_vnum(world[member->in_room].number);

      if(!gh)
      {
        send_to_char("You can only home inside of your guildhall!\r\n", member);
        return;
      }

      if( !IS_MEMBER(GET_A_BITS(member)) || GET_A_NUM(member) != gh->assoc_id )
      {
        send_to_char("You can only home inside of YOUR guildhall!\r\n", member);
        return;
      }

      int inn_vnum = gh->inn_vnum();
      if(!inn_vnum)
      {
        send_to_char("Your guildhall doesn't have an inn!\r\n", member);
        return;
      }

      sprintf(buf, "char %s home %d", J_NAME(member), inn_vnum);
      do_setbit(member, buf, CMD_SETHOME);
      sprintf(buf, "char %s orighome %d", J_NAME(member), inn_vnum);
      do_setbit(member, buf, CMD_SETHOME);

      break;
    }
// old guildhalls (deprecated)
//    house = house_ch_is_in(member);
//    if (!house)
//    {
//      send_to_char("You cannot home here.\r\n", member);
//      return;
//    }
//    if ((GET_A_NUM(member) != house->owner_guild) && (!IS_TRUSTED(member)) &&
//        (house->type == HCONTROL_GUILD))
//    {
//      send_to_char("You don't have the authority to home here.\r\n", member);
//      return;
//    }
//    else if (house->type == HCONTROL_HOUSE &&
//             !strcmp(house->owner, GET_NAME(member)))
//    {
//      send_to_char("This isn't your house bucko.\r\n", member);
//      return;
//    }
/*    sprintf(buf, "char %s home %d", J_NAME(member), house->room_vnums[0]);*/
//    sprintf(buf, "char %s home %d", J_NAME(member),
//            world[member->in_room].number);
//    do_setbit(member, buf, CMD_SETHOME);
/*    sprintf(buf, "char %s orighome %d", J_NAME(member), house->room_vnums[0]);*/
//    sprintf(buf, "char %s orighome %d", J_NAME(member),
//            world[member->in_room].number);
//    do_setbit(member, buf, CMD_SETHOME);
    /* pop up help for gods only */
  case 'l':
    do_soc_ledger(member);
    break;
  case '?':
  default:
      strcpy(buf, "\r\n");
      strcat(buf, "&+YSociety command list&n\r\n");
      strcat(buf, "&+y====================\r\n");
      strcat(buf, "<> - displays list of members and other info\r\n");
      strcat(buf, "<&+Ya&n>pply     <member>    - apply for membership to member\r\n");
      strcat(buf, "                          'apply me' to clear application\r\n");
      strcat(buf, "<&+Ye&n>nrol     <applicant> - enrol an applicant as member\r\n");
      strcat(buf, "<&+Yk&n>ickout   <member>    - delete a member from the books\r\n");
      strcat(buf, "<&+Ys&n>ecede    [confirm]   - leave association\r\n");
      strcat(buf, "<&+Yr&n>ank      <member> <levels> - changes rank by levels\r\n");
      strcat(buf, "<&+Yt&n>itle     <member>    - sets title of specific member\r\n");
      strcat(buf, "to<&+Yg&n>gle    retitle     - toggles title change allowance\r\n");
      strcat(buf, "<&+Yp&n>unish    <member>    - 1st punish=parole, 2nd=enemy\r\n");
      strcat(buf, "<&+Yc&n>hallenge <member>    - challenge member for his rank\r\n");
      strcat(buf, "<&+Yo&n>stracize <name>      - ostracize a non-member\r\n");
      strcat(buf, "<&+Yn&n>ame      <rank_keyword> <new_rank_name> - rename a rank\r\n");
      strcat(buf, "<&+Yf&n>ine      <member> <money> - fine member, 'all' allowed\r\n");
      strcat(buf, "<&+Yd&n>eposit   <money>     - give money to guild\r\n");
      strcat(buf, "<&+Yw&n>ithdraw  <money>     - get money from guild\r\n");
      strcat(buf, "<&+Yh&n>ome                  - set hometown to current room, must be in guild hall\r\n");      /* TASFALEN */
      strcat(buf, "<&+Yl&n>edger                - view the list of money transactions\r\n");      /* TASFALEN */
      strcat(buf, "&+y====================\r\n\r\n");
      strcat(buf, "<&+y?&n> - this help\r\n\r\n");      /* TASFALEN */
      send_to_char(buf, member);
    break;
  }
  /* success -> update */
  if (test == 1)
  {
    if ((victim = get_char_online(second)) != NULL)
      update_member(victim, full);
// Just because a member isn't online doesn't mean there's a bug.
//    else
//      send_to_char( "Couldn't update member.  Tell a God.\n", member );
  }
  if (test > 1)
    update_asc(member, GET_A_NUM(member), full);
  return;
}




/* set char newone up for applying to association of member. if member
   is equal to newone, then char is not interested in applying
   success returns association number or 0, no success returns -1 */

int apply_asc(P_char newone, P_char member)
{
  char     Gbuf[MAX_STR_NORMAL];
  uint     temp, mtemp;
  struct stat statbuf;
  ush_int  mnum;

  if (racewar(newone, member))
  {
    send_to_char("You don't want to associate with scum!\r\n", newone);
    return -1;
  }
  temp = GET_A_BITS(newone);
  /* members can't apply to other association (banned check in do_society) */
  if (IS_MEMBER(temp))
  {
    send_to_char("You are already a member in an association!\r\n", newone);
    return (-1);
  }
  /* no associations under level MIN_LEVEL */
  if (GET_LEVEL(newone) < MIN_LEVEL)
  {
    send_to_char("Associations are for experienced players only!\r\n",
                 newone);
    return (0);
  }
  /* he's an illithid!  ack, pfhfhfhfhfhthfhjhft */
#if 0
  if (IS_ILLITHID(newone) && GET_LEVEL(newone) < 40)
  {
    send_to_char("Hah!  Perhaps if you weren't an octopus on a stick..\r\n",
                 newone);
    return 0;
  }
#endif
  /* if he applies to himself, then set him to uninterested */
  if (newone == member)
  {
    SET_NO_THANKS(temp);
    GET_A_NUM(newone) = 0;
    send_to_char("Fine, fine. You are on your own now.\r\n", newone);
    set_title(newone);          /* clears it */
  }
  else
  {
    /* is member in any association? */
    mnum = GET_A_NUM(member);
    mtemp = GET_A_BITS(member);
    if (!IS_MEMBER(mtemp) || !mnum)
    {
      send_to_char("Try finding the right person to apply to!\r\n", newone);
      return (-1);
    }
    /* member has to be a senior or higher to enrol */
    if (!GT_NORMAL(mtemp))
    {
      send_to_char("You need a senior member to get enrolled!\r\n", newone);
      return (-1);
    }
    /* test association file to see if association exists */
    sprintf(Gbuf, "%sasc.%u", ASC_DIR, mnum);
    if (stat(Gbuf, &statbuf))
    {
      send_to_char("Too bad, that association has been deleted!", newone);
      update_member(member, 1);
      return (-1);
    }
    SET_APPLICANT(temp);
    GET_A_NUM(newone) = mnum;
    strcpy(Gbuf, "You ask ");
    strcat(Gbuf, GET_NAME(member));
    strcat(Gbuf, " to enroll you in the association.\r\n");
    send_to_char(Gbuf, newone);
    strcpy(Gbuf, GET_NAME(newone));
    strcat(Gbuf, " is applying for membership in your association!\r\n");
    send_to_char(Gbuf, member);
  }
  GET_A_BITS(newone) = temp;

  return (GET_A_NUM(newone));
}




/* char member enrols char newone to the guild
   if successful, then total number of players is returned,
   if there is a member of this name already or
   if there are too many members or insane call 0 is returned,
   if there is a problem with the association file -1 is returned */

int enrol_asc(P_char member, P_char newone)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     asc_name[MAX_STR_ASC];
  char     title[MAX_STR_TITLE], *new_title;
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     asc_bits, dummy1;
  ush_int  asc_number;

  /* checks on member */
  dummy1 = GET_A_BITS(member);
  asc_number = GET_A_NUM(member);
  /* only members can do this */
  if (!IS_MEMBER(dummy1) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", member);
    return (0);
  }
  /* idiot calls */
  if (member == newone)
  {
    send_to_char("Who do you think are you fooling?\r\n", member);
    return (0);
  }
  /* only seniors or higher can enrol new members */
  if (!GT_NORMAL(dummy1))
  {
    send_to_char("Maybe one day you will be trusted enough to do that.\r\n",
                 member);
    return (0);
  }
  /* illithid?  ack, phhbptphbpth */
#if 0
  if (IS_ILLITHID(newone) && GET_LEVEL(newone) < 40)
  {
    send_to_char("You crazy?  You'd be missing your brains in seconds.\r\n",
                 member);
    return 0;
  }
#endif
  /* is this one willing to join? */
  if (!IS_APPLICANT(GET_A_BITS(newone)))
  {
    send_to_char("Don't bug other people...\r\n", member);
    return (0);
  }
  if (GET_A_NUM(newone) != asc_number)
  {
    send_to_char("Trying to snatch applicants, eh?\r\n", member);
    return (0);
  }
  /* people with debts can't do this */
  if (IS_DEBT(dummy1) && !IS_TRUSTED(member))
  {
    send_to_char("Pay your dues! Let's see some cash...\r\n", member);
    return (0);
  }
  /* no associations under level MIN_LEVEL */
  if (GET_LEVEL(newone) < MIN_LEVEL)
  {
    send_to_char("Try to find more experienced applicants!\r\n", member);
    return (0);
  }
  /* not for mobs */
  if (IS_NPC(newone))
  {
    send_to_char
      ("Geez, hard up for applicants? Try getting a life, maybe you'll be a bit more popular.\r\n",
       member);
    return 0;
  }
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  /* goto and read association bits, copy "normal member" title */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  Gbuf2[strlen(Gbuf2) - 1] = 0;
  Gbuf2[MAX_STR_ASC - 1] = 0;
  strcpy(asc_name, Gbuf2);
  for (i = 1; i < 3; i++)
    fgets(Gbuf2, MAX_STR_NORMAL, f);
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  Gbuf2[strlen(Gbuf2) - 1] = 0;
  Gbuf2[MAX_STR_RANK - 1] = 0;
  if (!IS_HIDDENSUBTITLE(asc_number))
    strcpy(title, Gbuf2);
  for (i = 4; i < 9; i++)
    fgets(Gbuf2, MAX_STR_NORMAL, f);
  fscanf(f, "%u\n", &asc_bits);
  /* go past cash */
  fgets(Gbuf2, MAX_STR_NORMAL, f);

  /* control if newone doesn't exist already or guild is too big */
  i = 0;
  while (fscanf(f, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
                &dummy3, &dummy4, &dummy5) != EOF)
  {
    if (!str_cmp(GET_NAME(newone), Gbuf2))
    {
      fclose(f);
      send_to_char("That person is enrolled anyway.\r\n", member);
      return (0);
    }
    if (IS_MEMBER(dummy1))
      i++;
  }
  fclose(f);

  if (i >= max_assoc_size(asc_number) )
  {
    send_to_char("Your association cannot grow any further until you gain some prestige!\r\n", member);
    return (0);
  }
  
  /* write newone at end of list */
  f = fopen(Gbuf1, "a");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  dummy1 = asc_bits;
  SET_MEMBER(dummy1);
  SET_NORMAL(dummy1);
  fprintf(f, "%s %u %i %i %i %i\n", GET_NAME(newone), dummy1, 0, 0, 0, 0);
  fclose(f);

  /* setup newone */
  GET_A_NUM(newone) = asc_number;
  GET_A_BITS(newone) = dummy1;

  /* construct and add new title */
  strcat(title, asc_name);
  new_title = title_member(newone, title);
  if (new_title)
  {
    strcpy(Gbuf2, "You are now ");
    strcat(Gbuf2, title);
    strcat(Gbuf2, ".\r\n");
    send_to_char(Gbuf2, newone);
  }
  send_to_char("Yeah! Another member!\r\n", member);

  return (i);
}




/* online char member has his association info updated from
   the association file, full decides if title is shown */

void update_member(P_char member, int full)
{
  FILE    *f;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_STRING_LENGTH];
  char     asc_name[MAX_STR_ASC], home[1000];
  char     rank_name[8][MAX_STR_RANK];
  char     title[MAX_STR_TITLE], *new_title;
  int      i, flag, x;
  // old guildhalls (deprecated)
//  P_house  temp_house;

  ush_int  asc_number;
  int      dummy2, dummy3, dummy4, dummy5;
  uint     dummy1, temp;

  /* nothing to do */
  if (!member)
    return;
  /* this procedure is called from outside, so better safe than sorry */
  if (IS_NPC(member))
    return;

  /* can we fix this guy from a file? */
  asc_number = GET_A_NUM(member);
  temp = GET_A_BITS(member);

  // check to see if they are homed in a guildhall
  // and reset to their original birthplace if they are no longer a guild member, or are homed in the wrong guildhall
  Guildhall *gh = Guildhall::find_by_vnum(GET_BIRTHPLACE(member));

  // If they're homed in a guildhall,
  if( gh )
  {
    // If they're no longer a member, or are a member of a different guild.
    if( !IS_MEMBER(temp) || gh->assoc_id != asc_number )
    {
      logit(LOG_DEBUG, "%s was homed in GH but doesn't belong to guild, resetting to original home", GET_NAME(member));
      sprintf(home, "char %s home %d", J_NAME(member), GET_ORIG_BIRTHPLACE(member));
      do_setbit(member, home, CMD_SETHOME);
      sprintf(home, "char %s orighome %d", J_NAME(member), GET_ORIG_BIRTHPLACE(member));
      do_setbit(member, home, CMD_SETHOME);
    }
  }

// old guildhalls (deprecated)
//    /* let's check to see they are not still sethomed to a guildhall */
//    temp_house = first_house;
//    flag = 0;
//
//    while (!flag && temp_house)
//    {
//      for (x = 0; x < temp_house->num_of_rooms; x++)
//      {
//        if (temp_house->room_vnums[x] == member->player.birthplace)
//        {
//          sprintf(home, "char %s home %d", J_NAME(member),
//                  member->player.orig_birthplace);
//          do_setbit(member, home, CMD_SETHOME);
//          sprintf(home, "char %s orighome %d", J_NAME(member),
//                  member->player.orig_birthplace);
//          do_setbit(member, home, CMD_SETHOME);
//          flag = 1;
//        }
//      }
//      if (!flag)
//        temp_house = temp_house->next;
//    }
//    return;

  /* test for MIN_LEVEL */
  level_check(member);

  // If not guilded, return.
  if( asc_number <= 0 )
    return;

  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    send_to_char("Your association has ceased to exist!\r\n", member);
    GET_A_BITS(member) = 0;
    SET_NO_THANKS(GET_A_BITS(member));
    GET_A_NUM(member) = 0;
    set_title(member);          /* clears it */
    sprintf(home, "char %s home %d", J_NAME(member),
            member->player.orig_birthplace);
    do_setbit(member, home, CMD_SETHOME);
    sprintf(home, "char %s orighome %d", J_NAME(member),
            member->player.orig_birthplace);
    do_setbit(member, home, CMD_SETHOME);
    member->player.hometown = member->player.birthplace =
      member->player.orig_birthplace;
    return;
  }
  /* read the association name */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  Gbuf2[strlen(Gbuf2) - 1] = 0;
  Gbuf2[MAX_STR_ASC - 1] = 0;
  strcpy(asc_name, Gbuf2);
  /* read the rank names */
  for (i = 0; i < 8; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    Gbuf2[strlen(Gbuf2) - 1] = 0;
    Gbuf2[MAX_STR_RANK - 1] = 0;
    strcpy(rank_name[i], Gbuf2);
  }
  /* goto past cash */
  for (i = 0; i < 2; i++)
    fgets(Gbuf2, MAX_STR_NORMAL, f);

  /* control file info */
  while (fscanf(f, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
                &dummy3, &dummy4, &dummy5) != EOF)
    if (!str_cmp(GET_NAME(member), Gbuf2))
    {
      if (IS_NO_THANKS(dummy1))
        send_to_char("You aren't interested in associations.\r\n", member);
      else if (IS_APPLICANT(dummy1))
        send_to_char("You are applicant to an association.\r\n", member);
      else if (IS_BANNED(dummy1))
        send_to_char("You are banned from all associations!\r\n", member);


      if (IS_MEMBER(dummy1) && full && !(GET_A_BITS(member) & A_HIDETITLE))
      {
        strcpy(Gbuf1, "Your title is '");

        if ((GET_A_BITS(member) & A_STATICTITLE) && GET_TITLE(member))
        {
          strcat(Gbuf1, GET_TITLE(member));
          strcat(Gbuf1, "&n'\r\n");
          send_to_char(Gbuf1, member);
        }
        else
        {
          /* construct and add new title */
          strcpy(title, rank_name[(int) NR_RANK(dummy1)]);
          strcat(title, asc_name);
          new_title = title_member(member, title);
          if (new_title)
          {
            strcat(Gbuf1, title);
            strcat(Gbuf1, "&n'\r\n");
            send_to_char(Gbuf1, member);
          }
        }
      }
      if (IS_MEMBER(dummy1) && full && (GET_A_BITS(member) & A_HIDESUBTITLE))
      {
        send_to_char("!subtitle association flag set, removing subtitle.\r\n", member);
	strcpy(Gbuf1, "Your title is '");
        strcpy(title, asc_name);
        new_title = title_member(member, title);
        if (new_title)
	{
	  strcat(Gbuf1, title);
	  strcat(Gbuf1, "&n'\r\n");
	  send_to_char(Gbuf1, member);
	}
      }
      if (IS_DEBT(dummy1) && !IS_DEBT(temp))
        send_to_char("Don't forget to pay your association dues!\r\n",
                     member);
      if (!IS_DEBT(dummy1) && IS_DEBT(temp))
        send_to_char("You do not owe your association money anymore.\r\n",
                     member);

      /* semi-hack to save STATICTITLE flag..  ah well */
      if (GET_A_BITS(member) & A_STATICTITLE)
        dummy1 |= A_STATICTITLE;

      GET_A_BITS(member) = dummy1;
      fclose(f);
      return;
    }

  fclose(f);

  /* if we get here, the sucker has been deleted... protect gods though */
  if (IS_TRUSTED(member))
     return;

  send_to_char("You were kicked out of your association in disgrace!\r\n", member);
  set_title(member);
  GET_A_NUM(member) = 0;
  GET_NB_LEFT_GUILD(member) += 1;
  GET_TIME_LEFT_GUILD(member) = time(NULL);
  SET_NO_THANKS(GET_A_BITS(member));
//  sprintf(home, "char %s home %d", J_NAME(member), member->player.orig_birthplace);
  //do_setbit(member, home, CMD_SETHOME);
//  sprintf(home, "char %s orighome %d", J_NAME(member), member->player.orig_birthplace);
  //do_setbit(member, home, CMD_SETHOME);

  member->player.hometown = member->player.birthplace =
    member->player.orig_birthplace;

  return;
}





/* checks the level of member, and if he has fallen below MIN_LEVEL,
   he is put on parole, returns 1 if level ok or on parole/enemy already,
   0 if he is below, -1 on file problems */

int level_check(P_char member)
{
  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1;
  ush_int  asc_number;

  /* no need to do anything */
  if (GET_LEVEL(member) >= MIN_LEVEL)
    return (1);

  /* what association is member in? */
  asc_number = GET_A_NUM(member);
  dummy1 = GET_A_BITS(member);
  /* this only makes sense for members */
  if (!IS_MEMBER(dummy1) || !asc_number)
    return (1);

  /* he is set up correctly anyway */
  if (IS_PAROLE(dummy1) || IS_ENEMY(dummy1))
    return (1);

  /* set him up, that will also stop a 2nd call here due to if above */
  SET_PAROLE(dummy1);
  GET_A_BITS(member) = dummy1;

  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    return (-1);
  }
  /* goto past cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 10; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }


  /* control if member does exist, put on parole */
  i = 0;
  while (fgets(Gbuf3, MAX_STR_NORMAL, f))
  {
    sscanf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    if (!str_cmp(GET_NAME(member), Gbuf2))
    {
      /* if he is enemy/parole, we don't want to change that */
      if (IS_PAROLE(dummy1) || IS_ENEMY(dummy1))
      {
        fclose(f);
        return (0);
      }
      SET_PAROLE(dummy1);
      sprintf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
              dummy3, dummy4, dummy5);
      i++;
    }
    strcat(buf, Gbuf3);
  }
  fclose(f);
  /* couldn't find him, probably deleted */
  if (!i)
  {
    return (0);
  }
  /* now write the buffer with changes */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  /* tell the guy about it */
  send_to_char("You are on parole due to your low level!\r\n", member);

  return (0);
}





/* char member kicks out char name of the association
   if successful, then number of players deleted (1 ?!) is returned,
   if there is no member of this name or it's the last one 0 is returned,
   if there is a problem with the association file -1 is returned */

int kickout_member(P_char member, char *name)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, j, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1, temp;
  ush_int  asc_number;

  /* association and rank of member? */
  asc_number = GET_A_NUM(member);
  temp = GET_A_BITS(member);
  /* only members can do this */
  if (!IS_MEMBER(temp) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", member);
    return (0);
  }
  /* minimum rank to to this is deputy */
  if (!GT_OFFICER(temp))
  {
    send_to_char("You rank is too low. Forget it.\r\n", member);
    return (0);
  }
  /* people with debts can't do this */
  if (IS_DEBT(temp) && !IS_TRUSTED(member))
  {
    send_to_char("Pay your dues! Let's see some cash...\r\n", member);
    return (0);
  }
  /* test name for "me" and "all" */
  if (!str_cmp(name, "me") || !str_cmp(name, GET_NAME(member)))
  {
    send_to_char("You can't kick yourself out, try seceding.\r\n", member);
    return (0);
  }
  if (!str_cmp(name, "all"))
  {
    send_to_char("Ask a god to wipe out the guild instead.\r\n", member);
    return (0);
  }
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  /* goto past cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 10; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }

  /* control if member does exist, delete by not copying */
  i = j = 0;
  while (fgets(Gbuf3, MAX_STR_NORMAL, f))
  {
    j++;
    sscanf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    if (str_cmp(name, Gbuf2))
    {
      strcat(buf, Gbuf3);
    }
    else
    {
      /* delete only people of lower rank */
      if (NR_RANK(dummy1) >= NR_RANK(temp))
      {
        send_to_char("You can only kick out people below your rank!\r\n",
                     member);
        fclose(f);
        return (0);
      }
      i++;
    }
  }
  fclose(f);

  /* couldn't find anybody to delete, quit now */
  if (!i)
  {
    send_to_char("There is no one of that name in the association!\r\n",
                 member);
    return (0);
  }
  /* never delete the last player in association, kill the
     association instead! - no "empty" association files */
  if (j == 1)
  {
    send_to_char("Ask a god to delete the association instead!\r\n", member);
    return (0);
  }
  /* now write the buffer with deletions */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  send_to_char("That member is gone for good!\r\n", member);
  /* return number of deleted members (should be one!) */
  return (i);
}




/* char member secedes from the association
   if successful, then number of leaving players (1 ?!) is returned,
   if there is no member of this name or it's the last one 0 is returned,
   if there is a problem with the association file -1 is returned */

int secede_asc(P_char member)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE], home[1000];
  int      i, j, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1;
  ush_int  asc_number;

  /* what association is member in? */
  asc_number = GET_A_NUM(member);
  if (!asc_number || !IS_MEMBER(GET_A_BITS(member)))
  {
    send_to_char("You can't terminate a non-existent membership.\r\n",
                 member);
    return (0);
  }
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  /* goto past cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 10; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }


  /* control if member does exist, delete by not copying */
  i = j = 0;
  while (fgets(Gbuf3, MAX_STR_NORMAL, f))
  {
    j++;
    sscanf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    if (str_cmp(GET_NAME(member), Gbuf2))
    {
      strcat(buf, Gbuf3);
    }
    else
    {
      /* if the player is enemy, he is not deleted but ostracized,
         but still his bits are set, so that he can join elsewhere */
      if (IS_ENEMY(dummy1))
      {
        SET_NO_THANKS(dummy1);
        sprintf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
                dummy3, dummy4, dummy5);
        strcat(buf, Gbuf3);
      }
      i++;
    }
  }
  fclose(f);
  /* couldn't find his name, quit now */
  if (!i)
  {
    SET_NO_THANKS(GET_A_BITS(member));
    GET_A_NUM(member) = 0;
    Gbuf2[0] = 0;
    title_member(member, Gbuf2);
    send_to_char("You aren't enrolled anyway...\r\n", member);
    return (0);
  }
  /* never delete the last player in association, kill the
     association instead! - no "empty" association files */
  if (j == 1)
  {
    send_to_char("Ask a god to delete the association instead!\r\n", member);
    return (0);
  }
  /* now write the buffer with deletions */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  /* now we can safely unset membership */
  SET_NO_THANKS(GET_A_BITS(member));
  GET_A_NUM(member) = 0;
  /* you quit, you can never rejoin another guild, w/o god help */
  SET_BANNED(GET_A_BITS(member));
  GET_NB_LEFT_GUILD(member) += 1;
  GET_TIME_LEFT_GUILD(member) = time(NULL);
  Gbuf2[0] = '\0';
/*  title_member(member, Gbuf2);*/
  set_title(member);
  sprintf(home, "char %s home %d", J_NAME(member),
          member->player.orig_birthplace);
  //do_setbit(member, home, CMD_SETHOME);
  sprintf(home, "char %s orighome %d", J_NAME(member),
          member->player.orig_birthplace);
  //do_setbit(member, home, CMD_SETHOME);

  send_to_char("You are on your own once more.\r\n", member);
  /* return number of leaving members (should be one!) */
  return (i);
}




/* char member fines char name of the association
   fine is given in platinum coins (pc), gold (gc), silver (sc), copper (cc)
   if successful, then number of players fined (1 ?!) is returned,
   if there is no member of this name or bogus call 0 is returned,
   if there is a problem with the association file -1 is returned */

int fine_member(P_char member, char *name, int pc, int gc, int sc, int cc)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1, temp;
  ush_int  asc_number;


  /* sanity check, what association is member in? */
  temp = GET_A_BITS(member);
  asc_number = GET_A_NUM(member);
  /* only members can do this */
  if (!IS_MEMBER(temp) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", member);
    return (0);
  }
  /* minimum rank to do this is officer */
  if (!GT_SENIOR(temp))
  {
    send_to_char("You need to advance in rank to do this.\r\n", member);
    return (0);
  }
  /* people with debts can't do this */
  if (IS_DEBT(temp) && !IS_TRUSTED(member))
  {
    send_to_char("Pay your dues! Let's see some cash...\r\n", member);
    return (0);
  }
  /* allow "me" as name */
  if (!str_cmp(name, "me"))
    name = GET_NAME(member);

  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  /* goto past cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 10; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }

  /* control if member does exist, fine it if yes */
  /* "all" is allowed, only lower ranks are fined */
  i = 0;
  while (fgets(Gbuf3, MAX_STR_NORMAL, f))
  {
    sscanf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    if ((!str_cmp(name, Gbuf2) || !str_cmp(name, "all")) &&
        (NR_RANK(dummy1) < NR_RANK(temp)))
    {

      /* change fines into biggest coins, this will keep numbers small */
      sc += cc / 10;
      cc %= 10;
      gc += sc / 10;
      sc %= 10;
      pc += gc / 10;
      gc %= 10;

      /* negative values used to erase debt */
      dummy2 += pc;
      if (dummy2 < 0)
      {
        gc += 10 * dummy2;
        dummy2 = 0;
      }
      dummy3 += gc;
      if (dummy3 < 0)
      {
        sc += 10 * dummy3;
        dummy3 = 0;
      }
      dummy4 += sc;
      if (dummy4 < 0)
      {
        cc += 10 * dummy4;
        dummy4 = 0;
      }
      dummy5 += cc;
      if (dummy5 < 0)
        dummy5 = 0;

      if (dummy2 + dummy3 + dummy4 + dummy5)
        SET_DEBT(dummy1);
      else
        REMOVE_DEBT(dummy1);

      /* change buffer */
      sprintf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
              dummy3, dummy4, dummy5);

      i++;
    }
    strcat(buf, Gbuf3);
  }
  fclose(f);
  /* couldn't find anybody to fine, quit now */
  if (!i)
  {
    send_to_char("Couldn't find anyone to give a fine to.\r\n", member);
    return (0);
  }
  /* now write the buffer including fines */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  send_to_char("The fine is in the books now.\r\n", member);
  /* return number of finded members (should be one!) */
  return (i);
}




/* char member advances rank of char name by levels
   levels contains the number of leves to promote (>0) or demote (<0)
   if successful, then number of players deleted (1 ?!) is returned,
   if there is no member or rank request is stupid 0 is returned,
   if there is a problem with the association file -1 is returned */

int rank_member(P_char member, char *name, int levels)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1, temp;
  ush_int  asc_number;
  P_char   target = NULL;

  /* what association is member in? */
  asc_number = GET_A_NUM(member);
  dummy1 = GET_A_BITS(member);
  /* only members can do this */
  if (!IS_MEMBER(dummy1) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", member);
    return (0);
  }
  /* minimum rank to do this is deputy */
  if (!GT_OFFICER(dummy1))
  {
    send_to_char("You need to be high up in the ranks to do this.\r\n",
                 member);
    return (0);
  }
  /* people with debts can't do this */
  if (IS_DEBT(dummy1) && !IS_TRUSTED(member))
  {
    send_to_char("Pay your dues! Let's see some cash...\r\n", member);
    return (0);
  }
  /* test for stupidity of calling proc */
  if (!levels)
  {
    send_to_char("Ok, member rank didn't change. Happy now?", member);
    return (0);
  }
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  /* goto past cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 10; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }


  /* control if member does exist, change rank */
  i = 0;
  while (fgets(Gbuf3, MAX_STR_NORMAL, f))
  {
    sscanf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    if (!str_cmp(name, Gbuf2))
    {
      temp = GET_M_BITS(dummy1, A_RK_MASK) + (levels * A_RK1);

      if (IS_GOD(temp) && IS_TRUSTED(member))
      {
        send_to_char("Promoted to King!\r\n", member);
      }
      else if (GT_LEADER(temp) || LT_NORMAL(temp))
      {
        send_to_char("Choose a sensible rank change.\r\n", member);
        fclose(f);
        return (0);
      }
      /* deputies can't lift enemy status */
      if (IS_ENEMY(dummy1) && !GT_DEPUTY(GET_A_BITS(member)))
      {
        send_to_char("You can't promote a declared enemy!\r\n", member);
        fclose(f);
        return (0);
      }
      /* only leader or god can raise to leader */
      if (IS_LEADER(temp) && !GT_DEPUTY(GET_A_BITS(member)))
      {
        send_to_char("You can't promote to a higher rank than your own!\r\n",
                     member);
        fclose(f);
        return (0);
      }
      if (NR_RANK(GET_A_BITS(member)) < NR_RANK(dummy1))
      {
        send_to_char("You can't de-rank a superior!&N\r\n", member);
        fclose(f);
        return (0);
      }

#if 0
      if (IS_LEADER(temp) && !IS_TRUSTED(member) && RACE_PUNDEAD(member))
      {
        target = mm_get(dead_mob_pool);
        target->only.pc = mm_get(dead_pconly_pool);

        if ((restoreCharOnly(target, skip_spaces(name)) < 0) || !target)
        {
          if (target)
          {
            free_char(target);
            target = NULL;
          }
        }
      }
#endif
      /* setup member */
      SET_M_BITS(dummy1, A_RK_MASK, temp);
      sprintf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
              dummy3, dummy4, dummy5);
      i++;
    }
    strcat(buf, Gbuf3);
  }
  fclose(f);
  /* couldn't find anybody to rank, quit now */
  if (!i)
  {
    send_to_char("Whose rank do you want to change?\r\n", member);
    return (0);
  }
  /* now write the buffer with deletions */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  send_to_char("You changed the rank.\r\n", member);
  /* return number of ranked members (should be one!) */
  return (i);
}



/* char member punishes char name, first call -> parole, second call -> enemy
   if successful, then number of players punished (1 ?!) is returned,
   if there is no member of that name or request is stupid 0 is returned,
   if there is a problem with the association file -1 is returned */

int punish_member(P_char member, char *name)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      flag, i, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1;
  ush_int  asc_number;

  /* what association is member in? */
  asc_number = GET_A_NUM(member);
  dummy1 = GET_A_BITS(member);
  /* only members can do this */
  if (!IS_MEMBER(dummy1) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", member);
    return (0);
  }
  /* minimum rank to do this is leader */
  if (!GT_DEPUTY(dummy1))
  {
    send_to_char
      ("This is too serious, let the ones at the top handle it.\r\n", member);
    return (0);
  }
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  /* goto past cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 10; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }


  /* control if member does exist, punish */
  flag = 0;
  i = 0;
  while (fgets(Gbuf3, MAX_STR_NORMAL, f))
  {
    sscanf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    if (!str_cmp(name, Gbuf2))
    {
      /* leaders can't be punished, except by god */
      if (GT_DEPUTY(dummy1) && !IS_TRUSTED(member))
      {
        send_to_char("Sorry, you would need a god to punish that person.\r\n",
                     member);
        fclose(f);
        return (0);
      }
      /* first punish -> parole, second punish -> enemy */
      if (IS_PAROLE(dummy1))
      {
        flag = 1;
        SET_ENEMY(dummy1);
      }
      else
        SET_PAROLE(dummy1);
      sprintf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
              dummy3, dummy4, dummy5);
      i++;
    }
    strcat(buf, Gbuf3);
  }
  fclose(f);
  /* couldn't find anybody to punish, quit now */
  if (!i)
  {
    send_to_char("There is no person of that name in the books.\r\n", member);
    return (0);
  }
  /* now write the buffer with changes */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  if (flag)
    send_to_char("That person is a declared enemy now!\r\n", member);
  else
    send_to_char("You put that person on parole.\r\n", member);
  /* return number of ranked members (should be one!) */
  return (i);
}



/* pay money into association account, check for dues of player
   if successful, then 1 is returned,
   if there is no member or not enough money 0 is returned,
   if there is a problem with the association file -1 is returned */

int deposit_asc(P_char member, int pc, int gc, int sc, int cc)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1;
  long int temp;
  ush_int  asc_number;

  /* check for sanity of calling proc */
  if ((pc < 0) || (gc < 0) || (sc < 0) || (cc < 0))
  {
    send_to_char("You are not going to get money. No way.", member);
    return (0);
  }
  /* check if player has the money */
  if (GET_PLATINUM(member) < pc || GET_GOLD(member) < gc ||
      GET_SILVER(member) < sc || GET_COPPER(member) < cc)
  {
    send_to_char("You don't have that much money on you!\r\n", member);
    return (0);
  }
  /* what association is member in? */
  dummy1 = GET_A_BITS(member);
  if (IS_NO_THANKS(dummy1) || IS_BANNED(dummy1))
  {
    send_to_char("Why do you even try to pay?\r\n", member);
    return (0);
  }
  asc_number = GET_A_NUM(member);
  /* what association? */
  if (!asc_number)
  {
    send_to_char("How about joining an association first?\r\n", member);
    return (0);
  }

  if (!test_atm_present(member))
  {
    send_to_char("I don't see a bank around here.\r\n", member);
    return(0);
  }

  if( (pc+gc+sc+cc) <= 0 )
  {
    send_to_char("Don't be silly.\r\n", member);
    return(0);
  }

  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  /* goto cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 9; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }

  /* if we get this far, then player has the money */
  fscanf(f, "%i %i %i %i\n", &dummy2, &dummy3, &dummy4, &dummy5);
  sprintf(Gbuf2, "%i %i %i %i\n", dummy2 + pc, dummy3 + gc, dummy4 + sc,
          dummy5 + cc);
  strcat(buf, Gbuf2);

  /* control if member does exist, change debt if any... */
  i = 0;
  while (fgets(Gbuf3, MAX_STR_NORMAL, f))
  {
    sscanf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    if (!str_cmp(GET_NAME(member), Gbuf2))
    {
      /* total debt minus amount paid (in copper) */
      temp = 1000 * (dummy2 - pc);
      temp += 100 * (dummy3 - gc);
      temp += 10 * (dummy4 - sc);
      temp += dummy5 - cc;
      /* if result is >0 then debt is left, update */
      /* otherwise no more debt, rest is donation */
      if (temp > 0)
      {
        dummy2 = (int) temp / 1000;
        temp -= dummy2 * 1000;
        dummy3 = (int) temp / 100;
        temp -= dummy3 * 100;
        dummy4 = (int) temp / 10;
        temp -= dummy4 * 10;
        dummy5 = (int) temp;
        sprintf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
                dummy3, dummy4, dummy5);
        send_to_char("You still owe the association some money.\r\n", member);
      }
      else
      {
        REMOVE_DEBT(dummy1);
        sprintf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, dummy1, 0, 0, 0, 0);
        GET_A_BITS(member) = dummy1;
        send_to_char("Thanks for your payment!\r\n", member);
      }
      i++;
    }
    strcat(buf, Gbuf3);
  }
  fclose(f);

  /* if it's an applicant, then member is not in the files - but can pay
     entrance fee */
  if (IS_APPLICANT(GET_A_BITS(member)))
  {
    send_to_char("If this isn't a donation, tell them you paid up!\r\n",
                 member);
    i++;
  }
  /* couldn't find the guy in association, quit now */
  if (!i)
  {
    update_member(member, 1);
    return (0);
  }
  /* now write the buffer with money changes, only now change player money */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  GET_PLATINUM(member) -= pc;
  GET_GOLD(member) -= gc;
  GET_SILVER(member) -= sc;
  GET_COPPER(member) -= cc;
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  if( IS_PC(member) )
  {
    sprintf(buf, "&+y%s deposited &+W%dp&n&+y, &+Y%dg&n&+y, &+w%ds&n&+y, and &+y%dc", GET_NAME(member), pc, gc, sc, cc);
    insert_guild_transaction(asc_number, buf);
  }

  /* return number of changed players, should be one */
  return (i);
}





/* pay money from association account, deputy or higher
   if successful, then 1 is returned,
   if there is not enough money 0 is returned,
   if there is a problem with the association file -1 is returned */

int sub_money_asc(int asc, int pc, int gc, int sc, int cc)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, dummy2, dummy3, dummy4, dummy5;
  long int temp;
  ush_int  asc_number = (ush_int)asc;
  char     *tmp2;
  int      banksum, wsum;

	  
  /* check for sanity of calling proc */
  if ((pc < 0) || (gc < 0) || (sc < 0) || (cc < 0))
    return (0);

  if( (pc+gc+sc+cc) <= 0 )
    return(0);
 
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
    return (-1);
  /* goto cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 9; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }

  /* check if association has the money */
  fscanf(f, "%i %i %i %i\n", &dummy2, &dummy3, &dummy4, &dummy5);
  
  // convert to sum totals
  banksum = (dummy2*1000) + (dummy3*100) + (dummy4*10) + dummy5;
  wsum = (pc*1000) + (gc*100) + (sc*10) + cc;
  if (banksum < wsum)
  {
    fclose(f);
    return (0);
  }
  // convert back to pgsc format.
  banksum -= wsum; // subtract cash
  dummy2 = banksum / 1000;
  banksum = banksum % 1000;
  dummy3 = banksum / 100;
  banksum = banksum % 100;
  dummy4 = banksum / 10;
  banksum = banksum % 10;
  dummy5 = banksum;

  /* update association cash */
  sprintf(Gbuf2, "%i %i %i %i\n", dummy2, dummy3, dummy4, dummy5);
  strcat(buf, Gbuf2);

  /* copy past members */
  while (fgets(Gbuf2, MAX_STR_NORMAL, f))
    strcat(buf, Gbuf2);
  fclose(f);

  /* now write the buffer with money changes, only now change player money */
  f = fopen(Gbuf1, "w");
  if (!f)
    return (-1);
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);
  logit(LOG_PLAYER, "Guild Withdrawal %d p %d g %d s %d c by %s", pc, gc, sc,
        cc, str_dup("System"));
  
  sprintf(buf, "&+y%s withdrew &+W%dp&n&+y, &+Y%dg&n&+y, &+w%ds&n&+y, and &+y%dc", str_dup("System"), pc, gc, sc, cc);
  insert_guild_transaction(asc_number, buf);
  
  /* return one for success */
  return (1);
}


/* pay money from association account, deputy or higher
   if successful, then 1 is returned,
   if there is not enough money 0 is returned,
   if there is a problem with the association file -1 is returned */

int withdraw_asc(P_char member, int pc, int gc, int sc, int cc)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, dummy2, dummy3, dummy4, dummy5;
  long int temp;
  ush_int  asc_number;
  char     *tmp2;

	  
  /* check for sanity of calling proc */
  if ((pc < 0) || (gc < 0) || (sc < 0) || (cc < 0))
    return (0);

  temp = GET_A_BITS(member);
  asc_number = GET_A_NUM(member);
  
  if( (pc+gc+sc+cc) <= 0 )
  {
    send_to_char("Don't be silly.\r\n", member);
    return(0);
  }
  
  /* only members can do this */
  if(!IS_NPC(member)){
  
    if (!IS_MEMBER(temp) || !asc_number)
   {
    send_to_char("How about joining an association first?\r\n", member);
    return (0);
   }
 // Only banks can handle this kind of things, let people die with money

  if (!test_atm_present(member))
  {
    send_to_char("I don't see a bank around here.\r\n", member);
    return(0);
  }
  /* deputies or higher only */
   if (!GT_OFFICER(temp))
   {
    send_to_char("You wish you could do that, don't you?\r\n", member);
    return (0);
   }
  
  /* people with debts can't do this */
   if (IS_DEBT(temp) && !IS_TRUSTED(member))
   {
     send_to_char("Pay your dues! Let's see some cash...\r\n", member);
     return (0);
   }
  }
  else{
 
    tmp2 = strstr(GET_NAME(member), "assoc");
    if (!tmp2)
	 return 0;
     asc_number = (ush_int) atoi(tmp2 + 5);
  }
  

  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  /* goto cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 9; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }

  /* check if association has the money */
  fscanf(f, "%i %i %i %i\n", &dummy2, &dummy3, &dummy4, &dummy5);
  if (dummy2 < pc || dummy3 < gc || dummy4 < sc || dummy5 < cc)
  {
    fclose(f);
    send_to_char("Deficit spending is forbidden...\r\n", member);
    return (0);
  }
  /* update association cash */
  sprintf(Gbuf2, "%i %i %i %i\n", dummy2 - pc, dummy3 - gc, dummy4 - sc,
          dummy5 - cc);
  strcat(buf, Gbuf2);

  /* copy past members */
  while (fgets(Gbuf2, MAX_STR_NORMAL, f))
    strcat(buf, Gbuf2);
  fclose(f);

  /* now write the buffer with money changes, only now change player money */
  GET_PLATINUM(member) += pc;
  GET_GOLD(member) += gc;
  GET_SILVER(member) += sc;
  GET_COPPER(member) += cc;
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(member, 1);
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);
  logit(LOG_PLAYER, "Guild Withdrawal %d p %d g %d s %d c by %s", pc, gc, sc,
        cc, GET_NAME(member));
  send_to_char("Ok, use that money for your association though.\r\n", member);
  
  if( IS_PC(member) )
  {
    sprintf(buf, "&+y%s withdrew &+W%dp&n&+y, &+Y%dg&n&+y, &+w%ds&n&+y, and &+y%dc", GET_NAME(member), pc, gc, sc, cc);
    insert_guild_transaction(asc_number, buf);
  }
  
  /* return one for success */
  return (1);
}


/* this allows the leader to change the rank names, proc uses
   level name and new_name, returns 2 on success, 0
   on failure, -1 on file problems (2 is returned to trigger
   a general association update in do_society) */
int name_rank(P_char leader, char *level, char *new_name)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      rank_number;
  ush_int  asc_number;
  int      i;
  uint     temp;
  char     sign;


  temp = GET_A_BITS(leader);
  asc_number = GET_A_NUM(leader);
  /* only members can do this */
  if (!IS_MEMBER(temp) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", leader);
    return (0);
  }
  /* deputies or higher only */
  if (!GT_DEPUTY(temp))
  {
    send_to_char("You would need to be at the top of your association!\r\n",
                 leader);
    return (0);
  }
  sign = LOWER(*level);
  switch (sign)
  {
  case 'e':
    rank_number = 0;            /* enemy */
    break;
  case 'p':
    rank_number = 1;            /* parole */
    break;
  case 'n':
    rank_number = 2;            /* normal */
    break;
  case 's':
    rank_number = 3;            /* senior */
    break;
  case 'o':
    rank_number = 4;            /* officer */
    break;
  case 'd':
    rank_number = 5;            /* deputy */
    break;
  case 'l':
    rank_number = 6;            /* leader */
    break;
  case 'g':
    rank_number = 7;            /* god */
    break;
  default:
    send_to_char("Which rank name do you want to change?\r\n", leader);
    return (0);
  }

  /* only gods can change god rank name... */
  if (rank_number == 7 && !IS_GOD(temp))
  {
    send_to_char("Do you really want to offend the king?\r\n", leader);
    return (0);
  }
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(leader, 1);
    return (-1);
  }
  if (strlen(new_name) >= MAX_STR_RANK)
    new_name[MAX_STR_RANK - 1] = 0;

  /* read the association name */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);

  /* read and change the rank names */
  for (i = 0; i < 8; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    if (i == rank_number)
      sprintf(Gbuf2, "%s\n", new_name);
    strcat(buf, Gbuf2);
  }

  /* copy the rest */
  while (fgets(Gbuf2, MAX_STR_NORMAL, f))
    strcat(buf, Gbuf2);
  fclose(f);

  /* write the changes */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(leader, 1);
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  send_to_char("You changed the rank name.\r\n", leader);
  return (2);
}

/* member list: list the names of members in association */

void member_list(P_char member)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     Gbuf4[MAX_STR_NORMAL];
  char	   cash[MAX_STR_NORMAL];
  char     ostra[MAX_STR_NORMAL];
  char     buf[MAX_STRING_LENGTH];
  char     rank_names[8][MAX_STR_RANK];
  char     name[MAX_STR_NORMAL];
  char     guild_name[MAX_STR_NORMAL];
  const char *standard_names[] = { "enemy", "parole", "normal", "senior",
    "officer", "deputy", "leader", "king"
  };
  int      i, j, k, os, flag, dummy2, dummy3, dummy4, dummy5;
  ush_int  asc_number;
  uint     dummy1, temp, asc_bits;
  struct sortit
  {
    int      ranknr;
    char     info[MAX_STR_NORMAL];
  } displayed[MAX_DISPLAY];
  P_char   target = NULL;

  temp = GET_A_BITS(member);
  asc_number = GET_A_NUM(member);
  /* only members can do this */
  if (!IS_MEMBER(temp) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", member);
    return;
  }
  /* normal or higher only (not enemies/parole) */
  if (!GT_PAROLE(temp))
  {
    send_to_char("Maybe if you try hard you will get accepted again.\r\n",
                 member);
    return;
  }
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return;
  }
  fgets(guild_name, MAX_STR_NORMAL, f);

  /* title of list */
  sprintf(buf, "\r\n%s&n%s\r\n", guild_name,
          "\r-----------------------------------------------------------------");
  
  struct alliance_data *alliance = get_alliance(asc_number);
  if( alliance )
  {
    if( IS_FORGING_ASSOC(alliance, asc_number) )
    {
      sprintf(Gbuf2, "&+bAllied with &n%s\n\n", get_assoc_name(alliance->joining_assoc_id).c_str());
      strcat(buf, Gbuf2);
    }
    else if( IS_JOINING_ASSOC(alliance, asc_number) )
    {
      sprintf(Gbuf2, "&+bAllied to &n%s\n\n", get_assoc_name(alliance->forging_assoc_id).c_str());
      strcat(buf, Gbuf2);
    }
  }
  
  k = strlen(guild_name);
  guild_name[k - 1] = '\0';
  k = 0;
//  sprintf(guild_frags_data[asc_number].g_name, "%s", guild_name);

  /* display rank names if player is leader or bigger */
  for (i = 0; i < 8; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    Gbuf2[strlen(Gbuf2) - 1] = 0;
    Gbuf2[MAX_STR_RANK - 1] = 0;
    strcpy(rank_names[i], Gbuf2);
    /* now show */
    if (GT_DEPUTY(temp))
    {
      sprintf(buf + strlen(buf), "%-8s", standard_names[i]);
      strcat(buf, ": ");
      strcat(buf, rank_names[i]);
      strcat(buf, "&n\r\n");
    }
  }
  strcat(buf, "\r\n");

  /* if rank names are empty display standard info */
  for (i = 0; i < 8; i++)
    if (!rank_names[i][0])
      strcpy(rank_names[i], standard_names[i]);

  /* display guild bits */
  fscanf(f, "%u\n", &asc_bits);
  if (GT_LEADER(temp))
  {
    strcat(buf, "Type: ");
    if (!asc_bits)
      strcat(buf, "normal ");
    else if (IS_CHALL(asc_bits))
      strcat(buf, "challenge ");
    if (IS_HIDDENTITLE(asc_bits))
      strcat(buf, "hidden titles ");
    if (IS_HIDDENSUBTITLE(asc_bits))
      strcat(buf, "hidden subtitles ");
    strcat(buf, "\r\n");    
  }
  
  sprintf(buf + strlen(buf), "Prestige: %d\r\n", get_assoc_prestige(asc_number));
  sprintf(buf + strlen(buf), "Construction points: &+W%d&n\r\n", get_assoc_cps(asc_number));

  sprintf(buf + strlen(buf), "Maximum members: %d\r\n", max_assoc_size(asc_number));

  /* display associations cash if player is senior or bigger -
     seniors need to be able to check if applicants have paid up! */
  fscanf(f, "%i %i %i %i\n", &dummy2, &dummy3, &dummy4, &dummy5);
  if (GT_NORMAL(temp))
  {
    sprintf(cash,
            "Cash: &+W%i platinum&n, &+Y%i gold&n, %i silver, &+y%i copper&n\r\n\r\n",
            dummy2, dummy3, dummy4, dummy5);
  }
    
  i = os = 0;
  strcpy(ostra, "\r\n&+cSuffering ostracism:&n\r\n");
  
  /* display info on members */
  while (fgets(Gbuf3, MAX_STR_NORMAL, f) && i < MAX_DISPLAY)
  {
    sscanf(Gbuf3, "%s %u %i %i %i %i\r\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    /* check for ostracized people */
    if (IS_ENEMY(dummy1) && !IS_MEMBER(dummy1))
    {
      os++;
      /* protect the string from being overrun */
      if ((strlen(ostra) + strlen(Gbuf2)) < (MAX_STR_NORMAL - 10))
      {
        strcat(ostra, Gbuf2);
        strcat(ostra, " ");
      }
    }
    else
    {
      flag = (get_pc_vis(member, Gbuf2) != NULL);
/*      if (NR_RANK(temp) >= NR_RANK(dummy1) || flag) { */
      strcat(name, Gbuf2);
      if (1)
      {
        displayed[i].ranknr = (int) NR_RANK(dummy1);
        if (flag)
          strcpy(displayed[i].info, " &+Go&n ");
        else
          strcpy(displayed[i].info, "   ");
        strcat(displayed[i].info, Gbuf2);
        strcat(displayed[i].info, " |");
        strcat(displayed[i].info, rank_names[(int) NR_RANK(dummy1)]);
        strcat(displayed[i].info, "&n| ");
        if (IS_DEBT(dummy1) && NR_RANK(temp) >= NR_RANK(dummy1))
        {
          if (GT_SENIOR(temp) || !str_cmp(GET_NAME(member), Gbuf2))
            sprintf(Gbuf2, " &+W%i p&n, &+Y%i g&n, %i s, &+y%i c&n",
                    dummy2, dummy3, dummy4, dummy5);
          else
            strcpy(Gbuf2, " &+R(-)&n");
          strcat(displayed[i].info, Gbuf2);
        }
        strcat(displayed[i].info, "\r\n");
        i++;
      }
    }
  }
  fclose(f);
  
  sprintf(Gbuf2, "Current members: %d\r\n", i);
  strcat(buf, Gbuf2);
  strcat(buf, cash);
  
  /* now add the info string sorted by rank to buf */
  for (k = 7; k >= 0; k--)
    for (j = 0; j < i; j++)
      if (displayed[j].ranknr == k)
        strcat(buf, displayed[j].info);
  if (os)
  {
    strcat(buf, ostra);
    strcat(buf, "\r\n");
  }
  //strcat(buf, Gbuf2);
  strcat(buf, "\r\n");

  /* now send to char through pager */
  send_to_char(buf, member, LOG_NONE);
  return;
}






/* member small challenges member big for his position in the association
   returns 1 for success of small, 0 for failure, -1 for file problems  */

int challenge(P_char small, P_char big)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     smallstr[MAX_STR_NORMAL];
  char     bigstr[MAX_STR_NORMAL];
  char     buf[ASC_SAVE_SIZE];
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1, temp, sbits, bbits;
  ush_int  snum, bnum;


  /* small has to be a member of an assocition */
  sbits = GET_A_BITS(small);
  snum = GET_A_NUM(small);
  /* only members can do this */
  if (!IS_MEMBER(sbits) || !snum)
  {
    send_to_char("How about joining an association first?\r\n", small);
    return (0);
  }
  /* no mortal can become a god */
  if (IS_LEADER(sbits))
  {
    send_to_char("You are at the top already, what more could you want?\r\n",
                 small);
    return (0);
  }
  /* idiotic call */
  if (small == big)
  {
    send_to_char("Yes, you are challenged, mentally challenged...\r\n",
                 small);
    return (0);
  }
  /* normal or higher only (not enemies/parole) */
  if (!GT_PAROLE(sbits))
  {
    send_to_char("Nobody would accept you in a higher rank anyway.\r\n",
                 small);
    return (0);
  }
  /* big has to be member of the same association! */
  bbits = GET_A_BITS(big);
  bnum = GET_A_NUM(big);
  if (!IS_MEMBER(bbits) || bnum != snum)
  {
    send_to_char("Pick a fight with a member of your association!\r\n",
                 small);
    return (0);
  }
  /* test if this association allows challenges currently */
  /* test if one of them has the wrong challenge bits */
  if (IS_CHALL(sbits) != IS_CHALL(bbits))
  {
    update_member(small, 0);
    update_member(big, 0);
  }
  if (!IS_CHALL(sbits))
  {
    send_to_char("Your association frowns upon forcing your way up.\r\n",
                 small);
    return (0);
  }
  /* this makes no sense if small can't advance in rank */
  if (NR_RANK(bbits) < NR_RANK(sbits))
  {
    send_to_char
      ("And your grandmother is the next one on your list, huh?\r\n", small);
    return (0);
  }
  /* we only allow a rank change by one */
  if (NR_RANK(bbits) != (NR_RANK(sbits) + 1))
  {
    send_to_char("Try challenging someone of the next higher rank.\r\n",
                 small);
    return (0);
  }
  /* no matter if he wins or not, small suffers the same align loss
     as if he had KILLED big! this will keep people in good associations
     from doing it too often */
  change_alignment(small, big);
  sprintf(Gbuf2, "You try to challenge %s for %s position!\r\n",
          GET_NAME(big), HSHR(big));
  send_to_char(Gbuf2, small);
  sprintf(Gbuf2, "%s is challenging you for your rank!\r\n", GET_NAME(small));
  send_to_char(Gbuf2, big);

  /* now we determine if challenge is a success, if small has more
     exp points, he might do it... */
  if (GET_EXP(small) <= GET_EXP(big))
  {
    send_to_char("You suddenly are very, very scared and give up!\r\n",
                 small);
    sprintf(Gbuf2, "%s blushes and avoids your stern glance.\r\n",
            GET_NAME(small));
    send_to_char(Gbuf2, big);
    return (0);
  }
  /* make this easy at low levels, hard at high levels */
  if (number(1, 2 + (GET_LEVEL(big) / 10)) != 2)
  {
    send_to_char("You plans are thwarted!\r\n", small);
    sprintf(Gbuf2, "%s's feeble attempt to challenge you has failed.\r\n",
            GET_NAME(small));
    send_to_char(Gbuf2, big);
    return (0);
  }
  /* hey wow, he did it... now we have to switch ranks of the two... */
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, snum);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(small, 1);
    update_member(big, 1);
    return (-1);
  }
  /* goto past cash, copy on the way */
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  strcpy(buf, Gbuf2);
  for (i = 0; i < 10; i++)
  {
    fgets(Gbuf2, MAX_STR_NORMAL, f);
    strcat(buf, Gbuf2);
  }

  /* now try to update the association file, checks on +/- rank are done */
  *smallstr = 0;
  *bigstr = 0;
  while (fgets(Gbuf3, MAX_STR_NORMAL, f))
  {
    sscanf(Gbuf3, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
           &dummy3, &dummy4, &dummy5);
    if (!str_cmp(GET_NAME(small), Gbuf2))
    {
      temp = GET_M_BITS(dummy1, A_RK_MASK);
      if (GET_M_BITS(sbits, A_RK_MASK) != temp)
      {
        update_member(small, 1);
        return (0);
      }
      temp += A_RK1;
      SET_M_BITS(dummy1, A_RK_MASK, temp);
      sprintf(smallstr, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
              dummy3, dummy4, dummy5);
    }
    else if (!str_cmp(GET_NAME(big), Gbuf2))
    {
      temp = GET_M_BITS(dummy1, A_RK_MASK);
      if (GET_M_BITS(bbits, A_RK_MASK) != temp)
      {
        update_member(big, 1);
        return (0);
      }
      temp -= A_RK1;
      SET_M_BITS(dummy1, A_RK_MASK, temp);
      sprintf(bigstr, "%s %u %i %i %i %i\n", Gbuf2, dummy1, dummy2,
              dummy3, dummy4, dummy5);
    }
    else
      strcat(buf, Gbuf3);
  }
  if (!smallstr)
  {
    update_member(small, 1);
    return (0);
  }
  if (!bigstr)
  {
    update_member(big, 1);
    return (0);
  }
  /* ok, found them and everything was ok... */
  strcat(buf, smallstr);
  strcat(buf, bigstr);

  /* now write the file with changes */
  f = fopen(Gbuf1, "w");
  if (!f)
  {
    update_member(small, 1);
    update_member(big, 1);
    return (-1);
  }
  fwrite(buf, strlen(buf), 1, f);
  fclose(f);

  /* now we can notify them of what has happened */
  sprintf(Gbuf2, "Hah! %s kotows before you, accepting your superiority!\r\n",
          GET_NAME(big));
  send_to_char(Gbuf2, small);
  sprintf(Gbuf2, "You kotow before %s, trying to save your sad life!\r\n",
          GET_NAME(small));
  send_to_char(Gbuf2, big);
  /* give some exp based on humiliation, 2% of big's exp */
  update_member(small, 1);
  update_member(big, 1);
  return (1);
}




/* char leader declares char enemy as enemy of the association
   enemy is _not_ set up as member and members can not
   be declared as enemy by this (use punish), success returns
   1, no success 0 and file problems -1 */

int ostracize_enemy(P_char leader, char *enemy)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     asc_bits, dummy1;
  ush_int  asc_number;

  /* checks on leader */
  dummy1 = GET_A_BITS(leader);
  asc_number = GET_A_NUM(leader);
  /* only members can do this */
  if (!IS_MEMBER(dummy1) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", leader);
    return (0);
  }
  /* idiotic calls */
  if (!str_cmp(GET_NAME(leader), enemy) || !str_cmp("me", enemy))
  {
    send_to_char("Do you hate yourself that much?\r\n", leader);
    return (0);
  }
  if (!str_cmp("all", enemy))
  {
    send_to_char("Yeah, everybody hates you, too!\r\n", leader);
    return (0);
  }
  if (isname(enemy, "stravag"))
  {
    send_to_char
      ("Why would you want to do that, he'll just come down and beat you senseless.\r\n",
       leader);
    return 0;
  }

  /* only leaders or higher can declare enemies */
  if (!GT_DEPUTY(dummy1))
  {
    send_to_char("That's a job for the top!\r\n", leader);
    return (0);
  }
  /* people with debts can't do this */
  if (IS_DEBT(dummy1) && !IS_TRUSTED(leader))
  {
    send_to_char("Pay your dues! Let's see some cash...\r\n", leader);
    return (0);
  }
  /* names aren't too long - stop abuse */
  if (strlen(enemy) > 16)
  {
    send_to_char("Very funny. Choose a real name...\r\n", leader);
    return (0);
  }
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(leader, 1);
    return (-1);
  }
  /* goto and read association bits */
  for (i = 0; i < 9; i++)
    fgets(Gbuf2, MAX_STR_NORMAL, f);
  fscanf(f, "%u\n", &asc_bits);
  /* go past cash */
  fgets(Gbuf2, MAX_STR_NORMAL, f);

  /* control if enemy isn't in list already or too many enemies */
  i = 0;
  while (fscanf(f, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
                &dummy3, &dummy4, &dummy5) != EOF)
  {
    if (!str_cmp(enemy, Gbuf2))
    {
      fclose(f);
      send_to_char("That person is in the books already.\r\n", leader);
      return (0);
    }
    if (IS_ENEMY(dummy1))
      i++;
  }
  fclose(f);

  if (i >= MAX_ENEMIES)
  {
    send_to_char("Your have enough enemies already!\r\n", leader);
    return (0);
  }
  /* write enemy at end of list */
  f = fopen(Gbuf1, "a");
  if (!f)
  {
    update_member(leader, 1);
    return (-1);
  }
  dummy1 = asc_bits;
  SET_NO_THANKS(dummy1);
  SET_ENEMY(dummy1);
  fprintf(f, "%s %u %i %i %i %i\n", enemy, dummy1, 0, 0, 0, 0);
  fclose(f);

  send_to_char("The ostracism is in the books...\r\n", leader);

  return (i);
}


/* retitle a member, but only if yer a guild leader */
void retitle_member(P_char member, P_char titlee, char *title)
{
  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];

/*  char asc_name[MAX_STR_ASC];*/
  char    *new_title;

/*  int i;*/
  uint /*asc_bits, */ dummy1;
  ush_int  asc_number;

  /* checks on member */
  dummy1 = GET_A_BITS(member);
  asc_number = GET_A_NUM(member);

  if (!IS_TRUSTED(member) && IS_HIDDENTITLE(GET_A_BITS(titlee)))
  {
    send_to_char("Their association is flagged no title, but you set on anyways.\r\n", member);
    return;
  }

  /* only members can do this */
  if (!(IS_MEMBER(dummy1)) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", member);
    return;
  }
  /* only seniors or higher can set new titles */
  if (!IS_GOD(dummy1) && (IS_DEPUTY(dummy1) || IS_OFFICER(dummy1)))
/*!IS_LEADER(dummy1) &&
                                                   (!((titlee == member) && (IS_DEPUTY(dummy1) || IS_OFFICER(dummy1))))) */
  {
    send_to_char("Maybe one day you will be trusted enough to do that.\r\n",
                 member);
    return;
  }
  if (GET_A_NUM(titlee) != asc_number)
  {
    send_to_char
      ("Trying to change the title of someone not in your assocation, eh?\r\n",
       member);
    return;
  }

  if (!IS_TRUSTED(member) && (GET_A_BITS(titlee) & A_STATICTITLE))
  {
    send_to_char
      ("Their 'no title modification' flag is on - no title change allowed.\r\n",
       member);
    return;
  }

  if (!IS_TRUSTED(member) && (GET_A_BITS(titlee) & A_HIDESUBTITLE))
  {
    send_to_char("Your guild is not allowed to use sub titles.\r\n", member);
    return;
  }

  if (strlen(title) >= MAX_STR_RANK)
  {
    title[MAX_STR_RANK - 1] = '\0';
  }

  if( !is_valid_ansi_with_msg(titlee, title, FALSE) )
  {
     return;
  }
  
  /* get assoc name */

  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    update_member(member, 1);
    return;
  }
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  fclose(f);

  Gbuf2[strlen(Gbuf2) - 1] = '\0';

  /* construct and add new title */
  strcat(title, Gbuf2);
  new_title = title_member(titlee, title);
  if (new_title)
  {
    strcpy(Gbuf1, "You are now ");
    strcat(Gbuf1, title);
    strcat(Gbuf1, ".\r\n");
    send_to_char(Gbuf1, titlee);
  }
  send_to_char("Retitled.\r\n", member);

  if (!(GET_A_BITS(titlee) & A_STATICTITLE))
  {
    send_to_char("\r\n"
                 "&+WWarning:&n Without your 'no title mod' toggle on, your title will be\r\n"
                 "         reset when you next rent/camp.  Type 'soc g' to toggle it.\r\n",
                 titlee);
  }

  send_to_char("\r\n"
               "&+WNOTE:&n An inappropriate title (making your guild name appear to be\r\n"
               "something else, 'offensive' language, etc) will likely cause you to be\r\n"
               "kicked from your association.\r\n", member);
}


/* utility stuff */


/* this converts a string to money */

void str_to_money(char *string, int *pc, int *gc, int *sc, int *cc)
{

  char     num[MAX_INPUT_LENGTH], coin[MAX_INPUT_LENGTH],
    rest[MAX_INPUT_LENGTH];
  char     sign;

  *pc = *gc = *sc = *cc = 0;
  if (!*string)
    return;
  half_chop(string, num, rest);
  if (!*rest)
    return;
  do
  {
    half_chop(rest, coin, rest);
    sign = LOWER(*coin);
    switch (sign)
    {
    case 'p':
      *pc = atoi(num);
      break;
    case 'g':
      *gc = atoi(num);
      break;
    case 's':
      *sc = atoi(num);
      break;
    case 'c':
      *cc = atoi(num);
      break;
    default:
      return;
    }
    if (!*rest)
      return;
    half_chop(rest, num, rest);
  }
  while (*rest);
  return;
}

/* this titles a member, called with 0 title it just deletes the
   current title, returns NULL if no title is set, pointer to it
   otherwise. Limits title length to 39-(length of char name)
   letters (not counting ansi) */

char    *title_member(P_char member, char *title)
{
  char    *test;
  int      length, upper;

  if (!member)
    return (NULL);

  if (GET_A_BITS(member) & A_HIDETITLE)
    return GET_TITLE(member);
  if (GET_TITLE(member) && (GET_A_BITS(member) & A_STATICTITLE) &&
      !(GET_A_BITS(member) & A_HIDESUBTITLE))
    return GET_TITLE(member);

  /* remove old title */
  if (GET_TITLE(member))
  {
/*
    FREE(GET_TITLE(member));
    GET_TITLE(member) = NULL;
*/
    set_title(member);
  }
  if (*title)
  {
    upper = 41 - strlen(GET_NAME(member));
    test = title;
    length = 0;
    while (*test && (length < upper))
      /* check for ansi */
      if (*test == '&')
        switch (*(test + 1))
        {
        case 'n':
        case 'N':
          test += 2;
          break;
        case '+':
        case '-':
          switch (LOWER(*(test + 2)))
          {
          case 'w':
          case 'l':
          case 'r':
          case 'b':
          case 'g':
          case 'y':
          case 'c':
          case 'm':
            test += 3;
            break;
          default:
            test += 2;
            length += 2;
            break;
          }
          break;
        case '=':
          switch (LOWER(*(test + 2)))
          {
          case 'w':
          case 'l':
          case 'r':
          case 'b':
          case 'g':
          case 'y':
          case 'c':
          case 'm':
            switch (LOWER(*(test + 3)))
            {
            case 'w':
            case 'l':
            case 'r':
            case 'b':
            case 'g':
            case 'y':
            case 'c':
            case 'm':
              test += 4;
              break;
            default:
              test += 3;
              length += 3;
              break;
            }
            break;
          default:
            test += 2;
            length += 2;
            break;
          }
          break;
        default:
          test++;
          length++;
          break;
        }
      else
      {
        test++;
        length++;
      }

    /* end the string with a &n to set color back to normal */
    if (length < upper)
      strcpy(test, "&n");
    else
      strcpy(test - 2, "&n");

    /* now set the new title */
    GET_TITLE(member) = str_dup(title);
    return (GET_TITLE(member));
  }
  else
  {
    return (NULL);
  }
}

/* finds out if char is enemy of association number asc_number,
   returns 1 if yes, 0 otherwise */
int find_enemy(P_char enemy, ush_int asc_number)
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  ush_int  enemy_num;
  int      i, dummy2, dummy3, dummy4, dummy5;
  uint     dummy1;

  /* protect gods and non-player chars */
  if (IS_TRUSTED(enemy) || IS_NPC(enemy))
    return (0);

  enemy_num = GET_A_NUM(enemy);
  /* maybe it's obvious: he is a member and has rank enemy */
  if ((GET_A_NUM(enemy) == asc_number) && (IS_ENEMY(GET_A_BITS(enemy))))
    return (1);

  /* maybe the guy seceded, but is ostracized */
  /* open association file */
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, asc_number);
  f = fopen(Gbuf1, "r");
  if (!f)
  {
    return (0);
  }
  /* goto past cash */
  for (i = 0; i < 11; i++)
    fgets(Gbuf2, MAX_STR_NORMAL, f);

  /* control if enemy */
  while (fscanf(f, "%s %u %i %i %i %i\n", Gbuf2, &dummy1, &dummy2,
                &dummy3, &dummy4, &dummy5) != EOF)
  {
    if (!str_cmp(GET_NAME(enemy), Gbuf2) && IS_ENEMY(dummy1))
    {
      fclose(f);
      return (1);
    }
  }
  fclose(f);

  /* not found, no problem */
  return (0);
}

P_char get_guild_leader(int asc1)
{
  /* returns guild leader of guild # asc1 */
  FILE    *f;
  char     buf[MAX_STRING_LENGTH];
  uint     asc_bits, t_bits;
  int      i;
  char     buf2[MAX_STRING_LENGTH];
  P_char   temp;

  sprintf(buf, "%sasc.%u", ASC_DIR, asc1);
  f = fopen(buf, "r");
  if (!f)
    return 0;
  fgets(buf, MAX_STR_NORMAL, f);        /* name of guild */
  for (i = 0; i < 8; i++)       /* titles */
    fgets(buf, MAX_STR_NORMAL, f);
  fscanf(f, "%u\n", &asc_bits);
  fscanf(f, "%i %i %i %i\n", &i, &i, &i, &i);
  i = 0;
  while (fgets(buf, MAX_STR_NORMAL, f))
  {
    sscanf(buf, "%s %u %i %i %i %i", buf2, &asc_bits, &i, &i, &i, &i);
    t_bits = GET_M_BITS(asc_bits, A_RK_MASK);
    if (IS_LEADER(t_bits))
    {
       fclose(f);
       return get_char_online(buf2);
    }
  }

  fclose(f);
  return 0;
}

void do_gmotd(P_char ch, char *argument, int cmd)
{
  FILE    *f;
  int      assoc;
  P_obj    obj;
  uint     bits, temp;
  char    *text;
  char     buf[MAX_STRING_LENGTH];

  if (!ch)
  {
    return;
  }
  bits = GET_A_BITS(ch);
  if (!IS_MEMBER(bits))
  {
    send_to_char("You are not part of an association!\r\n", ch);
    return;
  }
  assoc = GET_A_NUM(ch);
  sprintf(buf, "%sasc.%d.motd", ASC_DIR, assoc);

  if (!*argument || (!IS_LEADER(bits) && !GT_LEADER(bits) && !IS_TRUSTED(ch)))
  {
    f = fopen(buf, "r");
    if (!f)
    {
      send_to_char("No guild motd.\r\n", ch);
      return;
    }
    fclose(f);
    text = file_to_string(buf);
    send_to_char("&+C----GUILD MOTD----&N\r\n", ch);
    send_to_char(text, ch);
    send_to_char("&+C------------------&N\r\n", ch);
    return;
  }
  obj = get_obj_in_list_vis(ch, argument, ch->carrying);
  if (!obj)
  {
    send_to_char("Paper not found!\r\n", ch);
    return;
  }
  if (obj->type != ITEM_NOTE)
  {
    send_to_char("Item must be a piece of paper with your guild motd!\r\n",
                 ch);
    return;
  }
  if (!obj->action_description)
  {
    send_to_char("Paper is empty, no empty motd!!\r\n", ch);
    return;
  }
  f = fopen(buf, "w");
  text = str_dup(obj->action_description);
  strcat(text, "\r\n");
  fprintf(f, "%s", text);
  fclose(f);
  obj_from_char(obj, TRUE);
  extract_obj(obj, TRUE);
  send_to_char("Guild Motd Updated.\r\n", ch);
}

#ifdef __NO_MYSQL__
int do_soc_ledger(P_char ch)
{
  send_to_char("disabled.\r\n", ch);
  return 0;
}

void insert_guild_transaction(int soc_id, char *buff)
{
}
#else
int do_soc_ledger(P_char ch)
{
  int asc_number = GET_A_NUM(ch);
  uint bits = GET_A_BITS(ch);

  if (!IS_MEMBER(bits) || !asc_number)
  {
    send_to_char("How about joining an association first?\r\n", ch);
    return 0;
  }
  
  if (!IS_LEADER(bits) && !IS_TRUSTED(ch) )
  {
    send_to_char("You must be a leader to look at such things!\r\n", ch);
    return 0;
  }

  send_to_char("&+YGuild Ledger:\r\n------------------------------\r\n", ch);

  char buff[MAX_STRING_LENGTH];

  if( !qry("SELECT transaction_info FROM guild_transactions WHERE soc_id = %d ORDER BY date DESC LIMIT 100", asc_number) )
  {
    send_to_char("disabled.\r\n", ch);
    return FALSE;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    send_to_char("&+yNo transactions on record.\r\n", ch);
    mysql_free_result(res);
    return FALSE;
  }

  MYSQL_ROW row;

  while( row = mysql_fetch_row(res) )
  {
    sprintf(buff, "%s\r\n", row[0]); 
    send_to_char(buff, ch);
  }

  mysql_free_result(res);
  return TRUE;
}

void insert_guild_transaction(int soc_id, char* buff)
{
  qry("INSERT INTO guild_transactions (date, soc_id, transaction_info) VALUES (unix_timestamp(), %d, '%s')", soc_id, buff); 
}
#endif

#ifdef __NO_MYSQL__

int get_assoc_prestige(int assoc_id)
{
  return 0;
}

int get_assoc_cps(int assoc_id)
{
  return 0;
}

void add_assoc_prestige(int assoc_id, int prestige) 
{
}

void set_assoc_prestige(int assoc_id, int prestige)
{
}

void add_assoc_cps(int assoc_id, int cps) 
{
}

void set_assoc_cps(int assoc_id, int cps)
{  
}

void show_prestige_list(P_char ch)
{
  send_to_char("Disabled.", ch);
}

void reload_assoc_table()
{}

void prestige_update()
{}

string get_assoc_name(int assoc_id)
{
  return string();
}

void do_prestige(P_char ch, char *argument, int cmd)
{}

void set_assoc_active(int assoc_id, bool active)
{
}

void add_assoc_overmax(int assoc_id, int overmax)
{
}

int get_assoc_overmax(int assoc_id)
{
  return 0;
}

#else
void set_assoc_active(int assoc_id, bool active)
{
  qry("UPDATE associations SET active = %d WHERE id = %d", active, (int) assoc_id);
}

int get_assoc_prestige(int assoc_id)
{
  if( !qry("SELECT prestige FROM associations WHERE id = %d AND active = 1", assoc_id) )
  {
    return 0;
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return 0;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);
  
  int prestige = atoi(row[0]);
  mysql_free_result(res);

  return prestige;
}

int get_assoc_cps(int assoc_id)
{
  if( !qry("SELECT construction_points FROM associations WHERE id = %d AND active = 1", assoc_id) )
  {
    return 0;
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return 0;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);
  
  int cps = atoi(row[0]);
  mysql_free_result(res);
  
  return cps;
}

void add_assoc_prestige(int assoc_id, int prestige_delta) 
{
  int prestige = get_assoc_prestige(assoc_id);

  debug("add_assoc_prestige(%d, %d): old prestige=%d", assoc_id, prestige_delta, prestige);
  logit(LOG_DEBUG, "add_assoc_prestige(%d, %d): old prestige=%d", assoc_id, prestige_delta, prestige);
  
  if( (prestige + prestige_delta) < 0 )
  {
    return;
  }
  
  qry("UPDATE associations SET prestige = prestige + %d WHERE id = %d AND active = 1", prestige_delta, assoc_id);
  statuslog(GREATER_G, "Association %s &ngained %d prestige points.", get_assoc_name(assoc_id).c_str(), prestige_delta);
  logit(LOG_STATUS, "Association %s &ngained %d prestige points.", get_assoc_name(assoc_id).c_str(), prestige_delta);
  
  // add construction points  
  int cp_notch_step = get_property("prestige.constructionPoints.notch", 100);  
  int cps_notches = MAX(0, (int) ((prestige+prestige_delta)/cp_notch_step) - (prestige/cp_notch_step));
  
  if( cps_notches )
  {
    statuslog(GREATER_G, "Association %s &ngained %d construction points.", get_assoc_name(assoc_id).c_str(), cps_notches);
    logit(LOG_STATUS, "Association %s &ngained %d construction points.", get_assoc_name(assoc_id).c_str(), cps_notches);
    add_assoc_cps(assoc_id, cps_notches);
  }
  
}

void set_assoc_prestige(int assoc_id, int prestige)
{
  qry("UPDATE associations SET prestige = %d WHERE id = %d AND active = 1", MAX(0, prestige), assoc_id);
}

void add_assoc_cps(int assoc_id, int cps)
{
  qry("UPDATE associations SET construction_points = construction_points + %d WHERE id = %d AND active = 1", cps, assoc_id);
}

void set_assoc_cps(int assoc_id, int cps)
{
  qry("UPDATE associations SET construction_points = %d WHERE id = %d AND active = 1", cps, assoc_id);
}

void do_prestige(P_char ch, char *argument, int cmd)
{
  show_prestige_list(ch);
}

void show_prestige_list(P_char ch)
{
  
  if( !qry("SELECT id, name, prestige, construction_points FROM associations WHERE active = 1 ORDER BY prestige DESC, id asc") )
  {
    send_to_char("Disabled.\r\n", ch);
    return;
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    send_to_char("There are no prestigious associations.\r\n", ch);
    return;
  }
  
  send_to_char("&+bPrestigious Associations\r\n&+W----------------------------------------\n", ch);  
  
  MYSQL_ROW row;
  
  char buff[MAX_STRING_LENGTH];
  while( row = mysql_fetch_row(res) )
  {
    int id = atoi(row[0]);
    string name = pad_ansi(trim(string(row[1]), " \t\n").c_str(), 40);
    int prestige = atoi(row[2]);
    int cps = atoi(row[3]);
    
    if( IS_TRUSTED(ch) )
    {
      sprintf(buff, "&+W%2d. &n%s &n(&+b%d&n:&+W%d&n)\n", id, name.c_str(), prestige, cps);
    }
    else
    {
      if( prestige < (int) get_property("prestige.list.viewThreshold", 500) )
      {
        continue;
      }
      
      sprintf(buff, "%s\n", name.c_str()); 
    }
    
    send_to_char(buff, ch);
  }
  
  mysql_free_result(res);  
}

void reload_assoc_table()
{
  char buf[MAX_STRING_LENGTH];
  int i, j;
  
  for (i = 1, j = 0; i < MAX_ASC; i++)
  {
    sprintf(buf, "%sasc.%d", ASC_DIR, i);
    
    FILE *f = fopen(buf, "r");
    if (f)
    {
      fgets(buf, MAX_STR_NORMAL, f);
      
      if( !qry("SELECT id, name FROM associations WHERE id = %d", i) )
      {
        logit(LOG_DEBUG, "Query failed in reload_assoc_table()");
        break;
      }

      string name = escape_str(buf);
      MYSQL_RES *res = mysql_store_result(DB);
      if( mysql_num_rows(res) < 1 )
      {
        qry("INSERT INTO associations (id, name, active) VALUES (%d, '%s', 1)", i, name.c_str());
      }
      else
      {
        qry("UPDATE associations SET name = '%s' WHERE id = %d", name.c_str(), i);
      }
      
      mysql_free_result(res);
            
      fclose(f);
      j = 0;
    }
    else
    {
      qry("UPDATE associations SET active = 0 WHERE id = %d", i);
      j++;
    }

    if (j > 20)
      break;
  }
    
}

// DEPRECATED - prestige is no longer automatically subtracted
void prestige_update()
{
  return;
  struct alliance_data *alliance;

  if( !has_elapsed("prestige_update", (int) get_property("prestige.update.secs", 60) ) )
  {
    return;    
  }
  
  if( !qry("SELECT id, prestige FROM associations"))
    return;

  MYSQL_RES *res = mysql_store_result(DB);
  MYSQL_ROW row;

  int kingdom_prestige = (int) get_property("prestige.kingdom.required", 400000);
  int guild_prestige = (int) get_property("prestige.guild.required", 200000);

  while( row = mysql_fetch_row(res) )
  {
    int id = atoi(row[0]);
    int prestige = atoi(row[1]);
    int new_prestige = prestige;
                alliance = get_alliance(id); 

    if( prestige > kingdom_prestige)
    {
      new_prestige = MAX(kingdom_prestige, prestige - (int) get_property("prestige.update.decrease.kingdom", 200));
    }
    else if( prestige > guild_prestige )
    {
      new_prestige = MAX(guild_prestige, prestige - (int) get_property("prestige.update.decrease.guild", 100));
    }
    else
    {
// clans don't lose prestige over time
      continue;
    }
    
// secondary guild's in alliances don't lose prestige
    if (alliance && (id == alliance->joining_assoc_id))
    {
      new_prestige = (int)(new_prestige * (1. - get_property("prestige.gain.passive", .300)));
    } 
    
    qry("UPDATE associations SET prestige = %d WHERE id = %d", new_prestige, id);
  }

  mysql_free_result(res);

  set_timer("prestige_update");
}

string get_assoc_name(int assoc_id)
{
  if( !qry("SELECT name FROM associations WHERE active = 1 AND id = %d", assoc_id) )
  {
    return string();
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return string();
  }
    
  MYSQL_ROW row = mysql_fetch_row(res);
  
  string name = trim(string(row[0]), " \t\n");
  mysql_free_result(res);
  
  return name;
}

#endif

void check_assoc_prestige_epics(P_char ch, int epics, int epic_type)
{
  if( !IS_PC(ch) || !GET_A_NUM(ch) || !ch->group || (epics < (int) get_property("prestige.epicsMinimum", 4.000)) )
    return;

  int assoc_members = 1;

  // Count group members in same guild
  for( struct group_list *gl = ch->group; gl; gl = gl->next )
  {
    if( ch != gl->ch && IS_PC(gl->ch) && ch->in_room == gl->ch->in_room)
    {
      if (GET_A_NUM(gl->ch) == GET_A_NUM(ch))
      {
        assoc_members++;
      }
    }
  }
  
  debug("check_assoc_prestige_epics(): assoc_members: %d, epics: %d, epic_type: %d", assoc_members, epics, epic_type);
  
  // If members in group are above 3...
  if( assoc_members >= (int) get_property("prestige.guildedInGroupMinimum", 3.000) )
  {
    int prestige = 0;

    switch( epic_type )
    {
      case EPIC_PVP:
      case EPIC_SHIP_PVP:
        prestige = (int) get_property("prestige.gain.pvp", 20);
        break;

      case EPIC_ZONE:
      case EPIC_QUEST:
      case EPIC_RANDOM_ZONE:
      case EPIC_NEXUS_STONE:
      default:
        prestige = (int) get_property("prestige.gain.default", 10);
    }    
    
    prestige = check_nexus_bonus(ch, prestige, NEXUS_BONUS_PRESTIGE);

    debug("check_assoc_prestige_epics(): gain: %d", prestige);

    send_to_char("&+bYour guild gained prestige!\r\n", ch);
    add_assoc_prestige( GET_A_NUM(ch), prestige);
  }
}
//
//bool is_clan(int asc_number)
//{
//  int prestige = get_assoc_prestige(asc_number);  
//
//  if( prestige < (int) get_property("prestige.guild.required", 2000) )
//  {
//    return TRUE;
//  }
//  
//  return FALSE;  
//}
//
//bool is_guild(int asc_number)
//{
//  int prestige = get_assoc_prestige(asc_number);  
//  
//  if( prestige >= (int) get_property("prestige.guild.required", 2000) &&
//      prestige < (int) get_property("prestige.kingdom.required", 4000) )
//  {
//    return TRUE;
//  }
//  
//  return FALSE;   
//}
//
//bool is_kingdom(int asc_number)
//{
//  int prestige = get_assoc_prestige(asc_number);  
//  
//  if( prestige >= (int) get_property("prestige.kingdom.required", 2000) )
//  {
//    return TRUE;
//  }
//  
//  return FALSE;    
//}

void add_assoc_overmax(int assoc_id, int overmax)
{
  qry("UPDATE associations SET over_max = over_max + %d WHERE id = %d AND active = 1", overmax, assoc_id);
}

int get_assoc_overmax(int asc_number)
{
  if( !qry("SELECT over_max FROM associations where id = %d", asc_number))
    return 0;

  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return 0;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);
  
  int overmax = atoi(row[0]);
  mysql_free_result(res);
  
  return overmax;
}

int max_assoc_size(int asc_number)
{  
  int prestige = get_assoc_prestige(asc_number);
  int base_size = get_property("guild.size.base", 0);
  int max_size = get_property("guild.size.max", 0);
  int step_size = get_property("guild.size.prestige.step", 0);  
  int members = base_size + (int) ( (float) ( MAX(0, prestige) ) / (float) MAX(1, step_size) );

  members = MIN(members, max_size);

  return (members + get_assoc_overmax(asc_number));
}

void get_assoc_name(int assoc, char *buf)
{
  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char    *p;

  *buf = '\0';
  sprintf(Gbuf1, "%sasc.%u", ASC_DIR, assoc);
  f = fopen(Gbuf1, "r");
  if (!f)
    return;
  fgets(Gbuf2, MAX_STR_NORMAL, f);
  p = striplinefeed(Gbuf2);
  strcat(buf, p);
  str_free(p);
  fclose(f);
}

// old guildhalls (deprecated)
//int is_in_own_guild(P_char member)
//{
//  P_house  house = NULL;
//
//  if (IS_APPLICANT(GET_A_BITS(member)))
//    return FALSE;
//  house = house_ch_is_in(member);
//  if (!house)
//    return FALSE;
//  if ((GET_A_NUM(member) != house->owner_guild) && (!IS_TRUSTED(member)) &&
//      (house->type == HCONTROL_GUILD))
//    return FALSE;
//  return TRUE;
//  return FALSE;
//}

//Init guild frags
void init_guild_frags()
{

  FILE    *f;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STR_NORMAL];
  char     Gbuf3[MAX_STR_NORMAL];
  char     Gbuf4[MAX_STR_NORMAL];
  char     ostra[MAX_STR_NORMAL];
  char     buf[MAX_STRING_LENGTH];
  char     rank_names[8][MAX_STR_RANK];
  char     name[MAX_STR_NORMAL];
  char     guild_name[40];
  const char *standard_names[] = { "enemy", "parole", "normal", "senior",
    "officer", "deputy", "leader", "king"
  };
  int      i, ii, j, k, os, flag, dummy2, dummy3, dummy4, dummy5;
  ush_int  asc_number;
  uint     dummy1, temp, asc_bits;
  struct sortit
  {
    int      ranknr;
    char     info[MAX_STR_NORMAL];
  } displayed[MAX_DISPLAY];
  P_char   target = NULL;

  wizlog(56, "creating guild fraglist....");
  memset(guild_frags_data, 0, sizeof(guild_frags_data));
  valid_assocs = 0;
  guild_frags_initialized = TRUE;

  for (i = 1, j = 0; i < MAX_ASC; i++)
  {
    sprintf(Gbuf1, "%sasc.%u", ASC_DIR, i);
    f = fopen(Gbuf1, "r");
    if (f)
    {
      guild_frags_data[valid_assocs].frags = 0;
      guild_frags_data[valid_assocs].top_frags = 0;
      sprintf(guild_frags_data[valid_assocs].g_topfragger, "%s", "Noone");

      fgets(Gbuf2, MAX_STR_NORMAL, f);
      os = strlen(Gbuf2);
      Gbuf2[os - 1] = '\0';
      os = 0;
      sprintf(guild_frags_data[valid_assocs].g_name, "%s", Gbuf2);

      for (ii = 0; ii < 8; ii++)
      {
        fgets(Gbuf2, MAX_STR_NORMAL, f);
      }
      fscanf(f, "%u\n", &asc_bits);
      fscanf(f, "%i %i %i %i\n", &dummy2, &dummy3, &dummy4, &dummy5);
      os = 0;
      while (fgets(Gbuf3, MAX_STR_NORMAL, f) && i < MAX_DISPLAY)
      {
        sscanf(Gbuf3, "%s %u %i %i %i %i\r\n", Gbuf2, &dummy1, &dummy2,
               &dummy3, &dummy4, &dummy5);
        /* check for ostracized people */
        if (!IS_ENEMY(dummy1) || IS_MEMBER(dummy1))
        {
          target = (struct char_data *) mm_get(dead_mob_pool);
          target->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
          if (restoreCharOnly(target, skip_spaces(Gbuf2)) >= 0)
          {
            if (target)
            {
              os++;
              guild_frags_data[valid_assocs].members = os;
              guild_frags_data[valid_assocs].frags =
                guild_frags_data[valid_assocs].frags + target->only.pc->frags;
              if (target->only.pc->frags >
                  guild_frags_data[valid_assocs].top_frags)
              {
                guild_frags_data[valid_assocs].top_frags =
                  target->only.pc->frags;
                sprintf(guild_frags_data[valid_assocs].g_topfragger, "%s",
                        GET_NAME(target));
              }
            }
          }
          if (target != NULL)
          {
            free_char(target);
            target = NULL;
          }
        }
      }
      //create a file for frags
      //      sprintf(Gbuf4, "%s%d_frags", ASC_DIR, i);
      //      f = fopen(Gbuf4, "w");
      //      fprintf(f, "%s\n%d", guild_frags_data[i].g_name , (int) guild_frags_data[i].frags );
      //      fclose(f);
      valid_assocs++;
    }
  }
  /* now send to char through pager */
  wizlog(56, "Generated guild frag list");
  return;
}// init_guild_frags

//Frags stuff here
void display_guild_frags(P_char god)
{
  FILE    *f;
  FILE    *f2;
  char     Gbuf1[MAX_STR_NORMAL];
  char     Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STR_NORMAL];
  char     buf[MAX_STRING_LENGTH];
  char    *buf_temp = 0;

  int      i, j, k;
  int      found = 0;

  struct guild_frags temp[20];

  if (!guild_frags_initialized)
    init_guild_frags();

  if(1)// (!guild_frags)
  {

    /* title of list */
    strcpy(buf, "\t&+L      -=&+W Top Guilds &+L=-\r\n"
           "    &+r---------------------------------------\r\n");


    for (i = valid_assocs - 1; i >= 0; i--)
    {
      for (j = 0; j < i; ++j)
      {
        if (guild_frags_data[j].frags > guild_frags_data[j + 1].frags)
        {
          temp[10] = guild_frags_data[j];
          guild_frags_data[j] = guild_frags_data[j + 1];
          guild_frags_data[j + 1] = temp[10];
        }

      }
    }

    i = valid_assocs - 1;
    while (i >= 0)
    {
      sprintf(Gbuf2, "\t%s\r\n", guild_frags_data[i].g_name);
      strcat(buf, Gbuf2);
//        send_to_char(Gbuf2, god);
      sprintf(Gbuf2, "\t&+WTotal Guild Frags: \t&+Y%+.2f&n\r\n",
              (float) (guild_frags_data[i].frags / 100.00));
      strcat(buf, Gbuf2);

      //send_to_char(Gbuf2, god);
      sprintf(Gbuf2, "\t&+WTop Fragger: &+Y%-10s %+.2f&n\r\n",
              guild_frags_data[i].g_topfragger,
              (float) (guild_frags_data[i].top_frags / 100.00));
      strcat(buf, Gbuf2);
      // send_to_char(Gbuf2, god);
      sprintf(Gbuf2, "\t&+WMembers:&+Y%2d&+W frags/member:&+Y%+.2f&n\r\n",
              guild_frags_data[i].members,
              (float) ((guild_frags_data[i].frags / 100.00) /
                       guild_frags_data[i].members));
      //send_to_char(Gbuf2, god);
      strcat(buf, Gbuf2);

      strcat(buf, "    &+L---------------------------------------\r\n");
      i--;
    }

    f = fopen(GUILD_FRAG_FILE, "w+");
    fprintf(f, "%s", buf);
    fclose(f);

    buf_temp = file_to_string(GUILD_FRAG_FILE);
//    guild_frags = buf_temp;
  }
  
  page_string(god->desc, buf_temp, 0);

  return;
}// display_guild_frags

