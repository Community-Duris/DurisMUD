//
//  File: menu.cpp       originally part of durisEdit
//
//  Usage: functions for handling menu display and input
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


#include <ctype.h>

#include "../fh.h"
#include "../misc/menu.h"
#include "../keys.h"
#include "../types.h"
#include "../obj/object.h"

extern flagDef g_roomManaList[], g_roomSectList[], g_objSizeList[], g_shopShopkeeperRaceList[],
               g_roomExitDirectionList[], g_npc_class_bits[], g_race_names[], g_objCraftsmanshipList[],
               g_objTypeList[], g_objTrapFlagDef[], g_objTrapDamList[], g_mobPositionList[], g_mobSexList[],
               g_mobHometownList[], g_mobSizeList[], g_zoneMiscFlagDef[], g_zoneResetModeList[],
               g_questGiveTypeList[], g_questReceiveTypeList[];

extern "C" flagDef room_bits[], extra_bits[], extra2_bits[], wear_bits[], affected1_bits[], 
                   affected2_bits[], affected3_bits[], affected4_bits[],
                   action_bits[], aggro_bits[], aggro2_bits[], aggro3_bits[];

extern uint g_roomFlagTemplates[], g_objExtraFlagTemplates[], g_objExtra2FlagTemplates[],
            g_objWearFlagTemplates[], g_objAntiFlagTemplates[], g_objAnti2FlagTemplates[],
            g_objAff1FlagTemplates[], g_objAff2FlagTemplates[], g_objAff3FlagTemplates[], 
            g_objAff4FlagTemplates[], g_mobActionFlagTemplates[], g_mobAff1FlagTemplates[], 
            g_mobAff2FlagTemplates[], g_mobAff3FlagTemplates[], g_mobAff4FlagTemplates[],
            g_mobAggroFlagTemplates[], g_mobAggro2FlagTemplates[], g_mobAggro3FlagTemplates[];

extern variable *g_varHead;

//
// checkMenuKey : keys common to 99% of all menus, return various keyed
//                values (MENUKEY_*, defined in menu.h)
//
//       ch : character entered by user
// flagMenu : if TRUE, check for Ctrl-Q and Ctrl-X instead of Q and X and
//            don't check for NEXT_CH and PREV_CH
//

char checkMenuKey(const usint ch, const bool flagMenu)
{
  if (flagMenu)
  {
    switch (ch)
    {
      case MENUKEY_SAVE_CH        :
      case MENUKEY_FLAG_SAVE2_CH  : return MENUKEY_SAVE;

      case MENUKEY_ABORT_CH       :
      case MENUKEY_FLAG_ABORT2_CH : return MENUKEY_ABORT;

      default : return MENUKEY_OTHER;
    }
  }
  else
  {
    switch (ch)
    {
      case MENUKEY_SAVE_CH   :
      case MENUKEY_SAVE2_CH  : return MENUKEY_SAVE;

      case MENUKEY_ABORT_CH  :
      case MENUKEY_ABORT2_CH : return MENUKEY_ABORT;

      case MENUKEY_NEXT2_CH  :
      case MENUKEY_NEXT_CH   : return MENUKEY_NEXT;

      case MENUKEY_PREV2_CH  :
      case MENUKEY_PREV_CH   : return MENUKEY_PREV;

      case MENUKEY_JUMP_CH   : return MENUKEY_JUMP;

      default : return MENUKEY_OTHER;
    }
  }
}


//
// getMenuDataTypeStrn : given dataType, entityPtr, and offset, write human readable version of data into valstrn
//
// valstrn must be at least 129 characters
//   intMaxLen is used where strings could conceivably be larger (mob class, keyword lists, ...)
long getMenuDataTypeStrn(char *valstrn, const menuChoiceDataType dataType, const size_t offset,
  const void *entityPtr, const size_t intMaxLen)
{
  char tempstrn[1024];
  extraDesc *edesc;  // used by obj ID edescs
  long lval = 0;

  switch (dataType)
  {
    case mctChar :
      lval = *((char *)entityPtr + offset);

      sprintf(valstrn, "%c", (char)lval);
      break;

    case mctByte :
      lval = *((char *)entityPtr + offset);

      sprintf(valstrn, "%ld", lval);
      break;

    case mctUByte :
      lval = *((char *)entityPtr + offset);

      sprintf(valstrn, "%lu", lval);
      break;

    case mctShort :
      lval = *(sint *)((char *)entityPtr + offset);

      sprintf(valstrn, "%hd", (sint)lval);
      break;

    case mctUShort :
      lval = *(usint *)((char *)entityPtr + offset);

      sprintf(valstrn, "%hu", (usint)lval);
      break;

    case mctInt :
      lval = *(int *)((char *)entityPtr + offset);

      sprintf(valstrn, "%ld", lval);
      break;

    case mctUInt :
    case mctVnum :
    case mctRoomFlag :
    case mctObjExtra :
    case mctObjExtra2 :
    case mctObjWear :
    case mctObjAnti :
    case mctObjAnti2 :
    case mctObjAffect1 :
    case mctObjAffect2 :
    case mctObjAffect3 :
    case mctObjAffect4 :
    case mctObjTrapFlag :
    case mctMobAction :
    case mctMobAggro :
    case mctMobAggro2 :
    case mctMobAggro3 :
    case mctMobAffect1 :
    case mctMobAffect2 :
    case mctMobAffect3 :
    case mctMobAffect4 :
    case mctShopFirstOpen :
    case mctShopFirstClose :
    case mctShopSecondOpen :
    case mctShopSecondClose :
    case mctZoneFlag :
    case mctLowLifespan :
    case mctHighLifespan :
      lval = *(uint *)((char *)entityPtr + offset);

      sprintf(valstrn, "%lu", lval);
      break;

    case mctFloat :
      sprintf(valstrn, "%f", *(float *)((char *)entityPtr + offset));
      break;

    case mctDouble :
      sprintf(valstrn, "%f", *(double *)((char *)entityPtr + offset));
      break;

    case mctBuyMultiplier :
    case mctSellMultiplier :
      sprintf(valstrn, "%.2f", *(double *)((char *)entityPtr + offset));
      break;

    case mctString :
    case mctDieString :
      strncpy(valstrn, (char *)entityPtr + offset, intMaxLen);
      valstrn[intMaxLen] = '\0';

      break;

    case mctPointerYesNo :
    case mctDescription :
    case mctDescriptionNoStartBlankLines :
      sprintf(valstrn, "exists: %s", getYesNoStrn(*(void **)((char *)entityPtr + offset)));
      break;

    case mctQuestDisappearanceDescription :
      sprintf(valstrn, "exists: %s [%s]", 
              getYesNoStrn(*(void **)((char *)entityPtr + offset)),
              *(void **)((char *)entityPtr + offset) ? "disappears" : "doesn't disappear");
      break;

    case mctRoomEdescs :
    case mctObjEdescs :
      lval = getNumbExtraDescs((*(extraDesc **)((char *)entityPtr + offset)));

      sprintf(valstrn, "%lu", lval);
      break;

    case mctExits :
      getRoomExitsShortStrn((room *)entityPtr, valstrn);
      break;

    case mctKeywords :
      getReadableKeywordStrn((*(stringNode **)((char *)entityPtr + offset)), valstrn, intMaxLen - 1);
      break;

    case mctMoney :
      sprintf(valstrn, "&n%s&+c", getMoneyStrn(*(uint *)((char *)entityPtr + offset), tempstrn));
      break;

    case mctPercentage :
      sprintf(valstrn, "%u%%", *(uint *)((char *)entityPtr + offset));
      break;

   // for object/mob limit, offset is used to store vnum

    case mctObjectLimit :
      lval = getNumbEntities(ENTITY_OBJECT, (uint)offset, false);

      sprintf(valstrn, "%ld", lval);
      break;

#ifdef GODMODE
    case mctItemValue :
      sprintf(valstrn, "%d", itemvalue( (objectType *)((char *)entityPtr + offset) ));
      break;
#endif

    case mctObjectLimitOverride :
      lval = getNumbEntities(ENTITY_OBJECT, (uint)offset, true);

      sprintf(valstrn, "%ld", lval);
      break;

    case mctMobLimit :
      lval = getNumbEntities(ENTITY_MOB, (uint)offset, false);

      sprintf(valstrn, "%ld", lval);
      break;

    case mctMobLimitOverride :
      lval = getNumbEntities(ENTITY_MOB, (uint)offset, true);

      sprintf(valstrn, "%ld", lval);
      break;

    case mctSpecies :
      strcpy(valstrn, getMobSpeciesStrn((char *)entityPtr + offset));
      break;

    case mctMobClass :
      getClassString((mobType *)entityPtr, valstrn, intMaxLen - 1);
      break;

    case mctPointerYesNoSansExists :
      strcpy(valstrn, getYesNoStrn(*(void **)((char *)entityPtr + offset)));
      break;

    case mctNumbShopSold :
      lval = getNumbShopSold((uint *)((char *)entityPtr + offset));
      sprintf(valstrn, "%ld", lval);
      break;

    case mctNumbShopBought :
      lval = getNumbShopBought((uint *)((char *)entityPtr + offset));
      sprintf(valstrn, "%ld", lval);
      break;

    case mctBoolYesNo :
      strcpy(valstrn, getYesNoStrn(*(bool *)((char *)entityPtr + offset)));
      break;

    case mctVarYesNo :
      strcpy(valstrn, getYesNoStrn(((bool(*)(void))(offset))()));
      break;

    case mctVarUInt :
      lval = ((uint(*)(void))(offset))();
      sprintf(valstrn, "%u", (uint)lval);
      break;

    case mctVarString :
      strncpy(valstrn, ((char *(*)(void))(offset))(), intMaxLen);
      valstrn[intMaxLen] = '\0';
      break;

    case mctObjIDKeywords :
      edesc = getEdescinList((extraDesc *)entityPtr, "_ID_NAME_");

      if (edesc)
      {
        if (edesc->extraDescStrnHead && edesc->extraDescStrnHead->string)
        {
          strncpy(valstrn, edesc->extraDescStrnHead->string, intMaxLen);
          valstrn[intMaxLen] = '\0';
        }
        else 
        {
          strcpy(valstrn, "missing info");
        }
      }
      else
      {
        strcpy(valstrn, "&+cnone exist");
      }

      break;

    case mctObjIDShort :
    case mctObjIDLong :
      if (dataType == mctObjIDShort)
        edesc = getEdescinList((extraDesc *)entityPtr, "_ID_SHORT_");
      else
        edesc = getEdescinList((extraDesc *)entityPtr, "_ID_DESC_");

      if (edesc)
      {
        if (edesc->extraDescStrnHead && edesc->extraDescStrnHead->string)
        {
          strncpy(valstrn, edesc->extraDescStrnHead->string, intMaxLen);
          valstrn[intMaxLen] = '\0';
        }
        else
        {
          strcpy(valstrn, "missing info");
        }
      }
      else
      {
        strcpy(valstrn, "&+cdoes not exist");
      }

      break;
  }
  return lval;
}


// getMenuListTypeStrn : given listType, value, and entityPtr, write verbal equivalent of enumerated value
//   verbosevalstrn should be at least 192
//
void getMenuListTypeStrn(char *verbosevalstrn, const menuChoiceListType listType, const int val,
                         const void *entityPtr, const size_t intMaxLen)
{
  const objectType *obj = (const objectType*)entityPtr;  // used for obj values below
  const mobType *mob = (const mobType*)entityPtr;
  const questItem *qstItem = (const questItem*)entityPtr;
  int intValNumb;

  switch (listType)
  {
    case mclRoomSector :
      getRoomSectorStrn(val, false, false, verbosevalstrn);
      break;

    case mclRoomManaFlag :
      strncpy(verbosevalstrn, getFlagNameFromList(g_roomManaList, val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclCurrentDirection :
      strncpy(verbosevalstrn, getExitStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclObjectType :
      strncpy(verbosevalstrn, getObjTypeStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclApplyWhere :
      strncpy(verbosevalstrn, getObjApplyStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclObjectSize :
      strncpy(verbosevalstrn, getFlagNameFromList(g_objSizeList, val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclMaterial :
      strncpy(verbosevalstrn, getMaterialStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclCraftsmanship :
      strncpy(verbosevalstrn, getObjCraftsmanshipStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclTrapFlags :
      getObjTrapFlagsStrn(val, verbosevalstrn);
      break;

    case mclTrapDamage :
      strncpy(verbosevalstrn, getObjTrapDamStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclHometown :
      strncpy(verbosevalstrn, getMobHometownStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclMobSpec :
      strncpy(verbosevalstrn, getMobSpecStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclMobSize :
      strncpy(verbosevalstrn, getMobSizeStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclPosition :
      strncpy(verbosevalstrn, getMobPosStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclSex :
      strncpy(verbosevalstrn, getMobSexStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclResetMode :
      strncpy(verbosevalstrn, getZoneResetStrn(val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

   // a little magic with object values - 1 is first value, etc

    case mclObjectValue1 :
    case mclObjectValue2 :
    case mclObjectValue3 :
    case mclObjectValue4 :
    case mclObjectValue5 :
    case mclObjectValue6 :
    case mclObjectValue7 :
    case mclObjectValue8 :
      intValNumb = listType - mclObjectValue1;

      strcpy(verbosevalstrn, 
        getObjValueStrn(obj->objType, intValNumb, obj->objValues[intValNumb], verbosevalstrn, true));

      break;

    case mclShopkeeperRace :
      strncpy(verbosevalstrn, getFlagNameFromList(g_shopShopkeeperRaceList, val), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclQuestItemGivenToMobType :
      strncpy(verbosevalstrn, getQuestItemTypeStrn(val, QUEST_GIVEITEM), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclQuestItemGivenToPCType :
      strncpy(verbosevalstrn, getQuestItemTypeStrn(val, QUEST_RECVITEM), intMaxLen);
      verbosevalstrn[intMaxLen] = '\0';
      break;

    case mclQuestItemGivenToMobValue :
      switch (qstItem->itemType)
      {
        case QUEST_GITEM_OBJ :
          strncpy(verbosevalstrn, getObjShortName(findObj(qstItem->itemVal)), intMaxLen);
          verbosevalstrn[intMaxLen] = '\0';
          break;

        case QUEST_GITEM_COINS :
          getMoneyStrn(qstItem->itemVal, verbosevalstrn);
          break;

        case QUEST_GITEM_OBJTYPE :
          strncpy(verbosevalstrn, getObjTypeStrn(val), intMaxLen);
          verbosevalstrn[intMaxLen] = '\0';
          break;

        default :
          strcpy(verbosevalstrn, "Unrecognized");
          break;
      }

      break;

    case mclQuestItemGivenToPCValue :
      switch (qstItem->itemType)
      {
        case QUEST_RITEM_OBJ :
          strncpy(verbosevalstrn, getObjShortName(findObj(qstItem->itemVal)), intMaxLen);
          verbosevalstrn[intMaxLen] = '\0';
          break;

        case QUEST_RITEM_COINS :
          getMoneyStrn(qstItem->itemVal, verbosevalstrn);
          break;

        case QUEST_RITEM_SKILL :
          strcpy(verbosevalstrn, getSkillTypeStrn(qstItem->itemVal));
          verbosevalstrn[intMaxLen] = '\0';
          break;

        case QUEST_RITEM_EXP :
          strcpy(verbosevalstrn, "experience");
          break;

        default :
          strcpy(verbosevalstrn, "Unrecognized");
          break;
      }

      break;
  }
}


//
// getKeyStrn : get readable key string shown at start of menu options
//

char *getKeyStrn(const char key, const char key2, char *strn)
{
  if (key2)
    sprintf(strn, " &+Y%c/%c&+L. &+w", key, key2);
  else
    sprintf(strn, "   &+Y%c&+L. &+w", key);

  return strn;
}


//
// displayMenuFooter : display common menu footer (Esc to quit, etc)
//

void displayMenuFooter(void)
{
  displayColorString("\n" MENU_COMMON "\n");
  displayColorString(getMenuPromptName());
}


//
// displayMenuNoFooter : display menu with no footer
//

void displayMenuNoFooter(const menu *menuDisp, const void *entityPtr)
{
  if (menuDisp->displayType == mtDisplayValuesInline)
    displayMenuInline(menuDisp, entityPtr);
  else
    displayMenuRightSide(menuDisp, entityPtr);
}


//
// displayMenu : display menu as defined in menu struct, split up between two different menu types
//               because the two are not very similar
//

void displayMenu(const menu *menuDisp, const void *entityPtr)
{
  displayMenuNoFooter(menuDisp, entityPtr);

  displayMenuFooter();
}


//
// displayMenuInline : display menu with current values shown inline, a la base room, obj, mob menus
//

void displayMenuInline(const menu *menuDisp, const void *entityPtr)
{
  const menuChoiceGroup *group = menuDisp->choiceGroupArr;
  const bool blnDispVerbose = getShowMenuInfoVal();
  const uint intScreenWidth = getScreenWidth();
  char currentLineStrn[4096];

  while (group->choiceArr)
  {

    // chKey == 0 means blank line
    if (group->chKey == 0)
    {
      currentLineStrn[0] = '\0';
    }
    else
    {
      const menuChoice *choice = group->choiceArr;

      bool displayedExtra = false;

      getKeyStrn(group->chKey, group->chKey2, currentLineStrn);

      // If choice has type direct, there are no other choices and only the first choice's group name is displayed
      if (choice->dataType == mctDirect)
      {
        strcat(currentLineStrn, choice->choiceGroupName);
      }
      else
      {

        // Run through each choice in the group, and for choices with multiple values, run through them too
        while (choice->choiceGroupName)
        {
          const menuChoice *activeChoice = choice;
          char addStrn[1024];
          bool gotMore = false;
          uint numbChoices = 0;
          uint intNumbExtraDisplayed = 0;

          // for first choice, always put 'Edit ' and value name on the first line
          if (choice == group->choiceArr)
          {
            sprintf(currentLineStrn + strlen(currentLineStrn), "Edit %s", choice->choiceGroupName);
            addStrn[0] = '\0';
          }
          else
          {
            strcpy(addStrn, choice->choiceGroupName);
          }

          // basic technique : run through each choice, as each item name and any current value are generated,
          // determine if name+value will fit on current line, if not, display the current line and bump
          // the new data to the next line
          while (activeChoice && activeChoice->choiceGroupName)
          {
            const menuChoiceDataType dataType = activeChoice->dataType;
            const menuChoiceListType listType = activeChoice->listType;
            const menuChoiceDisplayType displayType = activeChoice->displayType;
            const size_t offset = activeChoice->offset;

            // display current value of choice, but only if allowed by menu itself and by user
            if ((displayType != mcdDoNotDisplay) && blnDispVerbose)
            {
              char valstrn[512];
              char verbosevalstrn[512];

              const int val = getMenuDataTypeStrn(valstrn, dataType, offset, entityPtr, 511);

              // if first value, add left paren, otherwise add slash
              if (activeChoice == choice)
              {
                strcat(addStrn, " &+c(");
              }
              else if (choice->choiceMoreArr)
              {
                if ((displayType == mcdPipeEverySecond) && (intNumbExtraDisplayed == 2))
                {
                  strcat(addStrn, "|");
                  intNumbExtraDisplayed = 0;
                }
                else 
                {
                  strcat(addStrn, "/");
                }
              }

              if ((listType != mclNone) && (displayType != mcdNoListInline))
              {
                getMenuListTypeStrn(verbosevalstrn, listType, val, entityPtr, 511);
                sprintf(addStrn + strlen(addStrn), "%s &+c[%s]", verbosevalstrn, valstrn);
              }
              else
              {
                strcat(addStrn, valstrn);
              }

              intNumbExtraDisplayed++;
              displayedExtra = true;
            }

            if (gotMore)
            {
              activeChoice++;
            }
            else
            {
              activeChoice = choice->choiceMoreArr;

              gotMore = true;
            }
          } // while (activeChoice && activeChoice->choiceGroupName)

         // done with this choice and all its values, cap off list of values if there were any, add comma
         // & 'and' based on how many there were and how many remain

          if (displayedExtra)
            strcat(addStrn, "&+c)");

          if ((choice + 1)->choiceGroupName)
          {
            if ((choice + 2)->choiceGroupName)
              strcat(addStrn, "&n, ");
            else if (numbChoices == 1)
              strcat(addStrn, "&n and ");
            else
              strcat(addStrn, "&n, and ");
          }

         // if string being added fits on this line, add it to the current line, otherwise display the
         // current line and put the new string on a new line

          if (truestrlen(currentLineStrn) + truestrlen(addStrn) < intScreenWidth)
          {
            strcat(currentLineStrn, addStrn);
          }
          else
          {
            displayColorString(currentLineStrn);
            _outtext("\n");

            sprintf(currentLineStrn, "      %s", addStrn);
          }

          addStrn[0] = '\0';

          choice++;
          numbChoices++;
        }  // while (choice->choiceGroupName)
      }
    }

   // display any remaining line

    strcat(currentLineStrn, "\n");

    displayColorString(currentLineStrn);

    currentLineStrn[0] = '\0';

    group++;
  }
}


//
// reverseDisplayOrder : if true, show verbose description before numeric in right-side menu
//

bool reverseDisplayOrder(const menuChoiceListType mclType)
{
  return ((mclType >= mclObjectValue1) && (mclType <= mclObjectValue8));
}


//
// displayMenuRightSideProcess : show menu with current values on right side
//
//                               if intWidth == 0, determine width of max line, otherwise display
//

size_t displayMenuRightSideProcess(const menu *menuDisp, const void *entityPtr, const size_t intWidth)
{
  size_t intMaxLen = 0;

  const menuChoiceGroup *group = menuDisp->choiceGroupArr;
  char strn[4096];


  while (group->choiceArr)
  {
   // chKey == 0 is blank line

    if (group->chKey == 0)
    {
      if (intWidth)
        _outtext("\n");
    }
    else
    {
      const menuChoice *choice = group->choiceArr;
      bool gotMore = false;
      char key = group->chKey;

      while (choice && choice->choiceGroupName)
      {
        const menuChoiceDataType dataType = choice->dataType;
        const menuChoiceListType listType = choice->listType;
        const size_t offset = choice->offset;

        if (dataType != mctDirectNoKey)
        {
          getKeyStrn(key, group->chKey2, strn);
        }
        else
        {
          strn[0] = '\0';
        }

        if ((dataType != mctDirect) && (dataType != mctDirectNoKey) && !choice->choiceDescName)
        {
          strcat(strn, "Edit ");
        }

        if (choice->choiceDescName)
          strcat(strn, choice->choiceDescName);
        else if (choice->choiceName)
          strcat(strn, choice->choiceName);
        else
          strcat(strn, choice->choiceGroupName);

        if (choice->displayType != mcdDoNotDisplay)
        {
         // if in display mode, get value to display on the right, if in 'determine max length' mode,
         // don't need it yet

          const size_t len = strlen(strn);

          if (intWidth)
          {
            char valstrn[512];
            char verbosevalstrn[512];
            char addstrn[512];

            const int val = getMenuDataTypeStrn(valstrn, dataType, offset, entityPtr, 511);

            if (listType != mclNone)
            {
              getMenuListTypeStrn(verbosevalstrn, listType, val, entityPtr, 511);

              verbosevalstrn[0] = toupper(verbosevalstrn[0]);

              if (reverseDisplayOrder(listType))
                sprintf(addstrn, "%s &n(%s)", valstrn, verbosevalstrn);
              else
                sprintf(addstrn, "%s &n(%s)", verbosevalstrn, valstrn);
            }
            else
            {
              strcpy(addstrn, valstrn);
            }

            size_t i = len;

            for (; i < intWidth; i++)
              strn[i] = ' ';

            strn[i] = '\0';

            strcat(strn, addstrn);
          }
          else
          {
            if (len > intMaxLen)
              intMaxLen = len;
          }
        }

        strcat(strn, "\n");

        if (intWidth)
          displayColorString(strn);

        if (gotMore)
        {
          choice++;
        }
        else
        {
          choice = choice->choiceMoreArr;

          gotMore = true;
        }

        key++;
      }
    }

    group++;
  }

  return intMaxLen;
}


//
// displayMenuRightSide : right side menu, values shown to the right rather than inline with choices -
//                        first, determine max option width, then display appropriately formatted
//
//                        ignored: multiple choices per letter
//

void displayMenuRightSide(const menu *menuDisp, const void *entityPtr)
{
 // first get max len

  const size_t intMaxLen = displayMenuRightSideProcess(menuDisp, entityPtr, 0);

 // then display with appropriate width

  displayMenuRightSideProcess(menuDisp, entityPtr, intMaxLen + 8);
}


//
// interpretMenu : given user keypress, execute the appropriate choice
//
//                 returns true if it processed the key
//

bool interpretMenu(const menu *menuInterp, void *entityPtr, const usint ch)
{
  const menuChoiceGroup *group = menuInterp->choiceGroupArr;
  const menuChoice *maybeChoice = NULL;


 // menuKey of 0 in menuGroup is used to mean 'no key'

  if (ch == 0)
    return false;

  while (group->choiceArr)
  {
    if (group->chKey)
    {
      const menuChoice *choice = group->choiceArr;
      bool gotMore = false;
      char key = group->chKey;

      while (choice && choice->choiceGroupName)
      {
        if (ch == key)
        {
         // take keys that are directly defined in group first, then keys that are auto-inc'd from more
         // array

          if (gotMore)
            maybeChoice = choice;
          else
            return editMenuValue(choice, entityPtr);
        }

        if (gotMore)
        {
          choice++;
        }
        else
        {
          choice = choice->choiceMoreArr;

          gotMore = true;
        }

       // chKey2 may not be the ASCII character just after chKey, so assign key if this is the second choice

        if (key < group->chKey2)
          key = group->chKey2;
        else
          key++;
      }
    }

    group++;
  }

  if (maybeChoice)
    return editMenuValue(maybeChoice, entityPtr);

  return false;
}


//
// editMenuValue : given menu choice info and entity being edited, edit appropriate value
//
//                 returns true if it did something
//

bool editMenuValue(const menuChoice *choice, void *entityPtr)
{
  const menuChoiceDataType dataType = choice->dataType;
  const menuChoiceListType listType = choice->listType;
  const size_t offset = choice->offset;
  const char *editingStrn;
  room *roomPtr;
  objectType *obj;
  mobType *mob;
  zone *zonePtr;
  bool addedIdentKeyword;
  char promptStrn[512];
  bool *boolPtr;


  if (choice->choiceDescName)
    editingStrn = choice->choiceDescName;
  else if (choice->choiceName)
    editingStrn = choice->choiceName;
  else
    editingStrn = choice->choiceGroupName;

  sprintf(promptStrn, "&+CEnter %s: ", editingStrn);

  switch (dataType)
  {
   // basic types

    case mctString :
      editStrnVal((char *)entityPtr + offset, choice->intMaxLen, promptStrn);
      return true;

    case mctDieString :
      editDieStrnVal((char *)entityPtr + offset, choice->intMaxLen, promptStrn);
      return true;

    case mctShopMessage :
      editShopMessage((char *)entityPtr + offset, choice->intMaxLen, promptStrn, false, false);
      return true;

    case mctShopMessageProduced :
      editShopMessageProduced((shop*)entityPtr, (char *)entityPtr + offset, choice->intMaxLen, promptStrn);
      return true;

    case mctShopMessageTraded :
      editShopMessageTraded((shop*)entityPtr, (char *)entityPtr + offset, choice->intMaxLen, promptStrn);
      return true;

    case mctKeywords :
      editKeywords(choice->intMaxLen, (stringNode **)((char *)entityPtr + offset));
      return true;

    case mctInt :
    case mctIntNoZero :
      if (listType == mclNone)
      {
        editIntVal((int *)((char *)entityPtr + offset), (dataType == mctInt), promptStrn);
      }

     // has a list type

      else
      {
        editMenuValueList(choice, entityPtr);
      }

      return true;

    case mctUInt :
    case mctUIntNoZero :
    case mctPercentage :
    case mctMoney :
      if (listType == mclNone)
      {
        editUIntVal((uint *)((char *)entityPtr + offset), (dataType != mctUIntNoZero), promptStrn);
      }

     // has a list type

      else
      {
        editMenuValueList(choice, entityPtr);
      }

      return true;

    case mctBoolYesNo :
      boolPtr = (bool *)((char *)entityPtr + offset);
      *boolPtr = !(*boolPtr);

      return true;

    case mctDescriptionNoStartBlankLines :
    case mctDescription :
    case mctQuestDisappearanceDescription :
      *((stringNode **)((char *)entityPtr + offset)) = 
        editStringNodes(*((stringNode **)((char *)entityPtr + offset)), 
                        (dataType == mctDescriptionNoStartBlankLines));
      return true;

   // flags

    case mctRoomFlag :
      roomPtr = (room *)entityPtr;

      editFlags(room_bits, &(roomPtr->roomFlags), ENTITY_ROOM, roomPtr->roomName, roomPtr->roomNumber, 
                "room", g_roomFlagTemplates, 0, true);
      return true;

    case mctObjExtra :
      obj = (objectType *)entityPtr;

      editFlags(extra_bits, &(obj->extraBits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "extra", g_objExtraFlagTemplates, 0, true);
      return true;

    case mctObjExtra2 :
      obj = (objectType *)entityPtr;

      editFlags(extra2_bits, &(obj->extra2Bits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "extra2", g_objExtra2FlagTemplates, 0, true);
      return true;

    case mctObjWear :
      obj = (objectType *)entityPtr;

      editFlags(wear_bits, &(obj->wearBits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "wear", g_objWearFlagTemplates, 0, true);
      return true;

    case mctObjAnti :
      obj = (objectType *)entityPtr;

      editFlags(g_npc_class_bits, &(obj->antiBits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "anti", g_objAntiFlagTemplates, 0, true);
      return true;

    case mctObjAnti2 :
      obj = (objectType *)entityPtr;

      editFlags(g_race_names, &(obj->anti2Bits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "anti2", g_objAnti2FlagTemplates, 0, true);
      return true;

    case mctObjAffect1 :
      obj = (objectType *)entityPtr;

      editFlags(affected1_bits, &(obj->affect1Bits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "affect1", g_objAff1FlagTemplates, 0, true);
      return true;

    case mctObjAffect2 :
      obj = (objectType *)entityPtr;

      editFlags(affected2_bits, &(obj->affect2Bits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "affect2", g_objAff2FlagTemplates, 0, true);
      return true;

    case mctObjAffect3 :
      obj = (objectType *)entityPtr;

      editFlags(affected3_bits, &(obj->affect3Bits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "affect3", g_objAff3FlagTemplates, 0, true);
      return true;

    case mctObjAffect4 :
      obj = (objectType *)entityPtr;

      editFlags(affected4_bits, &(obj->affect4Bits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "affect4", g_objAff4FlagTemplates, 0, true);
      return true;

    case mctObjTrapFlag :
      obj = (objectType *)entityPtr;

      editFlags(g_objTrapFlagDef, &(obj->trapBits), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "trap", NULL, 0, true);
      return true;

    case mctMobAction :
      mob = (mobType *)entityPtr;

      editFlags(action_bits, &(mob->actionBits), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "action", g_mobActionFlagTemplates, 0, true);
      return true;

    case mctMobAggro :
      mob = (mobType *)entityPtr;

      editFlags(aggro_bits, &(mob->aggroBits), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "aggro", g_mobAggroFlagTemplates, 0, true);
      return true;

    case mctMobAggro2 :
      mob = (mobType *)entityPtr;

      editFlags(aggro2_bits, &(mob->aggro2Bits), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "aggro2", g_mobAggro2FlagTemplates, 0, true);
      return true;

    case mctMobAggro3 :
      mob = (mobType *)entityPtr;

      editFlags(aggro3_bits, &(mob->aggro3Bits), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "aggro3", g_mobAggro3FlagTemplates, 0, true);
      return true;

    case mctMobAffect1 :
      mob = (mobType *)entityPtr;

      editFlags(affected1_bits, &(mob->affect1Bits), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "affect1", g_mobAff1FlagTemplates, 0, true);
      return true;

    case mctMobAffect2 :
      mob = (mobType *)entityPtr;

      editFlags(affected2_bits, &(mob->affect2Bits), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "affect2", g_mobAff2FlagTemplates, 0, true);

      return true;

    case mctMobAffect3 :
      mob = (mobType *)entityPtr;

      editFlags(affected3_bits, &(mob->affect3Bits), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "affect3", g_mobAff3FlagTemplates, 0, true);
      return true;

    case mctMobAffect4 :
      mob = (mobType *)entityPtr;

      editFlags(affected4_bits, &(mob->affect4Bits), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "affect4", g_mobAff4FlagTemplates, 0, true);
      return true;

    case mctMobClass :
      mob = (mobType *)entityPtr;

      editFlags(g_npc_class_bits, &(mob->mobClass), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "class", NULL, 0, true);
      return true;

    case mctZoneFlag :

      zonePtr = (zone *)entityPtr;

      editFlags(g_zoneMiscFlagDef, &(zonePtr->miscBits.longIntFlags), ENTITY_ZONE, zonePtr->zoneName, 
                zonePtr->zoneNumber, "misc", NULL, 0, true);
      return true;

   // misc

    case mctRoomEdescs :
      editRoomExtraDesc((room *)entityPtr);
      return true;

    case mctObjEdescs :
      addedIdentKeyword = false;
      obj = (objectType *)entityPtr;

      if (editObjExtraDesc(obj, &addedIdentKeyword))
      {
       // user aborted, remove all _id_ keywords

        if (addedIdentKeyword)
          deleteMatchingStringNodes(&obj->keywordListHead, "_ID_");
      }

      return true;

    case mctExits :
      editRoomExits((room *)entityPtr);
      return true;

    case mctSpecies :
      editMenuValueListVerbose(choice, entityPtr);
      return true;

    case mctBuyMultiplier :
    case mctSellMultiplier :
      editShopMultiplier(choice, (shop *)entityPtr, promptStrn);
      return true;

    case mctShopFirstOpen :
    case mctShopSecondOpen :
      editShopOpen(choice, (shop *)entityPtr, promptStrn, dataType);
      return true;

    case mctShopFirstClose :
    case mctShopSecondClose :
      editShopClose(choice, (shop *)entityPtr, promptStrn, dataType);
      return true;

    case mctLowLifespan :
      editLowLifespan(choice, (zone *)entityPtr, promptStrn);
      return true;

    case mctHighLifespan :
      editHighLifespan(choice, (zone *)entityPtr, promptStrn);
      return true;

    case mctVarYesNo :
      bool (*getVarValFunc)(void) = (bool(*)(void))(choice->offset);
      setVarBoolVal(&g_varHead, choice->choiceGroupName, !getVarValFunc(), false);
      return true;
  }

  return false;
}


//
// editMenuValueList
//

void editMenuValueList(const menuChoice *choice, void *entityPtr)
{
  const menuChoiceListType listType = choice->listType;
  room *roomPtr;
  objectType *obj;
  mobType *mob;
  zone *zonePtr;

  switch (listType)
  {
    case mclRoomSector :
      roomPtr = (room *)entityPtr;

      editFlags(g_roomSectList, &(roomPtr->sectorType), ENTITY_ROOM, roomPtr->roomName, 
                roomPtr->roomNumber, "sector type", NULL, 0, false);
      break;

    case mclCurrentDirection :
      roomPtr = (room *)entityPtr;

      editFlags(g_roomExitDirectionList, &(roomPtr->currentDir), ENTITY_ROOM, roomPtr->roomName, 
                roomPtr->roomNumber, "current direction", NULL, 0, false);
      break;

    case mclRoomManaFlag :
      roomPtr = (room *)entityPtr;

      editFlags(g_roomManaList, &(roomPtr->manaFlag), ENTITY_ROOM, roomPtr->roomName, roomPtr->roomNumber,
                "mana type", NULL, 0, false);
      break;

    case mclObjectSize :
      obj = (objectType*)entityPtr;

      editFlags(g_objSizeList, &(obj->size), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "size", NULL, 0, false);
      break;

    case mclCraftsmanship :
      obj = (objectType*)entityPtr;

      editFlags(g_objCraftsmanshipList, &(obj->craftsmanship), ENTITY_OBJECT, getObjShortName(obj), 
                obj->objNumber, "craftsmanship", NULL, 0, false);
      break;

    case mclMaterial :
      editMenuValueListVerbose(choice, entityPtr);
      break;

    case mclObjectType :
      if (checkForObjHeresWithLoadedContainer(choice->intMaxLen))  // intMaxLen used to store original vnum
      {
        displayAnyKeyPrompt(
          "&+MCannot change type - objects exist of this type with items inside.  Press a key");
      }
      else
      {
        obj = (objectType*)entityPtr;

        editFlags(g_objTypeList, &(obj->objType), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                  "type", NULL, 0, false);
      }

      break;

    case mclObjectValue1 :
    case mclObjectValue2 :
    case mclObjectValue3 :
    case mclObjectValue4 :
    case mclObjectValue5 :
    case mclObjectValue6 :
    case mclObjectValue7 :
    case mclObjectValue8 :
      editObjValueField((objectType *)entityPtr, listType - mclObjectValue1);
      break;

    case mclApplyWhere :
      editMenuValueListVerbose(choice, entityPtr);
      break;

    case mclTrapDamage :
      obj = (objectType*)entityPtr;

      editFlags(g_objTrapDamList, &(obj->trapDam), ENTITY_OBJECT, getObjShortName(obj), obj->objNumber,
                "trap damage type", NULL, 0, false);
      break;

    case mclPosition :
      mob = (mobType*)entityPtr;

     // use offset and choice name since there are position and default position

      editFlags(g_mobPositionList, (int *)((char *)entityPtr + choice->offset), ENTITY_MOB, 
                getMobShortName(mob), mob->mobNumber, choice->choiceGroupName, NULL, 0, false);
      break;

    case mclSex :
      mob = (mobType*)entityPtr;

      editFlags(g_mobSexList, &(mob->sex), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "sex", NULL, 0, false);
      break;

    case mclHometown :
      mob = (mobType*)entityPtr;

      editFlags(g_mobHometownList, &(mob->mobHometown), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "hometown", NULL, 0, false);
      break;
    
    case mclMobSpec :
      mob = (mobType *)entityPtr;

      editSpecs(&(mob->mobSpec), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
	"specialize", NULL, 0, false);
      break;

    case mclMobSize :
      mob = (mobType*)entityPtr;

      editFlags(g_mobSizeList, &(mob->size), ENTITY_MOB, getMobShortName(mob), mob->mobNumber,
                "size", NULL, 0, false);
      break;

    case mclShopkeeperRace :
      mob = (mobType*)(choice->extraPtr);

      editFlags(g_shopShopkeeperRaceList, &(((shop*)entityPtr)->shopkeeperRace), ENTITY_SHOP, 
                getMobShortName(mob), mob->mobNumber, "race", NULL, 0, false);
      break;

    case mclResetMode :
      zonePtr = (zone*)entityPtr;

      editFlags(g_zoneResetModeList, &(zonePtr->resetMode), ENTITY_ZONE, zonePtr->zoneName, zonePtr->zoneNumber, 
                "reset mode", NULL, 0, false);
      break;

    case mclQuestItemGivenToMobType :
      mob = (mobType*)(choice->extraPtr);

      editFlags(g_questGiveTypeList, &(((questItem*)entityPtr)->itemType), ENTITY_QUEST, getMobShortName(mob), 
                mob->mobNumber, "type of thing given to mob", NULL, 0, false);
      break;

    case mclQuestItemGivenToPCType :
      mob = (mobType*)(choice->extraPtr);

      editFlags(g_questReceiveTypeList, &(((questItem*)entityPtr)->itemType), ENTITY_QUEST, 
                getMobShortName(mob), mob->mobNumber, "type of thing given to PC", NULL, 0, false);
      break;

    case mclQuestItemGivenToMobValue :
      editQuestItemGivenToMob(choice, (questItem *)entityPtr);
      break;

    case mclQuestItemGivenToPCValue :
      editQuestItemGivenToPC(choice, (questItem *)entityPtr);
      break;
  }
}


//
// editMenuValueListVerbose : given choice and pointer to entity being edited, edit enumerated value too
//                            long to be displayed in flag-style menu
//
//                            end var is either uint or string based on choice->dataType
//

void editMenuValueListVerbose(const menuChoice *choice, void *entityPtr)
{
  const menuChoiceDataType dataType = choice->dataType;
  const menuChoiceListType listType = choice->listType;
  const size_t offset = choice->offset;

  menuChoiceDataType dataTypeEditing = mctUInt;
  void (*displayListFuncPtr)(const char*) = NULL;

  const char *editingStrn;


  switch (dataType)
  {
    case mctSpecies :
      displayListFuncPtr = &displayMobSpeciesList;
      dataTypeEditing = mctString;
      break;
  }

  if (!displayListFuncPtr)
  {
    switch (listType)
    {
      case mclMaterial :
        displayListFuncPtr = &displayMaterialList;
        break;

      case mclApplyWhere :
        displayListFuncPtr = &displayObjApplyTypeList;
        break;
    }
  }

  if (choice->choiceDescName)
    editingStrn = choice->choiceDescName;
  else if (choice->choiceName)
    editingStrn = choice->choiceName;
  else
    editingStrn = choice->choiceGroupName;

  if (dataTypeEditing == mctString)
    editStrnValSearchableList((char *)entityPtr + offset, choice->intMaxLen, editingStrn, displayListFuncPtr);
  else
    editUIntValSearchableList((uint *)((char *)entityPtr + offset), true, editingStrn, displayListFuncPtr);
}


//
// editObjValueField : given obj being edited and value field #, edit value
//

void editObjValueField(objectType *obj, const uchar valueField)
{
  if (specialObjValEdit(obj, valueField, false))
  {
    specialObjValEdit(obj, valueField, true);
  }
  else
  {
    const bool helpAvail = checkForValueList(obj->objType, valueField),
               verboseAvail = checkForVerboseAvail(obj->objType, valueField),
               searchAvail = checkForSearchAvail(obj->objType, valueField);
    char promptstrn[256], valstrn[64];


    sprintf(promptstrn, "&+CEnter value for field #%u", valueField + 1);

   // assume verbose/search are only available if help is available

    if (helpAvail)
    {
      strcat(promptstrn, " (? for list");

      if (verboseAvail) 
        strcat(promptstrn, ", ?? for full list");

      if (searchAvail) 
        strcat(promptstrn, ", $ to search");

      strcat(promptstrn, "): ");
    }
    else 
    {
      strcat(promptstrn, ": ");
    }

    while (true)
    {
      sprintf(valstrn, "%d", obj->objValues[valueField]);

      editStrnVal(valstrn, 11, promptstrn);

      if (!strcmp(valstrn, "?") && helpAvail)
      {
        displayObjValueHelp(obj->objType, valueField, false);
      }
      else

     // below assumes that verbose is only available when help is available

      if (!strcmp(valstrn, "??") && verboseAvail)
      {
        displayObjValueHelp(obj->objType, valueField, true);
      }
      else

     // ditto for search

      if (!strcmp(valstrn, "$") && searchAvail)
      {
        searchObjValue(obj->objType, valueField);
      }
      else 
      if (strnumberneg(valstrn))
      {
        break;
      }
    }

    obj->objValues[valueField] = atoi(valstrn);
  }
}


//
// editShopMessage : edit message for a shop
//

void editShopMessage(char *message, const uint intMaxLen, const char *prompt, const bool blnOneOrTwoS,
                     const bool blnTwoS)
{
  char strn[MAX_SHOPSTRING_LEN + 1], howMany;
  const char *howManyErrStrn;


  while (true)
  {
    bool blnValid = false;

    strcpy(strn, message);

    editStrnVal(strn, intMaxLen, prompt);

    if (blnTwoS)
    {
      if (numbPercentS(strn) != 2)
      {
        howManyErrStrn = "two";
        howMany = 2;
      }
      else
      {
        blnValid = true;
      }
    }
    else
    {
      if (!blnOneOrTwoS)
      {
        if (numbPercentS(strn) != 1)
        {
          howManyErrStrn = "one";
          howMany = 1;
        }
        else
        {
          blnValid = true;
        }
      }
      else
      {
        if ((numbPercentS(strn) != 1) && (numbPercentS(strn) != 2))
        {
          howManyErrStrn = "one or two";
          howMany = 2;
        }
        else
        {
          blnValid = true;
        }
      }
    }

    if (blnValid)
    {
      strcpy(message, strn);
      break;
    }
    else
    {
      sprintf(strn, "&+YError - this message must contain %s '%%s'%s.  Press a key..", 
              howManyErrStrn, plural(howMany));
      displayAnyKeyPrompt(strn);
    }
  }
}


//
// editShopMessageProduced : allow 1/2 %s's or 2 %s's based on whether shop is selling anything
//

void editShopMessageProduced(const shop *shopPtr, char *message, const uint intMaxLen, const char *prompt)
{
 // if not selling anything, message can contain 1 or 2 %s, otherwise must be 2

  if (shopPtr->producedItemList[0])
    editShopMessage(message, intMaxLen, prompt, false, true);
  else
    editShopMessage(message, intMaxLen, prompt, true, false);
}


//
// editShopMessageTraded : allow 1/2 %s's or 2 %s's based on whether shop is buying anything
//

void editShopMessageTraded(const shop *shopPtr, char *message, const uint intMaxLen, const char *prompt)
{
 // if not buying anything, message can contain 1 or 2 %s, otherwise must be 2

  if (shopPtr->tradedItemList[0])
    editShopMessage(message, intMaxLen, prompt, false, true);
  else
    editShopMessage(message, intMaxLen, prompt, true, false);
}


//
// editShopMultiplier : edit either buy or sell multiplier
//

void editShopMultiplier(const menuChoice *choice, shop *shopPtr, const char *prompt)
{
  double *multPtr = (double *)((char *)shopPtr + choice->offset);
  double mult;

  while (true)
  {
    mult = *multPtr;

    editFloatVal(&mult, false, prompt, 2);

    if (choice->dataType == mctBuyMultiplier)
    {
      if ((mult <= 0.0) || (mult > 1.0))
      {
        displayAnyKeyPrompt("&+YError: Buy multiplier must be greater than 0.0 and less than or equal to 1.0");
      }
      else
      {
        break;
      }
    }
    else
    {
      if (mult < 1.0)
      {
        displayAnyKeyPrompt("&+YError: Sell multiplier must be greater than or equal to 1.0");
      }
      else
      {
        break;
      }
    }
  }

  *multPtr = mult;
}


//
// editShopOpen : edit shop open time
//

void editShopOpen(const menuChoice *choice, shop *shopPtr, const char *prompt, menuChoiceDataType dataType)
{
  uint *openPtr = NULL, *closePtr = NULL;
  uint val;

  if (dataType == mctShopFirstOpen)
  {
    openPtr = &(shopPtr->firstOpen);
    closePtr = &(shopPtr->firstClose);
  }
  else
  {
    openPtr = &(shopPtr->secondOpen);
    closePtr = &(shopPtr->secondClose);
  }

  do
  {
    val = *openPtr;

    editUIntVal(&val, true, prompt);
  } while (val >= *closePtr);

  *openPtr = val;
}


//
// editShopClose : edit shop close time
//

void editShopClose(const menuChoice *choice, shop *shopPtr, const char *prompt, menuChoiceDataType dataType)
{
  uint *openPtr = NULL, *closePtr = NULL;
  uint val;

  if (dataType == mctShopFirstClose)
  {
    openPtr = &(shopPtr->firstOpen);
    closePtr = &(shopPtr->firstClose);
  }
  else
  {
    openPtr = &(shopPtr->secondOpen);
    closePtr = &(shopPtr->secondClose);
  }

  do
  {
    val = *closePtr;

    editUIntVal(&val, true, prompt);
  } while (val <= *openPtr);

  *closePtr = val;
}


//
// editLowLifespan : edit zone low lifespan
//

void editLowLifespan(const menuChoice *choice, zone *zonePtr, const char *prompt)
{
  uint val;

  do
  {
    val = zonePtr->lifeLow;

    editUIntVal(&val, false, prompt);
  } while (val > zonePtr->lifeHigh);

  zonePtr->lifeLow = val;
}


//
// editHighLifespan : edit zone high lifespan
//

void editHighLifespan(const menuChoice *choice, zone *zonePtr, const char *prompt)
{
  uint val;

  do
  {
    val = zonePtr->lifeHigh;

    editUIntVal(&val, false, prompt);
  } while (val < zonePtr->lifeLow);

  zonePtr->lifeHigh = val;
}


//
// editQuestItemGivenToMob : edit quest item given to mob
//

void editQuestItemGivenToMob(const menuChoice *choice, questItem *qstItem)
{
  if (qstItem->itemType == QUEST_GITEM_OBJTYPE)
  {
    mobType *mob = (mobType *)choice->extraPtr;

    editFlags(g_objTypeList, &(qstItem->itemVal), ENTITY_QUEST, getMobShortName(mob), mob->mobNumber,
              "object type given to mob", NULL, 0, false);
  }
  else if (qstItem->itemType == QUEST_GITEM_OBJ)
  {
    editUIntValSearchableList(&(qstItem->itemVal), true, "object vnum", displayObjectTypeList);
  }
  else
  {
    editUIntVal(&(qstItem->itemVal), true, "&+CEnter value: ");
  }
}


//
// editQuestItemGivenToPC : edit quest item given to PC
//

void editQuestItemGivenToPC(const menuChoice *choice, questItem *qstItem)
{
  if (qstItem->itemType == QUEST_RITEM_SKILL)
  {
    editUIntValSearchableList(&(qstItem->itemVal), true, "skill", displaySkillList);
  }
  else if (qstItem->itemType == QUEST_RITEM_OBJ)
  {
    editUIntValSearchableList(&(qstItem->itemVal), true, "object vnum", displayObjectTypeList);
  }
  else
  {
    editUIntVal(&(qstItem->itemVal), true, "&+CEnter value: ");
  }
}
