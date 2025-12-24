/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A lite version of nnextsif adapted for xmapedit's preview mode feature.
// This file provides functionality for kModernCondition types which is part
// of nnexts.cpp. More info at http://cruo.bloodgame.ru/xxsystem
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
#include "tile.h"
#include "seq.h"
#include "nnextsif.h"
#include "preview.h"
#include "xmpmaped.h"


#define kMaxConditions              600
#define kSerialStep                 100000
typedef char (*CONDITION_FUNC)( void );
typedef int (*SEQ_CMP_FUNC)( int, int );

enum {
kSerialSector                       = kSerialStep   + kMaxSectors + kMaxWalls + kMaxSprites,
kSerialWall                         = kSerialSector + kSerialStep,
kSerialSprite                       = kSerialWall   + kSerialStep,
kSerialMax                          = kSerialSprite + kSerialStep,
};

enum {
kErrInvalidObject                   = 0,
kErrInvalidArg2                     = 1,
kErrInvalidSerial                   = 2,
kErrUnknownObject                   = 3,
kErrInvalidArgsPass                 = 4,
kErrObjectUnsupp                    = 5,
kErrNotImplementedCond              = 6,
kErrNotImplementedCond2             = 7,
};

enum {
CNON                                = 0,
CGAM                                = 1,
CMIX                                = 2,
CWAL                                = 3,
CSEC                                = 4,
CPLY                                = 5,
CDUD                                = 6,
CSPR                                = 7,
};

#pragma pack(push, 1)
struct CHECKFUNC_INFO
{
    CONDITION_FUNC pFunc;
    unsigned int type           : 4;
    char* name;
};

struct CONDITION_INFO
{
    CONDITION_FUNC pFunc;
    unsigned int id             : 10;
    unsigned int type           : 4;
    //unsigned int isBool       : 1;
    //unsigned int xReq         : 1;
    //char* name;
    CHECKFUNC_INFO* pCaller;    // normally must be set during init
};

struct TRACKING_CONDITION
{
    unsigned int id             : 16;
    OBJECT_LIST* objects;
};

struct STATUSBIT1
{
    unsigned int ok             : 1;
};
#pragma pack(pop)

static char* gErrors[] =
{
    "Object #%d (objType: %d) is not a %s!.",
    "Argument #1 (%d) must be less than argument #2 (%d).",
    "%d is not condition serial!",
    "Unknown object type %d, index %d.",
    "Invalid arguments passed.",
    "Unsupported %s type %d, extra: %d.",
    "Condition is not implemented.",
    "Condition #%d is not implemented.",
};

static STATUSBIT1 errShown[kMaxSprites];

/** TRACKING(LOOPED) coditions list
********************************************************************************/
static TRACKING_CONDITION* gTrackingConditionsList = NULL;
static unsigned int gTrackingConditionsListLength = 0;


/** Variables that is relative to current condition
********************************************************************************/
static spritetype* pCond = NULL; static XSPRITE* pXCond = NULL;     // current condition
static int arg1 = 0, arg2 = 0, arg3 = 0;                            // arguments of current condition (data2, data3, data4)
static int cmpOp = 0;                                               // current comparison operator (condition sprite cstat)
static char PUSH = false;                                           // current stack push status (kCmdNumberic)
static CONDITION_INFO* pEntry = NULL;                               // current condition db entry
static EVENT* pEvent = NULL;                                        // current event

static int objType = 0,     objIndex = 0;                           // object in the focus
static spritetype* pSpr  = NULL;    static XSPRITE* pXSpr  = NULL;  // (x)sprite in the focus
static walltype* pWall   = NULL;    static XWALL* pXWall   = NULL;  // (x)wall in the focus
static sectortype* pSect = NULL;    static XSECTOR* pXSect = NULL;  // (x)sector in the focus
static char xAvail = false;                                         // x-object indicator


/** INTERFACE functions
********************************************************************************/
static void Unserialize(int nSerial, int* oType, int* oIndex);
static int  Serialize(int oType, int oIndex);
static void Push(int oType, int oIndex);
static void TriggerObject(int nSerial);
static void Error(char* pFormat, ...);
static char Cmp(int val);
static void Restore();

static char CheckGeneric();
static char CheckSector();
static char CheckSprite();
static char CheckObject();
static char CheckWall();
static char CheckDude();


/** ERROR functions
********************************************************************************/
static char errCondUnknown(void)
{
    Error(gErrors[kErrNotImplementedCond]);
    return false;
}

static char errCondNotImplemented(void)
{
    if (!errShown[pCond->index].ok)
    {
        scrSetLogMessage(gErrors[kErrNotImplementedCond2], pXCond->data1);
        errShown[pCond->index].ok = 1;
    }
    return false;
}

/** A list of caller functions
********************************************************************************/
static CHECKFUNC_INFO gCheckFuncInfo[] =
{
    { CheckGeneric,             CGAM,       "Game"   },
    { CheckObject,              CMIX,       "Mixed"  },
    { CheckWall,                CWAL,       "Wall"   },
    { CheckSector,              CSEC,       "Sector" },
    { CheckGeneric,             CPLY,       "Player" },
    { CheckDude,                CDUD,       "Dude"   },
    { CheckSprite,              CSPR,       "Sprite" },
    { CheckGeneric,             CNON,       ""       },
};

static int qsSortCheckFuncInfo(CHECKFUNC_INFO *ref1, CHECKFUNC_INFO *ref2)
{
    return ref1->type - ref2->type;
}


/** GAME conditions
********************************************************************************/
static char gamCmpLevelMin(void)        { return Cmp(gPreview.levelTime / (kTicsPerSec * 60)); }
static char gamCmpLevelSec(void)        { return Cmp((gPreview.levelTime / kTicsPerSec) % 60); }
static char gamCmpLevelMsec(void)       { return Cmp(((gPreview.levelTime % kTicsPerSec) * 33) / 10); }
static char gamCmpLevelTime(void)       { return Cmp(gPreview.levelTime); }
static char gamCmpKillsTotal(void)      { return Cmp(gPreview.kills.total); }
static char gamCmpKillsDone(void)       { return Cmp(gPreview.kills.done); }
static char gamCmpSecretsTotal(void)    { return Cmp(gPreview.secrets.total); }
static char gamCmpSecretsDone(void)     { return Cmp(gPreview.secrets.done); }
static char gamCmpVisibility(void)      { return Cmp(visibility); }
static char gamChkChance(void)          { return Chance((0x10000 * arg3) / kPercFull); }
static char gamCmpRandom(void)          { return Cmp(nnExtRandom(arg1, arg2)); }
static char gamCmpStatnumCount(void)    { return Cmp(gStatCount[ClipRange(arg3, 0, kMaxStatus)]); }
static char gamCmpNumsprites(void)      { return Cmp(numsprites); }


/** SECTOR conditions
********************************************************************************/
static char sectCmpVisibility(void)     { return Cmp(pSect->visibility); }
static char sectCmpFloorSlope(void)     { return Cmp(pSect->floorheinum); }
static char sectCmpCeilSlope(void)      { return Cmp(pSect->ceilingheinum); }
static char sectChkSprTypeInSect(void)
{
    int i;
    for (i = headspritesect[objIndex]; i >= 0; i = nextspritesect[i])
    {
        if (!Cmp(sprite[i].type)) continue;
        else if (PUSH) Push(EVOBJ_SPRITE, i);
        return true;
    }
    return false;
}

static char sectHelperCmpHeight(char ceil)
{
    int nHeigh1, nHeigh2;
    switch (pSect->type)
    {
        case kSectorZMotion:
        case kSectorRotate:
        case kSectorSlide:
        case kSectorSlideMarked:
        case kSectorRotateMarked:
        case kSectorRotateStep:
            if (xAvail)
            {
                if (ceil)
                {
                    nHeigh1 = ClipLow(abs(pXSect->onCeilZ - pXSect->offCeilZ), 1);
                    nHeigh2 = abs(pSect->ceilingz - pXSect->offCeilZ);
                }
                else
                {
                    nHeigh1 = ClipLow(abs(pXSect->onFloorZ - pXSect->offFloorZ), 1);
                    nHeigh2 = abs(pSect->floorz - pXSect->offFloorZ);
                }

                return Cmp((kPercFull * nHeigh2) / nHeigh1);
            }
            fallthrough__;
        default:
            Error(gErrors[kErrObjectUnsupp], gSearchStatNames[OBJ_SECTOR], pSect->type, pSect->extra);
            return false;
    }
}
static char sectCmpFloorHeight(void)    { return sectHelperCmpHeight(false); }
static char sectCmpCeilHeight(void)     { return sectHelperCmpHeight(true);  }
static char sectChkUnderwater(void)     { return (xAvail) ? pXSect->underwater : false; }
static char sectChkPaused(void)         { return (xAvail) ? !pXSect->unused1   : false; }
static char sectCmpDepth(void)          { return Cmp((xAvail) ? pXSect->Depth : 0); }


/** WALL conditions
********************************************************************************/
static char wallCmpOverpicnum(void)     { return Cmp(pWall->overpicnum); }
static char wallChkIsMirror(void)
{
    int i = mirrorcnt;
    while(--i >= 0)
    {
        if (mirror[i].type == 0 && mirror[i].id == objIndex)
            return true;
    }

    return false;
}

static char wallHelperChkSector(int nSect)
{
    if (!sectRangeIsFine(nSect)) return false;
    else if (PUSH) Push(EVOBJ_SECTOR, nSect);
    return true;
}

static char wallHelperChkWall(int nWall)
{
    if (!wallRangeIsFine(nWall)) return false;
    else if (PUSH) Push(EVOBJ_WALL, nWall);
    return true;
}
static char wallChkSector(void)         { return wallHelperChkSector(sectorofwall(objIndex)); }
static char wallChkNextSector(void)     { return wallHelperChkSector(pWall->nextsector); }
static char wallChkNextWallSector(void) { return wallHelperChkSector(sectorofwall(pWall->nextwall)); }
static char wallChkNextWall(void)       { return wallHelperChkWall(pWall->nextwall); }
static char wallChkPoint2(void)         { return wallHelperChkWall(pWall->point2); }


/** MIXED OBJECT conditions
********************************************************************************/
static char mixChkObjSect(void)         { return (objType == EVOBJ_SECTOR && sectRangeIsFine(objIndex)); }
static char mixChkObjWall(void)         { return (objType == EVOBJ_WALL && wallRangeIsFine(objIndex));   }
static char mixChkObjSpr(void)          { return (objType == EVOBJ_SPRITE && spriRangeIsFine(objIndex)); }
static char mixChkXRange(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return xwallRangeIsFine(pWall->extra);
        case EVOBJ_SPRITE:  return xspriRangeIsFine(pSpr->extra);
        case EVOBJ_SECTOR:  return xsectRangeIsFine(pSect->extra);
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static char mixCmpLotag(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(pWall->type);
        case EVOBJ_SPRITE:  return Cmp(pSpr->type);
        case EVOBJ_SECTOR:  return Cmp(pSect->type);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpPicSurface(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(surfType[pWall->picnum]);
        case EVOBJ_SPRITE:  return Cmp(surfType[pSpr->picnum]);
        case EVOBJ_SECTOR:
            switch (arg3)
            {
                default:    return (Cmp(surfType[pSect->floorpicnum]) || Cmp(surfType[pSect->ceilingpicnum]));
                case  1:    return Cmp(surfType[pSect->floorpicnum]);
                case  2:    return Cmp(surfType[pSect->ceilingpicnum]);
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpPic(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(pWall->picnum);
        case EVOBJ_SPRITE:  return Cmp(pSpr->picnum);
        case EVOBJ_SECTOR:
            switch (arg3)
            {
                default:    return (Cmp(pSect->floorpicnum) || Cmp(pSect->ceilingpicnum));
                case  1:    return Cmp(pSect->floorpicnum);
                case  2:    return Cmp(pSect->ceilingpicnum);
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpPal(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(pWall->pal);
        case EVOBJ_SPRITE:  return Cmp(pSpr->pal);
        case EVOBJ_SECTOR:
            switch(arg3)
            {
                default:    return (Cmp(pSect->floorpal) || Cmp(pSect->ceilingpal));
                case  1:    return Cmp(pSect->floorpal);
                case  2:    return Cmp(pSect->ceilingpal);
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpShade(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(pWall->shade);
        case EVOBJ_SPRITE:  return Cmp(pSpr->shade);
        case EVOBJ_SECTOR:
            switch(arg3)
            {
                default:    return (Cmp(pSect->floorshade) || Cmp(pSect->ceilingshade));
                case  1:    return Cmp(pSect->floorshade);
                case  2:    return Cmp(pSect->ceilingshade);
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpCstat(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return (arg3) ? Cmp(pWall->cstat & arg3) : (pWall->cstat & arg1);
        case EVOBJ_SPRITE:  return (arg3) ? Cmp(pSpr->cstat & arg3)  : (pSpr->cstat & arg1);
        case EVOBJ_SECTOR:  // !!!
            switch (arg3)
            {
                default:    return ((pSect->floorstat & arg1) || (pSect->ceilingstat & arg1));
                case  1:    return (pSect->floorstat & arg1);
                case  2:    return (pSect->ceilingstat & arg1);
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpHitag(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return (arg3) ? Cmp(pWall->hitag & arg3)  : (pWall->hitag & arg1);
        case EVOBJ_SPRITE:  return (arg3) ? Cmp(pSpr->hitag & arg3)   : (pSpr->hitag & arg1);
        case EVOBJ_SECTOR:  return (arg3) ? Cmp(pSect->hitag & arg3)  : (pSect->hitag & arg1);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpXrepeat(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(pWall->xrepeat);
        case EVOBJ_SPRITE:  return Cmp(pSpr->xrepeat);
        case EVOBJ_SECTOR:  return Cmp(pSect->floorxpanning);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpXoffset(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(pWall->xpanning);
        case EVOBJ_SPRITE:  return Cmp(pSpr->xoffset);
        case EVOBJ_SECTOR:  return Cmp(pSect->ceilingxpanning);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpYrepeat(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(pWall->yrepeat);
        case EVOBJ_SPRITE:  return Cmp(pSpr->yrepeat);
        case EVOBJ_SECTOR:  return Cmp(pSect->floorypanning);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpYoffset(void)
{
    switch (objType)
    {
        case EVOBJ_WALL:    return Cmp(pWall->ypanning);
        case EVOBJ_SPRITE:  return Cmp(pSpr->yoffset);
        case EVOBJ_SECTOR:  return Cmp(pSect->ceilingypanning);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixHelperCmpData(int nData)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:
                return Cmp(pXWall->data);
            case EVOBJ_SPRITE:
                switch (nData)
                {
                    case 1:     return Cmp(pXSpr->data1);
                    case 2:     return Cmp(pXSpr->data2);
                    case 3:     return Cmp(pXSpr->data3);
                    case 4:     return Cmp(pXSpr->data4);
                }
                break;
            case EVOBJ_SECTOR:
                return Cmp(pXSect->data);
        }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpData1(void) { return mixHelperCmpData(1); }
static char mixCmpData2(void) { return mixHelperCmpData(2); }
static char mixCmpData3(void) { return mixHelperCmpData(3); }
static char mixCmpData4(void) { return mixHelperCmpData(4); }
static char mixCmpRXId(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return Cmp(pXWall->rxID);
            case EVOBJ_SPRITE:  return Cmp(pXSpr->rxID);
            case EVOBJ_SECTOR:  return Cmp(pXSect->rxID);
        }
    }
    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixCmpTXId(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return Cmp(pXWall->txID);
            case EVOBJ_SPRITE:  return Cmp(pXSpr->txID);
            case EVOBJ_SECTOR:  return Cmp(pXSect->txID);
        }
    }
    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixChkLock(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return pXWall->locked;
            case EVOBJ_SPRITE:  return pXSpr->locked;
            case EVOBJ_SECTOR:  return pXSect->locked;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static char mixChkTriggerOn(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return pXWall->triggerOn;
            case EVOBJ_SPRITE:  return pXSpr->triggerOn;
            case EVOBJ_SECTOR:  return pXSect->triggerOn;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static char mixChkTriggerOff(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return pXWall->triggerOff;
            case EVOBJ_SPRITE:  return pXSpr->triggerOff;
            case EVOBJ_SECTOR:  return pXSect->triggerOff;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static char mixChkTriggerOnce(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return pXWall->triggerOnce;
            case EVOBJ_SPRITE:  return pXSpr->triggerOnce;
            case EVOBJ_SECTOR:  return pXSect->triggerOnce;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static char mixChkIsTriggered(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return pXWall->isTriggered;
            case EVOBJ_SPRITE:  return pXSpr->isTriggered;
            case EVOBJ_SECTOR:  return pXSect->isTriggered;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static char mixChkState(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return pXWall->state;
            case EVOBJ_SPRITE:  return pXSpr->state;
            case EVOBJ_SECTOR:  return pXSect->state;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static char mixCmpBusy(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return Cmp((kPercFull * pXWall->busy) / 65536);
            case EVOBJ_SPRITE:  return Cmp((kPercFull * pXSpr->busy) / 65536);
            case EVOBJ_SECTOR:  return Cmp((kPercFull * pXSect->busy) / 65536);
        }
    }
    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char mixChkPlayerOnly(void)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_WALL:    return pXWall->dudeLockout;
            case EVOBJ_SPRITE:  return pXSpr->dudeLockout;
            case EVOBJ_SECTOR:  return pXSect->dudeLockout;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static char mixHelperCmpSeq(SEQ_CMP_FUNC pFunc)
{
    if (xAvail)
    {
        switch (objType)
        {
            case EVOBJ_SPRITE:
                return Cmp(pFunc(3, pSpr->extra));
            case EVOBJ_WALL:
                switch(arg3)
                {
                    default:    return Cmp(pFunc(0, pWall->extra)) || Cmp(pFunc(4, pWall->extra));
                    case  1:    return Cmp(pFunc(0, pWall->extra));
                    case  2:    return Cmp(pFunc(4, pWall->extra)); // masked wall
                }
                break;
            case EVOBJ_SECTOR:
                switch(arg3)
                {
                    default:    return Cmp(pFunc(1, pSect->extra)) || Cmp(pFunc(2, pSect->extra));
                    case  1:    return Cmp(pFunc(1, pSect->extra));
                    case  2:    return Cmp(pFunc(2, pSect->extra));
                }
                break;
        }
    }

    return Cmp(0);

}
/**---------------------------------------------------------------------------**/
static char mixCmpSeqID(void)       { return mixHelperCmpSeq(seqGetID); }
static char mixCmpSeqFrame(void)    { return mixHelperCmpSeq(seqGetStatus); }
static char mixCmpEventCmd(void)    { return Cmp(pEvent->cmd); }


/** SPRITE conditions
********************************************************************************/
static char sprCmpAng(void)         { return Cmp((arg3 == 0) ? (pSpr->ang & 2047) : pSpr->ang); }
static char sprCmpStatnum(void)     { return Cmp(pSpr->statnum); }
static char sprChkRespawn(void)     { return ((pSpr->flags & kHitagRespawn) || pSpr->statnum == kStatRespawn); }
static char sprCmpSlope(void)       { return Cmp(spriteGetSlope(pSpr->index)); }
static char sprCmpClipdist(void)    { return Cmp(pSpr->clipdist); }
/**---------------------------------------------------------------------------**/
static char sprChkOwner(void)
{
    if (!spriRangeIsFine(pSpr->owner)) return false;
    else if (PUSH) Push(EVOBJ_SPRITE, pSpr->owner);
    return true;
}
/**---------------------------------------------------------------------------**/
static char sprChkSector(void)      { return wallHelperChkSector(pSpr->sectnum); }
static char sprCmpVelocityNew(void)
{
    int xv = xvel[pSpr->index];
    int yv = yvel[pSpr->index];
    int zv = zvel[pSpr->index];

    switch(arg1)
    {
        case 0:     return (Cmp(xv) || Cmp(yv) || Cmp(zv));
        case 1:     return Cmp(xv);
        case 2:     return Cmp(xv);
        case 3:     return Cmp(zv);
    }

    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}
static char sprCmpChkVelocity(void)
{
    if (arg3)
        return sprCmpVelocityNew();

    int xv = xvel[pSpr->index];
    int yv = yvel[pSpr->index];
    int zv = zvel[pSpr->index];

    switch(arg1)
    {
        case 0:     return (xv || yv || zv);
        case 1:     return (xv);
        case 2:     return (yv);
        case 3:     return (zv);

    }

    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}

/**---------------------------------------------------------------------------**/
static char sprChkUnderwater(void)      { return isUnderwaterSector(pSpr->sectnum); }
static char sprChkDmgImmune(void)       { return errCondNotImplemented(); } // !!!
static char sprHelperChkHitscan(int nWhat)
{
    int nAng = pSpr->ang & kAngMask;
    int nOldStat = pSpr->cstat;
    int nHit  = -1, nSlope = 0;
    int nMask = -1;

    if ((nOldStat & kSprRelMask) == kSprSloped)
        nSlope = spriteGetSlope(pSpr->index);

    pSpr->cstat = 0;
    if (nOldStat & kSprOrigin)
        pSpr->cstat |= kSprOrigin;

    if ((nOldStat & kSprRelMask) == kSprFloor)
        pSpr->cstat |= kSprFloor;

    //if ((nOldStat & kSprRelMask) == kSprSloped)
        //pSpr->cstat |= kSprSloped;

    switch (arg1)
    {
        case  0:    nMask = CLIPMASK0 | CLIPMASK1; break;
        case  1:    nMask = CLIPMASK0; break;
        case  2:    nMask = CLIPMASK1; break;
    }

    // !!!
    //if ((pPlayer = getPlayerById(pSpr->type)) != NULL)
        //nHit = HitScan(pSpr, pPlayer->zWeapon, pPlayer->aim.dx, pPlayer->aim.dy, pPlayer->aim.dz, nMask, arg3 << 1);
    //if (IsDudeSprite(pSpr))
        //nHit = HitScan(pSpr, pSpr->z, Cos(nAng) >> 16, Sin(nAng) >> 16, gDudeSlope[pSpr->extra], nMask, arg3 << 1);

    if ((nOldStat & kSprRelMask) == kSprSloped)
    {
        nSlope = (nOldStat & kSprFlipY) ? (0x08000 - abs(nSlope)) : -(0x08000 - abs(nSlope));
        nHit = HitScan(pSpr, pSpr->z, Cos(nAng) >> 16, Sin(nAng) >> 16, nSlope, nMask, arg3 << 1);
    }
    else if ((nOldStat & kSprRelMask) == kSprFloor)
    {
        nSlope = (nOldStat & kSprFlipY) ? (0x10000) : -(0x10000);
        nHit = HitScan(pSpr, pSpr->z, Cos(nAng) >> 16, Sin(nAng) >> 16, nSlope, nMask, arg3 << 1);
    }
    else
    {
        nHit = HitScan(pSpr, pSpr->z, Cos(nAng) >> 16, Sin(nAng) >> 16, 0, nMask, arg3 << 1);
    }

    pSpr->cstat = nOldStat;

    if (nHit < 0)
        return false;

    switch (nWhat)
    {
        case 1: // ceil
            if (nHit != 1) return false;
            else if (PUSH) Push(EVOBJ_SECTOR, gHitInfo.hitsect);
            return true;
        case 2: // floor
            if (nHit != 2) return false;
            else if (PUSH) Push(EVOBJ_SECTOR, gHitInfo.hitsect);
            return true;
        case 3: // wall
            if (nHit != 0 && nHit != 4) return false;
            else if (PUSH) Push(EVOBJ_WALL, gHitInfo.hitwall);
            return true;
        case 4: // sprite
            if (nHit != 3) return false;
            else if (PUSH) Push(EVOBJ_SPRITE, gHitInfo.hitsprite);
            return true;
        case 5: // masked wall
            if (nHit != 4) return false;
            else if (PUSH) Push(EVOBJ_WALL, gHitInfo.hitwall);
            return true;
    }

    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}
/**---------------------------------------------------------------------------**/
static char sprChkHitscanCeil(void)     { return sprHelperChkHitscan(1); }
static char sprChkHitscanFloor(void)    { return sprHelperChkHitscan(2); }
static char sprChkHitscanWall(void)     { return sprHelperChkHitscan(3); }
static char sprChkHitscanSpr(void)      { return sprHelperChkHitscan(4); }
static char sprChkHitscanMasked(void)   { return sprHelperChkHitscan(5); }
static char sprChkIsTarget(void)
{
    int i;
    for (i = headspritestat[kStatDude]; i >= 0; i = nextspritestat[i])
    {
        if (pSpr->index == i)
            continue;

        spritetype* pDude = &sprite[i];
        if (IsDudeSprite(pDude) && xspriRangeIsFine(pDude->extra))
        {
            XSPRITE* pXDude = &xsprite[pDude->extra];
            if (pXDude->health <= 0 || pXDude->target != pSpr->index) continue;
            else if (PUSH) Push(EVOBJ_SPRITE, i);
            return true;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static char sprCmpHealth(void)  // !!!
{
    if (xAvail)
    {
        int t;
        int fullHealth = 100 << 4;

        if (IsDudeSprite(pSpr))
            t = (pXSpr->data4 > 0) ? ClipRange(pXSpr->data4 << 4, 1, 65535) : fullHealth;
        else if (pSpr->type == kThingBloodChunks)
            return Cmp(0);
        else if (IsThingSprite(pSpr))
            t = thingInfo[pSpr->type - kThingBase].startHealth << 4;

        t = ClipLow(t, 1);
        return Cmp((kPercFull * pXSpr->health) / t);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char sprChkTouchCeil(void)
{
    if (!xAvail || (gSpriteHit[pSpr->extra].ceilhit & 0xc000) != 0x4000) return false;
    else if (PUSH) Push(EVOBJ_SECTOR, gSpriteHit[pSpr->extra].ceilhit & 0x3fff);
    return true;
}
/**---------------------------------------------------------------------------**/
static char sprChkTouchFloor(void)
{
    if (!xAvail || (gSpriteHit[pSpr->extra].florhit & 0xc000) != 0x4000) return false;
    else if (PUSH) Push(EVOBJ_SECTOR, gSpriteHit[pSpr->extra].florhit & 0x3fff);
    return true;
}
/**---------------------------------------------------------------------------**/
static char sprChkTouchWall(void)
{
    if (!xAvail || (gSpriteHit[pSpr->extra].hit & 0xc000) != 0x8000) return false;
    else if (PUSH) Push(EVOBJ_WALL, gSpriteHit[pSpr->extra].hit & 0x3fff);
    return true;
}
/**---------------------------------------------------------------------------**/
static char sprChkTouchSpite(void)
{
    int i, id = -1;
    SPRITEHIT* pHit;

    if (xAvail)
    {
        pHit = &gSpriteHit[pSpr->extra];
        switch (arg3)
        {
            case 0:
            case 1:
                if ((pHit->florhit & 0xc000) == 0xc000) id = pHit->florhit & 0x3fff;
                if (arg3 || id >= 0) break;
                fallthrough__;
            case 2:
                if ((pHit->hit & 0xc000) == 0xc000) id = pHit->hit & 0x3fff;
                if (arg3 || id >= 0) break;
                fallthrough__;
            case 3:
                if ((pHit->ceilhit & 0xc000) == 0xc000) id = pHit->ceilhit & 0x3fff;
                break;
        }
    }

    // check if something touching this sprite
    if (id < 0 && sectRangeIsFine(pSpr->sectnum))
    {
        for (i = headspritesect[pSpr->sectnum]; i >= 0; i = nextspritesect[i])
        {
            if (sprite[i].extra <= 0)
                continue;

            pHit = &gSpriteHit[sprite[i].extra];
            if (arg3 == 1 || !arg3)
            {
                if ((pHit->ceilhit & 0xc000) == 0xc000 && (pHit->ceilhit & 0x3fff) == objIndex)
                {
                    id = i;
                    break;
                }
            }

            if (arg3 == 2 || !arg3)
            {
                if ((pHit->hit & 0xc000) == 0xc000 && (pHit->hit & 0x3fff) == objIndex)
                {
                    id = i;
                    break;
                }
            }

            if (arg3 == 3 || !arg3)
            {
                if ((pHit->florhit & 0xc000) == 0xc000 && (pHit->florhit & 0x3fff) == objIndex)
                {
                    id = i;
                    break;
                }
            }
        }
    }

    if (id < 0) return false;
    else if (PUSH) Push(EVOBJ_SPRITE, id);
    return true;
}
/**---------------------------------------------------------------------------**/
static char sprCmpBurnTime(void)
{
    if (xAvail)
    {
        int t = (IsDudeSprite(pSpr)) ? 2400 : 1200;
        if (!Cmp((kPercFull * pXSpr->burnTime) / t)) return false;
        else if (PUSH && spriRangeIsFine(pXSpr->burnSource)) Push(EVOBJ_SPRITE, pXSpr->burnSource);
        return true;
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static char sprChkIsFlareStuck(void)
{
    int i;
    for (i = headspritestat[kStatFlare]; i >= 0; i = nextspritestat[i])
    {
        spritetype* pFlare = &sprite[i];
        if (!xspriRangeIsFine(pFlare->extra) || (pFlare->flags & kHitagFree))
            continue;

        XSPRITE* pXFlare = &xsprite[pFlare->extra];
        if (pXFlare->target != objIndex) continue;
        else if (PUSH) Push(EVOBJ_SPRITE, i);
        return true;
    }

    return false;
}
static char sprCmpMass(void) { return Cmp((xAvail) ? getSpriteMassBySize(pSpr) : 0); }


/** DUDE functions
********************************************************************************/
static char dudChkFlagPatrol(void)          { return (xAvail) ? pXSpr->dudeFlag4  : false; };
static char dudChkFlagDeaf(void)            { return (xAvail) ? pXSpr->dudeDeaf   : false; };
static char dudChkFlagBlind(void)           { return (xAvail) ? pXSpr->dudeGuard  : false; };
static char dudChkFlagAlarm(void)           { return (xAvail) ? pXSpr->dudeAmbush : false; };
static char dudChkFlagStealth(void)         { return (xAvail) ? (pXSpr->unused1 & kDudeFlagStealth) : false; };
//static char dudCmpAiStateType(void)           { return Cmp((xAvail) ? pXSpr->aiState->stateType : 0); };

/** TABLE OF CONDITION FUNCTIONS
********************************************************************************/
static CONDITION_INFO gConditions[kMaxConditions] =
{
    { gamCmpLevelMin,           1,      CGAM },         // compare level minutes
    { gamCmpLevelSec,           2,      CGAM },         // compare level seconds
    { gamCmpLevelMsec,          3,      CGAM },         // compare level mseconds
    { gamCmpLevelTime,          4,      CGAM },         // compare level time (unsafe)
    { gamCmpKillsTotal,         5,      CGAM },         // compare current global kills counter
    { gamCmpKillsDone,          6,      CGAM },         // compare total global kills counter
    { gamCmpSecretsDone,        7,      CGAM },         // compare how many secrets found
    { gamCmpSecretsTotal,       8,      CGAM },         // compare total secrets
    { gamCmpVisibility,         20,     CGAM },         // compare global visibility value
    { gamChkChance,             30,     CGAM },         // check chance %
    { gamCmpRandom,             31,     CGAM },         // compare random
    { gamCmpStatnumCount,       47,     CGAM },         // compare counter of specific statnum sprites
    { gamCmpNumsprites,         48,     CGAM },         // compare counter of total sprites
    /**---------------------------------------------------------------------------**/
    { mixChkObjSect,            100,    CMIX },
    { mixChkObjWall,            105,    CMIX },
    { mixChkObjSpr,             110,    CMIX },
    { mixChkXRange,             115,    CMIX },
    { mixCmpLotag,              120,    CMIX },
    { mixCmpPicSurface,         124,    CMIX },
    { mixCmpPic,                125,    CMIX },
    { mixCmpPal,                126,    CMIX },
    { mixCmpShade,              127,    CMIX },
    { mixCmpCstat,              128,    CMIX },
    { mixCmpHitag,              129,    CMIX },
    { mixCmpXrepeat,            130,    CMIX },
    { mixCmpXoffset,            131,    CMIX },
    { mixCmpYrepeat,            132,    CMIX },
    { mixCmpYoffset,            133,    CMIX },
    { mixCmpData1,              141,    CMIX },
    { mixCmpData2,              142,    CMIX },
    { mixCmpData3,              143,    CMIX },
    { mixCmpData4,              144,    CMIX },
    { mixCmpRXId,               150,    CMIX },
    { mixCmpTXId,               151,    CMIX },
    { mixChkLock,               152,    CMIX },
    { mixChkTriggerOn,          153,    CMIX },
    { mixChkTriggerOff,         154,    CMIX },
    { mixChkTriggerOnce,        155,    CMIX },
    { mixChkIsTriggered,        156,    CMIX },
    { mixChkState,              157,    CMIX },
    { mixCmpBusy,               158,    CMIX },
    { mixChkPlayerOnly,         159,    CMIX },
    { mixCmpSeqID,              170,    CMIX },
    { mixCmpSeqFrame,           171,    CMIX },
    { mixCmpEventCmd,           199,    CMIX },         // this condition received N command?
    /**---------------------------------------------------------------------------**/
    { wallCmpOverpicnum,        200,    CWAL },
    { wallChkSector,            205,    CWAL },
    { wallChkIsMirror,          210,    CWAL },
    { wallChkNextSector,        215,    CWAL },
    { wallChkNextWall,          220,    CWAL },
    { wallChkPoint2,            221,    CWAL },
    { wallChkNextWallSector,    225,    CWAL },
    /**---------------------------------------------------------------------------**/
    { sectCmpVisibility,        300,    CSEC },         // compare visibility
    { sectCmpFloorSlope,        305,    CSEC },         // compare floor slope
    { sectCmpCeilSlope,         306,    CSEC },         // compare ceil slope
    { sectChkSprTypeInSect,     310,    CSEC },         // check is sprite with lotag N in sector
    { sectChkUnderwater,        350,    CSEC },         // sector is underwater?
    { sectCmpDepth,             351,    CSEC },         // compare depth level
    { sectCmpFloorHeight,       355,    CSEC },         // compare floor height (in %)
    { sectCmpCeilHeight,        356,    CSEC },         // compare ceil height (in %)
    { sectChkPaused,            357,    CSEC },         // this sector in movement?
    /**---------------------------------------------------------------------------**/
    { dudChkFlagPatrol,         455,    CDUD },
    { dudChkFlagDeaf,           456,    CDUD },
    { dudChkFlagBlind,          457,    CDUD },
    { dudChkFlagAlarm,          458,    CDUD },
    { dudChkFlagStealth,        459,    CDUD },
    /**---------------------------------------------------------------------------**/
    { sprCmpAng,                500,    CSPR },         // compare angle
    { sprCmpStatnum,            505,    CSPR },         // check statnum
    { sprChkRespawn,            506,    CSPR },         // check if on respawn list
    { sprCmpSlope,              507,    CSPR },         // compare slope
    { sprCmpClipdist,           510,    CSPR },         // compare clipdist
    { sprChkOwner,              515,    CSPR },         // check owner sprite
    { sprChkSector,             520,    CSPR },         // stays in a sector?
    { sprCmpChkVelocity,        525,    CSPR },         // check or compare velocity
    { sprCmpVelocityNew,        526,    CSPR },         // compare velocity
    { sprChkUnderwater,         530,    CSPR },         // sector of sprite is underwater?
    { sprChkDmgImmune,          531,    CSPR },         // !!! check if immune to N dmgType
    { sprChkHitscanCeil,        535,    CSPR },         // !!! hitscan: ceil?
    { sprChkHitscanFloor,       536,    CSPR },         // !!! hitscan: floor?
    { sprChkHitscanWall,        537,    CSPR },         // !!! hitscan: wall?
    { sprChkHitscanSpr,         538,    CSPR },         // !!! hitscan: sprite?
    { sprChkHitscanMasked,      539,    CSPR },         // !!! hitscan: masked wall?
    { sprChkIsTarget,           545,    CSPR },         // this sprite is a target of some dude?
    { sprCmpHealth,             550,    CSPR },         // !!! compare hp (in %)
    { sprChkTouchCeil,          555,    CSPR },         // touching ceil of sector?
    { sprChkTouchFloor,         556,    CSPR },         // touching floor of sector?
    { sprChkTouchWall,          557,    CSPR },         // touching walls of sector?
    { sprChkTouchSpite,         558,    CSPR },         // touching another sprite?
    { sprCmpBurnTime,           565,    CSPR },         // compare burn time (in %)
    { sprChkIsFlareStuck,       566,    CSPR },         // any flares stuck in this sprite?
    { sprCmpMass,               570,    CSPR },         // mass of the sprite in a range?
    /**---------------------------------------------------------------------------**/
    { NULL,                     0,      0   },          // indicate end of the list
};

/** CONDITION INTERFACE FUNCTIONS
********************************************************************************/
static char CheckGeneric()  { return pEntry->pFunc(); }
static char CheckSector()
{
    if (objType == EVOBJ_SECTOR && rngok(objIndex, 0, numsectors))
    {
        pSect = &sector[objIndex];
        if (pSect->extra > 0)
        {
            pXSect  = &xsector[pSect->extra];
            xAvail  = true;
        }
        else
        {
            pXSect  = NULL;
            xAvail  = false;
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, gSearchStatNames[OBJ_SECTOR]);
        return false;
    }

    return pEntry->pFunc();
}

static char CheckWall()
{
    if (objType == EVOBJ_WALL && rngok(objIndex, 0, numwalls))
    {
        pWall = &wall[objIndex];
        if (pWall->extra > 0)
        {
            pXWall  = &xwall[pWall->extra];
            xAvail  = true;
        }
        else
        {
            pXWall  = NULL;
            xAvail  = false;
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, gSearchStatNames[OBJ_WALL]);
        return false;
    }

    return pEntry->pFunc();
}

static char CheckDude()
{
    if (objType == EVOBJ_SPRITE && rngok(objIndex, 0, kMaxSprites))
    {
        pSpr = &sprite[objIndex];
        if (!IsDudeSprite(pSpr) || IsPlayerSprite(pSpr) || pSpr->type != kThingBloodChunks)
        {
            Error(gErrors[kErrInvalidObject], objIndex, objType, "enemy");
            return false;
        }

        if (pSpr->extra > 0)
        {
            pXSpr   = &xsprite[pSpr->extra];
            xAvail  = true;
        }
        else
        {
            pXSpr   = NULL;
            xAvail  = false;
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, gSearchStatNames[OBJ_SPRITE]);
        return false;
    }

    return pEntry->pFunc();
}

static char CheckSprite()
{
    if (objType == EVOBJ_SPRITE && rngok(objIndex, 0, kMaxSprites))
    {
        pSpr = &sprite[objIndex];
        if (pSpr->extra > 0)
        {
            pXSpr   = &xsprite[pSpr->extra];
            xAvail  = true;
        }
        else
        {
            pXSpr   = NULL;
            xAvail  = false;
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, gSearchStatNames[OBJ_SPRITE]);
        return false;
    }

    return pEntry->pFunc();
}

static char CheckObject()
{
    switch (objType)
    {
        case EVOBJ_SECTOR:  return CheckSector();
        case EVOBJ_SPRITE:  return CheckSprite();
        case EVOBJ_WALL:    return CheckWall();
    }

    return pEntry->pFunc();
}

static void Push(int oType, int oIndex)
{
   pXCond->targetX = Serialize(oType, oIndex);
}

static void Restore()
{
    pXCond->targetX = pXCond->targetY;
}

static int Serialize(int oType, int oIndex)
{
    switch (oType)
    {
        case EVOBJ_SECTOR: return kSerialSector + oIndex;
        case EVOBJ_WALL:   return kSerialWall   + oIndex;
        case EVOBJ_SPRITE: return kSerialSprite + oIndex;
    }

    Error(gErrors[kErrUnknownObject], oType, oIndex);
    return -1;
}

static void Unserialize(int nSerial, int* oType, int* oIndex)
{
    if (rngok(nSerial, kSerialSector, kSerialWall))
    {
        *oIndex = nSerial - kSerialSector;
        *oType  = EVOBJ_SECTOR;
    }
    else if (rngok(nSerial, kSerialWall, kSerialSprite))
    {
        *oIndex = nSerial - kSerialWall;
        *oType  = EVOBJ_WALL;
    }
    else if (rngok(nSerial, kSerialSprite, kSerialMax))
    {
        *oIndex = nSerial - kSerialSprite;
        *oType  = EVOBJ_SPRITE;
    }
    else
    {
        Error(gErrors[kErrInvalidSerial], nSerial);
    }
}

// normal comparison
static char Cmp(int val)
{
    if (cmpOp & 0x2000)
        return (cmpOp & kSprBlock) ? (val > arg1) : (val >= arg1); // blue sprite
    else if (cmpOp & 0x4000)
        return (cmpOp & kSprBlock) ? (val < arg1) : (val <= arg1); // green sprite
    else if (cmpOp & kSprBlock)
    {
        if (arg1 > arg2)
            Error(gErrors[kErrInvalidArg2], arg1, arg2);

        return (val >= arg1 && val <= arg2);
    }
    else return (val == arg1);
}

static void Error(char* pFormat, ...)
{
    if (errShown[pCond->index].ok)
        return;

    char buffer[512], buffer2[512], condType[32];
    Bsprintf(condType, (pEntry) ? gCheckFuncInfo[pEntry->type].name : "Unknown");
    Bstrupr(condType);

    Bsprintf(buffer, "%s CONDITION ID: %d,  RX: %d, TX: %d, SPRITE: #%d RETURNS:\n",
                        condType, pXCond->data1, pXCond->rxID, pXCond->txID, pXCond->reference);
    va_list args;
    va_start(args, pFormat);
    vsprintf(buffer2, pFormat, args);
    va_end(args);

    Bstrcat(buffer, buffer2);
    Bsprintf(buffer2,
    "\n\n"
    "Ignoring errors may lead to unexpected results.\n"
    "Disable this condition now?");

    Bstrcat(buffer, buffer2);

    if (Confirm(buffer))
        pXCond->isTriggered = true;
    else
        errShown[pCond->index].ok = 1;

}

static void TriggerObject(int nSerial)
{
    int oType, oIndex;
    Unserialize(nSerial, &oType, &oIndex);
    nnExtTriggerObject(oType, oIndex, pXCond->command);
}


void conditionsInit()
{
    int i, j, cnt = 0;
    static char done = false;

    memset(errShown, 0, sizeof(errShown));

    if (!done)
    {
        // sort out *condition function callers* list the right way
        qsort(gCheckFuncInfo, LENGTH(gCheckFuncInfo), sizeof(gCheckFuncInfo[0]), (int(*)(const void*,const void*))qsSortCheckFuncInfo);

        // find the end of *condition functions* list
        for (i = 0; i < LENGTH(gConditions); i++, cnt++)
        {
            CONDITION_INFO* pTemp = &gConditions[i];
            if (!pTemp->pFunc && !pTemp->type && !pTemp->id)
                break;
        }

        dassert(i < LENGTH(gConditions));
        CONDITION_INFO* pCopy = (CONDITION_INFO*)Bmalloc(sizeof(gConditions));
        memcpy(pCopy, gConditions, sizeof(gConditions));

        // check for duplicates
        for (i = 0; i < cnt; i++)
        {
            dassert(pCopy[i].id < kMaxConditions);

            for(j = 0; j < cnt; j++)
            {
                if (i != j)
                    dassert(pCopy[i].id != pCopy[j].id);
            }
        }

        // set proper id and dummy template
        for (i = 0; i < LENGTH(gConditions); i++)
        {
            CONDITION_INFO* pTemp = &gConditions[i];
            pTemp->pFunc    = errCondNotImplemented; pTemp->type = CNON;
            pTemp->pCaller  = &gCheckFuncInfo[pTemp->type];
            pTemp->id       = i;
        }

        // set proper caller info for each condition function
        for (i = 0; i < cnt; i++)
        {
            CONDITION_INFO* pTemp = &pCopy[i];
            for (j = 0; j < LENGTH(gCheckFuncInfo); j++)
            {
                if (gCheckFuncInfo[j].type == pTemp->type)
                {
                    pTemp->pCaller = &gCheckFuncInfo[j];
                    break;
                }
            }

            dassert(j < LENGTH(gCheckFuncInfo));

            // sort out condition list the right way
            memcpy(&gConditions[pCopy[i].id], pTemp, sizeof(gConditions[0]));
        }

        free(pCopy);
        done = true;
    }

    // allocate tracking conditions and collect objects for it
    conditionsTrackingAlloc();
}

void conditionsTrackingAlloc()
{
    int nCount = 0, i, j, s, e;
    conditionsTrackingClear();

    for (i = headspritestat[kStatModernCondition]; i >= 0; i = nextspritestat[i])
    {
        spritetype* pSprite =& sprite[i]; XSPRITE* pXSprite = &xsprite[pSprite->extra];
        if (pSprite->extra <= 0 || !pXSprite->busyTime || pXSprite->isTriggered)
            continue;

        gTrackingConditionsList = (TRACKING_CONDITION*)Brealloc(gTrackingConditionsList, (nCount+1)*sizeof(TRACKING_CONDITION));
        if (!gTrackingConditionsList)
            break;

        TRACKING_CONDITION* pTr = &gTrackingConditionsList[nCount];
        pTr->objects = new(OBJECT_LIST);

        for (j = 0; j < numsectors; j++)
        {
            for (s = headspritesect[j]; s >= 0; s = nextspritesect[s])
            {
                spritetype* pObj = &sprite[s];
                if (pObj->extra <= 0 || pObj->index == pSprite->index)
                    continue;

                XSPRITE* pXSpr = &xsprite[pObj->extra];
                if (pXSpr->txID != pXSprite->rxID)
                    continue;

                // check exceptions
                switch (pObj->type)
                {
                    case kSwitchToggle:
                    case kSwitchOneWay:
                    case kModernPlayerControl:
                    case kModernCondition:
                    case kModernConditionFalse:
                         continue;
                    default:
                        pTr->objects->Add(EVOBJ_SPRITE, pObj->index);
                        break;
                }
            }

            getSectorWalls(j, &s, &e);
            while(s <= e)
            {
                walltype* pObj = &wall[s];
                if (pObj->extra > 0 && xwall[pObj->extra].txID == pXSprite->rxID)
                {
                    // check exceptions
                    switch (pObj->type)
                    {
                        case kSwitchToggle:
                        case kSwitchOneWay:
                            break;
                        default:
                            pTr->objects->Add(EVOBJ_WALL, s);
                            break;
                    }
                }

                s++;
            }

            sectortype* pObj = &sector[j];
            if (pObj->extra > 0 && xsector[pObj->extra].txID == pXSprite->rxID)
                pTr->objects->Add(EVOBJ_SECTOR, j);
        }

        if (pTr->objects->Length())
        {
            pTr->id = pSprite->extra;
            nCount++;
        }
        else
        {
            delete pTr->objects;
            pTr->objects = NULL;
        }
    }

    if (!nCount)
        conditionsTrackingClear();

    gTrackingConditionsListLength = nCount;
}

void conditionsTrackingClear()
{
    int i = gTrackingConditionsListLength;
    if (gTrackingConditionsList)
    {
        while(--i >= 0)
        {
            TRACKING_CONDITION* pTr = &gTrackingConditionsList[i];
            if (pTr->objects)
            {
                delete(pTr->objects);
                pTr->objects = NULL;
            }
        }

        Bfree(gTrackingConditionsList);
    }

    gTrackingConditionsList         = NULL;
    gTrackingConditionsListLength   = 0;
}

void conditionsTrackingProcess()
{
    EVENT evn; OBJECT* o;
    evn.funcID = kCallbackMax; evn.cmd = kCmdOn;
    int i = gTrackingConditionsListLength;
    int t;

    while(--i >= 0)
    {
        TRACKING_CONDITION* pTr = &gTrackingConditionsList[i];

        XSPRITE* pXSpr = &xsprite[pTr->id];
        if (pXSpr->locked || pXSpr->isTriggered || ++pXSpr->busy < pXSpr->busyTime)
            continue;

        pXSpr->busy = 0;
        if ((t = pTr->objects->Length()) > 0)
        {
            o = pTr->objects->Ptr();
            while(--t >= 0)
            {
                evn.type = o[t].type; evn.index = o[t].index;
                useCondition(&sprite[pXSpr->reference], pXSpr, evn);
            }
        }
        else if (rngok(pXSpr->data1, 0, kMaxConditions))
        {
            CONDITION_INFO* pInfo = &gConditions[pXSpr->data1];
            if (pInfo->type != CGAM)
                continue;

            evn.type = EVOBJ_SPRITE; evn.index = pXSpr->reference;
            useCondition(&sprite[pXSpr->reference], pXSpr, evn);
        }
    }
}

void useCondition(spritetype* pSource, XSPRITE* pXSource, EVENT event)
{
    char ok = false;
    char srcIsCondition = false;

    // if it's a tracking condition, it must ignore all the commands sent from objects
    if (pXSource->busyTime && event.funcID != kCallbackMax)
        return;

    objType     = event.type;
    objIndex    = event.index;


    if (objType == EVOBJ_SPRITE && objIndex != pSource->index)
        srcIsCondition = (sprite[objIndex].type == kModernCondition || sprite[objIndex].type == kModernConditionFalse);

    if (!srcIsCondition)
    {
        // save object serials in the "stack" and make copy of initial object
        pXSource->targetX = pXSource->targetY = Serialize(objType, objIndex);
    }
    else
    {
        // or grab serials of objects from previous conditions
        pXSource->targetX = xsprite[sprite[objIndex].extra].targetX;
        pXSource->targetY = xsprite[sprite[objIndex].extra].targetY;
    }

    pEntry  = NULL;
    pCond   = pSource;
    pXCond  = pXSource;

    if (pXCond->restState == 0)
    {
        if (!pXCond->data1) ok = true;
        else if (rngok(pXCond->data1, 0, kMaxConditions))
        {
            PUSH    = (pXCond->command == 64);      cmpOp   = pCond->cstat;
            arg1    = pXCond->data2;                arg2    = pXCond->data3;
            arg3    = pXCond->data4;                pEvent  = &event;

            Unserialize(pXCond->targetX, &objType, &objIndex);
            pEntry = &gConditions[pXCond->data1];
            ok = pEntry->pCaller->pFunc();

            // (for preview mode) shutdown probably because of an error
            if (pXCond->isTriggered)
                return;
        }
        else
        {
            errCondUnknown();
            return;
        }

        pXCond->state = (ok ^ (pCond->type == kModernConditionFalse));

        if (pXCond->waitTime && pXCond->state)
        {
            pXCond->restState = 1;
            evKill(pCond->index, EVOBJ_SPRITE);
            evPost(pCond->index, EVOBJ_SPRITE, (pXCond->waitTime * 120) / 10, kCmdRepeat);
            return;
        }
    }
    else if (event.cmd == kCmdRepeat)
    {
        pXCond->restState = 0;
    }
    else
    {
        return;
    }

    if (pXCond->state)
    {
        // trigger once per result?
        if (pCond->flags & kModernTypeFlag4)
        {
            if (pXCond->unused2)
                return;

            pXCond->unused2 = 1;
        }

        if (!pXCond->isTriggered)
            pXCond->isTriggered = pXCond->triggerOnce;

        if (pXCond->command == 100)
            Restore(); // reset focus to the initial object

        // send command to rx bucket
        if (pXCond->txID)
            evSend(pCond->index, EVOBJ_SPRITE, pXCond->txID, (COMMAND_ID)pXCond->command);

        if (pCond->flags)
        {
            // send it for object that currently in the focus
            if (pCond->flags & kModernTypeFlag1)
            {
                TriggerObject(pXCond->targetX);
                if ((pCond->flags & kModernTypeFlag2) && pXCond->targetX == pXCond->targetY)
                    return;
            }

            // send it for initial object
            if (pCond->flags & kModernTypeFlag2)
            {
                TriggerObject(pXCond->targetY);
            }
        }
    }
    else
    {
        pXCond->unused2 = 0;
    }
}