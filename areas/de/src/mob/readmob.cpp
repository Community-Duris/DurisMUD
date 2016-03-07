//
//  File: readmob.cpp    originally part of durisEdit
//
//  Usage: functions for reading mob info from the .mob file
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


#include "types.h"
#include "fh.h"

#include "readfile.h"

#include "mob.h"



extern bool g_readFromSubdirs, g_madeChanges;
extern uint g_numbMobs, g_numbLookupEntries, g_numbMobTypes, g_highestMobNumber, g_lowestMobNumber;
extern mobType **g_mobLookup;

//
// convertMobMoneyRedundant : redundant code for convertMobMoney - returns new position of strn, sets
//                            *blnError to false on error
//

char *convertMobMoneyRedundant(char *strptr, uint *money, bool *blnError, const bool allowEndNull)
{
  const char *numbstart = strptr;


  while (*strptr && (*strptr != '.'))
    strptr++;

  if (!*strptr && !allowEndNull)
  {
    *blnError = true;
    return strptr;
  }

  *strptr = '\0';

  if (!strnumer(numbstart))
  {
    *blnError = true;
    return strptr;
  }

  *money = strtoul(numbstart, NULL, 10);

  strptr++;

  return strptr;
}


//
// convertMobMoney : Converts a dot-separated "money string" into separate
//                   integer amounts - for instance, "4.3.2.1" is 4 copper,
//                   3 silver, 2 gold, and 1 platinum - returns false if
//                   there is an error
//
//  *strn : string as read from mob file
//   *mob : corresponding mob record
//

bool convertMobMoney(char *strn, mobType *mob)
{
  bool blnError = false;


 // first comes the copper

  strn = convertMobMoneyRedundant(strn, &(mob->copper), &blnError, false);
  if (blnError)
    return false;

 // next, silver

  strn = convertMobMoneyRedundant(strn, &(mob->silver), &blnError, false);
  if (blnError)
    return false;

 // next, gold

  strn = convertMobMoneyRedundant(strn, &(mob->gold), &blnError, false);
  if (blnError)
    return false;

 // and finally, platinum

  strn = convertMobMoneyRedundant(strn, &(mob->platinum), &blnError, true);

  return !blnError;
}


//
// readMobFromFile
//

mobType *readMobFromFile(FILE *mobFile, const bool checkDupes, const bool incNumbMobs)
{
  char strn[512], moneyStrn[512], tempstrn[512], tempstrn2[512];
  mobType *mobPtr;


 // Read the mob number

  bool hitEOF;

  if (!readAreaFileLineAllowEOF(mobFile, strn, 512, ENTITY_MOB, ENTITY_NUMB_UNUSED, 
                                ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED, "vnum", 0, NULL, false, true, 
                                &hitEOF))
    exit(1);

  if (hitEOF)
    return NULL;  // reached end of mob file

 // allocate memory for mobType

  mobPtr = new(std::nothrow) mobType;
  if (!mobPtr)
  {
    displayAllocError("mobType", "readMobFromFile");

    exit(1);
  }

 // set everything in mob record to 0/NULL

  memset(mobPtr, 0, sizeof(mobType));

 // do stuff to vnum string

  if (strn[0] != '#')
  {
    _outtext(
"Line for mob that should have '#' and vnum doesn't - string read was\n"
"'");
    _outtext(strn);
    _outtext("'.  Aborting.\n");

    exit(1);
  }

  deleteChar(strn, 0);

  const uint mobNumb = strtoul(strn, NULL, 10);

  if (checkDupes)
  {
    if (mobNumb == 0)
    {
      _outtext("Error - mob in .mob file has an invalid vnum of 0.  Aborting.\n");

      exit(1);
    }

    if (mobExists(mobNumb))
    {
      char outstrn[512];

      sprintf(outstrn, "Error - mob #%u has more than one entry in the .mob file.  Aborting.\n",
              mobNumb);

      _outtext(outstrn);

      exit(1);
    }
  }

  if (mobNumb >= g_numbLookupEntries)
  {
    if (!changeMaxVnumAutoEcho(mobNumb + 1000))
      exit(1);
  }

  mobPtr->mobNumber = mobNumb;

 // Now, read the mob keywords

  if (!readAreaFileLine(mobFile, strn, MAX_MOBKEY_LEN + 2, ENTITY_MOB, mobNumb, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "keyword", 0, NULL, true, true))
    exit(1);

  mobPtr->keywordListHead = createKeywordList(strn);

  if (!strcmp(strn, "$"))  // end of file (actually $~ inside the file)
  {
    return mobPtr;
  }

 // read the short mob name

  if (!readAreaFileLine(mobFile, strn, MAX_MOBSNAME_LEN, ENTITY_MOB, mobNumb, ENTITY_TYPE_UNUSED, 
                        ENTITY_NUMB_UNUSED, "short name", 0, NULL, true, true))
    exit(1);

  strcpy(mobPtr->mobShortName, strn);

 // read the long mob name

  if (!readAreaFileLineTildeLine(mobFile, strn, MAX_MOBLNAME_LEN, ENTITY_MOB, mobNumb, ENTITY_TYPE_UNUSED, 
                                 ENTITY_NUMB_UNUSED, "long name", 0, NULL, false, true))
    exit(1);

  strcpy(mobPtr->mobLongName, strn);

 // now, read the mob description

  mobPtr->mobDescHead = readStringNodes(mobFile, TILDE_LINE, true);

 // read the rest of the mob info

 // read first line of misc mob info - action, agg, agg2, aff1, aff2, aff3, aff4 flags, align, S

  const size_t intMiscArgs[] = { 5, 7, 9, 10, 0 };

  if (!readAreaFileLine(mobFile, strn, 512, ENTITY_MOB, mobNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                        "misc info", 0, intMiscArgs, false, true))
    exit(1);

  if (numbArgs(strn) == 5)
  {
    sscanf(strn, "%u%u%u%d%s",
           &(mobPtr->actionBits),
           &(mobPtr->affect1Bits),
           &(mobPtr->affect2Bits),
           &(mobPtr->alignment),
           tempstrn);  // "S"
  }
  else if (numbArgs(strn) == 7)
  {
    sscanf(strn, "%u%u%u%u%u%d%s",
           &(mobPtr->actionBits),
           &(mobPtr->affect1Bits),
           &(mobPtr->affect2Bits),
           &(mobPtr->affect3Bits),
           &(mobPtr->affect4Bits),
           &(mobPtr->alignment),
           tempstrn);  // "S"
  }
  else if (numbArgs(strn) == 9)
  {
    sscanf(strn, "%u%u%u%u%u%u%u%d%s",
           &(mobPtr->actionBits),
           &(mobPtr->aggroBits),
           &(mobPtr->aggro2Bits),
           &(mobPtr->affect1Bits),
           &(mobPtr->affect2Bits),
           &(mobPtr->affect3Bits),
           &(mobPtr->affect4Bits),
           &(mobPtr->alignment),
           tempstrn);  // "S"
  }
  else if (numbArgs(strn) == 10)
  {
    sscanf(strn, "%u%u%u%u%u%u%u%u%d%s",
           &(mobPtr->actionBits),
           &(mobPtr->aggroBits),
           &(mobPtr->aggro2Bits),
           &(mobPtr->aggro3Bits),
           &(mobPtr->affect1Bits),
           &(mobPtr->affect2Bits),
           &(mobPtr->affect3Bits),
           &(mobPtr->affect4Bits),
           &(mobPtr->alignment),
           tempstrn);  // "S"
  }

 // check for required S at the end

  if (!strcmpnocase(tempstrn, "S"))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Letter after mob alignment for mob #%u not equal to S -\n"
"       Aborting.\n\n"
"The string read was '",
            mobNumb);

    _outtext(outstrn);
    _outtext(strn);
    _outtext("'.\n");

    exit(1);
  }

 // read second line of misc mob info - species, hometown, class, size

  const size_t intMiscArgs2[] = { 3, 4, 5, 0 };

  if (!readAreaFileLine(mobFile, strn, 512, ENTITY_MOB, mobNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                        "misc info 2", 0, intMiscArgs2, false, true))
    exit(1);

  if (numbArgs(strn) == 3)
  {
    sscanf(strn, "%s%d%u",
           tempstrn,
           &(mobPtr->mobHometown),
           &(mobPtr->mobClass));

    mobPtr->size = MOB_SIZE_DEFAULT;
  }
  else if (numbArgs(strn) == 4)
  {
    sscanf(strn, "%s%d%u%d",
           tempstrn,
           &(mobPtr->mobHometown),
           &(mobPtr->mobClass),
           &(mobPtr->size));
  }
  else
  {
    sscanf(strn, "%s%d%u%u%d",
	   tempstrn,
	   &(mobPtr->mobHometown),
	   &(mobPtr->mobClass),
	   &(mobPtr->mobSpec),
	   &(mobPtr->size));
  }

 // check/upcase species string, check mob class

  if (strlen(tempstrn) > MAX_SPECIES_LEN)
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Mob species string for mob #%u is too long (over %u\n"
"       characters).  Shortened to %u characters.\n\n", 
            mobNumb, (uint)MAX_SPECIES_LEN, (uint)MAX_SPECIES_LEN);

    displayAnyKeyPrompt(outstrn);

    tempstrn[MAX_SPECIES_LEN] = '\0';
    g_madeChanges = true;
  }

  strcpy(mobPtr->mobSpecies, tempstrn);

 // read third line of misc mob info - level, thac0, ac, hit points, damage

  if (!readAreaFileLine(mobFile, strn, 512, ENTITY_MOB, mobNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                        "misc info 3", 5, NULL, false, true))
    exit(1);

  sscanf(strn, "%d%d%d%s%s",
         &(mobPtr->level),
         &(mobPtr->thac0),
         &(mobPtr->ac),
         tempstrn,  // hp
         tempstrn2);// damage

  if ((strlen(tempstrn) > MAX_MOBHP_LEN) || (strlen(tempstrn2) > MAX_MOBDAM_LEN))
  {
    char outstrn[512];

    sprintf(outstrn, 
"Error: Either hit point or damage value for mob #%u is too long (over %u\n"
"       characters).  Aborting.",
            mobNumb, MAX_MOBDAM_LEN);

    _outtext(outstrn);

    exit(1);
  }

  strcpy(mobPtr->hitPoints, tempstrn);
  strcpy(mobPtr->mobDamage, tempstrn2);

 // check mob hit points/damage strings

  checkDieStrnValidity(mobPtr->hitPoints, mobPtr->mobNumber, "hit point");
  checkDieStrnValidity(mobPtr->mobDamage, mobPtr->mobNumber, "damage");

 // read fourth line of misc mob info - money, experience

  if (!readAreaFileLine(mobFile, strn, 512, ENTITY_MOB, mobNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                        "misc info 4", 2, NULL, false, true))
    exit(1);

  sscanf(strn, "%s%u",
         moneyStrn, &(mobPtr->exp));

 // get amount of money mob has

  if (!convertMobMoney(moneyStrn, mobPtr))
  {
    char outstrn[512];

    sprintf(outstrn,
"Error: Field that specifies mob money for mob #%u is somehow invalid -\n"
"       string read was '",
            mobNumb);

    _outtext(outstrn);
    _outtext(strn);
    _outtext("'.  Aborting.\n");

    exit(1);
  }

 // read fifth line of misc mob info - pos, default pos, sex

  if (!readAreaFileLine(mobFile, strn, 512, ENTITY_MOB, mobNumb, ENTITY_TYPE_UNUSED, ENTITY_NUMB_UNUSED,
                        "misc info 5", 3, NULL, false, true))
    exit(1);

  sscanf(strn, "%d%d%d",
         &(mobPtr->position), &(mobPtr->defaultPos), &(mobPtr->sex));

  if ((mobPtr->position < POSITION_LOWEST_LEGAL) || (mobPtr->position > POSITION_HIGHEST))
  {
    char outstrn[512];

    sprintf(outstrn,
"Warning: Position value for mob #%u (%d) is out of legal range.\n\n"
"         Setting position to 'standing'.\n",
            mobNumb, mobPtr->position);

    displayAnyKeyPrompt(outstrn);

    mobPtr->position = POSITION_STANDING;
    g_madeChanges = true;
  }

  if ((mobPtr->defaultPos < POSITION_LOWEST_LEGAL) || (mobPtr->defaultPos > POSITION_HIGHEST))
  {
    char outstrn[512];

    sprintf(outstrn,
"Warning: Default pos value for mob #%u (%d) is out of legal range.\n\n"
"         Setting position to 'standing'.\n",
            mobNumb, mobPtr->defaultPos);

    displayAnyKeyPrompt(outstrn);

    mobPtr->defaultPos = POSITION_STANDING;
    g_madeChanges = true;
  }

  if (incNumbMobs)
  {
    g_numbMobTypes++;

    g_mobLookup[mobNumb] = mobPtr;

    if (mobNumb > g_highestMobNumber) 
      g_highestMobNumber = mobNumb;

    if (mobNumb < g_lowestMobNumber)  
      g_lowestMobNumber = mobNumb;
  }

  return mobPtr;
}


//
// readMobFile : Reads all the mob records from the user-specified mob file -
//               returns TRUE if successful, FALSE otherwise
//
//   *filename : if non-NULL, used as filename of mob file, otherwise value in
//               MAINZONENAME is used
//

bool readMobFile(const char *filename)
{
  FILE *mobFile;
  char mobFilename[512] = "";

  mobType *mob;

  uint lastMob = 0;


 // assemble the filename of the mob file

  if (g_readFromSubdirs) 
    strcpy(mobFilename, "mob/");

  if (filename) 
    strcat(mobFilename, filename);
  else 
    strcat(mobFilename, getMainZoneNameStrn());

  strcat(mobFilename, ".mob");

 // open the mob file for reading

  if ((mobFile = fopen(mobFilename, "rt")) == NULL)
  {
    _outtext("Couldn't open ");
    _outtext(mobFilename);
    _outtext(", skipping\n");

    return false;
  }

  _outtext("Reading ");
  _outtext(mobFilename);
  _outtext("...\n");

 // this while loop reads mob by mob, one mob per iteration

  while (true)
  {
    mob = readMobFromFile(mobFile, true, true);
    if (!mob) 
      break;  // eof

    if (mob->mobNumber < lastMob)
    {
      char outstrn[1024];

      sprintf(outstrn,
"Warning: Mob numbers out of order - #%u and #%u\n",
              lastMob, mob->mobNumber);

      displayAnyKeyPrompt(outstrn);

      g_madeChanges = true;
    }
    else 
    {
      lastMob = mob->mobNumber;
    }
  }

  fclose(mobFile);

  return true;
}
