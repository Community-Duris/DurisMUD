//
//  File: strings.cpp    originally part of durisEdit
//
//  Usage: functions galore for manipulating strings
//

/*
  *Copyright (c) 1995-2007, Michael Glosenger
  *All rights reserved.
  *Redistribution and use in source and binary forms, with or without
  *modification, are permitted provided that the following conditions are met:
 *
  *     *Redistributions of source code must retain the above copyright
  *      notice, this list of conditions and the following disclaimer.
  *     *Redistributions in binary form must reproduce the above copyright
  *      notice, this list of conditions and the following disclaimer in the
  *      documentation and/or other materials provided with the distribution.
  *     *The name of Michael Glosenger may not be used to endorse or promote 
  *      products derived from this software without specific prior written 
  *      permission.
 *
  *THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
  *IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  *MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
  *EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  *SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
  *PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
  *OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
  *WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
  *OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  *ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <ctype.h>
#include "../types.h"
#include "../boolean.h"
#include "../fh.h"


//
// deleteChar: strnWork is pointer to char to delete
//

char *deleteChar(char *strn, char *strnWork)
{
  if (strnWork >= (strn + strlen(strn)))
    return strn;

  char *strnNext = strnWork + 1;

  while (*strnWork != '\0')
  {
    *strnWork = *strnNext;

    strnWork++;
    strnNext++;
  }

  return strn;
}


//
// deleteChar : "Deletes" the character in the specified position in the
//              specified string, moving all characters after the deleted
//              character back one.  The new string's address is returned
//
//     *strn : string to alter
//   strnPos : char position to delete
//

char *deleteChar(char *strn, const size_t strnPos)
{
  return deleteChar(strn, strn + strnPos);
}


//
// insertChar : Inserts a character at the position in the string passed to the
//              function
//
//    *strn : pointer to string
//  strnPos : position to insert character at
//       ch : character to insert
//

char *insertChar(char *strn, const size_t strnPos, const char ch)
{
  char *strnPre = strn + strlen(strn);
  char *strnWork = strnPre + 1;
  char *strnStop = strn + strnPos;


 // move chars one over, including null terminator

  while (strnWork != strnStop)
  {
    *strnWork = *strnPre;

    strnWork--;
    strnPre--;
  }

 // add char

  *strnStop = ch;

  return strn;
}


//
// remLeadingSpaces : Removes spaces from the start of a string, returning the
//                    new string.
//
//  *strn : string to alter
//

char *remLeadingSpaces(char *strn)
{
  while (*strn == ' ') 
    deleteChar(strn, strn);

  return strn;
}


//
// remTrailingSpaces : Removes spaces from the end of a string, returning the
//                     new string, as well as altering the one passed to the
//                     function.
//
//  *strn : string to alter
//

char *remTrailingSpaces(char *strn)
{
  char *strnWork = strn + strlen(strn) - 1;


  while (strnWork >= strn)
  {
    if (*strnWork == ' ') 
    {
      *strnWork = '\0';
      strnWork--;
    }
    else 
    {
      break;
    }
  }

  return strn;
}


//
// remSpacesBetweenArgs : Removes excess spaces between "arguments"
//

char *remSpacesBetweenArgs(char *strn)
{
  char *strnWork = strn;

  while (*strnWork != '\0')
  {
    if (*strnWork == ' ')
    {
      while (*(strnWork + 1) == ' ')
      {
        deleteChar(strn, strnWork + 1);
      }
    }

    strnWork++;
  }

  return strn;
}


//
// nolf : "Removes" linefeeds ('\n') and carriage returns ('\r') from strings
//        by changing them into null characters - assumes LF/CRs are at end of
//        string
//
//  *strn : string to alter
//

char *nolf(char *strn)
{
  char *strnWork = strn;


  while (*strnWork != '\0')
  {
    if ((*strnWork == '\n') || (*strnWork == '\r'))
    {
      *strnWork = '\0';
      break;
    }

    strnWork++;
  }

  return strn;
}


//
// numbArgs : returns number of arguments in string
//

size_t numbArgs(const char *strn)
{
  size_t intArgs = 0;
  bool blnWord = false;


  while (*strn != '\0')
  {
    if (isspace(*strn))
    {
      blnWord = false;
    }
    else 
    if (!blnWord)
    {
      intArgs++;
      blnWord = true;
    }

    strn++;
  }

  return intArgs;
}


//
// getArg : return specified whitespace-separated argument, 1 = first arg
//
//          intMaxLen is max len not including null
//      intCopied can be NULL and contains the amount of chars copied if not
//

char *getArg(const char *strn, const size_t argNumb, char *arg, const size_t intMaxLen, size_t *intCopied)
{
  size_t intArgs = 0;
  bool blnWord = false;


  while (*strn != '\0')
  {
    if (isspace(*strn))
    {
      blnWord = false;
    }
    else 
    if (!blnWord)
    {
      intArgs++;
      blnWord = true;

     // got the arg, grab it

      if (argNumb == intArgs)
      {
        char *argWork = arg;
        size_t len = 0;

        while ((*strn != '\0') && !isspace(*strn) && (len < intMaxLen))
        {
          *argWork = *strn;

          argWork++;
          strn++;
          len++;
        }

        *argWork = '\0';

        if (intCopied)
          *intCopied = len;

        return arg;
      }
    }

    strn++;
  }

 // not enough args

  *arg = '\0';

  if (intCopied)
    *intCopied = 0;

  return arg;
}


//
// getArg : four arg version, passes NULL for intCopied
//

char *getArg(const char *strn, const size_t argNumb, char *arg, const size_t intMaxLen)
{
  return getArg(strn, argNumb, arg, intMaxLen, NULL);
}


//
// upstrn : Upcases an entire string, altering the original and returning
//          the address of the altered string
//
//  *strn : string to upcase
//

char *upstrn(char *strn)
{
  char *strnWork = strn;


  while (*strnWork != '\0')
  {
    *strnWork = toupper(*strnWork);

    strnWork++;
  }

  return strn;
}


//
// upfirstarg : uppercases only the first argument in the string (assumes
//              no leading spaces)
//

char *upfirstarg(char *strn)
{
  char *strnWork = strn;

  while ((*strnWork != '\0') && (*strnWork != ' '))
  {
    *strnWork = toupper(*strnWork);

    strnWork++;
  }

  return strn;
}


//
// lowstrn : Lowercases an entire string, altering the original and returning
//           the address of the altered string
//
//  *strn : string to lowercase
//

char *lowstrn(char *strn)
{
  char *strnWork = strn;


  while (*strnWork != '\0')
  {
    *strnWork = tolower(*strnWork);

    strnWork++;
  }

  return strn;
}


//
// strnumber : returns TRUE if string is composed entirely of numerics, FALSE on 0 length string
//

bool strnumber(const char *strn)
{
  if (*strn == '\0')
    return false;

  while (*strn != '\0')
  {
    if (!isdigit(*strn)) 
      return false;

    strn++;
  }

  return true;
}


//
// strnumberneg : returns TRUE if string is composed entirely of numerics, FALSE on 0 length string - allows
//               negative sign at the start
//

bool strnumberneg(const char *strn)
{
  if (*strn == '-')
    strn++;

  if (*strn == '\0')
    return false;

  while (*strn != '\0')
  {
    if (!isdigit(*strn)) 
      return false;

    strn++;
  }

  return true;
}


//
// strfloat : returns true if string is a valid positive float
//

bool strfloat(const char *strn)
{
 // first, check for numbers

  if (*strn == '\0')
    return false;

  while (*strn != '\0')
  {
    if (!isdigit(*strn))
    {
      if (*strn == '.')
        break;
      else
        return false;
    }

    strn++;
  }

 // got all numbers, no decimal point

  if (*strn == '\0')
    return true;

 // go past decimal point

  strn++;

  while (*strn != '\0')
  {
    if (!isdigit(*strn))
      return false;

    strn++;
  }

  return true;
}


//
// strcmpnocase : returns true iff two strings match when ignoring case - would use strcmpi except that
//                it seems popular as a non-standard extension to the C library
//

bool strcmpnocase(const char *strn1, const char *strn2)
{
  while (*strn1 && *strn2)
  {
    if (toupper(*strn1) != toupper(*strn2))
      return false;

    strn1++;
    strn2++;
  }

 // if both pointers are on NULL chars, then strings match

  return (*strn1 == *strn2);
}


//
// strcmpnocasecount : returns true iff two strings match for a certain number of chars when ignoring case - 
//                     if end of strings is hit before count is fulfilled, returns true
//

bool strcmpnocasecount(const char *strn1, const char *strn2, const size_t intCount)
{
  size_t intCur = 0;

  while (*strn1 && *strn2 && (intCur < intCount))
  {
    if (toupper(*strn1) != toupper(*strn2))
      return false;

    strn1++;
    strn2++;
    intCur++;
  }

  return (intCur == intCount) || ((*strn1 == '\0') && (*strn2 == '\0'));
}


//
// strstrnocase : returns true iff strn contains substrn somewhere, ignoring case
//

bool strstrnocase(const char *strn, const char *substrn)
{
  while (*strn)
  {
    const char *strnptr = strn;
    const char *subptr = substrn;

    while (*subptr && *strnptr)
    {
      if (toupper(*strnptr) != toupper(*subptr))
        break;

      subptr++;
      strnptr++;
    }

   // if the above loop reached the end of substrn then it matched

    if (*subptr == '\0')
      return true;

    strn++;
  }

  return false;
}


//
// strleft : Checks to see if a substrn is on the "left" side of strn,
//           returning TRUE if so and FALSE if not
//
//    *strn : string to search for substrn in
// *substrn : substring to search for
//

bool strleft(const char *strn, const char *substrn)
{
  while ((*strn != '\0') && (*substrn != '\0'))
  {
    if (*strn != *substrn)
      return false;

    strn++;
    substrn++;
  }

  return (*substrn == '\0');
}


//
// strlefti : Like strleft(), but ignores case
//
//    *strn : string to search for substrn in
// *substrn : substring to search for
//

bool strlefti(const char *strn, const char *substrn)
{
  while ((*strn != '\0') && (*substrn != '\0'))
  {
    if (toupper(*strn) != toupper(*substrn))
      return false;

    strn++;
    substrn++;
  }

  return (*substrn == '\0');
}


//
// strlefticount : Like strlefti(), but only check substrn for count
//
//      *strn : string to search for substrn in
//   *substrn : substring to search for
//   intCount : how many chars
//

bool strlefticount(const char *strn, const char *substrn, const size_t intCount)
{
  size_t intCur = 0;

  while ((*strn != '\0') && (*substrn != '\0') && (intCur < intCount))
  {
    if (toupper(*strn) != toupper(*substrn))
      return false;

    strn++;
    substrn++;
    intCur++;
  }

  return (intCur == intCount);
}


//
// strright : Checks if substrn exists in the "right" side of strn - if so,
//            TRUE is returned, if not, FALSE
//
//    *strn : string to search
// *substrn : string to search for inside of strn
//

bool strright(const char *strn, const char *substrn)
{
  const size_t len = strlen(strn), sublen = strlen(substrn);


  if ((sublen > len) || (sublen == 0))
    return false;

  const char *strnPos = strn + len - 1;
  const char *subPos = substrn + sublen - 1;

  while (subPos >= substrn)
  {
    if (*subPos != *strnPos)
      return false;

    strnPos--;
    subPos--;
  }

  return true;
}


//
// strrighti : Like strright, but ignores case
//
//    *strn : string to search
// *substrn : string to search for inside of strn
//

bool strrighti(const char *strn, const char *substrn)
{
  const size_t len = strlen(strn), sublen = strlen(substrn);


  if ((sublen > len) || (sublen == 0))
    return false;

  const char *strnPos = strn + len - 1;
  const char *subPos = substrn + sublen - 1;

  while (subPos >= substrn)
  {
    if (toupper(*subPos) != toupper(*strnPos))
      return false;

    strnPos--;
    subPos--;
  }

  return true;
}


//
// strnSect : Runs through strn, putting the section of strn from start to end inclusive
//            in newStrn.  the address of newStrn is returned
//

char *strnSect(const char *strn, char *newStrn, const size_t start, size_t end)
{
  const size_t len = strlen(strn);


  if (start >= len)
  {
    newStrn[0] = '\0';

    return newStrn;
  }

  const char *strnEnd = strn;

  if (end >= len) 
    strnEnd += len - 1;
  else
    strnEnd += end;

  strn += start;

  while (strn <= strnEnd)
  {
    *newStrn = *strn;

    newStrn++;
    strn++;
  }

  *newStrn = '\0';

  return newStrn;
}


//
// numbPercentS: returns number of '%s' substrings in strn
//

size_t numbPercentS(const char *strn)
{
  size_t found = 0;


  while (*strn != '\0')
  {
    if ((*strn == '%') && (toupper(*(strn + 1)) == 'S')) 
      found++;

    strn++;
  }

  return found;
}


//
// lastCharLF : returns TRUE if last char is \n, FALSE otherwise
//

bool lastCharLF(const char *strn)
{
  if (!strn || !strn[0]) 
    return false;

  return (strn[strlen(strn) - 1] == '\n');
}


//
// numbLinefeeds
//

size_t numbLinefeeds(const char *strn)
{
  size_t numb = 0;


  while (*strn != '\0')
  {
    if (*strn == '\n') 
      numb++;

    strn++;
  }

  return numb;
}


//
// truestrlen : checks for ANSI codes, doesn't count em in length - only
//              truly accurate for strings with no linefeeds (linefeed
//              characters [\n] are not counted as part of length)
//

size_t truestrlen(const char *strn)
{
  uint numbSkip = 0;
  size_t len = 0;


  if (!strn) 
    return 0;

 // if color codes are shown, we count pretty much everything

  if (getShowColorVal()) 
    return strlen(strn) - numbLinefeeds(strn);

  while (*strn != '\0')
  {
    if (*strn == '&')
    {
      numbSkip = durisANSIcode(strn, 0);
    }

    if (numbSkip)
    {
      strn += numbSkip;
      numbSkip = 0;
    }
    else
    {
      if (*strn != '\n') 
        len++;

      strn++;
    }
  }

  return len;
}

// Make the first non-ansi char uppercase.
void CAP( char *strn )
{
  int count = 0, tmp;

  // While possible ansi start.
  while( strn[count] == '&' )
  {
    // If we don't have an actual ansi code, break instead of endless loop.
    if( (tmp = durisANSIcode( strn, count )) == 0 )
    {
      break;
    }
    count += tmp;
  }
  strn[count] = toupper(strn[count]);
}

// Make the first non-ansi char lowercase.
void DECAP( char *strn )
{
  int count = 0, tmp;

  // While possible ansi start.
  while( strn[count] == '&' )
  {
    // If we don't have an actual ansi code, break instead of endless loop.
    if( (tmp = durisANSIcode( strn, count )) == 0 )
    {
      break;
    }
    count += tmp;
  }
  strn[count] = tolower(strn[count]);
}

// Returns true iff there's no &n or &N terminating any color codes.
bool bleeding_ansi( const char *strn )
{
  bool bleeding = FALSE;
  int count = 0, tmp;

  // While there's more to peruse
  while( strn[count] != '\0' )
  {
    if( strn[count] == '&' )
    {
      // If we don't have an actual ansi code, move to the next character.
      if( (tmp = durisANSIcode( strn, count )) == 0 )
      {
        count++;
      }
      else
      {
        // Jump past the ansi code.
        count += tmp;
        // If tmp == 2, then we have an &n, so not bleeding..
        bleeding = (tmp != 2);
      }
    }
    else
    {
      count++;
    }
  }
  return bleeding;
}
