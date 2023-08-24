/**********************************************************************************
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
#include "xmpstub.h"
#include "tracker.h"
#include "gameutil.h"
#include "db.h"
#include "edit2d.h"
#include "xmpsnd.h"

#include "mapcmt.h"
#include "nnexts.h"
#include "gui.h"
#include "tile.h"
#include "editor.h"
#include "hglt.h"
#include "aadjust.h"
#include "preview.h"
#include "xmpview.h"
#include "xmpmisc.h"
#include "xmpexplo.h"
#include "xmptools.h"
#include "xmpconf.h"
#include "edit3d.h"
#include "sectorfx.h"


DOOR_ROTATE* pGDoorR = NULL;
DOOR_SLIDEMARKED* pGDoorSM = NULL;
CIRCLEWALL* pGCircleW = NULL;

short sectorhighlight;		// which sector mouse is inside in 2d mode
short pointdrag = -1;

void CIRCLEWALL::Start(int nWall, int x, int y)
{
	int i = LENGTH(coords);
	lockSectorDrawing();
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
	unlockSectorDrawing();
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
			
			if (shift && gridlock && grid)
				doGridCorrection(&x, &y, grid);
			
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
	
	fc = pScr->ColorGet(kClrCircleFill, h);
	if (pScr->prefs.useTransluc)
	{
		pScr->LayerOpen();
		pScr->FillPolygon(lines, i, fc, 1);
		pScr->LayerShowAndClose(kRSTransluc);
	}
	else
	{
		pScr->FillPolygon(lines, i, fc);
	}
	
	while(--i)
	{
		x1 = pScr->cscalex(coords[i].x);		x2 = pScr->cscalex(coords[i-1].x);
		y1 = pScr->cscaley(coords[i].y);	y2 = pScr->cscaley(coords[i-1].y);
		
		fc = pScr->GetWallColor(line, false);
		pScr->DrawLine(x1, y1, x2, y2,  fc, pWall->cstat & kWallBlock);
		
		fc = pScr->ColorGet(kClrVtxBord, h);
		bc = pScr->ColorGet(kClrVtxFill, h);
		pScr->DrawVertex(x1, y1, fc, bc, pScr->vertexSize);
		pScr->DrawVertex(x2, y2, fc, bc, pScr->vertexSize);
	}
		
	x1 = pScr->cscalex(cenx); y1 = pScr->cscaley(ceny);
	fc = pScr->ColorGet(kClrCircleCenter, 0);
	pScr->DrawIconCross(x1, y1, fc, 6);

}

void CIRCLEWALL::Insert()
{
	int i, j;
	i = count;
	while(--i > 0)
	{
		j = rngok(wall[line].nextwall, 0, line);
		insertpoint(line,coords[i].x, coords[i].y);
		line += j;
	}
	
	CleanUp();
}

char allWallsOfSectorInHglt(int nSect)
{
	int s, e;
	getSectorWalls(nSect, &s, &e);
	while(s <= e)
	{
		if (!TestBitString(show2dwall, s++))
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

int nextSpriteAt(int x, int y, int *prv)
{
	int i;
	for (i = ClipLow(*prv, 0); i < kMaxSprites; i++)
	{
		if (sprite[i].statnum >= kMaxStatus) continue;
		else if (sprite[i].x == x && sprite[i].y == y)
		{
			if (*prv == -1 || *prv < i)
			{
				*prv = i;
				return i;
			}
		}
	}
	
	*prv = -1;
	return -1;
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
	int i = numsectors, j;
	IDLIST stacks(true);
	spritetype* pSpr;
	
	while(--i >= 0) // collect all the stack data
	{
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			pSpr = &sprite[j];
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
	int j, d, i = 0, n = -1;
	int closest = divscale14(nTresh, nZoom);
	while(i < numsectors)
	{
		j = headspritesect[i++];
		while(j >= 0)
		{
			// last closest sprite
			if ((d = approxDist(x - sprite[j].x, y - sprite[j].y)) <= closest)
			{
				closest = d;
				n = j | 0x4000;
			}
			
			j = nextspritesect[j];
		}
	}
	
	for (i = 0; i < numwalls; i++)
	{
		// first closest wall
		if ((d = approxDist(x - wall[i].x, y - wall[i].y)) < closest)
		{
			closest = d;
			n = i;
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

void ProcessKeys2D( void )
{
	static int capstat = 0, capx = 0, capy = 0, spridx = -1;
	int x, y, j = 0, k = 0, z = 0, i = 0, type = 0, sect = -1;
	int dax = 0, day = 0, daz = 0, swal, ewal;
	
	short *hgltcntptr = NULL;
	short sect2 = -1, ACTION = -1, idx = -1;
	char keyhold = 0;
	
	processMove();
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
	
	/// !!!
	if (gHudPrefs.dynamicLayout2D)
	{
		if (newnumwalls >= numwalls)
		{
			if (gHudPrefs.layout2D == kHudLayoutFull)
			{
				gHudPrefs.layout2D = kHudLayoutCompact;
				hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
			}
		}
		else if (gHudPrefs.layout2D != kHudLayoutFull)
		{
			gHudPrefs.layout2D = kHudLayoutFull;
			hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
		}
	}
	
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

	if ((type = getHighlightedObject()) == 200 && !gPreviewMode)
	{
		gScreen2D.data.sprUnderCursor = countSpritesAt(sprite[searchwall].x, sprite[searchwall].y);
		if (gScreen2D.data.sprUnderCursor > 1)
		{
			if (alt && (gMouse.buttons & 48))
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

	if (keystatus[KEY_A] || (gMouse.buttons & 16))	zoom = ClipHigh(zoom + gFrameTicks*(zoom/0x18), 0x010000);
	else if (keystatus[KEY_Z] || (gMouse.buttons & 32)) zoom = ClipLow(zoom  - gFrameTicks*(zoom/0x18), 0x00020);

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
			
			if ((cmthglt & 0xc000) != 0)
				gMapedHud.SetComment(&gCommentMgr.comments[cmthglt & 0x3FFF]);
		}
		
		if (gMouse.hold & 1)
		{
			if (gridlock)
				doGridCorrection(&x, &y, grid);
			
			// drag highlight sectors
			if (highlightsectorcnt > 0)
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
					
					if (gCommentMgr.commentsCount)
					{
						gCommentMgr.cmtins = (char*)realloc(gCommentMgr.cmtins, gCommentMgr.commentsCount);
						memset(gCommentMgr.cmtins, 0, gCommentMgr.commentsCount);
						for (i = 0; i < highlightsectorcnt; i++)
						{
							j = highlightsector[i];
							for (k = 0; k < gCommentMgr.commentsCount; k++)
							{
								MAP_COMMENT* cmt =& gCommentMgr.comments[k];
								if (inside(cmt->cx, cmt->cy, j) > 0) gCommentMgr.cmtins[k] |= 0x01;
								if (inside(cmt->tx, cmt->ty, j) > 0) gCommentMgr.cmtins[k] |= 0x02;
							}
						}
					}
				}
				else // drag
				{
					// a hack to keep sectors panning correctly
					// see fixupPan() function to understand it
					if (!gridlock || !grid || grid > 6)
						doGridCorrection(&x, &y, 6);
					
					x-=capx, y-=capy; capx+=x, capy+=y;
					hgltSectCallFunc(sectChgXY, x, y);
					
					for (i = 0; i < gCommentMgr.commentsCount; i++)
					{
						MAP_COMMENT* cmt =& gCommentMgr.comments[i];
						if (gCommentMgr.cmtins[i] & 0x01) cmt->cx+=x, cmt->cy+=y;
						if (gCommentMgr.cmtins[i] & 0x02) cmt->tx+=x, cmt->ty+=y;
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
					
					for (i = 0; i < numsectors; i++)
					{
						if (allWallsOfSectorInHglt(i))
						{
							// a hack to keep sectors panning correctly
							// see fixupPan() function to understand it
							if (!gridlock || !grid || grid > 6)
								doGridCorrection(&x, &y, 6);
							
							sectChgXY(i, x, y);
						}
					}
					
					for (i = 0; i < highlightcnt; i++)
					{
						j = highlight[i];
						if ((j & 0xc000) == 0)
						{
							sect = sectorofwall(j);
							if (!allWallsOfSectorInHglt(sect))
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
								if (!allWallsOfSectorInHglt(pSpr->sectnum))
								{
									pSpr->x += x, pSpr->y += y;
									
									sect = pSpr->sectnum;
									if (FindSector(pSpr->x, pSpr->y, &sect) && sect != pSpr->sectnum)
										ChangeSpriteSect(pSpr->index, sect);
								}
							}
						}
					}
				}
				else if (searchstat == OBJ_SPRITE)
				{
					spritetype* pSpr =& sprite[searchwall];
					sect = (sectorhighlight >= 0) ? sectorhighlight : -1;
					pSpr->x = x, pSpr->y = y;
					if (sect >= 0)
					{
						if (sect != pSpr->sectnum) ChangeSpriteSect(pSpr->index, sect);
						if (FindSector(pSpr->x, pSpr->y, pSpr->z, &sect) && sect != pSpr->sectnum)
							ChangeSpriteSect(pSpr->index, sect);
					}
					
					clampsprite_TEMP(searchwall);
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
			switch (searchstat) {
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

			if (idx >= 0)
				ACTION = (hgltCheck(searchstat, idx) >= 0) ? 0 : 1;

			switch (ACTION) {
				case 1:
				case 0:
					if (ctrl && (i = getSector()) >= 0)
					{
						if (alt)
							hgltReset(kHgltPoint);
						
						int cnt = 0;
						switch (searchstat) {
							case OBJ_WALL:
								getSectorWalls(i, &swal, &ewal);
								for (i = swal; i <= ewal; i++)
									cnt+=hglt2dAdd(searchstat, i);
								break;
							case OBJ_SPRITE:
								for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
									cnt+=hglt2dAdd(searchstat, j);
								break;
						}
						
						scrSetMessage("%d %s(s) was added in a highlight.", cnt, gSearchStatNames[searchstat]);
						Beep(cnt);
					}				
					else if (ACTION == 1)
					{
						scrSetMessage("%d %s(s) was added in a highlight.", hglt2dAdd(searchstat, idx), gSearchStatNames[searchstat]);
						BeepOk();
					}
					else
					{
						scrSetMessage("%d %s(s) was removed from a highlight.", hglt2dRemove(searchstat, idx), gSearchStatNames[searchstat]);
						BeepFail();
					}
					break;
				default:
					BeepFail();
					break;
			}
		}
		else if (gMouse.release & 1)
		{
			fixspritesectors();
			capstat = 0;
			CleanUp();
			
			for (i = 0; i < numsectors; i++)
			{
				if (hgltCheck(OBJ_SECTOR, i) < 0)
					sectorAttach(i);
			}
			
			// fix repeats for walls
			if ((pointdrag & 0xc000) == 0)
			{
				int x1, y1, x2, y2, x3, y3;
				int s, k;
				
				j = pointdrag;
				getWallCoords(j, &x3, &y3);
				
				i = numwalls;
				while(--i >= 0)
				{
					getWallCoords(i, &x1, &y1, &x2, &y2);
					if ((x1 == x2 && y1 == y2) && ((x3 == x1 && y3 == y1) || (x3 == x2 && y3 == y2)))
					{
						s = sectorofwall(i);
						
						deletepoint(i);
						if (rngok(s, 0, numsectors) && sector[s].wallnum <= 1)
							sectDelete(s);
						
						for (k = 0; k < numsectors; k++)
							sectorAttach(k);
					}
				}
				
				for (i = 0; i < numwalls; i++)
				{
					k = wall[i].point2;
					
					// current wall
					if (!(wall[i].cstat & kWallMoveMask)) // skip kinetic motion marked
					{
						if (wall[i].x == wall[j].x && wall[i].y == wall[j].y)
							fixrepeats((short)i); // must be i here!
					}
					
					// wall that connects to current one
					if (!(wall[k].cstat & kWallMoveMask))  // skip kinetic motion marked
					{
						if (wall[k].x == wall[j].x && wall[k].y == wall[j].y)
							fixrepeats((short)i); // must be i here!
					}
				}
			}
			
			pointdrag = -1;
		}

		if (alt & 0x02) hgltReset(kHgltPoint), hgltType = kHgltSector, keyhold = 1;
		else if (shift & 0x02)
			hgltReset(kHgltSector), hgltType = kHgltPoint, keyhold = 1;

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
					hgltx2 = searchx, hglty2 = searchy;
				}
				else
				{
					hgltReset();
					hgltx1 = searchx, hglty1 = searchy;
					hgltx2 = searchx, hglty2 = searchy;
					*hgltcntptr = 0;
				}
			}
			else if (*hgltcntptr == 0)
			{
				gScreen2D.GetPoint(&hgltx1, &hglty1);
				gScreen2D.GetPoint(&hgltx2, &hglty2);
				if (hgltx1 > hgltx2) swapValues(&hgltx1, &hgltx2);
				if (hglty1 > hglty2) swapValues(&hglty1, &hglty2);
				
				hglt2dAddInXYRange(hgltType, hgltx1, hglty1, hgltx2, hglty2);
				if (hgltType == kHgltSector)
					hgltDetachSectors();
				
				hgltType = 0;
				if (*hgltcntptr <= 0)
					*hgltcntptr = -1;
			}
		}
		
		if (pGCircleW)
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
					keystatus[key] = 0;
					BeepOk();
					break;
			}
		}
		else if (pGDoorSM || pGDoorR)
		{
			sprintf(buffer, "Door wizard");
			int nWidth = (grid <= 0) ? 128 : (2048 >> grid), nStat;
			char* errMsg;
			
			if (pGDoorSM)
			{
				if (linehighlight >= 0)
				{
					i = linehighlight;
					getclosestpointonwall(mousxplc, mousyplc, i, &x, &y);
					if (!shift && grid)
					{
						if (wallVert(i)) 		doGridCorrection(&x, grid);
						else if (wallHoriz(i))	doGridCorrection(&y, grid);
						else 					doGridCorrection(&x, &y, 10);
					}
									
					if (!(pGDoorSM->Setup(i, nWidth, x, y)))
					{
						errMsg = retnCodeCheck(pGDoorSM->StatusGet(), gDoorWizardErrors);
						if (errMsg)
							gMapedHud.SetMsgImp(16, "%s error: %s", buffer, errMsg);
					}
					else
					{
						gMapedHud.SetMsgImp(16, "%s: Press SPACE to insert %s-sided door here", buffer,
						(pGDoorSM->info.twoSides) ? "DOUBLE" : "ONE");
					}
				}
				else
				{
					pGDoorSM->StatusClear();
					gMapedHud.SetMsgImp(16, "%s: Hover a solid wall", buffer);
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
						keystatus[key] = 0;
						BeepOk();
						break;
				}
			}
			else if (pGDoorR)
			{
				if (linehighlight >= 0)
				{
					i = linehighlight;
					x = mousxplc; y = mousyplc;
					if (!shift && grid && (pGDoorR->info.mode == 2 || (!pGDoorR->info.mode && wall[i].nextsector < 0)))
					{
						if (wallVert(i)) 		doGridCorrection(&x, grid);
						else if (wallHoriz(i))	doGridCorrection(&y, grid);
						else 					doGridCorrection(&x, &y, 10);
					}

					pGDoorR->Setup(i, nWidth, x, y);
					nStat = pGDoorR->StatusGet();
					
					if (nStat < 0)
					{
						errMsg = retnCodeCheck(nStat, gDoorWizardErrors);
						if (errMsg)
							gMapedHud.SetMsgImp(16, "%s error: %s", buffer, errMsg);
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
							keystatus[key] = 0;
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
	}
		
	switch (key)
	{
		case KEY_C:
			if (pGDoorSM)	DELETE_AND_NULL(pGDoorSM);
			if (pGDoorR)	DELETE_AND_NULL(pGDoorR);
			if (pGCircleW)  DELETE_AND_NULL(pGCircleW);
			if (Beep(linehighlight >= 0))
			{
				pGCircleW			= new CIRCLEWALL(linehighlight, x, y);
				pGCircleW->count	= gMisc.circlePoints;
				break;
			}
			break;
		case KEY_ENTER:
			if (!type)
			{
				BeepFail();
				break;
			}
			else if (somethingintab != searchstat)
			{
				if (somethingintab == OBJ_SECTOR)
				{
					if ((searchsector = getSector()) < 0)
					{
						scrSetMessage("Failed to get destination sector.");
						BeepFail();
						break;
					}

					searchstat = OBJ_SECTOR;
				}
				else
				{
					if (somethingintab == 255) scrSetMessage("There is nothing to paste.");
					else scrSetMessage("Can't copy from %s to %s", gSearchStatNames[ClipHigh(somethingintab, 7)], gSearchStatNames[searchstat]);
					BeepFail();
					break;
				}
			}
			
			switch (searchstat) {
				case OBJ_WALL:
					if (cpywall[kTabWall].extra > 0)
					{
						i = GetXWall(searchwall);
						xwall[i] = cpyxwall[kTabXWall];
						xwall[i].reference = searchwall;
						wall[searchwall].extra = (short)i;
					}
					else if (wall[searchwall].extra > 0)
					{
						dbDeleteXWall(wall[searchwall].extra);
						j = 1;
					}
					wall[searchwall].type = cpywall[kTabWall].type;
					i = searchwall;
					break;
				case OBJ_SPRITE:
					if (cpysprite[kTabSpr].extra > 0)
					{
						i = GetXSprite(searchwall);
						xsprite[i] = cpyxsprite[kTabXSpr];
						xsprite[i].reference = searchwall;
						sprite[searchwall].extra = (short)i;
					}
					else if (sprite[searchwall].extra > 0)
					{
						dbDeleteXSprite(sprite[searchwall].extra);
						j = 1;
					}
					sprite[searchwall].type = cpysprite[kTabSpr].type;
					i = searchwall;
					break;
				case OBJ_SECTOR:
					if (cpysector[kTabSect].extra > 0)
					{
						i = GetXSector(searchsector);
						xsector[i] = cpyxsector[kTabXSect];
						xsector[i].reference = searchsector;
						xsector[i].marker0 = xsector[i].marker1 = -1;
						sector[searchsector].extra = (short)i;
					}
					else if (sector[searchsector].extra > 0)
					{
						dbDeleteXSector(sector[searchsector].extra);
						j = 1;
					}
					sector[searchsector].type = cpysector[kTabSect].type;
					searchstat = OBJ_SECTOR;
					i = searchsector;
					break;
			}
			scrSetMessage("X-properties %s for %s[%d]", (j == 1) ? "cleared" : "pasted", gSearchStatNames[searchstat], i);
			CleanUp();
			BeepOk();
			break;
		case KEY_INSERT:
			if (highlightsectorcnt > 0)
			{
				int nIdx1, nIdx2;
				int nsects = numsectors, nwalls = numwalls;
				for (i = 0; i < highlightsectorcnt; i++)
				{
					sect = highlightsector[i];
					copysector(sect, nsects, nwalls, 1);
					nwalls += sector[sect].wallnum;
					nsects++;
				}
				
				for (i = 0; i < highlightsectorcnt; i++)
				{
					sect = highlightsector[i];
					getSectorWalls(sect, &swal, &ewal);
					for (j = swal; j <= ewal; j++)
					{
						checksectorpointer(wall[j].nextwall, wall[j].nextsector);
						checksectorpointer(j, sect);
					}
					
					highlightsector[i] = numsectors+i;
				}
				
				numsectors = nsects, numwalls = nwalls;
				updatenumsprites();
				CleanUp();
				
				for (j = 0; j < highlightsectorcnt; j++)
				{
					findSectorMarker(highlightsector[j], &nIdx1, &nIdx2);
					if (nIdx1 >= 0) ChangeSpriteSect(nIdx1, highlightsector[j]);
					if (nIdx2 >= 0) ChangeSpriteSect(nIdx2, highlightsector[j]);
				}
				
				scrSetMessage("%d sector(s) duplicated and stamped.", highlightsectorcnt);
				BeepOk();
			}
			else if (highlightcnt > 0 || type == 200)
			{
				if ((highlightcnt > 0 && type == 200 && sprInHglt(searchwall)) || type == 200)
				{
					scrSetMessage("%d sprite(s) duplicated and stamped.", ClipLow(hgltSprCallFunc(sprClone), 1));
					BeepOk();
					break;
				}
				else if (type >= 200) scrSetMessage("Must aim in objects in a highlight.");
				else if (type > 0) scrSetMessage("Must have no objects in a highlight.");
				BeepFail();
			}
			else if (linehighlight >= 0)
			{
				getclosestpointonwall(x, y, linehighlight, &x, &y);
				if (gridlock)
					doGridCorrection(&x, &y, grid);
				
				for (i = 0; i < numwalls; i++)
				{
					if (wall[i].x == x && wall[i].y == y)
						break;
				}
				
				if (Beep(i >= numwalls))
				{
					insertpoint(linehighlight, x, y);
					scrSetMessage("New point inserted at X:%d Y:%d", x, y);
				}
			}
			break;
		case KEY_F7:
			if (sectorhighlight < 0) break;
			searchstat = OBJ_FLOOR;
		// no break
		case KEY_F8:
			 i = (alt) ? 0 : 1;
			 switch (searchstat)
			 {
				 case OBJ_WALL:
				 case OBJ_SPRITE:
					CXTracker::Track(searchstat, searchwall, i);
					break;
				 case OBJ_FLOOR:
					CXTracker::Track(OBJ_SECTOR, sectorhighlight, i);
					break;
			 }
			 break;
		case KEY_T:
			if (ctrl)
			{
				gScreen2D.prefs.showTags = IncRotate(gScreen2D.prefs.showTags, kCaptionStyleMax);
				scrSetMessage("Show tags: %s", gCaptionStyleNames[gScreen2D.prefs.showTags]);
				Beep(gScreen2D.prefs.showTags);
				break;
			}
			// no break
		case KEY_H:
			if (type > 0)
			{
				if (type != 300) i = searchwall;
				else i = searchsector, searchstat = OBJ_SECTOR;
				sprintf(buffer, "%s #%d %s: ", gSearchStatNames[searchstat], i, key == KEY_T ? "lo-tag" : "hi-tag");

				switch (searchstat) {
					case OBJ_WALL:
						if (key == KEY_H)
						{
							if (ctrl) wall[i].hitag = ClipRange(GetNumberBox(buffer, wall[i].hitag, wall[i].hitag), 0, 32767);
							else if (wall[i].nextwall >= 0)
							{
								wallCstatToggle(i, kWallHitscan, !shift);
								scrSetMessage("Wall[%d] %s hitscan sensitive", i, isNot((wall[i].cstat & kWallHitscan)));
							}
						}
						else wall[i].type = ClipRange(GetNumberBox(buffer, wall[i].type, wall[i].type), 0, 32767);
						break;
					case OBJ_SPRITE:
						if (key == KEY_H)
						{
							if (ctrl) sprite[i].flags = ClipRange(GetNumberBox(buffer, sprite[i].flags, sprite[i].flags), 0, 32767);
							else
							{
								sprite[i].cstat ^= kSprHitscan;
								scrSetMessage("Sprite[%d] %s hitscan sensitive", i, isNot((sprite[i].cstat & kSprHitscan)));
							}
						}
						else sprite[i].type = ClipRange(GetNumberBox(buffer, sprite[i].type, sprite[i].type), 0, 32767);
						break;
					case OBJ_SECTOR:
						if (key == KEY_H)
						{
							if (ctrl)
								sector[i].hitag = ClipRange(GetNumberBox(buffer, sector[i].hitag, sector[i].hitag), 0, 32767);
						}
						else sector[i].type = ClipRange(GetNumberBox(buffer, sector[i].type, sector[i].type), 0, 32767);
						break;
				}
				BeepOk();
				break;
			}
			BeepFail();
			break;
		case KEY_F:
			if (linehighlight < 0 || sectorhighlight < 0) break;
			setFirstWall(sectorofwall(linehighlight),linehighlight);
			scrSetMessage("The wall %d now sector's %d first wall.", linehighlight, sectorhighlight);
			break;
		case KEY_G:
			if (ctrl) gAutoGrid.enabled^=1;
			else if (alt && grid > 0) grid = 0;
			else grid = (short)ClipLow(IncRotate(grid, (shift) ? kMaxGrids : 7), 1);
			scrSetMessage("Grid size: %d (Autogrid is %s)", grid, onOff(gAutoGrid.enabled));
			BeepOk();
			break;
		case KEY_L:
			scrSetMessage("Grid locking %s", onOff(gridlock ^= 1));
			break;
		case KEY_S:
		{
			if (alt)
			{
				if (type != 100)
					break;
				
				i = redSectorMake(searchwall);
				if (Beep(i == 0)) scrSetMessage("Sector #%d created.", sectorofwall(wall[searchwall].nextwall));
				else if (i == -1) scrSetMessage("%d is already red sector!", sectorofwall(searchwall));
				else if (i == -3) scrSetMessage("Max walls reached!");
				else if (i == -4) scrSetMessage("Max sectors reached!");
				else scrSetMessage("Can't make a sector out there.");
				break;
			}
			
			gScreen2D.GetPoint(searchx, searchy, &x, &y);
			dax = x, day = y;

			short sect2 = -1;
			updatesector(dax, day, &sect2);
			sect = sect2;
			if (sect < 0 || sect >= kMaxSectors)
			{
				BeepFail();
				break;
			}

			doGridCorrection(&dax, &day, grid);
			daz = getflorzofslope(sect,dax,day);

			if (shift)
			{
				if ((i = InsertGameObject(OBJ_FLOOR, sect, dax, day, daz, 1536)) >= 0)
				{
					scrSetMessage("%s sprite inserted.", gSpriteNames[sprite[i].type]);
					sprite[i].ang = 1536;
				}

			}
			else if ((i = InsertSprite(sect,0)) >= 0)
			{
				sprite[i].ang = 1536;
				sprite[i].x = dax, sprite[i].y = day, sprite[i].z = daz;

				if (somethingintab == OBJ_SPRITE)
				{
					sprite[i].picnum  = cpysprite[kTabSpr].picnum;
					sprite[i].shade   = cpysprite[kTabSpr].shade;
					sprite[i].pal 	  = cpysprite[kTabSpr].pal;
					sprite[i].xrepeat = cpysprite[kTabSpr].xrepeat;
					sprite[i].yrepeat = cpysprite[kTabSpr].yrepeat;
					sprite[i].xoffset = cpysprite[kTabSpr].xoffset;
					sprite[i].yoffset = cpysprite[kTabSpr].yoffset;

					if ((cpysprite[kTabSpr].cstat & kSprRelMask) == kSprVoxel)
						cpysprite[kTabSpr].cstat &= ~kSprVoxel;

					sprite[i].cstat = cpysprite[kTabSpr].cstat;

					if (sprite[tempidx].type == kMarkerPath)
					{
						sprite[i].type = kMarkerPath;
						ChangeSpriteStat(i, kStatPathMarker);

						sprite[i].flags    = sprite[tempidx].flags;
						sprite[i].clipdist = sprite[tempidx].clipdist;
						sprite[i].z        = sprite[tempidx].z; // it's worth probably to use same z

						j = GetXSprite(i); k = sprite[tempidx].extra;

						// create fully new path
						if (k <= 0 || xsprite[k].reference != tempidx)
						{
							xsprite[j].data1 = findUnusedPath();
							xsprite[j].data2 = findUnusedPath();
						}
						else
						{
							if (!markerIsNode(tempidx, 1))
							{
								xsprite[j].data1 = findUnusedPath(); // create new first id
								xsprite[k].data2 = xsprite[j].data1; // connect previous marker with current
							}
							else
							{
								xsprite[j].data1 = xsprite[k].data2; // start new branch
							}

							xsprite[j].data2    = xsprite[j].data1; // finalize current marker?
							xsprite[j].busyTime = xsprite[k].busyTime;
							xsprite[j].wave 	= xsprite[k].wave;

							short nAng = (short)(getangle(sprite[i].x - sprite[tempidx].x, sprite[i].y - sprite[tempidx].y) & kAngMask);
							sprite[i].ang = nAng;

							// change angle of the previous marker if it have less than 2 branches only
							if (!markerIsNode(tempidx, 2))
								sprite[tempidx].ang = nAng;
						}

						// make a loop?
						if (searchstat == OBJ_SPRITE && tempidx != searchwall && sprite[searchwall].type == kMarkerPath)
						{
							DeleteSprite(i);
							xsprite[k].data2 = xsprite[sprite[searchwall].extra].data1;
							if (!Confirm("Finish path drawing now?"))
							{
								cpysprite[kTabSpr]   = sprite[searchwall];
								cpyxsprite[kTabXSpr] = xsprite[searchwall];
								tempidx = searchwall;
							}
							else
							{
								somethingintab = 255; // path drawing is finished
							}
						}
						else
						{
							// update clipboard
							cpysprite[kTabSpr]   = sprite[i];
							cpyxsprite[kTabXSpr] = xsprite[j];
							tempidx = (short)i;
						}
					}

					CleanUp();
				}

				clampSprite(&sprite[i]);
				scrSetMessage("Sprite inserted.");
			}
			updatenumsprites();
			break;
		}
		case KEY_F10:
			if (highlightsectorcnt > 0 || highlightcnt > 0) break;
			else if (type == 200) {
				static int sum = 0;
				static int odata = 0;
				spritetype *pSprite = &sprite[searchwall];
				GetXSprite(searchwall);
				switch (pSprite->type) {
					case kSoundAmbient:
					case kDudeModernCustom:
						playSound(xsprite[pSprite->extra].data3);
						break;
					case kGenSound:
						playSound(xsprite[pSprite->extra].data2);
						break;
					case kSoundSector:
						odata = (searchwall != sum) ? 0 : IncRotate(odata, 4);
						playSound(getDataOf((BYTE)odata, OBJ_SPRITE, searchwall));
						break;
					case kSwitchToggle:
					case kSwitchOneWay:
						odata = (searchwall != sum) ? 0 : IncRotate(odata, 2);
						playSound(getDataOf((BYTE)odata, OBJ_SPRITE, searchwall));
						break;
					case kSwitchCombo:
					case kThingObjectGib:
					case kThingObjectExplode:
					case 425:
					case 426:
					case 427:
						playSound(xsprite[pSprite->extra].data4);
						break;
					case kMarkerWarpDest:
						if (pSprite->statnum == kStatMarker) break;
						playSound(xsprite[pSprite->extra].data4);
						break;
					case kSoundPlayer:
						playSound(xsprite[pSprite->extra].data1);
						break;
				}
				sum = searchwall;
			}
			break;
		case KEY_PADMINUS:
		case KEY_PADPLUS:
			if (type == 200) {
				spritetype *pSprite = &sprite[searchwall];
				BOOL both  = (!ctrl && !alt);
				BYTE step = (shift) ? 10 : 20;
				switch (pSprite->type) {
					case kSoundAmbient:
						if (key == KEY_PADMINUS) {
							if (both || ctrl) xsprite[pSprite->extra].data1 = ClipLow(xsprite[pSprite->extra].data1 - step, 0);
							if (both || alt)  xsprite[pSprite->extra].data2 = ClipLow(xsprite[pSprite->extra].data2 - step, 0);
						} else {
							if (both || ctrl) xsprite[pSprite->extra].data1 = ClipHigh(xsprite[pSprite->extra].data1 + step, 32767);
							if (both || alt)  xsprite[pSprite->extra].data2 = ClipHigh(xsprite[pSprite->extra].data2 + step, 32767);
						}
						AutoAdjustSprites();
						ShowSpriteData(searchwall, TRUE);
						break;
					default:
						BeepFail();
						break;
				}
			}
			break;
		case KEY_PAD1:
		case KEY_END:
			gScreen2D.prefs.ambRadius = (BYTE)IncRotate(gScreen2D.prefs.ambRadius, 5);
			if (gScreen2D.prefs.ambRadius == 4 && !gScreen2D.prefs.ambRadiusHover)
			{
				gScreen2D.prefs.ambRadiusHover = TRUE;
				gScreen2D.prefs.ambRadius = 3;
			}
			else
			{
				gScreen2D.prefs.ambRadiusHover = FALSE;
			}
			break;
	}
	
	if (key != 0)
		keyClear();
}
