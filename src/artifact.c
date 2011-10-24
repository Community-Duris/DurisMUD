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

#define ARTIFACT_DIR "Players/Artifacts/"
#define ARTIFACT_MORT_DIR "Players/Artifacts/Mortal/"
#define ARTI_BIND_DIR "Players/Artifacts/Bind/"
#define ARTIFACT_BLOOD_DAYS 3

extern P_index obj_index;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern char *artilist_mortal_main;
extern char *artilist_mortal_unique;
extern char *artilist_mortal_ioun;


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

  if (!(int)get_property("artifact.feed", 1))
  {
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
  
  if( obj->timer[3] < ( time_now - (5 * 86400) ) )
    obj->timer[3] = ( time_now - (5 * 86400) );
  
  obj->timer[3] += feed_seconds;
  if( obj->timer[3] > time_now )
    obj->timer[3] = time_now;
    
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


//
// do_list_artis
//

void do_list_artis(P_char ch, char *arg, int cmd)
{
  DIR     *dir;
  struct dirent *dire;
  char     htmllist[8048];
  char     strn[8048], strn1[8048], strn2[256], pname[512], dirname[512],
    blooddate[256];
  int      vnum, id, t_uo;
  int      show_iouns = 0;
  int      show_uniques = 0;
  time_t   last_time, blood;
  int      blood_time;
  int      days = 0, hours = 0, minutes = 0;
  P_obj    obj;
  int      writehtml = 0;
  int      goodies = 0;
  int      evils = 0;
  int      undeads = 0;
  int      others = 0;
  int      owning_side;

  if (!ch)
    return;

  sprintf(strn1, "");
//   send_to_char("&+YDisabled till we fix it, apprently this is crashing us once in a while.&n\r\n\r\n", ch);
//    return;

#ifdef THARKUN_ARTIS
  if (arg && *arg && is_abbrev(arg, "ioun") )
    show_iouns = 1;

  if (arg && *arg && is_abbrev(arg, "unique") )
    show_uniques = 1;
#else
  if (arg && *arg && is_abbrev(arg, "ioun"))
    show_iouns = 1;

  if (arg && *arg && is_abbrev(arg, "unique"))
    show_uniques = 1;
#endif


  if (!IS_TRUSTED(ch) && artilist_mortal_ioun && show_iouns && !show_uniques)
  {
    send_to_char("&+YOwner               Artifact\r\n\r\n", ch);
    page_string(ch->desc, artilist_mortal_ioun, 0);
    return;
  }
  if (!IS_TRUSTED(ch) && artilist_mortal_unique && !show_iouns &&
      show_uniques)
  {
    send_to_char("&+YOwner               Artifact\r\n\r\n", ch);
    page_string(ch->desc, artilist_mortal_unique, 0);
    return;
  }
  if (!IS_TRUSTED(ch) && artilist_mortal_main && !show_iouns && !show_uniques)
  {
    send_to_char("&+YOwner               Artifact\r\n\r\n", ch);
    page_string(ch->desc, artilist_mortal_main, 0);
    return;
  }


  //    strcpy(dirname, ARTIFACT_MORT_DIR);
  if (1)
  {
    strcpy(dirname, ARTIFACT_DIR);
//      artilist_mortal_main = file_to_string(MORTAL_ARTI_MAIN);
  }
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

    if ((show_iouns && !CAN_WEAR(obj, ITEM_WEAR_IOUN)) ||
        (!show_iouns && CAN_WEAR(obj, ITEM_WEAR_IOUN)))
    {
      extract_obj(obj, FALSE);
      continue;
    }

    if ((show_uniques && !isname("unique", obj->name)) ||
        (!show_uniques && isname("unique", obj->name)) ||
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
//      if (!t_uo)
//      {
      sprintf(strn, "%-20s%s\r\n", pname, obj->short_description);

      send_to_char(strn, ch);
      sprintf(strn1 + strlen(strn1), "%s", strn);

      if (show_iouns == FALSE && show_uniques == FALSE)
      {
        writehtml = 1;
      }
      else if (show_iouns == FALSE && show_uniques == TRUE)
      {
        writehtml = 2;
      }
      else if (show_iouns == TRUE && show_uniques == FALSE)
      {
        writehtml = 3;
      }
//      }
    }

    extract_obj(obj, FALSE);
  }

  sprintf(strn,
          "\r\n\t       &+r------&+LSummary&+r------&n\r\n\t\t&+WGoodies: \t%d&n\r\n\t\t&+rEvils: \t\t%d&n\r\n\t\t&+WTotal: \t\t&+W%d\r\n",
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
