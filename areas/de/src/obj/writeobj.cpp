//
//  File: writeobj.cpp   originally part of durisEdit
//
//  Usage: functions for writing object info to the .obj file
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
#include <string.h>

#include "fh.h"
#include "types.h"

#include "object.h"



extern bool g_readFromSubdirs;
extern uint g_numbObjTypes;

//
// writeObjecttoFile
//

void writeObjecttoFile(FILE *objectFile, const objectType *obj)
{
  char strn[512], i;
  extraDesc *descNode;


 // write the object number

  fprintf(objectFile, "#%u\n", obj->objNumber);

 // next, the object keyword list

  createKeywordString(obj->keywordListHead, strn, 511);
  lowstrn(strn);
  strcat(strn, "\n");

  fputs(strn, objectFile);

 // next, the short name of the object

  fprintf(objectFile, "%s~\n", obj->objShortName);

 // next, the long name of the object

  fprintf(objectFile, "%s~\n", obj->objLongName);

 // add another line with nothing but a tilde cuz it must be there

  fputs("~\n", objectFile);

 // next, write the type, material, size, space, extra flag, wear flag,
 // extra2 flag, anti flag and anti2 flag

  fprintf(objectFile, "%u %u %u %u " "%u %u " "%u %u %u %u %u\n",
                                    obj->objType,
                                    obj->material,
                                    obj->size,
                                    obj->space,

                                    obj->craftsmanship,
                                    obj->damResistBonus,

                                    obj->extraBits,
                                    obj->wearBits,
                                    obj->extra2Bits,
                                    obj->antiBits,
                                    obj->anti2Bits);

 // next, the "values"

  fprintf(objectFile, "%d %d %d %d %d %d %d %d\n",
          obj->objValues[0], obj->objValues[1],
          obj->objValues[2], obj->objValues[3],
          obj->objValues[4], obj->objValues[5],
          obj->objValues[6], obj->objValues[7]);

 // next, the weight, worth, and condition

  fprintf(objectFile, "%d %u %u",
          obj->weight, obj->worth, obj->condition);

 // if aff1 and/or aff2/3/4 flags are non-zero, write them

  if (obj->affect1Bits || obj->affect2Bits || obj->affect3Bits || obj->affect4Bits)
  {
    fprintf(objectFile, " %u %u %u %u\n",
            obj->affect1Bits, obj->affect2Bits,
            obj->affect3Bits, obj->affect4Bits);
  }
  else
  {
    fputs("\n", objectFile);
  }

 // write all the extra descs

  descNode = obj->extraDescHead;

  while (descNode)
  {
    fputs("E\n", objectFile);

    writeExtraDesctoFile(objectFile, descNode);

    descNode = descNode->Next;
  }

 // write the object apply info

  for (i = 0; i < NUMB_OBJ_APPLIES; i++)
  {
    if (obj->objApply[i].applyWhere)
    {
      fprintf(objectFile, "A\n%u %d\n", obj->objApply[i].applyWhere,
                                        obj->objApply[i].applyModifier);
    }
  }

 // write trap info, if any

  if (obj->trapBits)
  {
    fprintf(objectFile, "T\n%u %d %d %u\n",
            obj->trapBits, obj->trapDam, obj->trapCharge, obj->trapLevel);
  }
}


//
// writeObjectFile : Write the object file - contains all the objects
//

void writeObjectFile(const char *filename)
{
  FILE *objectFile;
  char objectFilename[512] = "";
  char strn[512];

  uint numb = getLowestObjNumber();
  const uint highest = getHighestObjNumber();


 // assemble the filename of the object file

  if (g_readFromSubdirs) 
    strcpy(objectFilename, "obj/");

  if (filename) 
    strcat(objectFilename, filename);
  else 
    strcat(objectFilename, getMainZoneNameStrn());

  strcat(objectFilename, ".obj");


 // open the object file for writing

  if ((objectFile = fopen(objectFilename, "wt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(objectFilename);
    _outtext(" for writing - aborting\n");

    return;
  }

  sprintf(strn, "Writing %s - %u object type%s\n",
          objectFilename, g_numbObjTypes, plural(g_numbObjTypes));

  _outtext(strn);

  while (numb <= highest)
  {
    const objectType *obj = findObj(numb);

    if (obj) 
      writeObjecttoFile(objectFile, obj);

    numb++;
  }

  fclose(objectFile);
}
