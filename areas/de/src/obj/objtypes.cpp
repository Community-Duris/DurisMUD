//
//  File: objtypes.cpp   originally part of durisEdit
//
//  Usage: innumerable functions used to get info on enumerated values
//         used for objects
//

/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <string.h>

#include "../fh.h"
#include "../types.h"

#include "../spells.h"
#include "armor.h"
#include "weapons.h"
#include "traps.h"
#include "objsize.h"
#include "material.h"
#include "liquids.h"
#include "shields.h"
#include "missiles.h"
#include "totem.h"

#include "../defines.h"

extern flagDef g_armorMiscFlagDef[], g_contFlagDef[], g_totemSphereFlagDef[],
               g_shieldMiscFlagDef[], g_roomExitDirectionList[], g_objInstrTypeList[],
               g_objWeaponTypeList[], objWeaponDamageTypeList[], g_objMissileTypeList[],
               g_objLiquidTypeList[], g_objArmorThicknessList[], g_objTypeList[],
               g_objShieldTypeList[], g_objShieldShapeList[], g_objShieldSizeList[],
               g_objCraftsmanshipList[], g_objTrapDamList[];

extern "C" const char *apply_types[];
extern "C" Skill skills[MAX_AFFECT_TYPES+1];
extern "C" struct material_data materials[];

extern char *g_apply_types_low[APPLY_LAST + 1];



//
// getObjTypeStrn : Returns a descriptive string based on the object type
//
//   objType : object type
//

const char *getObjTypeStrn(const uint objType)
{
  return getFlagNameFromList(g_objTypeList, objType);
}


//
// getObjApplyStrn : Returns a descriptive string based on where the
//                   application is applied.
//
//   applyWhere : apply type
//

const char *getObjApplyStrn(const int applyWhere)
{
  if ((applyWhere < 0) || (applyWhere > APPLY_LAST))
    return "apply loc invalid";
  else
    return g_apply_types_low[applyWhere];
}


//
// getObjCraftsmanshipStrn
//

const char *getObjCraftsmanshipStrn(const uint craft)
{
  return getFlagNameFromList(g_objCraftsmanshipList, craft);
}


//
// getMaterialStrn
//

const char *getMaterialStrn(const int material)
{
  if ((material < MAT_LOWEST) || (material > MAT_HIGHEST))
    return "&+Runrecog&n";

  return materials[material].name;
}


//
// getLiqTypeStrn
//

const char *getLiqTypeStrn(const uint liquidType)
{
  return getFlagNameFromList(g_objLiquidTypeList, liquidType);
}


//
// getWeapTypeStrn
//

const char *getWeapTypeStrn(const uint weaponType)
{
  return getFlagNameFromList(g_objWeaponTypeList, weaponType);
}


//
// getArmorThicknessStrn
//

const char *getArmorThicknessStrn(const int armorThickness)
{
  return getFlagNameFromList(g_objArmorThicknessList, armorThickness);
}


//
// getShieldTypeStrn
//

const char *getShieldTypeStrn(const int shieldType)
{
  return getFlagNameFromList(g_objShieldTypeList, shieldType);
}


//
// getShieldShapeStrnShort
//

const char *getShieldShapeStrn(const int shieldShape)
{
  return getFlagNameFromList(g_objShieldShapeList, shieldShape);
}


//
// getShieldSizeStrnShort
//

const char *getShieldSizeStrn(const int shieldSize)
{
  return getFlagNameFromList(g_objShieldSizeList, shieldSize);
}


//
// getSkillTypeStrn
//

const char *getSkillTypeStrn(const int skillType)
{
  if ((skillType > LAST_SKILL) || (skillType < FIRST_SKILL))
    return "out of range";

  return skills[skillType].name;
}


//
// getSpellTypeStrn
//

const char *getSpellTypeStrn(const int spellType)
{
  if (spellType == -1) 
    return "unused spell slot";

  if ((spellType > LAST_SPELL) || (spellType < 1))
    return "out of range";

  return skills[spellType].name;
}


//
// getInstrumentTypeStrn
//

const char *getInstrumentTypeStrn(const uint instType)
{
  return getFlagNameFromList(g_objInstrTypeList, instType);
}


//
// getMissileTypeStrn
//

const char *getMissileTypeStrn(const uint missileType)
{
  return getFlagNameFromList(g_objMissileTypeList, missileType);
}


//
// getTotemSphereStrn : strn should be at least 64 bytes
//

char *getTotemSphereStrn(const uint sphere, char *strn)
{
  strn[0] = '\0';

  if (sphere & TOTEM_LESSER_ANIMAL)   strcat(strn, "&+glA&n|");
  if (sphere & TOTEM_GREATER_ANIMAL)  strcat(strn, "&+GgA&n|");
  if (sphere & TOTEM_LESSER_ELEMENT)  strcat(strn, "&+ylE&n|");
  if (sphere & TOTEM_GREATER_ELEMENT) strcat(strn, "&+YgE&n|");
  if (sphere & TOTEM_LESSER_SPIRIT)   strcat(strn, "&+LlS&n|");
  if (sphere & TOTEM_GREATER_SPIRIT)  strcat(strn, "&+WgS&n|");

  if (strn[0])
    strn[strlen(strn) - 1] = '\0';

  return strn;
}


//
// getObjTrapFlagsStrn : strn should be at least 128 bytes
//

char *getObjTrapFlagsStrn(const uint flag, char *strn)
{
  char ch = 0;

  if (!flag)
  {
    strcpy(strn, "none");
    return strn;
  }

  strn[0] = '\0';

  if (flag & TRAP_EFF_MOVE) strcat(strn, "&+WM&n|");
  if (flag & TRAP_EFF_OBJECT) strcat(strn, "&+YO&n|");
  if (flag & TRAP_EFF_ROOM) strcat(strn, "&+rR&n|");
  if (flag & TRAP_EFF_NORTH) strcat(strn, "&+yN");
  if (flag & TRAP_EFF_SOUTH) strcat(strn, "&+yS");
  if (flag & TRAP_EFF_WEST) strcat(strn, "&+yW");
  if (flag & TRAP_EFF_EAST) strcat(strn, "&+yE");
  if (flag & TRAP_EFF_UP) strcat(strn, "&+yU");
  if (flag & TRAP_EFF_DOWN) strcat(strn, "&+yD");
  if (flag & TRAP_EFF_GLYPH) strcat(strn, "&+BG");

  if (flag & TRAP_EFF_OPEN)
  {
    if (strn[0]) 
      ch = strn[strlen(strn - 1)];

    if (ch && (ch != '|')) 
      strcat(strn, "&n|");

    strcat(strn, "&+cO&n|");
  }

  if (flag & TRAP_EFF_MULTI) 
    strcat(strn, "&+GM");

  if (strn[strlen(strn) - 1] == '|')
    strn[strlen(strn) - 1] = '\0';

  return strn;
}


//
// getObjTrapDamStrn
//

const char *getObjTrapDamStrn(const int type)
{
  return getFlagNameFromList(g_objTrapDamList, type);
}


//
// getObjValueStrn : Returns a descriptive string based on objType and
//                   valueField
//
//            objType : type of object
//         valueField : value field, 0-7
//           objValue : actual value of value field
//               strn : value strns can be unique, so must return stack string - should be at least 128 bytes
// showCurrentValInfo : if true, shows current type for appropriate value fields
//

const char *getObjValueStrn(const uint objType, const uint valueField, const int objValue, char *strn,
                            const bool showCurrentValInfo)
{
  char strn2[512], valstrn[64];

  if (valueField > (NUMB_OBJ_VALS - 1)) 
    return "value field out of range";

  if ((objType > ITEM_LAST) || (objType < ITEM_LOWEST))
    return "obj type out of range";

  switch (valueField)
  {
    case 0 : switch (objType)
             {
               case ITEM_POTION : 
               case ITEM_SCROLL : 
               case ITEM_WAND   : 
               case ITEM_STAFF  : return "spell level";
               case ITEM_WEAPON : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getWeapTypeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "weapon type [%s]", valstrn);
                                  }
                                  else strcpy(strn, "weapon type");

                                  return strn;
               case ITEM_MISSILE: return "to-hit modifier";
               case ITEM_FIREWEAPON: return "range";
               case ITEM_ARMOR  : return "armor class";
               case ITEM_CONTAINER : return "max holdable weight";
               case ITEM_NOTE   : return "language";
               case ITEM_DRINKCON : return "max. drink units";
               case ITEM_FOOD   : return "number of hours filled";
               case ITEM_MONEY  : return "numb copper coins";
               case ITEM_TELEPORT : return "target room";
               case ITEM_SWITCH : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getCommandStrn(objValue, true), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "trigger command [%s]", valstrn);
                                  }
                                  else strcpy(strn, "trigger command");

                                  return strn;
               case ITEM_QUIVER : return "max capacity";
               case ITEM_PICK : return "% chance added to picking lock";
               case ITEM_INSTRUMENT : if (showCurrentValInfo)
                                      {
                                        strncpy(valstrn, getInstrumentTypeStrn(objValue), 63);
                                        valstrn[63] = '\0';

                                        sprintf(strn, "instrument type [%s]", valstrn);
                                      }
                                      else strcpy(strn, "instrument type");

                                      return strn;
               case ITEM_TOTEM : if (showCurrentValInfo)
                                 {
                                   strncpy(valstrn, getTotemSphereStrn(objValue, strn2), 63);
                                   valstrn[63] = '\0';

                                   sprintf(strn, "sphere(s) [%s]", valstrn);
                                 }
                                 else strcpy(strn, "sphere(s)");

                                 return strn;
               case ITEM_SHIELD : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getShieldTypeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "shield type [%s]", valstrn);
                                  }
                                  else strcpy(strn, "shield type");

                                  return strn;

               default : return "not used";
             }

    case 1 : switch (objType)
             {
               case ITEM_POTION :
               case ITEM_SCROLL : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getSpellTypeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "spell type [%s]", valstrn);
                                  }
                                  else strcpy(strn, "spell type");

                                  return strn;
               case ITEM_WAND   :
               case ITEM_STAFF  : return "max charges";
               case ITEM_MISSILE :
               case ITEM_WEAPON : return "number of damage dice";
               case ITEM_FIREWEAPON: return "rate of fire";
               case ITEM_CONTAINER :
               case ITEM_QUIVER : return "container flags";
               case ITEM_DRINKCON : return "number of drink units left";
               case ITEM_KEY    : return "% chance of key breaking";
               case ITEM_MONEY  : return "numb silver coins";
               case ITEM_TELEPORT : if (showCurrentValInfo)
                                    {
                                      strncpy(valstrn, getCommandStrn(objValue, true), 63);
                                      valstrn[63] = '\0';

                                      sprintf(strn, "activating command [%s]", valstrn);
                                    }
                                    else strcpy(strn, "activating command");

                                    return strn;
               case ITEM_SWITCH : return "room # with blocked exit";
               case ITEM_PICK   : return "% chance of pick breaking";
               case ITEM_INSTRUMENT : return "level of effect";
               case ITEM_TOTEM : return "ward/ward type (unused)";
               case ITEM_SHIELD : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getShieldShapeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "shield shape [%s]", valstrn);
                                  }
                                  else strcpy(strn, "shield shape");

                                  return strn;

               default : return "not used";
             }

    case 2 : switch (objType)
             {
               case ITEM_LIGHT  : return "# hours light lasts";
               case ITEM_POTION :
               case ITEM_SCROLL : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getSpellTypeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "spell type [%s]", valstrn);
                                  }
                                  else strcpy(strn, "spell type");

                                  return strn;
               case ITEM_WAND   :
               case ITEM_STAFF  : return "# charges left";
               case ITEM_MISSILE:
               case ITEM_WEAPON : return "range of damage dice";
               case ITEM_CONTAINER : return "item that opens container";
               case ITEM_DRINKCON : if (showCurrentValInfo)
                                    {
                                      strncpy(valstrn, getLiqTypeStrn(objValue), 63);
                                      valstrn[63] = '\0';

                                      sprintf(strn, "type of liquid [%s&n]", valstrn);
                                    }
                                    else strcpy(strn, "type of liquid");

                                    return strn;
               case ITEM_MONEY  : return "numb gold coins";
               case ITEM_TELEPORT : return "# charges [-1 = infinite]";
               case ITEM_SWITCH : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getExitStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "dir of blocked exit [%s]", valstrn);
                                  }
                                  else strcpy(strn, "dir of blocked exit");

                                  return strn;

               case ITEM_QUIVER : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getMissileTypeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "missile type [%s]", valstrn);
                                  }
                                  else strcpy(strn, "missile type");

                                  return strn;
               case ITEM_INSTRUMENT : return "break chance (1000max)";
               case ITEM_SPELLBOOK : return "number of pages";
               case ITEM_TOTEM : return "level of ward (unused)";
               case ITEM_SHIELD : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getShieldSizeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "shield size [%s]", valstrn);
                                  }
                                  else strcpy(strn, "shield size");

                                  return strn;

               default : return "not used";
             }

    case 3 : switch (objType)
             {
               case ITEM_POTION :
               case ITEM_WAND   :
               case ITEM_STAFF  :
               case ITEM_SCROLL : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getSpellTypeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "spell type [%s]", valstrn);
                                  }
                                  else strcpy(strn, "spell type");

                                  return strn;

               case ITEM_FIREWEAPON :
               case ITEM_MISSILE: if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getMissileTypeStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "missile type [%s]", valstrn);
                                  }
                                  else strcpy(strn, "missile type");

                                  return strn;
               case ITEM_ARMOR  : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getArmorThicknessStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "armor thickness [%s]", valstrn);
                                  }
                                  else strcpy(strn, "armor thickness");

                                  return strn;
               case ITEM_CONTAINER : return "max space capacity";
               case ITEM_DRINKCON:
               case ITEM_FOOD   : return "poisoned if non-zero";
               case ITEM_MONEY  : return "numb platinum coins";
               case ITEM_SWITCH : return "0 = wall moves, 1 = switch moves";
               case ITEM_QUIVER : return "current amount of missiles";
               case ITEM_INSTRUMENT : return "min level to use";
               case ITEM_SHIELD : return "armor class";

               default : return "not used";
             }

    case 4 : switch (objType)
             {
               case ITEM_WEAPON : return "poison type (0 = none)";
               case ITEM_ARMOR : return "misc. armor flags";
               case ITEM_POTION : return "damage caused by drinking";
               case ITEM_SHIELD : if (showCurrentValInfo)
                                  {
                                    strncpy(valstrn, getArmorThicknessStrn(objValue), 63);
                                    valstrn[63] = '\0';

                                    sprintf(strn, "shield thickness [%s]", valstrn);
                                  }
                                  else strcpy(strn, "shield thickness");

                                  return strn;

               default : return "not used";
             }

    case 5 : switch (objType)
             {
               case ITEM_SHIELD : return "misc. shield flags";

               default : return "not used";
             }

    case 6 :
    case 7 : return "not used";

    default : return "unrecognized value field";
  }
}


//
// checkForValueList : checks to see if this object type and field can have
//                     a list available of types
//

bool checkForValueList(const uint objType, const uint valueField)
{
  switch (valueField)
  {
    case 0 : switch (objType)
             {
               case ITEM_TELEPORT :
               case ITEM_SWITCH : return true;

               default : return false;
             }

    case 1 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_POTION :
               case ITEM_TELEPORT :
               case ITEM_SWITCH : return true;

               default : return false;
             }

    case 2 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_POTION :
               case ITEM_CONTAINER :
               case ITEM_QUIVER : return true;

               default : return false;
             }

    case 3 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_WAND   :
               case ITEM_STAFF  :
               case ITEM_POTION : return true;

               default : return false;
             }

    default : return false;
  }
}


//
// checkForVerboseAvail : returns TRUE if an object field has not only help,
//                        but verbose help available
//

bool checkForVerboseAvail(const uint objType, const uint valueField)
{
  switch (valueField)
  {
    case 0 : switch (objType)
             {
               case ITEM_SWITCH : return true;

               default : return false;
             }

    case 1 : switch (objType)
             {
               case ITEM_TELEPORT : return true;

               default : return false;
             }

    default : return false;
  }
}


//
// checkForSearchAvail : Returns TRUE if you can search by substring, FALSE otherwise
//

bool checkForSearchAvail(const uint objType, const uint valueField)
{
  switch (valueField)
  {
    case 0 : switch (objType)
             {
               case ITEM_TELEPORT :   // target room
               case ITEM_SWITCH : return true;    // activating command

               default : return false;
             }

    case 1 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_POTION :    // spell type
               case ITEM_TELEPORT :   // activating command
               case ITEM_SWITCH : return true;    // target room

               default : return false;
             }

    case 2 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_POTION :      // spell type
               case ITEM_CONTAINER : return true;  // item that opens container

               default : return false;
             }

    case 3 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_WAND   :
               case ITEM_STAFF  :
               case ITEM_POTION : return true;   // ditto

               default : return false;
             }

    default : return false;
  }
}


//
// searchObjValue : Corresponds to above, executes the search
//

void searchObjValue(const uint objType, const uint valueField)
{
  char strn[128] = "";


  editStrnVal(strn, 127, "&+CEnter search string: ");

  if (strn[0] == '\0')
    return;

  switch (valueField)
  {
    case 0 : switch (objType)
             {
               case ITEM_TELEPORT : displayRoomList(strn);  // target room
                                    return;
               case ITEM_SWITCH : displayCommandList(strn); // command
                                  return;

               default : return;  // shouldn't happen ..
             }

    case 1 : switch (objType)
             {
               case ITEM_SCROLL :   // spell type
               case ITEM_POTION : displaySpellList(strn);
                                  return;
               case ITEM_TELEPORT : displayCommandList(strn);  // command
                                    return;
               case ITEM_SWITCH : displayRoomList(strn);    // target room
                                  return;

               default : return;
             }

    case 2 : switch (objType)
             {
               case ITEM_SCROLL :   // spell type
               case ITEM_POTION : displaySpellList(strn);
                                  return;
               case ITEM_CONTAINER : displayObjectTypeList(strn, true);  // item that opens container
                                     return;

               default : return;
             }

    case 3 : switch (objType)
             {
               case ITEM_SCROLL :   // spell type
               case ITEM_WAND   :
               case ITEM_STAFF  :
               case ITEM_POTION : displaySpellList(strn);
                                  return;

               default : return;
             }

    default : return;
  }
}


//
// displayObjValueHelp : displays "help" on object values depending on obj type
//

void displayObjValueHelp(const uint objType, const uint valueField, const bool verbose)
{
  switch (valueField)
  {
    case 0 : switch (objType)
             {
               case ITEM_TELEPORT : displayRoomList("");  return;
               case ITEM_SWITCH : displayCommandList(verbose);  return;

               default : return;
             }

    case 1 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_POTION : displaySpellList(NULL);  return;
               case ITEM_TELEPORT : displayCommandList(verbose);  return;
               case ITEM_SWITCH : displayRoomList("");  return;

               default : return;
             }

    case 2 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_POTION : displaySpellList(NULL);  return;
               case ITEM_CONTAINER : displayObjectTypeList(NULL, false);  return;

               default : return;
             }

    case 3 : switch (objType)
             {
               case ITEM_SCROLL :
               case ITEM_POTION :
               case ITEM_WAND   :
               case ITEM_STAFF  : displaySpellList(NULL);  return;

               default : return;
             }

    default : return;
  }
}


//
// fieldRefsObjNumb : returns true if object type and value field reference obj vnum
//

bool fieldRefsObjNumb(const uint objType, const uint valueField)
{
  switch (valueField)
  {
    case 2 : switch (objType)
             {
               case ITEM_CONTAINER : return true;

               default : return false;
             }

    default : return false;
  }
}


//
// fieldRefsRoomNumb : returns true if object type and value field reference room vnum
//

bool fieldRefsRoomNumb(const uint objType, const uint valueField)
{
  switch (valueField)
  {
    case 0 : switch (objType)
             {
               case ITEM_TELEPORT : return true;

               default : return false;
             }

    case 1 : switch (objType)
             {
               case ITEM_SWITCH : return true;

               default : return false;
             }

    default : return false;
  }
}


//
// specialObjValEditRedundant : redundant code when editing list/flag obj values
//

void specialObjValEditRedundant(objectType *obj, const uchar valueField, const flagDef *flagArr, 
                                const char *bitvName, const bool asBitV)
{
  editFlags(flagArr, &(obj->objValues[valueField]), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber, 
            bitvName, NULL, 0, asBitV);
}


//
// specialObjValEdit : edits special values (liquid type, weapon type, etc) if editing is FALSE; if
//                     editing is TRUE, returns true if field is special
//

bool specialObjValEdit(objectType *obj, const uchar valueField, const bool editing)
{
  uint objType;

  if (!obj)
    return false;

  objType = obj->objType;

  switch (objType)
  {
    case ITEM_DRINKCON :
      if (valueField == 2)
      {
        if (editing)
          specialObjValEditRedundant(obj, 2, g_objLiquidTypeList, "liquid type", false);

        return true;
      }

      return false;

    case ITEM_WEAPON    :
      if (valueField == 0)
      {
        if (editing)
          specialObjValEditRedundant(obj, 0, g_objWeaponTypeList, "weapon type", false);

        return true;
      }

      return false;

    case ITEM_MISSILE   :
    case ITEM_FIREWEAPON:
      if (valueField == 3)
      {
        if (editing)
          specialObjValEditRedundant(obj, 3, g_objMissileTypeList, "missile type", false);

        return true;
      }

      return false;

    case ITEM_ARMOR     :
      if (valueField == 3)
      {
        if (editing) 
          specialObjValEditRedundant(obj, 3, g_objArmorThicknessList, "armor thickness", false);

        return true;
      }
      else
      if (valueField == 4)
      {
        if (editing)
          specialObjValEditRedundant(obj, 4, g_armorMiscFlagDef, "misc armor", true);

        return true;
      }

      return false;

    case ITEM_SHIELD :
      if (valueField == 0)
      {
        if (editing)
          specialObjValEditRedundant(obj, 0, g_objShieldTypeList, "shield type", false);

        return true;
      }
      else
      if (valueField == 1)
      {
        if (editing)
          specialObjValEditRedundant(obj, 1, g_objShieldShapeList, "shield shape", false);

        return true;
      }
      else
      if (valueField == 2)
      {
        if (editing)
          specialObjValEditRedundant(obj, 2, g_objShieldSizeList, "shield size", false);

        return true;
      }
      else
      if (valueField == 4)
      {
        if (editing)
          specialObjValEditRedundant(obj, 4, g_objArmorThicknessList, "shield thickness", false);

        return true;
      }
      else
      if (valueField == 5)
      {
        if (editing)
          specialObjValEditRedundant(obj, 5, g_shieldMiscFlagDef, "misc shield", true);

        return true;
      }

      return false;

    case ITEM_SWITCH :
      if (valueField == 2)
      {
        if (editing)
          specialObjValEditRedundant(obj, 2, g_roomExitDirectionList, "switch direction", false);

        return true;
      }

      return false;

    case ITEM_QUIVER    :
    case ITEM_CONTAINER :
      if (valueField == 1)
      {
        if (editing)
          specialObjValEditRedundant(obj, 1, g_contFlagDef, "container", true);

        return true;
      }
      else
      if ((valueField == 2) && (objType == ITEM_QUIVER))
      {
        if (editing)
          specialObjValEditRedundant(obj, 2, g_objMissileTypeList, "missile type", false);

        return true;
      }

      return false;

    case ITEM_TOTEM :
      if (valueField == 0)
      {
        if (editing)
          specialObjValEditRedundant(obj, 0, g_totemSphereFlagDef, "totem sphere", true);

        return true;
      }

      return false;

    case ITEM_INSTRUMENT :
      if (valueField == 0)
      {
        if (editing)
          specialObjValEditRedundant(obj, 0, g_objInstrTypeList, "instrument type", false);

        return true;
      }

      return false;

    default : return false;
  }
}
