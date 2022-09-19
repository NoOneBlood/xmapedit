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
#include "xmpmisc.h"
extern "C" {
#include "a.h"
}

#define kCharReplace 0x3F
int clipX0 = 0, clipY0 = 0, clipX1 = 320, clipY1 = 200;
int gColor = 0, xscale = 0, xstep = 0, yscale = 320, ystep = 200;
ROMFONT vFonts[kMaxVFonts];
QFONT* qFonts[kMaxQFonts];
QBITMAP* pBitmaps[kMaxBitmaps];

Rect clipRect(clipX0, clipY0, clipX1, clipY1);

// cyrillic symbols makes text printing crash
inline BOOL charOk(char ch) { return (ch >= 32 && ch <= 126); }

void FASTCALL gfxBlitM2V(char* src, int bpl, int width, int height, int x, int y)
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

void FASTCALL gfxBlitMT2V(char* src, char tc, int bpl, int width, int height, int x, int y)
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

void FASTCALL gfxBlitMono(char *src, char mask, int bpl, int width, int height, int x, int y)
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

void FASTCALL gfxDrawBitmap(QBITMAP *qbm, int x, int y)
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

void FASTCALL gfxDrawBitmap(int id, int x, int y) {
	
	gfxDrawBitmap(pBitmaps[id],x ,y);
	
}

void FASTCALL gfxPixel(int x, int y)
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

void FASTCALL gfxHLine(int y, int x0, int x1)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	if (y < clipRect.y0 || y >= clipRect.y1)
		return;

	x0 = ClipLow(x0, clipRect.x0);
	x1 = ClipHigh(x1, clipRect.x1-1);
	if (x0 <= x1)
	{
		begindrawing();
		char* dest = (char*)frameplace+ylookup[y]+x0;
		memset(dest, gColor, x1-x0+1);
		enddrawing();
	}
}

void FASTCALL gfxVLine(int x, int y0, int y1)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	if (x < clipRect.x0 || x >= clipRect.x1)
		return;

	y0 = ClipLow(y0, clipRect.y0);
	y1 = ClipHigh(y1, clipRect.y1-1);
	if (y0 <= y1)
	{
		begindrawing();
		char* dest = (char*)frameplace+ylookup[y0]+x;
		for (register int i = 0; i < y1-y0+1; i++, dest += ylookup[1])
			*dest = (char)gColor;
		enddrawing();
	}
}

void FASTCALL gfxFillBox(int x0, int y0, int x1, int y1)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif
	
	Rect box(x0, y0, x1, y1); box &= clipRect;
	if (box.isEmpty())
		return;
	
	
	begindrawing();
	char* dest = (char*)frameplace+ylookup[y0]+x0;
	for (register int i = 0; i < box.y1-box.y0; i++, dest += ylookup[1])
		memset(dest, gColor, box.x1-box.x0);
	enddrawing();
	
}

void FASTCALL gfxSetClip(int x0, int y0, int x1, int y1)
{
	clipRect.x0 = x0;
	clipRect.y0 = y0;
	clipRect.x1 = x1;
	clipRect.y1 = y1;

	clipX0 = x0 << 8;
	clipY0 = y0 << 8;
	clipX1 = (x1 << 8)-1;
	clipY1 = (y1 << 8)-1;
}

void FASTCALL printext2(int x, int y, char fr, char* text, ROMFONT* pFont, char flags)
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

int FASTCALL gfxGetTextLen(char * pzText, QFONT *pFont, int a3)
{
	if (!pFont)
		return strlen(pzText)*8;
	
	char c;
	register int nLength = -pFont->charSpace;
	if (a3 <= 0)
		a3 = strlen(pzText);
	
	for (char* s = pzText; *s != 0 && a3 > 0; s++, a3--)
	{
		c = 0;
		if (!charOk(*s))
			c = *s, *s = kCharReplace;
		
	   nLength += pFont->info[*s].ox+pFont->charSpace;
	   if (c)
		   *s = c;
	}
	
	return nLength;
}

int FASTCALL gfxGetLabelLen(char *pzLabel, QFONT *pFont)
{
	char c;
	register int nLength = 0;
	if (pFont)
		nLength = -pFont->charSpace;

	for (char* s = pzLabel; *s != 0; s++)
	{
		c = 0;
		if (!charOk(*s))
			c = *s, *s = kCharReplace;
		
		if (*s == '&') continue;
		else if (!pFont) nLength += 8;
		else nLength += pFont->info[*s].ox+pFont->charSpace;
		
		if (c)
			*s = c;
	}
	return nLength;
}

int FASTCALL gfxFindTextPos(char *pzText, QFONT *pFont, int a3)
{
	if (!pFont)
	{
		return a3 / 8;
	}
	char c;
	register int nLength = -pFont->charSpace;
	register int pos = 0;

	for (char* s = pzText; *s != 0; s++, pos++)
	{
		c = 0;
		if (!charOk(*s))
			c = *s, *s = kCharReplace;
		
		nLength += pFont->info[*s].ox+pFont->charSpace;
		
		if (c)
			*s = c;
		
		if (nLength > a3)
			break;
	}
	return pos;
}

void FASTCALL gfxDrawText(int x, int y, int color, char* pzText, QFONT* pFont, bool label)
{
	#if USE_POLYMOST
		if (getrendermode() >= 3)
			return;
	#endif

	if (!pzText)
		return;
	
	char c;
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
	
	for (char* s = pzText; *s != 0; s++)
	{
		c = 0;
		if (!charOk(*s))
			c = *s, *s = kCharReplace;
		
		if (label && *s == '&')
		{
			underline = true;
			continue;
		}

		QFONTCHAR* pChar = &pFont->info[*s];
		if (!pChar)
		{
			if (c)
				*s = c;
			
			continue;
		}
		
		Rect rect1(x, y+pChar->oy, x+pChar->w, y+pChar->oy+pChar->h);

		rect1 &= clipRect;

		if (!rect1.isEmpty())
		{
			Rect rect2 = rect1;
			rect2.offset(-x, -(y+pChar->oy));

			switch (pFont->type) {
				case kFontTypeMono:
					gfxBlitMono(&pFont->data[pChar->offset + (rect2.y0/8) * pChar->w+ rect2.x0], 1<<(rect2.y0&7), pChar->w,
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
		if (c)
			*s = c;
		
		underline = false;
	}
}

void FASTCALL gfxDrawText(int x, int y, int fr, int bg, char* txt, QFONT* pFont, bool label)
{
	register int len = gfxGetTextLen(txt, pFont);
	register int heigh = (pFont) ? pFont->height-2 : 8;
	gfxSetColor(bg);
	gfxFillBox(x-1, y-1, x+len+1, y+heigh+1);
	gfxDrawText(x, y, fr, txt, pFont, label);
}

void FASTCALL gfxDrawLabel(int x, int y, int color, char* pzLabel, QFONT* pFont)
{
	gfxDrawText(x, y, color, pzLabel, pFont, true);
}

void FASTCALL gfxSetColor(char color) { gColor = color; }




void FASTCALL viewDrawChar( QFONT *pFont, BYTE c, int x, int y, BYTE *pPalookup ) {
	
#if USE_POLYMOST
	if (getrendermode() >= 3)
		return;
#endif
	dassert(pFont != NULL);

	register int i, cx, cy, sizeX, sizeY;
	QFONTCHAR *pInfo = &pFont->info[c];
	if (!pInfo || !pInfo->w || !pInfo->h)
		return;

	
	y += pFont->baseline + pInfo->oy;
	x = mulscale16(x, xscale);
	y = mulscale16(y, yscale);
	sizeX = mulscale16(pInfo->w, xscale);
	sizeY = mulscale16(pInfo->h, yscale);
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
		vince[i] = ystep;
	}

	BYTE *p = (BYTE*)(frameplace + ylookup[dest.y0] + dest.x0);

	x = dest.x0;
	register int u = 0;

	while (x < dest.x1 && (x & 3) )
	{
		BYTE *bufplc = (BYTE *)(pSource + (u >> 16) * pInfo->h);
		mvlineasm1(ystep, pPalookup, sizeY - 1, 0, bufplc, p);
		p++;
		x++;
		u += xstep;
	}

	while ( x + 3 < dest.x1 )
	{
		for (i = 0; i < 4; i++)
		{
			vplce[i] = 0;
			bufplce[i] = (intptr_t)(pSource + (u >> 16) * pInfo->h);
			u += xstep;
		}
		mvlineasm4(sizeY, (char*)p);
		p += 4;
		x += 4;
	}

	while ( x < dest.x1 )
	{
		BYTE *bufplc = (BYTE *)(pSource + (u >> 16) * pInfo->h);
		mvlineasm1(ystep, pPalookup, sizeY - 1, 0, bufplc, p);
		p++;
		x++;
		u += xstep;
	}
}

void FASTCALL viewDrawText(int x, int y, QFONT* pFont, char *string, int shade, int nPLU, int nAlign) {

#if USE_POLYMOST
	if (getrendermode() >= 3)
		return;
#endif
	
	char *s, c;
	dassert(string != NULL);
	BYTE *pPalookup = (BYTE*)(palookup[nPLU] + (qgetpalookup(nPLU, shade) << 8));
	setupmvlineasm(16);

	if ( nAlign != 0 )
	{
		register int nWidth = -pFont->charSpace;
		for ( s = string; *s; s++ )
			nWidth += pFont->info[*s].ox + pFont->charSpace;

		if ( nAlign == 1 )
			nWidth >>= 1;

		x -= nWidth;
	}

	for ( s = string; *s; s++ )
	{
		c = 0;
		if (!charOk(*s))
			c = *s, *s = kCharReplace;
		
		viewDrawChar(pFont, *s, x, y, pPalookup);
		x += pFont->info[*s].ox + pFont->charSpace;
		
		if (c)
			*s = c;
	}
}
