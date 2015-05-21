
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

//*****************************************************************
//	Gellz TEST Object procedure... ONLY for testing
//*****************************************************************
int gellz_test_obj_procs(P_obj obj, P_char ch, int cmd, char *argument)
{ //PLACEHOLDER ONLY
   char         *arg;
   int          curr_time;
   
   if (cmd == CMD_SET_PERIODIC) //Events have priority..
      return FALSE;
   if (!(cmd == CMD_SAY))
      return FALSE;
   if (argument && (cmd == CMD_SAY))
   { // Start KEYWORDS
     arg = argument;
     while (*arg == ' ') //While argument is not black
        arg++;
//*****************************************************************
// 		TESTBIT
//*****************************************************************
   char		argstring1[MAX_STRING_LENGTH];
   char		argstring2[MAX_STRING_LENGTH];
   char		argstring3[MAX_STRING_LENGTH];
     argument = one_argument(argument, argstring1); //get one argument from list
     argument = one_argument(argument, argstring2); //get one argument from list
     argument = one_argument(argument, argstring3); //get one argument from list
  P_ship    ship;
  char		buf[200];
  string	argstring;

	ShipVisitor svs;
  if (!(IS_TRUSTED(ch)))
     return FALSE; //if NOT immortal saying it..
      if (!strcmp(argstring1, "ship"))
      {
	 if (!strcmp(argstring2, "all"))
	 {
	    act ("&+yListing &+YALL ships &+yin game:&n", FALSE, ch, obj, obj, TO_CHAR);
	    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	    { //LOOP through all ships
                send_to_char_f(ch, "&+yShip named:&+C %s&+y is Owned by: &+C%s ", SHIP_NAME(svs), SHIP_OWNER(svs));
                send_to_char_f(ch, "&+yin Room: %s ", world[svs->location].name);
		if SHIP_DOCKED(svs)
			{send_to_char_f(ch, "&+y | &+LDOCKED&+y");}
		if SHIP_ANCHORED(svs)
			{send_to_char_f(ch, "&+y | &+YANCHORED&+y");}
		if SHIP_IMMOBILE(svs)
			{send_to_char_f(ch, "&+y | &+rIMMOBILE&+y");}
		if SHIP_SINKING(svs)
			{send_to_char_f(ch, "&+y | &+RSINKING&+y");}
                send_to_char_f(ch, "\n");
	    } // END loop for all ships.
	    return TRUE;
	 } //End say keyword ALL
	 else if (!strcmp(argstring2, "rename"))
	 {
	    act ("&+YWILL BE RENAMING SHIP HERE - if we find the svs", FALSE, ch, obj, obj, TO_CHAR);
	    // rename_ship_owner(oldname,newname);
	 } //end RENAME keyword
	 else if (!strcmp(argstring2, "delete"))
	 {
	    act ("&+BWILL BE DELETING SHIP HERE - once we find the svs", FALSE, ch, obj, obj, TO_CHAR);
	    // for all ships, if ship = xxx, then delete_ship(svs);
	 } //end delete keyword
	 else {
	    return FALSE;} //didnt say a Keyword after ship..
      } //end IF SAY SHIP
      else {
         return FALSE;} //DIdnt say SHIP...
   } // End SAY Command
} //End 
//*****************************************************************
// 		END GELLZ TESTBIT
//*****************************************************************

//*****************************************************************************
//*****************************************************************************
//                              GELLZ CARD GAME
//*****************************************************************************
int magic_deck(P_obj obj, P_char ch, int cmd, char *argument)
{ //Start Gellz_keyword_invoke
   char         *arg;
   int          curr_time;
   char         buf [MAX_STRING_LENGTH];
   char		betbuf2[MAX_STRING_LENGTH];
   char		betbuf1[MAX_STRING_LENGTH];

   if (cmd == CMD_SET_PERIODIC) //Events have priority?
      return FALSE;
// BETTING START
   if (cmd == CMD_OFFER)
   { // this is LATEST addition
//      if (game_on==0) {}
   if (!(betamt == 0))
   {act ("&+yA &+Wgame&+y already appears to be in progress. Please &+Wfold&+y, or complete that one first...", FALSE, ch, obj, obj, TO_CHAR);
   return TRUE;
   }
     argument = one_argument(argument, betbuf1); //get one argument from list
     argument = one_argument(argument, betbuf2); //get SECOND argument from list
     if (!betbuf1 || !betbuf2)
        return FALSE;
     betamt = atoi(betbuf1);
     bettype = coin_type(betbuf2);
     
     if (bettype==0) 
       {if (betamt > GET_COPPER(ch))
	{act (STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);}
	else
	{act (STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
	game_on = 1;
	 strbettype=STR_COPP;}}
     if (bettype==1) 
       {if (betamt > GET_SILVER(ch))
	{act (STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);}
	else
	{act (STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
	game_on = 1;
	strbettype=STR_SILV;}}
     if (bettype==2) 
       {if (betamt > GET_GOLD(ch))
	{act (STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);}
	else
	{act (STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
	game_on = 1;
	strbettype=STR_GOLD;}}
     if (bettype==3) 
       {if (betamt > GET_PLATINUM(ch))
	{act (STR_CARDS_CASH_FAIL, FALSE, ch, obj, obj, TO_CHAR);}
	else
	{act (STR_CARDS_CASH_OK, FALSE, ch, obj, obj, TO_CHAR);
	game_on = 1;
	strbettype=STR_PLAT;}}
    
     if (!(betamt>0) || (bettype<0))
	{act (STR_CARDS_TYPE_FAIL, FALSE, ch, obj, obj, TO_CHAR);
	game_on = 0;
	betamt = 0;
	bettype = 0;
	return TRUE;}
     if (!betbuf1 || !betbuf2)
        return FALSE;
     return TRUE;
   } //END latest adition
// BETTING END

   if (argument && (cmd == CMD_SAY)) 
   { // Start KEYWORDS
     arg = argument;
     while (*arg == ' ') //While argument is not blank
        arg++;
//*****************************************************************
//              THIS IS THE MAIN CALLS - Start
//*****************************************************************
/*      if (!strcmp(arg, "bet")) //CHANGE STRINGS FOR NEED TO BET ***********
      {                         // Start Keyword for 'fold' - player folds
         if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
         //Start Actual ACTIONS
         if (!(game_on == 2))
         {
            act (STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR); // Was game in progress.
         }// else // end game was already on.
       } //end IF bet 
*/
//*****************************************************************
/*      if (!strcmp(arg, "gametime"))
      {                    // Start Keyword for 'gametime' - prepare the game
	if ((!(betamt>0)) || (bettype<0))
	{act ("&+yYou must &+Woffer&+y a bet prior to attempting to play a game. Try and &+Wshowgame&+y to see if you are ready.", FALSE, ch, obj, obj, TO_CHAR);
	return FALSE;
	}
         if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
         //Start Actual ACTIONS
         if (game_on == 1)
         {
            act (STR_CARDS_GAME_ON, FALSE, ch, obj, obj, TO_CHAR); // Was game in progress.
         } else // end game was already on.
         { //game wasnt on already
         setup_deck();
         act (STR_CARDS_SHUFFLE, FALSE, ch, obj, ch, TO_CHAR);
         clear_hands(1);
         clear_hands(2);
         game_on = 1;
        //END of the actions
         return TRUE;
         } //end ELSE game was off.
      }                         // Endof SAY keyword for 'gametime'
*/
//*****************************************************************
//*****************************************************************
      if (!strcmp(arg, "deal"))
      {                         // Start Keyword for 'deal' - deal initial cards
         if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
         if (!(game_on == 1))
         {
            act (STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR); // Was game in progress.
         } else // end game was already on.
         { // Can deal and play game
            act (STR_CARDS_SHUFFLE, FALSE, ch, obj, ch, TO_CHAR);
	    setup_deck();
            clear_hands(1);
            clear_hands(2);
	    act (STR_CARDS_DEAL, FALSE, ch, obj, ch, TO_CHAR);
            needcard(1,ch); //player 1st card
            needcard(2,ch); //dealer 1st card
            needcard(1,ch); //player 2nd card
            showhand(obj, ch, cmd, arg, 1); //show player hand
            showhand(obj, ch, cmd, arg, 2); //show dealer hand 
         //   needcard(2,ch); //dealer 2nd card (HOW DO WE HIDE IT?)
            sprintf(buf, "&+yThe &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y flicker over the &+wdeck&+y once more, and you realise that you can choose to either &+Whit&+y, &+Wstay&+y or &+Wfold&+y simply by telling the cards what you wish to do.&n\n");
            act (buf, FALSE, ch, obj, ch, TO_CHAR);
            game_on = 2;
            return TRUE;
         //END of the actions
         } //END game was on
      }                         // End say keyword for 'deal'
//*****************************************************************
      if (!strcmp(arg, "stay"))
      {                         // Start Keyword for 'stay' - sitting
         if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
	 showhand(obj, ch, cmd, argument, 1); //show PLAYER hand
         if ((!(game_on == 2)) && (!(game_on ==3)))
         {
            act (STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR); // Was game in progress.
         } else // end game was already on.
         {
send_to_char("&+yThe &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y fly over the &+wdeck&+y as the &+CDealer&+y sorts his cards and begins his &+Yturn!&n.\n", ch);
	 do {  //Repeat dealer get card while dealer < 21 (22)
               act ("\n&+yThe &+CDealer&+y takes a new card...&n\n", FALSE, ch, obj, ch, TO_CHAR);
               needcard(2,ch); //dealer 1st card
	    } while((dealer_total<17) && (dealercards<5));
         if (dealer_total<22);
	   {showhand(obj, ch, cmd, argument, 2); //show DEALER hand
            act ("\n&+yThe &+CDealer&+y decides to &+Wstay&+y with his current hand!&n\n\n", FALSE, ch, obj, ch, TO_CHAR);}
        } //end ELSE game was ON.
	if (dealer_total > 21)
	   {sprintf(buf, "&+CDealer&+R BUST&+y, so &+RY&+CO&+BU &+GW&+YI&+MN&+C!&+R!&+y&n\n");
	    send_to_char(buf, ch);
	    do_win(ch, bettype, betamt, 1);
	}
	else if (player_total > dealer_total)
   	   {do_win(ch, bettype, betamt, 1);
	    sprintf(buf, "&+RY&+CO&+BU &+GW&+YI&+MN&+C!&+R!&+y! with %d versus the dealers %d.\n", player_total, dealer_total);
	    send_to_char(buf, ch);
	}
   	else if ((player_total == dealer_total) && (!(player_total==0)))
	   {act("&+yA &+YPUSH!&+y No winner no loser! Your &+Wbet&+y has been refunded.", FALSE, ch, obj, ch, TO_CHAR);}
        else if (dealer_total > player_total)
	   {sprintf(buf, "&+RYou LOOSE!!&+C Dealers %d &+rbeats your %d.\n", dealer_total, player_total);
	    send_to_char(buf, ch);
	    do_win(ch, bettype, betamt, 2);
	 } //End options -Reset to Defaults.
         clear_hands(1);
         clear_hands(2);
         game_on = 0;
	 player_total = 0;
	 dealer_total = 0;
	 bettype=0;
	 betamt=0;
	 return TRUE;
      }                         // End say keyword for 'stay'
//*****************************************************************
      if (!strcmp(arg, "fold"))
      {                         // Start Keyword for 'fold' - player folds
         if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
	 showhand(obj, ch, cmd, argument, 1); //show PLAYER hand
         if ((!(game_on == 2)) && (!(game_on ==3)))
         {
            act (STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR); // Was game in progress.
         } else // end Can fold user
         {
           act (STR_CARDS_FOLD, FALSE, ch, obj, ch, TO_CHAR);
	 }
	 do_win(ch, bettype, betamt, 2);
         clear_hands(1);
         clear_hands(2);
         game_on=0;
	 player_total=0;
	 dealer_total=0;
	 bettype=0;
	 betamt=0;
         return TRUE;
      }                         // End say keyword for 'fold'
//*****************************************************************
//*****************************************************************
      if (!strcmp(arg, "hit"))
      {                         // Start Keyword for 'xxx'
         if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
         if ((!(game_on == 2)) && (!(game_on ==3)))
         {
            act (STR_CARDS_GAME_0, FALSE, ch, obj, ch, TO_CHAR); // Was game in progress.
	    return TRUE;
         } else // Can HIT player
         {
         needcard(1, ch);
         act("&+yIn a card leaves the &+wdeck&+y and &+Yreveals&+y itself.&n", FALSE, ch, obj, ch, TO_CHAR);
	 dealer_hand[5].Value =0; // SOMEHOW somehow this is set wrong!
         showhand(obj, ch, cmd, argument, 1); //show player hand
         showhand(obj, ch, cmd, argument, 2); //show DEALER hand
         game_on = 3;
	 if (player_total>21)
	 {
	    sprintf(buf, "&+yYou &+RBUSTED&+y with a total of %d. Sorry, maybe try again later?.\n", player_total, dealer_total);
	   do_win(ch, bettype, betamt, 2);
	   send_to_char(buf, ch); 
           clear_hands(1);
           clear_hands(2);
           game_on = 0;
	   player_total = 0;
       	   dealer_total = 0;
	   bettype=0;
	   betamt=0;
           act (STR_CARDS_BUST, FALSE, ch, obj, ch, TO_CHAR);}
         //END of the actions
	   return TRUE;
         } //end else
      }                         // End say keyword for 'xxx'
//*****************************************************************
//*****************************************************************
      if (!strcmp(arg, "showgame"))
      {                         // Start Keyword for 'xxx'
         if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
         //Start Actual ACTIONS
         showhand(obj, ch, cmd, argument, 1);
         showhand(obj, ch, cmd, argument, 2);
	if (IS_TRUSTED(ch)) //Debug check for Imms
	{  sprintf(buf, "Game Status is: %d. \n", game_on);
	   send_to_char(buf, ch);}
         //END of the actions
         return TRUE;
      }                         // End say keyword for 'xxx'
//*****************************************************************
//*****************************************************************
      if (!strcmp(arg, "showhand11"))
      {                         // Start Keyword for 'xxx'
	if (!IS_TRUSTED(ch))
	   return FALSE;
        if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
         //Start Actual ACTIONS
         showhand(obj, ch, cmd, argument, 2);
         //END of the actions
         return TRUE;
      }                         // End say keyword for 'xxx'
//*****************************************************************
//*****************************************************************
      if (!strcmp(arg, "showhand1"))
      {                         // Start Keyword for 'xxx'
	if (!IS_TRUSTED(ch))
	   return FALSE;
         if (!say(ch,arg)) //thye have to say it..
            {act (STR_CARDS_FAILED, FALSE, ch, obj, obj, TO_CHAR);} // they didnt say it somehow?
         //Start Actual ACTIONS
         showhand(obj, ch, cmd, argument, 1);
         //END of the actions
         return TRUE;
      }                         // End say keyword for 'xxx'
//*****************************************************************
//*****************************************************************
   } //This is the END of KEYWORD section
    return FALSE;
} // End of the gellz magic-deck worn proc
// End GELLZ_ magic_deck

int clear_hands(char whoshand)
{//start clear hands
   if (whoshand == 1) // empty player hand
      { //empty player hand
         for (int tmpcounter=1;tmpcounter<6;tmpcounter++)
         { //for loop tmp clear
            player_hand[tmpcounter].Suit="";
            player_hand[tmpcounter].Number = 0;
            player_hand[tmpcounter].Value = 0;
            player_hand[tmpcounter].StillIn = 0;
         } //end for loop tmp clear
         return 0;
      } //end empty playerhand
   if (whoshand == 2) // empty dealer hand
      { //empty dealer hand
         for (int tmpcounter=1;tmpcounter<6;tmpcounter++)
         { //for loop tmp clear
            dealer_hand[tmpcounter].Suit="";
            dealer_hand[tmpcounter].Number = 0;
            dealer_hand[tmpcounter].Value = 0;
            dealer_hand[tmpcounter].StillIn = 0;
         } //end for loop tmp clear
         return 0;
      } //end empty dealerhand
}//end clear hands

int needcard(char whoscard, P_char ch)
{ // start whichcard
   switch (whoscard)
   { //switch whoscard
      case 1:
         if (player_hand[1].Value == 0)
            {get_card(1,1);}
         else if (player_hand[2].Value == 0)
            {get_card(1,2);}
         else if (player_hand[3].Value == 0)
            {get_card(1,3);}
         else if (player_hand[4].Value == 0)
            {get_card(1,4);}
         else if (player_hand[5].Value == 0)
            {get_card(1,5);}
         else
            {send_to_char("Player has 5 cards already..", ch);}
         break;
      case 2:
         if (dealer_hand[1].Value == 0)
            {get_card(2,1);}
         else if (dealer_hand[2].Value == 0)
            {get_card(2,2);}
         else if (dealer_hand[3].Value == 0)
            {get_card(2,3);}
         else if (dealer_hand[4].Value == 0)
            {get_card(2,4);}
         else if (dealer_hand[5].Value == 0)
            {get_card(2,5);}
         else
            {send_to_char("Dealer has 5 cards already..", ch);
	     dealercards=5;}
         break;
    } //end Switch whoscard
} // end whichcard

int do_win(P_char ch, int bettype, int betamt, int winloose)
{
   if (winloose==1)
   { //WINNING Tasks
     send_to_char_f(ch, "\n&+yYour bank account is &+Mcredited &+W%d ", betamt);
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
   } else if (winloose==2)
   {
     // TASKS FOR LOOSING
     send_to_char_f(ch, "\n&+yYour bank account is &+Rdebited &+W%d ", betamt);
     if (bettype==0) 
       {GET_COPPER(ch)-=betamt;
       send_to_char_f(ch, STR_COPP);}
     if (bettype==1) 
       {GET_SILVER(ch)-=betamt;
       send_to_char_f(ch, STR_SILV);}
     if (bettype==2) 
       {GET_GOLD(ch)-=betamt;
       send_to_char_f(ch, STR_GOLD);}
     if (bettype==3) 
       {GET_PLATINUM(ch)-=betamt;
       send_to_char_f(ch, STR_PLAT);}
     send_to_char_f(ch, "&+y.&n\n");
   // END TASKS FOR LOOSING 
   }
}

int get_card(char whoscard, int whatcard)
{//start get_card - whoscard1=player, 2=dealer
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
}//end get card

void setup_deck(void)
{// Gellz Setup Deck
  int tmpcounter = 0;
  for (int tmpsuit=1; tmpsuit<5; tmpsuit++)
  { //Start For tmpsuit
    for (int tmpcard = 1; tmpcard<14; tmpcard++)
      { tmpcounter=tmpcounter+1;
      switch(tmpsuit)
        {
          case 1:
            deck [tmpcounter].Suit=STR_HEARTS;
            break;
          case 2:
            deck [tmpcounter].Suit=STR_DIAMONDS;
            break;
          case 3:
            deck [tmpcounter].Suit=STR_CLUBS;
            break;
          case 4:
            deck [tmpcounter].Suit=STR_SPADES;
            break;
        }//End Switch/Case
        deck [tmpcounter].Number=tmpcounter;
        switch(tmpcard)
	   {
	   case 1:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_1;
		break;
	   case 2:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_2;
		break;
	   case 3:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_3;
		break;
	   case 4:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_4;
		break;
	   case 5:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_5;
		break;
	   case 6:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_6;
		break;
	   case 7:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_7;
		break;
	   case 8:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_8;
		break;
	   case 9:
	        deck [tmpcounter].Value=tmpcard;
        	deck [tmpcounter].Display=STR_CARD_9;
		break;
	   case 10:
	        deck [tmpcounter].Value=10;
        	deck [tmpcounter].Display=STR_CARD_10;
		break;
	   case 11:
	        deck [tmpcounter].Value=10;
        	deck [tmpcounter].Display=STR_CARD_J;
		break;
	   case 12:
	        deck [tmpcounter].Value=10;
        	deck [tmpcounter].Display=STR_CARD_Q;
		break;
	   case 13:
	        deck [tmpcounter].Value=10;
        	deck [tmpcounter].Display=STR_CARD_K;
		break;
	} //close Switch tmpCard
       	deck [tmpcounter].StillIn = 1;
      }//End FOr tmpcard
  }//End FOR tmpsuit
  player_total=0;
  dealer_total=0;
}// End gellz Setup Deck

int showhand(P_obj obj, P_char ch, int cmd, char *argument, int whoscard)
{ // Just showing all deck cards.
   char         buf [MAX_STRING_LENGTH];
   string 	buftest;
         for (int tmpcounter=1; tmpcounter<6; tmpcounter++)
         { // for loop - this is all DEBUG stuff
             if (whoscard == 1)
             { //if whos is 1
		if (!(player_hand[tmpcounter].Value == 0)){
                 sprintf(buf, "&+yYour cards are %s&+y of %s&+y.", player_hand[tmpcounter].Display, player_hand[tmpcounter].Suit);
                 act (buf, FALSE, ch,obj, ch, TO_CHAR);}
             } //end IF who is 1
             if (whoscard == 2)
             { //if whos is 1
		if (!(dealer_hand[tmpcounter].Number == 0)){
                 sprintf(buf, "&+yDealer shows cards %s&+y of %s&+y.", dealer_hand[tmpcounter].Display, dealer_hand[tmpcounter].Suit);
                 act (buf, FALSE, ch,obj, ch, TO_CHAR);}
             } //end IF who is 2
         } //end for loop
	if (whoscard == 1)
	{sprintf(buf, "&+mYour total is         &+Y-- &+M%d &+Y--&+y &+mand your bet is : &+Y%d %s.\n\n", player_total, betamt, strbettype);
	   send_to_char(buf,ch);}
	if (whoscard == 2)
	{  sprintf(buf, "&+mDealer is total is    &+Y-- &+r%d &+Y--&+y.\n\n", dealer_total);
	   send_to_char(buf,ch);}
} //end showhands

//GELLZ Card Game Mains End
//*****************************************************************************

