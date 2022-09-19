/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
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
#include "build.h"
#include "common_game.h"
#include "db.h"
#include "eventq.h"
#include "seq.h"
#include "gameutil.h"
#include "tile.h"
#include "preview.h"

#define kMaxClients 256
#define kMaxSequences 1024

static ACTIVE activeList[kMaxSequences];
static int activeCount = 0;
static int nClients = 0;
static void(*clientCallback[kMaxClients])(int, int);

int seqRegisterClient(void(*pClient)(int, int))
{
    dassert(nClients < kMaxClients);
    clientCallback[nClients] = pClient;
    return nClients++;
}

SEQINST siWall[kMaxXWalls];
SEQINST siCeiling[kMaxXSectors];
SEQINST siFloor[kMaxXSectors];
SEQINST siSprite[kMaxXSprites];
SEQINST siMasked[kMaxXWalls];

void UpdateSprite(int nXSprite, SEQFRAME *pFrame)
{
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    int nSprite = xsprite[nXSprite].reference;
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    dassert(pSprite->extra == nXSprite);
    if (pSprite->flags & 2)
    {
		if (tilesizy[pSprite->picnum] != tilesizy[seqGetTile(pFrame)] || panm[pSprite->picnum].ycenter != panm[seqGetTile(pFrame)].ycenter
            || (pFrame->yrepeat && pFrame->yrepeat != pSprite->yrepeat))
            pSprite->flags |= 4;
    }
    pSprite->picnum = seqGetTile(pFrame);
    if (pFrame->pal)
        pSprite->pal = pFrame->pal;
    pSprite->shade = pFrame->shade;
    
    int scale = xsprite[nXSprite].scale; // SEQ size scaling
    if (pFrame->xrepeat) {
        if (scale) pSprite->xrepeat = ClipRange(mulscale8(pFrame->xrepeat, scale), 0, 255);
        else pSprite->xrepeat = pFrame->xrepeat;
    }

    if (pFrame->yrepeat) {
        if (scale) pSprite->yrepeat = ClipRange(mulscale8(pFrame->yrepeat, scale), 0, 255);
        else pSprite->yrepeat = pFrame->yrepeat;
    }

	
    if (pFrame->transparent) pSprite->cstat |= 2;
    else  pSprite->cstat &= ~2;
   
    if (pFrame->transparent2) pSprite->cstat |= 512;
    else pSprite->cstat &= ~512;
   
    if (pFrame->blockable)
        pSprite->cstat |= 1;
    else
        pSprite->cstat &= ~1;
    if (pFrame->hittable)
        pSprite->cstat |= 256;
    else
        pSprite->cstat &= ~256;
    if (pFrame->invisible)
        pSprite->cstat |= 32768;
    else
        pSprite->cstat &= (unsigned short)~32768;
    
	if (pFrame->pushable)
        pSprite->cstat |= 4096;
    else
        pSprite->cstat &= ~4096;
    
	if (pFrame->smoke)
        pSprite->flags |= 256;
    else
        pSprite->flags &= ~256;
   
   if (pFrame->autoaim)
        pSprite->flags |= 8;
    else
        pSprite->flags &= ~8;
    
	if (pFrame->xflip)
        pSprite->flags |= 1024;
    else
        pSprite->flags &= ~1024;
   
   if (pFrame->yflip)
        pSprite->flags |= 2048;
    else
        pSprite->flags &= ~2048;
}

void UpdateWall(int nXWall, SEQFRAME *pFrame)
{
    dassert(nXWall > 0 && nXWall < kMaxXWalls);
    int nWall = xwall[nXWall].reference;
    dassert(nWall >= 0 && nWall < kMaxWalls);
    walltype *pWall = &wall[nWall];
    dassert(pWall->extra == nXWall);
    pWall->picnum = seqGetTile(pFrame);
    if (pFrame->pal)
        pWall->pal = pFrame->pal;
    if (pFrame->transparent)
        pWall->cstat |= 128;
    else
        pWall->cstat &= ~128;
    if (pFrame->transparent2)
        pWall->cstat |= 512;
    else
        pWall->cstat &= ~512;
    if (pFrame->blockable)
        pWall->cstat |= 1;
    else
        pWall->cstat &= ~1;
    if (pFrame->hittable)
        pWall->cstat |= 64;
    else
        pWall->cstat &= ~64;
}

void UpdateMasked(int nXWall, SEQFRAME *pFrame)
{
    dassert(nXWall > 0 && nXWall < kMaxXWalls);
    int nWall = xwall[nXWall].reference;
    dassert(nWall >= 0 && nWall < kMaxWalls);
    walltype *pWall = &wall[nWall];
    dassert(pWall->extra == nXWall);
    dassert(pWall->nextwall >= 0);
    walltype *pWallNext = &wall[pWall->nextwall];
    pWall->overpicnum = pWallNext->overpicnum = seqGetTile(pFrame);
    if (pFrame->pal)
        pWall->pal = pWallNext->pal = pFrame->pal;
    if (pFrame->transparent)
    {
        pWall->cstat |= 128;
        pWallNext->cstat |= 128;
    }
    else
    {
        pWall->cstat &= ~128;
        pWallNext->cstat &= ~128;
    }
    if (pFrame->transparent2)
    {
        pWall->cstat |= 512;
        pWallNext->cstat |= 512;
    }
    else
    {
        pWall->cstat &= ~512;
        pWallNext->cstat &= ~512;
    }
    if (pFrame->blockable)
    {
        pWall->cstat |= 1;
        pWallNext->cstat |= 1;
    }
    else
    {
        pWall->cstat &= ~1;
        pWallNext->cstat &= ~1;
    }
    if (pFrame->hittable)
    {
        pWall->cstat |= 64;
        pWallNext->cstat |= 64;
    }
    else
    {
        pWall->cstat &= ~64;
        pWallNext->cstat &= ~64;
    }
}

void UpdateFloor(int nXSector, SEQFRAME *pFrame)
{
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    int nSector = xsector[nXSector].reference;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sectortype *pSector = &sector[nSector];
    dassert(pSector->extra == nXSector);
    pSector->floorpicnum = seqGetTile(pFrame);
    pSector->floorshade = pFrame->shade;
    if (pFrame->pal)
        pSector->floorpal = pFrame->pal;
}

void UpdateCeiling(int nXSector, SEQFRAME *pFrame)
{
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    int nSector = xsector[nXSector].reference;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sectortype *pSector = &sector[nSector];
    dassert(pSector->extra == nXSector);
    pSector->ceilingpicnum = seqGetTile(pFrame);
    pSector->ceilingshade = pFrame->shade;
    if (pFrame->pal)
        pSector->ceilingpal = pFrame->pal;
}

void SEQINST::Update(ACTIVE *pActive)
{
    dassert(frameIndex < pSequence->nFrames);
    switch (pActive->type)
    {
		case 0:
			UpdateWall(pActive->xindex, &pSequence->frames[frameIndex]);
			break;
		case 1:
			UpdateCeiling(pActive->xindex , &pSequence->frames[frameIndex]);
			break;
		case 2:
			UpdateFloor(pActive->xindex, &pSequence->frames[frameIndex]);
			break;
		case 3: 
		{
			UpdateSprite(pActive->xindex, &pSequence->frames[frameIndex]);
			if (pSequence->frames[frameIndex].playSound)
			{
				// by NoOne: add random sound range feature
				int sound = pSequence->nSoundID;
				if (pSequence->frames[frameIndex].soundRange > 0)
					sound += Random(((pSequence->frames[frameIndex].soundRange == 1) ? 2 : pSequence->frames[frameIndex].soundRange));
				
				sfxPlay3DSound(&sprite[xsprite[pActive->xindex].reference], sound, -1, 0);
			}

			// by NoOne: add surfaceSound trigger feature
			spritetype* pSprite = &sprite[xsprite[pActive->xindex].reference];
			if (pSequence->frames[frameIndex].surfaceSound && zvel[pSprite->index] == 0 && xvel[pSprite->index] != 0)
			{
				if (gUpperLink[pSprite->sectnum] >= 0) break; // don't play surface sound for stacked sectors
				int surf = tileGetSurfType(pSprite->sectnum + 0x4000); if (!surf) break;
				static int surfSfxMove[15][4] = {
					/* {snd1, snd2, gameVolume, myVolume} */
					{800,801,80,25},
					{802,803,80,25},
					{804,805,80,25},
					{806,807,80,25},
					{808,809,80,25},
					{810,811,80,25},
					{812,813,80,25},
					{814,815,80,25},
					{816,817,80,25},
					{818,819,80,25},
					{820,821,80,25},
					{822,823,80,25},
					{824,825,80,25},
					{826,827,80,25},
					{828,829,80,25},
				};

				int sndId = surfSfxMove[surf][Random(2)];
				DICTNODE * hRes = gSoundRes.Lookup(sndId, "SFX"); SFX * pEffect = (SFX*)gSoundRes.Load(hRes);
				sfxPlay3DSoundCP(pSprite, sndId, -1, 0, 0, (surfSfxMove[surf][2] != pEffect->relVol) ? pEffect->relVol : surfSfxMove[surf][3]);
			}
			break;
		}
		case 4:
			UpdateMasked(pActive->xindex, &pSequence->frames[frameIndex]);
			break;
    }
	
    if (pSequence->frames[frameIndex].trigger && nCallbackID != -1)
        clientCallback[nCallbackID](pActive->type, pActive->xindex);
}

SEQINST * GetInstance(int nType, int nXIndex)
{
    switch (nType)
    {
    case 0:
        if (nXIndex > 0 && nXIndex < kMaxXWalls) return &siWall[nXIndex];
        break;
    case 1:
        if (nXIndex > 0 && nXIndex < kMaxXSectors) return &siCeiling[nXIndex];
        break;
    case 2:
        if (nXIndex > 0 && nXIndex < kMaxXSectors) return &siFloor[nXIndex];
        break;
    case 3:
        if (nXIndex > 0 && nXIndex < kMaxXSprites) return &siSprite[nXIndex];
        break;
    case 4:
        if (nXIndex > 0 && nXIndex < kMaxWalls) return &siMasked[nXIndex];
        break;
    }
    return NULL;
}

void UnlockInstance(SEQINST *pInst)
{
    dassert(pInst != NULL);
    dassert(pInst->hSeq != NULL);
    dassert(pInst->pSequence != NULL);
    gSysRes.Unlock(pInst->hSeq);
    pInst->hSeq = NULL;
    pInst->pSequence = NULL;
    pInst->isPlaying = 0;
}

void seqSpawn(int nSeq, int nType, int nXIndex, int nCallbackID)
{
    SEQINST *pInst = GetInstance(nType, nXIndex);
    if (!pInst) return;
    
    DICTNODE *hSeq = gSysRes.Lookup(nSeq, "SEQ");
    if (!hSeq)
        ThrowError("Missing sequence #%d", nSeq);

    int i = activeCount;
    if (pInst->isPlaying)
    {
        if (hSeq == pInst->hSeq)
            return;
        UnlockInstance(pInst);
        for (i = 0; i < activeCount; i++)
        {
            if (activeList[i].type == nType && activeList[i].xindex == nXIndex)
                break;
        }
        dassert(i < activeCount);
    }
    Seq *pSeq = (Seq*)gSysRes.Lock(hSeq);
    if (memcmp(pSeq->signature, "SEQ\x1a", 4) != 0)
        ThrowError("Invalid sequence %d", nSeq);
    if ((pSeq->version & 0xff00) != 0x300)
        ThrowError("Sequence %d is obsolete version", nSeq);
    if ((pSeq->version & 0xff) == 0x00)
    {
        for (int i = 0; i < pSeq->nFrames; i++)
            pSeq->frames[i].tile2 = 0;
    }
    pInst->isPlaying = 1;
    pInst->hSeq = hSeq;
    pInst->pSequence = pSeq;
    pInst->nSeq = nSeq;
    pInst->nCallbackID = nCallbackID;
    pInst->timeCount = pSeq->ticksPerFrame;
    pInst->frameIndex = 0;
    if (i == activeCount)
    {
        dassert(activeCount < kMaxSequences);
        activeList[activeCount].type = nType;
        activeList[activeCount].xindex = nXIndex;
        activeCount++;
    }
    pInst->Update(&activeList[i]);
}

void seqKill(int nType, int nXIndex)
{
    SEQINST *pInst = GetInstance(nType, nXIndex);
    if (!pInst || !pInst->isPlaying)
        return;
    int i;
    for (i = 0; i < activeCount; i++)
    {
        if (activeList[i].type == nType && activeList[i].xindex == nXIndex)
            break;
    }
    dassert(i < activeCount);
    activeCount--;
    activeList[i] = activeList[activeCount];
    pInst->isPlaying = 0;
    UnlockInstance(pInst);
}

void seqKillAll(void)
{
    for (int i = 0; i < kMaxXWalls; i++)
    {
        if (siWall[i].isPlaying)
            UnlockInstance(&siWall[i]);
        if (siMasked[i].isPlaying)
            UnlockInstance(&siMasked[i]);
    }
    for (int i = 0; i < kMaxXSectors; i++)
    {
        if (siCeiling[i].isPlaying)
            UnlockInstance(&siCeiling[i]);
        if (siFloor[i].isPlaying)
            UnlockInstance(&siFloor[i]);
    }
    for (int i = 0; i < kMaxXSprites; i++)
    {
        if (siSprite[i].isPlaying)
            UnlockInstance(&siSprite[i]);
    }
    activeCount = 0;
}

int seqGetStatus(int nType, int nXIndex)
{
    SEQINST *pInst = GetInstance(nType, nXIndex);
    if (pInst && pInst->isPlaying)
        return pInst->frameIndex;
    return -1;
}

int seqGetID(int nType, int nXIndex)
{
    SEQINST *pInst = GetInstance(nType, nXIndex);
    if (pInst)
        return pInst->nSeq;
    return -1;
}

void seqProcess(int nTicks)
{
    for (int i = 0; i < activeCount; i++)
    {
        SEQINST *pInst = GetInstance(activeList[i].type, activeList[i].xindex);
        Seq *pSeq = pInst->pSequence;
        dassert(pInst->frameIndex < pSeq->nFrames);
        pInst->timeCount -= nTicks;
        while (pInst->timeCount < 0)
        {
            pInst->timeCount += pSeq->ticksPerFrame;
            pInst->frameIndex++;
            if (pInst->frameIndex == pSeq->nFrames)
            {
                if (pSeq->flags & 1)
                    pInst->frameIndex = 0;
                else
                {
                    UnlockInstance(pInst);
                    if (pSeq->flags & 2)
                    {
                        switch (activeList[i].type) {
                        case 3:
                        {
                            int nXSprite = activeList[i].xindex;
                            int nSprite = xsprite[nXSprite].reference;
                            dassert(nSprite >= 0 && nSprite < kMaxSprites);
                            evKill(nSprite, 3);
							previewDelSprite(nSprite);
                            break;
                        }
                        case 4:
                        {
                            int nXWall = activeList[i].xindex;
                            int nWall = xwall[nXWall].reference;
                            dassert(nWall >= 0 && nWall < kMaxWalls);
                            wall[nWall].cstat &= ~(8 + 16 + 32);
                            if (wall[nWall].nextwall != -1)
                                wall[wall[nWall].nextwall].cstat &= ~(8 + 16 + 32);
                            break;
                        }
                        }
                    }
                    activeList[i--] = activeList[--activeCount];
                    break;
                }
            }
            pInst->Update(&activeList[i]);
        }
    }
}

int seqGetTile(SEQFRAME* pFrame)
{
    return pFrame->tile+(pFrame->tile2<<12);
}