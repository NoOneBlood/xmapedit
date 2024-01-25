/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
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
#include "common_game.h"
#include "gfx.h"
#include "replace.h"
#include "gui.h"
#include "tile.h"
#include "xmpmisc.h"
extern "C" {
#include "a.h"
}

int clip[4][4];
int clipCnt = 0;
Rect clipRect(0, 0, 0, 0);
ROMFONT vFonts[kMaxVFonts];
QFONT* qFonts[kMaxQFonts];
QBITMAP* pBitmaps[kMaxBitmaps];
char gColor = 0, gStdColor[32];

static const char* gStdColorNames[] =
{
"Black"				,	// kColorBlack
"Blue"				,	// kColorBlue
"Green"				,	// kColorGreen
"Cyan"				,	// kColorCyan
"Red"				,	// kColorRed
"Magenta"			,	// kColorMagenta
"Brown"				,	// kColorBrown
"LightGray"			,	// kColorLightGray
"DarkGray"			,	// kColorDarkGray
"LightBlue"			,	// kColorLightBlue
"LightGreen"		,	// kColorLightGreen
"LightCyan"			,	// kColorLightCyan
"LightRed"			,	// kColorLightRed
"LightMagenta"		,	// kColorLightMagenta
"Yellow"			,	// kColorYellow
"White"				,	// kColorWhite
"Grey16"			,	// Some additional colors...
"Grey17"			,
"Grey18"			,
"Grey19"			,
"Grey20"			,
"Grey21"			,
"Grey22"			,
"Grey23"			,
"Grey24"			,
"Grey25"			,
"Grey26"			,
"Grey27"			,
"Grey28"			,
"Grey29"			,
"Grey30"			,
"Grey31"			,
};

int gfxGetStdColor(const char* str)
{
	for (int i = 0; i < LENGTH(gStdColorNames); i++)
	{
		const char* pColor = gStdColorNames[i];
		if (Bstrcasecmp(str, pColor) == 0)
			return i;
	}
	
	return -1;
};

void gfxBlitM2V(char* src, int bpl, int width, int height, int x, int y)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	begindrawing();
	char* dest = (char*)frameplace+ylookup[y]+x;
	register int i = height;
	do
	{
		memcpy(dest, src, width);
		src += bpl;
		dest += ylookup[1];
	} while (--i);
	enddrawing();
}

void gfxBlitMT2V(char* src, char tc, int bpl, int width, int height, int x, int y)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	begindrawing();
	char* dest = (char*)frameplace+ylookup[y]+x;
	register int i = height;
	do
	{
		register int j = width;
		do
		{
			if (*src != tc)
				*dest = *src;
			src++;
			dest++;
		} while (--j);
		src += bpl-width;
		dest += ylookup[1]-width;
	} while (--i);
	enddrawing();
}

void gfxBlitMono(char *src, char mask, int bpl, int width, int height, int x, int y)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	begindrawing();
	char* dest = (char*)frameplace+ylookup[y]+x;
	register int i = height;
	do
	{
		register int j = width;
		do
		{
			if (*src&mask)
				*dest = (char)gColor;
			src++;
			dest++;
		} while (--j);
		src -= width;
		dest += ylookup[1]-width;
		if (mask&0x80)
		{
			mask = 1;
			src += bpl;
		}
		else
			mask <<= 1;
	} while (--i);
	enddrawing();
}

void gfxDrawBitmap(QBITMAP *qbm, int x, int y)
{
	dassert(qbm != NULL);
	Rect bitmap(x, y, x+qbm->width, y+qbm->height);
	bitmap &= clipRect;

	if (!bitmap)
		return;

	Rect bitmap2 = bitmap;

	bitmap2.offset(-x, -y);

	register int height = bitmap.height();
	register int width = bitmap.width();

	char* p = qbm->data;
	
	switch (qbm->type)
	{
		case 0:
			gfxBlitM2V(p+bitmap2.y0*qbm->bpl+bitmap2.x0, qbm->bpl, width, height, bitmap.x0, bitmap.y0);
			break;
		case 1:
			gfxBlitMT2V(p+bitmap2.y0*qbm->bpl+bitmap2.x0, qbm->tcolor, qbm->bpl, width, height, bitmap.x0, bitmap.y0);
			break;
	}
}

void gfxDrawBitmap(int id, int x, int y)
{
	gfxDrawBitmap(pBitmaps[id],x ,y);
}

void gfxPixel(int x, int y)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	if (clipRect.inside(x, y))
	{
		begindrawing();
		char* dest = (char*)frameplace+ylookup[y]+x;
		*dest = (char)gColor;
		enddrawing();
	}
}

void gfxHLine(int y, int x0, int x1)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	if (!irngok(y, clipRect.y0, clipRect.y1))
		return;
	
	x0 = ClipLow(x0, clipRect.x0);
	x1 = ClipHigh(x1, clipRect.x1);
	while(x0 <= x1)
		gfxPixel(x0++, y);
}

void gfxVLine(int x, int y0, int y1)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif

	if (!irngok(x, clipRect.x0, clipRect.x1))
		return;
	
	y0 = ClipLow(y0, clipRect.y0);
	y1 = ClipHigh(y1, clipRect.y1);
	while(y0 <= y1)
		gfxPixel(x, y0++);
	
}


// NOTE: this is drawline16 adapted for gfx
// JBF: Had to add extra tests to make sure x-coordinates weren't winding up -'ve
//   after clipping or crashes would ensue
void gfxLine(int x1, int y1, int x2, int y2)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	unsigned int drawpat = (drawlinepat != kPatNormal) ? drawlinepat : 0;
	int i = 0, t, dx, dy;
	unsigned int c = 0;
	
	dx = x2-x1; dy = y2-y1;

	if (dx >= 0)
	{
		if ((x1 >= xres) || (x2 < 0)) return;
		if (x1 < 0) { if (dy) y1 += scale(0-x1,dy,dx); x1 = 0; }
		if (x2 >= xres) { if (dy) y2 += scale(xres-1-x2,dy,dx); x2 = xres-1; }
	}
	else
	{
		if ((x2 >= xres) || (x1 < 0)) return;
		if (x2 < 0) { if (dy) y2 += scale(0-x2,dy,dx); x2 = 0; }
		if (x1 >= xres) { if (dy) y1 += scale(xres-1-x1,dy,dx); x1 = xres-1; }
	}
	if (dy >= 0)
	{
		if ((y1 >= yres) || (y2 < 0)) return;
		if (y1 < 0) { if (dx) x1 += scale(0-y1,dx,dy); y1 = 0; if (x1 < 0) x1 = 0; }
		if (y2 >= yres) { if (dx) x2 += scale(yres-1-y2,dx,dy); y2 = yres-1; if (x2 < 0) x2 = 0; }
	}
	else
	{
		if ((y2 >= yres) || (y1 < 0)) return;
		if (y2 < 0) { if (dx) x2 += scale(0-y2,dx,dy); y2 = 0; if (x2 < 0) x2 = 0; }
		if (y1 >= yres) { if (dx) x1 += scale(yres-1-y1,dx,dy); y1 = yres-1; if (x1 < 0) x1 = 0; }
	}
	
	if (!drawpat)
	{
		if (x1 == x2)
		{
			if (y2 < y1)
				swapValues(&y1, &y2);
			
			gfxVLine(x1, y1, y2);
			return;
		}
		
		if (y1 == y2)
		{
			if (x2 < x1)
				swapValues(&x1, &x2);
			
			gfxHLine(y1, x1, x2);
			return;
		}
	}
	
	dx = klabs(dx)+1;
	dy = klabs(dy)+1;
	

	if (dx >= dy)
	{
		if (x2 < x1)
		{
			swapValues(&x1, &x2);
			swapValues(&y1, &y2);
		}
				
		t = (y2 > y1) ? 1 : -1;
		while(x1 <= x2)
		{
			if (!drawpat || (drawpat & pow2long[(c++) & 31]))
				gfxPixel(x1, y1);

			i += dy;
			if (i >= dx)
			{
				i -= dx;
				y1 += t;
			}
			
			x1++;
		}
		return;
	}


	if (y2 < y1)
	{
		swapValues(&x1, &x2);
		swapValues(&y1, &y2);
	}
	
	t = (x2 > x1) ? 1 : -1;
	while(y1 <= y2)
	{
		if (!drawpat || (drawpat & pow2long[(c++) & 31]))
			gfxPixel(x1, y1);

		i += dx;
		if (i >= dy)
		{
			i -= dy;
			x1 += t;
		}
		
		y1++;
	}

}

void gfxRect(int x1, int y1, int x2, int y2)
{
	gfxHLine(y1, x1, x2);
	gfxHLine(y2, x1, x2);
	
	gfxVLine(x1, y1, y2);
	gfxVLine(x2, y1, y2);
}

void gfxFillBox(int x0, int y0, int x1, int y1)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	x0 = ClipRange(x0, clipRect.x0, clipRect.x1);
	x1 = ClipRange(x1, clipRect.x0, clipRect.x1);
	y0 = ClipRange(y0, clipRect.y0, clipRect.y1);
	y1 = ClipRange(y1, clipRect.y0, clipRect.y1);
	
	begindrawing();
	char* dest = (char*)frameplace+ylookup[y0]+x0;
	register int hg = y1 - y0, wh = x1 - x0;
	while(--hg >= 0)
	{
		memset(dest, gColor, wh);
		dest+=ylookup[1];
	}
	enddrawing();
	
}

void gfxFillBoxTrans(int x1, int y1, int x2, int y2, char color, char transLev)
{
	char flags = kRSCorner | kRSNoMask | kRSNoClip;
	static int nTile = -1; int t, wh, hg;
	BYTE* pTile;
	
	if (nTile >= 0 || (nTile = tileGetBlank()) >= 0)
	{
		if (x1 > x2) swapValues(&x1, &x2);
		if (y1 > y2) swapValues(&y1, &y2);

		wh = x2-x1, hg = y2-y1;
		if (tilesizx[nTile] == wh && tilesizy[nTile] == hg)
		{
			if ((pTile = tileLoadTile(nTile)) == NULL)
				return;
		}
		else
		{
			tileFreeTile(nTile);
			if ((pTile = tileAllocTile(nTile, wh, hg, 0, 0)) == NULL)
				return;
		}
		
		memset(pTile, color, wh*hg);
		flags |= ((transLev >= 2) ? kRSTransluc : kRSTransluc2);
		rotatesprite(x1 << 16, y1 << 16, 0x10000, 0, nTile, 0, 0, flags, x1, y1, x2, y2);
	}
}

void gfxSetClip(int x0, int y0, int x1, int y1)
{	
	clipRect.x0 = x0;
	clipRect.y0 = y0;
	clipRect.x1 = x1;
	clipRect.y1 = y1;
}

void gfxBackupClip()
{
	int* pClip = clip[clipCnt++];
	pClip[0]	= clipRect.x0;	pClip[2]	= clipRect.x1;
	pClip[1]	= clipRect.y0;	pClip[3]	= clipRect.y1;
}

void gfxRestoreClip()
{
	int* pClip = clip[--clipCnt];
	clipRect.x0 = pClip[0];		clipRect.x1 = pClip[2];
	clipRect.y0 = pClip[1];		clipRect.y1 = pClip[3];
}

void printext2(int x, int y, char fr, char* text, ROMFONT* pFont, char flags)
{
	register int i = 0, j, k, m, c, l, s;
	register int w = pFont->wh; s = pFont->ls;
	register int h = pFont->hg;
	BOOL shadow = (flags & 0x01);
	
	if (!pFont->data)
		return;
	

	gColor = fr;
	while(text[i])
	{
		for (j = 0; j < h; j++)
		{
			m = 0x80;
			for (k = 0; k < w; k++, m >>= 1)
			{
				c = pFont->data[text[i]*h+j];
				if (c & m)
				{
					if (shadow)
					{
						gColor = gStdColor[31];
						gfxPixel(x+k+1, y+j+1);
						gColor = fr;
					}
					
					gfxPixel(x+k, y+j);
				}
			}
		}
		
		i++, x+=s;
	}
}

int gfxGetTextLen(char * pzText, QFONT *pFont, int a3)
{
	if (!pFont)
		return strlen(pzText)*8;
	
	int nLength = -pFont->charSpace;
	if (a3 <= 0)
		a3 = strlen(pzText);
	
	for (uint8_t* s = (uint8_t*)pzText; *s != 0 && a3 > 0; s++, a3--)
	   nLength += pFont->info[*s].ox+pFont->charSpace;
   
	return nLength;
}

int gfxGetLabelLen(char *pzLabel, QFONT *pFont)
{
	int nLength = 0;
	if (pFont)
		nLength = -pFont->charSpace;

	for (uint8_t* s = (uint8_t*)pzLabel; *s != 0; s++)
	{
		if (*s == '&') continue;
		else if (!pFont) nLength += 8;
		else nLength += pFont->info[*s].ox+pFont->charSpace;
	}
	
	return nLength;
}

int gfxFindTextPos(char *pzText, QFONT *pFont, int a3)
{
	if (!pFont)
	{
		return a3 / 8;
	}
	
	int nLength = -pFont->charSpace;
	int pos = 0;
	for (uint8_t* s = (uint8_t*)pzText; *s != 0; s++, pos++)
	{
		nLength += pFont->info[*s].ox+pFont->charSpace;
		if (nLength > a3)
			break;
	}
	return pos;
}

void gfxDrawText(int x, int y, int color, char* pzText, QFONT* pFont, bool label)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif

	if (!pzText)
		return;
	
	bool underline = false;
	gfxSetColor(color);
	
	if (pFont != NULL)
	{
		if (pFont->type == kFontTypeRasterVert)
		{
			viewDrawText(x, y, pFont, pzText);
			return;
		}
		
		y += pFont->baseline;
	}
	else
	{
		printext2(x, y, color, pzText, &vFonts[0]);
		return;
	}
	
	for (uint8_t* s = (uint8_t*)pzText; *s != 0; s++)
	{
		if (label && *s == '&')
		{
			underline = true;
			continue;
		}

		QFONTCHAR* pChar = &pFont->info[*s];
		if (!pChar)
			continue;
		
		Rect rect1(x, y+pChar->oy, x+pChar->w, y+pChar->oy+pChar->h);

		rect1 &= clipRect;

		if (!rect1.isEmpty())
		{
			Rect rect2 = rect1;
			rect2.offset(-x, -(y+pChar->oy));

			switch (pFont->type) {
				case kFontTypeMono:
					gfxBlitMono(&pFont->data[pChar->offset + (rect2.y0/pChar->h) * pChar->w+ rect2.x0], 1<<(rect2.y0&7), pChar->w,
						rect1.x1-rect1.x0, rect1.y1-rect1.y0, rect1.x0, rect1.y0);
					break;
				case kFontTypeRasterHoriz:
					gfxBlitMT2V(&pFont->data[pChar->offset+rect2.y0*pChar->w+rect2.x0], pFont->charLast, pChar->w,
						rect1.x1-rect1.x0, rect1.y1-rect1.y0, rect1.x0, rect1.y0);
					break;
			}
			
			if (underline)
				gfxHLine(y + 2, x, x + pChar->h - 1);
		}
		
		x += pFont->charSpace + pChar->ox;
		underline = false;
	}
}

void gfxDrawTextRect(Rect** pARect, int flags, char fc, char* str, QFONT* pFont, int maxLines)
{
	Rect* pRect = *pARect;
	
	int wh = pRect->width(),	hg = pRect->height();
	int x1 = pRect->x0, 		y1 = pRect->y0;
	int x2 = pRect->x1,			y2 = pRect->y1;
	int dx = x1,				dy = y1;
	
	char shadow = ((flags & kTextShadow) > 0);
	
	QFONTCHAR* pChar;
	int c = '\n', g = 0, lineWidth;
	int nLines, fh = pFont->height;
	int i, j;

	i = j = 0;
	while(str[i])
	{
		pChar = &pFont->info[str[i++]];
		if (j + pChar->ox + pFont->charSpace >= wh)
			break;
		
		j += (pChar->ox + pFont->charSpace);
	}
		
	lineWidth = ClipLow(j, 1);
	nLines = ClipHigh(gfxGetTextLen(str, pFont) / lineWidth, maxLines-1);

	if (flags & kTextABottom)
	{
		dy = y2-(nLines*fh)-fh;
	}
	else if (flags & kTextAMiddle)
	{
		dy = y1+((hg>>1)-((nLines*fh) >> 1));
	}
	else
	{
		dy+=fh;
	}
	
	if (flags & kTextACenter)
	{
		pRect->x0 = pRect->x1 = x1+((wh>>1)-(lineWidth>>1));
	}
	else if (flags & kTextARight)
	{
		pRect->x0 = pRect->x1 = x1+(wh-g);
	}
	else
	{
		pRect->x0 = pRect->x1 = x1;
	}
	
	pRect->y0 = dy;
	
	i = j = nLines = 0;
	while(c != '\0')
	{
		c = str[i];
		pChar = &pFont->info[c];
		if ((c == '\0') || (nLines >= maxLines) || (g + pChar->ox + pFont->charSpace) >= wh)
		{
			if (flags & kTextACenter)		dx = x1+((wh>>1)-(g>>1));
			else if (flags & kTextARight)	dx = x1+(wh-g);
			
			if (dx < pRect->x0)
				pRect->x0 = dx;
			
			if (pRect->x0+g > pRect->x1)
				pRect->x1 = pRect->x0+g;
			
			if (c)
				str[i] = '\0';
			
			if (!(flags & kTextDryRun))
			{
				if (flags & kTextShadow)
					gfxDrawText(dx+1, dy+1, clr2std(31), &str[j], pFont);
				
				gfxDrawText(dx, dy, fc, &str[j], pFont);
			}
			
			if (c)
				str[i] = c;

			dy += fh;
			if (++nLines >= maxLines)
				break;
			
			g = 0;
			j = i;
		}
		else
		{
			g += (pChar->ox + pFont->charSpace);
		}
		
		i++;
	}
	
	pRect->y1 = dy;
}

void gfxDrawTextRect(Rect* pARect, int flags, char fc, char* str, QFONT* pFont, int maxLines)
{
	Rect* pDummy = new Rect(pARect->x0, pARect->y0, pARect->x1, pARect->y1);
	gfxDrawTextRect(&pDummy, flags, fc, str, pFont, maxLines);
}

void gfxGetTextRect(Rect** pARect, int flags, char fc, char* str, QFONT* pFont, int maxLines)
{
	gfxDrawTextRect(pARect, flags, fc, str, pFont, maxLines);
}


void gfxDrawText(int x, int y, int fr, int bg, char* txt, QFONT* pFont, bool label)
{
	register int len = gfxGetTextLen(txt, pFont);
	register int heigh = (pFont) ? pFont->height-2 : 8;
	gfxSetColor(bg);
	gfxFillBox(x-1, y-1, x+len+1, y+heigh+1);
	gfxDrawText(x, y, fr, txt, pFont, label);
}

void gfxDrawLabel(int x, int y, int color, char* pzLabel, QFONT* pFont)
{
	gfxDrawText(x, y, color, pzLabel, pFont, true);
}

void viewDrawChar( QFONT *pFont, BYTE c, int x, int y, BYTE *pPalookup )
{
	#define kScaleX		0x10000
	#define kScaleY		0x10000
	#define kStepX		0x10000
	#define kStepY		0x10000
	
	dassert(pFont != NULL);

	register int i, cx, cy, sizeX, sizeY;
	QFONTCHAR *pInfo = &pFont->info[c];
	if (!pInfo || !pInfo->w || !pInfo->h)
		return;

	
	y += pFont->baseline + pInfo->oy;
	x = mulscale16(x, kScaleX);
	y = mulscale16(y, kScaleY);
	sizeX = mulscale16(pInfo->w, kScaleX);
	sizeY = mulscale16(pInfo->h, kScaleY);
	cx = x + sizeX;
	cy = y + sizeY;

	if (!rngok(cy, 0, ydim) || !rngok(cx, 0, xdim))
		return;

	Rect dest(x, y, cx, cy);
	Rect screen(0, 0, xdim, ydim);
	dest &= screen;

	if ( !dest )
		return;

	char *pSource = &pFont->data[pInfo->offset];

	for (i = 0; i < 4; i++)
	{
		palookupoffse[i] = (intptr_t)pPalookup;
		vince[i] = kStepY;
	}

	BYTE *p = (BYTE*)(frameplace + ylookup[dest.y0] + dest.x0);

	x = dest.x0;
	register int u = 0;

	while (x < dest.x1 && (x & 3) )
	{
		BYTE *bufplc = (BYTE *)(pSource + (u >> 16) * pInfo->h);
		mvlineasm1(kStepY, pPalookup, sizeY - 1, 0, bufplc, p);
		p++;
		x++;
		u += kStepX;
	}

	while ( x + 3 < dest.x1 )
	{
		for (i = 0; i < 4; i++)
		{
			vplce[i] = 0;
			bufplce[i] = (intptr_t)(pSource + (u >> 16) * pInfo->h);
			u += kStepX;
		}
		mvlineasm4(sizeY, (char*)p);
		p += 4;
		x += 4;
	}

	while ( x < dest.x1 )
	{
		BYTE *bufplc = (BYTE *)(pSource + (u >> 16) * pInfo->h);
		mvlineasm1(kStepY, pPalookup, sizeY - 1, 0, bufplc, p);
		p++;
		x++;
		u += kStepX;
	}
}

void viewDrawText(int x, int y, QFONT* pFont, char *string, int shade, int nPLU, int nAlign) {

#if USE_POLYMOST
	if (getrendermode() >= 3)
		return;
#endif
	
	if (!string)
		return;
	
	uint8_t *s;
	BYTE *pPalookup = (BYTE*)(palookup[nPLU] + (qgetpalookup(nPLU, shade) << 8));
	setupmvlineasm(16);
	
	if ( nAlign != 0 )
	{
		int nWidth = -pFont->charSpace;
		for (s = (uint8_t*)string; *s; s++)
			nWidth += pFont->info[*s].ox + pFont->charSpace;

		if (nAlign == 1)
			nWidth >>= 1;

		x -= nWidth;
	}

	for (s = (uint8_t*)string; *s; s++)
	{
		viewDrawChar(pFont, *s, x, y, pPalookup);
		x += pFont->info[*s].ox + pFont->charSpace;
	}
}
