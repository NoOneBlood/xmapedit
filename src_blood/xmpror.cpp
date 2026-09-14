/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of mirrors.cpp adapted for level editor's Preview Mode
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
#include "xmpror.h"
#include "xmpmaped.h"
#include "xmpview.h"
#include "xmparted.h"
#include "tile.h"
#include "nnexts.h"

#define kMirrorPicStartVanilla          4080
#define kMirrorPicEndVanilla            kMirrorPicStartVanilla + 16

struct MIRROR
{
    uint8_t type, flags;
    uint16_t id, thisID;
    uint16_t picStart, picEnd, picOld;
    uint8_t hNeighID, vNeighID;
    POINT3D ofs;
};

struct RORCAM
{
    int32_t x, y, z, d;
    uint8_t index;
};

uint16_t mirrorcnt = 0;
uint16_t mirrorPicStart = 0;
uint16_t mirrorPicEnd   = 0;

static int16_t mirrorsector, mirrorwall[4];
static MIRROR mirror[kMaxROR];
static RORCAM vcam[kMaxROR];
static RORCAM hcam[kMaxROR];
static uint8_t list[kMaxROR];

short mirrorPicWidth, mirrorPicHeight;

// lower, upper
BYTE gStackDB[4][2] =
{
    {kMarkerLowLink,    kMarkerUpLink},
    {kMarkerLowWater,   kMarkerUpWater},
    {kMarkerLowGoo,     kMarkerUpGoo},
    {kMarkerLowStack,   kMarkerUpStack},
};

// functions for ROR drawing
//////////////////////////////////
void RestoreMirrorPic()
{
    tilesizx[kMirrorTile] = mirrorPicWidth;
    tilesizy[kMirrorTile] = mirrorPicHeight;
}

void ClearMirrorPic()
{
    mirrorPicWidth  = tilesizx[kMirrorTile];
    mirrorPicHeight = tilesizy[kMirrorTile];
    tilesizx[kMirrorTile] = 0;
    tilesizy[kMirrorTile] = 0;
}

int qsSortByDist(RORCAM *a, RORCAM *b)         { return a->d - b->d; }
inline char IsMirrorTile(int nTile)            { return irngok(nTile, mirrorPicStart, mirrorPicEnd); }
inline void tileDeleteRange(int s, int e)      { while (--e >= s) tileFreeTile(e); }
inline int ROR_GetOther(int nRor)              { return (mirror[nRor].type == OBJ_FLOOR) ? ++nRor : --nRor; }

void ROR_ClearGotPic(int n)
{
    int i = mirror[n].picEnd;
    while (--i >= mirror[n].picStart)
        ClearBitString(gotpic, i);
}

void ROR_ClearGotPic(void)
{
    int i = mirrorPicEnd;
    while (--i >= mirrorPicStart)
        ClearBitString(gotpic, i);
}

char ROR_TestGotPic(int n)
{
    int i = mirror[n].picEnd;
    while (--i >= mirror[n].picStart)
    {
        if (TestBitString(gotpic, i))
            return 1;
    }

    return 0;
}

void ROR_SetGotPic(int n)
{
    int i = mirror[n].picEnd;
    while (--i >= mirror[n].picStart)
        SetBitString(gotpic, i);
}

int ROR_FindBySector(int nSect, int nType)
{
    int i = mirrorcnt;
    while (--i >= 0 && (mirror[i].type != nType || mirror[i].id != nSect));
    return i;
}

int getDistToSect(int nSect, int x, int y)
{
    int s, e, d, nDist = 0x7FFFFFFF;
    int wx, wy;

    getSectorWalls(nSect, &s, &e);
    while(s <= e)
    {
        getclosestpointonwall(x, y, s, &wx, &wy);
        if ((d = approxDist(x - wx, y - wy)) < nDist)
            nDist = d;

        s++;
    }

    return nDist;

}

void ROR_CollectNeighborsH(int nStart, uint8_t *list, int* num)
{
    MIRROR* pRor = &mirror[nStart];
    int s, e, n, i;

    list[*num] = nStart; *num = *num + 1;
    getSectorWalls(nStart, &s, &e);

    while(s <= e) // collect all the matching sectors while not separated
    {
        if ((n = wall[s].nextsector) >= 0)
        {
            if ((n = ROR_FindBySector(n, pRor->type)) >= 0)
            {
                i = *num;
                while (--i >= 0 && list[i] != n);
                if (i < 0) ROR_CollectNeighborsH(n, list, num);
            }
        }

        s++;
    }
}

void ROR_CollectNeighborsV(int nStart, uint8_t* list, int* num)
{
    MIRROR *pRor = &mirror[nStart];
    short* linkArr = (pRor->type == OBJ_FLOOR) ? gUpperLink : gLowerLink;
    int nType, nSpr, s, n, t;

    nType = pRor->type, s = n = pRor->id;

    while( 1 )
    {
        t = nStart;
        do
        {
            pRor = &mirror[t];
            if (pRor->type == nType && pRor->id == n)
            {
                list[*num] = t;
                *num = *num + 1;
                break;
            }

            t = IncRotate(t, mirrorcnt);
        }
        while(t != nStart);

        if ((nSpr = linkArr[n]) < 0)
            break;

        n = sprite[nSpr].owner;
        n = sprite[n].sectnum;

        if (n < 0 || n == s)
            break;
    }
}

char IsRorSector(int nSect, int stat)
{
    if (stat == OBJ_FLOOR)
    {
        if (rngok(sector[nSect].floorpicnum, mirrorPicStart, mirrorPicEnd))     return (sector[nSect].floorstat & kSectTranslucR) ? 2 : 1;
        else if (sector[nSect].floorpicnum == kMirrorTile)                      return 1;
        else if ((sector[nSect].floorstat & kSectTranslucR) != 0)               return 2;
        else                                                                    return 0;
    }
    else if (rngok(sector[nSect].ceilingpicnum, mirrorPicStart, mirrorPicEnd))  return (sector[nSect].ceilingstat & kSectTranslucR) ? 2 : 1;
    else if (sector[nSect].ceilingpicnum == kMirrorTile)                        return 1;
    else if ((sector[nSect].ceilingstat & kSectTranslucR) != 0)                 return 2;
    else                                                                        return 0;
}

static int CreateMirrorPic(MIRROR* pFor, int nStart, int16_t* objTile)
{
    int32_t nObjTile = *objTile;
    PICANM* pnm = &panm[nObjTile];
    int32_t wh, hg, o;

    pFor->picEnd = pFor->picStart = pFor->picOld = nObjTile;
    pFor->picEnd += pnm->frames + 1;

    if (nStart < 0)
        return 0;

    o = 0;
    if (pnm->frames)
        o = (pnm->type == 3) ? -pnm->frames : pnm->frames;

    *objTile = nStart;
    pFor->picEnd = pFor->picStart = nStart;
    pFor->picEnd += pnm->frames + 1;

    if (o < 0) // backwards animation...
    {
        o = klabs(o);
        nObjTile -= o;
        *objTile += o;
    }

    while (o-- >= 0)
    {
        if ((wh = tilesizx[nObjTile]) > 0 && (hg = tilesizy[nObjTile]) > 0)
        {
            if (tileAllocTile(nStart, wh, hg) && tileLoadTile(nObjTile))
            {
                Bmemmove((void*)waloff[nStart], (void*)waloff[nObjTile], wh * hg);
                Bmemmove(&picanm[nStart], &picanm[nObjTile], sizeof(picanm[0]));
                Bmemmove(&surfType[nStart], &surfType[nObjTile], sizeof(surfType[0]));
            }
        }

        nObjTile++, nStart++;
    }

    return pFor->picEnd - pFor->picStart;
}

static void InitMirrorSector(void)
{
    // Create a room to translate the mirror
    mirrorsector = numsectors;
    for (int i = 0; i < 4; i++)
    {
        mirrorwall[i]                   = numwalls + i;
        wall[mirrorwall[i]].picnum      = kMirrorTile;
        wall[mirrorwall[i]].overpicnum  = kMirrorTile;
        wall[mirrorwall[i]].cstat       = 0;
        wall[mirrorwall[i]].nextsector  = -1;
        wall[mirrorwall[i]].nextwall    = -1;
        wall[mirrorwall[i]].point2      = numwalls + i + 1;
    }

    wall[mirrorwall[3]].point2          = mirrorwall[0];
    sector[mirrorsector].ceilingpicnum  = kMirrorTile;
    sector[mirrorsector].floorpicnum    = kMirrorTile;
    sector[mirrorsector].wallptr        = mirrorwall[0];
    sector[mirrorsector].wallnum        = 4;
}

static int MirrorPicsInit(int nRange)
{
    MIRROR* pRor;
    int nStart, i;
    int r;

    tileDeleteRange(kMirrorPicStartVanilla, kMirrorPicEndVanilla);                      // compatibility
    if (mirrorPicEnd > mirrorPicStart) tileDeleteRange(mirrorPicStart, mirrorPicEnd);   // previous session?

    mirrorPicStart = mirrorPicEnd = 0;
    if (nRange <= 0)
        return -1;

    if ((nStart = tileSearchFreeRange(nRange)) >= 0)
    {
        mirrorPicEnd = mirrorPicStart = nStart;
        mirrorPicEnd += nRange;
    }
    else
    {
        scrSetLogMessage("Not enough range of free tiles for mirrors. Required range = %d.", nRange);
    }

    r = nStart;
    i = mirrorcnt;
    while (--i >= 0)
    {
        pRor = &mirror[i];

        switch (pRor->type)
        {
            case OBJ_WALL:
                if (wall[pRor->id].type == kWallStack)
                    nStart += CreateMirrorPic(pRor, nStart, &wall[pRor->thisID].overpicnum);
                else
                    nStart += CreateMirrorPic(pRor, nStart, &wall[pRor->thisID].picnum);
                break;
            case OBJ_FLOOR:
                nStart += CreateMirrorPic(pRor, nStart, &sector[pRor->thisID].floorpicnum);
                break;
            case OBJ_CEILING:
                nStart += CreateMirrorPic(pRor, nStart, &sector[pRor->thisID].ceilingpicnum);
                break;
        }
    }

    return r;
}

void InitMirrors(void)
{
    walltype* pWall; uint8_t done[kMaxROR];
    int i, j, k, nLinkA, nLinkB;
    char rorTypeA, rorTypeB;
    int nRange = 0;

    ClearMirrorPic();

    Bmemset(mirror, 0, sizeof(mirror));
    mirrorsector = -1;
    mirrorcnt = 0;

    i = numwalls; // Prepare wall mirrors and stacks
    while(--i >= 0 && mirrorcnt < kMaxROR)
    {
        pWall = &wall[i];
        if (pWall->overpicnum == kMirrorTile && pWall->extra > 0 && pWall->type == kWallStack)
        {
            j = numwalls;
            while (--j >= 0)
            {
                if (j == i || wall[j].extra <= 0) continue;
                else if (wall[j].type != pWall->type) continue;
                else if (xwall[wall[j].extra].data != xwall[pWall->extra].data) continue;

                pWall->cstat               |= kWallOneWay;
                pWall->hitag                = j;
                wall[j].hitag               = i;

                mirror[mirrorcnt].type      = OBJ_WALL;
                mirror[mirrorcnt].thisID    = i;
                mirror[mirrorcnt].id        = j;
                mirrorcnt++;
                nRange++;
                break;
            }

            if (j < 0)
                scrSetLogMessage("Wall #%d has no matching wall link! (data = %d)", i, xwall[pWall->extra].data);
        }
        else if (pWall->picnum == kMirrorTile)
        {
            pWall->cstat               |= kWallOneWay;
            pWall->overpicnum           = kMirrorTile;

            mirror[mirrorcnt].type      = OBJ_WALL;
            mirror[mirrorcnt].thisID    = i;
            mirror[mirrorcnt].id        = i;
            mirrorcnt++;
            nRange++;
        }
    }

    if (mirrorcnt > 0)
    {
        if (numsectors + 1 >= kMaxSectors || numwalls + 4 >= kMaxWalls)
        {
            scrSetLogMessage("Must have at least %d sectors with %d walls free for mirrors!", 1, 4);
            mirrorcnt = 0; // cancel the wall mirrors
        }

        if (mirrorcnt > 0)
            InitMirrorSector();
    }

    i = numsectors; // Prepare sector stacks
    while(--i >= 0 && mirrorcnt < kMaxROR - 1)
    {
        if ((rorTypeA = IsRorSector(i, OBJ_FLOOR)) <= 0)
            continue;

        if ((nLinkA = gUpperLink[i]) < 0
            || (nLinkB = sprite[nLinkA].owner) < 0)
                continue;

        j = sprite[nLinkB].sectnum;
        if ((rorTypeB = IsRorSector(j, OBJ_CEILING)) <= 0)
            sector[j].ceilingpicnum = kMirrorTile; // force lower sector to be ROR

        mirror[mirrorcnt].type      = OBJ_FLOOR;
        mirror[mirrorcnt].thisID    = i;
        mirror[mirrorcnt].id        = j;

        mirror[mirrorcnt].ofs.x     = sprite[nLinkB].x - sprite[nLinkA].x;
        mirror[mirrorcnt].ofs.y     = sprite[nLinkB].y - sprite[nLinkA].y;
        mirror[mirrorcnt].ofs.z     = sprite[nLinkB].z - sprite[nLinkA].z;

        nRange += panm[sector[i].floorpicnum].frames+1;
        mirrorcnt++;

        mirror[mirrorcnt].type      = OBJ_CEILING;
        mirror[mirrorcnt].thisID    = j;
        mirror[mirrorcnt].id        = i;

        mirror[mirrorcnt].ofs.x     = sprite[nLinkA].x - sprite[nLinkB].x;
        mirror[mirrorcnt].ofs.y     = sprite[nLinkA].y - sprite[nLinkB].y;
        mirror[mirrorcnt].ofs.z     = sprite[nLinkA].z - sprite[nLinkB].z;

        nRange += panm[sector[j].ceilingpicnum].frames+1;
        mirrorcnt++;
    }

    if (MirrorPicsInit(nRange) < 0)
        mirrorcnt = 0; // cancel everything

    i = mirrorcnt;
    Bmemset(done, 0, sizeof(done));
    while (--i >= 0 && mirror[i].type != OBJ_WALL)
    {
        // Grouping splitted ROR sectors
        // for better and faster
        // drawing.

        if (done[i])
            continue;

        j = 0;
        ROR_CollectNeighborsH(i, list, &j);
        for (k = 0; k < j - 1; k++)
            mirror[list[k]].hNeighID = list[k + 1], done[list[k]] = 1;

        mirror[list[k]].hNeighID = list[0], done[list[k]] = 1;
    }

    i = mirrorcnt;
    Bmemset(done, 0, sizeof(done));
    while (--i >= 0 && mirror[i].type != OBJ_WALL)
    {
        // Grouping ceilings and floors
        // for faster access.

        if (done[i])
            continue;

        j = 0;
        ROR_CollectNeighborsV(i, list, &j);
        for (k = 0; k < j - 1; k++)
            mirror[list[k]].vNeighID = list[k + 1], done[list[k]] = 1;

        mirror[list[k]].vNeighID = list[0], done[list[k]] = 1;
    }

    if (gPreviewMode)
        scrSetLogMessage("%d of %d mirrors are in use.", mirrorcnt, kMaxROR);
}


void TranslateMirrorColors(int nShade, int nPalette)
{
    #if USE_POLYMOST
        if (getrendermode() >= 3)
            return;
    #endif

    int x1 = windowx1, y1 = windowy1;
    int x2 = windowx2, y2 = windowy2;
    int y;

    nShade = ClipRange(nShade, 0, NUMPALOOKUPS(1));
    unsigned char *pMap = (unsigned char*)(palookup[nPalette] + shgetpalookup(0, nShade));
    unsigned char *pFrame;

    begindrawing();

    while(x1 < x2)
    {
        y = y1;
        while(y < y2)
        {
            pFrame = (unsigned char*)FRAMEPLACE(x1, y);
            *pFrame = pMap[*pFrame];
            y++;
        }

        x1++;
    }

    enddrawing();
}

static int DoWallMirrors(int x, int y, int z, int a, int horiz)
{
    walltype* pWall; MIRROR* pRor;
    int32_t nSect, nNextW, nNextS;
    int32_t dx, dy;
    int32_t i;

    short ca;

    for (i = 0; i < mirrorcnt && mirror[i].type == OBJ_WALL; i++)
    {
        if (!ROR_TestGotPic(i))
            continue;

        ROR_ClearGotPic(i);

        pRor = &mirror[i];

        pWall = &wall[pRor->id];
        nSect = sectorofwall(pRor->id);

        nNextW = pWall->nextwall;
        nNextS = pWall->nextsector;

        pWall->nextwall = mirrorwall[0];
        pWall->nextsector = mirrorsector;

        wall[mirrorwall[0]].nextwall        = pRor->id;
        wall[mirrorwall[0]].nextsector      = nSect;
        wall[mirrorwall[0]].x               = wall[pWall->point2].x;
        wall[mirrorwall[0]].y               = wall[pWall->point2].y;
        wall[mirrorwall[1]].x               = pWall->x;
        wall[mirrorwall[1]].y               = pWall->y;
        wall[mirrorwall[2]].x               = wall[mirrorwall[1]].x+(wall[mirrorwall[1]].x-wall[mirrorwall[0]].x)*16;
        wall[mirrorwall[2]].y               = wall[mirrorwall[1]].y+(wall[mirrorwall[1]].y-wall[mirrorwall[0]].y)*16;
        wall[mirrorwall[3]].x               = wall[mirrorwall[0]].x+(wall[mirrorwall[0]].x-wall[mirrorwall[1]].x)*16;
        wall[mirrorwall[3]].y               = wall[mirrorwall[0]].y+(wall[mirrorwall[0]].y-wall[mirrorwall[1]].y)*16;

        sector[mirrorsector].floorz         = sector[nSect].floorz;
        sector[mirrorsector].ceilingz       = sector[nSect].ceilingz;

        if (pWall->type == kWallStack)
        {
            dx = x - (wall[pWall->hitag].x-wall[pWall->point2].x);
            dy = y - (wall[pWall->hitag].y-wall[pWall->point2].y);
            ca = a;
        }
        else
        {
            preparemirror(x, y, z, a, horiz, pRor->id, nSect, &dx, &dy, &ca);
        }

        drawrooms(dx, dy, z, ca, horiz, mirrorsector|kMaxSectors);
        viewProcessSprites(dx, dy, z, ca);
        drawmasks();

        if (pWall->type != kWallStack)
            completemirror();

        if (pWall->pal || pWall->shade)
            TranslateMirrorColors(pWall->shade, pWall->pal);

        pWall->nextwall = nNextW;
        pWall->nextsector = nNextS;
        return 1;
    }

    return 0;
}

static int DoRoomOverRoom(int x, int y, int z, short a, short horiz)
{
    // Current limitations:
    // 1. Can't see the wall mirrors/stacks and ROR at the same time.
    // 2. Can't see the RORs of other sectors through RORs.

    static int32_t hdrawcnt, vdrawcnt, i;

    MIRROR *pRor, *pOth; RORCAM* pCam; short *pSecStat, *linkArr;
    int32_t nIndex, t, n, oSectStat, dx, dy, dz;
    int32_t r = 0;

    hdrawcnt = 0;

    i = mirrorcnt;
    while(--i >= 0 && mirror[i].type != OBJ_WALL)
    {
        // First collect all the floors or ceilings
        // we are currently see
        // horizontally.

        if (!ROR_TestGotPic(i))
            continue;

        pCam = &hcam[hdrawcnt];
        pCam->d = 0x7FFFFFFF;

        n = i;
        do
        {
            pRor = &mirror[n];

            if (pCam->d > 0)
            {
                pOth = &mirror[ROR_GetOther(n)];

                if (inside(x, y, pOth->id))
                {
                    pCam->index = n;
                    pCam->d = 0; // Priority
                }
                else if (ROR_TestGotPic(n))
                {
                    if ((t = getDistToSect(pOth->id, x, y)) < pCam->d)
                    {
                        pCam->index = n;
                        pCam->d = t;
                    }
                }
            }

            ROR_ClearGotPic(n); // Must keep clearing for single drawing
            n = pRor->hNeighID;
        }
        while(n != i);

        hdrawcnt++;
    }

    // Sort the collected RORs by distance
    // so that closest to the camera
    // becomes first.

    if (hdrawcnt > 1)
        qsort((void*)hcam, hdrawcnt, sizeof(hcam[0]), (int(*)(const void*,const void*))qsSortByDist);

    while(--hdrawcnt >= 0)
    {
        // Processing from the most far to
        // the closest for better
        // covering.

        vdrawcnt = 0;
        pCam = &hcam[hdrawcnt]; nIndex = pCam->index; pRor = &mirror[nIndex];
        linkArr = (pRor->type == OBJ_FLOOR) ? gUpperLink : gLowerLink;
        n = pRor->id; dx = x, dy = y, dz = z;

        do
        {
            // Keep adding rooms until we reach the most far vertically.
            // For ceilings search to the top and for
            // floors to the bottom.

            t = nIndex;
            do
            {
                pOth = &mirror[t];
                if (pOth->type == pRor->type && pOth->id == n)
                {
                    ROR_ClearGotPic(t);
                    pCam = &vcam[vdrawcnt];
                    pCam->index = t;

                    dx += pOth->ofs.x;
                    dy += pOth->ofs.y;
                    dz += pOth->ofs.z;

                    pCam->x = dx;
                    pCam->y = dy;
                    pCam->z = dz;

                    vdrawcnt++;
                    break;
                }

                t = pOth->vNeighID;
            }
            while (t != nIndex);

            if ((n = linkArr[n]) >= 0)
                n = sprite[n].owner, n = sprite[n].sectnum;
        }
        while (n >= 0 && n != pRor->id);

        r += vdrawcnt;

        while(--vdrawcnt >= 0)
        {
            // Drawing from the most far room to
            // the current for better
            // covering.

            pCam = &vcam[vdrawcnt];
            pRor = &mirror[pCam->index];

            drawrooms(pCam->x, pCam->y, pCam->z, a, horiz, pRor->id | kMaxSectors);
            BackupHover();

            viewProcessSprites(pCam->x, pCam->y, pCam->z, a);
            drawmasks();

            // fix double draw of hovered wall
            if (searchstat == OBJ_MASKED)
                gHovWall = -1;

            pSecStat = (pRor->type == OBJ_CEILING)
                ? &sector[pRor->id].floorstat : &sector[pRor->id].ceilingstat;

            oSectStat = *pSecStat, *pSecStat |= kSectParallax;

            drawmasks();

            *pSecStat = oSectStat;
        }
    }

    return r;
}

char DrawMirrors(int x, int y, int z, int a, int horiz)
{
    if (mirrorsector >= 0 && DoWallMirrors(x, y, z, a, horiz))
    {
        ROR_ClearGotPic();
        return 1;
    }

    if (DoRoomOverRoom(x, y, z, a, horiz))
    {
        ROR_ClearGotPic();
        return 1;
    }

    return 0;
}

char IsRorMarker(int nType)
{
    int i = LENGTH(gStackDB);
    while(--i >= 0)
    {
        if (nType == gStackDB[i][0])    return 1;
        if (nType == gStackDB[i][1])    return 2;
    }

    return 0;
}

// functions to wrap through ROR links
//////////////////////////////////
void warpInit(void)
{
    int i, j, t;
    memset(gUpperLink, -1, sizeof(gUpperLink));
    memset(gLowerLink, -1, sizeof(gLowerLink));
    for (i = 0; i < numsectors; i++)
    {
        for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
        {
            if (sprite[j].extra <= 0)
                continue;

            spritetype* pSpr = &sprite[j];
            switch (pSpr->type) {
                case kMarkerUpLink:
                    gUpperLink[pSpr->sectnum] = j;
                    pSpr->cstat |= 32768;
                    pSpr->cstat &= ~257;
                    break;
                case kMarkerLowLink:
                    gLowerLink[pSpr->sectnum] = j;
                    pSpr->cstat |= 32768;
                    pSpr->cstat &= ~257;
                    break;
                case kMarkerUpWater:
                case kMarkerUpStack:
                case kMarkerUpGoo:
                    gUpperLink[pSpr->sectnum] = j;
                    pSpr->cstat |= 32768;
                    pSpr->cstat &= ~257;
                    pSpr->z = getflorzofslope(pSpr->sectnum, pSpr->x, pSpr->y);
                    break;
                case kMarkerLowWater:
                case kMarkerLowStack:
                case kMarkerLowGoo:
                    gLowerLink[pSpr->sectnum] = j;
                    pSpr->cstat |= 32768;
                    pSpr->cstat &= ~257;
                    pSpr->z = getceilzofslope(pSpr->sectnum, pSpr->x, pSpr->y);
                    break;

            }
        }
    }


    for (i = 0; i < numsectors; i++)
    {
        if (gUpperLink[i] < 0)
            continue;

        spritetype *pSpr = &sprite[gUpperLink[i]];
        if (pSpr->extra <= 0)
            continue;

        t = xsprite[pSpr->extra].data1;
        for (j = 0; j < numsectors; j++)
        {
            if (gLowerLink[j] < 0)
                continue;

            spritetype *pSpr2 = &sprite[gLowerLink[j]];
            if (pSpr2->extra <= 0 || xsprite[pSpr2->extra].data1 != t)
                continue;

            pSpr->owner = gLowerLink[j];
            pSpr2->owner = gUpperLink[i];
        }
    }

    if (gPreviewMode)
    {
        i = numwalls;
        while(--i >= 0)
        {
            walltype* pWallA = &wall[i];
            if (pWallA->extra <= 0 || !irngok(pWallA->type, kWallStack - 1, kWallStack))
                continue;

            j = numwalls;
            while(--j >= 0)
            {
                walltype* pWallB = &wall[j];
                if (j == i || pWallB->extra <= 0 || !irngok(pWallB->type, kWallStack - 1, kWallStack))
                    continue;

                if (xwall[pWallB->extra].data == xwall[pWallA->extra].data)
                {
                    pWallB->hitag = i;
                    pWallA->hitag = j;
                    break;
                }
            }
        }
    }
}

int CheckLinkSector(int *x, int *y, int *z, int* nSector)
{
    int z1, z2;
    int nUpper = gUpperLink[*nSector];
    int nLower = gLowerLink[*nSector];

    if (nUpper >= 0)
    {
        spritetype *pUpper = &sprite[nUpper];
        if (pUpper->statnum >= kMaxStatus)
            return -1;

        z1 = (pUpper->type == kMarkerUpLink) ? pUpper->z : getflorzofslope(*nSector, *x, *y);
        if (z1 <= *z)
        {
            nLower = pUpper->owner;
            if (!rngok(nLower, 0, kMaxSprites))
                return -2;

            spritetype *pLower = &sprite[nLower];
            if (!rngok(pLower->sectnum, 0, kMaxSectors))
                return -3;

            *nSector = pLower->sectnum;
            *x += pLower->x-pUpper->x;
            *y += pLower->y-pUpper->y;

            z2 = (pUpper->type == kMarkerLowLink) ? pLower->z : z2 = getceilzofslope(*nSector, *x, *y);
            *z += z2-z1;

            return pUpper->type;
        }
    }

    if (nLower >= 0)
    {
        spritetype *pLower = &sprite[nLower];
        if (pLower->statnum >= kMaxStatus)
            return -4;

        z1 = (pLower->type == kMarkerLowLink) ? pLower->z : getceilzofslope(*nSector, *x, *y);
        if (z1 >= *z)
        {
            nUpper = pLower->owner;
            if (!rngok(nUpper, 0, kMaxSprites))
                return -5;

            spritetype *pUpper = &sprite[nUpper];
            if (!rngok(pUpper->sectnum, 0, kMaxSectors))
                return -6;

            *nSector = pUpper->sectnum;
            *x += pUpper->x-pLower->x;
            *y += pUpper->y-pLower->y;

            z2 = (pLower->type == kMarkerUpLink) ? pUpper->z : getflorzofslope(*nSector, *x, *y);
            *z += z2-z1;

            return pLower->type;
        }
    }

    return 0;
}


int CheckLinkWall(int *x, int *y, int *z, int* nVar)
{
    #ifdef ENABLE_EXPERIMENTAL_FEATURES
        int x1, y1, x2, y2, z1, z2;
        walltype *pWallA = &wall[*nVar], *pWallB; XWALL *pXWallA, *pXWallB;
        if (pWallA->extra < 0 || !irngok(pWallA->type, kWallStack - 1, kWallStack) || !rngok(pWallA->hitag, 0, numwalls))
            return 0;

        pWallB = &wall[pWallA->hitag];
        if (pWallB->extra < 0 || !irngok(pWallB->type, kWallStack - 1, kWallStack) || pWallB->hitag != *nVar)
            return 0;

        pXWallA = &xwall[pWallA->extra];
        pXWallB = &xwall[pWallB->extra];
        if (pXWallA->locked || pXWallB->locked)
            return 0;

        getWallCoords(pWallA->hitag, &x1, &y1);
        getWallCoords(pWallA->point2, &x2, &y2);

        *x -= x2-x1, *y -= y2-y1;
        *nVar = sectorofwall(pWallA->hitag);
        getzsofslope(*nVar, *x, *y, &z1, &z2);
        *z = ClipRange(*z, z1, z2);

        return pWallB->type;
    #else
        return 0;
    #endif
}


int CheckLink(int *x, int *y, int *z, int *nID, char wallLink)
{
    int px = *x, py = *y, pz = *z, nSect = *nID;
    int nLink, nSpr;

    if (!wallLink)
    {
        if ((nLink = CheckLinkSector(&px, &py, &pz, &nSect)) > 0)
        {
            if (gModernMap && gPreviewMode)
            {
                if ((nSpr = gUpperLink[*nID]) < 0 || sprite[nSpr].type != nLink)
                {
                    if ((nSpr = gLowerLink[*nID]) < 0 || sprite[nSpr].type != nLink)
                        return 0;
                }

                spritetype* pSpr = &sprite[nSpr];
                if (pSpr->flags & kModernTypeFlag1)
                    return 0;
            }

            *x = px, *y = py, *z = pz; *nID = nSect;
            return nLink;
        }

        return 0;
    }

    return CheckLinkWall(x, y, z, nID);
}

int CheckLink(spritetype *pSprite, int nID, char wallLink)
{
    int nLink;
    if ((nLink = CheckLink(&pSprite->x, &pSprite->y, &pSprite->z, &nID, wallLink)) > 0)
    {
        if (nID != pSprite->sectnum)
            ChangeSpriteSect(pSprite->index, nID);

        return nLink;
    }

    return 0;
}

int CheckLinkCamera(int *x, int *y, int *z, int *nID, char wallLink)
{
    return CheckLink(x, y, z, nID, wallLink);
}