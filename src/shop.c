/****************************************************************************
 *  File: shop.c                                             Part of Duris *
 *  Usage: Procedures handling shops and shopkeepers.                        *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 *************************************************************************** */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "shop.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "justice.h"
#include "salchemist.h"
#include "specs.prototypes.h"
#include "sql.h"
#include "epic_bonus.h"
#include "damage.h"

/*
 * external variables
 */

extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const int top_of_world;
extern char *drinks[];
extern const struct stat_data stat_factor[];
extern const char *item_types[];
extern const flagDef extra_bits[];
extern struct str_app_type str_app[];
extern struct cha_app_type cha_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;

struct shop_data *shop_index;
int      number_of_shops = 0;
const char *operator_str[] = {
  "[({",
  "])}",
  "|+",
  "&*",
  "^'"
};

void lore_item( P_char ch, P_obj obj );

void push(struct stack_data *stack, int pushval)
{
  S_DATA(stack, S_LEN(stack)++) = pushval;
}

int topp(struct stack_data *stack)
{
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, S_LEN(stack) - 1));
  else
    return 0;
}

int pop(struct stack_data *stack)
{
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, --S_LEN(stack)));
  else
  {
    logit(LOG_DEBUG, "Illegal expression in shop keyword list");
    return (0);
  }
}

void evaluate_operation(struct stack_data *ops, struct stack_data *vals)
{
  int      oper;

  if ((oper = pop(ops)) == OPER_NOT)
    push(vals, !pop(vals));
  else if (oper == OPER_AND)
    push(vals, pop(vals) && pop(vals));
  else if (oper == OPER_OR)
    push(vals, pop(vals) || pop(vals));
}

int find_oper_num(char token)
{
  int      i;

  for (i = 0; i <= MAX_OPER; i++)
    if (strchr(operator_str[i], token))
      return (i);
  return 0;
}

int evaluate_expression(P_obj obj, char *expr)
{
  struct stack_data ops, vals;
  char    *ptr, *end, name[200];
  int      temp, i;

  if (!expr)
    return (TRUE);

  ops.len = vals.len = 0;
  ptr = expr;
  while (*ptr)
  {
    if (isspace(*ptr))
      ptr++;
    else
    {
      if( (temp = find_oper_num(*ptr)) == 0 )
      {
        end = ptr;
        while( *ptr && !isspace(*ptr) && (find_oper_num(*ptr) == 0) )
          ptr++;
        strncpy(name, end, (unsigned) (ptr - end));
        name[ptr - end] = 0;
        for (i = 0; extra_bits[i].flagShort; i++)
          if (!str_cmp(name, extra_bits[i].flagShort))
          {
            push(&vals, (int) IS_SET(obj->extra_flags, 1 << i));
            break;
          }
        if (!extra_bits[i].flagShort)
          push(&vals, isname(name, obj->name));
      }
      else
      {
        if (temp != OPER_OPEN_PAREN)
          while (topp(&ops) > temp)
            evaluate_operation(&ops, &vals);

        if (temp == OPER_CLOSE_PAREN)
        {
          if ((temp = pop(&ops)) != OPER_OPEN_PAREN)
          {
            logit(LOG_DEBUG,
                  "Illegal parenthesis in shop keyword expression");
            return (FALSE);
          }
        }
        else
          push(&ops, temp);
        ptr++;
      }
    }
  }
  while( topp(&ops) != 0 )
    evaluate_operation(&ops, &vals);
  temp = pop(&vals);
  if( topp(&vals) != 0 )
  {
    logit(LOG_DEBUG, "Extra operands left on shop keyword expression stack");
    return (FALSE);
  }
  return (temp);
}

int is_ok(P_char keeper, P_char ch, int shop_nr)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  if (shop_index[shop_nr].open1 > time_info.hour)
  {
    mobsay(keeper, "Come back later!");
    return (FALSE);
  }
  else if (shop_index[shop_nr].close1 < time_info.hour)
  {
    if (shop_index[shop_nr].open2 > time_info.hour)
    {
      mobsay(keeper, "Sorry, we have closed, but come back later.");
      return (FALSE);
    }
    else if (shop_index[shop_nr].close2 < time_info.hour)
    {
      mobsay(keeper, "Sorry, come back tomorrow.");
      return (FALSE);
    }
  }
  /*
   * If shopkeeper is racist, turn customer away. MIAX
   */
  if ((shop_index[shop_nr].racist == 1) && !IS_TRUSTED(ch))
  {
    if (shop_index[shop_nr].shopkeeper_race != (unsigned) GET_RACE(ch))
    {

      /*
       * old version gave namelist of keeper, not short descr. - DTS
       * snprintf(Gbuf1, MAX_STRING_LENGTH, "%s says to %s, '%s'", GET_NAME(keeper), GET_NAME(ch),
       * shop_index[shop_nr].racist_message);
       */
      snprintf(Gbuf1, MAX_STRING_LENGTH, "%s says to %s, '%s'",
              keeper->player.short_descr,
              ((IS_PC(ch)) ? GET_NAME(ch) : ch->player.short_descr),
              shop_index[shop_nr].racist_message);
      act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
      /*
       * old version gave namelist of keeper, not short descr. - DTS
       * snprintf(Gbuf1, MAX_STRING_LENGTH, "%s says to you, '%s'", GET_NAME(keeper),
       * shop_index[shop_nr].racist_message);
       */
      snprintf(Gbuf1, MAX_STRING_LENGTH, "%s says to you, '%s'",
              keeper->player.short_descr, shop_index[shop_nr].racist_message);
      act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
      return (FALSE);
    }
  }
  if (!(CAN_SEE(keeper, ch)) && !IS_TRUSTED(ch))
  {
    mobsay(keeper, "I don't trade with someone I can't see!");
    return (FALSE);
  };

  switch (shop_index[shop_nr].with_who)
  {
  case 0:
    return (TRUE);
  case 1:
    return (TRUE);
  default:
    return (TRUE);
  };
}

int same_obj(P_obj obj1, P_obj obj2)
{
  int      i;

  if (!obj1 || !obj2)
    return (obj1 == obj2);

  if (obj1->R_num != obj2->R_num)
    return (FALSE);

  if (obj1->cost != obj2->cost)
    return (FALSE);

  if (obj1->extra_flags != obj2->extra_flags)
    return (FALSE);

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if ((obj1->affected[i].location != obj2->affected[i].location) ||
        (obj1->affected[i].modifier != obj2->affected[i].modifier))
      return (FALSE);

  return (TRUE);
}

char    *times_message(P_obj obj, char *name, int num)
{
  static char buf[256];
  char    *ptr;

  if (obj)
    strcpy(buf, obj->short_description);
  else
  {
    if ((ptr = strchr(name, '.')) == NULL)
      ptr = name;
    else
      ptr++;
    snprintf(buf, 256, "A %s", ptr);
  }

  if (num > 1)
    snprintf(END_OF(buf), MAX_STRING_LENGTH, " (x %d)", num);
  return (buf);
}

P_obj get_slide_obj_vis(P_char ch, char *name, P_obj list)
{
  P_obj    i, last_match = NULL;
  int      j, num;
  char     tmpname[MAX_INPUT_LENGTH];
  char    *tmp;

  strcpy(tmpname, name);
  tmp = tmpname;
  if (!(num = get_number(&tmp)))
    return (0);

  for (i = list, j = 1; i && (j <= num); i = i->next_content)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i))
      {
        if (j == num)
          return (i);
        last_match = i;
        j++;
      }
  return (0);
}

P_obj get_hash_obj_vis(P_char ch, char *name, P_obj list)
{
  P_obj    loop, last_obj = NULL;
  int      i;

  if ((is_number(name + 1)))
    i = atoi(name + 1);
  else
    return (0);

  for (loop = list; loop; loop = loop->next_content)
    if (CAN_SEE_OBJ(ch, loop) && (loop->cost > 0))
      if (!same_obj(last_obj, loop))
      {
        if (--i == 0)
          return (loop);
        last_obj = loop;
      }
  return (0);
}

P_obj get_purchase_obj(P_char ch, char *arg, P_char keeper, int shop_nr, int msg)
{
  char  buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
  P_obj obj;

  one_argument(arg, name);
  do
  {
    if( *name == '#' )
      obj = get_hash_obj_vis(ch, name, keeper->carrying);
    else
      obj = get_slide_obj_vis(ch, name, keeper->carrying);
    if( !obj )
    {
      if (msg)
      {
        snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
      }
      return NULL;
    }
    if( obj->cost <= 0 )
    {
      extract_obj(obj, TRUE); // Shopkeeper with cost 0 arti?
      obj = NULL;
    }
  }
  while( !obj );
  return obj;
}

int trade_with(P_obj item, int shop_nr, char repairing)
{
  int      counter;

  if (item->cost < 1)
    return (OBJECT_NOTOK);

  if ((IS_OBJ_STAT(item, ITEM_NOSELL) && !repairing) ||
      IS_OBJ_STAT(item, ITEM_TRANSIENT))
    return (OBJECT_NOTOK);

  for (counter = 0; SHOP_BUYTYPE(shop_nr, counter) != 0; counter++)
  {
    if (SHOP_BUYTYPE(shop_nr, counter) == (item)->type)
    {
      if (((item->value[2] == 0) &&
           (((item)->type == ITEM_WAND) || ((item)->type == ITEM_STAFF))))
/*         (item->condition < 10))*/
        return (OBJECT_DEAD);
      else if (evaluate_expression(item, SHOP_BUYWORD(shop_nr, counter)))
        return (OBJECT_OK);
      return (OBJECT_OK);
    }
    else if (SHOP_BUYTYPE(shop_nr, counter) == ITEM_ARMOR &&
             item->type == ITEM_WORN)
/*        if (item->condition > 25)*/
      return (OBJECT_OK);
/*        else return (OBJECT_DEAD);*/
  }
  return (OBJECT_NOTOK);
}

int shop_producing(P_obj item, int shop_nr)
{
  int      counter;

  if (item->R_num < 0)
    return (FALSE);

  for (counter = 0; counter < shop_index[shop_nr].number_items_produced;
       counter++)
    if (shop_index[shop_nr].producing[counter] == item->R_num)
      return (TRUE);
  return (FALSE);
}

P_obj
get_selling_obj(P_char ch, char *name, P_char keeper, int shop_nr, int msg,
                char repairing)
{
  char     buf[MAX_STRING_LENGTH];
  P_obj    obj;
  int      result;

  if (!(obj = get_obj_in_list_vis(ch, name, ch->carrying)))
  {
    if (msg)
    {
      snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].no_such_item2, GET_NAME(ch));
      mobsay(keeper, buf);
    }
    return (0);
  }
  if ((result = trade_with(obj, shop_nr, repairing)) == OBJECT_OK)
    return (obj);

  switch (result)
  {
  case OBJECT_NOTOK:
    snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].do_not_buy, GET_NAME(ch));
    break;
  case OBJECT_DEAD:
    snprintf(buf, MAX_STRING_LENGTH, "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
    break;
  default:
    snprintf(buf, MAX_STRING_LENGTH, "Illegal return value of %d from trade_with() (shop.c)",
            result);
    logit(LOG_DEBUG, buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s An error has occurred.", GET_NAME(ch));
    break;
  }
  if (msg)
    mobsay(keeper, buf);
  return (0);
}

void shopping_buy(char *arg, P_char ch, P_char keeper, int shop_nr)
{
  char     argm[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  P_obj    temp1, gem = NULL, container;
  int      i = 0, sale;
  float    cost_factor;
  char     Gbuf1[MAX_STRING_LENGTH];

  if( !(is_ok(keeper, ch, shop_nr)) )
  {
    return;
  }

  arg = one_argument(arg, argm);
  if( !(*argm) )
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s what do you want to buy??", GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    return;
  };

  /*
   * Added if statement from epic code from buying by numbers MIAX
   */
  if( !(temp1 = get_obj_in_list_vis(ch, argm, keeper->carrying)) )
  {
    if( atoi(argm) )
    {
      for( temp1 = keeper->carrying; temp1; temp1 = temp1->next_content )
      {
        if( IS_ARTIFACT(temp1) || isname("encrust", temp1->name) )
        {
          wizlog(56, "(%s) shopkeeper just destroyed (%s).", GET_NAME(keeper), (temp1->short_description));
          extract_obj(temp1, TRUE); // Bye arti.
          continue;
        }

        if ((CAN_SEE_OBJ(ch, temp1)) && (temp1->cost > 0))
        {
          if (++i == atoi(argm))
          {
            break;
          }
        }
      }
    }
    if ( !temp1 )
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
      do_tell(keeper, Gbuf1, 0);
      return;
    }
  }

  if( temp1->cost <= 0 )
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    extract_obj(temp1, TRUE); // Arti with no cost?
    return;
  }

  cost_factor = (float) cha_app[STAT_INDEX(MAX(100, GET_C_CHA(ch)))].modifier;
  if( GET_RACE(ch) != GET_RACE(keeper) )
    cost_factor = cost_factor * 2.;

  cost_factor = shop_index[shop_nr].sell_percent * (1.0 - (cost_factor / 100.));
  if( cost_factor < shop_index[shop_nr].sell_percent )
  {
    cost_factor = shop_index[shop_nr].sell_percent + .01;
  }

  if( has_innate(ch, INNATE_BARTER) )
  {
    if (GET_C_CHA(ch) > number(0, 125))
    {
      cost_factor -= .25;
    }
    else
    {
      cost_factor += .10;
    }
  }

  sale = (int) (temp1->cost * cost_factor);

  // hook for epic bonus
  sale -= (int) (sale * get_epic_bonus(ch, EPIC_BONUS_SHOP));

  if (sale < 1)
  {
    sale = 1;
  }

  if( (GET_MONEY(ch) < sale) && !IS_TRUSTED(ch) )
  {
    gem = accept_gem_for_debt(ch, keeper, sale);
  }

  if( (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) )
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s : You can't carry that many items.\r\n",
            FirstWord(temp1->name));
    send_to_char(Gbuf1, ch);
    return;
  }
  if( !IS_TRUSTED(ch) )
  {
    if( !transact(ch, gem, keeper, sale) )
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
      mobsay(keeper, Gbuf1);
      return;
    }
  }
  act("$n buys $p.", FALSE, ch, temp1, 0, TO_ROOM);
  snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].message_buy, GET_NAME(ch),
          coin_stringv(sale) );
  do_tell(keeper, Gbuf1, 0);
 // SET_BIT(temp1->type, ITEM_TREASURE);
  snprintf(Gbuf1, MAX_STRING_LENGTH, "You now have %s.\r\n", temp1->short_description);
  send_to_char(Gbuf1, ch);

/*
  if (!number(0, 3))
    shop_index[shop_nr].buy_percent += .03;
  if (!number(0, 5))
    shop_index[shop_nr].sell_percent += .03;
*/
  /* Test if producing shop !   */
  if( shop_producing(temp1, shop_nr) )
  {
    temp1 = read_object(temp1->R_num, REAL);
  }
  else
  {
    obj_from_char(temp1);
  }
  SET_BIT(temp1->extra2_flags, ITEM2_STOREITEM);
  obj_to_char(temp1, ch);
  deleteShopKeeper(shop_nr);
  writeShopKeeper(keeper);

  // Format: buy <object> <container> <amount>
  if( shop_producing(temp1, shop_nr) )
  {
    arg = one_argument(arg, arg2);
    // Didn't specify a container.
    if( !(*arg2) )
    {
      return;
    }
    if( !(container = get_obj_in_list(arg2, ch->carrying)) )
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You don't seem to have a '%s'.\r\n", arg2);
      send_to_char(Gbuf1, ch);
      return;
    }
    if( container->type != ITEM_CONTAINER )
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "%s&n isn't a container.\r\n", container->short_description);
      send_to_char(Gbuf1, ch);
      return;
    }
    put( ch, temp1, container, TRUE );
    arg = one_argument(arg, arg3);
    if( atoi(arg3) > 1 )
    {
      if( atoi(arg3) > 50 )
      {
        send_to_char( "The limit for buying items is 50 at a time.\n\r", ch );
        return;
      }
      snprintf(Gbuf1, MAX_STRING_LENGTH, "%s %s %d", argm, arg2, atoi(arg3) - 1 );
      shop_keeper( keeper, ch, CMD_BUY, Gbuf1 );
    }
  }

  return;
}

void shopping_sell(char *arg, P_char ch, P_char keeper, int shop_nr)
{
  P_obj    temp1;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     argm[MAX_INPUT_LENGTH];
  float    cost_factor;
  int      sale;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  one_argument(arg, argm);

  if (!(*argm))
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s What do you want to sell??", GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    return;
  }
  temp1 = get_selling_obj(ch, argm, keeper, shop_nr, TRUE, FALSE);
  if (!temp1)
    return;

  if (obj_index[temp1->R_num].virtual_number > (FIRST_POTION_VIRTUAL - 1) &&
      (obj_index[temp1->R_num].virtual_number < (FIRST_POTION_VIRTUAL + 25)))
  {
    mobsay(keeper, "I do not want to buy that.");
    return;
  }

  if(IS_ARTIFACT(temp1))
  {
    wizlog(56, "ARTIFACT: (%s) just tried to sell (%s).",
      GET_NAME(ch), temp1->short_description);
    mobsay(keeper, "For fear of my life, I won't buy that.");
    return;
  }

  if(isname("encrust", temp1->name))
  {
    wizlog(56, "ENCRUST: (%s) just tried to sell (%s).",
      GET_NAME(ch), temp1->short_description);
    mobsay(keeper, "I do not need encrusted items.");
    return;
  }
  
  /* patch to avoid selling containers full of stuff.  JAB */
  if ((temp1->type == ITEM_CONTAINER ||
       temp1->type == ITEM_STORAGE) && temp1->contains)
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "%s HA!  I'm not buying that, who knows what might be in there!",
            GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    return;
  }
  cost_factor = (float) cha_app[STAT_INDEX(MAX(100, GET_C_CHA(ch)))].modifier;
  if (GET_RACE(ch) != GET_RACE(keeper))
    cost_factor = cost_factor / 2.;

  cost_factor =
    shop_index[shop_nr].buy_percent * (1.0 + (cost_factor / 100.));
  if (cost_factor > shop_index[shop_nr].buy_percent)
    cost_factor = shop_index[shop_nr].buy_percent - .01;

/*
  Taken out the staff/scroll/wand/potion modifier since players only
  sell these items now and never buy them
  if (temp1->type == ITEM_SCROLL || temp1->type == ITEM_POTION ||
      temp1->type == ITEM_STAFF || temp1->type == ITEM_WAND)
    cost_factor *= .1;
*/
  /* condition affects value too */
  sale = (int) (temp1->cost * cost_factor * MIN(100, temp1->condition) / 100);

  if (sale < 1)
    sale = 1;

  if ((GET_VNUM(keeper) != 11005 ) &&    /* guild shops don't lose cash */
      ((shop_index[shop_nr].shop_is_roaming == 1) &&
       (GET_MONEY(keeper) < sale)))
  {

    snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].missing_cash1, GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    return;
  }
  if (IS_SET(temp1->extra_flags, ITEM_NODROP))
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH,
            "%s That looks wonderful, but I don't feel comfortable buying that.",
            GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    return;
  }
  act("$n sells $p.", FALSE, ch, temp1, 0, TO_ROOM);

  snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].message_sell, GET_NAME(ch),
          coin_stringv(sale));
  
  int temp = 0;
  
  if( (temp = sql_shop_trophy(temp1)) > 1){
    int orig_sale = sale;

    sale -= (int) (get_property("shops.sellTrophyMod", 0.05) * temp * sale) ;
    sale = MAX( (int) (get_property("shops.sellMinPct", 0.10) * orig_sale), sale );
  
    if( sale < 1 )
      sale = 1;
  
    snprintf(Gbuf1, MAX_STRING_LENGTH, "The shopkeeper says 'This item is rather common, you won't get as much for it.'\r\n");
    send_to_char(Gbuf1, ch);
  }

  do_tell(keeper, Gbuf1, 0);
  
  sql_shop_sell(ch, temp1, sale);

  snprintf(Gbuf1, MAX_STRING_LENGTH, "The shopkeeper now has %s.\r\n", temp1->short_description);
  send_to_char(Gbuf1, ch);
   
  snprintf(Gbuf1, MAX_STRING_LENGTH, "The shopkeeper gives you %s.\r\n", coin_stringv(sale));
  send_to_char(Gbuf1, ch);
  
  if(sale &&
     sale >= get_property("stats.sell.log", 500000))
      statuslog(ch->player.level,
        "&+LSALE:&n (%s&n) just sold [%d] (%s&n) for (%s&n) at [%d]!",
          GET_NAME(ch),
          OBJ_VNUM(temp1),
          temp1->short_description,
          coin_stringv(sale),
          (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);

      
  ADD_MONEY(ch, sale);
/*  if (shop_index[shop_nr].shop_is_roaming == 1) */
  SUB_MONEY(keeper, sale, 0);
/*
  if (!number(0, 5))
    shop_index[shop_nr].buy_percent -= .03;
  if (!number(0, 3))
    shop_index[shop_nr].sell_percent -= .03;
*/
  if( (get_obj_in_list(argm, keeper->carrying)) || (GET_ITEM_TYPE(temp1) == ITEM_TRASH) )
  {
    extract_obj(temp1, TRUE); // Prolly not an arti if type trash.
    temp1 = NULL;
  }
  else
  {
    obj_from_char(temp1);
    obj_to_char(temp1, keeper);
  }

  deleteShopKeeper(shop_nr);
  writeShopKeeper(keeper);
  return;
}

void shopping_value(char *arg, P_char ch, P_char keeper, int shop_nr)
{
  char     argm[MAX_INPUT_LENGTH];
  P_obj    temp1;
  char     Gbuf1[MAX_STRING_LENGTH];
  float    cost_factor;
  int      sale;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  one_argument(arg, argm);

  if (!(*argm))
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s What do you want me to valuate??", GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    return;
  }
  temp1 = get_selling_obj(ch, argm, keeper, shop_nr, TRUE, FALSE);
  if (!temp1)
    return;

  if(IS_SET(temp1->extra_flags, ITEM_NODROP) ||
     IS_ARTIFACT(temp1) ||
     isname("encrust", temp1->name))
      
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s I wouldn't feel comfortable buying that!", GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    
    if(IS_ARTIFACT(temp1))
          wizlog(56, "ARTIFACT: (%s) tried to value (%s) at (%s).",
            GET_NAME(ch), (temp1->short_description), GET_NAME(keeper));

    return;
  }

  cost_factor = (float) cha_app[STAT_INDEX(MAX(100, GET_C_CHA(ch)))].modifier;
  
  if(GET_RACE(ch) != GET_RACE(keeper))
    cost_factor = cost_factor / 2;

  cost_factor =
    shop_index[shop_nr].buy_percent * (1.0 + (cost_factor / 100.));
  
  if (cost_factor > shop_index[shop_nr].buy_percent)
    cost_factor = shop_index[shop_nr].buy_percent - .01;

  if (has_innate(ch, INNATE_BARTER)) {
   if (GET_C_CHA(ch) > number(0, 125)) {
    cost_factor -= .25;
   } else {
    cost_factor += .10;
   }
  }
/*
  if (temp1->type == ITEM_SCROLL || temp1->type == ITEM_POTION ||
      temp1->type == ITEM_STAFF || temp1->type == ITEM_WAND)
    cost_factor *= .1;
*/
  /* condition affects value too */
  sale = (int) (temp1->cost * cost_factor * MIN(100, temp1->condition) / 100);

  if (sale < 1)
    sale = 1;
  
  int temp;
  
  if( (temp = sql_shop_trophy(temp1)) > 1){
    int orig_sale = sale;

    sale -= (int) (get_property("shops.sellTrophyMod", 0.05) * temp * sale);
    sale = MAX( (int) (get_property("shops.sellMinPct", 0.10) * orig_sale), sale );
   
    if(sale < 1)
      sale = 1;

    snprintf(Gbuf1, MAX_STRING_LENGTH, "The shopkeeper says 'This item is rather common, it's not worth that much.'\r\n");
    send_to_char(Gbuf1, ch);
  }

  snprintf(Gbuf1, MAX_STRING_LENGTH, "The shopkeeper says 'I'll give you %s for that!'\r\n",
           coin_stringv(sale));
  send_to_char(Gbuf1, ch);

  return;
}

void shopping_peruse(char *arg, P_char ch, P_char keeper, int shop_nr)
{
  char     argm[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  P_obj    temp1, gem = NULL;
  int      found_obj, sale, i = 0;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;
  
  one_argument(arg, argm);
  if (!(*argm))
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s what do you want to peruse?", GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    return;
  };

  if (!(temp1 = get_obj_in_list_vis(ch, arg, keeper->carrying)))
  {
    if (atoi(argm))
    {
      for (temp1 = keeper->carrying; temp1; temp1 = temp1->next_content)
      {
        if ((CAN_SEE_OBJ(ch, temp1)) && (temp1->cost > 0))
	{
	  if (++i == atoi(argm))
	    break;
	}
      }
    }
    if (!temp1)
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
      do_tell(keeper, Gbuf1, 0);
      return;
    }
  }
  if (temp1->cost <= 0)
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
    do_tell(keeper, Gbuf1, 0);
    extract_obj(temp1, TRUE);  // Arti with no cost?
    return;
  }

  sale = (int)get_property("shops.peruse.cost", 1000.000);

  if (!IS_TRUSTED(ch))
  {
    if (!transact(ch, gem, keeper, sale))
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
      mobsay(keeper, Gbuf1);
      return;
    }
  }

  snprintf(Gbuf1, MAX_STRING_LENGTH, "You peruse %s.\r\n", temp1->short_description);
  send_to_char(Gbuf1, ch);

  spell_identify(60, ch, NULL, 0, 0, temp1);

  return;
}

void shopping_list(char *arg, P_char ch, P_char keeper, int shop_nr)
{
  P_obj    obj1;
  int      found_obj, temp;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];
  float    cost_factor;
  int      sale;
  char     descbuf[MAX_STRING_LENGTH];


  if( !(is_ok(keeper, ch, shop_nr)) )
    return;

  cost_factor = (float) cha_app[STAT_INDEX(MAX(100, GET_C_CHA(ch)))].modifier;
  if( GET_RACE(ch) != GET_RACE(keeper) )
    cost_factor = cost_factor * 2.;

  cost_factor = shop_index[shop_nr].sell_percent * (1.0 - (cost_factor / 100.));

  /* This is dumb for prices to randomly change when you type 'list' at a shop.
   * It would be proper to show the full price, then add "You get %s at a %d%% discount." when buying.
  if( has_innate(ch, INNATE_BARTER) )
  {
    if (GET_C_CHA(ch) > number(0, 125))
    {
      cost_factor -= .25;
    }
    else
    {
      cost_factor += .10;
    }
  }
  */

  if( cost_factor < shop_index[shop_nr].sell_percent )
    cost_factor = shop_index[shop_nr].sell_percent + .01;

  /*
   * New routine for listing goods by numbers. MIAX
   */
  temp = 0;
  strcpy(Gbuf1, "You can buy:\r\n");

  //Start sellling random eq code..
  /*
  int      x = 0;
  int      original_lvl = GET_LEVEL(keeper);

  (keeper)->player.level = 10 + number(0, 5);

  if (!IS_SET(keeper->only.npc->spec[2], MOB_INIT_RANDOM_EQ) &&
      !(mob_index[GET_RNUM(keeper)].qst_func) &&
      !IS_SET(keeper->specials.act, ACT_NO_BASH))
  {

    while (x < 6)
    {
      obj1 = create_random_eq_new(keeper, keeper, -1, -1);
      obj_to_char(obj1, keeper);
      x++;
    }
    SET_BIT(keeper->only.npc->spec[2], MOB_INIT_RANDOM_EQ);
  }
  (keeper)->player.level = original_lvl;
  */
  //End selling random eq code..
  found_obj = FALSE;
  if( keeper->carrying )
  {
    for( obj1 = keeper->carrying; obj1; obj1 = obj1->next_content )
    {
      if( IS_ARTIFACT(obj1) || isname("encrust", obj1->name) )
      {
        wizlog(56, "(%s) shopkeeper just destroyed (%s %d).", GET_NAME(keeper), obj1->short_description, OBJ_VNUM(obj1));
        extract_obj(obj1, TRUE); // Bye arti.
        continue;
      }

      snprintf(descbuf, MAX_STRING_LENGTH, "%s%s", obj1->short_description, item_condition(obj1) );

      if( CAN_SEE_OBJ(ch, obj1) && (obj1->cost > 0) )
      {
        temp++;
        found_obj = TRUE;

        sale = (int) (obj1->cost * cost_factor);

        // hook for epic bonus
        sale -= (int) (sale * get_epic_bonus(ch, EPIC_BONUS_SHOP));

        if( sale < 1 )
          sale = 1;
        if( obj1->type != ITEM_DRINKCON )
        {
          snprintf(Gbuf2, MAX_STRING_LENGTH, "%s for %s.\r\n", pad_ansi(descbuf, 45).c_str(), coin_stringv(sale));
        }
        else
        {
          if( obj1->value[1] )
            snprintf(Gbuf3, MAX_STRING_LENGTH, "%s of %s", descbuf, drinks[obj1->value[2]]);
          else
            snprintf(Gbuf3, MAX_STRING_LENGTH, "%s", descbuf ? descbuf : "");

          snprintf(Gbuf2, MAX_STRING_LENGTH, "%s for %s.\r\n", pad_ansi(Gbuf3, 45).c_str(), coin_stringv(sale));
        }

        snprintf(Gbuf4, MAX_STRING_LENGTH, "%2d) ", temp);
        CAP(Gbuf2);
        strcat(Gbuf4, Gbuf2);
        strcat(Gbuf1, Gbuf4);
      }
    }
  }

  if (!found_obj)
    strcat(Gbuf1, "Nothing!\r\n");

  send_to_char(Gbuf1, ch);
  return;
}

void shopping_kill(char *arg, P_char ch, P_char keeper, int shop_nr)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  switch (shop_index[shop_nr].temper2)
  {
  case 0:
    if(IS_PC(ch))
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "Don't ever try that again %s!", GET_NAME(ch));
      do_say(keeper, Gbuf1, -4);
    }
    return;

  case 1:
    if(IS_PC(ch))
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "Scram - %s, you midget!", GET_NAME(ch));
      do_say(keeper, Gbuf1, -4);
    }
    return;

  default:
    return;
  }

  return;
}


void shopping_repair(char *arg, P_char ch, P_char keeper, int shop_nr)
{
  char     buf[MAX_INPUT_LENGTH], argm[MAX_INPUT_LENGTH];
  int      cond, cost, ave;
  P_obj    obj, gem = NULL, wpn = NULL;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  one_argument(arg, argm);
  if (!(*argm))
  {
    snprintf(buf, MAX_INPUT_LENGTH, "%s what do you want repaired??", GET_NAME(ch));
    do_tell(keeper, buf, 0);
    return;
  }
#if 0
  if (!(obj = get_obj_in_list_vis(ch, argm, ch->carrying)))
  {
    send_to_char("Give what?\r\n", ch);
    return;
  }
#else
  obj = get_selling_obj(ch, argm, keeper, shop_nr, TRUE, TRUE);
  if (!obj)
    return;
#endif
  act("You give $p to $N.", TRUE, ch, obj, keeper, TO_CHAR);
  act("$n gives $p to $N.", TRUE, ch, obj, keeper, TO_ROOM);
  act("$N looks at $p.", TRUE, ch, obj, keeper, TO_CHAR);
  act("$N looks at $p.", TRUE, ch, obj, keeper, TO_ROOM);

  if( IS_SET(obj->extra_flags, ITEM_NOREPAIR) || IS_ARTIFACT(obj) )
  {
    act("$N fiddles with $p, shaking his head.", TRUE, ch, obj, keeper,
        TO_ROOM);
    act("$N fiddles with $p, shaking his head.", TRUE, ch, obj, keeper,
        TO_CHAR);
    mobsay(keeper, "The power within this item is far beyond my ability");
    act("$N gives you $p.", TRUE, ch, obj, keeper, TO_CHAR);
    act("$N gives $p to $n.", TRUE, ch, obj, keeper, TO_ROOM);
    return;
  }

  /* make all the correct tests to make sure that everything is kosher */
#ifndef NEW_COMBAT
  if (obj->type == ITEM_WEAPON)
  {
    wpn = read_object(obj_index[obj->R_num].virtual_number, VIRTUAL);
    // If the type of damge (hit/slash/bludgeon/etc) was changed, fix it.
    if (wpn->value[3] != obj->value[3])
    {
      obj->value[3] = wpn->value[3];
    }
    // If it wasn't originally a weapon, change the attack type to slash?
    // Why are we not checking this first and leaving the new attack type if
    //   it wasn't originally a weapon?
    if (wpn->type != ITEM_WEAPON)
    {
      obj->value[3] = MSG_SLASH;
    }
    extract_obj(wpn);
  }
  if( obj->condition > 0 )
  {
    if (obj->condition < 100)
    {
      /* get the value of the object */
      cost = obj->cost;
      /* divide by condition   */
      cost /= obj->condition;
      /* then cost = difference between current and max */
/*      cost *= (100 - obj->condition);*/
      cost *= 5;
//      if (GET_LEVEL(keeper) > 35)       /* super repair guy */
//        cost *= (5 / 4);
      cost = BOUNDED(100, cost, 100000);

      if ((GET_MONEY(ch) < cost) && !IS_TRUSTED(ch))
        gem = accept_gem_for_debt(ch, keeper, cost);

      if (!transact(ch, gem, keeper, cost))
      {
        snprintf(buf, MAX_INPUT_LENGTH, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
        mobsay(keeper, buf);
        return;
      }
      else
      {
        /* fix the armor */
        act("$N fiddles with $p.", TRUE, ch, obj, keeper, TO_ROOM);
        act("$N fiddles with $p.", TRUE, ch, obj, keeper, TO_CHAR);
        if (GET_LEVEL(keeper) > 35)
        {
          ave = BOUNDED(0, (100 + number(-9, 10) + (GET_LEVEL(keeper) - 35)), 125);
        }
        else
          ave = BOUNDED(0, MAX(obj->condition, 50 + obj->condition), 100);
        obj->condition = number(ave, 100);
        if (obj->condition < 100)
          obj->condition = 100;
        mobsay(keeper, "All fixed!");
      }
    }
    else
    {
      mobsay(keeper, "Eh? Sure looks fine to me.");
    }
    act("$N gives you $p.", TRUE, ch, obj, keeper, TO_CHAR);
    act("$N gives $p to $n.", TRUE, ch, obj, keeper, TO_ROOM);
    return;
  }
  else
  {
    mobsay(keeper, "This is pretty bad, I'm afraid I can't help you.");
    return;
  }
#else
  cond = BOUNDED(1, (int) (((float) obj->curr_sp / (float) obj->max_sp) * 10.0), 12);

  if (cond > 0)
  {
    if (obj->curr_sp < obj->max_sp)
    {
      /* get the value of the object */
      cost = obj->cost;
      /* divide by condition   */
      cost /= (cond * 10);
      /* then cost = difference between current and max */
/*      cost *= (100 - obj->condition);*/
      cost *= 3;
      if (GET_LEVEL(keeper) > 35)       /* super repair guy */
        cost = ((cost * 5) / 4);
      cost = BOUNDED(number(75, 125), cost, number(99500, 100500));
      if ((GET_MONEY(ch) < cost) && !IS_TRUSTED(ch))
        gem = accept_gem_for_debt(ch, keeper, cost);

      if (!transact(ch, gem, keeper, cost))
      {
        snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
        mobsay(keeper, buf);
        return;
      }
      else
      {
        /* fix the armor */
        act("$N fiddles with $p.", TRUE, ch, obj, keeper, TO_ROOM);
        act("$N fiddles with $p.", TRUE, ch, obj, keeper, TO_CHAR);
        obj->curr_sp = obj->max_sp;
        mobsay(keeper, "All fixed!");
      }
    }
    else
    {
      mobsay(keeper, "Eh? Sure looks fine to me.");
    }
    act("$N gives you $p.", TRUE, ch, obj, keeper, TO_CHAR);
    act("$N gives $p to $n.", TRUE, ch, obj, keeper, TO_ROOM);
    return;
  }
  else
  {
    mobsay(keeper, "This is pretty bad, I'm afraid I can't help you.");
    return;
  }
#endif
}
int shop_keeper(P_char keeper, P_char ch, int cmd, char *arg)
{
  char     argm[MAX_INPUT_LENGTH], victim_name[MAX_STRING_LENGTH];
  int      shop_nr, room;
  P_obj    item, next_item;

  if (!cmd)
    return FALSE;

  if (cmd == CMD_FORGE)
    return smith(keeper, ch, cmd, arg);

  if (cmd == CMD_DEATH)
  {
    room = (keeper->in_room >= 0 && keeper->in_room <= top_of_world) ? keeper->in_room : NOWHERE;
    if( room == NOWHERE )
    {
      room = real_room(keeper->specials.was_in_room);
    }
    for (item = keeper->carrying; item; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      if( IS_ARTIFACT(item) && room != NOWHERE )
      {
        obj_to_room(item, room);
      }
      else
      {
        extract_obj(item, TRUE); // Shopkeep dies with arti? :(
      }
    }
    return TRUE;
  }
  for (shop_nr = 0; shop_index[shop_nr].keeper != GET_RNUM(keeper); shop_nr++) ;

  if (SHOP_FUNC(shop_nr))       /* Check secondary function  */
    if ((SHOP_FUNC(shop_nr)) (keeper, ch, cmd, arg))
      return (TRUE);

  /*
   * Print opening message if shop uses new options

   if ((shop_index[shop_nr].shop_new_options == 1) &&
   (shop_index[shop_nr].flag == 0)) {
   if ((time_info.hour == shop_index[shop_nr].open1) ||
   (time_info.hour == shop_index[shop_nr].open2)) {
   snprintf(Gbuf1, MAX_STRING_LENGTH, "%s", shop_index[shop_nr].open_message);
   do_say (keeper, Gbuf1, 0);
   shop_index[shop_nr].flag = 1;
   shop_index[shop_nr].temptime = (time_info.hour + 1);
   } else if ((time_info.hour == shop_index[shop_nr].close1) ||
   (time_info.hour == shop_index[shop_nr].close2)) {
   snprintf(Gbuf1, MAX_STRING_LENGTH, "%s", shop_index[shop_nr].close_message);
   do_say (keeper, Gbuf1, 0);
   shop_index[shop_nr].flag = 1;
   shop_index[shop_nr].temptime = (time_info.hour + 1);
   }
   } */
  if (shop_index[shop_nr].flag == 1)
  {
    if (shop_index[shop_nr].temptime == 24)
      shop_index[shop_nr].temptime = 0;
    if (shop_index[shop_nr].temptime == time_info.hour)
      shop_index[shop_nr].flag = 0;
  }
  if ((cmd == CMD_STEAL))
  {
    /* Steal */
    arg = one_argument(arg, argm);
    one_argument(arg, victim_name);
    if (keeper == get_char_room(victim_name, ch->in_room))
    {
      snprintf(argm, MAX_INPUT_LENGTH, "shout Guards! Come arrest %s for being a thief!",
              CAN_SEE(keeper, ch) ? J_NAME(ch) : "Someone");
      command_interpreter(keeper, argm);
      if (CHAR_IN_TOWN(keeper))
      {
        justice_send_guards(NOWHERE, ch, MOB_SPEC_J_OUTCAST, 2);
      }
      else
        act(MSG_NO_STEAL_HERE, FALSE, ch, 0, keeper, TO_CHAR);
      return (TRUE);
    }
  }
  if( cmd == CMD_BUY )
    if ((ch->in_room == real_room(shop_index[shop_nr].in_room)) ||
        (shop_index[shop_nr].shop_is_roaming == 1))
    {
      shopping_buy(arg, ch, keeper, shop_nr);
      return (TRUE);
    }
  if (cmd == CMD_SELL)
    if ((ch->in_room == real_room(shop_index[shop_nr].in_room)) ||
        (shop_index[shop_nr].shop_is_roaming == 1))
    {
      shopping_sell(arg, ch, keeper, shop_nr);
      return (TRUE);
    }
  if (cmd == CMD_VALUE)
    if ((ch->in_room == real_room(shop_index[shop_nr].in_room)) ||
        (shop_index[shop_nr].shop_is_roaming == 1))
    {
      shopping_value(arg, ch, keeper, shop_nr);
      return (TRUE);
    }
  if (cmd == CMD_PERUSE)
    if ((ch->in_room == real_room(shop_index[shop_nr].in_room)) ||
        (shop_index[shop_nr].shop_is_roaming == 1))
    {
      shopping_peruse(arg, ch, keeper, shop_nr);
      return (TRUE);
    }
  if (cmd == CMD_LIST)
    if ((ch->in_room == real_room(shop_index[shop_nr].in_room)) ||
        (shop_index[shop_nr].shop_is_roaming == 1))
    {
      shopping_list(arg, ch, keeper, shop_nr);
      return (TRUE);
    }
  if (cmd == CMD_REPAIR)
    if ((ch->in_room == real_room(shop_index[shop_nr].in_room)) ||
        (shop_index[shop_nr].shop_is_roaming == 1))
    {
      shopping_repair(arg, ch, keeper, shop_nr);
      return (TRUE);
    }
  if (((cmd == CMD_CAST) || (cmd == CMD_RECITE) || (cmd == CMD_USE) ||
       (cmd == CMD_WILL)) &&
      (!shop_index[shop_nr].magic_allowed &&
       (shop_index[number_of_shops].shop_is_roaming ||
        (keeper->in_room == real_room(GET_HOME(keeper))))))
  {
    /*
     * Cast, recite, use
     */
    act("&+W$N tells you 'No magic in here!'.",
        FALSE, ch, 0, keeper, TO_CHAR);
    return TRUE;
  }
  else if (IS_AGG_CMD(cmd) && !shop_index[shop_nr].shop_killable)
  {
    /*
     * Kill or Hit
     */
    one_argument(arg, argm);
    if ((keeper == get_char_room(argm, ch->in_room)) ||
        ((cmd == CMD_HITALL) && isname("all", argm)))
    {
      shopping_kill(arg, ch, keeper, shop_nr);
      return (TRUE);
    }
  }
  return (FALSE);
}

// Returns 1 iff there's an error, and 0 if it's ok.
int add_to_list(struct shop_buy_data *list, int type, int *len, int *val)
{
  if( *val > 0 )
  {
    if( *len < MAX_TRADE )
    {
      if( type == LIST_PRODUCE )
      {
        *val = real_object(*val);
      }
      if( *val >= 0 )
      {
        BUY_TYPE(list[*len]) = *val;
        BUY_WORD(list[(*len)++]) = 0;
      }
      else
      {
        *val = 0;
      }
      return 0;
    }
    else
    {
      return 1;
    }
  }
  return 0;
}

int read_type_list(FILE * shop_f, struct shop_buy_data *list, int max)
{
  int   i, num, len = 0, error = 0;
  char *ptr, buf[MAX_STRING_LENGTH];

  bzero(buf, MAX_STRING_LENGTH);
  do
  {
    // Read a line.
    fgets( buf, MAX_STRING_LENGTH - 1, shop_f );
    // Put a 0 (?) at the end of the line/at the ;.
    if( (ptr = strchr(buf, ';')) != NULL )
    {
      *ptr = 0;
    }
    else
    {
      *(END_OF(buf) - 1) = 0;
    }
    num = 0;
    // Walk through the list of item_types (WEAPON/SCROLL/etc).
/* This seems to do nothing, as we list shop types by # not name ..
    for( i = 0; *item_types[i] != '\n'; i++ )
    {
      if( !strn_cmp(item_types[i], buf, strlen(item_types[i])) )
      {
        num = i;
        strcpy(buf, buf + strlen(item_types[i]));
        break;
      }
    }
*/

    ptr = buf;
    if( num == 0 )
    {
      sscanf(buf, "%d", &num);
      while( !isdigit(*ptr) )
      {
        ptr++;
      }
      while( isdigit(*ptr) )
      {
        ptr++;
      }
    }
    while( isspace(*ptr) )
    {
      ptr++;
    }
    while( isspace(*(END_OF(ptr) - 1)) )
    {
      *(END_OF(ptr) - 1) = 0;
    }
    error += add_to_list(list, LIST_TRADE, &len, &num);
    if (*ptr)
    {
      BUY_WORD(list[len - 1]) = str_dup(ptr);
    }
  }
  while( num > 0 );

  if( error )
  {
    snprintf(buf, MAX_STRING_LENGTH, "Raise MAX_TRADE constant in config.h to %d", len + error);
    logit(LOG_DEBUG, buf);
  }

  BUY_WORD(list[len]) = 0;
  BUY_TYPE(list[len++]) = 0;
  return len;
}

/*
 * Boot routine re-written for new options MIAX
 */

void boot_the_shops(void)
{
  char     tbuf, *buf;
  int      temp, tmp, count;
  float    t_buy, t_sell;
  FILE    *shop_f;
  static bool shop_end = TRUE;
  struct shop_buy_data list[MAX_SHOP_OBJ + 1];

  if (!(shop_f = fopen(SHOP_FILE, "r")))
  {
    perror("boot_the_shops: Error in boot shop - Could not open world.shp!\n");
    raise(SIGSEGV);
  }
  number_of_shops = 0;

  for (;;)
  {
    buf = fread_string(shop_f);

    if (*buf == '#')
    {                           /* a new shop */
//      fprintf(stderr, "boot_the_shops: Booting shop: '%s'.\r\n", buf);
      shop_end = FALSE;

      // first shop
      if (number_of_shops == 0)
      {
        CREATE(shop_index, shop_data, 1, MEM_TAG_SHOPDAT);
      }
      else
      {
        RECREATE(shop_index, shop_data, (number_of_shops + 1));
        bzero(&shop_index[number_of_shops], sizeof(struct shop_data));
      }

      fscanf(shop_f, "%c \n", &tbuf);
      // Determine weather this shop is in the NEW or OLD format
      if (tbuf == 'N')
      {
        shop_index[number_of_shops].shop_new_options = 1;
      }
      else
      {
        fprintf(stderr, "boot_the_shops: Old shop: '%s'!\r\n", buf);
        perror("Old shop exists!");
        raise(SIGSEGV);
      }

      for (count = 0; count < MAX_PROD; count++)
      {
        fscanf(shop_f, "%d \n", &temp);
        if( temp > 0 )
        {
          shop_index[number_of_shops].producing[count] = real_object(temp);
        }
        else
        {
          shop_index[number_of_shops].number_items_produced = count;
          break;
        }
      }
      // Here's a bug catch.. need to wipe 'extra' objects in sell list over max.
      if( count == MAX_PROD && temp > 0 )
      {
        shop_index[number_of_shops].number_items_produced = MAX_PROD;
        fscanf(shop_f, "%d \n", &temp);
        while( temp > 0 )
        {
          fprintf(stderr, "boot_the_shops: Shop '%s' has too many items: Item %d.\n\r", buf, temp);
          fscanf(shop_f, "%d \n", &temp);
        }
      }

      /* Load in the percentages that the shop will use to make a profit. */
      if( fscanf(shop_f, "%f \n", &t_buy) != 1 )
      {
        fprintf(stderr, "boot_the_shops: '%s' has bad t_buy!\r\n", buf);
        raise(SIGSEGV);
      }
      if( fscanf(shop_f, "%f \n", &t_sell) != 1 )
      {
        fprintf(stderr, "boot_the_shops: '%s' has bad t_sell!\r\n", buf);
        raise(SIGSEGV);
      }

      shop_index[number_of_shops].sell_percent = t_sell;
      shop_index[number_of_shops].buy_percent = t_buy;

      /*
       * boundary conditions, shops will buy for < 80%, and sell for < 1000%
       * and sell % must be higher than buy % -JAB
       */
      if (shop_index[number_of_shops].sell_percent > 10.0)
        shop_index[number_of_shops].sell_percent = 10.0;
      if (shop_index[number_of_shops].sell_percent < 1.0)
        shop_index[number_of_shops].sell_percent = 1.0;

      /*
       * with a buy% > .7, it is possible for high charisma players to MAKE
       * money buying/selling (yah, I know that's called commerce, but when the
       * places you can do it are in the same city, and the shops CAN'T go out
       * of business, it's called a bug). JAB
       */
      if (shop_index[number_of_shops].buy_percent > 0.8)
        shop_index[number_of_shops].buy_percent = .8;
      if (shop_index[number_of_shops].buy_percent < 0.05)
        shop_index[number_of_shops].buy_percent = .05;

      if (shop_index[number_of_shops].sell_percent <=
          shop_index[number_of_shops].buy_percent)
      {
        if (shop_index[number_of_shops].sell_percent > 7.0)
          shop_index[number_of_shops].buy_percent =
            shop_index[number_of_shops].sell_percent - 1.0;
        else
          shop_index[number_of_shops].sell_percent =
            shop_index[number_of_shops].buy_percent + .25;
      }
      /*
       * now, the reason for t_buy and t_sell, if we had to 'adjust' the
       * buy/sell %, then we yell about it
       */
      if ((shop_index[number_of_shops].sell_percent != t_sell) ||
          (shop_index[number_of_shops].buy_percent != t_buy))
      {
        logit(LOG_DEBUG, "Shop %s: Old buy/sell: %f/%f, New: %f/%f.", buf,
              t_buy, t_sell, shop_index[number_of_shops].buy_percent,
              shop_index[number_of_shops].sell_percent);
      }
      /*
       * Load in the types that this shop trades in
       */
      temp = read_type_list(shop_f, list, MAX_TRADE);
      CREATE(shop_index[number_of_shops].type, shop_buy_data, (unsigned) temp, MEM_TAG_SHOPBUY);
      for( count = 0; count < temp; count++ )
      {
        SHOP_BUYTYPE(number_of_shops, count) = (byte) BUY_TYPE(list[count]);
        SHOP_BUYWORD(number_of_shops, count) = BUY_WORD(list[count]);
      }
      shop_index[number_of_shops].number_types_traded = count;


      shop_index[number_of_shops].no_such_item1 = fread_string(shop_f);
      shop_index[number_of_shops].no_such_item2 = fread_string(shop_f);
      shop_index[number_of_shops].do_not_buy = fread_string(shop_f);
      shop_index[number_of_shops].missing_cash1 = fread_string(shop_f);
      shop_index[number_of_shops].missing_cash2 = fread_string(shop_f);
      shop_index[number_of_shops].message_buy = fread_string(shop_f);
      shop_index[number_of_shops].message_sell = fread_string(shop_f);
      fscanf(shop_f, "%d \n", &tmp);
      shop_index[number_of_shops].temper1 = tmp;
      tmp = 0;
      fscanf(shop_f, "%d \n", &tmp);
      shop_index[number_of_shops].temper2 = tmp;
      fscanf(shop_f, "%d \n", &shop_index[number_of_shops].keeper);

      //logit(LOG_DEBUG, "Loaded shop '%d' list..", shop_index[number_of_shops].keeper); For debugging.
      shop_index[number_of_shops].keeper =
        real_mobile(shop_index[number_of_shops].keeper);

      fscanf(shop_f, "%d \n", &shop_index[number_of_shops].with_who);
      fscanf(shop_f, "%d \n", &shop_index[number_of_shops].in_room);
      fscanf(shop_f, "%d \n", &shop_index[number_of_shops].open1);
      fscanf(shop_f, "%d \n", &shop_index[number_of_shops].close1);
      fscanf(shop_f, "%d \n", &shop_index[number_of_shops].open2);
      fscanf(shop_f, "%d \n", &shop_index[number_of_shops].close2);

      fscanf(shop_f, "%c \n", &tbuf);
      if (tbuf == 'Y')
      {
        shop_index[number_of_shops].shop_is_roaming = 1;
      }
      else
      {
        shop_index[number_of_shops].shop_is_roaming = 0;
      }

      /* * Does the shopkeeper stop players from using magic? */
      fscanf(shop_f, "%c \n", &tbuf);
      if (tbuf == 'Y' && !shop_index[number_of_shops].shop_is_roaming)
        shop_index[number_of_shops].magic_allowed = 1;
      else
        shop_index[number_of_shops].magic_allowed = 0;

      /* * Does the shopkeeper stop players from attacking? */
      fscanf(shop_f, "%c \n", &tbuf);
      if (tbuf == 'Y')
        shop_index[number_of_shops].shop_killable = 1;
      else
        shop_index[number_of_shops].shop_killable = 0;

      /* * Messages we see when shop open and closes */
      shop_index[number_of_shops].open_message = fread_string(shop_f);
      shop_index[number_of_shops].close_message = fread_string(shop_f);

      /* * What hometown (if any) is this shop in? (0 for none) */
      fscanf(shop_f, "%d \n", &shop_index[number_of_shops].shop_hometown);

      /* * What catagory does this shopkeeper fall into for random socials? */
      fscanf(shop_f, "%d \n",
             &shop_index[number_of_shops].social_action_types);

      /* * What race is shopkeeper and do they trade with outsiders */
      fscanf(shop_f, "%c \n", &tbuf);
      if (tbuf == 'Y')
        shop_index[number_of_shops].racist = 1;
      else
        shop_index[number_of_shops].racist = 0;
      tmp = 0;
      fscanf(shop_f, "%d \n", &tmp);
      shop_index[number_of_shops].shopkeeper_race = tmp;
      buf = fread_string(shop_f);
      shop_index[number_of_shops].racist_message = buf;

      fscanf(shop_f, "%c \n", &tbuf);
      if (tbuf == 'X')
        shop_end = TRUE;


      number_of_shops++;

      /*
       * End load of New shop, ready to bounce to next shop or quit if EOF
       */
    }
    else if (*buf == '$')
    {                           /* EOF */
      fprintf(stderr, "-- Shop loading complete.\r\n");
      break;
    }
  }

  if (shop_end == FALSE)
  {
    fprintf(stderr,
            "WARNING! The shop file has an error in it! (boot stopped)\r\n");
    logit(LOG_STATUS,
          "WARNING! The shop file has an error in it! (boot stopped)");
    raise(SIGSEGV);
  }
  fclose(shop_f);
}

void assign_the_shopkeepers(void)
{
  int      temp1;

  for (temp1 = 0; temp1 < number_of_shops; temp1++)
  {
    if (mob_index[shop_index[temp1].keeper].func.mob)
      SHOP_FUNC(temp1) = mob_index[shop_index[temp1].keeper].func.mob;
    mob_index[shop_index[temp1].keeper].func.mob = shop_keeper;
  }
}

P_obj accept_gem_for_debt(P_char ch, P_char keeper, int debt)
{
  int      value = 2;
  P_obj    cobj = 0, obj;
  char     buf[MAX_STRING_LENGTH];
  return NULL;
  obj = ch->carrying;
  while (obj)
  {
    if (obj->cost > value && obj->type == ITEM_TREASURE
        && !IS_SET(obj->extra2_flags, ITEM2_MAGIC))
    {
      cobj = obj;
      value = cobj->cost;
    }
    if (OBJ_INSIDE(obj))
      if (obj->next_content)
        obj = obj->next_content;
      else
        obj = obj->loc.inside->next_content;
    else
      obj = obj->next_content;
  }

  if (!value || !cobj)
    return NULL;

  if (value > debt)
  {
    snprintf(buf, MAX_STRING_LENGTH, "$N accepts $p in leiu of your %s debt.",
            coin_stringv(debt));
    act(buf, FALSE, ch, cobj, keeper, TO_CHAR);
    return cobj;
  }
  return NULL;
}


/* if merchandise, accepts that as payment towards value, giving change.
   otherwise, simply takes cash. returns true if everything worked... */

bool transact(P_char from, P_obj merchandise, P_char to, int value)
{
  int      i, gem_value, change;
  char     Gbuf4[MAX_STRING_LENGTH];
 
//DISABLED
 merchandise = 0;
  if (from->in_room == to->in_room)
  {
    if (merchandise)
    {
      if (OBJ_WORN(merchandise))
      {
        for (i = 0; i < MAX_WEAR; i++)
          if (from->equipment[i] == merchandise)
          {
            unequip_char(from, i);
            break;
          }
        if (OBJ_WORN(merchandise))
        {
          logit(LOG_EXIT, "assert: couldn't unequip in transact()");
          raise(SIGSEGV);
        }
      }
      else
        obj_from_char(merchandise);
      obj_to_char(merchandise, to);
      gem_value = merchandise->cost * 3 / 4;
      change = gem_value - value;
      if (change > 0)
        ADD_MONEY(from, change);
      snprintf(Gbuf4, MAX_STRING_LENGTH, "You barter your debt and receive %s change.\r\n\r\n",
              coin_stringv(change));
      send_to_char(Gbuf4, from);
      snprintf(Gbuf4, MAX_STRING_LENGTH, "%s gives you %s as payment.\r\n\r\n", from->player.name,
              merchandise->short_description);
      send_to_char(Gbuf4, to);
      snprintf(Gbuf4, MAX_STRING_LENGTH, "%s pays a debt to %s with %s.\r\n\r\n",
              from->player.name, to->player.name,
              merchandise->short_description);
      send_to_room_except_two(Gbuf4, from->in_room, from, to);
      return TRUE;
    }
    else if (GET_MONEY(from) >= value)
    {
      if (SUB_MONEY(from, value, 0) > -1)
        ADD_MONEY(to, value);
      else
        return FALSE;
      snprintf(Gbuf4, MAX_STRING_LENGTH, "You give %s %s.\r\n\r\n",
              ((IS_PC(to)) ? GET_NAME(to) : to->player.short_descr),
              coin_stringv(value));
      send_to_char(Gbuf4, from);
      act("$n gives $N some money.", TRUE, from, 0, to, TO_NOTVICT);
      act("$N pays your price.", FALSE, to, 0, from, TO_CHAR);
      return TRUE;
    }
    else
    {
      snprintf(Gbuf4, MAX_STRING_LENGTH, "%s does not have the funds for the exchange.\r\n\r\n",
              ((IS_PC(from)) ? GET_NAME(from) : from->player.short_descr));
      send_to_char(Gbuf4, to);
      send_to_char("You do not have the funds for the exchange.\r\n\r\n",
                   from);
      return FALSE;
    }
  }
  return FALSE;
}

// Stats objects for sale, like in locker code.
// Must be duplicated because do_lore is limited to inventory.
void shopping_stat( P_char ch, P_char keeper, char *arg, int cmd )
{
   int i = 0;
   P_obj obj;

   if( !ch || !keeper )
      return;

   for( obj = keeper->carrying; obj; obj = obj->next_content )
      if( CAN_SEE_OBJ( ch, obj ) && ( obj->cost > 0 ) )
         if( ++i == atoi(arg) )
         { 
            lore_item( ch, obj );
            return;
         }
   mobsay( keeper, "I do not sell that item." );
}
