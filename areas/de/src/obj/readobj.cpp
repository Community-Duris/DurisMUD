//
//  File: readobj.cpp    originally part of durisEdit
//
//  Usage: functions used to read object info from the .obj file
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../fh.h"
#include "../types.h"

#include "../vardef.h"
#include "../edesc/edesc.h"
#include "object.h"

#include "../readfile.h"

#include "material.h"
#include "objsize.h"
#include "objcraft.h"

#include "../defines.h"



extern objectType **g_objLookup;
extern variable *g_varHead;
extern bool g_oldObjConv, g_madeChanges, g_readFromSubdirs;
extern uint g_numbLookupEntries, g_numbObjTypes, g_lowestObjNumber, g_highestObjNumber;


//
// readObjectFromFile
//

objectType *readObjectFromFile(FILE *objectFile, char *nextStrn, const bool checkDupes, const bool incNumbObjs)
{
  uchar applies = 0;
  bool preConv = g_oldObjConv;
  char strn[512];

  objectType *objPtr;


  strcpy(strn, nextStrn);

 // Read the object number, but only if it hasn't been previously read earlier

  if (!strlen(strn))
  {
    bool hitEOF;

    if (!readAreaFileLineAllowEOF(objectFile, strn, 512, ENTITY_OBJECT, ENTITY_NUMB_UNUSED, 
                                  ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, "vnum", 0, NULL, false, true, 
                                  &hitEOF))
      exit(1);

    if (hitEOF)
      return NULL;  // reached end of object file
  }

 // allocate memory for objectType

  objPtr = new(std::nothrow) objectType;
  if (!objPtr)
  {
    displayAllocError("objectType", "readObjectFromFile");

    exit(1);
  }

 // set everything in object record to 0/NULL

  memset(objPtr, 0, sizeof(objectType));

 // interpret vnum string

  if (strn[0] != '#')
  {
    _outtext("Line for object that should have '#' and vnum doesn't - string read was\n'");
    _outtext(strn);
    _outtext("'.  Aborting.\n");

    exit(1);
  }

  deleteChar(strn, 0);

  const uint objNumb = strtoul(strn, NULL, 10);

  if (checkDupes)
  {
    if (objNumb == 0)
    {
      _outtext("Error - object in .obj file has an invalid vnum of 0.  Aborting.\n");

      exit(1);
    }

    if (objExists(objNumb))
    {
      char outstrn[512];

      sprintf(outstrn, 
"Error: Object #%u has more than one entry in the .obj file.\n"
"       Aborting.\n",
              objNumb);

      _outtext(outstrn);

      exit(1);
    }
  }

  if (objNumb >= g_numbLookupEntries)
  {
    if (!changeMaxVnumAutoEcho(objNumb + 1000))
      exit(1);
  }

  objPtr->objNumber = objNumb;

 // Now, read the object keywords

  if (!readAreaFileLine(objectFile, strn, MAX_OBJKEY_LEN + 2, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "keyword", 0, NULL, true, true))
    exit(1);

  objPtr->keywordListHead = createKeywordList(strn);

  if (!strcmp(strn, "$"))  // end of file ($~ inside the file)
  {
    bool hitEOF;

    if (!readAreaFileLineAllowEOF(objectFile, nextStrn, 512, ENTITY_OBJECT, ENTITY_NUMB_UNUSED, 
                                  ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, "vnum", 0, NULL, false, true, 
                                  &hitEOF))
      exit(1);

    return objPtr;
  }

 // read the short object name

  if (!readAreaFileLine(objectFile, strn, MAX_OBJSNAME_LEN, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "short name", 0, NULL, true, true))
    exit(1);

  strcpy(objPtr->objShortName, strn);

 // read the long object name

  if (!readAreaFileLineTildeLine(objectFile, strn, MAX_OBJLNAME_LEN, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, 
                                 ENTITY_NUMB_UNUSED, "long name", 0, NULL, true, true))
    exit(1);

  strcpy(objPtr->objLongName, strn);

 // read first line of misc obj info - type, material, size, space, craftsmanship, dmg resist bonus, 
 //                                    extra, wear, extra2, anti, anti2

  const size_t intMiscArgs[] = { 3, 5, 6, 9, 11, 0 };

  if (!readAreaFileLine(objectFile, strn, 512, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                        "misc info", 0, intMiscArgs, false, true))
    exit(1);

  if (numbArgs(strn) == 11)
  {
    sscanf(strn, "%u%u%u%u" "%u%u" "%u%u%u%u%u",
           &(objPtr->objType),
           &(objPtr->material),
           &(objPtr->size),
           &(objPtr->space),

           &(objPtr->craftsmanship),
           &(objPtr->damResistBonus),

           &(objPtr->extraBits),
           &(objPtr->wearBits),
           &(objPtr->extra2Bits),
           &(objPtr->antiBits),
           &(objPtr->anti2Bits));
  }
  else
  if (numbArgs(strn) == 9)
  {
    sscanf(strn, "%u%u%u%u%u%u%u%u%u",
           &(objPtr->objType),
           &(objPtr->material),
           &(objPtr->size),
           &(objPtr->space),
           &(objPtr->extraBits),
           &(objPtr->wearBits),
           &(objPtr->extra2Bits),
           &(objPtr->antiBits),
           &(objPtr->anti2Bits));
  }
  else
  if (numbArgs(strn) == 6)
  {
    sscanf(strn, "%u%u%u%u%u%u",
           &(objPtr->objType),
           &(objPtr->extraBits),
           &(objPtr->wearBits),
           &(objPtr->extra2Bits),
           &(objPtr->antiBits),
           &(objPtr->anti2Bits));

    g_madeChanges = g_oldObjConv = true;
  }
  else
  if (numbArgs(strn) == 5)
  {
    sscanf(strn, "%u%u%u%u%u",
           &(objPtr->objType),
           &(objPtr->extraBits),
           &(objPtr->wearBits),
           &(objPtr->extra2Bits),
           &(objPtr->antiBits));

    g_madeChanges = g_oldObjConv = true;
  }
  else  // 3 args
  {
    sscanf(strn, "%u%u%u",
           &(objPtr->objType),
           &(objPtr->extraBits),
           &(objPtr->wearBits));

    g_madeChanges = g_oldObjConv = true;
  }

 // initialize to default values if not new format

  if (numbArgs(strn) != 11)
  {
    objPtr->craftsmanship = OBJCRAFT_AVERAGE;
  }

  if (g_oldObjConv != preConv)
  {
    displayAnyKeyPromptNoClr(
".obj file contains at least one object that does not have information\n"
"on object material, size, and volume.  Object(s) will automatically be\n"
"converted to new format, including copying material value from val4\n"
"of armor/worn objects to new location.  All other objects will have their\n"
"material, volume, and size set to default values and should be reviewed.\n\n"
"Press a key to continue...\n");
  }

 // read second line of misc obj info - object values (4 or 8)

  const size_t intMiscArgs2[] = { 4, 8, 0 };

  if (!readAreaFileLine(objectFile, strn, 512, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                        "misc info 2", 0, intMiscArgs2, false, true))
    exit(1);

  if (numbArgs(strn) == 4)
  {
    sscanf(strn, "%d%d%d%d",
           &(objPtr->objValues[0]),
           &(objPtr->objValues[1]),
           &(objPtr->objValues[2]),
           &(objPtr->objValues[3]));

    g_madeChanges = true;
  }
  else
  if (numbArgs(strn) == 8)
  {
    sscanf(strn, "%d%d%d%d" "%d%d%d%d",
           &(objPtr->objValues[0]),
           &(objPtr->objValues[1]),
           &(objPtr->objValues[2]),
           &(objPtr->objValues[3]),
           &(objPtr->objValues[4]),
           &(objPtr->objValues[5]),
           &(objPtr->objValues[6]),
           &(objPtr->objValues[7]));
  }

 // read third misc line - weight, value, condition, aff1, aff2, aff3, aff4

  const size_t intMiscArgs3[] = { 3, 5, 7, 0 };

  if (!readAreaFileLine(objectFile, strn, 512, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "misc info 3", 0, intMiscArgs3, false, true))
    exit(1);

  if (numbArgs(strn) == 3)
  {
    sscanf(strn, "%d%u%u", &(objPtr->weight), &(objPtr->worth),
                           &(objPtr->condition));
  }
  else
  if (numbArgs(strn) == 5)
  {
    sscanf(strn, "%d%u%u%u%u", &(objPtr->weight), &(objPtr->worth),
                               &(objPtr->condition),
                               &(objPtr->affect1Bits),
                               &(objPtr->affect2Bits));

    setVarBoolVal(&g_varHead, VAR_OBJAFFECT_NAME, true, false);
  }
  else if (numbArgs(strn) == 7)
  {
    sscanf(strn, "%d%u%u%u%u%u%u", &(objPtr->weight), &(objPtr->worth),
                                   &(objPtr->condition),
                                   &(objPtr->affect1Bits),
                                   &(objPtr->affect2Bits),
                                   &(objPtr->affect3Bits),
                                   &(objPtr->affect4Bits));

    setVarBoolVal(&g_varHead, VAR_OBJAFFECT_NAME, true, false);
  }

 // Now, check for E, A, T, #, or EOF - E is extra description, A is an "object
 // apply modifier", T is a trap, # at the start of a string means that we've 
 // hit the next object, and EOF means the list of objects is done

  while (true)
  {
    bool hitEOF;

    if (!readAreaFileLineAllowEOF(objectFile, strn, 512, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, 
                                  ENTITY_NUMB_UNUSED, "post-obj info", 0, NULL, false, true, &hitEOF))
      exit(1);

    if (hitEOF)
    {
      nextStrn[0] = '\0';
      break;
    }

    if (strn[0] == '\0') // blank line
    {
      continue;
    }
    else
    if (strcmpnocase(strn, "E"))  // extra description
    {
      readExtraDescFromFile(objectFile, ENTITY_OBJECT, objPtr->objNumber, ENTITY_O_EDESC, 
                            &(objPtr->extraDescHead));
    }
    else
    if (strcmpnocase(strn, "A"))  // apply modifier
    {
      if (applies >= NUMB_OBJ_APPLIES)
      {
        char outstrn[512];

        sprintf(outstrn,
"Error: Too many object apply modifiers for object #%u.  (The maximum\n"
"       number of apply modifiers supported is %u.)  Aborting.\n",
                objNumb, (uint)NUMB_OBJ_APPLIES);

        _outtext(outstrn);

        exit(1);
      }

      if (!readAreaFileLine(objectFile, strn, 512, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, 
                            ENTITY_NUMB_UNUSED, "apply info", 2, NULL, false, true))
        exit(1);

      sscanf(strn, "%u%d",
             &(objPtr->objApply[applies].applyWhere),
             &(objPtr->objApply[applies].applyModifier));

      applies++;
    }
    else

   // a trap!

    if (strcmpnocase(strn, "T"))  // trap
    {
      const size_t intTrapArgs[] = { 3, 4, 0 };

      if (!readAreaFileLine(objectFile, strn, 512, ENTITY_OBJECT, objNumb, ENTITY_TYPE_UNUSED, 
                            ENTITY_NUMB_UNUSED, "trap info", 0, intTrapArgs, false, true))
        exit(1);

     // support old format

      if (numbArgs(strn) == 3)
      {
        sscanf(strn, "%u%d%d",
               &(objPtr->trapBits),
               &(objPtr->trapDam),
               &(objPtr->trapCharge));

        objPtr->trapLevel = objPtr->condition;
        objPtr->condition = 100;
      }
      else
      if (numbArgs(strn) == 4)
      {
        sscanf(strn, "%u%d%d%u",
               &(objPtr->trapBits),
               &(objPtr->trapDam),
               &(objPtr->trapCharge),
               &(objPtr->trapLevel));
      }
    }
    else

   // hit next object, stick string in nextStrn

    if (strn[0] == '#')
    {
      strcpy(nextStrn, strn);

      break;
    }
    else
    {
      char outstrn[512];

      _outtext(
"Error: Unrecognized extra data - '");
      _outtext(strn);

      sprintf(outstrn, "' found in data for\n"
"       object #%u.  Aborting.\n", objNumb);

      _outtext(outstrn);

      exit(1);
    }
  }

  if (incNumbObjs)
  {
    g_numbObjTypes++;

    g_objLookup[objNumb] = objPtr;

    if (objNumb > g_highestObjNumber) 
      g_highestObjNumber = objNumb;

    if (objNumb < g_lowestObjNumber)  
      g_lowestObjNumber = objNumb;
  }

  return objPtr;
}


//
// readObjectFile : Reads all the object records from the user-specified object
//                  file, returning TRUE if the file was read successfully
//                  and FALSE otherwise
//
//    *filename : pointer to a filename string, if NULL, string in the
//                MAINZONENAME var is used
//

char readObjectFile(const char *filename)
{
  FILE *objectFile;
  char objectFilename[512] = "", nextStrn[512] = "";

  objectType *obj;

  uint lastObj = 0;


 // assemble the filename of the object file

  if (g_readFromSubdirs) 
    strcpy(objectFilename, "obj/");

  if (filename) 
    strcat(objectFilename, filename);
  else 
    strcat(objectFilename, getMainZoneNameStrn());

  strcat(objectFilename, ".obj");

 // open the object file for reading

  if ((objectFile = fopen(objectFilename, "rt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(objectFilename);
    _outtext(", skipping\n");

    return false;
  }

  _outtext("Reading ");
  _outtext(objectFilename);
  _outtext("...\n");

 // this while loop reads object by object, one object per iteration

  while (true)
  {
    obj = readObjectFromFile(objectFile, nextStrn, true, true);
    if (!obj) 
      break;

    if (obj->objNumber < lastObj)
    {
      char outstrn[512];

      sprintf(outstrn,
"Warning: object numbers out of order - #%u and #%u\n",
              lastObj, obj->objNumber);

      displayAnyKeyPrompt(outstrn);

      g_madeChanges = true;
    }
    else 
    {
      lastObj = obj->objNumber;
    }

   // check for '_ID_' impropriety

    if ((getEdescinList(obj->extraDescHead, "_ID_NAME_") ||
         getEdescinList(obj->extraDescHead, "_ID_SHORT_") ||
         getEdescinList(obj->extraDescHead, "_ID_DESC_")) &&
        !scanKeyword("_ID_", obj->keywordListHead))
    {
      char outstrn[512];

      sprintf(outstrn,
"Warning: object number %u has an 'identification extra desc'\n"
"         but doesn't have the '_ID_' keyword in its keyword list.  It has been\n"
"         automatically added.\n\n"
"Press a key to continue...\n\n",
              obj->objNumber);

      displayAnyKeyPrompt(outstrn);

      addKeywordtoList(&obj->keywordListHead, "_id_");

      g_madeChanges = true;
    }
  }

  fclose(objectFile);

 // done reading the file - but now check if objects need to be converted

  if (g_oldObjConv)
  {
    const uint highObjNumb = getHighestObjNumber();

    for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
    {
      if (objExists(objNumb))
      {
        obj = findObj(objNumb);

        obj->size = OBJSIZE_MEDIUM;

        if ((obj->objType == ITEM_WORN) || (obj->objType == ITEM_ARMOR))
        {
          switch (obj->objValues[3])
          {
            case  1 : obj->material = MAT_IRON; break;
            case  2 : obj->material = MAT_MITHRIL; break;
            case  3 : obj->material = MAT_HARDWOOD; break;
            case  4 : obj->material = MAT_CLOTH; break;
            case  5 : obj->material = MAT_LEATHER; break;
            case  6 : obj->material = MAT_SILICON; break;
            case  7 : obj->material = MAT_CRYSTAL; break;
            case  8 : obj->material = MAT_ADAMANTIUM; break;
            case  9 : obj->material = MAT_BONE; break;
            case 10 : obj->material = MAT_STONE; break;

            default : obj->material = MAT_IRON; break;
          }

          obj->objValues[3] = 0;
        }
        else 
        {
          obj->material = MAT_IRON;
        }
      }
    }
  }

  return true;
}
