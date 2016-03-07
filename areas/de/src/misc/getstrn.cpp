//
//  File: getstrn.cpp    originally part of durisEdit
//
//  Usage: getString function and supporting
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

#include "fh.h"
#include "types.h"
#include "keys.h"
#include "graphcon.h"
#include "getstrn.h"

extern stringNode *g_commandHistory;

//
// dispGetStrnField : Displays the visible part of getStrn field
//
//             strn : string - not const because char is set to null and then reset
//         startPos : position in strn to start displaying at
//
//           maxLen : width of getStrn input field - well uhh, sort of
//       wideString : set to true if maxLen > maxDisplayable (it's handy)
//   maxDisplayable : maximum displayable in field onscreen
//
//                x : x-coordinate where field starts
//                y : y-coordinate
//
//     fillerChStrn : if non-NULL, 'empty' part of field is filled with this
//

void dispGetStrnField(char *strn, const size_t startPos, const size_t maxLen,
                      const bool wideString, const uint maxDisplayable,
                      const sint x, const sint y, const char *fillerChStrn)
{
  _settextposition(y, x);

  if (wideString)
  {
    char *substrn = strn + startPos, *substrnend;
    char origch;
    bool blnMiddle = false;

    if (startPos + maxDisplayable < maxLen)
    {
      substrnend = substrn + maxDisplayable;

      origch = *substrnend;
      *substrnend = '\0';

      blnMiddle = true;
    }

    _outtext(substrn);

    if (fillerChStrn)
    {
// under UNIX, use a gray background color and spaces since the high-bit char used otherwise
// looks nutty - but only if the BG for the field isn't black (as it is for the main prompt)

#ifdef __UNIX__
      const uchar bg = _getbkcolor();

      if (bg != 0)
      {
        _setbkcolor(7);  // gray
      }
#endif

      for (size_t i = strlen(substrn); i < maxDisplayable; i++)
        _outtext((char *)fillerChStrn);

#ifdef __UNIX__
      if (bg != 0)
      {
        _setbkcolor(bg);
      }
#endif
    }

    if (blnMiddle)
      *substrnend = origch;
  }
  else
  {
    _outtext(strn);

    if (fillerChStrn)
    {
// under UNIX, use a gray background color and spaces since the high-bit char used otherwise
// looks nutty - but only if the BG for the field isn't black (as it is for the main prompt)

#ifdef __UNIX__
      const uchar bg = _getbkcolor();

      if (bg != 0)
      {
        _setbkcolor(7);  // gray
      }
#endif

      for (size_t i = strlen(strn); i < maxLen; i++)
        _outtext((char *)fillerChStrn);

#ifdef __UNIX__
      if (bg != 0)
      {
        _setbkcolor(bg);
      }
#endif
    }
  }
}


//
// getStrn : Gets a string from the user - spaces from cursor pos to
//           cursor pos + maxLen get filled with bgcol-colored fillerCh.
//           Text is drawn in fgcol, bgcol.  oldStrn is put into input string
//           if it is passed to the function.  lalala.
//
//     *strn : string that contains the string user entered
//    maxLen : maximum length of string
//
//     bgcol : background color
//     fgcol : foreground color
//
//  fillerCh : character used for "empty field"
//  *oldStrn : old string - if a non-null string, is in field at start
//
//  addToCommandHist :
//             if TRUE, adds the string entered to the global history
//             list
//
//    prompt : if TRUE, the escape key is treated differently - it clears the
//             input field like Ctrl-Y instead of returning oldStrn
//

char *getStrn(char *strn, const size_t maxLen, const char bgcol, const char fgcol, const uchar fillerCh, 
              const char *oldStrn, const bool addToCommandHist, const bool prompt)
{
 // if oldStrn and strn are one and the same, then toss off to version that creates its own internal buffer
 // for storing oldStrn

  if (strn == oldStrn)
  {
    return getStrn(strn, maxLen, bgcol, fgcol, fillerCh, addToCommandHist, prompt);
  }

 // variable declaration & initialization

  size_t strnPos = 0;   // points to where the next character
                        // to be typed should go

  size_t endofStrn = 0; // points to where the null character
                        // should be placed

  size_t startPos = 0;  // position in editStrn that field
                        // starts at onscreen - used for
                        // fields that are wider than one
                        // line can fit

  char fillerChStrn[2]; // _outtext only takes strings, for the "empty" parts of the
                        // input field

  usint ch;      // used for character input
  const size_t oldLen = strlen(oldStrn);
                 // golly

  const struct rccoord xy = _gettextposition();
  const usint startx = xy.col, starty = xy.row;
                                // used to store start position of field

  bool firstHistMod = true;     // I forget what this is for, but it's a
                                // very good variable

  const uint maxDisplayable = getScreenWidth() - startx;
     // right-most column shouldn't be filled - at any rate, this variable
     // stores the number of characters displayable on the screen at once

  const bool wideString = (maxLen > maxDisplayable);
     // if the max string length is longer than is displayable on the screen
     // at once, this is TRUE

  stringNode *commandHistPos = g_commandHistory;
     // will point to end of command history

  const uchar startfg = _gettextcolor(), startbg = _getbkcolor();



 // for errors, strn should be at least one char - numerics help to create self-repairing issues

  if (oldLen > maxLen)
  {
    _outtext("getStrn err #1");
    strcpy(strn, "1");
    return strn;
  }

 // preset stuff

  fillerChStrn[0] = fillerCh;
  fillerChStrn[1] = '\0';

 // get to end of command history

  if (commandHistPos)
  {
    while (commandHistPos->Next)
      commandHistPos = commandHistPos->Next;
  }

 // copy oldStrn into strn

  strcpy(strn, oldStrn);

 // set strnPos and endofStrn

  if (wideString && (oldLen > maxDisplayable))
    startPos = oldLen - maxDisplayable;

  strnPos = endofStrn = oldLen;

 // set colors

  _settextcolor(fgcol);
  _setbkcolor(bgcol);

 // display strn

  dispGetStrnField(strn, startPos, maxLen, wideString, maxDisplayable,
                   startx, starty, fillerChStrn);

 // reset the text position to be right after the old string

  _settextposition(starty, startx + (sint)(strnPos - startPos));  // let's assume

 // now, the input stuff

 // the first character entered has significance because ...  well just because
 // so treat it differently

  do
  {
    ch = getkey();
  } while ((ch != K_Escape) && (ch != K_Return) && (ch != K_LeftArrow)
        && (ch != K_RightArrow) && (ch != K_End) && (ch != K_Home)
        && ((ch < K_FirstPrint) || (ch > K_LastPrint))
        && (ch != K_Backspace) && (ch != K_Tab) && (ch != K_Ctrl_Y)
        && (ch != K_Delete) && ((ch != K_UpArrow) && (addToCommandHist))
        && ((ch != K_DownArrow) && (addToCommandHist)));

 // user hit escape or return ..

  if (((ch == K_Escape) && !prompt) || (ch == K_Return))
  {
    if (addToCommandHist) 
      addCommand(strn, &g_commandHistory);
    
    _settextcolor(startfg);
    _setbkcolor(startbg);

    return strn;
  }
  else
  if ((ch >= K_FirstPrint) && (ch <= K_LastPrint))
  {
    strn[0] = (char)ch;  // known to be in range..
    strn[1] = '\0';

    strnPos = endofStrn = 1;
    startPos = 0;

    dispGetStrnField(strn, startPos, maxLen, wideString,
                     maxDisplayable, startx, starty, fillerChStrn);

    _settextposition(starty, startx + 1);
  }
  else
  if ((ch == K_Backspace) || (ch == K_Ctrl_Y) || (ch == K_Delete) ||
      ((ch == K_Escape) && prompt))
  {
    strn[0] = '\0';
    strnPos = endofStrn = startPos = 0;

    dispGetStrnField(strn, startPos, maxLen, wideString,
                     maxDisplayable, startx, starty, fillerChStrn);

    _settextposition(starty, startx);

  }
  else
  if ((ch == K_LeftArrow) && strnPos)
  {
    strnPos--;

    _settextposition(starty, startx + (sint)(strnPos - startPos));  // assume
  }
  else
  if (ch == K_Home)
  {
    strnPos = startPos = 0;

    dispGetStrnField(strn, startPos, maxLen, wideString,
                     maxDisplayable, startx, starty, NULL);

    _settextposition(starty, startx);
  }
  else
  if ((ch == K_UpArrow) && addToCommandHist && commandHistPos && (commandHistPos->Last || firstHistMod))
  {
    if (!firstHistMod) 
      commandHistPos = commandHistPos->Last;
    else 
      firstHistMod = false;

    if (commandHistPos)
    {
      strncpy(strn, commandHistPos->string, maxLen - 1);
      strn[maxLen - 1] = '\0';

      strnPos = endofStrn = strlen(strn);

      if (endofStrn > maxDisplayable) 
        startPos = endofStrn - maxDisplayable;
      else 
        startPos = 0;

      dispGetStrnField(strn, startPos, maxLen, wideString, maxDisplayable, startx, starty, fillerChStrn);

      _settextposition(starty, startx + (sint)(strnPos - startPos));
    }
  }

 // loop for handling all keystrokes after the first

  while (true)
  {
   // get a character from the user

    ch = getkey();

    if ((endofStrn < maxLen) && (ch >= K_FirstPrint) && (ch <= K_LastPrint))
    {
      if (strnPos != endofStrn)
        insertChar(strn, strnPos, (char)ch);  // known to be in range..
      else
        strn[strnPos] = (char)ch;  // known to be in range..

      if (wideString && ((strnPos - startPos) >= maxDisplayable))
        startPos++;

      strnPos++;
      endofStrn++;
      strn[endofStrn] = '\0';

      if (wideString)
      {
        dispGetStrnField(strn, startPos, maxLen, wideString,
                         maxDisplayable, startx, starty, NULL);
      }
      else
      {
        _settextposition(starty, startx);
        _outtext(strn);
      }

      _settextposition(starty, startx + (sint)(strnPos - startPos));
    }
    else
    if (ch == K_Tab)
    {
    }
    else
    if ((ch == K_Ctrl_Y) || ((ch == K_Escape) && prompt))
    {
      strn[0] = '\0';
      strnPos = endofStrn = startPos = 0;

      dispGetStrnField(strn, startPos, maxLen, wideString,
                       maxDisplayable, startx, starty, fillerChStrn);

      _settextposition(starty, startx);
    }
    else

   // user hit escape, input field isn't prompt, so restore the field to the
   // previous

    if ((ch == K_Escape) && !prompt)
    {
      _settextposition(starty, startx);

      strcpy(strn, oldStrn);

      startPos = 0;

      dispGetStrnField(strn, startPos, maxLen, wideString,
                       maxDisplayable, startx, starty, " ");

      if (addToCommandHist) 
        addCommand(strn, &g_commandHistory);
      
      _settextcolor(startfg);
      _setbkcolor(startbg);

      return strn;
    }
    else
    if (ch == K_Return)
    {
      strn[endofStrn] = '\0';

      dispGetStrnField(strn, startPos, maxLen, wideString,
                       maxDisplayable, startx, starty, " ");

      if (addToCommandHist) 
        addCommand(strn, &g_commandHistory);

      _settextcolor(startfg);
      _setbkcolor(startbg);

      return strn;
    }
    else
    if ((ch == K_Backspace) && (strnPos > 0))
    {
      deleteChar(strn, strnPos - 1);

      if (wideString && (strnPos == startPos)) 
        startPos--;
      strnPos--;
      endofStrn--;
      strn[endofStrn] = '\0';

      _settextposition(starty, startx);

      if (wideString)
      {
        dispGetStrnField(strn, startPos, maxLen, wideString,
                         maxDisplayable, startx, starty, fillerChStrn);
      }
      else
      {
        _settextposition(starty, (sint)(startx + strnPos));

        _outtext(strn + strnPos);

        _outtext(fillerChStrn);
      }

      _settextposition(starty, startx + (sint)(strnPos - startPos));
    }
    else
    if ((ch == K_Delete) && (strnPos != endofStrn))
    {
      deleteChar(strn, strnPos);
      endofStrn--;
      strn[endofStrn] = '\0';

      if (wideString)
      {
        dispGetStrnField(strn, startPos, maxLen, wideString,
                         maxDisplayable, startx, starty, fillerChStrn);
      }
      else
      {
        _settextposition(starty, (sint)(startx + strnPos));

        _outtext(strn + strnPos);

        _outtext(fillerChStrn);
      }

      _settextposition(starty, startx + (sint)(strnPos - startPos));
    }
    else
    if ((ch == K_LeftArrow) && strnPos)
    {
      if (wideString && (strnPos == startPos))
      {
        startPos--;

        dispGetStrnField(strn, startPos, maxLen, wideString,
                         maxDisplayable, startx, starty, NULL);
      }

      strnPos--;

      _settextposition(starty, startx + (sint)(strnPos - startPos));
    }
    else
    if ((ch == K_RightArrow) && (strnPos != endofStrn))
    {
     // if field is wider than screen, check to see if we're at the edge of
     // the field - need to scroll it over

      if (wideString && ((strnPos - startPos) == maxDisplayable))
      {
        startPos++;

        dispGetStrnField(strn, startPos, maxLen, wideString,
                         maxDisplayable, startx, starty, NULL);
      }

      strnPos++;

      _settextposition(starty, startx + (sint)(strnPos - startPos));
    }
    else
    if (ch == K_End)
    {
      if (wideString && (endofStrn > maxDisplayable))
        startPos = endofStrn - maxDisplayable;
      else 
        startPos = 0;

      strnPos = endofStrn;

      dispGetStrnField(strn, startPos, maxLen, wideString,
                       maxDisplayable, startx, starty, fillerChStrn);

      _settextposition(starty, startx + (sint)(strnPos - startPos));
    }
    else
    if (ch == K_Home)
    {
      strnPos = startPos = 0;

      dispGetStrnField(strn, startPos, maxLen, wideString,
                       maxDisplayable, startx, starty, fillerChStrn);

      _settextposition(starty, startx);
    }
    else
    if ((ch == K_UpArrow) && addToCommandHist && g_commandHistory)
    {
      if (!firstHistMod && commandHistPos && commandHistPos->Last) 
      {
        commandHistPos = commandHistPos->Last;
      }
      else
      {
        firstHistMod = false;

        if (!commandHistPos)
        {
         // get last one - if commandHistPos is NULL but g_commandHistory isn't
         // it means user 'down arrowed' past last entry to clear line

          commandHistPos = g_commandHistory;

          while (commandHistPos->Next) 
            commandHistPos = commandHistPos->Next;
        }
      }

      if (commandHistPos)
      {
        strncpy(strn, commandHistPos->string, maxLen - 1);
        strn[maxLen - 1] = '\0';

        strnPos = endofStrn = strlen(strn);

        if (endofStrn > maxDisplayable) 
          startPos = endofStrn - maxDisplayable;
        else 
          startPos = 0;

        dispGetStrnField(strn, startPos, maxLen, wideString, maxDisplayable,
                         startx, starty, fillerChStrn);

        _settextposition(starty, startx + (sint)(strnPos - startPos));
      }
    }
    else
    if ((ch == K_DownArrow) && addToCommandHist)
    {
      if (commandHistPos && commandHistPos->Next)
      {
        commandHistPos = commandHistPos->Next;

        strncpy(strn, commandHistPos->string, maxLen - 1);
        strn[maxLen - 1] = '\0';

        strnPos = endofStrn = strlen(strn);

        if (endofStrn > maxDisplayable) 
          startPos = endofStrn - maxDisplayable;
        else 
          startPos = 0;
      }
      else
      {
        strn[0] = '\0';
        strnPos = endofStrn = startPos = 0;
        commandHistPos = NULL;
      }

      dispGetStrnField(strn, startPos, maxLen, wideString, maxDisplayable,
                       startx, starty, fillerChStrn);

      _settextposition(starty, startx + (sint)(strnPos - startPos));
    }
  }
}


//
// version of getStrn that doesn't require oldStrn
//

char *getStrn(char *strn, const size_t maxLen, const char bgcol, const char fgcol, const uchar fillerCh, 
              const bool addToCommandHist, const bool prompt)
{
  char oldStrn[MAX_GETSTRN_LEN + 1];
  char *oldptr;
  size_t actualMax;


  if (strlen(strn) > MAX_GETSTRN_LEN)
  {
    actualMax = strlen(strn);
    oldptr = new char[actualMax + 1];

    if (!oldptr)
    {
      _outtext("getStrn err #2");
      strcpy(strn, "2");
      return strn;
    }
  }
  else
  {
    actualMax = maxLen;
    oldptr = oldStrn;
  }

  strcpy(oldptr, strn);

  char *retval = getStrn(strn, actualMax, bgcol, fgcol, fillerCh, oldptr, addToCommandHist, prompt);

  if (oldptr != oldStrn)
    delete[] oldptr;

  return retval;
}
