/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026: Originally written by NoOne.
// Functions for editing the maps.
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

#ifndef __XMAPEDIT_H
#define __XMAPEDIT_H

#include "common_game.h"
#include "xmpdoorwiz.h"
#include "xmpshape.h"
#include "xmploop.h"
#include "xmparcwiz.h"
#include "xmplsplt.h"
#include "xmpstub.h"
#include "edit2d.h"
#include "edit3d.h"
#include "maproc.h"
#include "xmpmisc.h"
#include "hglt.h"

enum
{
kSectToolShape          = 0 ,
kSectToolLoopSplit          ,
kSectToolDoorWiz            ,
kSectToolArcWiz             ,
kSectToolCurveWall          ,
kSectToolUnk                ,
};

enum
{
ILOOP_AUTO          = 0x00,
ILOOP_DIR_FW        = 0x01,
ILOOP_DIR_BW        = 0x02,
ILOOP_DIR_ASIS      = 0x03,

ILOOP_NO_SET_FW     = 0x10,
ILOOP_NO_FIX_REP    = 0x20,
ILOOP_NO_FLIP       = 0x40,
};

enum
{
ISPLIT_TYPE_CROSS        = 1,
ISPLIT_TYPE_ONLINE          ,
ISLIT_TYPE_COVER            ,
ISPLIT_TYPE_POINT           ,
ISPLIT_TYPE_MAX             ,
};

struct ISPLITENTRY
{
    unsigned int type       : 4;
    unsigned int skip       : 2;
    unsigned int onpoint    : 1;
    int32_t x, y;
    int16_t s, w;
};

struct ISPLITANALYZE
{
    unsigned int splitType      : 2;    // classic or multi
    unsigned int splitError     : 2;    // obsolete or all skips
    unsigned int isHavePoints   : 1;
    unsigned int isSplitFirst   : 1;
};

struct ISPLITOUT
{
    VOIDLIST* entries;
    uint16_t numsectors;
    uint16_t numwalls;
    
    int AnalyzeEntries(ISPLITANALYZE* pOut)
    {
        memset(pOut, 0, sizeof(*pOut));
        
        if (!entries)
            return -1;
        
        ISPLITENTRY* p = (ISPLITENTRY*)entries->First();
        int coverCnt = 0, nSect, i;
        int c = entries->Length();
        
        pOut->splitError = 1;
        //if (c > 1 || !irngok(p->type, 1, 2))
            //pOut->splitError &= ~0x01;
        
        nSect = -1;
        for (i = 0; i < c; i++)
        {
            if (p[i].type == 3)
                coverCnt++;
            
            if (p[i].type == 4)
                pOut->isHavePoints = 1;
            
            if (irngok(p[i].type, 1, 2))
            {
                if (nSect < 0)
                {
                    pOut->splitType = 1;
                    nSect = p[i].s;
                }
                else if (nSect != p[i].s)
                {
                    pOut->splitType = 2;
                }
                
                if (p[i].skip == 0)     pOut->splitError   = 0;
                if (i == 0)             pOut->isSplitFirst = 1;
            }
        }
        
        if (coverCnt != c)
        {
            if (pOut->isHavePoints)
            {
                if (pOut->splitType && !pOut->splitError)
                    return 4;
                
                return 3;
            }
            
            //if (pOut->splitType && !pOut->splitError)
                return (pOut->splitType == 1) ? 1 : 2;
        }
        
        return 0;
    }
};

struct ISPLITPARAM
{
    unsigned int collect    : 1;
    unsigned int isloop     : 1;
    int32_t startSect;
    POINT2D* points;
    int32_t numpoints;
    ISPLITOUT out;
};

extern char* const gSectToolNames[];


void avePointLoop(int s, int e, int *x, int *y);
void midPointLoop(int nFirst, int nLast, int* ax, int *ay);
void avePointSector(int nSector, int *x, int *y);
void avePointWall(int nWall, int* x, int* y, int* z);
void avePointWall(int nWall, int* x, int* y);


void loopGetEdgeWalls(int nFirst, int nLast, int* l, int* r, int* t, int* b);
void loopGetWalls(int nStartWall, int* swal, int *ewal);
void loopGetBox(int nFirst, int nLast, int* x1, int* y1, int* x2, int *y2);
void loopChgPos(int s, int e, int bx, int by, int flags);
void loopRotate(int s, int e, int cx, int cy, int nAng, int flags);
void loopFlip(int s, int e, int cx, int cy, int flags);
void loopAttach(int nWall);
void loopDetach(int nWall);
void loopDelete(int nWall);
int loopTransfer(int nWall, int nSect, char flags = 0x03);


void getSectorWalls(int nSect, int* s, int *e);
void sectGetEdgeZ(int nSector, int* fz, int* cz);
void sectRotate(int nSect, int cx, int cy, int nAng, int flags);
void sectFlip(int nSect, int cx, int cy, int flags, int);
int sectSplit(int nSect, int nWallA, int nWallB, POINT2D* points, int nPoints);
int sectSplit(int nSect, POINT2D* points, int nPoints);
int intersectSplit(ISPLITPARAM* pParam);

short sectCstatAdd(int nSect, short cstat, int objType);
short sectCstatRem(int nSect, short cstat, int objType);
short sectCstatToggle(int nSect, short cstat, int objType);
short sectCstatGet(int nSect, int objType);
short sectCstatGet(int nSect, short cstat, int objType);
short sectCstatSet(int nSect, short cstat, int objType);
void sectDetach(int nSect);
void sectAttach(int nSect);
char sectAutoAlignSlope(int nSect, char which = 0x03);
int sectCountParts(int nSect);
void sectLoopMain(int nSect, int* s, int* e);
int sectContentsTransfer(int nSectA, int nSectB, char flags = 0xFF);
#define sectLoopTransfer(a, b) sectContentsTransfer(a, b, 0x01)

void wallDetach(int nWall);
void wallAttach(int nWall, int nNextS, int nNextW);
void wallAttach(int nWall, int nNextW);
double getWallLength(int nWall, int nGrid);
void getWallCoords(int nWall, int* x1 = NULL, int* y1 = NULL, int* x2 = NULL, int* y2 = NULL);
short wallCstatAdd(int nWall, short cstat, char nextWall = 1);
short wallCstatRem(int nWall, short cstat, char nextWall = 1);
short wallCstatToggle(int nWall, short cstat, char nextWall = 1);
char wallVisible(int nWall);
int wallGetSect(int nWall);
void wallRotateTile(int nWall, char enable);

int getSectorHeight(int nSector);
void setFirstWall(int nSect, int nWall);
char setAligntoWall(int nSect, int nWall);
char loopInside(int x, int y, int nWall);
char loopInside(int nSect, POINT2D* pPoint, int nCount, char full);
int insertLoopEx(int nSect, POINT2D* pInfo, int nCount, char flags = ILOOP_AUTO, walltype* pWModel = NULL, sectortype* pSModel = NULL);
int insertLoop(int nSect, POINT2D* pInfo, int nCount, walltype* pWModel = NULL, sectortype* pSModel = NULL);
int insertLoopEx(int nSect, walltype* pWalls, int nCount, char flags = ILOOP_AUTO, sectortype* pSModel = NULL);
int insertLoop(int nSect, walltype* pWalls, int nCount, sectortype* pSModel = NULL);
void insertPoints(WALLPOINT_INFO* pInfo, int nCount);
int makeSquareSector(int ax, int ay, int area);
int redSectorMake(int nStartWall);
int redSectorMerge(int nThis, int nWith);
char insidePoints(int x, int y, POINT2D* point, int c);
char insidePoints(int nSect, POINT2D* point, int c);

char pointOnLine(int x, int y, int x1, int y1, int x2, int y2, int d = 4);
inline char pointOnWallLine(int w, int x, int y, int d = 4)
{
    return pointOnLine(x, y, wall[w].x, wall[w].y, wall[wall[w].point2].x, wall[wall[w].point2].y, d);
}

int findWallAtPos(int x, int y);
int findWallAtPos(int nSect, int x, int y);
int findSolidWallAtPos(int x, int y);
int findWallAtPos(int x1, int y1, int x2, int y2);
int findWallAtPos(int nSect, int x1, int y1, int x2, int y2);
int findSplitWall(int nSect, int nWall, int x3, int y3, int x4, int y4);
int findNextWall(int nWall);
char isNextWallOf(int nSrc, int nDest);

char insideLoop(int x, int y, int nStartWall);

int insertPoint(int nWall, int x, int y);
void deletePoint(int nWall);

void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* zt = NULL, int* zb = NULL, char flags = 0x07);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, char flags = 0x07);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, int* zt, int* zb, char flags = 0x07);
void ceilGetEdgeZ(int nSector, int* zBot, int* zTop);
void floorGetEdgeZ(int nSector, int* zBot, int* zTop);
void setCstat(BOOL enable, short* pStat, int nStat);
char fixXSprite(int nID);
char fixXSector(int nID);
char fixXWall(int nID);
int fixWallLoops(void);

int worldSprCallFunc(HSPRITEFUNC pFunc, int nData = 0);
int collectWallsOfNode(IDLIST* pList, int nWall, char flags = 0x00);
char collectOuterWalls(int nWall, IDLIST* pLoop);
int isClockDir(POINT2D* p, int c);
int isClockDir(walltype* p, int c, char dragit = 1);
void flipPoints(POINT2D* p, int c);
void rotatePoints(POINT2D* point, int numpoints, int s);
int nextSpriteAt(int x, int y, int *prv);

char sectorToolDisableAll(char alsoFreeDraw);
char sectorToolEnable(int nType, int nData);
char sectorToolDlgLauncher();

inline uint32_t getlenbyrep(int32_t len, int32_t repeat)
{
    return (uint32_t)((repeat > 0) ? divscale12(len, repeat) : len<<12);
}

inline void fixxrepeat(int nWall, int lenrepquot)
{
    if (lenrepquot)
        wall[nWall].xrepeat = ClipRange(((getWallLength(nWall)<<12) + (1<<11)) / lenrepquot, 1, 255);
}

int GetWallZPeg(int nWall);
int AutoAlignWalls(int nWall0, char flags = 0x01);
char AlignWalls(int w0, int z0, int w1, int z1, char doxpan);
int AutoAlignSectors(int nStart, int nFor, IDLIST* pList);
int ED32_Inside(int x, int y, short nSect);

XSPRITE* pathMarkerFind(XSECTOR* pXSect, XSPRITE *pXMark, IDLIST* pDone, char dir);
inline XSPRITE* pathMarkerFind(XSECTOR* pXSect, XSPRITE *pXMark, char dir)
{
    return pathMarkerFind(pXSect, pXMark, NULL, dir);
}
XSECTOR* pathMarkerFindSector(XSPRITE *pXMark);
void pathMarkerChangeData(int nOld, int nNew, char dir = 1);
void pathChop(XSPRITE* xa, int nAng);


// Save & Load contents of the sector
struct SECTORSAVE
{
    VOIDLIST wList, wxList;
    VOIDLIST sList, sxList;
    sectortype sect;
    XSECTOR xsect;
    int sectID;
    
    SECTORSAVE(int nSect = -1)
    {
        wList.Init(sizeof(walltype));
        wxList.Init(sizeof(XWALL));
        
        sList.Init(sizeof(spritetype));
        sxList.Init(sizeof(XSPRITE));
        
        sectID = -1;
        
        if (nSect >= 0)
            Save(nSect);
    }
    
    int Save(int nSect)
    {
        int i, s, e, c;
        getSectorWalls(nSect, &s, &e);
        walltype loopsep; loopsep.point2 = -1;
        
        sectID = nSect;
        
        while(s <= e)
        {
            // Collect walls as loops. Cannot skip loop
            // length because point2 order may
            // be incorrect.
            
            i = s;
            c = 0;
            
            do
            {
                if (!wList.Exists(&wall[i]))
                {
                    wList.Add(&wall[i]);
                    if (wall[i].extra > 0)
                        wxList.Add(&xwall[wall[i].extra]);
                    
                    c++;
                }
                
                i = wall[i].point2;
            }
            while(i != s && c < kMaxWalls);
            
            if (c)
            {
                if (c >= kMaxWalls)
                    return -1;
                
                wList.Add(&loopsep); // insert loop separator
            }
            
            s++;
        }
        
        for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
        {
            sList.Add(&sprite[i]);
            if (sprite[i].extra > 0)
                sxList.Add(&xsprite[sprite[i].extra]);
        }
        
        for (i = headspritestat[kStatMarker]; i >= 0; i = nextspritestat[i])
        {
            if (sprite[i].owner != nSect || sList.Exists(&sprite[i]))
                continue;
            
            sList.Add(&sprite[i]);
            if (sprite[i].extra > 0)
                sxList.Add(&xsprite[sprite[i].extra]);
        }
        
        memcpy(&sect, &sector[nSect], sizeof(sect));
        if (sector[nSect].extra > 0)
            memcpy(&xsect, &xsector[sector[nSect].extra], sizeof(xsect));
        
        return 0;
    }
    
    int Load()
    {
        int nSect = -1, nXSect = sect.extra;
        int i, j, s;
        
        walltype *ws, *w1, *w2; XWALL *xw;
        spritetype *sp; XSPRITE *xsp;
        
        if (numsectors + 1 >= kMaxSectors)
            return -1;
        
        if (numwalls + sect.wallnum >= kMaxWalls)
            return -2;
        
        ws = (walltype*)wList.First();
        sect.extra = -1;
        
        xw = (XWALL*)wxList.First();
        
        // Insert sector and all of it's loops
        for (w1 = ws; wList.Valid(w1); w1++)
        {
            // get loop length
            for (w2 = w1, i = 0; wList.Valid(w2) && w2->point2 >= 0; w2++, i++);
            
            if ((s = insertLoopEx(nSect, w1, i, ILOOP_DIR_FW|ILOOP_NO_FLIP|ILOOP_NO_SET_FW, &sect)) >= 0)
            {
                if (nSect < 0)
                    nSect = sectorofwall(s);
                
                j = s;
                do
                {
                    if (numxwalls >= kMaxXWalls)
                        break;
                    
                    if (wall[j].extra > 0)
                    {
                        i = dbInsertXWall(j);
                        memcpy(&xwall[i], xw, sizeof(xwall[0]));
                        xwall[i].reference = j;
                        xw++;
                    }
                    
                    j = wall[j].point2;
                }
                while(j != s);
            }
            
            w1 = w2;
        }
        
        // Add xsector info
        if (nXSect > 0 && numxsectors < kMaxSectors)
        {
            j = dbInsertXSector(nSect);
            memcpy(&xsector[j], &xsect, sizeof(xsector[0]));
            xsector[j].reference = nSect;
        }
        
        if (sList.Length() > 0)
        {
            // Insert sprites & xsprites
            sp = (spritetype*)sList.First();
            xsp  = (XSPRITE*)sxList.First();
            while(numsprites < kMaxSprites && sList.Valid(sp))
            {
                i = InsertSprite(nSect, sp->statnum);
                memcpy(&sprite[i], sp, sizeof(sprite[0]));
                sprite[i].sectnum = nSect;
                sprite[i].index   = i;
                sprite[i].extra   = -1;
                
                if ((sp->cstat & 48) == 48)
                    spriteSetSlope(i, spriteGetSlope(i));
                
                if (sp->statnum == kStatMarker && sp->owner == sectID)
                {
                    sprite[i].owner = nSect;
                    if (sector[nSect].extra > 0)
                    {
                        // update markers that sector owns
                        XSECTOR* pXSect = &xsector[sector[nSect].extra];
                        if (pXSect->marker0 == sp->index) pXSect->marker0 = i;
                        if (pXSect->marker1 == sp->index) pXSect->marker1 = i;
                    }
                }
                
                if (sp->extra > 0 && numxsprites < kMaxSprites)
                {
                    j = dbInsertXSprite(i);
                    memcpy(&xsprite[j], xsp, sizeof(xsprite[0]));
                    xsprite[j].reference = i;
                    xsp++;
                }
                
                sp++;
            }
        }
                
        // Restore first wall for slope
        if ((i = findWallAtPos(nSect, ws->x, ws->y)) >= 0)
            setFirstWall(nSect, i);
        
        return nSect;
    }
    
    inline int NumSprites() { return sList.Length(); }
    inline int NumWalls()   { return sect.wallnum;   }
};

#endif