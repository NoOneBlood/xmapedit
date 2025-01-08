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
	{-7, "Wall or sector inside this loop"},
	{-999, NULL},
};

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
	destSect = -1; numPoints = 0;
	first = last = &point[0];
	splitA = splitB = -1;
	status = 0;
}

void LOOPBUILD::AddPoint(int x, int y, int nPoint)
{
	const int nSiz = sizeof(POINT2D)<<1;
	point = (POINT2D*)realloc(point, nSiz*(numPoints+1));
	dassert(point != NULL);
	
	last  = &point[numPoints];
	first = &point[0];
	
	if (!rngok(nPoint, 0, numPoints)) nPoint = numPoints;
	else memmove(&point[nPoint+1], &point[nPoint], nSiz*(numPoints-nPoint));
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
		memmove(&point[nPoint], &point[nPoint+1], nSiz*(numPoints-nPoint));

	numPoints--;
	point = (POINT2D*)realloc(point, nSiz*ClipLow(numPoints, 1));
	last  = &point[numPoints];
	first = &point[0];
}

int LOOPBUILD::Make()
{
	int nSect = -1, i, s, e;	
	switch(status)
	{
		case 1:
		case 2:
			if (splitA >= 0 && splitB >= 0)
			{
				return sectSplit(destSect, splitA, splitB, point, numPoints+1);
			}
			break;
		case 3:
		case 4:
		case 5:
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
				
				return i;
			}
			break;
	}
	
	return -1;
}

int LOOPBUILD::Process(int x, int y)
{
	char atFirstPoint = (first->x == x && first->y == y);
	int x1, y1, x2, y2, tx, ty, lna, lnb;
	int i, j, s, e;
	
	if (numPoints < 2)
		splitA = splitB = destSect = -1;
	
	if (numPoints > 0)
		UpdPoint(x, y, numPoints);
	
	if (numwalls >= kMaxWalls-1)		return -1;
	if (pointOnWallLine(x, y) == 3)		return -3;
	if (numPoints == 0)					return  0;  // ready for point insertion
	
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
			tx = ((first->x + x)>>1);
			ty = ((first->y + y)>>1);
			
			
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
							destSect		= i;
							splitA			= s;
							break;
						}
					}
					
					s++;
				}
			}
		}
		
		if (splitA >= 0)
		{
			getSectorWalls(destSect, &s, &e);
			splitB = -1;

			while(s <= e) // see if we can split sector now
			{
				if (splitA != s && wall[s].x == x && wall[s].y == y)
				{
					splitB = s;
					lna	= loopnumofsector(destSect, splitA);
					lnb	= loopnumofsector(destSect, splitB);
					
					if (lna == lnb)
					{
						if (numwalls + (numPoints<<1) >= kMaxWalls)		return -1;
						else if (numsectors + 1 >= kMaxSectors)			return -2;
						else											return  1; // ready for split
					}
					else if (numwalls + numPoints >= kMaxWalls)			return -1;
					else if (numsectors >= kMaxSectors)					return -2;
					else												return  2; // ready for join
				}
				
				s++;
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
						i = numsectors;
						while(--i >= 0)
						{
							getSectorWalls(i, &s, &e);
							while(s <= e)
							{
								//if (wall[s].nextsector == destSect || sectorofwall(s) == destSect)
								{
									if (insidePoints(wall[s].x, wall[s].y, point, numPoints+1))
										return -7;
								}
								
								s++;
							}
						}
							
						pModelW = &wall[sector[destSect].wallptr];
						return 3; // ready for solid wall loop inside sector
					}
				}
				
				return (numsectors >= kMaxSectors-1) ? -2 : 4; // ready for new solid island sector
			}
			
			pModelW = &wall[i]; pModelS = &sector[sectorofwall(i)];
			return (numsectors >= kMaxSectors-1) ? -2 : 5; // ready for new sold sector connected to another solid sector
		}
		
		return -5;
	}
	
	return 0; // ready for point insertion
	
}

char LOOPBUILD::Setup(int x, int y)
{
	if ((status = Process(x, y)) >= 0)
		return 1;
	
	return 0;
}

void LOOPBUILD::Draw(SCREEN2D* pScr)
{
 	int x1, y1, x2, y2, sx1, sy1, sx2, sy2;
	int i = numPoints;
	
	const char cWallNew 	= pScr->ColorGet(kColorLightGray);
	const char cStatusOk 	= pScr->ColorGet(kColorWhite);
	const char cStatusErr	= pScr->ColorGet(kColorDarkGray);
	const char cWallAng		= pScr->ColorGet(kColorBlack);
	const char cCaptFrg 	= pScr->ColorGet(kColorYellow);
	const char cCaptBgd 	= pScr->ColorGet(kColorBrown);
	const char cVertFrg		= pScr->ColorGet(kColorYellow);
	const char cVertBgd		= pScr->ColorGet(kColorBlack);
	char c;
	
	while(i > 0)
	{
		if (status < 0 && status != - 5)	c = cStatusErr;
		else if (status > 0)				c = cStatusOk;
		else								c = cWallNew;

		x2 = sx2 = point[i-0].x;	x1 = sx1 = point[i-1].x;
		y2 = sy2 = point[i-0].y;	y1 = sy1 = point[i-1].y;
		pScr->ScalePoints(&sx1, &sy1, &sx2, &sy2);
		
		pScr->DrawLine(sx1, sy1, sx2, sy2, c);
		if (i == numPoints && x1 != x2 && y1 != y2)
			pScr->DrawLine(sx1, sy1, sx2, sy2, cWallAng, 0, kPatDotted);
		
		pScr->DrawVertex(sx1, sy1, cVertFrg, cVertBgd, 5);
		pScr->CaptionPrintLineEdit(0, x1, y1, x2, y2, cCaptFrg, (unsigned char)cCaptBgd, pScr->pEditFont);
		i--;
	}
	
	// draw first point
	pScr->DrawVertex(sx1, sy1, cVertFrg, cVertBgd, 7);
}