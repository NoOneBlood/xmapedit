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


#include "compat.h"
#include "baselayer.h"
#include "common_game.h"
#include "db.h"
#include "gameutil.h"
#include "xmpror.h"
#include "xmpview.h"
#include "xmpconf.h"
#include "xmparted.h"
#include "gui.h"
#include "editor.h"

short mirrorcnt = 0, mirrorsector, mirrorwall[4];
short mirrorPicWidth, mirrorPicHeight;
MIRROR mirror[kMaxROR];

int CreateMirrorPic(int nObjTile, BOOL copy = TRUE)
{
	PICANM* pnm = &panm[nObjTile];
	register int o = 0, nNewTile, nRetn = nObjTile;

	if ((nNewTile = tileSearchFreeRange(pnm->frames)) >= 0)
	{
		nRetn = nNewTile;
		if (pnm->frames)
			o = (pnm->type == 3) ? -pnm->frames : pnm->frames;
		
		if (o < 0) // backwards animation...
		{
			o = klabs(o);
			mirror[mirrorcnt].basePic = nNewTile + o;
			nObjTile -= o;
		}
		else
		{
			mirror[mirrorcnt].basePic = nNewTile;
		}
		
		while (o-- >= 0)
		{
			gSysTiles.add(nNewTile);
			if (copy)
				artedCopyTile(nObjTile++, nNewTile);
			
			nNewTile++;
		}
	}
	
	return nRetn;
}

// functions for ROR drawing
//////////////////////////////////
void RestoreMirrorPic()
{
	tilesizx[kMirrorPic] = mirrorPicWidth;
	tilesizy[kMirrorPic] = mirrorPicHeight;
}

void ClearMirrorPic()
{
	mirrorPicWidth  = tilesizx[kMirrorPic];
	mirrorPicHeight = tilesizy[kMirrorPic];
	tilesizx[kMirrorPic] = 0;
	tilesizy[kMirrorPic] = 0;
}

void InitMirrors()
{
	sectortype* pSect; walltype* pWall;
	register int i, j, k, nSect, nLink, nLink2;
	
	ClearMirrorPic();
	
	for (i = 0; i < mirrorcnt; i++)
	{
		k = mirror[i].basePic;
		PICANM pnm = panm[k];
		if (pnm.type != 3)
		{
			for (j = k; j <= k + pnm.frames; j++)
			{
				tileFreeTile(j);
				gSysTiles.rem(j);
			}
		}
		else
		{
			for (j = k + pnm.frames; j >= k; j--)
			{
				tileFreeTile(j);
				gSysTiles.rem(j);
			}
		}
	}
	
	mirrorcnt = 0;
	
	i = numwalls; // prepare wall mirrors and stacks
	while(mirrorcnt < kMaxROR && --i >= 0)
	{
		pWall =& wall[i];
		if (IsMirrorPic(pWall->picnum))
		{
			pWall->cstat 		   |= kWallOneWay;
			pWall->picnum 			= CreateMirrorPic(pWall->picnum, FALSE);
			
			mirror[mirrorcnt].type  = OBJ_WALL;
			mirror[mirrorcnt].id 	= i;
			mirrorcnt++;
		}
		else if (IsMirrorPic(pWall->overpicnum) && pWall->extra > 0 && pWall->type == kWallStack)
		{
			j = numwalls;
			while(--j >= 0)
			{
				if (j == i || wall[j].extra <= 0) continue;
				else if (wall[j].type != kWallStack) continue;
				else if (xwall[wall[j].extra].data == xwall[pWall->extra].data)
				{
					pWall->cstat 		   |= kWallOneWay;
					pWall->overpicnum		= CreateMirrorPic(pWall->overpicnum, FALSE);
					pWall->hitag  			= j;
					wall[j].hitag 			= i;
					
					mirror[mirrorcnt].type 	= OBJ_WALL;
					mirror[mirrorcnt].id  	= j;
					mirrorcnt++;
					break;
				}
			}
		}
	}
	
	if (mirrorcnt > 0)
	{
		// create a room to translate the mirror
		mirrorsector = numsectors;
		for (i = 0; i < 4; i++)
		{
			mirrorwall[i] 					= numwalls+i;
			wall[mirrorwall[i]].picnum 		= 504;
			wall[mirrorwall[i]].overpicnum 	= 504;
			wall[mirrorwall[i]].cstat 		= 0;
			wall[mirrorwall[i]].nextsector 	= -1;
			wall[mirrorwall[i]].nextwall 	= -1;
			wall[mirrorwall[i]].point2 		= numwalls+i+1;
		}
		
		wall[mirrorwall[3]].point2 			= mirrorwall[0];
		sector[mirrorsector].ceilingpicnum 	= 504;
		sector[mirrorsector].floorpicnum 	= 504;
		sector[mirrorsector].wallptr 		= mirrorwall[0];
		sector[mirrorsector].wallnum 		= 4;
		
		if (mirrorcnt >= kMaxROR)
			return;
	}
	
	i = numsectors; // prepare sector stacks
	while(mirrorcnt < kMaxROR && --i >= 0)
	{
		pSect = &sector[i];
		if ((j = IsRorSector(i, OBJ_FLOOR)) > 0)
		{
			if ((nLink = gUpperLink[i]) < 0)
				continue;
			
			nLink2 = sprite[nLink].owner;
			nSect  = sprite[nLink2].sectnum;
			
			spritetype* pLink1 = &sprite[nLink];
			spritetype* pLink2 = &sprite[gLowerLink[nSect]];
			
			pSect->floorpicnum 			= CreateMirrorPic(pSect->floorpicnum, (j == 2));
			mirror[mirrorcnt].type 		= OBJ_FLOOR;
			mirror[mirrorcnt].id 		= nSect;
			mirror[mirrorcnt].point.x	= pLink2->x - pLink1->x;
			mirror[mirrorcnt].point.y	= pLink2->y - pLink1->y;
			mirror[mirrorcnt].point.z	= pLink2->z - pLink1->z;
			mirrorcnt++;
			
			if ((j = IsRorSector(nSect, OBJ_CEILING)) > 0)
			{
				sector[nSect].ceilingpicnum = CreateMirrorPic(sector[nSect].ceilingpicnum, (j == 2));
				mirror[mirrorcnt].type 		= OBJ_CEILING;
				mirror[mirrorcnt].id 		= i;
				mirror[mirrorcnt].point.x	= pLink1->x - pLink2->x;
				mirror[mirrorcnt].point.y	= pLink1->y - pLink2->y;
				mirror[mirrorcnt].point.z	= pLink1->z - pLink2->z;
				mirrorcnt++;
			}
		}
	}
}

void TranslateMirrorColors(int nShade, int nPalette)
{
	register int i, nPixels;
	
	begindrawing();
	
	nShade 			= ClipRange(nShade, 0, 63);
	BYTE *pMap 		= (BYTE*)(palookup[nPalette] + (nShade<<8));
	BYTE *pFrame 	= (BYTE*)frameplace;
	
	nPixels = xdim*ydim;
	for (i = 0; i < nPixels; i++, pFrame++)
		*pFrame = pMap[*pFrame];
	
	enddrawing();
}

bool DrawMirrors(int x, int y, int z, int a, int horiz)
{
	MIRROR* pRor;
	register int i = mirrorcnt, dx, dy, dz;

	while(--i >= 0)
	{
		pRor = &mirror[i];
		if (!TestBitString(gotpic, pRor->basePic))
			continue;
		
		ClearBitString(gotpic, pRor->basePic);
		
		if (pRor->type == OBJ_WALL)
		{
			register short ca;
			register int nSector, nNextWall;
			register int nNextSector;
				
			walltype *pWall = &wall[pRor->id];
			nSector = sectorofwall(pRor->id);
			
			nNextWall = pWall->nextwall;
			nNextSector = pWall->nextsector;
			
			pWall->nextwall					= mirrorwall[0];
			pWall->nextsector 				= mirrorsector;

			wall[mirrorwall[0]].nextwall 	= pRor->id;
			wall[mirrorwall[0]].nextsector 	= nSector;
			wall[mirrorwall[0]].x 			= wall[pWall->point2].x;
			wall[mirrorwall[0]].y 			= wall[pWall->point2].y;
			wall[mirrorwall[1]].x 			= pWall->x;
			wall[mirrorwall[1]].y 			= pWall->y;
			wall[mirrorwall[2]].x 			= wall[mirrorwall[1]].x+(wall[mirrorwall[1]].x-wall[mirrorwall[0]].x)<<4;
			wall[mirrorwall[2]].y 			= wall[mirrorwall[1]].y+(wall[mirrorwall[1]].y-wall[mirrorwall[0]].y)<<4;
			wall[mirrorwall[3]].x 			= wall[mirrorwall[0]].x+(wall[mirrorwall[0]].x-wall[mirrorwall[1]].x)<<4;
			wall[mirrorwall[3]].y 			= wall[mirrorwall[0]].y+(wall[mirrorwall[0]].y-wall[mirrorwall[1]].y)<<4;
			
			sector[mirrorsector].floorz 	= sector[nSector].floorz;
			sector[mirrorsector].ceilingz 	= sector[nSector].ceilingz;

			if (pWall->type == kWallStack)
			{
				 dx = x - (wall[pWall->hitag].x-wall[pWall->point2].x);
				 dy = y - (wall[pWall->hitag].y-wall[pWall->point2].y);
				 ca = a;
			}
			else
			{
				preparemirror(x, y, z, a, horiz, pRor->id, mirrorsector, &dx, &dy, &ca);
			}

			drawrooms(dx, dy, z, ca, horiz, mirrorsector | kMaxSectors);
			viewProcessSprites(dx, dy, z, ca);
			drawmasks();

			if (pWall->type != kWallStack)
				completemirror();
			
			if (pWall->pal || pWall->shade)
				TranslateMirrorColors(pWall->shade, pWall->pal);

			pWall->nextwall = nNextWall;
			pWall->nextsector = nNextSector;
		}
		else
		{
			dx = x + pRor->point.x; dy = y + pRor->point.y; dz = z + pRor->point.z;
			drawrooms(dx, dy, dz, a, horiz, pRor->id | kMaxSectors);
			viewProcessSprites(dx, dy, dz, a);
			drawmasks();
			
			// experimental...
			if (gModernMap)
			{
				int s;
				char nPal = 0, nShade = 0;				
				if (pRor->type == OBJ_FLOOR)
				{
					if ((s = i+1) < mirrorcnt)
					{
						s = mirror[s].id;
						if (sector[s].hitag & 0x01)	nPal   = sector[s].floorpal;
						if (sector[s].hitag & 0x02)	nShade = sector[s].floorshade;
					}
				}
				else if ((s = i-1) >= 0)
				{
					s = mirror[s].id;
					if (sector[s].hitag & 0x01)	nPal   = sector[s].ceilingpal;
					if (sector[s].hitag & 0x02)	nShade = sector[s].ceilingshade;
				}
				
				if (nShade || nPal)
					TranslateMirrorColors(nShade, nPal);
			}
		}
		
		return true;
	}
	
	return false;
}

bool IsMirrorPic(int nPic) { return (nPic == 504); }
char IsRorSector(int nSect, int stat)
{
	if (stat == OBJ_FLOOR)
	{
		if (IsMirrorPic(sector[nSect].floorpicnum))		 			return 1;
		else if ((sector[nSect].floorstat & kSectTranslucR) != 0)	return 2;
		else return 0;
	}
	else if (IsMirrorPic(sector[nSect].ceilingpicnum))				return 1;
	else if ((sector[nSect].ceilingstat & kSectTranslucR) != 0)		return 2;
	else return 0;
}

bool IsLinkCorrect(spritetype* pSpr)
{
	if (!pSpr || pSpr->statnum >= kMaxStatus) return false;
	else if (!rngok(pSpr->owner, 0, kMaxSprites)) return false;
	
	spritetype* pSpr2 = &sprite[pSpr->owner];
	if (pSpr2->owner != pSpr->index)
		return false;
	
	return true;
}




// functions to wrap through ROR links
//////////////////////////////////
void warpInit(void)
{
    register int i, j, t;
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
}

int CheckLink(int *x, int *y, int *z, int *nSector)
{
    register int z1, z2;
	register int nUpper = gUpperLink[*nSector];
    register int nLower = gLowerLink[*nSector];
	
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

int CheckLink(spritetype *pSprite)
{
	register int nLink = 0;
	register int nSector = pSprite->sectnum;
	if (!rngok(nSector, 0, numsectors)) return 0;
	else if ((nLink = CheckLink(&pSprite->x, &pSprite->y, &pSprite->z, &nSector)) > 0)
	{
		if (nSector != pSprite->sectnum)
			ChangeSpriteSect(pSprite->index, nSector);
	}
	
	return nLink;
}

int CheckLinkCamera(int *x, int *y, int *z, int *nSector)
{
	register int nSpr, nLink = 0;
	register int px = *x, py = *y, pz = *z, nSect = *nSector;
	if (rngok(nSect, 0, numsectors) && (nLink = CheckLink(&px, &py, &pz, &nSect)) > 0)
	{
		if (gModernMap && gPreviewMode)
		{
			if ((nSpr = gUpperLink[*nSector]) < 0 || sprite[nSpr].type != nLink)
			{
				if ((nSpr = gLowerLink[*nSector]) < 0 || sprite[nSpr].type != nLink)
					return 0;
			}
			
			spritetype* pSpr = &sprite[nSpr];
			if (pSpr->flags & 0x01)
				return 0;
		}
		
		*x = px, *y = py, *z = pz; *nSector = nSect;
	}
	
	return nLink;
}