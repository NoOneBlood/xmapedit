/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A lite version of game's view.cpp adapted for xmapedit.
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

#include "xmpmaped.h"
#include "xmpview.h"
#include "tile.h"
#include "aadjust.h"
#include "nnexts.h"
#include "xmpconf.h"
#include "xmpevox.h"
#include "seq.h"
#include "grdshd.h"
#include "hglt.h"

void viewDoQuake(int strength, int* x, int* y, int* z, short* a, int* h) {
	
	if (!strength)
		return;
	
	int nValue = ClipHigh(strength * 8, 2000);
	*x += QRandom2(nValue >> 4); *y += QRandom2(nValue >> 4);
	*h += QRandom2(nValue >> 8); *a += QRandom2(nValue >> 8);
	*z += QRandom2(nValue);
	
}

void viewRotateSprite(spritetype* pTSprite, int nView, int x, int y, int* offset) {
		
	int dx, dy, nOctant;
	if (nView != kSprViewFull5 && nView != kSprViewFull8)
		return;
	
	// Calculate which of the 8 angles of the sprite to draw (0-7)
	dx = x - pTSprite->x; dy = y - pTSprite->y;
	RotateVector(&dx, &dy, -pTSprite->ang + kAng45 / 2);
	nOctant = GetOctant(dx, dy);
	
	if (nView == kSprViewFull8) *offset = nOctant;
	else if (nOctant <= 4) *offset = nOctant, pTSprite->cstat &= ~4; // clear x-flipping bit
	else *offset = 8 - nOctant, pTSprite->cstat |= 4; // set x-flipping bit
	
}

spritetype *viewInsertTSprite( int nSector, int nStatus, spritetype *pSource = NULL ) {
	
	if (spritesortcnt >= kMaxViewSprites)
		return NULL;
	
	int nTSprite = spritesortcnt;
	spritetype *pTSprite = &tsprite[nTSprite];
	memset(pTSprite, 0, sizeof(spritetype));
	pTSprite->type = (short)-spritesortcnt;
	pTSprite->sectnum = (short)nSector;
	pTSprite->statnum = (short)nStatus;
	pTSprite->cstat = defaultspritecstat;
	pTSprite->xrepeat = 64;
	pTSprite->yrepeat = 64;
	pTSprite->owner = -1;
	pTSprite->extra = -1;
	spritesortcnt++;

	if ( pSource != NULL )
	{
		pTSprite->x = pSource->x;
		pTSprite->y = pSource->y;
		pTSprite->z = pSource->z;
		pTSprite->owner = pSource->owner;
		pTSprite->ang = pSource->ang;
	}
	
	return &tsprite[nTSprite];
}

spritetype* viewEffectCdudeVersion(spritetype* pSpr, int z, int rep = 48)
{
	int zTop, zBot;
	XSPRITE* pXSpr = &xsprite[pSpr->extra];
	int nPic = (pXSpr->sysData4 == 2) ? gSysTiles.icoVer2 : gSysTiles.icoVer1;
	if (!nPic)
		return NULL;
	
	spritetype *pTEffect = viewInsertTSprite(pSpr->sectnum, -32767, pSpr);
	
	pTEffect->z 			= z;
	pTEffect->picnum		= nPic;
	pTEffect->xrepeat		= rep;
	pTEffect->yrepeat		= ClipLow(rep-8, 8);
	pTEffect->shade			= -127;
	pTEffect->cstat 	   |= (kSprTransluc2);
	pTEffect->xoffset  		= panm[pSpr->picnum].xcenter;
	
	if (!isSkySector(pSpr->sectnum, OBJ_CEILING))
	{
		int fz, cz;
		GetSpriteExtents(pTEffect, &zTop, &zBot);
		getzsofslope(pTEffect->sectnum, pTEffect->x, pTEffect->y, &cz, &fz);
		if (pTEffect->z < cz)
			pTEffect->z = cz + klabs(zTop-zBot);
	}
	
	return pTEffect;
}

void viewAddEffect( int nTSprite, VIEW_EFFECT nViewEffect ) {
	
	if (spritesortcnt >= kMaxViewSprites - 1)
		return;
	
	int i, j, k, f, zTop, zBot, zTop2, zBot2;
	spritetype *pTSprite = &tsprite[nTSprite];

	switch (nViewEffect) {
		case kViewEffectCdudeVersion:
			GetSpriteExtents(pTSprite, &zTop, &zBot);
			viewEffectCdudeVersion(pTSprite, zTop-1024);
			break;
		case kViewEffectMiniCustomDude:
			if (!gModernMap) break;
			// no break
		case kViewEffectMiniDude:
		{
			int z, ofs = 0; short pic = -1, pal = -1, xr, yr;
			spritetype* pSpr = &sprite[pTSprite->owner];
			if (pSpr->extra <= 0)
				break;

			XSPRITE* pXSpr = &xsprite[pSpr->extra];
			GetSpriteExtents(pTSprite, &zTop, &zBot);
			
			switch (nViewEffect) {
				case kViewEffectMiniDude:
					z = zTop; f = (gModernMap) ? 4 : 1;
					for (i = 0; i < f; i++)
					{
						ofs = 0;
						j = getDataOf(i, OBJ_SPRITE, pTSprite->index);
						if (j < kDudeBase || j >= kDudeMax || (k = adjIdxByType(j)) < 0)
							continue;
						
						spritetype *pTEffect  = viewInsertTSprite(pTSprite->sectnum, -32767, pTSprite);
						
						// rotate ViewFullN sprites
						viewRotateSprite(pTEffect, viewType[autoData[k].picnum], posx, posy, &ofs);

						if ((pSpr->hitag & kModernTypeFlag1) && f > 1)
						{
							if (autoData[k].seq >= 0)
							{
								getSeqPrefs(autoData[k].seq, &pic, &xr, &yr, &pal);
								pal = (pal >= 0) ? autoData[k].plu : pSpr->pal;
							}
							else
							{
								pal = pSpr->pal;
							}
						}
						else
						{
							pal = autoData[k].plu;
						}
						
						pTEffect->picnum 	= ClipLow(autoData[k].picnum + ofs, 0);
						pTEffect->pal 		= ClipLow(pal, 0);
						pTEffect->xrepeat 	= pTEffect->yrepeat = 8;
						pTEffect->cstat    |= (kSprTransluc2);
						pTEffect->cstat    &= ~kSprOrigin;
						pTEffect->z 		= z;
						
						GetSpriteExtents(pTEffect, &zTop, &zBot);
						z-=((abs(zTop - zBot)) + 512);
					}
					break;
				default:
					pic = pXSpr->sysData1; pal = pSpr->pal;
					spritetype *pTEffect  = viewInsertTSprite(pTSprite->sectnum, -32767, pTSprite);
					
					// rotate ViewFullN sprites
					viewRotateSprite(pTEffect, viewType[pic], posx, posy, &ofs);
					
					pTEffect->picnum	 = ClipLow(pic + ofs, 0);
					pTEffect->pal		 = ClipLow((pSpr->hitag & kModernTypeFlag1) ? pSpr->pal : pal, 0);
					pTEffect->xrepeat 	 = pTEffect->yrepeat = 8;
					pTEffect->cstat 	|= (kSprTransluc2);
					pTEffect->cstat 	&= ~kSprOrigin;
					pTEffect->z 		 = zTop;
					
					GetSpriteExtents(pTEffect, &zTop, &zBot);
					viewEffectCdudeVersion(pTSprite, zTop-256, 32);
					break;
			}
		}
		break;
		case kViewEffectAngle:
		{
			spritetype *pTEffect  = viewInsertTSprite(pTSprite->sectnum, 0, pTSprite);
			spritetype *pTEffect2 = viewInsertTSprite(pTSprite->sectnum, 0, pTSprite);
			GetSpriteExtents(pTSprite, &zTop, &zBot);

			pTEffect->picnum   = pTEffect2->picnum   =  gSysTiles.angArrow;
			pTEffect->statnum  = pTEffect2->statnum  = (short)(pTSprite->statnum - 1);
			pTEffect->xrepeat  = pTEffect2->xrepeat  =  20;
			pTEffect->yrepeat  = pTEffect2->yrepeat  =  20;
			pTEffect->xoffset  = pTEffect2->xoffset  =  -48;		
			pTEffect->shade    = pTEffect2->shade    =  63;
			pTEffect->pal      = pTEffect2->pal      =  1;
			
			pTEffect->cstat  |= kSprFloor | kSprTransluc2;
			pTEffect2->cstat |= kSprWall | kSprTransluc2;

			pTEffect->ang  = (short) (pTSprite->ang - kAng90);
			pTEffect2->ang = (short) (pTSprite->ang + kAng90);
			
			pTEffect->z = pTEffect2->z = zBot + ((zTop - zBot) / 2);
		}
		break;
		case kViewEffectHighlight:
		{
			spritetype *pTEffect = viewInsertTSprite(pTSprite->sectnum, 0, pTSprite);
			GetSpriteExtents(pTSprite, &zTop, &zBot);

			// show in the top
			pTEffect->z = zTop - 1024;
			pTEffect->picnum = 2332;
			pTEffect->xrepeat = pTEffect->yrepeat = 20;
			pTEffect->shade = 63;
			pTEffect->pal = 1;

			GetSpriteExtents(pTEffect, &zTop2, &zBot2); // show in the bottom
			if (zTop2 <= getceilzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y)) {

				pTEffect->picnum = 2331;
				pTEffect->z = zBot + 1024;
				
			}
		}
		break;
		case kViewEffectCandleHalo:
		{
			spritetype *pTEffect = viewInsertTSprite(pTSprite->sectnum, 0x7FFF, pTSprite);
			GetSpriteExtents(pTSprite, &zTop, &zBot);
			pTEffect->z = zTop;
			pTEffect->cstat |= kSprTransluc1;
			pTEffect->shade = 26;
			pTEffect->pal = kPlu0;
			pTEffect->xrepeat = pTEffect->yrepeat = 24;
			pTEffect->picnum = 626;
		}
		break;
		case kViewEffectTorchHigh:
		case kViewEffectTorchLow:
		case kViewEffectSmokeLow:
		case kViewEffectSmokeHigh:
		{
			spritetype *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
			GetSpriteExtents(pTSprite, &zTop, &zBot);
			switch (nViewEffect) {
				case kViewEffectTorchHigh:
				case kViewEffectTorchLow:
					pTEffect->z = (nViewEffect == kViewEffectTorchHigh) ? zTop : zBot;
					pTEffect->picnum = 2101;
					pTEffect->shade = -128;
					pTEffect->xrepeat = pTEffect->yrepeat = (BYTE)(tilesizx[pTSprite->picnum] * pTSprite->xrepeat / 32);
					break;
				default:
					pTEffect->z = (nViewEffect == kViewEffectSmokeHigh) ? zTop : zBot;
					pTEffect->picnum = 754;
					pTEffect->shade = 8;
					pTEffect->cstat |= kSprTransluc1;
					pTEffect->xrepeat = pTSprite->xrepeat;
					pTEffect->yrepeat = pTSprite->yrepeat;
					break;
				
			}
		}
		break;
	}
	
}

enum {
kVoxTypeInternal		= 1,
kVoxTypeExternal		= 2,
};


int viewSpriteShade(int nShade, int nTile, int nSect)
{
	sectortype *pSect = &sector[nSect];
	if ((pSect->ceilingstat & kSectParallax) && !(pSect->floorstat & kSectShadeFloor))
		nShade += pSect->ceilingshade + tileShade[pSect->ceilingpicnum];
	else
		nShade += pSect->floorshade + tileShade[pSect->floorpicnum];
	
	nShade += tileShade[nTile];	// allow sprite tile shading
	return nShade;
}

unsigned char viewWallHighlightScale(int width, int height, int x, int y)
{
	#define kVal 0x1B000
	
	unsigned char size;
	size = (unsigned char)ClipRange(scale(height, width, kVal), 20, 48);
	size = (unsigned char)ClipRange(mulscale8(size, approxDist(x-posx, y-posy)/18), size, 164);
	return size;
}

void viewWallHighlight(int nWall, int nSect, POINT3D point[4])
{
	unsigned char size;
	
	spritetype* pView;
	sectortype* pSect = &sector[nSect];
	int nAng  = (GetWallAngle(nWall) + kAng90) & kAngMask;
	int zTop, zBot, nLen = getWallLength(nWall);
	int lHeigh = klabs(point[2].z - point[0].z);
	int rHeigh = klabs(point[3].z - point[1].z);
	int nFSpr;
	int i;

	if ((nFSpr = headspritestat[kStatFree]) < 0)
		return;
	
	for (i = 0; i < 4; i++)
	{
		if ((pView = viewInsertTSprite(nSect, -32767)) != NULL)
		{			
			switch(i)
			{
				case 0: // lt
				case 1: // rt
					size = viewWallHighlightScale(nLen, lHeigh, point[i].x, point[i].y);
					if (i == 0) pView->cstat |= kSprFlipX;
					break;
				case 2: // lb
				case 3: // rb
					size = viewWallHighlightScale(nLen, rHeigh, point[i].x, point[i].y);
					if (i == 2)
						pView->cstat |= kSprFlipX;
					
					pView->cstat |= kSprFlipY;
					break;
			}
			
			pView->picnum		= gSysTiles.wallHglt;
			pView->xrepeat		= size;
			pView->yrepeat		= size;
			pView->shade 		= -128;
			pView->ang 			= nAng;
			pView->cstat		|= kSprOneSided;			
			if (!h)
				pView->cstat |= kSprTransluc2;
			
			pView->x 			= point[i].x;
			pView->y 			= point[i].y;
			pView->z 			= point[i].z;
			pView->owner 		= nFSpr;
			
			doWallCorrection(nWall, &pView->x, &pView->y);
			GetSpriteExtents(pView, &zTop, &zBot);
			
			if (i > 1)
			{
				pView->z = zTop;
				if (pSect->floorslope)
					continue;
			}
			else
			{
				pView->z = zBot;
				if (pSect->ceilingslope)
					continue;
			}
			
			pView->cstat |= kSprWall;
		}
		
	}
}

void viewWallHighlight(int nWall, int nSect, char how, BOOL testPoint)
{
	int cz1, cz2, cz3, cz4, fz1, fz2, fz3, fz4;
	int nNext = wall[nWall].nextsector;
	int x1, y1, x2, y2;
	char which = 0;

	POINT3D point[4];
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	point[0].x = point[2].x = x1;	point[1].x = point[3].x = x2;
	point[0].y = point[2].y = y1; 	point[1].y = point[3].y = y2; 
		
	getzsofslope(nSect, x1, y1, &cz1, &fz1); // this left top bot
	getzsofslope(nSect, x2, y2, &cz2, &fz2); // this right top bot
	
	if (how < 0x10 || nNext < 0)
	{
		point[0].z = cz1;	point[1].z = cz2;
		point[2].z = fz1;	point[3].z = fz2;
		
		if (testPoint && nNext >= 0)
		{
			getzsofslope(nNext, x1, y1, &cz1, &fz1); // next left top bot
			getzsofslope(nNext, x2, y2, &cz2, &fz2); // next right top bot
			
			if (point[3].z > fz1 || point[1].z > fz2) which |= 0x01;
			if (point[2].z < cz1 || point[0].z < cz2) which |= 0x02;
			
			if (!how && !which) return;
			else if (how & 0x01)
			{
				switch (which & 0x03) {
					case 3:
						viewWallHighlight(nWall, nSect, (POINT3D*)point); 	// only full wall
						return;
					case 2:
						viewWallHighlight(nWall, nSect, 0x020, FALSE);		// only ceil wall
						return;
					case 1:
						viewWallHighlight(nWall, nSect, 0x010, FALSE);		// only floor wall
						return;
				}
			}
		}

		if (!testPoint || (point[2].z != point[0].z && point[3].z != point[1].z))
			viewWallHighlight(nWall, nSect, (POINT3D*)point);
	}
	else if (nNext >= 0)
	{
		// wall from floor
		if (how & 0x10)
		{
			point[0].z = ClipLow(getflorzofslope(nNext, x1, y1), cz1); 		// next left top
			point[1].z = ClipLow(getflorzofslope(nNext, x2, y2), cz2); 		// next right top
			point[2].z = getflorzofslope(nSect, x1, y1); 					// this left bot
			point[3].z = getflorzofslope(nSect, x2, y2); 					// this right bot
			
			if (!testPoint || point[2].z > point[0].z)
				viewWallHighlight(nWall, nSect, (POINT3D*)point);
		}
		
		// wall from ceil
		if (how & 0x20)
		{
			point[0].z = getceilzofslope(nSect, x1, y1); 					// this left top
			point[1].z = getceilzofslope(nSect, x2, y2); 					// this right top
			point[2].z = ClipHigh(getceilzofslope(nNext, x1, y1), fz1); 	// next left bot
			point[3].z = ClipHigh(getceilzofslope(nNext, x2, y2), fz2); 	// next right bot
			
			if (!testPoint || point[2].z > point[0].z)
				viewWallHighlight(nWall, nSect, (POINT3D*)point);
		}
		
		// between floor and ceil
		if (how & 0x40)
		{
			getzsofslope(nNext, x1, y1, &cz3, &fz3);	// next left top bot
			getzsofslope(nNext, x2, y2, &cz4, &fz4); 	// next right top bot
			
			point[0].z = (cz1 < cz3) ? cz3 : cz1;
			point[1].z = (cz2 < cz4) ? cz4 : cz2;
			point[2].z = (fz1 > fz3) ? fz3 : fz1;
			point[3].z = (fz2 > fz4) ? fz4 : fz2;
			
			if (!testPoint || (point[2].z != point[0].z && point[3].z != point[1].z))
				viewWallHighlight(nWall, nSect, (POINT3D*)point);
		}
	}
}

void viewSpriteHighlight(spritetype* pSpr)
{
	spritetype* pView; char floor = 0x0;
	int nSect = pSpr->sectnum, nAng = pSpr->ang, t = pSpr->ang;
	int i, x1, y1, x2, y2, x3, y3, x4, y4, zt, zb;
	int size, fz, cz;
	int nFSpr;
	
	if ((nFSpr = headspritestat[kStatFree]) < 0)
		return;
	
	switch (pSpr->cstat & kSprRelMask)
	{
		case kSprFace:
			pSpr->ang = nAng = (ang + kAng180) & kAngMask;
			GetSpriteExtents(pSpr, &x1, &y1, &x2, &y2, &zt, &zb);
			pSpr->ang = t;
			break;
		case kSprWall:
			GetSpriteExtents(pSpr, &x1, &y1, &x2, &y2, &zt, &zb);
			if ((x1-posx)*(y2-posy) < (x2-posx)*(y1-posy))
			{
				x3 = x1+((x2-x1)>>1); y3 = y1+((y2-y1)>>1);
				nAng = (pSpr->ang + kAng180) & kAngMask;
				RotatePoint(&x1, &y1, kAng180, x3, y3);
				RotatePoint(&x2, &y2, kAng180, x3, y3);
			}	
			break;
		case kSprFloor:
			GetSpriteExtents(pSpr, &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4);
			floor |= 0x01;
			if (spriteGetSlope(pSpr->index))
				floor |= 0x02;
			break;
	}
	
	getzsofslope(nSect, pSpr->x, pSpr->y, &cz, &fz);
	size = ClipRange(approxDist(pSpr->x - posx, pSpr->y - posy)/72, 20, 128);
	
	for (i = 0; i < 4; i++)
	{
		if ((pView = viewInsertTSprite(nSect, -32767, pSpr)) == NULL)
			break;
		
		pView->xoffset		= pView->yoffset = -1;
		pView->picnum 		= gSysTiles.wallHglt;
		pView->xrepeat 		= pView->yrepeat = size;
		pView->shade 		= -128;
		pView->ang 			= nAng;
		pView->owner 		= nFSpr;
		pView->pal			= 10;
		
		if (!h)
			pView->cstat |= kSprTransluc2;
		
		if (!floor)
		{
			pView->cstat |= (kSprOneSided | kSprWall);

			switch(i)
			{
				case 0: // lt
					pView->x 			= x1;
					pView->y 			= y1;
					pView->z 			= zt;
					pView->cstat    	|= kSprFlipX;
					break;
				case 1: // lb
					pView->x 			= x1;
					pView->y 			= y1;
					pView->z 			= zb;
					pView->cstat    	|= (kSprFlipX | kSprFlipY);
					break;
				case 2: //rt
					pView->x 			= x2;
					pView->y 			= y2;
					pView->z 			= zt;
					break;
				case 3: //rb
					pView->x 			= x2;
					pView->y 			= y2;
					pView->z 			= zb;
					pView->cstat    	|= kSprFlipY;
					break;
				
			}
			
			switch (i)
			{
				case 1:
				case 3:
					if (isSkySector(nSect, OBJ_FLOOR) || zb < fz) break;
					pView->yoffset = -2;
					break;
				case 0:
				case 2:
					if (isSkySector(nSect, OBJ_CEILING) || zt > cz) break;
					pView->yoffset = -2;
					break;
			}
		}
		else
		{
			pView->cstat |= kSprFloor;
			
			switch(i)
			{
				case 0: // lt
					pView->x 			= x1;
					pView->y 			= y1;
					pView->cstat    	|= kSprFlipX;
					break;
				case 1: // rt
					pView->x 			= x2;
					pView->y 			= y2;
					break;
				case 2: //rb
					pView->x 			= x3;
					pView->y 			= y3;
					pView->ang			= pSpr->ang + kAng90;
					break;
				case 3: //lb
					pView->x 			= x4;
					pView->y 			= y4;
					pView->ang			= pSpr->ang + kAng180;
					break;
				
			}
			
			if (floor & 0x02)
				pView->z = spriteGetZOfSlope(pSpr->index, pView->x, pView->y);
			else
				pView->z = pSpr->z - 16;
		}
	}
}

void viewObjectHighlight(int nType, int nID)
{
	spritetype* pView = NULL;
	int cx, cy, cz, fz, size;
	int nFSpr;
	int t;
		
	if ((nFSpr = headspritestat[kStatFree]) < 0)
		return;
	
	switch(nType)
	{
		case OBJ_FLOOR:
		case OBJ_CEILING:
			if ((pView = viewInsertTSprite(nID, -32767)) != NULL)
			{
				avePointSector(nID, &cx, &cy);
				getzsofslope(nID, cx, cy, &cz, &fz);
				if (nType == OBJ_FLOOR)
				{
					pView->z		= fz - 2048;
					pView->picnum	= 4518;
				}
				else
				{
					pView->z 		= cz + 2048;
					pView->picnum	= 4515;
				}
				
				pView->x 		= cx;
				pView->y 		= cy;
				pView->ang		= (short)((totalclock * 12) & kAngMask);
				pView->cstat   |= kSprWall;
			}
			break;
		case OBJ_WALL:
			if ((pView = viewInsertTSprite(sectorofwall(nID), -32767)) != NULL)
			{
				avePointWall(nID, &cx, &cy, &cz);
				doWallCorrection(nID, &cx, &cy);
				//pView->ang = GetWallAngle(nID) + kAng90;
				pView->picnum	= 4535;
				pView->x 		= cx;
				pView->y 		= cy;
				pView->z		= cz;
				pView->pal		= 10;
			}
			break;
	}
	
	if (pView)
	{
		t = 128;
		
		if (h)
		{
			pView->cstat |= kSprTransluc2;
			if (searchstat == nType && searchindex == nID)
				t = 64;
		}
		
		size = ClipRange(approxDist(pView->x - posx, pView->y - posy)/t, 32, 128);
		
		pView->owner  	= nFSpr;
		pView->shade 	= 63;
		pView->pal		= 1;
		pView->xrepeat	= size;
		pView->yrepeat  = size;
	}
	
}

void viewProcessSprites(int x, int y, int z, int a) {
	
	static int i, dx, dy, nOctant, nSpr, nXSpr, nTile;
	static int voxType, offset, nView, nTileNew;
	spritetype* pTSpr;
	XSPRITE* pTXSpr;
	
	if (gSysTiles.wallHglt > 0)
	{
		if (gHovWall >= 0)
		{
			if (gHovStat == OBJ_MASKED)
				viewWallHighlight(gHovWall, sectorofwall(gHovWall), 0x40);
			else if (searchwallcf)
				viewWallHighlight(gHovWall, searchsector, 0x10);
			else
				viewWallHighlight(gHovWall, searchsector, 0x20);
		}
	}
	
	if (gListGrd.Length())
	{
		OBJECT* pDb = gListGrd.Ptr();
		while(pDb->type != OBJ_NONE)
		{
			viewObjectHighlight(pDb->type, pDb->index);
			pDb++;
		}
	}
	
	i = spritesortcnt;
	while(--i >= 0)
	{
		pTSpr = &tsprite[i]; nSpr = pTSpr->owner;				
		nXSpr = (pTSpr->extra > 0) ? pTSpr->extra : 0;
		pTXSpr = (nXSpr) ? &xsprite[nXSpr] : NULL;
		
		voxType = offset = 0; nTile = pTSpr->picnum;
		nTile = ClipRange(nTile, 0, kMaxTiles);
		nView = panm[nTile].view;

		dassert(nSpr >= 0 && nSpr < kMaxSprites);

		pTSpr->shade = (schar)ClipRange(viewSpriteShade(pTSpr->shade, nTile, pTSpr->sectnum), -128, 127);
				
		if (nView >= kSprViewVox)
		{
			if (isExternalModel(nTile))
			{
				if (gPreviewMode || gMisc.externalModels == 1)
					voxType = kVoxTypeExternal;
			}
			else if (rngok(voxelIndex[nTile], 0, kMaxVoxels))
			{
				voxType = kVoxTypeInternal;
			}
			
			if (!voxType)
				nView = (viewType[nTile] < kSprViewVox) ? viewType[nTile] : kSprViewSingle;
		}
	
		if (nSpr == gHovSpr && gSysTiles.wallHglt > 0)
			viewSpriteHighlight(pTSpr);
		
		if (!gPreviewMode)
		{
			if (nSpr == gHovSpr)
			{
				switch (pTSpr->cstat & 48) {
					case kSprWall:
						if (pTSpr->cstat & kSprOneSided) break;
						// no break
					case kSprFace:
						if (nView == kSprViewVox || nView == kSprViewVoxSpin) break;
						viewAddEffect(i, kViewEffectAngle);
						break;
				}
			}
				
			if ((h) && (nSpr == gHighSpr || TestBitString(hgltspri, nSpr)))
				viewAddEffect(i, kViewEffectHighlight);
		}
		else if (showinvisibility && (sprite[nSpr].cstat & kSprInvisible))
			pTSpr->cstat |= kSprTransluc1;
		
		
		if (nXSpr > 0)
		{
			switch (sprite[nSpr].type) {
				case kSwitchToggle:
				case kSwitchOneWay:
					offset = xsprite[nXSpr].state;
					break;
				case kSwitchCombo:
					offset = xsprite[nXSpr].data1;
					break;
			}
		}
				
		if (!voxType)
			viewRotateSprite(pTSpr, nView, x, y, &offset); // rotate ViewFullN sprites

		for (; offset > 0; offset-- )
			pTSpr->picnum += 1 + panm[pTSpr->picnum].frames;

		if (pTSpr->statnum != kStatFX)
		{
			switch (pTSpr->type) {
				case kDecorationCandle:
					if (!pTXSpr || pTXSpr->state > 0)
					{
						pTSpr->shade = -128;
						viewAddEffect(i, kViewEffectCandleHalo);
					} 
					else
					{
						pTSpr->shade = -8;
					}
					break;
				case kDecorationTorch:
					if (!pTXSpr || pTXSpr->state > 0)
					{
						pTSpr->picnum++;
						viewAddEffect(i, kViewEffectTorchHigh);
					}
					else 
					{
						viewAddEffect(i, kViewEffectSmokeHigh);
					}
					break;
				case kMarkerDudeSpawn:
					viewAddEffect(i, kViewEffectMiniDude);
					break;
				case kModernCustomDudeSpawn:
					viewAddEffect(i, kViewEffectMiniCustomDude);
					break;
				case kDudeModernCustom:
					viewAddEffect(i, kViewEffectCdudeVersion);
					break;
				default:
					if (!(pTSpr->flags & kHitagSmoke)) break;
					viewAddEffect(i, kViewEffectSmokeHigh);
					break;
			}
		}	
				
		if (!tilesizx[pTSpr->picnum] && gSysTiles.icoNoTile && pTSpr->statnum != kStatMarker)
		{
			pTSpr->picnum = gSysTiles.icoNoTile;
			pTSpr->shade = -128;
			pTSpr->cstat &= ~(kSprFlipX|kSprFlipY);
		}
		
		if (nXSpr && seqGetStatus(OBJ_SPRITE, nXSpr) > 0)
		{
			if (sprite[nSpr].flags & 1024) pTSpr->cstat |= kSprFlipX;
			if (sprite[nSpr].flags & 2048) pTSpr->cstat |= kSprFlipY;
		}
		
		if (voxType)
		{
			switch(voxType) {
				case kVoxTypeInternal:
					pTSpr->cstat &= ~(kSprFlipX|kSprFlipY); // no flip for RFF voxels
					pTSpr->yoffset += panm[pTSpr->picnum].ycenter;
					pTSpr->picnum   = voxelIndex[pTSpr->picnum];
					pTSpr->cstat   |= kSprVoxel;
					break;
				case kVoxTypeExternal:
					pTSpr->picnum = tileGetPic(pTSpr->picnum); // animate
					break;
			}

			if (panm[nTile].view == kSprViewVoxSpin)
				pTSpr->ang = (short)((totalclock * 12) & kAngMask);
		}
		
		sectortype *pSect = &sector[pTSpr->sectnum];
		if (pSect->extra > 0)
		{
			XSECTOR *pXSect = &xsector[pSect->extra];
			if (pXSect->coloredLights)
				pTSpr->pal = (char)pSect->floorpal;
		}
	}


}