/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022: Originally written by NoOne.
// A class to treat parallax sectors as "local" (room) or "global" (map) sky.
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

#include "db.h"
#include "xmpsky.h"
#include "xmpmaped.h"

#define SET_GLOBAL_COUNTER(x) (x ? 0 : -1)
static int helperGetMostHigh(int* pMost, int nCnt, int* r)
{
	int t = 0;
	while(nCnt--)
	{
		if (pMost[nCnt] >= t)
			t = pMost[nCnt], *r = nCnt;
	}
	
	return t;
}

int Sky::pieces = 1;
unsigned char Sky::customBitsFlag = 0;
unsigned char Sky::tileRepeatCount = 0;
int Sky::MakeSimilar(int nSect, int nFor, BOOL global)
{
	sectortype* pSect;
	int nPic, nPal, nShd, nPny, got;
	char isCeil = (nFor == OBJ_CEILING);
	IDLIST collected(true);
	int32_t* pDb;

	nPic = (isCeil) ? sector[nSect].ceilingpicnum : sector[nSect].floorpicnum;
	CollectSectors(&collected, (isCeil) ? ceilPicMatch : floorPicMatch, nSect, nPic, SET_GLOBAL_COUNTER(global));
	collected.AddIfNot(nSect);

	// search for the rest most used properties in global scope
	got = GetMostUsed(nSect, nFor, TRUE, &nPic, &nPal, &nShd, &nPny);

	// parallax all possible non-parallaxed sectors with same tile and assimilate the sky
	pDb = collected.GetPtr();
	
	while(*pDb >= 0)
	{
		pSect = &sector[*pDb];
		if (isCeil)
		{
			if (!isSkySector(*pDb, nFor))
				pSect->ceilingstat |= kSectParallax;
			
			pSect->ceilingpicnum = nPic;
			
			if (got & 0x02)	pSect->ceilingpal	 	= nPal;
			if (got & 0x04)	pSect->ceilingshade	 	= nShd;
			if (got & 0x08)	pSect->ceilingypanning	= nPny;
		}
		else
		{
			if (!isSkySector(*pDb, nFor))
				pSect->floorstat |= kSectParallax;

			pSect->floorpicnum 	= nPic;
			
			if (got & 0x02)	pSect->floorpal	 		= nPal;
			if (got & 0x04)	pSect->floorshade	 	= nShd;
			if (got & 0x08)	pSect->floorypanning	= nPny;
		}
		
		pDb++;
	}
	
	return collected.GetLength();
}

int Sky::Disable(int nSect, int nFor, BOOL global)
{
	IDLIST collected(true);
	CollectSectors(&collected, isSkySector, nSect, nFor, SET_GLOBAL_COUNTER(global));
	int32_t* pDb = collected.GetPtr();
	
	while(*pDb >= 0)
	{
		sectCstatRem(*pDb, kSectParallax, nFor);
		sectCstatRem(*pDb, kSectShadeFloor, OBJ_FLOOR);
		pDb++;
	}
	
	scrSetMessage("%s sky is %s", GlobalOrLocal(global), onOff(0));
	return collected.GetLength();
}

int Sky::FixPan(int nSect, int nFor, BOOL global)
{
	int panY = (nFor == OBJ_CEILING) ? sector[nSect].ceilingypanning : sector[nSect].floorypanning;
	
	GetMostUsed(nSect, nFor, global, NULL, NULL, NULL, &panY);
	return Setup(nSect, nFor, 256, -1, -1, 0, panY, global);
}

int Sky::CollectSectors(IDLIST* pList, MATCHFUNC CheckFunc, int nSect, int nFor, int globcnt)
{
	int s, e;
	if (CheckFunc && !CheckFunc(nSect, nFor))
		return pList->GetLength();
	
	pList->AddIfNot(nSect);
	
	if (globcnt == 0)
	{
		while(globcnt < numsectors) // collect all the matching sectors in the whole map
			CollectSectors(pList, CheckFunc, globcnt++, nFor, 1);
	}
	else if (globcnt < 0)
	{
		getSectorWalls(nSect, &s, &e);
		while(s <= e) // collect all the matching sectors while not separated
		{
			if (wall[s].nextsector >= 0 && !pList->Exists(wall[s].nextsector))
				CollectSectors(pList, CheckFunc, wall[s].nextsector, nFor, -1);
			
			s++;
		}
	}
	
	return pList->GetLength();
}

int Sky::Setup(int nSect, int nFor, int nShade, int nPal, int nNewPic, int panX, int panY, BOOL global)
{
	IDLIST collected(true); char isCeil = (nFor == OBJ_CEILING);
	CollectSectors(&collected, isSkySector, nSect, nFor, SET_GLOBAL_COUNTER(global));
	int32_t* pDb = collected.GetPtr();
	
	while(*pDb >= 0)
	{
		sectortype* pSect = &sector[*pDb];
		if (isCeil)
		{
			if (panX >= 0) 		pSect->ceilingxpanning = panX;
			if (panY >= 0) 		pSect->ceilingypanning = panY;
			if (nShade != 256) 	pSect->ceilingshade = nShade;
			if (nPal >= 0) 		pSect->ceilingpal = nPal;
			if (nNewPic >= 0)	pSect->ceilingpicnum = nNewPic;
		}
		else
		{
			if (panX >= 0) 		pSect->floorxpanning = panX;
			if (panY >= 0) 		pSect->floorypanning = panY;
			if (nShade != 256) 	pSect->floorshade = nShade;
			if (nPal >= 0) 		pSect->floorpal = nPal;
			if (nNewPic >= 0)	pSect->floorpicnum = nNewPic;
		}
		
		pDb++;
	}
	
	return collected.GetLength();
}


int Sky::GetMostUsed(int nSect, int nFor, BOOL global, int* nPic, int* nPal, int* nShade, int* nPy)
{
	char isCeil = (nFor == OBJ_CEILING);
	int most[kMaxTiles], retn = 0;
	IDLIST collected(true);
	sectortype* pSect;
	int32_t* pDb;
		
	if (!CollectSectors(&collected, isSkySector, nSect, nFor, SET_GLOBAL_COUNTER(global)))
		return 0;
	
	if (nPic)
	{
		memset(most, 0, sizeof(most));
		pDb = collected.GetPtr();
		
		while(*pDb >= 0)
		{
			pSect = &sector[*pDb];
			most[(isCeil) ? pSect->ceilingpicnum : pSect->floorpicnum]++;
			pDb++;
		}
		
		if (helperGetMostHigh(most, kMaxTiles, nPic))
			retn |= 0x01;
	}
	
	if (nPal)
	{
		memset(most, 0, sizeof(most));
		pDb = collected.GetPtr();
		
		while(*pDb >= 0)
		{
			pSect = &sector[*pDb];
			most[(isCeil) ? pSect->ceilingpal : pSect->floorpal]++;
			pDb++;
		}
		
		if (helperGetMostHigh(most, 256, nPal))
			retn |= 0x02;
	}
	
	if (nShade)
	{
		memset(most, 0, sizeof(most));
		pDb = collected.GetPtr();
		
		while(*pDb >= 0)
		{
			pSect = &sector[*pDb];
			most[(unsigned char)((isCeil) ? pSect->ceilingshade : pSect->floorshade)]++;
			pDb++;
		}
		
		if (helperGetMostHigh(most, 256, nShade))
		{
			*nShade = (signed char)(*nShade);
			retn |= 0x04;
		}
	}
	
	if (nPy)
	{
		memset(most, 0, sizeof(most));
		pDb = collected.GetPtr();
		
		while(*pDb >= 0)
		{
			pSect = &sector[*pDb];
			most[(isCeil) ? pSect->ceilingypanning : pSect->floorypanning]++;
			pDb++;
		}
		
		if (helperGetMostHigh(most, 256, nPy))
			retn |= 0x08;
	}

	return retn;
}

BOOL Sky::RotateRecursive(int dir)
{
	int i;
	for (i = 0; i < pieces; i++)
	{
		if (pskyoff[i]) // must have 2 or more unique sky tiles
		{
			if (!dir) // left by 1 tile
			{
				i = pskyoff[0];
				memmove(&pskyoff[0], &pskyoff[1], sizeof(pskyoff[0])*(pieces-1));
				pskyoff[pieces - 1] = i;
			}
			else // left by 1 tile pieces-1 times (right by 1 tile)
			{
				i = pieces;
				while(--i)
					RotateRecursive(0);
			}
				
			return TRUE;
		}
	}
	
	return FALSE;
}

BOOL Sky::Rotate(BOOL right)
{
	BOOL retn;
	if ((retn = RotateRecursive(right)) == TRUE)
		scrSetMessage("%s sky rotate: %s", GlobalOrLocal(TRUE), RightOrLeft(right));
	else
		scrSetMessage("Must have 2 or more unique sky tiles to rotate.");

	return retn;
}

int Sky::SetPan(int nSect, int nFor, int panX, int panY, BOOL global)
{
	int i;
	if ((i = Setup(nSect, nFor, 256, -1, -1, panX, panY, global)) > 0)
		scrSetMessage("%s %s sky y-pos: %d", GlobalOrLocal(global), gSearchStatNames[nFor], panY);
	
	return i;
}

int Sky::SetPal(int nSect, int nFor, int nPal, BOOL global)
{
	int i;
	if ((i = Setup(nSect, nFor, 256, nPal, -1, -1, -1, global)) > 0)
		scrSetMessage("%s %s sky palette: #%d", GlobalOrLocal(global), gSearchStatNames[nFor], nPal);
	
	return i;
}

int Sky::SetShade(int nSect, int nFor, int nShade, BOOL global)
{
	int i;
	if ((i = Setup(nSect, nFor, nShade, -1, -1, -1, -1, global)) > 0)
		scrSetMessage("%s %s sky shade: %+d", GlobalOrLocal(global), gSearchStatNames[nFor], nShade);
	
	return i;
}

void Sky::SetBits(int nPic, int* nUnique)
{
	int i=0, t=0, u=0;
	if (!customBitsFlag)
		pskybits = (short)(10 - (picsiz[nPic] & 0x0F));
	
	//if (!rngok(pskybits, 0, 9))
		//pskybits = 8;
	
	pieces = 1 << pskybits;
	
	if (!tileRepeatCount)
	{
		while(u < pieces) // get unique tiles range
		{
			if (tilesizx[nPic + u] != tilesizx[nPic]) break;
			if (tilesizy[nPic + u] != tilesizy[nPic]) break;
			u++;
		}
	}
	else
	{
		u = tileRepeatCount; // fixed up amount to repeat
	}
	
	u = ClipHigh(u, pieces);
	
	// set unique tiles
	while(i < u)
		pskyoff[i] = i++;
	
	// repeat old tiles to wrap around
	while(i < pieces)
	{
		if (u % 2 != 0) // odd
			t = IncRotate(t, u);
		
		pskyoff[i++] = t;
		
		if (u % 2 == 0) // even
			t = IncRotate(t, u);
	}
	
	// rotate the sky around to update tile offset table
	// because it could be rotated by the user previously
	Sky::RotateRecursive(1);
	
	if (nUnique)
		*nUnique = u;
}

int Sky::SetPic(int nSect, int nFor, int nPic, BOOL global)
{
	int nUnique;
	SetBits(nPic, &nUnique);
	scrSetMessage("%s sky tile #%d, total tiles: %d/%d (Custom bits: %s)", GlobalOrLocal(global), nPic, nUnique, pieces, onOff(customBitsFlag));
	return Setup(nSect, nFor, 256, -1, nPic, -1, -1, global);
}

int Sky::ToggleFloorShade(int nSect, BOOL global)
{
	int c = 0; int32_t* pDb;
	IDLIST collected(true);
	sectortype* pSect;
	
	if (CollectSectors(&collected, isSkySector, nSect, OBJ_CEILING, SET_GLOBAL_COUNTER(global)))
	{
		pDb = collected.GetPtr();
		while(*pDb >= 0)
		{
			pSect =& sector[*pDb];
			if (pSect->floorstat & kSectShadeFloor)
				c++;
			
			pDb++;
		}
		
		pDb = collected.GetPtr();
		while(*pDb >= 0)
		{
			pSect =& sector[*pDb];
			if (!c)
				pSect->floorstat |= kSectShadeFloor;
			else 
				pSect->floorstat &= ~kSectShadeFloor;
			
			pDb++;
		}
		
		scrSetMessage("%s shade source is: %s", GlobalOrLocal(global), gSearchStatNames[(!c) ? OBJ_FLOOR : OBJ_CEILING]);
	}

	return c;
}