/*  nq.c - alternative quest system
 *  This file is a part of Duris MUD
 *
 *  Author: Michal Rembiszewski (Tharkun)
 */

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "prototypes.h"
#include "defines.h"
#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "string.h"
#include "utils.h"
#include "db.h"
#include "config.h"

#define NQ_RESET_NEVER     1
#define NQ_RESET_ALWAYS    2
#define NQ_RESET_REBOOT    3
#define NQ_RESET_COMPLETED 4

#define NQ_ACT_DISAPPEAR  BIT_1
#define NQ_ACT_COMPLETED  BIT_2
#define NQ_ACT_DEATH      BIT_3

#define NQ_MAX_LOCATION         10
#define NQ_MAX_ACTION_SET       10
#define NQ_MAX_REQUESTED_ITEMS 128
#define NQ_MAX_ACTORS           10
#define QUEST_DIR "quest"

#define NQ_QUEST               (const xmlChar*)"quest"
#define NQ_ACTOR               (const xmlChar*)"actor"
#define NQ_ACTION              (const xmlChar*)"a"
#define NQ_PHRASE              (const xmlChar*)"p"
#define NQ_REWARD              (const xmlChar*)"r"
#define NQ_ITEM                (const xmlChar*)"i"
#define NQ_SKILL               (const xmlChar*)"s"
#define NQ_TAG                 (const xmlChar*)"t"
#define NQ_NOTAG               (const xmlChar*)"nt"
#define NQ_LOCATION            (const xmlChar*)"l"
#define NQ_ACTIONSET           (const xmlChar*)"aset"
#define NQ_KEYWORDS            (const xmlChar*)"kw"
#define NQ_SHORT_DESC          (const xmlChar*)"short"
#define NQ_LONG_DESC           (const xmlChar*)"long"
#define NQ_CLASS               (const xmlChar*)"class"
#define NQ_RACE                (const xmlChar*)"race"
#define NQI_EXPIRES            (const xmlChar*)"expires"
#define NQI_LOG_ENTRY          (const xmlChar*)"logentry"
#define NQI_ACTOR              (const xmlChar*)"actorinstance"
#define NQI_INSTANCE           (const xmlChar*)"instance"
#define NQI_TMPL_IDX           (const xmlChar*)"templateindex"
#define NQI_ACTSET_IDX         (const xmlChar*)"actionsetindex"

extern P_room world;
extern P_index obj_index;
extern P_index mob_index;
extern P_char character_list;
extern struct s_skill skills[];
extern const char *spells[];
extern void act_convert(char *, const char *, P_char, P_char, P_obj, void *,
                        int);

struct nq_quest
{
  char    *id;                  // quest name
  char     once_per_player;     // each player can complete it only once
  int      reset_mode;          // flag definiting when if at all the quest resets
  int      duration_minutes;    // how much time player has for completing the quest
  int      allowed_instances;   // how many quest instances can run in parallel, -1 means unlimited
  uint     allowed_classes;     // vector with allowed classes
  int      allowed_races[LAST_RACE + 1];        // array with allowed races
  int      min_level;           // min level
  int      max_level;           // max level
  struct nq_skill *skill;       // skill requirements
  struct nq_actor *actor;       // global quest actors, can be empty when loading quest for the first time
  struct nq_actor_template *actor_template[NQ_MAX_ACTORS];      // templates for instance associated actors
  struct nq_instance *instance; // list of instances
  struct nq_quest *next;        // next quest in list
};

struct nq_actor_template
{
  int      vnum;                // mob number
  char    *keyword;             // mob keywords if different from default
  char    *short_desc;          // mob short description
  char    *long_desc;           // mob room description
  struct nq_string *tag;
  int      race;
  int      mob_class;
  int      location[NQ_MAX_LOCATION];   // list of possible pop rooms for this mob
  struct nq_action *action_set[NQ_MAX_ACTION_SET];      // array of alternative action sets
  struct nq_action *common_actions;
};

struct nq_actor
{
  P_char   ch;                  // pointer to a char structure, can be invalid!
  int      location;            // chosen location for this mob
  bool     loaded;              // indicates whether mob was loaded at instantiation
  struct nq_action *action;     // list of associated actions
  struct nq_actor_template *tmpl;       // template for this mob
  struct nq_actor *next;        // list in instance
};

struct nq_string
{
  char    *string;
  struct nq_string *next;
};

struct nq_instance
{
  char    *questor;             // name of the questing player
  int      expires;             // when quest expires
  struct nq_actor *actor;       // list of actors
  struct nq_string *log;        // quest log
  struct nq_string *tag;        // tags collected so far
  struct nq_instance *next;     // list of quest instances
};

struct nq_action
{
  int      flag;                // special action flags
  struct nq_item *item;         // required items
  int      cash;                // required cash
  struct nq_string *phrase;     // triggering phrases
  struct nq_string *tag;        // required tags
  struct nq_string *no_tag;     // forbidden tags
  struct nq_reward *reward;     // rewards
  struct nq_action *next;
};

struct nq_item
{
  int      vnum;                // item number
  char    *keyword;             // keyword if differs from the default
  char    *short_desc;          // short descriptions
  char    *long_desc;           // description when in room
  struct nq_item *next;
};

struct nq_reward
{
  struct nq_item *item;         // list of items
  int      cash;                // cash
  int      exp;                 // exp
  struct nq_skill *skill;       // skill
  char    *phrase;              // act string
  struct nq_string *tag;        // tags
  struct nq_reward *next;
};

struct nq_skill
{
  int      skill;
  int      value;
  bool     improve_only;
};

struct nq_quest *nq_list = NULL;

/*
 * functions nq_free_* release the resources associated with quest
 */
void nq_free_item(struct nq_item *item)
{
  if (item->keyword)
    FREE(item->keyword);

  if (item->short_desc)
    FREE(item->short_desc);

  if (item->long_desc)
    FREE(item->long_desc);

  FREE(item);
}

void nq_free_reward(struct nq_reward *reward)
{
  struct nq_item *item;
  struct nq_string *string;

  if (reward->phrase)
    FREE(reward->phrase);

  if (reward->skill)
    FREE(reward->skill);

  while (string = reward->tag)
  {
    reward->tag = string->next;
    FREE(string->string);
    FREE(string);
  }

  while (item = reward->item)
  {
    reward->item = item->next;
    nq_free_item(item);
  }
}

void nq_free_action(struct nq_action *action)
{
  struct nq_item *item;
  struct nq_reward *reward;
  struct nq_string *string;

  while (string = action->phrase)
  {
    action->phrase = string->next;
    FREE(string->string);
    FREE(string);
  }

  while (string = action->tag)
  {
    action->tag = string->next;
    FREE(string->string);
    FREE(string);
  }

  while (string = action->no_tag)
  {
    action->tag = string->next;
    FREE(string->string);
    FREE(string);
  }

  while (item = action->item)
  {
    action->item = item->next;
    nq_free_item(item);
  }

  while (reward = action->reward)
  {
    action->reward = reward->next;
    nq_free_reward(reward);
  }
}

void nq_free_actor_template(struct nq_actor_template *tmpl)
{
  struct nq_action *action;
  int      i;

  if (tmpl->keyword)
    FREE(tmpl->keyword);

  if (tmpl->short_desc)
    FREE(tmpl->short_desc);

  if (tmpl->long_desc)
    FREE(tmpl->long_desc);

  for (i = 0; i < NQ_MAX_ACTION_SET; i++)
    if (tmpl->action_set[i])
      while ((action = tmpl->action_set[i]) != tmpl->common_actions)
      {
        tmpl->action_set[i] = tmpl->action_set[i]->next;
        nq_free_action(action);
      }

  while (action = tmpl->common_actions)
  {
    tmpl->common_actions = tmpl->common_actions->next;
    nq_free_action(action);
  }

  FREE(tmpl);
}

void nq_free_instance(struct nq_instance *instance, P_char ch)
{
  struct nq_actor *actor;
  struct nq_string *log, *tag;
  char     buf[512];

  FREE(instance->questor);

  while (actor = instance->actor)
  {
    instance->actor = actor->next;
    if (actor->loaded && char_in_list(actor->ch))
    {
      if (ch)
      {
        snprintf(buf, 512, "Extracting $N in room [%d].", actor->ch->in_room);
        act("$n walks away..", FALSE, actor->ch, 0, 0, TO_ROOM);
        act(buf, FALSE, ch, 0, actor->ch, TO_CHAR);
      }
      // If it's not an immortal.
      if( IS_PC(ch) && (GET_LEVEL( ch ) < MINLVLIMMORTAL) )
      {
        update_ingame_racewar( -GET_RACEWAR(ch) );
      }
      extract_char(actor->ch);
    }
    FREE(actor);
  }

  while (log = instance->log)
  {
    instance->log = log->next;
    FREE(log->string);
    FREE(log);
  }

  while (tag = instance->tag)
  {
    instance->tag = tag->next;
    FREE(tag->string);
    FREE(tag);
  }

  FREE(instance);
}

void nq_free_quest(struct nq_quest *quest, P_char ch)
{
  struct nq_instance *instance;
  struct nq_actor *actor;
  int      i;

  FREE(quest->id);

  while (instance = quest->instance)
  {
    quest->instance = instance->next;
    nq_free_instance(instance, ch);
  }

  while (actor = quest->actor)
  {
    quest->actor = actor->next;
    FREE(actor);
  }

  for (i = 0; quest->actor_template[i]; i++)
  {
    nq_free_actor_template(quest->actor_template[i]);
  }

  FREE(quest);
}

/*
 * Creates nq_actor struct based on the provided template,
 * it picks a random action set and location
 */
struct nq_actor *nq_init_actor(struct nq_actor_template *tmpl)
{
  struct nq_actor *actor;
  int      i;

  CREATE(actor, nq_actor, 1, MEM_TAG_NQACTOR);
  memset(actor, 0, sizeof(struct nq_actor));
  actor->tmpl = tmpl;
  for (i = 0; tmpl->location[i]; i++)
    ;
  actor->location = tmpl->location[number(0, i - 1)];
  for (i = 0; tmpl->action_set[i]; i++)
    ;
  if (i)
    actor->action = tmpl->action_set[number(0, i - 1)];
  else
    actor->action = tmpl->common_actions;

  return actor;
}

/*
 * checks if mob is already an actor in the quest
 */
bool nq_acting(P_char ch, struct nq_quest * quest)
{
  struct nq_instance *instance;
  struct nq_actor *actor;

  for (actor = quest->actor; actor; actor = actor->next)
    if (actor->ch == ch)
      return TRUE;

  for (instance = quest->instance; instance; instance = instance->next)
    for (actor = instance->actor; actor; actor = actor->next)
      if (actor->ch == ch)
        return TRUE;

  return FALSE;
}

/*
 * associates 'actor' in the quest with an existing mob
 * if mob doesnt exist, it is created using the provided info
 */
int nq_load_actor(struct nq_actor *actor, struct nq_quest *quest)
{
  P_char   tch;
  int      room;
  int      vnum;

  room = real_room(actor->location);
  vnum = actor->tmpl->vnum;

  for (tch = world[room].people; tch; tch = tch->next_in_room)
    if (IS_NPC(tch) && !nq_acting(tch, quest)
        && GET_VNUM(tch) == vnum)
      break;

  if (!tch)
  {
    tch = read_mobile(vnum, VIRTUAL);
    actor->loaded = TRUE;
    if (tch)
      char_to_room(tch, room, -1);
    else
      return 0;
  }
  else
    actor->loaded = FALSE;

  if (actor->tmpl->keyword)
  {
    tch->only.npc->str_mask |= STRUNG_KEYS;
    tch->player.name = str_dup(actor->tmpl->keyword);
  }

  if (actor->tmpl->short_desc)
  {
    tch->only.npc->str_mask |= STRUNG_DESC2;
    tch->player.short_descr = str_dup(actor->tmpl->short_desc);
  }

  if (actor->tmpl->long_desc)
  {
    tch->only.npc->str_mask |= STRUNG_DESC1;
    tch->player.long_descr = str_dup(actor->tmpl->long_desc);
  }

  actor->ch = tch;

  return 1;
}

/*
 * loads an item using info provided in nq_item struct
 */
P_obj nq_create_item(struct nq_item * item)
{
  P_obj    obj;

  obj = read_object(item->vnum, VIRTUAL);

  if (item->keyword)
  {
    obj->str_mask |= STRUNG_KEYS;
    obj->name = str_dup(item->keyword);
  }

  if (item->short_desc)
  {
    obj->str_mask |= STRUNG_DESC2;
    obj->short_description = str_dup(item->short_desc);
  }

  if (item->long_desc)
  {
    obj->str_mask |= STRUNG_DESC1;
    obj->description = str_dup(item->long_desc);
  }

  return obj;
}

/*
 * associates the quest structure with physical mobs, creates them
 * when necessary
 */
void nq_init_quest(struct nq_quest *quest)
{
  struct nq_instance *instance;
  struct nq_actor *actor;
  int      i;

  for (i = 0; quest->actor_template[i]; i++)
    if (!quest->actor_template[i]->tag)
    {
      actor = nq_init_actor(quest->actor_template[i]);
      actor->next = quest->actor;
      quest->actor = actor;
      nq_load_actor(actor, quest);
    }

  for (instance = quest->instance; instance; instance = instance->next)
    for (actor = instance->actor; actor; actor = actor->next)
      nq_load_actor(actor, quest);
}

/*
 * for a given mob, retrieves the associated quest, instance and actor
 * structures if they exist
 */
int nq_find_actor(P_char mob, struct nq_quest **quest,
                  struct nq_instance **instance, struct nq_actor **actor)
{

  *quest = NULL;
  *instance = NULL;
  *actor = NULL;

  for (*quest = nq_list; *quest; *quest = (*quest)->next)
  {
    for (*actor = (*quest)->actor; *actor; *actor = (*actor)->next)
      if ((*actor)->ch == mob)
        return 1;
    for (*instance = (*quest)->instance; *instance;
         *instance = (*instance)->next)
      if ((*instance)->expires < time(0))
        for (*actor = (*instance)->actor; *actor; *actor = (*actor)->next)
          if ((*actor)->ch == mob)
            return 1;
  }

  return 0;
}

/*
 * find an item matching data from nq_item struct which is not in the
 * components array yet
 */
P_obj nq_find_item(struct nq_item * item, P_char ch, P_obj components[])
{
  P_obj    tobj;
  char     buf[128];
  int      i;

  for (tobj = ch->carrying; tobj; tobj = tobj->next_content)
    if (obj_index[tobj->R_num].virtual_number == item->vnum)
    {
      for (i = 0; components[i] && components[i] != tobj; i++)
        ;
      if (components[i] == 0)
        return tobj;
    }

  return NULL;
}

/*
 * handles rewarding the player after completing an action
 */
void nq_reward_player(struct nq_action *action, struct nq_instance *instance,
                      struct nq_quest *quest, P_char ch, P_char mob,
                      bool kill)
{
  struct nq_item *item;
  struct nq_instance *t_instance;
  struct nq_string *log_entry, *tag, *t_tag;
  char     buf[4096];
  P_obj    obj;

  if (action->reward->phrase)
  {
    act(action->reward->phrase, FALSE, mob, 0, ch, TO_VICT);
    if (instance)
    {                           // add entry to the quest log
      CREATE(log_entry, nq_string, 1, MEM_TAG_NQSTR);
      memset(log_entry, 0, sizeof(struct nq_string));
      act_convert(buf, action->reward->phrase, mob, ch, NULL, mob, TO_VICT);
      for (t_tag = instance->log; t_tag; t_tag = t_tag->next)
        if (!strcmp(t_tag->string, buf))
          break;
      if (!t_tag)
      {
        log_entry->string = str_dup(buf);
        log_entry->next = 0;
        if (instance->log)
        {
          for (t_tag = instance->log; t_tag->next; t_tag = t_tag->next)
            ;
          t_tag->next = log_entry;
        }
        else
          instance->log = log_entry;
      }
    }
  }

  if (action->reward->cash)
    ch->points.cash[3] += action->reward->cash / 1000;

  if (action->reward->exp)
    ;

  for (item = action->reward->item; item; item = item->next)
  {
    obj = nq_create_item(item);
    if (obj)
      if (kill)
        obj_to_char(obj, mob);
      else
        obj_to_char(obj, ch);
  }

  if (action->reward->skill)
  {
    ch->only.pc->skills[action->reward->skill->skill].taught +=
      action->reward->skill->value;
    if (!IS_SPELL(action->reward->skill->skill))
      snprintf(buf, 4096, "Ye feel yer potential in %s improve..\r\n",
              skills[action->reward->skill->skill].name);
    else
      snprintf(buf, 4096,
              "A sudden burst of arcane knowledge flows through your mind.\r\n"
              "You are now familiar with %s spell.\r\n",
              skills[action->reward->skill->skill].name);
    send_to_char(buf, ch);
  }

  if (!instance)
  {
    if (action->flag & NQ_ACT_COMPLETED)
      send_to_char("&+WCongratulations! You have completed this quest!&n\r\n",
                   ch);
    return;
  }

  if (!(action->flag & NQ_ACT_COMPLETED))
  {
    for (tag = action->reward->tag; tag; tag = tag->next)
    {
      // cannot get multiple identical tags
      for (t_tag = instance->tag; t_tag; t_tag = t_tag->next)
        if (strcmp(t_tag->string, tag->string) == 0)
          break;
      if (!t_tag)
      {
        CREATE(t_tag, nq_string, 1, MEM_TAG_NQSTR);
        t_tag->string = str_dup(tag->string);
        t_tag->next = instance->tag;
        instance->tag = t_tag;
      }
    }
  }
  else
  {
    if (quest->instance == instance)
      quest->instance = quest->instance->next;
    else
    {
      for (t_instance = quest->instance; t_instance->next != instance;
           t_instance = t_instance->next)
        ;
      t_instance->next = instance->next;
    }
    nq_free_instance(instance, NULL);
    send_to_char("Congratulations! You have completed this quest!\r\n", ch);
  }
}

/*
 * executes a given action:
 * checks whether requirements are met, if so extracts required items/cash
 */
int nq_test_single_action(struct nq_action *action,
                          struct nq_instance *instance,
                          struct nq_quest *quest, P_char ch, P_char mob,
                          char *phrase, bool kill)
{
  P_obj    obj;
  P_obj    components[NQ_MAX_REQUESTED_ITEMS];
  struct nq_item *item;
  struct nq_reward *reward;
  struct nq_instance *t_inst, *prev;
  struct nq_string *act_tag, *pc_tag, *string, *t_tag;
  int      found;
  int      mob_cash;

  if (((action->flag & NQ_ACT_DEATH) && !kill)
      || (!(action->flag & NQ_ACT_DEATH) && kill))
    return 0;

  // does pc have all required tags
  if (action->tag)
  {
    if (!instance)
      return 0;

    for (act_tag = action->tag; act_tag; act_tag = act_tag->next)
    {
      for (pc_tag = instance->tag; pc_tag; pc_tag = pc_tag->next)
        if (!strcmp(act_tag->string, pc_tag->string))
          break;
      if (!pc_tag)
        return 0;
    }
  }

  // does pc have any of the forbidden tags
  if (action->no_tag && instance)
  {
    for (act_tag = action->no_tag; act_tag; act_tag = act_tag->next)
      for (pc_tag = instance->tag; pc_tag; pc_tag = pc_tag->next)
        if (!strcmp(act_tag->string, pc_tag->string))
          return 0;
  }

  if (action->phrase)
  {
    if (!phrase)
      return 0;
    for (string = action->phrase; string; string = string->next)
    {
      if (!strcasecmp(string->string, phrase))
        break;
    }
    if (!string)
      return 0;
  }

  memset(components, 0, sizeof(components));

  for (found = 0, item = action->item; item; item = item->next)
    if (obj = nq_find_item(item, mob, components))
      components[found++] = obj;
    else
      return 0;

  mob_cash = mob->points.cash[0] + 10 * mob->points.cash[1] +
    100 * mob->points.cash[2] + 1000 * mob->points.cash[3];
  if (mob_cash < action->cash)
    return 0;

  /* success */
  while (found--)
  {
    extract_obj(components[found], TRUE); // Quest item artis?
  }

  if (instance)
    for (act_tag = action->tag; act_tag; act_tag = act_tag->next)
    {
      if (!strcmp(instance->tag->string, act_tag->string))
      {
        pc_tag = instance->tag;
        instance->tag = pc_tag->next;
      }
      else
      {
        for (t_tag = instance->tag; t_tag->next; t_tag = t_tag->next)
          if (!strcmp(t_tag->next->string, act_tag->string))
            break;
        pc_tag = t_tag->next;
        t_tag->next = t_tag->next->next;
      }
      FREE(pc_tag->string);
      FREE(pc_tag);
    }

  mob_cash -= action->cash;
  mob->points.cash[3] = mob_cash / 1000;
  mob_cash %= 1000;
  mob->points.cash[2] = mob_cash / 100;
  mob_cash %= 100;
  mob->points.cash[1] = mob_cash / 10;
  mob_cash %= 10;
  mob->points.cash[0] = mob_cash;

  return 1;
}

/*
 * creates an instance of a given quest with ch being the questor
 */
struct nq_instance *nq_accept_quest(struct nq_quest *quest, P_char ch)
{
  struct nq_instance *instance;
  struct nq_actor *actor, *actor_template;
  struct nq_action *action, *newact;
  int      i;

  CREATE(instance, nq_instance, 1, MEM_TAG_NQINST);
  memset(instance, 0, sizeof(struct nq_instance));
  instance->questor = str_dup(ch->player.name);
  instance->expires = time(0) + quest->duration_minutes * 60;

  for (i = 1; quest->actor_template[i]; i++)
  {
    actor = nq_init_actor(quest->actor_template[i]);
    actor->next = instance->actor;
    instance->actor = actor;
    nq_load_actor(actor, quest);
  }

  instance->next = quest->instance;
  quest->instance = instance;

  send_to_char("A new quest has been added to your journal!\r\n", ch);

  return instance;
}

/*
 * generic action check for killing, asking, giving
 */
int nq_action_check_all(P_char ch, P_char mob, char *phrase, bool kill)
{
  struct nq_quest *quest;
  struct nq_actor *actor;
  struct nq_action *action;
  struct nq_instance *instance, *t_instance;
  int      was_accepted, i;

  if (nq_find_actor(mob, &quest, &instance, &actor) == 0)
    return 0;

  was_accepted = FALSE;

  // a new quest instance is created when an action gives any tag and all requirements
  // are met, ie, class, race, level, free quest slots
  for (action = actor->action; action; action = action->next)
  {
    for (i = 0, t_instance = quest->instance; t_instance && !was_accepted;
         t_instance = t_instance->next)
    {
      if (!strcmp(t_instance->questor, ch->player.name))
      {
        was_accepted = TRUE;
        instance = t_instance;
      }
      i++;
    }
    if (was_accepted || !action->reward->tag)
    {
      if (nq_test_single_action
          (action, instance, quest, ch, mob, phrase, kill))
      {
        nq_reward_player(action, instance, quest, ch, mob, kill);
        return 1;
      }
    }
    else if ((ch->player.m_class & quest->allowed_classes) != 0
             && quest->allowed_races[GET_RACE(ch)] != 0
             && i < quest->allowed_instances
             && (!quest->skill
                 || ch->only.pc->skills[quest->skill->skill].taught ==
                 quest->skill->value)
             && nq_test_single_action(action, instance, quest, ch, mob,
                                      phrase, kill))
    {
      instance = nq_accept_quest(quest, ch);
      nq_reward_player(action, instance, quest, ch, mob, kill);
      return 1;
    }
  }

  return 0;
}

/*
 * called when victim is being killed by ch
 */
void nq_char_death(P_char ch, P_char victim)
{
  nq_action_check_all(ch, victim, NULL, 1);
}

/*
 * called after giving item or cash to a mob or after
 * asking, telling mob something. this is the main entry
 * point for interaction with this quest system
 */
int nq_action_check(P_char ch, P_char mob, char *phrase)
{
  nq_action_check_all(ch, mob, phrase, 0);
  return 0;
}

/*
 * functions nq_parse_* create nq_ data structures from libxml data
 * they should be replaced if quest data is to be stored via some
 * other mechanism
 */
struct nq_item *nq_parse_item(xmlNodePtr node)
{
  struct nq_item *item;
  char    *content;

  CREATE(item, nq_item, 1, MEM_TAG_NQITEM);
  memset(item, 0, sizeof(struct nq_item));

  node = node->xmlChildrenNode;
  content = (char *) xmlNodeGetContent(node);
  item->vnum = atoi(content);
  xmlFree(content);

  while (node)
  {
    if (!xmlStrcmp(node->name, NQ_KEYWORDS))
    {
      item->keyword = (char *) xmlNodeGetContent(node->xmlChildrenNode);
    }
    else if (!xmlStrcmp(node->name, NQ_SHORT_DESC))
    {
      item->short_desc = (char *) xmlNodeGetContent(node->xmlChildrenNode);
    }
    else if (!xmlStrcmp(node->name, NQ_LONG_DESC))
    {
      item->long_desc = (char *) xmlNodeGetContent(node->xmlChildrenNode);
    }
    node = node->next;
  }

  return item;
}

struct nq_skill *nq_parse_skill(xmlNodePtr node)
{
  struct nq_skill *skill;
  char    *content;
  int      skill_num;
  xmlChar *gain;

  content = (char *) xmlNodeGetContent(node->xmlChildrenNode);
  skill_num = search_block(content, spells, TRUE);
  gain = xmlGetProp(node, (const xmlChar *) "value");
  if (skill_num == -1 || gain == NULL)
    return NULL;

  CREATE(skill, nq_skill, 1, MEM_TAG_NQSKILL);
  memset(skill, 0, sizeof(struct nq_skill));

  skill->value = atoi((char *) gain);
  skill->skill = skill_num;
  skill->improve_only = FALSE;

  return skill;
}

struct nq_reward *nq_parse_reward(xmlNodePtr node)
{
  struct nq_reward *reward;
  struct nq_string *tag;
  struct nq_item *item;
  char    *content;

  CREATE(reward, nq_reward, 1, MEM_TAG_NQRWRD);
  memset(reward, 0, sizeof(struct nq_reward));

  node = node->xmlChildrenNode;

  while (node)
  {
    if (!xmlStrcmp(node->name, NQ_PHRASE))
    {
      content = (char *) xmlNodeGetContent(node->xmlChildrenNode);
      reward->phrase = str_dup(content);
      xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQ_ITEM))
    {
      item = nq_parse_item(node);
      item->next = reward->item;
      reward->item = item;
    }
    else if (!xmlStrcmp(node->name, NQ_TAG))
    {
      CREATE(tag, nq_string, 1, MEM_TAG_NQSTR);
      content = (char *) xmlNodeGetContent(node->xmlChildrenNode);
      tag->string = str_dup(content);
      tag->next = reward->tag;
      reward->tag = tag;
      xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQ_SKILL))
    {
      reward->skill = nq_parse_skill(node);
    }
    node = node->next;
  }

  return reward;
}

struct nq_action *nq_parse_action(xmlNodePtr node)
{
  struct nq_action *action;
  struct nq_reward *reward;
  struct nq_string *string, *tag;
  struct nq_item *item;
  xmlChar *att, *content;

  CREATE(action, nq_action, 1, MEM_TAG_NQACTN);
  memset(action, 0, sizeof(struct nq_action));

  if (xmlHasProp(node, (const xmlChar *) "flag"))
  {
    att = xmlGetProp(node, (const xmlChar *) "flag");
    if (!xmlStrcmp(att, (const xmlChar *) "disappear"))
      action->flag |= NQ_ACT_DISAPPEAR;
    if (!xmlStrcmp(att, (const xmlChar *) "completed"))
      action->flag |= NQ_ACT_COMPLETED;
    xmlFree(att);
  }

  content = xmlNodeGetContent(node->xmlChildrenNode);
  if (content && !xmlStrcmp(content, (xmlChar *) "death"))
    action->flag |= NQ_ACT_DEATH;
  if (content)
    xmlFree(content);

  node = node->xmlChildrenNode;

  while (node)
  {
    if (!xmlStrcmp(node->name, NQ_ITEM))
    {
      item = nq_parse_item(node);
      item->next = action->item;
      action->item = item;
    }
    else if (!xmlStrcmp(node->name, NQ_PHRASE))
    {
      CREATE(string, nq_string, 1, MEM_TAG_NQSTR);
      content = xmlNodeGetContent(node->xmlChildrenNode);
      string->string = str_dup((const char *) content);
      string->next = action->phrase;
      action->phrase = string;
      xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQ_REWARD))
    {
      action->reward = nq_parse_reward(node);
    }
    else if (!xmlStrcmp(node->name, NQ_TAG))
    {
      CREATE(tag, nq_string, 1, MEM_TAG_NQSTR);
      content = xmlNodeGetContent(node->xmlChildrenNode);
      tag->string = str_dup((const char *) content);
      tag->next = action->tag;
      action->tag = tag;
      xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQ_NOTAG))
    {
      CREATE(tag, nq_string, 1, MEM_TAG_NQSTR);
      content = xmlNodeGetContent(node->xmlChildrenNode);
      tag->string = str_dup((const char *) content);
      tag->next = action->no_tag;
      action->no_tag = tag;
      xmlFree(content);
    }
    node = node->next;
  }

  return action;
}

int nq_parse_int(xmlNodePtr node)
{
  xmlChar *content;

  content = xmlNodeGetContent(node);
  return atoi((const char *) content);
}

struct nq_actor_template *nq_parse_actor_template(xmlNodePtr node)
{
  struct nq_actor_template *actor_template;
  struct nq_action *action, *common_actions, *a_set_tails[NQ_MAX_ACTION_SET];
  struct nq_string *tag;
  int      loc_count = 0;
  int      actionset_count = 0;
  xmlNodePtr anode;
  xmlChar *content;

  CREATE(actor_template, nq_actor_template, 1, MEM_TAG_NQATMP);
  memset(actor_template, 0, sizeof(struct nq_actor_template));
  memset(a_set_tails, 0, sizeof(a_set_tails));
  common_actions = NULL;
  //parse attributes

  node = node->xmlChildrenNode;

  content = xmlNodeGetContent(node);
  actor_template->vnum = atoi((const char *) content);
  xmlFree(content);

  while (node)
  {
    if (!xmlStrcmp(node->name, NQ_LOCATION))
    {
      actor_template->location[loc_count++] = nq_parse_int(node);
    }
    else if (!xmlStrcmp(node->name, NQ_TAG))
    {
      CREATE(tag, nq_string, 1, MEM_TAG_NQSTR);
      content = xmlNodeGetContent(node->xmlChildrenNode);
      tag->string = str_dup((const char *) content);
      tag->next = actor_template->tag;
      actor_template->tag = tag;
      xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQ_KEYWORDS))
    {
      content = xmlNodeGetContent(node->xmlChildrenNode);
      actor_template->keyword = str_dup((const char *) content);
      xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQ_SHORT_DESC))
    {
      content = xmlNodeGetContent(node->xmlChildrenNode);
      actor_template->short_desc = str_dup((const char *) content);
      xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQ_LONG_DESC))
    {
      content = xmlNodeGetContent(node->xmlChildrenNode);
      actor_template->long_desc = str_dup((const char *) content);
      xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQ_ACTION))
    {
      action = nq_parse_action(node);
      action->next = common_actions;
      common_actions = action;
    }
    else if (!xmlStrcmp(node->name, NQ_ACTIONSET))
    {
      anode = node->xmlChildrenNode;
      while (anode)
      {
        if (!xmlStrcmp(anode->name, NQ_ACTION))
        {
          action = nq_parse_action(anode);
          action->next = actor_template->action_set[actionset_count];
          actor_template->action_set[actionset_count] = action;
          if (!a_set_tails[actionset_count])
            a_set_tails[actionset_count] = action;
        }
        anode = anode->next;
      }
      actionset_count++;
    }
    node = node->next;
  }

  while (actionset_count--)
  {
    if (a_set_tails[actionset_count])
      a_set_tails[actionset_count]->next = common_actions;
    else
      actor_template->action_set[actionset_count] = common_actions;
  }

  actor_template->common_actions = common_actions;

  return actor_template;
}

struct nq_actor *nq_parse_actor(xmlNodePtr node, struct nq_quest *quest)
{
  struct nq_actor *actor;
  int      template_index;
  int      action_set_index;

  CREATE(actor, nq_actor, 1, MEM_TAG_NQACTOR);
  memset(actor, 0, sizeof(struct nq_actor));

  node = node->xmlChildrenNode;

  while (node)
  {
    if (!xmlStrcmp(node->name, NQ_LOCATION))
    {
      actor->location = nq_parse_int(node->xmlChildrenNode);
    }
    else if (!xmlStrcmp(node->name, NQI_TMPL_IDX))
    {
      template_index = nq_parse_int(node->xmlChildrenNode);
      actor->tmpl = quest->actor_template[template_index];
    }
    else if (!xmlStrcmp(node->name, NQI_ACTSET_IDX))
    {
      action_set_index = nq_parse_int(node->xmlChildrenNode);
    }
    node = node->next;
  }

  actor->action = actor->tmpl->action_set[action_set_index];

  return actor;
}

struct nq_instance *nq_parse_instance(xmlNodePtr node, struct nq_quest *quest)
{
  struct nq_instance *instance;
  struct nq_string *log_entry, *tag;
  struct nq_actor *actor;
  xmlNodePtr t_node;
  char    *content;

  CREATE(instance, nq_instance, 1, MEM_TAG_NQINST);
  memset(instance, 0, sizeof(struct nq_instance));

  node = node->xmlChildrenNode;

  content = (char *) xmlNodeGetContent(node);
  instance->questor = str_dup(content);
  xmlFree(content);

  while (node)
  {
    if (!xmlStrcmp(node->name, NQI_ACTOR))
    {
      actor = nq_parse_actor(node, quest);
      actor->next = instance->actor;
      instance->actor = actor;
    }
    else if (!xmlStrcmp(node->name, NQI_LOG_ENTRY))
    {
      t_node = node->xmlChildrenNode;
      content = (char *) xmlNodeGetContent(t_node);
      if (content && *content)
      {
        CREATE(log_entry, nq_string, 1, MEM_TAG_NQSTR);
        memset(log_entry, 0, sizeof(struct nq_string));
        log_entry->string = str_dup(content);
        log_entry->next = instance->log;
        instance->log = log_entry;
      }
      else if (content)
        xmlFree(content);
    }
    else if (!xmlStrcmp(node->name, NQI_EXPIRES))
    {
      instance->expires = nq_parse_int(node->xmlChildrenNode);
    }
    else if (!xmlStrcmp(node->name, NQ_TAG))
    {
      t_node = node->xmlChildrenNode;
      content = (char *) xmlNodeGetContent(t_node);
      if (content && *content)
      {
        CREATE(tag, nq_string, 1, MEM_TAG_NQSTR);
        memset(tag, 0, sizeof(struct nq_string));
        tag->string = str_dup(content);
        tag->next = instance->tag;
        instance->tag = tag;
      }
      else if (content)
        xmlFree(content);
    }
    node = node->next;
  }

  return instance;
}

/*
 * creates quest structure from the data stored in file fname
 */
struct nq_quest *nq_parse_quest(char *fname)
{
  char     buf[256];
  xmlDocPtr doc;
  xmlNodePtr node;
  xmlChar *att;
  struct nq_quest *quest;
  struct nq_actor_template *actor_template;
  struct nq_actor *actor;
  struct nq_instance *instance;
  bool     allow_classes;
  bool     allow_races;
  int      actor_templates = 0;
  int      i;

  snprintf(buf, 256, "%s/%s", QUEST_DIR, fname);
  doc = xmlParseFile(buf);
  if (doc == NULL)
    return NULL;

  node = xmlDocGetRootElement(doc);
  if (node == NULL || xmlStrcmp(node->name, (const xmlChar *) "quest"))
  {
    xmlFreeDoc(doc);
    return NULL;
  }

  CREATE(quest, nq_quest, 1, MEM_TAG_NQQST);
  memset(quest, 0, sizeof(struct nq_quest));

  quest->id = (char *) xmlGetProp(node, (const xmlChar *) "id");
  att = xmlGetProp(node, (const xmlChar *) "onceperplayer");
  if (xmlStrcmp(att, (const xmlChar *) "YES"))
    quest->once_per_player = TRUE;
  else
    quest->once_per_player = FALSE;
  xmlFree(att);
  att = xmlGetProp(node, (const xmlChar *) "resetmode");
  if (xmlStrcmp(att, (const xmlChar *) "ALWAYS"))
    quest->reset_mode = NQ_RESET_ALWAYS;
  else if (xmlStrcmp(att, (const xmlChar *) "NEVER"))
    quest->reset_mode = NQ_RESET_NEVER;
  else if (xmlStrcmp(att, (const xmlChar *) "COMPLETED"))
    quest->reset_mode = NQ_RESET_COMPLETED;
  else
    quest->reset_mode = NQ_RESET_REBOOT;
  xmlFree(att);
  if (xmlHasProp(node, (const xmlChar *) "duration"))
  {
    att = xmlGetProp(node, (const xmlChar *) "duration");
    quest->duration_minutes = atoi((const char *) att);
    xmlFree(att);
  }
  att = xmlGetProp(node, (const xmlChar *) "allowedinstances");
  quest->allowed_instances = atoi((const char *) att);
  xmlFree(att);
  if (xmlHasProp(node, (const xmlChar *) "minlevel"))
  {
    att = xmlGetProp(node, (const xmlChar *) "minlevel");
    quest->min_level = atoi((const char *) att);
    xmlFree(att);
  }
  if (xmlHasProp(node, (const xmlChar *) "maxlevel"))
  {
    att = xmlGetProp(node, (const xmlChar *) "maxlevel");
    quest->max_level = atoi((const char *) att);
    xmlFree(att);
  }
  att = xmlGetProp(node, (const xmlChar *) "listedclasses");
  if (!xmlStrcmp(att, (const xmlChar *) "allow"))
  {
    quest->allowed_classes = 0;
    allow_classes = TRUE;
  }
  else
  {
    quest->allowed_classes = (1 << CLASS_COUNT) - 1;
    allow_classes = FALSE;
  }
  xmlFree(att);
  att = xmlGetProp(node, (const xmlChar *) "listedraces");
  if (!xmlStrcmp(att, (const xmlChar *) "allow"))
  {
    allow_races = TRUE;
    for (i = 0; i <= LAST_RACE; i++)
      quest->allowed_races[i] = 0;
  }
  else
  {
    for (i = 0; i <= LAST_RACE; i++)
      quest->allowed_races[i] = 1;
    allow_races = FALSE;
  }
  xmlFree(att);

  node = node->xmlChildrenNode;
  while (node)
  {
    if (!xmlStrcmp(node->name, NQ_ACTOR))
    {
      quest->actor_template[actor_templates++] =
        nq_parse_actor_template(node);
    }
    else if (!xmlStrcmp(node->name, NQI_INSTANCE))
    {
      instance = nq_parse_instance(node, quest);
      instance->next = quest->instance;
      quest->instance = instance;
    }
    else if (!xmlStrcmp(node->name, NQ_CLASS))
    {
      //!!!
    }
    else if (!xmlStrcmp(node->name, NQ_RACE))
    {
      //!!!
    }
    else if (!xmlStrcmp(node->name, NQ_SKILL))
    {
      quest->skill = nq_parse_skill(node);
    }
    node = node->next;
  }

  xmlFreeDoc(doc);
  return quest;
}

/*
 * saves the quest data to a file
 */
int nq_save_quest(struct nq_quest *quest)
{
  return 0;
}

void nq_interface_mortal_list(P_char ch, char *args)
{
}

void nq_interface_mortal_show(P_char ch, char *args)
{
  struct nq_quest *quest;
  struct nq_string *log_entry;
  struct nq_instance *instance = NULL;
  char     buf[4096];
  int      counter = 0;

  for (quest = nq_list; quest; quest = quest->next)
  {
    for (instance = quest->instance; instance; instance = instance->next)
      if (!strcmp(instance->questor, ch->player.name))
        break;
    if (!instance)
      continue;
    counter++;
    snprintf(buf, 4096, "[%d] &+B%s&n\r\n", counter, quest->id);
    for (log_entry = instance->log; log_entry; log_entry = log_entry->next)
    {
      strcat(buf, "\r\n");
      strcat(buf, log_entry->string);
    }
  }

  if (counter)
  {
    send_to_char("You take part in the following quests:\r\n", ch);
    strcat(buf, "\r\n");
    page_string(ch->desc, buf, 1);
  }
  else
    send_to_char("There is no quest info available at this time.\r\n", ch);
}

void nq_interface_mortal_delete(P_char ch, char *args)
{
}

void nq_interface_immo_list(P_char ch, char *args)
{
  struct nq_instance *instance;
  struct nq_actor *actor;
  struct nq_quest *quest;
  struct nq_string *tag;
  int      instances;
  bool     exists;
  P_char   tch;
  char     buf[256];

  send_to_char("The following quests have been loaded:\r\n\r\n", ch);
  for (quest = nq_list; quest; quest = quest->next)
  {
    snprintf(buf, 256, "&+B[%s]&n\r\n", quest->id);
    send_to_char(buf, ch);
    instances = 1;
    if (quest->actor)
    {
      exists = char_in_list(tch = quest->actor->ch);
      snprintf(buf, 256, "  Holder: %s (in %d)\r\n",
              exists ? tch->player.short_descr : "&+RMISSING&n",
              exists ? world[tch->in_room].number : 0);
      send_to_char(buf, ch);
    }

    for (instance = quest->instance; instance; instance = instance->next)
    {
      snprintf(buf, 256, "  [%d] %s &+g", instances++, instance->questor);
      for (tag = instance->tag; tag; tag = tag->next)
      {
        strcat(buf, tag->string);
        strcat(buf, " ");
      }
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      for (actor = instance->actor; actor; actor = actor->next)
      {
        exists = char_in_list(actor->ch);
        snprintf(buf, 256, "    %s (in %d)\r\n",
                exists ? actor->ch->player.short_descr : "&+RMISSING&n",
                exists ? world[actor->ch->in_room].number : 0);
        send_to_char(buf, ch);
      }
    }
  }
}

void nq_interface_immo_delete(P_char ch, char *args)
{
  struct nq_quest *prev, *quest;

  if (!args)
  {
    send_to_char("Quest not found.\r\n", ch);
    return;
  }

  for (prev = 0, quest = nq_list; quest; prev = quest, quest = quest->next)
  {
    if (!strcmp(quest->id, args))
    {
      if (prev)
        prev = quest->next;
      else
        nq_list = quest->next;
      nq_free_quest(quest, ch);
      send_to_char("Quest deleted successfuly.\r\n", ch);
      break;
    }
  }
  send_to_char("Quest not found.\r\n", ch);
}

void nq_interface_load(P_char ch, char *args)
{
  struct nq_quest *quest;

  if (!args || *args == 0)
    return;

  quest = nq_parse_quest(args);
  if (quest == NULL)
  {
    send_to_char("&+rFailed to load the quest.\r\n", ch);
    return;
  }
  else
  {
    nq_init_quest(quest);
    send_to_char("The quest loaded successfuly.\r\n", ch);
    quest->next = nq_list;
    nq_list = quest;
  }
}

void nq_interface_immo_show(P_char ch, char *args)
{
}

/*
 * the command interface to the quest system
 */
void do_quest2(P_char ch, char *args, int cmd)
{
  char    *command;
  char    *arg;
  int      i;
  struct nq_quest *quest, *prev;
  struct nq_interface_mapping
  {
    char    *command;
    void     (*immo_func) (P_char, char *);
    void     (*mortal_func) (P_char, char *);
  } interface_map[] =
  {
    {
    "list", nq_interface_immo_list, nq_interface_mortal_list},
    {
    "show", nq_interface_immo_show, nq_interface_mortal_show},
    {
    "delete", nq_interface_immo_delete, nq_interface_mortal_delete},
    {
    "load", nq_interface_load},
    {
    0}
  };

  command = (char *) strtok(args, " ");
  arg = (char *) strtok(NULL, "\0");

  if (!command || *command == '\0')
    return;

  for (i = 0; interface_map[i].command; i++)
  {
    if (!strcmp(command, interface_map[i].command))
      if (IS_TRUSTED(ch) && interface_map[i].immo_func)
        (interface_map[i].immo_func) (ch, arg);
      else if (!IS_TRUSTED(ch) && interface_map[i].mortal_func)
        (interface_map[i].mortal_func) (ch, arg);
  }
}
