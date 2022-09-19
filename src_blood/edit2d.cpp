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
#include "build.h"
#include "editor.h"
#include "pragmas.h"
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
#include "nnexts.h"
#include "aadjust.h"
#include "preview.h"
#include "xmpview.h"
#include "xmpmisc.h"
#include "xmpexplo.h"
#include "xmptools.h"
#include "xmpconf.h"
#include "edit3d.h"


short sectorhighlight;		// which sector mouse is inside in 2d mode
short sectorhighlightfill;

void getpoint2(int* x, int* y)
{
	*x = posx + divscale14(*x-halfxdim16,zoom);
	*y = posy + divscale14(*y-midydim16,zoom);
}

/* struct CIRCLEWALL
{
	signed   int line = -1;
	signed   int cenx, ceny;
	signed	 int ang1, ang2;
	signed   int angdif;
	unsigned int radius;
	unsigned int count;
	unsigned int angdir		: 1;
	unsigned int dogrid 	: 1;
	void Setup(int x, int y);
	void Draw();
	void Insert();
	
} gCircle;

void CIRCLEWALL::Setup(int x, int y)
{
	int i, j, x0, y0, x1, y1, x2, y2;
	x0 = wall[line].x;	x1 = wall[wall[line].point2].x;	x2 = x;
	y0 = wall[line].y;	y1 = wall[wall[line].point2].y;	y2 = y;
	
	adjustmark(&x2,&y2,newnumwalls);
	i = dmulscale4(x2-x1, x0-x2, y0-y2, y2-y1);
	j = dmulscale4(y0-y1, x0-x2, y0-y2, x1-x0);
	
	if (j != 0)
	{
		cenx = ((x0 + x1) + scale(y0 - y1, i, j)) >> 1;
		ceny = ((y0 + y1) + scale(x1 - x0, i, j)) >> 1;
		
		ang1   = getangle(x0 - cenx, y0 - ceny);
		ang2   = getangle(x1 - cenx, y1 - ceny);
		angdif = (ang2 - ang1) & kAngMask;
		if (mulscale4(x2 - x0, y2 - y0) < mulscale4(x1 - x0, y2 - y1))
			angdif = -((ang1 - ang2) & kAngMask);
		
		i = cenx - x0; j = ceny - y0;
		radius = ksqrt(dmulscale4(i, i, j, j))<<2;
	}
}

void CIRCLEWALL::Draw()
{
	int i, j, dx, dy;
	for (i = count; i > 0; i--)
	{
		j = (ang1 + scale(i, angdif, count+1)) & kAngMask;
		dx = cenx + mulscale14(sintable[(j+512)&2047], radius);
		dy = ceny + mulscale14(sintable[j], radius);
		
		dx = ClipRange(dx, -editorgridextent, editorgridextent);
		dy = ClipRange(dy, -editorgridextent, editorgridextent);
		
		dx = halfxdim16 + mulscale14(dx - posx, zoom);
		dy = midydim16  + mulscale14(dy - posy, zoom);
		
		Draw2dVertex(dx, dy, clr2std(kColorBrown), clr2std(kColorBlack));
	}
	
} 
*/

void FASTCALL sectorDetach(int nSector)
{
	int x1, y1, x2, y2;
	int sw, ew, sw2, ew2, i, j, k, sect;
	getSectorWalls(nSector, &sw, &ew); i = sw;

	 // first white out walls from the both sides
	while(i <= ew)
	{
		if (wall[i].nextwall >= 0)
		{
			wallDetach(wall[i].nextwall);
			wallDetach(i);
		}
		
		i++;
	}
	
	// restore red walls if sibling sectors stay highlight
	if (highlightsectorcnt > 0)
	{
		while(--i >= sw)
		{
			getWallCoords(i, &x1, &y1, &x2, &y2);
			
			j = highlightsectorcnt;
			while(j--)
			{
				sect = highlightsector[j];
				getSectorWalls(sect, &sw2, &ew2);
				for (k = sw2; k <= ew2; k++)
				{
					if (wall[wall[k].point2].x != x1 || wall[k].x != x2) continue;
					if (wall[wall[k].point2].y != y1 || wall[k].y != y2) continue;
					
					wall[i].nextsector = sect;		wall[i].nextwall = k;
					wall[k].nextsector = nSector;	wall[k].nextwall = i;
				}
			}
		}
	}
	
}

void FASTCALL sectorAttach(int nSector) {
	
	register int i, j, sw, ew;
	getSectorWalls(nSector, &sw, &ew); i = sw;
	
	while(i <= ew)
	{
		checksectorpointer(i++, nSector);
	}

	// if near sectors stay highlighted
	while (--i >= sw)
	{
		if (wall[i].nextwall < 0) continue;
		for (j = 0; j < highlightsectorcnt; j++)
		{
			if (highlightsector[j] != wall[i].nextsector) continue;
			wall[wall[i].nextwall].nextwall = wall[wall[i].nextwall].nextsector = -1;
			wall[i].nextwall = wall[i].nextsector = -1;
		}
	}
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


int redSectorCanMake(int nStartWall) {
	
	int addwalls = -1;
	if (wall[nStartWall].nextwall >= 0) return -1;
	else if (numsectors >= kMaxSectors) return -4;
	else if ((addwalls = whitelinescan(nStartWall)) < numwalls) return -2;
	else if (addwalls >= kMaxWalls) return -3;
	else return addwalls;
}


int redSectorMake(int nStartWall) {
	
	dassert(nStartWall >= 0 && nStartWall < numwalls);
	
	int i, addwalls, swal, ewal;
	if ((addwalls = redSectorCanMake(nStartWall)) <= 0)
		return addwalls;

	for (i = numwalls; i < addwalls; i++)
	{
		wall[wall[i].nextwall].nextwall = i;
		wall[wall[i].nextwall].nextsector = numsectors;
	}
		
	sectortype* pSect =& sector[numsectors];
	getSectorWalls(numsectors, &swal, &ewal);
	
	// we for sure don't need cstat inheriting for walls
	for (i = swal; i <= ewal; i++)
		wall[i].cstat = wall[wall[i].nextwall].cstat = 0;

	numwalls = addwalls;
	numsectors++;
	return 0;
}

int redSectorMerge(int nThis, int nWith) {

	int i, j, k, f, m, swal, ewal, tmp, nwalls = numwalls;
	short join[2]; join[0] = nThis, join[1] = nWith;

	for(i = 0; i < 2; i++)
	{
		getSectorWalls(join[i], &swal, &ewal);
		for(j = swal; j <= ewal; j++)
		{
			if (wall[j].cstat == 255)
				continue;
			
			tmp = i;
			if (wall[j].nextsector == join[1-tmp])
			{
				wall[j].cstat = 255;
				continue;
			}

			f = j;
			k = nwalls;
			do
			{
				memcpy(&wall[nwalls],&wall[f],sizeof(walltype));
				wall[nwalls].point2 = nwalls+1;
				nwalls++;
				wall[f].cstat = 255;

				f = wall[f].point2;
				if (wall[f].nextsector == join[1-tmp])
				{
					f = wall[wall[f].nextwall].point2;
					tmp = 1 - tmp;
				}
			}
			while ((wall[f].cstat != 255) && (wall[f].nextsector != join[1 - tmp]));
			wall[nwalls - 1].point2 = k;
		}
	}
	
	if (nwalls <= numwalls)
		return 0;

	memcpy(&sector[numsectors], &sector[join[0]], sizeof(sectortype));
	sector[numsectors].wallnum = nwalls - numwalls;
	sector[numsectors].wallptr = numwalls;

	for(i = numwalls;i < nwalls; i++)
	{
		if (wall[i].nextwall < 0) continue;
		wall[wall[i].nextwall].nextsector = numsectors;
		wall[wall[i].nextwall].nextwall = i;
	}

	for(i = 0; i < 2; i++)
	{
		getSectorWalls(join[i], &swal, &ewal);
		for(j = swal; j <= ewal; j++)
			wall[j].nextwall = wall[j].nextsector = -1;

		j = headspritesect[join[i]];
		while (j != -1)
		{
			k = nextspritesect[j];
			ChangeSpriteSect(j, numsectors);
			j = k;
		}
	}
	
	numwalls = nwalls, numsectors++;
	if (join[0] < join[1])
		join[1]--;
	
	deletesector(join[0]);
	deletesector(join[1]);
	return 0;
}

enum {
kFlagSide1			= 0x00,
kFlagSide2			= 0x01,
kFlagSprites		= 0x02,
kFlagRpoint			= 0x04,
kFlagGrid			= 0x08,
};


/* void RotatePoint2(int *x, int *y, int nAngle, int ox, int oy)
{
    //int dx = *x-ox;
    //int dy = *y-oy;
   // *x = ox+dmulscale30r(dx, Cos(nAngle), -dy, Sin(nAngle));
    //*y = oy+dmulscale30r(dx, Sin(nAngle), dy, Cos(nAngle));
	
	
  float s = sin(nAngle);
  float c = cos(nAngle);
	
  // translate point back to origin:
	int dx = *x-ox;
    int dy = *y-oy;

  // rotate point
  float xnew = dx * c - dy * s;
  float ynew = dx * s + dy * c;

  // translate point back:
  dx = (int)(xnew + ox);
  dy = (int)(ynew + oy);
  
  *x = dx;
  *y = dy;

} */



void performRotate(int* x, int* y, int nAng, int ax, int ay, BOOL rpoint = TRUE) {
	
	int dx = *x, dy = *y;
	
	if (rpoint)			RotatePoint(&dx, &dy, nAng, ax, ay);
	else if (nAng > 0)	dx = ax + ay - *y, dy = ay + *x - ax; // Ken's four side rotation
	else if (nAng < 0)	dx = ax + *y - ay, dy = ay + ax - *x; // Ken's four side rotation
	*x = dx, *y = dy;
	
} 

void loopGetWalls(int nStartWall, int* swal, int *ewal) {
	
	int i;
	*swal = *ewal = i = nStartWall;
	while(wall[i].point2 != nStartWall) 
	{
		if (wall[i].point2 < *swal) *swal = wall[i].point2;
		if (wall[i].point2 > *ewal) *ewal = wall[i].point2;
		i = wall[i].point2;
	}

}

void loopGetEdgeWalls(int nFirst, short* lw, short* rw, short* tw, short* bw) {
	
	register int i; int swal, ewal;
	
	*lw = *rw = *tw = *bw = nFirst;
	loopGetWalls(nFirst, &swal, &ewal);
	for (i = swal; i <= ewal; i++)
	{
		if (wall[i].x < wall[*lw].x) *lw = i;
		if (wall[i].x > wall[*rw].x) *rw = i;
		if (wall[i].y < wall[*tw].y) *tw = i;
		if (wall[i].y > wall[*bw].y) *bw = i;
	}

}


void avePointLoop2(int nFirst, int* ax, int* ay) {
		
	short lw, rw, bw, tw;
	loopGetEdgeWalls(nFirst, &lw, &rw, &tw, &bw);
	*ax = wall[lw].x + ((wall[rw].x - wall[lw].x) >> 1);
	*ay = wall[tw].y + ((wall[bw].y - wall[tw].y) >> 1);
}

void avePointLoop(int nFirst, int* ax, int* ay) {
	
	*ax = *ay = 0;
	int i, swal, ewal, cnt = 0;
	
	loopGetWalls(nFirst, &swal, &ewal);
	for (i = swal; i <= ewal; i++)
	{
		*ax+=wall[i].x;
		*ay+=wall[i].y;
		cnt++;
	}
	
	cnt = ClipLow(cnt, 1);
	*ax /= cnt;
	*ay /= cnt;
}

void loopRotateWalls(int nFirst, int nAng, int ax, int ay, char flags = kFlagRpoint) {
	
	int i, swal, ewal;
	loopGetWalls(nFirst, &swal, &ewal);
	for (i = swal; i <= ewal; i++)
	{
		performRotate(&wall[i].x, &wall[i].y, nAng, ax, ay, (flags & kFlagRpoint));
		if (flags & kFlagGrid)
			doGridCorrection(&wall[i].x, &wall[i].y, grid);
	}

}

void loopRotateWalls(int nFirst, int nAng, char flags = kFlagRpoint) {
	
	int ax, ay;
	avePointLoop2(nFirst, &ax, &ay);
	loopRotateWalls(nFirst, nAng, ax, ay, flags);
}

// get max grid size for XY at which it could be created
int getXYGrid(int x, int y, int min = 1, int max = 7) {
	
	int g, dx = x, dy = y;
	for (g = min; g < max; g++)
	{
		doGridCorrection(&x, &y, g);
		if (x == dx && y == dy)
			break;
	}
	
	return g;
}

void rotateSector(int nSector, int nAng, int ax, int ay, char flags = kFlagSprites) {
		
	int i, swal, ewal, nwal, g = grid;
	BOOL hglt = (flags & kFlagSide2) ? (hgltCheck(OBJ_SECTOR, nSector) >= 0) : FALSE;
	
	if (!(flags & kFlagRpoint))
		nAng = (nAng < 0) ? -512 : 512;

	if (hglt)
		sectorAttach(nSector);
	
	getSectorWalls(nSector, &swal, &ewal);
	for (i = swal; i <= ewal; i++)
	{
		nwal = wall[i].nextwall;
		performRotate(&wall[i].x, &wall[i].y, nAng, ax, ay, (flags & kFlagRpoint));		
		if (flags & kFlagGrid)
		{
			g = getXYGrid(wall[i].x, wall[i].y);
			doGridCorrection(&wall[i].x, &wall[i].y, g);
		}
		
		if ((flags & kFlagSide2) && nwal >= 0)
		{
			performRotate(&wall[nwal].x, &wall[nwal].y, nAng, ax, ay, (flags & kFlagRpoint));
			if (flags & kFlagGrid)
				doGridCorrection(&wall[nwal].x, &wall[nwal].y, g);
		}
	}
	
	if (hglt)
		sectorDetach(nSector);
	
	if (flags & kFlagSprites)
	{
		for (i = headspritesect[nSector]; i >= 0; i = nextspritesect[i])
		{
			performRotate(&sprite[i].x, &sprite[i].y, nAng, ax, ay, (flags & kFlagRpoint));
			if (flags & kFlagGrid)
				doGridCorrection(&sprite[i].x, &sprite[i].y, g);
			
			if (sprite[i].statnum != kStatMarker)
				sprite[i].ang = (sprite[i].ang + nAng) & kAngMask;
		}
	}
}

void drawHighlight(int x1, int y1, int x2, int y2, char color) {
	
	BYTE* pTile;
	int t, wh, hg, nTile = gSysTiles.hglt2d;
	char flags = kRSCorner | kRSNoMask | kRSNoClip | kRSTransluc;
	
	if (x1 > x2) swapValues(&x1, &x2);
	if (y1 > y2) swapValues(&y1, &y2);

	wh = x2-x1, hg = y2-y1;
	if (tilesizx[nTile] == wh && tilesizy[nTile] == hg)
	{
		pTile = tileLoadTile(nTile);
	}
	else
	{
		tileFreeTile(nTile);
		if ((pTile = tileAllocTile(nTile, wh, hg, 0, 0)) == NULL)
			return;
	}

	memset(pTile, color, wh*hg);
	rotatesprite(x1 << 16, y1 << 16, 0x10000, 0, nTile, 0, 0, flags, windowx1, windowy1, windowx2, windowy2);
}

void Draw2dWallMidPoint(int nWall, char color, char which) {

	register int ang, size, x0, y0, x1, y1;
	getWallCoords(nWall, &x0, &y0, &x1, &y1);
	x0 = halfxdim16 + mulscale14(x0 - posx, zoom);
	y0 = midydim16  + mulscale14(y0 - posy, zoom);
	x1 = halfxdim16 + mulscale14(x1 - posx, zoom);
	y1 = midydim16  + mulscale14(y1 - posy, zoom);
	
	int mx = (x0 + x1) >> 1; int my = (y0 + y1) >> 1;
	if (mx > 4 && mx < xdim - 5 && my > 4 && my < ydim16 - 5)
	{
		if ((size = ClipHigh(mulscale18(getWallLength(nWall), zoom), 4)) > 0)
		{
			ang	= GetWallAngle(nWall) + kAng90;
			if (which & 0x01)
			{
				x1  = mulscale30(size, Cos(ang));
				y1  = mulscale30(size, Sin(ang));
				drawline16(mx, my, mx + x1, my + y1, clr2std(color));
			}
			
			if (which & 0x02)
			{
				ang = (ang + kAng180) & kAngMask;
				x1  = mulscale30(size, Cos(ang));
				y1  = mulscale30(size, Sin(ang));
				drawline16(mx, my, mx + x1, my + y1, clr2std(color));
			}
		}
	}

}

void Draw2dFloorSpriteFull(int x, int y, char color, int idx) {

	begindrawing();
	int odrawlinepat = drawlinepat;
	int fx = mulscale6(tilesizx[sprite[idx].picnum], sprite[idx].xrepeat);
	int fy = mulscale6(tilesizy[sprite[idx].picnum], sprite[idx].yrepeat);
	int co[4][2], ii;
	int sinang = sintable[(sprite[idx].ang+512+1024)&2047];
	int cosang = sintable[(sprite[idx].ang+1024)&2047];
	int r,s;
	

	fx = mulscale10(fx,zoom) >> 1;
	fy = mulscale10(fy,zoom) >> 1;

	co[0][0] = -fx;
	co[0][1] = -fy;
	co[1][0] =  fx;
	co[1][1] = -fy;
	co[2][0] =  fx;
	co[2][1] =  fy;
	co[3][0] = -fx;
	co[3][1] =  fy;

	for (ii=0;ii<4;ii++)
	{
		r = mulscale14(cosang,co[ii][0]) - mulscale14(sinang,co[ii][1]);
		s = mulscale14(sinang,co[ii][0]) + mulscale14(cosang,co[ii][1]);
		co[ii][0] = r;
		co[ii][1] = s;
	}
	
	drawlinepat = kPatDotted;
	
	for (ii=0;ii<4;ii++)
	{
		drawline16(x + co[ii][0], y - co[ii][1], x + co[(ii+1)&3][0], y - co[(ii+1)&3][1], color);
	}
	drawlinepat = odrawlinepat;
	enddrawing();
}

void Draw2dFloorSprite(int x, int y, char color)
{
	gfxSetColor(color);
	
	gfxHLine(y - 3, x - 3, x + 3);
	gfxHLine(y + 3, x - 3, x + 3);
	
	gfxVLine(x - 3, y - 3, y + 3);
	gfxVLine(x + 3, y - 3, y + 3);
}

void DrawBuild2dFaceSprite(int x, int y, int color) {

	begindrawing();
	BYTE *templong = (BYTE*)(y * bytesperline + x + frameplace);
	drawpixel(templong-1-(bytesperline<<1), color);
	drawpixel(templong+0-(bytesperline<<1), color);
	drawpixel(templong+1-(bytesperline<<1), color);

	drawpixel(templong-1+(bytesperline<<1), color);
	drawpixel(templong+0+(bytesperline<<1), color);
	drawpixel(templong+1+(bytesperline<<1), color);

	drawpixel(templong-2-bytesperline, color);
	drawpixel(templong-2+0, color);
	drawpixel(templong-2+bytesperline, color);

	drawpixel(templong+2-bytesperline, color);
	drawpixel(templong+2+0, color);
	drawpixel(templong+2+bytesperline, color);

	drawpixel(templong+1+bytesperline, color);
	drawpixel(templong-1+bytesperline, color);
	drawpixel(templong+1-bytesperline, color);
	drawpixel(templong-1-bytesperline, color);
	enddrawing();
}

void Draw2dCross(int x, int y, int color, int size) {
	drawline16(x-size, y-size, x+size, y+size, color);
	drawline16(x-size, y+size, x+size, y-size, color);
}

BOOL testXSectorForLighting(short nXSect) {

	int sum = 0;
	XSECTOR* sc = &xsector[nXSect];
	if (obsoleteXObject(OBJ_FLOOR, nXSect))
		return FALSE;

	sum  = sc->state + sc->txID + sc->rxID + sc->command + sc->triggerOn + sc->triggerOff;
	sum += sc->decoupled + sc->triggerOnce + sc->locked + sc->interruptable + sc->dudeLockout;
	sum += sc->triggerOn + sc->busyTimeA + sc->busyWaveA + sc->reTriggerA;
	sum += sc->waitTimeA + sc->triggerOff + sc->busyTimeB + sc->busyWaveB + sc->reTriggerB;
	sum += sc->waitTimeB + sc->triggerPush + sc->triggerWallPush + sc->triggerEnter + sc->triggerExit;
	sum += sc->key + sc->Depth + sc->underwater + sc->crush + sc->data;
	return (BOOL)(sum == 0);

}

int findUnusedChannel(DIALOG_ITEM* dialog) {

	BOOL used[1024];
	register int i;
	
	memset(used, FALSE, sizeof(used));

	// search in dialog...
	if (dialog != NULL && dialog != dlgXSectorFX) {

		int nType = dialog->value;
		for (DIALOG_ITEM *control = dialog; control->type != CONTROL_END; control++) {

			// get rx and tx id fields
			if (control->tabGroup < 2) continue;
			else if (control->tabGroup == 2 || control->tabGroup == 3) used[control->value] = TRUE;
			else if (dialog != dlgXSprite) // below must be a sprite dialog
				break;

			// data fields
			if (nType == kModernRandomTX || nType == kModernSequentialTX) {
				if (control->tabGroup < kSprDialogData1) continue;
				if (chlRangeIsFine(control->value)) used[control->value] = TRUE;
				if (control->tabGroup >= kSprDialogData4)
					break;
			}

		}

	}
	
	for(i = 100; i < LENGTH(used); i++)
	{
		if (used[i]) continue;
		else if ((used[i] = (collectObjectsByChannel(i, FALSE) > 0)) != 0) continue;
		else used[i] = (collectObjectsByChannel(i, TRUE) > 0);
	}
	
	// find the first unused id
	for (i = 100; i < LENGTH(used); i++)
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

int findUnusedStack() {

	int i, j, k;
	int stacks[64];
	memset(stacks, -1, sizeof(stacks));

	// collect all the stack data
	for (i = 0, k = 0; i < numsectors; i++) {
		for (j = headspritesect[i]; j != -1; j = nextspritesect[j]) {
			if (sprite[j].extra > 0 && sprite[j].type >= kMarkerLowLink
				&& sprite[j].type <= kMarkerLowGoo && sprite[j].type != kMarkerWarpDest) {
					stacks[k++] = xsprite[sprite[j].extra].data1;
					if (k == 64) ThrowError("Out of free ROR Id!");
			}
		}
	}

	// search for free id
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) if (stacks[j] == i) break;
		if (j == 64) return i;
	}

	return -1;

}

void DialogText( int x, int y, short nFore, short nBack, char *string)
{
	int x1 = gMapedHud.content.x1+(x << 3);
	int y1 = gMapedHud.content.y1+(y << 3);
	int x2 = x1 + (strlen(string) << 3);
	int	y2 = y1 + 8;
	
	gfxSetColor(nBack);
	gfxFillBox(x1, y1, x2, y2);
	printextShadowL(x1, y1, nFore, string);
}

void controlSetReadyLabel(DIALOG_ITEM *control, char* str)
{
	control->readyLabel = (char*)realloc(control->readyLabel, strlen(str)+1);
	sprintf(control->readyLabel, str);
}

void PaintDialogItem(DIALOG_ITEM *dialog, DIALOG_ITEM *control, int focus)
{
	short foreColor = clr2std(kColorLightCyan);
	short backColor = clr2std(kColorDarkGray);

	if (focus)
	{
		foreColor = clr2std(kColorYellow);
		backColor = clr2std(kColorBlack);

		// assuming that we are in edit mode
		if (gHints)
			gMapedHud.DrawEditDialogHints(dialog, control);
	}
	else if (control->type == HEADER)
	{
		foreColor = clr2std(kColorWhite);
		backColor = clr2std(kColorGreen);
	}
	else if (!control->enabled)
	{
		foreColor = clr2std(20);
	}
	else if (control->flags > 0 && gMisc.showTypes != control->flags)
	{
		foreColor = clr2std(kColorCyan);
	}
	
	switch ( control->type )
	{
		case HEADER:
		case DIALOG:
			DialogText(control->x, control->y, foreColor, backColor, control->formatLabel);
			break;
		case LABEL:
			if (control->names)
			{
				sprintf(buffer, control->formatLabel, control->names[control->value]);
				controlSetReadyLabel(control, buffer);
				DialogText(control->x, control->y, foreColor, backColor, buffer);
			}
			else
			{
				DialogText(control->x, control->y, foreColor, backColor, control->formatLabel);
			}
			break;
		case NUMBER:
			if (dialog == dlgXSprite && control->tabGroup >= kSprDialogData1 && control->tabGroup <= kSprDialogData4)
			{
				DIALOG_ITEM *sprType = dialog;

				char* label = NULL, *defLabel = control->formatLabel;
				int i = 0, longest = 5, j = 0, k = 0, maxRng = LENGTH(gSpriteDataNames)-1;
				BYTE di = (BYTE)(((control->tabGroup - kSprDialogData4) + 4) - 1);
				if (!irngok(sprType->value, 0, maxRng) || (label = gSpriteDataNames[sprType->value][di]) == NULL)
					label = defLabel;

				if (label[0] == kPrefixSpecial) // special case
				{
					DIALOG_ITEM* ctrl;
					switch (sprType->value) {
						case 500: // data names depends on CMD type
							for (ctrl = dialog; ctrl->tabGroup != 5; ctrl++);
							for (j = 0; j < LENGTH(pCtrlDataNames); j++, label = defLabel)
							{
								if (pCtrlDataNames[j].var1 != ctrl->value) continue;
								else if (!pCtrlDataNames[j].dataOvr[di]) continue;
								else label = pCtrlDataNames[j].dataOvr[di];
								break;
							}

							// get longest strlen from special array
							if (j == LENGTH(pCtrlDataNames)) break;
							for (k = 0; k < 4; k++)
							{
								if (!pCtrlDataNames[j].dataOvr[k]) continue;
								if ((i = strlen(pCtrlDataNames[j].dataOvr[k])) > longest)
									longest = i;
							}
							break;
					}
				}

				if (!focus)
					foreColor = clr2std((label == defLabel) ? kColorCyan : kColorLightCyan);

				if (irngok(sprType->value, 0, maxRng))
				{
					for (i = 0; i < 4; i++)
					{
						if (gSpriteDataNames[sprType->value][i] == NULL) continue;
						else if ((j = strlen(gSpriteDataNames[sprType->value][i])) > longest)
							longest = j;
					}
				}

				i = sprintf(buffer, "%-0.10s", label);
				if ((j = strlen(label)) < longest)
				{
					k = ClipRange(longest - j, 0, 5);
					memset(&buffer[i], 32, k);
					i+=k;
				}
				
				// fix for unsigned data4
				if (control->tabGroup == kSprDialogData4 && control->value == 65535)
					control->value = -1; // just display -1, but set 65535
				
				sprintf(&buffer[i], ": %-6d", control->value);
			}
			else if (dialog == dlgXWall && control->tabGroup == kWallDialogData)
			{
				// get wall type
				DIALOG_ITEM *wallType = dialog;
				int maxRng = LENGTH(gWallDataNames)-1;
				char* label = NULL;
				
				if (!irngok(wallType->value, 0, maxRng) || (label = gWallDataNames[wallType->value][0]) == NULL)
				{
					label = control->formatLabel;
					if (!focus)
						foreColor = clr2std(kColorCyan);
				}

				sprintf(buffer, "%-0.11s: %-5d", label, control->value);
				
			}
			else if (dialog == dlgXSector && control->tabGroup == kSectDialogData)
			{
				// get sector type
				DIALOG_ITEM *sectType = dialog;
				int maxRng = LENGTH(gSectorDataNames)-1;
				char* label = NULL;
				
				if (!irngok(sectType->value, 0, maxRng) || (label = gSectorDataNames[sectType->value][0]) == NULL)
				{
					label = control->formatLabel;
					if (!focus)
						foreColor = clr2std(kColorCyan);
				}

				sprintf(buffer, "%-0.11s: %-5d", label, control->value);
			}
			else
			{
				sprintf(buffer, control->formatLabel, control->value);
			}
			controlSetReadyLabel(control, buffer);
			DialogText(control->x, control->y, foreColor, backColor, control->readyLabel);
			break;
		case LIST:
			dassert(control->names != NULL);
			sprintf(buffer, control->formatLabel, control->value,
				(control->names[control->value] != NULL) ? control->names[control->value] : "<unnamed>");
			
			controlSetReadyLabel(control, buffer);
			DialogText(control->x, control->y, foreColor, backColor, buffer);
			break;
		case CHECKBOX:
			sprintf(buffer, "%c %s", control->value ? 3 : 2, control->formatLabel);
			controlSetReadyLabel(control, buffer);
			DialogText(control->x, control->y, foreColor, backColor, buffer);
			break;
		case RADIOBUTTON:
			sprintf(buffer, "%c %s", control->value ? 5 : 4, control->formatLabel);
			controlSetReadyLabel(control, buffer);
			DialogText(control->x, control->y, foreColor, backColor, buffer);
			break;


	}
}


void PaintDialog(DIALOG_ITEM *dialog)
{
	gMapedHud.ClearContent();
	for (DIALOG_ITEM *control = dialog; control->type != CONTROL_END; control++)
		PaintDialogItem(dialog, control, false);
}

DIALOG_ITEM *FindGroup(DIALOG_ITEM *dialog, int group, int maxGroup, int dir = 0)
{
	DIALOG_ITEM *control;
	if (dir == -1 && --group < 1) group = maxGroup;
	else if (dir == 1 && ++group > maxGroup)
		group = 1;
	
	for (control = dialog; control->type != CONTROL_END; control++)
	{
		if (control->tabGroup != group) continue;
		else if (control->enabled) break;
		else if (dir == -1) group--;
		else group++;
		
		if (group < 1) group = maxGroup;
		else if (group > maxGroup)
			group = 1;
		
		control = dialog;
	}


	dassert(control->type != CONTROL_END);
	return control;
}

void SetControlValue(DIALOG_ITEM *dialog, int group, int value )
{
	DIALOG_ITEM *control;
	for (control = dialog; control->type != CONTROL_END; control++)
	{
		if ( control->tabGroup == group )
			break;
	}
	
	dassert(control->type != CONTROL_END);
	control->value = value;
}

int GetControlValue(DIALOG_ITEM *dialog, int group )
{
	DIALOG_ITEM *control;
	for (control = dialog; control->type != CONTROL_END; control++)
	{
		if ( control->tabGroup == group )
			break;
	}
	
	dassert(control->type != CONTROL_END);
	return control->value;
}

int drawHint(int x, int y, int ox, int oy, char fr, short bg, char* text)
{
	return 0;
}

int EditDialog(DIALOG_ITEM *dialog)
{
	char tmp[16], *label;
	BYTE key, ctrl, shift, alt;
	DIALOG_ITEM *control, *item; RESHANDLE hFile;
	int x1, x2, x3, x4, y1, y2, y3, y4, wh, hg, group = 1, maxGroup = 0, value, i;
	static int recurLev = 0;
	BOOL upd = TRUE;
	MOUSE mouse;
	
	keyClear();
		
	gMapedHud.GetWindowCoords(&gMapedHud.main, &x1, &y1, &x2, &y2);
	if (recurLev++ < 1)
	{
		drawHighlight(0, 0, x2, y1+(gMapedHud.tw>>1), clr2std(kColorBlack));
		gMapedHud.DrawLogo();

		if (gHints == NULL) // load hints for edit dialog
		{
			if ((hFile = gGuiRes.Lookup(kIniEditDialogHints, "INI")) != NULL)
				gHints = new IniFile(gGuiRes.Load(hFile));
		}
	}
	
	gMapedHud.GetWindowCoords(&gMapedHud.content, &x1, &y1, &x2, &y2);
	
	mouse.ChangeCursor(kBitmapMouseCursor);
	wh = mouse.cursor.width; hg = mouse.cursor.height >> 1;
	mouse.RangeSet(x1+wh, y1+hg, x2-wh, y2-ClipLow(hg, 9)); // in case mouse cursor is just too large
	mouse.VelocitySet(40, 35, false);
	mouse.wheelDelay = 14;

	// find max group
	for (control = dialog; control->type != CONTROL_END; control++)
	{
		if (control->tabGroup > maxGroup)
			maxGroup = control->tabGroup;
	}
	
	// focus on first group
	control = FindGroup(dialog, group, maxGroup, 0);
	mouse.X = x1 + (control->x << 3);
	mouse.Y = y1 + (control->y << 3);
	group = control->tabGroup;
	
	while ( 1 )
	{
		if (upd)
		{
			PaintDialog(dialog);
			PaintDialogItem(dialog, control, true);
			
			if (control->fieldHelpProc)
			{
				label = (control->readyLabel) ? control->readyLabel : control->formatLabel;
				
				x3 = control->x << 3, y3 = control->y << 3;
				x4 = x3 + (ClipLow(strlen(label), 1) << 3), y4 = y3 + 8;
				
				gfxSetColor(clr2std(kColorWhite));
				
				int l = (x4-x3)>>1;
				int mx = x3 + l;
				
				gfxVLine(mx, y1-(y3-y1), y3);
				
				gfxSetColor(clr2std(kColorBlue));
				gfxFillBox(mx-l-4, midydim16-4, mx+l+4, midydim16+8+4);
				printextShadowL(x3, midydim16, clr2std(kColorWhite), label);
				
			}
			
			
			mouse.Draw();
			showframe();
		}
		
		updateClocks();
		
		mouse.Read();
		keyGetHelper(&key, &ctrl, &shift, &alt);
		handleevents();
		
		upd = (key || mouse.buttons || mouse.dX2 || mouse.dY2);
		
		if (!upd) continue;
		else if ((mouse.dY2 || mouse.dX2) && !keystatus[KEY_Q])
		{
			// find item to focus with mouse
			for (item = dialog; item->type != CONTROL_END; item++)
			{
				if (item->tabGroup <= 0 || !item->enabled || !item->formatLabel) continue;
				else if (item->readyLabel) label = item->readyLabel;
				else label = item->formatLabel;
				
				x3 = item->x << 3, y3 = item->y << 3;
				x4 = x3 + (ClipLow(strlen(label), 1) << 3), y4 = y3 + 8;
				if (irngok(mouse.X, x1+x3, x1+x4) && irngok(mouse.Y, y1+y3, y1+y4))
				{
					group = item->tabGroup;
					control = item;
					break;
				}
			}
		}
		
		if (key)
		{
			if (control->fieldHelpProc)
				key = control->fieldHelpProc(dialog, control, key);
		}
		else if (mouse.buttons) // convert mouse buttons in keyboard scans
		{
			if (mouse.wheel && keystatus[KEY_Q])
			{
				key = (mouse.wheel < 0) ? KEY_LEFT : KEY_RIGHT;
				mouse.wheel = 0;
			}

			if ((mouse.press & 4) && control->fieldHelpProc)
			{
				key = control->fieldHelpProc(dialog, control, KEY_F10);
				continue;
			}
		
			if (mouse.press & 2)
			{
				control->value = 0;
			}
			
			switch (control->type) {
				case CHECKBOX:
					if (mouse.press & 1) key = KEY_SPACE;
					break;
				case DIALOG:
					if (!(mouse.press & 1)) break;
					control->fieldHelpProc(dialog, control, KEY_ENTER);
					continue;
				case LIST:
				case NUMBER:
					if (mouse.press & 4)
					{
						if (control->type == LIST) // force a dialog box
							key = KEY_1;
					}
					else if (mouse.wheel == -1)
					{
						if (ctrl || alt) key = KEY_PAGEUP;
						else if (shift) key = KEY_PADPLUS;
						else if (control->fieldHelpProc != AuditSound) key = KEY_UP;
						else control->fieldHelpProc(dialog, control, KEY_UP); // force skip non-existing sounds
					}
					else if (mouse.wheel == 1)
					{
						if (ctrl || alt) key = KEY_PAGEDN;
						else if (shift) key = KEY_PADMINUS;
						else if (control->fieldHelpProc != AuditSound) key = KEY_DOWN;
						else control->fieldHelpProc(dialog, control, KEY_DOWN); // force skip non-existing sounds
					}
					break;
			}
		}

		switch (key) {
			case KEY_ENTER:
			case KEY_PADENTER:
				BeepOk();
				// no break
			case KEY_ESC:
				keystatus[key] = 0;
				if (gHints && dialog != dlgXSectorFX)
				{
					delete(gHints);
					gHints = NULL;
				}
				sndKillAllSounds();
				recurLev--;
				return (key == KEY_ESC) ? 0 : 1;
			case KEY_TAB:
			case KEY_RIGHT:
				if (!shift)
				{
					PaintDialogItem(dialog, control, false);
					control = FindGroup(dialog, group, maxGroup, 1);
					group = control->tabGroup;
					continue;
				}
				// no break
			case KEY_LEFT:
				PaintDialogItem(dialog, control, false);
				control = FindGroup(dialog, group, maxGroup, -1);
				group = control->tabGroup;
				continue;
		}

		switch (control->type) {
			case NUMBER:
				switch (key) {
					case KEY_DELETE:
						control->value = 0;
						break;
					case KEY_PADPLUS:
					case KEY_UP:
						control->value++;
						break;
					case KEY_PADMINUS:
					case KEY_DOWN:
						control->value--;
						break;
					case KEY_PAGEUP:
						if (ctrl) control->value = control->maxValue;
						else control->value = IncBy(control->value, 10);
						break;
					case KEY_PAGEDN:
						if (ctrl) control->value = control->minValue;
						else control->value = DecBy(control->value, 10);
						break;
					case KEY_MINUS:
						if (control->minValue >= 0) break;
						else if (!control->value) control->value = -1;
						sprintf(tmp, "%d", (control->value > 0) ? -control->value : control->value);
						control->value = atoi(tmp);
						break;
					case KEY_PLUS:
						if (control->maxValue < 0) break;
						sprintf(tmp, "%d", abs(control->value));
						control->value = atoi(tmp);
						break;
					case KEY_BACKSPACE:
						if ((i = sprintf(tmp, "%d", control->value)) > 1) tmp[i-1] = 0;
						else tmp[0] = '\0';
						control->value = atoi(tmp);
						break;
					default:
						if (irngok(key, KEY_1, KEY_0) || irngok(key, KEY_PAD7, KEY_PAD0))
						{
							i = 0;
							i += sprintf(&tmp[i], "%d", control->value);
							i += sprintf(&tmp[i], "%c", keyAscii[key]);
							control->value = atoi(tmp);
						}
						break;
				}
				control->value = ClipRange(control->value, control->minValue, control->maxValue);
				break;
			case CHECKBOX:
				switch (key) {
					case KEY_SPACE:
					case KEY_BACKSPACE:
					case KEY_DELETE:
						control->value = !control->value;
						break;
				}
				break;
			case LIST:
				value = control->value;
				switch (key) {
					case KEY_DELETE:
					case KEY_SPACE:
						control->value = 0;
						break;
					case KEY_PAGEUP:
					case KEY_PADPLUS:
					case KEY_UP:
						if (!ctrl)
						{
							do
							{
								value = IncBy(value, (key == KEY_PAGEUP) ? 5 : 1);
								if (value > control->maxValue)
									value = control->minValue;
							}
							while (value <= control->maxValue && control->names[value] == NULL);
						}
						else
						{
							value = control->maxValue;
							while (value >= control->minValue && control->names[value] == NULL) value--;
						}
						break;
					case KEY_PAGEDN:
					case KEY_PADMINUS:
					case KEY_DOWN:
						if (!ctrl)
						{
							do
							{
								value = DecBy(value, (key == KEY_PAGEDN) ? 5 : 1);
								if (value < control->minValue)
									value = control->maxValue;
							}
							while (value >= control->minValue && control->names[value] == NULL);
						}
						else
						{
							value = control->minValue;
							while (value <= control->maxValue && control->names[value] == NULL) value++;
						}
						break;
					default:
						if (irngok(key, KEY_1, KEY_0) || irngok(key, KEY_PAD7, KEY_PAD0))
						{
							sprintf(buffer, "Enter value (%d - %d)", control->minValue, control->maxValue);
							value = GetNumberBox(buffer, value, value);
							if (!irngok(value, control->minValue, control->maxValue))
								Alert("Value must be in a range of %d and %d.", control->minValue, control->maxValue);
							else if (!control->names || control->names[value] == NULL)
								Alert("Value %d is reserved or not exist.", value);
						}
						break;
				}
				if (value < control->minValue || value > control->maxValue || !control->names[value]) break;
				control->value = value;
				break;
		}
		

	}
}

void XWallToDialog( int nWall )
{
	int nXWall = wall[nWall].extra;
	dassert(nXWall > 0 && nXWall < kMaxXWalls);
	XWALL *pXWall = &xwall[nXWall];

	SetControlValue(dlgXWall, 1, wall[nWall].type);
	SetControlValue(dlgXWall, 2, pXWall->rxID);
	SetControlValue(dlgXWall, 3, pXWall->txID);
	SetControlValue(dlgXWall, 4, pXWall->state);
	SetControlValue(dlgXWall, 5, pXWall->command);

	SetControlValue(dlgXWall, 6, pXWall->triggerOn);
	SetControlValue(dlgXWall, 7, pXWall->triggerOff);
	SetControlValue(dlgXWall, 8, pXWall->busyTime);
	SetControlValue(dlgXWall, 9, pXWall->waitTime);
	SetControlValue(dlgXWall, 10, pXWall->restState);

	SetControlValue(dlgXWall, 11, pXWall->triggerPush);
	SetControlValue(dlgXWall, 12, pXWall->triggerVector);
	SetControlValue(dlgXWall, 13, pXWall->triggerTouch);

	SetControlValue(dlgXWall, 14, pXWall->key);
	SetControlValue(dlgXWall, 15, pXWall->panXVel);
	SetControlValue(dlgXWall, 16, pXWall->panYVel);
	SetControlValue(dlgXWall, 17, pXWall->panAlways);

	SetControlValue(dlgXWall, 18, pXWall->decoupled);
	SetControlValue(dlgXWall, 19, pXWall->triggerOnce);
	SetControlValue(dlgXWall, 20, pXWall->locked);
	SetControlValue(dlgXWall, 21, pXWall->interruptable);
	SetControlValue(dlgXWall, 22, pXWall->dudeLockout);

	SetControlValue(dlgXWall, kWallDialogData, pXWall->data);

}

void DialogToXWall( int nWall )
{
	int nXWall = wall[nWall].extra;
	dassert(nXWall > 0 && nXWall < kMaxXWalls);
	XWALL *pXWall = &xwall[nXWall];

	wall[nWall].type			= (short)GetControlValue(dlgXWall, 1);
	pXWall->rxID				= GetControlValue(dlgXWall, 2);
	pXWall->txID				= GetControlValue(dlgXWall, 3);

	pXWall->state				= GetControlValue(dlgXWall, 4);
	pXWall->command				= GetControlValue(dlgXWall, 5);

	pXWall->triggerOn			= GetControlValue(dlgXWall, 6);
	pXWall->triggerOff			= GetControlValue(dlgXWall, 7);
	pXWall->busyTime			= GetControlValue(dlgXWall, 8);
	pXWall->waitTime			= GetControlValue(dlgXWall, 9);
	pXWall->restState			= GetControlValue(dlgXWall, 10);

	pXWall->triggerPush			= GetControlValue(dlgXWall, 11);
	pXWall->triggerVector		= GetControlValue(dlgXWall, 12);
	pXWall->triggerTouch		= GetControlValue(dlgXWall, 13);

	pXWall->key					= GetControlValue(dlgXWall, 14);
	pXWall->panXVel				= GetControlValue(dlgXWall, 15);
	pXWall->panYVel				= GetControlValue(dlgXWall, 16);
	pXWall->panAlways			= GetControlValue(dlgXWall, 17);

	pXWall->decoupled			= GetControlValue(dlgXWall, 18);
	pXWall->triggerOnce			= GetControlValue(dlgXWall, 19);
	pXWall->locked				= GetControlValue(dlgXWall, 20);
	pXWall->interruptable		= GetControlValue(dlgXWall, 21);
	pXWall->dudeLockout			= GetControlValue(dlgXWall, 22);

	pXWall->data				= GetControlValue(dlgXWall, kWallDialogData);
}

void WallToDialog(int nWall)
{
	dassert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall =&wall[nWall];

	SetControlValue(dlgWall, 1, pWall->x);
	SetControlValue(dlgWall, 2, pWall->y);
	SetControlValue(dlgWall, 3, pWall->point2);
	SetControlValue(dlgWall, 4, sectorofwall(nWall));
	SetControlValue(dlgWall, 5, pWall->nextwall);
	SetControlValue(dlgWall, 6, pWall->nextsector);
	SetControlValue(dlgWall, 7, pWall->hitag);
	SetControlValue(dlgWall, 8, pWall->lotag);
	SetControlValue(dlgWall, 9, pWall->extra);
	
	SetControlValue(dlgWall, 10, pWall->picnum);
	SetControlValue(dlgWall, 11, pWall->overpicnum);
	SetControlValue(dlgWall, 12, pWall->shade);
	SetControlValue(dlgWall, 13, pWall->pal);
	SetControlValue(dlgWall, 14, pWall->xrepeat);
	SetControlValue(dlgWall, 15, pWall->yrepeat);
	SetControlValue(dlgWall, 16, pWall->xpanning);
	SetControlValue(dlgWall, 17, pWall->ypanning);
	
/* 	SetControlValue(dlgWall, 18, pWall->cstat & kWallBlock);
	SetControlValue(dlgWall, 19, pWall->cstat & kWallSwap);
	SetControlValue(dlgWall, 20, pWall->cstat & kWallOrgBottom);
	SetControlValue(dlgWall, 21, pWall->cstat & kWallFlipX);
	SetControlValue(dlgWall, 22, pWall->cstat & kWallFlipY);
	SetControlValue(dlgWall, 23, pWall->cstat & kWallMasked);
	SetControlValue(dlgWall, 24, pWall->cstat & kWallOneWay);
	SetControlValue(dlgWall, 25, pWall->cstat & kWallHitscan);
	SetControlValue(dlgWall, 26, pWall->cstat & kWallTransluc2);
	SetControlValue(dlgWall, 27, pWall->cstat & kWallTranslucR);
	SetControlValue(dlgWall, 28, pWall->cstat & kWallMoveForward);
	SetControlValue(dlgWall, 29, pWall->cstat & kWallMoveReverse); */
		
	//SetControlValue(dlgWall, 30, GetWallAngle(nWall) & kAngMask);
	//SetControlValue(dlgWall, 31, getWallLength(nWall));
	//SetControlValue(dlgWall, 32, (pSect->floorz - pSect->ceilingz)>>8);


}

void DialogToWall(int nWall)
{
	dassert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall =&wall[nWall];

	pWall->x 			= GetControlValue(dlgWall, 1);
	pWall->y 			= GetControlValue(dlgWall, 2);
	pWall->point2 		= GetControlValue(dlgWall, 3);
	// 4
	pWall->nextwall 	= GetControlValue(dlgWall, 5);
	pWall->nextwall 	= GetControlValue(dlgWall, 6);
	pWall->hitag 		= GetControlValue(dlgWall, 7);
	pWall->type 		= GetControlValue(dlgWall, 8);
	pWall->extra 		= GetControlValue(dlgWall, 9);
	
	pWall->picnum 		= GetControlValue(dlgWall, 10);
	pWall->overpicnum 	= GetControlValue(dlgWall, 11);
	pWall->shade 		= GetControlValue(dlgWall, 12);
	pWall->pal 			= GetControlValue(dlgWall, 13);
	pWall->xrepeat 		= GetControlValue(dlgWall, 14);
	pWall->yrepeat 		= GetControlValue(dlgWall, 15);
	pWall->xpanning 	= GetControlValue(dlgWall, 16);
	pWall->ypanning 	= GetControlValue(dlgWall, 17);
	
/* 	setCstat(GetControlValue(dlgWall, 18), &pWall->cstat, kWallBlock);
	setCstat(GetControlValue(dlgWall, 19), &pWall->cstat, kWallSwap);
	setCstat(GetControlValue(dlgWall, 20), &pWall->cstat, kWallOrgBottom);
	setCstat(GetControlValue(dlgWall, 21), &pWall->cstat, kWallFlipX);
	setCstat(GetControlValue(dlgWall, 22), &pWall->cstat, kWallFlipY);
	setCstat(GetControlValue(dlgWall, 23), &pWall->cstat, kWallMasked);
	setCstat(GetControlValue(dlgWall, 24), &pWall->cstat, kWallOneWay);
	setCstat(GetControlValue(dlgWall, 25), &pWall->cstat, kWallHitscan);
	setCstat(GetControlValue(dlgWall, 26), &pWall->cstat, kWallTransluc);
	setCstat(GetControlValue(dlgWall, 27), &pWall->cstat, kWallTransluc2);
	setCstat(GetControlValue(dlgWall, 28), &pWall->cstat, kWallMoveForward);
	setCstat(GetControlValue(dlgWall, 29), &pWall->cstat, kWallMoveReverse); */
}

void SectorToDialog(int nSector)
{
	dassert(nSector >= 0 && nSector < kMaxSectors);
	
	int i, cnt = 0;
	sectortype* pSect =& sector[nSector];
	
	SetControlValue(dlgSector, 1, pSect->wallnum);
	SetControlValue(dlgSector, 2, pSect->wallptr);	
	for (i = headspritesect[nSector]; i >= 0; i = nextspritesect[i])
	{
		if (sprite[i].statnum >= kMaxStatus) continue;
		cnt++;
	}
	
	SetControlValue(dlgSector, 3, cnt);
	SetControlValue(dlgSector, 4, headspritesect[nSector]);
	SetControlValue(dlgSector, 5, pSect->hitag);
	SetControlValue(dlgSector, 6, pSect->lotag);
	SetControlValue(dlgSector, 7, pSect->visibility);
	SetControlValue(dlgSector, 8, pSect->extra);
	
	SetControlValue(dlgSector, 9, pSect->ceilingpicnum);
	SetControlValue(dlgSector, 10, pSect->ceilingshade);
	SetControlValue(dlgSector, 11, pSect->ceilingpal);
	SetControlValue(dlgSector, 12, pSect->ceilingxpanning);
	SetControlValue(dlgSector, 13, pSect->ceilingypanning);
	SetControlValue(dlgSector, 14, pSect->ceilingheinum);
	SetControlValue(dlgSector, 15, pSect->ceilingz);
	
	SetControlValue(dlgSector, 16, pSect->floorpicnum);
	SetControlValue(dlgSector, 17, pSect->floorshade);
	SetControlValue(dlgSector, 18, pSect->floorpal);
	SetControlValue(dlgSector, 19, pSect->floorxpanning);
	SetControlValue(dlgSector, 20, pSect->floorypanning);
	SetControlValue(dlgSector, 21, pSect->floorheinum);
	SetControlValue(dlgSector, 22, pSect->floorz);
	
/* 	SetControlValue(dlgSector, 23, pSect->ceilingstat  & kSectSloped);
	SetControlValue(dlgSector, 24, pSect->floorstat    & kSectSloped);
	
	SetControlValue(dlgSector, 25, pSect->ceilingstat  & kSectParallax);
	SetControlValue(dlgSector, 26, pSect->floorstat    & kSectParallax);
	
	SetControlValue(dlgSector, 27, pSect->ceilingstat  & kSectExpand);
	SetControlValue(dlgSector, 28, pSect->floorstat    & kSectExpand);
	
	SetControlValue(dlgSector, 29, pSect->ceilingstat  & kSectRelAlign);
	SetControlValue(dlgSector, 30, pSect->floorstat    & kSectRelAlign);
	
	SetControlValue(dlgSector, 31, pSect->ceilingstat  & kSectFlipX);
	SetControlValue(dlgSector, 32, pSect->floorstat    & kSectFlipX);
	
	SetControlValue(dlgSector, 33, pSect->ceilingstat  & kSectFlipY);
	SetControlValue(dlgSector, 34, pSect->floorstat    & kSectFlipY);
	
	BOOL floorshade = (!(pSect->ceilingstat & kSectParallax) || (pSect->ceilingstat & kSectShadeFloor));
	SetControlValue(dlgSector, 35, !floorshade);
	SetControlValue(dlgSector, 36,  floorshade);
	
	SetControlValue(dlgSector, 37, pSect->ceilingstat & kSectMasked);
	SetControlValue(dlgSector, 38, pSect->floorstat   & kSectMasked); */
}

void DialogToSector(int nSector)
{
	dassert(nSector >= 0 && nSector < kMaxSectors);
	
	int i, cnt = 0;
	sectortype* pSect =& sector[nSector];
	
	//pSect->wallnum				= GetControlValue(dlgSector, 1);
	//pSect->wallptr				= GetControlValue(dlgSector, 2);
	// 3
	//headspritesect[nSector]		= GetControlValue(dlgSector, 4);
	pSect->hitag					= GetControlValue(dlgSector, 5);
	pSect->lotag 					= GetControlValue(dlgSector, 6);
	pSect->visibility				= GetControlValue(dlgSector, 7);
	
	pSect->ceilingpicnum			= ClipHigh(GetControlValue(dlgSector, 9), gMaxTiles-1);
	pSect->ceilingshade				= GetControlValue(dlgSector, 10);
	pSect->ceilingpal				= GetControlValue(dlgSector, 11);
	pSect->ceilingxpanning			= GetControlValue(dlgSector, 12);
	pSect->ceilingypanning			= GetControlValue(dlgSector, 13);
	
	SetCeilingSlope(nSector,		GetControlValue(dlgSector, 14));
	SetCeilingZ(nSector, 			GetControlValue(dlgSector, 15));
	
	pSect->floorpicnum				= ClipHigh(GetControlValue(dlgSector, 16), gMaxTiles-1);
	pSect->floorshade				= GetControlValue(dlgSector, 17);
	pSect->floorpal					= GetControlValue(dlgSector, 18);
	pSect->floorxpanning			= GetControlValue(dlgSector, 19);
	pSect->floorypanning			= GetControlValue(dlgSector, 20);
	
	SetFloorSlope(nSector,			GetControlValue(dlgSector, 21));
	SetFloorZ(nSector,				GetControlValue(dlgSector, 22));

	
	/* i = kSectSloped;
	setCstat(GetControlValue(dlgSector, 23), &pSect->ceilingstat, i);
	setCstat(GetControlValue(dlgSector, 24), &pSect->floorstat, i);
	
	if (!(pSect->ceilingstat & i)) SetCeilingSlope(nSector, 0);
	if (!(pSect->floorstat & i)) SetFloorSlope(nSector, 0);
	
	i = kSectParallax;
	setCstat(GetControlValue(dlgSector, 25), &pSect->ceilingstat, i);
	setCstat(GetControlValue(dlgSector, 26), &pSect->floorstat, i);
	
	i = kSectExpand;
	setCstat(GetControlValue(dlgSector, 27), &pSect->ceilingstat, i);
	setCstat(GetControlValue(dlgSector, 28), &pSect->floorstat, i);
	
	i = kSectRelAlign;
	setCstat(GetControlValue(dlgSector, 29), &pSect->ceilingstat, i);
	setCstat(GetControlValue(dlgSector, 30), &pSect->floorstat, i);
	
	i = kSectFlipX;
	setCstat(GetControlValue(dlgSector, 31), &pSect->ceilingstat, i);
	setCstat(GetControlValue(dlgSector, 32), &pSect->floorstat, i);
	
	i = kSectFlipY;
	setCstat(GetControlValue(dlgSector, 33), &pSect->ceilingstat, i);
	setCstat(GetControlValue(dlgSector, 34), &pSect->floorstat, i);
	
	if (pSect->ceilingstat & kSectParallax)
	{
		// 35 for ceiling
		setCstat(GetControlValue(dlgSector, 36), &pSect->floorstat, kSectShadeFloor);
	}
	else
	{
		pSect->floorstat &= ~kSectShadeFloor;
	}
	
	i = kSectMasked;
	setCstat(GetControlValue(dlgSector, 37), &pSect->ceilingstat, i);
	setCstat(GetControlValue(dlgSector, 38), &pSect->floorstat, i); */
}


void SpriteToDialog(int nSprite)
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	spritetype* pSpr =&sprite[nSprite];
	
	SetControlValue(dlgSprite, 1, pSpr->x);
	SetControlValue(dlgSprite, 2, pSpr->y);
	SetControlValue(dlgSprite, 3, pSpr->z);
	SetControlValue(dlgSprite, 4, pSpr->sectnum);
	SetControlValue(dlgSprite, 5, pSpr->statnum);
	SetControlValue(dlgSprite, 6, pSpr->hitag);
	SetControlValue(dlgSprite, 7, pSpr->lotag);
	SetControlValue(dlgSprite, 8, pSpr->clipdist);
	SetControlValue(dlgSprite, 9, pSpr->extra);
		
	SetControlValue(dlgSprite, 10, pSpr->picnum);
	SetControlValue(dlgSprite, 11, pSpr->shade);
	SetControlValue(dlgSprite, 12, pSpr->pal);
	SetControlValue(dlgSprite, 13, pSpr->xrepeat);
	SetControlValue(dlgSprite, 14, pSpr->yrepeat);
	SetControlValue(dlgSprite, 15, pSpr->xoffset);
	SetControlValue(dlgSprite, 16, pSpr->yoffset);
	SetControlValue(dlgSprite, 17, spriteGetSlope(nSprite));
	
	SetControlValue(dlgSprite, 18, pSpr->ang);
	SetControlValue(dlgSprite, 19, pSpr->xvel);
	SetControlValue(dlgSprite, 20, pSpr->yvel);
	SetControlValue(dlgSprite, 21, pSpr->zvel);
	SetControlValue(dlgSprite, 22, pSpr->owner);
	
/* 	SetControlValue(dlgSprite, 23, (pSpr->cstat & kSprRelMask) == kSprFloor);
	SetControlValue(dlgSprite, 24, (pSpr->cstat & kSprRelMask) == kSprWall);
	SetControlValue(dlgSprite, 25, (pSpr->cstat & kSprRelMask) == kSprVoxel);
	SetControlValue(dlgSprite, 26, pSpr->cstat & kSprBlock);
	SetControlValue(dlgSprite, 27, pSpr->cstat & kSprHitscan);
	SetControlValue(dlgSprite, 28, pSpr->cstat & kSprOneSided);
	SetControlValue(dlgSprite, 29, pSpr->cstat & kSprInvisible);
	SetControlValue(dlgSprite, 30, pSpr->cstat & kSprOrigin);
	SetControlValue(dlgSprite, 31, pSpr->cstat & kSprFlipX);
	SetControlValue(dlgSprite, 32, pSpr->cstat & kSprFlipY);
	SetControlValue(dlgSprite, 33, pSpr->cstat & kSprTransluc2);
	SetControlValue(dlgSprite, 34, pSpr->cstat & kSprTranslucR);
	SetControlValue(dlgSprite, 35, pSpr->cstat & kSprMoveForward);
	SetControlValue(dlgSprite, 36, pSpr->cstat & kSprMoveReverse); */
}


void XSpriteToDialog( int nSprite )
{
	int nXSprite = sprite[nSprite].extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	XSPRITE *pXSprite = &xsprite[nXSprite];

	SetControlValue(dlgXSprite, 1, sprite[nSprite].type);
	SetControlValue(dlgXSprite, 2, pXSprite->rxID);
	SetControlValue(dlgXSprite, 3, pXSprite->txID);
	SetControlValue(dlgXSprite, 4, pXSprite->state);
	SetControlValue(dlgXSprite, 5, pXSprite->command);

	SetControlValue(dlgXSprite, 6, pXSprite->triggerOn);
	SetControlValue(dlgXSprite, 7, pXSprite->triggerOff);
	SetControlValue(dlgXSprite, 8, pXSprite->busyTime);
	SetControlValue(dlgXSprite, 9, pXSprite->waitTime);
	SetControlValue(dlgXSprite, 10, pXSprite->restState);

	SetControlValue(dlgXSprite, 11, pXSprite->triggerPush);
	SetControlValue(dlgXSprite, 12, pXSprite->triggerVector);
	SetControlValue(dlgXSprite, 13, pXSprite->triggerImpact);
	SetControlValue(dlgXSprite, 14, pXSprite->triggerPickup);
	SetControlValue(dlgXSprite, 15, pXSprite->triggerTouch);
	SetControlValue(dlgXSprite, 16, pXSprite->triggerProximity);
	SetControlValue(dlgXSprite, 17, pXSprite->triggerSight);

	SetControlValue(dlgXSprite, 18, (pXSprite->unused3 & 0x0001) ? 1 : 0);
	SetControlValue(dlgXSprite, 19, (pXSprite->unused3 & 0x0002) ? 1 : 0);

	//SetControlValue(dlgXSprite, 18, pXSprite->triggerReserved1);
	//SetControlValue(dlgXSprite, 19, pXSprite->triggerReserved2);

	SetControlValue(dlgXSprite, 20, !((pXSprite->lSkill >> 0) & 1));
	SetControlValue(dlgXSprite, 21, !((pXSprite->lSkill >> 1) & 1));
	SetControlValue(dlgXSprite, 22, !((pXSprite->lSkill >> 2) & 1));
	SetControlValue(dlgXSprite, 23, !((pXSprite->lSkill >> 3) & 1));
	SetControlValue(dlgXSprite, 24, !((pXSprite->lSkill >> 4) & 1));
	SetControlValue(dlgXSprite, 25, !pXSprite->lS);
	SetControlValue(dlgXSprite, 26, !pXSprite->lB);
	SetControlValue(dlgXSprite, 27, !pXSprite->lC);
	SetControlValue(dlgXSprite, 28, !pXSprite->lT);

	SetControlValue(dlgXSprite, 29, pXSprite->decoupled);
	SetControlValue(dlgXSprite, 30, pXSprite->triggerOnce);
	SetControlValue(dlgXSprite, 31, pXSprite->locked);
	SetControlValue(dlgXSprite, 32, pXSprite->interruptable);
	SetControlValue(dlgXSprite, 33, pXSprite->dudeLockout);

	SetControlValue(dlgXSprite, kSprDialogData1, pXSprite->data1);
	SetControlValue(dlgXSprite, kSprDialogData2, pXSprite->data2);
	SetControlValue(dlgXSprite, kSprDialogData3, pXSprite->data3);
	SetControlValue(dlgXSprite, kSprDialogData4, pXSprite->data4);


	SetControlValue(dlgXSprite, 38, pXSprite->respawn);
	SetControlValue(dlgXSprite, 39, pXSprite->dudeFlag4);
	SetControlValue(dlgXSprite, 40, pXSprite->dudeDeaf);
	SetControlValue(dlgXSprite, 41, pXSprite->dudeGuard);
	SetControlValue(dlgXSprite, 42, pXSprite->dudeAmbush);
	SetControlValue(dlgXSprite, 43, pXSprite->unused1); // used to set stealth flag for dude

	SetControlValue(dlgXSprite, 44, pXSprite->key);
	SetControlValue(dlgXSprite, 45, pXSprite->wave);
	SetControlValue(dlgXSprite, 46, sprite[nSprite].flags);
	SetControlValue(dlgXSprite, 47, pXSprite->lockMsg);
	SetControlValue(dlgXSprite, 48, pXSprite->dropItem);
}

void DialogToXSprite( int nSprite )
{
	int val, nXSprite = sprite[nSprite].extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	XSPRITE *pXSprite = &xsprite[nXSprite];

	sprite[nSprite].type		= (short)GetControlValue(dlgXSprite, 1);;
	pXSprite->rxID				= GetControlValue(dlgXSprite, 2);
	pXSprite->txID				= GetControlValue(dlgXSprite, 3);
	pXSprite->state				= GetControlValue(dlgXSprite, 4);
	pXSprite->command			= GetControlValue(dlgXSprite, 5);

	pXSprite->triggerOn			= GetControlValue(dlgXSprite, 6);
	pXSprite->triggerOff		= GetControlValue(dlgXSprite, 7);
	pXSprite->busyTime			= GetControlValue(dlgXSprite, 8);
	pXSprite->waitTime			= GetControlValue(dlgXSprite, 9);
	pXSprite->restState			= GetControlValue(dlgXSprite, 10);

	pXSprite->triggerPush		= GetControlValue(dlgXSprite, 11);
	pXSprite->triggerVector		= GetControlValue(dlgXSprite, 12);
	pXSprite->triggerImpact		= GetControlValue(dlgXSprite, 13);
	pXSprite->triggerPickup		= GetControlValue(dlgXSprite, 14);
 	pXSprite->triggerTouch		= GetControlValue(dlgXSprite, 15);
 	pXSprite->triggerProximity	= GetControlValue(dlgXSprite, 16);
	pXSprite->triggerSight		= GetControlValue(dlgXSprite, 17);
	if (GetControlValue(dlgXSprite, 18)) pXSprite->unused3 |= 0x0001;
	else pXSprite->unused3 &= ~0x0001;

 	if (GetControlValue(dlgXSprite, 19)) pXSprite->unused3 |= 0x0002;
	else pXSprite->unused3 &= ~0x0002;
	//pXSprite->triggerReserved1	= GetControlValue(dlgXSprite, 18);
	//pXSprite->triggerReserved2	= GetControlValue(dlgXSprite, 19);


 	pXSprite->lSkill		=
			(!GetControlValue(dlgXSprite, 20) << 0) |
			(!GetControlValue(dlgXSprite, 21) << 1) |
			(!GetControlValue(dlgXSprite, 22) << 2) |
			(!GetControlValue(dlgXSprite, 23) << 3) |
			(!GetControlValue(dlgXSprite, 24) << 4);
 	pXSprite->lS				= !GetControlValue(dlgXSprite, 25);
 	pXSprite->lB				= !GetControlValue(dlgXSprite, 26);
 	pXSprite->lC				= !GetControlValue(dlgXSprite, 27);
 	pXSprite->lT				= !GetControlValue(dlgXSprite, 28);

	pXSprite->decoupled			= GetControlValue(dlgXSprite, 29);
	pXSprite->triggerOnce		= GetControlValue(dlgXSprite, 30);
	pXSprite->locked			= GetControlValue(dlgXSprite, 31);
	pXSprite->interruptable		= GetControlValue(dlgXSprite, 32);
	pXSprite->dudeLockout		= GetControlValue(dlgXSprite, 33);

	pXSprite->data1				= GetControlValue(dlgXSprite, kSprDialogData1);
	pXSprite->data2				= GetControlValue(dlgXSprite, kSprDialogData2);
	pXSprite->data3				= GetControlValue(dlgXSprite, kSprDialogData3);
	
	val							= GetControlValue(dlgXSprite, kSprDialogData4);
	pXSprite->data4				= (val == -1) ? 65535 : val;

	pXSprite->respawn			= GetControlValue(dlgXSprite, 38);

	pXSprite->dudeFlag4			= GetControlValue(dlgXSprite, 39);
	pXSprite->dudeDeaf			= GetControlValue(dlgXSprite, 40);
	pXSprite->dudeGuard			= GetControlValue(dlgXSprite, 41);
	pXSprite->dudeAmbush		= GetControlValue(dlgXSprite, 42);
	pXSprite->unused1			= GetControlValue(dlgXSprite, 43);

	pXSprite->key				= GetControlValue(dlgXSprite, 44);
	pXSprite->wave				= GetControlValue(dlgXSprite, 45);
	sprite[nSprite].flags       = (short)GetControlValue(dlgXSprite, 46);
	pXSprite->lockMsg			= GetControlValue(dlgXSprite, 47);
	pXSprite->dropItem			= GetControlValue(dlgXSprite, 48);

}

void DialogToSprite(int nSprite)
{
	int i;
	dassert(nSprite > 0 && nSprite < kMaxSprites);
	spritetype *pSpr = &sprite[nSprite];
	
	pSpr->x 			= GetControlValue(dlgSprite, 1);
	pSpr->y 			= GetControlValue(dlgSprite, 2);
	pSpr->z 			= GetControlValue(dlgSprite, 3);
	pSpr->sectnum 		= GetControlValue(dlgSprite, 4);
	pSpr->statnum 		= GetControlValue(dlgSprite, 5);
	pSpr->hitag 		= GetControlValue(dlgSprite, 6);
	pSpr->type 			= GetControlValue(dlgSprite, 7);
	pSpr->clipdist 		= GetControlValue(dlgSprite, 8);
	pSpr->extra 		= GetControlValue(dlgSprite, 9);
	
	pSpr->picnum 		= GetControlValue(dlgSprite, 10);
	pSpr->shade 		= GetControlValue(dlgSprite, 11);
	pSpr->pal 			= GetControlValue(dlgSprite, 12);
	pSpr->xrepeat 		= GetControlValue(dlgSprite, 13);
	pSpr->yrepeat 		= GetControlValue(dlgSprite, 14);
	pSpr->xoffset 		= GetControlValue(dlgSprite, 15);
	pSpr->yoffset 		= GetControlValue(dlgSprite, 16);
	
	i = GetControlValue(dlgSprite, 17);
	if (i && (pSpr->cstat & kSprRelMask) != kSprFloor)
		pSpr->cstat |= kSprFloor;

	spriteSetSlope(nSprite, i);
	
	pSpr->ang 			= GetControlValue(dlgSprite, 18);
	pSpr->xvel 			= GetControlValue(dlgSprite, 19);
	pSpr->yvel 			= GetControlValue(dlgSprite, 20);
	pSpr->zvel 			= GetControlValue(dlgSprite, 21);
	pSpr->owner 		= GetControlValue(dlgSprite, 22);
	
	// 23
	// 24
	// 25
	
/* 	setCstat(GetControlValue(dlgSprite, 26), &pSpr->cstat, kSprBlock);
	setCstat(GetControlValue(dlgSprite, 27), &pSpr->cstat, kSprHitscan);
	setCstat(GetControlValue(dlgSprite, 28), &pSpr->cstat, kSprOneSided);
	setCstat(GetControlValue(dlgSprite, 29), &pSpr->cstat, kSprInvisible);
	setCstat(GetControlValue(dlgSprite, 30), &pSpr->cstat, kSprOrigin);
	setCstat(GetControlValue(dlgSprite, 31), &pSpr->cstat, kSprFlipX);
	setCstat(GetControlValue(dlgSprite, 32), &pSpr->cstat, kSprFlipY);
	setCstat(GetControlValue(dlgSprite, 33), &pSpr->cstat, kSprTransluc1);
	setCstat(GetControlValue(dlgSprite, 34), &pSpr->cstat, kSprTranslucR);
	setCstat(GetControlValue(dlgSprite, 35), &pSpr->cstat, kSprMoveForward);
	setCstat(GetControlValue(dlgSprite, 36), &pSpr->cstat, kSprMoveReverse); */
}

void XSectorToDialog(int nSector) {
	int nXSector = sector[nSector].extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	SetControlValue(dlgXSector, 1, sector[nSector].type);

	SetControlValue(dlgXSector, 2, pXSector->rxID);
	SetControlValue(dlgXSector, 3, pXSector->txID);
	SetControlValue(dlgXSector, 4, pXSector->state);
	SetControlValue(dlgXSector, 5, pXSector->command);

	SetControlValue(dlgXSector, 6, pXSector->decoupled);
	SetControlValue(dlgXSector, 7, pXSector->triggerOnce);
	SetControlValue(dlgXSector, 8, pXSector->locked);
	SetControlValue(dlgXSector, 9, pXSector->interruptable);
	SetControlValue(dlgXSector, 10, pXSector->dudeLockout);

	SetControlValue(dlgXSector, 11, pXSector->triggerOn);
	SetControlValue(dlgXSector, 12, pXSector->busyTimeA);
	SetControlValue(dlgXSector, 13, pXSector->busyWaveA);
	SetControlValue(dlgXSector, 14, pXSector->reTriggerA);
	SetControlValue(dlgXSector, 15, pXSector->waitTimeA);

	SetControlValue(dlgXSector, 16, pXSector->triggerOff);
	SetControlValue(dlgXSector, 17, pXSector->busyTimeB);
	SetControlValue(dlgXSector, 18, pXSector->busyWaveB);
	SetControlValue(dlgXSector, 19, pXSector->reTriggerB);
	SetControlValue(dlgXSector, 20, pXSector->waitTimeB);

	SetControlValue(dlgXSector, 21, pXSector->triggerPush);
	SetControlValue(dlgXSector, 22, pXSector->triggerWallPush);
	SetControlValue(dlgXSector, 23, pXSector->triggerEnter);
	SetControlValue(dlgXSector, 24, pXSector->triggerExit);

	short i = 0; // get values of first found setor sfx sprite
	for(i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
	{
		if (sprite[i].type == kSoundSector && sprite[i].extra > 0)
		{
			for (BYTE j = 0; j < 4; j++)
				SetControlValue(dlgXSector, 25 + j, getDataOf(j, OBJ_SPRITE, i));
			break;
		}
	}

	// nothing found? set all zeros
	if (i == -1)
	{
		for (i = 0; i < 4; i++)
			SetControlValue(dlgXSector, 25 + i, 0);
	}

	SetControlValue(dlgXSector, 29, pXSector->key);
	SetControlValue(dlgXSector, 30, pXSector->Depth);
	SetControlValue(dlgXSector, 31, pXSector->underwater);
	SetControlValue(dlgXSector, 32, pXSector->crush);

	SetControlValue(dlgXSector, kSectDialogData, pXSector->data);

	SetControlValue(dlgXSectorFX, 1, pXSector->shadeWave);
	SetControlValue(dlgXSectorFX, 2, pXSector->amplitude);
	SetControlValue(dlgXSectorFX, 3, pXSector->shadeFreq);
	SetControlValue(dlgXSectorFX, 4, pXSector->shadePhase);
	SetControlValue(dlgXSectorFX, 5, pXSector->shadeFloor);
	SetControlValue(dlgXSectorFX, 6, pXSector->shadeCeiling);
	SetControlValue(dlgXSectorFX, 7, pXSector->shadeWalls);
	SetControlValue(dlgXSectorFX, 8, pXSector->shadeAlways);

	SetControlValue(dlgXSectorFX, 9, pXSector->coloredLights);
	SetControlValue(dlgXSectorFX, 10, pXSector->ceilpal2);
	SetControlValue(dlgXSectorFX, 11, pXSector->floorpal2);

	SetControlValue(dlgXSectorFX, 12, pXSector->panVel);
	SetControlValue(dlgXSectorFX, 13, pXSector->panAngle);
	SetControlValue(dlgXSectorFX, 14, pXSector->panFloor);
	SetControlValue(dlgXSectorFX, 15, pXSector->panCeiling);
	SetControlValue(dlgXSectorFX, 16, pXSector->panAlways);
	SetControlValue(dlgXSectorFX, 17, pXSector->drag);

	SetControlValue(dlgXSectorFX, 18, pXSector->windVel);
	SetControlValue(dlgXSectorFX, 19, pXSector->windAng);
	SetControlValue(dlgXSectorFX, 20, pXSector->windAlways);

	SetControlValue(dlgXSectorFX, 21, pXSector->bobZRange);
	SetControlValue(dlgXSectorFX, 22, pXSector->bobTheta);
	SetControlValue(dlgXSectorFX, 23, pXSector->bobSpeed);
	SetControlValue(dlgXSectorFX, 24, pXSector->bobAlways);
	SetControlValue(dlgXSectorFX, 25, pXSector->bobFloor);
	SetControlValue(dlgXSectorFX, 26, pXSector->bobCeiling);
	SetControlValue(dlgXSectorFX, 27, pXSector->bobRotate);
	SetControlValue(dlgXSectorFX, 28, pXSector->damageType);

}


/***********************************************************************
 * DialogToXSector()
 *
 **********************************************************************/
void DialogToXSector( int nSector )
{
	int nXSector = sector[nSector].extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	sector[nSector].type 		= (short)GetControlValue(dlgXSector, 1);
	pXSector->rxID 				= GetControlValue(dlgXSector, 2);
	pXSector->txID 				= GetControlValue(dlgXSector, 3);
	pXSector->state 			= GetControlValue(dlgXSector, 4);
	pXSector->command 			= GetControlValue(dlgXSector, 5);

	pXSector->decoupled			= GetControlValue(dlgXSector, 6);
	pXSector->triggerOnce		= GetControlValue(dlgXSector, 7);
	pXSector->locked			= GetControlValue(dlgXSector, 8);
	pXSector->interruptable		= GetControlValue(dlgXSector, 9);
	pXSector->dudeLockout		= GetControlValue(dlgXSector, 10);

	pXSector->triggerOn  		= GetControlValue(dlgXSector, 11);
	pXSector->busyTimeA			= GetControlValue(dlgXSector, 12);
	pXSector->busyWaveA			= GetControlValue(dlgXSector, 13);
	pXSector->reTriggerA		= GetControlValue(dlgXSector, 14);
	pXSector->waitTimeA			= GetControlValue(dlgXSector, 15);

	pXSector->triggerOff 		= GetControlValue(dlgXSector, 16);
	pXSector->busyTimeB			= GetControlValue(dlgXSector, 17);
	pXSector->busyWaveB			= GetControlValue(dlgXSector, 18);
	pXSector->reTriggerB		= GetControlValue(dlgXSector, 19);
	pXSector->waitTimeB			= GetControlValue(dlgXSector, 20);

	pXSector->triggerPush		= GetControlValue(dlgXSector, 21);
	pXSector->triggerWallPush	= GetControlValue(dlgXSector, 22);

	pXSector->triggerEnter		= GetControlValue(dlgXSector, 23);
	pXSector->triggerExit		= GetControlValue(dlgXSector, 24);

	int i = 0;
	short cstat = 0;
	int x = kHiddenSpriteLoc, y = kHiddenSpriteLoc, z = sector[nSector].floorz;

	// delete all the sector sfx sprites in this sector
	for (i = headspritesect[nSector]; i != -1; i = nextspritesect[i]) {
		if (sprite[i].statnum >= kStatFree || sprite[i].type != kSoundSector)
			continue;

		// save info of first found sprite
		if (x == kHiddenSpriteLoc) {

			x = sprite[i].x;
			y = sprite[i].y;
			z = sprite[i].z;
			cstat = sprite[i].cstat;
		}

		DeleteSprite(i);
		i = headspritesect[nSector];
	}

	// create new sector sfx sprite if values are not zero
	for (i = 0; i < 4; i++) {
		if (GetControlValue(dlgXSector, 25 + i) <= 0)
			continue;

		// set new coords if there was no old sprite found
		if (x == kHiddenSpriteLoc)
			avePointSector(nSector, &x, &y);
			//getpoint(searchx, searchy, &x, &y);

		int nSprite = InsertSprite(nSector, 0);
		dassert(nSprite >= 0 && nSprite < kMaxSprites);

		sprite[nSprite].x = x;
		sprite[nSprite].y = y;
		sprite[nSprite].z = z;

		ChangeSpriteSect(nSprite, nSector);


		spritetype* pSprite = &sprite[nSprite];
		int nXSprite = GetXSprite(nSprite);
		dassert(nXSprite >= 0 && nXSprite < kMaxXSprites && nXSprite == pSprite->extra);
		pSprite->type = kSoundSector;
		adjSpriteByType(pSprite);

		for (i = 0; i < 4; i++)
			setDataValueOfObject(OBJ_SPRITE, pSprite->index, i + 1, GetControlValue(dlgXSector, 25 + i));

		if (cstat != 0) pSprite->cstat = cstat;
		else if (sector[nSector].type > kSectorTeleport && sector[nSector].type <= kSectorRotate)
			pSprite->cstat |= kSprMoveForward; // if sector is movable, set kinetic movement stat

		break;
	}

	pXSector->key				= GetControlValue(dlgXSector, 29);
	pXSector->Depth				= GetControlValue(dlgXSector, 30);
	pXSector->underwater		= GetControlValue(dlgXSector, 31);
	pXSector->crush				= GetControlValue(dlgXSector, 32);

	pXSector->data				= GetControlValue(dlgXSector, kSectDialogData);

	pXSector->shadeWave			= GetControlValue(dlgXSectorFX, 1);
	pXSector->amplitude     	= GetControlValue(dlgXSectorFX, 2);
	pXSector->shadeFreq        	= GetControlValue(dlgXSectorFX, 3);
	pXSector->shadePhase		= GetControlValue(dlgXSectorFX, 4);
	pXSector->shadeFloor    	= GetControlValue(dlgXSectorFX, 5);
	pXSector->shadeCeiling  	= GetControlValue(dlgXSectorFX, 6);
	pXSector->shadeWalls    	= GetControlValue(dlgXSectorFX, 7);
	pXSector->shadeAlways   	= GetControlValue(dlgXSectorFX, 8);

	pXSector->coloredLights		= GetControlValue(dlgXSectorFX, 9);
	pXSector->ceilpal2			= GetControlValue(dlgXSectorFX, 10);
	pXSector->floorpal2			= GetControlValue(dlgXSectorFX, 11);

	pXSector->panVel			= GetControlValue(dlgXSectorFX, 12);
	pXSector->panAngle			= GetControlValue(dlgXSectorFX, 13);
	pXSector->panFloor			= GetControlValue(dlgXSectorFX, 14);
	pXSector->panCeiling		= GetControlValue(dlgXSectorFX, 15);
	pXSector->panAlways			= GetControlValue(dlgXSectorFX, 16);
	pXSector->drag				= GetControlValue(dlgXSectorFX, 17);

	pXSector->windVel			= GetControlValue(dlgXSectorFX, 18);
	pXSector->windAng			= GetControlValue(dlgXSectorFX, 19);
	pXSector->windAlways		= GetControlValue(dlgXSectorFX, 20);

	pXSector->bobZRange			= GetControlValue(dlgXSectorFX, 21);
	pXSector->bobTheta			= GetControlValue(dlgXSectorFX, 22);
	pXSector->bobSpeed			= GetControlValue(dlgXSectorFX, 23);
	pXSector->bobAlways			= GetControlValue(dlgXSectorFX, 24);
	pXSector->bobFloor			= GetControlValue(dlgXSectorFX, 25);
	pXSector->bobCeiling		= GetControlValue(dlgXSectorFX, 26);
	pXSector->bobRotate			= GetControlValue(dlgXSectorFX, 27);
	pXSector->damageType		= GetControlValue(dlgXSectorFX, 28);
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


void ShowSectorData(int nSector, BOOL xFirst, BOOL dialog) {
	dassert(nSector >= 0 && nSector < kMaxSectors);
	
	sectortype* pSect =& sector[nSector];
	
	int i, nXSector;
	i = sprintf(buffer2, "Sector #%d: Area = %d", nSector, AreaOfSector(&sector[nSector]));
	if (pSect->alignto)
		i += sprintf(&buffer2[i], ", Auto-align to wall = #%d", pSect->wallptr + pSect->alignto);
	
	if (pSect->extra > 0 && xFirst)
	{
		nXSector = pSect->extra;
		if (dialog)
		{
			XSectorToDialog(nSector);
			PaintDialog(!testXSectorForLighting(nXSector) ? dlgXSector : dlgXSectorFX);
		}
		i += sprintf(&buffer2[i], ", Extra = %d, XRef = %d", nXSector, xsector[nXSector].reference);
	}
	else if (dialog)
	{
		SectorToDialog(nSector);
		PaintDialog(dlgSector);
	}
	
	gMapedHud.SetComment();
	if (pSect->extra > 0)
	{
		if ((i = gCommentMgr.IsBind(OBJ_SECTOR, pSect->extra)) >= 0)
			gMapedHud.SetComment(&gCommentMgr.comments[i]);
	}
	
	if (searchstat == OBJ_FLOOR)
		gMapedHud.SetTile(pSect->floorpicnum, pSect->floorpal, pSect->floorshade);
	else
		gMapedHud.SetTile(pSect->ceilingpicnum, pSect->ceilingpal, pSect->ceilingshade);
	
	gMapedHud.SetMsg(buffer2);
}

void ShowWallData(int nWall, BOOL xFirst, BOOL dialog) {

	int i = 0, j;
	int sect = sectorofwall(nWall);
	dassert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall =& wall[nWall];
	
	if (dialog)
	{
		if (pWall->extra > 0 && xFirst)
		{
			XWallToDialog(nWall);
			PaintDialog(dlgXWall);
		}
		else
		{
			WallToDialog(nWall);
			PaintDialog(dlgWall);
		}
	}
		
	if (!gMapedHud.wide) 				 i+=sprintf(&buffer[i], "Wall");
	else if (pWall->cstat & kWallMasked) i+=sprintf(&buffer[i], "Masked wall");
	else if (pWall->nextwall < 0) 		 i+=sprintf(&buffer[i], "Solid wall");
	else 								 i+=sprintf(&buffer[i], "Wall");
	
	j = getWallLength(nWall);
	i+=sprintf(&buffer[i], " #%d: Length = %d", nWall, j);
	if (grid)
	{
		j /= (2048 >> grid);
		i+=sprintf(&buffer[i], " (%dG)", j);
	}
	
	i+=sprintf(&buffer[i], ", Angle = %d", GetWallAngle(nWall) & kAngMask);
	
	gMapedHud.SetComment();
	if (pWall->extra > 0)
	{
		if ((j = gCommentMgr.IsBind(OBJ_WALL, pWall->extra)) >= 0)
			gMapedHud.SetComment(&gCommentMgr.comments[j]);
		
		if (gMapedHud.wide)
			i+=sprintf(&buffer[i], ", Extra = %d, XRef = %d", pWall->extra, xwall[pWall->extra].reference);
	}
	
	if (sector[sect].wallptr == nWall) i+=sprintf(&buffer[i], " [FIRST]");
	if (sector[sect].alignto && nWall-sector[sect].wallptr == sector[sect].alignto)
		i+=sprintf(&buffer[i], " [AUTO-ALIGN]");

	
	gMapedHud.SetTile((searchstat == OBJ_MASKED) ? pWall->overpicnum : pWall->picnum, pWall->pal, pWall->shade);
	gMapedHud.SetMsg(buffer);

}

void ShowSpriteData(int nSprite, BOOL xFirst, BOOL dialog) {

	int i, nShade;
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	spritetype* pSpr =& sprite[nSprite];
	
	i = sprintf(buffer2, "Sprite #%d: Statnum = %d", nSprite, pSpr->statnum);
	if (pSpr->extra > 0 && xFirst)
	{
		if (dialog)
		{
			XSpriteToDialog(nSprite);
			PaintDialog(dlgXSprite);
		}
		
		i += sprintf(&buffer2[i], ", Extra = %d, XRef = %d", pSpr->extra, xsprite[pSpr->extra].reference);
	}
	else if (dialog)
	{
		SpriteToDialog(nSprite);
		PaintDialog(dlgSprite);
	}
	
	if ((i = gCommentMgr.IsBind(OBJ_SPRITE, nSprite)) < 0)
		gMapedHud.SetComment();
	else
		gMapedHud.SetComment(&gCommentMgr.comments[i]);
	
	gMapedHud.SetTile(pSpr->picnum, pSpr->pal, pSpr->shade);
	gMapedHud.SetMsg(buffer2);
}

void EditSectorData(int nSector, BOOL xFirst) {

	dassert(nSector >= 0 && nSector < kMaxSectors);
	ShowSectorData(nSector, xFirst);
	if (xFirst)
	{
		GetXSector(nSector);
		XSectorToDialog(nSector);
		if (EditDialog(dlgXSector))
			DialogToXSector(nSector);
	}
	else
	{
		SectorToDialog(nSector);
		if (EditDialog(dlgSector))
			DialogToSector(nSector);
	}

	CleanUp();
	vel = svel = angvel = 0;
	ShowSectorData(nSector, TRUE);

}

void EditSectorLighting(int nSector) {

	dassert(nSector >= 0 && nSector < kMaxSectors);
	ShowSectorData(nSector, TRUE);
	GetXSector(nSector);

	XSectorToDialog(nSector);
	if (EditDialog(dlgXSectorFX))
		DialogToXSector(nSector);

	CleanUp();
	vel = svel = angvel = 0;
	ShowSectorData(nSector, TRUE);
}

void EditWallData(int nWall, BOOL xFirst) {

	dassert(nWall >= 0 && nWall < kMaxWalls);
	ShowWallData(nWall, xFirst);
	if (xFirst)
	{
		GetXWall(nWall);
		XWallToDialog(nWall);
		if (EditDialog(dlgXWall))
			DialogToXWall(nWall);
	}
	else
	{
		WallToDialog(nWall);
		if (EditDialog(dlgWall))
			DialogToWall(nWall);
	}
	
	CleanUp();
	vel = svel = angvel = 0;
	ShowWallData(nWall, TRUE);

}

void EditSpriteData(int nSprite, BOOL xFirst) {

	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	ShowSpriteData(nSprite, xFirst);
	if (xFirst)
	{
		GetXSprite(nSprite);
		XSpriteToDialog(nSprite);
		if (EditDialog(dlgXSprite))
			DialogToXSprite(nSprite);
	}
	else
	{
		SpriteToDialog(nSprite);
		if (EditDialog(dlgSprite))
			DialogToSprite(nSprite);
	}
	
	CleanUp();
	AutoAdjustSprites();
	vel = svel = angvel = 0;
	ShowSpriteData(nSprite, xFirst);

}

void Draw2dWall(int x0, int y0, int x1, int y1, char color, char thick, int pat)
{
	register int odrawlinepat = drawlinepat;
	drawlinepat = pat;

	begindrawing();
	drawline16(x0, y0, x1, y1, color);
	if (thick)
	{
		int dx = x1 - x0;
		int dy = y1 - y0;
		RotateVector(&dx, &dy, kAng45 / 2);
		switch (GetOctant(dx, dy))
		{
			case 0:
			case 4:
				drawline16(x0, y0-1, x1, y1-1, color);
				drawline16(x0, y0+1, x1, y1+1, color);
				break;

			case 1:
			case 5:
				if (klabs(dx) < klabs(dy) )
				{
					drawline16(x0-1, y0+1, x1-1, y1+1, color);
					drawline16(x0-1, y0, x1, y1+1, color);
					drawline16(x0+1, y0, x1+1, y1, color);
				}
				else
				{
					drawline16(x0-1, y0+1, x1-1, y1+1, color);
					drawline16(x0, y0+1, x1, y1+1, color);
					drawline16(x0, y0+1, x1, y1+1, color);
				}
				break;

			case 2:
			case 6:
				drawline16(x0-1, y0, x1-1, y1, color);
				drawline16(x0+1, y0, x1+1, y1, color);
				break;

			case 3:
			case 7:
				if (klabs(dx) < klabs(dy) )
				{
					drawline16(x0-1, y0-1, x1-1, y1-1, color);
					drawline16(x0-1, y0, x1-1, y1, color);
					drawline16(x0+1, y0, x1+1, y1, color);
				}
				else
				{
					drawline16(x0-1, y0-1, x1-1, y1-1, color);
					drawline16(x0, y0-1, x1, y1-1, color);
					drawline16(x0, y0+1, x1, y1+1, color);
				}
				break;
		}
	}

	enddrawing();
	drawlinepat = odrawlinepat;
}

void drawsq(int x1, int y1, int x2, int y2, char color, char thick = 0, int pat = kPatNormal) {
	
	Draw2dWall(x1, y1, x2, y1, color, thick, pat);
	Draw2dWall(x2, y1, x2, y2, color, thick, pat);
	Draw2dWall(x1, y1, x1, y2, color, thick, pat);
	Draw2dWall(x1, y2, x2, y2, color, thick, pat);
}

void Draw2dVertex(int x, int y, int color, int color2, int size)
{
	if (color2 >= 0)
	{
		gfxSetColor(color2);
		gfxFillBox(x - size, y - size, x + size, y + size);
	}
	
	drawsq(x - size, y - size, x + size, y + size, color);
}


void Draw2dFaceSprite( int x, int y, int color)
{
	begindrawing();
	BYTE *templong = (BYTE*)(y * bytesperline + x + frameplace);

	drawpixel(templong - 1 - (bytesperline*3), color);
	drawpixel(templong + 0 - (bytesperline*3), color);
	drawpixel(templong + 1 - (bytesperline*3), color);

	drawpixel(templong - 2 - (bytesperline<<1), color);
	drawpixel(templong + 2 - (bytesperline<<1), color);

	drawpixel(templong - 3 - bytesperline, color);
	drawpixel(templong + 3 - bytesperline, color);

	drawpixel(templong - 3 + 0, color);
	drawpixel(templong + 3 + 0, color);

	drawpixel(templong - 3 + bytesperline, color);
	drawpixel(templong + 3 + bytesperline, color);

	drawpixel(templong - 2 + (bytesperline<<1), color);
	drawpixel(templong + 2 + (bytesperline<<1), color);

	drawpixel(templong - 1 + (bytesperline*3), color);
	drawpixel(templong + 0 + (bytesperline*3), color);
	drawpixel(templong + 1 + (bytesperline*3), color);
	enddrawing();
}





void Draw2dMarker( int x, int y, int color)
{
	begindrawing();
	BYTE *templong = (BYTE*)(y * bytesperline + x + frameplace);

	drawpixel(templong - 1 - (bytesperline<<2), color);
	drawpixel(templong + 0 - (bytesperline<<2), color);
	drawpixel(templong + 1 - (bytesperline<<2), color);

	drawpixel(templong - 3 - (bytesperline*3), color);
	drawpixel(templong - 2 - (bytesperline*3), color);
	drawpixel(templong + 2 - (bytesperline*3), color);
	drawpixel(templong + 3 - (bytesperline*3), color);

	drawpixel(templong - 3 - (bytesperline<<1), color);
	drawpixel(templong - 2 - (bytesperline<<1), color);
	drawpixel(templong + 2 - (bytesperline<<1), color);
	drawpixel(templong + 3 - (bytesperline<<1), color);

	drawpixel(templong - 4 - bytesperline, color);
	drawpixel(templong - 1 - bytesperline, color);
	drawpixel(templong + 1 - bytesperline, color);
	drawpixel(templong + 4 - bytesperline, color);

	drawpixel(templong - 4, color);
	drawpixel(templong + 0, color);
	drawpixel(templong + 4, color);

	drawpixel(templong - 4 + bytesperline, color);
	drawpixel(templong - 1 + bytesperline, color);
	drawpixel(templong + 1 + bytesperline, color);
	drawpixel(templong + 4 + bytesperline, color);

	drawpixel(templong - 3 + (bytesperline<<1), color);
	drawpixel(templong - 2 + (bytesperline<<1), color);
	drawpixel(templong + 2 + (bytesperline<<1), color);
	drawpixel(templong + 3 + (bytesperline<<1), color);

	drawpixel(templong - 3 + (bytesperline*3), color);
	drawpixel(templong - 2 + (bytesperline*3), color);
	drawpixel(templong + 2 + (bytesperline*3), color);
	drawpixel(templong + 3 + (bytesperline*3), color);

	drawpixel(templong - 1 + (bytesperline<<2), color);
	drawpixel(templong + 0 + (bytesperline<<2), color);
	drawpixel(templong + 1 + (bytesperline<<2), color);
	enddrawing();
}

void DrawCircle( int x, int y, int radius, int nColor, BOOL dashed)
{
	register bool skip = false;
	register int a, x0 = x + radius, y0 = y, x1, y1;

	begindrawing();
	for (a = kAng5; a <= kAng360; a += kAng5, skip^=1)
	{
		x1 = x + mulscale30(Cos(a), radius);
		y1 = y + mulscale30(Sin(a), radius);
		if (!dashed || !skip)
			drawline16(x0, y0, x1, y1, nColor);
		
		x0 = x1;
		y0 = y1;
	}
	enddrawing();
}

BOOL objectHaveCaption(int otype, int oidx)
{
	char* t = NULL;
	switch (otype) {
		case OBJ_WALL:
		case OBJ_MASKED:
			t = ExtGetWallCaption(oidx);
			break;
		case OBJ_SPRITE:
			t = ExtGetSpriteCaption(oidx);
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
			t = ExtGetWallCaption(oidx);
			break;
	}
	
	return (t && t[0]);
}

void draw2dText(char* str, int cx, int cy, int xp, int yp, int nZoom, char fColor, short bColor = -1, int xf = 0, int yf = 0, QFONT* pFont = qFonts[1])
{	
	register int wh  = gfxGetTextLen(str, pFont) >> 1;
	register int hg  = pFont->height >> 1;
	register int x1, y1, x2, y2;
	
	cx = halfxdim16 + mulscale14(cx - xp, nZoom);
	cy = midydim16  + mulscale14(cy - yp, nZoom);
	
	x1 = cx - wh - 1;		x2 = cx + wh + 1;
	y1 = cy - hg - 1;		y2 = cy + hg + 1;
	if (x1 > 0 && x2 < xdim && y1 > 0 && y2 < ydim16)
	{
		if (bColor >= 0)
		{
			gColor = bColor;
			gfxFillBox(x1 + xf, y1 + yf, x2 + xf, y2 + yf);
		}
		
		gfxDrawText(cx + xf - wh, cy + yf - hg, fColor, str, pFont);
	}
}

#define kPadOff 2
#define kPadOn  3
#define kCaptFont 0
void draw2dCaptionSect(int nSect, int xp, int yp, int nZoom, BOOL hover, QFONT* pFont = qFonts[kCaptFont]) {
	
	register int x1, y1, x2, y2;
	register int dax, day, pd, wh, hg;
	
	char* caption = ExtGetSectorCaption(nSect);
	if (!caption[0])
		return;
	
	avePointSector(nSect, &dax, &day);
	dax = halfxdim16 + mulscale14(dax - xp, nZoom);
	day = midydim16  + mulscale14(day - yp, nZoom);
	
	pd = (hover) ? kPadOn : kPadOff;
	wh = gfxGetTextLen(caption, pFont) >> 1;
	hg = pFont->height >> 1;
	
	x1 = dax - wh - pd,		x2 = dax + wh + pd;
	y1 = day - hg - pd,		y2 = day + hg + pd;
	if (x1 > 0 && x2 < xdim && y1 > 0 && y2 < ydim16)
	{
		char nColor = kColorLightGray;
		if (hover)
			nColor ^= 8;
		
		gfxSetColor(gStdColor[nColor]);
		gfxFillBox(x1, y1, x2, y2);
		gfxDrawText(x1 + pd, y1 + pd, clr2std(kColorBlack), caption, pFont);
	}
}

void draw2dCaptionWall(int nWall, int xp, int yp, int nZoom, BOOL hover, QFONT* pFont = qFonts[kCaptFont]) {
	
	register int x1, y1, x2, y2;
	register int dax, day, pd, wh, hg;

	char* caption = ExtGetWallCaption(nWall);
	if (!caption[0])
		return;
	
	avePointWall(nWall, &dax, &day);	
	dax = halfxdim16 + mulscale14(dax - xp, nZoom);
	day = midydim16 +  mulscale14(day - yp, nZoom);
	
	pd = (hover) ? kPadOn : kPadOff;
	wh = gfxGetTextLen(caption, pFont) >> 1;
	hg = pFont->height >> 1;
	
	x1 = dax - wh - pd,		x2 = dax + wh + pd;
	y1 = day - hg - pd,		y2 = day + hg + pd;
	if (x1 > 0 && x2 < xdim && y1 > 0 && y2 < ydim16)
	{
		char nColor = kColorRed;
		if (hover)
			nColor ^= h;
		
		gfxSetColor(clr2std(nColor));
		gfxFillBox(x1, y1, x2, y2);
		gfxDrawText(x1 + pd, y1 + pd, clr2std(kColorBlack), caption, pFont);
	}
	
}

void draw2dCaptionSpr(int nSpr, int xp, int yp, int nZoom, BOOL hover, QFONT* pFont = qFonts[kCaptFont]) {
	
	register int x1, y1, x2, y2;
	register int dax, day, pd, wh, hg;
	
	char* caption = ExtGetSpriteCaption(nSpr);
	if (!caption[0])
		return;
	
	dax = halfxdim16 + mulscale14(sprite[nSpr].x - xp, nZoom);
	day = midydim16  + mulscale14(sprite[nSpr].y - yp, nZoom);
	
	pd = (hover) ? kPadOn : kPadOff;
	wh = gfxGetTextLen(caption, pFont) >> 1;
	hg = pFont->height >> 1;
	
	x1 = dax - wh - pd,		x2 = dax + wh + pd;
	y1 = day - hg - pd,		y2 = day + hg + pd;
	if (x1 > 0 && x2 < xdim && y1 > 0 && y2 < ydim16)
	{
		char nTColor, nBColor;
		short flags = sprite[nSpr].cstat;
		if (flags & kSprMoveForward) nBColor = kColorBlue;
		else if (flags & kSprMoveReverse) nBColor = kColorGreen;
		else nBColor = kColorCyan;
		
		// if sprite is blocking, make the background colors brighter
		if (flags & kSprBlock)
			nBColor ^= 8;
		
		if (flags & kSprHitscan) nTColor = kColorRed;
		else if (nBColor == kColorBlue) nTColor = kColorLightGray;
		else nTColor = kColorBlack;
		
		if (hover)
			nTColor ^= h, nBColor ^= h;
		
		gfxSetColor(clr2std(nBColor));
		gfxFillBox(x1, y1, x2, y2);
		gfxDrawText(x1+pd, y1+pd, clr2std(nTColor), caption, pFont);
	}
}

char wallCstat2Color(int nWall) {
	
	register short flags = wall[nWall].cstat;
	if (flags & kWallMoveForward)		return kColorLightBlue;
	else if (flags & kWallMoveReverse)	return kColorLightGreen;
	else if (wall[nWall].nextwall < 0)	return kColorWhite;
	else if (flags & kWallHitscan)		return kColorMagenta;
	else								return kColorRed;
}

void draw2dArrow(int x1, int y1, int nAng, int nSiz, int nZoom, int nColor, char thick = FALSE) {
	
	int x0, y0;
	if (x1 > 2 && x1 < xdim - 5 && y1 > 2 && y1 < ydim16 - 5)
	{
		scaleAngLine2d(nSiz, nAng, &x0, &y0);
		Draw2dWall(x1 + x0, y1 + y0, x1 - x0, y1 - y0, nColor, thick);
		Draw2dWall(x1 + x0, y1 + y0, x1 + y0, y1 - x0, nColor, thick);
		Draw2dWall(x1 + x0, y1 + y0, x1 - y0, y1 + x0, nColor, thick);
	}

}

void draw2dArrowMarker(int xS, int yS, int xD, int yD, int nColor, int nZoom, int nAng, char thick, int pat1, int pat2) {
	
	int nAngle, sAng, sZoom;
	Draw2dWall(xS, yS, xD, yD, nColor, thick, pat1);
	if (nAng > 0)
	{
		nAngle	= getangle(xS - xD, yS - yD);
		sZoom	= nZoom / 64;

		sAng = nAngle + nAng; 
		xS = mulscale30(sZoom, Cos(sAng));
		yS = mulscale30(sZoom, Sin(sAng));
		Draw2dWall(xD, yD, xD + xS, yD + yS, nColor, thick, pat2);
		
		sAng = nAngle - nAng;
		xS = mulscale30(sZoom, Cos(sAng));
		yS = mulscale30(sZoom, Sin(sAng));
		Draw2dWall(xD, yD, xD + xS, yD + yS, nColor, thick, pat2);
	}
}

BOOL inHgltRange(int x, int y)
{	
	int hx1 = hgltx1, hx2 = hgltx2, hy1 = hglty1, hy2 = hglty2;
	
	if (hx1 > hx2) swapValues(&hx1, &hx2);
	if (hy1 > hy2) swapValues(&hy1, &hy2);	
	return (x > hx1 && x < hx2 && y > hy1 && y < hy2);
}

void qdraw2dgrid(int posxe, int posye, short ange, int zoome, short gride)
{
	register int i, xp1 = 0, yp1, xp2 = 0, yp2;
	int siz = editorgridextent;
	int tempy = 0x80000000l;
	int dst = 2048 >> gride;
	int cnt = 0;
	
	gfxSetColor(gStdColor[29]);
	yp1 = midydim16 - mulscale14(posye + siz, zoome);
	yp2 = midydim16 - mulscale14(posye - siz, zoome);
	for(i = -siz; i <= siz && xp1 < xdim; i += dst)
	{
		xp2 = xp1;
		xp1 = halfxdim16 - mulscale14(posxe - i, zoome);
		if (xp2 == xp1) break;
		else gfxVLine(xp1, yp1, yp2);
	}

	xp1 = halfxdim16 - mulscale14(posxe + siz, zoome);
	xp2 = halfxdim16 - mulscale14(posxe - siz, zoome);
	for(i = -siz; i <= siz; i += dst)
	{
		yp1 = mulscale14(posye - i, zoome);
		if (yp1 == tempy) continue;
		else if (yp1 > midydim16 - ydim16 && yp1 <= midydim16)
		{
			gfxHLine(midydim16 - yp1, xp1, xp2);
			tempy = yp1;
		}
	}
}


void fillsector(int nSect, char nColor, int nStep = 2)
{
	static int fillist[640];
	int swal, ewal, x1, x2, y1, y2, y, dax, miny, maxy, fillcnt;
	register int i, j, sy;
	
	gfxSetColor(nColor);
	getSectorWalls(nSect, &swal, &ewal);
	
	miny = maxy = wall[swal].y;
	for(i = swal; i <= ewal; i++)
	{
		y1 = wall[i].y;
		y2 = wall[wall[i].point2].y;
		if (y1 < miny) miny = y1;
		if (y2 < miny) miny = y2;
		if (y1 > maxy) maxy = y1;
		if (y2 > maxy) maxy = y2;
	}
	
	
	miny = ClipLow(midydim16 + mulscale14(miny  - posy, zoom), 0);
	maxy = ClipHigh(midydim16 + mulscale14(maxy - posy, zoom), ydim16);
	
	for(sy = miny & nStep; sy <= maxy; sy += nStep)
	{
		fillist[0] = 0; fillcnt = 1;
		y = posy + divscale14(sy - midydim16, zoom);
		
		for(i = swal; i <= ewal; i++)
		{
			getWallCoords(i, &x1, &y1, &x2, &y2);
			if (y1 > y2)
			{
				swapValues(&x1, &x2);
				swapValues(&y1, &y2);
			}
			
			if (y1 <= y && y2 > y)
			{
				dax = x1+scale(y-y1,x2-x1,y2-y1);
				dax = halfxdim16 + mulscale14(dax - posx, zoom);
				if (dax >= 0)
					fillist[fillcnt++] = dax;
			}
		}

		for(i = 1; i < fillcnt; i++)
		{
			for (j = 0; j < i; j++)
			{
				if (fillist[i] >= fillist[j]) continue;
				else swapValues(&fillist[i], &fillist[j]);
			}
		}
		
		j = fillcnt - 1;
		for (i = (fillcnt & 1); i < j; i += 2)
		{
			if (fillist[i] >= xdim) break;
			else gfxHLine(sy, fillist[i], fillist[i+1]);
		}
	}
}

void fillSectorTransluc(int nSect, char color, int nTransluc = kRSTransluc, int nStep = 1)
{
	int nTile = gSysTiles.sectfil;
	memset((void*)waloff[nTile], 255, xdim*ydim);
	setviewtotile(nTile, ydim, xdim);
	
	fillsector(nSect, color, nStep);
	
	setviewback();
	rotatesprite((xdim>>1)<<16, (ydim>>1)<<16, 0x10000, 512, nTile, 0, 0, kRSNoMask | kRSNoClip | kRSYFlip | nTransluc, 0, 0, xdim, ydim16);
}

void hgltSectFill(char nColor, char nStep = 1, BOOL transluc = TRUE)
{
	register int i;
	if (transluc)
	{
		int nTile = gSysTiles.sectfil;
		memset((void*)waloff[nTile], 255, xdim*ydim);
		setviewtotile(nTile, ydim, xdim);
		
		for(i = 0; i < highlightsectorcnt; i++)
			fillsector(highlightsector[i], nColor, nStep);
		
		setviewback();
		rotatesprite((xdim>>1)<<16, (ydim>>1)<<16, 0x10000, 512, nTile, 0, 0, kRSNoMask | kRSNoClip | kRSYFlip | kRSTransluc2, 0, 0, xdim, ydim16);
	}
	else
	{
		for(i = 0; i < highlightsectorcnt; i++)
			fillsector(highlightsector[i], nColor, nStep);
	}
}

void qdraw2dscreen(int x, int y, short nAngle, int nZoom, short nGrid )
{
	char nColor, nColor2;
	int newwall = -1, newwall2;
	int x0, y0, x1, y1, x2, y2, ax0, ay0, ax1, ay1, thick, len;
	int xd = xdim - 5, yd = ydim16 - 5, flags, dax, day;
	int szoom, sang, endwall;
	short nType, nStat, nAng;
	register int i, j, k, f;
	BOOL hglt; BYTE tags = 0;

	if (showtags)
	{
		if (zoom >= 1024) tags |= 0x01;
		if (zoom >= 512 && (linehighlight >= 0 || pointhighlight >= 0))
			tags |= 0x02;
	}

	if (grid)
		qdraw2dgrid(x, y, nAngle, nZoom, grid);
		
	// flood sectors using translucent rotatesprite
	////////////////////////
	if (gMisc.useTranslucency)
	{
		// flood current sector
		if (sectorhighlightfill >= 0)
			fillSectorTransluc(sectorhighlightfill, clr2std(28));
		
		
		// flood sectors in a highlight
		if (highlightsectorcnt > 0)
			hgltSectFill(gStdColor[kColorGreen], 1, TRUE);
	}
	else if (highlightsectorcnt > 0)
		hgltSectFill(gStdColor[kColorGreen], 3, FALSE);
	
	// flood sector waiting for merging
	////////////////////////
	if (joinsector[0] >= 0)
		fillsector(joinsector[0], clr2std(kColorBrown));

	// draw start arrow
	///////////////////////
	x0 = halfxdim16 + mulscale14(startposx - x, zoom);
	y0 = midydim16  + mulscale14(startposy - y, zoom);
	draw2dArrow(x0, y0, startang, 80, zoom, clr2std(kColorBrown), TRUE);

	// draw walls
	////////////////////////////
	for (i = 0; i < numwalls; i++)
	{
		// only draw one side of walls
		if (wall[i].nextwall > i)
			continue;

		thick = 0;
		if (wall[i].point2 < numwalls)
		{
			if (wall[i].nextwall == -1)
			{
				flags  = wall[i].cstat;
				nColor = 17;
			}
			else
			{
				flags = wall[i].cstat | wall[wall[i].nextwall].cstat;

				// if one side is highlighted, only show that side's flags
				if (i == linehighlight) flags = wall[i].cstat;
				else if (wall[i].nextwall == linehighlight)
					flags = wall[wall[i].nextwall].cstat;
				
				if (flags & kWallHitscan) nColor = kColorMagenta;
				else nColor = kColorRed;
				
				if (flags & kWallBlock)
					thick = 1;
			}
						
			if (flags & kWallMoveForward) nColor = kColorLightBlue;
			else if (flags & kWallMoveReverse)
				nColor = kColorLightGreen;
			
			if (linehighlight >= 0 && (i == linehighlight || wall[i].nextwall == linehighlight))
				nColor ^= h;
		}
		else // you are in sector drawing mode
		{
			newwall = (short)i;
			nColor = kColorLightGray;
		}

		getWallCoords(i, &x0, &y0, &x1, &y1);
		x0 = halfxdim16 + mulscale14(x0 - x, zoom);
		y0 = midydim16  + mulscale14(y0 - y, zoom);
		x1 = halfxdim16 + mulscale14(x1 - x, zoom);
		y1 = midydim16  + mulscale14(y1 - y, zoom);
		
		Draw2dWall(x0, y0, x1, y1, clr2std(nColor), thick);
		if (zoom >= 300 && x0 > 4 && x0 < xd && y0 > 4 && y0 < yd)
		{
			nColor  = clr2std(kColorBrown); nColor2 = clr2std(28);
			Draw2dVertex(x0, y0, nColor, nColor2, zoom > 768 ? 2 : 1);
		}
	}

	// draw wall midpoint closest to mouse cursor
	////////////////////////////
	if (linehighlight >= 0)
	{
		getclosestpointonwall(mousxplc, mousyplc, linehighlight, &x1, &y1);
		if (gridlock)
			doGridCorrection(&x1, &y1, grid);
		
		x0 = halfxdim16 + mulscale14(x1 - x, zoom);
		y0 = midydim16  + mulscale14(y1 - y, zoom);
		nColor  = clr2std(wallCstat2Color(linehighlight) ^ h);
		Draw2dCross(x0, y0, nColor, 1);
	}
	
	// redraw highlighted wall points
	////////////////////////////
	if (zoom >= 256)
	{
		for (i = 0; i < numwalls; i++)
		{
			x0 = halfxdim16 + mulscale14(wall[i].x - x, zoom);
			y0 = midydim16  + mulscale14(wall[i].y - y, zoom);
			if (x0 > 4 && x0 < xd && y0 > 4 && y0 < yd)
			{
				hglt = (i == pointhighlight || (highlightcnt > 0 && TestBitString(show2dwall, i)));
				if (!hglt) hglt = (((shift & 0x02) || (alt & 0x02)) && inHgltRange(x0, y0));
				if (hglt)
					Draw2dVertex(x0, y0, clr2std(kColorBrown), clr2std(kColorBrown ^ h), 3);
			}
		}
	}
	
/* 	if (gCircle.line >= 0)
	{
		gCircle.Draw();
		scrSetMessage("??");
	} */
	
	
	for (i = 0; i < numsectors; i++)
	{		
		// draw busy / motion progress of the sector
		////////////////////////////
		if (gPreviewMode && sector[i].extra > 0)
		{
			XSECTOR* pXSector = &xsector[sector[i].extra];
			if (pXSector->busy > 0 && pXSector->busy < 65536)
			{
				if ((pXSector->unused1 && h) || !pXSector->unused1)
					fillsector(i, clr2std((pXSector->state) ? kColorRed : kColorBlue));
			}
			else if ((pXSector->bobFloor || pXSector->bobCeiling) && (pXSector->bobAlways || pXSector->state))
			{
				fillsector(i, clr2std(kColorDarkGray));
			}
		}
		
		if (zoom <= 128)
			continue;
		
		// draw sprite icons
		////////////////////////////
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			x0 = halfxdim16 + mulscale14(sprite[j].x - x, zoom);
			y0 = midydim16  + mulscale14(sprite[j].y - y, zoom);
			BOOL onscreen = (x0 > 4 && x0 < xd && y0 > 4 && y0 < yd);
			
			if ((j | 0x4000) == pointhighlight) hglt = 1;
			else if ((shift & 0x02) && inHgltRange(x0, y0)) hglt = 2;
			else if (highlightcnt > 0 && TestBitString(show2dsprite, j)) hglt = 3;
			else hglt = 0;
			
			int nXSprite = sprite[j].extra;
			if (sprite[j].extra <= 0 || sprite[j].extra > kMaxXSprites)
				nXSprite = 0;

			nStat = sprite[j].statnum;	nType = sprite[j].type;
			nAng = sprite[j].ang;		flags = sprite[j].cstat;

			thick = (flags & kSprBlock) ? 1 : 0;

			nColor = kColorCyan;
			if (flags & kSprHitscan) nColor = kColorMagenta;
			if (flags & kSprInvisible) nColor = 24;
			if (flags & kSprMoveForward) nColor = kColorLightBlue;
			else if (flags & kSprMoveReverse) nColor = kColorLightGreen;
			if (nColor == kColorCyan || nColor == kColorMagenta)
			{
				if (nStat == kStatItem) nColor = kColorLightMagenta;
				else if (nStat == kStatDude || nType == kMarkerDudeSpawn)
					nColor = kColorLightRed;
			}

			if (hglt)
			{
				nColor ^= h;
				if (nXSprite && xsprite[nXSprite].triggerProximity) // proximity trigger radius
					DrawCircle(x0, y0, mulscale10(ClipLow(sprite[j].clipdist * 3, 8), zoom), clr2std(nColor), TRUE);
			}
						
			if (nStat == kStatPathMarker)
			{
				if (nXSprite)
				{
					XSPRITE* pXMark1 = &xsprite[nXSprite];
					if (pXMark1->data2 >= 0 && pXMark1->data1 != pXMark1->data2)
					{
						ax0 = x0, ay0 = y0;
						// draw movement line to the destination marker
						for (k = headspritestat[nStat]; k != -1; k = nextspritestat[k])
						{
							spritetype *pMark2 = &sprite[k];
							if (pMark2->extra <= 0 || pMark2->index == sprite[j].index)
								continue;
							
							XSPRITE* pXMark2 = &xsprite[pMark2->extra];
							if (pXMark2->data1 == pXMark1->data2)
							{
								ax1 = halfxdim16 + mulscale14(sprite[pMark2->index].x - x, zoom);
								ay1 = midydim16 + mulscale14(sprite[pMark2->index].y - y, zoom);
								nColor2 = clr2std((hglt) ? kColorLightMagenta ^ h : kColorLightMagenta);
								Draw2dWall(ax0, ay0, ax1, ay1, nColor2, hglt, kPatDotted);
							}
						}
					}
				}

				if (onscreen)
				{
					nColor = clr2std(nColor);
					Draw2dFaceSprite(x0, y0, nColor);
					Draw2dCross(x0, y0, clr2std(kColorYellow), 1);
					scaleAngLine2d(100, nAng, &x1, &y1);
					Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
				}

				if (hglt)
					DrawCircle(x0, y0, mulscale10(ClipLow(sprite[j].clipdist << 1, 8), zoom), nColor, TRUE);
			}
			else if (nStat == kStatMarker)
			{
				int nXSector = sector[sprite[j].owner].extra;
				if (sprite[j].type == kMarkerOff)
				{
					// draw movement arrow
					int nSprite2 = xsector[nXSector].marker1;
					ax1 = halfxdim16 + mulscale14(sprite[nSprite2].x - x, zoom);
					ay1 = midydim16  + mulscale14(sprite[nSprite2].y - y, zoom);
					draw2dArrowMarker(x0, y0, ax1, ay1, clr2std(kColorYellow), zoom);
				}

				if (onscreen)
				{
					switch(nType) {
						case kMarkerOff:
						case kMarkerOn:
							nColor = clr2std((sprite[j].owner == sectorhighlight) ? kColorGreen : kColorLightCyan);
							if (nType != kMarkerOff) Draw2dCross(x0, y0, nColor, 4);
							else DrawBuild2dFaceSprite(x0, y0, clr2std(kColorYellow));
							if (nAng != 0)
							{
								scaleAngLine2d(128, nAng, &x1, &y1);
								Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, 0);
							}
							break;
						case kMarkerWarpDest:
						case kMarkerAxis:
							if (sprite[j].owner == sectorhighlight) nColor = kColorGreen;
							else nColor = (nType == kMarkerWarpDest) ? kColorBrown : kColorLightCyan;
							if (hglt)
								nColor ^= h;

							nColor = clr2std(nColor);
							Draw2dMarker(x0, y0, nColor);
							scaleAngLine2d(128, nAng, &x1, &y1);
							Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, 0);
							break;
					}
				}
			}
			else if (nStat == kStatAmbience)
			{
				if (onscreen)
				{
					nColor = (nXSprite && xsprite[nXSprite].state) ? kColorYellow : kColorBrown;
					if (hglt)
						nColor ^= h;
					
					nColor = clr2std(nColor);
					Draw2dCross(x0, y0, nColor, 2);
					Draw2dFaceSprite(x0, y0, nColor);
				}
				if (gMisc.ambShowRadius > 0 && nXSprite)
				{
					if ((gMisc.ambShowRadiusHglt && hglt) || (!gMisc.ambShowRadiusHglt && (!hglt || (hglt && h))))
					{
						int radius1 = (gMisc.ambShowRadius & 0x1) ? mulscale10(xsprite[nXSprite].data1, zoom) : 0;
						int radius2 = (gMisc.ambShowRadius & 0x2) ? mulscale10(xsprite[nXSprite].data2, zoom) : 0;
						if (xsprite[nXSprite].state)
						{
							if (radius1) DrawCircle(x0, y0, radius1, clr2std(kColorYellow));
							if (radius2) DrawCircle(x0, y0, radius2, clr2std(kColorBrown));
						}
						else if (radius1) DrawCircle(x0, y0, radius1, clr2std(kColorLightGray));
						else if (radius2) DrawCircle(x0, y0, radius2, clr2std(kColorDarkGray));
					}
				}
			}
			else if (onscreen)
			{
				if (nStat == kStatFX)
				{
					k = tileGetPic(sprite[j].picnum);
					if (!tilesizx[k] || !tilesizy[k])
						Draw2dCross(x0, y0, clr2std(nColor ^ h), 2);
					else
					{
						int wh, hg; f = ClipRange(zoom/256, 6, 22);
						tileDrawGetSize(k, f, &wh, &hg); wh >>= 1, hg >>= 1;
						tileDrawTile(x0 - wh, y0 - hg, k, f, sprite[j].pal, 0x02);
					}
				}
				else if (nStat == kStatExplosion)
				{
					if (nType >= 0 && nType < kExplosionMax)
					{
						nColor = clr2std(kColorLightGray ^ h);
						DrawCircle(x0, y0, mulscale10(explodeInfo[nType].radius, zoom), nColor);
						Draw2dCross(x0, y0, nColor, 4);
					}
				}
				else if (nStat == kStatProjectile)
				{
					nColor = clr2std((h) ? kColorLightBlue : kColorLightGreen);
					DrawCircle(x0, y0, mulscale12(sprite[j].clipdist, zoom), nColor);
					DrawBuild2dFaceSprite(x0, y0, nColor);
					Draw2dCross(x0, y0, nColor, 1);
				}
				else if ((nType >= 6 && nType <= 14 && nType != 8) || (nType >= 700 && nType <= 702) || (nType >= 706 && nType <= 711) || nType == 459 || nType == 19)
				{
					DrawBuild2dFaceSprite(x0, y0, clr2std(nColor));
				}
				else if (nType == kMarkerSPStart || nType == kMarkerMPStart)
				{
					nColor = xsprite[nXSprite].data1 & 7;
					if (hglt)
						nColor ^= h;
				
					draw2dArrow(x0, y0, nAng, 100, zoom, clr2std(8 + nColor));
				}
				else if (nType == kModernStealthRegion)
				{
					if (nXSprite && xsprite[nXSprite].data1 >= 0)
					{
						int radius = mulscale10(xsprite[nXSprite].data1, zoom);
						int value1 = xsprite[nXSprite].data2;
						int value2 = xsprite[nXSprite].data3;

						if (value1 > 0 || value2 > 0) nColor = kColorLightRed;
						else nColor = kColorCyan;
						if (hglt)
							nColor ^= h;
						
						nColor = clr2std(nColor);
						if (xsprite[nXSprite].data1 > 0) DrawCircle(x0, y0, radius, nColor, TRUE);
						else if (hglt)
							fillsector(i, nColor);
						
						Draw2dFaceSprite(x0, y0, nColor);
						Draw2dCross(x0, y0, nColor, 4);
					}
				}
				else if ((flags & kSprRelMask) == kSprFace)
				{
					nColor = clr2std(nColor);
					if (thick) Draw2dFaceSprite(x0, y0, nColor);
					else DrawBuild2dFaceSprite(x0, y0, nColor);
					scaleAngLine2d(80, nAng, &x1, &y1);
					Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
				}
				else if ((flags & kSprRelMask) == kSprWall)
				{
					nColor = clr2std(nColor);
					if (!thick)
						DrawBuild2dFaceSprite(x0, y0, nColor);

					if (tilesizx[sprite[j].picnum])
					{
						k = mulscale6(tilesizx[sprite[j].picnum], sprite[j].xrepeat);
						x1 = mulscale13(sintable[(nAng + kAng180) & kAngMask], zoom) * k / 0x1000;
						y1 = mulscale13(sintable[(nAng + kAng90)  & kAngMask], zoom) * k / 0x1000;
						Draw2dWall(x0 - x1, y0 - y1, x0 + x1, y0 + y1, nColor, thick, kPatDotted);
					}
					else
					{
						scaleAngLine2d(164, (nAng + kAng90) & kAngMask, &x1, &y1);
						Draw2dWall(x0 - x1, y0 - y1, x0 + x1, y0 + y1, nColor, thick);
					}
					
					if (flags & kSprOneSided)
					{
						scaleAngLine2d(128, nAng, &x1, &y1);
						Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
					}
					else
					{
						// back
						scaleAngLine2d(182, nAng, &x1, &y1);
						Draw2dWall(x0 - x1, y0 - y1, x0 + x1, y0 + y1, nColor, thick);

						// face side longer than back one
						scaleAngLine2d(128, nAng, &x1, &y1);
						Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
					}
				}
				else if ((flags & kSprRelMask) >= kSprFloor)
				{
					nColor = clr2std(nColor);
					if ((flags & kSprBlock) && (flags & kSprOneSided))
					{
						if (flags & kSprFlipY) drawline16(x0-3,y0-3,x0+3,y0+3, nColor);
						else drawline16(x0-3,y0+3,x0+3,y0-3, nColor);
					}

					Draw2dFloorSprite(x0, y0, nColor);
					scaleAngLine2d(200, nAng, &x1, &y1);
					Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
					if (hglt || zoom >= 1024)
						Draw2dFloorSpriteFull(x0, y0, nColor, j);
				}
			}
		}
	}
	
	// sector drawing process
	////////////////////////
	if (newwall != -1)
	{
		newwall2 = newwall;
		nColor = clr2std(kColorLightGray);
		for (i = numwalls - 1; i >= 0; i--)
		{
			if (wall[i].point2 != newwall2)
				continue;
			
			getWallCoords(i, &x0, &y0, &x1, &y1);
			
			x0 = halfxdim16 + mulscale14(x0 - x, zoom);
			y0 = midydim16  + mulscale14(y0 - y, zoom);
			x1 = halfxdim16 + mulscale14(x1 - x, zoom);
			y1 = midydim16  + mulscale14(y1 - y, zoom);

			Draw2dWall(x0, y0, x1, y1, nColor, 0);
			Draw2dWallMidPoint(i, nColor, 0x03);
			newwall2 = i, i = numwalls - 1;
		}
		
		scrSetMessage("Wall #%d: Length = %d, Angle = %d", newwall, getWallLength(newwall), GetWallAngle(newwall) & kAngMask);
		Draw2dWallMidPoint(newwall, nColor, 0x03);

		// draw midpoints on all walls of sector
		if ((i = sectorhighlight) >= 0)
		{
			getSectorWalls(i, &j, &endwall);
			while(j <= endwall)
			{
				nColor = wallCstat2Color(j);
				if (j == linehighlight) 
					nColor ^= h;
				
				Draw2dWallMidPoint(j++, nColor);
			}
		}		
	}
	else if (linehighlight >= 0) // only draw midpoint on current wall
	{
		Draw2dWallMidPoint(linehighlight, wallCstat2Color(linehighlight) ^ h);
	}
	
	// draw object captions
	////////////////////////
	if (tags & 0x01)
	{
		i = 0;
		while (i < numsectors)
		{
			// draw sector captions
			draw2dCaptionSect(i, x, y, zoom, (sectorhighlight == i));
			
			// draw wall captions
			getSectorWalls(i, &j, &endwall);
			while(j <= endwall)
				draw2dCaptionWall(j++, x, y, zoom, TestBitString(show2dwall, j));
			
			// draw sprite captions
			j = headspritesect[i];
			while(j >= 0)
			{
				draw2dCaptionSpr(j, x, y, zoom, TestBitString(show2dsprite, j));
				j = nextspritesect[j];
			}
			
			i++;
		}
		
		// redraw captions of a hovered objects
		////////////////////////		
		if (pointhighlight >= 0)
		{
			if ((pointhighlight & 0xc000) != 16384)
			{
				draw2dCaptionWall(pointhighlight, x, y, zoom, TRUE);
			}
			else
			{
				draw2dCaptionSpr(pointhighlight&16383, x, y, zoom, TRUE);
			}
		}
		else if (linehighlight >= 0)
		{
			draw2dCaptionWall(linehighlight, x, y, zoom, TRUE);
		}
	}
	
	// indicate first and alignto walls
	////////////////////////////
	if (sectorhighlightfill >= 0 && zoom > 100)
	{
		i = sector[sectorhighlightfill].wallptr;
		if (zoom > 512 || (gFrameClock & (kHClock<<1)))
		{
			avePointWall(i, &x0, &y0);
			
			k = 0; // caption y-offset
			if ((tags & 0x01) && objectHaveCaption(OBJ_WALL, i))
				k = (qFonts[kCaptFont]->height + 4);
			
			draw2dText("F", x0, y0, x, y, zoom, clr2std(kColorYellow), kColorBlack, 0, k);
		}
		
		if (sector[sectorhighlightfill].alignto && (zoom > 512 || !(gFrameClock & (kHClock<<1))))
		{
			i += sector[sectorhighlightfill].alignto;
			avePointWall(i, &x0, &y0);
			
			k = 0; // caption y-offset
			if ((tags & 0x01) && objectHaveCaption(OBJ_WALL, i))
				k = (qFonts[kCaptFont]->height + 4);
			
			draw2dText("A", x0, y0, x, y, zoom, clr2std(kColorWhite), kColorBlack, 0, k);
		}
	}
	
	// draw RX/TX tracker
	////////////////////////
	gXTracker.Draw(x, y, zoom);
	
	// draw map notes
	////////////////////////
	if (gCmtPrefs.enabled)
		gCommentMgr.Draw(x, y, zoom);
	

	// draw white arrow
	////////////////////////	
	scaleAngLine2d(128, ang, &x0, &y0); nColor = clr2std(kColorWhite);
	drawline16(halfxdim16+x0, midydim16+y0, halfxdim16-x0,midydim16-y0, nColor);
	drawline16(halfxdim16+x0, midydim16+y0, halfxdim16+y0,midydim16-x0, nColor);
	drawline16(halfxdim16+x0, midydim16+y0, halfxdim16-y0,midydim16+x0, nColor);
	
}

void hgltDetachSectors() {
	
	int i, j, k, swal, ewal, cnt = highlightsectorcnt;
	for(i = cnt - 1; i >= 0; i--)
	{
		getSectorWalls(highlightsector[i], &swal, &ewal);
		for(j = swal; j <= ewal; j++)
		{
			if (wall[j].nextwall < 0) continue;
			for(k = cnt - 1; k >= 0 && highlightsector[k] != wall[j].nextsector; k--);
			if (k < 0)
			{
				wall[wall[j].nextwall].nextwall = -1;
				wall[wall[j].nextwall].nextsector = -1;
				wall[j].nextwall = -1;
				wall[j].nextsector = -1;
			}
			
		}
	}
	
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
	
	static short hgltType = 0; short *hgltcntptr = NULL;
	short sect2 = -1, ACTION = -1, idx = -1;
	char bdclr, bgclr, keyhold = 0;

	processMove();
	gMouse.Read();
	searchx = ClipRange(searchx + gMouse.dX2, gMouse.left, gMouse.right);
	searchy = ClipRange(searchy + gMouse.dY2, gMouse.top,  gMouse.bottom);
	
	getpoint(searchx, searchy, &x, &y);
	updatesector(x, y, &sectorhighlight);
	
	i = (gMouse.hold & 1);
	if (!i || (i && (pointhighlight & 0xC000) != 0))
		sectorhighlightfill = sectorhighlight;
	
/* 	static int test = 0;
	static int test2 = 0;
	int t1 = perc2val(90, xdim);
	int t2 = perc2val(90, ydim16);
	if (appactive)
	{
		if (gMouse.Moved())
		{
			test = totalclock + 128;
			test2 = test;
		}
		if (totalclock < test)
		{
			if (searchx < t1 || searchx > xdim - t1)	posx += (gFrameTicks * mulscale16(x-posx, test-totalclock));
			else
				test = 0;
			
		}
		
		if (totalclock < test2)
		{
			if (searchy < t2 || searchy > ydim16 - t2)	posy += (gFrameTicks * mulscale16(y-posy, test2-totalclock));
			else
				test2 = 0;
		}
	} */
	

	// !!! for build.c
	mousxplc = x; mousyplc = y;
	
	if (gMouse.hold & 2)
	{
		cursectnum = sectorhighlight;
		searchx = halfxdim16;
		searchy = midydim16;
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
		pointhighlight = getpointhighlight(gMisc.hgltTreshold, x, y, zoom);
		if (linehighlight < 0 && pointhighlight >= 0 && (pointhighlight & 0x4000) == 0)
			linehighlight = pointhighlight;
	}

	if ((type = getHighlightedObject()) == 200 && !gPreviewMode)
	{
		if ((i = countSpritesAt(sprite[searchwall].x, sprite[searchwall].y)) > 1)
		{
			if (zoom >= 256)
			{
				sprintf(buffer, "[%d]", i);
				printextShadowL(searchx + 8, searchy + 4, gStdColor[kColorYellow], buffer);
			}
			
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
		
		//if (gCircle.line >= 0)
		//{
			//gCircle.Setup(x, y);
		//}
		
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
					
					for (i = 0; i < highlightcnt; i++)
					{
						j = highlight[i];
						if ((j & 0xc000) == 0)
						{
							wall[j].x += x;
							wall[j].y += y;
						}
						else
						{
							j = (j & 0x3FFF);
							if (sprite[j].statnum < kMaxStatus)
							{
								spritetype* pSpr =& sprite[j];
								pSpr->x += x, pSpr->y += y;
								if ((sect = pSpr->sectnum) > 0)
								{
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
					if (ctrl)
					{
						i = getSector();
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
			if ((pointhighlight & 0xc000) == 0)
			{
				j = pointhighlight;
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
		}

		if (alt & 0x02) hgltReset(kHgltPoint), hgltType = kHgltSector, keyhold = 1;
		else if (shift & 0x02)
			hgltReset(kHgltSector), hgltType = kHgltPoint, keyhold = 1;

		if (hgltType)
		{
			switch(hgltType) {
				case kHgltPoint:
					hgltcntptr =& highlightcnt;
					bdclr = kColorMagenta;
					bgclr = kColorLightCyan;
					break;
				default:
					hgltcntptr =& highlightsectorcnt;
					bdclr = kColorGreen;
					bgclr = kColorLightGreen;
					break;
			}
			
			if (keyhold)
			{
				if (*hgltcntptr == 0)
				{
					hgltx2 = searchx, hglty2 = searchy;
					drawsq(hgltx1, hglty1, hgltx2, hglty2, clr2std(bdclr ^ h));
					if (gMisc.useTranslucency)
						drawHighlight(hgltx1, hglty1, hgltx2, hglty2, clr2std(bgclr));
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
				getpoint2(&hgltx1, &hglty1);
				getpoint2(&hgltx2, &hglty2);
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
	}
	switch (key) {
/* 		case KEY_C:
			if (gCircle.line >= 0)
			{
				gCircle.line = -1;
			}
			else if (linehighlight >= 0)
			{
				gCircle.line	= linehighlight;
				gCircle.dogrid	= 1;
				gCircle.count 	= 6;
			}
			else
			{
				BeepFail();
			}
			break;
		case KEY_SPACE:
			if (gCircle.line < 0) break;
			break; */
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
			if ((searchsector = getSector()) == -1) break;
			searchstat = OBJ_FLOOR;
		// no break
		case KEY_F8:
			 i = (alt) ? 0 : 1;
			 switch (searchstat) {
				 case OBJ_WALL:
					gXTracker.TrackWall(searchwall, (char)i);
					break;
				 case OBJ_SPRITE:
					gXTracker.TrackSprite(searchwall, (char)i);
					break;
				 case OBJ_FLOOR:
					gXTracker.TrackSector(searchsector, (char)i);
					break;
			 }
			 break;
		case KEY_T:
			if (ctrl)
			{
				//if (alt)
				//{
					//scrSetMessage("Show comments is %s", onOff(gCmtPrefs.enabled ^= 1));
					//Beep(gCmtPrefs.enabled);
				//}
				//else
				//{
					showtags = IncRotate(showtags, kCaptionStyleMax);
					scrSetMessage("Show tags: %s", gCaptionStyleNames[showtags]);
					Beep(showtags);
				//}
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
			SetFirstWall(sectorofwall(linehighlight),linehighlight);
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
			
			getpoint(searchx, searchy, &x, &y);
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
		case KEY_PAD5:
		case KEY_PAD7:
		case KEY_PAD9:
		case KEY_PAD4:
		case KEY_PAD6:
		case KEY_PAD2:
		case KEY_PAD8:
			if (type != 200 || (sprite[searchwall].cstat & kSprRelMask) < kSprWall) break;
			switch (key) {
				case KEY_PAD7:
				case KEY_PAD9:
					i = (key == KEY_PAD7) ? -1 : 1;
					sprite[searchwall].xrepeat = ClipRange(sprite[searchwall].xrepeat + i, 0, 255);
					sprite[searchwall].yrepeat = ClipRange(sprite[searchwall].yrepeat + i, 0, 255);
					break;
				case KEY_PAD4:
				case KEY_PAD6:
					i = (key == KEY_PAD4) ? -1 : 1;
					sprite[searchwall].xrepeat = ClipRange(sprite[searchwall].xrepeat + i, 0, 255);
					break;
				case KEY_PAD2:
				case KEY_PAD8:
					i = (key == KEY_PAD2) ? -1 : 1;
					sprite[searchwall].yrepeat = ClipRange(sprite[searchwall].yrepeat + i, 0, 255);
					break;
				case KEY_PAD5:
					sprite[searchwall].xrepeat = 64;
					sprite[searchwall].yrepeat = 64;
					break;
			}
			scrSetMessage("sprite %i xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
			break;
		case KEY_PAD1:
		case KEY_END:
			gMisc.ambShowRadius = (BYTE)IncRotate(gMisc.ambShowRadius, 5);
			if (gMisc.ambShowRadius == 4 && gMisc.ambShowRadiusHglt != TRUE) {
				gMisc.ambShowRadiusHglt = TRUE;
				gMisc.ambShowRadius = 3;
			} else {
				gMisc.ambShowRadiusHglt = FALSE;
			}
			break;
		case KEY_COMMA:
		case KEY_PERIOD:
			i = (shift) ? ((ctrl) ? 128 : 256) : 512;
			i = (key == KEY_COMMA) ? -i : i;
			if (highlightsectorcnt > 0)
			{
				char nFlags = kFlagSprites;
				if (shift)
					nFlags |= kFlagRpoint;

				hgltSectAvePoint(&dax, &day);
				for (j = 0; j < highlightsectorcnt; j++)
					rotateSector(highlightsector[j], i, dax, day, nFlags);
				
				scrSetMessage("%d of %d sectors rotated by %d.", highlightsectorcnt, numsectors, i);
				BeepOk();
				break;
			}
			// no break
		case KEY_LBRACE:
		case KEY_RBRACE:
			if (type != 200) break;
			i = (shift) ? 16 : 256;
			switch (key) {
				case KEY_COMMA:
				case KEY_LBRACE:
					sprite[searchwall].ang = (short)DecNext(sprite[searchwall].ang, i);
					break;
				case KEY_PERIOD:
				case KEY_RBRACE:
					sprite[searchwall].ang = (short)IncNext(sprite[searchwall].ang, i);
					break;
			}
			
			if (key == KEY_COMMA || key == KEY_PERIOD)
				sprite[searchwall].ang = sprite[searchwall].ang & kAngMask;
			
			scrSetMessage("sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
			ShowSpriteData(searchwall, FALSE);
			BeepOk();
			break;
	}
	
	if (key != 0)
		keyClear();
}


void _fastcall scaleAngLine2d(int scale, int ang, int* x, int* y) {

	int szoom = zoom / scale;
	*x = mulscale30(szoom, Cos(ang));
	*y = mulscale30(szoom, Sin(ang));

}
