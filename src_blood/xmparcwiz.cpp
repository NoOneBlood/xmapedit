/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2024: Originally written by NoOne.
// Intends for automatic arc creation
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
#include "xmparcwiz.h" 
#include "xmpmaped.h"

#define kArcMinStepDist	32

NAMED_TYPE gAutoArcErrors[] =
{
	{-1, 	"No source wall"},
	{-2, 	"No sector of source wall"},
	{-3, 	"Empty rectangle sector required"},
	{-4, 	"Cannot reach the destination wall"},
	{-5, 	"Arc must have at least 2 pieces"},
	{-6,	"Not enough free sectors"},
	{-7,	"Not enough free walls"},
	{-8,	"Too small step distance"},
	{-10,	"Cannot set auto-align wall"},
	{-999,	NULL},
};

char dlgArcWizard(SECTAUTOARC* pArc)
{
	#define AFTERH(a, b) (a->left+a->width+b)
	#define AFTERV(a, b) (a->top+a->height+b)
	
	static int nHeigh = 30;
	static char bArcCeil = 1;
	static char bArcFlor = 0;
	static char bSetAutoAlign = 0;
	
	Window dialog(0, 0, 138, 138, "Arc Wizard");
	
	Label* lStep		= new Label(8, 10, "Height:");
	EditNumber* eStep	= new EditNumber(dialog.client->width-68, 4, 60, 16, nHeigh, '%', 1, 100);
	
	Panel* pPanelCb		= new Panel(8, AFTERV(eStep, 4), dialog.client->width-16, 40, 0, 0, 0);
	
	Checkbox* cArcAA	= new Checkbox(0, 0, bSetAutoAlign, "Keep auto-align");
	Checkbox* cArcC		= new Checkbox(0, AFTERV(cArcAA, 1), bArcCeil, "Arc ceiling");
	Checkbox* cArcF		= new Checkbox(0, AFTERV(cArcC, 1), bArcFlor, "Arc floor");
	
	TextButton* bOk		= new TextButton(4, AFTERV(pPanelCb, 4), dialog.client->width-8, 24, "&Ok", mrOk);
	bOk->fontColor		= kColorBlue;
	
	TextButton* bCancel	= new TextButton(4, AFTERV(bOk, 2), dialog.client->width-8, 20, "&Cancel", mrCancel);
	bCancel->fontColor	= kColorRed;
	
	dialog.Insert(lStep);
	dialog.Insert(eStep);
		
	pPanelCb->Insert(cArcAA);
	pPanelCb->Insert(cArcC);
	pPanelCb->Insert(cArcF);
	
	dialog.Insert(pPanelCb);
	
	dialog.Insert(bOk);
	dialog.Insert(bCancel);
		
	ShowModal(&dialog);
	eStep->ClampValue();
	
	nHeigh			= eStep->value;
	bArcCeil		= cArcC->checked;
	bArcFlor		= cArcF->checked;
	bSetAutoAlign	= cArcAA->checked;
	
	if (dialog.endState == mrOk)
	{		
		pArc->heightPerc	= nHeigh;
		pArc->arcFlags		= 0x00;
		
		if (bArcCeil)		pArc->arcFlags |= 0x01;
		if (bArcFlor)		pArc->arcFlags |= 0x02;
		if (bSetAutoAlign)	pArc->arcFlags |= 0x04;
		
		return 1;
	}
	
	return 0;
}

void SECTAUTOARC::Start()
{
	destSector		= -1;
	lines			= NULL;
	slist			= NULL;
	numpieces		= 0;
	midStep			= 0;
	sideStep		= 0;
	insertNew		= 0;
}

void SECTAUTOARC::Stop()
{
	if (lines)
		FREE_AND_NULL(lines);
	
	if (slist)
		DELETE_AND_NULL(slist);
}

int SECTAUTOARC::SetupPrivate(int nWall, int nGrid)
{
	SCANWALL src, dst; POSOFFS pos; LINE2D* l;
	int wx1, wy1, wx2, wy2, mrng[2] = {-1, -1};
	int nAng, nWallOther, nSect, s, e, i, t;
	
	if (lines)
		FREE_AND_NULL(lines);
	
	if (!insertNew)
	{
		if (slist && slist->GetLength() > 1)
			return 2;
		
		return -5;
	}
	else if (nWall < 0)
	{
		destSector = srcWall = -1;
		return - 1;
	}
	else if ((nSect = sectorofwall(nWall)) < 0)
	{
		destSector = srcWall = -1;
		return -2;
	}
	
	if (nSect != destSector || nWall != srcWall)
		midStep = 0, sideStep = 0;
	
	destSector	= nSect;
	srcWall		= nWall;
	
	getSectorWalls(destSector, &s, &e);
	if (!LoopIsRect(s, e))
		return -3;
	
	nAng = (GetWallAngle(nWall) + kAng90) & kAngMask;
	avePointWall(nWall, &wx1, &wy1);
	pos.New(nAng, wx1, wy1);
	
	src.pos = pos, src.pos.Forward(4);
	src.s = destSector;
	src.w = nWall;

	if (!scanWallOfSector(&src, &dst))
		return -4;
		
	avePointWall(dst.w, &wx2, &wy2);
	fullDist = exactDist(wx2-wx1, wy2-wy1);
	
	if (nGrid <= 0)
	{
		sideStep  = ClipLow(sideStep, 2);
		numpieces = sideStep;
		
		stepDist  = fullDist / numpieces;
	}
	else
	{
		stepDist = GRD2PIX(nGrid);
		if (sideStep < 0)
			stepDist /= (klabs(sideStep) + 1);
		else if (sideStep > 0)
			stepDist *= (sideStep + 1);
		
		numpieces = fullDist / stepDist;
	}
	
	if (numpieces > 1)
	{
		midDist	= 0;
		if ((t = midStep) > 0)
		{
			midDist += (((t + 1) * stepDist) - 1);
			numpieces = ClipLow(numpieces - t, 0);
			
			mrng[0] = (numpieces>>1) - 0;
			mrng[1] = (numpieces>>1) - 0;
		}
		
		if ((t = fullDist % stepDist) > 0)
		{
			if (!midDist)
				midDist += stepDist;
			
			if ((numpieces % 2) == 0)
			{
				mrng[0] = (numpieces>>1) - 1;
				mrng[1] = (numpieces>>1) - 0;
				midDist += (t>>1);
			}
			else
			{
				mrng[0] = (numpieces>>1) - 0;
				mrng[1] = (numpieces>>1) - 0;
				midDist += t;
			}
		}
	}
	
	if (numpieces > 1)
	{
		lines = (LINE2D*)malloc(sizeof(LINE2D) * (numpieces + 2));
		dassert(lines != NULL);

		l = &lines[0];			getWallCoords(nWall, &l->x1, &l->y1, &l->x2, &l->y2);
		l = &lines[numpieces];	getWallCoords(dst.w, &l->x1, &l->y1, &l->x2, &l->y2);
		
		for (i = 0, l = &lines[1]; i < numpieces - 1; i++, l++)
		{
			if (irngok(i, mrng[0], mrng[1]))
			{
				pos.Forward(midDist);
			}
			else
			{
				pos.Forward(stepDist);
			}
						
			pos.Turn(-kAng90);
			
			src.pos = pos, src.w = -1, src.s = destSector;
			if (!scanWallOfSector(&src, &dst))
			{
				numpieces = i;
				return -4;
			}
			
			l->x1 = dst.pos.x, l->y1 = dst.pos.y;
			
			doGridCorrection(&l->x1, &l->y1, 10);
			getclosestpointonwall(l->x1, l->y1, dst.w, &l->x1, &l->y1);
			doGridCorrection(&l->x1, &l->y1, 10);

			pos.Turn(kAng180);
			
			src.pos = pos, src.w = -1, src.s = destSector;
			if (!scanWallOfSector(&src, &dst))
			{
				numpieces = i;
				return -4;
			}
			
			l->x2 = dst.pos.x, l->y2 = dst.pos.y;
			doGridCorrection(&l->x1, &l->y1, 10);
			getclosestpointonwall(l->x2, l->y2, dst.w, &l->x2, &l->y2);
			doGridCorrection(&l->x2, &l->y2, 10);

			pos.Turn(-kAng90);
		}
		
		if (numsectors + numpieces >= kMaxSectors)			return -6;
		else if (numwalls + (numpieces<<2) >= kMaxWalls)	return -7; // worst case
		else if (stepDist < kArcMinStepDist)				return -8;
		else												return  1;
	}
	
	return -5;
}

char SECTAUTOARC::Setup(int nWall, int nGrid)
{
	status = SetupPrivate(nWall, nGrid);
	return (status > 0);
}

int SECTAUTOARC::Insert()
{
	int nSect = destSector, nNext, nWall;
	int nHalf = numpieces >> 1;
	int mx, wx, my, wy, i, t, a;
	int x1, y1, x2, y2;
	SCANWALL src, dst;
	POINT2D split[2];
	LINE2D* l;
	
	if (insertNew)
	{
		l = lines;
		wx = l->x1, wy = l->y1;
		sector[nSect].alignto = 0;
		ClearList();
		
		for (i = 0, l++; i < numpieces; i++, l++)
		{
			x1 = l->x1;	x2 = l->x2;
			y1 = l->y1;	y2 = l->y2;
			
			if (i < numpieces - 1)
			{
				// Because of an angled walls, for safety,
				// we may need to scan it
				// again
				
				mx = MIDPOINT(x1, x2); my = MIDPOINT(y1, y2);
				a = getangle(x1-x2, y1-y2);
				src.s = nSect;
				
				src.pos.New(a, mx, my);
				src.pos.Right(4);
				
				if ((nWall = WallSearch(nSect, 2, x1, y1)) >= 0)
				{
					x1 = wall[nWall].x;
					y1 = wall[nWall].y;
				}
				else if (scanWallOfSector(&src, &dst))
				{
					insertPoint(dst.w, x1, y1);
				}
				else
				{
					break;
				}
				
				src.pos.Turn(kAng180);

				if ((nWall = WallSearch(nSect, 2, x2, y2)) >= 0)
				{
					x2 = wall[nWall].x;
					y2 = wall[nWall].y;
				}
				else if (scanWallOfSector(&src, &dst))
				{
					insertPoint(dst.w, x2, y2);
				}
				else
				{
					break;
				}	
				
				split[0].x = x1;	split[1].x = x2;
				split[0].y = y1;	split[1].y = y2;

				sectSplit(nSect, split, 2);
			}
			
			nWall = WallSearch(-1, 0, x1, y1, x2, y2);
			nSect = sectorofwall(nWall);
			nNext = wall[nWall].nextsector;
			
			WallClean(nWall, 0x03);
			
			if (i < nHalf)
			{
				if ((t = WallSearch(nSect, 0, wx, wy)) >= 0)
				{
					setFirstWall(nSect, t);
					wx = x2, wy = y2;
				}
			}
			else if ((nWall = WallSearch(nSect, 0, x1, y1)) >= 0)
			{
				setFirstWall(nSect, nWall);
			}
			
			slist->Add(nSect);
			nSect = nNext;
		}
	}
	
	return BuildArc();
}

int SECTAUTOARC::BuildArc( void )
{
	int32_t nHeigh, nPerc, nSect, nPrev, nWall;
	int32_t cz1, cz2, cz3, fz1, fz2, fz3;
	int32_t nCount, nHalf, nHalfCount;
	int32_t *p, i, s, e, x, y;
	SCANWALL src, dst;
	
	char procCeil = ((arcFlags & 0x01) != 0);
	char procFlor = ((arcFlags & 0x02) != 0);
	
	if (slist == NULL
		|| (nCount = slist->GetLength()) < 2)
			return -5;

	nHalf = nCount >> 1, nHalfCount = nHalf;
	nPerc = 100 - (100 / nHalf);
	if ((nCount % 2) == 0)
		nHalfCount--;
	
	p = slist->GetPtr();
	
	for (i = 0; i < nCount; i++)
	{
		nSect = p[i];
		if (procCeil) SetCeilingSlope(nSect, 0);
		if (procFlor) SetFloorSlope(nSect, 0);

		if (!sector[nSect].alignto)
		{
			// Scan wall that is opposite
			// to first one and set it
			// for auto-align
			
			getSectorWalls(nSect, &s, &e);
			avePointWall(s, &x, &y);
			
			src.pos.New(GetWallAngle(s) + kAng90, x, y);
			src.pos.Forward(4);
			src.s = nSect;
			src.w = s;
			
			if (!scanWallOfSector(&src, &dst)
				|| !setAligntoWall(nSect, dst.w))
					return -10;
		}
	}
	
	// Put all sectors in list on
	// most low/high Z
	
	fz1 = sector[*p].floorz;
	cz1 = sector[*p].ceilingz;
	for (i = 0; i < nCount; i++)
	{
		nSect = p[i];
		if ((cz2 = sector[nSect].ceilingz) < cz1)	cz1 = cz2;
		if ((fz2 = sector[nSect].floorz) > fz1)		fz1 = fz2;
	}
	
	for (i = 0; i < nCount; i++)
	{
		nSect = p[i];
		if (procCeil) SetCeilingZ(nSect, cz1);
		if (procFlor) SetFloorZ(nSect, fz1);
	}
	
	nHeigh = perc2val(heightPerc, klabs(cz1-fz1));
	nPrev = *p;
	
	for (i = 0; i < nHalfCount; i++)
	{
		nSect = p[i];
		
		if (i)
		{
			cz1 = sector[nSect].ceilingz;
			cz2 = sector[nPrev].ceilingz;
			cz3 = perc2val(nPerc, klabs(cz1-cz2));
			
			fz1 = sector[nSect].floorz;
			fz2 = sector[nPrev].floorz;
			fz3 = perc2val(nPerc, klabs(fz1-fz2));
		}
		else
		{
			cz3 = nHeigh;
			fz3 = nHeigh;
		}
		
		if (procCeil) SetCeilingRelative(nSect, +cz3);
		if (procFlor) SetFloorRelative(nSect, -fz3);
		nPrev = nSect;
		
		nSect = p[nCount - i - 1];
		if (procCeil) SetCeilingRelative(nSect, +cz3);
		if (procFlor) SetFloorRelative(nSect, -fz3);
	}

	cz1 = sector[p[nHalfCount]].ceilingz;
	cz2 = sector[nSect].ceilingz;
	
	fz1 = sector[p[nHalfCount]].floorz;
	fz2 = sector[nSect].floorz;
	
	for (i = 0; i < nHalfCount; i++)
	{
		nPerc = 100 - (100 / (i + 1));
		cz3 = perc2val(nPerc, klabs(cz1-cz2));
		fz3 = perc2val(nPerc, klabs(fz1-fz2));
		
		nSect = p[i];
		if (procCeil) SetCeilingRelative(nSect, -cz3);
		if (procFlor) SetFloorRelative(nSect, +fz3);
		
		nSect = p[nCount - i - 1];
		if (procCeil) SetCeilingRelative(nSect, -cz3);
		if (procFlor) SetFloorRelative(nSect, +fz3);
	}
	
	for (i = 0; i < nCount; i++)
		sectAutoAlignSlope(p[i], (arcFlags & 0x03));
	
	if (nCount == 2)
	{
		// I don't really know how to count slope
		// value from z properly, so let it
		// count for me.
		
		nSect = p[0], nPrev = p[1];
		getWallCoords(ALIGNTO(nSect), &x, &y);
		nHeigh = ClipRange(nHeigh, -32768, 32767);
		
		if (procCeil)
		{
			SetCeilingRelative(nSect, nHeigh);
			alignceilslope(nSect, x, y, sector[nPrev].ceilingz);
			
			SetCeilingRelative(nPrev, nHeigh);
			SetCeilingSlope(nPrev, sector[nSect].ceilingheinum);
		}
		
		if (procFlor)
		{
			SetFloorRelative(nSect, -nHeigh);
			alignflorslope(nSect, x, y, sector[nPrev].floorz);
			
			SetFloorRelative(nPrev, -nHeigh);
			SetFloorSlope(nPrev, sector[nSect].floorheinum);
			
			SetFloorSlope(nSect, klabs(sector[nSect].floorheinum));
			SetFloorSlope(nPrev, klabs(sector[nPrev].floorheinum));
		}
	}
	else if ((nCount % 2) == 0)
	{
		nSect = p[nHalfCount - 0]; nPrev = p[nHalfCount - 1];
		cz1 = sector[nSect].ceilingz;
		fz1 = sector[nSect].floorz;
		
		if (procCeil) SetCeilingSlope(nSect,	perc2val(20, sector[nPrev].ceilingheinum));
		if (procFlor) SetFloorSlope(nSect,		perc2val(20, sector[nPrev].floorheinum));
		
		getWallCoords(ALIGNTO(nSect), &x, &y);
		getzsofslope(nSect, x, y, &cz2, &fz2);
		
		cz3 = klabs(cz2-cz1);
		fz3 = klabs(fz2-fz1);
		
		nSect = p[nHalfCount + 1]; nPrev = p[nHalfCount + 2];
		if (procCeil) SetCeilingSlope(nSect,	perc2val(20, sector[nPrev].ceilingheinum));
		if (procFlor) SetFloorSlope(nSect,		perc2val(20, sector[nPrev].floorheinum));
		
		for (i = 1; i < nCount-1; i++)
		{
			nSect = p[i];
			if (procCeil) SetCeilingRelative(nSect,	+cz3);
			if (procFlor) SetFloorRelative(nSect,	-fz3);
		}
		
		sectAutoAlignSlope(p[0],		(arcFlags & 0x03));
		sectAutoAlignSlope(p[nCount-1], (arcFlags & 0x03));
	}
	
	if ((arcFlags & 0x04) == 0)
	{
		for (i = 0; i < nCount; i++)
			sector[p[i]].alignto = 0;
	}
	
	return 1;
}

char SECTAUTOARC::IterateMidStep(int dir, int nGrid)
{
	int nVal = 1, nPieces;
	if ((numpieces % 2) != 0)
		nVal++;
	
	if (dir > 0)
	{
		if (numpieces - nVal >= 2)
		{
			midStep = midStep + klabs(nVal);
			return 1;
		}
	}
	else if (midStep > 0)
	{
		midStep = ClipLow(midStep - nVal, 0);
		return 1;
	}
	
	return 0;
}

char SECTAUTOARC::IterateSideStep(int dir, int nGrid)
{
	int nDist, nVal = 1;
	
	if (dir <= 0)
	{
		if (status != -8)
		{
			sideStep = sideStep - nVal;
			midStep = 0;
			return 1;
		}
	}
	else
	{
		if (nGrid == 0)
		{
			sideStep = ClipLow(sideStep + nVal, 2);
			midStep = 0;
			return 1;
		}
		else if (numpieces > 2)
		{
			nDist = GRD2PIX(nGrid);
			
			if (sideStep > 0)
				nDist *= (sideStep + nVal);
			
			if ((fullDist / nDist) > 1)
			{
				sideStep += nVal;
				midStep = 0;
				return 1;
			}
		}
	}
	
	return 0;
}

char SECTAUTOARC::LoopIsRect(int s, int e)
{
	int nPrevAng = GetWallAngle(s);
	int nDAng, nAng;
	int chg = 0;
	
	#define kFineVal kAng15
	
	while(s <= e)
	{
		if ((nAng = GetWallAngle(s)) != nPrevAng)
		{
			nDAng = DANGLE(nPrevAng, nAng);
			
			if (!irngok(nDAng, -kFineVal, +kFineVal))
			{
				if (!irngok(klabs(nDAng), kAng90-kFineVal, kAng90+kFineVal))
					return 0;
				
				chg++;
			}
			
			nPrevAng = nAng;
		}
		
		s++;
	}

	return irngok(chg, 3, 4);
}

int SECTAUTOARC::WallSearch(int nSect, int nRadius, int x1, int y1, int x2, int y2)
{
	int s, e, i, wx1, wy1, wx2, wy2;
	int nDist = nRadius;
	int nWall = -1;
	
	if (nSect >= 0)
	{
		getSectorWalls(nSect, &s, &e);

		while(s <= e)
		{
			getWallCoords(s, &wx1, &wy1);
			
			if (nRadius > 0)
			{
				if ((i = exactDist(wx1 - x1, wy1 - y1)) <= nDist)
					nDist = i, nWall = s;
			}
			else if (wx1 == x1 && wy1 == y1)
				return s;
				
			s++;
		}
	}
	else
	{
		nWall = numwalls;
		while(--nWall >= 0)
		{
			getWallCoords(nWall, &wx1, &wy1, &wx2, &wy2);
			if (wx1 == x1 && wx2 == x2 && wy1 == y1 && wy2 == y2)
				break;
		}
	}
	
	return nWall;
}

void SECTAUTOARC::WallClean(int nWall, char which)
{
	walltype* pWall	= &wall[nWall];
	int t = pWall->cstat;
	
	pWall->cstat = 0;
	if (t & kWallSwap)
		pWall->cstat |= kWallSwap;
	
	if (t & kWallOrgBottom)
		pWall->cstat |= kWallOrgBottom;
	
	pWall->type		= 0;
	pWall->hitag	= 0;
	
	if (pWall->extra > 0)
	{
		fixXWall(nWall);
		dbDeleteXWall(pWall->extra);
	}
	
	
	fixrepeats(nWall);
	if (pWall->nextwall >= 0 && (which & 0x01))
		WallClean(pWall->nextwall, 0x00);
}

void SECTAUTOARC::Draw(SCREEN2D* pScr)
{
	int32_t *p, i, x1, y1, x2, y2;
	char fillCol, lineCol;
	char vtxCol1, vtxCol2;
	char txtCol, tmp[8];
	LINE2D* l;
	
	if (insertNew)
	{
		fillCol = pScr->ColorGet((status < 0) ? kColorLightRed : kColorLightBlue);
		lineCol = pScr->ColorGet(kColorGrey18);
		vtxCol1 = pScr->ColorGet(kColorBlack);
		vtxCol2 = pScr->ColorGet(kColorYellow);
		
		if (destSector >= 0)
		{
			if (pScr->prefs.useTransluc)
			{
				pScr->LayerOpen();
				pScr->FillSector(destSector, fillCol, 1);
				pScr->LayerShowAndClose(kRSTransluc);
			}
			else
			{
				pScr->FillSector(destSector, fillCol);
			}
		}
		
		if (srcWall >= 0)
		{
			avePointWall(srcWall, &x1, &y1);
			pScr->ScalePoints(&x1, &y1);
			pScr->DrawIconCross(x1, y1, vtxCol2, 4);
		}
		
		
		if (lines)
		{
			i = numpieces;
			while(--i > 0)
			{
				l = &lines[i];
				x1 = pScr->cscalex(l->x1);
				y1 = pScr->cscaley(l->y1);
				
				x2 = pScr->cscalex(l->x2);
				y2 = pScr->cscaley(l->y2);
				
				pScr->DrawLine(x1, y1, x2, y2, lineCol, 0, kPatDotted);
				pScr->DrawVertex(x1, y1, vtxCol1, vtxCol2, pScr->vertexSize);
				pScr->DrawVertex(x2, y2, vtxCol1, vtxCol2, pScr->vertexSize);
			}
		}
	}
	else if (slist)
	{
		fillCol = pScr->ColorGet(kColorYellow);
		txtCol = pScr->ColorGet(kColorBlack);
		
		for (p = slist->GetPtr(), i = 1; *p >= 0; p++, i++)
		{
			avePointSector(*p, &x1, &y1);
			pScr->ScalePoints(&x1, &y1);
			sprintf(tmp, "#%d", i);
			
			pScr->CaptionPrint(tmp, x1, y1, 2, txtCol, fillCol, 0, pScr->pCaptFont);
		}
	}
}