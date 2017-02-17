/************************************************************************
* assocs.c - the procedures used for player associations                *
*                                                                       *
*  Redone by Lohrr in OOP style                                         *
************************************************************************/


#include <string.h>
#include "assocs.h"
#include "alliances.h"
#include "comm.h"
#include "files.h"
#include "epic.h"
#include "guildhall.h"
#include "interp.h"
#include "nexus_stones.h"
#include "prototypes.h"
#include "sql.h"
#include "utility.h"
#include "utils.h"

// External variables & functions
extern P_index mob_index;
extern P_char character_list;
extern P_room world;
extern char guild_default_titles[ASC_NUM_RANKS][ASC_MAX_STR_RANK];
int assoc_founder(P_char mob, P_char pl, int cmd, char *arg);
string trim(string const& str, char const* sep_chars);
extern const racewar_struct racewar_color[MAX_RACEWAR+2];

// Internal variables
P_Guild guild_list = NULL;

bool Guild::is_allied_with( P_Guild ally)
{
  P_Alliance alliance = get_alliance();

  if( !alliance )
    return FALSE;

  if( (alliance->get_forgers() == ally) && (alliance->get_joiners() == ally) )
    return TRUE;

  return FALSE;
}

P_Alliance Guild::get_alliance( )
{
  for( int i = 0; i < alliances.size(); i++ )
  {
    if( this == alliances[i].get_forgers() || this == alliances[i].get_joiners() )
    {
      return &alliances[i];
    }
  }
  return NULL;
}

P_Guild get_guild_from_id( int id_num )
{
  for( P_Guild guild = guild_list; guild; guild = guild->next() )
  {
    if( guild->get_id() == id_num )
      return guild;
  }
  return NULL;
}

void sql_update_assoc_table()
{
  char buf[MAX_STRING_LENGTH];
  int i, j;
  FILE *f;
  P_Guild guild;
  string name;

  for (i = 1, j = 0; i < MAX_ASC; i++)
  {
    if( (guild = get_guild_from_id( i )) != NULL )
    {

      if( !qry("SELECT id, name FROM associations WHERE id = %d", i) )
      {
        logit(LOG_DEBUG, "Query failed in reload_assoc_table()");
        break;
      }

      name = escape_str(guild->get_name().c_str());
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

      j = 0;
    }
    else
    {
      qry("UPDATE associations SET active = 0 WHERE id = %d", i);
      j++;
    }

    if( j > 20 )
      break;
  }
}

void Guild::add_points_from_epics( P_char ch, int epics, int epic_type )
{
  int assoc_members = 1;
  int room = ch->in_room;
  // add construction points
  int cp_notch = get_property("prestige.constructionPoints.notch", 100);
  int construction_points;

  if( (GET_ASSOC(ch) != this) || (epics < (int) get_property("prestige.epicsMinimum", 4.000)) )
  {
    return;
  }

  // Count group members in same guild
  for( struct group_list *gl = ch->group; gl; gl = gl->next )
  {
    // Don't count ch twice, don't count NPCs, and don't count ppl not in room.
    if( ch != gl->ch && IS_PC(gl->ch) && room == gl->ch->in_room )
    {
      // If they're in the same guild, count them.
      if( this == GET_ASSOC(gl->ch) )
      {
        assoc_members++;
      }
    }
  }

  debug("add_points_from_epics: assoc_members: %d, epics: %d, epic_type: %d", assoc_members, epics, epic_type);

  // If members in group are above 3...
  if( assoc_members >= (int) get_property("prestige.guildedInGroupMinimum", 3.000) )
  {
    int prest = 0;

    switch( epic_type )
    {
      case EPIC_PVP:
      case EPIC_SHIP_PVP:
        prest = (int) get_property("prestige.gain.pvp", 20);
        break;

      case EPIC_ZONE:
      case EPIC_QUEST:
      case EPIC_RANDOM_ZONE:
      case EPIC_NEXUS_STONE:
      default:
        prest = (int) get_property("prestige.gain.default", 10);
    }

    prest = check_nexus_bonus(ch, prest, NEXUS_BONUS_PRESTIGE);

    debug("add_points_from_epics(): '%s' gaining: %d", get_name().c_str(), prest);

    send_to_char("&+bYour guild gained prestige!\n", ch);
    construction_points = MAX(0, (int) ((prestige+prest) / cp_notch) - (prestige/cp_notch));
    add_prestige( prest );
  }

  if( construction_points > 0 )
  {
    statuslog(GREATER_G, "Association %s &ngained %d construction points.", name, construction_points );
    logit(LOG_STATUS, "Association %s &ngained %d construction points.", name, construction_points );
    construction += construction_points;
  }

}

bool Guild::sub_money( int p, int g, int s, int c )
{

//debug( "%s: %d/%d p %d/%d g %d/%d s %d/%d c.", name, p, platinum, g, gold, s, silver, c, copper );

  if( platinum < p || gold < g || silver < s || copper < c )
    return FALSE;

  platinum -= p;
  gold -= g;
  silver -= s;
  copper -= c;

  write_transaction_to_ledger( str_dup("System"), "withdrew", coins_to_string(p, g, s, c, "&+y") );
  return TRUE;
}

void Guild::add_frags( P_char ch, long new_frags )
{

  if( IS_NPC(ch) )
  {
    return;
  }

  // new frag leader in guild?
  if( GET_FRAGS(ch) > frags.top_frags )
  {
    sprintf( frags.topfragger, "%s", GET_NAME(ch) );
    frags.top_frags = GET_FRAGS(ch);
  }

  frags.frags += new_frags;
}

void show_guild_frags( P_char ch )
{
  char Gbuf1[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int  guild_count, i;

  struct guild_frag_info gfrag_list[MAX_ASC];
  bzero(&gfrag_list, sizeof(gfrag_list));
  guild_count = 0;

  // Create the list.
  for( P_Guild guild = guild_list; guild; guild = guild->next() )
  {
    i = 0;
    // While we're not at the end of the current list
    while( guild_count > i )
    {
      // If we've found a place to insert the new guild.
      if( gfrag_list[i].tot_frags < guild->get_frags() )
      {
        // Move each following entry up a spot
        for( int j = guild_count; j > i; j-- )
        {
          gfrag_list[j] = gfrag_list[j-1];
        }
        break;
      }
      i++;
    }
    // Insert the new entry into the open slot.
    strcpy( gfrag_list[i].guild_name, guild->get_name().c_str() );
    gfrag_list[i].tot_frags = guild->get_frags();
    sprintf( gfrag_list[i].top_fragger, "%s", guild->get_top_fragger() );
    gfrag_list[i].top_frags = guild->get_top_frags();
    gfrag_list[i].num_members = guild->get_num_members();
    guild_count++;
  }

  // Dislay the list
  // Title of list
  strcpy(buf, "\t&+L      -=&+W Top Guilds &+L=-\n"
    "    &+r---------------------------------------\n");

  i = 0;
  while( i < guild_count )
  {
    sprintf(Gbuf1, "\t%s\n\t&+WTotal Guild Frags: \t&+Y%+.2f&n\n"
      "\t&+WTop Fragger: &+Y%-10s %+.2f&n\n"
      "\t&+WMembers:&+Y%2d&+W Frags/Member:&+Y%+.2f&n\n"
      "    &+L---------------------------------------\n",
      gfrag_list[i].guild_name, gfrag_list[i].tot_frags / 100.00, gfrag_list[i].top_fragger,
      gfrag_list[i].top_frags / 100.00, gfrag_list[i].num_members,
      (float)(gfrag_list[i].tot_frags / ( gfrag_list[i].num_members * 100.00 )) );
    strcat(buf, Gbuf1);
    i++;
  }
  page_string(ch->desc, buf, 0);
}

/* God founds an association
 * Arguments are the char who is going to be leader of it,
 *   the name of the association, and a string triggering guild bits
 * Return value is TRUE on success, returns FALSE if there are issues:
 *  too many associations or if leader is already member somewhere
 *  else, or problem with the file
 */
bool found_asc(P_char god, P_char leader, char *bits, char *asc_name)
{
  char     file_name[MAX_STR_NORMAL], buf[MAX_STRING_LENGTH];
  ush_int  i;
  P_Guild  pNewGuild;
  int      asc_bits;

  // This is a god command only!
  // 5/6/13 - Drannak - modifying to allow mob proc to call this for automated guild creation.
  // Tweaked to make it allowable for Gods/mobs w/proc only.
  if( !IS_TRUSTED(god) && !(IS_NPC(god) && ( MOB_PROC(god) == assoc_founder )) )
  {
    send_to_char( "Arrrgghh! Out, out,...\n", god);
    send_to_char( "Error creating association... bad creator.\n", leader );
    return FALSE;
  }
  // Is this guy involved somewhere else?
  if( (bits[0] != 'k') && (GET_ASSOC( leader ) || !IS_NO_THANKS( GET_A_BITS(leader) )) )
  {
    if( IS_TRUSTED(god) )
      send_to_char("That person is unable to found an association...\n", god);
    else
      do_say( god, "You are unable to found an association.", CMD_SAY );
    return FALSE;
  }
  /* no associations under level ASC_MIN_LEVE */
  if( GET_LEVEL(leader) < ASC_MIN_LEVEL )
  {
    send_to_char("That person is too low in level!\n", god);
    return FALSE;
  }

  i = 1;
  while( (get_guild_from_id( i ) != NULL)  && (i < MAX_ASC) )
  {
    i++;
  }

  // too many associations
  if( i >= MAX_ASC )
  {
    send_to_char("There are too many associations to create a new one!\n", god);
    send_to_char("There are too many associations to create a new one!\n", leader);
    return FALSE;
  }

  /* name of association */
  if( strlen(asc_name) >= ASC_MAX_STR )
  {
    sprintf( buf, "found_asc: Truncating '%s' to '", asc_name );
    asc_name[ASC_MAX_STR - 1] = '\0';
    strcat( buf, asc_name );
    strcat( buf, "'." );
    debug( buf );
  }

  /* bits string interpreter */
  asc_bits = 0;
  while( LOWER(*bits) )
  {
    switch( *bits )
    {
    // challenge allowed
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
      asc_bits = 0;
      break;
    default:
      sprintf( buf, "'%c' is not a valid bit code.  Valid codes are: 'c', 'h', 's', and 'n'.\n"
        "'c' - Allow challenges.\n"
        "'h' - Hide titles.\n"
        "'s' - Hide subtitles.\n"
        "'n' - Reset the bit-string to 0 and start over.\n", *bits );
      if( IS_TRUSTED(god) )
        send_to_char( buf, god );
      else
        send_to_char( buf, leader );
      return FALSE;
      break;
    }
    bits++;
  }
  // The guild bits should have no player info.
  SET_M_BITS(asc_bits, A_P_MASK, 0);

  // Create the new guild
  pNewGuild = new Guild( asc_name, GET_RACEWAR(leader), i, 0, 0, 0, asc_bits );
  // Add the leader to it
  pNewGuild->add_member( leader, A_LEADER );
  // Add the guild to the head of the guild_list.
  pNewGuild->set_next( guild_list );
  guild_list = pNewGuild;

#ifndef __NO_MYSQL__
  mysql_real_escape_string( DB, buf, asc_name, strlen(asc_name) );
  qry( "REPLACE INTO associations (id, name, active) VALUES (%d, '%s', 1)", i, buf );
#endif

  send_to_char("Ok, new association is set up.\n", god);
  return TRUE;
}

Guild::Guild( char *_name, unsigned int _racewar, unsigned int _id_number, unsigned long _prestige, \
      unsigned long _construction, unsigned long _money, unsigned int _bits )
{
  strcpy( name, _name );
  racewar = _racewar;
  id_number = _id_number;
  prestige = _prestige;
  construction = _construction;
  copper = _money % 10;
  _money /= 10;
  silver = _money % 10;
  _money /= 10;
  gold = _money % 10;
  _money /= 10;
  platinum = _money;
  member_count = 0;
  bits = _bits;
  frags.frags = 0;
  frags.top_frags = 0;
  frags.topfragger[0] = '\0';
  overmax = 0;
  next_guild = NULL;
  members = NULL;
  for( int i = 0; i < ASC_NUM_RANKS; i++ )
  {
    sprintf( titles[i], "%s", guild_default_titles[i] );
  }
}

Guild::Guild( )
{
  name[0] = '\0';
  racewar = RACEWAR_NONE;
  id_number = -1;
  prestige = 0;
  construction = 0;
  copper = silver = gold = platinum = 0;
  member_count = 0;
  bits = 0;
  frags.frags = 0;
  frags.top_frags = 0;
  frags.topfragger[0] = '\0';
  overmax = 0;
  next_guild = NULL;
  members = NULL;
  for( int i = 0; i < ASC_NUM_RANKS; i++ )
  {
    sprintf( titles[i], "%s", guild_default_titles[i] );
  }
}

void Guild::initialize()
{
  int i, miss_count;

  for( i = 1, miss_count = 0; miss_count < 20; i++ )
  {
    if( load_guild(i) )
    {
      miss_count = 0;
    }
    else
    {
      miss_count++;
    }
  }
}

void Guild::save( )
{
  FILE *file;
  char  filename[MAX_STR_NORMAL], write_buf[MAX_STRING_LENGTH], buf[MAX_STR_NORMAL];
  P_member pMembers;

  sprintf( filename, "%sasc.%u", ASC_DIR, id_number );
  file = fopen( filename, "w" );
  if( !file )
  {
    debug( "Guild::save: Could not open file '%s' for writing! :(", filename );
    return;
  }

  // Print the name first.
  sprintf( write_buf, "%s\n", name );
  // Then the guild number and frag info.
  sprintf( buf, "%u %lu %lu %s\n", id_number, frags.frags, frags.top_frags, frags.topfragger );

  strcat( write_buf, buf );
  // Then the default guild titles
  for( int i = 0; i < ASC_NUM_RANKS; i++ )
  {
    // I'm not sure if it's faster to strcat to write_buf 2x, or to sprintf to buf then strcat buf to write_buf.
    // This seems faster, since we're only writing to one place instead of two, so no paging.
    strcat( write_buf, titles[i] );
    strcat( write_buf, "\n" );
  }
  // Then the guild bits, prestige and construction.
  sprintf( buf, "%u %lu %lu\n", bits, prestige, construction );
  strcat( write_buf, buf );
  // Then guild funds.
  sprintf( buf, "%u %u %u %u\n", platinum, gold, silver, copper );
  strcat( write_buf, buf );

  for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
  {
    sprintf( buf, "%s %u %u\n", pMembers->name, pMembers->bits, pMembers->debt );
    strcat( write_buf, buf );
  }


  fwrite( write_buf, strlen(write_buf), 1, file );
  fclose( file );
}

bool Guild::load_guild( int guild_num )
{
  P_Guild  new_guild;
  P_member last_member, new_member;
  FILE    *file;
  char     filename[MAX_STR_NORMAL], buf[MAX_STR_NORMAL], mem_name[MAX_NAME_LENGTH + 1];
  int      mem_bits, mem_debt;

  sprintf( filename, "%sasc.%u", ASC_DIR, guild_num );
  file = fopen( filename, "r" );
  if( !file )
  {
    return FALSE;
  }
  new_guild = new Guild();

  // Get the guild name.
  fgets( new_guild->name, ASC_MAX_STR + 1, file );
  // Cut the carriage return off.
  new_guild->name[strlen(new_guild->name) - 1] = '\0';
  // Then the guild number and frag info.
  fscanf( file, "%u %lu %lu %s\n", &(new_guild->racewar), &(new_guild->frags.frags), &(new_guild->frags.top_frags),
    new_guild->frags.topfragger );

  new_guild->id_number = guild_num;

  // Then get the default guild titles.
  for( int i = 0; i < ASC_NUM_RANKS; i++ )
  {
    fgets( buf, ASC_MAX_STR_RANK + 1, file );
    // Cut the carriage return off.
    buf[strlen(buf) - 1] = '\0';
    sprintf( new_guild->titles[i], "%s", buf );
  }
  // Then get the guild bits, prestige and construction.
  fgets( buf, MAX_STR_NORMAL, file );

  sscanf( buf, "%u %lu %lu\n", &new_guild->bits, &new_guild->prestige, &new_guild->construction );

  // Then get the money for the guild...
  fscanf( file, "%u %u %u %u\n", &(new_guild->platinum), &(new_guild->gold),
    &(new_guild->silver), &(new_guild->copper) );

  // Then get members.
  last_member = NULL;
  while( fscanf(file, "%s %u %u\n", mem_name, &mem_bits, &mem_debt) == 3 )
  {
    new_member = new guild_member();
    sprintf( new_member->name, "%s", mem_name );
    new_member->bits = mem_bits;
    new_member->debt = mem_debt;
    new_member->next = NULL;

    if( last_member == NULL )
    {
      new_guild->members = new_member;
      last_member = new_member;
    }
    else
    {
      last_member->next = new_member;
      last_member = new_member;
    }

    new_guild->member_count++;
  }
  fclose( file );

  new_guild->next_guild = guild_list;
  guild_list = new_guild;
  return TRUE;
}

// Destructor.. just need to remove from guild_list.
Guild::~Guild()
{
  P_Guild iterator = guild_list;
  char filename[MAX_STRING_LENGTH];
  P_char member;

  for( member = character_list; member; member = member->next )
  {
    if( IS_PC(member) && (GET_ASSOC( member ) == this) )
    {
      GET_ASSOC( member )->kick( member );
    }
  }

  sever_alliance( this );

  sprintf( filename, "%sasc.%u", ASC_DIR, id_number );
  unlink( filename );

  // We just need to remove it from guild list.
  if( !iterator )
  {
    wizlog( 56, "Deleting guild '%s' %d: guild_list is already empty?!", name, id_number );
    logit(LOG_DEBUG, "Deleting guild '%s' %d: guild_list is already empty?!", name, id_number );
  }
  else if( iterator == this )
  {
    guild_list = guild_list->next_guild;
  }
  else
  {
    while( (iterator->next_guild != NULL) && (iterator->next_guild != this) )
    {
      iterator = iterator->next_guild;
    }
    if( iterator->next_guild != this )
    {
      wizlog( 56, "Deleting guild '%s' %d: not on guild_list?!", name, id_number );
      logit(LOG_DEBUG, "Deleting guild '%s' %d: not on guild_list?!", name, id_number );
    }
    else
    {
      iterator->next_guild = this->next_guild;
    }
  }
}

// Returns TRUE iff ch was successfully added.
bool Guild::add_member( P_char ch, int rank )
{
  long new_frags = GET_FRAGS(ch);
  int length;
  P_member last_member;

  if( GET_ASSOC(ch) != NULL )
  {
    send_to_char( "Tried to add you to a guild, but you're already in one!\n", ch );
    debug( "Guild::add_member: '%s' %d is trying to enter guild '%s' %d, but already in guild '%s' %d.",
      J_NAME(ch), GET_ID(ch), name, id_number, GET_ASSOC(ch)->name, GET_ASSOC(ch)->id_number );
    return FALSE;
  }

  frags.frags += new_frags;
  if( new_frags > frags.top_frags )
  {
    strcpy( frags.topfragger, GET_NAME(ch) );
    frags.top_frags = new_frags;
  }

  SET_M_BITS( GET_A_BITS(ch), A_RK_MASK, rank );
  GET_ASSOC(ch) = this;
  SET_MEMBER( GET_A_BITS(ch) );
  default_title( ch );
  send_to_char_f( ch, "You are now known as %s %s!\n", GET_NAME(ch), GET_TITLE(ch) );

  // Add to the end of the member list.
  // If list is not empty - guild has members.
  if( (last_member = members) != NULL )
  {
    while( last_member->next != NULL )
    {
      last_member = last_member->next;
    }
    last_member->next = new guild_member;
    last_member = last_member->next;
  }
  else
  {
    members = new guild_member;
    last_member = members;
  }
  strcpy(last_member->name, GET_NAME(ch) );
  last_member->bits = GET_A_BITS(ch);
  last_member->debt = 0;
  last_member->next = NULL;
  member_count++;

  if( IS_PC(ch) )
  {
    SET_BIT(ch->specials.act, PLR_GCC);
    SET_BIT(ch->specials.act2, PLR2_ACC);
  }
  save( );
  return TRUE;
}

void Guild::default_title( P_char ch )
{
  char new_title[ASC_MAX_STR_TITLE];

  clear_title( ch );
  if( !IS_HIDDENSUBTITLE(GET_A_BITS(ch)) )
  {
    sprintf( new_title, "%s %s", titles[NR_RANK(GET_A_BITS(ch))], name );
  }
  else
  {
    sprintf( new_title, "%s", name );
  }

  GET_TITLE(ch) = str_dup(new_title);
}

void Guild::update_member( P_char ch )
{
  Guildhall *gh;
  char home_string[100], title[ASC_MAX_STR_TITLE];
  P_member pMembers;

  // Check level.
  if( GET_LEVEL(ch) < ASC_MIN_LEVEL )
  {
    // Set parole on ch's bits
    SET_PAROLE( GET_A_BITS(ch) );
    // Set parole in the members list.
    for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
    {
      if( !strcmp(pMembers->name, GET_NAME( ch )) )
      {
        SET_M_BITS( pMembers->bits, A_RK_MASK, A_PAROLE );
        break;
      }
    }
    send_to_char( "You have been placed on parole.\n", ch );
  }

  if( !IS_TRUSTED(ch) )
  {
    for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
    {
      if( !strcmp(pMembers->name, GET_NAME( ch )) )
      {
        break;
      }
    }

    if( pMembers == NULL )
    {
      frag_remove(ch);
      GET_ASSOC(ch) = NULL;
      GET_A_BITS(ch) = 0;
      send_to_char( "You have been kicked out of your association in disgrace.\n", ch );
      // add_member( ch, (A_GET_RANK(ch) > 0) ? A_GET_RANK(ch) : A_NORMAL );
    }
  }

  // Check homed in someone else's guildhall.
  if( (gh = Guildhall::find_by_vnum( GET_BIRTHPLACE(ch) )) != NULL )
  {
    if( !IS_MEMBER(GET_A_BITS( ch )) || gh->get_assoc() != GET_ASSOC(ch) )
    {
      logit( LOG_DEBUG, "%s was homed in GH but doesn't belong to guild, resetting to original home",
        GET_NAME(ch) );
      sprintf( home_string, "char %s home %d", J_NAME(ch), GET_ORIG_BIRTHPLACE(ch));
      do_setbit( ch, home_string, CMD_SETHOME);
      sprintf( home_string, "char %s orighome %d", J_NAME(ch), GET_ORIG_BIRTHPLACE(ch));
      do_setbit( ch, home_string, CMD_SETHOME);
    }
  }
  update_bits( ch );
  // Update title if supposed to.
  if( !IS_STT(GET_A_BITS( ch )) )
  {
    default_title( ch );
  }
}

void Guild::kick( P_char ch )
{
  Guildhall *gh;

  if( this != GET_ASSOC(ch) )
  {
    debug( "Trying to kick '%s' %d from guild '%d' when they are in guild '%d'",
      J_NAME(ch), GET_ID(ch), get_id(), GET_ASSOC(ch)->get_id() );
    return;
  }

  frag_remove(ch);
  GET_ASSOC(ch) = NULL;
  GET_NB_LEFT_GUILD(ch) += 1;
  GET_TIME_LEFT_GUILD(ch) = time(NULL);
  SET_NO_THANKS(GET_A_BITS(ch));
  clear_title(ch);

  send_to_char("You were kicked out of your association in disgrace!\n", ch);

  if( (gh = Guildhall::find_by_vnum( GET_BIRTHPLACE(ch) )) != NULL )
  {
    send_to_char( "You feel as if you've lost your home.\n", ch );
    GET_BIRTHPLACE(ch) = GET_HOME(ch) = GET_ORIG_BIRTHPLACE(ch);
  }
  remove_member_from_list( ch );
  member_count--;
  save( );
  writeCharacter(ch, RENT_CRASH, ch->in_room );
}

void Guild::kick( P_char kicker, char *char_name )
{
  P_member pMembers, pTargetMember;

  CAP(char_name);

  if( !char_name || !*char_name )
  {
    send_to_char( "Who?!\n", kicker );
    return;
  }

  pMembers = members;
  // If at the head of the list
  if( !strcmp(members->name, char_name) )
  {
    if( GET_RK_BITS(members->bits) > GET_RK_BITS(GET_A_BITS(kicker)) )
    {
      send_to_char( "You can't kick a superior?!\n", kicker );
      return;
    }

    // Move list down one and free memory.
    members = members->next;
    pMembers->next = NULL;
    delete pMembers;
  }
  else
  {
    // Find the member before (next exists and has a different name).
    while( (pMembers->next != NULL) && strcmp(pMembers->next->name, char_name) )
    {
      pMembers = pMembers->next;
    }
    // Not on list? WTF!?
    if( pMembers->next == NULL )
    {
      send_to_char_f( kicker, "'%s' is not on your guild list.\n", char_name );
      return;
    }
    if( GET_RK_BITS(pMembers->next->bits) > GET_RK_BITS(GET_A_BITS(kicker)) )
    {
      send_to_char( "You can't kick a superior?!\n", kicker );
      return;
    }
    // Remove them from the list.
    pTargetMember = pMembers->next;
    pMembers->next = pTargetMember->next;
    // And free memory.
    pTargetMember->next = NULL;
    delete pTargetMember;
  }
  member_count--;
  send_to_char_f( kicker, "You mercilessly kick %s from your guild.\n", char_name );
  save( );
}

void Guild::remove_member_from_list( P_char ch )
{
  P_member   pMembers, pTargetMember;
  char      *char_name;

  char_name = GET_NAME(ch);
  pMembers = members;
  // If at the head of the list
  if( !strcmp(members->name, char_name) )
  {
    // Move list down one and free memory.
    members = members->next;
    pMembers->next = NULL;
    delete pMembers;
  }
  else
  {
    // Find the member before (next exists and has a different name).
    while( (pMembers->next != NULL) && strcmp(pMembers->next->name, char_name) )
    {
      pMembers = pMembers->next;
    }
    // Not on list? WTF!?
    if( pMembers->next == NULL )
    {
      logit(LOG_DEBUG, "Guild::kick: '%s' %d not on guild %d's member list.", char_name, GET_ID(ch), id_number );
      debug( "Guild::kick: '%s' %d not on guild %d's member list.", char_name, GET_ID(ch), id_number );
      return;
    }
    // Remove them from the list.
    pTargetMember = pMembers->next;
    pMembers->next = pTargetMember->next;
    // And free memory.
    pTargetMember->next = NULL;
    delete pTargetMember;
  }
}

bool Guild::is_enemy( P_char enemy )
{
  P_char  master;
  char    buf[MAX_STR_NORMAL];
  FILE   *f;
  int     dummy1, dummy2, dummy3, dummy4, dummy5;

  if( IS_TRUSTED(enemy) )
  {
    return FALSE;
  }
  if( IS_NPC(enemy) )
  {
    if( (master = GET_MASTER( enemy )) != NULL )
    {
      return is_enemy( master );
    }
    return FALSE;
  }
  if( (GET_ASSOC( enemy ) == this) && (IS_ENEMY( GET_A_BITS(enemy) )) )
  {
    return TRUE;
  }

  sprintf(buf, "%sasc.%u", ASC_DIR, id_number);
  if( !(f = fopen( buf, "r" )) )
  {
    return FALSE;
  }

  // Go past cash
  for( int i = 0; i < 11; i++ )
    fgets(buf, MAX_STR_NORMAL, f);

  // Control if enemy
  while (fscanf(f, "%s %u %u %u %u %u\n", buf, &dummy1, &dummy2,
                &dummy3, &dummy4, &dummy5) != EOF)
  {
    if (!str_cmp(GET_NAME(enemy), buf) && IS_ENEMY(dummy1))
    {
      fclose(f);
      return (1);
    }
  }
  fclose(f);

  return FALSE;
}

void Guild::display( P_char member )
{
  char buf[MAX_STRING_LENGTH], Gbuf2[MAX_STR_NORMAL], *current_title;
  P_Alliance alliance;
  int gbits = bits & ASC_MASK;
  int rank = GET_A_BITS(member) & A_RK_MASK;
  const char *standard_names[ASC_NUM_RANKS] =
  {
    "enemy", "parole", "normal", "senior", "officer", "deputy", "leader", "king"
  };
  P_member pMembers;

  sprintf(buf, "\n%s&n\n-----------------------------------------------------------------", name );
  if( (alliance = get_alliance( )) != NULL )
  {
    if( alliance->get_forgers()->get_id() == id_number )
    {
      sprintf( Gbuf2, "\n&+bAllied with &n%s\n", alliance->get_joiners()->get_name().c_str() );
    }
    else
    {
      sprintf( Gbuf2, "\n&+bAllied to &n%s\n", alliance->get_forgers()->get_name().c_str() );
    }
    strcat(buf, Gbuf2);
  }
  strcat(buf, "\n");

  // Display rank names if leader or higher.
  if( rank >= A_LEADER )
  {
    for( int i = 0; i < ASC_NUM_RANKS; i++ )
    {
      sprintf(buf + strlen(buf), "%-8s : %s&n\n", standard_names[i], titles[i]);
    }
    strcat( buf, "\n" );
  }

  sprintf( buf + strlen(buf), "Type: %s%s%s%s.\n", (( gbits == 0 ) ? "normal " : ""),
    (( gbits & A_CHALL ) ? "challenge " : ""), (( gbits & A_HIDETITLE ) ? "hidden_titles " : ""),
    (( gbits & A_HIDESUBTITLE ) ? "hidden_subtitles " : "") );

  sprintf( buf + strlen(buf), "Prestige:            &+W%lu&n.\nConstruction points: &+W%lu&n.\n"
                              "Maximum members:     &+W%u&n.\nCurrent members:     &+W%u&n.\n"
                              "Cash:                &+W%u platinum&n, &+Y%u gold&n, &+w%u silver&n, &+y%u copper&n.\n",
    prestige, construction, get_max_members(), member_count, platinum, gold, silver, copper );

  sprintf(buf + strlen(buf), "Total Frags: &+W%.2f&N, Top Fragger: '&+W%s&N' with &+W%.2f&N frags.\n",
    frags.frags / 100., frags.topfragger, frags.top_frags / 100. );

  strcat(buf, "Members:\n");
  if( members != NULL )
  {
    update_online_members();

    // Start with A_LEADER -> 6 (we skip A_GOD and -1 because 0..7).
    // And we finish with 1 not 0, because we don't care about A_ENEMY.
    for( int i = ASC_NUM_RANKS - 2; i >= 0; i-- )
    {
      current_title = titles[i];
      pMembers = members;
      while( pMembers )
      {
        if( NR_RANK(pMembers->bits) == i )
        {
          sprintf( buf + strlen(buf), " %s | %-12s | %s | %s\n",
            (pMembers->online_status == GSTAT_ONLINE) ? "&+Go&n"
            : (pMembers->online_status == GSTAT_LINKDEAD) ? "&+y+&n" : " ", pMembers->name,
            current_title, (pMembers->debt && ( (rank > A_SENIOR) || !strcmp(pMembers->name, GET_NAME( member )) ))
            ? (( (rank > GET_RK_BITS( pMembers->bits )) || !strcmp(pMembers->name, GET_NAME( member )) )
            ? coin_stringv( pMembers->debt, 0 ) : "&+R(-)&n") : "" );
        }
        pMembers = pMembers->next;
      }
    }
  }
  else
  {
    strcat( buf, "None.\n" );
  }
  send_to_char( buf, member );
}

unsigned int Guild::get_max_members()
{
  int base_size, max_size, step_size, max_members;

  base_size = get_property("guild.size.base", 0);
  max_size = get_property("guild.size.max", 0);
  step_size = get_property("guild.size.prestige.step", 0);
  // Maximum number of members not taking into account global max + overmax
  max_members = base_size + ( prestige / step_size );

  return MIN( max_members, max_size ) + overmax;
}

void Guild::apply(P_char applicant, P_char member )
{
  char buf[MAX_STRING_LENGTH];

  if( GET_RACEWAR(applicant) != racewar )
  {
    send_to_char( "You don't want to associate with scum!\n", applicant );
    return;
  }
  if( IS_MEMBER(GET_A_BITS( applicant )) )
  {
    send_to_char( "You are already a member in an association!\n", applicant );
    return;
  }
  if( GET_LEVEL(applicant) < ASC_MIN_LEVEL )
  {
    send_to_char( "Associations are for experienced players only!\n", applicant );
    return;
  }
  if( applicant == member )
  {
    SET_NO_THANKS( GET_A_BITS(applicant) );
    GET_ASSOC( applicant ) = NULL;
    send_to_char("Fine, fine. You are on your own now.\n", applicant );
    clear_title( applicant );
    return;
  }
  if( !GT_NORMAL(GET_A_BITS( member )) )
  {
    send_to_char( "You need a senior member to get enrolled!\n", applicant );
    return;
  }
  SET_APPLICANT( GET_A_BITS(applicant) );
  GET_ASSOC(applicant) = this;

  sprintf( buf, "You ask $N to enroll you in %s.", name );
  act( buf, FALSE, applicant, NULL, member, TO_CHAR);
  act("$n is applying for membership in your association!", FALSE, applicant, NULL, member, TO_VICT);
}

void supervise_list( P_char god )
{
  char buf[MAX_STRING_LENGTH];
  char Gbuf1[MAX_STRING_LENGTH];
  int  id_num, misses;
  P_Guild guild;

  strcpy(buf, " Number : Racewar : Name of Player Associations\n"
              " ----------------------------------------------\n");
  for( id_num = 1, misses = 0; id_num < MAX_ASC; id_num++ )
  {
    guild = get_guild_from_id( id_num );
    if( guild != NULL )
    {
      sprintf( Gbuf1, "   %s %2u : &+%c%7s&n : %s&n\n", (GET_ASSOC(god) == guild) ? "&+Y*&n" : " ", id_num,
        racewar_color[guild->get_racewar()].color, racewar_color[guild->get_racewar()].name, guild->get_name().c_str() );
      strcat( buf, Gbuf1 );
      misses = 0;
    }
    else
    {
      // Count misses and cut out early if no guilds within 20.
      // Seeing as we only have one or two guilds disband during a wipe atm,
      //   this seems more than adequate.
      if( ++misses >= 20 )
      {
        break;
      }
    }
  }
  send_to_char( buf, god );
}

void do_asclist( P_char god, char *argument, int cmd )
{
  if( !IS_TRUSTED(god) )
  {
    send_to_char( "A mere mortal can't do this!\n", god );
    return;
  }
  supervise_list(god);
}

void do_supervise( P_char god, char *argument, int cmd )
{
  char first[MAX_INPUT_LENGTH], second[MAX_INPUT_LENGTH], third[MAX_INPUT_LENGTH];
  char fourth[MAX_INPUT_LENGTH], *rest, buf[MAX_STRING_LENGTH], *index;
  P_Guild guild;
  P_char member;
  int bits, count;

  if( !IS_TRUSTED(god) )
  {
    send_to_char( "A mere mortal can't do this!\n", god );
    return;
  }

  rest = lohrr_chop( argument, first );
  rest = lohrr_chop( rest, second );

  // If we had no arguments.
  if( *first == '\0' )
  {
    supervise_list( god );
    return;
  }

  if( is_abbrev(first, "found") )
  {
    if( !*second )
    {
      send_to_char( "Missing leader name.\n&+YSyntax: &+wsupervise found <leader> <bits> <guild name>&+Y.&n\n"
        "&+YNote: &+w<bits>&+Y = &+wn&+Y(clear bitstring) | &+wc&+Y(Challenge allowed) | &+wh&+Y(Hide titles) | "
        "&+ws&+Y(Hide subtitles).&n\n", god );
      return;
    }

    if( (member = get_char_vis( god, second )) == NULL )
    {
      send_to_char_f( god, "Could not find player '%s' in game.\n", second );
      return;
    }
    rest = lohrr_chop( rest, third );
    if( !*third )
    {
      send_to_char("Missing association bits string.\n"
        "&+YNote: &+w<bits>&+Y = &+wn&+Y(clear bitstring) | &+wc&+Y(Challenge allowed) | &+wh&+Y(Hide titles) | "
        "&+ws&+Y(Hide subtitles).&n\n", god );
      return;
    }
    // Verify a good bit string.
    for( index = third; *index != '\0'; index++ )
    {
      switch( LOWER(*index) )
      {
        case 'n':
        case 'c':
        case 'h':
        case 's':
          break;
        default:
          send_to_char_f( god, "'%c' is not a valid bit.\n"
            "&+YNote: &+w<bits>&+Y = &+wn&+Y(clear bitstring/none) | &+wc&+Y(Challenge allowed) "
            "| &+wh&+Y(Hide titles) | &+ws&+Y(Hide subtitles).&n\n", *index );
          return;
          break;
      }
    }
    if( !found_asc(god, member, third, rest) )
    {
      return;
    }
    wizlog( GET_LEVEL(god), "%s made %s founding leader of %s, bits string is '%s'.",
      GET_NAME(god), GET_NAME(member), rest, third );
    logit( LOG_WIZ, "%s made %s founding leader of %s, bits string is '%s'.",
      GET_NAME(god), GET_NAME(member), rest, third );
    sql_log( god, WIZLOG, "%s made %s founding leader of %s, bits string is '%s'.",
      GET_NAME(god), GET_NAME(member), rest, third );
    return;
  }
  if( is_abbrev(first, "delete") )
  {
    if( !is_number(second) || (( guild = get_guild_from_id(atoi( second )) ) == NULL) )
    {
      send_to_char_f( god, "'%s' is not a valid association number.\n"
        "&+YSyntax: &+wsupervise delete <association number>&+Y.&n\n", second );
      return;
    }
    delete guild;
    send_to_char_f( god, "Guild number %d deleted.\n", atoi(second) );
    return;
  }
  if( is_abbrev(first, "name") )
  {
    if( !is_number(second) || (( guild = get_guild_from_id(atoi( second )) ) == NULL) )
    {
      send_to_char_f( god, "'%s' is not a valid association number.\n"
        "&+YSyntax: &+wsupervise name <association number> <new name>&+Y.&n\n", second );
      return;
    }
    while( isspace(*rest) )
    {
      rest++;
    }
    if( !*rest )
    {
      send_to_char( "Missing association name.\n", god );
      return;
    }
    guild->set_name( rest );
    return;
  }
  if( is_abbrev(first, "type") )
  {
    if( *second == '\0' )
    {
      send_to_char("Missing association bits string (use n for none).\n"
        "&+YNote: &+w<bits>&+Y = &+wn&+Y(clear bitstring/none) | &+wc&+Y(Challenge allowed) "
        "| &+wh&+Y(Hide titles) | &+ws&+Y(Hide subtitles).&n\n", god );
    }

    index = second;
    bits = 0;
    while( *index != '\0' )
    {
      switch( *index )
      {
        case 'c':
          SET_CHALL(bits);
          break;
        case 'h':
          SET_HIDETITLE(bits);
          break;
        case 's':
          SET_HIDESUBTITLE(bits);
          break;
        case 'n':
          bits = 0;
          break;
        default:
          sprintf( buf, "'%c' is not a valid bit code.  Valid codes are: 'c', 'h', 's', and 'n'.\n"
          "'c' - Allow challenges.\n"
          "'h' - Hide titles.\n"
          "'s' - Hide subtitles.\n"
          "'n' - Reset the bit-string to 0 and start over.\n", *index );
          send_to_char( buf, god );
          return;
          break;
      }
    }
    lohrr_chop( rest, third );
    if( !is_number(third) || (( guild = get_guild_from_id(atoi( third )) ) == NULL) )
    {
      send_to_char_f( god, "'%s' is not a valid association number.\n"
        "&+YSyntax: &+wsupervise type <new bits> <association number>&+Y.&n\n", third );
      return;
    }
    guild->set_bits( bits );
    send_to_char_f( god, "The new bits string for '%s' %d is: %b.", guild->get_name().c_str(),
      guild->get_id(), bits );
    return;
  }
  if( is_abbrev(first, "govern") )
  {
    if( !is_number(second) || (( guild = get_guild_from_id(atoi( second )) ) == NULL) )
    {
      // Soc g 0 -> stop governing.
      if( is_number(second) && (atoi( second ) == 0) && (( guild = GET_ASSOC(god) ) != NULL) )
      {
        send_to_char_f( god, "You stop supervising '%s' %d.\n", guild->get_name().c_str(), guild->get_id() );
        GET_ASSOC(god) = NULL;
        SET_NO_THANKS(GET_A_BITS(god));
        return;
      }
      if( is_number(second) && (atoi( second ) == 0) )
        send_to_char( "But you're not governing an association at the moment.\n", god );
      else
        send_to_char_f( god, "'%s' is not a valid association number.\n"
          "&+YSyntax: &+wsupervise govern <association number>&+Y.\n"
          "&+YNote: You are%sgoverning an association currently.\n"
          "&+YTo stop governing an association, use &+wsupervise govern 0&+Y.&n\n",
          second, (GET_ASSOC(god) == NULL) ? " &+Wnot&+Y " : " " );
      return;
    }

    GET_ASSOC(god) = guild;
    SET_GOD(GET_A_BITS(god));
    SET_MEMBER(GET_A_BITS(god));
    send_to_char_f( god, "You are now a god of '%s' %d.\n", guild->get_name().c_str(), guild->get_id() );
    return;
  }
  if( is_abbrev(first, "update") )
  {
    if( !is_number(second) || (( guild = get_guild_from_id(atoi( second )) ) == NULL) )
    {
      send_to_char_f( god, "'%s' is not a valid association number.\n"
        "&+YSyntax: &+wsupervise update <association number>&+Y.&n\n", second );
      return;
    }
    count = guild->update( );
    send_to_char_f( god, "%d online member%shave been updated.\n", count, (count == 1) ? " " : "s " );
    return;
  }
  if( is_abbrev(first, "setbit") )
  {
    if( !is_number(second) || (( guild = get_guild_from_id(atoi( second )) ) == NULL) )
    {
      send_to_char_f( god, "'%s' is not a valid association number.\n"
        "&+YSyntax: &+wsupervise setbit <association number> <prestige|construction|frags|racewar> <amount>&+Y.&n\n", second );
      return;
    }

    rest = lohrr_chop( rest, third );
    if( !*third )
    {
      send_to_char_f( god, "&+YPlease enter &+wprestige&+Y, &+wconstruction&+Y, &+wfrags&+Y, or &+wracewar&+Y for which points you want to setbit.\n"
        "&+YSyntax: &+wsupervise setbit <association number> <prestige|construction|frags|racewar> <amount>&+Y.&n\n", second );
      return;
    }
    lohrr_chop( rest, fourth );
    if( !*fourth || !is_real_number(fourth) )
    {
      if( !*fourth )
        send_to_char( "Please enter the amount to which you wish to setbit the guild's points.\n"
          "&+YSyntax: &+wsupervise setbit <association number> <prestige|construction|frags|racewar> <amount>&+Y.&n\n", god );
      else
        send_to_char_f( god, "'%s' is not a valid amount to setbit the guild's points.\n"
          "&+YSyntax: &+wsupervise setbit <association number> <prestige|construction|frags|racewar> <amount>&+Y.&n\n", fourth );
      return;
    }

    if( is_abbrev(third, "prestige") )
    {
      guild->set_prestige( atoi(fourth) );
      guild->save();
      send_to_char_f( god, "You set %s's prestige points to %d.\n", guild->get_name().c_str(), guild->get_prestige() );
      return;
    }
    else if( is_abbrev(third, "construction") )
    {
      guild->set_construction( atoi(fourth) );
      guild->save();
      send_to_char_f( god, "You set %s's construction points to %d.\n", guild->get_name().c_str(), guild->get_construction() );
      return;
    }
    else if( is_abbrev(third, "frags") )
    {
      guild->set_frags( (long)(100. * ( atof(fourth) + .001 )) );
      guild->save();
      send_to_char_f( god, "You set %s's frags to %.2f.\n", guild->get_name().c_str(), guild->get_frags() / 100. );
      return;
    }
    else if( is_abbrev(third, "racewar") )
    {
      bits = atoi(fourth);
      if( (bits <= 0) || (bits > MAX_RACEWAR) )
      {
        send_to_char_f( god, "Invalid racewar side - %d (valid range is from 1 to %d).\n", bits, MAX_RACEWAR );
        return;
      }
      guild->set_racewar( bits );
      guild->save();
      send_to_char_f( god, "You set %s's racewar to %d - &+%c%s&n.\n", guild->get_name().c_str(), bits,
        racewar_color[bits].color, racewar_color[bits].name );
      return;
    }
    else
    {
      send_to_char_f( god, "&+Y'&n%s&+Y' is not a valid point type.\n"
        "&+YPlease enter &+wprestige&+Y, &+wconstruction&+Y, &+wfrags&+Y, or &+wracewar&+Y for which points you want to setbit.\n"
        "&+YSyntax: &+wsupervise setbit <association number> <prestige|construction|frags|racewar> <amount>&+Y.&n\n", third );
      return;
    }
  }
  if( is_abbrev(first, "ban") )
  {
    if( !*second )
    {
      send_to_char( "Missing mortal name to ban.\n", god );
      return;
    }
    if( (member = get_char_vis( god, second )) == NULL )
    {
      send_to_char_f( god, "'%s' doesn't seem to be online.\n", second );
      return;
    }
    if( IS_NPC(member) )
    {
      send_to_char( "You can't ban a NPC.\n", god );
      return;
    }
    if( IS_BANNED(GET_A_BITS( member )) )
    {
      send_to_char( "Whew, the association ban on you is lifted!\n", member );
      SET_NO_THANKS( GET_A_BITS(member) );
      clear_title( member );
      send_to_char_f( god, "You have lifted the association ban on %s.\n", GET_NAME(member) );
      return;
    }
    else
    {
      send_to_char( "Guess what? You are banned from all associations now...\n", member );
      if( IS_MEMBER(GET_A_BITS( member )) )
      {
        GET_ASSOC(member)->kick(member);
        send_to_char( "The members of your former association spit at you!\n", member );
      }
      SET_BANNED( GET_A_BITS(member) );
      clear_title( member );
      GET_TITLE( member ) = str_dup( "&+RAssociation &+YBan&+W!&n" );
      send_to_char_f( god, "You banned %s from all associations!\n", GET_NAME(member) );
      wizlog( GET_LEVEL(god), "%s bans %s from all associations.", GET_NAME(god), GET_NAME(member) );
      logit( LOG_WIZ, "%s bans %s from all associations.", GET_NAME(god), GET_NAME(member) );

      return;
    }
  }
  sprintf( buf, "\n&+RUsage:&n supervise <&ns>ubcommand <argumentlist>\n&+MStandard Guilds and Kingdoms&n\n"
    "&+m============================&n\n<> - displays list of existing associations\n"
    "<&+Mf&n>ound  <leader_name> <bits_string> <asc_name>\n<&+Md&n>elete <asc_number>\n"
    "<&+Mn&n>ame   <asc_number> <new_asc_name>\n<&+Mt&n>ype   <asc_number> <bits_string>\n"
    "<&+Mg&n>overn <asc_number>\n<&+Mu&n>pdate <asc_number>\n"
    "<&+Ms&n>etbit <asc_number> <prestige|construction_points|frags|racewar> <value>\n"
    "<&+Mb&n>an    <mortal_name>\n&+m============================&n\n"
    "   &+y<bits_string>: c = challenge, h = hide title, s = hide subtitle, combine for both, n = none&n\n"
    "   &+y(i.e. sup f johnny ch The Jumpin' Johnnies)&n\n\n" );
  send_to_char( buf, god );
}

void do_gmotd( P_char ch, char *argument, int cmd )
{
  uint     bits;
  P_Guild  guild;
  FILE    *f;
  char     buf[MAX_STRING_LENGTH];
  P_obj    obj;

  if( !IS_ALIVE(ch) && IS_PC(ch) )
  {
    return;
  }

  bits = GET_A_BITS(ch);
  if( !IS_MEMBER(bits) )
  {
    send_to_char("You are not part of an association!\n", ch);
    return;
  }
  guild = GET_ASSOC(ch);
  sprintf(buf, "%sasc.%d.motd", ASC_DIR, guild->get_id() );

  if( !*argument || (!IS_LEADER(bits) && !GT_LEADER(bits) && !IS_TRUSTED(ch)) )
  {
    if( !(f = fopen(buf, "r")) )
    {
      send_to_char("No guild motd.\n", ch);
      return;
    }
    fclose(f);
    send_to_char( "&+C----GUILD MOTD----&N\n", ch );
    send_to_char( file_to_string(buf), ch );
    send_to_char( "&+C------------------&N\n", ch );
    return;
  }
  obj = get_obj_in_list_vis( ch, argument, ch->carrying );
  if( !obj )
  {
    send_to_char_f( ch, "'%s' not found!\n", argument );
    send_to_char( "You need a piece of paper with the new gmotd written on it.\n"
      "Try '&+whelp write&n' for more info.\n", ch );
    return;
  }
  if (obj->type != ITEM_NOTE)
  {
    send_to_char_f( ch, "%s must be a piece of paper with your guild motd!\n", OBJ_SHORT(obj) );
    return;
  }
  if( !obj->action_description )
  {
    send_to_char("Paper is empty, no empty gmotd allowed!!\nTry '&+whelp write&n' for more info.\n", ch);
    return;
  }
  f = fopen(buf, "w");
  fprintf(f, "%s\n", obj->action_description);
  fclose(f);
  obj_from_char(obj);
  extract_obj(obj);
  send_to_char("Guild Motd Updated.\n", ch);
}

void do_prestige( P_char ch, char *argument, int cmd )
{
  MYSQL_RES *res;
  MYSQL_ROW  row;
  char       buf[MAX_STRING_LENGTH];
  int        id, prestige, cps, threshold;
  string     name;

  if( !qry("SELECT id, name, prestige, construction_points FROM associations WHERE active = 1 ORDER BY prestige DESC, id ASC") )
  {
    send_to_char("Disabled.\n", ch);
    return;
  }

  res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    send_to_char("There are no prestigious associations.\n", ch);
    return;
  }

  threshold = get_property("prestige.list.viewThreshold", 500);

  send_to_char("&+bPrestigious Associations\n"
    "&+W--------------------------------------------------------------\n", ch);

  while( row = mysql_fetch_row(res) )
  {
    id = atoi(row[0]);
    name = pad_ansi(trim(string(row[1]), " \t\n").c_str(), 40);
    prestige = atoi(row[2]);
    cps = atoi(row[3]);
    if( IS_TRUSTED(ch) )
    {
      sprintf(buf, "&+W%2d. &n%s &n(&+b%8d&n:&+W%8d&n)\n", id, name.c_str(), prestige, cps);
    }
    else
    {
      if( prestige < threshold )
      {
        continue;
      }

      sprintf(buf, "%s\n", name.c_str());
    }
    send_to_char(buf, ch);
  }
  mysql_free_result(res);
}


// This converts a string to money values.
bool str_to_money( char *string, int *pc, int *gc, int *sc, int *cc )
{

  char buf[MAX_INPUT_LENGTH], *rest;
  int amount, type;

  *pc = *gc = *sc = *cc = 0;

  // No string, FAIL.
  if( !*string )
    return FALSE;

  rest = string;
  do
  {
    rest = lohrr_chop( rest, buf );
    // If we don't have a valid number of coins, FAIL.
    if( !is_number(buf) )
    {
      return FALSE;
    }
    amount = atoi( buf );
    rest = lohrr_chop( rest, buf );
    switch( coin_type(buf) )
    {
    case COIN_PLATINUM:
      *pc += amount;
      break;
    case COIN_GOLD:
      *gc += amount;
      break;
    case COIN_SILVER:
      *sc += amount;
      break;
    case COIN_COPPER:
      *cc += amount;
      break;
    default:
      // Bad coin type, FAIL.
      return FALSE;
    }
  }
  while( *rest );

  // Processed all input, WIN.
  return TRUE;
}

#define SOC_CMD_NONE        0
#define SOC_CMD_APPLY       1
#define SOC_CMD_CHALLENGE   2
#define SOC_CMD_DEPOSIT     3
#define SOC_CMD_ENROLL      4
#define SOC_CMD_FINE        5
#define SOC_CMD_HELP        6
#define SOC_CMD_HOME        7
#define SOC_CMD_KICK        8
#define SOC_CMD_LEDGER      9
#define SOC_CMD_NAME       10
#define SOC_CMD_OSTRACIZE  11
#define SOC_CMD_PUNISH     12
#define SOC_CMD_RANK       13
#define SOC_CMD_SECEDE     14
#define SOC_CMD_TITLE      15
#define SOC_CMD_TOGGLE     16
#define SOC_CMD_WITHDRAW   17
// Performs various society (guild/association) commands.
void do_society( P_char member, char *argument, int cmd )
{
  time_t  temp_time;
  char   *timestr, buf[MAX_STRING_LENGTH];
  char    first[MAX_INPUT_LENGTH], second[MAX_INPUT_LENGTH], third[MAX_INPUT_LENGTH];
  char    fourth[MAX_INPUT_LENGTH], *rest;
  int     command, platinum, gold, silver, copper;
  P_char  victim;
  P_Guild guild;

  guild = GET_ASSOC(member);

  if( IS_BANNED(GET_A_BITS( member )) )
  {
    temp_time = GET_TIME_LEFT_GUILD(member);

    if( temp_time <= 0 )
    {
      send_to_char("You are banned from all associations, so sod off!\n", member);
      return;
    }
    if( GET_NB_LEFT_GUILD(member) <= 0 )
    {
      send_to_char("You have been banned from all associations, so sod off!\n", member);
      return;
    }
    temp_time += SECS_PER_REAL_DAY * get_property("guild.secede.delay.days", 2);

    if( time(NULL) >= temp_time )
    {
      send_to_char("Look like you can join another guild!\n", member);
      SET_NO_THANKS(GET_A_BITS(member));
    }
    else
    {
      timestr = asctime(localtime(&(temp_time)));
      timestr[10] = '\0';
      sprintf(buf, "You cannot join another guild until %s\n", timestr);
      send_to_char(buf, member);
      return;
    }
  }

  if( GET_LEVEL(member) < ASC_MIN_LEVEL )
  {
    if( guild )
      guild->update_member(member);
    return;
  }

  rest = lohrr_chop( argument, first );

  if( !*first )
  {
    if( !guild || !IS_MEMBER(GET_A_BITS( member )) )
    {
      send_to_char("How about joining an association first?\n", member);
      return;
    }
    guild->display(member);
    return;
  }

  // Figure out what the command was.
  command = SOC_CMD_NONE;
  if( is_abbrev(first, "apply") )
  {
    command = SOC_CMD_APPLY;
  }
  else if( is_abbrev(first, "challenge") )
  {
    command = SOC_CMD_CHALLENGE;
  }
  else if( is_abbrev(first, "deposit") )
  {
    command = SOC_CMD_DEPOSIT;
  }
  else if( is_abbrev(first, "enroll") )
  {
    command = SOC_CMD_ENROLL;
  }
  else if( is_abbrev(first, "fine") )
  {
    command = SOC_CMD_FINE;
  }
  else if( is_abbrev(first, "home") )
  {
    command = SOC_CMD_HOME;
  }
  // Help is out of order here intentionally!  We want 'soc h' to be 'soc home'.
  else if( is_abbrev(first, "help") || (first[0] == '?') )
  {
    command = SOC_CMD_HELP;
  }
  else if( is_abbrev(first, "kick") )
  {
    command = SOC_CMD_KICK;
  }
  else if( is_abbrev(first, "ledger") )
  {
    command = SOC_CMD_LEDGER;
  }
  else if( is_abbrev(first, "name") )
  {
    command = SOC_CMD_NAME;
  }
  else if( is_abbrev(first, "secede") )
  {
    command = SOC_CMD_SECEDE;
  }
  else if( is_abbrev(first, "ostracize") )
  {
    command = SOC_CMD_OSTRACIZE;
  }
  else if( is_abbrev(first, "punish") )
  {
    command = SOC_CMD_PUNISH;
  }
  else if( is_abbrev(first, "rank") )
  {
    command = SOC_CMD_RANK;
  }
  else if( is_abbrev(first, "title") )
  {
    command = SOC_CMD_TITLE;
  }
  // The "g" is from the old days, where we only looked at the first letter,
  //   so it had to be unique (t = title and o = ostracize).
  else if( is_abbrev(first, "toggle") || !strcmp( first, "g") )
  {
    command = SOC_CMD_TOGGLE;
  }
  else if( is_abbrev(first, "withdraw") )
  {
    command = SOC_CMD_WITHDRAW;
  }
  else
  {
    send_to_char_f( member, "'%s' is an unknown command.\n", first );
    command = SOC_CMD_HELP;
  }

  // The only command that doesn't require being in a guild already, is soc apply (to a guild).
  if( command != SOC_CMD_APPLY )
  {
    if( guild == NULL )
    {
      act("Try joining a guild first.", FALSE, member, NULL, NULL, TO_CHAR);
      return;
    }
  }

  // Pull second argument as PC victim if appropriate.
  // Pull second argument as coin string if appropriate.
  // Pull second argument as PC and third argument as coin string for fine.
  switch( command )
  {
    case SOC_CMD_APPLY:
    case SOC_CMD_CHALLENGE:
    case SOC_CMD_ENROLL:
    case SOC_CMD_KICK:
    case SOC_CMD_PUNISH:
    case SOC_CMD_RANK:
    case SOC_CMD_TITLE:
      rest = lohrr_chop( rest, second );
      if( !*second )
      {
        send_to_char( "Do what?  To whom??  Behind what barn???\n", member );
        return;
      }
      // get_char_room_vis checks for 'me' and 'self'.
      if( (victim = get_char_vis( member, second )) == NULL )
      {
        if( command == SOC_CMD_KICK )
        {
          guild->kick( member, second );
          return;
        }
        send_to_char_f( member, "Could not find player '%s'.\n", second );
        return;
      }
      else if( IS_NPC(victim) )
      {
        send_to_char( "We do &-Lnot&N preform society commands on mobs.\n", member );
        return;
      }
      break;
    case SOC_CMD_DEPOSIT:
    case SOC_CMD_WITHDRAW:
      if( !str_to_money(rest, &platinum, &gold, &silver, &copper) )
      {
        send_to_char_f( member, "Failed to interpret coin string '%s'.\n", rest );
        send_to_char( "A valid coin string is in the format '&+w<number> <coin type>&n' which can repeat for multiple coin types.\n", member );
        return;
      }
      if( (platinum < 0) || (gold < 0) || (silver < 0) || (copper < 0) )
      {
        send_to_char( "No negative transactions.\n", member );
        return;
      }
      if( !test_atm_present(member) )
      {
        send_to_char( "I don't see a bank around here.\n", member );
        return;
      }
      if( (platinum == 0) && (gold == 0) && (silver == 0) && (copper == 0) )
      {
        send_to_char( "Don't be silly.\n", member );
        return;
      }
      break;
    case SOC_CMD_FINE:
      rest = lohrr_chop( rest, second );
      if( !*second )
      {
        send_to_char( "Do what?  To whom??  Behind what barn???\n", member );
        return;
      }
      if( !str_to_money(rest, &platinum, &gold, &silver, &copper) )
      {
        send_to_char_f( member, "Failed to interpret coin string '%s'.\n", rest );
        send_to_char( "A valid coin string is in the format '&+w<number> <coin type>&n' which can repeat for multiple coin types.\n", member );
        return;
      }
      break;
    default:
      break;
  }

  switch( command )
  {
    case SOC_CMD_APPLY:
      // If member == victim then clearing application.
      if( (guild = GET_ASSOC( victim )) != NULL )
      {
        // This is a little fuzzy, member is applying to victim's guild.
        guild->apply( member, victim );
      }
      else
      {
        if( member == victim )
          send_to_char( "You're not applying to any guilds atm.\n", member );
        else
          act("$N is not in a guild for you to apply to.", FALSE, member, NULL, victim, TO_CHAR);
      }
      break;
    case SOC_CMD_CHALLENGE:
      if( guild != GET_ASSOC(victim) )
      {
        act("$N is not your guild.", FALSE, member, NULL, victim, TO_CHAR);
        return;
      }
      else
      {
        guild->challenge( member, victim );
      }
      break;
    case SOC_CMD_DEPOSIT:
      guild->deposit( member, platinum, gold, silver, copper );
      break;
    case SOC_CMD_ENROLL:
      if( !IS_APPLICANT(GET_A_BITS( victim )) || (guild != GET_ASSOC( victim )) )
      {
        act("$N does not want to be in your guild.", FALSE, member, NULL, victim, TO_CHAR);
        return;
      }
      guild->enroll( member, victim );
      break;
    case SOC_CMD_FINE:
      guild->fine( member, second, platinum, gold, silver, copper );
      break;
    // SOC_CMD_NONE is out of order 'cause we want to fall through to SOC_CMD_HELP.
    //   Also, this should never show up anyway.
    case SOC_CMD_NONE:
      send_to_char( "\n&+RHuh?&n\n", member );
    case SOC_CMD_HELP:
      // TASFALEN
      strcpy(buf, "\n&+YSociety command list&n\n&+y====================\n"
        "<> - displays list of members and other info\n"
        "<&+Ya&n>pply     <member>    - apply for membership to member\n"
        "                          'apply me' to clear application\n"
        "<&+Ye&n>nrol     <applicant> - enrol an applicant as member\n"
        "<&+Yk&n>ickout   <member>    - delete a member from the books\n"
        "<&+Ys&n>ecede    [confirm]   - leave association\n"
        "<&+Yr&n>ank      <member> <levels> - changes rank by levels\n"
        "<&+Yt&n>itle     <member>    - sets title of specific member\n"
        "to<&+Yg&n>gle    retitle     - toggles title change allowance\n"
        "<&+Yp&n>unish    <member>    - 1st punish=parole, 2nd=enemy\n"
        "<&+Yc&n>hallenge <member>    - challenge member for his rank\n"
        "<&+Yo&n>stracize <name>      - ostracize a non-member\n"
        "<&+Yn&n>ame      <rank_keyword> <new_rank_name> - rename a rank\n"
        "<&+Yf&n>ine      <member> <money> - fine member, 'all' allowed\n"
        "<&+Yd&n>eposit   <money>     - give money to guild\n"
        "<&+Yw&n>ithdraw  <money>     - get money from guild\n"
        "<&+Yh&n>ome                  - set hometown to current room, must be in guild hall\n"
        "<&+Yl&n>edger                - view the list of money transactions\n"
        "&+y====================\n\n"
        "<&+y?&n> - this help\n\nNote: You can now '&+wsoc ledger&n' as well as '&+wsoc l'.\n");
        send_to_char( buf, member );
      break;
    case SOC_CMD_HOME:
      guild->home( member );
      break;
    case SOC_CMD_KICK:
      if( guild != GET_ASSOC(victim) )
      {
        act("$N is not in your guild.", FALSE, member, NULL, victim, TO_CHAR);
        return;
      }
      if( A_GET_RANK(member) < A_OFFICER )
      {
        send_to_char( "You rank is too low. Forget it.\n", member );
        return;
      }
      if( IS_DEBT(GET_A_BITS( member )) && !IS_TRUSTED(member) )
      {
        send_to_char("Pay your dues! Let's see some cash...\n", member);
        return;
      }
      if( victim == member )
      {
        send_to_char( "You can't kick yourself out, try &+wsoc secede&n.\n", member );
        return;
      }
      if( A_GET_RANK(member) <= A_GET_RANK(victim) )
      {
        send_to_char( "You can only kick out people below your rank!\n", member );
        return;
      }
      guild->kick( victim );
      send_to_char( "That member is gone for good!\n", member );
      break;
    case SOC_CMD_LEDGER:
      guild->ledger( member, rest );
      break;
    case SOC_CMD_NAME:
      guild->name_title( member, rest );
      break;
    case SOC_CMD_OSTRACIZE:
      guild->ostracize( member, rest );
      break;
    case SOC_CMD_PUNISH:
      guild->punish( member, victim );
      break;
    case SOC_CMD_RANK:
      guild->rank( member, victim, rest );
      break;
    case SOC_CMD_SECEDE:
      rest = lohrr_chop( rest, second );
      if( strcmp(second, "confirm") )
      {
        send_to_char( "Use 'secede confirm' to really leave.\n", member );
        return;
      }
      guild->secede( member );
      break;
    case SOC_CMD_TITLE:
      guild->title( member, victim, rest );
      break;
    case SOC_CMD_TOGGLE:
      GET_A_BITS(member) ^= A_STATICTITLE;
      if( GET_A_BITS(member) & A_STATICTITLE )
      {
        send_to_char( "Your title will now not be changed by the leader/automatic rank title updates.\n", member );
      }
      else
      {
        send_to_char( "Your title can now be changed by the leader/automatic rank title updates.\n", member );
      }
      guild->update_member(member);
      guild->save( );
      break;
    case SOC_CMD_WITHDRAW:
      guild->withdraw( member, platinum, gold, silver, copper );
      break;
  }
}

// Assumes member and victim are in the same guild (checked in do_society).
void Guild::challenge( P_char member, P_char victim )
{
  int      mem_bits = GET_A_BITS(member), old_member_rank = A_GET_RANK(member);
  char    *char_name;
  P_member pMembers;

  // Check rank first.
  if( !IS_MEMBER(mem_bits) )
  {
    send_to_char( "How about joining an association first?\n", member );
    return;
  }
  if( member == victim )
  {
    send_to_char( "Yes, you are challenged, mentally challenged...\n", member );
    return;
  }
  // If they're parole / enemy.
  if( A_RANK_BITS(mem_bits) < A_NORMAL )
  {
    send_to_char( "Nobody would accept you in a higher rank anyway.\n", member );
    return;
  }
  // If guild doesn't allow challenges.
  if( !(IS_CHALL( bits )) )
  {
    send_to_char( "Your association frowns upon forcing your way up.\n", member );
    return;
  }
  if( NR_RANK(mem_bits) != (NR_RANK( GET_A_BITS(victim) ) + 1) )
  {
    send_to_char( "Try challenging someone of the &+Wnext higher&n rank.\n", member );
    return;
  }
  act("You try to challenge $N for $S position.", FALSE, member, NULL, victim, TO_CHAR);
  act("$n challenges you for your guild rank.", FALSE, member, NULL, victim, TO_VICT);

  // Lower levels or lots less exp fails (56k to 44.8M difference @ lvl 56, max 40M @ lvl 50).
  if( (GET_LEVEL( member ) < GET_LEVEL( victim )) || (GET_EXP( member ) < GET_EXP( victim ) - GET_LEVEL( victim ) * number( 10000, 800000 )) )
  {
    send_to_char( "You suddenly are very, very scared and give up!\n", member );
    act("$n blushes and avoids your stern glance.", FALSE, member, NULL, victim, TO_VICT);
    return;
  }
  // Harder at higher levels.
  if( number(0, 1 + ( GET_LEVEL(member) / 10 )) )
  {
    send_to_char( "You plans are thwarted!\n", member );
    act("$n's feeble attempt to challenge you has failed.\n", FALSE, member, NULL, victim, TO_VICT);
    return;
  }

  // Now the fun part, need to change guild ranks.
  // Do the actual re-ranking:
  GET_A_BITS(member) += A_RK1;
  GET_A_BITS(victim) -= A_RK1;
  // Find member in member list.
  char_name = GET_NAME(member);
  pMembers = members;
  while( strcmp(pMembers->name, char_name) )
  {
    if( (pMembers = pMembers->next) == NULL )
    {
      logit(LOG_DEBUG, "Guild::challenge: member '%s' %d not on guild %d's member list.", char_name, GET_ID(member), id_number );
      debug( "Guild::challenge: member '%s' %d not on guild %d's member list.", char_name, GET_ID(member), id_number );
      break;
    }
  }
  if( pMembers != NULL )
  {
    pMembers->bits = GET_A_BITS(member);
  }
  // Find victim in member list.
  char_name = GET_NAME(victim);
  pMembers = members;
  while( strcmp(pMembers->name, char_name) )
  {
    if( (pMembers = pMembers->next) == NULL )
    {
      logit(LOG_DEBUG, "Guild::challenge: victim '%s' %d not on guild %d's member list.", char_name, GET_ID(victim), id_number );
      debug( "Guild::challenge: victim '%s' %d not on guild %d's member list.", char_name, GET_ID(victim), id_number );
      break;
    }
  }
  if( pMembers != NULL )
  {
    pMembers->bits = GET_A_BITS(victim);
  }
  save( );
}

void Guild::write_transaction_to_ledger( char *name, char *trans_type, char *coin_str )
{
  qry("INSERT INTO guild_transactions (date, soc_id, transaction_info) VALUES (unix_timestamp(), %d, '&+y%s %s %s&+y.&n')",
    id_number, name, trans_type, coin_str);
}


// The coins are valid (positive numbers) via checks in do_society().
void Guild::deposit( P_char member, int p, int g, int s, int c )
{
  char *char_name;
  P_member pMembers;
  int iDeposit;

  // Verify they have enough money.
  if( (GET_PLATINUM( member ) < p) || (GET_GOLD( member ) < g) || (GET_SILVER( member ) < s) || (GET_COPPER( member ) < c) )
  {
    send_to_char( "You don't have that much coin on you!\n", member );
    return;
  }

  // Move the money.
  GET_PLATINUM( member ) -= p;
  platinum += p;
  GET_GOLD( member ) -= g;
  gold += g;
  GET_SILVER( member ) -= s;
  silver += s;
  GET_COPPER( member ) -= c;
  copper += c;

  send_to_char_f( member, "&+yYou deposited &+W%dp&n&+y, &+Y%dg&n&+y, &+w%ds&n&+y, and &+y%dc to %s&+y.\n",
    p, g, s, c, name );
  write_transaction_to_ledger( GET_NAME(member), "deposited", coins_to_string(p, g, s, c, "&+y") );

  // Lower fine if applicable.
  if( HAS_FINE( member ) )
  {
    char_name = GET_NAME(member);
    for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
    {
      if( !strcmp(char_name, pMembers->name) )
      {
        iDeposit = (1000 * p) + (100 * g) + (10 * s) + c;
        if( pMembers->debt <= iDeposit )
        {
          pMembers->debt = 0;
          // Remove debt flag from member list.
          REMOVE_DEBT( pMembers->bits );
          // Remove debt flag from member's bits.
          REMOVE_DEBT( GET_A_BITS(member) );
          send_to_char( "Your debt is paid off.. whew.\n", member );
        }
        else
        {
          pMembers->debt -= iDeposit;
          send_to_char( "You still owe the association some money.\n", member );
        }
        break;
      }
    }
  }

  // Save character
  writeCharacter(member, RENT_CRASH, member->in_room );
  // Save guild.
  save( );
}

// We know that member is a member of the guild that victim is applying for via checks in do_society.
void Guild::enroll( P_char member, P_char victim )
{
  int mem_bits = GET_A_BITS(member), vict_bits;
  // Only those "greater than" normal rank can enroll.
  if( !GT_NORMAL(mem_bits) )
  {
    send_to_char( "Maybe one day you will be trusted enough to do that.\n", member );
    return;
  }
  // Illithids must be lvl 40 to get guilded.
  if( IS_ILLITHID(victim) && GET_LEVEL(victim) < 40 )
  {
    send_to_char( "Your applicant is too weak to leave the &+Melder brain&n.\n", member );
    return;
  }
  if( !IS_APPLICANT(GET_A_BITS( victim )) )
  {
    send_to_char( "Don't bug other people...\n", member );
    return;
  }
  if( IS_DEBT(mem_bits) )
  {
    send_to_char( "Pay your dues! Let's see some cash first...\n", member );
    return;
  }

  if( member_count >= max_assoc_size() )
  {
    send_to_char( "Your association cannot grow any further until it gains some prestige!\r\n", member );
    return;
  }

  // Have to clear guild so add_member doesn't freak out.
  GET_ASSOC(victim) = NULL;
  add_member( victim, A_NORMAL );
  send_to_char( "Yeah! Another member!\n", member );
  save( );
}

// We know that member and victim are in the same guild (or victim is NULL), and that p/g/s/c are valid coin values
//   all from checks in do_society.
void Guild::fine( P_char member, char *target, int p, int g, int s, int c )
{
  unsigned int mem_bits = GET_A_BITS( member );
  long fine_in_copper = (1000 * p) + (100 * g) + (10 * s) + c;
  long new_debt;
  P_member pMembers;
  P_char victim;

  // Only officers and above can fine people.
  if( !GT_SENIOR(mem_bits) )
  {
    send_to_char( "You need to advance in rank to do this.\n", member );
    return;
  }
  if( IS_DEBT(mem_bits) )
  {
    send_to_char( "Pay your dues! Let's see some cash first...\n", member );
    return;
  }
  // Handle all down below.
  if( strcmp(target, "all") )
  {
    CAP(target);
    for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
    {
      if( !strcmp(pMembers->name, target) )
      {
        new_debt = pMembers->debt + fine_in_copper;
        if( new_debt < 0 )
        {
          pMembers->debt = 0;
        }
        else
        {
          pMembers->debt = new_debt;
        }
        if( pMembers->debt == 0 )
        {
          send_to_char_f( member, "You have cleared %s's fine.\n", pMembers->name );
          REMOVE_DEBT( pMembers->bits );
        }
        else
        {
          send_to_char_f( member, "You changed %s's debt to %s.\n", pMembers->name, coin_stringv(pMembers->debt, 0) );
          SET_DEBT( pMembers->bits );
        }
        if( (victim = get_char_online( pMembers->name )) )
        {
          update_bits( victim );
        }
        save( );
        return;
      }
    }
    send_to_char_f( member, "Could not find player '%s'.\n", target );
    return;
  }
  // If all then we walk through each member...
  for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
  {
    if( A_RANK_BITS(pMembers->bits) >= A_LEADER )
      continue;

    new_debt = pMembers->debt + fine_in_copper;

    if( new_debt < 0 )
    {
      pMembers->debt = 0;
    }
    else
    {
      pMembers->debt = new_debt;
    }
    if( pMembers->debt == 0 )
    {
      send_to_char_f( member, "You have cleared %s's fine.\n", pMembers->name );
      REMOVE_DEBT( pMembers->bits );
    }
    else
    {
      send_to_char_f( member, "You changed %s's debt to %s.\n", pMembers->name, coin_stringv(pMembers->debt, 0) );
      SET_DEBT( pMembers->bits );
    }
    if( (victim = get_char_online( pMembers->name )) )
    {
      update_bits( victim );
    }
  }
  save( );
}

// Updates where a char respawns when they die.
void Guild::home( P_char member )
{
  Guildhall* gh = Guildhall::find_by_vnum(world[member->in_room].number);
  int inn_vnum;

  if( gh == NULL )
  {
    send_to_char( "You're not in a guildhall.\n", member );
    return;
  }

  if( this != gh->get_assoc() )
  {
    send_to_char( "You can only home in your own guildhall.\n", member );
    return;
  }
  if( (inn_vnum = gh->inn_vnum( )) == 0 )
  {
    send_to_char( "Your guildhall has no inn!\n", member );
    return;
  }
  member->player.birthplace = member->player.hometown = inn_vnum;
  writeCharacter(member, RENT_CRASH, member->in_room );
  send_to_char_f( member, "Your new home is in %s.\n", world[real_room0(inn_vnum)].name );
}

// We already know member is a member of this because of the checks in do_society.
void Guild::ledger( P_char member, char *args )
{
  int member_bits = GET_A_BITS(member);
  char buff[MAX_STRING_LENGTH];
  MYSQL_RES *res;
  MYSQL_ROW row;

  if( !IS_LEADER(member_bits) && !IS_TRUSTED(member) )
  {
    send_to_char("You must be a leader to look at such things!\r\n", member );
    return;
  }

  while( isspace(*args) )
  {
    args++;
  }

  if( !args || is_abbrev(args, "player") )
  {
    if( !qry("SELECT transaction_info FROM guild_transactions WHERE soc_id = %d AND transaction_info NOT LIKE '%%System withdrew%%' ORDER BY date DESC LIMIT 100", id_number) )
    {
      send_to_char("No transactions found..\n", member );
      return;
    }
  }
  else if( is_abbrev(args, "system") || is_abbrev(args, "guild") )
  {
    if( !qry("SELECT transaction_info FROM guild_transactions WHERE soc_id = %d AND (transaction_info LIKE '%%System %%') ORDER BY date DESC LIMIT 100", id_number) )
    {
      send_to_char("No transactions found...\n", member );
      return;
    }
  }
  else
  {
    send_to_char("&+YSyntax: &+wsoc l [player|system|guild]&n\n", member );
    send_to_char("  Player - for a list of player transactions.\n", member );
    send_to_char("  System or guild - for a list of automated transactions.\n", member );
    return;
  }

  send_to_char("&+YGuild Ledger:\r\n------------------------------\r\n", member );

  res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    send_to_char("&+yNo transactions on record.\r\n", member );
    mysql_free_result(res);
    return;
  }
  while( row = mysql_fetch_row(res) )
  {
    sprintf(buff, "%s\r\n", row[0]);
    send_to_char(buff, member );
  }

  mysql_free_result(res);
}

// While this theoretically works, it has not been tested since
//   ostracize was disabled in the old code.
void Guild::ostracize( P_char member, char *target )
{
  P_member pMembers, new_member;

  send_to_char( "Temporarily disabled.\n", member );
  return;

  while( isspace(*target) )
  {
    target++;
  }

  // Same function as in char creation.
  if( strlen(target) > MAX_NAME_LENGTH )
  {
    send_to_char( "That's not a valid name.\n", member );
    return;
  }

  // Make sure char exists.
  if( !pfile_exists(SAVE_DIR, target) )
  {
    send_to_char( "No idea who that is.\n", member );
    return;
  }

  for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
  {
    if( !strcmp(target, pMembers->name) )
    {
      send_to_char( "That person is in the books already.\n", member );
      return;
    }
  }
  // Add to member list.
  new_member = new guild_member;
  strcpy(new_member->name, target );
  new_member->bits = A_ENEMY;
  new_member->debt = 0;
  new_member->next = members;
  members = new_member;
  member_count++;

}

// Puts them on parole.
void Guild::punish( P_char member, P_char victim )
{
  P_member pMembers;
  char    *vict_name = GET_NAME(victim);

  // Set bits in member list.
  for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
  {
    if( !strcmp(vict_name, pMembers->name) )
    {
      if( IS_PAROLE(pMembers->bits) )
      {
        send_to_char( "They're already on parole.\n", member );
        return;
      }
      SET_PAROLE( pMembers->bits );
      break;
    }
  }
  // Set bits on victim.
  SET_PAROLE( GET_A_BITS(victim) );
  // Save character
  writeCharacter(victim, RENT_CRASH, victim->in_room );
  // Save guild.
  save( );

  // Send messages.
  act("You put $N on parole.", FALSE, member, NULL, victim, TO_CHAR);
  act("$n put you on parole.", FALSE, member, NULL, victim, TO_VICT);
}

// Raise or lower someone's rank.
// We know both member and victim are in the guild because of checks in do_society.
void Guild::rank( P_char member, P_char victim, char *rank_change )
{
  int      rank_delta, vict_bits = GET_A_BITS(victim), memb_bits = GET_A_BITS(member);
  char    *vict_name = GET_NAME(victim);
  P_member pMembers;

  // Only Deputy / Leader / God.
  if( !IS_MEMBER(memb_bits) || !GT_OFFICER(memb_bits) )
  {
    send_to_char( "You need to be high up in the ranks to do this.\n", member );
    return;
  }

  if( HAS_FINE(member) )
  {
    send_to_char( "Pay your dues!  Let's see some cash first...\n", member );
    return;
  }

  if( !IS_MEMBER(vict_bits) )
  {
    send_to_char( "They need to be an actual member of the guild first (maybe &+wsoc enroll&n?).\n", member );
    return;
  }

  if( GET_RK_BITS(vict_bits) > GET_RK_BITS(memb_bits) )
  {
    send_to_char( "You can not change the rank of a superior.\n", member );
    return;
  }

  while( isspace(*rank_change) )
  {
    rank_change++;
  }

  // Easy check.
  if( !is_number(rank_change) || (( rank_delta = atoi(rank_change) ) == 0) )
  {
    send_to_char( "That's not a valid change.  Please enter a non-zero amount to change the rank.\n", member );
    return;
  }
  // Move the bits up to the rank bits.
  rank_delta *= A_RK1;
  // Hard check: Is new rank above leader or below member?  Then fail.
  if( (( rank_delta + GET_RK_BITS(vict_bits) ) > A_LEADER) || (( rank_delta + GET_RK_BITS(vict_bits) ) < A_NORMAL) )
  {
    send_to_char( "Choose a sensible rank change.\n", member );
    return;
  }
  // Trying to over-promote
  if( (rank_delta + GET_RK_BITS( vict_bits )) > GET_RK_BITS(memb_bits) )
  {
    send_to_char( "You can't promote someone to a rank above yours.\n", member );
    return;
  }
  // If we're demoting someone of the same rank, we must be a leader.
  if( (GET_RK_BITS( memb_bits ) < A_LEADER) && (GET_RK_BITS( memb_bits ) == GET_RK_BITS( vict_bits ))
    && (rank_delta < 0) )
  {
    send_to_char("You can't demote someone of your own rank unless you're a leader.", member );
    return;
  }

  // Update member
  GET_A_BITS(victim) += rank_delta;

  // Update member list
  for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
  {
    if( !strcmp(vict_name, pMembers->name) )
    {
      pMembers->bits += rank_delta;
      break;
    }
  }

  send_to_char_f( member, "You %s %s to %s %s.\n", (( rank_delta < 0 ) ? "demoted" : "promoted"),
    vict_name, titles[NR_RANK(GET_A_BITS( victim ))], name );
  send_to_char_f( victim, "You have been %s.\n", (rank_delta < 0) ? "demoted" : "promoted" );

  // Save character
  writeCharacter(victim, RENT_CRASH, victim->in_room );
  // Save guild.
  save( );
}

// member is leaving the guild.
void Guild::secede( P_char member )
{
  Guildhall *gh;

  if( this != GET_ASSOC(member) )
  {
    debug( "Trying to secede '%s' %d from guild '%d' when they are in guild '%d'",
      J_NAME(member), GET_ID(member), get_id(), GET_ASSOC(member)->get_id() );
    return;
  }

  if( member_count < 2 )
  {
    send_to_char( "You're the last one; ask a god to delete the association instead!\n", member );
    return;
  }

  GET_ASSOC(member) = NULL;
  GET_NB_LEFT_GUILD(member) += 1;
  GET_TIME_LEFT_GUILD(member) = time(NULL);
  SET_NO_THANKS(GET_A_BITS(member));
  clear_title(member);

  send_to_char( "You are on your own once more.\n", member );

  if( (gh = Guildhall::find_by_vnum( GET_BIRTHPLACE(member) )) != NULL )
  {
    send_to_char( "You feel as if you've lost your home.\n", member );
    GET_BIRTHPLACE(member) = GET_HOME(member) = GET_ORIG_BIRTHPLACE(member);
  }

  member_count--;
  remove_member_from_list( member );
  save( );
  writeCharacter( member, RENT_CRASH, member->in_room );
}

void Guild::title( P_char member, P_char victim, char *arg )
{
  char new_title[MAX_STRING_LENGTH];

  // Verify that member can change victim's title.
  if( IS_HIDDENTITLE(bits) && !IS_TRUSTED(member) )
  {
    send_to_char( "Your association is flagged no-title.\n", member );
    return;
  }

  // Guild not allowed subtitles.
  if( bits & A_HIDESUBTITLE )
  {
    if( IS_TRUSTED(member) )
    {
      send_to_char_f( member, "%s is not allowed to use sub titles - but you don't care.\n", name );
    }
    else
    {
      send_to_char_f( member, "%s is not allowed to use sub titles.\n", name );
      return;
    }
  }

  // Verify rank is high enough to set title.
  if( GET_RK_BITS(GET_A_BITS( member )) < A_OFFICER )
  {
    send_to_char( "Maybe one day you will be trusted enough to do that.\n", member );
    return;
  }
  if( (GET_RK_BITS( GET_A_BITS(member) ) == A_OFFICER) && (member != victim) )
  {
    send_to_char( "You can only set your own title at this rank.\n", member );
    return;
  }

  //   Verify member rank vs victim rank.
  if( GET_RK_BITS(GET_A_BITS( member )) < GET_RK_BITS(GET_A_BITS( victim )) )
  {
    send_to_char( "You may not set your superior's title.\n", member );
    return;
  }

  // No title flag on victim.
  if( GET_A_BITS(victim) & A_STATICTITLE )
  {
    if( IS_TRUSTED(member) )
    {
      send_to_char( "Their 'no title modification' flag is on - but you don't care.\n", member );
    }
    else
    {
      send_to_char( "Their 'no title modification' flag is on - no title change allowed.\n", member );
      return;
    }
  }

  title_trim( arg, new_title );

  if( *new_title == '\0' )
  {
    send_to_char( "Error with creating new title (was empty after examination).\n", member );
    return;
  }

  // Remove old title (if exists).
  clear_title( victim );
  // Add new title.
  GET_TITLE(victim) = str_dup(new_title);
  // Save it.
  writeCharacter(victim, RENT_CRASH, victim->in_room );

  send_to_char_f( victim, "You are now known as '%s'.\n", GET_TITLE(victim) );
  if( victim != member )
    send_to_char_f( member, "%s is now known as '%s'.\n", GET_NAME(victim), GET_TITLE(victim) );

  if( !(GET_A_BITS(victim) & A_STATICTITLE) )
  {
    send_to_char( "\n&+WWarning:&n Without your 'no title mod' toggle on, your title will be\n"
      "         reset when you next rent/camp.  Type 'soc g' to toggle it.\n", victim );
  }

  send_to_char( "\n&+WNOTE:&n An inappropriate title (making your guild name appear to be\n"
    "something else, 'offensive' language, etc) will likely cause you to be\n"
    "kicked from your association.\n", member);
}

void Guild::title_trim( char *raw_title, char *good_title )
{
  int guild_name_printed = strlen(strip_ansi(name).c_str());
  // Assuming 120 columns on avg screen
  // Who format: "[## <16 char class> ] <char name max 12> <char title> <guild name>"
  // So, 120 - 23 for level and class - 12 for char name - 2 for the spaces before and after title = 83.
  // But then you have race and prestige rank and (inv), (AFK), etc perhaps... so I dropped the 83 to 50.
  int printed_chars_left = 50 - guild_name_printed;
  bool colored;
  char *index;

  // Skip leading spaces first for the new title.
  while( isspace(*raw_title) )
    raw_title++;

  // Walk through keeping track of ansi state.
  // This is not simple: The big question is where/how to truncate.
  for( colored = FALSE, index = raw_title; (*index != '\0' && printed_chars_left > 0); )
  {
    // Possible start of ansi escape sequence.
    if( *index == '&' )
    {
      if( (index[1] == 'n') || (index[1] == 'N') || (index[1] == '&') )
      {
          // Copy the &
          *good_title = '&';
          good_title++;
          index++;
          // Copy the n or N or &.
          *good_title = *index;
          // Two &'s mean one printed &, otherwise we're removing color.
          if( *index == '&' )
          {
            printed_chars_left--;
          }
          else
          {
            colored = FALSE;
          }
          good_title++;
          index++;
      }
      else if( (index[1] == '+') || (index[1] == '-') )
      {
        // Then we have an ansi escape sequence.
        if( is_ansi_char(index[2]) )
        {
          // Copy the &<+|-><color>
          sprintf( good_title, "&%c%c", index[1], index[2] );
          good_title += 3;
          index += 3;

          colored = TRUE;
        }
        // At this point, we have "&+<char>" where <char> is a non-ansi character.
        // So we want to add all 3 if thers's room, or truncate if not.
        else
        {
          // Make one printed & and close color if applicable.
          if( printed_chars_left == 1 )
          {
            if( colored )
            {
              sprintf( good_title, "&&&N" );
              colored = FALSE;
              good_title += 4;
            }
            else
            {
              sprintf( good_title, "&&" );
              good_title += 2;
            }
            printed_chars_left--;
          }
          // We want "&<char>" to show where <char1> is the '+' or '-'
          else if( printed_chars_left == 2 )
          {
            if( colored )
            {
              sprintf( good_title, "&&%c&N", index[1] );
              colored = FALSE;
              good_title += 4;
            }
            else
            {
              sprintf( good_title, "&&%c", index[1] );
              good_title += 3;
            }
            printed_chars_left -= 2;
          }
          // We print the & (using 2 &'s to make it not buggy) and the '+' or '-',
          //   and decrement printed_chars_left by 2, and move index and good_title over 2.
          // Note: We don't try'n print the 3rd character since it might be the start
          //   of an ansi sequence.
          else
          {
            sprintf( good_title, "&&%c", index[1] );
            good_title += 3;
            index += 2;
            printed_chars_left -= 2;
          }
        }
      }
      else if( index[1] == '=' )
      {
        // Then we have an ansi escape sequence.
        if( is_ansi_char(index[2]) && is_ansi_char(index[3]) )
        {
          // Copy the &=<char><char>
          sprintf( good_title, "&=%c%c", index[2], index[3] );
          good_title += 4;
          index += 4;

          colored = TRUE;
        }
        // We want "&=" to show.
        else
        {
          if( printed_chars_left == 1 )
          {
            if( colored )
            {
              sprintf( good_title, "&&&N" );
              colored = FALSE;
              good_title += 4;
            }
            else
            {
              sprintf( good_title, "&&" );
              good_title += 2;
            }
            printed_chars_left--;
          }
          else if( printed_chars_left == 2 )
          {
            if( colored )
            {
              sprintf( good_title, "&&=&N" );
              colored = FALSE;
              good_title += 5;
            }
            else
            {
              sprintf( good_title, "&&=" );
              good_title += 3;
            }
            printed_chars_left -= 2;
          }
          // We print the & (using 2 &'s to make it not buggy) and the '=',
          //   and decrement printed_chars_left by 2, and move index and good_title over 2.
          // Note: We don't try'n print the 3rd character since it might be the start
          //   of an ansi sequence.
          else
          {
            sprintf( good_title, "&&=" );
            good_title += 3;
            index += 2;
            printed_chars_left -= 2;
          }
        }
      }
      // "&<char>" where <char> is a non-ansi.
      else
      {
        if( printed_chars_left == 1 )
        {
          break;
        }
        *(good_title++) = '&';
        *(good_title++) = '&';
        index++;
        printed_chars_left -= 2;
      }
    }
    // Regular character
    else
    {
      *good_title = *index;
      printed_chars_left--;
      good_title++;
      index++;
    }
  }

  // Might need to &N good_title here.
  if( colored )
  {
    sprintf( good_title, "&N %s", name );
  }
  else
  {
    sprintf( good_title, " %s", name );
  }

}

void Guild::withdraw( P_char member, int p, int g, int s, int c )
{
  char *char_name;
  P_member pMembers;

  // Verify they have enough money.
  if( (platinum < p) || (gold < g) || (silver < s) || (copper < c) )
  {
    send_to_char( "Deficit spending is forbidden...\n", member );
    return;
  }
  if( HAS_FINE(member) )
  {
    send_to_char( "Pay your dues! Let's see some cash first...\n", member );
    return;
  }
  if( !GT_OFFICER(GET_A_BITS(member)) )
  {
    send_to_char( "You wish you could do that, don't you?\n", member );
    return;
  }

  // Move the money.
  GET_PLATINUM( member ) += p;
  platinum -= p;
  GET_GOLD( member ) += g;
  gold -= g;
  GET_SILVER( member ) += s;
  silver -= s;
  GET_COPPER( member ) += c;
  copper -= c;

  send_to_char_f( member, "&+yYou withdrew &+W%dp&n&+y, &+Y%dg&n&+y, &+w%ds&n&+y, and &+y%dc from %s&+y.\n",
    p, g, s, c, name );
  write_transaction_to_ledger( GET_NAME(member), "withdrew", coins_to_string(p, g, s, c, "&+y") );
  logit(LOG_PLAYER, "Guild Withdrawal %d p %d g %d s %d c by %s", p, g, s, c, GET_NAME(member));

  // Save character
  writeCharacter(member, RENT_CRASH, member->in_room );
  // Save guild.
  save( );
}

void Guild::name_title( P_char member, char *args )
{
  char arg1[MAX_STRING_LENGTH];
  int rank_number;

  args = lohrr_chop( args, arg1 );

  if( *arg1 == '\0' || *arg1 == '?' )
  {
    send_to_char( "&+YSyntax: &+wsociety name <rank number|name> <new rank name>&+Y.&n\n", member );
    send_to_char( "Ranks:  0 = enemy, 1 = parole, 2 = normal, 3 = senior, 4 = officer, 5 = deputy,"
      " 6 = leader, 7 = king.\n", member );
    return;
  }

  if( GET_RK_BITS(GET_A_BITS( member )) < A_LEADER )
  {
    send_to_char( "You're too small for this.\n", member );
    return;
  }

  if( is_number(arg1) )
  {
    rank_number = atoi(arg1);
    if( (rank_number) < 0 || (rank_number >= ASC_NUM_RANKS) )
    {
      send_to_char_f( member, "'%s' is not a valid rank number (0-%d).\n", arg1, ASC_NUM_RANKS - 1 );
      return;
    }
  }
  else
  {
    if( is_abbrev(arg1, "enemy") )
    {
      rank_number = 0;
    }
    else if( is_abbrev(arg1, "parole") )
    {
      rank_number = 1;
    }
    else if( is_abbrev(arg1, "normal") )
    {
      rank_number = 2;
    }
    else if( is_abbrev(arg1, "senior") )
    {
      rank_number = 3;
    }
    else if( is_abbrev(arg1, "officer") )
    {
      rank_number = 4;
    }
    else if( is_abbrev(arg1, "deputy") )
    {
      rank_number = 5;
    }
    else if( is_abbrev(arg1, "leader") )
    {
      rank_number = 6;
    }
    else if( is_abbrev(arg1, "king") )
    {
      rank_number = 7;
    }
    else
    {
      send_to_char_f( member, "'%s' is not a valid rank number or name.\n", arg1 );
      send_to_char( "&+YSyntax: &+wsociety name <rank number|name> <new rank name>&+Y.&n\n", member );
      send_to_char( "Ranks:  0 = enemy, 1 = parole, 2 = normal, 3 = senior, 4 = officer, 5 = deputy,"
        " 6 = leader, 7 = king.\n", member );
    }
  }

  // Skip leading spaces first.
  while( isspace(*args) )
    args++;

  // Check string length and trim if necessry.
  trim_and_end_colorless( args, titles[rank_number], ASC_MAX_STR_RANK );

  save();
}

int Guild::max_assoc_size( )
{
  int base_size = get_property("guild.size.base", 0);
  int max_size = get_property("guild.size.max", 0);
  int step_size = get_property("guild.size.prestige.step", 0);
  int members = base_size + (int) ( (float) ( MAX(0, prestige) ) / (float) MAX(1, step_size) );

  members = MIN(members, max_size);

  return members + overmax;
}

void Guild::update_bits( P_char ch )
{
  P_member pMembers;
  char    *char_name = GET_NAME(ch);
  unsigned int ch_bits = GET_A_BITS(ch);

  // We don't update Immortal's bits.
  if( IS_TRUSTED(ch) )
    return;

  for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
  {
    if( !strcmp(pMembers->name, char_name) )
    {

      // Check the rank bits.
      if( A_RANK_BITS(ch_bits) != A_RANK_BITS(pMembers->bits) )
      {
        send_to_char( "Your guild rank has changed.\n", ch );
        // Set just the rank bits of ch's bits to pMembers bits.
        SET_M_BITS(GET_A_BITS(ch), A_RK_MASK, A_RANK_BITS(pMembers->bits));
      }
      // Check their guild status (member/applicant/banned/no thanks).
      if( A_SF_BITS(ch_bits) != A_SF_BITS(pMembers->bits) )
      {
        send_to_char( "Your guild status has changed.\n", ch );
        SET_M_BITS(GET_A_BITS(ch), A_SF_MASK, A_SF_BITS(pMembers->bits));
      }
      // Check their debt status.
      if( IS_DEBT(ch_bits) != IS_DEBT(pMembers->bits) )
      {
        // Change debt status w/message.
        if( IS_DEBT(ch_bits) )
        {
          send_to_char( "Your debt has been cleared.\n", ch );
          REMOVE_DEBT( GET_A_BITS(ch) );
        }
        else
        {
          send_to_char( "You have been fined by your guild.\n", ch );
          SET_DEBT( GET_A_BITS(ch) );
        }
        if( IS_STT(ch_bits) )
        {
          SET_STT( pMembers->bits );
        }
        else
        {
          REMOVE_STT( pMembers->bits );
        }
      }
      return;
    }
  }
  logit(LOG_DEBUG, "Guild::update_bits: '%s' %d not on guild %d's member list.", char_name, GET_ID(ch), id_number );
  debug( "Guild::update_bits: '%s' %d not on guild %d's member list.", char_name, GET_ID(ch), id_number );
}

void Guild::frag_remove( P_char member )
{
  if( !strcmp(GET_NAME( member ), frags.topfragger) )
  {
    frags.topfragger[0] = '\0';
    frags.top_frags = 0;
  }
  frags.frags -= GET_FRAGS(member);
  save( );
}

int Guild::update( )
{
  int count;
  P_char member;
  P_member pMembers, pMembers2, pMembers3;

  for( count = 0, member = character_list; member != NULL; member = member->next )
  {
    // Outposts & guildhall guards, etc.
    if( IS_NPC(member) )
      continue;
    if( GET_ASSOC(member) == this )
    {
      update_member(member);
      count++;
    }
  }

  // Delete duplicates of same member.
  for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
  {
    pMembers2 = pMembers->next;

    // While duplicate entries are right next to each other.
    while( pMembers2 != NULL )
    {
      if( !strcmp(pMembers->name, pMembers2->name) )
      {
        // Remove the 2nd entry from list.
        pMembers->next = pMembers2->next;
        // And free memory.
        pMembers2->next = NULL;
        delete pMembers2;
        pMembers2 = pMembers->next;
      }
      else
        break;
    }
    // If we still have 2nd / 3rd / etc entries, look at the entry after pMembers2 for a duplicate.
    if( pMembers2 != NULL )
    {
      while( pMembers2->next != NULL )
      {
        if( !strcmp(pMembers->name, pMembers2->next->name) )
        {
          // Get a pointer to the duplicate.
          pMembers3 = pMembers2->next;
          // Remove the duplicate from the list.
          pMembers2->next = pMembers3->next;
          // Delete the duplicate.
          pMembers3->next = NULL;
          delete pMembers3;
        }
        else
        {
          pMembers2 = pMembers2->next;
        }
      }
    }
  }

  member_count = 0;
  for( pMembers = members; pMembers != NULL; pMembers = pMembers->next )
  {
    member_count++;
  }
  save( );
}

void Guild::update_online_members()
{
  P_member pMember = members;
  P_char ch;

  while( pMember )
  {
    pMember->online_status = GSTAT_OFFLINE;
    pMember = pMember->next;
  }

  for( ch = character_list; ch != NULL; ch = ch->next )
  {
    // If they're a PC member of this guild (no golems etc).
    if( IS_PC(ch) && this == GET_ASSOC(ch) )
    {
      // Find the member in the list.
      for( pMember = members; pMember != NULL; pMember = pMember->next )
      {
        if( !strcmp(pMember->name, GET_NAME( ch )) )
        {
          pMember->online_status = ch->desc ? GSTAT_ONLINE : GSTAT_LINKDEAD;
          break;
        }
      }
    }
  }
}
