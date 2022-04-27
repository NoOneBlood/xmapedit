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

#include "common_game.h"
#include "eventq.h"
#include "db.h"

#pragma once

#define kMaxRandomizeRetries 16
#define kMaxSuperXSprites 128

// *modern types only hitag*
#define kModernTypeFlag0 0x0000
#define kModernTypeFlag1 0x0001
#define kModernTypeFlag2 0x0002
#define kModernTypeFlag3 0x0003
#define kModernTypeFlag4 0x0004

extern BOOL gEventRedirectsUsed;
extern short gProxySpritesList[kMaxSuperXSprites];
extern short gSightSpritesList[kMaxSuperXSprites];
extern short gProxySpritesCount;
extern short gSightSpritesCount;


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
kModernStealthRegion				= 16,
kModernCustomDudeSpawn              = 24,
kModernRandomTX                     = 25,
kModernSequentialTX                 = 26,
kModernSeqSpawner                   = 27,
kModernObjPropChanger         		= 28,
kModernObjPicChanger             	= 29,
kModernObjSizeChanger               = 31,
kModernDudeTargetChanger            = 33,
kModernSectFXChg	             	= 34,
kModernObjDataChanger               = 35,
kModernSpriteDamager                = 36,
kModernObjDataAccum          		= 37,
kModernEffectSpawner                = 38,
kModernWindGenerator                = 39,
kModernRandom                       = 40,
kModernRandom2                      = 80,
kDudeModernCustom                   = 254,
kDudeModernCustomBurning            = 255,
kGenModernMissileUniversal          = 704,
kModernPlayerControl                = 500, /// WIP
kModernCondition                    = 501, /// WIP, sends command only if specified conditions == true
kModernConditionFalse               = 502, /// WIP, sends command only if specified conditions != true
kGenModernSound						= kGenSound,
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

BOOL modernTypeOperateSprite(int nSprite, spritetype* pSprite, XSPRITE* pXSprite, EVENT event);
BOOL modernTypeOperateWall(int nWall, walltype* pWall, XWALL* pXWall, EVENT event);
BOOL modernTypeSetSpriteState(int nSprite, XSPRITE* pXSprite, int nState);
void modernTypeSendCommand(int nSprite, int destChannel, COMMAND_ID command);
void modernTypeTrigger(int destObjType, int destObjIndex, EVENT event);
void nnExtResetGlobals();
void nnExtInitModernStuff();
void nnExtProcessSuperSprites();

void useRandomItemGen(spritetype* pSource, XSPRITE* pXSource);
void useRandomTx(spritetype* pSource, COMMAND_ID cmd, BOOL setState);
void useSoundGen(XSPRITE* pXSource, spritetype* pSprite);
void useSequentialTx(spritetype* pSource, COMMAND_ID cmd, BOOL setState);
void useSectorWindGen(spritetype* pSource, sectortype* pSector);
void useSectorLigthChanger(spritetype* pSource, XSECTOR* pXSector);
void usePictureChanger(spritetype* pSource, int objType, int objIndex);
void useDataChanger(spritetype* pSource, int objType, int objIndex);
void useIncDecGen(spritetype* pSource, int objType, int objIndex);
void useObjResizer(spritetype* pSource, int objType, int objIndex);
void usePropertiesChanger(spritetype* pSource, int objType, int objIndex);
void useTeleportTarget(spritetype* pSource, spritetype* pSprite);
void useSeqSpawnerGen(spritetype* pSource, int objType, int index);
void useEffectGen(spritetype* pSource, spritetype* pSprite);
void useUniMissileGen(spritetype* pSprite);
void useSpriteDamager(spritetype* pSource, int objType, int objIndex);


int getDataFieldOfObject(int objType, int objIndex, int dataIndex);
BOOL setDataValueOfObject(int objType, int objIndex, int dataIndex, int value);
BOOL incDecGoalValueIsReached(spritetype* pSprite);
void seqTxSendCmdAll(spritetype* pSource, int nIndex, COMMAND_ID cmd, BOOL modernSend);
spritetype* cdGetNextIncarnation(spritetype* pSprite);
void cdTransform(spritetype* pSprite);
spritetype* cdSpawn(spritetype* pSprite, int nDist);
void cdGetSeq(spritetype* pSprite);
void callbackUniMissileBurst(int nSprite);
void callbackMakeMissileBlocking(int nSprite);
int GetDataVal(spritetype* pSprite, int data);
spritetype* evrIsRedirector(int nSprite);
spritetype* evrListRedirectors(int objType, int objXIndex, spritetype* pRedir, int* tx);
int listTx(spritetype* pRedir, int tx);
void seqSpawnerOffSameTx(spritetype* pSource);
void windGenStopWindOnSectors(spritetype* pSource);
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

