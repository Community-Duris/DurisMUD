//
//  File: inv.cpp        originally part of durisEdit
//
//  Usage: general inventory functions
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
#include <stdio.h>
#include <stdlib.h>

#include "fh.h"
#include "types.h"
#include "misc/editable.h"
#include "misc/master.h"
#include "obj/objhere.h"
#include "mob/mobhere.h"
#include "misc/master.h"
#include "readfile.h"
#include "inv.h"  // constant(s) for inventory file

extern editableListNode *g_inventory;
extern bool g_readFromSubdirs;
extern bool g_madeChanges;


//
// showInventory : Shows yer inventory
//

void showInventory(void)
{
  const editableListNode *editNode = g_inventory;
  const objectHere *objHere;
  const mobHere *mob;
  char strn[1536], keystrn[1024];
  size_t numbLines = 0;


  if (checkPause("\n\n&+MYou are carrying -&n\n\n", numbLines))
    return;

  if (editNode)
  {
    while (editNode)
    {
      switch (editNode->entityType)
      {
        case ENTITY_R_EDESC :
        case ENTITY_O_EDESC :
          sprintf(strn, "extra desc '%s'\n", 
                  getReadableKeywordStrn(((extraDesc *)(editNode->entityPtr))->keywordListHead, keystrn, 1023));

          if (checkPause(strn, numbLines))
            return;

          break;

        case ENTITY_OBJECT  :
          objHere = (objectHere *)(editNode->entityPtr);

          getObjShortNameDisplayStrn(strn, objHere, 0, true);

          if (checkPause(strn, numbLines))
            return;

         // display any objects inside container

          if (getShowObjContentsVal() && displayContainerContents(objHere->objInside, 2, numbLines))
            return;

          break;

        case ENTITY_MOB     :
          mob = (mobHere *)(editNode->entityPtr);

          if (mob->mobPtr)
            getReadableKeywordStrn(mob->mobPtr->keywordListHead, keystrn, 1023);
          else
            strcpy(keystrn, "keywords unknown");

          sprintf(strn, "%s&n (#%u) (%s) &+c(%u%%)&n\n",
                  getMobShortName(mob->mobPtr), mob->mobNumb, keystrn, mob->randomChance);

          if (checkPause(strn, numbLines))
            return;

         // display mob equipment

          if (getShowObjContentsVal() && displayMobEquip(mob, 2, numbLines))
            return;

         // display mob inventory

          if (mob->inventoryHead && getShowObjContentsVal())
          {
            if (checkPause("\nmob is carrying:\n", numbLines) ||
                displayContainerContents(mob->inventoryHead, 2, numbLines) ||
                checkPause("[[[ end mob inventory ]]]\n\n", numbLines))
              return;
          }

          break;
      }

      editNode = editNode->Next;
    }
  }
  else 
  {
    _outtext("nothing\n");
  }

  _outtext("\n");
}


//
// readInventoryFile :  read user's inventory from <mainzonename>.inv
//

void readInventoryFile(void)
{
  FILE *invFile;
  char invFilename[512] = "";
  char strn[640];
  uint itemReading = 1, intMaxInvEntities = INV_START_NUMB_ENTITIES;
  void **entityPtrs;  // two separate arrays because it's more efficient, also i don't want to make a struct
  char *entityTypes;
  uint numbEntities = 0;


 // assemble the filename of the inventory file

  if (g_readFromSubdirs) 
    strcpy(invFilename, "inv/");

  strcat(invFilename, getMainZoneNameStrn());

  strcat(invFilename, ".inv");

 // try to open the file

  invFile = fopen(invFilename, "rt");

 // if it doesn't exist, there's no inventory to read

  if (!invFile)
    return;

  _outtext("Reading inventory from ");
  _outtext(invFilename);
  _outtext("...\n");

 // read header line, check for validity

  if (!readAreaFileLine(invFile, strn, 512, ENTITY_INV_LINE, itemReading, ENTITY_TYPE_UNUSED, 
                          ENTITY_NUMB_UNUSED, "inventory header", 0, NULL, false, true))
    exit(1);

  if (!strleft(strn, INV_FILE_HEADER))
  {
    displayAnyKeyPromptNoClr("\nerror: inventory file is an invalid format - skipping\n");

    fclose(invFile);

    g_madeChanges = true;

    return;
  }

 // 5th arg is version

  char versionStrn[256];

  getArg(strn, 5, versionStrn, 255);

  if (strcmp(versionStrn, INV_FILE_HEADER_VERSION) && strcmp(versionStrn, INV_FILE_HEADER_VERSION1))
  {
    _outtext("\n"
"error: inventory file is an unrecognized version (must be v1 or v2, instead is\n");
    _outtext(versionStrn);
    displayAnyKeyPromptNoClr(") - skipping\n");

    fclose(invFile);

    g_madeChanges = true;

    return;
  }

 // allocate arrays for mob/obj references

  entityPtrs = new void*[intMaxInvEntities];
  entityTypes = new char[intMaxInvEntities];

  if (!entityPtrs || !entityTypes)
  {
    displayAllocError("void*[]/char[]", "readInventoryFile");

    if (entityPtrs)
      delete[] entityPtrs;

    if (entityTypes)
      delete[] entityTypes;

    fclose(invFile);

    return;
  }

 // read commands

  while (true)
  {
   // ensure user hasn't read too many entries - adjust as necessary

    if (numbEntities >= intMaxInvEntities)
    {
     // wow

      if (intMaxInvEntities >= (0xffffffff / 2))
      {
        displayAnyKeyPromptNoClr("\nYour inventory file is too big.  Skipping the rest.\n\n");

        delete[] entityPtrs;
        delete[] entityTypes;

        fclose(invFile);

        g_madeChanges = true;

        return;
      }

      const uint oldMax = intMaxInvEntities;
      intMaxInvEntities *= 2;

      void **newEntityPtrs = new void*[intMaxInvEntities];
      char *newEntityTypes = new char[intMaxInvEntities];

      if (!newEntityPtrs || !newEntityTypes)
      {
        displayAllocError("void*[]/char[]", "readInventoryFile");

        delete[] entityPtrs;
        delete[] entityTypes;

        if (newEntityPtrs)
          delete[] newEntityPtrs;

        if (newEntityTypes)
          delete[] newEntityTypes;

        fclose(invFile);

        g_madeChanges = true;

        return;
      }

      memcpy(newEntityPtrs, entityPtrs, oldMax * sizeof(void *));
      memcpy(newEntityTypes, entityTypes, oldMax * sizeof(char));

      delete[] entityPtrs;
      delete[] entityTypes;

      entityPtrs = newEntityPtrs;
      entityTypes = newEntityTypes;
    }

   // read command type

    if (!readAreaFileLine(invFile, strn, 512, ENTITY_INV_LINE, itemReading, ENTITY_TYPE_UNUSED, 
                          ENTITY_NUMB_UNUSED, "inventory command", 1, NULL, false, true))
      exit(1);

   // M = mob

    if (strcmpnocase(strn, "M"))
    {
     // read info from file

      if (!readAreaFileLine(invFile, strn, 512, ENTITY_INV_LINE, itemReading, ENTITY_TYPE_UNUSED, 
                            ENTITY_NUMB_UNUSED, "mob command", 2, NULL, false, true))
        exit(1);

     // create mob

      mobHere *mob = new mobHere;

     // couldn't alloc

      if (!mob)
      {
        displayAllocError("mob", "readInventoryFile");

        g_madeChanges = true;

        break;
      }

      memset(mob, 0, sizeof(mobHere));

     // get number and rand %

      sscanf(strn, "%u %u", &(mob->mobNumb), &(mob->randomChance));

      mob->mobPtr = findMob(mob->mobNumb);

     // create node for inventory and add to inventory

      editableListNode *edit;
      
      if (mob->mobPtr)
        edit = createEditableListNode(mob->mobPtr->keywordListHead, mob, ENTITY_MOB);
      else
        edit = createEditableListNode(NULL, mob, ENTITY_MOB);

      if (!edit)
      {
        deleteMobHere(mob, false);

        displayAllocError("editableListNode", "readInventoryFile");

        g_madeChanges = true;

        break;
      }

      addEditabletoList(&g_inventory, edit);

     // update reference tables for possible later commands

      entityPtrs[numbEntities] = mob;
      entityTypes[numbEntities] = ENTITY_MOB;

      numbEntities++;
    }
    else

   // O = object

    if (strcmpnocase(strn, "O"))
    {
     // read info from file

      if (!readAreaFileLine(invFile, strn, 512, ENTITY_INV_LINE, itemReading, ENTITY_TYPE_UNUSED, 
                            ENTITY_NUMB_UNUSED, "object command", 2, NULL, false, true))
        exit(1);

     // create object

      objectHere *obj = new objectHere;

     // couldn't alloc

      if (!obj)
      {
        displayAllocError("obj", "readInventoryFile");

        g_madeChanges = true;

        break;
      }

      memset(obj, 0, sizeof(objectHere));

     // get number and rand %

      sscanf(strn, "%u %u", &(obj->objectNumb), &(obj->randomChance));

      obj->objectPtr = findObj(obj->objectNumb);

     // create node for inventory and add to inventory

      editableListNode *edit;
      
      if (obj->objectPtr)
        edit = createEditableListNode(obj->objectPtr->keywordListHead, obj, ENTITY_OBJECT);
      else
        edit = createEditableListNode(NULL, obj, ENTITY_OBJECT);

      if (!edit)
      {
        deleteObjHere(obj, false);

        displayAllocError("editableListNode", "readInventoryFile");

        g_madeChanges = true;

        break;
      }

      addEditabletoList(&g_inventory, edit);

     // update reference tables for possible later commands

      entityPtrs[numbEntities] = obj;
      entityTypes[numbEntities] = ENTITY_OBJECT;

      numbEntities++;
    }
    else

   // P = put object in object or on mob

    if (strcmpnocase(strn, "P"))
    {
     // read info from file

      if (!readAreaFileLine(invFile, strn, 512, ENTITY_INV_LINE, itemReading, ENTITY_TYPE_UNUSED, 
                            ENTITY_NUMB_UNUSED, "put command", 3, NULL, false, true))
        exit(1);

     // create object

      objectHere *obj = new objectHere;

     // couldn't alloc

      if (!obj)
      {
        displayAllocError("obj", "readInventoryFile");

        g_madeChanges = true;

        break;
      }

      memset(obj, 0, sizeof(objectHere));

     // get number, rand %, and container obj/mob

      uint intContainerID;

      sscanf(strn, "%u %u %u", &(obj->objectNumb), &(obj->randomChance), &intContainerID);

     // check container ID validity - and don't allow command to place object inside itself

      if (intContainerID >= numbEntities)
      {
        _outtext("\nerror: command is invalid - '");
        _outtext(strn);
        displayAnyKeyPromptNoClr("' - skipping\n\n");

        delete obj;

        g_madeChanges = true;

        break;
      }

      obj->objectPtr = findObj(obj->objectNumb);

     // update reference tables for possible later commands

      entityPtrs[numbEntities] = obj;
      entityTypes[numbEntities] = ENTITY_OBJECT;

      numbEntities++;

     // add to either mob or object

      if (entityTypes[intContainerID] == ENTITY_OBJECT)
      {
        addObjHeretoList(&(((objectHere *)(entityPtrs[intContainerID]))->objInside), obj, false);
      }

     // must be mob

      else 
      {
        addObjHeretoList(&(((mobHere *)(entityPtrs[intContainerID]))->inventoryHead), obj, false);
      }
    }
    else

   // E = equip mob

    if (strcmpnocase(strn, "E"))
    {
     // read info from file

      if (!readAreaFileLine(invFile, strn, 512, ENTITY_INV_LINE, itemReading, ENTITY_TYPE_UNUSED, 
                            ENTITY_NUMB_UNUSED, "put command", 4, NULL, false, true))
        exit(1);

     // create object

      objectHere *obj = new objectHere;

     // couldn't alloc

      if (!obj)
      {
        displayAllocError("obj", "readInventoryFile");

        g_madeChanges = true;

        break;
      }

      memset(obj, 0, sizeof(objectHere));

     // get number, rand %, equipping mob, and position

      uint intMobID;
      uint eqPos;

      sscanf(strn, "%u %u %u %u", &(obj->objectNumb), &(obj->randomChance), &intMobID, &eqPos);

     // check container ID & equip pos validity

      bool blnError = false;

      if ((intMobID >= numbEntities) || (entityTypes[intMobID] != ENTITY_MOB) || (eqPos > CUR_MAX_WEAR))
      {
        blnError = true;
      }

      mobHere *mob;

     // if no error so far, check more stuff

      if (!blnError)
      {
        mob = (mobHere *)(entityPtrs[intMobID]);
        obj->objectPtr = findObj(obj->objectNumb);

        eqPos = getMobHereEquipSlot(mob, obj->objectPtr, eqPos);

       // did it work???!!?

        if (eqPos > WEAR_TRUEHIGH)
          blnError = true;
      }

      if (blnError)
      {
        _outtext("\nerror: command is invalid - '");
        _outtext(strn);
        displayAnyKeyPromptNoClr("' - skipping\n\n");

        delete obj;

        g_madeChanges = true;

        break;
      }

     // it worked

      mob->equipment[eqPos] = obj;

     // update reference tables for possible later commands

      entityPtrs[numbEntities] = obj;
      entityTypes[numbEntities] = ENTITY_OBJECT;

      numbEntities++;
    }
    else

   // D = extra desc

    if (strcmpnocase(strn, "D"))
    {
      extraDesc *desc = readExtraDescFromFile(invFile, ENTITY_INV_LINE, itemReading, ENTITY_R_EDESC, NULL);

     // couldn't alloc

      if (!desc)
      {
        displayAllocError("extraDesc", "readInventoryFile");

        g_madeChanges = true;

        continue;
      }

      editableListNode *edit = createEditableListNode(desc->keywordListHead, desc, ENTITY_R_EDESC);

      if (!edit)
      {
        deleteOneExtraDesc(desc);

        displayAllocError("editableListNode", "readInventoryFile");

        g_madeChanges = true;

        continue;
      }

      addEditabletoList(&g_inventory, edit);
    }
    else

   // S = end of file

    if (strcmpnocase(strn, "S"))
    {
      break;
    }
    else

   // invalid

    {
      _outtext("\nunrecognized command in inventory file - '");
      _outtext(strn);
      _outtext("'\n");

      fclose(invFile);

      exit(1);
    }

    itemReading++;
  }

  delete[] entityPtrs;
  delete[] entityTypes;

  fclose(invFile);
}


//
// writeInventoryFile : write user's inventory out to <mainzonename>.inv
//

void writeInventoryFile(void)
{
  FILE *invFile;
  char invFilename[512] = "";


 // assemble the filename of the inventory file

  if (g_readFromSubdirs) 
    strcpy(invFilename, "inv/");

  strcat(invFilename, getMainZoneNameStrn());

  strcat(invFilename, ".inv");

 // if there is no inventory, delete any existing file and exit

  if (!g_inventory)
  {
    deleteFile(invFilename);

    return;
  }
  else
  {
    _outtext("Writing inventory to ");
    _outtext(invFilename);
    _outtext("...\n");
  }

 // create inventory file

  invFile = fopen(invFilename, "wt");

  if (!invFile)
  {
    _outtext("Couldn't open ");
    _outtext(invFilename);
    _outtext(" for writing - aborting\n");

    return;
  }

 // write header

  fputs(INV_FILE_HEADER INV_FILE_HEADER_VERSION "\n\n", invFile);

 // run through inventory

  const editableListNode *inv = g_inventory;
  const objectHere *objH;
  const mobHere *mobH;
  const extraDesc *desc;
  uint intID = 0;  // each object/mob load gets an ID, used for puts and equips - ID is inc'd for each load
  uint intMobID;  // mobs can load both inventory and equipment

  while (inv)
  {
   // ignore non-objects-and-mobs

    switch (inv->entityType)
    {
    case ENTITY_OBJECT :
      objH = (objectHere *)(inv->entityPtr);

      fprintf(invFile, "O\n%u %u\n", objH->objectNumb, objH->randomChance);

      intID++;

     // write any objects inside

      writeInventoryFileObjInside(invFile, objH->objInside, intID - 1, &intID);

      break;

    case ENTITY_MOB :
      mobH = (mobHere *)(inv->entityPtr);

      fprintf(invFile, "M\n%u %u\n", mobH->mobNumb, mobH->randomChance);

      intMobID = intID;

      intID++;

     // write equipment

      writeInventoryFileMobEquipment(invFile, mobH, intMobID, &intID);

     // write any inventory

      writeInventoryFileObjInside(invFile, mobH->inventoryHead, intMobID, &intID);

      break;

    case ENTITY_R_EDESC :
    case ENTITY_O_EDESC :
      desc = (extraDesc *)(inv->entityPtr);

      fputs("D\n", invFile);

      writeExtraDesctoFile(invFile, desc);

      break;
    }

    inv = inv->Next;
  }

 // write EOF token, close file

  fputs("S\n", invFile);

  fclose(invFile);
}


//
// writeInventoryFileObjInside
//
//          invFile : file
//          objHere : head of would-be inventory list
//   intContainerID : ID # of container obj/mob
//            intID : pointer to ID next new item should take, updated to new slot
//

void writeInventoryFileObjInside(FILE *invFile, const objectHere *objH, const uint intContainerID, uint *intID)
{
  while (objH)
  {
    fprintf(invFile, "P\n%u %u %u\n", objH->objectNumb, objH->randomChance, intContainerID);

    (*intID)++;

    writeInventoryFileObjInside(invFile, objH->objInside, (*intID) - 1, intID);

    objH = objH->Next;
  }
}


//
// writeInventoryFileMobEquipment
//
//    invFile : file
//    mobHere : the mob
//   intMobID : ID # of mob
//      intID : pointer to ID next new item should take, updated to new slot
//

void writeInventoryFileMobEquipment(FILE *invFile, const mobHere *mobH, const uint intMobID, uint *intID)
{
  for (uint pos = 0; pos <= WEAR_TRUEHIGH; pos++)
  {
    objectHere *objH = mobH->equipment[pos];

   // obj equipped on mob

    if (objH)
    {
      fprintf(invFile, "E\n%u %u %u %u\n", objH->objectNumb, objH->randomChance, intMobID,
                                           getZoneCommandEquipPos(pos));

      (*intID)++;

     // load any objects inside the equipment

      writeInventoryFileObjInside(invFile, objH->objInside, (*intID) - 1, intID);
    }
  }
}
