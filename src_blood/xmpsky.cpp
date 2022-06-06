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
#include "screen.h"
#include "xmpsky.h"
#include "xmpstub.h"
#include "xmpmisc.h"

void Sky::ToggleBits(int nPic)
{
	int i;
	i = (10 - (picsiz[nPic] & 0x0F));
	pskybits = (short)IncRotate(pskybits, 6);
	gCustomSkyBits = (BOOL)(i != pskybits);
	gSkyCount = 1 << pskybits;
	
	scrSetMessage("Parallax tiles: %i (Custom amount: %s)", gSkyCount, onOff(gCustomSkyBits));
}

int Sky::MakeSimilar(int nSect, int nFor, BOOL global)
{
	int i, j; sectortype* pSect;
	unsigned short collected[kMaxSectors];
	int nPic = j = ((nFor == OBJ_CEILING) ? sector[nSect].ceilingpicnum : sector[nSect].floorpicnum);
	int nPal, nShd, nPny;
	
	// search most used properties in global scope
	int got = GetMostUsed(nSect, nFor, TRUE, &nPic, &nPal, &nShd, &nPny);
	if ((got & 0x01))	// got most used sky tile
	{
		if (j == nPic)	// sky tile == current non-parallax sector tile
		{
			// collect sectors that have same tile
			j = CollectSectors(collected, (nFor == OBJ_CEILING) ? ceilPicMatch : floorPicMatch, nSect, nPic, global);
		}
		else
		{
			// only add current sector
			collected[0] = nSect;
			j = 1;
		}
	}
	else
	{
		return 0; // nothing found?
	}

	
	// make all possible non-parallaxed sectors with same tile parallaxed
	// and assimilate the sky
	for (i = 0; i < j; i++)
	{
		pSect = &sector[collected[i]];
		if (nFor == OBJ_CEILING)
		{
			if (!isSkySector(collected[i], nFor))
				pSect->ceilingstat |= kSectParallax;
			
			if (got & 0x01)	pSect->ceilingpicnum 	= nPic;
			if (got & 0x02)	pSect->ceilingpal	 	= nPal;
			if (got & 0x04)	pSect->ceilingshade	 	= nShd;
			if (got & 0x08)	pSect->ceilingypanning	= nPny;
		}
		else
		{
			if (!isSkySector(collected[i], nFor))
				pSect->floorstat |= kSectParallax;
			
			if (got & 0x01)	pSect->floorpicnum 		= nPic;
			if (got & 0x02)	pSect->floorpal	 		= nPal;
			if (got & 0x04)	pSect->floorshade	 	= nShd;
			if (got & 0x08)	pSect->floorypanning	= nPny;
		}
	}
	
	return j;
	
}

int Sky::FixPan(int nSect, int nFor, BOOL global)
{
	int panY = (nFor == OBJ_CEILING) ? sector[nSect].ceilingypanning : sector[nSect].floorypanning;
	
	GetMostUsed(nSect, nFor, global, NULL, NULL, NULL, &panY);
	return Setup(nSect, nFor, 256, -1, -1, 0, panY, global);
}

void Sky::CollectSectors(BOOL done[kMaxSectors], MATCHFUNC CheckFunc, int nSect, int nFor, int globcnt)
{
	int i, j, swal, ewal;
	if (CheckFunc && !CheckFunc(nSect, nFor))
		return;
	
	done[nSect] = 1;
	if (globcnt == 0)
	{
		// collect all the matching sectors in the whole map
		while(globcnt < numsectors)
			CollectSectors(done, CheckFunc, globcnt++, nFor, 1);
	}
	else if (globcnt < 0)
	{
		// collect all the matching sectors while not separated
		getSectorWalls(nSect, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
		{
			if (wall[j].nextsector < 0 || done[wall[j].nextsector]) continue;
			CollectSectors(done, CheckFunc, wall[j].nextsector, nFor, -1);
		}
	}
}

int Sky::CollectSectors(unsigned short sorted[kMaxSectors], MATCHFUNC CheckFunc, int nSect, int nFor, BOOL global)
{
	int i, c = 0;
	BOOL done[kMaxSectors]; memset(done, 0, sizeof(done));
	CollectSectors(done, CheckFunc, nSect, nFor, (global) ? 0 : -1);
	for (i = 0; i < LENGTH(done); i++)
	{
		if (done[i])
			sorted[c++] = i;
	}
	
	return c;
}

int Sky::Setup(int nSect, int nFor, int nShade, int nPal, int nNewPic, int panX, int panY, BOOL global)
{
	int i, j;
	unsigned short collected[kMaxSectors];
	j = CollectSectors(collected, isSkySector, nSect, nFor, global);
	for (i = 0; i < j; i++)
	{
		sectortype* pSect = &sector[collected[i]];
		if (nFor == OBJ_CEILING)
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
	}
	
	return j;
	
}


int Sky::GetMostUsed(int nSect, int nFor, BOOL global, int* nPic, int* nPal, int* nShade, int* nPy)
{
	sectortype* pSect;
	unsigned short collected[kMaxSectors];
	int most[kMaxTiles + 256], i, j, c, found, retn = 0;
	
	if ((found = CollectSectors(collected, isSkySector, nSect, nFor, global)) <= 0)
		return retn;
	
	if (nPic)
	{
		i = found, c = 0, j = kMaxTiles;
		memset(most, 0, sizeof(most));
		
		while(i--)
		{
			pSect = &sector[collected[i]];
			if (nFor == OBJ_CEILING)
				most[pSect->ceilingpicnum]++;
			else
				most[pSect->floorpicnum]++;
		}
		
		while(j--)
		{
			if (most[j] >= c)
				c = most[j], *nPic = j;
		}
		
		if (c)
			retn |= 0x01;
	}
	
	if (nPal)
	{
		i = found, c = 0, j = 256;
		memset(most, 0, sizeof(most));
		
		while(i--)
		{
			pSect = &sector[collected[i]];
			if (nFor == OBJ_CEILING)
				most[pSect->ceilingpal]++;
			else
				most[pSect->floorpal]++;
		}
		
		while(j--)
		{
			if (most[j] >= c)
				c = most[j], *nPal = j;
		}
		
		if (c)
			retn |= 0x02;
	}
	
	if (nShade)
	{
		i = found, c = 0, j = 256;
		memset(most, 0, sizeof(most));
		
		while(i--)
		{
			pSect = &sector[collected[i]];
			if (nFor == OBJ_CEILING)
				most[(unsigned char)pSect->ceilingshade]++;
			else
				most[(unsigned char)pSect->floorshade]++;
		}
		
		while(j--)
		{
			if (most[j] >= c)
				c = most[j], *nShade = (signed char)j;
		}
		
		if (c)
			retn |= 0x04;
	}
	
	if (nPy)
	{
		i = found, c = 0, j = 256;
		memset(most, 0, sizeof(most));
		
		while(i--)
		{
			pSect = &sector[collected[i]];
			if (nFor == OBJ_CEILING)
				most[pSect->ceilingypanning]++;
			else
				most[pSect->floorypanning]++;
		}
		
		while(j--)
		{
			if (most[j] >= c)
				c = most[j], *nPy = j;
		}
		
		if (c)
			retn |= 0x08;
	}

	return retn;
}

BOOL Sky::RotateRecursive(int dir)
{
	// can only rotate if 2 or more sky tiles
	if (gSkyCount < 2)
		return FALSE;
		
	int i, j;
	int start = gSkyCount + (pskyoff[0] - gSkyCount) + 1;
	
	if (!dir) // left by 1 tile
	{
		j = 0, i = gSkyCount - start;
		while(i < gSkyCount)
			pskyoff[i++] = j++;
		
		i = gSkyCount - start;
		while(i--)
			pskyoff[i]++;
	}
 	else // left by 1 tile nSkyCount-1 times (right by 1 tile)
	{
		i = gSkyCount - 1;
		while(i--)
			RotateRecursive(0);
	}
	
	return TRUE;
}

BOOL Sky::Rotate(BOOL right)
{
	BOOL retn;
	if ((retn = RotateRecursive(right)) == TRUE)
		scrSetMessage("%s sky rotate: %s", GlobalOrLocal(TRUE), RightOrLeft(right));
	else
		scrSetMessage("Must have more than 1 sky tile to rotate.");

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

int Sky::SetPic(int nSect, int nFor, int nPic, BOOL global)
{
	int i = pskybits, j = i;
	if (!gCustomSkyBits)
		j = (short)(10 - (picsiz[nPic] & 0x0F));
	
	if (j != i)
	{
		pskybits = j;
	}
	
	gSkyCount = 1 << pskybits;
	for (i = 0; i < gSkyCount; i++)
		pskyoff[i] = i;	
	

	scrSetMessage("%s sky tile #%d, total tiles: %d (Custom bits: %s)", GlobalOrLocal(global), nPic, gSkyCount, onOff(gCustomSkyBits));
	return Setup(nSect, nFor, 256, -1, nPic, -1, -1, global);
}

int Sky::ToggleFloorShade(int nSect, BOOL global)
{
	int i, j = 0, k;
	unsigned short collected[kMaxSectors]; sectortype* pSect;
	if ((i = CollectSectors(collected, isSkySector, nSect, OBJ_CEILING, global)) > 0)
	{
		for (k = 0; k < i; k++)
		{
			pSect =& sector[collected[k]];
			if (pSect->floorstat & kSectShadeFloor)
				j++;
		}
		
		while(k--)
		{
			pSect =& sector[collected[k]];
			if (!j)
				pSect->floorstat |= kSectShadeFloor;
			else 
				pSect->floorstat &= ~kSectShadeFloor;
		}
		
		scrSetMessage("%s shade source is: %s", GlobalOrLocal(global), gSearchStatNames[(!j) ? OBJ_FLOOR : OBJ_CEILING]);
	}

	return j;
}