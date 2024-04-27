#include "xmpmaped.h"
#include "aadjust.h"

void avePointLoop(int s, int e, int *x, int *y)
{
	int dax = 0, day = 0, i;
	for(i = s; i <= e; i++)
	{
		dax += wall[i].x;
		day += wall[i].y;
	}
	
	if (e > s)
	{
		dax /= (e-s+1);
		day /= (e-s+1);
	}
	
	*x = dax;
	*y = day;
}

void midPointLoop(int nFirst, int nLast, int* ax, int *ay)
{
	int x1, y1, x2, y2;
	loopGetBox(nFirst, nLast, &x1, &y1, &x2, &y2);
	
	*ax = x1 + ((x2 - x1)>>1);
	*ay = y1 + ((y2 - y1)>>1);
}

void avePointSector(int nSector, int *x, int *y)
{
	int s, e;
	getSectorWalls(nSector, &s, &e);
	avePointLoop(s, e, x, y);
}

void avePointWall(int nWall, int* x, int* y, int* z)
{
	int nSect = sectorofwall(nWall);
	avePointWall(nWall, x, y);
	
	*z = 0;
	if (nSect >= 0)
	{
		int cz, fz;
		getzsofslope(nSect, *x, *y, &cz, &fz);
		*z = fz - klabs((cz-fz)>>1);
	}
}

void avePointWall(int nWall, int* x, int* y)
{
	*x = ((wall[nWall].x + wall[wall[nWall].point2].x) >> 1);
	*y = ((wall[nWall].y + wall[wall[nWall].point2].y) >> 1);
}



void loopGetWalls(int nStartWall, int* swal, int *ewal)
{
	int i;
	*swal = *ewal = i = nStartWall;
	while(wall[i].point2 != nStartWall) 
	{
		if (wall[i].point2 < *swal) *swal = wall[i].point2;
		if (wall[i].point2 > *ewal) *ewal = wall[i].point2;
		i = wall[i].point2;
	}

}

void loopGetEdgeWalls(int nFirst, int nLast, int* l, int* r, int* t, int* b)
{
	*l = *r = *t = *b = nFirst;
	
	while(nFirst <= nLast)
	{
		if (wall[nFirst].x < wall[*l].x) *l = nFirst;
		if (wall[nFirst].x > wall[*r].x) *r = nFirst;
		if (wall[nFirst].y < wall[*t].y) *t = nFirst;
		if (wall[nFirst].y > wall[*b].y) *b = nFirst;
		nFirst++;
	}
}

void loopGetBox(int nFirst, int nLast, int* x1, int* y1, int* x2, int *y2)
{
	int l, r, t, b;
	loopGetEdgeWalls(nFirst, nLast, &l, &r, &t, &b);
	
	*x1 = wall[l].x;	*x2 = *x1 + (wall[r].x - *x1);
	*y1 = wall[t].y;	*y2 = *y1 + (wall[b].y - *y1);
	
	if (*x1 > *x2)
		swapValues(x1, x2);
	
	if (*y1 > *y2)
		swapValues(y1, y2);
}

void loopChgPos(int s, int e, int bx, int by, int flags)
{
	while(s <= e)
		posChg(&wall[s].x, &wall[s].y, bx, by, flags), s++;
}

void loopRotate(int s, int e, int cx, int cy, int nAng, int flags)
{
	while(s <= e)
		posRotate(&wall[s].x, &wall[s].y, nAng, cx, cy, flags), s++;
}

void loopFlip(int s, int e, int cx, int cy, int flags)
{
	// based on eduke32
	////////////////////////////////////
	
	int32_t* pWalls = (int32_t*)malloc(sizeof(int32_t)*kMaxWalls);
	int i = numwalls, j, t, x1, y1, x2, y2, wA, wB;
	int nSLoop, nELoop;
	walltype buf;
	
	dassert(pWalls != NULL);
	
	while(--i >= 0)
		pWalls[i] = i;
	
	// save position of wall at start of loop
	getWallCoords(s, &x1, &y1, NULL, NULL);
	nSLoop = s; nELoop = e;
	
	// flip walls
	for (i = s; i <= e; i++)
	{
		getWallCoords(i, NULL, NULL, &x2, &y2);
		if (flags & 0x01)
		{
			wall[i].x = (cx - x2) + cx;
			wall[i].y = y2;
		}
		else
		{
			wall[i].x = x2;
			wall[i].y = (cy - y2) + cy;
		}
		
		if (wall[i].point2 == nSLoop)
		{
			nELoop = i;
			if (flags & 0x01)
			{
				wall[nELoop].x = (cx - x1) + cx;
				wall[nELoop].y = y1;
			}
			else
			{
				wall[nELoop].x = x1;
				wall[nELoop].y = (cy - y1) + cy;
			}
			
			t = (nELoop - nSLoop)>>1;
			for (j = 1; j <= t; j++)
			{
				wA = nSLoop + j;
				wB = nELoop - j + 1;
				
				memcpy(&buf, &wall[wA], sizeof(walltype));
				memcpy(&wall[wA], &wall[wB], sizeof(walltype));
				memcpy(&wall[wB], &buf, sizeof(walltype));

				pWalls[wA] = wB;
				pWalls[wB] = wA;
			}
			
			// make point2 point to next wall in loop
			for (j = nSLoop; j < nELoop; j++) wall[j].point2 = j + 1;
			wall[nELoop].point2 = nSLoop;
			nSLoop = nELoop + 1;
			
			// save position of wall at start of loop
			getWallCoords(nSLoop, &x1, &y1, NULL, NULL);
		}
	}
	
	// fix nextwalls
	for (i = s; i <= e; i++)
	{
		if (wall[i].nextwall >= 0)
			wall[i].nextwall = pWalls[wall[i].nextwall];
	}
	
	free(pWalls);
}

void loopDelete(int s, int e)
{
	int x1, y1, x2, y2, x3, y3;
	int i = numwalls;
	
	getWallCoords(s, &x3, &y3);
	while(s <= e)
		wall[s].x = x3, wall[s].y = y3, s++;

	while(--i >= 0)
	{
		getWallCoords(i, &x1, &y1, &x2, &y2);
		if ((x1 == x2 && y1 == y2) && ((x3 == x1 && y3 == y1) || (x3 == x2 && y3 == y2)))
			deletepoint(i);
	}
}



void getSectorWalls(int nSect, int* s, int *e)
{
	*s = sector[nSect].wallptr;
	*e = *s + sector[nSect].wallnum - 1;
}

void sectGetEdgeZ(int nSector, int* fz, int* cz)
{
	int s, e, x, y, z;
	getSectorWalls(nSector, &s, &e);
	
	*fz = sector[nSector].floorz;
	*cz = sector[nSector].ceilingz;
	while(s <= e)
	{
		getWallCoords(s++, &x, &y);
		if (fz && (z = getflorzofslope(nSector, x, y)) > *fz)
			*fz = z;
		
		if (cz && (z = getceilzofslope(nSector, x, y)) < *cz)
			*cz = z;
	}
}

void sectRotate(int nSect, int cx, int cy, int nAng, int flags)
{
	char rPoint = (klabs(nAng) != kAng90);
	int i, s, e;
	
	// rotate walls
	getSectorWalls(nSect, &s, &e);
	loopRotate(s, e, cx, cy, nAng, rPoint); // inside loop
	if ((flags & 0x02) && wall[s].nextwall >= 0)
	{
		// outer loop
		loopGetWalls(wall[s].nextwall, &s, &e);
		loopRotate(s, e, cx, cy, nAng, rPoint);
	}
	
	// rotate sprites
	for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
	{
		spritetype* pSpr = &sprite[i];
		
		posRotate(&pSpr->x, &pSpr->y, nAng, cx, cy, rPoint);
		if (pSpr->statnum != kStatMarker && pSpr->statnum != kStatPathMarker)
			pSpr->ang = (pSpr->ang + nAng) & kAngMask;
	}
}

void sectFlip(int nSect, int cx, int cy, int flags, int)
{
	char flipX = ((flags & 0x01) > 0);
	int i, s, e;
		
	// flip walls
	getSectorWalls(nSect, &s, &e);
	loopFlip(s, e, cx, cy, flags); // inside loop
	if ((flags & 0x02) && wall[s].nextwall >= 0)
	{
		// outer loop
		loopGetWalls(wall[s].nextwall, &s, &e);
		loopFlip(s, e, cx, cy, flags);
	}
	
	// flip sprites
	for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
	{
		spritetype* pSpr = &sprite[i];
		posFlip(&pSpr->x, &pSpr->y, cx, cy, flipX);
		if (pSpr->statnum != kStatMarker && pSpr->statnum != kStatPathMarker)
		{
			if (flipX)
				pSpr->ang = (kAng180 + (kAng360 - pSpr->ang)) & kAngMask;
			else
				pSpr->ang = (kAng360 - pSpr->ang) & kAngMask;
		}
	}
	
	if (flipX)
	{
		sector[nSect].ceilingstat ^= kSectFlipX;
		sector[nSect].floorstat ^= kSectFlipX;
	}
	else
	{
		sector[nSect].ceilingstat ^= kSectFlipY;
		sector[nSect].floorstat ^= kSectFlipY;
	}
}

int sectSplit(int nSect, int nWallA, int nWallB, POINT2D* points, int nPoints)
{
	int i, j, k, f, s, e, ax, ay, ls, le;
	int nWall, nWallOther, nWallNum;
	int nLoopNumA, nLoopNumB;
	
	if (nWallA < 0 || nWallB < 0)
		return -1;
	
	if (nWallA == nWallB)
		return -2;
	
	nLoopNumA	= loopnumofsector(nSect, nWallA);
	nLoopNumB	= loopnumofsector(nSect, nWallB);
	nWall		= nWallNum = numwalls+nPoints-1; // where to add more walls
	
	if (nLoopNumA == nLoopNumB)
	{
		if (numwalls + (nPoints<<1) >= kMaxWalls)	return -3;
		else if (numsectors + 1 >= kMaxSectors)		return -4;
	}
	else if (numwalls + nPoints >= kMaxWalls)	return -3;
	else if (numsectors >= kMaxSectors)			return -4;
	
	
	getSectorWalls(nSect, &s, &e);
	
	/* first fix up the new walls
	---------------------------------------------*/
	for (i = numwalls, j = 0; i <= nWallNum; i++, j++)
	{
		wall[i].x			= points[j].x;
		wall[i].y			= points[j].y;
		
		wall[i].cstat		= wall[s].cstat;
		wall[i].shade		= wall[s].shade;
		wall[i].xrepeat		= wall[s].xrepeat;
		wall[i].yrepeat		= wall[s].yrepeat;
		wall[i].picnum		= wall[s].picnum;
		wall[i].overpicnum	= wall[s].overpicnum;
		wall[i].point2		= i+1;
		wallDetach(i);
	}
	
	for (i = numwalls; i <= nWallNum; i++)
		fixrepeats(i);
	
	if (nLoopNumA == nLoopNumB)
	{
		/** NORMAL SPLIT
		//////////////////////////////////*/
		
		
		/* copy rest of loop next
		---------------------------------------------*/
		for (i = nWallB; i != nWallA; i = wall[i].point2)
		{
			memcpy(&wall[nWall], &wall[i], sizeof(walltype));
			wall[nWall].point2 = nWall+1;
			nWall++;
		}
		wall[nWall-1].point2 = numwalls;
		
		/* add other loops for 1st sector
		---------------------------------------------*/
		
		for (i = nLoopNumA, j = s; j <= e; j++)
		{
			if ((k = loopnumofsector(nSect, j)) != i && k != nLoopNumA)
			{
				i = k;
				loopGetWalls(j, &ls, &le);
				// copy the loop if at least one of walls is inside?
				while(ls <= le)
				{
					if (insideLoop(wall[ls].x, wall[ls].y, numwalls))
					{
						k = nWall;
						f = j;
						do
						{
							memcpy(&wall[nWall], &wall[f], sizeof(walltype));
							wall[nWall].point2 = nWall+1;
							f = wall[f].point2;
							nWall++;
						}
						while(f != j);
						wall[nWall-1].point2 = k;
						break;
					}
					
					ls++;
				}
			}
		}
		
		nWallOther = nWall;
		
		/* copy split points for other sector backwards
		---------------------------------------------*/
		for(i = nWallNum; i > numwalls; i--)
		{
			memcpy(&wall[nWall], &wall[i], sizeof(walltype));
			wall[nWall].point2 = nWall+1;
			wallDetach(nWall);
			nWall++;
		}
		
		/* copy rest of loop next
		---------------------------------------------*/
		for (i = nWallA; i != nWallB; i = wall[i].point2)
		{
			memcpy(&wall[nWall], &wall[i], sizeof(walltype));
			wall[nWall].point2 = nWall+1;
			nWall++;
		}
		wall[nWall-1].point2 = nWallOther;
		
		/* add other loops for 2nd sector
		---------------------------------------------*/
		for(i = nLoopNumA, j = s; j <= e; j++)
		{
			if ((k = loopnumofsector(nSect, j)) != i && k != nLoopNumA)
			{
				i = k;
				loopGetWalls(j, &ls, &le);
				// copy the loop if at least one of walls is inside?
				while(ls <= le)
				{
					if (insideLoop(wall[ls].x, wall[ls].y, nWallOther))
					{
						f = j;
						k = nWall;
						do
						{
							memcpy(&wall[nWall], &wall[f], sizeof(walltype));
							wall[nWall].point2 = nWall+1;
							f = wall[f].point2;
							nWall++;
						}
						while(f != j);
						wall[nWall-1].point2 = k;
						break;
					}
					
					ls++;
				}
			}
		}
		
		/* fix all next pointers on old sector line
		---------------------------------------------*/
		for (i = numwalls; i < nWall; i++)
		{
			if (wall[i].nextwall >= 0)
			{
				j = (i < nWallOther);
				wall[wall[i].nextwall].nextwall	= i;
				wall[wall[i].nextwall].nextsector = numsectors+j;
			}
		}
		
		/* set all next pointers on split
		---------------------------------------------*/
		for(i = numwalls; i < nWallNum; i++)
		{
			j = nWallOther + (nWallNum - 1 - i);
			wall[i].nextwall = j;
			wall[i].nextsector = numsectors+1;
			wall[j].nextwall = i;
			wall[j].nextsector = numsectors;
		}
		
		/* copy sector attributes & fix wall pointers
		---------------------------------------------*/
		sector[nSect].alignto = 0;
		
		memcpy(&sector[numsectors], &sector[nSect], sizeof(sectortype));
		sector[numsectors+0].wallnum = nWallOther - numwalls;
		sector[numsectors+0].wallptr = numwalls;
		
		memcpy(&sector[numsectors+1], &sector[nSect], sizeof(sectortype));
		sector[numsectors+1].wallnum = nWall - nWallOther;
		sector[numsectors+1].wallptr = nWallOther;
		
		
		/* fix sector of sprites
		---------------------------------------------*/
		while((i = headspritesect[nSect]) >= 0)
		{
			if (insideLoop(sprite[i].x, sprite[i].y, numwalls) == 0)
			{
				ChangeSpriteSect(i, numsectors+1);
			}
			else
			{
				ChangeSpriteSect(i, numsectors);
			}
		}
		
		j = nWall - numwalls;
		numwalls = nWall;
		numsectors += 2;
	
		/* detach old walls for safe deletesector
		---------------------------------------------*/
		for (i = s; i <= e; i++)
		{
			if (wall[i].nextwall >= 0)
				wallDetach(wall[i].nextwall);
			
			wallDetach(i);
		}
		deletesector(nSect);
		
		/* check pointers
		---------------------------------------------*/
		for(i = numwalls - j; i < numwalls; i++)
		{
			if (wall[i].nextwall >= 0)
				checksectorpointer(wall[i].nextwall, wall[i].nextsector);
			
			checksectorpointer(i, sectorofwall(i));
		}
		
		return 1;
	}
	else
	{
		/** LOOP JOINING
		//////////////////////////////////*/
		
		
		/* copy rest of loop next
		---------------------------------------------*/
		i = nWallB;
		do
		{
			memcpy(&wall[nWall], &wall[i], sizeof(walltype));
			wall[nWall].point2 = nWall+1;
			i = wall[i].point2;
			nWall++;
		}
		while(i != nWallB);
		
		/* copy split points for other sector backwards
		---------------------------------------------*/
		for(i = nWallNum; i > numwalls; i--)
		{
			memcpy(&wall[nWall], &wall[i], sizeof(walltype));
			wall[nWall].point2 = nWall+1;
			wallDetach(nWall);
			nWall++;
		}
		
		/* copy rest of loop next
		---------------------------------------------*/
		i = nWallA;
		do
		{
			memcpy(&wall[nWall], &wall[i], sizeof(walltype));
			wall[nWall].point2 = nWall+1;
			i = wall[i].point2;
			nWall++;
		}
		while(i != nWallA);
		wall[nWall-1].point2 = numwalls;
		
		
		/* add other loops for sector
		---------------------------------------------*/
		for (i = nLoopNumA, j = s; j <= e; j++)
		{
			if ((k = loopnumofsector(nSect, j)) == i) continue;
			else if (k == nLoopNumA || k == nLoopNumB)
				continue;

			i = k;
			f = j; k = nWall; //copy loop
			do
			{
				memcpy(&wall[nWall], &wall[f], sizeof(walltype));
				wall[nWall].point2 = nWall+1;
				f = wall[f].point2;
				nWall++;
			}
			while(f != j);
			wall[nWall-1].point2 = k;
		}
		
		/* fix all next pointers on old sector line
		---------------------------------------------*/
		for (i = numwalls; i < nWall; i++)
		{
			if (wall[i].nextwall >= 0)
			{
				wall[wall[i].nextwall].nextwall	= i;
				wall[wall[i].nextwall].nextsector = numsectors;
			}
		}
		
		/* copy sector attributes & fix wall pointers
		---------------------------------------------*/
		sector[numsectors] = sector[nSect];
		sector[numsectors].wallptr = numwalls;
		sector[numsectors].wallnum = nWall - numwalls;
		
		
		/* fix sector of sprites
		---------------------------------------------*/
		while((i = headspritesect[nSect]) >= 0)
			ChangeSpriteSect(i, numsectors);
		
		j = nWall - numwalls;
		numwalls = nWall;
		numsectors++;
		
		/* detach old walls for safe deletesector
		---------------------------------------------*/
		for (i = s; i <= e; i++)
		{
			if (wall[i].nextwall >= 0)
				wallDetach(wall[i].nextwall);
			
			wallDetach(i);
		}
		deletesector(nSect);
		
		/* check pointers
		---------------------------------------------*/
		for(i = numwalls - j; i < numwalls; i++)
		{
			if (wall[i].nextwall >= 0)
				checksectorpointer(wall[i].nextwall, wall[i].nextsector);
			
			checksectorpointer(i, numsectors-1);
		}
		
		return 2;
	}
	
	return 0;
}

int sectSplit(int nSect, POINT2D* points, int nPoints)
{
	POINT2D *first = &points[0], *last = &points[nPoints-1];
	int nWallA = -1, nWallB = -1;
	int i, s, e;
	
	getSectorWalls(nSect, &s, &e);
	for (i = s; i <= e; i++)
	{
		if (nWallA < 0)
		{
			if (wall[i].x == first->x && wall[i].y == first->y)
				nWallA = i;
		}
		
		if (nWallB < 0)
		{
			if (wall[i].x == last->x && wall[i].y == last->y)
				nWallB = i;
		}
		
		if (nWallA >= 0 && nWallB >= 0)
			break;
	}
	
	return sectSplit(nSect, nWallA, nWallB, points, nPoints);
}

int sectCloneMarker(XSECTOR* pXOwner, spritetype* pWich)
{
	int nSpr; spritetype* pSpr;
	if ((nSpr = InsertSprite(pWich->sectnum, kStatMarker)) < 0)
		return -1;
	
	pSpr = &sprite[nSpr];
	memcpy(pSpr, pWich, sizeof(spritetype));
	pSpr->owner = pXOwner->reference;
	pSpr->index = nSpr;
	pSpr->extra = -1;
	
	switch(pSpr->type)
	{
		case kMarkerWarpDest:
		case kMarkerAxis:
		case kMarkerOff:
			pXOwner->marker0 = nSpr; 
			break;
		case kMarkerOn:
			pXOwner->marker1 = nSpr;
			break;
	}
	
	return nSpr;
}

char sectClone(int nSrc, int nDstS, int nDstW, int flags)
{
	int i, s, e, nWall = nDstW;
	XSECTOR* pXSect;
	
	// clone walls
	getSectorWalls(nSrc, &s, &e);
	for (i = s; i <= e; i++, nWall++)
	{
		wall[nWall] = wall[i];
		if (flags & 0x04)
			fixXWall(nWall);
		
		wall[nWall].point2 += nDstW-s;
		if (wall[nWall].nextwall >= 0)
		{
			wall[nWall].nextsector	+= nDstS-nSrc;
			wall[nWall].nextwall	+= nDstW-s;
		}
	}
	
	if (nWall > nDstW)
	{
		// clone sector
		sector[nDstS]			= sector[nSrc];
		sector[nDstS].wallnum	= nWall-nDstW;
		sector[nDstS].wallptr	= nDstW;
		
		if (sector[nDstS].extra > 0) // fix xsector
		{
			if (flags & 0x04)
				fixXSector(nDstS);
			
			pXSect = &xsector[sector[nDstS].extra];
			
			if (flags & 0x03) // fix marker(s)
			{
				if (pXSect->marker0 >= 0 && sprite[pXSect->marker0].statnum == kStatMarker)
				{
					if ((i = sectCloneMarker(pXSect, &sprite[pXSect->marker0])) >= 0 && (flags & 0x02))
					{
						if (inside(sprite[pXSect->marker0].x, sprite[pXSect->marker0].y, nDstS))
							ChangeSpriteSect(i, nDstS);
					}
				}
				
				if (pXSect->marker1 >= 0 && sprite[pXSect->marker1].statnum == kStatMarker)
				{
					if ((i = sectCloneMarker(pXSect, &sprite[pXSect->marker1])) >= 0 && (flags & 0x02))
					{
						if (inside(sprite[pXSect->marker1].x, sprite[pXSect->marker1].y, nDstS))
							ChangeSpriteSect(i, nDstS);
					}
				}
			}
		}

		if (flags & 0x01)
		{
			// clone sprites
			for (i = headspritesect[nSrc]; i >= 0; i = nextspritesect[i])
			{
				// ignore these in this loop (even if don't own it)
				if (sprite[i].statnum == kStatMarker)
					continue;
				
				if ((s = InsertSprite(nDstS, sprite[i].statnum)) < 0)
					break;
				
				sprite[s]			= sprite[i];
				sprite[s].sectnum	= nDstS;
				sprite[s].index		= s;
				
				if (flags & 0x04)
					fixXSprite(s);
			}
		}
	}
	
	return 1;
}

short sectCstatAdd(int nSect, short cstat, int objType)
{
	if (objType == OBJ_FLOOR)
	{
		setCstat(TRUE, &sector[nSect].floorstat, cstat);
		return sector[nSect].floorstat;
	}
	else
	{
		setCstat(TRUE, &sector[nSect].ceilingstat, cstat);
		return sector[nSect].ceilingstat;
	}
}

short sectCstatRem(int nSect, short cstat, int objType)
{
	if (objType == OBJ_FLOOR)
	{
		setCstat(FALSE, &sector[nSect].floorstat, cstat);
		return sector[nSect].floorstat;
	}
	else
	{
		setCstat(FALSE, &sector[nSect].ceilingstat, cstat);
		return sector[nSect].ceilingstat;
	}
}

short sectCstatToggle(int nSect, short cstat, int objType) {
	
	if (objType == OBJ_FLOOR)
	{
		sector[nSect].floorstat ^= cstat;
		return sector[nSect].floorstat;
	}
	else
	{
		sector[nSect].ceilingstat ^= cstat;
		return sector[nSect].ceilingstat;
	}
}

short sectCstatGet(int nSect, int objType)
{
	if (objType == OBJ_FLOOR) 
		return sector[nSect].floorstat;
	
	return sector[nSect].ceilingstat;
	
}

short sectCstatSet(int nSect, short cstat, int objType)
{
	if (objType == OBJ_FLOOR)
	{
		sector[nSect].floorstat = cstat;
		return sector[nSect].floorstat;
	}
	else
	{
		sector[nSect].ceilingstat = cstat;
		return sector[nSect].ceilingstat;
	}
	
}

void sectDetach(int nSect)
{
	int s, e;
	getSectorWalls(nSect, &s, &e);

	while(s <= e)
	{
		if (wall[s].nextwall >= 0)
		{
			wallDetach(wall[s].nextwall);
			wallDetach(s);
		}
		
		s++;
	}
}

void sectAttach(int nSect)
{
	int s, e, n;
	getSectorWalls(nSect, &s, &e);

	while(s <= e)
	{
		if (wall[s].nextwall < 0 && (n = findNextWall(s)) >= 0)
		{
			wallAttach(s, sectorofwall(n), n);
			wallAttach(n, nSect, s);
		}
		
		s++;
	}
}

char pointOnLine(int x, int y, int x1, int y1, int x2, int y2)
{
	if ((x1 <= x && x <= x2) || (x2 <= x && x <= x1))
		if ((y1 <= y && y <= y2) || (y2 <= y && y <= y1))
			if ((x-x1)*(y2-y1) == (y-y1)*(x2-x1))
				return 1;
			
	return 0;
}

char pointOnWallLine(int nWall, int x, int y)
{
	int x1, y1, x2, y2;
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	if (x1 != x || y1 != y)
		if (x2 != x || y2 != y)
			return pointOnLine(x, y, x1, y1, x2, y2);

	return 0;
}

char pointOnWallLine(int x, int y)
{
	int i = numwalls;
	while(--i >= 0 && !pointOnWallLine(i, x, y));
	return (i >= 0);
}

int findWallAtPos(int x, int y)
{
	int i = numwalls;
	while(--i >= 0 && (wall[i].x != x || wall[i].y != y));
	return i;
}

void insertPoint_OneSide(int nWall, int x, int y)
{
	int nSect;
	int i;
	
	nSect = sectorofwall(nWall);
	sector[nSect].wallnum++;
	for(i=nSect+1;i<numsectors;i++)
		sector[i].wallptr++;

	movewalls(nWall+1, 1);
	memcpy(&wall[nWall+1], &wall[nWall], sizeof(walltype));
	wall[nWall].point2 = nWall+1;
	wall[nWall+1].x = x;
	wall[nWall+1].y = y;
	
	fixrepeats(nWall);
	
	if ((wall[nWall].cstat & kWallFlipMask) == 0)
		AlignWalls(nWall, GetWallZPeg(nWall), wall[nWall].point2, GetWallZPeg(wall[nWall].point2), wall[nWall].picnum);
	
	fixrepeats(nWall+1);
	fixXWall(nWall+1);
}

int insertPoint(int nWall, int x, int y)
{
	int i, j;
	
	insertPoint_OneSide(nWall, x, y);
	if (wall[nWall].nextwall >= 0)
	{
		i = wall[nWall].nextwall;
		insertPoint_OneSide(i, x, y);
		j = wall[i].nextwall;
		
        wall[j+0].nextwall = i+1;
        wall[j+1].nextwall = i+0;
        wall[i+0].nextwall = j+1;
        wall[i+1].nextwall = j+0;
		
		return 2;
	}
	
	return 1;
}

void wallDetach(int nWall)
{
	if (nWall >= 0)
	{
		wall[nWall].nextsector	= -1;
		wall[nWall].nextwall	= -1;
	}
}

void wallAttach(int nWall, int nNextS, int nNextW)
{
	if (nWall >= 0)
	{
		wall[nWall].nextsector	= nNextS;
		wall[nWall].nextwall	= nNextW;
	}
}


double getWallLength(int nWall, int nGrid)
{
	int nLen = getWallLength(nWall);
	if (nGrid)
		return (double)(nLen / (2048>>nGrid));
	
	return (double)nLen;
}


void getWallCoords(int nWall, int* x1, int* y1, int* x2, int* y2)
{
	if (x1)	*x1 = wall[nWall].x;
	if (y1)	*y1 = wall[nWall].y;
	if (x2)	*x2 = wall[wall[nWall].point2].x;
	if (y2)	*y2 = wall[wall[nWall].point2].y;
}

short wallCstatAdd(int nWall, short cstat, char nextWall)
{
	setCstat(1, &wall[nWall].cstat, cstat);
	if (nextWall && wall[nWall].nextwall >= 0)
		setCstat(1, &wall[wall[nWall].nextwall].cstat, cstat);

	return wall[nWall].cstat;
}

short wallCstatRem(int nWall, short cstat, char nextWall)
{
	setCstat(0, &wall[nWall].cstat, cstat);
	if (nextWall && wall[nWall].nextwall >= 0)
		setCstat(0, &wall[wall[nWall].nextwall].cstat, cstat);
	
	return wall[nWall].cstat;
}

short wallCstatToggle(int nWall, short cstat, char nextWall)
{
	wall[nWall].cstat ^= cstat;
	if (nextWall && wall[nWall].nextwall >= 0)
	{
		if ((wall[nWall].cstat & cstat))
		{
			if (!(wall[wall[nWall].nextwall].cstat & cstat))
				wallCstatAdd(wall[nWall].nextwall, cstat, 0);
		}
		else
		{
			if ((wall[wall[nWall].nextwall].cstat & cstat))
				wallCstatRem(wall[nWall].nextwall, cstat, 0);
		}
	}
	
	return wall[nWall].cstat;
}

char insideLoop(int x, int y, int nStartWall)
{
	int x1, y1, x2, y2;
	int i = nStartWall;
	int c;
	
	c = clockdir(i);
	do
	{
		getWallCoords(i, &x1, &y1, &x2, &y2);
		
		if (x1 >= x || x2 >= x)
		{
			if (y1 > y2)
			{
				swapValues(&x1, &x2);
				swapValues(&y1, &y2);
			}
			
			if (y1 <= y && y2 > y)
			{
				if (x1*(y-y2)+x2*(y1-y) <= x*(y1-y2))
					c ^= 1;
			}
		}
		
		i = wall[i].point2;
	}
	while(i != nStartWall);
	return c;
}

int getSectorHeight(int nSector)
{
	int fz, cz;
	sectGetEdgeZ(nSector, &fz, &cz);
	return fz-cz;
}

void setFirstWall(int nSect, int nWall)
{
	int start, length, shift;
	int i, j, k;
	walltype tempWall;

	// rotate the walls using the shift copy algorithm

	start = sector[nSect].wallptr;
	length = sector[nSect].wallnum;

	dassert(nWall >= start && nWall < start + length);
	shift = nWall - start;

	if (shift == 0)
		return;

	i = k = start;

	for (int n = length; n > 0; n--)
	{
		if (i == k)
			tempWall = wall[i];

		j = i + shift;
		while (j >= start + length)
			j -= length;

		if (j == k)
		{
			wall[i] = tempWall;
			i = ++k;
		}
		else
		{
			wall[i] = wall[j];
			i = j;
		}
	}

	for (i = start; i < start + length; i++)
	{
		if ((wall[i].point2 -= shift) < start)
			wall[i].point2 += length;

		if (wall[i].nextwall >= 0)
			wall[wall[i].nextwall].nextwall = (short)i;
	}
	
	CleanUp();
}

char loopInside(int nSect, POINT2D* pPoint, int c, char full)
{
	int nPrev, nAng, nLen, i, r;
	POINT2D *a = pPoint, *b;
	POSOFFS pos;

	for (i = 0; i < c; i++)
	{
		if (!inside(a[i].x, a[i].y, nSect))
			return 0;
	}
	
	if (full)
	{
		for (i = 0; i < c; i++)
		{
			if (i < c - 1)
			{
				a = &pPoint[i]; b = &pPoint[i+1];
			}
			else
			{
				a = &pPoint[0]; b = &pPoint[c-1];
			}
			
			nAng = (getangle(b->x - a->x, b->y - a->y) + kAng90) & kAngMask;
			nLen = exactDist(b->x - a->x, b->y - a->y);
			pos.New(nAng, a->x, a->y);
			
			while(pos.Distance() < nLen)
			{
				if (!inside(pos.x, pos.y, nSect))
					return 0;
				
				r = ClipLow(nLen >> 4, 2);
				if (r % 2)
					r++;
				
				nPrev = pos.Distance();
				while(nPrev == pos.Distance())
					pos.Right(r+=2);
			}
		}
	}
	
	return 1;
}

char insidePoints(int nSect, POINT2D* point, int c)
{
	int s, e;
	getSectorWalls(nSect, &s, &e);
	while(s <= e)
	{
		if (!insidePoints(wall[s].x, wall[s].y, point, c))
			return 0;
		
		s++;
	}
	
	return 1;
}


char insidePoints(int x, int y, POINT2D* point, int c)
{
	int i, x1, y1, x2, y2;
	unsigned int cnt = 0;
	
	for (i = 0; i < c; i++)
	{
		if (i < c - 1)
		{
			x1 = point[i].x-x,	x2 = point[i+1].x-x;
			y1 = point[i].y-y,	y2 = point[i+1].y-y;
		}
		else
		{
			x1 = point[0].x-x,	x2 = point[c-1].x-x;
			y1 = point[0].y-y,	y2 = point[c-1].y-y;
		}
		
		if ((y1^y2) < 0)
			cnt ^= ((x1^x2) >= 0) ? (x1) : ((x1*y2-x2*y1)^y2);
	}
	
	return ((cnt >> 31) > 0);
}

char sectWallsInsidePoints(int nSect, POINT2D* point, int c)
{
	int s, e;
	
	// make sure all points inside a sector
	///////////////////////
	
	if (loopInside(nSect, point, c, 1))
	{
		// make sure points didn't cover island walls
		///////////////////////
		
		getSectorWalls(nSect, &s, &e);
		while(s <= e)
		{
			if (insidePoints(wall[s].x, wall[s].y, point, c) == 1)
				return 1;
			
			s++;
		}
		
		return 0;
	}
	
	return 1;
}

int insertLoop(int nSect, POINT2D* pInfo, int nCount, walltype* pWModel, sectortype* pSModel)
{
	walltype* pWall; sectortype* pSect;
	int nStartWall = (nSect >= 0) ? sector[nSect].wallptr : numwalls;
	int nWall = nStartWall, i = 0; char insertInside = (nSect >= 0);
	int t;
		
	if (!insertInside)
	{
		// create new white sector
		pSect = &sector[numsectors];

		if (pSModel)
		{
			memcpy(pSect, pSModel, sizeof(sectortype));
		}
		else
		{
			memset(pSect, 0, sizeof(sectortype));
			
			pSect->floorz	= 8192<<2;
			pSect->ceilingz	= -pSect->floorz;
			pSect->extra	= -1;
		}
		
		pSect->wallptr	= nWall;
		pSect->wallnum	= nCount;		
		numsectors++;
	}
	
	movewalls(nWall, nCount); // increases numwalls automatically
	
	while(i < nCount)
	{
		pWall = &wall[nWall];
		
		if (pWModel)
		{
			memcpy(pWall, pWModel, sizeof(walltype));
		}
		else
		{
			memset(pWall, 0, sizeof(walltype));
			pWall->xrepeat = pWall->yrepeat = 8;
			pWall->extra = -1;
			fixrepeats(nWall);
		}

		pWall->point2		= ++nWall;
		pWall->nextwall		= -1;
		pWall->nextsector	= -1;
		
		pWall->x			= pInfo[i].x;
		pWall->y			= pInfo[i].y;
		i++;
	}
	
	pWall->point2 = nStartWall;
	
	if (clockdir(nStartWall) == 1)
		flipwalls(nStartWall, nStartWall + nCount);
	
	if (insertInside)
	{
		t = nSect;
		sector[t].wallnum += nCount;
		while(t++ < numsectors)
			sector[t].wallptr += nCount;
		
		if (clockdir(nStartWall) == 0)
			flipwalls(nStartWall, nStartWall + nCount);
		
		for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
		{
			spritetype* pSpr = &sprite[i]; t = nSect;
			if (FindSector(pSpr->x, pSpr->y, pSpr->z, &t) && t != nSect)
			{
				ChangeSpriteSect(pSpr->index, t);
				i = headspritesect[nSect];
			}
		}

		setFirstWall(nSect, nStartWall + nCount); // fix wallptr, so slope dir won't change
		nStartWall+=(sector[nSect].wallnum-nCount);
	}
	
	return nStartWall;
}

void insertPoints(WALLPOINT_INFO* pInfo, int nCount)
{
	for (int i = 0; i < nCount; i++)
		insertpoint(pInfo[i].w, pInfo[i].x, pInfo[i].y);
}

int makeSquareSector(int ax, int ay, int area)
{
	int nWall = -1;
	LOOPSHAPE shape(kLoopShapeSquare, -1, ax, ay); // create square sector
	shape.Setup(ax + area, ay + area, NULL);
	switch(shape.StatusGet())
	{
		default:
			nWall = shape.Insert();
			// no break;
		case -1:
		case -2:
		case -4:
		case -5:
			shape.Stop();
			break;
	}
	
	return (nWall >= 0) ? sectorofwall(nWall) : nWall;
}

int redSectorCanMake(int nStartWall)
{
	
	int addwalls = -1;
	if (wall[nStartWall].nextwall >= 0) return -1;
	else if (numsectors >= kMaxSectors) return -4;
	else if ((addwalls = whitelinescan(nStartWall)) < numwalls) return -2;
	else if (addwalls >= kMaxWalls) return -3;
	else return addwalls;
}


int redSectorMake(int nStartWall)
{
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
	
	for (i = swal; i <= ewal; i++)
	{
		// we for sure don't need cstat inheriting for walls
		wall[i].cstat = wall[wall[i].nextwall].cstat = 0;
		fixrepeats(wall[i].nextwall);
		fixrepeats(i);
	}
	numwalls = addwalls;
	numsectors++;
	return 0;
}

int redSectorMerge(int nThis, int nWith)
{
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

void setCstat(BOOL enable, short* pStat, int nStat)
{
	if (enable)
	{
		if (!(*pStat & nStat))
			*pStat |= (short)nStat;
	}
	else if ((*pStat & nStat))
		*pStat &= ~(short)nStat;
}

void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* zt, int* zb, char flags)
{
	int t, cx, cy, xoff = 0;
	int nPic = pSpr->picnum, nAng = pSpr->ang;
	int xrep = pSpr->xrepeat, wh = tilesizx[nPic];
	
	*x1 = *x2 = pSpr->x;
	*y1 = *y2 = pSpr->y;
	
	if (flags & 0x01)
	{
		xoff = panm[nPic].xcenter;
	}
	
	if (flags & 0x02)
	{
		xoff += pSpr->xoffset;
	}
	
	if (pSpr->cstat & kSprFlipX)
		xoff = -xoff;
	
	cx = sintable[nAng & kAngMask] * xrep;
	cy = sintable[(nAng + kAng90 + kAng180) & kAngMask] * xrep;
	t = (wh>>1)+xoff;
	
	*x1 -= mulscale16(cx, t);	*x2 = *x1 + mulscale16(cx, wh);
	*y1 -= mulscale16(cy, t);	*y2 = *y1 + mulscale16(cy, wh);
	
	if (zt || zb)
	{
		int tzt, tzb;
		GetSpriteExtents(pSpr, &tzt, &tzb);
		if (zt)
			*zt = tzt;
		
		if (zb)
			*zb = tzb;
	}
}


void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, char flags)
{
	int t, i, cx, cy, xoff = 0, yoff = 0;
	int nPic = pSpr->picnum, nAng = pSpr->ang & kAngMask;
	int xrep = pSpr->xrepeat, wh = tilesizx[nPic];
	int yrep = pSpr->yrepeat, hg = tilesizy[nPic];
	int nCos = sintable[(nAng + kAng90) & kAngMask];
	int nSin = sintable[nAng];
	
	*x1 = *x2 = *x3 = *x4 = pSpr->x;
	*y1 = *y2 = *y3 = *y4 =  pSpr->y;
	
	if (flags & 0x01)
	{
		xoff = panm[nPic].xcenter;
		yoff = panm[nPic].ycenter;
	}
	
	if ((flags & 0x02) && (pSpr->cstat & kSprRelMask) != kSprSloped)
	{
		xoff += pSpr->xoffset;
		yoff += pSpr->yoffset;
	}
	
	if (pSpr->cstat & kSprFlipX)
		xoff = -xoff;
	
	if (pSpr->cstat & kSprFlipY)
		yoff = -yoff;
	
	if (!(flags & 0x04))
	{
		if (wh % 2) wh++;
		if (hg % 2) hg++;
	}
	
	cx = ((wh>>1)+xoff)*xrep;
	cy = ((hg>>1)+yoff)*yrep;
		
	*x1 += dmulscale16(nSin, cx, nCos, cy);
	*y1 += dmulscale16(nSin, cy, -nCos, cx);
	
	t = wh*xrep;
	*x2 = *x1 - mulscale16(nSin, t);
	*y2 = *y1 + mulscale16(nCos, t);
	
	t = hg*yrep;
	i = -mulscale16(nCos, t);	*x3 = *x2 + i; *x4 = *x1 + i;
	i = -mulscale16(nSin, t);	*y3 = *y2 + i; *y4 = *y1 + i;
}

void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, int* zt, int* zb, char flags)
{
	GetSpriteExtents(pSpr, x1, y1, x2, y2, x3, y3, x4, y4, flags);
		
	if (zt || zb)
	{
		int tzt, tzb;
		GetSpriteExtents(pSpr, &tzt, &tzb);
		if (zt)
			*zt = tzt;
		
		if (zb)
			*zb = tzb;
	}
}

void ceilGetEdgeZ(int nSector, int* zBot, int* zTop)
{
	int s, e, x, y, z;
	sectortype* pSect = &sector[nSector];
	
	*zTop = *zBot = pSect->ceilingz;
	if ((pSect->ceilingstat & kSectSloped) && pSect->ceilingslope)
	{
		getSectorWalls(nSector, &s, &e);
		
		while(s <= e)
		{
			getWallCoords(s++, &x, &y);
			z = getceilzofslope(nSector, x, y);
			if (zBot && z < *zBot) *zBot = z;
			if (zTop && z > *zTop) *zTop = z;
		}
	}
}


void floorGetEdgeZ(int nSector, int* zBot, int* zTop)
{
	sectortype* pSect = &sector[nSector];
	int s, e, x, y, z;
	
	*zTop = *zBot = pSect->floorz;
	if ((pSect->floorstat & kSectSloped) && pSect->floorslope)
	{
		getSectorWalls(nSector, &s, &e);
		
		while(s <= e)
		{
			getWallCoords(s++, &x, &y);
			z = getflorzofslope(nSector, x, y);
			if (zBot && z > *zBot) *zBot = z;
			if (zTop && z < *zTop) *zTop = z;
		}
	}
}

char fixXSprite(int nID)
{
	int nOld;
	if ((nOld = sprite[nID].extra) > 0)
	{
		sprite[nID].extra						= dbInsertXSprite(nID);
		xsprite[sprite[nID].extra]				= xsprite[nOld];
		xsprite[sprite[nID].extra].reference	= nID;
		return 1;
	}
	
	return 0;
}

char fixXSector(int nID)
{
	int nOld;
	if ((nOld = sector[nID].extra) > 0)
	{
		sector[nID].extra						= dbInsertXSector(nID);
		xsector[sector[nID].extra]				= xsector[nOld];
		xsector[sector[nID].extra].reference	= nID;
		return 1;
	}
	
	return 0;
}

char fixXWall(int nID)
{
	int nOld;
	if ((nOld = wall[nID].extra) > 0)
	{
		wall[nID].extra							= dbInsertXWall(nID);
		xwall[wall[nID].extra]					= xwall[nOld];
		xwall[wall[nID].extra].reference		= nID;
		return 1;
	}
	
	return 0;
}