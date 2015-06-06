/*
 auction_houses.c
 -- torgal april 2006 (torgal@durismud.com)
*/

#include "auction_houses.h"
#include "interp.h"
#include "structs.h"
#include "files.h"
#include "db.h"
#include "sql.h"
#include <string.h>
#include <string>
#include <vector>
#include "prototypes.h"
#include "spells.h"
#include "utility.h"
using namespace std;

#ifdef __NO_MYSQL__
void init_auction_houses() {}
void shutdown_auction_houses() {}
void auction_houses_activity() {}
int auction_house_room_proc(int room_num, P_char ch, int cmd, char *arguments)
{
  send_to_char("Auctions are disabled.", ch);
  return TRUE;
}
#else
#include <mysql.h>

extern P_room world;
extern P_index obj_index;
extern P_desc descriptor_list;

char buff[MAX_STRING_LENGTH];

int DEFAULT_AUCTION_LENGTH;
int BID_TIME_EXTENSION;
int AUCTION_LIST_LIMIT;
int AUCTION_LISTING_FEE;
float AUCTION_START_PRICE_PCT_FEE;
float AUCTION_CLOSING_PCT_FEE;

#define STR_TOLOWER(str) { for( int _str_i = 0; _str_i < str.size(); _str_i++ ) str[_str_i] = tolower(str[_str_i]); }

extern MYSQL* DB;

EqSort *sorter;

void init_auction_houses() {
  fprintf(stderr, "-- Initializing Auctions\r\n");

  world[real_room0(16885)].funct = auction_house_room_proc;
  world[real_room0(83117)].funct = auction_house_room_proc;
  world[real_room0(97756)].funct = auction_house_room_proc;
  world[real_room0(17736)].funct = auction_house_room_proc;
  world[real_room0(55193)].funct = auction_house_room_proc;
  world[real_room0(888)].funct = auction_house_room_proc;
  world[real_room0(1200)].funct = auction_house_room_proc;
  world[real_room0(69)].funct = auction_house_room_proc;
  world[real_room0(420)].funct = auction_house_room_proc;
  world[real_room0(132821)].funct = auction_house_room_proc;

  sorter = new EqSort();

  DEFAULT_AUCTION_LENGTH = get_property("auctions.defaultLength", ( 2 * 24 * 60 * 60 ));
  BID_TIME_EXTENSION = get_property("auctions.bidTimeExtension", (5 * 60));	
  AUCTION_LIST_LIMIT = get_property("auctions.auctionListLimit", 100);
  AUCTION_LISTING_FEE = get_property("auctions.listingFee", 1000);
  AUCTION_START_PRICE_PCT_FEE = get_property("auctions.startPricePctFee", 0.02);
  AUCTION_CLOSING_PCT_FEE = get_property("auctions.closingPctFee", 0.03);

}

void shutdown_auction_houses()
{
  if( sorter )
  {
    delete sorter;
    sorter = NULL;
  }
}

bool check_db_active()
{
#ifdef __NO_MYSQL__
  return FALSE;
#endif

  if( !DB )
    return FALSE;
  else
    return TRUE;
}

void auction_error(P_char ch)
{
	send_to_char("&+WUnfortunately, auctions aren't active right now. Please "
                     "try again later.\r\n", ch);
}

void auction_houses_activity()
{
  if( !qry("SELECT id FROM auctions WHERE end_time < unix_timestamp() AND status = 'OPEN'") ) 
    return;
	
  MYSQL_RES *res = mysql_store_result(DB);

  MYSQL_ROW row;
  while( row = mysql_fetch_row(res) )
  {
    int auction_id = atoi(row[0]);

    if( !finalize_auction(auction_id, NULL) )
    {
      logit(LOG_DEBUG, "finalize_auction(%d) failed.", auction_id);
      continue;
    }
  }
	
  mysql_free_result(res);
}

// Returns TRUE iff ch is affected by SPELL_NOAUCTION. So much cleaner.
bool check_no_auction( P_char ch )
{
  if( !affected_by_spell(ch, SPELL_NOAUCTION) )
    return FALSE;
  // Gods can circumvent !auction flag.
  if( IS_TRUSTED( ch ) )
    return FALSE;

  send_to_char("&+RAuction House access has been temporarily disabled since you "
               "have recently &+Yremoved&+R a piece of worn equipment. Please try "
               "again in a little while.\r\n", ch);
  return TRUE;
}

void new_ah_call(P_char ch, char *arguments, int cmd)
{
  if ( cmd != CMD_AUCTION )
  return;

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("&+yYou're too busy fighting for your life to participate "
                 "in an auction!&n\r\n", ch);
    return;
  }

/* Some auction commands not disabled via SPELL_NOAUCTION.
  if( check_no_auction( ch ) )
    return;
*/

  if( !check_db_active() ) 
  {
    auction_error(ch);
    return;
  }

  char command[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
	
  half_chop(arguments, command, args);
	
  bool success = false;
	
  if( isname(command, "offer o"))
  {
    if( check_no_auction( ch ) )
      return;
    success = auction_offer(ch, args);
  }
  else if( isname(command, "list l")) success = auction_list(ch, args);
  else if( isname(command, "info i")) success = auction_info(ch, args);
  else if( isname(command, "bid b")) 
  {
    if( check_no_auction( ch ) )
      return;
    success = auction_bid(ch, args);
  }
  else if( isname(command, "pickup p")) success = auction_pickup(ch, args);
  else if( isname(command, "resort")) success = auction_resort(ch, args);
  else if( isname(command, "remove r")) success = auction_remove(ch, args);
  else success = auction_help(ch, arguments);
	
  if( !success ) auction_error(ch);
}

int auction_house_room_proc(int room_num, P_char ch, int cmd, char *arguments)
{
  if( cmd != CMD_AUCTION )
    return FALSE;

  if( !IS_PC(ch) )
    return FALSE;
	
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("&+yYou're too busy fighting for your life to participate "
                 "in an auction!&n\r\n", ch);
    return TRUE;
  }
		
  if( !check_db_active() )
  {
    auction_error(ch);
    return TRUE;
  }
		
  char command[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
	
  half_chop(arguments, command, args);
	
  bool success = false;
	
  if( isname(command, "offer o"))
    success = auction_offer(ch, args);
  else if( isname(command, "list l"))
    success = auction_list(ch, args);
  else if( isname(command, "info i"))
    success = auction_info(ch, args);
  else if( isname(command, "bid b"))
    success = auction_bid(ch, args);
  else if( isname(command, "pickup p"))
    success = auction_pickup(ch, args);
  else if( isname(command, "resort"))
    success = auction_resort(ch, args);
  else if( isname(command, "remove r"))
    success = auction_remove(ch, args);
  else
    success = auction_help(ch, arguments);
	
  if( !success )
    auction_error(ch);
	
  return TRUE;
}

// syntax: auction resort
bool auction_resort(P_char ch, char *args)
{
  if( !IS_TRUSTED(ch) )
    return auction_help(ch, args);
	
  if( !sorter )
  {
    send_to_char("sorter not initialized.\r\n", ch);
    return TRUE;
  }
	
  if( !qry("SELECT id, obj_short, obj_blob_str FROM auctions") )
    return FALSE;
	
  MYSQL_RES *res = mysql_store_result(DB);
	
  if( mysql_num_rows(res) < 1 )
  {
    send_to_char("No auctions to resort!\r\n", ch);
    mysql_free_result(res);
    return TRUE;
  }
	
  MYSQL_ROW row;
	
  int count = 0;
  while( row = mysql_fetch_row(res) )
  {
    int auction_id = atoi(row[0]);
    string obj_short(row[1]);
    char *obj_str = row[2];
				
    P_obj tmp_obj = read_one_object(obj_str);
		
    if( !tmp_obj )
      continue;

    string keywords = sorter->getSortFlagsString(tmp_obj);
    sprintf(buff, "%s: %s\r\n", obj_short.c_str(), keywords.c_str() );
    send_to_char(buff, ch);
		
    if( keywords.length() > 0 )
    {
      if( !qry("UPDATE auctions SET id_keywords = '%s' WHERE id = '%d'", keywords.c_str(), auction_id) )
        return FALSE;
    }

    extract_obj(tmp_obj, TRUE);
    count++;
  }

  sprintf(buff, "&+W%d items resorted.", count);
  send_to_char(buff, ch);
	
  mysql_free_result(res);

  return TRUE;
}

// syntax: auction offer item [starting price] [buy it now price]
bool auction_offer(P_char ch, char *args)
{
  char item_name[MAX_STRING_LENGTH];
	
  half_chop(args, item_name, args);

  P_obj tmp_obj = get_obj_in_list_vis(ch, item_name, ch->carrying);

  if( !tmp_obj )
  {
    send_to_char("&+WYou don't seem have that item!\r\n", ch);
    return TRUE;
  }

  if( IS_ARTIFACT(tmp_obj) )
  {
    send_to_char("&+WYou can't sell artifacts!\r\n", ch);
    return TRUE;
  }
	
  if( IS_SET(tmp_obj->extra_flags, ITEM_NODROP) )
  {
    send_to_char("&+WYou can't sell that item, it must be &+RCursed&+W!\r\n", ch);
    return TRUE;
  }

  if ( IS_SET(tmp_obj->extra_flags, ITEM_NORENT) || tmp_obj->condition < 90 )
  {
    send_to_char("&+WYou can't sell that item.\r\n", ch);
    return TRUE;
  }
	
  if ( tmp_obj->contains )
  {
    send_to_char("&+WYou can only sell containers if they are empty.\r\n", ch);
    return TRUE;
  }	
	
  half_chop(args, buff, args);
  int starting_price = 0;
  if( strlen(buff) )
    starting_price = atoi(buff) * 1000; // change to copper

  if( starting_price < 0 )
  {
    send_to_char("&+WInvalid starting price.\r\n", ch);
    return TRUE;
  }

  half_chop(args, buff, args);
  int buy_price = 0;
  if( strlen(buff) )
    buy_price = atoi(buff) * 1000; // change to copper

  if( buy_price && buy_price < starting_price )
  {
    send_to_char("&+WInvalid buy-it-now price.\r\n", ch);
    return TRUE;
  }

  half_chop(args, buff, args);
  int auction_length = DEFAULT_AUCTION_LENGTH;;
  if( strlen(buff) )
    auction_length = atoi(buff) * ( 24 * 60 * 60 );  // Sec * Min * Hrs in a Day.

  if( auction_length < (24*60*60) || auction_length > (7*24*60*60) )
  {
    send_to_char("&+WInvalid auction length: please enter 1 to 7 (days).\r\n", ch);
    return TRUE;
  }	

  half_chop(args, buff, args);
  int auction_quantity = 1;
  if( strlen(buff) )
    auction_quantity = atoi(buff);

  if( auction_quantity < 1 || auction_quantity > 9 )
  {
    send_to_char("&+WInvalid auction quantity: please enter 1 to 9 (items).\r\n", ch);
    return TRUE;
  }
  int i = auction_quantity;
  P_obj temp_obj = tmp_obj;
  // Check to see that they have enough of the item to auction.
  while( --i )
  {
    if(  temp_obj->next_content == NULL
      || temp_obj->R_num != temp_obj->next_content->R_num )
    {
      send_to_char( "You do not have enough of that item.\n", ch );
      return TRUE;
    }
    temp_obj = temp_obj->next_content;
  }

  // calculate listing fee price, in copper
  int fee = AUCTION_LISTING_FEE + (int) ( starting_price * AUCTION_START_PRICE_PCT_FEE );
	
  // check for enough money
  if( GET_MONEY(ch) < fee )
  {
    send_to_char("&+WYou don't have enough money to list the item.\r\n", ch);
    return TRUE;
  }

  bool saved_to_db = false;

  // save to db
  char obj_buff[MAX_STRING_LENGTH];
  char* obj_buff_ptr = obj_buff;
  int obj_buff_len = write_one_object(tmp_obj, obj_buff_ptr);

  mysql_real_escape_string(DB, buff, obj_buff, obj_buff_len);

  char desc_buff[MAX_STRING_LENGTH];
  mysql_real_escape_string(DB, desc_buff, tmp_obj->short_description, strlen(tmp_obj->short_description));

  int obj_vnum = (tmp_obj->R_num >= 0 ? obj_index[tmp_obj->R_num].virtual_number : -1);

  string obj_id_keywords = sorter->getSortFlagsString(tmp_obj);

  // Try new insert into auctions with quantity.
  if( qry( "INSERT INTO auctions (seller_pid, seller_name, start_time, end_time, obj_short, obj_vnum, obj_blob_str, cur_price, buy_price, id_keywords, quantity) VALUES ('%d', '%s', unix_timestamp(), unix_timestamp() + %d, '%s', '%d', '%s', '%d', '%d', '%s', '%d')", GET_PID(ch), ch->player.name, auction_length, desc_buff, obj_vnum, buff, starting_price, buy_price, obj_id_keywords.c_str(), auction_quantity ))
    saved_to_db = TRUE;

  if( !saved_to_db )
    return FALSE;
	
  logit(LOG_STATUS, "%s put %s up for auction.", ch->player.name, desc_buff);
  sprintf(buff, "&+WYou put &n%s &+Won the market.\r\n", tmp_obj->short_description);
  send_to_char(buff, ch);
	
  // remove money
  SUB_MONEY(ch, fee, 0);
  i = auction_quantity;
  temp_obj = tmp_obj;
  // Remove auction_quantity items.
  while( i-- )
  {
    // Set the object to be removed.
    tmp_obj = temp_obj;
    // Move to next object.
    temp_obj = temp_obj->next_content;
    // Then extract the object.
    extract_obj(tmp_obj, TRUE);
  }
  writeCharacter(ch, 1, ch->in_room);

  return TRUE;
}

// syntax: auction list
bool auction_list(P_char ch, char *args)
{
  char list_arg[MAX_STRING_LENGTH];
	
  half_chop(args, list_arg, args);

  char where_str[MAX_STRING_LENGTH];
  *where_str = '\0';

  if( isname(list_arg, "all a") || strlen(list_arg) < 1 )
    send_to_char("&+WAuctions closing soon:\r\n", ch);
  else if( isname(list_arg, "player p") )
  {
    half_chop(args, list_arg, args);

    if( strlen(list_arg) < 0 )
    {
      send_to_char("&+WPlease enter the name of a player.\r\n", ch);
      return TRUE;
    }

    list_arg[0] = toupper(list_arg[0]);

    sprintf(buff, "&+WAuctions by &n%s&+W:\r\n", list_arg);
    send_to_char(buff, ch);

    mysql_real_escape_string(DB, buff, list_arg, strlen(list_arg));

    sprintf(where_str, " and seller_name like '%s'", buff);

  }
  else if( isname(list_arg, "sort s") )
  {
    send_to_char("&+WAuctions for items:\r\n", ch);

    vector<string> list_args;
		
    while( strlen(args) > 0 )
    {
      half_chop(args, list_arg, args);
      list_args.push_back(string(list_arg));
			
      if( !sorter->isKeyword(list_arg) )
      {
        sprintf(buff, "&+W'&+Y%s&+W' is an invalid keyword!\r\n", list_arg);
        send_to_char(buff, ch);
        return TRUE;
      }

      sprintf(buff, "&+y%s\r\n", sorter->getDescString(list_arg).c_str() );
      send_to_char(buff, ch);

      mysql_real_escape_string(DB, buff, list_arg, strlen(list_arg));
      list_args.push_back(string(buff));
    }

    for( int i = 0; i < list_args.size(); i++ )
    {
      sprintf(buff, " and id_keywords like '%%%s,%%'", list_args[i].c_str() );
      strcat( where_str, buff );
    }

    send_to_char("\r\n", ch);
  }
  else
  {
    send_to_char("&+WAuction list syntax:\r\nauction list - show all auctions\n\r"
                 "auction list sort <keyword list> - show only auctions for items"
                 " with the specified attributes (like lockers)\n\r"
                 "auction list player <playername> - show all auctions by a player\n\r"
                 "\n\rValid keywords: horns, nose, tail, horse, back, badge, quiver, ear,"
                 " face, eyes, wield, wrist, waist, about, shield, arms, hands, feet,"
                 " legs, head, body, neck, finger, warrior, ranger, psionicist, paladin,"
                 " antipaladin, cleric, monk, druid, shaman, sorcerer, necromancer,"
                 " conjurer, assassin, mercenary, bard, thief, alchemist, berserker,"
                 " reaver, illusionist, dreadlord, ethermancer, totems, instruments,"
                 " potions, spellbooks, scrolls, containers, hitpoints, mana, moves,"
                 " hitroll, damroll, save_para, save_rod, save_fear, save_breath,"
                 " save_spell, str, dex, int, wis, con, agi, pow, cha, luck, karma,"
                 " str_max, dex_max, int_max, wis_max, con_max, agi_max, pow_max, cha_max,"
                 " luck_max, karma_max\r\n", ch);
    return TRUE;
  }

  if( !qry("SELECT id, seller_name, end_time - unix_timestamp() as secs_remaining, cur_price, buy_price, obj_short, obj_vnum, winning_bidder_pid, winning_bidder_name, seller_pid, quantity from auctions where status = 'OPEN' %s order by secs_remaining asc", where_str) )
    return FALSE;

  MYSQL_RES *res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    send_to_char("&+yNo auctions to list!\r\n", ch);
  }

  MYSQL_ROW row;				
  while( row = mysql_fetch_row(res) )
  {
    char *auction_id = row[0];
    char *seller_name = row[1];
    long secs_remaining = atol(row[2]);
    int cur_price = atoi(row[3]);
    int buy_price = atoi(row[4]);
    char *obj_short = row[5];
    int obj_vnum = atoi(row[6]);
    int winning_bidder_pid = atoi(row[7]);
    char *winning_bidder_name = row[8];
    int seller_pid = atoi(row[9]);
    int quantity = atoi(row[10]);

    // if( cur_price < 1 ) cur_price = 1;
    if( cur_price < 1000 ) cur_price = 1000; // change to copper

    char buf[128];
    sprintf(buf, "&+W%dp", (int) (cur_price/1000));
    string cur_price_str(buf);
    
    sprintf(buf, "&+W%dp", (int) (buy_price/1000));
    string buy_price_str(buf);

    char mine_flag[] = " ";
    if( GET_PID(ch) == seller_pid || GET_PID(ch) == winning_bidder_pid ) 
      strcpy(mine_flag, "*");

    // Only display Buy it now price if there is one.
    if( buy_price > 0 )
    {
      // Display vnum for gods.
      if( IS_TRUSTED(ch) )
        sprintf(buff, "&+W%s)&+W%s&n[&+B%5d&n] %d &n%s &n[%s&n] &+WBid: &n%s&+W Buy: &n%s\r\n", 
          auction_id, mine_flag, obj_vnum, quantity, pad_ansi(obj_short, 45).c_str(), 
          format_time(secs_remaining).c_str(), pad_ansi(cur_price_str.c_str(), 6).c_str(), 
          pad_ansi(buy_price_str.c_str(), 6).c_str() );
      else	
        sprintf(buff, "&+W%s)&+W%s&n %d %s &n[%s&n] &+WBid: &n%s&+W Buy: &n%s\r\n",
          auction_id, mine_flag, quantity, pad_ansi(obj_short, 45).c_str(), 
          format_time(secs_remaining).c_str(), pad_ansi(cur_price_str.c_str(), 6).c_str(), 
          pad_ansi(buy_price_str.c_str(), 6).c_str() );
    }
    else
    {
      if( IS_TRUSTED(ch) )
        sprintf(buff, "&+W%s)&+W%s&n[&+B%5d&n] %d &n%s &n[%s&n] &+WBid: &n%s&+W\r\n", 
          auction_id, mine_flag, obj_vnum, quantity, pad_ansi(obj_short, 45).c_str(),
          format_time(secs_remaining).c_str(), pad_ansi(cur_price_str.c_str(), 6).c_str() );
      else
        sprintf(buff, "&+W%s)&+W%s&n %d %s &n[%s&n] &+WBid: &n%s&+W\r\n",
          auction_id, mine_flag, quantity, pad_ansi(obj_short, 45).c_str(), 
          format_time(secs_remaining).c_str(), pad_ansi(cur_price_str.c_str(), 6).c_str() );
    }

    send_to_char(buff, ch);
  }

  mysql_free_result(res);
  return TRUE;
}

// syntax: auction info <auction id>
bool auction_info(P_char ch, char *args)
{
  char arg[MAX_STRING_LENGTH];
	
  half_chop(args, arg, args);
	
  int auction_id = atoi(arg);

  if( !qry("SELECT seller_name, end_time - unix_timestamp() as secs_remaining, cur_price, buy_price, obj_short, obj_vnum, winning_bidder_pid, winning_bidder_name, obj_blob_str, quantity FROM auctions WHERE id = '%d' and status = 'OPEN'", auction_id) )
    return FALSE;

  MYSQL_RES *res = mysql_store_result(DB);
	
  MYSQL_ROW row = mysql_fetch_row(res);
		
  if( !row )
  {
    send_to_char("&+WThere is no auction with that id!\r\n", ch);
    mysql_free_result(res);
    return TRUE;
  }

  char *seller_name = row[0];
  long secs_remaining = atol(row[1]);
  int cur_price = atoi(row[2]);
  int buy_price = atoi(row[3]);
  char *obj_short = row[4];
  int obj_vnum = atoi(row[5]);
  int winning_bidder_pid = atoi(row[6]);
  char *winning_bidder_name = row[7];
  char *obj_str = row[8];
  int quantity = atoi(row[9]);

  string cur_price_str(coin_stringv(cur_price));
  string buy_price_str(coin_stringv(buy_price));

  P_obj tmp_obj = read_one_object(obj_str);
  mysql_free_result(res);

  if( !tmp_obj )
  {
    logit(LOG_DEBUG, "auction_info(): problem retrieving item in auction [%d].\r\n", auction_id);
    return FALSE;
  }

  sprintf(buff, "&+WAuction &+W%d\r\n", auction_id);
  send_to_char(buff, ch);

  sprintf(buff, "&+WSeller: &n%s\r\n", seller_name);
  send_to_char(buff, ch);
	
  sprintf(buff, "&+WTime left: &n%s\r\n", format_time(secs_remaining).c_str() );
  send_to_char(buff, ch);

  if( winning_bidder_pid == 0 )
  {
    sprintf(buff, "&+WNo bids received. Starting bid: &n%s\r\n", cur_price_str.c_str());
    send_to_char(buff, ch);
  }
  else
  {
    sprintf(buff, "&+WHigh bid: &n%s&+W by &n%s\r\n", cur_price_str.c_str(), winning_bidder_name );
    send_to_char(buff, ch);
  }
	
  if( buy_price > 0 )
  {
    sprintf(buff, "&+WBuy-it-now price: &n%s\r\n", buy_price_str.c_str() );
    send_to_char(buff, ch);		
  }

  sprintf(buff, "&+WQuantity:&n %d\r\n", quantity );
  send_to_char(buff, ch);

  send_to_char("\r\n", ch);
	
  if( IS_TRUSTED(ch) )
  {
    sprintf(buff, "[&+B%d&n]\r\n", obj_vnum);
    send_to_char(buff, ch);
  }

  spell_identify(60, ch, NULL, 0, 0, tmp_obj);
	
  if (can_char_use_item(ch, tmp_obj))
  {
    send_to_char("&+WYour race and class is permitted to use this item.\r\n", ch);
  }
  else
  {
    send_to_char("&=LRYOU ARE UNABLE TO USE THIS ITEM.\r\n", ch);
  }

  extract_obj(tmp_obj, TRUE);
  return TRUE;
}

// syntax: auction remove <auction id>
bool auction_remove(P_char ch, char *args)
{
  if( !IS_TRUSTED(ch) )
    return auction_help(ch, "");

  char arg[MAX_STRING_LENGTH];
  half_chop(args, arg, args);
  int auction_id = atoi(arg);

  if( !qry("SELECT id FROM auctions WHERE id = '%d' and status = 'OPEN'", auction_id) )
    return FALSE;

  MYSQL_RES *res = mysql_store_result(DB);
	
  MYSQL_ROW auction_row = mysql_fetch_row(res);
		
  if( !auction_row )
  {
    send_to_char("&+WThere is no auction with that id!\r\n", ch);
    mysql_free_result(res);
    return TRUE;
  }

  mysql_free_result(res);
	
  if( !qry("UPDATE auctions SET status = 'REMOVED' WHERE id = '%d'", auction_id) )
    return FALSE;

  sprintf(buff, "&+WAuction %d removed.\r\n", auction_id);
  send_to_char(buff, ch);

  logit(LOG_WIZ, "Auction [%d] removed by %s", auction_id, ch->player.name);

  return TRUE;
}

// syntax: auction bid <auction id> <bid value in plat>
// TODO: look into advatoi() in auction.c
bool auction_bid(P_char ch, char *args)
{

  char b_arg[MAX_STRING_LENGTH];
  half_chop(args, b_arg, args);
  int auction_id = atoi(b_arg);

  if( !qry("SELECT cur_price, buy_price, obj_short, winning_bidder_pid, winning_bidder_name, quantity FROM auctions WHERE id = '%d' and status = 'OPEN'", auction_id) )
    return FALSE;

  MYSQL_RES *res = mysql_store_result(DB);

  MYSQL_ROW auction_row = mysql_fetch_row(res);

  if( !auction_row )
  {
    send_to_char("&+WThere is no auction with that id!\r\n", ch);
    mysql_free_result(res);
    return TRUE;
  }

  int cur_price = atoi(auction_row[0]);
  int buy_price = atoi(auction_row[1]);
  string obj_short(auction_row[2]);
  int winning_bidder_pid = atoi(auction_row[3]);
  string winning_bidder_name(auction_row[4]);
  int quantity = atoi( auction_row[5] );

  mysql_free_result(res);
	
  // calculate bid value
  half_chop(args, b_arg, args);
  int bid_value = atoi(b_arg) * 1000; // should change this to work in copper eventually

  if( bid_value <= 0 || ( !winning_bidder_pid && bid_value < cur_price )
    || ( winning_bidder_pid && bid_value <= cur_price ) )
  {
    send_to_char("&+WYou must bid higher than the current price!&n\r\n", ch);
    return TRUE;
  }

  // If they bid more than the buy it now price, set bid to the buy it now price.
  if( buy_price > 0 && bid_value >= buy_price )
    bid_value = buy_price;

  int to_pay = 0;

  // If the bidder is the current high bidder, pay the difference.
  if( GET_PID(ch) == winning_bidder_pid )
  {
    to_pay = bid_value - cur_price;
  }
  else
  {
    to_pay = bid_value;
  }

  // take away money
  if( GET_MONEY(ch) < to_pay )
  {
    sprintf(buff, "&+WYou don't have enough money!\r\nYou need: &n%s\r\n", 
      coin_stringv(to_pay) );
    send_to_char(buff, ch);
    return TRUE;
  }

  SUB_MONEY(ch, to_pay, 0);
  sprintf(buff, "&+WYou pay &n%s&n.\r\n", coin_stringv(to_pay));
  send_to_char(buff, ch);

  qry("INSERT INTO auction_bid_history (date, auction_id, bidder_pid, bidder_name, bid_amount) VALUES "
      "(unix_timestamp(), %d, %d, '%s', %d)", auction_id, GET_PID(ch), ch->player.name, bid_value);

  // check if its buy it now
  // if so, send money to seller, transfer item to buyer, close auction
  if( buy_price > 0 && bid_value >= buy_price )
  {
    // buy it now
    if( winning_bidder_pid && winning_bidder_pid != GET_PID(ch) )
    {
      insert_money_pickup(winning_bidder_pid, cur_price);
      logit(LOG_DEBUG, "%s was outbid on auction %d, refunding %s", winning_bidder_name.c_str(), auction_id, coin_stringv(cur_price));
    }

    if( !qry("UPDATE auctions SET winning_bidder_pid = '%d', winning_bidder_name = '%s', cur_price = '%d' WHERE id = '%d'", GET_PID(ch), ch->player.name, buy_price, auction_id) )
      return FALSE;

    finalize_auction(auction_id, ch);
    auction_pickup(ch, "");
    logit(LOG_STATUS, "%s buys-it-now auction %d for %s", ch->player.name, auction_id, coin_stringv(to_pay));
  }
  else
  {
    // normal bid
    sprintf(buff, "&+WYou bid &n%s&+W on &n%d %s&n.\r\n", coin_stringv(bid_value), quantity, obj_short.c_str());
    send_to_char(buff, ch);

    logit(LOG_STATUS, "%s bid %s on auction %d", ch->player.name, coin_stringv(bid_value), auction_id );

    // If the bidder is the current high bidder.
    if( GET_PID(ch) == winning_bidder_pid )
    {
      if( !qry("UPDATE auctions SET cur_price = '%d' WHERE id = '%d'", bid_value, auction_id) )
        return FALSE;
    }
    else
    {
      if( winning_bidder_pid != 0 )
      {
        insert_money_pickup(winning_bidder_pid, cur_price);
        logit(LOG_DEBUG, "%s was outbid on auction %d, refunding %s", 
          winning_bidder_name.c_str(), auction_id, coin_stringv(cur_price));
      }

      if( !qry("UPDATE auctions SET cur_price = '%d', winning_bidder_pid = '%d', winning_bidder_name = '%s', end_time = end_time + '%d'  WHERE id = '%d'", bid_value, GET_PID(ch), ch->player.name, BID_TIME_EXTENSION, auction_id) )
        return FALSE;
			
      // alert loser that they were outbid!
      sprintf(buff, "&+WA voice says in your mind, &+W'You were outbid in auction [&+W%d&+W]"
                    " for &n%s&+W, and your bid money is available for pickup.'\r\n", 
                    auction_id, obj_short.c_str() );

      if( !send_to_pid(buff, winning_bidder_pid) )
        send_to_pid_offline(buff, winning_bidder_pid);
    }
  }
  return TRUE;
}

// syntax: auction pickup
bool auction_pickup(P_char ch, char *args)
{

  if( strlen(args) > 0 )
  {
    char arg[MAX_STRING_LENGTH];
    half_chop(args, arg, args);

    if( IS_TRUSTED(ch) && strlen(arg) > 0 )
    {
      int auction_id = atoi(arg);

      if( !qry("SELECT id, obj_short FROM auctions WHERE id = '%d'", auction_id) )
        return FALSE;

      MYSQL_RES *res = mysql_store_result(DB);

      MYSQL_ROW auction_row = mysql_fetch_row(res);

      if( !auction_row )
      {
        send_to_char("&+WThere is no auction with that id!\r\n", ch);
        mysql_free_result(res);
        return TRUE;
      }

      string obj_short(auction_row[1]);

      if( !qry("INSERT INTO auction_item_pickups (pid, obj_blob_str) (SELECT '%d', obj_blob_str FROM auctions WHERE id = '%d')", GET_PID(ch), auction_id) )
        return FALSE;

      sprintf(buff, "&+WA voice in your mind says, &+W'&n%s &+Wis ready for pickup, oh Great Master!'\r\n", obj_short.c_str() );
      send_to_char(buff, ch);

      return TRUE;
    }
  }

  if( !qry("SELECT money FROM auction_money_pickups WHERE pid = '%d' and money > 0", GET_PID(ch) ) ) 
    return FALSE;

  MYSQL_RES *res = mysql_store_result(DB);

  bool no_money = FALSE;
  bool no_items = TRUE;

  MYSQL_ROW row = mysql_fetch_row(res);

  if( !row || atoi(row[0]) < 1 )
    no_money = TRUE;
  else
  {
    int money = atoi(row[0]);

    if( !qry("UPDATE auction_money_pickups SET money = money - %d WHERE pid = '%d'", money, GET_PID(ch)) )
    {
      logit(LOG_DEBUG, "pid [%d], money not able to be picked up\r\n", mysql_error(DB), GET_PID(ch), money);
      mysql_free_result(res);
      return FALSE;
    }

    ADD_MONEY(ch, money);
    logit(LOG_PLAYER, "%s picked up %s from the auction house.", GET_NAME(ch), coin_stringv(money));

    sprintf(buff, "&+WYou pick up &n%s&+W.&n\r\n", coin_stringv(money));
    send_to_char(buff, ch);		
  }
  mysql_free_result(res);

  if( !qry("SELECT id, obj_blob_str, quantity FROM auction_item_pickups WHERE pid = '%d' AND retrieved = 0", GET_PID(ch)) )
  {
    send_to_char( "Oh Noes!  Failed to read database.\n", ch );
    return FALSE;
  }
  res = mysql_store_result(DB);

  if( mysql_num_rows(res) >= 1 )
  {
    while( row = mysql_fetch_row(res) )
    {
      int id = atoi(row[0]);
      int quantity = atoi( row[2] );

      P_obj tmp_obj = read_one_object(row[1]);
      P_obj temp_obj = tmp_obj;
      if( !tmp_obj )
      {
        logit(LOG_DEBUG, "auction_pickup(): problem 1 retrieving auction_item_pickups[%d].\r\n", id);
        continue;
      }
      // While there is another object to load..
      if( quantity > 1 )
        logit(LOG_DEBUG, "auction_pickup(): Loading objects quantity: %d.", quantity );
      while( --quantity > 0 )
      {
        // Load another object.
        temp_obj->next_content = read_one_object( row[1] );
        if( !temp_obj )
        {
          logit(LOG_DEBUG, "auction_pickup(): problem 2 retrieving auction_item_pickups[%d].\r\n", id);
          quantity = -1;
          break;
        }
        else
          temp_obj = temp_obj->next_content;
      }

      if( quantity == -1 || !qry("UPDATE auction_item_pickups SET retrieved = 1 where id = '%d'", id) )
      {
        extract_obj( tmp_obj, TRUE );
        continue;
      }

      logit(LOG_PLAYER, "%s picked up %s [R:%d] from the auction house.", GET_NAME(ch), tmp_obj->short_description, tmp_obj->R_num);
      sprintf(buff, "&+WYou pick up &n%s&+W.\r\n", tmp_obj->short_description);
      send_to_char(buff, ch);
      while( tmp_obj->next_content )
      {
        temp_obj = tmp_obj->next_content;
        tmp_obj->next_content = NULL;
        obj_to_char(tmp_obj, ch);
        tmp_obj = temp_obj;

        logit(LOG_PLAYER, "%s picked up %s [R:%d] from the auction house.", GET_NAME(ch), tmp_obj->short_description, tmp_obj->R_num);
        sprintf(buff, "&+WYou pick up &n%s&+W.\r\n", tmp_obj->short_description);
        send_to_char(buff, ch);
      }
      obj_to_char(tmp_obj, ch);
      no_items = FALSE;
    }
  }

  mysql_free_result(res);

  if( no_money && no_items )
    send_to_char("&+WYou have no items or money to pickup!&n\r\n", ch);
  else
    writeCharacter(ch, 1, ch->in_room);

  return TRUE;
}

bool auction_help(P_char ch, char *arg) {
	send_to_char("&+WAuction syntax:\r\n- auction list [help]\r\n"
                     "- auction offer <item from your inventory> [starting price in plat] [buy-it-now price in plat] [length of auction in days] [quantity]\r\n"
                     "- auction bid <auction id> <value in plat>\r\n"
                     "- auction info <auction id>\r\n- auction pickup\r\n", ch);
	return TRUE;
}

bool finalize_auction(int auction_id, P_char to_ch)
{

  if( !qry("UPDATE auctions SET status = 'CLOSED' WHERE id = '%d'", auction_id) ) 
    return FALSE;
	
  if( !qry("SELECT seller_pid, winning_bidder_pid, cur_price, obj_short, obj_vnum, winning_bidder_name, quantity FROM auctions WHERE id = '%d' LIMIT 1", auction_id) )
    return FALSE;

  MYSQL_RES *res = mysql_store_result(DB);

  MYSQL_ROW auction_row = mysql_fetch_row(res);

  if( !auction_row )
  {
    logit(LOG_DEBUG, "finalize_auction(): auction id [%d] doesn't exist!", auction_id);
    mysql_free_result(res);
    return FALSE;
  }

  int seller_pid = atoi(auction_row[0]);
  int winning_bidder_pid = atoi(auction_row[1]);
  int final_price = atoi(auction_row[2]);
  string obj_short(auction_row[3]);
  int obj_vnum = atoi(auction_row[4]);
  string winning_bidder_name(auction_row[5]);
  int quantity = atoi(auction_row[6]);

  mysql_free_result(res);

  string final_price_str(coin_stringv(final_price));

  if( !winning_bidder_pid )
  {
    // no one bid, return item to seller
    if( !qry("INSERT INTO auction_item_pickups (pid, obj_blob_str, quantity) (SELECT '%d', obj_blob_str, '%d' FROM auctions WHERE id = '%d')", seller_pid,quantity, auction_id) )
      return FALSE;

    // alert seller that auction closed
    sprintf(buff, "&+WA voice says in your mind, &+W'Your auction for &n%d %s&+W received no bids, and is available for pickup.'\r\n", quantity, obj_short.c_str());
    if( !send_to_pid(buff, seller_pid) )
      send_to_pid_offline(buff, seller_pid);
  }
  else
  {
    int paid_price = final_price - (int) ( (float) final_price * AUCTION_CLOSING_PCT_FEE );
    //int paid_price = final_price;

    logit(LOG_DEBUG, "Auction [%d] closed, final price: %d, commission fee: %d", auction_id, final_price, (final_price-paid_price));

    logit(LOG_STATUS, "%s won auction %d, %d %s for %s", winning_bidder_name.c_str(), auction_id, quantity, obj_short.c_str(), coin_stringv(final_price));
  
    // money to seller
    insert_money_pickup(seller_pid, paid_price);		

    // item to buyer
    if( !qry("INSERT INTO auction_item_pickups (pid, obj_blob_str, quantity) (SELECT '%d', obj_blob_str, '%d' FROM auctions WHERE id = '%d')", winning_bidder_pid, quantity, auction_id) )
      return FALSE;

    // alert buyer and seller that auction closed
    sprintf(buff, "&+WA voice says in your mind, &+W'&n%d %s&+W was sold for &n%s&+W, and the money is available for pickup.'\r\n", quantity, obj_short.c_str(), final_price_str.c_str());
    if( !send_to_pid(buff, seller_pid) )
      send_to_pid_offline(buff, seller_pid);

    sprintf(buff, "&+WA voice says in your mind, &+W'You bought &n%d %s&+W for &n%s&+W, and it's now available for pickup.'\r\n", quantity, obj_short.c_str(), final_price_str.c_str());
    if( !send_to_pid(buff, winning_bidder_pid) )
      send_to_pid_offline(buff, winning_bidder_pid);
  }
  return TRUE;
}

bool insert_money_pickup(int pid, int money) {
	if( !qry("SELECT pid FROM auction_money_pickups WHERE pid = '%d' LIMIT 1", pid) ) 
		return FALSE;

	MYSQL_RES *res = mysql_store_result(DB);
		
	MYSQL_ROW row = mysql_fetch_row(res);
		
	if( !row ) {
		mysql_free_result(res);
		
		if( !qry("INSERT INTO auction_money_pickups (pid, money) VALUES ('%d', '%d')", pid, money) )
			return FALSE;
			
	} else {
		mysql_free_result(res);
		
		if( !qry("UPDATE auction_money_pickups SET money = money + %d WHERE pid = '%d'", money, pid) )
			return FALSE;
		
	}

  logit(LOG_STATUS, "PID %d picked up %d", pid, money);

	return TRUE;
}

string format_time(long seconds) {
	char tmp[128];
	
	if( seconds < 0 ) seconds = 0;
	
	if( seconds < 60 ) {
		sprintf(tmp, "&+R<1m&n", seconds );
		
	} else if( seconds < ( 60 * 60 ) ) {
		sprintf(tmp, "&+R%ldm&n", (seconds / 60) % 60 );
		
	} else if( seconds < ( 6 * 60 * 60  ) ) {
		sprintf(tmp, "&+Y%ldh %ldm&n", (seconds / 3600) % ( 60 * 60 ) , (seconds / 60 ) % 60 );
		
	} else {
		sprintf(tmp, "&+W%ldh&n", (seconds / 3600) % ( 60 * 60 ) );
		
	}
	
	return string(tmp);
}

EqSort::EqSort() {
	flags.push_back(new EqSlotFlag("horns", "worn on horns", ITEM_WEAR_HORN));
	flags.push_back(new EqSlotFlag("nose", "worn on nose", ITEM_WEAR_NOSE));
	flags.push_back(new EqSlotFlag("tail", "worn on tail", ITEM_WEAR_TAIL));
	flags.push_back(new EqSlotFlag("horse", "worn on a horses body", ITEM_HORSE_BODY));
	flags.push_back(new EqSlotFlag("back", "worn on a spider body", ITEM_SPIDER_BODY));
	flags.push_back(new EqSlotFlag("back", "worn on back", ITEM_WEAR_BACK));
	flags.push_back(new EqSlotFlag("badge", "worn as a badge", ITEM_GUILD_INSIGNIA));
	flags.push_back(new EqSlotFlag("quiver", "worn as a quiver", ITEM_WEAR_QUIVER));
	flags.push_back(new EqSlotFlag("ear", "worn on or in ear", ITEM_WEAR_EARRING));
	flags.push_back(new EqSlotFlag("face", "worn on face", ITEM_WEAR_FACE));
	flags.push_back(new EqSlotFlag("eyes", "worn on or over eyes", ITEM_WEAR_EYES));
	flags.push_back(new EqSlotFlag("wield", "used as a weapon or wielded", ITEM_WIELD));
	flags.push_back(new EqSlotFlag("wrist", "worn around wrist", ITEM_WEAR_WRIST));
	flags.push_back(new EqSlotFlag("waist", "worn about waist", ITEM_WEAR_WAIST));
	flags.push_back(new EqSlotFlag("about", "worn about body", ITEM_WEAR_ABOUT));
	flags.push_back(new EqSlotFlag("shield", "worn as a shield", ITEM_WEAR_SHIELD));
	flags.push_back(new EqSlotFlag("arms", "worn on arms", ITEM_WEAR_ARMS));
	flags.push_back(new EqSlotFlag("hands", "worn on hands", ITEM_WEAR_HANDS));
	flags.push_back(new EqSlotFlag("feet", "worn on feet", ITEM_WEAR_FEET));
	flags.push_back(new EqSlotFlag("legs", "worn on legs", ITEM_WEAR_LEGS));
	flags.push_back(new EqSlotFlag("head", "worn on head", ITEM_WEAR_HEAD));
	flags.push_back(new EqSlotFlag("body", "worn on body", ITEM_WEAR_BODY));
	flags.push_back(new EqSlotFlag("neck", "worn around neck", ITEM_WEAR_NECK));
	flags.push_back(new EqSlotFlag("finger", "worn on finger", ITEM_WEAR_FINGER));

	flags.push_back(new EqClassFlag("warrior", "usable by a warrior", CLASS_WARRIOR));
	flags.push_back(new EqClassFlag("ranger", "usable by a ranger", CLASS_RANGER));
	flags.push_back(new EqClassFlag("psionicist", "usable by a psionicist", CLASS_PSIONICIST));
	flags.push_back(new EqClassFlag("paladin", "usable by a paladin", CLASS_PALADIN));
	flags.push_back(new EqClassFlag("antipaladin", "usable by an antipaladin", CLASS_ANTIPALADIN));
	flags.push_back(new EqClassFlag("cleric", "usable by a cleric", CLASS_CLERIC));
	flags.push_back(new EqClassFlag("monk", "usable by a monk", CLASS_MONK));
	flags.push_back(new EqClassFlag("druid", "usable by a druid", CLASS_DRUID));
	flags.push_back(new EqClassFlag("shaman", "usable by a shaman", CLASS_SHAMAN));
	flags.push_back(new EqClassFlag("sorcerer", "usable by a sorcerer", CLASS_SORCERER));
	flags.push_back(new EqClassFlag("necromancer", "usable by a necromancer", CLASS_NECROMANCER));
	flags.push_back(new EqClassFlag("conjurer", "usable by a conjurer", CLASS_CONJURER));
	flags.push_back(new EqClassFlag("assassin", "usable by an assassin", CLASS_ASSASSIN));
	flags.push_back(new EqClassFlag("mercenary", "usable by a mercenary", CLASS_MERCENARY));
	flags.push_back(new EqClassFlag("bard", "usable by a bard", CLASS_BARD));
	flags.push_back(new EqClassFlag("thief", "usable by a thief", CLASS_THIEF));
	flags.push_back(new EqClassFlag("alchemist", "usable by an alchemist", CLASS_ALCHEMIST));
	flags.push_back(new EqClassFlag("berserker", "usable by a berserker", CLASS_BERSERKER));
	flags.push_back(new EqClassFlag("reaver", "usable by a reaver", CLASS_REAVER));
	flags.push_back(new EqClassFlag("illusionist", "usable by an illusionist", CLASS_ILLUSIONIST));
	flags.push_back(new EqClassFlag("dreadlord", "usable by a dreadlord", CLASS_DREADLORD));
	flags.push_back(new EqClassFlag("ethermancer", "usable by an ethermancer", CLASS_ETHERMANCER));

	flags.push_back(new EqTypeFlag("totems", "used as totems", ITEM_TOTEM));
	flags.push_back(new EqTypeFlag("instruments", "playable as bard instruments", ITEM_INSTRUMENT));
	flags.push_back(new EqTypeFlag("potions", "quaffable or used as potions", ITEM_POTION));
	flags.push_back(new EqTypeFlag("spellbooks", "used as spellbooks", ITEM_SPELLBOOK));
	flags.push_back(new EqTypeFlag("scrolls", "used as scrolls", ITEM_SCROLL));
	flags.push_back(new EqTypeFlag("containers", "that are containers", ITEM_CONTAINER));
	
	flags.push_back(new EqApplyFlag("hitpoints", "that affect hitpoints", APPLY_HIT));
	flags.push_back(new EqApplyFlag("mana", "that affect mana", APPLY_MANA));
	flags.push_back(new EqApplyFlag("moves", "that affect moves", APPLY_MOVE));
	flags.push_back(new EqApplyFlag("hitroll", "that affect hitroll", APPLY_HITROLL));
	flags.push_back(new EqApplyFlag("damroll", "that affect damroll", APPLY_DAMROLL));
	flags.push_back(new EqApplyFlag("save_para", "that affect save_para", APPLY_SAVING_PARA));
	flags.push_back(new EqApplyFlag("save_rod", "that affect save_rod", APPLY_SAVING_ROD));
	flags.push_back(new EqApplyFlag("save_fear", "that affect save_fear", APPLY_SAVING_FEAR));
	flags.push_back(new EqApplyFlag("save_breath", "that affect save_breath", APPLY_SAVING_BREATH));
	flags.push_back(new EqApplyFlag("save_spell", "that affect save_spell", APPLY_SAVING_SPELL));
	flags.push_back(new EqApplyFlag("str", "that affect strength", APPLY_STR));
	flags.push_back(new EqApplyFlag("dex", "that affect dexterity", APPLY_DEX));
	flags.push_back(new EqApplyFlag("int", "that affect intelligence", APPLY_INT));
	flags.push_back(new EqApplyFlag("wis", "that affect wisdom", APPLY_WIS));
	flags.push_back(new EqApplyFlag("con", "that affect constitution", APPLY_CON));
	flags.push_back(new EqApplyFlag("agi", "that affect agility", APPLY_AGI));
	flags.push_back(new EqApplyFlag("pow", "that affect power", APPLY_POW));
	flags.push_back(new EqApplyFlag("cha", "that affect charisma", APPLY_CHA));
	flags.push_back(new EqApplyFlag("luck", "that affect luck", APPLY_LUCK));
	flags.push_back(new EqApplyFlag("karma", "that affect karma", APPLY_KARMA));
	flags.push_back(new EqApplyFlag("str_max", "that affect maximum strength (str_max)", APPLY_STR_MAX));
	flags.push_back(new EqApplyFlag("dex_max", "that affect maximum dexterity (dex_max)", APPLY_DEX_MAX));
	flags.push_back(new EqApplyFlag("int_max", "that affect maximum intelligence (int_max)", APPLY_INT_MAX));
	flags.push_back(new EqApplyFlag("wis_max", "that affect maximum wisdom (wis_max)", APPLY_WIS_MAX));
	flags.push_back(new EqApplyFlag("con_max", "that affect maximum constitution (con_max)", APPLY_CON_MAX));
	flags.push_back(new EqApplyFlag("agi_max", "that affect maximum agility (agi_max)", APPLY_AGI_MAX));
	flags.push_back(new EqApplyFlag("pow_max", "that affect maximum power (pow_max)", APPLY_POW_MAX));
	flags.push_back(new EqApplyFlag("cha_max", "that affect maximum charisma (cha_max)", APPLY_CHA_MAX));
	flags.push_back(new EqApplyFlag("luck_max", "that affect maximum luck (luck_max)", APPLY_LUCK_MAX));
	flags.push_back(new EqApplyFlag("karma_max", "that affect maximum karma (karma_max)", APPLY_KARMA_MAX));
}

string EqSort::getSortFlagsString(P_obj obj) {
	string keywords;

	if( !obj ) return keywords;
	
	for( int i = 0; i < flags.size(); i++ ) {
		if( flags[i] && flags[i]->match(obj) ) {
			keywords += " ";
			keywords += flags[i]->keyword;
			keywords += ",";
		}
		
	}

	return keywords;
}

string EqSort::getDescString(const char *keyword) {
	if( strlen(keyword) < 1 ) return string();

	for( int i = 0; i < flags.size(); i++ ) {
		if( flags[i] && !strcmp(keyword, flags[i]->keyword.c_str() ) ) {
			return flags[i]->desc;
		}
	}

	return string();
}

bool EqSort::isKeyword(const char *keyword) {
	if( strlen(keyword) < 1 ) return false;

	for( int i = 0; i < flags.size(); i++ ) {
		if( flags[i] && !strcmp(keyword, flags[i]->keyword.c_str() ) ) {
			return true;
		}
	}

	return false;
}
#endif // #ifdef __NO_MYSQL__
