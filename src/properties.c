/****************************************************************************
 *  *
 *  *  File: properties.c                                           Part of Duris
 *  *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  *  Created by: Tharkun   Date: 2003-03-02
 *  * ***************************************************************************
 *  */


#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <fnmatch.h>
#include <string.h>
#include "structs.h"
#include "comm.h"
#include "prototypes.h"
#include "sql.h"
#include "utils.h"

#define PROPERTIES_FILE "lib/duris.properties"
#define MAX_PROPERTIES 3000
#ifndef FNM_CASEFOLD
#   define FNM_CASEFOLD 1<<4
#endif
int      properties_count;
extern void update_damage_data();
extern void update_spellpulse_data();
extern void update_racial_shrug_data();
extern void update_racial_exp_mods();
extern void update_racial_exp_mod_victims();
extern void update_exp_mods();
extern void update_stat_data();
extern void update_dam_factors();
extern void update_racial_dam_factors();
extern void update_saving_throws();
extern void update_breath_weapon_properties();
extern void update_regen_properties();
extern void update_misfire_properties();
extern float hp_mob_con_factor;
extern float hp_mob_npc_pc_ratio;
extern int damroll_cap;
extern int hitroll_cap;
extern int errand_notch;

void     update_exp_table();

struct property
{
  char    *key;
  float    value;
  float    old_value;
} duris_properties[MAX_PROPERTIES];

int key_property_comp(const void *key, const void *property)
{
  return strcmp((char *) key, ((struct property *) property)->key);
}

int property_comp(const void *property1, const void *property2)
{
  return strcmp(((struct property *) property1)->key,
                ((struct property *) property2)->key);
}

float get_property(const char *key, double default_value, bool fussy)
{
  struct property *result;

  result =
    (struct property *) bsearch(key, duris_properties, properties_count,
                                sizeof(struct property), key_property_comp);
  if (result)
    return result->value;
  else
  {
    if (0 && fussy)
      debug("requested property %s, but it was not found", key);
    return (float) default_value;
  }
}

float get_property(const char *key, double default_value)
{
  return get_property(key, default_value, true);
}

int get_property(const char *key, int default_value)
{
  return get_property(key, default_value, true);
}

int get_property(const char *key, int default_value, bool fuss)
{
  float    float_prop = get_property(key, (float) default_value, fuss);
  
  if (float_prop != ((float)((int)float_prop)))
  {
    char buf[500];
    sprintf(buf, "(int)get_property() called for \"%s\" which has a float value of %f.", key, float_prop);
    wizlog(58, buf);
  }

  return (int) float_prop;
}

void apply_properties()
{
  update_stat_data();
  update_damage_data();
  update_spellpulse_data();
  update_racial_shrug_data();
  update_racial_exp_mods();
  update_racial_exp_mod_victims();
  update_exp_mods();
  update_exp_table();
  update_dam_factors();
  update_racial_dam_factors();
  update_saving_throws();
  update_breath_weapon_properties();
  update_regen_properties();
  hp_mob_con_factor = get_property("hitpoints.mob.conFactor", 0.4);
  hp_mob_npc_pc_ratio = get_property("hitpoints.mob.NpcPcRatio", 2.0);
  damroll_cap = get_property("damage.damrollCap", 64);
  hitroll_cap = get_property("damage.hitrollCap", 75);
  errand_notch = get_property("epic.errandStep", 500);
  update_misfire_properties();
}

int parse_property(struct property *property, char *buf)
{
  char     key[128];
  float    value;
  char    *separator;

  separator = strchr(buf, '=');
  if (!separator)
    return 0;
  *separator = ' ';
  if (*buf == '[' || *buf == '#' || sscanf(buf, "%s%f", key, &value) != 2)
  {
    *separator = '=';
    return 0;
  }
  *separator = '=';
  property->key = (char *) str_dup(key);
  property->value = value;
  property->old_value = value;
  return 1;
}

int load_properties(struct property *properties)
{
  FILE    *f;
  char     buf[1024];
  int      count;

  count = 0;
  f = fopen(PROPERTIES_FILE, "r");
  if (f == NULL)
  {
    fprintf(stderr, "Cannot open properties file: %s.\n", PROPERTIES_FILE);
    return 0;
  }

  while (fgets(buf, 1024, f) != NULL)
  {
    if (parse_property(&properties[count], buf))
    {
      count++;
      if (count == MAX_PROPERTIES)
      {
        fprintf(stderr, "Too many properties in file %s, increase MAX_PROPERTIES value!\n", PROPERTIES_FILE);
        exit(1);
      }
    }
  }

  fclose(f);
  return count;
}

void save_properties(P_char ch)
{
  FILE    *f_old, *f_new;
  struct property property, *result;
  char     buf[4096];
  char     changes[4096];
  float    old_value, new_value;

  f_old = fopen(PROPERTIES_FILE, "r");
  f_new = fopen(PROPERTIES_FILE ".new", "w");
  changes[0] = '\0';

  while (fgets(buf, 1024, f_old) != NULL)
  {
    if (parse_property(&property, buf))
    {
      result =
        (struct property *) bsearch(property.key, duris_properties,
                                    properties_count, sizeof(struct property),
                                    key_property_comp);
      if (result)
      {
        if (property.value != result->value)
        {
          sprintf(changes + strlen(changes), "%s from %.3f to %.3f, ",
                  property.key, property.value, result->value);
          result->old_value = result->value;
        }
        sprintf(buf, "%s=%.3f\n", property.key, result->value);
      }
      FREE(property.key);
    }
    fputs(buf, f_new);
  }

  fclose(f_new);
  fclose(f_old);
  rename(PROPERTIES_FILE ".new", PROPERTIES_FILE);

  sprintf(buf, "%s saved the properties: %s", GET_NAME(ch), changes);
  wizlog(57, buf);
  logit(LOG_WIZ, buf);
  sql_log(ch, WIZLOG, "Saved properties");

  //sprintf(buf, "svn commit -m \'%s: %s\' " PROPERTIES_FILE, name, changes);
  //system(buf);
}

void initialize_properties()
{
  properties_count = load_properties(duris_properties);
  qsort(duris_properties, properties_count, sizeof(struct property), property_comp);
  apply_properties();
}

void do_properties(P_char ch, char *args, int cmd)
{
  char    *command;
  char    *pattern;
  char    *val_string;
  char     buf[256];
  float    new_value;
  int      show_help = 0;
  int      i;

  command = (char *) strtok(args, " ");
  pattern = (char *) strtok(NULL, " ");
  val_string = (char *) strtok(NULL, " ");

  if (command != NULL)
  {
    if (!strcmp(command, "show") && pattern != NULL)
    {
      send_to_char("The following properties match the pattern:\r\n\r\n", ch);
      for (i = 0; i < properties_count; i++)
      {
        if (fnmatch(pattern, duris_properties[i].key, FNM_CASEFOLD) == 0)
        {
          if (duris_properties[i].value != duris_properties[i].old_value)
            sprintf(buf, "*%s: %.3f(%.3f)\r\n", duris_properties[i].key,
                    duris_properties[i].value, duris_properties[i].old_value);
          else
            sprintf(buf, "%s: %.3f\r\n", duris_properties[i].key,
                    duris_properties[i].value);
          send_to_char(buf, ch);
        }
      }
    }
    else if (!strcmp(command, "diff"))
    {
      for (i = 0; i < properties_count; i++)
        if (duris_properties[i].value != duris_properties[i].old_value)
        {
          sprintf(buf, "%s=%.3f(%.3f)\r\n", duris_properties[i].key,
                  duris_properties[i].value, duris_properties[i].old_value);
          send_to_char(buf, ch);
        }
    }
    else if (!strcmp(command, "set") && (GET_LEVEL(ch) >= FORGER))
    {
      if (val_string && sscanf(val_string, "%f", &new_value) == 1)
      {
        bool success = FALSE;
        for (i = 0; i < properties_count; i++)
        {
          if (fnmatch(pattern, duris_properties[i].key, FNM_CASEFOLD) == 0)
          {
            duris_properties[i].value = new_value;
            sprintf(buf, "%s set %s to %.3f", ch->player.name,
                    duris_properties[i].key, new_value);
            wizlog(57, buf);
            logit(LOG_WIZ, buf);
            sql_log(ch, WIZLOG, "Set %s to %.3f", duris_properties[i].key, new_value);
            success = TRUE;
          }
        }
        if( !success )
        {
          sprintf(buf, "property %s not found", pattern);
          wizlog(57, buf);
        }
        else
        {
          apply_properties();
        }
      }
      else
        send_to_char("Could not parse the provided value.\r\n", ch);
    }
    else if (!strcmp(command, "reload") && (GET_LEVEL(ch) >= FORGER))
    {
      initialize_properties();
    }
    else if (!strcmp(command, "save") && (GET_LEVEL(ch) >= FORGER))
    {
      save_properties(ch);
    }
    else if (!strcmp(command, "revert") && (GET_LEVEL(ch) >= FORGER))
    {
      for (i = 0; i < properties_count; i++)
      {
        if (duris_properties[i].value != duris_properties[i].old_value)
        {
          sprintf(buf, "%s reverted to %.3f from %.3f\r\n",
                  duris_properties[i].key, duris_properties[i].old_value,
                  duris_properties[i].value);
          send_to_char(buf, ch);
          duris_properties[i].value = duris_properties[i].old_value;
        }
      }
      apply_properties();
      sprintf(buf, "%s reverted property changes.", ch->player.name);
      wizlog(57, buf);
      logit(LOG_WIZ, buf);
      sql_log(ch, WIZLOG, "Reverted property changes");
    }
    else
      show_help = 1;
  }
  else
    show_help = 1;


  if (show_help)
  {
    send_to_char("Use:\r\n"
                 "  &+Wproperties show <pattern>\r\n"
                 "  &+Wproperties set <pattern> <value>\r\n"
                 "  &+Wproperties save\r\n"
                 "  &+Wproperties revert\r\n"
                 "  &+Wproperties reload\r\n" "  &+Wproperties diff\r\n", ch);
    return;
  }
}
