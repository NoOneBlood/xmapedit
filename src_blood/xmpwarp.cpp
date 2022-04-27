/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of warp.cpp from Nblood adapted for Preview Mode
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

#include "db.h"
#include "common_game.h"
#include "gameutil.h"
#include "misc.h"
#include "xmpstub.h"
#include "xmpwarp.h"

void warpInit(void)
{
    for (int i = 0; i < kMaxSectors; i++)
    {
        gUpperLink[i] = -1;
        gLowerLink[i] = -1;
    }
	
    for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
    {
        if (sprite[nSprite].statnum < kMaxStatus) {
            spritetype *pSprite = &sprite[nSprite];
            int nXSprite = pSprite->extra;
            if (nXSprite > 0)
			{
                XSPRITE *pXSprite = &xsprite[nXSprite];
                switch (pSprite->type) {
                    case kMarkerUpLink:
                        gUpperLink[pSprite->sectnum] = nSprite;
                        pSprite->cstat |= 32768;
                        pSprite->cstat &= ~257;
                        break;
                    case kMarkerLowLink:
                        gLowerLink[pSprite->sectnum] = nSprite;
                        pSprite->cstat |= 32768;
                        pSprite->cstat &= ~257;
                        break;
                    case kMarkerUpWater:
                    case kMarkerUpStack:
                    case kMarkerUpGoo:
                        gUpperLink[pSprite->sectnum] = nSprite;
                        pSprite->cstat |= 32768;
                        pSprite->cstat &= ~257;
                        pSprite->z = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
                        break;
                    case kMarkerLowWater:
                    case kMarkerLowStack:
                    case kMarkerLowGoo:
                        gLowerLink[pSprite->sectnum] = nSprite;
                        pSprite->cstat |= 32768;
                        pSprite->cstat &= ~257;
                        pSprite->z = getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
                        break;
                }
            }
        }
    }

    for (int i = 0; i < kMaxSectors; i++)
    {
        int nSprite = gUpperLink[i];
        if (nSprite >= 0)
        {
            spritetype *pSprite = &sprite[nSprite];
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            int nLink = pXSprite->data1;
            for (int j = 0; j < kMaxSectors; j++)
            {
                int nSprite2 = gLowerLink[j];
                if (nSprite2 >= 0)
                {
                    spritetype *pSprite2 = &sprite[nSprite2];
                    int nXSprite = pSprite2->extra;
                    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
                    XSPRITE *pXSprite2 = &xsprite[nXSprite];
                    if (pXSprite2->data1 == nLink)
                    {
                        pSprite->owner = gLowerLink[j];
                        pSprite2->owner = gUpperLink[i];
                    }
                }
            }
        }
    }
}

int CheckLink(spritetype *pSprite)
{
    int nSector = pSprite->sectnum;
    int nUpper = gUpperLink[nSector];
    int nLower = gLowerLink[nSector];
    if (nUpper >= 0)
    {
        spritetype *pUpper = &sprite[nUpper];
        int z;
        if (pUpper->type == kMarkerUpLink)
            z = pUpper->z;
        else
            z = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
        if (z <= pSprite->z)
        {
            nLower = pUpper->owner;
            dassert(nLower >= 0 && nLower < kMaxSprites);
            spritetype *pLower = &sprite[nLower];
            dassert(pLower->sectnum >= 0 && pLower->sectnum < kMaxSectors);
            ChangeSpriteSect(pSprite->index, pLower->sectnum);
            pSprite->x += pLower->x-pUpper->x;
            pSprite->y += pLower->y-pUpper->y;
            int z2;
            if (pLower->type == kMarkerLowLink)
                z2 = pLower->z;
            else
                z2 = getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
            pSprite->z += z2-z;
            return pUpper->type;
        }
    }
    if (nLower >= 0)
    {
        spritetype *pLower = &sprite[nLower];
        int z;
        if (pLower->type == kMarkerLowLink)
            z = pLower->z;
        else
            z = getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
        if (z >= pSprite->z)
        {
            nUpper = pLower->owner;
            dassert(nUpper >= 0 && nUpper < kMaxSprites);
            spritetype *pUpper = &sprite[nUpper];
            dassert(pUpper->sectnum >= 0 && pUpper->sectnum < kMaxSectors);
            ChangeSpriteSect(pSprite->index, pUpper->sectnum);
            pSprite->x += pUpper->x-pLower->x;
            pSprite->y += pUpper->y-pLower->y;
            int z2;
            if (pUpper->type == kMarkerUpLink)
                z2 = pUpper->z;
            else
                z2 = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
            pSprite->z += z2-z;
            return pLower->type;
        }
    }
    return 0;
}

int CheckLink(int *x, int *y, int *z, int *nSector)
{
    int nUpper = gUpperLink[*nSector];
    int nLower = gLowerLink[*nSector];
    if (nUpper >= 0)
    {
        spritetype *pUpper = &sprite[nUpper];
        int z1;
        if (pUpper->type == kMarkerUpLink)
            z1 = pUpper->z;
        else
            z1 = getflorzofslope(*nSector, *x, *y);
        if (z1 <= *z)
        {
            nLower = pUpper->owner;
            dassert(nLower >= 0 && nLower < kMaxSprites);
            spritetype *pLower = &sprite[nLower];
            dassert(pLower->sectnum >= 0 && pLower->sectnum < kMaxSectors);
            *nSector = pLower->sectnum;
            *x += pLower->x-pUpper->x;
            *y += pLower->y-pUpper->y;
            int z2;
            if (pUpper->type == kMarkerLowLink)
                z2 = pLower->z;
            else
                z2 = getceilzofslope(*nSector, *x, *y);
            *z += z2-z1;
            return pUpper->type;
        }
    }
    if (nLower >= 0)
    {
        spritetype *pLower = &sprite[nLower];
        int z1;
        if (pLower->type == kMarkerLowLink)
            z1 = pLower->z;
        else
            z1 = getceilzofslope(*nSector, *x, *y);
        if (z1 >= *z)
        {
            nUpper = pLower->owner;
            dassert(nUpper >= 0 && nUpper < kMaxSprites);
            spritetype *pUpper = &sprite[nUpper];
            dassert(pUpper->sectnum >= 0 && pUpper->sectnum < kMaxSectors);
            *nSector = pUpper->sectnum;
            *x += pUpper->x-pLower->x;
            *y += pUpper->y-pLower->y;
            int z2;
            if (pLower->type == kMarkerUpLink)
                z2 = pUpper->z;
            else
                z2 = getflorzofslope(*nSector, *x, *y);
            *z += z2-z1;
            return pLower->type;
        }
    }
    return 0;
}