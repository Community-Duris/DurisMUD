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


// STRNNODE.H - header file for STRNNODE.C

#ifndef _STRNNODE_H_

#include <stdlib.h>
#include <string.h>
#include <new>    // nothrow

#include "types.h"

// string node stuff

// no real limit, but strings cannot be infinite

#define MAX_STRNNODE_LEN   (usint)16384

typedef struct _stringNode
{
  char *string;
  size_t len;

  struct _stringNode *Last;
  struct _stringNode *Next;

  _stringNode() : string(NULL), len(0), Next(NULL), Last(NULL)
  {
  }

  ~_stringNode()
  {
    if (string) 
      delete[] string;
  }

  void setString(const char *strn)
  {
    if (string)
      delete[] string;

    if (!strn)
    {
      string = NULL;
      len = 0;

      return;
    }

    len = strlen(strn);

    string = new(std::nothrow) char[len + 1];

   // calling function can handle failed alloc

    if (string)
      strcpy(string, strn);
  }
} stringNode;


// constants for readStringNodes

#define TILDE_LINE     0      // End of string nodes signaled by line with
                              // nothing but a tilde on it
#define ENDOFFILE      1      // Pretty self-explanatory methinks


#define _STRNNODE_H_
#endif
