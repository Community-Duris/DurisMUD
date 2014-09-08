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

#include "comm.h"
#include "db.h"
#include "files.h"
#include "mm.h"
#include "prototypes.h"
#include "spells.h"
#include "sql.h"
#include "structs.h"
#include "utility.h"
#include "utils.h"

#define ARTIFACT_DIR "Players/Artifacts/"
#define ARTIFACT_MORT_DIR "Players/Artifacts/Mortal/"
#define ARTI_BIND_DIR "Players/Artifacts/Bind/"
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
extern P_room world;

void poof_arti( P_char ch, char *arg );
void swap_arti( P_char ch, char *arg );
void set_timer_arti( P_char ch, char *arg );
void save_artifact_data( P_char owner, P_obj artifact );
int is_tracked( P_obj artifact );
void hunt_for_artis( P_char ch, char *arg );
void arti_clear( P_char ch, char *arg );
void nuke_eq( P_char );
void artifact_fight( P_char, P_obj );

// setupMortArtiList : copies everything over from 'real' arti list, to
//                     be called once at boot, or whenever you want list
//                     mortals see to be updated
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
// A hack... If mod is a negative number (other than -1), it will
//   force the arti timer to feed mod seconds (converted to a positive).
void UpdateArtiBlood(P_char ch, P_obj obj, int mod)
{
//  P_obj a;
//  int i = 0;
  FILE    *f;
  char     fname[256], name[256];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      vnum, id, uo, oldtime, diff, newtime;
  long unsigned last_time, blood;

//  a = obj;
  if (mod != -1)
  {
    // If arti is relic of gods (transforms).
    if (obj_index[obj->R_num].virtual_number == 58 ||
        obj_index[obj->R_num].virtual_number == 59 ||
        obj_index[obj->R_num].virtual_number == 68)
    {
      // 85 hrs ? WTH
      if (obj->timer[3] > time(NULL) - (85 * 60 * 60))
        obj->timer[3] = time(NULL) - (85 * 60 * 60);
      return;
    }
  }

  if( mod == -1 && obj )
    return;
/*
  {


    blood = (int) ((time(NULL) - obj->timer[3]) / SECS_PER_REAL_DAY);
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

  if( obj )
  {
    oldtime = obj->timer[3];
    diff = time(NULL) - oldtime;

    bool bIsIoun = CAN_WEAR(obj, ITEM_WEAR_IOUN);
    bool bIsUnique = (isname("unique", obj->name) && !isname("powerunique", obj->name));
    bool bIsTrueArti = (!bIsIoun && !bIsUnique);

    // If arti timer is longer than maximum number of days.
    if( diff > (SECS_PER_REAL_DAY * ARTIFACT_BLOOD_DAYS) )
    {
      diff = SECS_PER_REAL_DAY * ARTIFACT_BLOOD_DAYS;
      oldtime = time(NULL) - diff;
    }

    if( bIsTrueArti && (mod > -1) )
    {
      struct obj_affect *af;
      int afMod = 0;
      int afLength = get_property("artifact.feeding.accum.timer", (int)60);

      if( (af = get_obj_affect(obj, TAG_OBJ_RECENT_FRAG)) != NULL )
      {
        afMod = af->data;
      }
      affect_from_obj(obj, TAG_OBJ_RECENT_FRAG);

      // conditions for feed:
      //  mod > artifact.feeding.single.min, or
      //  mod+prevMod > 30

      // if not an insta-feed (less than 1 min)...
      if( mod <= get_property("artifact.feeding.single.min", 20) )
      {
        // if not qualified to feed from accumulated feeds...
        if( (-1 != afMod) && ((afMod + mod) <= get_property("artifact.feeding.accum.min", 30)) )
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
      diff = (SECS_PER_REAL_DAY * ARTIFACT_BLOOD_DAYS * mod) / 100;
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
      send_to_char("&+RYou feel a deep sense of satisfaction from somewhere...\r\n", ch);
    }
    else
    {
      send_to_char("&+RYou feel a light sense of satisfaction from somewhere...\r\n", ch);
    }


//        act("Your $q quietly hums briefly.", FALSE, ch, obj, 0, TO_CHAR);
    vnum = obj_index[obj->R_num].virtual_number;
    sprintf(fname, ARTIFACT_DIR "%d", vnum);
    f = fopen(fname, "rt");
    blood = obj->timer[3];
    // wizlog(56, "Artifact looted: %s now owns %s (#%d).", GET_NAME(ch), obj->short_description, vnum);
    // wizlog(56, "Artifact timer reset: %s now owns %s (#%d).", GET_NAME(ch), obj->short_description, vnum);

    if( f )
    {
      fscanf(f, "%s %d %lu %d %lu\n", name, &id, &last_time, &uo, &blood);

      if ((id != GET_PID(ch)) && !uo)
      {
        statuslog(56, "UpdateArtiBlood: tried to track arti vnum #%d on %s when already tracked on %s.",
          vnum, GET_NAME(ch), name);
      }

      fclose(f);
    }
    else
    {
      save_artifact_data( ch, obj );
    }
  }
}

void feed_artifact(P_char ch, P_obj obj, int feed_seconds, int bypass)
{
  FILE *f;
  char  fname[256], name[256];
  int   id, uo;
  long unsigned last_time, blood;
  int   owner_pid, timer, vnum;
  int   time_now = time(NULL);

  if( !ch || !obj )
  {
    statuslog(56, "feed_artifact: called with null ch or obj");
    debug( "feed_artifact: called with ch (%s) and obj (%s) %d.", ch ? J_NAME(ch) : "NULL",
      obj ? obj->short_description : "NULL", obj ? GET_OBJ_VNUM(obj) : -1 );
    return;
  }

  // Stop artis from feeding at all.  Still want to save arti data though.
  //feed_seconds = 0;

  vnum = GET_OBJ_VNUM(obj);
  sql_get_bind_data(vnum, &owner_pid, &timer);

  // Anti artifact sharing for feeding check
  if( !bypass && IS_PC(ch) && (owner_pid != -1) && (owner_pid != GET_PID(ch)) )
  {
    act("&+L$p &+Lhas yet to accept you as its owner.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  // Not allowing people to have artis with timer over limit
  if( obj->timer[3] + feed_seconds > time_now )
  {
    // Keep track of how much arti actually fed.
    feed_seconds = time_now - obj->timer[3];
    obj->timer[3] = time_now;
  }
  else
  {
    obj->timer[3] += feed_seconds;
  }

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

  /* Save to artifact files:
   * Try to load the artifact file; if it already exists, then check if it's already being tracked on another player.
   * Otherwise, update the time remaining timer.
   */

  vnum = GET_OBJ_VNUM(obj);
  blood = obj->timer[3];

  sprintf(fname, ARTIFACT_DIR "%d", vnum);
  f = fopen(fname, "rt");
  if( f )
  {
    fscanf(f, "%s %d %lu %d %lu\n", name, &id, &last_time, &uo, &blood);

    // If PIDs don't match && not on corpse..
    if ((id != GET_PID(ch)) && !uo)
    {
      statuslog(56, "feed_artifact: tried to track arti vnum #%d on %s when already tracked on %s.",
        vnum, GET_NAME(ch), name);
    }
    // Arti was on corpse or PIDs match, so write new data and close file.
    else
    {
      fprintf(f, "%s %d %lu 0 %lu", GET_NAME(ch), GET_PID(ch), time(NULL), blood);
    }
    fclose(f);
  }
  // No artifact file, so create one.
  else
  {
    save_artifact_data( ch, obj );
  }
}


//
// add_owned_artifact : returns FALSE on error - does what you might expect
//                      otherwise
//
//  arti : arti obj
//    ch : char who now 'owns' arti
//
bool add_owned_artifact(P_obj arti, P_char ch, long unsigned blood)
{
  FILE *f;
  char  fname[256], name[256];
  int   vnum, id, uo;
  long unsigned last_time, t_blood;


  if( !arti || !IS_ARTIFACT(arti) || !ch )
  {
    debug( "add_owned_artifact: called with ch (%s) and obj (%s) %d.", ch ? J_NAME(ch) : "NULL",
      arti ? arti->short_description : "NULL", arti ? GET_OBJ_VNUM(arti) : -1 );
    return FALSE;
  }
  // This is ok.. no need to report it.
  if( IS_TRUSTED(ch) || IS_NPC(ch) )
  {
    return FALSE;
  }

  vnum = obj_index[arti->R_num].virtual_number;

  sprintf(fname, ARTIFACT_DIR "%d", vnum);

  f = fopen(fname, "rt");

  // Any pre-existing arti getting reflagged should belong to same player -
  //   If not, spam status log on mud.
  if( f )
  {
    fscanf(f, "%s %d %lu %d %lu\n", name, &id, &last_time, &uo, &t_blood);
    fclose(f);

    // If tracked on another player (not a corpse)...
    if ((id != GET_PID(ch)) && !uo)
    {
      statuslog(56, "add_owned_artifact: tried to track arti vnum #%d on %s when already tracked on %s.",
        vnum, GET_NAME(ch), name);

      return FALSE;
    }
  }

  // didn't exist or falling through with same player owned, updating time
  f = fopen(fname, "wt");
  // if(exist)
  if( f )
  {
    fprintf(f, "%s %d %lu 0 %lu", GET_NAME(ch), GET_PID(ch), time(NULL), blood);
    /*
       else
       fprintf(f, "%s %d %d 0 %d", GET_NAME(ch), GET_PID(ch), time(NULL), time(NULL));
     */

    fclose(f);
    return TRUE;
  }

  statuslog(56, "add_owned_artifact: Arti vnum #%d on %s, couldn't open file '%s'.",
    vnum, GET_NAME(ch), fname);
  return FALSE;
}


// remove_owned_artifact : returns FALSE on error
//   does what you might expect otherwise
//
//  arti        : arti obj
//    ch        : char who currently 'owns' arti
//  full_remove : Is arti removed or just gone to corpse?
int remove_owned_artifact(P_obj arti, P_char ch, int full_remove)
{
  char     fname[256], name[256];
  int      vnum, id, true_u;
  long unsigned last_time, t_blood;
  FILE    *f;

  // Bad arguments...
  if( !arti || !IS_ARTIFACT(arti) )
  {
    debug( "remove_owned_artifact: called with obj (%s) %d.",
      arti ? arti->short_description : "NULL", arti ? GET_OBJ_VNUM(arti) : -1 );
    return FALSE;
  }
  // This is ok.. no need to report it.
  if( ch && (IS_TRUSTED(ch) || IS_NPC(ch)) )
  {
    return FALSE;
  }

  vnum = obj_index[arti->R_num].virtual_number;

  sprintf(fname, ARTIFACT_DIR "%d", vnum);

  if (full_remove)
  {
    unlink(fname);
  }
  else
  {
    f = fopen(fname, "rt");
    if( !f )
    {
      statuslog(56, "remove_owned_artifact: Arti vnum #%d on %s, couldn't open file '%s'.",
        vnum, ch ? J_NAME(ch) : "Unknown", fname);
      return FALSE;
    }

    fscanf(f, "%s %d %lu %d %lu\n", name, &id, &last_time, &true_u, &t_blood);

    if( true_u )
    {
      wizlog( 56, "error: attempt to flag arti #%d as 'actually unowned' when already set.",
        vnum );
      return FALSE;
    }

    fclose(f);

    f = fopen(fname, "wt");
    if( !f )
    {
      statuslog(56, "remove_owned_artifact: Arti vnum #%d on %s, couldn't open file '%s'.",
        vnum, ch ? J_NAME(ch) : "Unknown", fname);
      return FALSE;
    }

    fprintf(f, "%s %d %lu 1 %lu", name, id, last_time, t_blood);
    fclose(f);
  }

  return TRUE;
}


//
// Returns owner's racewar side if owner is found.  Otherwise returns 0.
// pname/id/last_time/truly_unowned/blood changed if artifact file was
//   successfully read.
//
// can specify obj by vnum or rnum - if rnum < 0, vnum is used
//
int get_current_artifact_info( int rnum, int vnum, char *pname, int *id,
      time_t * last_time, int *truly_unowned, int get_mort, time_t *blood )
{
  char     name[256], fname[256];
  int      t_id, t_tu;
  long unsigned t_last_time, t_blood;
  FILE    *f;
  P_char   owner;
  int      owner_race;


  if (rnum >= 0)
  {
    vnum = obj_index[rnum].virtual_number;
  }

  if( get_mort )
  {
    sprintf(fname, ARTIFACT_MORT_DIR "%d", vnum);
  }
  else
  {
    sprintf(fname, ARTIFACT_DIR "%d", vnum);
  }

  f = fopen(fname, "rt");

  if( !f )
  {
//  This is unnecessary and spammy since db.c calls without checking to see if vnum is an arti first.
//    statuslog(56, "get_current_artifact_info: Arti vnum #%d, couldn't open file '%s'.", vnum, name);
    return 0;
  }

  fscanf(f, "%s %d %lu %d %lu\n", name, &t_id, &t_last_time, &t_tu, &t_blood);

  fclose(f);

//  If on corpse return FALSE ?? why?
//  if (t_tu) return FALSE;

  // If we want the player's name..
  if( pname )
  {
    // Copy name into pname
    strcpy( pname, name );
  }

  owner = (struct char_data *) mm_get(dead_mob_pool);
  if (!dead_pconly_pool)
  {
    dead_pconly_pool = mm_create("PC_ONLY", sizeof(struct pc_only_data),
      offsetof(struct pc_only_data, switched),
      mm_find_best_chunk(sizeof (struct pc_only_data), 10, 25));
  }
  owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

  if( restoreCharOnly(owner, skip_spaces(name)) >= 0 )
  {
    if( RACE_GOOD(owner) )
    {
      owner_race = 1;
    }
    else if( RACE_EVIL(owner) )
    {
      owner_race = 2;
    }
    else if( RACE_PUNDEAD(owner) )
    {
      owner_race = 3;
    }
    else
    {
      owner_race = 4;
    }
    free_char(owner);
  }
  else
  {
    // Try to hunt for obj in room save.
    if( (f = fopen(fname, "rt")) )
    {
      sprintf( name, "%s", fread_string(f) );
      fscanf(f, " %d %lu %d %lu\n", &t_id, &t_last_time, &t_tu, &t_blood);
      if( pname )
      {
        // Copy name into pname
        strcpy( pname, name );
      }
      fclose(f);

      if( !str_cmp( name, world[real_room(t_id)].name ) )
      {
        owner_race = -1;
      }
      else
      {
        owner_race = 0;
      }
    }
    else
    {
      owner_race = 0;
    }
  }

  if( id )
  {
    *id = t_id;
  }
  if( last_time )
  {
    *last_time = t_last_time;
  }
  if( truly_unowned )
  {
    *truly_unowned = t_tu;
  }
  if( blood )
  {
    *blood = t_blood;
  }

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
  bool     mortal;

  sprintf(strn1, "");

  if( type < ARTIFACT_MAIN || type > ARTIFACT_IOUN )
  {
    send_to_char( "Invalid artifact type.\n\r", ch );
    return;
  }
  // Skip whitespaces..
  while ( *arg == ' ' )
  {
    arg++;
  }
  if( *arg && is_abbrev( arg, "mortal" ) )
  {
    mortal = TRUE;
  }
  else
  {
    mortal = FALSE;
  }
  if( (!IS_TRUSTED(ch) || mortal) && type == ARTIFACT_MAIN )
  {
    if( artilist_mortal_main )
    {
      send_to_char("&+YOwner               Artifact\r\n\r\n", ch);
      page_string(ch->desc, artilist_mortal_main, 0);
      return;
    }
  }
  if( (!IS_TRUSTED(ch) || mortal) && type == ARTIFACT_UNIQUE )
  {
    if( artilist_mortal_unique )
    {
      send_to_char("&+YOwner               Unique\r\n\r\n", ch);
      page_string(ch->desc, artilist_mortal_unique, 0);
      return;
    }
  }
  if( (!IS_TRUSTED(ch) || mortal) && type == ARTIFACT_IOUN )
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

  if( IS_TRUSTED(ch) && !mortal )
    send_to_char("&+YOwner               Time       Last Update                   Artifact\r\n\r\n", ch);
  else
    send_to_char("&+YOwner               Artifact\r\n\r\n", ch);

  while (dire = readdir(dir))
  {

    vnum = atoi(dire->d_name);
    if (!vnum)
      continue;

    owning_side = get_current_artifact_info(-1, vnum, pname, &id, &last_time, &t_uo,
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

    if (IS_TRUSTED(ch) && !mortal )
    {
      strcpy(strn2, ctime(&last_time));
      strn2[strlen(strn2) - 1] = '\0';
//      blood_time = (float) (ARTIFACT_BLOOD_DAYS - ((time(NULL) - blood) / SECS_PER_REAL_DAY));
      blood_time = ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY -(time(NULL) - blood);
      days = 0;
      minutes = 0;
      hours = 0;

      while (blood_time > 86399)
      {
        days++;
        blood_time -= SECS_PER_REAL_DAY;
      }
      while (blood_time > 3599)
      {
        hours++;
        blood_time -= 3600;
      }
      minutes = blood_time / 60;

      sprintf(blooddate, "%d:%02d:%02d ", days, hours, minutes);
      if( t_uo == -1 )
      {
        sprintf(strn, "%s\n%28s   %-30s%s (#%d)%s\r\n",
              pname, blooddate, strn2, obj->short_description, vnum,
              "(on ground)" );
      }
      else
      {
        sprintf(strn, "%-20s%-11s%-30s%s (#%d)%s\r\n",
              pname, blooddate, strn2, obj->short_description, vnum,
              t_uo ? "(on corpse)" : "");
      }
      send_to_char(strn, ch);
    }
    // Don't show artis on ground to mortals.
    else if( t_uo != -1 )
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
  // Skip whitespaces..
  while ( *arg == ' ' )
  {
    arg++;
  }

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

  if( GET_LEVEL(ch) < FORGER || IS_NPC(ch) )
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

  if( buf && *buf && is_abbrev(buf, "hunt") )
  {
    hunt_for_artis( ch, arg );
    return;
  }

  if( buf && *buf && is_abbrev(buf, "clear") )
  {
    arti_clear( ch, arg );
    return;
  }

  send_to_char( "Valid arguments are list, unique, ioun, swap, "
    "poof, timer, hunt or clear.\n\r", ch );

}

void event_artifact_poof(P_char ch, P_char victim, P_obj obj, void *data)
{
  if( !obj )
  {
    statuslog(56, "event_artifact_poof: called with null obj");
    debug( "event_artifact_poof: called with null obj." );
    return;
  }

/* If the arti hasn't had it's timer set yet.. hrm. poof it!
  if( obj->timer[3] == 0 )
    obj->timer[3] = time(NULL);
*/

  if( OBJ_WORN(obj) )
    ch = obj->loc.wearing;
  else if( OBJ_CARRIED(obj) )
    ch = obj->loc.carrying;
  // If it's on ground..
  else
  {
    if( obj->timer[3] + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY <= time(NULL) )
    {
      act("$p suddenly vanishes in a bright flash of light!", FALSE,
        NULL, obj, 0, TO_ROOM);
      act("$p suddenly vanishes in a bright flash of light!", FALSE,
        ch, obj, 0, TO_CHAR);
      extract_obj( obj, TRUE );
      return;
    }
    else
    {
      goto reschedule;
    }
  }
  // Important that this is <= and not just <.
  if( !IS_TRUSTED(ch) && obj->timer[3] + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY <= time(NULL) )
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
  int t_id, t_tu, i;
  long t_last_time, t_blood;
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
      obj->timer[3] = time(NULL) - ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
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
  closedir(dir);
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
    fscanf(f, "%s %d %lu %d %lu\n", buf2, &t_id, &t_last_time, &t_tu, &t_blood);
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
      nuke_eq( owner );
    }
    else
    {
      obj->timer[3] = time(NULL) - ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
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
  int t_id, t_tu;
  long unsigned t_last_time, t_blood;

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
  closedir(dir);
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

  // Now we know arti2 is not in the game or on a pfile.
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
  vnum = 0;
  // If not found in game, check save files
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
    closedir(dir);
  }
  // If !arti1 and !vnum then nothing to swap.
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
    fscanf(f, "%s %d %lu %d %lu\n", buf2, &t_id, &t_last_time, &t_tu, &t_blood);
    fclose(f);

    // Load pfile
    owner = (P_char) mm_get(dead_mob_pool);
    clear_char(owner);
    owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
    owner->desc = NULL;
    if( restoreCharOnly( owner, buf2 ) < 0 )
    {
      sprintf( buf, "swap_arti: %s has bad pfile.\n\r", buf2 );
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
          {
            nuke_eq( owner );
            extract_char( owner );
          }
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
      {
        nuke_eq( owner );
        extract_char( owner );
      }
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
          extract_obj( arti2, FALSE );
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
          extract_obj( arti2, FALSE );
          if( vnum )
          {
            nuke_eq( owner );
            extract_char( owner );
          }
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
        extract_obj( arti1, TRUE );
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
    // If vnum then owner was loaded, but not sent to room.
    if( vnum )
    {
      nuke_eq( owner );
      extract_char( owner );
    }
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
  int timer, t_id, t_tu, i;
  long unsigned t_last_time, t_blood;
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
    // Set timer to last timer minutes: current - ARTIFACT_BLOOD_DAY days + minutes,
    obj->timer[3] = time(NULL) - ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY + 60 * timer;
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
  closedir(dir);
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
    fscanf(f, "%s %d %lu %d %lu\n", buf2, &t_id, &t_last_time, &t_tu, &t_blood);
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
    // Find arti
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
      obj->timer[3] = time(NULL) - ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY + 60 * timer;
      // Update the artifact file.
      save_artifact_data( owner, obj );
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

void event_check_arti_poof( P_char ch, P_char vict, P_obj obj, void * arg )
{
  DIR           *dir;
  FILE          *f;
  struct dirent *dire;
  int            vnum, i;
  char           name[256];
  int            t_id, t_tu;
  long unsigned  t_last_time, t_blood;
  P_char         owner;
  P_obj          item;

  debug( "event_check_arti_poof: beginning..." );

  // Open the arti directory!
  dir = opendir(ARTIFACT_DIR);
  if (!dir)
  {
    statuslog( 56, "event_check_arti_poof: could not open arti dir (%s)\r\n", ARTIFACT_DIR );
    wizlog( 56, "event_check_arti_poof: could not open arti dir (%s)\r\n", ARTIFACT_DIR );
    return;
  }
  // Loop through arti files..
  while( dire = readdir(dir) )
  {
    vnum = atoi(dire->d_name);
    if (!vnum)
      continue;
    debug( "event_check_arti_poof: Checking '%s'", dire->d_name );

    sprintf(name, ARTIFACT_DIR "%d", vnum);
    f = fopen(name, "rt");

    if (!f)
    {
      statuslog( 56, "event_check_arti_poof: could not open arti file (%s)\r\n", name );
      wizlog( 56, "event_check_arti_poof: could not open arti file (%s)\r\n", name );
      continue;
    }

    // Read arti file.
    fscanf(f, "%s %d %lu %d %lu\n", name, &t_id, &t_last_time, &t_tu, &t_blood);
    fclose(f);

    // If arti is overdue to poof..
    if( (ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY + t_blood) < time(NULL) )
    {
      statuslog( 56, "Poofing arti vnum %d", vnum );
      wizlog( 56, "Poofing arti vnum %d", vnum );
      // Load pfile
      owner = (P_char) mm_get(dead_mob_pool);
      clear_char(owner);
      owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
      owner->desc = NULL;
      if( restoreCharOnly( owner, name ) < 0 )
      {
        statuslog( 56, "event_check_arti_poof: could not restoreCharOnly '%s'", name );
        wizlog( 56, "event_check_arti_poof: could not restoreCharOnly '%s'", name );
        extract_char( owner );
        continue;
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
      // If not wearing it..
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
        statuslog( 56, "Arti %d is not on %s's pfile.", vnum, name );
        wizlog( 56, "Arti %d is not on %s's pfile.", vnum, name );
        sprintf(name, ARTIFACT_DIR "%d", vnum);
        unlink(name);
        // Remove eq from char and extract.
        nuke_eq( owner );
        extract_char( owner );
      }
      else
      {
        event_artifact_poof(NULL, NULL, obj, NULL);
        writeCharacter( owner, RENT_POOFARTI, owner->in_room );
        // Free memory
        extract_char( owner );
      }
    }
  }
  closedir(dir);

  debug( "event_check_arti_poof: ended." );
  // 3600 = 60sec * 60min => Repeat every one hour (not too important to have it sooner).
  add_event( event_check_arti_poof, 3600 * WAIT_SEC, ch, vict, obj, 0, arg, 0 );
}

// Saves artifact data for one artifact.
void save_artifact_data( P_char owner, P_obj artifact )
{
  int   vnum = obj_index[artifact->R_num].virtual_number;
  char  fname[256];
  FILE *f;

  sprintf(fname, ARTIFACT_DIR "%d", vnum);
  f = fopen(fname, "wt");

  if (!f)
  {
    statuslog(56, "save_artifact_data: could not open arti file '%s' for writing", fname);
    debug( "save_artifact_data: could not open arti file '%s' for writing", fname);
    return;
  }

  fprintf(f, "%s %d %lu 0 %lu", GET_NAME(owner), GET_PID(owner), time(NULL), artifact->timer[3]);

  fclose(f);
}

// Returns -1 if artifact is not tracked, 0 if not an arti, or PID of arti owner.
int is_tracked( P_obj artifact )
{
  char  fname[256];
  FILE *f;
  char  name[256];
  int   id, uo, res;
  long unsigned last_time, blood;

  // Not an artifact ?!?
  if( !artifact || !IS_ARTIFACT(artifact) )
  {
    debug( "is_tracked: passed non-artifact!" );
    return 0;
  }

  sprintf( fname, "%s%d", ARTIFACT_DIR, obj_index[artifact->R_num].virtual_number );
  f = fopen( fname, "r" );

  // Not tracked
  if( f == NULL )
  {
    debug( "is_tracked: couldn't open file '%s'.", fname );
    return -1;
  }

  // If file is corrupted?
  if( (res = fscanf(f, "%s %d %lu %d %lu\n", name, &id, &last_time, &uo, &blood)) < 5)
  {
    debug( "is_tracked: fscanf returned bad result: %d.", res );
    fclose(f);
    return -1;
  }

  fclose(f);
  return id;
}

void event_hunt_for_artis(P_char ch, P_char victim, P_obj obj, void *data)
{
  if( !IS_ALIVE(ch) || !data )
  {
    statuslog( 56, "event_hunt_for_artis: bad arg: ch '%s', data '%s'", ch ? J_NAME(ch) : "NULL", data != NULL ? (char *)data : "NULL" );
    debug( "event_hunt_for_artis: bad arg: ch '%s', data '%s'", ch ? J_NAME(ch) : "NULL", data != NULL ? (char *)data : "NULL" );
    return;
  }

  hunt_for_artis( ch, (char *)data );
}

// Searches through all pfiles with initial *arg for artis.
void hunt_for_artis( P_char ch, char *arg )
{
  char  buf[MAX_STRING_LENGTH];
  char  dname[256];
  char  fname[256];
  char  initial;
  int   wearloc, pid, count;
  DIR  *dir;
  P_obj arti;
  P_char owner;
  struct dirent *dire;

  if( atoi(arg) == 1 )
  {
    // Search for a without delay.
    hunt_for_artis( ch, "a" );
    // For the rest of the letters search for them with an incremented delay to prevent lag.
    for( initial = 'b', count = 1;initial <= 'z';initial++, count++ )
    {
      sprintf( buf, "%c", initial );
      add_event(event_hunt_for_artis, count, ch, NULL, NULL, 0, &buf, sizeof(buf));
    }
    return;
  }
  if( !*arg || !isalpha(*arg) )
  {
    send_to_char( "Arti hunt needs a letter for which initial to hunt for, or 1 to search all pfiles.\n", ch );
    send_to_char( "Arti hunt will then search all pfiles with said initial for artifacts, and add them to the arti list if necessary.\n", ch );
    return;
  }
  if( *arg >= 'A' && *arg <= 'Z' )
  {
    *arg = *arg + 'a' - 'A';
  }

  // Read & loop through the directory..
  // Open the directory!
  sprintf( dname, "%s/%c", SAVE_DIR, *arg );
  dir = opendir( dname );
  if( !dir )
  {
    statuslog( 56, "hunt_for_artis: could not open arti dir (%s)\r\n", ARTIFACT_DIR );
    debug( "hunt_for_artis: could not open arti dir (%s)\r\n", ARTIFACT_DIR );
    return;
  }
  // Loop through the directory files.
  while (dire = readdir(dir))
  {
    // Skip backup/locker files/etc
    if( strstr( dire->d_name, "." ) )
      continue;
    owner = (struct char_data *) mm_get(dead_mob_pool);
    owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
    if( restoreCharOnly(owner, dire->d_name) < 0 )
    {
      sprintf( buf, "hunt_for_artis: %s has bad pfile.\n\r", dire->d_name );
      send_to_char( buf, ch );
      free_char( owner );
      continue;
    }
    if( IS_TRUSTED( owner ) )
    {
      free_char( owner );
      continue;
    }
// For debugging only.. gets spammy on live mud.
//    sprintf( buf, "Hunting pfile of '%s'.\n", J_NAME(owner) );
//    send_to_char( buf, ch );
    restoreItemsOnly( owner, 100 );
    owner->next = character_list;
    character_list = owner;
    setCharPhysTypeInfo( owner );
    // Search each pfile:
    // Search Worn equipment.
    for( wearloc = 0; wearloc < MAX_WEAR;wearloc++ )
    {
      if( owner->equipment[wearloc] != NULL && IS_ARTIFACT(owner->equipment[wearloc]) )
      {
        sprintf( buf, "'%s' has arti %s (%d) : ", J_NAME(owner),
          pad_ansi(owner->equipment[wearloc]->short_description, 35).c_str(),
          obj_index[owner->equipment[wearloc]->R_num].virtual_number );
        send_to_char( buf, ch );
        if( (pid = is_tracked( owner->equipment[wearloc] )) == -1 )
        {
          send_to_char( "Not yet tracked!\n", ch );
          save_artifact_data( owner, owner->equipment[wearloc] );
        }
        else if( pid == GET_PID(owner) )
        {
          send_to_char( "Already tracked.\n", ch );
        }
        else if( pid == 0 )
        {
          send_to_char( "Not an arti!!!\n", ch );
        }
        else
        {
          sprintf( buf, "&+ROn another char:&N %s\n", get_player_name_from_pid(pid) );
          send_to_char( buf, ch );
        }
      }
    }
    // Search inventory.
    for( arti = owner->carrying; arti; arti = arti->next_content )
    {
      if( IS_ARTIFACT( arti ) )
      {
        sprintf( buf, "'%s' has arti %s (%d) : ", J_NAME(owner),
          pad_ansi(arti->short_description, 35).c_str(),
          obj_index[arti->R_num].virtual_number );
        send_to_char( buf, ch );
        if( (pid = is_tracked( arti )) == -1 )
        {
          send_to_char( "Not yet tracked!\n", ch );
          save_artifact_data( owner, arti );
        }
        else if( pid == GET_PID(owner) )
        {
          send_to_char( "Already tracked.\n", ch );
        }
        else if( pid == 0 )
        {
          send_to_char( "Not an arti!!!\n", ch );
        }
        else
        {
          sprintf( buf, "&+ROn another char:&N %s\n", get_player_name_from_pid(pid) );
          send_to_char( buf, ch );
        }
      }
    }
    nuke_eq( owner );
    extract_char( owner );
  }
  // Close the directory!
  closedir( dir );
  sprintf( buf, "Arti hunted '%c' successfully!\n", *arg );
  send_to_char( buf, ch );
}

// Nukes the arti list.  Must be careful with this!
void arti_clear( P_char ch, char *arg )
{
  DIR *dir;
  FILE *f;
  char name[256];
  char buf[256];
  struct dirent *dire;
  int vnum;

  if( vnum = atoi(arg) )
  {
    sprintf( name, "%s%d", ARTIFACT_DIR, vnum );
    if( f = fopen( name, "r" ) )
    {
      fclose( f );
      sprintf( buf, "Deleting file '%s'...\n", name );
      send_to_char( buf, ch );
      wizlog( 56, "%s: Deleting file '%s'...", J_NAME(ch), name );
      unlink( name );
    }
    else
    {
      sprintf( buf, "File '%s' does not exist...\n", name );
      send_to_char( buf, ch );
    }
    return;
  }

  if( !isname(arg, "confirm") )
  {
    send_to_char( "You must type 'artifact clear confirm' to delete all artifact files.\n", ch );
    send_to_char( "You can type 'artifact clear <vnum>' to a single artifact file.\n", ch );
    send_to_char( "'artifact clear confirm' is probably a bad idea unless debugging.\n", ch );
    return;
  }

  send_to_char( "Deleting ALL live artifact files!!!\n", ch );
  wizlog( 56, "%s: Deleting ALL live artifact files!!!", J_NAME(ch) );
  dir = opendir( ARTIFACT_DIR );
  while( dire = readdir(dir) )
  {
    vnum = atoi(dire->d_name);
    if( !vnum )
      continue;
    sprintf( name, "%s%d", ARTIFACT_DIR, vnum );
    closedir( dir );
    wizlog( 56, "%s: Deleting file '%s'...", J_NAME(ch), name );
    sprintf( buf, "Deleting file '%s'...\n", name );
    send_to_char( buf, ch );
    unlink( name );
    dir = opendir( ARTIFACT_DIR );
  }
  closedir( dir );
}

// Removes an unsaved char's equipment from game without disturbing arti list.
void nuke_eq( P_char ch )
{
  P_obj item;

  for( int i = 0; i < MAX_WEAR; i++ )
  {
    if (ch->equipment[i])
    {
      item = unequip_char(ch, i);
      // This must be FALSE to prevent nuking arti files.
      extract_obj( item, FALSE );
    }
  }
  while( ch->carrying )
  {
    item = ch->carrying;
    obj_from_char( item, FALSE );
    // This must be FALSE to prevent nuking arti files.
    extract_obj( item, FALSE );
  }
}

// Feeds artifact to min_minutes or none if already over min_minutes.
void artifact_feed_to_min( P_obj arti, int min_minutes )
{
  P_char ch;
  int feed_time, feed_seconds;

  // Handle bad input..
  if( !arti || min_minutes < 1 )
  {
    debug( "artifact_feed_to_min: Bad arti or timer: %s #%d to feed %d minutes.",
      arti ? arti->short_description : "NULL", arti ? GET_OBJ_VNUM(arti) : -1, min_minutes );
    statuslog( 56, "artifact_feed_to_min: Bad arti or timer: %s #%d to feed %d minutes.",
      arti ? arti->short_description : "NULL", arti ? GET_OBJ_VNUM(arti) : -1, min_minutes );
    return;
  }
  if( !IS_ARTIFACT( arti ) )
  {
    debug( "artifact_feed_to_min: Non-artifact: %s #%d to feed %d minutes.",
      arti->short_description, GET_OBJ_VNUM(arti), min_minutes );
    statuslog( 56, "artifact_feed_to_min: Non-artifact: %s #%d to feed %d minutes.",
      arti->short_description, GET_OBJ_VNUM(arti), min_minutes );
    return;
  }

  // If min_minutes is more than max feed time, then cap it.
  if( min_minutes > ARTIFACT_BLOOD_DAYS * MINS_PER_REAL_DAY )
  {
    min_minutes = ARTIFACT_BLOOD_DAYS * MINS_PER_REAL_DAY;
  }
  // Set time to timer minutes: current - ARTIFACT_BLOOD_DAY days + min_minutes * 60 == min seconds.
  feed_time = time(NULL) - ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY + 60 * min_minutes;
  ch = OBJ_WORN(arti) ? arti->loc.wearing : NULL;

  // If object should feed, seconds to feed.  Otherwise 0.
  feed_seconds = ( arti->timer[3] < feed_time ) ? feed_time - arti->timer[3] : 0;
  if( feed_seconds )
  {
    arti->timer[3] = feed_time;
  }

  statuslog(56, "Artifact: %s [%d] on %s fed [&+G%ld&+Lh &+G%ld&+Lm &+G%ld&+Ls&n]",
    arti->short_description, GET_OBJ_VNUM(arti), ch ? J_NAME(ch) : "Nobody",
    feed_seconds / 3600, (feed_seconds / 60) % 60, feed_seconds % 60 );
  if( !ch )
  {
    return;
  }
  if( feed_seconds > ( 12 * 3600 ) )
  {
    send_to_char("&+RYou feel a deep sense of satisfaction from somewhere...\r\n", ch);
  }
  else
  {
    send_to_char("&+RYou feel a light sense of satisfaction from somewhere...\r\n", ch);
  }
}

// This function is designed to be called just before reboot/shutdown.  It looks
//   for any dropped artifacts, and creates an arti file listing them as on ground.
void dropped_arti_hunt()
{
  int   vnum;
  char  fname[256];
  FILE *f;
  P_obj obj;
  const int timelimit = time(NULL) - 2 * SECS_PER_REAL_DAY;

  for( obj = object_list; obj; obj = obj->next )
  {
    if( IS_ARTIFACT(obj) || CAN_WEAR(obj, ITEM_WEAR_IOUN) )
    {
      // If arti is on ground with less than 2 days on it.. then it's been dropped.
      // is_tracked: -1 means untracked arti, 0 means not an arti, and > 0 means tracked.
      // Note: is_tracked won't return > 0 if a God drops arti.. must be owned/dropped by a mort.
      if( OBJ_ROOM(obj) && obj->loc.room && obj->timer[3] <= timelimit )
      {
        vnum = obj_index[obj->R_num].virtual_number;
        sprintf(fname, ARTIFACT_DIR "%d", vnum);
        f = fopen(fname, "wt");

        if (!f)
        {
          statuslog(56, "dropped_arti_hunt: could not open arti file '%s' for writing", fname);
          logit(LOG_DEBUG,  "dropped_arti_hunt: could not open arti file '%s' for writing", fname);
          continue;
        }
        // Put Room's name, rooms vnum, time -1(!), obj timer.
        fprintf(f, "%s~\n %d %lu -1 %lu", world[obj->loc.room].name, world[obj->loc.room].number, time(NULL), obj->timer[3]);
        fclose(f);
      }
    }
  }
}

// This function runs through the owned artifact list and hunts for people
//   with two or more artifacts.  Then it makes the artifacts fight some and
//   reduces the timers on both.
// This must include artifacts on pfiles not in game.
void event_artifact_wars( P_char ch, P_char vict, P_obj obj, void * arg )
{
  DIR           *dir;
  FILE          *f;
  struct dirent *dire;
  int            vnum, i;
  char           name[256];
  int            t_id, t_tu;
  long unsigned  t_last_time, t_blood;
  P_char         owner;
  P_obj          item;

  debug( "event_artifact_wars: beginning..." );

  // Open the arti directory!
  dir = opendir(ARTIFACT_DIR);
  if (!dir)
  {
    statuslog( 56, "event_artifact_wars: could not open arti dir (%s)\r\n", ARTIFACT_DIR );
    wizlog( 56, "event_artifact_wars: could not open arti dir (%s)\r\n", ARTIFACT_DIR );
    return;
  }
  // Loop through arti files..
  while( dire = readdir(dir) )
  {
    vnum = atoi(dire->d_name);
    if( !vnum )
    {
      continue;
    }

    sprintf(name, ARTIFACT_DIR "%d", vnum);
    f = fopen(name, "rt");

    if( !f )
    {
      statuslog( 56, "event_artifact_wars: could not open arti file (%s)\r\n", name );
      wizlog( 56, "event_artifact_wars: could not open arti file (%s)\r\n", name );
      continue;
    }

    // Read arti file (skip artis on ground).
    if( fscanf(f, "%s %d %lu %d %lu\n", name, &t_id, &t_last_time, &t_tu, &t_blood) < 5 )
    {
      fclose(f);
      continue;
    }
    fclose(f);

    // If owner isn't in game.
    if( !(owner = get_char( name )) )
    {
      // Load pfile
      owner = (P_char) mm_get(dead_mob_pool);
      clear_char(owner);
      owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
      owner->desc = NULL;
      if( restoreCharOnly( owner, name ) < 0 )
      {
        statuslog( 56, "event_artifact_wars: could not restoreCharOnly '%s'", name );
        wizlog( 56, "event_artifact_wars: could not restoreCharOnly '%s'", name );
        extract_char( owner );
        continue;
      }
      restoreItemsOnly( owner, 100 );
      owner->next = character_list;
      character_list = owner;
      setCharPhysTypeInfo( owner );

      // Find arti
      for( i = 0; i < MAX_WEAR; i++ )
      {
        if( !owner->equipment[i] )
          continue;
        if( obj_index[owner->equipment[i]->R_num].virtual_number == vnum )
          break;
      }
      // If not wearing it..
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
        statuslog( 56, "event_artifact_wars: arti %d is not on %s's pfile.", vnum, name );
        wizlog( 56, "event_artifact_wars: arti %d is not on %s's pfile.", vnum, name );
        sprintf(name, ARTIFACT_DIR "%d", vnum);
        unlink(name);
        // Remove eq from char and extract.
        nuke_eq( owner );
        extract_char( owner );
      }
      else
      {
        artifact_fight( owner, obj );
        writeCharacter( owner, RENT_FIGHTARTI, owner->in_room );
        // Free memory
        extract_char( owner );
      }
    }
    else
    {
      // Find arti on PC in game..
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
        statuslog( 56, "event_artifact_wars: arti %d is not on %s!", vnum, name );
        wizlog( 56, "event_artifact_wars: arti %d is not on %s!", vnum, name );
        sprintf(name, ARTIFACT_DIR "%d", vnum);
        unlink(name);
      }
      else
      {
        artifact_fight( owner, obj );
        writeCharacter( owner, 1, owner->in_room );
      }
    }
  }
  closedir(dir);

  debug( "event_artifact_wars: ended." );
  // 1800 = 60sec * 30min => Repeat every half hour...
  add_event( event_artifact_wars, 1800 * WAIT_SEC, ch, vict, obj, 0, arg, 0 );
}

void artifact_fight( P_char owner, P_obj arti )
{
  int numartis, i;
  int main, unique, ioun;
  P_obj obj;

  main = unique = ioun = 0;
  // Count up the equipped artis.
  for( i = numartis = 0;i < MAX_WEAR;i++ )
  {
    if( owner->equipment[i] && (IS_ARTIFACT(owner->equipment[i]) || isname("powerunique", owner->equipment[i]->name)) )
    {
      if( IS_IOUN(owner->equipment[i]) )
      {
        ioun++;
      }
      else if( IS_UNIQUE(owner->equipment[i]) )
      {
        unique++;
      }
      else
      {
        main++;
      }
    }
  }
  // Yes, we count objects in inventory too!
  obj = owner->carrying;
  while( obj )
  {
    if( IS_ARTIFACT(obj) || isname("powerunique", obj->name) )
    {
      if( IS_IOUN(obj) )
      {
        ioun++;
      }
      else if( IS_UNIQUE(obj) )
      {
        unique++;
      }
      else
      {
        main++;
      }
    }
    obj = obj->next_content;
  }
  numartis = (main > 1) ? main - 1 : 0;
  numartis += (unique > 1) ? unique - 1 : 0;
  numartis += (ioun > 1) ? ioun - 1 : 0;
  if( numartis > 0 )
  {
    // 1: 2, 2: 6, 3: 16, 4: 30, 5: 48, 6: 70, 7: 96
    // So, 10 artis -> 96 mins loss every 1/2 hour -> ticker 4.2x as fast, or about 2 days from full to 0.
    if( numartis == 1 )
    {
      arti->timer[3] -= 60 * 2;
    }
    else
    {
      // 2x^2 - 2 function (in minutes).
      arti->timer[3] -= 60 * (2 * numartis * numartis - 2);
    }
    debug( "artifact_fight: '%s' (%d) is upset (%d) with %s.",
      arti->short_description, GET_OBJ_VNUM(arti), numartis, J_NAME(owner) );
    act("&+L$p &+Lseems very upset with you.&n", FALSE, owner, arti, 0, TO_CHAR);
  }
}
