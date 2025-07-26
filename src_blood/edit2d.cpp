/*********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Updated by NoOne.
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
#include "xmpmaped.h"
#include "tracker.h"
#include "hglt.h"
#include "aadjust.h"
#include "xmpexplo.h"
#include "mapcmt.h"
#include "nnexts.h"


SCREEN2D gScreen2D;
DOOR_ROTATE* pGDoorR = NULL;
DOOR_SLIDEMARKED* pGDoorSM = NULL;
CIRCLEWALL* pGCircleW = NULL;
LOOPSHAPE* pGLShape = NULL;
LOOPBUILD* pGLBuild = NULL;
SECTAUTOARC* pGArc = NULL;
LOOPSPLIT* pGLoopSplit = NULL;

short sectorhighlight;		// which sector mouse is inside in 2d mode
short pointdrag = -1;

void CIRCLEWALL::Start(int nWall, int x, int y)
{
	int i = LENGTH(coords);
	line	= nWall;
	count	= 6;
	cenx 	= x;
	ceny 	= y;
	
	while(--i >= 0)
	{
		coords[i].x = x;
		coords[i].y = y; 
	}
	
	Setup(x, y);
}

void CIRCLEWALL::Stop()
{
	line = -1;
}

void CIRCLEWALL::Setup(int x, int y)
{
	int32_t x1, y1, x2, y2;
	int64_t ang1, ang2, angdif, radius;
	int64_t i, j;
		
	getWallCoords(line, &x1, &y1, &x2, &y2);
	adjustmark(&x,&y,line);
	
	i = dmulscale4(x -  x2, x1 - x, y1 - y, y  - y2);
	j = dmulscale4(y1 - y2, x1 - x, y1 - y, x2 - x1);
	
	if (j != 0)
	{
		cenx = ((x1 + x2) + scale(y1 - y2, i, j)) >> 1;
		ceny = ((y1 + y2) + scale(x2 - x1, i, j)) >> 1;
		
		ang1   = getangle(x1 - cenx, y1 - ceny);
		ang2   = getangle(x2 - cenx, y2 - ceny);
		angdif = (ang2 - ang1) & kAngMask;
		if (mulscale4(x - x1, y2 - y1) < mulscale4(x2 - x1, y - y1))
			angdif = -((ang1 - ang2) & kAngMask);
				
		i = cenx - x1; j = ceny - y1;
		radius = ksqrt(dmulscale4(i, i, j, j))<<2;
		
		for (i = 1; i < count; i++)
		{
			j = (ang1 + scale(i, angdif, count)) & kAngMask;
			x = cenx + mulscale14(sintable[(j + kAng90) & kAngMask], radius);
			y = ceny + mulscale14(sintable[j], radius);
			
			if (shift)
				helperDoGridCorrection(&x, &y);
			
			x = ClipRange(x, -boardWidth, boardWidth);
			y = ClipRange(y, -boardHeight, boardHeight);
			
			coords[i].x = x;
			coords[i].y = y;
		}
		
		coords[0].x = x1;	coords[count].x = x2;
		coords[0].y = y1;	coords[count].y = y2;
	}
}

void CIRCLEWALL::Draw(SCREEN2D* pScr)
{
	walltype* pWall = &wall[line];
	LINE2D lines[LENGTH(coords)];
	int i, x1, y1, x2, y2;
	char fc, bc;
	
	for (i = 0; i <= count; i++)
		memcpy(&lines[i], &coords[i], sizeof(coords[0])<<1);
	
	// finish the loop
	lines[count].x2 = lines[0].x1;
	lines[count].y2 = lines[0].y1;		
	
	fc = pScr->ColorGet(kColorGrey26);
	if (pScr->prefs.useTransluc)
	{
		gfxTranslucency(1);
		pScr->FillPolygon(lines, i, fc, 1);
        gfxTranslucency(0);
	}
	else
	{
		pScr->FillPolygon(lines, i, fc);
	}
	
	while(--i > 0)
	{
		x1 = pScr->cscalex(coords[i].x);	x2 = pScr->cscalex(coords[i-1].x);
		y1 = pScr->cscaley(coords[i].y);	y2 = pScr->cscaley(coords[i-1].y);
		
		fc = pScr->GetWallColor(line, false);
		pScr->DrawLine(x1, y1, x2, y2,  fc, pWall->cstat & kWallBlock);
		
		fc = pScr->ColorGet(kColorGrey28);
		bc = pScr->ColorGet(kColorBrown);
		pScr->DrawVertex(x1, y1, fc, bc, pScr->vertexSize);
		pScr->DrawVertex(x2, y2, fc, bc, pScr->vertexSize);
	}
		
	x1 = pScr->cscalex(cenx); y1 = pScr->cscaley(ceny);
	pScr->DrawIconCross(x1, y1, pScr->ColorGet(kColorYellow), 6);

}

void CIRCLEWALL::Insert()
{
	int nSectA = sectorofwall(line);
    int nSectB = -1;
    int i, j;

    if ((j = wall[line].nextwall) >= 0)
        nSectB = sectorofwall(j);

    i = count;
	while(--i > 0)
	{
		j = rngok(wall[line].nextwall, 0, line);
		insertPoint(line,coords[i].x, coords[i].y);
		line += j;
	}
    
    if (nSectA >= 0 && nSectB >= 0)
    {
        sectLoopTransfer(nSectA, nSectB);
        sectLoopTransfer(nSectB, nSectA);
    }
    
	CleanUp();
}

char allWallsOfSectorInHglt(int nSect)
{
	int s, e;
	getSectorWalls(nSect, &s, &e);
	while(s <= e)
	{
		if (!TestBitString(hgltwall, s++))
			return 0;
	}
	
	return 1;
	
}

int countSpritesAt(int x, int y)
{
	int i, j = 0, k = 0;
	for (i = 0; k < numsprites && i < kMaxSprites; i++)
	{
		if (sprite[i].statnum >= kMaxStatus) continue;
		else if (sprite[i].x == x && sprite[i].y == y) j++;
		k++;
	}
	
	return j;
}

int findUnusedChannel(DIALOG_ITEM* dialog) {

	BOOL used[1024];
	int i, s, e;
	
	// search in objects...
	collectUsedChannels(used);
	
	// search in dialog...
	if (dialog != NULL && dialog != dlgXSectorFX)
	{
		int nType = dialog->value;
		for (DIALOG_ITEM *control = dialog; control->type != CONTROL_END; control++)
		{
			// get rx and tx id fields
			if (control->tabGroup < 2) continue;
			else if (control->tabGroup == 2 || control->tabGroup == 3) used[control->value] = TRUE;
			else if (dialog != dlgXSprite) // below must be a sprite dialog
				break;

			// data fields
			if (nType == kModernRandomTX || nType == kModernSequentialTX)
			{
				if (control->tabGroup < kSprDialogData1) continue;
				if (chlRangeIsFine(control->value)) used[control->value] = TRUE;
				if (control->tabGroup >= kSprDialogData4)
					break;
			}
		}
	}
	
	// get first unused channel
	for(i = 100; i < LENGTH(used); i++)
	{
		if (!used[i])
			return i;
	}
	
	return -1;
}

int findUnusedPath(DIALOG_ITEM* dialog, DIALOG_ITEM *control) {


	BOOL FOUND;
	int i = 0, id = -1, j = 0;
	control = control;

	id = 0;
	while(id < 32767) {

		// search in dialog...
		if (dialog == dlgXSprite) {

			FOUND = FALSE;
			for (DIALOG_ITEM *control = dialog; control->type != CONTROL_END; control++) {

				if (control->tabGroup < kSprDialogData1) continue;
				else if (id == control->value) {
					FOUND = TRUE;
					break;
				}

				if (control->tabGroup >= kSprDialogData2)
					break;
			}

			if (FOUND) {
				id++; continue;
			}

		}

		// search in objects
		for (i = headspritestat[kStatPathMarker]; i != -1; i = nextspritestat[i]) {
			if (sprite[i].extra <= 0 || sprite[i].type != kMarkerPath) continue;

			XSPRITE* pXSpr = &xsprite[sprite[i].extra];
			if (pXSpr->data1 == id || pXSpr->data2 == id)
				break;

		}

		if (i == -1)
			return id;

		id++;

	}

	return -1;
}

int findUnusedStack()
{
	int i = numsectors, s, e;
	IDLIST stacks(true);
	spritetype* pSpr;
	walltype* pWall;
	
	while(--i >= 0) // collect all the stack data
	{
		getSectorWalls(i, &s, &e);
		while(s <= e)
		{
			pWall = &wall[s++];
			if (pWall->extra > 0 && pWall->type == kWallStack)
				stacks.AddIfNot(xwall[pWall->extra].data);
		}
		
		
		for (s = headspritesect[i]; s >= 0; s = nextspritesect[s])
		{
			pSpr = &sprite[s];
			if (pSpr->extra > 0
				&& irngok(pSpr->type, kMarkerLowLink, kMarkerLowGoo) && pSpr->type != kMarkerWarpDest)
					stacks.AddIfNot(xsprite[pSpr->extra].data1);
			
			if (stacks.GetLength() >= 64)
				return -1;
		}
	}
	
	while(++i < 64) // search for free id	
	{
		if (!stacks.Exists(i))
			return i;
	}
	
	return -1;

}

void getclosestpointonwall(int x, int y, int nWall, int *nx, int *ny)
{
	int32_t x1, y1, x2, y2;
	int64_t dx, dy, i, j;
	
	*nx = *ny = 0;
	if (nWall >= 0)
	{
		getWallCoords(nWall, &x1, &y1, &x2, &y2);
		dx = x2 - x1; dy = y2 - y1;
		
		i = dx*(x-x1) + dy*(y-y1);
		if (i <= 0)
		{
			*nx = x1;
			*ny = y1;
			return;
		}
		
		j = dx*dx + dy*dy;
		if (i >= j)
		{
			*nx = x2;
			*ny = y2;
			return;
		}

		i=((i<<15)/j)<<15;
		*nx = x1 + mulscale30(dx,i);
		*ny = y1 + mulscale30(dy,i);
	}
}

int getlinehighlight(int nTresh, int x, int y, int nZoom)
{
	int i, dst, dist, closest, xi, yi;
	dist = divscale14(nTresh, nZoom);
	closest = -1;

	for (i = 0; i < numwalls; i++)
	{
		int lx = wall[wall[i].point2].x - wall[i].x;
		int ly = wall[wall[i].point2].y - wall[i].y;
		int qx = x - wall[i].x;
		int qy = y - wall[i].y;

		// is point on wrong side of wall?
		if ( qx * ly > qy * lx )
			continue;

		int num = dmulscale(qx, lx, qy, ly, 4);
		int den = dmulscale(lx, lx, ly, ly, 4);

		if (num <= 0 || num >= den)
			continue;

		xi = wall[i].x + scale(lx, num, den);
		yi = wall[i].y + scale(ly, num, den);
		dst = approxDist(xi - x, yi - y);
		
		if (dst <= dist)
		{
			dist = dst;
			closest = i;
		}
	}

	return closest;
}


int getpointhighlight(int nTresh, int x, int y, int nZoom)
{
	int closest = divscale14(nTresh, nZoom);
    int nSect = -1, i = 0, n = -1;
    int d, s, e;
    
    for (i = 0; i < numsectors; i++)
    {
        for (s = headspritesect[i]; s >= 0; s = nextspritesect[s])
        {
			// last closest sprite
			if ((d = approxDist(x - sprite[s].x, y - sprite[s].y)) <= closest)
			{
				n = s | 0x4000;
                closest = d;
                nSect = i;
			}
        }
        
        getSectorWalls(i, &s, &e);
        while(s <= e)
        {
            if ((d = approxDist(x - wall[s].x, y - wall[s].y)) < closest)
            {
                closest = d;
                nSect = i;
                n = s;
            }
            
            s++;
        }
    }
    
    if (n >= 0 && (n & 0xc000) == 0 && sectorhighlight >= 0 && nSect != sectorhighlight)
    {
        getSectorWalls(sectorhighlight, &s, &e);
        while(s <= e)
        {
            if ((d = approxDist(x - wall[s].x, y - wall[s].y)) <= closest)
            {
                closest = d;
                n = s;
            }
            
            s++;
        }
    }
    
	return n;
}

void clampsprite_TEMP(short nspr) {
	
	spritetype* pspr = &sprite[nspr];
	int templong = ((tilesizy[pspr->picnum]*pspr->yrepeat)<<2);
	int cz = getceilzofslope(pspr->sectnum,pspr->x,pspr->y);
	int fz = getflorzofslope(pspr->sectnum,pspr->x,pspr->y);

	pspr->z = min(pspr->z, fz);
	if (!(pspr->cstat & 0x0020))
	{
		if (pspr->z + templong < cz)
			pspr->z = max(pspr->z, cz + templong);
		
		if (pspr->z > fz)
			pspr->z = min(pspr->z, fz);
	}
	else
	{
		pspr->z = max(pspr->z, cz);
		pspr->z = min(pspr->z, fz);
	}
}

int getSectOf(int nType, int nID)
{
    switch(nType)
    {
        case OBJ_WALL:
        case OBJ_MASKED:
            return sectorofwall(nID);
        case OBJ_SPRITE:
            return sprite[nID].sectnum;
        case OBJ_FLOOR:
        case OBJ_CEILING:
        case OBJ_SECTOR:
            return nID;
    }
    
    return -1;
}

void ProcessInput2D( void )
{
	static int capstat = 0, capx = 0, capy = 0, spridx = -1;
	int x, y, j = 0, k = 0, i = 0, sect = -1;
	int nStat, nThick, swal, ewal; char* msg;
	int r;
	
	INPUTPROC* pInput;
	
	short *hgltcntptr = NULL;
	short ACTION = -1, idx = -1;
	char keyhold = 0;
	
	gMouse.Read();
	searchx = ClipRange(searchx + gMouse.dX2, gMouse.left, gMouse.right);
	searchy = ClipRange(searchy + gMouse.dY2, gMouse.top,  gMouse.bottom);

	gScreen2D.GetPoint(searchx, searchy, &x, &y);
	
	if (!(gMouse.buttons & 1) || sectorhighlight < 0)
		updatesector(x, y, &sectorhighlight);
	
	
	// !!! for build.c
	mousxplc = x; mousyplc = y;
	
	if (gMouse.hold & 2)
	{
		cursectnum = sectorhighlight;
		searchx = gScreen2D.view.wcx;
		searchy = gScreen2D.view.wcy;
		posx = x;
		posy = y;
	}
		
	gMouse.X = searchx;
	gMouse.Y = searchy;
	gMouse.Draw();
	

	linehighlight = getlinehighlight(gMisc.hgltTreshold, x, y, zoom);
	if (!(gMouse.buttons & 5))
	{
		cmthglt = gCommentMgr.ClosestToPoint(gMisc.hgltTreshold, x, y, zoom);
		i = pointhighlight = getpointhighlight(gMisc.hgltTreshold, x, y, zoom);
		if (linehighlight < 0 && i >= 0 && (i & 0x4000) == 0)
		{
			if (wall[i].nextwall >= 0 && sectorofwall(i) != sectorhighlight)
			{
				linehighlight = wall[i].nextwall;
			}
			else
			{
				linehighlight = i;
			}
		}
	}

	if (!gPreviewMode && pointhighlight >= 0 && (pointhighlight & 0x4000) != 0)
	{
		gScreen2D.data.sprUnderCursor = countSpritesAt(sprite[searchwall].x, sprite[searchwall].y);
		if (gScreen2D.data.sprUnderCursor > 1)
		{
			if (alt && gMouse.wheel)
			{
				if (totalclock >= gTimers.mouseWheelTimer1)
				{
					while( 1 )
					{
						if ((i = nextSpriteAt(sprite[searchwall].x, sprite[searchwall].y, &spridx)) >= 0)
						{
							scrSetMessage("Focus on sprite #%d at X=%d, Y=%d", sprite[i].index, sprite[i].x, sprite[i].y);
							ChangeSpriteSect(sprite[i].index, sprite[i].sectnum); // put sprite in bottom of the spritesect array
							BeepOk();
							break;
						}
					}
					
					gTimers.mouseWheelTimer1 = totalclock + 16;
				}
				
				// get new pointhighlight on next call?
				return;
			}
		}
	}
	
	if (gMouse.wheel < 0 | keystatus[KEY_A])
	{
		if (gMouse.wheel < 0 | !((ctrl && shift) | (ctrl && !shift)))
			zoom = ClipHigh(zoom + gFrameTicks * (zoom/0x18), 0x010000);
	}
	else if (gMouse.wheel > 0 | keystatus[KEY_Z])
	{
		if (gMouse.wheel > 0 | !((ctrl && shift) | (ctrl && !shift)))
			zoom = ClipLow(zoom  - gFrameTicks * (zoom/0x18), 0x00020);
	}
	
	if (grid > 0)
		doAutoGrid();
		
	if (!gPreviewMode)
	{
		if (cmthglt >= 0)
		{
			if (gMouse.press & 4)
			{
				gMouse.press &= ~4;
			}
			else if (gMouse.hold & 4)
			{
				if ((cmthglt & 0xc000) == 0)
					gCommentMgr.SetXYBody(cmthglt, x, y);
				else
					gCommentMgr.SetXYTail(cmthglt & 0x3FFF, x, y);
			}
			else if (gMouse.release & 4)
			{
				asksave = 1;
			}
		}
		
		if (gMouse.hold & 1)
		{
			helperDoGridCorrection(&x, &y);
			
			// create shape loop
			if (pGLShape)
			{
				if (!capstat)
				{
					pGLShape->Start(sectorhighlight, x, y);
					pGLShape->StatusSet(1);	// started
					capstat = 1;
				}
			}
			// drag highlight sectors
			else if (highlightsectorcnt > 0)
			{
				if (!capstat) // capture mouse coords first
				{
					for (i = 0; i < highlightsectorcnt; i++)
					{
						// check mouse coords because sectorhighlight could be wrong (ROR is a fine example)
						if (inside(x, y, highlightsector[i]) <= 0) continue;
						capx = x, capy = y;
						capstat = 1;
						break;
					}
				}
				else // drag
				{
					// a hack to keep sectors panning correctly
					// see fixupPan() function to understand it
					if (!shift && (!gridlock || !grid || grid > 6))
						doGridCorrection(&x, &y, 6);
					
					x-=capx, y-=capy; capx+=x, capy+=y;
					hgltSectCallFunc(sectChgXY, x, y, (shift) ? 0x02 : 0x03);
					if (capstat == 1)
					{
						hgltSectDetach();
						capstat++;
					}
					
					if (hgltCheck(OBJ_SECTOR, startsectnum) >= 0)
						startposx+=x, startposy+=y;
				}
			}
			else if (pointhighlight >= 0)
			{
				if ((pointhighlight & 0xc000) == 0)
				{
					searchstat = OBJ_WALL;
					searchwall = pointhighlight;
				}
				else
				{
					searchstat = OBJ_SPRITE;
					searchwall = pointhighlight & 16383;
				}
				
				pointdrag = pointhighlight;
				
				if (hgltCheck(searchstat, searchwall) >= 0)
				{
					if (searchstat == OBJ_SPRITE)
					{
						x -= sprite[searchwall].x;
						y -= sprite[searchwall].y;
					}
					else
					{
						x -= wall[searchwall].x;
						y -= wall[searchwall].y;
					}
					
					if (!shift && hgltWallCount() > 0)
					{
						i = numsectors;
						while(--i >= 0)
						{
							if (allWallsOfSectorInHglt(i))
							{
								// a hack to keep sectors panning correctly
								// see fixupPan() function to understand it
								if (!gridlock || !grid || grid > 6)
									doGridCorrection(&x, &y, 6);
								
								for (j = 0; j <= i; j++)
								{
									if (allWallsOfSectorInHglt(j))
										sectChgXY(j, x, y, 0x01);
								}
								
								break;
							}
						}
					}
					
					for (i = 0; i < highlightcnt; i++)
					{
						j = highlight[i];
						if ((j & 0xc000) == 0)
						{
							sect = sectorofwall(j);
							if (shift || (!shift && !allWallsOfSectorInHglt(sect)))
							{
								wall[j].x += x;
								wall[j].y += y;
							}
						}
						else
						{
							j = (j & 0x3FFF);
							spritetype* pSpr =& sprite[j];
							
							if (pSpr->statnum < kMaxStatus)
							{
                                pSpr->x += x, pSpr->y += y;

                                sect = pSpr->sectnum;
                                if (FindSector(pSpr->x, pSpr->y, pSpr->z, &sect) || FindSector(pSpr->x, pSpr->y, &sect))
                                {
                                    if (sect != pSpr->sectnum)
                                        ChangeSpriteSect(pSpr->index, sect);
                                }
							}
						}
					}
				}
				else if (searchstat == OBJ_SPRITE)
				{
					spritetype* pSpr =& sprite[searchwall];
					pSpr->x = x, pSpr->y = y;

					sect = ClipLow((sectorhighlight >= 0) ? sectorhighlight : pSpr->sectnum, 0);
					if (FindSector(pSpr->x, pSpr->y, pSpr->z, &sect) || FindSector(pSpr->x, pSpr->y, &sect))
					{
						if (sect != pSpr->sectnum)
							ChangeSpriteSect(pSpr->index, sect);
					}
					
					clampsprite_TEMP(searchwall);
					
					if (linehighlight >= 0 && alt)
					{
						int x1, y1;
						getclosestpointonwall(pSpr->x, pSpr->y, linehighlight, &x1, &y1);
						if (sectorofwall(linehighlight) == pSpr->sectnum)
							pSpr->ang = (GetWallAngle(linehighlight) + kAng90) & kAngMask;
					}
				}
				else if (shift & 0x01)
				{
					 wall[searchwall].x = x;
					 wall[searchwall].y = y;
				}
				else // drag all wall points at same xy
				{
					dragpoint(searchwall, x, y);
				}
			}
			else if (highlightcnt <= 0 && !(alt & 0x02)) // highlight walls/sprites
			{
				shift |= 0x02;
			}
		}
		else if (gMouse.press & 4)
		{
            getHighlightedObject();
            
            switch (searchstat)
            {
				case OBJ_SPRITE:
				case OBJ_WALL:
					if (!(shift & 0x01))
					{
						hgltReset(kHgltSector); // reset highlight sectors
						switch (searchstat) {
							case OBJ_WALL:
								// need a wall point here
								if (linehighlight >= 0)
								{
									if ((pointhighlight & 0xc000) == 0)
									{
										idx = pointhighlight;
										break;
									}
									else
									{
										scrSetMessage("Must select wall point.");
										break;
									}
								}
								// no break
							case OBJ_SPRITE:
								idx = searchwall;
								break;
						}
						break;
					}
					else if ((searchsector = getSector()) == -1) break;
					else searchstat = OBJ_FLOOR; // convert to sector
					// no break
				case OBJ_FLOOR:
					if (!(shift & 0x01)) break;
					hgltReset(kHgltPoint); // reset highlight points
					searchstat = OBJ_SECTOR; // for names
					idx = searchsector;
					break;
			}
            
            if (Beep(idx >= 0))
            {
                int nSect, ms, me;
                int cnt = 0;

                if (ctrl && (nSect = getSectOf(searchstat, idx)) >= 0)
                {
                    if (alt)
                        hgltReset(kHgltPoint);
                    
                    switch (searchstat)
                    {
                        case OBJ_WALL:
                            sectLoopMain(nSect, &ms, &me);
                            if (irngok(idx, ms, me))    getSectorWalls(nSect, &swal, &ewal);
                            else                        loopGetWalls(idx, &swal, &ewal);
                            
                            for (i = swal; i <= ewal && hgltCheck(searchstat, i) >= 0; i++);
                            
                            ACTION = (i <= ewal);
                            for (i = swal; i <= ewal; i++)
                                cnt += ((ACTION) ? hglt2dAdd(searchstat, i) : hglt2dRemove(searchstat, i));
                            
                            break;
                        case OBJ_SPRITE:
                            for (i = headspritesect[nSect]; i >= 0
                                && hgltCheck(searchstat, i) >= 0; i = nextspritesect[i]);
                            
                            ACTION = (i >= 0);
                            for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
                                cnt += ((ACTION) ? hglt2dAdd(searchstat, i) : hglt2dRemove(searchstat, i));
                            
                            break;
                    }
                }
                else
                {
                    ACTION = (hgltCheck(searchstat, idx) < 0);
                    cnt = (ACTION) ? hglt2dAdd(searchstat, idx)
                                        : hglt2dRemove(searchstat, idx);
                }
                
                if (ACTION == 1)
                {
                    scrSetMessage("%d %s(s) was added in a highlight.", cnt, gSearchStatNames[searchstat]);
                }
                else
                {
                    scrSetMessage("%d %s(s) was removed from a highlight.", cnt, gSearchStatNames[searchstat]);
                }
                
                Beep(cnt);
            }
		}
		else if (gMouse.release & 1)
		{
			capstat = 0;
			
			if (pGLShape)
			{
				if (pGLShape->StatusGet() == 1)
					pGLShape->StatusSet(2);	// finished drawing
			}
			else
			{
				if (highlightsectorcnt > 0)
					hgltSectAttach(), asksave = 1;

				if (pointdrag >= 0)
					asksave = 1;
				
				if ((pointdrag & 0xc000) == 0)
				{
					int x1, y1, x2, y2, x3, y3;
					int nNext;
					
					j = pointdrag;
					getWallCoords(j, &x3, &y3);
					worldSprCallFunc(sprFixSector, 0x01);
					
					if (wall[j].nextwall < 0 && (j = sectorofwall(j)) >= 0)
						sectAttach(j);
					
 					i = numwalls;
					while(--i >= 0)
					{
						getWallCoords(i, &x1, &y1, &x2, &y2);
						if ((x1 == x2 && y1 == y2) && ((x3 == x1 && y3 == y1) || (x3 == x2 && y3 == y2)))
							deletePoint(i);
					}
										
					// fix repeats for walls
					for (i = 0; i < numwalls; i++)
					{
						k = wall[i].point2;
						getWallCoords(i, &x1, &y1, &x2, &y2);
						
						// current wall
						if (!(wall[i].cstat & kWallMoveMask)) // skip kinetic motion marked
						{
							if (x1 == x3 && y1 == y3)
								fixrepeats(i); // must be i here!
						}
						
						// wall that connects to current one
						if (!(wall[k].cstat & kWallMoveMask))  // skip kinetic motion marked
						{
							if (x2 == x3 && y2 == y3)
								fixrepeats(i); // must be i here!
						}
					}
				}
				
				pointdrag = -1;
			}
		}
		
		if (pointdrag < 0)
		{
			if (alt & 0x02)
			{
				hgltReset(kHgltPoint);
				hgltType = keyhold = 0;
				if (!vel)
					hgltType = kHgltSector, keyhold = 1;
			}
			else if (shift & 0x02)
			{
				hgltReset(kHgltSector);
				hgltType = keyhold = 0;
				if (!vel)
					hgltType = kHgltPoint, keyhold = 1;
			}
		}

		if (hgltType)
		{
			switch(hgltType)
			{
				case kHgltPoint:
					hgltcntptr =& highlightcnt;
					break;
				default:
					hgltcntptr =& highlightsectorcnt;
					break;
			}
			
			if (keyhold)
			{
				if (*hgltcntptr == 0)
				{
					hgltx2 = mousxplc, hglty2 = mousyplc;
				}
				else
				{
					hgltReset();
					hgltx2 = mousxplc, hglty2 = mousyplc;
					hgltx1 = mousxplc, hglty1 = mousyplc;
					*hgltcntptr = 0;
				}
			}
			else if (*hgltcntptr == 0)
			{
				if (hgltx1 > hgltx2) swapValues(&hgltx1, &hgltx2);
				if (hglty1 > hglty2) swapValues(&hglty1, &hglty2);
				
				hglt2dAddInXYRange(hgltType, hgltx1, hglty1, hgltx2, hglty2);				
				hgltType = 0;
				
				if (*hgltcntptr <= 0)
					*hgltcntptr = -1;
			}
		}
		
		if (pGLBuild)
		{
			msg = buffer;
			msg+=sprintf(buffer, "Sector draw:");
			
            helperDoGridCorrection(&x, &y);
            
            if (shift
                && pointhighlight >= 0 && (pointhighlight & 0xc000) == 0)
                    getWallCoords(pointhighlight, &x, &y);

			pGLBuild->Setup(x, y);
			if ((nStat = pGLBuild->StatusGet()) >= 0)
			{
				msg+=sprintf(msg, " Press SPACE to");
				switch(nStat)
				{
					case 0:
						gMapedHud.SetMsgImp(16, "%s insert or BACKSPACE to remove a point", buffer);
						break;
					case 1:
						gMapedHud.SetMsgImp(16, "%s split sector", buffer);
						break;
					case 2:
						gMapedHud.SetMsgImp(16, "%s create a split line", buffer);
						break;
					case 3:
						gMapedHud.SetMsgImp(16, "%s insert walls", buffer);
						break;
					case 4:
					case 5:
                    case 7:
						gMapedHud.SetMsgImp(16, "%s create new sector", buffer);
						break;
                    case 6:
						gMapedHud.SetMsgImp(16, "%s auto-complete loop and create new sector", buffer);
						break;
				}
				
				switch(key)
				{
					case KEY_SPACE:
						switch(nStat)
						{
							case 0: 
								pGLBuild->AddPoint(x, y);
                                BeepOk();
                                break;
							default:
								pGLBuild->Make();
								DELETE_AND_NULL(pGLBuild);
								updatesector(posx, posy, &cursectnum);
                                BeepOk();
								break;
						}
						key = 0;
						break;
				}
				
			}
			else if ((msg = retnCodeCheck(nStat, gLoopBuildErrors)) != NULL)
			{
				gMapedHud.SetMsgImp(16, "%s %s", buffer, msg);
			}
		}
		else if (pGLShape)
		{
			nStat = pGLShape->StatusGet();
			sprintf(buffer, "Loop shape");
			
			OBJECT obj;
			obj.type = somethingintab; obj.index = tempidx;
			if (pGLShape->SectorGet() >= 0 && sectorhighlight >= 0)
				obj.type = OBJ_FLOOR, obj.index = sectorhighlight;
		
			if (capstat)
				pGLShape->Setup(x, y, &obj);
			
			if (nStat >= 0 || (msg = retnCodeCheck(nStat, gLoopShapeErrors)) == NULL)
			{
				if (nStat)
				{
					if (nStat == 1)
						gMapedHud.SetMsgImp(16, "%s: width: %d, height: %d, points: %d", buffer, pGLShape->Width(), pGLShape->Height(), pGLShape->NumPoints());
					
					if (nStat == 2)
						gMapedHud.SetMsgImp(16, "%s: Press SPACE to insert loop", buffer);
				}
				else
				{
					gMapedHud.SetMsgImp(16, "%s: Hold LEFT MOUSE and move to draw shape", buffer);
				}
			}
			else
			{
				gMapedHud.SetMsgImp(16, "%s error: %s", buffer, msg);
			}
			
			
			switch(key)
			{
				case KEY_PLUS:
				case KEY_MINUS:
				case KEY_SPACE:
				case KEY_COMMA:
				case KEY_PERIOD:
					switch(key)
					{
						case KEY_SPACE:
							if (!alt)
							{
								if (Beep(nStat > 0))
								{
									pGLShape->Insert(); pGLShape->StatusSet(0);
									updatesector(posx, posy, &cursectnum);
								}
							}
							break;
						case KEY_PLUS:
							pGLShape->AddPoints();
							break;
						case KEY_MINUS:
							pGLShape->RemPoints();
							break;
						case KEY_PERIOD:
							pGLShape->AddAngle();
							break;
						case KEY_COMMA:
							pGLShape->RemAngle();
							break;
							
					}
					if (key != KEY_SPACE || !alt)
					{
						key = 0;
						BeepOk();
					}
					break;
			}
			
		}
		else if (pGCircleW)
		{
			gMapedHud.SetMsgImp(16, "Press SPACE to insert %d points here", pGCircleW->count);
			pGCircleW->Setup(mousxplc, mousyplc);
			
			switch(key)
			{
				case KEY_PLUS:
				case KEY_MINUS:
				case KEY_SPACE:
					switch(key)
					{
						case KEY_PLUS:
						case KEY_MINUS:
							pGCircleW->SetPoints((key == KEY_PLUS));
							gMisc.circlePoints = pGCircleW->count;
							pGCircleW->Setup(mousxplc, mousyplc);
							break;
						case KEY_SPACE:
							scrSetMessage("Circle points inserted");
							pGCircleW->Insert();
							DELETE_AND_NULL(pGCircleW);
							break;
					}
					key = 0;
					BeepOk();
					break;
			}
		}
		else if (pGDoorSM || pGDoorR)
		{
			sprintf(buffer, "Door wizard");
			nThick = (grid <= 0) ? 128 : (2048 >> grid);
			
			if (pGDoorSM)
			{
				if (linehighlight >= 0)
				{
					x = mousxplc, y = mousyplc;
					i = linehighlight;

					if (!shift)
						helperDoGridCorrection(&x, &y);
				
					getclosestpointonwall(x, y, i, &x, &y);
					if (!(pGDoorSM->Setup(i, nThick, x, y)))
					{
						msg = retnCodeCheck(pGDoorSM->StatusGet(), gDoorWizardErrors);
						if (msg)
							gMapedHud.SetMsgImp(16, "%s error: %s", buffer, msg);
					}
					else
					{
						gMapedHud.SetMsgImp(16, "%s: Press SPACE to insert %s-sided door here", buffer,
						(pGDoorSM->info.twoSides) ? "DOUBLE" : "ONE");
					}
					
					switch(key)
					{
						case KEY_PLUS:
						case KEY_MINUS:
						case KEY_SPACE:
						case KEY_F2:
							switch(key)
							{
								case KEY_PLUS:
									pGDoorSM->SetPadding(pGDoorSM->info.padding + 32);
									break;
								case KEY_MINUS:
									pGDoorSM->SetPadding(pGDoorSM->info.padding - 32);
									break;
								case KEY_F2:
									pGDoorSM->prefs.twoSides^=1;
									break;
								case KEY_SPACE:
									if (pGDoorSM->StatusGet() > 0)
									{
										i = pGDoorSM->info.sector;
										sectortype* pSect = &sector[i];
										switch(pSect->type)
										{
											case kSectorSlide:
											case kSectorSlideMarked:
												if (!Confirm("This sector already a slider. Continue anyway?"))
													break;
											default:
												pGDoorSM->Insert();
												DELETE_AND_NULL(pGDoorSM);
												BeepOk();
												break;
										}
									}
									break;
							}
							key = 0;
							BeepOk();
							break;
					}
				}
				else
				{
					pGDoorSM->StatusClear();
					gMapedHud.SetMsgImp(16, "%s: Hover a solid wall", buffer);
				}
			}
			else if (pGDoorR)
			{
				if (linehighlight >= 0)
				{
					x = mousxplc; y = mousyplc;
					i = linehighlight;
					
					if (!shift)
						helperDoGridCorrection(&x, &y);
					
					getclosestpointonwall(x, y, i, &x, &y);
					pGDoorR->Setup(i, nThick, x, y);
					nStat = pGDoorR->StatusGet();
					
					if (nStat < 0)
					{
						msg = retnCodeCheck(nStat, gDoorWizardErrors);
						if (msg)
							gMapedHud.SetMsgImp(16, "%s error: %s", buffer, msg);
					}
					else
					{
						gMapedHud.SetMsgImp(16, "%s: Press SPACE to insert %s-sided door here", buffer,
						(pGDoorR->info.twoSides) ? "DOUBLE" : "ONE");
					}
					
					switch(key)
					{
						case KEY_F1:
						case KEY_F2:
						case KEY_F3:
						case KEY_F4:
						case KEY_PLUS:
						case KEY_MINUS:
						case KEY_SPACE:
							switch(key)
							{
								case KEY_PLUS:
								case KEY_MINUS:
									if (key == KEY_PLUS)
										pGDoorR->info.rotateAngle+=16;
									else
										pGDoorR->info.rotateAngle-=16;
									
									pGDoorR->info.rotateAngle = ClipRange(pGDoorR->info.rotateAngle, -kAng45, kAng45);
									break;
								case KEY_F2:
									pGDoorR->info.twoSides^=1;
									break;
								case KEY_F1:
									pGDoorR->info.dir^=1;
									break;
								case KEY_F3:
									pGDoorR->info.inside^=1;
									break;
								case KEY_F4:
									pGDoorR->info.mode = IncRotate(pGDoorR->info.mode, 3);
									break;
								case KEY_SPACE:
								{
									char a1 = pGDoorR->info.side[0].available;
									char a2 = pGDoorR->info.side[1].available;
									if (!a1 || (pGDoorR->info.twoSides && !a2))
									{
										if (!Confirm("One or more doors may be outside the sector.\nInsert anyway?"))
											break;										
									}
									
									pGDoorR->Insert();
									DELETE_AND_NULL(pGDoorR);
									break;
								}
							}
							key = 0;
							BeepOk();
							break;
					}
				}
				else
				{
					pGDoorR->StatusClear();
					gMapedHud.SetMsgImp(16, "%s: Hover a wall", buffer);
				}
			}
		}
		else if (pGArc)
		{
			msg = buffer;
			msg += sprintf(buffer, "Arc wizard");
			
			if (sectorhighlight >= 0 && hgltCheck(OBJ_SECTOR, sectorhighlight) >= 0)
			{
				pGArc->ClearList();
				pGArc->insertNew = 0;
				for (i = 0; i < highlightsectorcnt; i++)
					pGArc->slist->Add(highlightsector[i]);
			}
			else
			{
				pGArc->insertNew = 1;
			}
						
			pGArc->Setup(linehighlight, grid);
			
			switch(nStat = pGArc->StatusGet())
			{
				case 1:
					gMapedHud.SetMsgImp(16, "%s: Press SPACE to build arc from %d pieces", buffer, pGArc->NumPieces());
					break;
				case 2:
					gMapedHud.SetMsgImp(16, "%s: Press SPACE to build arc from %d sectors", buffer, highlightsectorcnt);
					break;
				default:
					if (nStat >= 0 || (msg = retnCodeCheck(nStat, gAutoArcErrors)) == NULL) break;
					gMapedHud.SetMsgImp(16, "%s error #%d: %s", buffer, klabs(nStat), msg);
					break;
			}
			
			switch (key)
			{
				case KEY_SPACE:
					if (pGArc->StatusGet() > 0)
					{
						if (dlgArcWizard(pGArc))
						{
							if ((nStat = pGArc->Insert()) < 0
								&& (msg = retnCodeCheck(nStat, gAutoArcErrors)) != NULL)
									Alert("Error #%d: %s",  klabs(nStat), msg);
							
							updatesector(posx, posy, &cursectnum);
							DELETE_AND_NULL(pGArc);
							Beep(nStat > 0);
						}
					}
					key = 0;
					break;
				case KEY_PLUS:
				case KEY_MINUS:
					if (pGArc->insertNew)
					{
						if (!shift) Beep(pGArc->IterateSideStep(key == KEY_PLUS, grid));
						else Beep(pGArc->IterateMidStep(key == KEY_PLUS, grid));
						break;
					}
					BeepFail();
					break;
			}
		}
		else if (pGLoopSplit)
		{
			msg = buffer;
			msg += sprintf(buffer, "Loop Split");

			pGLoopSplit->Setup(linehighlight);
			switch(nStat = pGLoopSplit->StatusGet())
			{
				case 1:
					gMapedHud.SetMsgImp(16, "%s: Press SPACE to split a sector", buffer);
					break;
				default:
					if (nStat >= 0 || (msg = retnCodeCheck(nStat, gLoopSplitErrors)) == NULL) break;
					gMapedHud.SetMsgImp(16, "%s error #%d: %s", buffer, klabs(nStat), msg);
					break;
			}

			switch(key)
			{
				case KEY_PLUS:
				case KEY_MINUS:
					if (!shift) Beep(pGLoopSplit->IterateAngleOffset(key == KEY_PLUS));
					else Beep(pGLoopSplit->ToggleSkipFlag());
					break;
				case KEY_SPACE:
					if (Beep(nStat > 0))
					{
						pGLoopSplit->Insert();
						DELETE_AND_NULL(pGLoopSplit);
					}
					key = 0;
					break;
			}
		}
	}
	
	pInput = &gEditInput2D[key];
	
	if (pInput->pFunc)
	{
		r = pInput->pFunc(key, ctrl, shift, alt);
		
		if (r & PROC_OK)
		{
			switch(r & PROC_UNDO_MASK)
			{
				case PROC_UNDO_CMP: asksave = 1; break;
				case PROC_UNDO_ADD: asksave = 2; break;
			}
		}
		
		if (r & PROC_BEEP)
			Beep(r & PROC_OK);
        
        keyClear();
	}
}
