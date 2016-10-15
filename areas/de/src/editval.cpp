//
//  File: editval.cpp   originally part of durisEdit
//
//  Usage: user-interface functions for editing strings and numbers
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


#include "fh.h"
#include "de.h"


//
// editStrnVal : get user string input of intMaxLen, restores original colors when done
//

char *editStrnVal(char *strn, const size_t intMaxLen, const char *prompt)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();

  _settextposition(coords.row, 1);

  clrline(coords.row);
  displayColorString(prompt);

  getStrn(strn, intMaxLen, 1, 7, UNUSED_FIELD_CH, strn, false, false);

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return strn;
}


//
// editDieStrnVal : edit a die string value - XdY+Z - max length of 511
//

char *editDieStrnVal(char *strn, const size_t intMaxLen, const char *prompt)
{
  size_t intActualMax;
  char valStrn[512];

  if (intMaxLen > 511)
    intActualMax = 511;
  else
    intActualMax = intMaxLen;

  while (true)
  {
    strcpy(valStrn, strn);

    editStrnVal(valStrn, intActualMax, prompt);

    remTrailingSpaces(valStrn);
    remLeadingSpaces(valStrn);

    if (checkDieStrnValidityShort(valStrn)) 
      break;
  }

  strcpy(strn, lowstrn(valStrn));

  return strn;
}


//
// editStrnValHelp : get user string input of intMaxLen, restores original colors when done,
//                   if user enters ?, set wantedHelp
//

char *editStrnValHelp(char *strn, const size_t intMaxLen, const char *prompt, bool *wantedHelp)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();

  _settextposition(coords.row, 1);

  clrline(coords.row);
  displayColorString(prompt);

  getStrn(strn, intMaxLen, 1, 7, UNUSED_FIELD_CH, strn, false, false);

  if (!strcmp(strn, "?"))
  {
    strn[0] = '\0';

    *wantedHelp = true;
  }
  else
  {
    *wantedHelp = false;
  }

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return strn;
}


//
// editStrnValHelpSearch : get user string input of intMaxLen, restores original colors when done,
//                         if user enters ?, set wantedHelp, if user enters $, set wantedSearch
//

char* editStrnValHelpSearch(char *strn, const size_t intMaxLen, const char *prompt, bool *wantedHelp,
                            bool *wantedSearch)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();

  _settextposition(coords.row, 1);

  clrline(coords.row);
  displayColorString(prompt);

  getStrn(strn, intMaxLen, 1, 7, UNUSED_FIELD_CH, strn, false, false);

  if (!strcmp(strn, "?"))
  {
    strn[0] = '\0';

    *wantedHelp = true;
    *wantedSearch = false;
  }
  else
  if (!strcmp(strn, "$"))
  {
    strn[0] = '\0';

    *wantedHelp = false;
    *wantedSearch = true;
  }
  else
  {
    *wantedHelp = false;
    *wantedSearch = false;
  }

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return strn;
}


//
// editStrnValSearchableList : get user string input of intMaxLen, restores original colors when done,
//                             if user enters ?, display list, if user enters $, display list with search
//
//                             max intMaxLen of 511
//

void editStrnValSearchableList(char *strn, const size_t intMaxLen, const char *editingStrn, 
                               void (*displayListFuncPtr)(const char *))
{
  char promptStrn[512], valstrn[512], editingResized[256];
  bool wantsHelp, wantsSearch;
  size_t intActualMax;


  strncpy(editingResized, editingStrn, 255);
  editingResized[255] = '\0';

  sprintf(promptStrn, "&+CEnter %s (? for list, $ to search): ", editingResized);

  if (intMaxLen > 511)
    intActualMax = 511;
  else
    intActualMax = intMaxLen;

  while (true)
  {
    strncpy(valstrn, strn, 511);
    valstrn[511] = '\0';

    editStrnValHelpSearch(valstrn, intActualMax, promptStrn, &wantsHelp, &wantsSearch);

    if (wantsSearch)
    {
      char searchstrn[128] = "";

      editStrnVal(searchstrn, 127, "&+CSearch for: ");

      if (searchstrn[0])
        displayListFuncPtr(searchstrn);
    }
    else
    if (wantsHelp) 
    {
      displayListFuncPtr(NULL);
    }
    else 
    {
      break;
    }
  }

  strcpy(strn, valstrn);
}


//
// editKeywords : edits keywords; dynamically allocates strn to maxLen
//
//  intMaxLen : max length of string, not including null terminator
//

stringNode *editKeywords(const size_t intMaxLen, stringNode **keywordListHeadPtr)
{
  char *strn;

  strn = new(std::nothrow) char[intMaxLen + 1];
  if (!strn)
  {
    displayAllocError("char[]", "editKeywords");

    return *keywordListHeadPtr;
  }

  createKeywordString(*keywordListHeadPtr, strn, intMaxLen - 1);
  strn[strlen(strn) - 1] = '\0';  // get rid of tilde

  editStrnVal(strn, intMaxLen - 1, "&+CEnter keywords: ");

  remTrailingSpaces(strn);
  remLeadingSpaces(strn);
  strcat(strn, "~");

  deleteStringNodes(*keywordListHeadPtr);
  *keywordListHeadPtr = createKeywordList(strn);

  delete[] strn;

  return *keywordListHeadPtr;
}


//
// editUIntVal : gets a string, returns uint value, keeps prompting if user enters non-numeric value
//
//         val : if non-NULL, set to new value
//   allowZero : if false, zero is considered an invalid value
//      prompt : text displayed as prompt
//

uint editUIntVal(uint *val, const bool allowZero, const char *prompt)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();
  char valStrn[11];


  while (true)
  {
    if (val)
      sprintf(valStrn, "%u", *val);
    else
      strcpy(valStrn, "0");

    _settextposition(coords.row, 1);

    clrline(coords.row);
    displayColorString(prompt);

    getStrn(valStrn, 10, 1, 7, UNUSED_FIELD_CH, valStrn, false, false);

    if (strnumber(valStrn) && (allowZero || (strtoul(valStrn, NULL, 10) > 0)))
      break;
  }

  const uint numb = strtoul(valStrn, NULL, 10);

  if (val)
    *val = numb;

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return numb;
}


//
// editUIntValHelp : gets a string, returns uint value, keeps prompting if user enters non-numeric value;
//                   allows 'help' - if user types '?', sets blnHelp val and returns 0
//
//         val : if non-NULL, set to new value
//   allowZero : if false, zero is considered an invalid value
//      prompt : text displayed as prompt
//

uint editUIntValHelp(uint *val, const bool allowZero, const char *prompt, bool *wantedHelp)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();
  char valStrn[11];


  while (true)
  {
    if (val)
      sprintf(valStrn, "%u", *val);
    else
      strcpy(valStrn, "0");

    _settextposition(coords.row, 1);

    clrline(coords.row);
    displayColorString(prompt);

    getStrn(valStrn, 10, 1, 7, UNUSED_FIELD_CH, valStrn, false, false);

    if (strnumber(valStrn) && (allowZero || (strtoul(valStrn, NULL, 10) > 0)))
      break;

    if (!strcmp(valStrn, "?"))
    {
      *wantedHelp = true;
      _settextcolor(oldfg);
      _setbkcolor(oldbg);

      return 0;
    }
  }

  *wantedHelp = false;

  const uint numb = strtoul(valStrn, NULL, 10);

  if (val)
    *val = numb;

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return numb;
}


//
// editUIntValHelpSearch : gets a string, returns uint value, keeps prompting if user enters non-numeric value;
//                         allows 'help' - if user types '?', sets blnHelp val and returns 0
//                         also allows 'search' - if user types '$', sets blnSearch val and returns 0
//
//         val : if non-NULL, set to new value
//   allowZero : if false, zero is considered an invalid value
//      prompt : text displayed as prompt
//

uint editUIntValHelpSearch(uint *val, const bool allowZero, const char *prompt, bool *wantedHelp,
                           bool *wantedSearch)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();
  char valStrn[11];


  while (true)
  {
    if (val)
      sprintf(valStrn, "%u", *val);
    else
      strcpy(valStrn, "0");

    _settextposition(coords.row, 1);

    clrline(coords.row);
    displayColorString(prompt);

    getStrn(valStrn, 10, 1, 7, UNUSED_FIELD_CH, valStrn, false, false);

    if (strnumber(valStrn) && (allowZero || (strtoul(valStrn, NULL, 10) > 0)))
      break;

    if (!strcmp(valStrn, "?"))
    {
      *wantedHelp = true;
      *wantedSearch = false;

      _settextcolor(oldfg);
      _setbkcolor(oldbg);

      return 0;
    }
    
    if (!strcmp(valStrn, "$"))
    {
      *wantedHelp = false;
      *wantedSearch = true;

      _settextcolor(oldfg);
      _setbkcolor(oldbg);

      return 0;
    }
  }

  *wantedHelp = false;
  *wantedSearch = false;

  const uint numb = strtoul(valStrn, NULL, 10);

  if (val)
    *val = numb;

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return numb;
}


//
// editUIntValSearchableList : gets a string, returns uint val, calls displayListFuncPtr if user enters ? or $
//

void editUIntValSearchableList(uint *val, const bool allowZero, const char *editingStrn, 
                               void (*displayListFuncPtr)(const char *))
{
  char promptStrn[512];
  bool wantsHelp, wantsSearch;


  sprintf(promptStrn, "&+CEnter %s (? for list, $ to search): ", editingStrn);

  while (true)
  {
    editUIntValHelpSearch(val, allowZero, promptStrn, &wantsHelp, &wantsSearch);

    if (wantsSearch)
    {
      char strn[128] = "";

      editStrnVal(strn, 127, "&+CSearch for: ");

      if (strn[0])
        displayListFuncPtr(strn);
    }
    else
    if (wantsHelp) 
    {
      displayListFuncPtr(NULL);
    }
    else 
    {
      break;
    }
  }
}


//
// editIntVal : gets a string, returns int value, keeps prompting if user enters non-numeric value
//
//         val : if non-NULL, set to new value
//   allowZero : if false, zero is considered an invalid value
//      prompt : text displayed as prompt
//

int editIntVal(int *val, const bool allowZero, const char *prompt)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();
  char valStrn[12];


  while (true)
  {
    if (val)
      sprintf(valStrn, "%d", *val);
    else
      strcpy(valStrn, "0");

    _settextposition(coords.row, 1);

    clrline(coords.row);
    displayColorString(prompt);

    getStrn(valStrn, 11, 1, 7, UNUSED_FIELD_CH, valStrn, false, false);

    if (strnumberneg(valStrn) && (allowZero || (atoi(valStrn) != 0)))
      break;
  }

  const int numb = atoi(valStrn);

  if (val)
    *val = numb;

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return numb;
}


//
// editIntValHelp : gets a string, returns int value, keeps prompting if user enters non-numeric value;
//                  allows 'help' - if user types '?', sets blnHelp val and returns 0
//
//         val : if non-NULL, set to new value
//   allowZero : if false, zero is considered an invalid value
//      prompt : text displayed as prompt
//

int editIntValHelp(int *val, const bool allowZero, const char *prompt, bool *wantedHelp)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();
  char valStrn[12];


  while (true)
  {
    if (val)
      sprintf(valStrn, "%d", *val);
    else
      strcpy(valStrn, "0");

    _settextposition(coords.row, 1);

    clrline(coords.row);
    displayColorString(prompt);

    getStrn(valStrn, 11, 1, 7, UNUSED_FIELD_CH, valStrn, false, false);

    if (strnumberneg(valStrn) && (allowZero || (atoi(valStrn) != 0)))
      break;

    if (!strcmp(valStrn, "?"))
    {
      *wantedHelp = true;

      _settextcolor(oldfg);
      _setbkcolor(oldbg);

      return 0;
    }
  }

  *wantedHelp = false;

  const int numb = atoi(valStrn);

  if (val)
    *val = numb;

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return numb;
}


//
// editIntValHelpSearch - gets a string, returns int value, keeps prompting if user enters non-numeric value;
//                        allows 'help' - if user types '?', sets blnHelp val and returns 0
//                        also allows 'search' - if user types '$', sets blnSearch val and returns 0
//
//         val : if non-NULL, sets to new value
//   allowZero : if false, zero is considered an invalid value
//      prompt : text displayed as prompt
//

int editIntValHelpSearch(int *val, const bool allowZero, const char *prompt, bool *wantedHelp,
                         bool *wantedSearch)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();
  char valStrn[12];


  while (true)
  {
    if (val)
      sprintf(valStrn, "%d", *val);
    else
      strcpy(valStrn, "0");

    _settextposition(coords.row, 1);

    clrline(coords.row);
    displayColorString(prompt);

    getStrn(valStrn, 11, 1, 7, UNUSED_FIELD_CH, valStrn, false, false);

    if (strnumberneg(valStrn) && (allowZero || (atoi(valStrn) != 0)))
      break;

    if (!strcmp(valStrn, "?"))
    {
      *wantedHelp = true;
      *wantedSearch = false;

      _settextcolor(oldfg);
      _setbkcolor(oldbg);

      return 0;
    }
    
    if (!strcmp(valStrn, "$"))
    {
      *wantedHelp = false;
      *wantedSearch = true;

      _settextcolor(oldfg);
      _setbkcolor(oldbg);

      return 0;
    }
  }

  *wantedHelp = false;
  *wantedSearch = false;

  const int numb = atoi(valStrn);

  if (val)
    *val = numb;

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return numb;
}


//
// editIntValSearchableList : returns int val of string entered by user, calls display list func if '?' or '$'
//

void editIntValSearchableList(int *val, const bool allowZero, const char *editingStrn, 
                              void (*displayListFuncPtr)(const char *))
{
  char promptStrn[512];
  bool wantsHelp, wantsSearch;


  sprintf(promptStrn, "&+CEnter %s (? for list, $ to search): ", editingStrn);

  while (true)
  {
    editIntValHelpSearch(val, allowZero, promptStrn, &wantsHelp, &wantsSearch);

    if (wantsSearch)
    {
      char strn[128] = "";

      editStrnVal(strn, 127, "&+CSearch for: ");

      if (strn[0])
        displayListFuncPtr(strn);
    }
    else
    if (wantsHelp) 
    {
      displayListFuncPtr(NULL);
    }
    else 
    {
      break;
    }
  }
}


//
// editFloatVal - gets a string, returns float value, keeps prompting if user enters non-numeric value
//
//                     val : if non-NULL, sets to new value
//               allowZero : if false, zero is considered an invalid value
//                  prompt : text displayed as prompt
//   intNumbMantissaDigits : if non-zero, specifies how many digits to show after the decimal, otherwise
//                           uses whatever the default for sprintf() is
//

double editFloatVal(double *val, const bool allowZero, const char *prompt, const uint intNumbMantissaDigits)
{
  const struct rccoord coords = _gettextposition();
  const uchar oldfg = _gettextcolor();
  const uchar oldbg = _getbkcolor();
  char valStrn[128], formatStrn[64];


  if (val)
  {
    if (intNumbMantissaDigits)
      sprintf(formatStrn, "%%.%uf", intNumbMantissaDigits);
    else
      strcpy(formatStrn, "%f");
  }

  while (true)
  {
    if (val)
      sprintf(valStrn, formatStrn, *val);
    else
      strcpy(valStrn, "0.0");

    _settextposition(coords.row, 1);

    clrline(coords.row);
    displayColorString(prompt);

    getStrn(valStrn, 127, 1, 7, UNUSED_FIELD_CH, valStrn, false, false);

    if (strfloat(valStrn) && (allowZero || (atof(valStrn) != 0.0)))
      break;
  }

  const double numb = atof(valStrn);

  if (val)
    *val = numb;

  _settextcolor(oldfg);
  _setbkcolor(oldbg);

  return numb;
}
