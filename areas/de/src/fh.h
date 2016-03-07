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


// FH.H - forward declarations for durisEdit, the all-purpose handy-dandy
//        Duris area editor

#ifndef _FH_H_

#ifdef _WIN32
#  pragma warning(disable:4996)  // getcwd, unlink, others are 'deprecated' (as if)

   typedef unsigned short WORD;  // if MS changes this to something else, feel free to fix it
#endif

#include <stdio.h>  // FILE

#include "graphcon.h"

#include "types.h"
#include "boolean.h"
#include "display.h"
#include "misc/strnnode.h"
#include "command/command.h"
#include "command/var.h"
#include "command/alias.h"
#include "misc/loaded.h"
#include "misc/menu.h"
#include "room/room.h"
#include "zone/dispzone.h"
#include "zone/readzon.h"
#include "zone/writezon.h"

// WTH is this even here?
//#define ITEM_WEAR_WAIST  ITEM_WEAR_WAISTE  // "waiste"?  COME ON NOW

// alias.cpp

bool aliasExists(const alias *aliasHead, const char *aliasName);
alias *getAlias(alias *aliasHead, const char *aliasName);
void unaliasCmd(const char *args, alias **aliasHead);
char addAlias(alias **aliasHead, const char *aliasStrn, const char *strn);
void addAliasArgs(const char *origargs, const bool addLFs, const bool updateChanges, alias **aliasHead,
                  const bool display);
void aliasCmd(const char *args, const bool addLFs, alias **aliasHead);
size_t expandAliasArgs(char *endStrn, const char *commandStrn, const char *args, const size_t intMaxLen);

// check.cpp

bool outCheckError(const char *error, FILE *file, size_t& numbLines);
uint checkFlags(FILE *file, const uint flagVal, const flagDef *flagArr, const char *flagName, 
                const char *entityName, const uint entityNumb, size_t& numbLines, bool *userQuit);
uint checkAllFlags(FILE *file, size_t& numbLines, bool *userQuit);
uint checkZone(FILE *file, size_t& numbLines, bool *userQuit);
uint checkRooms(FILE *file, size_t& numbLines, bool *userQuit);
uint checkLoneRooms(FILE *file, size_t& numbLines, bool *userQuit);
bool checkExitKeyLoaded(const roomExit *exitNode);
uint checkMissingKeys(FILE *file, size_t& numbLines, bool *userQuit);
uint checkLoaded(FILE *file, size_t& numbLines, bool *userQuit);
bool checkExit(const roomExit *exitNode);
uint checkExits(FILE *file, size_t& numbLines, bool *userQuit);
bool checkExitDesc(const roomExit *exitNode);
uint checkExitDescs(FILE *file, size_t& numbLines, bool *userQuit);
uint checkObjects(FILE *file, size_t& numbLines, bool *userQuit);
uint checkObjectValueRange(const objectType *obj, const uchar valNumb, const int lowRange, 
                           const int highRange, const int exclude, FILE *file, size_t& numbLines, 
                           bool *userQuit);
uint checkObjValValidLevel(const objectType *obj, const char valNumb, FILE *file, size_t& numbLines, 
                           bool *userQuit);
uint checkObjValValidSpellType(const objectType *obj, const char valNumb, FILE *file, size_t& numbLines, 
                               bool *userQuit);
uint checkObjectValues(FILE *file, size_t& numbLines, bool *userQuit);
uint checkObjMaterial(FILE *file, size_t& numbLines, bool *userQuit);
uint checkMobs(FILE *file, size_t& numbLines, bool *userQuit);
uint checkObjectDescs(FILE *file, size_t& numbLines, bool *userQuit);
uint checkMobDescs(FILE *file, size_t& numbLines, bool *userQuit);
char checkEdesc(const extraDesc *descNode);
uint checkAllEdescs(FILE *file, size_t& numbLines, bool *userQuit);
bool checkAll(void);

// clone.cpp

void cloneEntity(const char *args);
void cloneRoom(const uint vnum, const uint toclone);
void cloneObjectType(const uint vnum, const uint toclone);
void cloneMobType(const uint vnum, const uint toclone);

// command.cpp

bool checkCommands(const char *strn, const command *cmd, const char *notfoundStrn,
                   bool (*execCommand)(const usint commands, const char *args),
                   bool *cmdFound, usint *matchingCmd);

// config.cpp

void displayEditConfigMenu(void);
bool interpEditConfigMenu(const usint ch);
void editConfig(void);

// copydesc.cpp

void copyRoomDesc(const uint tocopy, const uint copyto);
void copyMobDesc(const uint tocopy, const uint copyto);

// create.cpp

void createEntity(const char *args);

// crtdesc.cpp

extraDesc *createExtraDesc(extraDesc **extraDescHead, const char *keywordStrn, const bool copyDefault);

// crtedit.cpp

void createEdit(const char *args);
void createEditExit(const char *args);

// crtexit.cpp

bool createExit(roomExit **exitNode, const bool incNumbExits);
bool createExit(const uint roomNumb, const int destNumb, const char exitDir);

// crtmob.cpp

mobType *createMobType(const bool incLoaded, const uint mobNumb, const bool getFreeNumb);

// crtmobhu.cpp

void createMobHereUser(void);
void createMobHereStrn(const char *args);

// crtmobtu.cpp

mobType *createMobTypeUser(const char *args);

// crtobj.cpp

objectType *createObjectType(const bool incLoaded, const uint objNumb, const bool getFreeNumb);

// crtobjhu.cpp

void createObjectHereUser(void);
void createObjectHereStrn(const char *args);

// crtobjtu.cpp

objectType *createObjectTypeUser(const char *args);

// crtrexit.cpp

char *commonDisplayExit(const char *chstr, const char *exitName, const uint flag, const bool negateFlag,
                        const bool comma, char *strn);
char *buildCommonDisplayExitsStrn(char *strn, const uint exitTaken, const bool negateFlag);
int createRoomExitPrompt(room *room, const uint exitTaken, const bool incLoaded);
int createRoomExit(const int exitdir);
void preCreateExit(const char *args);

// crtroom.cpp

room *createRoom(const bool incLoaded, const uint roomNumb, const bool getFreeNumb);

// crtroomu.cpp

bool createRoomPrompt(const char *args);

// default.cpp

void readDefaultsFromFiles(const char *filePrefix);
void writeDefaultFiles(const char *filePrefix);
void editDefaultRoom(void);
void editDefaultObject(void);
void editDefaultMob(void);
void editDefaultExit(void);
void editDefaultExtraDesc(void);
void editDefaultQuest(void);
void editDefaultShop(void);
void deleteDefaultRoom(void);
void deleteDefaultObject(void);
void deleteDefaultMob(void);
void deleteDefaultExit(void);
void deleteDefaultExtraDesc(void);
void deleteDefaultQuest(void);
void deleteDefaultShop(void);

// delete.cpp

void deleteEntity(const char *args);

// delexitu.cpp

void deleteExitPromptPrompt(room *room, const uint exitTaken, const bool decLoaded);
void deleteExitPrompt(const char dir);
void preDeleteExitPrompt(const char *args);

// delmobhu.cpp

void deleteMobHereUser(const char *args);

// delmobt.cpp

void deleteMobType(mobType *srcMob, const bool decNumbMobs);
void deleteMobTypeAssocLists(mobType *srcMob);

// delmobtu.cpp

bool checkDeleteMobsofType(const mobType *mobType);
void deleteMobTypeUser(const char *args);
void deleteMobTypeUserPrompt(void);

// delobjh.cpp

void deleteObjHereList(objectHere *srcObjHere, const bool decLoaded);
void deleteObjHere(objectHere *objHere, const bool decLoaded);
void deleteObjHereinList(objectHere **objHead, objectHere *objHere, const bool decLoaded);
void removeObjHerefromList(objectHere **objHead, objectHere *objHere, const bool decLoaded);
void deleteAllObjHereofTypeInList(objectHere **objHereHead, const objectType *objType,
                                  const bool decLoaded);
void deleteAllObjHeresofType(const objectType *objType, const bool decLoaded);

// delobjhu.cpp

void deleteObjectHereUser(const char *args);

// delobjt.cpp

void deleteObjectType(objectType *srcObject, const bool updateGlobal);
void deleteObjectTypeAssocLists(objectType *srcObject);

// delobjtu.cpp

bool checkDeleteObjectsofType(const objectType *objType);
void deleteObjectTypeUser(const char *args);
void deleteObjectTypeUserPrompt(void);

// delqstu.cpp

void deleteQuestUser(const char *args);
void deleteQuestUserPrompt(void);

// delroomu.cpp

void deleteRoomUser(const char *args);

// delshpu.cpp

void deleteShopUser(const char *args);
void deleteShopUserPrompt(void);

// dispflag.cpp

void displayFlagMenu(const uint val, const flagDef *flagArr, const char entityType, const char *entityName,
                     const uint entityNumb, const char *flagName, sint *colXpos, uint *colTopFlag, 
                     uint *numbCols, const bool asBitVect);
void updateSingleFlag(const uint flagVal, const uchar flagNumb, const uint numbFlags, const uint numbCols,
                      const sint *colXpos, const uint *colTopFlag, const sint headerOffset, 
                      const bool asBitVect, const flagDef *flagArr);
void updateFlags(const uint flagVal, const uint numbFlags, const uint numbCols, const sint *colXpos,
                 const uint *colTopFlag, const sint headerOffset, const uchar displayNonset, 
                 const bool asBitVect, const flagDef *flagArr);

// display.cpp

void displayColorString(const char *strn);
void displayColorString(char *strn);
uchar getTextColor(const char chCode, const bool isBG, bool *isValid, bool *isFlash);
void displayStringNodes(const stringNode *rootNode);
bool displayStringNodesPause(const stringNode *rootNode, size_t &numbLines);
void displayPrompt(void);
char *remColorCodes(char *strn);
void createMenuHeader(char *strn, const char entityType, const char *entityName, const uint entityNumb, 
                      const char *entitySubName, bool asBitVect);
void createShortMenuHeader(char *strn, const char entityType, const uint entityNumb, 
                           const char *entitySubName, bool asBitVect);
void createMenuHeaderBasic(char *strn, const char *preName, const char *entityName);
size_t preDisplayLongMenuHeader(char *newEntityName, const char *entityName, const size_t maxLen);
void displayMenuHeaderBasic(const char *preName, const char *entityName);
void displayMenuHeader(const char entityType, const char *entityName, const uint entityNumb, 
                       const char *entitySubName, bool asBitVect);
void displaySimpleMenuHeader(const char *editWhat);
void displayAnyKeyPrompt(const char *prompt);
void displayAnyKeyPromptNoClr(const char *prompt);
promptType displayYesNoPrompt(const char *prompt, const promptType defaultChoice, const bool blnClearLine);

// dispmisc.cpp

void displayList(const int startVal, const int endVal, const char *searchStrn,
                 const char *(*getStringVal)(const int val), 
                 const char *(*getIdentifierVal)(const int val),
                 const char *ignoreStrn, const char *whatList);
void displayMaterialList(const char *searchStrn);
void displaySpellList(const char *searchStrn);
void displaySkillList(const char *searchStrn);
void displayCommands(const command *startCmd);
bool checkPause(const char *strn, size_t &lines);
void displayAllocError(const char *whatAlloc, const char *whereAlloc);
void displayArgHelp(void);

// dispmob.cpp

char *getClassString(const mobType *mob, char *strn, const size_t intMaxLen);
void displayMobTypeList(const char *strn);
void displayMobHereList(void);
void displayMobSpeciesList(const char *searchStrn);
void displayMobInfo(const char *args);

// dispobj.cpp

void displayObjectTypeList(const char *strn);
void displayObjectTypeList(const char *strn, const bool scanTitle);
void displayObjectHereList(void);
bool displayEntireObjectHereList(const objectHere *objHead, const char *postStrn, size_t &lines,
                                 bool *foundMatch);
bool displayEntireObjectHereList(const objectHere *objHead, const char *postStrn,
                                 size_t &lines, const char *keyStrn,
                                 const uint roomNumber, bool *foundMatch);
bool displayEntireObjectHereList(const objectHere *objHead, const char *postStrn, size_t &lines,
                                 const uint objNumber, const uint roomNumber, bool *foundMatch);
void displayObjApplyTypeList(const char *searchStrn);
void displayObjectInfo(const char *args);

// disproom.cpp

void displayRoomList(const char *strn);
bool displayRoomFlagsandSector(size_t &numbLines);
void displayExitList(const char *strn);
char *getDoorStateStrn(const roomExit *exitNode, char *strn);
char *getExitDestStrn(const roomExit *exitNode, char *strn);
void displayRoomInfo(const char *args);
void displayRoomExitInfo(void);

// dispzone.cpp

void addZoneExittoList(zoneExit **head, const roomExit *ex, const char dir, const room *room);
void displayZoneExits(const char *args);
void displayZoneInfo(void);

// drop.cpp

bool dropEntityObj(objectHere *objH, objectHere **objHdest, const bool deleteOriginal, size_t &numbLines);
bool dropEntityMob(mobHere *mobH, mobHere **mobHdest, const bool deleteOriginal, size_t& numbLines);
bool dropEntityEdesc(extraDesc *desc, const bool deleteOriginal, size_t& numbLines);
void dropEntityCmd(const char *args, const bool deleteOriginal);

// edesc.cpp

uint getNumbExtraDescs(const extraDesc *extraDescNode);
extraDesc *copyExtraDescs(const extraDesc *srcExtraDesc);
void deleteExtraDescs(extraDesc *srcExtraDesc);
extraDesc *copyOneExtraDesc(const extraDesc *srcExtraDesc);
void addExtraDesctoList(extraDesc **descHead, extraDesc *desc);
void removeExtraDescfromList(extraDesc **descHead, extraDesc *desc);
void deleteOneExtraDesc(extraDesc *extraDescNode);
bool compareExtraDescs(const extraDesc *list1, const extraDesc *list2);
bool compareOneExtraDesc(const extraDesc *node1, const extraDesc *node2);
extraDesc *getEdescinList(extraDesc *descHead, const char *keyword);
const extraDesc *getEdescinList(const extraDesc *descHead, const char *keyword);
void displayExtraDescNodes(const extraDesc *extraDescNode);

// editable.cpp

void addEditabletoList(editableListNode **edListHead, editableListNode *edToAdd);
editableListNode *createEditableListNode(const stringNode *keywordList, void *entityPtr, const char entityType);
editableListNode *createEditableListNode(const editableListNode *srcNode);
editableListNode *createEditableList(room *roomPtr);
char checkEditableList(const char *strn, editableListNode *editNode,
                       char *whatMatched, editableListNode **matchingNode,
                       char currentEntity);
editableListNode *copyEditableList(const editableListNode *srcNode);
void deleteEditable(editableListNode *edit);
void deleteEditableList(editableListNode *node);
void deleteEditableinList(editableListNode **editHead, editableListNode *edit);

// editchck.cpp

void displayEditCheckMenu(void);
bool interpEditCheckMenu(const usint ch);
void editCheck(void);

// editdesc.cpp

void editDesc(stringNode **descHead);
void displayEditExtraDescMenu(const extraDesc *extraDescNode);
bool interpEditExtraDescMenu(const usint ch, extraDesc *extraDescNode);
bool editExtraDesc(extraDesc *extraDescNode);
void deleteExtraDescUser(extraDesc **extraDescHead, uint *numbExtraDescs);

// editdisp.cpp

void displayEditDisplayMenu(void);
bool interpEditDisplayMenu(const usint ch);
void editDisplay(void);

// editent.cpp

void editEntity(const char *args);

// editexit.cpp

void getExitStateMenuStrn(char *strn, const char ch, const char *descStrn, const bool active);
void displayEditExitStateMenu(const room *room, const roomExit *exitNode,
                              const char *exitName);
bool interpEditExitStateMenu(const usint ch, const room *room, roomExit *exitNode, const char *exitName);
void editExitState(const room *room, roomExit *exitNode, const char *exitName);
void displayEditExitMenu(const room *room, const char *exitName,
                         const roomExit *exitNode);
bool interpEditExitMenu(const usint ch, const room *room, roomExit *exitNode,
                        const char *exitName);
void editExit(room *room, roomExit **exitNode, const char *exitName,
              const bool updateMadeChanges);
void preEditExit(const char *args);

void editSector();

// editflag.cpp

bool canEditFlag(const flagDef &flagInf, uint flagState, const bool asBitV);
bool isFlagValid(const flagDef &flagInf, uint flagState, const bool asBitV);
bool checkCommonFlagKeys(const usint ch, const flagDef *flagArr, uint *flagValue, const int numbFlags,
                         const sint *colXpos, const int *colTopFlag, const int numbCols, 
                         const sint headerOffset, const sint row, const bool asBitVect);
bool checkCommonFlagKeys(const usint ch, const flagDef *flagArr, int *flagValue, const int numbFlags,
                         const sint *colXpos, const int *colTopFlag, const int numbCols, 
                         const sint headerOffset, const sint row, const bool asBitVect);
bool interpEditFlags(usint ch, const flagDef *flagArr, uint *flagVal, const int numbFlags, 
                     const sint *colXpos, const int *colTopFlag, const int numbCols, uint *templates, 
                     const bool asBitVect);
bool interpEditFlags(usint ch, const flagDef *flagArr, int *sflagVal, const int numbFlags, 
                     const sint *colXpos, const int *colTopFlag, const int numbCols, uint *templates, 
                     const bool asBitVect);
bool editFlags(const flagDef *flagArr, uint *flagVal, const char entityType, const char *entityName, 
               const uint entityNumb, const char *flagName, uint *templates, uint numbCols, 
               const bool asBitVect);
bool editFlags(const flagDef *flagArr, int *sflagVal, const char entityType, const char *entityName, 
               const uint entityNumb, const char *flagName, uint *templates, uint numbCols, 
               const bool asBitVect);
void updateSpecList(const mobType *mob);
bool editSpecs(uint *flagVal, const char entityType, const char *entityName, 
               const uint entityNumb, const char *flagName, uint *templates, uint numbCols,
               const bool asBitVect);

// editichk.cpp

void displayEditMiscCheckMenu(void);
bool interpEditMiscCheckMenu(const usint ch);
void editMiscCheck(void);

// editmchk.cpp

void displayEditMobCheckMenu(void);
bool interpEditMobCheckMenu(const usint ch);
void editMobCheck(void);

// editmms2.cpp

void displayEditMobMisc2Menu(const mobType *mob);
bool interpEditMobMisc2Menu(const usint ch, mobType *mob);
void editMobMisc2(mobType *mob);

// editmms3.cpp

void displayEditMobMisc3Menu(const mobType *mob);
bool interpEditMobMisc3Menu(const usint ch, mobType *mob);
void editMobMisc3(mobType *mob);

// editmmsc.cpp

void displayEditMobMiscMenu(const mobType *mob);
char interpEditMobMiscMenu(const usint ch, mobType *mob);
void editMobMisc(mobType *mob);

// editmob.cpp

void displayEditMobTypeMenu(const mobType *mob, const uint origVnum);
bool interpEditMobTypeMenu(const usint ch, mobType *mob, const uint origVnum);
mobType *realEditMobType(mobType *mob, const bool allowJump);
void editMobType(mobType *mob, const bool allowJump);
char jumpMob(uint *numb);

// editmobu.cpp

void editMobTypeStrn(const char *args);
void editMobTypePrompt(void);

// editobj.cpp

void displayEditObjTypeMenu(const objectType *obj, const uint origVnum);
bool interpEditObjTypeMenu(const usint ch, objectType *obj, const uint origVnum);
objectType *realEditObjType(objectType *obj, const bool allowJump);
void editObjType(objectType *obj, const bool allowJump);
char jumpObj(uint *numb);

// editobju.cpp

void editObjectTypeStrn(const char *args);
void editObjectTypePrompt(void);

// editochk.cpp

void displayEditObjCheckMenu(void);
bool interpEditObjCheckMenu(const usint ch);
void editObjCheck(void);

// editoexd.cpp

void displayEditObjExtraDescMenu(const objectType *obj, const extraDesc *extraDescHead,
                                 uint *numbExtraDescs);
bool interpEditObjExtraDescMenu(const usint ch, objectType *obj, extraDesc **extraDescHead,
                                uint *numbExtraDescs, bool *addedKeyword);
bool editObjExtraDesc(objectType *obj, bool *addedIdentKeyword);

// editoms2.cpp

void displayEditObjMisc2Menu(const objectType *obj);
bool interpEditObjMisc2Menu(const usint ch, objectType *obj);
void editObjMisc2(objectType *obj);

// editomsc.cpp

void displayEditObjMiscMenu(const objectType *obj);
bool interpEditObjMiscMenu(const usint ch, objectType *obj, const uint origVnum);
void editObjMisc(objectType *obj, const uint origVnum);

// editotrp.cpp

void displayEditObjTrapInfoMenu(const objectType *obj);
bool interpEditObjTrapInfoMenu(const usint ch, objectType *obj);
void editObjTrapInfo(objectType *obj);

// editqst.cpp

void displayEditQuestMenu(const quest *qst, const mobType *mob);
char interpEditQuestMenu(const usint ch, quest *qst, mobType *mob);
mobType *realEditQuest(quest *qst, mobType *mob, const bool allowJump);
void editQuest(mobType *mob, const bool allowJump);
char jumpQuest(uint *numb);

// editqsti.cpp

void displayEditQuestItemMenu(const questItem *item, const mobType *mob, const char giveRecv);
bool interpEditQuestItemMenu(const usint ch, questItem *item, const mobType *mob, const char giveRecv);
void editQuestItem(questItem *item, const mobType *mob, const char giveRecv);

// editqstm.cpp

void displayEditQuestMessageMenu(const questMessage *msg);
bool interpEditQuestMessageMenu(const usint ch, questMessage *msg);
bool editQuestMessage(questMessage *msg);

// editqstq.cpp

void displayEditQuestQuestMenu(const questQuest *qst, const mobType *mob);
bool interpEditQuestQuestMenu(const usint ch, questQuest *qst, const mobType *mob);
bool editQuestQuest(questQuest *qst, const mobType *mob);

// editqstu.cpp

void editQuestStrn(const char *strn);
void editQuestPrompt(void);

// editrchk.cpp

void displayEditRoomCheckMenu(void);
bool interpEditRoomCheckMenu(const usint ch);
void editRoomCheck(void);

// editrexd.cpp

void displayEditRoomExtraDescMenu(const room *room, const extraDesc *extraDescHead, uint *numbExtraDescs);
bool interpEditRoomExtraDescMenu(const usint ch, room *room, extraDesc **extraDescHead, 
                                 uint *numbExtraDescs);
void editRoomExtraDesc(room *room);

// editrext.cpp

void displayEditRoomExitsMenu(const room *room, uint *exitFlagsVal);
bool interpEditRoomExitsMenu(const usint ch, room *room, uint *exitFlagsVal);
void editRoomExits(room *room);

// editrmsc.cpp

void displayEditRoomMiscMenu(const room *room);
bool interpEditRoomMiscMenu(const usint ch, room *room);
void editRoomMisc(room *room);

// editroom.cpp

void displayEditRoomMenu(const room *room);
bool interpEditRoomMenu(const usint ch, room *roomPtr, const uint origVnum);
room *editRoom(room *roomPtr, const bool allowJump);
void preEditRoom(const char *args);
char jumpRoom(uint *numb);

// editshp.cpp

void displayEditShopMenu(const shop *shp, const mobType *mob);
char interpEditShopMenu(const usint ch, shop *shp, mobType *mob);
mobType *realEditShop(shop *shp, mobType *mob, const bool allowJump);
void editShop(mobType *mob, const bool allowJump);
char jumpShop(uint *numb);

// editshpb.cpp

void displayEditShopBooleansMenu(const shop *shp, const mobType *mob);
bool interpEditShopBooleansMenu(const usint ch, shop *shp, mobType *mob);
void editShopBooleans(shop *shp, mobType *mob);

// editshpc.cpp

void displayEditShopTimesMenu(const shop *shp, const mobType *mob);
bool interpEditShopTimesMenu(const usint ch, shop *shp, mobType *mob);
void editShopTimes(shop *shp, mobType *mob);

// editshpm.cpp

void displayEditShopMessagesMenu(const mobType *mob);
bool interpEditShopMessagesMenu(const usint ch, shop *shp, mobType *mob);
void editShopMessages(shop *shp, mobType *mob);

// editshpr.cpp

void displayEditShopRacistMenu(const shop *shp, const mobType *mob);
bool interpEditShopRacistMenu(const usint ch, shop *shp, mobType *mob);
void editShopRacist(shop *shp, mobType *mob);

// editshps.cpp

void displayEditShopSoldMenu(const shop *shp, const mobType *mob);
bool interpEditShopSoldMenu(const usint ch, shop *shp, mobType *mob);
void editShopSold(shop *shp, mobType *mob);

// editshpt.cpp

void displayEditShopBoughtMenu(const shop *shp, const mobType *mob);
bool interpEditShopBoughtMenu(const usint ch, shop *shp, mobType *mob);
void editShopBought(shop *shp, mobType *mob);

// editshpu.cpp

void editShopStrn(const char *args);
void editShopPrompt(void);

// editval.cpp

char *editStrnVal(char *strn, const size_t intMaxLen, const char *prompt);
char *editDieStrnVal(char *strn, const size_t intMaxLen, const char *prompt);
char *editStrnValHelp(char *strn, const size_t intMaxLen, const char *prompt, bool *wantedHelp);
char *editStrnValHelpSearch(char *strn, const size_t intMaxLen, const char *prompt, bool *wantedHelp,
                            bool *wantedSearch);
void editStrnValSearchableList(char *strn, const size_t intMaxLen, const char *editingStrn, 
                               void (*displayListFuncPtr)(const char *));
stringNode *editKeywords(const size_t intMaxLen, stringNode **keywordListHeadPtr);
uint editUIntVal(uint *val, const bool allowZero, const char *prompt);
uint editUIntValHelp(uint *val, const bool allowZero, const char *prompt, bool *wantedHelp);
uint editUIntValHelpSearch(uint *val, const bool allowZero, const char *prompt, bool *wantedHelp,
                           bool *wantedSearch);
void editUIntValSearchableList(uint *val, const bool allowZero, const char *editingStrn, 
                               void (*displayListFuncPtr)(const char *));
int editIntVal(int *val, const bool allowZero, const char *prompt);
int editIntValHelp(int *val, const bool allowZero, const char *prompt, bool *wantedHelp);
int editIntValHelpSearch(int *val, const bool allowZero, const char *prompt, bool *wantedHelp,
                         bool *wantedSearch);
void editIntValSearchableList(int *val, const bool allowZero, const char *editingStrn, 
                              void (*displayListFuncPtr)(const char *));
double editFloatVal(double *val, const bool allowZero, const char *prompt, const uint intNumbMantissaDigits);

// editzmsc.cpp

void displayEditZoneMiscMenu(const zone *zone);
bool interpEditZoneMiscMenu(const usint ch, zone *zone);
void editZoneMisc(zone *zone);

// editzone.cpp

void displayEditZoneMenu(const zone *zone);
bool interpEditZoneMenu(const usint ch, zone *zone);
void editZone(zone *zone);

// equip.cpp

bool equipEquiponMob(mobHere *mob, objectHere *obj, bool *equipped, size_t &numbLines);
void equipMobSpecific(mobHere *mob, const char *args);
void equipMob(const char *strn);

// exit.cpp

char getDirfromKeyword(const char *strn);
bool exitNeedsKey(const roomExit *ex, const int keyNumb);
void checkRoomExitKey(const roomExit *exitNode, const room *room, const char *exitName);
bool compareRoomExits(const roomExit *exit1, const roomExit *exit2);
void deleteRoomExit(roomExit *srcExit, const bool decNumbExits);
roomExit *copyRoomExit(const roomExit *srcExit, const bool incNumbLoaded);
uint getExitTakenFlags(const room *room);
char *getRoomExitsShortStrn(const room *room, char *strn);
bool swapExits(const char *args);
void swapExitsNorthSouth(void);
void swapExitsWestEast(void);
void swapExitsUpDown(void);
int getWorldDoorType(const roomExit *exitRec);
int getZoneDoorState(const roomExit *exitRec);
void resetExits(const int destRoom, const int newDest);
void clearExits(const int destRoom, const bool decNumbExits);
roomExit *findCorrespondingExit(const int srcRoom, const int destRoom, char *exitDir);
const char *getExitStrn(const int exitDir);
roomExit *getExitNode(const uint roomNumb, const char exitDir);
roomExit *getExitNode(const room *room, const char exitDir);
void checkExit(roomExit *exitNode, const char *exitName, const int roomNumb);
void checkAllExits(void);
char *getDescExitStrnforZonefile(const room *roomPtr, const roomExit *exitRec,
                                 char *endStrn, const char *exitName);
char getNumbExits(const room *room);
bool isExitOut(const roomExit *ex);
bool roomExitOutsideZone(const roomExit *ex);

// flags.cpp

int whichFlag(const char *input, const flagDef *flagArr);
char *getFlagStrn(const uint flagVal, const flagDef *flagArr, char *strn, const size_t intMaxLen);
const char *getFlagNameFromList(const flagDef *flagList, const int intVal);
uint fixFlags(uint *flagVal, const flagDef *flagArr);
void fixAllFlags(void);
bool checkForFlagExistence(const bool checkRooms, const bool checkObjs, const bool checkMobs,
                           const char *flagName);
int getFlagLocRoom(const char *flagName, uint *whichSet, const flagDef **flagListPtr);
int getFlagLocObj(const char *flagName, uint *whichSet, const flagDef **flagListPtr);
int getFlagLocMob(const char *flagName, uint *whichSet, const flagDef **flagListPtr);
uint *getRoomFlagPtr(room *roomPtr, const uint flagVect);
uint *getObjFlagPtr(objectType *obj, const uint flagVect);
uint *getMobFlagPtr(mobType *mob, const uint flagVect);
void which(const char *args);
void massSet(const char *args);

// getstrn.cpp

void dispGetStrnField(char *strn, const size_t startPos, const size_t maxLen,
                      const bool wideString, const uint maxDisplayable,
                      const sint x, const sint y, const char *fillerChStrn);
char *getStrn(char *strn, const size_t maxLen, const char bgcol,
              const char fgcol, const uchar fillerCh, const char *oldStrn,
              const bool addToCommandHist, const bool prompt);
char *getStrn(char *strn, const size_t maxLen, const char bgcol,
              const char fgcol, const uchar fillerCh, const bool addToCommandHist, 
              const bool prompt);

// history.cpp

void addCommand(const char *strn, stringNode **g_commandHistory);

// init.cpp

void startInit(void);

// interp.cpp

char *expandUserInputString(char *inputStrn, char *outputStrn1, char *outputStrn2, const size_t intMaxLen);
bool interpCommands(const char *inputStrn, const bool dispPrompt);

// inv.cpp

void showInventory(void);
void readInventoryFile(void);
void writeInventoryFile(void);
void writeInventoryFileObjInside(FILE *invFile, const objectHere *objH, const uint intContainerID, uint *intID);
void writeInventoryFileMobEquipment(FILE *invFile, const mobHere *mobH, const uint intMobID, uint *intID);

// keywords.cpp

stringNode *createKeywordList(const char *keywordStrn);
bool addKeywordtoList(stringNode **keywordHead, const char *keyword);
char *getReadableKeywordStrn(const stringNode *keywordHead, char *endStrn, const size_t intMaxLen);
bool scanKeyword(const char *userinput, const stringNode *keywordListHead);
char *createKeywordString(const stringNode *strnNode, char *keyStrn, const size_t intMaxLen);

// list.cpp

void list(const char *args);

// load.cpp

void loadEntity(const char *args);

// loaded.cpp

void addEntity(const char entityType, const uint entityNumb);
void setEntityOverride(const char entityType, const uint entityNumb, const uint override);
const entityLoaded *getFirstEntityOverride(const entityLoaded *);
void deleteEntityinLoaded(const uint entityType, const uint entityNumb);
void decEntity(const uint entityType, const uint entityNumb);
uint getNumbEntities(const uint entityType, const uint entityNumb, const bool includeOverride);
uint getEntityOverride(const uint entityType, const uint entityNumb);
const entityLoaded *getNumbEntitiesNode(const uint entityType, const uint entityNumb);
void resetNumbLoaded(const uint entityType, const uint oldNumb, const uint newNumb);
void displayLoadedList(const char *args);
void setLimitArgs(const char *args);
void setLimitArgsStartup(const char *args);
void setLimitOverrideObj(const char *args, const bool updateChanges, const bool display);
void setLimitOverrideMob(const char *args, const bool updateChanges, const bool display);

// look.cpp

char *getObjShortNameDisplayStrn(char *strn, const objectHere *objHere, const uint intIndent, 
                                 const bool blnKeywords);
bool displayIndividualObj(const objectHere *objHere, const uint intIndent, size_t &numbLines);
bool displayContainerContents(const objectHere *objInside, const uint intIndent, size_t &numbLines);
void lookObj(const objectHere *objHere, size_t &numbLines);
void lookMob(const mobHere *mob, size_t &numbLines);
bool displayMobEqPos(const mobHere *mob, const char pos, const char *posStrn, const uint intIndent,
                     size_t &numbLines);
bool displayMobEquip(const mobHere *mob, const uint intIndent, size_t& numbLines);
bool displayMobMisc(const mobHere *mob, size_t &numbLines);
void lookEntity(const char *args);
void lookExit(const roomExit *exitNode);
void lookInObj(const char *args);
void displayCurrentRoom(void);
void look(const char *args);

// maincomm.cpp

bool commandQuit(void);
bool mainExecCommand(const usint command, const char *inargs);

// map.cpp

bool isUDmapRoom(const uint numb);
void displayMap(void);

// master.cpp

const char *getEntityTypeStrn(const uchar entityType);
masterKeywordListNode *createMasterListNode(const stringNode *keywords, const void *entityPtr, 
                                            const stringNode *desc, const char entityType);
bool addMasterNodeToList(masterKeywordListNode *head, masterKeywordListNode *nodeAdd);
masterKeywordListNode *createMasterKeywordList(room *roomPtr);
const stringNode *checkCurrentMasterKeywordList(const char *strn, char *whatMatched, 
                                                masterKeywordListNode **matchingNode);
void deleteMasterKeywordNode(masterKeywordListNode *node);
void deleteMasterKeywordList(masterKeywordListNode *node);

// menu.cpp

long getMenuDataTypeStrn(char *valstrn, const menuChoiceDataType dataType, const size_t offset, 
                        const void *entityPtr, const size_t intMaxLen);
void getMenuListTypeStrn(char *verbosevalstrn, const menuChoiceListType listType, const int val, 
                         const void *entityPtr, const size_t intMaxLen);
char checkMenuKey(const usint ch, const bool flagMenu);
void displayMenuNoFooter(const menu *menuDisp, const void *entityPtr);
void displayMenu(const menu *menuDisp, const void *entityPtr);
void displayMenuFooter(void);
void displayMenuInline(const menu *menuDisp, const void *entityPtr);
void displayMenuRightSide(const menu *menuDisp, const void *entityPtr);
bool interpretMenu(const menu *menuInterp, void *entityPtr, const usint ch);
bool editMenuValue(const menuChoice *choice, void *entityPtr);
void editMenuValueList(const menuChoice *choice, void *entityPtr);
void editMenuValueListVerbose(const menuChoice *choice, void *entityPtr);
void editObjValueField(objectType *obj, const uchar valueField);
void editShopMessage(char *message, const uint intMaxLen, const char *prompt, const bool blnOneOrTwoS,
                     const bool blnTwoS);
void editShopMessageProduced(const shop *shopPtr, char *message, const uint intMaxLen, const char *prompt);
void editShopMessageTraded(const shop *shopPtr, char *message, const uint intMaxLen, const char *prompt);
void editShopMultiplier(const menuChoice *choice, shop *shopPtr, const char *prompt);
void editShopOpen(const menuChoice *choice, shop *shopPtr, const char *prompt, menuChoiceDataType dataType);
void editShopClose(const menuChoice *choice, shop *shopPtr, const char *prompt, menuChoiceDataType dataType);
void editLowLifespan(const menuChoice *choice, zone *zonePtr, const char *prompt);
void editHighLifespan(const menuChoice *choice, zone *zonePtr, const char *prompt);
void editQuestItemGivenToMob(const menuChoice *choice, questItem *qstItem);
void editQuestItemGivenToPC(const menuChoice *choice, questItem *qstItem);

// misc.cpp

uint getScreenHeight(void);
uint getScreenWidth(void);
size_t numbLinesStringNeeds(const char *strn);
bool confirmChanges(void);
bool writeFiles(void);
void createPrompt(void);
void displayRecordSizeInfo(void);
const char *getOnOffStrn(const bool value);
const char *getYesNoStrn(const bool value);
const char *getYesNoStrn(const void *ptr);
const char *plural(const int numb);
const char *getVowelN(const char ch);
bool hasHelpArg(const int argc, const char *argv[]);
void checkPreArgs(const int argc, const char *argv[]);
void checkArgs(const int argc, const char *argv[]);
void renumberCurrent(void);
void renumberAll(const char *args);
void lookup(const char *args);
void where(const char *args);
void dumpTextFile(const char *filename, const bool showColor);
void displayVersionInfo(void);
void verifyZoneFlags(void);
bool isValidBoolYesNo(const char *strn);
bool getBoolYesNo(const char *strn);
const char *getYesNoBool(const bool boolean);
void toggleGuildVar(void);
void at(const char *args);
void displayLookupList(const char whichLookup, uint start);
void preDisplayLookupList(const char *args);
void displayVnumInfo(void);
char *getMoneyStrn(uint coins, char *strn);
void deleteUnusedObjMobTypes(void);
void resetEntityPointersByNumb(const bool resetObjs, const bool resetMobs);
void resetObjCond(void);
void fixGuildStuff(void);
void editRoomsOnly(void);
void editObjsOnly(void);
void editMobsOnly(void);
bool validANSIletter(const char ch);
uchar durisANSIcode(const char *strn, const size_t pos);
bool changeMaxVnum(const uint newTopVnum);
void changeMaxVnumUser(const char *args);
bool changeMaxVnumAutoEcho(const uint newTopVnum);
size_t subZeroFloor(const size_t first, const size_t second);
void deleteFile(const char *filename);

// misccomm.cpp

bool lookupExecCommand(const usint command, const char *args);
bool createExecCommand(const usint command, const char *args);
bool loadExecCommand(const usint command, const char *args);
bool editExecCommand(const usint command, const char *args);
bool listExecCommand(const usint command, const char *args);
bool createEditExecCommand(const usint command, const char *args);
bool deleteExecCommand(const usint command, const char *args);
bool purgeExecCommand(const usint command, const char *args);
bool statExecCommand(const usint command, const char *args);
bool setLimitExecCommandStartup(const usint command, const char *args);
bool setLimitExecCommand(const usint command, const char *args);
bool cloneExecCommand(const usint command, const char *args);
bool copyDescExecCommand(const usint command, const char *args);
bool copyDefaultExecCommand(const usint command, const char *args);
bool copyExecCommand(const usint command, const char *args);
void copyCommand(const char *args);

// mob.cpp

bool noMobTypesExist(void);
const char *getMobShortName(const mobType *mob);
mobType *findMob(const uint mobNumber);
bool mobExists(const uint mobNumber);
mobType *copyMobType(const mobType *srcMob, const bool incLoaded);
bool compareMobType(const mobType *mob1, const mobType *mob2);
uint getHighestMobNumber(void);
uint getLowestMobNumber(void);
uint getFirstFreeMobNumber(void);
mobType *getMatchingMob(const char *strn);
void renumberMobs(const bool renumberHead, const uint oldHeadNumb);
mobType *getPrevMob(const uint mobNumb);
mobType *getNextMob(const uint mobNumb);
bool checkDieStrnValidityShort(const char *strn);
bool checkDieStrnValidity(const char *strn, const uint mobNumb, const char *fieldName);
void deleteMobsinInv(const mobType *mobPtr);
void updateInvKeywordsMob(const mobType *mobPtr);
void resetLowHighMob(void);

// mobhere.cpp

void addMobLoad(const mobHere *mob);
void removeMobLoad(const mobHere *mob);
void loadAllinMobHereList(const mobHere *mobHead);
mobHere *findMobHere(const uint mobNumber, uint *roomNumb, const bool checkOnlyCurrentRoom);
mobHere *copyMobHere(const mobHere *srcMobHere);
mobHere *copyMobHereList(const mobHere *srcMobHere, const bool incLoaded);
void deleteMobHereList(mobHere *srcMobHere, const bool decLoaded);
void deleteMobHere(mobHere *mobHere, const bool decLoaded);
bool checkForMobHeresofType(const mobType *mobType);
void addMobHeretoList(mobHere **mobListHead, mobHere *mobToAdd, const bool incLoaded);
void deleteAllMobHereofTypeInList(mobHere **mobHereHead, const mobType *mobType, const bool decLoaded);
void deleteAllMobHeresofType(const mobType *mobType, const bool decLoaded);
void deleteMobHereinList(mobHere **mobHead, mobHere *mob, const bool decLoaded);
void removeMobHerefromList(mobHere **mobHead, mobHere *mob, const bool decLoaded);
void resetMobHere(const uint oldNumb, const uint newNumb);
bool eqSlotFull(const mobHere *mob, const uint slot);
uint eqRestrict(const mobType *mob, const uint objExtra, uint objAnti, uint objAnti2);
char handsFree(const mobHere *mob);
uint getMobHereEquipSlot(const mobHere *mob, const objectType *obj, const int where);
uint eqRaceRestrict(const mobType *mob, const objectType *obj, const uint where);
uint canMobTypeEquip(const mobType *mob, const objectType *obj, const uint where);
const char *getCanEquipErrStrn(const uint err, char *strn);
const char *getVerboseCanEquipStrn(const uint err, const char *objName, char *strn);
bool mobsCanEquipEquipped(const bool fixEq);
bool mobsEqinCorrectSlot(const bool fixEq);
bool mobEquippingSomething(const mobHere *mob);
uint getNumbFollowers(const mobHere *mob);
bool createMobHereinRoom(mobType *mob, const uint numb);
uint getZoneCommandEquipPos(const uint eqpos);

// mobtypes.cpp

const char *getMobSpeciesStrn(const char *id);
const char *getMobSpeciesNoColorStrn(const char *id);
const char *getMobSpeciesNumb(const int numb);
const char *getMobSpeciesCode(const int numb);
const char *getMobPosStrn(const uint position);
const char *getMobSexStrn(const int sex);
const char *getMobHometownStrn(const int hometown);
const char *getMobSpecStrn(const int spec);
const char *getMobSizeStrn(const int mobSize);
bool isAggro(const mobType *mob);
bool castingClass(const uint cl);
uint countClass(const uint cl);
uint classNumb(const uint cl);

// mobu.cpp

void renumberMobsUser(const char *args);

// mountfol.cpp

void mountMob(const char *args);
void unmountMob(const char *args);
void followMob(const char *args);
void unfollowMob(const char *args);

// mudcomm.cpp

const char *getCommandStrn(const uint commandValue, const bool verboseDisplay);
void displayCommandList(const bool displayAll);
void displayCommandList(const char *searchStrn);

// object.cpp

bool noObjTypesExist(void);
const char *getObjShortName(const objectType *obj);
objectType *findObj(const uint objNumber);
bool objExists(const uint objNumber);
objectType *copyObjectType(const objectType *srcObj, const bool incNumbObjects);
bool compareObjectApply(const objApplyRec *app1, const objApplyRec *app2);
bool compareObjectType(const objectType *obj1, const objectType *obj2);
uint getHighestObjNumber(void);
uint getLowestObjNumber(void);
uint getFirstFreeObjNumber(void);
void checkAndFixRefstoObj(const uint oldNumb, const uint newNumb);
void renumberObjs(const bool renumberHead, const uint oldHeadNumb);
objectType *getPrevObj(const uint objNumb);
objectType *getNextObj(const uint objNumb);
void deleteObjsinInv(const objectType *objPtr);
void updateInvKeywordsObj(const objectType *objPtr);
void resetLowHighObj(void);
objectType *getMatchingObj(const char *strn);
void showKeyUsed(const char *args);
#ifdef GODMODE
void huntForOPEquipment(const char *args);
#endif
// objectu.cpp

void renumberObjectsUser(const char *args);

// objhere.cpp

void addObjLoad(const objectHere *objHere);
void removeObjLoad(const objectHere *objHere);
void loadAllinObjHereList(const objectHere *objHead);
objectHere *checkAllObjInsideForVnum(objectHere *objHere, const uint vnum);
objectHere *findObjHere(const uint objNumber, uint *roomNumb, const bool checkOnlyCurrentRoom, 
                        const bool checkOnlyVisible);
objectHere *copyObjHere(const objectHere *srcObjHere);
objectHere *copyObjHereList(const objectHere *srcObjHere, const bool incLoaded);
bool checkEntireObjHereListForType(const objectHere *objHereHead, const objectType *objType);
bool checkForObjHeresofType(const objectType *objType);
bool scanObjHereListForLoadedContainer(const objectHere *objHere, const uint containNumb);
bool checkForObjHeresWithLoadedContainer(const uint containNumb);
void resetObjHereList(const uint oldNumb, const uint newNumb, objectHere *objHead);
void resetAllObjHere(const uint oldNumb, const uint newNumb);
void addObjHeretoList(objectHere **objListHead, objectHere *objToAdd, const bool incLoaded);
const objectHere *objinInv(const mobHere *mob, const objectType *obj);
const objectHere *objinInv(const mobHere *mob, const uint objNumb);
bool isObjTypeUsed(const uint numb);
void resetObjHEntityPointersByNumb(objectHere *objH);
void createObjectHereinRoom(objectType *obj, const uint numb);
objectHere *createObjHere(const uint numb, const bool incLoadedList);

// objtypes.cpp

const char *getObjTypeStrn(const uint objType);
const char *getObjApplyStrn(const int applyWhere);
const char *getObjCraftsmanshipStrn(const uint craft);
const char *getMaterialStrn(const int material);
const char *getLiqTypeStrn(const uint liquidType);
const char *getWeapTypeStrn(const uint weaponType);
const char *getArmorThicknessStrn(const int armorThickness);
const char *getShieldTypeStrn(const int shieldType);
const char *getShieldShapeStrn(const int shieldShape);
const char *getShieldSizeStrn(const int shieldSize);
const char *getSkillTypeStrn(const int skillType);
const char *getSpellTypeStrn(const int spellType);
const char *getInstrumentTypeStrn(const uint instType);
const char *getMissileTypeStrn(const uint missileType);
char *getTotemSphereStrn(const uint sphere, char *strn);
char *getObjTrapFlagsStrn(const uint flag, char *strn);
const char *getObjTrapDamStrn(const int type);
const char *getObjValueStrn(const uint objType, const uint valueField, const int objValue, char *strn,
                            const bool showCurrentValInfo);
bool checkForValueList(const uint objType, const uint valueField);
bool checkForVerboseAvail(const uint objType, const uint valueField);
bool checkForSearchAvail(const uint objType, const uint valueField);
void searchObjValue(const uint objType, const uint valueField);
void displayObjValueHelp(const uint objType, const uint valueField, const bool verbose);
bool fieldRefsObjNumb(const uint objType, const uint valueField);
bool fieldRefsRoomNumb(const uint objType, const uint valueField);
void specialObjValEditRedundant(objectType *obj, const uchar valueField, const flagDef *flagArr, 
                                const char *bitvName, const bool asBitV);
bool specialObjValEdit(objectType *obj, const uchar valueField, const bool editing);

// purge.cpp

void purge(const char *args);
void purgeAll(void);
void purgeAllMob(void);
void purgeAllObj(void);
void purgeInv(void);

// put.cpp

bool putEntityObj(objectHere *objH, objectHere **objHdest, const bool deleteOriginal, const bool given, 
                  const char *inStrn, size_t& numbLines);
void putEntityCmd(const char *args, const bool origDeleteOriginal);

// qsttypes.cpp

const char *getQuestItemTypeStrn(const uint itemType, const char giveRecv);

// quest.cpp

bool questExists(const uint numb);
uint getLowestQuestMobNumber(void);
uint getHighestQuestMobNumber(void);
mobType *getPrevQuestMob(const uint mobNumb);
mobType *getNextQuestMob(const uint mobNumb);
bool compareQuestMessage(const questMessage *msg1, const questMessage *msg2);
questMessage *copyQuestMessage(const questMessage *msg);
questMessage *createQuestMessage(void);
void deleteQuestMessage(questMessage *msg);
void deleteQuestMessageList(questMessage *msg);
void addQuestMessagetoList(questMessage **msgListHead, questMessage *msgToAdd);
void deleteQuestMessageinList(questMessage **msgHead, questMessage *msg);
questItem *createQuestItem(void);
void addQuestItemtoList(questItem **itemListHead, questItem *itemToAdd);
void deleteQuestItem(questItem *item);
void deleteQuestIteminList(questItem **itemHead, questItem *item);
void deleteQuestItemList(questItem *item);
questQuest *createQuestQuest(void);
void deleteQuestQuest(questQuest *qst);
void deleteQuestQuestList(questQuest *qst);
void addQuestQuesttoList(questQuest **qstListHead, questQuest *qstToAdd);
void deleteQuestQuestinList(questQuest **qstHead, questQuest *qst);
questMessage *copyQuestMessageList(const questMessage *headQstMsg);
questItem *copyQuestItemList(const questItem *headQstItem);
bool compareQuestItemList(const questItem *item1, const questItem *item2);
bool compareQuestQuest(const questQuest *qst1, const questQuest *qst2);
questQuest *copyQuestQuest(const questQuest *qstQst);
questQuest *copyQuestQuestList(const questQuest *headQstQst);
quest *copyQuest(const quest *srcQst);
quest *createQuest(void);
void deleteQuest(quest *qst, const bool madeChanges);
void deleteQuestAssocLists(quest *qst);
bool compareQuestMessageLists(const questMessage *msg1, const questMessage *msg2);
bool compareQuestQuestLists(const questQuest *qst1, const questQuest *qst2);
bool compareQuestInfo(const quest *qst1, const quest *qst2);
char *getQuestRecvString(const questItem *item, char *strn, const size_t intMaxLen);
uint getNumbMessageNodes(const questMessage *messageHead);
uint getNumbQuestNodes(const questQuest *questHead);
uint getNumbItemNodes(const questItem *itemHead);
questMessage *getMessageNodeNumb(const uint numb, questMessage *msgHead);
questQuest *getQuestNodeNumb(const uint numb, questQuest *qstHead);
questItem *getItemNodeNumb(const uint numb, questItem *itemHead);
char *getQuestItemStrn(const questItem *item, const char itemList, char *strn);
uint getNumbQuestMobs(void);
bool checkForMobWithQuest(void);
void displayQuestList(const char *args);
void tellRedund(const bool toAll, const char *whatMessage, const char *whatContain, const stringNode *desc);
void tell(const char *args);
questQuest *getQuestTellCmd(const mobType *mob, const uint numbq, const char *strn);


// readexd.cpp

extraDesc *readExtraDescFromFile(FILE *edescFile, const uint parentEntityType, const uint entityNumb, 
                                 const uint extraDescType, extraDesc **extraDescHeadPtr);

// readfile.cpp

bool readAreaFileLine(FILE *file, char *strn, const size_t intMaxLen, const uint intEntityType,
                      const uint intEntityNumb, const uint intEntityType2, 
                      const uint intEntityNumb2, const char *strnReading, 
                      const size_t intNumbArgs, const size_t *intNumbArgsArr, 
                      const bool blnEndTilde, const bool blnRemLeadingSpaces);
bool readAreaFileLineAllowEOF(FILE *file, char *strn, const size_t intMaxLen, const uint intEntityType,
                      const uint intEntityNumb, const uint intEntityType2, 
                      const uint intEntityNumb2, const char *strnReading, 
                      const size_t intNumbArgs, const size_t *intNumbArgsArr, 
                      const bool blnEndTilde, const bool blnRemLeadingSpaces, bool *hitEOF);
bool readAreaFileLineTildeLine(
                      FILE *file, char *strn, const size_t intMaxLen, const uint intEntityType,
                      const uint intEntityNumb, const uint intEntityType2, 
                      const uint intEntityNumb2, const char *strnReading, 
                      const size_t intNumbArgs, const size_t *intNumbArgsArr, 
                      const bool blnEndTilde, const bool blnRemLeadingSpaces);

// readmob.cpp

char *convertMobMoneyRedundant(char *strptr, uint *money, bool *blnError, const bool allowEndNull);
bool convertMobMoney(const char *strn, mobType *mob);
mobType *readMobFromFile(FILE *mobFile, const bool checkDupes, const bool incNumbMobs);
bool readMobFile(const char *filename);

// readobj.cpp

objectType *readObjectFromFile(FILE *objectFile, char *nextStrn, const bool checkDupes, 
                               const bool incNumbObjs);
char readObjectFile(const char *filename);

// readqst.cpp

questMessage *readQuestFileMessage(FILE *questFile, const uint questNumb);
questQuest *readQuestFileQuest(FILE *questFile, bool *endofMobQuest,
                               char *hitWhat, const uint questNumb);
quest *readQuestFromFile(FILE *questFile, uint *questNumb);
bool readQuestFile(const char *filename);

// readset.cpp

void readSettingsFile(const char *filename);

// readshp.cpp

shop *readShopFromFile(FILE *shopFile, const bool defaultShop);
bool readShopFile(const char *filename);

// readwld.cpp

roomExit *readRoomExitFromFile(FILE *worldFile, room *roomPtr, const char *exitStrn, const bool incNumbExits);
room *readRoomFromFile(FILE *worldFile, const bool checkDupes, const bool incNumbRooms);
bool readWorldFile(const char *filename);

// readzon.cpp

void addLastObjLoaded(lastObjHereLoaded **lastHead, const uint objNumb, objectHere *objPtr);
objectHere *getLastObjLoaded(lastObjHereLoaded *lastHead, const uint objNumb);
void deleteLastObjLoadedList(lastObjHereLoaded *lastHead);
void addLimitSpec(limitSpecified **specHead, const char entType, const uint entNumb, const uint limit);
void deleteLimitSpecList(limitSpecified *limitHead);
void setOverrideFromLimSpec(limitSpecified *specHead);
void removeComments(char *strn);
void loadObj(const char *strn, lastObjHereLoaded **lastObjLoadedHead,
             limitSpecified **limitSpecHead, const uint linesRead);
void loadMob(const char *strn, mobHere **lastMob, mobHere **lastMobEquippable,
             limitSpecified **limitSpecHead, const uint linesRead);
void setDoorState(const char *, const uint);
void putObjObj(const char *strn, lastObjHereLoaded **lastObjLoaded,
               limitSpecified **limitSpecHead, bool *pauseonCont,
               const uint linesRead);
void equipMobObj(const char *strn, mobHere *lastMobRead, lastObjHereLoaded **lastObjLoaded,
                 limitSpecified **limitSpecHead, const uint linesRead);
void giveMobObj(const char *strn, mobHere *lastMobRead, lastObjHereLoaded **lastObjLoaded,
                limitSpecified **limitSpecHead, const uint linesRead);
void mountMob(const char *strn, mobHere **lastMobRead, limitSpecified **limitSpecHead, 
              const uint linesRead);
void followMob(const char *strn, mobHere *lastMobRead, mobHere **lastMobEquippable,
               limitSpecified **limitSpecHead, const uint linesRead);
bool readZoneFile(const char *filename);

// room.cpp

room *findRoom(const uint roomNumber);
bool roomExists(const uint roomNumber);
void goDirection(const uint direction);
void copyAllExits(const room *srcRoom, room *destRoom, const bool blnIncNumbExits);
void deleteAllExits(room *roomPtr, const bool blnDecNumbExits);
room *copyRoomInfo(const room *srcRoom, const bool incLoaded);
bool compareRoomInfo(const room *room1, const room *room2);
void deleteRoomInfo(room *srcRoom, const bool decNumbRooms, const bool decLoaded);
uint getHighestRoomNumber(void);
uint getLowestRoomNumber(void);
uint getFirstFreeRoomNumber(void);
void checkAndFixRefstoRoom(const uint oldNumb, const uint newNumb);
void renumberRooms(const bool renumberHead, const uint oldHeadNumb);
void updateAllObjMandElists(void);
void updateAllMobMandElists(void);
bool noExitsLeadtoRoom(const uint roomNumb);
bool noExitOut(const room *room);
int roomHasAllFollowers(void);
room *getRoomKeyword(const char *key, const bool checkRooms);
uint getNumbFreeRooms(void);
room *getPrevRoom(const uint roomNumb);
room *getPrevRoom(const room *roomPtr);
room *getNextRoom(const uint roomNumb);
room *getNextRoom(const room *roomPtr);
void checkMapTrueness(void);
bool checkMapTruenessRedund(const room *roomPtr, const uint exitNumb, const int dest, size_t &lines,
                            bool *gotOne);

// roomtype.cpp

char *getRoomSectorStrn(const uint sectorType, const bool addSpaces, const bool addBrackets, char *strn);

// roomu.cpp

void gotoRoomStrn(const char *args);
void gotoRoomPrompt(void);
bool createGrid(const uint sizex, const uint sizey, const uint sizez, const char exitCreation);
void createGridInterp(const char *args);
bool linkRooms(const int fromnumb, const int tonumb, const char direction);
void linkRoomsInterp(const char *args);
void renumberRoomsUser(const char *args);

// setcomm.cpp

bool setExecCommandFile(const usint command, const char *args);

// setrand.cpp

void setEntityRandomVal(const char *args);

// shop.cpp

bool shopExists(const uint numb);
uint getLowestShopMobNumber(void);
uint getHighestShopMobNumber(void);
mobType *getPrevShopMob(const uint mobNumb);
mobType *getNextShopMob(const uint mobNumb);
shop *createShop(void);
void deleteShop(shop *shp, const bool zoneChange);
shop *copyShop(const shop *);
bool compareShopInfo(const shop *shp1, const shop *shp2);
uint getNumbShopMobs(void);
bool checkForMobWithShop(void);
bool addingShopMakesTwoShopsInRoom(const uint mobNumb);
void displayShopList(const char *args);
uint getNumbShopSold(const uint *soldArr);
uint getNumbShopBought(const uint *soldArr);
const shop *getShopinRoom(const room *roomPtr);
const shop *getShopinCurrentRoom(void);
const mobType *getShopOwner(const shop *shp);
bool listShopSold(void);
mobHere *getMobHereShop(const shop *shp);
void checkShopsOnLoad(void);
bool checkShopsOnSave(void);
void checkForDupeShopLoaded(void);
void checkForMultipleShopsInOneRoom(void);

// skills.c

extern "C" void initialize_skills(void);

// stat.cpp

void statEntity(const char *args);

// strings.cpp

//char *deleteChar(char *strn, char *strnWork);  // not declared because 0 resolves to either pointer or int
char *deleteChar(char *strn, const size_t strnPos);
char *insertChar(char *strn, const size_t strnPos, const char ch);
char *remLeadingSpaces(char *strn);
char *remTrailingSpaces(char *strn);
char *remSpacesBetweenArgs(char *strn);
char *nolf(char *strn);
size_t numbArgs(const char *strn);
char *getArg(const char *strn, const size_t argNumb, char *arg, const size_t intMaxLen, size_t *intCopied);
char *getArg(const char *strn, const size_t argNumb, char *arg, const size_t intMaxLen);
char *upstrn(char *strn);
char *upfirstarg(char *strn);
char *lowstrn(char *strn);
bool strnumer(const char *strn);
bool strnumerneg(const char *strn);
bool strfloat(const char *strn);
bool strcmpnocase(const char *strn1, const char *strn2);
bool strcmpnocasecount(const char *strn1, const char *strn2, const size_t intCount);
bool strstrnocase(const char *strn, const char *substrn);
bool strleft(const char *strn, const char *substrn);
bool strlefti(const char *strn, const char *substrn);
bool strlefticount(const char *strn, const char *substrn, const size_t intCount);
bool strright(const char *strn, const char *substrn);
bool strrighti(const char *strn, const char *substrn);
char *strnSect(const char *strn, char *newStrn, const size_t start, size_t end);
size_t numbPercentS(const char *strn);
bool lastCharLF(const char *strn);
size_t numbLinefeeds(const char *strn);
size_t truestrlen(const char *strn);
void CAP(char *strn);
void DECAP(char *strn);
bool bleeding_ansi( const char *strn);

// strnnode.cpp

uint getNumbStringNodes(const stringNode *strnHead);
void fixStringNodes(stringNode **strnHead, const bool remStartLines);
stringNode *readStringNodes(FILE *inFile, const char EOSN, const bool remStartLines);
void writeStringNodes(FILE *outFile, const stringNode *strnNode);
void deleteStringNodes(stringNode *strnNode);
void deleteMatchingStringNodes(stringNode **strnHead, const char *strn);
stringNode *copyStringNodes(const stringNode *strnNode);
bool compareStringNodes(const stringNode *list1, const stringNode *list2);
bool compareStringNodesIgnoreCase(const stringNode *list1, const stringNode *list2);
stringNode *editStringNodes(stringNode *strnNode, const bool remStartLines);

// take.cpp

void takeEntityFromEntity(const char *objStrn, const bool deleteOriginal, const char *containerStrn);
void takeEntityCmd(const char *args, const bool deleteOriginal);
bool takeEntityRoomEdesc(extraDesc *desc, const bool deleteOriginal);
bool takeEntityObjEdesc(objectType *obj, extraDesc *desc, const bool deleteOriginal);
bool takeEntityObj(objectHere *objH, objectHere **objHsrc, const bool deleteOriginal, const char *fromStrn,
                   size_t &numbLines);
bool takeEntityMob(mobHere *mobH, const bool deleteOriginal, size_t &numbLines);
bool takeAllObj(objectHere **objHereHead, const bool deleteOriginal, const char *fromStrn, size_t &numbLines);
bool takeAllMob(mobHere **mobHereHead, const bool deleteOriginal, size_t &numbLines);

// template.cpp

bool displayTemplateRedundant(const uint templateNumb, const char *flagname, const uint templateVal,
                              size_t& numblines, bool *foundTemplate);
void setTemplateArgs(const char *args, const bool updateChanges, const bool displayStuff);

// unequip.cpp

void unequipMobSpecific(mobHere *mobH, const char *objStrn);
void unequipMob(const char *strn);

// unixgraph.cpp

#ifdef __UNIX__
void exitUnixCurses(void);
void initUnixCursesScreen(void);
void clrline(const sint line, const uchar fg, const uchar bg, const uchar ch);
void clrline(const sint line, const uchar fg, const uchar bg);
void clrline(const sint line);
usint getkey(void);
void _settextcolor(const uchar col);
void _setbkcolor(const uchar col);
uchar _gettextcolor(void);
uchar _getbkcolor(void);
void _settextposition(const int y, const int x);
struct rccoord _gettextposition(void);
void _outtext(const char *strn);
void clrscr(const uchar fg, const uchar bg);
#endif

// var.cpp

bool displayVarSettings(const variable *var, const char *preStrn, size_t& numbLines);
void unvarCmd(const char *args, variable **varHead);
char addVar(variable **varHead, const char *origVarStrn, const char *instrn);
void setVarArgs(variable **varHead, const char *args, const bool addLFs, const bool updateChanges,
                const bool display);
void varCmd(const char *args, const bool addLFs, variable **varHead);
const variable *getVar(const variable *varHead, const char *varName);
int getVarNumb(const variable *varHead, const char *varName, const int defVal);
uint getVarNumbUnsigned(const variable *varHead, const char *varName, const uint defVal);
const char *getVarStrn(const variable *varHead, const char *varName, const char *defVal);
const char *getVarStrn(const variable *varHead, const char *varName, char *strn, const char *defVal, 
                       const size_t maxLen);
size_t getVarStrnLen(const variable *varHead, const char *varName);
bool getVarBoolVal(const variable *varHead, const char *varName, const bool defVal);
void setVarBoolVal(variable **varHead, const char *varName, const bool boolVal,
                   const bool updateChanges);
bool varExists(const variable *varHead, const char *varName);
bool strIsBoolVal(const char *strn);
bool isVarBoolean(const variable *varHead, const char *varName);
bool isVarBoolean(const variable *var);
void toggleVar(variable **varHead, const char *args);
variable *copyVarList(const variable *varHead);
void deleteVarList(variable *varHead);
bool compareVarLists(const variable *varH1, const variable *varH2);

// vardef.cpp

uint getScreenHeightVal(void);
uint getScreenWidthVal(void);
uint getSaveHowOftenVal(void);
const char *getEditorName(void);
bool getIsMapZoneVal(void);
bool getVnumCheckVal(void);
bool getObjAffectVal(void);
bool getCheckLimitsVal(void);
bool getCheckZoneFlagsVal(void);
bool getShowObjContentsVal(void);
bool getShowObjFlagsVal(void);
bool getShowMobFlagsVal(void);
bool getShowMobPosVal(void);
bool getShowObjVnumVal(void);
bool getShowMobVnumVal(void);
bool getShowMobRideFollowVal(void);
bool getShowRoomExtraVal(void);
bool getShowRoomVnumVal(void);
bool getShowPricesAdjustedVal(void);
bool getInterpColorVal(void);
bool getShowColorVal(void);
const char *getMenuPromptName(void);
const char *getMainPromptStrn(void);
bool getWalkCreateVal(void);
bool getShowExitDestVal(void);
bool getShowExitFlagsVal(void);
uint getRoomStartVal(void);
bool getStartRoomActiveVal(void);
bool getShowMenuInfoVal(void);
const char *getMainZoneNameStrn(void);
const char *getLastCommandStrn(void);
bool getCheckSaveVal(void);
bool getCheckLoneRoomVal(void);
bool getCheckMissingKeysVal(void);
bool getCheckLoadedVal(void);
bool getCheckRoomVal(void);
bool getCheckExitVal(void);
bool getCheckExitDescVal(void);
bool getCheckObjVal(void);
bool getCheckObjValuesVal(void);
bool getCheckObjDescVal(void);
bool getCheckMobDescVal(void);
bool getCheckMobVal(void);
bool getCheckZoneVal(void);
bool getCheckEdescVal(void);
bool getNoSaveonCheckErrVal(void);
bool getPauseCheckScreenfulVal(void);
bool getCheckFlagsVal(void);
bool getSaveCheckLogVal(void);
bool getEditUneditableFlagsVal(void);
bool getAllMobsVal(void);
bool getIgnoreZoneSVal(void);
bool getCheckMobClassEqVal(void);
bool getCheckMobRaceEqVal(void);
bool getCheckMobSexEqVal(void);
bool getCheckObjMaterialVal(void);
bool getCheckGuildStuffVal(void);
bool getNegDestOutofZoneVal(void);
bool getSaveEveryXCommandsVal(void);

// win32.cpp

#ifdef _WIN32
bool checkWindowsVer(void);
bool setupWindowsConsole(void);
bool setWin32ExitFunc(void);
WORD getForegroundValue(const uchar fg);
WORD getBackgroundValue(const uchar fg);
void _settextposition(const sint y, const sint x);
struct rccoord _gettextposition(void);
void _outtext(const char *strn);
usint getkey(void);
void _settextcolor(const uchar col);
void _setbkcolor(const uchar col);
uchar _gettextcolor(void);
uchar _getbkcolor(void);
void clrscr(const uchar fg, const uchar bg);
void clrline(const sint line, const uchar fg, const uchar bg, const uchar ch);
void clrline(const sint line, const uchar fg, const uchar bg);
void clrline(const sint line);
bool handleNoArgs(char *filenameStrn, bool *newZone);
#endif

// writeexd.cpp

void writeExtraDesctoFile(FILE *edescFile, const extraDesc *extraDescNode);

// writemob.cpp

void writeMobtoFile(FILE *mobFile, const mobType *mob);
void writeMobFile(const char *filename);

// writeobj.cpp

void writeObjecttoFile(FILE *objectFile, const objectType *obj);
void writeObjectFile(const char *filename);

// writeqst.cpp

void writeQuesttoFile(FILE *questFile, const quest *qst, const uint mobNumb);
void writeQuestFile(const char *filename);

// writeset.cpp

void writeSettingsFile(const char *filename, const bool writeLimits);
void writeTemplateRedundant(FILE *setFile, const char *tempName, const uint tempNumb, const uint tempVal);

// writeshp.cpp

void writeShoptoFile(FILE *shopFile, shop *shopRec, const uint mobNumb, const bool defaultShop);
void writeShopFile(const char *filename);

// writewld.cpp

void writeWorldExittoFile(FILE *worldFile, const roomExit *exitNode, const char *exitStrn);
void writeRoomtoFile(FILE *worldFile, const room *room);
void writeWorldFile(char *filename);

// writezon.cpp

void addExitWrittentoList(exitWritten **exitHead, exitWritten *exittoAdd);
void deleteExitWrittenList(exitWritten *exitHead);
exitWritten *checkExitWrittenListforExit(exitWritten *exitHead, const int roomNumb, const char exitNumb);
roomExit *checkForPairedExit(const roomExit *exitNode, const int srcRoomNumb, char *exitDir);
bool writeAllExitsPairedRedundant(FILE *zoneFile, const room *room, exitWritten **exitWHead,
                                  roomExit *exitNode, const char *exitName, const char exitChar,
                                  const char origDir);
void writeAllExitsPaired(FILE *zoneFile);
void writeAllObjInsideSpecific(FILE *zoneFile, const objectHere *insidePtr,
                               const uint containerNumb, const uint extraSpaces,
                               const bool writeConts);
void writeAllObjInside(FILE *zoneFile, const objectHere *insidePtr,
                       const uint containerNumb, const uint extraSpaces);
void writeLoadObjectCmd(const room *room, const objectHere *objHere, FILE *zoneFile);
void writeLoadMobCommand(const room *room, const mobHere *mobHNode, FILE *zoneFile);
void writeEquipMobCommand(const objectHere *mobEquip, FILE *zoneFile, const uint origeqpos);
void writeGiveMobCommand(const objectHere *mobObj, FILE *zoneFile);
void writeRideMobCommand(const room *room, const mobHere *mobHNode, FILE *zoneFile);
void writeMobFollowerCommands(const mobHere *mobLeader, FILE *zoneFile);
void writeZoneFile(const char *filename);

// zone.cpp

void setZoneNumb(const uint numb, const bool dispText);
void setZoneNumbStrn(const char *args);
void setZoneNumbPrompt(void);

// zonetype.cpp

const char *getZoneResetStrn(const uint resetMode);

#define _FH_H_
#endif
