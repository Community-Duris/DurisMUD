//
//  File: editobj.cpp    originally part of durisEdit
//
//  Usage: functions for editing object types
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


#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../fh.h"
#include "../types.h"
#include "../misc/menu.h"
#include "../keys.h"

#include "../graphcon.h"

#include "object.h"

extern objectType **g_objLookup;
extern bool g_madeChanges;
extern uint g_lowestObjNumber, g_highestObjNumber, g_numbObjTypes, g_numbLookupEntries;
extern menuChoice objLimitChoiceOverrideExtraArr[];
extern menuChoice objLimitChoiceArr[];
extern menu g_objMenu, g_objNoAffMenu;



//
// displayEditObjTypeMenu : Displays edit object type menu
//
//       *obj : obj being edited
//   origVnum : used to get the proper limit/override limit even if vnum has been changed
//

void displayEditObjTypeMenu(const objectType *obj, const uint origVnum)
{
  displayMenuHeader(ENTITY_OBJECT, getObjShortName(obj), obj->objNumber, NULL, false);

  if (getObjAffectVal())
    displayMenu(&g_objMenu, obj);
  else
    displayMenu(&g_objNoAffMenu, obj);
}


//
// interpEditObjTypeMenu : Interprets user input for edit object type menu -
//                         returns TRUE if the user hits 'Q', false otherwise
//
//     ch : user input
//   *obj : object to edit
//

// Edit obj type, applies, values - needs original object vnum to properly check if it's okay
//   to change the type of a container object.
bool interpEditObjTypeMenu(const usint ch, objectType *obj, const uint origVnum)
{
  const menu *activeObjMenu;
  bool default_long = !strcmp(obj->objLongName, OBJ_DEFAULT_LONG_DESC);

  // Check to see if it's set to the generic message
  if( !default_long )
  {
    char buf[2048];
    sprintf( buf, "%s lies here.", obj->objShortName );
    CAP(buf);
    default_long = !strcmp( obj->objLongName, buf );
  }

  if( getObjAffectVal() )
    activeObjMenu = &g_objMenu;
  else
    activeObjMenu = &g_objNoAffMenu;

  if( ch == 'I' )
  {
    editObjMisc(obj, origVnum);

    displayEditObjTypeMenu(obj, origVnum);
  }
  // Edit obj weight, dam bonus, etc
  else if( ch == 'J' )
  {
    editObjMisc2(obj);

    displayEditObjTypeMenu(obj, origVnum);
  }
  // Edit obj trap info
  else if( ch == 'M' )
  {
    editObjTrapInfo(obj);

    displayEditObjTypeMenu(obj, origVnum);
  }
  // Hit menu for most
  else if( interpretMenu(activeObjMenu, obj, ch) )
  {
    // Make a quick'n easy long desc out of new short desc.
    if( ch == 'A' )
    {
      if( bleeding_ansi(obj->objShortName) )
      {
        strcat( obj->objShortName, "&n" );
      }
      // Make sure the first letter of the short description is lowercase.
      DECAP(obj->objShortName);
      if( default_long )
      {
        sprintf( obj->objLongName, "%s lies here.", obj->objShortName );
        // Make sure the first letter of the long description is uppercase.
        CAP(obj->objLongName);
      }
    }
    displayEditObjTypeMenu(obj, origVnum);

    return false;
  }
  // Change limit
  else if( ch == 'L' )
  {
    const uint numbLoad = getNumbEntities(ENTITY_OBJECT, origVnum, false);
    const uint numbOverride = getNumbEntities(ENTITY_OBJECT, origVnum, true);
    uint val = numbOverride;

    editUIntVal(&val, true, "&+CNew limit on loads for this object type (0 = number loaded): ");

    // Check user input
    if( (val <= numbLoad) && (val != 0) )
    {
      if( val < numbLoad )
      {
        displayAnyKeyPrompt("&+CAborting: Attempting to set limit lower than the number loaded - press a key");
      }
      displayEditObjTypeMenu(obj, origVnum);

      return false;
    }

    if( val != numbOverride )
    {
      setEntityOverride(ENTITY_OBJECT, origVnum, val);
      g_madeChanges = true;
    }

    displayEditObjTypeMenu(obj, origVnum);
  }
  // Change vnum
  else if( (ch == 'V') && !obj->defaultObj )
  {
    uint vnum;
    char strn[256];

    sprintf(strn, "&+CNew object vnum (highest allowed %u): ", g_numbLookupEntries - 1);

    while( true )
    {
      vnum = obj->objNumber;

      editUIntVal(&vnum, false, strn);

      if( (vnum >= g_numbLookupEntries) || (objExists(vnum) && (origVnum != vnum)) )
      {
        if( vnum != obj->objNumber )
        {
          displayAnyKeyPrompt("&+CError: vnum already exists or is too high - press a key");
        }
      }
      else
      {
        break;
      }
    }

    obj->objNumber = vnum;

    displayEditObjTypeMenu(obj, origVnum);
  }
  // Quit
  else if( checkMenuKey(ch, false) == MENUKEY_SAVE )
    return true;

  return false;
}


//
// editObjType : "main" function for edit object type menu
//
//  *obj : obj being edited
//

//
// objects and mobs edit the original and if changes are not kept, the copy is copied back into the original
// for rooms, a copy is made and then edited and if changes are kept, the copy is copied into the original
//

objectType *realEditObjType(objectType *obj, const bool allowJump)
{
  objectType *objOrigCopy;
  bool done = false;


  if (!obj) 
    return NULL;

  const uint origOverride = getEntityOverride(ENTITY_OBJECT, obj->objNumber);

 // copy obj into objOrigCopy and display the menu

  objOrigCopy = copyObjectType(obj, false);

  if (!objOrigCopy)
  {
    displayAllocError("objectType", "realEditObjType");

    return NULL;
  }

  displayEditObjTypeMenu(obj, obj->objNumber);

  while (true)
  {
    const usint ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
     // aborted, but user may have changed the object, so reset original object using settings 
     // from object copy

      deleteObjectTypeAssocLists(obj);

      memcpy(obj, objOrigCopy, sizeof(objectType));

      delete objOrigCopy;

     // update all rooms with objects, since edescs are now located elsewhere in memory

      updateAllObjMandElists();

     // reset override limit

      setEntityOverride(ENTITY_OBJECT, obj->objNumber, origOverride);

      _outtext("\n\n");

      return NULL;
    }

    if (allowJump)
    {
      if ((checkMenuKey(ch, false) == MENUKEY_NEXT) || (checkMenuKey(ch, false) == MENUKEY_PREV) ||
          (checkMenuKey(ch, false) == MENUKEY_JUMP))
      {
        done = true;
      }
    }

   // if interpEditObjTypeMenu is TRUE, user has quit

    if (!done)
      done = interpEditObjTypeMenu(ch, obj, objOrigCopy->objNumber);

    if (done)
    {
     // see if changes have been made

      if (!compareObjectType(obj, objOrigCopy))
      {
        bool passedEqCheck = true;

       // first, see if all mobs wearing eq can still wear it

        if (!mobsEqinCorrectSlot(false) || !mobsCanEquipEquipped(false))
        {
          usint eqch;


          displayColorString("\n\n&+c"
"Error - mobs that were equipping this object no longer can due to changes in\n"
"        the object's flags - &+Cr&+cevert object to its old settings, or stick all\n"
"        affected eq in mobs' &+CC&n&+carried list (&+Cr/C&+c)? &n");

          do
          {
            eqch = toupper(getkey());
          } while ((eqch != 'R') && (eqch != 'C') && (eqch != K_Enter));

          if (eqch != 'R')
          {
            _outtext("place in carried list");

            mobsEqinCorrectSlot(true);
            mobsCanEquipEquipped(true);
          }
          else 
          {
            _outtext("revert");

            deleteObjectTypeAssocLists(obj);

            memcpy(obj, objOrigCopy, sizeof(objectType));

            delete objOrigCopy;

           // reset override limit

            setEntityOverride(ENTITY_OBJECT, obj->objNumber, origOverride);

           // update all rooms with objects, since edescs are now located elsewhere in memory

            updateAllObjMandElists();

            passedEqCheck = false;
          }
        }

       // then, check if vnum has changed

        if (passedEqCheck)
        {
          if (obj->objNumber != objOrigCopy->objNumber)
          {
            const uint oldNumb = objOrigCopy->objNumber;
            const uint newNumb = obj->objNumber;

            if (g_numbObjTypes == 1) 
            {
              g_lowestObjNumber = g_highestObjNumber = newNumb;
            }
            else
            {
              if (oldNumb == g_lowestObjNumber)
                g_lowestObjNumber = getNextObj(oldNumb)->objNumber;
                
              if (newNumb < g_lowestObjNumber) 
                g_lowestObjNumber = newNumb;

              if (oldNumb == g_highestObjNumber)
                g_highestObjNumber = getPrevObj(oldNumb)->objNumber;

              if (newNumb > g_highestObjNumber) 
                g_highestObjNumber = newNumb;
            }

            resetAllObjHere(oldNumb, newNumb);
            resetNumbLoaded(ENTITY_OBJECT, oldNumb, newNumb);

            checkAndFixRefstoObj(oldNumb, newNumb);

            g_objLookup[newNumb] = obj;
            g_objLookup[oldNumb] = NULL;

           // for objHeres that used to have no type

            resetEntityPointersByNumb(true, false);
          }

         // done, delete copy

          deleteObjectType(objOrigCopy, false);

          g_madeChanges = true;

         // update all rooms with objects

          updateAllObjMandElists();

         // recreate inventory keywords for objects

          updateInvKeywordsObj(obj);
        }
      }

     // no changes made, delete copy made for editing

      else 
      {
        deleteObjectType(objOrigCopy, false);
      }

      if (allowJump)
      {
        if (checkMenuKey(ch, false) == MENUKEY_JUMP)
        {
          uint numb = obj->objNumber;

          switch (jumpObj(&numb))
          {
            case MENU_JUMP_ERROR : return obj;
            case MENU_JUMP_VALID : return findObj(numb);

            default : return NULL;
          }
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_NEXT)
        {
          if (obj->objNumber != getHighestObjNumber())
            return getNextObj(obj->objNumber);
          else
            return obj;
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_PREV)
        {
          if (obj->objNumber != getLowestObjNumber())
            return getPrevObj(obj->objNumber);
          else
            return obj;
        }
      }

      _outtext("\n\n");

      return NULL;
    }
  }
}


//
// editObjType
//

void editObjType(objectType *obj, const bool allowJump)
{
  do
  {
    objLimitChoiceOverrideExtraArr[0].offset = obj->objNumber;
    objLimitChoiceArr[0].offset = obj->objNumber;

    obj = realEditObjType(obj, allowJump);
  } while (obj);
}


//
// jumpObj : returns code based on user input
//

char jumpObj(uint *numb)
{
  char promptstrn[128];
  const struct rccoord coords = _gettextposition();

  clrline(coords.row, 7, 0);
  _settextposition(coords.row, 1);

  sprintf(promptstrn, "object vnum to jump to [%u-%u]", 
          getLowestObjNumber(), getHighestObjNumber());

  editUIntValSearchableList(numb, false, promptstrn, displayObjectTypeList);

  if ((*numb >= g_numbLookupEntries) || !objExists(*numb))
    return MENU_JUMP_ERROR;
  else
    return MENU_JUMP_VALID;
}
