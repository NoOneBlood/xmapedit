/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// Intends for various wall loop shape creation
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

NAMED_TYPE gLoopShapeErrors[] =
{
	{-1, "Incorrect shape size"},
	{-2, "Shape is invalid"},
	{-3, "Intersection"},
	{-4, "Sectors limit exceeded"},
	{-5, "Walls limit exceeded"},
	{-999, NULL},
};


NAMED_TYPE gLoopShapeTypes[] =
{
	{kLoopShapeRect,		"Rectangle"},
	{kLoopShapeCircle,		"Circle"},
	{kLoopShapeSquare,		"Square"},
	{kLoopShapeTriangle,	"Triangle"},
};

static short gShapeInfo[kLoopShapeMax][6] = 
{
	{4,		4, 		4,		16,		16,		kAng360},
	{6,		127,	16,		32,		32,		kAng360},
	{4,		4, 		4,		16,		16,		kAng360},
	{3,		3, 		3,		32,		32,		kAng360},
};

void LOOPSHAPE::Start(int nType, int nSect, int x, int y)
{
	int i = LENGTH(point);
	model.type	= OBJ_NONE;
	
	shapeType	    = nType;
	destSect	    = nSect;
    initDestSect	= nSect;
	status		    = 0;
	width		    = 0;
	height		    = 0;
	
	if (!active)
	{
		minPoints	= gShapeInfo[shapeType][0];
		maxPoints	= gShapeInfo[shapeType][1];
		numPoints	= gShapeInfo[shapeType][2];
		minWidth	= gShapeInfo[shapeType][3];
		minHeight	= gShapeInfo[shapeType][4];
		rotateAng	= gShapeInfo[shapeType][5];
	}
	
	sx = cx = ex = x;
	sy = cy = ey = y;
	
	while(--i >= 0)
	{
		point[i].x = x;
		point[i].y = y;
	}

	active = 1;
}

void LOOPSHAPE::Stop()
{
}

void LOOPSHAPE::SetupRectangle(int x2, int y2)
{
	width	= klabs(x2-sx);
	height	= klabs(y2-sy);
	size	= (width > height) ? width : height;
	
	point[0].x = sx;	point[1].x = x2;
	point[0].y = sy;	point[1].y = sy;
	
	point[2].x = x2;	point[3].x = sx;
	point[2].y = y2;	point[3].y = y2;
	
	cx = sx+((x2-sx)>>1);
	cy = sy+((y2-sy)>>1);
}

void LOOPSHAPE::SetupSquare(int x2, int y2)
{
	int wh = klabs(x2-sx);
	int hg = klabs(y2-sy);
	
	cx = sx; cy = sy;
	size	= (wh > hg) ? wh : hg;
	width	= size;
	height	= size;
	
	
	point[0].x = sx-size;		point[1].x = sx+size;
	point[0].y = sy-size;		point[1].y = sy-size;
	
	point[2].x = sx+size;		point[3].x = sx-size;
	point[2].y = sy+size;		point[3].y = sy+size;
}

void LOOPSHAPE::SetupTriangle(int x2, int y2)
{
	int wh = x2-sx;
	int hg = y2-sy;
	
	cx = sx; cy = sy;
	width	= klabs(wh);
	height	= klabs(hg);
	size	= (width > height) ? width : height;
	
	point[0].x = sx;		point[1].x = sx+wh;		point[2].x = sx-wh;	
	point[0].y = sy-hg;		point[1].y = sy+hg;		point[2].y = sy+hg;
}

void LOOPSHAPE::SetupCircle(int x2, int y2)
{
	int wh = klabs(x2-sx);
	int hg = klabs(y2-sy);
	
	size	= (wh > hg) ? wh : hg;
	width	= size;
	height	= size;
	
	int nAng = kAng360 / numPoints;
	int x1 = sx + size, y1 = sy;
	int a, i = 0;
	
	for (a = nAng; a <= kAng360; a += nAng)
	{
		x2 = sx + mulscale30(Cos(a & kAngMask), size);
		y2 = sy + mulscale30(Sin(a & kAngMask), size);
		
		point[i+0].x = x1;
		point[i+0].y = y1;
		
		point[i+1].x = x2;
		point[i+1].y = y2;

		x1 = x2;
		y1 = y2;
		i++;
	}
	
	cx = sx; cy = sy;
}

void LOOPSHAPE::UpdateShape(int x, int y)
{
	int i, s;
	
	ex = x, ey = y;
	switch(shapeType)
	{
		case kLoopShapeRect:
			SetupRectangle(ex, ey);
			break;
		case kLoopShapeCircle:
			SetupCircle(ex, ey);
			break;
		case kLoopShapeSquare:
			SetupSquare(ex, ey);
			break;
		case kLoopShapeTriangle:
			SetupTriangle(ex, ey);
			break;
	}
	
    i = numPoints;
    while(--i >= 0)
    {
        if (rotateAng != kAng360)
            RotatePoint(&point[i].x, &point[i].y, rotateAng, cx, cy);
        
        doGridCorrection(&point[i].x, &point[i].y, 10);
    }
    
	if (!width && !height)															status	=  0;
	else if (!width || !height)														status	= -2;
	else if (width < minWidth || height < minHeight)								status	= -1;
	else if (destSect >= 0 && CheckIntersection())                                  status  = -3;
	else if (destSect >= 0 && numsectors >= kMaxSectors - 1)						status	= -4;
	else if (numwalls >= kMaxWalls - numPoints - 1)									status  = -5;
	else if (status != 2)															status	=  1;
    
    if (status == 1)
    {
        // Try to update destination sector for
        // potential wall loop transfer and
        // auto red sector
        // creation.
        
        i = numPoints;
        if (destSect >= 0)
            while(--i >= 0 && inside(point[i].x, point[i].y, destSect));
        
        if (i >= 0)
        {
            s = numsectors;
            while(i >= 0 && --s >= 0)
            {
                if (s == destSect)
                    continue;
                    
                i = numPoints;
                while(--i >= 0 && inside(point[i].x, point[i].y, s));
            }
            
            if (initDestSect >= 0 && destSect >= 0)
            {
                (s >= 0) ? destSect = s : status = -3;
            }
            else
            {
                destSect = s;
            }
        }
    }
}

char LOOPSHAPE::CheckIntersection(void)
{
    
    int x1, y1, x2, y2, x3, y3, x4, y4;
    int i, j, s, e;
    
    if (destSect >= 0)
    {
        // Cannot cross sector walls and
        // can COVER loops
        // inside.

        getSectorWalls(destSect, &s, &e);
        while(s <= e)
        {
            getWallCoords(s, &x3, &y3, &x4, &y4);
            for (i = j = 0; i < numPoints; i++)
            {
                j = IncRotate(j, numPoints);
                x1 = point[i].x;    x2 = point[j].x;
                y1 = point[i].y;    y2 = point[j].y;
                
                if (kintersection(x1, y1, x2, y2, x3, y3, x4, y4, NULL, NULL))
                    return 1;
            }
            
            s++;
        }
    }
    
    return 0;
}

char LOOPSHAPE::Setup(int x2, int y2, OBJECT* pModel)
{
	UpdateShape(x2, y2);
    
	if (status > 0)
	{
		if (pModel)
			memcpy(&model, pModel, sizeof(OBJECT));
		
		return 1;
	}
	
	return 0;
}

void LOOPSHAPE::Draw(SCREEN2D* pScr)
{
	if (status == 0)
		return;
	
	char fillCol;
	char const vtxCol1 = pScr->ColorGet(kColorBlack);
	char const vtxCol2 = pScr->ColorGet(kColorYellow);
	char const lineCol = pScr->ColorGet(kColorGrey18);
	int n = numPoints, i, x1, y1, x2, y2;
	LINE2D fill[LENGTH(point)], *p;
	
    if (status > 0)
        fillCol = pScr->ColorGet((destSect >= 0) ? kColorLightBlue : kColorBlue);
    else
        fillCol = pScr->ColorGet(kColorLightRed);
    
	for (i = 0, p = fill; i < n; i++, p++)
	{
		if (i < n - 1)
		{
			x1 = point[i].x;	x2 = point[i+1].x;
			y1 = point[i].y;	y2 = point[i+1].y;
		}
		else
		{
			x1 = point[0].x;	x2 = point[n-1].x;
			y1 = point[0].y;	y2 = point[n-1].y;
		}
		
		p->x1 = x1,	p->x2 = x2;
		p->y1 = y1,	p->y2 = y2;
	}
	

    
	if (pScr->prefs.useTransluc)
	{
		gfxTranslucency(1);
        
        if (destSect >= 0)
            pScr->FillSector(destSect, pScr->ColorGet(kColorGrey28), 2);
    
		pScr->FillPolygon(fill, numPoints, fillCol, 1);
        gfxTranslucency(0);
	}
	else
	{
		pScr->FillPolygon(fill, numPoints, fillCol, 2);
	}
	
	for (i = 0, p = fill; i < n; i++, p++)
	{
		x1 = pScr->cscalex(p->x1),	x2 = pScr->cscalex(p->x2);
		y1 = pScr->cscaley(p->y1),	y2 = pScr->cscaley(p->y2);
		
		pScr->DrawLine(x1, y1, x2, y2, lineCol, 0, kPatDotted);
		pScr->DrawVertex(x1, y1, vtxCol1, vtxCol2, pScr->vertexSize);
		pScr->DrawVertex(x2, y2, vtxCol1, vtxCol2, pScr->vertexSize);
		
		x1 = p->x1, x2 = p->x2;
		y1 = p->y1, y2 = p->y2;
		pScr->CaptionPrintLineEdit(0, x1, y1, x2, y2,
					pScr->ColorGet(kColorYellow), -1, qFonts[3]);
	}
	
	x1 = pScr->cscalex(cx); y1 = pScr->cscaley(cy);
	pScr->DrawIconCross(x1, y1, lineCol, 2);
	
}

int LOOPSHAPE::Insert()
{
	walltype wmodel; sectortype smodel;
	int nSect = numsectors, nWall = -1;
	int t, ls, le, s, e;
	
	memset(&wmodel, 0, sizeof(wmodel));
	wmodel.extra		= -1;
	wmodel.xrepeat		= 8;
	wmodel.yrepeat		= 8;
	
	memset(&smodel, 0, sizeof(smodel));
	smodel.extra		= -1;
	smodel.floorz		= 8192<<2;
	smodel.ceilingz		= -smodel.floorz;
	
	switch(model.type)
	{
		case OBJ_WALL:
		case OBJ_MASKED:
			if (rngok(model.index, 0, numwalls))
			{
				memcpy(&wmodel, &wall[model.index], sizeof(walltype));
				if ((t = sectorofwall(model.index)) >= 0)
					memcpy(&smodel, &sector[t], sizeof(sectortype));
			}
			break;
		case OBJ_SPRITE:
			model.index = sprite[model.index].sectnum;
			// no break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			if (rngok(model.index, 0, numsectors))
			{
				t = sector[model.index].wallptr;
				memcpy(&smodel, &sector[model.index], sizeof(sectortype));
				memcpy(&wmodel, &wall[t], sizeof(walltype));
			}
			break;
	}
	
	if ((nWall = insertLoop(destSect, point, numPoints, &wmodel, &smodel)) >= 0)
	{
        loopGetWalls(nWall, &s, &e);
        
        for (t = s; t <= e; t++)
        {
            if (numsectors > nSect) checksectorpointer(t, nSect);
            fixrepeats(t);
        }
        
        AutoAlignWalls(s);
        
        if (destSect >= 0)
        {
            // Check if loop has been made around wall loop(s)
            // of the destination sector. In this case
            // auto create red sector and transfer
            // loops.
            
            getSectorWalls(destSect, &s, &e);
            while(s <= e)
            {
                loopGetWalls(s, &ls, &le);
                while(ls <= le && insidePoints(wall[ls].x, wall[ls].y, point, numPoints)) ls++;
                if (ls > le)
                {
                    redSectorMake(nWall);
                    break;
                }
                
                s++;
            }
        }

        CleanUp();
    }
    
	return nWall;
}

char LOOPSHAPE::ChgPoints(int nNum)
{
	int nOld = numPoints;
	numPoints = ClipRange(numPoints + nNum, minPoints, maxPoints);
	if (nOld != numPoints)
	{
		UpdateShape(ex, ey);
		return 1;
	}
	
	return 0;
}

void LOOPSHAPE::ChgAngle(int nAng)
{
	rotateAng = (rotateAng + nAng) & kAngMask;
	UpdateShape(ex, ey);
}