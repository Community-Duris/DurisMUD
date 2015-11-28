/*****************************************************************************
 * file: cardgames.c,                                          part of Duris *
 * Usage: This contains code for varoius cardgames within Duris              *
 * Inception: 5/21/2015                                                      *
 *****************************************************************************/

#include "cardgames.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "utils.h"
#include "vnum.obj.h"
#include <stdio.h>
#include <string.h>

// Card functions!
const char *Card::getSuit()
{
  static const char Suit[4][14] = {"&+LClubs&n", "&+rDiamonds&n", "&+rHearts&n", "&+LSpades&n"};
  return Suit[(value-1)/13];
}

const char *Card::getValue()
{
  static const char Number[13][3] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
  return Number[(value-1)%13];
}

char *Card::Display()
{
  static char buf[25];

  sprintf( buf, "&+C%s&n %s", getValue(), getSuit() );

  return buf;
}

// Display cards with spacing to make even columns
char *Card::Display2()
{
  static char buf[25];

  sprintf( buf, "&+C%2s&n %-14s", getValue(), getSuit() );

  return buf;
}

// Hand functions!
char *Hand::Display()
{
  static char buf[MAX_STRING_LENGTH];
  P_Card temp = cards;
  buf[0] = '\0';

  while( temp )
  {
    strcat( buf, temp->Display() );
    if( temp->nextCard )
    {
      strcat( buf, "  " );
    }
    temp = temp->nextCard;
  }

  return buf;
}

int Hand::BlackjackValue()
{
  P_Card temp = cards;
  int aces = 0, curr, sum = 0;
  while( temp )
  {
    curr = (temp->getNumber()-1)%13+1;
    // If ace...
    if( curr == 1 )
    {
      aces++;
    }
    // If number card...
    else if( curr < 11 )
    {
      sum += curr;
    }
    // If face card...
    else
    {
      sum += 10;
    }
    temp = temp->nextCard;
  }
  // This creative bit is to see if an Ace counts as 11.
  //   Note: Only one ace will count as 11 otherwise bust.
  if( sum + aces < 12 && aces > 0 )
  {
    sum += 10;
  }
  sum += aces;

  return sum;
}
// Deck functions!

// Create the deck
Deck::Deck()
{
  P_Card temp;
  cards = NULL;
  numCards = 0;

  // Values for the cards are 1 .. 52.
  for( int i = 52; i >= 1; i-- )
  {
    temp = new Card(i, cards);
    cards = temp;
    numCards++;
  }
}

void Deck::Shuffle( int numShuffles )
{
  int count;
  // Left and right hands to shuffle.
  P_Card left, right, temp;

  if( numCards < 2 )
  {
    debug( "Trying to shuffle a deck with (%d) less than 2 cards.  Absurd.", numCards );
    return;
  }

  while( numShuffles-- > 0 )
  {
    count = numCards / 2;

    left = cards;
    // Cut Deck in half
    while( count-- > 1 )
    {
      left = left->nextCard;
    }
    // Left now points to the card before the split.
    right = left->nextCard;
    // Actual cut.
    left->nextCard = NULL;
    // Left now points to the top of the left hand set of cards.
    left = cards;
    cards = NULL;

    // Shuffle it back together (we actually do this upside down, but *shrug*).
    for(count = 0; count < numCards; count++ )
    {
      // Percentage chance to pull from the left hand.
      if( get_property("cardgames.shuffle.percentage", 50) >= number(1, 100) )
      {
        // Pull from left hand if there is a card.
        if( left != NULL )
        {
          // Remove top card.
          temp = left;
          left = left->nextCard;
          // Place it on top of deck.
          temp->nextCard = cards;
          cards = temp;
        }
        else
        {
          // Remove top card.
          temp = right;
          right = right->nextCard;
          // Place it on top of deck.
          temp->nextCard = cards;
          cards = temp;
        }
      }
      else
      {
        if( right != NULL )
        {
          // Remove top card.
          temp = right;
          right = right->nextCard;
          // Place it on top of deck.
          temp->nextCard = cards;
          cards = temp;
        }
        else
        {
          // Remove top card.
          temp = left;
          left = left->nextCard;
          // Place it on top of deck.
          temp->nextCard = cards;
          cards = temp;
        }
      }
    }
    if( count != numCards || left != NULL || right != NULL )
    {
      debug( "Shuffle: We have a problem: count %d, numCards %d, left %d, right %d",
        count, numCards, left, right );
    }
  }
}

P_Card Deck::DealACard()
{
  P_Card retCard = cards;

  if( cards == NULL )
  {
    return NULL;
  }

  cards = cards->nextCard;
  retCard->nextCard = NULL;
  return retCard;
}

char *Deck::Display()
{
  static char buf[MAX_STRING_LENGTH];
  P_Card temp = cards;
  buf[0] = '\0';

  while( temp )
  {
    strcat( buf, temp->Display() );
    strcat( buf, "  " );
    temp = temp->nextCard;
  }

  return buf;
}

char *Deck::Display2()
{
  static char buf[MAX_STRING_LENGTH];
  int count = 0;
  P_Card temp = cards;
  buf[0] = '\0';

  while( temp )
  {
    strcat( buf, temp->Display2() );
    strcat( buf, " | " );
    temp = temp->nextCard;
    if( (++count)%5 == 0 )
    {
      strcat( buf, "\n\r" );
    }
  }

  return buf;
}

int cards_object( P_obj obj, P_char ch, int cmd, char *argument )
{
  char arg[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  static P_Deck theDeck = new Deck();
  int num;

  if( cmd == CMD_SET_PERIODIC || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_SAY )
  {
    argument = one_argument(argument, arg);
    if( !strcmp(arg, "create") )
    {
      if( theDeck != NULL )
      {
        send_to_char( "Deleting old deck...\n\r", ch );
        delete theDeck;
      }
      theDeck = new Deck();
    }
    else if( !strcmp(arg, "shuffle") )
    {
      argument = one_argument(argument, arg);
      num = atoi( arg );
      // Shuffle at least one time.
      num = (num<1) ? 1 : num;
      while( num-- > 0 )
      {
        send_to_char( "Shuffling...  ", ch );
        theDeck->Shuffle( 1 );
      }
      send_to_char( "\n\r", ch );
    }
    else if( !strcmp(arg, "shuffle2") )
    {
      argument = one_argument(argument, arg);
      num = atoi( arg );
      // Shuffle at least one time.
      num = (num<1) ? 1 : num;
      sprintf( buf, "Shuffling %d times....   ", num );
      send_to_char( buf, ch );
      theDeck->Shuffle( num );
      send_to_char( "done.\n\r", ch );
    }
    else if( !strcmp( arg, "display" ) )
    {
      send_to_char( "Your Deck:\n\r", ch );
      send_to_char( theDeck->Display(), ch );
      send_to_char( ".\n\r", ch );
    }
    else if( !strcmp( arg, "display2" ) )
    {
      send_to_char( "Your Deck:\n\r", ch );
      send_to_char( theDeck->Display2(), ch );
      send_to_char( ".\n\r", ch );
    }
  }

  return FALSE;
}

void event_dealersturn( P_char ch, P_char victim, P_obj obj, void *data );

// *****************************************************************************
// *****************************************************************************
//                              GELLZ BLACKJACK
// *****************************************************************************
// Values on table object: [0] == Game status: BJ_PREBID, BJ_POSTBID, BJ_POSTDEAL, BJ_POSTHIT, BJ_DEALERSTURN
// Timers on table object: [0] == theDeck, [1] == dealerHand, [2] == playerHand (possible more players?)
int blackjack_table(P_obj obj, P_char ch, int cmd, char *argument)
{
  P_Deck theDeck;
  P_Hand playerHand, dealerHand;
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int betamt, bettype;
  static bool lock_game = FALSE;

  if( cmd == CMD_SET_PERIODIC )
  {
    // Initialize the table here.
    if( obj )
    {
      obj->value[0] = BJ_PREBID;
      obj->timer[0] = obj->timer[1] = obj->timer[2] = obj->timer[3] = obj->timer[4] = obj->timer[5] = 0;
    }
    return FALSE;
  }

  if( !obj )
  {
    debug( "blackjack_table: No f'n table?!?" );
    return FALSE;
  }

  theDeck = (P_Deck) obj->timer[0];
  dealerHand = (P_Hand) obj->timer[1];
  playerHand = (P_Hand) obj->timer[2];

  // Aaah.. the suspense! ;)
  if( cmd == CMD_PERIODIC && obj->value[0] == BJ_DEALERSTURN )
  {
    ch = playerHand->getOwner();

    if( dealerHand->BlackjackValue() < 17 && dealerHand->numCards() < 5 )
    {
// This won't work yet, 'cause we have to save a pointer to ch somewhere and reference it
//   since ch is NULL when CMD_PERIODIC fires.
      act( "\n&+yThe &+CDealer&+y takes a new card...&n", FALSE, ch, obj, ch, TO_CHAR );
      dealerHand->ReceiveCard(theDeck->DealACard());
      sprintf(buf, "&+yDealer shows: %s.\n\r&+yDealer Total: &+Y%d&+y.&n\n\r", dealerHand->Display(), dealerHand->BlackjackValue() );
      send_to_char( buf, ch );
    }
    // End game
    else
    {
      if( dealerHand->BlackjackValue() < 22 )
      {
        act( "\n&+yThe &+CDealer&+y decides to &+Wstay&+y with his current hand!&n\n\n", FALSE, ch, obj, ch, TO_CHAR);
        if( playerHand->BlackjackValue() > dealerHand->BlackjackValue() )
        {
          sprintf( buf, "&+RY&+CO&+BU &+GW&+YI&+MN&+C!&+R!&+y! with &+Y%d&+y versus the dealers &+Y%d&+y.\n",
            playerHand->BlackjackValue(), dealerHand->BlackjackValue() );
          send_to_char(buf, ch);
          // Return the bid + winnings.
          ch->points.cash[obj->value[2]] += 2*obj->value[1];
          logit(LOG_CARDGAMES, "%s won %d %s at blackjack v2.0.", J_NAME(ch), obj->value[1], (obj->value[2]==0)?"copper":
            (obj->value[2]==1)?"silver":(obj->value[2]==2)?"gold":(obj->value[2]==3)?"platinum":"unknown");
        }
        else if( playerHand->BlackjackValue() == dealerHand->BlackjackValue() )
        {
          act("&+yA &+YPUSH!&+y No winner no loser! You pull your &+Wbet&+y from the table.", FALSE, ch, obj, ch, TO_CHAR);
          // Return the bid.
          ch->points.cash[obj->value[2]] += obj->value[1];
          logit(LOG_CARDGAMES, "%s had a push in blackjack v2.0.", J_NAME(ch) );
        }
        // Can assume player total < dealer total.
        else
        {
          sprintf(buf, "&+RYou LOSE!!&+C Dealers &+Y%d &+rbeats your &+Y%d&+r.\n",
            dealerHand->BlackjackValue(), playerHand->BlackjackValue() );
          logit(LOG_CARDGAMES, "%s lost %d %s at blackjack v2.0.", J_NAME(ch), obj->value[1], (obj->value[2]==0)?"copper":
            (obj->value[2]==1)?"silver":(obj->value[2]==2)?"gold":(obj->value[2]==3)?"platinum":"unknown");
          send_to_char(buf, ch);
        }
      }
      else
      {
        send_to_char("&+CDealer&+R BUST&+y, so &+RY&+CO&+BU &+GW&+YI&+MN&+C!&+R!&+y&n\n", ch);
        // Return the bid + winnings.
        logit(LOG_CARDGAMES, "%s won %d %s at blackjack v2.0.", J_NAME(ch), obj->value[1], (obj->value[2]==0)?"copper":
          (obj->value[2]==1)?"silver":(obj->value[2]==2)?"gold":(obj->value[2]==3)?"platinum":"unknown");
        ch->points.cash[obj->value[2]] += 2*obj->value[1];
      }
      // Reset the table:
      goto reset_table;
    }
    return TRUE;
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
    if( obj->value[0] != BJ_PREBID )
    {
      act ("&+yA &+Wgame&+y already appears to be in progress. Please &+Wfold&+y or complete that one first...", FALSE, ch, obj, obj, TO_CHAR);
      return TRUE;
    }

    argument = one_argument(argument, arg);
    one_argument(argument, arg2);
    // We only need to check the second argument, since if it exists, the first does also.
    if( !arg2 )
    {
      act( STR_CARDS_ARG_FAIL, FALSE, ch, NULL, NULL, TO_CHAR );
      return TRUE;
    }
    betamt = atoi(arg);
    bettype = coin_type(arg2);

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
        obj->value[0] = BJ_POSTBID;
      }
    }
    else if( bettype == 1 )
    {
      if( betamt > GET_SILVER(ch) )
      {
        act( STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);
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
        obj->value[0] = BJ_POSTBID;
      }
    }
    else if( bettype == 2 )
    {
      if( betamt > GET_GOLD(ch) )
      {
        act( STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);
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
        obj->value[0] = BJ_POSTBID;
      }
    }
    else if( bettype == 3 )
    {
      if( betamt > GET_PLATINUM(ch) )
      {
        act( STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);
      }
      else if( betamt > get_property("blackjack.MaxBetInPlatinum", 100) )
      {
        act(STR_CARDS_BIGBID_FAIL, FALSE, ch, obj, obj, TO_CHAR);
        return TRUE;
      }
	    else
      {
        act( STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
        obj->value[0] = BJ_POSTBID;
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
      debug( "blackjack_table: BROKEN COIN TYPE %d not between 0 and 3.", bettype );
      send_to_char_f(ch, "&=BRWTFs&n");
    }
    send_to_char_f(ch, "&+y on the table.&n\n");
    obj->value[1] = betamt;
    obj->value[2] = bettype;
    return TRUE;
  } // End cmd == CMD_OFFER

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
      if( obj->value[0] != BJ_POSTBID )
      {
        // If they haven't bid yet.
        if( obj->value[0] == BJ_PREBID )
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
        theDeck = new Deck;
        obj->timer[0] = (long) theDeck;
        dealerHand = new Hand;
        obj->timer[1] = (long) dealerHand;
        playerHand = new Hand(ch);
        obj->timer[2] = (long) playerHand;
        act( STR_CARDS_SHUFFLE, FALSE, ch, obj, ch, TO_CHAR);
        // Shuffle the deck 7 times...
        theDeck->Shuffle( 7 );
        act( STR_CARDS_DEAL, FALSE, ch, obj, ch, TO_CHAR);

        // The dealer's hidden card isn't dealt until the end.
        dealerHand->ReceiveCard(theDeck->DealACard());
        playerHand->ReceiveCard(theDeck->DealACard());
        playerHand->ReceiveCard(theDeck->DealACard());

        sprintf(buf, "&+yDealer shows: %s.\n\r&+yDealer Total: &+Y%d&+y.&n\n\r", dealerHand->Display(), dealerHand->BlackjackValue() );
        send_to_char( buf, ch );
        sprintf(buf, "&+yPlayer shows: %s.\n\r&+yPlayer Total: &+Y%d&+y.&n\n\r", playerHand->Display(), playerHand->BlackjackValue() );
        send_to_char( buf, ch );

        sprintf(buf, "&+yThe &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y flicker over the &+wdeck&+y once more, and you realise that you can choose to either &+Whit&+y, &+Wstay&+y or &+Wfold&+y simply by telling the cards what you wish to do.&n\n");
        act( buf, FALSE, ch, obj, ch, TO_CHAR);

        obj->value[0] = BJ_POSTDEAL;
        return TRUE;
      }
    } // End say keyword 'deal'
    // Start Keyword for 'stay' - sitting
    else if (!strcmp(arg, "stay"))
    {
      if( obj->value[0] != BJ_POSTDEAL && obj->value[0] != BJ_POSTHIT )
      {
        act( STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR);
        return TRUE;
      }
      if( !say(ch,arg) )
      {
        act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      // Show player's hand.
      sprintf(buf, "&+yPlayer shows: %s.\n\r&+yPlayer Total: &+Y%d&+y.&n\n\r", playerHand->Display(), playerHand->BlackjackValue() );
      send_to_char( buf, ch );

      // Dealer's turn to draw.
      send_to_char("&+yThe &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y fly over the &+wdeck&+y as the &+CDealer&+y begins his &+Yturn!&n.\n", ch);
      act( "\n&+yThe &+CDealer&+y turns over the other card...&n", FALSE, ch, obj, ch, TO_CHAR );
      dealerHand->ReceiveCard(theDeck->DealACard());
      sprintf(buf, "&+yDealer shows: %s.\n\r&+yDealer Total: &+Y%d&+y.&n\n\r", dealerHand->Display(), dealerHand->BlackjackValue() );
      send_to_char( buf, ch );

      obj->value[0] = BJ_DEALERSTURN;
      add_event(event_dealersturn, 2, 0, 0, obj, 0, 0, 0);
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
      if( obj->value[0] != BJ_POSTDEAL && obj->value[0] != BJ_POSTHIT )
      {
        act( STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR); // Was game in progress.
        return TRUE;
      }
      else
      {
        act( STR_CARDS_FOLD, FALSE, ch, obj, ch, TO_CHAR);
      }
      // Reset the table:
      goto reset_table;
    } // End say keyword for 'fold'
    // Start Keyword for 'hit' - add another card to player's hand.
    else if( !strcmp(arg, "hit") )
    {
      if( !say(ch,arg) )
      {
        act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);
      }
      // If we're not in a position to hit
      if( obj->value[0] != BJ_POSTDEAL && obj->value[0] != BJ_POSTHIT )
      {
        act( STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR);
        return TRUE;
      }

      act("&+yA card leaves the &+wdeck&+y and &+Yreveals&+y itself.&n", FALSE, ch, obj, ch, TO_CHAR);
      playerHand->ReceiveCard(theDeck->DealACard());

      sprintf(buf, "&+yDealer shows: %s.\n\r&+yDealer Total: &+Y%d&+y.&n\n\r", dealerHand->Display(), dealerHand->BlackjackValue() );
      send_to_char( buf, ch );
      sprintf(buf, "&+yPlayer shows: %s.\n\r&+yPlayer Total: &+Y%d&+y.&n\n\r", playerHand->Display(), playerHand->BlackjackValue() );
      send_to_char( buf, ch );
      obj->value[0] = BJ_POSTHIT;

      if( playerHand->BlackjackValue() > 21 )
      {
        sprintf(buf, "&+yYou &+RBUSTED&+y with a total of %d. Sorry, maybe try again later?.\n", playerHand->BlackjackValue() );
        send_to_char(buf, ch);
        act( STR_CARDS_BUST, FALSE, ch, obj, ch, TO_CHAR);
        logit(LOG_CARDGAMES, "%s lost %d %s at blackjack v2.0.", J_NAME(ch), obj->value[1], (obj->value[2]==0)?"copper":
          (obj->value[2]==1)?"silver":(obj->value[2]==2)?"gold":(obj->value[2]==3)?"platinum":"unknown");

        // Reset the table:
        goto reset_table;
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

      if( IS_TRUSTED(ch) )
      {
        sprintf(buf, "Game Status is: %d. \n", obj->value[0]);
        send_to_char(buf, ch);
      }
      // If we're pre-bid, then no sense in showing it (to mortals).
      if( IS_TRUSTED(ch) || obj->value[0] != BJ_PREBID )
      {
        sprintf( buf, "The current bid is: %d %s.\n\r", obj->value[1],
          (obj->value[2]==0) ? STR_COPP : (obj->value[2]==1) ? STR_SILV :
          (obj->value[2]==2) ? STR_GOLD : (obj->value[2]==3) ? STR_PLAT : "&=BRWTFs&n" );
        send_to_char( buf, ch );
      }
      if( obj->value[0] ==  BJ_PREBID )
      {
        send_to_char( "&+yThe &+Ygame&+y has yet to begin.  Make an &+Woffer&+y.&n\n\r", ch );
        return TRUE;
      }
      if( obj->value[0] ==  BJ_POSTBID )
      {
        send_to_char( "&+yThe &+Ybets&+y have been made. Time to &+Wdeal&+y.&n\n\r", ch );
        return TRUE;
      }
      if( obj->value[0] ==  BJ_POSTDEAL || obj->value[0] == BJ_POSTHIT )
      {
        send_to_char( "&+yThe &+Ygame&+y is on!&n\n\r", ch );
        sprintf(buf, "&+yDealer shows: %s.\n\r&+yDealer Total: &+Y%d&+y.&n\n\r", dealerHand->Display(), dealerHand->BlackjackValue() );
        send_to_char( buf, ch );
        sprintf(buf, "&+yPlayer shows: %s.\n\r&+yPlayer Total: &+Y%d&+y.&n\n\r", playerHand->Display(), playerHand->BlackjackValue() );
        send_to_char( buf, ch );
        return TRUE;
      }
      if( obj->value[0] ==  BJ_DEALERSTURN )
      {
        send_to_char( "&+yThe &+Ydealer&+y is thinking...&n\n\r", ch );
        sprintf(buf, "&+yDealer shows: %s.\n\r&+yDealer Total: &+Y%d&+y.&n\n\r", dealerHand->Display(), dealerHand->BlackjackValue() );
        send_to_char( buf, ch );
        sprintf(buf, "&+yPlayer shows: %s.\n\r&+yPlayer Total: &+Y%d&+y.&n\n\r", playerHand->Display(), playerHand->BlackjackValue() );
        send_to_char( buf, ch );
        return TRUE;
      }
      return TRUE;
    } // End keyword 'showhand'
  } // End cmd == CMD_SAY
    return FALSE;

reset_table:
  delete theDeck;
  delete dealerHand;
  delete playerHand;
  obj->value[0] = BJ_PREBID;
  obj->value[1] = obj->value[2] = 0;
  obj->timer[0] = obj->timer[1] = obj->timer[2] = obj->timer[3] = obj->timer[4] = obj->timer[5] = 0;
  return TRUE;
} // End GELLZ Blackjack Table

// All this does is call the object proc.
void event_dealersturn( P_char ch, P_char victim, P_obj obj, void *data )
{
  // If the object procs...
  if( blackjack_table( obj, ch, CMD_PERIODIC, NULL ) )
  {
    // If we need to proc again.
    if( obj->value[0] == BJ_DEALERSTURN )
    {
      add_event(event_dealersturn, 2, 0, 0, obj, 0, 0, 0);
    }
  }
}
// GELLZ Blackjack End
