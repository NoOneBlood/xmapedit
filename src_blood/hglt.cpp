/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions to work with objects in a highlight of any case (mostly sprites).
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
#include "editor.h"
#include "gameutil.h"
#include "screen.h"
#include "edit2d.h"
#include "edit3d.h"
#include "hglt.h"
#include "gui.h"
#include "grdshd.h"
#include "screen.h"
#include "edit2d.h"
#include "xmpconf.h"
#include "xmpmisc.h"

int hgltx1 = 0, hgltx2 = 0, hglty1 = 0, hglty2 = 0;
short hgltType = 0;

NAMED_TYPE gHgltSectAutoRedWallErrors[] =
{
	{-1, "Unable to find parent sector"},
	{-2, "Wall limits exceeded while tracing outer loop"},
	{-3, "Internal error while tracing outer loop: Didn't reach refwall"},
	{-999, NULL},
};

void sprFixSector(spritetype* pSprite, int) {
	
	register int i;
	int dax = pSprite->x, day = pSprite->y, daz;
	
	if (inside(dax,day,pSprite->sectnum) != 1) {
		daz = ((tilesizy[pSprite->picnum]*pSprite->yrepeat)<<2);
		for(i = 0; i < numsectors; i++) {
			if (inside(dax, day, i) == 1 && pSprite->z >= getceilzofslope((short)i, dax, day)
				&& pSprite->z-daz <= getflorzofslope((short)i,dax,day)) {
						ChangeSpriteSect(pSprite->index, (short)i);
						break;
				}
		}
	}
	
	
}

int hgltSprCount() {
	
	int total = 0;
	for (register int i = 0; i < highlightcnt; i++) {
		if ((highlight[i] & 0xC000) != 0 && hgltSprIsFine(highlight[i] & 16383))
			total++;
	}
	
	return total;
}

int hgltSprCallFunc(HSPRITEFUNC SpriteFunc, int nData) {
	
	int retn = 0;
	register int i, j;
	if (getHighlightedObject() == 200 && !sprInHglt(searchwall)) {
		SpriteFunc(&sprite[searchwall], nData);
		return 0;
	}

	for (i = highlightcnt - 1; i >= 0; i--) {
		
		if ((highlight[i] & 0xC000) == 0)
			continue;
		
		j = highlight[i] & 0x3FFF;
		if (sprite[j].statnum < kMaxStatus) {
			
			SpriteFunc(&sprite[j], nData);
			retn++;
			
		}
	}

	return retn;
}


int hgltSectCallFunc(HSECTORFUNC2 SectorFunc, int arg1, int arg2, int arg3, int arg4) {
	
	int retn = 0; register int i, j;
	
/* 	i = getSector();
	if (i < 0 && highlightsectorcnt <= 0) return 0;
	else if (i >= 0 && hgltCheck(OBJ_SECTOR, i) < 0)
	{
		SectorFunc(i, arg1, arg2, arg3, arg4);
		return 1;
	} */

	for (i = highlightsectorcnt - 1; i >= 0 ; i--)
	{
		SectorFunc(highlightsector[i], arg1, arg2, arg3, arg4);
		retn++;
	}

	return retn;
}

void hgltDetachSectors()
{
	int i = highlightsectorcnt;
	int j, s, e;
	
	while(--i >= 0)
	{
		getSectorWalls(highlightsector[i], &s, &e);
		while(s <= e)
		{
			if (wall[s].nextwall >= 0)
			{
				j = highlightsectorcnt;
				while(--j >= 0 && highlightsector[j] != wall[s].nextsector);
				if (j < 0)
				{
					wallDetach(wall[s].nextwall);
					wallDetach(s);
				}
			}
			
			s++;
		}
	}
}

short hgltCheck(int type, int idx) {
	
	register int i;
	switch (type) {
		case OBJ_WALL:
		case OBJ_MASKED:
			for (i = 0; i < highlightcnt; i++) {
				if ((highlight[i] & 0xC000) != 0) continue;
				else if (highlight[i] == idx) return (short)i;
			}
			break;
		case OBJ_SPRITE:
			for (i = 0; i < highlightcnt; i++) {
				if ((highlight[i] & 0xC000) != 0x4000) continue;
				else if ((highlight[i] & 0x3FFF) == idx) return (short)i;
			}
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			for (i = 0; i < highlightsectorcnt; i++) {
				if (highlightsector[i] == idx) return (short)i;
			}
			break;
	}
	
	return -1;
	
}

BOOL hgltRemove(int type, int idx) {
	
	register int i = -1;
	switch (type) {
		case OBJ_WALL:
		case OBJ_MASKED:
		case OBJ_SPRITE:
			if (highlightcnt > 0) {
				
				if ((i = hgltCheck(type, idx)) < 0) return FALSE;
				else if (type != OBJ_SPRITE) ClearBitString(show2dwall, idx);
				else ClearBitString(show2dsprite, idx);
				while(i < highlightcnt - 1) {
					highlight[i] = highlight[i + 1];
					i++;
				}
				if (--highlightcnt <= 0) highlightcnt = -1;
				return TRUE;
				
			}
			break;
		case OBJ_SECTOR:
		case OBJ_FLOOR:
		case OBJ_CEILING:
			if (highlightsectorcnt > 0) {
				
				if ((i = hgltCheck(type, idx)) < 0) return FALSE;
				else sectorAttach(idx);
				while (i < highlightsectorcnt - 1) {
					highlightsector[i] = highlightsector[i + 1];
					i++;
				}
				if (--highlightsectorcnt <= 0) highlightsectorcnt = -1;
				return TRUE;
				
			}
			break;
	}
	
	return FALSE;
	
}

int hgltAdd(int type, int idx) {
	
	register int retn = -1;
	switch (type) {
		case OBJ_SPRITE:
		case OBJ_WALL:
		case OBJ_MASKED:
			retn = highlightcnt = (short)ClipLow(highlightcnt, 0);
			switch (type) {
				case OBJ_SPRITE:
					highlight[highlightcnt++] = (short)(idx + 16384);
					SetBitString(show2dsprite, idx);
					break;
				default:
					highlight[highlightcnt++] = (short)idx;
					SetBitString(show2dwall, idx);
					break;
			}
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
		{
			retn = highlightsectorcnt = (short)ClipLow(highlightsectorcnt, 0);
			highlightsector[highlightsectorcnt++] = (short)idx;
			
			int s, e;
			getSectorWalls(idx, &s, &e);
			while(s <= e)
			{
				if (wall[s].nextwall >= 0)
					gNextWall[wall[s].nextwall] = wall[wall[s].nextwall].nextwall;
				
				gNextWall[s] = wall[s].nextwall;
				s++;
			}
			
			break;
		}
			
	}
	
	return retn;
	
}

short hglt2dAddInXYRange(int hgltType, int x1, int y1, int x2, int y2) {

	register int i, j; int swal, ewal, cnt = 0;
	for (i = 0; i < numsectors; i++)
	{
		if (hgltType & kHgltPoint)
		{
			getSectorWalls(i, &swal, &ewal);
			for (j = swal; j <= ewal; j++)
			{
				if (wall[j].x < x1 || wall[j].x > x2) continue;
				else if (wall[j].y < y1 || wall[j].y > y2) continue;
				else hgltAdd(OBJ_WALL, j);
				cnt++;
			}
			
			for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
			{
				if (sprite[j].statnum >= kMaxStatus) continue;
				else if (sprite[j].x < x1 || sprite[j].x > x2) continue;
				else if (sprite[j].y < y1 || sprite[j].y > y2) continue;
				else hgltAdd(OBJ_SPRITE, j);
				cnt++;
			}
		}
		
		if (hgltType & kHgltSector)
		{
			getSectorWalls(i, &swal, &ewal);
			for (j = swal; j <= ewal; j++)
			{
				if (wall[j].x < x1 || wall[j].x > x2) break;
				else if (wall[j].y < y1 || wall[j].y > y2) break;
			}
			
			if (j >= ewal + 1)
			{
				hgltAdd(OBJ_SECTOR, i);
				cnt++;
			}
		}
	}
		
	return cnt;
	
}


short hglt2dAdd(int type, int idx) {
	
	short cnt = 0;
	register int i = 0; int x, y, sect;
	switch (type) {
		case OBJ_SPRITE:
		case OBJ_WALL:
		case OBJ_MASKED:
			highlightcnt = (short)ClipLow(highlightcnt, 0);
			switch (type) {
				case OBJ_SPRITE:
					x = sprite[idx].x; y = sprite[idx].y, sect = sprite[idx].sectnum;
					for (i = headspritesect[sect]; i >= 0; i = nextspritesect[i])
					{
						if (sprite[i].statnum >= kStatFree) continue;
						else if (sprite[i].x == x && sprite[i].y == y && hgltCheck(type, i) < 0)
							hgltAdd(type, i), cnt++;
					}
					break;
				default:
					x = wall[idx].x; y = wall[idx].y;
					for (i = 0; i < numwalls; i++) {
						if (wall[i].x == x && wall[i].y == y && hgltCheck(type, i) < 0)
							hgltAdd(type, i), cnt++;
					}
					break;
			}
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			if (hgltCheck(type, idx) >= 0) break;
			hgltAdd(type, idx), cnt++;
			sectorDetach(idx);
			break;
	}
	
	return cnt;
	
}

short hglt2dRemove(int type, int idx) {

	short cnt = 0;
	register int i = 0; int x, y;
	switch (type)
	{
		case OBJ_SPRITE:
			x = sprite[idx].x; y = sprite[idx].y; // exclude all sprites with same xy
			for (i = highlightcnt - 1; i >= 0; i--)
			{
				if ((highlight[i] & 0xC000) == 0x4000)
				{
					idx = highlight[i] & 0x3FFF;
					if (sprite[idx].x == x && sprite[idx].y == y)
						hgltRemove(type, idx), cnt++;
				}
			}
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			x = wall[idx].x; y = wall[idx].y; // exclude all wall points with same xy
			for (i = highlightcnt - 1; i >= 0; i--)
			{
				if ((highlight[i] & 0xC000) == 0)
				{
					idx = highlight[i];
					if (wall[idx].x == x && wall[idx].y == y)
						hgltRemove(type, idx), cnt++;
				}
			}
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			if (hgltRemove(type, idx)) cnt++;
			break;
	}
	
	return cnt;
}

void hgltReset(int which) {
	
	register int i;
	if ((which & kHgltPoint) && highlightcnt > 0) {
		
		for (i = highlightcnt - 1; i >= 0; i--) {
			if ((highlight[i] & 0xC000) == 0) hgltRemove(OBJ_WALL, highlight[i]);
			else hgltRemove(OBJ_SPRITE, highlight[i] & 0x3FFF);
			
		}

		highlightcnt = -1;
		
	}
	
	if ((which & kHgltSector) && highlightsectorcnt > 0) {
		for (i = highlightsectorcnt - 1; i >= 0; i--)
			hgltRemove(OBJ_FLOOR, highlightsector[i]);
		
		highlightsectorcnt = -1;
	}
		
	if (which & kHgltGradient)
		gListGrd.Clear(); // clear out 3d mode highlights
}

void hgltSprGetEdgeSpr(short* ls, short* rs, short* ts, short* bs, short* zts, short* zbs) {
	
	register short j = -1; register int i;
	int zTop = 0, zBot = 0, zb = 0, zt = 0;
	
	for (i = 0; i < highlightcnt; i++) {
		
		if ((highlight[i] & 0xC000) == 0)
			continue;
		
		j = (short) (highlight[i] & 16383);
		if (!hgltSprIsFine(j))
			continue;

		zt  = zb  = sprite[j].z;
		*ls = *rs = *ts = *bs = *zts = *zbs = j;
		j = (short)i;
		break;
		
	}
	
	dassert(*ls != -1);
	
	for (i = j; i < highlightcnt; i++) {
		if ((highlight[i] & 0xC000) == 0)
			continue;
		
		j = (short) (highlight[i] & 16383);
		if (!hgltSprIsFine(j))
			continue;
		
		if (sprite[j].x < sprite[*ls].x) *ls = j;
		if (sprite[j].x > sprite[*rs].x) *rs = j;
		if (sprite[j].y < sprite[*bs].y) *bs = j;
		if (sprite[j].y > sprite[*ts].y) *ts = j;
		
		GetSpriteExtents(&sprite[j], &zTop, &zBot);
		
		if (zTop <= zt)
		{
			*zts = j;
			 zt = zTop;
		}
		
		if (zBot >= zb)
		{
			*zbs = j;
			 zb = zBot;
		}
	}

}

void hgltSprGetZEdgeSpr(short* lowest, short* highest) {
	
	int zTop = 0, zBot = 0;
	short ls = -1, rs = -1, bs = -1, ts = -1, zbs = -1, zts = -1;
	hgltSprGetEdgeSpr(&ls, &rs, &ts, &bs, &zts, &zbs);

	dassert(zts != -1 && zbs != -1);
	
	if (lowest) *lowest = zbs;
	if (highest) *highest = zts;

}


void hgltSprAvePoint(int* dax, int* day, int* daz) {
	
	short ls = -1, rs = -1, bs = -1, ts = -1, zbs = -1, zts = -1;
	hgltSprGetEdgeSpr(&ls, &rs, &ts, &bs, &zts, &zbs);
	
	
	*dax = sprite[ls].x + ((sprite[rs].x - sprite[ls].x) >> 1); 
	*day = sprite[ts].y + ((sprite[bs].y - sprite[ts].y) >> 1); 
	if (daz != NULL)
		*daz = sprite[zts].z + ((sprite[zbs].z - sprite[zts].z) >> 1);
	
}

void hgltSprGetEdges(int* left, int* right, int* top, int* bot, int* ztop, int* zbot, int* ztofs, int* zbofs) {
	
	int zTop = 0, zBot = 0;
	short ls = -1, rs = -1, bs = -1, ts = -1, zbs = -1, zts = -1;
	hgltSprGetEdgeSpr(&ls, &rs, &ts, &bs, &zts, &zbs);

	dassert(ls != -1);
	
	*left = sprite[ls].x; 	*right = sprite[rs].x;
	*top  = sprite[ts].y;  	*bot   = sprite[bs].y;
	*ztop = sprite[zts].z;	*zbot  = sprite[zbs].z;
	
	if (ztofs) {
		
		*ztofs = 0;
		GetSpriteExtents(&sprite[zts], &zTop, &zBot);
		if (zTop <= sector[sprite[zts].sectnum].ceilingz)
			*ztofs = sector[sprite[zts].sectnum].ceilingz - zTop;

	}
	
	if (zbofs) {
		
		*zbofs = 0;
		GetSpriteExtents(&sprite[zbs], &zTop, &zBot);
		if (zBot >= sector[sprite[zbs].sectnum].floorz)
			*zbofs = zBot - sector[sprite[zbs].sectnum].floorz;

	}

	
}

void sprGetZOffsets(short idx, int* ztofs, int* zbofs) {
	
	int left = 0, right = 0, top = 0, bottom = 0, ztop = 0, zbottom = 0, ztofs2 = 0, zbofs2 = 0;
	if (!sprInHglt(idx)) {
		
		int zTop, zBot;
		spritetype* pSprite = &sprite[idx];
		
		GetSpriteExtents(pSprite, &zTop, &zBot);
		if (zTop <= sector[pSprite->sectnum].ceilingz)
			ztofs2 = sector[pSprite->sectnum].ceilingz - zTop;
		
		if (zBot >= sector[pSprite->sectnum].floorz)
			zbofs2 = zBot - sector[pSprite->sectnum].floorz;
		
	
	} else {
		
		hgltSprGetEdges(&left, &right, &top, &bottom, &ztop, &zbottom, &ztofs2, &zbofs2);	

	}
	
	*ztofs = ztofs2;
	*zbofs = zbofs2;
	return;
	
}

// don't allow to go through floors or ceiling keeping the shape
void hgltSprClamp(int ofsAboveCeil, int ofsBelowFloor, int which) {
	

	short hg = -1, lw = -1;
	int zTop, zBot, value = 0, nz = 0, oz = 0;
	hgltSprGetZEdgeSpr(&lw, &hg);

	if (which & 0x0001) {
		
		GetSpriteExtents(&sprite[hg], &zTop, &zBot);
		if (zTop <= getceilzofslope(sprite[hg].sectnum, sprite[hg].x, sprite[hg].y))
			hgltSprPutOnCeiling(ofsAboveCeil);

	}
	
	
	if (which & 0x0002) {
		
		GetSpriteExtents(&sprite[lw], &zTop, &zBot);
		if (zBot >= getflorzofslope(sprite[lw].sectnum, sprite[lw].x, sprite[lw].y))
			hgltSprPutOnFloor(ofsBelowFloor);
		
	}
	
}

void hglt2Xlist(OBJECT_LIST* pList, char which)
{
	int i, s, e, t;
	
	if (which & kHgltSector)
	{
		for (i = 0; i < highlightsectorcnt; i++)
		{
			t = highlightsector[i];
			if (GetXSect(&sector[t]))
				pList->Add(OBJ_SECTOR, sector[t].extra);
			
			getSectorWalls(t, &s, &e);
			while(s <= e)
			{
				if (GetXWall(&wall[s]))
					pList->Add(OBJ_WALL, wall[s].extra);
				
				s++;
			}

			for (s = headspritesect[t]; s >= 0; s = nextspritesect[s])
			{
				if (GetXSpr(&sprite[s]))
					pList->Add(OBJ_SPRITE, sprite[s].extra);
			}
		}
	}
	
	if (which & kHgltPoint)
	{
		for (i = 0; i < highlightcnt; i++)
		{
			if ((highlight[i] & 0xC000) == 0x4000)
			{
				s = highlight[i] & 0x3FFF;
				if (GetXSpr(&sprite[s]))
					pList->Add(OBJ_SPRITE, sprite[s].extra);
			}
			else
			{
				s = highlight[i];
				if (GetXWall(&wall[s]))
					pList->Add(OBJ_WALL, wall[s].extra);
			}
		}
		
		// should we add sectors if all walls of sector in this highlight...?
	}
}

char hgltRelaceChannel(OBJECT_LIST* pList, int nOld, int nNew, char which)
{
	OBJECT* pObj = pList->Ptr();
	int nID, nPrev, i, t, tx[4];
	char rpc = 0;
	
	
	while(pObj->type != OBJ_NONE)
	{
		nID = pObj->index;
		switch(pObj->type)
		{
			case OBJ_SECTOR:
				if((which & 0x2) && xsector[nID].txID == nOld) xsector[nID].txID = nNew, rpc = 1;
				if((which & 0x1) && xsector[nID].rxID == nOld) xsector[nID].rxID = nNew, rpc = 1;
				break;
			case OBJ_SPRITE:
				if (which & 0x02)
				{
					if (xsprite[nID].txID == nOld)
						xsprite[nID].txID = nNew, rpc = 1;
					
					if (isMultiTx(xsprite[nID].reference))
					{
						if (multiTxGetRange(xsprite[nID].reference, tx))
						{
							for (i = tx[0]; i <= tx[1]; i++)
							{
								if (i == nOld)
								{
									// must try to set new range here
									if ((nPrev = findUnusedChannel()) >= 0)
									{
										t = tx[1]-tx[0];
										for (i = 0; i < t && !collectObjectsByChannel(++nPrev, 1, NULL, 0x0); i++);
										if (i >= t)
										{
											setDataOf(0, nPrev - t, OBJ_SPRITE, xsprite[nID].reference);	// data1
											setDataOf(3, nPrev, 	OBJ_SPRITE, xsprite[nID].reference);	// data4
											rpc = 1;
										}
										
										// !!! assign objects to a new range somehow
									}
									
									break;
								}
							}
						}
						else
						{
							for (i = 0; i < 4; i++)
							{
								if (tx[i] == nOld)
									setDataOf(i, nNew, OBJ_SPRITE, xsprite[nID].reference), rpc = 1;
							}
						}
					}
				}
				if((which & 0x1) && xsprite[nID].rxID == nOld) xsprite[nID].rxID = nNew, rpc = 1;
				break;
			case OBJ_WALL:
				if((which & 0x2) && xwall[nID].txID == nOld) xwall[nID].txID = nNew, rpc = 1;
				if((which & 0x1) && xwall[nID].rxID == nOld) xwall[nID].rxID = nNew, rpc = 1;
				break;
		}
		
		pObj++;
	}
	
	return rpc;
}

void hgltIsolateChannels(char which)
{
	unsigned char used[1024]; const int l = LENGTH(used);
	OBJECT_LIST hglt; OBJECT* pObj;
	int i, j, k = 0;

	hglt2Xlist(&hglt, which);
	collectUsedChannels(used);
	
	for (i = 100; i < l; i++)
	{
		if (k >= LENGTH(used)-100)
		{
			ThrowError("Out of free TX/RX channels!");
			break;
		}
		
		
		if (used[i] == 1) // must be originally busy channel
		{
			k++;
			for (j = 100; j < l ; j++)
			{
				if (used[j] == 0 && hgltRelaceChannel(&hglt, i, j, 0x03)) // replace TX and RX of the matching objects
				{
					if (!collectObjectsByChannel(i, 0, NULL, 0x0))
						used[i] = 0, k--; // this channel freed
					
					used[j] = 2, k++; // new used channel
					break;
				}
			}
		}
	}
}

void hgltIsolatePathMarkers(int which)
{
	spritetype *pMarkA, *pMarkB; int nOld, nA, nB;
	OBJECT_LIST hglt; OBJECT *a, *b;
	hglt2Xlist(&hglt, which);
	
	
	a = hglt.Ptr();
	while(a->type != OBJ_NONE)
	{
		nA = a->index;
		if (a->type == OBJ_SPRITE)
		{
			pMarkA = &sprite[xsprite[nA].reference];
			if (pMarkA->type == kMarkerPath)
			{
				nOld = xsprite[nA].data1;
				if ((xsprite[nA].data1 = findUnusedPath()) < 0)
					ThrowError("Out of free path ID!");
				
				// search all markers that have data1 == nOld or data2 == nOld and replace it
				
				b = hglt.Ptr();
				while(b->type != OBJ_NONE)
				{
					nB = b->index;
					if (b->type == OBJ_SPRITE)
					{
						pMarkB = &sprite[xsprite[nB].reference];
						if (pMarkB->type == kMarkerPath)
						{
							if (xsprite[nB].data1 == nOld)
								xsprite[nB].data1 = xsprite[nA].data1;
							
							if (xsprite[nB].data2 == nOld)
								xsprite[nB].data2 = xsprite[nA].data1;
						}
					}
					else if (b->type == OBJ_SECTOR)
					{
						// also replace data for path sectors if any
						sectortype* pSect = &sector[xsector[nB].reference];
						if (pSect->type == kSectorPath && xsector[nB].data == nOld)
							xsector[nB].data = xsprite[nA].data1;
					}
					
					b++;
				}
			}
		}
		
		a++;
	}
}

void hgltIsolateRorMarkers(int which)
{
	spritetype *pMarkA, *pMarkB; int nOld, nA, nB;
	OBJECT_LIST hglt; OBJECT *a, *b;
	hglt2Xlist(&hglt, which);
	
	a = hglt.Ptr();
	while(a->type != OBJ_NONE)
	{
		nA = a->index;
		if (a->type == OBJ_SPRITE)
		{
			pMarkA = &sprite[xsprite[nA].reference];
			if (irngok(pMarkA->type, kMarkerLowLink, kMarkerLowGoo) && pMarkA->type != kMarkerWarpDest)
			{
				nOld = xsprite[nA].data1;
				if ((xsprite[nA].data1 = findUnusedStack()) < 0)
					ThrowError("Out of free ROR ID!");
				
				// search all markers that have data1 == nOld and replace it
				
				b = hglt.Ptr();
				while(b->type != OBJ_NONE)
				{
					nB = b->index;
					if (b->type == OBJ_SPRITE)
					{
						pMarkB = &sprite[xsprite[nB].reference];
						if (irngok(pMarkB->type, kMarkerLowLink, kMarkerLowGoo) && pMarkB->type != kMarkerWarpDest)
						{
							if (xsprite[nB].data1 == nOld)
								xsprite[nB].data1 = xsprite[nA].data1;
						}
					}
					
					b++;
				}
			}
		}
		
		a++;
	}
}

void hgltSprSetXYZ(int x, int y, int z) {
	
	int dax, day, daz;
	hgltSprAvePoint(&dax, &day, &daz);
	hgltSprChgXYZ(x - dax, y - day, z - daz);

}

void hgltSprChgXYZ(int xstep, int ystep, int zstep) {
	
	register int i, j;
	for (i = 0; i < highlightcnt; i++) {
		if ((highlight[i] & 0xC000) == 0)
			continue;

		j = highlight[i] & 16383;
		if (sprite[j].statnum >= kStatFree)
			continue;

		if (xstep) sprite[j].x += xstep;
		if (ystep) sprite[j].y += ystep;
		if (zstep) sprite[j].z += zstep;
	}

	hgltSprCallFunc(sprFixSector);
	
}

// put sprites on floor keeping the shape
void hgltSprPutOnFloor(int ofs) {
	
	short lowest = -1;
	hgltSprGetZEdgeSpr(&lowest, NULL);
	if (lowest < 0)
		return;
		
	int oz = sprite[lowest].z;
	PutSpriteOnFloor(&sprite[lowest], 0);
	int nz = sprite[lowest].z;
	sprite[lowest].z = oz;

	hgltSprChgXYZ(0, 0,(nz - oz) + ofs);

}


// put sprites on ceiling keeping the shape
void hgltSprPutOnCeiling(int ofs) {
	
	short highest = -1;
	hgltSprGetZEdgeSpr(NULL, &highest);
	if (highest < 0)
		return;
		
	int oz = sprite[highest].z;
	PutSpriteOnCeiling(&sprite[highest], 0);
	int nz = sprite[highest].z;
	sprite[highest].z = oz;
	
	hgltSprChgXYZ(0, 0, (nz - oz) - ofs);

}

void hgltSprPutOnZ(int z, int which, int tofs, int bofs) {
	
	short hg = -1, lw = -1;
	int zTop, zBot, value = 0, nz = 0, oz = 0;
	hgltSprGetZEdgeSpr(&lw, &hg);
	
	if (hg >= 0 && (which & 0x0001)) {
		
		oz = sprite[hg].z;
		GetSpriteExtents(&sprite[hg], &zTop, &zBot);
		
		sprite[hg].z += z - zTop; nz = sprite[hg].z;
		sprite[hg].z = oz;

		hgltSprChgXYZ(0, 0, (nz - oz) - tofs);
	}
	
	if (lw >= 0 && (which & 0x0002)) {
		
		oz = sprite[lw].z;
		GetSpriteExtents(&sprite[lw], &zTop, &zBot);
		
		sprite[lw].z += z - zBot; nz = sprite[lw].z;
		sprite[lw].z = oz;

		hgltSprChgXYZ(0, 0, (nz - oz) + bofs);
	}

}

void hgltSprPutOnWall(int nwall, int x, int y) {

	short ls = -1, rs = -1, bs = -1, ts = -1, zbs = -1, zts = -1;
	hgltSprGetEdgeSpr(&ls, &rs, &ts, &bs, &zts, &zbs);

	int tang = getangle(sprite[ts].x - sprite[bs].x, sprite[ts].y - sprite[bs].y);
	int wang = GetWallAngle(nwall);
	
	int width  = abs(sprite[rs].x - sprite[ls].x);
	int height = abs(sprite[ts].y - sprite[bs].y);
	if (!width || !height)
		return;

	int dx = sprite[ls].x - sprite[rs].x;
	int dy = sprite[ts].y - sprite[bs].y;

	RotateVector(&dx, &dy, wang);

	int edge = sprite[ts].y;
	int oct = GetOctant(dx, dy);

	switch (oct) {
		case 0:
		case 1:
			edge = sprite[ls].x;
			//angw = abs(angw);
			//angh = abs(angh);
			break;
		case 2:
		case 3:
			edge = sprite[bs].y;
			break;
		case 4:
		case 5:
			edge = sprite[rs].x;
			break;

	}

	//scrSetMessage("wall %d:   %d / %d / %d / %d / %d", nwall, oct, wang, tang, x - dx, y - dy);

	if (wallHoriz(nwall)) hgltSprChgXYZ(x - edge, 0);
	else if (wallVert(nwall)) hgltSprChgXYZ(0, y - edge);
	// don't know how to handle angled walls...:(((
	//else hgltSprSetXYZ(x - ((sprite[rs].x - sprite[ls].x) >> 1), y - ((sprite[bs].y - sprite[ts].y) >> 1), z);
		//hgltSprChgXYZ(dx, dy);
	
}

void hgltSprRotate(int step) {

	register int i, j;
	int dax = 0, day = 0;
	
	
	hgltSprAvePoint(&dax, &day);
	
	
	for (i = 0; i < highlightcnt; i++) {
		
		if ((highlight[i] & 0xC000) == 0)
			continue;
		
		j = highlight[i] & 16383;
		if (sprite[j].statnum < kStatFree) {

			RotatePoint(&sprite[j].x, &sprite[j].y, step, dax, day);

			int nType = sprite[j].type; // check if allowed to change markers angle...
			if ((nType >= 3 && nType <= 5) && !(gRotateOpts.chgMarkerAng & 0x0002)) continue;
			else if ((nType == 15) && !(gRotateOpts.chgMarkerAng & 0x0001)) continue;
			
			sprite[j].ang = (short)(sprite[j].ang + step);
			if (!isMarkerSprite(j))
				sprite[j].ang = sprite[j].ang & kAngMask;
			
			sprFixSector(&sprite[j], 0);
			
		}
		
	}
	
}

int hgltWallCount(int* whicnt, int* redcnt)
{
	int i, j, total = 0;
	if (whicnt) *whicnt = 0;
	if (redcnt) *redcnt = 0;
	for (i = 0; i < highlightcnt; i++)
	{
		if ((highlight[i] & 0xC000) != 0)
			continue;

		total++;
		j = highlight[i];
		if (wall[j].nextwall >= 0 && redcnt != NULL) *redcnt = *redcnt + 1; 
		else if (whicnt != NULL)
			*whicnt = *whicnt + 1;
	}
	
	return total;
}

int hgltWallsCheckStat(int nStat, int which, int nMask)
{
	int i, nwall, j = 0, red, redcnt = 0, whicnt = 0, cstat;
	if (!hgltWallCount(&whicnt, &redcnt))
		return 0;

	for (i = 0; i < highlightcnt; i++)
	{
		if ((highlight[i] & 0xC000) != 0)
			continue;

		nwall = highlight[i];
		red = (wall[nwall].nextwall >= 0);
		cstat = wall[nwall].cstat;
		
		if (((which & 0x01) && !red) || ((which & 0x02) && red))
		{
			if (which & 0x04)
			{
				if ((nMask && (cstat & nMask) != nStat) || (!nMask && !(cstat & nStat)))
					j++;
			}
			else if ((nMask && (cstat & nMask) == nStat) || (!nMask && (cstat & nStat)))
				j++;
		}
	}
	
	if ((which & 0x03) == 0x03) return (j + redcnt + whicnt) - (redcnt + whicnt); 
	else if (which & 0x01) return (j + whicnt) - whicnt;
	else if (which & 0x02) return (j + redcnt) - redcnt;
	return 0;
}

void hgltSectGetBox(int* x1, int* y1, int* x2, int *y2)
{
	int nFirst = sector[highlightsector[0]].wallptr;
	int i = highlightsectorcnt;
	int tx1, ty1, tx2, ty2;
	int s, e;
	
	*x1 = *x2 = wall[nFirst].x;
	*y1 = *y2 = wall[nFirst].y;
	
	while(--i >= 0)
	{
		getSectorWalls(highlightsector[i], &s, &e);
		loopGetBox(s, e, &tx1, &ty1, &tx2, &ty2);
		if (tx1 < *x1) *x1 = tx1;
		if (tx2 > *x2) *x2 = tx2;
		
		if (ty1 < *y1) *y1 = ty1;
		if (ty2 > *y2) *y2 = ty2;
	}
}

void hgltSectMidPoint(int* ax, int* ay)
{
	int x1, y1, x2, y2;
	hgltSectGetBox(&x1, &y1, &x2, &y2);
	
	*ax = x1 + ((x2 - x1) >> 1);
	*ay = y1 + ((y2 - y1) >> 1);
}

char hgltSectInsideBox(int x, int y)
{
	if (highlightsectorcnt < 0)
		return 0;
	
	int x1, y1, x2, y2;
	hgltSectGetBox(&x1, &y1, &x2, &y2);
	return (irngok(x, x1, x2) && irngok(y, y1, y2));
}

int hgltSectFlip(int flags)
{
	int cx, cy;
	hgltSectMidPoint(&cx, &cy);

	if (flags & 0x04)
	{
		IDLIST done(true);
		int nSect = 0;
		int s, e;
		
		// first flip outer loops of whole highlight
		while(hgltListOuterLoops(&nSect, &s, &e, &done, kHgltSector))
			loopFlip(s, e, cx, cy, flags);
	}
	

	// flip sectors inside highlight
	return hgltSectCallFunc(sectFlip, cx, cy, flags);
}

int hgltSectRotate(int flags, int nAng)
{
	int cx, cy;
	hgltSectMidPoint(&cx, &cy);

	if (flags & 0x04)
	{
		IDLIST done(true);
		int nSect = 0;
		int s, e;
		
		// first rotate outer loops of whole highlight
		while(hgltListOuterLoops(&nSect, &s, &e, &done, kHgltSector))
			loopRotate(s, e, cx, cy, nAng, flags);
	}
	
	// rotate sectors inside highlight
	return hgltSectCallFunc(sectRotate, cx, cy, nAng, flags);
}

char hgltListOuterLoops(int* nStart, int* s, int* e, IDLIST* pDone, char which)
{
	int i, j, k, ts, te;
	int nNext, nSect;

	for (i = *nStart; i < highlightsectorcnt; i++)
	{
		nSect = highlightsector[i];
		getSectorWalls(nSect, &ts, &te);
		
		for (j = ts; j <= te; j++)
		{
			// attempt to find outer loop pointer here...
			////////////////////////////////////////////////
			
			if (wall[j].nextwall >= 0)					continue;	// must be solid because sectors detached
			else if (pDone->Exists(j))					continue;	// visited
			else if ((nNext = findNextWall(j)) < 0)		continue;	// not detached
			else if ((nSect = sectorofwall(nNext)) < 0)	continue;	// corrupted wall?
			else if (hgltCheck(OBJ_SECTOR, nSect) >= 0)	continue;	// inside highlight
			
			loopGetWalls(nNext, s, e); // got some outer loop
			
			// must have no connections outside highlight
			// so all nextwalls must be part of sectors
			// which is inside
			
			for (k = *s; k <= *e; k++)
			{
				if ((nNext = findNextWall(k)) < 0)			break;	// not detached
				else if ((nSect = sectorofwall(nNext)) < 0)	break;	// corrupted wall?
				else if (hgltCheck(OBJ_SECTOR, nSect) < 0)	break;	// outside highlight
			}
			
			if (pDone)
			{
				for (j = *s; j <= *e; j++)
					pDone->Add(j); // mark as done to meet once
			}
			
			if (k > *e)
			{
				*nStart	= i+1; // looks like a correct loop
				return 1;
			}
			
			break;
		}
	}
	
	*nStart = 0;
	return 0;
}

char ED32_insideExclude(int x, int y, int nSect, OBJECT_LIST* pExclude)
{
    return (rngok(nSect, 0, kMaxSectors) && !pExclude->Exists(OBJ_SECTOR, nSect) && inside(x, y, nSect));
}

void ED32_updateSectorExclude(int x, int y, int* nSect, OBJECT_LIST* pExclude)
{
    int nNext, nHigh, nLow;
	int s, e;
	
	if (!ED32_insideExclude(x, y, *nSect, pExclude))
	{
		if (rngok(*nSect, 0, numsectors))
		{
			getSectorWalls(*nSect, &s, &e);
			
			while(s <= e)
			{
				nNext = wall[s].nextsector;
				if (ED32_insideExclude(x, y, nNext, pExclude))
				{
					*nSect = nNext;
					return;
				}
				
				s++;
			}
		}
		
		s = (*nSect < 0) ? (numsectors >> 1) : *nSect;
		nHigh = nLow = s;
		s = 0;

		while(s < numsectors)
		{
			if (nHigh < numsectors - 1 && ED32_insideExclude(x, y, ++nHigh, pExclude))
			{
				*nSect = nHigh;
				return;
			}
			else if (nLow > 0 && ED32_insideExclude(x, y, --nLow, pExclude))
			{
				*nSect = nLow;
				return;
			}
			
			s++;
		}
		
		*nSect = -1;
	}
}



// returns:
//  0: continue
// >0: newnumwalls
// <0: error
// ignore_ret and refsect_ret are for the 'auto-red-wall' feature

int ED32_traceLoop(int j, OBJECT_LIST* pExclude, char *ignore_ret, int *refsect_ret)
{
    int refsect, ignore;
    int k, n, refwall;
	
    if (wall[j].nextwall>=0 || pExclude->Exists(OBJ_WALL, j))
        return 0;

    n=2*kMaxWalls;  // simple inf loop check
    refwall = j;
    k = numwalls;

    ignore = 0;

    if (ignore_ret)
    {
        refsect = -1;
		ED32_updateSectorExclude(wall[j].x, wall[j].y, &refsect, pExclude);
        if (refsect<0)
            return -1;
    }

    do
    {
        if (j!=refwall && pExclude->Exists(OBJ_WALL, j))
            ignore = 1;
        
		pExclude->Add(OBJ_WALL, j);

        if (ignore_ret)
        {
            if (inside(wall[j].x, wall[j].y, refsect) != 1)
				ignore = 1;
        }

        if (!ignore)
        {
            if (k>=kMaxWalls)
                return -2;
			
          //if (ignore_ret)  // auto-red wall feature
              // gNextWall[k] = gNextWall[j];
				
            memcpy(&wall[k], &wall[j], sizeof(walltype));
            wall[k].point2 = k+1;
// TODO: protect lotag/extra; see also hl-sector copying stuff
            wall[k].nextsector = wall[k].nextwall = wall[k].extra = -1;
            k++;
        }

        j = wall[j].point2;
        n--;

        while (wall[j].nextwall>=0 && n>0)
        {
            j = wall[wall[j].nextwall].point2;
            n--;
        }
    }
    while (j!=refwall && n>0);

    if (j!=refwall)
        return -3;

    if (ignore_ret)
    {
        *ignore_ret = ignore;
        if (refsect_ret)
            *refsect_ret = refsect;
    }

    return k;
}

int ED32_hgltSectAutoRedWall()
{
	int nWall, nNewWalls = -1, nMove, nNext, nParentNew, nParentOld;
	int nSect, nRef = -1, i, j, s, e, n, t = 0, c = 0;
	sectortype* pRef; walltype *pTmpWall, *pWalls;
	char ignore, zOffsetsDone = 0;
	OBJECT_LIST skipObjects;
		
	// !!! have no idea why gNextWall gets wrong indexes, so let's just save a
	// whole thing and restore with proper order after finishing inner loop
	// creation process.
	pWalls = (walltype*)malloc(numwalls*sizeof(walltype));
	dassert(pWalls != NULL);
	
	for (i = 0; i < highlightsectorcnt; i++)
	{
		nSect = highlightsector[i];
		getSectorWalls(nSect, &s, &e);

		while(s <= e)
		{
			if (wall[s].nextwall >= 0)
			{
				// walls are red, so they are inside highlight?
				skipObjects.Add(OBJ_WALL, wall[s].nextwall);
				skipObjects.Add(OBJ_WALL, s);
			}
			else
			{
				if ((nNext = gNextWall[s]) >= 0 && nNext < numwalls)
				{
					pWalls[t] = wall[nNext];						// use existing old next wall	
					pWalls[t].nextsector = sectorofwall(nNext);		// for easy parent sector search
				}
				else
				{
					pWalls[t] = wall[s];							// well, use internal wall as next wall
					pWalls[t].nextsector = nSect;					// for easy parent sector search
				}
				
				t++;
			}
			
			s++;
		}
		
		// mark as visited sector inside
		skipObjects.Add(OBJ_SECTOR, nSect);
	}
	
	for (i = 0; i < highlightsectorcnt && nNewWalls >= -1; i++)
	{
		nSect = highlightsector[i];
		getSectorWalls(nSect, &s, &e);

		for (j = s; j <= e; j++)
		{
			nNewWalls = ED32_traceLoop(j, &skipObjects, &ignore, &nRef);
			if (nNewWalls == -1 || nNewWalls == 0 || ignore) continue;
			else if (nNewWalls < 0)
				break;
			
			// done tracing one outer loop, finish it
			wall[nNewWalls-1].point2 = numwalls;
			if (clockdir(numwalls) != 0)
				continue;
		
			pRef = &sector[nRef];
			nMove = pRef->wallptr + pRef->wallnum;
			n = (nNewWalls - numwalls); // number of walls in just constructed loop
			
			flipWalls(numwalls, nNewWalls);
			
			pRef->wallnum += n;
			if (nRef != numsectors - 1)
			{
				pTmpWall = (walltype*)malloc(n * sizeof(wall[0]));
				dassert(pTmpWall != NULL);

				for (t = 0; t < numwalls; t++)
				{
					if (wall[t].nextwall >= nMove)
						wall[t].nextwall += n;
				}

				for (t = nRef + 1; t < numsectors; t++)	sector[t].wallptr += n;
				for (t = nMove; t < numwalls; t++)		wall[t].point2 += n;
				for (t = numwalls; t < nNewWalls; t++)	wall[t].point2 += (nMove-numwalls);
				
				memcpy(pTmpWall, &wall[numwalls],						n * sizeof(walltype));
				memmove(&wall[nMove + n], &wall[nMove],					(numwalls-nMove) * sizeof(walltype));
				memcpy(&wall[nMove], pTmpWall,							n * sizeof(walltype));
				
				free(pTmpWall);
			}
			
			numwalls = nNewWalls, c++;
		}
	}
	
	if (c)
	{
		// copy properties from old nextwalls with a
		// proper order here
		//////////////////////////////////////////////
		
		for (i = 0, t = 0; i < highlightsectorcnt; i++)
		{
			nSect = highlightsector[i];
			getSectorWalls(nSect, &s, &e);
			
			while(s <= e)
			{
				walltype* pWall = &wall[s];
				
				gNextWall[s] = pWall->nextwall;
				if (pWall->nextwall < 0 && (nNext = findNextWall(s)) >= 0)
				{
					gNextWall[s] = nNext;
					gNextWall[nNext] = s;
					
					walltype* pDst = &wall[nNext];
					walltype* pSrc = &pWalls[t++];

					pDst->picnum		= pSrc->picnum;
					pDst->overpicnum 	= pSrc->overpicnum;
					pDst->pal			= pSrc->pal;
					pDst->shade			= pSrc->shade;
					pDst->cstat			= pSrc->cstat;
					
					pDst->xpanning		= pSrc->xpanning;
					pDst->ypanning		= pSrc->ypanning;
					pDst->xrepeat		= pSrc->xrepeat;
					pDst->yrepeat		= pSrc->yrepeat;
					
					pDst->type			= pSrc->type;
					pDst->extra			= pSrc->extra;
					pDst->hitag			= pSrc->hitag;
					
					nParentNew			= sectorofwall(nNext);
					nParentOld			= pSrc->nextsector;
					
					pWall->nextsector	= nParentNew;
					pWall->nextwall		= nNext;
					
					pDst->nextsector	= nSect;
					pDst->nextwall		= s;
					
					if (!zOffsetsDone && nParentOld != nSect && nParentOld != nParentNew)
					{
						hgltSectCallFunc(sectSetupZOffset, nParentOld, nParentNew, 0x03);
						zOffsetsDone = 1;
					}
					
					for (j = headspritesect[nParentNew]; j >= 0; j = nextspritesect[j])
					{
						spritetype* pSpr = &sprite[j];
						if (inside(pSpr->x, pSpr->y, nSect))
						{
							ChangeSpriteSect(pSpr->index, nSect);
							clampSprite(pSpr);
						}
					}
				}
				
				s++;
			}
		}
	}
		
	free(pWalls);
	
	if (c)
		return c;
	
	return nNewWalls;
}


int hgltSectAutoRedWall()
{
	return ED32_hgltSectAutoRedWall();
}

unsigned char fixupPanCountShift(int nPicSiz)
{
	int nSiz = 16;
	int i = 0;
	
	while(i < 32)
	{
		if (nSiz == nPicSiz)
			return i;
		
		nSiz<<=1;
		i++;
	}
	
	return 0;
}

// !!! FIXME:
// Unfortunately, this code stops working when:
// picnum size is not power of two
// picnum size is less than 16 pixels
// dist step is less than 8 pixels
void fixupPan(int nSect, int nStat, int tx, int ty)
{
	sectortype* pSect = &sector[nSect];
	int nCstat = sectCstatGet(nSect, nStat);
	int nPic, x, y, t;
	
	char swap = ((nCstat & kSectSwapXY) > 0);
	char expd = ((nCstat & kSectExpand) > 0);
	unsigned char *pPanX, *pPanY;
		
	if (nStat == OBJ_FLOOR)
	{
		nPic  = pSect->floorpicnum;
		pPanX = &pSect->floorxpanning;
		pPanY = &pSect->floorypanning;
	}
	else
	{
		nPic  = pSect->ceilingpicnum;
		pPanX = &pSect->ceilingxpanning;
		pPanY = &pSect->ceilingypanning;
	}

	if (swap)
		swapValues(&tx, &ty);
	
	if (tx)
	{
		x = fixupPanCountShift(tilesizx[nPic])-expd;
		t = tx>>x;
		if (nCstat & kSectFlipX)
			t = revertValue(t);
		
		*pPanX += (swap) ? t : -t;
	}
	
	if (ty)
	{
		y = fixupPanCountShift(tilesizy[nPic])-expd;
		t = ty>>y;
		if (nCstat & kSectFlipY)
			t = revertValue(t);
		
		*pPanY += (swap) ? -t : t;
	}
}

void sectChgXY(int nSector, int bx, int by, int flags, int)
{
	int s, e;
	sectortype* pSect = &sector[nSector];
	getSectorWalls(nSector, &s, &e);
	
	if ((flags & 0x01))
	{
		if (!(pSect->floorstat & kSectRelAlign) && !(pSect->floorstat & kSectParallax))
			fixupPan(nSector, OBJ_FLOOR, bx, by);
		
		if (!(pSect->ceilingstat & kSectRelAlign) && !(pSect->ceilingstat & kSectParallax))
			fixupPan(nSector, OBJ_CEILING, bx, by);
	}
	
	while(s <= e)
	{
		wall[s].x += bx;
		wall[s].y += by;
		s++;
	}
	
	for (s = headspritesect[nSector]; s >= 0; s = nextspritesect[s])
	{
		sprite[s].x += bx;
		sprite[s].y += by;
	}
}

BOOL hgltShowStatus(int x, int y) {
	
	QFONT* pFont = qFonts[5];
	int xstep = pFont->width;
	int nLen = gListGrd.Length();
	
	
	if (nLen > 0 || highlightcnt > 0 || highlightsectorcnt > 0)
	{
		sprintf(buffer, "HIGHLIGHT:");
		gfxPrinTextShadow(x, y, gStdColor[kColorYellow], buffer, pFont);
		x += gfxGetTextLen(buffer, pFont) + xstep;
		
		if (nLen > 0)
		{
			sprintf(buffer, "GRADIENT=%d", nLen);
			gfxPrinTextShadow(x, y, gStdColor[kColorLightMagenta], buffer, pFont);
			x += gfxGetTextLen(buffer, pFont) + xstep;
		}
		
		if (highlightcnt > 0)
		{
			int sprites = hgltSprCount();
			if (sprites)
			{
				sprintf(buffer, "SPRITES=%d", sprites);
				gfxPrinTextShadow(x, y, gStdColor[kColorLightCyan], buffer, pFont);
				x += gfxGetTextLen(buffer, pFont) + xstep;
			}

			int walls = highlightcnt - sprites;
			if (walls > 0)
			{
				sprintf(buffer, "WALLS=%d", walls);
				gfxPrinTextShadow(x, y, gStdColor[kColorLightRed], buffer, pFont);
				x += gfxGetTextLen(buffer, pFont) + xstep;
			}
		}
		
		if (highlightsectorcnt > 0)
		{
			sprintf(buffer, "SECTORS=%d", highlightsectorcnt);
			gfxPrinTextShadow(x, y, gStdColor[kColorLightGreen], buffer, pFont);
			x += gfxGetTextLen(buffer, pFont) + xstep;
		}
		
		return TRUE;
	}
	
	return FALSE;
	
}

void sprPalSet(spritetype* pSprite, int value) {
	
	pSprite->pal = (char)ClipRange(value, 0, 255);
	
}

void sprShadeSet(spritetype* pSprite, int value) {
	
	pSprite->shade = (schar)ClipRange(value, -127, 64 - 1);
	
}

void sprShadeIterate(spritetype* pSprite, int step) {
	
	sprShadeSet(pSprite, pSprite->shade + step);
	
}

void sprClone(spritetype* pSprite, int) {
	
	int i;
	if ((i = InsertSprite(pSprite->sectnum, pSprite->statnum)) >= 0)
	{
		short sect = sprite[i].sectnum;
		memcpy(&sprite[i], pSprite, sizeof(spritetype));
		sprite[i].sectnum = sect;
		sprite[i].index = (short)i;
		
		ChangeSpriteSect((short)i, sect);
		
		if (sprInHglt(pSprite->index))
		{
			hgltRemove(OBJ_SPRITE, pSprite->index);
			hgltAdd(OBJ_SPRITE, sprite[i].index);
		}
	}
}

void sprDelete(spritetype* pSprite, int) {
	
	hgltRemove(OBJ_SPRITE, pSprite->index);
	if (pSprite->statnum < kMaxStatus)
		DeleteSprite(pSprite->index);

}

void sprSetXRepeat(spritetype* pSprite, int val) {
	
	pSprite->xrepeat += val;
	
}

void sprSetYRepeat(spritetype* pSprite, int val) {
	
	pSprite->yrepeat += val;
	
}

/* void sectFXChgFreq(int nSect, int nVal)
{
	int nXSect = sector[nSect].extra;
	if (nXSect > 0)
		xsector[nXSect].freq = ClipRange(xsector[nXSect].freq + nVal, 0, 255);
}

void sectFXChgPhase(int nSect, int nVal)
{
	int nXSect = GetXSector(nSect);
	xsector[nXSect].phase = ClipRange(xsector[nXSect].phase + nVal, 0, 255);
}

void sectFXChgAmplitude(int nSect, int nVal)
{
	int nXSect = GetXSector(nSect);
	xsector[nXSect].amplitude = ClipRange(xsector[nXSect].amplitude + nVal, -128, 127);
} */

void sectChgVisibility(int nSect, int nVis)
{
	sector[nSect].visibility = ClipRange(sector[nSect].visibility + nVis, 0, 239);
}

void sectChgShade(int nSect, int nOf, int nShade, int, int)
{
	if (nOf == OBJ_CEILING)
		sector[nSect].ceilingshade = ClipRange(sector[nSect].ceilingshade + nShade, -128, 63);
	else
		sector[nSect].floorshade   = ClipRange(sector[nSect].floorshade   + nShade, -128, 63);
}

void sectDelete(int nSector, int arg1, int arg2, int arg3, int arg4) {
	
	int i, j; BOOL found = 0;
	if (highlightsectorcnt > 0)
	{
		for (j = 0; j < highlightsectorcnt; j++)
		{
			if (highlightsector[j] == nSector)
			{
				deletesector(nSector);
				for(i = highlightsectorcnt - 1; i >= 0; i--)
				{
					if (highlightsector[i] >= nSector)
						highlightsector[i]--;
				}
				found = 1;
				break;
			}
		}
		
		if (!found)
		{
			for (j = 0; j < highlightsectorcnt; j++) sectorAttach(highlightsector[j]);
			deletesector(nSector);
		}
		
		highlightsectorcnt = -1;
	}
	else
	{
		deletesector(nSector);
	}
	
}

void sectSetupZOffset(int nSect, int nParentOld, int nParentNew, int flags, int)
{
	sectortype* pSect	 = &sector[nSect];
	sectortype* pOParent = &sector[nParentOld];
	sectortype* pNParent = &sector[nParentNew];
	XSECTOR* pXSect		 = GetXSect(pSect);
	
	int nFOffs = 0, nCOffs = 0;
	int z1, z2, z3, i;
	
	if (flags & 0x01)
	{
		z1  = pOParent->floorz;
		z2  = pNParent->floorz;
		z3  = pSect->floorz;
		
		nFOffs = (z3-z2) + (z1-z3);
		if (z2 < 0)
		{
			nFOffs = (z2 > z1) ? klabs(nFOffs) : -nFOffs;
		}
		else
		{
			nFOffs = (z2 < z1) ? -nFOffs : klabs(nFOffs);
		}

		if (pXSect)
		{
			pXSect->offFloorZ += nFOffs;
			pXSect->onFloorZ += nFOffs;
		}
		
		pSect->floorz += nFOffs;
		if (pSect->floorz < pNParent->ceilingz)
			pSect->ceilingz -= (pNParent->ceilingz - pSect->floorz);
		
		// offset relative to floor
		for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
		{
			spritetype* pSpr = &sprite[i];
			pSpr->z += nFOffs;
		}
	
	}
	else if (flags & 0x04)
	{
		pSect->floorz			= pNParent->floorz;
		pSect->floorslope		= pNParent->floorslope;
		pSect->floorpicnum		= pNParent->floorpicnum;
		pSect->floorshade		= pNParent->floorshade;
		pSect->floorstat		= pNParent->floorstat;
		pSect->floorpal			= pNParent->floorpal;
		
		pSect->floorxpanning	= pNParent->floorxpanning;
		pSect->floorypanning	= pNParent->floorypanning;
	}
	
	if (flags & 0x02)
	{
		z1 = pOParent->ceilingz;
		z2 = pNParent->ceilingz;
		z3 = pSect->ceilingz;
		
		nCOffs = (z3-z2) + (z1-z3);
		if (z2 < 0)
		{
			nCOffs = (z2 > z1) ? klabs(nCOffs) : -nCOffs;
		}
		else
		{
			nCOffs = (z2 < z1) ? -nCOffs : klabs(nCOffs);
		}
	
		if (pXSect)
		{
			pXSect->offCeilZ += nCOffs;
			pXSect->onCeilZ += nCOffs;
		}
	
		pSect->ceilingz += nCOffs;
		if (pSect->ceilingz > pNParent->floorz)
			pSect->floorz -= (pNParent->floorz - pSect->ceilingz);
	}
	else if (flags & 0x08)
	{
		pSect->ceilingz			= pNParent->ceilingz;
		pSect->ceilingslope		= pNParent->ceilingslope;
		pSect->ceilingpicnum	= pNParent->ceilingpicnum;
		pSect->ceilingshade		= pNParent->ceilingshade;
		pSect->ceilingstat		= pNParent->ceilingstat;
		pSect->ceilingpal		= pNParent->ceilingpal;
		
		pSect->ceilingxpanning	= pNParent->ceilingxpanning;
		pSect->ceilingypanning	= pNParent->ceilingypanning;
	}
}
