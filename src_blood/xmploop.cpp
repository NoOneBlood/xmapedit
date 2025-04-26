/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2024: Originally written by NoOne.
// New wall loop or sector creation class
// Adapted from overheadeditor() function in build.c
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
#include "xmpmaped.h"
#include "aadjust.h"
#include "db.h"

NAMED_TYPE gLoopBuildErrors[] =
{
	{-1, "Out of free walls"},
	{-2, "Out of free sectors"},
	{-3, "Must insert a point there first"},
	{-4, "Cannot draw over red lines"},
	{-5, "Cannot draw over old lines"},
	{-6, "Lines intersection"},
	{-7, "Wall or sector inside this loop"},
	{-999, NULL},
};

int findSplitWallAtPos(int nSect, int x, int y)
{
    int ms, me, s, e;
    int n = -1;
    
    getSectorWalls(nSect, &s, &e);
    sectLoopMain(nSect, &ms, &me);
	while(s <= e)
    {
        if (wall[s].x == x && wall[s].y == y)
        {
            if (irngok(s, ms, me)) // main loop is priority!
                return s;
            
            n = s;
        }
        
        s++;
    }
    
    return n;
}

void LOOPBUILD::Stop()
{
	if (point)
		free(point);
}

void LOOPBUILD::Start()
{
	Stop();
	point = (POINT2D*)malloc(sizeof(POINT2D*)<<1);
	dassert(point != NULL);
	
	pModelW = NULL; pModelS = NULL;
	numPoints = numAutoPoints = 0;
    destSect = -1;
    
    autoTimer = 0;
    autoState = 0;
    status = 0;
}

void LOOPBUILD::AddPoint(int x, int y, int nPoint)
{
	const int nSiz = sizeof(POINT2D)<<1;
	point = (POINT2D*)realloc(point, nSiz*(numPoints+1));
	dassert(point != NULL);
	
	if (!rngok(nPoint, 0, numPoints)) nPoint = numPoints;
	else memcpy(&point[nPoint+1], &point[nPoint], nSiz*(numPoints-nPoint));
	UpdPoint(x, y, numPoints);
	numPoints++;
	UpdPoint(x, y, numPoints);
}

void LOOPBUILD::RemPoint(int nPoint)
{
	const int nSiz = sizeof(POINT2D)<<1;
	if (numPoints <= 0)
		return;
	
	if (rngok(nPoint, 0, numPoints))
		memcpy(&point[nPoint], &point[nPoint+1], nSiz*(numPoints-nPoint));

	numPoints--;
	point = (POINT2D*)realloc(point, nSiz*ClipLow(numPoints, 1));
}

void LOOPBUILD::RemPointsTail(int nCount)
{
    const int nSiz = sizeof(POINT2D)<<1;
    if (numPoints <= 0 || nCount <= 0)
        return;
    
    numPoints-=nCount;
    point = (POINT2D*)realloc(point, nSiz*ClipLow(numPoints, 1));
}

void LOOPBUILD::RemAutoPoints(void)
{
    numPoints += numAutoPoints;
    RemPointsTail(numAutoPoints);
    numAutoPoints = 0;
}

int LOOPBUILD::Make()
{
	POINT2D *f = First(), *l = Last()+1;
    int nSect = -1, i, s, e;
    
	switch(status)
	{
		case 1:
		case 2:
			if (destSect < 0) break;
            else if ((s = findSplitWallAtPos(destSect, f->x, f->y)) < 0) break;
            else if ((e = findSplitWallAtPos(destSect, l->x, l->y)) < 0) break;
            else return sectSplit(destSect, s, e, point, numPoints+1);
        case 6: // autocompletion
            numPoints += numAutoPoints; // expand numpoints
            numAutoPoints = 0;
            // no break
		case 3:
		case 4:
		case 5:
        case 7:
			if ((i = insertLoop(destSect, point, numPoints, pModelW, pModelS)) >= 0)
			{	
				nSect = sectorofwall(i);
				loopGetWalls(i, &s, &e);
				while(s <= e)
				{
					checksectorpointer(s, nSect);
					fixrepeats(s);
					s++;
				}
				
                if (status == 7)
                    redSectorMake(i);
                
				return i;
			}
			break;
	}
	
	return -1;
}

char LOOPBUILD::DoAutoCompletion(IDLIST* pList, int wS, int wE, char d)
{
	int i, j, w, x1, y1, x2, y2, x3, y3, x4, y4;
    int32_t* p = pList->First(), *p2;
    int onumpoints = numPoints;
    char flip = 0;

	while(*p >= 0 && *p != wE) p++;
	if (*p < 0)
		return 0;
	
	for (i = 0; i < numPoints-1; i++)
	{
		x1 = point[i+0].x;	x2 = point[i+1].x;
		y1 = point[i+0].y;	y2 = point[i+1].y;
		
		for (p2 = p; *p2 >= 0; p2++)
		{
			if ((j = pointOnWallLine(*p2, x1, y1)) != 1 && j != 2)
			{
				getWallCoords(*p2, &x3, &y3, &x4, &y4);
				if (kintersection(x1, y1, x2, y2, x3, y3, x4, y4, NULL, NULL))
					return 0;
			}
		}
	}
    
	while( 1 )
	{
		w = (d) ? *(++p) : *(--p);
		if (w < 0 || w == wS)
			break;
        
        i = numPoints;
        while(--i >= 0)
        {
            if (point[i].x == wall[w].x && point[i].y == wall[w].y)
            {
                RemPointsTail(numPoints - onumpoints);
                return 0;
            }
        }
        
		AddPoint(wall[w].x, wall[w].y);
	}
	
	if ((flip = isClockDir(point, numPoints)) == 1)
		flipPoints(point, numPoints);
    
	i = numPoints;
	while(--i >= onumpoints)
	{
		x1 = point[i-1].x;	x2 = point[i-0].x;
		y1 = point[i-1].y;	y2 = point[i-0].y;
		if ((w = findWallAtPos(x1, y1, x2, y2)) >= 0)
		{
            if (flip) flipPoints(point, numPoints);
            RemPointsTail(numPoints - onumpoints);
            return 0;
		}
		
		for (p2 = pList->First(); *p2 >= 0; p2++)
		{
			getWallCoords(*p2, &x3, &y3, &x4, &y4);
			if (!dintersection(x1, y1, x2, y2, x3, y3, x4, y4, NULL, NULL))
                continue;
    
            if (flip) flipPoints(point, numPoints);
            RemPointsTail(numPoints - onumpoints);
            return 0;
		}
	}
    
    if (flip) flipPoints(point, numPoints);
	return 1;
}

char LOOPBUILD::SetupAutoCompletion(void)
{
    int x1 = point[0].x, x2 = point[numPoints-1].x;
    int y1 = point[0].y, y2 = point[numPoints-1].y;
    int onumpoints = numPoints;
    int s, e;
    
	if ((s = findSolidWallAtPos(x1, y1)) < 0
		|| (e = findSolidWallAtPos(x2, y2)) < 0)
			return 0;

	IDLIST list(1);
	
	// Can walls reach each other?
	if (collectOuterWalls(s, &list) && list.Exists(e) && list.Exists(s))
	{
        if (numAutoPoints)
            RemAutoPoints();
        
        if (DoAutoCompletion(&list, s, e, 1) || DoAutoCompletion(&list, s, e, 0))
        {
            if (SetupPrivate(x1, y1) == 5) // ready for new solid sector connected to another solid sector
            {
                numAutoPoints = numPoints - onumpoints;
                numPoints = onumpoints; // manually inserted points remains old
                return 1;
            }
        }
	}

	return 0;
}



int LOOPBUILD::SetupPrivate(int x, int y)
{
	POINT2D *first, *last; char atFirstPoint;
    int x1, y1, x2, y2, x3, y3, x4, y4, tx, ty, lna, lnb;
	int i, j, s, e, ls, le;

	if (numPoints < 2)
    {
		destSect = -1;
	}
    else if (destSect < 0)
    {
        last = Last();
        if (x == last->x && y == last->y)
        {
            if (autoState == 0 // not tried
                && (autoState = SetupAutoCompletion()) == 0)
                        autoState = 2; // tried and failed
            
            if (autoState == 1) // success
            {
                if (!autoTimer)
                    autoTimer = totalclock + 64; // delayed drawing?
                
                return 6;
            }
        }
        else
        {
            autoTimer = 0;
            autoState = 0;
        }
    }
    
    if (numAutoPoints)
        RemAutoPoints();
    
	if (numPoints > 0)
		UpdPoint(x, y, numPoints); // this updates tail point (tail != last)
    
	if (numwalls+1 >= kMaxWalls)        return -1;
	if (pointOnWallLine(x, y) == 3)		return -3;
	if (numPoints == 0)					return  0;  // ready for point insertion
    
    first = First(), last = Last(); // these must be updated after potential autocompletion
    atFirstPoint = (first->x == x && first->y == y);
    cross.w = -1;
    
	if (numPoints >= 2)
	{
		j = numPoints;
		i = j;
		
		x1 = point[j-0].x;	x2 = point[j-1].x;
		y1 = point[j-0].y;	y2 = point[j-1].y;
		
		while(--i > 0)
		{
			x3 = point[i-1].x;	x4 = point[i-0].x;
			y3 = point[i-1].y;	y4 = point[i-0].y;
			
			if ((x3 == x2 && y3 == y2) || (x3 == x1 && y3 == y1));
			else if ((x4 == x2 && y4 == y2) && (x4 == x1 && y4 == y1));
			else if (pointOnLine(x1, y1, x3, y3, x4, y4))
			{
				cross.w = i, cross.x = x1, cross.y = y1;
				return -5;
			}
			
			if (kintersection(x1, y1, x2, y2, x3, y3, x4, y4, &cross.x, &cross.y))
			{
				if (cross.x == first->x && cross.y == first->y
					&& x1 == first->x && y1 == first->y)
						continue;
						
				cross.w = i;
				return -6;
			}
		}
	}

	if (!atFirstPoint)
	{
		if (numPoints > 0)
		{
            for (i = 0; i < numPoints; i++)
			{
				if (point[i].x == x && point[i].y == y)
					return -5; // drawing over inserted points
			}
			
			for(i = 0, j = 0; i < numwalls; i++)
			{
				if (wall[i].nextwall >= 0)
				{
					getWallCoords(i, &x1, &y1, &x2, &y2);
					if (x1 == x && y1 == y && x2 == last->x && y2 == last->y) j = 1;
					if (x1 == last->x && y1 == last->y && x2 == x && y2 == y) j = 1;
				}
			}
			
			if (j)
				return -4; // drawing over double sided walls
		}

		if (numPoints == 1)
		{
            tx = first->x; ty = first->y;
            offsetPos(0, 12, 0, getangle(x-first->x, y-first->y) & kAngMask, &tx, &ty, NULL);
            			
			// check for potential sector split
			for (i = 0; destSect < 0 && i < numsectors; i++)
			{
				if (!inside(tx, ty, i))
					continue;
				
				getSectorWalls(i, &s, &e);
				while(s <= e)
				{
					getWallCoords(s, &x1, &y1, &x2, &y2);
					if (x1 == first->x && y1 == first->y && (x2 != x || y2 != y))
					{
						j = lastwall(s);
						if (wall[j].x != x || wall[j].y != y)
						{
							destSect = i;
							break;
						}
					}
					
					s++;
				}
			}
		}
		
		if (destSect >= 0)
		{
            // see if we can split sector now
            if ((s = findSplitWallAtPos(destSect, first->x, first->y)) < 0); // skip
            else if ((e = findSplitWallAtPos(destSect, x, y)) < 0);          // skip
            else if (s != e)
            {
                lna	= loopnumofsector(destSect, s);
                lnb	= loopnumofsector(destSect, e);
                
                if (lna == lnb)
                {
                    if (numwalls + (numPoints<<1) >= kMaxWalls)		return -1;
                    else if (numsectors + 2 >= kMaxSectors)			return -2;
                    else											return  1; // ready for split
                }
                else if (numwalls + numPoints >= kMaxWalls)			return -1;
                else if (numsectors >= kMaxSectors)					return -2;
                else												return  2; // ready for join
            }
		}
	}

	if (atFirstPoint)
	{
		if (numPoints > 2)
		{
			destSect = -1;
			pModelW = NULL, pModelS = NULL;
			if ((i = findWallAtPos(x, y)) < 0)
			{
				destSect = numsectors;
				while(--destSect >= 0)
				{
					if (inside(first->x, first->y, destSect))
					{
                        pModelW = &wall[sector[destSect].wallptr];
                        if (numsectors < kMaxSectors-1 && numwalls+(numPoints<<1) < kMaxWalls-1)
                        {
                            // Check if loop is going to be made around
                            // wall loop(s) of the destination sector.
                            // In this case auto create red sector
                            // and transfer loops.
                            
                            getSectorWalls(destSect, &s, &e);
                            while(s <= e)
                            {
                                loopGetWalls(s, &ls, &le);
                                while(ls <= le && insidePoints(wall[ls].x, wall[ls].y, point, numPoints+1)) ls++;
                                if (ls > le)
                                    return 7; // ready for red sector around wall loop
                                
                                s++;
                            }
                        }
                        
						return 3; // ready for solid wall loop inside sector
					}
				}
				
				return (numsectors >= kMaxSectors-1) ? -2 : 4; // ready for new solid island sector
			}
			
			pModelW = &wall[i]; pModelS = &sector[sectorofwall(i)];
			return (numsectors >= kMaxSectors-1) ? -2 : 5; // ready for new solid sector connected to another solid sector
		}
		
		return -5;
	}
	
	return 0; // ready for point insertion
	
}

char LOOPBUILD::Setup(int x, int y)
{
	if ((status = SetupPrivate(x, y)) >= 0)
		return 1;
	
	return 0;
}

void LOOPBUILD::Draw(SCREEN2D* pScr)
{
 	POINT2D *p, *first = First(), *last = Last(); LINE2D l; VOIDLIST* lines;
	int x1, y1, x2, y2, sx1, sy1, sx2, sy2;
    int nTotal, vs, i, j;
    char isAuto = 0, c;
    
    const char cWallNewA 	    = pScr->ColorGet(kColorLightGray);
    const char cWallNewB 	    = pScr->ColorGet(kColorLightRed);
	const char cWallAng		    = pScr->ColorGet(kColorBlack);
	const char cCaptFrgA 	    = pScr->ColorGet(kColorYellow);
    const char cCaptFrgB 	    = pScr->ColorGet(kColorLightGray);
	const char cVertFrgA	    = pScr->ColorGet(kColorYellow);
	const char cVertBgdA	    = pScr->ColorGet(kColorBlack);
	const char cVertFrgB	    = pScr->ColorGet(kColorLightMagenta);
	const char cVertBgdB	    = pScr->ColorGet(kColorMagenta);
    
	const char cFillOk		    = pScr->ColorGet(kColorLightBlue);
	const char cCross	        = pScr->ColorGet(kColorLightRed);
    
    if (status != 6)                    nTotal = numPoints;
    else if (totalclock < autoTimer)    nTotal = numPoints-1;
    else                                nTotal = numPoints+numAutoPoints, isAuto = 1;
    
    if (pScr->prefs.useTransluc)
    {
        if (numPoints >= 2 && irngok(status, 3, 7) && (isAuto || status != 6))
        {
            lines = new VOIDLIST(sizeof(l));
            for (i = j = 0; i < nTotal; i++)
            {
                j = IncRotate(j, nTotal);
                l.x1 = point[i].x;
                l.y1 = point[i].y;
                
                l.x2 = point[j].x;
                l.y2 = point[j].y;
                
                lines->Add(&l);
            }
            gfxTranslucency(1);
            pScr->FillPolygon((LINE2D*)lines->First(), lines->Length(), cFillOk, 1);
            gfxTranslucency(0);
            delete(lines);
        }
    }

	i = nTotal;
	while(i > 0)
	{
		isAuto = (isAuto && i >= numPoints);
        
		p = &point[i--];
		x2 = sx2 = p->x;
		y2 = sy2 = p->y;
		
		p--;
		
		x1 = sx1 = p->x;
		y1 = sy1 = p->y;
		
        if (isAuto)             c = cWallNewB;
        else if (status == 1)   c = cWallNewB;
        else                    c = cWallNewA;
        
		pScr->ScalePoints(&sx1, &sy1, &sx2, &sy2);
		pScr->DrawLine(sx1, sy1, sx2, sy2, c);
		if (p == last && !isAuto && x1 != x2 && y1 != y2)
			pScr->DrawLine(sx1, sy1, sx2, sy2, cWallAng, 0, kPatDotted);
		
		vs = (first == p) ? 7 : 5;
		if ((j = findWallAtPos(x1, y1)) >= 0)  pScr->DrawVertex(sx1, sy1, cVertFrgB, cVertBgdB, vs);
		else                                   pScr->DrawVertex(sx1, sy1, cVertFrgA, cVertBgdA, vs);
        
        c = (isAuto) ? cCaptFrgB : cCaptFrgA;
        pScr->CaptionPrintLineEdit(0, x1, y1, x2, y2, c, -1, pScr->pEditFont);
		
		if (cross.w >= 0 && BLINKTIME(16))
		{
			sx1 = cross.x; sy1 = cross.y;
			pScr->ScalePoints(&sx1, &sy1);
			pScr->DrawIconCross(sx1, sy1, cCross, 6);
		}
	}
}