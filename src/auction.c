/****************************************************************************
 *
 * File: auction.c, handling of the automated auction system.
 *
 ***************************************************************************/

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "config.h"
#include "justice.h"

extern P_index           mob_index;
extern P_index           obj_index;
extern P_room            world;
extern struct zone_data *zone_table;

struct auction_data auction[LAST_HOME];

void init_auctioneers(void)
{
	int i;
	int vnums[LAST_HOME + 1][2] = {
		/*
	     * auctioneer vnum, auction room
	     */
		{132598, 132821}, /* Tharnadia */
		{ 97590,  97756}, /* Shady */
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0},
		{     0,      0}
    };
	for (i = 1; i <= LAST_HOME; i++)
	{
		auction[i].auctioneer = vnums[i - 1][0];
		auction[i].room       = vnums[i - 1][1];
		auction[i].item       = NULL;
		auction[i].bet        = 0;
		auction[i].buyer      = NULL;
		auction[i].seller     = NULL;
		auction[i].pulse      = 0;
		auction[i].going      = 0;
	}
	return;
}

P_char find_auctioneer(int town)
{
	P_char ch, ch_next;

	if (!auction[town].auctioneer) /* Town hasn't been assigned one yet */
		return NULL;

	for (ch = world[real_room0(auction[town].room)].people; ch; ch = ch_next)
	{
		ch_next = ch->next_in_room;
		if (IS_NPC(ch) && auction[town].auctioneer == mob_index[GET_RNUM(ch)].virtual_number)
			return ch;
	}
	return NULL;
}

/*
 * put an item on auction, or see the stats on the current item or bet
 */
void do_auction(P_char ch, char *argument, int dummy)
{
	P_char auctioneer = NULL;
	P_obj  obj;
	char   arg1[MAX_INPUT_LENGTH];
	char   buf[MAX_STRING_LENGTH];
	int    i, room;
	int    newbet = 0;

	argument = one_argument(argument, arg1);
	i        = CHAR_IN_TOWN(ch);
	if (!i) /* not in a town */
		return;

	auctioneer = find_auctioneer(i);
	room       = real_room0(auction[i].room);
	if (!auctioneer || ch->in_room != room || auctioneer->in_room != room)
	{
		send_to_char("Are you sure you're in the auction house and the auctioneer is present?\r\n", ch);
		return;
	}
	if (arg1[0] == '\0')
		if (auction[i].item != NULL)
		{
			/* show item data here */
			spell_lore(50, ch, NULL, 0, 0, auction[i].item);
			if (auction[i].bet > 0)
				snprintf(buf, MAX_STRING_LENGTH, "Current bid on this item is %s.\r\n", coin_stringv(auction[i].bet));
			else
				snprintf(buf, MAX_STRING_LENGTH, "No bids on this item have been received.\r\n");
			send_to_char(buf, ch);
			return;
		}
		else
		{
			send_to_char("Auction WHAT?\r\n", ch);
			return;
		}
	if (IS_TRUSTED(ch) && !str_cmp(arg1, "debug"))
	{
		snprintf(buf,
		         MAX_STRING_LENGTH,
		         "Auctioneer: %d, Room: %d, Item: %s, Bid: %d, Buyer: %s, Seller: %s, Pulse: %d, Going: %d\r\n",
		         auction[i].auctioneer,
		         auction[i].room,
		         auction[i].item ? auction[i].item->short_description : "Nothing",
		         auction[i].bet,
		         auction[i].buyer ? J_NAME(auction[i].buyer) : "No-one",
		         auction[i].seller ? J_NAME(auction[i].seller) : "No-one",
		         auction[i].pulse,
		         auction[i].going);
		send_to_char(buf, ch);
		return;
	}
	if (IS_TRUSTED(ch) && !str_cmp(arg1, "stop"))
		if (auction[i].item == NULL)
		{
			send_to_char("There is no auction going on you can stop.\r\n", ch);
			return;
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH, "Sale of %s has been stopped by God. Item confiscated.", auction[i].item->short_description);
			mobsay(auctioneer, buf);
			obj_to_char(auction[i].item, ch);
			auction[i].item = NULL;
			/* return money to the buyer */
			if (auction[i].buyer != NULL)
			{
				ADD_MONEY(auction[i].buyer, auction[i].bet);
				send_to_char("Your money has been returned.\r\n", auction[i].buyer);
			}
			return;
		}
	if (!str_cmp(arg1, "bid"))
		if (auction[i].item != NULL)
		{
		/* make - perhaps - a bet now */
#if 0
      if (ch == auction[i].seller && !IS_TRUSTED(ch))
      {
        send_to_char("Sorry, you can't bid on your own items.\r\n", ch);
        return;
      }
#endif
			if (argument[0] == '\0')
			{
				send_to_char("Bid how much?\r\n", ch);
				return;
			}
			argument = skip_spaces(argument);
			newbet   = parsebet(auction[i].bet, argument);
			if (newbet)
			{
				snprintf(buf, MAX_STRING_LENGTH, "You bid %s.\r\n", coin_stringv(newbet));
				send_to_char(buf, ch);
			}
			if (newbet < (auction[i].bet + (auction[i].bet / 100 * 5)))
			{
				send_to_char("You must bid at least %5 over the current bid.\r\n", ch);
				return;
			}
			if (newbet > GET_MONEY(ch))
			{
				send_to_char("You don't have that much money!\r\n", ch);
				return;
			}
			if (!newbet)
			{
				send_to_char("Bid how much?\r\n", ch);
				return;
			}
			/* the actual bet is OK! */
			/* return the gold to the last buyer, if one exists */
			if (auction[i].buyer != NULL)
				ADD_MONEY(auction[i].buyer, auction[i].bet);
			SUB_MONEY(ch, newbet, 0);
			auction[i].buyer = ch;
			auction[i].bet   = newbet;
			auction[i].going = 0;
			auction[i].pulse = PULSE_AUCTION; /* start the auction over again */
			snprintf(buf, MAX_STRING_LENGTH, "%s has bid %s on %s.", J_NAME(auction[i].buyer), coin_stringv(newbet), auction[i].item->short_description);
			mobsay(auctioneer, buf);
			return;
		}
		else
		{
			send_to_char("There isn't anything being auctioned right now.\r\n", ch);
			return;
		}
	/*
	 * finally...
	 */

	obj = get_obj_in_list_vis(ch, arg1, ch->carrying);

	if (obj == NULL)
	{
		send_to_char("You aren't carrying that.\r\n", ch);
		return;
	}
	if (IS_SET(obj->extra_flags, ITEM_NODROP))
	{
		send_to_char("A cursed item? I don't think so!\r\n", ch);
		return;
	}

	/* added this, then realized it would be a bit silly, since the players
	   could simply auction it without the middleman - but let's keep it
	   available, eh?  -Tavril */

#if 0
  if (IS_SET(obj->extra_flags, ITEM_NOSELL))
  {
    send_to_char
      ("The auctioneer indicates with a curt nod that that item cannot be sold.\r\n",
       ch);
    return;
  }
#endif

	if (argument[0] != '\0')
	{
		argument       = skip_spaces(argument);
		newbet         = parsebet(1, argument);
		auction[i].bet = newbet;
	}
	if (auction[i].item == NULL)
	{
		obj_from_char(obj, FALSE);
		auction[i].item   = obj;
		auction[i].bet    = 0;
		auction[i].buyer  = NULL;
		auction[i].seller = ch;
		auction[i].pulse  = PULSE_AUCTION;
		auction[i].going  = 0;
		snprintf(buf, MAX_STRING_LENGTH, "A new item has been received: %s.", obj->short_description);
		mobsay(auctioneer, buf);
		if (auction[i].bet)
		{
			snprintf(buf, MAX_STRING_LENGTH, "Minimum bid on this item is %s.", coin_stringv(auction[i].bet));
			mobsay(auctioneer, buf);
		}
		return;
	}
	else
	{
		act("Try again later - $p is being auctioned right now!", FALSE, ch, auction[i].item, NULL, TO_CHAR);
		return;
	}
}

void auction_update(void)
{
	P_char auctioneer = NULL;
	char   buf[MAX_STRING_LENGTH];
	int    i;

	for (i = 1; i < LAST_HOME; i++)
	{
		auctioneer = find_auctioneer(i);
		if (auction[i].item != NULL && auctioneer) /* * decrease pulse */
			if (--auction[i].pulse <= 0)
			{
				auction[i].pulse = PULSE_AUCTION;
				/* * increase the going state */
				switch (++auction[i].going)
				{
					case 1: /* * going once */
					case 2: /* * going twice */
						if (auction[i].bet > 0)
							snprintf(buf, MAX_STRING_LENGTH, "%s: going %s for %s.", auction[i].item->short_description, ((auction[i].going == 1) ? "once" : "twice"), coin_stringv(auction[i].bet));
						else
							snprintf(buf, MAX_STRING_LENGTH, "%s: going %s with no bids yet received.", auction[i].item->short_description, ((auction[i].going == 1) ? "once" : "twice"));
						mobsay(auctioneer, buf);
						break;

					case 3: /* * SOLD! */

						if (auction[i].bet > 0)
						{
							snprintf(buf, MAX_STRING_LENGTH, "%s sold to %s for %s.", auction[i].item->short_description, J_NAME(auction[i].buyer), coin_stringv(auction[i].bet));
							mobsay(auctioneer, buf);
							ADD_MONEY(auction[i].seller, auction[i].bet);
							obj_to_char(auction[i].item, auction[i].buyer);
							/* tranaction */
							auction[i].item = NULL; /* * reset item */
						}
						else
						{ /* * not sold */
							snprintf(buf, MAX_STRING_LENGTH, "No bids received for %s - object has been removed.", auction[i].item->short_description);
							mobsay(auctioneer, buf);
							obj_to_char(auction[i].item, auction[i].seller);
							auction[i].item = NULL; /* clear auction */
						}
				}
			}
	}
}

/*
 * These functions allow the following kinds of bets to be made:
 *
 * Relative bet
 * ============
 *
 * bet +50 bet +33
 *
 * These bets are calculated relative to the current bet. The '+' symbol adds
 * a certain number of percent to the current bet. The default is 25, so
 * with a current bet of 1000, bet + gives 1250, bet +50 gives 1500 etc.
 * Please note that the number must follow exactly after the +, without any
 * spaces!
 *
 * The '*' or 'x' bet multiplies the current bet by the number specified,
 * defaulting to 2. If the current bet is 1000, bet x  gives 2000, bet x10
 * gives 10,000 etc.
 *
 * Absolute bet
 * ============
 *
 * bet 14p, bet 50g66, bet 100s
 *
 * Advanced strings can contain 'p', 'g' and/or 's' in them, not just
 * numbers. The letters multiply whatever is left of them by 1,000, 100, 10.
 * Example:
 *
 * 14p = 14 * 1,000 = 14,000
 * 23g = 23 * 100 = 2300
 *
 */

int advatoi(const char *s)
{
	char  string[MAX_INPUT_LENGTH];
	char *stringptr = string;
	int   num = 0, num2 = 0, num3 = 0;
	int   multiplier = 0;

	strcpy(string, s);
	while (isdigit(*stringptr))
	{                                        /* while current character is a digit */
		num = (num * 10) + stringptr[0] - '0'; /* add to current number */
		stringptr++;                         /* advance */
	}

	switch (UPPER(*stringptr))
	{
		case 'P':
			multiplier = 1000;
			num *= multiplier;
			stringptr++;
			break;
		case 'G':
			multiplier = 100;
			num *= multiplier;
			stringptr++;
			break;
		case 'S':
			multiplier = 10;
			num *= multiplier;
			stringptr++;
			break;
		case '\0':
			break;
		default:
			return 0;
	}

	while (isdigit(*stringptr))
	{                                      /* if any digits follow add those too */
		num2 = num2 + stringptr[0] - '0';
		stringptr++;
	}

	if (*stringptr != '\0' && !isdigit(*stringptr)) /* a non-digit character was found, other than NUL */
		switch (UPPER(*stringptr))
		{
			case 'G': /* If they didn't use 'P' first, fuck em */
				multiplier = 100;
				num2 *= multiplier;
				stringptr++;
				break;
			case 'S':
				multiplier = 10;
				num2 *= multiplier;
				stringptr++;
				break;
			case '\0':
				break;
			default:
				return 0;
		}
	while (isdigit(*stringptr))
	{                                      /* if any digits follow add those too */
		num3 = num3 + stringptr[0] - '0';
		stringptr++;
	}

	if (*stringptr != '\0' && !isdigit(*stringptr)) /* a non-digit character was found, other than NUL */
		switch (UPPER(*stringptr))
		{
			case 'S': /* Same here, moot point :P */
				multiplier = 10;
				num3 *= multiplier;
				stringptr++;
				break;
			case '\0':
				break;
			default:
				return 0;
		}
	while (isdigit(*stringptr))
	{                                      /* if any digits follow add those too */
		num3 = num3 + stringptr[0] - '0';
		stringptr++;
	}
	num = num + num2 + num3;
	return (num);
}

int parsebet(const int currentbet, const char *argument)
{

	int   newbet = 0;
	char  string[MAX_INPUT_LENGTH];
	char *stringptr = string;

	strcpy(string, argument); /* make a work copy of argument */

	if (*stringptr)
	{                                    /* check for an empty string */
		if (isdigit(*stringptr))         /* first char is a digit assume e.g. 433k */
			newbet = advatoi(stringptr); /* parse and set newbet to that value */

		else if (*stringptr == '+')
		{                                          /* add ?? percent */
			if (strlen(stringptr) == 1)            /* only + specified, assume default */
				newbet = (currentbet * 125) / 100; /* * default: add 25% */
			else
				newbet = (currentbet * (100 + atoi(++stringptr))) / 100;
		}
		else
		{
			if ((*stringptr == '*') || (*stringptr == 'x')) /* multiply */
				if (strlen(stringptr) == 1)                 /* only x specified, assume default */
					newbet = currentbet * 2;                /* default: twice */
				else                                        /* user specified a number */
					newbet = currentbet * atoi(++stringptr);
		}
	}
	return newbet; /* return the calculated bet */
}
