/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A lite version of nnexts.cpp adapted for xmapedit's preview mode feature.
// nnexts.cpp provides extended features for mappers
// more info at http://cruo.bloodgame.ru/xxsystem
//
// This file is part of XMAPEDIT.
//
// XMAPEDIT is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License version 2
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
////////////////////////////////////////////////////////////////////////////////////
***********************************************************************************/
#pragma once
#include "common_game.h"
#include "eventq.h"
#include "db.h"

#include "nnextslaser.h"


#define kSlopeDist 0x20
#define kPercFull 100

#define kMaxRandomizeRetries 16
#define kMaxSuperXSprites 512

// *modern types only hitag*
#define kModernTypeFlag0 0x0000
#define kModernTypeFlag1 0x0001
#define kModernTypeFlag2 0x0002
#define kModernTypeFlag3 0x0003
#define kModernTypeFlag4 0x0004
#define kModernTypeFlag8 0x0008
#define kModernTypeFlag16 0x0010
#define kModernTypeFlag64 0x0040

#define kTriggerSpriteScreen 0x0001
#define kTriggerSpriteAim    0x0002

// additional physics attributes for debris sprites
#define kPhysDebrisFloat 0x0008 // *debris* slowly goes up and down from it's position
#define kPhysDebrisFly 0x0010 // *debris* affected by negative gravity (fly instead of falling)
#define kPhysDebrisSwim 0x0020 // *debris* can swim underwater (instead of drowning)
#define kPhysDebrisTouch 0x0040 // *debris* can be moved via touch
//#define kPhysDebrisPush 0x0080 // *debris* can be moved via push
#define kPhysDebrisVector 0x0400 // *debris* can be affected by vector weapons
#define kPhysDebrisExplode 0x0800 // *debris* can be affected by explosions

#define kDudeFlagStealth    0x0001
#define kDudeFlagCrouch     0x0002

#define kCdudeFileNamePrefix        "CDUD"
#define kCdudeFileNamePrefixWild    "CDUD*"
#define kCdudeFileExt               "CDU"

#define kCitemFileName				"ITEMS"
#define kCitemFileExt				"ITM"

extern BOOL gEventRedirectsUsed;



extern IDLIST gProxySpritesList;
extern IDLIST gSightSpritesList;
extern IDLIST gImpactSpritesList;
extern IDLIST gPhysSpritesList;

// modern statnums
enum {
kStatModernBase                     = 20,
kStatModernDudeTargetChanger        = kStatModernBase,
kStatModernCondition                = 21,
kStatModernEventRedirector          = 22,
kStatModernPlayerLinker             = 23,
kStatModernBrokenDudeLeech          = 24,
kStatModernQavScene                 = 25,
kStatModernWindGen                  = 26,
kStatModernStealthRegion            = 27,
kStatModernTmp                      = 39,
kStatModernMax                      = 40,
};

// modern sprite types
enum {
kModernStealthRegion                = 16,
kModernCustomDudeSpawn              = 24,
kModernRandomTX                     = 25,
kModernSequentialTX                 = 26,
kModernSeqSpawner                   = 27,
kModernObjPropertiesChanger         = 28,
kModernObjPicnumChanger             = 29,
kModernObjSizeChanger               = 31,
kModernDudeTargetChanger            = 33,
kModernSectorFXChanger              = 34,
kModernObjDataChanger               = 35,
kModernSpriteDamager                = 36,
kModernObjDataAccumulator           = 37,
kModernEffectSpawner                = 38,
kModernWindGenerator                = 39,
kModernRandom                       = 40,
kModernRandom2                      = 80,
kItemModernMapLevel                 = 150,  // once picked up, draws whole minimap
kDudeModernCustom                   = kDudeVanillaMax,
kDudeModernCustomBurning            = 255,
kModernThingTNTProx                 = 433, // detects only players
kModernThingThrowableRock           = 434, // does small damage if hits target
kModernThingEnemyLifeLeech          = 435, // the same as normal, except it aims in specified target only
kModernPlayerControl                = 500, /// WIP
kModernCondition                    = 501, /// WIP, sends command only if specified conditions == true
kModernConditionFalse               = 502, /// WIP, sends command only if specified conditions != true
kModernSlopeChanger                 = 504,
kModernLaserGen						= 505,
kModernVelocityChanger              = 506,
kGenModernMissileUniversal          = 704,
kGenModernSound                     = 708,
};

// type of object
enum {
EVOBJ_WALL                          = 0,
EVOBJ_SPRITE                        = 3,
EVOBJ_SECTOR                        = 6,
};

// type of random
enum {
kRandomizeItem                      = 0,
kRandomizeDude                      = 1,
kRandomizeTX                        = 2,
};

struct SPRITEMASS
{
    int seqId;
    short picnum; // mainly needs for moving debris
    short xrepeat;
    short yrepeat;
    short clipdist; // mass multiplier
    int mass;
    short airVel; // mainly needs for moving debris
    int fraction; // mainly needs for moving debris
};

struct EXTERNAL_FILES_LIST
{
    const char* name;
    const char* ext;
};

extern SPRITEMASS gSpriteMass[kMaxXSprites];
extern EXTERNAL_FILES_LIST gExternFiles[2];
extern char gCustomDudeNames[kMaxSprites][32];


inline BOOL xspriRangeIsFine(int nXindex) {
    return (nXindex > 0 && nXindex < kMaxXSprites);
}

inline BOOL xsectRangeIsFine(int nXindex) {
    return (nXindex > 0 && nXindex < kMaxXSectors);
}

inline BOOL xwallRangeIsFine(int nXindex) {
    return (nXindex > 0 && nXindex < kMaxXWalls);
}

inline BOOL valueIsBetween(int val, int min, int max) {
    return (val > min && val < max);
}

inline BOOL spriRangeIsFine(int nIndex) {
    return (nIndex >= 0 && nIndex < kMaxSprites);
}

inline BOOL sectRangeIsFine(int nIndex) {
    return (nIndex >= 0 && nIndex < kMaxSectors);
}

inline BOOL wallRangeIsFine(int nIndex) {
    return (nIndex >= 0 && nIndex < kMaxWalls);
}

bool modernTypeOperateSprite(int nSprite, spritetype* pSprite, XSPRITE* pXSprite, EVENT event);
BOOL modernTypeOperateWall(int nWall, walltype* pWall, XWALL* pXWall, EVENT event);
BOOL modernTypeSetSpriteState(int nSprite, XSPRITE* pXSprite, int nState);
void modernTypeSendCommand(int nSprite, int destChannel, COMMAND_ID command);
void modernTypeTrigger(int destObjType, int destObjIndex, EVENT event);
void nnExtResetGlobals();
void nnExtInitModernStuff();
void nnExtProcessSuperSprites();

void useRandomTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState);
void useSequentialTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState);
void useSpriteDamager(XSPRITE* pXSource, int objType, int objIndex);
void useVelocityChanger(XSPRITE* pXSource, int causerID, short objType, int objIndex);
void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector);
void useSoundGen(XSPRITE* pXSource, spritetype* pSprite);
void useUniMissileGen(XSPRITE* pXSource, spritetype* pSprite);
void usePropertiesChanger(XSPRITE* pXSource, short objType, int objIndex);
void usePictureChanger(XSPRITE* pXSource, int objType, int objIndex);
void useSectorLigthChanger(XSPRITE* pXSource, XSECTOR* pXSector);
void useDataChanger(XSPRITE* pXSource, int objType, int objIndex);
void useIncDecGen(XSPRITE* pXSource, short objType, int objIndex);
void useObjResizer(XSPRITE* pXSource, short objType, int objIndex);
void useSlopeChanger(XSPRITE* pXSource, int objType, int objIndex);
void useEffectGen(XSPRITE* pXSource, spritetype* pSprite);
void useSeqSpawnerGen(XSPRITE* pXSource, int objType, int index);
void useTeleportTarget(XSPRITE* pXSource, spritetype* pSprite);
void useRandomItemGen(spritetype* pSource, XSPRITE* pXSource);
void useCustomDudeSpawn(XSPRITE* pXSource, spritetype* pSprite);
void useDudeSpawn(XSPRITE* pXSource, spritetype* pSprite);


bool incDecGoalValueIsReached(XSPRITE* pXSprite);
void seqTxSendCmdAll(XSPRITE* pXSource, int nIndex, COMMAND_ID cmd, bool modernSend);
void windGenStopWindOnSectors(XSPRITE* pXSource);
void seqSpawnerOffSameTx(XSPRITE* pXSource);
spritetype* randomSpawnDude(XSPRITE* pXSource, spritetype* pSprite, int a3, int a4);
spritetype* nnExtSpawnDude(XSPRITE* pXSource, spritetype* pSprite, short nType, int a3, int a4);


int getDataFieldOfObject(int objType, int objIndex, int dataIndex);
BOOL setDataValueOfObject(int objType, int objIndex, int dataIndex, int value);
spritetype* cdGetNextIncarnation(spritetype* pSprite);
void cdTransform(spritetype* pSprite);
spritetype* cdSpawn(XSPRITE* pXSource, spritetype* pSprite, int nDist);
void cdGetSeq(spritetype* pSprite);
void callbackUniMissileBurst(int nSprite);
void callbackMakeMissileBlocking(int nSprite);
int GetDataVal(spritetype* pSprite, int data);
void damageSprites(spritetype* pSource, spritetype* pSprite);
spritetype* randomSpawnDude(spritetype* pSource);
int randomGetDataValue(XSPRITE* pXSprite, int randType);
int nnExtRandom(int a, int b);

bool modernTypeOperateSector(int nSector, sectortype* pSector, XSECTOR* pXSector, EVENT event);
int sectorInMotion(int nSector);
void sectorKillSounds(int nSector);
void sectorContinueMotion(int nSector, EVENT event);
void sectorPauseMotion(int nSector);
spritetype* randomDropPickupObject(spritetype* pSource, short prevItem);



void debrisConcuss(int nOwner, int nDebris, int x, int y, int z, int dmg);
void debrisBubble(int nSprite);
void debrisMove(int listIndex);

bool xsprIsFine(spritetype* pSpr);
int getVelocityAngle(spritetype* pSpr);
void changeSpriteAngle(spritetype* pSpr, int nAng);

XSPRITE* evrListRedirectors(int objType, int objXIndex, XSPRITE* pXRedir, int* tx);
XSPRITE* evrIsRedirector(int nSprite);
int listTx(XSPRITE* pXRedir, int tx);
void nnExtTriggerObject(int objType, int objIndex, int command);
int getSpriteMassBySize(spritetype* pSprite);

int nnExtResAddExternalFiles(Resource* pIn, const char* pPath, EXTERNAL_FILES_LIST* pList, int nLen);
DICTNODE* nnExtResFileSearch(Resource* pIn, const char* pName, const char* pExt, char external = true);


void lasersInit();
void lasersProcess();
LASER* laserGet(spritetype* pSpr);
void lasersProcessView3D(int camx, int camy, int camz, int cama, int cams, int camh);

int userItemsInit();