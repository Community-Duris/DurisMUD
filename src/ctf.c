/* ctf.c

   - Property of Duris
     4/2011

*/

// TO DO:

#include <stdlib.h>
#include <cstring>
#include <vector>
using namespace std;

#include "prototypes.h"
#include "defines.h"
#include "utils.h"
#include "structs.h"
#include "sql.h"
#include "ctf.h"
#include "assocs.h"
#include "utility.h"
#include "utils.h"
#include "guildhall.h"
#include "alliances.h"
#include "db.h"
#include "interp.h"
#include "comm.h"
#include "spells.h"
#include "boon.h"
#include "epic.h"
#include "objmisc.h"

#ifdef __NO_MYSQL__

int init_ctf()
{
    // load nothing
}

void do_ctf(P_char ch, char *arg, int cmd) 
{
    // do nothing
}
#else

extern MYSQL* DB;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern P_desc descriptor_list;
extern struct zone_data *zone_table;

void set_short_description(P_obj t_obj, const char *newShort);
void set_long_description(P_obj t_obj, const char *newDescription);

struct ctfData ctfdata[] = {
  {0,	0,		0,		0,	0,	NULL}, // None
  {1,	CTF_PRIMARY,	RACEWAR_GOOD,	790,	132573, NULL}, // Tharn
  {2,	CTF_PRIMARY,	RACEWAR_EVIL,	791,	97628,	NULL}, // Shady
  {4,	CTF_SECONDARY,	RACEWAR_NONE,	792,	-1,	NULL}, // Random epic
  {5,	CTF_SECONDARY,	RACEWAR_NONE,	792,	-1,	NULL}, // Random epic
  {6,	CTF_SECONDARY,	RACEWAR_NONE,	792,	-1,	NULL}, // Random epic
  {7,	CTF_SECONDARY,	RACEWAR_NONE,	792,	-1,	NULL}, // Random epic
  {8,	CTF_BOON,	RACEWAR_NONE,	792,	0,	NULL},
  {9,	CTF_BOON,	RACEWAR_NONE,	792,	0,	NULL},
  {10,	CTF_BOON,	RACEWAR_NONE,	792,	0,	NULL},
  {11,	CTF_BOON,	RACEWAR_NONE,	792,	0,	NULL},
  {12,	CTF_BOON,	RACEWAR_NONE,	792,	0,	NULL},
  {0}
};

int init_ctf()
{
#if defined (CTF_MUD) && (CTF_MUD == 1)
  fprintf(stderr, "-- Loading ctf\r\n");
  
  load_ctf();
#endif
}

int load_ctf()
{
  char buff[MAX_STRING_LENGTH];
  P_obj flag;
  int i, r_num, boon = 0;
 
  ctf_populate_boons();

  for (i = 1; ctfdata[i].id; i++)
  {
    obj_index[real_object(ctfdata[i].flag)].func.obj = ctf_flag_proc;

    if ((r_num = real_object(ctfdata[i].flag)) < 0)
    {
      debug("CTF Flag # %d has failed to load, continuing.", i);
      continue;
    }
   
    if (ctfdata[i].room == -1)
      ctfdata[i].room = ctf_get_random_room(i);
    if (!ctfdata[i].room)
      continue;

    ctfdata[i].obj = read_object(r_num, REAL);
    
    if (!ctfdata[i].obj)
    {
      logit(LOG_DEBUG, "load_ctf(): obj %d [%d] not loadable", r_num, ctfdata[i].flag);
      continue;
    }

    // set the flag short/long desc if its a secondary or boon flag type
    if (ctfdata[i].type == CTF_SECONDARY ||
	ctfdata[i].type == CTF_BOON)
    {
      sprintf(buff, "&+Lthe flag of&n %s&n", zone_table[world[real_room0(ctfdata[i].room)].zone].name);
      set_short_description(ctfdata[i].obj, buff);
      sprintf(buff + strlen(buff), "&+L is here.&n");
      set_long_description(ctfdata[i].obj, buff);
    }
    
    obj_to_room(ctfdata[i].obj, real_room0(ctfdata[i].room));
  }

  return TRUE;
}

int ctf_flag_proc(P_obj flag, P_char ch, int cmd, char *argument)
{
  char arg[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH], buff2[MAX_STRING_LENGTH];
  P_obj tobj;
  int i;
  int block_cmd = 0;

  if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
    return FALSE;

  if (!flag || !ch)
    return FALSE;

  if (IS_NPC(ch) || IS_TRUSTED(ch))
    return FALSE;
 
  if (GET_RACEWAR(ch) != RACEWAR_GOOD &&
      GET_RACEWAR(ch) != RACEWAR_EVIL)
    return FALSE;

  if (*argument)
    argument = one_argument(argument, arg);

  if (cmd == CMD_TAKE || cmd == CMD_GET) 
  {
    int apply_flag = 0;

    if (*arg)
    {
      if(!strcmp(arg, "all"))
	block_cmd = FALSE;
      else if (get_obj_in_list_vis(ch, arg, world[ch->in_room].contents) != flag)
	return FALSE;
      else
	block_cmd = TRUE;
    }
    else
      return FALSE;

    // Error checking
    for (i = 1; ctfdata[i].id; i++)
    {
      if (ctfdata[i].obj == flag)
	break;
    }
    if (!ctfdata[i].id)
    {
      debug("Should have found a flag, but didn't.");
      send_to_char("Problem with ctf flag data, notify an Imm.\r\n", ch);
      return TRUE;
    }

    if (affected_by_spell(ch, TAG_CTF))
    {
      send_to_char("You can't carry more than one flag at a time.\r\n", ch);
      if (block_cmd)
	return TRUE;
      else
	return FALSE;
    }

    // Picking up own flag
    if (GET_RACEWAR(ch) == ctfdata[i].racewar)
    {
      // If they're in their flag room
      if (world[ch->in_room].number == ctfdata[i].room)
      {
	send_to_char("Why would you pick up your own flag? Go get theirs!\r\n", ch);
      }
      // If they're not in their flag room
      else
      {
	obj_from_room(flag);
	act("You reach out to grab $p.", TRUE, ch, flag, 0, TO_CHAR);
	act("$n reaches to grab $p.", TRUE, ch, flag, 0, TO_ROOM);
	act("Upon being touched, $p dissapears in a &+YBLINDING flash&n.", FALSE, ch, flag, 0, TO_CHAR);
	act("Upon being touched, $p dissapears in a &+YBLINDING flash&n.", FALSE, ch, flag, 0, TO_ROOM);
	obj_to_room(flag, real_room(ctfdata[i].room));
	send_to_room_f(real_room0(ctfdata[i].room), "%s suddenly appears in a &+YBLINDING flash&n.\r\n", flag->short_description);
	if (GET_RACEWAR(ch) == RACEWAR_GOOD)
	  sprintf(buff, "&+YA beautiful melody rings through your ears as %s reclaims your team flag!\r\n", GET_NAME(ch));
	else
	  sprintf(buff, "&+YYou feel the bloodlust of a thousand ogres rise within you as %s reclaims your team flag!&n\r\n", GET_NAME(ch));
	sprintf(buff2, "&+YTorment and anguish can be felt gripping your soul as %s reclaims %s team's flag!&n\r\n", GET_NAME(ch), HSHR(ch));
	ctf_notify(buff, GET_RACEWAR(ch));
	ctf_notify(buff2, (GET_RACEWAR(ch) == 1 ? 2 : 1));
	add_ctf_entry(ch, ctfdata[i].type, CTF_TYPE_RECLAIM);
      }
    }
    // Ok, they are obviously picking up the other flag, or a secondary flag...
    else
    {
      if (ctfdata[i].type == CTF_PRIMARY)
      {
	// perform stuff for picking up primary flag.. do we notify?
        sprintf(buff, "&+rSeveral dull tones and chimes can be heard sounding through the area as %s picks up the enemies flag!&n\r\n", GET_NAME(ch));
        sprintf(buff2, "&+rSeveral dull tones and chimes can be heard sounding through the area as %s picks up your flag!&n\r\n", GET_NAME(ch));
      }
      else if (ctfdata[i].type == CTF_SECONDARY ||
	       ctfdata[i].type == CTF_BOON)
      {
	// perform secondary flag pickup stuff
        sprintf(buff, "&+rSeveral dull tones and chimes can be heard sounding through the area as %s picks up %s!&n\r\n", GET_NAME(ch), ctfdata[i].obj->short_description);
        sprintf(buff2, "&+rSeveral dull tones and chimes can be heard sounding through the area as %s picks up %s!&n\r\n", GET_NAME(ch), ctfdata[i].obj->short_description);
      }
      send_to_char_f(ch, "You get %s.\r\n", flag->short_description);
      act("$n gets $p.", TRUE, ch, flag, 0, TO_ROOM);
      ctf_notify(buff, GET_RACEWAR(ch));
      ctf_notify(buff2, (GET_RACEWAR(ch) == 1 ? 2: 1));
      obj_from_room(flag);
      apply_flag = TRUE;
    }

    if (apply_flag)
    {
      struct affected_type af;

      memset(&af, 0, sizeof(af));
      af.type = TAG_CTF;
      af.modifier = ctfdata[i].id;
      af.duration = -1;
      af.flags = AFFTYPE_NODISPEL | AFFTYPE_NOMSG | AFFTYPE_NOSAVE;
      affect_to_char(ch, &af);
    }

    if (block_cmd)
      return TRUE;
    else
      return FALSE;
  }   

  /* // There is no flag in inventory, so no need for this.
  if (cmd == CMD_GIVE)
  {
    if (get_obj_in_list_vis(ch, arg, world[ch->in_room].contents) != flag)
      return FALSE;
    else
    {
      send_to_char("You can't give that to them.  Drop it or hold onto it.\r\n", ch);
      return TRUE;
    }
  }
  */

  /* // No flag in inventory, handling differently
  if (cmd == CMD_DROP)
  {
    int j;
    // Are we targeting the flag from our inventory?
    if (get_obj_in_list_vis(ch, arg, ch->carrying) != flag)
      return FALSE;

    // Ok we have a flag in inv, find it's ctfdata id
    for (i = 1; i < CTF_MAX; i++)
      if (ctfdata[i].flag == GET_OBJ_VNUM(flag))
	break;
    for (j = 1; j < CTF_MAX; j++)
      if (ctfdata[j].racewar == GET_RACEWAR(ch))
	break;

    // Are we dropping it at our flag room
    if (world[ch->in_room].number != ctfdata[j].room)
      // no we aren't so who cares
      return FALSE;
    else
    {
      // we are, so lets see if our flag is present

    // check if they dropped flag in a place acceptable to capture it.
    }
  }
  */

  /* // Flag not in inventory
  if (cmd == CMD_ENTER)
  {
    // that's a nono, drop the flag
  }

  if (cmd == CMD_RENT || cmd == CMD_CAMP)
  {
    // drop the flag automatically
  }
  */

  return FALSE;
}

void ctf_notify(const char *msg, int racewar)
{
  P_desc d;

  while (*msg == ' ' && *msg != '\0')
    msg++;

  if (!*msg)
  {
    debug("ctf_notify(): sent null msg to function");
    return;
  }
  else
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (d->connected == CON_PLYNG)
      {
	if (IS_TRUSTED(d->character) ||
	    racewar == 0 ||
	    GET_RACEWAR(d->character) == racewar)
	{
	  send_to_char(msg, d->character);
	  write_to_pc_log(d->character, msg, LOG_PRIVATE);
	}
      }
    }
  }
  return;
}

// Can just call ctfdata[i].obj now
P_obj get_ctf_flag(int id)
{
  //register P_obj flag;

  //for (flag = object_list; flag; flag = flag->next)
  //{
  //  if (flag == ctfdata[id].obj)
  //    return flag;
      return ctfdata[id].obj;
  //}
  //return NULL;
}

bool drop_ctf_flag(P_char ch)
{
  char buff[MAX_STRING_LENGTH];
  struct affected_type *af;

  if (!ch)
    return false;

  if ((af = get_spell_from_char(ch, TAG_CTF)) != NULL)
  {
    P_obj flag = get_ctf_flag(af->modifier);
    if (!flag)
    {
      debug("Dropping flag with null pointer, not dropping anything instead");
      affect_remove(ch, af);
      return false;
    }
    obj_to_room(flag, ch->in_room);
    affect_remove(ch, af);
    sprintf(buff, "&+Y%s has dropped %s at %s&n\r\n", GET_NAME(ch), flag->short_description, world[ch->in_room].name);
    ctf_notify(buff, 0);
    act("You drop $p&n.", TRUE, ch, flag, 0, TO_CHAR);
    act("$n drops $p&n.", TRUE, ch, flag, 0, TO_ROOM);
    check_ctf_capture(ch, flag);
    return true;
  }
  return false;
}

bool check_ctf_capture(P_char ch, P_obj flag)
{
  int i;
  int id = 0;

  if (!flag)
    return false;

  // lets check just in case to make sure our flags still exist in game
  if (get_ctf_flag(CTF_FLAG_GOOD) == NULL)
  {
    debug("Can't find goodie CTF flag.  Reloading.");
    if ((ctfdata[CTF_FLAG_GOOD].obj = read_object(real_object(ctfdata[CTF_FLAG_GOOD].flag), REAL)) == NULL)
    {
      debug("Error loading goodie ctf flag.");
      return false;
    }
    obj_to_room(ctfdata[CTF_FLAG_GOOD].obj, real_room0(ctfdata[CTF_FLAG_GOOD].room));
  }
  
  if (get_ctf_flag(CTF_FLAG_EVIL) == NULL)
  {
    debug("can't find evil CTF flag. reloading.");
    if ((ctfdata[CTF_FLAG_EVIL].obj = read_object(real_object(ctfdata[CTF_FLAG_EVIL].flag), REAL)) == NULL)
    {
      debug("Error loading evil ctf flag.");
      return false;
    }
    obj_to_room(ctfdata[CTF_FLAG_EVIL].obj, real_room0(ctfdata[CTF_FLAG_EVIL].room));
  }

  // find the captured flag's id
  for (id = 1; ctfdata[id].id; id++)
    if (ctfdata[id].obj == flag)
      break;

  // Let's check spots able to capture flags
  for (i = 1; ctfdata[i].id; i++)
  {
    // can't capture the same flag
    if (ctfdata[i].obj == flag)
      continue;

    // don't check if the flag is in the void (ie on a player)
    if (OBJ_ROOM(ctfdata[i].obj) && ctfdata[i].obj->loc.room <= 0)
      continue;

    // can only capture on primary flag locations
    if (ctfdata[i].type != CTF_PRIMARY)
      continue;

    // can't capture a flag on a flag home you don't own
    if (ctfdata[i].racewar != GET_RACEWAR(ch))
      continue;

    // can't capture a flag when you're flag isn't at it's home
    if (!OBJ_IN_ROOM(ctfdata[i].obj, real_room0(ctfdata[i].room)))
      continue;

    // did we drop the flag in the correct spot?
    if (!OBJ_IN_ROOM(flag, real_room0(ctfdata[i].room)))
      continue;

    // We should be good to capture!
    capture_flag(ch, flag, id);
    return true;
  }

  return false;
}

void capture_flag(P_char ch, P_obj flag, int id)
{
  char buff[MAX_STRING_LENGTH];
  char buff2[MAX_STRING_LENGTH];
  if (ctfdata[id].racewar == RACEWAR_EVIL)
  {
    sprintf(buff, "&+YThe sound of a thousand trumpets can be heard erupting through the area as %s captures the evil flag.&n\r\n", GET_NAME(ch));
    sprintf(buff2, "&+YThe souls of a thousand demons can be heard wailing in anguish as %s captures your flag!&n\r\n", GET_NAME(ch));
  }
  else if (ctfdata[id].racewar == RACEWAR_GOOD)
  {
    sprintf(buff, "&+YA chilling monotonous horn can be heard echoing through the area as %s captures the good flag.&n\r\n", GET_NAME(ch));
    sprintf(buff2, "&+YA high-pitched shriek screams through the area as %s captures your flag!&n\r\n", GET_NAME(ch));
  }
  else if (ctfdata[id].racewar == RACEWAR_NONE)
  {
    sprintf(buff, "&+YA calming feeling washes over you as %s captures %s&+Y.&n\r\n", GET_NAME(ch), flag->short_description);
    sprintf(buff2, "&+YA deep anger overcomes you as %s captures %s&+Y.&n\r\n", GET_NAME(ch), flag->short_description);
  }
  ctf_notify(buff, GET_RACEWAR(ch));
  ctf_notify(buff2, (GET_RACEWAR(ch) == 1 ? 2 : 1));
  act("You have captured $p!&n", TRUE, ch, flag, 0, TO_CHAR);
  obj_from_room(flag);
  add_ctf_entry(ch, ctfdata[id].type, CTF_TYPE_CAPTURE);
  if (ctfdata[id].type != CTF_BOON)
  {
    obj_to_room(flag, real_room0(ctfdata[id].room));
    send_to_room_f(real_room0(ctfdata[id].room), "%s &nappears.\r\n", flag->short_description);
  }
  check_boon_completion(ch, NULL, id, BOPT_CTF);
  check_boon_completion(ch, NULL, id, BOPT_CTFB);
  return;
}

int add_ctf_entry(P_char ch, int flagtype, int type)
{
  if (qry("INSERT INTO ctf_data (time, pid, type, flagtype, racewar) VALUES " \
	"(%d, %d, %d, %d, %d)",
	time(0), GET_PID(ch), type, flagtype, GET_RACEWAR(ch)))
    return TRUE;
  
 return FALSE;
}

void show_ctf(P_char ch)
{
  char buff[MAX_STRING_LENGTH], buff2[MAX_STRING_LENGTH];
  int i;

  send_to_char("&+WCapture The Flag&n\r\n", ch);
  sprintf(buff, "%-2s %-60s %-40s\r\n", "ID", "Flag", "Location");

  for (i = 1; ctfdata[i].id; i++)
  {
    if (ctfdata[i].room && ctfdata[i].obj)
    {
      sprintf(buff + strlen(buff), "%-2d %-60s ", ctfdata[i].id, pad_ansi(ctfdata[i].obj->short_description, 60).c_str());
      if (OBJ_ROOM(ctfdata[i].obj))
      {
	sprintf(buff + strlen(buff), "%-60s\r\n", pad_ansi(world[ctfdata[i].obj->loc.room].name, 60).c_str());
      }
      else if (get_flag_carrier(i) != NULL)
      {
	sprintf(buff2, "Carried by %s", GET_NAME(get_flag_carrier(i)));
	sprintf(buff + strlen(buff), "%-40s\r\n", buff2);
      }
    }
  }
  send_to_char(buff, ch);
  send_to_char("\r\n", ch);
  

  return;

  // Get statistical data


}

void do_ctf(P_char ch, char *arg, int cmd)
{
  show_ctf(ch);
  /*
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];

  if( !ch || IS_NPC(ch) )
    return;

  argument_interpreter(arg, buf2, buf3);
  
  if( IS_TRUSTED(ch) && !str_cmp("reset", buf2) )
  {
    if (GET_LEVEL(ch) != OVERLORD)
    {
      send_to_char("Ask an overlord, this command will reset all active ctf.\n", ch);
      return;
    }
    
    if (!str_cmp("all", buf3))
    {
      send_to_char("Resetting all ctf parameters.\n", ch);
      reset_ctf(ch);
    }
    else if (!isdigit(*buf3))
    {
      send_to_char("You must specify a guild or racewar side ID or 'all'.", ch);
      return;
    }
    else
    {
      int id = atoi(buf3);
      building 
      reset_one_guild_ctf(guild);
      sprintf(buf, "You reset guild # %d's ctf stats.", id);
      return;
    }

    return;
  }
  
  if( IS_TRUSTED(ch) )
    //show_ctf_wiz(ch);
    show_ctf(ch);
  else
    show_ctf(ch);
   
  */
}

void reset_ctf(P_char ch)
{

}

void ctf_populate_boons()
{
  if (!qry("SELECT criteria, criteria2 FROM boons WHERE active = 1 AND opt = %d", BOPT_CTFB))
  {
    debug("ctf_populate_boons(): cant read from boon db");
    return;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if (mysql_num_rows(res) < 1)
  {
    mysql_free_result(res);
    return;
  }

  MYSQL_ROW row;

  while (row = mysql_fetch_row(res))
    ctfdata[atoi(row[0])].room = atoi(row[1]);

  mysql_free_result(res);

  return;
}

int ctf_use_boon(BoonData *bdata)
{
  int i;
  for (i = 1; ctfdata[i].id; i++)
  {
    if (ctfdata[i].type != CTF_BOON)
      continue;

    if (!ctfdata[i].room)
      break;
  }

  //double check
  if (ctfdata[i].id &&
      ctfdata[i].type == CTF_BOON &&
      !ctfdata[i].room)
  {
    ctfdata[i].room = bdata->criteria2;
    bdata->criteria = i;

    // load ctf flag..
    ctf_reload_flag(i);
  }

  return 1;
}

int ctf_reload_flag(int id)
{
  char buff[MAX_STRING_LENGTH];
  int i;
  P_desc td;
  affected_type *afp;

  if (id <= 0)
  {
    debug("id less than 0 passed to ctf_reload_flag");
    return 0;
  }

  for (i = 1; ctfdata[i].id; i++)
  {
    if (i == id)
      break;
  }
  if (i != id)
  {
    debug("invalid id sent to ctf_reload_flag");
    return 0;
  }

  if (ctfdata[id].obj != NULL)
  {
    for (td = descriptor_list; td; td = td->next)
    {
      if (td->character && !td->connected && IS_PC(td->character))
      {
	if ((afp = get_spell_from_char(td->character, TAG_CTF)) != NULL)
	{
	  if (afp->modifier == id)
	  {
	    affect_remove(td->character, afp);
	  }
	}
      }
    }
    ctf_delete_flag(id);
    ctfdata[id].obj = NULL;
  }
  if (!ctfdata[id].room)
  {
    debug("Trying to reload ctf flag with no loading room");
    return 0;
  }

  ctfdata[id].obj = read_object(real_object(ctfdata[id].flag), REAL);
 
  if (ctfdata[id].obj == NULL)
  {
    debug("Failed to recreate flag object.");
    return 0;
  }

  if (ctfdata[id].type == CTF_SECONDARY ||
      ctfdata[id].type == CTF_BOON)
  {
    sprintf(buff, "&+Lthe flag of&n %s&n", zone_table[world[real_room0(ctfdata[i].room)].zone].name);
    set_short_description(ctfdata[id].obj, buff);
    sprintf(buff + strlen(buff), "&+L is here.&n");
    set_long_description(ctfdata[id].obj, buff);
  }
  
  obj_to_room(ctfdata[id].obj, real_room0(ctfdata[i].room));
  return 1;
}

P_char get_flag_carrier(int id)
{
  P_desc td;
  affected_type *afp;

  if (ctfdata[id].obj != NULL && !OBJ_ROOM(ctfdata[id].obj))
  {
    for (td = descriptor_list; td; td = td->next)
    {
      if (td->character && !td->connected && IS_PC(td->character))
      {
	if ((afp = get_spell_from_char(td->character, TAG_CTF)) != NULL)
	{
	  if (afp->modifier == id)
	    return td->character;
	}
      }
    }
  }
  else
    return NULL;
}

void ctf_delete_flag(int id)
{
  P_char ch;
  affected_type *afp;

  if (!ctfdata[id].id)
  {
    debug("ctf_delete_flag() called with invalid id");
    return;
  }

  if (ctfdata[id].obj == NULL)
    return;

  if ((ch = get_flag_carrier(id)) != NULL)
  {
    afp = get_spell_from_char(ch, TAG_CTF);
    affect_remove(ch, afp);
    send_to_char_f(ch, "Suddenly the %s &ndissapears from you posession!\r\n", ctfdata[id].obj->short_description);
  }

  extract_obj(ctfdata[id].obj, TRUE);
  ctfdata[id].obj = NULL;
}

int ctf_get_random_room(int id)
{
  vector<epic_zone_data> epic_zones = get_epic_zones();

  int epic_zone = 0;
  int zone_number = 0;
  int room = 0;
  P_obj obj = NULL;

  while (!room)
  {
    epic_zone = number(epic_zones.size()-25, epic_zones.size());

    zone_number = epic_zones[epic_zone].number;

    for (obj = object_list; obj; obj = obj->next)
    {
      if (obj_zone_id(obj) != zone_number)
	continue;
      int obj_vnum = obj_index[obj->R_num].virtual_number;
      if (obj_vnum != EPIC_SMALL_STONE &&
	  obj_vnum != EPIC_LARGE_STONE &&
	  obj_vnum != EPIC_MONOLITH)
	continue;
      room = world[obj_room_id(obj)].number;
      if (room < 0)
	continue;
    }
    if (!obj)
      break;
    for (int i = 1; ctfdata[i].id; i++)
    {
      if (ctfdata[i].room > 0 &&
	  ctfdata[i].room == room)
      {
	room = 0;
	break;
      }
    }
  }

  if (room > 0)
    return room;
  else
    return 0;
}

#endif
