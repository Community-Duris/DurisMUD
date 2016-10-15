//
//  File: editmob.cpp    originally part of durisEdit
//
//  Usage: user-interface functions for editing mobs
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
#include <ctype.h>
#include <stdlib.h>

#include "../fh.h"
#include "../types.h"
#include "../misc/menu.h"
#include "../keys.h"
#include "../graphcon.h"

#include "mob.h"

extern bool g_madeChanges;
extern uint g_numbLookupEntries, g_lowestMobNumber, g_highestMobNumber, g_numbMobTypes;
extern mobType **g_mobLookup;
extern menuChoice mobLimitChoiceExtraOverrideLimitArr[];
extern menuChoice mobLimitChoiceArr[];
extern menu g_mobMenu;
extern flagDef g_mobSpecList[];

//
// displayEditMobTypeMenu : Displays edit mob type menu
//
//   *mob : mob being edited
//

void displayEditMobTypeMenu(const mobType *mob, const uint origVnum)
{
  displayMenuHeader(ENTITY_MOB, getMobShortName(mob), mob->mobNumber, NULL, false);

  displayMenu(&g_mobMenu, mob);
}


//
// interpEditMobTypeMenu : Interprets user input for edit mob type menu -
//                         returns TRUE if the user hits 'Q', FALSE otherwise
//
//     ch : user input
//   *mob : mob to edit
// *origMob : used to facilitate checking if changing mob info would make eq invalid
//

bool interpEditMobTypeMenu(const usint ch, mobType *mob, const uint origVnum)
{
 // edit mob species, hometown, alignment, class

  if (ch == 'M')
  {
    editMobMisc(mob);

    updateSpecList(mob);
    displayEditMobTypeMenu(mob, origVnum);
  }
  else

 // edit mob level, position, default pos, sex

  if (ch == 'N')
  {
    editMobMisc2(mob);

    displayEditMobTypeMenu(mob, origVnum);
  }
  else

 // edit mob money, thac0, ac, hit points, damage

  if (ch == 'O')
  {
    editMobMisc3(mob);

    displayEditMobTypeMenu(mob, origVnum);
  }
  else

 // edit quest

  if (ch == 'Q')
  {
    if (!mob->questPtr)
    {
      if (displayYesNoPrompt("&+cThis mob currently has no quest information - create some", promptYes, true) 
              == promptNo)
      {
        displayEditMobTypeMenu(mob, origVnum);

        return false;
      }

      mob->questPtr = createQuest();
    }

    editQuest(mob, true);

    displayEditMobTypeMenu(mob, origVnum);
  }
  else

 // edit shop

  if (ch == 'S')
  {
    if (!mob->shopPtr)
    {
      if (getNumbEntities(ENTITY_MOB, mob->mobNumber, false) > 1)
      {
        displayAnyKeyPrompt(
"&+CThis mob has no shop, but more than one is loaded and there can be only one.");

        displayEditMobTypeMenu(mob, origVnum);

        return false;
      }

      if (addingShopMakesTwoShopsInRoom(mob->mobNumber))
      {
        displayAnyKeyPrompt(
"&+CThis mob has no shop, but adding one would cause two or more shops in a room.");

        displayEditMobTypeMenu(mob, origVnum);

        return false;
      }

      if (displayYesNoPrompt("&+cThis mob currently has no shop - create one", promptYes, true) == promptNo)
      {
        displayEditMobTypeMenu(mob, origVnum);

        return false;
      }

      mob->shopPtr = createShop();
    }

    editShop(mob, true);

    displayEditMobTypeMenu(mob, origVnum);
  }
  else

 // hit menu for most

  if (interpretMenu(&g_mobMenu, mob, ch))
  {
    displayEditMobTypeMenu(mob, origVnum);

    return false;
  }
  else

 // change limit

  if (ch == 'L')
  {
    const uint numbLoad = getNumbEntities(ENTITY_MOB, origVnum, false);
    const uint numbOverride = getNumbEntities(ENTITY_MOB, origVnum, true);
    uint val = numbOverride;

    editUIntVal(&val, true, "&+CNew limit on loads for this mob type (0 = number loaded): ");

   // check user input

    if ((val <= numbLoad) && (val != 0))
    {
      if (val < numbLoad)
      {
        displayAnyKeyPrompt("&+CError: Attempting to set limit lower than the number loaded - press a key");
      }

      displayEditMobTypeMenu(mob, origVnum);

      return false;
    }

    if (val != numbOverride)
    {
      setEntityOverride(ENTITY_MOB, origVnum, val);
      g_madeChanges = true;
    }

    displayEditMobTypeMenu(mob, origVnum);
  }
  else

 // change vnum

  if ((ch == 'V') && !mob->defaultMob)
  {
    uint vnum;
    char strn[256];


    sprintf(strn, "&+CNew vnum (highest allowed %u): ", g_numbLookupEntries - 1);

    while (true)
    {
      vnum = mob->mobNumber;

      editUIntVal(&vnum, false, strn);

     // check user input

      if ((vnum >= g_numbLookupEntries) || (mobExists(vnum) && (vnum != origVnum)))
      {
        if (vnum != mob->mobNumber)
        {
          displayAnyKeyPrompt("&+CError: vnum already exists or is too high - press a key");
        }
      }
      else
      {
        break;
      }
    }

    mob->mobNumber = vnum;

    displayEditMobTypeMenu(mob, origVnum);
  }
  else

 // quit

  if (checkMenuKey(ch, false) == MENUKEY_SAVE) 
    return true;

  return false;
}


//
// realEditMobType : "main" function for edit mob type menu
//
//  *mob : mob being edited
//

//
// objects and mobs edit the original and if changes are not kept, the copy is copied back into the original
// for rooms, a copy is made and then edited and if changes are kept, the copy is copied into the original
//

mobType *realEditMobType(mobType *mob, const bool allowJump)
{
  mobType *mobOrigCopy;
  bool done = false;


  if (!mob) 
    return NULL;

  const uint origOverride = getEntityOverride(ENTITY_MOB, mob->mobNumber);

 // copy mob into mobOrigCopy and display the menu

  mobOrigCopy = copyMobType(mob, false);

  if (!mobOrigCopy)
  {
    displayAllocError("mobType", "realEditMobType");

    return NULL;
  }
  
  updateSpecList(mob);
  displayEditMobTypeMenu(mob, mob->mobNumber);

  while (true)
  {
    const usint ch = toupper(getkey());

    if (checkMenuKey(ch, false) == MENUKEY_ABORT)
    {
     // aborted, but user may have changed the mob, so reset original mob using settings 
     // from mob copy

      deleteMobTypeAssocLists(mob);

      memcpy(mob, mobOrigCopy, sizeof(mobType));

      delete mobOrigCopy;

     // update all rooms with mobs, since descs are now located elsewhere in memory

      updateAllMobMandElists();

     // reset override limit

      setEntityOverride(ENTITY_MOB, mob->mobNumber, origOverride);

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

   // if interpEditMobTypeMenu is TRUE, user has quit

    if (!done) 
      done = interpEditMobTypeMenu(ch, mob, mobOrigCopy->mobNumber);

    if (done)
    {
     // see if changes have been made

      if (!compareMobType(mob, mobOrigCopy))
      {
        bool passedEqCheck = true;

       // first, see if all mobs wearing eq can still wear it

        if (!mobsCanEquipEquipped(false))
        {
          usint eqch;

          displayColorString("\n\n&+c"
"Error - mobs that were equipping objects no longer can due to changes in the\n"
"        mob's flags - &+Cr&+cevert mob to its old settings, or stick all affected\n"
"        eq in mobs' &+CC&+carried list (&+Cr/C&+c)? &n");

          do
          {
            eqch = toupper(getkey());
          } while ((eqch != 'R') && (eqch != 'C') && (eqch != K_Enter));

          if (eqch != 'R')
          {
            _outtext("place in carried list");

            mobsCanEquipEquipped(true);
          }
          else
          {
            _outtext("revert");

            deleteMobTypeAssocLists(mob);

            memcpy(mob, mobOrigCopy, sizeof(mobType));

            delete mobOrigCopy;

           // reset override limit

            setEntityOverride(ENTITY_MOB, mob->mobNumber, origOverride);

           // update all rooms with mobs, since descs are now located elsewhere in memory

            updateAllMobMandElists();

            passedEqCheck = false;
          }
        }

       // then, check if vnum has changed

        if (passedEqCheck)
        {
          if (mob->mobNumber != mobOrigCopy->mobNumber)
          {
            const uint oldNumb = mobOrigCopy->mobNumber;
            const uint newNumb = mob->mobNumber;

            if (g_numbMobTypes == 1) 
            {
              g_lowestMobNumber = g_highestMobNumber = newNumb;
            }
            else
            {
              if (oldNumb == g_lowestMobNumber)
                g_lowestMobNumber = getNextMob(oldNumb)->mobNumber;
                
              if (newNumb < g_lowestMobNumber)   
                g_lowestMobNumber = newNumb;

              if (oldNumb == g_highestMobNumber)
                g_highestMobNumber = getPrevMob(oldNumb)->mobNumber;

              if (newNumb > g_highestMobNumber) 
                g_highestMobNumber = newNumb;
            }

            resetMobHere(oldNumb, newNumb);
            resetNumbLoaded(ENTITY_MOB, oldNumb, newNumb);

            g_mobLookup[newNumb] = mob;
            g_mobLookup[oldNumb] = NULL;

           // for mobHeres that used to have no type

            resetEntityPointersByNumb(false, true);
          }

         // done, delete copy

          deleteMobType(mobOrigCopy, false);

          g_madeChanges = true;

         // update rooms with mobs

          updateAllMobMandElists();

         // recreate inventory keywords for mobs

          updateInvKeywordsMob(mob);
        }
      }

     // no changes made, delete copy made for editing

      else 
      {
        deleteMobType(mobOrigCopy, false);
      }

      if (allowJump)
      {
        if (checkMenuKey(ch, false) == MENUKEY_JUMP)
        {
          uint numb = mob->mobNumber;

          switch (jumpMob(&numb))
          {
            case MENU_JUMP_ERROR : return mob;
            case MENU_JUMP_VALID : return findMob(numb);

            default : return NULL;
          }
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_NEXT)
        {
          if (mob->mobNumber != getHighestMobNumber())
            return getNextMob(mob->mobNumber);
          else
            return mob;
        }
        else
        if (checkMenuKey(ch, false) == MENUKEY_PREV)
        {
          if (mob->mobNumber != getLowestMobNumber())
            return getPrevMob(mob->mobNumber);
          else
            return mob;
        }
      }

      _outtext("\n\n");

      return NULL;
    }
  }
}


//
// editMobType
//

void editMobType(mobType *mob, const bool allowJump)
{
  do
  {
    mobLimitChoiceExtraOverrideLimitArr[0].offset = mob->mobNumber;
    mobLimitChoiceArr[0].offset = mob->mobNumber;

    mob = realEditMobType(mob, allowJump);
  } while (mob);
}


//
// jumpMob : returns code based on what user selects
//

char jumpMob(uint *numb)
{
  char promptstrn[128];
  const struct rccoord coords = _gettextposition();

  clrline(coords.row, 7, 0);
  _settextposition(coords.row, 1);

  sprintf(promptstrn, "mob vnum to jump to [%u-%u]", 
          getLowestMobNumber(), getHighestMobNumber());

  editUIntValSearchableList(numb, false, promptstrn, displayMobTypeList);

  if ((*numb >= g_numbLookupEntries) || !mobExists(*numb))
    return MENU_JUMP_ERROR;
  else
    return MENU_JUMP_VALID;
}
