/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// Intends for various automatic door sector creation
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

#include "xmpdoorwiz.h"
#include "xmpmaped.h"
#include "aadjust.h"
#include "eventq.h"
#include "db.h"

NAMED_TYPE gSMDoorTypes[] =
{
	{0, "Standard" },
	{1, "Curtain" },
};

CHECKBOX_LIST gDoorOptions[] =
{
	{FALSE,	"Create a &double door (when possible)."},
	{TRUE,	"&Close it for easy editing."},
	{TRUE,	"Set &push trigger flags."},
};

NAMED_TYPE gDoorWizardMenu[] =
{
	{ 1, "Create a slide marked door" },
	{ 2, "Create a rotate door" },
	{ 3, "Reverse the door &position" },
};

NAMED_TYPE gDoorWizardErrors[] =
{
	{-1, "Not enough space to insert a door"},
	{-3, "Could not find source wall"},
	{-4, "Could not find destination wall"},
	{-9, "Source wall is not solid"},
	{-999, NULL},
};

//--------------------------------------------------------------------------------------------------------

char dlgDoorWizard()
{
	const char tmp[3][8] = {"walls", "sprites", "sectors"};
	int nPad = (grid <= 0) ? 256 : 2048 >> grid, nWall = linehighlight;
	int nSect = (nWall >= 0) ? sectorofwall(nWall) : getSector();
	int nResult, nReq, nDoorType;
	int i, t;
	
	while( 1 )
	{
		nResult		= showButtons(gDoorWizardMenu, LENGTH(gDoorWizardMenu), "Door wizard") - mrUser;
		nDoorType	= 0;
		i = -1;
		
		switch(nResult)
		{
			case 1:
			case 2:
				if (createCheckboxList(gDoorOptions, LENGTH(gDoorOptions), "Options", TRUE) != mrCancel)
				{
					if (nResult == 1)
					{
						nDoorType = showButtons(gSMDoorTypes, LENGTH(gSMDoorTypes), "Select door type") - mrUser;
						if (nDoorType < 0)
							continue;
		
						t = (gDoorOptions[0].checkbox->checked > 0);
						
						nReq = ((nDoorType == 0) ? 6 : 4) << t;
						if (numwalls + nReq >= kMaxWalls) i = 0;
						else if (numsprites + 2 >= kMaxSprites)
							i = 1;

						if (i < 0)
						{
							sectorToolDisableAll(1);
							
							pGDoorSM					= new DOOR_SLIDEMARKED(nDoorType, nPad);
							pGDoorSM->prefs.reverse		= (gDoorOptions[1].checkbox->checked > 0);
							pGDoorSM->prefs.trPush		= (gDoorOptions[2].checkbox->checked > 0);
							pGDoorSM->prefs.twoSides	= t;
							return 1;
						}
						else
						{
							Alert("Not enough %s!", tmp[i]);
						}
					}
					else if (nResult == 2)
					{
						t = (gDoorOptions[0].checkbox->checked > 0);

						nReq = 8 << t;
						if (numwalls + nReq >= kMaxWalls)
						{
							i = 0;
						}
						else
						{
							nReq = 1 << t;
							if (numsectors + nReq >= kMaxSectors) i = 2;
							else if (numsprites + nReq >= kMaxSprites)
								i = 1;
						}
						
						if (i < 0)
						{
							sectorToolDisableAll(1);
							
							pGDoorR						= new DOOR_ROTATE();
							pGDoorR->info.reverse		= (gDoorOptions[1].checkbox->checked > 0);
							pGDoorR->info.trPush		= (gDoorOptions[2].checkbox->checked > 0);
							pGDoorR->info.twoSides		= t;
							return 1;
						}
						else
						{
							Alert("Not enough %s!", tmp[i]);
						}
					}
				}
				continue;
			case 3:
				if ((t = reverseSectorPosition(nSect)) < 0)
				{
					if (t == -1 && wall[nWall].nextsector >= 0)
					{
						nSect = wall[nWall].nextsector;
						if ((t = reverseSectorPosition(nSect)) >= 0)
							return 1;
					}
					
					char* errMsg = retnCodeCheck(t, gReverseSectorErrors);
					if (errMsg)
						Alert(errMsg);
					
					continue;
				}
				return 1;
		}

		break;
	}
	
	return 0;
}

void DOOR_ROTATE::Start()
{
	info.rotateAngle	= 0;
	info.canDraw		= 0;
	info.inside			= 0;
	info.twoSides		= 0;
	info.mode			= 0;
	status				= 0;
}

void DOOR_ROTATE::Stop()
{
}

char DOOR_ROTATE::Setup(int nWall, int nWidth, int x, int y)
{
	SIDEINFO* pSide = info.side;
	SCANWALL src, self, other;
	int zTop, zBot, nAng, nSect, tSect;
	int x1, y1, x2, y2, d1, d2, i;
	POSOFFS pos;
	
	info.canDraw		= 0;
	info.parentSector	= sectorofwall(nWall);
	info.nextSector		= (wall[nWall].nextsector >= 0) ? wall[nWall].nextsector : info.parentSector;
	info.shade			= wall[nWall].shade;
	info.width			= nWidth;
	
	if ((info.mode == 2) || (!info.mode && wall[nWall].nextsector < 0))
	{
		nAng = GetWallAngle(nWall);
		floorGetEdgeZ(info.parentSector, &zBot, &zTop);
		src.pos.New(nAng, x, y, zTop);
		src.s = info.parentSector;
		src.w = nWall;
		
		src.pos.Turn(kAng90);
		src.pos.Offset(1, 8, 0); 	scanWallOfSector(&src, &other);
		src.pos.Turn(kAng180);		scanWallOfSector(&src, &self);
		src.pos.Reset();
		
		if (self.w < 0)
		{
			status = -3;
			return 0;
		}
		
		if (other.w < 0)
		{
			status = -4;
			return 0;
		}
		
		x1 = self.pos.x;	x2 = other.pos.x;
		y1 = self.pos.y;	y2 = other.pos.y;
		
		if (info.dir ^ (x1 > x2 || y1 > y2))
		{
			nAng = (nAng + kAng180) & kAngMask;
			swapValues(&x1, &x2);
			swapValues(&y1, &y2);
		}
		
		info.angle			= nAng;
		info.height			= exactDist(x2 - x1, y2 - y1);
		info.nextSector		= info.parentSector;
		info.solidWall		= 1;
		info.inside			= 0;
		
		d1 = exactDist(x1 - x, y1 - y);
		d2 = exactDist(x2 - x, y2 - y);
	}
	else
	{
		i = nWall;
		if ((nSect = wall[nWall].nextsector) >= 0)
		{
			nAng = GetWallAngle(nWall);
			
			do	// to the left
			{
				i = wall[i].point2;
				getWallCoords(i, &x2, &y2);
				
				if ((tSect = wall[i].nextsector) < 0)						break;
				else if (sector[tSect].floorz != sector[nSect].floorz)		break;
				else if (sector[tSect].ceilingz != sector[nSect].ceilingz)	break;
				else if (GetWallAngle(i) != nAng)							break;
			}
			while(i != nWall);
			
			
			i = nWall;
			do // to the right
			{
				getWallCoords(i, &x1, &y1);
				i = lastwall(i);
				
				if ((tSect = wall[i].nextsector) < 0)						break;
				else if (sector[tSect].floorz != sector[nSect].floorz)		break;
				else if (sector[tSect].ceilingz != sector[nSect].ceilingz)	break;
				else if (GetWallAngle(i) != nAng)							break;
			}
			while(i != nWall);
		}
		
		if (i == nWall)
			getWallCoords(nWall, &x1, &y1, &x2, &y2);
		
		info.angle			= (getangle(x2 - x1, y2 - y1) + kAng90) & kAngMask;
		info.height			= exactDist(x2 - x1, y2 - y1);
		info.solidWall		= 0;
		
		d1 = exactDist(x1 - x, y1 - y);
		d2 = exactDist(x2 - x, y2 - y);
	}
	
	info.line.x1 = x1;	info.line.x2 = x2;
	info.line.y1 = y1;	info.line.y2 = y2;
	
	if (info.twoSides)
	{
		info.height >>= 1;
		if (info.height % 2)
			info.height--;
		
		pSide->left = 1;
		pos.New(info.angle - info.rotateAngle, x1, y1);
		pSide->available = SetupPoints(pos, pSide);
		
		pSide++;
		
		pSide->left = 0;
		pos.New(info.angle + info.rotateAngle, x2, y2);
		pSide->available = SetupPoints(pos, pSide);
	}
	else
	{
		if (d1 > d2)
		{
			swapValues(&x1, &x2);
			swapValues(&y1, &y2);
		}
		
		pSide->left = (d1 < d2);
		
		if (pSide->left)
			pos.New(info.angle - info.rotateAngle, x1, y1);
		else
			pos.New(info.angle + info.rotateAngle, x1, y1);
		
		pSide->available = SetupPoints(pos, pSide);
	}
	
	info.canDraw = 1;
	status = 1;
	return 1;
}

char DOOR_ROTATE::SetupPoints(POSOFFS pos, SIDEINFO* pSide)
{
	POINT2D tmp[5], *p = pSide->point;
	int nHeigh = info.height, t;
	
	if (info.line.x1 != info.line.x2 && info.line.y1 != info.line.y2)
	{
		if (info.twoSides) nHeigh-=pSide->left;
		else if (info.inside)
			nHeigh-=1;
	}
	
	// real points (walls)
	if (!info.inside) pos.Forward(1);
	else pos.Offset((pSide->left) ? info.width : -info.width, info.width, 0);
	p->x = pos.x; p->y = pos.y;
	p++;
	
	pos.Forward(nHeigh);
	p->x = pos.x; p->y = pos.y;
	p++;
	
	pos.Offset((pSide->left) ? -info.width : info.width, 0, 0);
	p->x = pos.x; p->y = pos.y;
	p++;
		
	pos.Backward(nHeigh);
	p->x = pos.x; p->y = pos.y;
	p++;
	
	// real points (marker)
	t = (info.inside && !info.solidWall) ? 3 : 0;
	p->x = pSide->point[t].x;
	p->y = pSide->point[t].y;


	p = tmp;
	memcpy(p, pSide->point, sizeof(tmp));
	pos.Reset();


	// fake points for inside test (walls)
	if (info.solidWall) 	pos.Offset((pSide->left) ? -2 : 2, 0, 0);
	else if (info.inside)	pos.Offset((pSide->left) ? info.width : -info.width, info.width, 0);
	else 					pos.Forward(4);
	
	p->x = pos.x; p->y = pos.y;
	p++;
	
	pos.Forward(info.height-2);
	p->x = pos.x; p->y = pos.y;
	p++;
	
	pos.Offset((pSide->left) ? (-info.width+2) : (info.width-2), 0, 0);
	p->x = pos.x; p->y = pos.y;
	p++;
		
	pos.Backward(info.height-2);
	p->x = pos.x; p->y = pos.y;
	
	return loopInside(info.parentSector, tmp, 4, 1);
}

void DOOR_ROTATE::SetupWalls(SIDEINFO* pSide)
{
	walltype* pWall;
	int s, e;
	
	getSectorWalls(pSide->doorSector, &s, &e);
	while(s <= e)
	{
		pWall = &wall[wall[s].nextwall];
		
		switch(e-s)
		{
			case 0:
			case 2:
				pWall->xrepeat = 8;
				break;
			default:
				pWall->xrepeat = 2;
				break;
			
		}
		
		s++;
	}
}

void DOOR_ROTATE::SetupSector(SIDEINFO* pSide)
{
	int nSect = pSide->doorSector;
	int nAng = info.rotateAngle - kAng90;
	int z;
	
	sectortype* pDoor = &sector[nSect]; XSECTOR* pXDoor;
	sectortype* pNext = &sector[info.nextSector];
	spritetype* pMark;
	
	if (pDoor->extra > 0)
		dbDeleteXSector(pDoor->extra);
	
	z = pDoor->floorz;
	if (!(pNext->ceilingstat & kSectParallax))
		z = ClipLow(pNext->ceilingz, pDoor->ceilingz);

	pDoor->type				= kSectorRotate;
	pDoor->floorz			= z;
	pDoor->floorstat 		&= ~(kSectParallax | kSectSloped);
	pDoor->floorstat 		|= kSectRelAlign;
	pDoor->floorshade		= NUMPALOOKUPS(1);
	pDoor->floorslope		= 0;
	pDoor->floorxpanning 	= 0;
	pDoor->floorypanning 	= 0;

	pDoor->ceilingstat		&= ~(kSectRelAlign | kSectSloped);
	pDoor->ceilingslope 	= 0;
	
	pXDoor = &xsector[GetXSector(nSect)];
	CleanUp();
	
	if (pXDoor->marker0 >= 0)
	{
		pMark = &sprite[pXDoor->marker0];
		pMark->ang	= (pSide->left ^ info.solidWall) ? -nAng : nAng;
		pMark->x	= pSide->point[4].x;
		pMark->y	= pSide->point[4].y;
	}
	
	if (info.twoSides)
	{
		if (pSide->left)
		{
			pXDoor->rxID		= findUnusedChannel();
			pXDoor->txID		= findUnusedChannel();
			pXDoor->command		= kCmdLink;
		}
		else
		{
			SIDEINFO* pSideA = &info.side[0];
			if (pSideA == pSide)
				pSideA++;
			
			XSECTOR* pXSide0	= &xsector[sector[pSideA->doorSector].extra];
			pXDoor->rxID		= pXSide0->txID;
			pXDoor->txID		= pXSide0->rxID;
			pXDoor->command		= kCmdToggle;
			pXDoor->decoupled	= 1;
		}
	}
	
	if (info.trPush)
		pXDoor->triggerWallPush = 1;
	
	pXDoor->interruptable	= 1;
	pXDoor->busyTimeA		= 8;
	pXDoor->busyTimeB		= 6;
}

void DOOR_ROTATE::Insert()
{
	int i, t, c;
	SIDEINFO* pSide;
	
	walltype model;
	memset(&model, 0, sizeof(model));
	model.shade			= info.shade;
	model.nextwall		= -1;
	model.nextsector	= -1;
	model.extra			= -1;
	model.yrepeat		=  8;
	
	c = 0;
	for (i = 0; i <= info.twoSides; i++)
	{
		pSide = &info.side[i];
		pSide->doorSector = numsectors;
		t = insertLoop(info.parentSector, pSide->point, 4, &model);
		
		if (redSectorMake(t) == 0)
		{
			CleanUp();
			SetupWalls(pSide);
			SetupSector(pSide);
			
			if (info.reverse)
				reverseSectorPosition(pSide->doorSector);
			
			c += 4;
		}
	}
}

void DOOR_ROTATE::Draw(SCREEN2D* pScr)
{
 	if (!info.canDraw)
		return;
	
	int x1, y1, x2, y2, i, j, t;
	char c;
	
	LINE2D fill[4];
	POINT2D* point; SIDEINFO* pSide;
	
	// draw closed position lines
	t = mulscale14(info.width, pScr->data.zoom); c = pScr->ColorGet(kColorLightCyan);
	x1 = pScr->cscalex(info.line.x1);	x2 = pScr->cscalex(info.line.x2);
	y1 = pScr->cscaley(info.line.y1);	y2 = pScr->cscaley(info.line.y2);
	if (info.solidWall)
	{
		offsetPos(0, -t, 0, info.angle, &x1, &y1, NULL);
		offsetPos(0, -t, 0, info.angle, &x2, &y2, NULL);
	}
	
	pScr->DrawLine(x1, y1, x2, y2, c, 0, kPatDotted);
	
	offsetPos(0, t, 0, info.angle, &x1, &y1, NULL);
	offsetPos(0, t, 0, info.angle, &x2, &y2, NULL);
	pScr->DrawLine(x1, y1, x2, y2, c, 0, kPatDotted);
	
	// draw door walls
	for (i = 0; i <= info.twoSides; i++)
	{
		c = pScr->ColorGet(kColorRed);
		pSide = &info.side[i];
		point = pSide->point;
		
		for (j = 0; j < 4; j++)
		{
			if (j < 3)
			{
				x1 = point[j].x;	x2 = point[j+1].x;
				y1 = point[j].y;	y2 = point[j+1].y;
			}
			else
			{
				x1 = point[0].x;	x2 = point[3].x;
				y1 = point[0].y;	y2 = point[3].y;
			}
			
			fill[j].x1 = x1;	fill[j].x2 = x2;
			fill[j].y1 = y1;	fill[j].y2 = y2;
			
			pScr->ScalePoints(&x1, &y1, &x2, &y2);
			pScr->DrawLine(x1, y1, x2, y2, c, 0, kPatDotted);
		}
		
		c = (info.side[i].available) ? kColorLightBlue : kColorLightRed;
		
		if (pScr->prefs.useTransluc)
		{
			gfxTranslucency(1);
			pScr->FillPolygon(fill, LENGTH(fill), pScr->ColorGet(c), 1);
            gfxTranslucency(0);
		}
		else
		{
			pScr->FillPolygon(fill, LENGTH(fill), pScr->ColorGet(c), 2);
		}
		
		
		// draw marker position
		point = &pSide->point[4]; c = pScr->ColorGet(kColorLightCyan);
		x1 = pScr->cscalex(point->x); y1 = pScr->cscaley(point->y);
		pScr->DrawIconCross(x1, y1, c, 3);
	}
}


//--------------------------------------------------------------------------------------------------------


void DOOR_SLIDEMARKED::Start(int nType, int nPad)
{
	switch(nType)
	{
		case 0:	info.reqPoints = 6;	break;
		case 1:	info.reqPoints = 4;	break;
	}
	
	info.canDraw		= 0;
	prefs.type	 		= nType;
	info.padding 		= nPad;
	status				= 0;
}

void DOOR_SLIDEMARKED::Stop()
{
}

char DOOR_SLIDEMARKED::Setup(int nWall, int nWidth, int x, int y)
{
	SCANWALL src, dst, self[2], other[2];
	int nAng	= (GetWallAngle(nWall) + kAng90) & kAngMask;
	int nAng180 = (nAng + kAng180) & kAngMask;
	int zTop, zBot, w1, w2;
	sectortype* pSect;
	
	info.canDraw	= 0;
	info.width		= nWidth;
	info.height		= 0;
	info.sector		= sectorofwall(nWall);
	info.angle		= nAng;
	
	pSect = &sector[info.sector];

	floorGetEdgeZ(info.sector, &zBot, &zTop);
	src.pos.New(nAng, x, y, zTop);
	src.pos.Forward(8);
	src.s = info.sector;
	src.w = nWall;
	
	scanWallOfSector(&src, &dst);
	
	src.pos.Left(info.width>>1);
	
	src.pos.a = nAng180;			scanWallOfSector(&src,	&self[0]);
	src.pos.a = nAng;				scanWallOfSector(&src, 	&other[0]);
	src.pos.Right(info.width);		scanWallOfSector(&src, 	&other[1]);
	src.pos.a = nAng180;			scanWallOfSector(&src,	&self[1]);
	
	src.pos.Reset();
	
	status = -1;
	w1 = self[0].w;
	w2 = self[1].w;
	if (w1 < 0 || w2 < 0)
	{
		if ((w1 < 0) ^ (w2 < 0))
			status = -1;
		else
			status = -3;
		
		return 0;
	}
	else if (w1 != w2)
	{
		status = -1;
		return 0;
	}
	else if (wall[w1].nextwall >= 0)
	{
		status = -9;
		return 0;
	}
	
	w1 = other[0].w;
	w2 = other[1].w;
	if (w1 < 0 || w2 < 0)
	{
		if ((w1 < 0) ^ (w2 < 0))
			status = -1;
		else
			status = -4;
		
		return 0;
	}

	info.twoSides = (prefs.twoSides && w1 == w2 && wall[w1].nextwall < 0);

	if (info.twoSides)
	{
		info.height = exactDist(src.pos.x - dst.pos.x, src.pos.y - dst.pos.y) >> 1;
	}
	else
	{
		int d1 = exactDist(self[0].pos.x - other[0].pos.x, self[0].pos.y - other[0].pos.y);
		int d2 = exactDist(self[1].pos.x - other[1].pos.x, self[1].pos.y - other[1].pos.y);
		info.height = (d1 > d2) ? d2 : d1;
	}
	
	if (info.twoSides && (info.height % 2))
		info.height--;
	
	if (info.height < 128 || info.width < 16)
	{
		status = -1;
		return 0;
	}
	
	info.line.x1 = src.pos.x;	info.line.x2 = dst.pos.x;
	info.line.y1 = src.pos.y;	info.line.y2 = dst.pos.y;
	
	info.srcWall[1] = (src.w > dst.w) ? (dst.w) : (dst.w+info.reqPoints);
	info.srcWall[0] = src.w;
	
	SetPadding(info.padding);
	SetupPoints(src.pos, 0, info.srcWall[0], -1);
	if (info.twoSides)
		SetupPoints(dst.pos, 1, info.srcWall[1], nAng180);
		
	info.canDraw	= 1;
	status = 1;
	return 1;
}

void DOOR_SLIDEMARKED::SetupWalls(int nWall, char reverse)
{
	int nPoints = info.reqPoints, i;
	int nOffs;
	
	for (i = nWall; i < nWall + nPoints; i++)
	{
		walltype* pWall = &wall[i];
		nOffs = i - nWall;
		
		switch(prefs.type)
		{
			case 0:
				if (rngok(nOffs, 2, nPoints - 1))
				{
					pWall->cstat |= ((reverse) ? kWallMoveReverse : kWallMoveForward);
					
					if (prefs.trPush)
					{
						XWALL* pXWall		= &xwall[GetXWall(i)];
						pXWall->txID		= info.channel;
						pXWall->command		= kCmdToggle;
						pXWall->triggerOn	= 1;
						pXWall->triggerOff	= 1;
						pXWall->triggerPush = 1;
					}
				}
				break;
			case 1:
				if (nOffs > 0)
				{
					if (nOffs == 2)
						pWall->cstat |= ((reverse) ? kWallMoveReverse : kWallMoveForward);
					
					if (prefs.trPush)
					{
						XWALL* pXWall		= &xwall[GetXWall(i)];
						pXWall->txID		= info.channel;
						pXWall->command		= kCmdToggle;
						pXWall->triggerOn	= 1;
						pXWall->triggerOff	= 1;
						pXWall->triggerPush = 1;
					}
				}
				break;
		}
		
		if (nOffs > 0)
		{
			pWall->xpanning = 0;
			pWall->ypanning = 0;
			pWall->yrepeat	= 8;
			pWall->pal		= 0;
			pWall->picnum	= 0;
			pWall->xrepeat	= (nOffs == 3) ? 1 : 8;
			pWall->shade	= wall[info.srcWall[0]].shade;
			pWall->cstat 	&= ~(kWallFlipX | kWallFlipY);
		}
	}
}

void DOOR_SLIDEMARKED::SetPadding(int nPad)
{
	int t = info.height >> 2;
	int d = klabs(nPad - info.padding);
	if (klabs(nPad - info.padding) < 32)
		info.padding = 0;
	
	info.padding = ClipRange(nPad, -t, t);
}

void DOOR_SLIDEMARKED::SetupSector(int nSect)
{
	XSECTOR* pXSect	= &xsector[GetXSector(nSect)];
	sectortype* pSect = &sector[nSect];
	WALLPOINT_INFO *pPoint;
	spritetype* pMark;

	pSect->type = kSectorSlideMarked;
	CleanUp();
	
	if (pXSect->marker1 >= 0 && pXSect->marker0 >= 0)
	{
		pPoint = &info.points[0][2];
		POSOFFS pos(info.angle, pPoint->x, pPoint->y);
		pos.Left(info.width>>1);
		
		pMark = &sprite[pXSect->marker1];
		pMark->ang	= 0;
		pMark->x	= pos.x;
		pMark->y	= pos.y;
		pMark->z	= 0;
		
		pos.Forward(info.height-info.padding);

		pMark = &sprite[pXSect->marker0];
		pMark->ang	= 0;
		pMark->x	= pos.x;
		pMark->y	= pos.y;
		pMark->z	= 0;
	}
	
	pXSect->busyTimeA		= 8;
	pXSect->busyTimeB		= 6;
	pXSect->interruptable	= 1;
	
	if (prefs.trPush)
		pXSect->rxID = info.channel;
}

void DOOR_SLIDEMARKED::SetupPoints(POSOFFS pos, int nSide, int nStartWall, int nAng)
{
	WALLPOINT_INFO* pInfo = &info.points[nSide][0];
	
	if (nAng >= 0)
		pos.a = nAng;
	
	pos.Left(info.width>>1);
	pInfo->x = pos.x; pInfo->y = pos.y;
	pInfo->w = nStartWall++;
	
	pInfo++; pos.Right(info.width);
	pInfo->x = pos.x; pInfo->y = pos.y;
	pInfo->w = nStartWall;
	
	pInfo++; pos.Offset(0, info.padding, 0);
	pInfo->x = pos.x; pInfo->y = pos.y;
	pInfo->w = nStartWall;
	
	pInfo++; pos.Left(info.width);
	pInfo->x = pos.x; pInfo->y = pos.y;
	pInfo->w = nStartWall;
	
	switch(prefs.type)
	{
		case 0:
			pInfo++; pos.Backward(info.height);
			pInfo->x = pos.x; pInfo->y = pos.y;
			pInfo->w = nStartWall;
			
			pInfo++; pos.Right(info.width);
			pInfo->x = pos.x; pInfo->y = pos.y;
			pInfo->w = nStartWall+=3;
			break;
	}
}

void DOOR_SLIDEMARKED::Insert()
{
	sectortype* pSect = &sector[info.sector];
	int i;
	
	if (prefs.trPush)
	{
		if (pSect->extra > 0)
			info.channel = xsector[pSect->extra].rxID;
		
		if (info.channel < kChannelUser)
			info.channel = findUnusedChannel();
	}
	
	i = 0;
	while(i <= info.twoSides)
	{
		insertPoints(info.points[i], info.reqPoints);
		SetupWalls(info.srcWall[i], i);
		i++;
	}
	
	SetupSector(info.sector);
	if (prefs.reverse)
		reverseSectorPosition(info.sector);
}

void DOOR_SLIDEMARKED::Draw(SCREEN2D* pScr)
{
	if (!info.canDraw)
		return;
	
	WALLPOINT_INFO* pInfo;
	int x1, y1, x2, y2, i, t;
	int nWidth, nAng, nPad;
	char fc;
	
	nWidth	= mulscale14(info.width, pScr->data.zoom);
	nPad	= mulscale14(klabs(info.padding), pScr->data.zoom);
	
	x1 = pScr->cscalex(info.line.x1);	x2 = pScr->cscalex(info.line.x2);
	y1 = pScr->cscaley(info.line.y1);	y2 = pScr->cscaley(info.line.y2);
	pScr->DrawLine(x1, y1, x2, y2, pScr->ColorGet(kColorLightCyan), 0, kPatDotted);
	
	for (i = 0; i <= info.twoSides; i++)
	{
		pInfo	= info.points[i];
		fc		= pScr->ColorGet((i) ? kColorLightGreen : kColorLightBlue);
		
		nAng = info.angle;
		if (i == 1)
			nAng += kAng180;
				
		switch(prefs.type)
		{
			case 0:
				x1 = pScr->cscalex(pInfo[3].x);	x2 = pScr->cscalex(pInfo[4].x);
				y1 = pScr->cscaley(pInfo[3].y);	y2 = pScr->cscaley(pInfo[4].y);
				if (info.padding < 0)
					offsetPos(0, nPad, 0, nAng, &x1, &y1, NULL);
				t = nWidth;
				break;
			case 1:
				x1 = pScr->cscalex(pInfo[1].x);	x2 = pScr->cscalex(pInfo[2].x);
				y1 = pScr->cscaley(pInfo[1].y);	y2 = pScr->cscaley(pInfo[2].y);
				t = -nWidth;
				break;
		}
		
		pScr->DrawLine(x1, y1, x2, y2, fc, 0, kPatDotted);
		offsetPos(t, 0, 0, nAng, &x1, &y1, NULL);
		offsetPos(t, 0, 0, nAng, &x2, &y2, NULL);
		pScr->DrawLine(x1, y1, x2, y2, fc, 0, kPatDotted);
		
		x1 = pScr->cscalex(pInfo[2].x);	x2 = pScr->cscalex(pInfo[3].x);
		y1 = pScr->cscaley(pInfo[2].y);	y2 = pScr->cscaley(pInfo[3].y);
		pScr->DrawLine(x1, y1, x2, y2, fc, 0, kPatDotted);
	}
}