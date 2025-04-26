/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025: Originally written by NoOne.
// Loop split feature.
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
#include "xmplsplt.h" 
#include "xmpmaped.h"

#define kRaySnapDist	48

NAMED_TYPE gLoopSplitErrors[] =
{
	{-1, 	"No source wall"},
	{-2, 	"No sector of source wall"},
	{-3,	"Could not split here"},
	{-4,	"Not enough free sectors"},
	{-5,	"Not enough free walls"},
	{-6,	"Lines intersection"},
	{-999,	NULL},
};

void LOOPSPLIT::Start()
{
	list		= new VOIDLIST(sizeof(LINE2D));
	srcwall		= -1;
	destsector	= -1;
	angoffs		= kAng90;
	skipflag	= 0;
	status		= 0;
}

void LOOPSPLIT::Stop()
{
	if (list)
		DELETE_AND_NULL(list);
}

char LOOPSPLIT::IterateAngleOffset(char dir)
{
	int nBy = 16;
	if (!dir)
		nBy = -nBy;
	
	int nAng = angoffs + nBy;
	int dAng = klabs(nAng - kAng90);
	if (dAng < (kAng90 - kAng5))
	{
		angoffs = nAng;
		return 1;
	}
	
	return 0;
}

int LOOPSPLIT::SetupPrivate(int nWall)
{
	SCANWALL src, dst; LINE2D t, *p, *p2, *f;
	int x, y, mx, my, x2, y2, x3, y3;
	int nCode, nAng, i, s, e, c = 0;
	
	cross		= 0;
	destsector	= -1;
	srcwall		= -1;

	if (list->Length())
		list->Clear();
	
	if ((srcwall = nWall) < 0)
		return -1;
	
	if ((destsector = sectorofwall(nWall)) < 0)
		return -2;

	loopGetWalls(nWall, &s, &e);
	i = nWall;
	do
	{
		getWallCoords(i, &t.x1, &t.y1, &x2, &y2);
		getWallCoords(lastwall(i), &x3, &y3);
		nAng = (getangle(x2-x3, y2-y3) + angoffs) & kAngMask;
		
		src.pos.New(nAng, t.x1, t.y1);
		src.s = sectorofwall(i);
		src.pos.Forward(4);
		
		if ((nCode = scanWallOfSector(&src, &dst, kRaySnapDist)) > 0)
		{
			if (irngok(dst.w, s, e))
				cross = 1;
			
			t.x2 = dst.pos.x;
			t.y2 = dst.pos.y;
			list->Add(&t);
		}
		
		i = wall[i].point2;
	}
	while(i != nWall);
	
	p = f = (LINE2D*)list->First();
	mx = MIDPOINT(p->x2, p->x1);
	my = MIDPOINT(p->y2, p->y1);
	
	while(list->Valid(p) && cross < 2)
	{
		p2 = f;
		while(list->Valid(p2) && cross < 2)
		{
			if (kintersection(p, p2, &x, &y))
			{
				if (cross == 1)
				{
					// For now all lines must cross
					// at the middle point.
					
					if (klabs(x-mx) > 4 || klabs(y-my) > 4)
						cross = 2;
				}
				else
				{
					// None of lines must cross.
					cross = 2;
				}
				
				c++;
			}
			
			p2++;
		}
		
		p++;
	}
	
	if ((i = list->Length()) <= 0)					return -3;
	else if (numsectors + i >= kMaxSectors)			return -4;
	else if (numwalls + (i<<1) >= kMaxWalls)		return -5;
	else if ((cross == 2) || (cross == 1 && !c))	return -6;
	else											return  1;
}

char LOOPSPLIT::Setup(int nWall)
{
	return ((status = SetupPrivate(nWall)) > 0);
}

int LOOPSPLIT::Insert()
{
	SCANWALL src, dst; POINT2D sp[2]; LINE2D* p;
	int nLength, nCode, i, j, dx, dy;
	int done = 0;
	
	p = (LINE2D*)list->First();
	nLength = list->Length();
	for (i = j = 0; i < nLength; i++)
	{
		j = IncRotate(j, nLength);
		if ((src.w = findWallAtPos(p[i].x1, p[i].y1, p[j].x1, p[j].y1)) < 0) break;
		else if (skipflag && (i % 2))
			continue;

		dx = p[i].x2 - p[i].x1;
		dy = p[i].y2 - p[i].y1;

		src.pos.New(getangle(dx, dy), p[i].x1, p[i].y1);
		src.s = sectorofwall(src.w);
		src.pos.Forward(4);

		if ((nCode = scanWallOfSector(&src, &dst, kRaySnapDist)) > 0)
		{
			if (findWallAtPos(p[i].x1, p[i].y1, dst.pos.x, dst.pos.y) < 0)
			{
				sp[0].x = p[i].x1;	sp[1].x = dst.pos.x;
				sp[0].y = p[i].y1;	sp[1].y = dst.pos.y;
				
				if (nCode == 1)
				{
					doGridCorrection(&sp[1].x, &sp[1].y, 10);
					insertPoint(dst.w, sp[1].x, sp[1].y);
				}
				
				if (!sectSplit(src.s, sp, LENGTH(sp)))
					continue;
			}
			
			done++;
		}
	}
	
	return done;
}

void LOOPSPLIT::Draw(SCREEN2D* pScr)
{
	char fillCol, lineCol, vtxCol1, vtxCol2, txtCol;
	int x1, y1, x2, y2, c = 0;
	LINE2D* p;
	
	fillCol = pScr->ColorGet((status <= 0) ? kColorLightRed : kColorLightBlue);
	lineCol = pScr->ColorGet(kColorGrey18);
	vtxCol1 = pScr->ColorGet(kColorBlack);
	vtxCol2 = pScr->ColorGet(kColorYellow);
	
	if (destsector >= 0)
	{
		if (pScr->prefs.useTransluc)
		{
			gfxTranslucency(1);
			pScr->FillSector(destsector, fillCol, 1);
			gfxTranslucency(0);
		}
		else
		{
			pScr->FillSector(destsector, fillCol);
		}
	}
	
	for (p = (LINE2D*)list->First(), c = 0; list->Valid(p); p++, c++)
	{
		if (skipflag && (c % 2))
			continue;
		
		x1 = pScr->cscalex(p->x1);
		y1 = pScr->cscaley(p->y1);
		
		x2 = pScr->cscalex(p->x2);
		y2 = pScr->cscaley(p->y2);
		
		pScr->DrawLine(x1, y1, x2, y2, lineCol, 0, kPatDotted);
		pScr->DrawVertex(x1, y1, vtxCol1, vtxCol2, 4);
		pScr->DrawVertex(x2, y2, vtxCol1, vtxCol2, 4);
	}
	
	if (srcwall >= 0)
	{
		avePointWall(srcwall, &x1, &y1);
		pScr->ScalePoints(&x1, &y1);
		pScr->DrawIconCross(x1, y1, vtxCol2, 3);
	}
}
		
