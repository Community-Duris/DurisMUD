//
//  File: editflag.cpp   originally part of durisEdit
//
//  Usage: generalized functions for editing bit values in 32-bit
//         bitvectors and choosing specific values from enumerated
//         lists
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
#include <ctype.h>

#include "fh.h"
#include "de.h"
#include "types.h"
#include "keys.h"
#include "misc/menu.h"
#include "dispflag.h"

#include "graphcon.h"

#include "flagdef.h"



extern usint g_templateKeys[], g_setTemplateKeys[];
extern bool g_madeChanges;
extern "C" const char *specdata[][MAX_SPEC];

//
// canEditFlag : returns true if user can toggle flag
//

bool canEditFlag(const flagDef &flagInf, uint flagState, const bool asBitV)
{
  if (flagState) 
    flagState = 1;  // makes things easier to check below

  return (flagInf.editable || getEditUneditableFlagsVal() ||
          (asBitV && (flagState != flagInf.defVal)));
}


//
// isFlagValid : returns true if user can set flag to this value
//

bool isFlagValid(const flagDef &flagInf, uint flagState, const bool asBitV)
{
  if (flagState) 
    flagState = 1;  // makes things easier to check below

  return (flagInf.editable || getEditUneditableFlagsVal() ||
          (asBitV && (flagState == flagInf.defVal)));
}


// all the following functions have to be doubled up since occasionally I
// pass object value fields, which are signed longs (versus unsigned for
// everything else)..  the functions are different enough that I'd rather keep
// them separate

//
// checkCommonFlagKeys : returns TRUE if flags have been changed
//
//               ch : key entered by user
//
//       *flagValue : pointer to flag location
//        numbFlags : number of valid flags
//
//     headerOffset : y-offset due to menu heading
//
//              row : row input prompt is on
//

bool checkCommonFlagKeys(const usint ch, const flagDef *flagArr, uint *flagValue, const uint numbFlags,
                         const sint *colXpos, const uint *colTopFlag, const uint numbCols, 
                         const sint headerOffset, const sint row, const bool asBitVect)
{
  uint i;


 // enter flag value directly

  if ((ch == '!') || (ch == K_Ctrl_D))
  {
    const char* promptStrn;
    const uint origValue = *flagValue;  // for invalid value in non-bitvect


    if (asBitVect)
      promptStrn = "&+CEnter new flags value: ";
    else
      promptStrn = "&+CEnter new value: ";

    editUIntVal(flagValue, true, promptStrn);

    if (asBitVect)
    {
      for (i = 0; i < numbFlags; i++)
      {
        if (!isFlagValid(flagArr[i], *flagValue & (1 << i), true))
          *flagValue ^= (1 << i);
      }
    }
    else
    {
      for (i = 0; i < numbFlags; i++)
        if (flagArr[i].defVal == *flagValue)
          break;

      if ((i != numbFlags) && !isFlagValid(flagArr[i], *flagValue, false))
        *flagValue = origValue;
    }

    updateFlags(*flagValue, numbFlags, numbCols, colXpos, colTopFlag,
                headerOffset, true, asBitVect, flagArr);

    clrline(row, 7, 0);
    _settextposition(row, 1);

    displayColorString(getMenuPromptName());
  }
  else

 // turn all bits on

  if (asBitVect && ((ch == '>') || (ch == '.') || (ch == K_Ctrl_N)))
  {
    for (i = 0; i < numbFlags; i++)
    {
      if (canEditFlag(flagArr[i], *flagValue & (1 << i), asBitVect))
        *flagValue |= (1 << i);
    }

    updateFlags(*flagValue, numbFlags, numbCols, colXpos, colTopFlag,
                headerOffset, false, asBitVect, flagArr);  // shouldn't be any non-set..  shrug

    clrline(row, 7, 0);
    _settextposition(row, 1);

    displayColorString(getMenuPromptName());
  }
  else

 // turn all bits off

  if (asBitVect && ((ch == '<') || (ch == ',') || (ch == K_Ctrl_F)))
  {
    for (i = 0; i < numbFlags; i++)
    {
      if (canEditFlag(flagArr[i], *flagValue & (1 << i), asBitVect))
        *flagValue &= ~(1 << i);
    }

    updateFlags(*flagValue, numbFlags, numbCols, colXpos, colTopFlag,
                headerOffset, true, asBitVect, flagArr);

    clrline(row, 7, 0);
    _settextposition(row, 1);

    displayColorString(getMenuPromptName());
  }
  else

 // toggle all bits

  if (asBitVect && ((ch == '?') || (ch == '/') || (ch == K_Ctrl_T)))
  {
    for (i = 0; i < numbFlags; i++)
    {
      if (canEditFlag(flagArr[i], *flagValue & (1 << i), asBitVect))
        *flagValue ^= (1 << i);
    }

    updateFlags(*flagValue, numbFlags, numbCols, colXpos, colTopFlag,
                headerOffset, true, asBitVect, flagArr);

    clrline(row, 7, 0);
    _settextposition(row, 1);

    displayColorString(getMenuPromptName());
  }
  else return false;

  return true;
}


//
// checkCommonFlagKeys : returns TRUE if flags have been changed
//
//               ch : key entered by user
//
//       *flagValue : pointer to flag location
//        numbFlags : number of valid flags
//
//     headerOffset : y-offset due to menu heading
//
//              row : row input prompt is on
//

bool checkCommonFlagKeys(const usint ch, const flagDef *flagArr, int *flagValue, const uint numbFlags,
                         const sint *colXpos, const uint *colTopFlag, const uint numbCols, 
                         const sint headerOffset, const sint row, const bool asBitVect)
{
  uint i;


 // enter flag value directly

  if ((ch == '!') || (ch == K_Ctrl_D))
  {
    const char* promptStrn;
    const int origValue = *flagValue;  // for invalid value in non-bitvect


    if (asBitVect)
      promptStrn = "&+CEnter new flags value: ";
    else
      promptStrn = "&+CEnter new value: ";

    if (asBitVect)
      editUIntVal((uint *)flagValue, true, promptStrn);
    else
      editIntVal(flagValue, true, promptStrn);

   // don't allow user to change uneditable bits via value entry

    if (asBitVect)
    {
      for (i = 0; i < numbFlags; i++)
      {
        if (!isFlagValid(flagArr[i], *flagValue & (1 << i), true))
          *flagValue ^= (1 << i);
      }
    }
    else
    {
      for (i = 0; i < numbFlags; i++)
        if (flagArr[i].defVal == *flagValue)
          break;

      if ((i != numbFlags) && !isFlagValid(flagArr[i], *flagValue, false))
        *flagValue = origValue;
    }

    updateFlags(*flagValue, numbFlags, numbCols, colXpos, colTopFlag,
                headerOffset, true, asBitVect, flagArr);

    clrline(row, 7, 0);
    _settextposition(row, 1);

    displayColorString(getMenuPromptName());
  }
  else

 // turn all bits on

  if (asBitVect && ((ch == '>') || (ch == '.') || (ch == K_Ctrl_N)))
  {
    for (i = 0; i < numbFlags; i++)
    {
      if (canEditFlag(flagArr[i], *flagValue & (1 << i), asBitVect))
        *flagValue |= (1 << i);
    }

    updateFlags(*flagValue, numbFlags, numbCols, colXpos, colTopFlag,
                headerOffset, false, asBitVect, flagArr);

    clrline(row, 7, 0);
    _settextposition(row, 1);

    displayColorString(getMenuPromptName());
  }
  else

 // turn all bits off

  if (asBitVect && ((ch == '<') || (ch == ',') || (ch == K_Ctrl_F)))
  {
    for (i = 0; i < numbFlags; i++)
    {
      if (canEditFlag(flagArr[i], *flagValue & (1 << i), asBitVect))
        *flagValue &= ~(1 << i);
    }

    updateFlags(*flagValue, numbFlags, numbCols, colXpos, colTopFlag,
                headerOffset, true, asBitVect, flagArr);

    clrline(row, 7, 0);
    _settextposition(row, 1);

    displayColorString(getMenuPromptName());
  }
  else

 // toggle all bits

  if (asBitVect && ((ch == '?') || (ch == '/') || (ch == K_Ctrl_T)))
  {
    for (i = 0; i < numbFlags; i++)
    {
      if (canEditFlag(flagArr[i], *flagValue & (1 << i), asBitVect))
        *flagValue ^= (1 << i);
    }

    updateFlags(*flagValue, numbFlags, numbCols, colXpos, colTopFlag,
                headerOffset, true, asBitVect, flagArr);

    clrline(row, 7, 0);
    _settextposition(row, 1);

    displayColorString(getMenuPromptName());
  }
  else return false;

  return true;
}


//
// returns -1 on failure, actual pos otherwise (flagArr[0 through numbFlags - 1])
//

int getFlagNumbWithDefVal(const flagDef *flagArr, const uint numbFlags, const int defV)
{
  for (uint i = 0; i < numbFlags; i++)
  {
    if (flagArr[i].defVal == defV)
      return i;
  }

  return -1;
}


//
// interpEditFlags
//

bool interpEditFlags(usint ch, const flagDef *flagArr, uint *flagVal, const uint numbFlags, 
                     const sint *colXpos, const uint *colTopFlag, const uint numbCols, uint *templates, 
                     const bool asBitVect)
{
  struct rccoord xy = _gettextposition();
  uint oldflag;
  int i;


  if (checkMenuKey(ch, true) == MENUKEY_SAVE) 
    return true;

 // check common keys

  if (checkCommonFlagKeys(ch, flagArr, flagVal, numbFlags,
                          colXpos, colTopFlag, numbCols,
                          HEADER_OFFSETY, xy.row, asBitVect))
    return false;

  if ((ch >= '1') && (ch < 'A')) 
    ch += 42;

  if ((ch >= 'A') && (ch <= ('A' + (numbFlags - 1))))
  {
    ch -= 'A';

    if (asBitVect)
    {
      if (canEditFlag(flagArr[ch], *flagVal & (1 << ch), true))
        *flagVal ^= (1 << ch);
      else
        return false;
    }
    else
    {
      if (canEditFlag(flagArr[ch], ch, false) &&
          (*flagVal != flagArr[ch].defVal))
      {
        oldflag = *flagVal;

        *flagVal = flagArr[ch].defVal;
      }
      else 
      {
        return false;
      }
    }

    updateSingleFlag(*flagVal, (uchar)ch, numbFlags, numbCols, colXpos, colTopFlag,
                     HEADER_OFFSETY, asBitVect, flagArr);

   // clear previous flag if enum list

    if (!asBitVect)
    {
      i = getFlagNumbWithDefVal(flagArr, numbFlags, oldflag);

     // ignore i values of 0 or below because previous val may have been
     // one that isn't in the list

     // ch + 693 to clear old flag..

      if (i >= 0)
        updateSingleFlag(ch + 693, (uchar)i, numbFlags, numbCols, colXpos,
                         colTopFlag, HEADER_OFFSETY, false, flagArr);
    }
  }

 // check for template key

  if (templates)
  {
    for (i = 0; i < NUMB_FLAG_TEMPLATES; i++)
    {
      if (ch == g_templateKeys[i])
      {
       // set flags, but only flags that user is allowed to change

        for (uint j = 0; j < numbFlags; j++)
        {
          if (canEditFlag(flagArr[j], *flagVal & (1 << j), true))
          {
            if (templates[i] & (1 << j))
              *flagVal |= (1 << j);
            else
              *flagVal &= ~(1 << j);
          }
        }

        updateFlags(*flagVal, numbFlags, numbCols, colXpos, colTopFlag,
                    HEADER_OFFSETY, true, asBitVect, flagArr);

        _settextposition(xy.row, xy.col);

        break;
      }

      if (ch == g_setTemplateKeys[i])
      {
        templates[i] = *flagVal;
        g_madeChanges = true;

        break;
      }
    }
  }

  return false;
}


//
// interpEditFlags - signed int version
//

bool interpEditFlags(usint ch, const flagDef *flagArr, int *sflagVal, const uint numbFlags, 
                     const sint *colXpos, const uint *colTopFlag, const uint numbCols, uint *templates, 
                     const bool asBitVect)
{
  struct rccoord xy = _gettextposition();
  int i, oldflag;


  if (checkMenuKey(ch, true) == MENUKEY_SAVE) 
    return true;

 // check common keys

  if (checkCommonFlagKeys(ch, flagArr, sflagVal, numbFlags,
                          colXpos, colTopFlag, numbCols,
                          HEADER_OFFSETY, xy.row, asBitVect))
    return false;

  if ((ch >= '1') && (ch < 'A')) 
    ch += 42;

  if ((ch >= 'A') && (ch <= ('A' + (numbFlags - 1))))
  {
    ch -= 'A';

    if (asBitVect)
    {
      if (canEditFlag(flagArr[ch], *sflagVal & (1 << ch), asBitVect))
        *sflagVal ^= (1 << ch);
      else
        return false;
    }
    else
    {
      if (canEditFlag(flagArr[ch], ch, false) &&
          (*sflagVal != flagArr[ch].defVal))
      {
        oldflag = *sflagVal;

        *sflagVal = flagArr[ch].defVal;
      }
      else 
      {
        return false;
      }
    }

   // clear previous flag if enum list

    if (!asBitVect)
    {
      i = getFlagNumbWithDefVal(flagArr, numbFlags, oldflag);

     // ignore i values of 0 or below because previous val may have been
     // one that isn't in the list

     // ch + 693 to clear old flag..

      if (i >= 0)
        updateSingleFlag(ch + 693, (uchar)i, numbFlags, numbCols, colXpos,
                         colTopFlag, HEADER_OFFSETY, false, flagArr);
    }

    updateSingleFlag(*sflagVal, (uchar)ch, numbFlags, numbCols, colXpos, colTopFlag,
                     HEADER_OFFSETY, asBitVect, flagArr);
  }

 // no template key check because no template-able flags are signed

  return false;
}


//
// editFlags : returns FALSE if user aborted, TRUE if saved
//

bool editFlags(const flagDef *flagArr, uint *flagVal, const char entityType, const char *entityName, 
               const uint entityNumb, const char *flagName, uint *templates, uint numbCols, 
               const bool asBitVect)
{
  usint ch;
  uint colTopFlag[MAX_FLAG_COLUMNS];
  sint colXpos[MAX_FLAG_COLUMNS];
  uint oldFlags = *flagVal;
  uint numbFlags;


  numbFlags = 0;

  while (flagArr[numbFlags].flagShort)
    numbFlags++;

  displayFlagMenu(*flagVal, flagArr, entityType, entityName, entityNumb, flagName,
                  colXpos, colTopFlag, &numbCols, asBitVect);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, true) == MENUKEY_ABORT)
    {
      *flagVal = oldFlags;

      _outtext("\n\n");

      return false;
    }

    if (interpEditFlags(ch, flagArr, flagVal, numbFlags,
                        colXpos, colTopFlag, numbCols, templates, asBitVect))
    {
      return true;
    }
  }
}

// Because speclist is class dependent, we need to create g_mobSpecList on
// the fly.

flagDef g_mobSpecList[MAX_SPEC+2];

void updateSpecList(const mobType *mob)
{
  uint speclist = 0;
  uint specnum = 0;

  uint classCount = countClass(mob->mobClass);
  uint classnum = classNumb(mob->mobClass);

  while (speclist <= MAX_SPEC+1)
  {
    g_mobSpecList[speclist].flagShort = "Test";
    
    // We don't have a no spec in specdata, so creating it artificially here
    if (!speclist)
    {
      g_mobSpecList[speclist].flagLong = "None";
      g_mobSpecList[speclist].editable = 1;
    }
    else if (!classCount || classCount > 1)
    {
      g_mobSpecList[speclist].flagLong = "Not Available";
      g_mobSpecList[speclist].editable = 0;
    }
    else
    {
      if (classnum <= CLASS_COUNT)
      {
	g_mobSpecList[speclist].flagLong = (*specdata[classnum][specnum] ? specdata[classnum][specnum] : "Unused");
	g_mobSpecList[speclist].editable = (!*specdata[classnum][specnum] ? 0 : 1);
      }
      else
      {
	g_mobSpecList[speclist].flagLong = "Unused";
	g_mobSpecList[speclist].editable = 0;
      }
    }
    
    g_mobSpecList[speclist].defVal = speclist;
    
    if (speclist++)
      specnum++;
  }
  g_mobSpecList[MAX_SPEC+1].flagShort = 0; // SPEC_NONE + MAX_SPEC + NULL
}

bool editSpecs(uint *flagVal, const char entityType, const char *entityName, 
               const uint entityNumb, const char *flagName, uint *templates, uint numbCols,
               const bool asBitVect)
{
  usint ch;
  uint colTopFlag[MAX_FLAG_COLUMNS];
  sint colXpos[MAX_FLAG_COLUMNS];
  uint oldFlags = *flagVal;
  uint numbFlags;

  numbFlags = 0;
  while (g_mobSpecList[numbFlags].flagShort)
    numbFlags++;

  const flagDef *flagArr = g_mobSpecList;

  displayFlagMenu(*flagVal, flagArr, entityType, entityName, entityNumb, flagName,
                  colXpos, colTopFlag, &numbCols, asBitVect);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, true) == MENUKEY_ABORT)
    {
      *flagVal = oldFlags;

      _outtext("\n\n");

      return false;
    }

    if (interpEditFlags(ch, flagArr, flagVal, numbFlags,
                        colXpos, colTopFlag, numbCols, templates, asBitVect))
    {
      return true;
    }
  }
}


//
// editFlags - int flagVal version, for obj value fields
//

bool editFlags(const flagDef *flagArr, int *sflagVal, const char entityType, const char *entityName, 
               const uint entityNumb, const char *flagName, uint *templates, uint numbCols, 
               const bool asBitVect)
{
  usint ch;
  uint colTopFlag[MAX_FLAG_COLUMNS];
  sint colXpos[MAX_FLAG_COLUMNS];
  int oldFlags = *sflagVal;
  uint numbFlags;


  numbFlags = 0;

  while (flagArr[numbFlags].flagShort)
    numbFlags++;
  
  if (asBitVect && (numbFlags > 15)) 
    return false;
    // can't edit more than 15 flags with a signed long

  displayFlagMenu(*sflagVal, flagArr, entityType, entityName, entityNumb, flagName,
                  colXpos, colTopFlag, &numbCols, asBitVect);

  while (true)
  {
    ch = toupper(getkey());

    if (checkMenuKey(ch, true) == MENUKEY_ABORT)
    {
      *sflagVal = oldFlags;

      _outtext("\n\n");

      return false;
    }

    if (interpEditFlags(ch, flagArr, sflagVal, numbFlags,
                        colXpos, colTopFlag, numbCols, templates, asBitVect))
    {
      return true;
    }
  }
}
