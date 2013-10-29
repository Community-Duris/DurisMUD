/*
   ***************************************************************************
   *  File: artifact.c                                         Part of Duris *
   *  Usage: routines for artifact-tracking system                           *

   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "db.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "mm.h"
#include "spells.h"
#include "sql.h"
#include "files.h"

#define ARTIFACT_DIR "Players/Artifacts/"
#define ARTIFACT_MORT_DIR "Players/Artifacts/Mortal/"
#define ARTI_BIND_DIR "Players/Artifacts/Bind/"
#define ARTIFACT_BLOOD_DAYS 3
#define ARTIFACT_MAIN   0
#define ARTIFACT_UNIQUE 1
#define ARTIFACT_IOUN   2

extern P_obj object_list;
extern P_char character_list;
extern P_index obj_index;
extern int top_of_objt;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern char *artilist_mortal_main;
extern char *artilist_mortal_unique;
extern char *artilist_mortal_ioun;

void poof_arti( P_char ch, char *arg );
void swap_arti( P_char ch, char *arg );
void set_timer_arti( P_char ch, char *arg );

//
// setupMortArtiList : copies everything over from 'real' arti list, to
//                     be called once at boot, or whenever you want list
//                     mortals see to be updated
//

void setupMortArtiList(void)
{
  system("rm -f " ARTIFACT_MORT_DIR "*");
  system("cp " ARTIFACT_DIR "* " ARTIFACT_MORT_DIR);
}

void writeArtiList(const char *messg)
{
  int      i = 0;
  FILE    *f;
  char    *buf = 0;
  char     fname[256];
  char     buf2[MAX_STRING_LENGTH];

  f = fopen(MORTAL_ARTI_MAIN_FILE, "w+");
  if (!f)
    return;
  fprintf(f, "%s \0", messg);
  fclose(f);
}

void writeUniqueList(const char *messg)
{
  FILE    *f;
  char    *buf = 0;

  f = fopen(MORTAL_ARTI_UNIQUE_FILE, "w+");
  if (!f)
    return;
  fprintf(f, "%s \0", messg);
  fclose(f);
  buf = file_to_string(MORTAL_ARTI_UNIQUE_FILE);
  artilist_mortal_unique = buf;

}

void writeIounList(const char *messg)
{
  FILE    *f;
  char    *buf = 0;

  f = fopen(MORTAL_ARTI_IOUN_FILE, "w+");
  if (!f)
    return;
  fprintf(f, "%s \0", messg);
  fclose(f);

  buf = file_to_string(MORTAL_ARTI_IOUN_FILE);
  artilist_mortal_ioun = buf;

}
// a hack... if mod is a negative number (other than -1), it will
// force the arti timer to feed mod seconds (converted to a positive))
void UpdateArtiBlood(P_char ch, P_obj obj, int mod)
{
//  P_obj a;
//  int i = 0;
  FILE    *f;
  char     fname[256], name[256];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      vnum, id, last_time, uo, blood, oldtime, diff, newtime;

//  a = obj;
  if (mod != -1)

    if (obj_index[obj->R_num].virtual_number == 58 ||
        obj_index[obj->R_num].virtual_number == 59 ||
        obj_index[obj->R_num].virtual_number == 68)
    {
      if (obj->timer[3] > time(NULL) - (85 * 60 * 60))
        obj->timer[3] = time(NULL) - (85 * 60 * 60);
      return;
    }


//Spam the gods if more then 5 days passed since feed.
  if (mod == -1 && obj)
    return;
/*
  {


    blood = (int) ((time(NULL) - obj->timer[3]) / 86400);
    if (blood > 5 && (!isname("dragonslayer", obj->name)))
    {
      wizlog(56, "%s's %s was  removed from game. last feed: %d",
             GET_NAME(ch), obj->short_description, blood);
      statuslog(56, "%s's %s was  removed from game. last feed: %d",
                GET_NAME(ch), obj->short_description, blood);
      act
        ("$p&n &+ris no longer &+Rhappy&N&+r with its &+Rmaster&N&+r and &+Lvanishes&N&+r in an &+Wintense&N&+B en&+Yer&+Bgy&N&+b blast.\n\r.",
         0, ch, obj, 0, TO_ROOM);
      do_shout(ch, "Ouch!", 0);

      extract_obj(obj, TRUE);


      if (obj_index[obj->R_num].virtual_number == 58)
      {
        reset_lab(0);
        create_lab(0);
        return;
      }

      if (obj_index[obj->R_num].virtual_number == 68)
      {
        reset_lab(1);
        create_lab(1);
        return;
      }

      if (obj_index[obj->R_num].virtual_number == 59)
      {
        reset_lab(2);
        create_lab(2);
        return;
      }

      return;
    }
    return;
  }*/


  if (obj)
  {
    oldtime = obj->timer[3];
    diff = time(NULL) - oldtime;        //0-10

    bool bIsIoun = CAN_WEAR(obj, ITEM_WEAR_IOUN) ? true : false;
    bool bIsUnique = (isname("unique", obj->name) && !isname("powerunique", obj->name));
    bool bIsTrueArti = (!bIsIoun && !bIsUnique);

    if (diff > (86400 * 5))
    {
      diff = (86400 * 5);
      oldtime = time(NULL) - diff;
    }

    if (bIsTrueArti && (mod > -1))
    {
      struct obj_affect *af;
      int afMod = 0;
      int afLength = get_property("artifact.feeding.accum.timer", (int)60);
      
      af = get_obj_affect(obj, TAG_OBJ_RECENT_FRAG);
      if (af)
        afMod = af->data;
      affect_from_obj(obj, TAG_OBJ_RECENT_FRAG);
      
      // conditions for feed:
      //  prevMod is -1, or
      //  mod > 20, or
      //  mod+prevMod > 30
      if (mod <= get_property("artifact.feeding.single.min", (int)20))  // if not an insta-feed...
      {
        // if not qualified to feed from accumulated feeds...
        if ((-1 != afMod) && ((afMod + mod) <= get_property("artifact.feeding.accum.min", (int)30)))
        {
    	    send_to_char("&+RYou feel a brief sense of satisfaction before it fades into nothing.\r\n",
    		               ch);
    		  // set a new affect with the total frags
    		  mod += afMod;
    		  
          statuslog(56, "ArtiBlood: artifact #%d DEFERRED %.02f frags", 
                    obj_index[obj->R_num].virtual_number, mod/100.0f);
    		  
          set_obj_affected(obj, WAIT_SEC * afLength, TAG_OBJ_RECENT_FRAG, mod);
          return;
        }
        else if (-1 != afMod) // else if its a new qualified accumulation
        {
          mod += afMod; // this SHOULD be greater than 30
        }
      }
      // feed WILL occur on the arti.  Therefore, set a -1 frag timer so
      // any frags after this will automagically feed within a minute
      afMod = -1;
      set_obj_affected(obj, WAIT_SEC * afLength, TAG_OBJ_RECENT_FRAG, afMod);
    }  // bIsTrueArti
    
    if (mod > 1)
    {
      statuslog(56, "ArtiBlood: %s #%d fed %.02f frags", 
                bIsIoun ? "ioun" : bIsUnique ? "unique" : "artifact",
                obj_index[obj->R_num].virtual_number, mod/100.0f);
      diff = (86400 * 5 * mod) / 100;
      obj->timer[3] = oldtime + diff;
      if ((oldtime + diff) > time(NULL))
        obj->timer[3] = time(NULL);
    }
    else if (mod < -1)
    {
      diff = 0 - mod;
      obj->timer[3] = oldtime + diff;
      if ((oldtime + diff) > time(NULL))
        obj->timer[3] = time(NULL);      
    }

    if (mod >= 100)
    {
      send_to_char
        ("&+RYou feel a deep sense of satisfaction from somewhere...\r\n",
         ch);
    }
    else
    {
      send_to_char
        ("&+RYou feel a light sense of satisfaction from somewhere...\r\n",
         ch);
    }


//        act("Your $q quietly hums briefly.", FALSE, ch, obj, 0, TO_CHAR);
    vnum = obj_index[obj->R_num].virtual_number;
    sprintf(fname, ARTIFACT_DIR "%d", vnum);
    f = fopen(fname, "rt");
    blood = obj->timer[3];
    // wizlog(56, "Artifact looted: %s now owns %s (#%d).", GET_NAME(ch), obj->short_description, vnum);
    // wizlog(56, "Artifact timer reset: %s now owns %s (#%d).", GET_NAME(ch), obj->short_description, vnum);

    if (f)
    {
      fscanf(f, "%s %d %d %d %d", name, &id, &last_time, &uo, &blood);

      if ((id != GET_PID(ch)) && !uo)
      {
        statuslog(56,
                  "tried to track arti vnum #%d on %s when already tracked on %s.",
                  vnum, GET_NAME(ch), name);
      }

      fclose(f);
    }
    else
    {
      f = fopen(fname, "wt");
      if (!f)
      {
        statuslog(56, "could not open arti file %s for writing", fname);
        return;
      }
      fprintf(f, "%s %d %d 0 %d", GET_NAME(ch), GET_PID(ch), time(NULL),
              obj->timer[3]);
      fclose(f);
//                wizlog(56, "Artifact timer reset: %s now owns %s (#%d).", GET_NAME(ch), name, vnum);
    }
  }
}

void feed_artifact(P_char ch, P_obj obj, int feed_seconds, int bypass)
{
  if( !ch || !obj )
  {
    statuslog(56, "feed_artifact(): called with null ch or obj");
    return;
  }

  int owner_pid, timer, vnum;
  
  vnum = GET_OBJ_VNUM(obj);
  sql_get_bind_data(vnum, &owner_pid, &timer);

  // anti artifact sharing for feeding check
  if ( !bypass && IS_PC(ch) && (owner_pid != -1) && (owner_pid != GET_PID(ch)) )
  {
    act("&+L$p &+Lhas yet to accept you as its owner.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  int time_now = time(NULL);

  //time_now += (5 * 86400); //old 205 hours - drannak
  
  if( obj->timer[3] < ( time_now - (5 * 86400) ) )
    obj->timer[3] = ( time_now - (5 * 86400) );
  
  obj->timer[3] += feed_seconds;
  if( obj->timer[3] > (time_now + (5 * 86400)))
    obj->timer[3] = (time_now + 432000); //allowing people to feed over current limit (drannak)
    
  statuslog(56, "Artifact: %s [%d] on %s fed [&+G%ld&+Lh &+G%ld&+Lm &+G%ld&+Ls&n]", 
            obj->short_description, GET_OBJ_VNUM(obj), GET_NAME(ch),
            feed_seconds / 3600, (feed_seconds / 60) % 60, feed_seconds % 60 );
  
  if( feed_seconds > ( 12 * 3600 ) )
  {  
    send_to_char("&+RYou feel a deep sense of satisfaction from somewhere...\r\n", ch);
  }
  else
  {
    send_to_char("&+RYou feel a light sense of satisfaction from somewhere...\r\n", ch);
  }
  
  /*
   save to artifact files:
   try to load the artifact file; if it already exists, then it's already being tracked on another player.
   otherwise, update the time remaining timer
   */
  
  FILE     *f;
  char     fname[256], name[256];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      id, last_time, uo, blood, oldtime, diff, newtime;
  
  vnum = GET_OBJ_VNUM(obj);
  sprintf(fname, ARTIFACT_DIR "%d", vnum);
  f = fopen(fname, "rt");
  blood = obj->timer[3];
  
  if (f)
  {
    fscanf(f, "%s %d %d %d %d", name, &id, &last_time, &uo, &blood);
    
    if ((id != GET_PID(ch)) && !uo)
    {
      statuslog(56,
                "tried to track arti vnum #%d on %s when already tracked on %s.",
                vnum, GET_NAME(ch), name);
    }
    
    fclose(f);
  }
  else
  {
    f = fopen(fname, "wt");
    if (!f)
    {
      statuslog(56, "could not open arti file %s for writing", fname);
      return;
    }
    fprintf(f, "%s %d %d 0 %d", GET_NAME(ch), GET_PID(ch), time(NULL),
            obj->timer[3]);
    fclose(f);
  }
    
}


//
// add_owned_artifact : returns FALSE on error - does what you might expect
//                      otherwise
//
//  arti : arti obj
//    ch : char who now 'owns' arti
//

int add_owned_artifact(P_obj arti, P_char ch, int blood)
{
  FILE    *f;
  char     fname[256], name[256];
  int      exist = 0, vnum, id, last_time, uo, t_blood;


  if (!IS_ARTIFACT(arti) || !ch || IS_TRUSTED(ch) || IS_NPC(ch))
    return FALSE;

  vnum = obj_index[arti->R_num].virtual_number;

  sprintf(fname, ARTIFACT_DIR "%d", vnum);

  f = fopen(fname, "rt");

  // any pre-existing arti getting reflagged should belong to same player -
  // if not, spam status log on mud

  if (f)
  {
    exist = 1;
    fscanf(f, "%s %d %d %d %d", name, &id, &last_time, &uo, &t_blood);

    if ((id != GET_PID(ch)) && !uo)
    {
      statuslog(56,
                "tried to track arti vnum #%d on %s when already tracked on %s.",
                vnum, GET_NAME(ch), name);

      fclose(f);

      return FALSE;
    }

    // same player, fall through to rewrite

    fclose(f);
  }

  // didn't exist or falling through with same player owned, updating time

  f = fopen(fname, "wt");
  // if(exist)
  if (f)
  {
    fprintf(f, "%s %d %d 0 %d", GET_NAME(ch), GET_PID(ch), time(NULL), blood);
    /*
       else
       fprintf(f, "%s %d %d 0 %d", GET_NAME(ch), GET_PID(ch), time(NULL), time(NULL));
     */

    fclose(f);
  }

  return TRUE;
}


//
// remove_owned_artifact : returns FALSE on error - does what you might expect
//                         otherwise
//
//  arti : arti obj
//    ch : char who currently 'owns' arti
//

int remove_owned_artifact(P_obj arti, P_char ch, int full_remove)
{
  char     fname[256], name[256];
  int      vnum, id, last_time, true_u, t_blood;
  FILE    *f;


  if (!IS_ARTIFACT(arti) || (ch && (IS_TRUSTED(ch) || IS_NPC(ch))))
    return FALSE;

  vnum = obj_index[arti->R_num].virtual_number;

  sprintf(fname, ARTIFACT_DIR "%d", vnum);

  if (full_remove)
  {
    unlink(fname);
  }
  else
  {
    f = fopen(fname, "rt");
    if (!f)
      return FALSE;

    fscanf(f, "%s %d %d %d %d", name, &id, &last_time, &true_u, &t_blood);

    if (true_u)
    {
      wizlog(56,
             "error: attempt to flag arti #%d as 'actually unowned' when already set",
             vnum);
      return FALSE;
    }

    fclose(f);

    f = fopen(fname, "wt");

    fprintf(f, "%s %d %d 1 %d", name, id, last_time, t_blood);

    fclose(f);
  }

  return TRUE;
}


//
// returns FALSE if not found or if error - pname/id/last_time only changed
// if function returns TRUE
//
// can specify obj by vnum or rnum - if rnum < 0, vnum is used
//

int get_current_artifact_info(int rnum, int vnum, char *pname, int *id,
                              time_t * last_time, int *truly_unowned,
                              int get_mort, time_t * blood)
{
  char     name[256];
  int      t_id, t_last_time, t_tu, t_blood;
  FILE    *f;
  P_char   owner;
  int      owner_race = 4;


  if (rnum >= 0)
    vnum = obj_index[rnum].virtual_number;

  if (get_mort)
    sprintf(name, ARTIFACT_MORT_DIR "%d", vnum);
  else
    sprintf(name, ARTIFACT_DIR "%d", vnum);

  f = fopen(name, "rt");

  if (!f)
    return FALSE;

  fscanf(f, "%s %d %d %d %d", name, &t_id, &t_last_time, &t_tu, &t_blood);

  fclose(f);

//  if (t_tu) return FALSE;

  if (pname)
  {
    owner = (struct char_data *) mm_get(dead_mob_pool);
    owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

    if (restoreCharOnly(owner, skip_spaces(name)) >= 0)
    {
      if (RACE_GOOD(owner))
        owner_race = 1;
      else if (RACE_EVIL(owner))
        owner_race = 2;
      else if (RACE_PUNDEAD(owner))
        owner_race = 3;
      else
        owner_race = 4;
      free_char(owner);
    }
    strcpy(pname, name);
  }
  if (id)
    *id = t_id;
  if (last_time)
    *last_time = t_last_time;
  if (truly_unowned)
    *truly_unowned = t_tu;
  if (blood)
    *blood = t_blood;

  return owner_race;
}

void list_artifacts(P_char ch, char *arg, int type)
{
  DIR     *dir;
  struct   dirent *dire;
  char     dirname[512];
  char     htmllist[8048];
  char     strn[8048], strn1[8048], strn2[256], pname[512], blooddate[256];
  int      vnum, id, t_uo;
  time_t   last_time, blood;
  int      blood_time;
  int      days = 0, hours = 0, minutes = 0;
  int      writehtml = 0;
  int      goodies = 0;
  int      evils = 0;
  int      undeads = 0;
  int      others = 0;
  int      owning_side;
  P_obj    obj;

  sprintf(strn1, "");

  if( type < ARTIFACT_MAIN || type > ARTIFACT_IOUN )
  {
    send_to_char( "Invalid artifact type.\n\r", ch );
    return;
  }

  if( !IS_TRUSTED(ch) && type == ARTIFACT_MAIN )
  {
    if( artilist_mortal_main )
    {
      send_to_char("&+YOwner               Artifact\r\n\r\n", ch);
      page_string(ch->desc, artilist_mortal_main, 0);
      return;
    }
  }
  if( !IS_TRUSTED(ch) && type == ARTIFACT_UNIQUE )
  {
    if( artilist_mortal_unique )
    {
      send_to_char("&+YOwner               Unique\r\n\r\n", ch);
      page_string(ch->desc, artilist_mortal_unique, 0);
      return;
    }
  }
  if( !IS_TRUSTED(ch) && type == ARTIFACT_IOUN )
  {
    if( artilist_mortal_ioun )
    {
      send_to_char("&+YOwner               Ioun\r\n\r\n", ch);
      page_string(ch->desc, artilist_mortal_ioun, 0);
      return;
    }
  }

  strcpy(dirname, ARTIFACT_DIR);
  dir = opendir(dirname);

  if (!dir)
  {
    sprintf(strn, "could not open arti dir (%s)\r\n", dirname);
    send_to_char(strn, ch);
    return;
  }

  if (IS_TRUSTED(ch))
    send_to_char
      ("&+YOwner               Time       Last Update                   Artifact\r\n\r\n",
       ch);
  else
    send_to_char("&+YOwner               Artifact\r\n\r\n", ch);

  while (dire = readdir(dir))
  {

    vnum = atoi(dire->d_name);
    if (!vnum)
      continue;

    owning_side =
      get_current_artifact_info(-1, vnum, pname, &id, &last_time, &t_uo,
                                !IS_TRUSTED(ch), &blood);

    if (!owning_side)
      continue;

    obj = read_object(vnum, VIRTUAL);

    if (!obj)
      continue;

    if ((type == ARTIFACT_IOUN && !CAN_WEAR(obj, ITEM_WEAR_IOUN)) ||
        (type != ARTIFACT_IOUN && CAN_WEAR(obj, ITEM_WEAR_IOUN)))
    {
      extract_obj(obj, FALSE);
      continue;
    }

    if ((type == ARTIFACT_UNIQUE && !isname("unique", obj->name)) ||
        (type != ARTIFACT_UNIQUE && isname("unique", obj->name)) ||
        (GET_LEVEL(ch) < 57 && isname("token", obj->name)))
    {
      extract_obj(obj, FALSE);
      continue;
    }

    switch (owning_side)
    {
    case 1:
      goodies++;
      break;
    case 2:
      evils++;
      break;
    case 3:
      undeads++;
      break;
    default:
      others++;
    }

    if (IS_TRUSTED(ch))
    {
      strcpy(strn2, ctime(&last_time));
      strn2[strlen(strn2) - 1] = '\0';
      //    blood_time = (float) (ARTIFACT_BLOOD_DAYS - ((time(NULL) - blood) / 86400));
      blood_time = (time(NULL) - blood);
      days = 0;
      minutes = 0;
      hours = 0;

      while (blood_time > 86399)
      {
        days++;
        blood_time -= 86400;
      }
      while (blood_time > 3599)
      {
        hours++;
        blood_time -= 3600;
      }
      minutes = blood_time / 60;

      sprintf(blooddate, "%d:%02d:%02d ", days, hours, minutes);

      sprintf(strn, "%-20s%-11s%-30s%s (#%d)%s\r\n",
              pname, blooddate, strn2, obj->short_description, vnum,
              t_uo ? " (on corpse)" : "");

      send_to_char(strn, ch);
    }
    else
    {
      sprintf(strn, "%-20s%s\r\n", pname, obj->short_description);

      send_to_char(strn, ch);
      sprintf(strn1 + strlen(strn1), "%s", strn);

      if( type == ARTIFACT_MAIN )
      {
        writehtml = 1;
      }
      else if( type == ARTIFACT_UNIQUE )
      {
        writehtml = 2;
      }
      else if( type == ARTIFACT_IOUN )
      {
        writehtml = 3;
      }
    }

    extract_obj(obj, FALSE);
  }

  sprintf(strn,
          "\r\n       &+r------&+LSummary&+r------&n\r\n"
              "         &+WGoodies:      %d&n\r\n"
              "         &+rEvils:        %d&n\r\n"
              "         &+WTotal:        %d\r\n",
          goodies, evils,
          goodies + evils);
  send_to_char(strn, ch);

  if (writehtml == 1 && !artilist_mortal_main)
  {
    strcat(strn1, strn);
    writeArtiList(strn1);
  }

  if (writehtml == 2 && !artilist_mortal_unique)
  {
    strcat(strn1, strn);
    writeUniqueList(strn1);
  }

  if (writehtml == 3 && !artilist_mortal_ioun)
  {
    strcat(strn1, strn);
    writeIounList(strn1);
  }

  closedir(dir);

}


void do_artifact(P_char ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];

  if (!ch)
    return;

  arg = one_argument( arg, buf );

  if( !buf || !*buf || is_abbrev(buf, "list") )
  {
    list_artifacts( ch, arg, ARTIFACT_MAIN );
    return;
  }

  if( buf && *buf && is_abbrev(buf, "unique") )
  {
    list_artifacts( ch, arg, ARTIFACT_UNIQUE );
    return;
  }

  if( buf && *buf && is_abbrev(buf, "ioun") )
  {
    list_artifacts( ch, arg, ARTIFACT_IOUN );
    return;
  }

  if( GET_LEVEL(ch) < FORGER )
  {
    send_to_char( "Valid arguments are list, unique or ioun.\n\r", ch );
    return;
  }

  if( buf && *buf && is_abbrev(buf, "poof") )
  {
    poof_arti( ch, arg );
    return;
  }

  if( buf && *buf && is_abbrev(buf, "swap") )
  {
    swap_arti( ch, arg );
    return;
  }

  if( buf && *buf && is_abbrev(buf, "timer") )
  {
    set_timer_arti( ch, arg );
    return;
  }

  send_to_char( "Valid arguments are list, unique, ioun, swap, "
    "poof or timer.\n\r", ch );

}

void event_artifact_poof(P_char ch, P_char victim, P_obj obj, void *data)
{
  if (obj->timer[3] == 0)
    obj->timer[3] = time(NULL);

  if (OBJ_WORN(obj))
    ch = obj->loc.wearing;
  else if (OBJ_CARRIED(obj))
    ch = obj->loc.carrying;
  else
    goto reschedule;

  if (!IS_TRUSTED(ch) && obj->timer[3] + (5 * 24 * 60 * 60) < time(NULL))
  {
    act("Your $p vanishes with a bright flash of light!", FALSE, ch,
        obj, 0, TO_CHAR);
    act("$n's $p suddenly vanishes in a bright flash of light!", FALSE,
        ch, obj, 0, TO_ROOM);

    do_shout(ch, "Ouch!", 0);
    extract_obj(obj, TRUE);
    return;
  }

#ifndef __NO_MYSQL__
  artifact_switch_check(ch, obj);
#endif

reschedule:
  add_event(event_artifact_poof, 10 * WAIT_SEC, 0, 0, obj, 0, 0, 0);
}

void artifact_switch_check(P_char ch, P_obj obj)
{
  int owner_pid, op, timer, t, vnum;

  // And make sure it's and artifact
  if (!IS_ARTIFACT(obj))
    return;

  if (IS_TRUSTED(ch))
    return;

  vnum = GET_OBJ_VNUM(obj);

  sql_get_bind_data(vnum, &owner_pid, &timer);

  op = owner_pid;
  t = timer;

  // If a pvp loot happened, and timeframe has passed, set to 0 for binding
  if ( (owner_pid == -1) && (timer+(60*(int)get_property("artifact.feeding.switch.lootallowance.min", 30)) < time(NULL)) )
  {
    owner_pid = 0;
  }

  // If we are ready to bind (from above, or because we picked arti up from it's load mob)
  if (!owner_pid && IS_PC(ch))
  {
    // Set the artifact to the player
    act("&+L$p &+Lmerges with your &+wsoul&+L.", FALSE, ch, obj, 0, TO_CHAR);
    owner_pid = GET_PID(ch);
  }
  
  // If object is bound, and not being held by the owner
  if ( IS_PC(ch) && (owner_pid != -1) && (owner_pid != GET_PID(ch)) )
  {
    // If by some chance, the timer wasn't set set it
    if (!timer)
    {
      timer = time(NULL);
    }
    // otherwise if the timer is due, set it to the new player
    else if ( (timer+(60*(int)get_property("artifact.feeding.switch.timer.min", 30))) < time(NULL) )
    {
      act("&+L$p &+Lmerges with your &+wsoul&+L.", FALSE, ch, obj, 0, TO_CHAR);
      owner_pid = GET_PID(ch);
      timer = 0;
    }
  }
  // or if object is bound by the currently carried player, make sure the timer is reset
  else if ( IS_PC(ch) && (owner_pid == GET_PID(ch)) )
  {
    timer = 0;
  }
 
  if ((op != owner_pid) || (t != timer))
  {
    sql_update_bind_data(vnum, &owner_pid, &timer);  
  }
}

void poof_arti( P_char ch, char *arg )
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  P_obj obj;
  P_char owner;
  DIR *dir;
  FILE *f;
  struct dirent *dire;
  int vnum = 0;
  int t_id, t_last_time, t_tu, t_blood, i;

  one_argument( arg, buf );

  send_to_char( "\n\r | ", ch );
  send_to_char( buf, ch );
  send_to_char( " |\n\r", ch );

  // Check for artifact in game:
  obj = object_list;
  while( obj )
  {
    // Skip objects held by gods.
    if( OBJ_CARRIED(obj) && IS_TRUSTED(obj->loc.carrying)
      || OBJ_WORN(obj) && IS_TRUSTED(obj->loc.wearing) )
    {
      obj = obj->next;
      continue;
    }
    if( IS_ARTIFACT(obj) && isname(buf, obj->name) )
      break;
    obj = obj->next;
  }

  if( obj )
  {
    if( OBJ_WORN(obj) || OBJ_CARRIED(obj) )
    {
      // Set timer to poof and update.
      obj->timer[3] = time(NULL) - 6 * 86400;
      // Yes, artifact_poof only cares about obj.
      event_artifact_poof(NULL, NULL, obj, NULL);
    }
    else
    {
      act("$p suddenly vanishes in a bright flash of light!", FALSE,
        NULL, obj, 0, TO_ROOM);
      act("$p suddenly vanishes in a bright flash of light!", FALSE,
        ch, obj, 0, TO_CHAR);
      extract_obj( obj, TRUE );
    }
    return;
  }

  // Arti is not in game, so check pfiles:
  dir = opendir(ARTIFACT_DIR);
  if (!dir)
  {
    sprintf(buf, "poof_arti: could not open arti dir (%s)\r\n", ARTIFACT_DIR);
    send_to_char(buf, ch);
    return;
  }

  while( dire = readdir(dir) )
  {
    vnum = atoi(dire->d_name);
    if (!vnum)
      continue;
    obj = read_object(vnum, VIRTUAL);
    if( isname( buf, obj->name) )
    {
      extract_obj(obj, FALSE);
      break;
    }
    vnum = 0;
    extract_obj(obj, FALSE);
  }
  if( vnum )
  {
    // Get name from arti file.
    sprintf(buf2, ARTIFACT_DIR "%d", vnum);
    f = fopen(buf2, "rt");
    if (!f)
    {
      sprintf(buf2, "poof_arti: could not open arti dir (%s)\r\n", buf );
      send_to_char(buf2, ch);
      return;
    }
    fscanf(f, "%s %d %d %d %d", buf2, &t_id, &t_last_time, &t_tu, &t_blood);
    fclose(f);

    // Load pfile
    owner = (P_char) mm_get(dead_mob_pool);
    clear_char(owner);
    owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
    owner->desc = NULL;
    if( restoreCharOnly( owner, buf2 ) < 0 )
    {
      sprintf( buf, "poof_arti: %s has bad pfile.\n\r", buf2 );
      send_to_char( buf, ch );
      return;
    }
    restoreItemsOnly( owner, 100 );
    owner->next = character_list;
    character_list = owner;
    setCharPhysTypeInfo( owner );
    // Find/Poof arti
    for( i = 0; i < MAX_WEAR; i++ )
    {
      if( !owner->equipment[i] )
        continue;
      if( obj_index[owner->equipment[i]->R_num].virtual_number == vnum )
        break;
    }
    if( i == MAX_WEAR )
    {
      obj = owner->carrying;
      while( obj )
      {
        if( obj_index[obj->R_num].virtual_number == vnum )
          break;
        obj = obj->next_content;
      }
    }
    else
      obj = owner->equipment[i];
    // obj == artifact at this point or person doesn't have it?!
    if( !obj )
    {
      sprintf( buf3, "Arti '%s' %d is not on %s's pfile.", buf, vnum, buf2 );
      wizlog( 56, buf3 );
    }
    else
    {
      obj->timer[3] = time(NULL) - 6 * 86400;
      event_artifact_poof(NULL, NULL, obj, NULL);
      writeCharacter( owner, RENT_POOFARTI, owner->in_room );
    }

    // Free memory
    extract_char( owner );
  }
  else
  {
    send_to_char( "Could not find artifact '", ch );
    send_to_char( buf, ch );
    send_to_char( "'.\n\r", ch );
  }
}

void swap_arti( P_char ch, char *arg )
{
  char arti1name[MAX_STRING_LENGTH];
  char arti2name[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  P_obj arti1;
  P_obj arti2;
  P_char owner = NULL;
  DIR *dir;
  FILE *f;
  struct dirent *dire;
  int vnum = 0;
  int wearloc, i;
  int t_id, t_last_time, t_tu, t_blood;

  arg = one_argument( arg, arti1name );
  arg = one_argument( arg, arti2name );

  if( strlen(arti1name) == 0 || strlen(arti2name) == 0 )
  {
    send_to_char( "Format is 'artifact swap <artifact> <artifact>.\n\r", ch );
    return;
  }

  send_to_char( "\n\r | ", ch );
  send_to_char( arti1name, ch );
  send_to_char( " | ", ch );
  send_to_char( arti2name, ch );
  send_to_char( " |\n\r", ch );

  // Find arti2 in game:
  arti2 = object_list;
  while( arti2 )
  {
    // Skip objects held by gods.
    if( OBJ_CARRIED(arti2) && IS_TRUSTED(arti2->loc.carrying)
      || OBJ_WORN(arti2) && IS_TRUSTED(arti2->loc.wearing) )
    {
      arti2 = arti2->next;
      continue;
    }
    if( IS_ARTIFACT(arti2) && isname(arti2name, arti2->name) )
      break;
    arti2 = arti2->next;
  }

  // Arti2 is not in game, so check pfiles:
  if( !arti2 )
  {
    dir = opendir(ARTIFACT_DIR);
    if (!dir)
    {
      sprintf(buf, "swap_arti: could not open arti dir (%s)\r\n", ARTIFACT_DIR);
      send_to_char(buf, ch);
      return;
    }
    while( dire = readdir(dir) )
    {
      vnum = atoi(dire->d_name);
      if( !vnum )
        continue;
      arti2 = read_object(vnum, VIRTUAL);
      if( isname( arti2name, arti2->name) )
      {
        extract_obj(arti2, FALSE);
        arti2 = NULL;
        break;
      }
      vnum = 0;
      extract_obj(arti2, FALSE);
      arti2 = NULL;
    }
  }
  // If found, send error arti2 already in game and return
  if( arti2 || vnum )
  {
    sprintf(buf, "Artifact '%s' already in game.\r\n", arti2name);
    send_to_char(buf, ch);
    if( arti2 )
      sprintf( buf, "Artifact: %s.\n\r", arti2->short_description );
    else
      sprintf( buf, "Artifact: %d.\n\r", vnum );
    send_to_char(buf, ch);
    return;
  }

  // Find arti1 in game or save file.
  // Check for artifact in game:
  arti1 = object_list;
  while( arti1 )
  {
    // Skip objects held by gods.
    if( OBJ_CARRIED(arti1) && IS_TRUSTED(arti1->loc.carrying)
      || OBJ_WORN(arti1) && IS_TRUSTED(arti1->loc.wearing) )
    {
      arti1 = arti1->next;
      continue;
    }
    if( IS_ARTIFACT(arti1) && isname(arti1name, arti1->name) )
      break;
    arti1 = arti1->next;
  }
  // If not found, check save files
  vnum = 0;
  if( !arti1 )
  {
    dir = opendir(ARTIFACT_DIR);
    if (!dir)
    {
      sprintf(buf, "swap_arti: could not open arti dir (%s)\r\n", ARTIFACT_DIR);
      send_to_char(buf, ch);
      return;
    }
    while( dire = readdir(dir) )
    {
      vnum = atoi(dire->d_name);
      if( !vnum )
        continue;
      arti1 = read_object(vnum, VIRTUAL);
      if( isname( arti1name, arti1->name) )
      {
        extract_obj(arti1, FALSE);
        arti1 = NULL;
        break;
      }
      vnum = 0;
      extract_obj(arti1, FALSE);
      arti1 = NULL;
    }
  }
  // If vnum then arti is on pfile.
  if( vnum )
  {
    // Get name from arti file.
    sprintf(buf, ARTIFACT_DIR "%d", vnum);
    f = fopen(buf, "rt");
    if (!f)
    {
      sprintf(buf2, "swap_arti: could not open arti dir (%s)\r\n", buf );
      send_to_char(buf2, ch);
      return;
    }
    fscanf(f, "%s %d %d %d %d", buf2, &t_id, &t_last_time, &t_tu, &t_blood);
    fclose(f);

    // Load pfile
    owner = (P_char) mm_get(dead_mob_pool);
    clear_char(owner);
    owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
    owner->desc = NULL;
    if( restoreCharOnly( owner, buf2 ) < 0 )
    {
      sprintf( buf, "poof_arti: %s has bad pfile.\n\r", buf2 );
      send_to_char( buf, ch );
      return;
    }
    restoreItemsOnly( owner, 100 );
    owner->next = character_list;
    character_list = owner;
    setCharPhysTypeInfo( owner );
    // Find arti1 on owner.
    arti1 = owner->carrying;
    while( arti1 )
    {
      if( obj_index[arti1->R_num].virtual_number == vnum )
        break;
      arti1 = arti1->next_content;
    }
    // If not in inventory, check equipment
    if( !arti1 )
    {
      for( wearloc = 0; wearloc < MAX_WEAR; wearloc++ )
      {
        if( !owner->equipment[wearloc] )
          continue;
        if( obj_index[owner->equipment[wearloc]->R_num].virtual_number == vnum )
        {
          arti1 = owner->equipment[wearloc];
          break;
        }
      }
    }
  }
  if( arti1 )
  {
    // Find arti2 and load it.
    for( i = 0;i <= top_of_objt; i++ )
    {
      if( isname( arti2name, obj_index[i].keys ) )
      {
        // load arti2.
        arti2 = read_object(i, REAL);
        if( !arti2 )
        {
          send_to_char( "Could not load arti2.\n\r", ch );
          if( vnum )
            extract_char( owner );
          return;
        }
        if( IS_ARTIFACT(arti2) )
          break;
        else
        {
          // free arti2.
          extract_obj( arti2, TRUE );
          arti2 = NULL;
        }
      }
    }
    if( !arti2 )
    {
      sprintf( buf, "Could not find artifact '%s'.\n\r", arti2name );
      send_to_char( buf, ch );
      if( vnum )
        extract_char( owner );
      return;
    }

    // Set timer and swap.
    arti2->timer[3] = arti1->timer[3];
    if( !OBJ_ROOM(arti1) )
    {
      // Assumption: arti1 is not in a container, and not nowhere.
      // Thus, if it's not in a room, it's on a person.
      if( !owner )
      {
        if (OBJ_WORN(arti1))
          owner = arti1->loc.wearing;
        else if (OBJ_CARRIED(arti1))
          owner = arti1->loc.carrying;
        else
        {
          sprintf( buf, "Artifact '%s' in undefined position.\r\n", arti1name );
          send_to_char( buf, ch );
          return;
        }
      }
      // If worn, need to stick arti2 in its slot.
      if( OBJ_WORN( arti1 ) )
      {
        for( wearloc = 0; wearloc < MAX_WEAR;wearloc++ )
          if( owner->equipment[wearloc] == arti1 )
            break;
        // This should never happen, but better safe than sorry.
        if( wearloc == MAX_WEAR )
        {
          sprintf( buf, "Artifact '%s' not on owner, but is worn!?\r\n", arti1name );
          send_to_char( buf, ch );
          if( vnum )
            extract_char( owner );
          return;
        }
        act( "$p suddenly tranforms in a bright flash of light!", FALSE,
          NULL, arti1, 0, TO_ROOM );
        act( "$p suddenly tranforms in a bright flash of light!", FALSE,
          owner, arti1, 0, TO_CHAR );
        act( "$p suddenly tranforms in a bright flash of light!", FALSE,
          ch, arti1, 0, TO_CHAR );
        // Remove arti1 & poof it.
        unequip_char( owner, wearloc );
        extract_obj( arti1, TRUE );
        // Put arti2 in inventory.
        obj_to_char( arti2, owner );
      }
      else if (OBJ_CARRIED(arti1))
      {
        act( "$p suddenly tranforms in a bright flash of light!", FALSE,
          NULL, arti1, 0, TO_ROOM );
        act( "$p suddenly tranforms in a bright flash of light!", FALSE,
          owner, arti1, 0, TO_CHAR );
        act( "$p suddenly tranforms in a bright flash of light!", FALSE,
          ch, arti1, 0, TO_CHAR );
        obj_from_char( arti1, TRUE );
        obj_to_char( arti2, owner );
      }
      else
      {
        sprintf( buf, "Artifact '%s' in undefined position!\r\n", arti1name );
        send_to_char( buf, ch );
        return;
      }
    }
    else
    {
      arti2->timer[3] = arti1->timer[3];
      obj_to_room( arti2, arti1->loc.room );
      act( "$p suddenly tranforms in a bright flash of light!", FALSE,
        NULL, arti1, 0, TO_ROOM );
      act( "$p suddenly tranforms in a bright flash of light!", FALSE,
        ch, arti1, 0, TO_CHAR );
      extract_obj( arti1, TRUE );
      return;
    }
  }
  else
  {
    // If not found, send error not found and return.
    sprintf( buf, "Could not find artifact '%s'.\r\n", arti1name );
    send_to_char( buf, ch );
    if( vnum )
      extract_char( owner );
    return;
  }
  // write pfile if applicable
  if( vnum )
  {
    writeCharacter( owner, RENT_SWAPARTI, owner->in_room );
    extract_char( owner );
  }
}

void set_timer_arti( P_char ch, char *arg )
{
  char artiname[MAX_STRING_LENGTH];
  char strtimer[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  P_obj obj;
  P_char owner;
  DIR *dir;
  FILE *f;
  struct dirent *dire;
  int timer, t_id, t_last_time, t_tu, t_blood, i;
  int vnum = 0;

  arg = one_argument( arg, artiname );
  arg = one_argument( arg, strtimer );
  timer = atoi( strtimer );

  if( strlen(artiname) == 0 || strlen(strtimer) == 0 || timer <= 0 )
  {
    send_to_char( "Format is 'artifact timer <artifact> <minutes>'.\n\r", ch );
    return;
  }

  send_to_char( "\n\r | ", ch );
  send_to_char( artiname, ch );
  send_to_char( " | ", ch );
  send_to_char( strtimer, ch );
  send_to_char( " |\n\r", ch );

  // Find arti in game
  obj = object_list;
  while( obj )
  {
    // Skip objects held by gods.
    if( OBJ_CARRIED(obj) && IS_TRUSTED(obj->loc.carrying)
      || OBJ_WORN(obj) && IS_TRUSTED(obj->loc.wearing) )
    {
      obj = obj->next;
      continue;
    }
    if( IS_ARTIFACT(obj) && isname(artiname, obj->name) )
      break;
    obj = obj->next;
  }

  if( obj )
  {
    // Set timer to last timer minutes: current - 5 days + minutes,
    obj->timer[3] = time(NULL) - 5 * 86400 + 60 * timer;
    return;
  }

  // Arti is not in game, so check pfiles:
  dir = opendir(ARTIFACT_DIR);
  if (!dir)
  {
    sprintf(buf, "set_timer_arti: could not open arti dir (%s)\r\n", ARTIFACT_DIR);
    send_to_char(buf, ch);
    return;
  }
  while( dire = readdir(dir) )
  {
    vnum = atoi(dire->d_name);
    if (!vnum)
      continue;
    obj = read_object(vnum, VIRTUAL);
    if( isname( artiname, obj->name) )
    {
      extract_obj(obj, FALSE);
      break;
    }
    vnum = 0;
    extract_obj(obj, FALSE);
  }
  // If arti is on pfile
  if( vnum )
  {
    // Get name from arti file.
    sprintf(buf, ARTIFACT_DIR "%d", vnum);
    f = fopen(buf, "rt");
    if (!f)
    {
      sprintf(buf2, "set_timer_arti: could not open arti dir (%s)\r\n", buf );
      send_to_char(buf2, ch);
      return;
    }
    fscanf(f, "%s %d %d %d %d", buf2, &t_id, &t_last_time, &t_tu, &t_blood);
    fclose(f);

    // Load pfile
    owner = (P_char) mm_get(dead_mob_pool);
    clear_char(owner);
    owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
    owner->desc = NULL;
    if( restoreCharOnly( owner, buf2 ) < 0 )
    {
      sprintf( buf, "set_timer_arti: %s has bad pfile.\n\r", buf2 );
      send_to_char( buf, ch );
      return;
    }
    restoreItemsOnly( owner, 100 );
    owner->next = character_list;
    character_list = owner;
    setCharPhysTypeInfo( owner );
    // Find/Poof arti
    for( i = 0; i < MAX_WEAR; i++ )
    {
      if( !owner->equipment[i] )
        continue;
      if( obj_index[owner->equipment[i]->R_num].virtual_number == vnum )
        break;
    }
    if( i == MAX_WEAR )
    {
      obj = owner->carrying;
      while( obj )
      {
        if( obj_index[obj->R_num].virtual_number == vnum )
          break;
        obj = obj->next_content;
      }
    }
    else
      obj = owner->equipment[i];
    // obj == artifact at this point or person doesn't have it?!
    if( !obj )
    {
      sprintf( buf3, "Arti '%s' %d is not on %s's pfile.", buf, vnum, buf2 );
      wizlog( 56, buf3 );
    }
    else
    {
      obj->timer[3] = time(NULL) - 5 * 86400 + 60 * timer;
      writeCharacter( owner, RENT_INN, owner->in_room );
    }

    // Free memory
    extract_char( owner );
  }
  else
  {
    send_to_char( "Could not find artifact '", ch );
    send_to_char( buf, ch );
    send_to_char( "'.\n\r", ch );
  }
}
