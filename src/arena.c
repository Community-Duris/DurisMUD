/*************************************************************
* arena.c
* Arena subsystem stuff
*************************************************************/

#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "arenadef.h"
#include "comm.h"
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

struct arena_data arena;
static char buf[MAX_INPUT_LENGTH];

//Prototypes
void     arena_char_spawn(P_char ch);

const int max_team_size[] = {
  MAX_GOODIE_TEAM,
  MAX_EVIL_TEAM,
  MAX_UNDEAD_TEAM
};

const char *game_type[] = {
  "Capture the Flag",
  "Jail Break",
  "King of the Hill",
  "Team DeathMatch",
  "IT",
  "Deathmatch",
  "\n"
};

const char *arena_stage[] = {
  "&+GAccepting players&N",
  "&+rPre Match prep&N",
  "&=LRMatch in progress&N",
  "&=LRMatch in progress&N",
  "&+LPost match aftermath&N",
  "&+LPost match aftermath&N"
};
const char *death_type[] = {
  "Winner Takes ALL",
  "Even Trade",
  "Entrance Fee, Winner takes Pot",
  "Free Play",
  "\n"
};

const char *map_name[] = {
  "Opposing Castles",
  "\n"
};

extern const int arena_hometown_location[];
const int arena_hometown_location[] = {
  6075,                         //Goodie
  29700,                        //EVIL
  -1
};

const int map_spec[MAX_MAP][5] = {
/* start, end, respawn1, respawn2, respawn3 */
  {67906, 67986, 67907, 67983, -1}
};

const int arena_start_room[MAX_RACES][2] = {
/* Meeting room, vault */
  {67904, 67905},
  {67902, 67903},
  {67900, 67901}
};

int loadmap(int map)
{

  if ((map < 0) || (map > (MAX_MAP - 1)))
    return FALSE;

  arena.map.num = map;
  arena.map.startroom = map_spec[map][0];
  arena.map.endroom = map_spec[map][1];
  arena.map.spawn[GOODIE] = map_spec[map][2];
  arena.map.spawn[EVIL] = map_spec[map][3];
  arena.map.spawn[UNDEAD] = map_spec[map][4];
  return TRUE;
}

void send_to_arena(char *msg, int race)
{
  int      i, j, room;
  P_char   ch;
  P_desc   d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->connected == CON_PLAYING)
    {
      if (CHAR_IN_ARENA(d->character))
      {
        switch (race)
        {
        case GOODIE:
          if (IS_RACEWAR_GOOD(d->character))
            send_to_char(msg, d->character);
          break;
        case EVIL:
          if (IS_RACEWAR_EVIL(d->character))
            send_to_char(msg, d->character);
          break;
        case UNDEAD:
          if (IS_RACEWAR_UNDEAD(d->character))
            send_to_char(msg, d->character);
          break;
        default:
          send_to_char(msg, d->character);
          break;
        }
      }
      if (race == -1)
      {
        for (i = 0; i < MAX_RACES; i++)
        {
          if (arena_hometown_location[i] != -1)
          {
            if (real_room(arena_hometown_location[i]) ==
                d->character->in_room)
            {
              send_to_char(msg, d->character);
            }
          }
        }
      }
    }
  }
}

char    *arena_death_msg(P_obj p_weapon)
{
  return "killed";
}

int arena_id(P_char ch)
{
  int      i, j;

  for (i = 0; i < MAX_RACES; i++)
  {
    for (j = 0; j < MAX_TEAM; j++)
    {
      if (arena.team[i].player[j].ch == ch)
      {
        return (i * 100) + j;
      }
    }
  }
  return -1;
}

int arena_team(P_char ch)
{
  if (arena_id(ch) == -1)
    return -1;

  return (int) (arena_id(ch) / 100);
}

int arena_player(P_char ch)
{
  int      i;

  if (arena_id(ch) == -1)
    return -1;

  i = arena_id(ch);

  i -= (int) (i / 100) * 100;

  return i;
}

void init_teams()
{
  int      i, j;

  for (i = 0; i < MAX_RACES; i++)
  {
    for (j = 0; j < MAX_TEAM; j++)
    {
      arena.team[i].player[j].ch = NULL;
      arena.team[i].player[j].frags = 0;
      arena.team[i].player[j].flags = 0;
      arena.team[i].player[j].lives = 0;
    }
    arena.team[i].flags = 0;
    arena.team[i].score = 0;
  }
}

int arena_team_count(int team)
{
  int      i, count = 0;

  if ((team < MAX_RACES) && (team > -1))
  {
    for (i = 0; i < MAX_TEAM; i++)
    {
      if (!IS_SET(arena.team[team].player[i].flags, PLAYER_DEAD) &&
          arena.team[team].player[i].ch != NULL)
        count++;
    }
  }
  return count;
}

void initialize_arena(void)
{
  if (DEFAULT_ENABLED)
    SET_BIT(arena.flags, FLAG_ENABLED);
  if (DEFAULT_SEENAME)
    SET_BIT(arena.flags, FLAG_SEENAME);

  loadmap(DEFAULT_MAP);
  arena.type = DEFAULT_TYPE;
  arena.deathmode = DEFAULT_DEATH;
  arena.timer[0] = DEFAULT_TIMER_OPEN;
  init_teams();
}

void players_to_map()
{
  P_char   ch, next_ch = NULL;
  int      i, j, k;

  for (i = 0; i < MAX_RACES; i++)
  {
    k = 0;
    for (j = 0; j < 2; j++)
    {
      for (ch = world[real_room(arena_start_room[i][j])].people; ch;
           ch = next_ch)
      {
        if (ch == NULL)
          break;

        next_ch = ch->next_in_room;
        if (!IS_TRUSTED(ch) && IS_PC(ch))
        {
          arena.team[i].player[k].ch = ch;
          arena.team[i].player[k].frags = 0;
          if (!IS_SET(arena.flags, FLAG_TOURNAMENT))
            arena.team[i].player[k].lives = DEFAULT_LIVES;
          arena.team[i].player[k].flags = 0;
          snprintf(arena.team[i].player[k].name, MAX_STRING_LENGTH, "%s", GET_NAME(ch));
          k++;
        }
        char_from_room(ch);
        arena_char_spawn(ch);
      }
    }
  }
}

void players_from_map()
{
  int      i, j;
  P_char   ch, next_ch = NULL;

  for (i = arena.map.startroom; i <= arena.map.endroom; i++)
  {
    for (ch = world[real_room(i)].people; ch; ch = next_ch)
    {
      if (ch == NULL)
        continue;
      next_ch = ch->next_in_room;

      if (!IS_TRUSTED(ch))
      {
        char_from_room(ch);
        char_to_room(ch, real_room(arena_start_room[arena_team(ch)][0]), -1);
      }
    }
  }


}

void show_scores_to_char(P_char ch, int team)
{
  int      i;
  char     tmp_buf[MAX_STRING_LENGTH];
  P_char   tch;

  if (team == -1)
  {
    show_scores_to_char(ch, GOODIE);
    show_scores_to_char(ch, EVIL);
    return;
  }
  if (team == GOODIE)
  {
    if (ch == NULL)
    {
      snprintf(tmp_buf, MAX_STRING_LENGTH, "&+YTeam Goodie: &+W%d&N\r\n",
              arena.team[GOODIE].score);
      send_to_arena(tmp_buf, -1);

      for (i = 0; i < MAX_GOODIE_TEAM; i++)
      {
        tch = arena.team[GOODIE].player[i].ch;
        if (tch != NULL)
        {
          snprintf(tmp_buf, MAX_STRING_LENGTH,
                  "&+W%-20s&N &+LFrags: &+W%-3d&N &+WLives Remaining: %d&N\r\n",
                  arena.team[GOODIE].player[i].name,
                  arena.team[GOODIE].player[i].frags,
                  arena.team[GOODIE].player[i].lives);
          send_to_arena(tmp_buf, -1);
        }
      }
    }
    else
    {
      snprintf(tmp_buf, MAX_STRING_LENGTH, "&+YTeam Goodie: &+W%d&N\r\n",
              arena.team[GOODIE].score);
      send_to_char(tmp_buf, ch);

      for (i = 0; i < MAX_GOODIE_TEAM; i++)
      {
        tch = arena.team[GOODIE].player[i].ch;
        if (tch != NULL)
        {
          snprintf(tmp_buf, MAX_STRING_LENGTH,
                  "&+W%-20s&N &+LFrags: &+W%-3d&N &+WLives Remaining: %d&N\r\n",
                  arena.team[GOODIE].player[i].name,
                  arena.team[GOODIE].player[i].frags,
                  arena.team[GOODIE].player[i].lives);
          send_to_char(tmp_buf, ch);
        }
      }
    }
  }
  if (team == EVIL)
  {
    if (ch == NULL)
    {
      snprintf(tmp_buf, MAX_STRING_LENGTH, "&+rTeam Evil: &+W%d&N\r\n", arena.team[EVIL].score);
      send_to_arena(tmp_buf, -1);

      for (i = 0; i < MAX_EVIL_TEAM; i++)
      {
        tch = arena.team[EVIL].player[i].ch;
        if (tch != NULL)
        {
          snprintf(tmp_buf, MAX_STRING_LENGTH,
                  "&+W%-20s&N &+LFrags: &+W%-3d&N &+WLives Remaining: %d&N\r\n",
                  arena.team[EVIL].player[i].name,
                  arena.team[EVIL].player[i].frags,
                  arena.team[EVIL].player[i].lives);
          send_to_arena(tmp_buf, -1);
        }
      }
    }
    else
    {
      snprintf(tmp_buf, MAX_STRING_LENGTH, "&+rTeam Evil: &+W%d&N\r\n", arena.team[EVIL].score);
      send_to_char(tmp_buf, ch);

      for (i = 0; i < MAX_EVIL_TEAM; i++)
      {
        tch = arena.team[EVIL].player[i].ch;
        if (tch != NULL)
        {
          snprintf(tmp_buf, MAX_STRING_LENGTH,
                  "&+W%-20s&N &+LFrags: &+W%-3d&N &+WLives Remaining: %d&N\r\n",
                  arena.team[EVIL].player[i].name,
                  arena.team[EVIL].player[i].frags,
                  arena.team[EVIL].player[i].lives);
          send_to_char(tmp_buf, ch);
        }
      }
    }
  }
}

void show_scores(int team)
{
  if (team == -1)
  {
    show_scores(GOODIE);
    show_scores(EVIL);
    return;
  }
  if (team == EVIL)
  {
    show_scores_to_char(NULL, EVIL);
    return;
  }
  if (team == GOODIE)
  {
    show_scores_to_char(NULL, GOODIE);
    return;
  }
}


void extract_from_arena()
{
  P_desc   d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->character != NULL)
    {
      if (CHAR_IN_ARENA(d->character))
      {
        char_from_room(d->character);
        char_to_room(d->character,
                     real_room(arena_hometown_location
                               [arena_team(d->character)]), -1);
      }
    }
  }
}
void show_stats_to_char(P_char ch)
{
  P_char   tch;
  char     tbuf[MAX_STRING_LENGTH];
  int      countg, counte;

  if (ch == NULL)
  {

  }
  else
  {
    send_to_char("&+YCurrent Arena Statistics:&N\r\n", ch);
    snprintf(tbuf, MAX_STRING_LENGTH, "&+LMap: &+W%s&N\r\n", map_name[arena.map.num]);
    send_to_char(tbuf, ch);
    snprintf(tbuf, MAX_STRING_LENGTH, "&+LGame Type: &+W%-20s &+LDeath Mode: &+W%s\r\n&N",
            game_type[arena.type], death_type[arena.deathmode]);
    send_to_char(tbuf, ch);
    countg = 0;
    counte = 0;
    for (tch = world[real_room0(arena_start_room[GOODIE][0])].people; tch;
         tch = tch->next_in_room)
    {
      if (!IS_TRUSTED(tch) && IS_PC(tch))
      {
        countg++;
      }
    }
    for (tch = world[real_room0(arena_start_room[EVIL][0])].people; tch;
         tch = tch->next_in_room)
    {
      if (!IS_TRUSTED(tch) && IS_PC(tch))
      {
        counte++;
      }
    }
    snprintf(tbuf, MAX_STRING_LENGTH,
            "&+LPeople in arena - &+YGoodies: &+W%-2d &+rEvils: &+W%-2d&N\r\n",
            countg, counte);
    send_to_char(tbuf, ch);
    snprintf(tbuf, MAX_STRING_LENGTH, "&+LArena phase: %-30s &+LTime left: &+W%d&N\r\n",
            arena_stage[arena.stage], arena.timer[0]);
    send_to_char(tbuf, ch);
  }
}

int arenaobj_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  char     name[MAX_INPUT_LENGTH], tbuf[MAX_STRING_LENGTH];
  P_obj    obj_entered;
  P_char   temp;
  int      race, i, j, count = 0;

  /* check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_ENTER)
  {
    one_argument(arg, name);

    if (!ch)
      return FALSE;

    obj_entered = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
    if (obj_entered != obj)
      return (FALSE);

    if (!obj)
      return FALSE;

    if (!IS_SET(arena.flags, FLAG_ENABLED) && !CHAR_IN_ARENA(ch))
    {
      send_to_char("&+LArena currently not open, sorry.&N\r\n", ch);
      return TRUE;
    }

    if (arena.stage != STAGE_OPEN && !CHAR_IN_ARENA(ch))
    {
      send_to_char
        ("&+YMatch currently in progress, wait for the next one.&N\r\n", ch);
      return TRUE;
    }

    if (IS_RACEWAR_EVIL(ch))
      race = EVIL;
    else if (IS_RACEWAR_GOOD(ch))
      race = GOODIE;
    else if (IS_RACEWAR_UNDEAD(ch))
      race = UNDEAD;

    for (i = 0; i < 2; i++)
    {
      for (temp = world[real_room(arena_start_room[race][i])].people; temp;
           temp = temp->next_in_room)
      {
        if (!temp)
          continue;

        if (!IS_TRUSTED(temp))
        {
          count++;
        }
      }
    }

    if (count >= max_team_size[race] && !IS_TRUSTED(ch) && !CHAR_IN_ARENA(ch))
    {
      send_to_char
        ("&+LI'm sorry but the team is currently full, please wait for the next match.&N\r\n",
         ch);
      return TRUE;
    }

    act("$n enters $p.", TRUE, ch, obj, 0, TO_ROOM);
    if (!CHAR_IN_ARENA(ch))
    {
      char_from_room(ch);
      char_to_room(ch, real_room(arena_start_room[race][0]), -1);
      act("$n steps out of $p.", TRUE, ch, obj, 0, TO_ROOM);
      send_to_char("Please wait while everyone else arrives.\r\n", ch);
      return TRUE;
    }
    else
    {
      char_from_room(ch);
      char_to_room(ch,
                   real_room(arena_hometown_location
                             [IS_RACEWAR_EVIL(ch) ? EVIL : GOODIE]), -1);
      act("$n steps out of $p.", TRUE, ch, obj, 0, TO_ROOM);
      return TRUE;
    }
  }
  if (cmd == CMD_LOOK)
  {
    if (isname(arg, "score"))
    {
      show_scores_to_char(ch, -1);
      return TRUE;
    }
    if (isname(arg, "arena"))
    {
      if (IS_SET(arena.flags, FLAG_ENABLED))
      {
        show_stats_to_char(ch);
      }
      else
      {
        send_to_char("&+LArena is currently closed.&N\r\n", ch);
      }
      return TRUE;
    }
  }
  return FALSE;
}

void arena_char_spawn(P_char ch)
{
  int      i;

  switch (arena.type)
  {
  case TYPE_TEAM_DM:
    char_to_room(ch, real_room(arena.map.spawn[arena_team(ch)]), -1);
    break;
  case TYPE_DEATHMATCH:
    i = number(0, (arena.map.endroom - arena.map.startroom));
    char_to_room(ch, real_room(arena.map.startroom + i), -1);
    break;
  default:
    char_to_room(ch, real_room(arena.map.spawn[arena_team(ch)]), -1);
    break;
  }
}

void arena_activity()
{
  P_char   ch1, ch2;
  int      i, j;

  if (IS_SET(arena.flags, FLAG_ENABLED))
  {
    if (IS_SET(arena.flags, FLAG_SHUTTING_DOWN))
    {
      arena.stage = STAGE_END;
      players_from_map();
      send_to_arena
        ("&+WARENA SHUTTING DOWN, ALL PLAYERS BEING EVICTED.&N\r\n", -1);
      REMOVE_BIT(arena.flags, FLAG_SHUTTING_DOWN);
      REMOVE_BIT(arena.flags, FLAG_ENABLED);
    }
    if (arena.stage == STAGE_OPEN)
    {
      switch (arena.timer[0])
      {
      case 0:
        send_to_arena
          ("&+WArena Entrance is now closed, let's get ready to RUMBLE!&N\r\n",
           -1);
        break;
      case 10:
        send_to_arena("&+L10 seconds left!\r\n", -1);
        break;
      case 30:
        send_to_arena("&+L30 seconds till arena doors close!\r\n&N", -1);
        break;
      case 60:
        send_to_arena("&+L1 minute till arena doors close.\r\n", -1);
        break;
      default:
        break;
      }

      if (arena.timer[0] < 1)
      {
        snprintf(buf, MAX_STRING_LENGTH,
                "&+W%d seconds till the match begins, those of you in the arena who wish to leave, \r\n&+Wnow is your last chance!&N\r\n",
                DEFAULT_TIMER_ACCEPT);
        send_to_arena(buf, -1);
        arena.stage++;
        arena.timer[0] = DEFAULT_TIMER_ACCEPT;
        return;
      }
    }

    if (arena.stage == STAGE_ACCEPT)
    {
      switch (arena.timer[0])
      {
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
        snprintf(buf, MAX_STRING_LENGTH, "&+W%d!!&N\r\n", arena.timer[0]);
        send_to_arena(buf, -1);
        break;
      case 10:
        send_to_arena("&+W10 seconds!!&N\r\n", -1);
        break;
      case 30:
        send_to_arena("&+W30 seconds till the match begins!\r\n&N", -1);
        break;
      default:
        break;
      }


      if (arena.timer[0] < 1)
      {

        j = 3;
        for (i = 0; i < MAX_RACES; i++)
        {
          ch1 = world[real_room(arena_start_room[i][0])].people;
          ch2 = world[real_room(arena_start_room[i][1])].people;

          if (!ch1 && !ch2)
          {
            j--;
          }
        }
        if (j < 2 && arena.type != TYPE_DEATHMATCH &&
            arena.type != TYPE_KING_OF_THE_HILL)
        {
          send_to_arena
            ("&+LSince one side has chickened out, next match will begin in 15 minutes.&N\r\n",
             -1);
          arena.stage--;
          arena.timer[0] = (15 * 60);
          return;
        }
        arena.stage++;
        return;
      }
    }

    if (arena.stage == STAGE_BEGIN)
    {

      players_to_map();
      send_to_arena("&+WThe Match has Begun!! Let's RUMBLE!&N\r\n", -1);
      arena.stage++;
      arena.timer[0] = DEFAULT_TIMER_MATCH;
      return;
    }

    if (arena.stage == STAGE_END)
    {
      players_from_map();
      send_to_arena
        ("&+WThe Match has ended the ending scores are as follows:&N\r\n",
         -1);
      show_scores(-1);
      arena.stage++;
      arena.timer[0] = DEFAULT_TIMER_AFTERMATH;
      return;
    }

    if (arena.stage == STAGE_MATCH)
    {
      switch (arena.timer[0])
      {
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
        snprintf(buf, MAX_STRING_LENGTH, "&+W%d!!!\r\n&N", arena.timer[0]);
        send_to_arena(buf, -1);
        break;
      case 10:
        send_to_arena("&+W10 seconds!!&N\r\n", -1);
        break;
      case 30:
        send_to_arena("&+W30 seconds left in the match!&N\r\n", -1);
        break;
      case 60:
        send_to_arena("&+W60 seconds left in the match!&N\r\n", -1);
        break;
      }

      if (arena.timer[0] < 1)
      {
        arena.stage++;
        return;
      }
    }

    if (arena.stage == STAGE_AFTERMATH)
    {
      if (arena.timer[0] < 1)
      {
        send_to_arena("&+WArena is now open for next batch of victims!\r\n",
                      -1);
        extract_from_arena();
        init_teams();
        arena.stage = STAGE_OPEN;
        arena.timer[0] = DEFAULT_TIMER_OPEN;
        return;
      }
    }
    for (i = 0; i < MAX_ARENA_TIMER; i++)
    {
      if (arena.timer[i] >= 1)
      {
        arena.timer[i]--;
      }
    }
  }

}
