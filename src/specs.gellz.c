
/*
        This is where Gellz's Procs and Special Procedures and Events reside.
        Nathan Wheeler - 21-05-2015 - email nwheeler@iinet.net.au
*/
#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <list>
#include <vector>
#include <math.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "map.h"
#include "damage.h"
#include "structs.h"
#include "specs.gellz.h"
using namespace std;
#include "ships.h"

// *****************************************************************
//	Gellz TEST Object procedure... ONLY for testing
// *****************************************************************
int gellz_test_obj_procs(P_obj obj, P_char ch, int cmd, char *argument)
{ //PLACEHOLDER ONLY
  int         curr_time;
  char        argstring1[MAX_STRING_LENGTH];
  char        argstring2[MAX_STRING_LENGTH];
  char        argstring3[MAX_STRING_LENGTH];
  char		    buf[MAX_STRING_LENGTH];
  string      argstring;
  P_ship      ship;
	ShipVisitor svs;
  P_char      owner;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_SAY || !*argument || !IS_ALIVE(ch) || !IS_TRUSTED(ch) )
  {
    return FALSE;
  }

  argument = one_argument(argument, argstring1); //get one argument from list
  argument = one_argument(argument, argstring2); //get one argument from list
  argument = one_argument(argument, argstring3); //get one argument from list

  if (!strcmp(argstring1, "achieve"))
  {
      act("&+ySAID ACHIEVE\n&n", FALSE, ch, obj, obj, TO_CHAR );
    if (!strcmp(argstring2, "reset"))
    {
      act("&+ySaid Achieve - RESET \n&n", FALSE, ch, obj, obj, TO_CHAR );
      if(affected_by_spell(ch, ACH_DEATHSDOOR))
        {affect_from_char(ch, ACH_DEATHSDOOR);
      act("&+YWe ran the affect_from...\n&n", FALSE, ch, obj, obj, TO_CHAR );}
    } //end reset keyword
  } //end achieve keyword

  if( !strcmp(argstring1, "ship") )
  {
    if( !*argstring2 )
    {
      send_to_char( "This command needs arguments:\n"
        "'all'  - list all ships and locations.\n"
        "rename - rename a ship (not done since you can just 'rename ship <owner> <new name>'.\n"
        "owner  - change the owner of a ship from one char to another (useful for failed renames).\n"
        "delete - delete a ship.\n", ch );
      return TRUE;
    }
    if( !strcmp(argstring2, "all") )
    {
      act( "&+yListing &+YALL ships &+yin game:&n", FALSE, ch, obj, obj, TO_CHAR );
      // LOOP through all ships
      for( bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs) )
	    {
        send_to_char_f(ch, "&+yShip:&+C %s&+y Owner: &+C%s ", SHIP_NAME(svs), SHIP_OWNER(svs));
        send_to_char_f(ch, "&+yRoom: %s ", world[svs->location].name);
        if( SHIP_DOCKED(svs) )
        {
          send_to_char_f(ch, "&+y | &+LDOCKED&+y");
        }
        if( SHIP_ANCHORED(svs) )
        {
          send_to_char_f(ch, "&+y | &+YANCHORED&+y");
        }
        if( SHIP_IMMOBILE(svs) )
        {
          send_to_char_f(ch, "&+y | &+rIMMOBILE&+y");
        }
        if( SHIP_SINKING(svs) )
        {
          send_to_char_f(ch, "&+y | &+RSINKING&+y");
        }
        send_to_char_f(ch, "\n");
	    }
	    return TRUE;
    } //End say keyword ALL
    else if (!strcmp(argstring2, "rename"))
    {
      act ("&+YJust use the &+w'rename ship <owner> <new name>'&+Y command.", FALSE, ch, obj, obj, TO_CHAR);
    }
    else if (!strcmp(argstring2, "owner"))
    {
      if( !*argstring3 )
      {
        send_to_char( "&+YYou must supply the name of the &+yold&+Y captain and the &+Bnew&+Y captain of the ship.&n\n", ch);
        return TRUE;
      }
      if( !(ship = get_ship_from_owner( argstring3 )) )
      {
        snprintf(buf, MAX_STRING_LENGTH, "Could not find a ship for '%s'.\n"
          "&+YYou must supply the name of the &+yold&+Y captain of the ship.&n\n", skip_spaces(argument) );
        send_to_char( buf, ch );
        return TRUE;
      }
      if( !*argument || !(owner = get_char_vis(ch, skip_spaces(argument))) )
      {
        snprintf(buf, MAX_STRING_LENGTH, "Could not find char '%s' (must be in game).\n"
          "&+YYou must supply the name of the &+Bnew&+Y captain of the ship.&n\n", skip_spaces(argument) );
        send_to_char( buf, ch );
        return TRUE;
      }
      if( !rename_ship_owner(argstring3, skip_spaces(argument)) )
      {
        send_to_char( "Ship re-owner failed.", ch );
      }
      else
      {
        snprintf(buf, MAX_STRING_LENGTH, "Ship '%s' owner changed from %s to %s.", strip_ansi(ship->name).c_str(), argstring3,
          skip_spaces(argument) );
        logit(LOG_SHIP, buf );
        send_to_char_f( ch, "Ship '%s' now owned by %s.\n", ship->name, GET_NAME(owner) );
      }
      return TRUE;
    }
    else if (!strcmp(argstring2, "delete"))
    {
      if( !*argstring3 )
      {
        send_to_char( "&+YYou must supply the name of the captain of the ship you want to delete.&n\n\r", ch);
        return TRUE;
      }
	    act ("&+BExperimental Deletion of Ship...", FALSE, ch, NULL, NULL, TO_CHAR);
      CAP(argstring3);
      // First, we hunt for the ship, and make sure there is one (we can use the same loop as above).
      for( bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs) )
	    {
        // Skip pirate ships.
        if( SHIP_OWNER(svs) == NULL )
        {
          continue;
        }
        // Check if we have the right ship using the same code as to display the owner's name
        if( !strcmp( argstring3, SHIP_OWNER(svs) ) )
        {
          act( "&+RDeleting ship...&n", FALSE, ch, NULL, NULL, TO_CHAR );
          P_ship ship = svs;
          fn = shipObjHash.erase(svs);
          delete_ship(ship);
          return TRUE;
        }
      }
      snprintf(buf, MAX_STRING_LENGTH, "&+WShip of '%s' not found!", argstring3 );
      act( buf, FALSE, ch, NULL, NULL, TO_CHAR );
      return TRUE;
    }
  } // End arg1 == "ship"
  return FALSE;
} //End gellz_test_obj_procs

// *****************************************************************************
// *****************************************************************************
//                              GELLZ CARD GAME
// *****************************************************************************
int magic_deck(P_obj obj, P_char ch, int cmd, char *argument)
{
  static int game_on = BJ_PREBID;
  static bool lock_game = FALSE;
  int   curr_time;
  char  arg[MAX_INPUT_LENGTH];
  char  buf [MAX_STRING_LENGTH];
  char  betbuf2[MAX_STRING_LENGTH];
  char  betbuf1[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
  {
    // Gonna cheese this and initialize here.
    clear_hands( 1 );
    clear_hands( 2 );
    player_total = dealer_total = dealercards = bettype = betamt = 0;
    return TRUE;
  }

  // Aaah.. the suspense! ;)
  if( cmd == CMD_PERIODIC && game_on == BJ_DEALERSTURN )
  {
    if( dealer_total < 17 && dealercards < 5 )
    {
// This won't work yet, 'cause we have to save a pointer to ch somewhere and reference it
//   since ch is NULL when CMD_PERIODIC fires.
      act( "\n&+yThe &+CDealer&+y takes a new card...&n", FALSE, ch, obj, ch, TO_CHAR );
      needcard(2, ch);
      showhand(obj, ch, cmd, argument, 2);
    }
    // End game
    else
    {
      if( dealer_total < 22 )
      {
        act( "\n&+yThe &+CDealer&+y decides to &+Wstay&+y with his current hand!&n\n\n", FALSE, ch, obj, ch, TO_CHAR);
      }
      else
      {
        send_to_char("&+CDealer&+R BUST&+y, so &+RY&+CO&+BU &+GW&+YI&+MN&+C!&+R!&+y&n\n", ch);
        do_win(ch, bettype, betamt, 1);
      }
    }
  }

  if( cmd == CMD_SAY && IS_ALIVE(ch) && IS_TRUSTED(ch) )
  {
    one_argument(argument, arg);
    if( !strcmp( arg, "lock" ) )
    {
      lock_game = TRUE;
      send_to_char( "&+RBlackjack game locked.\n\r", ch );
      return TRUE;
    }
    if( !strcmp( arg, "unlock" ) )
    {
      lock_game = FALSE;
      send_to_char( "&+RBlackjack game unlocked.\n\r", ch );
      return TRUE;
    }
  }

  if( lock_game )
  {
    return FALSE;
  }

  // BETTING START
  if( cmd == CMD_OFFER )
  {
    if( game_on != BJ_PREBID )
    {
      act ("&+yA &+Wgame&+y already appears to be in progress. Please &+Wfold&+y or complete that one first...", FALSE, ch, obj, obj, TO_CHAR);
      return TRUE;
    }

    argument = one_argument(argument, betbuf1); //get one argument from list
    argument = one_argument(argument, betbuf2); //get SECOND argument from list
    // We only need to check the second argument, since if it exists, the first does also.
    if( !betbuf2 )
    {
      act( STR_CARDS_ARG_FAIL, FALSE, ch, NULL, NULL, TO_CHAR );
      return TRUE;
    }
    betamt = atoi(betbuf1);
    bettype = coin_type(betbuf2);

    // Both args entered, but bad args (no mithril/adamantium coins).
    if( betamt <= 0 || bettype < 0 || bettype > 3 )
    {
      act( STR_CARDS_TYPE_FAIL, FALSE, ch, obj, obj, TO_CHAR );
      betamt = 0;
      bettype = 0;
      return TRUE;
    }

    if( bettype == 0 )
    {
      if( betamt > GET_COPPER(ch) )
      {
        act(STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
      // 1000 Copper per Platinum.
      else if( betamt > 1000 * get_property("blackjack.MaxBetInPlatinum", 100) )
      {
        act(STR_CARDS_BIGBID_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
      else
      {
        act(STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
        game_on = BJ_POSTBID;
        strbettype = STR_COPP;
      }
    }
    else if( bettype == 1 )
    {
      if( betamt > GET_SILVER(ch) )
      {
        act( STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
      // 100 Silver per Platinum.
      else if( betamt > 100 * get_property("blackjack.MaxBetInPlatinum", 100) )
      {
        act(STR_CARDS_BIGBID_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
      else
      {
        act( STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
        game_on = BJ_POSTBID;
        strbettype=STR_SILV;
      }
    }
    else if( bettype == 2 )
    {
      if( betamt > GET_GOLD(ch) )
      {
        act( STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
      // 10 Gold per Platinum.
      else if( betamt > 10 * get_property("blackjack.MaxBetInPlatinum", 100) )
      {
        act(STR_CARDS_BIGBID_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
      else
      {
        act (STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
        game_on = BJ_POSTBID;
        strbettype = STR_GOLD;
      }
    }
    else if( bettype == 3 )
    {
      if( betamt > GET_PLATINUM(ch) )
      {
        act( STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
      else if( betamt > get_property("blackjack.MaxBetInPlatinum", 100) )
      {
        act(STR_CARDS_BIGBID_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
	    else
      {
        act( STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
        game_on = BJ_POSTBID;
        strbettype = STR_PLAT;
      }
    }
    // Remove cash from player and put it on the table!
    send_to_char_f(ch, "\n&+yYou toss &n%d ", betamt);
    if( bettype==0 )
    {
      GET_COPPER(ch)-=betamt;
      send_to_char_f(ch, STR_COPP);
    }
    else if( bettype==1 )
    {
      GET_SILVER(ch)-=betamt;
      send_to_char_f(ch, STR_SILV);
    }
    else if( bettype==2 )
    {
      GET_GOLD(ch)-=betamt;
      send_to_char_f(ch, STR_GOLD);
    }
    else if( bettype==3 )
    {
      GET_PLATINUM(ch)-=betamt;
      send_to_char_f(ch, STR_PLAT);
    }
    else
    {
      debug( "magic_deck: BROKEN COIN TYPE %d not between 0 and 3.", bettype );
      send_to_char_f(ch, "&=BRWTFs&n");
    }
    send_to_char_f(ch, "&+y on the table.&n\n");
    return TRUE;
  } // End cmd == CMD_OFFER

  // We don't want to bother with in-game stuff if the game hasn't started.
  if( game_on == BJ_PREBID )
  {
    return FALSE;
  }

  // If they said something..
  if( cmd == CMD_SAY && *argument )
  {
    one_argument(argument, arg); //get one argument from list

    // Start Keyword for 'deal' - deal initial cards
    if( !strcmp(arg, "deal") )
    {
      // Silenced players get a warning?
      if( !say(ch, argument) )
      {
        act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      // If we're not post-bid, we're not ready to deal.
      if( game_on != BJ_POSTBID )
      {
        // If they haven't bid yet.
        if( game_on == BJ_PREBID )
        {
          act( STR_CARDS_BID_FAIL, FALSE, ch, obj, ch, TO_CHAR);
        }
        // Otherwise a game is already in progress.
        else
        {
          act( STR_CARDS_GAME_ON, FALSE, ch, obj, ch, TO_CHAR);
        }
        return TRUE;
      }
      // Can deal and play game
      else
      {
        act( STR_CARDS_SHUFFLE, FALSE, ch, obj, ch, TO_CHAR);
        setup_deck();
        clear_hands(1);
        clear_hands(2);
        act( STR_CARDS_DEAL, FALSE, ch, obj, ch, TO_CHAR);
        needcard( 1, ch); //player 1st card
        needcard( 2, ch); //dealer 1st card
        needcard( 1, ch); //player 2nd card
        showhand(obj, ch, cmd, arg, 1); //show player hand
        showhand(obj, ch, cmd, arg, 2); //show dealer hand
        // Assuming you just wait until the end of game to deal it?
//        needcard(2,ch); //dealer 2nd card (HOW DO WE HIDE IT?)
        snprintf(buf, MAX_STRING_LENGTH, "&+yThe &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y flicker over the &+wdeck&+y once more, and you realise that you can choose to either &+Whit&+y, &+Wstay&+y or &+Wfold&+y simply by telling the cards what you wish to do.&n\n");
        act( buf, FALSE, ch, obj, ch, TO_CHAR);
        game_on = BJ_POSTDEAL;
        return TRUE;
      }
    } // End say keyword 'deal'
    // Start Keyword for 'stay' - sitting
    else if (!strcmp(arg, "stay"))
    {
      if( game_on != BJ_POSTDEAL && game_on != BJ_POSTHIT )
      {
        act( STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR);
        return TRUE;
      }
      if( !say(ch,arg) )
      {
        act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      // Show player's hand.
      showhand(obj, ch, cmd, argument, 1);
      // Dealer's turn to draw.
      send_to_char("&+yThe &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y fly over the &+wdeck&+y as the &+CDealer&+y begins his &+Yturn!&n.\n", ch);

      act( "\n&+yThe &+CDealer&+y turns over the other card...&n", FALSE, ch, obj, ch, TO_CHAR );
      needcard(2, ch);
      showhand(obj, ch, cmd, argument, 2);

      // Repeat dealer get card while dealer < 21 (22)
      while( dealer_total < 17 && dealercards < 5 )
      {
        act( "\n&+yThe &+CDealer&+y takes a new card...&n", FALSE, ch, obj, ch, TO_CHAR );
        needcard(2, ch);
      }
      if( dealer_total < 22 )
      {
        act ("\n&+yThe &+CDealer&+y decides to &+Wstay&+y with his current hand!&n\n\n", FALSE, ch, obj, ch, TO_CHAR);
      }
      showhand(obj, ch, cmd, argument, 2);

      if( dealer_total > 21 )
      {
        snprintf(buf, MAX_STRING_LENGTH, "&+CDealer&+R BUST&+y, so &+RY&+CO&+BU &+GW&+YI&+MN&+C!&+R!&+y&n\n");
        send_to_char(buf, ch);
        do_win(ch, bettype, 2*betamt, 1);
      }
      else if( player_total > dealer_total )
      {
        do_win(ch, bettype, 2*betamt, 1);
        snprintf(buf, MAX_STRING_LENGTH, "&+RY&+CO&+BU &+GW&+YI&+MN&+C!&+R!&+y! with %d versus the dealers %d.\n", player_total, dealer_total);
        send_to_char(buf, ch);
      }
      else if( player_total == dealer_total )
      {
        act("&+yA &+YPUSH!&+y No winner no loser! Your &+Wbet&+y has been refunded.", FALSE, ch, obj, ch, TO_CHAR);
        do_win(ch, bettype, betamt, 1);
      }
      // Can assume player total < dealer total.
      else
      {
        snprintf(buf, MAX_STRING_LENGTH, "&+RYou LOOSE!!&+C Dealers %d &+rbeats your %d.\n", dealer_total, player_total);
        send_to_char(buf, ch);
        do_win(ch, bettype, betamt, 2);
      }
      // Game over - reset
      clear_hands(1);
      clear_hands(2);
      game_on = BJ_PREBID;
      player_total = dealer_total = dealercards = bettype = betamt = 0;
      return TRUE;
    }                         // End say keyword for 'stay'
    // Start Keyword for 'fold' - player folds
    else if( !strcmp(arg, "fold") )
    {
      if( !say(ch,arg) )
      {
        act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      // If we're not in a position to fold (haven't bid yet/dealt)
      if( game_on != BJ_POSTDEAL && game_on != BJ_POSTHIT )
      {
        act( STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR); // Was game in progress.
      }
      else
      {
        act( STR_CARDS_FOLD, FALSE, ch, obj, ch, TO_CHAR);
      }
      // Is actually a loss, but ok; dealer wins.
      do_win(ch, bettype, betamt, 2);
      clear_hands(1);
      clear_hands(2);
      game_on = BJ_PREBID;
      player_total = dealer_total = dealercards = bettype = betamt = 0;
      return TRUE;
    } // End say keyword for 'fold'
    // Start Keyword for 'hit' - add another card to player's hand.
    else if( !strcmp(arg, "hit") )
    {
      if( !say(ch,arg) )
      {
        act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      // If we're not in a position to hit
      if( game_on != BJ_POSTDEAL && game_on != BJ_POSTHIT )
      {
        act( STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR);
        return TRUE;
      }
      needcard(1, ch);
      act("&+yIn a card leaves the &+wdeck&+y and &+Yreveals&+y itself.&n", FALSE, ch, obj, ch, TO_CHAR);
      showhand(obj, ch, cmd, argument, 1); //show player hand
      showhand(obj, ch, cmd, argument, 2); //show DEALER hand
      game_on = BJ_POSTHIT;
      if( player_total>21 )
      {
        snprintf(buf, MAX_STRING_LENGTH, "&+yYou &+RBUSTED&+y with a total of %d. Sorry, maybe try again later?.\n", player_total);
        send_to_char(buf, ch);
        do_win(ch, bettype, betamt, 2);
        clear_hands(1);
        clear_hands(2);
        game_on = BJ_PREBID;
        player_total = dealer_total = dealercards = bettype = betamt = 0;
        act( STR_CARDS_BUST, FALSE, ch, obj, ch, TO_CHAR);
      }
      return TRUE;
    } // End say keyword 'hit'
    // Keyword showgame -> displays current state of game.
    else if( !strcmp(arg, "showgame") )
    {
      if( !say(ch,arg) )
      {
        act( STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      showhand(obj, ch, cmd, argument, 1);
      showhand(obj, ch, cmd, argument, 2);
      if( IS_TRUSTED(ch) )
      {
        snprintf(buf, MAX_STRING_LENGTH, "Game Status is: %d. \n", game_on);
        send_to_char(buf, ch);
      }
      return TRUE;
    } // End keyword 'showhand'
    // Imm only showhand.
    else if (!strcmp(arg, "showhand2"))
    {
      if( !IS_TRUSTED(ch) )
      {
        return FALSE;
      }
      if( !say(ch,arg) )
      {
        act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      showhand(obj, ch, cmd, argument, 2);
      return TRUE;
    } // End keyword 'showhand2'
    else if( !strcmp(arg, "showhand1") )
    {
      if( !IS_TRUSTED(ch) )
      {
        return FALSE;
      }
      if( !say(ch,arg) )
      {
        act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      showhand(obj, ch, cmd, argument, 1);
      return TRUE;
    }
  } // End cmd == CMD_SAY
    return FALSE;
} // End of the gellz magic-deck worn proc
// End GELLZ_ magic_deck

void clear_hands(char whoshand)
{
  // empty player hand
  if( whoshand == 1 )
  {
    for( int tmpcounter = 0;tmpcounter < 5;tmpcounter++ )
    {
      player_hand[tmpcounter].Suit="";
      player_hand[tmpcounter].Number = 0;
      player_hand[tmpcounter].Value = 0;
      player_hand[tmpcounter].StillIn = 0;
    }
  }
  // empty dealer hand
  else if( whoshand == 2 )
  {
    for( int tmpcounter = 0;tmpcounter < 5; tmpcounter++ )
    {
      dealer_hand[tmpcounter].Suit="";
      dealer_hand[tmpcounter].Number = 0;
      dealer_hand[tmpcounter].Value = 0;
      dealer_hand[tmpcounter].StillIn = 0;
    }
  }
  else
  {
    debug( "clear_hands: Bad hand to be cleared (%d).", whoshand );
  }
}

int needcard(char whoscard, P_char ch)
{ // start whichcard
   switch (whoscard)
   { //switch whoscard
      case 1:
         if (player_hand[0].Value == 0)
            {get_card(1,0);}
         else if (player_hand[1].Value == 0)
            {get_card(1,1);}
         else if (player_hand[2].Value == 0)
            {get_card(1,2);}
         else if (player_hand[3].Value == 0)
            {get_card(1,3);}
         else if (player_hand[4].Value == 0)
            {get_card(1,4);}
         else
            {send_to_char("Player has 5 cards already..", ch);}
         break;
      case 2:
         if (dealer_hand[0].Value == 0)
            {get_card(2,0);
	    dealercards=1;}
         else if (dealer_hand[1].Value == 0)
            {get_card(2,1);
	    dealercards=2;}
         else if (dealer_hand[2].Value == 0)
            {get_card(2,2);
	    dealercards=3;}
         else if (dealer_hand[3].Value == 0)
            {get_card(2,3);
	    dealercards=4;}
         else if (dealer_hand[4].Value == 0)
            {get_card(2,4);
	    dealercards=5;}
         else
            {send_to_char("Dealer has 5 cards already..", ch);
	     dealercards=5;}
         break;
    } //end Switch whoscard
  return 0;
} // end whichcard

int do_win(P_char ch, int bettype, int betamt, int winloose)
{
   if (winloose==1)
   { //WINNING Tasks
     send_to_char_f(ch, "\n&+yYour account is &+Mcredited &+W%d ", betamt);
     if (bettype==0) 
       {GET_COPPER(ch)+=betamt;
       send_to_char_f(ch, STR_COPP);}
     if (bettype==1) 
       {GET_SILVER(ch)+=betamt;
       send_to_char_f(ch, STR_SILV);}
     if (bettype==2) 
       {GET_GOLD(ch)+=betamt;
       send_to_char_f(ch, STR_GOLD);}
     if (bettype==3) 
       {GET_PLATINUM(ch)+=betamt;
       send_to_char_f(ch, STR_PLAT);}
     send_to_char_f(ch, "&+y.&n\n");
     logit(LOG_CARDGAMES, "%s won %d %s at blackjack.", J_NAME(ch), betamt,
      (bettype==0)?"copper":(bettype==1)?"silver":(bettype==2)?"gold":(bettype==3)?"platinum":"unknown");
   } else if (winloose==2)
   {
     logit(LOG_CARDGAMES, "%s lost %d %s at blackjack.", J_NAME(ch), betamt,
      (bettype==0)?"copper":(bettype==1)?"silver":(bettype==2)?"gold":(bettype==3)?"platinum":"unknown");
   // END TASKS FOR LOOSING 
   }
  return 0;
}

//start get_card - whoscard1=player, 2=dealer
int get_card(char whoscard, int whatcard)
{
   int tmpRandomCard;
   do { //start DO while loop
      tmpRandomCard = number(1, 52);
      switch (whoscard)
      {//switch whoscard
         case 1:
           player_hand[whatcard].Suit=deck[tmpRandomCard].Suit;
           player_hand[whatcard].Value=deck[tmpRandomCard].Value;
           player_hand[whatcard].Display=deck[tmpRandomCard].Display;
           player_hand[whatcard].Number=deck[tmpRandomCard].Number;
           break;
         case 2:
           dealer_hand[whatcard].Suit=deck[tmpRandomCard].Suit;
           dealer_hand[whatcard].Value=deck[tmpRandomCard].Value;
           dealer_hand[whatcard].Display=deck[tmpRandomCard].Display;
           dealer_hand[whatcard].Number=deck[tmpRandomCard].Number;
           break;
      }//switch whoscard
     } while (deck[tmpRandomCard].StillIn == 0); //end DO While loop
   deck[tmpRandomCard].StillIn = 0;
   if (whoscard==1)
      {player_total=player_total+deck[tmpRandomCard].Value;}
   if (whoscard==2)
      {dealer_total=dealer_total+deck[tmpRandomCard].Value;}
  return 0;
}//end get card

// Gellz Setup Deck
void setup_deck(void)
{
  int tmpcounter = 0;
  // Start For tmpsuit
  for( int tmpsuit=1; tmpsuit < 5; tmpsuit++ )
  {
    for( int tmpcard = 1; tmpcard<14; tmpcard++ )
    {
      switch(tmpsuit)
      {
        case 1:
          deck[tmpcounter].Suit=STR_HEARTS;
          break;
        case 2:
          deck[tmpcounter].Suit=STR_DIAMONDS;
          break;
        case 3:
          deck[tmpcounter].Suit=STR_CLUBS;
          break;
        case 4:
          deck[tmpcounter].Suit=STR_SPADES;
          break;
      }
      switch( tmpcard )
      {
        case 1:
        	deck[tmpcounter].Display=STR_CARD_1;
          break;
        case 2:
        	deck[tmpcounter].Display=STR_CARD_2;
          break;
        case 3:
        	deck[tmpcounter].Display=STR_CARD_3;
          break;
        case 4:
        	deck [tmpcounter].Display=STR_CARD_4;
		      break;
	      case 5:
        	deck [tmpcounter].Display=STR_CARD_5;
		      break;
	      case 6:
        	deck [tmpcounter].Display=STR_CARD_6;
		      break;
	      case 7:
        	deck [tmpcounter].Display=STR_CARD_7;
		      break;
	      case 8:
        	deck [tmpcounter].Display=STR_CARD_8;
		      break;
	      case 9:
        	deck [tmpcounter].Display=STR_CARD_9;
		      break;
	      case 10:
        	deck [tmpcounter].Display=STR_CARD_10;
		      break;
	      case 11:
        	deck [tmpcounter].Display=STR_CARD_J;
		      break;
	      case 12:
        	deck [tmpcounter].Display=STR_CARD_Q;
		      break;
	      case 13:
        	deck [tmpcounter].Display=STR_CARD_K;
		      break;
	    } //close Switch tmpCard
      deck[tmpcounter].Number=tmpcounter;
	    deck[tmpcounter].Value=(tmpcard>10) ? 10 : tmpcard;
      deck[tmpcounter].StillIn = TRUE;
      tmpcounter++;
    }//End FOR tmpcard
  }//End FOR tmpsuit
  player_total=0;
  dealer_total=0;
}// End gellz Setup Deck

// Just showing all cards in a hand.
int showhand(P_obj obj, P_char ch, int cmd, char *argument, int whoscard)
{
  char    buf [MAX_STRING_LENGTH];
  string  buftest;

  // for loop - this is all DEBUG stuff
  for( int tmpcounter = 0; tmpcounter < 5; tmpcounter++ )
  {
    if( whoscard == 1 )
    {
      if( !(player_hand[tmpcounter].Value == 0) )
      {
        snprintf(buf, MAX_STRING_LENGTH, "&+yYour card is %s&+y of %s&+y.", player_hand[tmpcounter].Display, player_hand[tmpcounter].Suit);
        act (buf, FALSE, ch,obj, ch, TO_CHAR);
      }
      else
      {
        // No need to continue after we hit a 0 card.
        break;
      }
    }
    else if( whoscard == 2 )
    {
      if( !(dealer_hand[tmpcounter].Number == 0))
      {
        snprintf(buf, MAX_STRING_LENGTH, "&+yDealer shows a %s&+y of %s&+y.", dealer_hand[tmpcounter].Display, dealer_hand[tmpcounter].Suit);
        act( buf, FALSE, ch,obj, ch, TO_CHAR);
      }
      else
      {
        // No need to continue after we hit a 0 card.
        break;
      }
    }
  }
	if( whoscard == 1 )
	{
    snprintf(buf, MAX_STRING_LENGTH, "&+mYour total is         &+Y-- &+M%d &+Y--&+y &+mand your bet is : &+Y%d %s.\n\n", player_total, betamt, strbettype);
    send_to_char(buf,ch);
  }
	else if( whoscard == 2 )
	{
    snprintf(buf, MAX_STRING_LENGTH, "&+mDealer is total is    &+Y-- &+r%d &+Y--&+y.\n\n", dealer_total);
    send_to_char(buf,ch);
  }
  return 0;
} //end showhands

//GELLZ Card Game Mains End
//*****************************************************************************

//*****************************************************************************
//                              GELLZ ACTUAL USED PROCS
//*****************************************************************************
/*	Deaths Door achievment proc for all 100 base stats - gellz	*/
void do_deaths_door(P_char ch, char *arg, int cmd)
{
  struct affected_type af;
  char buf[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }
  if( !(affected_by_spell(ch, ACH_DEATHSDOOR)) )
  {
    act("&+yYou do not posess the ability to use &+LDea&+wths &+LDo&+wor&+y portals...\n&n", FALSE, ch, 0, 0, TO_CHAR );
    // Don't show stats to lowbies.
    if( GET_LEVEL(ch) < MIN_LEVEL_FOR_ATTRIBUTES )
    {
      return;
    }
    snprintf(buf, MAX_STRING_LENGTH, "&+yYou still need&+W: " );
    if( ch->base_stats.Str < 100 )
    {
      snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "&+w%d &+LStr&+y, ", 100 - ch->base_stats.Str );
    }
    if( ch->base_stats.Dex < 100 )
    {
      snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "&+w%d &+LDex&+y, ", 100 - ch->base_stats.Dex );
    }
    if( ch->base_stats.Int < 100 )
    {
      snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "&+w%d &+LInt&+y, ", 100 - ch->base_stats.Int );
    }
    if( ch->base_stats.Wis < 100 )
    {
      snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "&+w%d &+LWis&+y, ", 100 - ch->base_stats.Wis );
    }
    if( ch->base_stats.Agi < 100 )
    {
      snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "&+w%d &+LAgi&+y, ", 100 - ch->base_stats.Agi );
    }
    if( ch->base_stats.Con < 100 )
    {
      snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "&+w%d &+LCon&+y, ", 100 - ch->base_stats.Con );
    }
    if( ch->base_stats.Pow < 100 )
    {
      snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "&+w%d &+LPow&+y, ", 100 - ch->base_stats.Pow );
    }
    if( ch->base_stats.Cha < 100 )
    {
      snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "&+w%d &+LCha&+y, ", 100 - ch->base_stats.Cha );
    }
    snprintf(buf + strlen(buf) - 2, MAX_STRING_LENGTH, "&+y.\n" );
    send_to_char(buf, ch );
    return;
  }
  if( !*arg )
  {
    act("Pardon? Im not sure what you are trying to do.  Maybe open a &+Wdoor&n?\n", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if( !strcmp(arg, "door") )
  {
    if( affected_by_spell(ch, TAG_DEATHSDOOR) )
    {
      act("&+yYour &+LDea&+wths &+LDo&+wor&+y fails to open. You need to &+Lwa&+wit &+ylonger...", FALSE, ch, 0, 0, TO_CHAR);
      return;
    } //end timer delay
    act("&+yYou draw a &+gmys&+Gti&+gcal &+Gpe&+gnt&+Gag&+gram&+y on the ground, and a &+LDea&+wths &+LDo&+wor&+y portal appears.\n&n", FALSE, ch, 0, 0, TO_CHAR );
// RESET timer delay here
    memset(&af, 0, sizeof(struct affected_type));
    af.type = TAG_DEATHSDOOR;
    af.modifier = 0;
    af.duration = 15;
    af.location = 0;
    af.flags = AFFTYPE_NODISPEL;
    affect_to_char(ch, &af);
// END timer delay
    spell_corpse_portal(60, ch, arg, 0, ch, 0);

    CharWait(ch, 5*WAIT_SEC);
    return;
  }
  else
  {
    act("Pardon? Im not sure what you are trying to do.  Maybe open a &+Wdoor&n?\n", FALSE, ch, 0, 0, TO_CHAR);
    return;
  } //End ELSE - didnt say DOOR 
} //End Deaths DOOR proc

//*****************************************************************
