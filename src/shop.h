/*
 * ***************************************************************************
 * *  File: shop.h                                             Part of Duris 
 * *  Usage: Defines and structs for handling shops and shopkeepers.                       
 * *  Copyright  1990, 1991 - see 'license.doc' for complete information.      
 * *  Copyright 1994 - 2008 - Duris Systems Ltd.                             
 * ***************************************************************************
 */

#define NOTHING 0
#define SHOP_FUNC(i)         (shop_index[(i)].func)
#define SHOP_NUM(i)   (shop_index[(i)].virtual)
#define SHOP_KEEPER(i)    (shop_index[(i)].keeper)
#define SHOP_OPEN1(i)   (shop_index[(i)].open1)
#define SHOP_CLOSE1(i)    (shop_index[(i)].close1)
#define SHOP_OPEN2(i)   (shop_index[(i)].open2)
#define SHOP_CLOSE2(i)    (shop_index[(i)].close2)
#define SHOP_ROOM(i, num) (shop_index[(i)].in_room[(num)])
#define SHOP_BUYTYPE(i, num)  (BUY_TYPE(shop_index[(i)].type[(num)]))
#define SHOP_BUYWORD(i, num)  (BUY_WORD(shop_index[(i)].type[(num)]))
#define SHOP_PRODUCT(i, num)  (shop_index[(i)].producing[(num)])
#define SHOP_BUYPROFIT(i) (shop_index[(i)].profit_buy)
#define SHOP_SELLPROFIT(i)  (shop_index[(i)].profit_sell)
#define MSG_NOT_OPEN_YET  "Come back later!"
#define MSG_NOT_REOPEN_YET  "Sorry, we have closed, but come back later."
#define MSG_CLOSED_FOR_DAY  "Sorry, come back tomorrow."
#define MSG_NO_STEAL_HERE "$n is a bloody thief!"
#define MSG_NO_SEE_CHAR   "I don't trade with someone I can't see!"
#define MSG_NO_SELL_ALIGN "Get out of here before I call the guards!"
#define MSG_NO_SELL_CLASS "We don't serve your kind here!"
#define MSG_NO_USED_WANDSTAFF "I don't buy used up wands or staves!"
#define MSG_CANT_KILL_KEEPER  "Get out of here before I call the guards!"
#define BUY_TYPE(i)   ((i).type)
#define BUY_WORD(i)   ((i).keywords)
#define MAX_SHOP_OBJ  100	/*
				 * "Soft" maximum for list maximums 
				 */
#define END_OF(buffer)    ((buffer) + strlen((buffer)))
#define S_DATA(stack, index)  ((stack)->data[(index)])
#define S_LEN(stack)    ((stack)->len)
#define OPER_OPEN_PAREN   0
#define OPER_CLOSE_PAREN  1
#define OPER_OR     2
#define OPER_AND    3
#define OPER_NOT    4
#define MAX_OPER    4
#define OBJECT_DEAD   0
#define OBJECT_NOTOK    1
#define OBJECT_OK   2
#define LIST_PRODUCE    0
#define LIST_TRADE    1
#define LIST_ROOM   2
