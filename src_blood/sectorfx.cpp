/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
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

#include "compat.h"
#include "build.h"
#include "pragmas.h"
#include "common_game.h"
#include "db.h"
#include "trig.h"
#include "sectorfx.h"

unsigned char flicker1[] = {
    0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0,
    1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1
};

unsigned char flicker2[] = {
    1, 2, 4, 2, 3, 4, 3, 2, 0, 0, 1, 2, 4, 3, 2, 0,
    2, 1, 0, 1, 0, 2, 3, 4, 3, 2, 1, 1, 2, 0, 0, 1,
    1, 2, 3, 4, 4, 3, 2, 1, 2, 3, 4, 4, 2, 1, 0, 1,
    0, 0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 2, 3, 4, 3, 2
};

unsigned char flicker3[] = {
    4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 2, 4, 3, 4, 4,
    4, 4, 2, 1, 3, 3, 3, 4, 3, 4, 4, 4, 4, 4, 2, 4,
    4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 1, 0, 1,
    0, 1, 0, 1, 0, 2, 3, 4, 4, 4, 4, 4, 4, 4, 3, 4
};

unsigned char flicker4[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 3, 0, 1, 0, 1, 0, 4, 4, 4, 4, 4, 2, 0,
    0, 0, 0, 4, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1, 4, 3, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0, 0, 0 ,0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0, 0, 0 ,0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0, 0, 0 ,0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0, 0, 0 ,0, 0
};

unsigned char strobe[] = {
    64, 64, 64, 48, 36, 27, 20, 15, 11, 9, 6, 5, 4, 3, 2, 2,
    1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int GetWaveValue(int a, int b, int c)
{
    b &= 2047;
    switch (a)
    {
    case 0:
        return c;
    case 1:
        return (b>>10)*c;
    case 2:
        return (klabs(128-(b>>3))*c)>>7;
    case 3:
        return ((b>>3)*c)>>8;
    case 4:
        return ((255-(b>>3))*c)>>8;
    case 5:
        return (c+mulscale30(c,Sin(b)))>>1;
    case 6:
        return flicker1[b>>5]*c;
    case 7:
        return (flicker2[b>>5]*c)>>2;
    case 8:
        return (flicker3[b>>5]*c)>>2;
    case 9:
        return (flicker4[b>>4]*c)>>2;
    case 10:
        return (strobe[b>>5]*c)>>6;
    case 11:
        if (b*4 > 2048)
            return 0;
        return (c-mulscale30(c, Cos(b*4)))>>1;
    }
    return 0;
}

int shadeCount = 0;
short shadeList[kMaxXSectors];
int panCount = 0;
short panList[kMaxXSectors];

void DoSectorLighting(void)
{
    for (int i = 0; i < shadeCount; i++)
    {
        int nXSector = shadeList[i];
        XSECTOR *pXSector = &xsector[nXSector];
        int nSector = pXSector->reference;
        dassert(sector[nSector].extra == nXSector);
        if (pXSector->shade)
        {
            int v4 = pXSector->shade;
            if (pXSector->shadeFloor)
            {
                sector[nSector].floorshade -= v4;
                if (pXSector->coloredLights)
                {
                    int nTemp = pXSector->floorpal2;
                    pXSector->floorpal2 = sector[nSector].floorpal;
                    sector[nSector].floorpal = nTemp;
                }
            }
            if (pXSector->shadeCeiling)
            {
                sector[nSector].ceilingshade -= v4;
                if (pXSector->coloredLights)
                {
                    int nTemp = pXSector->ceilpal2;
                    pXSector->ceilpal2 = sector[nSector].ceilingpal;
                    sector[nSector].ceilingpal = nTemp;
                }
            }
            if (pXSector->shadeWalls)
            {
                int nStartWall = sector[nSector].wallptr;
                int nEndWall = nStartWall + sector[nSector].wallnum;
                for (int j = nStartWall; j < nEndWall; j++)
                {
                    wall[j].shade -= v4;
                    if (pXSector->coloredLights)
                    {
                        wall[j].pal = sector[nSector].floorpal;
                    }
                }
            }
            pXSector->shade = 0;
        }
        if (pXSector->shadeAlways || pXSector->busy)
        {
            int t1 = pXSector->shadeWave;
            int t2 = pXSector->amplitude;
            if (!pXSector->shadeAlways && pXSector->busy)
            {
                t2 = mulscale16(t2, pXSector->busy);
            }
            int v4 = GetWaveValue(t1, pXSector->shadePhase*8+pXSector->shadeFreq*(int)totalclock, t2);
            if (pXSector->shadeFloor)
            {
                sector[nSector].floorshade = ClipRange(sector[nSector].floorshade+v4, -128, 127);
                if (pXSector->coloredLights && v4 != 0)
                {
                    int nTemp = pXSector->floorpal2;
                    pXSector->floorpal2 = sector[nSector].floorpal;
                    sector[nSector].floorpal = nTemp;
                }
            }
            if (pXSector->shadeCeiling)
            {
                sector[nSector].ceilingshade = ClipRange(sector[nSector].ceilingshade+v4, -128, 127);
                if (pXSector->coloredLights && v4 != 0)
                {
                    int nTemp = pXSector->ceilpal2;
                    pXSector->ceilpal2 = sector[nSector].ceilingpal;
                    sector[nSector].ceilingpal = nTemp;
                }
            }
            if (pXSector->shadeWalls)
            {
                int nStartWall = sector[nSector].wallptr;
                int nEndWall = nStartWall + sector[nSector].wallnum;
                for (int j = nStartWall; j < nEndWall; j++)
                {
                    wall[j].shade = ClipRange(wall[j].shade+v4, -128, 127);
                    if (pXSector->coloredLights && v4 != 0)
                    {
                        wall[j].pal = sector[nSector].floorpal;
                    }
                }
            }
            pXSector->shade = v4;
        }
    }
}

void UndoSectorLighting(void)
{
    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            XSECTOR *pXSector = &xsector[nXSector];
            if (pXSector->shade)
            {
                int v4 = pXSector->shade;
                if (pXSector->shadeFloor)
                {
                    sector[i].floorshade -= v4;
                    if (pXSector->coloredLights)
                    {
                        int nTemp = pXSector->floorpal2;
                        pXSector->floorpal2 = sector[i].floorpal;
                        sector[i].floorpal = nTemp;
                    }
                }
                if (pXSector->shadeCeiling)
                {
                    sector[i].ceilingshade -= v4;
                    if (pXSector->coloredLights)
                    {
                        int nTemp = pXSector->ceilpal2;
                        pXSector->ceilpal2 = sector[i].ceilingpal;
                        sector[i].ceilingpal = nTemp;
                    }
                }
                if (pXSector->shadeWalls)
                {
                    int nStartWall = sector[i].wallptr;
                    int nEndWall = nStartWall + sector[i].wallnum;
                    for (int j = nStartWall; j < nEndWall; j++)
                    {
                        wall[j].shade -= v4;
                        if (pXSector->coloredLights)
                        {
                            wall[j].pal = sector[i].floorpal;
                        }
                    }
                }
                pXSector->shade = 0;
            }
        }
    }
}

short wallPanList[kMaxXWalls];
int wallPanCount;

void DoSectorPanning(void)
{
    for (int i = 0; i < panCount; i++)
    {
        int nXSector = panList[i];
        XSECTOR *pXSector = &xsector[nXSector];
        int nSector = pXSector->reference;
        dassert(nSector >= 0 && nSector < kMaxSectors);
        sectortype *pSector = &sector[nSector];
        dassert(pSector->extra == nXSector);
        if (pXSector->panAlways || pXSector->busy)
        {
            int angle = pXSector->panAngle+1024;
            int speed = pXSector->panVel<<10;
            if (!pXSector->panAlways && (pXSector->busy&0xffff))
                speed = mulscale16(speed, pXSector->busy);

            if (pXSector->panFloor) // Floor
            {
                int nTile = pSector->floorpicnum;
                int px = (pSector->floorxpanning<<8)+pXSector->floorXPanFrac;
                int py = (pSector->floorypanning<<8)+pXSector->floorYPanFrac;
                if (pSector->floorstat&64)
                    angle -= 512;
                int xBits = (picsiz[nTile]&15)-((pSector->floorstat&8)!=0);
                px += mulscale30(speed<<2, Cos(angle))>>xBits;
                int yBits = (picsiz[nTile]/16)-((pSector->floorstat&8)!=0);
                py -= mulscale30(speed<<2, Sin(angle))>>yBits;
                pSector->floorxpanning = px>>8;
                pSector->floorypanning = py>>8;
                pXSector->floorXPanFrac = px&255;
                pXSector->floorYPanFrac = py&255;
            }
            if (pXSector->panCeiling) // Ceiling
            {
                int nTile = pSector->ceilingpicnum;
                int px = (pSector->ceilingxpanning<<8)+pXSector->ceilXPanFrac;
                int py = (pSector->ceilingypanning<<8)+pXSector->ceilYPanFrac;
                if (pSector->ceilingstat&64)
                    angle -= 512;
                int xBits = (picsiz[nTile]&15)-((pSector->ceilingstat&8)!=0);
                px += mulscale30(speed<<2, Cos(angle))>>xBits;
                int yBits = (picsiz[nTile]/16)-((pSector->ceilingstat&8)!=0);
                py -= mulscale30(speed<<2, Sin(angle))>>yBits;
                pSector->ceilingxpanning = px>>8;
                pSector->ceilingypanning = py>>8;
                pXSector->ceilXPanFrac = px&255;
                pXSector->ceilYPanFrac = py&255;
            }
        }
    }
    for (int i = 0; i < wallPanCount; i++)
    {
        int nXWall = wallPanList[i];
        XWALL *pXWall = &xwall[nXWall];
        int nWall = pXWall->reference;
        dassert(wall[nWall].extra == nXWall);
        if (pXWall->panAlways || pXWall->busy)
        {
            int psx = pXWall->panXVel<<10;
            int psy = pXWall->panYVel<<10;
            if (!pXWall->panAlways && (pXWall->busy & 0xffff))
            {
                psx = mulscale16(psx, pXWall->busy);
                psy = mulscale16(psy, pXWall->busy);
            }
            int nTile = wall[nWall].picnum;
            int px = (wall[nWall].xpanning<<8)+pXWall->xpanFrac;
            int py = (wall[nWall].ypanning<<8)+pXWall->ypanFrac;
            px += (psx<<2)>>((uint8_t)picsiz[nTile]&15);
            py += (psy<<2)>>((uint8_t)picsiz[nTile]/16);
            wall[nWall].xpanning = px>>8;
            wall[nWall].ypanning = py>>8;
            pXWall->xpanFrac = px&255;
            pXWall->ypanFrac = py&255;
        }
    }
}

void InitSectorFX(void)
{
    shadeCount = 0;
    panCount = 0;
    wallPanCount = 0;
    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            XSECTOR *pXSector = &xsector[nXSector];
            if (pXSector->amplitude)
                shadeList[shadeCount++] = nXSector;
            if (pXSector->panVel)
                panList[panCount++] = nXSector;
        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        int nXWall = wall[i].extra;
        if (nXWall > 0)
        {
            XWALL *pXWall = &xwall[nXWall];
            if (pXWall->panXVel || pXWall->panYVel)
                wallPanList[wallPanCount++] = nXWall;
        }
    }
}