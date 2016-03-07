//
//  File: win32.cpp      originally part of durisEdit
//
//  Usage: Win32 console app API funcs - wrappers of original
//         Watcom text functions and misc Win32-specific funcs
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

#ifndef _WIN32
#  error WIN32.CPP should only be used for Windows
#endif

#include <mapiutil.h>
#include <mapiwin.h>
#include <wincon.h>
#include <stdio.h>
#include <direct.h>

#include "types.h"
#include "system.h"
#include "graphcon.h"
#include "keys.h"
#include "fh.h"

static uchar fg = 7, bg = 0;
static HANDLE outpHand, inpHand;

// Win32's 'get input' func may specify that a particular key has been hit X times

static WORD inpBufKeyHit = 0;
static CHAR inpBufKeyHitAscii;
static WORD inpBufHitTimes;
static bool inpShiftDown = false;


//
// checkWindowsVer : returns true if not the right version of Windows
//

bool checkWindowsVer(void)
{
  OSVERSIONINFO osVer;

  osVer.dwOSVersionInfoSize = sizeof(osVer);

  if (!GetVersionEx(&osVer)) 
    return true;

  if (osVer.dwPlatformId == VER_PLATFORM_WIN32s)
  {
    MessageBox(NULL, "durisEdit cannot run under WIN32s.", "Error", MB_OK);

    return true;
  }

  return false;
}


//
// pauseOnExit
//

void __stdcall pauseOnExit(void)
{
  _outtext("\nPress a key to exit...");
  getkey();
}


//
// setupWindowsConsole : returns true if there's an error
//

bool setupWindowsConsole(void)
{
  DWORD dwMode;


  inpHand = GetStdHandle(STD_INPUT_HANDLE);
  outpHand = GetStdHandle(STD_OUTPUT_HANDLE);

  if (!GetConsoleMode(inpHand, &dwMode)) 
    return true;

  if (!SetConsoleMode(inpHand, dwMode & ~ENABLE_LINE_INPUT & ~ENABLE_ECHO_INPUT))
    return true;

  SetConsoleTitle(DE_WIN32_CONSOLEAPP_NAME);

  return false;
}


//
// setWin32ExitFunc : returns false on error
//

bool setWin32ExitFunc(void)
{
  if (atexit((void (__cdecl *)(void))pauseOnExit))
    return false;
  else
    return true;
}


//
// getForegroundValue
//

WORD getForegroundValue(const uchar fg)
{
  WORD consoleCol = 0;

  switch (fg & 7)
  {
    case 0  : consoleCol = 0;  break;  // black
    case 1  : consoleCol = FOREGROUND_BLUE;  break;  // blue
    case 2  : consoleCol = FOREGROUND_GREEN;  break;  // dark green
    case 3  : consoleCol = FOREGROUND_GREEN | FOREGROUND_BLUE;  break;  // cyan
    case 4  : consoleCol = FOREGROUND_RED;  break;  // red
    case 5  : consoleCol = FOREGROUND_RED | FOREGROUND_BLUE;  break;  // purple
    case 6  : consoleCol = FOREGROUND_RED | FOREGROUND_GREEN; break;  // brown
    case 7  : consoleCol = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;  break;  // white
  }

  if (fg > 7) 
    consoleCol |= FOREGROUND_INTENSITY;

  return consoleCol;
}


//
// getBackgroundValue
//

WORD getBackgroundValue(const uchar bg)
{
  WORD consoleCol = 0;

  switch (bg & 7)
  {
    case 0  : consoleCol = 0;  break;  // black
    case 1  : consoleCol = BACKGROUND_BLUE;  break;  // blue
    case 2  : consoleCol = BACKGROUND_GREEN;  break;  // dark green
    case 3  : consoleCol = BACKGROUND_GREEN | BACKGROUND_BLUE;  break;  // cyan
    case 4  : consoleCol = BACKGROUND_RED;  break;  // red
    case 5  : consoleCol = BACKGROUND_RED | BACKGROUND_BLUE;  break;  // purple
    case 6  : consoleCol = BACKGROUND_RED | BACKGROUND_GREEN; break;  // brown
    case 7  : consoleCol = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;  break;  // white
  }

  if (bg > 7) 
    consoleCol |= BACKGROUND_INTENSITY;

  return consoleCol;
}


//
// _settextposition
//

void _settextposition(const sint y, const sint x)
{
  COORD coords;
  DWORD err;

  if ((y < 1) || (x < 1)) 
    return;

  coords.X = x - 1;
  coords.Y = y - 1;

  if (!SetConsoleCursorPosition(outpHand, coords))
  {
    err = GetLastError();

    char strn[1024];

    sprintf(strn, "_settextposition(): SetConsoleCursorPosition() call failed\n" 
                  "error was %d, pos was x=%d y=%d\n",
            err, x, y);

    _outtext(strn);
  }
}


//
// _gettextposition
//

struct rccoord _gettextposition(void)
{
  struct rccoord coords;
  CONSOLE_SCREEN_BUFFER_INFO csbInfo;

  if (!GetConsoleScreenBufferInfo(outpHand, &csbInfo))
    _outtext("_gettextposition(): GetConsoleScreenBufferInfo() call failed\n");

  coords.row = csbInfo.dwCursorPosition.Y + 1;
  coords.col = csbInfo.dwCursorPosition.X + 1;

 // sometimes when the cursor is at the bottom of the screen GetConsoleSBI will return a value for y that
 // is way too high (bug, I assume), so fix it

  if (coords.row > csbInfo.dwSize.Y)
    coords.row = csbInfo.dwSize.Y;

  return coords;
}


//
// _outtext
//

void _outtext(const char *strn)
{
  DWORD writ;
  size_t len;


  if (!strn[0]) 
    return;

 // write to the console in chunks of no more than 65535 bytes each, as limited by the API

  len = strlen(strn);

  while (len >= 65536)
  {
    if (!WriteConsole(outpHand, strn, 65535, &writ, NULL))
      puts("_outtext(): WriteConsole() call failed");

    len -= 65535;
    strn += 65535;
  }

 // final write

  if (len)
  {
    if (!WriteConsole(outpHand, strn, (DWORD)len, &writ, NULL))
      puts("_outtext(): WriteConsole() call failed");
  }
}


//
// clrline
//

void clrline(const sint line, const uchar fg, const uchar bg, const uchar ch)
{
  COORD coords;
  DWORD writ;

  if (line < 1) 
    return;

  coords.X = 0;
  coords.Y = line - 1;

  if (!FillConsoleOutputCharacter(outpHand, ch, getScreenWidth(), coords, &writ) ||
      !FillConsoleOutputAttribute(outpHand, getForegroundValue(fg) | getBackgroundValue(bg),
      getScreenWidth(), coords, &writ))
    _outtext("clrline(): FillConsole...() call failed\n");
}


//
// clrline
//

void clrline(const sint line, const uchar fg, const uchar bg)
{
  clrline(line, fg, bg, ' ');
}


//
// clrline
//

void clrline(const sint line)
{
  clrline(line, 7, 0, ' ');
}


//
// getkey
//

usint getkey(void)
{
  usint retVal;
  INPUT_RECORD inpRec;
  DWORD dwRead;
  WORD keycode;
  CHAR keycodeAscii;


 // read a character from the console input

  while (true)
  {
   // if key has been hit > 1 time, return it

    if (inpBufKeyHit != 0)
    {
      keycode = inpBufKeyHit;
      keycodeAscii = inpBufKeyHitAscii;

      if (inpBufHitTimes == 1)
        inpBufKeyHit = 0;
      else
        inpBufHitTimes--;

      break;
    }

   // nothing buffered, get input from keyboard

    else
    {
      if (!ReadConsoleInput(inpHand, &inpRec, 1, &dwRead))
      {
        _outtext("getkey(): error in ReadConsoleInput()\n");
        return ' ';
      }

     // ignore Control, Alt - user could also push tons of other useless keys that would generate an
     // event, like Caps Lock, Pause, and so forth, but I DON'T CARE

     // process shift and set inpShiftDown, then ignore it

      if ((inpRec.EventType == KEY_EVENT) && (inpRec.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT))
      {
        if (inpRec.Event.KeyEvent.bKeyDown == TRUE)
          inpShiftDown = true;
        else
          inpShiftDown = false;

        continue;
      }

      if ((inpRec.EventType != KEY_EVENT) || (inpRec.Event.KeyEvent.bKeyDown == FALSE) ||
          (inpRec.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL) ||      
          (inpRec.Event.KeyEvent.wVirtualKeyCode == VK_MENU))
        continue;

      keycode = inpRec.Event.KeyEvent.wVirtualKeyCode;
      keycodeAscii = inpRec.Event.KeyEvent.uChar.AsciiChar;

     // if repeat count is greater than one, make key repeat

      if (inpRec.Event.KeyEvent.wRepeatCount > 1)
      {
        inpBufKeyHit = keycode;
        inpBufKeyHitAscii = keycodeAscii;

        inpBufHitTimes = inpRec.Event.KeyEvent.wRepeatCount - 1;
      }

      break;
    }
  }

  switch (keycode)
  {
    case VK_END    : retVal = K_End;  break;
    case VK_HOME   : retVal = K_Home;  break;
    case VK_LEFT   : retVal = K_LeftArrow;  break;
    case VK_RIGHT  : retVal = K_RightArrow;  break;
    case VK_UP     : retVal = K_UpArrow;  break;
    case VK_DOWN   : retVal = K_DownArrow;  break;
    case VK_DELETE : retVal = K_Delete;  break;

    case VK_F1 : if (inpShiftDown)
                   retVal = K_Shift_F1;
                 else
                   retVal = K_F1;
                 break;
    case VK_F2 : if (inpShiftDown)
                   retVal = K_Shift_F2;
                 else
                   retVal = K_F2;
                 break;
    case VK_F3 : if (inpShiftDown)
                   retVal = K_Shift_F3;
                 else
                   retVal = K_F3;
                 break;
    case VK_F4 : if (inpShiftDown)
                   retVal = K_Shift_F4;
                 else
                   retVal = K_F4;  
                 break;
    case VK_F5 : if (inpShiftDown)
                   retVal = K_Shift_F5;
                 else
                   retVal = K_F5;  
                 break;
    case VK_F6 : if (inpShiftDown)
                   retVal = K_Shift_F6;
                 else
                   retVal = K_F6;  
                 break;
    case VK_F7 : if (inpShiftDown)
                   retVal = K_Shift_F7;
                 else
                   retVal = K_F7;  
                 break;
    case VK_F8 : if (inpShiftDown)
                   retVal = K_Shift_F8;
                 else
                   retVal = K_F8;  
                 break;
    case VK_F9 : if (inpShiftDown)
                   retVal = K_Shift_F9;
                 else
                   retVal = K_F9;  
                 break;
    case VK_F10 : if (inpShiftDown)
                   retVal = K_Shift_F10;
                 else
                   retVal = K_F10;  
                 break;
    case VK_F11 : if (inpShiftDown)
                   retVal = K_Shift_F11;
                 else
                   retVal = K_F11;  
                 break;
    case VK_F12 : if (inpShiftDown)
                   retVal = K_Shift_F12;
                 else
                   retVal = K_F12;
                 break;

    default : retVal = keycodeAscii;
  }

  return retVal;
}


//
// _settextcolor
//

void _settextcolor(const uchar col)
{
  WORD consoleCol;

  if ((fg == col) || (col > 15)) 
    return;

  fg = col;

  consoleCol = getForegroundValue(fg) | getBackgroundValue(bg);

  if (!SetConsoleTextAttribute(outpHand, consoleCol))
    _outtext("_settextcolor(): SetConsoleTextAttribute() call failed\n");
}


//
// _setbkcolor
//

void _setbkcolor(const uchar col)
{
  WORD consoleCol;

  if ((bg == col) || (col > 15)) 
    return;

  bg = col;

  consoleCol = getForegroundValue(fg) | getBackgroundValue(bg);

  if (!SetConsoleTextAttribute(outpHand, consoleCol))
    _outtext("_setbkcolor(): SetConsoleTextAttribute() call failed\n");
}


//
// _gettextcolor
//

uchar _gettextcolor(void)
{
  return fg;
}


//
// _getbkcolor
//

uchar _getbkcolor(void)
{
  return bg;
}


//
// clrscr
//

void clrscr(const uchar fg, const uchar bg)
{
  COORD coords;
  DWORD writ, numbChars;
  WORD color;
  CONSOLE_SCREEN_BUFFER_INFO csbi;


  color = getForegroundValue(fg) | getBackgroundValue(bg);

  coords.X = 0;
  coords.Y = 0;

  if (!GetConsoleScreenBufferInfo(outpHand, &csbi))
  {
    _outtext("clrscr(): GetConsoleScreenBufferInfo() call failed\n");
    return;
  }

 // clear everything from the screen buffer with appropriately-colored spaces

  numbChars = csbi.dwSize.X * csbi.dwSize.Y;

  if (!FillConsoleOutputCharacter(outpHand, ' ', numbChars, coords, &writ) ||
      !FillConsoleOutputAttribute(outpHand, color, numbChars, coords, &writ))
    _outtext("clrscr(): FillConsole...() call failed\n");

  _settextposition(1, 1);
}



//
// handleNoArgs : handles user input of base zone filename string when no args are specified for Win32
//

bool handleNoArgs(char *filenameStrn, bool *newZone)
{
  char dirStrn[512];
  bool openZone;  // if false, selected 'create new zone'
  BOOL success;  // from common dialog
  usint ch;


  displayColorString(
    "&+cNo zone name was specified - would you like to create a &+Cn&+cew zone, &+CO&+cpen an\n"
    "existing zone, or neither (&+Cq&+cuit) (n/O/q)? &n");

  do
  {
    ch = toupper(getkey());
  } while ((ch != 'N') && (ch != 'O') && (ch != 'Q') && (ch != K_Enter));

  getcwd(dirStrn, 512);

  if (ch == 'N')
  {
    _outtext("new\n\n");

    openZone = false;

    *newZone = true;
  }
  else if (ch == 'Q')
  {
    _outtext("quit\n\n");
    exit(1);
  }
  else
  {
    _outtext("open\n\n");

    openZone = true;

    *newZone = false;
  }

  filenameStrn[0] = '\0';

  OPENFILENAME ofn;

  memset(&ofn, 0, sizeof(ofn));

  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFilter = "Zone files (*.zon)\0*.zon\0All files (*.*)\0*.*\0\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = filenameStrn;
  ofn.nMaxFile = MAX_VARVAL_LEN - 1;
  ofn.lpstrInitialDir = dirStrn;
  ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

  if (openZone)
  {
    ofn.lpstrTitle = "Open Zone";

    ofn.Flags |= OFN_FILEMUSTEXIST;

    success = GetOpenFileName(&ofn);
  }
  else
  {
    ofn.lpstrTitle = "New Zone";

    ofn.Flags |= OFN_OVERWRITEPROMPT;

    success = GetSaveFileName(&ofn);
  }

  if ((CommDlgExtendedError() == FNERR_BUFFERTOOSMALL) || (strlen(filenameStrn) >= MAX_VARVAL_LEN))
  {
    char errorStrn[128];

    sprintf(errorStrn, "That filename is too long (more than %u characters).",
            MAX_VARVAL_LEN - 1);

    MessageBox(NULL, errorStrn, "Error", MB_OK);

    success = FALSE;
  }

  if (!success)
  {
    return false;
  }
  else
  {
   // remove file extension

   // 0 means there is no period

    if (ofn.nFileExtension)
      filenameStrn[ofn.nFileExtension - 1] = '\0';

    return true;
  }
}
