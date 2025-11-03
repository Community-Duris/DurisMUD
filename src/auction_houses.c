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
  if( cmd != CMD_AUCTION || !IS_ALIVE(ch) || IS_NPC(ch) )
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
    send_to_char("Sorter not initialized.\r\n", ch);
    return TRUE;
  }

  if( !qry("SELECT id, obj_short, obj_blob_str FROM auctions") )
  {
    return FALSE;
  }

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
    snprintf(buff, MAX_STRING_LENGTH, "%s: %s\r\n", obj_short.c_str(), keywords.c_str() );
    send_to_char(buff, ch);

    if( keywords.length() > 0 )
    {
      if( !qry("UPDATE auctions SET id_keywords = '%s' WHERE id = '%d'", keywords.c_str(), auction_id) )
        return FALSE;
    }

    extract_obj(tmp_obj);
    count++;
  }

  snprintf(buff, MAX_STRING_LENGTH, "&+W%d items resorted.", count);
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
  snprintf(buff, MAX_STRING_LENGTH, "&+WYou put &n%s &+Won the market.\r\n", tmp_obj->short_description);
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
    extract_obj(tmp_obj);
  }
  writeCharacter(ch, 1, ch->in_room);

  return TRUE;
}

// syntax: auction list
bool auction_list(P_char ch, char *args)
{
  char list_arg[MAX_STRING_LENGTH];
  char where_str[MAX_STRING_LENGTH];
  int i, count;

  half_chop(args, list_arg, args);
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

    snprintf(buff, MAX_STRING_LENGTH, "&+WAuctions by &n%s&+W:\r\n", list_arg);
    send_to_char(buff, ch);

    mysql_real_escape_string(DB, buff, list_arg, strlen(list_arg));

    snprintf(where_str, MAX_STRING_LENGTH, " and seller_name like '%s'", buff);

  }
  else if( isname(list_arg, "sort s") )
  {
    vector<string> list_args;

    if( !*args )
    {
      count = snprintf(buff, MAX_STRING_LENGTH, "&+WValid keywords: &+Y%s", sorter->getKeyword(0).c_str() );
      for( i = 1;i < sorter->getSize();i++ )
      {
        count += snprintf(buff + count, MAX_STRING_LENGTH, ", %s", sorter->getKeyword(i).c_str());
      }
      snprintf(buff + count, MAX_STRING_LENGTH, ".\n\r" );
      send_to_char( buff, ch );
      return TRUE;
    }

    while( strlen(args) > 0 )
    {
      half_chop(args, list_arg, args);
      list_args.push_back(string(list_arg));

      if( !sorter->isKeyword(list_arg) )
      {
        snprintf(buff, MAX_STRING_LENGTH, "&+W'&+Y%s&+W' is an invalid keyword!\r\n", list_arg);
        send_to_char(buff, ch);
        return TRUE;
      }

      snprintf(buff, MAX_STRING_LENGTH, "&+WAuctions for items &+y%s&+W:&n\n", sorter->getDescString(list_arg).c_str() );
      send_to_char(buff, ch);

      mysql_real_escape_string(DB, buff, list_arg, strlen(list_arg));
      list_args.push_back(string(buff));
    }

    for( int i = 0; i < list_args.size(); i++ )
    {
      snprintf(buff, MAX_STRING_LENGTH, " and id_keywords like '%% %s,%%'", list_args[i].c_str() );
      strcat( where_str, buff );
    }

    send_to_char("\r\n", ch);
  }
  else
  {
    send_to_char("&+WAuction list syntax:\r\nauction list - show all auctions\n\r"
                 "auction list sort <keyword list> - show only auctions for items"
                 " with the specified attributes (like lockers)\n\r"
                 "auction list player <playername> - show all auctions by a player\n\r", ch );
    count = snprintf(buff, MAX_STRING_LENGTH, "&+WValid keywords: &+Y%s", sorter->getKeyword(0).c_str() );
    for( i = 1;i < sorter->getSize();i++ )
    {
      count += snprintf(buff + count, MAX_STRING_LENGTH, ", %s", sorter->getKeyword(i).c_str());
    }
    snprintf(buff + count, MAX_STRING_LENGTH, ".\n\r" );
    send_to_char( buff, ch );
    return TRUE;
  }

  if( !qry("SELECT id, seller_name, end_time - unix_timestamp() as secs_remaining, cur_price, buy_price, obj_short, obj_vnum, winning_bidder_pid, winning_bidder_name, seller_pid, quantity from auctions where status = 'OPEN' %s order by secs_remaining asc", where_str) )
    return FALSE;

  MYSQL_RES *res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    if( !*list_arg )
      send_to_char("&+yNo auctions to list!\r\n", ch);
    else
      send_to_char("&+yNo auctions found!\r\n", ch);
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
    snprintf(buf, 128, "&+W%dp", (int) (cur_price/1000));
    string cur_price_str(buf);

    snprintf(buf, 128, "&+W%dp", (int) (buy_price/1000));
    string buy_price_str(buf);

    char mine_flag[] = " ";
    if( GET_PID(ch) == seller_pid || GET_PID(ch) == winning_bidder_pid )
      strcpy(mine_flag, "*");

    // Only display Buy it now price if there is one.
    if( buy_price > 0 )
    {
      // Display vnum for gods.
      if( IS_TRUSTED(ch) )
        snprintf(buff, MAX_STRING_LENGTH, "&+W%s)&+W%s&n[&+B%6d&n] %d &n%s&n [%s&n] &+WBid: &n%s&+W Buy: &n%s\r\n",
          auction_id, mine_flag, obj_vnum, quantity, pad_ansi(obj_short, 45, TRUE).c_str(),
          format_time(secs_remaining).c_str(), pad_ansi(cur_price_str.c_str(), 7).c_str(),
          pad_ansi(buy_price_str.c_str(), 7).c_str() );
      else
        snprintf(buff, MAX_STRING_LENGTH, "&+W%s)&+W%s&n %d %s&n [%s&n] &+WBid: &n%s&+W Buy: &n%s\r\n",
          auction_id, mine_flag, quantity, pad_ansi(obj_short, 45, TRUE).c_str(),
          format_time(secs_remaining).c_str(), pad_ansi(cur_price_str.c_str(), 6).c_str(),
          pad_ansi(buy_price_str.c_str(), 6).c_str() );
    }
    else
    {
      if( IS_TRUSTED(ch) )
        snprintf(buff, MAX_STRING_LENGTH, "&+W%s)&+W%s&n[&+B%6d&n] %d &n%s&n [%s&n] &+WBid: &n%s&+W\r\n",
          auction_id, mine_flag, obj_vnum, quantity, pad_ansi(obj_short, 45, TRUE).c_str(),
          format_time(secs_remaining).c_str(), pad_ansi(cur_price_str.c_str(), 6).c_str() );
      else
        snprintf(buff, MAX_STRING_LENGTH, "&+W%s)&+W%s&n %d %s&n [%s&n] &+WBid: &n%s&+W\r\n",
          auction_id, mine_flag, quantity, pad_ansi(obj_short, 45, TRUE).c_str(),
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

  snprintf(buff, MAX_STRING_LENGTH, "&+WAuction &+W%d\r\n", auction_id);
  send_to_char(buff, ch);

  snprintf(buff, MAX_STRING_LENGTH, "&+WSeller: &n%s\r\n", seller_name);
  send_to_char(buff, ch);

  snprintf(buff, MAX_STRING_LENGTH, "&+WTime left: &n%s\r\n", format_time(secs_remaining).c_str() );
  send_to_char(buff, ch);

  if( winning_bidder_pid == 0 )
  {
    snprintf(buff, MAX_STRING_LENGTH, "&+WNo bids received. Starting bid: &n%s\r\n", cur_price_str.c_str());
    send_to_char(buff, ch);
  }
  else
  {
    snprintf(buff, MAX_STRING_LENGTH, "&+WHigh bid: &n%s&+W by &n%s\r\n", cur_price_str.c_str(), winning_bidder_name );
    send_to_char(buff, ch);
  }

  if( buy_price > 0 )
  {
    snprintf(buff, MAX_STRING_LENGTH, "&+WBuy-it-now price: &n%s\r\n", buy_price_str.c_str() );
    send_to_char(buff, ch);
  }

  snprintf(buff, MAX_STRING_LENGTH, "&+WQuantity:&n %d\r\n", quantity );
  send_to_char(buff, ch);

  send_to_char("\r\n", ch);

  if( IS_TRUSTED(ch) )
  {
    snprintf(buff, MAX_STRING_LENGTH, "[&+B%d&n]\r\n", obj_vnum);
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

  extract_obj(tmp_obj);
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
  bool removeAll = FALSE;
  int auc_ids[100], i;
  MYSQL_RES *res;
  MYSQL_ROW auction_row;

  memset(&auc_ids, 0, sizeof(auc_ids));

  if( !strcmp(arg, "all") && GET_LEVEL(ch) == OVERLORD )
  {
    removeAll = TRUE;
    if( !qry("SELECT id FROM auctions WHERE status = 'OPEN'", auction_id) )
      return FALSE;
  }
  else
  {
    if( !qry("SELECT id FROM auctions WHERE id = '%d' and status = 'OPEN'", auction_id) )
      return FALSE;
  }

  res = mysql_store_result(DB);

  i = 0;
  while( auction_row = mysql_fetch_row(res) )
  {
    auc_ids[i++] = atoi(auction_row[0]);
    if( i == 100 )
    {
      break;
    }
  }

  if( !auc_ids[0] )
  {
    send_to_char("&+WThere is no auction with that id!\r\n", ch);
    mysql_free_result(res);
    return TRUE;
  }

  mysql_free_result(res);

  while( i > 0 )
  {
    if( qry("UPDATE auctions SET status = 'REMOVED' WHERE id = '%d'", auc_ids[--i]) )
    {
      snprintf(buff, MAX_STRING_LENGTH, "&+WAuction %d removed.\r\n", auc_ids[i]);
      send_to_char(buff, ch);
      logit(LOG_WIZ, "Auction [%d] removed by %s", auc_ids[i], ch->player.name);
    }
  }
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
    snprintf(buff, MAX_STRING_LENGTH, "&+WYou don't have enough money!\r\nYou need: &n%s\r\n", 
      coin_stringv(to_pay) );
    send_to_char(buff, ch);
    return TRUE;
  }

  SUB_MONEY(ch, to_pay, 0);
  snprintf(buff, MAX_STRING_LENGTH, "&+WYou pay &n%s&n.\r\n", coin_stringv(to_pay));
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
    snprintf(buff, MAX_STRING_LENGTH, "&+WYou bid &n%s&+W on &n%d %s&n.\r\n", coin_stringv(bid_value), quantity, obj_short.c_str());
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
      snprintf(buff, MAX_STRING_LENGTH, "&+WA voice says in your mind, &+W'You were outbid in auction [&+W%d&+W]"
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
      mysql_free_result(res);

      if( !qry("INSERT INTO auction_item_pickups (pid, obj_blob_str) (SELECT '%d', obj_blob_str FROM auctions WHERE id = '%d')", GET_PID(ch), auction_id) )
        return FALSE;

      snprintf(buff, MAX_STRING_LENGTH, "&+WA voice in your mind says, &+W'&n%s &+Wis ready for pickup, oh Great Master!'\r\n", obj_short.c_str() );
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

    snprintf(buff, MAX_STRING_LENGTH, "&+WYou pick up &n%s&+W.&n\r\n", coin_stringv(money));
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
        extract_obj( tmp_obj );
        continue;
      }

      logit(LOG_PLAYER, "%s picked up %s [R:%d] from the auction house.", GET_NAME(ch), tmp_obj->short_description, tmp_obj->R_num);
      snprintf(buff, MAX_STRING_LENGTH, "&+WYou pick up &n%s&+W.\r\n", tmp_obj->short_description);
      send_to_char(buff, ch);
      while( tmp_obj->next_content )
      {
        temp_obj = tmp_obj->next_content;
        tmp_obj->next_content = NULL;
        obj_to_char(tmp_obj, ch);
        tmp_obj = temp_obj;

        logit(LOG_PLAYER, "%s picked up %s [R:%d] from the auction house.", GET_NAME(ch), tmp_obj->short_description, tmp_obj->R_num);
        snprintf(buff, MAX_STRING_LENGTH, "&+WYou pick up &n%s&+W.\r\n", tmp_obj->short_description);
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

bool auction_help(P_char ch, char *arg)
{
	send_to_char("&+WAuction syntax:\r\n- auction list [help]\r\n"
                     "- auction offer <item from your inventory> [starting price in plat] [buy-it-now price in plat] [length of auction in days] [quantity]\r\n"
                     "- auction bid <auction id> <value in plat>\r\n"
                     "- auction info <auction id>\r\n- auction pickup\r\n", ch);
  if( IS_TRUSTED(ch) )
  {
    send_to_char( "- auction resort\r\n- auction remove\r\n", ch );
  }
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
    snprintf(buff, MAX_STRING_LENGTH, "&+WA voice says in your mind, &+W'Your auction for &n%d %s&+W received no bids, and is available for pickup.'\r\n", quantity, obj_short.c_str());
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
    snprintf(buff, MAX_STRING_LENGTH, "&+WA voice says in your mind, &+W'&n%d %s&+W was sold for &n%s&+W, and the money is available for pickup.'\r\n", quantity, obj_short.c_str(), final_price_str.c_str());
    if( !send_to_pid(buff, seller_pid) )
      send_to_pid_offline(buff, seller_pid);

    snprintf(buff, MAX_STRING_LENGTH, "&+WA voice says in your mind, &+W'You bought &n%d %s&+W for &n%s&+W, and it's now available for pickup.'\r\n", quantity, obj_short.c_str(), final_price_str.c_str());
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
		snprintf(tmp, 128, "&+R<1m&n");
		
	} else if( seconds < ( 60 * 60 ) ) {
		snprintf(tmp, 128, "&+R%ldm&n", (seconds / 60) % 60 );
		
	} else if( seconds < ( 6 * 60 * 60  ) ) {
		snprintf(tmp, 128, "&+Y%ldh %ldm&n", (seconds / 3600) % ( 60 * 60 ) , (seconds / 60 ) % 60 );
		
	} else {
		snprintf(tmp, 128, "&+W%ldh&n", (seconds / 3600) % ( 60 * 60 ) );
		
	}
	
	return string(tmp);
}

EqSort::EqSort()
{
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

	flags.push_back(new EqExtraFlag("glow", "that glow", ITEM_GLOW));
	flags.push_back(new EqExtraFlag("noshow", "that don't show", ITEM_NOSHOW));
	flags.push_back(new EqExtraFlag("buried", "that are buried", ITEM_BURIED));
	flags.push_back(new EqExtraFlag("nosell", "that can't be sold", ITEM_NOSELL));
	flags.push_back(new EqExtraFlag("throw2", "that can be thrown from offhand", ITEM_CAN_THROW2));
	flags.push_back(new EqExtraFlag("invisible", "that are invisible", ITEM_INVISIBLE));
	flags.push_back(new EqExtraFlag("norepair", "that can't be repaired", ITEM_NOREPAIR));
	flags.push_back(new EqExtraFlag("cursed", "that are cursed", ITEM_NODROP));
	flags.push_back(new EqExtraFlag("boomerang", "that return after throwing", ITEM_RETURNING));
	flags.push_back(new EqExtraFlag("allowed_races", "that reverse the races can-use list", ITEM_ALLOWED_RACES));
	flags.push_back(new EqExtraFlag("allowed_classes", "that reverse the classes can-use list", ITEM_ALLOWED_CLASSES));
	flags.push_back(new EqExtraFlag("proclib", "that use the old proc format", ITEM_PROCLIB));
	flags.push_back(new EqExtraFlag("hidden", "that are hidden", ITEM_SECRET));
	flags.push_back(new EqExtraFlag("float", "that float on water", ITEM_FLOAT));
	flags.push_back(new EqExtraFlag("noreset", "who knows what it did, now unused", ITEM_NORESET));
	flags.push_back(new EqExtraFlag("nolocate", "that block the locate object spell", ITEM_NOLOCATE));
	flags.push_back(new EqExtraFlag("noidentify", "that block the identify spell", ITEM_NOIDENTIFY));
	flags.push_back(new EqExtraFlag("nosummon", "that block the summon spell", ITEM_NOSUMMON));
	flags.push_back(new EqExtraFlag("lit", "that illuminate the room when worn", ITEM_LIT));
	flags.push_back(new EqExtraFlag("transient", "that dissolve when dropped", ITEM_TRANSIENT));
	flags.push_back(new EqExtraFlag("nosleep", "that block the sleep spell", ITEM_NOSLEEP));
	flags.push_back(new EqExtraFlag("nocharm", "that block charming spells", ITEM_NOCHARM));
	flags.push_back(new EqExtraFlag("twohands", "that require two hands for use", ITEM_TWOHANDS));
	flags.push_back(new EqExtraFlag("norent", "that disappear when renting", ITEM_NORENT));
	flags.push_back(new EqExtraFlag("throw1", "that can be thrown from primary hand", ITEM_CAN_THROW1));
	flags.push_back(new EqExtraFlag("humming", "that hum", ITEM_HUM));
	flags.push_back(new EqExtraFlag("levitates", "that don't fall when dropped", ITEM_LEVITATES));
	flags.push_back(new EqExtraFlag("ignore", "that skip tauto-weight/cost", ITEM_IGNORE));
	flags.push_back(new EqExtraFlag("artifact", "that are artifacts and should not be auctionable", ITEM_ARTIFACT));
	flags.push_back(new EqExtraFlag("wholebody", "that cover the whole body", ITEM_WHOLE_BODY));
	flags.push_back(new EqExtraFlag("wholehead", "that cover the whole head", ITEM_WHOLE_HEAD));
	flags.push_back(new EqExtraFlag("encrusted", "that have a gem encrusted on them", ITEM_ENCRUSTED));

	flags.push_back(new EqExtra2Flag("silver", "that hit monsters vunerable to silver", ITEM2_SILVER));
  flags.push_back(new EqExtra2Flag("bless", "that are blessed", ITEM2_BLESS));
  flags.push_back(new EqExtra2Flag("slay_good", "that hit good alignment hard", ITEM2_SLAY_GOOD));
  flags.push_back(new EqExtra2Flag("slay_evil", "that hit evil alignment hard", ITEM2_SLAY_EVIL));
  flags.push_back(new EqExtra2Flag("slay_undead", "that hit undead hard", ITEM2_SLAY_UNDEAD));
  flags.push_back(new EqExtra2Flag("slay_living", "that hit the living hard", ITEM2_SLAY_LIVING));
  flags.push_back(new EqExtra2Flag("magic", "that are magical", ITEM2_MAGIC));
  flags.push_back(new EqExtra2Flag("linkable", "that can be hitched", ITEM2_LINKABLE));
  flags.push_back(new EqExtra2Flag("noproc", "that are random items", ITEM2_NOPROC));
  flags.push_back(new EqExtra2Flag("notimer", "that are random items", ITEM2_NOTIMER));
  flags.push_back(new EqExtra2Flag("noloot", "that can not be taken from a corpse", ITEM2_NOLOOT));
  flags.push_back(new EqExtra2Flag("crumbleloot", "that crumble when taken", ITEM2_CRUMBLELOOT));
  flags.push_back(new EqExtra2Flag("storeitem", "that were bought in a store", ITEM2_STOREITEM));
  flags.push_back(new EqExtra2Flag("soulbound", "that were soulbound", ITEM2_SOULBIND));
  flags.push_back(new EqExtra2Flag("crafted", "that were crafted", ITEM2_CRAFTED));

	flags.push_back(new EqAffFlag("blind", "that make the wearer blind", AFF_BLIND));
  flags.push_back(new EqAffFlag("invisibility", "that make the wearer invisible", AFF_INVISIBLE));
  flags.push_back(new EqAffFlag("farsee", "that grant farsee", AFF_FARSEE));
  flags.push_back(new EqAffFlag("det_invis", "that detect invisible", AFF_DETECT_INVISIBLE));
  flags.push_back(new EqAffFlag("haste", "that grant haste", AFF_HASTE));
  flags.push_back(new EqAffFlag("sense_life", "that sense lifeforms", AFF_SENSE_LIFE));
  flags.push_back(new EqAffFlag("minor_globe", "that provide some magical protection", AFF_MINOR_GLOBE));
  flags.push_back(new EqAffFlag("stone_skin", "that grant the wearer stone skin", AFF_STONE_SKIN));
  flags.push_back(new EqAffFlag("ud_vision", "that grant underdark vision", AFF_UD_VISION));
  flags.push_back(new EqAffFlag("armor", "that block armor spells", AFF_ARMOR));
  flags.push_back(new EqAffFlag("wraithform", "that grant wraithform and fall off", AFF_WRAITHFORM));
  flags.push_back(new EqAffFlag("waterbreath", "that grant waterbreathing", AFF_WATERBREATH));
  flags.push_back(new EqAffFlag("ko", "that knock the wearer out", AFF_KNOCKED_OUT));
  flags.push_back(new EqAffFlag("prot_evil", "that grant protection from evil", AFF_PROTECT_EVIL));
  flags.push_back(new EqAffFlag("bound", "that bind the wearer", AFF_BOUND));
  flags.push_back(new EqAffFlag("slow_poison", "that delay the effects of poison", AFF_SLOW_POISON));
  flags.push_back(new EqAffFlag("prot_good", "that grant protection from good", AFF_PROTECT_GOOD));
  flags.push_back(new EqAffFlag("sleep", "that used to put the wearer to sleep", AFF_SLEEP));
  flags.push_back(new EqAffFlag("skill_aware", "that shouldn't exist", AFF_SKILL_AWARE));
  flags.push_back(new EqAffFlag("sneak", "that grant sneak to the wearer", AFF_SNEAK));
  flags.push_back(new EqAffFlag("hide", "that used to grant the user hide", AFF_HIDE));
  flags.push_back(new EqAffFlag("fear", "that look really scary", AFF_FEAR));
  flags.push_back(new EqAffFlag("charm", "that look really sweet", AFF_CHARM));
  flags.push_back(new EqAffFlag("meditate", "that look tranquilizing", AFF_MEDITATE));
  flags.push_back(new EqAffFlag("barkskin", "that grant the wearer barkskin", AFF_BARKSKIN));
  flags.push_back(new EqAffFlag("infravision", "that grant the wearer infravision", AFF_INFRAVISION));
  flags.push_back(new EqAffFlag("levitate", "that levitate the wearer", AFF_LEVITATE));
  flags.push_back(new EqAffFlag("fly", "that make the wearer fly", AFF_FLY));
  flags.push_back(new EqAffFlag("aware", "that make the wearer more aware of their surroundings", AFF_AWARE));
  flags.push_back(new EqAffFlag("prot_fire", "that grant protection from fire", AFF_PROT_FIRE));
  flags.push_back(new EqAffFlag("camping", "that look like tent poles", AFF_CAMPING));
  flags.push_back(new EqAffFlag("biofeedback", "that grant the wearer biofeedback", AFF_BIOFEEDBACK));

  flags.push_back(new EqAff2Flag("fireshield", "that grant the wearer fireshield", AFF2_FIRESHIELD));
  flags.push_back(new EqAff2Flag("ultra", "that grant the wearer ultravision", AFF2_ULTRAVISION));
  flags.push_back(new EqAff2Flag("det_evil", "that help the wearer sense evil", AFF2_DETECT_EVIL));
  flags.push_back(new EqAff2Flag("det_good", "that help the wearer snse good", AFF2_DETECT_GOOD));
  flags.push_back(new EqAff2Flag("det_magic", "that help the wearer sense magic", AFF2_DETECT_MAGIC));
  flags.push_back(new EqAff2Flag("maj_phys", "that look big and physical", AFF2_MAJOR_PHYSICAL));
  flags.push_back(new EqAff2Flag("prot_cold", "that grant protection from cold", AFF2_PROT_COLD));
  flags.push_back(new EqAff2Flag("prot_light", "that grant protection from lightning", AFF2_PROT_LIGHTNING));
  flags.push_back(new EqAff2Flag("minor_para", "that used to paralyze the wearer", AFF2_MINOR_PARALYSIS));
  flags.push_back(new EqAff2Flag("major_para", "that used to permenantly paralyze the wearer", AFF2_MAJOR_PARALYSIS));
  flags.push_back(new EqAff2Flag("slow", "that slow the wearer", AFF2_SLOW));
  flags.push_back(new EqAff2Flag("globe", "that provide magical protection from most spells", AFF2_GLOBE));
  flags.push_back(new EqAff2Flag("prot_gas", "that grant protection from gas", AFF2_PROT_GAS));
  flags.push_back(new EqAff2Flag("prot_acid", "that grant protection from acid", AFF2_PROT_ACID));
  flags.push_back(new EqAff2Flag("poisoned", "that poison the wearer", AFF2_POISONED));
  flags.push_back(new EqAff2Flag("soulshield", "that grant soulshield", AFF2_SOULSHIELD));
  flags.push_back(new EqAff2Flag("silenced", "that silence the wearer", AFF2_SILENCED));
  flags.push_back(new EqAff2Flag("minor_invis", "that grant minor invisibility", AFF2_MINOR_INVIS));
  flags.push_back(new EqAff2Flag("vamp", "that grant vampiric touch", AFF2_VAMPIRIC_TOUCH));
  flags.push_back(new EqAff2Flag("stunned", "that look absolutely stunning", AFF2_STUNNED));
  flags.push_back(new EqAff2Flag("earth_aura", "that grant earth aura", AFF2_EARTH_AURA));
  flags.push_back(new EqAff2Flag("water_aura", "that grant water aura", AFF2_WATER_AURA));
  flags.push_back(new EqAff2Flag("fire_aura", "that grant fire aura", AFF2_FIRE_AURA));
  flags.push_back(new EqAff2Flag("air_aura", "that grant air aura", AFF2_AIR_AURA));
  flags.push_back(new EqAff2Flag("hold_breath", "that are breath taking", AFF2_HOLDING_BREATH));
  flags.push_back(new EqAff2Flag("memming", "that are mesmorizing", AFF2_MEMORIZING));
  flags.push_back(new EqAff2Flag("drowning", "that make you want to choke", AFF2_IS_DROWNING));
  flags.push_back(new EqAff2Flag("passdoor", "that grant the ability to walk through doors", AFF2_PASSDOOR));
  flags.push_back(new EqAff2Flag("flurry", "that grant flurry", AFF2_FLURRY));
  flags.push_back(new EqAff2Flag("casting", "that invoke feelings of casting a spell", AFF2_CASTING));
  flags.push_back(new EqAff2Flag("scribing", "that make you want to write", AFF2_SCRIBING));
  flags.push_back(new EqAff2Flag("hunter", "that make you want to kill", AFF2_HUNTER));

  flags.push_back(new EqAff3Flag("tensors", "that make you wonder", AFF3_TENSORS_DISC));
  flags.push_back(new EqAff3Flag("tracking", "that make you want to search", AFF3_TRACKING));
  flags.push_back(new EqAff3Flag("singing", "that make you think you can sing", AFF3_SINGING));
  flags.push_back(new EqAff3Flag("ecto", "that grant ecotplasmic form", AFF3_ECTOPLASMIC_FORM));
  flags.push_back(new EqAff3Flag("absorbing", "that make you feel fat", AFF3_ABSORBING));
  flags.push_back(new EqAff3Flag("prot_animal", "that grant protection from animals", AFF3_PROT_ANIMAL));
  flags.push_back(new EqAff3Flag("sp_ward", "that grant spirit ward", AFF3_SPIRIT_WARD));
  flags.push_back(new EqAff3Flag("gr_sp_ward", "that grant greater spirit ward", AFF3_GR_SPIRIT_WARD));
  flags.push_back(new EqAff3Flag("mindblank", "that grant the wearer mindblank status", AFF3_NON_DETECTION));
  flags.push_back(new EqAff3Flag("silver", "that can cut lycanthropes", AFF3_SILVER));
  flags.push_back(new EqAff3Flag("plusone", "that can hit slightly magical creatures", AFF3_PLUSONE));
  flags.push_back(new EqAff3Flag("plustwo", "that can hit some magical creatures", AFF3_PLUSTWO));
  flags.push_back(new EqAff3Flag("plusthree", "that can hit most magical creatures", AFF3_PLUSTHREE));
  flags.push_back(new EqAff3Flag("plusfour", "that can hit really magical creatures", AFF3_PLUSFOUR));
  flags.push_back(new EqAff3Flag("plusfive", "that can hit all creatures", AFF3_PLUSFIVE));
  flags.push_back(new EqAff3Flag("enlarge", "that make the wearer bigger", AFF3_ENLARGE));
  flags.push_back(new EqAff3Flag("reduce", "that make the wearer smaller", AFF3_REDUCE));
  flags.push_back(new EqAff3Flag("cover", "that make the wearer harder to hit via range", AFF3_COVER));
  flags.push_back(new EqAff3Flag("fourarms", "that grant the wearer four arms to fight with", AFF3_FOUR_ARMS));
  flags.push_back(new EqAff3Flag("inertial", "that grant the wearer inertial barrier", AFF3_INERTIAL_BARRIER));
  flags.push_back(new EqAff3Flag("lightningshield", "that grant the wearer lightningshield", AFF3_LIGHTNINGSHIELD));
  flags.push_back(new EqAff3Flag("coldshield", "that grant the wearer coldshield", AFF3_COLDSHIELD));
  flags.push_back(new EqAff3Flag("cannibalize", "that feed the wearer mana from spelldamage", AFF3_CANNIBALIZE));
  flags.push_back(new EqAff3Flag("swimming", "that make the wearer want a tan", AFF3_SWIMMING));
  flags.push_back(new EqAff3Flag("toiw", "that grant the wearer a tower of iron will", AFF3_TOWER_IRON_WILL));
  flags.push_back(new EqAff3Flag("underwater", "that make the wearer feel bankrupt", AFF3_UNDERWATER));
  flags.push_back(new EqAff3Flag("blur", "that grant the wearer blur", AFF3_BLUR));
  flags.push_back(new EqAff3Flag("healing", "that grant the wearer enhanced healing", AFF3_ENHANCE_HEALING));
  flags.push_back(new EqAff3Flag("elemental_form", "that block elemental form", AFF3_ELEMENTAL_FORM));
  flags.push_back(new EqAff3Flag("pwt", "that prevent the wearer from leaving tracks", AFF3_PASS_WITHOUT_TRACE));
  flags.push_back(new EqAff3Flag("pal_aura", "that make you feel more holy", AFF3_PALADIN_AURA));
  flags.push_back(new EqAff3Flag("famine", "that make you hungry", AFF3_FAMINE));

  flags.push_back(new EqAff4Flag("looter", "that mark you as a corpse looter", AFF4_LOOTER));
  flags.push_back(new EqAff4Flag("plague", "that make you really sick", AFF4_CARRY_PLAGUE));
  flags.push_back(new EqAff4Flag("sacking", "that make you feel unemployed", AFF4_SACKING));
  flags.push_back(new EqAff4Flag("sense_follower", "that grants the wearer awareness of invisible followers", AFF4_SENSE_FOLLOWER));
  flags.push_back(new EqAff4Flag("stornogs", "that block stornogs spheres", AFF4_STORNOGS_SPHERES));
  flags.push_back(new EqAff4Flag("stornogs_gr", "that block greater stornogs spheres", AFF4_STORNOGS_GREATER_SPHERES));
  flags.push_back(new EqAff4Flag("vamp_form", "that make you a vampire", AFF4_VAMPIRE_FORM));
  flags.push_back(new EqAff4Flag("no_unmorph", "that keep you morphed", AFF4_NO_UNMORPH));
  flags.push_back(new EqAff4Flag("holy_sac", "that grant holy sacrifice", AFF4_HOLY_SACRIFICE));
  flags.push_back(new EqAff4Flag("battle_ecs", "that grant battle ecstasy", AFF4_BATTLE_ECSTASY));
  flags.push_back(new EqAff4Flag("dazzle", "that grant dazzle", AFF4_DAZZLER));
  flags.push_back(new EqAff4Flag("phan_form", "that grant phantasmal form", AFF4_PHANTASMAL_FORM));
  flags.push_back(new EqAff4Flag("nofear", "that make the wearer immune to fear-based magic", AFF4_NOFEAR));
  flags.push_back(new EqAff4Flag("regen", "that grant regeneration", AFF4_REGENERATION));
  flags.push_back(new EqAff4Flag("deaf", "that make you hard of hearing", AFF4_DEAF));
  flags.push_back(new EqAff4Flag("battletide", "heals group members when wearer does damage", AFF4_BATTLETIDE));
  flags.push_back(new EqAff4Flag("epic_increase", "that grant an bonus to earned epics", AFF4_EPIC_INCREASE));
  flags.push_back(new EqAff4Flag("mage_flame", "that grant a magical flame for vision", AFF4_MAGE_FLAME));
  flags.push_back(new EqAff4Flag("globe_dark", "that grant a globe of darkness to avoid the sun", AFF4_GLOBE_OF_DARKNESS));
  flags.push_back(new EqAff4Flag("deflect", "that grant perm deflect to the wearer", AFF4_DEFLECT));
  flags.push_back(new EqAff4Flag("hawkvision", "that grant hawkvision", AFF4_HAWKVISION));
  flags.push_back(new EqAff4Flag("multiclass", "that really screw things up", AFF4_MULTI_CLASS));
  flags.push_back(new EqAff4Flag("sanctuary", "that grant sanctuary", AFF4_SANCTUARY));
  flags.push_back(new EqAff4Flag("hellfire", "that grant hellfire", AFF4_HELLFIRE));
  flags.push_back(new EqAff4Flag("sense_holy", "that sense holiness", AFF4_SENSE_HOLINESS));
  flags.push_back(new EqAff4Flag("prot_living", "that grant protection from the living", AFF4_PROT_LIVING));
  flags.push_back(new EqAff4Flag("det_illusion", "that grant awareness to illusions", AFF4_DETECT_ILLUSION));
  flags.push_back(new EqAff4Flag("ice_aura", "that grant ice aura", AFF4_ICE_AURA));
  flags.push_back(new EqAff4Flag("reverse_polarity", "that make you hate shaman heals", AFF4_REV_POLARITY));
  flags.push_back(new EqAff4Flag("neg_shield", "that grant a negative shield", AFF4_NEG_SHIELD));
  flags.push_back(new EqAff4Flag("tupor", "that make you sleepy", AFF4_TUPOR));
  flags.push_back(new EqAff4Flag("wildmagic", "that grant wildmagic status", AFF4_WILDMAGIC));

  flags.push_back(new EqAff5Flag("dazzlee", "that make the world sparkle", AFF5_DAZZLEE));
  flags.push_back(new EqAff5Flag("mental_anguish", "that make it really hard to focus", AFF5_MENTAL_ANGUISH));
  flags.push_back(new EqAff5Flag("memory_block", "that make it really hard to cast", AFF5_MEMORY_BLOCK));
  flags.push_back(new EqAff5Flag("vines", "that make you one with the earth", AFF5_VINES));
  flags.push_back(new EqAff5Flag("ethereal_alliance", "that make you one with the ether", AFF5_ETHEREAL_ALLIANCE));
  flags.push_back(new EqAff5Flag("blood_scent", "that make you smell blood", AFF5_BLOOD_SCENT));
  flags.push_back(new EqAff5Flag("flesh_armor", "that grant immunity to flesh armor", AFF5_FLESH_ARMOR));
  flags.push_back(new EqAff5Flag("wet", "that make you all wet", AFF5_WET));
  flags.push_back(new EqAff5Flag("holy_dharma", "that prevent holy dharma from working", AFF5_HOLY_DHARMA));
  flags.push_back(new EqAff5Flag("enhanced_hide", "that make you really hidden", AFF5_ENH_HIDE));
  flags.push_back(new EqAff5Flag("listen", "that make the birds really loud", AFF5_LISTEN));
  flags.push_back(new EqAff5Flag("prot_undead", "that grant protection from undead", AFF5_PROT_UNDEAD));
  flags.push_back(new EqAff5Flag("imprison", "that make you feel like your wearing stripes", AFF5_IMPRISON));
  flags.push_back(new EqAff5Flag("titan_form", "that make you really big", AFF5_TITAN_FORM));
  flags.push_back(new EqAff5Flag("delirium", "that make you really confused", AFF5_DELIRIUM));
  flags.push_back(new EqAff5Flag("shade_movement", "that allow you to remain hidden where there are shadows", AFF5_SHADE_MOVEMENT));
  flags.push_back(new EqAff5Flag("noblind", "that prevent blindness stopping you", AFF5_NOBLIND));
  flags.push_back(new EqAff5Flag("magic_glow", "that make you feel like you've just had sex", AFF5_MAGICAL_GLOW));
  flags.push_back(new EqAff5Flag("refreshing_glow", "that make you feel like you just showered", AFF5_REFRESHING_GLOW));
  flags.push_back(new EqAff5Flag("mine", "that grant miner's sight", AFF5_MINE));
  flags.push_back(new EqAff5Flag("stance_offensive", "that make you more offensive", AFF5_STANCE_OFFENSIVE));
  flags.push_back(new EqAff5Flag("stance_defensive", "that make you more defensive", AFF5_STANCE_DEFENSIVE));
  flags.push_back(new EqAff5Flag("obscuring_mist", "that surround the wearer in a concealing mist", AFF5_OBSCURING_MIST));
  flags.push_back(new EqAff5Flag("not_offensive", "that make you very peaceful", AFF5_NOT_OFFENSIVE));
  flags.push_back(new EqAff5Flag("decaying_flesh", "that make your flesh decay", AFF5_DECAYING_FLESH));
  flags.push_back(new EqAff5Flag("dreadnaught", "that make you feel sturdier", AFF5_DREADNAUGHT));
  flags.push_back(new EqAff5Flag("forest_sight", "that make the forests open up", AFF5_FOREST_SIGHT));
  flags.push_back(new EqAff5Flag("thornskin", "that grant thornskin", AFF5_THORNSKIN));
  flags.push_back(new EqAff5Flag("following", "that screws with your ability to follow", AFF5_FOLLOWING));
}

string EqSort::getSortFlagsString(P_obj obj)
{
	string keywords;

	if( !obj )
  {
    return keywords;
  }

	for( int i = 0; i < flags.size(); i++ )
  {
		if( flags[i] && flags[i]->match(obj) )
    {
			keywords += " ";
			keywords += flags[i]->gKey();
			keywords += ",";
		}
	}

	return keywords;
}

string EqSort::getDescString(const char *keyword)
{
	if( strlen(keyword) < 1 )
    return string();

	for( int i = 0; i < flags.size(); i++ )
  {
		if( flags[i] && !strcmp(keyword, flags[i]->gKey() ) )
    {
			return flags[i]->gDesc();
		}
	}

	return string();
}

string EqSort::getKeyword(const int num)
{
  if( num < 0 || num > flags.size() )
  {
    return "NULL";
  }

  return flags[num]->gKey();
}

bool EqSort::isKeyword(const char *keyword)
{
	if( strlen(keyword) < 1 )
    return false;

	for( int i = 0; i < flags.size(); i++ )
  {
		if( flags[i] && !strcmp(keyword, flags[i]->gKey() ) )
    {
			return true;
		}
	}

	return false;
}
#endif // #ifdef __NO_MYSQL__
