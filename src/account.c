/*************************************************************
* account.c
*************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include "account.h"
#include "comm.h"
#include "mm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "structs.h"
#include "spells.h"
#include "utils.h"
#include <math.h>

// External Stuff
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern struct time_info_data time_info;
extern const char *dirs[];
extern P_desc descriptor_list;
extern P_char character_list;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;

struct acct_entry *account_list = NULL;

#define ACCT_SERIAL                1
#define ACCOUNT_EMAIL_DB                "Accounts/email.db"

bool     account_exists(const char *, char *);


void select_accountname(P_desc d, char *arg)
{
  char     tmp_name[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  P_desc   t_d = NULL;
  char     acct_in_game = 0;

  for (; isspace(*arg); arg++) ;
  if (!*arg)
  {
    close_socket(d);
    return;
  }

  if (_parse_name(arg, tmp_name))
  {
    SEND_TO_Q("Illegal account name, please try another.\r\n", d);
    SEND_TO_Q("Account Name: ", d);
    return;
  }

  *tmp_name = toupper(*tmp_name);

  if (!d->account)
  {
    d->account = allocate_account();
    if (!d->account)
    {
      SEND_TO_Q
        ("ERROR:  Could not allocate a new account, notify an immortal!\r\n",
         d);
      statuslog(56,
                "&+RALERT&n:  Could not allocate memory for a new account!");
      STATE(d) = CON_FLUSH;
      return;
    }
  }

  d->account->acct_name = str_dup(tmp_name);

  if (account_exists("Accounts", tmp_name))
  {
    if (read_account(d->account) == -1)
    {
      SEND_TO_Q
        ("There is an error with your account, please notify an immortal!\r\n",
         d);
      statuslog(56, "&+RALERT&n:  Account corrupt: %s", tmp_name);
      d->account = free_account(d->account);
      STATE(d) = CON_FLUSH;
      return;
    }

    SEND_TO_Q("Please enter your password: ", d);
    echo_off(d);
    STATE(d) = CON_GET_ACCT_PASSWD;
    return;
  }

  verify_account_name(d, NULL);
  STATE(d) = CON_VERIFY_NEW_ACCT_NAME;
  return;
}

void get_account_password(P_desc d, char *arg)
{
  // skip whitespace
  for (; isspace(*arg); arg++) ;

  if (!arg)
  {
    d->account = free_account(d->account);
    close_socket(d);
    return;
  }

  if (*arg == -1)
  {
    if (arg[1] != '0' && arg[2] != '0')
    {
      if (arg[3] == '0')
      {                         /* Password on next read  */
        return;
      }
      else
      {                         /* Password available */
        arg = arg + 3;
      }
    }
    else
      d->account = free_account(d->account);
    close_socket(d);
  }

  if (!arg)
  {
    d->account = free_account(d->account);
    close_socket(d);
    return;
  }

  if (strn_cmp
      (CRYPT(arg, d->account->acct_password), d->account->acct_password, 10))
  {
    SEND_TO_Q("Invalid Password ... disconnecting\r\n", d);
    d->account = free_account(d->account);
    STATE(d) = CON_FLUSH;
    return;
  }
  else
  {
    echo_on(d);
    if (is_account_confirmed(d))
    {
      display_account_menu(d, NULL);
      update_account_iplist(d);
      STATE(d) = CON_DISPLAY_ACCT_MENU;
      return;
    }
    else
    {
      confirm_account(d, NULL);
      STATE(d) = CON_CONFIRM_ACCT;
      return;
    }
  }
}

void display_account_menu(P_desc d, char *arg)
{
  if (!arg)
  {
    SEND_TO_Q("\r\n\r\n1) Select a character to play\r\n", d);
    SEND_TO_Q("2) Create a new character\r\n", d);
    SEND_TO_Q("3) Delete a character\r\n", d);
    SEND_TO_Q("\r\n", d);
    SEND_TO_Q("4) Display account information\r\n", d);
    SEND_TO_Q("5) Change registered email address\r\n", d);
    SEND_TO_Q("6) Change account password\r\n", d);
    SEND_TO_Q("7) Delete this account\r\n", d);
    SEND_TO_Q("\r\n", d);
    SEND_TO_Q("99) Disconnect from this account\r\n", d);
    SEND_TO_Q("----------------------------------\r\n", d);
    SEND_TO_Q("Please select an option: ", d);
    return;
  }
  switch (atoi(arg))
  {
  case 99:
    STATE(d) = CON_FLUSH;
    SEND_TO_Q("\r\n\r\nThank you for playing!\r\n", d);
    write_account(d->account);
    break;

  case 1:
    STATE(d) = CON_ACCT_SELECT_CHAR;
    account_select_char(d, NULL);
    break;

  case 2:
    SEND_TO_Q("Enter your new name:  ", d);
    STATE(d) = CON_ACCT_NEW_CHAR_NAME;
    break;

  case 3:
    STATE(d) = CON_ACCT_DELETE_CHAR;
    account_delete_char(d, NULL);
    break;

  case 4:
    STATE(d) = CON_ACCT_DISPLAY_INFO;
    account_display_info(d, NULL);
    break;

  case 5:
    STATE(d) = CON_ACCT_CHANGE_EMAIL;
    get_new_account_email(d, NULL);
    break;

  case 6:
    STATE(d) = CON_ACCT_CHANGE_PASSWD;
    get_new_account_password(d, NULL);
    break;

  case 7:
    STATE(d) = CON_ACCT_DELETE_ACCT;
    delete_account(d, NULL);
    break;

  default:
    SEND_TO_Q("Invalid Selection, please try again.\r\n", d);
    display_account_menu(d, NULL);
    break;
  }
}

void confirm_account(P_desc d, char *arg)
{
  if (!arg)
  {
    SEND_TO_Q("Please enter the confirmation code received in your email:  ",
              d);
    return;
  }

  if (str_cmp(arg, d->account->acct_confirmation))
  {
    SEND_TO_Q("Invalid confirmation code ... disconnecting.\r\n", d);
    d->account = free_account(d->account);
    STATE(d) = CON_FLUSH;
  }
  else
  {
    SEND_TO_Q("Thank you for confirming your account!\r\n", d);
    d->account->acct_confirmed = 1;
    if (-1 == write_account(d->account))
    {
      SEND_TO_Q
        ("Oh no, I couldn't write your account information to disk, notify a god!\r\n",
         d);
      d->account = free_account(d->account);
      STATE(d) = CON_FLUSH;
    }
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    write_account(d->account);
  }

}

void verify_account_name(P_desc d, char *arg)
{
  char     buf[1024];

  if (!arg)
  {
    *d->account->acct_name = toupper(*d->account->acct_name);
    sprintf(buf, "You chose the name %s, is this correct? (Y/N)  ",
            d->account->acct_name);
    SEND_TO_Q(buf, d);
    return;
  }

  if ((arg[0] == 'y') || (arg[0] == 'Y'))
  {
    SEND_TO_Q("Confirmed...\r\n", d);
    STATE(d) = CON_GET_NEW_ACCT_EMAIL;
    get_new_account_email(d, NULL);
    return;
  }
  else if ((arg[0] == 'n') || (arg[0] == 'N'))
  {
    SEND_TO_Q("Ok, what then?\r\n", d);
    d->account = free_account(d->account);
    STATE(d) = CON_GET_ACCT_NAME;
    return;
  }
  else
  {
    SEND_TO_Q("Invalid choice...\r\n", d);
    verify_account_name(d, NULL);
  }
}



void get_new_account_email(P_desc d, char *arg)
{
  if (!arg)
  {
    SEND_TO_Q("\r\nPlease enter your email address:  ", d);
    return;
  }
  for (; isspace(*arg); arg++) ;

  d->account->acct_email = str_dup(arg);
  STATE(d) = CON_VERIFY_NEW_ACCT_EMAIL;
  verify_new_account_email(d, NULL);
  return;
}

void verify_new_account_email(P_desc d, char *arg)
{
  char     buf[1024];

  if (!arg)
  {
    sprintf(buf, "\r\nYou entered %s, is this correct?  (Y/N)",
            d->account->acct_email);
    SEND_TO_Q(buf, d);
    return;
  }
  if ((arg[0] == 'y') || (arg[0] == 'Y'))
  {
    SEND_TO_Q("Confirmed...\r\n", d);
    if (d->account->acct_confirmed == 0)
    {
      STATE(d) = CON_GET_NEW_ACCT_PASSWD;
      get_new_account_password(d, NULL);
    }
    else
    {
      STATE(d) = CON_DISPLAY_ACCT_MENU;
      display_account_menu(d, NULL);
      write_account(d->account);
    }
    return;
  }
  else if ((arg[0] == 'n') || (arg[0] == 'N'))
  {
    SEND_TO_Q("Ok, what then?\r\n", d);
    FREE(d->account->acct_email);
    d->account->acct_email = NULL;
    STATE(d) = CON_GET_NEW_ACCT_EMAIL;
    return;
  }
  else
  {
    SEND_TO_Q("Invalid choice...\r\n", d);
    verify_new_account_email(d, NULL);
  }

}

void get_new_account_password(P_desc d, char *arg)
{
  char     password[256];

  if (!arg)
  {
    echo_on(d);
    SEND_TO_Q("Please enter your password:  ", d);
    echo_off(d);
    return;
  }
  echo_on(d);

  for (; isspace(*arg); arg++) ;

  if (!arg)
  {
    SEND_TO_Q("Invalid Password, try again.\r\n", d);
    get_new_account_password(d, NULL);
    return;
  }

  if (!valid_password(d, arg))
  {
    get_new_account_password(d, NULL);
    return;
  }

  strncpy(password, CRYPT(arg, d->account->acct_name), 10);
  *(password + 10) = '\0';
  d->account->acct_password = str_dup(password);
  STATE(d) = CON_VERIFY_NEW_ACCT_PASSWD;
  verify_new_account_password(d, NULL);
  return;
}

void verify_new_account_password(P_desc d, char *arg)
{
  if (!arg)
  {
    echo_on(d);
    SEND_TO_Q("Please verify your password:  ", d);
    echo_off(d);
    return;
  }
  echo_on(d);

  if (strn_cmp
      (CRYPT(arg, d->account->acct_password), d->account->acct_password, 10))
  {
    SEND_TO_Q("Passwords do not match!\r\n", d);
    get_new_account_password(d, NULL);
    FREE(d->account->acct_password);
    d->account->acct_password = NULL;
    STATE(d) = CON_GET_NEW_ACCT_PASSWD;
    return;
  }

  if (d->account->acct_confirmed == 0)
  {
    STATE(d) = CON_VERIFY_NEW_ACCT_INFO;
    verify_new_account_information(d, NULL);
  }
  else
  {
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    write_account(d->account);
  }
  return;
}

void verify_new_account_information(P_desc d, char *arg)
{
  if (!arg)
  {
    display_account_information(d);
    SEND_TO_Q("\r\nIs this information correct?  (Y/N) ", d);
    return;
  }
  if ((arg[0] == 'y') || (arg[0] == 'Y'))
  {
    SEND_TO_Q
      ("You will receive a confimation code in your email.\r\nYou must confirm your account before using it.\r\n",
       d);
    generate_account_confirmation_code(d, NULL);
    write_account(d->account);
    d->account = free_account(d->account);
    STATE(d) = CON_FLUSH;
    return;
  }
  else if ((arg[0] == 'n') || (arg[0] == 'N'))
  {
    SEND_TO_Q("Ok, starting over!\r\n", d);
    d->account = free_account(d->account);
    STATE(d) = CON_GET_ACCT_NAME;
    SEND_TO_Q("Please enter your account name: ", d);
    return;
  }
  else
  {
    SEND_TO_Q("Invalid choice...\r\n", d);
    verify_new_account_information(d, NULL);
  }
}

void update_account_iplist(P_desc d)
{
  P_acct   acct = d->account;
  struct acct_ip *ip = NULL;

  ip = find_ip_entry(acct, d);

  if (!ip)
  {
    add_ip_entry(acct, d);
    return;
  }
  else
  {
    ip->count++;
    write_account(acct);
    return;
  }
}

struct acct_ip *find_ip_entry(P_acct acct, P_desc d)
{
  struct acct_ip *a = acct->acct_unique_ips;

  if (!a)
    return NULL;

  while (a)
  {
    if (!strcmp(a->hostname, d->host))
      return a;
    else
      a = a->next;
  }

  return NULL;
}

void add_ip_entry(P_acct acct, P_desc d)
{
  char     host[512];
  struct acct_ip *a = NULL;
  struct acct_ip *b = NULL;

  sprintf(host, "%s", d->host);

  CREATE(a, acct_ip, 1, MEM_TAG_OTHER);

  a->hostname = str_dup(host);
  a->count = 1;
  a->ip_address = str_dup(host);
  acct->num_ips++;
  a->next = acct->acct_unique_ips;
  acct->acct_unique_ips = a;
  return;
}

void account_select_char(P_desc d, char *arg)
{
  struct acct_chars *c = NULL;
  P_char   ch = NULL;

  if (!arg)
  {
    display_character_list(d);
    return;
  }

  c = find_char_in_list(d->account->acct_character_list, arg);

  if (!c)
  {
    SEND_TO_Q("Sorry, I couldn't find that character!\r\n", d);
    display_account_menu(d, NULL);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    return;
  }

  if (!can_connect(c, d))
  {
    SEND_TO_Q("Sorry, you can't play that character right now!\r\n", d);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return;
  }

  if (is_char_in_game(c, d))
  {
    return;
  }

  ch = load_char_into_game(c, d);

  if (!ch)
  {
    SEND_TO_Q("Sorry, I couldn't load that character!\r\n", d);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return;
  }

  // gotta put code in here so they can PLAY the character.
  if (IS_TRUSTED(ch))
    SEND_TO_Q(wizmotd, d);
  else
    SEND_TO_Q(motd, d);

  echo_on(d);
  STATE(d) = CON_PLYNG;
  d->character = ch;
  enter_game(d);
  d->prompt_mode = 1;

  return;

}

void display_character_list(P_desc d)
{
  struct acct_chars *c = d->account->acct_character_list;
  char     buf[256];

  if (!c)
  {
    SEND_TO_Q("You currently don't have any characters.\r\n", d);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return;
  }

  SEND_TO_Q("You have the following characters available:\r\n", d);
  while (c)
  {
    sprintf(buf, "%s\r\n", c->charname);
    SEND_TO_Q(buf, d);
    c = c->next;
  }
  SEND_TO_Q("\r\n\r\nWhich character would you like to play?  ", d);
}




int can_connect(struct acct_chars *c, P_desc d)
{
  int      current_time = time(NULL);

  if (c->blocked)
    return 0;

  if ((c->racewar == ACCT_IMMORTAL))
    return 1;

  if ((c->racewar == ACCT_GOOD) &&
      (current_time < (d->account->acct_evil + 3600)))
    return 0;

  if ((c->racewar == ACCT_EVIL) &&
      (current_time < (d->account->acct_good + 3600)))
    return 0;

  return 1;
}


int is_char_in_game(struct acct_chars *c, P_desc d)
{
  P_desc   k = descriptor_list;
  P_desc   x = NULL;
  P_char   ch = character_list;

  for (; k; k = k->next)
  {
    if ((k != d) && k->character && GET_NAME(k->character) &&
        !strcasecmp(GET_NAME(k->character), c->charname))
    {
      // ok, same character, take over the descriptor
      d->character = k->character;
      d->character->desc = d;
      close_socket(k);
      SEND_TO_Q("Overriding old connection...\r\n", d);
    }
  }

  for (; ch; ch = ch->next)
  {
    if (IS_PC(ch) && !ch->desc && GET_NAME(ch) &&
        !strcasecmp(GET_NAME(ch), c->charname))
    {
      echo_on(d);
      SEND_TO_Q("Reconnecting...\r\n", d);
      act("$n has reconnected.", TRUE, ch, 0, 0, TO_ROOM);
      d->character = ch;
      ch->desc = d;
      sql_update_playerIP(ch);
      ch->specials.timer = 0;
      STATE(d) = CON_PLYNG;

      logit(LOG_COMM, "%s [%s@%s] has reconnected.", GET_NAME(d->character),
            d->login, d->host);
      loginlog(d->character->player.level, "%s [%s@%s] has reconnected.",
               GET_NAME(d->character), d->login, d->host);
      sql_log(ch, CONNECT_LOG, "Reconnected");

#if 0
      /* panic, lets check for spellcast events and nuke them, hopefully allowing a release from
         spellcast bug */
      if (IS_AFFECTED2(d->character, AFF2_CASTING))
      {
        P_event  ev;

        LOOP_EVENTS(ev, (d->character)->events)
          if (ev->type == EVENT_SPELLCAST)
        {
          statuslog(AVATAR, "Spellcast bug on %s aborted",
                    GET_NAME(d->character));
          StopCasting(d->character);
        }
      }
      /* if they were morph'ed when they lost link, put them
         back... */
#endif
      if (IS_SET(ch->specials.act, PLR_MORPH))
      {
        if (!ch->only.pc->switched || !IS_MORPH(ch->only.pc->switched) ||
            /*              (ch != ((P_char)
               ch->only.pc->switched->only.npc->memory))) */
            (ch != ch->only.pc->switched->only.npc->orig_char))
        {
          logit(LOG_EXIT,
                "Something fucked while trying to reconnect linkless morph");
          raise(SIGSEGV);
        }
        d->original = ch;
        d->character = ch->only.pc->switched;
        d->character->desc = d;
        ch->desc = NULL;
      }
      return 1;
    }
  }
  return 0;
}

struct acct_chars *find_char_in_list(struct acct_chars *list, char *arg)
{
  struct acct_chars *c = NULL;

  if (!list)
    return NULL;

  while (list)
  {
    if (!strcasecmp(list->charname, arg))
      return list;
    else
      list = list->next;
  }
  return NULL;
}

P_char load_char_into_game(struct acct_chars * c, P_desc d)
{

  P_char   player = NULL;
  int      status = 0;

  player = (P_char) mm_get(dead_mob_pool);

  clear_char(player);

  if (!dead_pconly_pool)
    dead_pconly_pool = mm_create("PC_ONLY",
                                 sizeof(struct pc_only_data),
                                 offsetof(struct pc_only_data, switched),
                                 mm_find_best_chunk(sizeof
                                                    (struct pc_only_data), 10,
                                                    25));

  player->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
  player->desc = d;

  setCharPhysTypeInfo(player);
  status = restoreCharOnly(player, c->charname);

  if (status == -1)
  {
    SEND_TO_Q("Couldn't find that pfile!\r\n", d);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return NULL;
  }
  else if (status == -2)
  {
    SEND_TO_Q("There was an error reading your pfile!\r\n", d);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return NULL;
  }
  else
  {
    d->rtype = status;
    return player;
  }
}

void account_new_char(P_desc d, char *arg)
{
  SEND_TO_Q("Enter your new name:  ", d);
  STATE(d) = CON_ACCT_NEW_CHAR_NAME;
  return;
}

void account_new_char_name(P_desc d, char *arg)
{
  P_char   player = NULL;
  char     tmp_name[1024];

  if (!arg)
  {
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return;
  }

  for (; isspace(*arg); arg++) ;

  if (_parse_name(arg, tmp_name))
  {
    SEND_TO_Q("Illegal account name, please try another.\r\n", d);
    SEND_TO_Q("Account Name: ", d);
    return;
  }

  arg = tmp_name;

  if (!account_exists(SAVE_DIR, arg) &&
      account_exists(BADNAME_DIR, arg))
  {
    SEND_TO_Q
      ("That name has been declined before, and would be now too!\r\nName:",
       d);
    return;
  }
  if (account_exists(SAVE_DIR, arg))
  {
    SEND_TO_Q("Name is in use already. Please enter new name.\r\nName:", d);
    return;
  }
  else if (account_exists(BADNAME_DIR, arg))
  {
    SEND_TO_Q
      ("That name has been declined before, and would be now too!\r\nName:",
       d);
    return;
  }
  if (IS_SET(game_locked, LOCK_CREATE))
  {
    SEND_TO_Q("Game is currently not allowing creation of new characters.\r\n"
              "Please use an existing character, or try again later.\r\n\r\n",
              d);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return;
  }
  else if (bannedsite(d->host, 1))
  {
    SEND_TO_Q
      ("New characters have been banned from your site. If you want the ban lifted\r\n"
       "mail duris@duris.org with a _LENGTHY_ explanation about\r\n"
       "why, or who could have forced us to ban the site in the first place.\r\n"
       "          - The Management \r\n\r\n", d);
    banlog(AVATAR, "&+yNew Character reject from %s, banned.", d->host);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return;
  }
  else if ((game_locked & LOCK_CONNECTIONS) ||
           ((game_locked & LOCK_MAX_PLAYERS) &&
            (number_of_players() >= MAX_PLAYERS_BEFORE_LOCK)))
  {
    SEND_TO_Q("Game is temporarily full.  Please try again later.\r\n", d);
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return;
  }

  // Ok, we got this far, so it's ok to make a character, yay!
  if (d->character)
  {
    player = d->character;
  }
  else
  {
    player = (P_char) mm_get(dead_mob_pool);

    clear_char(player);

    if (!dead_pconly_pool)
      dead_pconly_pool = mm_create("PC_ONLY",
                                   sizeof(struct pc_only_data),
                                   offsetof(struct pc_only_data, switched),
                                   mm_find_best_chunk(sizeof
                                                      (struct pc_only_data),
                                                      10, 25));

    player->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
    player->desc = d;

    setCharPhysTypeInfo(player);

    d->character = player;
  }

  strcpy(d->character->only.pc->pwd, d->account->acct_password);
  d->character->player.name = str_dup(arg);
  *d->character->player.name = toupper(*d->character->player.name);
  SEND_TO_Q("You chose the name ", d);
  SEND_TO_Q(d->character->player.name, d);
  SEND_TO_Q("  Is this correct?  (Y/N)  ", d);
  STATE(d) = CON_NMECNF;
  return;
}

void add_char_to_account(P_desc d)
{
  P_char   player = d->character;
  struct acct_chars *c = NULL;

  CREATE(c, acct_chars, 1, MEM_TAG_OTHER);

  c->charname = str_dup(player->player.name);
  c->count = 1;
  c->last = time(NULL);
  c->blocked = 0;
  if (EVIL_RACE(player))
    c->racewar = ACCT_EVIL;
  else
    c->racewar = ACCT_GOOD;
  c->next = d->account->acct_character_list;
  d->account->acct_character_list = c;
  write_account(d->account);
}


void account_delete_char(P_desc d, char *arg)
{
  P_char   ch = NULL;
  struct acct_chars *c = NULL;
  char     buf[256];

  if (!arg)
  {
    SEND_TO_Q("Which character would you like to delete?  ", d);
    return;
  }
  if (!strcasecmp(arg, "y") || !strcasecmp(arg, "yes"))
  {
    if (!d->character)
    {
      SEND_TO_Q("\r\n Odd, couldn't delete that char.\r\n", d);
      return;
    }
    SEND_TO_Q("\r\nDeleting character...\r\n\r\n", d);
    statuslog(d->character->player.level, "%s deleted %sself (%s@%s).",
              GET_NAME(d->character),
              GET_SEX(d->character) == SEX_MALE ? "him" : "her", d->login,
              d->host);
    logit(LOG_PLAYER, "%s deleted %sself (%s@%s).", GET_NAME(d->character),
          GET_SEX(d->character) == SEX_MALE ? "him" : "her", d->login,
          d->host);
    deleteCharacter(d->character);
    STATE(d) = CON_FLUSH;
//              display_account_menu(d, NULL);
    return;
  }
  else if (!strcasecmp(arg, "n"))
  {
    STATE(d) = CON_DISPLAY_ACCT_MENU;
    display_account_menu(d, NULL);
    return;
  }
  else
  {
    c = find_char_in_list(d->account->acct_character_list, arg);
    if (c)
      ch = load_char_into_game(c, d);

    if (!ch)
    {
      SEND_TO_Q("Couldn't find that character!", d);
      STATE(d) = CON_DISPLAY_ACCT_MENU;
      display_account_menu(d, NULL);
      return;
    }

    sprintf(buf, "Are you sure you want to delete %s?  ", c->charname);
    SEND_TO_Q(buf, d);
    d->character = ch;
    return;
  }
}

void remove_char_from_list(P_acct acct, char *ch)
{
  struct acct_chars *c = NULL;
  struct acct_chars *d = NULL;

  c = acct->acct_character_list;
  if (!strcasecmp(ch, c->charname))
  {
    acct->acct_character_list = c->next;
    FREE(c->charname);
    FREE(c);
    write_account(acct);
    return;
  }

  while (c)
  {
    if (!strcasecmp(ch, c->charname))
    {
      d->next = c->next;
      FREE(c->charname);
      FREE(c);
      write_account(acct);
      return;
    }
    d = c;
    c = c->next;
  }
}

void account_display_info(P_desc d, char *arg)
{
  display_account_information(d);
  STATE(d) = CON_DISPLAY_ACCT_MENU;
  display_account_menu(d, NULL);
  return;
}

void delete_account(P_desc d, char *arg)
{

}

void verify_delete_account(P_desc d, char *arg)
{

}

int read_account(P_acct acct)   // returns -1 if error, 1 if no errors
{
  FILE    *f = NULL;
  char     name[4096], filename[4096], buf[4096], *ptr = NULL;
  int      serial = 0;


  sprintf(name, "%s", acct->acct_name);
  ptr = name;

  for (; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  sprintf(buf, "Accounts/%c/%s", (*name), name);
  logit(LOG_FILE, "Loading Account %s in %s.", name, buf);

  f = fopen(buf, "r");

  if (!f)
  {
    logit(LOG_FILE, "Couldn't open Account file: %s", buf);
    return -1;
  }

  fscanf(f, "%d\n", &serial);
  fscanf(f, "%s\n", buf);
  acct->acct_name = str_dup(buf);
  fscanf(f, "%s\n", buf);
  acct->acct_email = str_dup(buf);
  fscanf(f, "%s\n", buf);
  acct->acct_password = str_dup(buf);
  fscanf(f, "%s\n", buf);
  acct->acct_confirmation = str_dup(buf);

  read_unique_ip(acct, f);
  read_character_list(acct, f);

  fscanf(f, "%d\n", &acct->acct_blocked);
  fscanf(f, "%d\n", &acct->acct_confirmed);
  fscanf(f, "%d\n", &acct->acct_confirmation_sent);

  fscanf(f, "%li\n", &acct->acct_last);
  fscanf(f, "%li\n", &acct->acct_good);
  fscanf(f, "%li\n", &acct->acct_evil);
  fscanf(f, "%li\n", &acct->acct_flags1);
  fscanf(f, "%li\n", &acct->acct_flags2);
  fscanf(f, "%li\n", &acct->acct_flags3);
  fscanf(f, "%li\n", &acct->acct_flags4);


}

int write_account(P_acct acct)  // returns -1 if error, 1 if no errors
{
  FILE    *f = NULL;
  char     buf[4096], name[4096], *ptr = NULL;
  struct stat statbuf;
  P_desc   d = NULL;

  if (!acct)
    return -1;

  sprintf(name, "%s", acct->acct_name);

  ptr = name;

  for (; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  sprintf(buf, "Accounts/%c/%s", (*name), name);
  logit(LOG_FILE, "Saving Account %s in %s.", name, buf);
  sprintf(name, "%s.bak", buf);


  if (stat(buf, &statbuf) == 0)
  {
    if (rename(buf, name) == -1)
    {
      logit(LOG_FILE, "Problem with player save files directory!\n");
      wizlog(AVATAR, "&+R&-LPANIC!&N  Error backing up account for %s!",
             acct->acct_name);
      return -1;
    }
  }

  f = fopen(buf, "w");
  if (!f)
  {
    logit(LOG_FILE, "Fopen failed while creating account file: %s\n", buf);
    return -1;
  }

  fprintf(f, "%d\n", ACCT_SERIAL);
  fprintf(f, "%s\n", acct->acct_name);
  fprintf(f, "%s\n", acct->acct_email);
  fprintf(f, "%s\n", acct->acct_password);
  fprintf(f, "%s\n", acct->acct_confirmation);

  write_unique_ip(acct, f);
  write_character_list(acct, f);

  fprintf(f, "%d\n", acct->acct_blocked);
  fprintf(f, "%d\n", acct->acct_confirmed);
  fprintf(f, "%d\n", acct->acct_confirmation_sent);

  fprintf(f, "%li\n", acct->acct_last);
  fprintf(f, "%li\n", acct->acct_good);
  fprintf(f, "%li\n", acct->acct_evil);
  fprintf(f, "%li\n", acct->acct_flags1);
  fprintf(f, "%li\n", acct->acct_flags2);
  fprintf(f, "%li\n", acct->acct_flags3);
  fprintf(f, "%li\n", acct->acct_flags4);


  fprintf(f, "###\n");
  fclose(f);
  for (d = descriptor_list; d; d = d->next)
  {
    if (d->account && !strcasecmp(acct->acct_name, d->account->acct_name))
      read_account(d->account);
  }
  return 1;
}

void write_unique_ip(P_acct acct, FILE * f)
{
  int      count = 0;
  struct acct_ip *c = NULL;

  c = acct->acct_unique_ips;
  if (!c)
  {
    fprintf(f, "0\n");
    return;
  }

  while (c)
  {
    count++;
    c = c->next;
  }

  fprintf(f, "%d\n", count);
  c = acct->acct_unique_ips;
  while (c)
  {
    fprintf(f, "%s\n%s\n%li\n", c->hostname, c->ip_address, c->count);
    c = c->next;
  }
}

void read_unique_ip(P_acct acct, FILE * f)
{
  int      count = 0;
  int      i;
  struct acct_ip *c = NULL;
  struct acct_ip *d = NULL;
  char     buf[256];

  fscanf(f, "%d\n", &count);
  if (count == 0)
    return;

  for (i = 0; i < count; i++)
  {
    CREATE(c, acct_ip, 1, MEM_TAG_OTHER);

    fscanf(f, "%s\n", buf);
    c->hostname = str_dup(buf);
    fscanf(f, "%s\n", buf);
    c->ip_address = str_dup(buf);
    fscanf(f, "%d\n", &c->count);
    if (i == 0)
      acct->acct_unique_ips = c;
    if (d)
      d->next = c;
    d = c;
  }
}

void write_character_list(P_acct acct, FILE * f)
{
  int      count = 0;
  struct acct_chars *c = NULL;

  c = acct->acct_character_list;
  if (!c)
  {
    fprintf(f, "0\n");
    return;
  }

  while (c)
  {
    count++;
    c = c->next;
  }

  fprintf(f, "%d\n", count);
  c = acct->acct_character_list;
  while (c)
  {
    fprintf(f, "%s\n%li %li %d %d\n", c->charname, c->count, c->last,
            c->blocked, c->racewar);
    c = c->next;
  }
}

void read_character_list(P_acct acct, FILE * f)
{
  int      count = 0;
  int      i;
  struct acct_chars *c = NULL;
  struct acct_chars *d = NULL;
  char     buf[256];

  fscanf(f, "%d\n", &count);
  if (count == 0)
    return;

  for (i = 0; i < count; i++)
  {
    CREATE(c, acct_chars, 1, MEM_TAG_OTHER);

    fscanf(f, "%s\n", buf);
    c->charname = str_dup(buf);
    fscanf(f, "%d %d %d %d\n", &c->count, &c->last, &c->blocked, &c->racewar);
    if (i == 0)
      acct->acct_character_list = c;
    if (d)
      d->next = c;
    d = c;
  }

}





void generate_account_confirmation_code(P_desc d, char *arg)
{
  char     a[256], b[256];
  FILE    *f = NULL;

  sprintf(a, "%d%d", rand(), time(NULL));
  sprintf(b, "%s", CRYPT(a, d->account->acct_name));

  sprintf(a, "/tmp/%s.confirmation", d->account->acct_name);
  f = fopen(a, "w");
  if (!f)
  {
    ereglog(AVATAR, "Couldn't open account confirmation temp file!");
    SEND_TO_Q
      ("Sorry, there was an error emailing your confirmation code!\r\n", d);
    d->account = free_account(d->account);
    STATE(d) = CON_FLUSH;
    return;
  }

  fprintf(f, "  *** Duris Account Confirmation Code ***\n\n\n");
  fprintf(f, "Your account confirmation code is:  %s\n", b);
  fclose(f);
  d->account->acct_confirmation = str_dup(b);
  write_account(d->account);


  sprintf(b, "mail -s \"%s\" %s < %s", "Duris Account Confirmation",
          d->account->acct_email, a);
  system(b);
  unlink(a);

  f = fopen(ACCOUNT_EMAIL_DB, "a");
  if (!f)
  {
    statuslog(56, "Couldn't open Email DB!");
  }
  else
  {
    fprintf(f, "%s\n", d->account->acct_email);
    fclose(f);
  }

  return;
}

void display_account_information(P_desc d)
{
  char     buffer[4096];

  sprintf(buffer, "Account Name:              %s\r\n", d->account->acct_name);
  SEND_TO_Q(buffer, d);
  sprintf(buffer, "Email Address:             %s\r\n",
          d->account->acct_email);
  SEND_TO_Q(buffer, d);

}

char is_account_confirmed(P_desc d)
{
  if (d->account && d->account->acct_confirmed)
    return 1;
  else
    return 0;
}

void clear_account(P_acct acct)
{
  struct acct_ip *curr_ip = NULL;
  struct acct_ip *next_ip = NULL;
  struct acct_chars *curr_char = NULL;
  struct acct_chars *next_char = NULL;

  acct->acct_name = check_and_clear(acct->acct_name);
  acct->acct_email = check_and_clear(acct->acct_email);
  acct->acct_password = check_and_clear(acct->acct_password);
  acct->acct_confirmation = check_and_clear(acct->acct_confirmation);

  if (acct->acct_unique_ips)
  {
    for (curr_ip = acct->acct_unique_ips; curr_ip; curr_ip = next_ip)
    {
      curr_ip->hostname = check_and_clear(curr_ip->hostname);
      curr_ip->ip_address = check_and_clear(curr_ip->ip_address);
      next_ip = curr_ip->next;
      FREE(curr_ip);
    }
    acct->acct_unique_ips = NULL;
  }

  if (acct->acct_character_list)
  {
    for (curr_char = acct->acct_character_list; curr_char;
         curr_char = next_char)
    {
      curr_char->charname = check_and_clear(curr_char->charname);
      next_char = curr_char->next;
      FREE(curr_char);
    }
    acct->acct_character_list = NULL;
  }

  acct->acct_blocked = 0;
  acct->acct_confirmed = 0;
  acct->acct_confirmation_sent = 0;

  acct->acct_last = 0;
  acct->acct_good = 0;
  acct->acct_evil = 0;

  acct->acct_flags1 = 0;
  acct->acct_flags2 = 0;
  acct->acct_flags3 = 0;
  acct->acct_flags4 = 0;

  acct->next = NULL;
}

char    *check_and_clear(char *ptr)
{
  if (ptr)
    FREE(ptr);
  return NULL;
}

P_acct free_account(P_acct acct)
{
  if (acct)
  {
    remove_account_from_list(acct);
    clear_account(acct);
    FREE(acct);
  }
  return NULL;
}

P_acct allocate_account(void)
{
  P_acct   acct = NULL;


  CREATE(acct, acct_entry, 1, MEM_TAG_OTHER);

  if (!acct)
    raise(SIGSEGV);

  if (acct)
  {
    clear_account(acct);
    add_account_to_list(acct);
  }

  return acct;
}

void add_account_to_list(P_acct acct)
{
  P_acct   i = NULL;

  if (!acct)
    return;

  if (account_list == NULL)
  {
    account_list = acct;
    return;
  }
  else
  {
    acct->next = account_list;
    account_list = acct;
  }
}

void remove_account_from_list(P_acct acct)
{
  P_acct   i = NULL;

  if (!acct)
    return;

  i = account_list;
  if (i == acct)
  {
    account_list = i->next;
    return;
  }
  while (i && (i->next != acct))
  {
    i = i->next;
  }

  if (i)
  {
    i->next = acct->next;
  }
  return;
}

bool account_exists(const char *dir, char *name)
{
  char     buf[256], *buff;
  struct stat statbuf;
  char     Gbuf1[MAX_STRING_LENGTH];

  strcpy(buf, name);
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  sprintf(Gbuf1, "%s/%c/%s", dir, buf[0], buf);
  if (stat(Gbuf1, &statbuf) != 0)
  {
    sprintf(Gbuf1, "%s/%c/%s", dir, buf[0], name);
    if (stat(Gbuf1, &statbuf) != 0)
      return FALSE;
  }
  return TRUE;
}
