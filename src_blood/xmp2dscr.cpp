/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// Screen drawing for 2D editing mode.
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

#include "xmp2dscr.h"
#include "xmpmaped.h"
#include "xmpact.h"
#include "tile.h"
#include "hglt.h"
#include "tracker.h"
#include "img2tile.h"
#include "mapcmt.h"
#include "nnexts.h"

#define kCaptPadNormal		2
#define kCaptPadHover		kCaptPadNormal + 1
#define kZoomTags			512
#define kZoomSpritesUnder	256
#define kZoomSprites		128
#define kZoomEditCaption	256
#define kZoomFAWalls		100
#define kZoomVertexAll		300
#define kZoomVertexHglt		256

enum
{
kColorTypeRGB	= 0,
};

static NAMED_TYPE gColorType[] =
{
	{kColorTypeRGB, "RGB"},
};


enum
{
kMarkWallFirst	= 0,
kMarkWallAlign,
};

void SCREEN2D::ColorInit(IniFile* pIni)
{
	char key[256], val[256], *pKey, *pVal; unsigned char rgb[3];
	const char* pGroup = "Palette";
	int nID, nPrevNode = -1;
	int t, i, o;
	
	while(pIni->GetNextString(NULL, &pKey, &pVal, &nPrevNode, (char*)pGroup))
	{
		if (!isufix(pKey))
			continue;

		nID = atoi(pKey);
		if (!rngok(nID, 0, LENGTH(colors)))
			continue;

		o = i = 0;
		while(i < LENGTH(colors[0]) && (o = enumStr(o, pVal, key, val)) > 0)
		{
			if ((t = findNamedID(key, gColorType, LENGTH(gColorType))) == kColorTypeRGB)
			{
				if (parseRGBString(val, rgb))
					ColorFill(nID, countBestColor(gamepal, rgb[0], rgb[1], rgb[2]), i);
			}
			else if ((t = gfxGetStdColor(key)) >= 0)
			{
				if (t < LENGTH(gStdColor))
					ColorFill(nID, gStdColor[t], i);
			}
			else if (isufix(key))
			{
				if ((t = atoi(key)) < 256)
					ColorFill(nID, t, i);
			}
			
			i++;
		}
	}
}

void SCREEN2D::ColorFill(char which, char color, int nStart)
{
	while(nStart < LENGTH(colors[0]))
		colors[which][nStart++] = color;
}

char SCREEN2D::ColorGet(char which, char hover)
{
	return colors[which][hover ? 1 : 0];
}

void SCREEN2D::GetPoint(int scrX, int scrY, int* posX, int* posY, char clamp)
{
	*posX = data.camx + divscale14(scrX - view.wcx, data.zoom);
	*posY = data.camy + divscale14(scrY - view.wcy, data.zoom);
	
	if (clamp)
	{
		*posX = ClipRange(*posX, -boardWidth,  boardWidth);
		*posY = ClipRange(*posY, -boardHeight, boardHeight);
	}
}

void SCREEN2D::GetPoint(int scrX, int scrY, int* posX, int* posY)
{
	GetPoint(scrX, scrY, posX, posY, true);
}

void SCREEN2D::GetPoint(int* x, int* y)
{
	*x = data.camx + divscale14(*x - view.wcx, data.zoom);
	*y = data.camy + divscale14(*y - view.wcy, data.zoom);
}

void SCREEN2D::FillPolygon(LINE2D* coords, int nCount, char c, char nStep, int drawPat)
{
	static int fillist[MAXXDIM];
	int x1, x2, y1, y2, y, dax, miny, maxy, fillcnt;
	int i, j, sy;
	
	gfxSetColor(c);
	miny = maxy = coords[0].y1;
	
	i = nCount;
	while(--i >= 0)
	{
		y1 = coords[i].y1;
		y2 = coords[i].y2;
		
		if (y1 < miny) miny = y1;
		else if (y1 > maxy)
			maxy = y1;
		
		if (y2 < miny) miny = y2;
		else if (y2 > maxy)
			maxy = y2;
	}
	
	miny = ClipLow(cscaley(miny) & nStep,  view.wy1);
	maxy = ClipHigh(cscaley(maxy), view.wy2);
	
	for(sy = miny; sy <= maxy; sy += nStep)
	{
		fillist[0] = view.wx1; fillcnt = 1;
		y = data.camy + divscale14(sy - view.wcy, data.zoom);
		
		i = nCount;
		while(--i >= 0 && fillcnt < LENGTH(fillist))
		{
			x1 = coords[i].x1; x2 = coords[i].x2;
			y1 = coords[i].y1; y2 = coords[i].y2;

			if (y1 > y2)
			{
				swapValues(&x1, &x2);
				swapValues(&y1, &y2);
			}
			
			if (y1 <= y && y2 > y)
			{
				dax = cscalex(x1+scale(y-y1,x2-x1,y2-y1));
				if (dax >= 0)
					fillist[fillcnt++] = ClipRange(dax, view.wx1, view.wx2);
			}
		}
		
		for(i = 1; i < fillcnt; i++)
		{
			for (j = 0; j < i; j++)
			{
				if (fillist[i] >= fillist[j]) continue;
				else swapValues(&fillist[i], &fillist[j]);
			}
		}
		
		j = fillcnt - 1;
		if (drawPat == kPatNormal)
		{
			for (i = (fillcnt & 1); i < j; i += 2)
			{
				if (fillist[i] >= view.wx2) break;
				else gfxHLine(sy, fillist[i], fillist[i+1]);
			}
		}
		else
		{
			for (i = (fillcnt & 1); i < j; i += 2)
			{
				if (fillist[i] >= view.wx2) break;
				else DrawLine(fillist[i], sy, fillist[i+1], sy, c, 0, drawPat);
			}
		}
	}
}

void SCREEN2D::FillSector(int nSect, char c, char nStep, int drawPat)
{
	int i = 0, s, e;
	if ((s = sector[nSect].wallnum) > numflines)
	{
		if ((flines = (LINE2D*)realloc(flines, sizeof(LINE2D)*s)) == NULL)
		{
			numflines = 0;
			return;
		}
		
		numflines = s;
	}
	
	if (flines)
	{
		getSectorWalls(nSect, &s, &e);
		while(s <= e)
		{
			getWallCoords(s++, &flines[i].x1, &flines[i].y1, &flines[i].x2, &flines[i].y2);
			i++;
		}
		
		FillPolygon(flines, i, c, nStep, drawPat);
	}
}

void SCREEN2D::DrawLine(int x1, int y1, int x2, int y2, char nColor)
{
	gfxSetColor(nColor);
	gfxLine(x1, y1, x2, y2);
}

void SCREEN2D::DrawLine(int x1, int y1, int x2, int y2, char c, char b, int drawPat)
{
	int odrawlinepat = drawlinepat;
	drawlinepat = drawPat;
	
	DrawLine(x1, y1, x2, y2, c);
	if (b)
	{
		int dx = x2 - x1;
		int dy = y2 - y1;
		RotateVector(&dx, &dy, kAng45 / 2);
		switch (GetOctant(dx, dy))
		{
			case 0:
			case 4:
				DrawLine(x1, y1-1, x2, y2-1, c);
				DrawLine(x1, y1+1, x2, y2+1, c);
				break;
			case 1:
			case 5:
				if (klabs(dx) < klabs(dy))
				{
					DrawLine(x1-1, y1+1, x2-1, y2+1, c);
					DrawLine(x1-1, y1, x2, y2+1, c);
					DrawLine(x1+1, y1, x2+1, y2, c);
				}
				else
				{
					DrawLine(x1-1, y1+1, x2-1, y2+1, c);
					DrawLine(x1, y1+1, x2, y2+1, c);
					DrawLine(x1, y1+1, x2, y2+1, c);
				}
				break;
			case 2:
			case 6:
				DrawLine(x1-1, y1, x2-1, y2, c);
				DrawLine(x1+1, y1, x2+1, y2, c);
				break;
			case 3:
			case 7:
				if (klabs(dx) < klabs(dy))
				{
					DrawLine(x1-1, y1-1, x2-1, y2-1, c);
					DrawLine(x1-1, y1, x2-1, y2, c);
					DrawLine(x1+1, y1, x2+1, y2, c);
				}
				else
				{
					DrawLine(x1-1, y1-1, x2-1, y2-1, c);
					DrawLine(x1, y1-1, x2, y2-1, c);
					DrawLine(x1, y1+1, x2, y2+1, c);
				}
				break;
		}
	}
	
	drawlinepat = odrawlinepat;
}

void SCREEN2D::DrawAngLine(int nScale, int nAng, int x1, int y1, char c, char b)
{
	int x2, y2;
	ScaleAngLine(nScale, nAng, &x2, &y2);
	DrawLine(x1, y1, x1 + x2, y1 + y2, c, b);
}

void SCREEN2D::DrawCircle(int x, int y, int r, char c, char b, int drawPat)
{
	int a, x1 = x + r, y1 = y, x2, y2;
	for (a = kAng5; a <= kAng360; a += kAng5)
	{
		x2 = x + mulscale30(Cos(a), r);
		y2 = y + mulscale30(Sin(a), r);
		DrawLine(x1, y1, x2, y2, c, b, drawPat);
		
		x1 = x2;
		y1 = y2;
	}
}

void SCREEN2D::DrawVertex(int x, int y, char bc, char fc, char s)
{
	int i, j;
	char hs = s >> 1;
	char d = 0;
	
	gfxSetColor(fc);
	
	j = hs;
	for (i = y - hs; i < y + hs; i++)
	{
		gfxHLine(i, x+(j-hs), x+(hs-j));
		if (j > 0 && !d)
		{
			j--;
		}
		else
		{
			j++;
			d = 1;
		}
	}
	
	DrawLine(x - hs, y, x, y - hs,	bc);
	DrawLine(x, y - hs, x + hs, y,	bc);	
	DrawLine(x + hs, y, x, y + hs, 	bc);
	DrawLine(x, y + hs, x - hs, y,	bc);
}


void SCREEN2D::DrawArrow(int x, int y, char c, int nAng, int nZoom, int nSiz, char b)
{
	int dx, dy;
	ScaleAngLine(nSiz, nAng, &dx, &dy);
	DrawLine(x + dx, y + dy, x - dx, y - dy, c, b);
	DrawLine(x + dx, y + dy, x + dy, y - dx, c, b);
	DrawLine(x + dx, y + dy, x - dy, y + dx, c, b);
}

void SCREEN2D::DrawArrow(int xS, int yS, int xD, int yD, char c, int nZoom, int nAng, char b, int pat1, int pat2)
{
	int nAngle, sAng, sZoom;
	DrawLine(xS, yS, xD, yD, c, b, pat1);
	if (nAng > 0)
	{
		nAngle	= getangle(xS - xD, yS - yD);
		sZoom	= nZoom / 64;

		sAng = nAngle + nAng; 
		xS = mulscale30(sZoom, Cos(sAng));
		yS = mulscale30(sZoom, Sin(sAng));
		DrawLine(xD, yD, xD + xS, yD + yS, c, b, pat2);
		
		sAng = nAngle - nAng;
		xS = mulscale30(sZoom, Cos(sAng));
		yS = mulscale30(sZoom, Sin(sAng));
		DrawLine(xD, yD, xD + xS, yD + yS, c, b, pat2);
	}
}

void SCREEN2D::DrawRect(int x1, int y1, int x2, int y2, char c, char b, int pat)
{
	DrawLine(x1, y1, x2, y1, c, b, pat);
	DrawLine(x1, y1, x1, y2, c, b, pat);
	DrawLine(x1, y2, x2, y2, c, b, pat);
	DrawLine(x2, y1, x2, y2, c, b, pat);
}

void SCREEN2D::DrawSquare(int x, int y, char bc, int s)
{
	gfxSetColor(bc);
	gfxHLine(y-s, x-s, x+s);
	gfxHLine(y+s, x-s, x+s);
	gfxVLine(x-s, y-s, y+s);
	gfxVLine(x+s, y-s, y+s);
}

char SCREEN2D::GetWallColor(int nWall, char hover)
{
	char nColor;
	walltype* pWall = &wall[nWall];
	if (pWall->cstat & kWallMoveForward)		nColor = kColorLightBlue;
	else if (pWall->cstat & kWallMoveReverse)	nColor = kColorLightGreen;
	else if (pWall->nextwall < 0)				nColor = kColorGrey18;
	else if (pWall->cstat & kWallHitscan)		nColor = kColorMagenta;
	else										nColor = kColorRed;
	
	return ColorGet(nColor, hover);
}

char SCREEN2D::GetSpriteColor(spritetype* pSpr, char hover)
{
	char nColor = kColorCyan;
	if (pSpr->cstat & kSprHitscan)			nColor = kColorMagenta;
	if (pSpr->cstat & kSprInvisible)		nColor = kColorGrey24;
	if (pSpr->cstat & kSprMoveForward)		nColor = kColorLightBlue;
	else if (pSpr->cstat & kSprMoveReverse)	nColor = kColorLightGreen;
	
	if (nColor == kColorMagenta || nColor == kColorCyan)
	{
		switch(pSpr->statnum)
		{
			case kStatDude:
				nColor = kColorLightRed;
				break;
			case kStatItem:
				nColor = kColorLightMagenta;
				break;
			case kStatFX:
			case kStatProjectile:
			case kStatExplosion:
				break;
			default:
				switch(pSpr->type)
				{
					case kModernCustomDudeSpawn:
						if (!gModernMap) break;
					case kMarkerDudeSpawn:
						nColor = kColorLightRed;
						break;
				}
				break;
		}
	}
	
	
	return ColorGet(nColor, hover);
}

void SCREEN2D::DrawIconFaceSpr_SMALL(int x, int y, char c)
{
	gfxSetColor(c);
	
	y-=2;
	gfxPixel(x-1, y);
	gfxPixel(x+0, y);
	gfxPixel(x+1, y);
	
	y++;	gfxPixel(x-2, y);	gfxPixel(x+2, y);	gfxPixel(x-1, y);	gfxPixel(x+1, y);
	y++;	gfxPixel(x-2, y);	gfxPixel(x+2, y);
	y++;	gfxPixel(x-2, y);	gfxPixel(x+2, y);	gfxPixel(x-1, y);	gfxPixel(x+1, y);
	
	y++;
	gfxPixel(x-1, y);
	gfxPixel(x+0, y);
	gfxPixel(x+1, y);
}

void SCREEN2D::DrawIconFaceSpr_NORMAL(int x, int y, char c)
{
	gfxSetColor(c);
	
	y-=3;
	gfxPixel(x-1, y);
	gfxPixel(x+0, y);
	gfxPixel(x+1, y);
	
	y++;	gfxPixel(x-2, y);	gfxPixel(x+2, y);
	y++;	gfxPixel(x-3, y);	gfxPixel(x+3, y);
	y++;	gfxPixel(x-3, y);	gfxPixel(x+3, y);
	y++;	gfxPixel(x-3, y);	gfxPixel(x+3, y);
	y++;	gfxPixel(x-2, y);	gfxPixel(x+2, y);
	
	y++;
	gfxPixel(x-1, y);
	gfxPixel(x+0, y);
	gfxPixel(x+1, y);
}

void SCREEN2D::DrawIconMarker(int x, int y, char c)
{
	gfxSetColor(c);
	
	y-=4;
	gfxPixel(x-1, y);
	gfxPixel(x+0, y);
	gfxPixel(x+1, y);
	
	y++;
	gfxPixel(x-3, y);	gfxPixel(x+3, y);
	gfxPixel(x-2, y);	gfxPixel(x+2, y);
	
	y++;
	gfxPixel(x-3, y);	gfxPixel(x+3, y);
	gfxPixel(x-2, y);	gfxPixel(x+2, y);
	
	y++;
	gfxPixel(x-4, y);	gfxPixel(x+4, y);
	gfxPixel(x-1, y);	gfxPixel(x+1, y);
		
	y++;
	gfxPixel(x-4, y);
	gfxPixel(x+0, y);
	gfxPixel(x+4, y);
	
	y++;
	gfxPixel(x-4, y);	gfxPixel(x+4, y);
	gfxPixel(x-1, y);	gfxPixel(x+1, y);
		
	y++;
	gfxPixel(x-3, y);	gfxPixel(x+3, y);
	gfxPixel(x-2, y);	gfxPixel(x+2, y);
	
	y++;
	gfxPixel(x-3, y);	gfxPixel(x+3, y);
	gfxPixel(x-2, y);	gfxPixel(x+2, y);
	
	y++;
	gfxPixel(x-1, y);
	gfxPixel(x+0, y);
	gfxPixel(x+1, y);
}


void SCREEN2D::DrawIconCross(int x, int y, char c, char s)
{
	DrawLine(x-s, y-s, x+s, y+s, c);
	DrawLine(x-s, y+s, x+s, y-s, c);
}

void SCREEN2D::DrawIconCross(int x, int y, int nAng, char c, char s)
{
	int x1, y1, x2, y2;
	
	x1 = x;		x2 = x;
	y1 = y-s;	y2 = y+s;
	
	RotatePoint(&x1, &y1, nAng, x, y);
	RotatePoint(&x2, &y2, nAng, x, y);
	DrawLine(x1, y1, x2, y2, c);
	
	y1 = y;		y2 = y;
	x1 = x-s;	x2 = x+s;
	
	RotatePoint(&x1, &y1, nAng, x, y);
	RotatePoint(&x2, &y2, nAng, x, y);
	DrawLine(x1, y1, x2, y2, c);
	
}

void SCREEN2D::ScaleAngLine(int nScale, int nAng, int* x, int* y)
{
	int t = data.zoom / nScale;
	*x = mulscale30(t, Cos(nAng));
	*y = mulscale30(t, Sin(nAng));
}

void SCREEN2D::MarkWall(int nWall, int nMarkType)
{
	char m[2] = "\0", fc, bc;
	int x1, y1;
	
	switch(nMarkType)
	{
		case kMarkWallFirst:
			fc = ColorGet(kColorWhite);
			bc = ColorGet(kColorBlack);
			m[0] = 'F';
			break;
		case kMarkWallAlign:
			fc = ColorGet(kColorYellow);
			bc = ColorGet(kColorBlack);
			m[0] = 'A';
			break;
		default:
			return;
	}
	
	avePointWall(nWall, &x1, &y1);
	ScalePoints(&x1, &y1);
	
	offsetPos(0, 8, 0, GetWallAngle(nWall) + kAng90, &x1, &y1, NULL);
	if (prefs.showTags && CaptionGet(OBJ_WALL, nWall))
		offsetPos(0, perc2val(300, pCaptFont->height), 0, 1536, &x1, &y1, NULL);
	
	CaptionPrint(m, x1, y1, 1, fc, (unsigned char)bc, false, qFonts[1]);
}

void SCREEN2D::CaptionPrintLineEdit(int nOffs, int x1, int y1, int x2, int y2, char fc, char bc, QFONT* pFont)
{
	int sx1 = x1, sy1 = y1, sx2 = x2, sy2 = y2;
	int nAng, sLen, cx, cy;
	
	double nGrid, t;
	double nLen = approxDist(x2 - x1, y2 - y1);
	char buf1[32] = "\0", buf2[32] = "\0";
	
	QFONT* pFont1 = pFont;
	QFONT* pFont2 = qFonts[3];
	
	if (data.grid)
	{
		nGrid = (double)(nLen / (2048>>data.grid));
		if (modf(nGrid, &t))
		{
			sprintf(buf1, "%2.1fG", nGrid);
		}
		else
		{
			sprintf(buf1, "%dG", (int)nGrid);
		}
	}
	else
	{
		sprintf(buf1, "%dP", (int)nLen);
	}
	
	ScalePoints(&sx1, &sy1, &sx2, &sy2);
	sLen = perc2val(75, approxDist(sx2 - sx1, sy2 - sy1));
	if (gfxGetTextLen(buf1, pFont1) > sLen)
		return;
	
	if (x1 != x2 && y1 != y2)
	{
		if (x1 > x2)
			swapValues(&x1, &x2);
		if (y1 > y2)
			swapValues(&y1, &y2);
		
		nAng = getangle(x2 - x1, y2 - y1);
		sprintf(buf2, "%dD", ((nAng & kAngMask)*360)/kAng360);
	}
	
	if (buf1[0])
	{
		cx = sx1+((sx2-sx1)>>1); cy = sy1+((sy2-sy1)>>1);
		
		if (nOffs)
			offsetPos(0, nOffs, 0, kAng90, &cx, &cy, NULL);
		
		// print length
		CaptionPrint(buf1, cx, cy, 1, fc, -1, true, pFont1);
		if (buf2[0])
		{
			// print angle
			offsetPos(0, perc2val(110, pFont1->height), 0, kAng90, &cx, &cy, NULL);
			t = pFont2->charSpace, pFont2->charSpace = 2;
			CaptionPrint(buf2, cx, cy, 1, fc, -1, true, qFonts[3]);
			pFont2->charSpace = t;
		}
	}
}

void SCREEN2D::CaptionPrintWallEdit(int nWall, char fc, char bc, QFONT* pFont)
{
	int nNext = wall[nWall].nextwall;
	int x1, y1, x2, y2, nOffs = 0;
	
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	if (prefs.showTags && (CaptionGet(OBJ_WALL, nWall) || nNext >= 0 && CaptionGet(OBJ_WALL, nNext)))
		nOffs = perc2val(150, pCaptFont->height);
	
	CaptionPrintLineEdit(nOffs, x1, y1, x2, y2, fc, bc, pFont);
}

void SCREEN2D::DrawWallMidPoint(int nWall, char c, char s, char thick, char which)
{
	int nAng, nSize, nLen, x1, y1, x2, y2, cx, cy;
	
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	ScalePoints(&x1, &y1, &x2, &y2);
	cx = (x1+x2)>>1; cy = (y1+y2)>>1;
	
	nLen = approxDist(x1 - x2, y1 - y2);
	
	if (OnScreen(cx, cy, s) && nLen >= 256)
	{
		nSize = mulscale12(s, data.zoom);
		if (nSize > 0)
		{
			nAng = (getangle(x2-x1, y2-y1) + kAng90) & kAngMask;
			
			if (which & 0x1)
			{
				x2  = mulscale30(nSize, Cos(nAng));
				y2  = mulscale30(nSize, Sin(nAng));
				DrawLine(cx, cy, cx + x2, cy + y2, c, thick);
			}
			
			if (which & 0x2)
			{
				nAng = (nAng + kAng180) & kAngMask;
				x2  = mulscale30(nSize, Cos(nAng));
				y2  = mulscale30(nSize, Sin(nAng));
				DrawLine(cx, cy, cx + x2, cy + y2, c, thick);
			}
		}
	}
}

void SCREEN2D::DrawWall(int nWall)
{
	walltype* pWall = &wall[nWall];
	int nNext = pWall->nextwall;
	char masked = 0;
	short flags;
	
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	ScalePoints(&x1, &y1, &x2, &y2);
	THICK = 0;
	
	if (nNext >= 0)
	{
		if (nWall == linehighlight)			flags = pWall->cstat;
		else if (nNext == linehighlight)	flags = wall[nNext].cstat, nWall = nNext;
		else								flags = pWall->cstat | wall[nNext].cstat;
		
		masked = ((flags & kWallMasked) > 0);
		THICK = ((flags & kWallBlock) > 0);
	}

	color = GetWallColor(nWall, (nWall == linehighlight) ? h : 0);
	DrawLine(x1, y1, x2, y2, color, THICK);
	
	if (masked)
	{
		color = ColorGet(kColorLightMagenta,  0);
		DrawLine(x1, y1, x2, y2, color, 0, 0xFFF);
	}
	
}

char SCREEN2D::OnScreen(int x1, int y1, int x2, int y2)
{
	// Check if *some part* of the rect on the screen
	if ((x2 > view.wx1 && x1 < view.wx2))
	{
		if (y2 > view.wy1 && y1 < view.wy2)
			return 1;
	}

	return 0;
}

char SCREEN2D::InHighlight(int x, int y)
{	
	if (hgltType)
	{
		int hx1 = hgltx1;
		int hx2 = hgltx2;
		int hy1 = hglty1;
		int hy2 = hglty2;
		
		ScalePoints(&hx1, &hy1, &hx2, &hy2);
		if (hx1 > hx2) swapValues(&hx1, &hx2);
		if (hy1 > hy2) swapValues(&hy1, &hy2);
		return (rngok(x, hx1, hx2) && rngok(y, hy1, hy2));
	}
	
	return false;
}

const char* SCREEN2D::CaptionGet(int nType, int nID)
{
	const char* t = NULL;
	switch (nType)
	{
		case OBJ_WALL:
		case OBJ_MASKED:
			t = ExtGetWallCaption(nID, prefs.showTags);
			break;
		case OBJ_SPRITE:
			t = ExtGetSpriteCaption(nID, prefs.showTags);
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
			t = ExtGetSectorCaption(nID, prefs.showTags);
			break;
	}
	
	return (!isempty(t)) ? t : NULL;
}

void SCREEN2D::CaptionPrint(const char* text, int cx, int cy, char pd, char fc, short bc, char shadow, QFONT* pFont)
{
	int x1, x2, y1, y2, wh, hg;
	wh = gfxGetTextLen((char*)text, pFont) >> 1;
	hg = pFont->height >> 1;
	
	x1 = cx - wh - pd,	x2 = cx + wh + pd;
	y1 = cy - hg - pd,	y2 = cy + hg + pd;
	
	if (OnScreen(x1, y1, x2, y2))
	{
		if (bc >= 0)
		{
			gfxSetColor((unsigned char)bc);
			gfxFillBox(x1, y1+1, x2, y2-1);
			gfxHLine(y1, x1+1, x2-2);
			gfxHLine(y2-1, x1+1, x2-2);
		}
		
		if (!shadow)
		{
			gfxDrawText(x1 + pd, y1 + pd, fc, (char*)text, pFont);
		}
		else
		{
			gfxPrinTextShadow(x1 + pd, y1 + pd, fc, (char*)text, pFont);
		}
	}
}

void SCREEN2D::CaptionPrintSector(int nSect, char hover, QFONT* pFont)
{	
	int cx, cy;
	const char* caption = ExtGetSectorCaption(nSect, prefs.showTags);
	unsigned char fc, pd, bc;
	
	if (!caption[0])
		return;
	
	avePointSector(nSect, &cx, &cy);
	ScalePoints(&cx, &cy);
	
	fc = ColorGet(kColorBlack);
	bc = ColorGet(kColorLightGray, hover);
	
	pd = (hover) ? kCaptPadHover : kCaptPadNormal;
	CaptionPrint(caption, cx, cy, pd, fc, bc, false, pFont);

}

void SCREEN2D::CaptionPrintSprite(int nSpr, char hover, QFONT* pFont)
{
	int cx, cy; short flags;
	const char* caption = ExtGetSpriteCaption(nSpr, prefs.showTags);
	unsigned char fc, pd, bc, bh = 0, dark = 0;
	
	if (!caption[0])
		return;
	
	cx = cscalex(sprite[nSpr].x);
	cy = cscaley(sprite[nSpr].y);
	flags = sprite[nSpr].cstat;
	
	bh = (flags & kSprBlock);
	if (flags & kSprMoveForward)
	{
		bc = ColorGet(kColorLightBlue, 	(hover) ? h : !bh);
		dark = !bh;
	}
	else if (flags & kSprMoveReverse)		bc = ColorGet(kColorLightGreen, (hover) ? h : !bh);
	else									bc = ColorGet(kColorCyan, 		(hover) ? h :  bh);
		
	if (flags & kSprHitscan)				fc = ColorGet(kColorRed, 		(hover && h));
	else if (dark)							fc = ColorGet(kColorLightGray, 	(hover && h));
	else									fc = ColorGet(kColorBlack, 		(hover && h));
	
	pd = (hover) ? kCaptPadHover : kCaptPadNormal;
	CaptionPrint(caption, cx, cy, pd, fc, bc, false, pFont);
}

void SCREEN2D::CaptionPrintWall(int nWall, char hover, QFONT* pFont)
{
	int cx, cy;
	const char* caption = ExtGetWallCaption(nWall, prefs.showTags);
	unsigned char fc, pd, bc;
	
	if (!caption[0])
		return;
	
	fc = ColorGet(kColorBlack);
	bc = ColorGet(kColorRed, hover && h);
	
	avePointWall(nWall, &cx, &cy);	
	ScalePoints(&cx, &cy);
	
	pd = (hover) ? kCaptPadHover : kCaptPadNormal;
	CaptionPrint(caption, cx, cy, pd, fc, bc, false, pFont);
}

void SCREEN2D::SetView(int x1, int y1, int x2, int y2, char clip)
{
	view.wx1 = x1;	view.wy1 = y1;
	view.wx2 = x2;	view.wy2 = y2;
	
	view.wwh = view.wx2 - view.wx1;
	view.whg = view.wy2 - view.wy1;
	
	view.wcx = view.wx1 + (view.wwh >> 1);
	view.wcy = view.wy1 + (view.whg >> 1);
	
	prefs.clipView = (clip > 0);
	
	if (fullscreen && ydim <= 600)
	{
		pCaptFont = qFonts[3];
		pEditFont = qFonts[3];
	}
	else
	{
		pCaptFont = qFonts[0];
		pEditFont = qFonts[1];
	}	
}

void SCREEN2D::ScreenClear()
{
	gfxSetColor(ColorGet(kColorGrey30));
	gfxFillBox(view.wx1, view.wy1, view.wx2, view.wy2);
}

void SCREEN2D::ScreenDraw(void)
{
	XSECTOR* pXSect;
	int nSect, nPat, i, j, e, t;
	char fc, bc;
	
	x1 = x2 = x3 = 0;
	y1 = y2 = y3 = 0;
	
	if (prefs.clipView)
	{
		gfxBackupClip();
		gfxSetClip(view.wx1, view.wy1, view.wx2, view.wy2);
	}
	
	spritetype* pHSpr = NULL;
	if (pointhighlight >= 0 && (pointhighlight & 0xc000))
		pHSpr = &sprite[pointhighlight & 16383];
	

	if (!prefs.showVertex)			vertexSize = 0;
	else if (data.zoom	<= 512)		vertexSize = 2;
	else if (data.zoom	<= 1024)	vertexSize = 4;
	else							vertexSize = 6;

	gridSize = (data.grid) ? mulscale14(2048 >> data.grid, data.zoom) : 0;
	
	if (prefs.showMap)
	{
		if (data.grid)
		{
			if (gridSize > 1)
			{
				ShowMap(prefs.showMap);
				DrawGrid();
			}
			else
			{
				DrawGrid(); // fill the board box
				ShowMap(prefs.showMap);
			}
		}
		else
			ShowMap(prefs.showMap);	
	}
	else if (data.grid)
		DrawGrid();


	/* flood sectors using translucent rotatesprite */
	////////////////////////////
	if (prefs.useTransluc)
	{
		// flood current sector
		if (sectorhighlight >= 0)
		{
			LayerOpen();
			color = ColorGet(kColorGrey28);
			FillSector(sectorhighlight, color, 1);
			LayerShowAndClose(kRSTransluc);
		}
		
		if (highlightsectorcnt > 0 || joinsector[0] >= 0)
		{
			LayerOpen();
			color = ColorGet(kColorLightGreen);
			
			// flood sectors in a highlight
			for (i = 0; i < highlightsectorcnt; i++)
			{
				nSect = highlightsector[i];
				if (nSect != joinsector[0])
					FillSector(nSect, color, 1);
			}
			
			// flood sector waiting for merging
			if (joinsector[0] >= 0)
			{
				color = ColorGet(kColorYellow);
				FillSector(joinsector[0], color, 1);
			}
			
			LayerShowAndClose(kRSTransluc);
		}
	}
	else
	{
		// flood sectors in a highlight
		if (highlightsectorcnt > 0)
		{
			color = ColorGet(kColorLightGreen);
			i = highlightsectorcnt;
			while(--i >= 0)
				FillSector(highlightsector[i], color);
		}
		
		// flood sector waiting for merging
		if (joinsector[0] >= 0)
		{
			color = ColorGet(kColorYellow);
			FillSector(joinsector[0], color);
		}
	}
	
	/* draw busy / motion progress of the sector */
	////////////////////////////
	if (gPreviewMode)
	{
		for (i = 0; i < numsectors; i++)
		{
			if ((pXSect = GetXSect(&sector[i])) != NULL)
			{
				if (rngok(pXSect->busy, 1, 65536))
				{
					if ((pXSect->unused1 && h) || !pXSect->unused1)
						FillSector(i, ColorGet((pXSect->state) ? kColorRed : kColorBlue));
				}
				else if ((pXSect->bobFloor || pXSect->bobCeiling) && (pXSect->bobAlways || pXSect->state))
				{
					FillSector(i, ColorGet(kColorDarkGray));
				}
			}
		}
	}
	
	/* draw avepoint of the sector */
	////////////////////////////
	if ((gFrameClock & 64) && sectorhighlight >= 0)
	{
		avePointSector(sectorhighlight, &x1, &y1);
		ScalePoints(&x1, &y1);
		
		gfxSetColor(ColorGet(kColorWhite));
		gfxPixel(x1, y1);
	}
	
	/* draw wall midpoint closest to mouse cursor */
	////////////////////////////
	if (linehighlight >= 0)
	{
		getclosestpointonwall(mousxplc, mousyplc, linehighlight, &x1, &y1);
		if (gridlock)
			doGridCorrection(&x1, &y1, data.grid);
		
		getWallCoords(linehighlight, &x2, &y2, &x3, &y3);
		if ((x1 != x2 && x1 != x3) || (y1 != y2 && y1 != y3))
		{
			ScalePoints(&x1, &y1);
			DrawIconCross(x1, y1, GetWallColor(linehighlight, false), 2);
		}
	}

	/* draw walls */
	////////////////////////////
	fc = ColorGet(kColorGrey28);
	bc = ColorGet(kColorBrown);
	for (i = 0; i < numwalls; i++)
	{
		if (wall[i].nextwall > i)
			continue;
		
		DrawWall(i);
		if (data.zoom >= kZoomVertexAll && vertexSize)
		{
			if (OnScreen(x1, y1, vertexSize))	DrawVertex(x1, y1, fc, bc, vertexSize);
			if (OnScreen(x2, y2, vertexSize))	DrawVertex(x2, y2, fc, bc, vertexSize);
		}
	}
	
	/* redraw hovered wall line */
	////////////////////////////
	if (linehighlight >= 0)
	{
		DrawWall(linehighlight);
		if (data.zoom >= kZoomVertexAll && vertexSize)
		{
			if (OnScreen(x1, y1, vertexSize))	DrawVertex(x1, y1, fc, bc, vertexSize);
			if (OnScreen(x2, y2, vertexSize))	DrawVertex(x2, y2, fc, bc, vertexSize);
			
			gfxSetColor(ColorGet(kColorWhite, h));
			gfxPixel(x1, y1);
		}
	}
	
	/* draw highlight sector walls that's gonna be detached */
	////////////////////////////
	fc = ColorGet(kColorYellow);
	if (highlightsectorcnt > 0)
	{
		i = highlightsectorcnt;
		while(--i >= 0)
		{
			getSectorWalls(highlightsector[i], &j, &e);
			while(j <= e)
			{
				if ((t = wall[j].nextsector) >= 0 && hgltCheck(OBJ_SECTOR, t) < 0)
				{
					getWallCoords(j, &x1, &y1, &x2, &y2); ScalePoints(&x1, &y1, &x2, &y2);
					DrawLine(x1, y1, x2, y2, ColorGet(kColorGrey16), 0, kPatDotted);
					
					if (data.zoom >= kZoomVertexAll && vertexSize)
					{
						if (OnScreen(x1, y1, vertexSize))	DrawVertex(x1, y1, fc, bc, vertexSize);
						if (OnScreen(x2, y2, vertexSize))	DrawVertex(x2, y2, fc, bc, vertexSize);
					}
				}
				
				j++;
			}
		}
	}
	
	/* redraw highlighted wall points */
	////////////////////////////
	if (h && data.zoom >= kZoomVertexHglt && vertexSize)
	{
		fc = ColorGet(kColorYellow, h);
		bc = ColorGet(kColorBrown, h);
		
		if (hgltType)
		{
			for (i = 0; i < numwalls; i++)
			{
				x1 = cscalex(wall[i].x);
				y1 = cscaley(wall[i].y);
				if (OnScreen(x1, y1, vertexSize))
				{
					if (highlightcnt <= 0 || !TestBitString(hgltwall, i))
					{
						if (!InHighlight(x1, y1))
							continue;
					}
					
					DrawVertex(x1, y1, fc, bc, vertexSize);
				}
			}
		}
		else
		{
			for (i = 0; i < highlightcnt; i++)
			{
				if ((highlight[i] & 0xC000) != 0)
					continue;
				
				j = highlight[i];
				x1 = cscalex(wall[j].x);
				y1 = cscaley(wall[j].y);
				
				if (OnScreen(x1, y1, vertexSize))
					DrawVertex(x1, y1, fc, bc, vertexSize);
			}
		}
		
		if (pointhighlight >= 0 && (pointhighlight & 0xC000) == 0)
		{
			i = pointhighlight;
			x1 = cscalex(wall[i].x);
			y1 = cscaley(wall[i].y);
			if (OnScreen(x1, y1, vertexSize))
				DrawVertex(x1, y1, fc, bc, vertexSize);
		}
	}
	
	if (prefs.showSprites)
	{
		/* draw some marker tracers */
		////////////////////////////
		for (i = headspritestat[kStatMarker]; i >= 0; i = nextspritestat[i])
		{
			pSpr = &sprite[i];
			if (pSpr->type == kMarkerAxis || !rngok(pSpr->owner, 0, numsectors))
				continue;
			
			HOVER = (pSpr->owner == sectorhighlight);
			x1 = cscalex(pSpr->x);
			y1 = cscaley(pSpr->y);
			
			switch(pSpr->type)
			{
				case kMarkerOff:
					pXSect = GetXSect(&sector[pSpr->owner]);
					if (pXSect && rngok(pXSect->marker1, 0, kMaxSprites))
					{
						spritetype* pSpr2 = &sprite[pXSect->marker1];
						if (!HOVER)
							HOVER = ((pHSpr == pSpr2 || pHSpr == pSpr) && h);
						
						x2 = cscalex(pSpr2->x);
						y2 = cscaley(pSpr2->y);
						color = ColorGet(kColorYellow);
						t = (HOVER) ? kPatDotted : kPatNormal;
						DrawArrow(x1, y1, x2, y2, color, data.zoom, kAng30, 0, t, t);
					}
					break;
				case kMarkerWarpDest:
					if (pSpr->owner != pSpr->sectnum)
					{
						avePointSector(pSpr->owner, &x2, &y2);
						if (!HOVER)
							HOVER = (pHSpr == pSpr && h);
						
						ScalePoints(&x2, &y2);
						color = ColorGet(kColorDarkGray, HOVER);
						DrawArrow(x2, y2, x1, y1, color, data.zoom, kAng30, 0, kPatDotted);
					}
					break;
			}
		}
		
		/* draw sprites */
		////////////////////////////
		if (data.zoom >= kZoomSprites)
		{
			for (i = 0; i < numsectors; i++)
			{
				for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
					DrawSprite(&sprite[j]);
			}
			
			/* redraw hovered sprite */
			if (pHSpr)
				DrawSprite(pHSpr);
		}
	}
	

		
	/* sector drawing process */
	////////////////////////////
	if (pGLBuild)
	{
		pGLBuild->Draw(this);
		
		// draw midpoints on all walls of a sector
		if ((i = sectorhighlight) >= 0)
		{
			getSectorWalls(i, &j, &e);
			while(j <= e)
			{
				color = GetWallColor(j, (h && j == linehighlight));
				DrawWallMidPoint(j, color);
				j++;
			}
		}
	}
	else if (linehighlight >= 0) // draw midpoint for current wall
	{
		DrawWallMidPoint(linehighlight, GetWallColor(linehighlight, h));
	}

	if (prefs.showTags)
	{
		/* draw object captions */
		////////////////////////////
		if (data.zoom >= kZoomTags)
		{
			for (i = 0; i < numsectors; i++)
				CaptionPrintSector(i, (sectorhighlight == i), pCaptFont);
			
			for (i = 0; i < numwalls; i++)
				CaptionPrintWall(i, TestBitString(hgltwall, i), pCaptFont);
			
			if (prefs.showSprites)
			{
				for (i = 0; i < numsectors; i++)
				{
					for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
						CaptionPrintSprite(j, TestBitString(hgltspri, j), pCaptFont);
				}
			}
			
			/* redraw captions of a hovered objects	*/
			////////////////////////////
			if (pointhighlight >= 0)
			{
				if ((pointhighlight & 0xc000) != 16384)
				{
					CaptionPrintWall(pointhighlight, true, pCaptFont);
				}
				else if (prefs.showSprites)
				{
					CaptionPrintSprite(pointhighlight&16383, true, pCaptFont);
				}
			}
			else if (linehighlight >= 0)
				CaptionPrintWall(linehighlight, true, pCaptFont);
		}
	}
	
	/* indicate first and alignto walls */
	////////////////////////////
	if (sectorhighlight >= 0 && data.zoom > kZoomFAWalls)
	{
		sectortype* pSect = &sector[sectorhighlight];
		if (gFrameClock & (kHClock<<1))
			MarkWall(pSect->wallptr, kMarkWallFirst);
		else if (pSect->alignto)
			MarkWall(pSect->alignto + pSect->wallptr, kMarkWallAlign);
	}
	
	/* draw start arrow */
	////////////////////////////
	x1 = cscalex(startposx); y1 = cscaley(startposy);
	DrawArrow(x1, y1, ColorGet(kColorBrown), startang, data.zoom, (int)80, (char)true);
	
	/* draw current position */
	////////////////////////////
	x1 = cscalex(posx); y1 = cscaley(posy);
	DrawArrow(x1, y1, ColorGet(kColorWhite), data.ang, data.zoom);
	
	/* draw highlight */
	////////////////////////////
	if (prefs.showHighlight && hgltType)
	{
		x1 = hgltx1;	x2 = hgltx2;
		y1 = hglty1;	y2 = hglty2;
		
		switch(hgltType)
		{
			case kHgltPoint:
				fc = ColorGet(kColorMagenta);
				bc = ColorGet(kColorLightCyan);
				break;
			default:
				fc = ColorGet(kColorLightGreen);
				bc = ColorGet(kColorLightGreen);
				break;
		}
		
		ScalePoints(&x1, &y1, &x2, &y2);
		if (x1 > x2) swapValues(&x1, &x2);
		if (y1 > y2) swapValues(&y1, &y2);
		DrawRect(x1-1, y1-1, x2, y2, fc);
		
		if (prefs.useTransluc)
		{
			gfxSetColor(bc);
			LayerOpen();
			gfxFillBox(x1, y1, x2, y2);
			LayerShowAndClose(kRSTransluc);
		}
	}
		
	if (pointhighlight >= 0)
	{
		/* show how many sprites is under curstor */
		////////////////////////////
		if (pHSpr)
		{
			if (data.zoom >= kZoomSpritesUnder && data.sprUnderCursor > 1)
			{
				pFont = qFonts[0];
				color = ColorGet(kColorYellow);
				sprintf(buffer, "[%d]", data.sprUnderCursor);
				
				i = gfxGetTextLen(buffer, pFont);
				CaptionPrint(buffer, searchx + i, searchy + pFont->height, 0, color, -1, true, pFont);
			}
		}
		/* show info for walls that you drag */
		////////////////////////////
		else if (pointdrag >= 0 && data.zoom >= kZoomEditCaption)
		{
			fc = ColorGet(kColorYellow);
			bc = ColorGet(kColorBrown);
			getWallCoords(pointdrag, &x3, &y3);
			for (i = 0; i < numwalls; i++)
			{
				if (wall[i].nextwall > i)
					continue;
				
				getWallCoords(i, &x1, &y1, &x2, &y2);
				if ((x3 == x1 && y3 == y1) || (x3 == x2 && y3 == y2))
					CaptionPrintWallEdit(i, fc, (unsigned char)bc, pEditFont);
			}
		}
	}
	
	if (prefs.showFeatures)
	{
		/* draw circle wall and door wizard features */
		////////////////////////////
		if (pGCircleW)									pGCircleW->Draw(this);
		else if (pGDoorSM && pGDoorSM->StatusGet() > 0) pGDoorSM->Draw(this);
		else if (pGDoorR && pGDoorR->StatusGet() > 0)	pGDoorR->Draw(this);
		else if (pGLShape)								pGLShape->Draw(this);
		else if (pGArc)									pGArc->Draw(this);
			
		
		/* draw RX/TX tracker */
		////////////////////////////
		if (CXTracker::HaveObjects())
			CXTracker::Draw(this);
		
		/* draw map notes */
		////////////////////////////
		if (gCmtPrefs.enabled)
			gCommentMgr.Draw(this);
	}
		
	if (prefs.clipView)
		gfxRestoreClip();
}

void SCREEN2D::DrawSpritePathMarker(void)
{
	char c; int i;
	
	if (pXSpr && pXSpr->data2 >= 0 && pXSpr->data1 != pXSpr->data2)
	{
		for (i = headspritestat[kStatPathMarker]; i != -1; i = nextspritestat[i])
		{
			spritetype *pSpr2 = &sprite[i];
			if (pSpr2->index == pSpr->index)
				continue;
			
			XSPRITE* pXSpr2 = GetXSpr(pSpr2);
			if (!pXSpr || pXSpr2->data1 != pXSpr->data2)
				continue;
			
			x2 = cscalex(pSpr2->x); y2 = cscaley(pSpr2->y);
			DrawLine(x1, y1, x2, y2, ColorGet(kColorLightMagenta, BLINK), HOVER, kPatDotted);
		}
	}
	
	c = GetSpriteColor(pSpr, BLINK);
	if (gModernMap && HOVER)
	{
		i = mulscale10(ClipLow(pSpr->clipdist << 1, 8), data.zoom);
		DrawCircle(x1, y1, i, c);
	}

	if (OnScreen(x1, y1, 3))
	{
		DrawAngLine(100, pSpr->ang, x1, y1, c, THICK);
		DrawIconFaceSpr_NORMAL(x1, y1, c);
		
		c = ColorGet(kColorYellow, BLINK);
		DrawIconCross(x1, y1, c, 1);
	}
}

void SCREEN2D::DrawSprite(spritetype* pSprite)
{
	int i = 0, dx[4], dy[4];
	
	pSpr	= pSprite;
	pXSpr	= GetXSpr(pSpr);
	THICK   = (pSpr->cstat & kSprBlock);
	x1		= cscalex(pSpr->x);
	y1		= cscaley(pSpr->y);

	if ((pSpr->index | 0x4000) == pointhighlight) HOVER = 1;
	else if (highlightcnt > 0 && TestBitString(hgltspri, pSpr->index)) HOVER = 1;
	else if (InHighlight(x1, y1)) HOVER = 1;
	else HOVER = 0;

	BLINK = (HOVER && h);
	color = GetSpriteColor(pSpr, BLINK);
	if (HOVER && pXSpr && pXSpr->triggerProximity)
	{
		if (gModernMap)
		{
			DrawCircle(x1, y1, mulscale10(ClipLow(pSpr->clipdist * 3, 8), data.zoom), color, false, kPatDotted);
		}
		else if (pSpr->statnum == kStatThing || pSpr->statnum == kStatDude)
		{
			DrawCircle(x1, y1, mulscale10(96, data.zoom), color, false, kPatDotted);
		}
	}
	
	if (tilesizx[pSpr->picnum])
	{
		switch(pSpr->cstat & kSprRelMask)
		{
			case kSprWall:
				GetSpriteExtents(pSpr, &dx[0], &dy[0], &dx[1], &dy[1], NULL, NULL, 0x0);
				ScalePoints(&dx[0], &dy[0], &dx[1], &dy[1]);
				DrawLine(dx[0], dy[0], dx[1], dy[1], color, THICK, kPatDotted);
				break;
			case kSprFloor:
			case kSprSloped:
				if (HOVER || data.zoom >= 1280 || pointdrag >= 0)
				{
					GetSpriteExtents(pSpr, &dx[0], &dy[0], &dx[1], &dy[1], &dx[2], &dy[2], &dx[3], &dy[3], 0x0);
					ScalePoints(&dx[0], &dy[0], &dx[1], &dy[1]);
					ScalePoints(&dx[2], &dy[2], &dx[3], &dy[3]);
					
					DrawLine(dx[0], dy[0], dx[1], dy[1], color, THICK, kPatDotted2); // T
					DrawLine(dx[1], dy[1], dx[2], dy[2], color, THICK, kPatDotted2); // R
					DrawLine(dx[2], dy[2], dx[3], dy[3], color, THICK, kPatDotted2); // B
					DrawLine(dx[3], dy[3], dx[0], dy[0], color, THICK, kPatDotted2); // L

					if (HOVER)
					{
						DrawLine(dx[1], dy[1], dx[3], dy[3], color, 0, kPatDotted2); // TL->BR
						DrawLine(dx[0], dy[0], dx[2], dy[2], color, 0, kPatDotted2); // TR->BL
					}
				}
				break;
		}
	}
	
	switch(pSpr->statnum)
	{
		case kStatPathMarker:	DrawSpritePathMarker();		break;
		case kStatMarker:		DrawSpriteMarker();			break;
		case kStatAmbience:		DrawSpriteAmbient();		break;
		case kStatFX:			DrawSpriteFX();				break;
		case kStatExplosion:	DrawSpriteExplosion();		break;
		case kStatProjectile:	DrawSpriteProjectile();		break;
		default:
			switch(pSpr->type)
			{
				case kMarkerSPStart:
				case kMarkerMPStart:
					DrawSpritePlayer();
					break;
				case kModernLaserGen:
					DrawSpriteLaser();
					break;
				case kModernStealthRegion:
					DrawSpriteStealth();
					break;
				default:
					DrawSpriteCommon();
					break;
			}
			break;
	}
}

void SCREEN2D::DrawSpriteMarker(void)
{
	char c;
	if (!OnScreen(x1, y1, 4) || (pSpr->owner == sectorhighlight && !h))
		return;
	
	switch(pSpr->type)
	{
		case kMarkerOff:
		case kMarkerOn:
			if (pSpr->owner == sectorhighlight) c = kColorLightGreen;
			else c = kColorLightCyan;
				
			c = ColorGet(c, BLINK);
			DrawIconFaceSpr_SMALL(x1, y1, c);
			if (pSpr->ang)
				DrawAngLine(128, pSpr->ang, x1, y1, c);
			
			break;
		case kMarkerAxis:
		case kMarkerWarpDest:
			if (pSpr->owner == sectorhighlight)		c = kColorLightGreen;
			else if (pSpr->type == kMarkerWarpDest) c = kColorBrown;
			else c = kColorLightCyan;
				
			c = ColorGet(c, BLINK);
			DrawIconMarker(x1, y1, c);
			DrawAngLine(128, pSpr->ang, x1, y1, c);
			break;
	}
}

void SCREEN2D::DrawSpriteAmbient(void)
{
	int i, r[2];
	char c;
	
	if (OnScreen(x1, y1, 4))
	{
		if (pXSpr)
		{
			c = ColorGet((pXSpr->state) ? kColorYellow : kColorLightGray, BLINK);
		}
		else
		{
			c = GetSpriteColor(pSpr, BLINK);
		}
		
		DrawIconFaceSpr_NORMAL(x1, y1, c);
		DrawIconCross(x1, y1, c);
	}
	
	if (pXSpr && prefs.ambRadius)
	{
		char showOnHover = prefs.ambRadiusHover;
		if ((showOnHover && HOVER) || (!showOnHover && (!HOVER || h)))
		{
			r[0] = (prefs.ambRadius & 0x02) ? mulscale10(pXSpr->data2, data.zoom) : 0;
			r[1] = (prefs.ambRadius & 0x01) ? mulscale10(pXSpr->data1, data.zoom) : 0;
			
			i = 0;
			while(i < 2)
			{
				if (r[i])
				{
					if (i == 1)				c = (pXSpr->state) ? kColorYellow : kColorLightGray;
					else if (!pXSpr->state)	c = kColorDarkGray;
					else					c = kColorBrown;		
					
					DrawCircle(x1, y1, r[i], ColorGet(c));
				}
				i++;
			}
		}
	}
}

void SCREEN2D::DrawSpriteStealth(void)
{
	char c, more;
	
	if (gModernMap)
	{
		if (pXSpr && pXSpr->data1 >= 0)
		{
			more = (pXSpr->data2 > 0 || pXSpr->data3 > 0);
			c = ColorGet((more) ? kColorLightRed : kColorLightCyan);
			
			if (pXSpr->data1 > 0)
				DrawCircle(x1, y1, mulscale10(pXSpr->data1, data.zoom), c, false, kPatDashed);
			else if (HOVER)
				FillSector(pSpr->sectnum, c);
			
			if (OnScreen(x1, y1, 4))
			{
				c = ColorGet((more) ? kColorLightRed : kColorLightCyan, BLINK);
				DrawIconFaceSpr_NORMAL(x1, y1, c);
				DrawIconCross(x1, y1, c, 4);
			}
			
			return;
		}
	}
	
	DrawSpriteCommon();
}

void SCREEN2D::DrawSpriteLaser(void)
{
	DrawSpriteCommon();
	if (gModernMap && gPreviewMode)
		laserGet(pSpr)->RayShow(this);
}

void SCREEN2D::DrawSpriteFX(void)
{
	char c = GetSpriteColor(pSpr, h);
	DrawIconCross(x1, y1, c, 3);
}

void SCREEN2D::DrawSpriteExplosion(void)
{
	char c; int r;
	if (rngok(pSpr->type, 0, kExplosionMax))
	{
		r = mulscale10(explodeInfo[pSpr->type].radius, data.zoom);
		c = ColorGet((h) ? kColorLightGray : kColorDarkGray);
		DrawCircle(x1, y1, r, c);
		DrawIconCross(x1, y1, c, 4);
		return;
	}
}

void SCREEN2D::DrawSpriteProjectile(void)
{
	char c = ColorGet((h) ? kColorLightBlue : kColorLightGreen, HOVER);
	DrawCircle(x1, y1, mulscale12(pSpr->clipdist, data.zoom), c);
	DrawIconFaceSpr_SMALL(x1, y1, c);
	DrawIconCross(x1, y1, c, 1);
}


void SCREEN2D::DrawSpritePlayer(void)
{
	char c = kColorGrey24;
	static char playerColors[kMaxPlayers] =
	{
		kColorWhite,		kColorLightBlue,
		kColorLightRed,		kColorLightGreen,
		kColorLightMagenta,	kColorLightGray,
		kColorYellow,		kColorGrey24,
	};
	
	if (pXSpr && rngok(pXSpr->data1, 0, kMaxPlayers))
		c = playerColors[pXSpr->data1];
	
	c = ColorGet(c, BLINK);
	DrawArrow(x1, y1, c, (int)pSpr->ang, (int)data.zoom, (int)96, (char)(pSpr->type == kMarkerSPStart));
}

void SCREEN2D::DrawSpriteCommon(void)
{
	if (!OnScreen(x1, y1, 8))
		return;
	
	
	switch(pSpr->cstat & kSprRelMask)
	{
		case kSprWall:
			if (!tilesizx[pSpr->picnum])
			{
				ScaleAngLine(80, pSpr->ang + kAng90, &x2, &y2);
				DrawLine(x1-x2, y1-y2, x1+x2, y1+y2, color, THICK);
			}
			
			if (!THICK)
				DrawIconFaceSpr_SMALL(x1, y1, color);
			
			DrawAngLine(128, pSpr->ang, x1, y1, color, THICK); // face
			if (!(pSpr->cstat & kSprOneSided))
				DrawAngLine(182, pSpr->ang + kAng180, x1, y1, color, THICK); // back
			break;
		case kSprFloor:
			DrawAngLine(200, pSpr->ang, x1, y1, color, THICK);
			DrawSquare(x1, y1, color, 3 + THICK);
			break;
		case kSprSloped:
			DrawAngLine(200, pSpr->ang, x1, y1, color, THICK);
			DrawVertex(x1, y1, color, color, 8 + THICK);
			break;
		default:
			DrawAngLine(80, pSpr->ang, x1, y1, color, THICK);
			if (THICK) DrawIconFaceSpr_NORMAL(x1, y1, color);
			else DrawIconFaceSpr_SMALL(x1, y1, color);
			break;
	}
}

void SCREEN2D::DrawGrid()
{
	int i, x, y, xp1, yp1, xp2, yp2;
	int dst;

	dst = 2048 >> data.grid;
	GetPoint(view.wx1, view.wy1, &xp1, &yp1, 1);
	GetPoint(view.wx2, view.wy2, &xp2, &yp2, 1);
		
	doGridCorrection(&xp1, &yp1, data.grid);
	doGridCorrection(&xp2, &yp2, data.grid);
		
	x1 = cscalex(xp1);	y1 = cscaley(yp1);
	x2 = cscalex(xp2);	y2 = cscaley(yp2);
	
	if (gridSize > 1)
	{
		gfxSetColor(ColorGet(kColorGrey29));
		while(xp1 <= xp2) gfxVLine(cscalex(xp1), y1, y2), xp1+=dst;
		while(yp1 <= yp2) gfxHLine(cscaley(yp1), x1, x2), yp1+=dst;
		return;
	}
	
	gfxSetColor(ColorGet(kColorGrey29));
	gfxFillBox(x1, y1, x2, y2);
	return;
}

void SCREEN2D::LayerOpen()
{
	int wh = xdim;
	int hg = ydim;
	int nTile = gSysTiles.drawBuf;
	if (waloff[nTile])
	{
		memset((void*)waloff[nTile], 255, wh*hg);
		setviewtotile(nTile, hg, wh);
	}
}

void SCREEN2D::LayerShowAndClose(char flags)
{
	LayerClose();
	LayerShow(flags);
}

void SCREEN2D::LayerClose()
{
	setviewback();
}

void SCREEN2D::LayerShow(char flags)
{
	rotatesprite
	(
		0<<16, 0<<16, 0x10000, kAng90, gSysTiles.drawBuf, 0, 0,
		kRSCorner | kRSNoMask | kRSNoClip | kRSYFlip | flags,
		view.wx1, view.wy1, view.wx2, view.wy2
	);
}

void SCREEN2D::ShowMap(char flags)
{
	int op = pixelaspect;
	Rect* b;
	
	pixelaspect = 65535;
	if (prefs.clipView)
	{
		b = new Rect(windowx1, windowy1, windowx2, windowy2);
		setview(view.wx1, view.wy1, ClipHigh(view.wx2, xdim-1), ClipHigh(view.wy2, ydim-1));
	}
	
	flags = 12;
	if (prefs.showMap > 1)
		flags++;
		
	ExtPreCheckKeys();
	drawmapview
	(
		data.camx, data.camy, data.zoom, 1536,
		view.wcx<<1, view.wcy<<1,
		flags
	);
	
	pixelaspect = op;
	if (prefs.clipView)
		setview(b->x0, b->y0, b->x1, b->y1);

}

void SCREEN2D::ScalePoints(int* x1, int* y1, int* x2, int* y2)
{
	if (x1) *x1 = cscalex(*x1);
	if (y1) *y1 = cscaley(*y1);
	if (x2) *x2 = cscalex(*x2);
	if (y2) *y2 = cscaley(*y2);
}