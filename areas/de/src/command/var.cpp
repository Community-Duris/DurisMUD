//
//  File: var.cpp        originally part of durisEdit
//
//  Usage: functions for handling variable-related functions and
//         internal structures
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

#include "../types.h"
#include "../fh.h"
#include "../keys.h"

#include "var.h"
#include "../vardef.h"
#include "../system.h"



extern bool g_madeChanges;

//
// displayVarSettings : used in addVar, toggleVar, and varCmd; displays settings for particular var,
//                      returns false if user quit out of pause
//
//         var : variable
//     preStrn : shown at very start of line
//   numbLines : number of lines
//

bool displayVarSettings(const variable *var, const char *preStrn, size_t& numbLines)
{
  displayColorString(preStrn);

  displayColorString("&+c");
  _outtext(var->varName);
  displayColorString("&n is set to '&+Y");
  _outtext(var->varValue);
  displayColorString("&n'\n");

  char outstrn[MAX_VARNAME_LEN + MAX_VARVAL_LEN + 128];

  sprintf(outstrn, "  &+c%s&n is set to '&+Y%s&n'\n",
          var->varName, var->varValue);

  numbLines += numbLinesStringNeeds(outstrn);  // close enough

  return !checkPause(NULL, numbLines);
}


//
// unvarCmd : removes a variable from the list pointed to by varHead
//            based on the user input in args
//
//      args : arguments entered by user
//   varHead : pointer to pointer to head of list of variables
//

void unvarCmd(const char *args, variable **varHead)
{
  variable *node = *varHead, *old = NULL;


  if (numbArgs(args) != 1)
  {
    _outtext("\nThe 'unset' command takes one argument - the variable to delete.\n\n");
    return;
  }

  if (!node)
  {
    _outtext("\nNo variables are defined.\n\n");
    return;
  }

  while (node)
  {
    if (strcmpnocase(node->varName, args))
    {
      if (node == *varHead) 
        *varHead = (*varHead)->Next;

      if (old) 
        old->Next = node->Next;

      displayColorString("\nDeleted variable '&+c");
      _outtext(node->varName);
      displayColorString("'\n\n");

      createPrompt();  // might have deleted prompt var

      delete node;

      g_madeChanges = true;

      return;
    }

    old = node;
    node = node->Next;
  }

  _outtext("\nNo matching variable found.\n\n");
}


//
// addVar : returns value based on what happened - adds a variable to a
//          variable list with specified var name and value
//
//   varHead : pointer to pointer to head of linked list of vars
//   varStrn : name of variable
//    instrn : value of variable
//

char addVar(variable **varHead, const char *origVarStrn, const char *instrn)
{
  variable *node = *varHead, *adding, *old = NULL;
  char strn[MAX_VARVAL_LEN], varStrn[MAX_VARNAME_LEN];


  strncpy(strn, instrn, MAX_VARVAL_LEN - 1);
  strncpy(varStrn, origVarStrn, MAX_VARNAME_LEN - 1);

  strn[MAX_VARVAL_LEN - 1] = '\0';
  varStrn[MAX_VARNAME_LEN - 1] = '\0';

 // ensure that screen width/height never go below minimum

  if (strcmpnocase(varStrn, VAR_SCREENWIDTH_NAME) && strlen(strn) &&
      (strtoul(strn, NULL, 10) < MINIMUM_SCREEN_WIDTH))
    return VAR_BELOW_MINIMUM;

  if (strcmpnocase(varStrn, VAR_SCREENHEIGHT_NAME) && strlen(strn) &&
      (strtoul(strn, NULL, 10) < MINIMUM_SCREEN_HEIGHT))
    return VAR_BELOW_MINIMUM;

 // check for dupes

  while (node)
  {
    if ((strcmpnocase(node->varName, varStrn)))
    {
      if (!strlen(strn))  // display var contents
      {
        size_t numbLines = 0;

        displayVarSettings(node, "", numbLines);

        return VAR_DISPLAY;
      }
      else
      {
        if (!strcmp(node->varValue, strn)) 
          return VAR_SAMEVAL;

        strcpy(node->varValue, strn);
        return VAR_REPLACED;
      }
    }

    old = node;
    node = node->Next;
  }

  if (!strlen(strn))  // no match for var name, ignore it
    return VAR_BLANK;

  node = *varHead;

  adding = new(std::nothrow) variable;
  if (!adding)
  {
    displayAllocError("variable", "addVar");

    return VAR_ERROR;
  }

  memset(adding, 0, sizeof(variable));

  strcpy(adding->varName, varStrn);
  strcpy(adding->varValue, strn);

  if (!*varHead) 
  {
    *varHead = adding;
  }
  else
  {
    while (node->Next)
      node = node->Next;

    node->Next = adding;
  }

  return VAR_ADDED;
}


//
// setVarArgs : sets a var with an argument string - called from varCmd
//
//        varHead : pointer to variable list
//           args : arguments entered by user
//         addLFs : if TRUE, linefeeds are added to output
//  updateChanges : if TRUE, g_madeChanges is updated if changes are made
//

void setVarArgs(variable **varHead, const char *args, const bool addLFs, const bool updateChanges,
                const bool display)
{
  char var[768];
  const char *strptr;  // why bother with strcpy() on a literal
  size_t len = 0;


 // get variable name

  while (*args && (*args == ' '))
    args++;

  char *varptr = var;

  while (*args && (*args != ' ') && (len < 767))
  {
    *varptr = *args;

    varptr++;
    args++;
    len++;
  }

  *varptr = '\0';

 // move args ptr to start of args

  while (*args && (*args == ' '))
    args++;

  const char result = addVar(varHead, var, args);

  switch (result)
  {
    case VAR_ADDED    :
    case VAR_REPLACED : if (display)
                        {
                          if (addLFs) 
                            _outtext("\n");

                          size_t numbLines = 0;

                          displayVarSettings(getVar(*varHead, var), "", numbLines);

                          if (addLFs) 
                            _outtext("\n");
                        }

                        createPrompt();  // might have updated prompt var
                        if (updateChanges) 
                          g_madeChanges = true;

                        return;

    case VAR_BLANK    : strptr = "The format of the set command is 'set <var> <value>'.\n"; break;

    case VAR_CANNOT_BE_ZERO : strptr = "The value of that variable cannot be set to zero.\n"; break;

    case VAR_BELOW_MINIMUM : strptr = "That variable cannot be set to a value that low.\n"; break;

    case VAR_SAMEVAL  : strptr = "That variable is already set to that value.\n"; break;

    case VAR_DISPLAY  :
    case VAR_ERROR    : 
    default :           return;
  }

  if (display)
  {
    if (addLFs) 
      _outtext("\n");

    displayColorString(strptr);

    if (addLFs) 
      _outtext("\n");
  }
}


//
// varCmd : interprets user args, displaying list if none and calling
//          setVarArgs otherwise
//
//    args : user input
//  addLFs : if TRUE, linefeeds are added to output
// varHead : pointer to pointer of head of linked list of variables
//

void varCmd(const char *args, const bool addLFs, variable **varHead)
{
  variable *node = *varHead;
  size_t numbLines = 1;


  if (!strlen(args))
  {
    if (!node) 
      _outtext("\nThere are currently no variables defined.\n");
    else 
      _outtext("\n\n");

    while (node)
    {
      if (!displayVarSettings(node, "  ", numbLines))
        return;

      node = node->Next;
    }

    _outtext("\n");

    return;
  }
  else 
  {
    setVarArgs(varHead, args, addLFs, true, true);
  }
}


//
// getVar : returns const pointer to variable that matches varName, if any
//

const variable *getVar(const variable *varHead, const char *varName)
{
  while (varHead)
  {
    if (strcmpnocase(varName, varHead->varName))
      return varHead;

    varHead = varHead->Next;
  }

  return NULL;
}


//
// getVarNumb : converts the value in a variable to a long int, if possible
//
//    varHead : head of variable list to scan through
//    varName : name of variable to get value of
//     defVal : default value returned if var not found
//

int getVarNumb(const variable *varHead, const char *varName, const int defVal)
{
  const variable *var = getVar(varHead, varName);

  if (var && strnumber(var->varValue))
    return atoi(var->varValue);
  else
    return defVal;
}


//
// getVarNumbUnsigned : converts the value in a variable to an unsigned int, if possible
//
//    varHead : head of variable list to scan through
//    varName : name of variable to get value of
//     defVal : default value returned if var not found
//

uint getVarNumbUnsigned(const variable *varHead, const char *varName, const uint defVal)
{
  const variable *var = getVar(varHead, varName);

  if (var && strnumber(var->varValue)) 
    return strtoul(var->varValue, NULL, 10);
  else 
    return defVal;
}


//
// getVarStrn : returns string value of a specific variable
//
//    varHead : head of list to scan through
//    varName : name of variable to return value of
//       strn : if non-NULL, value of variable is copied into this
//     defVal : returned/copied if variable not found
//     maxLen : maximum length to copy into strn, not including null char - strn should be at least + 1 
//              this size
//

const char *getVarStrn(const variable *varHead, const char *varName, char *strn, const char *defVal,
                       const size_t intMaxLen)
{
  const variable *var = getVar(varHead, varName);

  if (var)
  {
    if (strn) 
    {
      strncpy(strn, var->varValue, intMaxLen);
      strn[intMaxLen] = '\0';
    }

    return var->varValue;
  }

 // no match found

  if (strn) 
  {
    strncpy(strn, defVal, intMaxLen);
    strn[intMaxLen] = '\0';
  }

  return defVal;
}


//
// getVarStrn : simpler version of above, no need for strn or maxLen
//

const char *getVarStrn(const variable *varHead, const char *varName, const char *defVal)
{
  return getVarStrn(varHead, varName, NULL, defVal, 0);
}


//
// getVarStrnLen : returns length of variable value, 0 if no variable exists
//

size_t getVarStrnLen(const variable *varHead, const char *varName)
{
  const variable *var = getVar(varHead, varName);

  if (var)
    return strlen(var->varValue);
  else
    return 0;
}


//
// setVarBoolVal : sets a variable to a specified boolean val
//
//        varHead : pointer to pointer to head of linked list of vars
//        varName : name of variable to set boolean value of
//        boolVal : value to set
//  updateChanges : if TRUE, updates g_madeChanges appropriately
//

void setVarBoolVal(variable **varHead, const char *varName, const bool boolVal, const bool updateChanges)
{
  char addResult;

  if (boolVal) 
    addResult = addVar(varHead, varName, "true");
  else 
    addResult = addVar(varHead, varName, "false");

  if (updateChanges && (addResult != VAR_BLANK) && (addResult != VAR_SAMEVAL) && (addResult != VAR_ERROR))
    g_madeChanges = true;
}


//
// getVarBoolVal : returns the boolean equivalent of a variable, if possible
//
//     varHead : pointer to head of linked list of variables
//     varName : name of variable to get value of
//      defVal : default value to return if var not found
//

bool getVarBoolVal(const variable *varHead, const char *varName, const bool defVal)
{
  const variable *var = getVar(varHead, varName);

  if (var)
  {
    if ((strcmpnocase(var->varValue, "TRUE")) || (strcmpnocase(var->varValue, "1")) || 
        (strcmpnocase(var->varValue, "ON")))
    {
      return true;
    }

    if ((strcmpnocase(var->varValue, "FALSE")) || (strcmpnocase(var->varValue, "0")) || 
        (strcmpnocase(var->varValue, "OFF")))
    {
      return false;
    }
  }

  return defVal;
}


//
// varExists : checks to see if specified variable exists
//
//    varHead : head of variable list
//    varName : name of variable to check for
//

bool varExists(const variable *varHead, const char *varName)
{
  const variable *var = getVar(varHead, varName);

  return (var != NULL);
}


//
// strIsBoolVal
//

bool strIsBoolVal(const char *strn)
{
  return (strcmpnocase(strn, "TRUE") || strcmpnocase(strn, "FALSE") ||
          strcmpnocase(strn, "ON")   || strcmpnocase(strn, "OFF") ||
          !strcmp(strn, "1")         || !strcmp(strn, "0"));
}


//
// isVarBoolean : checks a variable to see if it contains a boolean value
//
//   varHead : head of list of vars to check
//   varName : name of variable to check for booleanism
//

bool isVarBoolean(const variable *varHead, const char *varName)
{
  if (!varExists(varHead, varName)) 
    return false;

  return strIsBoolVal(getVarStrn(varHead, varName, ""));
}


//
// isVarBoolean : caller has already found the variable rec
//

bool isVarBoolean(const variable *var)
{
  return strIsBoolVal(var->varValue);
}


//
// toggleVar : toggles a boolean variable based on args
//
//    varHead : pointer to pointer to head of linked list of vars
//       args : arguments as entered by user
//

void toggleVar(variable **varHead, const char *origargs)
{
  usint ch;
  size_t numbLines = 1;
  char args[1024];
  bool foundBool = false;
  variable *node = *varHead;


  strncpy(args, origargs, 1023);
  args[1023] = '\0';

  if (!strlen(args))
  {
    if (!node)
    {
      _outtext("\nThere are currently no variables defined.\n\n");

      return;
    }

    _outtext("\n\n");

    while (node)
    {
      if (isVarBoolean(node))
      {
        if (!displayVarSettings(node, "  ", numbLines))
          return;

        foundBool = true;
      }

      node = node->Next;
    }

    if (!foundBool)
      _outtext("There are currently no boolean variables defined.\n");

    _outtext("\n");

    return;
  }
  else
  {
    char* argsptr = args;

    while (*argsptr)  // get var name, cut out anything after the space
    {
      if (*argsptr == ' ')
      {
        *argsptr = '\0';
        break;
      }

      argsptr++;
    }

    if (!varExists(*varHead, args))
    {
      _outtext("\nThat variable does not exist.\n\n"
               "Set to true (T), false (F), or neither (Q) (t/f/Q)? ");

      do
      {
        ch = toupper(getkey());
      } while ((ch != 'T') && (ch != 'F') && (ch != 'Q') && (ch != K_Enter));

      switch (ch)
      {
        case 'T' : _outtext("true\n\n");
                   setVarBoolVal(varHead, args, true, true); return;
        case 'F' : _outtext("false\n\n");
                   setVarBoolVal(varHead, args, false, true); return;
        default : _outtext("neither\n\n"); return;
      }
    }

    if (!isVarBoolean(*varHead, args))
    {
      _outtext("\nThat variable is not a boolean.\n\n");
      return;
    }

    setVarBoolVal(varHead, args, !getVarBoolVal(*varHead, args, false), true);

    const bool result = getVarBoolVal(*varHead, args, false);

    displayColorString("\nVariable '&+c");
    _outtext(args);
    displayColorString("&n' toggled to '&+Y");

    if (result)
    {
      displayColorString("true");
    }
    else
    {
      displayColorString("false");
    }

    displayColorString("&n'.\n\n");
  }
}


//
// copyVarList : copies the list of variables pointed to by varHead,
//               returning the address of the head of the new list
//
//   varHead : head of list to copy
//

variable *copyVarList(const variable *varHead)
{
  variable *newVar, *prevVar = NULL, *headVar = NULL;


  if (!varHead) 
    return NULL;

  while (varHead)
  {
    newVar = new(std::nothrow) variable;
    if (!newVar)
    {
      displayAllocError("variable", "copyVarList");

      return headVar;
    }

    if (!headVar) 
      headVar = newVar;

    if (prevVar) 
      prevVar->Next = newVar;

    prevVar = newVar;

    memcpy(newVar, varHead, sizeof(variable));
    newVar->Next = NULL;

    varHead = varHead->Next;
  }

  return headVar;
}


//
// deleteVarList : Deletes an entire variable list
//
//    *varHead : head of list
//

void deleteVarList(variable *varHead)
{
  variable *nextVar;

  while (varHead)
  {
    nextVar = varHead->Next;

    delete varHead;

    varHead = nextVar;
  }
}


//
// compareVarLists : compare two variable lists - FALSE, no match, TRUE,
//                   match
//
//   varH1 : head of first list
//   varH2 : head of second list
//

bool compareVarLists(const variable *varH1, const variable *varH2)
{
  const variable *v1 = varH1, *v2 = varH2;

  // first, make sure both lists contain the same variables

  while (v1)
  {
    if (!varExists(varH2, v1->varName)) 
      return false;

    v1 = v1->Next;
  }

  while (v2)
  {
    if (!varExists(varH1, v2->varName)) 
      return false;

    v2 = v2->Next;
  }

 // now, make sure that the values in both lists are the same - different case doesn't match

  v1 = varH1;
  while (v1)
  {
    if (strcmp(getVarStrn(varH2, v1->varName, ""), v1->varValue))
    {
      return false;
    }

    v1 = v1->Next;
  }

  return true;
}
