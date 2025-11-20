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
extern const racewar_struct racewar_color[MAX_RACEWAR+2];

vector<struct Alliance> alliances;

const char *ALLIANCE_FORMAT =
  "\r\n"
  " &+BAlliance syntax\n"
  " &+B-----------------------------\n"
  " &+bForging guild leader:    &+Walliance propose <leader of second guild>\n"
  " &+bLeader of joining guild: &+Walliance accept  <proposing leader>\n\n"
  " &+bRevoking proposal:       &+Walliance revoke\n\n"
  " &+bSevering:                &+Walliance sever confirm\n";


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
  if( !qry("SELECT forging_assoc_id, joining_assoc_id, tribute_owed FROM alliances") )
  {
    return;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  MYSQL_ROW row;

  Alliance alliance;
  while ((row = mysql_fetch_row(res)))
  {
    alliance.forging_assoc = get_guild_from_id(atoi(row[0]));
    alliance.joining_assoc = get_guild_from_id(atoi(row[1]));
    alliance.tribute_owed = atoi(row[2]);
    alliances.push_back(alliance);
  }

  mysql_free_result(res);

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
        alliances[i].forging_assoc->get_id(), alliances[i].joining_assoc->get_id());
  }
  
#endif // __NO_MYSQL__
}

Alliance::Alliance( P_Guild forgers, P_Guild joiners, int tribute_owed )
{
  char buff[MAX_STRING_LENGTH];

  forging_assoc = forgers;
  joining_assoc = joiners;

  alliances.push_back(*this);
  save_alliances();

  snprintf(buff, MAX_STRING_LENGTH, "%s &+band &n%s &+bhave formed an alliance!\n",
    forgers->get_name( ).c_str(), joiners->get_name( ).c_str() );
  send_to_alliance(buff, this );
}

// Remove the alliance between guild and their signifigant other.
void sever_alliance( P_Guild guild )
{
  bool   found = FALSE;
  string severee_name;
  char   buff[MAX_STRING_LENGTH];
  vector<Alliance>::iterator it;

//  for( int number = 0; number < alliances.size(); number++ )
  for( it = alliances.begin(); it != alliances.end(); it++ )
  {
    if( guild == it->get_forgers() )
    {
      found = TRUE;
      severee_name = it->get_joiners()->get_name();
      break;
    }
    if( guild == it->get_joiners() )
    {
      found = TRUE;
      severee_name = it->get_forgers()->get_name();
      break;
    }
  }

  if( !found )
    return;

  snprintf(buff, MAX_STRING_LENGTH, "%s &+bhave severed their alliance with &n%s&+b!\n", guild->get_name().c_str(), severee_name.c_str() );
  send_to_alliance( buff, &(*it) );

  alliances.erase( it );
  save_alliances();
}

void do_alliance( P_char ch, char *arg, int cmd )
{
  char buff[MAX_STRING_LENGTH], sub_command_str[MAX_INPUT_LENGTH], victim_str[MAX_INPUT_LENGTH];
  uint abits = GET_A_BITS(ch);
  string name;
  P_Guild guild = GET_ASSOC(ch), guild2;
  P_Alliance alliance = (guild == NULL) ? NULL : guild->get_alliance();
  P_char victim;
  struct affected_type af, *paf;

  // Check that char is in guild and leader
  if( !IS_MEMBER(abits) || (guild == NULL) )
  {
    send_to_char("&+bYou cannot forge alliances on your own!\n", ch);
    return;
  }

  if( !IS_LEADER(abits) && !IS_GOD(abits) )
  {
    send_to_char("&+bOnly guild leaders can forge or sever alliances!\n", ch);
    return;
  }

  // Check for command & target
  argument_interpreter(arg, sub_command_str, victim_str);

  if( strlen(sub_command_str) < 1 )
  {
    if( alliance == NULL )
    {
      send_to_char("&+bYour association is not allied with anyone.\n", ch);
      return;
    }

    if( alliance->get_forgers() == guild )
    {
      name = alliance->get_joiners()->get_name();
    }
    else
    {
      name = alliance->get_forgers()->get_name();
    }

    snprintf(buff, MAX_STRING_LENGTH, "&+bYou are allied with &n%s&+b.\n", name.c_str());
    send_to_char(buff, ch);
    return;
  }
  else if( !strcmp(sub_command_str, "propose") )
  {
    if( alliance != NULL )
    {
      send_to_char("&+bYour association has already forged an alliance!\n", ch);
      return;
    }

    victim = get_char_room_vis(ch, victim_str);

    if( !victim || victim == ch || !IS_PC(victim) )
    {
      send_to_char("&+bYou can only forge an alliance with the leader of another association.\n", ch);
      return;
    }

    if( racewar(ch, victim) )
    {
      send_to_char("&+bYou don't want an alliance with scum!\n", ch);
      return;
    }

    uint abits2 = GET_A_BITS(victim);
    guild2 = GET_ASSOC(victim);

    if( !IS_MEMBER(abits2) || (guild2 == NULL) || !IS_LEADER(abits2) )
    {
      send_to_char("&+bYou can only forge an alliance with the leader of another association.\n", ch);
      return;
    }

    //----------------------------
    //valid to propose?
    //----------------------------
    if( guild->get_prestige( ) < get_property("prestige.alliance.required", 0) )
    {
       send_to_char("&+bYour association is not famous enough to forge an alliance.\n", ch);
       return;
    }

    /* For now we are letting any guild be a secondary guild... -Venthix
    if( guild2->get_prestige( ) < needed_prestige )
    {
       send_to_char("&+bAlliance with the unknown? No way!\n", ch);
       return;
    }
    */

    // Forge it!
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
    if( alliance )
    {
      send_to_char("&+bYour association has already forged an alliance!\n", ch);
      return;
    }

    victim = get_char_room_vis(ch, victim_str);

    if( !victim || victim == ch || !IS_PC(victim) )
    {
      send_to_char("&+bYou can only forge an alliance with the leader of another association.\n", ch);
      return;
    }

    uint abits2 = GET_A_BITS(victim);
    guild2 = GET_ASSOC(victim);

    if( !IS_MEMBER(abits2) || (guild2 == NULL) || ( !IS_LEADER(abits2) && !IS_TRUSTED(victim) ) )
    {
      send_to_char("&+bYou can only forge an alliance with the leader of another association.\n", ch);
      return;
    }

    if( !(paf = get_spell_from_char(victim, TAG_ALLIANCE)) || paf->modifier != GET_PID(ch) )
    {
      send_to_char("&+bYou can only accept a proposal after it has been proposed!\n", ch);
      return;
    }

    affect_remove(victim, paf);
    new Alliance( guild2, guild, 0 );
  }
  else if( !strcmp(sub_command_str, "revoke") )
  {
    if( !(paf = get_spell_from_char(ch, TAG_ALLIANCE)) )
    {
      send_to_char("&+bYou haven't proposed any alliances!\n", ch);
      return;
    }

    send_to_char("&+bYou revoke your alliance proposal.\n", ch);
    affect_remove(ch, paf);
  }
  else if( !strcmp(sub_command_str, "sever") && !strcmp(victim_str, "confirm") )
  {
    if( !alliance )
    {
      send_to_char("&+bYour association has not forged an alliance.\n", ch);
      return;
    }

    sever_alliance( guild );
  }
  else
  {
    // help message
    send_to_char(ALLIANCE_FORMAT, ch);
  }
}

void send_to_alliance(char *str, P_Alliance alliance )
{
  P_Guild forgers, joiners;

  forgers = alliance->get_forgers();
  joiners = alliance->get_joiners();

  if( !alliance )
    return;

  for( P_desc td = descriptor_list; td; td = td->next )
  {
    if( (td->connected == CON_PLAYING) && IS_MEMBER(GET_A_BITS(td->character))
      && ( GET_ASSOC(td->character) == forgers || GET_ASSOC(td->character) == joiners ) )
    {
      send_to_char(str, td->character, LOG_PRIVATE);
    }
  }
}

void do_acc(P_char ch, char *argument, int cmd)
{
  P_desc     i;
  P_char     to_ch;
  char       Gbuf1[MAX_STRING_LENGTH], color;
  P_Alliance alliance;
  P_Guild    guild1, guild2;

  guild1 = GET_ASSOC(ch);

  // Must be a Member of the guild (not banned/applicant)
  if( (guild1 == NULL) || !IS_MEMBER(GET_A_BITS(ch)) )
  {
    send_to_char("You are not a member of any associations.\r\n", ch);
    return;
  }
  // Can't speak.
  if( IS_SET(ch->specials.act, PLR_SILENCE) || IS_AFFECTED2(ch, AFF2_SILENCED)
    || affected_by_spell(ch, SPELL_SUPPRESSION) || !IS_SET(ch->specials.act2, PLR2_ACC) || is_silent(ch, TRUE) )
  {
    send_to_char("You can't use the ACC channel!\r\n", ch);
    return;
  }
  if( IS_AFFECTED(ch, AFF_WRAITHFORM) )
  {
    send_to_char("You can't speak in this form!\r\n", ch);
    return;
  }

  if( (alliance = guild1->get_alliance()) == NULL )
    return do_gcc(ch, argument, cmd);

  argument = skip_spaces(argument);

  if( !*argument )
  {
    send_to_char("ACC? Yes! Fine! Chat we must, but WHAT??\r\n", ch);
  }
  else if( (IS_NPC(ch) || can_talk(ch)) )
  {
    if( ch->desc )
    {
      if( IS_NPC(ch) || IS_SET(ch->specials.act, PLR_ECHO) )
      {
        snprintf(Gbuf1, MAX_STRING_LENGTH, "&+yYou tell your alliance '&+Y%s&+y'\r\n", argument);
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
      if( to_ch == ch || i->connected != CON_PLAYING )
      {
        continue;
      }

      // Dereference once for ease and speed.
      guild2 = GET_ASSOC(to_ch);

      color = racewar_color[guild1->get_racewar()].color;

      // Immortals follow different acc rules:
      if( IS_TRUSTED(to_ch) )
      {
        // If they'r governing a diff't association (or ally) or they have ACC toggled off
        if( (guild2 && guild2 != alliance->get_forgers() && guild2 != alliance->get_joiners())
          || !PLR2_FLAGGED(to_ch, PLR2_ACC) || to_ch->only.pc->ignored == ch )
        {
          continue;
        }
      }
      // Mortals need ACC on, and must be a guilded in the same guild/ally, must be a member, not on parole.
      //   And can't be ignoring ch.
      else if( !PLR2_FLAGGED(to_ch, PLR2_ACC) || !guild2
        || !(guild2 == alliance->get_forgers() || guild2 == alliance->get_joiners())
        || !IS_MEMBER(GET_A_BITS(to_ch)) || !GT_PAROLE(GET_A_BITS(to_ch)) || to_ch->only.pc->ignored == ch )
      {
        continue;
      }
      if( IS_TRUSTED(to_ch) )
      {
        snprintf(Gbuf1, MAX_STRING_LENGTH, "&+y%s&+y tells the alliance (&+%c%d %d&+y) '&+Y%s&+y'\r\n", PERS(ch, to_ch, FALSE),
          color, alliance->get_forgers()->get_id(), alliance->get_joiners()->get_id(), argument);
      }
      else
      {
        snprintf(Gbuf1, MAX_STRING_LENGTH, "&+y%s&+y tells your alliance '&+Y%s&+y'\r\n", PERS(ch, to_ch, FALSE),
          language_CRYPT(ch, to_ch, argument));
      }
      send_to_char(Gbuf1, to_ch, LOG_PRIVATE);
    }
  }
}

