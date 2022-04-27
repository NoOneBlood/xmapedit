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
#include "trig.h"
#include "screen.h"
#include "edit3d.h"
#include "hglt.h"
#include "gui.h"
#include "grdshd.h"
#include "screen.h"
#include "edit2d.h"
#include "xmpconf.h"
#include "xmpmisc.h"
#include "misc.h"

int hgltx1 = 0, hgltx2 = 0, hglty1 = 0, hglty2 = 0;


/* CHECKBOX_LIST gIsolateType[] = {
	
	{TRUE, "Isolate RX/TX trigger channels."},
	{TRUE, "Isolate Room Over Room markers."},
	{TRUE, "Isolate Path markers and sectors."},
	
}; */

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
				else redRestore(idx);
				while (i < highlightsectorcnt - 1) {
					highlightsector[i] = highlightsector[i + 1];
					i++;
				}
				if (--highlightsectorcnt <= 0) highlightsectorcnt = -1;
				return TRUE;
				
			}
			break;
		default:
			ThrowError("Unknown object type %d.", type);
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
			retn = highlightsectorcnt = (short)ClipLow(highlightsectorcnt, 0);
			highlightsector[highlightsectorcnt++] = (short)idx;
			break;
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
				//whiteOut(i);
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
			whiteOut(idx);
			break;
	}
	
	return cnt;
	
}

short hglt2dRemove(int type, int idx) {

	short cnt = 0;
	register int i = 0; int x, y;
	switch (type) {
		case OBJ_SPRITE:
		case OBJ_WALL:
		case OBJ_MASKED:
			switch (type) {
				case OBJ_SPRITE:
					x = sprite[idx].x; y = sprite[idx].y; // exclude all sprites with same xy
					for (i = highlightcnt - 1; i >= 0; i--) {
						if ((highlight[i] & 0xC000) == 0x4000) {
							idx = highlight[i] & 0x3FFF;
							if (sprite[idx].x == x && sprite[idx].y == y)
								hgltRemove(type, idx), cnt++;
						}
					}
					break;
				default:
					x = wall[idx].x; y = wall[idx].y; // exclude all wall points with same xy
					for (i = highlightcnt - 1; i >= 0; i--) {
						if ((highlight[i] & 0xC000) == 0) {
							idx = highlight[i];
							if (wall[idx].x == x && wall[idx].y == y)
								hgltRemove(type, idx), cnt++;
						}
					}
					break;
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
	
	
	if (which & kHgltGradient) {
		
		gHgltc = 0; // clear out 3d mode highlights
		for (i = 0; i < kMaxHgltObjects; i++) {
			gHglt[i].type = -1;
			gHglt[i].idx  = -1; 
		}
		
	}
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

void hgltIsolateChannels(int htype, int var1) {
	
	int nIdx, nIdx2, nIdx3;
	int oldtx = 0, oldrx = 0, newtx = 0, newrx = 0;
	
	register int i, k, j, z;
	if ((htype & kHgltSector) && highlightsectorcnt > 0) {
		
		
		if (var1 & kChlCheckOutsideR) {
			

			// check if could RECEIVE from outside of the highlight
			// -----------------------------------
			for (i = 0; i < highlightsectorcnt; i++) {
				nIdx = highlightsector[i];

				// sector
				if (sector[nIdx].extra > 0) {
					if ((newrx = findUnusedChannel()) == -1) { ThrowError("Out of free TX/RX channels!"); }
					else if ((oldrx = xsector[sector[nIdx].extra].rxID) >= 100 && chlOutHighlight(oldrx, 3, kHgltSector))
						generateChannelSequence(oldrx, newrx, kHgltSector, 3);
				}

				// all walls of sector
				for (k = sector[nIdx].wallptr; k < sector[nIdx].wallptr + sector[nIdx].wallnum; k++) {
					if (wall[k].extra <= 0 || (oldrx = xwall[wall[k].extra].rxID) < 100) continue;
					else if (!chlOutHighlight(oldrx, 3, kHgltSector)) continue;
					else if ((newrx = findUnusedChannel()) == -1) { ThrowError("Out of free TX/RX channels!"); }
					else generateChannelSequence(oldrx, newrx, kHgltSector, 3);	
				}
				
				// all sprites of sector
				for (k = headspritesect[nIdx]; k != -1; k = nextspritesect[k]) {
					if (sprite[k].extra <= 0 || (oldrx = xsprite[sprite[k].extra].rxID) < 100) continue;
					else if (!chlOutHighlight(oldrx, 3, kHgltSector)) continue;
					else if ((newrx = findUnusedChannel()) == -1) { ThrowError("Out of free TX/RX channels!"); }
					else generateChannelSequence(oldrx, newrx, kHgltSector, 3);
				}
			}
		}

		if (var1 & kChlCheckInsideRS) {
			
			// set new channels in the highlight area
			// -----------------------------------
			
			// from the sectors perspective...
			for (i = 0; i < highlightsectorcnt; i++) {
				nIdx = highlightsector[i];
				if (sector[nIdx].extra <= 0 || (oldtx = xsector[sector[nIdx].extra].txID) < 100) continue;
				else if ((newtx = findUnusedChannel()) == -1)
					ThrowError("Out of free TX/RX channels!");
				
				generateChannelSequence(oldtx, newtx, kHgltSector, 3);

			}

			// from the walls perspective...
			for (i = 0; i < highlightsectorcnt; i++) {
				
				nIdx = highlightsector[i];
				for (k = sector[nIdx].wallptr; k < sector[nIdx].wallptr + sector[nIdx].wallnum; k++) {
					if (wall[k].extra <= 0 || (oldtx = xwall[wall[k].extra].txID) < 100) continue;
					else if ((newtx = findUnusedChannel()) == -1)
						ThrowError("Out of free TX/RX channels!");

					generateChannelSequence(oldtx, newtx, kHgltSector, 3);
				}
				
			}

			// from the sprites perspective...
			for (i = 0; i < highlightsectorcnt; i++) {
				
				nIdx = highlightsector[i];
				for (k = headspritesect[nIdx]; k != -1; k = nextspritesect[k]) {
					if (sprite[k].extra <= 0 || (oldtx = xsprite[sprite[k].extra].txID) < 100) continue;
					else if ((newtx = findUnusedChannel()) == -1)
						ThrowError("Out of free TX/RX channels!");

					generateChannelSequence(oldtx, newtx, kHgltSector, 3);
				}
			}
		}
		
		if (var1 & kChlCheckOutsideS) {
			
			// check if could SEND to outside of the highlight
			// -----------------------------------
			for (i = 0; i < highlightsectorcnt; i++) {
				nIdx = highlightsector[i];
				
				// sector
				if (sector[nIdx].extra > 0) {
					if ((newtx = findUnusedChannel()) == -1) { ThrowError("Out of free TX/RX channels!"); }
					else if ((oldtx = xsector[sector[nIdx].extra].txID) >= 100 && chlOutHighlight(oldtx, 3, kHgltSector))
						generateChannelSequence(oldtx, newtx, kHgltSector, 3);
				}
				
				// all walls of sector
				for (k = sector[nIdx].wallptr; k < sector[nIdx].wallptr + sector[nIdx].wallnum; k++) {
					if (wall[k].extra <= 0 || (oldtx = xwall[wall[k].extra].txID) < 100) continue;
					else if (!chlOutHighlight(oldtx, 3, kHgltSector)) continue;
					else if ((newtx = findUnusedChannel()) == -1) { ThrowError("Out of free TX/RX channels!"); }
					else generateChannelSequence(oldtx, newtx, kHgltSector, 3);
					
				}
				
				// all sprites of sector
				for (k = headspritesect[nIdx]; k != -1; k = nextspritesect[k]) {
					if (sprite[k].extra <= 0 || (oldtx = xsprite[sprite[k].extra].txID) < 100) continue;
					else if (!chlOutHighlight(oldtx, 3, kHgltSector)) continue;
					else if ((newtx = findUnusedChannel()) == -1) { ThrowError("Out of free TX/RX channels!"); }
					else generateChannelSequence(oldtx, newtx, kHgltSector, 3);
				}
			}
		}
		
	}
	
	if ((htype & kHgltPoint) && highlightcnt > 0) {
		
		if (var1 & kChlCheckOutsideR) {
			
			// set RX ID for objects that have it outside of highlight array
			for (i = 0; i < highlightcnt; i++) {
				if ((highlight[i] & 0xC000) != 0x4000) continue;
				nIdx = highlight[i] & 0x3FFF;
				if (sprite[nIdx].extra <= 0 || xsprite[sprite[nIdx].extra].rxID < 100)
					continue;
				
				oldrx = xsprite[sprite[nIdx].extra].rxID; // save old rx id
				
				// search if there is no objects in highlight with txID == oldrx
				for (j = 0; j < highlightcnt; j++) {
					if ((highlight[j] & 0xC000) != 0x4000) continue;
					nIdx2 = highlight[j] & 0x3FFF;
					if (sprite[nIdx2].extra <= 0) continue;
					else if (xsprite[sprite[nIdx2].extra].txID == oldrx)
						break;
				}
				
				
				if (j != highlightcnt) continue; // found some object, no need to change it in this loop
				else if ((newrx = findUnusedChannel()) == -1)
					ThrowError("Out of free TX/RX channels!");
				
				// set newrx for all objects in highlight with rxID = oldrx
				for (j = 0; j < highlightcnt; j++) {
					if ((highlight[j] & 0xC000) != 0x4000) continue;
					nIdx2 = highlight[j] & 0x3FFF;
					if (sprite[nIdx2].extra <= 0) continue;
					else if (xsprite[sprite[nIdx2].extra].rxID == oldrx)
						setChannelId(OBJ_SPRITE, (short)nIdx2, (ushort)newrx, 1);
				}
			}
		}
		
		if (var1 & kChlCheckInsideRS) {
			
			// set TX ID and RX ID for all objects in highlight and point each other
			for (i = 0; i < highlightcnt; i++) {
				if ((highlight[i] & 0xC000) != 0x4000) continue;
				nIdx = highlight[i] & 0x3FFF;
				if (sprite[nIdx].extra <= 0 || xsprite[sprite[nIdx].extra].txID < 100)
					continue;

				oldtx = xsprite[sprite[nIdx].extra].txID; // save old tx id
				if ((newtx = findUnusedChannel()) == -1)
					ThrowError("Out of free TX/RX channels!");
				
				// set newtx for all highlighted objects that have same oldtx
				for (j = 0; j < highlightcnt; j++) {
					if ((highlight[j] & 0xC000) != 0x4000) continue;
					nIdx2 = highlight[j] & 0x3FFF;
					if (sprite[nIdx2].extra <= 0) continue;
					else if (xsprite[sprite[nIdx2].extra].txID == oldtx)
						setChannelId(OBJ_SPRITE, (short)nIdx2, (ushort)newtx, 2);
					
					// change rxID to the newtx for objects that have rxID = oldtx
					for (z = 0; z < highlightcnt; z++) {
						if ((highlight[z] & 0xC000) != 0x4000)
							continue;
						
						nIdx3 = highlight[z] & 0x3FFF;
						if (sprite[nIdx3].extra <= 0) continue;
						else if (xsprite[sprite[nIdx3].extra].rxID == oldtx)
							setChannelId(OBJ_SPRITE, (short)nIdx3, (ushort)newtx, 1);
						
					}
				}
			}
		}
		
		if (var1 & kChlCheckOutsideS) {
			
			// set TX ID for objects that have it outside of highlight array
			for (i = 0; i < highlightcnt; i++) {
				if ((highlight[i] & 0xC000) != 0x4000) continue;
				nIdx = highlight[i] & 0x3FFF;
				if (sprite[nIdx].extra <= 0 || xsprite[sprite[nIdx].extra].txID < 100)
					continue;
				
				oldtx = xsprite[sprite[nIdx].extra].txID; // save old tx id
				
				// search if there is no objects in highlight with rxID == oldtx
				for (j = 0; j < highlightcnt; j++) {
					if ((highlight[j] & 0xC000) != 0x4000) continue;
					nIdx2 = highlight[j] & 0x3FFF;
					if (sprite[nIdx2].extra <= 0) continue;
					else if (xsprite[sprite[nIdx2].extra].rxID == oldtx)
						break;
				}
				
				
				if (j != highlightcnt) continue; // found some object, no need to change it in this loop
				else if ((newtx = findUnusedChannel()) == -1)
					ThrowError("Out of free TX/RX channels!");
				
				// set newtx for all objects in highlight with txID = oldtx
				for (j = 0; j < highlightcnt; j++) {
					if ((highlight[j] & 0xC000) != 0x4000) continue;
					nIdx2 = highlight[j] & 0x3FFF;
					if (sprite[nIdx2].extra <= 0) continue;
					else if (xsprite[sprite[nIdx2].extra].txID == oldtx)
						setChannelId(OBJ_SPRITE, (short)nIdx2, (ushort)newtx, 2);
				}
				
			}
		}
	}

}

enum{
kFitWall = 0x0001,
kFitSpr  = 0x0002,
kFitWallContinue = 0x0004,
kFitSprContinue  = 0x0008,
};

enum {
kListHgltRewind = 0x0001,
	
};

BOOL listHglt(int* hidx, int* type, int* idx1, int* idx2, int* which, int* fit) {
	
	int i = 0;
	int start = *hidx;
	
	if (*which & kHgltPoint) {
		
		
		if (*hidx >= 0 && *hidx < highlightcnt) {

			if (*fit & kFitWall) {
	
				*type = -1;
				for (i = *hidx; i < highlightcnt; i++) {
					if ((highlight[i] & 0xC000) == 0x4000) continue;
					*type = OBJ_WALL, *idx1 = highlight[i];
					break;
				}

				if (*type == -1) {
					
					*hidx = 0;
					*fit &= ~kFitWall;
					if (*which & kHgltSector)
						*fit |= kFitWallContinue;
					
				} else {
					
					*hidx = i + 1;
					if (*hidx >= highlightcnt) {
						*hidx = 0;
						*fit &= ~kFitWall;
						if (*which & kHgltSector)
							*fit |= kFitWallContinue;
					}

					return TRUE;
					
				}
				
			}
			
			
			if (*fit & kFitSpr) {
	
				*type = -1;
				for (i = *hidx; i < highlightcnt; i++) {
					if ((highlight[i] & 0xC000) != 0x4000) continue;
					*type = OBJ_SPRITE, *idx1 = highlight[i] & 0x3FFF;
					break;
				}
				
				if (*type == -1) {	
					
					*hidx = 0;
					*fit &= ~kFitSpr;
					if (*which & kHgltSector)
						*fit |= kFitSprContinue;
					
				} else {
					
					*hidx = i + 1;
					if (*hidx >= highlightcnt) {
						*hidx = 0;
						*fit &= ~kFitSpr;
						if (*which & kHgltSector)
							*fit |= kFitSprContinue;
					}
					
					return TRUE;
					
				}
			}
		}

		*which &= ~kHgltPoint;
		if (*fit & kFitWallContinue) {
			*fit &= ~kFitWallContinue;
			*fit |= kFitWall;
		}
		
		if (*fit & kFitSprContinue) {
			*fit &= ~kFitSprContinue;
			*fit |= kFitSpr;
		}
			
		*hidx = start = 0;
		*idx1 = *idx2 = -1;
		

	}
	
	if (*which & kHgltSector) {
		
		
		while (start >= 0 && start < highlightsectorcnt) {
			
			*idx2 = highlightsector[start];
			// first get all walls
			if (*fit & kFitWall) {

				if (*idx1 < 0) *idx1 = sector[*idx2].wallptr;
				else if ((*idx1 = *idx1 + 1) >= sector[*idx2].wallptr + sector[*idx2].wallnum) {
					
					*idx1 = -1, *hidx = start = start + 1;
					if (start >= highlightsectorcnt) {
						*fit &= ~kFitWall;
						*hidx = start = 0;
					}
					
					continue;
					
				}

				*type = OBJ_WALL;
				return TRUE;

			}
			
			// get all sprites
			if (*fit & kFitSpr) {

				*idx1 = (*idx1 < 0) ? headspritesect[*idx2] : nextspritesect[*idx1];
				if (*idx1 == -1) {
					*hidx = start = start + 1;
					if (start >= highlightsectorcnt) break; 
					else continue;
				}

				*type = OBJ_SPRITE;
				return TRUE;
			}
			
			break;
			
		}
		
		*which &= ~kHgltSector;
		*hidx = start = 0;
		*idx1 = *idx2 = -1;

	}
	
	return FALSE;
	
}

void hgltIsolatePathMarkers(int which) {
	

	int odata1, ndata1, sect;
	int fit1 = kFitSpr, hidx1 = 0, oidx1 = -1, otype1 = -1, sidx1 = -1, htype1 = which;
	int fit2 = kFitSpr, hidx2 = 0, oidx2 = -1, otype2 = -1, sidx2 = -1, htype2 = which;
	
	while (listHglt(&hidx1, &otype1, &oidx1, &sidx1, &htype1, &fit1)) {
		
		if (sprite[oidx1].type == kMarkerPath && sprite[oidx1].extra > 0) {
	
			odata1 = xsprite[sprite[oidx1].extra].data1;
			if ((ndata1 = findUnusedPath()) < 0)
				ThrowError("Failed to get free path marker id!");
			
			xsprite[sprite[oidx1].extra].data1 = ndata1;
			
			// search all markers that have data1 == odata1 or data2 == odata1 and replace it
			fit2 = kFitSpr, hidx2 = 0, oidx2 = -1, otype2 = -1, sidx2 = -1, htype2 = which;
			while (listHglt(&hidx2, &otype2, &oidx2, &sidx2, &htype2, &fit2)) {
				if (sprite[oidx2].type == kMarkerPath && sprite[oidx2].extra > 0) {
					
					XSPRITE* pXSpr = &xsprite[sprite[oidx2].extra];
					if (pXSpr->data1 == odata1) pXSpr->data1 = ndata1;
					if (pXSpr->data2 == odata1) pXSpr->data2 = ndata1;

				}
			
			}
			
		}
		
		// set new id for path sectors, if any
		fit2 = kFitWall, hidx2 = 0, oidx2 = -1, otype2 = -1, sidx2 = -1, htype2 = which;
		while (listHglt(&hidx2, &otype2, &oidx2, &sidx2, &htype2, &fit2)) {
			
			sect = sectorofwall(oidx2);
			if (sector[sect].type == kSectorPath && sector[sect].extra > 0 
				&& xsector[sector[sect].extra].data == odata1) {
					xsector[sector[sect].extra].data = ndata1;
			}
			
		}

	}
	
}

void hgltIsolateRorMarkers(int which) {
	
	int oldId, newId;

	int fit1 = kFitSpr, hidx1 = 0, oidx1 = -1, otype1 = -1, sidx1 = -1, htype1 = which;
	int fit2 = kFitSpr, hidx2 = 0, oidx2 = -1, otype2 = -1, sidx2 = -1, htype2 = which;
	while (listHglt(&hidx1, &otype1, &oidx1, &sidx1, &htype1, &fit1)) {
		if (sprite[oidx1].extra <= 0) continue;
		else if (sprite[oidx1].type > 6 && sprite[oidx1].type <= 14 && sprite[oidx1].type != 8) {
			
			newId = findUnusedStack();
			oldId = xsprite[sprite[oidx1].extra].data1;
			if (newId == oldId)
				continue;
			
			fit2 = kFitSpr, hidx2 = 0, oidx2 = -1, otype2 = -1, sidx2 = -1, htype2 = which;
			while (listHglt(&hidx2, &otype2, &oidx2, &sidx2, &htype2, &fit2)) {
				if (sprite[oidx2].extra <= 0 || xsprite[sprite[oidx2].extra].data1 != oldId) continue;
				else if (sprite[oidx2].type < 6 || sprite[oidx2].type > 14 || sprite[oidx2].type == 8)
					continue;
				
				xsprite[sprite[oidx2].extra].data1 = newId;
				
			}
			
		}
	
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
			
			sprite[j].ang = (short) ((sprite[j].ang + step) & kAngMask);
			
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

/* void hgltSectGetPolygon(int* x1, int* y1, int* x2, int* y2) {
	
	short nwalls[4];
	hgltSectGetEdgeWalls(&nwalls[0], &nwalls[1], &nwalls[2], &nwalls[3]);
	*x1 = wall[nwalls[0]].x;
	*x2 = wall[nwalls[1]].x;
	*y1 = wall[nwalls[2]].y;
	*y2 = wall[nwalls[3]].y;
} */

void hgltSectGetEdgeWalls(short* lw, short* rw, short* tw, short* bw) {
	
	int i, j, swal, ewal; short dlw, drw, dtw, dbw;
	
	getSectorWalls(highlightsector[0], &swal, &ewal);
	*lw = *rw = *tw = *bw = swal;
	
	for(i = 0; i < highlightsectorcnt; i++)
	{
		j = highlightsector[i];
		loopGetEdgeWalls(sector[j].wallptr, &dlw, &drw, &dtw, &dbw);
		if (wall[*lw].x > wall[dlw].x) *lw = dlw;
		if (wall[*rw].x < wall[drw].x) *rw = drw;
		if (wall[*tw].y > wall[dtw].y) *tw = dtw;
		if (wall[*bw].y < wall[dbw].y) *bw = dbw;
	}
}

void hgltSectGetEdges(int* left, int* right, int* top, int* bot) {
	
	short lw, rw, bw, tw;
	hgltSectGetEdgeWalls(&lw, &rw, &bw, &tw);
	*left = wall[lw].x; *right = wall[rw].x;
	*top  = wall[tw].y; *bot   = wall[bw].y;

}

void hgltSectAvePoint(int* ax, int* ay) {
	
	short lw, rw, bw, tw;
	hgltSectGetEdgeWalls(&lw, &rw, &bw, &tw);
	*ax = wall[lw].x + ((wall[rw].x - wall[lw].x) >> 1);
	*ay = wall[tw].y + ((wall[bw].y - wall[tw].y) >> 1);
}

void sectChgXY(int nSector, int bx, int by, int, int) {
	
	int i, swal, ewal;
	getSectorWalls(nSector, &swal, &ewal);
	for (i = swal; i <= ewal; i++)
	{
		wall[i].x += bx;
		wall[i].y += by;
	}
	
	for (i = headspritesect[nSector]; i >= 0; i = nextspritesect[i])
	{
		sprite[i].x += bx;
		sprite[i].y += by;
	}
}

void setChannelId(ushort otype, short oidx, ushort newchl, BYTE which) {
	
	switch (otype) {
		case OBJ_SPRITE:
			dassert(oidx >= 0 && oidx < kMaxSprites);
			dassert(sprite[oidx].extra > 0 && sprite[oidx].extra < kMaxXSprites);
			if (which & 0x0001) xsprite[sprite[oidx].extra].rxID = newchl;
			if (which & 0x0002) xsprite[sprite[oidx].extra].txID = newchl;
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			dassert(oidx >= 0 && oidx < kMaxSectors);
			dassert(sector[oidx].extra > 0 && sector[oidx].extra < kMaxXSectors);
			if (which & 0x0001) xsector[sector[oidx].extra].rxID = newchl;
			if (which & 0x0002) xsector[sector[oidx].extra].txID = newchl;
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			dassert(oidx >= 0 && oidx < kMaxWalls);
			dassert(wall[oidx].extra > 0 && wall[oidx].extra < kMaxXWalls);
			if (which & 0x0001) xwall[wall[oidx].extra].rxID = newchl;
			if (which & 0x0002) xwall[wall[oidx].extra].txID = newchl;
			break;
	}
	
}

BOOL chlOutHighlight(int chl, int which, int highlightType) {
	
	register int i, j; int sect;
	BOOL rx = (BOOL)(which & 0x0001);
	BOOL tx = (BOOL)(which & 0x0002);
	for (i = 0; i < numsectors; i++)
	{
		if (sector[i].extra <= 0) continue;
		else if ((rx && xsector[sector[i].extra].rxID == chl) || (tx && xsector[sector[i].extra].txID == chl))
		{
			if ((highlightType & kHgltSector) && hgltCheck(OBJ_SECTOR, i) < 0)
				return TRUE;
		}
	}
	
	for (i = 0; i < numwalls; i++)
	{
		if (wall[i].extra <= 0) continue;
		else if ((rx && xwall[wall[i].extra].rxID == chl) || (tx && xwall[wall[i].extra].txID == chl))
		{
			if ((highlightType & kHgltSector) && hgltCheck(OBJ_SECTOR, sectorofwall(i)) < 0)
				return TRUE;
		}
	}
	
	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum >= kMaxStatus || sprite[i].extra <= 0) continue;
		else if ((rx && xsprite[sprite[i].extra].rxID == chl) || (tx && xsprite[sprite[i].extra].txID == chl))
		{
			if ((highlightType & kHgltSector) && hgltCheck(OBJ_SECTOR, sprite[i].sectnum) < 0)
				return TRUE;
		}
	}
	
	return FALSE;
	
}

void generateChannelSequence(int oldtx, int newtx, int highlightType, int which) {

	int i = 0, j = 0;
	
	// all highlighted sectors
	if (highlightType & kHgltSector) {
		
		int sect = -1;
		for (i = 0; i < highlightsectorcnt; i++) {

			sect = highlightsector[i];
			if (sector[sect].extra > 0) {
				
				if((which & 0x2) && xsector[sector[sect].extra].txID == oldtx) xsector[sector[sect].extra].txID = newtx;
				if((which & 0x1) && xsector[sector[sect].extra].rxID == oldtx) xsector[sector[sect].extra].rxID = newtx;
				
			}
			
			// all walls of all highlighted sectors
			for (j = sector[sect].wallptr; j < sector[sect].wallptr + sector[sect].wallnum; j++) {
				if(wall[j].extra <= 0) continue;
				if((which & 0x2) && xwall[wall[j].extra].txID == oldtx) xwall[wall[j].extra].txID = newtx;
				if((which & 0x1) && xwall[wall[j].extra].rxID == oldtx) xwall[wall[j].extra].rxID = newtx;
			}
			
			// all sprites of all highlighted sectors
			for (j = headspritesect[sect]; j != -1; j = nextspritesect[j]) {
				if(sprite[j].extra <= 0 || sprite[j].statnum == kMaxStatus) continue;
				if((which & 0x2) && xsprite[sprite[j].extra].txID == oldtx) xsprite[sprite[j].extra].txID = newtx;
				if((which & 0x1) && xsprite[sprite[j].extra].rxID == oldtx) xsprite[sprite[j].extra].rxID = newtx;
			}
			
		}
		
	}
	
	// all highlighted sprites
	if (highlightType & kHgltPoint) {
		
		int spr = -1;
		for (i = 0; i < highlightcnt; i++) {
			if ((highlight[i] & 0xC000) != 0x4000) continue;
			spr = highlight[i] & 0x3FFF;
			if (sprite[spr].extra <= 0) continue;
			if (xsprite[sprite[spr].extra].txID == oldtx) xsprite[sprite[j].extra].txID = newtx;
			if (xsprite[sprite[spr].extra].rxID == oldtx) xsprite[sprite[j].extra].rxID = newtx;
		}
		
	}
	
}

BOOL hgltShowStatus(int x, int y) {
	
	QFONT* pFont = qFonts[5];
	int xstep = pFont->width, len = 0;

	if (gHgltc > 0 || highlightcnt > 0 || highlightsectorcnt > 0)
	{
		sprintf(buffer, "HIGHLIGHT:");
		gfxPrinTextShadow(x, y, gStdColor[kColorYellow], buffer, pFont);
		x += gfxGetTextLen(buffer, pFont) + xstep;
		
		if (gHgltc > 0)
		{
			sprintf(buffer, "GRADIENT=%d", gHgltc);
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
			for (j = 0; j < highlightsectorcnt; j++) redRestore(highlightsector[j]);
			deletesector(nSector);
		}
		
		highlightsectorcnt = -1;
	}
	else
	{
		deletesector(nSector);
	}
	
}
