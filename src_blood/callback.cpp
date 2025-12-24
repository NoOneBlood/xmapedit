/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Written by NoOne.
// A lite version of callback.cpp from Nblood adapted for level editor's Preview Mode
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

#include "callback.h"
#include "db.h"
#include "preview.h"
#include "seq.h"

static void Remove(int nSprite)
{
    spritetype *pSprite = &sprite[nSprite];
    evKill(nSprite, 3);
    if (pSprite->extra > 0)
        seqKill(3, pSprite->extra);
    sfxKill3DSound(pSprite);
    
    if (pSprite->statnum < kStatFree)
        DeleteSprite(nSprite);
}

void FlareBurst(int nSprite)
{
    spritetype *pSprite = &sprite[nSprite];
    int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
    int nRadius = 0x55555;
    for (int i = 0; i < 8; i++)
    {
        spritetype *pSpawn = actSpawnSprite(pSprite, 5);
        pSpawn->picnum = 2427;
        pSpawn->shade = -128;
        pSpawn->xrepeat = pSpawn->yrepeat = 32;
        pSpawn->type = kMissileFlareAlt;
        pSpawn->clipdist = 2;
        pSpawn->owner = pSprite->owner;

        int nAngle2 = (i<<11)/8;
        int dx = 0;
        int dy = mulscale30r(nRadius, Sin(nAngle2));
        int dz = mulscale30r(nRadius, -Cos(nAngle2));
        if (i&1)
        {
            dy >>= 1;
            dz >>= 1;
        }
        RotateVector(&dx, &dy, nAngle);
        xvel[pSpawn->index] += dx;
        yvel[pSpawn->index] += dy;
        zvel[pSpawn->index] += dz;
        evPost(pSpawn->index, 3, 960, kCallbackRemove);
    }
    evPost(nSprite, 3, 0, kCallbackRemove);
}

void CounterCheck(int nSector) // 12
{
    if (sector[nSector].type != kSectorCounter) return;
    if (sector[nSector].extra <= 0) return;

    XSECTOR *pXSector = &xsector[sector[nSector].extra];
    int nReq = pXSector->waitTimeA; int nType = pXSector->data; int nCount = 0;
    if (!nType || !nReq) return;

    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite]) {
        if (sprite[nSprite].type == nType) nCount++;
    }

    if (nCount < nReq) {
        evPost(nSector, 6, 5, kCallbackCounterCheck);
        return;
    } else {
        //pXSector->waitTimeA = 0; //do not reset necessary objects counter to zero
        trTriggerSector(nSector, pXSector, kCmdOn);
        pXSector->locked = 1; //lock sector, so it can be opened again later
    }
}

CALLBACK_FUNC gCallback[kCallbackMax] = {
    Remove,             // kCallbackRemove
    callbackUniMissileBurst,
    callbackMakeMissileBlocking,
    FlareBurst,
    CounterCheck,
};
