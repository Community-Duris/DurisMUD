/****************************************************************************
 *
 *  File: randomeq.c                                           Part of Duris
 *  Usage: randomboject.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark                   Date: 2002-04-18
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "guildhall.h"
#include "interp.h"
#include "mm.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "specs.barovia.h"
#include "specs.eth2.h"
#include "specs.jubilex.h"
#include "specs.keleks.h"
#include "specs.ravenloft.h"
#include "specs.snogres.h"
#include "specs.winterhaven.h"
#include "specs.zion.h"
#include "spells.h"
#include "structs.h"
#include "utility.h"
#include "utils.h"
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "weather.h"
#include "sound.h"
#include "objmisc.h"
#include "tradeskill.h"
#include "map.h"
#include "specs.prototypes.h"
#include "assocs.h"
#include "vnum.obj.h"

#define SMITH_MAX_ITEMS   20


/*
 * external variables
 */
extern Skill skills[];
extern struct zone_data *zone_table;
extern const char *material_names[];
extern P_char character_list;
extern P_desc descriptor_list;
extern P_event event_type_list[];
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern char debug_mode;
extern const char *race_types[];
extern const char *rude_ass[];

//extern const int material_absorbtion[][];
extern const struct stat_data stat_factor[];
extern float fake_sqrt_table[];
extern int pulse;
extern int arena_hometown_location[];
extern struct arena_data arena;
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct message_list fight_messages[];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern void material_restrictions(P_obj);
extern int find_map_place();
extern struct forge_item forge_item_list[];

int cards_object(P_obj obj, P_char ch, int cmd, char *argument);
int magic_deck(P_obj obj, P_char ch, int cmd, char *argument);
int unmulti_altar(P_obj obj, P_char ch, int cmd, char *arg);
int blackjack_table(P_obj obj, P_char ch, int cmd, char *argument);

void     set_keywords(P_obj t_obj, const char *newKeys);
void     set_short_description(P_obj t_obj, const char *newShort);
void     set_long_description(P_obj t_obj, const char *newDescription);
int      get_ival_from_proc( obj_proc_type );
int      get_mincircle( int spell );
int      calc_ore_cost( P_char ch, P_obj ore );

struct mine_range_data {
  char *name;
  char *abbrev;
  int start;
  int end;
  int type;
  int mine_duration;
  bool reload;
} mine_data[] = {
  // Note: Don't comment these out, just set max number to 0 in duris.properties.
  //  Also, to add a type, you need to add to #defines MINES_... in tradeskill.h and
  //    catch the type in mines_properties() in this file, and add to duris.properties.
  // Zone display name, command argument matching, start range, end range, mine type, duration, reloading mines?
  // Note: mine_duration is in sets of 4 sec: 11 -> 9 * 4 = 36 sec, since event_mine_check occurs every 4 sec.
  {"Surface Map",     "map",       500000, 659999,    VOBJ_MINE,  9, TRUE },
  {"Underdark",       "ud",        700000, 859999,    VOBJ_MINE,  9, TRUE },
  {"Tharnadia Rift",  "tharnrift", 110000, 119999,    VOBJ_MINE,  9, FALSE},
  {"Surface Map - G", "mapg",      500000, 659999, VOBJ_GEMMINE, 15, TRUE },
  {"Underdark - G",   "udg",       700000, 859999, VOBJ_GEMMINE, 15, TRUE },
  {0}
};

struct mines_event_data {
  int map;
};

int forge_prices[] = {2000, 5000, 15000, 30000, 50000};

struct smith_data {
  int vnum;
  short int items[SMITH_MAX_ITEMS+1];
} smith_array[] = {
  {76010, { 131, 132,  53,  54,  55,  56,  -1}},
  {82518, {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  -1}},
  {22269, {  92,  93,  94,  95,  96,  -1}},
  {76691, {  88,  89,  91, 103,  -1}},
  {20240, {  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21, 22,   -1}},
  {40712, {  41,  42,  43,  44,  45,  46,  47,  -1}},
  {99716, {  62,  63,  64,  65,  66,  67,  68,  69,  76,  77,  78,  79,  80,  -1}},
  {9412,  {  60,  61,  49,  50,  70,  71,  72,  73,  73,  75,  -1}},
  {96058, { 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 119,  -1}},
  {6002,  {   6,   7,   8,   9,  10,  23,  24,  25,  26,  30,  37,  38,  39,  40, 113,  -1}},
  {66410, { 127, 128, 129, 130,  88,  89,  90,  91,  -1}},
  {1420,  {  70,  57,  58,  59,  48,  36,  38,  37,  39,  40,  -1}}, // kobolds
  {96916, {  57,  58,  59,  27,  29,  97, 101, 102, 107, 109, 112,  -1}}, //troll caves
  {75628, {  12,  13,  14,  66,  67,  67,  64,  65,  77,  76,  -1}}, //citadel
  {76502, {  60,  61,  34,  31,  32,  33,  74,  75,  88,  89,  92,  -1}},
  {28917, {  46,  47,  49,  45,  35,  78,  77,  86,  87,  85,  -1}}, //torg
  {37755, {  30,  26,  22,  23,  24,  25,  21,  11,  12,  13,  14,  15,  16,  -1}}, //nax
  {0, {-1}}
};

int mines_properties(int map)
{
  switch (map)
  {
  case MINES_MAP_SURFACE:
    return (int)get_property("mines.maxSurfaceMap", 50);
    break;
  case MINES_MAP_UD:
    return (int)get_property("mines.maxUD", 50);
    break;
  case MINES_MAP_THARNRIFT:
    return (int)get_property("mines.maxTharnRift", 50);
    break;
  case MINES_GEM_SURFACE:
    return (int)get_property("mines.maxGemSurface", 6);
    break;
  case MINES_GEM_UD:
    return (int)get_property("mines.maxGemUD", 2);
    break;
  default:
    logit(LOG_DEBUG, "mines_properties(): passing invalid map to function, using default 50 mines");
    return 50;
    break;
  }
}

P_obj get_hammer(P_char ch)
{
  P_obj    t_obj, next_obj;

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if (obj_index[t_obj->R_num].virtual_number == HAMMER_VNUM)
    {
      return t_obj;
    }
  }

  return NULL;
}

// #define IS_MINING_PICK(obj) ( OBJ_VNUM(obj) == 253 || \
                              // OBJ_VNUM(obj) == 338 || \
                              // OBJ_VNUM(obj) == 10640 || \
                              // OBJ_VNUM(obj) == 95531 || \
                              // OBJ_VNUM(obj) == 49018 )

#define IS_MINING_PICK(obj) (isname("pick", obj->name) && obj->type == ITEM_WEAPON)

P_obj get_pick(P_char ch)
{
  if( !ch )
    return NULL;

  if( ch->equipment[WIELD] && IS_MINING_PICK( ch->equipment[WIELD] ) )
    return ch->equipment[WIELD];

  if( ch->equipment[WIELD2] && IS_MINING_PICK( ch->equipment[WIELD2] ) )
    return ch->equipment[WIELD2];

  return NULL;
}

int parchment_forge(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char temp_ch;
  if(!ch)
    return FALSE;

  if(cmd != CMD_RECITE)
    return FALSE;

  temp_ch = obj->loc.wearing;

  if(!temp_ch)
    return FALSE;;

  if(ch != temp_ch)
    return FALSE;


  for (int tmp = 1; tmp < MAX_FORGE_ITEMS; tmp++)
  {
    if(ch->only.pc->learned_forged_list[tmp] == obj->value[0])
    {
      send_to_char("You already know how to forge this item.\r\n", ch);
      return TRUE;
    }

    if( forge_item_list[ch->only.pc->learned_forged_list[obj->value[0]]].skill_min > GET_CHAR_SKILL(ch, SKILL_FORGE))
    {
      send_to_char("This parchment contains smithing too advanced for you.\r\n", ch);
      return TRUE;

    }
    if(ch->only.pc->learned_forged_list[tmp] == 0)
    {
      ch->only.pc->learned_forged_list[tmp] = obj->value[0];
      send_to_char("You learn a new item to forge!\r\n", ch);
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      return TRUE;
    }
  }
}

void create_parchment(P_char ch)
{
  P_obj obj = read_object(PARCHMENT_VNUM, VIRTUAL);
  if(!obj)
    return;

  int howmany = 0;
  for (int tmp = 1; tmp < MAX_FORGE_ITEMS; tmp++)
  {
    if(forge_item_list[tmp].id == 0)
      break;

    howmany++;
  }
  obj->value[0] = number(1,howmany); 

  //If it's not rare dont load it!
  if( forge_item_list[ch->only.pc->learned_forged_list[obj->value[0]]].how_rare < number(1,100))
    return; 

  obj_to_room(obj, ch->in_room);
  act("$n's digging turns up $p&n!", TRUE, ch, obj, 0, TO_ROOM);
  act("Your digging turns up $p&n!", TRUE, ch, obj, 0, TO_CHAR);
}

P_obj forge_create(int choice, P_char ch, int material)
{
  char     dummy[MAX_INPUT_LENGTH];
  char     short_desc[MAX_STRING_LENGTH];
  char     long_desc[MAX_INPUT_LENGTH];
  char     keywords[MAX_INPUT_LENGTH];
  P_obj obj;

  if( !(obj = read_object(1255, VIRTUAL)) )
  {
    wizlog(56, "unabled to load object 1255");
    return 0;
  }

  sprintf(keywords, "%s", forge_item_list[choice].keywords);
  sprintf(dummy, "%s", forge_item_list[choice].short_desc);
  sprintf(short_desc, dummy, GET_NAME(ch));
  sprintf(dummy, "%s", forge_item_list[choice].long_desc);
  sprintf(long_desc, dummy, GET_NAME(ch));

  set_short_description(obj, short_desc);
  set_long_description(obj, long_desc);
  set_keywords(obj, keywords);

  obj->material = material;
  obj->affected[0].location = forge_item_list[choice].loc0;
  obj->affected[0].modifier = number(forge_item_list[choice].min0, forge_item_list[choice].max0);
  obj->affected[1].location = forge_item_list[choice].loc1;
  obj->affected[1].modifier = number(forge_item_list[choice].min1, forge_item_list[choice].max1);

  SET_BIT(obj->wear_flags, ITEM_TAKE);
  SET_BIT(obj->wear_flags, forge_item_list[choice].wear_flags);

  SET_BIT(obj->bitvector, forge_item_list[choice].aff1);
  SET_BIT(obj->bitvector2, forge_item_list[choice].aff2);
  SET_BIT(obj->bitvector3, forge_item_list[choice].aff3);
  SET_BIT(obj->bitvector4, forge_item_list[choice].aff4);

  obj->anti_flags |= forge_item_list[choice].classes;

  if(forge_item_list[choice].allow_anti)
    obj->extra_flags = ITEM_ALLOWED_CLASSES;;
  if (isname("quiver", obj->name))
  {
    obj->value[0] = number(20, 80);
    obj->value[1] = number(0, 1);
    obj->value[2] = 1;
    obj->value[3] = 0;
    obj->type = ITEM_QUIVER;
  }
  else
    obj->type == ITEM_ARMOR;
  return obj;
}

void forge_describe(int choice, P_char ch)
{
  char buffer[1024];
  int i;

  sprintf(buffer, "To create %s you need:\n", forge_item_list[choice].short_desc);  
  for (i = 0; i < 5 && forge_item_list[choice].ore_needed[i]; i++)
    sprintf(buffer + strlen(buffer), "%s\n", 
        obj_index[real_object(forge_item_list[choice].ore_needed[i])].desc2 ); 

  send_to_char(buffer, ch);
  sprintf(buffer, "It will cost you %s to forge this.\n", coin_stringv(forge_prices[i-1]));
  send_to_char(buffer, ch);
}

P_obj check_foundry(P_char ch)
{
  P_obj t_obj;

  for (t_obj = world[ch->in_room].contents; t_obj; t_obj = t_obj->next_content)
  {
    if (obj_index[t_obj->R_num].virtual_number == 361)
    {
      return t_obj;
    }
  }

  return NULL;
}

void do_forge(P_char ch, char *argument, int cmd)
{
  int      skillLevel, objVnum, recipeNumber, commandType, iVal, invVnum;
  int      numHighQuality, numLowQuality, lowQualityMaterialVnum, highQualityMaterialVnum;
  int      invLowMats, invHighMats, invEssences;
  char     buf[256];
  char     recipe[256];
  char     Gbuf1[MAX_STRING_LENGTH];
  char     first[MAX_INPUT_LENGTH];
  char     second[MAX_INPUT_LENGTH];
  char     keywords[MAX_STRING_LENGTH];
  char     short_desc[MAX_STRING_LENGTH];
  char    *rest;
  bool     hasAffect, hasFlux;
  FILE    *recipeBookFile;
  P_obj    obj, lowQualityMaterial, highQualityMaterial, inventory, invNextObj;

  if( !(skillLevel = GET_CHAR_SKILL(ch, SKILL_FORGE)) )
  {
    act("You do not know how to &+Lforge&n.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }


  // Directory to ch's recipe book.
  strcpy(buf, GET_NAME(ch));
  // Only need to lower the first initial (the rest are always lowercase).
  *buf = LOWER(*buf);
  //buf[0] snags first character of name
  sprintf(Gbuf1, "Players/Tradeskills/%c/%s.crafting", buf[0], buf);
  // If we can't open the recipe book file.
  if( !(recipeBookFile = fopen(Gbuf1, "r")) )
  {
    send_to_char("You dont know any recipes yet.\r\n", ch);
    return;
  }
  rest = one_argument(argument, first);
  one_argument(rest, second);
  objVnum = atoi(second);

  // If we don't have a first argument.
  if( !*first )
  {
    commandType = 0;
  }
  else if( is_abbrev(first, "info") )
  {
    commandType = 1;
  }
  else if( is_abbrev(first, "stat") )
  {
    commandType = 2;
  }
  else if( is_abbrev(first, "make") )
  {
    commandType = 3;
  }
  // The first argument is invalid.
  else
  {
    commandType = 0;
  }

  if( commandType == 0 )
  {
    send_to_char("&+wForge Syntax:\n&n", ch);
    send_to_char("&+w(forge info <number> - list required materials to forge the item.)\n&n", ch);
    send_to_char("&+w(forge stat <number> - display properties of the item.)\n&n", ch);
    send_to_char("&+w(forge make <number> - create the item.)\n&n", ch);
    send_to_char("&+yYou know the following recipes:\n&n", ch);
    send_to_char("----------------------------------------------------------------------------\n", ch);
    send_to_char("&+BRecipe Number		              &+MItem&n\n\r", ch);

    while( (fscanf(recipeBookFile, "%i", &objVnum)) != EOF )
    {
      if( !(obj = read_object( objVnum, VIRTUAL )) )
      {
        logit( LOG_DEBUG, "'%s' has bad recipe vnum %d.", ch ? J_NAME(ch) : "NULL", objVnum );
      }
      sprintf(recipe, "   &+W%-22d&n%s&n\n", objVnum, obj->short_description);
      page_string(ch->desc, recipe, 1);
      send_to_char("----------------------------------------------------------------------------\n", ch);
      extract_obj(obj);
    }
    fclose(recipeBookFile);
    return;
  }

  // If the second argument doesn't exist or isn't a positive integer.
  if( objVnum <= 0 )
  {
    if( commandType == 1 )
    {
      send_to_char("What &+Wrecipe&n would you like &+yinformation&n about?\n", ch);
    }
    else if( commandType == 2 )
    {
      send_to_char("What &+Wrecipe&n would you like &+ystatistics&n about?\n", ch);
    }
    else
    {
      send_to_char("What &+Witem &nare you attempting to forge?\n", ch);
    }
    fclose(recipeBookFile);
    return;
  }

  // Find objVnum in the recipe book.
  while( (fscanf(recipeBookFile, "%i", &recipeNumber)) != EOF )
  {
    if( recipeNumber == objVnum )
    {
       break;
    }
	}
  fclose(recipeBookFile);

  // If we couldn't find the vnum in the recipe book (commandType is irrelevant).
  if( recipeNumber != objVnum )
  {
    send_to_char("You dont appear to have that &+Wrecipe&n in your list.&n\n", ch);
    return;
  }

  // Attempt to load the object we're inspecting/making.
  if( !(obj = read_object( objVnum, VIRTUAL )) )
  {
    sprintf( Gbuf1, "Your recipe # %d seems to be &+rcorrupted&n. Please tell a &+WGod.\n\r", objVnum );
    logit( LOG_DEBUG, "do_forge: '%s' has bad recipe vnum (%d) - couldn't load object.",
      ch ? J_NAME(ch) : "NULL", objVnum );
    return;
  }


  // 1 -> "info"
  if( commandType == 1 )
  {
    // Display required materials to make obj - formula below:
    iVal = itemvalue(obj);
    numHighQuality = (iVal + 4) / 5;
    numLowQuality = (iVal + 4) - numHighQuality * 5;
    lowQualityMaterialVnum = get_matstart(obj);
    highQualityMaterialVnum = lowQualityMaterialVnum + 4;
    lowQualityMaterial = read_object( lowQualityMaterialVnum, VIRTUAL );
    highQualityMaterial = read_object( highQualityMaterialVnum, VIRTUAL );

    if( lowQualityMaterial == NULL || highQualityMaterial == NULL )
    {
      send_to_char( "You couldn't figure out what materials to use.\n\r", ch );
      if( lowQualityMaterial != NULL )
      {
        extract_obj( lowQualityMaterial );
      }
      if( highQualityMaterial != NULL )
      {
        extract_obj( highQualityMaterial );
      }
      extract_obj( obj );
      return;
    }

    send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
    if( numLowQuality == 0 )
    {
      sprintf( recipe, "To forge this item, you will need %d of %s.\r\n&n",
        numHighQuality, highQualityMaterial->short_description );
    }
    else
    {
      // numHighQuality will always be >= 1 with the code the way it is on 2/11/2015.
      if( numHighQuality == 0 )
      {
        sprintf( recipe, "To forge this item, you will need %d of %s.\r\n&n",
          numLowQuality, lowQualityMaterial->short_description );
      }
      else
      {
        sprintf( recipe, "To forge this item, you will need %d of %s and %d of %s.\r\n&n",
          numHighQuality, highQualityMaterial->short_description,
          numLowQuality, lowQualityMaterial->short_description );
      }
    }
    // If we're going to require multiple essences, need to edit this if statement.
    if( has_affect(obj) )
    {
      strcat( recipe, "You must have &+W1 &nof &+ma &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml &+messence&n due to the &+mmagical &nproperties this item possesses.\r\n" );
    }

    page_string(ch->desc, recipe, 1);
    extract_obj( obj );
    extract_obj( highQualityMaterial );
    extract_obj( lowQualityMaterial );
    return;
  }
  // 2 -> "stat"
  else if( commandType == 2 )
  {
    // Display info on obj:
    send_to_char( "&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch );
    spell_identify(GET_LEVEL(ch), ch, 0, 0, 0, obj);
    extract_obj( obj );
    return;
  }
  // 3 -> "make"
  else if( commandType == 3 )
  {
    // Attempt to make obj:
    iVal = itemvalue(obj);
    if( iVal > 100 || IS_OBJ_STAT2(obj, ITEM2_QUESTITEM) )
    {
      act("You look at the recipe for $p&n, but can't seem to discern how to make it.  &+mHow strange.&N",
        FALSE, ch, obj, 0, TO_CHAR);
      extract_obj(obj);
      return;
    }
    numHighQuality = (iVal + 4) / 5;
    numLowQuality = (iVal + 4) - numHighQuality * 5;
    lowQualityMaterialVnum = get_matstart(obj);
    highQualityMaterialVnum = lowQualityMaterialVnum + 4;
    hasAffect = has_affect( obj );

    /* Foundry code here (not requiring one atm I guess):
    if( !check_foundry(ch) )
    {
      act("&+LYou need to be by your foundry to forge...&n", FALSE, ch, 0, 0, TO_CHAR);
      extract_obj( obj );
      return;
    }
    */

    hasFlux = FALSE;
    invLowMats = invHighMats = invEssences = 0;
    // Count up the materials ch has on hand.
    for( inventory = ch->carrying; inventory; inventory = inventory->next_content )
    {
      invVnum = OBJ_VNUM( inventory );

      if( invVnum == lowQualityMaterialVnum )
      {
        invLowMats++;
      }
      else if( invVnum == highQualityMaterialVnum )
      {
        invHighMats++;
      }
      else if( invVnum == VOBJ_FORGING_ESSENCE )
      {
        invEssences++;
      }
      else if( invVnum == VOBJ_FORGING_FLUX )
      {
        hasFlux = TRUE;
      }
    }

    // Check to make sure ch has enough materials.
    if( hasFlux == FALSE )
    {
      send_to_char("You must have a &+Lblacksmithing &nflux to complete the smithing process!\r\n", ch);
      extract_obj( obj );
      return;
    }
    if( (hasAffect && invEssences < 1) || (invLowMats < numLowQuality) || (invHighMats < numHighQuality) )
    {
      send_to_char("You do not have the required &+ysalvaged &+Ymaterials &nin your inventory.\r\n", ch);
      extract_obj( obj );
      return;
    }

    // Ok, ch has the materials needed to create obj in inventory.. Now take them away, muahahah!
    for( inventory = ch->carrying; inventory; inventory = invNextObj )
    {
      invNextObj = inventory->next_content;
      invVnum = OBJ_VNUM( inventory );

      if( (invVnum == lowQualityMaterialVnum) && (numLowQuality > 0) )
      {
        extract_obj( inventory, TRUE ); // Not an arti, but 'in game.'
        numLowQuality--;
      }
      else if( (invVnum == highQualityMaterialVnum) && (numHighQuality > 0) )
      {
        extract_obj( inventory, TRUE ); // Not an arti, but 'in game.'
        numHighQuality--;
      }
      else if( hasAffect && (invVnum == VOBJ_FORGING_ESSENCE) )
      {
        extract_obj( inventory, TRUE ); // Not an arti, but 'in game.'
        hasAffect = FALSE;
      }
      else if( hasFlux && (invVnum == VOBJ_FORGING_FLUX) )
      {
        extract_obj( inventory, TRUE ); // Not an arti, but 'in game.'
        hasFlux = FALSE;
      }
    }

    wizlog(56, "%s forged '%s' (%d) ival %d.", GET_NAME(ch), obj->short_description, objVnum, iVal );
    notch_skill(ch, SKILL_FORGE, 50);
    SET_BIT(obj->extra2_flags, ITEM2_CRAFTED);
    SET_BIT(obj->extra_flags, ITEM_NOREPAIR);
    REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
    randomizeitem(ch, obj);

    sprintf( keywords, "%s %s tradeskill", obj->name, GET_NAME(ch));
    sprintf( short_desc, "%s &+ymade by&n &+r%s&n", obj->short_description, GET_NAME(ch));
    set_keywords( obj, keywords );
    set_short_description( obj, short_desc );

    obj_to_char( obj, ch );

    act("&+W$n &+Lgently takes their &+ymaterials&+L, their &nflux&+L, and places them into the &+rf&+Ro&+Yr&+Rg&+re&+L.\r\n"
      "&+W$n &+Lremoves the &+yitems &+Lfrom the &+rheat &+Land starts to &nhammer &+Laway at the mixture..\r\n"
      "&+L...after shedding plenty of &+Wsweat&+L, &+W$n &+Lsteps back, admiring their new $p.&N",
      TRUE, ch, obj, 0, TO_ROOM);
    act("You &+Lgently take your &+ymaterials&+L, the &nflux&+L, and place them into the &+rf&+Ro&+Yr&+Rg&+re&+L.\r\n"
      "You &+Lremove the &+yitems &+Lfrom the &+rheat &+Land start to &nhammer &+Laway at the mixture..\r\n"
      "&+L...after shedding plenty of &+Wsweat&+L, you &+Lstep back, admiring your new $p.&N",
      FALSE, ch, obj, 0, TO_CHAR);

    gain_exp(ch, NULL, (iVal * 1000), EXP_BOON);
  }
  else
  {
    send_to_char( "&+RBad command type&n: Please report this to an Immortal.\n\r", ch );
    logit( LOG_DEBUG, "do_forge: bad command arguments '%s' by '%s'.", argument, ch ? J_NAME(ch) : "NULL" );
    extract_obj( obj );
    return;
  }
}

/* OLD FORGE CODE:
  if (GET_CHAR_SKILL(ch, SKILL_FORGE) == 0){
    send_to_char("&+LYou dont know how to forge.\r\n", ch);
    return;
  }

  foundry = check_foundry(ch);

  if (!foundry) 
   { // No furnace/foundry/forge in room
	act("&+LYou need to be by your foundry to forge...&n", FALSE, ch, 0, 0, TO_CHAR);
	return;
   }

  *buf1 = '\0';
  if (!*argument){
    sprintf(buf1 + strlen(buf1), "Syntax: 'forge info #', 'forge create #'\n"); 
    sprintf(buf1 + strlen(buf1), "You know how to forge the following items:\n");
    if(IS_TRUSTED(ch))
      for (int tmp = 1; tmp < MAX_FORGE_ITEMS; tmp++)
      {
        if(forge_item_list[tmp].short_desc == NULL)
          break;
        sprintf(buf1 + strlen(buf1), "%d -  %s\n", tmp, forge_item_list[tmp].short_desc);
      }
    else

      for (int tmp = 1; tmp < MAX_FORGE_ITEMS; tmp++)
      {
        if(ch->only.pc->learned_forged_list[tmp] != 0)
          sprintf(buf1 + strlen(buf1), "%d -  %s\n", tmp, forge_item_list[ch->only.pc->learned_forged_list[tmp]].short_desc );
      }
    strcat(buf1, "\n");
    page_string(ch->desc, buf1, 1);
    return;
  }

  sprintf(buf1, "");
  half_chop(argument, first, rest);
  half_chop(rest, second, rest);
  choice = (ush_int) atoi(second);

  if (choice < 1 || choice >= MAX_FORGE_ITEMS) {
    send_to_char("This is not a valid number.\n", ch);
    return;
  }

  if (is_abbrev(first, "info"))
  { 

    if(!choice){
      send_to_char("Missing forge list number 'forge info 1'.\r\n", ch);   
      return;      
    }
    if(IS_TRUSTED(ch))
      choice = choice;
    else
      choice  = ch->only.pc->learned_forged_list[choice];

    if(!choice){
      send_to_char("You do not know how to make that. Type 'forge' to get a list of know items..\r\n", ch);
      return;
    }

    forge_describe(choice, ch);
    return;
  }

  if (is_abbrev(first, "create"))
  {
    if(!choice){
      send_to_char("Missing forge list number 'forge create 1'.\r\n", ch);
      return;
    }
    if(IS_TRUSTED(ch))
      choice = choice;
    else

      choice  = ch->only.pc->learned_forged_list[choice];
    if(!choice){
      send_to_char("You do not know how to make that. Type 'forge' to get a list of know items..\r\n", ch);
      return;
    }

    hammer = get_hammer(ch);
    if(!hammer)
    {
      act("You need to have a forge hammer in your inventory.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }

    if(forge_item_list[choice].short_desc == NULL)
    {
      send_to_char("Yes you are a god, but why try to create something that's not even possible?!?!.\r\n", ch);
      return;
    }

    int material = 0;     
    int NEEDED = 0;
    int FOUND = 0;
    P_obj t_obj, next_obj;
    while(forge_item_list[choice].ore_needed[i] != 0)
    {
      NEEDED++;
      for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
      {
        next_obj = t_obj->next_content;
        if(obj_index[t_obj->R_num].virtual_number == forge_item_list[choice].ore_needed[i]){
          if(!material)
            material = t_obj->material;
          sprintf(buf1, "You start to work on %s\n", t_obj->short_description );
          send_to_char(buf1, ch);
          extract_obj(t_obj);
          FOUND++;
          if(!number(0,2))
            if(GET_CHAR_SKILL(ch, SKILL_FORGE) < number(0, 99))
            {
              send_to_char("You swing wildly, slip, fall, hurt yourself, and destroy the ore!\r\n", ch);
              return;
            }       
          break;
        }
      }
      i++;
    }
    if(FOUND == NEEDED || IS_TRUSTED(ch))
    {
      if(FOUND == 0)
        send_to_char("You are a god! Material type is created from the ore, since you used none it will be wrong..fyi!\n",ch); 
      P_obj obj = forge_create(choice, ch, material);
      if (!obj)
        return;
      obj_to_char(obj, ch);
      sprintf(buf1, "You finishing forging: %s!\n", obj->short_description); 
      send_to_char(buf1, ch);  
      wizlog(56, "%s forged %s" , GET_NAME(ch), buf1);
      notch_skill(ch, SKILL_FORGE, 50);
      return;
    }
    else
    {
      send_to_char("You do not  have the required items. Type forge info # to find out what you need. You wasted some ore.\r\n", ch);
      return;
    }
    return;
  }
*/

bool mine_friendly(int to_room) {
  if (world[to_room].sector_type == SECT_HILLS)
    return true;

  if (world[to_room].dir_option[NORTH] &&
      (world[world[to_room].dir_option[NORTH]->to_room].sector_type == SECT_HILLS ||
       world[world[to_room].dir_option[NORTH]->to_room].sector_type == SECT_MOUNTAIN))
    return true;
  
  if (world[to_room].dir_option[EAST] &&
      (world[world[to_room].dir_option[EAST]->to_room].sector_type == SECT_HILLS ||
       world[world[to_room].dir_option[EAST]->to_room].sector_type == SECT_MOUNTAIN))
    return true;
  
  if (world[to_room].dir_option[SOUTH] &&
      (world[world[to_room].dir_option[SOUTH]->to_room].sector_type == SECT_HILLS ||
       world[world[to_room].dir_option[SOUTH]->to_room].sector_type == SECT_MOUNTAIN))
    return true;

  if (world[to_room].dir_option[WEST] &&
      (world[world[to_room].dir_option[WEST]->to_room].sector_type == SECT_HILLS ||
       world[world[to_room].dir_option[WEST]->to_room].sector_type == SECT_MOUNTAIN))
    return true;

  return false;
}

int get_mine_content(P_obj mine)
{
  return mine->value[0];
}

int remove_mine_content(P_obj mine)
{
  return (mine->value[0]--);
}

// Takes a mine quality (regular mine, not gems) and returns a vnum for an ore.
//   This is just for raw ore, the char's luck check (increases value) is in event_mine_check.
//   Mine quality ranges from 0 to 3 (I hope someone edits here if it changes).
// Iron: 4/400 large, 15/400 medium, 7/400 small -> 26/400 total -> 6.5% chance.
// Adamantium: 1/400 large, 8/400 medium, 29/400 small -> 38/400 total -> 9.5% chance.
// Other 6 metals: 4/400 large, 20/400 medium, 32/400 small -> 56/400 total -> 14% chance.
// Note: This makes adamantium a little more likely than iron, but that's ok; it makes people happy.
//   You're most likely to find a small intermediate (non-iron, non-adamantium) ore, then small adamantium,
//   medium intermediate, medium iron, medium adamantium, small iron, large non-adamantium, lastly large adamantium.
int random_ore( int mine_quality )
{
  // Start with a random percentage.
  int quality = number( 1, 100);
  // metal_type ranges from 0 to 7 (iron, steel, copper, silver, gold, platinum, mithril, adamantium)
  //   For mine_quality 0 -> 1-11 : 0, 12-25: 1, ... 82-95: 6, 96-100: 7
  //   For mine_quality 1 -> 1-08 : 0, 09-22: 1, ... 79-92: 6, 93-100: 7
  //   For mine_quality 2 -> 1-05 : 0, 06-19: 1, ... 76-89: 6, 90-100: 7
  //   For mine_quality 3 -> 1-02 : 0, 03-16: 1, ... 73-86: 6, 87-100: 7
  // It's important that there's a 14% chance to get adamantium with mine_quality 3, so we can get a large
  //   adamantium ore per the ore_size = ... % 14 must be 13 for a large ore.
  int ore_type = (quality+3*mine_quality+2) / 14;
  // ore_quality ranges from 0 to 2 (small, medium, large)
  //   For each metal type, we have a domain of 0-13 to map to 0-2.
  // Below => (quality+3*mine_quality+2)%14 : ore_size -> 0-7: 0, 8-12: 1, 13: 2
  //   This correlates to 8/14 chance for small, 5/14 for med, 1/14 for large.
  //   Except adamantium only has a domain of 0-4 for mine_quality 0 so it will be small (5% chance) regardless.
  //     And adamantium only has a domain of 0-7 for mine_quality 1 so it will be small 8% regardless.
  //     And adamantium only has a domain of 0-10 for mine_quality 2 so it will be small 8% or medium 3%.
  //     And adamantium has a domain of 0-13 for mine_quality 3 so it will be 8% small, 5% medium, 1% large.
  //   Except iron has a domain of 3-13 for mine_quality 0 so it will be 5/11 small, 5/11 medium, 1/11 large.
  //     And iron has a domain of 6-13 for mine_quality 1 so it will be 2/8 small, 5/8 medium, 1/8 large.
  //     And iron has a domain of 9-13 for mine_quality 2 so it will be 4/5 medium, 1/5 large.
  //     And iron has a domain of 12-13 for mine_quality 3 so it will be 1/2 medium, 1/2 large.
  //     Note: The iron-size probabilities are assuming that the metal is iron (they are numerator% chance overall).
  int ore_size = (((quality+3*mine_quality+2) % 14) - 3) / 5;

  // Range needs to be from 400260 to 400283 : 0 * 3 + 0 + 400260 to 7 * 3 + 2 + 400260.
  return (ore_type * 3) + ore_size + LOWEST_ORE_VNUM;

/* Old code from when vnums were not in order.
 * The "LARGE_ADAMANTIUM_ORE" constants and such are not updated.
 * The above is so much nicer to read, and has decent comments and no if's.

  // So, quality 0: 1 - 99, 1: 2 - 109, 2: 3 - 119, 3: 4 - 129
  int x = number(1 + mine_quality, 99 + mine_quality * 10);

  if(x >= 99 + mine_quality * 9) // 0: 99, 1: 108, 2: 117, 3: 126
    return LARGE_ADAMANTIUM_ORE;
  if(x >= 98 + mine_quality * 9) // 0: 98, 1: 107, 2: 116, 3: 125
    return MEDIUM_ADAMANTIUM_ORE;
  if(x >= 97 + mine_quality * 9)
    return SMALL_ADAMANTIUM_ORE;

  if(x >= 96 + mine_quality * 8)
    return LARGE_MITHRIL_ORE;
  if(x >= 95 + mine_quality * 7)
    return MEDIUM_MITHRIL_ORE;
  if(x >= 94 + mine_quality * 6)
    return SMALL_MITHRIL_ORE;

  if(x >= 91 + mine_quality * 5)
    return LARGE_PLATINUM_ORE;
  if(x >= 88 + mine_quality * 4)
    return MEDIUM_PLATINUM_ORE;
  if(x >= 85 + mine_quality * 3)
    return SMALL_PLATINUM_ORE;

  if(x >= 81 + mine_quality * 2)
    return LARGE_GOLD_ORE;
  if(x >= 77 + mine_quality * 1)
    return MEDIUM_GOLD_ORE;
  if(x >= 73 + mine_quality * 0)
    return SMALL_GOLD_ORE;

  if(x >= 66)
    return LARGE_SILVER_ORE;
  if(x >= 59)
    return MEDIUM_SILVER_ORE;
  if(x >= 52)
    return SMALL_SILVER_ORE;

  if(x >= 44)
    return LARGE_COPPER_ORE;
  if(x >= 36)
    return MEDIUM_COPPER_ORE;
  if(x >= 28)
    return SMALL_COPPER_ORE;

  if(x >= 19)
    return LARGE_IRON_ORE;
  if(x >= 10)
    return MEDIUM_IRON_ORE;

  return SMALL_IRON_ORE;
*/
}

P_obj get_ore_from_mine(P_char ch, int mine_quality)
{
  P_obj ore;
  int ore_type = random_ore(mine_quality);
  ore = read_object(ore_type, VIRTUAL);
  if( !ore )
  {
    return NULL;
  }
  ore->value[4] = time(NULL);
  return ore;
}

P_obj get_gem_from_mine(P_char ch, int mine_quality)
{
  P_obj gem;
  int gem_type;

  // 1-50 taken by just each item (+ a few). 51-170 are repeats heavier for cheaper.
  switch( number( 1, 170 ) )
  {
    case 1:
    case 9:
    case 165:
    case 166:
    case 167:
    case 168:
    case 169:
    case 170:
      gem_type = TINY_IMP_TOPAZ;
      break;
    case 2:
    case 10:
    case 159:
    case 160:
    case 161:
    case 162:
    case 163:
    case 164:
      gem_type = REG_IMP_TOPAZ;
      break;
    case 3:
    case 154:
    case 155:
    case 156:
    case 157:
    case 158:
      gem_type = LG_IMP_TOPAZ;
      break;
    case 4:
    case 149:
    case 150:
    case 151:
    case 152:
    case 153:
      gem_type = TINY_REG_TOPAZ;
      break;
    case 5:
    case 144:
    case 145:
    case 146:
    case 147:
    case 148:
      gem_type = REG_REG_TOPAZ;
      break;
    case 6:
    case 139:
    case 140:
    case 141:
    case 142:
    case 143:
      gem_type = LG_REG_TOPAZ;
      break;
    case 7:
    case 134:
    case 135:
    case 136:
    case 137:
    case 138:
      gem_type = FLAWLESS_TOPAZ;
      break;
    case 8:
    case 129:
    case 130:
    case 131:
    case 132:
    case 133:
      gem_type = LG_FLAWLESS_TOPAZ;
      break;
    case 11:
    case 19:
    case 125:
    case 126:
    case 127:
    case 128:
      gem_type = TINY_IMP_SAPPHIRE;
      break;
    case 12:
    case 20:
    case 121:
    case 122:
    case 123:
    case 124:
      gem_type = REG_IMP_SAPPHIRE;
      break;
    case 13:
    case 117:
    case 118:
    case 119:
    case 120:
      gem_type = LG_IMP_SAPPHIRE;
      break;
    case 14:
    case 113:
    case 114:
    case 115:
    case 116:
      gem_type = TINY_REG_SAPPHIRE;
      break;
    case 15:
    case 109:
    case 110:
    case 111:
    case 112:
      gem_type = REG_REG_SAPPHIRE;
      break;
    case 16:
    case 105:
    case 106:
    case 107:
    case 108:
      gem_type = LG_REG_SAPPHIRE;
      break;
    case 17:
    case 101:
    case 102:
    case 103:
    case 104:
      gem_type = FLAWLESS_SAPPHIRE;
      break;
    case 18:
    case 97:
    case 98:
    case 99:
    case 100:
      gem_type = LG_FLAWLESS_SAPPHIRE;
      break;
    case 21:
    case 29:
    case 94:
    case 95:
    case 96:
      gem_type = TINY_IMP_EMERALD;
      break;
    case 22:
    case 30:
    case 91:
    case 92:
    case 93:
      gem_type = REG_IMP_EMERALD;
      break;
    case 23:
    case 88:
    case 89:
    case 90:
      gem_type = LG_IMP_EMERALD;
      break;
    case 24:
    case 85:
    case 86:
    case 87:
      gem_type = TINY_REG_EMERALD;
      break;
    case 25:
    case 82:
    case 83:
    case 84:
      gem_type = REG_REG_EMERALD;
      break;
    case 26:
    case 79:
    case 80:
    case 81:
      gem_type = LG_REG_EMERALD;
      break;
    case 27:
    case 76:
    case 77:
    case 78:
      gem_type = FLAWLESS_EMERALD;
      break;
    case 28:
    case 73:
    case 74:
    case 75:
      gem_type = LG_FLAWLESS_EMERALD;
      break;
    case 31:
    case 39:
    case 71:
    case 72:
      gem_type = TINY_IMP_DIAMOND;
      break;
    case 32:
    case 40:
    case 69:
    case 70:
      gem_type = REG_IMP_DIAMOND;
      break;
    case 33:
    case 67:
    case 68:
      gem_type = LG_IMP_DIAMOND;
      break;
    case 34:
    case 65:
    case 66:
      gem_type = TINY_REG_DIAMOND;
      break;
    case 35:
    case 63:
    case 64:
      gem_type = REG_REG_DIAMOND;
      break;
    case 36:
    case 61:
    case 62:
      gem_type = LG_REG_DIAMOND;
      break;
    case 37:
    case 59:
    case 60:
      gem_type = FLAWLESS_DIAMOND;
      break;
    case 38:
    case 57:
    case 58:
      gem_type = LG_FLAWLESS_DIAMOND;
      break;
    case 41:
    case 49:
    case 56:
      gem_type = TINY_IMP_RUBY;
      break;
    case 42:
    case 50:
    case 55:
      gem_type = REG_IMP_RUBY;
      break;
    case 43:
    case 54:
      gem_type = LG_IMP_RUBY;
      break;
    case 44:
    case 53:
      gem_type = TINY_REG_RUBY;
      break;
    case 45:
    case 52:
      gem_type = REG_REG_RUBY;
      break;
    case 46:
    case 51:
      gem_type = LG_REG_RUBY;
      break;
    case 47:
      gem_type = FLAWLESS_RUBY;
      break;
    case 48:
      gem_type = LG_FLAWLESS_RUBY;
      break;
    default:
      gem_type = TINY_IMP_TOPAZ;
      break;
  }

  gem = read_object(gem_type, VIRTUAL);
  if( !gem )
  {
    return NULL;
  }
  gem->value[4] = time(NULL);
  return gem;
}

P_obj get_pole(P_char ch)
{

  P_obj    t_obj, next_obj;

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if (obj_index[t_obj->R_num].virtual_number == POLE_VNUM ||
        obj_index[t_obj->R_num].virtual_number == 6025  ||
        obj_index[t_obj->R_num].virtual_number == 26200 ||
        obj_index[t_obj->R_num].virtual_number == 43120 ||
        obj_index[t_obj->R_num].virtual_number == 66709 ||
        obj_index[t_obj->R_num].virtual_number == 93913 ||
        obj_index[t_obj->R_num].virtual_number == 29439 ||
        obj_index[t_obj->R_num].virtual_number == 88903
       )
    {
      return t_obj;
    }
  }
  return NULL;
}

void do_fish(P_char ch, char*, int cmd)
{
  P_obj pole;
  struct fishing_data data;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_NPC(ch) || GET_CHAR_SKILL(ch, SKILL_FISHING) == 0 )
  {
    send_to_char("You don't know how to fish.\n", ch);
    return;
  }

  if( get_scheduled(ch, event_fish_check) )
  {
    send_to_char("You're already fishing!\n", ch);
    return;
  }

  if( !MIN_POS(ch, POS_STANDING + STAT_NORMAL) )
  {
    send_to_char("You're too relaxed to fish.\n", ch);
    return;
  }

  if( !(pole = get_pole(ch)) )
  {
    send_to_char("How are you supposed to fish when you don't have anything ready to fish with?\n", ch);
    return;
  }

  if( !IS_WATER_ROOM(ch->in_room) )
  {
    send_to_char("Well you DO have a fishing pole, but where do you plan to fish???\n", ch);
    return;
  }

  if( IS_DISGUISE(ch) )
  {
    send_to_char("Fishing will ruin your disguise!\r\n", ch);
    return;
  }

  // Start fishing
  send_to_char("You cast your pole and start waiting...\n", ch);

  data.room = ch->in_room;
  // Two ticks for Immortals.. for testing
  data.counter = IS_TRUSTED(ch) ? 2 : (140 - GET_CHAR_SKILL(ch, SKILL_FISHING)) / 4;

  add_event( event_fish_check, PULSE_VIOLENCE, ch, 0, 0, 0, &data, sizeof(struct fishing_data));
}

void event_fish_check(P_char ch, P_char victim, P_obj, void *data)
{
  struct fishing_data *fdata = (struct fishing_data*)data;
  P_obj pole, fish;
  char  buf[MAX_STRING_LENGTH], dbug[MAX_STRING_LENGTH];
  const int fishes[12] = {293 , 294 , 295 , 318 , 319 , 330 , 332 , 333 , 334 , 335, 355, 356};
  int random = number(0,11);
  pole = get_pole(ch);
  struct affected_type *afp, af;

  if( !ch->desc || IS_FIGHTING(ch) || IS_DESTROYING(ch) || (ch->in_room != fdata->room)
    || !MIN_POS(ch, POS_STANDING + STAT_NORMAL) || IS_SET(ch->specials.affected_by, AFF_HIDE)
    || IS_IMMOBILE(ch) || !IS_AWAKE(ch) || IS_STUNNED(ch) || IS_CASTING(ch) )
  {
    send_to_char("You stop fishing.\n", ch);
    return;
  }

  if( !IS_WATER_ROOM(ch->in_room) )
  {
    send_to_char("You stop fishing.\n", ch);
    return;
  }

  if( IS_DISGUISE(ch) )
  {
    send_to_char("Fishing will ruin your disguise!\r\n", ch);
    return;
  }

  if( !pole )
  {
    send_to_char("How are you supposed to fish when you don't have anything ready to fish with?\n", ch);
    return;
  }

  if( fdata->counter == 0 )
  {

    if( GET_CHAR_SKILL(ch, SKILL_FISHING) < number(1,125) )
    {
      send_to_char("You didn't catch a thing..\n", ch);
      return;
    }

  if( !(fish = read_object(fishes[random], VIRTUAL)) )
  {
    wizlog(56, "Bug with fishing no fish with vnum %d", fishes[random]);
    return;
  }

  if( !number(0,2) )
  {
    DamageOneItem(ch, 9, pole, FALSE);
  }

  act("You reel in $p on your fishing pole!", FALSE, ch, fish, NULL, TO_CHAR);
  act("$n reels in $p on $s fishing pole", FALSE, ch, fish, NULL, TO_ROOM);

  // Diminishing returns to stop afk bot-fishing for exp, clears after 10 ticks of not catching anything.
  if( !(afp = get_spell_from_char(ch, SKILL_FISHING)) )
  {
    bzero(&af, sizeof(af));
    af.type = SKILL_FISHING;
    af.modifier = 500;
    af.duration = 10;
    af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOMSG | AFFTYPE_OFFLINE;
    afp = affect_to_char(ch, &af);
  }
  else
  {
    afp->modifier -= 10;
    afp->modifier = MAX( afp->modifier, 20 );
    afp->duration = 10;
  }

  gain_exp(ch, NULL, (GET_CHAR_SKILL(ch, SKILL_FISHING) * afp->modifier)/100, EXP_BOON);

  //fish->timer[0] = time(NULL); Fish no longer decay - drannak 5/13/13
  obj_to_char(fish, ch);
  return;
  }


  if (GET_VITALITY(ch) < 10)
  {
    send_to_char("You are too exhausted to continue fishing.\n", ch);
    return;
  }

  send_to_char("You continue fishing...\n", ch);
  notch_skill(ch, SKILL_FISHING, 10);
  GET_VITALITY(ch) -= (number(0,100) > GET_CHAR_SKILL(ch, SKILL_FISHING)) ? 3 : 2;

  fdata->counter--;
  add_event(event_fish_check, PULSE_VIOLENCE, ch, 0, 0, 0, fdata, sizeof(struct fishing_data));

  //noise distance calc
  for (P_desc i = descriptor_list; i; i = i->next)
    {
      if( i->connected != CON_PLYNG ||
          ch == i->character ||
          i->character->following == ch ||
          world[i->character->in_room].zone != world[ch->in_room].zone ||
          ch->in_room == i->character->in_room ||
          ch->in_room == real_room(i->character->specials.was_in_room) ||
          real_room(ch->specials.was_in_room) == i->character->in_room )
      {
        continue;
      }
      
      int dist = calculate_map_distance(ch->in_room, i->character->in_room);

  if(dist <= 550 && (number(1, 12) < 3))
  {
  zone_spellmessage(ch->in_room,
    "&+cThe &+Csoft &+csound of &+Cwater&+c splashing can be heard in the distance...&n\r\n",
    "&+cThe &+Csoft &+csound of &+Cwater&+c splashing can be heard to the %s...&n\r\n");
  }
 }
}

int mine(P_obj obj, P_char ch, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( obj->value[0] <= 0 )
    {
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      return TRUE;
    }
  }

  if( cmd == CMD_MINE )
  {
    if( !ch || !IS_PC(ch) || !IS_ALIVE(ch) )
    {
      return FALSE;
    }

    if( GET_CHAR_SKILL(ch, SKILL_MINE) == 0 )
    {
      send_to_char("You don't know how to mine.\n", ch);
      return TRUE;
    }

    if( get_scheduled(ch, event_mine_check) )
    {
      send_to_char("You're already mining!\n", ch);
      return TRUE;
    }

    if( !MIN_POS(ch, POS_STANDING + STAT_NORMAL) )
    {
      send_to_char("You're too relaxed to mine.\n", ch);
      return TRUE;
    }

    P_obj pick = get_pick(ch);
    if( !pick )
    {
      send_to_char("You need to be wielding a suitable mining pick.\n", ch);
      return TRUE;
    }

    if( get_mine_content(obj) <= 0 )
    {
      send_to_char("This area has been completely deplenished!\n", ch);
      return TRUE;
    }

    // start mining
    send_to_char("You begin to mine...\n", ch);

    struct mining_data data;
    data.room = ch->in_room;
    // Immortals get 1 bout of 'You continue mining..' to make sure it works right.
    if( IS_TRUSTED(ch) )
    {
      data.counter = 2;
    }
    else
    {
      // At 1 skill, roughly twice as long as 100 skill.  At 100 skill, ticks represented by val2 = mine_duration.
      data.counter = (obj->value[2] * 200) / (100 + GET_CHAR_SKILL( ch, SKILL_MINE ));
      // Anti-cheater code: less than 16 sec?
      if( data.counter < 4 )
      {
        // Punish with a long counter... 3 mins sounds good.
        data.counter = 45;
      }
      // Add a little real life luck variance to create the myths!
      data.counter += number( -1, 1 );
    }
    data.mine_quality = obj->value[1];
    data.mine_type = obj_index[obj->R_num].virtual_number;

    remove_mine_content(obj);

    if( get_mine_content(obj) <= 0 )
    {
      send_to_char("There is very little left, but you keep digging one more time!\n", ch);
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'
    }

    add_event( event_mine_check, PULSE_VIOLENCE, ch, 0, 0, 0, &data, sizeof(struct mining_data));
    return TRUE;
  }

  return FALSE;
}

void event_mine_check( P_char ch, P_char victim, P_obj, void *data )
{
  struct mining_data *mdata = (struct mining_data*)data;
  P_obj ore, pick;
  char  buf[MAX_STRING_LENGTH], dbug[MAX_STRING_LENGTH];
  float newcost;
  bool randommob;
  P_char mob;

  pick = get_pick(ch);

  if( !IS_ALIVE(ch) )
  {
    logit( LOG_DEBUG, "event_mine_check: bad ch (%s)", ch ? J_NAME(ch) : "NULL" );
    return;
  }

  if( !ch->desc || IS_FIGHTING(ch) || IS_DESTROYING(ch)
    || (ch->in_room != mdata->room) || !MIN_POS(ch, POS_STANDING + STAT_NORMAL)
    || IS_SET(ch->specials.affected_by, AFF_HIDE) || IS_IMMOBILE(ch)
    || !IS_AWAKE(ch) || IS_STUNNED(ch) || IS_CASTING(ch) || IS_AFFECTED2(ch, AFF2_CASTING) )
  {
    send_to_char("You stop mining.\n", ch);
    return;
  }

  // No more invis mining.
  appear(ch);

  if( IS_DISGUISE(ch) )
  {
    send_to_char("Mining will ruin your disguise!\r\n", ch);
    return;
  }


  if( !pick )
  {
    send_to_char("How are you supposed to mine when you don't have anything ready to mine with?\n", ch);
    return;
  }

  if( --mdata->counter <= 0 )
  {
    if( mdata->mine_type == VOBJ_MINE )
    {
      ore = get_ore_from_mine(ch, mdata->mine_quality);
      if( !ore )
      {
        wizlog(56, "event_mine_check: couldn't load ore, quality %d.", mdata->mine_quality );
        logit( LOG_DEBUG, "event_mine_check: couldn't load ore, quality %d.", mdata->mine_quality );
        send_to_char( "Your efforts were thwarted by an unseen force.  Tell a God.\n\r", ch );
        return;
      }
      // Moved to a function to make it more readable.
      newcost = calc_ore_cost( ch, ore );
    }
    else if( mdata->mine_type == VOBJ_GEMMINE )
    {
      randommob = FALSE;
      if( !number(0,20) )
      {
        randommob = TRUE;
        send_to_char( "You dug up something .. that moves!\n\r", ch );
      }
      ore = get_gem_from_mine(ch, mdata->mine_quality);
      if( !ore )
      {
        wizlog(56, "event_mine_check: couldn't load gem, quality %d.", mdata->mine_quality );
        logit( LOG_DEBUG, "event_mine_check: couldn't load ge,, quality %d.", mdata->mine_quality );
        send_to_char( "Your efforts were thwarted by a mysterious force.  Tell a God.\n\r", ch );
        return;
      }
      int type = OBJ_VNUM(ore) - 504;
      // Base price based on gem type.
      switch( type / 8 )
      {
        // Topaz
        case 0:
          newcost = 200000;
          if( randommob )
          {
            // A hideous zombie (#92)
            // a corpse gatherer (#89)
            mob = read_mobile(number(0, 1) ? 89 : 92, VIRTUAL);
            // Enters from below.
            char_to_room( mob, mdata->room, 4 );
          }
          break;
        // Sapphire
        case 1:
          newcost = 250000;
          if( randommob )
          {
            // An antlion (#94)
            // A purple worm (#95)
            mob = read_mobile(number(94, 95), VIRTUAL);
            // Enters from below.
            char_to_room( mob, mdata->room, 4 );
          }
          break;
        // Emerald
        case 2:
          newcost = 300000;
          if( randommob )
          {
            // a swarm of earth beetles (#97)
            // A skeletal warrior (#93)
            mob = read_mobile(number(0, 1) ? 93 : 97, VIRTUAL);
            // Enters from below.
            char_to_room( mob, mdata->room, 4 );
          }
          break;
        // Diamond
        case 3:
          newcost = 250000;
          if( randommob )
          {
            // A mohrg (#91)
            // a sleeping earth elemental (#96)
            mob = read_mobile(number(0, 1) ? 91 : 96, VIRTUAL);
            // Enters from below.
            char_to_room( mob, mdata->room, 4 );
          }
          break;
        // Ruby
        case 4:
          newcost = 300000;
          if( randommob )
          {
            // a GIANT purple worm (#90)
            // a burrow wraith (#98)
            mob = read_mobile( number(0, 1) ? 90 : 98, VIRTUAL);
            // Enters from below.
            char_to_room( mob, mdata->room, 4 );
          }
          break;
        // Buggy gemstones are 1 plat base.
        default:
          newcost = 1000;
          break;
      }
      newcost *= (float)GET_LEVEL(ch) / 56.0;
      newcost *= (GET_CHAR_SKILL(ch, SKILL_MINE)) / 100.0;
      switch( type % 8 )
      {
        // Tiny imperfect.
        case 0:
          break;
        // Reg Imperfect.
        case 1:
          newcost *= 1.05;
          break;
        // Large Imperfect.
        case 2:
          newcost *= 1.15;
          break;
        // Tiny Reg.
        case 3:
          newcost *= 1.20;
          break;
        // Reg Reg.
        case 4:
          newcost *= 1.25;
          break;
        // Large Reg.
        case 5:
          newcost *= 1.35;
          break;
        // Reg Flawless.
        case 6:
          newcost *= 1.65;
          break;
        // Large Flawless.
        case 7:
          newcost *= 1.85;
          break;
        default:
          break;
      }
    }
    else
    {
      wizlog(56, "event_mine_check: unknown mine type %d.", mdata->mine_type );
      logit( LOG_DEBUG, "event_mine_check: unknown mine type %d.", mdata->mine_type );
      send_to_char( "Your mine doesn't seem to be a mine anymore.  Tell a God.\n\r", ch );
      return;
    }

    if(number(80, 140) < GET_C_LUK(ch))
    {
      newcost *= 1.3;
      send_to_char("&+yYou &+Ygently&+y break the &+Lore &+yfree from the &+Lrock&+y, preserving its natural form.&n\r\n", ch);
    }

    act("Your mining efforts turn up $p&n!", FALSE, ch, ore, 0, TO_CHAR);
    act("$n finds $p&n!", FALSE, ch, ore, 0, TO_ROOM);

    gain_exp(ch, NULL, (GET_CHAR_SKILL(ch, SKILL_MINE) * 4), EXP_BOON);
    ore->cost = newcost;
    obj_to_room(ore, ch->in_room);
    return;
  }

  if( get_property("halloween", 0.000) && (number(0, 100) < get_property("halloween.zombie.chance", 5.000)))
  {
    halloween_mine_proc(ch);
  }

  if (GET_VITALITY(ch) < 10)
  {
    send_to_char("You are too exhausted to continue mining.\n", ch);
    return;
  }

  if( IS_RIDING(ch) )
  {
    send_to_char( "Mining while mounted?  Good luck!\n", ch );
    if( !number( 0, GET_CHAR_SKILL(ch, SKILL_MINE)/20 ) )
    {
      act( "You fumble your $p!", FALSE, ch, pick, 0, TO_CHAR );
      if( ch->equipment[WIELD] == pick )
      {
        unequip_char( ch, WIELD );
        obj_to_char( pick, ch );
      }
      if( ch->equipment[WIELD2] == pick )
      {
        unequip_char( ch, WIELD2 );
        obj_to_char( pick, ch );
      }
      else
      {
        logit(LOG_DEBUG, "event_mine_check: %s has pick '%s' (%d) but not in slot WIELD/WIELD2.",
          J_NAME(ch), pick->short_description, OBJ_VNUM(pick) );
      }
      return;
    }
  }

  if( !notch_skill(ch, SKILL_MINE, get_property("skill.notch.mining", 2.5))
    && !number(0, (GET_CHAR_SKILL(ch, SKILL_MINE) * 2) ))
  {
    send_to_char("You thought you found something, but it was just worthless rock.\n", ch);  
    return;
  }

  if( !number(0, 999) )
  {
    create_parchment(ch);
  }

  // If pick breaks, return.
  if( !number(0,4) && (OBJ_VNUM(pick) != 83318) && DamageOneItem(ch, 1, pick, false) )
  {
    return;
  }

  send_to_char("You continue mining...\n", ch);
  notch_skill(ch, SKILL_MINE, get_property("skill.notch.mining", 2.5));
  GET_VITALITY(ch) -= (number(0,100) > GET_CHAR_SKILL(ch, SKILL_MINE)) ? 3 : 2;

  add_event(event_mine_check, PULSE_VIOLENCE, ch, 0, 0, 0, mdata, sizeof(struct mining_data));

  // Noise distance calc
  for (P_desc i = descriptor_list; i; i = i->next)
  {
    if( i->connected != CON_PLYNG || ch == i->character
      || i->character->following == ch
      || world[i->character->in_room].zone != world[ch->in_room].zone
      || ch->in_room == i->character->in_room
      || ch->in_room == real_room(i->character->specials.was_in_room)
      || real_room(ch->specials.was_in_room) == i->character->in_room )
    {
      continue;
    }

    int dist = calculate_map_distance(ch->in_room, i->character->in_room);

    if(dist <= 550 && (number(1, 12) < 3))
    {
      zone_spellmessage(ch->in_room,
      "&+yThe sound of &+wmetal &+yhewing &+Lrock&+y can be heard in the distance...&n\r\n",
      "&+yThe sound of &+wmetal &+yhewing &+Lrock&+y can be heard in the distance to the %s...&n\r\n");
    }
  }
}

int smith(P_char ch, P_char pl, int cmd, char *arg)
{
  struct smith_data *sdata;
  char buffer[256];
  P_obj tobj, needed_ore[5];
  int i, j, choice;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && !number(0,6))
  {
    act("$n hums a soft melody as $e forges another piece of armor.",
        FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  if( cmd != CMD_FORGE )
  {
    return FALSE;
  }

  j = GET_VNUM(ch);
  for (i = 0; smith_array[i].vnum > 0; i++)
  {
    if( smith_array[i].vnum == j )
    {
      break;
    }
  }

  if( smith_array[i].vnum == 0 )
  {
    return FALSE;
  }
  else
  {
    sdata = &smith_array[i];
  }

  if( !arg || !*arg )
  {
    act("$n tells you, 'I can forge the following items:'", FALSE, ch, 0, pl, TO_VICT);
    for( i = 0; i < SMITH_MAX_ITEMS && sdata->items[i] != -1; i++ )
    {
      sprintf(buffer, "%d) %s\n", i + 1, forge_item_list[sdata->items[i]].short_desc);
      send_to_char(buffer, pl);
    }
    return TRUE;
  }

  choice = atoi(arg);

  // Can't 'forge 0' or 'forge -1'.  Also, can't 'forge <num>' where <num> goes off the list.
  if( choice < 1 || choice > i )
    return FALSE;

  // Convert from list item to forge_item_list entry.
  choice = sdata->items[choice - 1];

  // If they're too broke,
  if( GET_MONEY(pl) < forge_prices[i-1] )
  {
    // Explain reality.
    forge_describe(choice, pl);
    return TRUE;
  }

  // Nullify all needed_ore entries.
  for( i = 0; i < 5; i++ )
  {
    needed_ore[i] = NULL;
  }
  // Look through inventory for each needed ore.
  for( i = 0, j = 0; i < 5 && forge_item_list[choice].ore_needed[i]; i++ )
  {
    for( tobj = ch->carrying; tobj; tobj = tobj->next_content )
    {
      // If we found a needed ore.
      if( forge_item_list[choice].ore_needed[i] == OBJ_VNUM(tobj) )
      {
        // Stick it in the needed_ore array
        needed_ore[j++] = tobj;
        // And pull it from pl, so we don't find the same ore twice.
        obj_from_char(tobj);
        break;
      }
    }
    // If we encounter a needed ore that pl doesn't have,
    if( !tobj )
    {
      // Send a message about it,
      forge_describe(choice, pl);
      // And give back the ores we pulled.
      while( j-- > 0 )
      {
        obj_to_char(needed_ore[j], pl);
      }
      return TRUE;
    }
  }

  // Create item 'choice' for 'pl' out of material type 'material'
  if( !(tobj = forge_create(choice, pl, needed_ore[0]->material)) )
  {
    // Send an error message if we failed to create item.
    send_to_char("&+YFailed to create the item.  Please tell an Immortal if you continue to have problems.\n\r", pl);
    // And give back the ores we pulled.
    while( j-- > 0 )
    {
      obj_to_char(needed_ore[j], pl);
    }
    return TRUE;
  }

  // Take their money.
  sprintf(buffer, "You hand $N %s.", coin_stringv(forge_prices[i-1]));
  act(buffer, FALSE, pl, 0, ch, TO_CHAR);
  SUB_MONEY(pl, forge_prices[i-1], 0);

  // And their ore.
  while( j-- > 0 )
  {
    extract_obj(needed_ore[j], TRUE); // Ore is not an arti, but was 'in game.'
  }

  act("$n takes the pieces of ore and starts to work on them.\n"
      "With powerful hammer strikes, the material starts to quickly take\n"
      "shape of $p.\n"
      "$n puts it in water which explodes in a cloud of &+Csteam&n hissing loudly.\n"
      "'&+WThere you go!&n', $n gives you $p.", FALSE, ch, tobj, pl, TO_VICT);
  act("$n takes the pieces of ore and starts to work on them.\n"
      "With powerful hammer strikes, the material starts to quickly take\n"
      "shape of $p.\n"
      "$n puts it in water which explodes in a cloud of &+Csteam&n hissing loudly.\n"
      "'&+WThere you go!&n', $n gives $N $p.", FALSE, ch, tobj, pl, TO_NOTVICT);

  obj_to_char(tobj, pl);
  return TRUE;
}

void initialize_tradeskills()
{
  int i;

  obj_index[real_object0(VOBJ_MINE)].func.obj = mine;
  obj_index[real_object0(VOBJ_GEMMINE)].func.obj = mine;

  for (i = 0; mine_data[i].name; i++)
  {
    load_mines(mine_data[i].reload, TRUE, i);
  }

  // set procs on smiths
  for (i = 0; smith_array[i].vnum; i++)
    if (!mob_index[real_mobile0(smith_array[i].vnum)].func.mob)
      mob_index[real_mobile0(smith_array[i].vnum)].func.mob = smith;
}

bool invalid_mine_room(int rroom_id)
{
  if( IS_SET(world[rroom_id].room_flags, PRIVATE) || 
      IS_SET(world[rroom_id].room_flags, PRIV_ZONE) || 
      //IS_SET(world[rroom_id].room_flags, NO_TELEPORT) || 
      world[rroom_id].dir_option[DOWN] || 
      IS_WATER_ROOM(rroom_id) || 
      world[rroom_id].sector_type == SECT_MOUNTAIN || 
      world[rroom_id].sector_type == SECT_ROAD || 
      world[rroom_id].sector_type == SECT_UNDRWLD_MOUNTAIN || 
      world[rroom_id].sector_type == SECT_UNDRWLD_NOGROUND ||
      world[rroom_id].sector_type == SECT_UNDRWLD_NOSWIM ||
      world[rroom_id].sector_type == SECT_UNDRWLD_WATER ||
      world[rroom_id].sector_type == SECT_UNDRWLD_INSIDE ||
      world[rroom_id].sector_type == SECT_UNDRWLD_CITY ||
      world[rroom_id].sector_type == SECT_OCEAN ||
      world[rroom_id].sector_type == SECT_INSIDE ||
      world[rroom_id].sector_type == SECT_CASTLE ||
      world[rroom_id].sector_type == SECT_CASTLE_WALL ||
      world[rroom_id].sector_type == SECT_CASTLE_GATE)
    return TRUE;
  
  for( P_obj tobj = world[rroom_id].contents; tobj; tobj = tobj->next )
  {
    if( OBJ_VNUM(tobj) == VOBJ_MINE )
      return TRUE;
  }

  return FALSE;
}

bool load_one_mine(int map)
{
  P_obj mine = read_object(mine_data[map].type, VIRTUAL);

  if( !mine )
  {
    wizlog(56, "Error loading mine obj [%d]", VOBJ_MINE);
    return FALSE;
  }

  int start = real_room(mine_data[map].start);
  int end = real_room(mine_data[map].end);

  int tries = 0;
  int to_room = -1;

  do {
    to_room = number(start, end);

    /* if it's valid and a mine friendly location, or just lucky, put a mine there */
    if( !invalid_mine_room(to_room) && (mine_friendly(to_room) || number(0,100) < 15) )
      break;

    tries++;
  } while ( tries < 10000 );

  if( tries >= 10000 || invalid_mine_room(to_room) )
  {
    return FALSE;
  }

  int random = number(0,99);

  mine->value[2] = mine_data[map].mine_duration;
  if( mine_data[map].type == VOBJ_GEMMINE )
  {
    mine->value[0] = number(10, 25);
    mine->value[1] = 100 + number(0, 3);
    // Description already set in heavens.obj file.
  }
  else if( random < 3 )
  {
    mine->value[0] = number(20, 30);
    mine->value[1] = 3;
    mine->description = str_dup("&+LThe &+yearth &+Lhere is &+cbr&+Lim&+Cm&+Ling with &+Yore&+L - it's the &+GMother &+LLode!&n");
  }
  else if( random < 20 )
  {
    mine->value[0] = number(10, 15);
    mine->value[1] = 2;
    mine->description = str_dup("&+LThe &+yearth&+L here is &+cst&+Lrea&+ck&+Led &+Lwith &+core&+L.&n");
  }
  else if( random < 60 )
  {
    mine->value[0] = number( 8, 12);
    mine->value[1] = 1;
    mine->description = str_dup("&+LA few chunks of &+Yore &+Lpoke out of the ground here.&n");
  }
  else
  {
    mine->value[0] = number( 5, 10);
    mine->value[1] = 0;
    mine->description = str_dup("&+LA few glimmers &+Ws&+wpa&+Wrk&+wle&+L in the &+yearth &+Lhere.&n");
  }

  obj_to_room(mine, to_room);
  wizlog(56, "Mine (%d) loaded in %s [%d]", mine->value[1], world[to_room].name, ROOM_VNUM(to_room) );

  return TRUE;
}

void load_mines(bool set_event, bool load_all, int map)
{
  int max_mines, num_mines = 0, mine_type;
  struct mines_event_data mdata;

  mine_type = mine_data[map].type;
  for( P_obj tobj = object_list; tobj; tobj = tobj->next )
  {
    if( (OBJ_VNUM(tobj) == mine_type) && (tobj->loc.room > 0) &&
        (world[tobj->loc.room].number >= mine_data[map].start) &&
        (world[tobj->loc.room].number <= mine_data[map].end) )
    {
      num_mines++;
    }
  }
  max_mines = mines_properties(map);
  max_mines += number(-max_mines/6,max_mines/6);
  //debug("mines currently loaded: %d / %d", num_mines, max_mines );

  if( num_mines < max_mines )
  {
    if( load_all )
    {
      for( int i = 0; (i < ( max_mines - num_mines )); i++ )
      {
        load_one_mine(map);
      }
    }
    else
    {
      load_one_mine(map);
    }
  }

  mdata.map = map;

  if( set_event )
  {
    add_event(event_load_mines, ( WAIT_SEC * 60 ) * ((int) get_property("mines.reloadMins", 10)), NULL, NULL, 0, 0, &mdata, sizeof(mdata));
  }
}

void event_load_mines(P_char ch, P_char victim, P_obj, void *data)
{
  struct mines_event_data *mdata = (struct mines_event_data*)data;

  if (!mdata)
  {
    debug("Passed null pointer to event_load_mines()");
    return;
  }

  load_mines(TRUE, FALSE, mdata->map);
}

P_obj get_bandage(P_char ch)
{
  P_obj    t_obj, next_obj;

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if (t_obj->type == ITEM_BANDAGE)
    {
      return t_obj;
    }
  }

  return NULL;
}

int poison_common_remove(P_char ch);

struct bandage_data {
  int room;
  int healed;
  int maxheal;
};

void event_bandage_check(P_char ch, P_char victim, P_obj, void *data)
{
  struct bandage_data *mdata = (struct bandage_data*)data;
  P_obj bandage;
  char  buf[MAX_STRING_LENGTH];
  int healed = 0;

  if(!ch || !victim)
    return;

  if (mdata->healed >= mdata->maxheal) {
    sprintf(buf, "You can't &+Wbandage&n any more with this bandage.\n");
    send_to_char(buf, ch);
    return;
  }

  if (!ch->desc ||
      IS_FIGHTING(ch) ||
      IS_DESTROYING(ch) ||
      (ch->in_room != mdata->room) ||
      (GET_STAT(ch) < STAT_SLEEPING) ||                    
      IS_SET(ch->specials.affected_by, AFF_HIDE) ||
      IS_FIGHTING(victim) ||
      IS_DESTROYING(victim) ||
      (victim->in_room != mdata->room) ||            
      IS_SET(victim->specials.affected_by, AFF_HIDE) ||
      affected_by_spell(ch, TAG_FIRING) ||
      affected_by_spell(victim, TAG_FIRING) ||
      IS_AFFECTED2(ch, AFF2_MEMORIZING) ||
      IS_AFFECTED2(victim, AFF2_MEMORIZING) ||
      IS_IMMOBILE(ch))
  {
    send_to_char("You are no longer bandaging...\n", ch);
    send_to_char("You are no longer being &+Wbandaged&n.\n", victim);
    return;  
  }

  if (GET_VITALITY(ch) < 10) {
    send_to_char("You are too exhausted to continue.\n", ch);
    send_to_char("You are no longer being &+Wbandaged&n.\n", victim);
    return;
  }

  if( !notch_skill(ch, SKILL_BANDAGE, get_property("skill.notch.bandage", 3)) &&
      !number(number(0,3), (GET_CHAR_SKILL(ch, SKILL_BANDAGE) ) ))
  {
    send_to_char("You are not focused enough, you destroy the &+Wbandage&n.\n", ch);  
    send_to_char("You are no longer being &+Wbandaged&n.\n", victim);
    return; 
  }

  if(ch!= victim)
    act("You continue &+Wbandaging&n $N.", FALSE, ch, 0,victim, TO_CHAR);
  else
    act("You continue &+Wbandaging&n yourself.", FALSE, ch, 0, victim, TO_CHAR);

  act("$n continues to &+Wbandage&n you.", FALSE, ch, 0,
      victim, TO_VICT);
  act("$n continues to &+Wbandage&n $N", FALSE, ch, 0,
      victim, TO_NOTVICT);

  healed = (mdata->maxheal / 20 + GET_CHAR_SKILL(ch, SKILL_BANDAGE) / 7);
  healed = MIN(healed, mdata->maxheal - mdata->healed);
  mdata->healed = mdata->healed + healed;
  healed = vamp(victim, healed, GET_MAX_HIT(victim) * 0.875);
  update_pos(victim);

  if( IS_AFFECTED2(victim, AFF2_POISONED) && 
      !number(0, 4) &&
      ( number(0,100) < GET_CHAR_SKILL(ch, SKILL_BANDAGE)) )
  {
    send_to_char("&+WThe poison in your bloodstream is neutralized!\n", victim);
    poison_common_remove(victim);
  }

  if (healed == 0)
  {
    send_to_char("&+WYour bandage won't do any more good now&n.\n", ch); 
    send_to_char("You are no longer being bandaged.\n", victim);
    return; 
  }

  GET_VITALITY(ch) -= (number(0,100) > GET_CHAR_SKILL(ch, SKILL_BANDAGE)) ? 3 : 2;
  add_event(event_bandage_check, PULSE_VIOLENCE, ch, victim, 0, 0, mdata, sizeof(struct bandage_data));
}

// This is just for God mine commands (and a stub).
//  For actual mining stuff see event_mine_check and 'int mine(..)' in this file.
void do_mine(P_char ch, char *arg, int cmd)
{

  if( !ch || IS_NPC(ch) )
    return;

  // Anyone wana take a crack at this below to make it work correctly?
  // If you don't get the idea, give me a hollar.
  // From hearing Torgal's responses to it as well as knowing nobody ever uses
  // this command, i'm going to go ahead and get the engine in game. -Venthix
  if( GET_CHAR_SKILL(ch, SKILL_MINE) <= 1 )
  {
    send_to_char("&+LYou dont know how to mine.\r\n", ch);
    return;
  }

  int i;
  char buff[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  half_chop(arg, arg1, arg2);
  //one_argument(arg, buff);

  //debug("(arg) %s, (arg1) %s, (arg2) %s", arg, arg1, arg2);

  if( !strcmp(arg1, "reset") && IS_TRUSTED(ch) )
  {
    for (i = 0; mine_data[i].start; i++)
    {
      if (isname(arg2, mine_data[i].abbrev))
      {
        sprintf(buf2, "purge %s", mine_data[i].abbrev);
        do_mine(ch, buf2, CMD_MINE);
        wizlog(56, "%s loaded mines in %s", GET_NAME(ch), mine_data[i].name);
        logit(LOG_WIZ, "%s loaded mines in %s", GET_NAME(ch), mine_data[i].name);
        load_mines(FALSE, TRUE, i);
        return;
      }
    }
    sprintf(buf2, "Available options for mine reset: map | ud\n");
    /*
      for (i = 0; mine_data[i].start; i++);
      {
      strcat(buf2, (mine_data[i].abbrev));
      if (mine_data[i+1].abbrev)
        strcat(buf2, " | ");
    }
    strcat(buf2, "\n");
    */
    send_to_char(buf2, ch);
  }
  else if( !strcmp(arg1, "load") && IS_TRUSTED(ch) )
  {
    for (i = 0; mine_data[i].start; i++)
    {
      if (!strcmp(arg, mine_data[i].abbrev))
      {
        wizlog(56, "%s loaded mine in %s", GET_NAME(ch), mine_data[i].name);
        logit(LOG_WIZ, "%s loaded mine in %s", GET_NAME(ch), mine_data[i].name);
        load_one_mine(i);
        return;
      }
    }
    sprintf(buf2, "Available options for mine load: map | ud\n");
    /*for (i = 0; mine_data[i].abbrev; i++);
    {
      debug("%s", mine_data[i].abbrev);
      strcat(buf2,  mine_data[i].abbrev);
      if (mine_data[i+1].abbrev)
        strcat(buf2, " | ");
    }
    strcat(buf2, "\n");
    */
    send_to_char(buf2, ch);
  }
  else if( !strcmp(buff, "purge") && IS_TRUSTED(ch) )
  {
    P_obj tobj = object_list;
    P_obj next = object_list->next;

    for (i = 0; mine_data[i].start; i++)
    {
      if (!strcmp(arg, mine_data[i].abbrev))
        break;
    }

    for ( ; tobj && next; tobj = next )
    {
      next = tobj->next;

      if( (OBJ_VNUM(tobj) == VOBJ_MINE)
        && (!strcmp(arg, mine_data[i].abbrev))
        && (world[tobj->loc.room].number >= mine_data[i].start)
        && (world[tobj->loc.room].number <= mine_data[i].end) )
      {
        extract_obj(tobj, TRUE); // Not an arti, but 'in game.'
      }
      // The all factor
      else if ( OBJ_VNUM(tobj) == VOBJ_MINE && (!strcmp(arg, "all")) )
      {
        extract_obj(tobj, TRUE);
      }
    }
    if (!strcmp(arg, "all"))
    {
      wizlog(56, "%s purged all mines.", GET_NAME(ch));
      logit(LOG_WIZ, "%s purged all mines.", GET_NAME(ch));
      return;
    }
    else if (!strcmp(arg, mine_data[i].abbrev))
    {
      wizlog(56, "%s purged %s mines.", GET_NAME(ch), mine_data[i].name);
      logit(LOG_WIZ, "%s purged %s mines.", GET_NAME(ch), mine_data[i].name);
      return;
    }
    else
    {
      sprintf(buf2, "Available options for mine purge: all | map | tharnrift\n");
      /*
      for (i = 0; mine_data[i].start; i++);
      {
        strcat(buf2, mine_data[i].abbrev);
        if (mine_data[i+1].abbrev)
          strcat(buf2, " | ");
      }
      strcat(buf2, " | all\n");
      */
      send_to_char(buf2, ch);
    }
  }
  send_to_char("You can't mine here!\n", ch);
}


void do_bandage(P_char ch, char *arg, int cmd)
{
  struct affected_type af;
  char     name[MAX_INPUT_LENGTH];
  struct bandage_data data;
  char  buf1[MAX_STRING_LENGTH];
  P_char   victim = NULL;
  P_obj bandage;

  P_char   t_char = NULL;

  if (!SanityCheck(ch, "do_bandage"))
    return;

  if (IS_NPC(ch))
  {
    send_to_char("You're too NPC-like to try.\r\n", ch);
    return;
  }

  if (get_scheduled(ch, event_bandage_check))
  {
    send_to_char("Yes you are &+Wbandaging&n.\n", ch);
    return;
  }

  victim = ParseTarget(ch, arg);
  if (!victim)
  {
    send_to_char("Who are you trying to &+Wbandage&n?\r\n", ch);
    return;
  }

  if (affected_by_spell(victim, SKILL_BANDAGE) && GET_HIT(victim) >= 0)
  {
    send_to_char("&+WBandaging&n again wont do any good now.\r\n", ch);
    return;
  }

  if (IS_PC(ch) && GET_CHAR_SKILL(ch, SKILL_BANDAGE) == 0)
  {
    send_to_char
      ("Maybe you should leave that to someone skilled in battlefield first aid.\r\n",
       ch);
    return;
  }

  if( IS_FIGHTING(victim) || IS_DESTROYING(victim)
    || IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("The battle in room prevents you from that.\r\n", ch);
    return;
  }

  if (NumAttackers(victim))
  {
    send_to_char
      ("That person is still being attacked, maybe you should wait?\r\n", ch);
    return;
  }

  if (GET_HIT(victim) > GET_MAX_HIT(victim) - (GET_MAX_HIT(victim) / 8))
  {
    if(ch == victim)
      send_to_char("Try again when you are more hurt..\r\n", ch);
    else
      send_to_char("Try again when they more hurt.\r\n", ch);

    return;
  }

  bandage = get_bandage(ch);

  if(!bandage)
  {
    act("You have no &+Wbandage&n.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  data.maxheal = bandage->value[0];
  extract_obj(bandage, TRUE); // Not an arti, but 'in game.'

  if(ch!= victim)
    act("You attempt to &+Wbandage&n $N.", FALSE, ch, 0,victim, TO_CHAR);
  else
    act("You attempt to &+Wbandage&n yourself.", FALSE, ch, 0, victim, TO_CHAR);

  act("$n attempts to &+Wbandage&n you.", FALSE, ch, 0,
      victim, TO_VICT);
  act("$n attempts to &+Wbandage&n $N", FALSE, ch, 0,
      victim, TO_NOTVICT);

  data.room = ch->in_room;
  data.healed = 0;

  add_event(event_bandage_check, PULSE_VIOLENCE, ch, victim, 0, 0, &data, sizeof(struct bandage_data));

  bzero(&af, sizeof(af));
  af.duration = 3;
  af.type = SKILL_BANDAGE;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
  affect_to_char(victim, &af);

  CharWait(ch, 2 * PULSE_VIOLENCE);
  return;
}


//Drannak Stuff
static FILE *recipefile;

void create_recipes_file(const char *dir, char *name)
{
  char     buf[256], *buff;
  char     Gbuf1[MAX_STRING_LENGTH];
  FILE    *f;

  strcpy(buf, name);
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  sprintf(Gbuf1, "%s/%c/%s.crafting", dir, buf[0], buf);
  f = fopen(Gbuf1, "w");
  fclose(f);
}

void create_recipes_name(char *name)
{
  create_recipes_file("Players/Tradeskills", name);
}

int learn_recipe(P_obj obj, P_char ch, int cmd, char *arg)
{

  P_obj tobj;
  char     buf[256], *buff;
  char     Gbuf1[MAX_STRING_LENGTH], *c;
  FILE    *f;
  FILE    *recipelist;
  int recipenumber = obj->value[6];
  int recnum;
  bool forge = FALSE;
  bool craft = FALSE;
  P_char temp_ch;

  // If not reciting, bad char, or char doesn't have obj.
  if( cmd != CMD_RECITE || !IS_ALIVE(ch) || !(OBJ_WORN_BY(obj, ch) || OBJ_CARRIED_BY(obj, ch)) || !arg )
  {
    return FALSE;
  }

  // Trying to recite a different object.
  if( obj != get_obj_in_list_vis(ch, skip_spaces(arg), ch->carrying) && obj != get_object_in_equip_vis(ch, arg, &recnum) )
  {
    return FALSE;
  }

  if( recipenumber == 0 )
  {
    send_to_char("This item is useless!\r\n", ch);
    return TRUE;
  }

  tobj = read_object(recipenumber, VIRTUAL);

  if( IS_SET(tobj->wear_flags, ITEM_WEAR_HEAD)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_BODY)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_ARMS)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_FEET)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_SHIELD)
    || IS_SET(tobj->wear_flags, ITEM_WIELD)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_LEGS)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_HANDS) )
  {
    forge = TRUE;
  }
  if( IS_SET(tobj->wear_flags, ITEM_WEAR_ABOUT)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_WAIST)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_EARRING)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_NECK)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_WRIST)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_FINGER)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_EYES)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_QUIVER)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_TAIL)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_NOSE)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_HORN)
    || IS_SET(tobj->wear_flags, ITEM_WEAR_FACE) )
  {
    craft = TRUE;
  }

  if( forge )
  {
    if( GET_CHAR_SKILL(ch, SKILL_FORGE) < 1 )
    {
      forge = FALSE;
      if( craft )
      {
        if( GET_CHAR_SKILL(ch, SKILL_CRAFT) < 1 )
        {
  	      send_to_char("This recipe can only be used by someone with the &+rCraft&n or &+LForge&n skill.\r\n", ch);
	        return TRUE;
        }
      }
      else
      {
    	  send_to_char("This recipe can only be used by someone with the &+LForge&n skill.\r\n", ch);
    	  return TRUE;
      }
    }
  }
  else if( craft )
  {
    if( GET_CHAR_SKILL(ch, SKILL_CRAFT) < 1 )
    {
  	  send_to_char("This recipe can only be used by someone with the &+rCraft&n skill.\r\n", ch);
	    return TRUE;
    }
  }

  //Create buffers for name
  strcpy(buf, GET_NAME(ch));
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  //buf[0] snags first character of name
  sprintf(Gbuf1, "Players/Tradeskills/%c/%s.crafting", buf[0], buf);

  /*just a debug test
  send_to_char(Gbuf1, ch);*/

  //check if tradeskill file exists for player
  recipelist = fopen(Gbuf1, "rt");

  if (!recipelist)
  {
    send_to_char("As you examine the recipe, small &+mm&+Mag&+Wi&+Mca&+ml &+Wmists&+w begin to form...\r\n", ch);
    send_to_char("...without warning, a &+Ltome &+yof &+Ycraf&+ytsman&+Lship&n appears in your hands.\r\n", ch);
    create_recipes_name(GET_NAME(ch));
    recipelist = fopen(Gbuf1, "rt");
  }

  /* Check to see if recipe exists */
  while((fscanf(recipelist, "%i", &recnum)) != EOF )
	{
    if(recnum == recipenumber)
    {
      send_to_char("You already know how to create that item!&n\r\n", ch);
      extract_obj(tobj);
      return TRUE;
    }
  }

  fclose(recipelist);
  recipefile = fopen(Gbuf1, "a");
  fprintf(recipefile, "%d\n", recipenumber);
  act("$n opens their &+Ltome &+yof &+Ycraf&+ytsman&+Lship&n and begins scribing the &+yrecipe&n...\n"
    "As they finish the last entry of the &+yrecipe&n, a &+Mbri&+mgh&+Wt &nflash of &+Clight&n appears,\n"
    "quickly consuming $p, which vanishes from sight.\r\n", FALSE, ch, obj, 0, TO_ROOM);
  act("You open your &+Ltome &+yof &+Ycraf&+ytsman&+Lship&n and begin scribing the recipe...\n"
    "As you finish the last entry of the &+yrecipe&n, a &+Mbri&+mgh&+Wt &nflash of &+Clight&n appears,\n"
    "quickly consuming $p, which vanishes from sight.\r\n", FALSE, ch, obj, 0, TO_CHAR);   
  fclose(recipefile);

  extract_obj(obj, TRUE); // Not an arti, but 'in game.'

  if( forge )
  {
    notch_skill(ch, SKILL_FORGE, 1);
  }
  else if( craft )
  {
    notch_skill(ch, SKILL_CRAFT, 1);
  }
  else
  {
    // This should never happen.
    debug("learn_recipe: %s learned %s (%d) without craft or forge!?", J_NAME(ch), tobj->short_description, recipenumber );
    logit(LOG_DEBUG, "learn_recipe: %s learned %s (%d) without craft or forge!?", J_NAME(ch), tobj->short_description, recipenumber );
  }
  extract_obj(tobj);
  return TRUE;
}

/* This function handles buying epic items and attaches to
 *   the mob (shopkeeper) in specs_assign.c.
 * Recommended change: Variables for item vnum and epics needed
 *   and then just set them in the CMD_BUY case.  At the end,
 *   if( pl->..epics < epics needed ) {fail message and return}
 *   load item via var, message char "You bought $p", subtract epics,
 *   send item to char, and save char.
 */
#define COST_EPIC_MUSHROOM           225
#define COST_EPIC_FIX_SCROLL         105
#define COST_EPIC_FAERIE_BAG         100
#define COST_EPIC_BATTLEROBE        7500
#define COST_EPIC_TOCORPSE_POTION    500
#define COST_EPIC_LANTAN_TOOLS       500
#define COST_EPIC_FOREST_EYES       4500
#define COST_EPIC_BOTTLE_EPICS       100

int epic_store(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buffer[MAX_STRING_LENGTH];
  char     buf[256], *buff;
  char     Gbuf1[MAX_STRING_LENGTH], *c;
  P_obj    obj;

  if( cmd == CMD_LIST )
  {
    if( !arg || !*arg )
    {
      // list called with no arguments
      sprintf(buffer,
        "&+WKannard&+L slowly lifts his hood and smiles.'\n"
	      "&+WKannard&+L &+wsays 'Welcome adventurer. I offer exotic items from the far reaches beyond our own realm in exchange for &+cepic points&n.'\n"
	      "&+WKannard&+L &+wsays 'Please &+Yrefer to my &-L&+ysign&n&-l for an explanation of each of these items and their affects.'\n"
        "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"
        "&+y|		&+cItem Name					           Epic Cost            &+y|\n"
        "&+y|&+W 1) &+ga M&+Ga&+Wg&+Gi&+gc&+Ga&+Wl &+GGreen &+gMu&+Gshro&+gom from the &+GSylvan &+yWoods&n&+C%30d&n		&+y|\n"
        "&+y|&+W 2) &+ya tightly wrapped vellum scroll named '&+LFix&+y'&n   &+C%30d&n		&+y|\n"
        "&+y|&+W 3) &+Wa &+mm&+My&+Ys&+Bt&+Gc&+Ra&+Gl &+MFaerie &+Wbag of &+Lstolen loot&n           &+C%30d&n               &+y|\n"
        "&+y|&+W 4) &+Ya r&+ro&+Yb&+re &+Yof a &+mN&+We&+Mt&+Wh&+me&+Wr&+Mi&+Wl &+rBa&+Ytt&+rle&+Y M&+rag&+Ye&n              &+C%30d&n               &+y|\n"
        "&+y|&+W 5) &+La &+Gbottle &+Lof &+GT&+go&+GR&+gM&+Ge&+gN&+GT&+ge&+GD &+gS&+Goul&+gs     &n              &+C%30d&n               &+y|\n"
        "&+y|&+W 6) &+ca &+Cbr&+Will&+Bia&+Wnt &+cset of &+rLantan &+CScientific&+L Tools&n    &n&+C%30d&n               &+y|\n"
        "&+y|&+W 7) &+Lthe &+ge&+Gy&+ge&+Gs &+Lof the &+gHi&+Ggh For&+gest&n     &n              &+C%30d&n&+y               |\n"
        "&+y|&+W 8) &+Ca &+Wbottle &+Cof &+GPa&+gs&+Lt Exp&+gerien&+Gces&n     &n             &+C%30d&n&+y               |\n"
        "&+y|&+R Notice: You can not buy multiple items at once here!                                          &+y|&n\n"
        "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"
        "\n", COST_EPIC_MUSHROOM, COST_EPIC_FIX_SCROLL, COST_EPIC_FAERIE_BAG, COST_EPIC_BATTLEROBE,
        COST_EPIC_TOCORPSE_POTION, COST_EPIC_LANTAN_TOOLS, COST_EPIC_FOREST_EYES, COST_EPIC_BOTTLE_EPICS );

      send_to_char(buffer, pl);
      return TRUE;
    }
  }
  else if( cmd == CMD_BUY )
  {
    if( !arg || !*arg )
    {
      sprintf(buffer, "&+WKannard&+L &+wsays 'What item would you like to buy?'\n");
      send_to_char(buffer, pl);
      return TRUE;
    }
    // 1 - lvl mushroom
    else if( strstr(arg, "1") )
    {
      // Check for the epics required to purchase mushroom..
      if( pl->only.pc->epics < COST_EPIC_MUSHROOM )
      {
        send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
        return TRUE;
      }
      pl->only.pc->epics -= COST_EPIC_MUSHROOM;
      obj = read_object(VOBJ_EPIC_MUSHROOM, VIRTUAL);
      send_to_char("&+WKannard&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
      send_to_char("&+WKannard &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      obj_to_char(obj, pl);
      do_save_silent(pl, 1);
      // Log the transaction in epic log file.
      epiclog( 56, "%s bought '%s' (%d) at the epic store for %d epics.", J_NAME(pl), obj->short_description,
        VOBJ_EPIC_MUSHROOM, COST_EPIC_MUSHROOM );
      return TRUE;
    }
    // 2 - fix scroll
    else if(strstr(arg, "2"))
    {
      // Check for the epics required for scroll..
      if( pl->only.pc->epics < COST_EPIC_FIX_SCROLL )
      {
        send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
        return TRUE;
      }
      pl->only.pc->epics -= COST_EPIC_FIX_SCROLL;
      obj = read_object(VOBJ_EPIC_FIX_SCROLL, VIRTUAL);
      send_to_char("&+WKannard&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
      send_to_char("&+WKannard &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      obj_to_char( obj, pl );
      do_save_silent(pl, 1);
      // Log the transaction in epic log file.
      epiclog( 56, "%s bought '%s' (%d) at the epic store for %d epics.", J_NAME(pl), obj->short_description,
        VOBJ_EPIC_FIX_SCROLL, COST_EPIC_FIX_SCROLL );
      return TRUE;
    }
    // 3 - faerie bag
    else if(strstr(arg, "3"))
    {
      // Check for the epics required for bag..
      if( pl->only.pc->epics < COST_EPIC_FAERIE_BAG )
      {
        send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
        return TRUE;
      }
      pl->only.pc->epics -= COST_EPIC_FAERIE_BAG;
      obj = read_object(VOBJ_EPIC_FAERIE_BAG, VIRTUAL);
      send_to_char("&+WKannard&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
      send_to_char("&+WKannard &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      obj_to_char(obj, pl);
      do_save_silent(pl, 1);
      // Log the transaction in epic log file.
      epiclog( 56, "%s bought '%s' (%d) at the epic store for %d epics.", J_NAME(pl), obj->short_description,
        VOBJ_EPIC_FAERIE_BAG, COST_EPIC_FAERIE_BAG );
      return TRUE;
    }
    // 4 - netheril robe
    else if(strstr(arg, "4"))
    {
      // Check for the epics required for robe..
      if( pl->only.pc->epics < COST_EPIC_BATTLEROBE )
      {
        send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
        return TRUE;
      }
      pl->only.pc->epics -= COST_EPIC_BATTLEROBE;
      obj = read_object( VOBJ_EPIC_BATTLEROBE, VIRTUAL);
      send_to_char("&+WKannard&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
      send_to_char("&+WKannard &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      obj_to_char( obj, pl );
      do_save_silent(pl, 1);
      // Log the transaction in epic log file.
      epiclog( 56, "%s bought '%s' (%d) at the epic store for %d epics.", J_NAME(pl), obj->short_description,
        VOBJ_EPIC_BATTLEROBE, COST_EPIC_BATTLEROBE );
      return TRUE;
    }
    // 5 - corpse portal potion
    else if(strstr(arg, "5"))
    {
      // Check for the epics required for potion..
      if( pl->only.pc->epics < COST_EPIC_TOCORPSE_POTION )
      {
        send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
        return TRUE;
      }
      obj = read_object( VOBJ_EPIC_TOCORPSE_POTION, VIRTUAL );
      pl->only.pc->epics -= COST_EPIC_TOCORPSE_POTION;
      send_to_char("&+WKannard&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
      send_to_char("&+WKannard &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      obj_to_char( obj, pl );
      do_save_silent(pl, 1);
      // Log the transaction in epic log file.
      epiclog( 56, "%s bought '%s' (%d) at the epic store for %d epics.", J_NAME(pl), obj->short_description,
        VOBJ_EPIC_TOCORPSE_POTION, COST_EPIC_TOCORPSE_POTION );
      return TRUE;
    }
    // 6 - lantan tools
    else if(strstr(arg, "6"))
    {
      // Gellz 08/02/2015 - Removed Lantan for now.
      send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but due to such a high demand, those items seem to be temporarily out of stock.\r\n&n", pl);
      return TRUE;
      // Check for the epics required for lantans..
      if( pl->only.pc->epics < COST_EPIC_LANTAN_TOOLS )
      {
        send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
        return TRUE;
      }
      obj = read_object( VOBJ_EPIC_LANTAN_TOOLS, VIRTUAL );
      pl->only.pc->epics -= COST_EPIC_LANTAN_TOOLS;
      send_to_char("&+WKannard&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
      send_to_char("&+WKannard &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      obj_to_char( obj, pl );
      do_save_silent(pl, 1);
      // Log the transaction in epic log file.
      epiclog( 56, "%s bought '%s' (%d) at the epic store for %d epics.", J_NAME(pl), obj->short_description,
        VOBJ_EPIC_LANTAN_TOOLS, COST_EPIC_LANTAN_TOOLS);
      return TRUE;
    }
    // 7 - forest sight
    else if(strstr(arg, "7"))
    {
      // Check for the epics required for forest goggles..
      if( pl->only.pc->epics < COST_EPIC_FOREST_EYES )
      {
        send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
        return TRUE;
      }
      obj = read_object( VOBJ_EPIC_FOREST_EYES, VIRTUAL );
      pl->only.pc->epics -= COST_EPIC_FOREST_EYES;
      send_to_char("&+WKannard&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
      send_to_char("&+WKannard &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      // Why isn't this set in the zone files?  How peculiar.
      SET_BIT(obj->bitvector5, AFF5_FOREST_SIGHT);
      obj_to_char( obj, pl );
      do_save_silent(pl, 1);
      // Log the transaction in epic log file.
      epiclog( 56, "%s bought '%s' (%d) at the epic store for %d epics.", J_NAME(pl), obj->short_description,
        VOBJ_EPIC_FOREST_EYES, COST_EPIC_FOREST_EYES );
      return TRUE;
    }
	else if(strstr(arg, "8"))
    {
      // Check for the epics required for forest goggles..
      if( pl->only.pc->epics < COST_EPIC_BOTTLE_EPICS )
      {
        send_to_char("&+WKannard&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
        return TRUE;
      }
      obj = read_object( VOBJ_EPIC_BOTTLE_EPICS, VIRTUAL );
      pl->only.pc->epics -= COST_EPIC_BOTTLE_EPICS;
      send_to_char("&+WKannard&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
      send_to_char("&+WKannard &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      obj_to_char( obj, pl );
      do_save_silent(pl, 1);
      // Log the transaction in epic log file.
      epiclog( 56, "%s bought '%s' (%d) at the epic store for %d epics.", J_NAME(pl), obj->short_description,
        VOBJ_EPIC_BOTTLE_EPICS, COST_EPIC_BOTTLE_EPICS );
      return TRUE;
    }
  }
  return FALSE;
}

int learn_tradeskill(P_char ch, P_char pl, int cmd, char *arg)
{
  char buffer[MAX_STRING_LENGTH];
  char     buf[256], *buff;
  char     Gbuf1[MAX_STRING_LENGTH], *c;
  FILE    *f;
  FILE    *recipelist;


  if(cmd == CMD_PRACTICE)
  {
    
       
        if(!arg || !*arg)
    {
      // practice called with no arguments
      sprintf(buffer,
              "'Greetings Adventurer!'\n"
              "'The choice of a &+Wtradeskill&n is an important one, as only &+Yone&n can be made.'\n"
		"'Of the abilities which I can train you are &+Lm&+wi&+Wn&+wi&+Lng&n, &+Lforging&n, or &+rcrafting&n.'\n"
		"'&+LM&+wi&+Wn&+wi&+Lng&n&n will allow you to gather raw &+mvaluable&n &+Lore&n from &+ymines&n scattered throughout the realm.'\n"
		"'&+LForging&n allows one to create amazing &+Larmor&n and &+Wweapons&n from &+yrecipes&n found through the &+ysalvage&n skill.'\n"
		"'&+rCrafting&n is the meticulous skill allowing one to craft &+Mjewelry&n, &+maccessories&n, and &+ccloak&n items from &+ysalvaged&n recipes.'\n"
              "'Please think long and hard about your decision, and then &+RPractice &neither &+Lforge&n, &+ymine&n, or &+Rcraft&n.\n"
		"'\n"
		"'If you are unhappy with your current &+ytradeskill&n, you can &+Rpractice reset&n and I will unlearn any &+ytradeskill&n you'\n"
		"'might know for the low price of &+c200 &+Cepic points&n. &+rBEWARE&n: this will wipe any learned &+Yrecipes&n!\n");
      send_to_char(buffer, pl);


         if(GET_CHAR_SKILL(pl, SKILL_FORGE) >= 1 || GET_CHAR_SKILL(pl, SKILL_MINE) >= 1 || GET_CHAR_SKILL(pl, SKILL_CRAFT) >= 1 )
      {
        send_to_char("\n"
		 "'Unfortunately, I cannot teach you anything more, as you have already learned a tradeskill!'\n", pl);
        return TRUE;
      }
      
      return TRUE;
    }
    //Drannak - ability to reset their tradeskill 3/22/13
    else if(strstr(arg, "reset"))
    {
	//reset called

       //check for 200 epics required to reset
	int availepics = pl->only.pc->epics;
	if (availepics < 200)
	{
	  if(pl->in_room == real_room(133071))
	  send_to_char("&nJodnan says '&+GI'm sorry, but you do not seem to have the &+Wepics&+G available for me to reset your &+gtradeskill&+G.\r\n&n", pl);
	  else
	  send_to_char("&nGixnif says '&+GI'm sorry, but you do not seem to have the &+Wepics&+G available for me to reset your &+gtradeskill&+G.\r\n&n", pl);
	  return TRUE;
        }
	//subtract 200 epics
	pl->only.pc->epics -= 200;

       //wipe their learned recipes
       create_recipes_name(GET_NAME(pl));

        strcpy(buf, GET_NAME(pl));
     	 buff = buf;
 	 for (; *buff; buff++)
  	 *buff = LOWER(*buff);
  	 sprintf(Gbuf1, "Players/Tradeskills/%c/%s.crafting", buf[0], buf);
        recipelist = fopen(Gbuf1, "w");
        fclose(recipelist);

      sprintf(buffer, "Your teacher takes you aside, and performs a cleansing geasture about your body&n. Your mind feels &+Wrenewed&n!\n");
      act(buffer, FALSE, ch, 0, pl, TO_VICT);
      pl->only.pc->skills[SKILL_FORGE].taught = 0;
        pl->only.pc->skills[SKILL_FORGE].learned = 0;
      pl->only.pc->skills[SKILL_MINE].taught = 0;
        pl->only.pc->skills[SKILL_MINE].learned = 0;
      pl->only.pc->skills[SKILL_CRAFT].taught = 0;
        pl->only.pc->skills[SKILL_CRAFT].learned = 0;

      do_save_silent(pl, 1); // tradeskills require a save.
      CharWait(pl, PULSE_VIOLENCE);

       
      return TRUE;


    }


    else if(strstr(arg, "forge"))
    {
      // called with skill name
        if(GET_CHAR_SKILL(pl, SKILL_FORGE) >= 1 || GET_CHAR_SKILL(pl, SKILL_MINE) >= 1 || GET_CHAR_SKILL(pl, SKILL_CRAFT) >= 1 )
      {
        send_to_char("Unfortunately, I cannot teach you anything more, as you have already learned a tradeskill!\n", pl);
        return TRUE;
      }
      sprintf(buffer, "Your teacher takes you aside and teaches you the finer points of &+W%s&n.\n"
                      "&+cYou feel your skill in %s improving.&n\n",
              skills[SKILL_FORGE].name, skills[SKILL_FORGE].name);
      act(buffer, FALSE, ch, 0, pl, TO_VICT);
        pl->only.pc->skills[SKILL_FORGE].learned = 10;
      pl->only.pc->skills[SKILL_FORGE].taught = 100;
        pl->only.pc->skills[SKILL_FORGE].learned = 10;
        //MIN(100, pl->only.pc->skills[SKILL_FORGE].learned + 10);
      do_save_silent(pl, 1); // tradeskills require a save.
      CharWait(pl, PULSE_VIOLENCE);
      return TRUE;
    }
	else if(strstr(arg, skills[SKILL_MINE].name))
    {
      // called with skill name
        if(GET_CHAR_SKILL(pl, SKILL_FORGE) >= 1 || GET_CHAR_SKILL(pl, SKILL_MINE) >= 1 || GET_CHAR_SKILL(pl, SKILL_CRAFT) >= 1 )
      {
        send_to_char("Unfortunately, I cannot teach you anything more, you have already learned a tradeskill!\n", pl);
        return TRUE;
      }
      sprintf(buffer, "Your teacher takes you aside and teaches you the finer points of &+W%s&n.\n"
                      "&+cYou feel your skill in %s improving.&n\n",
              skills[SKILL_MINE].name, skills[SKILL_MINE].name);
      act(buffer, FALSE, ch, 0, pl, TO_VICT);
      pl->only.pc->skills[SKILL_MINE].taught = 100;
        pl->only.pc->skills[SKILL_MINE].learned = 10;
        //MIN(100, pl->only.pc->skills[SKILL_MINE].learned + 10);
      do_save_silent(pl, 1); // tradeskills require a save.
      CharWait(pl, PULSE_VIOLENCE);
      return TRUE;
    }
	else if(strstr(arg, skills[SKILL_CRAFT].name))
    {
      // called with skill name
        if(GET_CHAR_SKILL(pl, SKILL_FORGE) >= 1 || GET_CHAR_SKILL(pl, SKILL_MINE) >= 1 || GET_CHAR_SKILL(pl, SKILL_CRAFT) >= 1 )
      {
        send_to_char("Unfortunately, I cannot teach you anything more, you have already learned a tradeskill!\n", pl);
        return TRUE;
      }
      sprintf(buffer, "Your teacher takes you aside and teaches you the finer points of &+W%s&n.\n"
                      "&+cYou feel your skill in %s improving.&n\n",
              skills[SKILL_CRAFT].name, skills[SKILL_CRAFT].name);
      act(buffer, FALSE, ch, 0, pl, TO_VICT);
      pl->only.pc->skills[SKILL_CRAFT].taught = 100;
        pl->only.pc->skills[SKILL_CRAFT].learned = 10;
       // MIN(100, pl->only.pc->skills[SKILL_CRAFT].learned + 10);
      do_save_silent(pl, 1); // tradeskills require a save.
      CharWait(pl, PULSE_VIOLENCE);
      return TRUE;
    }
	
  }
  return FALSE;
}


int itemvalue( P_obj obj )
{
  long workingvalue = 0;
  double multiplier = 1;
  double mod;

  if( !obj )
  {
    return 0;
  }

  if(IS_SET(obj->wear_flags, ITEM_WEAR_EYES))
    multiplier *= 1.3;

  if(IS_SET(obj->wear_flags, ITEM_WEAR_EARRING))
    multiplier *= 1.2;

  if (IS_SET(obj->wear_flags, ITEM_WEAR_FACE))
    multiplier *= 1.3;

  if (IS_SET(obj->wear_flags, ITEM_WEAR_QUIVER))
    multiplier *= 1.1;

  if (IS_SET(obj->wear_flags, ITEM_WEAR_FINGER))
    multiplier *= 1.2;

  if (IS_SET(obj->wear_flags, ITEM_GUILD_INSIGNIA))
    multiplier *= 1.5;

  if (IS_SET(obj->wear_flags, ITEM_WEAR_NECK))
    multiplier *= 1.2;

  if (IS_SET(obj->wear_flags, ITEM_WEAR_WAIST))
    multiplier *= 1.1;

  if (IS_SET(obj->wear_flags, ITEM_WEAR_WRIST))
    multiplier *= 1.1;

  // Aff's add to the base value.
  if (IS_SET(obj->bitvector, AFF_STONE_SKIN))
	  workingvalue += 125;

  if (IS_SET(obj->bitvector, AFF_BIOFEEDBACK))
	  workingvalue += 110;

  if (IS_SET(obj->bitvector, AFF_FARSEE))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector, AFF_DETECT_INVISIBLE))
	  workingvalue += 90;

  if (IS_SET(obj->bitvector, AFF_HASTE))
	  workingvalue += 75;

  if (IS_SET(obj->bitvector, AFF_INVISIBLE))
	  workingvalue += 35;

  if (IS_SET(obj->bitvector, AFF_SENSE_LIFE))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector, AFF_MINOR_GLOBE))
	  workingvalue += 28;

  if (IS_SET(obj->bitvector, AFF_UD_VISION))
	  workingvalue += 40;

  if (IS_SET(obj->bitvector, AFF_WATERBREATH))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector, AFF_PROTECT_EVIL))
	  workingvalue += 35;

  if (IS_SET(obj->bitvector, AFF_SLOW_POISON))
	  workingvalue += 20;

  if (IS_SET(obj->bitvector, AFF_SNEAK))
	  workingvalue += 125;

  if (IS_SET(obj->bitvector, AFF_BARKSKIN))
	  workingvalue += 25;

  if (IS_SET(obj->bitvector, AFF_INFRAVISION))
	  workingvalue += 7;

  if (IS_SET(obj->bitvector, AFF_LEVITATE))
	  workingvalue += 13;

  if (IS_SET(obj->bitvector, AFF_HIDE))
	  workingvalue += 85;

  if (IS_SET(obj->bitvector, AFF_FLY))
	  workingvalue += 75;

  if (IS_SET(obj->bitvector, AFF_AWARE))
	  workingvalue += 75;

  if (IS_SET(obj->bitvector, AFF_PROT_FIRE))
	  workingvalue += 20;

  if (IS_SET(obj->bitvector2, AFF2_FIRESHIELD))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector2, AFF2_ULTRAVISION))
	  workingvalue += 80;

  if (IS_SET(obj->bitvector2, AFF2_DETECT_EVIL))
	  workingvalue += 5;

  if (IS_SET(obj->bitvector2, AFF2_DETECT_GOOD))
	  workingvalue += 5;

  if (IS_SET(obj->bitvector2, AFF2_DETECT_MAGIC))
	  workingvalue += 10;

  if (IS_SET(obj->bitvector2, AFF2_PROT_COLD))
	  workingvalue += 20;

  if (IS_SET(obj->bitvector2, AFF2_PROT_LIGHTNING))
    workingvalue += 30;

  if (IS_SET(obj->bitvector2, AFF2_GLOBE))
	  workingvalue += 75;

  if (IS_SET(obj->bitvector2, AFF2_PROT_GAS))
    workingvalue += 30;

  if (IS_SET(obj->bitvector2, AFF2_PROT_ACID))
    workingvalue += 30;

  if (IS_SET(obj->bitvector2, AFF2_SOULSHIELD))
    workingvalue += 45;

  if (IS_SET(obj->bitvector2, AFF2_MINOR_INVIS))
	  workingvalue += 15;

  if (IS_SET(obj->bitvector2, AFF2_VAMPIRIC_TOUCH))
	  workingvalue += 65;

  if (IS_SET(obj->bitvector2, AFF2_EARTH_AURA))
	  workingvalue += 110;

  if (IS_SET(obj->bitvector2, AFF2_WATER_AURA))
	  workingvalue += 115;

  if (IS_SET(obj->bitvector2, AFF2_FIRE_AURA))
	  workingvalue += 120;

  if (IS_SET(obj->bitvector2, AFF2_AIR_AURA))
	  workingvalue += 130;

  if (IS_SET(obj->bitvector2, AFF2_PASSDOOR))
	  workingvalue += 80;

  if (IS_SET(obj->bitvector2, AFF2_FLURRY))
	  workingvalue += 150;

  if (IS_SET(obj->bitvector3, AFF3_PROT_ANIMAL))
	  workingvalue += 20;

  if (IS_SET(obj->bitvector3, AFF3_SPIRIT_WARD))
	  workingvalue += 35;

  if (IS_SET(obj->bitvector3, AFF3_GR_SPIRIT_WARD))
	  workingvalue += 55;

  if (IS_SET(obj->bitvector3, AFF3_ENLARGE))
	  workingvalue += 120;

  if (IS_SET(obj->bitvector3, AFF3_REDUCE))
	  workingvalue += 120;

  if (IS_SET(obj->bitvector3, AFF3_INERTIAL_BARRIER))
	  workingvalue += 135;

  if (IS_SET(obj->bitvector3, AFF3_COLDSHIELD))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector3, AFF3_TOWER_IRON_WILL))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector3, AFF3_BLUR))
	  workingvalue += 65;

  if (IS_SET(obj->bitvector3, AFF3_PASS_WITHOUT_TRACE))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector4, AFF4_VAMPIRE_FORM))
	  workingvalue += 90;

  if (IS_SET(obj->bitvector4, AFF4_HOLY_SACRIFICE))
	  workingvalue += 105;

  if (IS_SET(obj->bitvector4, AFF4_BATTLE_ECSTASY))
	  workingvalue += 105;

  if (IS_SET(obj->bitvector4, AFF4_DAZZLER))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector4, AFF4_PHANTASMAL_FORM))
	  workingvalue += 105;

  if (IS_SET(obj->bitvector4, AFF4_NOFEAR))
	  workingvalue += 40;

  if (IS_SET(obj->bitvector4, AFF4_REGENERATION))
	  workingvalue += 60;

  if (IS_SET(obj->bitvector4, AFF4_GLOBE_OF_DARKNESS))
	  workingvalue += 15;

  if (IS_SET(obj->bitvector4, AFF4_HAWKVISION))
	  workingvalue += 20;

  if (IS_SET(obj->bitvector4, AFF4_SANCTUARY))
	  workingvalue += 105;

  if (IS_SET(obj->bitvector4, AFF4_HELLFIRE))
	  workingvalue += 110;

  if (IS_SET(obj->bitvector4, AFF4_SENSE_HOLINESS))
	  workingvalue += 15;

  if (IS_SET(obj->bitvector4, AFF4_PROT_LIVING))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector4, AFF4_DETECT_ILLUSION))
	  workingvalue += 40;

  if (IS_SET(obj->bitvector4, AFF4_ICE_AURA))
	  workingvalue += 90;

  if (IS_SET(obj->bitvector4, AFF4_NEG_SHIELD))
	  workingvalue += 45;

  if (IS_SET(obj->bitvector4, AFF4_WILDMAGIC))
	  workingvalue += 240;

  // Has a old school proc (Up to three spells).
  // Can un-comment the debug stuff if you want to modify this.
  if(IS_SET(obj->wear_flags, ITEM_WIELD) && (obj->value[5] > 0))
  {
    int spells[3];
    int spellcirclesum, numspells;

    // val5 : 3 spells + all or one.
    spells[0] = obj->value[5] % 1000;
    spells[1] = obj->value[5] % 1000000 / 1000;
    spells[2] = obj->value[5] % 1000000000 / 1000000;
//    debug( "Spells0: %d, Spells1: %d, Spells2: %d.", spells[0], spells[1], spells[2] );

    // val6 = level * val7 = chance -> 1/30 chance = 1, 1/60 chance = .5, 1/15 chance = 2, etc.
    mod = ((obj->value[6] > 19) ? obj->value[6] / 10.0 : 1) * ( 30.0 / obj->value[7] );
//    debug( "mod: %f, objval6/10: %f, 30/objval7: %f", mod, (obj->value[6] > 19) ? obj->value[6] / 10.0 : 1, (30.0 / obj->value[7]) );

    spellcirclesum = get_mincircle( spells[0] );
    spellcirclesum += get_mincircle( spells[1] );
    spellcirclesum += get_mincircle( spells[2] );
//    debug( "spellcirclesum: %d, circle0: %d, circle1: %d, circle2: %d.", spellcirclesum, get_mincircle(spells[0]), get_mincircle(spells[1]), get_mincircle(spells[2]) );

    // 1 lvl 10 spell  2nd circle 1/60 chance = 1*1* 2*.5 =  1
    // 1 lvl 60 spell  1st circle 1/30 chance = 1*6* 1*1  =  6
    // 1 lvl 40 spell  3rd circle 1/30 chance = 1*4* 3*1  = 12
    // 1 lvl 60 spell 12th circle 1/30 chance = 1*6*12*1  = 72 etc.
    // val5 / 1000000000 -> 1, otherwise casts all.
    if( obj->value[5] / 1000000000 )
    {
      // Add up number of spells
      numspells = ((spells[0]) ? 1 : 0)+((spells[1]) ? 1 : 0)+((spells[2]) ? 1 : 0);
      // If there are none?!, set to 1 anyway.
      numspells = numspells ? numspells : 1;
      // Compute average circle.
//      debug( "mod * spellcirclesum / numspells: %d.",(int) (mod * (spellcirclesum / numspells)) );
      workingvalue += (int) (mod * (spellcirclesum / numspells));
    }
    else
    {
//      debug( "spellcirclesum * mod: %d.", (int) (spellcirclesum * mod) );
      workingvalue += mod * spellcirclesum;
    }
  }

  // Real Obj procs
  if( obj_index[obj->R_num].func.obj )
  {
    workingvalue += get_ival_from_proc(obj_index[obj->R_num].func.obj);
  }

  //------- A0/A1/A2 -------------
  int i = 0;
  while( i < MAX_OBJ_AFFECT )
  {
    mod = obj->affected[i].modifier;
    //dam/hitroll are normal values
    if( (obj->affected[i].location == APPLY_DAMROLL)
	    || (obj->affected[i].location == APPLY_HITROLL) )
    {
      // 1:1, 2:2, 3:6, 4:12, 5:20, 6:30, 7:42, 8:56, 9:72, 10:90, 11: 110..
      workingvalue += (mod <= 2) ? mod : mod * (mod - 1);
      // Translates to 1:1, 2:2, 3:7, 4:15, 5:25, 6:37, 7:52, 8:70, 9:90, 10:112
      multiplier *= 1.25;
      // So a 5/5 item is essentially 40 * 1.25 * 1.25 = 62.5 (before adding other stats).
      // A 6/6 item (no other stats) is 93, a 2d2 6/6 sword would be 62 * 1.25^2 = 96, and 5d5 6/6 = 112.
    }

    // Regular stats can be high numbers - half them
    if( (obj->affected[i].location == APPLY_STR)
	    || (obj->affected[i].location == APPLY_DEX)
	    || (obj->affected[i].location == APPLY_INT)
	    || (obj->affected[i].location == APPLY_WIS)
	    || (obj->affected[i].location == APPLY_CON)
	    || (obj->affected[i].location == APPLY_AGI) )
    {
      // 1:2, 2:4, 3:6, 4:9, 5:16, 6:25, 7:36, 8:49, 9:64, 10:81, 11:100
      workingvalue += (mod <= 3) ? 2 * mod : (mod - 1) * (mod - 1);
    }

    // These are used a little less
	  if( (obj->affected[i].location == APPLY_POW)
	    || (obj->affected[i].location == APPLY_CHA)
	    || (obj->affected[i].location == APPLY_LUCK) )
    {
      // 1:2, 2:4, 3:6, 4:8, 5:10, 6:12, 7:15, 8:26, 9:39, 10:54, 11:71, 12:90, 13: 111
      workingvalue += (mod <= 6) ? 2 * mod : (mod - 2) * (mod - 2) - 10;
    }

    // Hitpoints.
    if( obj->affected[i].location == APPLY_HIT )
    {
      // 1 : 2, 4 : 8, 5 : 11, 10 : 29, 20 : 65, 30 : 101 (can't be crafted), 32 : 108 (can't be enhanced).
      workingvalue += (mod <= 4) ? 2 * mod : (18 * mod) / 5 - 7;
    }

    // Moves and mana are generally large #'s
    if( (obj->affected[i].location == APPLY_MOVE)
	    || (obj->affected[i].location == APPLY_MANA) )
    {
      // Right now, 25 : 25, 35 : 65, 44 : 101, 45 : 105 - not enhanceable.
      workingvalue += (mod <= 25) ? mod : 4 * mod - 75;
    }

    // Hit, move, mana, regen are generally large #'s, but we don't want above 9.
    if( (obj->affected[i].location == APPLY_HIT_REG)
	    || (obj->affected[i].location == APPLY_MOVE_REG)
	    || (obj->affected[i].location == APPLY_MANA_REG) )
    {
      // 1:1, 2:2, 3:3, 4:5, 5:8, 6:12, 7:16, 8:21, 9:27, 10:33
      // 11:40, 12:48, 13:56, 14:65, 15:75, 16:85, 17:96, 18:108
      workingvalue += (mod < 4) ? mod : (mod * mod) / 3;
    }

    // Racial attributes #'s - Do we still have these?
    if( (obj->affected[i].location == APPLY_AGI_RACE)
	    || (obj->affected[i].location == APPLY_STR_RACE)
	    || (obj->affected[i].location == APPLY_CON_RACE)
	    || (obj->affected[i].location == APPLY_INT_RACE)
	    || (obj->affected[i].location == APPLY_WIS_RACE)
	    || (obj->affected[i].location == APPLY_CHA_RACE)
	    || (obj->affected[i].location == APPLY_DEX_RACE) )
    {
      if( mod < 1 || mod > LAST_RACE )
      {
        debug( "itemvalue: obj '%s' %d has APPLY_..._RACE %d and bad modifier %d.",
          obj->short_description, OBJ_VNUM(obj), obj->affected[i].location, mod );
        workingvalue += 100;
      }
      else
      {
        switch( obj->affected[i].location )
        {
          // We're looking for the stat vs 100. 75->0pts, 100->50pts, 150->150pts, 200->250pts
          case APPLY_AGI_RACE:
            workingvalue += 2 * stat_factor[(int)mod].Agi - 150;
            break;
          case APPLY_STR_RACE:
            workingvalue += 2 * stat_factor[(int)mod].Str - 150;
            break;
          case APPLY_CON_RACE:
            workingvalue += 2 * stat_factor[(int)mod].Con - 150;
            break;
          case APPLY_INT_RACE:
            workingvalue += 2 * stat_factor[(int)mod].Int - 150;
            break;
          case APPLY_WIS_RACE:
            workingvalue += 2 * stat_factor[(int)mod].Wis - 150;
            break;
          case APPLY_CHA_RACE:
            workingvalue += 2 * stat_factor[(int)mod].Cha - 150;
            break;
          case APPLY_DEX_RACE:
            workingvalue += 2 * stat_factor[(int)mod].Dex - 150;
            break;
          // Should never be the case but..
          default:
            debug( "itemvalue: obj '%s' %d has 'bad' APPLY_..._RACE %d, modifier %d.",
              obj->short_description, OBJ_VNUM(obj), obj->affected[i].location, mod );
            workingvalue += 100;
            break;
        }
      }
    }

    // AC negative is good, not reducing itemvalue for items that make ac worse.
    if( (obj->affected[i].location == APPLY_AC) && mod != 0 )
    {
      // 1.5 points for each point of armor class.
      // 1 : 1, 2 : 3, 3 : 4, 5 : 7, ... 50 : 75, 67 : 100, 68 : 102 (!craft), 70 : 105 (!enhance).
      if( mod < 0 )
      {
        mod *= -1;
      }
      workingvalue += (3 * mod) / 2;
      // +10% at 50ac.
      multiplier += mod / 500.;
    }

    //saving throw values (good) are negative
    if( (obj->affected[i].location == APPLY_SAVING_PARA)
	    || (obj->affected[i].location == APPLY_SAVING_ROD)
	    || (obj->affected[i].location == APPLY_SAVING_FEAR)
	    || (obj->affected[i].location == APPLY_SAVING_BREATH)
	    || (obj->affected[i].location == APPLY_SAVING_SPELL) )
    {
      // -1:2, -2:8, -3:18, -4:32, -5:50, -6:72, -7:98, -8:128
      workingvalue += mod * mod * ((mod <= 0) ? 2 : -2);
    }

    //pulse is quite valuable and negative is good
    if( (obj->affected[i].location == APPLY_COMBAT_PULSE)
      || (obj->affected[i].location == APPLY_SPELL_PULSE) )
    {
      multiplier *= 2;
      workingvalue += mod * -75;
    }

    //max_stats double points
    if( (obj->affected[i].location == APPLY_STR_MAX)
      || (obj->affected[i].location == APPLY_DEX_MAX)
      || (obj->affected[i].location == APPLY_INT_MAX)
      || (obj->affected[i].location == APPLY_WIS_MAX)
      || (obj->affected[i].location == APPLY_CON_MAX)
      || (obj->affected[i].location == APPLY_CHA_MAX)
      || (obj->affected[i].location == APPLY_AGI_MAX)
      || (obj->affected[i].location == APPLY_POW_MAX)
      || (obj->affected[i].location == APPLY_LUCK_MAX) )
    {
      // 1:2, 2:6, 3:14, 4:25, 5:39, 6:56, 7:77, 8:100
      workingvalue += (mod < 2) ? 2 * mod : 1.5625 * mod * mod;
    }
    i++;
  }

  if( obj->type == ITEM_WEAPON )
  {
    // Add avg damage.
    workingvalue += (obj->value[1] * obj->value[2]);
    // 1d1 = .7%, 5d5 = 17.5%, 10d10 = 70%.
    multiplier += obj->value[1] * obj->value[2] * .005;
    // Backstabbing weapons get a big ival for big dice.
    if( IS_BACKSTABBER(obj) )
    {
      mod = obj->value[2];
      // workingvalue increases quadratic for every die roll and cubic for dice size.
      // For number of dice: 1:1, 2:1.15, 3:1.4, 4:1.75, 5:2.2, 6:2.75, 7:3.4, 8:4.15, 9:5, 10:5.95
      // For number of die sides: 1:0, 2:1, 3:5, 4:12, 5:25, 6:43, 7: 68, 8:102, 9:145, 10: 200
      // So, 1d8 / 3d7 stabber is !forge and !enhance (115 ival when combined with above).
      workingvalue += ( (obj->value[1] * obj->value[1] + 19.) / 20. ) * ( mod * mod * mod ) / 5.;
    }
  }

  // Two handed items have less ival.
  if( IS_SET(obj->extra_flags, ITEM_TWOHANDS) )
  {
    multiplier *= .80;
  }

  if( workingvalue < 1 )
  {
    workingvalue = 1;
  }

  workingvalue *= multiplier;

  if( (!CAN_WEAR(obj, ITEM_TAKE) && obj->type == ITEM_TELEPORT) || obj->type == ITEM_KEY || obj->type == ITEM_SWITCH
    || obj->type == ITEM_VEHICLE || obj->type == ITEM_SHIP || obj->type == ITEM_STORAGE )
  {
    if( workingvalue != 1 )
    {
      debug( "Always load obj '%s' %d has stats giving ival %d.", OBJ_SHORT(obj), OBJ_VNUM(obj), workingvalue );
    }
    return 1;
  }

  //debug("&+YItem value is: &n%d", workingvalue); 
  return workingvalue;
}

void do_salvation(P_char ch, char *arg, int cmd)
{
    struct affected_type af;
 if(!affected_by_spell(ch, ACH_MAYIHEALSYOU) && !IS_TRUSTED(ch))
  {
    send_to_char("&+CYou have not earned the right to use this skill.&n\r\n", ch);
    return;
  }

 if(affected_by_spell(ch, TAG_SALVATION))
 {
  send_to_char("You must rest before invoking this spell again.\r\n", ch);
  return;
 }

   if(!ch)
   return;
       memset(&af, 0, sizeof(struct affected_type));
  	af.type = TAG_SALVATION;
  	af.modifier = 0;
  	af.duration = 24;
       af.location = 0;
       af.flags = AFFTYPE_NODISPEL;
       affect_to_char(ch, &af);
    act("&+YYou raise your eyes &+Cskyward&+Y in a plea for assistance...\n"
        "&+Y..after a brief moment, your body &+Ctingles&+Y with warmth, and you feel &+Crenewed&+Y.", FALSE, ch, 0, ch, TO_CHAR);
    act("&+Y$n raises their eyes &+Cskyward&+Y in a plea for assistance...\n"
        "&+Y..after a brief moment, $n's body &+Ctingles&+Y with warmth, and they feel &+Crenewed&+Y.", FALSE, ch, 0, 0, TO_ROOM);
        vamp(ch, number(100, 200), GET_MAX_HIT(ch));
}

void do_drandebug(P_char ch, char *arg, int cmd)
{
  return;
 /*
 P_obj obj;
 int value = itemvalue(ch, obj);
 debug("Item's value is: %d\r\n&n", value);
 */
}

// Returns the lowest material vnum (lowest quality) for material with the same type as obj->material.
// I.e. for a material type FEATHER -> returns 400000 (vnum for a small feather).
//   Material range for FEATHER is 400000 (lowest quality) - 400004 (highest quality).
int get_matstart(P_obj obj)
{
  int matstart;
  byte objmat;

  if( !obj )
  {
    return 0;
  }

  objmat = obj->material;

  switch( objmat )
  {
    case MAT_NONSUBSTANTIAL:
      matstart = 400205;
      break;
    case MAT_FLESH:
      matstart = 400005;
      break;
    case MAT_CLOTH:
      matstart = 400015;
      break;
    case MAT_BARK:
      matstart = 400035;
      break;
    case MAT_SOFTWOOD:
      matstart = 400040;
      break;
    case MAT_HARDWOOD:
      matstart = 400050;
      break;
/*
    case MAT_SILICON:
      matstart = 67283;
      break;
*/
    case MAT_CRYSTAL:
      matstart = 400090;
      break;
/*
    case MAT_CERAMIC:
      matstart = 67283;
      break;
*/
    case MAT_BONE:
      matstart = 400065;
      break;
    case MAT_STONE:
      matstart = 400095;
      break;
    case MAT_HIDE:
      matstart = 400030;
      break;
    case MAT_LEATHER:
      matstart = 400045;
      break;
    case MAT_CURED_LEATHER:
      matstart = 400060;
      break;
    case MAT_IRON:
      matstart = 400110;
      break;
    case MAT_STEEL:
      matstart = 400120;
      break;
    case MAT_BRASS:
      matstart = 400125;
      break;
    case MAT_MITHRIL:
      matstart = 400185;
      break;
    case MAT_ADAMANTIUM:
      matstart = 400195;
      break;
    case MAT_BRONZE:
      matstart = 400130;
      break;
    case MAT_COPPER:
      matstart = 400135;
      break;
    case MAT_SILVER:
      matstart = 400140;
      break;
    case MAT_ELECTRUM:
      matstart = 400145;
      break;
    case MAT_GOLD:
      matstart = 400150;
      break;
    case MAT_PLATINUM:
      matstart = 400180;
      break;
    case MAT_GEM:
      matstart = 400155;
      break;
    case MAT_DIAMOND:
      matstart = 400190;
    break;
/*
    case MAT_LEAVES:
      matstart = 67283;
      break;
*/
    case MAT_RUBY:
      matstart = 400165;
      break;
    case MAT_EMERALD:
      matstart = 400160;
      break;
    case MAT_SAPPHIRE:
      matstart = 400170;
      break;
    case MAT_IVORY:
      matstart = 400070;
      break;
    case MAT_DRAGONSCALE:
      matstart = 400200;
      break;
    case MAT_OBSIDIAN:
      matstart = 400175;
      break;
    case MAT_GRANITE:
      matstart = 400100;
      break;
    case MAT_MARBLE:
      matstart = 400105;
      break;
/*
    case MAT_LIMESTONE:
      matstart = 67283;
      break;
*/
    case MAT_BAMBOO:
      matstart = 400055;
      break;
    case MAT_REEDS:
      matstart = 400010;
      break;
    case MAT_HEMP:
      matstart = 400020;
      break;
    case MAT_GLASSTEEL:
      matstart = 400115;
      break;
    case MAT_CHITINOUS:
      matstart = 400080;
      break;
    case MAT_REPTILESCALE:
      matstart = 400085;
      break;
    case MAT_RUBBER:
      matstart = 400025;
      break;
    case MAT_FEATHER:
      matstart = 400000;
      break;
    case MAT_PEARL:
      matstart = 400075;
      break;
    default:
      matstart = 400205;
      break;
  }
  return matstart;
}

bool has_affect(P_obj obj)
{

         if (IS_SET(obj->bitvector, AFF_STONE_SKIN) ||
            IS_SET(obj->bitvector, AFF_HIDE) ||
            IS_SET(obj->bitvector, AFF_SNEAK) ||
            IS_SET(obj->bitvector, AFF_FLY) ||
            IS_SET(obj->bitvector, AFF4_NOFEAR) ||
            IS_SET(obj->bitvector2, AFF2_AIR_AURA) ||
            IS_SET(obj->bitvector2, AFF2_EARTH_AURA) ||
            IS_SET(obj->bitvector3, AFF3_INERTIAL_BARRIER) ||
            IS_SET(obj->bitvector3, AFF3_REDUCE) ||
            IS_SET(obj->bitvector2, AFF2_GLOBE) ||
            IS_SET(obj->bitvector, AFF_HASTE) ||
            IS_SET(obj->bitvector, AFF_DETECT_INVISIBLE) ||            
            IS_SET(obj->bitvector4, AFF4_DETECT_ILLUSION))
	    {
		return TRUE;
	    }
    return FALSE;
}

void do_refine(P_char ch, char *arg, int cmd)
{
  P_obj obj;
  P_obj t_obj, nextobj;
  int   i = 0, o = 0, vnum;
  int   orechance;
  bool  plat = FALSE;
  char  gbuf1[MAX_STRING_LENGTH], gbuf2[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH], gbuf3[MAX_STRING_LENGTH];

  argument_interpreter(arg, gbuf1, gbuf3);

  // If first argument.
  if(*gbuf1)
  {
    obj = get_obj_in_list(gbuf1, ch->carrying);
    if(!obj)
    {
      send_to_char("&nYou must have the item you wish to &+yre&+Yfi&+yne &nin your inventory.&n\r\n", ch);
      return;
    }

    vnum = OBJ_VNUM(obj);
    // Must be salvaged material, and cannot be the highest salvage type.
    if( (vnum > 400208) || (vnum < 400000) )
    {
      send_to_char("That item is not a &+ysalvaged&n item!\r\n", ch);
      return;
    }
    if( vnum == (get_matstart(obj) + 4) )
    {
      send_to_char("That &+bmaterial&n is already of the &+Bhighest&n quality.\n", ch );
      return;
    }
  }
  else
  {
    send_to_char("What &+ysalvaged &+Ymaterial &nwould you like to &+yre&+Yfi&+yne?\r\n", ch);
    return;
  }

  // Check for other materials of same quality/type
  for( t_obj = ch->carrying; t_obj; t_obj = t_obj->next_content )
  {
    if(OBJ_VNUM(t_obj) == vnum)
    {
      i++;
    }
    if((OBJ_VNUM(t_obj) > 193) && (OBJ_VNUM(t_obj) < 234))
    {
      o++;
    }
  }
  if(i < 2)
  {
    send_to_char("You need at least &+Y2 &nof the &+ymaterials&n in your inventory in order to &+yre&+Yfi&+yne&n it.\r\n", ch);
    return;
  }
  // If no ore, check for 50 plat..
  if(o != 1)
  {
    if( SUB_MONEY( ch, 50000, 0 ) != 0 )
    {
      send_to_char("You must have &+Wexactly &+Rone&n &+Lm&+yi&+Ln&+ye&+Ld ore &nin your inventory or 50 &+Wplatinum&n in order to &+yre&+Yfi&+yne&n it.\r\n", ch);
      return;
    }
    else
    {
      plat = TRUE;
    }
  }

  i = 2;
  for (t_obj = ch->carrying; t_obj; t_obj = nextobj)
  {
    nextobj = t_obj->next_content;

    if((OBJ_VNUM(t_obj) == vnum) && i > 0 )
    {
      obj_from_char(t_obj);
      extract_obj(t_obj, TRUE); // Not an arti, but 'in game.'
      send_to_char("You take the item and gently pour the melted ore over the item...\n", ch);
      i--;
    }
    if( o && (OBJ_VNUM(t_obj) > 193) && (OBJ_VNUM(t_obj) < 234) )
    {
      o--;
      obj_from_char(t_obj);
      extract_obj(t_obj, TRUE); // Not an arti, but 'in game.'
      orechance = (OBJ_VNUM(t_obj) - 194);
    }
  }

  int success = 61;
  // Increase by ore type or 10 pts for using platinum.
  success += plat ? 10 : orechance;
  if(success < number(1, 100))
  {
    if( plat )
    {
      act("&+W$n &+Ltakes their &+Wcoins&+L and begins to &+rh&+Rea&+Yt &+Lit in the &+yforge&+L.\r\n"
        "&+W$n &+Lgently removes the &+rm&+Ro&+Ylt&+Re&+rn &+ymetal &+Land starts to spread it about their $ps&+L, which &-L&+Rshatters&n &+Lfrom the intense &+rheat&+L!&N",
        TRUE, ch, obj, 0, TO_ROOM);
      act("&+LYou &+Ltake your &+Wcoins&+L and &+rh&+Rea&+Yt &+Lit in the &+yforge&+L.\r\n"
        "&+LYou &+Lgently remove the &+rm&+Ro&+Ylt&+Re&+rn &+ymetal &+Land start to spread it about your $ps&+L, which &-L&+Rshatters&n &+Lfrom the intense &+rheat&+L!&N",
        FALSE, ch, obj, 0, TO_CHAR);
    }
    else
    {
      act("&+W$n &+Ltakes their &+yore&+L and begins to &+rh&+Rea&+Yt &+Lit in the &+yforge&+L.\r\n"
        "&+W$n &+Lgently removes the &+rm&+Ro&+Ylt&+Re&+rn &+yore &+Land starts to spread it about their $ps&+L, which &-L&+Rshatters&n &+Lfrom the intense &+rheat&+L!&N",
        TRUE, ch, obj, 0, TO_ROOM);
      act("&+LYou &+Ltake your &+yore&+L and &+rh&+Rea&+Yt &+Lit in the &+yforge&+L.\r\n"
        "&+LYou &+Lgently remove the &+rm&+Ro&+Ylt&+Re&+rn &+yore &+Land start to spread it about your $ps&+L, which &-L&+Rshatters&n &+Lfrom the intense &+rheat&+L!&N",
        FALSE, ch, obj, 0, TO_CHAR);
    }
    return;
  }

  // Success!
  // Give them the new material.
  obj = read_object( vnum+1, VIRTUAL );
  obj_to_char(obj, ch);
  // Load a temp copy of the old material for messages.
  obj = read_object( vnum, VIRTUAL );
  act("&+W$n &+Ltakes their &+yore&+L and begins to &+rh&+Rea&+Yt &+Lit in the &+yforge&+L.\r\n"
    "&+W$n &+Lgently removes the &+rm&+Ro&+Ylt&+Re&+rn &+yore &+Land starts to spread it about their $ps&+L, which &+ycrack &+Land &+yreform&+L under the intense &+rheat&+L.&N",
    TRUE, ch, obj, 0, TO_ROOM);
  act("&+LYou &+Ltake your &+yore&+L and &+rh&+Rea&+Yt &+Lit in the &+yforge&+L.\r\n"
    "&+LYou &+Lgently remove the &+rm&+Ro&+Ylt&+Re&+rn &+yore &+Land start to spread it about your $ps&+L, which &+ycrack &+Land &+yreform&+L under the intense &+rheat&+L.&N",
    FALSE, ch, obj, 0, TO_CHAR);
  // Then remove it from game.
  extract_obj( obj );
}

void do_dice(P_char ch, char *arg, int cmd)
{
   char     first_arg[MAX_INPUT_LENGTH], second_arg[MAX_INPUT_LENGTH], gbuf[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      numdice, dice;

  arg = one_argument(arg, first_arg);

  if (is_number(first_arg))
  {
    numdice = atoi(first_arg);
    if (numdice > 5)
    {
      send_to_char("Sorry, dice rolls are limited to 5 at a time.\r\n", ch);
      return;
    }

 
    arg = one_argument(arg, second_arg);
    if (!*second_arg)
    {
      send_to_char("How many sides should the dice have?\r\n", ch);
      return;
    }
    if (!is_number(second_arg))
    {
      send_to_char("That is not a valid number.\r\n", ch);
      return;
    }
    dice = atoi(second_arg);
     if (dice > 100)
    {
      send_to_char("Dice can only have a maximum of &+W100&n sides.", ch);
      return;
    }

    int result;
    int i = 0;
    while (i < numdice)
    {
	result = number(1, dice);
       i++;
      sprintf(gbuf, "&+yResult for &+W%s's&+y roll &+L#&+W%d&+y of a &+r%d&+y sided die: &+W%d&+y.", GET_NAME(ch), i, dice, result);
     act(gbuf, FALSE, ch, 0, 0, TO_CHAR);
     act(gbuf, FALSE, ch, 0, 0, TO_ROOM);

     // send_to_char(gbuf, ch);
    }
   return; 
  }
 send_to_char("Roll some dice! Syntax: dice <number> <sides>\r\n", ch);
}

int assoc_founder(P_char mob, P_char pl, int cmd, char *arg)
{
  char buffer[MAX_STRING_LENGTH], buffer2[MAX_STRING_LENGTH], guild_name[MAX_INPUT_LENGTH];
  int qend;

  if( cmd == CMD_LIST )
  {
    if( !arg || !*arg )
    {
      sprintf( buffer2, "%s", PERS(mob, pl, FALSE) );
      sprintf( buffer, "%s&+L looks you over briefly and then chuckles.'\n"
	      "%s &+wsays 'Welcome adventurer.  If it is a guild ye are wishing to found, then ye have come to the right place&n.'\n"
	      "%s &+wsays 'Ye will find the command to create a guild listed here, as well as the cost.'\n"
        "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+\n"
		    "&+y|&+WFounding a guild will cost &+R%s &+Wplatinum coins.                                       &+y|\n"
        "&+y|&+WTo found the guild, type the following command and have the coins on your character:      &+y|\n"
        "&+y|&+Wbuy '<name of guild>'                                                                     &+y|\n"
        "&+y|                                                                                             &+y|\n"
        "&+y|&+Wexample: buy '&&+Lthe &&+MNetheril &&+mMages&&n'                                          &+y|\n"
        "&+y|&+WThis will create a guild named: '&+Lthe &+MNetheril &+mMages&n'                           &+y|\n"
        "&+y|&nSee help &+cansi &nand help &+ctestcolor&n for help in creatnig your guild colors.         &+y|\n"
        "&+y|&+WBe warned! Once you create the guild, there is no way to rename it, for now.              &+y|\n"
        "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+\n"
        "&+RNO racist, hate speech, or curse words will be tolerated in guild names. Violations will be met with a ban.\n"
		    "\n", buffer2, buffer2, buffer2, comma_string(GUILD_COST / 1000) );
      send_to_char(buffer, pl);
      return TRUE;
    }//endifnoarg
  }//endiflist

  if( cmd == CMD_BUY )
  {

    if( GET_ASSOC(pl) != NULL )
    {
     send_to_char("You cannot form a guild if you are currently a member of a guild.\r\n", pl);
     return TRUE;
    }

    arg = skip_spaces(arg);
    if( (*arg != '\'') || (arg[strlen( arg ) - 1] != '\'') )
    {
      send_to_char("The guild name must be enclosed in 'apostrophes'.\n", pl);
      send_to_char_f( pl, "\"%s\" is not good enough.\n", arg );
      return TRUE;
    }
    // Skip the opening '.
    sprintf( guild_name, "%s", arg + 1 );
    // Overwrite the closing ' with a color normal.
    sprintf( guild_name + strlen(guild_name) - 1, "&n" );

    if( sub_string_cs(guild_name, "&-") || sub_string_cs(guild_name, "&=") )
    {
      send_to_char("You may not have blinking ansi or background colors in a guild name.\r\n", pl);
      return TRUE;
    }

    if( sub_string_set(guild_name, rude_ass) )
    {
      send_to_char("No curse words. Violations will be met with ban.\r\n", pl);
      debug( "Player '%s' %d is tryiing to make a rude-named guild: '%s'", J_NAME(pl), GET_ID(pl), guild_name );
      return TRUE;
    }

    sprintf(buffer2, "You have selected: %s for your guild name, is this correct? (y/n)\n", guild_name );
    send_to_char(buffer2, pl);

    strcpy(pl->desc->last_command, guild_name);
    strcpy(pl->desc->client_str, "found_asc");
    pl->desc->confirm_state = CONFIRM_AWAIT;

    return TRUE;
  }
  return FALSE;
}

int get_mincircle( int spell )
{
  int i, j;
  int lowest = 13;

  // If not a spell, return 0.
  if( spell < FIRST_SPELL || spell > LAST_SPELL )
  {
    return 0;
  }

  // Skip CLASS_NONE..
  for( i = 1; i <= CLASS_COUNT; i++ )
  {
    for( j = 0; j < MAX_SPEC; j++ )
    {
      if( (skills[spell].m_class[i].rlevel[j] > 0) && (skills[spell].m_class[i].rlevel[j] <= lowest))
      {
        lowest = skills[spell].m_class[i].rlevel[j];
      }
    }
  }

  return lowest;
}

// New object proc?  Add it here with a value associated with it.
int get_ival_from_proc( obj_proc_type proc )
{

  // Procs stoneskin on 1 min timer.
  if( proc == artifact_stone )
  {
    return 125;
  }
  // Gives bio.
  if( proc == artifact_biofeedback )
  {
    return 110;
  }
  // Hides char.
  if( proc == artifact_hide )
  {
    return 85;
  }
  // Turn invis on say invis proc.
  if( proc == artifact_invisible )
  {
    return 80;
  }
  // Poof if in sulight proc.
  if( proc == generic_drow_eq )
  {
    return 5;
  }
  // Str/Enhance Str proc.
  if( proc == jet_black_maul )
  {
    return 50;
  }
  // (un)holy word + dispel good|evil area.
  if( proc == faith )
  {
    return 60;
  }
  // Area damage of 12 + target damage of 50.
  if( proc == mistweave )
  {
    return 60;
  }
  // Procs pseudo-dodge 1/50 chance.
  if( proc == leather_vest )
  {
    return 50;
  }
  // Area levitate/fly.
  if( proc == deva_cloak )
  {
    return 75;
  }
  // Procs ice storm once per minute.
  if( proc == icicle_cloak )
  {
    return 50;
  }
  // 1/15 chance to proc big fist on an ogre.
  if( proc == ogrebane )
  {
    return 60;
  }
  // 1/20 chance to proc fist on giant !dragon.
  if( proc == giantbane )
  {
    return 65;
  }
  // 1/25 chance to proc wither/lightning on either dwarf.
  if( proc == dwarfslayer )
  {
    return 65;
  }
  // 1/30 chance of uber feeblemind.
  if( proc == mindbreaker )
  {
    return 70;
  }
  // Level potions? hah.
  if( proc == treasure_chest )
  {
    return 200;
  }
  // Monolith: feeds artis.
  if( proc == artifact_monolith )
  {
    return 200;
  }
  // Gives miners sight.
  if( proc == miners_helmet )
  {
    return 125;
  }
  // 1/30 chance for destroy undead or dispel evil or cause crit.
  if( proc == glades_dagger )
  {
    return 85;
  }
  // 1/30 chance to proc extra attacks.
  if( proc == lucky_weapon )
  {
    return 55;
  }
  // 1/10 chance for 400-500 dam to dragons.
  if( proc == mace_dragondeath )
  {
    return 135;
  }
  // 1/30 chance to proc random spellup.
  if( proc == khildarak_warhammer )
  {
    return 65;
  }
  // 1/30 chance to proc berserk.
  if( proc == ogre_warlords_sword )
  {
    return 45;
  }
  // 1/30 chance to proc random fire spell.
  if( proc == flaming_axe_of_azer )
  {
    return 70;
  }
  // 1/14 chance for small fireball.
  if( proc == mrinlor_whip )
  {
    return 45;
  }
  // ' sphinx -> mordenkainens_lucubration evey 5 min.
  if( proc == sphinx_prefect_crown )
  {
    return 125;
  }
  // 1min stone timer and hummer
  if( proc == platemail_of_defense )
  {
    return 130;
  }
  // Effects based on # items worn
  if( proc == master_set )
  {
    return 35;
  }
  // Feeds the user.. not really useful.
  if( proc == ioun_sustenance )
  {
    return 45;
  }
  // 1/4 chance to deflect nuke to someone else.
  if( proc == deflect_ioun )
  {
    return 225;
  }
  // Raises a level on 1st touch.  Destroys all eq in room on second touch.
  if( proc == ioun_testicle )
  {
    return 250;
  }
  // Relocate ioun usable each minute
  if( proc == ioun_warp )
  {
    return 225;
  }
  // Monk artifact tendrils proc (riposte + stone).
  if( proc == tendrils )
  {
    return 205;
  }
  // 1/4 chance to deflect spell.
  if( proc == elvenkind_cloak )
  {
    return 225;
  }
  // 1/10 chance to proc earthquake if fighting.
  if( proc == earthquake_gauntlet )
  {
    return 65;
  }
  // 1/25 chance to proc blind if fighting (limited targets).
  if( proc == blind_boots )
  {
    return 50;
  }
  // Invigorates every 200 sec.
  if( proc == thanksgiving_wings )
  {
    return 85;
  }
  // Pass without trace proc every 800 sec.
  if( proc == pathfinder )
  {
    return 85;
  }
  // 1/16 chance on hit to proc disintegrate.
  if( proc == orb_of_destruction )
  {
    return 115;
  }
  // armor/bless invoke proc every 5 min.
  if( proc == sanguine )
  {
    return 55;
  }
  // 1/30 chance during battle to proc 75 damage.
  if( proc == neg_orb )
  {
    return 85;
  }
  // Lots of say procs and no timer and battle proc.
  if( proc == totem_of_mastery )
  {
    return 350;
  }
  // Procs heal on rub invoke every 150 sec = 2 1/2 min.
  if( proc == ring_of_regeneration )
  {
    return 110;
  }
  // Summons water mental invoke every 3 min.
  if( proc == glowing_necklace )
  {
    return 120;
  }
  // Summons shadow pet every 12 min. agg if under.
  if( proc == staff_shadow_summoning )
  {
    return 115;
  }
  // 1/16 chance during battle to proc stornogs_lowered_magical_res.
  if( proc == rod_of_magic )
  {
    return 95;
  }
  // Skill whirlwind on invoke every 5 min.
  if( proc == proc_whirlwinds )
  {
    return 150;
  }
  // 1/3 chance procs random bard off songs. Bard prot songs every 30 sec.
  if( proc == lyrical_instrument_of_time )
  {
    return 220;
  }
  // 1/25 chance to proc flee.
  if( proc == sinister_tactics_staff )
  {
    return 85;
  }
  // 1/25 chance for low lvl feeb.
  if( proc == shard_frozen_styx_water )
  {
    return 65;
  }
  // Ambran/DeathRider proc.
  if( proc == holy_weapon )
  {
    return 350;
  }
  // Mox totem proc (heal / 30 sec & 1/3 chance to debuff).
  if( proc == mox_totem )
  {
    return 150;
  }
  // Cures blind every 30 min on invoke.
  if( proc == flayed_mind_mask )
  {
    return 25;
  }
  // Enlarge/Reduce on say 3 min timer/Hide 1 min timer.
  if( proc == stalker_cloak )
  {
    return 235;
  }
  // Airy water on 1 min timer.
  if( proc == finslayer_air )
  {
    return 105;
  }
  // Procs hide every 15 min.
  if( proc == aboleth_pendant )
  {
    return 95;
  }
  // 1/30 chance on got-hit to proc lightning/call lightning.
  if( proc == lightning_armor )
  {
    return 115;
  }
  // Procs major para for 5 mins on each got-hit.
  if( proc == imprison_armor )
  {
    return 550;
  }
  // 1/2 chance to proc magic missile + ice missile.
  if( proc == fun_dagger )
  {
    return 50;
  }
  // 1/2 chance to bigbys hand.
  if( proc == rax_red_dagger )
  {
    return 200;
  }
  // Procs damage when try to pick up.
  if( proc == cutting_dagger )
  {
    return 100;
  }
  // Procs mass heal on say, and 1/15 chance for lightning bolt in battle.
  if( proc == circlet_of_light )
  {
    return 220;
  }
  // 1/30 lvl 25 greater spirit anguish heh.
  if( proc == ljs_sword )
  {
    return 75;
  }
  // 1/30 chance to proc lvl 1 magic missile.
  if( proc == wuss_sword )
  {
    return 5;
  }
  // 1/30 chance to proc lvl 30 magma burst.
  if( proc == head_guard_sword )
  {
    return 65;
  }
  // Procs mass heal every 2 min.
  if( proc == priest_rudder )
  {
    return 125;
  }
  // Makes ingreds every minute.
  if( proc == alch_bag )
  {
    return 75;
  }
  // 1/30 chance to proc mental/shadow monsters
  if( proc == alch_rod )
  {
    return 105;
  }
  // 1/10 chance to prot vs undead/living depending on victim.
  if( proc == ljs_armor )
  {
    return 75;
  }
  // 1/2 periodic proc of destroy undead & various detect buffs every minute.
  if( proc == dragon_skull_helm )
  {
    return 225;
  }
  // 1/30 chance to proc drain moves.
  if( proc == nightcrawler_dagger )
  {
    return 50;
  }
  // 1/30 chance to proc virtue then procs damage spell.
  if( proc == righteous_blade )
  {
    return 85;
  }
  // 1/20 chance to proc a nuke.
  if( proc == xmas_cap )
  {
    return 125;
  }
  // 1/50 chance to remove skin spell.
  if( proc == khaziddea_blade )
  {
    return 75;
  }
  // Makes ch a Revenant.
  if( proc == revenant_helm )
  {
    return 225;
  }
  // Stornogs spheres every 30 min & stone every 30 sec.
  if( proc == dragonlord_plate )
  {
    return 255;
  }
  // 1/25 chance for sunray/heal & stone every 30 sec.
  if( proc == sunblade )
  {
    return 275;
  }
  // Procs damage/vamp.
  if( proc == bloodfeast )
  {
    return 175;
  }
  // Must be lvl < 36, 1/20 chance of haste/stone/burning hands on dragon.
  if( proc == dragonslayer )
  {
    return 55;
  }
  // 1/15 chance to proc stun/nightmare/shatter on men.
  if( proc == mankiller )
  {
    return 115;
  }
  // 1/30 chance to blur, 1/20 chance to riposte (multiple hits possible).
  if( proc == madman_mangler )
  {
    return 220;
  }
  // 1/10 chance to proc 10-20 damage back on attacker.
  if( proc == madman_shield )
  {
    return 115;
  }
  // Reflections on a 5 min timer.
  if( proc == mentality_mace )
  {
    return 115;
  }
  // 1/10 chance of damage+vamp proc, coldshield/fireshield, globe.
  if( proc == vapor )
  {
    return 275;
  }
  // 2min 40sec timer for random buff.
  if( proc == serpent_of_miracles )
  {
    return 115;
  }
  // 1/30 chance for 300 damage proc.
  if( proc == transparent_blade )
  {
    return 175;
  }
  // Invoke heal 3 min timer, blocks/bashes.
  if( proc == Einjar )
  {
    return 200;
  }
  // 1/10 chance to 70% chance knockdown or fail.
  if( proc == tripboots )
  {
    return 125;
  }
  // 1/6 chance to blind target (70%) or ch (30%)/
  if( proc == blindbadge )
  {
    return 85;
  }
  // Makes you drop yer weapon?!?
  if( proc == fumblegaunts )
  {
    return 5;
  }
  // 1/30 chance to inflict pain/ego blast.
  if( proc == confusionsword )
  {
    return 65;
  }
  // Random stat buffs.
  if( proc == guild_badge )
  {
    return 125;
  }
  // Random object proc (6 same zone == stone etc).
  if( proc == set_proc )
  {
    return 55;
  }
  // All in room cure crit proc every periodic (kills vamp).
  if( proc == thrusted_eq_proc )
  {
    return 105;
  }
  // Random skill or energy drain.
  if( proc == encrusted_eq_proc )
  {
    return 125;
  }
  // Forge recipe.
  if( proc == parchment_forge )
  {
    return 25;
  }
  // Old proc.. Holy relic of the Gods.
  if( proc == relic_proc )
  {
    return 200;
  }
  // 1/15 chance to summon mentals on hit, 1/4 chance to revenge nuke (Alchemist arti).
  if( proc == cold_hammer )
  {
    return 175;
  }
  // 1/30 chance to inflict pain/feeb or zerk.
  if( proc == brainripper )
  {
    return 125;
  }
  // 1/30 chance to double lightningbolt.
  if( proc == hammer_titans )
  {
    return 75;
  }
  // 1/30 chance for cyclone then another area nuke.
  if( proc == stormbringer )
  {
    return 175;
  }
  // Necroplasm arti proc..
  if( proc == living_necroplasm )
  {
    return 500;
  }
  // Vit mask.
  if( proc == vigor_mask )
  {
    return 125;
  }
  // Armor/bless proc invoke every 15sec.
  if( proc == church_door )
  {
    return 35;
  }
  // Double cure crit invoke every 15min.
  if( proc == splinter )
  {
    return 75;
  }
  // 1/25 chance to damage moves.
  if( proc == demo_scimitar )
  {
    return 65;
  }
  // 1/10 chance to proc flee on ch->fighting.
  if( proc == dranum_mask )
  {
    return 105;
  }
  // Procs invoke area slow every 24 min.
  if( proc == golem_chunk )
  {
    return 155;
  }
  // Mayhem/Symmetry.. big arti proc.
  if( proc == good_evil_sword )
  {
    return 500;
  }
  // Stones every 30 sec, invoke invis/hide every 2 min.
  if( proc == transp_tow_misty_gloves )
  {
    return 240;
  }
  // Big god-only arti
  if( proc == gfstone )
  {
    return 750;
  }
  // 1/10 chance to proc disarm on got-hit.
  if( proc == disarm_pick_gloves )
  {
    return 165;
  }
  // Yeah.. reload/fire pistol until dead.
  if( proc == roulette_pistol )
  {
    return 200;
  }
  // Invoke proc of mirage every 3 min.
  if( proc == orb_of_deception )
  {
    return 180;
  }
  // Zombies game proc.  God only.
  if( proc == zombies_game )
  {
    return 1000;
  }
  // Sword of sharpness proc: cuts limbs.
  if( proc == illesarus )
  {
    return 185;
  }
  // 30% chance on got-hit dispel good/disease/dispel magic
  if( proc == vecna_pestilence )
  {
    return 95;
  }
  // 6% chance to proc level 51 bigbys on hit.
  if( proc == vecna_minifist )
  {
    return 130;
  }
  // 5% chance to level 51 dispel magic proc on hit.
  if( proc == vecna_dispel )
  {
    return 125;
  }
  // 6% chance to level 51 iceball proc on hit.
  if( proc == vecna_boneaxe )
  {
    return 130;
  }
  // 10% chance for off proc, say procs.
  if( proc == vecna_staffoaken )
  {
    return 225;
  }
  // Krindors device.. big arti.
  if( proc == vecna_krindor_main )
  {
    return 255;
  }
  // Various damage procs.
  if( proc == vecna_death_mask )
  {
    return 135;
  }
  // Various procs vs undead.
  if( proc == mob_vecna_procs )
  {
    return 150;
  }
  // 1/4 chance to bleed proc on periodic.
  if( proc == lifereaver )
  {
    return 105;
  }
  // Kills stats on item if given to another.
  if( proc == flame_blade )
  {
    return 10;
  }
  // Area vit and tsunami wave proc.
  if( proc == SeaKingdom_Tsunami )
  {
    return 225;
  }
  // 1/30 chance to proc lightningbolt.
  if( proc == shimmering_longsword )
  {
    return 65;
  }
  // 3% chance to do mega damage (female/Drow/Cleric only).
  if( proc == rod_of_zarbon )
  {
    return 225;
  }
  // Procs coldshield/fireshield on invoke when lit.
  if( proc == frost_elb_dagger )
  {
    return 85;
  }
  // Procs feeblemind on backstab.
  if( proc == dagger_submission )
  {
    return 185;
  }
  // 1/4 chance to proc wither on hit, 60 sec stone proc.
  if( proc == trans_tower_shadow_globe )
  {
    return 195;
  }
  // Burns the target upon get/take/etc.
  if( proc == burn_touch_obj )
  {
    return 105;
  }
  // 5% chance to proc silence.
  if( proc == squelcher )
  {
    return 115;
  }
  // Absorbs dragonbreath.
  if( proc == dragonarmor )
  {
    return 225;
  }
  // Turns char into a Kearonor Beast (Demon).
  if( proc == kearonor_hide )
  {
    return 185;
  }
  // Allows clairovoyance to anyone while playing organ.
  if( proc == hewards_mystical_organ )
  {
    return 275;
  }
  // Clairovoyance to specific room on timer.
  if( proc == amethyst_orb )
  {
    return 105;
  }
  // Random procs on use wand (limited uses).
  if( proc == wand_of_wonder )
  {
    return 125;
  }
  // 1/31 chance to holy word.
  if( proc == blade_of_paladins )
  {
    return 125;
  }
  // 1/31 chance to proc level 35 power word blind.  invoke invis once per mud day.
  if( proc == fade_drusus )
  {
    return 95;
  }
  // Procs chain lightning and eats corpses and stuff.
  if( proc == lightning_sword )
  {
    return 225;
  }
  // Changes stats based on target.
  if( proc == elfdawn_sword )
  {
    return 105;
  }
  // Invoke fly/proc flamestrike 1/31 chance.
  if( proc == flame_of_north_sword )
  {
    return 130;
  }
  // 1/31 chance to feeb, and updates stats based on target.
  if( proc == magebane_falchion )
  {
    return 175;
  }
  // Heals wielder on hit.
  if( proc == woundhealer_scimitar )
  {
    return 125;
  }
  // 1/30 chance to proc harm on hit, invoke full heal once per mud day.
  if( proc == martelo_mstar )
  {
    return 210;
  }
  // 1/400 chance to proc behead (instadeath).
  if( proc == githpc_special_weap )
  {
    return 550;
  }
  // Allows accept/decline/ptell imm commands to user.
  if( proc == trustee_artifact )
  {
    return 10000;
  }
  // Invoke proc that sacrifices/terminates/CDs target.
  if( proc == orcus_wand )
  {
    return 10000;
  }
  // Command murder -> Instakill target.
  if( proc == varon )
  {
    return 10000;
  }
  // Proc for traps.
  if( proc == huntsman_ward )
  {
    return 125;
  }
  // 1/20 chance to proc lightning bolt on hit.
  if( proc == hammer )
  {
    return 45;
  }
  // The uber warhammer arti proc.
  if( proc == barb )
  {
    return 550;
  }
  // 1/10 chance to proc harm.
  if( proc == gesen )
  {
    return 85;
  }
  // Labelas god proc.
  if( proc == labelas )
  {
    return 10000;
  }
  // Various dragon procs.
  if( proc == dragonkind )
  {
    return 235;
  }
  // Invoke area res once per hour, 1/17 chance to proc group heal periodic.
  if( proc == resurrect_totem )
  {
    return 325;
  }
  // Slowly disappears on cast(s).
  if( proc == crystal_spike )
  {
    return 75;
  }
  // Opens a doorway (invaluable in a zone).
  if( proc == automaton_lever )
  {
    return 200;
  }
  // Object turns into a corpse heh.
  if( proc == forest_corpse )
  {
    return 100;
  }
  // Procs uber blind/poison.
  if( proc == torment )
  {
    return 125;
  }
  // Avernus proc.  Uber arti.
  if( proc == avernus )
  {
    return 250;
  }
  // 1/35 chance to proc behead/big hurt.
  if( proc == githyanki )
  {
    return 550;
  }
  // 1/30 chance to proc firestorm/immo/ice storm/cone of cold.
  if( proc == kvasir_dagger )
  {
    return 105;
  }
  // Mega arti Doombringer proc.
  if( proc == doombringer )
  {
    return 350;
  }
  // 1/30 chance for incendiary, burning hands & fireball.
  if( proc == flamberge )
  {
    return 220;
  }
  // Charms mentals.  Limited usage.
  if( proc == ring_elemental_control )
  {
    return 175;
  }
  // 1/25 chance to proc flamestrike & full harm, procs healing 1/4 chance on got hit.
  if( proc == holy_mace )
  {
    return 185;
  }
  // Uber arti staff of blue flames proc.
  if( proc == staff_of_blue_flames )
  {
    return 320;
  }
  // 1/16 chance to proc psychic crush.  Various psi spellups.
  if( proc == staff_of_power )
  {
    return 320;
  }
  // Summons a pegaus pet.
  if( proc == reliance_pegasus )
  {
    return 180;
  }
  // 1/16 chance for triple lightning bolt proc, invoke area bash.
  if( proc == lightning )
  {
    return 225;
  }
  // Procs stop battles.
  if( proc == dagger_of_wind )
  {
    return 125;
  }
  // 1/90 chance to banish to water plane.
  if( proc == orb_of_the_sea )
  {
    return 280;
  }
  // 1/25 chance to proc earthquake & earthen rain.  Invoke stone, proc stone on got hit.
  if( proc == zion_mace_of_earth )
  {
    return 280;
  }
  // 1/25 chance for damage + vamp on melee hit proc.
  if( proc == unholy_avenger_bloodlust )
  {
    return 240;
  }
  // 1/16 to proc major para spell.
  if( proc == tiamat_stinger )
  {
    return 280;
  }
  // 1/16 chance to proc 3 fireballs and a flamestrike.
  if( proc == dispator )
  {
    return 280;
  }
  // 1/30 chance to proc uber sleep and 1/40 to proc darkness.
  if( proc == nightbringer )
  {
    return 175;
  }
  // 1/30 chance to proc attempt to charm undead.
  if( proc == undead_trident )
  {
    return 185;
  }
  // 1/30 chance to proc disarm.
  if( proc == iron_flindbar )
  {
    return 100;
  }
  // 1/20 chance to proc parry
  if( proc == generic_parry_proc )
  {
    return 135;
  }
  // 1/20 chance to proc riposte
  if( proc == generic_riposte_proc )
  {
    return 155;
  }
  // Various Reaver spellups.
  if( proc == flame_of_north )
  {
    return 215;
  }
  // Summons a mob and breaks.
  if( proc == menden_figurine )
  {
    return 105;
  }
  // 1/33 chance to do 10d24 spell damage proc & 1/33 chance to do 2 extra hits.
  if( proc == sevenoaks_longsword )
  {
    return 225;
  }
  // 1/30 chance for lightning bolt proc.
  if( proc == mace_of_sea )
  {
    return 115;
  }
  // 1/30 chance for poison & minor para.
  if( proc == serpent_blade )
  {
    return 145;
  }
  // 1/30 chance to proc uber wither.
  if( proc == lich_spine )
  {
    return 185;
  }
  // Max 1/9 chance to proc random necro debuff.
  if( proc == doom_blade_Proc )
  {
    return 125;
  }
  // 1/50 chance for random fire spell.
  if( proc == dagger_ra )
  {
    return 195;
  }
  // 1/35 chance to proc psychic crush or bash, and 10 min invoke ether warp.
  if( proc == illithid_axe )
  {
    return 320;
  }
  // 1/33 chance for random damage proc.  Lots of random buffs/heals.
  if( proc == deathseeker_mace )
  {
    return 235;
  }
  // 1/33 chance to nuke, and nice buffs on 10 min timer.
  if( proc == demon_slayer )
  {
    return 235;
  }
  // 10 min timer for skin spells & 1/33 chance for random nuke.
  if( proc == snowogre_warhammer )
  {
    return 235;
  }
  // 1/33 chance to proc heal/bash/energy drain & lame teleport proc.
  if( proc == volo_longsword )
  {
    return 185;
  }
  // 1/25 chance random nuke, blur 10 min timer, shadow shield & vanish on 10 min timer.
  if( proc == blur_shortsword )
  {
    return 235;
  }
  // 7 min 30 sec timer for double group heal/vit/soulshield,accel healing & endurance(all area).  1/25 chance to proc dispel align.
  if( proc == buckler_saints )
  {
    return 235;
  }
  // Invoke vamp trance.
  if( proc == helmet_vampires )
  {
    return 300;
  }
  // Invoke call lightning 10 min timer, 1/30 chance to proc double lightning, regen & endurance on 10 min timer.
  if( proc == storm_legplates )
  {
    return 205;
  }
  // 1/10 chance to hit/vamp, 3 min stone timer, 10 min invoke nonexistance timer.
  if( proc == gauntlets_legend )
  {
    return 235;
  }
  // 1/30 chance to proc backstab on melee hit.
  if( proc == gladius_backstabber )
  {
    return 205;
  }
  // Transforms on rub wand.
  if( proc == elemental_wand )
  {
    return 105;
  }
  // Adds 2 maxstats to earring random every 10 min.
  if( proc == earring_powers )
  {
    return 125;
  }
  // Transforms into another object.
  if( proc == lorekeeper_scroll )
  {
    return 135;
  }
  // 1/20 chance to proc curse or malison.
  if( proc == damnation_staff )
  {
    return 115;
  }
  // 1/20 chance to proc curse or malison.
  if( proc == nuke_damnation )
  {
    return 115;
  }
  // Summons an ice mental, 10 min timer.
  if( proc == collar_frost )
  {
    return 185;
  }
  // Summons a fire mental, 10 min timer.
  if( proc == collar_flames )
  {
    return 185;
  }
  // Creates a gift when opened.
  if( proc == lancer_gift )
  {
    return 150;
  }
  // 1/20 chance to absorb spell and blind ch.
  if( proc == zion_shield_absorb_proc )
  {
    return 195;
  }
  // 1/5 chance to block a hit.
  if( proc == generic_shield_block_proc )
  {
    return 185;
  }
  // Turns land to lava, fire auras, and 1/25 procs fire spells.
  if( proc == zion_fnf )
  {
    return 245;
  }
  // 1/25 chance to proc nuke, 1 min timer on stone + buffs.
  if( proc == zion_light_dark )
  {
    return 235;
  }
  // Procs nukes on undead.
  if( proc == barovia_undead_necklace )
  {
    return 185;
  }
  // Procs shadow shield every minute as needed.
  if( proc == artifact_shadow_shield )
  {
    return 135;
  }
  // Opens swordcase in zone.
  if( proc == ravenloft_bell )
  {
    return 200;
  }
  // Curses and blurs wielder.
  if( proc == shimmer_shortsword )
  {
    return 175;
  }
  // 1/50 chance to proc hellfire
  if( proc == hellfire_axe )
  {
    return 175;
  }
  // 1/30 chance to proc trip.
  if( proc == illithid_whip )
  {
    return 210;
  }
  // Auto-wear leggings. lame.
  if( proc == skull_leggings )
  {
    return 25;
  }
  // 1/20 chance to proc damage to undead.
  if( proc == deliverer_hammer )
  {
    return 155;
  }
  // Invoke armor every 60 sec.
  if( proc == blue_sword_armor )
  {
    return 45;
  }
  // 1/30 chance to proc level 30 dispel magic.
  if( proc == sword_named_magik )
  {
    return 65;
  }
  // 1/20 chance to proc 400 damage & vamp, invoke ilienzes flame sword every 4 minues.
  if( proc == bel_sword )
  {
    return 210;
  }
  // 1/25 chance to proc destroy undead & cure serious.
  if( proc == zarthos_vampire_slayer )
  {
    return 115;
  }
  // 1/30 chance to proc bash/stun/blind.
  if( proc == critical_attack_proc )
  {
    return 155;
  }
  // Just displays cool periodic messages.
  if( proc == mask_of_wildmagic )
  {
    return 100;
  }
  // Invoke auto-mem a spell on 5 min 50 sec timer.
  if( proc == flow_amulet )
  {
    return 200;
  }
  // Invoke proc blink every 15-20 sec, Invoke proc group deflect every 8 min 20 sec.
  if( proc == zion_netheril )
  {
    return 275;
  }
  // 1/20 chance to proc blind, disease, curse or dispel magic on hit.  Procs heal every 20 sec as necessary.
  if( proc == eth2_godsfury )
  {
    return 185;
  }
  // 1/5 chance to proc summon helper on periodic.
  if( proc == eth2_aramus_crown )
  {
    return 175;
  }
  // Procs word of recall on 30 min timer.
  if( proc == lucrot_mindstone )
  {
    return 200;
  }

  // Will make itemvalue(P_obj) return 1 (hopefully).
  // This is important since we want these items to always load. (load % based on itemvalue for 2015-6 wipe).
  if( proc == teleporting_map_pool || proc == storage_locker_obj_hook || proc == guildhall_door
    || proc == guildhall_heartstone || proc == tharnrifts_portal || proc == olympus_portal
    || proc == vecna_deathportal || proc == portal_door || proc == portal_wormhole || proc == verzanan_portal
    || proc == newbie_portal || proc == no_kill_priest_obj || proc == magic_pool || proc == moonstone_fragment
    || proc == treasure_chest || proc == cards_object || proc == artifact_monolith || proc == burbul_map_obj
    || proc == chyron_search_obj || proc == blood_stains || proc == ice_shattered_bits || proc == tracks
    || proc == frost_beacon || proc == ice_block || proc == charon_ship || proc == moonstone || proc == nexus
    || proc == pesky_imp_chest || proc == tower_summoning || proc == shabo_trap_north || proc == shabo_trap_north_two
    || proc == shabo_trap_south || shabo_trap_south_two || proc == shabo_trap_up || proc == shabo_trap_down
    || proc == shabo_trap_up_two || proc == slot_machine || proc == unspec_altar || proc == epic_stone
    || proc == stat_pool_str || proc == stat_pool_dex || proc == stat_pool_agi || proc == stat_pool_con
    || proc == stat_pool_pow || proc == stat_pool_int || proc == stat_pool_wis || proc == stat_pool_cha
    || proc == stat_pool_luc || proc == spell_pool || proc == druid_spring || proc == blighter_pond
    || proc == flying_citadel || proc == trap_razor_hooks || proc == trap_tower1_para || proc == trap_tower2_sleep
    || proc == hoa_plat || proc == vecna_deathportal || proc == vecna_deathaltar || proc == vecna_stonemist
    || proc == vecna_ghosthands || proc == vecna_torturerroom || proc == vecna_gorge || proc == mob_vecna_procs
    || proc == arenaobj_proc || proc == burn_touch_obj || proc == drowcrusher || proc == hewards_mystical_organ
    || proc == mir_fire || proc == board || proc == random_tomb || proc == random_glass || proc == random_slab
    || proc == refreshing_fountain || proc == magical_fountain || proc == changelog || proc == wall_generic
    || proc == huntsman_ward || proc == item_switch || proc == clock_tower || proc == jailtally
    || proc == verzanan_portal || proc == die_roller || proc == elfgate || proc == guildwindow || proc == guildhome
    || proc == automaton_lever || proc == illithid_teleport_veil || proc == teleporting_pool || proc == llyms_altar
    || proc == newbie_sign1 || proc == newbie_sign2 || proc == vareena_statue || proc == wh_corpse_decay
    || proc == dragon_heart_decay || proc == ravenloft_bell || proc == toe_chamber_switch || proc == flesh_golem_repop
    || proc == unmulti_altar || proc == jubilex_grid_mob_generator || proc == eth2_tree_obj || proc == magic_deck
    || proc == blackjack_table || proc == toe_chamber_switch || proc == drowcrusher )
  {
    return -100000;
  }

  // Default.. tough call here, but 100 guarentees it's not craftable.
  return 100;
}

// Calculates the base cost of ore (metal and size variables & level and skill of ch).
int calc_ore_cost( P_char ch, P_obj ore )
{
  // Cost in copper.
  float newcost;
  int vnum = OBJ_VNUM(ore);

  // Type of metal: 0=iron, 1=steel, 2=copper, 3=silver, 4=gold, 5=platinum, 6=mithril, 7=adamantium.
  switch( (vnum-LOWEST_ORE_VNUM) / 3 )
  {
    case 0: // Iron
      newcost = 20000.0;
      break;
    case 1: // Tin
      newcost = 35000.0;
      break;
    case 2: // Copper
      newcost = 50000.0;
      break;
    case 3: // Silver
      newcost = 60000.0;
      break;
    case 4: // Gold
      newcost = 80000.0;
      break;
    case 5: // Platinum
      newcost = 100000.0;
      break;
    case 6: // Mithril
      newcost = 135000.0;
      break;
    case 7: // Adamantium
      newcost = 175000.0;
      break;
    default:
      // Base for unknown is 1 copper.
      newcost = 1.0;
      break;
  }

  // Size of metal: 0=small, 1=medium, 2=large
  switch( (vnum-LOWEST_ORE_VNUM) % 3 )
  {
    case 0:
      // 75% cost for small.
      newcost *= .75;
      break;
    case 1:
      // 100% cost for meduim.
      newcost *= 1.0;
      break;
    case 2:
      // 150% cost for large.
      newcost *= 1.50;
      break;
    default:
      // Leave if unknown size.
      break;
  }

  // Add a little randomness for shape +/- 3%.
  newcost = newcost * number(970, 1030) / 1000.0;

  // Level/skill multipliers
  newcost *= (float)GET_LEVEL(ch) / 56.0;
  newcost *= (GET_CHAR_SKILL(ch, SKILL_MINE)) / 100.0;

  debug( "New ore: '%s' (lvl: %d, skill: %d) value: %d,%d.", ore->short_description, GET_LEVEL(ch),
    GET_CHAR_SKILL(ch, SKILL_MINE), (int)newcost/1000, (int)newcost%1000 );

  return (int)newcost;
}
