//
//  File: readexd.cpp    originally part of durisEdit
//
//  Usage: function(s) for reading extra desc info from a file
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



#include "../fh.h"
#include "../readfile.h"


//
// readExtraDesc : Reads an extra description - adds it to the end of the list pointed to by
//                 extraDescHeadPtr if non-NULL, and returns the address of the new extra 
//                 description node
//
//    *edescFile        : edesc file
//   **extraDescHeadPtr : pointer to head of edesc list
//

extraDesc *readExtraDescFromFile(FILE *edescFile, const uint parentEntityType, const uint entityNumb, 
                                 const uint extraDescType, extraDesc **extraDescHeadPtr)
{
  extraDesc *extraDescNode;
  char strn[MAX_EDESCKEY_LEN + 5];  // allow for extra


 // create new extra description node

  extraDescNode = new(std::nothrow) extraDesc;
  if (!extraDescNode)
  {
    displayAllocError("extraDesc", "readExtraDesc");

    exit(1);
  }

 // set stuff

  memset(extraDescNode, 0, sizeof(extraDesc));

 // set linked list pointers, if head ptr is non-NULL

  if (extraDescHeadPtr)
  {
    if (!*extraDescHeadPtr)
    {
      *extraDescHeadPtr = extraDescNode;
    }
    else
    {
      extraDesc *lastNode = *extraDescHeadPtr;

      while (lastNode->Next)
        lastNode = lastNode->Next;

      lastNode->Next = extraDescNode;
      extraDescNode->Last = lastNode;
    }
  }

 // read the extra description keyword list - all on one line

  if (!readAreaFileLine(edescFile, strn, MAX_EDESCKEY_LEN + 2, extraDescType, ENTITY_NUMB_UNUSED, 
                        parentEntityType, entityNumb, "keyword", 0, NULL, true, true))
    exit(1);

 // generate a keyword list with this string

  extraDescNode->keywordListHead = createKeywordList(strn);

 // now, read the actual description

  extraDescNode->extraDescStrnHead = readStringNodes(edescFile, TILDE_LINE, false);


  return extraDescNode;
}
