//
//  File: editent.cpp    originally part of durisEdit
//
//  Usage: handling for entire 'edit' syntax
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

#include "../types.h"
#include "../fh.h"
#include "../room/room.h"
#include "../misc/master.h"

extern command g_editCommands[];
extern editableListNode *g_inventory;
extern room *g_currentRoom;


//
// editEntity : Edits something - first, runs through the arg string looking
//              for a keyword match with something in the current room, then 
//              looks for matches in inventory
//

void editEntity(const char *args)
{
  editableListNode *editNode;
  char whatMatched, grabbing;


 // check the editable entity list for a match

  grabbing = checkEditableList(args, g_currentRoom->editableListHead, &whatMatched, &editNode, 1);

 // check inventory if room search failed

  if (whatMatched == NO_MATCH)
  {
    checkEditableList(args, g_inventory, &whatMatched, &editNode, grabbing);
  }

  switch (whatMatched)
  {
    case NO_MATCH       : 
      break;

    case ENTITY_OBJECT  :
      editObjType(((objectHere *)(editNode->entityPtr))->objectPtr, true);
      return;

    case ENTITY_O_EDESC  :
      if (editExtraDesc((extraDesc *)(editNode->entityPtr)))
      {
       // update all appropriate masterKeyword and editable lists

        updateAllObjMandElists();
      }

      return;

    case ENTITY_R_EDESC :
      if (editExtraDesc((extraDesc *)(editNode->entityPtr)))
      {
        deleteMasterKeywordList(g_currentRoom->masterListHead);
        g_currentRoom->masterListHead = createMasterKeywordList(g_currentRoom);

        deleteEditableList(g_currentRoom->editableListHead);
        g_currentRoom->editableListHead = createEditableList(g_currentRoom);
      }

      return;

    case ENTITY_MOB     :
      editMobType(((mobHere *)(editNode->entityPtr))->mobPtr, true);
      return;
  }

  checkCommands(args, g_editCommands, "\n"
"Specify a keyword matching something in the room or try one of <room|exit|\n"
"object|mob|zone|quest|shop|desc|defroom|defobject|defmob|defexit|defedesc|\n"
"defquest|defshop>.\n\n",
                editExecCommand, NULL, NULL);
}
