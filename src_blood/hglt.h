/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions to work with objects in a highlight of any case (mostly sprites).
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
#ifndef _HGLT_H
#define _HGLT_H
#include "common_game.h"


typedef void HSECTORFUNC(int nSector, int nData);
typedef void HSECTORFUNC2(int nSector, int arg1, int arg2, int arg3, int arg4);
typedef void HSPRITEFUNC(spritetype *pSprite, int nData);


enum {
kHgltSector         = 0x0001,
kHgltPoint          = 0x0002,
kHgltGradient       = 0x0004,
};


extern BITARRAY16 hgltspri, hgltwall;
extern int hgltx1, hgltx2, hglty1, hglty2;
extern short hgltType;

void sprFixSector(spritetype* pSprite, int flags = 0);
void sprDelete(spritetype* pSprite, int dummy = 0);
void sprPalSet(spritetype* pSprite, int value);
void sprShadeSet(spritetype* pSprite, int value);
void sprShadeIterate(spritetype* pSprite, int step);
void sprClone(spritetype* pSprite, int);
void sprSetXRepeat(spritetype* pSprite, int val);
void sprSetYRepeat(spritetype* pSprite, int val);
int hgltSprCallFunc(HSPRITEFUNC SpriteFunc,  int nData = 0);
int hgltSprCount();


void sectChgVisibility(int nSect, int nVis);
void sectChgShade(int nSect, int nOf, int nShade, int a3=0, int a4=0);
void sectDelete(int nSector, int a1=0, int a2=0, int a3=0, int a4=0);
void sectChgXY(int nSector, int bx, int by, int a3=0, int a4=0);
void sectChgZ(int nSector, int bz, int a2 = 0, int flags = 0, int a4 = 0);
void sectSetupZOffset(int nSect, sectortype* pOParent, sectortype* pNParent, int flags);
void sectSetupZOffset(int nSect, int nParentOld, int nParentNew, int flags, int);
int hgltSectCallFunc(HSECTORFUNC2 SectorFunc, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);


void hgltIsolatePathMarkers(int which = kHgltPoint | kHgltSector);


void hgltSprPutOnWall(int nwall, int x, int y);
void sprGetZOffsets(short idx, int* top, int* bot);

BOOL hgltShowStatus(int x, int y);
short hgltCheck(int type, int idx);
int hgltAdd(int type, int idx);
short hglt2dAddInXYRange(int hgltType, int x1, int y1, int x2, int y2);


BOOL hgltRemove(int type, int idx);
void hgltReset(int which = kHgltPoint | kHgltSector | kHgltGradient);

short hglt2dAdd(int type, int idx);
short hglt2dRemove(int type, int idx);

void hgltSprAvePoint(int* dax, int* day, int* daz = NULL);
void hgltSprSetXYZ(int x, int y, int z = 0);
void hgltSprChgXYZ(int xstep, int ystep, int zstep = 0);
void hgltSprPutOnCeiling(int ofs = 0);
void hgltSprPutOnFloor(int ofs = 0);
void hgltSprPutOnZ(int z, int which, int tofs = 0, int bofs = 0);
void hgltSprGetZEdgeSpr(short* lowest, short* highest);
void hgltSprGetEdges(int* left, int* right, int* top, int* bot, int* ztop, int* zbot, int* ztofs = NULL, int* zbofs = NULL);
void hgltSprGetEdgeSpr(short* ls, short* rs, short* ts, short* bs, short* zts, short* zbs);
void hgltSprClamp(int ofsAboveCeil = 0, int ofsBelowFloor = 0, int which = 0x0003);
void hgltSprRotate(int step);
inline char hgltSprIsFine(int nSprite)
{
    return (nSprite >= 0 && nSprite < kMaxSprites && sprite[nSprite].statnum < kStatFree);
}

inline char sprInHglt(int idx)
{
    return (hgltCheck(OBJ_SPRITE, idx) >= 0);
}

inline char sectInHglt(int idx)
{
    return (hgltCheck(OBJ_SECTOR, idx) >= 0);
}

inline char wallInHglt(int idx)
{
    return (hgltCheck(OBJ_WALL, idx) >= 0);
}

int hgltWallCount(int* whicnt = NULL, int* redcnt = NULL);
int hgltWallsCheckStat(int nStat, int which = 0x03, int nMask = 0x0);

void hgltSectDetach();
void hgltSectAttach();
void hgltSectGetBox(int* x1, int* y1, int* x2, int *y2);
void hgltSectMidPoint(int* ax, int* ay);
char hgltSectInsideBox(int x, int y);
int hgltSectFlip(int flags);
int hgltSectRotate(int flags, int nAng);
int hgltSectAutoRedWall(int flags = 0x00);
int hgltSectDelete();

char hgltListOuterLoops(int* s, int* e, IDLIST* slist, IDLIST* wlist, char which);
void hgltIsolateChannels(char which);
void hgltIsolateRorMarkers(int which);

void hglt2List(OBJECT_LIST* pList, char asX, char which);
#endif