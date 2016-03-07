//
//  File: crtdesc.cpp    originally part of durisEdit
//
//  Usage: function(s) for creating extra descs
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

#include "types.h"
#include "fh.h"

#include "edesc.h"

extern extraDesc *g_defaultExtraDesc;



//
// createExtraDesc : Function to add a new extra desc to the end of a list
//
//   extraDescHead : pointer to pointer to head of extra desc list - can be NULL
//     keywordStrn : if non-NULL, new extra desc gets these keywords, otherwise DEFAULT
//     copyDefault : if true, use any default edesc for the initial keywords/desc
//

extraDesc *createExtraDesc(extraDesc **extraDescHead, const char *keywordStrn, const bool copyDefault)
{
  extraDesc *newExtraDesc, *extraDescNode;


 // create a new extra desc

  newExtraDesc = new(std::nothrow) extraDesc;
  if (!newExtraDesc)
  {
    displayAllocError("extraDesc", "createExtraDesc");

    return NULL;
  }

  memset(newExtraDesc, 0, sizeof(extraDesc));

  if (!copyDefault || !g_defaultExtraDesc)
  {
    if (!keywordStrn)
      newExtraDesc->keywordListHead = createKeywordList("default~");
    else
      newExtraDesc->keywordListHead = createKeywordList(keywordStrn);
  }
  else
  {
    newExtraDesc->keywordListHead = copyStringNodes(g_defaultExtraDesc->keywordListHead);
    newExtraDesc->extraDescStrnHead = copyStringNodes(g_defaultExtraDesc->extraDescStrnHead);
  }

 // get to the end of the list

  if (extraDescHead)
  {
    if (!(*extraDescHead))  // no list exists
    {
      *extraDescHead = newExtraDesc;
    }
    else
    {
      extraDescNode = *extraDescHead;

      while (extraDescNode->Next)
        extraDescNode = extraDescNode->Next;

      newExtraDesc->Last = extraDescNode;
      extraDescNode->Next = newExtraDesc;
    }
  }

  return newExtraDesc;
}
