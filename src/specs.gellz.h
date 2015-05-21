/*
        * Gellz procedures and definitions for standards here.
*/
#ifndef _SPECS_GELLZ_H_
#define _SPECS_GELLZ_H_

#define STR_CARDS_TYPE_FAIL "&+ySorry, we only accept &+Yvalid cash&+y amounts here. Please ensure you offer a valid amount and selection of coins!!&n"
#define STR_CARDS_CASH_FAIL "&+yYou &+Ydont have enough &+ycoins of that type available!&n"
#define STR_CARDS_CASH_OK "&+ySeeing that you &+Yhave enough &+ycoins available to cover your &+Wbet&+y it looks like its time to &+Wdeal&+y!!&n"
#define STR_CARDS_FAILED "Failed - inform an immortal."
#define STR_CARDS_GAME_ON "&+yWe thought you were already playing... We cannot restart a new game until that one is finshed. Please &+Wfold&+y that game first.."
#define STR_CARDS_SHUFFLE "&+yThe cards are quickly &+Rs&+Ch&+Bu&+Gf&+Yf&+Ml&+Ce&+Rd&+y and stacked, ready for a new &+Wgame!!&n\n"
#define STR_CARDS_DEAL "&+yA brief sizzle of &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y flickers over the deck, and some of the cards begin to &+Rr&+Ca&+Bn&+Gd&+Yo&+Mm&+Cl&+Ry&+y separate into two piles. One pile is before you, and the other is in front of the &+wdeck&+y itself. The cards reveal themselves to you quickly.&n\n"
#define STR_CARDS_CHOOSE "&+yThe &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y flicker over the &+wdeck&+y once more, and you realise that you can choose to either &+Whit&+y, &+Wstay&+y or &+Wfold&+y simply by telling the cards what you wish to do.&n\n"
#define STR_CARDS_GAME_0 "&+yYou dont appear to be in a game yet, or there are no cards dealt. Please tell me when its time to &+Wdeal&+y cards and we can begin playing!"
#define STR_CARDS_GAME_DEAL "&+yThere were already some cards out for you, you should either &+Whit&+y, &+Wstay&+y or &+Wfold&+y your current game."
#define STR_CARDS_FOLD "&+yYou decide to &+Rfold&+y and retire this game session. The &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y crawl over the cards and they all return to the &+wdeck&+y, taking whatever &+Wcash&+y you bet as well!!&n\n"
#define STR_CARDS_BUST "&+yYou &+RBUSTED!!&+y. The &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y crawl over the cards and they all return to the &+wdeck&+y, taking whatever &+Wcash&+y you bet as well!!&n\n"
#define STR_DIAMONDS "&+Y| &+WDiamonds&n &+Y|&n"
#define STR_HEARTS   "&+Y|  &+RHearts&n  &+Y|&n"
#define STR_SPADES   "&+Y|  &+LSpades&n  &+Y|&n"
#define STR_CLUBS    "&+Y|  &+wClubs&n   &+Y|&n"
#define STR_CARD_1   "&+Y| &+C 1&n  &+Y|&n";
#define STR_CARD_2   "&+Y| &+C 2&n  &+Y|&n";
#define STR_CARD_3   "&+Y| &+C 3&n  &+Y|&n";
#define STR_CARD_4   "&+Y| &+C 4&n  &+Y|&n";
#define STR_CARD_5   "&+Y| &+C 5&n  &+Y|&n";
#define STR_CARD_6   "&+Y| &+C 6&n  &+Y|&n";
#define STR_CARD_7   "&+Y| &+C 7&n  &+Y|&n";
#define STR_CARD_8   "&+Y| &+C 8&n  &+Y|&n";
#define STR_CARD_9   "&+Y| &+C 9&n  &+Y|&n";
#define STR_CARD_10   "&+Y| &+C10&n  &+Y|&n";
#define STR_CARD_J   "&+Y|&+C Jack&n&+Y|&n";
#define STR_CARD_Q   "&+Y|&+CQueen&n&+Y|&n";
#define STR_CARD_K   "&+Y|&+C King&n&+Y|&n";
#define STR_PLAT   "&+WPlatinum&n"
#define STR_GOLD   "&+YGold&n"
#define STR_SILV   "&+wSilver&n"
#define STR_COPP   "&+yCopper&n"

#include <iostream>
#include <vector>
using namespace std;

#include "structs.h"

struct cards
{
   const char* Suit;
   int  Number;
   int  Value;
   int  StillIn;
   const char* Display;
} deck [52];

cards player_hand[5];
cards dealer_hand[5];

int     playertotal;
int     dealertotal;
int     game_on;
int     bet_on;
int	player_total;
int	dealer_total;
int	betamt;
int	bettype;
int	dealercards;
const char*	strbettype;

//  PROTOTYPES REQUIRED FOR CARD GAME

int gellz_test_obj_procs(P_obj obj, P_char ch, int cmd, char *argument);

int magic_deck(P_obj obj, P_char ch, int cmd, char *argument);
int clear_hands(char whoshand);
int needcard(char whoscard, P_char ch);
int do_win(P_char ch, int bettype, int betamt, int winloose);
int get_card(char whoscard, int whatcard);
void setup_deck();
int showhand(P_obj obj, P_char ch, int cmd, char *argument, int whoscard);

#endif

