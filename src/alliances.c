#include <string.h>
#include <stdio.h>

#include <vector>
using namespace std;

#include "comm.h"
#include "structs.h"
#include "utils.h"
#include "assocs.h"
#include "prototypes.h"
#include "utils.h"
#include "alliances.h"
#include "sql.h"
#include "spells.h"

extern P_desc descriptor_list;

vector<struct alliance_data> alliances;

const char *ALLIANCE_FORMAT =
  "\r\n"
  " &+BAlliance syntax\n"
  " &+B-----------------------------\n"
  " &+bForging guild leader:    &+Walliance propose <leader of second guild>\n"
  " &+bLeader of joining guild: &+Walliance accept  <proposing leader>\n\n"
  " &+bRevoking proposal:       &+Walliance revoke\n\n"
  " &+bSevering:                &+Walliance sever confirm\n";


bool is_allied_with(int assoc_id1, int assoc_id2)
{
  struct alliance_data *alliance = get_alliance(assoc_id1);
  
  if( !alliance )
    return FALSE;
  
  if( IS_FORGING_ASSOC(alliance, assoc_id1) && 
      alliance->joining_assoc_id == assoc_id2)
    return TRUE;
  
  if( IS_JOINING_ASSOC(alliance, assoc_id1) && 
      alliance->forging_assoc_id == assoc_id2)
    return TRUE;
  
  return FALSE;  
}

void load_alliances()
{
  // search through DB, creating alliances in memory 
  
  while( alliances.size() )
  {
    alliances.pop_back();
  }
  
#ifdef __NO_MYSQL__
  return;
#else
  if( !qry("SELECT forging_assoc_id, joining_assoc_id FROM alliances") )
  {
    return;    
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  MYSQL_ROW row;
  
  struct alliance_data alliance;
  while( row = mysql_fetch_row(res) )
  {
    alliance.forging_assoc_id = atoi(row[0]);
    alliance.joining_assoc_id = atoi(row[1]);
    alliances.push_back(alliance);
  }
  
  logit(LOG_STATUS, "Alliances loaded from database.");
  
#endif //__NO_MYSQL__
}

void save_alliances()
{
  // save the current alliance list to database

#ifdef __NO_MYSQL__
  return;
#else
  
  if( !qry("DELETE FROM alliances") )
    return;
  
  for( int i = 0; i < alliances.size(); i++ )
  {
    qry("INSERT INTO alliances (forging_assoc_id, joining_assoc_id) VALUES ('%d', '%d')",
        alliances[i].forging_assoc_id, alliances[i].joining_assoc_id);
  }
  
#endif // __NO_MYSQL__
}

alliance_data *get_alliance(int assoc_id)
{
  for( int i = 0; i < alliances.size(); i++ )
  {
    if( assoc_id == alliances[i].forging_assoc_id ||
        assoc_id == alliances[i].joining_assoc_id )
    {
      return &alliances[i];
    }
  }
  return NULL;  
}

void forge_alliance(int forging_assoc_id, int joining_assoc_id)
{
  struct alliance_data new_alliance;
  new_alliance.forging_assoc_id = forging_assoc_id;
  new_alliance.joining_assoc_id = joining_assoc_id;
  
  alliances.push_back(new_alliance);
  save_alliances();

  string forging_name = get_assoc_name(forging_assoc_id);
  string joining_name = get_assoc_name(joining_assoc_id);

  char buff[MAX_STRING_LENGTH];
  sprintf(buff, "%s &+band &n%s &+bhave formed an alliance!\n", forging_name.c_str(), joining_name.c_str());
  send_to_alliance(buff, forging_assoc_id);
}

void sever_alliance(int assoc_id)
{
  vector<struct alliance_data>::iterator it;

  for( it = alliances.begin(); it != alliances.end(); it++ )
  {
    if( it->forging_assoc_id == assoc_id || 
        it->joining_assoc_id == assoc_id )
      break;
  }
  
  if( it == alliances.end() )
    return;  

  string severing_name = get_assoc_name(assoc_id);
  string other_name;
  
  if( it->forging_assoc_id == assoc_id )
  {
    other_name = get_assoc_name(it->joining_assoc_id);
  }
  else
  {
    other_name = get_assoc_name(it->forging_assoc_id);
  }
    
  char buff[MAX_STRING_LENGTH];
  
  sprintf(buff, "%s &+bhave severed their alliance with &n%s&+b!\n", severing_name.c_str(), other_name.c_str());
  send_to_alliance(buff, assoc_id);
  
  alliances.erase(it);
  
  save_alliances();
}



void do_alliance(P_char ch, char *arg, int cmd)
{
  char buff[MAX_STRING_LENGTH];
  int  prestige1 = 0, prestige2 = 0;

  // Lom: ok this for now god only command, once all testings passes and more stuff added
  //      then we can make it for mortals too
  //if( !IS_TRUSTED(ch) )
  //{
  //  send_to_char("Huh?\n", ch);
  //  return;
  //}
  
  // check that char is in guild and leader
  // check for command
  // check for target

  uint abits = GET_A_BITS(ch);
  ush_int assoc_id = GET_A_NUM(ch);

  if( !IS_MEMBER(abits) || !assoc_id )
  {
    send_to_char("&+bYou cannot forge alliances on your own!\n", ch);
    return;
  }

  if( !IS_LEADER(abits) && !IS_GOD(abits)) //&& !IS_TRUSTED(ch) )
  {
    send_to_char("&+bOnly guild leaders can forge or sever alliances!\n", ch);
    return;
  }

  char sub_command_str[MAX_INPUT_LENGTH], victim_str[MAX_INPUT_LENGTH];
  
  argument_interpreter(arg, sub_command_str, victim_str);
    
  if( strlen(sub_command_str) < 1 )
  {
    // check for existing alliance
    struct alliance_data *alliance = get_alliance(assoc_id);
    
    if( !alliance )
    {
      send_to_char("&+bYour association is not allied with anyone.\n", ch);
      return;      
    }
    
    string name;    
    if( IS_FORGING_ASSOC(alliance, assoc_id) )
    {    
      name = get_assoc_name(alliance->joining_assoc_id);
    }
    else
    {
      name = get_assoc_name(alliance->forging_assoc_id);
    }
        
    sprintf(buff, "&+bYou are allied with &n%s&+b.\n", name.c_str());
    send_to_char(buff, ch);
    return;
  }
  else if( !strcmp(sub_command_str, "propose") )
  {
    struct alliance_data *alliance = get_alliance(assoc_id);
    
    if( alliance )
    {
      send_to_char("&+bYour association has already forged an alliance!\n", ch);
      return;
    }
    
    P_char victim = get_char_room_vis(ch, victim_str);
    
    if( !victim || victim == ch || !IS_PC(victim) )
    {
      send_to_char("&+bYou can only forge an alliance with the leader of another association.\n", ch);
      return;
    }

    if (racewar(ch, victim))
    {
      send_to_char("&+bYou don't want an alliance with scum!\n", ch);
      return;
    }

    uint abits2 = GET_A_BITS(victim);
    ush_int assoc_id2 = GET_A_NUM(victim);
    
    if( !IS_MEMBER(abits2) || !assoc_id2 || !IS_LEADER(abits2) )
    {
      send_to_char("&+bYou can only forge an alliance with the leader of another association.\n", ch);
      return;
    }

    //----------------------------
    //valid to propose?
    //----------------------------
    prestige1 = get_assoc_prestige(assoc_id);
    prestige2 = get_assoc_prestige(assoc_id2);

    if(prestige1 < get_property("prestige.alliance.required", 0))
    {
       send_to_char("&+bYour association is not famous enough to forge an alliance.\n", ch);
       return;
    }
    // For now we are letting any guild be a secondary guild... -Venthix
    //if(prestige2 < needed_prestige)
    //{
    //   send_to_char("&+bAlliance with unknown? No way!\n", ch);
    //   return;
    //}
    //----------------------------

    // forge it!
    struct affected_type af;
    memset(&af, 0, sizeof(af));
           
    af.type = TAG_ALLIANCE;
    af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
    af.duration = 60 * WAIT_SEC;
    af.modifier = GET_PID(victim);
    affect_to_char(ch, &af);
           
    act("&+b$n &+bhas proposed an alliance with your association!", FALSE, ch, 0, victim, TO_VICT);
    act("&+bYou propose an alliance with $N&+b's association!", FALSE, ch, 0, victim, TO_CHAR);    
  }
  else if( !strcmp(sub_command_str, "accept") )
  {
    struct alliance_data *alliance = get_alliance(assoc_id);
    
    if( alliance )
    {
      send_to_char("&+bYour association has already forged an alliance!\n", ch);
      return;
    }
    
    P_char victim = get_char_room_vis(ch, victim_str);
    
    if( !victim || victim == ch || !IS_PC(victim) )
    {
      send_to_char("&+bYou can only forge an alliance with the leader of another association.\n", ch);
      return;
    }

    uint abits2 = GET_A_BITS(victim);
    ush_int assoc_id2 = GET_A_NUM(victim);
    
    if( !IS_MEMBER(abits2) || !assoc_id2 || ( !IS_LEADER(abits2) && !IS_TRUSTED(victim) ) )
    {
      send_to_char("&+bYou can only forge an alliance with the leader of another association.\n", ch);
      return;
    }    
    
    struct affected_type *af;
    
    if( !(af = get_spell_from_char(victim, TAG_ALLIANCE)) || af->modifier != GET_PID(ch) )
    {
      send_to_char("&+bYou can only accept a proposal after it has been proposed!\n", ch);
      return;
    }

    affect_remove(victim, af);
    
    forge_alliance( assoc_id2, assoc_id );
  }
  else if( !strcmp(sub_command_str, "revoke") )
  {
    struct affected_type *af;
    
    if( !(af = get_spell_from_char(ch, TAG_ALLIANCE)) )
    {
      send_to_char("&+bYou haven't proposed any alliances!\n", ch);
      return;
    }
    
    send_to_char("&+bYou revoke your alliance proposal.\n", ch);
    affect_remove(ch, af);
  }
  else if( !strcmp(sub_command_str, "sever") && !strcmp(victim_str, "confirm") )
  {
    struct alliance_data *alliance = get_alliance(assoc_id);
    
    if( !alliance )
    {
      send_to_char("&+bYour association has not forged an alliance.\n", ch);
      return;      
    }
    
    sever_alliance(assoc_id);
  }
  else
  {
    // help message
    send_to_char(ALLIANCE_FORMAT, ch);
  }
}

void send_to_alliance(char *str, int assoc_id)
{
  struct alliance_data *alliance = get_alliance(assoc_id);
  
  if( !alliance )
    return;
  
  for( P_desc td = descriptor_list; td; td = td->next )
  {
    if( (td->connected == CON_PLYNG) &&
        !is_silent(td->character, FALSE) &&
        IS_MEMBER(GET_A_BITS(td->character)) &&
        ( GET_A_NUM(td->character) == alliance->forging_assoc_id ||
          GET_A_NUM(td->character) == alliance->joining_assoc_id ) )
    {
      send_to_char(str, td->character, LOG_PRIVATE);      
    }
  }
}

void do_acc(P_char ch, char *argument, int cmd)
{
  P_desc   i;
  P_char   to_ch;
  ush_int  to_guild;
  char     Gbuf1[MAX_STRING_LENGTH];

  // Golems/Magic Mouth
  if( IS_NPC(ch) && !GET_A_NUM(ch) )
  {
    return;
  }
  // Must be a Member of the guild (not banned/applicant)
  else if( !GET_A_NUM(ch) || !IS_MEMBER(GET_A_BITS(ch)) )
  {
    send_to_char("You are not a member of any associations.\r\n", ch);
    return;
  }
  // Can't speak.
  else if( IS_SET(ch->specials.act, PLR_SILENCE) || IS_AFFECTED2(ch, AFF2_SILENCED)
    || affected_by_spell(ch, SPELL_SUPPRESSION) || !IS_SET(ch->specials.act2, PLR2_ACC) || is_silent(ch, TRUE) )
  {
    send_to_char("You can't use the ACC channel!\r\n", ch);
    return;
  }

  struct alliance_data *alliance = get_alliance(GET_A_NUM(ch));

  if( !alliance )
  {
    send_to_char("Your association is not in an alliance!\n", ch);
    return;
  }

  if( IS_AFFECTED(ch, AFF_WRAITHFORM) )
  {
    send_to_char("You can't speak in this form!\r\n", ch);
    return;
  }

  while( *argument == ' ' && *argument != '\0')
  {
    argument++;
  }

  if( !*argument )
  {
    send_to_char("ACC? Yes! Fine! Chat we must, but WHAT??\r\n", ch);
  }
  else if( (IS_NPC(ch) || can_talk(ch)) )
  {
    if( ch->desc )
    {
      if( IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(ch) )
      {
        sprintf(Gbuf1, "&+yYou tell your alliance '&+Y%s&+y'\r\n", argument);
        send_to_char(Gbuf1, ch, LOG_PRIVATE);
      }
      else
      {
        send_to_char("&+bOk.\r\n", ch);
      }
    }

    if( get_property("logs.chat.status", 0.000) && IS_PC(ch) )
    {
      logit(LOG_CHAT, "%s acc's '%s'", GET_NAME(ch), argument);
    }
    for( i = descriptor_list; i; i = i->next )
    {
      if( !(to_ch = i->character) )
      {
        continue;
      }
      if( IS_NPC(to_ch) )
      {
        logit( LOG_DEBUG, "do_acc: Character (%s) is on descriptor_list but is a NPC!", J_NAME(to_ch) );
        continue;
      }

      // Skip ch, ppl at menu/char creation/etc, and deaf ppl.
      if( to_ch == ch || i->connected != CON_PLYNG || is_silent( to_ch, FALSE )
        || IS_AFFECTED4(i->character, AFF4_DEAF) )
      {
        continue;
      }

      // Dereference once for ease and speed.
      to_guild = GET_A_NUM(to_ch);

      // Immortals follow different acc rules:
      if( IS_TRUSTED(to_ch) )
      {
        // If they'r governing a diff't association (or ally) or they have ACC toggled off
        if( (to_guild && to_guild != alliance->forging_assoc_id && to_guild != alliance->joining_assoc_id)
          || !PLR2_FLAGGED(to_ch, PLR2_ACC) || to_ch->only.pc->ignored == ch )
        {
          continue;
        }
      }
      // Mortals need ACC on, and must be a guilded in the same guild/ally, must be a member, not on parole.
      //   And can't be ignoring ch.
      else if( !PLR2_FLAGGED(to_ch, PLR2_ACC) || !to_guild
        || !(to_guild == alliance->forging_assoc_id || to_guild == alliance->joining_assoc_id)
        || !IS_MEMBER(GET_A_BITS(to_ch)) || !GT_PAROLE(GET_A_BITS(to_ch)) || to_ch->only.pc->ignored == ch )
      {
        continue;
      }
      if( IS_TRUSTED(to_ch) )
      {
        sprintf(Gbuf1, "&+y%s&+y tells the alliance (%d %d) '&+Y%s&+y'\r\n", PERS(ch, to_ch, FALSE),
          alliance->forging_assoc_id, alliance->joining_assoc_id, argument);
      }
      else
      {
        sprintf(Gbuf1, "&+y%s&+y tells your alliance '&+Y%s&+y'\r\n", PERS(ch, to_ch, FALSE),
          language_CRYPT(ch, to_ch, argument));
      }
      send_to_char(Gbuf1, to_ch, LOG_PRIVATE);
    }
  }
}

void sever_revert_sethome(int assoc_id)
{
  P_desc i;
  P_char ch;

  for (i = descriptor_list; i; i = i->next)
  {
    if (i->character && !i->connected && (GET_A_NUM(i->character) == assoc_id))
      ch = i->character;
    else
      continue;

    revert_sethome(ch);
  }
}
