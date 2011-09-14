// epic_bonus.c
//
// created by: Venthix 9-11-11
//
// Selectable bonuses granted based on epics recieved within the last week.
// Choosing a new bonus will reset your counter, you can only accumulate epics towards
// the bonus chosen, if a new bonus is chosen your accumulated epics will be reset to
// the date and time you chose.

#include <string.h>

#include "config.h"
#include "structs.h"
#include "prototypes.h"
#include "utils.h"
#include "epic_bonus.h"
#include "sql.h"

struct epic_bonus_data ebd[] = {
  {EPIC_BONUS_NONE, "none", "No Epic Bonus"},
  {EPIC_BONUS_CARGO, "cargo", "Cargo Discount"},
  {EPIC_BONUS_SHOP, "shop", "Shop Discount"},
  {EPIC_BONUS_EXP, "exp", "Experience Bonus"},
  {EPIC_BONUS_EPIC_POINT, "epic", "Epic Points Bonus"},
  {EPIC_BONUS_HEALTH, "health", "Health Regen Bonus"},
  {EPIC_BONUS_MOVES, "moves", "Movement Regen Bonus"},
  {0},
};

// command interpreter for epic_bonus
void do_epic_bonus(P_char ch, char *arg, int cmd)
{
  char buff[MAX_STRING_LENGTH];
  int type = 0;

  // Clear "bonus" from args
  arg = one_argument(arg, buff);
  // Get first argument
  arg = one_argument(arg, buff);

  for (int i = 1; ebd[i].type; i++)
  {
    if (!str_cmp(buff, ebd[i].name))
    {
      type = ebd[i].type;
      break;
    }
  }

  if (type || !str_cmp(buff, "none"))
  {
    epic_bonus_set(ch, type);
    return;
  }
  else
  {
    epic_bonus_help(ch);
    return;
  }
}

// Display help on epic bonus
void epic_bonus_help(P_char ch)
{
  EpicBonusData ebdata;
  if (!get_epic_bonus_data(ch, &ebdata))
  {
    ebdata.type = EPIC_BONUS_NONE;
  }

  send_to_char("&+WEpic Bonus:&n\r\n\r\n", ch);
  send_to_char_f(ch, "&+cYou are currently benefiting from the &+C%s &+c(&+C%.2f%&+c).\r\n\r\n", ebd[ebdata.type].description, get_epic_bonus(ch, ebdata.type)*100);
  send_to_char("&+CYou can choose from the following bonuses:&n\r\n", ch);
  send_to_char_f(ch, "&+C%10s &+c(&+CMAX: %3d%%&+c) &+w- &+c%s\r\n", ebd[0].name, (int)(get_epic_bonus_max(0)*100), ebd[0].description);
  for (int i = 1; ebd[i].type; i++)
  {
    send_to_char_f(ch, "&+C%10s &+c(&+CMax: %3d%%&+c) &+w- &+c%s\r\n", ebd[i].name, (int)(get_epic_bonus_max(i)*100), ebd[i].description);
  }
  send_to_char("\r\n", ch);
  return;
}

// Set the new epic bonus on the character, and reset their timer to now()
void epic_bonus_set(P_char ch, int type)
{
  EpicBonusData ebdata;
  if (!get_epic_bonus_data(ch, &ebdata))
  {
    qry("INSERT INTO epic_bonus VALUES ('%i', '%i', now())", GET_PID(ch), type);
  }
  else
  {
    qry("UPDATE epic_bonus SET type = '%i', time = now() WHERE pid = '%i'", type, GET_PID(ch));
  }

  send_to_char_f(ch, "Your epic bonus has been changed to %s.\r\n", ebd[type].description);
  send_to_char("&+RPlease be aware your timer has been reset to now.&n\r\n", ch);

  return;
}

float get_epic_bonus_max(int id)
{
  char buff[MAX_STRING_LENGTH];

  int i;
  for (i = 1; ebd[i].type; i++);
  
  if (id < 0 || id >= i)
  {
    debug("get_epic_bonus_max(): called with invalid id: %d", id);
    return 0;
  }

  sprintf(buff, "epic.bonus.max.%s", ebd[id].name);

  return get_property(buff, 0.000);
}

bool get_epic_bonus_data(P_char ch, EpicBonusData *ebdata)
{
  if (!ebdata)
    return false;

  if (!qry("SELECT * FROM epic_bonus where pid = '%i'", GET_PID(ch)))
    return false;

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return false;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  ebdata->pid = atoi(row[0]);
  ebdata->type = atoi(row[1]);
  ebdata->time = row[2];

  mysql_free_result(res);

  return true;
}

float get_epic_bonus(P_char ch, int type)
{
  if (!IS_PC(ch))
    return 0;

  EpicBonusData ebdata;
  if (!get_epic_bonus_data(ch, &ebdata))
    return 0;

  if (ebdata.type != type)
    return 0;

  int accum_epics = 0;

  if (!qry("SELECT SUM(epics) FROM epic_gain WHERE pid = '%i' AND time > DATE_SUB(curdate(), INTERVAL 7 DAY) AND time >'%s'", GET_PID(ch), ebdata.time))
    return 0;

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return 0;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  if (!row[0])
  {
    mysql_free_result(res);
    return 0;
  }

  accum_epics = atoi(row[0]);

  mysql_free_result(res);

  float epic_max = get_property("epic.bonus.epic.cap", 1.000);
  return ((float)MIN(accum_epics, epic_max) / epic_max)*get_epic_bonus_max(type);
}
