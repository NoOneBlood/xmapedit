/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A version of fx.cpp from Nblood adapted for level editor's Preview Mode
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
#include "eventq.h"
#include "gameutil.h"
#include "seq.h"
#include "preview.h"
#include "xmpstub.h"
#include "screen.h"

#define kMaxEffectSprites	512

CFX gFX;

FXDATA gFXData[] = {
    { kCallbackNone, 0, 49, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 50, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 51, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 52, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 44, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 45, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 46, 1, -128, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 42, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 43, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 48, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 60, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 0, 1, 46603, 2048, 480, 2155, 40, 40, 0, -12, 0 },
    { kCallbackNone, 2, 0, 3, 46603, 5120, 480, 2269, 24, 24, 0, -128, 0 },
    { kCallbackNone, 2, 0, 3, 46603, 5120, 480, 1720, 24, 24, 0, -128, 0 },
    { kCallbackNone, 1, 0, 1, 58254, 3072, 480, 2280, 48, 48, 0, -128, 0 },
    { kCallbackNone, 1, 0, 1, 58254, 3072, 480, 3135, 48, 48, 0, -128, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 1024, 480, 3261, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3265, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3269, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3273, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3277, 32, 32, 0, 0, 0 },
    { kCallbackNone, 2, 0, 1, -27962, 8192, 600, 1128, 16, 16, 514, -16, 0 }, // bubble 1
    { kCallbackNone, 2, 0, 1, -18641, 8192, 600, 1128, 12, 12, 514, -16, 0 }, // bubble 2
    { kCallbackNone, 2, 0, 1, -9320, 8192, 600, 1128, 8, 8, 514, -16, 0 }, // bubble 3
    { kCallbackNone, 2, 0, 1, -18641, 8192, 600, 1131, 32, 32, 514, -16, 0 },
    { kCallbackNone, 2, 0, 3, 27962, 4096, 480, 733, 32, 32, 0, -16, 0 },
    { kCallbackNone, 1, 0, 3, 18641, 4096, 120, 2261, 12, 12, 0, -128, 0 },
    { kCallbackNone, 0, 47, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 3328, 480, 2185, 48, 48, 0, 0, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 1024, 480, 2620, 48, 48, 0, 0, 0 },
    { kCallbackNone, 1, 55, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 56, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 57, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 58, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 0, 0, 0, 0, 960, 956, 32, 32, 610, 0, 0 },
    { kCallbackNone, 2, 62, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 63, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 64, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 65, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 66, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 67, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 0, 0, 0, 838, 16, 16, 80, -8, 0 },
    { kCallbackNone, 0, 0, 3, 34952, 8192, 0, 2078, 64, 64, 0, -8, 0 },
    { kCallbackNone, 0, 0, 3, 34952, 8192, 0, 1106, 64, 64, 0, -8, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 3328, 480, 2406, 48, 48, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 46603, 4096, 480, 3511, 64, 64, 0, -128, 0 },
    { kCallbackNone, 0, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 11, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 11, 3, 0, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 30, 3, 0, 0, 0, 0, 40, 40, 80, -8, 0 },
    { kCallbackNone, 2, 0, 3, 27962, 4096, 480, 4023, 32, 32, 0, -16, 0 },
    { kCallbackNone, 2, 0, 3, 27962, 4096, 480, 4028, 32, 32, 0, -16, 0 },
    { kCallbackNone, 2, 0, 0, 0, 0, 480, 926, 32, 32, 610, -12, 0 },
    { kCallbackNone, 1, 70, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
	
	{ kCallbackNone, 1, 0, 3, 58254, 3328, 480, 2185, 48, 48, 0, 0, 0 },
	{ kCallbackNone, 2, 0, 1, 27962*2, 0, 600, 1147, 64, 64, kSprOrigin, 0, 0 },
	{ kCallbackNone, 2, 0, 1, 27962*2, 0, 600, 1160, 64, 64, kSprOrigin, 0, 0 },
};

void CFX::sub_73FB0(int nSprite) {
    if (nSprite < 0 || nSprite >= kMaxSprites)
        return;
    
	previewDelSprite(nSprite);
}

void CFX::sub_73FFC(int nSprite) {
    if (nSprite < 0 || nSprite >= kMaxSprites) 
		return;
    spritetype *pSprite = &sprite[nSprite];
	if (pSprite->extra > 0)
		seqKill(3, pSprite->extra);
		actPostSprite((short)nSprite, kStatFree);
	
}

spritetype * CFX::fxSpawn(FX_ID nFx, int nSector, int x, int y, int z, unsigned int a6)
{
	if (nSector < 0 || nSector >= numsectors)
        return NULL;
    //int nSector2 = nSector;
    //if (!FindSector(x, y, z, &nSector2))
       // return NULL;

    if (nFx < 0 || nFx >= kFXMax)
        return NULL;
    FXDATA *pFX = &gFXData[nFx];
    if (gStatCount[1] >= kMaxEffectSprites) {
		for (int nSprite = headspritestat[kStatFX]; nSprite != -1; nSprite = nextspritestat[nSprite]) {
			if ((sprite[nSprite].flags & 32))
				sub_73FB0(nSprite);
		}
		return NULL;
    }
    spritetype *pSprite = actSpawnSprite(nSector, x, y, z, 1, 0);
	if (!pSprite)
		return NULL;
	
    pSprite->type = nFx;
    pSprite->picnum = pFX->at12;
    pSprite->cstat |= pFX->at16;
    pSprite->shade = pFX->at18;
    pSprite->pal = pFX->at19;
	
	pSprite->yvel = kDeleteReally;
	
	//pSprite->detail = pFX->at1;
    if (pFX->at14 > 0) pSprite->xrepeat = pFX->at14;
    if (pFX->at15 > 0) pSprite->yrepeat = pFX->at15;
	if ((pFX->at4 & 1) && Chance(0x8000)) pSprite->cstat |= 4;
	if ((pFX->at4 & 2) && Chance(0x8000)) pSprite->cstat |= 8;
	if (pFX->at2)
		seqSpawn(pFX->at2, 3, GetXSprite(pSprite->index), -1);
    
    if (a6 == 0) a6 = pFX->ate;
    if (a6) evPost(pSprite->index, 3, ClipLow(a6+BiRandom(a6>>1), 0), kCallbackRemoveSpecial);
	return pSprite;
}

void CFX::fxProcess(void)
{
	
	for (int nSprite = headspritestat[kStatFX]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {

		spritetype *pSprite = &sprite[nSprite];
		if (pSprite->statnum != kStatFX)
			continue;

        short nSector = pSprite->sectnum;
		FXDATA *pFXData = &gFXData[pSprite->type];
        actAirDrag(pSprite, pFXData->ata);
        if (xvel[nSprite])
            pSprite->x += xvel[nSprite]>>12;
        if (yvel[nSprite])
            pSprite->y += yvel[nSprite]>>12;
        if (zvel[nSprite])
            pSprite->z += zvel[nSprite]>>8;
		
		
        // Weird...
        if (xvel[nSprite] || (yvel[nSprite] && pSprite->z >= sector[pSprite->sectnum].floorz))
        {
            updatesector(pSprite->x, pSprite->y, &nSector);
            if (nSector == -1)
            {
                sub_73FFC(nSprite);
                continue;
            }
            if (getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) <= pSprite->z)
            {
                if (pFXData->funcID < 0 || pFXData->funcID >= kCallbackMax)
                {
                    sub_73FFC(nSprite);
                    continue;
                }
                gCallback[pFXData->funcID](nSprite);
                continue;
            }
            if (nSector != pSprite->sectnum)
            {
                ChangeSpriteSect(nSprite, nSector);
            }
        }
        if (xvel[nSprite] || yvel[nSprite] || zvel[nSprite])
        {
            int floorZ, ceilZ;
            getzsofslope(nSector, pSprite->x, pSprite->y, &ceilZ, &floorZ);
            if (ceilZ > pSprite->z && !(sector[nSector].ceilingstat&1))
            {
                sub_73FFC(nSprite);
                continue;
            }
            if (floorZ < pSprite->z)
            {
                if (pFXData->funcID < 0 || pFXData->funcID >= kCallbackMax)
                {
                    sub_73FFC(nSprite);
                    continue;
                }
                gCallback[pFXData->funcID](nSprite);
                continue;
            }
        }
        zvel[nSprite] += pFXData->at6;
    }
}
