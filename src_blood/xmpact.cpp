/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of actor.cpp from Nblood adapted for Preview Mode
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

#include "xmpstub.h"
#include "seq.h"
#include "screen.h"
#include "gameutil.h"
#include "db.h"
#include "aadjust.h"
#include "preview.h"
#include "gui.h"
#include "editor.h"
#include "xmpmisc.h"

POSTPONE gPost[kMaxSprites];
int gPostCount = 0;

short gAffectedSectors[kMaxSectors], gAffectedXWalls[kMaxXWalls];

EXPLODE_INFO explodeInfo[] = { 
	{ 40, 10, 10, 75, 450, 0, 60, 80, 40 }, 
	{ 80, 20, 10, 150, 900, 0, 60, 160, 60 }, 
	{ 120, 40, 15, 225, 1350, 0, 60, 240, 80 }, 
	{ 80, 5, 10, 120, 20, 10, 60, 0, 40 }, 
	{ 120, 10, 10, 180, 40, 10, 60, 0, 80 }, 
	{ 160, 15, 10, 240, 60, 10, 60, 0, 120 }, 
	{ 40, 20, 10, 120, 0, 10, 30, 60, 40 }, 
	{ 80, 20, 10, 150, 800, 5, 60, 160, 60 },
};

MissileType missileInfo[18] = {
	{ 2138, 978670, 512, 40, 40, (char)-16, 16, }, 	// Cleaver
	{ 2427, 3145728, 0, 32, 32, (char)-128, 32, }, 	// Regular flare
	{ 3056, 2796202, 0, 32, 32, (char)-128, 32, }, 	// Tesla alt
	{ 2427, 2446677, 0, 32, 32, (char)-128, 4, }, 	// Flare alt
	{ 0, 1118481, 0, 24, 24, (char)-128, 16, }, 	// Spray flame
	{ 0, 1118481, 0, 32, 32, (char)-128, 32, }, 	// Fireball
	{ 2135, 2796202, 0, 32, 32, (char)-128, 16, }, 	// Tesla regular
	{ 870, 699050, 0, 32, 32, (char)-24, 32, }, 	// EctoSkull
	{ 0, 1118481, 0, 24, 24, (char)-128, 16, }, 	// Hellhound flame
	{ 0, 838860, 0, 16, 16, (char)-16, 16, }, 		// Puke
	{ 0, 838860, 0, 8, 8, (char)0, 16, }, 			// Reserved
	{ 3056, 2097152, 0, 32, 32, (char)-128, 16, }, 	// Stone gargoyle projectile
	{ 0, 2446677, 0, 30, 30, (char)-128, 24, }, 	// Napalm launcher
	{ 0, 2446677, 0, 30, 30, (char)-128, 24, }, 	// Cerberus fireball
	{ 0, 1398101, 0, 24, 24, (char)-128, 16, }, 	// Tchernobog fireball
	{ 2446, 2796202, 0, 32, 32, (char)-128, 16, }, 	// Regular life leech
	{ 3056, 2446677, 0, 16, 16, (char)-128, 16, }, 	// Dropped life leech (enough ammo)
	{ 3056, 1747626, 0, 32, 32, (char)-128, 16, }	// Dropped life leech (no ammo)
};

BOOL actCheckProximity(int x, int y, int z, int nSector, int dist)
{
	int dx = klabs(x - posx) >> 4;
	if (dx < dist)
	{
		int dy = klabs(y - posy) >> 4;
		if (dy < dist)
		{
			int dz = klabs(z - posz) >> 8;
			if ( dz < dist && approxDist(dx, dy) < dist )
			{
				int zTop = posz - kensplayerheight, zBot = posz;
				//GetSpriteExtents(pSprite, &zTop, &zBot);

				if (cansee(posx, posy, posz, cursectnum, x, y, z, nSector) )
					return TRUE;

				if ( cansee(posx, posy, zTop, cursectnum,
					x, y, z, nSector) )
					return TRUE;

				if ( cansee(posx, posy, zBot, cursectnum,
					x, y, z, nSector) )
					return TRUE;
			}
		}
	}
	return FALSE;
}


void actHitcodeToData(int a1, HITINFO *pHitInfo, int *a3, spritetype **a4, XSPRITE **a5, int *a6, walltype **a7, XWALL **a8, int *a9, sectortype **a10, XSECTOR **a11)
{
    dassert(pHitInfo != NULL);
    int nSprite = -1;
    spritetype *pSprite = NULL;
    XSPRITE *pXSprite = NULL;
    int nWall = -1;
    walltype *pWall = NULL;
    XWALL *pXWall = NULL;
    int nSector = -1;
    sectortype *pSector = NULL;
    XSECTOR *pXSector = NULL;
    switch (a1)
    {
    case 3:
    case 5:
        nSprite = pHitInfo->hitsprite;
        dassert(nSprite >= 0 && nSprite < kMaxSprites);
        pSprite = &sprite[nSprite];
        if (pSprite->extra > 0)
            pXSprite = &xsprite[pSprite->extra];
        break;
    case 0:
    case 4:
        nWall = pHitInfo->hitwall;
        dassert(nWall >= 0 && nWall < kMaxWalls);
        pWall = &wall[nWall];
        if (pWall->extra > 0)
            pXWall = &xwall[pWall->extra];
        break;
    case 1:
    case 2:
    case 6:
        nSector = pHitInfo->hitsect;
        dassert(nSector >= 0 && nSector < kMaxSectors);
        pSector = &sector[nSector];
        if (pSector->extra > 0)
            pXSector = &xsector[pSector->extra];
        break;
    }
    if (a3)
        *a3 = nSprite;
    if (a4)
        *a4 = pSprite;
    if (a5)
        *a5 = pXSprite;
    if (a6)
        *a6 = nWall;
    if (a7)
        *a7 = pWall;
    if (a8)
        *a8 = pXWall;
    if (a9)
        *a9 = nSector;
    if (a10)
        *a10 = pSector;
    if (a11)
        *a11 = pXSector;
}

void actImpactMissile(spritetype *pMissile, int hitCode)
{

	int nXMissile = pMissile->extra;
    dassert(nXMissile > 0 && nXMissile < kMaxXSprites);
    XSPRITE *pXMissile = &xsprite[pMissile->extra];
    
    int nSpriteHit = -1; int nWallHit = -1; int nSectorHit = -1;
    spritetype *pSpriteHit = NULL; XSPRITE *pXSpriteHit = NULL;
    walltype *pWallHit = NULL; XWALL *pXWallHit = NULL;
    sectortype *pSectorHit = NULL; XSECTOR *pXSectorHit = NULL;

    actHitcodeToData(hitCode, &gHitInfo, &nSpriteHit, &pSpriteHit, &pXSpriteHit, &nWallHit, &pWallHit, &pXWallHit, &nSectorHit, &pSectorHit, &pXSectorHit);
    switch (pMissile->type) {
        case kMissileLifeLeechRegular:
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileTeslaAlt:
            //sub_51340(pMissile, hitCode);
            switch (hitCode) {
                case 0:
                case 4:
                    if (pWallHit) {
                        spritetype* pFX = gFX.fxSpawn(FX_52, pMissile->sectnum, pMissile->x, pMissile->y, pMissile->z, 0);
                        if (pFX) pFX->ang = (short) ((GetWallAngle(nWallHit) + 512) & 2047);
                    }
                    break;
            }
            GibSprite(pMissile, GIBTYPE_24, NULL, NULL);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissilePukeGreen:
            seqKill(3, nXMissile);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileArcGargoyle:
            sfxKill3DSound(pMissile);
            sfxPlay3DSound(pMissile->x, 306, 0, 0);
            GibSprite(pMissile, GIBTYPE_6, NULL, NULL);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileLifeLeechAltNormal:
        case kMissileLifeLeechAltSmall:
            sfxKill3DSound(pMissile);
            sfxPlay3DSound(pMissile->x, 306, 0, 0);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileFireball:
        case kMissileFireballNapalm:
            actExplodeSprite(pMissile);
            break;
        case kMissileFlareAlt:
            sfxKill3DSound(pMissile);
            actExplodeSprite(pMissile);
            break;
        case kMissileFlareRegular:
            sfxKill3DSound(pMissile);
            GibSprite(pMissile, GIBTYPE_17, NULL, NULL);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileFlameSpray:
        case kMissileFlameHound:
            break;
        case kMissileFireballCerberus:
            actExplodeSprite(pMissile);
            actExplodeSprite(pMissile);
            break;
        case kMissileFireballTchernobog:
            actExplodeSprite(pMissile);
            actExplodeSprite(pMissile);
            break;
        case kMissileEctoSkull:
            sfxKill3DSound(pMissile);
            sfxPlay3DSound(pMissile->x, 522, 0, 0);
            actPostSprite(pMissile->index, kStatDebris);
            seqSpawn(20, 3, pMissile->extra, -1);
            break;
        case kMissileButcherKnife:
            actPostSprite(pMissile->index, kStatDebris);
            pMissile->cstat &= ~16;
            pMissile->type = 0;
            seqSpawn(20, 3, pMissile->extra, -1);
            break;
        case kMissileTeslaRegular:
            sfxKill3DSound(pMissile);
            sfxPlay3DSound(pMissile->x, 518, 0, 0);
            GibSprite(pMissile, (hitCode == 2) ? GIBTYPE_23 : GIBTYPE_22, NULL, NULL);
            evKill(pMissile->index, 3);
            seqKill(3, nXMissile);
            actPostSprite(pMissile->index, kStatFree);
            break;
        default:
            seqKill(3, nXMissile);
            actPostSprite(pMissile->index, kStatFree);
            break;
    }
    
    //if (gModernMap && pXSpriteHit && pXSpriteHit->state != pXSpriteHit->restState && pXSpriteHit->Impact)
        //trTriggerSprite(nSpriteHit, pXSpriteHit, kCmdSpriteImpact);
    pMissile->cstat &= ~257;
}

void actBuildMissile(spritetype* pMissile, int nXSprite, int nSprite) {
    int nMissile = pMissile->index;
    switch (pMissile->type) {
        case kMissileLifeLeechRegular:
            //evPost(nMissile, 3, 0, kCallbackFXFlameLick);
            break;
        case kMissileTeslaAlt:
            //evPost(nMissile, 3, 0, kCallbackFXTeslaAlt);
            //pMissile->cstat |= kSprFloor;
			break;
        case kMissilePukeGreen:
            seqSpawn(29, 3, nXSprite, -1);
            break;
        case kMissileButcherKnife:
            pMissile->cstat |= 16;
            break;
        case kMissileTeslaRegular:
            sfxPlay3DSound(pMissile, 251, 0, 0);
            break;
        case kMissileEctoSkull:
            seqSpawn(2, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 493, 0, 0);
            break;
        case kMissileFireballNapalm:
            seqSpawn(61, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFireball:
            seqSpawn(22, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFlameHound:
            seqSpawn(27, 3, nXSprite, -1);
            xvel[nMissile] += xvel[nSprite] / 2 + BiRandom(0x11111);
            yvel[nMissile] += yvel[nSprite] / 2 + BiRandom(0x11111);
            zvel[nMissile] += zvel[nSprite] / 2 + BiRandom(0x11111);
            break;
        case kMissileFireballCerberus:
            seqSpawn(61, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFireballTchernobog:
            seqSpawn(23, 3, nXSprite, -1);
            xvel[nMissile] += xvel[nSprite] / 2 + BiRandom(0x11111);
            yvel[nMissile] += yvel[nSprite] / 2 + BiRandom(0x11111);
            zvel[nMissile] += zvel[nSprite] / 2 + BiRandom(0x11111);
            break;
        case kMissileFlameSpray:
            if (Chance(0x8000))
                seqSpawn(0, 3, nXSprite, -1);
            else
                seqSpawn(1, 3, nXSprite, -1);
            xvel[nMissile] += xvel[nSprite] + BiRandom(0x11111);
            yvel[nMissile] += yvel[nSprite] + BiRandom(0x11111);
            zvel[nMissile] += zvel[nSprite] + BiRandom(0x11111);
            break;
        case kMissileFlareAlt:
            evPost(nMissile, 3, 30, kCallbackFXFlareBurst);
            sfxPlay3DSound(pMissile, 422, 0, 0);
            break;
        case kMissileFlareRegular:
            sfxPlay3DSound(pMissile, 422, 0, 0);
            break;
        case kMissileLifeLeechAltSmall:
            break;
        case kMissileArcGargoyle:
            sfxPlay3DSound(pMissile, 252, 0, 0);
            break;
    }
}

spritetype* actFireMissile(spritetype *pSprite, int a2, int a3, int a4, int a5, int a6, int nType)
{
    
	dassert(nType >= kMissileBase && nType < kMissileMax);
    char v4 = 0;
    int nSprite = pSprite->index;
    MissileType *pMissileInfo = &missileInfo[nType-kMissileBase];
    int x = pSprite->x+mulscale30(a2, Cos(pSprite->ang+512));
    int y = pSprite->y+mulscale30(a2, Sin(pSprite->ang+512));
    int z = pSprite->z+a3;
    int clipdist = pMissileInfo->clipDist+pSprite->clipdist;
    x += mulscale28(clipdist, Cos(pSprite->ang));
    y += mulscale28(clipdist, Sin(pSprite->ang));
    int hit = HitScan(pSprite, z, x-pSprite->x, y-pSprite->y, 0, BLOCK_MOVE, clipdist);
    if (hit != -1)
    {
        if (hit == 3 || hit == 0)
        {
            v4 = 1;
            x = gHitInfo.hitx-mulscale30(Cos(pSprite->ang), 16);
            y = gHitInfo.hity-mulscale30(Sin(pSprite->ang), 16);
        }
        else
        {
            x = gHitInfo.hitx-mulscale28(pMissileInfo->clipDist<<1, Cos(pSprite->ang));
            y = gHitInfo.hity-mulscale28(pMissileInfo->clipDist<<1, Sin(pSprite->ang));
        }
    }
	//return NULL;
    spritetype *pMissile = actSpawnSprite(pSprite->sectnum, x, y, z, 5, 1);
	
	if (!pMissile)
		return NULL;
	
    int nMissile = pMissile->index;
    SetBitString(show2dsprite, nMissile);
    pMissile->type = (short) nType;
    pMissile->shade = pMissileInfo->shade;
    pMissile->pal = 0;
    pMissile->clipdist = pMissileInfo->clipDist;
    pMissile->flags = 1;
    pMissile->xrepeat = pMissileInfo->xrepeat;
    pMissile->yrepeat = pMissileInfo->yrepeat;
    pMissile->picnum = pMissileInfo->picnum;
    pMissile->ang = (short) ((pSprite->ang+pMissileInfo->angleOfs)&2047);
	
	pMissile->yvel = kDeleteReally;
	
    xvel[nMissile] = mulscale(pMissileInfo->velocity, a4, 14);
    yvel[nMissile] = mulscale(pMissileInfo->velocity, a5, 14);
    zvel[nMissile] = mulscale(pMissileInfo->velocity, a6, 14);
    //actPropagateSpriteOwner(pMissile, pSprite);
    pMissile->cstat |= 1;
    int nXSprite = pMissile->extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    xsprite[nXSprite].target = -1;
    evPost(nMissile, 3, 600, kCallbackRemoveSpecial);
   
    actBuildMissile(pMissile, nXSprite, nSprite);
    
    if (v4)
    {
        actImpactMissile(pMissile, hit);
        pMissile = NULL;
    }
    return pMissile;
}

int MoveMissile(spritetype *pSprite)
{
    
	dassert(xspriRangeIsFine(pSprite->extra));
	int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int vdi = -1;

	gHitInfo.hitsect = -1;
    gHitInfo.hitwall = -1;
    gHitInfo.hitsprite = -1;
    if (pSprite->type == kMissileFlameSpray)
        actAirDrag(pSprite, 0x1000);
   
    int nSprite = pSprite->index;
    int vx = xvel[nSprite]>>12;
    int vy = yvel[nSprite]>>12;
    int vz = zvel[nSprite]>>8;
    
	int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    
	
	int i = 1;
    while ( 1 ) {
       
        int nSector2 = pSprite->sectnum;
		int x = pSprite->x, y = pSprite->y, z = pSprite->z;
        clipmoveboxtracenum = 1;
		int vdx = ClipMove(&x, &y, &z, &nSector2, vx, vy, pSprite->clipdist<<2, (z-top)/4, (bottom-z)/4, BLOCK_MOVE);
        clipmoveboxtracenum = 3;
		int ceilZ, floorZ, ceilHit, floorHit;
		short nSector = (short)nSector2;
        
		if (nSector2 < 0)
        {
            vdi = -1;
            break;
        }

        if (vdx)
        {
            int nHitSprite = vdx & 0x3fff;
            if ((vdx&0xc000) == 0xc000)
            {
                gHitInfo.hitsprite = nHitSprite;
                vdi = 3;
            }
            else if ((vdx & 0xc000) == 0x8000)
            {
                gHitInfo.hitwall = nHitSprite;
                if (wall[nHitSprite].nextsector == -1)
                    vdi = 0;
                else
                {
                    int fz, cz;
                    getzsofslope(wall[nHitSprite].nextsector, x, y, &cz, &fz);
                    if (z <= cz || z >= fz)
                        vdi = 0;
                    else
                        vdi = 4;
                }
            }
        }
        if (vdi == 4)
        {
            walltype *pWall = &wall[gHitInfo.hitwall];
            if (pWall->extra > 0 && xwall[pWall->extra].triggerVector) {
                XWALL *pXWall = &xwall[pWall->extra];
                trTriggerWall(gHitInfo.hitwall, &xwall[pWall->extra], kCmdWallImpact);
                if (!(pWall->cstat&64)) {
					vdi = -1;
					if (i-- > 0) continue;
					vdi = 0;
					break;
                }
                
            }
        }
        if (vdi >= 0 && vdi != 3)
        {
            int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
            x -= mulscale30(Cos(nAngle), 16);
            y -= mulscale30(Sin(nAngle), 16);
            int nVel = approxDist(xvel[nSprite], yvel[nSprite]);
            vz -= scale(0x100, zvel[nSprite], nVel);
            updatesector(x, y, &nSector);
            nSector2 = nSector;
        }
        
        GetZRangeAtXYZ(x, y, z, nSector2, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, BLOCK_MOVE);
        GetSpriteExtents(pSprite, &top, &bottom);
        top += vz;
        bottom += vz;
        if (bottom >= floorZ)
        {
            vz += floorZ-bottom;
            vdi = 2;
        }
        if (top <= ceilZ)
        {
            vz += ClipLow(ceilZ-top, 0);
            vdi = 1;
        }
        
		pSprite->x = x;
        pSprite->y = y;
        pSprite->z = z+vz;
        updatesector(x, y, &nSector);
        if (nSector >= 0 && nSector != pSprite->sectnum)
        {
            dassert(nSector >= 0 && nSector < kMaxSectors);
            ChangeSpriteSect(nSprite, nSector);
        }
        CheckLink(pSprite);
        gHitInfo.hitsect = pSprite->sectnum;
        gHitInfo.hitx = pSprite->x;
        gHitInfo.hity = pSprite->y;
        gHitInfo.hitz = pSprite->z;
        break;
    }
	
    return vdi;
}

void actProcessSprites() {
	
	nnExtProcessSuperSprites();
	//return;
	
	short nSprite, nDude, nNextDude;
	for (nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		spritetype *pDude = &sprite[nSprite];
		if (pDude->extra > 0 && CheckProximity(pDude, posx, posy, posz, cursectnum, 220))
		{
			XSPRITE *pXDude = &xsprite[pDude->extra];
			switch (pDude->type) {
				case kDudeZombieAxeLaying:
				case kDudeZombieAxeBuried:
					previewMorphDude(pDude, pXDude);
					break;
			}
		}
	}

	// process things for effects
	for (nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
		spritetype *pSprite = &sprite[nSprite];
		if ((pSprite->flags & kHitagFree) || pSprite->extra <= 0)
			continue;
		
		XSPRITE *pXSprite = &xsprite[pSprite->extra];
		if (pXSprite->triggerProximity) {
			for (nDude = headspritestat[kStatDude]; nDude >= 0; nDude = nNextDude) {
				nNextDude = nextspritestat[nDude];
				spritetype *pDude = &sprite[nDude];
				if (pDude->flags & kHitagFree) continue;
				else if (CheckProximity(pDude, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 96))
					trTriggerSprite(nSprite, pXSprite, kCmdSpriteProximity);
				
			}
			
			if (actCheckProximity(pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 96))
				trTriggerSprite(pSprite->index, pXSprite, kCmdSpriteProximity);
		}
		
		
	}
	
	// process missile sprites
	for (nSprite = headspritestat[kStatProjectile]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
		spritetype *pSprite = &sprite[nSprite];
		if ( pSprite->flags & kHitagFree )
			continue;

		int hitCode = MoveMissile(pSprite);

		// process impacts
		if ( hitCode >= 0 )
			actImpactMissile(pSprite, hitCode);
	}
	
	// process explosions
	for (nSprite = headspritestat[kStatExplosion]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
		spritetype *pSprite = &sprite[nSprite];
		if (pSprite->flags & kHitagFree)
			continue;
		
		BYTE sectmap[(kMaxSectors+7)>>3];
		gAffectedSectors[0] = -1;
        gAffectedXWalls[0] = -1;
		
		int nType = pSprite->type;
		dassert(nType >= 0 && nType < kExplosionMax);
		EXPLODE_INFO *pExp = &explodeInfo[nType];

		int nXSprite = pSprite->extra;
		dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
		XSPRITE *pXSprite = &xsprite[nXSprite];
				
        int radius = pExp->radius;
        if (gModernMap && pXSprite->data4 > 0)
            radius = pXSprite->data4;

		
		int nAffected;
		int x = pSprite->x, y = pSprite->y, z = pSprite->z, nSector = pSprite->sectnum;
		
		int dx = (x - posx)>>4;
		int dy = (y - posy)>>4;
		int dz = (z - posz)>>8;
		int nDist = dx*dx+dy*dy+dz*dz+0x40000;
		int t = divscale16(pXSprite->data2, nDist);
		gPreview.scrEffects[kScrEffectQuake2] += t;
		
		GetClosestSpriteSectors(nSector, x, y, radius, gAffectedSectors, sectmap, gAffectedXWalls);
		
        for (int i = 0; i < kMaxXWalls; i++)
        {
            int nWall = gAffectedXWalls[i];
            if (nWall == -1)
                break;
            XWALL *pXWall = &xwall[wall[nWall].extra];
            trTriggerWall(nWall, pXWall, kCmdWallImpact);
        }
		
		
		for (nAffected = headspritestat[kStatDude]; nAffected >= 0; nAffected = nextspritestat[nAffected]) {
			spritetype *pDude = &sprite[nAffected];
			if (pXSprite->data1 && CheckProximity(pDude, x, y, z, nSector, radius))
			{
				if (pDude->extra < 0) continue;
				XSPRITE *pXDude = &xsprite[pDude->extra];
				previewKillDude(pDude, pXDude);
			}
		}

		for (nAffected = headspritestat[kStatThing]; nAffected >= 0; nAffected = nextspritestat[nAffected]) {
			spritetype *pThing = &sprite[nAffected];
			if (pXSprite->data1 && CheckProximity(pThing, x, y, z, nSector, radius))
			{
				if (pThing->extra < 0) continue;
				XSPRITE *pXThing = &xsprite[pThing->extra];
				trTriggerSprite(pThing->index, pXThing, kCmdOff);
			}
		}
		
		int ticks = 4 << 1;
		pXSprite->data1 = ClipLow(pXSprite->data1 - ticks, 0);
		pXSprite->data2 = ClipLow(pXSprite->data2 - ticks, 0);
		pXSprite->data3 = ClipLow(pXSprite->data3 - ticks, 0);
		if (pXSprite->data1 == 0 && pXSprite->data2 == 0 && pXSprite->data3 == 0 && seqGetStatus(OBJ_SPRITE, nXSprite) < 0) {
			actPostSprite(nSprite, kStatFree);
		}
	}
	
	gFX.fxProcess();

}

void actAirDrag(int* vx, int* vy, int* vz, int nSect, int nDrag)
{
	int windX = 0, windY = 0;
	dassert(nSect >= 0 && nSect < kMaxSectors);
	sectortype *pSector = &sector[nSect];
	
	if (pSector->extra > 0)
	{
		XSECTOR *pXSector = &xsector[pSector->extra];
		if (pXSector->windVel && (pXSector->windAlways || pXSector->busy))
		{
			int windVel = pXSector->windVel << 12;

			if ( !pXSector->windAlways && pXSector->busy )
				windVel = mulscale16(windVel, pXSector->busy);

			windX = mulscale30(windVel, Cos(pXSector->windAng));
			windY = mulscale30(windVel, Sin(pXSector->windAng));
		}
	}
	
	if (vx) *vx += mulscale16(windX - *vx, nDrag);
	if (vy) *vy += mulscale16(windY - *vy, nDrag);
	if (vz) *vz -= mulscale16(*vz, nDrag);
	
}

void actAirDrag( spritetype *pSpr, int nDrag ) {
	
	int nSpr = pSpr->index;
	actAirDrag(&xvel[nSpr], &yvel[nSpr], &zvel[nSpr], pSpr->sectnum, nDrag);
	
/* 	int windX = 0, windY = 0;
	int nSector = pSprite->sectnum;
	dassert(nSector >= 0 && nSector < kMaxSectors);
	sectortype *pSector = &sector[nSector];
	int nXSector = pSector->extra;
	if (nXSector > 0) {
		dassert(nXSector < kMaxXSectors);
		XSECTOR *pXSector = &xsector[nXSector];
		if (pXSector->windVel && (pXSector->windAlways || pXSector->busy))
		{
			int windVel = pXSector->windVel << 12;

			if ( !pXSector->windAlways && pXSector->busy )
				windVel = mulscale16(windVel, pXSector->busy);

			windX = mulscale30(windVel, Cos(pXSector->windAng));
			windY = mulscale30(windVel, Sin(pXSector->windAng));
		}
	}
	int nSprite = pSprite->index;
	xvel[nSprite] += mulscale16(windX - xvel[nSprite], nDrag);
	yvel[nSprite] += mulscale16(windY - yvel[nSprite], nDrag);
	zvel[nSprite] -= mulscale16(zvel[nSprite], nDrag); */
	
}

void actExplodeSprite(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    if (nXSprite <= 0 || nXSprite >= kMaxXSprites || pSprite->statnum == kStatExplosion)
        return;

	sfxKill3DSound(pSprite);
    evKill(pSprite->index, 3);
    short nType = kExplosionStandard;

    switch (pSprite->type) {
		case kMissileFireballNapalm:
			nType = kExplosionNapalm;
			seqSpawn(4, 3, nXSprite, -1);
			if (Chance(0x8000)) pSprite->cstat |= 4;
			sfxPlay3DSound(pSprite, 303, -1, 0);
			break;
		case kMissileFlareAlt:
			nType = kExplosionFireball;
			seqSpawn(9, 3, nXSprite, -1);
			if (Chance(0x8000)) pSprite->cstat |= 4;
			sfxPlay3DSound(pSprite, 306, 24+(pSprite->index&3), 1);
			break;
		case kMissileFireballCerberus:
		case kMissileFireballTchernobog:
			nType = kExplosionFireball;
			seqSpawn(5, 3, nXSprite, -1);
			sfxPlay3DSound(pSprite, 304, -1, 0);
			break;
		case kThingArmedTNTStick:
			nType = kExplosionSmall;
			seqSpawn(3,3,nXSprite,-1);
			sfxPlay3DSound(pSprite, 303, -1, 0);
			break;
		case kThingArmedProxBomb:
		case kThingArmedRemoteBomb:
		case kThingArmedTNTBundle:
			nType = 1;
			seqSpawn(3, OBJ_SPRITE, nXSprite);
			sfxPlay3DSound(pSprite, 303, -1, 0);
			break;
		case kThingArmedSpray:
			nType = kExplosionSpray;
			seqSpawn(5, 3, nXSprite, -1);
			sfxPlay3DSound(pSprite, 307, -1, 0);
			break;
		case kThingTNTBarrel:
			nType = kExplosionLarge;
			seqSpawn(4, 3, nXSprite, -1);
			sfxPlay3DSound(pSprite, 305, -1, 0);
			break;
		case kTrapExploder:	{
			// Defaults for exploder
			nType = 1; int nSnd = 304; int nSeq = 4;

			// allow to customize hidden exploder trap
			if (gModernMap) {
				// Temp variables for override via data fields
				int tSnd = 0; int tSeq = 0;


				XSPRITE* pXSPrite = &xsprite[nXSprite];
				nType = (short)pXSPrite->data1;  // Explosion type
				tSeq = pXSPrite->data2; // SEQ id
				tSnd = pXSPrite->data3; // Sound Id

				if (nType <= 1 || nType > kExplosionMax) { nType = 1; nSeq = 4; nSnd = 304; }
				else if (nType == 2) { nSeq = 4; nSnd = 305; }
				else if (nType == 3) { nSeq = 9; nSnd = 307; }
				else if (nType == 4) { nSeq = 5; nSnd = 307; }
				else if (nType <= 6) { nSeq = 4; nSnd = 303; }
				else if (nType == 7) { nSeq = 4; nSnd = 303; }
				else if (nType == 8) { nType = 0; nSeq = 3; nSnd = 303; }

				// Override previous sound and seq assigns
				if (tSeq > 0) nSeq = tSeq;
				if (tSnd > 0) nSnd = tSnd;
			}

			
			if (gSysRes.Lookup(nSeq, "SEQ"))
				seqSpawn(nSeq, 3, nXSprite, -1);

			sfxPlay3DSound(pSprite, nSnd, -1, 0);
		}
			break;
		case kThingPodFireBall:
			nType = kExplosionFireball;
			seqSpawn(9, 3, nXSprite, -1);
			sfxPlay3DSound(pSprite, 307, -1, 0);
			break;
		default:
			nType = kExplosionStandard;
			seqSpawn(4, 3, nXSprite, -1);
			if (Chance(0x8000))
				pSprite->cstat |= 4;
			sfxPlay3DSound(pSprite, 303, -1, 0);
			break;
    }
    
	int nSprite = pSprite->index;
    xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
    actPostSprite(nSprite, kStatExplosion);
	
	EXPLODE_INFO *pExplodeInfo = &explodeInfo[nType];
	pSprite->type = nType;
	pSprite->flags &= ~3;
    
	pSprite->xrepeat = pSprite->yrepeat = pExplodeInfo->repeat;
    xsprite[nXSprite].target = 0;
    xsprite[nXSprite].data1 = pExplodeInfo->ticks;
    xsprite[nXSprite].data2 = pExplodeInfo->quakeEffect;
    xsprite[nXSprite].data3 = pExplodeInfo->flashEffect;
	
}

spritetype *actSpawnSprite( int nSector, int x, int y, int z, int nStatus, BOOL bAddXSprite) {
	
	if (nSector < 0)
		ThrowError("Out of sector!");

	int nSprite = InsertSprite(nSector, nStatus);
	spritetype *pSprite = &sprite[nSprite];
	
	setsprite(pSprite->index, x, y, z);
	pSprite->yvel = kDeleteReally;
	pSprite->type    = 0;
	
	if (bAddXSprite && pSprite->extra == -1)
		dbInsertXSprite(pSprite->index);

	return pSprite;
	
}

spritetype *actSpawnSprite( spritetype *pSource, int nStat ) {
	
	spritetype *pSprite = actSpawnSprite(pSource->sectnum, pSource->x, pSource->y, pSource->z, nStat, TRUE);
	if (pSprite) {
		xvel[pSprite->index] = xvel[pSource->index];
		yvel[pSprite->index] = yvel[pSource->index];
		zvel[pSprite->index] = zvel[pSource->index];
		pSprite->flags = 0;
	}
	
	return pSprite;
}

spritetype* actDropObject( spritetype *pActor, int nObject ) {
	dassert(pActor->statnum < kMaxStatus);
	dassert( (nObject >= kItemBase && nObject < kItemMax)
		|| (nObject >= kItemAmmoBase && nObject < kItemAmmoMax)
		|| (nObject >= kItemWeaponBase && nObject < kItemWeaponMax) );

	// create a sprite for the dropped ammo
	int x = pActor->x;
	int y = pActor->y;
	if (pActor->sectnum < 0 || pActor->sectnum >= kMaxSectors)
	{
		previewMessage("%s (%d) have no sector!", gSpriteNames[pActor->type], pActor->index);
		return NULL;
	}
	
	int z = getflorzofslope(pActor->sectnum, x, y);
	spritetype *pSprite = actSpawnSprite(pActor->sectnum, x, y, z, kStatItem, FALSE);
	pSprite->type = (short)nObject;
	adjSpriteByType(pSprite);
	pSprite->shade = -127;
	
	clampSprite(pSprite);
	return pSprite;

}

spritetype *actSpawnDude(spritetype *pSource, short nType, int a3, int a4) {
	if (pSource->sectnum < 0) return NULL;
	XSPRITE* pXSource = &xsprite[pSource->extra];
	a3 = 0;
	int x, y, z;
	z = a4 + pSource->z;
	x = pSource->x;
	y = pSource->y;
	
	spritetype *pSprite2 = actSpawnSprite(pSource->sectnum, x, y, z, kStatDude, TRUE);
	if (!pSprite2) return NULL;
	XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
	pXSprite2->state = 1;
	pXSprite2->health = 100 << 4;
	pSprite2->type = nType;
	pSprite2->ang = pSource->ang;
	//setsprite(pSprite2->index, x, y, z);
	
	
	pSprite2->x = x;
	pSprite2->y = y;
	pSprite2->z = z;
	
	changespritesect(pSprite2->index, pSource->sectnum);
	
	pSprite2->cstat |= 0x1101;
	pSprite2->clipdist = 32;

	adjSpriteByType(pSprite2);

	if (pSprite2->sectnum >= 0)
		clampSprite(pSprite2);

	return pSprite2;
}

void actActivateGibObject(spritetype *pSprite, XSPRITE *pXSprite) {
	
	if (pSprite->flags & 32)
		return;
	
	int vdx = ClipRange(pXSprite->data1, 0, 31);
	int vc = ClipRange(pXSprite->data2, 0, 31);
	int v4 = ClipRange(pXSprite->data3, 0, 31);

	if (vdx > 0) GibSprite(pSprite, (GIBTYPE)(vdx-1), NULL, NULL);
	if (vc > 0) GibSprite(pSprite, (GIBTYPE)(vc-1), NULL, NULL);
	if (v4 > 0 && pXSprite->burnTime > 0) GibSprite(pSprite, (GIBTYPE)(v4-1), NULL, NULL);
   
	if (pXSprite->data4)
		sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, pXSprite->data4);

	if (pXSprite->dropItem)
		actDropObject(pSprite, pXSprite->dropItem);
	
	//if (pXSprite->key)
		//actDropObject(pSprite, pXSprite->key + kItemBase - 1);
	
	if (!(pSprite->cstat & kSprInvisible))
		hideSprite(pSprite);
		//actPostSprite(pSprite->index, kStatFree);
}

void actPostSprite(int nSprite, int nStatus) {
	int n;
	dassert(gPostCount < kMaxSprites);
	dassert(nSprite < kMaxSprites && sprite[nSprite].statnum < kMaxStatus);
	dassert(nStatus >= 0 && nStatus <= kStatFree);
	if (sprite[nSprite].flags & 32)
	{
		for (n = 0; n < gPostCount; n++)
			if (gPost[n].at0 == nSprite)
				break;
		dassert(n < gPostCount);
	}
	else
	{
		n = gPostCount;
		sprite[nSprite].flags |= 32;
		gPostCount++;
	}
	gPost[n].at0 = (short)nSprite;
	gPost[n].at2 = (short)nStatus;
}

void actPostProcess(void) {
	
	for (int i = 0; i < gPostCount; i++) {
		POSTPONE *pPost = &gPost[i];
		int nSprite = pPost->at0;
		spritetype *pSprite = &sprite[nSprite];
		pSprite->flags &= ~32;
		int nStatus = pPost->at2;
		if (nStatus == kStatFree) previewDelSprite(nSprite);
		else ChangeSpriteStat(nSprite, nStatus);
	}
	
	gPostCount = 0;
}


