#include <cmath>
#include "xmp2dscr.h"
#include "screen.h"
#include "editor.h"
#include "nnexts.h"
#include "xmpmisc.h"
#include "xmpstub.h"
#include "xmpact.h"
#include "tile.h"
#include "hglt.h"
#include "tracker.h"
#include "xmpstr.h"
#include "img2tile.h"


#define kCaptPadNormal		2
#define kCaptPadHover		kCaptPadNormal + 1
#define kZoomTags			512
#define kZoomSpritesUnder	256
#define kZoomSprites		128
#define kZoomEditCaption	256
#define kZoomEditSmallFont	128
#define kZoomFAWalls		100
#define kZoomVertexAll		300
#define kZoomVertexHglt		256
#define kMinWallLengthG		64
#define kMinWallLengthA		32

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
kPrefClrBase	= 0,
kPrefClrWal,
kPrefClrSpr,
kPrefClrSec,
kPrefClrMax,
};

static NAMED_TYPE gColorSections[] = 
{
	{kPrefClrBase,	"Main"},
	{kPrefClrWal, 	"Wall"},
	{kPrefClrSpr, 	"Sprite"},
	{kPrefClrSec, 	"Sector"},
	{-1,			NULL},
};

enum
{
kMarkWallFirst	= 0,
kMarkWallAlign,
};

SCREEN2D gScreen2D;

void SCREEN2D::ColorInit(IniFile* pIni)
{
	char key[256], val[256], group[32], *pKey, *pVal;
	NAMED_TYPE* pColors = gColorSections;
	unsigned char rgb[3];
	int nBase, nID, nPrevNode;
	int t, i;
	int o;
	
	while(pColors->name)
	{
		if (rngok(pColors->id, kPrefClrBase, kPrefClrMax))
		{
			sprintf(group, "Colors.%s", pColors->name);
			nPrevNode = -1; pKey = NULL; pVal = NULL;
			
			switch(pColors->id)
			{
				case kPrefClrBase:	nBase = kClrEdBase;		break;
				case kPrefClrWal:	nBase = kClrWalBase;	break;
				case kPrefClrSpr:	nBase = kClrSprBase;	break;
				case kPrefClrSec:	nBase = kClrSectBase;	break;
			}
			
			while(pIni->GetNextString(NULL, &pKey, &pVal, &nPrevNode, group))
			{
				if (!isufix(pKey))
					continue;
				
				nID = atoi(pKey) + nBase;
				if (!rngok(nID, kClrBase, kClrMax))
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
		
		pColors++;
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
	static int fillist[640];
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
	LINE2D* coords = (LINE2D*)malloc(sizeof(LINE2D)*sector[nSect].wallnum);
	if (coords)
	{
		getSectorWalls(nSect, &s, &e);
		while(s <= e)
		{
			getWallCoords(s++, &coords[i].x1, &coords[i].y1, &coords[i].x2, &coords[i].y2);
			i++;
		}
		
		FillPolygon(coords, i, c, nStep, drawPat);
		free(coords);
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
	if (pWall->cstat & kWallMoveForward)		nColor = kClrWalMveF;
	else if (pWall->cstat & kWallMoveReverse)	nColor = kClrWalMveR;
	else if (pWall->nextwall < 0)				nColor = kClrWalBase;
	else if (pWall->cstat & kWallHitscan)		nColor = kClrWalHscn;
	else										nColor = kClrWalNext;
	
	return ColorGet(nColor, hover);
}

char SCREEN2D::GetSpriteColor(spritetype* pSpr, char hover)
{
	char nColor = kClrSprBase;
	if (pSpr->cstat & kSprHitscan)			nColor = kClrSprHscn;
	if (pSpr->cstat & kSprInvisible)		nColor = kClrSprInvs;
	if (pSpr->cstat & kSprMoveForward)		nColor = kClrSprMveF;
	else if (pSpr->cstat & kSprMoveReverse)	nColor = kClrSprMveR;
	
	if (nColor == kClrSprHscn || nColor == kClrSprBase)
	{
		switch(pSpr->statnum)
		{
			case kStatDude:
				nColor = kClrSprDude;
				break;
			case kStatItem:
				nColor = kClrSprItem;
				break;
			case kStatFX:
			case kStatProjectile:
			case kStatExplosion:
				break;
			default:
				switch(pSpr->type)
				{
					case 24:
						if (!gModernMap) break;
					case kMarkerDudeSpawn:
						nColor = kClrSprDude;
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
			fc = ColorGet(kClrEdCaptWalFT);
			bc = ColorGet(kClrEdCaptWalFB);
			m[0] = 'F';
			break;
		case kMarkWallAlign:
			fc = ColorGet(kClrEdCaptWalAT);
			bc = ColorGet(kClrEdCaptWalAB);
			m[0] = 'A';
			break;
		default:
			return;
	}
	
	avePointWall(nWall, &x1, &y1);
	x1 = cscalex(x1); y1 = cscaley(y1);
	
	nnExtOffsetPos(0, 8, 0, GetWallAngle(nWall) + kAng90, &x1, &y1, NULL);
	if (prefs.showTags && CaptionGet(OBJ_WALL, nWall))
		nnExtOffsetPos(0, perc2val(300, pCaptFont->height), 0, 1536, &x1, &y1, NULL);
	
	CaptionPrint(m, x1, y1, 1, fc, (unsigned char)bc, false, qFonts[1]);
}

void SCREEN2D::CaptionPrintWallEdit(int nWall, char fc, char bc, QFONT* pFont)
{
	int x1, y1, x2, y2, cx, cy;
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	
	double nGrid, t;
	double nLen = approxDist(x2 - x1, y2 - y1);
	char buf1[32] = "\0", buf2[32] = "\0";
	int nNext, nAng;
	
	if (nLen > kMinWallLengthG)
	{
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
	}
	
	if (nLen > kMinWallLengthA && x1 != x2 && y1 != y2)
	{
		if (x1 > x2)
			swapValues(&x1, &x2);
		if (y1 > y2)
			swapValues(&y1, &y2);
		
		nAng = getangle(x2 - x1, y2 - y1);
		sprintf(buf2, "%dD", ((nAng & kAngMask)*360)/kAng360);
	}
	
	if (buf1[0] || buf2[0])
	{
		x1 = cscalex(x1);	x2 = cscalex(x2);
		y1 = cscaley(y1);	y2 = cscaley(y2);
		
		cx = x1+((x2-x1)>>1); cy = y1+((y2-y1)>>1); nNext = wall[nWall].nextwall;
		if (prefs.showTags && (CaptionGet(OBJ_WALL, nWall) || nNext >= 0 && CaptionGet(OBJ_WALL, nNext)))
			nnExtOffsetPos(0, perc2val(150, pCaptFont->height), 0, kAng90, &cx, &cy, NULL);
		
		// print length
		CaptionPrint(buf1, cx, cy, 1, fc, -1, true, pFont);
		nnExtOffsetPos(0, perc2val(110, pCaptFont->height), 0, kAng90, &cx, &cy, NULL);
		
		// print angle below
		t = qFonts[3]->charSpace, qFonts[3]->charSpace = 2;
		CaptionPrint(buf2, cx, cy, 1, fc, -1, true, qFonts[3]);
		qFonts[3]->charSpace = t;
	}
}

void SCREEN2D::DrawWallMidPoint(int nWall, char c, char s, char thick, char which)
{
	int nAng, nSize, nLen, x1, y1, x2, y2, cx, cy;
	
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	x1 = cscalex(x1);	x2 = cscalex(x2);
	y1 = cscaley(y1);	y2 = cscaley(y2);
	cx = (x1 + x2) >> 1;	cy = (y1 + y2) >> 1;
	
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
	short flags;
	
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	x1 = cscalex(x1); y1 = cscaley(y1);
	x2 = cscalex(x2); y2 = cscaley(y2);
	THICK = 0;
	
	if (nWall == linehighlight)
		color = GetWallColor(nWall, h);
	else
		color = GetWallColor(nWall, 0);

	if (nNext >= 0)
	{
		flags = wall[nNext].cstat;
		
		if (linehighlight >= 0 && nNext == linehighlight)
		{
			color = GetWallColor(nNext, h);
			flags = wall[nNext].cstat;
		}
		
		THICK = ((flags & kWallBlock) > 0);
	}
	
	DrawLine(x1, y1, x2, y2, color, THICK);

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
	
	return (t && t[0]) ? t : NULL;
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
	cx = cscalex(cx);
	cy = cscaley(cy);
	
	fc = ColorGet(kClrCaptSectT, hover);
	bc = ColorGet(kClrCaptSectB, hover);
	
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
		bc = ColorGet(kClrSprMveF, 	(hover) ? h : !bh);
		dark = !bh;
	}
	else if (flags & kSprMoveReverse)		bc = ColorGet(kClrSprMveR, 		(hover) ? h : !bh);
	else									bc = ColorGet(kClrSprBase, 		(hover) ? h :  bh);
		
	if (flags & kSprHitscan)				fc = ColorGet(kClrCaptSprTHscn, (hover && h));
	else if (dark)							fc = ColorGet(kClrCaptSprTMveF, (hover && h));
	else									fc = ColorGet(kClrCaptSprT, 	(hover && h));
	
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
	
	fc = ColorGet(kClrCaptWalT, hover && h);
	bc = ColorGet(kClrCaptWalB, hover && h);
	
	avePointWall(nWall, &cx, &cy);	
	cx = cscalex(cx);
	cy = cscaley(cy);
	
	pd = (hover) ? kCaptPadHover : kCaptPadNormal;
	CaptionPrint(caption, cx, cy, pd, fc, bc, false, pFont);
}

void SCREEN2D::SetView(int x1, int y1, int x2, int y2)
{
	view.wx1 = x1;	view.wy1 = y1;
	view.wx2 = x2;	view.wy2 = y2;
	
	view.wwh = view.wx2 - view.wx1;
	view.whg = view.wy2 - view.wy1;
	
	view.wcx = view.wx1 + (view.wwh >> 1);
	view.wcy = view.wy1 + (view.whg >> 1);
}

void SCREEN2D::ScreenClear()
{
	gfxSetColor(ColorGet(kClrEdBgd));
	gfxFillBox(view.wx1, view.wy1, view.wx2, view.wy2);
}

void SCREEN2D::ScreenDraw(void)
{
	XSECTOR* pXSect;
	int nSect, i, j, e, t;
	char fc, bc;
	
	x1 = x2 = x3 = 0;
	y1 = y2 = y3 = 0;
	
	spritetype* pHSpr = NULL;
	if (pointhighlight >= 0 && (pointhighlight & 0xc000))
		pHSpr = &sprite[pointhighlight & 16383];
	
	pCaptFont = qFonts[0];
	pEditFont = qFonts[(data.zoom <= kZoomEditSmallFont) ? 3 : 1];
	
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
			color = ColorGet(kClrSectFillHovr);
			FillSector(sectorhighlight, color, 1);
			LayerShowAndClose(kRSTransluc);
		}
		
		if (highlightsectorcnt > 0 || joinsector[0] >= 0)
		{
			LayerOpen();
			color = ColorGet(kClrSectFillHglt);
			
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
				color = ColorGet(kClrSectFillMerg);
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
			color = ColorGet(kClrSectFillHglt);
			i = highlightsectorcnt;
			while(--i >= 0)
				FillSector(highlightsector[i], color);
		}
		
		// flood sector waiting for merging
		if (joinsector[0] >= 0)
		{
			color = ColorGet(kClrSectFillMerg);
			FillSector(joinsector[0], color);
		}
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
			x1 = cscalex(x1); y1 = cscaley(y1);
			DrawIconCross(x1, y1, GetWallColor(linehighlight, false), 2);
		}
	}
	

	
	/* draw walls */
	////////////////////////////
	fc = ColorGet(kClrVtxBord);
	bc = ColorGet(kClrVtxFill);
	for (i = 0; i < numwalls; i++)
	{
		if (wall[i].nextwall > i || wall[i].point2 >= numwalls)
			continue;
		
		DrawWall(i);
		if (data.zoom >= kZoomVertexAll && vertexSize)
		{
			if (OnScreen(x1, y1, vertexSize))	DrawVertex(x1, y1, fc, bc, vertexSize);
			if (OnScreen(x2, y2, vertexSize))	DrawVertex(x2, y2, fc, bc, vertexSize);
		}
	}
	
	/* draw sector highlight box */
	////////////////////////////
	if (highlightsectorcnt > 0)
	{
		hgltSectGetBox(&x1, &y1, &x2, &y2);
		if (irngok(mousxplc, x1, x2) && irngok(mousyplc, y1, y2))
		{
			x1 = cscalex(x1);	x2 = cscalex(x2);
			y1 = cscaley(y1);	y2 = cscaley(y2);
			DrawRect(x1, y1, x2, y2, ColorGet(kClrEdHgltSectBox), 0, kPatDashed);
		}
	}
	
	/* redraw highlighted wall points */
	////////////////////////////
	if (h && data.zoom >= kZoomVertexHglt && vertexSize)
	{
		fc = ColorGet(kClrVtxBord, h);
		bc = ColorGet(kClrVtxFill, h);
		
		for (i = 0; i < numwalls; i++)
		{
			x1 = cscalex(wall[i].x);
			y1 = cscaley(wall[i].y);
			if (OnScreen(x1, y1, vertexSize))
			{
				if (highlightcnt <= 0 || !TestBitString(show2dwall, i))
				{
					if (!InHighlight(x1, y1))
						continue;
				}
				
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
						color = ColorGet(kClrEdTracerSlide, HOVER);
						DrawArrow(x1, y1, x2, y2, color, data.zoom, kAng30, 0, kPatNormal);
						
						/*if (sectorHasMoveRObjects(pSpr->owner))
						{
							RotatePoint(&x2, &y2, kAng180, x1, y1);
							color = ColorGet(kClrTracerSlideR, HOVER);
							DrawArrow(x1, y1, x2, y2, color, data.zoom, kAng30, 0, kPatDotted);
						}*/
					}
					break;
				case kMarkerWarpDest:
					if (pSpr->owner != pSpr->sectnum)
					{
						avePointSector(pSpr->owner, &x2, &y2);
						if (!HOVER)
							HOVER = (pHSpr == pSpr && h);
						
						x2 = cscalex(x2);
						y2 = cscaley(y2);
						color = ColorGet(kClrEdTracerWarp, HOVER);
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
						FillSector(i, ColorGet(kClrPrvSectOff + pXSect->state));
				}
				else if ((pXSect->bobFloor || pXSect->bobCeiling) && (pXSect->bobAlways || pXSect->state))
				{
					FillSector(i, ColorGet(kClrPrvSectMotFx));
				}
			}
		}
	}
		
	/* sector drawing process */
	////////////////////////////
	if (newnumwalls > numwalls)
	{
		color = ColorGet(kClrEdWalNew, h);
		i = newnumwalls;
		
		while(--i >= numwalls)
		{
			getWallCoords(i, &x1, &y1, &x2, &y2);
			x1 = cscalex(x1);	x2 = cscalex(x2);
			y1 = cscaley(y1);	y2 = cscaley(y2);
			
			DrawLine(x1, y1, x2, y2, color);
			DrawWallMidPoint(i, color, 0x02);
			if (i == newnumwalls - 1 && x1 != x2 && y1 != y2)
				DrawLine(x1, y1, x2, y2, ColorGet(kClrEdWallAngled), 0, kPatDotted);

			if (data.zoom >= kZoomEditCaption)
			{
				fc = ColorGet(kClrEdCaptWalNewT);
				bc = ColorGet(kClrEdCaptWalNewB);
				CaptionPrintWallEdit(i, fc, (unsigned char)bc, pEditFont);
			}
		}
		
		// draw first point
		DrawVertex(x1, y1, ColorGet(kClrEdFirstVtxBrd), ColorGet(kClrEdFirstVtxBgd), 6);
		
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
				CaptionPrintWall(i, TestBitString(show2dwall, i), pCaptFont);
			
			if (prefs.showSprites)
			{
				for (i = 0; i < numsectors; i++)
				{
					for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
						CaptionPrintSprite(j, TestBitString(show2dsprite, j), pCaptFont);
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
	DrawArrow(x1, y1, ColorGet(kClrEdStartPos), startang, data.zoom, (int)80, (char)true);
	
	/* draw current position */
	////////////////////////////
	x1 = cscalex(posx); y1 = cscaley(posy);
	DrawArrow(x1, y1, ColorGet(kClrEdCurPos), data.ang, data.zoom);
	
	/* draw highlight */
	////////////////////////////
	if (prefs.showHighlight && hgltType)
	{
		x1 = hgltx1;	x2 = hgltx2;
		y1 = hglty1;	y2 = hglty2;
		
		switch(hgltType)
		{
			case kHgltPoint:
				fc = ColorGet(kClrEdHgltPointB);
				bc = ColorGet(kClrEdHgltPointF);
				break;
			default:
				fc = ColorGet(kClrEdHgltSectB);
				bc = ColorGet(kClrEdHgltSectF);
				break;
		}
		
		
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
				color = ColorGet(kClrEdSprCountUnder);
				sprintf(buffer, "[%d]", data.sprUnderCursor);
				
				i = gfxGetTextLen(buffer, pFont);
				CaptionPrint(buffer, searchx + i, searchy + pFont->height, 0, color, -1, true, pFont);
			}
		}
		/* show info for walls that you drag */
		////////////////////////////
		else if (pointdrag >= 0 && data.zoom >= kZoomEditCaption)
		{
			fc = ColorGet(kClrEdCaptWalNewT);
			bc = ColorGet(kClrEdCaptWalNewB);
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
		
		/* draw RX/TX tracker */
		////////////////////////////
		if (CXTracker::HaveObjects())
			CXTracker::Draw(this);
		
		/* draw map notes */
		////////////////////////////
		if (gCmtPrefs.enabled)
			gCommentMgr.Draw(this);
	}
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
			DrawLine(x1, y1, x2, y2, ColorGet(kClrEdTracerPath, BLINK), HOVER, kPatDotted);
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
		
		c = ColorGet(kClrSprMarkPath, BLINK);
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
	else if (highlightcnt > 0 && TestBitString(show2dsprite, pSpr->index)) HOVER = 1;
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
				while(i < 2)
				{
					dx[i] = cscalex(dx[i]);
					dy[i] = cscaley(dy[i]);
					i++;
				}
				
				DrawLine(dx[0], dy[0], dx[1], dy[1], color, THICK, kPatDotted);
				break;
			case kSprFloor:
			case kSprSloped:
				if (HOVER || data.zoom >= 1280 || pointdrag >= 0)
				{
					GetSpriteExtents(pSpr, &dx[0], &dy[0], &dx[1], &dy[1], &dx[2], &dy[2], &dx[3], &dy[3], 0x0);
					while(i < 4)
					{
						dx[i] = cscalex(dx[i]);
						dy[i] = cscaley(dy[i]);
						i++;
					}
					
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
			if (pSpr->owner == sectorhighlight) c = kClrSprMarkOwn;
			else c = kClrSprMark;
				
			c = ColorGet(c, BLINK);
			DrawIconFaceSpr_SMALL(x1, y1, c);
			if (pSpr->ang)
				DrawAngLine(128, pSpr->ang, x1, y1, c);
			
			break;
		case kMarkerAxis:
		case kMarkerWarpDest:
			if (pSpr->owner == sectorhighlight)		c = kClrSprMarkOwn;
			else if (pSpr->type == kMarkerWarpDest) c = kClrSprMarkWarp;
			else c = kClrSprMark;
				
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
		c = ColorGet(kClrSprAmbOff + (pXSpr && pXSpr->state), BLINK);
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
			
			c = (pXSpr->state) ? kClrEdAmbRad2On : kClrEdAmbRad2Off;
			
			i = 0;
			while(i < 2)
			{
				if (r[i])
					DrawCircle(x1, y1, r[i], ColorGet(c + i));
				
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
			c = ColorGet(more ? kClrSprStealMore : kClrSprStealLess);
			
			if (pXSpr->data1 > 0)
				DrawCircle(x1, y1, mulscale10(pXSpr->data1, data.zoom), c, false, kPatDashed);
			else if (HOVER)
				FillSector(pSpr->sectnum, c);
			
			if (OnScreen(x1, y1, 4))
			{
				c = ColorGet(more ? kClrSprStealMore : kClrSprStealLess, BLINK);
				DrawIconFaceSpr_NORMAL(x1, y1, c);
				DrawIconCross(x1, y1, c, 4);
			}
			
			return;
		}
	}
	
	DrawSpriteCommon();
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
		c = ColorGet(kClrEdExpRad, h);
		DrawCircle(x1, y1, r, c);
		DrawIconCross(x1, y1, c, 4);
		return;
	}
}

void SCREEN2D::DrawSpriteProjectile(void)
{
	char c = ColorGet((h) ? kClrSprMissile1 : kClrSprMissile2, HOVER);
	DrawCircle(x1, y1, mulscale12(pSpr->clipdist, data.zoom), c);
	DrawIconFaceSpr_SMALL(x1, y1, c);
	DrawIconCross(x1, y1, c, 1);
}

void SCREEN2D::DrawSpritePlayer(void)
{
	char c = kClrSprPly1;
	if (pXSpr)
		c += ClipHigh(pXSpr->data1, 8);
	
	c = ColorGet(c, BLINK);
	DrawArrow(x1, y1, c, (int)pSpr->ang, (int)data.zoom, (int)96, (char)(pSpr->type == kMarkerSPStart));
}

void SCREEN2D::DrawSpriteCommon(void)
{
	char onScreen = OnScreen(x1, y1, 8);
	switch(pSpr->cstat & kSprRelMask)
	{
		case kSprWall:
			if (onScreen)
			{
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
			}
			break;
		case kSprFloor:
		case kSprSloped:
			if (onScreen)
			{
				DrawAngLine(200, pSpr->ang, x1, y1, color, THICK);
				DrawSquare(x1, y1, color, 3 + THICK);
			}
			break;
		default:
			if (onScreen)
			{
				DrawAngLine(80, pSpr->ang, x1, y1, color, THICK);
				if (THICK) DrawIconFaceSpr_NORMAL(x1, y1, color);
				else DrawIconFaceSpr_SMALL(x1, y1, color);
			}
			break;
	}
}

void SCREEN2D::DrawGrid()
{
	int i, cx, cy, xp, yp;
	int dst;
	
	gfxSetColor(ColorGet(kClrEdGrd));
	x1 = cscalex(-boardWidth);	y1 = cscaley(-boardHeight);
	x2 = cscalex(boardWidth);	y2 = cscaley(boardHeight);
	
	if (gridSize > 1)
	{
		cx = data.camx, cy = data.camy;
		doGridCorrection(&cx, &cy, data.grid);
		dst = 2048 >> data.grid;
		
		GetPoint(view.wx1, view.wy1, &xp, &yp, 1);
		i = cx; while(i >= xp) gfxVLine(cscalex(i), y1, y2), i-=dst;
		i = cy; while(i >= yp) gfxHLine(cscaley(i), x1, x2), i-=dst;
		
		GetPoint(view.wx2, view.wy2, &xp, &yp, 1);
		i = cx; while(i <= xp) gfxVLine(cscalex(i), y1, y2), i+=dst;
		i = cy; while(i <= yp) gfxHLine(cscaley(i), x1, x2), i+=dst;
		
		return;
	}
	
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
	//int bwx1 = windowx1; windowx1 = view.wx1;
	//int bwx2 = windowx2; windowx2 = view.wx2;
	//int bwy1 = windowy1; windowy1 = view.wy1;
	//int bwy2 = windowy2; windowy2 = view.wy2;
	
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
	
	//windowx1 = bwx1;	windowx2 = bwx2;
	//windowy1 = bwy1;	windowy2 = bwy2;
}