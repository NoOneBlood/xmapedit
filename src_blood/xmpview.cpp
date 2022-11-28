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

#include "common_game.h"
#include "build.h"
#include "xmpstub.h"
#include "xmpview.h"
#include "db.h"
#include "gameutil.h"
#include "screen.h"
#include "tile.h"
#include "aadjust.h"
#include "preview.h"
#include "nnexts.h"
#include "xmptools.h"
#include "xmpmisc.h"
#include "xmpconf.h"
#include "xmpevox.h"
#include "editor.h"
#include "seq.h"

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

void viewAddEffect( int nTSprite, VIEW_EFFECT nViewEffect ) {
	
	if (spritesortcnt >= kMaxViewSprites - 1)
		return;
	
	int i, j, k, f, zTop, zBot, zTop2, zBot2;
	spritetype *pTSprite = &tsprite[nTSprite];

	switch (nViewEffect) {
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
					// it's better to use picnum from autoData if no seq loaded
					if (pXSpr->data2 <= 0 || !getSeqPrefs(pXSpr->data2, &pic, &xr, &yr, &pal))
					{
						if ((k = adjIdxByType(kDudeModernCustom)) < 0)
							break;
						
						pic = autoData[k].picnum;
						pal = autoData[k].plu;
					}
					
					spritetype *pTEffect  = viewInsertTSprite(pTSprite->sectnum, -32767, pTSprite);
					
					// rotate ViewFullN sprites
					viewRotateSprite(pTEffect, viewType[pic], posx, posy, &ofs);
					
					pTEffect->picnum	 = ClipLow(pic + ofs, 0);
					pTEffect->pal		 = ClipLow((pSpr->hitag & kModernTypeFlag1) ? pSpr->pal : pal, 0);
					pTEffect->xrepeat 	 = pTEffect->yrepeat = 8;
					pTEffect->cstat 	|= (kSprTransluc2);
					pTEffect->cstat 	&= ~kSprOrigin;
					pTEffect->z 		 = zTop;
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
	
	spritetype* pEffect;
	int nAng  = (GetWallAngle(nWall) + kAng90) & kAngMask;
	int zTop, zBot, nLen = getWallLength(nWall);
	int lHeigh = klabs(point[2].z - point[0].z);
	int rHeigh = klabs(point[3].z - point[1].z);
	int nFSpr;
	
	BOOL badfslope = (sector[nSect].floorslope != 0);
	BOOL badcslope = (sector[nSect].ceilingslope != 0);
	
	if ((nFSpr = headspritestat[kStatFree]) < 0)
		return;
	
	// left top
	if ((pEffect = viewInsertTSprite(nSect, -32767)) != NULL)
	{
		size = viewWallHighlightScale(nLen, lHeigh, point[0].x, point[0].y);

		pEffect->picnum 	= gSysTiles.wallHglt;
		pEffect->cstat 	   |= (kSprFlipX | kSprOneSided);
		
		// leave sprite face oriented if we cannot see it because of slope
		if (!badcslope)
			pEffect->cstat |= kSprWall;
		
		if (!h)
			pEffect->cstat |= kSprTransluc2;
		
		pEffect->xrepeat 	= pEffect->yrepeat = size;
		pEffect->shade 		= -128;
		pEffect->ang 		= nAng;
		
		pEffect->x 			= point[0].x;
		pEffect->y 			= point[0].y;
		pEffect->z 			= point[0].z;
		
		pEffect->owner 		= nFSpr;
		
		doWallCorrection(nWall, &pEffect->x, &pEffect->y);
		GetSpriteExtents(pEffect, &zTop, &zBot);
		pEffect->z = zBot;
	}
	
	// right top
	if ((pEffect = viewInsertTSprite(nSect, -32767)) != NULL)
	{
		size = viewWallHighlightScale(nLen, rHeigh, point[1].x, point[1].y);
		
		pEffect->picnum 	= gSysTiles.wallHglt;
		pEffect->cstat 	 	|= (kSprOneSided);
		
		// leave sprite face oriented if we cannot see it because of slope
		if (!badcslope)
			pEffect->cstat |= kSprWall;
		
		if (!h)
			pEffect->cstat |= kSprTransluc2;
		
		pEffect->xrepeat 	= pEffect->yrepeat = size;
		pEffect->shade 		= -128;
		pEffect->ang 		= nAng;
		
		pEffect->x 			= point[1].x;
		pEffect->y 			= point[1].y;
		pEffect->z 			= point[1].z;
		
		pEffect->owner 		= nFSpr;
		
		doWallCorrection(nWall, &pEffect->x, &pEffect->y);
		GetSpriteExtents(pEffect, &zTop, &zBot);
		pEffect->z = zBot;
	}
	
	// left bot
	if ((pEffect = viewInsertTSprite(nSect, -32767)) != NULL)
	{
		size = viewWallHighlightScale(nLen, lHeigh, point[2].x, point[2].y);
		
		pEffect->picnum 	= gSysTiles.wallHglt;
		pEffect->cstat 	   |= (kSprFlipX | kSprFlipY | kSprOneSided);
		
		// leave sprite face oriented if we cannot see it because of slope
		if (!badfslope)
			pEffect->cstat |= kSprWall;
		
		if (!h)
			pEffect->cstat |= kSprTransluc2;
		
		pEffect->xrepeat 	= pEffect->yrepeat = size;
		pEffect->shade 		= -128;
		pEffect->ang 		= nAng;
		
		pEffect->x 			= point[2].x;
		pEffect->y 			= point[2].y;
		pEffect->z 			= point[2].z;
		
		pEffect->owner 		= nFSpr;
		
		doWallCorrection(nWall, &pEffect->x, &pEffect->y);
		GetSpriteExtents(pEffect, &zTop, &zBot);
		pEffect->z = zTop;
	}
	
	//right bot
	if ((pEffect = viewInsertTSprite(nSect, -32767)) != NULL)
	{
		size = viewWallHighlightScale(nLen, rHeigh, point[3].x, point[3].y);
		
		pEffect->picnum 	= gSysTiles.wallHglt;
		pEffect->cstat 	   |= (kSprFlipY | kSprOneSided);
		
		// leave sprite face oriented if we cannot see it because of slope
		if (!badfslope)
			pEffect->cstat |= kSprWall;
		
		if (!h)
			pEffect->cstat |= kSprTransluc2;
		
		pEffect->xrepeat 	= pEffect->yrepeat = size;
		pEffect->shade 		= -128;
		pEffect->ang 		= nAng;
		
		pEffect->x 			= point[3].x;
		pEffect->y 			= point[3].y;
		pEffect->z 			= point[3].z;
		
		pEffect->owner 		= nFSpr;
		
		doWallCorrection(nWall, &pEffect->x, &pEffect->y);
		GetSpriteExtents(pEffect, &zTop, &zBot);
		pEffect->z = zTop;
	}
}

void viewWallHighlight(int nWall, int nSect, char how, BOOL testPoint)
{
	char which = 0;
	int x1, y1, x2, y2;
	int cz1, cz2, fz1, fz2;
	int nNext = wall[nWall].nextsector;
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
			getzsofslope(nNext, x1, y1, &point[0].z, &point[2].z); 			// next left top bot
			getzsofslope(nNext, x2, y2, &point[1].z, &point[3].z); 			// next right top bot
			
			if (!testPoint || (point[2].z != point[0].z && point[3].z != point[1].z))
				viewWallHighlight(nWall, nSect, (POINT3D*)point);
		}
	}
}

void viewProcessSprites(int x, int y, int z, int a) {
	
	static int i, dx, dy, nOctant, nSprite, nXSprite, nTile, nShade;
	static int voxType, offset, nView, nTileNew;
	
	if (gSysTiles.wallHglt > 0 && gHovWall >= 0)
	{
		if (gHovStat == OBJ_MASKED)
			viewWallHighlight(gHovWall, sectorofwall(gHovWall), 0, FALSE);
		else if (searchwallcf)
			viewWallHighlight(gHovWall, searchsector, 0x10);
		else
			viewWallHighlight(gHovWall, searchsector, 0x20);
	}
	
/* 	if (searchsector >= 0)
	{
		int s, e;
		getSectorWalls(searchsector, &s, &e);
		
		int nFSpr;
		if ((nFSpr = headspritestat[kStatFree]) < 0)
			return;
		
		
		while(s <= e)
		{
			
			//int zTop, int zBot;
			int x1, y1;
			getWallCoords(s, &x1, &y1);
			
			unsigned char size;
			
			spritetype* pEffect;
			if ((pEffect = viewInsertTSprite(searchsector, -32767)) != NULL)
			{
				pEffect->picnum 	= gSysTiles.pixel;
				pEffect->cstat 	   |= (kSprFloor);
				
				if (!h)
					pEffect->cstat |= kSprTransluc2;
				
				
				pEffect->shade 		= -128;
				pEffect->ang 		= GetWallAngle(s) + kAng90;
				
				pEffect->x 			= x1;
				pEffect->y 			= y1;
				pEffect->z 			= getflorzofslope(searchsector, x1, y1);
				
				pEffect->owner 		= nFSpr;
				
				size = (unsigned char)ClipRange(mulscale8(64, approxDist(x1-posx, y1-posy)>>5), 4, 164);
				pEffect->xrepeat 	= pEffect->yrepeat = size;
				
				doWallCorrection(s, &pEffect->x, &pEffect->y);
				//GetSpriteExtents(pEffect, &zTop, &zBot);
				//pEffect->z = zTop;
			}
			
			s++;
		}
		
	} */

	for (i = spritesortcnt - 1; i >= 0; i--)
	{
		spritetype *pTSprite = &tsprite[i];
		XSPRITE *pTXSprite = (pTSprite->extra >= 0) ? &xsprite[pTSprite->extra] : NULL;
		nXSprite = pTSprite->extra;
		
		voxType = offset = 0; nTile = pTSprite->picnum;
		dassert(nTile >= 0 && nTile < kMaxTiles);
		
		nView = panm[nTile].view;
		
		nSprite = pTSprite->owner;
		dassert(nSprite >= 0 && nSprite < kMaxSprites);

		nShade = pTSprite->shade;
		sectortype *pSector = &sector[pTSprite->sectnum];
		XSECTOR *pXSector = (pSector->extra > 0) ? &xsector[pSector->extra] : NULL;
		pTSprite->shade = (schar)ClipRange(viewSpriteShade(nShade, nTile, pTSprite->sectnum), -128, 127);
				
		if (nView >= kSprViewVox)
		{
			if (extVoxelPath[nTile])
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

		if (!gPreviewMode)
		{
			if (nSprite == gHovSpr)
			{
				switch (pTSprite->cstat & 48) {
					case kSprWall:
						if (pTSprite->cstat & kSprOneSided) break;
						// no break
					case kSprFace:
						if (nView == kSprViewVox || nView == kSprViewVoxSpin) break;
						viewAddEffect(i, kViewEffectAngle);
						break;
				}
			}
				
			if ((h) && (nSprite == gHighSpr || TestBitString(show2dsprite, nSprite)))
				viewAddEffect(i, kViewEffectHighlight);
		}
		else if (showinvisibility && (sprite[nSprite].cstat & kSprInvisible))
			pTSprite->cstat |= kSprTransluc1;
		
		
		if (nXSprite > 0)
		{
			switch (sprite[nSprite].type) {
				case kSwitchToggle:
				case kSwitchOneWay:
					offset = xsprite[nXSprite].state;
					break;
				case kSwitchCombo:
					offset = xsprite[nXSprite].data1;
					break;
			}
		}
				
		if (!voxType)
			viewRotateSprite(pTSprite, nView, x, y, &offset); // rotate ViewFullN sprites

		for (; offset > 0; offset-- )
			pTSprite->picnum += 1 + panm[pTSprite->picnum].frames;

		if (pTSprite->statnum == 0)
		{
			switch (pTSprite->type) {
				case kDecorationCandle:
					if (!pTXSprite || pTXSprite->state > 0)
					{
						pTSprite->shade = -128;
						viewAddEffect(i, kViewEffectCandleHalo);
					} 
					else
					{
						pTSprite->shade = -8;
					}
					break;
				case kDecorationTorch:
					if (!pTXSprite || pTXSprite->state > 0)
					{
						pTSprite->picnum++;
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
				default:
					if (!(pTSprite->flags & kHitagSmoke)) break;
					viewAddEffect(i, kViewEffectSmokeHigh);
					break;
			}
		}	
		
		if (!tilesizx[pTSprite->picnum] && gSysTiles.noTileIco && pTSprite->statnum != kStatMarker)
		{
			pTSprite->picnum = gSysTiles.noTileIco;
			pTSprite->shade = -128;
			pTSprite->cstat &= ~(kSprFlipX|kSprFlipY);
		}
		
		if (nXSprite > 0 && seqGetStatus(OBJ_SPRITE, nXSprite) > 0)
		{
			if (sprite[nSprite].flags & 1024) pTSprite->cstat |= kSprFlipX;
			if (sprite[nSprite].flags & 2048) pTSprite->cstat |= kSprFlipY;
		}
		
		if (voxType)
		{
			switch(voxType) {
				case kVoxTypeInternal:
					pTSprite->cstat &= ~(kSprFlipX|kSprFlipY); // no flip for RFF voxels
					pTSprite->yoffset += panm[pTSprite->picnum].ycenter;
					pTSprite->picnum   = voxelIndex[pTSprite->picnum];
					pTSprite->cstat   |= kSprVoxel;
					break;
				case kVoxTypeExternal:
					pTSprite->picnum = tileGetPic(pTSprite->picnum); // animate
					break;
			}

			if (panm[nTile].view == kSprViewVoxSpin)
				pTSprite->ang = (short)((totalclock * 12) & kAngMask);
		}

		if (pXSector && pXSector->coloredLights)
			pTSprite->pal = (char)pSector->floorpal;
	}


}